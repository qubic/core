using namespace QPI;

constexpr uint32 RANDOM_BITFEE = 100;
constexpr uint32 RANDOM_STREAM_CAPACITY = 1365;
constexpr uint32 RANDOM_MAX_PROVIDERS = 4096;

struct RANDOM2
{
};

struct RANDOM : public ContractBase
{
public:
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

	struct Fees_output
	{
		Array<uint32, 16> fees;
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
	};

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

			if(!locals.trusteeOk)
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

		// Reject empty commit before any state change — double-refund risk: reveal path sets
		// the participation flag, then END_TICK would refund the collateral a second time.
		if (input.commit == id::zero())
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		locals.stream = mod<uint32>(qpi.tick(), 3);

		if (input.reveal != BIT4096_ZERO)
		{
			// Reveal path: verify preimage of prior commit and re-commit for next round.
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
			    state.get().revealOrCommitFlags.get(locals.stream * RANDOM_STREAM_CAPACITY + locals.i)) // same-tick commit+reveal is forbidden
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

		state.mut().providers.set(locals.index, qpi.invocator());
		state.mut().collateralTiers.set(locals.index, locals.collateralTier);
		state.mut().commits.set(locals.index, input.commit);
		state.mut().reveals.set(locals.index, BIT4096_ZERO);

		// Lock collateral until future reveal (refund) or no-show (slash).
		state.mut().lockedCollateralAmounts.set(locals.index, qpi.invocationReward());

		state.mut().revealOrCommitFlags.set(locals.index, 1);
		state.mut().revealedThisTickFlags.set(locals.index, 0);

		state.mut().populations.set(locals.stream, locals.i + 1);
	}

	struct END_TICK_locals
	{
		bit_4096 entropy;
		uint32 stream;
		uint32 i, j;
		uint32 index;
		uint32 lastIndex;
		uint32 tier;
		uint16 collateralTierFlags;
		uint64 lockedAmount;
	};

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
				}
				state.mut().providers.set(locals.lastIndex, id::zero());
				state.mut().collateralTiers.set(locals.lastIndex, 0);
				state.mut().commits.set(locals.lastIndex, id::zero());
				state.mut().reveals.set(locals.lastIndex, BIT4096_ZERO);
				state.mut().lockedCollateralAmounts.set(locals.lastIndex, 0);
				state.mut().revealOrCommitFlags.set(locals.lastIndex, 0);
				state.mut().revealedThisTickFlags.set(locals.lastIndex, 0);
				state.mut().contributedToEntropyFlags.set(locals.lastIndex, 0);

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
					}
					// Clear reveal regardless — provider can reveal again next round.
					state.mut().reveals.set(locals.index, BIT4096_ZERO);
				}

				state.mut().revealOrCommitFlags.set(locals.index, 0);
				state.mut().revealedThisTickFlags.set(locals.index, 0);
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
				}
				state.mut().providers.set(locals.lastIndex, id::zero());
				state.mut().collateralTiers.set(locals.lastIndex, 0);
				state.mut().commits.set(locals.lastIndex, id::zero());
				state.mut().reveals.set(locals.lastIndex, BIT4096_ZERO);
				state.mut().lockedCollateralAmounts.set(locals.lastIndex, 0);
				state.mut().revealOrCommitFlags.set(locals.lastIndex, 0);
				state.mut().revealedThisTickFlags.set(locals.lastIndex, 0);
				state.mut().contributedToEntropyFlags.set(locals.lastIndex, 0);

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

	REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
	{
		REGISTER_USER_FUNCTION(Fees, 1);

		REGISTER_USER_PROCEDURE(RevealAndCommit, 1);
		REGISTER_USER_PROCEDURE(BuyEntropy, 2);
	}

	INITIALIZE()
	{
		state.mut().bitFee = RANDOM_BITFEE;
	}
};