#pragma once

#include "score_common.h"
#include "task_file.h"

// bpp9000 scorer: recurrent ternary ANN (trits {0,1,2}, 2=UNKNOWN); only per-neuron LUTs mutate.
// Bit-exact with test/score_bpp9000_reference.h.
namespace score_engine
{

// Largest L (mutations per step) the scorer clamps nonce[1] to
static constexpr unsigned int MAX_LUT_ENTRIES_PER_STEP = 10;

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
    // LUT row storage stride, padded to 32; only leading lutSize entries logical (index <= 26).
    static constexpr unsigned long long lutStride = 32;

    static_assert(lutSize <= lutStride, "LUT rows must fit the padded stride");

    // A nonce is canonical if its score-irrelevant knob bytes are canonical:
    // nonce[0] = algo (enforced by routing), nonce[1] = L in [1, MAX_LUT_ENTRIES_PER_STEP],
    // nonce[2] = K. The standalone walk pins K to 0, so only 0 is canonical there.
    static bool isCanonicalStandaloneNonce(const unsigned char* nonce)
    {
        return (getAlgoType(nonce) == AlgoType::Bpp9000)
            && (nonce[1] >= 1)
            && (nonce[1] <= MAX_LUT_ENTRIES_PER_STEP)
            && (nonce[2] == 0);
    }

    // K is a real degree of freedom here: the walk restores K = nonce[2] as its explore-step count
    static bool isCanonicalAntNonce(const unsigned char* nonce)
    {
        return (getAlgoType(nonce) == AlgoType::Bpp9000)
            && (nonce[1] >= 1)
            && (nonce[1] <= MAX_LUT_ENTRIES_PER_STEP)
            && (nonce[2] <= numberOfMutations);
    }

    // random2 draw sizes padded up to a multiple of 64 bytes; leading bytes bit-exact with reference.
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

    // Per-neuron role classification (neuronTypes[]).
    enum NeuronType
    {
        kInput,
        kOutput,
        kEvolution,
    };

    // An ANN is its per-neuron LUT: maxNumberOfNeurons rows of lutSize entries
    // resourceTestingDigest, written to snapshots, sent to miners...
    struct ANN
    {
        unsigned char lut[maxNumberOfNeurons * lutSize];
    };

    // padding LUT for a SIMD
    struct PaddedLut
    {
        alignas(64) unsigned char lut[maxNumberOfNeurons * lutStride];
    };

    PaddedLut currentANN;
    PaddedLut prevANN;

    // LUT that produced the score returned by the walk, in working layout. The ant colony stores this
    // as the child's inherited state, so a child branches from the best-ever LUT rather than the last
    // one walked; read it with getBestANN().
    PaddedLut bestANN;

    struct InitValue
    {
        unsigned char lutInit[lutInitPaddedBytes];
        unsigned long long mutationSeed[mutationSeedPaddedBytes / sizeof(unsigned long long)];
    } initValue;

    unsigned char inputs[sequenceLength][numberOfInputNeurons];
    unsigned char outputs[sequenceLength][numberOfOutputNeurons];

    // Stride of the interleaved wiring tuples {self, nbr0, nbr1, nbr2} (see neuronWiring below).
    static constexpr unsigned int WIRING_STRIDE = 4;

#if defined(__AVX512F__)
    // AVX-512 window-batched stat
    static constexpr unsigned long long SIMD_LANES = 64;
    static constexpr unsigned long long SIMD_WINDOWS = 128;
    static constexpr unsigned long long SIMD_LUT_STRIDE = 64;
    alignas(64) unsigned char simdCur[maxNumberOfNeurons * SIMD_LANES];
    alignas(64) unsigned char simdNxt[maxNumberOfNeurons * SIMD_LANES];
    alignas(64) unsigned short simdFeedCounter[SIMD_LANES];    // lo-half feed counters
    alignas(64) unsigned short simdFeedCounterHi[SIMD_LANES];  // hi-half feed counters
    alignas(64) unsigned char simdPredicted[SIMD_LANES];       // lo-half captured predictions
    alignas(64) unsigned char simdPredictedHi[SIMD_LANES];     // hi-half captured predictions

    // SIMD wiring
    static constexpr unsigned int SIMD_WIRING_STRIDE = 3;
    alignas(64) unsigned int neuronWiringSimd[maxNumberOfNeurons * SIMD_WIRING_STRIDE];
    unsigned int simdRowOf[maxNumberOfNeurons];   // absolute neuron -> SIMD plane row
    unsigned int simdSigIdx;                       // signal neuron's SIMD row
    unsigned int simdOutIdx;                        // output neuron's SIMD row
    unsigned int simdInputRow[numberOfInputNeurons];

    unsigned long long numberOfSimdComputed;
    unsigned int simdCanonicalRow[maxNumberOfNeurons];

    // Packed base-4 LUT
    alignas(64) unsigned char simdLut[maxNumberOfNeurons * SIMD_LUT_STRIDE];
    unsigned long long numLiveInputs;
    alignas(64) unsigned char inputsLive[sequenceLength][numberOfInputNeurons];
    alignas(64) unsigned char inputsLiveT[numberOfInputNeurons][sequenceLength + 128];

#endif

    // Precomputed wiring for updated neurons
    unsigned int neuronWiring[maxNumberOfNeurons * WIRING_STRIDE];

    unsigned int neighborIndices[populationThreshold * numberOfNeighbors];

    unsigned int inputNeuronIndices[numberOfInputNeurons];
    unsigned int outputNeuronIndices[numberOfOutputNeurons];

    unsigned int signalNeuronIndex;

    NeuronType neuronTypes[maxNumberOfNeurons];

    unsigned long long updatedNeuronIndices[maxNumberOfNeurons];
    unsigned long long numberOfUpdatedNeurons;

#if !defined(__AVX512F__)   // AVX2 fallback (score_common.h requires AVX2 or AVX-512)
    // AVX2 fallback state: SIMD_LANES windows/batch, one trit per byte-lane, [neuron][lane] layout.
    static constexpr unsigned long long SIMD_LANES = 32;
    alignas(64) unsigned char simdCur[maxNumberOfNeurons * SIMD_LANES];
    alignas(64) unsigned char simdNxt[maxNumberOfNeurons * SIMD_LANES];
    unsigned short simdFeedCounter[SIMD_LANES];   // per-lane fed-sample count
    unsigned char simdPredicted[SIMD_LANES];      // per-lane captured output trit
    bool simdDone[SIMD_LANES];                     // per-lane settled flag
#endif

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
            neuronTypes[i] = NeuronType::kEvolution;
        }
        for (unsigned long long i = 0; i < numberOfInputNeurons; ++i)
        {
            neuronTypes[inputNeuronIndices[i]] = NeuronType::kInput;
        }
        for (unsigned long long i = 0; i < numberOfOutputNeurons; ++i)
        {
            neuronTypes[outputNeuronIndices[i]] = NeuronType::kOutput;
        }

        numberOfUpdatedNeurons = 0;
        for (unsigned long long i = 0; i < populationThreshold; ++i)
        {
            if (neuronTypes[i] != NeuronType::kInput)
            {
                updatedNeuronIndices[numberOfUpdatedNeurons] = i;
                numberOfUpdatedNeurons++;
            }
        }

        // Precompute the {self, nbr0, nbr1, nbr2} wiring for the updated neurons.
        for (unsigned long long k = 0; k < numberOfUpdatedNeurons; ++k)
        {
            const unsigned long long n = updatedNeuronIndices[k];
            neuronWiring[k * WIRING_STRIDE + 0] = (unsigned int)n;
            neuronWiring[k * WIRING_STRIDE + 1] = neighborIndices[n * numberOfNeighbors + 0];
            neuronWiring[k * WIRING_STRIDE + 2] = neighborIndices[n * numberOfNeighbors + 1];
            neuronWiring[k * WIRING_STRIDE + 3] = neighborIndices[n * numberOfNeighbors + 2];
        }

#if defined(__AVX512F__)
        // Cone of influence: closure of {signal, output} under neighbour edges. Skipping updates outside
        // the cone is bit-exact.
        bool inCone[maxNumberOfNeurons];
        for (unsigned long long i = 0; i < maxNumberOfNeurons; ++i)
        {
            inCone[i] = false;
        }
        {
            unsigned int stack[maxNumberOfNeurons];
            int sp = 0;
            inCone[signalNeuronIndex] = true; stack[sp++] = signalNeuronIndex;
            if (!inCone[outputNeuronIndices[0]])
            {
                inCone[outputNeuronIndices[0]] = true; stack[sp++] = (unsigned int)outputNeuronIndices[0];
            }
            while (sp > 0)
            {
                const unsigned int n = stack[--sp];
                if (neuronTypes[n] == NeuronType::kInput)
                {
                    continue; // inputs are leaves
                }
                for (unsigned long long j = 0; j < numberOfNeighbors; ++j)
                {
                    const unsigned int nb = neighborIndices[n * numberOfNeighbors + j];
                    if (!inCone[nb])
                    {
                        inCone[nb] = true; stack[sp++] = nb;
                    }
                }
            }
        }

        // live[]/liveCanon[]: live (cone) updated neurons in canonical order; liveCanon[j] = dense LUT row.
        unsigned int live[maxNumberOfNeurons];
        unsigned int liveCanon[maxNumberOfNeurons];
        unsigned long long nLive = 0;
        for (unsigned long long k = 0; k < numberOfUpdatedNeurons; ++k)
        {
            if (inCone[updatedNeuronIndices[k]])
            {
                liveCanon[nLive] = (unsigned int)k;
                live[nLive] = (unsigned int)updatedNeuronIndices[k];
                nLive++;
            }
        }
        numberOfSimdComputed = nLive;

        // Rows 0..L-1 = live neurons in canonical order, rows L.. = dead; inputs at row
        // numberOfUpdatedNeurons+i. simdCanonicalRow[j] = canonical LUT row.
        unsigned long long simdOrder[maxNumberOfNeurons];
        unsigned int canonOf[maxNumberOfNeurons];
        copyMem(canonOf, liveCanon, nLive * sizeof(canonOf[0]));
        unsigned long long jj = nLive;
        for (unsigned long long j = 0; j < nLive; ++j)
        {
            simdOrder[j] = live[j];
        }
        for (unsigned long long k = 0; k < numberOfUpdatedNeurons; ++k)
        {
            if (!inCone[updatedNeuronIndices[k]])
            {
                simdOrder[jj] = updatedNeuronIndices[k];
                canonOf[jj] = (unsigned int)k;
                jj++;
            }
        }

        copyMem(simdCanonicalRow, canonOf, numberOfUpdatedNeurons * sizeof(simdCanonicalRow[0]));
        for (unsigned long long j = 0; j < numberOfUpdatedNeurons; ++j)
        {
            simdRowOf[simdOrder[j]] = (unsigned int)j;
        }
        // Partition input neurons live-first: live inputs take the first numLiveInputs rows, dead inputs
        // the rest (never fed/reset, bit-exact). liveSrc[r] = original input position feeding live row r.
        unsigned int liveSrc[numberOfInputNeurons];
        {
            unsigned long long r = 0;
            for (unsigned long long i = 0; i < numberOfInputNeurons; ++i)
            {
                if (inCone[inputNeuronIndices[i]])
                {
                    simdRowOf[inputNeuronIndices[i]] = (unsigned int)(numberOfUpdatedNeurons + r);
                    liveSrc[r] = (unsigned int)i;
                    r++;
                }
            }
            numLiveInputs = r;
            for (unsigned long long i = 0; i < numberOfInputNeurons; ++i)
            {
                if (!inCone[inputNeuronIndices[i]])
                {
                    simdRowOf[inputNeuronIndices[i]] = (unsigned int)(numberOfUpdatedNeurons + r);
                    r++;
                }
            }
        }
        // Wiring: 3 neighbour byte-offsets (row * SIMD_LANES) for SIMD row j; self implicit (store = row j).
        for (unsigned long long j = 0; j < numberOfUpdatedNeurons; ++j)
        {
            const unsigned long long n = simdOrder[j];
            neuronWiringSimd[j * SIMD_WIRING_STRIDE + 0] = simdRowOf[neighborIndices[n * numberOfNeighbors + 0]] * (unsigned int)SIMD_LANES;
            neuronWiringSimd[j * SIMD_WIRING_STRIDE + 1] = simdRowOf[neighborIndices[n * numberOfNeighbors + 1]] * (unsigned int)SIMD_LANES;
            neuronWiringSimd[j * SIMD_WIRING_STRIDE + 2] = simdRowOf[neighborIndices[n * numberOfNeighbors + 2]] * (unsigned int)SIMD_LANES;
        }
        simdSigIdx = simdRowOf[signalNeuronIndex];
        simdOutIdx = simdRowOf[outputNeuronIndices[0]];
        for (unsigned long long i = 0; i < numberOfInputNeurons; ++i)
        {
            simdInputRow[i] = simdRowOf[inputNeuronIndices[i]];
        }
        // Repack samples live-first in field-replicated form (v*5 = 0x00/0x05/0x0A).
        for (unsigned long long sample = 0; sample < sequenceLength; ++sample)
        {
            for (unsigned long long r = 0; r < numLiveInputs; ++r)
            {
                inputsLive[sample][r] = (unsigned char)(inputs[sample][liveSrc[r]] * 5);
            }
        }

        // Transposed live-input stream for the vectorized feed (inputsLiveT[r][s] == inputsLive[s][r]).
        for (unsigned long long r = 0; r < numLiveInputs; ++r)
        {
            for (unsigned long long sample = 0; sample < sequenceLength; ++sample)
            {
                inputsLiveT[r][sample] = (unsigned char)(inputs[sample][liveSrc[r]] * 5);
            }
            for (unsigned long long p = sequenceLength; p < sequenceLength + 128; ++p)
            {
                inputsLiveT[r][p] = 0;
            }
        }

        // Zero the 22 unreachable pad slots per simdLut row once (scoreSIMD only rewrites the 27 reachable).
        setMem(simdLut, sizeof(simdLut), 0);
#endif
    }

    // Sliding-window self-clocked score via the window-batched SIMD kernel
    // Apply on the curANN
    unsigned int score()
    {
        // PROFILE_NAMED_SCOPE("bpp9000:score");
        return scoreSIMD();
    }

#if defined(__AVX512F__)
    // Ttry to minimal the L1-L3 cache as much as possible
    inline void tickNeuron(const unsigned char* cur, unsigned char* nxt,
                           const unsigned char* lut, unsigned long long k)
    {
        const unsigned long long o0 = neuronWiringSimd[k * SIMD_WIRING_STRIDE + 0];
        const unsigned long long o1 = neuronWiringSimd[k * SIMD_WIRING_STRIDE + 1];
        const unsigned long long o2 = neuronWiringSimd[k * SIMD_WIRING_STRIDE + 2];
        const __m512i t0 = _mm512_loadu_si512((const void*)&cur[o0]);
        const __m512i t1 = _mm512_loadu_si512((const void*)&cur[o1]);
        const __m512i t2 = _mm512_loadu_si512((const void*)&cur[o2]);

        const __m512i m30 = _mm512_set1_epi8((char)0x30);
        const __m512i mCC = _mm512_set1_epi8((char)0xCC);
        const __m512i mF0 = _mm512_set1_epi8((char)0xF0);

        // Merge t0t1
        const __m512i M = _mm512_ternarylogic_epi32(t0, t1, mCC, 0xD8);

        // low index
        const __m512i idxLo = _mm512_ternarylogic_epi32(M, _mm512_slli_epi16(t2, 4), m30, 0xD8);

        // gihh index
        const __m512i idxHi = _mm512_ternarylogic_epi32(_mm512_srli_epi16(M, 4), t2, m30, 0xD8);

        const __m512i tbl = _mm512_loadu_si512((const void*)&lut[k * SIMD_LUT_STRIDE]);
        const __m512i rLo = _mm512_permutexvar_epi8(idxLo, tbl);
        const __m512i rHi = _mm512_permutexvar_epi8(idxHi, tbl);
        const __m512i res = _mm512_ternarylogic_epi32(rLo, rHi, mF0, 0xD8);
        _mm512_storeu_si512((void*)&nxt[k * SIMD_LANES], res);
    }

    // Drives tickNeuron across the live neurons this help to improve cache access
    void processTickSIMD(const unsigned char* cur, unsigned char* nxt)
    {
        const unsigned char* lut = simdLut;
        const unsigned long long m = numberOfSimdComputed;
        for (unsigned long long k = 0; k < m; ++k)
        {
            tickNeuron(cur, nxt, lut, k);
        }
    }

    // Window-batched score: SIMD_WINDOWS windows/batch, two per byte-lane (lo bits[0:3], hi bits[4:7]),
    // halves independent. INFINITE_ERROR iff any window times out, else the failure count.
    unsigned int scoreSIMD()
    {
        unsigned int numberOfFailures = 0;

        const unsigned int sigIdx = simdSigIdx;
        const unsigned int outIdx = simdOutIdx;
        const __m512i vTWO            = _mm512_set1_epi8((char)0x0A); // lo window UNKNOWN, fields 0+1
        const __m512i vTWENTY         = _mm512_set1_epi8((char)0xA0); // hi window UNKNOWN, fields 2+3
        const __m512i vPACKED_UNKNOWN = _mm512_set1_epi8((char)0xAA);
        const __m512i mF0 = _mm512_set1_epi8((char)0xF0);
        const __m512i m03 = _mm512_set1_epi8((char)0x03);
        const __m512i m02 = _mm512_set1_epi8((char)0x02);
        const __m512i m20 = _mm512_set1_epi8((char)0x20);

        // Rebuild the packed base-4 LUT rows, help to reduce the ALU/FP port bound
        for (unsigned long long j = 0; j < numberOfSimdComputed; ++j)
        {
            const unsigned char* s = &currentANN.lut[(unsigned long long)simdCanonicalRow[j] * lutStride];
            unsigned char* d = &simdLut[j * SIMD_LUT_STRIDE];
            for (unsigned c = 0; c < 3; ++c)
            {
                for (unsigned b = 0; b < 3; ++b)
                {
                    for (unsigned a = 0; a < 3; ++a)
                    {
                        d[a + 4 * b + 16 * c] = (unsigned char)(s[a + 3 * b + 9 * c] * 0x55);
                    }
                }
            }
        }

        unsigned char* cur = simdCur;
        unsigned char* nxt = simdNxt;

        // Constants for the vectorized feed step (shared by both halves).
        const __m512i kLaneLo = _mm512_set_epi16(31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0);
        const __m512i kLaneHi = _mm512_set_epi16(63,62,61,60,59,58,57,56,55,54,53,52,51,50,49,48,47,46,45,44,43,42,41,40,39,38,37,36,35,34,33,32);
        const __m512i kFFFF = _mm512_set1_epi16((short)0xFFFF);
        const __m512i k127  = _mm512_set1_epi16(127);
        const __m512i k63   = _mm512_set1_epi16(63);
        const __m512i kOneB = _mm512_set1_epi8(1);
        const __m512i k127b = _mm512_set1_epi8((char)127);
        const __m512i kFFb  = _mm512_set1_epi8((char)0xFF);
        const __m512i kLaneB = _mm512_set_epi8(
            63,62,61,60,59,58,57,56,55,54,53,52,51,50,49,48,47,46,45,44,43,42,41,40,39,38,37,36,35,34,33,32,
            31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0);
        const __m512i k255w = _mm512_set1_epi16(255);
        // thr[l] = l + windowWidth - base, saturated bytes; clamp never observable (bit-exact).
        auto makeThr = [&](unsigned int b) -> __m512i {
            if (b <= (unsigned int)windowWidth)
            {
                unsigned int d = (unsigned int)windowWidth - b;
                if (d > 255u)
                {
                    d = 255u;
                }
                return _mm512_adds_epu8(kLaneB, _mm512_set1_epi8((char)(unsigned char)d));
            }
            unsigned int d = b - (unsigned int)windowWidth;
            if (d > 255u)
            {
                d = 255u;
            }
            return _mm512_subs_epu8(kLaneB, _mm512_set1_epi8((char)(unsigned char)d));
        };

        for (unsigned long long base = 0; base < numberOfWindows; base += SIMD_WINDOWS)
        {
            const unsigned long long batch =
                (numberOfWindows - base) < SIMD_WINDOWS ? (numberOfWindows - base) : SIMD_WINDOWS;
            const unsigned long long loBatch = batch < 64 ? batch : 64;
            const unsigned long long hiBatch = batch > 64 ? batch - 64 : 0;
            const unsigned long long hiOrigin = base + 64;

            // Lanes [0,loBatch)/[0,hiBatch) are active
            const __mmask64 activeInitLo = (loBatch < 64) ? (((unsigned long long)1 << loBatch) - 1) : ~0ULL;
            const __mmask64 activeInitHi = (hiBatch < 64) ? (((unsigned long long)1 << hiBatch) - 1) : ~0ULL;
            __mmask64 doneLo = ~activeInitLo;
            __mmask64 doneHi = ~activeInitHi;

            for (unsigned long long l = 0; l < SIMD_LANES; ++l)
            {
                simdPredicted[l] = TRIT_UNKNOWN;
                simdPredictedHi[l] = TRIT_UNKNOWN;
            }
            // Reduce op count for speed up
            setMem(cur, maxNumberOfNeurons * SIMD_LANES, 0xAA);

            __mmask64 fcGEMaskLo = 0;
            __mmask64 fcGEMaskHi = 0;
            __m512i relLo = kLaneB;
            __m512i relHi = kLaneB;
            unsigned int bLo = 0, bHi = 0;
            // Byte-domain fcGE thresholds, refreshed only when the cached base changes
            __m512i thrLo = makeThr(0);
            __m512i thrHi = makeThr(0);
            // position for feed counter
            _mm512_storeu_si512((void*)&simdFeedCounter[0], kLaneLo);
            _mm512_storeu_si512((void*)&simdFeedCounter[32], kLaneHi);
            _mm512_storeu_si512((void*)&simdFeedCounterHi[0], kLaneLo);
            _mm512_storeu_si512((void*)&simdFeedCounterHi[32], kLaneHi);

            unsigned long long tick;
            for (tick = 0; tick < maxNumberOfTicks; tick++)
            {
                const __m512i sig = _mm512_loadu_si512((const void*)&cur[(unsigned long long)sigIdx * SIMD_LANES]);
                const __mmask64 unkLo = _mm512_test_epi8_mask(sig, m02);
                const __mmask64 unkHi = _mm512_test_epi8_mask(sig, m20);
                const __mmask64 unkActiveLo = unkLo & ~doneLo;
                const __mmask64 unkActiveHi = unkHi & ~doneHi;

                // feedCounter >= windowWidth, move the fcGE
                const __mmask64 fcGELo = fcGEMaskLo;
                const __mmask64 fcGEHi = fcGEMaskHi;

                const __mmask64 finishLo = unkActiveLo & fcGELo;
                const __mmask64 finishHi = unkActiveHi & fcGEHi;
                const __mmask64 feedLo = unkActiveLo & ~fcGELo;
                const __mmask64 feedHi = unkActiveHi & ~fcGEHi;

                // Output complete
                if (finishLo | finishHi)
                {
                    const __m512i outv = _mm512_loadu_si512((const void*)&cur[(unsigned long long)outIdx * SIMD_LANES]);
                    if (finishLo)
                    {
                        const __m512i loVal = _mm512_and_si512(outv, m03);
                        _mm512_mask_storeu_epi8((void*)simdPredicted, finishLo, loVal);
                        doneLo |= finishLo;
                    }
                    if (finishHi)
                    {
                        const __m512i hiVal = _mm512_and_si512(_mm512_srli_epi16(outv, 4), m03);
                        _mm512_mask_storeu_epi8((void*)simdPredictedHi, finishHi, hiVal);
                        doneHi |= finishHi;
                    }
                    if (doneLo == ~0ULL && doneHi == ~0ULL)
                    {
                        break;
                    }
                }

                // Now we check the feed data
                if (feedLo | feedHi)
                {
                    const bool loEmpty = (feedLo == 0);
                    const bool hiEmpty = (feedHi == 0);
                    bool loScalar = false, hiScalar = false;
                    unsigned int mnLo = 0, mnHi = 0;
                    __m512i idx8Lo = _mm512_setzero_si512();
                    __m512i idx8Hi = _mm512_setzero_si512();
                    alignas(64) unsigned char scalarLoBuf[numberOfInputNeurons][64];
                    alignas(64) unsigned char scalarHiBuf[numberOfInputNeurons][64];

                    if (!loEmpty)
                    {
                        const __mmask32 fmLo = (__mmask32)feedLo;
                        const __mmask32 fmHi = (__mmask32)(feedLo >> 32);
                        __mmask64 ov = _mm512_mask_cmpgt_epu8_mask(feedLo, relLo, k127b);
                        if (ov)
                        {
                            // Positions from bytes
                            const __m512i relW0 = _mm512_cvtepu8_epi16(_mm512_castsi512_si256(relLo));
                            const __m512i relW1 = _mm512_cvtepu8_epi16(_mm512_extracti64x4_epi64(relLo, 1));
                            const __m512i bv    = _mm512_set1_epi16((short)bLo);
                            __m512i soLo = _mm512_add_epi16(relW0, bv);
                            __m512i soHi = _mm512_add_epi16(relW1, bv);
                            const __mmask32 sat0 = _mm512_cmpeq_epi16_mask(relW0, k255w);
                            const __mmask32 sat1 = _mm512_cmpeq_epi16_mask(relW1, k255w);
                            soLo = _mm512_mask_loadu_epi16(soLo, sat0, (const void*)&simdFeedCounter[0]);
                            soHi = _mm512_mask_loadu_epi16(soHi, sat1, (const void*)&simdFeedCounter[32]);
                            _mm512_storeu_si512((void*)&simdFeedCounter[0],  soLo);
                            _mm512_storeu_si512((void*)&simdFeedCounter[32], soHi);
                            // Minmal on cache miss
                            __m512i mnv = _mm512_min_epu16(
                                _mm512_mask_blend_epi16(fmLo, kFFFF, soLo),
                                _mm512_mask_blend_epi16(fmHi, kFFFF, soHi));
                            __m256i mnv256 = _mm256_min_epu16(_mm512_castsi512_si256(mnv), _mm512_extracti64x4_epi64(mnv, 1));
                            __m128i mnv128 = _mm_min_epu16(_mm256_castsi256_si128(mnv256), _mm256_extracti128_si256(mnv256, 1));
                            mnv128 = _mm_min_epu16(mnv128, _mm_shuffle_epi32(mnv128, 0x4E));
                            mnv128 = _mm_min_epu16(mnv128, _mm_shuffle_epi32(mnv128, 0xB1));
                            mnv128 = _mm_min_epu16(mnv128, _mm_shufflelo_epi16(mnv128, 0xB1));
                            const unsigned int mn = (unsigned int)(unsigned short)_mm_extract_epi16(mnv128, 0);
                            bLo = mn;
                            thrLo = makeThr(mn);
                            const __m512i mnb = _mm512_set1_epi16((short)mn);
                            const __m512i rA = _mm512_sub_epi16(soLo, mnb);
                            const __m512i rB = _mm512_sub_epi16(soHi, mnb);
                            relLo = _mm512_inserti64x4(
                                _mm512_castsi256_si512(_mm512_cvtusepi16_epi8(rA)),
                                _mm512_cvtusepi16_epi8(rB), 1);
                            ov = _mm512_mask_cmpgt_epu8_mask(feedLo, relLo, k127b);
                        }
                        mnLo = bLo;
                        if (ov == 0)
                        {
                            idx8Lo = _mm512_mask_mov_epi8(vPACKED_UNKNOWN, feedLo, relLo);
                            relLo = _mm512_mask_adds_epu8(relLo, feedLo, relLo, kOneB);
                            fcGEMaskLo = _kor_mask64(fcGEMaskLo,
                                _mm512_mask_cmp_epu8_mask(feedLo, relLo, thrLo, _MM_CMPINT_NLT));
                        }
                        else
                        {
                            relLo = kFFb;
                            // The scalart path where data is not fited
                            loScalar = true;
                            setMem(scalarLoBuf, sizeof(scalarLoBuf), 0x0A);
                            unsigned long long fm = feedLo;
                            while (fm)
                            {
                                const unsigned long long l = countTrailingZerosAssumeNonZero64(fm);
                                fm &= fm - 1;
                                const unsigned long long sample = base + (unsigned long long)simdFeedCounter[l];
                                const unsigned char* const s = &inputsLive[sample][0];
                                for (unsigned long long r = 0; r < numLiveInputs; ++r)
                                {
                                    scalarLoBuf[r][l] = s[r];
                                }
                                simdFeedCounter[l]++;
                                if ((unsigned long long)simdFeedCounter[l] >= windowWidth + l)
                                {
                                    fcGEMaskLo = _kor_mask64(fcGEMaskLo, (__mmask64)(1ULL << l));
                                }
                            }
                        }
                    }

                    if (!hiEmpty)
                    {
                        const __mmask32 fmLo = (__mmask32)feedHi;
                        const __mmask32 fmHi = (__mmask32)(feedHi >> 32);
                        __mmask64 ov = _mm512_mask_cmpgt_epu8_mask(feedHi, relHi, k127b);
                        if (ov)
                        {
                            // Position in byte state
                            const __m512i relW0 = _mm512_cvtepu8_epi16(_mm512_castsi512_si256(relHi));
                            const __m512i relW1 = _mm512_cvtepu8_epi16(_mm512_extracti64x4_epi64(relHi, 1));
                            const __m512i bv    = _mm512_set1_epi16((short)bHi);
                            __m512i soLo = _mm512_add_epi16(relW0, bv);
                            __m512i soHi = _mm512_add_epi16(relW1, bv);
                            const __mmask32 sat0 = _mm512_cmpeq_epi16_mask(relW0, k255w);
                            const __mmask32 sat1 = _mm512_cmpeq_epi16_mask(relW1, k255w);
                            soLo = _mm512_mask_loadu_epi16(soLo, sat0, (const void*)&simdFeedCounterHi[0]);
                            soHi = _mm512_mask_loadu_epi16(soHi, sat1, (const void*)&simdFeedCounterHi[32]);
                            _mm512_storeu_si512((void*)&simdFeedCounterHi[0],  soLo);
                            _mm512_storeu_si512((void*)&simdFeedCounterHi[32], soHi);
                            __m512i mnv = _mm512_min_epu16(
                                _mm512_mask_blend_epi16(fmLo, kFFFF, soLo),
                                _mm512_mask_blend_epi16(fmHi, kFFFF, soHi));
                            __m256i mnv256 = _mm256_min_epu16(_mm512_castsi512_si256(mnv), _mm512_extracti64x4_epi64(mnv, 1));
                            __m128i mnv128 = _mm_min_epu16(_mm256_castsi256_si128(mnv256), _mm256_extracti128_si256(mnv256, 1));
                            mnv128 = _mm_min_epu16(mnv128, _mm_shuffle_epi32(mnv128, 0x4E));
                            mnv128 = _mm_min_epu16(mnv128, _mm_shuffle_epi32(mnv128, 0xB1));
                            mnv128 = _mm_min_epu16(mnv128, _mm_shufflelo_epi16(mnv128, 0xB1));
                            const unsigned int mn = (unsigned int)(unsigned short)_mm_extract_epi16(mnv128, 0);
                            bHi = mn;
                            thrHi = makeThr(mn);
                            const __m512i mnb = _mm512_set1_epi16((short)mn);
                            const __m512i rA = _mm512_sub_epi16(soLo, mnb);
                            const __m512i rB = _mm512_sub_epi16(soHi, mnb);
                            relHi = _mm512_inserti64x4(
                                _mm512_castsi256_si512(_mm512_cvtusepi16_epi8(rA)),
                                _mm512_cvtusepi16_epi8(rB), 1);
                            ov = _mm512_mask_cmpgt_epu8_mask(feedHi, relHi, k127b);
                        }
                        mnHi = bHi;
                        if (ov == 0)
                        {
                            idx8Hi = _mm512_mask_mov_epi8(vPACKED_UNKNOWN, feedHi, relHi);
                            relHi = _mm512_mask_adds_epu8(relHi, feedHi, relHi, kOneB);
                            fcGEMaskHi = _kor_mask64(fcGEMaskHi,
                                _mm512_mask_cmp_epu8_mask(feedHi, relHi, thrHi, _MM_CMPINT_NLT));
                        }
                        else
                        {
                            relHi = kFFb;
                            hiScalar = true;
                            setMem(scalarHiBuf, sizeof(scalarHiBuf), 0xA0);
                            unsigned long long fm = feedHi;
                            while (fm)
                            {
                                const unsigned long long l = countTrailingZerosAssumeNonZero64(fm);
                                fm &= fm - 1;
                                const unsigned long long sample = hiOrigin + (unsigned long long)simdFeedCounterHi[l];
                                const unsigned char* const s = &inputsLive[sample][0];
                                for (unsigned long long r = 0; r < numLiveInputs; ++r)
                                {
                                    scalarHiBuf[r][l] = (unsigned char)(s[r] << 4);
                                }
                                simdFeedCounterHi[l]++;
                                if ((unsigned long long)simdFeedCounterHi[l] >= windowWidth + l)
                                {
                                    fcGEMaskHi = _kor_mask64(fcGEMaskHi, (__mmask64)(1ULL << l));
                                }
                            }
                        }
                    }

                    unsigned char* const curInBase = cur + numberOfUpdatedNeurons * SIMD_LANES;
                    for (unsigned long long r = 0; r < numLiveInputs; ++r)
                    {
                        __m512i pLo;
                        if (loEmpty)
                        {
                            pLo = vTWO;
                        }
                        else if (loScalar)
                        {
                            pLo = _mm512_loadu_si512((const void*)scalarLoBuf[r]);
                        }
                        else
                        {
                            // [CACHE] gather the fed samples from the transposed input table (load-bound).
                            const unsigned char* const w0 = &inputsLiveT[r][base + mnLo];
                            const __m512i gLo = _mm512_permutex2var_epi8(_mm512_loadu_si512((const void*)w0), idx8Lo, _mm512_loadu_si512((const void*)(w0 + 64)));
                            pLo = _mm512_mask_mov_epi8(vTWO, feedLo, gLo);
                        }

                        __m512i pHi;
                        if (hiEmpty)
                        {
                            pHi = vTWENTY;
                        }
                        else if (hiScalar)
                        {
                            pHi = _mm512_loadu_si512((const void*)scalarHiBuf[r]);
                        }
                        else
                        {
                            const unsigned char* const w0 = &inputsLiveT[r][hiOrigin + mnHi];
                            const __m512i gHi = _mm512_permutex2var_epi8(_mm512_loadu_si512((const void*)w0), idx8Hi, _mm512_loadu_si512((const void*)(w0 + 64)));
                            pHi = _mm512_mask_mov_epi8(vTWENTY, feedHi, _mm512_slli_epi16(gHi, 4));
                        }

                        _mm512_storeu_si512((void*)(curInBase + r * SIMD_LANES), _mm512_or_si512(pLo, pHi));
                    }
                }
                else
                {
                    unsigned char* const curInBase = cur + numberOfUpdatedNeurons * SIMD_LANES;
                    for (unsigned long long r = 0; r < numLiveInputs; ++r)
                    {
                        _mm512_storeu_si512((void*)(curInBase + r * SIMD_LANES), vPACKED_UNKNOWN);
                    }
                }

                processTickSIMD(cur, nxt);

                // Swapt pointer
                unsigned char* const tmp = cur;
                cur = nxt;
                nxt = tmp;
            }

            if (tick == maxNumberOfTicks)
            {
                // At least one window did not finish -> timeout -> whole score is INFINITE_ERROR.
                return INFINITE_ERROR;
            }

            for (unsigned long long l = 0; l < loBatch; ++l)
            {
                const unsigned char expected = outputs[base + l + windowWidth][0];
                if (simdPredicted[l] != expected)
                {
                    numberOfFailures++;
                }
            }
            for (unsigned long long l = 0; l < hiBatch; ++l)
            {
                const unsigned char expected = outputs[hiOrigin + l + windowWidth][0];
                if (simdPredictedHi[l] != expected)
                {
                    numberOfFailures++;
                }
            }
        }

        return numberOfFailures;
    }
#endif // defined(__AVX512F__)

#if !defined(__AVX512F__)
    void processTickSIMD(const unsigned char* cur, unsigned char* nxt)
    {
        // PROFILE_NAMED_SCOPE("bpp9000:processTick");
        const unsigned char* lut = currentANN.lut;
        const unsigned int* wiring = neuronWiring;
        const unsigned long long m = numberOfUpdatedNeurons;
        const __m256i fifteen = _mm256_set1_epi8(15);
        for (unsigned long long k = 0; k < m; ++k)
        {
            const unsigned int* w = &wiring[k * WIRING_STRIDE];
            const __m256i t0 = _mm256_loadu_si256((const __m256i*)(cur + (unsigned long long)w[1] * SIMD_LANES));
            const __m256i t1 = _mm256_loadu_si256((const __m256i*)(cur + (unsigned long long)w[2] * SIMD_LANES));
            const __m256i t2 = _mm256_loadu_si256((const __m256i*)(cur + (unsigned long long)w[3] * SIMD_LANES));

            const __m256i t2x3 = _mm256_add_epi8(_mm256_add_epi8(t2, t2), t2);
            const __m256i inner = _mm256_add_epi8(t1, t2x3);
            const __m256i inner3 = _mm256_add_epi8(_mm256_add_epi8(inner, inner), inner);
            const __m256i idx = _mm256_add_epi8(t0, inner3);
            const __m256i tblLo = _mm256_broadcastsi128_si256(_mm_loadu_si128((const __m128i*)(lut + k * lutStride)));
            const __m256i tblHi = _mm256_broadcastsi128_si256(_mm_loadu_si128((const __m128i*)(lut + k * lutStride + 16)));
            const __m256i rLo = _mm256_shuffle_epi8(tblLo, idx);
            const __m256i rHi = _mm256_shuffle_epi8(tblHi, idx);
            const __m256i ge16 = _mm256_cmpgt_epi8(idx, fifteen);
            const __m256i res = _mm256_blendv_epi8(rLo, rHi, ge16);
            _mm256_storeu_si256((__m256i*)(nxt + (unsigned long long)w[0] * SIMD_LANES), res);
        }
    }

    // AVX2 window-batched score
    unsigned int scoreSIMD()
    {
        unsigned int numberOfFailures = 0;
        const unsigned int sigIdx = signalNeuronIndex;
        const unsigned int outIdx = outputNeuronIndices[0];
        unsigned char* cur = simdCur;
        unsigned char* nxt = simdNxt;

        for (unsigned long long base = 0; base < numberOfWindows; base += SIMD_LANES)
        {
            const unsigned long long batch =
                (numberOfWindows - base) < SIMD_LANES ? (numberOfWindows - base) : SIMD_LANES;

            setMem(cur, maxNumberOfNeurons * SIMD_LANES, TRIT_UNKNOWN);
            for (unsigned long long l = 0; l < SIMD_LANES; ++l)
            {
                simdFeedCounter[l] = 0;
                simdPredicted[l] = TRIT_UNKNOWN;
                simdDone[l] = (l >= batch);
            }
            unsigned long long remaining = batch;

            unsigned long long tick;
            for (tick = 0; tick < maxNumberOfTicks; ++tick)
            {
                const unsigned char* const sigRow = cur + (unsigned long long)sigIdx * SIMD_LANES;
                for (unsigned long long l = 0; l < batch; ++l)
                {
                    if (simdDone[l])
                    {
                        continue;
                    }
                    if (sigRow[l] == TRIT_UNKNOWN)
                    {
                        if (simdFeedCounter[l] >= windowWidth)
                        {
                            simdPredicted[l] = cur[(unsigned long long)outIdx * SIMD_LANES + l];
                            simdDone[l] = true;
                            --remaining;
                            continue;
                        }
                        const unsigned long long sample = base + l + simdFeedCounter[l];
                        for (unsigned long long i = 0; i < numberOfInputNeurons; ++i)
                        {
                            cur[(unsigned long long)inputNeuronIndices[i] * SIMD_LANES + l] = inputs[sample][i];
                        }
                        ++simdFeedCounter[l];
                    }
                    else
                    {
                        for (unsigned long long i = 0; i < numberOfInputNeurons; ++i)
                        {
                            cur[(unsigned long long)inputNeuronIndices[i] * SIMD_LANES + l] = TRIT_UNKNOWN;
                        }
                    }
                }

                if (remaining == 0)
                {
                    break;
                }

                processTickSIMD(cur, nxt);

                unsigned char* const tmp = cur;
                cur = nxt;
                nxt = tmp;
            }

            if (remaining != 0)
            {
                // A window did not settle within maxNumberOfTicks -> whole score fails.
                return INFINITE_ERROR;
            }

            for (unsigned long long l = 0; l < batch; ++l)
            {
                const unsigned char expected = outputs[base + l + windowWidth][0];
                if (simdPredicted[l] != expected)
                {
                    ++numberOfFailures;
                }
            }
        }

        return numberOfFailures;
    }
#endif // !defined(__AVX512F__)

    // Flip one LUT entry of one non-input neuron (bit 0 picks the new trit, the rest picks the line).
    void mutate(unsigned long long mutationSeed)
    {
        const unsigned long long delta = mutationSeed & 1ULL;

        const unsigned long long totalLines = numberOfUpdatedNeurons * lutSize;
        // LUT stored densely by updated-neuron position: flatIdx = position * lutSize + line.
        const unsigned long long flatIdx = (mutationSeed >> 1) % totalLines;

        // Map flatIdx from the lutSize-based RNG index space onto the stride-padded storage row.
        const unsigned long long storageIdx = (flatIdx / lutSize) * lutStride + (flatIdx % lutSize);
        const unsigned char oldTrit = currentANN.lut[storageIdx];
        const unsigned char newTrit = (unsigned char)((oldTrit + 1 + delta) % 3);
        currentANN.lut[storageIdx] = newTrit;
    }

    // Derive the root LUT material
    void deriveRootLut(const unsigned char* publicKey, const unsigned char* pRandom2Pool)
    {
        unsigned char rootHash[32];
        KangarooTwelve(publicKey, 32, rootHash, 32);
        random2(rootHash, pRandom2Pool, (unsigned char*)&initValue.lutInit, lutInitPaddedBytes);
    }

    // Derive the mutation-walk seeds. nonce[0..2] are the algo/L/K knobs and stay excluded from the RNG,
    // so K and L can be chosen freely without reseeding the walk. anchorTickDigest == nullptr for the
    // standalone walk; the ant colony binds a child's walk to the tick it anchors on.
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
        random2(searchHash, pRandom2Pool, (unsigned char*)&initValue.mutationSeed, mutationSeedPaddedBytes);
    }

    // Store the LUT densely by updated-neuron position k (row k): row k holds neuron
    // updatedNeuronIndices[k]'s LUT. RNG draw into initValue.lutInit unchanged (bit-exact).
    void applyRootLut(PaddedLut& target)
    {
        // The loop below writes only rows [0, numberOfUpdatedNeurons) columns [0, lutSize), and the
        // SIMD path loads whole rows, so the rest has to be initialised rather than left as whatever
        // the buffer previously held.
        setMem(&target, sizeof(target), 0);

        for (unsigned long long k = 0; k < numberOfUpdatedNeurons; ++k)
        {
            const unsigned long long n = updatedNeuronIndices[k];
            for (unsigned long long line = 0; line < lutSize; ++line)
            {
                target.lut[k * lutStride + line] = (unsigned char)(initValue.lutInit[n * lutSize + line] % 3);
            }
        }
    }

    // Working layout to ANN remove the stride padding.
    void compact(const PaddedLut& src, ANN& out) const
    {
        for (unsigned long long k = 0; k < maxNumberOfNeurons; ++k)
        {
            copyMem(out.lut + k * lutSize, src.lut + k * lutStride, lutSize);
        }
    }

    // Restores the stride and zeroes the padding.
    void expand(const ANN& src, PaddedLut& out) const
    {
        setMem(&out, sizeof(out), 0);
        for (unsigned long long k = 0; k < maxNumberOfNeurons; ++k)
        {
            copyMem(out.lut + k * lutStride, src.lut + k * lutSize, lutSize);
        }
    }

    // The LUT behind the score the last walk returned.
    void getBestANN(ANN& out) const
    {
        compact(bestANN, out);
    }

    // Standalone path only: root LUT from the pubkey alone; the ant path derives its shared epoch
    // root via deriveRootANN(rootSeed) instead. Mutation seeds from pubkey+nonce (nonce[0..2] are
    // the algo/L/K knobs, excluded from the RNG). Returns the start score.
    unsigned int initializeANN(
        const unsigned char* publicKey,
        const unsigned char* nonce,
        const unsigned char* pRandom2Pool)
    {
        // PROFILE_NAMED_SCOPE("bpp9000:initializeANN");
        deriveRootLut(publicKey, pRandom2Pool);
        deriveMutationSeeds(publicKey, nonce, nullptr, pRandom2Pool);
        applyRootLut(currentANN);

        return score();
    }

    // Miner-chosen number of LUT entries rewritten per step, clamped to the verifiable range.
    static unsigned int lutEntriesPerStep(const unsigned char* nonce)
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
        return L;
    }

    // Anti-attractor walk starting from the LUT already in currentANN: L mutations/step; accept
    // worse-or-equal for the first K steps (explore), then better-or-equal (exploit); one-step rollback.
    // Returns the best score found and leaves the LUT that produced it in bestANN.
    unsigned int computeScoreFromCurrent(unsigned int L, unsigned long long K, unsigned int startScore)
    {
        unsigned int cur = startScore;
        unsigned int best = cur;
        copyMem(&bestANN, &currentANN, sizeof(bestANN));

        for (unsigned long long s = 0; s < numberOfMutations; ++s)
        {
            copyMem(&prevANN, &currentANN, sizeof(prevANN));

            for (unsigned int i = 0; i < L; ++i)
            {
                mutate(initValue.mutationSeed[s * MAX_LUT_ENTRIES_PER_STEP + i]);
            }

            unsigned int r = score();

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
        return best;
    }

    // Anti-attractor search: L mutations/step; accept worse-or-equal for the first K steps (explore),
    // then better-or-equal (exploit); one-step rollback; keep and return the best score found.
    unsigned int computeScore(
        const unsigned char* publicKey,
        const unsigned char* nonce,
        const unsigned char* pRandom2Pool)
    {
        // PROFILE_NAMED_SCOPE("bpp9000:computeScore");
        const unsigned int L = lutEntriesPerStep(nonce);
        // Explore disabled for the standalone algorithm (K=0); the ant colony passes K = nonce[2].
        const unsigned long long K = 0;

        const unsigned int cur = initializeANN(publicKey, nonce, pRandom2Pool);

        return computeScoreFromCurrent(L, K, cur);
    }

    // Ant colony: the shared per-epoch network every identity's tree starts from. rootSeed is the
    // epoch-start spectrum digest, so all identities derive the identical root; only the mutation
    // walks stay per-identity. Written to a buffer the caller owns, so two roots can be derived on
    // one engine without the first silently becoming the second - a child scored against the wrong
    // root would differ only in resourceTestingDigest.
    // Uses currentANN as its working buffer, so it destroys whatever the engine was holding. Callers
    // derive a root and then score from it, which overwrites currentANN anyway.
    void deriveRootANN(const unsigned char* rootSeed, const unsigned char* pRandom2Pool, ANN& out)
    {
        deriveRootLut(rootSeed, pRandom2Pool);
        applyRootLut(currentANN);
        compact(currentANN, out);
    }

    // Ant colony: score a child by inheriting the parent's LUT and walking it with the child's own seeds
    unsigned int computeScoreFromParent(
        const ANN& parentANN, 
        const unsigned char* publicKey,
        const unsigned char* nonce,
        const unsigned char* anchorTickDigest,
        const unsigned char* pRandom2Pool)
    {
        // The canonical rule
        if (!isCanonicalAntNonce(nonce))
        {
            return INVALID_SCORE_VALUE;
        }

        // Get the ANN from parent, also init the new mutation starting point
        expand(parentANN, currentANN);
        deriveMutationSeeds(publicKey, nonce, anchorTickDigest, pRandom2Pool);

        // Both knobs are already in range: the check above is what puts them there.
        const unsigned int L = lutEntriesPerStep(nonce);
        const unsigned long long K = nonce[2];

        const unsigned int cur = score();

        return computeScoreFromCurrent(L, K, cur);
    }

    int getLastOutput(unsigned char* requestedOutput, int requestedSizeInBytes)
    {
        return 0;
    }
};

}
