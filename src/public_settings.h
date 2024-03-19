#pragma once

////////// Public Settings \\\\\\\\\\

// no need to define AVX512 here anymore, just change the project settings to use the AVX512 version

#define MAX_NUMBER_OF_PROCESSORS 32
#define NUMBER_OF_SOLUTION_PROCESSORS 6 // do not increase this for this epoch, because there may be issues due too fast ticking

#define VERSION_A 1
#define VERSION_B 196
#define VERSION_C 0

#define EPOCH 100
#define TICK 12950000

// random seed is now obtained from spectrumDigests

#define ARBITRATOR "AFZPUAIYVPNUYGJRQVLUKOPPVLHAZQTGLYAAUUNBXFTVTAMSBKQBLEIEPCVJ"

static unsigned short SYSTEM_FILE_NAME[] = L"system";
static unsigned short SPECTRUM_FILE_NAME[] = L"spectrum.???";
static unsigned short UNIVERSE_FILE_NAME[] = L"universe.???";
static unsigned short SCORE_CACHE_FILE_NAME[] = L"score.???";
static unsigned short CONTRACT_FILE_NAME[] = L"contract????.???";

#define DATA_LENGTH 256
#define INFO_LENGTH 128
#define NUMBER_OF_INPUT_NEURONS 1024
#define NUMBER_OF_OUTPUT_NEURONS 1024
#define MAX_INPUT_DURATION 256
#define MAX_OUTPUT_DURATION 256
#define NEURON_VALUE_LIMIT 1LL
#define SOLUTION_THRESHOLD_DEFAULT 44
#define USE_SCORE_CACHE 1
#define SCORE_CACHE_SIZE 1000000 // the larger the better
#define SCORE_CACHE_COLLISION_RETRIES 20 // number of retries to find entry in cache in case of hash collision
