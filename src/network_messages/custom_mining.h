#pragma once

#include "network_message_type.h"


// Message struture for request custom mining data
struct RequestCustomMiningData
{
    static constexpr unsigned char type()
    {
        return NetworkMessageType::REQUEST_CUSTOM_MINING_DATA;
    }

    enum
    {
        taskType = 0,
        solutionType = 1,
    };
    // Task index information:
    // - For taskType: 'fromTaskIndex' is the lower bound and 'toTaskIndex' is the upper bound of the task range [fromTaskIndex, toTaskIndex].
    // - For solutionType: only 'fromTaskIndex' is used, since the solution response is tied to a single task.
    unsigned long long fromTaskIndex;
    unsigned long long toTaskIndex;

    // Type of the request: either task (taskType) or solution (solutionType).
    long long dataType;
};

// Message struture for respond custom mining data
struct RespondCustomMiningData
{
    static constexpr unsigned char type()
    {
        return NetworkMessageType::RESPOND_CUSTOM_MINING_DATA;
    }

    enum
    {
        taskType = 0,
        solutionType = 1,
    };
    // The 'payload' variable is defined externally and usually contains a byte array.
    // Ussualy: [CustomMiningRespondDataHeader ... NumberOfItems * ItemSize];
};

struct RequestCustomMiningSolutionVerification
{
    static constexpr unsigned char type()
    {
        return NetworkMessageType::REQUEST_CUSTOM_MINING_SOLUTION_VERIFICATION;
    }

    unsigned long long taskIndex;
    unsigned long long nonce;
    unsigned long long encryptionLevel;
    unsigned long long computorRandom;
    unsigned long long reserve2;
    unsigned long long isValid;  // validity of the solution. 0: invalid, >0: valid

};
struct RespondCustomMiningSolutionVerification
{
    static constexpr unsigned char type()
    {
        return NetworkMessageType::RESPOND_CUSTOM_MINING_SOLUTION_VERIFICATION;
    }

    enum
    {
        notExisted = 0,             // solution not existed in cache
        valid = 1,                  // solution are set as valid
        invalid = 2,                // solution are set as invalid
        customMiningStateEnded = 3, // not in custom mining state
    };
    unsigned long long taskIndex;
    unsigned long long nonce;
    unsigned long long encryptionLevel;
    unsigned long long computorRandom;
    unsigned long long reserve2;
    long long status;       // Flag indicate the status of solution
};

