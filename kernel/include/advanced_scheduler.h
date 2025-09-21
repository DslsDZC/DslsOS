/**
 * @file advanced_scheduler.h
 * @brief Advanced task scheduler interface
 * @author DslsOS Team
 * @version 1.0
 * @date 2024
 */

#ifndef _ADVANCED_SCHEDULER_H_
#define _ADVANCED_SCHEDULER_H_

#include "dslos.h"
#include "kernel.h"

// Scheduler constants
#define SCHEDULER_PRIORITY_LEVELS 8
#define SCHEDULER_TIME_SLICE_BASE 10

// Scheduler statistics
typedef struct _SCHEDULER_STATS {
    ULONG64 TotalSchedules;
    ULONG64 ContextSwitches;
    ULONG64 ReadyQueueLength;
    ULONG64 AverageWaitTime;
    ULONG64 StarvationCount;
    ULONG64 LoadBalanceOperations;
} SCHEDULER_STATS, *PSCHEDULER_STATS;

// Scheduler algorithms
typedef enum _SCHEDULER_ALGORITHM {
    SCHED_ALGORITHM_ROUND_ROBIN,
    SCHED_ALGORITHM_PRIORITY,
    SCHED_ALGORITHM_FAIR_SHARE,
    SCHED_ALGORITHM_REAL_TIME,
    SCHED_ALGORITHM_LOAD_BALANCING,
    SCHED_ALGORITHM_ADAPTIVE
} SCHEDULER_ALGORITHM, *PSCHEDULER_ALGORITHM;

// CPU topology
typedef struct _CPU_TOPOLOGY {
    ULONG CpuCount;
    ULONG ActiveCpus;
    ULONG* CpuLoad;
    ULONG* CpuTemperature;
    BOOLEAN* CpuOnline;
} CPU_TOPOLOGY, *PCPU_TOPOLOGY;

// Power modes
typedef enum _POWER_MODE {
    POWER_MODE_PERFORMANCE,
    POWER_MODE_BALANCED,
    POWER_MODE_POWERSAVE
} POWER_MODE, *PPOWER_MODE;

// Function prototypes

// Scheduler initialization
NTSTATUS
NTAPI
KeInitializeAdvancedScheduler(VOID);

NTSTATUS
NTAPI
KeStartScheduler(VOID);

NTSTATUS
NTAPI
KeStopScheduler(VOID);

// Thread management
NTSTATUS
NTAPI
KeAddThreadToScheduler(
    _In_ PTHREAD Thread
);

NTSTATUS
NTAPI
KeRemoveThreadFromScheduler(
    _In_ PTHREAD Thread
);

// Scheduling
PTHREAD
NTAPI
KeScheduleNextThread(VOID);

VOID
NTAPI
KeTimerInterruptHandler(VOID);

// Statistics and configuration
NTSTATUS
NTAPI
KeGetSchedulerStatistics(
    _Out_ PSCHEDULER_STATS Stats
);

NTSTATUS
NTAPI
KeSetSchedulerAlgorithm(
    _In_ SCHEDULER_ALGORITHM Algorithm
);

// Fair share scheduling
NTSTATUS
NTAPI
KeCreateFairShareGroup(
    _In_ PCWSTR GroupName,
    _In_ ULONG Weight,
    _Out_ PGROUP_ID GroupId
);

// Thread affinity
NTSTATUS
NTAPI
KeSetThreadAffinity(
    _In_ PTHREAD Thread,
    _In_ ULONG64 Affinity
);

// CPU topology
NTSTATUS
NTAPI
KeGetCpuTopology(
    _Out_ PCPU_TOPOLOGY Topology
);

VOID
NTAPI
KeUpdateCpuLoad(
    _In_ ULONG CpuId,
    _In_ ULONG Load
);

// Status
BOOLEAN
NTAPI
KeIsAdvancedSchedulerInitialized(VOID);

#endif // _ADVANCED_SCHEDULER_H_