using namespace QPI;

struct RANDOM2
{
};

struct RANDOM : public ContractBase
{
public:
	struct StateData
	{
		uint64 _earnedAmount;
		uint64 _distributedAmount;
		uint64 _burnedAmount;

		uint32 _bitFee; // Amount of qus
	};

	struct RevealAndCommit_input
	{
		bit_4096 revealedBits;
		id committedDigest;
	};
	struct RevealAndCommit_output
	{
	};

	PUBLIC_PROCEDURE(RevealAndCommit)
	{
		qpi.transfer(qpi.invocator(), qpi.invocationReward());
	}

	REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
	{
		REGISTER_USER_PROCEDURE(RevealAndCommit, 1);
	}

	INITIALIZE()
	{
		state.mut()._bitFee = 1000;
	}
};
