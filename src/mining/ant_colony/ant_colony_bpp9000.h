#pragma once

#include "mining/ant_colony/ant_colony.h"
#include "score.h"

// Binds the colony to bpp9000. This is the only place a concrete scorer is named, which is why
// ant_colony.h itself can stay free of score.h and everything it drags in.
using AntColonyBpp9000T = AntColony<score_engine::ScoreBpp9000T>;
