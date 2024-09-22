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
    unsigned int numberOfHiddenNeurons,
    unsigned int numberOfNeighborNeurons,
    unsigned int maxDuration,
    unsigned int solutionBufferCount
>
struct ScoreFunction
{
    static constexpr const int inNeuronsCount = numberOfHiddenNeurons + dataLength;
    static constexpr const unsigned long long allParamsCount = dataLength + numberOfHiddenNeurons + dataLength;
    static constexpr unsigned long long synapseInputSize = inNeuronsCount * numberOfNeighborNeurons;
    static constexpr unsigned long long priorSynapsesLength = 0;//numberOfNeighborNeurons > 3200 ? 3200 : numberOfNeighborNeurons / 2;
    static constexpr unsigned long long priorSynapsesOffset = numberOfNeighborNeurons - priorSynapsesLength;
    static constexpr unsigned int numberOfCheckPoints = 2;
    /* 
    DURATION 65536 | MAX_NUM_MODS 48
    DURATION 32768 | MAX_NUM_MODS 44
    DURATION 16384  | MAX_NUM_MODS 41
    DURATION 4096  | MAX_NUM_MODS 34
    DURATION 2048  | MAX_NUM_MODS 30
    DURATION 1024  | MAX_NUM_MODS 26
    DURATION 512   | MAX_NUM_MODS 22
    */
    static constexpr unsigned int maxNumMods = (maxDuration <= 512)  ? 22 :
                                               (maxDuration <= 1024  ? 26 :
                                               (maxDuration <= 2048  ? 30 :
                                               (maxDuration <= 4096  ? 34 :
                                               (maxDuration <= 16384 ? 41 :
                                               (maxDuration <= 32768 ? 44 :
                                                48)))));

    long long miningData[dataLength];
    struct synapseStruct
    {
        char* inputLength = nullptr;
    };
    synapseStruct* _synapses = nullptr;

    struct queueItem {
        unsigned int tick;
        unsigned int neuronIdx;
    };
    struct queueState {
        char sum0, sum1, sum2, nSum;
        int currentTopMaxCount;
        bool priorCompute;
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
            char inputAtTick[maxDuration + 1][dataLength + numberOfHiddenNeurons + dataLength + numberOfNeighborNeurons];
        } neurons;
        char* inputLength;
        unsigned int* nnNeuronIndicePos[inNeuronsCount];
        int synapseBucketPos[inNeuronsCount][129];
        bool isGeneratedBucketOffset[inNeuronsCount];

        static_assert((allParamsCount) % 8 == 0, "need to check this packed synapse");
        static_assert(maxDuration <= 65536, "need to check this maxDuration and adjust maxNumMods");
        static_assert(dataLength <= numberOfNeighborNeurons, "data length need to be smaller than or equal numberOfNeighborNeurons");

        queueItem queue[allParamsCount * 2];
        bool isProcessing[allParamsCount * 2];
        queueState state[allParamsCount * 2];
        unsigned int _maxIndexBuffer[allParamsCount * 2][maxNumMods];
        int buffer[256];
        K12EngineX1 k12;
        synapseCheckpoint sckpInput[(numberOfHiddenNeurons + dataLength)][numberOfCheckPoints];
        bool isGeneratedSynapseOffset[numberOfHiddenNeurons + dataLength];
        bool isGeneratedSynapseFull[numberOfHiddenNeurons + dataLength];

        unsigned char* _poolBuffer;

    } *_computeBuffer = nullptr;
    unsigned int* _indiceBigBuffer = nullptr;

    // _totalModNum[i]: total of divisible numbers of i
    unsigned char _totalModNum[maxDuration + 1];
    // i is divisible by _modNum[i][j], j < _totalModNum[i]
    unsigned char _modNum[maxDuration + 1][129];
    unsigned char _tickDiviable[maxDuration + 1][129];

    m256i currentRandomSeed;

    volatile char solutionEngineLock[solutionBufferCount];

    unsigned int* _neuronNNIndices[inNeuronsCount];

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
            setMem(_totalModNum, sizeof(_totalModNum), 0);
            setMem(_modNum, sizeof(_modNum), 0);
            setMem(_tickDiviable, sizeof(_tickDiviable), 0);

            // init the divisible table
            for (int i = 1; i <= maxDuration; i++) 
            {
                for (int j = 1; j <= 127; j++) // exclude 128
                { 
                    if (j && i % j == 0) 
                    {
                        _modNum[i][_totalModNum[i]++] = j;
                        _tickDiviable[i][j] = 1;
                    }
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
            _synapses = nullptr;
        }

        if (_indiceBigBuffer)
        {
            freePool(_indiceBigBuffer);
            _indiceBigBuffer = nullptr;
        }

        if (_computeBuffer)
        {
            for (unsigned int i = 0; i < solutionBufferCount; i++)
            {
                if (_computeBuffer[i]._poolBuffer)
                {
                    freePool(_computeBuffer[i]._poolBuffer);
                }
            }

            freePool(_computeBuffer);
            _computeBuffer = nullptr;
        }
    }

    bool initMemory()
    {
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

            if (!allocatePool(sizeof(unsigned int) * numberOfNeighborNeurons * inNeuronsCount * solutionBufferCount, (void**)&_indiceBigBuffer))
            {
                logToConsole(L"Failed to allocate memory for score indice pos!");
                return false;
            }

            unsigned int* bigBuffer = _indiceBigBuffer;
            for (int bufId = 0; bufId < solutionBufferCount; bufId++)
            {
                auto& cb = _computeBuffer[bufId];
                for (int i = 0; i < inNeuronsCount; i++)
                {
                    cb.nnNeuronIndicePos[i] = bigBuffer;
                    bigBuffer += numberOfNeighborNeurons;
                }

                if (!allocatePool(RANDOM2_POOL_SIZE, (void**)&(cb._poolBuffer)))
                {
                    logToConsole(L"Failed to allocate memory for score pool buffer!");
                    return false;
                }
            }
        }

        for (int i = 0; i < solutionBufferCount; i++) {
            //setMem(&_synapses[i], sizeof(synapseStruct), 0);

            setMem(_synapses[i].inputLength, synapseInputSize, 0);
            setMem(&_computeBuffer[i].neurons, sizeof(_computeBuffer[i].neurons), 0);

            setMem(_computeBuffer[i].nnNeuronIndicePos[0], sizeof(unsigned int) * numberOfNeighborNeurons * inNeuronsCount, 0); // it's continuous memory region
            setMem(_computeBuffer[i].synapseBucketPos, sizeof(_computeBuffer[i].synapseBucketPos), 0);
            setMem(_computeBuffer[i].isGeneratedBucketOffset, sizeof(_computeBuffer[i].isGeneratedBucketOffset), 0);
            setMem(_computeBuffer[i].queue, sizeof(_computeBuffer[i].queue), 0);
            setMem(_computeBuffer[i].isProcessing, sizeof(_computeBuffer[i].isProcessing), 0);
            setMem(_computeBuffer[i].state, sizeof(_computeBuffer[i].state), 0);
            setMem(_computeBuffer[i]._maxIndexBuffer, sizeof(_computeBuffer[i]._maxIndexBuffer), 0);
            setMem(_computeBuffer[i].buffer, sizeof(_computeBuffer[i].buffer), 0);
            setMem(&_computeBuffer[i].k12, sizeof(_computeBuffer[i].k12), 0);
            setMem(_computeBuffer[i].sckpInput, sizeof(_computeBuffer[i].sckpInput), 0);
            setMem(_computeBuffer[i].isGeneratedSynapseFull, sizeof(_computeBuffer[i].isGeneratedSynapseFull), 0);
            setMem(_computeBuffer[i].isGeneratedSynapseOffset, sizeof(_computeBuffer[i].isGeneratedSynapseOffset), 0);
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

    static inline void zeroOutSynapses2(char* synapses, int idx) {
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
        random2(&publicKey.m256i_u8[0], &nonce.m256i_u8[0], (unsigned char*)(synapses.inputLength), synapseInputSize, cb._poolBuffer);
    }

    void cacheBucketIndices(const char* synapseLength, computeBuffer& cb, size_t nrIdx, size_t fromSynapseOffset, size_t toSynapseOffset) {
        int* buffer = cb.buffer;
        synapseLength += nrIdx * numberOfNeighborNeurons;
        for (size_t j = fromSynapseOffset; j <= toSynapseOffset; j++) {
            const char len = synapseLength[j];
            if (len == 0 || len == -128) continue;
            cb.synapseBucketPos[nrIdx][abs(len)]++;
        }

        buffer[0] = 0;
        for (size_t j = 1; j <= 128; j++) {
            buffer[j] = buffer[j - 1] + cb.synapseBucketPos[nrIdx][j - 1];
        }
        copyMem(cb.synapseBucketPos[nrIdx], buffer, 129 * sizeof(int));

        for (size_t j = fromSynapseOffset; j <= toSynapseOffset; j++) {
            const char len = synapseLength[j];
            if (len == 0 || len == -128) continue;
            unsigned int sign = (len > 0) ? 1 : 0;
            cb.nnNeuronIndicePos[nrIdx][buffer[abs(len)]++] = (unsigned int)(j << 1) | sign;
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

    // Get the max of synapse index 
    // TODO: with the neigbor that ro
    template <int neurBefore, bool isInput>
    char findTopFromBucket(computeBuffer& cb,
        const int tick,
        const int neuronIdx,
        const unsigned int* indices,
        const int* bucket,
        const unsigned char* modList,
        unsigned int& outIdx,
        const int numMods,
        unsigned int* maxIndexBuffer, // counting the number of neuron that have the max 
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
        const unsigned int synapseIdx = currentMax >> 1;
        unsigned int nnNeuronIdx = neuronIdx - neurBefore + 1 + synapseIdx;
        if (nnNeuronIdx >= allParamsCount)
        {
            nnNeuronIdx -= allParamsCount;
        }
        char v = accessNeuron<neurBefore, isInput>(cb, tick, nnNeuronIdx);
        if (v) {
            if (v != NOT_CALCULATED) {
                v *= (currentMax & 1) ? 1 : -1;
                outIdx = (v > 0) ? (currentMax | 1) : (currentMax & 0);
                currentCount++;
                maxIndexBuffer[max_id]++;
            }
            else {
                outIdx = nnNeuronIdx;
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
        const int outNrIdx)
    {
        char v = 0;
        for (int i = 0; i < numberOfNeighborNeurons; i++)
        {
            unsigned long long nnNeuronIdx = neuronIdx + 1 + i;
            if (nnNeuronIdx >= allParamsCount)
            {
                nnNeuronIdx -= allParamsCount;
            }
            if (nnNeuronIdx < neurBefore)
            {
                unsigned long long synapseIdx = neuronIdx * numberOfNeighborNeurons + i;
                const char s = sy[synapseIdx];
                if (s == 1 || s == -1 && pNr[nnNeuronIdx])
                {
                    v += pNr[nnNeuronIdx] * s;
                    clampNeuron(v);
                }
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
            // Index of neuron in the synapse table
            const unsigned int idx = neuronIdx - neurBefore;
            unsigned char* synapseOfNeuron = (unsigned char*)cb.inputLength + idx * (unsigned long long)numberOfNeighborNeurons;
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
                        cb.nnNeuronIndicePos[idx],
                        cb.synapseBucketPos[idx],
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
                        // Else
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
    bool isValidScore(unsigned int solutionScore)
    {
        return (solutionScore >=0 && solutionScore <= DATA_LENGTH);
    }
    bool isGoodScore(unsigned int solutionScore, int threshold)
    {
        return (threshold <= (DATA_LENGTH / 3)) && ((solutionScore >= (unsigned int)((DATA_LENGTH / 3) + threshold)) || (solutionScore <= (unsigned int)((DATA_LENGTH / 3) - threshold)));
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

        auto& synapses = _synapses[solutionBufIdx];
        auto& cb = _computeBuffer[solutionBufIdx];

        generateSynapse(cb, solutionBufIdx, publicKey, nonce);
        cb.inputLength = synapses.inputLength;

        setMem(cb.synapseBucketPos, sizeof(cb.synapseBucketPos), 0);
        setMem(cb.isGeneratedBucketOffset, sizeof(cb.isGeneratedBucketOffset), 0);

        // ComputeInput
        {
            for (unsigned long long idx = 0; idx < inNeuronsCount; idx++)
            {
                setMem(cb.synapseBucketPos[idx], sizeof(cb.synapseBucketPos[idx]), 0);
                cacheBucketIndices(cb.inputLength, cb, idx, 0, numberOfNeighborNeurons - 1);
                cb.isGeneratedBucketOffset[idx] = false;
                cb.isGeneratedSynapseFull[idx] = true;
            }

            setMem(cb.neurons.inputAtTick, sizeof(cb.neurons.inputAtTick), NOT_CALCULATED);

            setMem(cb.neurons.inputAtTick[0], allParamsCount + numberOfNeighborNeurons, 0);
            for (int i = 0; i < dataLength; i++) {
                cb.neurons.inputAtTick[0][i] = (char)miningData[i];
            }
            copyMem(cb.neurons.inputAtTick[0] + allParamsCount, cb.neurons.inputAtTick[0], numberOfNeighborNeurons);

            for (int i = 0; i < dataLength; i++)
            {
                cb.neurons.inputAtTick[1][i] = (char)miningData[i];
            }
            static constexpr int OFFSET = 16;
            static constexpr int OFFSET_1 = OFFSET - 1;

            for (int tick = 1; tick < maxDuration; tick++)
            {
                copyMem(cb.neurons.inputAtTick[tick - 1] + allParamsCount, cb.neurons.inputAtTick[tick - 1], numberOfNeighborNeurons);
                for (int i = 0; i < dataLength; i++)
                {
                    cb.neurons.inputAtTick[tick][i] = (char)miningData[i];
                }

                const int numMods = _totalModNum[tick];
                for (unsigned int inputNeuronIndex = 0; inputNeuronIndex < numberOfHiddenNeurons + dataLength; inputNeuronIndex++)
                {
                    char* pSynapseInput = synapses.inputLength + inputNeuronIndex * numberOfNeighborNeurons;
                    char prev = 0;
                    char sum = 0;
                    char sum0 = 0;
                    char sum1 = 0;
                    char sum2 = 0;
                    bool foundShortCut = false;
                    long long i = (long long)numberOfNeighborNeurons - OFFSET;
                    for (; i >= 0 && !foundShortCut; i -= OFFSET)
                    {
                        char* pNNSynapse = pSynapseInput + i;
                        char* pNNNr = cb.neurons.inputAtTick[tick - 1] + inputNeuronIndex + 1 + i;
                        unsigned long long negMask = 0;
                        unsigned long long nonZerosMask = 0;

                        const __m128i neurons128 = _mm_loadu_si128((const __m128i*)(pNNNr));
                        const __m128i synapses128 = _mm_loadu_si128((const __m128i*)(pNNSynapse));
                        const __m128i absSynapse = _mm_abs_epi8(synapses128);
                        const __m128i zeros128 = _mm_setzero_si128();
                        __m128i nonZeros128 = zeros128;
                        for (int modIdx = 0; modIdx < numMods; modIdx++)
                        {
                            nonZeros128 = _mm_or_si128(nonZeros128, _mm_cmpeq_epi8(absSynapse, _mm_set1_epi8(_modNum[tick][modIdx])));
                        }

                        nonZerosMask = (unsigned long long)(~(_mm_movemask_epi8(_mm_cmpeq_epi8(neurons128, zeros128))) & _mm_movemask_epi8(nonZeros128));
                        const unsigned int negSyn = _mm_movemask_epi8(_mm_and_si128(_mm_cmpgt_epi8(zeros128, synapses128), nonZeros128));
                        const unsigned int negNr = _mm_movemask_epi8(_mm_and_si128(_mm_cmpgt_epi8(zeros128, neurons128), nonZeros128));
                        negMask = (unsigned long long)(negSyn ^ negNr);
                        constexpr unsigned long long markBit = (1ULL << 63);
                        if (nonZerosMask)
                        {
                            while (nonZerosMask && !foundShortCut)
                            {
                                const unsigned long long maskBit = markBit >> _lzcnt_u64(nonZerosMask);
                                const char nnV = (maskBit & negMask) ? -1 : 1;
                                {
                                    sum2 = sum1;
                                    sum1 = sum0;
                                    sum0 = nnV;
                                    if (prev > 1)
                                    {
                                        if (sum2 > 0) sum++;
                                        else sum--;
                                    }
                                }

                                if (prev > 1 && sum0 == sum1)
                                {
                                    char result = (sum0 > 0) ? NEURON_VALUE_LIMIT : -NEURON_VALUE_LIMIT;
                                    sum += result;
                                    clampNeuron(sum);
                                    foundShortCut = true;
                                    break;
                                }
                                nonZerosMask ^= maskBit;
                                prev++;
                            }
                        }
                    }

                    if (!foundShortCut)
                    {
                        i = numberOfNeighborNeurons & OFFSET_1;
                        if (i > 0)
                        {
                            for (; i >= 0 && !foundShortCut; i--)
                            {
                                char s = pSynapseInput[i];
                                char absS = _tickDiviable[tick][(unsigned char)abs(s)];
                                if (absS)
                                {
                                    unsigned long long anotherInputNeuronIndex = inputNeuronIndex + 1 + i;
                                    char nnV = cb.neurons.inputAtTick[tick - 1][anotherInputNeuronIndex];
                                    if (!nnV)
                                        continue;
                                    nnV = s > 0 ? nnV : -nnV;

                                    {
                                        sum2 = sum1;
                                        sum1 = sum0;
                                        sum0 = nnV;
                                        if (prev > 1)
                                        {
                                            if (sum2 > 0) sum++;
                                            else sum--;
                                        }
                                    }
                                    prev++;

                                    if (prev > 1 && sum0 == sum1)
                                    {
                                        char result = (sum0 > 0) ? NEURON_VALUE_LIMIT : -NEURON_VALUE_LIMIT;
                                        sum += result;
                                        clampNeuron(sum);
                                        foundShortCut = true;
                                        break;
                                    }
                                    prev++;
                                }
                            }
                        }
                    }

                    if (!foundShortCut)
                    {
                        char v = cb.neurons.inputAtTick[tick - 1][dataLength + inputNeuronIndex];
                        v += sum0;
                        clampNeuron(v);
                        v += sum1;
                        clampNeuron(v);
                        sum += v;
                        clampNeuron(sum);
                    }
                    cb.neurons.inputAtTick[tick][dataLength + inputNeuronIndex] = sum;
                }
            }
        }

        {
            int tick = maxDuration;
            for (unsigned int inputNeuronIndex = numberOfHiddenNeurons; inputNeuronIndex < numberOfHiddenNeurons + dataLength; inputNeuronIndex++) {
                cb.neurons.inputAtTick[tick][dataLength + inputNeuronIndex] = solveNeuron<dataLength, true>(cb, tick, dataLength + inputNeuronIndex);
            }
        }

        score = 0;
        for (unsigned int i = 0; i < dataLength; i++) {
            if (miningData[i] == cb.neurons.inputAtTick[maxDuration][dataLength + numberOfHiddenNeurons + i]) {
                score++;
            }
        }

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
