#pragma once

#include "public_settings.h"

#include "platform/read_write_lock.h"
#include "platform/debugging.h"
#include "platform/memory.h"

#include "contract_core/contract_def.h"
#include "contract_core/stack_buffer.h"
#include "contract_core/contract_action_tracker.h"

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
};

// Used to store: locals and for first invocation level also input and output
typedef StackBuffer<unsigned int, 32 * 1024 * 1024> ContractLocalsStack;
ContractLocalsStack contractLocalsStack[NUMBER_OF_CONTRACT_EXECUTION_BUFFERS];
static volatile char contractLocalsStackLock[NUMBER_OF_CONTRACT_EXECUTION_BUFFERS];
static volatile long contractLocalsStackLockWaitingCount = 0;
static long contractLocalsStackLockWaitingCountMax = 0;


static ReadWriteLock contractStateLock[contractCount];
static unsigned char* contractStates[contractCount];
static volatile long long contractTotalExecutionTicks[contractCount];
static unsigned int contractError[contractCount];

// TODO: If we ever have parallel procedure calls (of different contracts), we need to make
// access to contractStateChangeFlags thread-safe
static unsigned long long* contractStateChangeFlags = NULL;

static ContractActionTracker<1024> contractActionTracker;


bool initContractExec()
{
    for (ContractLocalsStack::SizeType i = 0; i < NUMBER_OF_CONTRACT_EXECUTION_BUFFERS; ++i)
        contractLocalsStack[i].init();
    setMem((void*)contractLocalsStackLock, sizeof(contractLocalsStackLock), 0);

    setMem((void*)contractTotalExecutionTicks, sizeof(contractTotalExecutionTicks), 0);
    setMem((void*)contractError, sizeof(contractError), 0);
    for (int i = 0; i < contractCount; ++i)
    {
        contractStateLock[i].reset();
    }

    return true;
}

// Acquire lock of an currently unused stack (may block if all in use)
// stacksToIgnore > 0 can be passed by low priority tasks to keep some stacks reserved for high prio purposes.
void acquireContractLocalsStack(int& stackIdx, unsigned int stacksToIgnore = 0)
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
}

// Release locked stack (and reset stackIdx)
void releaseContractLocalsStack(int& stackIdx)
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
    newContext.init(otherContractIndex, _originator, _currentContractId, _invocationReward);
    return newContext;
}

// Called before one contract calls a procedure of a different contract
const QpiContextProcedureCall& QPI::QpiContextProcedureCall::__qpiConstructContextOtherContractProcedureCall(unsigned int otherContractIndex, QPI::sint64 invocationReward) const
{
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
    newContext.init(otherContractIndex, _originator, _currentContractId, invocationReward);
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
    ASSERT(contractIndex < contractCount);
    contractStateLock[contractIndex].acquireRead();
    return contractStates[contractIndex];
}

// Used to release a contract state lock when one contract calls a function of a different contract
void QPI::QpiContextFunctionCall::__qpiReleaseStateForReading(unsigned int contractIndex) const
{
    ASSERT(contractIndex < contractCount);
    contractStateLock[contractIndex].releaseRead();
}

// Used to acquire a contract state lock when one contract calls a procedure of a different contract
void* QPI::QpiContextProcedureCall::__qpiAcquireStateForWriting(unsigned int contractIndex) const
{
    ASSERT(contractIndex < contractCount);
    contractStateLock[contractIndex].acquireWrite();
    return contractStates[contractIndex];
}

// Used to release a contract state lock when one contract calls a procedure of a different contract
void QPI::QpiContextProcedureCall::__qpiReleaseStateForWriting(unsigned int contractIndex) const
{
    ASSERT(contractIndex < contractCount);
    contractStateLock[contractIndex].releaseWrite();
    contractStateChangeFlags[_currentContractIndex >> 6] |= (1ULL << (_currentContractIndex & 63));
}

// Used to call a special system procedure of another contract from within a contract /for example in asset management rights transfer
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

    // Check that contract index to be called is valid and not different from the caller contract (if compiled in Debug mode)
    ASSERT(otherContractIndex < contractCount);
    ASSERT(otherContractIndex != _currentContractIndex);

    // Initialize output with 0
    setMem(&output, sizeof(output), 0);

    // Create context for other contract and lock state for writing
    const QpiContextProcedureCall& otherContractContext = __qpiConstructContextOtherContractProcedureCall(otherContractIndex, invocationReward);
    void* otherContractState = __qpiAcquireStateForWriting(otherContractIndex);

    // Run procedure
    contractSystemProcedures[otherContractIndex][sysProcId](otherContractContext, otherContractState, &input, &output);

    // Release lock and free context
    __qpiReleaseStateForWriting(otherContractIndex);
    __qpiFreeContextOtherContract();
}

// Enter endless loop leading to timeout of contractProcessor() and rollback
void QPI::QpiContextFunctionCall::__qpiAbort(unsigned int errorCode) const
{
    ASSERT(_currentContractIndex < contractCount);
    contractError[_currentContractIndex] = errorCode;

#ifndef NDEBUG
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





// QPI context used to call contract system procedure from qubic core (contract processor)
struct QpiContextSystemProcedureCall : public QPI::QpiContextProcedureCall
{
    QpiContextSystemProcedureCall(unsigned int contractIndex) : QPI::QpiContextProcedureCall(contractIndex, NULL_ID, 0)
    {
        contractActionTracker.init();
    }

    void call(SystemProcedureID systemProcId)
    {
        ASSERT(_currentContractIndex < contractCount);
        ASSERT(systemProcId < contractSystemProcedureCount);

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
        contractSystemProcedures[_currentContractIndex][systemProcId](*this, contractStates[_currentContractIndex], &noInOutData, &noInOutData);
        _interlockedadd64(&contractTotalExecutionTicks[_currentContractIndex], __rdtsc() - startTick);

        // release lock of contract state and set state to changed
        contractStateLock[_currentContractIndex].releaseWrite();
        contractStateChangeFlags[_currentContractIndex >> 6] |= (1ULL << (_currentContractIndex & 63));
    }
};

// QPI context used to call contract user procedure from qubic core (contract processor)
struct QpiContextUserProcedureCall : public QPI::QpiContextProcedureCall
{
    QpiContextUserProcedureCall(unsigned int contractIndex, const m256i& originator, long long invocationReward) : QPI::QpiContextProcedureCall(contractIndex, originator, invocationReward)
    {
        contractActionTracker.init();
        if (!contractActionTracker.addQuTransfer(_originator, _currentContractId, _invocationReward))
            __qpiAbort(ContractErrorTooManyActions);
    }

    void call(unsigned short inputType, const void* inputPtr, unsigned short inputSize)
    {
#ifndef NDEBUG
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
        ASSERT(contractLocalsStack[_stackIndex].size() == 0);
        if (contractLocalsStack[_stackIndex].size())
            contractLocalsStack[_stackIndex].freeAll();

        // allocate input, output, and locals buffer from stack and init them
        unsigned short fullInputSize = contractUserProcedureInputSizes[_currentContractIndex][inputType];
        unsigned short outputSize = contractUserProcedureOutputSizes[_currentContractIndex][inputType];
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

        char* outputBuffer = inputBuffer + fullInputSize;
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

        // acquire lock of contract state for writing (may block)
        contractStateLock[_currentContractIndex].acquireWrite();

        // run procedure
        const unsigned long long startTick = __rdtsc();
        contractUserProcedures[_currentContractIndex][inputType](*this, contractStates[_currentContractIndex], inputBuffer, outputBuffer, localsBuffer);
        _interlockedadd64(&contractTotalExecutionTicks[_currentContractIndex], __rdtsc() - startTick);

        // release lock of contract state and set state to changed
        contractStateLock[_currentContractIndex].releaseWrite();
        contractStateChangeFlags[_currentContractIndex >> 6] |= (1ULL << (_currentContractIndex & 63));

        // free data on stack (output is unused)
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

    QpiContextUserFunctionCall(unsigned int contractIndex) : QPI::QpiContextFunctionCall(contractIndex, NULL_ID, 0)
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
#ifndef NDEBUG
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
        ASSERT(contractLocalsStack[_stackIndex].size() == 0);
        if (contractLocalsStack[_stackIndex].size())
            contractLocalsStack[_stackIndex].freeAll();

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
