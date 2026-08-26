using namespace QPI;

#include "oracle_interfaces/EvmCommon.h"

/**
* Oracle interface "EvmLogRead" (see Price.h for general documentation about oracle interfaces).
*
* Generic cross-chain read of a SINGLE event log from a FINALIZED EVM receipt. Given
* (chainId, txHash, logIndex) the oracle machine returns that one log verbatim: the emitter
* address, its topics and its data, as RAW bytes. No event/ABI semantics are baked in here -
* this interface knows nothing about tokens, amounts or any specific event; the querying smart
* contract decodes the raw log itself.
*
* Determinism requirement (see Price.h): the oracle machine MUST only answer from receipts at or
* below the chain's finalized block (RESULT_TX_NOT_FINALIZED otherwise) and MUST zero all reply
* value fields except code on failure, so replies are byte-identical across computors.
*
* This is the generic read_evm_log service contract. Byte layout mirrors oracle-machine
* oracles/read_evm_log (48-byte query, 440-byte reply).
*/
struct EvmLogRead
{
    //-------------------------------------------------------------------------
    // Mandatory oracle interface definitions

    /// Oracle interface index
    static constexpr uint32 oracleInterfaceIndex = ORACLE_INTERFACE_INDEX;

    //--- Result codes returned in OracleReply.code. 0 means success; any non-zero value is a failure
    //    reason and implies all other reply fields are all-zero.
    static constexpr uint64 RESULT_SUCCESS                = 0;  ///< log found and finalized; reply fields valid
    static constexpr uint64 RESULT_BAD_QUERY              = 1;  ///< malformed query (zero tx hash, ...)
    static constexpr uint64 RESULT_CHAIN_UNSUPPORTED      = 2;  ///< chainId not served by this oracle
    static constexpr uint64 RESULT_TX_NOT_FOUND           = 3;  ///< no such transaction on the chain
    static constexpr uint64 RESULT_TX_NOT_FINALIZED       = 4;  ///< tx pending / reverted / not yet past finality
    static constexpr uint64 RESULT_LOG_INDEX_OUT_OF_RANGE = 5;  ///< logIndex >= number of logs in the receipt
    static constexpr uint64 RESULT_LOG_DATA_TOO_LARGE     = 6;  ///< log data exceeds the 256-byte reply capacity
    // add new result codes above this line
    //
    // Codes 3/4 are time-varying observations: treat as retriable, never proof of permanent
    // absence; near the finality boundary computors may split and the query just times out.
    // Retrying is the job of querier and not part of the oracle machine.

    /// Oracle query data / input to the oracle machine. Fixed 48 bytes.
    struct OracleQuery
    {
        /// EVM chain id (decimal), e.g. Evm::ChainId::ethereum (1) or Evm::ChainId::base.
        uint64 chainId;          // [0..8)

        /// Transaction hash to inspect (32-byte big-endian EVM tx hash).
        Evm::Bytes32 txHash;     // [8..40)

        /// Index of the log entry within the transaction receipt (receipt-local log index).
        uint64 logIndex;         // [40..48)
    };

    /// Oracle reply data / output of the oracle machine. Fixed 440 bytes.
    /// On failure (code != RESULT_SUCCESS) all other fields MUST be all-zero so the reply is
    /// canonical across computors.
    struct OracleReply
    {
        /// One of the RESULT_* codes above.
        uint64 code;                    // [0..8)

        /// Log emitter (20-byte address right-aligned into a 32-byte word, upper 12 bytes zero).
        Evm::Address address;           // [8..40)

        /// Number of topics present in this log (0..4).
        uint64 topicCount;              // [40..48)

        /// The log topics, raw 32-byte words, zero-padded beyond topicCount.
        Array<Evm::Bytes32, 4> topics;  // [48..176)

        /// Number of valid bytes in data (0..256).
        uint64 dataLen;                 // [176..184)

        /// The log data, raw bytes, zero-padded beyond dataLen.
        Array<uint8, 256> data;         // [184..440)
    };

    /// Return query fee. Cross-chain EVM reads are comparatively expensive.
    static sint64 getQueryFee(const OracleQuery& query)
    {
        return 1000;
    }

    //-------------------------------------------------------------------------
    // Optional: convenience features for contracts using the oracle interface

    /// True if the reply carries a usable raw-log attestation.
    static bool replyIsValid(const OracleReply& reply)
    {
        return reply.code == RESULT_SUCCESS;
    }
};

static_assert(sizeof(EvmLogRead::OracleQuery) == 48, "EvmLogRead::OracleQuery must be 48 bytes");
static_assert(sizeof(EvmLogRead::OracleReply) == 440, "EvmLogRead::OracleReply must be 440 bytes");
