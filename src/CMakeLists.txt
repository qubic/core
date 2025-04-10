cmake_minimum_required(VERSION 3.14)
project(qubic_core CXX)

# GoogleTest requires at least C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


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
    -ffreestanding # No std
    -mno-red-zone
    -fshort-wchar # Use 16bit chars
    # -Xclang -fno-wchar
    -mrdrnd #random intrinsics
    -target x86_64-unknown-windows
    --sysroot="${CLANG_SYSROOT}"
    -fno-stack-protector
    -fno-strict-aliasing
    -fno-builtin
    -fno-rtti
    -fno-exceptions
    -Wall
    -Wextra
    -Werror
    -Wno-unused-parameter
)


include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(
  Qubic
  qubic.cpp
    )


set(CMAKE_LINKER "lld")
set(CMAKE_CXX_LINK_EXECUTABLE
    "${CMAKE_LINKER} -flavor link \
    -subsystem:efi_application \
    -entry:efi_main \
    <OBJECTS> \
    -out:<TARGET>"
)

# Set output suffix to .efi
set_target_properties(Qubic PROPERTIES
    SUFFIX ".efi"
)

