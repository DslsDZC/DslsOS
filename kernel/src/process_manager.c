/**
 * @file process_manager.c
 * @brief Process management subsystem implementation
 * @author DslsOS Team
 * @version 1.0
 * @date 2024
 */

#include "../include/kernel.h"
#include "../include/dslos.h"
#include <string.h>

// Process manager state
typedef struct _PROCESS_MANAGER_STATE {
    BOOLEAN Initialized;
    KSPIN_LOCK ProcessLock;
    KSPIN_LOCK ThreadLock;

    // Process management
    LIST_ENTRY ProcessListHead;
    ULONG ProcessCount;
    ULONG NextProcessId;

    // Thread management
    LIST_ENTRY ThreadListHead;
    ULONG ThreadCount;
    ULONG NextThreadId;

    // System processes
    PPROCESS_CONTROL_BLOCK IdleProcess;
    PPROCESS_CONTROL_BLOCK SystemProcess;

    // Process statistics
    PROCESS_STATISTICS Statistics;

    // Handle table for processes
    LIST_ENTRY HandleTableHead;
    ULONG HandleCount;
} PROCESS_MANAGER_STATE;

static PROCESS_MANAGER_STATE g_ProcessManager = {0};

// Handle entry structure
typedef struct _HANDLE_ENTRY {
    HANDLE Handle;
    PKERNEL_OBJECT Object;
    ACCESS_MASK GrantedAccess;
    ULONG HandleAttributes;
    LIST_ENTRY HandleListEntry;
} HANDLE_ENTRY, *PHANDLE_ENTRY;

// Process statistics structure
typedef struct _PROCESS_STATISTICS {
    ULONG TotalProcessesCreated;
    ULONG TotalProcessesTerminated;
    ULONG TotalThreadsCreated;
    ULONG TotalThreadsTerminated;
    ULONG ActiveProcessCount;
    ULONG ActiveThreadCount;
    LARGE_INTEGER TotalCpuTime;
} PROCESS_STATISTICS, *PPROCESS_STATISTICS;

// Process creation flags
#define CREATE_PROCESS_SUSPENDED    0x00000001
#define CREATE_PROCESS_DEBUG         0x00000002
#define CREATE_PROCESS_INHERIT_HANDLES 0x00000004

// Thread creation flags
#define CREATE_THREAD_SUSPENDED     0x00000001
#define CREATE_THREAD_DEBUG          0x00000002
#define CREATE_THREAD_HIDE_FROM_DEBUGGER 0x00000004

/**
 * @brief Initialize process manager
 * @return NTSTATUS Status code
 */
NTSTATUS PsInitializeProcessManager(VOID)
{
    if (g_ProcessManager.Initialized) {
        return STATUS_SUCCESS;
    }

    KeInitializeSpinLock(&g_ProcessManager.ProcessLock);
    KeInitializeSpinLock(&g_ProcessManager.ThreadLock);

    // Initialize process list
    InitializeListHead(&g_ProcessManager.ProcessListHead);
    g_ProcessManager.ProcessCount = 0;
    g_ProcessManager.NextProcessId = 1;

    // Initialize thread list
    InitializeListHead(&g_ProcessManager.ThreadListHead);
    g_ProcessManager.ThreadCount = 0;
    g_ProcessManager.NextThreadId = 1;

    // Initialize handle table
    InitializeListHead(&g_ProcessManager.HandleTableHead);
    g_ProcessManager.HandleCount = 0;

    // Initialize statistics
    RtlZeroMemory(&g_ProcessManager.Statistics, sizeof(PROCESS_STATISTICS));

    // Create system processes
    NTSTATUS status = PsCreateSystemProcesses();
    if (!NT_SUCCESS(status)) {
        return status;
    }

    g_ProcessManager.Initialized = TRUE;
    return STATUS_SUCCESS;
}

/**
 * @brief Create system processes
 * @return NTSTATUS Status code
 */
static NTSTATUS PsCreateSystemProcesses(VOID)
{
    // Create idle process
    NTSTATUS status = PsCreateProcessInternal(&g_ProcessManager.IdleProcess,
                                            L"\\System\\Idle.exe",
                                            NULL,
                                            PROCESS_PRIORITY_IDLE);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    g_ProcessManager.IdleProcess->State = PROCESS_STATE_RUNNING;

    // Create system process
    status = PsCreateProcessInternal(&g_ProcessManager.SystemProcess,
                                   L"\\System\\System.exe",
                                   NULL,
                                   PROCESS_PRIORITY_HIGH);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    g_ProcessManager.SystemProcess->State = PROCESS_STATE_RUNNING;

    return STATUS_SUCCESS;
}

/**
 * @brief Create a new process
 * @param Process Pointer to receive process pointer
 * @param ImagePath Process image path
 * @param Parent Parent process (can be NULL)
 * @return NTSTATUS Status code
 */
NTSTATUS PsCreateProcess(PPROCESS_CONTROL_BLOCK* Process, PCSTR ImagePath, PPROCESS_CONTROL_BLOCK Parent)
{
    if (Process == NULL || ImagePath == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    // Convert image path to unicode
    UNICODE_STRING unicode_path;
    NTSTATUS status = RtlCreateUnicodeStringFromAnsiString(&unicode_path, ImagePath);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    // Create process
    status = PsCreateProcessInternal(Process, unicode_path.Buffer, Parent, PROCESS_PRIORITY_NORMAL);

    RtlFreeUnicodeString(&unicode_path);
    return status;
}

/**
 * @brief Internal process creation function
 * @param Process Pointer to receive process pointer
 * @param ImagePath Process image path
 * @param Parent Parent process
 * @param Priority Process priority
 * @return NTSTATUS Status code
 */
static NTSTATUS PsCreateProcessInternal(PPROCESS_CONTROL_BLOCK* Process, PCWSTR ImagePath,
                                       PPROCESS_CONTROL_BLOCK Parent, LONG Priority)
{
    // Allocate process control block
    PPROCESS_CONTROL_BLOCK new_process = ExAllocatePool(NonPagedPool, sizeof(PROCESS_CONTROL_BLOCK));
    if (new_process == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(new_process, sizeof(PROCESS_CONTROL_BLOCK));

    // Initialize process header
    new_process->Header.ObjectType = KERNEL_OBJECT_TYPE_PROCESS;
    new_process->Header.ReferenceCount = 1;
    new_process->Header.Flags = 0;
    InitializeListHead(&new_process->Header.ObjectListEntry);

    // Set process identification
    new_process->ProcessId = (PROCESS_ID)g_ProcessManager.NextProcessId++;
    new_process->ParentProcessId = (Parent != NULL) ? Parent->ProcessId : 0;
    new_process->SessionId = 0;

    // Set process state
    new_process->State = PROCESS_STATE_CREATED;
    new_process->ExitStatus = STATUS_PENDING;

    // Set priority
    new_process->PrivilegeLevel = (Parent != NULL) ? Parent->PrivilegeLevel : SECURITY_LEVEL_USER;

    // Initialize process lists
    InitializeListHead(&new_process->ProcessListEntry);
    InitializeListHead(&new_process->ThreadListHead);
    new_process->ThreadCount = 0;

    // Create address space
    NTSTATUS status = MmCreateAddressSpace(new_process);
    if (!NT_SUCCESS(status)) {
        ExFreePool(new_process);
        return status;
    }

    // Set resource limits
    new_process->CpuTimeLimit = 0; // No limit
    new_process->MemoryLimit = 0;   // No limit
    new_process->HandleLimit = 4096;

    // Initialize security token
    if (Parent != NULL) {
        // Inherit from parent
        status = SeCreateToken(&new_process->SecurityToken, Parent->PrivilegeLevel);
    } else {
        // Create new token
        status = SeCreateToken(&new_process->SecurityToken, SECURITY_LEVEL_USER);
    }

    if (!NT_SUCCESS(status)) {
        MmDestroyAddressSpace(new_process);
        ExFreePool(new_process);
        return status;
    }

    // Set creation time
    KeQuerySystemTime(&new_process->CreateTime);
    new_process->ExitTime.QuadPart = 0;

    // Add to process list
    KIRQL old_irql;
    KeAcquireSpinLock(&g_ProcessManager.ProcessLock, &old_irql);
    InsertTailList(&g_ProcessManager.ProcessListHead, &new_process->ProcessListEntry);
    g_ProcessManager.ProcessCount++;
    g_ProcessManager.Statistics.TotalProcessesCreated++;
    g_ProcessManager.Statistics.ActiveProcessCount++;
    KeReleaseSpinLock(&g_ProcessManager.ProcessLock, old_irql);

    // Create main thread
    PTHREAD_CONTROL_BLOCK main_thread;
    status = PsCreateMainThread(new_process, &main_thread);
    if (!NT_SUCCESS(status)) {
        // Clean up process
        KeAcquireSpinLock(&g_ProcessManager.ProcessLock, &old_irql);
        RemoveEntryList(&new_process->ProcessListEntry);
        g_ProcessManager.ProcessCount--;
        g_ProcessManager.Statistics.ActiveProcessCount--;
        KeReleaseSpinLock(&g_ProcessManager.ProcessLock, old_irql);

        MmDestroyAddressSpace(new_process);
        ExFreePool(new_process);
        return status;
    }

    // Update process statistics
    new_process->ThreadCount = 1;

    *Process = new_process;
    return STATUS_SUCCESS;
}

/**
 * @brief Create main thread for process
 * @param Process Process to create thread for
 * @param Thread Pointer to receive thread pointer
 * @return NTSTATUS Status code
 */
static NTSTATUS PsCreateMainThread(PPROCESS_CONTROL_BLOCK Process, PTHREAD_CONTROL_BLOCK* Thread)
{
    // Allocate thread control block
    PTHREAD_CONTROL_BLOCK new_thread = ExAllocatePool(NonPagedPool, sizeof(THREAD_CONTROL_BLOCK));
    if (new_thread == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(new_thread, sizeof(THREAD_CONTROL_BLOCK));

    // Initialize thread header
    new_thread->Header.ObjectType = KERNEL_OBJECT_TYPE_THREAD;
    new_thread->Header.ReferenceCount = 1;
    new_thread->Header.Flags = 0;
    InitializeListHead(&new_thread->Header.ObjectListEntry);

    // Set thread identification
    new_thread->ThreadId = (THREAD_ID)g_ProcessManager.NextThreadId++;
    new_thread->Process = Process;

    // Set thread state
    new_thread->State = THREAD_STATE_CREATED;
    new_thread->WaitReason = WAIT_REASON_EXECUTIVE;

    // Set thread priority
    new_thread->Priority = 8; // Default priority
    new_thread->BasePriority = 8;

    // Allocate thread stack
    NTSTATUS status = PsAllocateThreadStack(new_thread);
    if (!NT_SUCCESS(status)) {
        ExFreePool(new_thread);
        return status;
    }

    // Initialize thread context (simplified)
    status = PsInitializeThreadContext(new_thread);
    if (!NT_SUCCESS(status)) {
        PsFreeThreadStack(new_thread);
        ExFreePool(new_thread);
        return status;
    }

    // Initialize thread lists
    InitializeListHead(&new_thread->ThreadListEntry);
    InitializeListHead(&new_thread->ReadyListEntry);
    InitializeListHead(&new_thread->WaitListEntry);

    // Set creation time
    KeQuerySystemTime(&new_thread->CreateTime);
    new_thread->KernelTime.QuadPart = 0;
    new_thread->UserTime.QuadPart = 0;
    new_thread->ContextSwitchCount = 0;

    // Initialize TLS
    new_thread->TlsArray = NULL;
    new_thread->TlsSize = 0;

    // Add to thread list
    KIRQL old_irql;
    KeAcquireSpinLock(&g_ProcessManager.ThreadLock, &old_irql);
    InsertTailList(&g_ProcessManager.ThreadListHead, &new_thread->ThreadListEntry);
    g_ProcessManager.ThreadCount++;
    g_ProcessManager.Statistics.TotalThreadsCreated++;
    g_ProcessManager.Statistics.ActiveThreadCount++;
    KeReleaseSpinLock(&g_ProcessManager.ThreadLock, old_irql);

    // Add to process thread list
    KeAcquireSpinLock(&g_ProcessManager.ProcessLock, &old_irql);
    InsertTailList(&Process->ThreadListHead, &new_thread->ThreadListEntry);
    KeReleaseSpinLock(&g_ProcessManager.ProcessLock, old_irql);

    // Add thread to ready queue
    KeAddThreadToReadyQueue(new_thread);

    *Thread = new_thread;
    return STATUS_SUCCESS;
}

/**
 * @brief Allocate thread stack
 * @param Thread Thread to allocate stack for
 * @return NTSTATUS Status code
 */
static NTSTATUS PsAllocateThreadStack(PTHREAD_CONTROL_BLOCK Thread)
{
    // Allocate kernel stack
    SIZE_T kernel_stack_size = 16 * 1024; // 16KB kernel stack
    Thread->KernelStack = MmAllocateVirtualMemory(Thread->Process, NULL, kernel_stack_size, PAGE_READWRITE);
    if (Thread->KernelStack == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Allocate user stack
    SIZE_T user_stack_size = 1024 * 1024; // 1MB user stack
    Thread->UserStack = MmAllocateVirtualMemory(Thread->Process, NULL, user_stack_size, PAGE_READWRITE);
    if (Thread->UserStack == NULL) {
        MmFreeVirtualMemory(Thread->Process, Thread->KernelStack, kernel_stack_size);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Set stack boundaries
    Thread->StackBase = Thread->UserStack;
    Thread->StackLimit = (PVOID)((UINT_PTR)Thread->UserStack + user_stack_size);

    return STATUS_SUCCESS;
}

/**
 * @brief Free thread stack
 * @param Thread Thread to free stack for
 */
static VOID PsFreeThreadStack(PTHREAD_CONTROL_BLOCK Thread)
{
    if (Thread->KernelStack != NULL) {
        MmFreeVirtualMemory(Thread->Process, Thread->KernelStack, 16 * 1024);
        Thread->KernelStack = NULL;
    }

    if (Thread->UserStack != NULL) {
        MmFreeVirtualMemory(Thread->Process, Thread->UserStack, 1024 * 1024);
        Thread->UserStack = NULL;
    }
}

/**
 * @brief Initialize thread context
 * @param Thread Thread to initialize context for
 * @return NTSTATUS Status code
 */
static NTSTATUS PsInitializeThreadContext(PTHREAD_CONTROL_BLOCK Thread)
{
    // This is a simplified implementation
    // In a real implementation, this would:
    // - Set up proper CPU context
    // - Set instruction pointer to entry point
    // - Set stack pointer
    // - Set up initial registers

    Thread->InstructionPointer = (PVOID)0x10000000; // Dummy entry point

    return STATUS_SUCCESS;
}

/**
 * @brief Terminate a process
 * @param Process Process to terminate
 * @param ExitStatus Exit status code
 * @return NTSTATUS Status code
 */
NTSTATUS PsTerminateProcess(PPROCESS_CONTROL_BLOCK Process, NTSTATUS ExitStatus)
{
    if (Process == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    // Cannot terminate system processes
    if (Process == g_ProcessManager.IdleProcess || Process == g_ProcessManager.SystemProcess) {
        return STATUS_ACCESS_DENIED;
    }

    // Set process state to terminating
    Process->State = PROCESS_STATE_TERMINATED;
    Process->ExitStatus = ExitStatus;
    KeQuerySystemTime(&Process->ExitTime);

    // Terminate all threads in the process
    KIRQL old_irql;
    KeAcquireSpinLock(&g_ProcessManager.ProcessLock, &old_irql);

    while (!IsListEmpty(&Process->ThreadListHead)) {
        PLIST_ENTRY entry = RemoveHeadList(&Process->ThreadListHead);
        PTHREAD_CONTROL_BLOCK thread = CONTAINING_RECORD(entry, THREAD_CONTROL_BLOCK, ThreadListEntry);
        Process->ThreadCount--;

        // Terminate thread
        PsTerminateThreadInternal(thread, STATUS_PROCESS_TERMINATED);
    }

    KeReleaseSpinLock(&g_ProcessManager.ProcessLock, old_irql);

    // Update statistics
    InterlockedIncrement(&g_ProcessManager.Statistics.TotalProcessesTerminated);
    InterlockedDecrement(&g_ProcessManager.Statistics.ActiveProcessCount);

    // Schedule process for cleanup
    PsScheduleProcessCleanup(Process);

    return STATUS_SUCCESS;
}

/**
 * @brief Terminate a thread
 * @param Thread Thread to terminate
 * @param ExitStatus Exit status code
 * @return NTSTATUS Status code
 */
NTSTATUS PsTerminateThread(PTHREAD_CONTROL_BLOCK Thread, NTSTATUS ExitStatus)
{
    if (Thread == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    return PsTerminateThreadInternal(Thread, ExitStatus);
}

/**
 * @brief Internal thread termination function
 * @param Thread Thread to terminate
 * @param ExitStatus Exit status code
 * @return NTSTATUS Status code
 */
static NTSTATUS PsTerminateThreadInternal(PTHREAD_CONTROL_BLOCK Thread, NTSTATUS ExitStatus)
{
    // Set thread state to terminated
    Thread->State = THREAD_STATE_TERMINATED;

    // Remove from ready queue if present
    KeRemoveThreadFromReadyQueue(Thread);

    // Update statistics
    InterlockedIncrement(&g_ProcessManager.Statistics.TotalThreadsTerminated);
    InterlockedDecrement(&g_ProcessManager.Statistics.ActiveThreadCount);

    // Remove from global thread list
    KIRQL old_irql;
    KeAcquireSpinLock(&g_ProcessManager.ThreadLock, &old_irql);
    RemoveEntryList(&Thread->ThreadListEntry);
    g_ProcessManager.ThreadCount--;
    KeReleaseSpinLock(&g_ProcessManager.ThreadLock, old_irql);

    // Remove from process thread list
    if (Thread->Process != NULL) {
        KeAcquireSpinLock(&g_ProcessManager.ProcessLock, &old_irql);
        if (Thread->Process->ThreadCount > 0) {
            Thread->Process->ThreadCount--;
        }
        KeReleaseSpinLock(&g_ProcessManager.ProcessLock, old_irql);
    }

    // Schedule thread for cleanup
    PsScheduleThreadCleanup(Thread);

    return STATUS_SUCCESS;
}

/**
 * @brief Schedule process for cleanup
 * @param Process Process to cleanup
 */
static VOID PsScheduleProcessCleanup(PPROCESS_CONTROL_BLOCK Process)
{
    // This is a simplified implementation
    // In a real implementation, this would:
    // - Add process to cleanup queue
    // - Signal cleanup thread
    // - Clean up resources asynchronously

    // For now, clean up immediately
    PsCleanupProcess(Process);
}

/**
 * @brief Schedule thread for cleanup
 * @param Thread Thread to cleanup
 */
static VOID PsScheduleThreadCleanup(PTHREAD_CONTROL_BLOCK Thread)
{
    // This is a simplified implementation
    // In a real implementation, this would:
    // - Add thread to cleanup queue
    // - Signal cleanup thread

    // For now, clean up immediately
    PsCleanupThread(Thread);
}

/**
 * @brief Clean up process resources
 * @param Process Process to cleanup
 */
static VOID PsCleanupProcess(PPROCESS_CONTROL_BLOCK Process)
{
    if (Process == NULL) {
        return;
    }

    // Clean up address space
    MmDestroyAddressSpace(Process);

    // Clean up security token
    if (Process->SecurityToken != NULL) {
        ExFreePool(Process->SecurityToken);
        Process->SecurityToken = NULL;
    }

    // Dereference process object
    ObDereferenceObject(&Process->Header);
}

/**
 * @brief Clean up thread resources
 * @param Thread Thread to cleanup
 */
static VOID PsCleanupThread(PTHREAD_CONTROL_BLOCK Thread)
{
    if (Thread == NULL) {
        return;
    }

    // Free thread stack
    PsFreeThreadStack(Thread);

    // Free TLS array
    if (Thread->TlsArray != NULL) {
        ExFreePool(Thread->TlsArray);
        Thread->TlsArray = NULL;
    }

    // Dereference thread object
    ObDereferenceObject(&Thread->Header);
}

/**
 * @brief Get process by ID
 * @param ProcessId Process ID to find
 * @return Process control block or NULL
 */
PPROCESS_CONTROL_BLOCK PsGetProcessById(PROCESS_ID ProcessId)
{
    KIRQL old_irql;
    KeAcquireSpinLock(&g_ProcessManager.ProcessLock, &old_irql);

    PLIST_ENTRY entry = g_ProcessManager.ProcessListHead.Flink;
    while (entry != &g_ProcessManager.ProcessListHead) {
        PPROCESS_CONTROL_BLOCK process = CONTAINING_RECORD(entry, PROCESS_CONTROL_BLOCK, ProcessListEntry);
        if (process->ProcessId == ProcessId) {
            // Reference the process to prevent it from being deleted
            ObReferenceObject(&process->Header);
            KeReleaseSpinLock(&g_ProcessManager.ProcessLock, old_irql);
            return process;
        }
        entry = entry->Flink;
    }

    KeReleaseSpinLock(&g_ProcessManager.ProcessLock, old_irql);
    return NULL;
}

/**
 * @brief Get thread by ID
 * @param ThreadId Thread ID to find
 * @return Thread control block or NULL
 */
PTHREAD_CONTROL_BLOCK PsGetThreadById(THREAD_ID ThreadId)
{
    KIRQL old_irql;
    KeAcquireSpinLock(&g_ProcessManager.ThreadLock, &old_irql);

    PLIST_ENTRY entry = g_ProcessManager.ThreadListHead.Flink;
    while (entry != &g_ProcessManager.ThreadListHead) {
        PTHREAD_CONTROL_BLOCK thread = CONTAINING_RECORD(entry, THREAD_CONTROL_BLOCK, ThreadListEntry);
        if (thread->ThreadId == ThreadId) {
            // Reference the thread to prevent it from being deleted
            ObReferenceObject(&thread->Header);
            KeReleaseSpinLock(&g_ProcessManager.ThreadLock, old_irql);
            return thread;
        }
        entry = entry->Flink;
    }

    KeReleaseSpinLock(&g_ProcessManager.ThreadLock, old_irql);
    return NULL;
}

/**
 * @brief Get current process
 * @return Current process control block
 */
PPROCESS_CONTROL_BLOCK PsGetCurrentProcess(VOID)
{
    // This is a simplified implementation
    // In a real implementation, this would:
    // - Get current thread from CPU context
    // - Return thread's process

    return g_ProcessManager.SystemProcess; // For now, return system process
}

/**
 * @brief Get current thread
 * @return Current thread control block
 */
PTHREAD_CONTROL_BLOCK PsGetCurrentThread(VOID)
{
    // This is a simplified implementation
    // In a real implementation, this would:
    // - Get current thread from CPU context
    // - Return thread control block

    // For now, return first thread from system process
    if (!IsListEmpty(&g_ProcessManager.SystemProcess->ThreadListHead)) {
        PLIST_ENTRY entry = g_ProcessManager.SystemProcess->ThreadListHead.Flink;
        PTHREAD_CONTROL_BLOCK thread = CONTAINING_RECORD(entry, THREAD_CONTROL_BLOCK, ThreadListEntry);
        return thread;
    }

    return NULL;
}

/**
 * @brief Get process statistics
 * @param Statistics Statistics structure to fill
 */
VOID PsGetProcessStatistics(PPROCESS_STATISTICS Statistics)
{
    if (Statistics == NULL) {
        return;
    }

    KIRQL old_irql;
    KeAcquireSpinLock(&g_ProcessManager.ProcessLock, &old_irql);
    RtlCopyMemory(Statistics, &g_ProcessManager.Statistics, sizeof(PROCESS_STATISTICS));
    KeReleaseSpinLock(&g_ProcessManager.ProcessLock, old_irql);
}

/**
 * @brief Initialize thread manager
 * @return NTSTATUS Status code
 */
NTSTATUS PsInitializeThreadManager(VOID)
{
    // Thread manager is initialized as part of process manager
    return STATUS_SUCCESS;
}