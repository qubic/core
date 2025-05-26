cmake_minimum_required(VERSION 3.15)
project(qubic_core CXX)

# GoogleTest requires at least C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/lib)

# Define the executable
add_executable(
  Qubic
  qubic.cpp
)

# Apply EFI-specific compiler flags from the centralized detection module
apply_efi_compiler_flags(Qubic)

# Add additional compiler-specific flags for the main application
if(IS_CLANG)
    # Clang-specific flags
    target_compile_options(Qubic PRIVATE
        -mrdrnd
        -target x86_64-unknown-windows
        $<$<BOOL:${CLANG_SYSROOT}>:--sysroot="${CLANG_SYSROOT}">
        -Werror
        -Wno-unused-parameter
    )
elseif(IS_MSVC)
    # MSVC-specific flags
    target_compile_options(Qubic PRIVATE
        /Gs1638400  # Control stack checking
        /W3         # Warning level
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

# Configure linker settings based on compiler
if(IS_MSVC)
    # MSVC-specific linker settings
    set_target_properties(Qubic PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:EFI_APPLICATION /ENTRY:efi_main /IGNORE:4086 /IGNORE:4108 /NODEFAULTLIB"
        SUFFIX ".efi"
    )
    
    # Set stack size for MSVC
    set_property(TARGET Qubic APPEND_STRING PROPERTY LINK_FLAGS " /STACK:131072")
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
    set_target_properties(Qubic PROPERTIES
        SUFFIX ".efi"
    )
endif()

