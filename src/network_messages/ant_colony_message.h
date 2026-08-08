#pragma once

#include "common_def.h"

// Asks for the parents one identity can branch a child from. Scoped by pubkey because a child must
// name a parent in its OWN tree - validate() rejects anything else with RejectWrongTree -
// Operator-signed: the request payload is followed by SIGNATURE_SIZE bytes signed by
// operatorPublicKey. Signature only, with no monotonic nonce. A nonce exists to make an operator
// ACTION execute exactly once; replaying a read just costs a duplicate answer, while consuming the
// nonce would put a polling miner in contention with every other operator command.
//
// Paginated via fromIndex / nextIndex.
struct RequestAntMineableParents
{
    // Whose tree to report. Usually the caller's own.
    m256i pubkey;
    // Record index to resume scanning from (0 on the first call).
    unsigned int fromIndex;
    unsigned int padding;
    static constexpr unsigned char type()
    {
        return REQUEST_ANT_MINEABLE_PARENTS;
    }
};
static_assert(sizeof(RequestAntMineableParents) == 40, "RequestAntMineableParents unexpected size");

// Max mineable-parent entries returned per response. Miners page through the
// rest via the nextIndex cursor.
constexpr unsigned int ANT_MINEABLE_PARENTS_PER_RESPONSE = 64;

// Max records scanned per request
constexpr unsigned int ANT_MINEABLE_PARENTS_SCAN_BUDGET = 1024;

// One stored node of the requested identity's tree. selfTickOffset/selfSolutionIndexInTick is the
// ref a child sets as its own parentRef to extend this node; parentTickOffset/parentSolutionIndexInTick
// is this node's OWN parent - (0, 0xFFFFFFFF) means the root - so paging every node of a pubkey
// reconstructs the whole tree, edges included, without fetching any network bytes.
// The score is an error count, so smaller is better: a child must score strictly below score and
// strictly below siblingFloor (the best sibling more than N ticks earlier, computed for a child
// anchoring at the current tick).
struct AntMineableParent
{
    unsigned int selfTickOffset;
    unsigned int selfSolutionIndexInTick;
    unsigned int parentTickOffset;
    unsigned int parentSolutionIndexInTick;
    unsigned int score;
    unsigned int siblingFloor;
    unsigned int anchorTick;            // this node's own anchor tick number
    unsigned int depth;
};
static_assert(sizeof(AntMineableParent) == 32, "AntMineableParent unexpected size");

// Metadata header only; followed by count * AntMineableParent (count * itemSize
// bytes). itemSize lets the receiver validate the payload without hardcoding the
// entry size.
struct RespondAntMineableParentsHeader
{
    // Number of AntMineableParent entries that follow this header.
    unsigned int count;
    // Size in bytes of one AntMineableParent entry.
    unsigned int itemSize;
    // Resume cursor for the next request; 0 means no more records.
    unsigned int nextIndex;
    static constexpr unsigned char type()
    {
        return RESPOND_ANT_MINEABLE_PARENTS;
    }
};
static_assert(sizeof(RespondAntMineableParentsHeader) == 12, "RespondAntMineableParentsHeader unexpected size");

// The largest a mineable-parents response can be, the header followed by a full page of entries
struct AntMineableParentsResponse
{
    RespondAntMineableParentsHeader header;
    AntMineableParent items[ANT_MINEABLE_PARENTS_PER_RESPONSE];
};
static_assert(sizeof(AntMineableParentsResponse)
    == sizeof(RespondAntMineableParentsHeader)
     + ANT_MINEABLE_PARENTS_PER_RESPONSE * sizeof(AntMineableParent),
    "AntMineableParentsResponse must have no padding between the header and the items");

// RespondAntAnnStateHeader.status values.
constexpr unsigned char ANT_ANN_STATUS_OK = 0;        // ANN bytes follow the header
constexpr unsigned char ANT_ANN_STATUS_NOT_FOUND = 1; // parentRef has no record
constexpr unsigned char ANT_ANN_STATUS_IS_ROOT = 2;   // ROOT_REF; no ANN payload - miner derives its own per-identity root

// Operator-signed. The request payload is followed by SIGNATURE_SIZE bytes signed by
// operatorPublicKey.
struct RequestAntAnnState
{
    // Monotonic per-operator nonce: must exceed the last one the node accepted.
    unsigned long long everIncreasingNonce;
    unsigned int parentRefTickOffset;
    unsigned int parentRefSolutionIndexInTick;
    static constexpr unsigned char type()
    {
        return REQUEST_ANT_ANN_STATE;
    }
};
static_assert(sizeof(RequestAntAnnState) == 16, "RequestAntAnnState unexpected size");

// Metadata header only; when status is Ok or IsRoot, annSizeBytes bytes of packed
// ANN follow the header (annSizeBytes is 0 otherwise). Kept ANN-agnostic here to
// avoid a heavy include; the receiver uses annSizeBytes to read the trailing blob.
struct RespondAntAnnStateHeader
{
    unsigned int parentRefTickOffset;
    unsigned int parentRefSolutionIndexInTick;
    // Bytes of packed ANN that follow this header (0 unless status is Ok/IsRoot).
    unsigned int annSizeBytes;
    unsigned char status;
    unsigned char padding[3];
    static constexpr unsigned char type()
    {
        return RESPOND_ANT_ANN_STATE;
    }
};
static_assert(sizeof(RespondAntAnnStateHeader) == 16, "RespondAntAnnStateHeader unexpected size");

struct RequestAntEpochContext
{
    static constexpr unsigned char type()
    {
        return REQUEST_ANT_EPOCH_CONTEXT;
    }
};

// Per-epoch ant-colony parameters a miner needs to start building solutions:
// the score threshold, the freshness window, the epoch's root seed, and pool occupancy.
// The anchor digest is not included; a miner derives it from the standard
// protocol as K12(anchorTick || transactionDigest), with transactionDigest taken from the
// anchor tick's quorum votes (REQUEST_QUORUM_TICK).
#pragma pack(push, 1)
struct RespondAntEpochContext
{
    // The epoch-start spectrum digest
    m256i spectrumDigest;
    // score threshold for this epoch
    unsigned int threshold;
    // ANT_FRESHNESS_WINDOW_TICKS (N): publish within N of the anchor tick; siblings within N coexist
    unsigned int freshnessWindow;
    // accepted solutions so far this epoch
    unsigned int solutionCount;
    // free slots in the live ANN pool
    unsigned int freeAnnSlotsCount;
    // epoch this context is for
    unsigned short epoch;
    unsigned short padding;

    static constexpr unsigned char type()
    {
        return RESPOND_ANT_EPOCH_CONTEXT;
    }
};
#pragma pack(pop)
static_assert(sizeof(RespondAntEpochContext) == 52, "RespondAntEpochContext unexpected size");
