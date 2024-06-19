using namespace QPI;

struct RANDOM2
{
};

struct RANDOM
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

	BEGIN_EPOCH
	_

	END_EPOCH
	_

	BEGIN_TICK
	_

	END_TICK
	_

	PRE_ACQUIRE_SHARES
	_

	POST_ACQUIRE_SHARES
	_

	PRE_RELEASE_SHARES
	_

	POST_RELEASE_SHARES
	_

	EXPAND
	_
};