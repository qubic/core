#pragma once

#include "network_messages/transactions.h"

#include "platform/memory.h"
#include "platform/concurrency.h"
#include "platform/console_logging.h"

#include "public_settings.h"

// Mempool that saves pending transactions (txs) of all entities.
// This is a kind of singleton class with only static members (so all instances refer to the same data).
class TxsPool
{
private:
	static constexpr unsigned long long maxNumTicksPerEpoch = MAX_NUMBER_OF_TICKS_PER_EPOCH;
	static constexpr unsigned long long maxNumTicksToSave = maxNumTicksPerEpoch + TICKS_TO_KEEP_FROM_PRIOR_EPOCH;
	static constexpr unsigned long long maxNumTxsToSave = maxNumTicksToSave * NUMBER_OF_TRANSACTIONS_PER_TICK;

	inline static volatile char lockTxsAndDigests = 0;
	inline static volatile char lockNumSaved = 0;

	inline static Transaction* transactions = nullptr;
	inline static m256i* digests = nullptr;

	inline static unsigned int numSavedTxsPerTick[maxNumTicksToSave];

	inline static unsigned int firstSavedTick = 0;

public:
	// Init at node startup.
	static bool init()
	{
		if (!allocatePool(maxNumTxsToSave * MAX_TRANSACTION_SIZE, (void**)&transactions)
			|| !allocatePool(maxNumTxsToSave * 32ULL, (void**)&digests))
		{
			logToConsole(L"Failed to allocate transaction pool memory!");
			return false;
		}

		ASSERT(lockTxsAndDigests == 0);
		ASSERT(lockNumSaved == 0);
		setMem((void*)numSavedTxsPerTick, sizeof(numSavedTxsPerTick), 0);

		return true;
	}

	// Cleanup at node shutdown.
	static void deinit()
	{
		if (transactions)
		{
			freePool(transactions);
		}
		if (digests)
		{
			freePool(digests);
		}
	}

	// Acquire lock for returned pointers to transactions or digests.
	inline static void acquireLock()
	{
		ACQUIRE(lockTxsAndDigests);
	}

	// Release lock for returned pointers to transactions or digests.
	inline static void releaseLock()
	{
		RELEASE(lockTxsAndDigests);
	}

	// Return number of transactions scheduled later than the specified tick.
	static unsigned int getNumberOfPendingTxs(unsigned int tick)
	{
		unsigned int res = 0;
		ACQUIRE(lockNumSaved);
		unsigned int startTickIndex = tick < firstSavedTick ? 0 : tick - firstSavedTick + 1;
		for (unsigned int index = startTickIndex; index < maxNumTicksToSave; ++index)
		{
			res += numSavedTxsPerTick[index];
		}
		RELEASE(lockNumSaved);
		return res;
	}

	// Check validity of transaction and add to the pool.
	static void update(Transaction* tx)
	{
		// A node doesn't need to store a transaction that is scheduled on a tick that node will never reach.
		// Notice: MAX_NUMBER_OF_TICKS_PER_EPOCH is not set globally since every node may have different TARGET_TICK_DURATION time due to memory limitation.
		if (firstSavedTick <= tx->tick && tx->tick < firstSavedTick + maxNumTicksToSave)
		{
			unsigned int tickIndex = tx->tick - firstSavedTick;

			ACQUIRE(lockTxsAndDigests);
			ACQUIRE(lockNumSaved);
			if (numSavedTxsPerTick[tickIndex] < NUMBER_OF_TRANSACTIONS_PER_TICK)
			{
				const unsigned int transactionSize = tx->totalSize();
				copyMem(&transactions[tickIndex * NUMBER_OF_TRANSACTIONS_PER_TICK + numSavedTxsPerTick[tickIndex]], tx, transactionSize);
				KangarooTwelve(tx, transactionSize, &digests[tickIndex], 32);
				numSavedTxsPerTick[tickIndex]++;
			}
			// TODO: add a strategy to potentially overwrite existing transactions if max number of transations for tick has been reached. 

			RELEASE(lockNumSaved);
			RELEASE(lockTxsAndDigests);
		}
	}

	// Get a transaction for the specified tick.
	// If no more transactions for this tick, return nullptr.
	static Transaction* get(unsigned int tick, unsigned int index)
	{
		if (firstSavedTick <= tick && tick < firstSavedTick + maxNumTicksToSave)
		{
			unsigned int tickIndex = tick - firstSavedTick;
			ACQUIRE(lockNumSaved);
			bool hasTx = index < numSavedTxsPerTick[tickIndex];
			RELEASE(lockNumSaved);
			if (hasTx)
			{
				return &transactions[tickIndex * NUMBER_OF_TRANSACTIONS_PER_TICK + index];
			}
			else
			{
				return nullptr;
			}
		}
		return nullptr;
	}

	// Get a transaction digest for the specified tick.
	// If no more transactions for this tick, return nullptr.
	static m256i* getDigest(unsigned int tick, unsigned int index)
	{
		if (firstSavedTick <= tick && tick < firstSavedTick + maxNumTicksToSave)
		{
			unsigned int tickIndex = tick - firstSavedTick;
			ACQUIRE(lockNumSaved);
			bool hasTx = index < numSavedTxsPerTick[tickIndex];
			RELEASE(lockNumSaved);
			if (hasTx)
			{
				return &digests[tickIndex * NUMBER_OF_TRANSACTIONS_PER_TICK + index];
			}
			else
			{
				return nullptr;
			}
		}
		return nullptr;
	}

	// Begin new epoch. Delete transactions from previous epoch (except txs from the last TICKS_TO_KEEP_FROM_PRIOR_EPOCH ticks).
	static void beginEpoch(unsigned int newInitialTick)
	{
		// TODO
	}

};
