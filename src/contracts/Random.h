using namespace QPI;

constexpr uint32_t MAX_RECENT_MINERS = 369;
constexpr uint32_t MAX_COMMITMENTS = 1024;
constexpr uint32_t ENTROPY_HISTORY_LEN = 3; // For 2-tick-back entropy pool

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
	} commitments[MAX_COMMITMENTS];
	uint32 commitmentCount;

	// Helper functions (static inline)
	// Remove local variables, pass as arguments if needed
	static inline void updateEntropyPoolData(RANDOM& stateRef, const bit_4096& newEntropy)
	{
		const uint64* entropyData = reinterpret_cast<const uint64*>(&newEntropy);

		for (uint32 i = 0; i < 4; i++)
		{
			stateRef.currentEntropyPool.m256i_u64[i] ^= entropyData[i];
		}

		stateRef.entropyHistoryHead = mod(stateRef.entropyHistoryHead + 1U, ENTROPY_HISTORY_LEN);
		stateRef.entropyHistory[stateRef.entropyHistoryHead] = stateRef.currentEntropyPool;
		stateRef.entropyPoolVersion++;
		stateRef.entropyPoolVersionHistory[stateRef.entropyHistoryHead] = stateRef.entropyPoolVersion;
	}

	static inline void generateRandomBytesData(const RANDOM& stateRef, uint8* output, uint32 numBytes, uint32 historyIdx, uint32 currentTick)
	{
		const m256i selectedPool = stateRef.entropyHistory[mod(stateRef.entropyHistoryHead + ENTROPY_HISTORY_LEN - historyIdx, ENTROPY_HISTORY_LEN)];
		m256i tickEntropy;
		tickEntropy.m256i_u64[0] = static_cast<uint64_t>(currentTick);
		tickEntropy.m256i_u64[1] = 0;
		tickEntropy.m256i_u64[2] = 0;
		tickEntropy.m256i_u64[3] = 0;
		m256i combinedEntropy;

		for (uint32 i = 0; i < 4; i++)
		{
			combinedEntropy.m256i_u64[i] = selectedPool.m256i_u64[i] ^ tickEntropy.m256i_u64[i];
		}

		for (uint32 i = 0; i < ((numBytes > 32U) ? 32U : numBytes); i++)
		{
			output[i] = combinedEntropy.m256i_u8[i];
		}
	}

	static inline bool isValidDepositAmountCheck(const RANDOM& stateRef, uint64 amount)
	{
		for (uint32 i = 0; i < 16U; i++)
		{
			if (amount == stateRef.validDepositAmounts[i])
			{
				return true;
			}
		}
		return false;
	}

	static inline bool isEqualIdCheck(const id& a, const id& b) 
	{ 
		return a == b;
	}

	static inline bool isZeroIdCheck(const id& value) 
	{ 
		return isZero(value); 
	}

	static inline bool isZeroBitsCheck(const bit_4096& value)
	{
		const uint64* data = reinterpret_cast<const uint64*>(&value);
		for (uint32 i = 0; i < 64U; i++) {
			if (data[i] != 0) 
			{
				return false;
			}
		}
			
		return true;
	}

	static inline bool k12CommitmentMatches(const QPI::QpiContextFunctionCall& qpi, const QPI::bit_4096& revealedBits, const QPI::id& committedDigest)
	{
		QPI::id computedDigest = qpi.K12(revealedBits);
		for (QPI::uint32 i = 0; i < 32U; i++)
		{
			if (computedDigest.m256i_u8[i] != committedDigest.m256i_u8[i]) return false;
		}
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

	// Locals structs for WITH_LOCALS macros
	struct RevealAndCommit_locals {
		uint32 currentTick;
		bool hasRevealData;
		bool hasNewCommit;
		bool isStoppingMining;
		sint32 existingIndex;
		uint32 i;
		uint32 rm;
		uint32 lowestIx;
		bool hashMatches;
	};
	struct BuyEntropy_locals {
		uint32 currentTick;
		bool eligible;
		uint64 usedMinerDeposit;
		uint32 i;
		uint64 minPrice;
		uint64 buyerFee;
		uint32 histIdx;
		uint64 half;
	};
	struct END_EPOCH_locals {
		uint32 currentTick;
		uint32 i;
		uint64 payout;
	};

	// --------------------------------------------------
	// Mining: RevealAndCommit
	PUBLIC_PROCEDURE_WITH_LOCALS(RevealAndCommit)
	{
		locals.currentTick = qpi.tick();

		for (locals.i = 0; locals.i < state.commitmentCount; ) 
		{
			if (!state.commitments[locals.i].hasRevealed &&
				locals.currentTick > state.commitments[locals.i].revealDeadlineTick)
			{
				uint64 lostDeposit = state.commitments[locals.i].amount;
				state.lostDepositsRevenue += lostDeposit;
				state.totalRevenue += lostDeposit;
				state.pendingShareholderDistribution += lostDeposit;
				state.totalSecurityDepositsLocked -= lostDeposit;
				if (locals.i != state.commitmentCount - 1)
					state.commitments[locals.i] = state.commitments[state.commitmentCount - 1];
				state.commitmentCount--;
			}
			else 
			{
				locals.i++;
			}
		}

		if (qpi.numberOfTickTransactions() == -1) 
		{
			for (locals.i = 0; locals.i < state.commitmentCount; ) 
			{
				if (!state.commitments[locals.i].hasRevealed &&
					state.commitments[locals.i].revealDeadlineTick == qpi.tick())
				{
					qpi.transfer(state.commitments[locals.i].invocatorId, state.commitments[locals.i].amount);
					state.totalSecurityDepositsLocked -= state.commitments[locals.i].amount;
					if (locals.i != state.commitmentCount - 1)
						state.commitments[locals.i] = state.commitments[state.commitmentCount - 1];
					state.commitmentCount--;
				}
				else 
				{
					locals.i++;
				}
			}
			return;
		}

		locals.hasRevealData = !isZeroBitsCheck(input.revealedBits);
		locals.hasNewCommit = !isZeroIdCheck(input.committedDigest);
		locals.isStoppingMining = (qpi.invocationReward() == 0);

		if (locals.hasRevealData)
		{
			for (locals.i = 0; locals.i < state.commitmentCount; ) 
			{
				if (!state.commitments[locals.i].hasRevealed &&
					isEqualIdCheck(state.commitments[locals.i].invocatorId, qpi.invocator()))
				{
					locals.hashMatches = k12CommitmentMatches(qpi, input.revealedBits, state.commitments[locals.i].digest);

					if (locals.hashMatches)
					{
						if (locals.currentTick > state.commitments[locals.i].revealDeadlineTick)
						{
							uint64 lostDeposit = state.commitments[locals.i].amount;
							state.lostDepositsRevenue += lostDeposit;
							state.totalRevenue += lostDeposit;
							state.pendingShareholderDistribution += lostDeposit;
							output.revealSuccessful = false;
						}
						else
						{
							updateEntropyPoolData(state, input.revealedBits);
							qpi.transfer(qpi.invocator(), state.commitments[locals.i].amount);
							output.revealSuccessful = true;
							output.depositReturned = state.commitments[locals.i].amount;
							state.totalReveals++;
							state.totalSecurityDepositsLocked -= state.commitments[locals.i].amount;
							locals.existingIndex = -1;

							for (locals.rm = 0; locals.rm < state.recentMinerCount; ++locals.rm) 
							{
								if (isEqualIdCheck(state.recentMiners[locals.rm].minerId, qpi.invocator())) 
								{
									locals.existingIndex = locals.rm;
									break;
								}
							}

							if (locals.existingIndex >= 0) 
							{
								if (state.recentMiners[locals.existingIndex].deposit < state.commitments[locals.i].amount) 
								{
									state.recentMiners[locals.existingIndex].deposit = state.commitments[locals.i].amount;
									state.recentMiners[locals.existingIndex].lastEntropyVersion = state.entropyPoolVersion;
								}
								state.recentMiners[locals.existingIndex].lastRevealTick = locals.currentTick;
							}
							else 
							{
								if (state.recentMinerCount < MAX_RECENT_MINERS) 
								{
									state.recentMiners[state.recentMinerCount].minerId = qpi.invocator();
									state.recentMiners[state.recentMinerCount].deposit = state.commitments[locals.i].amount;
									state.recentMiners[state.recentMinerCount].lastEntropyVersion = state.entropyPoolVersion;
									state.recentMiners[state.recentMinerCount].lastRevealTick = locals.currentTick;
									state.recentMinerCount++;
								}
								else 
								{
									locals.lowestIx = 0;

									for (locals.rm = 1; locals.rm < MAX_RECENT_MINERS; ++locals.rm) 
									{
										if (state.recentMiners[locals.rm].deposit < state.recentMiners[locals.lowestIx].deposit ||
											(state.recentMiners[locals.rm].deposit == state.recentMiners[locals.lowestIx].deposit &&
												state.recentMiners[locals.rm].lastEntropyVersion < state.recentMiners[locals.lowestIx].lastEntropyVersion))
										{
											locals.lowestIx = locals.rm;
										}
									}

									if (state.commitments[locals.i].amount > state.recentMiners[locals.lowestIx].deposit ||
										(state.commitments[locals.i].amount == state.recentMiners[locals.lowestIx].deposit &&
											state.entropyPoolVersion > state.recentMiners[locals.lowestIx].lastEntropyVersion))
									{
										state.recentMiners[locals.lowestIx].minerId = qpi.invocator();
										state.recentMiners[locals.lowestIx].deposit = state.commitments[locals.i].amount;
										state.recentMiners[locals.lowestIx].lastEntropyVersion = state.entropyPoolVersion;
										state.recentMiners[locals.lowestIx].lastRevealTick = locals.currentTick;
									}
								}
							}
						}

						state.totalSecurityDepositsLocked -= state.commitments[locals.i].amount;

						if (locals.i != state.commitmentCount - 1)
						{
							state.commitments[locals.i] = state.commitments[state.commitmentCount - 1];
						}
						state.commitmentCount--;

						continue;
					}
				}
				locals.i++;
			}
		}

		if (locals.hasNewCommit && !locals.isStoppingMining)
		{
			if (isValidDepositAmountCheck(state, qpi.invocationReward()) && qpi.invocationReward() >= state.minimumSecurityDeposit)
			{
				if (state.commitmentCount < MAX_COMMITMENTS)
				{
					state.commitments[state.commitmentCount].digest = input.committedDigest;
					state.commitments[state.commitmentCount].invocatorId = qpi.invocator();
					state.commitments[state.commitmentCount].amount = qpi.invocationReward();
					state.commitments[state.commitmentCount].commitTick = locals.currentTick;
					state.commitments[state.commitmentCount].revealDeadlineTick = locals.currentTick + state.revealTimeoutTicks;
					state.commitments[state.commitmentCount].hasRevealed = false;
					state.commitmentCount++;
					state.totalCommits++;
					state.totalSecurityDepositsLocked += qpi.invocationReward();
					output.commitSuccessful = true;
				}
			}
		}

		generateRandomBytesData(state, output.randomBytes, 32, 0, locals.currentTick);
		output.entropyVersion = state.entropyPoolVersion;
	}

	// BUY ENTROPY / RANDOM BYTES
	PUBLIC_PROCEDURE_WITH_LOCALS(BuyEntropy)
	{
		locals.currentTick = qpi.tick();

		for (locals.i = 0; locals.i < state.commitmentCount; ) 
		{
			if (!state.commitments[locals.i].hasRevealed &&
				locals.currentTick > state.commitments[locals.i].revealDeadlineTick)
			{
				uint64 lostDeposit = state.commitments[locals.i].amount;
				state.lostDepositsRevenue += lostDeposit;
				state.totalRevenue += lostDeposit;
				state.pendingShareholderDistribution += lostDeposit;
				state.totalSecurityDepositsLocked -= lostDeposit;
				if (locals.i != state.commitmentCount - 1)
				{
					state.commitments[locals.i] = state.commitments[state.commitmentCount - 1];
				}
				state.commitmentCount--;
			}
			else 
			{
				locals.i++;
			}
		}

		if (qpi.numberOfTickTransactions() == -1) 
		{
			output.success = false;
			return;
		}

		output.success = false;
		locals.buyerFee = qpi.invocationReward();
		locals.eligible = false;
		locals.usedMinerDeposit = 0;

		for (locals.i = 0; locals.i < state.recentMinerCount; ++locals.i) 
		{
			if (state.recentMiners[locals.i].deposit >= input.minMinerDeposit &&
				(locals.currentTick - state.recentMiners[locals.i].lastRevealTick) <= state.revealTimeoutTicks) 
			{
				locals.eligible = true;
				locals.usedMinerDeposit = state.recentMiners[locals.i].deposit;
				break;
			}
		}

		if (!locals.eligible)
		{
			return;
		}

		locals.minPrice = state.pricePerByte
			* input.numberOfBytes
			* (div(input.minMinerDeposit, state.priceDepositDivisor) + 1);

		if (locals.buyerFee < locals.minPrice)
		{
			return;
		}

		locals.histIdx = mod(state.entropyHistoryHead + ENTROPY_HISTORY_LEN - 2, ENTROPY_HISTORY_LEN);
		generateRandomBytesData(state, output.randomBytes, (input.numberOfBytes > 32 ? 32 : input.numberOfBytes), locals.histIdx, locals.currentTick);
		output.entropyVersion = state.entropyPoolVersionHistory[locals.histIdx];
		output.usedMinerDeposit = locals.usedMinerDeposit;
		output.usedPoolVersion = state.entropyPoolVersionHistory[locals.histIdx];
		output.success = true;

		locals.half = div(locals.buyerFee, (uint64)2);
		state.minerEarningsPool += locals.half;
		state.shareholderEarningsPool += (locals.buyerFee - locals.half);
	}

	// --------------------------------------------------
	// Read-only contract info
	PUBLIC_FUNCTION(GetContractInfo)
	{
		uint32 currentTick = qpi.tick();
		uint32 activeCount = 0;
		uint32 i;

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

		for (i = 0; i < 16; i++)
		{
			output.validDepositAmounts[i] = state.validDepositAmounts[i];
		}

		for (i = 0; i < state.commitmentCount; i++)
		{
			if (!state.commitments[i].hasRevealed)
			{
				activeCount++;
			}
		}
		output.activeCommitments = activeCount;
	}

	PUBLIC_FUNCTION(GetUserCommitments)
	{
		uint32 userCommitmentCount = 0;
		uint32 i;

		for (i = 0; i < state.commitmentCount && userCommitmentCount < 32; i++)
		{
			if (isEqualIdCheck(state.commitments[i].invocatorId, input.userId))
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
		output.price = state.pricePerByte
			* input.numberOfBytes
			* (div(input.minMinerDeposit, (uint64)state.priceDepositDivisor) + 1);
	}

	END_EPOCH_WITH_LOCALS()
	{
		locals.currentTick = qpi.tick();

		for (locals.i = 0; locals.i < state.commitmentCount; ) {
			if (!state.commitments[locals.i].hasRevealed &&
				locals.currentTick > state.commitments[locals.i].revealDeadlineTick)
			{
				uint64 lostDeposit = state.commitments[locals.i].amount;
				state.lostDepositsRevenue += lostDeposit;
				state.totalRevenue += lostDeposit;
				state.pendingShareholderDistribution += lostDeposit;
				state.totalSecurityDepositsLocked -= lostDeposit;
				if (locals.i != state.commitmentCount - 1)
				{
					state.commitments[locals.i] = state.commitments[state.commitmentCount - 1];
				}
				state.commitmentCount--;
			}
			else 
			{
				locals.i++;
			}
		}

		if (state.minerEarningsPool > 0 && state.recentMinerCount > 0) 
		{
			locals.payout = div(state.minerEarningsPool, (uint64)state.recentMinerCount);
			for (locals.i = 0; locals.i < state.recentMinerCount; ++locals.i) 
			{
				if (!isZeroIdCheck(state.recentMiners[locals.i].minerId))
				{
					qpi.transfer(state.recentMiners[locals.i].minerId, locals.payout);
				}
			}
			state.minerEarningsPool = 0;
			for (locals.i = 0; locals.i < MAX_RECENT_MINERS; ++locals.i)
			{
				state.recentMiners[locals.i] = RecentMiner{};
			}
			state.recentMinerCount = 0;
		}

		if (state.shareholderEarningsPool > 0) 
		{
			qpi.distributeDividends(div(state.shareholderEarningsPool, (uint64)NUMBER_OF_COMPUTORS));
			state.shareholderEarningsPool = 0;
		}

		if (state.pendingShareholderDistribution > 0)
		{
			qpi.distributeDividends(div(state.pendingShareholderDistribution, (uint64)NUMBER_OF_COMPUTORS));
			state.pendingShareholderDistribution = 0;
		}
	}

	REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
	{
		REGISTER_USER_FUNCTION(GetContractInfo, 1);
		REGISTER_USER_FUNCTION(GetUserCommitments, 2);
		REGISTER_USER_FUNCTION(QueryPrice, 3);

		REGISTER_USER_PROCEDURE(RevealAndCommit, 1);
		REGISTER_USER_PROCEDURE(BuyEntropy, 2);
	}

	INITIALIZE()
	{
		state.entropyHistoryHead = 0;
		state.minimumSecurityDeposit = 1;
		state.revealTimeoutTicks = 9;
		state.pricePerByte = 10;
		state.priceDepositDivisor = 1000;

		for (uint32 i = 0; i < 16; i++) 
		{
			state.validDepositAmounts[i] = 1ULL;
			for (uint32 j = 0; j < i; j++)
			{
				state.validDepositAmounts[i] *= 10;
			}
		}
	}
};
