#pragma once

#include "score_common.h"
#include "task_file.h"
#include "trit_pack.h"

// bpp9000 scorer: autonomous recurrent ternary network (trits {0,1,2}, 2=UNKNOWN)
namespace score_engine
{

// Largest L (changes per step) the scorer clamps the nonce to.
static constexpr unsigned int BPP9000_MAX_CHANGES_PER_STEP = 10;

// Mutation modes, declared by the miner in nonce[1].
static constexpr unsigned char BPP9000_MODE_START = 1;    // mutate the start state
static constexpr unsigned char BPP9000_MODE_WIRING = 2;   // mutate the wiring
static constexpr unsigned char BPP9000_MODE_LUT = 3;      // mutate the LUTs

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
    // Number of graded emits the network must produce.
    static constexpr unsigned long long numberOfWindows = sequenceLength - windowWidth;

    static constexpr unsigned char TRIT_UNKNOWN = 2;
    static constexpr unsigned int INFINITE_ERROR = 0xFFFFFFFFU;
    static constexpr unsigned int INVALID_SCORE_VALUE = 0xFFFFFFFFU;
    static constexpr unsigned long long lutSize = 27;   // 3^numberOfNeighbors, base-3 index t0 + 3*t1 + 9*t2

    // Directed links: one index per neuron neighbor slot.
    static constexpr unsigned long long numberOfLinks = populationThreshold * numberOfNeighbors;

    static_assert(numberOfNeighbors == 3, "the LUT index is hardcoded for 3 neighbors");
    static_assert((populationThreshold & (populationThreshold - 1)) == 0, "populationThreshold must be a power of 2");
    static_assert(numberOfOutputNeurons == 1, "score() grades only output neuron 0");
    static_assert(numberOfWindows >= 1 && numberOfWindows < sequenceLength, "the emit count must be positive and within the target sequence");
    static_assert(maxNumberOfTicks > numberOfWindows, "maxNumberOfTicks must exceed the emit count so all emits can fit");
    static_assert(populationThreshold <= 65536, "ANN.neighbor is a 16-bit transfer index");

    // nonce[1] layout: bits 0-3 = L in [1, BPP9000_MAX_CHANGES_PER_STEP], bits 4-5 = mode in [1, 3], bits 6-7 = 0.
    static unsigned int changesPerStep(const unsigned char* nonce)
    {
        return nonce[1] & 0x0F;
    }
    static unsigned char modeOf(const unsigned char* nonce)
    {
        return (unsigned char)((nonce[1] >> 4) & 0x03);
    }

    // A nonce is canonical if L, mode and the reserved bits are in range; nonce[0] = algo, nonce[2] = K.
    static bool isCanonicalNonceCommon(const unsigned char* nonce)
    {
        const unsigned int L = nonce[1] & 0x0F;
        const unsigned char mode = (unsigned char)((nonce[1] >> 4) & 0x03);
        return (getAlgoType(nonce) == AlgoType::Bpp9000)
            && (L >= 1)
            && (L <= BPP9000_MAX_CHANGES_PER_STEP)
            && (mode >= BPP9000_MODE_START)
            && (mode <= BPP9000_MODE_LUT)
            && ((nonce[1] & 0xC0) == 0);
    }

    // The standalone walk pins K to 0, so only 0 is canonical there.
    static bool isCanonicalStandaloneNonce(const unsigned char* nonce)
    {
        return isCanonicalNonceCommon(nonce) && (nonce[2] == 0);
    }

    // The ant walk uses K = nonce[2] explore steps, so K in [0, numberOfMutations] is canonical.
    static bool isCanonicalAntNonce(const unsigned char* nonce)
    {
        return isCanonicalNonceCommon(nonce) && (nonce[2] <= numberOfMutations);
    }

    // Per-identity root material from the pubkey seed: trit bytes, and one unsigned long long per wiring link
    // (the width random2 writes, reduced % population).
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
    static constexpr unsigned long long mutationSeedCount = numberOfMutations * BPP9000_MAX_CHANGES_PER_STEP;
    // Rounded up to a whole 64-byte block for random2, the walk reads only the first mutationSeedCount.
    static constexpr unsigned long long mutationSeedPaddedCount =
        ((mutationSeedCount * sizeof(unsigned long long) + 63) / 64) * 64 / sizeof(unsigned long long);

    // The exchanged form: what a child inherits from its parent, what is sent in node-miner messages
    struct ANN
    {
        unsigned short neighbor[numberOfLinks];
        unsigned char initialNeuronValues[maxNumberOfNeurons];
        unsigned char lut[maxNumberOfNeurons * lutSize];
    };
    static_assert(sizeof(ANN) == numberOfLinks * sizeof(unsigned short) + maxNumberOfNeurons + maxNumberOfNeurons * lutSize,
        "ANN must be padding-free");

    // Compact in-store form: wiring + start state copied through, the LUTs packed 2 bits per trit.
    struct StoredAnn
    {
        unsigned short neighbor[numberOfLinks];
        unsigned char initialNeuronValues[maxNumberOfNeurons];
        PackedTrits<maxNumberOfNeurons, lutSize> lut;
    };
    static_assert(sizeof(StoredAnn)
            == numberOfLinks * sizeof(unsigned short) + maxNumberOfNeurons + sizeof(PackedTrits<maxNumberOfNeurons, lutSize>),
        "StoredAnn must be padding-free");

    static void store(const ANN& a, StoredAnn& out)
    {
        copyMem(out.initialNeuronValues, a.initialNeuronValues, sizeof(out.initialNeuronValues));
        for (unsigned long long i = 0; i < numberOfLinks; ++i)
        {
            out.neighbor[i] = a.neighbor[i];
        }
        out.lut.pack(a.lut);
    }

    static void load(const StoredAnn& s, ANN& out)
    {
        copyMem(out.initialNeuronValues, s.initialNeuronValues, sizeof(out.initialNeuronValues));
        for (unsigned long long i = 0; i < numberOfLinks; ++i)
        {
            out.neighbor[i] = s.neighbor[i];
        }
        s.lut.unpack(out.lut);
    }

    // Self-clock and graded neuron: global for the epoch (deriveControlOutput), never mutated.
    unsigned int controlIndex;
    unsigned int outputIndex;
    unsigned char targetOutputs[sequenceLength];

    // Working state the walk mutates and scores.
    unsigned char curInitial[maxNumberOfNeurons];
    unsigned int neighborIndices[numberOfLinks];
    unsigned char curLut[maxNumberOfNeurons * lutSize];

    // rollback (prev) and best snapshots (only the active mode's component actually changes).
    unsigned char prevInitial[maxNumberOfNeurons];
    unsigned int prevNeighborIndices[numberOfLinks];
    unsigned char prevLut[maxNumberOfNeurons * lutSize];
    unsigned char bestInitial[maxNumberOfNeurons];
    unsigned int bestNeighborIndices[numberOfLinks];
    unsigned char bestLut[maxNumberOfNeurons * lutSize];

    unsigned char neuronOut[maxNumberOfNeurons];
    unsigned char neuronPrev[maxNumberOfNeurons];

    // Per-identity root material (from the pubkey seed).
    RootMaterial rootMaterial;
    // The walk's mutation seeds (from pubkey + nonce + anchor).
    unsigned long long mutationSeed[mutationSeedPaddedCount];

    void initMemory()
    {
    }

    // In-memory task load: the network is derived, so only the target output sequence is read from the
    // data block. The topology block is unused, and the input column of each data row is skipped.
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

    // The control and output neurons: global for the epoch, drawn from the digest alone so every identity
    // shares them. Must be called once (with the epoch digest and pool) before any score.
    void deriveControlOutput(const unsigned char* seed, const unsigned char* pRandom2Pool)
    {
        unsigned char seedHash[32];
        KangarooTwelve(seed, 32, seedHash, 32);
        unsigned long long material[8];   // 64-byte minimum random2 draw; only [0] and [1] are used
        random2(seedHash, pRandom2Pool, (unsigned char*)material, sizeof(material));
        controlIndex = (unsigned int)(material[0] % populationThreshold);
        unsigned int output = (unsigned int)(material[1] % populationThreshold);
        if (output == controlIndex)
        {
            output = (unsigned int)((output + 1) % populationThreshold);
        }
        outputIndex = output;
    }

    bool validateTopology()
    {
        if (controlIndex >= populationThreshold || outputIndex >= populationThreshold || controlIndex == outputIndex)
        {
            return false;
        }
        for (unsigned long long i = 0; i < numberOfLinks; ++i)
        {
            if (neighborIndices[i] >= populationThreshold)
            {
                return false;
            }
        }
        return true;
    }

    // Autonomous rollout: run from the start state; the control neuron gates a graded emit, timing out at maxNumberOfTicks.
    unsigned int score()
    {
        for (unsigned long long n = 0; n < populationThreshold; ++n)
        {
            neuronOut[n] = curInitial[n];
        }

        unsigned int failures = 0;
        unsigned long long counter = 0;
        unsigned long long ticks = 0;
        while (counter < numberOfWindows)
        {
            if (++ticks >= maxNumberOfTicks)
            {
                return INFINITE_ERROR;
            }

            copyMem(neuronPrev, neuronOut, sizeof(neuronPrev));
            for (unsigned long long n = 0; n < populationThreshold; ++n)
            {
                const unsigned long long t0 = neuronPrev[neighborIndices[n * numberOfNeighbors + 0]];
                const unsigned long long t1 = neuronPrev[neighborIndices[n * numberOfNeighbors + 1]];
                const unsigned long long t2 = neuronPrev[neighborIndices[n * numberOfNeighbors + 2]];
                neuronOut[n] = curLut[n * lutSize + (t0 + 3 * t1 + 9 * t2)];
            }

            if (neuronOut[controlIndex] != TRIT_UNKNOWN)
            {
                if (neuronOut[outputIndex] != targetOutputs[counter])
                {
                    failures++;
                }
                counter++;
            }
        }
        return failures;
    }

    // Flip one neuron's start-state trit (bit 0 picks +1 vs +2, the rest picks the neuron).
    void mutateStartState(unsigned long long mutationSeed)
    {
        const unsigned long long delta = mutationSeed & 1ULL;
        const unsigned long long n = (mutationSeed >> 1) % populationThreshold;
        curInitial[n] = (unsigned char)((curInitial[n] + 1 + delta) % 3);
    }

    // Rewire one link to a different target neuron (self-loops and duplicate neighbors are allowed).
    void mutateWiring(unsigned long long mutationSeed)
    {
        const unsigned long long flatSlot = mutationSeed % numberOfLinks;
        unsigned int target = (unsigned int)((mutationSeed / numberOfLinks) % populationThreshold);
        const unsigned int previous = neighborIndices[flatSlot];
        while (target == previous)
        {
            target = (unsigned int)((target + 1) % populationThreshold);
        }
        neighborIndices[flatSlot] = target;
    }

    // Flip one LUT entry of one neuron (bit 0 picks +1 vs +2, the rest picks the entry).
    void mutateLut(unsigned long long mutationSeed)
    {
        const unsigned long long delta = mutationSeed & 1ULL;
        const unsigned long long flatIdx = (mutationSeed >> 1) % (maxNumberOfNeurons * lutSize);
        curLut[flatIdx] = (unsigned char)((curLut[flatIdx] + 1 + delta) % 3);
    }

    // Apply one mutation of the declared mode. Mode is 1, 2 or 3 only; a non-canonical nonce (any other
    // mode) is rejected before scoring, so no other value reaches here and there is no fallback.
    void mutate(unsigned char mode, unsigned long long mutationSeed)
    {
        if (mode == BPP9000_MODE_START)
        {
            mutateStartState(mutationSeed);
        }
        else if (mode == BPP9000_MODE_WIRING)
        {
            mutateWiring(mutationSeed);
        }
        else if (mode == BPP9000_MODE_LUT)
        {
            mutateLut(mutationSeed);
        }
    }

    // Fill the root material (LUTs, start state, wiring) from the pubkey seed in one draw.
    void deriveRootMaterial(const unsigned char* seed, const unsigned char* pRandom2Pool)
    {
        unsigned char rootHash[32];
        KangarooTwelve(seed, 32, rootHash, 32);
        random2(rootHash, pRandom2Pool, (unsigned char*)&rootMaterial, sizeof(rootMaterial));
    }

    // Mutation-walk seeds. nonce[0..2] (algo/L/mode/K) are zeroed out of the hash so they do not reseed the
    // walk; anchorTickDigest is null for standalone, the anchor tick for an ant child.
    void deriveMutationSeeds(
        const unsigned char* publicKey,
        const unsigned char* nonce,
        const unsigned char* anchorTickDigest,
        const unsigned char* pRandom2Pool)
    {
        unsigned char searchHash[32];
        unsigned char combined[96];
        copyMem(combined, publicKey, 32);
        copyMem(combined + 32, nonce, 32);
        combined[32] = 0;
        combined[33] = 0;
        combined[34] = 0;
        unsigned int combinedSize = 64;
        if (anchorTickDigest != nullptr)
        {
            copyMem(combined + 64, anchorTickDigest, 32);
            combinedSize = 96;
        }
        KangarooTwelve(combined, combinedSize, searchHash, 32);
        random2(searchHash, pRandom2Pool, (unsigned char*)&mutationSeed, sizeof(mutationSeed));
    }

    // Load the root material into the working state (LUTs, start state, wiring), absolute by neuron index.
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

    // Working state -> ANN.
    void compact(ANN& out) const
    {
        copyMem(out.initialNeuronValues, curInitial, sizeof(out.initialNeuronValues));
        for (unsigned long long i = 0; i < numberOfLinks; ++i)
        {
            out.neighbor[i] = (unsigned short)neighborIndices[i];
        }
        copyMem(out.lut, curLut, sizeof(out.lut));
    }

    // ANN -> working state.
    void expand(const ANN& src)
    {
        copyMem(curInitial, src.initialNeuronValues, sizeof(curInitial));
        for (unsigned long long i = 0; i < numberOfLinks; ++i)
        {
            neighborIndices[i] = src.neighbor[i];
        }
        copyMem(curLut, src.lut, sizeof(curLut));
    }

    // The network behind the score the last walk returned.
    void getBestANN(ANN& out) const
    {
        copyMem(out.initialNeuronValues, bestInitial, sizeof(out.initialNeuronValues));
        for (unsigned long long i = 0; i < numberOfLinks; ++i)
        {
            out.neighbor[i] = (unsigned short)bestNeighborIndices[i];
        }
        copyMem(out.lut, bestLut, sizeof(out.lut));
    }

    // Anti-attractor walk: L mutations/step of the mode; explore (accept worse-or-equal) for K steps, then
    // exploit (better-or-equal); one-step rollback. Returns the best score, leaving that network in best*.
    unsigned int computeScoreFromCurrent(unsigned int L, unsigned long long K, unsigned char mode, unsigned int startScore)
    {
        unsigned int cur = startScore;
        unsigned int best = cur;
        snapshotBest();

        for (unsigned long long s = 0; s < numberOfMutations; ++s)
        {
            snapshotPrev();

            for (unsigned int i = 0; i < L; ++i)
            {
                mutate(mode, mutationSeed[s * BPP9000_MAX_CHANGES_PER_STEP + i]);
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
                rollbackPrev();
            }

            if (cur < best)
            {
                best = cur;
                snapshotBest();
            }
        }
        return best;
    }

    void snapshotPrev()
    {
        copyMem(prevInitial, curInitial, sizeof(prevInitial));
        copyMem(prevNeighborIndices, neighborIndices, sizeof(prevNeighborIndices));
        copyMem(prevLut, curLut, sizeof(prevLut));
    }

    void rollbackPrev()
    {
        copyMem(curInitial, prevInitial, sizeof(curInitial));
        copyMem(neighborIndices, prevNeighborIndices, sizeof(neighborIndices));
        copyMem(curLut, prevLut, sizeof(curLut));
    }

    void snapshotBest()
    {
        copyMem(bestInitial, curInitial, sizeof(bestInitial));
        copyMem(bestNeighborIndices, neighborIndices, sizeof(bestNeighborIndices));
        copyMem(bestLut, curLut, sizeof(bestLut));
    }

    // Standalone: root from the pubkey, walk with K = 0 (no explore). control/output must already be set
    // by deriveControlOutput.
    unsigned int computeScore(
        const unsigned char* publicKey,
        const unsigned char* nonce,
        const unsigned char* pRandom2Pool)
    {
        const unsigned int L = changesPerStep(nonce);
        const unsigned char mode = modeOf(nonce);

        deriveRootMaterial(publicKey, pRandom2Pool);
        deriveMutationSeeds(publicKey, nonce, nullptr, pRandom2Pool);
        applyRootMaterial();

        const unsigned int cur = score();
        return computeScoreFromCurrent(L, 0, mode, cur);
    }

    // Ant colony: the identity's root - LUTs, start state and wiring derived from rootSeed (the identity
    // pubkey), so each identity's tree starts from its own root.
    void deriveRootANN(const unsigned char* rootSeed, const unsigned char* pRandom2Pool, ANN& out)
    {
        deriveRootMaterial(rootSeed, pRandom2Pool);
        applyRootMaterial();
        compact(out);
    }

    // Ant colony: score a child by inheriting the parent's network, walking with the child's seeds.
    // control/output must already be set by deriveControlOutput.
    unsigned int computeScoreFromParent(
        const ANN& parentANN,
        const unsigned char* publicKey,
        const unsigned char* nonce,
        const unsigned char* anchorTickDigest,
        const unsigned char* pRandom2Pool)
    {
        if (!isCanonicalAntNonce(nonce))
        {
            return INVALID_SCORE_VALUE;
        }

        expand(parentANN);
        if (!validateTopology())
        {
            return INVALID_SCORE_VALUE;
        }
        deriveMutationSeeds(publicKey, nonce, anchorTickDigest, pRandom2Pool);

        const unsigned int L = changesPerStep(nonce);
        const unsigned long long K = nonce[2];
        const unsigned char mode = modeOf(nonce);

        const unsigned int cur = score();
        return computeScoreFromCurrent(L, K, mode, cur);
    }

    int getLastOutput(unsigned char* requestedOutput, int requestedSizeInBytes)
    {
        return 0;
    }
};

}
