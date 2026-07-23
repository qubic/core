#pragma once

#include "../src/mining/score_common.h"
#include "../src/public_settings.h"

#include <tuple>

namespace score_params
{

// bpp9000 test configs - plain Bpp9000Params (the leaf is tested directly, no 2-slot ConfigPair).
// These match the golden set header in data/scores_bpp9000.csv, which was generated on
// data/example_task_bpp9000.bin; configs 0..2 are the first-T-rows subviews, config 3 is the full task.
// Bpp9000Params order: <inputNeurons, outputNeurons, seqLength, winWidth, maxTicks, neighbor, population, mutations, threshold>
using Config0 = score_engine::Bpp9000Params<18, 1, 128, 32, 5000, 3, 64, 10, 76>;
using Config1 = score_engine::Bpp9000Params<18, 1, 512, 128, 20000, 3, 64, 15, 306>;
using Config2 = score_engine::Bpp9000Params<18, 1, 2048, 512, 80000, 3, 64, 20, 1228>;
using Config3 = score_engine::Bpp9000Params<18, 1, 8760, 672, 100000, 3, 64, 5, 6469>;

using ConfigList = std::tuple<Config0, Config1, Config2, Config3>;
static constexpr std::size_t CONFIG_COUNT = std::tuple_size_v<ConfigList>;

// The production node config (the BPP9000_* macros from public_settings.h) - the real mining setting.
using ProductionConfig = score_engine::Bpp9000Params<
    BPP9000_NUMBER_OF_INPUT_NEURONS, BPP9000_NUMBER_OF_OUTPUT_NEURONS, BPP9000_SEQUENCE_LENGTH,
    BPP9000_WINDOW_WIDTH, BPP9000_MAX_NUMBER_OF_TICKS, BPP9000_NUMBER_OF_NEIGHBORS,
    BPP9000_POPULATION_THRESHOLD, BPP9000_NUMBER_OF_MUTATIONS, BPP9000_SOLUTION_THRESHOLD_DEFAULT>;

}
