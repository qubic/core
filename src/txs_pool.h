#pragma once

#include "network_messages/transactions.h"

#include "platform/memory.h"
#include "platform/concurrency.h"
#include "platform/console_logging.h"

#include "public_settings.h"
#include "tick_txs_storage.h"
#include <kangaroo_twelve.h>

// Mempool that saves pending transactions (txs) of all entities.
// This is a kind of singleton class with only static members (so all instances refer to the same data).
class TxsPool
{
private:
	static constexpr unsigned long long txsDigestsLengthCurrentEpoch = ((unsigned long long)MAX_NUMBER_OF_TICKS_PER_EPOCH) * NUMBER_OF_TRANSACTIONS_PER_TICK;
	static constexpr unsigned long long txsDigestsLengthPreviousEpoch = ((unsigned long long)TICKS_TO_KEEP_FROM_PRIOR_EPOCH) * NUMBER_OF_TRANSACTIONS_PER_TICK;
	static constexpr unsigned long long txsDigestsSizeCurrentEpoch = txsDigestsLengthCurrentEpoch * 32ULL;
	static constexpr unsigned long long txsDigestsSizePreviousEpoch = txsDigestsLengthPreviousEpoch * 32ULL;
	static constexpr unsigned long long txsDigestsSize = txsDigestsSizeCurrentEpoch + txsDigestsSizePreviousEpoch;
	static constexpr unsigned long long maxNumTicksToSave = MAX_NUMBER_OF_TICKS_PER_EPOCH + TICKS_TO_KEEP_FROM_PRIOR_EPOCH;

	// Tick number range of current epoch storage
	inline static unsigned int tickBegin = 0;
	inline static unsigned int tickEnd = 0;

	// Tick number range of previous epoch storage
	inline static unsigned int oldTickBegin = 0;
	inline static unsigned int oldTickEnd = 0;

	// Allocated txsDigests buffer with txsDigestsLength elements (includes current and previous epoch data)
	inline static m256i* txsDigestsPtr = nullptr;

	// Transaction digests buffer of previous epoch. Points to txsDigestsPtr + txsDigestsLengthCurrentEpoch
	inline static m256i* oldTxsDigestsPtr = nullptr;

	// Records the number of saved transactions for each tick (includes current and previous epoch data)
	inline static unsigned int numSavedTxsPerTick[maxNumTicksToSave];

	// Lock for securing txsDigests
	inline static volatile char txsDigestsLock = 0;
	// Lock for securing numSavedTxsPerTick
	inline static volatile char numSavedLock = 0;

	// Storage for transactions data
	inline static TickTransactionsStorage transactionsStorage;

public:
	// Init at node startup.
	static bool init()
	{
		if (!allocatePool(txsDigestsSize, (void**)&txsDigestsPtr)
			|| !transactionsStorage.init())
		{
			logToConsole(L"Failed to allocate transaction pool memory!");
			return false;
		}

		ASSERT(txsDigestsLock == 0);
		ASSERT(numSavedLock == 0);
		setMem((void*)numSavedTxsPerTick, sizeof(numSavedTxsPerTick), 0);

		oldTxsDigestsPtr = txsDigestsPtr + txsDigestsLengthCurrentEpoch;

		tickBegin = 0;
		tickEnd = 0;
		oldTickBegin = 0;
		oldTickEnd = 0;

		return true;
	}

	// Cleanup at node shutdown.
	static void deinit()
	{
		if (txsDigestsPtr)
		{
			freePool(txsDigestsPtr);
		}
		transactionsStorage.deinit();
	}

	// Acquire lock for returned pointers to transactions or digests.
	inline static void acquireLock()
	{
		ACQUIRE(txsDigestsLock);
		transactionsStorage.tickTransactions.acquireLock();
	}

	// Release lock for returned pointers to transactions or digests.
	inline static void releaseLock()
	{
		transactionsStorage.tickTransactions.acquireLock();
		RELEASE(txsDigestsLock);
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
	unsigned static int tickToIndexCurrentEpoch(unsigned int tick)
	{
		return tick - tickBegin;
	}

	// Return index of tick data in previous epoch (does not check that it is stored).
	unsigned static int tickToIndexPreviousEpoch(unsigned int tick)
	{
		return tick - oldTickBegin + MAX_NUMBER_OF_TICKS_PER_EPOCH;
	}

	// Return number of transactions scheduled later than the specified tick.
	static unsigned int getNumberOfPendingTxs(unsigned int tick)
	{
		unsigned int res = 0;
		unsigned int startTick = tickEnd;
		unsigned int oldStartTick = oldTickEnd;

		if (tick < oldTickBegin)
		{
			startTick = tickBegin;
			oldStartTick = oldTickBegin;
		}
		else if (tickInPreviousEpochStorage(tick))
		{
			startTick = tickBegin;
			oldStartTick = tick + 1;
		}
		else if (tickInCurrentEpochStorage(tick))
		{
			startTick = tick + 1;
		}

		ACQUIRE(numSavedLock);
		for (unsigned int t = startTick; t < tickEnd; ++t)
		{
			res += numSavedTxsPerTick[tickToIndexCurrentEpoch(t)];
		}
		for (unsigned int t = oldStartTick; t < oldTickEnd; ++t)
		{
			res += numSavedTxsPerTick[tickToIndexPreviousEpoch(t)];
		}
		RELEASE(numSavedLock);

		return res;
	}

	// Check validity of transaction and add to the pool.
	static void update(Transaction* tx)
	{
		if (tx->checkValidity() && tickInCurrentEpochStorage(tx->tick))
		{
			unsigned int tickIndex = tickToIndexCurrentEpoch(tx->tick);
			const unsigned int transactionSize = tx->totalSize();

			ACQUIRE(numSavedLock);
			acquireLock();

			if (numSavedTxsPerTick[tickIndex] < NUMBER_OF_TRANSACTIONS_PER_TICK
				&& transactionsStorage.nextTickTransactionOffset + transactionSize <= transactionsStorage.tickTransactions.storageSpaceCurrentEpoch)
			{
				unsigned long long& transactionOffset = transactionsStorage.tickTransactionOffsets.getByTickIndex(tickIndex)[numSavedTxsPerTick[tickIndex]];
				ASSERT(transactionOffset == 0);

				KangarooTwelve(tx, transactionSize, &txsDigestsPtr[tickIndex * NUMBER_OF_TRANSACTIONS_PER_TICK + numSavedTxsPerTick[tickIndex]], 32ULL);
				transactionOffset = transactionsStorage.nextTickTransactionOffset;
				bs->CopyMem(transactionsStorage.tickTransactions(transactionOffset), tx, transactionSize);
				transactionsStorage.nextTickTransactionOffset += transactionSize;

				numSavedTxsPerTick[tickIndex]++;
			}

			releaseLock();
			RELEASE(numSavedLock);
		}
	}

	// Get a transaction for the specified tick.
	// If no more transactions for this tick, return nullptr.
	static Transaction* get(unsigned int tick, unsigned int index)
	{
		unsigned int tickIndex;
		if (tickInCurrentEpochStorage(tick))
		{
			tickIndex = tickToIndexCurrentEpoch(tick);
		}
		else if (tickInPreviousEpochStorage(tick))
		{
			tickIndex = tickToIndexPreviousEpoch(tick);
		}
		else
		{
			return nullptr;
		}

		ACQUIRE(numSavedLock);
		bool hasTx = index < numSavedTxsPerTick[tickIndex];
		RELEASE(numSavedLock);

		if (hasTx)
		{
			unsigned long long offset = transactionsStorage.tickTransactionOffsets.getByTickIndex(tickIndex)[index];
			ASSERT(offset != 0);
			return transactionsStorage.tickTransactions.ptr(offset);
		}
		else
		{
			return nullptr;
		}
	}

	// Get a transaction digest for the specified tick.
	// If no more transactions for this tick, return nullptr.
	static m256i* getDigest(unsigned int tick, unsigned int index)
	{
		unsigned int tickIndex;
		if (tickInCurrentEpochStorage(tick))
		{
			tickIndex = tickToIndexCurrentEpoch(tick);
		}
		else if (tickInPreviousEpochStorage(tick))
		{
			tickIndex = tickToIndexPreviousEpoch(tick);
		}
		else
		{
			return nullptr;
		}

		ACQUIRE(numSavedLock);
		bool hasTx = index < numSavedTxsPerTick[tickIndex];
		RELEASE(numSavedLock);

		if (hasTx)
		{
			return &txsDigestsPtr[tickIndex * NUMBER_OF_TRANSACTIONS_PER_TICK + index];
		}
		else
		{
			return nullptr;
		}
	}


	// Begin new epoch. If not called the first time (seamless transition), assume that the ticks to keep
	// are ticks in [newInitialTick-TICKS_TO_KEEP_FROM_PRIOR_EPOCH, newInitialTick-1].
	void beginEpoch(unsigned int newInitialTick)
	{
		if (tickBegin && tickInCurrentEpochStorage(newInitialTick) && tickBegin < newInitialTick)
		{
			// seamless epoch transition: keep some ticks of prior epoch
			oldTickEnd = newInitialTick;
			oldTickBegin = newInitialTick - TICKS_TO_KEEP_FROM_PRIOR_EPOCH;
			if (oldTickBegin < tickBegin)
				oldTickBegin = tickBegin;

			const unsigned int tickIndex = tickToIndexCurrentEpoch(oldTickBegin);
			const unsigned int tickCount = oldTickEnd - oldTickBegin;

			copyMem(oldTxsDigestsPtr, &txsDigestsPtr[tickIndex * NUMBER_OF_TRANSACTIONS_PER_TICK], tickCount * NUMBER_OF_TRANSACTIONS_PER_TICK * 32ULL);
			copyMem(&numSavedTxsPerTick[MAX_NUMBER_OF_TICKS_PER_EPOCH], &numSavedTxsPerTick[tickIndex], tickCount * sizeof(numSavedTxsPerTick[0]));

			setMem(txsDigestsPtr, txsDigestsSizeCurrentEpoch, 0);
		}
		else
		{
			// node startup with no data of prior epoch (also use storage for prior epoch for current)
			setMem(txsDigestsPtr, txsDigestsSize, 0);
			oldTickBegin = 0;
			oldTickEnd = 0;
		}

		tickBegin = newInitialTick;
		tickEnd = newInitialTick + MAX_NUMBER_OF_TICKS_PER_EPOCH;

		transactionsStorage.beginEpoch(newInitialTick);
	}

};
