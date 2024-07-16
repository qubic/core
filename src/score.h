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

template<
    unsigned int dataLength,
    unsigned int numberOfInputNeurons,
    unsigned int _numberOfOutputNeurons,
    unsigned int maxInputDuration,
    unsigned int _maxOutputDuration,
    unsigned int solutionBufferCount
>
struct ScoreFunction
{
    static constexpr const int inNeuronsCount = numberOfInputNeurons + dataLength;
    static constexpr const int maxNeuronsCount = inNeuronsCount;
    static constexpr const unsigned long long allParamsCount = dataLength + numberOfInputNeurons + dataLength;
    static constexpr unsigned long long synapseInputSize =inNeuronsCount * allParamsCount;
    long long miningData[dataLength];
    struct synapseStruct
    {
        char* inputLength = nullptr;
    };
    synapseStruct *_synapses;

    struct queueItem {
        unsigned int tick;
        unsigned int neuronIdx;
    };
    struct queueState {
        char sum0, sum1, sum2, nSum;
        int currentTopMaxCount;
    };

    struct synapseCheckpoint {
        unsigned long long ckp[25];
        int ignoreByteInState;
    };

    struct K12EngineX1 {
        unsigned long long Aba, Abe, Abi, Abo, Abu;
        unsigned long long Aga, Age, Agi, Ago, Agu;
        unsigned long long Aka, Ake, Aki, Ako, Aku;
        unsigned long long Ama, Ame, Ami, Amo, Amu;
        unsigned long long Asa, Ase, Asi, Aso, Asu;
        unsigned long long scatteredStates[25];
        int leftByteInCurrentState;
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
        void initState(const unsigned long long* comp_u64, const unsigned long long* nonce_u64) {
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
        }
        void write(unsigned char* out0, int size) {
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

        void saveCheckpoint(synapseCheckpoint** p_sckp) {
            synapseCheckpoint& sckp_0 = *(p_sckp[0]);
            unsigned long long* output0 = sckp_0.ckp;
            copyToStateScalar(output0)
            sckp_0.ignoreByteInState = 200 - leftByteInCurrentState;
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

    struct computeBuffer {
        // neuron only has values [-1, 0, 1]
        struct {
            char inputAtTick[maxInputDuration + 1][dataLength + numberOfInputNeurons + dataLength];
        } neurons;
        char* inputLength;
        unsigned int* indicePos[maxNeuronsCount];
        int bucketPos[maxNeuronsCount][129];
        bool isGeneratedBucket[maxNeuronsCount];
        static_assert((allParamsCount) % 8 == 0, "need to check this packed synapse");

        queueItem queue[allParamsCount * 2];
        bool isProcessing[allParamsCount * 2];
        queueState state[allParamsCount * 2];
        unsigned int _maxIndexBuffer[allParamsCount * 2][32];
        int buffer[256];
        K12EngineX1 k12;
        synapseCheckpoint sckpInput[(numberOfInputNeurons + dataLength)][1];
        bool isGeneratedSynapse[numberOfInputNeurons + dataLength];
    } *_computeBuffer;
    unsigned int* _indiceBigBuffer;

    static_assert(maxInputDuration <= 256, "Need to regenerate mod num table");
    // _totalModNum[i]: total of divisible numbers of i
    unsigned char _totalModNum[257];
    // i is divisible by _modNum[i][j], j < _totalModNum[i]
    unsigned char _modNum[257][129];

    m256i initialRandomSeed;

    volatile char solutionEngineLock[solutionBufferCount];

#if USE_SCORE_CACHE
    volatile char scoreCacheLock;
    ScoreCache<SCORE_CACHE_SIZE, SCORE_CACHE_COLLISION_RETRIES> scoreCache;
#endif

    void initMiningData(m256i randomSeed)
    {
        initialRandomSeed = randomSeed; // persist the initial random seed to be able to sned it back on system info response
        random((unsigned char*)&randomSeed, (unsigned char*)&randomSeed, (unsigned char*)miningData, sizeof(miningData));
        for (unsigned int i = 0; i < dataLength; i++)
        {
            miningData[i] = (miningData[i] >= 0 ? 1 : -1);
        }
        setMem(_totalModNum, sizeof(_totalModNum), 0);
        setMem(_modNum, sizeof(_modNum), 0);

        // init the divisible table
        for (int i = 1; i <= 256; i++) {
            for (int j = 1; j <= 127; j++) { // exclude 128
                if (j && i % j == 0) {
                    _modNum[i][_totalModNum[i]++] = j;
                }
            }
        }
    }

    ~ScoreFunction()
    {
        freeMemory();
    }

    void freeMemory()
    {
        if (_synapses)
        {
            for (unsigned int i = 0; i < solutionBufferCount; i++)
            {
                if (_synapses[i].inputLength)
                {
                    freePool(_synapses[i].inputLength);
                }
            }
            freePool(_synapses);
        }
    }

    bool initMemory()
    {
        // TODO: call freePool() for buffers allocated below
        if (_synapses == nullptr) {
            if (!allocatePool(sizeof(synapseStruct) * solutionBufferCount, (void**)&_synapses))
            {
                logToConsole(L"Failed to allocate memory for score solution buffer!");
                return false;
            }

            for (unsigned int i = 0; i < solutionBufferCount; i++)
            {
                if (!allocatePool(synapseInputSize, (void**)&(_synapses[i].inputLength)))
                {
                    logToConsole(L"Failed to allocate memory for synapses input length!");
                    return false;
                }
            }
        }
        if (_computeBuffer == nullptr) {
            if (!allocatePool(sizeof(computeBuffer) * solutionBufferCount, (void**)&_computeBuffer))
            {
                logToConsole(L"Failed to allocate memory for score solution buffer!");
                return false;
            }

            if (!allocatePool(sizeof(unsigned int) * allParamsCount * maxNeuronsCount * solutionBufferCount, (void**)&_indiceBigBuffer))
            {
                logToConsole(L"Failed to allocate memory for score indice pos!");
                return false;
            }

            unsigned int* bigBuffer = _indiceBigBuffer;
            for (int bufId = 0; bufId < solutionBufferCount; bufId++)
            {
                auto& cb = _computeBuffer[bufId];
                for (int i = 0; i < maxNeuronsCount; i++)
                {
                    cb.indicePos[i] = bigBuffer;
                    bigBuffer += allParamsCount;
                }
            }
        }

        for (int i = 0; i < solutionBufferCount; i++) {
            //setMem(&_synapses[i], sizeof(synapseStruct), 0);
            setMem(_synapses[i].inputLength, synapseInputSize, 0);
            setMem(&_computeBuffer[i].neurons, sizeof(_computeBuffer[i].neurons), 0);            
            _computeBuffer[i].inputLength = nullptr;
            setMem(_computeBuffer[i].indicePos[0], sizeof(unsigned int) * allParamsCount * maxNeuronsCount, 0); // it's continuous memory region
            setMem(_computeBuffer[i].bucketPos, sizeof(_computeBuffer[i].bucketPos), 0);
            setMem(_computeBuffer[i].isGeneratedBucket, sizeof(_computeBuffer[i].isGeneratedBucket), 0);
            setMem(_computeBuffer[i].queue, sizeof(_computeBuffer[i].queue), 0);
            setMem(_computeBuffer[i].isProcessing, sizeof(_computeBuffer[i].isProcessing), 0);
            setMem(_computeBuffer[i].state, sizeof(_computeBuffer[i].state), 0);
            setMem(_computeBuffer[i]._maxIndexBuffer, sizeof(_computeBuffer[i]._maxIndexBuffer), 0);
            setMem(_computeBuffer[i].buffer, sizeof(_computeBuffer[i].buffer), 0);
            setMem(&_computeBuffer[i].k12, sizeof(_computeBuffer[i].k12), 0);
            setMem(_computeBuffer[i].sckpInput, sizeof(_computeBuffer[i].sckpInput), 0);
            setMem(_computeBuffer[i].isGeneratedSynapse, sizeof(_computeBuffer[i].isGeneratedSynapse), 0);
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

    static inline void zeroOutSynapses(char* synapses, int idx) {
        synapses[idx + dataLength] = 0;
    }

    void continueGeneratingSynapseFromCkp(synapseCheckpoint& ckp, unsigned char* out, int size) {
        unsigned long long buffer[25];
        unsigned char* buffer_u8 = (unsigned char*)buffer;
        unsigned long long* state = ckp.ckp;
        unsigned char* state_u8 = (unsigned char*)state;

        declareABCDEScalar
            copyFromStateScalar(state)
            int leftByte = 200 - ckp.ignoreByteInState;
        if (leftByte) {
            int copySize = leftByte < size ? leftByte : size;
            copyMem(out, state_u8 + 200 - leftByte, copySize);
            out += copySize;
            size -= copySize;
            leftByte -= copySize;
        }
        while (size) {
            if (!leftByte) {
                rounds12Scalar
                    copyToStateScalar(buffer)
                    leftByte = 200;
            }
            int copySize = leftByte < size ? leftByte : size;
            copyMem(out, buffer_u8 + 200 - leftByte, copySize);
            size -= copySize;
            leftByte -= copySize;
            out += copySize;
        }
    }

    void generateSynapse(computeBuffer& cb, int solutionBufIdx, const m256i& publicKey, const m256i& nonce)
    {
        auto& synapses = _synapses[solutionBufIdx];
        cb.k12.initState(publicKey.m256i_u64, nonce.m256i_u64);
        for (unsigned long long i = 0; i < numberOfInputNeurons; i++) {
            synapseCheckpoint* p_sckp[1] = { &cb.sckpInput[i][0] };
            cb.k12.saveCheckpoint(p_sckp);
            cb.k12.hashWithoutWrite(allParamsCount);
            cb.isGeneratedSynapse[i] = false;
        };
        cb.k12.scatterFromVector();
        for (unsigned long long i = numberOfInputNeurons; i < inNeuronsCount; i++) {
            cb.k12.write((unsigned char*)(&synapses.inputLength[0] + i * allParamsCount), allParamsCount);
            cb.isGeneratedSynapse[i] = true;
        }
        for (unsigned long long i = numberOfInputNeurons; i < inNeuronsCount; i++) {
            zeroOutSynapses(synapses.inputLength + i * allParamsCount, int(i));
        }
    }


    void cacheBucketIndices(const char* synapseLength, computeBuffer& cb, size_t nrIdx) {
        int* buffer = cb.buffer;
        synapseLength += nrIdx * allParamsCount;
        for (size_t j = 0; j < allParamsCount; j++) {
            const char len = synapseLength[j];
            if (len == 0 || len == -128) continue;
            cb.bucketPos[nrIdx][abs(len)]++;
        }

        buffer[0] = 0;
        for (size_t j = 1; j <= 128; j++) {
            buffer[j] = buffer[j - 1] + cb.bucketPos[nrIdx][j - 1];
        }
        copyMem(cb.bucketPos[nrIdx], buffer, 129 * sizeof(int));

        for (size_t j = 0; j < allParamsCount; j++) {
            const char len = synapseLength[j];
            if (len == 0 || len == -128) continue;
            unsigned int sign = (len > 0) ? 1 : 0;
            cb.indicePos[nrIdx][buffer[abs(len)]++] = (unsigned int)(j << 1) | sign;
        }
    }

    template <int neurBefore, bool isInput>
    char accessNeuron(computeBuffer& cb, const int currentTick, const int accessNeuronIdx)
    {
        if (accessNeuronIdx < neurBefore) return cb.neurons.inputAtTick[1][accessNeuronIdx];
        const int targetTick = (currentTick - 1);
        if (targetTick == 0) return 0;
        return cb.neurons.inputAtTick[targetTick][accessNeuronIdx];
    }

    template <int neurBefore, bool isInput>
    char findTopFromBucket(computeBuffer& cb,
        const int tick,
        const int neuronIdx,
        const unsigned int* indices,
        const int* bucket,
        const unsigned char* modList,
        unsigned int& outIdx,
        const int numMods,
        unsigned int* maxIndexBuffer,
        int& currentCount) {
        int currentMax = -1;
        int max_id = -1;
        for (int i = 0; i < numMods; i++) {
            int mod = modList[i];
            int start = bucket[mod];
            int end = bucket[mod + 1];
            if (start + int(maxIndexBuffer[i]) < end) {
                if (int(indices[end - int(maxIndexBuffer[i]) - 1]) > currentMax) {
                    currentMax = indices[end - maxIndexBuffer[i] - 1];
                    max_id = i;
                }
            }
        }
        if (currentMax == -1) return NULL_INDEX;
        char v = accessNeuron<neurBefore, isInput>(cb, tick, currentMax >> 1);
        if (v) {
            if (v != NOT_CALCULATED) {
                v *= (currentMax & 1) ? 1 : -1;
                outIdx = (v > 0) ? (currentMax | 1) : (currentMax & 0);
                currentCount++;
                maxIndexBuffer[max_id]++;
            }
            else {
                outIdx = currentMax >> 1;
            }
        }
        else {
            maxIndexBuffer[max_id]++;
        }
        return v;
    }

    template <bool isInput>
    void setNeuronVal(computeBuffer& cb, int tick, int neuronIdx, char val)
    {
        cb.neurons.inputAtTick[tick][neuronIdx] = val;
    }

    template <int neurBefore>
    void fullComputeNeuron(const int tick,
        const unsigned int neuronIdx,
        computeBuffer& cb,
        char* pNr,
        const char* sy,
        const int outNrIdx) {
        char v = 0;
        for (int i = 0; i < neurBefore; i++) {
            unsigned long long idx = neuronIdx * allParamsCount + i;
            const char s = sy[idx];
            if (s == 1 || s == -1 && pNr[i])
            {
                v += pNr[i] * s;
                clampNeuron(v);
            }
        }
        pNr[outNrIdx] = v;
    }

    template <int neurBefore, bool isInput>
    char solveNeuron(computeBuffer& cb, int targetTick, int targetNeuronIdx)
    {
        auto& queue = cb.queue;
        auto& isProcessing = cb.isProcessing;
        auto& state = cb.state;
        auto& _maxIndexBuffer = cb._maxIndexBuffer;

        int size = 1;
        queue[0].neuronIdx = targetNeuronIdx;
        queue[0].tick = targetTick;
        isProcessing[0] = false;
        bool goDeeper = false;
        bool foundShortcut = false;
        while (size)
        {
            //pop
            int tick = queue[size - 1].tick;
            const int numMods = _totalModNum[tick];
            const auto* modList = _modNum[tick];
            unsigned int neuronIdx = queue[size - 1].neuronIdx;
            const unsigned int idx = neuronIdx - neurBefore;
            auto& maxIndexBuffer = _maxIndexBuffer[size - 1];
            auto& sum0 = state[size - 1].sum0;
            auto& sum1 = state[size - 1].sum1;
            auto& sum2 = state[size - 1].sum2;
            auto& nSum = state[size - 1].nSum;
            auto& currentTopMaxCount = state[size - 1].currentTopMaxCount;
            if (!isProcessing[size - 1]) {
                setMem(maxIndexBuffer, numMods * sizeof(maxIndexBuffer[0]), 0);
                sum0 = 0;
                sum1 = 0;
                sum2 = 0;
                nSum = 0;
                currentTopMaxCount = 0;
                isProcessing[size - 1] = true;
            }
            if (!cb.isGeneratedSynapse[idx])
            {
                continueGeneratingSynapseFromCkp(cb.sckpInput[idx][0], (unsigned char*)cb.inputLength + idx * (unsigned long long)allParamsCount, allParamsCount);
                zeroOutSynapses(cb.inputLength + idx * allParamsCount, idx);
                cb.isGeneratedSynapse[idx] = true;
            }
            if (!cb.isGeneratedBucket[neuronIdx - neurBefore])
            {
                cb.isGeneratedBucket[neuronIdx - neurBefore] = true;
                cacheBucketIndices(cb.inputLength, cb, neuronIdx - neurBefore);
            }
            
            if (tick == 1)
            {
                // special case, process with unique function
                fullComputeNeuron<neurBefore>(1,
                    idx,
                    cb,
                    cb.neurons.inputAtTick[1],
                    cb.inputLength,
                    neuronIdx);
            }
            else
            {
                goDeeper = false;
                foundShortcut = false;
                while (1) {
                    int prev = currentTopMaxCount;
                    unsigned int foundPos = -1;
                    char res = findTopFromBucket<neurBefore, isInput>(cb, tick, neuronIdx,
                        cb.indicePos[neuronIdx - neurBefore],
                        cb.bucketPos[neuronIdx - neurBefore],
                        modList,
                        foundPos,
                        numMods,
                        maxIndexBuffer,
                        currentTopMaxCount);
                    if (res == NULL_INDEX) break;
                    if (res == NOT_CALCULATED) {
                        // add to queue
                        queue[size].tick = tick - 1;
                        queue[size].neuronIdx = foundPos;
                        goDeeper = true;
                        isProcessing[size] = false;
                        size++;
                        break;
                    }
                    if (!res) {
                        continue;
                    }

                    {
                        sum2 = sum1;
                        sum1 = sum0;
                        sum0 = (foundPos & 1) ? 1 : -1;
                        if (prev > 1) {
                            if (sum2 > 0) nSum++;
                            else nSum--;
                        }
                    }
                    if (prev == 0) continue;
                    if (sum0 == sum1) {
                        char result = (sum0 > 0) ? NEURON_VALUE_LIMIT : -NEURON_VALUE_LIMIT;
                        result += nSum;
                        clampNeuron(result);
                        setNeuronVal<isInput>(cb, tick, neuronIdx, result);
                        foundShortcut = true;
                        break;
                    }
                }
                if (!foundShortcut) {
                    if (goDeeper) {
                        continue;
                    }
                    else { // need full compute
                        char v = accessNeuron<neurBefore, isInput>(cb, tick, neuronIdx);
                        if (v == NOT_CALCULATED) {
                            // add to queue
                            queue[size].tick = tick - 1;
                            queue[size].neuronIdx = neuronIdx;
                            isProcessing[size] = false;
                            size++;
                            continue;
                        }
                        else {
                            v += sum0;
                            clampNeuron(v);
                            v += sum1;
                            clampNeuron(v);
                            v += nSum;
                            clampNeuron(v);
                            setNeuronVal<isInput>(cb, tick, neuronIdx, v);
                        }
                    }
                }
            }            
            size--;
        }

        return cb.neurons.inputAtTick[targetTick][targetNeuronIdx];
    }

    // main score function
    unsigned int operator()(const unsigned long long processor_Number, const m256i& publicKey, const m256i& nonce)
    {
        int score = 0;
#if USE_SCORE_CACHE
        unsigned int scoreCacheIndex = scoreCache.getCacheIndex(publicKey, nonce);
        score = scoreCache.tryFetching(publicKey, nonce, scoreCacheIndex);
        if (score >= scoreCache.MIN_VALID_SCORE)
        {
            return score;
        }
        score = 0;
#endif

        const int solutionBufIdx = (int)(processor_Number % solutionBufferCount);
        ACQUIRE(solutionEngineLock[solutionBufIdx]);

        auto& synapses = _synapses[solutionBufIdx];
        auto& cb = _computeBuffer[solutionBufIdx];

        generateSynapse(cb, solutionBufIdx, publicKey, nonce);
        cb.inputLength = synapses.inputLength;

        {
            setMem(cb.bucketPos, sizeof(cb.bucketPos), 0);
            setMem(cb.isGeneratedBucket, sizeof(cb.isGeneratedBucket), false);
        }

        // ComputeInput
        {
            setMem(cb.neurons.inputAtTick, sizeof(cb.neurons.inputAtTick), NOT_CALCULATED);
            for (int i = 0; i < dataLength; i++) {
                cb.neurons.inputAtTick[0][i] = (char)miningData[i];
                cb.neurons.inputAtTick[1][i] = (char)miningData[i];
            }

            setMem(cb.neurons.inputAtTick[0] + dataLength, inNeuronsCount * sizeof(cb.neurons.inputAtTick[0][0]), 0);
            for (unsigned int inputNeuronIndex = numberOfInputNeurons; inputNeuronIndex < numberOfInputNeurons + dataLength; inputNeuronIndex++) {
                fullComputeNeuron<dataLength>(1,
                    inputNeuronIndex,
                    cb,
                    cb.neurons.inputAtTick[1],
                    synapses.inputLength,
                    dataLength + inputNeuronIndex);
            }
            for (int tick = 2; tick <= maxInputDuration; tick++)
            {
                for (unsigned int inputNeuronIndex = numberOfInputNeurons; inputNeuronIndex < numberOfInputNeurons + dataLength; inputNeuronIndex++) {
                    cb.neurons.inputAtTick[tick][dataLength + inputNeuronIndex] = solveNeuron<dataLength, true>(cb, tick, dataLength + inputNeuronIndex);
                }
            }
        }

        score = 0;
        for (unsigned int i = 0; i < dataLength; i++) {
            if (miningData[i] == cb.neurons.inputAtTick[maxInputDuration][dataLength + numberOfInputNeurons + i]) {
                score++;
            }
        }

        RELEASE(solutionEngineLock[solutionBufIdx]);
#if USE_SCORE_CACHE
        scoreCache.addEntry(publicKey, nonce, scoreCacheIndex, score);
#endif
#ifdef NO_UEFI
        int y = 2 + score;
        unsigned long long ss = top_of_stack - ((unsigned long long)(&y));
        std::cout << "Stack size: " << ss << " bytes\n";
#endif
        return score;
    }

    // Multithreaded solutions verification:
    // This module mainly serve tick processor in qubic core node, thus the queue size is limited at NUMBER_OF_TRANSACTIONS_PER_TICK 
    // for future use for somewhere else, you can only increase the size.

    volatile char taskQueueLock = 0;
    struct {
        m256i publicKey[NUMBER_OF_TRANSACTIONS_PER_TICK];
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
    void addTask(m256i publicKey, m256i nonce)
    {
        ACQUIRE(taskQueueLock);
        if (_nTask < NUMBER_OF_TRANSACTIONS_PER_TICK)
        {
            unsigned int index = _nTask++;
            taskQueue.publicKey[index] = publicKey;
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
    bool getTask(m256i* publicKey, m256i* nonce)
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
        m256i nonce;
        bool res = this->getTask(&publicKey, &nonce);
        if (res)
        {
            (*this)(processorNumber, publicKey, nonce);
            this->finishTask();
        }
    }
};
