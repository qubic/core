// Defines messages used in the "private" communication channel between core and OC machine node.
// These are used as the payload together with RequestResponseHeader.
//
// The connection between OC machine node and core node is an outgoing connection of the core, permanently kept open.
// The OC machine node implements whitelisting of IPs, only allowing a specific set of core nodes to connect.
//
// Mirrors core_om_network_messages.h.

#pragma once

#include "network_messages/common_def.h"


// Single (computor index, signature) pair in an OcMachineInvocation authorization bundle.
struct SignerEntry
{
    unsigned short computorIndex;            // index into the issuing epoch's computor list
    unsigned char signature[SIGNATURE_SIZE]; // 64 bytes; SchnorrQ over authMessage
};

static_assert(sizeof(SignerEntry) == 66, "SignerEntry must be exactly 66 bytes.");


// Message sent from Core Node to OC machine Node when an invocation transitions to AUTHORIZED.
// Carries the pinned OcRequest payload followed by QUORUM SignerEntries.
//
// Wire layout:
//     OcMachineInvocation header (fixed)
//     unsigned char requestData[requestSize]
//     SignerEntry signers[signatureCount]
//
// Transmitted ONLY over the private out-of-band channel. Not subject to MAX_INPUT_SIZE.
// Maximum size: 16 + MAX_OC_REQUEST_SIZE + QUORUM * sizeof(SignerEntry) ≈ 30,790 bytes.
struct OcMachineInvocation
{
    /// Type to be used in RequestResponseHeader
    static constexpr unsigned char type()
    {
        return NetworkMessageType::OC_MACHINE_INVOCATION;
    }

    long long invocationId;          // 8 bytes
    unsigned short epoch;            // 2 bytes
    unsigned short interfaceIndex;   // 2 bytes
    unsigned short requestSize;      // 2 bytes
    unsigned short signatureCount;   // 2 bytes; MUST equal QUORUM
    // followed by: requestSize bytes of pinned OcRequest payload
    // followed by: signatureCount x SignerEntry
};

static_assert(sizeof(OcMachineInvocation) == 16, "OcMachineInvocation header must be exactly 16 bytes.");
