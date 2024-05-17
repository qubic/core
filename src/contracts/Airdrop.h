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
#define AIRDROP_START_FEE 1000000000LL 
#define AIRDROP_TRANSER_FEE 1000000LL // Amount of qus

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
    AirdropLogger logger;
    uint64 assetName;
    uint64 _earnedAmount;
    ::Entity entitys;
    id currentID;

public:
    struct Fees_input
	{
	};
	struct Fees_output
	{
        uint32 airdropStartFee;  // Amount of qus
		uint32 transferFee; // Amount of qus
	};

    // Variables to store input parameters when calling the StartAirdrop() function`
    struct StartAirdrop_input
    {
        uint64 assetName;
		sint64 numberOfShares;
		sint8 numberOfDecimalPlaces;
        uint64 unitOfMeasurement;
    };

    struct StartAirdrop_output
    {
		sint64 issuedNumberOfShares;
    };

    struct DistributeToken_input
    {
        id issuer;
		uint64 assetName;
    };

    struct DistributeToken_output
    {
        sint64 transferredAmount;
    };

    struct TransferToken_input {
        id issuer;
		uint64 assetName;
        uint64 amount;
        id receiver;
    };

    struct TransferToken_output {
        sint64 transferredAmount;
    };
    // getting fees
    PUBLIC(Fees)
		output.airdropStartFee = AIRDROP_START_FEE;
		output.transferFee = AIRDROP_TRANSER_FEE;
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

			output.issuedNumberOfShares = 0;
		}
		else
		{
			if (qpi.invocationReward() > AIRDROP_START_FEE)
			{
				qpi.transfer(qpi.invocator(), qpi.invocationReward() - AIRDROP_START_FEE);
			}
			state._earnedAmount += AIRDROP_START_FEE;

			output.issuedNumberOfShares = qpi.issueAsset(input.assetName, qpi.invocator(), input.numberOfDecimalPlaces, input.numberOfShares, input.unitOfMeasurement);
		}
    _

    // Procedure to be call When there is a user that meets the conditions
    PUBLIC(DistributeToken)
        if (qpi.invocationReward() < AIRDROP_TRANSER_FEE)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }

            output.transferredAmount = 0;
            return ;
        }
        if (qpi.invocationReward() > AIRDROP_TRANSER_FEE)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - AIRDROP_TRANSER_FEE);
        }
        output.transferredAmount = 0;
        state.currentID = qpi.nextId(qpi.invocator());
        while(state.currentID != qpi.invocator()) {
            if(state.currentID != NULL_ID) {
                qpi.getEntity(state.currentID, state.entitys);
                if(state.entitys.incomingAmount - state.entitys.outgoingAmount > 0) {
                    qpi.transferShareOwnershipAndPossession(input.assetName, input.issuer, qpi.invocator(), qpi.invocator(), 1, state.entitys.publicKey);
                    output.transferredAmount++;
                }
            }
            state.currentID = qpi.nextId(state.currentID);
        }
    _

    // Procedure for transferring the token
    PUBLIC(TransferToken)
        if (qpi.invocationReward() < AIRDROP_TRANSER_FEE)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }

            output.transferredAmount = 0;
            return ;
        }
        if (qpi.invocationReward() > AIRDROP_TRANSER_FEE)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - AIRDROP_TRANSER_FEE);
        }
        output.transferredAmount = qpi.transferShareOwnershipAndPossession(input.assetName, input.issuer, qpi.invocator(), qpi.invocator(), input.amount, input.receiver);
    _

    REGISTER_USER_FUNCTIONS_AND_PROCEDURES
        REGISTER_USER_FUNCTION(Fees, 1);

        REGISTER_USER_PROCEDURE(StartAirdrop, 1);
        REGISTER_USER_PROCEDURE(DistributeToken, 2);
        REGISTER_USER_PROCEDURE(TransferToken, 3);
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