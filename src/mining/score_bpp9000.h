#pragma once

// Neuron-parallel bit-sliced scorer for the bpp9000 rolling-frame mining algorithm: one candidate
// network at a time, all populationThreshold neurons ticked together in SIMD
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

    // advanceThreshold: shift advances when the frame error drops to <= 1/3 of the frame.
    static constexpr unsigned int advanceThreshold = (unsigned int)(windowWidth / 3);

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
    // 4 spare bytes: the AVX2 tick gathers 4 bytes per lane.
    unsigned char neuronPrev[maxNumberOfNeurons + 4];

    RootMaterial rootMaterial;
    unsigned long long mutationSeed[mutationSeedPaddedCount];

    // Mode of the walk in progress; the AVX-512 advanceShift() reads it to pick which derived cache
    // (gather controls for WIRING, table area for LUT) to refresh. Unused in the scalar build.
    unsigned char activeMode = 0;

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
    // Returns the best-ever rating, leaving that network in best*.
    // Shared by both builds; only advanceShift() differs (bit-sliced vs scalar).
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
                // Anti-attractor: worse-or-equal error only; records nothing.
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
    // Bit-sliced tick: all neurons advance together in 64-neuron blocks, one lane per neuron. State is
    // two bit-planes (trit t = b0 | b1<<1, 2 = UNKNOWN). Per block and slot j, built from the wiring:
    //   gIdx[b][j][64] = source byte index & 127, its position within its 128-byte window
    //   gWin[b][j][w]  = mask of the lanes whose source byte falls in window w
    //   gCtl[b][j][64] = (lane&7)*8 + (source&7), the multishift control for the wanted bit

    static constexpr unsigned long long B = populationThreshold / 64;        // 64-neuron blocks
    static constexpr unsigned long long PBYTES = populationThreshold / 8;    // bytes per bit-plane
    static constexpr unsigned long long PHALVES = PBYTES / 128;              // 128-byte windows/plane
    static_assert(populationThreshold % 1024 == 0,
        "the bit-sliced tick assumes populationThreshold/8 is a whole number of 128-byte gather "
        "windows (populationThreshold a multiple of 1024) -- true at both gated populations, 1024 and 2048");
    static_assert(PHALVES >= 1 && PHALVES <= 8, "gWin's window-index field assumes a small window count");

    alignas(64) unsigned char plane0[PBYTES];
    alignas(64) unsigned char plane1[PBYTES];
    alignas(64) unsigned char planeOut0[PBYTES];
    alignas(64) unsigned char planeOut1[PBYTES];

    // Gather controls, rebuilt from neighborIndices -- see advanceShift() below for when.
    alignas(64) unsigned char gIdx[B][numberOfNeighbors][64];
    unsigned long long gWin[B][numberOfNeighbors][PHALVES];
    alignas(64) unsigned char gCtl[B][numberOfNeighbors][64];

    // Table area, rebuilt from curLut: 64 neurons x 32-byte LUT slots per block, first 27 bytes used.
    alignas(64) unsigned char tbl[B][2048];

    // Hoisted constants, built once in initMemory().
    unsigned long long tmask[16];         // tmask[r]: the 64-lane mask selecting rows [4r, 4r+4)
    alignas(64) unsigned char laneBase[64];   // (lane % 4) * 32: this lane's own 32-byte slot bias
    alignas(64) unsigned char sh3[64];        // shuffle constant: 3 * (i % 16)
    alignas(64) unsigned char sh9[64];        // shuffle constant: 9 * (i % 16)
    alignas(64) unsigned char onesConst[64];

    // Per-emit rolling-frame bookkeeping; one candidate, so one running sum and ring buffer suffice.
    alignas(64) unsigned char ring[windowWidth];

    void buildHoistedConstants()
    {
        for (unsigned int r = 0; r < 16; ++r)
        {
            tmask[r] = 0;
            for (unsigned int l = 0; l < 64; ++l)
            {
                if (l / 4 == r) { tmask[r] |= 1ull << l; }
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

    // Rebuilds the gather controls from the current neighborIndices.
    void buildGatherControls()
    {
        for (unsigned long long b = 0; b < B; ++b)
        {
            for (unsigned long long j = 0; j < numberOfNeighbors; ++j)
            {
                for (unsigned long long w = 0; w < PHALVES; ++w)
                {
                    gWin[b][j][w] = 0;
                }
                for (unsigned long long l = 0; l < 64; ++l)
                {
                    const unsigned int s = neighborIndices[(b * 64 + l) * numberOfNeighbors + j];
                    const unsigned int byteIdx = s >> 3, win = byteIdx / 128;
                    gIdx[b][j][l] = (unsigned char)(byteIdx & 127);
                    gWin[b][j][win] |= 1ull << l;
                    gCtl[b][j][l] = (unsigned char)((l & 7) * 8 + (s & 7));
                }
            }
        }
    }

    // Rebuilds the table area from the current curLut.
    void buildTableArea()
    {
        for (unsigned long long b = 0; b < B; ++b)
        {
            unsigned char* t = tbl[b];
            for (unsigned long long l = 0; l < 64; ++l)
            {
                copyMem(t + l * 32, curLut + (b * 64 + l) * lutSize, lutSize);
            }
        }
    }

    void packPlanesFromCurInitial()
    {
        setMem(plane0, PBYTES, 0);
        setMem(plane1, PBYTES, 0);
        for (unsigned long long n = 0; n < populationThreshold; ++n)
        {
            if (curInitial[n] & 1) { plane0[n >> 3] |= (unsigned char)(1u << (n & 7)); }
            if (curInitial[n] & 2) { plane1[n >> 3] |= (unsigned char)(1u << (n & 7)); }
        }
    }

    unsigned int getTrit(unsigned int n) const
    {
        const unsigned int b0 = (plane0[n >> 3] >> (n & 7)) & 1u;
        const unsigned int b1 = (plane1[n >> 3] >> (n & 7)) & 1u;
        return b0 | (b1 << 1);
    }

    // One gathered trit vector for block b, slot j. m1 is the all-ones-byte constant.
    static inline __m512i gatherTrits(const unsigned char* pl0, const unsigned char* pl1,
        const unsigned char* idxBlockSlot, const unsigned long long* winBlockSlot, const unsigned char* ctlBlockSlot,
        __m512i m1)
    {
        const __m512i idx = _mm512_loadu_si512((const void*)idxBlockSlot);
        const __m512i ctl = _mm512_loadu_si512((const void*)ctlBlockSlot);
        __m512i g0, g1;
        {
            __m512i acc = idx;
            for (unsigned long long w = 0; w < PHALVES; ++w)
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
            for (unsigned long long w = 0; w < PHALVES; ++w)
            {
                acc = _mm512_mask2_permutex2var_epi8(
                    _mm512_loadu_si512((const void*)(pl1 + w * 128)), acc,
                    (__mmask64)winBlockSlot[w],
                    _mm512_loadu_si512((const void*)(pl1 + w * 128 + 64)));
            }
            g1 = _mm512_and_si512(_mm512_multishift_epi64_epi8(ctl, acc), m1);
        }
        return _mm512_or_si512(g0, _mm512_add_epi8(g1, g1));   // t = b0 | (b1 << 1)
    }

    // Advances plane0/plane1 by one tick: gather the three neighbour trits per block, combine into a
    // base-3 LUT index biased by laneBase, look it up over the block's table area, then extract the
    // result into two bit-planes.
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
            *(unsigned long long*)(void*)(planeOut0 + b * 8) = (unsigned long long)_mm512_test_epi8_mask(idx, m1);
            *(unsigned long long*)(void*)(planeOut1 + b * 8) = (unsigned long long)_mm512_test_epi8_mask(idx, _mm512_add_epi8(m1, m1));
        }
        copyMem(plane0, planeOut0, PBYTES);
        copyMem(plane1, planeOut1, PBYTES);
    }

    // Grades the one frame at the current shift, leaving shift alone. Mirrors the #else score().
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
            if (++ticks >= maxNumberOfTicks)
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

    // Scores from the current shift and advances while each frame is mastered, as one continuous
    // replay instead of one scalar call per frame. score() replays from tick 0 whatever the shift, so
    // the per-emit "wrong" bit sequence is fixed for a candidate and advancing the shift is a sliding
    // sum over consecutive width-windowWidth windows; the ring buffer and running sum reproduce that
    // in one pass, leaving the tick sequence and the `ticks` timeout unchanged.
    // The caches are refreshed for the active mode every call: mutate() has already changed the real
    // neighborIndices/curLut, so a rejected candidate would otherwise score against a stale cache.
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
            if (++ticks >= maxNumberOfTicks)
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
                        // Frame mastered and shiftCap not reached: the ring/sum already hold the next window.
                    }
                }
            }
        }
    }

    Rating computeScoreFromCurrent(unsigned int L, unsigned long long K, unsigned char mode)
    {
        activeMode = mode;
        buildGatherControls();
        buildTableArea();
        Rating cur = advanceShift();
        Rating best = cur;
        snapshotBest();
        return rollingStepsFrom(0, L, K, mode, cur, best);
    }
#else
    // AVX2 tick; score() and advanceShift() below stay scalar and are what decides a Rating.
    static_assert(populationThreshold % 16 == 0, "the AVX2 tick walks 16 neurons per iteration");

    // Rebuilt at the top of every score(): sources one array per slot, LUT 2 bits per entry per neuron.
    unsigned int slotIndex[numberOfNeighbors][maxNumberOfNeurons];
    unsigned long long packedLut[maxNumberOfNeurons];

    void buildTickCaches()
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
        // Trits land in the even lanes of each half; interleave, then restore neuron order.
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
    void tickNeurons()
    {
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

    // Emits shift+windowWidth outputs, grades only [shift, shift+windowWidth). Times out at maxNumberOfTicks.
    unsigned int score()
    {
        buildTickCaches();
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

            copyMem(neuronPrev, neuronOut, maxNumberOfNeurons);
            tickNeurons();

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

    // Standalone: root from the pubkey, walk with K = 0 (no explore). control/output must already be set
    // by deriveControlOutput. Does not check isCanonicalStandaloneNonce itself (the caller is expected
    // to check before calling in).
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
