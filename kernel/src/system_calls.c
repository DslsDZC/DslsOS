/**
 * @file system_calls.c
 * @brief System call interface implementation
 * @author DslsOS Team
 * @version 1.0
 * @date 2024
 */

#include "../include/kernel.h"
#include "../include/dslos.h"
#include <string.h>

// System call table
typedef struct _SYSCALL_ENTRY {
    ULONG SystemCallNumber;
    NTSTATUS (*Handler)(PVOID Parameters, ULONG ParameterLength);
    ULONG ParameterSize;
    ULONG Flags;
} SYSCALL_ENTRY;

// System call handler types
typedef NTSTATUS (*SYSCALL_HANDLER)(PVOID Parameters, ULONG ParameterLength);

// System call state
typedef struct _SYSCALL_STATE {
    BOOLEAN Initialized;
    SYSCALL_ENTRY SystemCallTable[SYSCALL_MAX];
    KSPIN_LOCK SyscallLock;
    ULONG SyscallCount;
    ULONG TotalSyscalls;
} SYSCALL_STATE;

static SYSCALL_STATE g_SyscallState = {0};

// Forward declarations for system call handlers
static NTSTATUS SyscallProcessCreate(PVOID Parameters, ULONG ParameterLength);
static NTSTATUS SyscallProcessTerminate(PVOID Parameters, ULONG ParameterLength);
static NTSTATUS SyscallMemoryAllocate(PVOID Parameters, ULONG ParameterLength);
static NTSTATUS SyscallMemoryFree(PVOID Parameters, ULONG ParameterLength);
static NTSTATUS SyscallIpcSend(PVOID Parameters, ULONG ParameterLength);
static NTSTATUS SyscallIpcReceive(PVOID Parameters, ULONG ParameterLength);
static NTSTATUS SyscallDeviceIoctl(PVOID Parameters, ULONG ParameterLength);
static NTSTATUS SyscallThreadCreate(PVOID Parameters, ULONG ParameterLength);
static NTSTATUS SyscallThreadTerminate(PVOID Parameters, ULONG ParameterLength);
static NTSTATUS SyscallThreadSuspend(PVOID Parameters, ULONG ParameterLength);
static NTSTATUS SyscallThreadResume(PVOID Parameters, ULONG ParameterLength);

// System call parameter structures
typedef struct _SYSCALL_PROCESS_CREATE_PARAMS {
    UNICODE_STRING ImagePath;
    UNICODE_STRING CommandLine;
    PVOID Environment;
    ULONG CreationFlags;
    HANDLE ParentProcess;
} SYSCALL_PROCESS_CREATE_PARAMS, *PSYSCALL_PROCESS_CREATE_PARAMS;

typedef struct _SYSCALL_PROCESS_TERMINATE_PARAMS {
    HANDLE ProcessHandle;
    NTSTATUS ExitCode;
} SYSCALL_PROCESS_TERMINATE_PARAMS, *PSYSCALL_PROCESS_TERMINATE_PARAMS;

typedef struct _SYSCALL_MEMORY_ALLOCATE_PARAMS {
    PVOID BaseAddress;
    SIZE_T RegionSize;
    ULONG AllocationType;
    ULONG Protect;
} SYSCALL_MEMORY_ALLOCATE_PARAMS, *PSYSCALL_MEMORY_ALLOCATE_PARAMS;

typedef struct _SYSCALL_MEMORY_FREE_PARAMS {
    PVOID BaseAddress;
    SIZE_T RegionSize;
    ULONG FreeType;
} SYSCALL_MEMORY_FREE_PARAMS, *PSYSCALL_MEMORY_FREE_PARAMS;

typedef struct _SYSCALL_IPC_SEND_PARAMS {
    HANDLE PortHandle;
    PVOID Message;
    SIZE_T MessageSize;
    PVOID Reply;
    SIZE_T ReplySize;
    ULONG Timeout;
} SYSCALL_IPC_SEND_PARAMS, *PSYSCALL_IPC_SEND_PARAMS;

typedef struct _SYSCALL_IPC_RECEIVE_PARAMS {
    HANDLE PortHandle;
    PVOID Message;
    SIZE_T MessageSize;
    ULONG Timeout;
} SYSCALL_IPC_RECEIVE_PARAMS, *PSYSCALL_IPC_RECEIVE_PARAMS;

typedef struct _SYSCALL_DEVICE_IOCTL_PARAMS {
    HANDLE DeviceHandle;
    ULONG IoControlCode;
    PVOID InputBuffer;
    SIZE_T InputBufferLength;
    PVOID OutputBuffer;
    SIZE_T OutputBufferLength;
    ULONG_PTR BytesReturned;
} SYSCALL_DEVICE_IOCTL_PARAMS, *PSYSCALL_DEVICE_IOCTL_PARAMS;

typedef struct _SYSCALL_THREAD_CREATE_PARAMS {
    HANDLE ProcessHandle;
    PVOID StartAddress;
    PVOID Parameter;
    ULONG StackSize;
    ULONG CreationFlags;
} SYSCALL_THREAD_CREATE_PARAMS, *PSYSCALL_THREAD_CREATE_PARAMS;

typedef struct _SYSCALL_THREAD_TERMINATE_PARAMS {
    HANDLE ThreadHandle;
    NTSTATUS ExitCode;
} SYSCALL_THREAD_TERMINATE_PARAMS, *PSYSCALL_THREAD_TERMINATE_PARAMS;

/**
 * @brief Initialize system call interface
 * @return NTSTATUS Status code
 */
NTSTATUS KeInitializeSystemCalls(VOID)
{
    if (g_SyscallState.Initialized) {
        return STATUS_SUCCESS;
    }

    KeInitializeSpinLock(&g_SyscallState.SyscallLock);
    g_SyscallState.SyscallCount = 0;
    g_SyscallState.TotalSyscalls = 0;

    // Register system call handlers
    NTSTATUS status = KeRegisterSyscallHandler(SYSCALL_PROCESS_CREATE, SyscallProcessCreate,
                                             sizeof(SYSCALL_PROCESS_CREATE_PARAMS), 0);
    if (!NT_SUCCESS(status)) return status;

    status = KeRegisterSyscallHandler(SYSCALL_PROCESS_TERMINATE, SyscallProcessTerminate,
                                     sizeof(SYSCALL_PROCESS_TERMINATE_PARAMS), 0);
    if (!NT_SUCCESS(status)) return status;

    status = KeRegisterSyscallHandler(SYSCALL_MEMORY_ALLOCATE, SyscallMemoryAllocate,
                                     sizeof(SYSCALL_MEMORY_ALLOCATE_PARAMS), 0);
    if (!NT_SUCCESS(status)) return status;

    status = KeRegisterSyscallHandler(SYSCALL_MEMORY_FREE, SyscallMemoryFree,
                                     sizeof(SYSCALL_MEMORY_FREE_PARAMS), 0);
    if (!NT_SUCCESS(status)) return status;

    status = KeRegisterSyscallHandler(SYSCALL_IPC_SEND, SyscallIpcSend,
                                     sizeof(SYSCALL_IPC_SEND_PARAMS), 0);
    if (!NT_SUCCESS(status)) return status;

    status = KeRegisterSyscallHandler(SYSCALL_IPC_RECEIVE, SyscallIpcReceive,
                                     sizeof(SYSCALL_IPC_RECEIVE_PARAMS), 0);
    if (!NT_SUCCESS(status)) return status;

    status = KeRegisterSyscallHandler(SYSCALL_DEVICE_IOCTL, SyscallDeviceIoctl,
                                     sizeof(SYSCALL_DEVICE_IOCTL_PARAMS), 0);
    if (!NT_SUCCESS(status)) return status;

    status = KeRegisterSyscallHandler(SYSCALL_THREAD_CREATE, SyscallThreadCreate,
                                     sizeof(SYSCALL_THREAD_CREATE_PARAMS), 0);
    if (!NT_SUCCESS(status)) return status;

    status = KeRegisterSyscallHandler(SYSCALL_THREAD_TERMINATE, SyscallThreadTerminate,
                                     sizeof(SYSCALL_THREAD_TERMINATE_PARAMS), 0);
    if (!NT_SUCCESS(status)) return status;

    status = KeRegisterSyscallHandler(SYSCALL_THREAD_SUSPEND, SyscallThreadSuspend, 0, 0);
    if (!NT_SUCCESS(status)) return status;

    status = KeRegisterSyscallHandler(SYSCALL_THREAD_RESUME, SyscallThreadResume, 0, 0);
    if (!NT_SUCCESS(status)) return status;

    g_SyscallState.Initialized = TRUE;
    return STATUS_SUCCESS;
}

/**
 * @brief Register system call handler
 * @param SystemCallNumber System call number
 * @param Handler Handler function
 * @param ParameterSize Size of parameter structure
 * @param Flags Handler flags
 * @return NTSTATUS Status code
 */
NTSTATUS KeRegisterSyscallHandler(ULONG SystemCallNumber, SYSCALL_HANDLER Handler,
                                 ULONG ParameterSize, ULONG Flags)
{
    if (SystemCallNumber >= SYSCALL_MAX || Handler == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    if (g_SyscallState.SystemCallTable[SystemCallNumber].Handler != NULL) {
        return STATUS_OBJECT_NAME_COLLISION;
    }

    KIRQL old_irql;
    KeAcquireSpinLock(&g_SyscallState.SyscallLock, &old_irql);

    g_SyscallState.SystemCallTable[SystemCallNumber].SystemCallNumber = SystemCallNumber;
    g_SyscallState.SystemCallTable[SystemCallNumber].Handler = Handler;
    g_SyscallState.SystemCallTable[SystemCallNumber].ParameterSize = ParameterSize;
    g_SyscallState.SystemCallTable[SystemCallNumber].Flags = Flags;

    g_SyscallState.SyscallCount++;

    KeReleaseSpinLock(&g_SyscallState.SyscallLock, old_irql);

    return STATUS_SUCCESS;
}

/**
 * @brief System call dispatcher
 * @param SystemCallNumber System call number
 * @param Parameters System call parameters
 * @param ParameterLength Length of parameters
 * @return NTSTATUS Status code
 */
NTSTATUS KeDispatchSystemCall(ULONG SystemCallNumber, PVOID Parameters, ULONG ParameterLength)
{
    if (SystemCallNumber >= SYSCALL_MAX || Parameters == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    KIRQL old_irql;
    KeAcquireSpinLock(&g_SyscallState.SyscallLock, &old_irql);

    SYSCALL_ENTRY* entry = &g_SyscallState.SystemCallTable[SystemCallNumber];
    if (entry->Handler == NULL) {
        KeReleaseSpinLock(&g_SyscallState.SyscallLock, old_irql);
        return STATUS_INVALID_SYSTEM_SERVICE;
    }

    // Validate parameter size
    if (entry->ParameterSize > 0 && ParameterLength < entry->ParameterSize) {
        KeReleaseSpinLock(&g_SyscallState.SyscallLock, old_irql);
        return STATUS_BUFFER_TOO_SMALL;
    }

    SYSCALL_HANDLER handler = entry->Handler;
    KeReleaseSpinLock(&g_SyscallState.SyscallLock, old_irql);

    // Update statistics
    InterlockedIncrement(&g_SyscallState.TotalSyscalls);

    // Call handler
    return handler(Parameters, ParameterLength);
}

/**
 * @brief Get system call statistics
 * @param TotalSyscalls Total system calls executed
 * @param RegisteredSyscalls Number of registered system calls
 */
VOID KeGetSyscallStatistics(PULONG TotalSyscalls, PULONG RegisteredSyscalls)
{
    if (TotalSyscalls) {
        *TotalSyscalls = g_SyscallState.TotalSyscalls;
    }
    if (RegisteredSyscalls) {
        *RegisteredSyscalls = g_SyscallState.SyscallCount;
    }
}

// System call handler implementations

/**
 * @brief Process creation system call handler
 */
static NTSTATUS SyscallProcessCreate(PVOID Parameters, ULONG ParameterLength)
{
    PSYSCALL_PROCESS_CREATE_PARAMS params = (PSYSCALL_PROCESS_CREATE_PARAMS)Parameters;

    // Validate parameters
    if (params == NULL || ParameterLength < sizeof(SYSCALL_PROCESS_CREATE_PARAMS)) {
        return STATUS_INVALID_PARAMETER;
    }

    // Validate image path
    if (params->ImagePath.Buffer == NULL || params->ImagePath.Length == 0) {
        return STATUS_INVALID_PARAMETER;
    }

    // Create process
    HANDLE process_handle;
    NTSTATUS status = NtCreateProcess(&process_handle,
                                     PROCESS_ALL_ACCESS,
                                     NULL,
                                     params->ParentProcess,
                                     params->CreationFlags,
                                     NULL);

    if (NT_SUCCESS(status)) {
        // Start the process if requested
        if (params->CreationFlags & CREATE_PROCESS_IMMEDIATE) {
            status = NtResumeThread(process_handle, NULL);
        }
    }

    return status;
}

/**
 * @brief Process termination system call handler
 */
static NTSTATUS SyscallProcessTerminate(PVOID Parameters, ULONG ParameterLength)
{
    PSYSCALL_PROCESS_TERMINATE_PARAMS params = (PSYSCALL_PROCESS_TERMINATE_PARAMS)Parameters;

    if (params == NULL || ParameterLength < sizeof(SYSCALL_PROCESS_TERMINATE_PARAMS)) {
        return STATUS_INVALID_PARAMETER;
    }

    if (params->ProcessHandle == NULL) {
        return STATUS_INVALID_HANDLE;
    }

    return NtTerminateProcess(params->ProcessHandle, params->ExitCode);
}

/**
 * @brief Memory allocation system call handler
 */
static NTSTATUS SyscallMemoryAllocate(PVOID Parameters, ULONG ParameterLength)
{
    PSYSCALL_MEMORY_ALLOCATE_PARAMS params = (PSYSCALL_MEMORY_ALLOCATE_PARAMS)Parameters;

    if (params == NULL || ParameterLength < sizeof(SYSCALL_MEMORY_ALLOCATE_PARAMS)) {
        return STATUS_INVALID_PARAMETER;
    }

    return NtAllocateVirtualMemory(NtCurrentProcess(),
                                   &params->BaseAddress,
                                   0,
                                   &params->RegionSize,
                                   params->AllocationType,
                                   params->Protect);
}

/**
 * @brief Memory free system call handler
 */
static NTSTATUS SyscallMemoryFree(PVOID Parameters, ULONG ParameterLength)
{
    PSYSCALL_MEMORY_FREE_PARAMS params = (PSYSCALL_MEMORY_FREE_PARAMS)Parameters;

    if (params == NULL || ParameterLength < sizeof(SYSCALL_MEMORY_FREE_PARAMS)) {
        return STATUS_INVALID_PARAMETER;
    }

    return NtFreeVirtualMemory(NtCurrentProcess(),
                              &params->BaseAddress,
                              &params->RegionSize,
                              params->FreeType);
}

/**
 * @brief IPC send system call handler
 */
static NTSTATUS SyscallIpcSend(PVOID Parameters, ULONG ParameterLength)
{
    PSYSCALL_IPC_SEND_PARAMS params = (PSYSCALL_IPC_SEND_PARAMS)Parameters;

    if (params == NULL || ParameterLength < sizeof(SYSCALL_IPC_SEND_PARAMS)) {
        return STATUS_INVALID_PARAMETER;
    }

    return NtRequestWaitReplyPort(params->PortHandle,
                                  params->Message,
                                  params->Reply);
}

/**
 * @brief IPC receive system call handler
 */
static NTSTATUS SyscallIpcReceive(PVOID Parameters, ULONG ParameterLength)
{
    PSYSCALL_IPC_RECEIVE_PARAMS params = (PSYSCALL_IPC_RECEIVE_PARAMS)Parameters;

    if (params == NULL || ParameterLength < sizeof(SYSCALL_IPC_RECEIVE_PARAMS)) {
        return STATUS_INVALID_PARAMETER;
    }

    LARGE_INTEGER timeout;
    timeout.QuadPart = params->Timeout * -10000LL; // Convert milliseconds to 100-nanosecond units

    return NtReplyWaitReceivePort(params->PortHandle,
                                  NULL,
                                  params->Message,
                                  &timeout);
}

/**
 * @brief Device I/O control system call handler
 */
static NTSTATUS SyscallDeviceIoctl(PVOID Parameters, ULONG ParameterLength)
{
    PSYSCALL_DEVICE_IOCTL_PARAMS params = (PSYSCALL_DEVICE_IOCTL_PARAMS)Parameters;

    if (params == NULL || ParameterLength < sizeof(SYSCALL_DEVICE_IOCTL_PARAMS)) {
        return STATUS_INVALID_PARAMETER;
    }

    IO_STATUS_BLOCK io_status;
    return NtDeviceIoControlFile(params->DeviceHandle,
                                NULL,
                                NULL,
                                NULL,
                                &io_status,
                                params->IoControlCode,
                                params->InputBuffer,
                                params->InputBufferLength,
                                params->OutputBuffer,
                                params->OutputBufferLength);
}

/**
 * @brief Thread creation system call handler
 */
static NTSTATUS SyscallThreadCreate(PVOID Parameters, ULONG ParameterLength)
{
    PSYSCALL_THREAD_CREATE_PARAMS params = (PSYSCALL_THREAD_CREATE_PARAMS)Parameters;

    if (params == NULL || ParameterLength < sizeof(SYSCALL_THREAD_CREATE_PARAMS)) {
        return STATUS_INVALID_PARAMETER;
    }

    HANDLE thread_handle;
    CLIENT_ID client_id;

    return NtCreateThread(&thread_handle,
                         THREAD_ALL_ACCESS,
                         NULL,
                         params->ProcessHandle,
                         &client_id,
                         params->StartAddress,
                         params->Parameter,
                         params->CreationFlags,
                         0,
                         params->StackSize,
                         0);
}

/**
 * @brief Thread termination system call handler
 */
static NTSTATUS SyscallThreadTerminate(PVOID Parameters, ULONG ParameterLength)
{
    PSYSCALL_THREAD_TERMINATE_PARAMS params = (PSYSCALL_THREAD_TERMINATE_PARAMS)Parameters;

    if (params == NULL || ParameterLength < sizeof(SYSCALL_THREAD_TERMINATE_PARAMS)) {
        return STATUS_INVALID_PARAMETER;
    }

    return NtTerminateThread(params->ThreadHandle, params->ExitCode);
}

/**
 * @brief Thread suspend system call handler
 */
static NTSTATUS SyscallThreadSuspend(PVOID Parameters, ULONG ParameterLength)
{
    HANDLE thread_handle = (HANDLE)Parameters;

    if (thread_handle == NULL) {
        return STATUS_INVALID_HANDLE;
    }

    return NtSuspendThread(thread_handle, NULL);
}

/**
 * @brief Thread resume system call handler
 */
static NTSTATUS SyscallThreadResume(PVOID Parameters, ULONG ParameterLength)
{
    HANDLE thread_handle = (HANDLE)Parameters;

    if (thread_handle == NULL) {
        return STATUS_INVALID_HANDLE;
    }

    return NtResumeThread(thread_handle, NULL);
}