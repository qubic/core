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
    {64, 64, 50, 64, 178, 50, 36},
    {256, 256, 120, 256, 612, 100, 171},
    {512, 512, 150, 512, 1174, 150, 300},
    {1024, 1024, 200, 1024, 3000, 200, 600}
};

static constexpr unsigned long long kProfileSettings[][MAX_PARAM_TYPE] = {
    {::NUMBER_OF_INPUT_NEURONS, ::NUMBER_OF_OUTPUT_NEURONS, ::NUMBER_OF_TICKS, ::NUMBER_OF_NEIGHBORS, ::POPULATION_THRESHOLD, ::NUMBER_OF_MUTATIONS, ::SOLUTION_THRESHOLD_DEFAULT},
};

}
