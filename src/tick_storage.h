#pragma once

#include "network_messages/tick.h"

#include "platform/memory.h"
#include "platform/concurrency.h"
#include "platform/console_logging.h"

#include "public_settings.h"



// Tick data storage of current epoch that can additionally keep the last ticks of the previous epoch.
// CAUTION: Constructor or memset zero needs to be called to initialize properly.
class TickDataStorage
{
private:
    static constexpr unsigned long long tickDataLength = MAX_NUMBER_OF_TICKS_PER_EPOCH + TICKS_TO_KEEP_FROM_PRIOR_EPOCH;
    static constexpr unsigned long long tickDataSize = tickDataLength * sizeof(TickData);
    
    // Allocated tick data buffer with tickDataLength elements (includes current and previous epoch data)
    TickData* tickData = nullptr;
    volatile char tickDataLock = 0;
    unsigned int tickBegin = 0;
    unsigned int tickEnd = 0;

    // Data of previous epoch. Points to tickData + MAX_NUMBER_OF_TICKS_PER_EPOCH
    TickData* oldTickData = nullptr;
    unsigned int oldTickBegin = 0;
    unsigned int oldTickEnd = 0;

public:
    // Init at node startup
    bool init()
    {
        EFI_STATUS status;
        if ((status = bs->AllocatePool(EfiRuntimeServicesData, tickDataSize, (void**)&tickData)))
        {
            logStatusToConsole(L"EFI_BOOT_SERVICES.AllocatePool() fails", status, __LINE__);

            return false;
        }

        oldTickData = tickData + MAX_NUMBER_OF_TICKS_PER_EPOCH;
        return true;
    }

    // Cleanup at node shutdown
    void deinit()
    {
        if (tickData)
        {
            bs->FreePool(tickData);
        }
    }

    // Begin new epoch. If not called the first time (seamless transition), assume that the ticks to keep
    // are ticks in [newInitialTick-TICKS_TO_KEEP_FROM_PRIOR_EPOCH, newInitialTick-1].
    void beginEpoch(unsigned int newInitialTick)
    {
        if (tickBegin && tickInCurrentEpochStorage(newInitialTick))
        {
            // seamless epoch transition: keep some ticks of prior epoch
            oldTickEnd = newInitialTick;
            oldTickBegin = newInitialTick - TICKS_TO_KEEP_FROM_PRIOR_EPOCH;
            if (oldTickBegin < tickBegin)
                oldTickBegin = tickBegin;

            copyMem(oldTickData, tickData + tickToIndexCurrentEpoch(oldTickBegin), (oldTickEnd - oldTickBegin) * sizeof(TickData));
            setMem(tickData, MAX_NUMBER_OF_TICKS_PER_EPOCH * sizeof(TickData), 0);

            tickEnd = newInitialTick + MAX_NUMBER_OF_TICKS_PER_EPOCH;
        }
        else
        {
            // node startup with no data of prior epoch (also use storage for prior epoch for current)
            setMem(tickData, tickDataSize, 0);
            oldTickBegin = 0;
            oldTickEnd = 0;
            tickEnd = newInitialTick + MAX_NUMBER_OF_TICKS_PER_EPOCH; // +TICKS_TO_KEEP_FROM_PRIOR_EPOCH; // to add this, also include other tick buffer
        }
        
        tickBegin = newInitialTick;
    }

    inline void acquireLock()
    {
        ACQUIRE(tickDataLock);
    }

    inline void releaseLock()
    {
        RELEASE(tickDataLock);
    }

    // Check whether tick is stored in the current epoch storage.
    inline bool tickInCurrentEpochStorage(unsigned int tick) const
    {
        return tick >= tickBegin && tick < tickEnd;
    }

    // Check whether tick is stored in the previous epoch storage.
    inline bool tickInPreviousEpochStorage(unsigned int tick) const
    {
        return oldTickBegin <= tick && tick < oldTickEnd;
    }

    // Return index of tick data in current epoch (does not check tick).
    inline unsigned int tickToIndexCurrentEpoch(unsigned int tick) const
    {
        return tick - tickBegin;
    }

    // Return index of tick data in previous epoch (does not check that it is stored).
    inline unsigned int tickToIndexPreviousEpoch(unsigned int tick) const
    {
        return tick - oldTickBegin + MAX_NUMBER_OF_TICKS_PER_EPOCH;
    }

    // Return tick if it is stored and not empty (checks tick).
    TickData * getIfNotEmpty(unsigned int tick)
    {
        unsigned int index;
        if (tickInCurrentEpochStorage(tick))
            index = tickToIndexCurrentEpoch(tick);
        else if (tickInPreviousEpochStorage(tick))
            index = tickToIndexPreviousEpoch(tick);
        else
            return nullptr;

        TickData * td = tickData + index;
        if (td->epoch == 0)
            return nullptr;

        return td;
    }

    // Get tick data by tick (not checking index)
    TickData & getByTickInCurrentEpoch(unsigned int tick)
    {
        return tickData[tickToIndexCurrentEpoch(tick)];
    }

    // Get tick data at index (not checking index)
    TickData & operator[](unsigned int index)
    {
        return tickData[index];
    }

    // Get tick data at index (not checking index)
    const TickData & operator[](unsigned int index) const
    {
        return tickData[index];
    }
};