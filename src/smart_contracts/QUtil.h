using namespace QPI;
/**************************************/
/**************SC UTILS****************/
/**************************************/
/*
* A collection of useful functions for smart contract on Qubic:
* - SendToManyV1 (STM1): Sending qu from a single address to multiple addresses (upto 25)
* ...
* ...
*/

// Return code for logger and return struct
#define STM1_SUCCESS 0
#define STM1_INVALID_AMOUNT_NUMBER 1
#define STM1_INSUFFICIENT_FUND 2
#define STM1_TRIGGERED 3
#define STM1_SEND_FUND 4
enum QUtilLog {
    success = 0,
    invalidAmountNumber = 1,
    insufficientFund = 2,
    triggerSendToManyV1 = 3,
    sendFundViaSendToManyV1 = 4,
    totalLogType = 5
};
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

struct QUTIL
{
private:
    // registers, logger and other buffer
    sint32 _i0, _i1, _i2, _i3;
    sint64 _i64_0, _i64_1, _i64_2, _i64_3;
    uint64 _r0, _r1, _r2, _r3;
    sint64 total;
    QUtilLogger logger;
public:
    struct SendToManyV1_input
    {
        id dst0, dst1, dst2, dst3, dst4, dst5, dst6, dst7, dst8, dst9, dst10, dst11, dst12,
            dst13, dst14, dst15, dst16, dst17, dst18, dst19, dst20, dst21, dst22, dst23, dst24;
        sint64 amt0, amt1, amt2, amt3, amt4, amt5, amt6, amt7, amt8, amt9, amt10, amt11, amt12,
            amt13, amt14, amt15, amt16, amt17, amt18, amt19, amt20, amt21, amt22, amt23, amt24;
    };
    struct SendToManyV1_output
    {
    };

    PUBLIC(SendToManyV1)
        state.logger = QUtilLogger{ 0,  0, invocator(), SELF, invocationReward(), QUtilLog::triggerSendToManyV1 };
        LOG_INFO(state.logger);
        state.total = input.amt0 + input.amt1 + input.amt2 + input.amt3 + input.amt4 + input.amt5 + input.amt6 + input.amt7 + input.amt8 + input.amt9 + input.amt10 + input.amt11 + input.amt12 + input.amt13 + input.amt14 + input.amt15 + input.amt16 + input.amt17 + input.amt18 + input.amt19 + input.amt20 + input.amt21 + input.amt22 + input.amt23 + input.amt24;
        // invalid amount (<0), return fund and exit
        if ((input.amt0 < 0) || (input.amt1 < 0) || (input.amt2 < 0) || (input.amt3 < 0) || (input.amt4 < 0) || (input.amt5 < 0) || (input.amt6 < 0) || (input.amt7 < 0) || (input.amt8 < 0) || (input.amt9 < 0) || (input.amt10 < 0) || (input.amt11 < 0) || (input.amt12 < 0) || (input.amt13 < 0) || (input.amt14 < 0) || (input.amt15 < 0) || (input.amt16 < 0) || (input.amt17 < 0) || (input.amt18 < 0) || (input.amt19 < 0) || (input.amt20 < 0) || (input.amt21 < 0) || (input.amt22 < 0) || (input.amt23 < 0) || (input.amt24 < 0))
        {
            state.logger = QUtilLogger{ 0,  0, invocator(), SELF, invocationReward(), QUtilLog::invalidAmountNumber };
            LOG_INFO(state.logger);
            if (invocationReward() > 0)
            {
                transfer(invocator(), invocationReward());
            }
        }
        // insufficient fund, return fund and exit
        if (invocationReward() < state.total)
        {
            state.logger = QUtilLogger{ 0,  0, invocator(), SELF, invocationReward(), QUtilLog::insufficientFund };
            LOG_INFO(state.logger);
            if (invocationReward() > 0)
            {
                transfer(invocator(), invocationReward());
            }
            return;
        }

        if (input.amt0 > 0)
        {
            state.logger = QUtilLogger{ 0,  0, invocator(), input.dst0, input.amt0, QUtilLog::sendFundViaSendToManyV1 };
            LOG_INFO(state.logger);
            transfer(input.dst0, input.amt0);
        }
        if (input.amt1 > 0)
        {
            state.logger = QUtilLogger{ 0,  0, invocator(), input.dst1, input.amt1, QUtilLog::sendFundViaSendToManyV1 };
            LOG_INFO(state.logger);
            transfer(input.dst1, input.amt1);
        }
        if (input.amt2 > 0)
        {
            state.logger = QUtilLogger{ 0,  0, invocator(), input.dst2, input.amt2, QUtilLog::sendFundViaSendToManyV1 };
            LOG_INFO(state.logger);
            transfer(input.dst2, input.amt2);
        }
        if (input.amt3 > 0)
        {
            state.logger = QUtilLogger{ 0,  0, invocator(), input.dst3, input.amt3, QUtilLog::sendFundViaSendToManyV1 };
            LOG_INFO(state.logger);
            transfer(input.dst3, input.amt3);
        }
        if (input.amt4 > 0)
        {
            state.logger = QUtilLogger{ 0,  0, invocator(), input.dst4, input.amt4, QUtilLog::sendFundViaSendToManyV1 };
            LOG_INFO(state.logger);
            transfer(input.dst4, input.amt4);
        }
        if (input.amt5 > 0)
        {
            state.logger = QUtilLogger{ 0,  0, invocator(), input.dst5, input.amt5, QUtilLog::sendFundViaSendToManyV1 };
            LOG_INFO(state.logger);
            transfer(input.dst5, input.amt5);
        }
        if (input.amt6 > 0)
        {
            state.logger = QUtilLogger{ 0,  0, invocator(), input.dst6, input.amt6, QUtilLog::sendFundViaSendToManyV1 };
            LOG_INFO(state.logger);
            transfer(input.dst6, input.amt6);
        }
        if (input.amt7 > 0)
        {
            state.logger = QUtilLogger{ 0,  0, invocator(), input.dst7, input.amt7, QUtilLog::sendFundViaSendToManyV1 };
            LOG_INFO(state.logger);
            transfer(input.dst7, input.amt7);
        }
        if (input.amt8 > 0)
        {
            state.logger = QUtilLogger{ 0,  0, invocator(), input.dst8, input.amt8, QUtilLog::sendFundViaSendToManyV1 };
            LOG_INFO(state.logger);
            transfer(input.dst8, input.amt8);
        }
        if (input.amt9 > 0)
        {
            state.logger = QUtilLogger{ 0,  0, invocator(), input.dst9, input.amt9, QUtilLog::sendFundViaSendToManyV1 };
            LOG_INFO(state.logger);
            transfer(input.dst9, input.amt9);
        }
        if (input.amt10 > 0)
        {
            state.logger = QUtilLogger{ 0,  0, invocator(), input.dst10, input.amt10, QUtilLog::sendFundViaSendToManyV1 };
            LOG_INFO(state.logger);
            transfer(input.dst10, input.amt10);
        }
        if (input.amt11 > 0)
        {
            state.logger = QUtilLogger{ 0,  0, invocator(), input.dst11, input.amt11, QUtilLog::sendFundViaSendToManyV1 };
            LOG_INFO(state.logger);
            transfer(input.dst11, input.amt11);
        }
        if (input.amt12 > 0)
        {
            state.logger = QUtilLogger{ 0,  0, invocator(), input.dst12, input.amt12, QUtilLog::sendFundViaSendToManyV1 };
            LOG_INFO(state.logger);
            transfer(input.dst12, input.amt12);
        }
        if (input.amt13 > 0)
        {
            state.logger = QUtilLogger{ 0,  0, invocator(), input.dst13, input.amt13, QUtilLog::sendFundViaSendToManyV1 };
            LOG_INFO(state.logger);
            transfer(input.dst13, input.amt13);
        }
        if (input.amt14 > 0)
        {
            state.logger = QUtilLogger{ 0,  0, invocator(), input.dst14, input.amt14, QUtilLog::sendFundViaSendToManyV1 };
            LOG_INFO(state.logger);
            transfer(input.dst14, input.amt14);
        }
        if (input.amt15 > 0)
        {
            state.logger = QUtilLogger{ 0,  0, invocator(), input.dst15, input.amt15, QUtilLog::sendFundViaSendToManyV1 };
            LOG_INFO(state.logger);
            transfer(input.dst15, input.amt15);
        }
        if (input.amt16 > 0)
        {
            state.logger = QUtilLogger{ 0,  0, invocator(), input.dst16, input.amt16, QUtilLog::sendFundViaSendToManyV1 };
            LOG_INFO(state.logger);
            transfer(input.dst16, input.amt16);
        }
        if (input.amt17 > 0)
        {
            state.logger = QUtilLogger{ 0,  0, invocator(), input.dst17, input.amt17, QUtilLog::sendFundViaSendToManyV1 };
            LOG_INFO(state.logger);
            transfer(input.dst17, input.amt17);
        }
        if (input.amt18 > 0)
        {
            state.logger = QUtilLogger{ 0,  0, invocator(), input.dst18, input.amt18, QUtilLog::sendFundViaSendToManyV1 };
            LOG_INFO(state.logger);
            transfer(input.dst18, input.amt18);
        }
        if (input.amt19 > 0)
        {
            state.logger = QUtilLogger{ 0,  0, invocator(), input.dst19, input.amt19, QUtilLog::sendFundViaSendToManyV1 };
            LOG_INFO(state.logger);
            transfer(input.dst19, input.amt19);
        }
        if (input.amt20 > 0)
        {
            state.logger = QUtilLogger{ 0,  0, invocator(), input.dst20, input.amt20, QUtilLog::sendFundViaSendToManyV1 };
            LOG_INFO(state.logger);
            transfer(input.dst20, input.amt20);
        }
        if (input.amt21 > 0)
        {
            state.logger = QUtilLogger{ 0,  0, invocator(), input.dst21, input.amt21, QUtilLog::sendFundViaSendToManyV1 };
            LOG_INFO(state.logger);
            transfer(input.dst21, input.amt21);
        }
        if (input.amt22 > 0)
        {
            state.logger = QUtilLogger{ 0,  0, invocator(), input.dst22, input.amt22, QUtilLog::sendFundViaSendToManyV1 };
            LOG_INFO(state.logger);
            transfer(input.dst22, input.amt22);
        }
        if (input.amt23 > 0)
        {
            state.logger = QUtilLogger{ 0,  0, invocator(), input.dst23, input.amt23, QUtilLog::sendFundViaSendToManyV1 };
            LOG_INFO(state.logger);
            transfer(input.dst23, input.amt23);
        }
        if (input.amt24 > 0)
        {
            state.logger = QUtilLogger{ 0,  0, invocator(), input.dst24, input.amt24, QUtilLog::sendFundViaSendToManyV1 };
            LOG_INFO(state.logger);
            transfer(input.dst24, input.amt24);
        }
        // return changes
        if (invocationReward() > state.total)
        {
            transfer(invocator(), invocationReward() - state.total);
        }
        state.logger = QUtilLogger{ 0,  0, invocator(), SELF, state.total, QUtilLog::success };
        LOG_INFO(state.logger);
    _
    REGISTER_USER_FUNCTIONS
    _

    REGISTER_USER_PROCEDURES
        REGISTER_USER_PROCEDURE(SendToManyV1, 1);
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