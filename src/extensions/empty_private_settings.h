#pragma once

////////// Private Settings \\\\\\\\\\

// Do NOT share the data of "Private Settings" section with anybody!!!

static std::string OPERATOR = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";

static std::vector<std::string> computorSeeds = {
};

static unsigned char broadcastedComputorSeeds[][55 + 1] = {};

// If you want to fund any of your custom seeds with initial spectrum, add them here.
static unsigned char customSeeds[][55 + 1] = {};

// number of private ips for computor's internal services
// these are the first N ip in knownPublicPeers, these IPs will never be shared or deleted
#define NUMBER_OF_PRIVATE_IP 0
// Enter static IPs of peers (ideally at least 4 including your own IP) to disseminate them to other peers.
// You can find current peer IPs at https://app.qubic.li/network/live
static const unsigned char knownPublicPeers[][4] = {
    {127, 0, 0, 1}, // DONT REMOVE THIS
    // Add more node ips here
};

/* Whitelisting has been disabled, as requesting the IP of the incoming connection freezes the node occasionally
// Enter static IPs that shall be prioritized in incoming connection
// There are connection slots reserved for those whitelist IPs
static const unsigned char whiteListPeers[][4] = {
     {127, 0, 0, 1}, // REMOVE THIS ENTRY AND REPLACE IT WITH YOUR OWN IP ADDRESSES
};
*/

#define ENABLE_QUBIC_LOGGING_EVENT 1 // turn on logging events

// Virtual memory settings for logging
#define LOG_BUFFER_PAGE_SIZE 300000000ULL
#define PMAP_LOG_PAGE_SIZE 30000000ULL
#define IMAP_LOG_PAGE_SIZE 10000ULL
#define VM_NUM_CACHE_PAGE 8

#if ENABLE_QUBIC_LOGGING_EVENT
// DO NOT MODIFY THIS AREA UNLESS YOU ARE DEVELOPING LOGGING FEATURES
#define LOG_UNIVERSE 1
#define LOG_SPECTRUM 1
#define LOG_CONTRACT_ERROR_MESSAGES 1
#define LOG_CONTRACT_WARNING_MESSAGES 1
#define LOG_CONTRACT_INFO_MESSAGES 1
#define LOG_CONTRACT_DEBUG_MESSAGES 1
#define LOG_CUSTOM_MESSAGES 1
#else
#define LOG_UNIVERSE 0
#define LOG_SPECTRUM 0
#define LOG_CONTRACT_ERROR_MESSAGES 0
#define LOG_CONTRACT_WARNING_MESSAGES 0
#define LOG_CONTRACT_INFO_MESSAGES 0
#define LOG_CONTRACT_DEBUG_MESSAGES 0
#define LOG_CUSTOM_MESSAGES 0
#endif

static unsigned long long logReaderPasscodes[4] = {
    0, 0, 0, 0 // REMOVE THIS ENTRY AND REPLACE IT WITH YOUR OWN RANDOM NUMBERS IN [0..18446744073709551615] RANGE IF LOGGING IS ENABLED
};

// Mode for auto save ticks:
// 0: disable
// 1: save tick storage every TICK_STORAGE_AUTOSAVE_TICK_PERIOD ticks, only AUX mode
// 2: save tick storage only when pressing the `F8` key or it is requested remotely
#define TICK_STORAGE_AUTOSAVE_MODE 2 
// NOTE: Strategy to pick TICK_STORAGE_AUTOSAVE_TICK_PERIOD:
// Although the default value is 1000, there is a chance that your node can be misaligned at tick XXXX2000,XXXX3000,XXXX4000,... 
// Perform state persisting when your node is misaligned will also make your node misaligned after resuming.
// Thus, picking various TICK_STORAGE_AUTOSAVE_TICK_PERIOD numbers across AUX nodes is recommended.
// some suggested prime numbers you can try: 971 977 983 991 997
#define TICK_STORAGE_AUTOSAVE_TICK_PERIOD 1337