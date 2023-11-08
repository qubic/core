#pragma once

////////// Public Settings \\\\\\\\\\

#define AVX512 0
#define MAX_NUMBER_OF_PROCESSORS 32

#define VERSION_A 1
#define VERSION_B 179
#define VERSION_C 0

#define EPOCH 82
#define TICK 10600000

#define ARBITRATOR "AEVTNYLMHCFOOCNGYGKPFTHHJTRCKLSSXYRAEHHSKACWGVSUUMJIWVZBPLVD"

#define IGNORE_RESOURCE_TESTING 0

static unsigned short SYSTEM_FILE_NAME[] = L"system";
static unsigned short SPECTRUM_FILE_NAME[] = L"spectrum.???";
static unsigned short UNIVERSE_FILE_NAME[] = L"universe.???";
static unsigned short SCORE_CACHE_FILE_NAME[] = L"score.???";
static unsigned short CONTRACT_FILE_NAME[] = L"contract????.???";

#define DATA_LENGTH 2000
#define INFO_LENGTH 1000
#define NUMBER_OF_INPUT_NEURONS 1000
#define NUMBER_OF_OUTPUT_NEURONS 1000
#define MAX_INPUT_DURATION 200
#define MAX_OUTPUT_DURATION 200
#define SOLUTION_THRESHOLD 1115
#define USE_SCORE_CACHE 1
#define SCORE_CACHE_SIZE 1000000 // the larger the better