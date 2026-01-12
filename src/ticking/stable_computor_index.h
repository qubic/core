#pragma once

#include "platform/m256.h"
#include "platform/memory.h"
#include "public_settings.h"

// Minimum buffer size: NUMBER_OF_COMPUTORS * sizeof(m256i) + 2 * NUMBER_OF_COMPUTORS bytes (~23KB)
constexpr unsigned long long stableComputorIndexBufferSize()
{
    return NUMBER_OF_COMPUTORS * sizeof(m256i) + 2 * NUMBER_OF_COMPUTORS;
}

// Reorders futureComputors so requalifying computors keep their current index.
// New computors fill remaining slots. See doc/stable_computor_index_diagram.svg
// Returns false if there aren't enough computors to fill all slots.
static bool calculateStableComputorIndex(
    m256i* futureComputors,
    const m256i* currentComputors,
    void* tempBuffer)
{
    m256i* tempComputorList = (m256i*)tempBuffer;
    bool* isIndexTaken = (bool*)(tempComputorList + NUMBER_OF_COMPUTORS);
    bool* isFutureComputorUsed = isIndexTaken + NUMBER_OF_COMPUTORS;

    setMem(tempComputorList, NUMBER_OF_COMPUTORS * sizeof(m256i), 0);
    setMem(isIndexTaken, NUMBER_OF_COMPUTORS, 0);
    setMem(isFutureComputorUsed, NUMBER_OF_COMPUTORS, 0);

    // Step 1: Requalifying computors keep their current index
    for (unsigned int futureIdx = 0; futureIdx < NUMBER_OF_COMPUTORS; futureIdx++)
    {
        for (unsigned int currentIdx = 0; currentIdx < NUMBER_OF_COMPUTORS; currentIdx++)
        {
            if (futureComputors[futureIdx] == currentComputors[currentIdx])
            {
                tempComputorList[currentIdx] = futureComputors[futureIdx];
                isIndexTaken[currentIdx] = true;
                isFutureComputorUsed[futureIdx] = true;
                break;
            }
        }
    }

    // Step 2: New computors fill remaining slots
    unsigned int nextNewComputorIdx = 0;
    for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
    {
        if (!isIndexTaken[i])
        {
            while (nextNewComputorIdx < NUMBER_OF_COMPUTORS && isFutureComputorUsed[nextNewComputorIdx])
            {
                nextNewComputorIdx++;
            }

            if (nextNewComputorIdx >= NUMBER_OF_COMPUTORS)
            {
                return false;
            }

            tempComputorList[i] = futureComputors[nextNewComputorIdx];
            isFutureComputorUsed[nextNewComputorIdx] = true;
            nextNewComputorIdx++;
        }
    }

    copyMem(futureComputors, tempComputorList, NUMBER_OF_COMPUTORS * sizeof(m256i));

    return true;
}
