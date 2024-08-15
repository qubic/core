struct OracleReplyCommitTransaction
{
	static constexpr unsigned char transactionType()
	{
		return 2; // TODO: Set actual value
	}

	Transaction transaction;
	unsigned long long queryIndex;
	m256i replyDigest;
	m256i replyKnowledgeProof;
	unsigned char signature[SIGNATURE_SIZE];
};

struct OracleReplyRevealTransactionPrefix
{
	static constexpr unsigned char transactionType()
	{
		return 3; // TODO: Set actual value
	}

	Transaction transaction;
	unsigned long long queryIndex;
};

struct OracleReplyRevealTransactionPostfix
{
	unsigned char signature[SIGNATURE_SIZE];
};