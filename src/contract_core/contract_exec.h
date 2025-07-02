#pragma once

#include "public_settings.h"

#include "contracts/qpi.h"

#include "platform/global_var.h"
#include "platform/read_write_lock.h"
#include "platform/debugging.h"
#include "platform/memory.h"

#include "contract_core/contract_def.h"
#include "contract_core/stack_buffer.h"
#include "contract_core/contract_action_tracker.h"

#include "logging/logging.h"
#include "common_buffers.h"

// TODO: remove, only for debug output
#include "system.h"

#include <lib/platform_common/long_jump.h>


enum ContractError
{
    NoContractError = 0,
    ContractErrorAllocInputOutputFailed,
    ContractErrorAllocLocalsFailed,
    ContractErrorAllocContextOtherFunctionCallFailed,
    ContractErrorAllocContextOtherProcedureCallFailed,
    ContractErrorTooManyActions,
    ContractErrorTimeout,
    ContractErrorStoppedToResolveDeadlock, // only returned by function call, not set to contractError
};

// Used to store: locals and for first invocation level also input and output
typedef StackBuffer<unsigned int, 32 * 1024 * 1024> ContractLocalsStack;
GLOBAL_VAR_DECL ContractLocalsStack contractLocalsStack[NUMBER_OF_CONTRACT_EXECUTION_BUFFERS];
GLOBAL_VAR_DECL volatile char contractLocalsStackLock[NUMBER_OF_CONTRACT_EXECUTION_BUFFERS];
GLOBAL_VAR_DECL volatile long contractLocalsStackLockWaitingCount;
GLOBAL_VAR_DECL long contractLocalsStackLockWaitingCountMax;

struct ContractExecErrorData
{
    LongJumpBuffer longJumpBuffer;
    unsigned int errorCode;
    unsigned int _paddingTo8;
};
GLOBAL_VAR_DECL ContractExecErrorData contractExecutionErrorData[contractCount];

GLOBAL_VAR_DECL ReadWriteLock contractStateLock[contractCount];
GLOBAL_VAR_DECL unsigned char* contractStates[contractCount];
GLOBAL_VAR_DECL volatile long long contractTotalExecutionTicks[contractCount];

// Contract error state, persistent and only set on error of procedure (TODO: only execute procedures if NoContractError)
GLOBAL_VAR_DECL unsigned int contractError[contractCount];

// TODO: If we ever have parallel procedure calls (of different contracts), we need to make
// access to contractStateChangeFlags thread-safe
GLOBAL_VAR_DECL unsigned long long* contractStateChangeFlags GLOBAL_VAR_INIT(nullptr);


// Contract system procedures that serve as callbacks, such as PRE_ACQUIRE_SHARES,
// break the rule that contracts can only call other contracts with lower index.
// This requires special handling in order to prevent deadlocks.
// Further, calling certain QPI functions in callbacks must be prevented in order
// to prevent cyclic calling patterns.
GLOBAL_VAR_DECL volatile unsigned int contractCallbacksRunning;
enum ContractCallbacksRunningFlags
{
    NoContractCallback = 0,
    ContractCallbackManagementRightsTransfer = 1,
    ContractCallbackPostIncomingTransfer = 2,
};


GLOBAL_VAR_DECL ContractActionTracker<CONTRACT_ACTION_TRACKER_SIZE> contractActionTracker;

// Instances of this struct are pushed on the contractLocalsStack during execution to support rollback of locks etc
// in case of an error
struct ContractRollbackInfo
{
    enum Type
    {
        ContractStateReuseLock = 0,
        ContractStateWriteLock = 1,
        ContractStateReadLock = 2,
    };
    unsigned int type : 2;
    unsigned int contractIndex : 30;
    static constexpr int i = (1 << 30) - 1;
    static_assert(contractCount < (1 << 30) - 1, "Implementation assumes fewer contracts and must be changed!");
};

static inline ContractRollbackInfo* contractStackUnwindRollbackInfo(int stackIndex)
{
    ASSERT(stackIndex >= 0 && stackIndex < NUMBER_OF_CONTRACT_EXECUTION_BUFFERS);
    char* ptr;
    unsigned int size;
    bool specialBlock;
    bool ok = contractLocalsStack[stackIndex].unwind(ptr, size, specialBlock);
    ASSERT(ok);
    ASSERT(ptr != nullptr);
    ASSERT(size == sizeof(ContractRollbackInfo));
    ASSERT(specialBlock);
    return reinterpret_cast<ContractRollbackInfo*>(ptr);
}

static bool rollbackContractFunctionCall(int stackIndex)
{
    ASSERT(stackIndex >= 0);
    ASSERT(stackIndex < NUMBER_OF_CONTRACT_EXECUTION_BUFFERS);
    ASSERT(contractLocalsStackLock[stackIndex]);
    if (stackIndex < 0 || stackIndex >= NUMBER_OF_CONTRACT_EXECUTION_BUFFERS || !contractLocalsStackLock[stackIndex])
        return false;

    char* ptr;
    unsigned int size;
    bool specialBlock;
    while (contractLocalsStack[stackIndex].unwind(ptr, size, specialBlock))
    {
        if (specialBlock && size == sizeof(ContractRollbackInfo))
        {
            auto cri = reinterpret_cast<ContractRollbackInfo*>(ptr);
            ASSERT(cri->type == ContractRollbackInfo::ContractStateReadLock);
            ASSERT(cri->contractIndex < contractCount);
            ASSERT(contractStateLock[cri->contractIndex].getCurrentReaderLockCount() > 0);
            if (cri->type == ContractRollbackInfo::ContractStateReadLock
                && cri->contractIndex < contractCount
                && contractStateLock[cri->contractIndex].getCurrentReaderLockCount() > 0)
            {
                contractStateLock[cri->contractIndex].releaseRead();
            }
        }
    }

    return true;
}

static bool initContractExec()
{
    for (unsigned int contractIndex = 0; contractIndex < contractCount; contractIndex++)
    {
        contractStates[contractIndex] = nullptr;
    }
    setMem(contractSystemProcedures, sizeof(contractSystemProcedures), 0);
    setMem(contractSystemProcedureLocalsSizes, sizeof(contractSystemProcedureLocalsSizes), 0);
    setMem(contractUserFunctions, sizeof(contractUserFunctions), 0);
    setMem(contractUserProcedures, sizeof(contractUserProcedures), 0);
    setMem(contractUserFunctionInputSizes, sizeof(contractUserFunctionInputSizes), 0);
    setMem(contractUserFunctionOutputSizes, sizeof(contractUserFunctionOutputSizes), 0);
    setMem(contractUserFunctionLocalsSizes, sizeof(contractUserFunctionLocalsSizes), 0);
    setMem(contractUserProcedureInputSizes, sizeof(contractUserProcedureInputSizes), 0);
    setMem(contractUserProcedureOutputSizes, sizeof(contractUserProcedureOutputSizes), 0);
    setMem(contractUserProcedureLocalsSizes, sizeof(contractUserProcedureLocalsSizes), 0);

    for (ContractLocalsStack::SizeType i = 0; i < NUMBER_OF_CONTRACT_EXECUTION_BUFFERS; ++i)
        contractLocalsStack[i].init();
    setMem((void*)contractLocalsStackLock, sizeof(contractLocalsStackLock), 0);
    contractLocalsStackLockWaitingCount = 0;
    contractLocalsStackLockWaitingCountMax = 0;

    setMem((void*)contractTotalExecutionTicks, sizeof(contractTotalExecutionTicks), 0);
    setMem((void*)contractError, sizeof(contractError), 0);
    setMem((void*)contractExecutionErrorData, sizeof(contractExecutionErrorData), 0);
    for (int i = 0; i < contractCount; ++i)
    {
        contractStateLock[i].reset();
    }

    if (!allocPoolWithErrorLog(L"contractStateChangeFlags", MAX_NUMBER_OF_CONTRACTS / 8, (void**)&contractStateChangeFlags, __LINE__))
    {
        return false;
    }
    setMem(contractStateChangeFlags, MAX_NUMBER_OF_CONTRACTS / 8, 0xFF);

    contractCallbacksRunning = NoContractCallback;

    if (!contractActionTracker.allocBuffer())
        return false;

    return true;
}

static void deinitContractExec()
{
    if (contractStateChangeFlags)
    {
        freePool(contractStateChangeFlags);
    }

    contractActionTracker.freeBuffer();
}

// Acquire lock of an currently unused stack (may block if all in use)
// stacksToIgnore > 0 can be passed by low priority tasks to keep some stacks reserved for high prio purposes.
static void acquireContractLocalsStack(int& stackIdx, unsigned int stacksToIgnore = 0)
{
    static_assert(NUMBER_OF_CONTRACT_EXECUTION_BUFFERS >= 2, "NUMBER_OF_CONTRACT_EXECUTION_BUFFERS should be at least 2.");
    ASSERT(stackIdx < 0);
    ASSERT(stacksToIgnore < NUMBER_OF_CONTRACT_EXECUTION_BUFFERS);

    long waitingCount = _InterlockedIncrement(&contractLocalsStackLockWaitingCount);
    if (contractLocalsStackLockWaitingCountMax < waitingCount)
        contractLocalsStackLockWaitingCountMax = waitingCount;

    int i = stacksToIgnore;
    BEGIN_WAIT_WHILE(TRY_ACQUIRE(contractLocalsStackLock[i]) == false)
    {
        ++i;
        if (i == NUMBER_OF_CONTRACT_EXECUTION_BUFFERS)
            i = stacksToIgnore;
    }
    END_WAIT_WHILE();

    _InterlockedDecrement(&contractLocalsStackLockWaitingCount);

    stackIdx = i;
    ASSERT(stackIdx >= 0);

    ASSERT(contractLocalsStack[stackIdx].size() == 0);
    if (contractLocalsStack[stackIdx].size())
        contractLocalsStack[stackIdx].freeAll();
}

// Release locked stack (and reset stackIdx)
static void releaseContractLocalsStack(int& stackIdx)
{
    ASSERT(stackIdx >= 0);
    ASSERT(stackIdx < NUMBER_OF_CONTRACT_EXECUTION_BUFFERS);
    ASSERT(contractLocalsStackLock[stackIdx]);
    RELEASE(contractLocalsStackLock[stackIdx]);
    stackIdx = -1;
}

// Allocate storage on ContractLocalsStack of QPI execution context
void* QPI::QpiContextFunctionCall::__qpiAllocLocals(unsigned int sizeOfLocals) const
{
    ASSERT(_stackIndex >= 0 && _stackIndex < NUMBER_OF_CONTRACT_EXECUTION_BUFFERS);
    if (_stackIndex < 0 || _stackIndex >= NUMBER_OF_CONTRACT_EXECUTION_BUFFERS)
    {
#ifndef NDEBUG
        CHAR16 dbgMsgBuf[100];
        setText(dbgMsgBuf, L"__qpiAllocLocals in ");
        appendNumber(dbgMsgBuf, system.tick, FALSE);
        appendText(dbgMsgBuf, L" called with stackIndex ");
        appendNumber(dbgMsgBuf, _stackIndex, FALSE);
        addDebugMessage(dbgMsgBuf);
#endif
        // abort execution of contract here
        __qpiAbort(ContractErrorAllocLocalsFailed);
    }
    void* p = contractLocalsStack[_stackIndex].allocate(sizeOfLocals);
    if (!p)
    {
#ifndef NDEBUG
        CHAR16 dbgMsgBuf[400];
        setText(dbgMsgBuf, L"__qpiAllocLocals stack buffer alloc failed in tick ");
        appendNumber(dbgMsgBuf, system.tick, FALSE);
        addDebugMessage(dbgMsgBuf);
        setText(dbgMsgBuf, L"allocSize ");
        appendNumber(dbgMsgBuf, sizeOfLocals, FALSE);
        appendText(dbgMsgBuf, L", contractIndex ");
        appendNumber(dbgMsgBuf, _currentContractIndex, FALSE);
        appendText(dbgMsgBuf, L", stackIndex ");
        appendNumber(dbgMsgBuf, _stackIndex, FALSE);
        addDebugMessage(dbgMsgBuf);
#endif
        // abort execution of contract here
        __qpiAbort(ContractErrorAllocLocalsFailed);
    }
    setMem(p, sizeOfLocals, 0);
    return p;
}

// Free last allocated storage on ContractLocalsStack of QPI execution context
void QPI::QpiContextFunctionCall::__qpiFreeLocals() const
{
    ASSERT(_stackIndex >= 0 && _stackIndex < NUMBER_OF_CONTRACT_EXECUTION_BUFFERS);
    if (_stackIndex < 0 || _stackIndex >= NUMBER_OF_CONTRACT_EXECUTION_BUFFERS)
        return;
    contractLocalsStack[_stackIndex].free();
}

// Called before one contract calls a function of a different contract
const QpiContextFunctionCall& QPI::QpiContextFunctionCall::__qpiConstructContextOtherContractFunctionCall(unsigned int otherContractIndex) const
{
    ASSERT(otherContractIndex < _currentContractIndex);
    ASSERT(_stackIndex >= 0 && _stackIndex < NUMBER_OF_CONTRACT_EXECUTION_BUFFERS);
    char * buffer = contractLocalsStack[_stackIndex].allocate(sizeof(QpiContextFunctionCall));
    if (!buffer)
    {
#ifndef NDEBUG
        CHAR16 dbgMsgBuf[400];
        setText(dbgMsgBuf, L"__qpiConstructContextOtherContractFunctionCall stack buffer alloc failed in tick ");
        appendNumber(dbgMsgBuf, system.tick, FALSE);
        addDebugMessage(dbgMsgBuf);
        setText(dbgMsgBuf, L"allocSize ");
        appendNumber(dbgMsgBuf, sizeof(QpiContextFunctionCall), FALSE);
        appendText(dbgMsgBuf, L", contractIndex ");
        appendNumber(dbgMsgBuf, _currentContractIndex, FALSE);
        appendText(dbgMsgBuf, L", stackIndex ");
        appendNumber(dbgMsgBuf, _stackIndex, FALSE);
        addDebugMessage(dbgMsgBuf);
#endif
        // abort execution of contract here
        __qpiAbort(ContractErrorAllocContextOtherFunctionCallFailed);
    }
    QpiContextFunctionCall& newContext = *reinterpret_cast<QpiContextFunctionCall*>(buffer);
    newContext.init(otherContractIndex, _originator, _currentContractId, _invocationReward, _entryPoint, _stackIndex);
    return newContext;
}

// Called before a contract runs a user procedure of another contract or a system procedure
const QpiContextProcedureCall& QPI::QpiContextProcedureCall::__qpiConstructProcedureCallContext(unsigned int procContractIndex, QPI::sint64 invocationReward) const
{
    ASSERT(_entryPoint != USER_FUNCTION_CALL);
    ASSERT(_stackIndex >= 0 && _stackIndex < NUMBER_OF_CONTRACT_EXECUTION_BUFFERS);

    // A contract can only run a procedure of a contract with a lower index, exceptions are callback system procedures
    ASSERT(procContractIndex < _currentContractIndex || contractCallbacksRunning != NoContractCallback);

    char* buffer = contractLocalsStack[_stackIndex].allocate(sizeof(QpiContextProcedureCall));
    if (!buffer)
    {
#ifndef NDEBUG
        CHAR16 dbgMsgBuf[400];
        setText(dbgMsgBuf, L"__qpiConstructProcedureCallContext stack buffer alloc failed in tick ");
        appendNumber(dbgMsgBuf, system.tick, FALSE);
        addDebugMessage(dbgMsgBuf);
        setText(dbgMsgBuf, L"allocSize ");
        appendNumber(dbgMsgBuf, sizeof(QpiContextProcedureCall), FALSE);
        appendText(dbgMsgBuf, L", contractIndex ");
        appendNumber(dbgMsgBuf, _currentContractIndex, FALSE);
        appendText(dbgMsgBuf, L", stackIndex ");
        appendNumber(dbgMsgBuf, _stackIndex, FALSE);
        addDebugMessage(dbgMsgBuf);
#endif
        // abort execution of contract here
        __qpiAbort(ContractErrorAllocContextOtherProcedureCallFailed);
    }

    // If transfer isn't possible, set invocation reward to 0
    if (transfer(QPI::id(procContractIndex, 0, 0, 0), invocationReward) < 0)
        invocationReward = 0;

    QpiContextProcedureCall& newContext = *reinterpret_cast<QpiContextProcedureCall*>(buffer);
    newContext.init(procContractIndex, _originator, _currentContractId, invocationReward, _entryPoint, _stackIndex);

    return newContext;
}

// Called after a contract has run a function or procedure of a different contract or a system procedure
void QPI::QpiContextFunctionCall::__qpiFreeContext() const
{
    ASSERT(_stackIndex >= 0 && _stackIndex < NUMBER_OF_CONTRACT_EXECUTION_BUFFERS);
    contractLocalsStack[_stackIndex].free();
}

static void addDebugMessageAboutContractStateLockChange(const CHAR16* functionName, unsigned int contractIdxCalling, unsigned int contractIdxToLock, unsigned char entryPoint)
{
#if !defined(NDEBUG)
    CHAR16 dbgMsgBuf[200 + contractCount * 5];
    setText(dbgMsgBuf, functionName);
    appendText(dbgMsgBuf, L"(");
    appendNumber(dbgMsgBuf, contractIdxToLock, FALSE);
    appendText(dbgMsgBuf, L"), called by contract ");
    appendNumber(dbgMsgBuf, contractIdxCalling, FALSE);
    appendText(dbgMsgBuf, (entryPoint == USER_FUNCTION_CALL) ? L" function" : L" procedure");
    appendText(dbgMsgBuf, L"; initial state: ");
    for (unsigned int i = 0; i < contractCount; ++i)
    {
        int readerLockCount = contractStateLock[i].getCurrentReaderLockCount();
        if (readerLockCount != 0)
        {
            appendNumber(dbgMsgBuf, i, FALSE);
            appendText(dbgMsgBuf, (readerLockCount > 0) ? L"r " : L"w ");
        }
    }
    addDebugMessage(dbgMsgBuf);
#endif
}

// Used to acquire a contract state lock when one contract calls a function of a different contract
void* QPI::QpiContextFunctionCall::__qpiAcquireStateForReading(unsigned int contractIndex) const
{
#if !defined(NDEBUG) && defined(PRINT_CONTRACT_LOCK_DETAILS)
    addDebugMessageAboutContractStateLockChange(L"__qpiAcquireStateForReading", _currentContractIndex, contractIndex, _entryPoint);
#endif

    ASSERT(_stackIndex >= 0 && _stackIndex < NUMBER_OF_CONTRACT_EXECUTION_BUFFERS);
    ASSERT(contractIndex < contractCount);
    ASSERT(contractIndex <= _currentContractIndex);

    // Add rollback info for this lock to the stack
    auto rollbackInfo = reinterpret_cast<ContractRollbackInfo*>(contractLocalsStack[_stackIndex].allocateSpecial(sizeof(ContractRollbackInfo)));
    rollbackInfo->contractIndex = contractIndex;

    // Default case: no callback is running
    // -> Contracts can only call contracts with lower index.
    // -> No deadlock possible, because multiple readers are supported by the lock
    //    and only contract processor acquires write lock
    //
    // A callback may also call a contract with a higher index, making deadlocks possible.
    // -> Deadlocks with function calls and a procedure running a callback are resolved by stopping the function
    // -> Procedures may already have acquired write lock of a state before

    // Lock depending on cases
    if (_entryPoint == USER_FUNCTION_CALL)
    {
        // Entry point is user function (running in request processor)
        // -> Default case: either get lock immediately or retry as long as no callback is running
        BEGIN_WAIT_WHILE(!contractStateLock[contractIndex].tryAcquireRead())
        {
            if (contractCallbacksRunning != NoContractCallback)
            {
                // Special case: callback is running
                // -> Waiting for this lock may cause a deadlock
                // -> Contract function is low priority and is stopped to prevent potential deadlocks
                contractLocalsStack[_stackIndex].free();
                __qpiAbort(ContractErrorStoppedToResolveDeadlock);
            }
        }
        END_WAIT_WHILE();
        rollbackInfo->type = ContractRollbackInfo::ContractStateReadLock;
    }
    else
    {
        // Entry point is procedure (running in contract processor)
        //
        // If running in a callback, the current contract can call a contract of any index and the state may be already
        // locked for writing!
        //    For example:
        //      1. Contract C2 invokes a C1 procedure (C2 and C1 states locked for writing)
        //      2. Contract C1 runs qpi.releaseShares(), which needs to call the PRE_ACQUIRE_SHARES
        //         callback of C1.
        //      3. C1 then calls a function of C2. For this, it tries to acquire a read lock of
        //         C2's state, ending up here.
        // -> Because we know that this runs within a procedure entry point, we only have one
        //    processor running procedures, and a function can never call a procedure, we know that:
        //      1. all write locks are owned by contract processor (running this) and
        //      2. all read locks are owned by request processors (other processors).
        if (contractCallbacksRunning != NoContractCallback
            && contractStateLock[contractIndex].isLockedForWriting())
        {
            // already locked by this processor
            // -> signal situation for corresponding __qpiReleaseStateForReading()
            // -> give read access to state without further ado
            rollbackInfo->type = ContractRollbackInfo::ContractStateReuseLock;
        }
        else
        {
            // Cases:
            // - no callback running or
            // - state not locked or
            // - already locked by a request processor for reading (contract function)
            //
            // -> acquire lock as usual
            // -> if there is a deadlock with a request processor, the following acquireRead()
            //    needs to wait until it gets resolved by stopping the request processor
            contractStateLock[contractIndex].acquireRead();
            rollbackInfo->type = ContractRollbackInfo::ContractStateReadLock;
        }
    }

    return contractStates[contractIndex];
}

// Used to release a contract state lock when one contract calls a function of a different contract
void QPI::QpiContextFunctionCall::__qpiReleaseStateForReading(unsigned int contractIndex) const
{
#if !defined(NDEBUG) && defined(PRINT_CONTRACT_LOCK_DETAILS)
    addDebugMessageAboutContractStateLockChange(L"__qpiReleaseStateForReading", _currentContractIndex, contractIndex, _entryPoint);
#endif

    ASSERT(_stackIndex >= 0 && _stackIndex < NUMBER_OF_CONTRACT_EXECUTION_BUFFERS);
    ASSERT(contractIndex < contractCount);
    ASSERT(contractIndex <= _currentContractIndex);
    if (contractCallbacksRunning == NoContractCallback)
    {
        // Default case: no callback is running
        // - release read lock
        contractStateLock[contractIndex].releaseRead();
#if !defined(NO_UEFI) && defined(NDEBUG)
        // - discard rollback info from stack
        contractLocalsStack[_stackIndex].free();
#else
        // - free and check rollback info from stack
        ContractRollbackInfo* cri = contractStackUnwindRollbackInfo(_stackIndex);
        ASSERT(cri->type == ContractRollbackInfo::ContractStateReadLock);
        ASSERT(cri->contractIndex == contractIndex);
#endif
    }
    else
    {
        // Special case: callback is running (locks may be reused)
        ContractRollbackInfo* cri = contractStackUnwindRollbackInfo(_stackIndex);
        ASSERT(cri->type == ContractRollbackInfo::ContractStateReadLock || cri->type == ContractRollbackInfo::ContractStateReuseLock);
        ASSERT(cri->contractIndex == contractIndex);
        if (cri->type == ContractRollbackInfo::ContractStateReadLock)
        {
            contractStateLock[contractIndex].releaseRead();
        }
    }
}

// Used to acquire a contract state lock when one contract calls a procedure of a different contract
void* QPI::QpiContextProcedureCall::__qpiAcquireStateForWriting(unsigned int contractIndex) const
{
#if !defined(NDEBUG) && defined(PRINT_CONTRACT_LOCK_DETAILS)
    addDebugMessageAboutContractStateLockChange(L"__qpiAcquireStateForWriting", _currentContractIndex, contractIndex, _entryPoint);
#endif

    // Entry point is procedure (running in contract processor), because functions cannot acquire write lock.
    ASSERT(_entryPoint != USER_FUNCTION_CALL);
    ASSERT(contractIndex < contractCount);
    ASSERT(_stackIndex >= 0 && _stackIndex < NUMBER_OF_CONTRACT_EXECUTION_BUFFERS);

    // Add rollback info for this lock to the stack
    auto rollbackInfo = reinterpret_cast<ContractRollbackInfo*>(contractLocalsStack[_stackIndex].allocateSpecial(sizeof(ContractRollbackInfo)));
    rollbackInfo->contractIndex = contractIndex;

    if (contractCallbacksRunning == NoContractCallback)
    {
        // Default case: no callback is running
        // -> Contracts can only call contracts with lower index (such as C2 calls C1).
        // -> No deadlock possible, because multiple readers are supported by the lock
        //    and only contract processor acquires write lock
        ASSERT(contractIndex < _currentContractIndex);
        // this needs to be split in a loop considering all cases below, because a deadlock may also happen when the functions is already waiting here
        contractStateLock[contractIndex].acquireWrite();
        rollbackInfo->type = ContractRollbackInfo::ContractStateWriteLock;
    }
    else
    {
        // Special case: callback is running
        // -> Special handling needed to prevent deadlocks, because:
        //    Contract can call contract of any index and state may be already locked for writing!
        //    For example:
        //      1. Contract C2 invokes a procedure of C1 -> C2 and C1 states are locked for writing
        //      2. Contract C1 runs qpi.releaseShares(), which needs to call the PRE_ACQUIRE_SHARES
        //         callback of C2 -> __qpiCallSystemProc() tries to acquire write
        //         lock of C2 through __qpiAcquireStateForWriting()
        // -> Because we know that this runs within a procedure entry point, we only have one
        //    processor running procedures, and a function can never call a procedure, we know that:
        //      1. all write locks are owned by contract processor (running this) and
        //      2. all read locks are owned by request processors (other processors).
        if (contractStateLock[contractIndex].isLockedForWriting())
        {
            // already locked by this processor
            // -> signal situation for corresponding __qpiReleaseStateForWriting()
            // -> give write access to state without further ado
            rollbackInfo->type = ContractRollbackInfo::ContractStateReuseLock;
        }
        else
        {
            // not locked or already locked by a request processor for reading (contract function)
            // -> acquire lock as usual (which may need to wait if locked for reading)
            // -> if there is a deadlock with a request processor, the following acquireWrite()
            //    needs to wait until it gets resolved in __qpiAcquireStateForReading()
            contractStateLock[contractIndex].acquireWrite();
            rollbackInfo->type = ContractRollbackInfo::ContractStateWriteLock;
        }
    }

    return contractStates[contractIndex];
}

// Used to release a contract state lock when one contract calls a procedure of a different contract
void QPI::QpiContextProcedureCall::__qpiReleaseStateForWriting(unsigned int contractIndex) const
{
#if !defined(NDEBUG) && defined(PRINT_CONTRACT_LOCK_DETAILS)
    addDebugMessageAboutContractStateLockChange(L"__qpiReleaseStateForWriting", _currentContractIndex, contractIndex, _entryPoint);
#endif

    ASSERT(_stackIndex >= 0 && _stackIndex < NUMBER_OF_CONTRACT_EXECUTION_BUFFERS);
    ASSERT(_entryPoint != USER_FUNCTION_CALL);
    ASSERT(contractIndex < contractCount);
    if (contractCallbacksRunning == NoContractCallback)
    {
        // Default case: no callback is running
        ASSERT(contractIndex < _currentContractIndex);
        // - release write lock
        contractStateLock[contractIndex].releaseWrite();
#if !defined(NO_UEFI) && defined(NDEBUG)
        // - discard rollback info from stack
        contractLocalsStack[_stackIndex].free();
#else
        // - free and check rollback info from stack
        ContractRollbackInfo* cri = contractStackUnwindRollbackInfo(_stackIndex);
        ASSERT(cri->type == ContractRollbackInfo::ContractStateWriteLock);
        ASSERT(cri->contractIndex == contractIndex);
#endif
    }
    else
    {
        // Special case: callback is running (locks may be reused)
        ContractRollbackInfo* cri = contractStackUnwindRollbackInfo(_stackIndex);
        ASSERT(cri->type == ContractRollbackInfo::ContractStateWriteLock || cri->type == ContractRollbackInfo::ContractStateReuseLock);
        ASSERT(cri->contractIndex == contractIndex);
        if (cri->type == ContractRollbackInfo::ContractStateWriteLock)
        {
            contractStateLock[contractIndex].releaseWrite();
        }
    }
    contractStateChangeFlags[_currentContractIndex >> 6] |= (1ULL << (_currentContractIndex & 63));
}

// Used to run a special system procedure from within a contract for example in asset management rights transfer
template <unsigned int sysProcId, typename InputType, typename OutputType>
void QPI::QpiContextProcedureCall::__qpiCallSystemProc(unsigned int sysProcContractIndex, InputType& input, OutputType& output, QPI::sint64 invocationReward) const
{
    // Make sure this function is used with an expected combination of sysProcId, input,
    // and output
    static_assert(
        (sysProcId == PRE_RELEASE_SHARES && sizeof(InputType) == sizeof(QPI::PreManagementRightsTransfer_input) && sizeof(OutputType) == sizeof(QPI::PreManagementRightsTransfer_output))
        || (sysProcId == PRE_ACQUIRE_SHARES && sizeof(InputType) == sizeof(QPI::PreManagementRightsTransfer_input) && sizeof(OutputType) == sizeof(QPI::PreManagementRightsTransfer_output))
        || (sysProcId == POST_RELEASE_SHARES && sizeof(InputType) == sizeof(QPI::PostManagementRightsTransfer_input) && sizeof(OutputType) == sizeof(QPI::NoData))
        || (sysProcId == POST_ACQUIRE_SHARES && sizeof(InputType) == sizeof(QPI::PostManagementRightsTransfer_input) && sizeof(OutputType) == sizeof(QPI::NoData))
        || (sysProcId == POST_INCOMING_TRANSFER && sizeof(InputType) == sizeof(QPI::PostIncomingTransfer_input) && sizeof(OutputType) == sizeof(QPI::NoData))
        , "Unsupported __qpiCallSystemProc() call"
    );

    // Check that this internal function is used correctly
    ASSERT(_entryPoint != USER_FUNCTION_CALL);
    ASSERT(sysProcContractIndex < contractCount);
    ASSERT(contractStates[sysProcContractIndex] != nullptr);
    if (sysProcId == PRE_RELEASE_SHARES || sysProcId == PRE_ACQUIRE_SHARES
        || sysProcId == POST_RELEASE_SHARES || sysProcId == POST_ACQUIRE_SHARES)
    {
        ASSERT(sysProcContractIndex != _currentContractIndex);
    }

    // Initialize output with 0
    setMem(&output, sizeof(output), 0);

    // Empty procedures lead to null pointer in contractSystemProcedures -> return default output (all zero/false)
    if (!contractSystemProcedures[sysProcContractIndex][sysProcId])
        return;

    // Set flags of callbacks currently running (to prevent deadlocks and nested calling of QPI functions)
    auto contractCallbacksRunningBefore = contractCallbacksRunning;
    if (sysProcId == POST_INCOMING_TRANSFER)
    {
        contractCallbacksRunning |= ContractCallbackPostIncomingTransfer;
    }
    else if (sysProcId == PRE_RELEASE_SHARES || sysProcId == PRE_ACQUIRE_SHARES
        || sysProcId == POST_RELEASE_SHARES || sysProcId == POST_ACQUIRE_SHARES)
    {
        contractCallbacksRunning |= ContractCallbackManagementRightsTransfer;
    }

    // Create context
    const QpiContextProcedureCall& context = __qpiConstructProcedureCallContext(sysProcContractIndex, invocationReward);

    // Get state (lock state for writing if other contract)
    const bool otherContract = sysProcContractIndex != _currentContractIndex;
    void* state = (otherContract) ? __qpiAcquireStateForWriting(sysProcContractIndex) : contractStates[sysProcContractIndex];

    // Alloc locals
    unsigned short localsSize = contractSystemProcedureLocalsSizes[sysProcContractIndex][sysProcId];
    char* localsBuffer = contractLocalsStack[_stackIndex].allocate(localsSize);
    if (!localsBuffer)
        __qpiAbort(ContractErrorAllocLocalsFailed);
    setMem(localsBuffer, localsSize, 0);

    // Run procedure
    contractSystemProcedures[sysProcContractIndex][sysProcId](context, state, &input, &output, localsBuffer);

    // Cleanup: free locals, release state, and free context
    contractLocalsStack[_stackIndex].free();
    if (otherContract)
        __qpiReleaseStateForWriting(sysProcContractIndex);
    __qpiFreeContext();

    // Restore flags of callbacks currently running
    contractCallbacksRunning = contractCallbacksRunningBefore;
}

// If dest is a contract, notify contract by running system procedure POST_INCOMING_TRANSFER
void QPI::QpiContextProcedureCall::__qpiNotifyPostIncomingTransfer(const QPI::id& source, const QPI::id& dest, QPI::sint64 amount, QPI::uint8 type) const
{
    if (dest.u64._3 != 0 || dest.u64._2 != 0 || dest.u64._3 != 0 || dest.u64._0 >= contractCount || amount <= 0)
        return;

    unsigned int destContractIndex = static_cast<unsigned int>(dest.u64._0);
    if (!contractSystemProcedures[destContractIndex][POST_INCOMING_TRANSFER])
        return;

    QPI::PostIncomingTransfer_input input{ source, amount, type };
    QPI::NoData output; // output is zeroed in __qpiCallSystemProc
    __qpiCallSystemProc<POST_INCOMING_TRANSFER>(destContractIndex, input, output, 0);
}

// Enter endless loop leading to timeout
// -> TODO: unlock everything in case of function entry point, maybe retry later in case of deadlock handling
// -> TODO: rollback of contract actions on contractProcessor()
// -> For critical errors, keep error state (contract is dead)
void QPI::QpiContextFunctionCall::__qpiAbort(unsigned int errorCode) const
{
    ASSERT(_currentContractIndex < contractCount);

#if !defined(NDEBUG)
    CHAR16 dbgMsgBuf[200];
    setText(dbgMsgBuf, L"__qpiAbort() called in tick ");
    appendNumber(dbgMsgBuf, system.tick, FALSE);
    appendText(dbgMsgBuf, L", contractIndex ");
    appendNumber(dbgMsgBuf, _currentContractIndex, FALSE);
    appendText(dbgMsgBuf, L", errorCode ");
    appendNumber(dbgMsgBuf, errorCode, FALSE);
    appendText(dbgMsgBuf, (_entryPoint == USER_FUNCTION_CALL) ? L", by user function" : L", by procedure");
    addDebugMessage(dbgMsgBuf);
#endif

    if (_entryPoint == USER_FUNCTION_CALL)
    {
        contractExecutionErrorData[_stackIndex].errorCode = errorCode;
        LongJump(&contractExecutionErrorData[_stackIndex].longJumpBuffer, 1);
    }
    else
    {
        // TODO: long jump can be also used for procedures
        contractError[_currentContractIndex] = errorCode;
    }

    // TODO: How to do error handing in user functions? (request processor has no timeout / respawn)
    //       Caller need to be distinguished here (function may be called from procedure)

    // we have to wait for the timeout, because there seems to be no function to stop the processor
    // TODO: we may add a function to CustomStack for directly returning from the runFunction()
    // Idea: use exception to also release locks with destructors of guard objects?
    while (1)
        _mm_pause();
}

// TODO: don't call faulty contracts

//void QpiContextProcedureCall::__qpiRollbackContractTransaction()
// Undo whole transaction of contract, unlock/free all resources
// rollback should mark contract as faulty, state may be inconsistent, so exclude it from contractDigest?


// Prologue of contract functions / procedures
static void __beginFunctionOrProcedure(const unsigned int functionOrProcedureId)
{
    // TODO
    // called by all non-empty system procedures, user procedures, and user functions
    // purpose:
    // - make sure the limit of nested calls is not violated
    // - measure execution time
    // - construction of execution graph
    // - debugging
}

// Epilogue of contract functions / procedures
static void __endFunctionOrProcedure(const unsigned int functionOrProcedureId)
{
    // TODO
}

QPI::QpiContextForInit::QpiContextForInit(unsigned int contractIndex) : QpiContext(contractIndex, NULL_ID, NULL_ID, 0, REGISTER_USER_FUNCTIONS_AND_PROCEDURES_CALL)
{
}

void QPI::QpiContextForInit::__registerUserFunction(USER_FUNCTION userFunction, unsigned short inputType, unsigned short inputSize, unsigned short outputSize, unsigned int localsSize) const
{
    contractUserFunctions[_currentContractIndex][inputType] = userFunction;
    contractUserFunctionInputSizes[_currentContractIndex][inputType] = inputSize;
    contractUserFunctionOutputSizes[_currentContractIndex][inputType] = outputSize;
    contractUserFunctionLocalsSizes[_currentContractIndex][inputType] = localsSize;
}

void QPI::QpiContextForInit::__registerUserProcedure(USER_PROCEDURE userProcedure, unsigned short inputType, unsigned short inputSize, unsigned short outputSize, unsigned int localsSize) const
{
    contractUserProcedures[_currentContractIndex][inputType] = userProcedure;
    contractUserProcedureInputSizes[_currentContractIndex][inputType] = inputSize;
    contractUserProcedureOutputSizes[_currentContractIndex][inputType] = outputSize;
    contractUserProcedureLocalsSizes[_currentContractIndex][inputType] = localsSize;
}



// QPI context used to call contract system procedure from qubic core (contract processor)
// Currently, all system procedures that are run from outside a QpiContext are without invocation reward.
struct QpiContextSystemProcedureCall : public QPI::QpiContextProcedureCall
{
    QpiContextSystemProcedureCall(unsigned int contractIndex, SystemProcedureID systemProcId) : QPI::QpiContextProcedureCall(contractIndex, NULL_ID, 0, systemProcId)
    {
        contractActionTracker.init();
    }

    // Run system procedure without input and output
    void call()
    {
        ASSERT(
            _entryPoint == INITIALIZE
            || _entryPoint == BEGIN_EPOCH
            || _entryPoint == END_EPOCH
            || _entryPoint == BEGIN_TICK
            || _entryPoint == END_TICK
        );
        QPI::NoData noInOutData;
        runCall(&noInOutData, &noInOutData);
    }

    // Run callback system procedure POST_INCOMING_TRANSFER
    void call(QPI::PostIncomingTransfer_input& input)
    {
        ASSERT(_entryPoint == POST_INCOMING_TRANSFER);
        contractCallbacksRunning = ContractCallbackPostIncomingTransfer;
        QPI::NoData noOutData;
        runCall(&input, &noOutData);
        contractCallbacksRunning = NoContractCallback;
    }

private:
    void runCall(void* input, void* output)
    {
        const int systemProcId = _entryPoint;

        ASSERT(_currentContractIndex < contractCount);
        ASSERT(systemProcId < contractSystemProcedureCount);

        // Empty procedures lead to null pointer in contractSystemProcedures -> nothing to call
        if (!contractSystemProcedures[_currentContractIndex][systemProcId])
            return;

        // reserve stack for this processor (may block), needed even if there are no locals, because procedure may call
        // functions / procedures / notifications that create locals etc.
        acquireContractLocalsStack(_stackIndex);

        // acquire state for writing (may block)
        contractStateLock[_currentContractIndex].acquireWrite();

        const unsigned long long startTick = __rdtsc();
        unsigned short localsSize = contractSystemProcedureLocalsSizes[_currentContractIndex][systemProcId];
        if (localsSize == sizeof(QPI::NoData))
        {
            // no locals -> call
            QPI::NoData locals;
            contractSystemProcedures[_currentContractIndex][systemProcId](*this, contractStates[_currentContractIndex], input, output, &locals);
        }
        else
        {
            // locals required: use stack (should not block because stack 0 is reserved for procedures)
            char* localsBuffer = contractLocalsStack[_stackIndex].allocate(localsSize);
            if (!localsBuffer)
                __qpiAbort(ContractErrorAllocLocalsFailed);
            setMem(localsBuffer, localsSize, 0);

            // call system proc
            contractSystemProcedures[_currentContractIndex][systemProcId](*this, contractStates[_currentContractIndex], input, output, localsBuffer);

            // free data on stack
            contractLocalsStack[_stackIndex].free();
            ASSERT(contractLocalsStack[_stackIndex].size() == 0);
        }
        _interlockedadd64(&contractTotalExecutionTicks[_currentContractIndex], __rdtsc() - startTick);

        // release lock of contract state and set state to changed
        contractStateLock[_currentContractIndex].releaseWrite();
        contractStateChangeFlags[_currentContractIndex >> 6] |= (1ULL << (_currentContractIndex & 63));

        // release stack
        releaseContractLocalsStack(_stackIndex);
    }
};

// QPI context used to call contract user procedure from qubic core (contract processor), after transfer of invocation reward
struct QpiContextUserProcedureCall : public QPI::QpiContextProcedureCall
{
    char* outputBuffer;
    unsigned short outputSize;

    QpiContextUserProcedureCall(unsigned int contractIndex, const m256i& originator, long long invocationReward) : QPI::QpiContextProcedureCall(contractIndex, originator, invocationReward, USER_PROCEDURE_CALL)
    {
        contractActionTracker.init();
        if (!contractActionTracker.addQuTransfer(_originator, _currentContractId, _invocationReward))
            __qpiAbort(ContractErrorTooManyActions);
        outputBuffer = nullptr;
        outputSize = 0;
    }

    ~QpiContextUserProcedureCall()
    {
        freeBuffer();
    }

    void call(unsigned short inputType, const void* inputPtr, unsigned short inputSize)
    {
#if !defined(NDEBUG) && !defined(NO_UEFI)
        CHAR16 dbgMsgBuf[400];
        setText(dbgMsgBuf, L"QpiContextUserProcedureCall in tick ");
        appendNumber(dbgMsgBuf, system.tick, FALSE);
        appendText(dbgMsgBuf, L": contract ");
        appendNumber(dbgMsgBuf, _currentContractIndex, FALSE);
        appendText(dbgMsgBuf, L", inputType ");
        appendNumber(dbgMsgBuf, inputType, FALSE);
        appendText(dbgMsgBuf, L", inputSize ");
        appendNumber(dbgMsgBuf, inputSize, FALSE);
        addDebugMessage(dbgMsgBuf);
#endif
        ASSERT(_currentContractIndex < contractCount);
        ASSERT(contractUserProcedures[_currentContractIndex][inputType]);

        // reserve stack for this processor (may block)
        acquireContractLocalsStack(_stackIndex);

        // allocate input, output, and locals buffer from stack and init them
        unsigned short fullInputSize = contractUserProcedureInputSizes[_currentContractIndex][inputType];
        outputSize = contractUserProcedureOutputSizes[_currentContractIndex][inputType];
        unsigned int localsSize = contractUserProcedureLocalsSizes[_currentContractIndex][inputType];
        char* inputBuffer = contractLocalsStack[_stackIndex].allocate(fullInputSize + outputSize + localsSize);
        if (!inputBuffer)
        {
#ifndef NDEBUG
            CHAR16 dbgMsgBuf[400];
            setText(dbgMsgBuf, L"QpiContextUserProcedureCall stack buffer alloc failed in tick ");
            appendNumber(dbgMsgBuf, system.tick, FALSE);
            addDebugMessage(dbgMsgBuf);
            setText(dbgMsgBuf, L"fullInputSize ");
            appendNumber(dbgMsgBuf, fullInputSize, FALSE);
            appendText(dbgMsgBuf, L", outputSize ");
            appendNumber(dbgMsgBuf, outputSize, FALSE);
            appendText(dbgMsgBuf, L", localsSize ");
            appendNumber(dbgMsgBuf, localsSize, FALSE);
            appendText(dbgMsgBuf, L", contractIndex ");
            appendNumber(dbgMsgBuf, _currentContractIndex, FALSE);
            appendText(dbgMsgBuf, L", inputType ");
            appendNumber(dbgMsgBuf, inputType, FALSE);
            appendText(dbgMsgBuf, L", stackIndex ");
            appendNumber(dbgMsgBuf, _stackIndex, FALSE);
            addDebugMessage(dbgMsgBuf);
#endif
            // abort execution of contract here
            __qpiAbort(ContractErrorAllocInputOutputFailed);
        }

        outputBuffer = inputBuffer + fullInputSize;
        char* localsBuffer = outputBuffer + outputSize;
        if (inputSize < fullInputSize)
        {
            // less input data than expected by contract -> fill with 0
            setMem(inputBuffer + inputSize, fullInputSize - inputSize, 0);
        }
        else if (inputSize > fullInputSize)
        {
            // more input data than expected by contract -> discard additional bytes
            inputSize = fullInputSize;
        }
        copyMem(inputBuffer, inputPtr, inputSize);
        setMem(outputBuffer, outputSize + localsSize, 0);

        // acquire lock of contract state for writing (shouldn't block because 1 stack is not used by functions and thus kept free for procedures)
        contractStateLock[_currentContractIndex].acquireWrite();

        // run procedure
        const unsigned long long startTick = __rdtsc();
        contractUserProcedures[_currentContractIndex][inputType](*this, contractStates[_currentContractIndex], inputBuffer, outputBuffer, localsBuffer);
        _interlockedadd64(&contractTotalExecutionTicks[_currentContractIndex], __rdtsc() - startTick);

        // release lock of contract state and set state to changed
        contractStateLock[_currentContractIndex].releaseWrite();
        contractStateChangeFlags[_currentContractIndex >> 6] |= (1ULL << (_currentContractIndex & 63));
    }

    // free buffer after output has been copied (or isn't needed anymore)
    void freeBuffer()
    {
        if (_stackIndex < 0)
            return;

        // free data on stack
        contractLocalsStack[_stackIndex].free();
        ASSERT(contractLocalsStack[_stackIndex].size() == 0);

        // release stack lock
        releaseContractLocalsStack(_stackIndex);
    }
};

// QPI context used to call contract user function from qubic core (request processor)
struct QpiContextUserFunctionCall : public QPI::QpiContextFunctionCall
{
    char* outputBuffer;
    unsigned short outputSize;

    QpiContextUserFunctionCall(unsigned int contractIndex) : QPI::QpiContextFunctionCall(contractIndex, NULL_ID, 0, USER_FUNCTION_CALL)
    {
        outputBuffer = nullptr;
        outputSize = 0;
    }

    ~QpiContextUserFunctionCall()
    {
        freeBuffer();
    }

    // call function and return error code ContractError (output is invalid if != NoContractError)
    unsigned int call(unsigned short inputType, const void* inputPtr, unsigned short inputSize)
    {
#if !defined(NDEBUG) && !defined(NO_UEFI)
        CHAR16 dbgMsgBuf[300];
        setText(dbgMsgBuf, L"QpiContextUserFunctionCall in tick ");
        appendNumber(dbgMsgBuf, system.tick, FALSE);
        appendText(dbgMsgBuf, L": contract ");
        appendNumber(dbgMsgBuf, _currentContractIndex, FALSE);
        appendText(dbgMsgBuf, L", inputType ");
        appendNumber(dbgMsgBuf, inputType, FALSE);
        appendText(dbgMsgBuf, L", inputSize ");
        appendNumber(dbgMsgBuf, inputSize, FALSE);
        addDebugMessage(dbgMsgBuf);
#endif

        ASSERT(_currentContractIndex < contractCount);
        ASSERT(contractUserFunctions[_currentContractIndex][inputType]);

        // reserve stack for this processor (may block)
        constexpr unsigned int stacksNotUsedToReserveThemForStateWriter = 1;
        acquireContractLocalsStack(_stackIndex, stacksNotUsedToReserveThemForStateWriter);

        // allocate input, output, and locals buffer from stack and init them
        unsigned short fullInputSize = contractUserFunctionInputSizes[_currentContractIndex][inputType];
        outputSize = contractUserFunctionOutputSizes[_currentContractIndex][inputType];
        unsigned int localsSize = contractUserFunctionLocalsSizes[_currentContractIndex][inputType];
        char* inputBuffer = contractLocalsStack[_stackIndex].allocate(fullInputSize + outputSize + localsSize);
        if (!inputBuffer)
        {
#ifndef NDEBUG
            CHAR16 dbgMsgBuf[400];
            setText(dbgMsgBuf, L"QpiContextUserFunctionCall stack buffer alloc failed in tick ");
            appendNumber(dbgMsgBuf, system.tick, FALSE);
            addDebugMessage(dbgMsgBuf);
            setText(dbgMsgBuf, L"fullInputSize ");
            appendNumber(dbgMsgBuf, fullInputSize, FALSE);
            appendText(dbgMsgBuf, L", outputSize ");
            appendNumber(dbgMsgBuf, outputSize, FALSE);
            appendText(dbgMsgBuf, L", localsSize ");
            appendNumber(dbgMsgBuf, localsSize, FALSE);
            appendText(dbgMsgBuf, L", contractIndex ");
            appendNumber(dbgMsgBuf, _currentContractIndex, FALSE);
            appendText(dbgMsgBuf, L", inputType ");
            appendNumber(dbgMsgBuf, inputType, FALSE);
            appendText(dbgMsgBuf, L", stackIndex ");
            appendNumber(dbgMsgBuf, _stackIndex, FALSE);
            addDebugMessage(dbgMsgBuf);
#endif
            // abort execution of contract here
            __qpiAbort(ContractErrorAllocInputOutputFailed);
        }
        outputBuffer = inputBuffer + fullInputSize;
        char* localsBuffer = outputBuffer + outputSize;
        if (inputSize < fullInputSize)
        {
            // less input data than expected by contract -> fill with 0
            setMem(inputBuffer + inputSize, fullInputSize - inputSize, 0);
        }
        else if (inputSize > fullInputSize)
        {
            // more input data than expected by contract -> discard additional bytes
            inputSize = fullInputSize;
        }
        copyMem(inputBuffer, inputPtr, inputSize);
        setMem(outputBuffer, outputSize + localsSize, 0);

        // set error handler for canceling
        contractExecutionErrorData[_stackIndex].errorCode = NoContractError;
        if (SetJump(&contractExecutionErrorData[_stackIndex].longJumpBuffer) > 0)
        {
            // error handling code (long jump returns to here from somewhere inside the call of
            // contractUserFunctions() below)
            unsigned int errorCode = contractExecutionErrorData[_stackIndex].errorCode;

            // release all locks using stack unwinding
            rollbackContractFunctionCall(_stackIndex);
            ASSERT(contractLocalsStack[_stackIndex].size() == 0);

            // release stack
            releaseContractLocalsStack(_stackIndex);

            // return error code, allowing to handle error, for example retry later
            return errorCode;
        }

        // acquire lock of contract state for reading (may block)
        __qpiAcquireStateForReading(_currentContractIndex);

        // run function
        const unsigned long long startTick = __rdtsc();
        contractUserFunctions[_currentContractIndex][inputType](*this, contractStates[_currentContractIndex], inputBuffer, outputBuffer, localsBuffer);
        _interlockedadd64(&contractTotalExecutionTicks[_currentContractIndex], __rdtsc() - startTick);

        // release lock of contract state
        __qpiReleaseStateForReading(_currentContractIndex);

        return NoContractError;
    }

    // free buffer after output has been copied
    void freeBuffer()
    {
        if (_stackIndex < 0)
            return;

        // free data on stack
        contractLocalsStack[_stackIndex].free();
        ASSERT(contractLocalsStack[_stackIndex].size() == 0);

        // release stack
        releaseContractLocalsStack(_stackIndex);
    }
};
