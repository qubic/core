struct MiningSolutionTransaction : public Transaction
{
	static constexpr unsigned char transactionType()
	{
		return 2; // TODO: Set actual value
	}

	static constexpr long long minAmount()
	{
		return SOLUTION_SECURITY_DEPOSIT;
	}

	static constexpr unsigned short minInputSize()
	{
		return sizeof(miningSeed) + sizeof(nonce);
	}

	m256i miningSeed;
	m256i nonce;
	unsigned char signature[SIGNATURE_SIZE];
};

struct CustomMiningTasksTransactionPrefix : public Transaction
{
	static constexpr unsigned char transactionType()
	{
		return 6; // TODO: Set actual value
	}

	static constexpr long long minAmount()
	{
		return 0;
	}

	static constexpr unsigned short minInputSize()
	{
		return sizeof(codeFileTrailerDigest) + sizeof(dataFileTrailerDigest);
	}

	m256i codeFileTrailerDigest;
	m256i dataFileTrailerDigest;
};

struct CustomMiningSolutionTransactionPrefix : public Transaction
{
	static constexpr unsigned char transactionType()
	{
		return 7; // TODO: Set actual value
	}

	static constexpr long long minAmount()
	{
		return 0;
	}

	static constexpr unsigned short minInputSize()
	{
		return sizeof(codeFileTrailerDigest) + sizeof(dataFileTrailerDigest);
	}

	m256i codeFileTrailerDigest;
	m256i dataFileTrailerDigest;
};