cmake_minimum_required(VERSION 3.15)
project(qubic_core CXX ASM_NASM)

# GoogleTest requires at least C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable FetchContent module
include(FetchContent)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/lib/)

set(ASM_FILE "${CMAKE_CURRENT_SOURCE_DIR}/platform/custom_stack.nasm")
set(ASM_OBJ "${CMAKE_CURRENT_BINARY_DIR}/custom_stack.obj")

add_custom_command(
    OUTPUT ${ASM_OBJ}
    COMMAND ${NASM_EXECUTABLE} -f elf64 ${ASM_FILE} -o ${ASM_OBJ}
    DEPENDS ${ASM_FILE}
    COMMENT "Assembling ${ASM_FILE}"
)

# Extra libraries
FetchContent_Declare(
        drogon
        GIT_REPOSITORY https://github.com/drogonframework/drogon.git
        GIT_TAG v1.9.11
)
FetchContent_MakeAvailable(drogon)

# Define the shared library (matches Visual Studio DynamicLibrary configuration)
add_executable(
  Qubic
  qubic.cpp
  ${ASM_OBJ}
)

if(TESTNET)
    target_compile_definitions(Qubic PRIVATE TESTNET)
endif()

if(ONLY_LOGGING)
    target_compile_definitions(Qubic PRIVATE ONLY_LOGGING)
endif()

if(CMAKE_NO_USE_SWAP)
    target_compile_definitions(Qubic PRIVATE CMAKE_NO_USE_SWAP)
endif()

if(DEFINED ENABLE_QUBIC_LOGGING_EVENT)
    target_compile_definitions(Qubic PRIVATE ENABLE_QUBIC_LOGGING_EVENT=${ENABLE_QUBIC_LOGGING_EVENT})
endif()

# Set specific properties for assembly files to avoid C++ flags being applied
set_source_files_properties(platform/custom_stack.nasm PROPERTIES
    COMPILE_FLAGS ""
    LANGUAGE ASM_MASM
)

# Add additional compiler-specific flags for the main application
if(IS_CLANG)
    # Clang-specific flags
    target_compile_options(Qubic PRIVATE
        -D_CONSOLE
        $<$<CONFIG:Release>:-DNDEBUG>
        -mrdrnd
        -mbmi
        -mlzcnt
        -fshort-wchar
        -w
    )
elseif(IS_MSVC)
    # MSVC-specific flags
    target_compile_options(Qubic PRIVATE
       /D_CONSOLE
       $<$<CONFIG:Release>:/DNDEBUG>
    )
endif()

find_package(Threads REQUIRED)
# Link with platform libraries
# if(BUILD_TESTS)
#   # When building for tests, link only with platform_common and platform_os
#   target_link_libraries(Qubic
#     platform_common
#     platform_os
#   )
# else()
  # Apply common compiler flags from the centralized detection module
  apply_common_compiler_flags(Qubic)
  # When building the application, link with platform_common and platform_efi
  target_link_libraries(Qubic
    platform_common
    platform_efi
    c++ c++abi
    Threads::Threads drogon
  )
# endif()

# Configure linker settings based on compiler
if(IS_MSVC)
    # MSVC-specific linker settings
    set_target_properties(Qubic PROPERTIES SUFFIX ".efi")
    
elseif(IS_CLANG)
    # Use default linker settings for Clang
endif()

