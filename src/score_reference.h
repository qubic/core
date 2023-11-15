#pragma once

#include "platform/memory.h"
#include "public_settings.h"
#include "four_q.h"

////////// Original (reference) scoring algorithm \\\\\\\\\\

static int miningData[DATA_LENGTH];
static struct
{
    int input[DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + INFO_LENGTH];
    int output[INFO_LENGTH + NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH];
} neurons[MAX_NUMBER_OF_PROCESSORS];
static struct
{
    char input[(NUMBER_OF_INPUT_NEURONS + INFO_LENGTH) * (DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + INFO_LENGTH)];
    char output[(NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH) * (INFO_LENGTH + NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH)];
    unsigned short lengths[MAX_INPUT_DURATION * (NUMBER_OF_INPUT_NEURONS + INFO_LENGTH) + MAX_OUTPUT_DURATION * (NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH)];
} synapses[MAX_NUMBER_OF_PROCESSORS];

static unsigned int score(unsigned long long processorNumber, unsigned char* publicKey, unsigned char* nonce)
{
    processorNumber %= MAX_NUMBER_OF_PROCESSORS;
    random(publicKey, nonce, (unsigned char*)&synapses[processorNumber], sizeof(synapses[0]));
    for (unsigned int inputNeuronIndex = 0; inputNeuronIndex < NUMBER_OF_INPUT_NEURONS + INFO_LENGTH; inputNeuronIndex++)
    {
        for (unsigned int anotherInputNeuronIndex = 0; anotherInputNeuronIndex < DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + INFO_LENGTH; anotherInputNeuronIndex++)
        {
            const unsigned int offset = inputNeuronIndex * (DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + INFO_LENGTH) + anotherInputNeuronIndex;
            synapses[processorNumber].input[offset] = (((unsigned char)synapses[processorNumber].input[offset]) % 3) - 1;
        }
    }
    for (unsigned int outputNeuronIndex = 0; outputNeuronIndex < NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH; outputNeuronIndex++)
    {
        for (unsigned int anotherOutputNeuronIndex = 0; anotherOutputNeuronIndex < INFO_LENGTH + NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH; anotherOutputNeuronIndex++)
        {
            const unsigned int offset = outputNeuronIndex * (INFO_LENGTH + NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH) + anotherOutputNeuronIndex;
            synapses[processorNumber].output[offset] = (((unsigned char)synapses[processorNumber].output[offset]) % 3) - 1;
        }
    }
    for (unsigned int inputNeuronIndex = 0; inputNeuronIndex < NUMBER_OF_INPUT_NEURONS + INFO_LENGTH; inputNeuronIndex++)
    {
        synapses[processorNumber].input[inputNeuronIndex * (DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + INFO_LENGTH) + (DATA_LENGTH + inputNeuronIndex)] = 0;
    }
    for (unsigned int outputNeuronIndex = 0; outputNeuronIndex < NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH; outputNeuronIndex++)
    {
        synapses[processorNumber].output[outputNeuronIndex * (INFO_LENGTH + NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH) + (INFO_LENGTH + outputNeuronIndex)] = 0;
    }

    unsigned int lengthIndex = 0;

    copyMem(&neurons[processorNumber].input[0], &miningData, sizeof(miningData));
    setMem(&neurons[processorNumber].input[sizeof(miningData) / sizeof(neurons[0].input[0])], sizeof(neurons[0]) - sizeof(miningData), 0);

    for (unsigned int tick = 0; tick < MAX_INPUT_DURATION; tick++)
    {
        unsigned short neuronIndices[NUMBER_OF_INPUT_NEURONS + INFO_LENGTH];
        unsigned short numberOfRemainingNeurons = 0;
        for (numberOfRemainingNeurons = 0; numberOfRemainingNeurons < NUMBER_OF_INPUT_NEURONS + INFO_LENGTH; numberOfRemainingNeurons++)
        {
            neuronIndices[numberOfRemainingNeurons] = numberOfRemainingNeurons;
        }
        while (numberOfRemainingNeurons)
        {
            const unsigned short neuronIndexIndex = synapses[processorNumber].lengths[lengthIndex++] % numberOfRemainingNeurons;
            const unsigned short inputNeuronIndex = neuronIndices[neuronIndexIndex];
            neuronIndices[neuronIndexIndex] = neuronIndices[--numberOfRemainingNeurons];
            for (unsigned short anotherInputNeuronIndex = 0; anotherInputNeuronIndex < DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + INFO_LENGTH; anotherInputNeuronIndex++)
            {
                int value = neurons[processorNumber].input[anotherInputNeuronIndex] >= 0 ? 1 : -1;
                value *= synapses[processorNumber].input[inputNeuronIndex * (DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + INFO_LENGTH) + anotherInputNeuronIndex];
                neurons[processorNumber].input[DATA_LENGTH + inputNeuronIndex] += value;
            }
        }
    }

    copyMem(&neurons[processorNumber].output[0], &neurons[processorNumber].input[DATA_LENGTH + NUMBER_OF_INPUT_NEURONS], INFO_LENGTH * sizeof(neurons[0].input[0]));

    for (unsigned int tick = 0; tick < MAX_OUTPUT_DURATION; tick++)
    {
        unsigned short neuronIndices[NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH];
        unsigned short numberOfRemainingNeurons = 0;
        for (numberOfRemainingNeurons = 0; numberOfRemainingNeurons < NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH; numberOfRemainingNeurons++)
        {
            neuronIndices[numberOfRemainingNeurons] = numberOfRemainingNeurons;
        }
        while (numberOfRemainingNeurons)
        {
            const unsigned short neuronIndexIndex = synapses[processorNumber].lengths[lengthIndex++] % numberOfRemainingNeurons;
            const unsigned short outputNeuronIndex = neuronIndices[neuronIndexIndex];
            neuronIndices[neuronIndexIndex] = neuronIndices[--numberOfRemainingNeurons];
            for (unsigned int anotherOutputNeuronIndex = 0; anotherOutputNeuronIndex < INFO_LENGTH + NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH; anotherOutputNeuronIndex++)
            {
                int value = neurons[processorNumber].output[anotherOutputNeuronIndex] >= 0 ? 1 : -1;
                value *= synapses[processorNumber].output[outputNeuronIndex * (INFO_LENGTH + NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH) + anotherOutputNeuronIndex];
                neurons[processorNumber].output[INFO_LENGTH + outputNeuronIndex] += value;
            }
        }
    }

    unsigned int score = 0;

    for (unsigned int i = 0; i < DATA_LENGTH; i++)
    {
        if ((miningData[i] >= 0) == (neurons[processorNumber].output[INFO_LENGTH + NUMBER_OF_OUTPUT_NEURONS + i] >= 0))
        {
            score++;
        }
    }

    return score;
}