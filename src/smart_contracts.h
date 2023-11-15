#pragma once

#include <intrin.h>

////////// Smart contracts \\\\\\\\\\

typedef void (*SYSTEM_PROCEDURE)(void*);
typedef void (*EXPAND_PROCEDURE)(void*, void*);
typedef void (*USER_PROCEDURE)(void*, void*, void*);

struct Entity
{
    unsigned char publicKey[32];
    long long incomingAmount, outgoingAmount;
    unsigned int numberOfIncomingTransfers, numberOfOutgoingTransfers;
    unsigned int latestIncomingTransferTick, latestOutgoingTransferTick;
};

static __m256i __arbitrator();
static void __beginFunctionOrProcedure(const unsigned int);
static __m256i __computor(unsigned short);
static unsigned char __day();
static unsigned char __dayOfWeek(unsigned char, unsigned char, unsigned char);
static void __endFunctionOrProcedure(const unsigned int);
static unsigned short __epoch();
static bool __getEntity(__m256i, ::Entity&);
static unsigned char __hour();
static unsigned short __millisecond();
static unsigned char __minute();
static unsigned char __month();
static __m256i __nextId(__m256i);
static __m256i __originator();
static void __registerUserFunction(USER_PROCEDURE, unsigned short, unsigned short);
static void __registerUserProcedure(USER_PROCEDURE, unsigned short, unsigned short);
static unsigned char __second();
static unsigned int __tick();
static long long __transfer(__m256i, long long);
static long long __transferAssetOwnershipAndPossession(unsigned long long, __m256i, __m256i, __m256i, long long, __m256i);
static unsigned char __year();

#include "qpi.h"

#define QX_CONTRACT_INDEX 1
#define CONTRACT_INDEX QX_CONTRACT_INDEX
#define CONTRACT_STATE_TYPE QX
#define CONTRACT_STATE2_TYPE QX2
#include "smart_contracts/Qx.h"
static CONTRACT_STATE_TYPE* _QX;

#define QUOTTERY_CONTRACT_INDEX 2
#define CONTRACT_INDEX QUOTTERY_CONTRACT_INDEX
#define CONTRACT_STATE_TYPE QUOTTERY
#define CONTRACT_STATE2_TYPE QUOTTERY2
#include "smart_contracts/Quottery.h"
static CONTRACT_STATE_TYPE* _QUOTTERY;

#define MAX_CONTRACT_ITERATION_DURATION 1000 // In milliseconds, must be above 0
#define MAX_NUMBER_OF_CONTRACTS 1024 // Must be 1024

struct Contract0State
{
    long long contractFeeReserves[MAX_NUMBER_OF_CONTRACTS];
};

struct IPO
{
    unsigned char publicKeys[NUMBER_OF_COMPUTORS][32];
    long long prices[NUMBER_OF_COMPUTORS];
};

constexpr struct ContractDescription
{
    char assetName[8];
    unsigned short constructionEpoch, destructionEpoch;
    unsigned long long stateSize;
} contractDescriptions[] = {
    {"", 0, 0, sizeof(Contract0State)},
    {"QX", 71, 10000, sizeof(QX)},
    {"QTRY", 72, 10000, sizeof(IPO)}
};

static SYSTEM_PROCEDURE contractSystemProcedures[sizeof(contractDescriptions) / sizeof(contractDescriptions[0])][5];
static EXPAND_PROCEDURE contractExpandProcedures[sizeof(contractDescriptions) / sizeof(contractDescriptions[0])];
static USER_PROCEDURE contractUserProcedures[sizeof(contractDescriptions) / sizeof(contractDescriptions[0])][65536];
static unsigned short contractUserProcedureInputSizes[sizeof(contractDescriptions) / sizeof(contractDescriptions[0])][65536];

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
    }
}