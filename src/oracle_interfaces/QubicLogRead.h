using namespace QPI;

/**
* Oracle interface "QubicLogRead" (see Price.h for general documentation about oracle interfaces).
*
* Generic read of a SINGLE Qubic log event: given (tick, txHash, logId) the oracle machine
* returns that one log verbatim - its type, emitting contract index and raw body bytes. logId is
* the GLOBAL log id (counted from epoch's first log event onward), matched against the logId
* of each log in the transaction's receipt. No semantics are baked in; the querying smart
* contract decodes the raw bytes itself.
*
* A CONTRACT_INFORMATION_MESSAGE (logType 6) can only be emitted by code running inside a
* contract, and the core stamps the emitting contractIndex itself, so (logType, contractIndex)
* authenticates the emitter the same way (address, topic0) does for an EVM log.
*
* Determinism (see Price.h): a log is immutable history - any node that executed the tick serves
* byte-identical bytes. The oracle machine must only answer executed transactions and must zero
* all value fields except code on failure.
*
* Served by oracle-machine oracles/read_qubic_log (reads per-operator bob nodes, all configured
* bobs must agree byte-for-byte).
*/
struct QubicLogRead
{
    //-------------------------------------------------------------------------
    // Mandatory oracle interface definitions

    /// Oracle interface index
    static constexpr uint32 oracleInterfaceIndex = ORACLE_INTERFACE_INDEX;

    //--- Result codes returned in OracleReply.code. 0 means success; any non-zero value is a failure
    //    reason and implies all other reply fields are all-zero.
    static constexpr uint64 RESULT_SUCCESS                = 0;  ///< log found; reply fields valid
    static constexpr uint64 RESULT_BAD_QUERY              = 1;  ///< malformed query (zero tx hash or zero tick)
    static constexpr uint64 RESULT_TX_NOT_FOUND           = 2;  ///< no such transaction, also include the too-old transactions (usually 2 month old transactions can't be found efficiently anymore)
    static constexpr uint64 RESULT_TX_NOT_EXECUTED        = 3;  ///< tx pending or failed (no logs exist)
    static constexpr uint64 RESULT_TICK_MISMATCH          = 4;  ///< tx exists but not in the queried tick
    static constexpr uint64 RESULT_LOG_NOT_FOUND          = 5;  ///< no log with the queried logId in this tx
    static constexpr uint64 RESULT_LOG_DATA_TOO_LARGE     = 6;  ///< log body exceeds the 256-byte reply capacity
    // add new result codes above this line
    //
    // Codes 2/3 are time-varying observations: treat as retriable, never proof of permanent
    // absence; around the execution boundary computors may split and the query just times out.

    /// Oracle query data / input to the oracle machine. Fixed 48 bytes (mirrors EvmLogRead).
    struct OracleQuery
    {
        /// Tick the transaction was executed in.
        uint64 tick;             // [0..8)

        /// Transaction hash (32-byte digest).
        Array<uint8, 32> txHash; // [8..40)

        /// Global log id of the requested log event (counted from epoch's first log).
        uint64 logId;            // [40..48)
    };

    /// Oracle reply data / output of the oracle machine. Fixed 288 bytes.
    /// On failure (code != RESULT_SUCCESS) all other fields MUST be all-zero.
    struct OracleReply
    {
        /// One of the RESULT_* codes above.
        uint64 code;             // [0..8)

        /// Emitting contract index for contract-emitted log types; 0 otherwise.
        uint64 contractIndex;    // [8..16)

        /// Log event type (QU_TRANSFER 0, ..., CONTRACT_INFORMATION_MESSAGE 6, ...).
        uint64 logType;          // [16..24)

        /// Number of valid bytes in data (0..256).
        uint64 dataLen;          // [24..32)

        /// The raw log body, zero-padded beyond dataLen. For contract-emitted log types (4..7)
        /// the body BEGINS with the core-stamped 8-byte prefix (contractIndex u32 LE |
        /// contract-defined type u32 LE), so the usable contract payload is at most 248 bytes.
        /// The consumer contract defines the layout of the rest.
        Array<uint8, 256> data;  // [32..288)
    };

    /// Return query fee.
    static sint64 getQueryFee(const OracleQuery& query)
    {
        return 100;
    }

    //-------------------------------------------------------------------------
    // Optional: convenience features for contracts using the oracle interface

    /// True if the reply carries a usable raw-log attestation.
    static bool replyIsValid(const OracleReply& reply)
    {
        return reply.code == RESULT_SUCCESS;
    }
};
