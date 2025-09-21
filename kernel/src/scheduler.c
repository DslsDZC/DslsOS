/**
 * @file scheduler.c
 * @brief Thread scheduler implementation
 * @author DslsOS Team
 * @version 1.0
 * @date 2024
 */

#include "../include/kernel.h"
#include "../include/dslos.h"

// Scheduler state
typedef struct _SCHEDULER_STATE {
    BOOLEAN Initialized;
    BOOLEAN Running;
    KSPIN_LOCK SchedulerLock;

    // Ready queues
    LIST_ENTRY ReadyQueues[32];    // Priority levels 0-31
    ULONG ReadyThreadCounts[32];
    PTHREAD_CONTROL_BLOCK CurrentThread[64]; // Per CPU current thread
    ULONG CurrentProcessor;

    // Idle threads
    PTHREAD_CONTROL_BLOCK IdleThreads[64];

    // Scheduler statistics
    SCHEDULER_STATISTICS Statistics;

    // Time quantum
    ULONG TimeQuantum;
    ULONG QuantumRemaining;

    // Load balancing
    BOOLEAN LoadBalancingEnabled;
    ULONG LoadBalanceInterval;
    ULONG LastLoadBalanceTime;

    // Preemption
    BOOLEAN PreemptionEnabled;
    ULONG PreemptionThreshold;

    // Affinity
    ULONG DefaultAffinity;
} SCHEDULER_STATE;

static SCHEDULER_STATE g_Scheduler = {0};

// Scheduler statistics structure
typedef struct _SCHEDULER_STATISTICS {
    ULONG ContextSwitches;
    ULONG ThreadSwitches;
    ULONG IdleSwitches;
    ULONG Preemptions;
    ULONG LoadBalanceOperations;
    LARGE_INTEGER TotalCpuTime;
    LARGE_INTEGER IdleTime;
} SCHEDULER_STATISTICS, *PSCHEDULER_STATISTICS;

// Priority levels
#define PRIORITY_IDLE              0
#define PRIORITY_LOWEST            1
#define PRIORITY_BELOW_NORMAL      6
#define PRIORITY_NORMAL            8
#define PRIORITY_ABOVE_NORMAL      10
#define PRIORITY_HIGHEST           15
#define PRIORITY_REALTIME          24
#define PRIORITY_CRITICAL          31

// Time quantum (milliseconds)
#define DEFAULT_TIME_QUANTUM       10

/**
 * @brief Initialize scheduler
 * @return NTSTATUS Status code
 */
NTSTATUS KeInitializeScheduler(VOID)
{
    if (g_Scheduler.Initialized) {
        return STATUS_SUCCESS;
    }

    KeInitializeSpinLock(&g_Scheduler.SchedulerLock);
    g_Scheduler.Running = FALSE;
    g_Scheduler.CurrentProcessor = 0;
    g_Scheduler.TimeQuantum = DEFAULT_TIME_QUANTUM;
    g_Scheduler.QuantumRemaining = DEFAULT_TIME_QUANTUM;
    g_Scheduler.LoadBalancingEnabled = TRUE;
    g_Scheduler.LoadBalanceInterval = 1000; // 1 second
    g_Scheduler.LastLoadBalanceTime = 0;
    g_Scheduler.PreemptionEnabled = TRUE;
    g_Scheduler.PreemptionThreshold = 5;
    g_Scheduler.DefaultAffinity = 0xFFFFFFFF; // All CPUs

    // Initialize ready queues
    for (ULONG i = 0; i < 32; i++) {
        InitializeListHead(&g_Scheduler.ReadyQueues[i]);
        g_Scheduler.ReadyThreadCounts[i] = 0;
    }

    // Initialize current thread array
    for (ULONG i = 0; i < 64; i++) {
        g_Scheduler.CurrentThread[i] = NULL;
    }

    // Initialize idle threads
    for (ULONG i = 0; i < 64; i++) {
        g_Scheduler.IdleThreads[i] = NULL;
    }

    // Initialize statistics
    RtlZeroMemory(&g_Scheduler.Statistics, sizeof(SCHEDULER_STATISTICS));

    g_Scheduler.Initialized = TRUE;
    return STATUS_SUCCESS;
}

/**
 * @brief Start scheduler
 */
VOID KeStartScheduler(VOID)
{
    if (!g_Scheduler.Initialized) {
        return;
    }

    g_Scheduler.Running = TRUE;

    // Create idle threads for each processor
    for (ULONG i = 0; i < g_Scheduler.SystemInfo.dwNumberOfProcessors; i++) {
        KeCreateIdleThread(i);
    }

    // Start scheduling on current processor
    KeSchedule();
}

/**
 * @brief Main scheduler function
 */
VOID KeSchedule(VOID)
{
    if (!g_Scheduler.Running) {
        return;
    }

    KIRQL old_irql;
    KeAcquireSpinLock(&g_Scheduler.SchedulerLock, &old_irql);

    // Get current processor
    ULONG current_cpu = KeGetCurrentProcessorNumber();

    // Get current thread
    PTHREAD_CONTROL_BLOCK current_thread = g_Scheduler.CurrentThread[current_cpu];
    PTHREAD_CONTROL_BLOCK next_thread = NULL;

    // Find next thread to run
    next_thread = KeFindNextThread(current_cpu);

    if (next_thread != current_thread) {
        // Switch to new thread
        KeSwitchContext(next_thread);
    }

    KeReleaseSpinLock(&g_Scheduler.SchedulerLock, old_irql);
}

/**
 * @brief Find next thread to run
 * @param CurrentProcessor Current processor number
 * @return Next thread to run
 */
static PTHREAD_CONTROL_BLOCK KeFindNextThread(ULONG CurrentProcessor)
{
    // Find highest priority ready thread
    for (LONG priority = 31; priority >= 0; priority--) {
        if (!IsListEmpty(&g_Scheduler.ReadyQueues[priority])) {
            PLIST_ENTRY entry = g_Scheduler.ReadyQueues[priority].Flink;
            PTHREAD_CONTROL_BLOCK thread = CONTAINING_RECORD(entry, THREAD_CONTROL_BLOCK, ReadyListEntry);

            // Check thread affinity
            if (thread->CpuAffinity == 0 || (thread->CpuAffinity & (1ULL << CurrentProcessor))) {
                // Remove from ready queue
                RemoveEntryList(&thread->ReadyListEntry);
                g_Scheduler.ReadyThreadCounts[priority]--;

                // Set thread state to running
                thread->State = THREAD_STATE_RUNNING;

                return thread;
            }
        }
    }

    // No ready threads, run idle thread
    return g_Scheduler.IdleThreads[CurrentProcessor];
}

/**
 * @brief Switch to new thread context
 * @param NewThread New thread to switch to
 */
VOID KeSwitchContext(PTHREAD_CONTROL_BLOCK NewThread)
{
    ULONG current_cpu = KeGetCurrentProcessorNumber();
    PTHREAD_CONTROL_BLOCK current_thread = g_Scheduler.CurrentThread[current_cpu];

    if (current_thread == NewThread) {
        return; // No switch needed
    }

    // Update statistics
    InterlockedIncrement(&g_Scheduler.Statistics.ContextSwitches);

    if (current_thread != NULL) {
        // Save current thread context
        KeSaveThreadContext(current_thread);

        // Update thread state
        if (current_thread->State == THREAD_STATE_RUNNING) {
            current_thread->State = THREAD_STATE_READY;
            KeAddThreadToReadyQueue(current_thread);
        }

        InterlockedIncrement(&g_Scheduler.Statistics.ThreadSwitches);
    } else {
        InterlockedIncrement(&g_Scheduler.Statistics.IdleSwitches);
    }

    // Set new current thread
    g_Scheduler.CurrentThread[current_cpu] = NewThread;

    // Restore new thread context
    KeRestoreThreadContext(NewThread);

    // Reset time quantum
    g_Scheduler.QuantumRemaining = g_Scheduler.TimeQuantum;

    // Update thread CPU affinity tracking
    NewThread->CpuAffinity = 1ULL << current_cpu;
}

/**
 * @brief Add thread to ready queue
 * @param Thread Thread to add
 * @return NTSTATUS Status code
 */
NTSTATUS KeAddThreadToReadyQueue(PTHREAD_CONTROL_BLOCK Thread)
{
    if (Thread == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    KIRQL old_irql;
    KeAcquireSpinLock(&g_Scheduler.SchedulerLock, &old_irql);

    // Validate thread priority
    if (Thread->Priority < 0 || Thread->Priority >= 32) {
        Thread->Priority = PRIORITY_NORMAL;
    }

    // Add to appropriate ready queue
    InsertTailList(&g_Scheduler.ReadyQueues[Thread->Priority], &Thread->ReadyListEntry);
    g_Scheduler.ReadyThreadCounts[Thread->Priority]++;

    // Set thread state to ready
    Thread->State = THREAD_STATE_READY;

    KeReleaseSpinLock(&g_Scheduler.SchedulerLock, old_irql);

    return STATUS_SUCCESS;
}

/**
 * @brief Remove thread from ready queue
 * @param Thread Thread to remove
 */
VOID KeRemoveThreadFromReadyQueue(PTHREAD_CONTROL_BLOCK Thread)
{
    if (Thread == NULL) {
        return;
    }

    KIRQL old_irql;
    KeAcquireSpinLock(&g_Scheduler.Scheduler.Lock, &old_irql);

    // Remove from ready queue if present
    if (!IsListEmpty(&Thread->ReadyListEntry)) {
        RemoveEntryList(&Thread->ReadyListEntry);
        if (Thread->Priority < 32) {
            g_Scheduler.ReadyThreadCounts[Thread->Priority]--;
        }
    }

    KeReleaseSpinLock(&g_Scheduler.SchedulerLock, old_irql);
}

/**
 * @brief Create idle thread
 * @param Processor Processor number
 */
VOID KeCreateIdleThread(ULONG Processor)
{
    // Create idle thread for specified processor
    PTHREAD_CONTROL_BLOCK idle_thread = ExAllocatePool(NonPagedPool, sizeof(THREAD_CONTROL_BLOCK));
    if (idle_thread == NULL) {
        return;
    }

    RtlZeroMemory(idle_thread, sizeof(THREAD_CONTROL_BLOCK));

    // Initialize idle thread
    idle_thread->Header.ObjectType = KERNEL_OBJECT_TYPE_THREAD;
    idle_thread->Header.ReferenceCount = 1;
    idle_thread->ThreadId = (THREAD_ID)(0x1000 + Processor); // Special ID range for idle threads
    idle_thread->Process = g_ProcessManager.IdleProcess;
    idle_thread->Priority = PRIORITY_IDLE;
    idle_thread->BasePriority = PRIORITY_IDLE;
    idle_thread->State = THREAD_STATE_READY;
    idle_thread->CpuAffinity = 1ULL << Processor;

    InitializeListHead(&idle_thread->Header.ObjectListEntry);
    InitializeListHead(&idle_thread->ThreadListEntry);
    InitializeListHead(&idle_thread->ReadyListEntry);
    InitializeListHead(&idle_thread->WaitListEntry);

    // Set creation time
    KeQuerySystemTime(&idle_thread->CreateTime);

    // Set as current thread for this processor
    g_Scheduler.CurrentThread[Processor] = idle_thread;
    g_Scheduler.IdleThreads[Processor] = idle_thread;

    // Add to process thread list
    KIRQL old_irql;
    KeAcquireSpinLock(&g_ProcessManager.ProcessLock, &old_irql);
    InsertTailList(&g_ProcessManager.IdleProcess->ThreadListHead, &idle_thread->ThreadListEntry);
    g_ProcessManager.IdleProcess->ThreadCount++;
    KeReleaseSpinLock(&g_ProcessManager.ProcessLock, old_irql);
}

/**
 * @brief Save thread context
 * @param Thread Thread to save context for
 */
VOID KeSaveThreadContext(PTHREAD_CONTROL_BLOCK Thread)
{
    // This is a simplified implementation
    // In a real implementation, this would:
    // - Save CPU registers
    // - Save FPU state
    // - Save segment registers
    // - Update thread statistics

    UNREFERENCED_PARAMETER(Thread);
}

/**
 * @brief Restore thread context
 * @param Thread Thread to restore context for
 */
VOID KeRestoreThreadContext(PTHREAD_CONTROL_BLOCK Thread)
{
    // This is a simplified implementation
    // In a real implementation, this would:
    // - Restore CPU registers
    // - Restore FPU state
    // - Restore segment registers
    // - Load page directory if process changed

    UNREFERENCED_PARAMETER(Thread);
}

/**
 * @brief Update thread times
 */
VOID KeUpdateThreadTimes(VOID)
{
    ULONG current_cpu = KeGetCurrentProcessorNumber();
    PTHREAD_CONTROL_BLOCK current_thread = g_Scheduler.CurrentThread[current_cpu];

    if (current_thread == NULL || current_thread == g_Scheduler.IdleThreads[current_cpu]) {
        return;
    }

    // Update thread execution times
    LARGE_INTEGER current_time;
    KeQuerySystemTime(&current_time);

    // This is a simplified implementation
    // In a real implementation, this would:
    // - Calculate time since last update
    // - Add to kernel or user time based on execution mode
    // - Update process times

    current_thread->ContextSwitchCount++;
}

/**
 * @brief Preempt current thread
 */
VOID KePreemptCurrentThread(VOID)
{
    if (!g_Scheduler.PreemptionEnabled) {
        return;
    }

    ULONG current_cpu = KeGetCurrentProcessorNumber();
    PTHREAD_CONTROL_BLOCK current_thread = g_Scheduler.CurrentThread[current_cpu];

    if (current_thread == NULL || current_thread == g_Scheduler.IdleThreads[current_cpu]) {
        return;
    }

    // Check if preemption threshold is met
    if (g_Scheduler.QuantumRemaining <= g_Scheduler.PreemptionThreshold) {
        // Trigger context switch
        KeSchedule();
    }
}

/**
 * @brief Set thread priority
 * @param Thread Thread to set priority for
 * @param Priority New priority
 * @return NTSTATUS Status code
 */
NTSTATUS KeSetThreadPriority(PTHREAD_CONTROL_BLOCK Thread, LONG Priority)
{
    if (Thread == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    // Validate priority
    if (Priority < PRIORITY_IDLE || Priority > PRIORITY_CRITICAL) {
        return STATUS_INVALID_PARAMETER;
    }

    KIRQL old_irql;
    KeAcquireSpinLock(&g_Scheduler.SchedulerLock, &old_irql);

    LONG old_priority = Thread->Priority;
    Thread->Priority = Priority;

    // If thread is in ready queue and priority changed, move it
    if (Thread->State == THREAD_STATE_READY && old_priority != Priority) {
        RemoveEntryList(&Thread->ReadyListEntry);
        g_Scheduler.ReadyThreadCounts[old_priority]--;
        InsertTailList(&g_Scheduler.ReadyQueues[Priority], &Thread->ReadyListEntry);
        g_Scheduler.ReadyThreadCounts[Priority]++;
    }

    KeReleaseSpinLock(&g_Scheduler.SchedulerLock, old_irql);

    return STATUS_SUCCESS;
}

/**
 * @brief Boost thread priority temporarily
 * @param Thread Thread to boost
 * @param BoostAmount Priority boost amount
 */
VOID KeBoostThreadPriority(PTHREAD_CONTROL_BLOCK Thread, LONG BoostAmount)
{
    if (Thread == NULL || BoostAmount <= 0) {
        return;
    }

    LONG new_priority = Thread->Priority + BoostAmount;
    if (new_priority > PRIORITY_CRITICAL) {
        new_priority = PRIORITY_CRITICAL;
    }

    KeSetThreadPriority(Thread, new_priority);
}

/**
 * @brief Handle timer interrupt
 */
VOID KeHandleTimerInterrupt(VOID)
{
    if (!g_Scheduler.Running) {
        return;
    }

    // Decrement quantum
    if (g_Scheduler.QuantumRemaining > 0) {
        g_Scheduler.QuantumRemaining--;
    }

    // Check if quantum expired
    if (g_Scheduler.QuantumRemaining == 0) {
        KePreemptCurrentThread();
    }

    // Update thread times
    KeUpdateThreadTimes();

    // Check if load balancing is needed
    if (g_Scheduler.LoadBalancingEnabled) {
        LARGE_INTEGER current_time;
        KeQuerySystemTime(&current_time);
        if (current_time.QuadPart - g_Scheduler.LastLoadBalanceTime > g_Scheduler.LoadBalanceInterval * 10000LL) {
            KePerformLoadBalancing();
            g_Scheduler.LastLoadBalanceTime = current_time.QuadPart;
        }
    }
}

/**
 * @brief Perform load balancing
 */
VOID KePerformLoadBalancing(VOID)
{
    // This is a simplified implementation
    // In a real implementation, this would:
    // - Analyze CPU load distribution
    // - Migrate threads between processors
    // - Balance ready queue lengths
    // - Consider thread affinity

    InterlockedIncrement(&g_Scheduler.Statistics.LoadBalanceOperations);
}

/**
 * @brief Get scheduler statistics
 * @param Statistics Statistics structure to fill
 */
VOID KeGetSchedulerStatistics(PSCHEDULER_STATISTICS Statistics)
{
    if (Statistics == NULL) {
        return;
    }

    KIRQL old_irql;
    KeAcquireSpinLock(&g_Scheduler.SchedulerLock, &old_irql);
    RtlCopyMemory(Statistics, &g_Scheduler.Statistics, sizeof(SCHEDULER_STATISTICS));
    KeReleaseSpinLock(&g_Scheduler.SchedulerLock, old_irql);
}

/**
 * @brief Set scheduler parameters
 * @param TimeQuantum Time quantum in milliseconds
 * @param PreemptionEnabled Whether preemption is enabled
 * @param LoadBalancingEnabled Whether load balancing is enabled
 */
VOID KeSetSchedulerParameters(ULONG TimeQuantum, BOOLEAN PreemptionEnabled, BOOLEAN LoadBalancingEnabled)
{
    KIRQL old_irql;
    KeAcquireSpinLock(&g_Scheduler.SchedulerLock, &old_irql);

    g_Scheduler.TimeQuantum = TimeQuantum;
    g_Scheduler.QuantumRemaining = TimeQuantum;
    g_Scheduler.PreemptionEnabled = PreemptionEnabled;
    g_Scheduler.LoadBalancingEnabled = LoadBalancingEnabled;

    KeReleaseSpinLock(&g_Scheduler.SchedulerLock, old_irql);
}