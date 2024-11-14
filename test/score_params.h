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
    {512, 512, 5288ULL},
    {512, 256, 20972ULL},
    {512, 512, 20152ULL},

    {2048, 1024, 41944ULL},
    {2048, 2048, 41904ULL},
    {2048, 1024, 83168ULL},
    {2048, 2048, 81868ULL},

    {8192, 4096, 33552ULL},
    {8192, 8192, 33532ULL},
    {8192, 4096, 60864ULL},
    {8192, 8192, 67184ULL},

    {6000, 2376, 11996136ULL},
    {6000, 832, 52158792ULL},
    {6080, 2792, 45545677ULL},
    {6024, 4784, 35247639ULL},
    {7192, 5184, 32472324ULL},
    {12088, 7864, 22188546ULL},

    {24000, 10000, 5000000ULL}

};
}
