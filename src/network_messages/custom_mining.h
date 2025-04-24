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
        type = 56,
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
        type = 57,
    };
    unsigned long long taskIndex;
    unsigned int nonce; // nonce of invalid solution
    unsigned int padding;
};


//template <typename TaskT, typename SolT>
//struct RespondCustomMiningTask
//{
//    static constexpr int _maxNumberOfTasks = 4;
//    static constexpr int _maxNumberSolutions = 64;
//    enum
//    {
//        type = 57,
//    };
//    unsigned long long _taskCount;
//    unsigned long long _maxTaskCount;
//
//    unsigned long long _solutionsCount;
//    unsigned long long _maxSolutionsCount;
//
//    TaskT _task[_maxNumberOfTasks];
//    SolT _solutions[_maxNumberSolutions];
//
//    static_assert(sizeof(_task) % 8 == 0, "The max number of tasks must be a multiple of 8 bytes");
//    static_assert(sizeof(_solutions) % 8 == 0, "The max number of solutions must be a multiple of 8 bytes");
//};
