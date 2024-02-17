#pragma once

#include "platform/memory.h"
#include "platform/m256.h"
#include "platform/concurrency.h"
#include "smart_contracts/math_lib.h"
#include "public_settings.h"

#include "score_cache.h"

template <typename T, T errorValue, int Capacity>
struct minHeapTree
{
    static_assert(Capacity < 1048576, "Heap tree: Max depth is 20");
public:
    T pop()
    {
        T deleteItem;
        if (size == 0) {
            return errorValue;
        }

        deleteItem = buffer[0];
        buffer[0] = buffer[size - 1];
        size--;
        _heapify(0);
        return deleteItem;
    }
    void insert(T data)
    {
        if (size < Capacity) {
            buffer[size] = data;
            _insertHeap(size);
            size++;
        }
    }
    void reset()
    {
        size = 0;
    }
    int getSize()
    {
        return size;
    }
private:
    T buffer[Capacity];
    int size;

    void _heapify(int index)
    {
        if (index < 0) return;
        int left = index * 2 + 1;
        int right = index * 2 + 2;
        int min = index;
        if (left >= size || left < 0)
            left = -1;
        if (right >= size || right < 0)
            right = -1;

        if (left != -1 && buffer[left] < buffer[index])
            min = left;
        if (right != -1 && buffer[right] < buffer[min])
            min = right;

        // Swapping the nodes
        if (min != index) {
            int temp = buffer[min];
            buffer[min] = buffer[index];
            buffer[index] = temp;
            _heapify(min);
        }
    }
    void _insertHeap(int index)
    {
        if (index == 0) return;
        int parent = (index - 1) / 2;
        while (buffer[parent] > buffer[index]) {
            int temp = buffer[parent];
            buffer[parent] = buffer[index];
            buffer[index] = temp;
            index = parent;
            if (index == 0) break;
            parent = (index - 1) / 2;
        }
    }
};

////////// Scoring algorithm \\\\\\\\\\

template<
    unsigned int dataLength,
    unsigned int infoLength,
    unsigned int numberOfInputNeurons,
    unsigned int numberOfOutputNeurons,
    unsigned int maxInputDuration,
    unsigned int maxOutputDuration,
    unsigned int solutionBufferCount
>
struct ScoreFunction
{
    long long miningData[dataLength];
    struct
    {
        long long input[DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + INFO_LENGTH];
        long long output[INFO_LENGTH + NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH];
    } _neurons[solutionBufferCount];
    struct
    {
        char inputLength[(NUMBER_OF_INPUT_NEURONS + INFO_LENGTH) * (DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + INFO_LENGTH)];
        char outputLength[(NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH) * (INFO_LENGTH + NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH)];
    } _synapses[solutionBufferCount];

    // _totalModNum[i]: total of divisible numbers of i
    int _totalModNum[256];
    // i is divisible by _modNum[i][j], j < _totalModNum[i]
    int _modNum[257][256];
    // indice pos
    #define indiceSizeInByte (sizeof(unsigned short) * (long long)(DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + INFO_LENGTH) * (long long)(NUMBER_OF_INPUT_NEURONS + INFO_LENGTH))
    unsigned short _indicePosInput[solutionBufferCount][NUMBER_OF_INPUT_NEURONS + INFO_LENGTH][DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + INFO_LENGTH];
    unsigned short _indicePosOutput[solutionBufferCount][NUMBER_OF_INPUT_NEURONS + INFO_LENGTH][INFO_LENGTH + NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH];

    #define bucketSizeInByte (sizeof(int) * (long long)(DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + INFO_LENGTH) * (long long)(257))

    int _bucketPosInput[solutionBufferCount][DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + INFO_LENGTH][257];
    int _bufferPosInput[solutionBufferCount][DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + INFO_LENGTH][257];    

    int _bucketPosOutput[solutionBufferCount][DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + INFO_LENGTH][257];
    int _bufferPosOutput[solutionBufferCount][DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + INFO_LENGTH][257];

    m256i initialRandomSeed;

    volatile char solutionEngineLock[solutionBufferCount];

#if USE_SCORE_CACHE
    ScoreCache<SCORE_CACHE_SIZE, SCORE_CACHE_COLLISION_RETRIES> scoreCache;
#endif

    void initMiningData(m256i randomSeed)
    {
        initialRandomSeed = randomSeed; // persist the initial random seed to be able to sned it back on system info response
        random((unsigned char*)&randomSeed, (unsigned char*)&randomSeed, (unsigned char*)miningData, sizeof(miningData));
        for (unsigned int i = 0; i < DATA_LENGTH; i++)
        {
            miningData[i] = (miningData[i] >= 0 ? 1 : -1);
        }
        setMem(_totalModNum, sizeof(_totalModNum), 0);
        setMem(_modNum, sizeof(_modNum), 0);

        // init the divisible table
        for (int i = 1; i <= 256; i++) {
            for (int j = -127; j <= 127; j++) {
                if (j && i % j == 0) {
                    _modNum[i][_totalModNum[i]++] = j;
                }
            }
        }
    }

    // Save score cache to SCORE_CACHE_FILE_NAME
    void saveScoreCache()
    {
#if USE_SCORE_CACHE
        scoreCache.save(SCORE_CACHE_FILE_NAME);
#endif
    }

    // Update score cache filename with epoch and try to load file
    bool loadScoreCache(int epoch)
    {
        bool success = true;
#if USE_SCORE_CACHE
        SCORE_CACHE_FILE_NAME[sizeof(SCORE_CACHE_FILE_NAME) / sizeof(SCORE_CACHE_FILE_NAME[0]) - 4] = epoch / 100 + L'0';
        SCORE_CACHE_FILE_NAME[sizeof(SCORE_CACHE_FILE_NAME) / sizeof(SCORE_CACHE_FILE_NAME[0]) - 3] = (epoch % 100) / 10 + L'0';
        SCORE_CACHE_FILE_NAME[sizeof(SCORE_CACHE_FILE_NAME) / sizeof(SCORE_CACHE_FILE_NAME[0]) - 2] = epoch % 10 + L'0';
        success = scoreCache.load(SCORE_CACHE_FILE_NAME);
#endif
        return success;
    }
    // main score function
    unsigned int operator()(const unsigned long long processor_Number, const m256i& publicKey, const m256i& nonce)
    {
        int score = 0;
#if USE_SCORE_CACHE
        unsigned int scoreCacheIndex = scoreCache.getCacheIndex(publicKey, nonce);
        score = scoreCache.tryFetching(publicKey, nonce, scoreCacheIndex);
        if (score >= scoreCache.MIN_VALID_SCORE)
        {
            return score;
        }
        score = 0;
#endif

        const unsigned long long solutionBufIdx = processor_Number % solutionBufferCount;
        ACQUIRE(solutionEngineLock[solutionBufIdx]);

        auto& neurons = _neurons[solutionBufIdx];
        auto& synapses = _synapses[solutionBufIdx];

        auto& indicePosInput = _indicePosInput[solutionBufIdx];
        auto& indicePosOutput = _indicePosOutput[solutionBufIdx];
        auto& bucketPosInput = _bucketPosInput[solutionBufIdx];
        auto& bufferPosInput = _bufferPosInput[solutionBufIdx];
        auto& bucketPosOutput = _bucketPosOutput[solutionBufIdx];
        auto& bufferPosOutput = _bufferPosOutput[solutionBufIdx];

        setMem(&neurons, sizeof(neurons), 0);
        random(publicKey.m256i_u8, nonce.m256i_u8, (unsigned char*)&synapses, sizeof(synapses));

        for (unsigned int inputNeuronIndex = 0; inputNeuronIndex < NUMBER_OF_INPUT_NEURONS + INFO_LENGTH; inputNeuronIndex++)
        {
            for (unsigned int anotherInputNeuronIndex = 0; anotherInputNeuronIndex < DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + INFO_LENGTH; anotherInputNeuronIndex++)
            {
                const unsigned int offset = inputNeuronIndex * (DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + INFO_LENGTH) + anotherInputNeuronIndex;
                if (synapses.inputLength[offset] == -128)
                {
                    synapses.inputLength[offset] = 0;
                }
            }
        }
        for (unsigned int outputNeuronIndex = 0; outputNeuronIndex < NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH; outputNeuronIndex++)
        {
            for (unsigned int anotherOutputNeuronIndex = 0; anotherOutputNeuronIndex < INFO_LENGTH + NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH; anotherOutputNeuronIndex++)
            {
                const unsigned int offset = outputNeuronIndex * (INFO_LENGTH + NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH) + anotherOutputNeuronIndex;
                if (synapses.outputLength[offset] == -128)
                {
                    synapses.outputLength[offset] = 0;
                }
            }
        }
        for (unsigned int inputNeuronIndex = 0; inputNeuronIndex < NUMBER_OF_INPUT_NEURONS + INFO_LENGTH; inputNeuronIndex++)
        {
            synapses.inputLength[inputNeuronIndex * (DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + INFO_LENGTH) + (DATA_LENGTH + inputNeuronIndex)] = 0;
        }
        for (unsigned int outputNeuronIndex = 0; outputNeuronIndex < NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH; outputNeuronIndex++)
        {
            synapses.outputLength[outputNeuronIndex * (INFO_LENGTH + NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH) + (INFO_LENGTH + outputNeuronIndex)] = 0;
        }

        // compute bucket for input
        setMem(bucketPosInput, bucketSizeInByte, 0);
        setMem(bufferPosInput, bucketSizeInByte, 0);
        setMem(indicePosInput, indiceSizeInByte, 0);
        
        for (int i = 0; i < NUMBER_OF_INPUT_NEURONS + INFO_LENGTH; i++) {
            const unsigned int base = i * (INFO_LENGTH + NUMBER_OF_INPUT_NEURONS + DATA_LENGTH);
            for (int j = 0; j < DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + INFO_LENGTH; j++) {
                int v = synapses.inputLength[base + j];
                if (v == 0) continue;
                bucketPosInput[i][v + 128]++;
            }
        }
        // do exclusive sum per row
        for (int i = 0; i < NUMBER_OF_INPUT_NEURONS + INFO_LENGTH; i++) {
            for (int j = 1; j < 257; j++) {
                bufferPosInput[i][j] = bufferPosInput[i][j - 1] + bucketPosInput[i][j - 1];
            }
        }
        copyMem(bucketPosInput, bufferPosInput, sizeof(bucketPosInput));
        // fill indices to index table
        for (int i = 0; i < NUMBER_OF_INPUT_NEURONS + INFO_LENGTH; i++) {
            const unsigned int base = i * (INFO_LENGTH + NUMBER_OF_INPUT_NEURONS + DATA_LENGTH);
            for (int j = 0; j < DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + INFO_LENGTH; j++) {
                int v = synapses.inputLength[base + j];
                if (v == 0) continue;
                v += 128;
                indicePosInput[i][bufferPosInput[i][v]++] = j;
            }
        }

        copyMem(&neurons.input[0], &miningData, sizeof(miningData));
        minHeapTree<int, DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + INFO_LENGTH + 1, DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + INFO_LENGTH> mht;
        for (int tick = 1; tick <= MAX_INPUT_DURATION; tick++) {
            for (unsigned int inputNeuronIndex = 0; inputNeuronIndex < NUMBER_OF_INPUT_NEURONS + INFO_LENGTH; inputNeuronIndex++) {
                {
                    mht.reset();
                    for (int i = 0; i < _totalModNum[tick]; i++) {
                        int mod = _modNum[tick][i] + 128;
                        int start = bucketPosInput[inputNeuronIndex][mod];
                        int end = bucketPosInput[inputNeuronIndex][mod + 1];
                        for (int j = start; j < end; j++) {
                            // TODO: optimize later, for now just use heap tree. Can tree-1 and tree-2 can often be reused 
                            mht.insert(indicePosInput[inputNeuronIndex][j]);
                        }
                    }
                    // compute in correct order
                    while (mht.getSize())
                    {
                        unsigned int anotherInputNeuronIndex = mht.pop();
                        const unsigned int offset = inputNeuronIndex * (DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + INFO_LENGTH) + anotherInputNeuronIndex;
                        if (synapses.inputLength[offset] > 0)
                        {
                            neurons.input[DATA_LENGTH + inputNeuronIndex] += neurons.input[anotherInputNeuronIndex];
                        }
                        else
                        {
                            neurons.input[DATA_LENGTH + inputNeuronIndex] -= neurons.input[anotherInputNeuronIndex];
                        }

                        if (neurons.input[DATA_LENGTH + inputNeuronIndex] > NEURON_VALUE_LIMIT)
                        {
                            neurons.input[DATA_LENGTH + inputNeuronIndex] = NEURON_VALUE_LIMIT;
                        }
                        if (neurons.input[DATA_LENGTH + inputNeuronIndex] <= -NEURON_VALUE_LIMIT)
                        {
                            neurons.input[DATA_LENGTH + inputNeuronIndex] = -NEURON_VALUE_LIMIT + 1;
                        }
                    }
                }
            }
        }

        for (unsigned int i = 0; i < INFO_LENGTH; i++)
        {
            neurons.output[i] = (neurons.input[DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + i] >= 0 ? 1 : -1);
        }

        // compute bucket for output
        setMem(bucketPosOutput, bucketSizeInByte, 0);
        setMem(bufferPosOutput, bucketSizeInByte, 0);
        setMem(indicePosOutput, indiceSizeInByte, 0);

        for (int i = 0; i < NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH; i++) {
            const unsigned int base = i * (INFO_LENGTH + NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH);
            for (int j = 0; j < DATA_LENGTH + NUMBER_OF_OUTPUT_NEURONS + INFO_LENGTH; j++) {
                int v = synapses.outputLength[base + j];
                if (v == 0) continue;
                bucketPosOutput[i][v + 128]++;
            }
        }
        // do exclusive sum per row
        for (int i = 0; i < NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH; i++) {
            for (int j = 1; j < 257; j++) {
                bufferPosOutput[i][j] = bufferPosOutput[i][j - 1] + bucketPosOutput[i][j - 1];
            }
        }
        copyMem(bucketPosOutput, bufferPosOutput, sizeof(bucketPosOutput));
        // fill indices to index table
        for (int i = 0; i < NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH; i++) {
            const unsigned int base = i * (INFO_LENGTH + NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH);
            for (int j = 0; j < DATA_LENGTH + NUMBER_OF_OUTPUT_NEURONS + INFO_LENGTH; j++) {
                int v = synapses.outputLength[base + j];
                if (v == 0) continue;
                v += 128;
                indicePosOutput[i][bufferPosOutput[i][v]++] = j;
            }
        }

        for (int tick = 1; tick <= MAX_OUTPUT_DURATION; tick++) {
            for (unsigned int outputNeuronIndex = 0; outputNeuronIndex < NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH; outputNeuronIndex++) {
                {
                    mht.reset();
                    for (int i = 0; i < _totalModNum[tick]; i++) {
                        int mod = _modNum[tick][i] + 128;
                        int start = bucketPosOutput[outputNeuronIndex][mod];
                        int end = bucketPosOutput[outputNeuronIndex][mod + 1];
                        for (int j = start; j < end; j++) {
                            // TODO: optimize later, for now just use heap tree. Can tree-1 and tree-2 can often be reused 
                            mht.insert(indicePosOutput[outputNeuronIndex][j]);
                        }
                    }
                    // compute in correct order
                    while (mht.getSize())
                    {
                        unsigned int anotherOutputNeuronIndex = mht.pop();
                        const unsigned int offset = outputNeuronIndex * (DATA_LENGTH + NUMBER_OF_OUTPUT_NEURONS + INFO_LENGTH) + anotherOutputNeuronIndex;
                        if (synapses.outputLength[offset] > 0)
                        {
                            neurons.output[INFO_LENGTH + outputNeuronIndex] += neurons.output[anotherOutputNeuronIndex];
                        }
                        else
                        {
                            neurons.output[INFO_LENGTH + outputNeuronIndex] -= neurons.output[anotherOutputNeuronIndex];
                        }

                        if (neurons.output[INFO_LENGTH + outputNeuronIndex] > NEURON_VALUE_LIMIT)
                        {
                            neurons.output[INFO_LENGTH + outputNeuronIndex] = NEURON_VALUE_LIMIT;
                        }
                        if (neurons.output[INFO_LENGTH + outputNeuronIndex] <= -NEURON_VALUE_LIMIT)
                        {
                            neurons.output[INFO_LENGTH + outputNeuronIndex] = -NEURON_VALUE_LIMIT + 1;
                        }
                    }
                }
            }
        }

        for (unsigned int i = 0; i < DATA_LENGTH; i++)
        {
            if ((miningData[i] >= 0) == (neurons.output[INFO_LENGTH + NUMBER_OF_OUTPUT_NEURONS + i] >= 0))
            {
                score++;
            }
        }

        RELEASE(solutionEngineLock[solutionBufIdx]);
#if USE_SCORE_CACHE
        scoreCache.addEntry(publicKey, nonce, scoreCacheIndex, score);
#endif
        return score;
    }

    // Multithreaded solutions verification:
    // This module mainly serve tick processor in qubic core node, thus the queue size is limited at NUMBER_OF_TRANSACTIONS_PER_TICK 
    // for future use for somewhere else, you can only increase the size.
    
    volatile char taskQueueLock = 0;
    struct {
        m256i publicKey[NUMBER_OF_TRANSACTIONS_PER_TICK];
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
    void addTask(m256i publicKey, m256i nonce)
    {   
        ACQUIRE(taskQueueLock);
        if (_nTask < NUMBER_OF_TRANSACTIONS_PER_TICK)
        {
            unsigned int index = _nTask++;
            taskQueue.publicKey[index] = publicKey;
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
    bool getTask(m256i* publicKey, m256i* nonce)
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
        m256i nonce;
        bool res = this->getTask(&publicKey, &nonce);
        if (res)
        {
            (*this)(processorNumber, publicKey, nonce);
            this->finishTask();
        }
    }
};
