#pragma once

#include "../src/platform/memory.h"
#include "../src/four_q.h"

////////// Original (reference) scoring algorithm \\\\\\\\\\

template<
    unsigned int dataLength,
    unsigned int numberOfHiddenNeurons,
    unsigned int numberOfNeighborNeurons,
    unsigned int maxDuration,
    unsigned int solutionBufferCount
>
struct ScoreReferenceImplementation
{
    static constexpr unsigned long long synapseInputSize = ((unsigned long long)numberOfHiddenNeurons + dataLength) * numberOfNeighborNeurons;
    long long miningData[dataLength];

    //neuron only has values [-1, 0, 1]
    struct
    {
        char input[dataLength + numberOfHiddenNeurons + dataLength];
        char neuronBuffer[dataLength + numberOfHiddenNeurons + dataLength];
    } _neurons[solutionBufferCount];
    struct Synapse
    {
        char* inputLength;
    };

    Synapse _synapses[solutionBufferCount];

    void initMemory()
    {
        for (int i = 0; i < solutionBufferCount; i++)
        {
            allocatePool(synapseInputSize, (void**)&(_synapses[i].inputLength));
            setMem(_synapses[i].inputLength, synapseInputSize, 0);
            setMem(_neurons[i].neuronBuffer, sizeof(_neurons[i].neuronBuffer), 0);
            setMem(_neurons[i].input, sizeof(_neurons[i].input), 0);
        }
    }

    void freeMemory()
    {
        for (int i = 0; i < solutionBufferCount; i++)
        {
            freePool(_synapses[i].inputLength);
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
        auto& neuronBufferInput = neurons.neuronBuffer;
        auto& synapses = _synapses[processorNumber];
        memset(neurons.input, 0, sizeof(neurons.input));
        random(publicKey, nonce, (unsigned char*)synapses.inputLength, synapseInputSize);
        for (unsigned long long synapseIndex = 0; synapseIndex < (numberOfHiddenNeurons + dataLength) * numberOfNeighborNeurons; synapseIndex++)
        {
            if (synapses.inputLength[synapseIndex] == -128)
            {
                synapses.inputLength[synapseIndex] = 0;
            }
        }

        for (int i = 0; i < dataLength; i++)
        {
            neurons.input[i] = (char)(miningData[i]);
        }

        for (int tick = 1; tick <= maxDuration; tick++)
        {
            copyMem(&neuronBufferInput[0], &neurons.input[0], sizeof(neurons.input));
            for (unsigned long long inputNeuronIndex = 0; inputNeuronIndex < numberOfHiddenNeurons + dataLength; inputNeuronIndex++)
            {
                for (unsigned long long i = 0; i < numberOfNeighborNeurons; i++)
                {
                    const unsigned long long offset = inputNeuronIndex * numberOfNeighborNeurons + i;
                    if (synapses.inputLength[offset] != 0
                        && tick % synapses.inputLength[offset] == 0)
                    {
                        unsigned long long anotherInputNeuronIndex = (inputNeuronIndex + 1 + i) % (dataLength + numberOfHiddenNeurons + dataLength);
                        if (synapses.inputLength[offset] > 0)
                        {
                            neurons.input[dataLength + inputNeuronIndex] += neuronBufferInput[anotherInputNeuronIndex];
                        }
                        else
                        {
                            neurons.input[dataLength + inputNeuronIndex] -= neuronBufferInput[anotherInputNeuronIndex];
                        }

                        if (neurons.input[dataLength + inputNeuronIndex] > NEURON_VALUE_LIMIT)
                        {
                            neurons.input[dataLength + inputNeuronIndex] = NEURON_VALUE_LIMIT;
                        }
                        if (neurons.input[dataLength + inputNeuronIndex] < -NEURON_VALUE_LIMIT)
                        {
                            neurons.input[dataLength + inputNeuronIndex] = -NEURON_VALUE_LIMIT;
                        }
                    }
                }
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
};
