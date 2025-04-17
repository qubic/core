cmake_minimum_required(VERSION 3.14)
project(qubic_core_tests CXX C)

# GoogleTest requires at least C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.16.0
  )
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)


message(STATUS "Compiler ID: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Compiler Path: ${CMAKE_CXX_COMPILER}")

# Compiler-Specific Flags
# MSVC specific flags
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    message(STATUS "Configuring for MSVC")
    add_compile_options(
        /W4 # Set a high warning level (equivalent to -Wall)
        /WX # Treat warnings as errors (optional, but good practice)
        /Zc:wchar_t- # Equivalent to -fshort-wchar
        # /wd<XXXX> # Disable specific warnings if needed, e.g., /wd4251
    )
    if(ENABLE_AVX512)
        add_compile_options(/arch:AVX512)
        message(STATUS "MSVC: Enabling AVX-512 (/arch:AVX512)")
    else()
        add_compile_options(/arch:AVX2) # Assuming AVX2 is the fallback/default
        message(STATUS "MSVC: Enabling AVX2 (/arch:AVX2)")
        message(STATUS "AVX-512 is disabled. If you would like to activate make sure you set ENABLE_AVX512 to ON while running cmake.")
    endif()
    # MSVC does not have direct equivalents for -Wcast-align or -fsanitize=alignment via simple flags.
    # It has runtime checks (/RTC) and AddressSanitizer (/fsanitize=address).

# GCC/Clang specific flags
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(STATUS "Configuring for GCC/Clang")
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Werror # Treat warnings as errors (optional)
        -fshort-wchar # Use 16bit chars
        -mrdrnd # Random intrinsics flag (often covered by headers too)
        -Wcast-align # *** This is the flag causing the original error on MSVC ***
        -fsanitize=alignment
        -fno-sanitize-recover=alignment
    )
    add_link_options(
        -fsanitize=alignment
    )

    if (ENABLE_AVX512)
        add_compile_options(-mavx512f -mavx512cd -mavx512vl -mavx512bw -mavx512dq)
        # AVX/AVX2 flags are usually implied by AVX512, but adding them might be needed depending on the exact code/compiler version
        add_compile_options(-mavx -mavx2)
        message(STATUS "GCC/Clang: Enabling AVX-512 and AVX/AVX2")
    else()
        add_compile_options(-mavx -mavx2) # Enable AVX/AVX2 if AVX512 is off
        message(STATUS "GCC/Clang: Enabling AVX/AVX2")
        message(STATUS "AVX-512 is disabled. If you would like to activate make sure you set ENABLE_AVX512 to ON while running cmake.")
    endif()

else()
    message(WARNING "Compiler ${CMAKE_CXX_COMPILER_ID} not recognized. Specific optimization/warning flags may not be set.")
endif()



get_filename_component(PROJECT_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../lib/platform_common)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../lib/platform_os)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../lib/platform_efi) # Currently still needed due to various imports
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src)
include_directories(${PROJECT_ROOT_DIR})


add_executable(
  qubic_core_tests
  # assets.cpp
  # common_def.cpp
  # contract_core.cpp
  # contract_qearn.cpp
  # contract_qvault.cpp
  # contract_qx.cpp
  # kangaroo_twelve.cpp
  # m256.cpp
  math_lib.cpp
  # network_messages.cpp
  # platform.cpp
  # qpi_collection.cpp
  # qpi.cpp
  # qpi_hash_map.cpp
  # score_cache.cpp
  # score.cpp
  # spectrum.cpp
  # stdlib_impl.cpp
  # tick_storage.cpp
  # tx_status_request.cpp
  # vote_counter.cpp
)


target_link_libraries(
  qubic_core_tests PRIVATE
  GTest::gtest_main
  platform_common
  platform_os
  platform_efi #Currently still needed due to various imports
)

include(GoogleTest)
gtest_discover_tests(qubic_core_tests)