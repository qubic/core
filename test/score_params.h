#pragma once

#include "../src/mining/score_common.h"
#include "../src/public_settings.h"

#include <tuple>

namespace score_params
{

// The production node config (the BPP9000_* macros from public_settings.h) - the real mining setting.
using ProductionConfig = score_engine::Bpp9000Params<
    BPP9000_NUMBER_OF_INPUT_NEURONS, BPP9000_NUMBER_OF_OUTPUT_NEURONS, BPP9000_SEQUENCE_LENGTH,
    BPP9000_WINDOW_WIDTH, BPP9000_MAX_NUMBER_OF_TICKS, BPP9000_NUMBER_OF_NEIGHBORS,
    BPP9000_POPULATION_THRESHOLD, BPP9000_NUMBER_OF_MUTATIONS, BPP9000_SOLUTION_THRESHOLD_DEFAULT,
    BPP9000_SHIFT_CAP>;

}
