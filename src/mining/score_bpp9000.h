#pragma once

#include "score_common.h"
#include "task_file.h"

// bpp9000 scorer: a recurrent ternary ANN (trits {0,1,2}, 2 = UNKNOWN) predicting a windowed series;
// only the per-neuron LUTs change under mutation. Freestanding node port of the reference in
// test/score_bpp9000_reference.h - the two must stay bit-exact.
namespace score_engine
{

template<typename Params>
struct ScoreBpp9000
{
    static constexpr unsigned long long numberOfInputNeurons = Params::numberOfInputNeurons;
    static constexpr unsigned long long numberOfOutputNeurons = Params::numberOfOutputNeurons;
    static constexpr unsigned long long sequenceLength = Params::sequenceLength;
    static constexpr unsigned long long windowWidth = Params::windowWidth;
    static constexpr unsigned long long maxNumberOfTicks = Params::maxNumberOfTicks;
    static constexpr unsigned long long numberOfNeighbors = Params::numberOfNeighbors;
    static constexpr unsigned long long populationThreshold = Params::populationThreshold;
    static constexpr unsigned long long numberOfMutations = Params::numberOfMutations;
    static constexpr unsigned int solutionThreshold = Params::solutionThreshold;

    static constexpr unsigned long long maxNumberOfNeurons = populationThreshold;
    static constexpr unsigned long long numberOfWindows = sequenceLength - windowWidth;

    static constexpr unsigned char TRIT_UNKNOWN = 2;
    static constexpr unsigned int INFINITE_ERROR = 0xFFFFFFFFU;
    static constexpr unsigned long long lutSize = 27;
    static constexpr unsigned int MAX_LUT_ENTRIES_PER_STEP = 10;

    // The node random2 requires the draw size to be a multiple of 64 bytes. Pad each draw up to the next
    // multiple and use only the leading (actual) bytes; this yields exactly the same leading bytes as the
    // reference's internally-padded random2, so the two implementations stay bit-exact.
    static constexpr unsigned long long lutInitBytes = maxNumberOfNeurons * lutSize;
    static constexpr unsigned long long lutInitPaddedBytes = ((lutInitBytes + 63) / 64) * 64;
    static constexpr unsigned long long mutationSeedCount = numberOfMutations * MAX_LUT_ENTRIES_PER_STEP;
    static constexpr unsigned long long mutationSeedBytes = mutationSeedCount * sizeof(unsigned long long);
    static constexpr unsigned long long mutationSeedPaddedBytes = ((mutationSeedBytes + 63) / 64) * 64;

    static_assert(
        numberOfNeighbors == 3,
        "the LUT index is hardcoded for 3 neighbours");
    static_assert(
        populationThreshold > numberOfInputNeurons + numberOfOutputNeurons + 1,
        "populationThreshold must leave room for the evolution neurons and the signal neuron");
    static_assert(
        (populationThreshold & (populationThreshold - 1)) == 0,
        "populationThreshold must be a power of 2");
    static_assert(
        windowWidth >= 2 && windowWidth < sequenceLength,
        "windowWidth must be at least 2 and leave room for the target after the window");
    static_assert(
        numberOfOutputNeurons == 1,
        "score() grades only output neuron 0");
    static_assert(
        maxNumberOfTicks > windowWidth,
        "maxNumberOfTicks must exceed windowWidth so a window can be fully fed before timing out");

    struct Neuron
    {
        enum Type
        {
            kInput,
            kOutput,
            kEvolution,
        };
        Type type;
        unsigned char value;
    };

    struct ANN
    {
        Neuron neurons[maxNumberOfNeurons];
        unsigned char lut[maxNumberOfNeurons * lutSize];
    };
    ANN bestANN;
    ANN currentANN;
    ANN prevANN;

    struct InitValue
    {
        unsigned char lutInit[lutInitPaddedBytes];
        unsigned long long mutationSeed[mutationSeedPaddedBytes / sizeof(unsigned long long)];
    } initValue;

    unsigned char inputs[sequenceLength][numberOfInputNeurons];
    unsigned char outputs[sequenceLength][numberOfOutputNeurons];

    unsigned char nextNeuronValue[maxNumberOfNeurons];

    unsigned int neighborIndices[populationThreshold * numberOfNeighbors];

    unsigned int inputNeuronIndices[numberOfInputNeurons];
    unsigned int outputNeuronIndices[numberOfOutputNeurons];

    unsigned int signalNeuronIndex;

    typename Neuron::Type neuronTypes[maxNumberOfNeurons];

    unsigned long long updatedNeuronIndices[maxNumberOfNeurons];
    unsigned long long numberOfUpdatedNeurons;

    void initMemory()
    {
    }

    // In-memory task load: parse/validate the topology and unpack the data block directly (no file I/O).
    bool loadTaskFromMemory(const unsigned char* topoBlock, const unsigned char* dataBlock)
    {
        score_task_file::parseTopologyBlock(topoBlock, numberOfInputNeurons, numberOfOutputNeurons, populationThreshold, numberOfNeighbors,
                                      inputNeuronIndices, outputNeuronIndices, &signalNeuronIndex, neighborIndices);
        if (!validateTopology())
        {
            return false;
        }
        if (!score_task_file::unpackDataBlock(numberOfInputNeurons, numberOfOutputNeurons, sequenceLength, dataBlock, &inputs[0][0], &outputs[0][0]))
        {
            return false;
        }
        deriveNeuronRoles();
        return true;
    }

    bool validateTopology()
    {
        for (unsigned long long i = 0; i < numberOfInputNeurons; ++i)
        {
            if (inputNeuronIndices[i] >= populationThreshold)
            {
                return false;
            }
        }
        for (unsigned long long i = 0; i < numberOfOutputNeurons; ++i)
        {
            if (outputNeuronIndices[i] >= populationThreshold)
            {
                return false;
            }
        }
        if (signalNeuronIndex >= populationThreshold)
        {
            return false;
        }

        bool seen[populationThreshold] = {};
        for (unsigned long long i = 0; i < numberOfInputNeurons; ++i)
        {
            if (seen[inputNeuronIndices[i]])
            {
                return false;
            }
            seen[inputNeuronIndices[i]] = true;
        }
        for (unsigned long long i = 0; i < numberOfOutputNeurons; ++i)
        {
            if (seen[outputNeuronIndices[i]])
            {
                return false;
            }
            seen[outputNeuronIndices[i]] = true;
        }
        if (seen[signalNeuronIndex])
        {
            return false;
        }

        for (unsigned long long i = 0; i < populationThreshold * numberOfNeighbors; ++i)
        {
            if (neighborIndices[i] >= populationThreshold)
            {
                return false;
            }
        }
        return true;
    }

    void deriveNeuronRoles()
    {
        for (unsigned long long i = 0; i < populationThreshold; ++i)
        {
            neuronTypes[i] = Neuron::kEvolution;
        }
        for (unsigned long long i = 0; i < numberOfInputNeurons; ++i)
        {
            neuronTypes[inputNeuronIndices[i]] = Neuron::kInput;
        }
        for (unsigned long long i = 0; i < numberOfOutputNeurons; ++i)
        {
            neuronTypes[outputNeuronIndices[i]] = Neuron::kOutput;
        }

        numberOfUpdatedNeurons = 0;
        for (unsigned long long i = 0; i < populationThreshold; ++i)
        {
            if (neuronTypes[i] != Neuron::kInput)
            {
                updatedNeuronIndices[numberOfUpdatedNeurons] = i;
                numberOfUpdatedNeurons++;
            }
        }
    }

    // One inference tick: each non-input neuron looks up its next trit from its 3 neighbours.
    void processTick()
    {
        const unsigned long long population = populationThreshold;
        Neuron* neurons = currentANN.neurons;

        for (unsigned long long n = 0; n < population; ++n)
        {
            if (Neuron::kInput == neurons[n].type)
            {
                nextNeuronValue[n] = neurons[n].value;
                continue;
            }

            // Explicit per-neuron neighbours; base-3 LUT index = t0 + 3*t1 + 9*t2.
            const unsigned long long t0 = neurons[neighborIndices[n * numberOfNeighbors + 0]].value;
            const unsigned long long t1 = neurons[neighborIndices[n * numberOfNeighbors + 1]].value;
            const unsigned long long t2 = neurons[neighborIndices[n * numberOfNeighbors + 2]].value;
            nextNeuronValue[n] = currentANN.lut[n * lutSize + (t0 + 3 * t1 + 9 * t2)];
        }

        for (unsigned long long n = 0; n < population; ++n)
        {
            if (Neuron::kInput != neurons[n].type)
            {
                neurons[n].value = nextNeuronValue[n];
            }
        }
    }

    // Sliding-window self-clocked score: feed W samples (signal-paced), settle until the signal is
    // UNKNOWN again, then grade the output vs outputs[t+W]. A timed-out window fails the whole ANN.
    // Returns the failure count (lower is better), or INFINITE_ERROR on timeout.
    unsigned int score()
    {
        unsigned int numberOfFailures = 0;

        Neuron* neurons = currentANN.neurons;

        for (unsigned long long trainingEntryIndex = 0; trainingEntryIndex < numberOfWindows; ++trainingEntryIndex)
        {
            unsigned long long feedCounter = 0;

            for (unsigned long long n = 0; n < populationThreshold; ++n)
            {
                neurons[n].value = TRIT_UNKNOWN;
            }

            unsigned long long tick;
            for (tick = 0; tick < maxNumberOfTicks; tick++)
            {
                if (neurons[signalNeuronIndex].value == TRIT_UNKNOWN)
                {
                    // Whole window is in and the signal is ready -> output is settled, read it.
                    if (feedCounter >= windowWidth)
                    {
                        break;
                    }
                    for (unsigned long long i = 0; i < numberOfInputNeurons; ++i)
                    {
                        neurons[inputNeuronIndices[i]].value = inputs[trainingEntryIndex + feedCounter][i];
                    }
                    feedCounter++;
                }
                else
                {
                    for (unsigned long long i = 0; i < numberOfInputNeurons; ++i)
                    {
                        neurons[inputNeuronIndices[i]].value = TRIT_UNKNOWN;
                    }
                }

                processTick();
            }

            if (tick == maxNumberOfTicks)
            {
                return INFINITE_ERROR;
            }

            const unsigned char predicted = neurons[outputNeuronIndices[0]].value;
            const unsigned char expected = outputs[trainingEntryIndex + feedCounter][0];
            if (predicted != expected)
            {
                numberOfFailures++;
            }
        }

        return numberOfFailures;
    }

    // Flip one LUT entry of one non-input neuron (bit 0 picks the new trit, the rest picks the line).
    void mutate(unsigned long long mutationSeed)
    {
        const unsigned long long delta = mutationSeed & 1ULL;

        const unsigned long long totalLines = numberOfUpdatedNeurons * lutSize;
        const unsigned long long flatIdx = (mutationSeed >> 1) % totalLines;
        const unsigned long long neuronIdx = updatedNeuronIndices[flatIdx / lutSize];
        const unsigned long long line = flatIdx % lutSize;

        const unsigned char oldTrit = currentANN.lut[neuronIdx * lutSize + line];
        const unsigned char newTrit = (unsigned char)((oldTrit + 1 + delta) % 3);
        currentANN.lut[neuronIdx * lutSize + line] = newTrit;
    }

    // Seed the ANN: root LUT from the pubkey alone (each computor's fixed root); mutation seeds from
    // pubkey+nonce (nonce[0..2] are the algo/L/K knobs, excluded from the RNG). Returns the start score.
    unsigned int initializeANN(const unsigned char* publicKey, const unsigned char* nonce, const unsigned char* pRandom2Pool)
    {
        const unsigned long long population = populationThreshold;
        Neuron* neurons = currentANN.neurons;

        unsigned char rootHash[32];
        KangarooTwelve(publicKey, 32, rootHash, 32);
        random2(rootHash, pRandom2Pool, (unsigned char*)&initValue.lutInit, lutInitPaddedBytes);

        unsigned char searchHash[32];
        unsigned char combined[64];
        copyMem(combined, publicKey, 32);
        copyMem(combined + 32, nonce, 32);
        combined[32] = 0;
        combined[33] = 0;
        combined[34] = 0;
        KangarooTwelve(combined, 64, searchHash, 32);
        random2(searchHash, pRandom2Pool, (unsigned char*)&initValue.mutationSeed, mutationSeedPaddedBytes);

        for (unsigned long long i = 0; i < population; ++i)
        {
            neurons[i].type = neuronTypes[i];
            neurons[i].value = TRIT_UNKNOWN;
        }

        for (unsigned long long n = 0; n < population; ++n)
        {
            for (unsigned long long line = 0; line < lutSize; ++line)
            {
                currentANN.lut[n * lutSize + line] = (unsigned char)(initValue.lutInit[n * lutSize + line] % 3);
            }
        }

        return score();
    }

    // Anti-attractor search: L mutations/step; accept worse-or-equal for the first K steps (explore),
    // then better-or-equal (exploit); one-step rollback; keep and return the best score found. Returns the
    // failure count (lower is better), or numberOfWindows on timeout. K is fixed to 0 in the pre-ant phase
    // (see below), so every step is exploit (greedy descent).
    unsigned int computeScore(const unsigned char* publicKey, const unsigned char* nonce, const unsigned char* pRandom2Pool)
    {
        unsigned int L = nonce[1];
        if (L < 1)
        {
            L = 1;
        }
        if (L > MAX_LUT_ENTRIES_PER_STEP)
        {
            L = MAX_LUT_ENTRIES_PER_STEP;
        }
        unsigned long long K = nonce[2];
        if (K > numberOfMutations)
        {
            K = numberOfMutations;
        }

        unsigned int cur = initializeANN(publicKey, nonce, pRandom2Pool);
        copyMem(&bestANN, &currentANN, sizeof(bestANN));
        unsigned int best = cur;

        for (unsigned long long s = 0; s < numberOfMutations; ++s)
        {
            copyMem(&prevANN, &currentANN, sizeof(prevANN));

            for (unsigned int i = 0; i < L; ++i)
            {
                mutate(initValue.mutationSeed[s * MAX_LUT_ENTRIES_PER_STEP + i]);
            }

            const unsigned int r = score();

            bool accept = false;
            if (s < K)
            {
                accept = (r >= cur);
            }
            else
            {
                accept = (r <= cur);
            }

            if (accept)
            {
                cur = r;
            }
            else
            {
                copyMem(&currentANN, &prevANN, sizeof(currentANN));
            }

            if (cur < best)
            {
                best = cur;
                copyMem(&bestANN, &currentANN, sizeof(bestANN));
            }
        }
        // Error semantics (smaller is better); a timeout maps to the worst in-range value.
        return (best == INFINITE_ERROR) ? (unsigned int)numberOfWindows : best;
    }

    int getLastOutput(unsigned char* requestedOutput, int requestedSizeInBytes)
    {
        return 0;
    }
};

}
