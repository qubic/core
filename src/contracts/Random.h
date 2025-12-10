using namespace QPI;

// Max miners to consider for distribution
#define MAX_RECENT_MINERS 369
#define ENTROPY_HISTORY_LEN 3 // For 2-tick-back entropy pool

struct RANDOM2
{
};

struct RANDOM : public ContractBase
{
private:
	// Entropy pool history (circular buffer for look-back)
	m256i entropyHistory[ENTROPY_HISTORY_LEN];
	uint64 entropyPoolVersionHistory[ENTROPY_HISTORY_LEN];
	uint32 entropyHistoryHead; // points to most recent

	// Global entropy pool - combines all revealed entropy
	m256i currentEntropyPool;
	uint64 entropyPoolVersion;

	// Tracking statistics
	uint64 totalCommits;
	uint64 totalReveals;
	uint64 totalSecurityDepositsLocked;

	// Contract configuration
	uint64 minimumSecurityDeposit;
	uint32 revealTimeoutTicks; // e.g. 9 ticks

	// Revenue distribution system
	uint64 totalRevenue;
	uint64 pendingShareholderDistribution;
	uint64 lostDepositsRevenue;

	// Earnings pools
	uint64 minerEarningsPool;
	uint64 shareholderEarningsPool;

	// Pricing config
	uint64 pricePerByte;    // e.g. 10 QU (default)
	uint64 priceDepositDivisor;  // e.g. 1000 (matches contract formula)

	// Epoch tracking for miner rewards
	struct RecentMiner {
		id minerId;
		uint64 deposit;
		uint64 lastEntropyVersion;
		uint32 lastRevealTick;
	} recentMiners[MAX_RECENT_MINERS];
	uint32 recentMinerCount;

	// Valid deposit amounts (powers of 10)
	uint64 validDepositAmounts[16];

	// Commitment tracking
	struct EntropyCommitment {
		id digest;
		id invocatorId;
		uint64 amount;
		uint32 commitTick;
		uint32 revealDeadlineTick;
		bool hasRevealed;
	} commitments[1024];
	uint32 commitmentCount;

	// Helper functions (static inline)
	static inline void updateEntropyPoolData(RANDOM& stateRef, const bit_4096& newEntropy)
	{
		// XOR new entropy into the global pool
		// Access BitArray internal data through a uint64 pointer cast
		const uint64* entropyData = reinterpret_cast<const uint64*>(&newEntropy);
		for (uint32 i = 0; i < 4; i++)
			stateRef.currentEntropyPool.m256i_u64[i] ^= entropyData[i];

		// Update entropy history (circular buffer)
		stateRef.entropyHistoryHead = (stateRef.entropyHistoryHead + 1U) % ENTROPY_HISTORY_LEN;
		stateRef.entropyHistory[stateRef.entropyHistoryHead] = stateRef.currentEntropyPool;
		stateRef.entropyPoolVersion++;
		stateRef.entropyPoolVersionHistory[stateRef.entropyHistoryHead] = stateRef.entropyPoolVersion;
	}

	static inline void generateRandomBytesData(const RANDOM& stateRef, uint8* output, uint32 numBytes, uint32 historyIdx, uint32 currentTick)
	{
		const m256i selectedPool = stateRef.entropyHistory[(stateRef.entropyHistoryHead + ENTROPY_HISTORY_LEN - historyIdx) % ENTROPY_HISTORY_LEN];
		m256i tickEntropy;
		tickEntropy.m256i_u64[0] = static_cast<uint64_t>(currentTick);
		tickEntropy.m256i_u64[1] = 0;
		tickEntropy.m256i_u64[2] = 0;
		tickEntropy.m256i_u64[3] = 0;
		m256i combinedEntropy;
		for (uint32 i = 0; i < 4; i++)
			combinedEntropy.m256i_u64[i] = selectedPool.m256i_u64[i] ^ tickEntropy.m256i_u64[i];

		// Copy bytes from the combined entropy to output
		for (uint32 i = 0; i < ((numBytes > 32U) ? 32U : numBytes); i++)
			output[i] = combinedEntropy.m256i_u8[i];
	}

	static inline bool isValidDepositAmountCheck(const RANDOM& stateRef, uint64 amount)
	{
		for (uint32 i = 0; i < 16U; i++)
			if (amount == stateRef.validDepositAmounts[i]) return true;
		return false;
	}

	static inline bool isEqualIdCheck(const id& a, const id& b)
	{
		for (uint32 i = 0; i < 32U; i++)
			if (a.m256i_u8[i] != b.m256i_u8[i]) return false;
		return true;
	}

	static inline bool isZeroIdCheck(const id& value)
	{
		for (uint32 i = 0; i < 32U; i++)
			if (value.m256i_u8[i] != 0) return false;
		return true;
	}

	static inline bool isZeroBitsCheck(const bit_4096& value)
	{
		// Access BitArray internal data through a uint64 pointer cast
		const uint64* data = reinterpret_cast<const uint64*>(&value);
		for (uint32 i = 0; i < 64U; i++)
			if (data[i] != 0) return false;
		return true;
	}

	// Helper functions (static inline)
	static inline bool k12CommitmentMatches(const QPI::QpiContextFunctionCall& qpi, const QPI::bit_4096& revealedBits, const QPI::id& committedDigest)
	{
		// QPI K12 returns id (m256i)
		QPI::id computedDigest = qpi.K12(revealedBits);
		for (QPI::uint32 i = 0; i < 32U; i++)
			if (computedDigest.m256i_u8[i] != committedDigest.m256i_u8[i]) return false;
		return true;
	}

public:
	// --------------------------------------------------
	// Entropy mining (commit-reveal)
	struct RevealAndCommit_input
	{
		bit_4096 revealedBits;   // Previous entropy to reveal (or zeros for first commit)
		id committedDigest;      // Hash of new entropy to commit (or zeros if stopping)
	};

	struct RevealAndCommit_output
	{
		uint8 randomBytes[32];
		uint64 entropyVersion;
		bool revealSuccessful;
		bool commitSuccessful;
		uint64 depositReturned;
	};

	// --------------------------------------------------
	// READ-ONLY FUNCTIONS

	struct GetContractInfo_input {};
	struct GetContractInfo_output
	{
		uint64 totalCommits;
		uint64 totalReveals;
		uint64 totalSecurityDepositsLocked;
		uint64 minimumSecurityDeposit;
		uint32 revealTimeoutTicks;
		uint32 activeCommitments;
		uint64 validDepositAmounts[16];
		uint32 currentTick;
		uint64 entropyPoolVersion;
		// Revenue + pools
		uint64 totalRevenue;
		uint64 pendingShareholderDistribution;
		uint64 lostDepositsRevenue;
		uint64 minerEarningsPool;
		uint64 shareholderEarningsPool;
		uint32 recentMinerCount;
	};

	struct GetUserCommitments_input { id userId; };
	struct GetUserCommitments_output
	{
		struct UserCommitment {
			id digest;
			uint64 amount;
			uint32 commitTick;
			uint32 revealDeadlineTick;
			bool hasRevealed;
		} commitments[32];
		uint32 commitmentCount;
	};

	// --------------------------------------------------
	// SELL ENTROPY (random bytes)
	struct BuyEntropy_input
	{
		uint32 numberOfBytes;      // 1-32
		uint64 minMinerDeposit;    // required deposit of recent miner
	};
	struct BuyEntropy_output
	{
		bool success;
		uint8 randomBytes[32];
		uint64 entropyVersion; // version of pool 2 ticks ago!
		uint64 usedMinerDeposit;
		uint64 usedPoolVersion;
	};

	// --------------------------------------------------
	// CLAIMING
	struct ClaimMinerEarnings_input {};
	struct ClaimMinerEarnings_output
	{
		uint64 payout;
	};

	// --------------------------------------------------
	// PRICE QUERY
	struct QueryPrice_input {
		uint32 numberOfBytes;
		uint64 minMinerDeposit;
	};
	struct QueryPrice_output {
		uint64 price;
	};

	// --------------------------------------------------
  // Mining: RevealAndCommit 
	PUBLIC_PROCEDURE(RevealAndCommit)
	{
		// Process timeouts first
		uint32 currentTick = qpi.tick();
		for (uint32 i = 0; i < state.commitmentCount; ) {
			if (!state.commitments[i].hasRevealed &&
				currentTick > state.commitments[i].revealDeadlineTick)
			{
				uint64 lostDeposit = state.commitments[i].amount;
				state.lostDepositsRevenue += lostDeposit;
				state.totalRevenue += lostDeposit;
				state.pendingShareholderDistribution += lostDeposit;
				state.totalSecurityDepositsLocked -= lostDeposit;
				if (i != state.commitmentCount - 1)
					state.commitments[i] = state.commitments[state.commitmentCount - 1];
				state.commitmentCount--;
			}
			else {
				i++;
			}
		}

		// Empty tick handling:
		if (qpi.numberOfTickTransactions() == -1) {
			for (uint32 i = 0; i < state.commitmentCount; ) {
				if (!state.commitments[i].hasRevealed &&
					state.commitments[i].revealDeadlineTick == qpi.tick())
				{
					qpi.transfer(state.commitments[i].invocatorId, state.commitments[i].amount);
					state.totalSecurityDepositsLocked -= state.commitments[i].amount;
					// Remove this slot by moving last in
					if (i != state.commitmentCount - 1)
						state.commitments[i] = state.commitments[state.commitmentCount - 1];
					state.commitmentCount--;
					// Do not increment i, so the moved entry is checked next
				}
				else {
					i++;
				}
			}
			copyMemory(output, RevealAndCommit_output{});
			return;
		}

		const RevealAndCommit_input& inputData = input;
		id invocatorId = qpi.invocator();
		uint64 invocatorAmount = qpi.invocationReward();
		copyMemory(output, RevealAndCommit_output{});

		bool hasRevealData = !isZeroBitsCheck(inputData.revealedBits);
		bool hasNewCommit = !isZeroIdCheck(inputData.committedDigest);
		bool isStoppingMining = (invocatorAmount == 0);

		// Step 1: Process reveal if provided
		if (hasRevealData)
		{
			for (uint32 i = 0; i < state.commitmentCount; ) {
				if (!state.commitments[i].hasRevealed &&
					isEqualIdCheck(state.commitments[i].invocatorId, invocatorId))
				{
					// Use QPI K12 for cryptographic binding
					bool hashMatches = k12CommitmentMatches(qpi, inputData.revealedBits, state.commitments[i].digest);
					if (hashMatches)
					{
						if (currentTick > state.commitments[i].revealDeadlineTick)
						{
							uint64 lostDeposit = state.commitments[i].amount;
							state.lostDepositsRevenue += lostDeposit;
							state.totalRevenue += lostDeposit;
							state.pendingShareholderDistribution += lostDeposit;
							output.revealSuccessful = false;
						}
						else
						{
							updateEntropyPoolData(state, inputData.revealedBits);

							qpi.transfer(invocatorId, state.commitments[i].amount);

							output.revealSuccessful = true;
							output.depositReturned = state.commitments[i].amount;

							state.totalReveals++;
							state.totalSecurityDepositsLocked -= state.commitments[i].amount;

							// Update RecentMiners (with per-miner freshness)
							sint32 existingIndex = -1;
							for (uint32 rm = 0; rm < state.recentMinerCount; ++rm) {
								if (isEqualIdCheck(state.recentMiners[rm].minerId, invocatorId)) {
									existingIndex = rm;
									break;
								}
							}
							if (existingIndex >= 0) {
								if (state.recentMiners[existingIndex].deposit < state.commitments[i].amount) {
									state.recentMiners[existingIndex].deposit = state.commitments[i].amount;
									state.recentMiners[existingIndex].lastEntropyVersion = state.entropyPoolVersion;
								}
								state.recentMiners[existingIndex].lastRevealTick = currentTick;
							}
							else {
								if (state.recentMinerCount < MAX_RECENT_MINERS) {
									state.recentMiners[state.recentMinerCount].minerId = invocatorId;
									state.recentMiners[state.recentMinerCount].deposit = state.commitments[i].amount;
									state.recentMiners[state.recentMinerCount].lastEntropyVersion = state.entropyPoolVersion;
									state.recentMiners[state.recentMinerCount].lastRevealTick = currentTick;
									state.recentMinerCount++;
								}
								else {
									// Overflow: evict
									uint32 lowestIx = 0;
									for (uint32 rm = 1; rm < MAX_RECENT_MINERS; ++rm) {
										if (state.recentMiners[rm].deposit < state.recentMiners[lowestIx].deposit ||
											(state.recentMiners[rm].deposit == state.recentMiners[lowestIx].deposit &&
												state.recentMiners[rm].lastEntropyVersion < state.recentMiners[lowestIx].lastEntropyVersion))
											lowestIx = rm;
									}
									if (state.commitments[i].amount > state.recentMiners[lowestIx].deposit ||
										(state.commitments[i].amount == state.recentMiners[lowestIx].deposit &&
											state.entropyPoolVersion > state.recentMiners[lowestIx].lastEntropyVersion))
									{
										state.recentMiners[lowestIx].minerId = invocatorId;
										state.recentMiners[lowestIx].deposit = state.commitments[i].amount;
										state.recentMiners[lowestIx].lastEntropyVersion = state.entropyPoolVersion;
										state.recentMiners[lowestIx].lastRevealTick = currentTick;
									}
								}
							}
						}
						// Compaction after reveal
						state.totalSecurityDepositsLocked -= state.commitments[i].amount;
						if (i != state.commitmentCount - 1)
							state.commitments[i] = state.commitments[state.commitmentCount - 1];
						state.commitmentCount--;
						// do not increment i so new moved slot is checked
						continue;
					}
				}
				i++;
			}
		}

		// Step 2: Process new commitment
		if (hasNewCommit && !isStoppingMining)
		{
			if (isValidDepositAmountCheck(state, invocatorAmount) && invocatorAmount >= state.minimumSecurityDeposit)
			{
				if (state.commitmentCount < 1024)
				{
					state.commitments[state.commitmentCount].digest = inputData.committedDigest;
					state.commitments[state.commitmentCount].invocatorId = invocatorId;
					state.commitments[state.commitmentCount].amount = invocatorAmount;
					state.commitments[state.commitmentCount].commitTick = currentTick;
					state.commitments[state.commitmentCount].revealDeadlineTick = currentTick + state.revealTimeoutTicks;
					state.commitments[state.commitmentCount].hasRevealed = false;
					state.commitmentCount++;
					state.totalCommits++;
					state.totalSecurityDepositsLocked += invocatorAmount;
					output.commitSuccessful = true;
				}
			}
		}

		// Always return random bytes (current pool)
		generateRandomBytesData(state, output.randomBytes, 32, 0, currentTick); // 0 = current pool
		output.entropyVersion = state.entropyPoolVersion;
	}

	// --------------------------------------------------
	// BUY ENTROPY / RANDOM BYTES 
	PUBLIC_PROCEDURE(BuyEntropy)
	{
		// Process timeouts first
		uint32 currentTick = qpi.tick();
		for (uint32 i = 0; i < state.commitmentCount; ) {
			if (!state.commitments[i].hasRevealed &&
				currentTick > state.commitments[i].revealDeadlineTick)
			{
				uint64 lostDeposit = state.commitments[i].amount;
				state.lostDepositsRevenue += lostDeposit;
				state.totalRevenue += lostDeposit;
				state.pendingShareholderDistribution += lostDeposit;
				state.totalSecurityDepositsLocked -= lostDeposit;
				if (i != state.commitmentCount - 1)
					state.commitments[i] = state.commitments[state.commitmentCount - 1];
				state.commitmentCount--;
			}
			else {
				i++;
			}
		}

		if (qpi.numberOfTickTransactions() == -1) {
			copyMemory(output, BuyEntropy_output{});
			output.success = false;
			return;
		}

		const BuyEntropy_input& inputData = input;
		copyMemory(output, BuyEntropy_output{});
		output.success = false;
		uint64 buyerFee = qpi.invocationReward();

		bool eligible = false;
		uint64 usedMinerDeposit = 0;
		for (uint32 i = 0; i < state.recentMinerCount; ++i) {
			if (state.recentMiners[i].deposit >= inputData.minMinerDeposit &&
				(currentTick - state.recentMiners[i].lastRevealTick) <= state.revealTimeoutTicks) {
				eligible = true;
				usedMinerDeposit = state.recentMiners[i].deposit;
				break;
			}
		}

		if (!eligible)
			return;

		uint64 minPrice = state.pricePerByte
			* inputData.numberOfBytes
			* (inputData.minMinerDeposit / state.priceDepositDivisor + 1);
		if (buyerFee < minPrice)
			return;

		uint32 histIdx = (state.entropyHistoryHead + ENTROPY_HISTORY_LEN - 2) % ENTROPY_HISTORY_LEN;
		generateRandomBytesData(state, output.randomBytes, (inputData.numberOfBytes > 32 ? 32 : inputData.numberOfBytes), histIdx, currentTick);
		output.entropyVersion = state.entropyPoolVersionHistory[histIdx];
		output.usedMinerDeposit = usedMinerDeposit;
		output.usedPoolVersion = state.entropyPoolVersionHistory[histIdx];
		output.success = true;
		uint64 half = buyerFee / 2;
		state.minerEarningsPool += half;
		state.shareholderEarningsPool += (buyerFee - half);
	}

	// --------------------------------------------------
// Read-only contract info
	PUBLIC_FUNCTION(GetContractInfo)
	{
		uint32 currentTick = qpi.tick();

		output.totalCommits = state.totalCommits;
		output.totalReveals = state.totalReveals;
		output.totalSecurityDepositsLocked = state.totalSecurityDepositsLocked;
		output.minimumSecurityDeposit = state.minimumSecurityDeposit;
		output.revealTimeoutTicks = state.revealTimeoutTicks;
		output.currentTick = currentTick;
		output.entropyPoolVersion = state.entropyPoolVersion;

		output.totalRevenue = state.totalRevenue;
		output.pendingShareholderDistribution = state.pendingShareholderDistribution;
		output.lostDepositsRevenue = state.lostDepositsRevenue;

		output.minerEarningsPool = state.minerEarningsPool;
		output.shareholderEarningsPool = state.shareholderEarningsPool;
		output.recentMinerCount = state.recentMinerCount;

		for (uint32 i = 0; i < 16; i++)
			output.validDepositAmounts[i] = state.validDepositAmounts[i];

		uint32 activeCount = 0;
		for (uint32 i = 0; i < state.commitmentCount; i++)
			if (!state.commitments[i].hasRevealed)
				activeCount++;
		output.activeCommitments = activeCount;
	}

	PUBLIC_FUNCTION(GetUserCommitments)
	{
		const GetUserCommitments_input& inputData = input;

		copyMemory(output, GetUserCommitments_output{});
		uint32 userCommitmentCount = 0;
		for (uint32 i = 0; i < state.commitmentCount && userCommitmentCount < 32; i++)
		{
			if (isEqualIdCheck(state.commitments[i].invocatorId, inputData.userId))
			{
				output.commitments[userCommitmentCount].digest = state.commitments[i].digest;
				output.commitments[userCommitmentCount].amount = state.commitments[i].amount;
				output.commitments[userCommitmentCount].commitTick = state.commitments[i].commitTick;
				output.commitments[userCommitmentCount].revealDeadlineTick = state.commitments[i].revealDeadlineTick;
				output.commitments[userCommitmentCount].hasRevealed = state.commitments[i].hasRevealed;
				userCommitmentCount++;
			}
		}
		output.commitmentCount = userCommitmentCount;
	}

	PUBLIC_FUNCTION(QueryPrice)
	{
		const QueryPrice_input& inputData = input;
		output.price = state.pricePerByte
			* inputData.numberOfBytes
			* (inputData.minMinerDeposit / state.priceDepositDivisor +1);
	}

	// --------------------------------------------------
	  // Epoch End: Distribute pools
	END_EPOCH()
	{
		// Process timeouts first
		uint32 currentTick = qpi.tick();
		for (uint32 i = 0; i < state.commitmentCount; ) {
			if (!state.commitments[i].hasRevealed &&
				currentTick > state.commitments[i].revealDeadlineTick)
			{
				uint64 lostDeposit = state.commitments[i].amount;
				state.lostDepositsRevenue += lostDeposit;
				state.totalRevenue += lostDeposit;
				state.pendingShareholderDistribution += lostDeposit;
				state.totalSecurityDepositsLocked -= lostDeposit;
				if (i != state.commitmentCount - 1)
					state.commitments[i] = state.commitments[state.commitmentCount - 1];
				state.commitmentCount--;
			}
			else {
				i++;
			}
		}

		// Distribute miner pool
		if (state.minerEarningsPool > 0 && state.recentMinerCount > 0) {
			uint64 payout = state.minerEarningsPool / state.recentMinerCount;
			for (uint32 i = 0; i < state.recentMinerCount; ++i) {
				if (!isZeroIdCheck(state.recentMiners[i].minerId))
					qpi.transfer(state.recentMiners[i].minerId, payout);
			}
			state.minerEarningsPool = 0;
			for (uint32 i = 0; i < MAX_RECENT_MINERS; ++i)
				state.recentMiners[i] = RecentMiner{};
			state.recentMinerCount = 0;
		}

		// Distribute to shareholders
		if (state.shareholderEarningsPool > 0) {
			qpi.distributeDividends(state.shareholderEarningsPool / NUMBER_OF_COMPUTORS);
			state.shareholderEarningsPool = 0;
		}

		// Continue current lost deposit distribution as before
		if (state.pendingShareholderDistribution > 0)
		{
			qpi.distributeDividends(state.pendingShareholderDistribution / NUMBER_OF_COMPUTORS);
			state.pendingShareholderDistribution = 0;
		}
	}

	REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
	{
		// READ-ONLY USER FUNCTIONS 
		REGISTER_USER_FUNCTION(GetContractInfo, 1);
		REGISTER_USER_FUNCTION(GetUserCommitments, 2);
		REGISTER_USER_FUNCTION(QueryPrice, 3);

		// USER PROCEDURES 
		REGISTER_USER_PROCEDURE(RevealAndCommit, 1);
		REGISTER_USER_PROCEDURE(BuyEntropy, 2);
	}

	INITIALIZE()
	{
		state.entropyHistoryHead = 0;
		for (uint32 i = 0; i < ENTROPY_HISTORY_LEN; ++i) {
			copyMemory(state.entropyHistory[i], m256i{});
			state.entropyPoolVersionHistory[i] = 0;
		}
		copyMemory(state.currentEntropyPool, m256i{});
		state.entropyPoolVersion = 0;
		state.totalCommits = 0;
		state.totalReveals = 0;
		state.totalSecurityDepositsLocked = 0;
		state.minimumSecurityDeposit = 1; // Now allow1 QU
		state.revealTimeoutTicks = 9;
		state.commitmentCount = 0;
		state.totalRevenue = 0;
		state.pendingShareholderDistribution = 0;
		state.lostDepositsRevenue = 0;
		state.minerEarningsPool = 0;
		state.shareholderEarningsPool = 0;
		state.recentMinerCount = 0;
		state.pricePerByte = 10;
		state.priceDepositDivisor = 1000;
		for (uint32 i = 0; i < 16; i++) {
			state.validDepositAmounts[i] = 1ULL;
			for (uint32 j = 0; j < i; j++)
				state.validDepositAmounts[i] *= 10;
		}
		for (uint32 i = 0; i < 1024; ++i)
			state.commitments[i] = EntropyCommitment{};
		for (uint32 i = 0; i < MAX_RECENT_MINERS; ++i)
			state.recentMiners[i] = RecentMiner{};
	}
};