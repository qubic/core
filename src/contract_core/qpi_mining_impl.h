#pragma once

#include "contracts/qpi.h"
#include "score.h"

static ScoreFunction<
    NUMBER_OF_INPUT_NEURONS,
    NUMBER_OF_OUTPUT_NEURONS,
    NUMBER_OF_TICKS*2,
    NUMBER_OF_NEIGHBORS,
    POPULATION_THRESHOLD,
    NUMBER_OF_MUTATIONS,
    SOLUTION_THRESHOLD_DEFAULT,
    1
>* score_qpi = nullptr; // NOTE: SC is single-threaded

m256i QPI::QpiContextFunctionCall::computeMiningFunction(const m256i miningSeed, const m256i publicKey, const m256i nonce) const
{
    // Score's currentRandomSeed is initialized to zero by setMem(score_qpi, sizeof(*score_qpi), 0)
    // If the mining seed changes, we must reinitialize it
    if (miningSeed != score_qpi->currentRandomSeed)
    {
        score_qpi->initMiningData(miningSeed);
    }
    (*score_qpi)(0, publicKey, miningSeed, nonce);
    return score_qpi->getLastOutput(0);
}
