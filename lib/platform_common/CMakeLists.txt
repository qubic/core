cmake_minimum_required(VERSION 3.15)

# Project definition - Enable C always
project(PlatformCommon LANGUAGES C)

# --- Platform and Toolchain Detection ---
set(IS_WINDOWS FALSE)
set(IS_LINUX_CLANG FALSE)
set(ASM_LANG "")

if(MSVC)
    # Windows with MSVC compiler
    set(IS_WINDOWS TRUE)
    enable_language(ASM_MASM)
    set(ASM_LANG ASM_MASM)
    message(STATUS "Configuring for Windows/MSVC (MASM)")
elseif(CMAKE_SYSTEM_NAME MATCHES Linux AND (CMAKE_C_COMPILER_ID MATCHES Clang OR CMAKE_C_COMPILER_ID MATCHES GNU))
    # Linux with Clang or GCC (will use NASM)
    set(IS_LINUX_CLANG TRUE) # Keep variable name for consistency, works for GCC too
    # NASM is not enabled via enable_language when using custom commands
    set(ASM_LANG ASM_NASM) # Indicate NASM usage conceptually
    message(STATUS "Configuring for Linux/${CMAKE_C_COMPILER_ID} (NASM via custom command)")
    find_program(NASM_EXECUTABLE nasm REQUIRED) # Make sure NASM is found
else()
    message(WARNING "Platform/Compiler not automatically configured for assembly: ${CMAKE_SYSTEM_NAME}/${CMAKE_C_COMPILER_ID}")
    # Add error or default behavior if needed
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
    add_library(platform_common_asm OBJECT
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

# Link the assembly object libraries (if any) into the main static library
if(ASM_OBJECT_LIBS)
    target_link_libraries(platform_common PRIVATE ${ASM_OBJECT_LIBS})
endif()


# --- Include Directories (Common) ---
# Get the project root directory by going up two levels from the current directory
get_filename_component(PROJECT_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
message(STATUS "Project root directory: ${PROJECT_ROOT_DIR}")

target_include_directories(platform_common PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/edk2_mdepkg/Include"
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${PROJECT_ROOT_DIR}"  # Add project root directory to match vcxproj configuration
)

# --- Common Preprocessor Definitions ---
target_compile_definitions(platform_common PRIVATE
    _LIB
    "$<$<CONFIG:Release>:NDEBUG>"
)

# --- Platform-Specific Settings ---
if(IS_WINDOWS)
    # Windows/MSVC specific settings
    target_compile_definitions(platform_common PRIVATE UNICODE _UNICODE)

    target_compile_options(platform_common PRIVATE
        # Common MSVC flags
        /GS- /EHsc- /guard:cf- /fp:except- /GF /W3
        # Debug MSVC flags
        "$<$<CONFIG:Debug>:/RTC1>"
        # Release MSVC flags
        "$<$<CONFIG:Release>:/GL->" "$<$<CONFIG:Release>:/Gw>" "$<$<CONFIG:Release>:/Oi>"
        "$<$<CONFIG:Release>:/Ob2>" "$<$<CONFIG:Release>:/O2>" "$<$<CONFIG:Release>:/Oy>"
    )
    # Apply MASM flags if needed to the platform_common_asm target
    # target_compile_options(platform_common_asm PRIVATE ...)

    set(CMAKE_STATIC_LIBRARY_FLAGS "${CMAKE_STATIC_LIBRARY_FLAGS} /IGNOREALLDEFAULTLIBS")

elseif(IS_LINUX_CLANG)
    # Linux/Clang specific settings for C files
    target_compile_options(platform_common PRIVATE
        -ffreestanding # No std
        -mno-red-zone
        -fshort-wchar # Use 16bit chars
        -Wall                     # Warning Level (/W3)
        -fno-stack-protector      # /GS-
        -fno-exceptions           # /EHsc- (mainly C++)
        # -fno-trapping-math      # /fp:except- (often default)
        -ffunction-sections       # /Gw (usually used with -fdata-sections)
        -fdata-sections
        # Debug Clang flags (/Zi -> -g)
        "$<$<CONFIG:Debug>:-g>"
        # Release Clang flags
        "$<$<CONFIG:Release>:-O2>"                   # /O2
        "$<$<CONFIG:Release>:-fomit-frame-pointer>" # /Oy
        "$<$<CONFIG:Release>:-fno-lto>"              # /GL- (disabled LTO)
    )
    # Note: No direct Clang equivalent needed for /IGNOREALLDEFAULTLIBS for static libs
endif()