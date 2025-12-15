#pragma once

#include "network_messages/transactions.h"


struct OracleReplyCommitTransactionItem
{
	unsigned long long queryId;
	m256i replyDigest;
	m256i replyKnowledgeProof;
};

// Transaction for committing one or multiple oracle replies. The tx prefix is followed by one or multiple
// instances of OracleReplyCommitTransactionItem as input data, and subsequently the signature.
struct OracleReplyCommitTransactionPrefix : public Transaction
{
	static constexpr unsigned char transactionType()
	{
		return 6; // TODO: Set actual value
	}

	static constexpr unsigned short minInputSize()
	{
		return sizeof(OracleReplyCommitTransactionItem);
	}
};

// Transaction for revealing oracle reply. The tx prefix is followed by the OracleReply data
// as defined by the oracle interface of the query, and subsequently the postfix (signature).
struct OracleReplyRevealTransactionPrefix : public Transaction
{
	static constexpr unsigned char transactionType()
	{
		return 7; // TODO: Set actual value
	}

	static constexpr unsigned short minInputSize()
	{
		return sizeof(queryId);
	}

	unsigned long long queryId;
};

// Transaction for querying oracle. The tx prefix is followed by the OracleQuery data
// as defined by the oracle interface, and subsequently the signature.
struct OracleUserQueryTransactionPrefix : public Transaction
{
	static constexpr unsigned char transactionType()
	{
		return 9; // TODO: Set actual value
	}

	static constexpr unsigned short minInputSize()
	{
		return sizeof(oracleInterfaceIndex) + sizeof(timeoutMilliseconds);
	}

	unsigned int oracleInterfaceIndex;
	unsigned int timeoutMilliseconds;
};
