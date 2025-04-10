cmake_minimum_required(VERSION 3.14)
project(qubic_core_tests CXX C)

# GoogleTest requires at least C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)


message(STATUS "Compiler ID: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Compiler Path: ${CMAKE_CXX_COMPILER}")


# Allow the user to enable AVX-512
option(ENABLE_AVX512 "Enable AVX-512 instructions" ON)

if (ENABLE_AVX512)
    add_compile_options(-mavx512f -mavx512cd -mavx512vl -mavx512bw -mavx512dq)
else()
    message(STATUS "AVX-512 is disabled. If you would like to activate make sure you set ENABLE_AVX512 to ON while running cmake.")
endif()

add_compile_options(-mavx -mavx2)

add_compile_options(
    -fshort-wchar # Use 16bit chars
    -mrdrnd #random intrinsics
    -Wcast-align
    -fsanitize=alignment
    -fno-sanitize-recover=alignment
)

add_link_options(
  -fsanitize=alignment
)

enable_testing()

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
  qubic_core_tests
  GTest::gtest_main
  platform_common
  platform_os
  platform_efi #Currently still needed due to various imports
)

include(GoogleTest)
gtest_discover_tests(qubic_core_tests)
