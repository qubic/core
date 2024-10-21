struct FileHeaderTransaction : public Transaction
{
	static constexpr unsigned char transactionType()
	{
		return 3; // TODO: Set actual value
	}

	static constexpr long long minAmount()
	{
		return 0;
	}

	static constexpr unsigned short minInputSize()
	{
		return sizeof(fileSize)
			+ sizeof(numberOfFragments)
			+ sizeof(fileFormat);
	}

	unsigned long long fileSize;
	unsigned long long numberOfFragments;
	unsigned char fileFormat[8];
	unsigned char signature[SIGNATURE_SIZE];
};

struct FileFragmentTransactionPrefix : public Transaction
{
	static constexpr unsigned char transactionType()
	{
		return 4; // TODO: Set actual value
	}

	static constexpr long long minAmount()
	{
		return 0;
	}

	static constexpr unsigned short minInputSize()
	{
		return sizeof(fragmentIndex)
			+ sizeof(prevFileFragmentTransactionDigest);
	}

	unsigned long long fragmentIndex;
	m256i prevFileFragmentTransactionDigest;
};

struct FileFragmentTransactionPostfix
{
	unsigned char signature[SIGNATURE_SIZE];
};

struct FileTrailerTransaction : public Transaction
{
	static constexpr unsigned char transactionType()
	{
		return 5; // TODO: Set actual value
	}

	static constexpr long long minAmount()
	{
		return 0;
	}

	static constexpr unsigned short minInputSize()
	{
		return sizeof(fileSize)
			+ sizeof(numberOfFragments)
			+ sizeof(fileFormat)
			+ sizeof(lastFileFragmentTransactionDigest);
	}

	unsigned long long fileSize;
	unsigned long long numberOfFragments;
	unsigned char fileFormat[8];
	m256i lastFileFragmentTransactionDigest;
	unsigned char signature[SIGNATURE_SIZE];
};