#pragma once

////////// Public Settings \\\\\\\\\\

//////////////////////////////////////////////////////////////////////////
// Config options for operators

// no need to define AVX512 here anymore, just change the project settings to use the AVX512 version
// random seed is now obtained from spectrumDigests

#define MAX_NUMBER_OF_PROCESSORS 32
#define NUMBER_OF_SOLUTION_PROCESSORS 6 // do not increase this for this epoch, because there may be issues due to too fast ticking

// Number of buffers available for executing contract functions in parallel; having more means reserving a bit more RAM (+1 = +32 MB)
// and less waiting in request processors if there are more parallel contract function requests. The maximum value that may make sense
// is MAX_NUMBER_OF_PROCESSORS - 1.
#define NUMBER_OF_CONTRACT_EXECUTION_BUFFERS 10

#define USE_SCORE_CACHE 1
#define SCORE_CACHE_SIZE 2000000 // the larger the better
#define SCORE_CACHE_COLLISION_RETRIES 20 // number of retries to find entry in cache in case of hash collision

// Number of ticks from prior epoch that are kept after seamless epoch transition. These can be requested after transition.
#define TICKS_TO_KEEP_FROM_PRIOR_EPOCH 100

#define TARGET_TICK_DURATION 2000
#define TRANSACTION_SPARSENESS 3

// Below are 2 variables that are used for auto-F5 feature:
#define AUTO_FORCE_NEXT_TICK_THRESHOLD 0ULL // Multiplier of TARGET_TICK_DURATION for the system to detect "F5 case" | set to 0 to disable
                                            // to prevent bad actor causing misalignment.
											// depends on actual tick time of the network, operators should set this number randomly in this range [12, 26]
                                            // eg: If AUTO_FORCE_NEXT_TICK_THRESHOLD is 8 and TARGET_TICK_DURATION is 2, then the system will start "auto F5 procedure" after 16 seconds after receveing 451+ votes

#define PROBABILITY_TO_FORCE_EMPTY_TICK 800 // after (AUTO_FORCE_NEXT_TICK_THRESHOLD x TARGET_TICK_DURATION) seconds, the node will start casting F5 randomly every second with this probability
                                            // to prevent bad actor causing misalignment, operators should set this number in this range [700, 900] (aka [7%, 9%])


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
#define VERSION_B 209
#define VERSION_C 0

// Epoch and initial tick for node startup
#define EPOCH 115
#define TICK 14600000

#define ARBITRATOR "AFZPUAIYVPNUYGJRQVLUKOPPVLHAZQTGLYAAUUNBXFTVTAMSBKQBLEIEPCVJ"

static unsigned short SYSTEM_FILE_NAME[] = L"system";
static unsigned short SYSTEM_END_OF_EPOCH_FILE_NAME[] = L"system.eoe";
static unsigned short SPECTRUM_FILE_NAME[] = L"spectrum.???";
static unsigned short UNIVERSE_FILE_NAME[] = L"universe.???";
static unsigned short SCORE_CACHE_FILE_NAME[] = L"score.???";
static unsigned short CONTRACT_FILE_NAME[] = L"contract????.???";

#define DATA_LENGTH 256
#define NUMBER_OF_INPUT_NEURONS 32768
#define NUMBER_OF_OUTPUT_NEURONS 32768
#define MAX_INPUT_DURATION 256
#define MAX_OUTPUT_DURATION 256
#define NEURON_VALUE_LIMIT 1LL
#define SOLUTION_THRESHOLD_DEFAULT 44

// include commonly needed definitions
#include "network_messages/common_def.h"

#define MAX_NUMBER_OF_TICKS_PER_EPOCH (((((60 * 60 * 24 * 7) / (TARGET_TICK_DURATION / 1000)) + NUMBER_OF_COMPUTORS - 1) / NUMBER_OF_COMPUTORS) * NUMBER_OF_COMPUTORS)
#define FIRST_TICK_TRANSACTION_OFFSET sizeof(unsigned long long)
#define MAX_TRANSACTION_SIZE (MAX_INPUT_SIZE + sizeof(Transaction) + SIGNATURE_SIZE)

#define STACK_SIZE 4194304
#define TRACK_MAX_STACK_BUFFER_SIZE
