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
typedef void (*SYSTEM_PROCEDURE)(const QPI::QpiContextProcedureCall&, void*);
typedef void (*EXPAND_PROCEDURE)(const QPI::QpiContextProcedureCall&, void*, void*);
typedef void (*USER_FUNCTION)(const QPI::QpiContextFunctionCall&, void* state, void* input, void* output, void* locals);
typedef void (*USER_PROCEDURE)(const QPI::QpiContextProcedureCall&, void* state, void* input, void* output, void* locals);

// Maximum size of local variables that may be used by a contract function or procedure
// If increased, the size of contractLocalsStack should be increased as well.
constexpr unsigned int MAX_SIZE_OF_CONTRACT_LOCALS = 128 * 1024;

// TODO: make sure the limit of nested calls is not violated
constexpr unsigned short MAX_NESTED_CONTRACT_CALLS = 10;


// TODO: rename these to __qpiX (forbidding to call them outside macros)
static void __beginFunctionOrProcedure(const unsigned int);
static void __endFunctionOrProcedure(const unsigned int);
template <typename T> static m256i __K12(T);
template <typename T> static void __logContractDebugMessage(unsigned int, const T&);
template <typename T> static void __logContractErrorMessage(unsigned int, const T&);
template <typename T> static void __logContractInfoMessage(unsigned int, const T&);
template <typename T> static void __logContractWarningMessage(unsigned int, const T&);
static void* __scratchpad();    // TODO: concurrency support (n buffers for n allowed concurrent contract executions)
// static void* __tryAcquireScratchpad(unsigned int size);  // Thread-safe, may return nullptr if no appropriate buffer is available
// static void __ReleaseScratchpad(void*);

#include "contracts/qpi.h"

#define QX_CONTRACT_INDEX 1
#define CONTRACT_INDEX QX_CONTRACT_INDEX
#define CONTRACT_STATE_TYPE QX
#define CONTRACT_STATE2_TYPE QX2
#include "contracts/Qx.h"
// TODO: remove state variables to prevent manipulation and make sure contracts can only call other contracts through the API
static CONTRACT_STATE_TYPE* _QX;

#undef CONTRACT_INDEX
#undef CONTRACT_STATE_TYPE
#undef CONTRACT_STATE2_TYPE

#define QUOTTERY_CONTRACT_INDEX 2
#define CONTRACT_INDEX QUOTTERY_CONTRACT_INDEX
#define CONTRACT_STATE_TYPE QUOTTERY
#define CONTRACT_STATE2_TYPE QUOTTERY2
#include "contracts/Quottery.h"
static CONTRACT_STATE_TYPE* _QUOTTERY;

#undef CONTRACT_INDEX
#undef CONTRACT_STATE_TYPE
#undef CONTRACT_STATE2_TYPE

#define RANDOM_CONTRACT_INDEX 3
#define CONTRACT_INDEX RANDOM_CONTRACT_INDEX
#define CONTRACT_STATE_TYPE RANDOM
#define CONTRACT_STATE2_TYPE RANDOM2
#include "contracts/Random.h"
static CONTRACT_STATE_TYPE* _RANDOM;

#undef CONTRACT_INDEX
#undef CONTRACT_STATE_TYPE
#undef CONTRACT_STATE2_TYPE

#define QUTIL_CONTRACT_INDEX 4
#define CONTRACT_INDEX QUTIL_CONTRACT_INDEX
#define CONTRACT_STATE_TYPE QUTIL
#define CONTRACT_STATE2_TYPE QUTIL2
#include "contracts/QUtil.h"
static CONTRACT_STATE_TYPE* _QUTIL;

#define MAX_CONTRACT_ITERATION_DURATION 1000 // In milliseconds, must be above 0

#undef INITIALIZE
#undef BEGIN_EPOCH
#undef END_EPOCH
#undef BEGIN_TICK
#undef END_TICK


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
    {"QTRY", 72, 10000, sizeof(IPO)},
    {"RANDOM", 88, 10000, sizeof(IPO)},
    {"QUTIL", 99, 10000, sizeof(IPO)},
};

constexpr unsigned int contractCount = sizeof(contractDescriptions) / sizeof(contractDescriptions[0]);

static SYSTEM_PROCEDURE contractSystemProcedures[contractCount][5];
static EXPAND_PROCEDURE contractExpandProcedures[contractCount];

// TODO: all below are filled very sparsely, so a better data structure could save almost all the memory
static USER_FUNCTION contractUserFunctions[contractCount][65536];
static unsigned short contractUserFunctionInputSizes[contractCount][65536];
static unsigned short contractUserFunctionOutputSizes[contractCount][65536];
static unsigned int contractUserFunctionLocalsSizes[contractCount][65536];
static USER_PROCEDURE contractUserProcedures[contractCount][65536];
static unsigned short contractUserProcedureInputSizes[contractCount][65536];
static unsigned short contractUserProcedureOutputSizes[contractCount][65536];
static unsigned int contractUserProcedureLocalsSizes[contractCount][65536];

// TODO: allow parallel reading with distinguishing read/write lock
static volatile char contractStateLock[contractCount];
static unsigned char* contractStates[contractCount];
static unsigned long long contractTotalExecutionTicks[contractCount];

enum SystemProcedureID
{
    INITIALIZE = 0,
    BEGIN_EPOCH = 1,
    END_EPOCH = 2,
    BEGIN_TICK = 3,
    END_TICK = 4,
};

#define REGISTER_CONTRACT_FUNCTIONS_AND_PROCEDURES(contractName)\
_##contractName = (contractName*)contractState;\
contractSystemProcedures[contractIndex][INITIALIZE] = (SYSTEM_PROCEDURE)contractName::__initialize;\
contractSystemProcedures[contractIndex][BEGIN_EPOCH] = (SYSTEM_PROCEDURE)contractName::__beginEpoch;\
contractSystemProcedures[contractIndex][END_EPOCH] = (SYSTEM_PROCEDURE)contractName::__endEpoch;\
contractSystemProcedures[contractIndex][BEGIN_TICK] = (SYSTEM_PROCEDURE)contractName::__beginTick;\
contractSystemProcedures[contractIndex][END_TICK] = (SYSTEM_PROCEDURE)contractName::__endTick;\
contractExpandProcedures[contractIndex] = (EXPAND_PROCEDURE)contractName::__expand;\
_##contractName->__registerUserFunctionsAndProcedures(qpi);


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
    }
}
