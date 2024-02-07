#pragma once

////////// Private Settings \\\\\\\\\\

// Do NOT share the data of "Private Settings" section with anybody!!!

#define OPERATOR "MEFKYFCDXDUILCAJKOIKWQAPENJDUHSSYPBRWFOTLALILAYWQFDSITJELLHG"

static unsigned char computorSeeds[][55 + 1] = {
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
};

// Enter static IPs of peers (ideally at least 4 including your own IP) to disseminate them to other peers.
// You can find current peer IPs at https://app.qubic.li/network/live
static const unsigned char knownPublicPeers[][4] = {
    {127, 0, 0, 1}, // REMOVE THIS ENTRY AND REPLACE IT WITH YOUR OWN IP ADDRESSES
};

#define LOG_BUFFER_SIZE 16777200 // Must be less or equal to 16777200
#define LOG_QU_TRANSFERS 1 // "0" disables logging, "1" enables it
#define LOG_QU_TRANSFERS_TRACK_TRANSFER_ID 1
#define LOG_ASSET_ISSUANCES 1
#define LOG_ASSET_OWNERSHIP_CHANGES 1
#define LOG_ASSET_POSSESSION_CHANGES 1
#define LOG_CONTRACT_ERROR_MESSAGES 1
#define LOG_CONTRACT_WARNING_MESSAGES 1
#define LOG_CONTRACT_INFO_MESSAGES 1
#define LOG_CONTRACT_DEBUG_MESSAGES 1
#define LOG_BURNINGS 1
#define LOG_CUSTOM_MESSAGES 1
static unsigned long long logReaderPasscodes[][4] = {
    {1, 2, 3, 4}, // change the passcode if needed
    {5, 6, 7, 8}, 
};