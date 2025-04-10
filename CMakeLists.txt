cmake_minimum_required(VERSION 3.14)
project(qubic CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Allow the user to enable AVX-512
option(ENABLE_AVX512 "Enable AVX-512 instructions" ON)

# Option to build tests
option(BUILD_TESTS "Build the test suite" OFF)

# Always include platform_common as it's needed for both application and tests
add_subdirectory(lib/platform_common)

# Conditionally include other components based on whether we're building tests or the application
if(BUILD_TESTS)
    # For tests, include platform_os, src, and test
    message(STATUS "Building test configuration")
    add_subdirectory(lib/platform_os)
    add_subdirectory(lib/platform_efi) # Currenlty still needed due to various imports
    add_subdirectory(src EXCLUDE_FROM_ALL)  # Build src but don't make it a default target
    add_subdirectory(test)
else()
    # For application, include platform_efi and src
    message(STATUS "Building application configuration")
    add_subdirectory(lib/platform_efi)
    add_subdirectory(src)
endif()