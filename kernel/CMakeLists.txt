# DslsOS Kernel CMakeLists.txt

# Create kernel library
add_library(dsls_kernel STATIC
    src/kernel_main.c
    src/object_manager.c
    src/process_manager.c
    src/thread_manager.c
    src/memory_manager.c
    src/ipc_manager.c
    src/scheduler.c
    src/hardware_abstraction.c
    src/system_calls.c
    src/interrupt_handler.c
    src/timer.c
    src/device_manager.c
    src/driver_interface.c
    src/dslsfs.c
    src/kernel_loader.c
    src/system_test.c
    src/advanced_scheduler.c
    src/container_system.c
    src/security_architecture.c
    src/distributed_management.c
    src/composite_ui.c
)

# Include directories
target_include_directories(dsls_kernel
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# Compile definitions
target_compile_definitions(dsls_kernel
    PRIVATE
    DSLOS_KERNEL
    DSLOS_BUILD_KERNEL
)

# Compiler flags
if(MSVC)
    target_compile_options(dsls_kernel PRIVATE /kernel)
else()
    target_compile_options(dsls_kernel PRIVATE -fno-stack-protector -fno-pie)
endif()

# Link libraries
target_link_libraries(dsls_kernel
    PUBLIC
    # Add any required kernel dependencies here
)

# Install kernel headers
install(DIRECTORY include/
    DESTINATION include/dsls/kernel
    FILES_MATCHING
    PATTERN "*.h"
)