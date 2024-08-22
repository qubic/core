using namespace QPI;

struct RANDOM2
{
};

struct RANDOM : public ContractBase
{
public:
	struct RevealAndCommit_input
	{
		bit_4096 revealedBits;
		id committedDigest;
	};
	struct RevealAndCommit_output
	{
	};

private:
	uint64 _earnedAmount;
	uint64 _distributedAmount;
	uint64 _burnedAmount;

	uint32 _bitFee; // Amount of qus

	PUBLIC_PROCEDURE(RevealAndCommit)

		qpi.transfer(qpi.invocator(), qpi.invocationReward());
	_

	REGISTER_USER_FUNCTIONS_AND_PROCEDURES

		REGISTER_USER_PROCEDURE(RevealAndCommit, 1);
	_

	INITIALIZE

		state._bitFee = 1000;
	_
};
