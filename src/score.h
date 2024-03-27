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
    int _modNum[257][129];
    // indice pos
    unsigned short _indicePosInput[solutionBufferCount][numberOfInputNeurons + infoLength][dataLength + numberOfInputNeurons + infoLength];
    unsigned short _indicePosOutput[solutionBufferCount][numberOfOutputNeurons + dataLength][infoLength + numberOfOutputNeurons + dataLength];
    
    int nSample;

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
        nSample = 8;
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

    void clampNeuron(long long& val)
    {
        if (val > NEURON_VALUE_LIMIT) {
            val = NEURON_VALUE_LIMIT;
        }
        else if (val < -NEURON_VALUE_LIMIT) {
            val = -NEURON_VALUE_LIMIT;
        }
    }

    void merge(const unsigned short* A, const unsigned short* B, unsigned short* C, const unsigned short lenA, const unsigned short lenB)
    {
        unsigned short lA = 0, lB = 0;
        int count = 0;
        while (lA < lenA && lB < lenB) {
            if (A[lA] < B[lB]) {
                C[count++] = A[lA++];
            }
            else { //guarantee unique
                C[count++] = B[lB++];
            }
        }
        while (lA < lenA) C[count++] = A[lA++];
        while (lB < lenB) C[count++] = B[lB++];
    }

    int mergeSortBucket(unsigned short* indices, const int* bucket, const int* modNum, unsigned short* output, unsigned short* buffer, const int totalModNum)
    {
        if (totalModNum == 1) {
            int mod = modNum[0];
            int start = bucket[mod];
            int len = bucket[mod + 1] - start;
            copyMem(output, indices + start, len * sizeof(unsigned short));
            return len;
        }
        if (totalModNum == 2) {
            int mod = modNum[0];
            int start = bucket[mod];
            unsigned short* seg0 = indices + start;
            unsigned short len0 = bucket[mod + 1] - start;

            mod = modNum[1];
            start = bucket[mod];
            unsigned short* seg1 = indices + start;
            unsigned short len1 = bucket[mod + 1] - start;

            merge(seg0, seg1, output, len0, len1);
            return len0 + len1;
        }
        // max depth = 5
        // it is guaranteed that there is no more than 32 segments if tick <= 256
        static_assert(MAX_INPUT_DURATION <= 256 && MAX_OUTPUT_DURATION <= 256, "Need to increase seg count");
        unsigned short* seg0_buffer[32];
        unsigned short* seg1_buffer[32];
        unsigned short len0_buffer[32];
        unsigned short len1_buffer[32];
        setMem(seg0_buffer, sizeof(seg0_buffer), 0);
        setMem(seg1_buffer, sizeof(seg1_buffer), 0);
        setMem(len0_buffer, sizeof(len0_buffer), 0);
        setMem(len1_buffer, sizeof(len1_buffer), 0);

        unsigned short** seg0 = seg0_buffer;
        unsigned short** seg1 = seg1_buffer;
        unsigned short* len0 = len0_buffer;
        unsigned short* len1 = len1_buffer;
        for (int i = 0; i < totalModNum; i++) {
            int mod = modNum[i];
            int start = bucket[mod];
            seg0[i] = indices + start;
            len0[i] = bucket[mod + 1] - start;
        }

        int nSegment = totalModNum;
        for (int depth = 0; depth < 5; depth++) {
            int newSegCount = 0;
            for (int i = 0; i < nSegment; i += 2) {
                if (i + 1 == nSegment) {
                    seg1[newSegCount] = seg0[i];
                    len1[newSegCount] = len0[i];
                    newSegCount++;
                    continue;
                }
                seg1[newSegCount] = buffer;
                merge(seg0[i], seg0[i + 1], seg1[newSegCount], len0[i], len0[i + 1]);
                len1[newSegCount] = len0[i] + len0[i + 1];
                buffer += len1[newSegCount];
                newSegCount++;
            }
            {
                //swap ptr
                unsigned short ** tmp = seg0;
                seg0 = seg1;
                seg1 = tmp;
            }
            {
                unsigned short * tmp = len0;
                len0 = len1;
                len1 = tmp;
            }
            nSegment = newSegCount;
            if (newSegCount <= 1) { // guaranteed will end up here
                if (len0[0]) {
                    copyMem(output, seg0[0], len0[0] * sizeof(output[0]));
                }
                return len0[0];
            }
        }
        return -1;
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

    void getLastNeurons(const unsigned short* indices, const int* bucket, const int* modNum, unsigned short* topMax,
                        const int nMax, const int totalModNum, int& currentCount, long long* neuron)
    {
        int index[64];
        static_assert(MAX_INPUT_DURATION <= 256 && MAX_OUTPUT_DURATION <= 256, "Need to increase array length");
        setMem(index, sizeof(index), 0);
        while (currentCount < nMax)
        {
            int current_max = -1;
            int max_id = -1;
            for (int i = 0; i < totalModNum; i++) {
                int mod = modNum[i];
                int start = bucket[mod];
                int end = bucket[mod + 1];
                if (start + index[i] < end) {
                    if (indices[end - index[i] - 1] > current_max) {
                        current_max = indices[end - index[i] - 1];
                        max_id = i;
                    }
                }
            }
            if (current_max == -1) return;
            if (neuron[current_max]) topMax[currentCount++] = current_max;
            index[max_id]++;
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
                // pre scan for shortcut
                if (tick > 3)
                {
                    int elemCount = 0;
                    getLastNeurons(indicePosInput[inputNeuronIndex], bucketPosInput[inputNeuronIndex], _modNum[tick], indices,
                        nSample, _totalModNum[tick], elemCount, neurons.input);
                    for (int i = 0; i < elemCount; i++)
                    {
                        unsigned int anotherInputNeuronIndex = indices[i];
                        const unsigned int offset = inputNeuronIndex * (dataLength + numberOfInputNeurons + infoLength) + anotherInputNeuronIndex;
                        if (synapses.inputLength[offset] > 0) {
                            sumBuffer[i] = neurons.input[anotherInputNeuronIndex];
                        }
                        else {
                            sumBuffer[i] = -neurons.input[anotherInputNeuronIndex];
                        }
                    }
                    int found = -1;
                    long long s = 0;
                    for (int i = 0; i < elemCount - 1; i++)
                    {
                        if ((sumBuffer[i] > 0) == (sumBuffer[i + 1] > 0))
                        {
                            found = i + 1;
                            break;
                        }
                    }
                    if (found != -1)
                    {
                        s = 0;
                        for (int i = found; i >= 0; i--)
                        {
                            s += sumBuffer[i];
                            clampNeuron(s);
                        }
                        neurons.input[dataLength + inputNeuronIndex] = s;
                        continue;
                    }
                }
                // full compute
                totalIndice = mergeSortBucket(indicePosInput[inputNeuronIndex], bucketPosInput[inputNeuronIndex], _modNum[tick], indices, (unsigned short*)sumBuffer, _totalModNum[tick]);
                if (totalIndice == 0) continue;

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
                    clampNeuron(neurons.input[dataLength + inputNeuronIndex]);
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
                // pre scan for shortcut
                if (tick > 3)
                {
                    int elemCount = 0;
                    getLastNeurons(indicePosOutput[outputNeuronIndex], bucketPosOutput[outputNeuronIndex], _modNum[tick], indices,
                        nSample, _totalModNum[tick], elemCount, neurons.output);
                    for (int i = 0; i < elemCount; i++)
                    {
                        unsigned int anotherOutputNeuronIndex = indices[i];
                        const unsigned int offset = outputNeuronIndex * (dataLength + numberOfOutputNeurons + infoLength) + anotherOutputNeuronIndex;
                        if (synapses.outputLength[offset] > 0) {
                            sumBuffer[i] = neurons.output[anotherOutputNeuronIndex];
                        }
                        else {
                            sumBuffer[i] = -neurons.output[anotherOutputNeuronIndex];
                        }
                    }
                    int found = -1;
                    long long s = 0;
                    for (int i = 0; i < elemCount - 1; i++)
                    {
                        if ((sumBuffer[i] > 0) == (sumBuffer[i + 1] > 0))
                        {
                            found = i + 1;
                            break;
                        }
                    }
                    if (found != -1)
                    {
                        s = 0;
                        for (int i = found; i >= 0; i--)
                        {
                            s += sumBuffer[i];
                            clampNeuron(s);
                        }
                        neurons.output[infoLength + outputNeuronIndex] = s;
                        continue;
                    }
                }
                {
                    totalIndice = mergeSortBucket(indicePosOutput[outputNeuronIndex], bucketPosOutput[outputNeuronIndex], _modNum[tick], indices, (unsigned short*)sumBuffer, _totalModNum[tick]);
                    if (totalIndice == 0) continue;

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
                        clampNeuron(neurons.output[infoLength + outputNeuronIndex]);
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

        const int solutionBufIdx = (int)(processor_Number % solutionBufferCount);
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
