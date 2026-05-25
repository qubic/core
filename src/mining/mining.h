#pragma once

#include "platform/assert.h"
#include "platform/concurrency.h"
#include "platform/memory.h"
#include "platform/memory_util.h"
#include "network_messages/custom_mining.h"
#include "network_messages/transactions.h"
#include "kangaroo_twelve.h"

#include <lib/platform_efi/uefi.h>

static unsigned int getTickInDogeBroadcastCycle()
{
#ifdef NO_UEFI
    return 0;
#else
    return (system.tick) % DOGE_BROADCAST_CYCLE;
#endif
}

struct MiningSolutionTransaction : public Transaction
{
    static constexpr unsigned char transactionType()
    {
        return 2; // TODO: Set actual value
    }

    static constexpr long long minAmount()
    {
        return SOLUTION_SECURITY_DEPOSIT;
    }

    static constexpr unsigned short minInputSize()
    {
        return sizeof(miningSeed) + sizeof(nonce);
    }

    static bool isSolutionTransaction(const Transaction* tx)
    {
        return isZero(tx->destinationPublicKey)
            && tx->inputType == transactionType()
            && tx->amount >= minAmount()
            && tx->inputSize >= minInputSize();
    }

    m256i miningSeed;
    m256i nonce;
    unsigned char signature[SIGNATURE_SIZE];
};

// Define in doc/protocol.md
constexpr int DOGE_MINING_SHARE_COUNTER_INPUT_TYPE = 11;
struct DogeMiningShareTransaction : public Transaction
{
    static constexpr unsigned char transactionType()
    {
        return DOGE_MINING_SHARE_COUNTER_INPUT_TYPE;
    }
};


#define CUSTOM_MINING_SHARES_COUNT_SIZE_IN_BYTES 848
#define CUSTOM_MINING_SOLUTION_NUM_BIT_PER_COMP 10
#define TICK_VOTE_COUNTER_PUBLICATION_OFFSET 2 // Must be 2
static constexpr int CUSTOM_MINING_SOLUTION_SHARES_COUNT_MAX_VAL = (1U << CUSTOM_MINING_SOLUTION_NUM_BIT_PER_COMP) - 1;
static_assert((1 << CUSTOM_MINING_SOLUTION_NUM_BIT_PER_COMP) >= NUMBER_OF_COMPUTORS, "Invalid number of bit per datum");
static_assert(CUSTOM_MINING_SHARES_COUNT_SIZE_IN_BYTES * 8 >= NUMBER_OF_COMPUTORS * CUSTOM_MINING_SOLUTION_NUM_BIT_PER_COMP, "Invalid data size");

struct CustomMiningSharePayload
{
    Transaction transaction;
    unsigned char packedScore[CUSTOM_MINING_SHARES_COUNT_SIZE_IN_BYTES];
    m256i dataLock;
    unsigned char signature[SIGNATURE_SIZE];
};

struct BroadcastCustomMiningTransaction
{
    CustomMiningSharePayload payload;
    bool isBroadcasted;
};

static BroadcastCustomMiningTransaction gDogeMiningBroadcastTxBuffer[NUMBER_OF_COMPUTORS];

class CustomMiningSharesCounter
{
private:
    unsigned int _shareCount[NUMBER_OF_COMPUTORS];
    unsigned long long _accumulatedSharesCount[NUMBER_OF_COMPUTORS];
    unsigned int _buffer[NUMBER_OF_COMPUTORS];
protected:
    unsigned int extract10Bit(const unsigned char* data, unsigned int idx)
    {
        //TODO: simplify this
        unsigned int byte0 = data[idx + (idx >> 2)];
        unsigned int byte1 = data[idx + (idx >> 2) + 1];
        unsigned int last_bit0 = 8 - (idx & 3) * 2;
        unsigned int first_bit1 = 10 - last_bit0;
        unsigned int res = (byte0 & ((1 << last_bit0) - 1)) << first_bit1;
        res |= (byte1 >> (8 - first_bit1));
        return res;
    }
    void update10Bit(unsigned char* data, unsigned int idx, unsigned int value)
    {
        //TODO: simplify this
        unsigned char& byte0 = data[idx + (idx >> 2)];
        unsigned char& byte1 = data[idx + (idx >> 2) + 1];
        unsigned int last_bit0 = 8 - (idx & 3) * 2;
        unsigned int first_bit1 = 10 - last_bit0;
        unsigned char mask0 = ~((1 << last_bit0) - 1);
        unsigned char mask1 = ((1 << (8 - first_bit1)) - 1);
        byte0 &= mask0;
        byte1 &= mask1;
        unsigned char ubyte0 = (value >> first_bit1);
        unsigned char ubyte1 = (value & ((1 << first_bit1) - 1)) << (8 - first_bit1);
        byte0 |= ubyte0;
        byte1 |= ubyte1;
    }

    void accumulateSharesCount(unsigned int computorIdx, unsigned int value)
    {
        _accumulatedSharesCount[computorIdx] += value;
    }

public:

    static constexpr unsigned int _customMiningSolutionCounterDataSize = sizeof(_shareCount) + sizeof(_accumulatedSharesCount);
    void init()
    {
        setMem(_shareCount, sizeof(_shareCount), 0);
        setMem(_accumulatedSharesCount, sizeof(_accumulatedSharesCount), 0);
    }

    void registerNewShareCount(const unsigned int* sharesCount)
    {
        copyMem(_shareCount, sharesCount, sizeof(_shareCount));
    }

    bool isEmptyPacket(const unsigned char* data) const
    {
        for (int i = 0; i < CUSTOM_MINING_SHARES_COUNT_SIZE_IN_BYTES; i++)
        {
            if (data[i] != 0)
            {
                return false;
            }
        }
        return true;
    }

    // get and compress number of shares of 676 computors to 676x10 bit numbers
    void compressNewSharesPacket(unsigned int ownComputorIdx, unsigned char customMiningShareCountPacket[CUSTOM_MINING_SHARES_COUNT_SIZE_IN_BYTES])
    {
        setMem(customMiningShareCountPacket, CUSTOM_MINING_SHARES_COUNT_SIZE_IN_BYTES, 0);
        setMem(_buffer, sizeof(_buffer), 0);
        for (int j = 0; j < NUMBER_OF_COMPUTORS; j++)
        {
            _buffer[j] = _shareCount[j];
        }

        _buffer[ownComputorIdx] = 0; // remove self-report
        for (unsigned int i = 0; i < NUMBER_OF_COMPUTORS; i++)
        {
            update10Bit(customMiningShareCountPacket, i, _buffer[i]);
        }
    }

    bool validateNewSharesPacket(const unsigned char* customMiningShareCountPacket, unsigned int computorIdx)
    {
        // check #1: own number of share must be zero
        if (extract10Bit(customMiningShareCountPacket, computorIdx) != 0)
        {
            return false;
        }
        return true;
    }

    void addShares(const unsigned char* newSharePacket, unsigned int computorIdx)
    {
        if (validateNewSharesPacket(newSharePacket, computorIdx))
        {
            for (int i = 0; i < NUMBER_OF_COMPUTORS; i++)
            {
                unsigned int shareCount = extract10Bit(newSharePacket, i);
                accumulateSharesCount(i, shareCount);
            }
        }
    }

    unsigned long long getSharesCount(unsigned int computorIdx)
    {
        return _accumulatedSharesCount[computorIdx];
    }

    void saveAllDataToArray(unsigned char* dst)
    {
        copyMem(dst, &_shareCount[0], sizeof(_shareCount));
        copyMem(dst + sizeof(_shareCount), &_accumulatedSharesCount[0], sizeof(_accumulatedSharesCount));
    }

    void loadAllDataFromArray(const unsigned char* src)
    {
        copyMem(&_shareCount[0], src, sizeof(_shareCount));
        copyMem(&_accumulatedSharesCount[0], src + sizeof(_shareCount), sizeof(_accumulatedSharesCount));
    }

    void processTransactionData(const Transaction* transaction, const m256i& dataLock)
    {
#ifndef NO_UEFI
        int computorIndex = transaction->tick % NUMBER_OF_COMPUTORS;
        int tickInDogeCycle = getTickInDogeBroadcastCycle();
        if (transaction->sourcePublicKey == broadcastedComputors.computors.publicKeys[computorIndex] // this tx was sent by the tick leader of this tick
            && transaction->inputSize == CUSTOM_MINING_SHARES_COUNT_SIZE_IN_BYTES + sizeof(m256i)
            && tickInDogeCycle <= NUMBER_OF_COMPUTORS + TICK_VOTE_COUNTER_PUBLICATION_OFFSET) // DOGE share tx acceptance window: +2 ticks after the start of each broadcast cycle
        {
            if (!transaction->amount)
            {
                m256i txDataLock = m256i(transaction->inputPtr() + CUSTOM_MINING_SHARES_COUNT_SIZE_IN_BYTES);
                if (txDataLock == dataLock)
                {
                    addShares(transaction->inputPtr(), computorIndex);
                }
#ifndef NDEBUG
                else
                {
                    CHAR16 dbg[256];
                    setText(dbg, L"TRACE: [Custom mining point tx] Wrong datalock from comp ");
                    appendNumber(dbg, computorIndex, false);
                    addDebugMessage(dbg);
                }
#endif
            }
        }
#endif
    }
};

// Stats of custom mining.
// Reset after epoch change.
// Some variable is reset after end of each custom mining phase.
struct CustomMiningStats
{
    struct Counter
    {
        long long tasks;        // Task that replaced by node

        // Shares related
        long long shares;       // Total shares/solutions = unverifed/no-task + valid + invalid
        long long valid;        // Solutions are marked as valid by verififer
        long long invalid;      // Solutions are marked as invalid by verififer
        long long duplicated;   // Duplicated solutions, ussually causes by solution message broadcasted or

        void reset()
        {
            ATOMIC_STORE64(tasks, 0);

            ATOMIC_STORE64(shares, 0);
            ATOMIC_STORE64(valid, 0);
            ATOMIC_STORE64(invalid, 0);
            ATOMIC_STORE64(duplicated, 0);
        }
    };

    // Stats of current epoch until last custom mining phase end
    Counter lastPhases;              // Stats of all last/previous phases
    long long maxOverflowShareCount; // Max number of shares that exceed the data packed in transaction
    long long maxCollisionShareCount; // Max number of shares that are not save in cached because of collision

    // Stats of current custom mining phase
    Counter phaseV2;

    // Asume at begining of epoch.
    void epochReset()
    {
        lastPhases.reset();
        phaseV2.reset();
        ATOMIC_STORE64(maxOverflowShareCount, 0);
        ATOMIC_STORE64(maxCollisionShareCount, 0);
    }

    // At the end of phase. Ussually the task/sols message still arrive
    void phaseResetAndEpochAccumulate()
    {
        // Load the phase stats
        long long tasks = 0;
        long long shares = 0;
        long long valid = 0;
        long long invalid = 0;
        long long duplicated = 0;
        tasks += ATOMIC_LOAD64(phaseV2.tasks);
        shares += ATOMIC_LOAD64(phaseV2.shares);
        valid += ATOMIC_LOAD64(phaseV2.valid);
        invalid += ATOMIC_LOAD64(phaseV2.invalid);
        duplicated += ATOMIC_LOAD64(phaseV2.duplicated);

        // Accumulate the phase into last phases
        ATOMIC_ADD64(lastPhases.tasks, tasks);
        ATOMIC_ADD64(lastPhases.shares, shares);
        ATOMIC_ADD64(lastPhases.valid, valid);
        ATOMIC_ADD64(lastPhases.invalid, invalid);
        ATOMIC_ADD64(lastPhases.duplicated, duplicated);

        phaseV2.reset();
    }

    void appendLog(CHAR16* message)
    {
        long long customMiningTasks = 0;
        long long customMiningShares = 0;
        long long customMiningValidShares = 0;
        long long customMiningInvalidShares = 0;
        long long customMiningDuplicated = 0;
        customMiningTasks = ATOMIC_LOAD64(phaseV2.tasks);
        customMiningShares = ATOMIC_LOAD64(phaseV2.shares);
        customMiningValidShares = ATOMIC_LOAD64(phaseV2.valid);
        customMiningInvalidShares = ATOMIC_LOAD64(phaseV2.invalid);
        customMiningDuplicated = ATOMIC_LOAD64(phaseV2.duplicated);

        appendText(message, L"Phase:");
        appendText(message, L" Tasks: ");
        appendNumber(message, customMiningTasks, true);
        appendText(message, L" | Shares: ");
        appendNumber(message, customMiningShares, true);
        appendText(message, L" | Valid: ");
        appendNumber(message, customMiningValidShares, true);
        appendText(message, L" | InValid: ");
        appendNumber(message, customMiningInvalidShares, true);
        appendText(message, L" | Duplicated: ");
        appendNumber(message, customMiningDuplicated, true);

        long long customMiningEpochTasks = customMiningTasks + ATOMIC_LOAD64(lastPhases.tasks);
        long long customMiningEpochShares = customMiningShares + ATOMIC_LOAD64(lastPhases.shares);
        long long customMiningEpochInvalidShares = customMiningInvalidShares + ATOMIC_LOAD64(lastPhases.invalid);
        long long customMiningEpochValidShares = customMiningValidShares + ATOMIC_LOAD64(lastPhases.valid);
        long long customMiningEpochDuplicated = customMiningDuplicated + ATOMIC_LOAD64(lastPhases.duplicated);

        appendText(message, L". Epoch:");
        appendText(message, L" Tasks: ");
        appendNumber(message, customMiningEpochTasks, false);
        appendText(message, L" | Shares: ");
        appendNumber(message, customMiningEpochShares, false);
        appendText(message, L" | Valid: ");
        appendNumber(message, customMiningEpochValidShares, false);
        appendText(message, L" | Invalid: ");
        appendNumber(message, customMiningEpochInvalidShares, false);
        appendText(message, L" | Duplicated: ");
        appendNumber(message, customMiningEpochDuplicated, false);
    }
};

static CustomMiningStats gDogeMiningStats;
