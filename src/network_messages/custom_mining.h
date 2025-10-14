#pragma once

// Message struture for request custom mining data
struct RequestedCustomMiningData
{
    enum
    {
        type = 60,
    };
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
    enum
    {
        type = 61,
    };
    enum
    {
        taskType = 0,
        solutionType = 1,
    };
    // The 'payload' variable is defined externally and usually contains a byte array.
    // Ussualy: [CustomMiningRespondDataHeader ... NumberOfItems * ItemSize];
};

struct RequestedCustomMiningSolutionVerification
{
    enum
    {
        type = 62,
    };
    unsigned long long taskIndex;
    unsigned long long nonce;
    unsigned long long encryptionLevel;
    unsigned long long computorRandom;
    unsigned long long reserve2;
    unsigned long long isValid;  // validity of the solution. 0: invalid, >0: valid

};
struct RespondCustomMiningSolutionVerification
{
    enum
    {
        type = 63,
    };
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

