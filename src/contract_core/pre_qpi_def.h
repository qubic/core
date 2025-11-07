#pragma once

#include "network_messages/common_def.h"
#include "platform/m256.h"

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

// Size of the contract action tracker, limits the number of transfers that one contract call can execute.
constexpr unsigned long long CONTRACT_ACTION_TRACKER_SIZE = 16 * 1024 * 1024;


static void __beginFunctionOrProcedure(const unsigned int); // TODO: more human-readable form of function ID?
static void __endFunctionOrProcedure(const unsigned int);
template <typename T> static m256i __K12(T);
template <typename T> static void __logContractDebugMessage(unsigned int, T&);
template <typename T> static void __logContractErrorMessage(unsigned int, T&);
template <typename T> static void __logContractInfoMessage(unsigned int, T&);
template <typename T> static void __logContractWarningMessage(unsigned int, T&);
static void __pauseLogMessage();
static void __resumeLogMessage();

// Get buffer for temporary use. Can only be used in contract procedures / tick processor / contract processor!
// Always returns the same one buffer, no concurrent access!
static void* __scratchpad(unsigned long long sizeToMemsetZero = 0);

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
