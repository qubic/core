cmake_minimum_required(VERSION 3.15)

# Project definition - Enable C always
project(platform_common LANGUAGES C)

# Use the centralized compiler detection
# Note: IS_WINDOWS, IS_LINUX, IS_MSVC, IS_CLANG, IS_GCC, and ASM_LANG are already set
# by the CompilerDetection.cmake module included from the root CMakeLists.txt

# For backward compatibility with existing code
if(IS_LINUX AND (IS_CLANG OR IS_GCC))
    set(IS_LINUX_CLANG TRUE)
else()
    set(IS_LINUX_CLANG FALSE)
endif()

# --- Source File Definitions ---
set(C_SOURCES
    edk2_mdepkg/Library/BaseLib/LongJump.c
    edk2_mdepkg/Library/BaseLib/SetJump.c
)

# --- Conditional Assembly Sources & Build Steps ---
set(ASM_OBJECT_LIBS "") # Store object libraries generated from assembly

if(IS_WINDOWS AND ASM_LANG STREQUAL ASM_MASM)
    # Add MASM files directly if using MSVC
    add_library(platform_common_asm STATIC
        edk2_mdepkg/Library/BaseLib/X64/LongJump.asm
        edk2_mdepkg/Library/BaseLib/X64/SetJump.asm
    )
    list(APPEND ASM_OBJECT_LIBS platform_common_asm)

elseif(IS_LINUX_CLANG AND ASM_LANG STREQUAL ASM_NASM)
    # Use custom commands for NASM files requiring preprocessing (.nasm extension assumed)
    # --- NASM File 1: LongJump ---
    set(NASM_SRC_LJ ${CMAKE_CURRENT_SOURCE_DIR}/edk2_mdepkg/Library/BaseLib/X64/LongJump.nasm)
    set(NASM_PREPROC_LJ ${CMAKE_CURRENT_BINARY_DIR}/LongJump.nasm.preprocessed)
    set(NASM_OBJ_LJ ${CMAKE_CURRENT_BINARY_DIR}/LongJump.nasm.o)
    set(ASM_PFX_DEFINE "ASM_PFX(x)=x") # Define for NASM preprocessing

    add_custom_command(
        OUTPUT ${NASM_OBJ_LJ}
        COMMAND ${CMAKE_C_COMPILER} -E -P -x assembler-with-cpp "-D${ASM_PFX_DEFINE}" ${NASM_SRC_LJ} -o ${NASM_PREPROC_LJ}
        COMMAND ${NASM_EXECUTABLE} -f elf64 ${NASM_PREPROC_LJ} -o ${NASM_OBJ_LJ}
        DEPENDS ${NASM_SRC_LJ}
        COMMENT "Preprocessing/Assembling (NASM) ${NASM_SRC_LJ}"
        VERBATIM
    )
    add_library(LongJumpNasmObject OBJECT ${NASM_OBJ_LJ})
    set_target_properties(LongJumpNasmObject PROPERTIES LINKER_LANGUAGE C)
    list(APPEND ASM_OBJECT_LIBS LongJumpNasmObject)

    # --- NASM File 2: SetJump ---
    set(NASM_SRC_SJ ${CMAKE_CURRENT_SOURCE_DIR}/edk2_mdepkg/Library/BaseLib/X64/SetJump.nasm)
    set(NASM_PREPROC_SJ ${CMAKE_CURRENT_BINARY_DIR}/SetJump.nasm.preprocessed)
    set(NASM_OBJ_SJ ${CMAKE_CURRENT_BINARY_DIR}/SetJump.nasm.o)

    add_custom_command(
        OUTPUT ${NASM_OBJ_SJ}
        COMMAND ${CMAKE_C_COMPILER} -E -P -x assembler-with-cpp "-D${ASM_PFX_DEFINE}" ${NASM_SRC_SJ} -o ${NASM_PREPROC_SJ}
        COMMAND ${NASM_EXECUTABLE} -f elf64 ${NASM_PREPROC_SJ} -o ${NASM_OBJ_SJ}
        DEPENDS ${NASM_SRC_SJ}
        COMMENT "Preprocessing/Assembling (NASM) ${NASM_SRC_SJ}"
        VERBATIM
    )
    add_library(SetJumpNasmObject OBJECT ${NASM_OBJ_SJ})
    set_target_properties(SetJumpNasmObject PROPERTIES LINKER_LANGUAGE C)
    list(APPEND ASM_OBJECT_LIBS SetJumpNasmObject)

endif()

# --- Target Definition ---
add_library(platform_common STATIC ${C_SOURCES})

# Apply common compiler flags from the centralized detection module
apply_common_compiler_flags(platform_common)

# Link the assembly object libraries (if any) into the main static library
if(ASM_OBJECT_LIBS)
    if(IS_WINDOWS)
        target_link_libraries(platform_common PRIVATE platform_common_asm)
    else()
        target_link_libraries(platform_common PRIVATE ${ASM_OBJECT_LIBS})
    endif()
endif()


# --- Include Directories (Common) ---
# Get the project root directory by going up two levels from the current directory
get_filename_component(PROJECT_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)

target_include_directories(platform_common PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/edk2_mdepkg/Include"
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${PROJECT_ROOT_DIR}"  # Add project root directory to match vcxproj configuration
)