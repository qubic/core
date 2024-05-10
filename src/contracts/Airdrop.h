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
#define AIRDROP_START_FEE 100000000LL 
#define AIRDROP_TRANSER_FEE 1000LL // Amount of qus

struct AirdropLogger
{
    uint32 contractId; // to distinguish bw SCs (set when LOG_*() is called
    uint32 padding;
    id src;
    id dst;
    sint64 amt;
    uint32 logtype;
    char _terminator; // Only data before "_terminator" are logged
};

struct AIRDROP2
{
};

struct AIRDROP
{
private:
    id contractOwner;             // address of owner
    sint64 TotalAmountForAirdrop; // amount of Airdrop
    uint64 start_time;            // start time of Airdrop
    uint64 end_time;              // end time of Airdrop
    uint64 _burnedAmount;
    AirdropLogger logger;
    uint64 assetName;

public:
    struct Fees_input
	{
	};
	struct Fees_output
	{
        uint32 airdropStartFee;  // Amount of qus
		uint32 transferFee; // Amount of qus
	};

    struct Getbalance_input 
    {
        id address;
    };

    struct Getbalance_output
    {
        uint64 amount;
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
        id* issuers;
		id newOwnerAndPossessor;
		uint64 assetName;
		sint64 amount;
    };

    struct DistributeToken_output
    {
        sint64 transferredAmount;
    };
    // getting fees
    PUBLIC(Fees)
		output.airdropStartFee = AIRDROP_START_FEE;
		output.transferFee = AIRDROP_TRANSER_FEE;
	_

    PUBLIC(Getbalance)
        output.amount = qpi.numberOfPossessedShares(state.assetName, state.contractOwner, qpi.invocator(), qpi.invocator(), SELF_INDEX, SELF_INDEX);
    _

    // Start Airdrop(Setting detailed Airdrop)
    PUBLIC(StartAirdrop)
        state.logger = AirdropLogger{0, 0, qpi.invocator(), SELF, qpi.invocationReward(), AIRDROP_STARTED};
        LOG_INFO(state.logger);
        if (qpi.invocationReward() < AIRDROP_START_FEE)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            state.logger = AirdropLogger{0, 0, qpi.invocator(), SELF, qpi.invocationReward(), AIRDROP_INSUFFICIENT_FUND};
            output.returnCode = AIRDROP_START_ERROR;
            LOG_INFO(state.logger);
            return;
        }
        if (input._start_time > input._end_time || input._end_time < 0)
        {
            state.logger = AirdropLogger{0, 0, qpi.invocator(), SELF, qpi.invocationReward(), AIRDROP_INCORRECT_TIME};
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.returnCode = AIRDROP_START_ERROR;
            LOG_INFO(state.logger);
            return;
        }
        state.assetName = input.assetName;
        state.contractOwner = input._contractOwner;
        state.start_time = input._start_time;
        state.end_time = input._end_time;
        state.TotalAmountForAirdrop = qpi.issueAsset(input.assetName, qpi.invocator(), input.numberOfDecimalPlaces, input.numberOfShares, input.unitOfMeasurement);

        if(qpi.invocationReward() > AIRDROP_START_FEE)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - AIRDROP_START_FEE);
        }
        state.logger = AirdropLogger{0, 0, qpi.invocator(), SELF, qpi.invocationReward(), AIRDROP_START_SUCCESS};
        LOG_INFO(state.logger);
        output.returnCode = AIRDROP_START_SUCCESS;
        qpi.burn(AIRDROP_START_FEE);
        state._burnedAmount += AIRDROP_START_FEE; 
    _

    // Procedure to be call When there is a user that meets the conditions
    PUBLIC(DistributeToken)
        uint64 total = input.amount * sizeof(input.issuers) / sizeof(int);
        if (qpi.invocationReward() < AIRDROP_TRANSER_FEE)
        {
            state.logger = AirdropLogger{0, 0, qpi.invocator(), SELF, qpi.invocationReward(), AIRDROP_INSUFFICIENT_FUND};
            LOG_INFO(state.logger);
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.transferredAmount = 0;
            return;
        }
        if (input._current_time > state.end_time)
        {
            state.logger = AirdropLogger{0, 0, qpi.invocator(), SELF, qpi.invocationReward(), AIRDROP_PERIOD_LIMITED};
            LOG_INFO(state.logger);
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.transferredAmount = 0;
            return;
        }
        if (qpi.numberOfPossessedShares(input.assetName, qpi.invocator(), qpi.invocator(), qpi.invocator(), SELF_INDEX, SELF_INDEX) < total)
        {
            output.transferredAmount = 0;
            state.logger = AirdropLogger{0, 0, qpi.invocator(), SELF, input.amount, AIRDROP_INSUFFICIENT_TOKEN};
            LOG_INFO(state.logger);
        }
        else
        {
            for(int i = 0 ; i < sizeof(input.issuers) / sizeof(int); i++) 
            {
                qpi.transferShareOwnershipAndPossession(input.assetName, input.issuers[i], qpi.invocator(), qpi.invocator(), input.amount, input.newOwnerAndPossessor);
            }
            output.transferredAmount = total;
            if(output.transferredAmount == 0) 
            {
                state.logger = AirdropLogger{0, 0, qpi.invocator(), SELF, input.amount, AIRDROP_DISTRIBUTE_FAILD};
                LOG_INFO(state.logger);
            }
            else 
            {
                if (qpi.invocationReward() > AIRDROP_TRANSER_FEE)
                {
                    qpi.transfer(qpi.invocator(), qpi.invocationReward() - AIRDROP_TRANSER_FEE);
                }
                qpi.burn(AIRDROP_TRANSER_FEE);
                state._burnedAmount += AIRDROP_TRANSER_FEE; 
                state.logger = AirdropLogger{0, 0, qpi.invocator(), input.newOwnerAndPossessor, input.amount, AIRDROP_DISTRIBUTE_SUCCESS};
                LOG_INFO(state.logger);
            }
        }
    _

    REGISTER_USER_FUNCTIONS_AND_PROCEDURES
        REGISTER_USER_FUNCTION(Fees, 1);
        REGISTER_USER_FUNCTION(Getbalance, 2);

        REGISTER_USER_PROCEDURE(StartAirdrop, 1);
        REGISTER_USER_PROCEDURE(DistributeToken, 2);
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

    EXPAND 
    _
};