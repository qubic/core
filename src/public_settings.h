#pragma once

////////// Public Settings \\\\\\\\\\

// no need to define AVX512 here anymore, just change the project settings to use the AVX512 version

#define MAX_NUMBER_OF_PROCESSORS 32

#define VERSION_A 1
#define VERSION_B 188
#define VERSION_C 0

#define EPOCH 91
#define TICK 11950000

#define RANDOM_SEED0 62
#define RANDOM_SEED1 69
#define RANDOM_SEED2 244
#define RANDOM_SEED3 233
#define RANDOM_SEED4 77
#define RANDOM_SEED5 13
#define RANDOM_SEED6 169
#define RANDOM_SEED7 148

#define ARBITRATOR "AFZPUAIYVPNUYGJRQVLUKOPPVLHAZQTGLYAAUUNBXFTVTAMSBKQBLEIEPCVJ"

#define IGNORE_RESOURCE_TESTING 0

static unsigned short SYSTEM_FILE_NAME[] = L"system";
static unsigned short SPECTRUM_FILE_NAME[] = L"spectrum.???";
static unsigned short UNIVERSE_FILE_NAME[] = L"universe.???";
static unsigned short SCORE_CACHE_FILE_NAME[] = L"score.???";
static unsigned short CONTRACT_FILE_NAME[] = L"contract????.???";

#define DATA_LENGTH 1200
#define INFO_LENGTH 1200
#define NUMBER_OF_INPUT_NEURONS 2400
#define NUMBER_OF_OUTPUT_NEURONS 2400
#define MAX_INPUT_DURATION 200
#define MAX_OUTPUT_DURATION 200
#define SOLUTION_THRESHOLD 692
#define USE_SCORE_CACHE 1
#define SCORE_CACHE_SIZE 1000000 // the larger the better
#define SCORE_CACHE_COLLISION_RETRIES 20 // number of retries to find entry in cache in case of hash collision
