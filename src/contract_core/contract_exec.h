#pragma once

#include "public_settings.h"

#include "platform/read_write_lock.h"
#include "platform/debugging.h"
#include "platform/memory.h"

#include "contract_core/contract_def.h"
#include "contract_core/stack_buffer.h"

// TODO: remove, only for debug output
#include "system.h"

// Used to store: locals and for first invocation level also input and output
typedef StackBuffer<unsigned int, 32 * 1024 * 1024> ContractLocalsStack;
ContractLocalsStack contractLocalsStack[NUMBER_OF_CONTRACT_EXECUTION_PROCESSORS];
static volatile char contractLocalsStackLock[NUMBER_OF_CONTRACT_EXECUTION_PROCESSORS];


static ReadWriteLock contractStateLock[contractCount];
static unsigned char* contractStates[contractCount];
static volatile long long contractTotalExecutionTicks[contractCount];

// TODO: If we ever have parallel procedure calls (of different contracts), we need to make
// access to contractStateChangeFlags thread-safe
static unsigned long long* contractStateChangeFlags = NULL;


bool initContractExec()
{
    for (ContractLocalsStack::SizeType i = 0; i < NUMBER_OF_CONTRACT_EXECUTION_PROCESSORS; ++i)
        contractLocalsStack[i].init();
    setMem((void*)contractLocalsStackLock, sizeof(contractLocalsStackLock), 0);

    setMem((void*)contractTotalExecutionTicks, sizeof(contractTotalExecutionTicks), 0);
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
    static_assert(NUMBER_OF_CONTRACT_EXECUTION_PROCESSORS >= 2, "NUMBER_OF_CONTRACT_EXECUTION_PROCESSORS should be at least 2.");
    ASSERT(stackIdx < 0);
    ASSERT(stacksToIgnore < NUMBER_OF_CONTRACT_EXECUTION_PROCESSORS);

    int i = stacksToIgnore;
    while (TRY_ACQUIRE(contractLocalsStackLock[i]) == false)
    {
        _mm_pause();
        ++i;
        if (i == NUMBER_OF_CONTRACT_EXECUTION_PROCESSORS)
            i = stacksToIgnore;
    }

    stackIdx = i;
    ASSERT(stackIdx >= 0);
}

// Release locked stack (and reset stackIdx)
void releaseContractLocalsStack(int& stackIdx)
{
    ASSERT(stackIdx >= 0);
    ASSERT(stackIdx < NUMBER_OF_CONTRACT_EXECUTION_PROCESSORS);
    ASSERT(contractLocalsStackLock[stackIdx]);
    RELEASE(contractLocalsStackLock[stackIdx]);
    stackIdx = -1;
}

// Allocate storage on ContractLocalsStack of QPI execution context
void* QPI::QpiContextFunctionCall::__qpiAllocLocals(unsigned int sizeOfLocals) const
{
    ASSERT(_stackIndex >= 0 && _stackIndex < NUMBER_OF_CONTRACT_EXECUTION_PROCESSORS);
    if (_stackIndex < 0 || _stackIndex >= NUMBER_OF_CONTRACT_EXECUTION_PROCESSORS)
    {
#ifndef NDEBUG
        CHAR16 dbgMsgBuf[100];
        setText(dbgMsgBuf, L"__qpiAllocLocals in ");
        appendNumber(dbgMsgBuf, system.tick, FALSE);
        appendText(dbgMsgBuf, L" called with stackIndex ");
        appendNumber(dbgMsgBuf, _stackIndex, FALSE);
        addDebugMessage(dbgMsgBuf);
#endif
        return 0; // TODO: log problem / restart processor here instead of returning
    }
    void* p = contractLocalsStack[_stackIndex].allocate(sizeOfLocals);
#ifndef NDEBUG
    if (!p)
    {
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
    }
#endif
    if (p)
        setMem(p, sizeOfLocals, 0);
    return p;
}

// Free last allocated storage on ContractLocalsStack of QPI execution context
void QPI::QpiContextFunctionCall::__qpiFreeLocals() const
{
    ASSERT(_stackIndex >= 0 && _stackIndex < NUMBER_OF_CONTRACT_EXECUTION_PROCESSORS);
    if (_stackIndex < 0 || _stackIndex >= NUMBER_OF_CONTRACT_EXECUTION_PROCESSORS)
        return;
    contractLocalsStack[_stackIndex].free();
}

// Called before one contract calls a function of a different contract
const QpiContextFunctionCall& QPI::QpiContextFunctionCall::__qpiConstructContextOtherContractFunctionCall(unsigned int otherContractIndex) const
{
    ASSERT(_stackIndex >= 0 && _stackIndex < NUMBER_OF_CONTRACT_EXECUTION_PROCESSORS);
    char * buffer = contractLocalsStack[_stackIndex].allocate(sizeof(QpiContextFunctionCall));
#ifndef NDEBUG
    if (!buffer)
    {
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
    }
#endif
    QpiContextFunctionCall& newContext = *reinterpret_cast<QpiContextFunctionCall*>(buffer);
    newContext.init(otherContractIndex, _originator, _currentContractId, _invocationReward);
    return newContext;
}

// Called before one contract calls a procedure of a different contract
const QpiContextProcedureCall& QPI::QpiContextProcedureCall::__qpiConstructContextOtherContractProcedureCall(unsigned int otherContractIndex, QPI::sint64 invocationReward) const
{
    ASSERT(_stackIndex >= 0 && _stackIndex < NUMBER_OF_CONTRACT_EXECUTION_PROCESSORS);
    char* buffer = contractLocalsStack[_stackIndex].allocate(sizeof(QpiContextProcedureCall));
#ifndef NDEBUG
    if (!buffer)
    {
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
    }
#endif
    QpiContextProcedureCall& newContext = *reinterpret_cast<QpiContextProcedureCall*>(buffer);
    if (transfer(QPI::id(otherContractIndex, 0, 0, 0), invocationReward) < 0)
        invocationReward = 0;
    newContext.init(otherContractIndex, _originator, _currentContractId, invocationReward);
    return newContext;
}

// Called before one contract calls a function or procedure of a different contract
void QPI::QpiContextFunctionCall::__qpiFreeContextOtherContract() const
{
    ASSERT(_stackIndex >= 0 && _stackIndex < NUMBER_OF_CONTRACT_EXECUTION_PROCESSORS);
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


// QPI context used to call contract system procedure from qubic core (contract processor)
struct QpiContextSystemProcedureCall : public QPI::QpiContextProcedureCall
{
    QpiContextSystemProcedureCall(unsigned int contractIndex) : QPI::QpiContextProcedureCall(contractIndex, NULL_ID, 0)
    {
    }

    void call(SystemProcedureID systemProcId)
    {
        ASSERT(_currentContractIndex < contractCount);

        // reserve resources for this processor (may block)
        contractStateLock[_currentContractIndex].acquireWrite();

        const unsigned long long startTick = __rdtsc();
        contractSystemProcedures[_currentContractIndex][systemProcId](*this, contractStates[_currentContractIndex]);
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
    }

    void call(unsigned short inputType, const void* inputPtr, unsigned short inputSize)
    {
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
        ASSERT(inputBuffer); // TODO: error handling
#ifndef NDEBUG
        if (!inputBuffer)
        {
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
        }
#endif

        char* outputBuffer = inputBuffer + fullInputSize;
        char* localsBuffer = outputBuffer + outputSize;
        if (fullInputSize > inputSize)
            setMem(inputBuffer + inputSize, fullInputSize - inputSize, 0);
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
        ASSERT(inputBuffer); // TODO: error handling
#ifndef NDEBUG
        if (!inputBuffer)
        {
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
        }
#endif
        outputBuffer = inputBuffer + fullInputSize;
        char* localsBuffer = outputBuffer + outputSize;
        if (fullInputSize > inputSize)
            setMem(inputBuffer + inputSize, fullInputSize - inputSize, 0);
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



