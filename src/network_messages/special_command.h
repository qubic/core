#pragma once

#include "common_def.h"


struct SpecialCommand
{
    unsigned long long everIncreasingNonceAndCommandType;

    enum {
        type = 255,
    };
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

#pragma pack(pop)
