/**
 * @file main.c
 * @brief DslsOS main entry point
 * @author DslsOS Team
 * @version 1.0
 * @date 2024
 */

#include "../kernel/include/kernel.h"
#include "../kernel/include/dslos.h"

// Boot information structure (simplified)
typedef struct _BOOT_INFORMATION {
    ULONG BootType;
    ULONG BootFlags;
    LARGE_INTEGER BootTime;
    UNICODE_STRING BootDevice;
    UNICODE_STRING KernelPath;
    ULONG MemorySize;
    ULONG NumberOfProcessors;
} BOOT_INFORMATION, *PBOOT_INFORMATION;

// Boot types
#define BOOT_TYPE_BIOS              0x01
#define BOOT_TYPE_UEFI              0x02

// Boot flags
#define BOOT_FLAG_DEBUG             0x00000001
#define BOOT_FLAG_VERBOSE           0x00000010

// Forward declarations
extern NTSTATUS KiKernelMain(PBOOT_INFORMATION BootInfo);

/**
 * @brief Main entry point
 * @return Status code
 */
int main(void)
{
    // Initialize boot information
    BOOT_INFORMATION boot_info;
    RtlZeroMemory(&boot_info, sizeof(BOOT_INFORMATION));

    // Set boot information
    boot_info.BootType = BOOT_TYPE_BIOS;
    boot_info.BootFlags = BOOT_FLAG_DEBUG | BOOT_FLAG_VERBOSE;
    boot_info.MemorySize = 1024 * 1024 * 1024; // 1GB
    boot_info.NumberOfProcessors = 1;

    // Set boot device (simplified)
    WCHAR boot_device[] = L"\\Device\\Harddisk0\\Partition1";
    boot_info.BootDevice.Buffer = boot_device;
    boot_info.BootDevice.Length = (USHORT)wcslen(boot_device) * sizeof(WCHAR);
    boot_info.BootDevice.MaximumLength = boot_info.BootDevice.Length + sizeof(WCHAR);

    // Set kernel path (simplified)
    WCHAR kernel_path[] = L"\\System\\kernel.exe";
    boot_info.KernelPath.Buffer = kernel_path;
    boot_info.KernelPath.Length = (USHORT)wcslen(kernel_path) * sizeof(WCHAR);
    boot_info.KernelPath.MaximumLength = boot_info.KernelPath.Length + sizeof(WCHAR);

    // Get boot time
    KeQuerySystemTime(&boot_info.BootTime);

    // Display startup message
    HalDisplayString(L"DslsOS Bootloader Starting...\r\n");
    HalDisplayString(L"Loading kernel...\r\n");

    // Call kernel main
    NTSTATUS status = KiKernelMain(&boot_info);

    if (NT_SUCCESS(status)) {
        HalDisplayString(L"\r\nDslsOS kernel loaded successfully!\r\n");

        // Run system tests
        HalDisplayString(L"\r\nRunning system tests...\r\n");
        TmInitializeTestManager();
        TmRunAllTests();

        HalDisplayString(L"\r\nSystem ready. Press any key to shutdown...\r\n");
        HalWaitForKeyPress();

        // Shutdown system
        HalDisplayString(L"Shutting down DslsOS...\r\n");
        HalShutdownSystem();
    } else {
        HalDisplayString(L"\r\nFailed to load DslsOS kernel!\r\n");

        WCHAR error_msg[64];
        RtlStringCchPrintfW(error_msg, 64, L"Error: 0x%08X\r\n", status);
        HalDisplayString(error_msg);

        HalDisplayString(L"System halted.\r\n");
        HalHaltSystem();
    }

    return (int)status;
}