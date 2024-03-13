#pragma once

#include "common_def.h"

struct ComputorProposal
{
    unsigned char uriSize;
    unsigned char uri[255];
};
struct ComputorBallot
{
    unsigned char zero;
    unsigned char votes[(NUMBER_OF_COMPUTORS * 3 + 7) / 8];
    unsigned char quasiRandomNumber;
};

struct SpecialCommand
{
    unsigned long long everIncreasingNonceAndCommandType;

    enum {
        type = 255,
    };
};

#define SPECIAL_COMMAND_SHUT_DOWN 0ULL

#define SPECIAL_COMMAND_GET_PROPOSAL_AND_BALLOT_REQUEST 1ULL
struct SpecialCommandGetProposalAndBallotRequest
{
    unsigned long long everIncreasingNonceAndCommandType;
    unsigned short computorIndex;
    unsigned char padding[6];
    unsigned char signature[SIGNATURE_SIZE];
};

#define SPECIAL_COMMAND_GET_PROPOSAL_AND_BALLOT_RESPONSE 2ULL
struct SpecialCommandGetProposalAndBallotResponse
{
    unsigned long long everIncreasingNonceAndCommandType;
    unsigned short computorIndex;
    unsigned char padding[6];
    ComputorProposal proposal;
    ComputorBallot ballot;
};

#define SPECIAL_COMMAND_SET_PROPOSAL_AND_BALLOT_REQUEST 3ULL
struct SpecialCommandSetProposalAndBallotRequest
{
    unsigned long long everIncreasingNonceAndCommandType;
    unsigned short computorIndex;
    unsigned char padding[6];
    ComputorProposal proposal;
    ComputorBallot ballot;
    unsigned char signature[SIGNATURE_SIZE];
};

#define SPECIAL_COMMAND_SET_PROPOSAL_AND_BALLOT_RESPONSE 4ULL
struct SpecialCommandSetProposalAndBallotResponse
{
    unsigned long long everIncreasingNonceAndCommandType;
    unsigned short computorIndex;
    unsigned char padding[6];
};

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
