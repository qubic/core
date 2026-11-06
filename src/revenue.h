#pragma once

#include "platform/memory.h"
#include "platform/assert.h"
#include "network_messages/common_def.h"
#include "contracts/math_lib.h"
#include "vote_counter.h"
#include "public_settings.h"

// 1 = DOGE has no effect on revenue (dogeFactor forced to SCALE for all).
// 0 = normal DOGE.
#define REVENUE_DOGE_DISABLED 1

// Get the lower bound that start to separate the QUORUM region of score
unsigned long long getQuorumScore(
    const unsigned long long* score,
    const unsigned int numberOfComputors = NUMBER_OF_COMPUTORS,
    const unsigned int quorum = QUORUM)
{
    ASSERT((quorum > 0) && (quorum <= QUORUM));
    if ((quorum == 0) || (quorum > QUORUM))
    {
        return 1;
    }

    unsigned long long sortedScore[QUORUM + 1];
    // Sort revenue scores to get lowest score of quorum
    setMem(sortedScore, (quorum + 1) * sizeof(unsigned long long), 0);
    for (unsigned short computorIndex = 0; computorIndex < numberOfComputors; computorIndex++)
    {
        sortedScore[quorum] = score[computorIndex];
        unsigned int i = quorum;
        while (i
            && sortedScore[i - 1] < sortedScore[i])
        {
            const unsigned long long tmp = sortedScore[i - 1];
            sortedScore[i - 1] = sortedScore[i];
            sortedScore[i--] = tmp;
        }
    }
    if (!sortedScore[quorum - 1])
    {
        sortedScore[quorum - 1] = 1;
    }
    return sortedScore[quorum - 1];
}

// This function will sort the score take the 450th score at reference score
// Score greater than or equal this will be assigned as scalingThreshold value
// Score is zero will become zero
// Score lower than this value will be scaled at score * scalingThreshold / value
static void computeRevFactor(
    const unsigned long long* score,
    const unsigned long long scalingThreshold,
    unsigned long long* outputScoreFactor,
    const unsigned int numberOfComputors = NUMBER_OF_COMPUTORS,
    const unsigned int quorum = QUORUM)
{
    ASSERT(scalingThreshold > 0);

    // Sort revenue scores to get lowest score of quorum
    unsigned long long quorumScore = getQuorumScore(score, numberOfComputors, quorum);

    for (unsigned int computorIndex = 0; computorIndex < numberOfComputors; computorIndex++)
    {
        unsigned long long scoreFactor = 0;
        if (score[computorIndex] == 0)
        {
            scoreFactor = 0;
        }
        else if (score[computorIndex] >= quorumScore)
        {
            scoreFactor = scalingThreshold;
        }
        else // Lower than QUORUM score.
        {
            // Checking overflow. We don't expect falling
            ASSERT(0xFFFFFFFFFFFFFFFFULL / scalingThreshold >= score[computorIndex]);
            scoreFactor = scalingThreshold * score[computorIndex] / quorumScore;
        }

        outputScoreFactor[computorIndex] = scoreFactor;
    }
}

static constexpr unsigned int REVENUE_HALF_WINDOW = NUMBER_OF_COMPUTORS - 1;  // 675
static constexpr unsigned int REVENUE_WINDOW_SIZE = 2 * REVENUE_HALF_WINDOW + 1;      // 1351
static_assert(MAX_NUMBER_OF_TICKS_PER_EPOCH >= REVENUE_WINDOW_SIZE,
    "MAX_NUMBER_OF_TICKS_PER_EPOCH must be >= REVENUE_WINDOW_SIZE; epochs shorter than one window produce zero revenue.");
static constexpr unsigned long long REVENUE_SCALE = 1024;
static constexpr long long REVENUE_IPC = ISSUANCE_RATE / NUMBER_OF_COMPUTORS;

static constexpr unsigned int REVENUE_DOGE_K = 2;
static constexpr unsigned int REVENUE_CONTRACT_DIMS = contractCount;
static constexpr unsigned int REVENUE_TX_DIM = NUMBER_OF_COMPUTORS + REVENUE_CONTRACT_DIMS + 1;

static constexpr unsigned int m_SRC_COMPUTOR = 1;
static constexpr unsigned int m_CONTRACT = 1;
static constexpr unsigned int m_TRANSFER = 1;
static constexpr unsigned int REVENUE_M_MAX = 1;
static constexpr unsigned long long REVENUE_TX_WINDOWSUM_MAX =
(unsigned long long)NUMBER_OF_TRANSACTIONS_PER_TICK * REVENUE_WINDOW_SIZE;
static_assert(REVENUE_DOGE_K >= 1 && REVENUE_DOGE_K <= 4, "");
static_assert((unsigned long long)REVENUE_IPC* REVENUE_SCALE* REVENUE_SCALE* REVENUE_SCALE
    <= 0xFFFFFFFFFFFFFFFFULL, "IPC*S^3 overflow");
static_assert((unsigned long long)REVENUE_TX_DIM* REVENUE_M_MAX* REVENUE_M_MAX
    <= 0xFFFFFFFFFFFFFFFFULL / (REVENUE_TX_WINDOWSUM_MAX * REVENUE_TX_WINDOWSUM_MAX), "L2 sumsq overflow");

// Struct record all data so that we can do the offline analysis
struct EpochRevenueData
{
    // Header
    unsigned int initialTick;
    unsigned int totalTicks;                                        // system.tick - system.initialTick

    // Per-computor arrays (NUMBER_OF_COMPUTORS entries each)
    unsigned long long oracleScore[NUMBER_OF_COMPUTORS];            // oracle commit/reveal revenue points
    unsigned long long dogeMiningScore[NUMBER_OF_COMPUTORS];        // DOGE merged-mining shares (raw)

    // Per-tick TX counts: total, 3 categories and tick leader
    unsigned short perTickTxCount[MAX_NUMBER_OF_TICKS_PER_EPOCH];           // total of txs exclude the tx of tick leader
    unsigned short perTickProtocolTxCount[MAX_NUMBER_OF_TICKS_PER_EPOCH];   // protocol txs (votes, mining, oracle, etc.)
    unsigned short perTickContractTxCount[MAX_NUMBER_OF_TICKS_PER_EPOCH];   // contract txs
    unsigned short perTickOtherTxCount[MAX_NUMBER_OF_TICKS_PER_EPOCH];   // mainly: user transfers
    unsigned short perTickTxTickLeaderCount[MAX_NUMBER_OF_TICKS_PER_EPOCH];           // total of txs
};

static EpochRevenueData gEpochRevenueData;

static constexpr unsigned long long revenueDataPerTickArrays = 5;
static constexpr unsigned long long revenueDataFixedSize =
    sizeof(EpochRevenueData) - revenueDataPerTickArrays * MAX_NUMBER_OF_TICKS_PER_EPOCH * sizeof(unsigned short);
static_assert(revenueDataFixedSize == 8 + 2ULL * NUMBER_OF_COMPUTORS * sizeof(unsigned long long),
    "EpochRevenueData has unexpected padding; the snapshot re-layout below relies on its exact layout");

// Restore a revenue snapshot saved with a different MAX_NUMBER_OF_TICKS_PER_EPOCH.
static bool remapEpochRevenueData(const unsigned char* saved, unsigned long long savedSize, EpochRevenueData& out)
{
    constexpr unsigned long long stride = revenueDataPerTickArrays * sizeof(unsigned short);
    if (savedSize < revenueDataFixedSize || (savedSize - revenueDataFixedSize) % stride)
    {
        return false;
    }
    const unsigned long long savedTicks = (savedSize - revenueDataFixedSize) / stride;
    if (savedTicks % NUMBER_OF_COMPUTORS)
    {
        return false;
    }

    // static_assert pins the sizes; this pins the field order the copies below rely on
    ASSERT((const unsigned char*)out.perTickTxCount == (const unsigned char*)&out + revenueDataFixedSize);

    copyMem(&out, saved, revenueDataFixedSize);

    const unsigned long long copyTicks = savedTicks < MAX_NUMBER_OF_TICKS_PER_EPOCH ? savedTicks : MAX_NUMBER_OF_TICKS_PER_EPOCH;
    unsigned short* dst[revenueDataPerTickArrays] = {
        out.perTickTxCount, out.perTickProtocolTxCount, out.perTickContractTxCount,
        out.perTickOtherTxCount, out.perTickTxTickLeaderCount };
    const unsigned char* src = saved + revenueDataFixedSize;
    for (unsigned long long i = 0; i < revenueDataPerTickArrays; i++)
    {
        copyMem(dst[i], src + i * savedTicks * sizeof(unsigned short), copyTicks * sizeof(unsigned short));
        setMem(dst[i] + copyTicks, (MAX_NUMBER_OF_TICKS_PER_EPOCH - copyTicks) * sizeof(unsigned short), 0);
    }
    return true;
}

static unsigned int gTxObservation[REVENUE_TX_DIM];
struct MultiDimRevenue
{
    // Header
    unsigned int initialTick;
    unsigned int totalTicks;                                        // system.tick - system.initialTick

    unsigned short revenueRing[REVENUE_WINDOW_SIZE * REVENUE_TX_DIM];        // last W ticks 
    unsigned short revenueHead[2 * REVENUE_HALF_WINDOW * REVENUE_TX_DIM];    // first 2H ticks
    unsigned long long windowSum[REVENUE_TX_DIM];                         // running col sums
    unsigned long long txScore[NUMBER_OF_COMPUTORS];                      // accumulated L2 TX score
    long long revenue[NUMBER_OF_COMPUTORS];                      // final output
};
static MultiDimRevenue gMultiDimRevenue;

static unsigned short* ringRow(unsigned int t)
{
    return gMultiDimRevenue.revenueRing + (unsigned long long)(t % REVENUE_WINDOW_SIZE) * REVENUE_TX_DIM;
}

static unsigned short* headRow(unsigned int t) 
{ 
    return gMultiDimRevenue.revenueHead + (unsigned long long)t * REVENUE_TX_DIM;
}

// per-dimension multiplier by category
static unsigned int revenueMul(unsigned int d)
{
    if (d < NUMBER_OF_COMPUTORS)
    {
        return m_SRC_COMPUTOR;
    }
    if (d < NUMBER_OF_COMPUTORS + REVENUE_CONTRACT_DIMS)
    {
        return m_CONTRACT;
    }
    return m_TRANSFER;
}

// Compute the score at tickOffset and accumulate it into gMultiDimRevenue.txScore
// windowSum is accumulated windows around tickOffset
static void finalizeTickScore(unsigned int tickOffset, const unsigned short* observed, const unsigned long long* windowSum)
{
    unsigned long long sumDef = 0;
    unsigned long long sumCap = 0;
    for (unsigned int d = 0; d < REVENUE_TX_DIM; d++)
    {
        const unsigned long long ws = windowSum[d];
        const unsigned long long wo = (unsigned long long)REVENUE_WINDOW_SIZE * observed[d];
        const unsigned long long m = revenueMul(d);
        const unsigned long long deficit = (wo >= ws) ? 0ULL : m * (ws - wo);
        const unsigned long long cap = m * ws;
        sumDef += deficit * deficit;
        sumCap += cap * cap;
    }

    unsigned long long tickScore;
    if (sumCap == 0)
    {
        tickScore = REVENUE_SCALE;
    }
    else
    {
        const unsigned long long pen = math_lib::irootK64<2>(sumDef);
        const unsigned long long cap = math_lib::irootK64<2>(sumCap);
        tickScore = REVENUE_SCALE - (REVENUE_SCALE * pen) / cap;
    }
    const unsigned int leader = (gMultiDimRevenue.initialTick + tickOffset) % NUMBER_OF_COMPUTORS;
    gMultiDimRevenue.txScore[leader] += tickScore;
}

static void revenueOnTick(unsigned int tickOffset, unsigned int* obs)
{
    // slot currently holds tick (t - W)
    unsigned short* slot = ringRow(tickOffset);

    // evict the tick that currently occupies this ring slot (tick t-W), if any
    if (tickOffset >= REVENUE_WINDOW_SIZE)
    {
        for (unsigned int d = 0; d < REVENUE_TX_DIM; d++) 
        { 
            gMultiDimRevenue.windowSum[d] -= slot[d];
        }
    }
    // write this tick into the ring + windowSum (and into the frozen head for the first 2H ticks)
    const bool inHead = (tickOffset < 2u * REVENUE_HALF_WINDOW);
    unsigned short* h = inHead ? headRow(tickOffset) : nullptr;
    for (unsigned int d = 0; d < REVENUE_TX_DIM; d++)
    {
        const unsigned short v = (unsigned short)obs[d];
        slot[d] = v;
        gMultiDimRevenue.windowSum[d] += v;
        if (inHead) 
        { 
            h[d] = v;
        }
    }

    // now windowSum covers ticks [t-2H, t] (W entries). Finalize the centered INTERIOR tick c = t-H.
    if (tickOffset >= 2u * REVENUE_HALF_WINDOW)
    {
        const unsigned int c = tickOffset - REVENUE_HALF_WINDOW;      // interior: c in [H, N-1-H]; window [c-H,c+H]=[t-2H,t], no wrap
        finalizeTickScore(c, ringRow(c), gMultiDimRevenue.windowSum);
    }
}

// Get an observation at x. 
static const unsigned short* obsAt(unsigned int x)
{
    return (x < 2U * REVENUE_HALF_WINDOW) ? headRow(x) : ringRow(x);
}

static unsigned long long edgeWindowSum[REVENUE_TX_DIM];
// Function handle scoring at the ticks at the epoch boundary (tick < REVENUE_HALF_WINDOW or tick >= lastTick - REVENUE_HALF_WINDOW) 
// which we need to do circular window wraping around
static void scoreEpochEdgeTicks(unsigned int tickEdge)
{
    const unsigned int H = REVENUE_HALF_WINDOW;
    if (tickEdge < REVENUE_WINDOW_SIZE) 
    { 
        return;
    }

    // Prepare the windows at tick = N - REVENUE_HALF_WINDOW
    setMem(edgeWindowSum, sizeof(edgeWindowSum), 0);
    for (unsigned int x = tickEdge - 2 * H; x <= tickEdge - 1; x++)
    {
        const unsigned short* row = ringRow(x);
        for (unsigned int d = 0; d < REVENUE_TX_DIM; d++)
        {
            edgeWindowSum[d] += row[d];
        }
    }

    // The last element of windows is circular to the first tick
    const unsigned short* row = headRow(0);
    for (unsigned int d = 0; d < REVENUE_TX_DIM; d++)
    {
        edgeWindowSum[d] += row[d];
    }

    // Slide the windows start at tick = N - REVENUE_HALF_WINDOW
    unsigned int c = tickEdge - H;
    for (unsigned int step = 0; step < 2u * H; step++)
    {
        finalizeTickScore(c, obsAt(c), edgeWindowSum);
        // Last tick. Everything is done, sliding window don't need to be updated
        if (step + 1u == 2u * H) 
        { 
            break;
        }

        // Determine the enterring and leaving data
        const unsigned int leaving = (c >= H) ? (c - H) : (c + tickEdge - H);
        const unsigned int entering = (c + H + 1u < tickEdge) ? (c + H + 1u) : (c + H + 1u - tickEdge);
        const unsigned short* lv = obsAt(leaving);
        const unsigned short* en = obsAt(entering);
        for (unsigned int d = 0; d < REVENUE_TX_DIM; d++) 
        { 
            edgeWindowSum[d] = edgeWindowSum[d] - lv[d] + en[d];
        }
        c = (c + 1u < tickEdge) ? (c + 1u) : 0u;
    }
}

static void computeMultiDimRevenue()
{
    static unsigned long long txFactor[NUMBER_OF_COMPUTORS];
    static unsigned long long oracleFactor[NUMBER_OF_COMPUTORS];
    static unsigned long long dogeFactor[NUMBER_OF_COMPUTORS];

    // finish boundary ticks
    scoreEpochEdgeTicks(gEpochRevenueData.totalTicks);
    computeRevFactor(gMultiDimRevenue.txScore, REVENUE_SCALE, txFactor);

    // oracle factor (READ-ONLY shared input) + epoch-GLOBAL no-activity valve (an idle factor must
    // NOT zero all 676 -> arbitrator)
    bool anyOracle = false;
    for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++) 
    { 
        if (gEpochRevenueData.oracleScore[i]) 
        { 
            anyOracle = true;
            break; 
        } 
    }
    if (anyOracle)
    { 
        computeRevFactor(gEpochRevenueData.oracleScore, REVENUE_SCALE, oracleFactor);
    }
    else 
    { 
        for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++) 
        { 
            oracleFactor[i] = REVENUE_SCALE; 
        } 
    }

    // DOGE factor (READ-ONLY shared input) + the SAME global valve (required by the hard gate)
    bool anyDoge = false;
    for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++) 
    { 
        if (gEpochRevenueData.dogeMiningScore[i]) 
        { 
            anyDoge = true; 
            break; 
        } 
    }
    if (!REVENUE_DOGE_DISABLED && anyDoge) 
    { 
        computeRevFactor(gEpochRevenueData.dogeMiningScore, REVENUE_SCALE, dogeFactor);
    }
    else 
    {
        for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
        {
            dogeFactor[i] = REVENUE_SCALE;
        } 
    }

    const unsigned long long sPowKm1 = math_lib::ipow(REVENUE_SCALE, REVENUE_DOGE_K - 1);
    for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
    {
        const unsigned long long dogeRootScaled = math_lib::irootK64<REVENUE_DOGE_K>(dogeFactor[i] * sPowKm1); // [0,S]
        const unsigned long long num = (unsigned long long)REVENUE_IPC * txFactor[i] * oracleFactor[i] * dogeRootScaled;
        gMultiDimRevenue.revenue[i] = (long long)(num / (REVENUE_SCALE * REVENUE_SCALE * REVENUE_SCALE));
    }
}

