#pragma once
#ifdef NO_UEFI
static unsigned long long top_of_stack;
#endif
#include "platform/memory_util.h"
#include "platform/m256.h"
#include "platform/concurrency.h"
#include "platform/profiling.h"
#include "public_settings.h"
#include "score_cache.h"
#include "mining/score_engine.h"

template <unsigned long long solutionBufferCount>
struct ScoreFunction
{
    score_engine::ScoreEngine<
        score_engine::HyperIdentityParams<
        HYPERIDENTITY_NUMBER_OF_INPUT_NEURONS,
        HYPERIDENTITY_NUMBER_OF_OUTPUT_NEURONS,
        HYPERIDENTITY_NUMBER_OF_TICKS,
        HYPERIDENTITY_NUMBER_OF_NEIGHBORS,
        HYPERIDENTITY_POPULATION_THRESHOLD,
        HYPERIDENTITY_NUMBER_OF_MUTATIONS,
        HYPERIDENTITY_SOLUTION_THRESHOLD_DEFAULT>,

        score_engine::AdditionParams<
        ADDITION_NUMBER_OF_INPUT_NEURONS,
        ADDITION_NUMBER_OF_OUTPUT_NEURONS,
        ADDITION_NUMBER_OF_TICKS,
        ADDITION_NUMBER_OF_NEIGHBORS,
        ADDITION_POPULATION_THRESHOLD,
        ADDITION_NUMBER_OF_MUTATIONS,
        ADDITION_SOLUTION_THRESHOLD_DEFAULT>
    > _computeBuffer[solutionBufferCount];

    volatile char random2PoolLock;
    unsigned char state[score_engine::STATE_SIZE];
    unsigned char externalPoolVec[score_engine::POOL_VEC_PADDING_SIZE];
    unsigned char poolVec[score_engine::POOL_VEC_PADDING_SIZE];

    void initPool(const unsigned char* miningSeed)
    {
        // Init random2 pool with mining seed
        score_engine::generateRandom2Pool(miningSeed, state, externalPoolVec);
    }

    m256i currentRandomSeed;

    volatile char solutionEngineLock[solutionBufferCount];

#if USE_SCORE_CACHE
    volatile char scoreCacheLock;
    ScoreCache<SCORE_CACHE_SIZE, SCORE_CACHE_COLLISION_RETRIES> scoreCache;
#endif

    void initMiningData(m256i randomSeed)
    {
        // Below assume when a new mining seed is provided, we need to re-calculate the random2 pool
        // Check if random pool need to be re-generated
        if (!isZero(randomSeed))
        {
            initPool(randomSeed.m256i_u8);
        }
        currentRandomSeed = randomSeed; // persist the initial random seed to be able to send it back on system info response

        ACQUIRE(random2PoolLock);
        copyMem(poolVec, externalPoolVec, score_engine::POOL_VEC_PADDING_SIZE);
        RELEASE(random2PoolLock);
    }

    ~ScoreFunction()
    {
        freeMemory();
    }

    void freeMemory()
    {
    }

    bool initMemory()
    {
        random2PoolLock = 0;

        // Make sure all padding data is set as zeros
        setMem(_computeBuffer, sizeof(_computeBuffer), 0);
        for (int i = 0; i < solutionBufferCount; i++)
        {
            _computeBuffer[i].initMemory();
        }

        for (int i = 0; i < solutionBufferCount; i++)
        {
            solutionEngineLock[i] = 0;
        }

#if USE_SCORE_CACHE
        scoreCacheLock = 0;
        setMem(&scoreCache, sizeof(scoreCache), 0);
#endif

        return true;
    }

    // Save score cache to SCORE_CACHE_FILE_NAME
    void saveScoreCache(int epoch, CHAR16* directory = NULL)
    {
#if USE_SCORE_CACHE
        ACQUIRE(scoreCacheLock);
        SCORE_CACHE_FILE_NAME[sizeof(SCORE_CACHE_FILE_NAME) / sizeof(SCORE_CACHE_FILE_NAME[0]) - 4] = epoch / 100 + L'0';
        SCORE_CACHE_FILE_NAME[sizeof(SCORE_CACHE_FILE_NAME) / sizeof(SCORE_CACHE_FILE_NAME[0]) - 3] = (epoch % 100) / 10 + L'0';
        SCORE_CACHE_FILE_NAME[sizeof(SCORE_CACHE_FILE_NAME) / sizeof(SCORE_CACHE_FILE_NAME[0]) - 2] = epoch % 10 + L'0';
        scoreCache.save(SCORE_CACHE_FILE_NAME, directory);
        RELEASE(scoreCacheLock);
#endif
    }

    // Update score cache filename with epoch and try to load file
    bool loadScoreCache(int epoch)
    {
        bool success = true;
#if USE_SCORE_CACHE
        ACQUIRE(scoreCacheLock);
        SCORE_CACHE_FILE_NAME[sizeof(SCORE_CACHE_FILE_NAME) / sizeof(SCORE_CACHE_FILE_NAME[0]) - 4] = epoch / 100 + L'0';
        SCORE_CACHE_FILE_NAME[sizeof(SCORE_CACHE_FILE_NAME) / sizeof(SCORE_CACHE_FILE_NAME[0]) - 3] = (epoch % 100) / 10 + L'0';
        SCORE_CACHE_FILE_NAME[sizeof(SCORE_CACHE_FILE_NAME) / sizeof(SCORE_CACHE_FILE_NAME[0]) - 2] = epoch % 10 + L'0';
        success = scoreCache.load(SCORE_CACHE_FILE_NAME);
        RELEASE(scoreCacheLock);
#endif
        return success;
    }

    bool isValidScore(unsigned int solutionScore, score_engine::AlgoType selectedAlgo)
    {
        if (selectedAlgo == score_engine::AlgoType::HyperIdentity)
        {
            return (solutionScore >= 0) 
                && (solutionScore <= HYPERIDENTITY_NUMBER_OF_OUTPUT_NEURONS)
                && (solutionScore != score_engine::INVALID_SCORE_VALUE);
        }
        else if (selectedAlgo == score_engine::AlgoType::Addition)
        {
            return (solutionScore >= 0 )
                && (solutionScore <= ADDITION_NUMBER_OF_OUTPUT_NEURONS * (1ULL << ADDITION_NUMBER_OF_INPUT_NEURONS))
                && (solutionScore != score_engine::INVALID_SCORE_VALUE);
        }
        return false;
    }
    bool isGoodScore(unsigned int solutionScore, int threshold, score_engine::AlgoType selectedAlgo)
    {
        return checkAlgoThreshold(threshold, selectedAlgo) && (solutionScore >= (unsigned int)threshold);
    }

    unsigned int computeScore(const unsigned long long solutionBufIdx, const m256i& publicKey, const m256i& nonce)
    {
        return _computeBuffer[solutionBufIdx].computeScore(publicKey.m256i_u8, nonce.m256i_u8, poolVec);
    }

    m256i getLastOutput(const unsigned long long processor_Number)
    {
        ACQUIRE(solutionEngineLock[processor_Number]);

        m256i result = _computeBuffer[processor_Number].getLastOutput();

        RELEASE(solutionEngineLock[processor_Number]);
        return result;
    }
    // main score function
    unsigned int operator()(const unsigned long long processor_Number, const m256i& publicKey, const m256i& miningSeed, const m256i& nonce)
    {
        PROFILE_SCOPE();

        if (isZero(miningSeed) || miningSeed != currentRandomSeed)
        {
            return score_engine::INVALID_SCORE_VALUE;
        }

        int score = 0;
#if USE_SCORE_CACHE
        unsigned int scoreCacheIndex = scoreCache.getCacheIndex(publicKey, miningSeed, nonce);
        score = scoreCache.tryFetching(publicKey, miningSeed, nonce, scoreCacheIndex);
        if (score >= scoreCache.MIN_VALID_SCORE)
        {
            return score;
        }
        score = 0;
#endif

        const int solutionBufIdx = (int)(processor_Number % solutionBufferCount);
        ACQUIRE(solutionEngineLock[solutionBufIdx]);

        score = computeScore(solutionBufIdx, publicKey, nonce);

        RELEASE(solutionEngineLock[solutionBufIdx]);
#if USE_SCORE_CACHE
        scoreCache.addEntry(publicKey, miningSeed, nonce, scoreCacheIndex, score);
#endif
#ifdef NO_UEFI
        int y = 2 + score;
        stackSize = top_of_stack - ((unsigned long long)(&y));
#endif
        return score;
    }

#ifdef NO_UEFI
    unsigned long long stackSize = 0;
#endif

    // Multithreaded solutions verification:
    // This module mainly serve tick processor in qubic core node, thus the queue size is limited at NUMBER_OF_TRANSACTIONS_PER_TICK 
    // for future use for somewhere else, you can only increase the size.

    volatile char taskQueueLock = 0;
    struct
    {
        m256i publicKey[NUMBER_OF_TRANSACTIONS_PER_TICK];
        m256i miningSeed[NUMBER_OF_TRANSACTIONS_PER_TICK];
        m256i nonce[NUMBER_OF_TRANSACTIONS_PER_TICK];
    } taskQueue;
    unsigned int _nTask;
    unsigned int _nProcessing;
    unsigned int _nFinished;
    bool _nIsTaskQueueReady;

    void resetTaskQueue()
    {
        ACQUIRE(taskQueueLock);
        _nTask = 0;
        _nProcessing = 0;
        _nFinished = 0;
        _nIsTaskQueueReady = false;
        RELEASE(taskQueueLock);
    }

    // add task to the queue
    // queue size is limited at NUMBER_OF_TRANSACTIONS_PER_TICK 
    void addTask(m256i publicKey, m256i miningSeed, m256i nonce)
    {
        ACQUIRE(taskQueueLock);
        if (_nTask < NUMBER_OF_TRANSACTIONS_PER_TICK)
        {
            unsigned int index = _nTask++;
            taskQueue.publicKey[index] = publicKey;
            taskQueue.miningSeed[index] = miningSeed;
            taskQueue.nonce[index] = nonce;
        }
        RELEASE(taskQueueLock);
    }

    void startProcessTaskQueue()
    {
        ACQUIRE(taskQueueLock);
        _nIsTaskQueueReady = true;
        RELEASE(taskQueueLock);
    }

    void stopProcessTaskQueue()
    {
        ACQUIRE(taskQueueLock);
        _nIsTaskQueueReady = false;
        RELEASE(taskQueueLock);
    }

    // get a task, can call on any thread
    bool getTask(m256i* publicKey, m256i* miningSeed, m256i* nonce)
    {
        if (!_nIsTaskQueueReady)
        {
            return false;
        }
        bool result = false;
        ACQUIRE(taskQueueLock);
        if (_nProcessing < _nTask)
        {
            unsigned int index = _nProcessing++;
            *publicKey = taskQueue.publicKey[index];
            *miningSeed = taskQueue.miningSeed[index];
            *nonce = taskQueue.nonce[index];
            result = true;
        }
        else
        {
            result = false;
        }
        RELEASE(taskQueueLock);
        return result;
    }
    void finishTask()
    {
        ACQUIRE(taskQueueLock);
        _nFinished++;
        RELEASE(taskQueueLock);
    }

    bool isTaskQueueProcessed()
    {
        return _nFinished == _nTask;
    }

    void tryProcessSolution(unsigned long long processorNumber)
    {
        m256i publicKey;
        m256i miningSeed;
        m256i nonce;
        bool res = this->getTask(&publicKey, &miningSeed, &nonce);
        if (res)
        {
            (*this)(processorNumber, publicKey, miningSeed, nonce);
            this->finishTask();
        }
    }
};
