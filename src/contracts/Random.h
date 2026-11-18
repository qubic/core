using namespace QPI;

constexpr uint32 RANDOM_BITFEE = 100;
constexpr uint32 RANDOM_STREAM_CAPACITY = 1365;
constexpr uint32 RANDOM_MAX_PROVIDERS = 4096;
constexpr uint64 RANDOM_PROVIDER_REWARD_PERCENT = 50;
constexpr uint64 RANDOM_SHAREHOLDER_REWARD_PERCENT = 50;
constexpr uint64 RANDOM_BURN_PERCENT = 0;

struct RANDOM2
{
};

struct RANDOM : public ContractBase
{
public:
	STATIC_ASSERT(RANDOM_PROVIDER_REWARD_PERCENT <= 100 && RANDOM_SHAREHOLDER_REWARD_PERCENT <= 100 && RANDOM_BURN_PERCENT <= 100 &&
	                  RANDOM_PROVIDER_REWARD_PERCENT + RANDOM_SHAREHOLDER_REWARD_PERCENT + RANDOM_BURN_PERCENT == 100,
	              RANDOM_REVENUE_PERCENTAGES_MUST_TOTAL_100);

	struct RevealAndCommit_input
	{
		bit_4096 reveal;
		id commit;
	};

	struct RevealAndCommit_output
	{
	};

	struct RevealAndCommit_locals
	{
		uint32 stream;
		uint32 collateralTier;
		uint32 i;
		uint32 index;
	};

	struct Fees_input
	{
	};

	using FundReserve_input = NoData;

	struct FundReserve_output
	{
		sint64 amount;
	};

	struct Fees_output
	{
		Array<uint32, 16> fees;
	};

	using GetRevenueInfo_input = NoData;

	struct GetRevenueInfo_output
	{
		uint64 burnedAmount;
		uint64 revenueBurnedAmount;
		uint64 distributedAmount;
		uint64 earnedAmount;
	};

	struct BuyEntropy_input
	{
		uint8 collateralTier;
		uint16 numberOfBits;
		id trustee;
	};

	struct BuyEntropy_output
	{
		bit_4096 entropy;
	};

	struct BuyEntropy_locals
	{
		bit_4096 zeroEntropy;
		bit_4096 entropy;
		uint64 i;
		uint64 entropyIdx;
		sint64 entropyCost;
		uint32 stream;
		uint32 index;
		sint8 trusteeOk;
	};

	// Read-only status query so an off-chain provider can discover *where* it is
	// enrolled (which streams/tiers) and whether it is still active, without having
	// to know its stream/tier in advance. If count == 0 the provider is not enrolled
	// anywhere and should (re-)join with a first-commit (reveal == 0).
	struct GetProviderStatus_input
	{
		id provider;
	};

	struct GetProviderStatus_output
	{
		uint32 count;                          // number of (stream, tier) slots held by `provider`
		Array<uint32, 32> stream;              // stream of slot k
		Array<uint32, 32> collateralTier;      // collateral tier of slot k
		Array<uint64, 32> lockedCollateral;    // collateral currently locked for slot k
		Array<uint8, 32> contributedToEntropy; // 1 if slot k's reveal was mixed into entropy this cycle
		// Tick of slot k's last accepted RevealAndCommit call. A client that submitted
		// a tx targeting tick T can compare this to T after the fact: equal means the
		// tx was accepted; anything earlier means it was rejected (wrong preimage,
		// same-tick resubmit, stream full, etc.) and the attached amount was refunded.
		Array<uint32, 32> lastUpdateTick;
	};

	struct GetProviderStatus_locals
	{
		uint32 s;
		uint32 i;
		uint32 index;
		uint32 count;
	};

	struct END_TICK_locals
	{
		bit_4096 entropy;
		uint64 lockedAmount;
		uint64 contributions;
		uint32 stream;
		uint32 i, j;
		uint32 index;
		uint32 lastIndex;
		uint32 tier;
		uint16 collateralTierFlags;
	};

	struct ProviderDividend_input
	{
		uint64 pool;
		uint64 contributions;
		uint64 total;
	};

	struct ProviderDividend_output
	{
		uint64 amount;
	};

	struct ProviderDividend_locals
	{
		uint128 weightedPool;
	};

	using RestoreProviderAccounting_input = NoData;

	using RestoreProviderAccounting_output = NoData;

	struct RestoreProviderAccounting_locals
	{
		id provider;
		uint32 stream;
		uint32 i;
	};

	struct BEGIN_EPOCH_locals
	{
		RestoreProviderAccounting_input restoreInput;
		RestoreProviderAccounting_output restoreOutput;
	};

	using DistributeEpochRewards_input = NoData;

	using DistributeEpochRewards_output = NoData;

	struct DistributeEpochRewards_locals
	{
		ProviderDividend_input dividendInput;
		ProviderDividend_output dividendOutput;
		uint64 revenue;
		uint64 shareholderPool;
		uint64 burnAmount;
		uint64 amountPerShare;
		sint64 element;
	};

	struct END_EPOCH_locals
	{
		DistributeEpochRewards_input rewardsInput;
		DistributeEpochRewards_output rewardsOutput;
	};

	struct StateData
	{
		uint64 earnedAmount;
		uint64 distributedAmount;
		uint64 burnedAmount;

		uint32 bitFee;
		
		Array<uint32, 4> populations;
		Array<id, RANDOM_MAX_PROVIDERS> providers;
		Array<uint64, RANDOM_MAX_PROVIDERS> collateralTiers;
		Array<id, RANDOM_MAX_PROVIDERS> commits;
		Array<bit_4096, RANDOM_MAX_PROVIDERS> reveals;
		bit_4096 revealOrCommitFlags;
		Array<bit_4096, 32> entropy;

		// lockedCollateralAmounts: stake locked per-provider; refunded on reveal, burned on no-show.
		// revealedThisTickFlags: set when provider revealed this tick (not just committed).
		Array<uint64, RANDOM_MAX_PROVIDERS> lockedCollateralAmounts;
		bit_4096 revealedThisTickFlags;
		// Cleared each END_TICK; set when reveal is XOR'd into entropy. BuyEntropy uses for trustee verification.
		bit_4096 contributedToEntropyFlags;
		// Tick of the last RevealAndCommit call accepted for this slot. Never touched
		// on a rejected call, so it is a durable, tick-precise signal off-chain clients
		// can use (via GetProviderStatus) to tell whether a submitted tx took effect.
		Array<uint32, RANDOM_MAX_PROVIDERS> lastUpdateTick;

		// Append-only state extension. Entries survive slot removal until epoch settlement.
		HashMap<id, uint64, RANDOM_MAX_PROVIDERS> epochContributions;
		uint64 totalEpochContributions;
		// Only burns paid from sales revenue reduce the amount available for future distributions.
		uint64 revenueBurnedAmount;
	};

	/** @brief Returns the cumulative accounting counters in QU without changing state.
	 * @param input No parameters.
	 * @param output Recorded burns, revenue burns, distributions, and earned revenue.
	 */
	PUBLIC_FUNCTION(GetRevenueInfo)
	{
		output.burnedAmount = state.get().burnedAmount;
		output.revenueBurnedAmount = state.get().revenueBurnedAmount;
		output.distributedAmount = state.get().distributedAmount;
		output.earnedAmount = state.get().earnedAmount;
	}

	PUBLIC_FUNCTION(Fees)
	{
		output.fees.set(0, state.get().bitFee);
		output.fees.set(1, state.get().bitFee);
		output.fees.set(2, state.get().bitFee);
		output.fees.set(3, state.get().bitFee);
		output.fees.set(4, state.get().bitFee);
		output.fees.set(5, state.get().bitFee);
		output.fees.set(6, state.get().bitFee);
		output.fees.set(7, state.get().bitFee);
		output.fees.set(8, state.get().bitFee);
		output.fees.set(9, state.get().bitFee);
	}

	PUBLIC_FUNCTION_WITH_LOCALS(GetProviderStatus)
	{
		// Scan every stream for slots owned by input.provider and report each one.
		// A provider may hold several slots (different streams and/or tiers).
		locals.count = 0;
		for (locals.s = 0; locals.s < 3; locals.s++)
		{
			for (locals.i = 0; locals.i < state.get().populations.get(locals.s); locals.i++)
			{
				locals.index = locals.s * RANDOM_STREAM_CAPACITY + locals.i;
				if (state.get().providers.get(locals.index) == input.provider && locals.count < 32)
				{
					output.stream.set(locals.count, locals.s);
					output.collateralTier.set(locals.count, static_cast<uint32>(state.get().collateralTiers.get(locals.index)));
					output.lockedCollateral.set(locals.count, state.get().lockedCollateralAmounts.get(locals.index));
					output.lastUpdateTick.set(locals.count, state.get().lastUpdateTick.get(locals.index));
					output.contributedToEntropy.set(locals.count, 0);
					if (state.get().contributedToEntropyFlags.get(locals.index))
					{
						output.contributedToEntropy.set(locals.count, 1);
					}
					locals.count++;
				}
			}
		}
		output.count = locals.count;
	}

	PUBLIC_PROCEDURE_WITH_LOCALS(BuyEntropy)
	{
		locals.entropyCost = static_cast<sint64>(input.numberOfBits) * state.get().bitFee;

		if (input.collateralTier <= 9
			&& input.numberOfBits >= 1 && input.numberOfBits <= 4096
			&& qpi.invocationReward() >= locals.entropyCost)
		{
			// Offset +2: skip the current tick's slot (being overwritten) and read the last finalized stream.
			locals.stream = mod<uint32>(qpi.tick() + 2, 3);
			locals.entropyIdx = locals.stream * 10 + input.collateralTier;
			locals.entropy = state.get().entropy.get(locals.entropyIdx);

			locals.trusteeOk = (input.trustee == id::zero()) ? 1 : 0;

			if (!locals.trusteeOk)
			{
				for (locals.i = 0; locals.i < state.get().populations.get(locals.stream); locals.i++)
				{
					locals.index = locals.stream * RANDOM_STREAM_CAPACITY + static_cast<uint32>(locals.i);
					if (input.trustee == state.get().providers.get(locals.index)
						&& input.collateralTier == state.get().collateralTiers.get(locals.index)
						&& state.get().contributedToEntropyFlags.get(locals.index))
					{
						locals.trusteeOk = 1;
						break;
					}
				}
			}

			if (locals.entropy == locals.zeroEntropy || !locals.trusteeOk)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			else
			{
				state.mut().earnedAmount += static_cast<uint64>(locals.entropyCost);
				if (qpi.invocationReward() > locals.entropyCost)
				{
					// Refund overpayment.
					qpi.transfer(qpi.invocator(), qpi.invocationReward() - locals.entropyCost);
				}

				// Copy requested bits; remaining output bits are zero (framework-zeroed).
				for (locals.i = 0; locals.i < input.numberOfBits; locals.i++)
				{
					output.entropy.set(locals.i, locals.entropy.get(locals.i));
				}
			}
		}
		else
		{
			// Invalid input — refund in full.
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}
	}

private:
	PUBLIC_PROCEDURE_WITH_LOCALS(RevealAndCommit)
	{
		// Entropy providers must be user accounts, not smart contracts.
		if (qpi.isContractId(qpi.invocator()))
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		switch (qpi.invocationReward())
		{
			case 1: locals.collateralTier = 0; break;

			case 10: locals.collateralTier = 1; break;

			case 100: locals.collateralTier = 2; break;

			case 1000: locals.collateralTier = 3; break;

			case 10000: locals.collateralTier = 4; break;

			case 100000: locals.collateralTier = 5; break;

			case 1000000: locals.collateralTier = 6; break;

			case 10000000: locals.collateralTier = 7; break;

			case 100000000: locals.collateralTier = 8; break;

			case 1000000000: locals.collateralTier = 9; break;

			default: qpi.transfer(qpi.invocator(), qpi.invocationReward()); return;
		}

		locals.stream = mod<uint32>(qpi.tick(), 3);

		// Empty commit = "reveal and leave": the provider reveals its outstanding
		// commit and unsubscribes. The reveal is still recorded here so END_TICK mixes
		// it into this round's entropy (count-it); END_TICK then evicts the provider
		// (identified by commit == 0) with no slashing (then-leave). Collateral is
		// returned now and lockedCollateralAmounts is zeroed so END_TICK cannot refund
		// it a second time — the exact double-refund the old guard avoided by rejecting.
		if (input.commit == id::zero())
		{
			if (input.reveal == BIT4096_ZERO)
			{
				// Nothing to reveal — cannot safely leave with an outstanding commitment.
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
				return;
			}

			for (locals.i = 0; locals.i < state.get().populations.get(locals.stream); locals.i++)
			{
				if (qpi.invocator() == state.get().providers.get(locals.stream * RANDOM_STREAM_CAPACITY + locals.i) &&
				    locals.collateralTier == state.get().collateralTiers.get(locals.stream * RANDOM_STREAM_CAPACITY + locals.i))
				{
					break;
				}
			}
			if (locals.i == state.get().populations.get(locals.stream) ||
			    state.get().reveals.get(locals.stream * RANDOM_STREAM_CAPACITY + locals.i) != BIT4096_ZERO ||
			    qpi.K12(input.reveal) != state.get().commits.get(locals.stream * RANDOM_STREAM_CAPACITY + locals.i) ||
			    state.get().revealOrCommitFlags.get(locals.stream * RANDOM_STREAM_CAPACITY + locals.i)) // not found / stale / wrong preimage / same-tick
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
				return;
			}

			locals.index = locals.stream * RANDOM_STREAM_CAPACITY + locals.i;

			// Return the collateral locked for the revealed commit, plus the amount
			// attached to this call (used only to select the tier; nothing is re-committed).
			if (state.get().lockedCollateralAmounts.get(locals.index) > 0)
			{
				qpi.transfer(qpi.invocator(), state.get().lockedCollateralAmounts.get(locals.index));
			}
			qpi.transfer(qpi.invocator(), qpi.invocationReward());

			// Record the reveal for END_TICK entropy mixing, then flag the slot as leaving.
			state.mut().reveals.set(locals.index, input.reveal);
			state.mut().lockedCollateralAmounts.set(locals.index, 0);
			state.mut().commits.set(locals.index, id::zero()); // leaving marker: enrolled providers always keep a non-zero commit
			state.mut().revealOrCommitFlags.set(locals.index, 1);
			state.mut().revealedThisTickFlags.set(locals.index, 1);
			state.mut().lastUpdateTick.set(locals.index, qpi.tick());

			return;
		}

		if (input.reveal != BIT4096_ZERO)
		{
			// Reveal path: verify preimage of prior commit and re-commit for next round.
			// A provider that isn't registered (e.g. evicted by END_TICK after missing a
			// reveal) is rejected here — it must re-enroll with a fresh first-commit
			// (reveal == 0). The contract does NOT silently re-enroll it; off-chain
			// clients detect eviction via the GetProviderStatus function and decide.
			for (locals.i = 0; locals.i < state.get().populations.get(locals.stream); locals.i++)
			{
				if (qpi.invocator() == state.get().providers.get(locals.stream * RANDOM_STREAM_CAPACITY + locals.i) &&
				    locals.collateralTier == state.get().collateralTiers.get(locals.stream * RANDOM_STREAM_CAPACITY + locals.i))
				{
					break;
				}
			}
			if (locals.i == state.get().populations.get(locals.stream) ||
			    state.get().reveals.get(locals.stream * RANDOM_STREAM_CAPACITY + locals.i) != BIT4096_ZERO ||
			    qpi.K12(input.reveal) != state.get().commits.get(locals.stream * RANDOM_STREAM_CAPACITY + locals.i) ||
			    state.get().revealOrCommitFlags.get(locals.stream * RANDOM_STREAM_CAPACITY + locals.i)) // not found / stale / wrong preimage / same-tick
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
				return;
			}

			locals.index = locals.stream * RANDOM_STREAM_CAPACITY + locals.i;

			// Refund prior collateral — reveal fulfills obligation.
			if (state.get().lockedCollateralAmounts.get(locals.index) > 0)
			{
				qpi.transfer(qpi.invocator(), state.get().lockedCollateralAmounts.get(locals.index));
			}

			// Record the reveal for END_TICK entropy mixing.
			state.mut().reveals.set(locals.index, input.reveal);

			// Store the next commit and lock fresh collateral for it.
			state.mut().commits.set(locals.index, input.commit);
			state.mut().lockedCollateralAmounts.set(locals.index, qpi.invocationReward());

			state.mut().revealOrCommitFlags.set(locals.index, 1);
			state.mut().revealedThisTickFlags.set(locals.index, 1);
			state.mut().lastUpdateTick.set(locals.index, qpi.tick());

			return;
		}

		// First-commit path: register a brand-new provider for this stream/tier.
		for (locals.i = 0; locals.i < state.get().populations.get(locals.stream); locals.i++)
		{
			if (qpi.invocator() == state.get().providers.get(locals.stream * RANDOM_STREAM_CAPACITY + locals.i) &&
			    locals.collateralTier == state.get().collateralTiers.get(locals.stream * RANDOM_STREAM_CAPACITY + locals.i))
			{
				// Existing provider must reveal before re-committing.
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
				return;
			}
		}
		if (locals.i == RANDOM_STREAM_CAPACITY)
		{
			// Stream is full.
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		locals.index = locals.stream * RANDOM_STREAM_CAPACITY + locals.i;

		// Reserve accounting capacity before accepting a new address or locking its collateral.
		if (!state.get().epochContributions.contains(qpi.invocator()) && state.mut().epochContributions.set(qpi.invocator(), 0) == NULL_INDEX)
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		state.mut().providers.set(locals.index, qpi.invocator());
		state.mut().collateralTiers.set(locals.index, locals.collateralTier);
		state.mut().commits.set(locals.index, input.commit);
		state.mut().reveals.set(locals.index, BIT4096_ZERO);

		// Lock collateral until future reveal (refund) or no-show (slash).
		state.mut().lockedCollateralAmounts.set(locals.index, qpi.invocationReward());

		state.mut().revealOrCommitFlags.set(locals.index, 1);
		state.mut().revealedThisTickFlags.set(locals.index, 0);
		state.mut().lastUpdateTick.set(locals.index, qpi.tick());

		state.mut().populations.set(locals.stream, locals.i + 1);
	}

	END_TICK_WITH_LOCALS()
	{
		locals.stream = mod<uint32>(qpi.tick(), 3);

		// Entropy for this stream is recomputed from scratch every cycle.
		for (locals.i = 0; locals.i < 10; locals.i++)
		{
			state.mut().entropy.set(locals.stream * 10 + locals.i, BIT4096_ZERO);
		}

		// Reset contribution flags; only this tick's reveals can satisfy BuyEntropy trustee checks.
		for (locals.i = 0; locals.i < state.get().populations.get(locals.stream); locals.i++)
		{
			state.mut().contributedToEntropyFlags.set(
				locals.stream * RANDOM_STREAM_CAPACITY + locals.i, 0);
		}

		// Check if any provider revealed this tick.
		for (locals.i = 0; locals.i < state.get().populations.get(locals.stream); locals.i++)
		{
			if (state.get().revealedThisTickFlags.get(locals.stream * RANDOM_STREAM_CAPACITY + locals.i))
			{
				break;
			}
		}
		if (locals.i == state.get().populations.get(locals.stream))
		{
			// Empty tick: no reveals. Refund stale (unflagged) providers; keep fresh commits.
			// No-shows are not slashed when the whole pool went silent.
			for (locals.i = state.get().populations.get(locals.stream); locals.i--;)
			{
				locals.index = locals.stream * RANDOM_STREAM_CAPACITY + locals.i;
				if (state.get().revealOrCommitFlags.get(locals.index))
				{
					state.mut().revealOrCommitFlags.set(locals.index, 0);
					continue;
				}

				locals.lockedAmount = state.get().lockedCollateralAmounts.get(locals.index);
				if (locals.lockedAmount > 0)
				{
					qpi.transfer(state.get().providers.get(locals.index),
					             static_cast<sint64>(locals.lockedAmount));
				}

				// Swap-delete: move the last active provider into this slot.
				locals.lastIndex = locals.stream * RANDOM_STREAM_CAPACITY + state.get().populations.get(locals.stream) - 1;
				if (locals.index != locals.lastIndex)
				{
					state.mut().providers.set(locals.index, state.get().providers.get(locals.lastIndex));
					state.mut().collateralTiers.set(locals.index, state.get().collateralTiers.get(locals.lastIndex));
					state.mut().commits.set(locals.index, state.get().commits.get(locals.lastIndex));
					state.mut().reveals.set(locals.index, state.get().reveals.get(locals.lastIndex));
					state.mut().lockedCollateralAmounts.set(locals.index, state.get().lockedCollateralAmounts.get(locals.lastIndex));
					state.mut().revealOrCommitFlags.set(locals.index, state.get().revealOrCommitFlags.get(locals.lastIndex));
					state.mut().revealedThisTickFlags.set(locals.index, state.get().revealedThisTickFlags.get(locals.lastIndex));
					state.mut().contributedToEntropyFlags.set(locals.index, state.get().contributedToEntropyFlags.get(locals.lastIndex));
					state.mut().lastUpdateTick.set(locals.index, state.get().lastUpdateTick.get(locals.lastIndex));
				}
				state.mut().providers.set(locals.lastIndex, id::zero());
				state.mut().collateralTiers.set(locals.lastIndex, 0);
				state.mut().commits.set(locals.lastIndex, id::zero());
				state.mut().reveals.set(locals.lastIndex, BIT4096_ZERO);
				state.mut().lockedCollateralAmounts.set(locals.lastIndex, 0);
				state.mut().revealOrCommitFlags.set(locals.lastIndex, 0);
				state.mut().revealedThisTickFlags.set(locals.lastIndex, 0);
				state.mut().contributedToEntropyFlags.set(locals.lastIndex, 0);
				state.mut().lastUpdateTick.set(locals.lastIndex, 0);

				state.mut().populations.set(locals.stream, state.get().populations.get(locals.stream) - 1);
			}
			return;
		}

		// Back-to-front walk so swap-delete doesn't disturb unvisited slots.
		for (locals.i = state.get().populations.get(locals.stream); locals.i--;)
		{
			locals.index = locals.stream * RANDOM_STREAM_CAPACITY + locals.i;
			locals.tier = static_cast<uint32>(state.get().collateralTiers.get(locals.index));

			if (state.get().revealOrCommitFlags.get(locals.index))
			{
				// Participated this tick; collateral remains locked.
				if (state.get().revealedThisTickFlags.get(locals.index))
				{
					// XOR reveal into tier entropy unless the tier is already poisoned by a no-show.
					if (!(locals.collateralTierFlags & (1 << locals.tier)))
					{
						locals.entropy = state.get().entropy.get(locals.stream * 10 + locals.tier);
						for (locals.j = 0; locals.j < 4096; locals.j++)
						{
							locals.entropy.set(locals.j,
							                   locals.entropy.get(locals.j) ^ state.get().reveals.get(locals.index).get(locals.j));
						}
						state.mut().entropy.set(locals.stream * 10 + locals.tier, locals.entropy);
						// Mark contribution for BuyEntropy trustee verification.
						state.mut().contributedToEntropyFlags.set(locals.index, 1);

						// Count the mixing event, even if this tier is invalidated later.
						locals.contributions = 0;
						state.get().epochContributions.get(state.get().providers.get(locals.index), locals.contributions);
						state.mut().epochContributions.set(state.get().providers.get(locals.index), locals.contributions + 1);
						state.mut().totalEpochContributions++;
					}
					// Clear reveal regardless — provider can reveal again next round.
					state.mut().reveals.set(locals.index, BIT4096_ZERO);
				}

				state.mut().revealOrCommitFlags.set(locals.index, 0);
				state.mut().revealedThisTickFlags.set(locals.index, 0);

				// "Reveal and leave": an empty commit (commit == 0) marks a provider that
				// revealed and unsubscribed. Its reveal was mixed into entropy above; now
				// remove it. Collateral was already returned in RevealAndCommit
				// (lockedCollateralAmounts is 0), so there is nothing to burn or refund.
				if (state.get().commits.get(locals.index) == id::zero())
				{
					// Swap-delete: move the last active provider into this slot.
					locals.lastIndex = locals.stream * RANDOM_STREAM_CAPACITY + state.get().populations.get(locals.stream) - 1;
					if (locals.index != locals.lastIndex)
					{
						state.mut().providers.set(locals.index, state.get().providers.get(locals.lastIndex));
						state.mut().collateralTiers.set(locals.index, state.get().collateralTiers.get(locals.lastIndex));
						state.mut().commits.set(locals.index, state.get().commits.get(locals.lastIndex));
						state.mut().reveals.set(locals.index, state.get().reveals.get(locals.lastIndex));
						state.mut().lockedCollateralAmounts.set(locals.index, state.get().lockedCollateralAmounts.get(locals.lastIndex));
						state.mut().revealOrCommitFlags.set(locals.index, state.get().revealOrCommitFlags.get(locals.lastIndex));
						state.mut().revealedThisTickFlags.set(locals.index, state.get().revealedThisTickFlags.get(locals.lastIndex));
						state.mut().contributedToEntropyFlags.set(locals.index, state.get().contributedToEntropyFlags.get(locals.lastIndex));
						state.mut().lastUpdateTick.set(locals.index, state.get().lastUpdateTick.get(locals.lastIndex));
					}
					state.mut().providers.set(locals.lastIndex, id::zero());
					state.mut().collateralTiers.set(locals.lastIndex, 0);
					state.mut().commits.set(locals.lastIndex, id::zero());
					state.mut().reveals.set(locals.lastIndex, BIT4096_ZERO);
					state.mut().lockedCollateralAmounts.set(locals.lastIndex, 0);
					state.mut().revealOrCommitFlags.set(locals.lastIndex, 0);
					state.mut().revealedThisTickFlags.set(locals.lastIndex, 0);
					state.mut().contributedToEntropyFlags.set(locals.lastIndex, 0);
					state.mut().lastUpdateTick.set(locals.lastIndex, 0);

					state.mut().populations.set(locals.stream, state.get().populations.get(locals.stream) - 1);
				}
			}
			else
			{
				// No-show: burn collateral and evict from pool.
				locals.lockedAmount = state.get().lockedCollateralAmounts.get(locals.index);
				if (locals.lockedAmount > 0)
				{
					qpi.burn(static_cast<sint64>(locals.lockedAmount));
					state.mut().burnedAmount += locals.lockedAmount;
				}

				// A missing reveal invalidates this tier's entropy for the round.
				locals.collateralTierFlags |= (1 << locals.tier);

				// Swap-delete: move the last active provider into this slot.
				locals.lastIndex = locals.stream * RANDOM_STREAM_CAPACITY + state.get().populations.get(locals.stream) - 1;
				if (locals.index != locals.lastIndex)
				{
					state.mut().providers.set(locals.index, state.get().providers.get(locals.lastIndex));
					state.mut().collateralTiers.set(locals.index, state.get().collateralTiers.get(locals.lastIndex));
					state.mut().commits.set(locals.index, state.get().commits.get(locals.lastIndex));
					state.mut().reveals.set(locals.index, state.get().reveals.get(locals.lastIndex));
					state.mut().lockedCollateralAmounts.set(locals.index, state.get().lockedCollateralAmounts.get(locals.lastIndex));
					state.mut().revealOrCommitFlags.set(locals.index, state.get().revealOrCommitFlags.get(locals.lastIndex));
					state.mut().revealedThisTickFlags.set(locals.index, state.get().revealedThisTickFlags.get(locals.lastIndex));
					state.mut().contributedToEntropyFlags.set(locals.index, state.get().contributedToEntropyFlags.get(locals.lastIndex));
					state.mut().lastUpdateTick.set(locals.index, state.get().lastUpdateTick.get(locals.lastIndex));
				}
				state.mut().providers.set(locals.lastIndex, id::zero());
				state.mut().collateralTiers.set(locals.lastIndex, 0);
				state.mut().commits.set(locals.lastIndex, id::zero());
				state.mut().reveals.set(locals.lastIndex, BIT4096_ZERO);
				state.mut().lockedCollateralAmounts.set(locals.lastIndex, 0);
				state.mut().revealOrCommitFlags.set(locals.lastIndex, 0);
				state.mut().revealedThisTickFlags.set(locals.lastIndex, 0);
				state.mut().contributedToEntropyFlags.set(locals.lastIndex, 0);
				state.mut().lastUpdateTick.set(locals.lastIndex, 0);

				state.mut().populations.set(locals.stream, state.get().populations.get(locals.stream) - 1);
			}
		}

		// Drop entropy for any tier that had a no-show this round.
		for (locals.i = 0; locals.i < 10; locals.i++)
		{
			if (locals.collateralTierFlags & (1 << locals.i))
			{
				state.mut().entropy.set(locals.stream * 10 + locals.i, BIT4096_ZERO);
			}
		}
	}

	BEGIN_EPOCH_WITH_LOCALS()
	{
		CALL(RestoreProviderAccounting, locals.restoreInput, locals.restoreOutput);
	}

	END_EPOCH_WITH_LOCALS()
	{
		CALL(DistributeEpochRewards, locals.rewardsInput, locals.rewardsOutput);
	}

	/**
	 * Burns the attached QU to replenish RANDOM's execution fee reserve.
	 * @param input Empty; the amount is supplied as the invocation reward.
	 * @param output Amount burned, or -1 for a nonpositive amount or failed burn.
	 * @note Failed burns are refunded. Funding is excluded from dividend revenue.
	 */
	PUBLIC_PROCEDURE(FundReserve)
	{
		output.amount = -1;

		if (qpi.invocationReward() <= 0)
		{
			return;
		}

		if (qpi.burn(qpi.invocationReward()) >= 0)
		{
			output.amount = qpi.invocationReward();
		}
		else
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}
	}

	PRIVATE_PROCEDURE_WITH_LOCALS(RestoreProviderAccounting)
	{
		// Also seeds existing providers on activation of the appended, zero-padded state.
		for (locals.stream = 0; locals.stream < 3; locals.stream++)
		{
			for (locals.i = 0; locals.i < state.get().populations.get(locals.stream); locals.i++)
			{
				locals.provider = state.get().providers.get(locals.stream * RANDOM_STREAM_CAPACITY + locals.i);
				if (!state.get().epochContributions.contains(locals.provider))
				{
					state.mut().epochContributions.set(locals.provider, 0);
				}
			}
		}
	}

	PRIVATE_FUNCTION_WITH_LOCALS(ProviderDividend)
	{
		output.amount = 0;

		if (input.total == 0 || input.contributions == 0 || input.pool == 0 || input.contributions > input.total)
		{
			return;
		}

		// A 128-bit product holds two uint64 factors; contributions <= total bounds the result by pool.
		locals.weightedPool = static_cast<uint128>(input.pool) * static_cast<uint128>(input.contributions);
		output.amount = div<uint128>(locals.weightedPool, input.total).low;
	}

	PRIVATE_PROCEDURE_WITH_LOCALS(DistributeEpochRewards)
	{
		if (state.get().earnedAmount > state.get().distributedAmount &&
		    state.get().earnedAmount - state.get().distributedAmount > state.get().revenueBurnedAmount)
		{
			locals.revenue = state.get().earnedAmount - state.get().distributedAmount - state.get().revenueBurnedAmount;
			locals.dividendInput.pool = div(smul(locals.revenue, RANDOM_PROVIDER_REWARD_PERCENT), 100ULL);
			locals.shareholderPool = div(smul(locals.revenue, RANDOM_SHAREHOLDER_REWARD_PERCENT), 100ULL);
			locals.burnAmount = div(smul(locals.revenue, RANDOM_BURN_PERCENT), 100ULL);
			locals.dividendInput.total = state.get().totalEpochContributions;
			locals.amountPerShare = div(locals.shareholderPool, static_cast<uint64>(NUMBER_OF_COMPUTORS));

			for (locals.element = state.get().epochContributions.nextElementIndex(NULL_INDEX); locals.element != NULL_INDEX;
			     locals.element = state.get().epochContributions.nextElementIndex(locals.element))
			{
				locals.dividendInput.contributions = state.get().epochContributions.value(locals.element);
				CALL(ProviderDividend, locals.dividendInput, locals.dividendOutput);

				if (locals.dividendOutput.amount > 0 &&
				    qpi.transfer(state.get().epochContributions.key(locals.element), static_cast<sint64>(locals.dividendOutput.amount)) >= 0)
				{
					state.mut().distributedAmount += locals.dividendOutput.amount;
				}
			}

			if (locals.amountPerShare > 0 && qpi.distributeDividends(static_cast<sint64>(locals.amountPerShare)))
			{
				state.mut().distributedAmount += locals.amountPerShare * NUMBER_OF_COMPUTORS;
			}

			if (locals.burnAmount > 0 && qpi.burn(static_cast<sint64>(locals.burnAmount)) >= 0)
			{
				state.mut().revenueBurnedAmount += locals.burnAmount;
				state.mut().burnedAmount += locals.burnAmount;
			}
		}

		// Unspent revenue (excluding successful revenue burns) is shared again next epoch.
		state.mut().epochContributions.reset();
		state.mut().totalEpochContributions = 0;
	}

	REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
	{
		REGISTER_USER_FUNCTION(Fees, 1);
		REGISTER_USER_FUNCTION(GetProviderStatus, 2);
		REGISTER_USER_FUNCTION(GetRevenueInfo, 3);

		REGISTER_USER_PROCEDURE(RevealAndCommit, 1);
		REGISTER_USER_PROCEDURE(BuyEntropy, 2);
		REGISTER_USER_PROCEDURE(FundReserve, 3);
	}

	INITIALIZE()
	{
		state.mut().bitFee = RANDOM_BITFEE;
	}
};
