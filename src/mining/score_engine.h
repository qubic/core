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

    void initMemory()
    {
        setMem(&_hyperIdentityScore, sizeof(ScoreHyperIdentity<HyperIdentityParamsT>), 0);
    }

    // Unused function
    void initMiningData(unsigned char* randomPool)
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
        // TODO: switch algorithm depend on nonce
        //return computeHyperIdentityScore(publicKey, nonce, randomPool);

        if ((nonce[0] & 1) == 0)
        {
            return computeHyperIdentityScore(publicKey, nonce, randomPool);
        }
        else
        {
            return computeAdditionScore(publicKey, nonce, randomPool);
        }
        return 0;
    }

    // returns last computed output neurons, only returns 256 non-zero neurons, neuron values are compressed to bit
    // TODO: support later
    m256i getLastOutput()
    {
        m256i result;
        result = m256i::zero();

        //unsigned long long population = bestANN.population;
        //Neuron* neurons = bestANN.neurons;
        //NeuronType* neuronTypes = bestANN.neuronTypes;
        //int count = 0;
        //int byteCount = 0;
        //uint8_t A = 0;
        //for (unsigned long long i = 0; i < population; i++)
        //{
        //    if (neuronTypes[i] == OUTPUT_NEURON_TYPE)
        //    {
        //        if (neurons[i])
        //        {
        //            uint8_t v = (neurons[i] > 0);
        //            v = v << (7 - count);
        //            A |= v;
        //            if (++count == 8)
        //            {
        //                result.m256i_u8[byteCount++] = A;
        //                A = 0;
        //                count = 0;
        //                if (byteCount >= 32)
        //                {
        //                    break;
        //                }
        //            }
        //        }
        //    }
        //}

        return result;
    }

};

}
