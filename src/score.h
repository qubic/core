#pragma once

#include "public_settings.h"
#include "platform/memory.h"
#include "platform/m256.h"
#include "platform/concurrency.h"
#include "platform/file_io.h"
#include "platform/logging.h"
#include "platform/time_stamp_counter.h"
#include "kangaroo_twelve.h"

////////// Scoring algorithm \\\\\\\\\\

#define SOLUTION_BUFFER_COUNT 8
static int miningData[DATA_LENGTH];
static struct
{
    int input[DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + INFO_LENGTH];
    int output[INFO_LENGTH + NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH];
} neurons[SOLUTION_BUFFER_COUNT];
static struct
{
    char input[(NUMBER_OF_INPUT_NEURONS + INFO_LENGTH) * (DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + INFO_LENGTH)];
    char output[(NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH) * (INFO_LENGTH + NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH)];
    unsigned short lengths[MAX_INPUT_DURATION * (NUMBER_OF_INPUT_NEURONS + INFO_LENGTH) + MAX_OUTPUT_DURATION * (NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH)];
} synapses[SOLUTION_BUFFER_COUNT];

struct
{
    char input[(NUMBER_OF_INPUT_NEURONS + INFO_LENGTH) * (DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + INFO_LENGTH) / 4];
    char output[(NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH) * (INFO_LENGTH + NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH) / 4];
} synapses2Bit[SOLUTION_BUFFER_COUNT];

static volatile char solutionEngineLock[SOLUTION_BUFFER_COUNT];

#if USE_SCORE_CACHE
struct
{
    m256i publicKey;
    m256i nonce;
    int score;
} scoreCache[SCORE_CACHE_SIZE]; // set zero or load from a file on init

static volatile char scoreCacheLock;

static unsigned int scoreCacheHit = 0;
static unsigned int scoreCacheMiss = 0;
static unsigned int scoreCacheUnknown = 0;

static unsigned int getScoreCacheIndex(const m256i& publicKey, const m256i& nonce)
{
    m256i buffer[2] = { publicKey, nonce };
    unsigned char digest[32];
    KangarooTwelve64To32(buffer, digest);
    unsigned int result = *((unsigned long long*)digest) % SCORE_CACHE_SIZE;

    return result;
}

static int tryFetchingScoreCache(const m256i& publicKey, const m256i& nonce, unsigned int scoreCacheIndex)
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

static void addScoreCache(const m256i& publicKey, const m256i& nonce, unsigned int scoreCacheIndex, int score)
{
    ACQUIRE(scoreCacheLock);
    scoreCache[scoreCacheIndex].publicKey = publicKey;
    scoreCache[scoreCacheIndex].nonce = nonce;
    scoreCache[scoreCacheIndex].score = score;
    RELEASE(scoreCacheLock);
}

static void initEmptyScoreCache()
{
    ACQUIRE(scoreCacheLock);
    setMem((unsigned char*)scoreCache, sizeof(scoreCache), 0);
    RELEASE(scoreCacheLock);
}
#endif

// Save score cache to SCORE_CACHE_FILE_NAME
static void saveScoreCache()
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
static bool loadScoreCache(int epoch)
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

static void setSynapse2Bits(char* p_nr, unsigned int idx, const char val) {
    int pos = ((idx & 3) << 1);
    *((unsigned char*)(&p_nr[idx >> 2])) = (*((unsigned char*)(&p_nr[idx >> 2])) & ~(3 << pos)) | ((val & 3) << pos);
}

static void setZeroSynapse2Bits(char* p_nr, unsigned int idx) {
    *((unsigned char*)(&p_nr[idx >> 2])) &= ~(3 << ((idx & 3) << 1));
}

static char getSynapse2Bits(char* p_nr, unsigned int idx) {
    char r = (((char*)p_nr)[idx >> 2] >> ((idx & 3) << 1)) & 3;
    return -2 * (r >> 1) + (r & 1);
}

static int computeNeuron64Bit(unsigned long long A, unsigned long long B) {
    unsigned long long A_high = A & 0xaaaaaaaaaaaaaaaall; // b10101010 10101010 high part of A
    unsigned long long A_low = A & 0x5555555555555555ll; // b01010101 01010101 low part of A
    unsigned long long B_high = B & 0xaaaaaaaaaaaaaaaall;
    unsigned long long B_low = B & 0x5555555555555555ll;
    unsigned long long zero_check = ((A_low << 1) | A_high);
    unsigned long long lv = -2 * __popcnt64(A_high ^ B_high & zero_check);
    lv += __popcnt64(A_low & B_low);
    return int(lv);
}

static unsigned int score(const unsigned long long processorNumber, const m256i& publicKey, const m256i& nonce)
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

    const unsigned long long solutionBufIdx = processorNumber % SOLUTION_BUFFER_COUNT;
    ACQUIRE(solutionEngineLock[solutionBufIdx]);
    char neuronValue[1000];
    setMem(neuronValue, sizeof(neuronValue), 0);
    setMem(synapses2Bit[solutionBufIdx].input, sizeof(synapses2Bit[solutionBufIdx]), 0);
    random(publicKey.m256i_u8, nonce.m256i_u8, (unsigned char*)&synapses[solutionBufIdx], sizeof(synapses[0]));
    for (unsigned int inputNeuronIndex = 0; inputNeuronIndex < NUMBER_OF_INPUT_NEURONS + INFO_LENGTH; inputNeuronIndex++)
    {
        for (unsigned int anotherInputNeuronIndex = 0; anotherInputNeuronIndex < DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + INFO_LENGTH; anotherInputNeuronIndex++)
        {
            const unsigned int offset = inputNeuronIndex * (DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + INFO_LENGTH) + anotherInputNeuronIndex;
            setSynapse2Bits((char*)(synapses2Bit[solutionBufIdx].input), offset, char(((unsigned char)synapses[solutionBufIdx].input[offset]) % 3) - 1);
        }
    }
    for (unsigned int outputNeuronIndex = 0; outputNeuronIndex < NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH; outputNeuronIndex++)
    {
        for (unsigned int anotherOutputNeuronIndex = 0; anotherOutputNeuronIndex < INFO_LENGTH + NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH; anotherOutputNeuronIndex++)
        {
            const unsigned int offset = outputNeuronIndex * (INFO_LENGTH + NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH) + anotherOutputNeuronIndex;
            setSynapse2Bits((char*)(synapses2Bit[solutionBufIdx].output), offset, char(((unsigned char)synapses[solutionBufIdx].output[offset]) % 3) - 1);
        }
    }
    for (unsigned int inputNeuronIndex = 0; inputNeuronIndex < NUMBER_OF_INPUT_NEURONS + INFO_LENGTH; inputNeuronIndex++)
    {
        setZeroSynapse2Bits((char*)(synapses2Bit[solutionBufIdx].input), inputNeuronIndex * (DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + INFO_LENGTH) + (DATA_LENGTH + inputNeuronIndex));
    }
    for (unsigned int outputNeuronIndex = 0; outputNeuronIndex < NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH; outputNeuronIndex++)
    {
        setZeroSynapse2Bits((char*)(synapses2Bit[solutionBufIdx].output), outputNeuronIndex * (INFO_LENGTH + NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH) + (INFO_LENGTH + outputNeuronIndex));
    }

    unsigned int lengthIndex = 0;

    copyMem(&neurons[solutionBufIdx].input[0], miningData, sizeof(miningData));
    setMem(&neurons[solutionBufIdx].input[sizeof(miningData) / sizeof(neurons[0].input[0])], sizeof(neurons[0]) - sizeof(miningData), 0);

    for (int i = 0; i < DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + INFO_LENGTH; i++) {
        setSynapse2Bits(neuronValue, i, neurons[solutionBufIdx].input[i] >= 0 ? 1 : -1);
    }

    for (unsigned int tick = 0; tick < MAX_INPUT_DURATION; tick++)
    {
        unsigned short neuronIndices[NUMBER_OF_INPUT_NEURONS + INFO_LENGTH];
        unsigned short numberOfRemainingNeurons = 0;
        for (numberOfRemainingNeurons = 0; numberOfRemainingNeurons < NUMBER_OF_INPUT_NEURONS + INFO_LENGTH; numberOfRemainingNeurons++)
        {
            neuronIndices[numberOfRemainingNeurons] = numberOfRemainingNeurons;
        }
        while (numberOfRemainingNeurons)
        {
            const unsigned short neuronIndexIndex = synapses[solutionBufIdx].lengths[lengthIndex++] % numberOfRemainingNeurons;
            const unsigned short inputNeuronIndex = neuronIndices[neuronIndexIndex];
            neuronIndices[neuronIndexIndex] = neuronIndices[--numberOfRemainingNeurons];

            // hot position
            int left = (DATA_LENGTH + inputNeuronIndex) / 32;
            unsigned long long* sy_ptr = (unsigned long long*)(synapses2Bit[solutionBufIdx].input + (inputNeuronIndex * (DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + INFO_LENGTH)) / 4);
            for (int i = 0; i < left; i++) {
                unsigned long long A = sy_ptr[i];
                unsigned long long B = ((unsigned long long*)neuronValue)[i];
                int lv = computeNeuron64Bit(A, B);
                neurons[solutionBufIdx].input[DATA_LENGTH + inputNeuronIndex] += lv;
            }
            for (int i = left * 32; i < (left + 1) * 32; i++) {
                int value = neurons[solutionBufIdx].input[i] >= 0 ? 1 : -1;
                char sy = getSynapse2Bits((char*)synapses2Bit[solutionBufIdx].input, inputNeuronIndex * (DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + INFO_LENGTH) + i);
                value *= sy;
                neurons[solutionBufIdx].input[DATA_LENGTH + inputNeuronIndex] += value;
            }
            // update hot position
            for (int i = (left + 1); i < (DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + INFO_LENGTH) / 32; i++) {
                unsigned long long A = sy_ptr[i];
                unsigned long long B = ((unsigned long long*)neuronValue)[i];
                int lv = computeNeuron64Bit(A, B);
                neurons[solutionBufIdx].input[DATA_LENGTH + inputNeuronIndex] += lv;
            }
            setSynapse2Bits(neuronValue, DATA_LENGTH + inputNeuronIndex, neurons[solutionBufIdx].input[DATA_LENGTH + inputNeuronIndex] >= 0 ? 1 : -1);
        }
    }

    copyMem(&neurons[solutionBufIdx].output[0], &neurons[solutionBufIdx].input[DATA_LENGTH + NUMBER_OF_INPUT_NEURONS], INFO_LENGTH * sizeof(neurons[0].input[0]));
    for (int i = 0; i < DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + INFO_LENGTH; i++) {
        setSynapse2Bits(neuronValue, i, neurons[solutionBufIdx].output[i] >= 0 ? 1 : -1);
    }
    for (unsigned int tick = 0; tick < MAX_OUTPUT_DURATION; tick++)
    {
        unsigned short neuronIndices[NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH];
        unsigned short numberOfRemainingNeurons = 0;
        for (numberOfRemainingNeurons = 0; numberOfRemainingNeurons < NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH; numberOfRemainingNeurons++)
        {
            neuronIndices[numberOfRemainingNeurons] = numberOfRemainingNeurons;
        }
        while (numberOfRemainingNeurons)
        {
            const unsigned short neuronIndexIndex = synapses[solutionBufIdx].lengths[lengthIndex++] % numberOfRemainingNeurons;
            const unsigned short outputNeuronIndex = neuronIndices[neuronIndexIndex];
            neuronIndices[neuronIndexIndex] = neuronIndices[--numberOfRemainingNeurons];
            // hot position
            int left = (INFO_LENGTH + outputNeuronIndex) / 32;
            unsigned long long* sy_ptr = (unsigned long long*)(synapses2Bit[solutionBufIdx].output + (outputNeuronIndex * (DATA_LENGTH + NUMBER_OF_OUTPUT_NEURONS + INFO_LENGTH)) / 4);
            {
                int i = 0;
                while (i < left)
                {
                    unsigned long long A = sy_ptr[i];
                    unsigned long long B = ((unsigned long long*)neuronValue)[i];
                    int lv = computeNeuron64Bit(A, B);
                    neurons[solutionBufIdx].output[INFO_LENGTH + outputNeuronIndex] += lv;
                    i++;
                }
            }

            for (int i = left * 32; i < (left + 1) * 32; i++) {
                int value = neurons[solutionBufIdx].output[i] >= 0 ? 1 : -1;
                char sy = getSynapse2Bits((char*)synapses2Bit[solutionBufIdx].output, outputNeuronIndex * (DATA_LENGTH + NUMBER_OF_OUTPUT_NEURONS + INFO_LENGTH) + i);
                value *= sy;
                neurons[solutionBufIdx].output[INFO_LENGTH + outputNeuronIndex] += value;
            }
            // update hot position
            for (int i = (left + 1); i < (DATA_LENGTH + NUMBER_OF_OUTPUT_NEURONS + INFO_LENGTH) / 32; i++) {
                unsigned long long A = sy_ptr[i];
                unsigned long long B = ((unsigned long long*)neuronValue)[i];
                int lv = computeNeuron64Bit(A, B);
                neurons[solutionBufIdx].output[INFO_LENGTH + outputNeuronIndex] += lv;
            }
            setSynapse2Bits(neuronValue, INFO_LENGTH + outputNeuronIndex, neurons[solutionBufIdx].output[INFO_LENGTH + outputNeuronIndex] >= 0 ? 1 : -1);
        }
    }

    for (unsigned int i = 0; i < DATA_LENGTH; i++)
    {
        if ((miningData[i] >= 0) == (neurons[solutionBufIdx].output[INFO_LENGTH + NUMBER_OF_OUTPUT_NEURONS + i] >= 0))
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