using namespace QPI;

// Random contract: collects entropy reveals (commit-reveal), maintains an entropy pool,
// lets buyers purchase bytes of entropy, and pays miners/shareholders.

// Key sizes and limits:
constexpr uint32_t RANDOM_MAX_RECENT_MINERS = 512;   // 2^9
constexpr uint32_t RANDOM_MAX_COMMITMENTS = 1024;    // 2^10
constexpr uint32_t RANDOM_ENTROPY_HISTORY_LEN = 4;   // 2^2, even if 3 would suffice
constexpr uint32_t RANDOM_VALID_DEPOSIT_AMOUNTS = 16;
constexpr uint32_t RANDOM_MAX_USER_COMMITMENTS = 32;
constexpr uint32_t RANDOM_RANDOMBYTES_LEN = 32;

struct RANDOM2 {};

// Recent miner info (LRU-ish tracking used to reward miners)
struct RANDOM_RecentMiner
{
	id     minerId;
	uint64 deposit;
	uint64 lastEntropyVersion;
	uint32 lastRevealTick;
};

// Stored commitment created by miners (commit-reveal scheme)
struct RANDOM_EntropyCommitment
{
	id     digest;                // K12(revealedBits) stored at commit time
	id     invocatorId;           // who committed
	uint64 amount;                // security deposit
	uint32 commitTick;
	uint32 revealDeadlineTick;
	bool   hasRevealed;
};

// Contract state and logic
struct RANDOM : public ContractBase
{
private:
	// --- QPI contract state ---
	
	// Circular history of recent entropy pools (m256i)
	Array<m256i, RANDOM_ENTROPY_HISTORY_LEN> entropyHistory;
	Array<uint64, RANDOM_ENTROPY_HISTORY_LEN> entropyPoolVersionHistory;
	uint32 entropyHistoryHead;

	// current 256-bit entropy pool and its version
	m256i currentEntropyPool;
	uint64 entropyPoolVersion;

	// Metrics and bookkeeping
	uint64 totalCommits;
	uint64 totalReveals;
	uint64 totalSecurityDepositsLocked;

	// Configurable parameters
	uint64 minimumSecurityDeposit;
	uint32 revealTimeoutTicks;

	// Revenue accounting
	uint64 totalRevenue;
	uint64 pendingShareholderDistribution;
	uint64 lostDepositsRevenue;
	uint64 minerEarningsPool;
	uint64 shareholderEarningsPool;

	// Pricing
	uint64 pricePerByte;
	uint64 priceDepositDivisor;

	// Recent miners (LRU-like), used to split miner earnings
	Array<RANDOM_RecentMiner, RANDOM_MAX_RECENT_MINERS> recentMiners;
	uint32 recentMinerCount;

	// Allowed deposit amounts (valid security deposits)
	Array<uint64, RANDOM_VALID_DEPOSIT_AMOUNTS> validDepositAmounts;

	// Active commitments (commitments array + count)
	Array<RANDOM_EntropyCommitment, RANDOM_MAX_COMMITMENTS> commitments;
	uint32 commitmentCount;

	// --- QPI-compliant helpers ---
	
	// Simple helpers that avoid forbidden constructs in contracts.

	static inline bool validDepositAmountAt(const RANDOM& stateRef, uint64 amount, uint32 idx)
	{
		return amount == stateRef.validDepositAmounts.get(idx);
	}

	static inline bool isEqualIdCheck(const id& a, const id& b)
	{
		return a == b;
	}

	static inline bool isZeroIdCheck(const id& value)
	{
		return isZero(value);
	}
	
	static inline uint64 calculatePrice(const RANDOM& state, uint32 numberOfBytes, uint64 minMinerDeposit)
	{
	    return state.pricePerByte * numberOfBytes *
	        (div(minMinerDeposit, state.priceDepositDivisor) + 1ULL);
	}

public:
	// --- Inputs / outputs for user-facing procedures and functions ---

	struct RevealAndCommit_input
	{
		bit_4096 revealedBits;
		id committedDigest;
	};
	struct RevealAndCommit_output
	{
		Array<uint8, RANDOM_RANDOMBYTES_LEN> randomBytes;
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
		Array<uint64, RANDOM_VALID_DEPOSIT_AMOUNTS> validDepositAmounts;
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
		};
		Array<UserCommitment, RANDOM_MAX_USER_COMMITMENTS> commitments;
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
		Array<uint8, RANDOM_RANDOMBYTES_LEN> randomBytes;
		uint64 entropyVersion;
		uint64 usedMinerDeposit;
		uint64 usedPoolVersion;
	};

	struct QueryPrice_input { uint32 numberOfBytes; uint64 minMinerDeposit; };
	struct QueryPrice_output { uint64 price; };

	//---- Locals storage for procedures ---

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

		// locals for random-bytes generation (no stack locals)
		uint32 histIdx;
		uint32 rb_i;

		// precompute K12 digest of revealedBits once per call
		id revealedDigest;

		// per-iteration commitment
		RANDOM_EntropyCommitment cmt;

		// per-iteration temporaries (moved into locals for compliance)
		uint64 lostDeposit;
		RANDOM_RecentMiner recentMinerA;
		RANDOM_RecentMiner recentMinerB;

		// deposit validity flag moved into locals
		bool depositValid;

		// temporary used to create new commitments (moved out of stack)
		RANDOM_EntropyCommitment ncmt;
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

		RANDOM_EntropyCommitment cmt;

		// per-iteration temporaries
		uint64 lostDeposit;
		RANDOM_RecentMiner recentMinerTemp;
	};
	struct END_EPOCH_locals
	{
		uint32 currentTick;
		uint32 i;
		uint64 payout;

		RANDOM_EntropyCommitment cmt;

		// per-iteration temporaries
		uint64 lostDeposit;
		RANDOM_RecentMiner recentMinerTemp;
	};
	struct GetUserCommitments_locals
	{
		uint32 userCommitmentCount;
		uint32 i;

		RANDOM_EntropyCommitment cmt;
		GetUserCommitments_output::UserCommitment ucmt;
	};
	struct GetContractInfo_locals
	{
		uint32 currentTick;
		uint32 activeCount;
		uint32 i;
	};
	struct INITIALIZE_locals
	{
		uint32 i;
		uint32 j;
		uint64 val;
	};

	// --------------------------------------------------
	// RevealAndCommit procedure:
	// - Removes expired commitments
	// - Optionally processes a reveal (preimage) and returns deposit if valid
	// - Optionally accepts a new commitment (invocation reward as deposit)

	PUBLIC_PROCEDURE_WITH_LOCALS(RevealAndCommit)
	{
		locals.currentTick = qpi.tick();

		// Remove expired commitments (sweep) -- reclaim lost deposits into revenue pools
		for (locals.i = 0; locals.i < state.commitmentCount;)
		{
			locals.cmt = state.commitments.get(locals.i);
			if (!locals.cmt.hasRevealed && locals.currentTick > locals.cmt.revealDeadlineTick)
			{
				// Move deposit into lost revenue and remove commitment (swap-with-last)
				locals.lostDeposit = locals.cmt.amount;
				state.lostDepositsRevenue += locals.lostDeposit;
				state.totalRevenue += locals.lostDeposit;
				state.pendingShareholderDistribution += locals.lostDeposit;
				state.totalSecurityDepositsLocked -= locals.lostDeposit;
				if (locals.i != state.commitmentCount - 1)
				{
					locals.cmt = state.commitments.get(state.commitmentCount - 1);
					state.commitments.set(locals.i, locals.cmt);
				}
				state.commitmentCount--;
			}
			else
			{
				locals.i++;
			}
		}

		// Special-case early epoch: forcibly return deposits that expire exactly this tick
		if (qpi.numberOfTickTransactions() == -1)
		{
			for (locals.i = 0; locals.i < state.commitmentCount;)
			{
				locals.cmt = state.commitments.get(locals.i);
				if (!locals.cmt.hasRevealed && locals.cmt.revealDeadlineTick == qpi.tick())
				{
					// refund directly in early epoch mode
					qpi.transfer(locals.cmt.invocatorId, locals.cmt.amount);
					state.totalSecurityDepositsLocked -= locals.cmt.amount;
					if (locals.i != state.commitmentCount - 1)
					{
						locals.cmt = state.commitments.get(state.commitmentCount - 1);
						state.commitments.set(locals.i, locals.cmt);
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

		// Precompute digest of revealedBits once to avoid forbidden casts into bit_4096
		locals.revealedDigest = qpi.K12(input.revealedBits);

		// Presence of reveal is treated as an attempt to match stored digests
		locals.hasRevealData = true;
		locals.hasNewCommit = !isZeroIdCheck(input.committedDigest);
		locals.isStoppingMining = (qpi.invocationReward() == 0);

		// If reveal provided: search for matching commitment(s) by this invocator
		for (locals.i = 0; locals.i < state.commitmentCount;)
		{
			locals.cmt = state.commitments.get(locals.i);
			if (!locals.cmt.hasRevealed && isEqualIdCheck(locals.cmt.invocatorId, qpi.invocator()))
			{
				// Compare stored digest to precomputed K12(revealedBits)
				locals.hashMatches = (locals.cmt.digest == locals.revealedDigest);

				if (locals.hashMatches)
				{
					// If reveal too late, deposit is forfeited; otherwise update entropy pool and refund.
					if (locals.currentTick > locals.cmt.revealDeadlineTick)
					{
						locals.lostDeposit = locals.cmt.amount;
						state.lostDepositsRevenue += locals.lostDeposit;
						state.totalRevenue += locals.lostDeposit;
						state.pendingShareholderDistribution += locals.lostDeposit;
						output.revealSuccessful = false;
					}
					else
					{
						// Apply the 256-bit digest to the pool by XORing the 4 x 64-bit lanes.
						// This avoids inspecting bit_4096 internals and keeps everything QPI-compliant.
						state.currentEntropyPool.u64._0 ^= locals.revealedDigest.u64._0;
						state.currentEntropyPool.u64._1 ^= locals.revealedDigest.u64._1;
						state.currentEntropyPool.u64._2 ^= locals.revealedDigest.u64._2;
						state.currentEntropyPool.u64._3 ^= locals.revealedDigest.u64._3;

						// Advance circular history with copy of new pool and bump version.
						state.entropyHistoryHead = (state.entropyHistoryHead + 1) & (RANDOM_ENTROPY_HISTORY_LEN - 1);
						state.entropyHistory.set(state.entropyHistoryHead, state.currentEntropyPool);

						state.entropyPoolVersion++;
						state.entropyPoolVersionHistory.set(state.entropyHistoryHead, state.entropyPoolVersion);

						// Refund deposit to invocator and update stats.
						qpi.transfer(qpi.invocator(), locals.cmt.amount);
						output.revealSuccessful = true;
						output.depositReturned = locals.cmt.amount;
						state.totalReveals++;
						state.totalSecurityDepositsLocked -= locals.cmt.amount;

						// Maintain recentMiners LRU: update existing entry, append if space, or replace lowest.
						locals.existingIndex = -1;
						for (locals.rm = 0; locals.rm < state.recentMinerCount; ++locals.rm)
						{
							locals.recentMinerA = state.recentMiners.get(locals.rm);
							if (isEqualIdCheck(locals.recentMinerA.minerId, qpi.invocator()))
							{
								locals.existingIndex = locals.rm;
								break;
							}
						}
						if (locals.existingIndex >= 0)
						{
							// update stored recent miner entry
							locals.recentMinerA = state.recentMiners.get(locals.existingIndex);
							if (locals.recentMinerA.deposit < locals.cmt.amount)
							{
								locals.recentMinerA.deposit = locals.cmt.amount;
								locals.recentMinerA.lastEntropyVersion = state.entropyPoolVersion;
							}
							locals.recentMinerA.lastRevealTick = locals.currentTick;
							state.recentMiners.set(locals.existingIndex, locals.recentMinerA);
						}
						else if (state.recentMinerCount < RANDOM_MAX_RECENT_MINERS)
						{
							// append new recent miner
							locals.recentMinerA.minerId = qpi.invocator();
							locals.recentMinerA.deposit = locals.cmt.amount;
							locals.recentMinerA.lastEntropyVersion = state.entropyPoolVersion;
							locals.recentMinerA.lastRevealTick = locals.currentTick;
							state.recentMiners.set(state.recentMinerCount, locals.recentMinerA);
							state.recentMinerCount++;
						}
						else
						{
							// Find lowest-ranked miner and replace if current qualifies
							locals.lowestIx = 0;
							for (locals.rm = 1; locals.rm < RANDOM_MAX_RECENT_MINERS; ++locals.rm)
							{
								locals.recentMinerA = state.recentMiners.get(locals.rm);
								locals.recentMinerB = state.recentMiners.get(locals.lowestIx);
								if (locals.recentMinerA.deposit < locals.recentMinerB.deposit ||
									(locals.recentMinerA.deposit == locals.recentMinerB.deposit && locals.recentMinerA.lastEntropyVersion < locals.recentMinerB.lastEntropyVersion))
								{
									locals.lowestIx = locals.rm;
								}
							}
							locals.recentMinerA = state.recentMiners.get(locals.lowestIx);
							if (
								locals.cmt.amount > locals.recentMinerA.deposit ||
								(locals.cmt.amount == locals.recentMinerA.deposit && state.entropyPoolVersion > locals.recentMinerA.lastEntropyVersion)
								)
							{
								locals.recentMinerA.minerId = qpi.invocator();
								locals.recentMinerA.deposit = locals.cmt.amount;
								locals.recentMinerA.lastEntropyVersion = state.entropyPoolVersion;
								locals.recentMinerA.lastRevealTick = locals.currentTick;
								state.recentMiners.set(locals.lowestIx, locals.recentMinerA);
							}
						}
					}

					// Remove commitment (swap with last) and continue scanning without incrementing i.
					state.totalSecurityDepositsLocked -= locals.cmt.amount;
					if (locals.i != state.commitmentCount - 1)
					{
						locals.cmt = state.commitments.get(state.commitmentCount - 1);
						state.commitments.set(locals.i, locals.cmt);
					}
					state.commitmentCount--;
					continue;
				}
			}
			locals.i++;
		}

		// If caller provided a new commitment (invocationReward used as deposit) and not stopping mining,
		// accept it if deposit is valid and meets minimum.
		if (locals.hasNewCommit && !locals.isStoppingMining)
		{
			// Inline deposit validity check using allowed-values array
			locals.depositValid = false;
			for (locals.i = 0; locals.i < RANDOM_VALID_DEPOSIT_AMOUNTS; ++locals.i)
			{
				if (validDepositAmountAt(state, qpi.invocationReward(), locals.i))
				{
					locals.depositValid = true;
					break;
				}
			}

			if (locals.depositValid && qpi.invocationReward() >= state.minimumSecurityDeposit)
			{
				if (state.commitmentCount < RANDOM_MAX_COMMITMENTS)
				{
					// Use locals.ncmt (approved locals) as temporary to avoid stack-local.
					locals.ncmt.digest = input.committedDigest;
					locals.ncmt.invocatorId = qpi.invocator();
					locals.ncmt.amount = qpi.invocationReward();
					locals.ncmt.commitTick = locals.currentTick;
					locals.ncmt.revealDeadlineTick = locals.currentTick + state.revealTimeoutTicks;
					locals.ncmt.hasRevealed = false;
					state.commitments.set(state.commitmentCount, locals.ncmt);
					state.commitmentCount++;
					state.totalCommits++;
					state.totalSecurityDepositsLocked += qpi.invocationReward();
					output.commitSuccessful = true;
				}
			}
		}

		// Produce 32 random-like bytes from latest entropy history and current tick:
		// - take most recent history entry (histIdx) and extract bytes from its 64-bit lanes,
		// - XOR first 8 bytes with tick-derived bytes to add per-tick variation.
		locals.histIdx = (state.entropyHistoryHead + RANDOM_ENTROPY_HISTORY_LEN - 0) & (RANDOM_ENTROPY_HISTORY_LEN - 1);
		for (locals.rb_i = 0; locals.rb_i < RANDOM_RANDOMBYTES_LEN; ++locals.rb_i)
		{
			// Extract the correct 64-bit lane and then the requested byte without using plain [].
			output.randomBytes.set(
				locals.rb_i,
				static_cast<uint8_t>(
					(
						(
							(locals.rb_i < 8) ? state.entropyHistory.get(locals.histIdx).u64._0 :
							(locals.rb_i < 16) ? state.entropyHistory.get(locals.histIdx).u64._1 :
							(locals.rb_i < 24) ? state.entropyHistory.get(locals.histIdx).u64._2 :
							state.entropyHistory.get(locals.histIdx).u64._3
							) >> (8 * (locals.rb_i & 7))
						) & 0xFF
					) ^
				(locals.rb_i < 8 ? static_cast<uint8_t>((static_cast<uint64_t>(locals.currentTick) >> (8 * locals.rb_i)) & 0xFF) : 0)
			);
		}

		output.entropyVersion = state.entropyPoolVersion;
	}

	// BuyEntropy procedure:
	// - Removes expired commitments (same sweep)
	// - Checks buyer fee and miner eligibility
	// - Charges buyer and returns requested bytes from slightly older pool version
	PUBLIC_PROCEDURE_WITH_LOCALS(BuyEntropy)
	{
	    locals.currentTick = qpi.tick();
	
	    // Sweep expired commitments
	    for (locals.i = 0; locals.i < state.commitmentCount;)
	    {
	        locals.cmt = state.commitments.get(locals.i);
	        if (!locals.cmt.hasRevealed && locals.currentTick > locals.cmt.revealDeadlineTick)
	        {
	            locals.lostDeposit = locals.cmt.amount;
	            state.lostDepositsRevenue += locals.lostDeposit;
	            state.totalRevenue += locals.lostDeposit;
	            state.pendingShareholderDistribution += locals.lostDeposit;
	            state.totalSecurityDepositsLocked -= locals.lostDeposit;
	            if (locals.i != state.commitmentCount - 1)
	            {
	                locals.cmt = state.commitments.get(state.commitmentCount - 1);
	                state.commitments.set(locals.i, locals.cmt);
	            }
	            state.commitmentCount--;
	        }
	        else
	        {
	            locals.i++;
	        }
	    }
	
	    // Disallow in early-epoch mode -- refund buyer
	    if (qpi.numberOfTickTransactions() == -1)
	    {
	        output.success = false;
	        qpi.transfer(qpi.invocator(), qpi.invocationReward()); // <-- refund buyer
	        return;
	    }
	
	    output.success = false;
	    locals.buyerFee = qpi.invocationReward();
	    locals.eligible = false;
	    locals.usedMinerDeposit = 0;
	
	    // Find an eligible recent miner whose deposit >= minMinerDeposit and who revealed recently
	    for (locals.i = 0; locals.i < state.recentMinerCount; ++locals.i)
	    {
	        locals.recentMinerTemp = state.recentMiners.get(locals.i);
	        if (locals.recentMinerTemp.deposit >= input.minMinerDeposit &&
	            (locals.currentTick - locals.recentMinerTemp.lastRevealTick) <= state.revealTimeoutTicks)
	        {
	            locals.eligible = true;
	            locals.usedMinerDeposit = locals.recentMinerTemp.deposit;
	            break;
	        }
	    }
	
	    if (!locals.eligible)
	    {
	        qpi.transfer(qpi.invocator(), qpi.invocationReward()); // <-- refund buyer (no entropy available)
	        return;
	    }
	
	    // Compute minimum price and check buyer fee
        locals.minPrice = calculatePrice(state, input.numberOfBytes, input.minMinerDeposit);
	
	    if (locals.buyerFee < locals.minPrice)
	    {
	        qpi.transfer(qpi.invocator(), qpi.invocationReward()); // <-- refund buyer (not enough fee)
	        return;
	    }
	
	    // Use the previous-but-one history entry for purchased entropy (to avoid last-second reveals)
	    locals.histIdx = (state.entropyHistoryHead + RANDOM_ENTROPY_HISTORY_LEN - 2) & (RANDOM_ENTROPY_HISTORY_LEN - 1);
	
	    // Produce requested bytes (bounded by RANDOM_RANDOMBYTES_LEN)
	    for (locals.i = 0; locals.i < ((input.numberOfBytes > RANDOM_RANDOMBYTES_LEN) ? RANDOM_RANDOMBYTES_LEN : input.numberOfBytes); ++locals.i)
	    {
	        output.randomBytes.set(
	            locals.i,
	            static_cast<uint8_t>(
	                (
	                    (
	                        (locals.i < 8)   ? state.entropyHistory.get(locals.histIdx).u64._0 :
	                        (locals.i < 16)  ? state.entropyHistory.get(locals.histIdx).u64._1 :
	                        (locals.i < 24)  ? state.entropyHistory.get(locals.histIdx).u64._2 :
	                                           state.entropyHistory.get(locals.histIdx).u64._3
	                    ) >> (8 * (locals.i & 7))
	                ) & 0xFF
	            ) ^
	            (locals.i < 8 ? static_cast<uint8_t>((static_cast<uint64_t>(locals.currentTick) >> (8 * locals.i)) & 0xFF) : 0)
	        );
	    }
	
	    // Return entropy pool/version info and signal success
	    output.entropyVersion = state.entropyPoolVersionHistory.get(locals.histIdx);
	    output.usedMinerDeposit = locals.usedMinerDeposit;
	    output.usedPoolVersion = state.entropyPoolVersionHistory.get(locals.histIdx);
	    output.success = true;
	
	    // Split fee: half to miners pool, half to shareholders
	    locals.half = div(locals.buyerFee, 2ULL);
	    state.minerEarningsPool += locals.half;
	    state.shareholderEarningsPool += (locals.buyerFee - locals.half);
	}

	// GetContractInfo: return public state summary
	PUBLIC_FUNCTION_WITH_LOCALS(GetContractInfo)
	{
		locals.currentTick = qpi.tick();
		locals.activeCount = 0;

		output.totalCommits = state.totalCommits;
		output.totalReveals = state.totalReveals;
		output.totalSecurityDepositsLocked = state.totalSecurityDepositsLocked;
		output.minimumSecurityDeposit = state.minimumSecurityDeposit;
		output.revealTimeoutTicks = state.revealTimeoutTicks;
		output.currentTick = locals.currentTick;
		output.entropyPoolVersion = state.entropyPoolVersion;

		output.totalRevenue = state.totalRevenue;
		output.pendingShareholderDistribution = state.pendingShareholderDistribution;
		output.lostDepositsRevenue = state.lostDepositsRevenue;
		output.minerEarningsPool = state.minerEarningsPool;
		output.shareholderEarningsPool = state.shareholderEarningsPool;
		output.recentMinerCount = state.recentMinerCount;

		// Copy valid deposit amounts
		copyMemory(output.validDepositAmounts, state.validDepositAmounts);
		
		// Count active commitments
		for (locals.i = 0; locals.i < state.commitmentCount; ++locals.i)
		{
			if (!state.commitments.get(locals.i).hasRevealed)
			{
				locals.activeCount++;
			}
		}
		output.activeCommitments = locals.activeCount;
	}

	// GetUserCommitments: list commitments for a user (bounded)
	PUBLIC_FUNCTION_WITH_LOCALS(GetUserCommitments)
	{
		locals.userCommitmentCount = 0;
		for (locals.i = 0; locals.i < state.commitmentCount && locals.userCommitmentCount < RANDOM_MAX_USER_COMMITMENTS; ++locals.i)
		{
			locals.cmt = state.commitments.get(locals.i);
			if (isEqualIdCheck(locals.cmt.invocatorId, input.userId))
			{
				// copy to output buffer
				locals.ucmt.digest = locals.cmt.digest;
				locals.ucmt.amount = locals.cmt.amount;
				locals.ucmt.commitTick = locals.cmt.commitTick;
				locals.ucmt.revealDeadlineTick = locals.cmt.revealDeadlineTick;
				locals.ucmt.hasRevealed = locals.cmt.hasRevealed;
				output.commitments.set(locals.userCommitmentCount, locals.ucmt);
				locals.userCommitmentCount++;
			}
		}
		output.commitmentCount = locals.userCommitmentCount;
	}

	// QueryPrice: compute price for a buyer based on requested bytes and min miner deposit
	PUBLIC_FUNCTION(QueryPrice)
	{
		output.price = calculatePrice(state, input.numberOfBytes, input.minMinerDeposit);
	}

	// END_EPOCH: sweep expired commitments and distribute earnings to recent miners and shareholders
	END_EPOCH_WITH_LOCALS()
	{
		locals.currentTick = qpi.tick();

		// Sweep expired commitments (same logic)
		for (locals.i = 0; locals.i < state.commitmentCount;)
		{
			locals.cmt = state.commitments.get(locals.i);
			if (!locals.cmt.hasRevealed && locals.currentTick > locals.cmt.revealDeadlineTick)
			{
				locals.lostDeposit = locals.cmt.amount;
				state.lostDepositsRevenue += locals.lostDeposit;
				state.totalRevenue += locals.lostDeposit;
				state.pendingShareholderDistribution += locals.lostDeposit;
				state.totalSecurityDepositsLocked -= locals.lostDeposit;

				if (locals.i != state.commitmentCount - 1)
				{
					locals.cmt = state.commitments.get(state.commitmentCount - 1);
					state.commitments.set(locals.i, locals.cmt);
				}
				state.commitmentCount--;
			}
			else
			{
				locals.i++;
			}
		}

		// Pay miners equally from minerEarningsPool
		if (state.minerEarningsPool > 0 && state.recentMinerCount > 0)
		{
			locals.payout = div(state.minerEarningsPool, (uint64)state.recentMinerCount);
			for (locals.i = 0; locals.i < state.recentMinerCount; ++locals.i)
			{
				locals.recentMinerTemp = state.recentMiners.get(locals.i);
				if (!isZeroIdCheck(locals.recentMinerTemp.minerId))
				{
					qpi.transfer(locals.recentMinerTemp.minerId, locals.payout);
				}
			}
			// reset miner pool and recentMiners
			state.minerEarningsPool = 0;
			for (locals.i = 0; locals.i < RANDOM_MAX_RECENT_MINERS; ++locals.i)
			{
				locals.recentMinerTemp.minerId = id::zero();
				locals.recentMinerTemp.deposit = 0;
				locals.recentMinerTemp.lastEntropyVersion = 0;
				locals.recentMinerTemp.lastRevealTick = 0;
				state.recentMiners.set(locals.i, locals.recentMinerTemp);
			}
			state.recentMinerCount = 0;
		}

		// Distribute any pending shareholder distribution (from earnings and/or lost deposits)
		uint64 totalShareholderPayout = state.shareholderEarningsPool + state.pendingShareholderDistribution;
		if (totalShareholderPayout > 0)
		{
		    qpi.distributeDividends(div(totalShareholderPayout, (uint64)NUMBER_OF_COMPUTORS));
		    state.shareholderEarningsPool = 0;
		    state.pendingShareholderDistribution = 0;
		}
	}

	// Register functions and procedures (standard QPI boilerplate)
	REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
	{
		REGISTER_USER_FUNCTION(GetContractInfo, 1);
		REGISTER_USER_FUNCTION(GetUserCommitments, 2);
		REGISTER_USER_FUNCTION(QueryPrice, 3);

		REGISTER_USER_PROCEDURE(RevealAndCommit, 1);
		REGISTER_USER_PROCEDURE(BuyEntropy, 2);
	}

	// INITIALIZE: set defaults and fill valid deposit amounts array (powers of 10)
	INITIALIZE_WITH_LOCALS()
	{
		locals.i = 0;
		locals.j = 0;
		locals.val = 0;

		state.entropyHistoryHead = 0;
		state.minimumSecurityDeposit = 1;
		state.revealTimeoutTicks = 9;
		state.pricePerByte = 10;
		state.priceDepositDivisor = 1000;

		// validDepositAmounts: 1, 10, 100, 1000, ...
		for (locals.i = 0; locals.i < RANDOM_VALID_DEPOSIT_AMOUNTS; ++locals.i)
		{
			locals.val = 1ULL;
			for (locals.j = 0; locals.j < locals.i; ++locals.j)
			{
				locals.val *= 10;
			}
			state.validDepositAmounts.set(locals.i, locals.val);
		}
	}
};
