#pragma once

#include "platform/m256.h"

#include "network_messages/common_def.h"

////////// Smart contracts \\\\\\\\\\

typedef void (*SYSTEM_PROCEDURE)(void*);
typedef void (*EXPAND_PROCEDURE)(void*, void*);
typedef void (*USER_FUNCTION)(void*, void*, void*);
typedef void (*USER_PROCEDURE)(void*, void*, void*);



static const m256i& __arbitrator();
static long long __burn(long long);
static void __beginFunctionOrProcedure(const unsigned int);
static const m256i& __computor(unsigned short);
static unsigned char __day();
static unsigned char __dayOfWeek(unsigned char, unsigned char, unsigned char);
static void __endFunctionOrProcedure(const unsigned int);
static unsigned short __epoch();
static bool __getEntity(const m256i&, ::Entity&);
static unsigned char __hour();
static long long __invocationReward();
static const m256i& __invocator();
static long long __issueAsset(unsigned long long, const m256i&, char, long long, unsigned long long);
template <typename T> static m256i __K12(T);
template <typename T> static void __logContractDebugMessage(T);
template <typename T> static void __logContractErrorMessage(T);
template <typename T> static void __logContractInfoMessage(T);
template <typename T> static void __logContractWarningMessage(T);
static unsigned short __millisecond();
static unsigned char __minute();
static unsigned char __month();
static m256i __nextId(const m256i&);
static long long __numberOfPossessedShares(unsigned long long, const m256i&, const m256i&, const m256i&, unsigned short, unsigned short);
static m256i __originator();
static void __registerUserFunction(USER_FUNCTION, unsigned short, unsigned short, unsigned short);
static void __registerUserProcedure(USER_PROCEDURE, unsigned short, unsigned short, unsigned short);
static void* __scratchpad();
static unsigned char __second();
static unsigned int __tick();
static long long __transfer(const m256i&, long long);
static long long __transferShareOwnershipAndPossession(unsigned long long, const m256i&, const m256i&, const m256i&, long long, const m256i&);
static unsigned char __year();

#include "smart_contracts/qpi.h"

#define QX_CONTRACT_INDEX 1
#define CONTRACT_INDEX QX_CONTRACT_INDEX
#define CONTRACT_STATE_TYPE QX
#define CONTRACT_STATE2_TYPE QX2
#include "smart_contracts/Qx.h"
static CONTRACT_STATE_TYPE* _QX;

#undef CONTRACT_INDEX
#undef CONTRACT_STATE_TYPE
#undef CONTRACT_STATE2_TYPE

#define QUOTTERY_CONTRACT_INDEX 2
#define CONTRACT_INDEX QUOTTERY_CONTRACT_INDEX
#define CONTRACT_STATE_TYPE QUOTTERY
#define CONTRACT_STATE2_TYPE QUOTTERY2
#include "smart_contracts/Quottery.h"
static CONTRACT_STATE_TYPE* _QUOTTERY;

#undef CONTRACT_INDEX
#undef CONTRACT_STATE_TYPE
#undef CONTRACT_STATE2_TYPE

#define RANDOM_CONTRACT_INDEX 3
#define CONTRACT_INDEX RANDOM_CONTRACT_INDEX
#define CONTRACT_STATE_TYPE RANDOM
#define CONTRACT_STATE2_TYPE RANDOM2
#include "smart_contracts/Random.h"
static CONTRACT_STATE_TYPE* _RANDOM;

#undef CONTRACT_INDEX
#undef CONTRACT_STATE_TYPE
#undef CONTRACT_STATE2_TYPE

#define QUTIL_CONTRACT_INDEX 4
#define CONTRACT_INDEX QUTIL_CONTRACT_INDEX
#define CONTRACT_STATE_TYPE QUTIL
#define CONTRACT_STATE2_TYPE QUTIL2
#include "smart_contracts/QUtil.h"
static CONTRACT_STATE_TYPE* _QUTIL;

#define MAX_CONTRACT_ITERATION_DURATION 1000 // In milliseconds, must be above 0

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

static SYSTEM_PROCEDURE contractSystemProcedures[sizeof(contractDescriptions) / sizeof(contractDescriptions[0])][5];
static EXPAND_PROCEDURE contractExpandProcedures[sizeof(contractDescriptions) / sizeof(contractDescriptions[0])];
static USER_FUNCTION contractUserFunctions[sizeof(contractDescriptions) / sizeof(contractDescriptions[0])][65536];
static unsigned short contractUserFunctionInputSizes[sizeof(contractDescriptions) / sizeof(contractDescriptions[0])][65536];
static unsigned short contractUserFunctionOutputSizes[sizeof(contractDescriptions) / sizeof(contractDescriptions[0])][65536];
static USER_PROCEDURE contractUserProcedures[sizeof(contractDescriptions) / sizeof(contractDescriptions[0])][65536];
static unsigned short contractUserProcedureInputSizes[sizeof(contractDescriptions) / sizeof(contractDescriptions[0])][65536];
static unsigned short contractUserProcedureOutputSizes[sizeof(contractDescriptions) / sizeof(contractDescriptions[0])][65536];

#pragma warning(push)
#pragma warning(disable: 4005)
#define INITIALIZE 0
#define BEGIN_EPOCH 1
#define END_EPOCH 2
#define BEGIN_TICK 3
#define END_TICK 4
#pragma warning(pop)

#define REGISTER_CONTRACT_FUNCTIONS_AND_PROCEDURES(contractName)\
_##contractName = (contractName*)contractState;\
contractSystemProcedures[contractIndex][INITIALIZE] = (void (*)(void*))contractName::__initialize;\
contractSystemProcedures[contractIndex][BEGIN_EPOCH] = (void (*)(void*))contractName::__beginEpoch;\
contractSystemProcedures[contractIndex][END_EPOCH] = (void (*)(void*))contractName::__endEpoch;\
contractSystemProcedures[contractIndex][BEGIN_TICK] = (void (*)(void*))contractName::__beginTick;\
contractSystemProcedures[contractIndex][END_TICK] = (void (*)(void*))contractName::__endTick;\
contractExpandProcedures[contractIndex] = (void (*)(void*, void*))contractName::__expand;\
executedContractIndex = contractIndex;\
_##contractName->__registerUserFunctions();\
_##contractName->__registerUserProcedures();

static volatile unsigned int executedContractIndex;

static void initializeContract(const unsigned int contractIndex, void* contractState)
{
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
