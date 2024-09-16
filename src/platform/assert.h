#pragma once

#if defined(EXPECT_TRUE)

// in gtest context, use EXPECT_TRUE as ASSERT
#define ASSERT EXPECT_TRUE

#elif defined(NDEBUG)

// with NDEBUG, make ASSERT disappear
#define ASSERT(expression) ((void)0)

#else

// otherwise, use addDebugMessageAssert() defined in debugging.h
#define ASSERT(expression) (void)(                                                       \
            (!!(expression)) ||                                                              \
            (addDebugMessageAssert(_CRT_WIDE(#expression), _CRT_WIDE(__FILE__), (unsigned int)(__LINE__)), 0) \
        )

static void addDebugMessageAssert(const wchar_t* message, const wchar_t* file, const unsigned int lineNumber);

#endif
