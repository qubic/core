#include <lib/platform_common/qintrin.h>
#include <lib/platform_efi/uefi.h>
#include <lib/platform_efi/enable_avx.h>
#include <src/platform/m256.h>

#define EFI_TEXT(s) (CHAR16*)(L##s)

// Global SystemTable pointer
EFI_SYSTEM_TABLE *gSystemTable = nullptr;
static unsigned long long g_tsc_frequency = 0;

// Helper function to convert unsigned long long to CHAR16 hex string
void ull_to_hex_str(unsigned long long n, CHAR16* out_str) {
    if (out_str == nullptr) return;

    const CHAR16* hex_chars = EFI_TEXT("0123456789ABCDEF");
    // Max 16 hex digits for 64-bit number + null terminator
    CHAR16 buffer[17]; 
    int i = 15;
    buffer[16] = L'\0'; 

    if (n == 0) {
        out_str[0] = L'0';
        out_str[1] = L'\0';
        return;
    }

    while (n > 0 && i >= 0) {
        buffer[i--] = hex_chars[n % 16];
        n /= 16;
    }
    
    // Copy to output string, skipping leading zeros if any
    int j = 0;
    while(buffer[i+1+j] == L'0' && buffer[i+2+j] != L'\0') { // keep last zero if number is 0
        i++;
    }
    
    int k = 0;
    if (i < 0) i = 0; // handle case where n was 0 initially
    while(buffer[i+1+k] != L'\0') {
        out_str[k] = buffer[i+1+k];
        k++;
    }
    out_str[k] = L'\0';

    if (out_str[0] == L'\0' && n == 0) { // if original n was 0
         out_str[0] = L'0';
         out_str[1] = L'\0';
    } else if (out_str[0] == L'\0') { // If number was non-zero but resulted in empty string (e.g. due to buffer logic)
        // This indicates an issue with the loop or buffer indexing,
        // for safety, put "Error" or a known pattern.
        // For now, let's assume the logic above handles it, but this is a safeguard.
        const CHAR16* err_str = EFI_TEXT("CONV_ERR");
        int l = 0;
        for(; err_str[l] != L'\0'; ++l) out_str[l] = err_str[l];
        out_str[l] = L'\0';
    }
}

// Helper to print a string followed by a newline
void print_line(const CHAR16* str) {
    if (gSystemTable && gSystemTable->ConOut) {
        gSystemTable->ConOut->OutputString(gSystemTable->ConOut, (CHAR16*)str);
        gSystemTable->ConOut->OutputString(gSystemTable->ConOut, EFI_TEXT("\r\n"));
    }
}

#if defined(__clang__)
void print_line(const wchar_t* str) {
    print_line((CHAR16*)(str));
}
#endif
// Helper to print a string followed by a hex value and newline
void print_value_hex(const CHAR16* prefix, unsigned long long value) {
    if (gSystemTable && gSystemTable->ConOut) {
        CHAR16 buffer[256];
        CHAR16 value_str[20]; // For hex representation of u64

        ull_to_hex_str(value, value_str);

        // Simple concatenation
        int i = 0;
        while (prefix[i] != L'\0' && i < 200) { // 200 to leave space for value and terminator
            buffer[i] = prefix[i];
            i++;
        }
        buffer[i++] = L' '; // Add a space
        
        int j = 0;
        while (value_str[j] != L'\0' && i < 254) { // 254 to leave space for terminator
            buffer[i] = value_str[j];
            i++; j++;
        }
        buffer[i] = L'\0';

        print_line(buffer);
    }
}

#if defined(__clang__)
void print_value_hex(const wchar_t* prefix, unsigned long long value) {
    print_value_hex((CHAR16*)(prefix), value);\
}
#endif

static void calibrate_tsc_frequency_minimal() {
    if (!gSystemTable || !gSystemTable->BootServices) {
        print_line(EFI_TEXT("Cannot calibrate TSC: SystemTable or BootServices not available."));
        g_tsc_frequency = 0; // Ensure frequency is 0 if calibration fails
        return;
    }

    print_line(EFI_TEXT("Performing minimal TSC calibration..."));
    unsigned long long start_ticks = __rdtsc();
    gSystemTable->BootServices->Stall(100000); // Stall for 100ms (100,000 microseconds)
    unsigned long long end_ticks = __rdtsc();
    unsigned long long ticks_difference = end_ticks - start_ticks;
    
    if (ticks_difference == 0) { // Avoid division by zero if stall is too short or TSC is not advancing
         print_line(EFI_TEXT("TSC calibration failed: No tick difference."));
         g_tsc_frequency = 0;
    } else {
        g_tsc_frequency = ticks_difference * 10; // Multiply by 10 because stall was for 1/10th of a second
        print_value_hex(EFI_TEXT("Approx. TSC Freq (Hz):"), g_tsc_frequency);
    }
    print_line(EFI_TEXT("Minimal TSC calibration done."));
}

static void run_assignment_benchmark(uint32_t iterations) {
    print_line(L"--- Starting Assignment Benchmark ---");

    volatile m256i source(0,0,0,0);
    volatile m256i dest;
    uint64_t accumulator = 0; // Not volatile, rely on printing.

    if (iterations == 0) {
        print_line(L"No iterations performed.");
        print_line(L"--- Assignment Benchmark Complete ---");
        print_line(L"");
        return;
    }

    unsigned long long start_tsc = __rdtsc();
    for (uint32_t i = 0; i < iterations; ++i) {
        source.u32._0 = (uint32_t)i; // Vary the source
        dest = source;
        accumulator += dest.u32._0; // Use the result
    }
    unsigned long long end_tsc = __rdtsc();

    unsigned long long total_cycles = end_tsc - start_tsc;
    unsigned long long avg_cycles_per_iteration = total_cycles / iterations;

    print_value_hex(L"Total cycles (Assignment):", total_cycles);
    print_value_hex(L"Avg cycles per iteration (Assignment):", avg_cycles_per_iteration);
    print_value_hex(L"Accumulator (Assignment):", accumulator);

    print_line(L"--- Assignment Benchmark Complete ---");
    print_line(L""); // Blank line for spacing
}

static void run_comparison_benchmark(uint32_t iterations) {
    print_line(L"--- Starting Comparison Benchmark ---");

    m256i val1;
    m256i val2;
    // Define fixed_different_val using CHAR16 for string literals if it were for print_line,
    // but it's for m256i constructor which takes ULLs.
    const m256i fixed_different_val(0xAAAAAAAAAAAAAAAAULL, 0xBBBBBBBBBBBBBBBBULL, 0xCCCCCCCCCCCCCCCCULL, 0xDDDDDDDDDDDDDDDDULL); // A distinct pattern
    uint64_t accumulator = 0; // Counts true comparisons

    if (iterations == 0) {
        print_line(L"No iterations performed.");
        print_line(L"--- Comparison Benchmark Complete ---");
        print_line(L"");
        return;
    }

    unsigned long long start_tsc = __rdtsc();
    for (uint32_t i = 0; i < iterations; ++i) {
        // Vary val1 based on i - let's make all its parts depend on i
        val1 = m256i(i, i + 1, i + 2, i + 3); // Constructor takes ULLs

        if ((i % 4) == 0) {
            val2 = val1; // Comparison will be true
        } else {
            val2 = fixed_different_val; // Comparison will be false typically
        }
        
        bool comparison_result = (val1 == val2);
        accumulator += comparison_result; 
    }
    unsigned long long end_tsc = __rdtsc();

    unsigned long long total_cycles = end_tsc - start_tsc;
    unsigned long long avg_cycles_per_iteration = total_cycles / iterations;

    print_value_hex(L"Total cycles (Comparison):", total_cycles);
    print_value_hex(L"Avg cycles per iteration (Comparison):", avg_cycles_per_iteration);
    print_value_hex(L"Accumulator (True count):", accumulator);

    print_line(L"--- Comparison Benchmark Complete ---");
    print_line(L""); // Blank line for spacing
}

static void run_setrandom_benchmark(uint32_t iterations) {
    print_line(L"--- Starting SetRandom Benchmark ---");

    m256i val(0,0,0,0);
    uint64_t accumulator = 0;

    if (iterations == 0) {
        print_line(L"No iterations performed.");
        print_line(L"--- SetRandom Benchmark Complete ---");
        print_line(L"");
        return;
    }

    unsigned long long start_tsc = __rdtsc();
    for (uint32_t i = 0; i < iterations; ++i) {
        val.setRandomValue();
        accumulator += val.m256i_u64[0]; // Use one part of the random value
    }
    unsigned long long end_tsc = __rdtsc();

    unsigned long long total_cycles = end_tsc - start_tsc;
    unsigned long long avg_cycles_per_iteration = total_cycles / iterations;

    print_value_hex(L"Total cycles (SetRandom):", total_cycles);
    print_value_hex(L"Avg cycles per iteration (SetRandom):", avg_cycles_per_iteration);
    print_value_hex(L"Accumulator (SetRandom):", accumulator); // Value will be somewhat random

    print_line(L"--- SetRandom Benchmark Complete ---");
    print_line(L""); // Blank line for spacing
}

static void run_rdrnd_benchmark(uint32_t iterations) {
    print_line(L"--- Starting RdRnd Benchmark ---");

    uint64_t accumulator = 0;

    uint64_t rndValue = 0;
    unsigned long long start_tsc = __rdtsc();
    for (uint32_t i = 0; i < iterations; ++i) {
        _rdrand64_step(&rndValue);
        accumulator += rndValue; // Use one part of the random value
    }
    unsigned long long end_tsc = __rdtsc();

    unsigned long long total_cycles = end_tsc - start_tsc;
    unsigned long long avg_cycles_per_iteration = total_cycles / iterations;

    print_value_hex(L"Total cycles (SetRandom):", total_cycles);
    print_value_hex(L"Avg cycles per iteration (SetRandom):", avg_cycles_per_iteration);
    print_value_hex(L"Accumulator (SetRandom):", accumulator); // Value will be somewhat random

    print_line(L"--- RdRnd Benchmark Complete ---");
    print_line(L""); // Blank line for spacing
}

static void run_zero_benchmark(uint32_t iterations) {
    print_line(L"--- Starting Zero Benchmark ---");

    m256i val(1,2,3,4); // Initialize to non-zero to ensure assignment happens
    uint64_t accumulator = 0;

    if (iterations == 0) {
        print_line(L"No iterations performed.");
        print_line(L"--- Zero Benchmark Complete ---");
        print_line(L"");
        return;
    }

    unsigned long long start_tsc = __rdtsc();
    for (uint32_t i = 0; i < iterations; ++i) {
        val = m256i::zero();
        accumulator += val.u32._0; // Use one part of the zeroed value
    }
    unsigned long long end_tsc = __rdtsc();

    unsigned long long total_cycles = end_tsc - start_tsc;
    unsigned long long avg_cycles_per_iteration = total_cycles / iterations;

    print_value_hex(L"Total cycles (Zero):", total_cycles);
    print_value_hex(L"Avg cycles per iteration (Zero):", avg_cycles_per_iteration);
    print_value_hex(L"Accumulator (Zero):", accumulator); // Will be 0

    print_line(L"--- Zero Benchmark Complete ---");
    print_line(L""); // Blank line for spacing
}

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    // Suppress unused parameter warnings
    (void)ImageHandle;

    // Store SystemTable globally
    gSystemTable = SystemTable;

    // Basic check for SystemTable and ConOut
    if (!gSystemTable || !gSystemTable->ConOut) {
        return EFI_UNSUPPORTED;
    }

    // Clear the screen
    gSystemTable->ConOut->ClearScreen(gSystemTable->ConOut);

    enableAVX();
    print_line(L"UEFI m256i Benchmark Application");
    print_line(L"=================================");

    const unsigned long long iterations = 100000; // 100k iterations
    calibrate_tsc_frequency_minimal();
    print_line(L"Starting m256i benchmark...");

    print_value_hex(L"Iterations per benchmark:", iterations); // Changed "Iterations:" to "Iterations per benchmark:"
    print_line(L""); // Add a blank line for spacing

    run_assignment_benchmark(iterations);
    run_comparison_benchmark(iterations);
    run_setrandom_benchmark(iterations);
    run_zero_benchmark(iterations);
    run_rdrnd_benchmark(iterations);

    print_line(L"=================================");
    print_line(L"Benchmark complete. System will halt in a moment or press ESC to exit.");

    // Wait for a key press (optional, good for seeing output)
    EFI_INPUT_KEY Key;
    SystemTable->ConIn->Reset(SystemTable->ConIn, FALSE);
    while (SystemTable->ConIn->ReadKeyStroke(SystemTable->ConIn, &Key) == EFI_NOT_READY);

    return EFI_SUCCESS;
}
