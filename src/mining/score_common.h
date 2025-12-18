#pragma once

namespace score_engine
{

enum AlgoType
{
    HyperIdentity = 1 << 0,
    Addition = 1 << 1,
    AllAlgo = 1 << 2
};

// =============================================================================
// Algorithm 0: HyperIdentity Parameters
// =============================================================================
template<
    unsigned long long inputNeurons,   // numberOfInputNeurons
    unsigned long long outputNeurons,   // numberOfOutputNeurons
    unsigned long long ticks,   // numberOfTicks
    unsigned long long neighbor,  // numberOfNeighbors
    unsigned long long population,   // populationThreshold
    unsigned long long mutations,   // numberOfMutations
    unsigned int threshold          // solutionThreshold
>
struct HyperIdentityParams
{
    static constexpr unsigned long long numberOfInputNeurons = inputNeurons;
    static constexpr unsigned long long numberOfOutputNeurons = outputNeurons;
    static constexpr unsigned long long numberOfTicks = ticks;
    static constexpr unsigned long long numberOfNeighbors = neighbor;
    static constexpr unsigned long long populationThreshold = population;
    static constexpr unsigned long long numberOfMutations = mutations;
    static constexpr unsigned int solutionThreshold = threshold;

    static constexpr AlgoType algoType = AlgoType::HyperIdentity;
    static constexpr unsigned int paramsCount = 7;
};

// =============================================================================
// Algorithm 1: Addition Parameters
// =============================================================================
template<
    unsigned long long inputNeurons,   // numberOfInputNeurons
    unsigned long long outputNeurons,   // numberOfOutputNeurons
    unsigned long long ticks,   // numberOfTicks
    unsigned long long neighbor,  // maxNumberOfNeigbor
    unsigned long long population,   // populationThreshold
    unsigned long long mutations,   // numberOfMutations
    unsigned int threshold          // solutionThreshold
>
struct AdditionParams
{
    static constexpr unsigned long long numberOfInputNeurons = inputNeurons;
    static constexpr unsigned long long numberOfOutputNeurons = outputNeurons;
    static constexpr unsigned long long numberOfTicks = ticks;
    static constexpr unsigned long long numberOfNeighbors = neighbor;
    static constexpr unsigned long long populationThreshold = population;
    static constexpr unsigned long long numberOfMutations = mutations;
    static constexpr unsigned int solutionThreshold = threshold;

    static constexpr AlgoType algoType = AlgoType::Addition;
    static constexpr unsigned int paramsCount = 7;
};

}
