#pragma once

#include "network_messages/tick.h"

#include "platform/memory.h"
#include "platform/concurrency.h"
#include "platform/console_logging.h"

#include "public_settings.h"



// Encapsulated tick storage of current epoch that can additionally keep the last ticks of the previous epoch.
// This is a kind of singleton class with only static members (so all instances refer to the same data).
class TickStorage
{
private:
    static constexpr unsigned long long tickDataLength = MAX_NUMBER_OF_TICKS_PER_EPOCH + TICKS_TO_KEEP_FROM_PRIOR_EPOCH;
    static constexpr unsigned long long tickDataSize = tickDataLength * sizeof(TickData);
    
    // Allocated tick data buffer with tickDataLength elements (includes current and previous epoch data)
    inline static TickData* tickDataPtr = nullptr;
    inline static volatile char tickDataLock = 0;
    inline static unsigned int tickBegin = 0;
    inline static unsigned int tickEnd = 0;

    // Data of previous epoch. Points to tickData + MAX_NUMBER_OF_TICKS_PER_EPOCH
    inline static TickData* oldTickDataPtr = nullptr;
    inline static unsigned int oldTickBegin = 0;
    inline static unsigned int oldTickEnd = 0;

public:

    // Init at node startup
    static bool init()
    {
        EFI_STATUS status;
        if ((status = bs->AllocatePool(EfiRuntimeServicesData, tickDataSize, (void**)&tickDataPtr)))
        {
            logStatusToConsole(L"EFI_BOOT_SERVICES.AllocatePool() fails", status, __LINE__);

            return false;
        }

        oldTickDataPtr = tickDataPtr + MAX_NUMBER_OF_TICKS_PER_EPOCH;
        return true;
    }

    // Cleanup at node shutdown
    static void deinit()
    {
        if (tickDataPtr)
        {
            bs->FreePool(tickDataPtr);
        }
    }

    // Begin new epoch. If not called the first time (seamless transition), assume that the ticks to keep
    // are ticks in [newInitialTick-TICKS_TO_KEEP_FROM_PRIOR_EPOCH, newInitialTick-1].
    static void beginEpoch(unsigned int newInitialTick)
    {
        if (tickBegin && tickInCurrentEpochStorage(newInitialTick))
        {
            // seamless epoch transition: keep some ticks of prior epoch
            oldTickEnd = newInitialTick;
            oldTickBegin = newInitialTick - TICKS_TO_KEEP_FROM_PRIOR_EPOCH;
            if (oldTickBegin < tickBegin)
                oldTickBegin = tickBegin;

            copyMem(oldTickDataPtr, tickDataPtr + tickToIndexCurrentEpoch(oldTickBegin), (oldTickEnd - oldTickBegin) * sizeof(TickData));
            setMem(tickDataPtr, MAX_NUMBER_OF_TICKS_PER_EPOCH * sizeof(TickData), 0);

            tickEnd = newInitialTick + MAX_NUMBER_OF_TICKS_PER_EPOCH;
        }
        else
        {
            // node startup with no data of prior epoch (also use storage for prior epoch for current)
            setMem(tickDataPtr, tickDataSize, 0);
            oldTickBegin = 0;
            oldTickEnd = 0;
            tickEnd = newInitialTick + MAX_NUMBER_OF_TICKS_PER_EPOCH; // +TICKS_TO_KEEP_FROM_PRIOR_EPOCH; // to add this, also include other tick buffer
        }
        
        tickBegin = newInitialTick;
    }

    // Check whether tick is stored in the current epoch storage.
    inline static bool tickInCurrentEpochStorage(unsigned int tick)
    {
        return tick >= tickBegin && tick < tickEnd;
    }

    // Check whether tick is stored in the previous epoch storage.
    inline static bool tickInPreviousEpochStorage(unsigned int tick)
    {
        return oldTickBegin <= tick && tick < oldTickEnd;
    }

    // Return index of tick data in current epoch (does not check tick).
    inline static unsigned int tickToIndexCurrentEpoch(unsigned int tick)
    {
        return tick - tickBegin;
    }

    // Return index of tick data in previous epoch (does not check that it is stored).
    inline static unsigned int tickToIndexPreviousEpoch(unsigned int tick)
    {
        return tick - oldTickBegin + MAX_NUMBER_OF_TICKS_PER_EPOCH;
    }

    // Struct for structured, convenient access via ".tickData"
    struct
    {
        inline static void acquireLock()
        {
            ACQUIRE(tickDataLock);
        }

        inline static void releaseLock()
        {
            RELEASE(tickDataLock);
        }

        // Return tick if it is stored and not empty (checks tick).
        TickData* getByTickIfNotEmpty(unsigned int tick)
        {
            unsigned int index;
            if (tickInCurrentEpochStorage(tick))
                index = tickToIndexCurrentEpoch(tick);
            else if (tickInPreviousEpochStorage(tick))
                index = tickToIndexPreviousEpoch(tick);
            else
                return nullptr;

            TickData* td = tickDataPtr + index;
            if (td->epoch == 0)
                return nullptr;

            return td;
        }

        // Get tick data by tick (not checking tick)
        TickData& getByTickInCurrentEpoch(unsigned int tick)
        {
            return tickDataPtr[tickToIndexCurrentEpoch(tick)];
        }

        // Get tick data at index (not checking index)
        TickData& operator[](unsigned int index)
        {
            return tickDataPtr[index];
        }

        // Get tick data at index (not checking index)
        const TickData& operator[](unsigned int index) const
        {
            return tickDataPtr[index];
        }
    } tickData;

};
