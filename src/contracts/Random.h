using namespace QPI;

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
		bit_4096 zeroReveal; // TODO: Use a constant from either QPI or global state
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
	};

	struct StateData
	{
		uint64 earnedAmount;
		uint64 distributedAmount;
		uint64 burnedAmount;

		uint32 bitFee; // Amount of qus

		Array<uint32, 4> populations;        // 3
		Array<id, 4096> providers;           // 3 * 1365
		Array<uint64, 4096> collateralTiers; // 3 * 1365
		Array<id, 4096> commits;             // 3 * 1365
		Array<bit_4096, 4096> reveals;       // 3 * 1365
		bit_4096 revealOrCommitFlags;        // 3 * 1365
		Array<bit_4096, 32> entropy;         // 3 * 10

		// Collateral lifecycle (appended fields):
		// - lockedCollateralAmounts[index] holds the exact stake currently
		//   locked for a provider. It is refunded only when the provider
		//   reveals, and slashed (burned) if they fail to reveal.
		// - revealedThisTickFlags[index] is set when the provider revealed
		//   this tick (vs. only committing), so END_TICK knows whether to mix
		//   their reveal into entropy.
		Array<uint64, 4096> lockedCollateralAmounts; // 3 * 1365
		bit_4096 revealedThisTickFlags;              // 3 * 1365
	};

	PUBLIC_FUNCTION(Fees)
	{
		output.fees.set(0, 100);
		output.fees.set(1, 100);
		output.fees.set(2, 100);
		output.fees.set(3, 100);
		output.fees.set(4, 100);
		output.fees.set(5, 100);
		output.fees.set(6, 100);
		output.fees.set(7, 100);
		output.fees.set(8, 100);
		output.fees.set(9, 100);
	}

	PUBLIC_PROCEDURE_WITH_LOCALS(BuyEntropy)
	{
		locals.entropyCost = static_cast<sint64>(input.numberOfBits) * 100;

		if (input.collateralTier <= 9
			&& input.numberOfBits >= 1 && input.numberOfBits <= 4096
			&& qpi.invocationReward() >= locals.entropyCost)
		{
			// Read from the stream finalized at the previous END_TICK -- the
			// current tick's entropy slot is overwritten at the start of this
			// tick's END_TICK and not yet refilled.
			locals.stream = mod<uint32>(qpi.tick() + 2, 3);
			locals.entropyIdx = locals.stream * 10 + input.collateralTier;
			locals.entropy = state.get().entropy.get(locals.entropyIdx);

			if (locals.entropy == locals.zeroEntropy)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
			}
			else
			{
				state.mut().earnedAmount += static_cast<uint64>(locals.entropyCost);
				if (qpi.invocationReward() > locals.entropyCost)
				{
					// Refund any overpayment beyond the per-bit cost.
					qpi.transfer(qpi.invocator(), qpi.invocationReward() - locals.entropyCost);
				}

				// Copy the first numberOfBits bits of entropy into the output.
				// Remaining bits stay zero (output buffer is zeroed by the framework).
				for (locals.i = 0; locals.i < input.numberOfBits; locals.i++)
				{
					output.entropy.set(locals.i, locals.entropy.get(locals.i));
				}
			}
		}
		else
		{
			// Invalid input: no entropy is delivered, so refund in full.
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
		}
	}

private:
	PUBLIC_PROCEDURE_WITH_LOCALS(RevealAndCommit)
	{
		// TODO: Reject transactions from smart contracts!

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

		// A commit for the next round is mandatory. Reject an empty commit
		// BEFORE touching any state: otherwise the reveal path below would set
		// the participation flag and refund here, and END_TICK would then
		// refund the collateral a second time (double-pay).
		if (input.commit == id::zero())
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		locals.stream = mod<uint32>(qpi.tick(), 3);

		if (input.reveal != locals.zeroReveal)
		{
			// Reveal path: an existing provider reveals the preimage of their
			// previous commit and, in the same transaction, commits for the
			// next round.
			for (locals.i = 0; locals.i < state.get().populations.get(locals.stream); locals.i++)
			{
				if (qpi.invocator() == state.get().providers.get(locals.stream * 1365 + locals.i) &&
				    locals.collateralTier == state.get().collateralTiers.get(locals.stream * 1365 + locals.i))
				{
					break;
				}
			}
			if (locals.i == state.get().populations.get(locals.stream) ||
			    state.get().reveals.get(locals.stream * 1365 + locals.i) != locals.zeroReveal ||
			    qpi.K12(input.reveal) != state.get().commits.get(locals.stream * 1365 + locals.i))
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
				return;
			}

			locals.index = locals.stream * 1365 + locals.i;

			// Refund the collateral locked by the previous commit; the provider
			// fulfilled their obligation by revealing.
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
			if (qpi.invocator() == state.get().providers.get(locals.stream * 1365 + locals.i) &&
			    locals.collateralTier == state.get().collateralTiers.get(locals.stream * 1365 + locals.i))
			{
				// An existing provider cannot replace their commit without
				// first revealing the previous one.
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
				return;
			}
		}
		if (locals.i == 1365)
		{
			// The stream is full.
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		locals.index = locals.stream * 1365 + locals.i;

		state.mut().providers.set(locals.index, qpi.invocator());
		state.mut().collateralTiers.set(locals.index, locals.collateralTier);
		state.mut().commits.set(locals.index, input.commit);
		state.mut().reveals.set(locals.index, locals.zeroReveal);

		// Lock the collateral. It stays in the contract until the provider
		// reveals (refund) or fails to reveal (slash) in a future round.
		state.mut().lockedCollateralAmounts.set(locals.index, qpi.invocationReward());

		state.mut().revealOrCommitFlags.set(locals.index, 1);
		state.mut().revealedThisTickFlags.set(locals.index, 0);

		state.mut().populations.set(locals.stream, locals.i + 1);
	}

	struct END_TICK_locals
	{
		bit_4096 zeroReveal; // TODO: Use a constant from either QPI or global state
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
			state.mut().entropy.set(locals.stream * 10 + locals.i, locals.zeroReveal);
		}

		// Walk providers back-to-front so removed slots can be swap-deleted
		// without disturbing slots not yet visited.
		for (locals.i = state.get().populations.get(locals.stream); locals.i--;)
		{
			locals.index = locals.stream * 1365 + locals.i;
			locals.tier = static_cast<uint32>(state.get().collateralTiers.get(locals.index));

			if (state.get().revealOrCommitFlags.get(locals.index))
			{
				// Provider participated this tick (revealed and/or committed).
				// Their collateral stays locked until they reveal it later.
				if (state.get().revealedThisTickFlags.get(locals.index))
				{
					// A valid reveal contributes to this tier's entropy, unless
					// the tier was already poisoned by a no-show found earlier
					// in the walk.
					if (!(locals.collateralTierFlags & (1 << locals.tier)))
					{
						locals.entropy = state.get().entropy.get(locals.stream * 10 + locals.tier);
						for (locals.j = 0; locals.j < 4096; locals.j++)
						{
							locals.entropy.set(locals.j,
							                   locals.entropy.get(locals.j) ^ state.get().reveals.get(locals.index).get(locals.j));
						}
						state.mut().entropy.set(locals.stream * 10 + locals.tier, locals.entropy);
					}
					// Always clear the reveal so the provider can reveal again
					// next round, even when the XOR above was skipped because
					// the tier was poisoned.
					state.mut().reveals.set(locals.index, locals.zeroReveal);
				}

				state.mut().revealOrCommitFlags.set(locals.index, 0);
				state.mut().revealedThisTickFlags.set(locals.index, 0);
			}
			else
			{
				// Provider did nothing this tick: slash the collateral locked
				// by their last commit and remove them from the pool.
				locals.lockedAmount = state.get().lockedCollateralAmounts.get(locals.index);
				if (locals.lockedAmount > 0)
				{
					qpi.burn(static_cast<sint64>(locals.lockedAmount));
					state.mut().burnedAmount += locals.lockedAmount;
				}

				// A missing reveal invalidates this tier's entropy for the round.
				locals.collateralTierFlags |= (1 << locals.tier);

				// Swap-delete: move the last active provider into this slot.
				locals.lastIndex = locals.stream * 1365 + state.get().populations.get(locals.stream) - 1;
				if (locals.index != locals.lastIndex)
				{
					state.mut().providers.set(locals.index, state.get().providers.get(locals.lastIndex));
					state.mut().collateralTiers.set(locals.index, state.get().collateralTiers.get(locals.lastIndex));
					state.mut().commits.set(locals.index, state.get().commits.get(locals.lastIndex));
					state.mut().reveals.set(locals.index, state.get().reveals.get(locals.lastIndex));
					state.mut().lockedCollateralAmounts.set(locals.index, state.get().lockedCollateralAmounts.get(locals.lastIndex));
					state.mut().revealOrCommitFlags.set(locals.index, state.get().revealOrCommitFlags.get(locals.lastIndex));
					state.mut().revealedThisTickFlags.set(locals.index, state.get().revealedThisTickFlags.get(locals.lastIndex));
				}
				state.mut().providers.set(locals.lastIndex, id::zero());
				state.mut().collateralTiers.set(locals.lastIndex, 0);
				state.mut().commits.set(locals.lastIndex, id::zero());
				state.mut().reveals.set(locals.lastIndex, locals.zeroReveal);
				state.mut().lockedCollateralAmounts.set(locals.lastIndex, 0);
				state.mut().revealOrCommitFlags.set(locals.lastIndex, 0);
				state.mut().revealedThisTickFlags.set(locals.lastIndex, 0);

				state.mut().populations.set(locals.stream, state.get().populations.get(locals.stream) - 1);
			}
		}

		// Drop entropy for any tier that had a no-show this round.
		for (locals.i = 0; locals.i < 10; locals.i++)
		{
			if (locals.collateralTierFlags & (1 << locals.i))
			{
				state.mut().entropy.set(locals.stream * 10 + locals.i, locals.zeroReveal);
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
		// state.mut().bitFee = 1000;
	}
};