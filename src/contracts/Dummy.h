using namespace QPI;

struct DUMMY2
{
};

// --- Dummy contract for EP218 ---

//struct DUMMY : public ContractBase
//{
//	// All persistent state fields must be declared inside StateData.
//	// Access state with state.get().field (read) and state.mut().field (write).
//	// state.mut() marks the contract state as dirty for automatic change detection.
//	struct StateData
//	{
//		uint32 numBeginEpoch;
//		uint32 numEndEpoch;
//		SlowAnySizeArray<uint32, 10000> dummyArray; // to make state size larger than sizeof(IPO)
//	};
//
//	struct INITIALIZE_locals
//	{
//		uint32 i;
//	};
//
//	INITIALIZE_WITH_LOCALS()
//	{
//		state.mut().numBeginEpoch = 0;
//        state.mut().numEndEpoch = 0;
//
//		for (locals.i = 0; locals.i < 10000; locals.i++)
//		{
//			state.mut().dummyArray.set(locals.i, locals.i);
//		}
//	}
//
//	BEGIN_EPOCH()
//	{
//        state.mut().numBeginEpoch++;
//	}
//
//	END_EPOCH()
//	{
//        state.mut().numEndEpoch++;
//	}
//
//	REGISTER_USER_FUNCTIONS_AND_PROCEDURES() {}
//
//};


// --- Dummy contract for EP219 ---

struct DUMMY : public ContractBase
{
	struct OldStateData
	{
		uint32 numBeginEpoch;
		uint32 numEndEpoch;
		SlowAnySizeArray<uint32, 10000> dummyArray; // to make state size larger than sizeof(IPO)
	};

	// All persistent state fields must be declared inside StateData.
	// Access state with state.get().field (read) and state.mut().field (write).
	// state.mut() marks the contract state as dirty for automatic change detection.
	struct StateData
	{
		uint32 numBeginEpoch;
        uint32 numEndEpoch;

		struct DummyInfo
		{
			uint32 a;
			sint32 b;
		};

		SlowAnySizeArray<DummyInfo, 10000> dummyArray;
	};

	struct MIGRATE_locals
	{
		sint32 i;
	};

	MIGRATE_WITH_LOCALS()
	{
		state.mut().numBeginEpoch = oldState.numBeginEpoch;
		state.mut().numEndEpoch = oldState.numEndEpoch;

		for (locals.i = 0; locals.i < 10000; locals.i++)
		{
			state.mut().dummyArray.set(locals.i, StateData::DummyInfo{ .a = oldState.dummyArray.get(locals.i), .b = -locals.i });
		}
	}

	INITIALIZE()
	{
		state.mut().numBeginEpoch = 0;
		state.mut().numEndEpoch = 0;
	}

	BEGIN_EPOCH()
	{
		state.mut().numBeginEpoch++;
	}

	END_EPOCH()
	{
		state.mut().numEndEpoch++;
	}

	REGISTER_USER_FUNCTIONS_AND_PROCEDURES() {}

};
