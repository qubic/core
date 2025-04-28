#pragma once

// Message struture for request custom mining data
struct RequestedCustomMiningData
{
    enum
    {
        type = 55,
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
    unsigned int dataType;
};

// Message struture for respond custom mining data
struct RespondCustomMiningData
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
    // The 'payload' variable is defined externally and usually contains a byte array.
    // Ussualy: [CustomMiningRespondDataHeader ... NumberOfItems * ItemSize];
};

struct RequestedCustomMiningSolutionVerification
{
    enum
    {
        type = 61,
    };
    unsigned long long taskIndex;
    unsigned int nonce;
    unsigned int padding;   // XMR padding data
    unsigned char isValid;  // validity of the solution. 0: invalid, >0: valid
};
