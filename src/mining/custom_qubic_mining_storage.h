#pragma once

#include <cstdint>

#include "platform/memory.h"
#include "platform/memory_util.h"
#include "platform/concurrency.h"
#include "platform/m256.h"
#include "oracle_core/oracle_transactions.h"
#include "oracle_core/oracle_interfaces_def.h"
#include "network_messages/custom_mining.h"
#include "contract_core/qpi_hash_map_impl.h"
#include "kangaroo_twelve.h"


class CustomQubicMiningStorage
{
public:
    static constexpr unsigned int maxNumTasks = 32;

    // A struct for storing an active doge mining task on the node.
    struct StoredDogeMiningTask
    {
        uint8_t dispatcherTarget[32]; // dispatcher target, usually easier than pool and network difficulty, full 32-byte representation

        // Full header can be constructed via concatenating version + prevHash + merkleRoot + miner's nTime + nBits + miner's nonce.
        uint8_t version[4]; // 4 bytes version
        uint8_t prevHash[32]; // 32 bytes prevBlockHash
        uint8_t nBits[4]; // 4 bytes network difficulty (nBits)
    };

    enum OracleQueryStatus : uint8_t
    {
        SCHEDULED,
        STARTED,
    };

    struct OracleQueryInfo
    {
        unsigned int tick;
        OracleQueryStatus status;
        m256i sourcePublicKey;
        unsigned int numTries;
    };

    bool init();

    void deinit();

    bool addTask(const CustomQubicMiningTask* task, unsigned int size);

    // Return -1 if the solution is invalid (stale or duplicate), 0 if the solution is valid but not added due to storage limit,
    // and 1 if the solution is valid and added successfully. If taskDescription is not nullptr and the solution corresponds to an active task,
    // the task description is written into the provided pointer.
    int addSolution(const CustomQubicMiningSolution* solution, unsigned int size, unsigned char* taskDescription = nullptr);

    bool containsTask(uint8_t customMiningType, uint64_t jobId);

    bool addOracleQuery(const OracleUserQueryTransactionPrefix* queryTx);

    // Returns true if the fail counter for this query could be increased without hitting maxNumTries. Returns false if the query is not found
    // or the counter has hit maxNumTries. A query that has hit maxNumTries is removed from storage.
    bool increaseOracleQueryFailCounter(const OracleUserQueryTransactionPrefix* queryTx);

    bool updateOracleQueryScheduledTick(const OracleUserQueryTransactionPrefix* queryTx, unsigned int newScheduledTick);

    bool removeOracleQuery(const OracleUserQueryTransactionPrefix* queryTx);

    bool markOracleQueryStarted(const OracleUserQueryTransactionPrefix* queryTx);

    void resendNotStartedOracleQueriesForTick(unsigned int tick, unsigned long long compSeedsCount, const m256i* compSubseeds, const m256i* compPubKeys);

private:
    static constexpr unsigned int maxNumSolutionsPerTask = 128;
    static constexpr unsigned int oracleQueryMaxNumTries = 5;

    // General storage that is the same for all mining types.
    uint64_t activeTasks[CustomMiningType::TOTAL_NUM_TYPES][maxNumTasks];
    unsigned int nextTaskIndex[CustomMiningType::TOTAL_NUM_TYPES];
    OracleQueryInfo oracleQueries[CustomMiningType::TOTAL_NUM_TYPES][maxNumTasks * maxNumSolutionsPerTask];

    // For each task, we store a set of received solution hashes to prevent duplicate solutions.
    // Two-dimensional array [CustomMiningType::TOTAL_NUM_TYPES][maxNumTasks] indexed by mining type and type-specific task index.
    QPI::HashSet<m256i, maxNumSolutionsPerTask>* receivedSolutions;
    static constexpr unsigned long long receivedSolutionsSize = CustomMiningType::TOTAL_NUM_TYPES * maxNumTasks * sizeof(QPI::HashSet<m256i, maxNumSolutionsPerTask>);

    // Storage for type-specific task descriptions.
    StoredDogeMiningTask dogeTasks[maxNumTasks];

    // Storage for type-specific oracle queries.
    OI::DogeShareValidation::OracleQuery dogeOracleQueries[maxNumTasks * maxNumSolutionsPerTask];

    inline static volatile char lock = 0;

    // Return the mining-type-specific index of the task with the given jobId and customMiningType, or -1 if not found.
    // The returned index can then be used to access the type-specific task descriptions and the solution hash set for this task.
    int findTask(uint8_t customMiningType, uint64_t jobId) const;

    // Return the mining-type-specific index of the oracle query, or -1 if not found.
    int findOracleQuery(uint8_t customMiningType, const OracleUserQueryTransactionPrefix* queryTx) const;

    bool isQueryEqual(uint8_t customMiningType, unsigned int queryIndex, const char* typeSpecificOracleQuery) const;

    void removeOracleQuery(uint8_t customMiningType, unsigned int queryIndex);
};
