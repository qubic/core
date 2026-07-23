#pragma once

#include "../src/mining/score_common.h"
#include "../src/mining/task_file.h"
#include "score_common_reference.h"
#include "kangaroo_twelve.h"

#include <vector>
#include <cstring>

// Generic LUT scorer: a recurrent ternary ANN (trits {0,1,2}, 2 = UNKNOWN) predicting a windowed
// series; only the per-neuron LUTs change under mutation.
namespace score_bpp9000_reference
{

template <typename Params>
struct Miner
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

    static constexpr unsigned long long topoBlockSize =
        (numberOfInputNeurons + numberOfOutputNeurons + 1 + populationThreshold * numberOfNeighbors) * sizeof(uint32_t);
    static constexpr unsigned long long dataBlockSize =
        sequenceLength * (((numberOfInputNeurons + score_task_file::TRITS_PER_BYTE - 1) / score_task_file::TRITS_PER_BYTE)
                          + ((numberOfOutputNeurons + score_task_file::TRITS_PER_BYTE - 1) / score_task_file::TRITS_PER_BYTE));

    static constexpr unsigned char TRIT_UNKNOWN = 2;

    static constexpr unsigned int INFINITE_ERROR = 0xFFFFFFFFU;

    static constexpr unsigned long long lutSize = 27;

    static constexpr unsigned int MAX_LUT_ENTRIES_PER_STEP = 10;

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

    std::vector<unsigned char> poolVec;

    void initialize(const unsigned char miningSeed[32])
    {
        poolVec.resize(score_reference::POOL_VEC_PADDING_SIZE);
        score_reference::generateRandom2Pool(miningSeed, poolVec.data());
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

    unsigned char inputs[sequenceLength][numberOfInputNeurons];
    unsigned char outputs[sequenceLength][numberOfOutputNeurons];

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
        unsigned char lutInit[maxNumberOfNeurons * lutSize];
        unsigned long long mutationSeed[numberOfMutations * MAX_LUT_ENTRIES_PER_STEP];
    } initValue;

    unsigned char nextNeuronValue[maxNumberOfNeurons];

    unsigned int neighborIndices[populationThreshold * numberOfNeighbors];

    unsigned int inputNeuronIndices[numberOfInputNeurons];
    unsigned int outputNeuronIndices[numberOfOutputNeurons];

    unsigned int signalNeuronIndex;

    typename Neuron::Type neuronTypes[maxNumberOfNeurons];

    unsigned long long updatedNeuronIndices[maxNumberOfNeurons];
    unsigned long long numberOfUpdatedNeurons;

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
    unsigned int initializeANN(const unsigned char* publicKey, const unsigned char* nonce)
    {
        const unsigned long long population = populationThreshold;
        Neuron* neurons = currentANN.neurons;

        unsigned char rootHash[32];
        KangarooTwelve(publicKey, 32, rootHash, 32);
        score_reference::random2(rootHash, poolVec.data(), (unsigned char*)&initValue.lutInit, sizeof(initValue.lutInit));

        unsigned char searchHash[32];
        unsigned char combined[64];
        memcpy(combined, publicKey, 32);
        memcpy(combined + 32, nonce, 32);
        combined[32] = 0;
        combined[33] = 0;
        combined[34] = 0;
        KangarooTwelve(combined, 64, searchHash, 32);
        score_reference::random2(searchHash, poolVec.data(), (unsigned char*)&initValue.mutationSeed, sizeof(initValue.mutationSeed));

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
    // then better-or-equal (exploit); one-step rollback; keep and return the best score found.
    unsigned int computeScore(const unsigned char* publicKey, const unsigned char* nonce)
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
        // Pre-ant-colony phase, the anti-attractor (explore) is disabled to match the production scorer and
        // Qiner. nonce[2] is temporarily unused here; restore K = nonce[2] (clamped) when ants return.
        const unsigned long long K = 0;

        unsigned int cur = initializeANN(publicKey, nonce);
        memcpy(&bestANN, &currentANN, sizeof(bestANN));
        unsigned int best = cur;

        for (unsigned long long s = 0; s < numberOfMutations; ++s)
        {
            memcpy(&prevANN, &currentANN, sizeof(prevANN));

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
                memcpy(&currentANN, &prevANN, sizeof(currentANN));
            }

            if (cur < best)
            {
                best = cur;
                memcpy(&bestANN, &currentANN, sizeof(bestANN));
            }
        }
        return best;
    }
};

}
