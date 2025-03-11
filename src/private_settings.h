#pragma once

////////// Private Settings \\\\\\\\\\

// Do NOT share the data of "Private Settings" section with anybody!!!

#define OPERATOR "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"

static unsigned char computorSeeds[][55 + 1] = {
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
};

// Enter static IPs of peers (ideally at least 4 including your own IP) to disseminate them to other peers.
// You can find current peer IPs at https://app.qubic.li/network/live
static const unsigned char knownPublicPeers[][4] = {
    {127, 0, 0, 1}, // REMOVE THIS ENTRY AND REPLACE IT WITH YOUR OWN IP ADDRESSES
};

/* Whitelisting has been disabled, as requesting the IP of the incoming connection freezes the node occasionally
// Enter static IPs that shall be prioritized in incoming connection
// There are connection slots reserved for those whitelist IPs
static const unsigned char whiteListPeers[][4] = {
     {127, 0, 0, 1}, // REMOVE THIS ENTRY AND REPLACE IT WITH YOUR OWN IP ADDRESSES
};
*/

#define LOG_QU_TRANSFERS 0 // "0" disables logging, "1" enables it
#define LOG_BURNINGS 0
#define LOG_DUST_BURNINGS 0
#define LOG_SPECTRUM_STATS 0
#define LOG_ASSET_ISSUANCES 0
#define LOG_ASSET_OWNERSHIP_CHANGES 0
#define LOG_ASSET_POSSESSION_CHANGES 0
#define LOG_ASSET_OWNERSHIP_MANAGING_CONTRACT_CHANGES 0
#define LOG_ASSET_POSSESSION_MANAGING_CONTRACT_CHANGES 0
#define LOG_CONTRACT_ERROR_MESSAGES 0
#define LOG_CONTRACT_WARNING_MESSAGES 0
#define LOG_CONTRACT_INFO_MESSAGES 0
#define LOG_CONTRACT_DEBUG_MESSAGES 0
#define LOG_CUSTOM_MESSAGES 0
static unsigned long long logReaderPasscodes[4] = {
    0, 0, 0, 0 // REMOVE THIS ENTRY AND REPLACE IT WITH YOUR OWN RANDOM NUMBERS IN [0..18446744073709551615] RANGE IF LOGGING IS ENABLED
};

// Mode for auto save ticks:
// 0: disable
// 1: save tick storage every TICK_STORAGE_AUTOSAVE_TICK_PERIOD ticks, only AUX mode
#define TICK_STORAGE_AUTOSAVE_MODE 0
// NOTE: Strategy to pick TICK_STORAGE_AUTOSAVE_TICK_PERIOD:
// Although the default value is 1000, there is a chance that your node can be misaligned at tick XXXX2000,XXXX3000,XXXX4000,... 
// Perform state persisting when your node is misaligned will also make your node misaligned after resuming.
// Thus, picking various TICK_STORAGE_AUTOSAVE_TICK_PERIOD numbers across AUX nodes is recommended.
// some suggested prime numbers you can try: 971 977 983 991 997
#define TICK_STORAGE_AUTOSAVE_TICK_PERIOD 1000