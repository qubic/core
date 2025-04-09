#include "platform/memory.h"
#include "network_messages/transactions.h"

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

    m256i miningSeed;
    m256i nonce;
    unsigned char signature[SIGNATURE_SIZE];
};

constexpr int CUSTOM_MINING_SHARE_COUNTER_INPUT_TYPE = 8;
constexpr int TICK_CUSTOM_MINING_SHARE_COUNTER_PUBLICATION_OFFSET = 4;

struct CustomMiningSolutionTransaction : public Transaction
{
    static constexpr unsigned char transactionType()
    {
        return CUSTOM_MINING_SHARE_COUNTER_INPUT_TYPE;
    }
};

struct CustomMiningTask
{
    unsigned long long taskIndex; // ever increasing number (unix timestamp in ms)

    unsigned char blob[408]; // Job data from pool
    unsigned long long size;  // length of the blob
    unsigned long long target; // Pool difficulty
    unsigned long long height; // Block height
    unsigned char seed[32]; // Seed hash for XMR
    unsigned int extraNonce;
};

struct CustomMiningSolution
{
    unsigned long long taskIndex; // should match the index from task
    unsigned int nonce;         // xmrig::JobResult.nonce
    unsigned int padding;
    m256i result;               // xmrig::JobResult.result, 32 bytes
};

#define CUSTOM_MINING_SHARES_COUNT_SIZE_IN_BYTES 848
#define CUSTOM_MINING_SOLUTION_NUM_BIT_PER_COMP 10
static_assert((1 << CUSTOM_MINING_SOLUTION_NUM_BIT_PER_COMP) >= NUMBER_OF_COMPUTORS, "Invalid number of bit per datum");
static_assert(CUSTOM_MINING_SHARES_COUNT_SIZE_IN_BYTES * 8 >= NUMBER_OF_COMPUTORS * CUSTOM_MINING_SOLUTION_NUM_BIT_PER_COMP, "Invalid data size");
static char accumulatedSharedCountLock = 0;

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

    // get and compress number of shares of 676 computors to 676x10 bit numbers
    void compressNewSharesPacket(unsigned int ownComputorIdx, unsigned char customMiningShareCountPacket[CUSTOM_MINING_SHARES_COUNT_SIZE_IN_BYTES])
    {
        setMem(customMiningShareCountPacket, sizeof(customMiningShareCountPacket), 0);
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
};

// In charge of storing custom mining tasks
constexpr unsigned long long CUSTOM_MINING_TASK_STORAGE_COUNT = CUSTOM_MINING_TASK_STORAGE_SIZE / sizeof(CustomMiningTask);
class CustomMiningTaskStorage
{
public:
    static constexpr unsigned long long _invalidTaskIndex = 0xFFFFFFFFFFFFFFFFULL;
    void init()
    {
        allocatePool(CUSTOM_MINING_TASK_STORAGE_COUNT * sizeof(CustomMiningTask), (void**)&_customMiningTasks);
        allocatePool(CUSTOM_MINING_TASK_STORAGE_COUNT * sizeof(unsigned long long), (void**)&_taskIndices);
        _storageIndex = 0;
        _customMiningPhaseCount = 0;
    }

    void deinit()
    {
        if (NULL != _customMiningTasks)
        {
            freePool(_customMiningTasks);
            _customMiningTasks = NULL;
        }
        if (NULL != _taskIndices)
        {
            freePool(_taskIndices);
            _taskIndices = NULL;
        }
    }

    void reset()
    {
        _storageIndex = 0;
        _customMiningPhaseCount = 0;
        //setMem(_taskIndices, sizeof(_taskIndices), 0xFF);
    }

    void checkAndReset()
    {
        _customMiningPhaseCount++;
        if (_customMiningPhaseCount % CUSTOM_MINING_TASK_STORAGE_RESET_PHASE == 0)
        {
            reset();
        }
    }

    // Binary search for taskIndex or closest less-than index
    unsigned long long searchTaskIndex(unsigned long long taskIndex, bool& exactMatch) const
    {
        unsigned long long left = 0, right = (_storageIndex > 0) ? _storageIndex - 1 : 0;
        unsigned long long result = _invalidTaskIndex;
        exactMatch = false;

        while (left <= right && left < _storageIndex)
        {
            unsigned long long mid = (left + right) / 2;
            unsigned long long midTaskIndex = _customMiningTasks[_taskIndices[mid]].taskIndex;

            if (midTaskIndex == taskIndex)
            {
                exactMatch = true;
                return mid;
            }
            else if (midTaskIndex < taskIndex)
            {
                left = mid + 1;
            }
            else
            {
                result = mid;
                if (mid == 0)
                {
                    break; // prevent underflow
                }
                right = mid - 1;
            }
        }

        return result;
    }


    // Return the task whose task index >= taskIndex
    unsigned long long lookForTaskGE(unsigned long long taskIndex)
    {
        bool exactMatch = false;
        unsigned long long idx = searchTaskIndex(taskIndex, exactMatch);
        return idx;
    }

    // Return the task whose task index == taskIndex
    unsigned long long lookForTask(unsigned long long taskIndex)
    {
        bool exactMatch = false;
        unsigned long long idx = searchTaskIndex(taskIndex, exactMatch);
        if (exactMatch)
        {
            return idx;
        }
        return _invalidTaskIndex;
    }

    bool taskExisted(const CustomMiningTask& task)
    {
        bool exactMatch = false;
        searchTaskIndex(task.taskIndex, exactMatch);
        return exactMatch;
    }

    // Add the task to the storage
    void addTask(const CustomMiningTask& task)
    {
        // Don't added if the task already existed
        if (taskExisted(task))
        {
            return;
        }
        
        // Reset the storage if the number of tasks exceeds the limit
        if (_storageIndex >= CUSTOM_MINING_TASK_STORAGE_COUNT)
        {
            reset();
        }

        unsigned long long newIndex = _storageIndex;
        _customMiningTasks[newIndex] = task;

        unsigned long long insertPos = 0;
        unsigned long long left = 0, right = (_storageIndex > 0) ? _storageIndex - 1 : 0;

        while (left <= right && left < _storageIndex)
        {
            unsigned long long mid = (left + right) / 2;
            if (_customMiningTasks[_taskIndices[mid]].taskIndex < task.taskIndex)
            {
                left = mid + 1;
            }
            else
            {
                if (mid == 0) break;
                right = mid - 1;
            }
        }
        insertPos = left;

        // Shift indices right
        for (unsigned long long i = _storageIndex; i > insertPos; --i)
        {
            _taskIndices[i] = _taskIndices[i - 1];
        }

        _taskIndices[insertPos] = newIndex;
        _storageIndex++;

    }

    // Get the task from raw index
    CustomMiningTask* getTaskByIndex(unsigned long long index)
    {
        if (index >= _storageIndex)
        {
            return NULL;
        }
        return &_customMiningTasks[_taskIndices[index]];
    }

    // Get the total number of tasks
    unsigned long long getTaskCount() const
    {
        return _storageIndex;
    }

private:
    CustomMiningTask* _customMiningTasks;
    unsigned long long* _taskIndices;
    unsigned long long _storageIndex;
    unsigned long long _customMiningPhaseCount;
};
