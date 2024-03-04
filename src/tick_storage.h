#pragma once

#include "network_messages/tick.h"

#include "platform/memory.h"
#include "platform/concurrency.h"
#include "platform/console_logging.h"

#include "public_settings.h"



// Encapsulated tick storage of current epoch that can additionally keep the last ticks of the previous epoch.
// This is a kind of singleton class with only static members (so all instances refer to the same data).
// It comprises:
// - tickData (one TickData struct per tick)
// - ticks (one Tick struct per tick and Computor)
class TickStorage
{
private:
    static constexpr unsigned long long tickDataLength = MAX_NUMBER_OF_TICKS_PER_EPOCH + TICKS_TO_KEEP_FROM_PRIOR_EPOCH;
    static constexpr unsigned long long tickDataSize = tickDataLength * sizeof(TickData);
    
    static constexpr unsigned long long ticksLengthCurrentEpoch = ((unsigned long long)MAX_NUMBER_OF_TICKS_PER_EPOCH) * NUMBER_OF_COMPUTORS;
    static constexpr unsigned long long ticksLengthPreviousEpoch = ((unsigned long long)TICKS_TO_KEEP_FROM_PRIOR_EPOCH) * NUMBER_OF_COMPUTORS;
    static constexpr unsigned long long ticksLength = ticksLengthCurrentEpoch + ticksLengthPreviousEpoch;
    static constexpr unsigned long long ticksSize = ticksLength * sizeof(Tick);


    // Tick number range of current epoch storage
    inline static unsigned int tickBegin = 0;
    inline static unsigned int tickEnd = 0;

    // Tick number range of previous epoch storage
    inline static unsigned int oldTickBegin = 0;
    inline static unsigned int oldTickEnd = 0;

    // Allocated tick data buffer with tickDataLength elements (includes current and previous epoch data)
    inline static TickData* tickDataPtr = nullptr;

    // Allocated ticks buffer with ticksLength elements (includes current and previous epoch data)
    inline static Tick* ticksPtr = nullptr;

    // Tick data of previous epoch. Points to tickData + MAX_NUMBER_OF_TICKS_PER_EPOCH
    inline static TickData* oldTickDataPtr = nullptr;

    // Ticks of previous epoch. Points to ticksPtr + ticksLengthCurrentEpoch
    inline static Tick* oldTicksPtr = nullptr;

    // Lock for securing tickData
    inline static volatile char tickDataLock = 0;

    // One lock per computor for securing ticks element in current tick (only the tick system.tick is written)
    inline static volatile char ticksLocks[NUMBER_OF_COMPUTORS];


public:

    // Init at node startup
    static bool init()
    {
        // TODO: allocate everything with one continuous buffer
        EFI_STATUS status;
        if ((status = bs->AllocatePool(EfiRuntimeServicesData, tickDataSize, (void**)&tickDataPtr)))
        {
            logStatusToConsole(L"EFI_BOOT_SERVICES.AllocatePool() fails", status, __LINE__);

            return false;
        }
        if (status = bs->AllocatePool(EfiRuntimeServicesData, ticksSize, (void**)&ticksPtr))
        {
            logStatusToConsole(L"EFI_BOOT_SERVICES.AllocatePool() fails", status, __LINE__);

            return false;
        }

        setMem((void*)ticksLocks, sizeof(ticksLocks), 0);

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

        if (ticksPtr)
        {
            bs->FreePool(ticksPtr);
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

            const unsigned int tickIndex = tickToIndexCurrentEpoch(oldTickBegin);
            const unsigned int tickCount = oldTickEnd - oldTickBegin;

            copyMem(oldTickDataPtr, tickDataPtr + tickIndex, tickCount * sizeof(TickData));
            setMem(tickDataPtr, MAX_NUMBER_OF_TICKS_PER_EPOCH * sizeof(TickData), 0);

            copyMem(oldTicksPtr, ticksPtr + (tickIndex * NUMBER_OF_COMPUTORS), tickCount * NUMBER_OF_COMPUTORS * sizeof(Tick));
            setMem(ticksPtr, ticksLengthCurrentEpoch * sizeof(Tick), 0);

            tickEnd = newInitialTick + MAX_NUMBER_OF_TICKS_PER_EPOCH;
        }
        else
        {
            // node startup with no data of prior epoch (also use storage for prior epoch for current)
            setMem(tickDataPtr, tickDataSize, 0);
            setMem(ticksPtr, ticksSize, 0);
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

    // Struct for structured, convenient access via ".ticks"
    struct
    {
        // Acquire lock for ticks element of specific computor (only ticks >= system.tick are written)
        inline static void acquireLock(unsigned short computorIndex)
        {
            ACQUIRE(ticksLocks[computorIndex]);
        }

        // Release lock for ticks element of specific computor (only ticks >= system.tick are written)
        inline static void releaseLock(unsigned short computorIndex)
        {
            RELEASE(ticksLocks[computorIndex]);
        }

        // Return pointer to array of one Tick per computor in current epoch (not checking tick)
        Tick* getComputorsTicksInCurrentEpoch(unsigned int tick)
        {
            return ticksPtr + tickToIndexCurrentEpoch(tick) * NUMBER_OF_COMPUTORS;
        }

        // Return pointer to array of one Tick per computor in previous epoch (not checking tick)
        Tick* getComputorsTicksInPreviousEpoch(unsigned int tick)
        {
            return ticksPtr + tickToIndexPreviousEpoch(tick) * NUMBER_OF_COMPUTORS;
        }

        // Get ticks element at offset (not checking offset)
        Tick& operator[](unsigned int offset)
        {
            return ticksPtr[offset];
        }

        // Get ticks element at offset (not checking offset)
        const Tick& operator[](unsigned int offset) const
        {
            return ticksPtr[offset];
        }
    } ticks;
};
