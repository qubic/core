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


enum ContractError
{
    NoContractError = 0,
    ContractErrorAllocInputOutputFailed,
    ContractErrorAllocLocalsFailed,
    ContractErrorAllocContextOtherFunctionCallFailed,
    ContractErrorAllocContextOtherProcedureCallFailed,
    ContractErrorTooManyActions,
    ContractErrorTimeout,
    ContractErrorStoppedToResolveDeadlock,
};

// Used to store: locals and for first invocation level also input and output
typedef StackBuffer<unsigned int, 32 * 1024 * 1024> ContractLocalsStack;
GLOBAL_VAR_DECL ContractLocalsStack contractLocalsStack[NUMBER_OF_CONTRACT_EXECUTION_BUFFERS];
GLOBAL_VAR_DECL volatile char contractLocalsStackLock[NUMBER_OF_CONTRACT_EXECUTION_BUFFERS];
GLOBAL_VAR_DECL volatile long contractLocalsStackLockWaitingCount;
GLOBAL_VAR_DECL long contractLocalsStackLockWaitingCountMax;


GLOBAL_VAR_DECL ReadWriteLock contractStateLock[contractCount];
GLOBAL_VAR_DECL unsigned char* contractStates[contractCount];
GLOBAL_VAR_DECL volatile long long contractTotalExecutionTicks[contractCount];
GLOBAL_VAR_DECL unsigned int contractError[contractCount];

// TODO: If we ever have parallel procedure calls (of different contracts), we need to make
// access to contractStateChangeFlags thread-safe
GLOBAL_VAR_DECL unsigned long long* contractStateChangeFlags GLOBAL_VAR_INIT(nullptr);


// Contract system procedures that serve as callbacks, such as PRE_ACQUIRE_SHARES,
// break the rule that contracts can only call other contracts with lower index.
// This requires special handling in order to prevent deadlocks.
// Further, calling certain QPI functions in callbacks must be prevented in order
// to prevent cyclic calling patterns.
GLOBAL_VAR_DECL unsigned int contractCallbacksRunning;
enum ContractCallbacksRunningFlags
{
    NoContractCallback = 0,
    ContractCallbackManagementRightsTransfer = 1,
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
    while (TRY_ACQUIRE(contractLocalsStackLock[i]) == false)
    {
        _mm_pause();
        ++i;
        if (i == NUMBER_OF_CONTRACT_EXECUTION_BUFFERS)
            i = stacksToIgnore;
    }

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

// Called before one contract calls a procedure of a different contract
const QpiContextProcedureCall& QPI::QpiContextProcedureCall::__qpiConstructContextOtherContractProcedureCall(unsigned int otherContractIndex, QPI::sint64 invocationReward) const
{
    ASSERT(_entryPoint != USER_FUNCTION_CALL);
    ASSERT(otherContractIndex < _currentContractIndex || contractCallbacksRunning != NoContractCallback);
    ASSERT(_stackIndex >= 0 && _stackIndex < NUMBER_OF_CONTRACT_EXECUTION_BUFFERS);
    char* buffer = contractLocalsStack[_stackIndex].allocate(sizeof(QpiContextProcedureCall));
    if (!buffer)
    {
#ifndef NDEBUG
        CHAR16 dbgMsgBuf[400];
        setText(dbgMsgBuf, L"__qpiConstructContextOtherContractProcedureCall stack buffer alloc failed in tick ");
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
    QpiContextProcedureCall& newContext = *reinterpret_cast<QpiContextProcedureCall*>(buffer);
    if (transfer(QPI::id(otherContractIndex, 0, 0, 0), invocationReward) < 0)
        invocationReward = 0;
    newContext.init(otherContractIndex, _originator, _currentContractId, invocationReward, _entryPoint, _stackIndex);
    return newContext;
}

// Called before one contract calls a function or procedure of a different contract
void QPI::QpiContextFunctionCall::__qpiFreeContextOtherContract() const
{
    ASSERT(_stackIndex >= 0 && _stackIndex < NUMBER_OF_CONTRACT_EXECUTION_BUFFERS);
    contractLocalsStack[_stackIndex].free();
}

// Used to acquire a contract state lock when one contract calls a function of a different contract
void* QPI::QpiContextFunctionCall::__qpiAcquireStateForReading(unsigned int contractIndex) const
{
    ASSERT(_stackIndex >= 0 && _stackIndex < NUMBER_OF_CONTRACT_EXECUTION_BUFFERS);
    ASSERT(contractIndex < contractCount);
    ASSERT(contractIndex < _currentContractIndex);

    // Add rollback info for this lock to the stack
    auto rollbackInfo = reinterpret_cast<ContractRollbackInfo*>(contractLocalsStack[_stackIndex].allocateSpecial(sizeof(ContractRollbackInfo)));
    rollbackInfo->contractIndex = contractIndex;

    if (contractCallbacksRunning == NoContractCallback)
    {
        // Default case: no callback is running
        // -> Contracts can only call contracts with lower index.
        // -> No deadlock possible, because multiple readers are supported by the lock
        //    and only contract processor acquires write lock
        contractStateLock[contractIndex].acquireRead();
        rollbackInfo->type = ContractRollbackInfo::ContractStateReadLock;
    }
    else
    {
        // Special case: callback is running
        // -> Special handling needed to prevent deadlocks
        if (_entryPoint == USER_FUNCTION_CALL)
        {
            // Entry point is user function (running in request processor)
            if (!contractStateLock[contractIndex].tryAcquireRead())
            {
                // Waiting for this lock may cause a deadlock
                // -> This is low priority and should be stopped to prevent potential deadlocks
                contractLocalsStack[_stackIndex].free();
                __qpiAbort(ContractErrorStoppedToResolveDeadlock);
            }
        }
        else
        {
            // Entry point is procedure (running in contract processor)
            // -> Contract can call contract of any index and state may be already locked for writing!
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
            if (contractStateLock[contractIndex].isLockedForWriting())
            {
                // already locked by this processor
                // -> signal situation for corresponding __qpiReleaseStateForReading()
                // -> give read access to state without further ado
                rollbackInfo->type = ContractRollbackInfo::ContractStateReuseLock;
            }
            else
            {
                // not locked or already locked by a request processor for reading (contract function)
                // -> acquire lock as usual
                // -> if there is a deadlock with a request processor, the following acquireRead()
                //    needs to wait until it gets resolved by stopping the request processor
                contractStateLock[contractIndex].acquireRead();
                rollbackInfo->type = ContractRollbackInfo::ContractStateReadLock;
            }
        }
    }

    return contractStates[contractIndex];
}

// Used to release a contract state lock when one contract calls a function of a different contract
void QPI::QpiContextFunctionCall::__qpiReleaseStateForReading(unsigned int contractIndex) const
{
    ASSERT(_stackIndex >= 0 && _stackIndex < NUMBER_OF_CONTRACT_EXECUTION_BUFFERS);
    ASSERT(contractIndex < contractCount);
    ASSERT(contractIndex < _currentContractIndex);
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
        //         callback of C2 -> __qpiCallSystemProcOfOtherContract() tries to acquire write
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

// Used to call a special system procedure of another contract from within a contract for example in asset management rights transfer
template <unsigned int sysProcId, typename InputType, typename OutputType>
void QPI::QpiContextProcedureCall::__qpiCallSystemProcOfOtherContract(unsigned int otherContractIndex, InputType& input, OutputType& output, QPI::sint64 invocationReward) const
{
    // Make sure this function is used with an expected combination of sysProcId, input,
    // and output
    static_assert(
        (sysProcId == PRE_RELEASE_SHARES && sizeof(InputType) == sizeof(QPI::PreManagementRightsTransfer_input) && sizeof(OutputType) == sizeof(QPI::PreManagementRightsTransfer_output))
        || (sysProcId == PRE_ACQUIRE_SHARES && sizeof(InputType) == sizeof(QPI::PreManagementRightsTransfer_input) && sizeof(OutputType) == sizeof(QPI::PreManagementRightsTransfer_output))
        || (sysProcId == POST_RELEASE_SHARES && sizeof(InputType) == sizeof(QPI::PostManagementRightsTransfer_input) && sizeof(OutputType) == sizeof(QPI::NoData))
        || (sysProcId == POST_ACQUIRE_SHARES && sizeof(InputType) == sizeof(QPI::PostManagementRightsTransfer_input) && sizeof(OutputType) == sizeof(QPI::NoData))
        , "Unsupported __qpiCallSystemProcOfOtherContract() call"
    );

    // Check that this internal function is used correctly
    ASSERT(_entryPoint != USER_FUNCTION_CALL);
    ASSERT(otherContractIndex < contractCount);
    ASSERT(otherContractIndex != _currentContractIndex);

    // Initialize output with 0
    setMem(&output, sizeof(output), 0);

    // Empty procedures lead to null pointer in contractSystemProcedures -> return default output (all zero/false)
    if (!contractSystemProcedures[otherContractIndex][sysProcId])
        return;

    // Set flags of callbacks currently running (to prevent deadlocks and nested calling of QPI functions)
    auto contractCallbacksRunningBefore = contractCallbacksRunning;
    if (sysProcId == PRE_RELEASE_SHARES || sysProcId == PRE_ACQUIRE_SHARES
        || sysProcId == POST_RELEASE_SHARES || sysProcId == POST_ACQUIRE_SHARES)
    {
        contractCallbacksRunning |= ContractCallbackManagementRightsTransfer;
    }

    // Create context for other contract and lock state for writing
    const QpiContextProcedureCall& otherContractContext = __qpiConstructContextOtherContractProcedureCall(otherContractIndex, invocationReward);
    void* otherContractState = __qpiAcquireStateForWriting(otherContractIndex);

    // Alloc locals
    unsigned short localsSize = contractSystemProcedureLocalsSizes[otherContractIndex][sysProcId];
    char* localsBuffer = contractLocalsStack[_stackIndex].allocate(localsSize);
    if (!localsBuffer)
        __qpiAbort(ContractErrorAllocLocalsFailed);
    setMem(localsBuffer, localsSize, 0);

    // Run procedure
    contractSystemProcedures[otherContractIndex][sysProcId](otherContractContext, otherContractState, &input, &output, localsBuffer);

    // Release lock and free context and locals
    contractLocalsStack[_stackIndex].free();
    __qpiReleaseStateForWriting(otherContractIndex);
    __qpiFreeContextOtherContract();

    // Restore flags of callbacks currently running
    contractCallbacksRunning = contractCallbacksRunningBefore;
}

// Enter endless loop leading to timeout of contractProcessor() and rollback
void QPI::QpiContextFunctionCall::__qpiAbort(unsigned int errorCode) const
{
    ASSERT(_currentContractIndex < contractCount);
    contractError[_currentContractIndex] = errorCode;

#if !defined(NDEBUG)
    CHAR16 dbgMsgBuf[200];
    setText(dbgMsgBuf, L"__qpiAbort() called in tick ");
    appendNumber(dbgMsgBuf, system.tick, FALSE);
    addDebugMessage(dbgMsgBuf);
    setText(dbgMsgBuf, L"contractIndex ");
    appendNumber(dbgMsgBuf, _currentContractIndex, FALSE);
    appendText(dbgMsgBuf, L", errorCode ");
    appendNumber(dbgMsgBuf, errorCode, FALSE);
    addDebugMessage(dbgMsgBuf);
#endif
    // TODO: How to do error handing in user functions? (request processor has no timeout / respawn)
    //       Caller need to be distinguished here (function may be called from procedure)

    // we have to wait for the timeout, because there seems to be no function to stop the processor
    // TODO: we may add a function to CustomStack for directly returning from the runFunction()
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
struct QpiContextSystemProcedureCall : public QPI::QpiContextProcedureCall
{
    QpiContextSystemProcedureCall(unsigned int contractIndex, SystemProcedureID systemProcId) : QPI::QpiContextProcedureCall(contractIndex, NULL_ID, 0, systemProcId)
    {
        contractActionTracker.init();
    }

    void call()
    {
        const int systemProcId = _entryPoint;

        ASSERT(_currentContractIndex < contractCount);
        ASSERT(systemProcId < contractSystemProcedureCount);

        // Empty procedures lead to null pointer in contractSystemProcedures -> nothing to call
        if (!contractSystemProcedures[_currentContractIndex][systemProcId])
            return;

        // At the moment, all contract called from without a QpiContext are without input, output, and invocation reward.
        // If this changes in the future, we should add a template overload function call() with passing
        // the input and output types and instance references as additional parameters. Also, the invocation reward should
        // be passed as an argument to the constructor.
        ASSERT(
            systemProcId == INITIALIZE
            || systemProcId == BEGIN_EPOCH
            || systemProcId == END_EPOCH
            || systemProcId == BEGIN_TICK
            || systemProcId == END_TICK
        );
        QPI::NoData noInOutData;
        // reserve resources for this processor (may block)
        contractStateLock[_currentContractIndex].acquireWrite();

        const unsigned long long startTick = __rdtsc();
        unsigned short localsSize = contractSystemProcedureLocalsSizes[_currentContractIndex][systemProcId];
        if (localsSize == sizeof(QPI::NoData))
        {
            // no locals -> call
            contractSystemProcedures[_currentContractIndex][systemProcId](*this, contractStates[_currentContractIndex], &noInOutData, &noInOutData, &noInOutData);
        }
        else
        {
            // locals required: reserve stack and use stack (should not block because stack 0 is reserved for procedures)
            acquireContractLocalsStack(_stackIndex);
            char* localsBuffer = contractLocalsStack[_stackIndex].allocate(localsSize);
            if (!localsBuffer)
                __qpiAbort(ContractErrorAllocLocalsFailed);
            setMem(localsBuffer, localsSize, 0);

            // call system proc
            contractSystemProcedures[_currentContractIndex][systemProcId](*this, contractStates[_currentContractIndex], &noInOutData, &noInOutData, localsBuffer);

            // free data on stack and release stack
            contractLocalsStack[_stackIndex].free();
            ASSERT(contractLocalsStack[_stackIndex].size() == 0);
            releaseContractLocalsStack(_stackIndex);
        }
        _interlockedadd64(&contractTotalExecutionTicks[_currentContractIndex], __rdtsc() - startTick);

        // release lock of contract state and set state to changed
        contractStateLock[_currentContractIndex].releaseWrite();
        contractStateChangeFlags[_currentContractIndex >> 6] |= (1ULL << (_currentContractIndex & 63));
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

    // call function
    void call(unsigned short inputType, const void* inputPtr, unsigned short inputSize)
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

        // acquire lock of contract state for reading (may block)
        contractStateLock[_currentContractIndex].acquireRead();

        // run function
        const unsigned long long startTick = __rdtsc();
        contractUserFunctions[_currentContractIndex][inputType](*this, contractStates[_currentContractIndex], inputBuffer, outputBuffer, localsBuffer);
        _interlockedadd64(&contractTotalExecutionTicks[_currentContractIndex], __rdtsc() - startTick);

        // release lock of contract state
        contractStateLock[_currentContractIndex].releaseRead();
    }

    // free buffer after output has been copied
    void freeBuffer()
    {
        if (_stackIndex < 0)
            return;

        // free data on stack
        contractLocalsStack[_stackIndex].free();
        ASSERT(contractLocalsStack[_stackIndex].size() == 0);

        // release locks
        releaseContractLocalsStack(_stackIndex);
    }
};
