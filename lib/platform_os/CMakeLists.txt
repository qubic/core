cmake_minimum_required(VERSION 3.14)
project(platform_os CXX C)

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
    sleep.cpp
)

# Create static library
add_library(platform_os STATIC ${SOURCES})

# Apply OS-specific compiler flags from the centralized detection module
apply_os_compiler_flags(platform_os)

# Set library properties
set_target_properties(platform_os PROPERTIES
    LINKER_LANGUAGE CXX
)

# Install
install(TARGETS platform_os
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)