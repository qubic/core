using namespace QPI;

#define AIRDROP_START_SUCCESS 0
#define AIRDROP_INSUFFICIENT_FUND 1
#define AIRDROP_STARTED 2
#define AIRDROP_SEND_FUND 3
#define AIRDROP_START_ERROR 4
#define AIRDROP_INCORRECT_TIME 5
#define AIRDROP_DISTRIBUTE_SUCCESS 6
#define AIRDROP_DISTRIBUTE_FAILD 7
#define AIRDROP_PERIOD_LIMITED 8
#define AIRDROP_FUND_LIMITED 9
#define AIRDROP_INSUFFICIENT_TOKEN 10

struct AirdropLogger
{
    id src;
    id dst;
    sint64 amt;
    uint32 logtype;
    char _terminator;
};

struct AIRDROP2
{
};

struct AIRDROP
{
private:
    struct _AssetOrder
	{
		id entity;
		sint64 numberOfShares;
	};
	collection<_AssetOrder, 2097152 * X_MULTIPLIER> _assetOrders;

	struct _EntityOrder
	{
		id issuer;
		uint64 assetName;
		sint64 numberOfShares;
	};
	collection<_EntityOrder, 2097152 * X_MULTIPLIER> _entityOrders;

    id contractOwner;             // address of owner
    sint64 TotalAmountForAirdrop; // amount of Airdrop
    uint64 start_time;            // start time of Airdrop
    uint64 end_time;              // end time of Airdrop
    uint64 _burnedAmount;
    AirdropLogger logger;
    uint32 _airdropStartFee;// Amount of qus
	uint32 _transferFee; // Amount of qus
    sint64 _elementIndex, _elementIndex2;
	id _issuerAndAssetName;
	_AssetOrder _assetOrder;
	_EntityOrder _entityOrder;

public:
    struct Fees_input
	{
	};
	struct Fees_output
	{
        uint32 airdropStartFee;  // Amount of qus
		uint32 assetIssuanceFee; // Amount of qus
		uint32 transferFee; // Amount of qus
	};

    // Variables to store input parameters when calling the StartAirdrop() function`
    struct StartAirdrop_input
    {
        id _contractOwner;  // address of owner
        uint64 _start_time; // start time of Airdrop
        uint64 _end_time;   // end time of Airdrop
        uint64 assetName;
		sint64 numberOfShares;
		uint64 unitOfMeasurement;
		sint8 numberOfDecimalPlaces;
    };

    struct StartAirdrop_output
    {
        sint32 returnCode;
    };

    struct DistributeToken_input
    {
        uint64 _current_time;
        id issuer;
		id newOwnerAndPossessor;
		uint64 assetName;
		sint64 numberOfShares;
    };

    struct DistributeToken_output
    {
        sint64 transferredNumberOfShares;
    };
    // getting fees
    PUBLIC(Fees)

		output.airdropStartFee = state._airdropStartFee;
		output.transferFee = state._transferFee;
	_

    // Start Airdrop(Setting detailed Airdrop)
    PUBLIC(StartAirdrop)
    state.logger = AirdropLogger{qpi.invocator(), SELF, qpi.invocationReward(), AIRDROP_STARTED};
    LOG_INFO(state.logger);
    if (qpi.invocationReward() < state._airdropStartFee)
    {
        if (qpi.invocationReward() > 0)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
        }
        state.logger = AirdropLogger{qpi.invocator(), SELF, qpi.invocationReward(), AIRDROP_INSUFFICIENT_FUND};
        output.returnCode = AIRDROP_START_ERROR;
        LOG_INFO(state.logger);
        return;
    }
    if (input._start_time > input._end_time || input._end_time < 0)
    {
        state.logger = AirdropLogger{qpi.invocator(), SELF, qpi.invocationReward(), AIRDROP_INCORRECT_TIME};
        qpi.transfer(qpi.invocator(), qpi.invocationReward());
        output.returnCode = AIRDROP_START_ERROR;
        LOG_INFO(state.logger);
        return;
    }
    state.contractOwner = input._contractOwner;
    state.start_time = input._start_time;
    state.end_time = input._end_time;
    state.TotalAmountForAirdrop = qpi.issueAsset(input.assetName, qpi.invocator(), input.numberOfDecimalPlaces, input.numberOfShares, input.unitOfMeasurement);

    if(qpi.invocationReward() > state._airdropStartFee)
    {
        qpi.transfer(qpi.invocator(), qpi.invocationReward() - state._airdropStartFee);
    }
    state.logger = AirdropLogger{qpi.invocator(), SELF, qpi.invocationReward(), AIRDROP_START_SUCCESS};
    LOG_INFO(state.logger);
    output.returnCode = AIRDROP_START_SUCCESS;
    qpi.burn(state._airdropStartFee);
    state._burnedAmount += state._airdropStartFee; 
    _

    // Procedure to be call When there is a user that meets the conditions
    PUBLIC(DistributeToken)
    {
        if (qpi.invocationReward() < state._transferFee)
        {
            state.logger = AirdropLogger{qpi.invocator(), SELF, qpi.invocationReward(), AIRDROP_INSUFFICIENT_FUND};
            LOG_INFO(state.logger);
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.transferredNumberOfShares = 0;
            return;
        }
        if (input._current_time > state.end_time)
        {
            state.logger = AirdropLogger{qpi.invocator(), SELF, qpi.invocationReward(), AIRDROP_PERIOD_LIMITED};
            LOG_INFO(state.logger);
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.transferredNumberOfShares = 0;
            return;
        }
        if (qpi.numberOfPossessedShares(input.assetName, input.issuer, qpi.invocator(), qpi.invocator(), SELF_INDEX, SELF_INDEX) < input.numberOfShares)
        {
            output.transferredNumberOfShares = 0;
            state.logger = AirdropLogger{qpi.invocator(), SELF, input.numberOfShares, AIRDROP_INSUFFICIENT_TOKEN};
            LOG_INFO(state.logger);
        }
        else
        {
            output.transferredNumberOfShares = qpi.transferShareOwnershipAndPossession(input.assetName, input.issuer, qpi.invocator(), qpi.invocator(), input.numberOfShares, input.newOwnerAndPossessor) < 0 ? 0 : input.numberOfShares;
            if(output.transferredNumberOfShares == 0) 
            {
                state.logger = AirdropLogger{qpi.invocator(), SELF, input.numberOfShares, AIRDROP_DISTRIBUTE_FAILD};
                LOG_INFO(state.logger);
            }
            else 
            {
                if (qpi.invocationReward() > state._transferFee)
                {
                    qpi.transfer(qpi.invocator(), qpi.invocationReward() - state._transferFee);
                }
                qpi.burn(state._transferFee);
                state._burnedAmount += state._transferFee; 
                state.logger = AirdropLogger{qpi.invocator(), input.newOwnerAndPossessor, input.numberOfShares, AIRDROP_DISTRIBUTE_SUCCESS};
                LOG_INFO(state.logger);
            }
        }
    }

    _

    REGISTER_USER_FUNCTIONS_AND_PROCEDURES
        REGISTER_USER_FUNCTION(Fees, 1);

        REGISTER_USER_PROCEDURE(StartAirdrop, 1);
        REGISTER_USER_PROCEDURE(DistributeToken, 2);
    _

    INITIALIZE
        state._transferFee = 1000;   // fee to discribute to user
        state._airdropStartFee = 100000000; // fee to start Airdrop
    _

    BEGIN_EPOCH
    _

    END_EPOCH
    _

    BEGIN_TICK
    _

    END_TICK
    _

    EXPAND _
};
