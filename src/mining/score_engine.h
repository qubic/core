#pragma once


#include "score_hyperidentity.h"
#include "score_addition.h"

namespace score_engine
{

template<typename HyperIdentityParamsT, typename AdditionParamsT>
struct ScoreEngine
{
    ScoreHyperIdentity<HyperIdentityParamsT> _hyperIdentityScore;
    ScoreAddition<AdditionParamsT> _additionScore;
    unsigned char lastNonceByte0;

    void initMemory()
    {
        setMem(&_hyperIdentityScore, sizeof(ScoreHyperIdentity<HyperIdentityParamsT>), 0);
        setMem(&_additionScore, sizeof(ScoreAddition<AdditionParamsT>), 0);

        _hyperIdentityScore.initMemory();
        _additionScore.initMemory();
    }

    // Unused function
    void initMiningData(const unsigned char* randomPool)
    {

    }

    unsigned int computeHyperIdentityScore(const unsigned char* publicKey, const unsigned char* nonce, const unsigned char* randomPool)
    {
        return _hyperIdentityScore.computeScore(publicKey, nonce, randomPool);
    }

    unsigned int computeAdditionScore(const unsigned char* publicKey, const unsigned char* nonce, const unsigned char* randomPool)
    {
        return _additionScore.computeScore(publicKey, nonce, randomPool);
    }

    unsigned int computeScore(const unsigned char* publicKey, const unsigned char* nonce, const unsigned char* randomPool)
    {
        lastNonceByte0 = nonce[0];
        if ((nonce[0] & 1) == 0)
        {
            return computeHyperIdentityScore(publicKey, nonce, randomPool);
        }
        else
        {
            return computeAdditionScore(publicKey, nonce, randomPool);
        }
    }

    // returns last computed output neurons, only returns 256 non-zero neurons, neuron values are compressed to bit
    m256i getLastOutput()
    {
        // Only hyperidentity score support
        m256i result;
        result = m256i::zero();
        if ((lastNonceByte0 & 1) == 0)
        {
            _hyperIdentityScore.getLastOutput(result.m256i_u8, 32);
        }
        return result;
    }

};

}
