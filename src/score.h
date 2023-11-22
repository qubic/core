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
    char input_positive[(NUMBER_OF_INPUT_NEURONS + INFO_LENGTH) * (DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + INFO_LENGTH) / 8];
    char input_negative[(NUMBER_OF_INPUT_NEURONS + INFO_LENGTH) * (DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + INFO_LENGTH) / 8];
    char output_positive[(NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH) * (INFO_LENGTH + NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH) / 8];
    char output_negative[(NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH) * (INFO_LENGTH + NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH) / 8];
} synapses1Bit[SOLUTION_BUFFER_COUNT];

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

static unsigned int score(const unsigned long long processor_Number, const m256i& publicKey, const m256i& nonce)
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

    const unsigned long long solutionBufIdx = processor_Number % SOLUTION_BUFFER_COUNT;
    ACQUIRE(solutionEngineLock[solutionBufIdx]);
#define SYNAPSE_CHUNK_SIZE (DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + INFO_LENGTH)
#define SYNAPSE_CHUNK_SIZE_BIT (SYNAPSE_CHUNK_SIZE>>3)
#define SYNAPSE_CHUNK_SIZE_BIT_ROUND_UP (((SYNAPSE_CHUNK_SIZE_BIT+7)>>3)<<3)
#define NEURON_SCANNED_ROUND (SYNAPSE_CHUNK_SIZE_BIT_ROUND_UP >> 3)
#define SYNAPSE_MISSING_BYTES (8-(SYNAPSE_CHUNK_SIZE_BIT % 8))
#define LAST_ELEMENT_MASK (0xFFFFFFFFFFFFFFFFULL >> (SYNAPSE_MISSING_BYTES*8))
    unsigned char nrVal1Bit[SYNAPSE_CHUNK_SIZE_BIT_ROUND_UP];
    random(publicKey.m256i_u8, nonce.m256i_u8, (unsigned char*)&synapses[solutionBufIdx], sizeof(synapses[0]));

    for (unsigned int inputNeuronIndex = 0; inputNeuronIndex < NUMBER_OF_INPUT_NEURONS + INFO_LENGTH; inputNeuronIndex++)
    {
        unsigned long long* p = (unsigned long long*)(synapses[solutionBufIdx].input + inputNeuronIndex * (DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + INFO_LENGTH));
        for (unsigned int anotherInputNeuronIndex = 0; anotherInputNeuronIndex < (DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + INFO_LENGTH) / 8; anotherInputNeuronIndex++)
        {
            const unsigned int offset = inputNeuronIndex * (DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + INFO_LENGTH) / 8 + anotherInputNeuronIndex;
            neuronU64To1Bit(p[anotherInputNeuronIndex], (unsigned char*)synapses1Bit[solutionBufIdx].input_positive + offset,
                (unsigned char*)synapses1Bit[solutionBufIdx].input_negative + offset);
        }
    }
    for (unsigned int outputNeuronIndex = 0; outputNeuronIndex < NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH; outputNeuronIndex++)
    {
        unsigned long long* p = (unsigned long long*)(synapses[solutionBufIdx].output + outputNeuronIndex * (DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + INFO_LENGTH));
        for (unsigned int anotherOutputNeuronIndex = 0; anotherOutputNeuronIndex < (INFO_LENGTH + NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH) / 8; anotherOutputNeuronIndex++)
        {
            const unsigned int offset = outputNeuronIndex * (INFO_LENGTH + NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH) / 8 + anotherOutputNeuronIndex;
            neuronU64To1Bit(p[anotherOutputNeuronIndex], (unsigned char*)synapses1Bit[solutionBufIdx].output_positive + offset,
                (unsigned char*)synapses1Bit[solutionBufIdx].output_negative + offset);
        }
    }

    for (unsigned int inputNeuronIndex = 0; inputNeuronIndex < NUMBER_OF_INPUT_NEURONS + INFO_LENGTH; inputNeuronIndex++)
    {
        clearBitNeuron((unsigned char*)synapses1Bit[solutionBufIdx].input_positive, inputNeuronIndex * (DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + INFO_LENGTH) + (DATA_LENGTH + inputNeuronIndex));
        clearBitNeuron((unsigned char*)synapses1Bit[solutionBufIdx].input_negative, inputNeuronIndex * (DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + INFO_LENGTH) + (DATA_LENGTH + inputNeuronIndex));
    }
    for (unsigned int outputNeuronIndex = 0; outputNeuronIndex < NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH; outputNeuronIndex++)
    {
        clearBitNeuron((unsigned char*)synapses1Bit[solutionBufIdx].output_positive, outputNeuronIndex * (INFO_LENGTH + NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH) + (INFO_LENGTH + outputNeuronIndex));
        clearBitNeuron((unsigned char*)synapses1Bit[solutionBufIdx].output_negative, outputNeuronIndex * (INFO_LENGTH + NUMBER_OF_OUTPUT_NEURONS + DATA_LENGTH) + (INFO_LENGTH + outputNeuronIndex));
    }

    unsigned int lengthIndex = 0;

    copyMem(&neurons[solutionBufIdx].input[0], miningData, DATA_LENGTH * sizeof(miningData[0]));
    setMem(&neurons[solutionBufIdx].input[sizeof(miningData) / sizeof(neurons[0].input[0])], sizeof(neurons[0]) - sizeof(miningData), 0);
    setMem(nrVal1Bit, sizeof(nrVal1Bit), 0);

    for (int i = 0; i < DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + INFO_LENGTH; i++) {
        if (neurons[solutionBufIdx].input[i] < 0) {
            setBitNeuron(nrVal1Bit, i);
        }
        else {
            clearBitNeuron(nrVal1Bit, i);
        }
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
            unsigned long long* sy_pos = (unsigned long long*)(synapses1Bit[solutionBufIdx].input_positive + (inputNeuronIndex * (DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + INFO_LENGTH)) / 8);
            unsigned long long* sy_neg = (unsigned long long*)(synapses1Bit[solutionBufIdx].input_negative + (inputNeuronIndex * (DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + INFO_LENGTH)) / 8);

            int lv = 0;
            for (int i = 0; i < (NEURON_SCANNED_ROUND-1); i++) {
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
                neurons[solutionBufIdx].input[DATA_LENGTH + inputNeuronIndex] += lv;
                if (neurons[solutionBufIdx].input[DATA_LENGTH + inputNeuronIndex] < 0) {
                    setBitNeuron(nrVal1Bit, DATA_LENGTH + inputNeuronIndex);
                }
                else {
                    clearBitNeuron(nrVal1Bit, DATA_LENGTH + inputNeuronIndex);
                }
            }
        }
    }

    copyMem(&neurons[solutionBufIdx].output[0], &neurons[solutionBufIdx].input[DATA_LENGTH + NUMBER_OF_INPUT_NEURONS], INFO_LENGTH * sizeof(neurons[0].input[0]));
    for (int i = 0; i < DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + INFO_LENGTH; i++) {
        if (neurons[solutionBufIdx].output[i] < 0) {
            setBitNeuron(nrVal1Bit, i);
        }
        else {
            clearBitNeuron(nrVal1Bit, i);
        }
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
            unsigned long long* sy_pos = (unsigned long long*)(synapses1Bit[solutionBufIdx].output_positive + (outputNeuronIndex * (DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + INFO_LENGTH)) / 8);
            unsigned long long* sy_neg = (unsigned long long*)(synapses1Bit[solutionBufIdx].output_negative + (outputNeuronIndex * (DATA_LENGTH + NUMBER_OF_INPUT_NEURONS + INFO_LENGTH)) / 8);
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
                neurons[solutionBufIdx].output[INFO_LENGTH + outputNeuronIndex] += lv;
                if (neurons[solutionBufIdx].output[INFO_LENGTH + outputNeuronIndex] < 0) {
                    setBitNeuron(nrVal1Bit, INFO_LENGTH + outputNeuronIndex);
                }
                else {
                    clearBitNeuron(nrVal1Bit, INFO_LENGTH + outputNeuronIndex);
                }
            }
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