cmake_minimum_required(VERSION 3.14)
project(qubic_core CXX)

# GoogleTest requires at least C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/lib)

add_executable(
  Qubic
  qubic.cpp
)

# Apply EFI-specific compiler flags from the centralized detection module
apply_efi_compiler_flags(Qubic)

# Add additional EFI-specific flags for the main application
if(IS_CLANG)
    # These flags are specific to the main application and not needed for libraries
    target_compile_options(Qubic PRIVATE
        -mrdrnd
        -target x86_64-unknown-windows
        $<$<BOOL:${CLANG_SYSROOT}>:--sysroot="${CLANG_SYSROOT}">
        -Werror
        -Wno-unused-parameter
    )
endif()

# Link with platform libraries
if(BUILD_TESTS)
  # When building for tests, link only with platform_common and platform_os
  target_link_libraries(Qubic
    platform_common
    platform_os
  )
else()
  # When building the application, link with platform_common and platform_efi
  target_link_libraries(Qubic
    platform_common
    platform_efi
  )
endif()


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

