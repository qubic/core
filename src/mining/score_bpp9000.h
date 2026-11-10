#pragma once

// bpp9000 scorer: every build returns the same Rating and best ANN bytes as test/score_bpp9000_reference.h

#include "score_common.h"
#include "rating.h"
#include "task_file.h"
#include "trit_pack.h"

namespace score_engine
{

static constexpr unsigned int BPP9000_MAX_CHANGES_PER_STEP = 10;

static constexpr unsigned char BPP9000_MODE_START = 1;
static constexpr unsigned char BPP9000_MODE_WIRING = 2;
static constexpr unsigned char BPP9000_MODE_LUT = 3;

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
    static constexpr unsigned long long shiftCap = Params::shiftCap;

    static constexpr unsigned long long maxNumberOfNeurons = populationThreshold;
    static constexpr unsigned long long numberOfWindows = sequenceLength - windowWidth;

    static constexpr unsigned char TRIT_UNKNOWN = 2;
    static constexpr unsigned int INFINITE_ERROR = 0xFFFFFFFFU;
    static constexpr unsigned int INVALID_SCORE_VALUE = 0xFFFFFFFFU;
    static constexpr unsigned long long lutSize = 27;

    // The shift advances while the frame error stays <= advanceThreshold.
    static constexpr unsigned int advanceThreshold = BPP9000_ADVANCE_THRESHOLD;

    static constexpr unsigned long long numberOfLinks = populationThreshold * numberOfNeighbors;

    static_assert(numberOfNeighbors == 3, "the LUT index is hardcoded for 3 neighbors");
    static_assert(populationThreshold % 16 == 0, "populationThreshold must be a multiple of 16 so sizeof(RootMaterial) stays a multiple of 64 for the random2 draw");
    static_assert(numberOfOutputNeurons == 1, "score() grades only output neuron 0");
    static_assert(numberOfWindows >= 1 && numberOfWindows < sequenceLength, "the frame must leave targets after it");
    static_assert(shiftCap >= 1 && shiftCap <= numberOfWindows, "shiftCap must be positive and keep the last frame inside the data");
    static_assert(maxNumberOfTicks > shiftCap + windowWidth, "maxNumberOfTicks must exceed the deepest emit count so all emits can fit");
    static_assert(advanceThreshold < windowWidth, "the advance gate must be reachable inside one frame");
    // The frame-0 floor sits between the advance gate and the frame width.
    static_assert(solutionThreshold > advanceThreshold && solutionThreshold < windowWidth, "the frame-0 floor must admit some root and still reject the worst");
    static_assert(populationThreshold <= 65536, "ANN.neighbor is a 16-bit transfer index");

    static unsigned int changesPerStep(const unsigned char* nonce)
    {
        return nonce[1] & 0x0F;
    }
    static unsigned char modeOf(const unsigned char* nonce)
    {
        return (unsigned char)((nonce[1] >> 4) & 0x03);
    }

    // Only the wiring mode is accepted: the start state and the LUTs stay as the identity's root derived them.
    static bool isCanonicalNonceCommon(const unsigned char* nonce)
    {
        const unsigned int L = nonce[1] & 0x0F;
        const unsigned char mode = (unsigned char)((nonce[1] >> 4) & 0x03);
        return (getAlgoType(nonce) == AlgoType::Bpp9000)
            && (L >= 1)
            && (L <= BPP9000_MAX_CHANGES_PER_STEP)
            && (mode == BPP9000_MODE_WIRING)
            && ((nonce[1] & 0xC0) == 0);
    }

    static bool isCanonicalStandaloneNonce(const unsigned char* nonce)
    {
        return isCanonicalNonceCommon(nonce) && (nonce[2] == 0);
    }

    static bool isCanonicalAntNonce(const unsigned char* nonce)
    {
        return isCanonicalNonceCommon(nonce) && (nonce[2] <= numberOfMutations);
    }

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
    static constexpr unsigned long long mutationSeedPaddedCount =
        ((mutationSeedCount * sizeof(unsigned long long) + 63) / 64) * 64 / sizeof(unsigned long long);

    struct ANN
    {
        unsigned short neighbor[numberOfLinks];
        unsigned char initialNeuronValues[maxNumberOfNeurons];
        unsigned char lut[maxNumberOfNeurons * lutSize];
    };
    static_assert(sizeof(ANN) == numberOfLinks * sizeof(unsigned short) + maxNumberOfNeurons + maxNumberOfNeurons * lutSize,
        "ANN must be padding-free");

    // Stored wiring indices use only the bits the population needs.
    static constexpr unsigned int neighborIndexBits = bitsToIndex(populationThreshold);

    using StoredNeighbors = PackedIndices<numberOfLinks, neighborIndexBits>;
    using StoredStartState = PackedBase3<maxNumberOfNeurons>;
    using StoredLut = PackedBase3<maxNumberOfNeurons * lutSize>;

    static constexpr unsigned long long storedAnnDataBytes =
        sizeof(StoredNeighbors) + sizeof(StoredStartState) + sizeof(StoredLut);
    // Pads StoredAnn to a multiple of 8 bytes (at least 1) so a struct holding it has no implicit gap.
    static constexpr unsigned long long storedAnnPadBytes = 8 - (storedAnnDataBytes % 8);

    // In-store form: byte arrays only, so alignment 1 and padding-free.
    struct StoredAnn
    {
        StoredNeighbors neighbor;
        StoredStartState initialNeuronValues;
        StoredLut lut;
        unsigned char padding[storedAnnPadBytes];
    };
    static_assert(sizeof(StoredAnn) == storedAnnDataBytes + storedAnnPadBytes,
        "StoredAnn must be padding-free");
    static_assert(sizeof(StoredAnn) % 8 == 0,
        "StoredAnn must be eight-byte sized so the structs holding it have no implicit gap");

    static void store(const ANN& a, StoredAnn& out)
    {
        out.neighbor.pack(a.neighbor);
        out.initialNeuronValues.pack(a.initialNeuronValues);
        out.lut.pack(a.lut);
        for (unsigned long long i = 0; i < storedAnnPadBytes; ++i)
        {
            out.padding[i] = 0;
        }
    }

    static void load(const StoredAnn& s, ANN& out)
    {
        s.neighbor.unpack(out.neighbor);
        s.initialNeuronValues.unpack(out.initialNeuronValues);
        s.lut.unpack(out.lut);
    }

    unsigned int controlIndex;
    unsigned int outputIndex;
    unsigned char targetOutputs[sequenceLength];

    unsigned char curInitial[maxNumberOfNeurons];
    unsigned int neighborIndices[numberOfLinks];
    unsigned char curLut[maxNumberOfNeurons * lutSize];

    unsigned char prevInitial[maxNumberOfNeurons];
    unsigned int prevNeighborIndices[numberOfLinks];
    unsigned char prevLut[maxNumberOfNeurons * lutSize];
    unsigned char bestInitial[maxNumberOfNeurons];
    unsigned int bestNeighborIndices[numberOfLinks];
    unsigned char bestLut[maxNumberOfNeurons * lutSize];

    unsigned long long shift = 0;

    unsigned char neuronOut[maxNumberOfNeurons];
    // 4 spare bytes: the AVX2 gather tick reads 4 bytes per lane.
    unsigned char neuronPrev[maxNumberOfNeurons + 4];

    RootMaterial rootMaterial;
    unsigned long long mutationSeed[mutationSeedPaddedCount];

    // Mode of the walk in progress; tells the AVX-512 advanceShift() which derived cache to refresh.
    unsigned char activeMode = 0;

    static constexpr unsigned long long speculationBatchCap = 80;

    void initMemory()
    {
#ifdef __AVX512F__
        buildHoistedConstants();
#endif
    }

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

    void deriveControlOutput(const unsigned char* seed, const unsigned char* pRandom2Pool)
    {
        unsigned char seedHash[32];
        KangarooTwelve(seed, 32, seedHash, 32);
        unsigned long long material[8];
        random2(seedHash, pRandom2Pool, (unsigned char*)material, sizeof(material));
        controlIndex = (unsigned int)(material[0] % populationThreshold);
        unsigned int output = (unsigned int)(material[1] % populationThreshold);
        if (output == controlIndex)
        {
            output = (unsigned int)((output + 1) % populationThreshold);
        }
        outputIndex = output;
    }

    // Rejects out-of-range wiring and non-trit start/LUT bytes: the tick indexes LUTs with them.
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
        for (unsigned long long n = 0; n < populationThreshold; ++n)
        {
            if (curInitial[n] > TRIT_UNKNOWN)
            {
                return false;
            }
        }
        for (unsigned long long i = 0; i < maxNumberOfNeurons * lutSize; ++i)
        {
            if (curLut[i] > TRIT_UNKNOWN)
            {
                return false;
            }
        }
        return true;
    }

    void mutateStartState(unsigned long long mutationSeed)
    {
        const unsigned long long delta = mutationSeed & 1ULL;
        const unsigned long long n = (mutationSeed >> 1) % populationThreshold;
        curInitial[n] = (unsigned char)((curInitial[n] + 1 + delta) % 3);
    }

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

    void mutateLut(unsigned long long mutationSeed)
    {
        const unsigned long long delta = mutationSeed & 1ULL;
        const unsigned long long flatIdx = (mutationSeed >> 1) % (maxNumberOfNeurons * lutSize);
        curLut[flatIdx] = (unsigned char)((curLut[flatIdx] + 1 + delta) % 3);
    }

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

    void deriveRootMaterial(const unsigned char* seed, const unsigned char* pRandom2Pool)
    {
        unsigned char rootHash[32];
        KangarooTwelve(seed, 32, rootHash, 32);
        random2(rootHash, pRandom2Pool, (unsigned char*)&rootMaterial, sizeof(rootMaterial));
    }

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

    void compact(ANN& out) const
    {
        copyMem(out.initialNeuronValues, curInitial, sizeof(out.initialNeuronValues));
        for (unsigned long long i = 0; i < numberOfLinks; ++i)
        {
            out.neighbor[i] = (unsigned short)neighborIndices[i];
        }
        copyMem(out.lut, curLut, sizeof(out.lut));
    }

    void expand(const ANN& src)
    {
        copyMem(curInitial, src.initialNeuronValues, sizeof(curInitial));
        for (unsigned long long i = 0; i < numberOfLinks; ++i)
        {
            neighborIndices[i] = src.neighbor[i];
        }
        copyMem(curLut, src.lut, sizeof(curLut));
    }

    void getBestANN(ANN& out) const
    {
        copyMem(out.initialNeuronValues, bestInitial, sizeof(out.initialNeuronValues));
        for (unsigned long long i = 0; i < numberOfLinks; ++i)
        {
            out.neighbor[i] = (unsigned short)bestNeighborIndices[i];
        }
        copyMem(out.lut, bestLut, sizeof(out.lut));
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

    // L mutations/step, explore for K steps then exploit, one-step rollback of the network and shift.
    // Returns the best rating, leaving that network in best*.
    Rating rollingStepsFrom(unsigned long long sStart, unsigned int L, unsigned long long K, unsigned char mode, Rating cur, Rating best)
    {
        for (unsigned long long s = sStart; s < numberOfMutations; ++s)
        {
            snapshotPrev();
            const unsigned long long prevShift = shift;

            for (unsigned int i = 0; i < L; ++i)
            {
                mutate(mode, mutationSeed[s * BPP9000_MAX_CHANGES_PER_STEP + i]);
            }

            const Rating r = advanceShift();

            // A timed-out rollout is never accepted.
            if (s < K)
            {
                // Explore (anti-attractor): worse-or-equal error only; records nothing.
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
                // Exploit: anything no worse than where the walk stands.
                if (r.isValid() && r.isNotWorseThan(cur))
                {
                    cur = r;
                }
                else
                {
                    rollbackPrev();
                    shift = prevShift;
                }

                // Records on the same test as the accept above.
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

#ifdef __AVX512F__

    // Neuron-parallel engine: one network, all neurons ticked together in 64-neuron blocks, one lane per neuron.
    static constexpr unsigned long long B = (populationThreshold + 63) / 64;
    static constexpr unsigned long long neuronsInBlock(unsigned long long b)
    {
        return (populationThreshold - b * 64 < 64) ? (populationThreshold - b * 64) : 64;
    }
    static constexpr unsigned long long PBYTES = populationThreshold / 8;
    static constexpr unsigned long long PWINDOWS = (PBYTES + 127) / 128;
    // Planes are padded to whole 128-byte gather windows; the padding is never selected.
    static constexpr unsigned long long PPADDED = PWINDOWS * 128;

    // Neuron-parallel state: bit n of plane0 / plane1 is bit 0 / bit 1 of neuron n's trit.
    unsigned char plane0[PPADDED];
    unsigned char plane1[PPADDED];
    unsigned long long planeOut0[PPADDED / 8];
    unsigned long long planeOut1[PPADDED / 8];

    // Gather controls per block and slot: byte index in its 128-byte window, window masks, bit select.
    unsigned char gIdx[B][numberOfNeighbors][64];
    unsigned long long gWin[B][numberOfNeighbors][PWINDOWS];
    unsigned char gCtl[B][numberOfNeighbors][64];

    // Table area: 64 neurons x 32-byte LUT slots per block, first 27 bytes used.
    unsigned char tbl[B][2048];

    // Constants built once in initMemory().
    unsigned long long tmask[16];
    unsigned char laneBase[64];
    unsigned char sh3[64];
    unsigned char sh9[64];
    unsigned char onesConst[64];

    // Wrong bits of the frame being graded; one network, so one ring and one running sum.
    unsigned char ring[windowWidth];

    void buildHoistedConstants()
    {
        for (unsigned int r = 0; r < 16; ++r)
        {
            tmask[r] = 0;
            for (unsigned int l = 0; l < 64; ++l)
            {
                if (l / 4 == r)
                {
                    tmask[r] |= 1ull << l;
                }
            }
        }
        for (unsigned int l = 0; l < 64; ++l)
        {
            laneBase[l] = (unsigned char)((l % 4) * 32);
        }
        for (unsigned int i = 0; i < 64; ++i)
        {
            sh3[i] = (unsigned char)(3 * (i % 16));
            sh9[i] = (unsigned char)(9 * (i % 16));
            onesConst[i] = 1;
        }
    }

    // Rebuilds the gather controls from neighborIndices.
    void buildGatherControls()
    {
        for (unsigned long long b = 0; b < B; ++b)
        {
            const unsigned long long neuronCount = neuronsInBlock(b);
            for (unsigned long long j = 0; j < numberOfNeighbors; ++j)
            {
                for (unsigned long long w = 0; w < PWINDOWS; ++w)
                {
                    gWin[b][j][w] = 0;
                }
                for (unsigned long long l = 0; l < neuronCount; ++l)
                {
                    const unsigned int s = neighborIndices[(b * 64 + l) * numberOfNeighbors + j];
                    const unsigned int byteIdx = s >> 3;
                    const unsigned int win = byteIdx / 128;
                    gIdx[b][j][l] = (unsigned char)(byteIdx & 127);
                    gWin[b][j][win] |= 1ull << l;
                    gCtl[b][j][l] = (unsigned char)((l & 7) * 8 + (s & 7));
                }
            }
        }
    }

    // Rebuilds the table area from curLut.
    void buildTableArea()
    {
        for (unsigned long long b = 0; b < B; ++b)
        {
            unsigned char* t = tbl[b];
            const unsigned long long neuronCount = neuronsInBlock(b);
            for (unsigned long long l = 0; l < neuronCount; ++l)
            {
                copyMem(t + l * 32, curLut + (b * 64 + l) * lutSize, lutSize);
            }
        }
    }

    void packPlanesFromCurInitial()
    {
        setMem(plane0, PPADDED, 0);
        setMem(plane1, PPADDED, 0);
        for (unsigned long long n = 0; n < populationThreshold; ++n)
        {
            if (curInitial[n] & 1)
            {
                plane0[n >> 3] |= (unsigned char)(1u << (n & 7));
            }
            if (curInitial[n] & 2)
            {
                plane1[n >> 3] |= (unsigned char)(1u << (n & 7));
            }
        }
    }

    unsigned int getTrit(unsigned int n) const
    {
        const unsigned int b0 = (plane0[n >> 3] >> (n & 7)) & 1u;
        const unsigned int b1 = (plane1[n >> 3] >> (n & 7)) & 1u;
        return b0 | (b1 << 1);
    }

    // One gathered trit vector for a block and slot; m1 is the all-ones byte constant.
    static inline __m512i gatherTrits(const unsigned char* pl0, const unsigned char* pl1,
        const unsigned char* idxBlockSlot, const unsigned long long* winBlockSlot, const unsigned char* ctlBlockSlot,
        __m512i m1)
    {
        const __m512i idx = _mm512_loadu_si512((const void*)idxBlockSlot);
        const __m512i ctl = _mm512_loadu_si512((const void*)ctlBlockSlot);
        __m512i g0;
        __m512i g1;
        {
            __m512i acc = idx;
            for (unsigned long long w = 0; w < PWINDOWS; ++w)
            {
                acc = _mm512_mask2_permutex2var_epi8(
                    _mm512_loadu_si512((const void*)(pl0 + w * 128)), acc,
                    (__mmask64)winBlockSlot[w],
                    _mm512_loadu_si512((const void*)(pl0 + w * 128 + 64)));
            }
            g0 = _mm512_and_si512(_mm512_multishift_epi64_epi8(ctl, acc), m1);
        }
        {
            __m512i acc = idx;
            for (unsigned long long w = 0; w < PWINDOWS; ++w)
            {
                acc = _mm512_mask2_permutex2var_epi8(
                    _mm512_loadu_si512((const void*)(pl1 + w * 128)), acc,
                    (__mmask64)winBlockSlot[w],
                    _mm512_loadu_si512((const void*)(pl1 + w * 128 + 64)));
            }
            g1 = _mm512_and_si512(_mm512_multishift_epi64_epi8(ctl, acc), m1);
        }
        return _mm512_or_si512(g0, _mm512_add_epi8(g1, g1));
    }

    // One tick: gather three neighbour trits, form the base-3 LUT index, look it up, split into two planes.
    void tickBitslice()
    {
        const __m512i m3 = _mm512_loadu_si512((const void*)sh3);
        const __m512i m9 = _mm512_loadu_si512((const void*)sh9);
        const __m512i lb = _mm512_loadu_si512((const void*)laneBase);
        const __m512i m1 = _mm512_loadu_si512((const void*)onesConst);
        for (unsigned long long b = 0; b < B; ++b)
        {
            const __m512i t0 = gatherTrits(plane0, plane1, gIdx[b][0], gWin[b][0], gCtl[b][0], m1);
            const __m512i t1 = gatherTrits(plane0, plane1, gIdx[b][1], gWin[b][1], gCtl[b][1], m1);
            const __m512i t2 = gatherTrits(plane0, plane1, gIdx[b][2], gWin[b][2], gCtl[b][2], m1);
            __m512i idx = _mm512_add_epi8(
                _mm512_add_epi8(t0, _mm512_shuffle_epi8(m3, t1)),
                _mm512_add_epi8(_mm512_shuffle_epi8(m9, t2), lb));
            for (unsigned int r = 0; r < 16; ++r)
            {
                idx = _mm512_mask2_permutex2var_epi8(
                    _mm512_loadu_si512((const void*)(tbl[b] + r * 128)), idx,
                    (__mmask64)tmask[r],
                    _mm512_loadu_si512((const void*)(tbl[b] + r * 128 + 64)));
            }
            planeOut0[b] = (unsigned long long)_mm512_test_epi8_mask(idx, m1);
            planeOut1[b] = (unsigned long long)_mm512_test_epi8_mask(idx, _mm512_add_epi8(m1, m1));
        }
        copyMem(plane0, planeOut0, PBYTES);
        copyMem(plane1, planeOut1, PBYTES);
    }

    // Grades the frame at the current shift, leaving shift alone.
    unsigned int score()
    {
        buildGatherControls();
        buildTableArea();
        packPlanesFromCurInitial();

        unsigned int failures = 0;
        unsigned long long counter = 0;
        unsigned long long ticks = 0;
        while (counter < shift + windowWidth)
        {
            ++ticks;
            if (ticks >= maxNumberOfTicks)
            {
                return INFINITE_ERROR;
            }

            tickBitslice();

            if (getTrit(controlIndex) != TRIT_UNKNOWN)
            {
                if (counter >= shift && getTrit(outputIndex) != targetOutputs[counter])
                {
                    failures++;
                }
                counter++;
            }
        }
        return failures;
    }

    // Scores from the current shift and slides the frame while it is mastered, in one replay from tick 0.
    Rating advanceShift()
    {
        if (activeMode == BPP9000_MODE_WIRING)
        {
            buildGatherControls();
        }
        else if (activeMode == BPP9000_MODE_LUT)
        {
            buildTableArea();
        }

        packPlanesFromCurInitial();

        const unsigned long long f0 = shift;
        unsigned long long emitCount = 0;
        unsigned int winSum = 0;
        unsigned long long ticks = 0;

        for (;;)
        {
            ++ticks;
            if (ticks >= maxNumberOfTicks)
            {
                return Rating{ INFINITE_ERROR, (unsigned int)shift };
            }

            tickBitslice();

            const unsigned int ctrlTrit = getTrit(controlIndex);
            if (ctrlTrit != TRIT_UNKNOWN)
            {
                const unsigned int outTrit = getTrit(outputIndex);
                const unsigned long long absI = emitCount++;
                if (absI >= f0)
                {
                    const unsigned long long relJ = absI - f0;
                    const unsigned int bit = (outTrit != targetOutputs[absI]) ? 1u : 0u;
                    const unsigned int slot = (unsigned int)(relJ % windowWidth);
                    if (relJ >= windowWidth)
                    {
                        winSum -= ring[slot];
                    }
                    ring[slot] = (unsigned char)bit;
                    winSum += bit;
                    if (relJ + 1 >= windowWidth)
                    {
                        const unsigned long long frame = f0 + (relJ + 1 - windowWidth);
                        if (!(winSum <= advanceThreshold && frame < shiftCap))
                        {
                            shift = frame;
                            return Rating{ winSum, (unsigned int)frame };
                        }
                        // The reference's shift++: a timeout reports the frame being filled.
                        shift = frame + 1;
                    }
                }
            }
        }
    }

    // Mutation-parallel engine: up to 64 walk steps per batch, one per lane, all on one base network. Each step is
    // assumed rejected, steps are resolved in order, and only the accepted one is replayed through mutate().
    static constexpr unsigned int SIMD_LANES = 64;
    static_assert(SIMD_LANES == 64, "a bit-plane row is one 64-bit word per plane");
    static_assert(populationThreshold % 4 == 0, "rows are stored four to a 64-byte group");
    static_assert(populationThreshold <= 65536, "bpRec packs internal row indices into 16-bit fields");

    static constexpr unsigned int SIMD_MAX_FIXUPS = SIMD_LANES * BPP9000_MAX_CHANGES_PER_STEP;
    static_assert(SIMD_MAX_FIXUPS <= 65536, "fix-up windows pack a pool index into 16 bits");

    // bpState[buffer][row] = { plane0, plane1 }, bit l = lane l's trit bit; rows in ordNeuron order.
    unsigned long long bpState[2][maxNumberOfNeurons][2];
    unsigned char bpTab[maxNumberOfNeurons][64];
    unsigned char bpTabInt[maxNumberOfNeurons][64];
    // Base sources of each row as row indices: s0 | s1 << 16 | s2 << 32. bpTab/bpTabInt hold the LUT by code.
    unsigned long long bpRec[maxNumberOfNeurons];
    unsigned int ordNeuron[maxNumberOfNeurons];
    unsigned int intIdx[maxNumberOfNeurons];
    unsigned char evalMark[maxNumberOfNeurons];
    unsigned int bpCleanRows;
    unsigned int bpEvalRows;
    unsigned int bpCleanGroups;
    unsigned int bpEvalGroups;
    bool bpStartOrderValid;

    // Fix-up shape of a row; rows are sorted by shape so each run takes straight-line code.
    enum : unsigned char
    {
        BP_W1R0 = 0,
        BP_W1R1 = 1,
        BP_W1R2 = 2,
        BP_WPAD = 3,
        BP_WGEN = 4,
        BP_L1 = 5,
        BP_L2 = 6,
        BP_LN = 7,
        BP_CLEAN = 8,
        BP_SHAPES = 9
    };
    unsigned char bpNeuronShape[maxNumberOfNeurons];
    unsigned char bpRowShape[maxNumberOfNeurons];
    unsigned int bpRowFix[maxNumberOfNeurons];
    struct BpWireFix
    {
        unsigned long long mask;
        unsigned int alt;
        unsigned int role;
    };
    struct BpLutFix
    {
        unsigned long long mask;   // byte positions, not lanes (lanesToPositions)
        unsigned char code;
        unsigned char value;
    };
    BpWireFix bpWireFix[2 * SIMD_MAX_FIXUPS];
    BpLutFix bpLutFix[SIMD_MAX_FIXUPS];

    static constexpr unsigned char BP_MIXED = 255;
    static constexpr unsigned int BP_MAX_RUNS = 64;
    unsigned char bpRunShape[BP_MAX_RUNS];
    unsigned int bpRunBegin[BP_MAX_RUNS];
    unsigned int bpRunEnd[BP_MAX_RUNS];
    unsigned int bpNumRuns;

    unsigned char bpPermIn[64];
    unsigned char bpPermOut[64];

    // Batch width: 64 lanes (bpState) or 32 lanes (bpState32).
    unsigned int bpWidth;
    unsigned int bpState32[2][maxNumberOfNeurons][2];
    unsigned char bpPermIn32[32];
    unsigned char bpPermOut32[32];

    // Backward reachability from control and output through the base wiring.
    unsigned char cone[maxNumberOfNeurons];
    unsigned int coneStack[maxNumberOfNeurons];
    unsigned int simdWire[maxNumberOfNeurons * numberOfNeighbors];

    // START mode only: reverse adjacency and a 16-tick base trace for the dead-flip test.
    unsigned int readerStart[maxNumberOfNeurons + 1];
    unsigned int readerFill[maxNumberOfNeurons];
    unsigned int readerList[numberOfLinks];
    unsigned char baseTrace[17][maxNumberOfNeurons];
    unsigned int deltaSetNeurons[128];
    unsigned char deltaSetValues[128];
    unsigned int nextDeltaNeurons[128];
    unsigned char nextDeltaValues[128];
    unsigned char inDeltaSet[maxNumberOfNeurons];
    unsigned char deltaValueOf[maxNumberOfNeurons];
    unsigned char queued[maxNumberOfNeurons];
    unsigned int queuedList[maxNumberOfNeurons];

    // Batch-start snapshot, undo log of folded neutral steps, per-lane step bookkeeping.
    unsigned char batchInitial[maxNumberOfNeurons];
    struct UndoEntry
    {
        unsigned int index;
        unsigned int oldValue;
    };
    UndoEntry undoLog[mutationSeedCount];
    unsigned int undoLogLen;
    unsigned int laneUndoMarker[SIMD_LANES];
    unsigned long long laneStepOf[SIMD_LANES];
    unsigned int localTouchIndex[BPP9000_MAX_CHANGES_PER_STEP];
    unsigned int localTouchValue[BPP9000_MAX_CHANGES_PER_STEP];
    unsigned int localTouchCount;

    // Lanes in laneMask read altOffset instead of the base source of this slot.
    struct WireFixup
    {
        unsigned int altOffset;
        unsigned long long laneMask;
        int next;
    };
    WireFixup wireFixups[SIMD_MAX_FIXUPS];
    int wireFixupHead[numberOfLinks];
    unsigned int numWireFixups;
    unsigned int wireTouchedSlots[SIMD_MAX_FIXUPS];
    unsigned int numWireTouchedSlots;
    unsigned char wireIsDirty[maxNumberOfNeurons];
    unsigned char wireEvalSet[maxNumberOfNeurons];
    unsigned int batchNeighborIndices[numberOfLinks];

    // Lanes in laneMask take value when the neuron's LUT index equals entryIdx.
    struct LutFixup
    {
        unsigned char entryIdx;
        unsigned char value;
        unsigned long long laneMask;
        int next;
    };
    LutFixup lutFixups[SIMD_MAX_FIXUPS];
    int lutFixupHead[maxNumberOfNeurons];
    unsigned int numLutFixups;
    unsigned int lutTouchedNeurons[maxNumberOfNeurons];
    unsigned int numLutTouchedNeurons;
    unsigned char batchLut[maxNumberOfNeurons * lutSize];

    // Per lane: next emit index, frame error, last 64 wrong bits; ringBits keeps older bits for sliding lanes.
    int laneCount[SIMD_LANES];
    unsigned int laneSum[SIMD_LANES];
    unsigned long long laneBits[SIMD_LANES];
    static constexpr unsigned long long ringWords = (shiftCap + windowWidth) / 64 + 1;
    static_assert(windowWidth >= 64, "a sliding lane reads the bit leaving its frame from ringBits");
    unsigned long long ringBits[SIMD_LANES][ringWords];
    unsigned long long slidingLanes;

    // A batch stops once its outcome is fixed (batchDecided).
    bool bpEarlyStop;
    unsigned long long bpLaneRange;
    unsigned long long bpExploitLanes;
    unsigned long long bpFinishedLanes;
    unsigned long long bpAcceptLanes;
    unsigned int bpDoomBound;
    Rating bpCur;
    unsigned long long bpK;
    // targetOutputs widened to 32 bits for the per-lane gather.
    unsigned int targetDw[sequenceLength];
    static_assert(shiftCap + windowWidth < 0x7FFFFFFFULL, "emit indices are 32-bit lanes");
    unsigned int laneErr[SIMD_LANES];
    unsigned int laneShift[SIMD_LANES];

    template<typename F>
    void diffBytes(const unsigned char* a, const unsigned char* b, unsigned long long n, F&& onDiff)
    {
        unsigned long long i = 0;
        for (; i + 64 <= n; i += 64)
        {
            const __m512i va = _mm512_loadu_si512((const void*)(a + i));
            const __m512i vb = _mm512_loadu_si512((const void*)(b + i));
            unsigned long long mm = (unsigned long long)_mm512_cmpneq_epi8_mask(va, vb);
            while (mm)
            {
                const unsigned int bit = (unsigned int)_tzcnt_u64(mm);
                mm &= (mm - 1);
                onDiff(i + bit);
            }
        }
        for (; i < n; ++i)
        {
            if (a[i] != b[i])
            {
                onDiff(i);
            }
        }
    }

    template<typename F>
    void diffU32(const unsigned int* a, const unsigned int* b, unsigned long long n, F&& onDiff)
    {
        unsigned long long i = 0;
        for (; i + 16 <= n; i += 16)
        {
            const __m512i va = _mm512_loadu_si512((const void*)(a + i));
            const __m512i vb = _mm512_loadu_si512((const void*)(b + i));
            unsigned int mm = (unsigned int)_mm512_cmpneq_epi32_mask(va, vb);
            while (mm)
            {
                const unsigned int bit = (unsigned int)_tzcnt_u32(mm);
                mm &= (mm - 1);
                onDiff(i + bit);
            }
        }
        for (; i < n; ++i)
        {
            if (a[i] != b[i])
            {
                onDiff(i);
            }
        }
    }

    // The input transpose puts lane 8j+i at byte 8j+7-i, so a lane mask is bit-reversed within each byte.
    static unsigned long long lanesToPositions(unsigned long long m)
    {
        m = ((m >> 1) & 0x5555555555555555ULL) | ((m & 0x5555555555555555ULL) << 1);
        m = ((m >> 2) & 0x3333333333333333ULL) | ((m & 0x3333333333333333ULL) << 2);
        m = ((m >> 4) & 0x0F0F0F0F0F0F0F0FULL) | ((m & 0x0F0F0F0F0F0F0F0FULL) << 4);
        return m;
    }

    unsigned long long rowMaskOf(unsigned long long laneMask) const
    {
        return (bpWidth == 32) ? ((laneMask & 0xFFFFFFFFULL) | (laneMask << 32)) : laneMask;
    }

    // code = t0 | t1 << 2 | t2 << 4 of base-3 entry e = t0 + 3 * t1 + 9 * t2.
    static unsigned int codeOfEntry(unsigned int e)
    {
        return (e % 3) | (((e / 3) % 3) << 2) | ((e / 9) << 4);
    }

    // Transpose and regroup permutations for 64- and 32-lane batches.
    void buildBpConstants()
    {
        const unsigned int base[6] = { 0, 8, 16, 24, 64, 72 };
        for (unsigned int j = 0; j < 8; ++j)
        {
            for (unsigned int k = 0; k < 8; ++k)
            {
                const unsigned int plane = 7 - k;
                bpPermIn[8 * j + k] = (unsigned char)((plane < 6) ? (base[plane] + j) : 56);
            }
        }
        const unsigned int base32[6] = { 0, 4, 8, 12, 32, 36 };
        for (unsigned int j = 0; j < 4; ++j)
        {
            for (unsigned int k = 0; k < 8; ++k)
            {
                const unsigned int plane = 7 - k;
                bpPermIn32[8 * j + k] = (unsigned char)((plane < 6) ? (base32[plane] + j) : 16);
            }
        }
        for (unsigned int slot = 0; slot < 4; ++slot)
        {
            for (unsigned int b = 0; b < 2; ++b)
            {
                for (unsigned int j = 0; j < 4; ++j)
                {
                    bpPermOut32[8 * slot + 4 * b + j] = (unsigned char)(8 * j + 2 * slot + b);
                }
            }
        }
        for (unsigned int s = 0; s < 4; ++s)
        {
            for (unsigned int b = 0; b < 2; ++b)
            {
                for (unsigned int j = 0; j < 8; ++j)
                {
                    bpPermOut[16 * s + 8 * b + j] = (unsigned char)(8 * j + 2 * s + b);
                }
            }
        }
    }

    void buildCone()
    {
        setMem(cone, sizeof(cone), 0);
        unsigned int sp = 0;
        cone[controlIndex] = 1;
        coneStack[sp++] = controlIndex;
        if (!cone[outputIndex])
        {
            cone[outputIndex] = 1;
            coneStack[sp++] = outputIndex;
        }
        while (sp > 0)
        {
            const unsigned int n = coneStack[--sp];
            for (unsigned long long r = 0; r < numberOfNeighbors; ++r)
            {
                const unsigned int src = neighborIndices[n * numberOfNeighbors + r];
                if (!cone[src])
                {
                    cone[src] = 1;
                    coneStack[sp++] = src;
                }
            }
        }
    }

    void buildReaders()
    {
        setMem(readerStart, sizeof(readerStart), 0);
        for (unsigned long long i = 0; i < numberOfLinks; ++i)
        {
            readerStart[neighborIndices[i] + 1]++;
        }
        for (unsigned long long n = 0; n < populationThreshold; ++n)
        {
            readerStart[n + 1] += readerStart[n];
        }
        copyMem(readerFill, readerStart, populationThreshold * sizeof(unsigned int));
        for (unsigned long long i = 0; i < numberOfLinks; ++i)
        {
            const unsigned int src = neighborIndices[i];
            readerList[readerFill[src]++] = (unsigned int)i;
        }
    }

    // Base wiring, cone and readers; rebuilt per walk and after each accepted rewire.
    void buildSimdWireBase()
    {
        copyMem(simdWire, neighborIndices, sizeof(simdWire));
        setMem(wireFixupHead, sizeof(wireFixupHead), 0xFF);
        setMem(wireIsDirty, sizeof(wireIsDirty), 0);
        numWireTouchedSlots = 0;
        buildCone();
        buildReaders();
        bpStartOrderValid = false;
    }

    void rebuildBpTab(const unsigned char* srcLut)
    {
        setMem(bpTab, sizeof(bpTab), 0);
        for (unsigned long long n = 0; n < populationThreshold; ++n)
        {
            unsigned char* table = bpTab[n];
            for (unsigned int e = 0; e < lutSize; ++e)
            {
                table[codeOfEntry(e)] = srcLut[n * lutSize + e];
            }
        }
    }

    // Base LUT tables; rebuilt per walk and after each accepted LUT edit.
    void buildSimdLutBase()
    {
        setMem(lutFixupHead, sizeof(lutFixupHead), 0xFF);
        numLutTouchedNeurons = 0;
        rebuildBpTab(curLut);
        bpStartOrderValid = false;
    }

    // Row order: clean evaluated rows, then fix-up rows by shape, then rows nothing evaluated reads.
    void buildOrder(const unsigned char* evalSet)
    {
        unsigned int k = 0;
        for (unsigned int n = 0; n < (unsigned int)populationThreshold; ++n)
        {
            if (evalSet[n] && bpNeuronShape[n] == BP_CLEAN)
            {
                ordNeuron[k++] = n;
            }
        }
        bpCleanRows = k;
        for (unsigned int shape = 0; shape < BP_CLEAN; ++shape)
        {
            for (unsigned int n = 0; n < (unsigned int)populationThreshold; ++n)
            {
                if (evalSet[n] && bpNeuronShape[n] == shape)
                {
                    ordNeuron[k++] = n;
                }
            }
        }
        bpEvalRows = k;
        for (unsigned int n = 0; n < (unsigned int)populationThreshold; ++n)
        {
            if (!evalSet[n])
            {
                ordNeuron[k++] = n;
            }
        }
        for (unsigned int i = 0; i < (unsigned int)populationThreshold; ++i)
        {
            intIdx[ordNeuron[i]] = i;
        }
        bpCleanGroups = bpCleanRows / 4;
        bpEvalGroups = (bpEvalRows + 3) / 4;

        const unsigned int rows = bpEvalGroups * 4;
        for (unsigned int i = 0; i < rows; ++i)
        {
            const unsigned int n = ordNeuron[i];
            const unsigned int* w = &simdWire[n * numberOfNeighbors];
            bpRec[i] = (unsigned long long)intIdx[w[0]] | ((unsigned long long)intIdx[w[1]] << 16)
                | ((unsigned long long)intIdx[w[2]] << 32);
            const unsigned int shiftBits = 2 * (i & 3);
            const __m512i t = _mm512_loadu_si512((const void*)bpTab[n]);
            _mm512_storeu_si512((void*)bpTabInt[i], _mm512_sll_epi16(t, _mm_cvtsi32_si128((int)shiftBits)));
        }
        const unsigned int fixStart = bpCleanGroups * 4;
        if (rows > fixStart)
        {
            setMem(bpRowFix + fixStart, (rows - fixStart) * sizeof(bpRowFix[0]), 0);
        }
        for (unsigned int i = fixStart; i < rows; ++i)
        {
            bpRowShape[i] = (i < bpEvalRows) ? bpNeuronShape[ordNeuron[i]] : (unsigned char)BP_CLEAN;
        }
    }

    void buildRuns()
    {
        bpNumRuns = 0;
        for (unsigned int g = bpCleanGroups; g < bpEvalGroups; ++g)
        {
            const unsigned int i0 = g * 4;
            const unsigned char s0 = bpRowShape[i0];
            const bool uniform = (s0 != BP_CLEAN) && bpRowShape[i0 + 1] == s0 && bpRowShape[i0 + 2] == s0 && bpRowShape[i0 + 3] == s0;
            const unsigned char shape = uniform ? s0 : BP_MIXED;
            if (bpNumRuns > 0 && bpRunShape[bpNumRuns - 1] == shape && bpRunEnd[bpNumRuns - 1] == g)
            {
                bpRunEnd[bpNumRuns - 1] = g + 1;
            }
            else if (bpNumRuns < BP_MAX_RUNS)
            {
                bpRunShape[bpNumRuns] = shape;
                bpRunBegin[bpNumRuns] = g;
                bpRunEnd[bpNumRuns] = g + 1;
                ++bpNumRuns;
            }
            else
            {
                bpRunShape[bpNumRuns - 1] = BP_MIXED;
                bpRunEnd[bpNumRuns - 1] = bpEvalGroups;
                break;
            }
        }
    }

    void buildBpInitialState(const unsigned char* initial)
    {
        if (bpWidth == 32)
        {
            for (unsigned long long n = 0; n < populationThreshold; ++n)
            {
                const unsigned int i = intIdx[n];
                bpState32[0][i][0] = (initial[n] & 1) ? ~0U : 0U;
                bpState32[0][i][1] = (initial[n] & 2) ? ~0U : 0U;
            }
            return;
        }
        for (unsigned long long n = 0; n < populationThreshold; ++n)
        {
            const unsigned int i = intIdx[n];
            bpState[0][i][0] = (initial[n] & 1) ? ~0ULL : 0ULL;
            bpState[0][i][1] = (initial[n] & 2) ? ~0ULL : 0ULL;
        }
    }

    void setLaneInitial(unsigned int n, unsigned int lane, unsigned char value)
    {
        const unsigned int i = intIdx[n];
        if (bpWidth == 32)
        {
            const unsigned int bit = 1U << lane;
            bpState32[0][i][0] = (bpState32[0][i][0] & ~bit) | ((value & 1) ? bit : 0U);
            bpState32[0][i][1] = (bpState32[0][i][1] & ~bit) | ((value & 2) ? bit : 0U);
            return;
        }
        const unsigned long long bit = 1ULL << lane;
        bpState[0][i][0] = (bpState[0][i][0] & ~bit) | ((value & 1) ? bit : 0ULL);
        bpState[0][i][1] = (bpState[0][i][1] & ~bit) | ((value & 2) ? bit : 0ULL);
    }

    void computeBaseTrace()
    {
        copyMem(baseTrace[0], curInitial, populationThreshold);
        for (unsigned int t = 1; t <= 16; ++t)
        {
            const unsigned char* prevT = baseTrace[t - 1];
            unsigned char* curT = baseTrace[t];
            for (unsigned long long n = 0; n < populationThreshold; ++n)
            {
                const unsigned long long t0 = prevT[neighborIndices[n * numberOfNeighbors + 0]];
                const unsigned long long t1 = prevT[neighborIndices[n * numberOfNeighbors + 1]];
                const unsigned long long t2 = prevT[neighborIndices[n * numberOfNeighbors + 2]];
                curT[n] = curLut[n * lutSize + (t0 + 3 * t1 + 9 * t2)];
            }
        }
    }

    // START mode: true when the flip dies out within 16 ticks without reaching control or output.
    bool isDeadFlipStart()
    {
        unsigned int curCount = localTouchCount;
        for (unsigned int i = 0; i < curCount; ++i)
        {
            const unsigned int n = localTouchIndex[i];
            deltaSetNeurons[i] = n;
            deltaSetValues[i] = (unsigned char)localTouchValue[i];
            inDeltaSet[n] = 1;
            deltaValueOf[n] = (unsigned char)localTouchValue[i];
        }
        bool alive = false;
        for (unsigned int i = 0; i < curCount; ++i)
        {
            if (deltaSetNeurons[i] == controlIndex || deltaSetNeurons[i] == outputIndex)
            {
                alive = true;
            }
        }
        bool dead = false;
        for (unsigned int t = 1; !alive && !dead && t <= 16; ++t)
        {
            unsigned int candCount = 0;
            for (unsigned int i = 0; i < curCount; ++i)
            {
                const unsigned int n = deltaSetNeurons[i];
                for (unsigned int k = readerStart[n]; k < readerStart[n + 1]; ++k)
                {
                    const unsigned int flatSlot = readerList[k];
                    const unsigned int m = flatSlot / (unsigned int)numberOfNeighbors;
                    if (m == controlIndex || m == outputIndex)
                    {
                        alive = true;
                    }
                    if (!queued[m])
                    {
                        queued[m] = 1;
                        queuedList[candCount++] = m;
                    }
                }
            }
            unsigned int newCount = 0;
            for (unsigned int i = 0; i < candCount; ++i)
            {
                const unsigned int m = queuedList[i];
                queued[m] = 0;
                const unsigned int s0 = neighborIndices[m * numberOfNeighbors + 0];
                const unsigned int s1 = neighborIndices[m * numberOfNeighbors + 1];
                const unsigned int s2 = neighborIndices[m * numberOfNeighbors + 2];
                const unsigned int v0 = inDeltaSet[s0] ? deltaValueOf[s0] : baseTrace[t - 1][s0];
                const unsigned int v1 = inDeltaSet[s1] ? deltaValueOf[s1] : baseTrace[t - 1][s1];
                const unsigned int v2 = inDeltaSet[s2] ? deltaValueOf[s2] : baseTrace[t - 1][s2];
                const unsigned char newVal = curLut[m * lutSize + (v0 + 3 * v1 + 9 * v2)];
                if (newVal != baseTrace[t][m])
                {
                    if (newCount < 96)
                    {
                        nextDeltaNeurons[newCount] = m;
                        nextDeltaValues[newCount] = newVal;
                    }
                    ++newCount;
                }
            }
            for (unsigned int i = 0; i < curCount; ++i)
            {
                inDeltaSet[deltaSetNeurons[i]] = 0;
            }
            if (!alive)
            {
                if (newCount > 64)
                {
                    alive = true;
                }
                else if (newCount == 0)
                {
                    dead = true;
                }
                else
                {
                    for (unsigned int i = 0; i < newCount; ++i)
                    {
                        deltaSetNeurons[i] = nextDeltaNeurons[i];
                        deltaSetValues[i] = nextDeltaValues[i];
                        inDeltaSet[nextDeltaNeurons[i]] = 1;
                        deltaValueOf[nextDeltaNeurons[i]] = nextDeltaValues[i];
                    }
                    curCount = newCount;
                }
            }
        }
        if (!dead)
        {
            for (unsigned int i = 0; i < curCount; ++i)
            {
                inDeltaSet[deltaSetNeurons[i]] = 0;
            }
        }
        return dead;
    }

    void undoStartNeutralsBack(unsigned int marker)
    {
        while (undoLogLen > marker)
        {
            --undoLogLen;
            curInitial[undoLog[undoLogLen].index] = (unsigned char)undoLog[undoLogLen].oldValue;
        }
    }

    void undoWiringNeutralsBack(unsigned int marker)
    {
        while (undoLogLen > marker)
        {
            --undoLogLen;
            neighborIndices[undoLog[undoLogLen].index] = undoLog[undoLogLen].oldValue;
        }
    }

    void undoLutNeutralsBack(unsigned int marker)
    {
        while (undoLogLen > marker)
        {
            --undoLogLen;
            curLut[undoLog[undoLogLen].index] = (unsigned char)undoLog[undoLogLen].oldValue;
        }
    }

    void prepareStartBatch()
    {
        if (!bpStartOrderValid)
        {
            setMem(bpNeuronShape, sizeof(bpNeuronShape), BP_CLEAN);
            buildOrder(cone);
            buildRuns();
            bpStartOrderValid = true;
        }
        buildBpInitialState(curInitial);
    }

    unsigned int collectStartDeviations(unsigned long long& stepCursor, unsigned int L, bool curValid, unsigned int firstLane)
    {
        copyMem(batchInitial, curInitial, sizeof(batchInitial));
        undoLogLen = 0;
        computeBaseTrace();

        unsigned int nLanes = firstLane;
        while (nLanes < bpWidth && stepCursor < numberOfMutations)
        {
            snapshotPrev();
            for (unsigned int i = 0; i < L; ++i)
            {
                mutate(BPP9000_MODE_START, mutationSeed[stepCursor * BPP9000_MAX_CHANGES_PER_STEP + i]);
            }

            localTouchCount = 0;
            diffBytes(curInitial, prevInitial, populationThreshold, [&](unsigned long long n)
            {
                localTouchIndex[localTouchCount] = (unsigned int)n;
                localTouchValue[localTouchCount] = curInitial[n];
                ++localTouchCount;
            });

            bool anyInCone = false;
            for (unsigned int i = 0; i < localTouchCount; ++i)
            {
                if (cone[localTouchIndex[i]])
                {
                    anyInCone = true;
                    break;
                }
            }

            bool neutral;
            if (!curValid)
            {
                neutral = false;
            }
            else if (!anyInCone)
            {
                neutral = true;
            }
            else
            {
                neutral = isDeadFlipStart();
            }

            if (neutral)
            {
                for (unsigned int i = 0; i < localTouchCount; ++i)
                {
                    undoLog[undoLogLen].index = localTouchIndex[i];
                    undoLog[undoLogLen].oldValue = prevInitial[localTouchIndex[i]];
                    ++undoLogLen;
                }
                computeBaseTrace();
                ++stepCursor;
            }
            else
            {
                laneUndoMarker[nLanes] = undoLogLen;
                laneStepOf[nLanes] = stepCursor;
                const unsigned int lane = nLanes;
                diffBytes(curInitial, batchInitial, populationThreshold, [&](unsigned long long n)
                {
                    setLaneInitial((unsigned int)n, lane, curInitial[n]);
                });
                ++nLanes;
                rollbackPrev();
                ++stepCursor;
            }
        }
        return nLanes;
    }

    // A step that cannot reach control/output ties cur and is folded in without a lane, but only while cur is
    // valid: the walk rejects a tie against an invalid cur. The same rule holds in the START and LUT collectors.
    unsigned int collectWiringDeviations(unsigned long long& stepCursor, unsigned int L, bool curValid, unsigned int firstLane)
    {
        for (unsigned int i = 0; i < numWireTouchedSlots; ++i)
        {
            const unsigned int slot = wireTouchedSlots[i];
            wireFixupHead[slot] = -1;
            wireIsDirty[slot / numberOfNeighbors] = 0;
        }
        numWireTouchedSlots = 0;
        numWireFixups = 0;
        undoLogLen = 0;
        copyMem(batchNeighborIndices, neighborIndices, sizeof(batchNeighborIndices));
        copyMem(simdWire, neighborIndices, sizeof(simdWire));

        unsigned int pendingTouches = 0;
        unsigned int nLanes = firstLane;
        while (nLanes < bpWidth && stepCursor < numberOfMutations)
        {
            if (numWireFixups + pendingTouches + L > SIMD_MAX_FIXUPS)
            {
                break;
            }
            snapshotPrev();
            for (unsigned int i = 0; i < L; ++i)
            {
                mutate(BPP9000_MODE_WIRING, mutationSeed[stepCursor * BPP9000_MAX_CHANGES_PER_STEP + i]);
            }

            localTouchCount = 0;
            diffU32(neighborIndices, prevNeighborIndices, numberOfLinks, [&](unsigned long long flatSlot)
            {
                localTouchIndex[localTouchCount] = (unsigned int)flatSlot;
                localTouchValue[localTouchCount] = neighborIndices[flatSlot];
                ++localTouchCount;
            });

            bool anyInCone = false;
            for (unsigned int i = 0; i < localTouchCount; ++i)
            {
                if (cone[localTouchIndex[i] / (unsigned int)numberOfNeighbors])
                {
                    anyInCone = true;
                    break;
                }
            }

            const bool neutral = curValid && !anyInCone;

            if (neutral)
            {
                for (unsigned int i = 0; i < localTouchCount; ++i)
                {
                    undoLog[undoLogLen].index = localTouchIndex[i];
                    undoLog[undoLogLen].oldValue = prevNeighborIndices[localTouchIndex[i]];
                    ++undoLogLen;
                }
                pendingTouches += localTouchCount;
                ++stepCursor;
            }
            else
            {
                laneUndoMarker[nLanes] = undoLogLen;
                laneStepOf[nLanes] = stepCursor;
                const unsigned int j = nLanes;
                diffU32(neighborIndices, batchNeighborIndices, numberOfLinks, [&](unsigned long long flatSlot)
                {
                    const unsigned int altOffset = (unsigned int)neighborIndices[flatSlot];
                    const bool firstTouch = (wireFixupHead[flatSlot] < 0);
                    int k = wireFixupHead[flatSlot];
                    int found = -1;
                    while (k >= 0)
                    {
                        if (wireFixups[k].altOffset == altOffset)
                        {
                            found = k;
                            break;
                        }
                        k = wireFixups[k].next;
                    }
                    if (found >= 0)
                    {
                        wireFixups[found].laneMask |= (1ULL << j);
                    }
                    else
                    {
                        if (firstTouch)
                        {
                            wireTouchedSlots[numWireTouchedSlots++] = (unsigned int)flatSlot;
                            wireIsDirty[flatSlot / numberOfNeighbors] = 1;
                        }
                        const unsigned int idx = numWireFixups++;
                        wireFixups[idx].altOffset = altOffset;
                        wireFixups[idx].laneMask = (1ULL << j);
                        wireFixups[idx].next = wireFixupHead[flatSlot];
                        wireFixupHead[flatSlot] = (int)idx;
                    }
                });

                pendingTouches = 0;
                ++nLanes;
                rollbackPrev();
                ++stepCursor;
            }
        }

        copyMem(wireEvalSet, cone, populationThreshold);
        unsigned int sp = 0;
        for (unsigned long long n = 0; n < populationThreshold; ++n)
        {
            if (cone[n])
            {
                coneStack[sp++] = (unsigned int)n;
            }
        }
        while (sp > 0)
        {
            const unsigned int n = coneStack[--sp];
            const unsigned int flatBase = n * numberOfNeighbors;
            for (unsigned long long r = 0; r < numberOfNeighbors; ++r)
            {
                const unsigned int slot = flatBase + (unsigned int)r;
                const unsigned int src = simdWire[slot];
                if (!wireEvalSet[src])
                {
                    wireEvalSet[src] = 1;
                    coneStack[sp++] = src;
                }
                for (int k = wireFixupHead[slot]; k >= 0; k = wireFixups[k].next)
                {
                    const unsigned int altSrc = wireFixups[k].altOffset;
                    if (!wireEvalSet[altSrc])
                    {
                        wireEvalSet[altSrc] = 1;
                        coneStack[sp++] = altSrc;
                    }
                }
            }
        }

        for (unsigned int n = 0; n < (unsigned int)populationThreshold; ++n)
        {
            unsigned char shape = BP_CLEAN;
            if (wireIsDirty[n])
            {
                unsigned int cnt[3] = { 0, 0, 0 };
                for (unsigned int r = 0; r < 3; ++r)
                {
                    for (int k = wireFixupHead[n * numberOfNeighbors + r]; k >= 0; k = wireFixups[k].next)
                    {
                        ++cnt[r];
                    }
                }
                const unsigned int total = cnt[0] + cnt[1] + cnt[2];
                if (cnt[0] >= 2 || cnt[1] >= 2 || cnt[2] >= 2)
                {
                    shape = BP_WGEN;
                }
                else if (total == 1)
                {
                    shape = (cnt[0] == 1) ? (unsigned char)BP_W1R0 : (cnt[1] == 1) ? (unsigned char)BP_W1R1 : (unsigned char)BP_W1R2;
                }
                else if (total > 1)
                {
                    shape = BP_WPAD;
                }
            }
            bpNeuronShape[n] = shape;
        }
        buildOrder(wireEvalSet);

        unsigned int pos = 0;
        for (unsigned int i = bpCleanRows; i < bpEvalRows; ++i)
        {
            const unsigned int n = ordNeuron[i];
            const unsigned int start = pos;
            if (bpRowShape[i] == BP_WPAD)
            {
                for (unsigned int r = 0; r < 3; ++r)
                {
                    const int k = wireFixupHead[n * numberOfNeighbors + r];
                    bpWireFix[pos].mask = (k >= 0) ? rowMaskOf(wireFixups[k].laneMask) : 0ULL;
                    bpWireFix[pos].alt = (k >= 0) ? intIdx[wireFixups[k].altOffset] : intIdx[simdWire[n * numberOfNeighbors + r]];
                    bpWireFix[pos].role = r;
                    ++pos;
                }
            }
            else
            {
                for (unsigned int r = 0; r < (unsigned int)numberOfNeighbors; ++r)
                {
                    for (int k = wireFixupHead[n * numberOfNeighbors + r]; k >= 0; k = wireFixups[k].next)
                    {
                        bpWireFix[pos].mask = rowMaskOf(wireFixups[k].laneMask);
                        bpWireFix[pos].alt = intIdx[wireFixups[k].altOffset];
                        bpWireFix[pos].role = r;
                        ++pos;
                    }
                }
            }
            bpRowFix[i] = start | ((pos - start) << 16);
        }
        buildRuns();

        buildBpInitialState(curInitial);
        return nLanes;
    }

    unsigned int collectLutDeviations(unsigned long long& stepCursor, unsigned int L, bool curValid, unsigned int firstLane)
    {
        for (unsigned int i = 0; i < numLutTouchedNeurons; ++i)
        {
            lutFixupHead[lutTouchedNeurons[i]] = -1;
        }
        numLutTouchedNeurons = 0;
        numLutFixups = 0;
        undoLogLen = 0;
        copyMem(batchLut, curLut, sizeof(batchLut));
        const unsigned long long total = maxNumberOfNeurons * lutSize;

        unsigned int pendingTouches = 0;
        unsigned int nLanes = firstLane;
        while (nLanes < bpWidth && stepCursor < numberOfMutations)
        {
            if (numLutFixups + pendingTouches + L > SIMD_MAX_FIXUPS)
            {
                copyMem(batchLut, curLut, sizeof(batchLut));
                rebuildBpTab(batchLut);
                pendingTouches = 0;
                if (numLutFixups + L > SIMD_MAX_FIXUPS)
                {
                    break;
                }
            }
            snapshotPrev();
            for (unsigned int i = 0; i < L; ++i)
            {
                mutate(BPP9000_MODE_LUT, mutationSeed[stepCursor * BPP9000_MAX_CHANGES_PER_STEP + i]);
            }

            localTouchCount = 0;
            diffBytes(curLut, prevLut, total, [&](unsigned long long flatIdx)
            {
                localTouchIndex[localTouchCount] = (unsigned int)flatIdx;
                localTouchValue[localTouchCount] = curLut[flatIdx];
                ++localTouchCount;
            });
            bool anyInCone = false;
            for (unsigned int i = 0; i < localTouchCount; ++i)
            {
                if (cone[localTouchIndex[i] / (unsigned int)lutSize])
                {
                    anyInCone = true;
                    break;
                }
            }

            const bool neutral = curValid && !anyInCone;

            if (neutral)
            {
                for (unsigned int i = 0; i < localTouchCount; ++i)
                {
                    undoLog[undoLogLen].index = localTouchIndex[i];
                    undoLog[undoLogLen].oldValue = prevLut[localTouchIndex[i]];
                    ++undoLogLen;
                }
                pendingTouches += localTouchCount;
                ++stepCursor;
            }
            else
            {
                laneUndoMarker[nLanes] = undoLogLen;
                laneStepOf[nLanes] = stepCursor;
                const unsigned int j = nLanes;
                diffBytes(curLut, batchLut, total, [&](unsigned long long flatIdx)
                {
                    const unsigned long long n = flatIdx / lutSize;
                    const unsigned int entryIdx = (unsigned int)(flatIdx % lutSize);
                    const unsigned char value = curLut[flatIdx];
                    const bool firstTouch = (lutFixupHead[n] < 0);
                    int k = lutFixupHead[n];
                    int found = -1;
                    while (k >= 0)
                    {
                        if (lutFixups[k].entryIdx == entryIdx && lutFixups[k].value == value)
                        {
                            found = k;
                            break;
                        }
                        k = lutFixups[k].next;
                    }
                    if (found >= 0)
                    {
                        lutFixups[found].laneMask |= (1ULL << j);
                    }
                    else
                    {
                        if (firstTouch)
                        {
                            lutTouchedNeurons[numLutTouchedNeurons++] = (unsigned int)n;
                        }
                        const unsigned int idx = numLutFixups++;
                        lutFixups[idx].entryIdx = (unsigned char)entryIdx;
                        lutFixups[idx].value = value;
                        lutFixups[idx].laneMask = (1ULL << j);
                        lutFixups[idx].next = lutFixupHead[n];
                        lutFixupHead[n] = (int)idx;
                    }
                });

                pendingTouches = 0;
                ++nLanes;
                rollbackPrev();
                ++stepCursor;
            }
        }

        for (unsigned int n = 0; n < (unsigned int)populationThreshold; ++n)
        {
            unsigned int cnt = 0;
            for (int k = lutFixupHead[n]; k >= 0; k = lutFixups[k].next)
            {
                ++cnt;
            }
            bpNeuronShape[n] = (cnt == 0) ? (unsigned char)BP_CLEAN : (cnt == 1) ? (unsigned char)BP_L1
                : (cnt == 2) ? (unsigned char)BP_L2 : (unsigned char)BP_LN;
        }
        buildOrder(cone);

        unsigned int pos = 0;
        for (unsigned int i = bpCleanRows; i < bpEvalRows; ++i)
        {
            const unsigned int n = ordNeuron[i];
            const unsigned int start = pos;
            const unsigned int shiftBits = 2 * (i & 3);
            for (int k = lutFixupHead[n]; k >= 0; k = lutFixups[k].next)
            {
                bpLutFix[pos].mask = lanesToPositions(lutFixups[k].laneMask);
                bpLutFix[pos].code = (unsigned char)codeOfEntry(lutFixups[k].entryIdx);
                bpLutFix[pos].value = (unsigned char)(lutFixups[k].value << shiftBits);
                ++pos;
            }
            bpRowFix[i] = start | ((pos - start) << 16);
        }
        buildRuns();

        buildBpInitialState(curInitial);
        return nLanes;
    }

    static __mmask64 laneMaskOf(unsigned int nLanes)
    {
        return (nLanes >= SIMD_LANES) ? ((__mmask64)~0ULL) : (__mmask64)((1ULL << nLanes) - 1ULL);
    }

    // One tick of emits for all lanes: grade, slide each lane's frame, finish the lanes that stop.
    void processEmits(unsigned long long ctrlPlane1, unsigned long long out0, unsigned long long out1, __mmask64& activeMask, unsigned long long f0)
    {
        const unsigned long long emit = ~ctrlPlane1 & (unsigned long long)activeMask;
        if (!emit)
        {
            return;
        }
        const __m512i f0v = _mm512_set1_epi32((int)f0);
        const __m512i one = _mm512_set1_epi32(1);
        const __m512i two = _mm512_set1_epi32(2);
        const __m512i low6 = _mm512_set1_epi32(63);
        const __m512i lastRel = _mm512_set1_epi32((int)windowWidth - 1);
        const unsigned long long slidingBefore = slidingLanes;
        unsigned long long graded = 0;
        unsigned long long wrong = 0;
        unsigned long long full = 0;
        unsigned long long complete = 0;
        for (unsigned int q = 0; q < 4; ++q)
        {
            const __mmask16 e = (__mmask16)(emit >> (16 * q));
            if (!e)
            {
                continue;
            }
            const __m512i cnt = _mm512_loadu_si512((const void*)(laneCount + 16 * q));
            const __mmask16 g = _mm512_mask_cmpge_epi32_mask(e, cnt, f0v);
            const __m512i target = _mm512_mask_i32gather_epi32(_mm512_setzero_si512(), g, cnt, (const void*)targetDw, 4);
            const __m512i trit = _mm512_mask_mov_epi32(_mm512_maskz_mov_epi32((__mmask16)(out0 >> (16 * q)), one),
                (__mmask16)(out1 >> (16 * q)), two);
            const __mmask16 w = _mm512_mask_cmpneq_epi32_mask(g, trit, target);
            const __m512i rel = _mm512_sub_epi32(cnt, f0v);
            const __mmask16 fl = _mm512_mask_cmpeq_epi32_mask(g, _mm512_and_si512(rel, low6), low6);
            const __mmask16 cp = _mm512_mask_cmpeq_epi32_mask(g, rel, lastRel);
            _mm512_storeu_si512((void*)(laneCount + 16 * q), _mm512_mask_add_epi32(cnt, e, cnt, one));
            const __m512i sum = _mm512_loadu_si512((const void*)(laneSum + 16 * q));
            _mm512_storeu_si512((void*)(laneSum + 16 * q), _mm512_mask_add_epi32(sum, (__mmask16)(w & ~(slidingBefore >> (16 * q))), sum, one));
            graded |= (unsigned long long)g << (16 * q);
            wrong |= (unsigned long long)w << (16 * q);
            full |= (unsigned long long)fl << (16 * q);
            complete |= (unsigned long long)cp << (16 * q);
        }
        if (!graded)
        {
            return;
        }
        const __m512i top = _mm512_set1_epi64((long long)0x8000000000000000ULL);
        for (unsigned int m = 0; m < 8; ++m)
        {
            const __mmask8 g8 = (__mmask8)(graded >> (8 * m));
            if (!g8)
            {
                continue;
            }
            __m512i reg = _mm512_loadu_si512((const void*)(laneBits + 8 * m));
            reg = _mm512_mask_srli_epi64(reg, g8, reg, 1);
            reg = _mm512_mask_or_epi64(reg, (__mmask8)(wrong >> (8 * m)), reg, top);
            _mm512_storeu_si512((void*)(laneBits + 8 * m), reg);
        }
        for (unsigned long long fm = full; fm; fm &= fm - 1)
        {
            const unsigned int l = (unsigned int)_tzcnt_u64(fm);
            const unsigned int relJ = (unsigned int)(laneCount[l] - 1 - (int)f0);
            ringBits[l][relJ >> 6] = laneBits[l];
        }
        for (unsigned long long cm = complete; cm; cm &= cm - 1)
        {
            const unsigned int l = (unsigned int)_tzcnt_u64(cm);
            if (laneSum[l] <= advanceThreshold && f0 < shiftCap)
            {
                slidingLanes |= 1ULL << l;
            }
            else
            {
                finishLane(l, laneSum[l], (unsigned int)f0);
                activeMask = (__mmask64)(activeMask & ~(1ULL << l));
            }
        }
        for (unsigned long long sm = slidingBefore & graded; sm; sm &= sm - 1)
        {
            const unsigned int l = (unsigned int)_tzcnt_u64(sm);
            const unsigned long long relJ = (unsigned long long)(laneCount[l] - 1 - (int)f0);
            const unsigned long long gone = relJ - windowWidth;
            const unsigned int leaving = (unsigned int)((ringBits[l][gone >> 6] >> (gone & 63)) & 1ULL);
            laneSum[l] = laneSum[l] + (unsigned int)((wrong >> l) & 1ULL) - leaving;
            const unsigned long long frame = f0 + relJ + 1 - windowWidth;
            if (!(laneSum[l] <= advanceThreshold && frame < shiftCap))
            {
                finishLane(l, laneSum[l], (unsigned int)frame);
                activeMask = (__mmask64)(activeMask & ~(1ULL << l));
            }
        }
    }

    void timeoutRemaining(__mmask64& active)
    {
        unsigned long long am = (unsigned long long)active;
        while (am)
        {
            const unsigned int l = (unsigned int)_tzcnt_u64((unsigned long long)(am));
            am &= (am - 1);
            laneErr[l] = INFINITE_ERROR;
            laneShift[l] = 0;
        }
        active = 0;
    }

    void finishLane(unsigned int l, unsigned int error, unsigned int frame)
    {
        laneErr[l] = error;
        laneShift[l] = frame;
        bpFinishedLanes |= 1ULL << l;
        const Rating r{ error, frame };
        const bool ok = (laneStepOf[l] < bpK) ? (r.isValid() && r.errorWorseOrEqual(bpCur)) : (r.isValid() && r.isNotWorseThan(bpCur));
        if (ok)
        {
            bpAcceptLanes |= 1ULL << l;
        }
    }

    // True once every lane before the first accepting one is known rejected; lanes still running get an invalid rating.
    bool batchDecided(__mmask64& activeMask)
    {
        const unsigned long long active = (unsigned long long)activeMask;
        const __m512i bound = _mm512_set1_epi32((int)bpDoomBound);
        unsigned long long over = 0;
        for (unsigned int q = 0; q < 4; ++q)
        {
            const __mmask16 a = (__mmask16)(active >> (16 * q));
            if (a)
            {
                over |= (unsigned long long)_mm512_mask_cmpgt_epu32_mask(a, _mm512_loadu_si512((const void*)(laneSum + 16 * q)), bound) << (16 * q);
            }
        }
        const unsigned long long doomed = over & bpExploitLanes & ~slidingLanes;
        const unsigned long long rejected = (bpFinishedLanes & ~bpAcceptLanes) | doomed;
        const unsigned long long open = bpLaneRange & ~rejected;
        if (open != 0 && (bpAcceptLanes & (open & (0ULL - open))) == 0)
        {
            return false;
        }
        for (unsigned long long am = active; am; am &= am - 1)
        {
            const unsigned int l = (unsigned int)_tzcnt_u64(am);
            laneErr[l] = INFINITE_ERROR;
            laneShift[l] = 0;
        }
        activeMask = 0;
        return true;
    }

    // One byte per lane: code = t0 | t1 << 2 | t2 << 4 from the three source rows (bits 6-7 undefined).
    static inline __m512i bpCode(__m512i a, __m512i b, __m512i c, __m512i permIn, __m512i eyeRev)
    {
        const __m256i a256 = _mm512_castsi512_si256(a);
        const __m512i ab = _mm512_inserti64x4(_mm512_castsi256_si512(_mm256_inserti128_si256(a256, _mm512_castsi512_si128(b), 1)), a256, 1);
        const __m512i planes = _mm512_permutex2var_epi8(ab, permIn, c);
        return _mm512_gf2p8affine_epi64_epi8(eyeRev, planes, 0);
    }

    static inline __m512i bpCodeRows(const unsigned long long (*prev)[2], unsigned int ra, unsigned int rb, unsigned int rc,
        __m512i permIn, __m512i eyeRev)
    {
        const __m256i a256 = _mm256_broadcastsi128_si256(_mm_loadu_si128((const __m128i*)(const void*)prev[ra]));
        const __m256i ab256 = _mm256_inserti128_si256(a256, _mm_loadu_si128((const __m128i*)(const void*)prev[rb]), 1);
        const __m512i ab = _mm512_inserti64x4(_mm512_castsi256_si512(ab256), a256, 1);
        const __m512i c = _mm512_broadcast_i32x4(_mm_loadu_si128((const __m128i*)(const void*)prev[rc]));
        return _mm512_gf2p8affine_epi64_epi8(eyeRev, _mm512_permutex2var_epi8(ab, permIn, c), 0);
    }

    static inline __m512i bpRow(const unsigned long long (*prev)[2], unsigned int row)
    {
        return _mm512_broadcast_i64x2(_mm_loadu_si128((const __m128i*)(const void*)prev[row]));
    }

    static inline __m512i bpCleanRow(const unsigned long long (*prev)[2], const unsigned long long* rec,
        const unsigned char (*tab)[64], unsigned int i, __m512i permIn, __m512i eyeRev)
    {
        const unsigned long long r = rec[i];
        const __m512i code = bpCodeRows(prev, (unsigned int)(r & 0xFFFF), (unsigned int)((r >> 16) & 0xFFFF), (unsigned int)((r >> 32) & 0xFFFF),
            permIn, eyeRev);
        return _mm512_permutexvar_epi8(code, _mm512_loadu_si512((const void*)tab[i]));
    }

    // Four rows' outputs transposed back into bit-planes and stored with one 64-byte write.
    static inline void bpStore4(unsigned long long (*out)[2], unsigned int i0, __m512i o0, __m512i o1, __m512i o2,
        __m512i o3, __m512i permOut, __m512i eye)
    {
        const __m512i packed = _mm512_or_si512(_mm512_ternarylogic_epi64(o0, o1, o2, 0xFE), o3);
        _mm512_storeu_si512((void*)out[i0], _mm512_permutexvar_epi8(permOut, _mm512_gf2p8affine_epi64_epi8(eye, packed, 0)));
    }

    static inline void bpCleanGroup(const unsigned long long (*prev)[2], unsigned long long (*out)[2],
        const unsigned long long* rec, const unsigned char (*tab)[64], unsigned int i0,
        __m512i permIn, __m512i permOut, __m512i eye, __m512i eyeRev)
    {
        const __m512i o0 = bpCleanRow(prev, rec, tab, i0 + 0, permIn, eyeRev);
        const __m512i o1 = bpCleanRow(prev, rec, tab, i0 + 1, permIn, eyeRev);
        const __m512i o2 = bpCleanRow(prev, rec, tab, i0 + 2, permIn, eyeRev);
        const __m512i o3 = bpCleanRow(prev, rec, tab, i0 + 3, permIn, eyeRev);
        bpStore4(out, i0, o0, o1, o2, o3, permOut, eye);
    }

    // Lanes in f.mask read the alternate source row.
    static inline __m512i bpMerge(const unsigned long long (*prev)[2], __m512i base, const BpWireFix& f)
    {
        return _mm512_ternarylogic_epi64(_mm512_set1_epi64((long long)f.mask), bpRow(prev, f.alt), base, 0xCA);
    }

    // Positions in f.mask whose code equals f.code take f.value.
    static inline __m512i bpPatch(__m512i out, __m512i cleanCode, const BpLutFix& f)
    {
        const __mmask64 hit = _mm512_mask_cmpeq_epi8_mask((__mmask64)f.mask, cleanCode, _mm512_set1_epi8((char)f.code));
        return _mm512_mask_mov_epi8(out, hit, _mm512_set1_epi8((char)f.value));
    }

    __m512i bpDirtyRow(const unsigned long long (*prev)[2], unsigned int i, __m512i permIn, __m512i eyeRev) const
    {
        const unsigned long long r = bpRec[i];
        __m512i a = bpRow(prev, (unsigned int)(r & 0xFFFF));
        __m512i b = bpRow(prev, (unsigned int)((r >> 16) & 0xFFFF));
        __m512i c = bpRow(prev, (unsigned int)((r >> 32) & 0xFFFF));
        const unsigned int shape = bpRowShape[i];
        const unsigned int fx = bpRowFix[i] & 0xFFFF;
        const unsigned int fixEnd = fx + (bpRowFix[i] >> 16);
        switch (shape)
        {
        case BP_W1R0:
            a = bpMerge(prev, a, bpWireFix[fx]);
            break;
        case BP_W1R1:
            b = bpMerge(prev, b, bpWireFix[fx]);
            break;
        case BP_W1R2:
            c = bpMerge(prev, c, bpWireFix[fx]);
            break;
        case BP_WPAD:
            a = bpMerge(prev, a, bpWireFix[fx]);
            b = bpMerge(prev, b, bpWireFix[fx + 1]);
            c = bpMerge(prev, c, bpWireFix[fx + 2]);
            break;
        case BP_WGEN:
            for (unsigned int k = fx; k < fixEnd; ++k)
            {
                const BpWireFix& f = bpWireFix[k];
                if (f.role == 0)
                {
                    a = bpMerge(prev, a, f);
                }
                else if (f.role == 1)
                {
                    b = bpMerge(prev, b, f);
                }
                else
                {
                    c = bpMerge(prev, c, f);
                }
            }
            break;
        default:
            break;
        }
        const __m512i code = bpCode(a, b, c, permIn, eyeRev);
        __m512i o = _mm512_permutexvar_epi8(code, _mm512_loadu_si512((const void*)bpTabInt[i]));
        if (shape >= BP_L1 && shape <= BP_LN)
        {
            const __m512i clean = _mm512_and_si512(code, _mm512_set1_epi8(0x3F));
            o = bpPatch(o, clean, bpLutFix[fx]);
            if (shape != BP_L1)
            {
                o = bpPatch(o, clean, bpLutFix[fx + 1]);
                for (unsigned int k = fx + 2; k < fixEnd; ++k)
                {
                    o = bpPatch(o, clean, bpLutFix[k]);
                }
            }
        }
        return o;
    }

    template<unsigned int SHAPE>
    __m512i bpShapeRow(const unsigned long long (*prev)[2], unsigned int i, __m512i permIn, __m512i eyeRev,
        __m512i lowSix) const
    {
        const unsigned long long r = bpRec[i];
        const unsigned int fx = bpRowFix[i] & 0xFFFF;
        __m512i code;
        if (SHAPE == BP_L1 || SHAPE == BP_L2)
        {
            code = bpCodeRows(prev, (unsigned int)(r & 0xFFFF), (unsigned int)((r >> 16) & 0xFFFF), (unsigned int)((r >> 32) & 0xFFFF), permIn, eyeRev);
        }
        else
        {
            __m512i a = bpRow(prev, (unsigned int)(r & 0xFFFF));
            __m512i b = bpRow(prev, (unsigned int)((r >> 16) & 0xFFFF));
            __m512i c = bpRow(prev, (unsigned int)((r >> 32) & 0xFFFF));
            if (SHAPE == BP_W1R0 || SHAPE == BP_WPAD)
            {
                a = bpMerge(prev, a, bpWireFix[fx]);
            }
            if (SHAPE == BP_W1R1)
            {
                b = bpMerge(prev, b, bpWireFix[fx]);
            }
            if (SHAPE == BP_WPAD)
            {
                b = bpMerge(prev, b, bpWireFix[fx + 1]);
            }
            if (SHAPE == BP_W1R2)
            {
                c = bpMerge(prev, c, bpWireFix[fx]);
            }
            if (SHAPE == BP_WPAD)
            {
                c = bpMerge(prev, c, bpWireFix[fx + 2]);
            }
            code = bpCode(a, b, c, permIn, eyeRev);
        }
        __m512i o = _mm512_permutexvar_epi8(code, _mm512_loadu_si512((const void*)bpTabInt[i]));
        if (SHAPE == BP_L1 || SHAPE == BP_L2)
        {
            const __m512i clean = _mm512_and_si512(code, lowSix);
            o = bpPatch(o, clean, bpLutFix[fx]);
            if (SHAPE == BP_L2)
            {
                o = bpPatch(o, clean, bpLutFix[fx + 1]);
            }
        }
        return o;
    }

    template<unsigned int SHAPE>
    void bpShapeGroup(const unsigned long long (*prev)[2], unsigned long long (*out)[2],
        unsigned int i0, __m512i permIn, __m512i permOut, __m512i eye, __m512i eyeRev, __m512i lowSix) const
    {
        const __m512i o0 = bpShapeRow<SHAPE>(prev, i0 + 0, permIn, eyeRev, lowSix);
        const __m512i o1 = bpShapeRow<SHAPE>(prev, i0 + 1, permIn, eyeRev, lowSix);
        const __m512i o2 = bpShapeRow<SHAPE>(prev, i0 + 2, permIn, eyeRev, lowSix);
        const __m512i o3 = bpShapeRow<SHAPE>(prev, i0 + 3, permIn, eyeRev, lowSix);
        bpStore4(out, i0, o0, o1, o2, o3, permOut, eye);
    }

    template<unsigned int SHAPE>
    void bpShapeRun(const unsigned long long (*prev)[2], unsigned long long (*out)[2],
        unsigned int gBegin, unsigned int gEnd, __m512i permIn, __m512i permOut, __m512i eye, __m512i eyeRev, __m512i lowSix) const
    {
        for (unsigned int g = gBegin; g < gEnd; ++g)
        {
            bpShapeGroup<SHAPE>(prev, out, g * 4, permIn, permOut, eye, eyeRev, lowSix);
        }
    }

    void resetLaneBookkeeping()
    {
        setMem(laneCount, sizeof(laneCount), 0);
        setMem(laneSum, sizeof(laneSum), 0);
        setMem(laneBits, sizeof(laneBits), 0);
    }

    // Ticks a 64-lane batch until every lane finishes, times out, or the batch is decided.
    void runTickLoop(unsigned int nLanes, unsigned long long f0)
    {
        unsigned int prevIdx = 0;
        unsigned int outIdx = 1;
        __mmask64 active = laneMaskOf(nLanes);
        resetLaneBookkeeping();
        slidingLanes = 0;
        bpFinishedLanes = 0;
        bpAcceptLanes = 0;
        const __m512i permIn = _mm512_loadu_si512((const void*)bpPermIn);
        const __m512i permOut = _mm512_loadu_si512((const void*)bpPermOut);
        const __m512i eye = _mm512_set1_epi64((long long)0x8040201008040201ULL);
        const __m512i eyeRev = _mm512_set1_epi64((long long)0x0102040810204080ULL);
        const __m512i lowSix = _mm512_set1_epi8(0x3F);
        const unsigned int cleanGroups = bpCleanGroups;
        const unsigned int numRuns = bpNumRuns;
        const unsigned int ic = intIdx[controlIndex];
        const unsigned int io = intIdx[outputIndex];
        unsigned long long ticks = 0;
        while (active)
        {
            ++ticks;
            if (ticks >= maxNumberOfTicks)
            {
                timeoutRemaining(active);
                break;
            }
            const unsigned long long (*prev)[2] = bpState[prevIdx];
            unsigned long long (*out)[2] = bpState[outIdx];
            for (unsigned int g = 0; g < cleanGroups; ++g)
            {
                bpCleanGroup(prev, out, bpRec, bpTabInt, g * 4, permIn, permOut, eye, eyeRev);
            }
            for (unsigned int k = 0; k < numRuns; ++k)
            {
                const unsigned int gb = bpRunBegin[k];
                const unsigned int ge = bpRunEnd[k];
                switch (bpRunShape[k])
                {
                case BP_W1R0: bpShapeRun<BP_W1R0>(prev, out, gb, ge, permIn, permOut, eye, eyeRev, lowSix); break;
                case BP_W1R1: bpShapeRun<BP_W1R1>(prev, out, gb, ge, permIn, permOut, eye, eyeRev, lowSix); break;
                case BP_W1R2: bpShapeRun<BP_W1R2>(prev, out, gb, ge, permIn, permOut, eye, eyeRev, lowSix); break;
                case BP_WPAD: bpShapeRun<BP_WPAD>(prev, out, gb, ge, permIn, permOut, eye, eyeRev, lowSix); break;
                case BP_L1: bpShapeRun<BP_L1>(prev, out, gb, ge, permIn, permOut, eye, eyeRev, lowSix); break;
                case BP_L2: bpShapeRun<BP_L2>(prev, out, gb, ge, permIn, permOut, eye, eyeRev, lowSix); break;
                default:
                    for (unsigned int g = gb; g < ge; ++g)
                    {
                        const unsigned int i0 = g * 4;
                        const __m512i o0 = bpDirtyRow(prev, i0 + 0, permIn, eyeRev);
                        const __m512i o1 = bpDirtyRow(prev, i0 + 1, permIn, eyeRev);
                        const __m512i o2 = bpDirtyRow(prev, i0 + 2, permIn, eyeRev);
                        const __m512i o3 = bpDirtyRow(prev, i0 + 3, permIn, eyeRev);
                        bpStore4(out, i0, o0, o1, o2, o3, permOut, eye);
                    }
                    break;
                }
            }
            processEmits(out[ic][1], out[io][0], out[io][1], active, f0);
            if (bpEarlyStop && batchDecided(active))
            {
                break;
            }
            const unsigned int tmp = prevIdx;
            prevIdx = outIdx;
            outIdx = tmp;
        }
    }

    // 32-lane batches: the same tick on ymm registers over bpState32.
    static inline __m256i bpRow32(const unsigned int (*prev)[2], unsigned int row)
    {
        return _mm256_broadcastq_epi64(_mm_loadl_epi64((const __m128i*)(const void*)prev[row]));
    }

    static inline __m256i bpCode32(__m256i a, __m256i b, __m256i c, __m256i permIn, __m256i eyeRev)
    {
        return _mm256_gf2p8affine_epi64_epi8(eyeRev, _mm256_permutex2var_epi8(_mm256_blend_epi32(a, b, 0x0C), permIn, c), 0);
    }

    static inline __m256i bpLook32(__m256i code, const unsigned char* table)
    {
        return _mm256_permutex2var_epi8(_mm256_loadu_si256((const __m256i*)(const void*)table), code,
            _mm256_loadu_si256((const __m256i*)(const void*)(table + 32)));
    }

    static inline void bpStore32(unsigned int (*out)[2], unsigned int i0, __m256i o0, __m256i o1, __m256i o2, __m256i o3,
        __m256i permOut, __m256i eye)
    {
        const __m256i packed = _mm256_or_si256(_mm256_ternarylogic_epi64(o0, o1, o2, 0xFE), o3);
        _mm256_storeu_si256((__m256i*)(void*)out[i0], _mm256_permutexvar_epi8(permOut, _mm256_gf2p8affine_epi64_epi8(eye, packed, 0)));
    }

    static inline __m256i bpMerge32(const unsigned int (*prev)[2], __m256i base, const BpWireFix& f)
    {
        return _mm256_ternarylogic_epi64(_mm256_set1_epi64x((long long)f.mask), bpRow32(prev, f.alt), base, 0xCA);
    }

    static inline __m256i bpPatch32(__m256i out, __m256i cleanCode, const BpLutFix& f)
    {
        const __mmask32 hit = _mm256_mask_cmpeq_epi8_mask((__mmask32)f.mask, cleanCode, _mm256_set1_epi8((char)f.code));
        return _mm256_mask_mov_epi8(out, hit, _mm256_set1_epi8((char)f.value));
    }

    __m256i bpCleanRow32(const unsigned int (*prev)[2], unsigned int i, __m256i permIn, __m256i eyeRev) const
    {
        const unsigned long long r = bpRec[i];
        const __m256i code = bpCode32(bpRow32(prev, (unsigned int)(r & 0xFFFF)), bpRow32(prev, (unsigned int)((r >> 16) & 0xFFFF)),
            bpRow32(prev, (unsigned int)((r >> 32) & 0xFFFF)), permIn, eyeRev);
        return bpLook32(code, bpTabInt[i]);
    }

    void bpCleanGroup32(const unsigned int (*prev)[2], unsigned int (*out)[2], unsigned int i0,
        __m256i permIn, __m256i permOut, __m256i eye, __m256i eyeRev) const
    {
        const __m256i o0 = bpCleanRow32(prev, i0 + 0, permIn, eyeRev);
        const __m256i o1 = bpCleanRow32(prev, i0 + 1, permIn, eyeRev);
        const __m256i o2 = bpCleanRow32(prev, i0 + 2, permIn, eyeRev);
        const __m256i o3 = bpCleanRow32(prev, i0 + 3, permIn, eyeRev);
        bpStore32(out, i0, o0, o1, o2, o3, permOut, eye);
    }

    template<unsigned int SHAPE>
    __m256i bpShapeRow32(const unsigned int (*prev)[2], unsigned int i, __m256i permIn, __m256i eyeRev,
        __m256i lowSix) const
    {
        const unsigned long long r = bpRec[i];
        __m256i a = bpRow32(prev, (unsigned int)(r & 0xFFFF));
        __m256i b = bpRow32(prev, (unsigned int)((r >> 16) & 0xFFFF));
        __m256i c = bpRow32(prev, (unsigned int)((r >> 32) & 0xFFFF));
        const unsigned int fx = bpRowFix[i] & 0xFFFF;
        if (SHAPE == BP_W1R0 || SHAPE == BP_WPAD)
        {
            a = bpMerge32(prev, a, bpWireFix[fx]);
        }
        if (SHAPE == BP_W1R1)
        {
            b = bpMerge32(prev, b, bpWireFix[fx]);
        }
        if (SHAPE == BP_WPAD)
        {
            b = bpMerge32(prev, b, bpWireFix[fx + 1]);
        }
        if (SHAPE == BP_W1R2)
        {
            c = bpMerge32(prev, c, bpWireFix[fx]);
        }
        if (SHAPE == BP_WPAD)
        {
            c = bpMerge32(prev, c, bpWireFix[fx + 2]);
        }
        const __m256i code = bpCode32(a, b, c, permIn, eyeRev);
        __m256i o = bpLook32(code, bpTabInt[i]);
        if (SHAPE == BP_L1 || SHAPE == BP_L2)
        {
            const __m256i clean = _mm256_and_si256(code, lowSix);
            o = bpPatch32(o, clean, bpLutFix[fx]);
            if (SHAPE == BP_L2)
            {
                o = bpPatch32(o, clean, bpLutFix[fx + 1]);
            }
        }
        return o;
    }

    template<unsigned int SHAPE>
    void bpShapeGroup32(const unsigned int (*prev)[2], unsigned int (*out)[2], unsigned int i0,
        __m256i permIn, __m256i permOut, __m256i eye, __m256i eyeRev, __m256i lowSix) const
    {
        const __m256i o0 = bpShapeRow32<SHAPE>(prev, i0 + 0, permIn, eyeRev, lowSix);
        const __m256i o1 = bpShapeRow32<SHAPE>(prev, i0 + 1, permIn, eyeRev, lowSix);
        const __m256i o2 = bpShapeRow32<SHAPE>(prev, i0 + 2, permIn, eyeRev, lowSix);
        const __m256i o3 = bpShapeRow32<SHAPE>(prev, i0 + 3, permIn, eyeRev, lowSix);
        bpStore32(out, i0, o0, o1, o2, o3, permOut, eye);
    }

    template<unsigned int SHAPE>
    void bpShapeRun32(const unsigned int (*prev)[2], unsigned int (*out)[2], unsigned int gBegin,
        unsigned int gEnd, __m256i permIn, __m256i permOut, __m256i eye, __m256i eyeRev, __m256i lowSix) const
    {
        for (unsigned int g = gBegin; g < gEnd; ++g)
        {
            bpShapeGroup32<SHAPE>(prev, out, g * 4, permIn, permOut, eye, eyeRev, lowSix);
        }
    }

    __m256i bpDirtyRow32(const unsigned int (*prev)[2], unsigned int i, __m256i permIn, __m256i eyeRev, __m256i lowSix) const
    {
        const unsigned long long r = bpRec[i];
        __m256i a = bpRow32(prev, (unsigned int)(r & 0xFFFF));
        __m256i b = bpRow32(prev, (unsigned int)((r >> 16) & 0xFFFF));
        __m256i c = bpRow32(prev, (unsigned int)((r >> 32) & 0xFFFF));
        const unsigned int shape = bpRowShape[i];
        const unsigned int fx = bpRowFix[i] & 0xFFFF;
        const unsigned int count = bpRowFix[i] >> 16;
        if (shape <= BP_WGEN)
        {
            for (unsigned int k = fx; k < fx + count; ++k)
            {
                const BpWireFix& f = bpWireFix[k];
                if (f.role == 0)
                {
                    a = bpMerge32(prev, a, f);
                }
                else if (f.role == 1)
                {
                    b = bpMerge32(prev, b, f);
                }
                else
                {
                    c = bpMerge32(prev, c, f);
                }
            }
        }
        const __m256i code = bpCode32(a, b, c, permIn, eyeRev);
        __m256i o = bpLook32(code, bpTabInt[i]);
        if (shape >= BP_L1 && shape <= BP_LN)
        {
            const __m256i clean = _mm256_and_si256(code, lowSix);
            for (unsigned int k = fx; k < fx + count; ++k)
            {
                o = bpPatch32(o, clean, bpLutFix[k]);
            }
        }
        return o;
    }

    void runTickLoop32(unsigned int nLanes, unsigned long long f0)
    {
        unsigned int prevIdx = 0;
        unsigned int outIdx = 1;
        __mmask64 active = laneMaskOf(nLanes);
        resetLaneBookkeeping();
        slidingLanes = 0;
        bpFinishedLanes = 0;
        bpAcceptLanes = 0;
        const __m256i permIn = _mm256_loadu_si256((const __m256i*)(const void*)bpPermIn32);
        const __m256i permOut = _mm256_loadu_si256((const __m256i*)(const void*)bpPermOut32);
        const __m256i eye = _mm256_set1_epi64x((long long)0x8040201008040201ULL);
        const __m256i eyeRev = _mm256_set1_epi64x((long long)0x0102040810204080ULL);
        const __m256i lowSix = _mm256_set1_epi8(0x3F);
        const unsigned int cleanGroups = bpCleanGroups;
        const unsigned int numRuns = bpNumRuns;
        const unsigned int ic = intIdx[controlIndex];
        const unsigned int io = intIdx[outputIndex];
        unsigned long long ticks = 0;
        while (active)
        {
            ++ticks;
            if (ticks >= maxNumberOfTicks)
            {
                timeoutRemaining(active);
                break;
            }
            const unsigned int (*prev)[2] = bpState32[prevIdx];
            unsigned int (*out)[2] = bpState32[outIdx];
            for (unsigned int g = 0; g < cleanGroups; ++g)
            {
                bpCleanGroup32(prev, out, g * 4, permIn, permOut, eye, eyeRev);
            }
            for (unsigned int k = 0; k < numRuns; ++k)
            {
                const unsigned int gb = bpRunBegin[k];
                const unsigned int ge = bpRunEnd[k];
                switch (bpRunShape[k])
                {
                case BP_W1R0: bpShapeRun32<BP_W1R0>(prev, out, gb, ge, permIn, permOut, eye, eyeRev, lowSix); break;
                case BP_W1R1: bpShapeRun32<BP_W1R1>(prev, out, gb, ge, permIn, permOut, eye, eyeRev, lowSix); break;
                case BP_W1R2: bpShapeRun32<BP_W1R2>(prev, out, gb, ge, permIn, permOut, eye, eyeRev, lowSix); break;
                case BP_WPAD: bpShapeRun32<BP_WPAD>(prev, out, gb, ge, permIn, permOut, eye, eyeRev, lowSix); break;
                case BP_L1: bpShapeRun32<BP_L1>(prev, out, gb, ge, permIn, permOut, eye, eyeRev, lowSix); break;
                case BP_L2: bpShapeRun32<BP_L2>(prev, out, gb, ge, permIn, permOut, eye, eyeRev, lowSix); break;
                default:
                    for (unsigned int g = gb; g < ge; ++g)
                    {
                        const unsigned int i0 = g * 4;
                        bpStore32(out, i0, bpDirtyRow32(prev, i0 + 0, permIn, eyeRev, lowSix), bpDirtyRow32(prev, i0 + 1, permIn, eyeRev, lowSix),
                            bpDirtyRow32(prev, i0 + 2, permIn, eyeRev, lowSix), bpDirtyRow32(prev, i0 + 3, permIn, eyeRev, lowSix), permOut, eye);
                    }
                    break;
                }
            }
            processEmits(out[ic][1], out[io][0], out[io][1], active, f0);
            if (bpEarlyStop && batchDecided(active))
            {
                break;
            }
            const unsigned int tmp = prevIdx;
            prevIdx = outIdx;
            outIdx = tmp;
        }
    }

    // First lane, in step order, that the walk's accept test takes.
    bool scanForAccept(unsigned int firstLane, unsigned int nLanes, const Rating& cur, unsigned long long K, unsigned int& acceptedLane, Rating& acceptedRating) const
    {
        for (unsigned int j = firstLane; j < nLanes; ++j)
        {
            const Rating r{ laneErr[j], laneShift[j] };
            const bool ok = (laneStepOf[j] < K) ? (r.isValid() && r.errorWorseOrEqual(cur)) : (r.isValid() && r.isNotWorseThan(cur));
            if (ok)
            {
                acceptedLane = j;
                acceptedRating = r;
                return true;
            }
        }
        return false;
    }

    // Walk driver. Lane 0 of the first batch rates the unmutated network; best is recorded on every exploit
    // step that ties or beats it, folded neutral steps included. After speculationBatchCap batches the rest
    // of the walk goes one step at a time.
    Rating computeScoreFromCurrentSimd(unsigned int L, unsigned long long K, unsigned char mode)
    {
        buildBpConstants();
        for (unsigned long long t = 0; t < sequenceLength; ++t)
        {
            targetDw[t] = targetOutputs[t];
        }
        const unsigned long long entryShift = shift;
        snapshotBest();
        Rating cur = Rating::worst();
        Rating best = Rating::worst();
        bool baseInBatch = true;

        unsigned long long s = 0;
        unsigned long long batchesUsed = 0;
        bool wireTablesNeedRebuild = true;
        bool lutTablesNeedRebuild = true;
        unsigned int nextWidth = 32;

        while (s < numberOfMutations)
        {
            ++batchesUsed;
            if (batchesUsed > speculationBatchCap)
            {
                if (baseInBatch)
                {
                    cur = advanceShift();
                    best = cur;
                }
                return rollingStepsFrom(s, L, K, mode, cur, best);
            }

            if (wireTablesNeedRebuild)
            {
                buildSimdWireBase();
                wireTablesNeedRebuild = false;
            }
            if (lutTablesNeedRebuild)
            {
                buildSimdLutBase();
                lutTablesNeedRebuild = false;
            }

            const unsigned int firstLane = baseInBatch ? 1u : 0u;
            const bool curValid = baseInBatch || cur.isValid();
            const bool tieAtEntry = baseInBatch || (cur.isValid() && cur.isNotWorseThan(best));
            const unsigned long long batchStart = s;
            const unsigned long long exploitStart = (batchStart > K) ? batchStart : K;

            unsigned long long stepCursor = s;
            unsigned int nLanes;
            bpWidth = nextWidth;
            if (mode == BPP9000_MODE_START)
            {
                prepareStartBatch();
                nLanes = collectStartDeviations(stepCursor, L, curValid, firstLane);
            }
            else if (mode == BPP9000_MODE_WIRING)
            {
                nLanes = collectWiringDeviations(stepCursor, L, curValid, firstLane);
            }
            else
            {
                nLanes = collectLutDeviations(stepCursor, L, curValid, firstLane);
            }
            if (nLanes == 0)
            {
                if (tieAtEntry && stepCursor > exploitStart)
                {
                    best = cur;
                    snapshotBest();
                }
                s = stepCursor;
                continue;
            }

            bpEarlyStop = !baseInBatch;
            bpCur = cur;
            bpK = K;
            bpDoomBound = (cur.error > advanceThreshold) ? cur.error : advanceThreshold;
            bpLaneRange = laneMaskOf(nLanes) & ~((1ULL << firstLane) - 1ULL);
            bpExploitLanes = 0;
            for (unsigned int j = firstLane; j < nLanes; ++j)
            {
                if (laneStepOf[j] >= K)
                {
                    bpExploitLanes |= 1ULL << j;
                }
            }
            if (bpWidth == 32)
            {
                runTickLoop32(nLanes, shift);
            }
            else
            {
                runTickLoop(nLanes, shift);
            }

            if (baseInBatch)
            {
                baseInBatch = false;
                const Rating base{ laneErr[0], laneShift[0] };
                if (!base.isValid() || base.shift != entryShift)
                {
                    if (mode == BPP9000_MODE_START)
                    {
                        undoStartNeutralsBack(0);
                    }
                    else if (mode == BPP9000_MODE_WIRING)
                    {
                        undoWiringNeutralsBack(0);
                    }
                    else
                    {
                        undoLutNeutralsBack(0);
                    }
                    shift = entryShift;
                    cur = advanceShift();
                    best = cur;
                    continue;
                }
                cur = base;
                best = cur;
            }

            unsigned int acceptedLane = 0;
            Rating acceptedRating = Rating::worst();
            if (scanForAccept(firstLane, nLanes, cur, K, acceptedLane, acceptedRating))
            {
                const unsigned int marker = laneUndoMarker[acceptedLane];
                if (mode == BPP9000_MODE_START)
                {
                    undoStartNeutralsBack(marker);
                }
                else if (mode == BPP9000_MODE_WIRING)
                {
                    undoWiringNeutralsBack(marker);
                }
                else
                {
                    undoLutNeutralsBack(marker);
                }

                const unsigned long long winStep = laneStepOf[acceptedLane];
                nextWidth = (acceptedLane - firstLane < 32) ? 32u : 64u;

                if (tieAtEntry && winStep > exploitStart)
                {
                    best = cur;
                    snapshotBest();
                }

                for (unsigned int i = 0; i < L; ++i)
                {
                    mutate(mode, mutationSeed[winStep * BPP9000_MAX_CHANGES_PER_STEP + i]);
                }
                cur = acceptedRating;
                shift = cur.shift;

                if (winStep >= K && cur.isValid() && cur.isNotWorseThan(best))
                {
                    best = cur;
                    snapshotBest();
                }

                if (mode == BPP9000_MODE_WIRING)
                {
                    wireTablesNeedRebuild = true;
                }
                else if (mode == BPP9000_MODE_LUT)
                {
                    lutTablesNeedRebuild = true;
                }
                s = winStep + 1;
            }
            else
            {
                nextWidth = 64;

                if (tieAtEntry && stepCursor > exploitStart)
                {
                    best = cur;
                    snapshotBest();
                }
                s = stepCursor;
            }
        }

        if (baseInBatch)
        {
            cur = advanceShift();
            best = cur;
        }
        shift = best.shift;
        return best;
    }

    // Canonical L and mode take the mutation-parallel walk; anything else the neuron-parallel one.
    Rating computeScoreFromCurrent(unsigned int L, unsigned long long K, unsigned char mode)
    {
        activeMode = mode;
        buildGatherControls();
        buildTableArea();
        if (L >= 1 && L <= BPP9000_MAX_CHANGES_PER_STEP && mode >= BPP9000_MODE_START && mode <= BPP9000_MODE_LUT)
        {
            return computeScoreFromCurrentSimd(L, K, mode);
        }
        Rating cur = advanceShift();
        Rating best = cur;
        snapshotBest();
        return rollingStepsFrom(0, L, K, mode, cur, best);
    }
#else
    // AVX2: one step at a time; score() replays from tick 0 for every frame, like the reference.
    static_assert(numberOfNeighbors == 3 && lutSize == 27, "the AVX2 ticks assume 3 neighbors");
    static_assert(populationThreshold % 16 == 0, "the gather tick walks 16 neurons per iteration");

    static constexpr bool useBitSlicedTick = (populationThreshold % 256 == 0) && (populationThreshold <= 2048);
    static constexpr unsigned long long avxGroups = useBitSlicedTick ? populationThreshold / 32 : 2;
    static constexpr unsigned long long avxChunks = useBitSlicedTick ? populationThreshold / 128 : 1;
    static constexpr unsigned long long avxWords = useBitSlicedTick ? populationThreshold / 64 : 4;
    static constexpr unsigned long long gatherNeurons = useBitSlicedTick ? 16 : maxNumberOfNeurons;

    // Bit-sliced state: bit n of avxState[0] / avxState[1] is set when neuron n's trit is 1 / 2.
    unsigned char avxCtrl[numberOfNeighbors][avxGroups][avxChunks][32];
    unsigned char avxBit[numberOfNeighbors][avxGroups][32];
    unsigned long long avxLut[lutSize][2][avxWords];
    unsigned long long avxIn[numberOfNeighbors][2][avxWords];
    unsigned long long avxState[2][avxWords];

    // Gather tick: sources one array per slot, LUT 2 bits per entry per neuron.
    unsigned int slotIndex[numberOfNeighbors][gatherNeurons];
    unsigned long long packedLut[gatherNeurons];

    // vpshufb controls per slot and 32-neuron group, and the LUT as 27 x 2 bit-planes.
    void buildBitSlicedCaches()
    {
        setMem(avxCtrl, sizeof(avxCtrl), 0x80);
        for (unsigned long long j = 0; j < numberOfNeighbors; ++j)
        {
            for (unsigned long long n = 0; n < populationThreshold; ++n)
            {
                const unsigned int s = neighborIndices[n * numberOfNeighbors + j];
                avxCtrl[j][n >> 5][s >> 7][n & 31] = (unsigned char)((s >> 3) & 15);
                avxBit[j][n >> 5][n & 31] = (unsigned char)(1u << (s & 7));
            }
        }
        setMem(avxLut, sizeof(avxLut), 0);
        for (unsigned long long n = 0; n < populationThreshold; ++n)
        {
            const unsigned char* row = curLut + n * lutSize;
            const unsigned long long bit = 1ULL << (n & 63);
            for (unsigned long long e = 0; e < lutSize; ++e)
            {
                if (row[e] == 1)
                {
                    avxLut[e][0][n >> 6] |= bit;
                }
                else if (row[e] == 2)
                {
                    avxLut[e][1][n >> 6] |= bit;
                }
            }
        }
    }

    void loadStatePlanes(const unsigned char* trits)
    {
        setMem(avxState, sizeof(avxState), 0);
        for (unsigned long long n = 0; n < populationThreshold; ++n)
        {
            if (trits[n] == 1)
            {
                avxState[0][n >> 6] |= 1ULL << (n & 63);
            }
            else if (trits[n] == 2)
            {
                avxState[1][n >> 6] |= 1ULL << (n & 63);
            }
        }
    }

    unsigned int stateTrit(unsigned int n) const
    {
        return (unsigned int)((avxState[0][n >> 6] >> (n & 63)) & 1) | ((unsigned int)((avxState[1][n >> 6] >> (n & 63)) & 1) << 1);
    }

    static inline __m256i mux3(__m256i x, __m256i y, __m256i z, __m256i lo, __m256i hi, __m256i known)
    {
        return _mm256_or_si256(_mm256_andnot_si256(known, x),
            _mm256_or_si256(_mm256_and_si256(lo, y), _mm256_and_si256(hi, z)));
    }

    // Pass 1 fetches every neuron's three source trits; pass 2 runs the LUT mux tree, 256 neurons per vector.
    void tickBitSliced()
    {
        const unsigned char* plane0 = (const unsigned char*)(const void*)avxState[0];
        const unsigned char* plane1 = (const unsigned char*)(const void*)avxState[1];
        for (unsigned long long j = 0; j < numberOfNeighbors; ++j)
        {
            unsigned int* out0 = (unsigned int*)(void*)avxIn[j][0];
            unsigned int* out1 = (unsigned int*)(void*)avxIn[j][1];
            for (unsigned long long g = 0; g < avxGroups; g += 2)
            {
                __m256i accA0 = _mm256_setzero_si256();
                __m256i accA1 = _mm256_setzero_si256();
                __m256i accB0 = _mm256_setzero_si256();
                __m256i accB1 = _mm256_setzero_si256();
                for (unsigned long long c = 0; c < avxChunks; ++c)
                {
                    const __m256i t0 = _mm256_broadcastsi128_si256(_mm_loadu_si128((const __m128i*)(const void*)(plane0 + 16 * c)));
                    const __m256i t1 = _mm256_broadcastsi128_si256(_mm_loadu_si128((const __m128i*)(const void*)(plane1 + 16 * c)));
                    const __m256i ca = _mm256_loadu_si256((const __m256i*)(const void*)avxCtrl[j][g][c]);
                    const __m256i cb = _mm256_loadu_si256((const __m256i*)(const void*)avxCtrl[j][g + 1][c]);
                    accA0 = _mm256_or_si256(accA0, _mm256_shuffle_epi8(t0, ca));
                    accA1 = _mm256_or_si256(accA1, _mm256_shuffle_epi8(t1, ca));
                    accB0 = _mm256_or_si256(accB0, _mm256_shuffle_epi8(t0, cb));
                    accB1 = _mm256_or_si256(accB1, _mm256_shuffle_epi8(t1, cb));
                }
                const __m256i ba = _mm256_loadu_si256((const __m256i*)(const void*)avxBit[j][g]);
                const __m256i bb = _mm256_loadu_si256((const __m256i*)(const void*)avxBit[j][g + 1]);
                out0[g] = (unsigned int)_mm256_movemask_epi8(_mm256_cmpeq_epi8(_mm256_and_si256(accA0, ba), ba));
                out1[g] = (unsigned int)_mm256_movemask_epi8(_mm256_cmpeq_epi8(_mm256_and_si256(accA1, ba), ba));
                out0[g + 1] = (unsigned int)_mm256_movemask_epi8(_mm256_cmpeq_epi8(_mm256_and_si256(accB0, bb), bb));
                out1[g + 1] = (unsigned int)_mm256_movemask_epi8(_mm256_cmpeq_epi8(_mm256_and_si256(accB1, bb), bb));
            }
        }
        for (unsigned long long w = 0; w < avxWords; w += 4)
        {
            const __m256i a0 = _mm256_loadu_si256((const __m256i*)(const void*)(avxIn[0][0] + w));
            const __m256i a1 = _mm256_loadu_si256((const __m256i*)(const void*)(avxIn[0][1] + w));
            const __m256i b0 = _mm256_loadu_si256((const __m256i*)(const void*)(avxIn[1][0] + w));
            const __m256i b1 = _mm256_loadu_si256((const __m256i*)(const void*)(avxIn[1][1] + w));
            const __m256i c0 = _mm256_loadu_si256((const __m256i*)(const void*)(avxIn[2][0] + w));
            const __m256i c1 = _mm256_loadu_si256((const __m256i*)(const void*)(avxIn[2][1] + w));
            const __m256i out0 = lutTree(avxLut[0][0] + w, a0, a1, b0, b1, c0, c1);
            const __m256i out1 = lutTree(avxLut[0][1] + w, a0, a1, b0, b1, c0, c1);
            _mm256_storeu_si256((__m256i*)(void*)(avxState[0] + w), out0);
            _mm256_storeu_si256((__m256i*)(void*)(avxState[1] + w), out1);
        }
    }

    static inline __m256i lutPlane(const unsigned long long* lut, unsigned int e)
    {
        return _mm256_loadu_si256((const __m256i*)(const void*)(lut + (unsigned long long)e * 2 * avxWords));
    }

    // The 27-entry ternary mux tree for one output plane, written out so it stays in registers.
    static inline __m256i lutTree(const unsigned long long* lut,
        __m256i a0, __m256i a1, __m256i b0, __m256i b1, __m256i c0, __m256i c1)
    {
        const __m256i ak = _mm256_or_si256(a0, a1);
        const __m256i bk = _mm256_or_si256(b0, b1);
        const __m256i n0 = mux3(
            mux3(lutPlane(lut, 0), lutPlane(lut, 1), lutPlane(lut, 2), a0, a1, ak),
            mux3(lutPlane(lut, 3), lutPlane(lut, 4), lutPlane(lut, 5), a0, a1, ak),
            mux3(lutPlane(lut, 6), lutPlane(lut, 7), lutPlane(lut, 8), a0, a1, ak), b0, b1, bk);
        const __m256i n1 = mux3(
            mux3(lutPlane(lut, 9), lutPlane(lut, 10), lutPlane(lut, 11), a0, a1, ak),
            mux3(lutPlane(lut, 12), lutPlane(lut, 13), lutPlane(lut, 14), a0, a1, ak),
            mux3(lutPlane(lut, 15), lutPlane(lut, 16), lutPlane(lut, 17), a0, a1, ak), b0, b1, bk);
        const __m256i n2 = mux3(
            mux3(lutPlane(lut, 18), lutPlane(lut, 19), lutPlane(lut, 20), a0, a1, ak),
            mux3(lutPlane(lut, 21), lutPlane(lut, 22), lutPlane(lut, 23), a0, a1, ak),
            mux3(lutPlane(lut, 24), lutPlane(lut, 25), lutPlane(lut, 26), a0, a1, ak), b0, b1, bk);
        return mux3(n0, n1, n2, c0, c1, _mm256_or_si256(c0, c1));
    }

    void buildGatherCaches()
    {
        for (unsigned long long n = 0; n < populationThreshold; ++n)
        {
            for (unsigned long long j = 0; j < numberOfNeighbors; ++j)
            {
                slotIndex[j][n] = neighborIndices[n * numberOfNeighbors + j];
            }
            unsigned long long packed = 0;
            for (unsigned long long e = 0; e < lutSize; ++e)
            {
                packed |= (unsigned long long)(curLut[n * lutSize + e] & 3) << (2 * e);
            }
            packedLut[n] = packed;
        }
        setMem(neuronPrev + maxNumberOfNeurons, 4, 0);
    }

    // Eight source trits, one per 32-bit lane.
    static __m256i gatherSlot(const unsigned char* prev, const unsigned int* idx)
    {
        const __m256i raw = _mm256_i32gather_epi32((const int*)(const void*)prev,
            _mm256_loadu_si256((const __m256i*)(const void*)idx), 1);
        return _mm256_and_si256(raw, _mm256_set1_epi32(0xFF));
    }

    // 2 * (t0 + 3 * t1 + 9 * t2): the entry's bit offset inside the neuron's packed word.
    static __m256i lutBitOffset(__m256i t0, __m256i t1, __m256i t2)
    {
        const __m256i a = _mm256_add_epi32(t1, _mm256_slli_epi32(t1, 1));
        const __m256i b = _mm256_add_epi32(t2, _mm256_slli_epi32(t2, 3));
        return _mm256_slli_epi32(_mm256_add_epi32(t0, _mm256_add_epi32(a, b)), 1);
    }

    // Eight trits, one in the low byte of each 32-bit lane.
    __m256i readPackedLut(unsigned long long n, __m256i offset) const
    {
        const __m256i entryMask = _mm256_set1_epi64x(3);
        const __m256i wordsLo = _mm256_loadu_si256((const __m256i*)(const void*)(packedLut + n));
        const __m256i wordsHi = _mm256_loadu_si256((const __m256i*)(const void*)(packedLut + n + 4));
        const __m256i lo = _mm256_and_si256(_mm256_srlv_epi64(wordsLo,
            _mm256_cvtepu32_epi64(_mm256_castsi256_si128(offset))), entryMask);
        const __m256i hi = _mm256_and_si256(_mm256_srlv_epi64(wordsHi,
            _mm256_cvtepu32_epi64(_mm256_extracti128_si256(offset, 1))), entryMask);
        return _mm256_permutevar8x32_epi32(
            _mm256_blend_epi32(lo, _mm256_slli_epi64(hi, 32), 0xAA),
            _mm256_setr_epi32(0, 2, 4, 6, 1, 3, 5, 7));
    }

    // The low byte of each 32-bit lane, written as eight bytes.
    static void storeEight(unsigned char* out, __m256i v)
    {
        const __m256i pick = _mm256_setr_epi8(
            0, 4, 8, 12, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            0, 4, 8, 12, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
        const __m256i packed = _mm256_shuffle_epi8(v, pick);
        _mm_storel_epi64((__m128i*)(void*)out, _mm_unpacklo_epi32(
            _mm256_castsi256_si128(packed), _mm256_extracti128_si256(packed, 1)));
    }

    // One tick over every neuron, 16 at a time.
    void tickGather()
    {
        copyMem(neuronPrev, neuronOut, maxNumberOfNeurons);
        for (unsigned long long n = 0; n < populationThreshold; n += 16)
        {
            const __m256i a0 = gatherSlot(neuronPrev, slotIndex[0] + n);
            const __m256i a1 = gatherSlot(neuronPrev, slotIndex[1] + n);
            const __m256i a2 = gatherSlot(neuronPrev, slotIndex[2] + n);
            const __m256i b0 = gatherSlot(neuronPrev, slotIndex[0] + n + 8);
            const __m256i b1 = gatherSlot(neuronPrev, slotIndex[1] + n + 8);
            const __m256i b2 = gatherSlot(neuronPrev, slotIndex[2] + n + 8);
            storeEight(neuronOut + n, readPackedLut(n, lutBitOffset(a0, a1, a2)));
            storeEight(neuronOut + n + 8, readPackedLut(n + 8, lutBitOffset(b0, b1, b2)));
        }
    }

    void startTicks()
    {
        if constexpr (useBitSlicedTick)
        {
            buildBitSlicedCaches();
            loadStatePlanes(curInitial);
        }
        else
        {
            buildGatherCaches();
            copyMem(neuronOut, curInitial, maxNumberOfNeurons);
        }
    }

    void tick()
    {
        if constexpr (useBitSlicedTick)
        {
            tickBitSliced();
        }
        else
        {
            tickGather();
        }
    }

    unsigned int tritOf(unsigned int n) const
    {
        if constexpr (useBitSlicedTick)
        {
            return stateTrit(n);
        }
        else
        {
            return neuronOut[n];
        }
    }

    // Emits shift + windowWidth outputs and grades [shift, shift + windowWidth). Times out at maxNumberOfTicks.
    unsigned int score()
    {
        startTicks();

        unsigned int failures = 0;
        unsigned long long counter = 0;
        unsigned long long ticks = 0;
        while (counter < shift + windowWidth)
        {
            ++ticks;
            if (ticks >= maxNumberOfTicks)
            {
                return INFINITE_ERROR;
            }

            tick();

            if (tritOf(controlIndex) != TRIT_UNKNOWN)
            {
                if (counter >= shift && tritOf(outputIndex) != targetOutputs[counter])
                {
                    failures++;
                }
                counter++;
            }
        }
        return failures;
    }

    // Slides the frame while it is mastered, one score() per frame; stops at shiftCap.
    Rating advanceShift()
    {
        for (;;)
        {
            const unsigned int frameError = score();
            if (frameError > advanceThreshold)
            {
                return Rating{ frameError, (unsigned int)shift };
            }
            if (shift == shiftCap)
            {
                return Rating{ frameError, (unsigned int)shift };
            }
            shift++;
        }
    }

    Rating computeScoreFromCurrent(unsigned int L, unsigned long long K, unsigned char mode)
    {
        activeMode = mode;
        Rating cur = advanceShift();
        Rating best = cur;
        snapshotBest();
        return rollingStepsFrom(0, L, K, mode, cur, best);
    }
#endif

    // Standalone: root from the pubkey, walk with K = 0. Control/output must be set; the caller checks the nonce.
    Rating computeScore(
        const unsigned char* publicKey,
        const unsigned char* nonce,
        const unsigned char* pRandom2Pool)
    {
        const unsigned int L = changesPerStep(nonce);
        const unsigned char mode = modeOf(nonce);

        deriveRootMaterial(publicKey, pRandom2Pool);
        deriveMutationSeeds(publicKey, nonce, nullptr, pRandom2Pool);
        applyRootMaterial();

        shift = 0;
        return computeScoreFromCurrent(L, 0, mode);
    }

    void deriveRootANN(const unsigned char* rootSeed, const unsigned char* pRandom2Pool, ANN& out)
    {
        deriveRootMaterial(rootSeed, pRandom2Pool);
        applyRootMaterial();
        compact(out);
    }

    // Ant colony: walk the child from its parent's network and shift, with the child's own seeds.
    Rating computeScoreFromParent(
        const ANN& parentANN,
        unsigned long long parentShift,
        const unsigned char* publicKey,
        const unsigned char* nonce,
        const unsigned char* anchorTickDigest,
        const unsigned char* pRandom2Pool)
    {
        if (!isCanonicalAntNonce(nonce) || parentShift > shiftCap)
        {
            return Rating::worst();
        }

        expand(parentANN);
        if (!validateTopology())
        {
            return Rating::worst();
        }
        deriveMutationSeeds(publicKey, nonce, anchorTickDigest, pRandom2Pool);

        const unsigned int L = changesPerStep(nonce);
        const unsigned long long K = nonce[2];
        const unsigned char mode = modeOf(nonce);

        shift = parentShift;
        return computeScoreFromCurrent(L, K, mode);
    }

    int getLastOutput(unsigned char* requestedOutput, int requestedSizeInBytes)
    {
        return 0;
    }
};

}
