#pragma once

#include "../src/platform/memory.h"
#include "../src/four_q.h"

////////// Original (reference) scoring algorithm \\\\\\\\\\

template<
    unsigned int dataLength,
    unsigned int infoLength,
    unsigned int numberOfInputNeurons,
    unsigned int numberOfOutputNeurons,
    unsigned int maxInputDuration,
    unsigned int maxOutputDuration,
    unsigned int solutionBufferCount
>
struct ScoreReferenceImplementation
{
    long long miningData[dataLength];
    struct
    {
        long long input[dataLength + numberOfInputNeurons + infoLength];
        long long output[infoLength + numberOfOutputNeurons + dataLength];
    } _neurons[solutionBufferCount];
    struct
    {
        char inputLength[(numberOfInputNeurons + infoLength) * (dataLength + numberOfInputNeurons + infoLength)];
        char outputLength[(numberOfOutputNeurons + dataLength) * (infoLength + numberOfOutputNeurons + dataLength)];
    } _synapses[solutionBufferCount];


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

    static inline void clampNeuron(long long& val)
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
        memset(&neurons, 0, sizeof(neurons));
        random(publicKey, nonce, (unsigned char*)&synapses, sizeof(synapses));
        for (unsigned int inputNeuronIndex = 0; inputNeuronIndex < numberOfInputNeurons + infoLength; inputNeuronIndex++)
        {
            for (unsigned int anotherInputNeuronIndex = 0; anotherInputNeuronIndex < dataLength + numberOfInputNeurons + infoLength; anotherInputNeuronIndex++)
            {
                const unsigned int offset = inputNeuronIndex * (dataLength + numberOfInputNeurons + infoLength) + anotherInputNeuronIndex;
                if (synapses.inputLength[offset] == -128)
                {
                    synapses.inputLength[offset] = 0;
                }
            }
        }
        for (unsigned int outputNeuronIndex = 0; outputNeuronIndex < numberOfOutputNeurons + dataLength; outputNeuronIndex++)
        {
            for (unsigned int anotherOutputNeuronIndex = 0; anotherOutputNeuronIndex < infoLength + numberOfOutputNeurons + dataLength; anotherOutputNeuronIndex++)
            {
                const unsigned int offset = outputNeuronIndex * (infoLength + numberOfOutputNeurons + dataLength) + anotherOutputNeuronIndex;
                if (synapses.outputLength[offset] == -128)
                {
                    synapses.outputLength[offset] = 0;
                }
            }
        }
        for (unsigned int inputNeuronIndex = 0; inputNeuronIndex < numberOfInputNeurons + infoLength; inputNeuronIndex++)
        {
            synapses.inputLength[inputNeuronIndex * (dataLength + numberOfInputNeurons + infoLength) + (dataLength + inputNeuronIndex)] = 0;
        }
        for (unsigned int outputNeuronIndex = 0; outputNeuronIndex < numberOfOutputNeurons + dataLength; outputNeuronIndex++)
        {
            synapses.outputLength[outputNeuronIndex * (infoLength + numberOfOutputNeurons + dataLength) + (infoLength + outputNeuronIndex)] = 0;
        }

        memcpy(&neurons.input[0], &miningData, sizeof(miningData));

        for (int tick = 1; tick <= maxInputDuration; tick++)
        {
            for (unsigned int inputNeuronIndex = 0; inputNeuronIndex < numberOfInputNeurons + infoLength; inputNeuronIndex++)
            {
                for (unsigned int anotherInputNeuronIndex = 0; anotherInputNeuronIndex < dataLength + numberOfInputNeurons + infoLength; anotherInputNeuronIndex++)
                {
                    const unsigned int offset = inputNeuronIndex * (dataLength + numberOfInputNeurons + infoLength) + anotherInputNeuronIndex;
                    if ((synapses.inputLength[offset] != 0)
                        && (tick % synapses.inputLength[offset] == 0))
                    {
                        if (synapses.inputLength[offset] > 0)
                        {
                            neurons.input[dataLength + inputNeuronIndex] += neurons.input[anotherInputNeuronIndex];
                        }
                        else
                        {
                            neurons.input[dataLength + inputNeuronIndex] -= neurons.input[anotherInputNeuronIndex];
                        }
                        clampNeuron(neurons.input[dataLength + inputNeuronIndex]);
                    }
                }
            }
        }

        for (unsigned int i = 0; i < infoLength; i++)
        {
            neurons.output[i] = (neurons.input[dataLength + numberOfInputNeurons + i] >= 0 ? 1 : -1);
        }

        for (int tick = 1; tick <= maxOutputDuration; tick++)
        {
            for (unsigned int outputNeuronIndex = 0; outputNeuronIndex < numberOfOutputNeurons + dataLength; outputNeuronIndex++)
            {
                for (unsigned int anotherOutputNeuronIndex = 0; anotherOutputNeuronIndex < infoLength + numberOfOutputNeurons + dataLength; anotherOutputNeuronIndex++)
                {
                    const unsigned int offset = outputNeuronIndex * (infoLength + numberOfOutputNeurons + dataLength) + anotherOutputNeuronIndex;
                    if (synapses.outputLength[offset] != 0
                        && tick % synapses.outputLength[offset] == 0)
                    {
                        if (synapses.outputLength[offset] > 0)
                        {
                            neurons.output[infoLength + outputNeuronIndex] += neurons.output[anotherOutputNeuronIndex];
                        }
                        else
                        {
                            neurons.output[infoLength + outputNeuronIndex] -= neurons.output[anotherOutputNeuronIndex];
                        }
                        clampNeuron(neurons.output[infoLength + outputNeuronIndex]);
                    }
                }
            }
        }

        unsigned int score = 0;

        for (unsigned int i = 0; i < dataLength; i++)
        {
            if (miningData[i] == neurons.output[infoLength + numberOfOutputNeurons + i])
            {
                score++;
            }
        }

        return score;
    }
};
