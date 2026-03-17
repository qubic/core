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
	};

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

		locals.stream = mod<uint32>(qpi.tick(), 3);

		if (input.reveal != locals.zeroReveal) // Don't need to initialize [locals.zeroReveal] because
		                                       // locals struct has been zeroed (bad practice, but
		                                       // it's for spreading awareness about this nuance)
		{
			for (; locals.i < state.get().populations.get(locals.stream); locals.i++) // Don't need to initialize [locals.i] because locals
			                                                                          // struct has been zeroed (bad practice, but it's for
			                                                                          // spreading awareness about this nuance)
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

			state.mut().reveals.set(locals.stream * 1365 + locals.i, input.reveal);
			state.mut().revealOrCommitFlags.set(locals.stream * 1365 + locals.i, 1);
		}

		if (input.commit == id::zero())
		{
			qpi.transfer(qpi.invocator(), qpi.invocationReward());
			return;
		}

		for (locals.i = 0; locals.i < state.get().populations.get(locals.stream); locals.i++)
		{
			if (qpi.invocator() == state.get().providers.get(locals.stream * 1365 + locals.i) &&
			    locals.collateralTier == state.get().collateralTiers.get(locals.stream * 1365 + locals.i))
			{
				break;
			}
		}
		if (locals.i == state.get().populations.get(locals.stream))
		{
			if (locals.i == 1365)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
				return;
			}

			state.mut().providers.set(locals.stream * 1365 + locals.i, qpi.invocator());
			state.mut().collateralTiers.set(locals.stream * 1365 + locals.i, locals.collateralTier);
			state.mut().populations.set(locals.stream, locals.i + 1);
		}
		else
		{
			if (state.get().reveals.get(locals.stream * 1365 + locals.i) == locals.zeroReveal)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward());
				return;
			}
		}
		state.mut().commits.set(locals.stream * 1365 + locals.i, input.commit);
		state.mut().revealOrCommitFlags.set(locals.stream * 1365 + locals.i, 1);
	}

	struct END_TICK_locals
	{
		bit_4096 zeroReveal; // TODO: Use a constant from either QPI or global state
		bit_4096 entropy;
		id collateralRecipient;
		uint32 stream;
		uint32 i, j;
		uint16 collateralTierFlags;
	};

	END_TICK_WITH_LOCALS()
	{
		locals.stream = mod<uint32>(qpi.tick(), 3);

		for (; locals.i < 10; locals.i++) // Don't need to initialize [locals.i] because locals
		                                  // struct has been zeroed (bad practice, but it's for
		                                  // spreading awareness about this nuance)
		{
			state.mut().entropy.set(locals.stream * 10 + locals.i,
			                        locals.zeroReveal); // Don't need to initialize [locals.zeroReveal]
			                                            // because locals struct has been zeroed (bad
			                                            // practice, but it's for spreading awareness
			                                            // about this nuance)
		}

		for (locals.i = 0; locals.i < state.get().populations.get(locals.stream); locals.i++)
		{
			if (state.get().revealOrCommitFlags.get(locals.stream * 1365 + locals.i))
			{
				break;
			}
		}
		if (locals.i == state.get().populations.get(locals.stream)) // Nobody provided their reveal, that
		                                                            // tick was probably empty
		{
			while (locals.i--)
			{
				switch (state.get().collateralTiers.get(locals.stream * 1365 + locals.i))
				{
					case 0: qpi.transfer(state.get().providers.get(locals.stream * 1365 + locals.i), 1); break;

					case 1: qpi.transfer(state.get().providers.get(locals.stream * 1365 + locals.i), 10); break;

					case 2: qpi.transfer(state.get().providers.get(locals.stream * 1365 + locals.i), 100); break;

					case 3: qpi.transfer(state.get().providers.get(locals.stream * 1365 + locals.i), 1000); break;

					case 4: qpi.transfer(state.get().providers.get(locals.stream * 1365 + locals.i), 10000); break;

					case 5: qpi.transfer(state.get().providers.get(locals.stream * 1365 + locals.i), 100000); break;

					case 6: qpi.transfer(state.get().providers.get(locals.stream * 1365 + locals.i), 1000000); break;

					case 7: qpi.transfer(state.get().providers.get(locals.stream * 1365 + locals.i), 10000000); break;

					case 8: qpi.transfer(state.get().providers.get(locals.stream * 1365 + locals.i), 100000000); break;

					default: qpi.transfer(state.get().providers.get(locals.stream * 1365 + locals.i), 1000000000);
				}
				state.mut().providers.set(locals.stream * 1365 + locals.i, id::zero());
				state.mut().collateralTiers.set(locals.stream * 1365 + locals.i, 0);
				// Don't need to zero [state.reveals], they are all-zeros anyway
				state.mut().commits.set(locals.stream * 1365 + locals.i, id::zero());
			}
			state.mut().populations.set(locals.stream, 0);
		}
		else
		{
			// Don't need to initialize [locals.collateralTierFlags] because locals
			// struct has been zeroed (bad practice, but it's for spreading awareness
			// about this nuance)

			for (locals.i = state.get().populations.get(locals.stream); locals.i--;)
			{
				if (state.get().revealOrCommitFlags.get(locals.stream * 1365 + locals.i))
				{
					locals.collateralRecipient = state.get().providers.get(locals.stream * 1365 + locals.i);
				}
				else
				{
					locals.collateralRecipient = id::zero();
				}

				switch (state.get().collateralTiers.get(locals.stream * 1365 + locals.i))
				{
					case 0: qpi.transfer(locals.collateralRecipient, 1); break;

					case 1: qpi.transfer(locals.collateralRecipient, 10); break;

					case 2: qpi.transfer(locals.collateralRecipient, 100); break;

					case 3: qpi.transfer(locals.collateralRecipient, 1000); break;

					case 4: qpi.transfer(locals.collateralRecipient, 10000); break;

					case 5: qpi.transfer(locals.collateralRecipient, 100000); break;

					case 6: qpi.transfer(locals.collateralRecipient, 1000000); break;

					case 7: qpi.transfer(locals.collateralRecipient, 10000000); break;

					case 8: qpi.transfer(locals.collateralRecipient, 100000000); break;

					default: qpi.transfer(locals.collateralRecipient, 1000000000);
				}

				if (locals.collateralRecipient == id::zero())
				{
					locals.collateralTierFlags |= (1 << state.get().collateralTiers.get(locals.stream * 1365 + locals.i));

					state.mut().providers.set(locals.stream * 1365 + locals.i,
					                          state.get().providers.get(locals.stream * 1365 + state.get().populations.get(locals.stream)));
					state.mut().providers.set(locals.stream * 1365 + state.get().populations.get(locals.stream), id::zero());

					state.mut().collateralTiers.set(
					    locals.stream * 1365 + locals.i,
					    state.get().collateralTiers.get(locals.stream * 1365 + state.get().populations.get(locals.stream)));
					state.mut().collateralTiers.set(locals.stream * 1365 + state.get().populations.get(locals.stream), 0);

					state.mut().reveals.set(locals.stream * 1365 + locals.i,
					                        state.get().reveals.get(locals.stream * 1365 + state.get().populations.get(locals.stream)));
					state.mut().reveals.set(locals.stream * 1365 + state.get().populations.get(locals.stream), locals.zeroReveal);

					state.mut().commits.set(locals.stream * 1365 + locals.i,
					                        state.get().commits.get(locals.stream * 1365 + state.get().populations.get(locals.stream)));
					state.mut().commits.set(locals.stream * 1365 + state.get().populations.get(locals.stream), id::zero());

					state.mut().populations.set(locals.stream, state.get().populations.get(locals.stream) - 1);
				}
				else
				{
					if (!(locals.collateralTierFlags & (1 << state.get().collateralTiers.get(locals.stream * 1365 + locals.i))))
					{
						locals.entropy =
						    state.get().entropy.get(locals.stream * 10 + state.get().collateralTiers.get(locals.stream * 1365 + locals.i));
						for (locals.j = 0; locals.j < 4096; locals.j++)
						{
							locals.entropy.set(locals.j,
							                   locals.entropy.get(locals.j) ^ state.get().reveals.get(locals.stream * 1365 + locals.i).get(locals.j));
						}
						state.mut().entropy.set(locals.stream * 10 + state.get().collateralTiers.get(locals.stream * 1365 + locals.i),
						                        locals.entropy);
						state.mut().reveals.set(locals.stream * 1365 + locals.i, locals.zeroReveal);
					}
				}

				state.mut().revealOrCommitFlags.set(locals.stream * 1365 + locals.i, 0);
			}

			for (locals.i = 0; locals.i < 10; locals.i++)
			{
				if (locals.collateralTierFlags & (1 << locals.i))
				{
					state.mut().entropy.set(locals.stream * 10 + locals.i, locals.zeroReveal);
				}
			}
		}
	}

	REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
	{
		REGISTER_USER_PROCEDURE(RevealAndCommit, 1);
	}

	INITIALIZE()
	{
		state.mut().bitFee = 1000;
	}
};
