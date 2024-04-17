#pragma once

////////// Public Settings \\\\\\\\\\

//////////////////////////////////////////////////////////////////////////
// Config options for operators

// no need to define AVX512 here anymore, just change the project settings to use the AVX512 version
// random seed is now obtained from spectrumDigests

#define MAX_NUMBER_OF_PROCESSORS 32
#define NUMBER_OF_SOLUTION_PROCESSORS 6 // do not increase this for this epoch, because there may be issues due too fast ticking

#define USE_SCORE_CACHE 1
#define SCORE_CACHE_SIZE 2000000 // the larger the better
#define SCORE_CACHE_COLLISION_RETRIES 20 // number of retries to find entry in cache in case of hash collision

// Number of ticks to from prior epoch that are kept after seamless epoch transition. These can be requested after transition.
#define TICKS_TO_KEEP_FROM_PRIOR_EPOCH 100

#define TARGET_TICK_DURATION 2500
#define TRANSACTION_SPARSENESS 4

// Set START_NETWORK_FROM_SCRATCH to 0 if you start the node for syncing with the already ticking network.
// If this flag is 1, it indicates that the whole network (all 676 IDs) will start from scratch and agree that the very first tick time will be set at (2022-04-13 Wed 12:00:00.000UTC).
// If this flag is 0, the node will try to fetch data of the initial tick of the epoch from other nodes, because the tick's timestamp may differ from (2022-04-13 Wed 12:00:00.000UTC).
// If you restart your node after seamless epoch transition, make sure EPOCH and TICK are set correctly for the currently running epoch.
#define START_NETWORK_FROM_SCRATCH 0

//////////////////////////////////////////////////////////////////////////
// Config options that should NOT be changed by operators

#define VERSION_A 1
#define VERSION_B 199
#define VERSION_C 0

// Epoch and initial tick for node startup
#define EPOCH 105
#define TICK 13461049

#define ARBITRATOR "AFZPUAIYVPNUYGJRQVLUKOPPVLHAZQTGLYAAUUNBXFTVTAMSBKQBLEIEPCVJ"

static unsigned short SYSTEM_FILE_NAME[] = L"system";
static unsigned short SYSTEM_END_OF_EPOCH_FILE_NAME[] = L"system.eoe";
static unsigned short SPECTRUM_FILE_NAME[] = L"spectrum.???";
static unsigned short UNIVERSE_FILE_NAME[] = L"universe.???";
static unsigned short SCORE_CACHE_FILE_NAME[] = L"score.???";
static unsigned short CONTRACT_FILE_NAME[] = L"contract????.???";

#define DATA_LENGTH 256
#define INFO_LENGTH 128
#define NUMBER_OF_INPUT_NEURONS 4096
#define NUMBER_OF_OUTPUT_NEURONS 4096
#define MAX_INPUT_DURATION 256
#define MAX_OUTPUT_DURATION 256
#define NEURON_VALUE_LIMIT 1LL
#define SOLUTION_THRESHOLD_DEFAULT 43

// include commonly needed definitions
#include "network_messages/common_def.h"

#define MAX_NUMBER_OF_TICKS_PER_EPOCH (((((60 * 60 * 24 * 7) / (TARGET_TICK_DURATION / 1000)) + NUMBER_OF_COMPUTORS - 1) / NUMBER_OF_COMPUTORS) * NUMBER_OF_COMPUTORS)
#define FIRST_TICK_TRANSACTION_OFFSET sizeof(unsigned long long)
#define MAX_TRANSACTION_SIZE (MAX_INPUT_SIZE + sizeof(Transaction) + SIGNATURE_SIZE)

