// Defines messages used in the "private" communication channel between core and oracle machine node.
// These are used as the payload together with RequestResponseHeader.
//
// The connection between oracle machine node and core node is an outgoing connection of the core, permanently kept open.
// The OM node implements whitelisting of IPs, only allowing a specific set of core nodes to connect.

#pragma once

#include "network_messages/common_def.h"


// Message sent from Core Node to OM Node in order to query data.
// Behind this struct, the oracle query data is attached, with size and content defined by oracleInterfaceIdx.
struct OracleMachineQuery
{
    /// Type to be used in RequestResponseHeader
    static constexpr unsigned char type = 190;

    /// Query ID for later accociating reply with query
    unsigned long long oracleQueryId;

    /// Oracle interface, defining query data / reply data format
    unsigned int oracleInterfaceIndex;

    /// Timeout in milliseconds, defining OM can give up to get reply from oracle (no response needed)
    unsigned int timeoutInMilliseconds;
};

// Message sent from OM Node to Core Node in order to send the oracle reply data (following query).
// Behind this struct, the oracle reply data is attached, with size and content defined by oracleInterfaceIdx of the oracle query.
struct OracleMachineReply
{
    /// Type to be used in RequestResponseHeader
    static constexpr unsigned char type = 191;

    /// Query ID that links reply with query
    unsigned long long oracleQueryId;

    /// Allow to return error flags in order to simplify debugging (see network_messages/common_def.h for predefined error flags)
    unsigned short oracleMachineErrorFlags;

    unsigned short _padding0;
    unsigned int _padding1;
};

// TODO: maybe later extend the protocol to handle subscriptions more efficiently
// - Core Node sends all subscriptions to OM Node when the connection to the OM Node is established
// - Core Node sends updates on subscriptions/unsubscription
// - Core Node sends oracleQueryId + timestamp for each query in a subscription instead of QueryOracleFromOracleMachineNode
//   (without oracle query data, because except for timestamp, the oracle query data stays constant in a subscription)
