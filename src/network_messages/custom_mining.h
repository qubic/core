#pragma once

struct RequestedCustomMiningVerification
{
    enum
    {
        type = 55,
    };
    unsigned long long everIncreasingNonceAndCommandType;
    unsigned long long taskIndex;
    unsigned int nonce;
    unsigned int padding; // use later
};

struct RequestedCustomMiningData
{
    enum
    {
        type = 56,
    };
    unsigned long long fromTaskIndex;
};

template <typename TaskT, typename SolT>
struct RespondCustomMiningTask
{
    static constexpr int _maxNumberOfTasks = 4;
    static constexpr int _maxNumberSolutions = 64;
    enum
    {
        type = 57,
    };
    unsigned long long _taskCount;
    unsigned long long _maxTaskCount;

    unsigned long long _solutionsCount;
    unsigned long long _maxSolutionsCount;

    TaskT _task[_maxNumberOfTasks];
    SolT _solutions[_maxNumberSolutions];

    static_assert(sizeof(_task) % 8 == 0, "The max number of tasks must be a multiple of 8 bytes");
    static_assert(sizeof(_solutions) % 8 == 0, "The max number of solutions must be a multiple of 8 bytes");
};
