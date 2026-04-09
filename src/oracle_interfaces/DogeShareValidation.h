#pragma once

#include "network_messages/common_def.h"
#include "oracle_core/oracle_transactions.h"

using namespace QPI;

/**
* Oracle interface "DogeShareValidation" (see Price.h for general documentation about oracle interfaces).
*
* This is used for validating Doge shares.
*/
struct DogeShareValidation
{
    /// Oracle interface index
    static constexpr uint32 oracleInterfaceIndex = ORACLE_INTERFACE_INDEX;

    /// Oracle query data / input to the oracle machine
    struct OracleQuery
    {
        Array<uint8, 32> target;

        Array<uint8, 4> solutionTime;
        Array<uint8, 4> solutionNonce;
        Array<uint8, 8> solutionExtraNonce2;
        
        Array<uint8, 4> taskPartialHeaderVersion;
        Array<uint8, 4> taskPartialHeaderDifficultyNBits;
        Array<uint8, 32> taskPartialHeaderPrevBlockHash;
        uint32 extraNonce1NumBytes;
        uint32 coinbase1NumBytes;
        uint32 coinbase2NumBytes;
        uint32 numMerkleBranches;

        static constexpr uint64 additionalDataSize = MAX_INPUT_SIZE
            - (32 + 4 + 4 + 8 + 4 + 4 + 32 + 4 * sizeof(uint32)) // size of all previous struct members
            - OracleUserQueryTransactionPrefix::minInputSize();

        uint8 additionalData[additionalDataSize];
        // Layout for additional data:
        // - extraNonce1
        // - coinbase1
        // - coinbase2
        // - merkleBranch1NumBytes (unsigned int), ... , merkleBranchNNumBytes (unsigned int)
        // - merkleBranch1, ... , merkleBranchN
        // Note: The size of the components contained in the additional data varies, hence the total occupied bytes in the array is not fixed.
    };

    static_assert(sizeof(OracleQuery) + OracleUserQueryTransactionPrefix::minInputSize() == MAX_INPUT_SIZE, "DogeShareValidation::OracleQuery has wrong size");

    /// Oracle reply data / output of the oracle machine
    struct OracleReply
    {
        bit isValid;
        uint32 compIndex;
    };

    /// Return query fee, which may depend on the specific query (for example on the oracle).
    static sint64 getQueryFee(const OracleQuery& query)
    {
        return 1000;
    }
};
