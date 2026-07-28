#ifndef CROSSCHAINBRIDGE_H
#define CROSSCHAINBRIDGE_H

#include "qpi.h"

#define CCB_CONTRACT_INDEX 5
#define CCB_MAX_VALIDATORS 16
#define CCB_REPLAY_BUFFER_SIZE 256

struct CROSSCHAINBRIDGE {
    id contractOwner;
    uint8 isPaused;
    uint64 totalLockedQus;
    uint64 totalBurnedFees;
    uint64 fixedBridgeFee;

    id validators[CCB_MAX_VALIDATORS];
    uint32 validatorCount;
    uint32 requiredSignatures;

    uint64 nonce;

    uint8_32 processedTxHashes[CCB_REPLAY_BUFFER_SIZE];
    uint32 replayBufferIndex;
};

struct CCB_LockQus_input {
    uint8_32 destinationChainAddress;
    uint32 destinationChainId;
    uint64 amount;
};

struct CCB_LockQus_output {
    uint64 lockTxNonce;
    uint8 status;
};

struct CCB_UnlockQus_input {
    uint8_32 txHash;
    uint8_32 targetQubicAddress;
    uint64 amount;
    uint64 lockTxNonce;
    uint8 signatures[CCB_MAX_VALIDATORS][64];
};

struct CCB_UnlockQus_output {
    uint8 status;
};

struct CCB_SetPause_input {
    id caller;
    uint8 pauseState;
};

struct CCB_SetPause_output {
    uint8 status;
};

struct CCB_SetQuorum_input {
    id caller;
    uint32 newRequiredSignatures;
};

struct CCB_SetQuorum_output {
    uint8 status;
};

struct CCB_SetFee_input {
    id caller;
    uint64 newFee;
};

struct CCB_SetFee_output {
    uint8 status;
};

struct CCB_GetStateSummary_output {
    uint64 totalLockedQus;
    uint64 totalBurnedFees;
    uint64 nonce;
    uint32 validatorCount;
    uint32 requiredSignatures;
    uint8 isPaused;
};

#endif // CROSSCHAINBRIDGE_H
