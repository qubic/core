#pragma once

#include "qpi/qpi.h"
#include "score.h"

static ScoreFunction<1>* score_qpi = nullptr; // NOTE: SC is single-threaded

m256i QPI::QpiContextFunctionCall::computeMiningFunction(const m256i miningSeed, const m256i publicKey, const m256i nonce) const
{
    // Score's currentRandomSeed is initialized to zero by setMem(score_qpi, sizeof(*score_qpi), 0)
    // If the mining seed changes, we must reinitialize it
    if (miningSeed != score_qpi->currentRandomSeed)
    {
        score_qpi->initMiningData(miningSeed);
    }
    m256i bpp9000Nonce = nonce;
    // Only the active odd-nonce (bpp9000) slot supports the last output
    bpp9000Nonce.m256i_u8[0] = (bpp9000Nonce.m256i_u8[0] | 0x01);
    ASSERT((bpp9000Nonce.m256i_u8[0] & 1) == 1);
    (*score_qpi)(0, publicKey, miningSeed, bpp9000Nonce);
    return score_qpi->getLastOutput(0);
}
