#pragma once

////////// Public Settings \\\\\\\\\\

// no need to define AVX512 here anymore, just change the project settings to use the AVX512 version

#define MAX_NUMBER_OF_PROCESSORS 32

#define VERSION_A 1
#define VERSION_B 186
#define VERSION_C 0

#define EPOCH 89
#define TICK 11700000

#define RANDOM_SEED0 170
#define RANDOM_SEED1 7
#define RANDOM_SEED2 69
#define RANDOM_SEED3 88
#define RANDOM_SEED4 150
#define RANDOM_SEED5 0
#define RANDOM_SEED6 72
#define RANDOM_SEED7 147

#define ARBITRATOR "AFZPUAIYVPNUYGJRQVLUKOPPVLHAZQTGLYAAUUNBXFTVTAMSBKQBLEIEPCVJ"

#define IGNORE_RESOURCE_TESTING 0

static unsigned short SYSTEM_FILE_NAME[] = L"system";
static unsigned short SPECTRUM_FILE_NAME[] = L"spectrum.???";
static unsigned short UNIVERSE_FILE_NAME[] = L"universe.???";
static unsigned short SCORE_CACHE_FILE_NAME[] = L"score.???";
static unsigned short CONTRACT_FILE_NAME[] = L"contract????.???";

#define DATA_LENGTH 1200
#define INFO_LENGTH 1200
#define NUMBER_OF_INPUT_NEURONS 1200
#define NUMBER_OF_OUTPUT_NEURONS 1200
#define MAX_INPUT_DURATION 200
#define MAX_OUTPUT_DURATION 200
#define SOLUTION_THRESHOLD 692
#define USE_SCORE_CACHE 1
#define SCORE_CACHE_SIZE 1000000 // the larger the better