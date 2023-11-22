#pragma once

#include "../src/platform/memory.h"
#include "../src/four_q.h"

////////// Original (reference) scoring algorithm \\\\\\\\\\

template<
    unsigned int dataLength,
    unsigned int infoLength,
    unsigned int numberOfInputNeutrons,
    unsigned int numberOfOutputNeutrons,
    unsigned int maxInputDuration,
    unsigned int maxOutputDuration,
    unsigned int maxNumberOfProcessors
>
struct ScoreReferenceImplementation
{
    int miningData[dataLength];
    struct
    {
        int input[dataLength + numberOfInputNeutrons + infoLength];
        int output[infoLength + numberOfOutputNeutrons + dataLength];
    } neurons[maxNumberOfProcessors];
    struct
    {
        char input[(numberOfInputNeutrons + infoLength) * (dataLength + numberOfInputNeutrons + infoLength)];
        char output[(numberOfOutputNeutrons + dataLength) * (infoLength + numberOfOutputNeutrons + dataLength)];
        unsigned short lengths[maxInputDuration * (numberOfInputNeutrons + infoLength) + maxOutputDuration * (numberOfOutputNeutrons + dataLength)];
    } synapses[maxNumberOfProcessors];

    void initMiningData()
    {
        unsigned char randomSeed[32];
        setMem(randomSeed, 32, 0);
        randomSeed[0] = RANDOM_SEED0;
        randomSeed[1] = RANDOM_SEED1;
        randomSeed[2] = RANDOM_SEED2;
        randomSeed[3] = RANDOM_SEED3;
        randomSeed[4] = RANDOM_SEED4;
        randomSeed[5] = RANDOM_SEED5;
        randomSeed[6] = RANDOM_SEED6;
        randomSeed[7] = RANDOM_SEED7;
        random(randomSeed, randomSeed, (unsigned char*)miningData, sizeof(miningData));
    }

    unsigned int operator()(unsigned long long processorNumber, unsigned char* publicKey, unsigned char* nonce)
    {
        processorNumber %= maxNumberOfProcessors;
        random(publicKey, nonce, (unsigned char*)&synapses[processorNumber], sizeof(synapses[0]));
        for (unsigned int inputNeuronIndex = 0; inputNeuronIndex < numberOfInputNeutrons + infoLength; inputNeuronIndex++)
        {
            for (unsigned int anotherInputNeuronIndex = 0; anotherInputNeuronIndex < dataLength + numberOfInputNeutrons + infoLength; anotherInputNeuronIndex++)
            {
                const unsigned int offset = inputNeuronIndex * (dataLength + numberOfInputNeutrons + infoLength) + anotherInputNeuronIndex;
                synapses[processorNumber].input[offset] = (((unsigned char)synapses[processorNumber].input[offset]) % 3) - 1;
            }
        }
        for (unsigned int outputNeuronIndex = 0; outputNeuronIndex < numberOfOutputNeutrons + dataLength; outputNeuronIndex++)
        {
            for (unsigned int anotherOutputNeuronIndex = 0; anotherOutputNeuronIndex < infoLength + numberOfOutputNeutrons + dataLength; anotherOutputNeuronIndex++)
            {
                const unsigned int offset = outputNeuronIndex * (infoLength + numberOfOutputNeutrons + dataLength) + anotherOutputNeuronIndex;
                synapses[processorNumber].output[offset] = (((unsigned char)synapses[processorNumber].output[offset]) % 3) - 1;
            }
        }
        for (unsigned int inputNeuronIndex = 0; inputNeuronIndex < numberOfInputNeutrons + infoLength; inputNeuronIndex++)
        {
            synapses[processorNumber].input[inputNeuronIndex * (dataLength + numberOfInputNeutrons + infoLength) + (dataLength + inputNeuronIndex)] = 0;
        }
        for (unsigned int outputNeuronIndex = 0; outputNeuronIndex < numberOfOutputNeutrons + dataLength; outputNeuronIndex++)
        {
            synapses[processorNumber].output[outputNeuronIndex * (infoLength + numberOfOutputNeutrons + dataLength) + (infoLength + outputNeuronIndex)] = 0;
        }

        unsigned int lengthIndex = 0;

        copyMem(&neurons[processorNumber].input[0], &miningData, sizeof(miningData));
        setMem(&neurons[processorNumber].input[sizeof(miningData) / sizeof(neurons[0].input[0])], sizeof(neurons[0]) - sizeof(miningData), 0);

        for (unsigned int tick = 0; tick < maxInputDuration; tick++)
        {
            unsigned short neuronIndices[numberOfInputNeutrons + infoLength];
            unsigned short numberOfRemainingNeurons = 0;
            for (numberOfRemainingNeurons = 0; numberOfRemainingNeurons < numberOfInputNeutrons + infoLength; numberOfRemainingNeurons++)
            {
                neuronIndices[numberOfRemainingNeurons] = numberOfRemainingNeurons;
            }
            while (numberOfRemainingNeurons)
            {
                const unsigned short neuronIndexIndex = synapses[processorNumber].lengths[lengthIndex++] % numberOfRemainingNeurons;
                const unsigned short inputNeuronIndex = neuronIndices[neuronIndexIndex];
                neuronIndices[neuronIndexIndex] = neuronIndices[--numberOfRemainingNeurons];
                for (unsigned short anotherInputNeuronIndex = 0; anotherInputNeuronIndex < dataLength + numberOfInputNeutrons + infoLength; anotherInputNeuronIndex++)
                {
                    int value = neurons[processorNumber].input[anotherInputNeuronIndex] >= 0 ? 1 : -1;
                    value *= synapses[processorNumber].input[inputNeuronIndex * (dataLength + numberOfInputNeutrons + infoLength) + anotherInputNeuronIndex];
                    neurons[processorNumber].input[dataLength + inputNeuronIndex] += value;
                }
            }
        }

        copyMem(&neurons[processorNumber].output[0], &neurons[processorNumber].input[dataLength + numberOfInputNeutrons], infoLength * sizeof(neurons[0].input[0]));

        for (unsigned int tick = 0; tick < maxOutputDuration; tick++)
        {
            unsigned short neuronIndices[numberOfOutputNeutrons + dataLength];
            unsigned short numberOfRemainingNeurons = 0;
            for (numberOfRemainingNeurons = 0; numberOfRemainingNeurons < numberOfOutputNeutrons + dataLength; numberOfRemainingNeurons++)
            {
                neuronIndices[numberOfRemainingNeurons] = numberOfRemainingNeurons;
            }
            while (numberOfRemainingNeurons)
            {
                const unsigned short neuronIndexIndex = synapses[processorNumber].lengths[lengthIndex++] % numberOfRemainingNeurons;
                const unsigned short outputNeuronIndex = neuronIndices[neuronIndexIndex];
                neuronIndices[neuronIndexIndex] = neuronIndices[--numberOfRemainingNeurons];
                for (unsigned int anotherOutputNeuronIndex = 0; anotherOutputNeuronIndex < infoLength + numberOfOutputNeutrons + dataLength; anotherOutputNeuronIndex++)
                {
                    int value = neurons[processorNumber].output[anotherOutputNeuronIndex] >= 0 ? 1 : -1;
                    value *= synapses[processorNumber].output[outputNeuronIndex * (infoLength + numberOfOutputNeutrons + dataLength) + anotherOutputNeuronIndex];
                    neurons[processorNumber].output[infoLength + outputNeuronIndex] += value;
                }
            }
        }

        unsigned int score = 0;

        for (unsigned int i = 0; i < dataLength; i++)
        {
            if ((miningData[i] >= 0) == (neurons[processorNumber].output[infoLength + numberOfOutputNeutrons + i] >= 0))
            {
                score++;
            }
        }

        return score;
    }
};
