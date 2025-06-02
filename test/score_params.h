#pragma once

namespace score_params
{
enum ParamType
{
    NUMBER_OF_INPUT_NEURONS, // K
    NUMBER_OF_OUTPUT_NEURONS,// L
    NUMBER_OF_TICKS,        // N
    NUMBER_OF_NEIGHBORS,    // 2M
    POPULATION_THRESHOLD,  // P
    NUMBER_OF_MUTATIONS,    // S
    SOLUTION_THRESHOLD,
    MAX_PARAM_TYPE
};


// Comment out when we want to reduce the number of running test
static constexpr unsigned long long kSettings[][MAX_PARAM_TYPE] = {
    //{32, 32, 20, 16, 100, 8, 12},
    //{512, 128, 20, 64, 1000, 20, 100},
    {1024, 1024, 100, 1024, 3000, 100, 768},
};
}
