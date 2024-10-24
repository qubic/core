#pragma once
#ifdef NO_UEFI
unsigned long long top_of_stack;
#endif
#include "platform/memory.h"
#include "platform/m256.h"
#include "platform/concurrency.h"
#include "public_settings.h"
#include "score_cache.h"


////////// Scoring algorithm \\\\\\\\\\

#define NOT_CALCULATED -127 //not yet calculated
#define NULL_INDEX -2

#if !(defined (__AVX512F__) || defined(__AVX2__))
static_assert(false, "Either AVX2 or AVX512 is required.");
#endif

template<
    unsigned long long dataLength,
    unsigned long long numberOfHiddenNeurons,
    unsigned long long numberOfNeighborNeurons,
    unsigned long long maxDuration,
    unsigned long long solutionBufferCount
>
struct ScoreFunction
{
    static constexpr unsigned long long allNeuronsCount = dataLength + numberOfHiddenNeurons + dataLength;
    static constexpr unsigned long long computeNeuronsCount = numberOfHiddenNeurons + dataLength;
    static constexpr unsigned long long synapseSignsCount = (dataLength + numberOfHiddenNeurons + dataLength) * numberOfNeighborNeurons / 64;
    static constexpr unsigned long long synapseInputCount = synapseSignsCount + maxDuration;

    static_assert(allNeuronsCount < 0xFFFFFFFF, "Current implementation only support MAX_UINT32 neuron");
    static_assert(numberOfNeighborNeurons < 0x7FFFFFFF, "Current implementation only support MAX_UINT32 number of neighbors");
    static_assert((allNeuronsCount* numberOfNeighborNeurons) % 64 == 0, "numberOfNeighborNeurons * allNeuronsCount must dividable by 64");

    long long miningData[dataLength];

    struct K12EngineX1 {
        unsigned long long Aba, Abe, Abi, Abo, Abu;
        unsigned long long Aga, Age, Agi, Ago, Agu;
        unsigned long long Aka, Ake, Aki, Ako, Aku;
        unsigned long long Ama, Ame, Ami, Amo, Amu;
        unsigned long long Asa, Ase, Asi, Aso, Asu;
        unsigned long long scatteredStates[25];
        int leftByteInCurrentState;
        unsigned char* _pPoolBuffer;
        unsigned int _x;
    private:
        void _scatterFromVector() {
            copyToStateScalar(scatteredStates)
        }
        void hashNewChunk() {
            declareBCDEScalar
                rounds12Scalar
        }
        void hashNewChunkAndSaveToState() {
            hashNewChunk();
            _scatterFromVector();
            leftByteInCurrentState = 200;
        }
    public:
        K12EngineX1() {}
        void initState(const unsigned long long* comp_u64, const unsigned long long* nonce_u64, unsigned char* pPoolBuffer)
        {
            Aba = comp_u64[0];
            Abe = comp_u64[1];
            Abi = comp_u64[2];
            Abo = comp_u64[3];
            Abu = nonce_u64[0];
            Aga = nonce_u64[1];
            Age = nonce_u64[2];
            Agi = nonce_u64[3];
            Ago = Agu = Aka = Ake = Aki = Ako = Aku = Ama = Ame = Ami = Amo = Amu = Asa = Ase = Asi = Aso = Asu = 0;
            leftByteInCurrentState = 0;

            _x = 0;
            _pPoolBuffer = pPoolBuffer;
            write(_pPoolBuffer, RANDOM2_POOL_SIZE);
        }

        void write(unsigned char* out0, int size)
        {
            unsigned char* s0 = (unsigned char*)scatteredStates;
            if (leftByteInCurrentState) {
                int copySize = size < leftByteInCurrentState ? size : leftByteInCurrentState;
                copyMem(out0, s0 + 200 - leftByteInCurrentState, copySize);
                size -= copySize;
                leftByteInCurrentState -= copySize;
                out0 += copySize;
            }
            while (size) {
                if (!leftByteInCurrentState) {
                    hashNewChunkAndSaveToState();
                }
                int copySize = size < leftByteInCurrentState ? size : leftByteInCurrentState;
                copyMem(out0, s0 + 200 - leftByteInCurrentState, copySize);
                size -= copySize;
                leftByteInCurrentState -= copySize;
                out0 += copySize;
            }
        }

        unsigned int random2FromPrecomputedPool(unsigned char* output, unsigned long long outputSize)
        {
            for (unsigned long long i = 0; i < outputSize; i += 8)
            {
                *((unsigned long long*) & output[i]) = *((unsigned long long*) & _pPoolBuffer[_x & (RANDOM2_POOL_ACTUAL_SIZE - 1)]);
                _x = _x * 1664525 + 1013904223;// https://en.wikipedia.org/wiki/Linear_congruential_generator#Parameters_in_common_use
            }
            return _x;
        }

        void scatterFromVector() {
            _scatterFromVector();
        }
        void hashWithoutWrite(int size) {
            if (leftByteInCurrentState) {
                int copySize = size < leftByteInCurrentState ? size : leftByteInCurrentState;
                size -= copySize;
                leftByteInCurrentState -= copySize;
            }
            while (size) {
                if (!leftByteInCurrentState) {
                    hashNewChunk();
                    leftByteInCurrentState = 200;
                }
                int copySize = size < leftByteInCurrentState ? size : leftByteInCurrentState;
                size -= copySize;
                leftByteInCurrentState -= copySize;
            }
        }
    };

    struct PoolSynapseData
    {
        unsigned int neuronIndex;
        unsigned int supplierIndexWithSign;
    };

    struct computeBuffer {
        struct Neuron
        {
           char* input;
        };
        struct Synapse
        {
            // Pointer to data
            unsigned long long* signs;
        };
        K12EngineX1 k12;
        Neuron _neurons;
        Synapse _synapses;
        PoolSynapseData* _poolSynapseData;
        unsigned char* _poolRandom2Buffer;

    } *_computeBuffer = nullptr;
    m256i currentRandomSeed;

    volatile char solutionEngineLock[solutionBufferCount];

#if USE_SCORE_CACHE
    volatile char scoreCacheLock;
    ScoreCache<SCORE_CACHE_SIZE, SCORE_CACHE_COLLISION_RETRIES> scoreCache;
#endif

    void initMiningData(m256i randomSeed)
    {
        currentRandomSeed = randomSeed; // persist the initial random seed to be able to send it back on system info response
        if (!isZero(currentRandomSeed))
        {
            random((unsigned char*)&randomSeed, (unsigned char*)&randomSeed, (unsigned char*)miningData, sizeof(miningData));
            for (unsigned int i = 0; i < dataLength; i++)
            {
                miningData[i] = (miningData[i] >= 0 ? 1 : -1);
            }
        }
    }

    ~ScoreFunction()
    {
        freeMemory();
    }

    void freeMemory()
    {
        if (_computeBuffer)
        {
            for (unsigned int i = 0; i < solutionBufferCount; i++)
            {
                if (_computeBuffer[i]._poolRandom2Buffer)
                {
                    freePool(_computeBuffer[i]._poolRandom2Buffer);
                }

                if (_computeBuffer[i]._neurons.input)
                {
                    freePool(_computeBuffer[i]._neurons.input);
                    _computeBuffer[i]._neurons.input = nullptr;
                }
                if (_computeBuffer[i]._synapses.signs)
                {
                    freePool(_computeBuffer[i]._synapses.signs);
                    _computeBuffer[i]._synapses.signs = nullptr;
                }
            }

            freePool(_computeBuffer);
            _computeBuffer = nullptr;
        }
    }

    bool initMemory()
    {
        if (_computeBuffer == nullptr)
        {
            if (!allocatePool(sizeof(computeBuffer) * solutionBufferCount, (void**)&_computeBuffer))
            {
                logToConsole(L"Failed to allocate memory for score solution buffer!");
                return false;
            }

            for (int bufId = 0; bufId < solutionBufferCount; bufId++)
            {
                auto& cb = _computeBuffer[bufId];

                if (!allocatePool(RANDOM2_POOL_SIZE, (void**)&(cb._poolRandom2Buffer)))
                {
                    logToConsole(L"Failed to allocate memory for score pool buffer!");
                    return false;
                }

                if (!allocatePool(allNeuronsCount, (void**)&(cb._neurons.input)))
                {
                    CHAR16 log[256];
                    setText(log, L"Failed to allocate memory for neurons! Try to allocated ");
                    appendNumber(log, allNeuronsCount / 1024, true);
                    appendText(log, L" KB");
                    logToConsole(log);
                    return false;
                }

                if (!allocatePool(synapseSignsCount * sizeof(unsigned long long), (void**)&(cb._synapses.signs)))
                {
                    CHAR16 log[256];
                    setText(log, L"Failed to allocate memory for synapses! Try to allocated ");
                    appendNumber(log, synapseSignsCount * sizeof(unsigned long long) / 1024, true);
                    appendText(log, L" KB");
                    logToConsole(log);
                    return false;
                }

                if (!allocatePool(RANDOM2_POOL_SIZE * sizeof(PoolSynapseData), (void**)&(cb._poolSynapseData)))
                {
                    CHAR16 log[256];
                    setText(log, L"Failed to allocate memory for pool synapse data! Try to allocated ");
                    appendNumber(log, RANDOM2_POOL_SIZE * sizeof(PoolSynapseData) / 1024, true);
                    appendText(log, L" KB");
                    logToConsole(log);
                    return false;
                }
            }
        }

        for (int i = 0; i < solutionBufferCount; i++) {
            setMem(_computeBuffer[i]._synapses.signs, sizeof(_computeBuffer[i]._synapses.signs[0]) * synapseSignsCount, 0);
            setMem(_computeBuffer[i]._poolSynapseData, sizeof(_computeBuffer[i]._poolSynapseData[0]) * RANDOM2_POOL_SIZE, 0);
            setMem(_computeBuffer[i]._neurons.input, sizeof(_computeBuffer[i]._neurons.input[0]) * allNeuronsCount, 0);
            solutionEngineLock[i] = 0;
        }

#if USE_SCORE_CACHE
        scoreCacheLock = 0;
        setMem(&scoreCache, sizeof(scoreCache), 0);
#endif

        return true;
    }

    // Save score cache to SCORE_CACHE_FILE_NAME
    void saveScoreCache(int epoch, CHAR16* directory = NULL)
    {
#if USE_SCORE_CACHE
        ACQUIRE(scoreCacheLock);
        SCORE_CACHE_FILE_NAME[sizeof(SCORE_CACHE_FILE_NAME) / sizeof(SCORE_CACHE_FILE_NAME[0]) - 4] = epoch / 100 + L'0';
        SCORE_CACHE_FILE_NAME[sizeof(SCORE_CACHE_FILE_NAME) / sizeof(SCORE_CACHE_FILE_NAME[0]) - 3] = (epoch % 100) / 10 + L'0';
        SCORE_CACHE_FILE_NAME[sizeof(SCORE_CACHE_FILE_NAME) / sizeof(SCORE_CACHE_FILE_NAME[0]) - 2] = epoch % 10 + L'0';
        scoreCache.save(SCORE_CACHE_FILE_NAME, directory);
        RELEASE(scoreCacheLock);
#endif
    }

    // Update score cache filename with epoch and try to load file
    bool loadScoreCache(int epoch)
    {
        bool success = true;
#if USE_SCORE_CACHE
        ACQUIRE(scoreCacheLock);
        SCORE_CACHE_FILE_NAME[sizeof(SCORE_CACHE_FILE_NAME) / sizeof(SCORE_CACHE_FILE_NAME[0]) - 4] = epoch / 100 + L'0';
        SCORE_CACHE_FILE_NAME[sizeof(SCORE_CACHE_FILE_NAME) / sizeof(SCORE_CACHE_FILE_NAME[0]) - 3] = (epoch % 100) / 10 + L'0';
        SCORE_CACHE_FILE_NAME[sizeof(SCORE_CACHE_FILE_NAME) / sizeof(SCORE_CACHE_FILE_NAME[0]) - 2] = epoch % 10 + L'0';
        success = scoreCache.load(SCORE_CACHE_FILE_NAME);
        RELEASE(scoreCacheLock);
#endif
        return success;
    }

    template <typename T>
    inline constexpr T abs(const T& a)
    {
        return (a < 0) ? -a : a;
    }

    template  <typename T>
    void clampNeuron(T& val)
    {
        if (val > NEURON_VALUE_LIMIT) {
            val = NEURON_VALUE_LIMIT;
        }
        else if (val < -NEURON_VALUE_LIMIT) {
            val = -NEURON_VALUE_LIMIT;
        }
    }

    void generateSynapse(computeBuffer& cb, int solutionBufIdx, const m256i& publicKey, const m256i& nonce)
    {
        random2(&publicKey.m256i_u8[0], &nonce.m256i_u8[0], (unsigned char*)(cb._synapses.data), synapseInputCount * sizeof(cb._synapses.data[0]), cb._poolRandom2Buffer);
    }

    bool isValidScore(unsigned int solutionScore)
    {
        return (solutionScore >=0 && solutionScore <= DATA_LENGTH);
    }
    bool isGoodScore(unsigned int solutionScore, int threshold)
    {
        return (threshold <= (DATA_LENGTH / 3)) && ((solutionScore >= (unsigned int)((DATA_LENGTH / 3) + threshold)) || (solutionScore <= (unsigned int)((DATA_LENGTH / 3) - threshold)));
    }

    // Compute score
    unsigned int computeScore(const unsigned long long processor_Number, const m256i& publicKey, const m256i& miningSeed, const m256i& nonce)
    {
        const int solutionBufIdx = (int)(processor_Number % solutionBufferCount);

        unsigned int score = 0;
        unsigned int random2XVal = 0;
        computeBuffer& cb = _computeBuffer[solutionBufIdx];
        unsigned long long* pSynapseSigns = cb._synapses.signs;
        PoolSynapseData* pPoolSynapseData = cb._poolSynapseData;
        auto& neurons = cb._neurons;

        setMem(neurons.input, sizeof(neurons.input[0]) * allNeuronsCount, 0);
        for (int i = 0; i < dataLength; i++)
        {
            neurons.input[i] = (char)miningData[i];
        }

        //generateSynapse(cb, solutionBufIdx, publicKey, nonce);
        cb.k12.initState(&publicKey.m256i_u64[0], &nonce.m256i_u64[0], cb._poolRandom2Buffer);

        random2XVal = cb.k12.random2FromPrecomputedPool((unsigned char*)pSynapseSigns, synapseSignsCount * sizeof(unsigned long long));

        for (unsigned int i = 0; i < RANDOM2_POOL_SIZE; i++)
        {
            const unsigned int poolIdx = i & (RANDOM2_POOL_ACTUAL_SIZE - 1);
            const unsigned long long poolValue = *((unsigned long long*) & cb._poolRandom2Buffer[poolIdx]);
            const unsigned long long neuronIndex = dataLength + poolValue % computeNeuronsCount;
            const unsigned long long neighborNeuronIndex = (poolValue / computeNeuronsCount) % numberOfNeighborNeurons;
            unsigned long long supplierNeuronIndex;
            if (neighborNeuronIndex < numberOfNeighborNeurons / 2)
            {
                supplierNeuronIndex = (neuronIndex - (numberOfNeighborNeurons / 2) + neighborNeuronIndex + allNeuronsCount) % allNeuronsCount;
            }
            else
            {
                supplierNeuronIndex = (neuronIndex + 1 - (numberOfNeighborNeurons / 2) + neighborNeuronIndex + allNeuronsCount) % allNeuronsCount;
            }
            const unsigned long long offset = neuronIndex * numberOfNeighborNeurons + neighborNeuronIndex;

            unsigned int isPositive = !(pSynapseSigns[offset >> 6] & (1ULL << (offset & 63ULL))) ? 1 : 0;

            pPoolSynapseData[i].neuronIndex = (unsigned int)neuronIndex;
            pPoolSynapseData[i].supplierIndexWithSign = ((unsigned int)supplierNeuronIndex << 1) | isPositive;
        }

        for (long long tick = 0; tick < maxDuration; tick++)
        {
            PoolSynapseData data = pPoolSynapseData[random2XVal & (RANDOM2_POOL_ACTUAL_SIZE - 1)];
            unsigned int neuronIndex = data.neuronIndex;
            unsigned int supplierNeuronIndex = (data.supplierIndexWithSign >> 1);
            unsigned int sign = (data.supplierIndexWithSign & 1U);

            char nnV = neurons.input[supplierNeuronIndex];
            nnV = sign ? nnV : -nnV;
            neurons.input[neuronIndex] += nnV;
            clampNeuron(neurons.input[neuronIndex]);

            random2XVal = random2XVal * 1664525 + 1013904223;
        }

        score = 0;
        for (unsigned int i = 0; i < dataLength; i++)
        {
            if (miningData[i] == neurons.input[dataLength + numberOfHiddenNeurons + i])
            {
                score++;
            }
        }
        return score;
    }

    // main score function
    unsigned int operator()(const unsigned long long processor_Number, const m256i& publicKey, const m256i& miningSeed, const m256i& nonce)
    {
        if (isZero(miningSeed) || miningSeed != currentRandomSeed)
        {
            return DATA_LENGTH + 1; // return invalid score
        }

        int score = 0;
#if USE_SCORE_CACHE
        unsigned int scoreCacheIndex = scoreCache.getCacheIndex(publicKey, miningSeed, nonce);
        score = scoreCache.tryFetching(publicKey, miningSeed, nonce, scoreCacheIndex);
        if (score >= scoreCache.MIN_VALID_SCORE)
        {
            return score;
        }
        score = 0;
#endif

        const int solutionBufIdx = (int)(processor_Number % solutionBufferCount);
        ACQUIRE(solutionEngineLock[solutionBufIdx]);

        score = computeScore(processor_Number, publicKey, miningSeed, nonce);

        RELEASE(solutionEngineLock[solutionBufIdx]);
#if USE_SCORE_CACHE
        scoreCache.addEntry(publicKey, miningSeed, nonce, scoreCacheIndex, score);
#endif
#ifdef NO_UEFI
        int y = 2 + score;
        stackSize = top_of_stack - ((unsigned long long)(&y));
#endif
        return score;
    }

#ifdef NO_UEFI
    unsigned long long stackSize = 0;
#endif

    // Multithreaded solutions verification:
    // This module mainly serve tick processor in qubic core node, thus the queue size is limited at NUMBER_OF_TRANSACTIONS_PER_TICK 
    // for future use for somewhere else, you can only increase the size.

    volatile char taskQueueLock = 0;
    struct {
        m256i publicKey[NUMBER_OF_TRANSACTIONS_PER_TICK];
        m256i miningSeed[NUMBER_OF_TRANSACTIONS_PER_TICK];
        m256i nonce[NUMBER_OF_TRANSACTIONS_PER_TICK];
    } taskQueue;
    unsigned int _nTask;
    unsigned int _nProcessing;
    unsigned int _nFinished;
    bool _nIsTaskQueueReady;

    void resetTaskQueue()
    {
        ACQUIRE(taskQueueLock);
        _nTask = 0;
        _nProcessing = 0;
        _nFinished = 0;
        _nIsTaskQueueReady = false;
        RELEASE(taskQueueLock);
    }

    // add task to the queue
    // queue size is limited at NUMBER_OF_TRANSACTIONS_PER_TICK 
    void addTask(m256i publicKey, m256i miningSeed, m256i nonce)
    {
        ACQUIRE(taskQueueLock);
        if (_nTask < NUMBER_OF_TRANSACTIONS_PER_TICK)
        {
            unsigned int index = _nTask++;
            taskQueue.publicKey[index] = publicKey;
            taskQueue.miningSeed[index] = miningSeed;
            taskQueue.nonce[index] = nonce;
        }
        RELEASE(taskQueueLock);
    }

    void startProcessTaskQueue()
    {
        ACQUIRE(taskQueueLock);
        _nIsTaskQueueReady = true;
        RELEASE(taskQueueLock);
    }

    void stopProcessTaskQueue()
    {
        ACQUIRE(taskQueueLock);
        _nIsTaskQueueReady = false;
        RELEASE(taskQueueLock);
    }

    // get a task, can call on any thread
    bool getTask(m256i* publicKey, m256i* miningSeed, m256i* nonce)
    {
        if (!_nIsTaskQueueReady)
        {
            return false;
        }
        bool result = false;
        ACQUIRE(taskQueueLock);
        if (_nProcessing < _nTask)
        {
            unsigned int index = _nProcessing++;
            *publicKey = taskQueue.publicKey[index];
            *miningSeed = taskQueue.miningSeed[index];
            *nonce = taskQueue.nonce[index];
            result = true;
        }
        else
        {
            result = false;
        }
        RELEASE(taskQueueLock);
        return result;
    }
    void finishTask()
    {
        ACQUIRE(taskQueueLock);
        _nFinished++;
        RELEASE(taskQueueLock);
    }

    bool isTaskQueueProcessed()
    {
        return _nFinished == _nTask;
    }

    void tryProcessSolution(unsigned long long processorNumber)
    {
        m256i publicKey;
        m256i miningSeed;
        m256i nonce;
        bool res = this->getTask(&publicKey, &miningSeed, &nonce);
        if (res)
        {
            (*this)(processorNumber, publicKey, miningSeed, nonce);
            this->finishTask();
        }
    }
};
