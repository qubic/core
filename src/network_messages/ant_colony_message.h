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
struct RequestAntIdentityTree
{
    // Whose tree to report. Usually the caller's own.
    m256i pubkey;
    // Record index to resume scanning from (0 on the first call).
    unsigned int fromIndex;
    unsigned int padding;
    static constexpr unsigned char type()
    {
        return REQUEST_ANT_IDENTITY_TREE;
    }
};
static_assert(sizeof(RequestAntIdentityTree) == 40, "RequestAntIdentityTree unexpected size");

// A pool miner hands its computor a solution over BroadcastMessage(MESSAGE_TYPE_ANT_SOLUTION); this
// is the payload that follows the header.
struct AntSolutionBroadcastPayload
{
    unsigned int parentTick;            // ABSOLUTE
    unsigned int parentSolutionIndexInTick;
    unsigned int anchorTick;            // ABSOLUTE
    unsigned int claimedScore;
    m256i nonce;
};
static_assert(sizeof(AntSolutionBroadcastPayload) == 48, "AntSolutionBroadcastPayload unexpected size");

// Max identity-tree nodes returned per response. Miners page through the
// rest via the nextIndex cursor.
constexpr unsigned int ANT_IDENTITY_TREE_NODES_PER_RESPONSE = 64;

// Max records scanned per request
constexpr unsigned int ANT_IDENTITY_TREE_SCAN_BUDGET = 1024;

// One stored node of the requested identity's tree. selfTick/selfSolutionIndexInTick is the
// ref a child sets as its own parentRef to extend this node; parentTick/parentSolutionIndexInTick
// is this node's OWN parent - (0, 0xFFFFFFFF) means the root - so paging every node of a pubkey
// reconstructs the whole tree, edges included, without fetching any network bytes.
// The score is an error count, so smaller is better: a child must score strictly below score.
// childCount is how many children this node already holds, capped at ANT_MAX_CHILDREN_PER_PARENT; at
// the cap it takes no more children (0 for the cap means unbound).
struct AntIdentityTreeNode
{
    unsigned int selfTick;
    unsigned int selfSolutionIndexInTick;
    unsigned int parentTick;
    unsigned int parentSolutionIndexInTick;
    unsigned int score;
    unsigned int childCount;
    unsigned int anchorTick;            // this node's own anchor tick number (ABSOLUTE)
    unsigned int depth;
};
static_assert(sizeof(AntIdentityTreeNode) == 32, "AntIdentityTreeNode unexpected size");

// Metadata header only; followed by count * AntIdentityTreeNode (count * itemSize
// bytes). itemSize lets the receiver validate the payload without hardcoding the
// entry size.
struct RespondAntIdentityTreeHeader
{
    // Number of AntIdentityTreeNode entries that follow this header.
    unsigned int count;
    // Size in bytes of one AntIdentityTreeNode entry.
    unsigned int itemSize;
    // Resume cursor for the next request; 0 means no more records.
    unsigned int nextIndex;
    static constexpr unsigned char type()
    {
        return RESPOND_ANT_IDENTITY_TREE;
    }
};
static_assert(sizeof(RespondAntIdentityTreeHeader) == 12, "RespondAntIdentityTreeHeader unexpected size");

// The largest an identity-tree response can be, the header followed by a full page of entries
struct AntIdentityTreeResponse
{
    RespondAntIdentityTreeHeader header;
    AntIdentityTreeNode items[ANT_IDENTITY_TREE_NODES_PER_RESPONSE];
};
static_assert(sizeof(AntIdentityTreeResponse)
    == sizeof(RespondAntIdentityTreeHeader)
     + ANT_IDENTITY_TREE_NODES_PER_RESPONSE * sizeof(AntIdentityTreeNode),
    "AntIdentityTreeResponse must have no padding between the header and the items");

// RespondAntParentAnnHeader.status values.
constexpr unsigned char ANT_PARENT_ANN_STATUS_OK = 0;        // ANN bytes follow the header
constexpr unsigned char ANT_PARENT_ANN_STATUS_NOT_FOUND = 1; // parentRef has no record
constexpr unsigned char ANT_PARENT_ANN_STATUS_IS_ROOT = 2;   // ROOT_REF; no ANN payload - miner derives the shared epoch root

// ONE tree node's stored network, named by parentRef - the ANN state a miner mutates to extend
// that node. The tree itself is listed by the identity-tree query; this fetches the material for a
// single chosen parent.
// Operator-signed: the request payload is followed by SIGNATURE_SIZE bytes signed by
// operatorPublicKey
struct RequestAntParentAnn
{
    unsigned int parentRefTick;
    unsigned int parentRefSolutionIndexInTick;
    static constexpr unsigned char type()
    {
        return REQUEST_ANT_PARENT_ANN;
    }
};
static_assert(sizeof(RequestAntParentAnn) == 8, "RequestAntParentAnn unexpected size");

// Metadata header, when status is Ok, annSizeBytes bytes of CANONICAL ANN follow it - one trit per
// byte, the form the scorer consumes, so the receiver does no unpacking. annSizeBytes is 0 for every
// other status. Kept ANN-agnostic here to avoid a heavy include; the receiver reads the trailing
// blob by annSizeBytes.
struct RespondAntParentAnnHeader
{
    unsigned int parentRefTick;
    unsigned int parentRefSolutionIndexInTick;
    // Bytes of canonical ANN that follow this header: ANN LUT size when status is Ok, 0 for every other
    // status.
    unsigned int annSizeBytes;
    unsigned char status;
    unsigned char padding[3];
    static constexpr unsigned char type()
    {
        return RESPOND_ANT_PARENT_ANN;
    }
};
static_assert(sizeof(RespondAntParentAnnHeader) == 16, "RespondAntParentAnnHeader unexpected size");

struct RequestAntEpochContext
{
    static constexpr unsigned char type()
    {
        return REQUEST_ANT_EPOCH_CONTEXT;
    }
};

// Per-epoch ant-colony parameters a miner needs to start building solutions:
// the score threshold, the freshness window, the epoch's root seed, pool occupancy,
// and the per-parent child cap.
// The anchor digest is not included; a miner derives it from the anchor tick's TickData
// (REQUEST_TICK_DATA): transactionDigest = K12(TickData), then K12(anchorTick || transactionDigest).
#pragma pack(push, 1)
struct RespondAntEpochContext
{
    // The epoch-start spectrum digest
    m256i spectrumDigest;
    // confirm its task file matches the one the node scores against.
    m256i topologyHash;
    m256i dataHash;
    // score threshold for this epoch
    unsigned int threshold;
    // ANT_PUBLISH_WINDOW_TICKS: publish within this many ticks of the anchor.
    unsigned int freshnessWindow;
    // accepted solutions so far this epoch
    unsigned int solutionCount;
    // free slots in the live ANN pool
    unsigned int freeAnnSlotsCount;
    // ANT_MAX_CHILDREN_PER_PARENT: max children a parent takes; 0 = unbound
    unsigned int maxChildrenPerParent;
    // epoch this context is for
    unsigned short epoch;
    unsigned short padding;

    static constexpr unsigned char type()
    {
        return RESPOND_ANT_EPOCH_CONTEXT;
    }
};
#pragma pack(pop)
static_assert(sizeof(RespondAntEpochContext) == 120, "RespondAntEpochContext unexpected size");
