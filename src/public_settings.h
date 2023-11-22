#pragma once

////////// Public Settings \\\\\\\\\\

#define AVX512 0
#define MAX_NUMBER_OF_PROCESSORS 32

#define VERSION_A 1
#define VERSION_B 181
#define VERSION_C 0

#define EPOCH 84
#define TICK 10950000

#define RANDOM_SEED0 66
#define RANDOM_SEED1 99
#define RANDOM_SEED2 25
#define RANDOM_SEED3 11
#define RANDOM_SEED4 169
#define RANDOM_SEED5 122
#define RANDOM_SEED6 77
#define RANDOM_SEED7 137

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
#define SOLUTION_THRESHOLD 690
#define USE_SCORE_CACHE 1
#define SCORE_CACHE_SIZE 1000000 // the larger the better