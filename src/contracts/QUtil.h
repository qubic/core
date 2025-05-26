using namespace QPI;
/**************************************/
/**************SC UTILS****************/
/**************************************/
/*
* A collection of useful functions for smart contract on Qubic:
* - SendToManyV1 (STM1): Sending qu from a single address to multiple addresses (upto 25)
* - SendToManyBenchmark: Sending n transfers of 1 qu each to the specified number of addresses 
* ...
* ...
*/

// Return code for logger and return struct
constexpr uint64 STM1_SUCCESS = 0;
constexpr uint64 STM1_INVALID_AMOUNT_NUMBER = 1;
constexpr uint64 STM1_WRONG_FUND = 2;
constexpr uint64 STM1_TRIGGERED = 3;
constexpr uint64 STM1_SEND_FUND = 4;
constexpr sint64 STM1_INVOCATION_FEE = 10LL; // fee to be burned and make the SC running

struct QUtilLogger
{
    uint32 contractId; // to distinguish bw SCs
    uint32 padding;
    id src;
    id dst;
    sint64 amt;
    uint32 logtype;
    // Other data go here
    char _terminator; // Only data before "_terminator" are logged
};

struct QUTIL2
{
};

struct QUTIL : public ContractBase
{
private:
    // registers, logger and other buffer
    sint32 _i0, _i1, _i2, _i3;
    sint64 _i64_0, _i64_1, _i64_2, _i64_3;
    uint64 _r0, _r1, _r2, _r3;
    sint64 total;
    QUtilLogger logger;
public:
    /**************************************/
    /********INPUT AND OUTPUT STRUCTS******/
    /**************************************/
    struct SendToManyV1_input
    {
        id dst0, dst1, dst2, dst3, dst4, dst5, dst6, dst7, dst8, dst9, dst10, dst11, dst12,
            dst13, dst14, dst15, dst16, dst17, dst18, dst19, dst20, dst21, dst22, dst23, dst24;
        sint64 amt0, amt1, amt2, amt3, amt4, amt5, amt6, amt7, amt8, amt9, amt10, amt11, amt12,
            amt13, amt14, amt15, amt16, amt17, amt18, amt19, amt20, amt21, amt22, amt23, amt24;
    };
    struct SendToManyV1_output
    {
        sint32 returnCode;
    };

    struct GetSendToManyV1Fee_input
    {
    };
    struct GetSendToManyV1Fee_output
    {
        sint64 fee;
    };

    struct SendToManyBenchmark_input
    {
        sint64 dstCount;
        sint64 numTransfersEach;
    };
    struct SendToManyBenchmark_output
    {
        sint64 dstCount;
        sint32 returnCode;
        sint64 total;
    };

    struct SendToManyBenchmark_locals
    {
        id currentId;
        sint64 t;
        bool useNext;
    };

    struct BurnQubic_input
    {
        sint64 amount;
    };
    struct BurnQubic_output
    {
        sint64 amount;
    };

    typedef Asset GetTotalNumberOfAssetShares_input;
    typedef sint64 GetTotalNumberOfAssetShares_output;

    /**************************************/
    /************CORE FUNCTIONS************/
    /**************************************/
    /*
    * @return return SendToManyV1 fee per invocation
    */
    PUBLIC_FUNCTION(GetSendToManyV1Fee)
    {
        output.fee = STM1_INVOCATION_FEE;
    }

    /**
    * Send qu from a single address to multiple addresses
    * @param list of 25 destination addresses (800 bytes): 32 bytes for each address, leave empty(zeroes) for unused memory space
    * @param list of 25 amounts (200 bytes): 8 bytes(long long) for each amount, leave empty(zeroes) for unused memory space
    * @return returnCode (0 means success)
    */
    PUBLIC_PROCEDURE(SendToManyV1)
    {
        state.logger = QUtilLogger{ 0,  0, qpi.invocator(), SELF, qpi.invocationReward(), STM1_TRIGGERED };
        LOG_INFO(state.logger);
        state.total = input.amt0 + input.amt1 + input.amt2 + input.amt3 + input.amt4 + input.amt5 + input.amt6 + input.amt7 + input.amt8 + input.amt9 + input.amt10 + input.amt11 + input.amt12 + input.amt13 + input.amt14 + input.amt15 + input.amt16 + input.amt17 + input.amt18 + input.amt19 + input.amt20 + input.amt21 + input.amt22 + input.amt23 + input.amt24 + STM1_INVOCATION_FEE;
        // invalid amount (<0), return fund and exit
        if ((input.amt0 < 0) || (input.amt1 < 0) || (input.amt2 < 0) || (input.amt3 < 0) || (input.amt4 < 0) || (input.amt5 < 0) || (input.amt6 < 0) || (input.amt7 < 0) || (input.amt8 < 0) || (input.amt9 < 0) || (input.amt10 < 0) || (input.amt11 < 0) || (input.amt12 < 0) || (input.amt13 < 0) || (input.amt14 < 0) || (input.amt15 < 0) || (input.amt16 < 0) || (input.amt17 < 0) || (input.amt18 < 0) || (input.amt19 < 0) || (input.amt20 < 0) || (input.amt21 < 0) || (input.amt22 < 0) || (input.amt23 < 0) || (input.amt24 < 0))
        {
            state.logger = QUtilLogger{ 0,  0, qpi.invocator(), SELF, qpi.invocationReward(), STM1_INVALID_AMOUNT_NUMBER };
            output.returnCode = STM1_INVALID_AMOUNT_NUMBER;
            LOG_INFO(state.logger);
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
        }
        // insufficient or too many qubic transferred, return fund and exit (we don't want to return change)
        if (qpi.invocationReward() != state.total)
        {
            state.logger = QUtilLogger{ 0,  0, qpi.invocator(), SELF, qpi.invocationReward(), STM1_WRONG_FUND };
            LOG_INFO(state.logger);
            output.returnCode = STM1_WRONG_FUND;
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return;
        }

        if (input.dst0 != NULL_ID)
        {
            state.logger = QUtilLogger{ 0,  0, qpi.invocator(), input.dst0, input.amt0, STM1_SEND_FUND };
            LOG_INFO(state.logger);
            qpi.transfer(input.dst0, input.amt0);
        }
        if (input.dst1 != NULL_ID)
        {
            state.logger = QUtilLogger{ 0,  0, qpi.invocator(), input.dst1, input.amt1, STM1_SEND_FUND };
            LOG_INFO(state.logger);
            qpi.transfer(input.dst1, input.amt1);
        }
        if (input.dst2 != NULL_ID)
        {
            state.logger = QUtilLogger{ 0,  0, qpi.invocator(), input.dst2, input.amt2, STM1_SEND_FUND };
            LOG_INFO(state.logger);
            qpi.transfer(input.dst2, input.amt2);
        }
        if (input.dst3 != NULL_ID)
        {
            state.logger = QUtilLogger{ 0,  0, qpi.invocator(), input.dst3, input.amt3, STM1_SEND_FUND };
            LOG_INFO(state.logger);
            qpi.transfer(input.dst3, input.amt3);
        }
        if (input.dst4 != NULL_ID)
        {
            state.logger = QUtilLogger{ 0,  0, qpi.invocator(), input.dst4, input.amt4, STM1_SEND_FUND };
            LOG_INFO(state.logger);
            qpi.transfer(input.dst4, input.amt4);
        }
        if (input.dst5 != NULL_ID)
        {
            state.logger = QUtilLogger{ 0,  0, qpi.invocator(), input.dst5, input.amt5, STM1_SEND_FUND };
            LOG_INFO(state.logger);
            qpi.transfer(input.dst5, input.amt5);
        }
        if (input.dst6 != NULL_ID)
        {
            state.logger = QUtilLogger{ 0,  0, qpi.invocator(), input.dst6, input.amt6, STM1_SEND_FUND };
            LOG_INFO(state.logger);
            qpi.transfer(input.dst6, input.amt6);
        }
        if (input.dst7 != NULL_ID)
        {
            state.logger = QUtilLogger{ 0,  0, qpi.invocator(), input.dst7, input.amt7, STM1_SEND_FUND };
            LOG_INFO(state.logger);
            qpi.transfer(input.dst7, input.amt7);
        }
        if (input.dst8 != NULL_ID)
        {
            state.logger = QUtilLogger{ 0,  0, qpi.invocator(), input.dst8, input.amt8, STM1_SEND_FUND };
            LOG_INFO(state.logger);
            qpi.transfer(input.dst8, input.amt8);
        }
        if (input.dst9 != NULL_ID)
        {
            state.logger = QUtilLogger{ 0,  0, qpi.invocator(), input.dst9, input.amt9, STM1_SEND_FUND };
            LOG_INFO(state.logger);
            qpi.transfer(input.dst9, input.amt9);
        }
        if (input.dst10 != NULL_ID)
        {
            state.logger = QUtilLogger{ 0,  0, qpi.invocator(), input.dst10, input.amt10, STM1_SEND_FUND };
            LOG_INFO(state.logger);
            qpi.transfer(input.dst10, input.amt10);
        }
        if (input.dst11 != NULL_ID)
        {
            state.logger = QUtilLogger{ 0,  0, qpi.invocator(), input.dst11, input.amt11, STM1_SEND_FUND };
            LOG_INFO(state.logger);
            qpi.transfer(input.dst11, input.amt11);
        }
        if (input.dst12 != NULL_ID)
        {
            state.logger = QUtilLogger{ 0,  0, qpi.invocator(), input.dst12, input.amt12, STM1_SEND_FUND };
            LOG_INFO(state.logger);
            qpi.transfer(input.dst12, input.amt12);
        }
        if (input.dst13 != NULL_ID)
        {
            state.logger = QUtilLogger{ 0,  0, qpi.invocator(), input.dst13, input.amt13, STM1_SEND_FUND };
            LOG_INFO(state.logger);
            qpi.transfer(input.dst13, input.amt13);
        }
        if (input.dst14 != NULL_ID)
        {
            state.logger = QUtilLogger{ 0,  0, qpi.invocator(), input.dst14, input.amt14, STM1_SEND_FUND };
            LOG_INFO(state.logger);
            qpi.transfer(input.dst14, input.amt14);
        }
        if (input.dst15 != NULL_ID)
        {
            state.logger = QUtilLogger{ 0,  0, qpi.invocator(), input.dst15, input.amt15, STM1_SEND_FUND };
            LOG_INFO(state.logger);
            qpi.transfer(input.dst15, input.amt15);
        }
        if (input.dst16 != NULL_ID)
        {
            state.logger = QUtilLogger{ 0,  0, qpi.invocator(), input.dst16, input.amt16, STM1_SEND_FUND };
            LOG_INFO(state.logger);
            qpi.transfer(input.dst16, input.amt16);
        }
        if (input.dst17 != NULL_ID)
        {
            state.logger = QUtilLogger{ 0,  0, qpi.invocator(), input.dst17, input.amt17, STM1_SEND_FUND };
            LOG_INFO(state.logger);
            qpi.transfer(input.dst17, input.amt17);
        }
        if (input.dst18 != NULL_ID)
        {
            state.logger = QUtilLogger{ 0,  0, qpi.invocator(), input.dst18, input.amt18, STM1_SEND_FUND };
            LOG_INFO(state.logger);
            qpi.transfer(input.dst18, input.amt18);
        }
        if (input.dst19 != NULL_ID)
        {
            state.logger = QUtilLogger{ 0,  0, qpi.invocator(), input.dst19, input.amt19, STM1_SEND_FUND };
            LOG_INFO(state.logger);
            qpi.transfer(input.dst19, input.amt19);
        }
        if (input.dst20 != NULL_ID)
        {
            state.logger = QUtilLogger{ 0,  0, qpi.invocator(), input.dst20, input.amt20, STM1_SEND_FUND };
            LOG_INFO(state.logger);
            qpi.transfer(input.dst20, input.amt20);
        }
        if (input.dst21 != NULL_ID)
        {
            state.logger = QUtilLogger{ 0,  0, qpi.invocator(), input.dst21, input.amt21, STM1_SEND_FUND };
            LOG_INFO(state.logger);
            qpi.transfer(input.dst21, input.amt21);
        }
        if (input.dst22 != NULL_ID)
        {
            state.logger = QUtilLogger{ 0,  0, qpi.invocator(), input.dst22, input.amt22, STM1_SEND_FUND };
            LOG_INFO(state.logger);
            qpi.transfer(input.dst22, input.amt22);
        }
        if (input.dst23 != NULL_ID)
        {
            state.logger = QUtilLogger{ 0,  0, qpi.invocator(), input.dst23, input.amt23, STM1_SEND_FUND };
            LOG_INFO(state.logger);
            qpi.transfer(input.dst23, input.amt23);
        }
        if (input.dst24 != NULL_ID)
        {
            state.logger = QUtilLogger{ 0,  0, qpi.invocator(), input.dst24, input.amt24, STM1_SEND_FUND };
            LOG_INFO(state.logger);
            qpi.transfer(input.dst24, input.amt24);
        }
        state.logger = QUtilLogger{ 0,  0, qpi.invocator(), SELF, state.total, STM1_SUCCESS };
        LOG_INFO(state.logger);
        output.returnCode = STM1_SUCCESS;
        qpi.burn(STM1_INVOCATION_FEE);
    }

    /**
    * Send n transfers of 1 qu each from a single address to a specified number of addresses.
    * If there are not enough entities in the spectrum, a single entity might be chosen multiple times.
    * @param number of addresses that will be sent n times 1 qubic
    * @param number of transfers for each address
    * @return returnCode (0 means success)
    */
    PUBLIC_PROCEDURE_WITH_LOCALS(SendToManyBenchmark)
    {
        state.logger = QUtilLogger{ 0,  0, qpi.invocator(), SELF, qpi.invocationReward(), STM1_TRIGGERED };
        LOG_INFO(state.logger);
        output.total = 0;

        // Number of addresses and transfers is > 0 and total transfers do not exceed limit (including 2 transfers from invocator to contract and contract to invocator)
        if (input.dstCount <= 0 || input.numTransfersEach <= 0 || input.dstCount * input.numTransfersEach + 2 > CONTRACT_ACTION_TRACKER_SIZE)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            state.logger = QUtilLogger{ 0,  0, qpi.invocator(), SELF, qpi.invocationReward(), STM1_INVALID_AMOUNT_NUMBER };
            LOG_INFO(state.logger);
            output.returnCode = STM1_INVALID_AMOUNT_NUMBER;
            return;
        }

        // Check the fund is enough
        if (qpi.invocationReward() < input.dstCount * input.numTransfersEach)
        {
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            state.logger = QUtilLogger{ 0,  0, qpi.invocator(), SELF, qpi.invocationReward(), STM1_INVALID_AMOUNT_NUMBER };
            LOG_INFO(state.logger);
            output.returnCode = STM1_INVALID_AMOUNT_NUMBER;
            return;
        }

        // Loop through the number of addresses and do the transfers
        locals.currentId = qpi.invocator();
        locals.useNext = true;
        while (output.dstCount < input.dstCount)
        {
            if (locals.useNext)
                locals.currentId = qpi.nextId(locals.currentId);
            else
                locals.currentId = qpi.prevId(locals.currentId);
            if (locals.currentId == m256i::zero())
            {
                locals.currentId = qpi.invocator();
                locals.useNext = !locals.useNext;
                continue;
            }

            output.dstCount++;
            for (locals.t = 0; locals.t < input.numTransfersEach; locals.t++)
            {
                qpi.transfer(locals.currentId, 1);
                output.total += 1;
            }
        }

        // Return the change if there is any
        if (output.total < qpi.invocationReward())
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - output.total);
        }

        state.logger = QUtilLogger{ 0,  0, qpi.invocator(), SELF, output.total, STM1_SUCCESS };
        LOG_INFO(state.logger);
    }

    /**
    * Practicing burning qubic in the QChurch
    * @param the amount of qubic to burn
    * @return the amount of qubic has burned, < 0 if failed to burn
    */
    PUBLIC_PROCEDURE(BurnQubic)
    {
        // lack of fund => return the coins
        if (input.amount < 0) // invalid input amount
        {
            output.amount = -1;
            return;
        }
        if (input.amount == 0)
        {
            output.amount = 0;
            return;            
        }
        if (qpi.invocationReward() < input.amount) // not sending enough qu to burn
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
            output.amount = -1;
            return;
        }
        if (qpi.invocationReward() > input.amount) // send more than qu to burn
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - input.amount); // return the changes
        }
        qpi.burn(input.amount);
        output.amount = input.amount;
        return;
    }

    /*
    * @return Return total number of shares that currently exist of the asset given as input
    */
    PUBLIC_FUNCTION(GetTotalNumberOfAssetShares)
    {
        output = qpi.numberOfShares(input);
    }

    REGISTER_USER_FUNCTIONS_AND_PROCEDURES()
    {
        REGISTER_USER_FUNCTION(GetSendToManyV1Fee, 1);
        REGISTER_USER_FUNCTION(GetTotalNumberOfAssetShares, 2);

        REGISTER_USER_PROCEDURE(SendToManyV1, 1);
        REGISTER_USER_PROCEDURE(BurnQubic, 2);
        REGISTER_USER_PROCEDURE(SendToManyBenchmark, 3);
    }
};
