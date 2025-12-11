using namespace QPI;

constexpr uint32_t RANDOM_MAX_RECENT_MINERS = 512;  
constexpr uint32_t RANDOM_MAX_COMMITMENTS = 1024;  
constexpr uint32_t RANDOM_ENTROPY_HISTORY_LEN = 4;

struct RANDOM2 {};

struct RecentMiner
{
	id     minerId;
	uint64 deposit;
	uint64 lastEntropyVersion;
	uint32 lastRevealTick;
};

struct EntropyCommitment
{
	id     digest;
	id     invocatorId;
	uint64 amount;
	uint32 commitTick;
	uint32 revealDeadlineTick;
	bool   hasRevealed;
};

struct RANDOM : public ContractBase
{
private:
	// Entropy pool history (circular buffer for look-back; N must be 2^N)
	Array<m256i, RANDOM_ENTROPY_HISTORY_LEN> entropyHistory;
	Array<uint64, RANDOM_ENTROPY_HISTORY_LEN> entropyPoolVersionHistory;
	uint32 entropyHistoryHead; // points to most recent tick

	// Global entropy pool - combines all revealed entropy
	m256i  currentEntropyPool;
	uint64 entropyPoolVersion;

	// Tracking statistics
	uint64 totalCommits;
	uint64 totalReveals;
	uint64 totalSecurityDepositsLocked;

	// Contract configuration
	uint64 minimumSecurityDeposit;
	uint32 revealTimeoutTicks; // e.g. 9 ticks

	// Revenue tracking
	uint64 totalRevenue;
	uint64 pendingShareholderDistribution;
	uint64 lostDepositsRevenue;
	uint64 minerEarningsPool;
	uint64 shareholderEarningsPool;

	// Pricing config
	uint64 pricePerByte;         // e.g. 10 QU (default)
	uint64 priceDepositDivisor;  // e.g. 1000 (matches contract formula)

	// Miners (recent entropy providers) - LRU of high-value miners
	Array<RecentMiner, RANDOM_MAX_RECENT_MINERS> recentMiners;
	uint32 recentMinerCount;

	// Valid deposit amounts (powers of 10)
	uint64 validDepositAmounts[16]; // Scalar QPI array allowed

	// Commitment tracking
	Array<EntropyCommitment, RANDOM_MAX_COMMITMENTS> commitments;
	uint32 commitmentCount;

	// ----- Helpers -----
	static inline void updateEntropyPoolData(RANDOM& stateRef, const bit_4096& newEntropy)
	{
		const uint64* entropyData = reinterpret_cast<const uint64*>(&newEntropy);
		for (uint32 i = 0; i < 4; i++)
		{
			stateRef.currentEntropyPool.m256i_u64[i] ^= entropyData[i];
		}

		stateRef.entropyHistoryHead = (stateRef.entropyHistoryHead + 1) % RANDOM_ENTROPY_HISTORY_LEN;
		stateRef.entropyHistory.set(stateRef.entropyHistoryHead, stateRef.currentEntropyPool);

		stateRef.entropyPoolVersion++;
		stateRef.entropyPoolVersionHistory.set(stateRef.entropyHistoryHead, stateRef.entropyPoolVersion);
	}

	static inline void generateRandomBytesData(const RANDOM& stateRef, uint8* output, uint32 numBytes, uint32 historyIdx, uint32 currentTick)
	{
		const m256i selectedPool = stateRef.entropyHistory.get(
			(stateRef.entropyHistoryHead + RANDOM_ENTROPY_HISTORY_LEN - historyIdx) % RANDOM_ENTROPY_HISTORY_LEN
		);

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
		for (uint32 i = 0; i < 64U; i++)
		{
			if (data[i] != 0)
			{
				return false;
			}
		}
		return true;
	}

	static inline bool k12CommitmentMatches(
		const QPI::QpiContextFunctionCall& qpi,
		const QPI::bit_4096& revealedBits,
		const QPI::id& committedDigest)
	{
		QPI::id computedDigest = qpi.K12(revealedBits);
		for (QPI::uint32 i = 0; i < 32U; i++)
		{
			if (computedDigest.m256i_u8[i] != committedDigest.m256i_u8[i])
			{
				return false;
			}
		}
		return true;
	}

public:
	// ---------- API in/out types ----------
	struct RevealAndCommit_input
	{
		bit_4096 revealedBits;
		id committedDigest;
	};
	struct RevealAndCommit_output
	{
		uint8  randomBytes[32];
		uint64 entropyVersion;
		bool   revealSuccessful;
		bool   commitSuccessful;
		uint64 depositReturned;
	};

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
		uint64 totalRevenue;
		uint64 pendingShareholderDistribution;
		uint64 lostDepositsRevenue;
		uint64 minerEarningsPool;
		uint64 shareholderEarningsPool;
		uint32 recentMinerCount;
	};

	struct GetUserCommitments_input
	{
		id userId;
	};
	struct GetUserCommitments_output
	{
		struct UserCommitment
		{
			id digest;
			uint64 amount;
			uint32 commitTick;
			uint32 revealDeadlineTick;
			bool hasRevealed;
		} commitments[32];
		uint32 commitmentCount;
	};

	struct BuyEntropy_input
	{
		uint32 numberOfBytes;
		uint64 minMinerDeposit;
	};
	struct BuyEntropy_output
	{
		bool   success;
		uint8  randomBytes[32];
		uint64 entropyVersion;
		uint64 usedMinerDeposit;
		uint64 usedPoolVersion;
	};

	struct ClaimMinerEarnings_input {};
	struct ClaimMinerEarnings_output { uint64 payout; };

	struct QueryPrice_input {
		uint32 numberOfBytes;
		uint64 minMinerDeposit;
	};
	struct QueryPrice_output { uint64 price; };

	// --- Locals for macro-produced procedures ---
	struct RevealAndCommit_locals
	{
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
	struct BuyEntropy_locals
	{
		uint32 currentTick;
		bool eligible;
		uint64 usedMinerDeposit;
		uint32 i;
		uint64 minPrice;
		uint64 buyerFee;
		uint32 histIdx;
		uint64 half;
	};
	struct END_EPOCH_locals
	{
		uint32 currentTick;
		uint32 i;
		uint64 payout;
	};

	// --------------------------------------------------
	PUBLIC_PROCEDURE_WITH_LOCALS(RevealAndCommit)
	{
		locals.currentTick = qpi.tick();

		// Remove expired commitments
		for (locals.i = 0; locals.i < state.commitmentCount;)
		{
			EntropyCommitment cmt = state.commitments.get(locals.i); // value copy
			if (!cmt.hasRevealed && locals.currentTick > cmt.revealDeadlineTick)
			{
				uint64 lostDeposit = cmt.amount;
				state.lostDepositsRevenue += lostDeposit;
				state.totalRevenue += lostDeposit;
				state.pendingShareholderDistribution += lostDeposit;
				state.totalSecurityDepositsLocked -= lostDeposit;
				if (locals.i != state.commitmentCount - 1)
				{
					state.commitments.set(locals.i, state.commitments.get(state.commitmentCount - 1));
				}
				state.commitmentCount--;
			}
			else
			{
				locals.i++;
			}
		}

		// Unclaimed deposits: return forcibly at end of tick
		if (qpi.numberOfTickTransactions() == -1)
		{
			for (locals.i = 0; locals.i < state.commitmentCount;)
			{
				EntropyCommitment cmt = state.commitments.get(locals.i);
				if (!cmt.hasRevealed && cmt.revealDeadlineTick == qpi.tick())
				{
					qpi.transfer(cmt.invocatorId, cmt.amount);
					state.totalSecurityDepositsLocked -= cmt.amount;
					if (locals.i != state.commitmentCount - 1)
					{
						state.commitments.set(locals.i, state.commitments.get(state.commitmentCount - 1));
					}
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

		// Reveal logic (return deposit, add entropy, update recent miner stats)
		if (locals.hasRevealData)
		{
			for (locals.i = 0; locals.i < state.commitmentCount;)
			{
				EntropyCommitment cmt = state.commitments.get(locals.i);
				if (!cmt.hasRevealed && isEqualIdCheck(cmt.invocatorId, qpi.invocator()))
				{
					locals.hashMatches = k12CommitmentMatches(qpi, input.revealedBits, cmt.digest);

					if (locals.hashMatches)
					{
						if (locals.currentTick > cmt.revealDeadlineTick)
						{
							uint64 lostDeposit = cmt.amount;
							state.lostDepositsRevenue += lostDeposit;
							state.totalRevenue += lostDeposit;
							state.pendingShareholderDistribution += lostDeposit;
							output.revealSuccessful = false;
						}
						else
						{
							updateEntropyPoolData(state, input.revealedBits);
							qpi.transfer(qpi.invocator(), cmt.amount);
							output.revealSuccessful = true;
							output.depositReturned = cmt.amount;
							state.totalReveals++;
							state.totalSecurityDepositsLocked -= cmt.amount;

							// Maintain LRU recentMiner list for rewards
							locals.existingIndex = -1;
							for (locals.rm = 0; locals.rm < state.recentMinerCount; ++locals.rm)
							{
								RecentMiner rm = state.recentMiners.get(locals.rm);
								if (isEqualIdCheck(rm.minerId, qpi.invocator()))
								{
									locals.existingIndex = locals.rm;
									break;
								}
							}
							if (locals.existingIndex >= 0)
							{
								RecentMiner rm = state.recentMiners.get(locals.existingIndex);
								if (rm.deposit < cmt.amount)
								{
									rm.deposit = cmt.amount;
									rm.lastEntropyVersion = state.entropyPoolVersion;
								}
								rm.lastRevealTick = locals.currentTick;
								state.recentMiners.set(locals.existingIndex, rm);
							}
							else if (state.recentMinerCount < RANDOM_MAX_RECENT_MINERS)
							{
								RecentMiner rm;
								rm.minerId = qpi.invocator();
								rm.deposit = cmt.amount;
								rm.lastEntropyVersion = state.entropyPoolVersion;
								rm.lastRevealTick = locals.currentTick;
								state.recentMiners.set(state.recentMinerCount, rm);
								state.recentMinerCount++;
							}
							else
							{
								locals.lowestIx = 0;
								for (locals.rm = 1; locals.rm < RANDOM_MAX_RECENT_MINERS; ++locals.rm)
								{
									RecentMiner test = state.recentMiners.get(locals.rm);
									RecentMiner lo = state.recentMiners.get(locals.lowestIx);
									if (test.deposit < lo.deposit ||
										(test.deposit == lo.deposit && test.lastEntropyVersion < lo.lastEntropyVersion))
									{
										locals.lowestIx = locals.rm;
									}
								}
								RecentMiner rm = state.recentMiners.get(locals.lowestIx);
								if (
									cmt.amount > rm.deposit ||
									(cmt.amount == rm.deposit && state.entropyPoolVersion > rm.lastEntropyVersion)
									)
								{
									rm.minerId = qpi.invocator();
									rm.deposit = cmt.amount;
									rm.lastEntropyVersion = state.entropyPoolVersion;
									rm.lastRevealTick = locals.currentTick;
									state.recentMiners.set(locals.lowestIx, rm);
								}
							}
						}

						state.totalSecurityDepositsLocked -= cmt.amount;

						if (locals.i != state.commitmentCount - 1)
						{
							state.commitments.set(locals.i, state.commitments.get(state.commitmentCount - 1));
						}
						state.commitmentCount--;
						continue;
					}
				}
				locals.i++;
			}
		}

		// New commitment/registration for reward round
		if (locals.hasNewCommit && !locals.isStoppingMining)
		{
			if (
				isValidDepositAmountCheck(state, qpi.invocationReward()) &&
				qpi.invocationReward() >= state.minimumSecurityDeposit
				)
			{
				if (state.commitmentCount < RANDOM_MAX_COMMITMENTS)
				{
					EntropyCommitment ncmt;
					ncmt.digest = input.committedDigest;
					ncmt.invocatorId = qpi.invocator();
					ncmt.amount = qpi.invocationReward();
					ncmt.commitTick = locals.currentTick;
					ncmt.revealDeadlineTick = locals.currentTick + state.revealTimeoutTicks;
					ncmt.hasRevealed = false;
					state.commitments.set(state.commitmentCount, ncmt);
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

	PUBLIC_PROCEDURE_WITH_LOCALS(BuyEntropy)
	{
		locals.currentTick = qpi.tick();

		// Housekeeping: remove expired commitments
		for (locals.i = 0; locals.i < state.commitmentCount;)
		{
			EntropyCommitment cmt = state.commitments.get(locals.i);
			if (!cmt.hasRevealed && locals.currentTick > cmt.revealDeadlineTick)
			{
				uint64 lostDeposit = cmt.amount;
				state.lostDepositsRevenue += lostDeposit;
				state.totalRevenue += lostDeposit;
				state.pendingShareholderDistribution += lostDeposit;
				state.totalSecurityDepositsLocked -= lostDeposit;
				if (locals.i != state.commitmentCount - 1)
				{
					state.commitments.set(locals.i, state.commitments.get(state.commitmentCount - 1));
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

		// Find eligible recent miner
		for (locals.i = 0; locals.i < state.recentMinerCount; ++locals.i)
		{
			RecentMiner rm = state.recentMiners.get(locals.i);
			if (
				rm.deposit >= input.minMinerDeposit &&
				(locals.currentTick - rm.lastRevealTick) <= state.revealTimeoutTicks
				)
			{
				locals.eligible = true;
				locals.usedMinerDeposit = rm.deposit;
				break;
			}
		}

		if (!locals.eligible)
		{
			return;
		}

		locals.minPrice = state.pricePerByte * input.numberOfBytes *
			(div(input.minMinerDeposit, state.priceDepositDivisor) + 1);

		if (locals.buyerFee < locals.minPrice)
		{
			return;
		}

		locals.histIdx = (state.entropyHistoryHead + RANDOM_ENTROPY_HISTORY_LEN - 2) % RANDOM_ENTROPY_HISTORY_LEN;
		generateRandomBytesData(
			state,
			output.randomBytes,
			(input.numberOfBytes > 32 ? 32 : input.numberOfBytes),
			locals.histIdx,
			locals.currentTick
		);

		output.entropyVersion = state.entropyPoolVersionHistory.get(locals.histIdx);
		output.usedMinerDeposit = locals.usedMinerDeposit;
		output.usedPoolVersion = state.entropyPoolVersionHistory.get(locals.histIdx);
		output.success = true;

		locals.half = div(locals.buyerFee, (uint64)2);
		state.minerEarningsPool += locals.half;
		state.shareholderEarningsPool += (locals.buyerFee - locals.half);
	}

	PUBLIC_FUNCTION(GetContractInfo)
	{
		uint32 currentTick = qpi.tick();
		uint32 activeCount = 0;

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

		for (uint32 i = 0; i < 16; ++i)
		{
			output.validDepositAmounts[i] = state.validDepositAmounts[i];
		}
		for (uint32 i = 0; i < state.commitmentCount; ++i)
		{
			if (!state.commitments.get(i).hasRevealed)
			{
				activeCount++;
			}
		}
		output.activeCommitments = activeCount;
	}

	PUBLIC_FUNCTION(GetUserCommitments)
	{
		uint32 userCommitmentCount = 0;
		for (uint32 i = 0; i < state.commitmentCount && userCommitmentCount < 32; i++)
		{
			EntropyCommitment cmt = state.commitments.get(i);
			if (isEqualIdCheck(cmt.invocatorId, input.userId))
			{
				output.commitments[userCommitmentCount].digest = cmt.digest;
				output.commitments[userCommitmentCount].amount = cmt.amount;
				output.commitments[userCommitmentCount].commitTick = cmt.commitTick;
				output.commitments[userCommitmentCount].revealDeadlineTick = cmt.revealDeadlineTick;
				output.commitments[userCommitmentCount].hasRevealed = cmt.hasRevealed;
				userCommitmentCount++;
			}
		}
		output.commitmentCount = userCommitmentCount;
	}

	PUBLIC_FUNCTION(QueryPrice)
	{
		output.price = state.pricePerByte * input.numberOfBytes *
			(div(input.minMinerDeposit, (uint64)state.priceDepositDivisor) + 1);
	}

	END_EPOCH_WITH_LOCALS()
	{
		locals.currentTick = qpi.tick();
		for (locals.i = 0; locals.i < state.commitmentCount;)
		{
			EntropyCommitment cmt = state.commitments.get(locals.i);
			if (!cmt.hasRevealed && locals.currentTick > cmt.revealDeadlineTick)
			{
				uint64 lostDeposit = cmt.amount;
				state.lostDepositsRevenue += lostDeposit;
				state.totalRevenue += lostDeposit;
				state.pendingShareholderDistribution += lostDeposit;
				state.totalSecurityDepositsLocked -= lostDeposit;

				if (locals.i != state.commitmentCount - 1)
				{
					state.commitments.set(locals.i, state.commitments.get(state.commitmentCount - 1));
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
				RecentMiner rm = state.recentMiners.get(locals.i);
				if (!isZeroIdCheck(rm.minerId))
				{
					qpi.transfer(rm.minerId, locals.payout);
				}
			}
			state.minerEarningsPool = 0;
			for (locals.i = 0; locals.i < RANDOM_MAX_RECENT_MINERS; ++locals.i)
			{
				state.recentMiners.set(locals.i, RecentMiner{});
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

		for (uint32 i = 0; i < 16; ++i)
		{
			state.validDepositAmounts[i] = 1ULL;
			for (uint32 j = 0; j < i; ++j)
			{
				state.validDepositAmounts[i] *= 10;
			}
		}
	}
};
