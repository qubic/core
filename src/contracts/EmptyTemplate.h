using namespace QPI;

struct CNAME2
{
};

struct CNAME : public ContractBase
{
	// All persistent state fields must be declared inside StateData.
	// Access state with state.get().field (read) and state.mut().field (write).
	// state.mut() marks the contract state as dirty for automatic change detection.
	struct StateData
	{
	};

	REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
	{
	}

	INITIALIZE()
	{
	}

	BEGIN_EPOCH()
	{
	}

	END_EPOCH()
	{
	}

	BEGIN_TICK()
	{
	}

	END_TICK()
	{
	}

	PRE_ACQUIRE_SHARES()
	{
	}

	POST_ACQUIRE_SHARES()
	{
	}

	PRE_RELEASE_SHARES()
	{
	}

	POST_RELEASE_SHARES()
	{
	}

	POST_INCOMING_TRANSFER()
	{
	}

	EXPAND()
	{
	}
};
