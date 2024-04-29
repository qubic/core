#pragma once

#include "platform/m256.h"

#include "network_messages/common_def.h"

////////// Smart contracts \\\\\\\\\\

namespace QPI
{
    struct QpiContext;
}

typedef void (*SYSTEM_PROCEDURE)(const QPI::QpiContext&, void*);
typedef void (*EXPAND_PROCEDURE)(const QPI::QpiContext&, void*, void*);
typedef void (*USER_FUNCTION)(const QPI::QpiContext&, void*, void*, void*);
typedef void (*USER_PROCEDURE)(const QPI::QpiContext&, void*, void*, void*);


static void __beginFunctionOrProcedure(const unsigned int);
static void __endFunctionOrProcedure(const unsigned int);
template <typename T> static m256i __K12(T);
template <typename T> static void __logContractDebugMessage(unsigned int, const T&);
template <typename T> static void __logContractErrorMessage(unsigned int, const T&);
template <typename T> static void __logContractInfoMessage(unsigned int, const T&);
template <typename T> static void __logContractWarningMessage(unsigned int, const T&);
static void* __scratchpad();    // TODO: concurrency support (n buffers for n allowed concurrent contract executions)

#include "contracts/qpi.h"

#define QX_CONTRACT_INDEX 1
#define CONTRACT_INDEX QX_CONTRACT_INDEX
#define CONTRACT_STATE_TYPE QX
#define CONTRACT_STATE2_TYPE QX2
#include "contracts/Qx.h"
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
static USER_FUNCTION contractUserFunctions[contractCount][65536];
static unsigned short contractUserFunctionInputSizes[contractCount][65536];
static unsigned short contractUserFunctionOutputSizes[contractCount][65536];
static USER_PROCEDURE contractUserProcedures[contractCount][65536];
static unsigned short contractUserProcedureInputSizes[contractCount][65536];
static unsigned short contractUserProcedureOutputSizes[contractCount][65536];

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
