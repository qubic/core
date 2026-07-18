#pragma once

#include "common_def.h"

// Query the node for the current (approximate) revenue scores of all computors.
// The node already classifies every transaction into the multi-dimension revenue
// vectors and accumulates them; this returns the reduced per-computor score arrays
// so a consumer can compute revenue without reprocessing transactions.
//
// The scores reflect the node's live in-memory state at the returned `tick`. They are
// approximate mid-epoch (the sliding-window tail is not finalized until end of epoch)
// and exact only at epoch end.
struct RequestRevenueData
{
    static constexpr unsigned char type()
    {
        return NetworkMessageType::REQUEST_REVENUE_DATA;
    }
};

#pragma pack(push, 1)
struct RespondRevenueData
{
    static constexpr unsigned char type()
    {
        return NetworkMessageType::RESPOND_REVENUE_DATA;
    }

    unsigned int tick;          // current tick the scores correspond to (epoch/initialTick via RequestSystemInfo)
    unsigned short dogeK;       // REVENUE_DOGE_K, so the consumer's math tracks the active value
    long long ipc;              // REVENUE_IPC, the per-computor revenue cap

    unsigned long long txScore[NUMBER_OF_COMPUTORS];
    unsigned long long oracleScore[NUMBER_OF_COMPUTORS];
    unsigned long long dogeScore[NUMBER_OF_COMPUTORS];
};
#pragma pack(pop)

static_assert(sizeof(RespondRevenueData) == 4 + 2 + 8 + 3 * 8 * NUMBER_OF_COMPUTORS,
    "Something is wrong with the struct size of RespondRevenueData.");
