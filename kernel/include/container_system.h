/**
 * @file container_system.h
 * @brief Container system interface
 * @author DslsOS Team
 * @version 1.0
 * @date 2024
 */

#ifndef _CONTAINER_SYSTEM_H_
#define _CONTAINER_SYSTEM_H_

#include "dslos.h"
#include "kernel.h"

// Container ID type
typedef ULONG CONTAINER_ID, *PCONTAINER_ID;

// Container types
typedef enum _CONTAINER_TYPE {
    CONTAINER_TYPE_SYSTEM,
    CONTAINER_TYPE_APPLICATION,
    CONTAINER_TYPE_SERVICE,
    CONTAINER_TYPE_MICROSERVICE,
    CONTAINER_TYPE_SANDBOX
} CONTAINER_TYPE, *PCONTAINER_TYPE;

// Container states
typedef enum _CONTAINER_STATE {
    CONTAINER_STATE_CREATED,
    CONTAINER_STATE_INITIALIZING,
    CONTAINER_STATE_RUNNING,
    CONTAINER_STATE_PAUSED,
    CONTAINER_STATE_STOPPING,
    CONTAINER_STATE_STOPPED,
    CONTAINER_STATE_DESTROYING,
    CONTAINER_STATE_DESTROYED
} CONTAINER_STATE, *PCONTAINER_STATE;

// Container capabilities
typedef struct _CONTAINER_CAPABILITIES {
    ULONG Capabilities;
#define CONTAINER_CAP_NETWORK      0x00000001
#define CONTAINER_CAP_FILESYSTEM   0x00000002
#define CONTAINER_CAP_DEVICES      0x00000004
#define CONTAINER_CAP_PROCESS      0x00000008
#define CONTAINER_CAP_MEMORY       0x00000010
#define CONTAINER_CAP_IPC          0x00000020
#define CONTAINER_CAP_SECURITY     0x00000040
#define CONTAINER_CAP_MONITORING   0x00000080
#define CONTAINER_CAP_DEBUG        0x00000100
#define CONTAINER_CAP_ALL          0xFFFFFFFF
} CONTAINER_CAPABILITIES, *PCONTAINER_CAPABILITIES;

// Resource limits
typedef struct _CONTAINER_LIMITS {
    ULONG64 MaxMemory;
    ULONG64 MaxCpuTime;
    ULONG64 MaxDiskSpace;
    ULONG MaxProcesses;
    ULONG MaxThreads;
    ULONG MaxFileDescriptors;
    ULONG MaxNetworkConnections;
    ULONG CpuShares;
    ULONG Priority;
} CONTAINER_LIMITS, *PCONTAINER_LIMITS;

// Container statistics
typedef struct _CONTAINER_STATS {
    ULONG64 MemoryUsage;
    ULONG64 CpuUsage;
    ULONG64 DiskUsage;
    ULONG64 NetworkUsage;
    ULONG ProcessCount;
    ULONG ThreadCount;
    ULONG64 Uptime;
    ULONG64 StartTime;
    ULONG64 BlockIo;
    ULONG64 NetworkIo;
} CONTAINER_STATS, *PCONTAINER_STATS;

// Container information
typedef struct _CONTAINER_INFO {
    CONTAINER_ID ContainerId;
    CONTAINER_TYPE ContainerType;
    CONTAINER_STATE State;
    UNICODE_STRING ContainerName;
    UNICODE_STRING ContainerImage;
    CONTAINER_LIMITS Usage;
    CONTAINER_STATS Statistics;
    LARGE_INTEGER CreationTime;
    ULONG64 Uptime;
} CONTAINER_INFO, *PCONTAINER_INFO;

// Container system statistics
typedef struct _CONTAINER_SYSTEM_STATS {
    ULONG TotalContainers;
    ULONG ActiveContainers;
    ULONG StoppedContainers;
} CONTAINER_SYSTEM_STATS, *PCONTAINER_SYSTEM_STATS;

// Container runtime configuration
typedef struct _CONTAINER_RUNTIME_CONFIG {
    UNICODE_STRING RuntimePath;
    UNICODE_STRING RuntimeArgs;
    UNICODE_STRING WorkingDirectory;
    UNICODE_STRING EnvironmentVariables;
    UNICODE_STRING Command;
    UNICODE_STRING Args;
    BOOLEAN Interactive;
    BOOLEAN Tty;
    BOOLEAN Detached;
    BOOLEAN AutoRemove;
    BOOLEAN Privileged;
} CONTAINER_RUNTIME_CONFIG, *PCONTAINER_RUNTIME_CONFIG;

// Container structure (opaque)
typedef struct _CONTAINER CONTAINER, *PCONTAINER;

// Function prototypes

// Container system initialization
NTSTATUS
NTAPI
CsInitializeContainerSystem(VOID);

// Container lifecycle management
NTSTATUS
NTAPI
CsCreateContainer(
    _In_ PCWSTR ContainerName,
    _In_ PCWSTR ImageName,
    _In_opt_ PCONTAINER_RUNTIME_CONFIG Config,
    _Out_ PCONTAINER_ID ContainerId
);

NTSTATUS
NTAPI
CsStartContainer(
    _In_ CONTAINER_ID ContainerId
);

NTSTATUS
NTAPI
CsStopContainer(
    _In_ CONTAINER_ID ContainerId,
    _In_ BOOLEAN Force
);

NTSTATUS
NTAPI
CsPauseContainer(
    _In_ CONTAINER_ID ContainerId
);

NTSTATUS
NTAPI
CsResumeContainer(
    _In_ CONTAINER_ID ContainerId
);

NTSTATUS
NTAPI
CsDestroyContainer(
    _In_ CONTAINER_ID ContainerId,
    _In_ BOOLEAN Force
);

// Container execution
NTSTATUS
NTAPI
CsExecuteInContainer(
    _In_ CONTAINER_ID ContainerId,
    _In_ PCWSTR Command,
    _In_opt_ PCWSTR Args,
    _Out_opt_ PPROCESS_ID ProcessId
);

// Container information and queries
NTSTATUS
NTAPI
CsGetContainerInfo(
    _In_ CONTAINER_ID ContainerId,
    _Out_ PCONTAINER_INFO Info
);

PCONTAINER
NTAPI
CsFindContainerById(
    _In_ CONTAINER_ID ContainerId
);

PCONTAINER
NTAPI
CsFindContainerByName(
    _In_ PCWSTR ContainerName
);

NTSTATUS
NTAPI
CsEnumerateContainers(
    _Out_writes_bytes_(BufferSize) PCONTAINER_INFO Buffer,
    _In_ ULONG BufferSize,
    _Out_ PULONG Count
);

// Resource management
NTSTATUS
NTAPI
CsSetContainerLimits(
    _In_ CONTAINER_ID ContainerId,
    _In_ PCONTAINER_LIMITS Limits
);

NTSTATUS
NTAPI
CsAddContainerVolume(
    _In_ CONTAINER_ID ContainerId,
    _In_ PCWSTR SourcePath,
    _In_ PCWSTR TargetPath,
    _In_ BOOLEAN ReadOnly
);

NTSTATUS
NTAPI
CsRemoveContainerVolume(
    _In_ CONTAINER_ID ContainerId,
    _In_ PCWSTR VolumeName
);

// Statistics and monitoring
NTSTATUS
NTAPI
CsGetContainerSystemStatistics(
    _Out_ PCONTAINER_SYSTEM_STATS Stats
);

// Status
BOOLEAN
NTAPI
CsIsContainerSystemInitialized(VOID);

#endif // _CONTAINER_SYSTEM_H_