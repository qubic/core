using namespace QPI;

struct CNAME2
{
};

struct CNAME : public ContractBase
{
	REGISTER_USER_FUNCTIONS_AND_PROCEDURES
	_

	INITIALIZE
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
