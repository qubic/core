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
        long long input[DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + INFO_LENGTH];
        long long output[INFO_LENGTH + NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH];
    } _neurons[solutionBufferCount];
    struct
    {
        char inputLength[(NUMBER_OF_INPUT_NEURONS + INFO_LENGTH) * (DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + INFO_LENGTH)];
        char outputLength[(NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH) * (INFO_LENGTH + NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH)];
    } _synapses[solutionBufferCount];


    void initMiningData()
    {
        unsigned char randomSeed[32];
        setMem(randomSeed, 32, 0);
        random(randomSeed, randomSeed, (unsigned char*)miningData, sizeof(miningData));
        for (unsigned int i = 0; i < DATA_LENGTH; i++)
        {
            miningData[i] = (miningData[i] >= 0 ? 1 : -1);
        }
    }

    unsigned int operator()(unsigned long long processorNumber, unsigned char* publicKey, unsigned char* nonce)
    {
        processorNumber %= solutionBufferCount;
        auto& neurons = _neurons[processorNumber];
        auto& synapses = _synapses[processorNumber];
        memset(&neurons, 0, sizeof(neurons));
        random(publicKey, nonce, (unsigned char*)&synapses, sizeof(synapses));
        for (unsigned int inputNeuronIndex = 0; inputNeuronIndex < NUMBER_OF_INPUT_NEURONS + INFO_LENGTH; inputNeuronIndex++)
        {
            for (unsigned int anotherInputNeuronIndex = 0; anotherInputNeuronIndex < DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + INFO_LENGTH; anotherInputNeuronIndex++)
            {
                const unsigned int offset = inputNeuronIndex * (DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + INFO_LENGTH) + anotherInputNeuronIndex;
                if (synapses.inputLength[offset] == -128)
                {
                    synapses.inputLength[offset] = 0;
                }
            }
        }
        for (unsigned int outputNeuronIndex = 0; outputNeuronIndex < NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH; outputNeuronIndex++)
        {
            for (unsigned int anotherOutputNeuronIndex = 0; anotherOutputNeuronIndex < INFO_LENGTH + NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH; anotherOutputNeuronIndex++)
            {
                const unsigned int offset = outputNeuronIndex * (INFO_LENGTH + NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH) + anotherOutputNeuronIndex;
                if (synapses.outputLength[offset] == -128)
                {
                    synapses.outputLength[offset] = 0;
                }
            }
        }
        for (unsigned int inputNeuronIndex = 0; inputNeuronIndex < NUMBER_OF_INPUT_NEURONS + INFO_LENGTH; inputNeuronIndex++)
        {
            synapses.inputLength[inputNeuronIndex * (DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + INFO_LENGTH) + (DATA_LENGTH + inputNeuronIndex)] = 0;
        }
        for (unsigned int outputNeuronIndex = 0; outputNeuronIndex < NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH; outputNeuronIndex++)
        {
            synapses.outputLength[outputNeuronIndex * (INFO_LENGTH + NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH) + (INFO_LENGTH + outputNeuronIndex)] = 0;
        }

        memcpy(&neurons.input[0], &miningData, sizeof(miningData));

        for (int tick = 1; tick <= MAX_INPUT_DURATION; tick++)
        {
            for (unsigned int inputNeuronIndex = 0; inputNeuronIndex < NUMBER_OF_INPUT_NEURONS + INFO_LENGTH; inputNeuronIndex++)
            {
                for (unsigned int anotherInputNeuronIndex = 0; anotherInputNeuronIndex < DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + INFO_LENGTH; anotherInputNeuronIndex++)
                {
                    const unsigned int offset = inputNeuronIndex * (DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + INFO_LENGTH) + anotherInputNeuronIndex;
                    if ((synapses.inputLength[offset] != 0)
                        && (tick % synapses.inputLength[offset] == 0))
                    {
                        if (synapses.inputLength[offset] > 0)
                        {
                            neurons.input[DATA_LENGTH + inputNeuronIndex] += neurons.input[anotherInputNeuronIndex];
                        }
                        else
                        {
                            neurons.input[DATA_LENGTH + inputNeuronIndex] -= neurons.input[anotherInputNeuronIndex];
                        }

                        if (neurons.input[DATA_LENGTH + inputNeuronIndex] > NEURON_VALUE_LIMIT)
                        {
                            neurons.input[DATA_LENGTH + inputNeuronIndex] = NEURON_VALUE_LIMIT;
                        }
                        if (neurons.input[DATA_LENGTH + inputNeuronIndex] <= -NEURON_VALUE_LIMIT)
                        {
                            neurons.input[DATA_LENGTH + inputNeuronIndex] = -NEURON_VALUE_LIMIT + 1;
                        }
                    }
                }
            }
        }

        for (unsigned int i = 0; i < INFO_LENGTH; i++)
        {
            neurons.output[i] = (neurons.input[DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + i] >= 0 ? 1 : -1);
        }

        for (int tick = 1; tick <= MAX_OUTPUT_DURATION; tick++)
        {
            for (unsigned int outputNeuronIndex = 0; outputNeuronIndex < NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH; outputNeuronIndex++)
            {
                for (unsigned int anotherOutputNeuronIndex = 0; anotherOutputNeuronIndex < INFO_LENGTH + NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH; anotherOutputNeuronIndex++)
                {
                    const unsigned int offset = outputNeuronIndex * (INFO_LENGTH + NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH) + anotherOutputNeuronIndex;
                    if (synapses.outputLength[offset] != 0
                        && tick % synapses.outputLength[offset] == 0)
                    {
                        if (synapses.outputLength[offset] > 0)
                        {
                            neurons.output[INFO_LENGTH + outputNeuronIndex] += neurons.output[anotherOutputNeuronIndex];
                        }
                        else
                        {
                            neurons.output[INFO_LENGTH + outputNeuronIndex] -= neurons.output[anotherOutputNeuronIndex];
                        }

                        if (neurons.output[INFO_LENGTH + outputNeuronIndex] > NEURON_VALUE_LIMIT)
                        {
                            neurons.output[INFO_LENGTH + outputNeuronIndex] = NEURON_VALUE_LIMIT;
                        }
                        if (neurons.output[INFO_LENGTH + outputNeuronIndex] <= -NEURON_VALUE_LIMIT)
                        {
                            neurons.output[INFO_LENGTH + outputNeuronIndex] = -NEURON_VALUE_LIMIT + 1;
                        }
                    }
                }
            }
        }

        unsigned int score = 0;

        for (unsigned int i = 0; i < DATA_LENGTH; i++)
        {
            if ((miningData[i] >= 0) == (neurons.output[INFO_LENGTH + NUMBER_OF_OUTPUT_NEURONS + i] >= 0))
            {
                score++;
            }
        }

        return score;
    }
};
