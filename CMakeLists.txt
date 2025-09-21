cmake_minimum_required(VERSION 3.16)
project(DslsOS VERSION 1.0.0 LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler flags
if(MSVC)
    add_compile_options(/W4)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DUNICODE -D_UNICODE)
else()
    add_compile_options(-Wall -Wextra)
    add_definitions(-D_GNU_SOURCE)
    add_definitions(-D_DSLOS_KERNEL_)
    add_definitions(-D_KERNEL_MODE_)
endif()

# Architecture detection
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    set(ARCH_X64 1)
    add_definitions(-DARCH_X64)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "i386|i686")
    set(ARCH_X86 1)
    add_definitions(-DARCH_X86)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
    set(ARCH_ARM64 1)
    add_definitions(-DARCH_ARM64)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm")
    set(ARCH_ARM 1)
    add_definitions(-DARCH_ARM)
endif()

# Include directories
include_directories(include)
include_directories(kernel/include)

# Add subdirectories
add_subdirectory(kernel)

# Create main executable
add_executable(dslsos
    src/main.c
    src/system.c
    src/hal.c
)

# Link libraries
target_link_libraries(dslsos
    dsls_kernel
)

# Set output directory
set_target_properties(dslsos PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Install targets
install(TARGETS dslsos DESTINATION bin)

# Package configuration
set(CPACK_PACKAGE_NAME "DslsOS")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "DslsOS - Advanced Distributed Operating System")
set(CPACK_PACKAGE_VENDOR "DslsOS Team")

include(CPack)
