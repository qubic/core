#pragma once

#include "../src/platform/memory.h"
#include "../src/four_q.h"

////////// Original (reference) scoring algorithm \\\\\\\\\\

template<
    unsigned int dataLength,
    unsigned int numberOfInputNeurons,
    unsigned int _numberOfOutputNeurons,
    unsigned int maxInputDuration,
    unsigned int _maxOutputDuration,
    unsigned int solutionBufferCount
>
struct ScoreReferenceImplementation
{
    static constexpr unsigned long long synapseInputSize = ((unsigned long long)numberOfInputNeurons + dataLength) * (dataLength + numberOfInputNeurons + dataLength);
    long long miningData[dataLength];

    //neuron only has values [-1, 0, 1]
    struct
    {
        char input[dataLength + numberOfInputNeurons + dataLength];
        char neuronBuffer[dataLength + numberOfInputNeurons + dataLength];
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
            allocatePool(synapseInputSize, (void**) & (_synapses[i].inputLength));
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

    void initMiningData()
    {
        unsigned char randomSeed[32];
        setMem(randomSeed, 32, 0);
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
        memset(&neurons, 0, sizeof(neurons));
        random(publicKey, nonce, (unsigned char*)synapses.inputLength, synapseInputSize);
        for (unsigned long long inputNeuronIndex = 0; inputNeuronIndex < numberOfInputNeurons + dataLength; inputNeuronIndex++)
        {
            for (unsigned long long anotherInputNeuronIndex = 0; anotherInputNeuronIndex < dataLength + numberOfInputNeurons + dataLength; anotherInputNeuronIndex++)
            {
                const unsigned long long offset = inputNeuronIndex * (dataLength + numberOfInputNeurons + dataLength) + anotherInputNeuronIndex;
                if (synapses.inputLength[offset] == -128)
                {
                    synapses.inputLength[offset] = 0;
                }
            }
        }
        for (unsigned long long inputNeuronIndex = 0; inputNeuronIndex < numberOfInputNeurons + dataLength; inputNeuronIndex++)
        {
            synapses.inputLength[inputNeuronIndex * (dataLength + numberOfInputNeurons + dataLength) + (dataLength + inputNeuronIndex)] = 0;
        }
        for (int i = 0; i < dataLength; i++)
        {
            neurons.input[i] = (char)(miningData[i]);
        }

        for (int tick = 1; tick <= maxInputDuration; tick++)
        {
            copyMem(&neuronBufferInput[0], &neurons.input[0], sizeof(neurons.input));
            for (unsigned long long inputNeuronIndex = 0; inputNeuronIndex < numberOfInputNeurons + dataLength; inputNeuronIndex++)
            {
                for (unsigned long long anotherInputNeuronIndex = 0; anotherInputNeuronIndex < dataLength + numberOfInputNeurons + dataLength; anotherInputNeuronIndex++)
                {
                    const unsigned long long offset = inputNeuronIndex * (dataLength + numberOfInputNeurons + dataLength) + anotherInputNeuronIndex;
                    if (synapses.inputLength[offset] != 0
                        && tick % synapses.inputLength[offset] == 0)
                    {
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
            if (miningData[i] == neurons.input[dataLength + numberOfInputNeurons + i])
            {
                score++;
            }
        }

        return score;
    }
};
