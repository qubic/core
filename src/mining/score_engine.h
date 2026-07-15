#pragma once


#include "score_bpp9000.h"

namespace score_engine
{

template<typename NeuraxonParamsT, typename Bpp9000ParamsT>
struct ScoreEngine
{
    ScoreBpp9000<Bpp9000ParamsT> _bpp9000Score;
    unsigned char lastNonceByte0;

    void initMemory()
    {
        setMem(&_bpp9000Score, sizeof(ScoreBpp9000<Bpp9000ParamsT>), 0);

        _bpp9000Score.initMemory();
    }

    // Unused function
    void initMiningData(const unsigned char* randomPool)
    {

    }

    // Load the task blocks into the active bpp9000 leaf; returns false on invalid topology/data.
    bool loadTask(const unsigned char* topoBlock, const unsigned char* dataBlock)
    {
        return _bpp9000Score.loadTaskFromMemory(topoBlock, dataBlock);
    }

    unsigned int computeNeuraxonScore(const unsigned char* publicKey, const unsigned char* nonce, const unsigned char* randomPool)
    {
        // Neuraxon is a reserved placeholder - never executed, never yields a valid score.
        return INVALID_SCORE_VALUE;
    }

    unsigned int computeBpp9000Score(const unsigned char* publicKey, const unsigned char* nonce, const unsigned char* randomPool)
    {
        // The score IS the error count - smaller is better. A timeout maps to the worst in-range value
        // rather than INVALID_SCORE_VALUE, which the score cache cannot store (it reads back as a miss).
        const unsigned int failures = _bpp9000Score.computeScore(publicKey, nonce, randomPool);
        return (failures == ScoreBpp9000<Bpp9000ParamsT>::INFINITE_ERROR)
            ? (unsigned int)ScoreBpp9000<Bpp9000ParamsT>::numberOfWindows
            : failures;
    }

    unsigned int computeScore(const unsigned char* publicKey, const unsigned char* nonce, const unsigned char* randomPool)
    {
        lastNonceByte0 = nonce[0];
        if ((nonce[0] & 1) == 0)
        {
            return computeNeuraxonScore(publicKey, nonce, randomPool);
        }
        else
        {
            return computeBpp9000Score(publicKey, nonce, randomPool);
        }
    }

    // returns last computed output neurons of the active odd-nonce (bpp9000) slot
    m256i getLastOutput()
    {
        m256i result;
        result = m256i::zero();
        if ((lastNonceByte0 & 1) != 0)
        {
            _bpp9000Score.getLastOutput(result.m256i_u8, 32);
        }
        return result;
    }

};

}
