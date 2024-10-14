#pragma once

namespace score_params
{
enum ParamType
{
    NR_NEURONS = 0,
    NR_NEIGHBOR_NEURONS,
    DURATIONS,
    MAX_PARAM_TYPE
};

static constexpr unsigned long long kDataLength = 256;

// Comment out when we want to reduce the number of running test
static constexpr unsigned long long kSettings[][MAX_PARAM_TYPE] = {
    {512, 512, 1024},
    {512, 256, 4096},
    {512, 512, 4096},

    {2048, 1024, 1024},
    {2048, 2048, 1024},
    {2048, 1024, 2048},
    {2048, 2048, 2048},

    {8192, 4096, 4096},
    {8192, 8192, 4096},
    {8192, 4096, 8192},
    {8192, 8192, 8192},

    {6000, 2376, 1000},
    {6000, 832, 686},
    {6080, 792, 1574},
    {6024, 1560, 2023},
    {7192, 520, 2000},
    {7192, 2184, 854},
    {12088, 7864, 623},

    {6000, 2000, 2000},
    {12000, 2000, 1000},
    {12000, 2000, 2000},
};
}
