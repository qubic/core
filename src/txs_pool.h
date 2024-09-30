#pragma once

#include <network_messages/transactions.h>

// Mempool that saves pending transactions (txs) of all entities.
// This is a kind of singleton class with only static members (so all instances refer to the same data).
class TxsPool
{
private:
	static constexpr unsigned long long maxNumTicksToSave = MAX_NUMBER_OF_TICKS_PER_EPOCH + TICKS_TO_KEEP_FROM_PRIOR_EPOCH;
	static constexpr unsigned long long maxNumTxsToSave = maxNumTicksToSave * NUMBER_OF_TRANSACTIONS_PER_TICK;
	static constexpr unsigned long long maxNumTicksPerEpoch = MAX_NUMBER_OF_TICKS_PER_EPOCH;

	static volatile char digestsLock;
	static volatile char txsLock;

	inline static Transaction* transactions;
	inline static m256i* digests;

	inline static unsigned int* numSavedTxsPerTick;

	static unsigned int firstSavedTick;

public:
	// Init at node startup
	static bool init();

	// Cleanup at node shutdown
	static void deinit();

	// Return number of transaction scheduled later than the specified tick.
	static unsigned int getNumberOfPendingTxs(unsigned int tick);

	// Check validity of transaction and add to the pool.
	static void update(Transaction* tx)
	{
		// A node doesn't need to store a transaction that is scheduled on a tick that node will never reach.
		// Notice: MAX_NUMBER_OF_TICKS_PER_EPOCH is not set globally since every node may have different TARGET_TICK_DURATION time due to memory limitation.
		if (tx->tick >= firstSavedTick && tx->tick < system.initialTick + maxNumTicksPerEpoch)
		{
			unsigned int tickIndex = tx->tick - firstSavedTick;

			ACQUIRE(txsLock);
			ACQUIRE(digestsLock);
			if (numSavedTxsPerTick[tickIndex] < NUMBER_OF_TRANSACTIONS_PER_TICK)
			{
				const unsigned int transactionSize = tx->totalSize();
				copyMem(&transactions[tickIndex * NUMBER_OF_TRANSACTIONS_PER_TICK + numSavedTxsPerTick[tickIndex]], tx, transactionSize);
				KangarooTwelve(tx, transactionSize, &digests[tickIndex], 32);
				numSavedTxsPerTick[tickIndex]++;
			}
			// TODO: add a strategy to potentially overwrite existing transactions if max number of transations for tick has been reached. 

			RELEASE(digestsLock);
			RELEASE(txsLock);
		}
	}

	// Get a transaction for the specified tick.
	// If no more transactions for this tick, return nullptr.
	static Transaction* get(unsigned int tick, int index);

	// Get a transaction digest for the specified tick.
	// If no more transactions for this tick, return nullptr.
	static m256i getDigest(unsigned int tick, int index);

	// Delete transactions from previous epoch (except for the last TICKS_TO_KEEP_FROM_PRIOR_EPOCH ticks).
	static void transitionEpoch(unsigned int epochInitialTick);

};
