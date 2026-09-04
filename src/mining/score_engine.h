#pragma once


#include "score_bpp9000.h"

namespace score_engine
{

template<typename NeuraxonParamsT, typename Bpp9000ParamsT>
struct ScoreEngine
{
    ScoreBpp9000<Bpp9000ParamsT> _bpp9000Score;
    unsigned char lastNonceByte0;

    // The inheritable per-neuron LUT the ant colony branches on.
    using AntAnn = typename ScoreBpp9000<Bpp9000ParamsT>::ANN;

    void initMemory()
    {
        setMem(&_bpp9000Score, sizeof(ScoreBpp9000<Bpp9000ParamsT>), 0);

        _bpp9000Score.initMemory();
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
        const unsigned int failures = _bpp9000Score.computeScore(publicKey, nonce, randomPool);
        return (failures == ScoreBpp9000<Bpp9000ParamsT>::INFINITE_ERROR)
            ? (unsigned int)ScoreBpp9000<Bpp9000ParamsT>::numberOfWindows
            : failures;
    }

    unsigned int computeScore(const unsigned char* publicKey, const unsigned char* nonce, const unsigned char* randomPool)
    {
        lastNonceByte0 = nonce[0];
        switch (getAlgoType(nonce))
        {
        case AlgoType::Bpp9000:
            return computeBpp9000Score(publicKey, nonce, randomPool);
        case AlgoType::Neuraxon:
            return computeNeuraxonScore(publicKey, nonce, randomPool);
        default:
            return INVALID_SCORE_VALUE;
        }
    }

    // Each engine owns its canonical standalone-nonce rule
    static bool isCanonicalStandaloneNonce(const unsigned char* nonce)
    {
        switch (getAlgoType(nonce))
        {
        case AlgoType::Bpp9000:
            return ScoreBpp9000<Bpp9000ParamsT>::isCanonicalStandaloneNonce(nonce);
        default:
            return false;
        }
    }

    // Each engine owns its canonical ant-nonce rule
    static bool isCanonicalAntNonce(const unsigned char* nonce)
    {
        switch (getAlgoType(nonce))
        {
        case AlgoType::Bpp9000:
            return ScoreBpp9000<Bpp9000ParamsT>::isCanonicalAntNonce(nonce);
        default:
            return false;
        }
    }

    // Ant colony: the shared per-epoch network every identity's tree starts from; rootSeed is the
    // epoch-start spectrum digest
    void deriveAntRootANN(const unsigned char* rootSeed, const unsigned char* randomPool, AntAnn& out)
    {
        _bpp9000Score.deriveRootANN(rootSeed, randomPool, out);
    }

    // Ant colony: score a child by inheriting the parent's network and walking it with the child's
    // own seeds. Returns INVALID_SCORE_VALUE for a non-canonical nonce or an unsupported algorithm.
    unsigned int computeAntScoreFromParent(const AntAnn& parent, const unsigned char* publicKey,
        const unsigned char* nonce, const unsigned char* anchorDigest, const unsigned char* randomPool)
    {
        switch (getAlgoType(nonce))
        {
            case AlgoType::Bpp9000:
                return _bpp9000Score.computeScoreFromParent(parent, publicKey, nonce, anchorDigest, randomPool);
            default:
                return INVALID_SCORE_VALUE;
        }
    }

    // Ant colony: the network that produced the score the walk returned.
    void getAntBestANN(AntAnn& out)
    {
        _bpp9000Score.getBestANN(out);
    }

    // returns last computed output neurons of the active bpp9000 slot
    m256i getLastOutput()
    {
        m256i result;
        result = m256i::zero();
        if (lastNonceByte0 == AlgoType::Bpp9000)
        {
            _bpp9000Score.getLastOutput(result.m256i_u8, 32);
        }
        return result;
    }

};

}
