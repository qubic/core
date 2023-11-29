#pragma once

#include "platform/memory.h"
#include "platform/m256.h"
#include "platform/concurrency.h"
#include "platform/file_io.h"
#include "platform/logging.h"
#include "platform/time_stamp_counter.h"
#include "kangaroo_twelve.h"

////////// Scoring algorithm \\\\\\\\\\

template<
    unsigned int dataLength,
    unsigned int infoLength,
    unsigned int numberOfInputNeutrons,
    unsigned int numberOfOutputNeutrons,
    unsigned int maxInputDuration,
    unsigned int maxOutputDuration,
    unsigned int maxNumberOfProcessors,
    unsigned int solutionBufferCount = 8
>
struct ScoreFunction
{
    int miningData[dataLength];
    struct
    {
        int input[dataLength + numberOfInputNeutrons + infoLength];
        int output[infoLength + numberOfInputNeutrons + dataLength];
    } neurons[solutionBufferCount];
    struct
    {
        char input[(numberOfInputNeutrons + infoLength) * (dataLength + numberOfInputNeutrons + infoLength)];
        char output[(numberOfInputNeutrons + dataLength) * (infoLength + numberOfInputNeutrons + dataLength)];
        unsigned short lengths[maxInputDuration * (numberOfInputNeutrons + infoLength) + MAX_OUTPUT_DURATION * (numberOfInputNeutrons + dataLength)];
    } synapses[solutionBufferCount];

    struct
    {
        char input_positive[(numberOfInputNeutrons + infoLength) * (dataLength + numberOfInputNeutrons + infoLength) / 8];
        char input_negative[(numberOfInputNeutrons + infoLength) * (dataLength + numberOfInputNeutrons + infoLength) / 8];
        char output_positive[(numberOfInputNeutrons + dataLength) * (infoLength + numberOfInputNeutrons + dataLength) / 8];
        char output_negative[(numberOfInputNeutrons + dataLength) * (infoLength + numberOfInputNeutrons + dataLength) / 8];
    } synapses1Bit[solutionBufferCount];

    volatile char solutionEngineLock[solutionBufferCount];

#if USE_SCORE_CACHE
    struct
    {
        m256i publicKey;
        m256i nonce;
        int score;
    } scoreCache[SCORE_CACHE_SIZE]; // set zero or load from a file on init

    volatile char scoreCacheLock;

    unsigned int scoreCacheHit = 0;
    unsigned int scoreCacheMiss = 0;
    unsigned int scoreCacheUnknown = 0;

    unsigned int getScoreCacheIndex(const m256i& publicKey, const m256i& nonce)
    {
        m256i buffer[2] = { publicKey, nonce };
        unsigned char digest[32];
        KangarooTwelve64To32(buffer, digest);
        unsigned int result = *((unsigned long long*)digest) % SCORE_CACHE_SIZE;

        return result;
    }

    int tryFetchingScoreCache(const m256i& publicKey, const m256i& nonce, unsigned int scoreCacheIndex)
    {
        ACQUIRE(scoreCacheLock);
        const m256i& cachedPublicKey = scoreCache[scoreCacheIndex].publicKey;
        const m256i& cachedNonce = scoreCache[scoreCacheIndex].nonce;
        int retVal;
        if (isZero(cachedPublicKey))
        {
            scoreCacheUnknown++;
            retVal = -1;
        }
        else if (cachedPublicKey == publicKey && cachedNonce == nonce)
        {
            scoreCacheHit++;
            retVal = scoreCache[scoreCacheIndex].score;
        }
        else
        {
            scoreCacheMiss++;
            retVal = -1;
        }
        RELEASE(scoreCacheLock);
        return retVal;
    }

    void addScoreCache(const m256i& publicKey, const m256i& nonce, unsigned int scoreCacheIndex, int score)
    {
        ACQUIRE(scoreCacheLock);
        scoreCache[scoreCacheIndex].publicKey = publicKey;
        scoreCache[scoreCacheIndex].nonce = nonce;
        scoreCache[scoreCacheIndex].score = score;
        RELEASE(scoreCacheLock);
    }

    void initEmptyScoreCache()
    {
        ACQUIRE(scoreCacheLock);
        setMem((unsigned char*)scoreCache, sizeof(scoreCache), 0);
        RELEASE(scoreCacheLock);
    }
#endif

    void initMiningData()
    {
        unsigned char randomSeed[32];
        setMem(randomSeed, 32, 0);
        randomSeed[0] = RANDOM_SEED0;
        randomSeed[1] = RANDOM_SEED1;
        randomSeed[2] = RANDOM_SEED2;
        randomSeed[3] = RANDOM_SEED3;
        randomSeed[4] = RANDOM_SEED4;
        randomSeed[5] = RANDOM_SEED5;
        randomSeed[6] = RANDOM_SEED6;
        randomSeed[7] = RANDOM_SEED7;
        random(randomSeed, randomSeed, (unsigned char*)miningData, sizeof(miningData));
    }

    // Save score cache to SCORE_CACHE_FILE_NAME
    void saveScoreCache()
    {
#if USE_SCORE_CACHE
        const unsigned long long beginningTick = __rdtsc();
        ACQUIRE(scoreCacheLock);
        long long savedSize = save(SCORE_CACHE_FILE_NAME, sizeof(scoreCache), (unsigned char*)&scoreCache);
        RELEASE(scoreCacheLock);
        if (savedSize == sizeof(scoreCache))
        {
            setNumber(message, savedSize, TRUE);
            appendText(message, L" bytes of the score cache data are saved (");
            appendNumber(message, (__rdtsc() - beginningTick) * 1000000 / frequency, TRUE);
            appendText(message, L" microseconds).");
            log(message);
        }
#endif
    }

    // Update score cache filename with epoch and try to load file
    bool loadScoreCache(int epoch)
    {
        bool success = true;
#if USE_SCORE_CACHE
        setText(message, L"Loading score cache...");
        log(message);
        SCORE_CACHE_FILE_NAME[sizeof(SCORE_CACHE_FILE_NAME) / sizeof(SCORE_CACHE_FILE_NAME[0]) - 4] = epoch / 100 + L'0';
        SCORE_CACHE_FILE_NAME[sizeof(SCORE_CACHE_FILE_NAME) / sizeof(SCORE_CACHE_FILE_NAME[0]) - 3] = (epoch % 100) / 10 + L'0';
        SCORE_CACHE_FILE_NAME[sizeof(SCORE_CACHE_FILE_NAME) / sizeof(SCORE_CACHE_FILE_NAME[0]) - 2] = epoch % 10 + L'0';
        // init, set zero all scorecache
        initEmptyScoreCache();
        ACQUIRE(scoreCacheLock);
        long long loadedSize = load(SCORE_CACHE_FILE_NAME, sizeof(scoreCache), (unsigned char*)scoreCache);
        RELEASE(scoreCacheLock);
        if (loadedSize != sizeof(scoreCache))
        {
            if (loadedSize == -1)
            {
                setText(message, L"Error while loading score cache: File does not exists (ignore this error if this is the epoch start)");
            }
            else if (loadedSize < sizeof(scoreCache))
            {
                setText(message, L"Error while loading score cache: Score cache file is smaller than defined. System may not work properly");
            }
            else
            {
                setText(message, L"Error while loading score cache: Score cache file is larger than defined. System may not work properly");
            }
            success = false;
        }
        else
        {
            setText(message, L"Loaded score cache data!");
        }
        log(message);
#endif
        return success;
    }

    static void setBitNeuron(unsigned char* A, int idx) {
        int m = idx & 7;
        idx = idx >> 3;
        A[idx] |= ((unsigned char)1 << m);
    }
    static void clearBitNeuron(unsigned char* A, int idx) {
        int m = idx & 7;
        idx = idx >> 3;
        A[idx] &= ~((unsigned char)1 << m);
    }
    static void neuronU64To1Bit(unsigned long long u64, unsigned char* positive, unsigned char* negative) {
        for (int m = 0; m < 8; m++)
        {
            char val = char(((u64 >> (m * 8)) & 0xff) % 3) - 1;
            *positive &= ~((unsigned char)1 << m); // clear bit
            *positive |= ((unsigned char)(val > 0) << m); // set bit
            *negative &= ~((unsigned char)1 << m); // clear bit
            *negative |= ((unsigned char)(val < 0) << m); // set bit
        }
    }

    // main score function
    unsigned int operator()(const unsigned long long processor_Number, const m256i& publicKey, const m256i& nonce)
    {
        int score = 0;
#if USE_SCORE_CACHE
        unsigned int scoreCacheIndex = getScoreCacheIndex(publicKey, nonce);
        score = tryFetchingScoreCache(publicKey, nonce, scoreCacheIndex);
        if (score != -1)
        {
            return score;
        }
        score = 0;
#endif

        const unsigned long long solutionBufIdx = processor_Number % solutionBufferCount;
        ACQUIRE(solutionEngineLock[solutionBufIdx]);
        constexpr unsigned int SYNAPSE_CHUNK_SIZE = (dataLength + numberOfInputNeutrons + infoLength);
        constexpr unsigned int SYNAPSE_CHUNK_SIZE_BIT = (SYNAPSE_CHUNK_SIZE >> 3);
        constexpr unsigned int SYNAPSE_CHUNK_SIZE_BIT_ROUND_UP = (((SYNAPSE_CHUNK_SIZE_BIT + 7) >> 3) << 3);
        constexpr unsigned int NEURON_SCANNED_ROUND = (SYNAPSE_CHUNK_SIZE_BIT_ROUND_UP >> 3);
        constexpr unsigned int SYNAPSE_MISSING_BYTES = (8 - (SYNAPSE_CHUNK_SIZE_BIT % 8));
        constexpr unsigned long long LAST_ELEMENT_MASK = (0xFFFFFFFFFFFFFFFFULL >> (SYNAPSE_MISSING_BYTES * 8));
        unsigned char nrVal1Bit[SYNAPSE_CHUNK_SIZE_BIT_ROUND_UP];
        random(publicKey.m256i_u8, nonce.m256i_u8, (unsigned char*)&synapses[solutionBufIdx], sizeof(synapses[0]));

        for (unsigned int inputNeuronIndex = 0; inputNeuronIndex < numberOfInputNeutrons + infoLength; inputNeuronIndex++)
        {
            unsigned long long* p = (unsigned long long*)(synapses[solutionBufIdx].input + inputNeuronIndex * (dataLength + numberOfInputNeutrons + infoLength));
            for (unsigned int anotherInputNeuronIndex = 0; anotherInputNeuronIndex < (dataLength + numberOfInputNeutrons + infoLength) / 8; anotherInputNeuronIndex++)
            {
                const unsigned int offset = inputNeuronIndex * (dataLength + numberOfInputNeutrons + infoLength) / 8 + anotherInputNeuronIndex;
                neuronU64To1Bit(p[anotherInputNeuronIndex], (unsigned char*)synapses1Bit[solutionBufIdx].input_positive + offset,
                    (unsigned char*)synapses1Bit[solutionBufIdx].input_negative + offset);
            }
        }
        for (unsigned int outputNeuronIndex = 0; outputNeuronIndex < numberOfInputNeutrons + dataLength; outputNeuronIndex++)
        {
            unsigned long long* p = (unsigned long long*)(synapses[solutionBufIdx].output + outputNeuronIndex * (dataLength + numberOfInputNeutrons + infoLength));
            for (unsigned int anotherOutputNeuronIndex = 0; anotherOutputNeuronIndex < (infoLength + numberOfInputNeutrons + dataLength) / 8; anotherOutputNeuronIndex++)
            {
                const unsigned int offset = outputNeuronIndex * (infoLength + numberOfInputNeutrons + dataLength) / 8 + anotherOutputNeuronIndex;
                neuronU64To1Bit(p[anotherOutputNeuronIndex], (unsigned char*)synapses1Bit[solutionBufIdx].output_positive + offset,
                    (unsigned char*)synapses1Bit[solutionBufIdx].output_negative + offset);
            }
        }

        for (unsigned int inputNeuronIndex = 0; inputNeuronIndex < numberOfInputNeutrons + infoLength; inputNeuronIndex++)
        {
            clearBitNeuron((unsigned char*)synapses1Bit[solutionBufIdx].input_positive, inputNeuronIndex * (dataLength + numberOfInputNeutrons + infoLength) + (dataLength + inputNeuronIndex));
            clearBitNeuron((unsigned char*)synapses1Bit[solutionBufIdx].input_negative, inputNeuronIndex * (dataLength + numberOfInputNeutrons + infoLength) + (dataLength + inputNeuronIndex));
        }
        for (unsigned int outputNeuronIndex = 0; outputNeuronIndex < numberOfInputNeutrons + dataLength; outputNeuronIndex++)
        {
            clearBitNeuron((unsigned char*)synapses1Bit[solutionBufIdx].output_positive, outputNeuronIndex * (infoLength + numberOfInputNeutrons + dataLength) + (infoLength + outputNeuronIndex));
            clearBitNeuron((unsigned char*)synapses1Bit[solutionBufIdx].output_negative, outputNeuronIndex * (infoLength + numberOfInputNeutrons + dataLength) + (infoLength + outputNeuronIndex));
        }

        unsigned int lengthIndex = 0;

        copyMem(&neurons[solutionBufIdx].input[0], miningData, dataLength * sizeof(miningData[0]));
        setMem(&neurons[solutionBufIdx].input[sizeof(miningData) / sizeof(neurons[0].input[0])], sizeof(neurons[0]) - sizeof(miningData), 0);
        setMem(nrVal1Bit, sizeof(nrVal1Bit), 0);

        for (int i = 0; i < dataLength + numberOfInputNeutrons + infoLength; i++) {
            if (neurons[solutionBufIdx].input[i] < 0) {
                setBitNeuron(nrVal1Bit, i);
            }
            else {
                clearBitNeuron(nrVal1Bit, i);
            }
        }

        for (unsigned int tick = 0; tick < maxInputDuration; tick++)
        {
            unsigned short neuronIndices[numberOfInputNeutrons + infoLength];
            unsigned short numberOfRemainingNeurons = 0;
            for (numberOfRemainingNeurons = 0; numberOfRemainingNeurons < numberOfInputNeutrons + infoLength; numberOfRemainingNeurons++)
            {
                neuronIndices[numberOfRemainingNeurons] = numberOfRemainingNeurons;
            }
            while (numberOfRemainingNeurons)
            {
                const unsigned short neuronIndexIndex = synapses[solutionBufIdx].lengths[lengthIndex++] % numberOfRemainingNeurons;
                const unsigned short inputNeuronIndex = neuronIndices[neuronIndexIndex];
                neuronIndices[neuronIndexIndex] = neuronIndices[--numberOfRemainingNeurons];
                unsigned long long* sy_pos = (unsigned long long*)(synapses1Bit[solutionBufIdx].input_positive + (inputNeuronIndex * (dataLength + numberOfInputNeutrons + infoLength)) / 8);
                unsigned long long* sy_neg = (unsigned long long*)(synapses1Bit[solutionBufIdx].input_negative + (inputNeuronIndex * (dataLength + numberOfInputNeutrons + infoLength)) / 8);

                int lv = 0;
                for (int i = 0; i < (NEURON_SCANNED_ROUND - 1); i++) {
                    unsigned long long A0 = sy_pos[i];
                    unsigned long long A1 = sy_neg[i];
                    unsigned long long B = ((unsigned long long*)nrVal1Bit)[i];
                    int s = __popcnt64(A0 ^ B);
                    s -= __popcnt64(A1 ^ B);
                    lv += s;
                }
                {
                    unsigned long long A0 = (*(unsigned long long*)(sy_pos + (NEURON_SCANNED_ROUND - 1))) & LAST_ELEMENT_MASK;
                    unsigned long long A1 = (*(unsigned long long*)(sy_neg + (NEURON_SCANNED_ROUND - 1))) & LAST_ELEMENT_MASK;
                    unsigned long long B = (*(unsigned long long*)(nrVal1Bit + (NEURON_SCANNED_ROUND - 1) * 8)) & LAST_ELEMENT_MASK;
                    int s = __popcnt64(A0 ^ B);
                    s -= __popcnt64(A1 ^ B);
                    lv += s;
                    neurons[solutionBufIdx].input[dataLength + inputNeuronIndex] += lv;
                    if (neurons[solutionBufIdx].input[dataLength + inputNeuronIndex] < 0) {
                        setBitNeuron(nrVal1Bit, dataLength + inputNeuronIndex);
                    }
                    else {
                        clearBitNeuron(nrVal1Bit, dataLength + inputNeuronIndex);
                    }
                }
            }
        }

        copyMem(&neurons[solutionBufIdx].output[0], &neurons[solutionBufIdx].input[dataLength + numberOfInputNeutrons], infoLength * sizeof(neurons[0].input[0]));
        for (int i = 0; i < dataLength + numberOfInputNeutrons + infoLength; i++) {
            if (neurons[solutionBufIdx].output[i] < 0) {
                setBitNeuron(nrVal1Bit, i);
            }
            else {
                clearBitNeuron(nrVal1Bit, i);
            }
        }
        for (unsigned int tick = 0; tick < MAX_OUTPUT_DURATION; tick++)
        {
            unsigned short neuronIndices[numberOfInputNeutrons + dataLength];
            unsigned short numberOfRemainingNeurons = 0;
            for (numberOfRemainingNeurons = 0; numberOfRemainingNeurons < numberOfInputNeutrons + dataLength; numberOfRemainingNeurons++)
            {
                neuronIndices[numberOfRemainingNeurons] = numberOfRemainingNeurons;
            }
            while (numberOfRemainingNeurons)
            {
                const unsigned short neuronIndexIndex = synapses[solutionBufIdx].lengths[lengthIndex++] % numberOfRemainingNeurons;
                const unsigned short outputNeuronIndex = neuronIndices[neuronIndexIndex];
                neuronIndices[neuronIndexIndex] = neuronIndices[--numberOfRemainingNeurons];
                unsigned long long* sy_pos = (unsigned long long*)(synapses1Bit[solutionBufIdx].output_positive + (outputNeuronIndex * (dataLength + numberOfInputNeutrons + infoLength)) / 8);
                unsigned long long* sy_neg = (unsigned long long*)(synapses1Bit[solutionBufIdx].output_negative + (outputNeuronIndex * (dataLength + numberOfInputNeutrons + infoLength)) / 8);
                int lv = 0;
                for (int i = 0; i < (NEURON_SCANNED_ROUND - 1); i++) {
                    unsigned long long A0 = sy_pos[i];
                    unsigned long long A1 = sy_neg[i];
                    unsigned long long B = ((unsigned long long*)nrVal1Bit)[i];
                    int s = __popcnt64(A0 ^ B);
                    s -= __popcnt64(A1 ^ B);
                    lv += s;
                }
                {
                    unsigned long long A0 = (*(unsigned long long*)(sy_pos + (NEURON_SCANNED_ROUND - 1))) & LAST_ELEMENT_MASK;
                    unsigned long long A1 = (*(unsigned long long*)(sy_neg + (NEURON_SCANNED_ROUND - 1))) & LAST_ELEMENT_MASK;
                    unsigned long long B = (*(unsigned long long*)(nrVal1Bit + (NEURON_SCANNED_ROUND - 1) * 8)) & LAST_ELEMENT_MASK;
                    int s = __popcnt64(A0 ^ B);
                    s -= __popcnt64(A1 ^ B);
                    lv += s;
                    neurons[solutionBufIdx].output[infoLength + outputNeuronIndex] += lv;
                    if (neurons[solutionBufIdx].output[infoLength + outputNeuronIndex] < 0) {
                        setBitNeuron(nrVal1Bit, infoLength + outputNeuronIndex);
                    }
                    else {
                        clearBitNeuron(nrVal1Bit, infoLength + outputNeuronIndex);
                    }
                }
            }
        }

        for (unsigned int i = 0; i < dataLength; i++)
        {
            if ((miningData[i] >= 0) == (neurons[solutionBufIdx].output[infoLength + numberOfInputNeutrons + i] >= 0))
            {
                score++;
            }
        }
        RELEASE(solutionEngineLock[solutionBufIdx]);
#if USE_SCORE_CACHE
        addScoreCache(publicKey, nonce, scoreCacheIndex, score);
#endif
        return score;
    }
};
