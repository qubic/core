#pragma once
#ifdef NO_UEFI
int top_of_stack;
#endif
#include "platform/memory.h"
#include "platform/m256.h"
#include "platform/concurrency.h"
#include "smart_contracts/math_lib.h"
#include "public_settings.h"
#include "score_cache.h"


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
        long long input[dataLength + numberOfInputNeurons + infoLength];
        long long output[infoLength + numberOfOutputNeurons + dataLength];
    } _neurons[solutionBufferCount];
    struct
    {
        char inputLength[(numberOfInputNeurons + infoLength) * (dataLength + numberOfInputNeurons + infoLength)];
        char outputLength[(numberOfOutputNeurons + dataLength) * (infoLength + numberOfOutputNeurons + dataLength)];
    } _synapses[solutionBufferCount];

    static_assert(maxInputDuration <= 256 && maxOutputDuration <= 256, "Need to regenerate mod num table");
    // _totalModNum[i]: total of divisible numbers of i
    int _totalModNum[257];
    // i is divisible by _modNum[i][j], j < _totalModNum[i]
    int _modNum[257][128];
    // indice pos
    unsigned short _indicePosInput[solutionBufferCount][numberOfInputNeurons + infoLength][dataLength + numberOfInputNeurons + infoLength];
    unsigned short _indicePosOutput[solutionBufferCount][numberOfOutputNeurons + dataLength][infoLength + numberOfOutputNeurons + dataLength];

    int _bucketPosInput[solutionBufferCount][numberOfInputNeurons + infoLength][129];
    int _bufferPosInput[solutionBufferCount][numberOfInputNeurons + infoLength][129];

    int _bucketPosOutput[solutionBufferCount][dataLength + numberOfOutputNeurons][129];
    int _bufferPosOutput[solutionBufferCount][dataLength + numberOfOutputNeurons][129];

    long long _sumBuffer[solutionBufferCount][dataLength + numberOfInputNeurons + infoLength];
    unsigned short _indices[solutionBufferCount][dataLength + numberOfInputNeurons + infoLength];

    m256i initialRandomSeed;

    volatile char solutionEngineLock[solutionBufferCount];

#if USE_SCORE_CACHE
    ScoreCache<SCORE_CACHE_SIZE, SCORE_CACHE_COLLISION_RETRIES> scoreCache;
#endif

    void initMiningData(m256i randomSeed)
    {
        initialRandomSeed = randomSeed; // persist the initial random seed to be able to sned it back on system info response
        random((unsigned char*)&randomSeed, (unsigned char*)&randomSeed, (unsigned char*)miningData, sizeof(miningData));
        for (unsigned int i = 0; i < dataLength; i++)
        {
            miningData[i] = (miningData[i] >= 0 ? 1 : -1);
        }
        setMem(_totalModNum, sizeof(_totalModNum), 0);
        setMem(_modNum, sizeof(_modNum), 0);

        // init the divisible table
        for (int i = 1; i <= 256; i++) {
            for (int j = 1; j <= 127; j++) { // exclude 128
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

    template <typename T>
    inline constexpr T abs(const T& a)
    {
        return (a < 0) ? -a : a;
    }

    void generateSynapse(int solutionBufIdx, const m256i& publicKey, const m256i& nonce)
    {
        auto& synapses = _synapses[solutionBufIdx];
        random(publicKey.m256i_u8, nonce.m256i_u8, (unsigned char*)&synapses, sizeof(synapses));

        for (unsigned int inputNeuronIndex = 0; inputNeuronIndex < numberOfInputNeurons + infoLength; inputNeuronIndex++)
        {
            for (unsigned int anotherInputNeuronIndex = 0; anotherInputNeuronIndex < dataLength + numberOfInputNeurons + infoLength; anotherInputNeuronIndex++)
            {
                const unsigned int offset = inputNeuronIndex * (dataLength + numberOfInputNeurons + infoLength) + anotherInputNeuronIndex;
                if (synapses.inputLength[offset] == -128)
                {
                    synapses.inputLength[offset] = 0;
                }
            }
        }
        for (unsigned int outputNeuronIndex = 0; outputNeuronIndex < numberOfOutputNeurons + dataLength; outputNeuronIndex++)
        {
            for (unsigned int anotherOutputNeuronIndex = 0; anotherOutputNeuronIndex < infoLength + numberOfOutputNeurons + dataLength; anotherOutputNeuronIndex++)
            {
                const unsigned int offset = outputNeuronIndex * (infoLength + numberOfOutputNeurons + dataLength) + anotherOutputNeuronIndex;
                if (synapses.outputLength[offset] == -128)
                {
                    synapses.outputLength[offset] = 0;
                }
            }
        }
        for (unsigned int inputNeuronIndex = 0; inputNeuronIndex < numberOfInputNeurons + infoLength; inputNeuronIndex++)
        {
            synapses.inputLength[inputNeuronIndex * (dataLength + numberOfInputNeurons + infoLength) + (dataLength + inputNeuronIndex)] = 0;
        }
        for (unsigned int outputNeuronIndex = 0; outputNeuronIndex < numberOfOutputNeurons + dataLength; outputNeuronIndex++)
        {
            synapses.outputLength[outputNeuronIndex * (infoLength + numberOfOutputNeurons + dataLength) + (infoLength + outputNeuronIndex)] = 0;
        }
    }

    void computeInputBucket(int solutionBufIdx)
    {
        auto& synapses = _synapses[solutionBufIdx];
        auto& indicePosInput = _indicePosInput[solutionBufIdx];
        auto& bucketPosInput = _bucketPosInput[solutionBufIdx];
        auto& bufferPosInput = _bufferPosInput[solutionBufIdx];
        // compute bucket for input
        setMem(bucketPosInput, sizeof(bucketPosInput), 0);
        setMem(bufferPosInput, sizeof(bufferPosInput), 0);
        setMem(indicePosInput, sizeof(indicePosInput), 0);

        for (int i = 0; i < numberOfInputNeurons + infoLength; i++) {
            const unsigned int base = i * (infoLength + numberOfInputNeurons + dataLength);
            for (int j = 0; j < dataLength + numberOfInputNeurons + infoLength; j++) {
                int v = synapses.inputLength[base + j];
                if (v == 0) continue;
                v = abs(v);
                bucketPosInput[i][v]++;
            }
        }
        // do exclusive sum per row
        for (int i = 0; i < numberOfInputNeurons + infoLength; i++) {
            for (int j = 1; j <= 128; j++) {
                bufferPosInput[i][j] = bufferPosInput[i][j - 1] + bucketPosInput[i][j - 1];
            }
        }
        copyMem(bucketPosInput, bufferPosInput, sizeof(bucketPosInput));
        // fill indices to index table
        for (int i = 0; i < numberOfInputNeurons + infoLength; i++) {
            const unsigned int base = i * (infoLength + numberOfInputNeurons + dataLength);
            for (int j = 0; j < dataLength + numberOfInputNeurons + infoLength; j++) {
                int v = synapses.inputLength[base + j];
                if (v == 0) continue;
                v = abs(v);
                indicePosInput[i][bufferPosInput[i][v]++] = j;
            }
        }
    }

    void computeInputNeuron(int solutionBufIdx)
    {
        auto& neurons = _neurons[solutionBufIdx];
        auto& synapses = _synapses[solutionBufIdx];
        auto& indicePosInput = _indicePosInput[solutionBufIdx];
        auto& bucketPosInput = _bucketPosInput[solutionBufIdx];
        auto& bufferPosInput = _bufferPosInput[solutionBufIdx];
        auto& sumBuffer = _sumBuffer[solutionBufIdx];
        auto& indices = _indices[solutionBufIdx];

        copyMem(&neurons.input[0], miningData, sizeof(miningData));
        int totalIndice;
        for (int tick = 1; tick <= maxInputDuration; tick++) {
            for (unsigned int inputNeuronIndex = 0; inputNeuronIndex < numberOfInputNeurons + infoLength; inputNeuronIndex++) {
                {
                    totalIndice = 0;
                    for (int i = 0; i < _totalModNum[tick]; i++) {
                        int mod = _modNum[tick][i];
                        int start = bucketPosInput[inputNeuronIndex][mod];
                        int end = bucketPosInput[inputNeuronIndex][mod + 1];
                        if (end - start > 0) {
                            copyMem(indices + totalIndice, indicePosInput[inputNeuronIndex] + start, sizeof(unsigned short) * (end - start));
                            totalIndice += end - start;
                        }
                    }

                    for (int i = 1; i < totalIndice; i++) {
                        unsigned short key = indices[i];
                        int j = i - 1;
                        while (j >= 0 && indices[j] > key) {
                            indices[j + 1] = indices[j];
                            j = j - 1;
                        }
                        indices[j + 1] = key;
                    }

                    for (int i = 0; i < totalIndice; i++) {
                        unsigned int anotherInputNeuronIndex = indices[i];
                        const unsigned int offset = inputNeuronIndex * (dataLength + numberOfInputNeurons + infoLength) + anotherInputNeuronIndex;
                        if (synapses.inputLength[offset] > 0) {
                            sumBuffer[i] = neurons.input[anotherInputNeuronIndex];
                        }
                        else {
                            sumBuffer[i] = -neurons.input[anotherInputNeuronIndex];
                        }
                    }
                    for (int i = 0; i < totalIndice; i++)
                    {
                        neurons.input[dataLength + inputNeuronIndex] += sumBuffer[i];
                        if (neurons.input[dataLength + inputNeuronIndex] > NEURON_VALUE_LIMIT)
                        {
                            neurons.input[dataLength + inputNeuronIndex] = NEURON_VALUE_LIMIT;
                        }
                        if (neurons.input[dataLength + inputNeuronIndex] <= -NEURON_VALUE_LIMIT)
                        {
                            neurons.input[dataLength + inputNeuronIndex] = -NEURON_VALUE_LIMIT + 1;
                        }
                    }
                }
            }
        }
    }

    void computeOutputBucket(int solutionBufIdx)
    {
        auto& synapses = _synapses[solutionBufIdx];
        auto& indicePosOutput = _indicePosOutput[solutionBufIdx];
        auto& bucketPosOutput = _bucketPosOutput[solutionBufIdx];
        auto& bufferPosOutput = _bufferPosOutput[solutionBufIdx];
        // compute bucket for output
        setMem(bucketPosOutput, sizeof(bucketPosOutput), 0);
        setMem(bufferPosOutput, sizeof(bufferPosOutput), 0);
        setMem(indicePosOutput, sizeof(indicePosOutput), 0);

        for (int i = 0; i < numberOfOutputNeurons + dataLength; i++) {
            const unsigned int base = i * (infoLength + numberOfOutputNeurons + dataLength);
            for (int j = 0; j < dataLength + numberOfOutputNeurons + infoLength; j++) {
                int v = synapses.outputLength[base + j];
                if (v == 0) continue;
                v = abs(v);
                bucketPosOutput[i][v]++;
            }
        }
        // do exclusive sum per row
        for (int i = 0; i < numberOfOutputNeurons + dataLength; i++) {
            for (int j = 1; j <= 128; j++) {
                bufferPosOutput[i][j] = bufferPosOutput[i][j - 1] + bucketPosOutput[i][j - 1];
            }
        }
        copyMem(bucketPosOutput, bufferPosOutput, sizeof(bucketPosOutput));
        // fill indices to index table
        for (int i = 0; i < numberOfOutputNeurons + dataLength; i++) {
            const unsigned int base = i * (infoLength + numberOfOutputNeurons + dataLength);
            for (int j = 0; j < dataLength + numberOfOutputNeurons + infoLength; j++) {
                int v = synapses.outputLength[base + j];
                if (v == 0) continue;
                v = abs(v);
                indicePosOutput[i][bufferPosOutput[i][v]++] = j;
            }
        }
    }

    void computeOutputNeuron(int solutionBufIdx)
    {
        auto& neurons = _neurons[solutionBufIdx];
        auto& synapses = _synapses[solutionBufIdx];

        auto& indicePosOutput = _indicePosOutput[solutionBufIdx];
        auto& bucketPosOutput = _bucketPosOutput[solutionBufIdx];
        auto& bufferPosOutput = _bufferPosOutput[solutionBufIdx];
        auto& sumBuffer = _sumBuffer[solutionBufIdx];
        auto& indices = _indices[solutionBufIdx];
        int totalIndice;
        for (int tick = 1; tick <= maxOutputDuration; tick++) {
            for (unsigned int outputNeuronIndex = 0; outputNeuronIndex < numberOfOutputNeurons + dataLength; outputNeuronIndex++) {
                {
                    totalIndice = 0;
                    for (int i = 0; i < _totalModNum[tick]; i++) {
                        int mod = _modNum[tick][i];
                        int start = bucketPosOutput[outputNeuronIndex][mod];
                        int end = bucketPosOutput[outputNeuronIndex][mod + 1];
                        if (end - start > 0) {
                            copyMem(indices + totalIndice, indicePosOutput[outputNeuronIndex] + start, sizeof(unsigned short) * (end - start));
                            totalIndice += end - start;
                        }
                    }

                    for (int i = 1; i < totalIndice; i++) {
                        unsigned short key = indices[i];
                        int j = i - 1;
                        while (j >= 0 && indices[j] > key) {
                            indices[j + 1] = indices[j];
                            j = j - 1;
                        }
                        indices[j + 1] = key;
                    }

                    for (int i = 0; i < totalIndice; i++) {
                        unsigned int anotherOutputNeuronIndex = indices[i];
                        const unsigned int offset = outputNeuronIndex * (dataLength + numberOfOutputNeurons + infoLength) + anotherOutputNeuronIndex;
                        if (synapses.outputLength[offset] > 0) {
                            sumBuffer[i] = neurons.output[anotherOutputNeuronIndex];
                        }
                        else {
                            sumBuffer[i] = -neurons.output[anotherOutputNeuronIndex];
                        }
                    }

                    for (int i = 0; i < totalIndice; i++)
                    {
                        neurons.output[infoLength + outputNeuronIndex] += sumBuffer[i];
                        if (neurons.output[infoLength + outputNeuronIndex] > NEURON_VALUE_LIMIT)
                        {
                            neurons.output[infoLength + outputNeuronIndex] = NEURON_VALUE_LIMIT;
                        }
                        if (neurons.output[infoLength + outputNeuronIndex] <= -NEURON_VALUE_LIMIT)
                        {
                            neurons.output[infoLength + outputNeuronIndex] = -NEURON_VALUE_LIMIT + 1;
                        }
                    }
                }
            }
        }
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

        setMem(&neurons.input[0], sizeof(neurons), 0);
        generateSynapse(solutionBufIdx, publicKey, nonce);

        computeInputBucket(solutionBufIdx);

        computeInputNeuron(solutionBufIdx);

        for (unsigned int i = 0; i < infoLength; i++)
        {
            neurons.output[i] = (neurons.input[dataLength + numberOfInputNeurons + i] >= 0 ? 1 : -1);
        }

        computeOutputBucket(solutionBufIdx);

        computeOutputNeuron(solutionBufIdx);

        for (unsigned int i = 0; i < dataLength; i++)
        {
            if ((miningData[i] >= 0) == (neurons.output[infoLength + numberOfOutputNeurons + i] >= 0))
            {
                score++;
            }
        }

        RELEASE(solutionEngineLock[solutionBufIdx]);
#if USE_SCORE_CACHE
        scoreCache.addEntry(publicKey, nonce, scoreCacheIndex, score);
#endif
#ifdef NO_UEFI
        int y = 2 + score;
        int ss = top_of_stack - ((int)(&y));
        std::cout << "Stack size: " << ss << " bytes\n";
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
