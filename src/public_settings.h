#pragma once

////////// Public Settings \\\\\\\\\\

//////////////////////////////////////////////////////////////////////////
// Config options for operators

// no need to define AVX512 here anymore, just change the project settings to use the AVX512 version
// random seed is now obtained from spectrumDigests

#define MAX_NUMBER_OF_PROCESSORS 32
#define NUMBER_OF_SOLUTION_PROCESSORS 6 // do not increase this for this epoch, because there may be issues due too fast ticking
#define NUMBER_OF_CONTRACT_EXECUTION_PROCESSORS 2  // number of processors that can execute contract functions in parallel

#define USE_SCORE_CACHE 1
#define SCORE_CACHE_SIZE 2000000 // the larger the better
#define SCORE_CACHE_COLLISION_RETRIES 20 // number of retries to find entry in cache in case of hash collision

// Number of ticks to from prior epoch that are kept after seamless epoch transition. These can be requested after transition.
#define TICKS_TO_KEEP_FROM_PRIOR_EPOCH 100

#define TARGET_TICK_DURATION 7000
#define TRANSACTION_SPARSENESS 4

// Set START_NETWORK_FROM_SCRATCH to 0 if you start the node for syncing with the already ticking network.
// If this flag is 1, it indicates that the whole network (all 676 IDs) will start from scratch and agree that the very first tick time will be set at (2022-04-13 Wed 12:00:00.000UTC).
// If this flag is 0, the node will try to fetch data of the initial tick of the epoch from other nodes, because the tick's timestamp may differ from (2022-04-13 Wed 12:00:00.000UTC).
// If you restart your node after seamless epoch transition, make sure EPOCH and TICK are set correctly for the currently running epoch.
#define START_NETWORK_FROM_SCRATCH 1

// Addons: If you don't know it, leave it 0.
#define ADDON_TX_STATUS_REQUEST 0

//////////////////////////////////////////////////////////////////////////
// Config options that should NOT be changed by operators

#define VERSION_A 1
#define VERSION_B 205
#define VERSION_C 1

// Epoch and initial tick for node startup

#define EPOCH 108
#define TICK 13820000

// random seed is now obtained from spectrumDigests

#define ARBITRATOR "MEFKYFCDXDUILCAJKOIKWQAPENJDUHSSYPBRWFOTLALILAYWQFDSITJELLHG"

static unsigned short SYSTEM_FILE_NAME[] = L"system";
static unsigned short SYSTEM_END_OF_EPOCH_FILE_NAME[] = L"system.eoe";
static unsigned short SPECTRUM_FILE_NAME[] = L"spectrum.???";
static unsigned short UNIVERSE_FILE_NAME[] = L"universe.???";
static unsigned short SCORE_CACHE_FILE_NAME[] = L"score.???";
static unsigned short CONTRACT_FILE_NAME[] = L"contract????.???";

#define DATA_LENGTH 256
#define INFO_LENGTH 128
#define NUMBER_OF_INPUT_NEURONS 16384
#define NUMBER_OF_OUTPUT_NEURONS 16384
#define MAX_INPUT_DURATION 256
#define MAX_OUTPUT_DURATION 256
#define NEURON_VALUE_LIMIT 1LL
#define SOLUTION_THRESHOLD_DEFAULT 44

// include commonly needed definitions
#include "network_messages/common_def.h"

#define MAX_NUMBER_OF_TICKS_PER_EPOCH (((((60 * 60 * 24 * 1) / (TARGET_TICK_DURATION / 1000)) + NUMBER_OF_COMPUTORS - 1) / NUMBER_OF_COMPUTORS) * NUMBER_OF_COMPUTORS)
#define FIRST_TICK_TRANSACTION_OFFSET sizeof(unsigned long long)
#define MAX_TRANSACTION_SIZE (MAX_INPUT_SIZE + sizeof(Transaction) + SIGNATURE_SIZE)

#define STACK_SIZE 4194304
#define TRACK_MAX_STACK_BUFFER_SIZE
