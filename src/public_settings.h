#pragma once

////////// Public Settings \\\\\\\\\\

//////////////////////////////////////////////////////////////////////////
// Config options for operators

// no need to define AVX512 here anymore, just change the project settings to use the AVX512 version
// random seed is now obtained from spectrumDigests

#define MAX_NUMBER_OF_PROCESSORS 32
#define NUMBER_OF_SOLUTION_PROCESSORS 12

// Number of buffers available for executing contract functions in parallel; having more means reserving a bit more RAM (+1 = +32 MB)
// and less waiting in request processors if there are more parallel contract function requests. The maximum value that may make sense
// is MAX_NUMBER_OF_PROCESSORS - 1.
#define NUMBER_OF_CONTRACT_EXECUTION_BUFFERS 10

#define USE_SCORE_CACHE 1
#define SCORE_CACHE_SIZE 2000000 // the larger the better
#define SCORE_CACHE_COLLISION_RETRIES 20 // number of retries to find entry in cache in case of hash collision

// Number of ticks from prior epoch that are kept after seamless epoch transition. These can be requested after transition.
#define TICKS_TO_KEEP_FROM_PRIOR_EPOCH 100

// The tick duration used for timing and scheduling logic.
#define TARGET_TICK_DURATION 1000

// The tick duration used to calculate the size of memory buffers.
// This determines the memory footprint of the application.
#define TICK_DURATION_FOR_ALLOCATION_MS 750
#define TRANSACTION_SPARSENESS 1

// Below are 2 variables that are used for auto-F5 feature:
#define AUTO_FORCE_NEXT_TICK_THRESHOLD 0ULL // Multiplier of TARGET_TICK_DURATION for the system to detect "F5 case" | set to 0 to disable
                                            // to prevent bad actor causing misalignment.
                                            // depends on actual tick time of the network, operators should set this number randomly in this range [12, 26]
                                            // eg: If AUTO_FORCE_NEXT_TICK_THRESHOLD is 8 and TARGET_TICK_DURATION is 2, then the system will start "auto F5 procedure" after 16 seconds after receveing 451+ votes

#define PROBABILITY_TO_FORCE_EMPTY_TICK 800 // after (AUTO_FORCE_NEXT_TICK_THRESHOLD x TARGET_TICK_DURATION) seconds, the node will start casting F5 randomly every second with this probability
                                            // to prevent bad actor causing misalignment, operators should set this number in this range [700, 900] (aka [7%, 9%])

#define NEXT_TICK_TIMEOUT_THRESHOLD 5ULL // Multiplier of TARGET_TICK_DURATION for the system to discard next tick in tickData.
                                         // This will lead to zero `expectedNextTickTransactionDigest` in consensus

#define PEER_REFRESHING_PERIOD 120000ULL
#if AUTO_FORCE_NEXT_TICK_THRESHOLD != 0
static_assert(NEXT_TICK_TIMEOUT_THRESHOLD < AUTO_FORCE_NEXT_TICK_THRESHOLD, "Timeout threshold must be smaller than auto F5 threshold");
static_assert(AUTO_FORCE_NEXT_TICK_THRESHOLD* TARGET_TICK_DURATION >= PEER_REFRESHING_PERIOD, "AutoF5 threshold must be greater than PEER_REFRESHING_PERIOD");
#endif
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
#define VERSION_B 264
#define VERSION_C 0

// Epoch and initial tick for node startup
#define EPOCH 183
#define TICK 34815000
#define TICK_IS_FIRST_TICK_OF_EPOCH 1 // Set to 0 if the network is restarted during the EPOCH with a new initial TICK

#define ARBITRATOR "AFZPUAIYVPNUYGJRQVLUKOPPVLHAZQTGLYAAUUNBXFTVTAMSBKQBLEIEPCVJ"
#define DISPATCHER "XPXYKFLGSWRHRGAUKWFWVXCDVEYAPCPCNUTMUDWFGDYQCWZNJMWFZEEGCFFO"

static unsigned short SYSTEM_FILE_NAME[] = L"system";
static unsigned short SYSTEM_END_OF_EPOCH_FILE_NAME[] = L"system.eoe";
static unsigned short SPECTRUM_FILE_NAME[] = L"spectrum.???";
static unsigned short UNIVERSE_FILE_NAME[] = L"universe.???";
static unsigned short SCORE_CACHE_FILE_NAME[] = L"score.???";
static unsigned short CONTRACT_FILE_NAME[] = L"contract????.???";
static unsigned short CUSTOM_MINING_REVENUE_END_OF_EPOCH_FILE_NAME[] = L"custom_revenue.eoe";
static unsigned short CUSTOM_MINING_CACHE_FILE_NAME[] = L"custom_mining_cache.???";

static constexpr unsigned long long NUMBER_OF_INPUT_NEURONS = 512;     // K
static constexpr unsigned long long NUMBER_OF_OUTPUT_NEURONS = 512;    // L
static constexpr unsigned long long NUMBER_OF_TICKS = 1000;               // N
static constexpr unsigned long long NUMBER_OF_NEIGHBORS = 728;    // 2M. Must be divided by 2
static constexpr unsigned long long NUMBER_OF_MUTATIONS = 150;
static constexpr unsigned long long POPULATION_THRESHOLD = NUMBER_OF_INPUT_NEURONS + NUMBER_OF_OUTPUT_NEURONS + NUMBER_OF_MUTATIONS; // P
static constexpr long long NEURON_VALUE_LIMIT = 1LL;
static constexpr unsigned int SOLUTION_THRESHOLD_DEFAULT = 321;

#define SOLUTION_SECURITY_DEPOSIT 1000000

// Signing difficulty
#define TARGET_TICK_VOTE_SIGNATURE 0x00095CBEU // around 7000 signing operations per ID

// include commonly needed definitions
#include "network_messages/common_def.h"

#define MAX_NUMBER_OF_TICKS_PER_EPOCH (((((60ULL * 60 * 24 * 7 * 1000) / TICK_DURATION_FOR_ALLOCATION_MS) + NUMBER_OF_COMPUTORS - 1) / NUMBER_OF_COMPUTORS) * NUMBER_OF_COMPUTORS)
#define FIRST_TICK_TRANSACTION_OFFSET sizeof(unsigned long long)
#define MAX_TRANSACTION_SIZE (MAX_INPUT_SIZE + sizeof(Transaction) + SIGNATURE_SIZE)

#define INTERNAL_COMPUTATIONS_INTERVAL 676
#define EXTERNAL_COMPUTATIONS_INTERVAL (676 + 1)
static_assert(INTERNAL_COMPUTATIONS_INTERVAL >= NUMBER_OF_COMPUTORS, "Internal computation phase needs to be at least equal NUMBER_OF_COMPUTORS");

// List of start-end for full external computation times. The event must not be overlap.
// Format is DoW-hh-mm-ss in hex format, total 4 bytes, each use 1 bytes
// DoW: Day of the week 0: Sunday, 1 = Monday ...
static unsigned int gFullExternalComputationTimes[][2] =
{
    {0x040C0000U, 0x050C0000U}, // Thu 12:00:00 - Fri 12:00:00
    {0x060C0000U, 0x000C0000U}, // Sat 12:00:00 - Sun 12:00:00
    {0x010C0000U, 0x020C0000U}, // Mon 12:00:00 - Tue 12:00:00
};

#define STACK_SIZE 4194304
#define TRACK_MAX_STACK_BUFFER_SIZE
