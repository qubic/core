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

struct QAirdropLogger
{
    id src;
    id dst;
    sint64 amt;
    uint32 logtype;
};

struct QAIRDROP2
{
};

struct QAIRDROP
{
private:
    id contractOwner; // address of owner
    uint64 TotalAmountForAirdrop; // amount of Airdrop
    uint64 start_time; // start time of Airdrop
    uint64 end_time; // end time of Airdrop
    QAirdropLogger logger;
    uint32 _transferFee;
    uint32 _airdropStartFee;

public:
    //Variables to store input parameters when calling the StartAirdrop() function`
    struct StartAirdrop_input
    {
        id _contractOwner; // address of owner
        uint64 _start_time; // start time of Airdrop
        uint64 _end_time; // end time of Airdrop
    };

    struct StartAirdrop_output
    {
        sint32 returnCode;
    };

    struct DistributeToken_input
    {
        id _user;
        uint64 _current_time;
        sint64 _amount;
    };

    struct DistributeToken_output
    {
        sint32 returnCode;
    };

    //Variable to store input parameters when calling the OnlyOwner() function
    //contractOwner
    struct OnlyOwner_input
    {
        id _contractOwner;
    };

    //Variable to store output parameter for the OnlyOwner() function
    //Variable that determines whether the caller is the contract owner
    struct OnlyOwner_output
    {
        bit msg;
    };

    // Function to ensure only the owner can call certain functions
    PUBLIC_FUNCTION(OnlyOwner)
        if(state.contractOwner != input._contractOwner) output.msg = false;
        else output.msg = true;
    _

    // Start Airdrop(Setting detailed Airdrop)
    PUBLIC_PROCEDURE(StartAirdrop)
        state.logger = QAirdropLogger{qpi.invocator(), SELF, qpi.invocationReward(), AIRDROP_STARTED };
        LOG_INFO(state.logger);
        if(qpi.invocationReward() < state._airdropStartFee) 
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            state.logger = QAirdropLogger{qpi.invocator(), SELF, qpi.invocationReward(), AIRDROP_INSUFFICIENT_FUND };
            output.returnCode = AIRDROP_START_ERROR;
            LOG_INFO(state.logger);
            return ;
        }
        if(input._start_time > input._end_time || input._end_time < 0)
        {
            state.logger = QAirdropLogger{qpi.invocator(), SELF, qpi.invocationReward(), AIRDROP_INCORRECT_TIME };
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.returnCode = AIRDROP_START_ERROR;
            LOG_INFO(state.logger);
            return ;
        }
        state.contractOwner = input._contractOwner;
        state.TotalAmountForAirdrop = qpi.invocationReward() - state._airdropStartFee;
        state.start_time = input._start_time;
        state.end_time = input._end_time;

        qpi.transfer(qpi.invocator(), qpi.invocationReward() - state._airdropStartFee);
        state.logger = QAirdropLogger{qpi.invocator(), SELF, qpi.invocationReward(), AIRDROP_START_SUCCESS};
        LOG_INFO(state.logger);
        output.returnCode = AIRDROP_START_SUCCESS;
        qpi.burn(state._airdropStartFee);
    _
    
    // Procedure to be call When there is a user that meets the conditions
    PUBLIC_PROCEDURE(DistributeToken)
    {
        if(qpi.invocationReward() < state._transferFee)
        {
            state.logger = QAirdropLogger{qpi.invocator(), SELF, qpi.invocationReward(), AIRDROP_INSUFFICIENT_FUND };
            LOG_INFO(state.logger);
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = AIRDROP_DISTRIBUTE_FAILD;
            return ;
        }
        if(input._current_time > state.end_time)
        {
            state.logger = QAirdropLogger{qpi.invocator(), SELF, qpi.invocationReward(), AIRDROP_PERIOD_LIMITED };
            LOG_INFO(state.logger);
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = AIRDROP_PERIOD_LIMITED;
            return ;
        }
        if(state.TotalAmountForAirdrop < qpi.invocationReward()) 
        {
            state.logger = QAirdropLogger{qpi.invocator(), SELF, qpi.invocationReward(), AIRDROP_FUND_LIMITED };
            LOG_INFO(state.logger);
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            output.returnCode = AIRDROP_PERIOD_LIMITED;
            return ;
        }
        state.TotalAmountForAirdrop -= qpi.invocationReward();
        qpi.transfer(input._user, input._amount);
        qpi.burn(state._transferFee);
        state.logger = QAirdropLogger{qpi.invocator(), input._user, input._amount, AIRDROP_DISTRIBUTE_SUCCESS };
        LOG_INFO(state.logger);
        output.returnCode = AIRDROP_DISTRIBUTE_SUCCESS;
    }

    _

    REGISTER_USER_FUNCTIONS_AND_PROCEDURES
        REGISTER_USER_PROCEDURE(StartAirdrop, 1);

        REGISTER_USER_PROCEDURE(DistributeToken, 2);
	_

	INITIALIZE
        state._transferFee = 100;  // fee to discribute to user
        state._airdropStartFee = 100000000;  // fee to start Airdrop
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
