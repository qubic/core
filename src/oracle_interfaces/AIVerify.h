using namespace QPI;

/**
* Oracle interface "AIVerify" for decentralized AI inference verification.
*
* Allows QAGENT contract to request that the 676 computor quorum independently
* verify an AI agent's claimed result against a task specification.
*
* The quorum reaches consensus on whether the result is valid, invalid, or
* unresolvable. A 451-of-676 majority provides the same security as the chain itself.
*
* See Price.h for general documentation about oracle interfaces.
*/
struct AIVerify
{
	//-------------------------------------------------------------------------
	// Mandatory oracle interface definitions

	/// Oracle interface index
	static constexpr uint32 oracleInterfaceIndex = ORACLE_INTERFACE_INDEX;

	/// Oracle query data / input to the oracle machine
	struct OracleQuery
	{
		/// Task ID from the QAGENT contract
		uint64 taskId;

		/// Hash of the task specification (deterministic input)
		id specHash;

		/// Agent's claimed result hash
		id resultHash;

		/// Expected model identifier (for deterministic verification)
		id modelHash;

		/// Task type (mirrors QAGENT task type enum)
		uint8 taskType;

		/// Expected output format (0=hash, 1=structured, 2=freeform)
		uint8 outputFormat;

		/// Inference config (0=default, 1=deterministic_int8, 2=deterministic_ternary)
		uint8 inferenceConfig;

		uint8 _pad0;
		uint8 _pad1;
		uint8 _pad2;
		uint8 _pad3;
		uint8 _pad4;
	};

	/// Oracle reply data / output of the oracle machine
	struct OracleReply
	{
		/// Verdict: 0=abstain, 1=valid, 2=invalid
		uint8 verdict;

		uint8 _pad0;
		uint8 _pad1;
		uint8 _pad2;

		/// Confidence level in basis points (0-10000)
		uint32 confidence;

		/// Oracle's independently computed result hash
		id verifiedHash;
	};

	/// Return query fee
	static sint64 getQueryFee(const OracleQuery& query)
	{
		return 1000;
	}

	/// Return subscription fee (subscriptions not typical for verification)
	static sint64 getSubscriptionFee(const OracleQuery& query, uint16 notifyIntervalInMinutes)
	{
		return 0;
	}

	//-------------------------------------------------------------------------
	// Optional: convenience features

	/// Check if the oracle reply indicates a decisive verdict
	static bool replyIsValid(const OracleReply& reply)
	{
		return reply.verdict > 0 && reply.confidence > 0;
	}

	/// Verdict constants
	static constexpr uint8 VERDICT_ABSTAIN = 0;
	static constexpr uint8 VERDICT_VALID = 1;
	static constexpr uint8 VERDICT_INVALID = 2;
};
