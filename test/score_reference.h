#pragma once

#include "../src/platform/memory_util.h"
#include "../src/four_q.h"
#include "score_hyperidentity_reference.h"
#include "score_addition_reference.h"
#include <vector>

////////// Original (reference) scoring algorithm \\\\\\\\\\

namespace score_reference
{

template<typename HyperIdentityParamsT, typename AdditionParamsT>
struct ScoreReferenceImplementation
{
    using HyperIdentityScore = ::score_hyberidentity_reference::Miner<HyperIdentityParamsT>;
    using AdditionScore = ::score_addition_reference::Miner<AdditionParamsT>;

    std::unique_ptr<HyperIdentityScore> _hyperIdentityScore;
    std::unique_ptr<AdditionScore> _additionScore;

    void initMemory()
    {
        _hyperIdentityScore = std::make_unique<HyperIdentityScore>();
        _additionScore = std::make_unique<AdditionScore>();
    }
    void initMiningData(const unsigned char* miningSeed)
    {
        _hyperIdentityScore->initialize(miningSeed);
        _additionScore->initialize(miningSeed);
    }

    unsigned int computeHyperIdentityScore(const unsigned char* publicKey, const unsigned char* nonce, const unsigned char* randomPool = nullptr)
    {
        return _hyperIdentityScore->computeScore(publicKey, nonce);
    }

    unsigned int computeAdditionScore(const unsigned char* publicKey, const unsigned char* nonce, const unsigned char* randomPool = nullptr)
    {
        return _additionScore->computeScore(publicKey, nonce);
    }

    // Return score depend on the nonce
    unsigned int computeScore(const unsigned char* publicKey, const unsigned char* nonce, const unsigned char* randomPool = nullptr)
    {
        if ((nonce[0] & 1) == 0)
        {
            return computeHyperIdentityScore(publicKey, nonce);
        }
        else
        {
            return computeAdditionScore(publicKey, nonce);
        }
        return 0;
    }

};

}
