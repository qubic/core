#pragma once

#include "../src/mining/score_common.h"

namespace score_params
{

static constexpr unsigned int MAX_PARAM_TYPE = 7;

template<typename HI, typename ADD>
struct ConfigPair
{
    using HyperIdentity = HI;
    using Addition = ADD;
};

// All configurations
using Config0 = ConfigPair<
    score_engine::HyperIdentityParams<64, 64, 50, 64, 178, 50, 36>,
    score_engine::AdditionParams<2 * 2, 3, 50, 64, 100, 50, 36>
>;

using Config1 = ConfigPair<
    score_engine::HyperIdentityParams<256, 256, 120, 256, 612, 100, 171>,
    score_engine::AdditionParams<4 * 2, 5, 120, 256, 100 + 8 + 5, 100, 171>
>;

using Config2 = ConfigPair<
    score_engine::HyperIdentityParams<512, 512, 150, 512, 1174, 150, 300>,
    score_engine::AdditionParams<7 * 2, 8, 150, 512, 150 + 14 + 8, 150, 600>
>;

using Config3 = ConfigPair<
    score_engine::HyperIdentityParams<1024, 1024, 200, 1024, 3000, 200, 600>,
    score_engine::AdditionParams<9 * 2, 10, 200, 1024, 200 + 18 + 10, 200, 600>
>;

using ConfigProfile = ConfigPair<
    score_engine::HyperIdentityParams<
        HYPERIDENTITY_NUMBER_OF_INPUT_NEURONS,
        HYPERIDENTITY_NUMBER_OF_OUTPUT_NEURONS,
        HYPERIDENTITY_NUMBER_OF_TICKS,
        HYPERIDENTITY_NUMBER_OF_NEIGHBORS,
        HYPERIDENTITY_POPULATION_THRESHOLD,
        HYPERIDENTITY_NUMBER_OF_MUTATIONS,
        HYPERIDENTITY_SOLUTION_THRESHOLD_DEFAULT>,
    score_engine::AdditionParams<
        ADDITION_NUMBER_OF_INPUT_NEURONS,
        ADDITION_NUMBER_OF_OUTPUT_NEURONS,
        ADDITION_NUMBER_OF_TICKS,
        ADDITION_NUMBER_OF_NEIGHBORS,
        ADDITION_POPULATION_THRESHOLD,
        ADDITION_NUMBER_OF_MUTATIONS,
        ADDITION_SOLUTION_THRESHOLD_DEFAULT>
>;

using ConfigList = std::tuple<Config0, Config1, Config2, Config3>;

static constexpr std::size_t CONFIG_COUNT = std::tuple_size_v<ConfigList>;

using ProfileConfigList = std::tuple<ConfigProfile>;
static constexpr std::size_t PROFILE_CONFIG_COUNT = std::tuple_size_v<ProfileConfigList>;


}
