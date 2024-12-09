#pragma once
#ifdef NO_UEFI
unsigned long long top_of_stack;
#endif
#include "platform/memory_util.h"
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
    unsigned long long numberOfOptimizationSteps,
    unsigned long long solutionBufferCount
>
struct ScoreFunction
{
    static constexpr unsigned long long allNeuronsCount = dataLength + numberOfHiddenNeurons + dataLength;
    static constexpr unsigned long long computeNeuronsCount = numberOfHiddenNeurons + dataLength;
    static constexpr unsigned long long synapseSignsCount = (dataLength + numberOfHiddenNeurons + dataLength) * numberOfNeighborNeurons / 64;
    static constexpr unsigned long long synapseInputCount = synapseSignsCount + maxDuration;

    static constexpr const unsigned char candidateSkipTickMaskBits = 2;
    static constexpr const unsigned char skippedTickMaskBits = 1;
    static constexpr const unsigned char clearSkippedTickMaskBits = ~skippedTickMaskBits;

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
        PoolSynapseData* _poolSynapseTickData;
        unsigned char* _poolRandom2Buffer;

        // Save skipped ticks
        long long* _skipTicks;

        // Map of skipped ticks
        unsigned char * _skipTicksMap;

        // Contained all ticks possible value
        long long* _ticksNumbers;

        unsigned char* _prvCachedNeurons;
        unsigned char* _curCachedNeurons;

    } *_computeBuffer = nullptr;
    m256i currentRandomSeed;
    unsigned int randomXNeuronStart;
    unsigned int randomXOpStart;

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

                if (_computeBuffer[i]._skipTicks)
                {
                    freePool(_computeBuffer[i]._skipTicks);
                    _computeBuffer[i]._skipTicks = nullptr;
                }

                if (_computeBuffer[i]._ticksNumbers)
                {
                    freePool(_computeBuffer[i]._ticksNumbers);
                    _computeBuffer[i]._ticksNumbers = nullptr;
                }

                if (_computeBuffer[i]._skipTicksMap)
                {
                    freePool(_computeBuffer[i]._skipTicksMap);
                    _computeBuffer[i]._skipTicksMap = nullptr;
                }

                if (_computeBuffer[i]._poolSynapseTickData)
                {
                    freePool(_computeBuffer[i]._poolSynapseTickData);
                    _computeBuffer[i]._poolSynapseTickData = nullptr;
                }

                if (_computeBuffer[i]._poolSynapseData)
                {
                    freePool(_computeBuffer[i]._poolSynapseData);
                    _computeBuffer[i]._poolSynapseData = nullptr;
                }

                if (_computeBuffer[i]._prvCachedNeurons)
                {
                    freePool(_computeBuffer[i]._prvCachedNeurons);
                    _computeBuffer[i]._prvCachedNeurons = nullptr;
                }

                if (_computeBuffer[i]._curCachedNeurons)
                {
                    freePool(_computeBuffer[i]._curCachedNeurons);
                    _computeBuffer[i]._curCachedNeurons = nullptr;
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
            if (!allocPoolWithErrorLog(L"computeBuffer (score solution buffer)", sizeof(computeBuffer) * solutionBufferCount, (void**)&_computeBuffer, __LINE__))
            {
                return false;
            }

            for (int bufId = 0; bufId < solutionBufferCount; bufId++)
            {
                auto& cb = _computeBuffer[bufId];

                if (!allocPoolWithErrorLog(L"poolRandom2Buffer (score pool buffer)", RANDOM2_POOL_SIZE, (void**)&(cb._poolRandom2Buffer), __LINE__))
                {
                    return false;
                }

                if (!allocPoolWithErrorLog(L"neurons.input", allNeuronsCount, (void**)&(cb._neurons.input), __LINE__))
                {
                    return false;
                }

                if (!allocPoolWithErrorLog(L"synapses.signs", synapseSignsCount * sizeof(unsigned long long), (void**)&(cb._synapses.signs), __LINE__))
                {
                    return false;
                }

                if (!allocPoolWithErrorLog(L"poolSynapseData", RANDOM2_POOL_SIZE * sizeof(PoolSynapseData), (void**)&(cb._poolSynapseData), __LINE__))
                {
                    return false;
                }

                if (!allocPoolWithErrorLog(L"poolSynapseTickData", maxDuration * sizeof(PoolSynapseData), (void**)&(cb._poolSynapseTickData), __LINE__))
                {
                    return false;
                }

                if (!allocPoolWithErrorLog(L"skipTicks", numberOfOptimizationSteps * sizeof(long long), (void**)&(cb._skipTicks), __LINE__))
                {
                    return false;
                }

                if (!allocPoolWithErrorLog(L"ticksNumbers", maxDuration * sizeof(long long), (void**)&(cb._ticksNumbers), __LINE__))
                {
                    return false;
                }

                if (!allocPoolWithErrorLog(L"skipTicksMap", maxDuration, (void**)&(cb._skipTicksMap), __LINE__))
                {
                    return false;
                }

                if (!allocatePool(maxDuration, (void**)&(cb._prvCachedNeurons)))
                {
                    logToConsole(L"Failed to allocate memory for prev cached neurons!");
                    return false;
                }

                if (!allocatePool(maxDuration, (void**)&(cb._curCachedNeurons)))
                {
                    logToConsole(L"Failed to allocate memory for cur cached neurons!");
                    return false;
                }
            }
        }

        for (int i = 0; i < solutionBufferCount; i++) {
            setMem(_computeBuffer[i]._synapses.signs, sizeof(_computeBuffer[i]._synapses.signs[0]) * synapseSignsCount, 0);
            setMem(_computeBuffer[i]._poolSynapseData, sizeof(_computeBuffer[i]._poolSynapseData[0]) * RANDOM2_POOL_SIZE, 0);
            setMem(_computeBuffer[i]._poolSynapseTickData, sizeof(_computeBuffer[i]._poolSynapseTickData[0]) * maxDuration, 0);
            setMem(_computeBuffer[i]._neurons.input, sizeof(_computeBuffer[i]._neurons.input[0]) * allNeuronsCount, 0);
            setMem(_computeBuffer[i]._ticksNumbers, sizeof(_computeBuffer[i]._ticksNumbers[0]) * maxDuration, 0);
            setMem(_computeBuffer[i]._skipTicks, sizeof(_computeBuffer[i]._skipTicks[0]) * numberOfOptimizationSteps, 0);
            setMem(_computeBuffer[i]._skipTicksMap, sizeof(_computeBuffer[i]._skipTicksMap[0]) * maxDuration, 0);
            setMem(_computeBuffer[i]._prvCachedNeurons, sizeof(_computeBuffer[i]._prvCachedNeurons[0]) * maxDuration, 0);
            setMem(_computeBuffer[i]._curCachedNeurons, sizeof(_computeBuffer[i]._curCachedNeurons[0]) * maxDuration, 0);
            solutionEngineLock[i] = 0;
        }

        unsigned int x = 0;
        for (unsigned long long i = 0; i < synapseSignsCount; i++)
        {
            x = x * 1664525 + 1013904223;
        }
        randomXNeuronStart = x;
        for (unsigned long long i = 0; i < maxDuration; i++)
        {
            x = x * 1664525 + 1013904223;
        }
        randomXOpStart = x;

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
        return (threshold <= DATA_LENGTH) && (solutionScore >= (unsigned int)threshold);
    }

    void computePoolSynapseData(const unsigned long long* pSynapseSigns, const unsigned char* pPoolBuffer, PoolSynapseData* pPoolSynapseData, PoolSynapseData* pPoolSynapseTick)
    {
        for (unsigned int i = 0; i < RANDOM2_POOL_SIZE; i++)
        {
            const unsigned int poolIdx = i & (RANDOM2_POOL_ACTUAL_SIZE - 1);
            const unsigned long long poolValue = *((unsigned long long*) & pPoolBuffer[poolIdx]);
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

        unsigned int random2XVal = randomXNeuronStart;
        for (long long tick = 0; tick < maxDuration; tick++)
        {
            PoolSynapseData data = pPoolSynapseData[random2XVal & (RANDOM2_POOL_ACTUAL_SIZE - 1)];
            pPoolSynapseTick[tick] = data;
            random2XVal = random2XVal * 1664525 + 1013904223;
        }
    }

    unsigned int computeFullNeurons(
        const PoolSynapseData* pPoolSynapseTick,
        const unsigned char* skipTicksMap,
        unsigned char* curCachedNeurons,
        computeBuffer::Neuron& neurons)
    {
        setMem(neurons.input, sizeof(neurons.input[0]) * allNeuronsCount, 0);
        for (int i = 0; i < dataLength; i++)
        {
            neurons.input[i] = (char)miningData[i];
        }
        for (long long tick = 0; tick < maxDuration; tick++)
        {
            PoolSynapseData data = pPoolSynapseTick[tick];
            unsigned int neuronIndex = data.neuronIndex;
            unsigned int supplierNeuronIndex = (data.supplierIndexWithSign >> 1);
            unsigned int sign = (data.supplierIndexWithSign & 1U);

            char nnV = neurons.input[supplierNeuronIndex];
            nnV = sign ? nnV : -nnV;

            char oldNeuronValue = neurons.input[neuronIndex];
            neurons.input[neuronIndex] += nnV;
            clampNeuron(neurons.input[neuronIndex]);

            if (skipTicksMap[tick] & candidateSkipTickMaskBits)
            {
                curCachedNeurons[tick] = (oldNeuronValue == neurons.input[neuronIndex]);
            }
        }

        unsigned int score = 0;
        for (unsigned int i = 0; i < dataLength; i++)
        {
            if (miningData[i] == neurons.input[dataLength + numberOfHiddenNeurons + i])
            {
                score++;
            }
        }
        return score;
    }

    void computeSkipTicks(const unsigned char* poolRandom2Buffer, long long* skipTicks, unsigned char* skipTicksMap, long long* ticksNumbers)
    {
        long long tailTick = maxDuration - 1;
        for (long long tick = 0; tick < maxDuration; tick++)
        {
            ticksNumbers[tick] = tick;
        }
        setMem(skipTicksMap, maxDuration, 0);

        unsigned int random2XValOpt = randomXOpStart;
        for (long long l = 0; l < numberOfOptimizationSteps - 1; l++)
        {
            const unsigned int poolIdx = random2XValOpt & (RANDOM2_POOL_ACTUAL_SIZE - 1);
            const unsigned long long poolValue = *((unsigned long long*) & poolRandom2Buffer[poolIdx]);

            // Randomly choose a tick to skip for the next round and avoid duplicated pick already chosen one
            long long randomTick = poolValue % (maxDuration - l);
            skipTicks[l] = ticksNumbers[randomTick];
            skipTicksMap[skipTicks[l]] |= candidateSkipTickMaskBits;

            // Replace the chosen tick position with current tail to make sure if this possiton is chosen again
            // the skipTick is still not duplicated with previous ones.
            ticksNumbers[randomTick] = ticksNumbers[tailTick];
            tailTick--;

            random2XValOpt = random2XValOpt * 1664525 + 1013904223;
        }
    }

    unsigned int computeSkipTicksNeurons(const PoolSynapseData* pPoolSynapseTick, const unsigned char* skipTicksMap, unsigned char* curCachedNeurons, computeBuffer::Neuron& neurons)
    {
        // Reset neuron values
        setMem(neurons.input, sizeof(neurons.input[0]) * allNeuronsCount, 0);
        for (int i = 0; i < dataLength; i++)
        {
            neurons.input[i] = (char)miningData[i];
        }

        // Compute
        for (long long tick = 0; tick < maxDuration; tick++)
        {
            // Tick not on the list, accumulate as normal
            PoolSynapseData data = pPoolSynapseTick[tick];
            unsigned int neuronIndex = data.neuronIndex;
            unsigned int supplierNeuronIndex = (data.supplierIndexWithSign >> 1);
            unsigned int sign = (data.supplierIndexWithSign & 1U);

            char nnV = neurons.input[supplierNeuronIndex];
            nnV = sign ? nnV : -nnV;

            char oldNeuronValue = neurons.input[neuronIndex];
            neurons.input[neuronIndex] += nnV;
            clampNeuron(neurons.input[neuronIndex]);

            if (skipTicksMap[tick] & candidateSkipTickMaskBits)
            {
                curCachedNeurons[tick] = (oldNeuronValue == neurons.input[neuronIndex]);
                if (skipTicksMap[tick] & skippedTickMaskBits)
                {
                    neurons.input[neuronIndex] = oldNeuronValue;
                }
            }
        }

        // Calculate the score
        unsigned int currentScore = 0;
        for (unsigned int i = 0; i < dataLength; i++)
        {
            if (miningData[i] == neurons.input[dataLength + numberOfHiddenNeurons + i])
            {
                currentScore++;
            }
        }
        return currentScore;
    }

    // Compute score
    unsigned int computeScore(const unsigned long long processor_Number, const m256i& publicKey, const m256i& miningSeed, const m256i& nonce)
    {
        const int solutionBufIdx = (int)(processor_Number % solutionBufferCount);

        computeBuffer& cb = _computeBuffer[solutionBufIdx];
        PoolSynapseData* pPoolSynapseTick = cb._poolSynapseTickData;
        auto& neurons = cb._neurons;
        auto* prvCachedNeurons = cb._prvCachedNeurons;
        auto* curCachedNeurons = cb._curCachedNeurons;

        setMem(prvCachedNeurons, sizeof(prvCachedNeurons[0]) * maxDuration, 0);
        setMem(curCachedNeurons, sizeof(curCachedNeurons[0]) * maxDuration, 0);

        //generateSynapse(cb, solutionBufIdx, publicKey, nonce);
        cb.k12.initState(&publicKey.m256i_u64[0], &nonce.m256i_u64[0], cb._poolRandom2Buffer);
        cb.k12.random2FromPrecomputedPool((unsigned char*)cb._synapses.signs, synapseSignsCount * sizeof(unsigned long long));

        // Cache the pool synapse data
        computePoolSynapseData(cb._synapses.signs, cb._poolRandom2Buffer, cb._poolSynapseData, pPoolSynapseTick);

        // Next run for optimization steps
        // Generate a list of possible skip ticks
        computeSkipTicks(cb._poolRandom2Buffer, cb._skipTicks, cb._skipTicksMap, cb._ticksNumbers);

        // First run to get the score of fulll 
        unsigned int score = computeFullNeurons(pPoolSynapseTick, cb._skipTicksMap, prvCachedNeurons, neurons);

        // Run the optimization steps
        for (long long l = 0; l < numberOfOptimizationSteps - 1; l++)
        {
            const long long skipTick = cb._skipTicks[l];
            cb._skipTicksMap[skipTick] |= skippedTickMaskBits;

            if (prvCachedNeurons[skipTick])
            {
                continue;
            }

            // reset map
            for (long long k = 0; k < numberOfOptimizationSteps - 1; k++)
            {
                curCachedNeurons[cb._skipTicks[k]] = 0;
            }

            unsigned int currentScore = computeSkipTicksNeurons(pPoolSynapseTick, cb._skipTicksMap, curCachedNeurons, neurons);

            // Check if this tick is good to skip
            if (currentScore >= score)
            {
                score = currentScore;

                // Swap
                unsigned char* tmp = prvCachedNeurons;
                prvCachedNeurons = curCachedNeurons;
                curCachedNeurons = tmp;
            }
            else // Make score worse, reset it
            {
                cb._skipTicksMap[skipTick] &= clearSkippedTickMaskBits;
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
