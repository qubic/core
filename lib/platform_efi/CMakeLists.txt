cmake_minimum_required(VERSION 3.14)
project(platform_efi CXX C)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# --- Include Directories (Common) ---
# Get the project root directory by going up two levels from the current directory
get_filename_component(PROJECT_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
    ${PROJECT_ROOT_DIR}
)

# Source files
set(SOURCES
    edk2_debug.c
    processor.cpp
    sleep.cpp
    uefi_globals.cpp
)

# Create static library
add_library(platform_efi STATIC ${SOURCES})

# Apply EFI-specific compiler flags from the centralized detection module
apply_efi_compiler_flags(platform_efi)

# Set library properties
set_target_properties(platform_efi PROPERTIES
    LINKER_LANGUAGE CXX
)

# Install
install(TARGETS platform_efi
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

# Install headers
install(FILES
    uefi.h
    uefi_globals.h
    DESTINATION include/platform_efi
)
