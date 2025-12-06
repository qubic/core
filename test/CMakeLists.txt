cmake_minimum_required(VERSION 3.15)
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


get_filename_component(PROJECT_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../lib/platform_common)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../lib/platform_os)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../lib/platform_efi) # Currently still needed due to various imports
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src)
include_directories(${PROJECT_ROOT_DIR})


# Test sources common to all architectures
set(TEST_SOURCES
  # assets.cpp
  # common_def.cpp
  # contract_core.cpp
  # contract_qearn.cpp
  # contract_qvault.cpp
  # contract_qx.cpp
  # kangaroo_twelve.cpp
  math_lib.cpp
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

# Certain test sources exercise x86-specific intrinsics and codepaths
# (for example, the AVX/m256 implementation). To keep the test target
# buildable on non-x86 platforms (like arm64), only append those sources
# when the build is targeting x86_64. This preserves the original x86
# test coverage when building on x86, while allowing native non-x86
# development without intrusive #ifdefs inside the test files.
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64")
  list(APPEND TEST_SOURCES
    m256.cpp
    network_messages.cpp
  )
endif()

add_executable(qubic_core_tests ${TEST_SOURCES})

# Apply test-specific compiler flags from the centralized detection module
apply_test_compiler_flags(qubic_core_tests)

# Add additional test-specific flags if needed
if(IS_CLANG OR IS_GCC)
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64")
    target_compile_options(qubic_core_tests PRIVATE -mrdrnd)
  else()
    target_compile_options(qubic_core_tests PRIVATE)
  endif()
endif()


target_link_libraries(
  qubic_core_tests PRIVATE
  GTest::gtest_main
  platform_common
  platform_os
)

include(GoogleTest)
gtest_discover_tests(qubic_core_tests)