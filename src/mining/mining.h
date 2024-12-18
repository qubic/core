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