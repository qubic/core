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
    unsigned long long numberOfOptimizationSteps,
    unsigned long long solutionBufferCount
>
struct ScoreFunction
{
    typedef int neuron_t;

    static constexpr unsigned long long allNeuronsCount = dataLength + numberOfHiddenNeurons + dataLength;
    static constexpr unsigned long long computeNeuronsCount = numberOfHiddenNeurons + dataLength;
    static constexpr unsigned long long synapseSignsCount = (dataLength + numberOfHiddenNeurons + dataLength) * numberOfNeighborNeurons / 64;
    static constexpr unsigned long long synapseInputCount = synapseSignsCount + maxDuration;

    static constexpr const unsigned char candidateSkipTickMaskBits = 2;
    static constexpr const unsigned char skippedTickMaskBits = 1;
    static constexpr const unsigned char clearSkippedTickMaskBits = ~skippedTickMaskBits;

#if defined (__AVX512F__)
    static constexpr int BATCH_SIZE = 16;
#else
    static constexpr int BATCH_SIZE = 8;
#endif
    static_assert(maxDuration % BATCH_SIZE == 0, "maxDuration must be dividable by BATCH_SIZE ");
    static_assert(allNeuronsCount < 0xFFFFFFFF, "Current implementation only support MAX_UINT32 neuron");
    static_assert(numberOfNeighborNeurons < 0x7FFFFFFF, "Current implementation only support MAX_UINT32 number of neighbors");
    static_assert((allNeuronsCount* numberOfNeighborNeurons) % 64 == 0, "numberOfNeighborNeurons * allNeuronsCount must dividable by 64");

    long long miningData[dataLength];

    struct K12EngineX1
    {
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
        void _scatterFromVector()
        {
            copyToStateScalar(scatteredStates)
        }
        void hashNewChunk()
        {
            declareBCDEScalar
                rounds12Scalar
        }
        void hashNewChunkAndSaveToState()
        {
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
            if (leftByteInCurrentState)
            {
                int copySize = size < leftByteInCurrentState ? size : leftByteInCurrentState;
                copyMem(out0, s0 + 200 - leftByteInCurrentState, copySize);
                size -= copySize;
                leftByteInCurrentState -= copySize;
                out0 += copySize;
            }
            while (size)
            {
                if (!leftByteInCurrentState)
                {
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

        void scatterFromVector()
        {
            _scatterFromVector();
        }
        void hashWithoutWrite(int size)
        {
            if (leftByteInCurrentState)
            {
                int copySize = size < leftByteInCurrentState ? size : leftByteInCurrentState;
                size -= copySize;
                leftByteInCurrentState -= copySize;
            }
            while (size)
            {
                if (!leftByteInCurrentState)
                {
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

    struct computeBuffer
    {
        struct Neuron
        {
            neuron_t* input;
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

        unsigned short* _poolNeuronIndices;
        unsigned short* _poolsupplierIndexWithSign;

        // Save skipped ticks
        long long* _skipTicks;

        // Map of skipped ticks
        unsigned char* _skipTicksMap;

        unsigned char* _parBatches;

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

                if (_computeBuffer[i]._parBatches)
                {
                    freePool(_computeBuffer[i]._parBatches);
                    _computeBuffer[i]._parBatches = nullptr;
                }

                if (_computeBuffer[i]._poolNeuronIndices)
                {
                    freePool(_computeBuffer[i]._poolNeuronIndices);
                    _computeBuffer[i]._poolNeuronIndices = nullptr;
                }

                if (_computeBuffer[i]._poolsupplierIndexWithSign)
                {
                    freePool(_computeBuffer[i]._poolsupplierIndexWithSign);
                    _computeBuffer[i]._poolsupplierIndexWithSign = nullptr;
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

                if (!allocatePool((maxDuration + BATCH_SIZE - 1) / BATCH_SIZE, (void**)&(cb._parBatches)))
                {
                    logToConsole(L"Failed to allocate memory for _parBatches! Try to allocated ");
                    return false;
                }

                if (!allocatePool(allNeuronsCount * sizeof(neuron_t), (void**)&(cb._neurons.input)))
                {
                    logToConsole(L"Failed to allocate memory for neurons! Try to allocated ");
                    return false;
                }

                if (!allocatePool(synapseSignsCount * sizeof(unsigned long long), (void**)&(cb._synapses.signs)))
                {
                    logToConsole(L"Failed to allocate memory for synapses! Try to allocated ");
                    return false;
                }

                if (!allocatePool(RANDOM2_POOL_SIZE * sizeof(PoolSynapseData), (void**)&(cb._poolSynapseData)))
                {
                    logToConsole(L"Failed to allocate memory for pool synapse data!");
                    return false;
                }

                if (!allocatePool(numberOfOptimizationSteps * sizeof(long long), (void**)&(cb._skipTicks)))
                {
                    logToConsole(L"Failed to allocate memory for skip ticks buffer!");
                    return false;
                }

                if (!allocatePool(maxDuration * sizeof(long long), (void**)&(cb._ticksNumbers)))
                {
                    logToConsole(L"Failed to allocate memory for ticks number buffer!");
                    return false;
                }

                if (!allocatePool(maxDuration, (void**)&(cb._skipTicksMap)))
                {
                    logToConsole(L"Failed to allocate memory for ticks map buffer!");
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

                if (!allocatePool(maxDuration * sizeof(unsigned short), (void**)&(cb._poolNeuronIndices)))
                {
                    logToConsole(L"Failed to allocate memory for _poolNeuronIndices!");
                    return false;
                }

                if (!allocatePool(maxDuration * sizeof(unsigned short), (void**)&(cb._poolsupplierIndexWithSign)))
                {
                    logToConsole(L"Failed to allocate memory for _poolsupplierIndexWithSign!");
                    return false;
                }
            }
        }

        for (int i = 0; i < solutionBufferCount; i++)
        {
            setMem(_computeBuffer[i]._synapses.signs, sizeof(_computeBuffer[i]._synapses.signs[0]) * synapseSignsCount, 0);
            setMem(_computeBuffer[i]._poolSynapseData, sizeof(_computeBuffer[i]._poolSynapseData[0]) * RANDOM2_POOL_SIZE, 0);
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
        if (val > NEURON_VALUE_LIMIT)
        {
            val = NEURON_VALUE_LIMIT;
        }
        else if (val < -NEURON_VALUE_LIMIT)
        {
            val = -NEURON_VALUE_LIMIT;
        }
    }

    void generateSynapse(computeBuffer& cb, int solutionBufIdx, const m256i& publicKey, const m256i& nonce)
    {
        random2(&publicKey.m256i_u8[0], &nonce.m256i_u8[0], (unsigned char*)(cb._synapses.data), synapseInputCount * sizeof(cb._synapses.data[0]), cb._poolRandom2Buffer);
    }

    bool isValidScore(unsigned int solutionScore)
    {
        return (solutionScore >= 0 && solutionScore <= DATA_LENGTH);
    }
    bool isGoodScore(unsigned int solutionScore, int threshold)
    {
        return (threshold <= DATA_LENGTH) && (solutionScore >= (unsigned int)threshold);
    }

    void computePoolSynapseData(
        const unsigned long long* pSynapseSigns,
        const unsigned char* pPoolBuffer,
        PoolSynapseData* pPoolSynapseData,
        unsigned short* pNeuronIdices,
        unsigned short* pNeuronSupplier)
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
            pNeuronIdices[tick] = data.neuronIndex;
            pNeuronSupplier[tick] = data.supplierIndexWithSign;

            random2XVal = random2XVal * 1664525 + 1013904223;
        }
    }

    unsigned int computeFullNeurons(
        const unsigned short* pNeuronIdices,
        const unsigned short* pNeuronSupplier,
        const unsigned char* skipTicksMap,
        unsigned char* curCachedNeurons,
        unsigned char* batches,
        computeBuffer::Neuron& neurons32)
    {
        return computeNeurons<false>(pNeuronIdices, pNeuronSupplier, skipTicksMap, curCachedNeurons, batches, neurons32);
    }

    void checkParallelBatch(
        const unsigned short* pNeuronIdices,
        const unsigned short* pNeuronSupplier,
        unsigned char* pBatch)
    {
        int tickBatch = 0;
        setMem(pBatch, sizeof(pBatch[0]) * (maxDuration + BATCH_SIZE - 1) / BATCH_SIZE, 0);
        for (long long batch = 0; batch < maxDuration; batch += BATCH_SIZE, tickBatch++)
        {
            if (areElementsUnique(pNeuronIdices + batch, BATCH_SIZE)
                && !isAnyElementInBContainedInA(pNeuronIdices + batch, pNeuronSupplier + batch, BATCH_SIZE, 1))
            {
                pBatch[tickBatch] = 1;
            }
        }
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

    template<typename T>
    bool areElementsUnique(const T* A, int size)
    {
        for (int i = 0; i < size; ++i)
        {
            for (int j = i + 1; j < size; ++j)
            {
                if (A[i] == A[j])
                {
                    return false; // Duplicate found
                }
            }
        }
        return true; // All elements are unique
    }

    template<typename T>
    bool isAnyElementInBContainedInA(const T* A, const  T* B, int size, int shiftB = 0)
    {
        for (int i = 0; i < size; ++i)
        {
            for (int j = 0; j < size; ++j)
            {
                if ((B[i] >> shiftB) == A[j])
                {
                    return true; // Element in B found in A
                }
            }
        }
        return false; // No element in B is found in A
    }


    template <bool skipTickFlag, int batchSize>
    void computeNeuronsBatch(
        long long tickOffset,
        const unsigned short* pNeuronIdices,
        const unsigned short* pNeuronSupplier,
        const unsigned char* skipTicksMap,
        unsigned char* curCachedNeurons,
        computeBuffer::Neuron& neurons32)
    {
        long long tick = tickOffset;
        for (int k = 0; k < batchSize; k++, tick++)
        {
            unsigned int supplierIndexWithSign = pNeuronSupplier[tick];
            unsigned short supplierNeuronIndex = supplierIndexWithSign >> 1;
            unsigned char sign = supplierIndexWithSign & 1U;
            char nnV = neurons32.input[supplierNeuronIndex];
            nnV = sign ? nnV : -nnV;
            unsigned short neuronIndex = pNeuronIdices[tick];
            char oldNeuronValue = neurons32.input[neuronIndex];
            neurons32.input[neuronIndex] += nnV;
            clampNeuron(neurons32.input[neuronIndex]);
            if (skipTicksMap[tick] & candidateSkipTickMaskBits)
            {
                curCachedNeurons[tick] = (oldNeuronValue == neurons32.input[neuronIndex]);
                if (skipTickFlag && (skipTicksMap[tick] & skippedTickMaskBits))
                {
                    neurons32.input[neuronIndex] = oldNeuronValue;
                }
            }
        }
    }

    template <bool skipTickFlag, int batchSize>
    void computeNeuronsBatchSIMD(
        long long batch,
        const unsigned short* pNeuronIdices,
        const unsigned short* pNeuronSupplier,
        const unsigned char* skipTicksMap,
        neuron_t* pBuffers,
        unsigned char* curCachedNeurons,
        computeBuffer::Neuron& neurons32)
    {
#if defined (__AVX512F__)
        __m512i supplierNeuronIndex = _mm512_cvtepu16_epi32(_mm256_loadu_si256((__m256i*)(pNeuronSupplier + batch)));
        __m512i neuronIndex = _mm512_cvtepu16_epi32(_mm256_loadu_si256((__m256i*)(pNeuronIdices + batch)));
        __m512i skipCheck = _mm512_cvtepu8_epi32(_mm_loadu_si128((__m128i*)(skipTicksMap + batch)));

        // Load supplier indices with sign
        __m512i sign = _mm512_and_epi32(supplierNeuronIndex, _mm512_set1_epi32(1));
        supplierNeuronIndex = _mm512_srai_epi32(supplierNeuronIndex, 1);

        // Gather neuron values
        __m512i gatheredValues = _mm512_i32gather_epi32(supplierNeuronIndex, (const neuron_t*)neurons32.input, sizeof(neuron_t));

        // Apply sign
        __m512i negatedValues = _mm512_sub_epi32(_mm512_setzero_si512(), gatheredValues);
        __mmask16 mask = _mm512_cmpeq_epi32_mask(sign, _mm512_set1_epi32(1));
        __m512i nnV = _mm512_mask_blend_epi32(mask, negatedValues, gatheredValues);

        // Gather old neuron values
        __m512i oldNeuronValues = _mm512_i32gather_epi32(neuronIndex, (const neuron_t*)neurons32.input, sizeof(neuron_t));

        // Add nnV to old neuron values
        __m512i newNeuronValues = _mm512_add_epi32(oldNeuronValues, nnV);
        newNeuronValues = _mm512_max_epi32(newNeuronValues, _mm512_set1_epi32(-NEURON_VALUE_LIMIT));
        newNeuronValues = _mm512_min_epi32(newNeuronValues, _mm512_set1_epi32(NEURON_VALUE_LIMIT));

        // Skip tick check
        if (skipTickFlag)
        {
            __mmask16 candidateCheck = _mm512_cmpeq_epi32_mask(_mm512_and_epi32(skipCheck, _mm512_set1_epi32(skippedTickMaskBits)), _mm512_setzero_si512());
            newNeuronValues = _mm512_mask_blend_epi32(candidateCheck, oldNeuronValues, newNeuronValues);
        }

        // Scatter new neuron values back to neuronsInput
        _mm512_i32scatter_epi32((int*)neurons32.input, neuronIndex, newNeuronValues, sizeof(neuron_t));

        _mm512_storeu_si512((__m512i*)pBuffers, newNeuronValues);
        _mm512_storeu_si512((__m512i*)(pBuffers + batchSize), oldNeuronValues);
        for (int k = 0; k < batchSize; ++k)
        {
            const long long tick = batch + k;
            if (skipTicksMap[tick] & candidateSkipTickMaskBits)
            {
                curCachedNeurons[tick] = (pBuffers[k] == pBuffers[batchSize + k]);
            }
        }
#else
        __m256i supplierNeuronIndex = _mm256_cvtepu16_epi32(_mm_loadu_epi16((__m128i*)(pNeuronSupplier + batch)));
        __m256i neuronIndex = _mm256_cvtepu16_epi32(_mm_loadu_epi16((__m128i*)(pNeuronIdices + batch)));
        __m256i skipCheck = _mm256_cvtepu8_epi32(_mm_loadl_epi64((__m128i*)(skipTicksMap + batch)));

        // Load supplier indices with sign
        __m256i sign = _mm256_and_epi32(supplierNeuronIndex, _mm256_set1_epi32(1));
        supplierNeuronIndex = _mm256_srai_epi32(supplierNeuronIndex, 1);

        // Gather neuron values
        __m256i gatheredValues = _mm256_i32gather_epi32((const int*)neurons32.input, supplierNeuronIndex, sizeof(int));

        // Apply sign
        __m256i negatedValues = _mm256_sub_epi32(_mm256_setzero_si256(), gatheredValues);
        __m256i mask = _mm256_cmpeq_epi32(sign, _mm256_set1_epi32(1));
        __m256i nnV = _mm256_blendv_epi8(negatedValues, gatheredValues, mask);

        // Gather old neuron values
        __m256i oldNeuronValues = _mm256_i32gather_epi32((const int*)neurons32.input, neuronIndex, sizeof(int));

        // Add nnV to old neuron values
        __m256i newNeuronValues = _mm256_add_epi32(oldNeuronValues, nnV);
        newNeuronValues = _mm256_max_epi32(newNeuronValues, _mm256_set1_epi32(-NEURON_VALUE_LIMIT));
        newNeuronValues = _mm256_min_epi32(newNeuronValues, _mm256_set1_epi32(NEURON_VALUE_LIMIT));

        _mm256_storeu_si256((__m256i*)pBuffers, newNeuronValues);
        _mm256_storeu_si256((__m256i*)(pBuffers + batchSize), oldNeuronValues);

        for (int k = 0; k < batchSize; ++k)
        {
            const long long tick = batch + k;
            unsigned short neuronIndex = pNeuronIdices[tick];
            char oldNeuronValue = pBuffers[batchSize + k];
            char newNeuronValue = pBuffers[k];
            neurons32.input[neuronIndex] = newNeuronValue;
            if (skipTicksMap[tick] & candidateSkipTickMaskBits)
            {
                curCachedNeurons[tick] = (newNeuronValue == oldNeuronValue);
                if (skipTickFlag && (skipTicksMap[tick] & skippedTickMaskBits))
                {
                    neurons32.input[neuronIndex] = oldNeuronValue;
                }
            }
        }
#endif
    }

    template <bool skipTickFlag>
    unsigned int computeNeurons(
        const unsigned short* pNeuronIdices,
        const unsigned short* pNeuronSupplier,
        const unsigned char* skipTicksMap,
        unsigned char* curCachedNeurons,
        const unsigned char* batches,
        computeBuffer::Neuron& neurons32)
    {
        setMem(neurons32.input, sizeof(neurons32.input[0]) * allNeuronsCount, 0);
        for (int i = 0; i < dataLength; i++)
        {
            neurons32.input[i] = (char)miningData[i];
        }
        neuron_t neuronBuffer[2 * BATCH_SIZE];
        long long batchIdx = 0;
        for (long long batch = 0; batch < maxDuration; batch += BATCH_SIZE, batchIdx++)
        {
            if (batches[batchIdx])
            {
                computeNeuronsBatchSIMD<skipTickFlag, BATCH_SIZE>(
                    batch,
                    pNeuronIdices,
                    pNeuronSupplier,
                    skipTicksMap,
                    neuronBuffer,
                    curCachedNeurons,
                    neurons32);
            }
            else
            {
                computeNeuronsBatch<skipTickFlag, BATCH_SIZE>(
                    batch,
                    pNeuronIdices,
                    pNeuronSupplier,
                    skipTicksMap,
                    curCachedNeurons,
                    neurons32);
            }
        }

        // Calculate the score
        unsigned int currentScore = 0;
        for (unsigned int i = 0; i < dataLength; i++)
        {
            if (miningData[i] == neurons32.input[dataLength + numberOfHiddenNeurons + i])
            {
                currentScore++;
            }
        }
        return currentScore;
    }


    unsigned int computeSkipTicksNeurons(
        const unsigned short* pNeuronIdices,
        const unsigned short* pNeuronSupplier,
        const unsigned char* skipTicksMap,
        unsigned char* curCachedNeurons,
        unsigned char* batches,
        computeBuffer::Neuron& neurons32)
    {
        return computeNeurons<true>(pNeuronIdices, pNeuronSupplier, skipTicksMap, curCachedNeurons, batches, neurons32);
    }

    // Compute score
    unsigned int computeScore(const unsigned long long processor_Number, const m256i& publicKey, const m256i& miningSeed, const m256i& nonce)
    {
        const int solutionBufIdx = (int)(processor_Number % solutionBufferCount);

        computeBuffer& cb = _computeBuffer[solutionBufIdx];
        auto* prvCachedNeurons = cb._prvCachedNeurons;
        auto* curCachedNeurons = cb._curCachedNeurons;

        setMem(prvCachedNeurons, sizeof(prvCachedNeurons[0]) * maxDuration, 0);
        setMem(curCachedNeurons, sizeof(curCachedNeurons[0]) * maxDuration, 0);

        //generateSynapse(cb, solutionBufIdx, publicKey, nonce);
        cb.k12.initState(&publicKey.m256i_u64[0], &nonce.m256i_u64[0], cb._poolRandom2Buffer);
        cb.k12.random2FromPrecomputedPool((unsigned char*)cb._synapses.signs, synapseSignsCount * sizeof(unsigned long long));

        // Cache the pool synapse data
        computePoolSynapseData(cb._synapses.signs, cb._poolRandom2Buffer, cb._poolSynapseData, cb._poolNeuronIndices, cb._poolsupplierIndexWithSign);

        // Next run for optimization steps
        // Generate a list of possible skip ticks
        computeSkipTicks(cb._poolRandom2Buffer, cb._skipTicks, cb._skipTicksMap, cb._ticksNumbers);

        // Calculate batches that can run parallel
        checkParallelBatch(cb._poolNeuronIndices, cb._poolsupplierIndexWithSign, cb._parBatches);

        // First run to get the score of fulll
        unsigned int score = computeFullNeurons(cb._poolNeuronIndices, cb._poolsupplierIndexWithSign, cb._skipTicksMap, prvCachedNeurons, cb._parBatches, cb._neurons);

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

            unsigned int currentScore = computeSkipTicksNeurons(cb._poolNeuronIndices, cb._poolsupplierIndexWithSign, cb._skipTicksMap, curCachedNeurons, cb._parBatches, cb._neurons);

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
    struct
    {
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
