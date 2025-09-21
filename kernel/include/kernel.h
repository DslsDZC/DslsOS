/**
 * @file kernel.h
 * @brief DslsOS Kernel definitions and interfaces
 * @author DslsOS Team
 * @version 1.0
 * @date 2024
 */

#ifndef DSLOS_KERNEL_H
#define DSLOS_KERNEL_H

#include "../include/dslos.h"

// Kernel service types
typedef enum {
    KERNEL_SERVICE_PROCESS = 1,   // Process management service
    KERNEL_SERVICE_MEMORY,        // Memory management service
    KERNEL_SERVICE_IPC,           // IPC service
    KERNEL_SERVICE_SCHEDULER,     // Scheduler service
    KERNEL_SERVICE_DEVICE,        // Device management service
    KERNEL_SERVICE_SECURITY,      // Security service
    KERNEL_SERVICE_MAX
} KERNEL_SERVICE_TYPE;

// Kernel object types
typedef enum {
    KERNEL_OBJECT_TYPE_PROCESS = 1,
    KERNEL_OBJECT_TYPE_THREAD,
    KERNEL_OBJECT_TYPE_FILE,
    KERNEL_OBJECT_TYPE_DEVICE,
    KERNEL_OBJECT_TYPE_PORT,
    KERNEL_OBJECT_TYPE_EVENT,
    KERNEL_OBJECT_TYPE_MUTEX,
    KERNEL_OBJECT_TYPE_SEMAPHORE,
    KERNEL_OBJECT_TYPE_TIMER,
    KERNEL_OBJECT_TYPE_MAX
} KERNEL_OBJECT_TYPE;

// Kernel object base structure
typedef struct _KERNEL_OBJECT {
    ULONG ObjectType;             // Object type
    ULONG ReferenceCount;         // Reference count
    ULONG Flags;                  // Object flags
    PVOID SecurityDescriptor;     // Security descriptor
    LIST_ENTRY ObjectListEntry;   // Object list entry
} KERNEL_OBJECT, *PKERNEL_OBJECT;

// Process state
typedef enum {
    PROCESS_STATE_CREATED = 1,
    PROCESS_STATE_READY,
    PROCESS_STATE_RUNNING,
    PROCESS_STATE_WAITING,
    PROCESS_STATE_TERMINATED,
    PROCESS_STATE_ZOMBIE
} PROCESS_STATE;

// Thread state
typedef enum {
    THREAD_STATE_CREATED = 1,
    THREAD_STATE_READY,
    THREAD_STATE_RUNNING,
    THREAD_STATE_WAITING,
    THREAD_STATE_TERMINATED
} THREAD_STATE;

// Wait reason
typedef enum {
    WAIT_REASON_EXECUTIVE = 0,
    WAIT_REASON_FREE_PAGE,
    WAIT_REASON_PAGE_IN,
    WAIT_REASON_POOL_ALLOCATION,
    WAIT_REASON_DELAY_EXECUTION,
    WAIT_REASON_SUSPENDED,
    WAIT_REASON_USER_REQUEST,
    WAIT_REASON_WRITER_REQUEST,
    WAIT_REASON_KERNEL_REQUEST,
    WAIT_REASON_ALERT,
    WAIT_REASON_NETWORK_IO,
    WAIT_REASON_PAGE_FAULT,
    WAIT_REASON_VSUSPEND,
    WAIT_REASON_SYSTEM
} WAIT_REASON;

// Security token
typedef struct _SECURITY_TOKEN {
    ULONG TokenId;
    ULONG PrivilegeLevel;
    ULONG Capabilities;
    ULONG SessionId;
    PVOID TokenData;
} SECURITY_TOKEN, *PSECURITY_TOKEN;

// Process Control Block (PCB)
typedef struct _PROCESS_CONTROL_BLOCK {
    KERNEL_OBJECT Header;          // Kernel object header

    // Identification
    PROCESS_ID ProcessId;          // Process ID
    PROCESS_ID ParentProcessId;    // Parent process ID
    ULONG SessionId;               // Session ID

    // Memory management
    PVOID PageDirectory;           // Page directory
    PVOID AddressSpace;            // Address space
    SIZE_T TotalMemory;            // Total memory allocated
    SIZE_T PeakMemory;             // Peak memory usage

    // Resource limits
    ULONGLONG CpuTimeLimit;        // CPU time limit
    SIZE_T MemoryLimit;            // Memory limit
    ULONG HandleLimit;             // Handle limit

    // Security context
    PSECURITY_TOKEN SecurityToken; // Security token
    ULONG PrivilegeLevel;          // Privilege level

    // Statistics
    LARGE_INTEGER CreateTime;      // Create time
    LARGE_INTEGER ExitTime;        // Exit time
    ULONG ExitStatus;              // Exit status
    ULONG HandleCount;             // Handle count
    ULONG ThreadCount;             // Thread count

    // State management
    volatile PROCESS_STATE State;  // Process state
    LIST_ENTRY ProcessListEntry;   // Process list entry
    LIST_ENTRY ThreadListHead;     // Thread list head
} PROCESS_CONTROL_BLOCK, *PPROCESS_CONTROL_BLOCK;

// Thread Control Block (TCB)
typedef struct _THREAD_CONTROL_BLOCK {
    KERNEL_OBJECT Header;          // Kernel object header

    // Identification
    THREAD_ID ThreadId;            // Thread ID
    PPROCESS_CONTROL_BLOCK Process; // Owner process

    // Execution context
    PVOID KernelStack;             // Kernel stack
    PVOID UserStack;               // User stack
    PVOID StackBase;               // Stack base
    PVOID StackLimit;              // Stack limit
    PVOID InstructionPointer;      // Instruction pointer

    // Scheduling
    volatile LONG Priority;         // Thread priority
    volatile LONG BasePriority;     // Base priority
    ULONG CpuAffinity;             // CPU affinity
    volatile THREAD_STATE State;    // Thread state
    WAIT_REASON WaitReason;        // Wait reason
    PVOID WaitObject;              // Wait object
    ULONG WaitTime;                // Wait time

    // Statistics
    LARGE_INTEGER CreateTime;      // Create time
    LARGE_INTEGER KernelTime;      // Kernel time
    LARGE_INTEGER UserTime;        // User time
    ULONG ContextSwitchCount;      // Context switch count

    // TLS
    PVOID TlsArray;                // TLS array
    ULONG TlsSize;                 // TLS size

    // List management
    LIST_ENTRY ThreadListEntry;    // Thread list entry
    LIST_ENTRY ReadyListEntry;     // Ready list entry
    LIST_ENTRY WaitListEntry;     // Wait list entry
} THREAD_CONTROL_BLOCK, *PTHREAD_CONTROL_BLOCK;

// System call numbers
#define SYSCALL_PROCESS_CREATE   1
#define SYSCALL_PROCESS_TERMINATE 2
#define SYSCALL_MEMORY_ALLOCATE  3
#define SYSCALL_MEMORY_FREE      4
#define SYSCALL_IPC_SEND         5
#define SYSCALL_IPC_RECEIVE      6
#define SYSCALL_DEVICE_IOCTL     7
#define SYSCALL_THREAD_CREATE    8
#define SYSCALL_THREAD_TERMINATE 9
#define SYSCALL_THREAD_SUSPEND   10
#define SYSCALL_THREAD_RESUME    11
#define SYSCALL_MAX              11

// System call entry
typedef NTSTATUS (*SYSCALL_ENTRY)(PVOID Parameters, ULONG ParameterLength);

// Kernel initialization
NTSTATUS KiInitializeKernel(VOID);
VOID KiInitializeHardware(VOID);
VOID KiInitializeSystemServices(VOID);
VOID KiStartScheduler(VOID);

// Process management
NTSTATUS PsCreateProcess(PPROCESS_CONTROL_BLOCK* Process, PCSTR ImageName, PPROCESS_CONTROL_BLOCK Parent);
NTSTATUS PsTerminateProcess(PPROCESS_CONTROL_BLOCK Process, NTSTATUS ExitStatus);
NTSTATUS PsCreateThread(PPROCESS_CONTROL_BLOCK Process, PTHREAD_CONTROL_BLOCK* Thread, PVOID StartRoutine, PVOID Parameter);
NTSTATUS PsTerminateThread(PTHREAD_CONTROL_BLOCK Thread, NTSTATUS ExitStatus);

// Memory management
NTSTATUS MmInitializeMemoryManager(VOID);
PVOID MmAllocatePhysicalMemory(SIZE_T Size);
VOID MmFreePhysicalMemory(PVOID Address, SIZE_T Size);
NTSTATUS MmCreateAddressSpace(PPROCESS_CONTROL_BLOCK Process);
NTSTATUS MmDestroyAddressSpace(PPROCESS_CONTROL_BLOCK Process);
PVOID MmAllocateVirtualMemory(PPROCESS_CONTROL_BLOCK Process, PVOID BaseAddress, SIZE_T Size, ULONG Protect);
VOID MmFreeVirtualMemory(PPROCESS_CONTROL_BLOCK Process, PVOID Address, SIZE_T Size);

// Thread scheduling
VOID KeInitializeScheduler(VOID);
VOID KeSchedule(VOID);
NTSTATUS KeAddThreadToReadyQueue(PTHREAD_CONTROL_BLOCK Thread);
VOID KeRemoveThreadFromReadyQueue(PTHREAD_CONTROL_BLOCK Thread);
VOID KeSwitchContext(PTHREAD_CONTROL_BLOCK NewThread);
VOID KeUpdateThreadTimes(VOID);

// IPC management
NTSTATUS IpcInitializeIpc(VOID);
NTSTATUS IpcCreatePort(PHANDLE PortHandle, ULONG MaxConnections);
NTSTATUS IpcConnectPort(HANDLE PortHandle, HANDLE ServerPort);
NTSTATUS IpcSendRequest(HANDLE PortHandle, PVOID Request, SIZE_T RequestSize, PVOID* Reply, SIZE_T* ReplySize);
NTSTATUS IpcReceiveRequest(HANDLE PortHandle, PVOID* Request, SIZE_T* RequestSize);
NTSTATUS IpcSendReply(HANDLE PortHandle, PVOID Reply, SIZE_T ReplySize);

// Object management
NTSTATUS ObInitializeObjectManager(VOID);
NTSTATUS ObCreateObject(KERNEL_OBJECT_TYPE Type, SIZE_T ObjectSize, PKERNEL_OBJECT* Object);
VOID ObReferenceObject(PKERNEL_OBJECT Object);
VOID ObDereferenceObject(PKERNEL_OBJECT Object);
NTSTATUS ObGetObjectByName(PUNICODE_STRING Name, PKERNEL_OBJECT* Object);

// Security management
NTSTATUS SeInitializeSecurity(VOID);
NTSTATUS SeCreateToken(PSECURITY_TOKEN* Token, ULONG PrivilegeLevel);
NTSTATUS SeCheckAccess(PSECURITY_TOKEN Token, ACCESS_MASK DesiredAccess, PKERNEL_OBJECT Object);
NTSTATUS SeImpersonateClient(PSECURITY_TOKEN ClientToken);

// Hardware abstraction
VOID HalInitializeProcessor(VOID);
VOID HalInitializeInterrupts(VOID);
VOID HalInitializeTimers(VOID);
VOID HalDisableInterrupts(VOID);
VOID HalEnableInterrupts(VOID);
VOID HalHaltSystem(VOID);

#endif // DSLOS_KERNEL_H