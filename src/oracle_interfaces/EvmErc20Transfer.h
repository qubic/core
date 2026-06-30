using namespace QPI;

#include "oracle_interfaces/EvmCommon.h"

/**
* Oracle interface "EvmErc20Transfer" (see Price.h for general documentation about oracle interfaces).
*
* Cross-chain read of a single ERC20 token transfer (deposit) contained in a given EVM transaction.
*
* Given a transaction hash and chain id (plus optional constraints), the oracle reads the transaction
* on the target EVM chain and reports which ERC20 token was transferred and how much. The intended use
* is bridge/deposit verification: "did tx X on chain Y deposit some ERC20 token to address Z, and how
* much?".
*
* Determinism requirement (see Price.h): consecutive queries with the same OracleQuery must always
* yield the exact same OracleReply, because each computor queries the oracle independently and a quorum
* must agree on the exact reply bytes. A transaction is immutable once finalized, so the oracle machine
* MUST only answer for transactions that are final (enough confirmations / past the chain's finality),
* otherwise a re-org could make computors disagree. Near the chain head the OM should treat the tx as
* not-yet-confirmed (RESULT_TX_NOT_CONFIRMED).
*
* Extending EVM oracles later: add a new interface header that also includes EvmCommon.h and reuses
* Evm::Address / Evm::Uint256 / Evm::ChainId, then register it in oracle_interfaces_def.h with a new
* index. Keep one interface per read kind so each has a fixed OracleQuery/OracleReply layout.
*/
struct EvmErc20Transfer
{
    //-------------------------------------------------------------------------
    // Mandatory oracle interface definitions

    /// Oracle interface index
    static constexpr uint32 oracleInterfaceIndex = ORACLE_INTERFACE_INDEX;

    //--- Constraint flags: bitmask in OracleQuery.constraintFlags selecting which optional
    //    constraints are active. A constraint whose bit is 0 is ignored (its fields need not be set).
    static constexpr uint64 CONSTRAINT_NONE         = 0;
    static constexpr uint64 CONSTRAINT_TIME_RANGE   = 1ULL << 0;  ///< check block timestamp in [minTimestamp, maxTimestamp]
    static constexpr uint64 CONSTRAINT_BLOCK_HEIGHT = 1ULL << 1;  ///< check block height in [minBlockHeight, maxBlockHeight]
    static constexpr uint64 CONSTRAINT_SOURCE       = 1ULL << 2;  ///< check token sender (transfer "from") equals sourceAddress
    static constexpr uint64 CONSTRAINT_DEST         = 1ULL << 3;  ///< check token recipient (transfer "to") equals destAddress

    //--- Result codes returned in OracleReply.code. 0 means success; any non-zero value is a failure
    //    reason and implies tokenAddress and amount are all-zero. New reasons may be appended later.
    static constexpr uint64 RESULT_SUCCESS                  = 0;  ///< all constraints passed; exactly one ERC20 transfer found
    static constexpr uint64 RESULT_BAD_QUERY                = 1;  ///< malformed query (zero tx hash, conflicting fields, ...)
    static constexpr uint64 RESULT_CHAIN_UNSUPPORTED        = 2;  ///< chainId not served by this oracle
    static constexpr uint64 RESULT_TX_NOT_FOUND             = 3;  ///< no such transaction on the chain
    static constexpr uint64 RESULT_TX_NOT_CONFIRMED         = 4;  ///< tx pending / reverted / not yet final
    static constexpr uint64 RESULT_NO_ERC20_TRANSFER        = 5;  ///< tx contains no matching ERC20 transfer
    static constexpr uint64 RESULT_MULTIPLE_ERC20_TRANSFER  = 6;  ///< more than one matching ERC20 transfer (ambiguous)
    static constexpr uint64 RESULT_CONSTRAINT_TIME_RANGE    = 7;  ///< block timestamp outside requested range
    static constexpr uint64 RESULT_CONSTRAINT_BLOCK_HEIGHT  = 8;  ///< block height outside requested range
    static constexpr uint64 RESULT_CONSTRAINT_SOURCE        = 9;  ///< token sender does not match sourceAddress
    static constexpr uint64 RESULT_CONSTRAINT_DEST          = 10; ///< token recipient does not match destAddress
    // add new result codes above this line

    /// Oracle query data / input to the oracle machine
    struct OracleQuery
    {
        /// Transaction hash to inspect (32-byte big-endian EVM tx hash).
        Evm::Bytes32 txHash;

        /// EVM chain id (decimal), e.g. Evm::ChainId::ethereum (1) or Evm::ChainId::bsc (56).
        uint64 chainId;

        /// Bitmask of active optional constraints (see CONSTRAINT_* above). 0 = no extra constraints.
        uint64 constraintFlags;

        /// CONSTRAINT_TIME_RANGE: inclusive block-timestamp range the tx must fall within.
        DateAndTime minTimestamp;
        DateAndTime maxTimestamp;

        /// CONSTRAINT_BLOCK_HEIGHT: inclusive block-height range the tx must fall within.
        uint64 minBlockHeight;
        uint64 maxBlockHeight;

        /// CONSTRAINT_SOURCE: required token sender (ERC20 transfer "from"), ABI-padded 32-byte address.
        Evm::Address sourceAddress;

        /// CONSTRAINT_DEST: required token recipient (ERC20 transfer "to"), ABI-padded 32-byte address.
        Evm::Address destAddress;
    };

    /// Oracle reply data / output of the oracle machine.
    /// On failure (code != RESULT_SUCCESS) tokenAddress and amount MUST be all-zero so the reply is
    /// canonical across computors.
    struct OracleReply
    {
        /// One of the RESULT_* codes above.
        uint64 code;

        /// ERC20 token contract address (ABI-padded 32-byte). Valid only if code == RESULT_SUCCESS.
        Evm::Address tokenAddress;

        /// Transferred amount as 32-byte big-endian uint256. Valid only if code == RESULT_SUCCESS.
        Evm::Uint256 amount;
    };

    /// Return query fee. Cross-chain EVM reads are comparatively expensive.
    static sint64 getQueryFee(const OracleQuery& query)
    {
        return 1000;
    }

    //-------------------------------------------------------------------------
    // Optional: convenience features for contracts using the oracle interface

    /// True if the reply carries a usable token address + amount.
    static bool replyIsValid(const OracleReply& reply)
    {
        return reply.code == RESULT_SUCCESS;
    }
};
