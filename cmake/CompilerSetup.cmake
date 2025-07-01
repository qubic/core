# CompilerDetection.cmake
# Central location for compiler and system detection logic

# --- Platform and Compiler Detection ---
message(STATUS "Detecting compiler and platform...")
message(STATUS "Compiler ID: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Compiler Path: ${CMAKE_CXX_COMPILER}")
message(STATUS "System Name: ${CMAKE_SYSTEM_NAME}")

# Set platform detection variables
set(IS_WINDOWS FALSE CACHE INTERNAL "Windows platform detected")
set(IS_LINUX FALSE CACHE INTERNAL "Linux platform detected")
set(IS_MSVC FALSE CACHE INTERNAL "MSVC compiler detected")
set(IS_CLANG FALSE CACHE INTERNAL "Clang compiler detected")
set(IS_GCC FALSE CACHE INTERNAL "GCC compiler detected")
set(ASM_LANG "" CACHE INTERNAL "Assembly language to use")

# Detect Windows platform
if(CMAKE_SYSTEM_NAME MATCHES "Windows")
    set(IS_WINDOWS TRUE CACHE INTERNAL "Windows platform detected" FORCE)
    message(STATUS "Windows platform detected")
endif()

# Detect Linux platform
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(IS_LINUX TRUE CACHE INTERNAL "Linux platform detected" FORCE)
    message(STATUS "Linux platform detected")
endif()

# Detect MSVC compiler
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(IS_MSVC TRUE CACHE INTERNAL "MSVC compiler detected" FORCE)
    message(STATUS "MSVC compiler detected")
    
    # Set assembly language for MSVC
    enable_language(ASM_MASM)
    set(ASM_LANG ASM_MASM CACHE INTERNAL "Assembly language to use" FORCE)
    message(STATUS "Using MASM for assembly")
endif()

# Detect Clang compiler
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(IS_CLANG TRUE CACHE INTERNAL "Clang compiler detected" FORCE)
    message(STATUS "Clang compiler detected")
    
    # For Clang on Linux, we'll use NASM
    if(IS_LINUX)
        set(ASM_LANG ASM_NASM CACHE INTERNAL "Assembly language to use" FORCE)
        find_program(NASM_EXECUTABLE nasm REQUIRED)
        message(STATUS "Using NASM for assembly via custom command")
    endif()
endif()

# Detect GCC compiler
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(IS_GCC TRUE CACHE INTERNAL "GCC compiler detected" FORCE)
    message(STATUS "GCC compiler detected")
    
    # For GCC on Linux, we'll use NASM
    if(IS_LINUX)
        set(ASM_LANG ASM_NASM CACHE INTERNAL "Assembly language to use" FORCE)
        find_program(NASM_EXECUTABLE nasm REQUIRED)
        message(STATUS "Using NASM for assembly via custom command")
    endif()
endif()

# --- Clear all default flags to use only specified ones ---
set(CMAKE_CONFIGURATION_TYPES Debug Release CACHE STRING "Available build types" FORCE)

message(STATUS "CLEARING CMAKE DEFAULT FLAGS")
# Set all default flag variables to an empty string to take full control.
set(CMAKE_C_FLAGS "" CACHE INTERNAL "")
set(CMAKE_CXX_FLAGS "" CACHE INTERNAL "")
set(CMAKE_EXE_LINKER_FLAGS "" CACHE INTERNAL "")

set(CMAKE_C_FLAGS_DEBUG "" CACHE INTERNAL "")
set(CMAKE_CXX_FLAGS_DEBUG "" CACHE INTERNAL "")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "" CACHE INTERNAL "")

set(CMAKE_C_FLAGS_RELEASE "" CACHE INTERNAL "")
set(CMAKE_CXX_FLAGS_RELEASE "" CACHE INTERNAL "")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "" CACHE INTERNAL "")

# set(CMAKE_C_FLAGS_RELWITHDEBINFO "" CACHE INTERNAL "")
# set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "" CACHE INTERNAL "")
# set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "" CACHE INTERNAL "")

# set(CMAKE_C_FLAGS_MINSIZEREL "" CACHE INTERNAL "")
# set(CMAKE_CXX_FLAGS_MINSIZEREL "" CACHE INTERNAL "")
# set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "" CACHE INTERNAL "")

# --- Common Compiler Flags ---

# Define common flags for all compilers
set(COMMON_C_FLAGS "" CACHE INTERNAL "Common C compiler flags")
set(COMMON_CXX_FLAGS "" CACHE INTERNAL "Common C++ compiler flags")
set(COMMON_DEBUG_FLAGS "" CACHE INTERNAL "Common debug compiler flags")
set(COMMON_RELEASE_FLAGS "" CACHE INTERNAL "Common release compiler flags")
set(COMMON_LINK_FLAGS "" CACHE INTERNAL "Common linker flags")

# Define EFI-specific flags
set(EFI_C_FLAGS "" CACHE INTERNAL "EFI-specific C compiler flags")
set(EFI_CXX_FLAGS "" CACHE INTERNAL "EFI-specific C++ compiler flags")

# Define OS-specific flags
set(OS_C_FLAGS "" CACHE INTERNAL "OS-specific C compiler flags")
set(OS_CXX_FLAGS "" CACHE INTERNAL "OS-specific C++ compiler flags")

# Define platform-specific flags
if(IS_MSVC)
    # MSVC-specific common flags
    set(COMMON_C_FLAGS "/W4 /GF" CACHE INTERNAL "Common C compiler flags" FORCE)
    set(COMMON_CXX_FLAGS "${COMMON_C_FLAGS}" CACHE INTERNAL "Common C++ compiler flags" FORCE)
    set(COMMON_DEBUG_FLAGS "" CACHE INTERNAL "Common debug compiler flags" FORCE)
    set(COMMON_RELEASE_FLAGS "/GL- /Gw /Oi /Ob2 /O2 /Oy" CACHE INTERNAL "Common release compiler flags" FORCE)
    set(COMMON_LINK_FLAGS "" CACHE INTERNAL "Common linker flags" FORCE)
    
    # MSVC-specific EFI flags
    # /W3                      # WarningLevel: Level3
    # /permissive-             # ConformanceMode: true
    # /Zc:wchar_t-             # TreatWChar_tAsBuiltInType: false
    # /EHs-c-                  # ExceptionHandling: false
    # /GS-                     # BufferSecurityCheck: false / SDLCheck: false
    # /GF                      # StringPooling: true
    # /Gy-                     # FunctionLevelLinking: false
    # /Oi                      # IntrinsicFunctions: true
    # /O2                      # Optimization: MaxSpeed
    # /Ot                      # FavorSizeOrSpeed: Speed
    # /Ob2                     # InlineFunctionExpansion: AnySuitable
    # /Oy                      # OmitFramePointers: true
    # /GT                      # EnableFiberSafeOptimizations: true
    # /fp:except-              # FloatingPointExceptions: false
    # /guard:cf-               # ControlFlowGuard: false
    # /Gs1638400               # AdditionalOptions: /Gs...
    # /Zl                      # OmitDefaultLibName: true
    # /FAcs                    # AssemblerOutput: All (DISABLED)
    # /Fa${CMAKE_INTDIR}/Qubic.asm # AssemblerListingLocation (DISABLED)
    set(EFI_C_FLAGS "/W3 /permissive- /Zc:wchar_t- /EHs-c- /GS- /GF /Gy- /Oi /O2 /Ot /Ob2 /Oy /GT /fp:except- /guard:cf- /Gs1638400 /Zl" CACHE INTERNAL "EFI-specific C compiler flags" FORCE)
    set(EFI_CXX_FLAGS "${EFI_C_FLAGS}" CACHE INTERNAL "EFI-specific C++ compiler flags" FORCE)

    # /SUBSYSTEM:EFI_APPLICATION # SubSystem: EFI Application
    # /ENTRY:efi_main            # EntryPointSymbol: efi_main
    # /NODEFAULTLIB              # IgnoreAllDefaultLibraries: true
    # /DEBUG:NONE                # GenerateDebugInformation: false
    # /OPT:REF                   # EnableCOMDATFolding: true
    # /OPT:ICF                   # OptimizeReferences: true
    # /DYNAMICBASE:NO            # RandomizedBaseAddress: false
    # /NXCOMPAT:NO               # DataExecutionPrevention: false
    # /MANIFEST:NO               # GenerateManifest: false
    # /STACK:131072              # StackReserveSize / StackCommitSize
    set(EFI_LINK_FLAGS "/SUBSYSTEM:EFI_APPLICATION /ENTRY:efi_main /NODEFAULTLIB /DEBUG:NONE /OPT:REF /OPT:ICF /MANIFEST:NO /DYNAMICBASE:NO /NXCOMPAT:NO /STACK:131072" CACHE INTERNAL "EFI-specific linker flags" FORCE)
    
    # MSVC-specific OS flags
    set(OS_C_FLAGS "" CACHE INTERNAL "OS-specific C compiler flags" FORCE)
    set(OS_CXX_FLAGS "" CACHE INTERNAL "OS-specific C++ compiler flags" FORCE)
    
elseif(IS_CLANG OR IS_GCC)
    # Clang/GCC-specific common flags
    set(COMMON_C_FLAGS "-Wall -Wextra -fshort-wchar" CACHE INTERNAL "Common C compiler flags" FORCE)
    set(COMMON_CXX_FLAGS "${COMMON_C_FLAGS}" CACHE INTERNAL "Common C++ compiler flags" FORCE)
    set(COMMON_DEBUG_FLAGS "-g" CACHE INTERNAL "Common debug compiler flags" FORCE)
    set(COMMON_RELEASE_FLAGS "-O2 -fomit-frame-pointer -fno-lto" CACHE INTERNAL "Common release compiler flags" FORCE)
    set(COMMON_LINK_FLAGS "" CACHE INTERNAL "Common linker flags" FORCE)
    
    # Clang/GCC-specific EFI flags
    set(EFI_C_FLAGS "-ffreestanding -mno-red-zone -fno-stack-protector -fno-strict-aliasing -fno-builtin" CACHE INTERNAL "EFI-specific C compiler flags" FORCE)
    set(EFI_CXX_FLAGS "${EFI_C_FLAGS} -fno-rtti -fno-exceptions" CACHE INTERNAL "EFI-specific C++ compiler flags" FORCE)
    
    # Clang/GCC-specific OS flags
    set(OS_C_FLAGS "-fno-stack-protector" CACHE INTERNAL "OS-specific C compiler flags" FORCE)
    set(OS_CXX_FLAGS "${OS_C_FLAGS}" CACHE INTERNAL "OS-specific C++ compiler flags" FORCE)
endif()

# --- CPU Instruction Set Flags ---

# Allow the user to enable AVX-512
option(ENABLE_AVX512 "Enable AVX-512 instructions" ON)

# Define CPU instruction set flags
set(CPU_INSTRUCTION_FLAGS "" CACHE INTERNAL "CPU instruction set flags")

if(IS_MSVC)
    if(ENABLE_AVX512)
        set(CPU_INSTRUCTION_FLAGS "/arch:AVX512" CACHE INTERNAL "CPU instruction set flags" FORCE)
        message(STATUS "MSVC: Enabling AVX-512 (/arch:AVX512)")
    else()
        set(CPU_INSTRUCTION_FLAGS "/arch:AVX2" CACHE INTERNAL "CPU instruction set flags" FORCE)
        message(STATUS "MSVC: Enabling AVX2 (/arch:AVX2)")
        message(STATUS "AVX-512 is disabled. If you would like to activate make sure you set ENABLE_AVX512 to ON while running cmake.")
    endif()
elseif(IS_CLANG OR IS_GCC)
    if(ENABLE_AVX512)
        set(CPU_INSTRUCTION_FLAGS "-mavx -mavx2 -mavx512f -mavx512cd -mavx512vl -mavx512bw -mavx512dq" CACHE INTERNAL "CPU instruction set flags" FORCE)
        message(STATUS "GCC/Clang: Enabling AVX-512 and AVX/AVX2")
    else()
        set(CPU_INSTRUCTION_FLAGS "-mavx -mavx2" CACHE INTERNAL "CPU instruction set flags" FORCE)
        message(STATUS "GCC/Clang: Enabling AVX/AVX2")
        message(STATUS "AVX-512 is disabled. If you would like to activate make sure you set ENABLE_AVX512 to ON while running cmake.")
    endif()
endif()

# --- Test-specific flags ---
set(TEST_SPECIFIC_FLAGS "" CACHE INTERNAL "Test-specific compiler flags")

if(IS_MSVC)
    set(TEST_SPECIFIC_FLAGS "/WX /EHsc" CACHE INTERNAL "Test-specific compiler flags" FORCE)
elseif(IS_CLANG OR IS_GCC)
    if(USE_SANITIZER)
        set(TEST_SPECIFIC_FLAGS "-Wpedantic -Werror -mrdrnd -Wcast-align -fsanitize=alignment -fno-sanitize-recover=alignment" CACHE INTERNAL "Test-specific compiler flags" FORCE)
        set(TEST_SPECIFIC_LINK_FLAGS "-fsanitize=alignment" CACHE INTERNAL "Test-specific linker flags" FORCE)
    else()
        set(TEST_SPECIFIC_FLAGS "-Wpedantic -Werror -mrdrnd -Wcast-align " CACHE INTERNAL "Test-specific compiler flags" FORCE)
        set(TEST_SPECIFIC_LINK_FLAGS "" CACHE INTERNAL "Test-specific linker flags" FORCE)
    endif()
endif()

# --- EFI-specific flags ---
set(EFI_SPECIFIC_FLAGS "" CACHE INTERNAL "EFI-specific compiler flags")

if(IS_MSVC)
    set(EFI_SPECIFIC_FLAGS "" CACHE INTERNAL "EFI-specific compiler flags" FORCE)
elseif(IS_CLANG OR IS_GCC)
    set(EFI_SPECIFIC_FLAGS "-fno-rtti -fno-exceptions" CACHE INTERNAL "EFI-specific compiler flags" FORCE)
endif()

# Function to apply common compiler flags to a target
function(apply_common_compiler_flags target)
    if(IS_MSVC)
        # Convert space-separated flags to list for MSVC
        separate_arguments(C_FLAGS WINDOWS_COMMAND ${COMMON_C_FLAGS})
        separate_arguments(CXX_FLAGS WINDOWS_COMMAND ${COMMON_CXX_FLAGS})
        separate_arguments(DEBUG_FLAGS WINDOWS_COMMAND ${COMMON_DEBUG_FLAGS})
        separate_arguments(RELEASE_FLAGS WINDOWS_COMMAND ${COMMON_RELEASE_FLAGS})
        separate_arguments(CPU_FLAGS WINDOWS_COMMAND ${CPU_INSTRUCTION_FLAGS})
        
        target_compile_options(${target} PRIVATE
            ${C_FLAGS}
            $<$<COMPILE_LANGUAGE:CXX>:${CXX_FLAGS}>
            $<$<CONFIG:Debug>:${DEBUG_FLAGS}>
            $<$<CONFIG:Release>:${RELEASE_FLAGS}>
            ${CPU_FLAGS}
        )
        target_compile_definitions(${target} PRIVATE
            _LIB
            UNICODE _UNICODE
            $<$<CONFIG:Release>:NDEBUG>
        )
        message("Apply Common Flags MSVC to " ${target})
    else()
        # Convert space-separated flags to list for Clang/GCC
        separate_arguments(C_FLAGS UNIX_COMMAND ${COMMON_C_FLAGS})
        separate_arguments(CXX_FLAGS UNIX_COMMAND ${COMMON_CXX_FLAGS})
        separate_arguments(DEBUG_FLAGS UNIX_COMMAND ${COMMON_DEBUG_FLAGS})
        separate_arguments(RELEASE_FLAGS UNIX_COMMAND ${COMMON_RELEASE_FLAGS})
        separate_arguments(CPU_FLAGS UNIX_COMMAND ${CPU_INSTRUCTION_FLAGS})
        
        target_compile_options(${target} PRIVATE
            ${C_FLAGS}
            $<$<COMPILE_LANGUAGE:CXX>:${CXX_FLAGS}>
            $<$<CONFIG:Debug>:${DEBUG_FLAGS}>
            $<$<CONFIG:Release>:${RELEASE_FLAGS}>
            ${CPU_FLAGS}
        )
        target_compile_definitions(${target} PRIVATE
            _LIB
            $<$<CONFIG:Release>:NDEBUG>
        )
        message("Apply Common Flags Clang to " ${target})
    endif()
endfunction()

# Function to apply OS-specific compiler flags to a target
function(apply_os_compiler_flags target)
    apply_common_compiler_flags(${target})
    
    if(IS_MSVC)
        separate_arguments(OS_C_FLAGS_LIST WINDOWS_COMMAND ${OS_C_FLAGS})
        separate_arguments(OS_CXX_FLAGS_LIST WINDOWS_COMMAND ${OS_CXX_FLAGS})
        
        target_compile_options(${target} PRIVATE
            ${OS_C_FLAGS_LIST}
            $<$<COMPILE_LANGUAGE:CXX>:${OS_CXX_FLAGS_LIST}>
        )
        message("Apply OS Flags MSVC to " ${target})
    else()
        separate_arguments(OS_C_FLAGS_LIST UNIX_COMMAND ${OS_C_FLAGS})
        separate_arguments(OS_CXX_FLAGS_LIST UNIX_COMMAND ${OS_CXX_FLAGS})
        
        target_compile_options(${target} PRIVATE
            ${OS_C_FLAGS_LIST}
            $<$<COMPILE_LANGUAGE:CXX>:${OS_CXX_FLAGS_LIST}>
        )
        message("Apply OS Flags CLANG to " ${target})
    endif()
endfunction()

# Function to apply EFI-specific compiler flags to a target
function(apply_efi_compiler_flags target)
    apply_common_compiler_flags(${target})
    
    if(IS_MSVC)
        separate_arguments(EFI_C_FLAGS_LIST WINDOWS_COMMAND ${EFI_C_FLAGS})
        separate_arguments(EFI_CXX_FLAGS_LIST WINDOWS_COMMAND ${EFI_CXX_FLAGS})
        
        target_compile_options(${target} PRIVATE
            ${EFI_C_FLAGS_LIST}
            $<$<COMPILE_LANGUAGE:CXX>:${EFI_CXX_FLAGS_LIST}>
        )
        
        if(EFI_LINK_FLAGS)
            set_property(TARGET ${target} APPEND_STRING PROPERTY LINK_FLAGS ${EFI_LINK_FLAGS})
        endif()
        message("Apply Efi Flags MSVC to " ${target})
    else()
        separate_arguments(EFI_C_FLAGS_LIST UNIX_COMMAND ${EFI_C_FLAGS})
        separate_arguments(EFI_CXX_FLAGS_LIST UNIX_COMMAND ${EFI_CXX_FLAGS})
        
        target_compile_options(${target} PRIVATE
            ${EFI_C_FLAGS_LIST}
            $<$<COMPILE_LANGUAGE:CXX>:${EFI_CXX_FLAGS_LIST}>
        )
        message("Apply Efi Flags CLANG to " ${target})
    endif()
endfunction()

# Function to apply test-specific compiler flags to a target
function(apply_test_compiler_flags target)
    apply_os_compiler_flags(${target})
    
    if(IS_MSVC)
        separate_arguments(TEST_FLAGS WINDOWS_COMMAND ${TEST_SPECIFIC_FLAGS})
        target_compile_options(${target} PRIVATE ${TEST_FLAGS})
        message("Apply Test Flags CLANG to " ${target})
    else()
        separate_arguments(TEST_FLAGS UNIX_COMMAND ${TEST_SPECIFIC_FLAGS})
        target_compile_options(${target} PRIVATE ${TEST_FLAGS})
        
        if(TEST_SPECIFIC_LINK_FLAGS)
            separate_arguments(TEST_LINK_FLAGS UNIX_COMMAND ${TEST_SPECIFIC_LINK_FLAGS})
            target_link_options(${target} PRIVATE ${TEST_LINK_FLAGS})
        endif()
        message("Apply Test Flags CLANG to " ${target})
    endif()
endfunction()