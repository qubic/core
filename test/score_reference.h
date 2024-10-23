#pragma once

#include "../src/platform/memory.h"
#include "../src/four_q.h"

////////// Original (reference) scoring algorithm \\\\\\\\\\

template<
    unsigned long long dataLength,
    unsigned long long numberOfHiddenNeurons,
    unsigned long long numberOfNeighborNeurons,
    unsigned long long maxDuration,
    unsigned long long solutionBufferCount
>
struct ScoreReferenceImplementation
{
    static constexpr unsigned long long neuronsCount = dataLength + numberOfHiddenNeurons + dataLength;
    static constexpr unsigned long long synapseSignsCount = (dataLength + numberOfHiddenNeurons + dataLength) * numberOfNeighborNeurons / 64;
    static constexpr unsigned long long synapseInputCount = synapseSignsCount + maxDuration;
    long long miningData[dataLength];

    //neuron only has values [-1, 0, 1]
    struct Neuron
    {
        //long long input[dataLength + numberOfHiddenNeurons + dataLength];
        long long* input;
    };
    struct Synapse
    {
        //unsigned long long signs[(dataLength + numberOfHiddenNeurons + dataLength) * numberOfNeighborNeurons / 64];
        //unsigned long long sequence[maxDuration];
        unsigned long long* data;

        // Pointer to data
        unsigned long long* signs;
        unsigned long long* sequence;
    };

    Neuron _neurons[solutionBufferCount];
    Synapse _synapses[solutionBufferCount];
    unsigned char* _poolBuffer[solutionBufferCount];

    void initMemory()
    {
        for (int i = 0; i < solutionBufferCount; i++)
        {
            allocatePool(RANDOM2_POOL_SIZE, (void**)&(_poolBuffer[i]));

            allocatePool(sizeof(long long) * neuronsCount, (void**)(&(_neurons[i].input)));

            allocatePool(sizeof(unsigned long long) * synapseInputCount, (void**)(&(_synapses[i].data)));
            _synapses[i].signs = _synapses[i].data;
            _synapses[i].sequence = _synapses[i].data + synapseSignsCount;
        }

        static_assert(synapseInputCount * sizeof(*(_synapses->data)) % 8 == 0, "Random2 require output size is a multiplier of 8");
    }

    void freeMemory()
    {
        for (int i = 0; i < solutionBufferCount; i++)
        {
            freePool(_poolBuffer[i]);
            freePool(_neurons[i].input);
            freePool(_synapses[i].data);
        }
    }

    ~ScoreReferenceImplementation()
    {
        freeMemory();
    }

    void initMiningData(m256i randomSeed)
    {
        random(randomSeed.m256i_u8, randomSeed.m256i_u8, (unsigned char*)miningData, sizeof(miningData));
        for (unsigned int i = 0; i < dataLength; i++)
        {
            miningData[i] = (miningData[i] >= 0 ? 1 : -1);
        }
    }

    void initMiningData(unsigned char* seed)
    {
        unsigned char randomSeed[32];
        copyMem(randomSeed, seed, 32);
        random(randomSeed, randomSeed, (unsigned char*)miningData, sizeof(miningData));
        for (unsigned int i = 0; i < dataLength; i++)
        {
            miningData[i] = (miningData[i] >= 0 ? 1 : -1);
        }
    }

    static inline void clampNeuron(char& val)
    {
        if (val > NEURON_VALUE_LIMIT) {
            val = NEURON_VALUE_LIMIT;
        }
        else if (val < -NEURON_VALUE_LIMIT) {
            val = -NEURON_VALUE_LIMIT;
        }
    }

    unsigned int operator()(unsigned long long processorNumber, unsigned char* publicKey, unsigned char* nonce)
    {
        processorNumber %= solutionBufferCount;
        auto& neurons = _neurons[processorNumber];
        auto& synapses = _synapses[processorNumber];
        auto& random2PoolBuffer = _poolBuffer[processorNumber];

        memset(neurons.input, 0, sizeof(neurons.input[0]) * neuronsCount);

        random2(publicKey, nonce, (unsigned char*)synapses.data, synapseInputCount * sizeof(synapses.data[0]), random2PoolBuffer);
        memcpy(&neurons.input[0], miningData, sizeof(miningData));

        for (long long tick = 0; tick < maxDuration; tick++)
        {
            const unsigned long long neuronIndex = dataLength + synapses.sequence[tick] % (numberOfHiddenNeurons + dataLength);
            const unsigned long long neighborNeuronIndex = (synapses.sequence[tick] / (numberOfHiddenNeurons + dataLength)) % numberOfNeighborNeurons;
            unsigned long long supplierNeuronIndex;
            if (neighborNeuronIndex < numberOfNeighborNeurons / 2)
            {
                supplierNeuronIndex = (neuronIndex - (numberOfNeighborNeurons / 2) + neighborNeuronIndex + (dataLength + numberOfHiddenNeurons + dataLength)) % (dataLength + numberOfHiddenNeurons + dataLength);
            }
            else
            {
                supplierNeuronIndex = (neuronIndex + 1 - (numberOfNeighborNeurons / 2) + neighborNeuronIndex + (dataLength + numberOfHiddenNeurons + dataLength)) % (dataLength + numberOfHiddenNeurons + dataLength);
            }
            const unsigned long long offset = neuronIndex * numberOfNeighborNeurons + neighborNeuronIndex;

            if (!(synapses.signs[offset / 64] & (1ULL << (offset % 64))))
            {
                neurons.input[neuronIndex] += neurons.input[supplierNeuronIndex];
            }
            else
            {
                neurons.input[neuronIndex] -= neurons.input[supplierNeuronIndex];
            }

            if (neurons.input[neuronIndex] > 1)
            {
                neurons.input[neuronIndex] = 1;
            }
            if (neurons.input[neuronIndex] < -1)
            {
                neurons.input[neuronIndex] = -1;
            }
        }

        unsigned int score = 0;
        for (unsigned long long i = 0; i < dataLength; i++)
        {
            if (miningData[i] == neurons.input[dataLength + numberOfHiddenNeurons + i])
            {
                score++;
            }
        }

        return score;
    }
};
