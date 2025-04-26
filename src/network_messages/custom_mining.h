#pragma once

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

    unsigned long long fromTaskIndex;
    unsigned long long toTaskIndex;
    unsigned int dataType;
};

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
};

struct RequestedCustomMiningInvalidateSolution
{
    enum
    {
        type = 61,
    };
    unsigned long long taskIndex;
    unsigned int nonce; // nonce of invalid solution
    unsigned int padding;
};
