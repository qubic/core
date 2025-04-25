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
            (addDebugMessageAssert(#expression, __FILE__, (unsigned int)(__LINE__)), 0) \
        )

static void addDebugMessageAssert(const char* message, const char* file, const unsigned int lineNumber);

#endif
