#pragma once

#include "common_def.h"


struct SpecialCommand
{
    unsigned long long everIncreasingNonceAndCommandType;

    static constexpr unsigned char type()
    {
        return NetworkMessageType::SPECIAL_COMMAND;
    }
};

#define SPECIAL_COMMAND_SHUT_DOWN 0ULL


#define SPECIAL_COMMAND_SET_SOLUTION_THRESHOLD_REQUEST 5ULL
#define SPECIAL_COMMAND_SET_SOLUTION_THRESHOLD_RESPONSE 6ULL
struct SpecialCommandSetSolutionThresholdRequestAndResponse
{
    unsigned long long everIncreasingNonceAndCommandType;
    unsigned int epoch;
    int threshold;
};


#define SPECIAL_COMMAND_TOGGLE_MAIN_MODE_REQUEST 7ULL // F12
#define SPECIAL_COMMAND_TOGGLE_MAIN_MODE_RESPONSE 8ULL // F12
struct SpecialCommandToggleMainModeRequestAndResponse
{
    unsigned long long everIncreasingNonceAndCommandType;
    unsigned char mainModeFlag; // 0 Aux - 1 Main
    unsigned char padding[7];
};
#define SPECIAL_COMMAND_REFRESH_PEER_LIST 9ULL // F4
#define SPECIAL_COMMAND_FORCE_NEXT_TICK 10ULL // F5
#define SPECIAL_COMMAND_REISSUE_VOTE 11ULL // F9


struct UtcTime
{
    unsigned short    year;              // 1900 - 9999
    unsigned char     month;             // 1 - 12
    unsigned char     day;               // 1 - 31
    unsigned char     hour;              // 0 - 23
    unsigned char     minute;            // 0 - 59
    unsigned char     second;            // 0 - 59
    unsigned char     pad1;
    unsigned int      nanosecond;        // 0 - 999,999,999
};

#define SPECIAL_COMMAND_QUERY_TIME 12ULL    // send this to node to query time, responds with time read from clock
#define SPECIAL_COMMAND_SEND_TIME 13ULL     // send this to node to set time, responds with time read from clock after setting

struct SpecialCommandSendTime
{
    unsigned long long everIncreasingNonceAndCommandType;
    UtcTime utcTime;
};

#define SPECIAL_COMMAND_GET_MINING_SCORE_RANKING 14ULL
#pragma pack( push, 1)
template<unsigned int maxNumberOfMiners>
struct SpecialCommandGetMiningScoreRanking
{
    struct ScoreRankingEntry {
        m256i minerPublicKey;
        unsigned int minerScore;
    };

    unsigned long long everIncreasingNonceAndCommandType;
    unsigned int numberOfRankings;
    ScoreRankingEntry rankings[maxNumberOfMiners];
};

#define SPECIAL_COMMAND_FORCE_SWITCH_EPOCH 15ULL // F7
#define SPECIAL_COMMAND_CONTINUE_SWITCH_EPOCH 16ULL // F10 (only log-enabled nodes need this)

#define SPECIAL_COMMAND_SET_CONSOLE_LOGGING_MODE 17ULL // PAUSE key
struct SpecialCommandSetConsoleLoggingModeRequestAndResponse
{
    unsigned long long everIncreasingNonceAndCommandType;
    unsigned char loggingMode; // 0 disabled, 1 low computational cost, 2 full logging
    unsigned char padding[7];
};

#define SPECIAL_COMMAND_SAVE_SNAPSHOT 18ULL // F8 key
struct SpecialCommandSaveSnapshotRequestAndResponse
{
    enum
    {
        SAVING_TRIGGERED = 0,
        SAVING_IN_PROGRESS,
        REMOTE_SAVE_MODE_DISABLED,
        UNKNOWN_FAILURE,
    };

    unsigned long long everIncreasingNonceAndCommandType;
    unsigned int currentTick;
    unsigned char status;
    unsigned char padding[3];
};

#pragma pack(pop)

#define SPECIAL_COMMAND_SET_EXECUTION_FEE_MULTIPLIER 19ULL
#define SPECIAL_COMMAND_GET_EXECUTION_FEE_MULTIPLIER 20ULL
// This struct is used as response for the get command and as request and response for the set command.
struct SpecialCommandExecutionFeeMultiplierRequestAndResponse
{
    unsigned long long everIncreasingNonceAndCommandType;
    unsigned long long multiplierNumerator;
    unsigned long long multiplierDenominator;
};