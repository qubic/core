cmake_minimum_required(VERSION 3.10)

project(M256UefiBenchmark CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(m256_uefi_benchmark benchmark_main.cpp)

apply_efi_compiler_flags(m256_uefi_benchmark)


include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/lib)
include_directories(${CMAKE_SOURCE_DIR}/src)

# Add additional compiler-specific flags for the main application
if(IS_CLANG)
    # Clang-specific flags
    target_compile_options(m256_uefi_benchmark PRIVATE
        -mrdrnd
        -target x86_64-unknown-windows
        $<$<BOOL:${CLANG_SYSROOT}>:--sysroot="${CLANG_SYSROOT}">
        -Werror
        -Wno-unused-parameter
    )
elseif(IS_MSVC)
    # MSVC-specific flags
    target_compile_options(m256_uefi_benchmark PRIVATE)
endif()

# Link with platform libraries
  # When building the application, link with platform_common and platform_efi
target_link_libraries(m256_uefi_benchmark
    platform_common
    platform_efi
)

# Configure linker settings based on compiler
if(IS_MSVC)
    # MSVC-specific linker settings
    set_target_properties(m256_uefi_benchmark PROPERTIES SUFFIX ".efi")
    
elseif(IS_CLANG)
    # Clang-specific linker settings
    set(CMAKE_LINKER "lld")
    set(CMAKE_CXX_LINK_EXECUTABLE
        "${CMAKE_LINKER} -flavor link \
        -subsystem:efi_application \
        -entry:efi_main \
        <OBJECTS> \
        -out:<TARGET>"
    )
    
    # Set output suffix to .efi
    set_target_properties(m256_uefi_benchmark PROPERTIES
        SUFFIX ".efi"
    )
endif()
