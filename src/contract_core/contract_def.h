#pragma once

#include "platform/m256.h"

#include "network_messages/common_def.h"

////////// Smart contracts \\\\\\\\\\

// The order in this file is very important, because it restricts what is available to the contracts.
// For example, a contract may only call a contract with lower index, which is enforced by order of
// include / availability of definition.

namespace QPI
{
    struct QpiContextProcedureCall;
    struct QpiContextFunctionCall;
}

// TODO: add option for having locals to SYSTEM and EXPAND procedures
typedef void (*SYSTEM_PROCEDURE)(const QPI::QpiContextProcedureCall&, void* state, void* input, void* output);
typedef void (*EXPAND_PROCEDURE)(const QPI::QpiContextProcedureCall&, void*, void*);
typedef void (*USER_FUNCTION)(const QPI::QpiContextFunctionCall&, void* state, void* input, void* output, void* locals);
typedef void (*USER_PROCEDURE)(const QPI::QpiContextProcedureCall&, void* state, void* input, void* output, void* locals);

// Maximum size of local variables that may be used by a contract function or procedure
// If increased, the size of contractLocalsStack should be increased as well.
constexpr unsigned int MAX_SIZE_OF_CONTRACT_LOCALS = 32 * 1024;

// TODO: make sure the limit of nested calls is not violated
constexpr unsigned short MAX_NESTED_CONTRACT_CALLS = 10;


// TODO: rename these to __qpiX (forbidding to call them outside macros)
static void __beginFunctionOrProcedure(const unsigned int); // TODO: more human-readable form of function ID?
static void __endFunctionOrProcedure(const unsigned int);
template <typename T> static m256i __K12(T);
template <typename T> static void __logContractDebugMessage(unsigned int, T&);
template <typename T> static void __logContractErrorMessage(unsigned int, T&);
template <typename T> static void __logContractInfoMessage(unsigned int, T&);
template <typename T> static void __logContractWarningMessage(unsigned int, T&);
static void* __scratchpad();    // TODO: concurrency support (n buffers for n allowed concurrent contract executions)
// static void* __tryAcquireScratchpad(unsigned int size);  // Thread-safe, may return nullptr if no appropriate buffer is available
// static void __ReleaseScratchpad(void*);

template <unsigned int functionOrProcedureId>
struct __FunctionOrProcedureBeginEndGuard
{
    // Constructor calling __beginFunctionOrProcedure()
    __FunctionOrProcedureBeginEndGuard()
    {
        __beginFunctionOrProcedure(functionOrProcedureId);
    }

    // Destructor making sure __endFunctionOrProcedure() is called for every return path
    ~__FunctionOrProcedureBeginEndGuard()
    {
        __endFunctionOrProcedure(functionOrProcedureId);
    }
};


#include "contracts/qpi.h"

#define QX_CONTRACT_INDEX 1
#define CONTRACT_INDEX QX_CONTRACT_INDEX
#define CONTRACT_STATE_TYPE QX
#define CONTRACT_STATE2_TYPE QX2
#include "contracts/Qx.h"

#undef CONTRACT_INDEX
#undef CONTRACT_STATE_TYPE
#undef CONTRACT_STATE2_TYPE

#define QUOTTERY_CONTRACT_INDEX 2
#define CONTRACT_INDEX QUOTTERY_CONTRACT_INDEX
#define CONTRACT_STATE_TYPE QUOTTERY
#define CONTRACT_STATE2_TYPE QUOTTERY2
#include "contracts/Quottery.h"

#undef CONTRACT_INDEX
#undef CONTRACT_STATE_TYPE
#undef CONTRACT_STATE2_TYPE

#define RANDOM_CONTRACT_INDEX 3
#define CONTRACT_INDEX RANDOM_CONTRACT_INDEX
#define CONTRACT_STATE_TYPE RANDOM
#define CONTRACT_STATE2_TYPE RANDOM2
#include "contracts/Random.h"

#undef CONTRACT_INDEX
#undef CONTRACT_STATE_TYPE
#undef CONTRACT_STATE2_TYPE

#define QUTIL_CONTRACT_INDEX 4
#define CONTRACT_INDEX QUTIL_CONTRACT_INDEX
#define CONTRACT_STATE_TYPE QUTIL
#define CONTRACT_STATE2_TYPE QUTIL2
#include "contracts/QUtil.h"

#undef CONTRACT_INDEX
#undef CONTRACT_STATE_TYPE
#undef CONTRACT_STATE2_TYPE

#define MLM_CONTRACT_INDEX 5
#define CONTRACT_INDEX MLM_CONTRACT_INDEX
#define CONTRACT_STATE_TYPE MLM
#define CONTRACT_STATE2_TYPE MLM2
#include "contracts/MyLastMatch.h"

#define MAX_CONTRACT_ITERATION_DURATION 0 // In milliseconds, must be above 0; for now set to 0 to disable timeout, because a rollback mechanism needs to be implemented to properly handle timeout

#undef INITIALIZE
#undef BEGIN_EPOCH
#undef END_EPOCH
#undef BEGIN_TICK
#undef END_TICK
#undef PRE_RELEASE_SHARES
#undef PRE_ACQUIRE_SHARES
#undef POST_RELEASE_SHARES
#undef POST_ACQUIRE_SHARES


struct Contract0State
{
    long long contractFeeReserves[MAX_NUMBER_OF_CONTRACTS];
};

struct IPO
{
    m256i publicKeys[NUMBER_OF_COMPUTORS];
    long long prices[NUMBER_OF_COMPUTORS];
};

static_assert(sizeof(IPO) == 32 * NUMBER_OF_COMPUTORS + 8 * NUMBER_OF_COMPUTORS, "Something is wrong with the struct size.");


constexpr struct ContractDescription
{
    char assetName[8];
    // constructionEpoch needs to be set to after IPO (IPO is before construction)
    unsigned short constructionEpoch, destructionEpoch;
    unsigned long long stateSize;
} contractDescriptions[] = {
    {"", 0, 0, sizeof(Contract0State)},
    {"QX", 66, 10000, sizeof(QX)},
    {"QTRY", 72, 10000, sizeof(QUOTTERY)},
    {"RANDOM", 88, 10000, sizeof(IPO)},
    {"QUTIL", 99, 10000, sizeof(IPO)},
    {"MLM", 112, 10000, sizeof(IPO)},
};

constexpr unsigned int contractCount = sizeof(contractDescriptions) / sizeof(contractDescriptions[0]);

static EXPAND_PROCEDURE contractExpandProcedures[contractCount];

// TODO: all below are filled very sparsely, so a better data structure could save almost all the memory
static USER_FUNCTION contractUserFunctions[contractCount][65536];
static unsigned short contractUserFunctionInputSizes[contractCount][65536];
static unsigned short contractUserFunctionOutputSizes[contractCount][65536];
// This has been changed to unsigned short to avoid the misalignment issue happening in epochs 109 and 110,
// probably due to too high numbers in contractUserProcedureLocalsSizes causing stack buffer alloc to fail
// probably due to buffer overflow that is difficult to reproduce in test net
// TODO: change back to unsigned int
static unsigned short contractUserFunctionLocalsSizes[contractCount][65536];
static USER_PROCEDURE contractUserProcedures[contractCount][65536];
static unsigned short contractUserProcedureInputSizes[contractCount][65536];
static unsigned short contractUserProcedureOutputSizes[contractCount][65536];
// This has been changed to unsigned short to avoid the misalignment issue happening in epochs 109 and 110,
// probably due to too high numbers in contractUserProcedureLocalsSizes causing stack buffer alloc to fail
// probably due to buffer overflow that is difficult to reproduce in test net
// TODO: change back to unsigned int
static unsigned short contractUserProcedureLocalsSizes[contractCount][65536];

enum SystemProcedureID
{
    INITIALIZE = 0,
    BEGIN_EPOCH,
    END_EPOCH,
    BEGIN_TICK,
    END_TICK,
    PRE_RELEASE_SHARES,
    PRE_ACQUIRE_SHARES,
    POST_RELEASE_SHARES,
    POST_ACQUIRE_SHARES,
    contractSystemProcedureCount,
};

enum MoreProcedureIDs
{
    // Used together with SystemProcedureID values, so there must be not overlap!
    USER_PROCEDURE_CALL = contractSystemProcedureCount + 1,
};

static SYSTEM_PROCEDURE contractSystemProcedures[contractCount][contractSystemProcedureCount];


#define REGISTER_CONTRACT_FUNCTIONS_AND_PROCEDURES(contractName)\
contractSystemProcedures[contractIndex][INITIALIZE] = (SYSTEM_PROCEDURE)contractName::__initialize;\
contractSystemProcedures[contractIndex][BEGIN_EPOCH] = (SYSTEM_PROCEDURE)contractName::__beginEpoch;\
contractSystemProcedures[contractIndex][END_EPOCH] = (SYSTEM_PROCEDURE)contractName::__endEpoch;\
contractSystemProcedures[contractIndex][BEGIN_TICK] = (SYSTEM_PROCEDURE)contractName::__beginTick;\
contractSystemProcedures[contractIndex][END_TICK] = (SYSTEM_PROCEDURE)contractName::__endTick;\
contractSystemProcedures[contractIndex][PRE_RELEASE_SHARES] = (SYSTEM_PROCEDURE)contractName::__preReleaseShares;\
contractSystemProcedures[contractIndex][PRE_ACQUIRE_SHARES] = (SYSTEM_PROCEDURE)contractName::__preAcquireShares;\
contractSystemProcedures[contractIndex][POST_RELEASE_SHARES] = (SYSTEM_PROCEDURE)contractName::__postReleaseShares;\
contractSystemProcedures[contractIndex][POST_ACQUIRE_SHARES] = (SYSTEM_PROCEDURE)contractName::__postAcquireShares;\
contractExpandProcedures[contractIndex] = (EXPAND_PROCEDURE)contractName::__expand;\
((contractName*)contractState)->__registerUserFunctionsAndProcedures(qpi);


static void initializeContract(const unsigned int contractIndex, void* contractState)
{
    QpiContextForInit qpi(contractIndex);
    switch (contractIndex)
    {
    case QX_CONTRACT_INDEX:
    {
        REGISTER_CONTRACT_FUNCTIONS_AND_PROCEDURES(QX);
    }
    break;

    case QUOTTERY_CONTRACT_INDEX:
    {
        REGISTER_CONTRACT_FUNCTIONS_AND_PROCEDURES(QUOTTERY);
    }
    break;

    case RANDOM_CONTRACT_INDEX:
    {
        REGISTER_CONTRACT_FUNCTIONS_AND_PROCEDURES(RANDOM);
    }
    break;

    case QUTIL_CONTRACT_INDEX:
    {
        REGISTER_CONTRACT_FUNCTIONS_AND_PROCEDURES(QUTIL);
    }
    break;

    case MLM_CONTRACT_INDEX:
    {
        REGISTER_CONTRACT_FUNCTIONS_AND_PROCEDURES(MLM);
    }
    break;
    }
}
