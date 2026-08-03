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

// Operational status of the scorer, surfaced to the main thread for reporting. Extend with new error
// kinds (e.g. an invalid/rejected task, a corrupted pool) as the engine gains more failure modes.
enum ScoreStatus
{
    ScoreStatusOk = 0,
    ScoreStatusTaskNotLoaded,
};

namespace score_engine
{
    using Bpp9000ParamsT = Bpp9000Params<
        BPP9000_NUMBER_OF_INPUT_NEURONS,
        BPP9000_NUMBER_OF_OUTPUT_NEURONS,
        BPP9000_SEQUENCE_LENGTH,
        BPP9000_WINDOW_WIDTH,
        BPP9000_MAX_NUMBER_OF_TICKS,
        BPP9000_NUMBER_OF_NEIGHBORS,
        BPP9000_POPULATION_THRESHOLD,
        BPP9000_NUMBER_OF_MUTATIONS,
        BPP9000_SOLUTION_THRESHOLD_DEFAULT>;

    using NeuraxonParamsT = NeuraxonParams<
        NEURAXON_NUMBER_OF_INPUT_NEURONS,
        NEURAXON_NUMBER_OF_OUTPUT_NEURONS,
        NEURAXON_NUMBER_OF_TICKS,
        NEURAXON_NUMBER_OF_NEIGHBORS,
        NEURAXON_POPULATION_THRESHOLD,
        NEURAXON_NUMBER_OF_MUTATIONS,
        NEURAXON_SOLUTION_THRESHOLD_DEFAULT>;

    // The bpp9000 scorer the ant colony branches on; exposes ANN (the inheritable per-neuron LUT).
    using ScoreBpp9000T = ScoreBpp9000<Bpp9000ParamsT>;

    using ScoreEngineT = ScoreEngine<NeuraxonParamsT, Bpp9000ParamsT>;
}

template <unsigned long long solutionBufferCount>
struct ScoreFunction
{
private:
    // The engine scratch buffers and the locks guarding them. Private on purpose: a work function run
    // by the task queue cannot reach them, so it cannot take a slot lock and then call a method that
    // takes the same one. Every route into the engine locks exactly once, inside this class.
    score_engine::ScoreEngineT _computeBuffer[solutionBufferCount];
    volatile char solutionEngineLock[solutionBufferCount];

public:
    volatile char random2PoolLock;
    unsigned char state[score_engine::STATE_SIZE];
    unsigned char externalPoolVec[score_engine::POOL_VEC_PADDING_SIZE];
    unsigned char poolVec[score_engine::POOL_VEC_PADDING_SIZE];

    // Last operational status of the scorer
    volatile ScoreStatus _lastStatus;

    ScoreStatus getLastStatus() const
    {
        return _lastStatus;
    }

    void initPool(const unsigned char* miningSeed)
    {
        // Init random2 pool with mining seed
        score_engine::generateRandom2Pool(miningSeed, state, externalPoolVec);
    }

    m256i currentRandomSeed;

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

    // Load the task blocks into every compute buffer; returns false if any leaf rejects them.
    bool loadTask(const unsigned char* topoBlock, const unsigned char* dataBlock)
    {
        bool ok = true;
        for (unsigned long long i = 0; i < solutionBufferCount; i++)
        {
            ok = _computeBuffer[i].loadTask(topoBlock, dataBlock) && ok;
        }
        _lastStatus = ok ? ScoreStatusOk : ScoreStatusTaskNotLoaded;
        return ok;
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
        _lastStatus = ScoreStatusTaskNotLoaded;

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
        if (selectedAlgo == score_engine::AlgoType::Bpp9000)
        {
            return (solutionScore <= BPP9000_NUMBER_OF_WINDOWS)
                && (solutionScore != score_engine::INVALID_SCORE_VALUE);
        }
        // Neuraxon slot is reserved and not yet minable.
        return false;
    }
    // Score is an error count, so a solution is good when it is at or below the threshold.
    bool isGoodScore(unsigned int solutionScore, int threshold, score_engine::AlgoType selectedAlgo)
    {
        return checkAlgoThreshold(threshold, selectedAlgo) && (solutionScore <= (unsigned int)threshold);
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

        // TODO: When neuraxon's going, this check need to be modified
        if (!score_engine::isCanonicalBpp9000Nonce(nonce.m256i_u8))
        {
            return score_engine::INVALID_SCORE_VALUE;
        }

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

    // Multithreaded solutions verification.
    //
    // A task is a (work function, payload) pair rather than a fixed tuple, so different kinds of
    // scoring work can share one queue and one drain: the queue arbitrates nothing except who runs
    // next. The payload is COPIED in, so the caller may reuse or discard its buffer immediately - a
    // pointer here would make every caller responsible for keeping data alive across a drain.
    //
    // The work function is responsible for taking whatever locks it needs, including
    // solutionEngineLock. The queue must not take it: solutionEngineLock is a non-reentrant spinlock
    // and operator() takes it itself, so a queue that pre-acquired would deadlock any work function
    // that reuses operator().
    typedef void (*WorkFunc)(unsigned long long processorNumber, void* payload);

    static constexpr unsigned int TASK_PAYLOAD_MAX = 128;

private:
    static constexpr unsigned int TASK_QUEUE_CAPACITY = NUMBER_OF_TRANSACTIONS_PER_TICK;

    struct Task
    {
        WorkFunc func;
        // 8-byte aligned: m256i is a plain union accessed with unaligned intrinsics, so it needs no more.
        unsigned long long payload[TASK_PAYLOAD_MAX / sizeof(unsigned long long)];
    };

    volatile char taskQueueLock = 0;
    Task taskQueue[TASK_QUEUE_CAPACITY];
    unsigned int _nTask;
    unsigned int _nProcessing;
    unsigned int _nFinished;
    volatile bool _nIsTaskQueueReady;

public:
    void resetTaskQueue()
    {
        ACQUIRE(taskQueueLock);
        _nTask = 0;
        _nProcessing = 0;
        _nFinished = 0;
        _nIsTaskQueueReady = false;
        RELEASE(taskQueueLock);
    }

    // Copies size bytes of data. Returns false if the queue is full or the payload does not fit.
    bool addTask(WorkFunc func, const void* data, unsigned int size)
    {
        if (size > TASK_PAYLOAD_MAX)
        {
            return false;
        }
        bool added = false;
        ACQUIRE(taskQueueLock);
        if (_nTask < TASK_QUEUE_CAPACITY)
        {
            Task& t = taskQueue[_nTask++];
            t.func = func;
            copyMem(t.payload, data, size);
            added = true;
        }
        RELEASE(taskQueueLock);
        return added;
    }

    // Outcome of one dispatch attempt, so a caller waiting for the batch does not need a second
    // lock acquisition just to ask whether it is over.
    enum TaskDispatchResult
    {
        TaskRan,         // a task was taken and executed
        TaskNonePending, // nothing left to take, but tasks are still running elsewhere
        TaskAllDone      // every queued task has finished
    };

    // Run one task if any is pending. Called from request processors' idle path and from the drain.
    TaskDispatchResult tryProcessOneTask(unsigned long long processorNumber)
    {
        if (!_nIsTaskQueueReady)
        {
            // No thing to process
            return TaskNonePending;
        }

        WorkFunc func = nullptr;
        unsigned long long payload[TASK_PAYLOAD_MAX / sizeof(unsigned long long)];
        TaskDispatchResult result = TaskNonePending;

        ACQUIRE(taskQueueLock);
        if (_nFinished >= _nTask)
        {
            result = TaskAllDone;
        }
        else if (_nIsTaskQueueReady && _nProcessing < _nTask)
        {
            const Task& t = taskQueue[_nProcessing++];
            func = t.func;
            copyMem(payload, t.payload, TASK_PAYLOAD_MAX);
            result = TaskRan;
        }
        RELEASE(taskQueueLock);

        if (func == nullptr)
        {
            return result;
        }
        func(processorNumber, payload);

        ACQUIRE(taskQueueLock);
        _nFinished++;
        RELEASE(taskQueueLock);
        return result;
    }

    // Open the queue and work it down. The caller participates rather than spinning idle, and returns
    // only once every task has finished - including those running on other threads
    void runUntilDone(unsigned long long processorNumber)
    {
        ACQUIRE(taskQueueLock);
        _nIsTaskQueueReady = true;
        RELEASE(taskQueueLock);

        // Wait for task queue finish
        for (;;)
        {
            const TaskDispatchResult result = tryProcessOneTask(processorNumber);
            if (result == TaskAllDone)
            {
                break;
            }
            if (result == TaskNonePending)
            {
                _mm_pause();
            }
        }

        ACQUIRE(taskQueueLock);
        _nIsTaskQueueReady = false;
        RELEASE(taskQueueLock);
    }
};
