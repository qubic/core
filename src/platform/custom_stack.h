#pragma once

#include "memory_util.h"
#include "debugging.h"

typedef unsigned int CustomStackSizeType;
typedef EFI_AP_PROCEDURE CustomStackProcessorFunc;

// Function call stack with custom size that can be used for running a function on it. Memory is allocated with allocPoolWithErrorLog().
class CustomStack
{
public:
    // Constructor (disabled because not called without MS CRT, you need to call init() to init)
    //CustomStack()
    //{
    //    init();
    //}

    // Init (set all to 0).
    void init()
    {
        stackTop = nullptr;
        stackBottom = nullptr;
        setupFuncToCall = nullptr;
        setupDataToPass = nullptr;
    }

    // Allocate memory, return if successful
    bool alloc(CustomStackSizeType size)
    {
        free();

        if (!allocPoolWithErrorLog(L"stackBottom", size, (void**)&stackBottom, __LINE__))
            return false;

        stackTop = stackBottom + size;

        setMem(stackBottom, size, 0x55);

        return true;
    }

    // Free memory
    void free()
    {
        if (stackBottom)
        {
            freePool(stackBottom);
            stackBottom = nullptr;
        }
        stackTop = nullptr;
    }

    // Get maximum stack size used so far
    CustomStackSizeType maxStackUsed() const
    {
        ASSERT(stackTop >= stackBottom);
        for (char* p = stackBottom; p < stackTop; ++p)
        {
            if (*p != 0x55)
                return CustomStackSizeType(stackTop - p);
        }
        return 0;
    }

    // Prepare function call with run()
    void setupFunction(CustomStackProcessorFunc functionToCall, void* dataToPassToFunction)
    {
        setupFuncToCall = functionToCall;
        setupDataToPass = dataToPassToFunction;
    }

    // Run function using custom stack allocated with alloc(), function is set with setup()
    static void runFunction(void* data);

private:
    char* stackTop;
    char* stackBottom;
    CustomStackProcessorFunc setupFuncToCall;
    void* setupDataToPass;
};

extern "C" void __customStackSetupAndRunFunc(void* newStackTop, CustomStackProcessorFunc funcToCall, void* dataToPass);

void CustomStack::runFunction(void* data)
{
    ASSERT(data != nullptr);
    CustomStack* me = reinterpret_cast<CustomStack*>(data);

    ASSERT(me->stackTop != nullptr);
    ASSERT(me->stackBottom != nullptr);
    ASSERT(me->setupFuncToCall != nullptr);
    ASSERT(me->stackTop > me->stackBottom);

    __customStackSetupAndRunFunc(me->stackTop, me->setupFuncToCall, me->setupDataToPass);
}

