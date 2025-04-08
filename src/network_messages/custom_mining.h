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

struct RequestedCustomMiningTask
{
    enum
    {
        type = 56,
    };
    unsigned long long fromTaskIndex;
};

template <typename T>
struct RespondCustomMiningTask
{
    static constexpr int _maxNumberOfTasks = 32;
    enum
    {
        type = 57,
    };
    T _task[_maxNumberOfTasks];
    unsigned long long _taskCount;
    unsigned long long _maxTaskCount;

    static_assert(sizeof(_task) % 8 == 0, "The max number of tasks must be a multiple of 8 bytes");
};
