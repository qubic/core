// This header allows access to the CR4 control register in x86/x86_64 architectures for different compilers.
// This is relevant to setting and reading the CR4 register, which controls various processor features such as paging, protection, 
// and in particular the use of AVX and other advanced features in the processor.

#pragma once

#include <lib/platform_common/qstdint.h>

#define _XCR_XFEATURE_ENABLED_MASK 0

#if defined(_MSC_VER) && !defined(__clang__)
    #pragma message("Compiling with MSVC path")
     // MSVC - Use built-in intrinsics
    #include <intrin.h>
    
    static inline uint64_t read_cr4(void) {
        return __readcr4();
    }
    
    static inline void write_cr4(uint64_t val) {
        __writecr4(val);
    }

#elif defined(__clang__)
    // Clang - Use inline assembly
   
    #include <immintrin.h>
    
    static inline uint64_t read_cr4(void) {
        uint64_t val;
        __asm__ volatile("mov %%cr4, %0" : "=r" (val));
        return val;
    }
    
    static inline void write_cr4(uint64_t val) {
        __asm__ volatile("mov %0, %%cr4" : : "r" (val) : "memory");
    }

#else
    #error "Unsupported compiler for CR4 access"
#endif

static void enableAVX()
{
    // Enable OSXSAVE (bit 18) in CR4
    write_cr4(read_cr4() | 0x40000);

    // Enable x87 FPU state (bit 0), SSE state (bit 1), and AVX state (bit 2) in XCR0
    _xsetbv(_XCR_XFEATURE_ENABLED_MASK, _xgetbv(_XCR_XFEATURE_ENABLED_MASK) | (7
#ifdef __AVX512F__
        // If AVX512F is defined, enable AVX-512 features:
        // - AVX512 opmask (bit 5)
        // - AVX-512 upper 256 bits oZMM0-ZMM15 (bit 6) 
        // - AVX-512 ZMM16-ZMM31 (bit 7)
        | 224
#endif
        ));
}