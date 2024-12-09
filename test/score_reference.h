#pragma once

#include "../src/platform/memory_util.h"
#include "../src/four_q.h"

////////// Original (reference) scoring algorithm \\\\\\\\\\

template<
    unsigned long long dataLength,
    unsigned long long numberOfHiddenNeurons,
    unsigned long long numberOfNeighborNeurons,
    unsigned long long maxDuration,
    unsigned long long numberOfOptimizationSteps,
    unsigned long long solutionBufferCount
>
struct ScoreReferenceImplementation
{
    static constexpr unsigned long long neuronsCount = dataLength + numberOfHiddenNeurons + dataLength;
    static constexpr unsigned long long synapseSignsCount = (dataLength + numberOfHiddenNeurons + dataLength) * numberOfNeighborNeurons / 64;
    static constexpr unsigned long long synapseInputCount = synapseSignsCount + maxDuration + numberOfOptimizationSteps;
    long long miningData[dataLength];

    static_assert(((dataLength + numberOfHiddenNeurons + dataLength) * numberOfNeighborNeurons) % 64 == 0, "Synapse signs need to be a multipler of 64");
    static_assert(numberOfOptimizationSteps < maxDuration, "Number of optimization steps need to smaller than maxDuration");

    //neuron only has values [-1, 0, 1]
    struct Neuron
    {
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
        unsigned long long* skipTicksNumber;
    };

    Neuron _neurons[solutionBufferCount];
    Synapse _synapses[solutionBufferCount];
    unsigned char* _poolBuffer[solutionBufferCount];

    // Save skipped ticks
    long long* _skipTicks[solutionBufferCount];

    // Contained all ticks possible value
    long long* _ticksNumbers[solutionBufferCount];


    void initMemory()
    {
        for (int i = 0; i < solutionBufferCount; i++)
        {
            allocPoolWithErrorLog(L"poolBuffer", RANDOM2_POOL_SIZE, (void**)&(_poolBuffer[i]), __LINE__);

            allocPoolWithErrorLog(L"neurons[i].input", sizeof(long long) * neuronsCount, (void**)(&(_neurons[i].input)), __LINE__);

            allocPoolWithErrorLog(L"synapses[i].data", sizeof(unsigned long long) * synapseInputCount, (void**)(&(_synapses[i].data)), __LINE__);
            _synapses[i].signs = _synapses[i].data;
            _synapses[i].sequence = _synapses[i].data + synapseSignsCount;
            _synapses[i].skipTicksNumber = _synapses[i].data + synapseSignsCount + maxDuration;


            allocPoolWithErrorLog(L"skipTicks[i]", sizeof(long long) * numberOfOptimizationSteps, (void**)&(_skipTicks[i]), __LINE__);
            allocPoolWithErrorLog(L"ticksNumbers[i]", sizeof(long long) * maxDuration, (void**)&(_ticksNumbers[i]), __LINE__);
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
            freePool(_ticksNumbers[i]);
            freePool(_skipTicks[i]);
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
        auto& ticksNumbers = _ticksNumbers[processorNumber];
        auto& skipTicks = _skipTicks[processorNumber];

        // Random generator
        random2(publicKey, nonce, (unsigned char*)synapses.data, synapseInputCount * sizeof(synapses.data[0]), random2PoolBuffer);

        unsigned int score = 0;
        long long tailTick = maxDuration - 1;
        for (long long tick = 0; tick < maxDuration; tick++)
        {
            ticksNumbers[tick] = tick;
        }

        for (long long l = 0; l < numberOfOptimizationSteps; l++)
        {
            skipTicks[l] = -1LL;
        }

        // Calculate the score with a list of randomly skipped ticks. This list grows if an additional skipped tick
        // does not worsen the score compared to the previous one.
        // - Initialize skippedTicks = []
        // - First, use all ticks. Compute score0 and update the score with score0.
        // - In the second run, ignore ticks in skippedTicks and try skipping a random tick 'a'.
        //    + Compute score1.
        //    + If score1 is not worse than score, add tick 'a' to skippedTicks and update the score with score1.
        //    + Otherwise, ignore tick 'a'.
        // - In the third run, ignore ticks in skippedTicks and try skipping a random tick 'b'.
        //    + Compute score2.
        //    + If score2 is not worse than score, add tick 'b' to skippedTicks and update the score with score2.
        //    + Otherwise, ignore tick 'b'.
        // - Continue this process iteratively.
        unsigned long long numberOfSkippedTicks = 0;
        long long skipTick = -1;
        for (long long l = 0; l < numberOfOptimizationSteps; l++)
        {
            // Reset the neurons
            memset(neurons.input, 0, sizeof(neurons.input[0]) * neuronsCount);
            memcpy(&neurons.input[0], miningData, sizeof(miningData));

            for (long long tick = 0; tick < maxDuration; tick++)
            {
                // Check if current tick should be skipped
                if (tick == skipTick)
                {
                    continue;
                }

                // Skip recorded skipped ticks
                bool tickShouldBeSkipped = false;
                for (unsigned long long tickIdx = 0; tickIdx < numberOfSkippedTicks; tickIdx++)
                {
                    if (skipTicks[tickIdx] == tick)
                    {
                        tickShouldBeSkipped = true;
                        break;
                    }
                }
                if (tickShouldBeSkipped)
                {
                    continue;
                }

                // Compute neurons
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

            // Compute the score
            unsigned int currentScore = 0;
            for (unsigned long long i = 0; i < dataLength; i++)
            {
                if (miningData[i] == neurons.input[dataLength + numberOfHiddenNeurons + i])
                {
                    currentScore++;
                }
            }

            // Update score if below satisfied
            // - This is the first run without skipping any ticks
            // - Current score is not worse than previous score
            if (skipTick == -1 || currentScore >= score)
            {
                score = currentScore;
                // For the first run, don't need to update the skipped ticks list
                if (skipTick != -1)
                {
                    skipTicks[numberOfSkippedTicks] = skipTick;
                    numberOfSkippedTicks++;
                }
            }

            // Randomly choose a tick to skip for the next round and avoid duplicated pick already chosen one
            long long randomTick = synapses.skipTicksNumber[l] % (maxDuration - l);
            skipTick = ticksNumbers[randomTick];
            // Replace the chosen tick position with current tail to make sure if this possiton is chosen again
            // the skipTick is still not duplicated with previous ones.
            ticksNumbers[randomTick] = ticksNumbers[tailTick];
            tailTick--;

        }
        return score;
    }
};
