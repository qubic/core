#pragma once
#ifdef NO_UEFI
static unsigned long long top_of_stack;
#endif
#include "platform/memory_util.h"
#include "platform/m256.h"
#include "platform/concurrency.h"
#include "platform/profiling.h"
#include "public_settings.h"
#include "score_cache.h"

#if defined(_MSC_VER)

#define popcnt32(x)  static_cast<int>(__popcnt(static_cast<unsigned int>(x)))
#define popcnt64(x)  static_cast<int>(__popcnt64(static_cast<unsigned long long>(x)))

#else

#define popcnt32(x)  __builtin_popcount  (static_cast<unsigned int>(x))
#define popcnt64(x)  __builtin_popcountll(static_cast<unsigned long long>(x))

#endif


////////// Scoring algorithm \\\\\\\\\\

#define NOT_CALCULATED -127 //not yet calculated
#define NULL_INDEX -2

constexpr unsigned char INPUT_NEURON_TYPE = 0;
constexpr unsigned char OUTPUT_NEURON_TYPE = 1;
constexpr unsigned char EVOLUTION_NEURON_TYPE = 2;

#if !(defined (__AVX512F__) || defined(__AVX2__))
static_assert(false, "Either AVX2 or AVX512 is required.");
#endif

#if defined (__AVX512F__)
    static constexpr int BATCH_SIZE = 64;
#elif defined(__AVX2__)
    static constexpr int BATCH_SIZE = 32;
#endif

constexpr unsigned long long POOL_VEC_SIZE = (((1ULL << 32) + 64)) >> 3; // 2^32+64 bits ~ 512MB
constexpr unsigned long long POOL_VEC_PADDING_SIZE = (POOL_VEC_SIZE + 200 - 1) / 200 * 200; // padding for multiple of 200
constexpr unsigned long long STATE_SIZE = 200;
static const char gLUT3States[] = { 0, 1, -1 };

static void generateRandom2Pool(const unsigned char* miningSeed, unsigned char* state, unsigned char* pool)
{
    // same pool to be used by all computors/candidates and pool content changing each phase
    copyMem(&state[0], miningSeed, 32);
    setMem(&state[32], STATE_SIZE - 32, 0);

    for (unsigned int i = 0; i < POOL_VEC_PADDING_SIZE; i += STATE_SIZE)
    {
        KeccakP1600_Permute_12rounds(state);
        copyMem(&pool[i], state, STATE_SIZE);
    }
}

static void random2(
    unsigned char* seed,                // 32 bytes
    const unsigned char* pool,
    unsigned char* output,
    unsigned long long outputSizeInByte // must be divided by 64
)
{
    ASSERT(outputSizeInByte % 64 == 0);

    unsigned long long segments = outputSizeInByte / 64;
    unsigned int x[8] = { 0 };
    for (int i = 0; i < 8; i++)
    {
        x[i] = ((unsigned int*)seed)[i];
    }

    for (int j = 0; j < segments; j++)
    {
        // Each segment will have 8 elements. Each element have 8 bytes
        for (int i = 0; i < 8; i++)
        {
            unsigned int base = (x[i] >> 3) >> 3;
            unsigned int m = x[i] & 63;

            unsigned long long u64_0 = ((unsigned long long*)pool)[base];
            unsigned long long u64_1 = ((unsigned long long*)pool)[base + 1];

            // Move 8 * 8 * j to the current segment. 8 * i to current 8 bytes element
            if (m == 0)
            {
                // some compiler doesn't work with bit shift 64
                *((unsigned long long*) & output[j * 8 * 8 + i * 8]) = u64_0;
            }
            else
            {
                *((unsigned long long*) & output[j * 8 * 8 + i * 8]) = (u64_0 >> m) | (u64_1 << (64 - m));
            }

            // Increase the positions in the pool for each element.
            x[i] = x[i] * 1664525 + 1013904223; // https://en.wikipedia.org/wiki/Linear_congruential_generator#Parameters_in_common_use
        }
    }

}

// Clamp the neuron value
template  <typename T>
static T clampNeuron(T val)
{
    if (val > NEURON_VALUE_LIMIT)
    {
        return NEURON_VALUE_LIMIT;
    }
    else if (val < -NEURON_VALUE_LIMIT)
    {
        return -NEURON_VALUE_LIMIT;
    }
    return val;
}

static void extract64Bits(unsigned long long number, char* output)
{
    int count = 0;
    for (int i = 0; i < 64; ++i)
    {
        output[i] = ((number >> i) & 1);
    }
}


template <
    unsigned long long numberOfInputNeurons, // K
    unsigned long long numberOfOutputNeurons,// L
    unsigned long long numberOfTicks,        // N
    unsigned long long numberOfNeighbors,    // 2M
    unsigned long long populationThreshold,  // P
    unsigned long long numberOfMutations,    // S
    unsigned int solutionThreshold,
    unsigned long long solutionBufferCount
>
struct ScoreFunction
{
    static constexpr unsigned long long numberOfNeurons = numberOfInputNeurons + numberOfOutputNeurons;
    static constexpr unsigned long long maxNumberOfNeurons = populationThreshold;
    static constexpr unsigned long long maxNumberOfSynapses = populationThreshold * numberOfNeighbors;
    static constexpr unsigned long long initNumberOfSynapses = numberOfNeurons * numberOfNeighbors;
    static constexpr long long radius = (long long)numberOfNeighbors / 2;
    static constexpr long long paddingNeuronsCount = maxNumberOfNeurons + numberOfNeighbors;

    static_assert(numberOfInputNeurons % 64 == 0, "numberOfInputNeurons must be divided by 64");
    static_assert(numberOfOutputNeurons % 64 == 0, "numberOfOutputNeurons must be divided by 64");
    static_assert(maxNumberOfSynapses <= (0xFFFFFFFFFFFFFFFF << 1ULL), "maxNumberOfSynapses must less than or equal MAX_UINT64/2");
    static_assert(initNumberOfSynapses % 32 == 0, "initNumberOfSynapses must be divided by 32");
    static_assert(numberOfNeighbors % 2 == 0, "numberOfNeighbors must be divided by 2");
    static_assert(populationThreshold > numberOfNeurons, "populationThreshold must be greater than numberOfNeurons");
    static_assert(numberOfNeurons > numberOfNeighbors, "Number of neurons must be greater than the number of neighbors");
    static_assert(numberOfNeighbors < ((1ULL << 63) - 1), "numberOfNeighbors must be in long long range");

    // Intermediate data
    struct InitValue
    {
        unsigned long long outputNeuronPositions[numberOfOutputNeurons];
        unsigned long long synapseWeight[initNumberOfSynapses / 32]; // each 64bits elements will decide value of 32 synapses
        unsigned long long synpaseMutation[numberOfMutations];
    };

    struct MiningData
    {
        unsigned long long inputNeuronRandomNumber[numberOfInputNeurons / 64];  // each bit will use for generate input neuron value
        unsigned long long outputNeuronRandomNumber[numberOfOutputNeurons / 64]; // each bit will use for generate expected output neuron value
    };
    static constexpr unsigned long long paddingInitValueSizeInBytes = (sizeof(InitValue) + 64 - 1) / 64 * 64;

    volatile char random2PoolLock;
    unsigned char state[STATE_SIZE];
    unsigned char externalPoolVec[POOL_VEC_PADDING_SIZE];
    unsigned char poolVec[POOL_VEC_PADDING_SIZE];

    void initPool(const unsigned char* miningSeed)
    {
        // Init random2 pool with mining seed
        generateRandom2Pool(miningSeed, state, externalPoolVec);
    }

    struct computeBuffer
    {
        typedef char Synapse;
        typedef char Neuron;
        typedef unsigned char NeuronType;

 
        // Data for roll back
        struct ANN
        {
            void init()
            {
                neurons = paddingNeurons + radius;
            }
            void prepareData()
            {
                // Padding start and end of neuron array
                neurons = paddingNeurons + radius;
                copyMem(paddingNeurons, neurons + population - radius, radius * sizeof(Neuron));
                copyMem(paddingNeurons + radius + population, neurons, radius * sizeof(Neuron));
            }

            void copyDataTo(ANN& rOther)
            {
                copyMem(rOther.neurons, neurons, population * sizeof(Neuron));
                copyMem(rOther.neuronTypes, neuronTypes, population * sizeof(NeuronType));
                copyMem(rOther.synapses, synapses, maxNumberOfSynapses * sizeof(Synapse));

                rOther.population = population;
            }

            Neuron* neurons;
            // Padding start and end of neurons so that we can reduce the condition checking
            Neuron paddingNeurons[maxNumberOfNeurons + numberOfNeighbors];
            NeuronType neuronTypes[maxNumberOfNeurons];
            Synapse synapses[maxNumberOfSynapses];

            unsigned long long population;
        };
        ANN bestANN;
        ANN currentANN;

        // Intermediate data
        unsigned char paddingInitValue[paddingInitValueSizeInBytes];
        MiningData miningData;

        unsigned long long neuronIndices[numberOfNeurons];
        Neuron previousNeuronValue[maxNumberOfNeurons];

        Neuron outputNeuronExpectedValue[numberOfOutputNeurons];

        Neuron neuronValueBuffer[maxNumberOfNeurons];
        unsigned char hash[32];
        unsigned char combined[64];

        unsigned long long removalNeuronsCount;

        // Contain incomming synapse of neurons. The center one will be zeros
        Synapse incommingSynapses[maxNumberOfSynapses + maxNumberOfNeurons];


        void mutate(unsigned long long mutateStep)
        {
            // Mutation
            unsigned long long population = currentANN.population;
            unsigned long long synapseCount = population * numberOfNeighbors;
            Synapse* synapses = currentANN.synapses;
            InitValue* initValue = (InitValue*)paddingInitValue;

            // Randomly pick a synapse, randomly increase or decrease its weight by 1 or -1
            unsigned long long synapseMutation = initValue->synpaseMutation[mutateStep];
            unsigned long long synapseIdx = (synapseMutation >> 1) % synapseCount;
            // Randomly increase or decrease its value
            char weightChange = 0;
            if ((synapseMutation & 1ULL) == 0)
            {
                weightChange = -1;
            }
            else
            {
                weightChange = 1;
            }

            char newWeight = synapses[synapseIdx] + weightChange;

            // Valid weight. Update it
            if (newWeight >= -1 && newWeight <= 1)
            {
                synapses[synapseIdx] = newWeight;
            }
            else // Invalid weight. Insert a neuron
            {
                // Insert the neuron
                insertNeuron(synapseIdx);
            }

            // Clean the ANN
            while (scanRedundantNeurons() > 0)
            {
                cleanANN();
            }
        }

        // Get the pointer to all outgoing synapse of a neurons
        Synapse* getSynapses(unsigned long long neuronIndex)
        {
            return &currentANN.synapses[neuronIndex * numberOfNeighbors];
        }

        // Circulate the neuron index
        unsigned long long clampNeuronIndex(long long neuronIdx, long long value)
        {
            const long long population = (long long)currentANN.population;
            long long nnIndex = neuronIdx + value;

            if (nnIndex >= population)
            {
                nnIndex -= population;
            }
            else if (nnIndex < 0)
            {
                nnIndex += population;
            }
            return (unsigned long long)nnIndex;
        }


        // Remove a neuron and all synapses relate to it
        void removeNeuron(unsigned long long neuronIdx)
        {
            // Scan all its neigbor to remove their outgoing synapse point to the neuron
            for (long long neighborOffset = -(long long)numberOfNeighbors / 2; neighborOffset <= (long long)numberOfNeighbors / 2; neighborOffset++)
            {
                unsigned long long nnIdx = clampNeuronIndex(neuronIdx, neighborOffset);
                Synapse* pNNSynapses = getSynapses(nnIdx);

                long long synapseIndexOfNN = getIndexInSynapsesBuffer(nnIdx, -neighborOffset);
                if (synapseIndexOfNN < 0)
                {
                    continue;
                }

                // The synapse array need to be shifted regard to the remove neuron
                // Also neuron need to have 2M neighbors, the addtional synapse will be set as zero weight
                // Case1 [S0 S1 S2 - SR S5 S6]. SR is removed, [S0 S1 S2 S5 S6 0]
                // Case2 [S0 S1 SR - S3 S4 S5]. SR is removed, [0 S0 S1 S3 S4 S5]
                if (synapseIndexOfNN >= numberOfNeighbors / 2)
                {
                    for (long long k = synapseIndexOfNN; k < numberOfNeighbors - 1; ++k)
                    {
                        pNNSynapses[k] = pNNSynapses[k + 1];
                    }
                    pNNSynapses[numberOfNeighbors - 1] = 0;
                }
                else
                {
                    for (long long k = synapseIndexOfNN; k > 0; --k)
                    {
                        pNNSynapses[k] = pNNSynapses[k - 1];
                    }
                    pNNSynapses[0] = 0;
                }
            }

            // Shift the synapse array and the neuron array, also reduce the current ANN population
            currentANN.population--;
            for (unsigned long long shiftIdx = neuronIdx; shiftIdx < currentANN.population; shiftIdx++)
            {
                currentANN.neurons[shiftIdx] = currentANN.neurons[shiftIdx + 1];
                currentANN.neuronTypes[shiftIdx] = currentANN.neuronTypes[shiftIdx + 1];

                // Also shift the synapses
                copyMem(getSynapses(shiftIdx), getSynapses(shiftIdx + 1), numberOfNeighbors * sizeof(Synapse));
            }
        }

        unsigned long long getNeighborNeuronIndex(unsigned long long neuronIndex, unsigned long long neighborOffset)
        {
            unsigned long long nnIndex = 0;
            if (neighborOffset < (numberOfNeighbors / 2))
            {
                nnIndex = clampNeuronIndex(neuronIndex + neighborOffset, -(long long)numberOfNeighbors / 2);
            }
            else
            {
                nnIndex = clampNeuronIndex(neuronIndex + neighborOffset + 1, -(long long)numberOfNeighbors / 2);
            }
            return nnIndex;
        }

        void updateSynapseOfInsertedNN(unsigned long long insertedNeuronIdx)
        {
            // The change of synapse only impact neuron in [originalNeuronIdx - numberOfNeighbors / 2 + 1, originalNeuronIdx +  numberOfNeighbors / 2]
            // In the new index, it will be  [originalNeuronIdx + 1 - numberOfNeighbors / 2, originalNeuronIdx + 1 + numberOfNeighbors / 2]
            // [N0 N1 N2 original inserted N4 N5 N6], M = 2.
            for (long long delta = -(long long)numberOfNeighbors / 2; delta <= (long long)numberOfNeighbors / 2; ++delta)
            {
                // Only process the neigbors
                if (delta == 0)
                {
                    continue;
                }
                unsigned long long updatedNeuronIdx = clampNeuronIndex(insertedNeuronIdx, delta);

                // Generate a list of neighbor index of current updated neuron NN
                // Find the location of the inserted neuron in the list of neighbors
                long long insertedNeuronIdxInNeigborList = -1;
                for (long long k = 0; k < numberOfNeighbors; k++)
                {
                    unsigned long long nnIndex = getNeighborNeuronIndex(updatedNeuronIdx, k);
                    if (nnIndex == insertedNeuronIdx)
                    {
                        insertedNeuronIdxInNeigborList = k;
                    }
                }

                ASSERT(insertedNeuronIdxInNeigborList >= 0);

                Synapse* pUpdatedSynapses = getSynapses(updatedNeuronIdx);
                // [N0 N1 N2 original inserted N4 N5 N6], M = 2.
                // Case: neurons in range [N0 N1 N2 original], right synapses will be affected
                if (delta < 0)
                {
                    // Left side is kept as it is, only need to shift to the right side
                    for (long long k = numberOfNeighbors - 1; k >= insertedNeuronIdxInNeigborList; --k)
                    {
                        // Updated synapse
                        pUpdatedSynapses[k] = pUpdatedSynapses[k - 1];
                    }

                    // Incomming synapse from original neuron -> inserted neuron must be zero
                    if (delta == -1)
                    {
                        pUpdatedSynapses[insertedNeuronIdxInNeigborList] = 0;
                    }
                }
                else // Case: neurons in range [inserted N4 N5 N6], left synapses will be affected
                {
                    // Right side is kept as it is, only need to shift to the left side
                    for (long long k = 0; k < insertedNeuronIdxInNeigborList; ++k)
                    {
                        // Updated synapse
                        pUpdatedSynapses[k] = pUpdatedSynapses[k + 1];
                    }
                }

            }
        }

        void insertNeuron(unsigned long long synapseIdx)
        {
            // A synapse have incomingNeighbor and outgoingNeuron, direction incomingNeuron -> outgoingNeuron
            unsigned long long incomingNeighborSynapseIdx = synapseIdx % numberOfNeighbors;
            unsigned long long outgoingNeuron = synapseIdx / numberOfNeighbors;

            Synapse* synapses = currentANN.synapses;
            Neuron* neurons = currentANN.neurons;
            NeuronType* neuronTypes = currentANN.neuronTypes;
            unsigned long long& population = currentANN.population;

            // Copy original neuron to the inserted one and set it as  EVOLUTION_NEURON_TYPE type
            Neuron insertNeuron = neurons[outgoingNeuron];
            unsigned long long insertedNeuronIdx = outgoingNeuron + 1;

            Synapse originalWeight = synapses[synapseIdx];

            // Insert the neuron into array, population increased one, all neurons next to original one need to shift right
            for (unsigned long long i = population; i > outgoingNeuron; --i)
            {
                neurons[i] = neurons[i - 1];
                neuronTypes[i] = neuronTypes[i - 1];

                // Also shift the synapses to the right
                copyMem(getSynapses(i), getSynapses(i - 1), numberOfNeighbors * sizeof(Synapse));
            }
            neurons[insertedNeuronIdx] = insertNeuron;
            neuronTypes[insertedNeuronIdx] = EVOLUTION_NEURON_TYPE;
            population++;

            // Try to update the synapse of inserted neuron. All outgoing synapse is init as zero weight
            Synapse* pInsertNeuronSynapse = getSynapses(insertedNeuronIdx);
            for (unsigned long long synIdx = 0; synIdx < numberOfNeighbors; ++synIdx)
            {
                pInsertNeuronSynapse[synIdx] = 0;
            }

            // Copy the outgoing synapse of original neuron
            // Outgoing points to the left
            if (incomingNeighborSynapseIdx < numberOfNeighbors / 2)
            {
                if (incomingNeighborSynapseIdx > 0)
                {
                    // Decrease by one because the new neuron is next to the original one
                    pInsertNeuronSynapse[incomingNeighborSynapseIdx - 1] = originalWeight;
                }
                // Incase of the outgoing synapse point too far, don't add the synapse
            }
            else
            {
                // No need to adjust the added neuron but need to remove the synapse of the original neuron
                pInsertNeuronSynapse[incomingNeighborSynapseIdx] = originalWeight;
            }

            updateSynapseOfInsertedNN(insertedNeuronIdx);
        }

        long long getIndexInSynapsesBuffer(unsigned long long neuronIdx, long long neighborOffset)
        {
            // Skip the case neuron point to itself and too far neighbor
            if (neighborOffset == 0
                || neighborOffset < -(long long)numberOfNeighbors / 2
                || neighborOffset >(long long)numberOfNeighbors / 2)
            {
                return -1;
            }

            long long synapseIdx = (long long)numberOfNeighbors / 2 + neighborOffset;
            if (neighborOffset >= 0)
            {
                synapseIdx = synapseIdx - 1;
            }

            return synapseIdx;
        }

        bool isAllOutgoingSynapsesZeros(unsigned long long neuronIdx)
        {
            Synapse* synapse = getSynapses(neuronIdx);
            for (unsigned long long n = 0; n < numberOfNeighbors; n++)
            {
                Synapse synapseW = synapse[n];
                if (synapseW != 0)
                {
                    return false;
                }
            }
            return true;
        }

        bool isAllIncomingSynapsesZeros(unsigned long long neuronIdx)
        {
            // Loop through the neighbor neurons to check all incoming synapses
            for (long long neighborOffset = -(long long)numberOfNeighbors / 2; neighborOffset <= (long long)numberOfNeighbors / 2; neighborOffset++)
            {
                unsigned long long nnIdx = clampNeuronIndex(neuronIdx, neighborOffset);
                Synapse* nnSynapses = getSynapses(nnIdx);

                long long synapseIdx = getIndexInSynapsesBuffer(nnIdx, -neighborOffset);
                if (synapseIdx < 0)
                {
                    continue;
                }
                Synapse synapseW = nnSynapses[synapseIdx];

                if (synapseW != 0)
                {
                    return false;
                }
            }
            return true;
        }

        // Check which neurons/synapse need to be removed after mutation
        unsigned long long scanRedundantNeurons()
        {
            unsigned long long population = currentANN.population;
            Synapse* synapses = currentANN.synapses;
            Neuron* neurons = currentANN.neurons;
            NeuronType* neuronTypes = currentANN.neuronTypes;

            removalNeuronsCount = 0;
            // After each mutation, we must verify if there are neurons that do not affect the ANN output.
            // These are neurons that either have all incoming synapse weights as 0,
            // or all outgoing synapse weights as 0. Such neurons must be removed.
            for (unsigned long long i = 0; i < population; i++)
            {
                if (neuronTypes[i] == EVOLUTION_NEURON_TYPE)
                {
                    if (isAllOutgoingSynapsesZeros(i) || isAllIncomingSynapsesZeros(i))
                    {
                        neuronIndices[removalNeuronsCount] = i;
                        removalNeuronsCount++;
                    }
                }
            }
            return removalNeuronsCount;
        }

        // Remove neurons and synapses that do not affect the ANN
        void cleanANN()
        {
            // Scan and remove neurons/synapses
            for (unsigned long long i = 0; i < removalNeuronsCount; i++)
            {
                unsigned long long neuronIdx = neuronIndices[i];
                // Remove it from the neuron list. Overwrite data
                // Remove its synapses in the synapses array
                removeNeuron(neuronIdx);
            }
            removalNeuronsCount = 0;
        }

        void processTick()
        {
            unsigned long long population = currentANN.population;

            // Prepare the padding regions
            currentANN.prepareData();

            // Memset value of current one
            setMem(neuronValueBuffer, sizeof(neuronValueBuffer), 0);
            Neuron* pPaddingNeurons = currentANN.paddingNeurons;
            Synapse* synapses = incommingSynapses;
            Neuron* neurons = currentANN.neurons;

            for (unsigned long long n = 0; n < population; ++n, pPaddingNeurons++, synapses += (numberOfNeighbors + 1))
            {
                int neuronValue = 0;
                long long m = 0;
#if defined (__AVX512F__)
                const __m512i zeros512 = _mm512_setzero_si512();
                for (; m + BATCH_SIZE <= numberOfNeighbors; m += BATCH_SIZE)
                {
                    const __m512i neurons512 = _mm512_loadu_si512((const __m512i*)(pPaddingNeurons + m));
                    const __m512i synapses512 = _mm512_loadu_si512((const __m512i*)(synapses + m));

                    __mmask64 nonZerosMask = _mm512_cmpneq_epi8_mask(synapses512, zeros512) & _mm512_cmpneq_epi8_mask(neurons512, zeros512);
                    __mmask64 negMask = nonZerosMask & _mm512_cmpgt_epi8_mask(zeros512, _mm512_xor_si512(synapses512, neurons512));
                    __mmask64 posMask = nonZerosMask & ~negMask;

                    neuronValue += popcnt64(posMask);
                    neuronValue -= popcnt64(negMask);
                }

#elif defined(__AVX2__)
                const __m256i zeros256 = _mm256_setzero_si256();
                const __m256i allOnes256 = _mm256_set1_epi8(-1);
                unsigned int negMask = 0;
                unsigned int posMask = 0;
                for (; m + BATCH_SIZE <= numberOfNeighbors; m += BATCH_SIZE)
                {
                    const __m256i neurons256 = _mm256_loadu_si256((const __m256i*)(pPaddingNeurons + m));
                    const __m256i synapses256 = _mm256_loadu_si256((const __m256i*)(synapses + m));

                    __m256i nonZeros256 = _mm256_andnot_si256(_mm256_or_si256(_mm256_cmpeq_epi8(neurons256, zeros256), _mm256_cmpeq_epi8(synapses256, zeros256)), allOnes256);
                    __m256i neg256 = _mm256_cmpgt_epi8(zeros256, _mm256_and_si256(_mm256_xor_si256(synapses256, neurons256), nonZeros256));
                    __m256i pos256 = _mm256_andnot_si256(neg256, nonZeros256);

                    int tmp = _mm256_movemask_epi8(neg256);
                    negMask = *((unsigned int*) &tmp);

                    tmp = _mm256_movemask_epi8(pos256);
                    posMask = *((unsigned int*) &tmp);
                    
                    neuronValue -= popcnt32(negMask);
                    neuronValue += popcnt32(posMask);
                }

#endif

                for (; m <= numberOfNeighbors; ++m)
                {
                    const Synapse synapseWeight = synapses[m];
                    const Neuron nVal = pPaddingNeurons[m];

                    // Weight-sum
                    neuronValue += synapseWeight * nVal;
                }

                neuronValueBuffer[n] = (Neuron)clampNeuron(neuronValue);
            }

            copyMem(neurons, neuronValueBuffer, population * sizeof(Neuron));
        }

        void runTickSimulation()
        {
            unsigned long long population = currentANN.population;
            Synapse* synapses = currentANN.synapses;
            Neuron* neurons = currentANN.neurons;
            NeuronType* neuronTypes = currentANN.neuronTypes;

            // Save the neuron value for comparison
            for (unsigned long long i = 0; i < population; ++i)
            {
                // Backup the neuron value
                previousNeuronValue[i] = neurons[i];
            }

            // Compute the incomming synapse of each neurons
           
            for (unsigned long long n = 0; n < population; ++n)
            {
               const Synapse* kSynapses = getSynapses(n);
               // Scan through all neighbor neurons and sum all connected neurons.
               // The synapses are arranged as neuronIndex * numberOfNeighbors
               for (long long m = 0; m < radius; m++)
               {
                   Synapse synapseWeight = kSynapses[m];
                   unsigned long long nnIndex =  clampNeuronIndex(n + m, -(long long)numberOfNeighbors / 2);
                   incommingSynapses[nnIndex * (numberOfNeighbors + 1) + (numberOfNeighbors - m)] = synapseWeight; // need to pad 1
               }

               incommingSynapses[n * (numberOfNeighbors + 1) + radius] = 0;

               for (long long m = radius; m < numberOfNeighbors; m++)
               {
                   Synapse synapseWeight = kSynapses[m];
                   unsigned long long nnIndex = clampNeuronIndex(n + m + 1, -(long long)numberOfNeighbors / 2);
                   incommingSynapses[nnIndex * (numberOfNeighbors + 1) + (numberOfNeighbors - m - 1)] = synapseWeight;
               }
            }

            for (unsigned long long tick = 0; tick < numberOfTicks; ++tick)
            {
                processTick();
                // Check exit conditions:
                // - N ticks have passed (already in for loop)
                // - All neuron values are unchanged
                // - All output neurons have non-zero values
                bool shouldExit = true;
                bool allNeuronsUnchanged = true;
                bool allOutputNeuronsIsNonZeros = true;
                for (unsigned long long n = 0; n < population; ++n)
                {
                    // Neuron unchanged check
                    if (previousNeuronValue[n] != neurons[n])
                    {
                        allNeuronsUnchanged = false;
                    }

                    // Ouput neuron value check
                    if (neuronTypes[n] == OUTPUT_NEURON_TYPE && neurons[n] == 0)
                    {
                        allOutputNeuronsIsNonZeros = false;
                    }
                }

                if (allOutputNeuronsIsNonZeros || allNeuronsUnchanged)
                {
                    break;
                }

                // Copy the neuron value
                copyMem(previousNeuronValue, neurons, population * sizeof(Neuron));
            }
        }

        unsigned int computeNonMatchingOutput()
        {
            unsigned long long population = currentANN.population;
            Neuron* neurons = currentANN.neurons;
            NeuronType* neuronTypes = currentANN.neuronTypes;

            // Compute the non-matching value R between output neuron value and initial value
            // Because the output neuron order never changes, the order is preserved
            unsigned int R = 0;
            unsigned long long outputIdx = 0;
            for (unsigned long long i = 0; i < population; i++)
            {
                if (neuronTypes[i] == OUTPUT_NEURON_TYPE)
                {
                    if (neurons[i] != outputNeuronExpectedValue[outputIdx])
                    {
                        R++;
                    }
                    outputIdx++;
                }
            }
            return R;
        }

        void initInputNeuron()
        {
            unsigned long long population = currentANN.population;
            Neuron* neurons = currentANN.neurons;
            NeuronType* neuronTypes = currentANN.neuronTypes;
            Neuron neuronArray[64] = { 0 };
            unsigned long long inputNeuronInitIndex = 0;
            for (unsigned long long i = 0; i < population; ++i)
            {
                // Input will use the init value
                if (neuronTypes[i] == INPUT_NEURON_TYPE)
                {
                    // Prepare new pack
                    if (inputNeuronInitIndex % 64 == 0)
                    {
                        extract64Bits(miningData.inputNeuronRandomNumber[inputNeuronInitIndex / 64], neuronArray);
                    }
                    char neuronValue = neuronArray[inputNeuronInitIndex % 64];

                    // Convert value of neuron to trits (keeping 1 as 1, and changing 0 to -1.).
                    neurons[i] = (neuronValue == 0) ? -1 : neuronValue;

                    inputNeuronInitIndex++;
                }
            }
        }

        void initNeuronValue()
        {
            initInputNeuron();

            // Starting value of output neuron is zero
            Neuron* neurons = currentANN.neurons;
            NeuronType* neuronTypes = currentANN.neuronTypes;
            unsigned long long population = currentANN.population;
            for (unsigned long long i = 0; i < population; ++i)
            {
                if (neuronTypes[i] == OUTPUT_NEURON_TYPE)
                {
                    neurons[i] = 0;
                }
            }
        }

        void initNeuronType()
        {
            unsigned long long population = currentANN.population;
            Neuron* neurons = currentANN.neurons;
            NeuronType* neuronTypes = currentANN.neuronTypes;
            InitValue* initValue = (InitValue*)paddingInitValue;

            // Randomly choose the positions of neurons types
            for (unsigned long long i = 0; i < population; ++i)
            {
                neuronIndices[i] = i;
                neuronTypes[i] = INPUT_NEURON_TYPE;
            }
            unsigned long long neuronCount = population;
            for (unsigned long long i = 0; i < numberOfOutputNeurons; ++i)
            {
                unsigned long long outputNeuronIdx = initValue->outputNeuronPositions[i] % neuronCount;

                // Fill the neuron type
                neuronTypes[neuronIndices[outputNeuronIdx]] = OUTPUT_NEURON_TYPE;

                // This index is used, copy the end of indices array to current position and decrease the number of picking neurons
                neuronCount = neuronCount - 1;
                neuronIndices[outputNeuronIdx] = neuronIndices[neuronCount];
            }
        }

        void initExpectedOutputNeuron()
        {
            Neuron neuronArray[64] = { 0 };
            for (unsigned long long i = 0; i < numberOfOutputNeurons; ++i)
            {
                // Prepare new pack
                if (i % 64 == 0)
                {
                    extract64Bits(miningData.outputNeuronRandomNumber[i / 64], neuronArray);
                }
                char neuronValue = neuronArray[i % 64];
                // Convert value of neuron (keeping 1 as 1, and changing 0 to -1.).
                outputNeuronExpectedValue[i] = (neuronValue == 0) ? -1 : neuronValue;
            }
        }

        void initializeRandom2(
            const unsigned char* publicKey, 
            const unsigned char* nonce,
            const unsigned char* pRandom2Pool)
        {
            copyMem(combined, publicKey, 32);
            copyMem(combined + 32, nonce, 32);
            KangarooTwelve(combined, 64, hash, 32);

            // Initalize with nonce and public key
            {
                random2(hash, pRandom2Pool, paddingInitValue, paddingInitValueSizeInBytes);

                copyMem((unsigned char*)&miningData, pRandom2Pool, sizeof(MiningData));
            }

        }

        unsigned int initializeANN()
        {
            currentANN.init();
            currentANN.population = numberOfNeurons;
            bestANN.init();
            bestANN.population = numberOfNeurons;

            unsigned long long& population = currentANN.population;
            Synapse* synapses = currentANN.synapses;
            Neuron* neurons = currentANN.neurons;
            InitValue* initValue = (InitValue*)paddingInitValue;

            // Initialization
            population = numberOfNeurons;
            removalNeuronsCount = 0;

            // Synapse weight initialization
            for (unsigned long long i = 0; i < (initNumberOfSynapses / 32); ++i)
            {
                const unsigned long long mask = 0b11;
                for (int j = 0; j < 32; ++j)
                {
                    int shiftVal = j * 2;
                    unsigned char extractValue = (unsigned char)((initValue->synapseWeight[i] >> shiftVal) & mask);
                    switch (extractValue)
                    {
                        case 2: synapses[32 * i + j] = -1; break;
                        case 3: synapses[32 * i + j] = 1; break;
                        default: synapses[32 * i + j] = 0;
                    }
                }
            }

            // Init the neuron type positions in ANN
            initNeuronType();

            // Init input neuron value and output neuron
            initNeuronValue();

            // Init expected output neuron
            initExpectedOutputNeuron();

            // Ticks simulation
            runTickSimulation();

            // Copy the state for rollback later
            currentANN.copyDataTo(bestANN);

            // Compute R and roll back if neccessary
            unsigned int R = computeNonMatchingOutput();

            return R;

        }

        // Main function for mining
        unsigned int computeScore(const unsigned char* publicKey, const unsigned char* nonce, const unsigned char* pRandom2Pool)
        {
            // Setup the random starting point 
            initializeRandom2(publicKey, nonce, pRandom2Pool);
            
            // Initialize
            unsigned int bestR = initializeANN();

            for (unsigned long long s = 0; s < numberOfMutations; ++s)
            {

                // Do the mutation
                mutate(s);

                // Exit if the number of population reaches the maximum allowed
                if (currentANN.population >= populationThreshold)
                {
                    break;
                }

                // Ticks simulation
                runTickSimulation();

                // Compute R and roll back if neccessary
                unsigned int R = computeNonMatchingOutput();
                if (R > bestR)
                {
                    // Roll back
                    //copyMem(&currentANN, &bestANN, sizeof(ANN));
                    bestANN.copyDataTo(currentANN);
                }
                else
                {
                    bestR = R;

                    // Better R. Save the state
                    //copyMem(&bestANN, &currentANN, sizeof(ANN));
                    currentANN.copyDataTo(bestANN);
                }

                ASSERT(bestANN.population <= populationThreshold);
            }

            unsigned int score = numberOfOutputNeurons - bestR;
            return score;
        }

    } _computeBuffer[solutionBufferCount];
    m256i currentRandomSeed;

    volatile char solutionEngineLock[solutionBufferCount];

#if USE_SCORE_CACHE
    volatile char scoreCacheLock;
    ScoreCache<SCORE_CACHE_SIZE, SCORE_CACHE_COLLISION_RETRIES> scoreCache;
#endif

    void initMiningData(m256i randomSeed)
    {
        // Below assume when a new mining seed is provided, we need to re-calculate the random2 pool
        // Check if random pool need to be re-generated
        if (!isZero(randomSeed))
        {
            initPool(randomSeed.m256i_u8);
        }
        currentRandomSeed = randomSeed; // persist the initial random seed to be able to send it back on system info response

        ACQUIRE(random2PoolLock);
        copyMem(poolVec, externalPoolVec, POOL_VEC_PADDING_SIZE);
        RELEASE(random2PoolLock);
    }

    ~ScoreFunction()
    {
        freeMemory();
    }

    void freeMemory()
    {
    }

    bool initMemory()
    {
        random2PoolLock = 0;
        setMem(_computeBuffer, sizeof(_computeBuffer), 0);

        for (int i = 0; i < solutionBufferCount; i++)
        {
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

    bool isValidScore(unsigned int solutionScore)
    {
        return (solutionScore >= 0 && solutionScore <= numberOfOutputNeurons);
    }
    bool isGoodScore(unsigned int solutionScore, int threshold)
    {
        return (threshold <= numberOfOutputNeurons) && (solutionScore >= (unsigned int)threshold);
    }

    unsigned int computeScore(const unsigned long long solutionBufIdx, const m256i& publicKey, const m256i& nonce)
    {
        return _computeBuffer[solutionBufIdx].computeScore(publicKey.m256i_u8, nonce.m256i_u8, poolVec);
    }

    // main score function
    unsigned int operator()(const unsigned long long processor_Number, const m256i& publicKey, const m256i& miningSeed, const m256i& nonce)
    {
        PROFILE_SCOPE();

        if (isZero(miningSeed) || miningSeed != currentRandomSeed)
        {
            return numberOfOutputNeurons + 1; // return invalid score
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

        score = computeScore(solutionBufIdx, publicKey, nonce);

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


