/**
 * DslsOS Thread Manager
 * 基于模块化设计的线程生命周期管理
 * 与进程管理器、调度器、内存管理器等深度集成
 */

#include <kernel/kernel.h>
#include <kernel/thread_manager.h>
#include <kernel/process_manager.h>
#include <kernel/scheduler.h>
#include <kernel/memory_manager.h>
#include <kernel/ipc_manager.h>
#include <kernel/object_manager.h>
#include <kernel/synchronization.h>
#include <kernel/debug.h>

 // 线程管理器全局状态
static THREAD_MANAGER_STATE g_ThreadManager = { 0 };

// 当前线程指针（每个CPU核心一个）
__declspec(thread) PTHREAD_CONTROL_BLOCK g_CurrentThread = NULL;

// 线程ID生成器
static volatile ULONG g_NextThreadId = 1;

// 内部函数声明
static NTSTATUS TmInitializeThreadContext(
    _In_ PTHREAD_CONTROL_BLOCK Thread,
    _In_ PVOID StartAddress,
    _In_ PVOID Parameter
);

static VOID TmCleanupThreadResources(
    _In_ PTHREAD_CONTROL_BLOCK Thread
);

static BOOLEAN TmValidateWaitObject(
    _In_ PVOID WaitObject
);

static NTSTATUS TmReleaseOwnedObjects(
    _In_ PTHREAD_CONTROL_BLOCK Thread
);

static VOID TmUpdateStatistics(
    _In_ THREAD_OPERATION Operation,
    _In_opt_ PTHREAD_CONTROL_BLOCK Thread
);

// 线程管理器初始化
NTSTATUS TmInitialize(VOID)
{
    KIRQL oldIrql;
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_INFO("[TM] Initializing Thread Manager...\n");

    // 初始化全局线程链表
    InitializeListHead(&g_ThreadManager.GlobalThreadList);

    // 初始化自旋锁（复用同步模块）
    KeInitializeSpinLock(&g_ThreadManager.ThreadListLock);

    // 重置计数器
    g_ThreadManager.TotalThreadCount = 0;
    g_ThreadManager.ActiveThreadCount = 0;
    g_ThreadManager.PeakThreadCount = 0;
    g_ThreadManager.TotalContextSwitches = 0;
    g_ThreadManager.TotalTlsAllocations = 0;
    KeQuerySystemTime(&g_ThreadManager.LastResetTime);

    // 初始化状态计数数组
    for (ULONG i = 0; i < THREAD_STATE_MAX; i++) {
        g_ThreadManager.ThreadsInState[i] = 0;
    }

    g_ThreadManager.Initialized = TRUE;

    TRACE_SUCCESS("[TM] Thread Manager initialized successfully\n");
    return status;
}

// 线程管理器清理
VOID TmCleanup(VOID)
{
    KIRQL oldIrql;
    PLIST_ENTRY entry;
    PTHREAD_CONTROL_BLOCK thread;

    TRACE_INFO("[TM] Cleaning up Thread Manager...\n");

    if (!g_ThreadManager.Initialized) {
        return;
    }

    // 获取锁保护全局链表
    KeAcquireSpinLock(&g_ThreadManager.ThreadListLock, &oldIrql);

    // 遍历并清理所有线程（正常情况下应该只有系统线程存在）
    while (!IsListEmpty(&g_ThreadManager.GlobalThreadList)) {
        entry = RemoveHeadList(&g_ThreadManager.GlobalThreadList);
        thread = CONTAINING_RECORD(entry, THREAD_CONTROL_BLOCK, ThreadListEntry);

        // 强制终止线程
        TmTerminateThread(thread);
    }

    KeReleaseSpinLock(&g_ThreadManager.ThreadListLock, oldIrql);

    g_ThreadManager.Initialized = FALSE;
    TRACE_SUCCESS("[TM] Thread Manager cleanup completed\n");
}

// 内部线程创建实现
NTSTATUS TmCreateThreadInternal(
    _In_ PPROCESS_CONTROL_BLOCK Process,
    _In_ PVOID StartAddress,
    _In_ PVOID Parameter,
    _In_ BOOLEAN CreateSuspended,
    _Out_ PTHREAD_CONTROL_BLOCK* Thread
)
{
    NTSTATUS status;
    KIRQL oldIrql;
    PTHREAD_CONTROL_BLOCK newThread = NULL;

    // 参数验证
    if (!Process || !StartAddress || !Thread) {
        return STATUS_INVALID_PARAMETER;
    }

    TRACE_DEBUG("[TM] Creating thread in process %p, start: %p\n",
        Process, StartAddress);

    TM_PERF_START();

    // 分配线程控制块（复用内存管理器）
    newThread = (PTHREAD_CONTROL_BLOCK)ExAllocatePool(
        NonPagedPool,
        sizeof(THREAD_CONTROL_BLOCK)
    );

    if (!newThread) {
        TRACE_ERROR("[TM] Failed to allocate TCB\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // 清零TCB
    RtlZeroMemory(newThread, sizeof(THREAD_CONTROL_BLOCK));

    // 初始化内核对象头部（复用对象管理器）
    newThread->Header.Type = KERNEL_OBJECT_THREAD;
    newThread->Header.ReferenceCount = 1; // 初始引用计数
    newThread->Header.Size = sizeof(THREAD_CONTROL_BLOCK);

    // 分配线程ID
    newThread->ThreadId = InterlockedIncrement(&g_NextThreadId);

    // 设置所属进程
    newThread->Process = Process;

    // 设置优先级（继承进程优先级或使用默认值）
    newThread->BasePriority = Process->BasePriority;
    newThread->Priority = Process->BasePriority;

    // 设置初始状态
    newThread->State = CreateSuspended ? THREAD_STATE_SUSPENDED : THREAD_STATE_READY;
    newThread->WaitReason = WaitReasonNone;

    // 初始化链表项
    InitializeListHead(&newThread->ThreadListEntry);
    InitializeListHead(&newThread->ReadyListEntry);
    InitializeListHead(&newThread->WaitListEntry);
    InitializeListHead(&newThread->OwnedObjectsList);

    // 记录创建时间
    KeQuerySystemTime(&newThread->CreateTime);

    // 分配内核栈（复用内存管理器）
    status = MmAllocateKernelStack(&newThread->KernelStack, KERNEL_STACK_SIZE);
    if (!NT_SUCCESS(status)) {
        TRACE_ERROR("[TM] Failed to allocate kernel stack: 0x%X\n", status);
        ExFreePool(newThread);
        return status;
    }

    // 分配用户栈（如果是在用户进程）
    if (Process->AddressSpace != NULL) {
        status = PsAllocateThreadStack(Process, &newThread->UserStack);
        if (!NT_SUCCESS(status)) {
            TRACE_ERROR("[TM] Failed to allocate user stack: 0x%X\n", status);
            MmFreeKernelStack(newThread->KernelStack);
            ExFreePool(newThread);
            return status;
        }
    }

    // 初始化线程上下文（使用HAL抽象）
    status = TmInitializeThreadContext(newThread, StartAddress, Parameter);
    if (!NT_SUCCESS(status)) {
        TRACE_ERROR("[TM] Failed to initialize thread context: 0x%X\n", status);
        TmCleanupThreadResources(newThread);
        ExFreePool(newThread);
        return status;
    }

    // 将线程添加到进程的线程列表（复用进程管理器）
    KeAcquireSpinLock(&Process->ProcessLock, &oldIrql);
    InsertTailList(&Process->ThreadListHead, &newThread->ThreadListEntry);
    Process->ThreadCount++;
    KeReleaseSpinLock(&Process->ProcessLock, oldIrql);

    // 将线程添加到全局线程列表
    KeAcquireSpinLock(&g_ThreadManager.ThreadListLock, &oldIrql);
    InsertTailList(&g_ThreadManager.GlobalThreadList, &newThread->ThreadListEntry);

    // 更新统计信息
    TmUpdateStatistics(ThreadCreate, newThread);

    KeReleaseSpinLock(&g_ThreadManager.ThreadListLock, oldIrql);

    // 如果不是挂起创建，则加入调度器就绪队列
    if (!CreateSuspended) {
        KeAddThreadToReadyQueue(newThread);
    }

    *Thread = newThread;

    TM_PERF_END("Thread Creation");

    TRACE_SUCCESS("[TM] Thread %u created successfully in process %u\n",
        newThread->ThreadId, Process->ProcessId);

    return STATUS_SUCCESS;
}

// 初始化线程上下文（使用HAL抽象）
static NTSTATUS TmInitializeThreadContext(
    _In_ PTHREAD_CONTROL_BLOCK Thread,
    _In_ PVOID StartAddress,
    _In_ PVOID Parameter
)
{
    BOOLEAN userThread = (Thread->Process->AddressSpace != NULL);

    // 使用HAL接口初始化上下文
    return ArchInitializeThreadContext(Thread, StartAddress, Parameter, userThread);
}

// 终止线程
NTSTATUS TmTerminateThread(
    _In_ PTHREAD_CONTROL_BLOCK Thread
)
{
    KIRQL oldIrql;
    PPROCESS_CONTROL_BLOCK process;

    if (!Thread) {
        return STATUS_INVALID_PARAMETER;
    }

    TRACE_DEBUG("[TM] Terminating thread %u\n", Thread->ThreadId);

    // 释放线程持有的所有同步对象（防止资源泄漏）
    TmReleaseOwnedObjects(Thread);

    // 如果线程正在运行，需要先切换到其他线程
    if (Thread->State == THREAD_STATE_RUNNING) {
        // 从调度器运行队列移除
        KeRemoveThreadFromReadyQueue(Thread);
    }

    // 更新线程状态
    Thread->State = THREAD_STATE_TERMINATED;

    process = Thread->Process;

    // 从进程线程列表中移除
    if (process) {
        KeAcquireSpinLock(&process->ProcessLock, &oldIrql);
        if (!IsListEmpty(&Thread->ThreadListEntry)) {
            RemoveEntryList(&Thread->ThreadListEntry);
            process->ThreadCount--;

            // 如果这是进程的最后一个线程，标记进程为终止
            if (process->ThreadCount == 0 && process->State != PROCESS_STATE_TERMINATED) {
                process->State = PROCESS_STATE_TERMINATED;
                TRACE_INFO("[TM] Last thread terminated, marking process %u as terminated\n",
                    process->ProcessId);
            }
        }
        KeReleaseSpinLock(&process->ProcessLock, oldIrql);
    }

    // 从全局线程列表中移除
    KeAcquireSpinLock(&g_ThreadManager.ThreadListLock, &oldIrql);
    if (!IsListEmpty(&Thread->ThreadListEntry)) {
        RemoveEntryList(&Thread->ThreadListEntry);

        // 更新统计信息
        TmUpdateStatistics(ThreadTerminate, Thread);
    }
    KeReleaseSpinLock(&g_ThreadManager.ThreadListLock, oldIrql);

    // 清理线程资源
    TmCleanupThreadResources(Thread);

    TRACE_SUCCESS("[TM] Thread %u terminated safely\n", Thread->ThreadId);
    return STATUS_SUCCESS;
}

// 清理线程资源
static VOID TmCleanupThreadResources(
    _In_ PTHREAD_CONTROL_BLOCK Thread
)
{
    // 释放内核栈
    if (Thread->KernelStack) {
        MmFreeKernelStack(Thread->KernelStack);
        Thread->KernelStack = NULL;
    }

    // 释放用户栈
    if (Thread->UserStack) {
        PsFreeThreadStack(Thread->Process, Thread->UserStack);
        Thread->UserStack = NULL;
    }

    // 释放TLS
    if (Thread->TlsArray) {
        ExFreePool(Thread->TlsArray);
        Thread->TlsArray = NULL;
        Thread->TlsSize = 0;
        Thread->MaxTlsIndex = 0;
        Thread->LastTlsSearchIndex = 0;
    }

    // 减少对象引用计数（复用对象管理器）
    ObDereferenceObject(&Thread->Header);
}

// 同步对象类型校验
static BOOLEAN TmValidateWaitObject(
    _In_ PVOID WaitObject
)
{
    PKERNEL_OBJECT kernelObj = (PKERNEL_OBJECT)WaitObject;

    if (!WaitObject) {
        return FALSE;
    }

    // 检查对象类型是否为合法的同步对象
    switch (kernelObj->Type) {
    case KERNEL_OBJECT_SEMAPHORE:
    case KERNEL_OBJECT_MUTEX:
    case KERNEL_OBJECT_EVENT:
    case KERNEL_OBJECT_WAIT_BLOCK:
        return TRUE;

    default:
        TRACE_WARNING("[TM] Invalid wait object type: %d\n", kernelObj->Type);
        return FALSE;
    }
}

// 等待单个对象
NTSTATUS TmWaitForSingleObject(
    _In_ PVOID WaitObject,
    _In_ ULONG Timeout
)
{
    KIRQL oldIrql;
    PTHREAD_CONTROL_BLOCK currentThread = g_CurrentThread;

    if (!WaitObject || !currentThread) {
        return STATUS_INVALID_PARAMETER;
    }

    // 校验等待对象类型
    if (!TmValidateWaitObject(WaitObject)) {
        return STATUS_INVALID_OBJECT_TYPE;
    }

    TRACE_DEBUG("[TM] Thread %u waiting for object %p (type: %d)\n",
        currentThread->ThreadId, WaitObject,
        ((PKERNEL_OBJECT)WaitObject)->Type);

    KeAcquireSpinLock(&g_ThreadManager.ThreadListLock, &oldIrql);

    // 记录等待对象到线程的持有对象列表（用于安全终止）
    PKERNEL_OBJECT kernelObj = (PKERNEL_OBJECT)WaitObject;
    InsertTailList(&currentThread->OwnedObjectsList, &kernelObj->OwnedListEntry);
    kernelObj->OwnerThread = currentThread;

    // 更新线程状态
    currentThread->State = THREAD_STATE_WAITING;
    currentThread->WaitObject = WaitObject;
    currentThread->WaitReason = WaitReasonExecutive;

    // 从调度器就绪队列移除
    KeRemoveThreadFromReadyQueue(currentThread);

    // 加入等待队列
    InsertTailList(&((PKWAIT_BLOCK)WaitObject)->WaitList, &currentThread->WaitListEntry);

    // 更新统计信息
    TmUpdateStatistics(ThreadStateChange, currentThread);

    KeReleaseSpinLock(&g_ThreadManager.ThreadListLock, oldIrql);

    // 触发重新调度
    KeSchedule();

    TRACE_DEBUG("[TM] Thread %u resumed from wait\n", currentThread->ThreadId);
    return STATUS_SUCCESS;
}

// 通知对象（唤醒等待线程）
NTSTATUS TmSignalObject(
    _In_ PVOID WaitObject
)
{
    KIRQL oldIrql;
    PLIST_ENTRY entry;
    PTHREAD_CONTROL_BLOCK thread;

    if (!WaitObject) {
        return STATUS_INVALID_PARAMETER;
    }

    // 校验等待对象类型
    if (!TmValidateWaitObject(WaitObject)) {
        return STATUS_INVALID_OBJECT_TYPE;
    }

    TRACE_DEBUG("[TM] Signaling object %p (type: %d)\n",
        WaitObject, ((PKERNEL_OBJECT)WaitObject)->Type);

    KeAcquireSpinLock(&g_ThreadManager.ThreadListLock, &oldIrql);

    // 遍历等待队列，唤醒所有等待该对象的线程
    LIST_ENTRY* waitList = &((PKWAIT_BLOCK)WaitObject)->WaitList;

    while (!IsListEmpty(waitList)) {
        entry = RemoveHeadList(waitList);
        thread = CONTAINING_RECORD(entry, THREAD_CONTROL_BLOCK, WaitListEntry);

        // 从线程的持有对象列表中移除
        PKERNEL_OBJECT kernelObj = (PKERNEL_OBJECT)WaitObject;
        RemoveEntryList(&kernelObj->OwnedListEntry);
        kernelObj->OwnerThread = NULL;

        // 恢复线程状态
        thread->State = THREAD_STATE_READY;
        thread->WaitObject = NULL;
        thread->WaitReason = WaitReasonNone;

        // 加入调度器就绪队列
        KeAddThreadToReadyQueue(thread);

        // 更新统计信息
        TmUpdateStatistics(ThreadStateChange, thread);
    }

    KeReleaseSpinLock(&g_ThreadManager.ThreadListLock, oldIrql);

    // 触发重新调度
    KeSchedule();

    return STATUS_SUCCESS;
}

// 释放线程持有的所有同步对象
static NTSTATUS TmReleaseOwnedObjects(
    _In_ PTHREAD_CONTROL_BLOCK Thread
)
{
    KIRQL oldIrql;
    PLIST_ENTRY entry;
    PKERNEL_OBJECT object;
    NTSTATUS status = STATUS_SUCCESS;

    if (!Thread) {
        return STATUS_INVALID_PARAMETER;
    }

    TRACE_DEBUG("[TM] Releasing owned objects for thread %u\n", Thread->ThreadId);

    KeAcquireSpinLock(&g_ThreadManager.ThreadListLock, &oldIrql);

    // 遍历线程持有的对象列表
    while (!IsListEmpty(&Thread->OwnedObjectsList)) {
        entry = RemoveHeadList(&Thread->OwnedObjectsList);
        object = CONTAINING_RECORD(entry, KERNEL_OBJECT, OwnedListEntry);

        // 根据对象类型进行适当的释放操作
        switch (object->Type) {
        case KERNEL_OBJECT_MUTEX:
            // 对于互斥锁，需要释放并唤醒等待者
            status = KeReleaseMutex((PMUTEX)object, TRUE);
            if (!NT_SUCCESS(status)) {
                TRACE_WARNING("[TM] Failed to release mutex %p: 0x%X\n", object, status);
            }
            break;

        case KERNEL_OBJECT_SEMAPHORE:
            // 信号量不需要特殊处理，等待者会自动处理
            break;

        case KERNEL_OBJECT_EVENT:
            // 事件对象，设置信号状态
            KeSetEvent((PEVENT)object, IO_NO_INCREMENT, FALSE);
            break;

        default:
            TRACE_WARNING("[TM] Unknown object type %d in owned list\n", object->Type);
            break;
        }

        // 清除对象的所有者
        object->OwnerThread = NULL;
    }

    KeReleaseSpinLock(&g_ThreadManager.ThreadListLock, oldIrql);

    return status;
}

// 设置线程状态
NTSTATUS TmSetThreadState(
    _In_ PTHREAD_CONTROL_BLOCK Thread,
    _In_ THREAD_STATE NewState
)
{
    KIRQL oldIrql;
    THREAD_STATE oldState;

    if (!Thread) {
        return STATUS_INVALID_PARAMETER;
    }

    KeAcquireSpinLock(&g_ThreadManager.ThreadListLock, &oldIrql);

    oldState = Thread->State;
    Thread->State = NewState;

    // 更新状态计数
    if (oldState < THREAD_STATE_MAX) {
        g_ThreadManager.ThreadsInState[oldState]--;
    }
    if (NewState < THREAD_STATE_MAX) {
        g_ThreadManager.ThreadsInState[NewState]++;
    }

    KeReleaseSpinLock(&g_ThreadManager.ThreadListLock, oldIrql);

    // 更新统计信息
    TmUpdateStatistics(ThreadStateChange, Thread);

    TRACE_DEBUG("[TM] Thread %u state changed: %d -> %d\n",
        Thread->ThreadId, oldState, NewState);

    return STATUS_SUCCESS;
}

// 获取线程状态
THREAD_STATE TmGetThreadState(
    _In_ PTHREAD_CONTROL_BLOCK Thread
)
{
    THREAD_STATE state;
    KIRQL oldIrql;

    if (!Thread) {
        return THREAD_STATE_INVALID;
    }

    KeAcquireSpinLock(&g_ThreadManager.ThreadListLock, &oldIrql);
    state = Thread->State;
    KeReleaseSpinLock(&g_ThreadManager.ThreadListLock, oldIrql);

    return state;
}

// 优化的TLS分配函数
NTSTATUS TmAllocateTls(
    _In_ PTHREAD_CONTROL_BLOCK Thread,
    _Out_ PULONG TlsIndex
)
{
    ULONG i;
    ULONG currentSlots;
    ULONG newSlots;
    PVOID newArray;

    if (!Thread || !TlsIndex) {
        return STATUS_INVALID_PARAMETER;
    }

    // 如果TLS数组未初始化，则初始化
    if (!Thread->TlsArray) {
        Thread->TlsSize = TLS_INITIAL_SLOTS * TLS_SLOT_SIZE;
        Thread->TlsArray = ExAllocatePool(PagedPool, Thread->TlsSize);
        if (!Thread->TlsArray) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        RtlZeroMemory(Thread->TlsArray, Thread->TlsSize);
        Thread->MaxTlsIndex = 0; // 记录最大有效索引
        Thread->LastTlsSearchIndex = 0;
    }

    currentSlots = Thread->TlsSize / TLS_SLOT_SIZE;

    // 查找空闲槽位（从上次分配位置开始，提高局部性）
    for (i = Thread->LastTlsSearchIndex; i < currentSlots; i++) {
        if (((PVOID*)Thread->TlsArray)[i] == NULL) {
            *TlsIndex = i;
            Thread->LastTlsSearchIndex = i + 1; // 更新搜索起始位置
            Thread->MaxTlsIndex = max(Thread->MaxTlsIndex, i);
            TmUpdateStatistics(ThreadTlsAllocation, Thread);
            return STATUS_SUCCESS;
        }
    }

    // 从头开始查找
    for (i = 0; i < Thread->LastTlsSearchIndex; i++) {
        if (((PVOID*)Thread->TlsArray)[i] == NULL) {
            *TlsIndex = i;
            Thread->LastTlsSearchIndex = i + 1;
            Thread->MaxTlsIndex = max(Thread->MaxTlsIndex, i);
            TmUpdateStatistics(ThreadTlsAllocation, Thread);
            return STATUS_SUCCESS;
        }
    }

    // 没有空闲槽位，按固定步长扩展
    if (currentSlots >= TLS_MAX_SLOTS) {
        TRACE_ERROR("[TM] TLS slots exhausted for thread %u\n", Thread->ThreadId);
        return STATUS_NO_MORE_ENTRIES;
    }

    newSlots = currentSlots + TLS_EXPANSION_STEP;
    if (newSlots > TLS_MAX_SLOTS) {
        newSlots = TLS_MAX_SLOTS;
    }

    newArray = ExAllocatePool(PagedPool, newSlots * TLS_SLOT_SIZE);
    if (!newArray) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // 复制旧数据并初始化新空间
    RtlCopyMemory(newArray, Thread->TlsArray, Thread->TlsSize);
    RtlZeroMemory((PUCHAR)newArray + Thread->TlsSize,
        (newSlots * TLS_SLOT_SIZE) - Thread->TlsSize);

    ExFreePool(Thread->TlsArray);
    Thread->TlsArray = newArray;
    Thread->TlsSize = newSlots * TLS_SLOT_SIZE;

    // 分配第一个新槽位
    *TlsIndex = currentSlots;
    Thread->LastTlsSearchIndex = currentSlots + 1;
    Thread->MaxTlsIndex = currentSlots;
    TmUpdateStatistics(ThreadTlsAllocation, Thread);

    TRACE_DEBUG("[TM] Expanded TLS for thread %u to %u slots\n",
        Thread->ThreadId, newSlots);

    return STATUS_SUCCESS;
}

// 获取TLS值
PVOID TmGetTlsValue(
    _In_ PTHREAD_CONTROL_BLOCK Thread,
    _In_ ULONG TlsIndex
)
{
    if (!Thread || !Thread->TlsArray || TlsIndex > Thread->MaxTlsIndex) {
        return NULL;
    }

    // 直接使用预计算的边界检查，避免除法
    return ((PVOID*)Thread->TlsArray)[TlsIndex];
}

// 设置TLS值
NTSTATUS TmSetTlsValue(
    _In_ PTHREAD_CONTROL_BLOCK Thread,
    _In_ ULONG TlsIndex,
    _In_ PVOID Value
)
{
    if (!Thread || !Thread->TlsArray || TlsIndex > Thread->MaxTlsIndex) {
        return STATUS_INVALID_PARAMETER;
    }

    ((PVOID*)Thread->TlsArray)[TlsIndex] = Value;

    // 更新最大索引（如果设置的值在之前未使用的槽位）
    if (Value != NULL && TlsIndex > Thread->MaxTlsIndex) {
        Thread->MaxTlsIndex = TlsIndex;
    }

    return STATUS_SUCCESS;
}

// 释放TLS槽位
NTSTATUS TmFreeTls(
    _In_ PTHREAD_CONTROL_BLOCK Thread,
    _In_ ULONG TlsIndex
)
{
    if (!Thread || !Thread->TlsArray || TlsIndex > Thread->MaxTlsIndex) {
        return STATUS_INVALID_PARAMETER;
    }

    ((PVOID*)Thread->TlsArray)[TlsIndex] = NULL;

    // 如果释放的是最大索引，需要更新MaxTlsIndex
    if (TlsIndex == Thread->MaxTlsIndex) {
        // 向后查找第一个非空槽位
        while (Thread->MaxTlsIndex > 0 &&
            ((PVOID*)Thread->TlsArray)[Thread->MaxTlsIndex - 1] == NULL) {
            Thread->MaxTlsIndex--;
        }
    }

    // 更新搜索起始位置（优化下次分配）
    if (TlsIndex < Thread->LastTlsSearchIndex) {
        Thread->LastTlsSearchIndex = TlsIndex;
    }

    return STATUS_SUCCESS;
}

// 设置线程优先级
NTSTATUS TmSetThreadPriority(
    _In_ PTHREAD_CONTROL_BLOCK Thread,
    _In_ LONG Priority
)
{
    KIRQL oldIrql;

    if (!Thread || Priority < THREAD_PRIORITY_MIN || Priority > THREAD_PRIORITY_MAX) {
        return STATUS_INVALID_PARAMETER;
    }

    TRACE_DEBUG("[TM] Setting thread %u priority: %d -> %d\n",
        Thread->ThreadId, Thread->BasePriority, Priority);

    KeAcquireSpinLock(&g_ThreadManager.ThreadListLock, &oldIrql);

    Thread->BasePriority = Priority;
    Thread->Priority = Priority; // 简化实现，实际可能有动态调整

    KeReleaseSpinLock(&g_ThreadManager.ThreadListLock, oldIrql);

    // 通知调度器更新优先级
    KeUpdateThreadPriority(Thread);

    return STATUS_SUCCESS;
}

// 根据ID获取线程
NTSTATUS TmGetThreadById(
    _In_ THREAD_ID ThreadId,
    _Out_ PTHREAD_CONTROL_BLOCK* Thread
)
{
    KIRQL oldIrql;
    PLIST_ENTRY entry;
    PTHREAD_CONTROL_BLOCK current;

    if (!Thread) {
        return STATUS_INVALID_PARAMETER;
    }

    *Thread = NULL;

    KeAcquireSpinLock(&g_ThreadManager.ThreadListLock, &oldIrql);

    // 遍历全局线程链表
    LIST_FOR_EACH(entry, &g_ThreadManager.GlobalThreadList) {
        current = CONTAINING_RECORD(entry, THREAD_CONTROL_BLOCK, ThreadListEntry);

        if (current->ThreadId == ThreadId && current->State != THREAD_STATE_TERMINATED) {
            // 增加引用计数
            ObReferenceObject(&current->Header);
            *Thread = current;
            break;
        }
    }

    KeReleaseSpinLock(&g_ThreadManager.ThreadListLock, oldIrql);

    return *Thread ? STATUS_SUCCESS : STATUS_NOT_FOUND;
}

// 枚举进程中的线程
NTSTATUS TmEnumThreads(
    _In_ PPROCESS_CONTROL_BLOCK Process,
    _In_ PENUM_THREADS_CALLBACK Callback,
    _In_ PVOID Context
)
{
    KIRQL oldIrql;
    PLIST_ENTRY entry;
    PTHREAD_CONTROL_BLOCK thread;
    BOOLEAN continueEnum = TRUE;

    if (!Process || !Callback) {
        return STATUS_INVALID_PARAMETER;
    }

    KeAcquireSpinLock(&Process->ProcessLock, &oldIrql);

    // 遍历进程线程链表
    LIST_FOR_EACH(entry, &Process->ThreadListHead) {
        thread = CONTAINING_RECORD(entry, THREAD_CONTROL_BLOCK, ThreadListEntry);

        // 增加引用计数
        ObReferenceObject(&thread->Header);

        // 调用回调函数
        continueEnum = Callback(thread, Context);

        // 减少引用计数
        ObDereferenceObject(&thread->Header);

        if (!continueEnum) {
            break;
        }
    }

    KeReleaseSpinLock(&Process->ProcessLock, oldIrql);

    return STATUS_SUCCESS;
}

// 获取当前线程
PTHREAD_CONTROL_BLOCK TmGetCurrentThread(VOID)
{
    return g_CurrentThread;
}

// 设置当前线程（由调度器调用）
VOID TmSetCurrentThread(
    _In_ PTHREAD_CONTROL_BLOCK Thread
)
{
    if (g_CurrentThread != Thread) {
        // 更新上下文切换统计
        if (g_CurrentThread) {
            TmUpdateStatistics(ThreadContextSwitch, g_CurrentThread);
        }

        g_CurrentThread = Thread;
    }

    if (Thread) {
        // 更新线程状态为运行
        TmSetThreadState(Thread, THREAD_STATE_RUNNING);
    }
}

// 更新统计信息
static VOID TmUpdateStatistics(
    _In_ THREAD_OPERATION Operation,
    _In_opt_ PTHREAD_CONTROL_BLOCK Thread
)
{
    KIRQL oldIrql;

    KeAcquireSpinLock(&g_ThreadManager.ThreadListLock, &oldIrql);

    switch (Operation) {
    case ThreadCreate:
        g_ThreadManager.TotalThreadCount++;
        g_ThreadManager.ActiveThreadCount++;
        if (g_ThreadManager.TotalThreadCount > g_ThreadManager.PeakThreadCount) {
            g_ThreadManager.PeakThreadCount = g_ThreadManager.TotalThreadCount;
        }
        break;

    case ThreadTerminate:
        g_ThreadManager.TotalThreadCount--;
        g_ThreadManager.ActiveThreadCount--;
        break;

    case ThreadContextSwitch:
        g_ThreadManager.TotalContextSwitches++;
        if (Thread) {
            Thread->ContextSwitchCount++;
        }
        break;

    case ThreadTlsAllocation:
        g_ThreadManager.TotalTlsAllocations++;
        break;

    default:
        break;
    }

    KeReleaseSpinLock(&g_ThreadManager.ThreadListLock, oldIrql);
}

// 获取线程管理器统计信息
VOID TmGetStatistics(
    _Out_ PTHREAD_MANAGER_STATISTICS Stats
)
{
    KIRQL oldIrql;
    PLIST_ENTRY entry;
    PTHREAD_CONTROL_BLOCK thread;

    if (!Stats) {
        return;
    }

    KeAcquireSpinLock(&g_ThreadManager.ThreadListLock, &oldIrql);

    Stats->TotalThreads = g_ThreadManager.TotalThreadCount;
    Stats->ActiveThreads = g_ThreadManager.ActiveThreadCount;
    Stats->PeakThreadCount = g_ThreadManager.PeakThreadCount;
    Stats->Initialized = g_ThreadManager.Initialized;
    Stats->TotalContextSwitches = g_ThreadManager.TotalContextSwitches;
    Stats->TotalTlsAllocations = g_ThreadManager.TotalTlsAllocations;
    KeQuerySystemTime(&Stats->LastResetTime);

    // 复制状态计数数组
    for (ULONG i = 0; i < THREAD_STATE_MAX; i++) {
        Stats->ThreadsInState[i] = g_ThreadManager.ThreadsInState[i];
    }

    KeReleaseSpinLock(&g_ThreadManager.ThreadListLock, oldIrql);
}

// 线程信息转储函数
NTSTATUS TmDumpThread(
    _In_ PTHREAD_CONTROL_BLOCK Thread
)
{
    CHAR stateNames[][16] = {
        "INVALID", "CREATED", "READY", "RUNNING",
        "WAITING", "SUSPENDED", "TERMINATED"
    };

    if (!Thread) {
        return STATUS_INVALID_PARAMETER;
    }

    TRACE_INFO("=== Thread Dump: ID=%u ===\n", Thread->ThreadId);
    TRACE_INFO("  State: %s\n", stateNames[Thread->State]);
    TRACE_INFO("  Priority: %d (Base: %d)\n", Thread->Priority, Thread->BasePriority);
    TRACE_INFO("  Process: %p (PID: %u)\n", Thread->Process,
        Thread->Process ? Thread->Process->ProcessId : 0);
    TRACE_INFO("  Kernel Stack: %p\n", Thread->KernelStack);
    TRACE_INFO("  User Stack: %p\n", Thread->UserStack);
    TRACE_INFO("  Instruction Pointer: %p\n", Thread->InstructionPointer);
    TRACE_INFO("  Wait Object: %p (Reason: %d)\n", Thread->WaitObject, Thread->WaitReason);
    TRACE_INFO("  CPU Affinity: 0x%X\n", Thread->CpuAffinity);
    TRACE_INFO("  Context Switches: %u\n", Thread->ContextSwitchCount);
    TRACE_INFO("  Kernel Time: %I64d\n", Thread->KernelTime.QuadPart);
    TRACE_INFO("  User Time: %I64d\n", Thread->UserTime.QuadPart);
    TRACE_INFO("  TLS Size: %u bytes (Max Index: %u)\n", Thread->TlsSize, Thread->MaxTlsIndex);
    TRACE_INFO("  Create Time: %I64d\n", Thread->CreateTime.QuadPart);

    // 如果有等待对象，显示详细信息
    if (Thread->WaitObject && TmValidateWaitObject(Thread->WaitObject)) {
        PKERNEL_OBJECT obj = (PKERNEL_OBJECT)Thread->WaitObject;
        TRACE_INFO("  Wait Object Type: %d\n", obj->Type);
    }

    // 显示持有的对象数量
    if (!IsListEmpty(&Thread->OwnedObjectsList)) {
        ULONG ownedCount = 0;
        PLIST_ENTRY entry;
        LIST_FOR_EACH(entry, &Thread->OwnedObjectsList) {
            ownedCount++;
        }
        TRACE_INFO("  Owned Objects: %u\n", ownedCount);
    }

    TRACE_INFO("=== End Thread Dump ===\n");

    return STATUS_SUCCESS;
}

// 转储所有线程信息（调试用）
NTSTATUS TmDumpAllThreads(VOID)
{
    KIRQL oldIrql;
    PLIST_ENTRY entry;
    PTHREAD_CONTROL_BLOCK thread;
    ULONG count = 0;

    TRACE_INFO("=== Dumping All Threads ===\n");

    KeAcquireSpinLock(&g_ThreadManager.ThreadListLock, &oldIrql);

    LIST_FOR_EACH(entry, &g_ThreadManager.GlobalThreadList) {
        thread = CONTAINING_RECORD(entry, THREAD_CONTROL_BLOCK, ThreadListEntry);
        TmDumpThread(thread);
        count++;
    }

    KeReleaseSpinLock(&g_ThreadManager.ThreadListLock, oldIrql);

    TRACE_INFO("=== Total %u threads dumped ===\n", count);
    return STATUS_SUCCESS;
}
