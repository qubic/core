// Not used anymore because custom_stack.h is easier to use provides same/more features -> may be removed
#pragma once

#include "debugging.h"


// Define StackSizeTracker, recommended to use in global scope
#define DEFINE_STACK_SIZE_TRACKER(name) static StackSizeTracker name

// Init stack size tracker, use this as in thread entry point function
#define INIT_STACK_SIZE_TRACKER(name) name.initStackTop(_AddressOfReturnAddress())

// Update tracker, call this in functions called by thread
#define UPDATE_STACK_SIZE_TRACKER(name) name.update()


// Track size of function call stack used (x86 stack grows downwards)
// This is not byte-accurate, but approximate.
class StackSizeTracker
{
public:
    static constexpr unsigned long long uninitialized = -1;

    StackSizeTracker()
    {
        reset();
    }

    void reset()
    {
        pTop = uninitialized;
        pBottom = uninitialized;
    }

    // Call this once passing the top of the stack (first variable at entry point)
    void initStackTop(void* stackTop)
    {
        unsigned long long pTopNew = (unsigned long long)stackTop;
        if (pTop != uninitialized && pBottom != uninitialized && pTop != pTopNew)
        {
            // Keep max size value if the stack tracker is reused with other tack top
            ASSERT(pTop >= pBottom);
            unsigned long long maxSize = pTop - pBottom;
            pBottom = pTopNew - maxSize;
        }
        pTop = pTopNew;
    }

    // Update the maximum used stack size. Call this in deep functions
    void update()
    {
        unsigned long long pCurBottom = (unsigned long long) & pCurBottom;
        if (pCurBottom < pBottom)
            pBottom = pCurBottom;
    }

    // Maximum of stack size used based on calls of update() and initStackTop(),
    // or unitialized if one of the two functions has not been called.
    unsigned long long maxStackSize() const
    {
        if (pTop == uninitialized || pBottom == uninitialized)
            return uninitialized;
        ASSERT(pTop >= pBottom);
        return pTop - pBottom;
    }

private:
    unsigned long long pTop, pBottom;
};
