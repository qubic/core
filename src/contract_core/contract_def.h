#pragma once

#include "platform/m256.h"

////////// Smart contracts \\\\\\\\\\

// The order in this file is very important, because it restricts what is available to the contracts.
// For example, a contract may only call a contract with lower index, which is enforced by order of
// include / availability of definition.
// Additionally, most types, functions, and variables of the core have to be defined after including
// the contract to keep them unavailable in the contract code.

namespace QPI
{
    struct QpiContextProcedureCall;
    struct QpiContextFunctionCall;
}

// TODO: add option for having locals to SYSTEM and EXPAND procedures
typedef void (*SYSTEM_PROCEDURE)(const QPI::QpiContextProcedureCall&, void* state, void* input, void* output, void* locals);
typedef void (*EXPAND_PROCEDURE)(const QPI::QpiContextFunctionCall&, void*, void*); // cannot not change anything except state
typedef void (*USER_FUNCTION)(const QPI::QpiContextFunctionCall&, void* state, void* input, void* output, void* locals);
typedef void (*USER_PROCEDURE)(const QPI::QpiContextProcedureCall&, void* state, void* input, void* output, void* locals);

constexpr unsigned long long MAX_CONTRACT_STATE_SIZE = 1073741824;

// Maximum size of local variables that may be used by a contract function or procedure
// If increased, the size of contractLocalsStack should be increased as well.
constexpr unsigned int MAX_SIZE_OF_CONTRACT_LOCALS = 32 * 1024;

// TODO: make sure the limit of nested calls is not violated
constexpr unsigned short MAX_NESTED_CONTRACT_CALLS = 10;


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


// With no other includes before, the following are the only headers available to contracts.
// When adding something, be cautious to keep access of contracts limited to safe features only.
#include "contracts/qpi.h"
#include "qpi_proposal_voting.h"

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

#undef CONTRACT_INDEX
#undef CONTRACT_STATE_TYPE
#undef CONTRACT_STATE2_TYPE

#define GQMPROP_CONTRACT_INDEX 6
#define CONTRACT_INDEX GQMPROP_CONTRACT_INDEX
#define CONTRACT_STATE_TYPE GQMPROP
#define CONTRACT_STATE2_TYPE GQMPROP2
#include "contracts/GeneralQuorumProposal.h"

#undef CONTRACT_INDEX
#undef CONTRACT_STATE_TYPE
#undef CONTRACT_STATE2_TYPE

#define SWATCH_CONTRACT_INDEX 7
#define CONTRACT_INDEX SWATCH_CONTRACT_INDEX
#define CONTRACT_STATE_TYPE SWATCH
#define CONTRACT_STATE2_TYPE SWATCH2
#include "contracts/SupplyWatcher.h"

#undef CONTRACT_INDEX
#undef CONTRACT_STATE_TYPE
#undef CONTRACT_STATE2_TYPE

#define CCF_CONTRACT_INDEX 8
#define CONTRACT_INDEX CCF_CONTRACT_INDEX
#define CONTRACT_STATE_TYPE CCF
#define CONTRACT_STATE2_TYPE CCF2
#include "contracts/ComputorControlledFund.h"

#undef CONTRACT_INDEX
#undef CONTRACT_STATE_TYPE
#undef CONTRACT_STATE2_TYPE

#define QEARN_CONTRACT_INDEX 9
#define CONTRACT_INDEX QEARN_CONTRACT_INDEX
#define CONTRACT_STATE_TYPE QEARN
#define CONTRACT_STATE2_TYPE QEARN2
#include "contracts/Qearn.h"

#undef CONTRACT_INDEX
#undef CONTRACT_STATE_TYPE
#undef CONTRACT_STATE2_TYPE

#define QVAULT_CONTRACT_INDEX 10
#define CONTRACT_INDEX QVAULT_CONTRACT_INDEX
#define CONTRACT_STATE_TYPE QVAULT
#define CONTRACT_STATE2_TYPE QVAULT2
#include "contracts/QVAULT.h"

#undef CONTRACT_INDEX
#undef CONTRACT_STATE_TYPE
#undef CONTRACT_STATE2_TYPE

#define ETHBRIDGE_CONTRACT_INDEX 11
#define CONTRACT_INDEX ETHBRIDGE_CONTRACT_INDEX
#define CONTRACT_STATE_TYPE ETHBRIDGE
#define CONTRACT_STATE2_TYPE ETHBRIDGE2
#include "contracts/EthBridge.h"

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

// The following are included after the contracts to keep their definitions and dependencies
// inaccessible for contracts
#include "qpi_collection_impl.h"
#include "qpi_trivial_impl.h"

#include "platform/global_var.h"

#include "network_messages/common_def.h"

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
    {"GQMPROP", 123, 10000, sizeof(GQMPROP)},
    {"SWATCH", 123, 10000, sizeof(IPO)},
    {"CCF", 127, 10000, sizeof(CCF)}, // proposal in epoch 125, IPO in 126, construction and first use in 127
    {"QEARN", 137, 10000, sizeof(QEARN)}, // proposal in epoch 135, IPO in 136, construction in 137 / first donation after END_EPOCH, first round in epoch 138
    {"QVAULT", 138, 10000, sizeof(IPO)}, // proposal in epoch 136, IPO in 137, construction and first use in 138
    {"BRIDGE", 136, 10000, sizeof(ETHBRIDGE)}, // proposal in epoch 136, IPO in 137, construction and first use in 138
};

constexpr unsigned int contractCount = sizeof(contractDescriptions) / sizeof(contractDescriptions[0]);

GLOBAL_VAR_DECL EXPAND_PROCEDURE contractExpandProcedures[contractCount];

// TODO: all below are filled very sparsely, so a better data structure could save almost all the memory
GLOBAL_VAR_DECL USER_FUNCTION contractUserFunctions[contractCount][65536];
GLOBAL_VAR_DECL unsigned short contractUserFunctionInputSizes[contractCount][65536];
GLOBAL_VAR_DECL unsigned short contractUserFunctionOutputSizes[contractCount][65536];
// This has been changed to unsigned short to avoid the misalignment issue happening in epochs 109 and 110,
// probably due to too high numbers in contractUserProcedureLocalsSizes causing stack buffer alloc to fail
// probably due to buffer overflow that is difficult to reproduce in test net
// TODO: change back to unsigned int
GLOBAL_VAR_DECL unsigned short contractUserFunctionLocalsSizes[contractCount][65536];
GLOBAL_VAR_DECL USER_PROCEDURE contractUserProcedures[contractCount][65536];
GLOBAL_VAR_DECL unsigned short contractUserProcedureInputSizes[contractCount][65536];
GLOBAL_VAR_DECL unsigned short contractUserProcedureOutputSizes[contractCount][65536];
// This has been changed to unsigned short to avoid the misalignment issue happening in epochs 109 and 110,
// probably due to too high numbers in contractUserProcedureLocalsSizes causing stack buffer alloc to fail
// probably due to buffer overflow that is difficult to reproduce in test net
// TODO: change back to unsigned int
GLOBAL_VAR_DECL unsigned short contractUserProcedureLocalsSizes[contractCount][65536];

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

GLOBAL_VAR_DECL SYSTEM_PROCEDURE contractSystemProcedures[contractCount][contractSystemProcedureCount];
GLOBAL_VAR_DECL unsigned short contractSystemProcedureLocalsSizes[contractCount][contractSystemProcedureCount];


#define REGISTER_CONTRACT_FUNCTIONS_AND_PROCEDURES(contractName) { \
constexpr unsigned int contractIndex = contractName##_CONTRACT_INDEX; \
if (!contractName::__initializeEmpty) contractSystemProcedures[contractIndex][INITIALIZE] = (SYSTEM_PROCEDURE)contractName::__initialize;\
contractSystemProcedureLocalsSizes[contractIndex][INITIALIZE] = contractName::__initializeLocalsSize; \
if (!contractName::__beginEpochEmpty) contractSystemProcedures[contractIndex][BEGIN_EPOCH] = (SYSTEM_PROCEDURE)contractName::__beginEpoch;\
contractSystemProcedureLocalsSizes[contractIndex][BEGIN_EPOCH] = contractName::__beginEpochLocalsSize; \
if (!contractName::__endEpochEmpty) contractSystemProcedures[contractIndex][END_EPOCH] = (SYSTEM_PROCEDURE)contractName::__endEpoch;\
contractSystemProcedureLocalsSizes[contractIndex][END_EPOCH] = contractName::__endEpochLocalsSize; \
if (!contractName::__beginTickEmpty) contractSystemProcedures[contractIndex][BEGIN_TICK] = (SYSTEM_PROCEDURE)contractName::__beginTick;\
contractSystemProcedureLocalsSizes[contractIndex][BEGIN_TICK] = contractName::__beginTickLocalsSize; \
if (!contractName::__endTickEmpty) contractSystemProcedures[contractIndex][END_TICK] = (SYSTEM_PROCEDURE)contractName::__endTick;\
contractSystemProcedureLocalsSizes[contractIndex][END_TICK] = contractName::__endTickLocalsSize; \
if (!contractName::__preAcquireSharesEmpty) contractSystemProcedures[contractIndex][PRE_ACQUIRE_SHARES] = (SYSTEM_PROCEDURE)contractName::__preAcquireShares;\
contractSystemProcedureLocalsSizes[contractIndex][PRE_ACQUIRE_SHARES] = contractName::__preAcquireSharesSize; \
if (!contractName::__preReleaseSharesEmpty) contractSystemProcedures[contractIndex][PRE_RELEASE_SHARES] = (SYSTEM_PROCEDURE)contractName::__preReleaseShares;\
contractSystemProcedureLocalsSizes[contractIndex][PRE_RELEASE_SHARES] = contractName::__preReleaseSharesSize; \
if (!contractName::__postAcquireSharesEmpty) contractSystemProcedures[contractIndex][POST_ACQUIRE_SHARES] = (SYSTEM_PROCEDURE)contractName::__postAcquireShares;\
contractSystemProcedureLocalsSizes[contractIndex][POST_ACQUIRE_SHARES] = contractName::__postAcquireSharesSize; \
if (!contractName::__postReleaseSharesEmpty) contractSystemProcedures[contractIndex][POST_RELEASE_SHARES] = (SYSTEM_PROCEDURE)contractName::__postReleaseShares;\
contractSystemProcedureLocalsSizes[contractIndex][POST_RELEASE_SHARES] = contractName::__postReleaseSharesSize; \
if (!contractName::__expandEmpty) contractExpandProcedures[contractIndex] = (EXPAND_PROCEDURE)contractName::__expand;\
QpiContextForInit qpi(contractIndex); \
contractName::__registerUserFunctionsAndProcedures(qpi); \
}


static void initializeContracts()
{
    REGISTER_CONTRACT_FUNCTIONS_AND_PROCEDURES(QX);
    REGISTER_CONTRACT_FUNCTIONS_AND_PROCEDURES(QUOTTERY);
    REGISTER_CONTRACT_FUNCTIONS_AND_PROCEDURES(RANDOM);
    REGISTER_CONTRACT_FUNCTIONS_AND_PROCEDURES(QUTIL);
    REGISTER_CONTRACT_FUNCTIONS_AND_PROCEDURES(MLM);
    REGISTER_CONTRACT_FUNCTIONS_AND_PROCEDURES(GQMPROP);
    REGISTER_CONTRACT_FUNCTIONS_AND_PROCEDURES(SWATCH);
    REGISTER_CONTRACT_FUNCTIONS_AND_PROCEDURES(CCF);
    REGISTER_CONTRACT_FUNCTIONS_AND_PROCEDURES(QEARN);
    REGISTER_CONTRACT_FUNCTIONS_AND_PROCEDURES(QVAULT);
    REGISTER_CONTRACT_FUNCTIONS_AND_PROCEDURES(ETHBRIDGE);
}
