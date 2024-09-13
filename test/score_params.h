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
    {512, 256, 1024},
    {512, 512, 1024},
    {512, 256, 4096},
    {512, 512, 4096},

    {1024, 512, 1024},
    {1024, 1024, 1024},
    {1024, 512, 2048},
    {1024, 1024, 2048},

    {2048, 1024, 1024},
    {2048, 2048, 1024},
    {2048, 1024, 2048},
    {2048, 2048, 2048},

    {4096, 2048, 4096},
    {4096, 4096, 4096},
    {4096, 2048, 8192},
    {4096, 4096, 8192},

    {8192, 4096, 4096},
    {8192, 8192, 4096},
    {8192, 4096, 8192},
    {8192, 8192, 8192},
};
}
