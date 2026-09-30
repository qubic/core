#pragma once

#include "../src/mining/score_common.h"
#include "../src/mining/rating.h"
#include "../src/mining/task_file.h"
#include "score_common_reference.h"
#include "kangaroo_twelve.h"

#include <vector>
#include <cstring>

// Independent reference for the bpp9000 autonomous scorer (trits {0,1,2}, 2 = UNKNOWN, no inputs).
namespace score_bpp9000_reference
{

static constexpr unsigned char BPP9000_MODE_START = 1;
static constexpr unsigned char BPP9000_MODE_WIRING = 2;
static constexpr unsigned char BPP9000_MODE_LUT = 3;

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
    static constexpr unsigned long long shiftCap = Params::shiftCap;

    static constexpr unsigned long long maxNumberOfNeurons = populationThreshold;
    static constexpr unsigned long long numberOfWindows = sequenceLength - windowWidth;

    static constexpr unsigned char TRIT_UNKNOWN = 2;
    static constexpr unsigned int INFINITE_ERROR = 0xFFFFFFFFU;
    static constexpr unsigned int INVALID_SCORE_VALUE = 0xFFFFFFFFU;
    static constexpr unsigned long long lutSize = 27;
    static constexpr unsigned int MAX_CHANGES_PER_STEP = 10;
    static constexpr unsigned long long numberOfLinks = populationThreshold * numberOfNeighbors;
    // shift advances when the frame error drops to <= 1/3 of the frame.
    static constexpr unsigned int advanceThreshold = (unsigned int)(windowWidth / 3);

    static_assert(numberOfNeighbors == 3, "the LUT index is hardcoded for 3 neighbors");
    static_assert(populationThreshold % 16 == 0, "populationThreshold must be a multiple of 16 so sizeof(RootMaterial) stays a multiple of 64 for the random2 draw");
    static_assert(numberOfOutputNeurons == 1, "score() grades only output neuron 0");
    static_assert(numberOfWindows >= 1 && numberOfWindows < sequenceLength, "the emit count must be positive and within the target sequence");
    static_assert(maxNumberOfTicks > shiftCap + windowWidth, "maxNumberOfTicks must exceed the deepest emit count so all emits can fit");
    static_assert(shiftCap >= 1 && shiftCap <= numberOfWindows, "shiftCap must keep the last frame inside the data");
    static_assert(populationThreshold <= 65536, "the transfer index is 16-bit");

    // Root material drawn from the pubkey over the epoch pool: trit bytes plus one unsigned long long per link.
    struct RootMaterial
    {
        unsigned char lut[maxNumberOfNeurons * lutSize];
        unsigned char start[maxNumberOfNeurons];
        unsigned long long wire[numberOfLinks];
    };
    static_assert(sizeof(RootMaterial)
            == maxNumberOfNeurons * lutSize + maxNumberOfNeurons + numberOfLinks * sizeof(unsigned long long),
        "RootMaterial must be padding-free");
    static_assert(sizeof(RootMaterial) % 64 == 0, "root-material draw must be 64-byte aligned for random2");

    static constexpr unsigned long long mutationSeedCount = numberOfMutations * MAX_CHANGES_PER_STEP;
    static constexpr unsigned long long mutationSeedPaddedCount =
        ((mutationSeedCount * sizeof(unsigned long long) + 63) / 64) * 64 / sizeof(unsigned long long);

    std::vector<unsigned char> poolVec;

    // Global control (self-clock) and graded output neuron: from the digest, shared by every identity.
    unsigned int controlIndex;
    unsigned int outputIndex;
    unsigned char targetOutputs[sequenceLength];

    // Working state the walk mutates and scores.
    unsigned char curInitial[maxNumberOfNeurons];
    unsigned int neighborIndices[numberOfLinks];
    unsigned char curLut[maxNumberOfNeurons * lutSize];

    unsigned char prevInitial[maxNumberOfNeurons];
    unsigned int prevNeighborIndices[numberOfLinks];
    unsigned char prevLut[maxNumberOfNeurons * lutSize];
    unsigned char bestInitial[maxNumberOfNeurons];
    unsigned int bestNeighborIndices[numberOfLinks];
    unsigned char bestLut[maxNumberOfNeurons * lutSize];

    unsigned char neuronOut[maxNumberOfNeurons];
    unsigned char neuronPrev[maxNumberOfNeurons];

    // Rolling-frame position; every score() grades [shift, shift + windowWidth).
    unsigned long long shift = 0;

    RootMaterial rootMaterial;
    unsigned long long mutationSeed[mutationSeedPaddedCount];

    // Build the pool and derive the epoch's control/output from the mining seed (once, before scoring).
    void initialize(const unsigned char miningSeed[32])
    {
        poolVec.resize(score_reference::POOL_VEC_PADDING_SIZE);
        score_reference::generateRandom2Pool(miningSeed, poolVec.data());
        deriveControlOutput(miningSeed);
    }

    // The network is derived, so only the target output column of each data row is read here.
    bool loadTaskFromMemory(const unsigned char* topoBlock, const unsigned char* dataBlock)
    {
        (void)topoBlock;

        const unsigned long long inBytes = score_task_file::packedBytes(numberOfInputNeurons);
        const unsigned long long outBytes = score_task_file::packedBytes(numberOfOutputNeurons);
        const unsigned long long rowBytes = inBytes + outBytes;
        for (unsigned long long t = 0; t < sequenceLength; ++t)
        {
            unsigned char outTrit[numberOfOutputNeurons];
            if (!score_task_file::unpackTrits(dataBlock + t * rowBytes + inBytes, numberOfOutputNeurons, outTrit))
            {
                return false;
            }
            targetOutputs[t] = outTrit[0];
        }
        return true;
    }

    void deriveControlOutput(const unsigned char* digest)
    {
        unsigned char seedHash[32];
        KangarooTwelve(digest, 32, seedHash, 32);
        unsigned long long material[8];
        score_reference::random2(seedHash, poolVec.data(), (unsigned char*)material, sizeof(material));
        controlIndex = (unsigned int)(material[0] % populationThreshold);
        unsigned int output = (unsigned int)(material[1] % populationThreshold);
        if (output == controlIndex)
        {
            output = (unsigned int)((output + 1) % populationThreshold);
        }
        outputIndex = output;
    }

    // Autonomous rollout from the start state; the control neuron gates a graded emit, timeout at maxNumberOfTicks.
    unsigned int score()
    {
        for (unsigned long long n = 0; n < populationThreshold; ++n)
        {
            neuronOut[n] = curInitial[n];
        }

        unsigned int failures = 0;
        unsigned long long counter = 0;
        unsigned long long ticks = 0;
        while (counter < shift + windowWidth)
        {
            if (++ticks >= maxNumberOfTicks)
            {
                return INFINITE_ERROR;
            }

            memcpy(neuronPrev, neuronOut, sizeof(neuronPrev));
            for (unsigned long long n = 0; n < populationThreshold; ++n)
            {
                const unsigned long long t0 = neuronPrev[neighborIndices[n * numberOfNeighbors + 0]];
                const unsigned long long t1 = neuronPrev[neighborIndices[n * numberOfNeighbors + 1]];
                const unsigned long long t2 = neuronPrev[neighborIndices[n * numberOfNeighbors + 2]];
                neuronOut[n] = curLut[n * lutSize + (t0 + 3 * t1 + 9 * t2)];
            }

            if (neuronOut[controlIndex] != TRIT_UNKNOWN)
            {
                if (counter >= shift && neuronOut[outputIndex] != targetOutputs[counter])
                {
                    failures++;
                }
                counter++;
            }
        }
        return failures;
    }

    void mutateStartState(unsigned long long seed)
    {
        const unsigned long long delta = seed & 1ULL;
        const unsigned long long n = (seed >> 1) % populationThreshold;
        curInitial[n] = (unsigned char)((curInitial[n] + 1 + delta) % 3);
    }

    void mutateWiring(unsigned long long seed)
    {
        const unsigned long long flatSlot = seed % numberOfLinks;
        unsigned int target = (unsigned int)((seed / numberOfLinks) % populationThreshold);
        const unsigned int previous = neighborIndices[flatSlot];
        while (target == previous)
        {
            target = (unsigned int)((target + 1) % populationThreshold);
        }
        neighborIndices[flatSlot] = target;
    }

    void mutateLut(unsigned long long seed)
    {
        const unsigned long long delta = seed & 1ULL;
        const unsigned long long flatIdx = (seed >> 1) % (maxNumberOfNeurons * lutSize);
        curLut[flatIdx] = (unsigned char)((curLut[flatIdx] + 1 + delta) % 3);
    }

    void mutate(unsigned char mode, unsigned long long seed)
    {
        if (mode == BPP9000_MODE_START)
        {
            mutateStartState(seed);
        }
        else if (mode == BPP9000_MODE_WIRING)
        {
            mutateWiring(seed);
        }
        else if (mode == BPP9000_MODE_LUT)
        {
            mutateLut(seed);
        }
    }

    void deriveRootMaterial(const unsigned char* seed)
    {
        unsigned char rootHash[32];
        KangarooTwelve(seed, 32, rootHash, 32);
        score_reference::random2(rootHash, poolVec.data(), (unsigned char*)&rootMaterial, sizeof(rootMaterial));
    }

    void deriveMutationSeeds(const unsigned char* publicKey, const unsigned char* nonce, const unsigned char* anchorTickDigest)
    {
        unsigned char searchHash[32];
        unsigned char combined[96];
        memcpy(combined, publicKey, 32);
        memcpy(combined + 32, nonce, 32);
        combined[32] = 0;
        combined[33] = 0;
        combined[34] = 0;
        unsigned int combinedSize = 64;
        if (anchorTickDigest != nullptr)
        {
            memcpy(combined + 64, anchorTickDigest, 32);
            combinedSize = 96;
        }
        KangarooTwelve(combined, combinedSize, searchHash, 32);
        score_reference::random2(searchHash, poolVec.data(), (unsigned char*)&mutationSeed, sizeof(mutationSeed));
    }

    void applyRootMaterial()
    {
        for (unsigned long long i = 0; i < maxNumberOfNeurons * lutSize; ++i)
        {
            curLut[i] = (unsigned char)(rootMaterial.lut[i] % 3);
        }
        for (unsigned long long n = 0; n < populationThreshold; ++n)
        {
            curInitial[n] = (unsigned char)(rootMaterial.start[n] % 3);
        }
        for (unsigned long long i = 0; i < numberOfLinks; ++i)
        {
            neighborIndices[i] = (unsigned int)(rootMaterial.wire[i] % populationThreshold);
        }
    }

    void snapshotPrev()
    {
        memcpy(prevInitial, curInitial, sizeof(prevInitial));
        memcpy(prevNeighborIndices, neighborIndices, sizeof(prevNeighborIndices));
        memcpy(prevLut, curLut, sizeof(prevLut));
    }

    void rollbackPrev()
    {
        memcpy(curInitial, prevInitial, sizeof(curInitial));
        memcpy(neighborIndices, prevNeighborIndices, sizeof(neighborIndices));
        memcpy(curLut, prevLut, sizeof(curLut));
    }

    void snapshotBest()
    {
        memcpy(bestInitial, curInitial, sizeof(bestInitial));
        memcpy(bestNeighborIndices, neighborIndices, sizeof(bestNeighborIndices));
        memcpy(bestLut, curLut, sizeof(bestLut));
    }

    static unsigned int changesPerStep(const unsigned char* nonce)
    {
        return nonce[1] & 0x0F;
    }

    static unsigned char modeOf(const unsigned char* nonce)
    {
        return (unsigned char)((nonce[1] >> 4) & 0x03);
    }

    // Slides the frame while this fixed network holds it; stops when it cannot, or at shiftCap.
    score_engine::Rating advanceShift()
    {
        for (;;)
        {
            const unsigned int frameError = score();
            if (frameError > advanceThreshold)
            {
                return score_engine::Rating{ frameError, (unsigned int)shift };
            }
            if (shift == shiftCap)
            {
                return score_engine::Rating{ frameError, (unsigned int)shift };
            }
            shift++;
        }
    }

    // Anti-attractor walk: L mutations/step, explore for K steps then exploit, one-step rollback of the
    // network and the shift. Explore compares error and records nothing; exploit compares the rating.
    // Returns the committed rating, leaving that network in best*.
    score_engine::Rating computeScoreFromCurrent(unsigned int L, unsigned long long K, unsigned char mode)
    {
        score_engine::Rating cur = advanceShift();
        score_engine::Rating best = cur;
        snapshotBest();

        for (unsigned long long s = 0; s < numberOfMutations; ++s)
        {
            snapshotPrev();
            const unsigned long long prevShift = shift;

            for (unsigned int i = 0; i < L; ++i)
            {
                mutate(mode, mutationSeed[s * MAX_CHANGES_PER_STEP + i]);
            }

            const score_engine::Rating r = advanceShift();

            // A timed-out rollout is never accepted, in either phase.
            if (s < K)
            {
                // Anti-attractor: takes only a worse-or-equal error, and records nothing.
                if (r.isValid() && r.errorWorseOrEqual(cur))
                {
                    cur = r;
                }
                else
                {
                    rollbackPrev();
                    shift = prevShift;
                }
            }
            else
            {
                // Takes anything no worse than where the walk stands.
                if (r.isValid() && r.isNotWorseThan(cur))
                {
                    cur = r;
                }
                else
                {
                    rollbackPrev();
                    shift = prevShift;
                }

                // Records on the same keep-if-not-worse test as the accept above.
                if (cur.isValid() && cur.isNotWorseThan(best))
                {
                    best = cur;
                    snapshotBest();
                }
            }
        }

        shift = best.shift;
        return best;
    }

    // Standalone: root from the pubkey, walk with K = 0 (no explore). control/output already set by initialize.
    score_engine::Rating computeScore(const unsigned char* publicKey, const unsigned char* nonce)
    {
        const unsigned int L = changesPerStep(nonce);
        const unsigned char mode = modeOf(nonce);

        deriveRootMaterial(publicKey);
        deriveMutationSeeds(publicKey, nonce, nullptr);
        applyRootMaterial();

        shift = 0;   // standalone has no parent, so the frame starts at the first window
        return computeScoreFromCurrent(L, 0, mode);
    }
};

}
