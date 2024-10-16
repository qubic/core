#pragma once

#include "platform/global_var.h"
#include "platform/memory.h"

#include "network_messages/entity.h"
#include "network_messages/assets.h"


constexpr unsigned long long spectrumSizeInBytes = SPECTRUM_CAPACITY * sizeof(::Entity);
constexpr unsigned long long universeSizeInBytes = ASSETS_CAPACITY * sizeof(Asset);

// Buffer used for reorganizing spectrum and universe hash maps, currently also used as scratchpad buffer for contracts
// Must be large enough to fit any contract, full spectrum, and full universe!
GLOBAL_VAR_DECL void* reorgBuffer GLOBAL_VAR_INIT(nullptr);

static bool initCommonBuffers()
{
    // TODO: check that max contract state size does not exceed size of spectrum or universe
    constexpr unsigned long long reorgBufferSize = (spectrumSizeInBytes >= universeSizeInBytes) ? spectrumSizeInBytes : universeSizeInBytes;
    if (!allocatePool(reorgBufferSize, (void**)&reorgBuffer))
    {
        logToConsole(L"Failed to allocate common buffers!");
        return false;
    }

    return true;
}

static void deinitCommonBuffers()
{
    if (reorgBuffer)
    {
        freePool(reorgBuffer);
        reorgBuffer = nullptr;
    }
}

static void* __scratchpad()
{
    return reorgBuffer;
}
