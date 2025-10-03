/**
 * DslsOS Thread Manager
 * ����ģ�黯��Ƶ��߳��������ڹ���
 * ����̹����������������ڴ����������ȼ���
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

 // �̹߳�����ȫ��״̬
static THREAD_MANAGER_STATE g_ThreadManager = { 0 };

// ��ǰ�߳�ָ�루ÿ��CPU����һ����
__declspec(thread) PTHREAD_CONTROL_BLOCK g_CurrentThread = NULL;

// �߳�ID������
static volatile ULONG g_NextThreadId = 1;

// �ڲ���������
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

// �̹߳�������ʼ��
NTSTATUS TmInitialize(VOID)
{
    KIRQL oldIrql;
    NTSTATUS status = STATUS_SUCCESS;

    TRACE_INFO("[TM] Initializing Thread Manager...\n");

    // ��ʼ��ȫ���߳�����
    InitializeListHead(&g_ThreadManager.GlobalThreadList);

    // ��ʼ��������������ͬ��ģ�飩
    KeInitializeSpinLock(&g_ThreadManager.ThreadListLock);

    // ���ü�����
    g_ThreadManager.TotalThreadCount = 0;
    g_ThreadManager.ActiveThreadCount = 0;
    g_ThreadManager.PeakThreadCount = 0;
    g_ThreadManager.TotalContextSwitches = 0;
    g_ThreadManager.TotalTlsAllocations = 0;
    KeQuerySystemTime(&g_ThreadManager.LastResetTime);

    // ��ʼ��״̬��������
    for (ULONG i = 0; i < THREAD_STATE_MAX; i++) {
        g_ThreadManager.ThreadsInState[i] = 0;
    }

    g_ThreadManager.Initialized = TRUE;

    TRACE_SUCCESS("[TM] Thread Manager initialized successfully\n");
    return status;
}

// �̹߳���������
VOID TmCleanup(VOID)
{
    KIRQL oldIrql;
    PLIST_ENTRY entry;
    PTHREAD_CONTROL_BLOCK thread;

    TRACE_INFO("[TM] Cleaning up Thread Manager...\n");

    if (!g_ThreadManager.Initialized) {
        return;
    }

    // ��ȡ������ȫ������
    KeAcquireSpinLock(&g_ThreadManager.ThreadListLock, &oldIrql);

    // ���������������̣߳����������Ӧ��ֻ��ϵͳ�̴߳��ڣ�
    while (!IsListEmpty(&g_ThreadManager.GlobalThreadList)) {
        entry = RemoveHeadList(&g_ThreadManager.GlobalThreadList);
        thread = CONTAINING_RECORD(entry, THREAD_CONTROL_BLOCK, ThreadListEntry);

        // ǿ����ֹ�߳�
        TmTerminateThread(thread);
    }

    KeReleaseSpinLock(&g_ThreadManager.ThreadListLock, oldIrql);

    g_ThreadManager.Initialized = FALSE;
    TRACE_SUCCESS("[TM] Thread Manager cleanup completed\n");
}

// �ڲ��̴߳���ʵ��
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

    // ������֤
    if (!Process || !StartAddress || !Thread) {
        return STATUS_INVALID_PARAMETER;
    }

    TRACE_DEBUG("[TM] Creating thread in process %p, start: %p\n",
        Process, StartAddress);

    TM_PERF_START();

    // �����߳̿��ƿ飨�����ڴ��������
    newThread = (PTHREAD_CONTROL_BLOCK)ExAllocatePool(
        NonPagedPool,
        sizeof(THREAD_CONTROL_BLOCK)
    );

    if (!newThread) {
        TRACE_ERROR("[TM] Failed to allocate TCB\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // ����TCB
    RtlZeroMemory(newThread, sizeof(THREAD_CONTROL_BLOCK));

    // ��ʼ���ں˶���ͷ�������ö����������
    newThread->Header.Type = KERNEL_OBJECT_THREAD;
    newThread->Header.ReferenceCount = 1; // ��ʼ���ü���
    newThread->Header.Size = sizeof(THREAD_CONTROL_BLOCK);

    // �����߳�ID
    newThread->ThreadId = InterlockedIncrement(&g_NextThreadId);

    // ������������
    newThread->Process = Process;

    // �������ȼ����̳н������ȼ���ʹ��Ĭ��ֵ��
    newThread->BasePriority = Process->BasePriority;
    newThread->Priority = Process->BasePriority;

    // ���ó�ʼ״̬
    newThread->State = CreateSuspended ? THREAD_STATE_SUSPENDED : THREAD_STATE_READY;
    newThread->WaitReason = WaitReasonNone;

    // ��ʼ��������
    InitializeListHead(&newThread->ThreadListEntry);
    InitializeListHead(&newThread->ReadyListEntry);
    InitializeListHead(&newThread->WaitListEntry);
    InitializeListHead(&newThread->OwnedObjectsList);

    // ��¼����ʱ��
    KeQuerySystemTime(&newThread->CreateTime);

    // �����ں�ջ�������ڴ��������
    status = MmAllocateKernelStack(&newThread->KernelStack, KERNEL_STACK_SIZE);
    if (!NT_SUCCESS(status)) {
        TRACE_ERROR("[TM] Failed to allocate kernel stack: 0x%X\n", status);
        ExFreePool(newThread);
        return status;
    }

    // �����û�ջ����������û����̣�
    if (Process->AddressSpace != NULL) {
        status = PsAllocateThreadStack(Process, &newThread->UserStack);
        if (!NT_SUCCESS(status)) {
            TRACE_ERROR("[TM] Failed to allocate user stack: 0x%X\n", status);
            MmFreeKernelStack(newThread->KernelStack);
            ExFreePool(newThread);
            return status;
        }
    }

    // ��ʼ���߳������ģ�ʹ��HAL����
    status = TmInitializeThreadContext(newThread, StartAddress, Parameter);
    if (!NT_SUCCESS(status)) {
        TRACE_ERROR("[TM] Failed to initialize thread context: 0x%X\n", status);
        TmCleanupThreadResources(newThread);
        ExFreePool(newThread);
        return status;
    }

    // ���߳���ӵ����̵��߳��б����ý��̹�������
    KeAcquireSpinLock(&Process->ProcessLock, &oldIrql);
    InsertTailList(&Process->ThreadListHead, &newThread->ThreadListEntry);
    Process->ThreadCount++;
    KeReleaseSpinLock(&Process->ProcessLock, oldIrql);

    // ���߳���ӵ�ȫ���߳��б�
    KeAcquireSpinLock(&g_ThreadManager.ThreadListLock, &oldIrql);
    InsertTailList(&g_ThreadManager.GlobalThreadList, &newThread->ThreadListEntry);

    // ����ͳ����Ϣ
    TmUpdateStatistics(ThreadCreate, newThread);

    KeReleaseSpinLock(&g_ThreadManager.ThreadListLock, oldIrql);

    // ������ǹ��𴴽���������������������
    if (!CreateSuspended) {
        KeAddThreadToReadyQueue(newThread);
    }

    *Thread = newThread;

    TM_PERF_END("Thread Creation");

    TRACE_SUCCESS("[TM] Thread %u created successfully in process %u\n",
        newThread->ThreadId, Process->ProcessId);

    return STATUS_SUCCESS;
}

// ��ʼ���߳������ģ�ʹ��HAL����
static NTSTATUS TmInitializeThreadContext(
    _In_ PTHREAD_CONTROL_BLOCK Thread,
    _In_ PVOID StartAddress,
    _In_ PVOID Parameter
)
{
    BOOLEAN userThread = (Thread->Process->AddressSpace != NULL);

    // ʹ��HAL�ӿڳ�ʼ��������
    return ArchInitializeThreadContext(Thread, StartAddress, Parameter, userThread);
}

// ��ֹ�߳�
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

    // �ͷ��̳߳��е�����ͬ�����󣨷�ֹ��Դй©��
    TmReleaseOwnedObjects(Thread);

    // ����߳��������У���Ҫ���л��������߳�
    if (Thread->State == THREAD_STATE_RUNNING) {
        // �ӵ��������ж����Ƴ�
        KeRemoveThreadFromReadyQueue(Thread);
    }

    // �����߳�״̬
    Thread->State = THREAD_STATE_TERMINATED;

    process = Thread->Process;

    // �ӽ����߳��б����Ƴ�
    if (process) {
        KeAcquireSpinLock(&process->ProcessLock, &oldIrql);
        if (!IsListEmpty(&Thread->ThreadListEntry)) {
            RemoveEntryList(&Thread->ThreadListEntry);
            process->ThreadCount--;

            // ������ǽ��̵����һ���̣߳���ǽ���Ϊ��ֹ
            if (process->ThreadCount == 0 && process->State != PROCESS_STATE_TERMINATED) {
                process->State = PROCESS_STATE_TERMINATED;
                TRACE_INFO("[TM] Last thread terminated, marking process %u as terminated\n",
                    process->ProcessId);
            }
        }
        KeReleaseSpinLock(&process->ProcessLock, oldIrql);
    }

    // ��ȫ���߳��б����Ƴ�
    KeAcquireSpinLock(&g_ThreadManager.ThreadListLock, &oldIrql);
    if (!IsListEmpty(&Thread->ThreadListEntry)) {
        RemoveEntryList(&Thread->ThreadListEntry);

        // ����ͳ����Ϣ
        TmUpdateStatistics(ThreadTerminate, Thread);
    }
    KeReleaseSpinLock(&g_ThreadManager.ThreadListLock, oldIrql);

    // �����߳���Դ
    TmCleanupThreadResources(Thread);

    TRACE_SUCCESS("[TM] Thread %u terminated safely\n", Thread->ThreadId);
    return STATUS_SUCCESS;
}

// �����߳���Դ
static VOID TmCleanupThreadResources(
    _In_ PTHREAD_CONTROL_BLOCK Thread
)
{
    // �ͷ��ں�ջ
    if (Thread->KernelStack) {
        MmFreeKernelStack(Thread->KernelStack);
        Thread->KernelStack = NULL;
    }

    // �ͷ��û�ջ
    if (Thread->UserStack) {
        PsFreeThreadStack(Thread->Process, Thread->UserStack);
        Thread->UserStack = NULL;
    }

    // �ͷ�TLS
    if (Thread->TlsArray) {
        ExFreePool(Thread->TlsArray);
        Thread->TlsArray = NULL;
        Thread->TlsSize = 0;
        Thread->MaxTlsIndex = 0;
        Thread->LastTlsSearchIndex = 0;
    }

    // ���ٶ������ü��������ö����������
    ObDereferenceObject(&Thread->Header);
}

// ͬ����������У��
static BOOLEAN TmValidateWaitObject(
    _In_ PVOID WaitObject
)
{
    PKERNEL_OBJECT kernelObj = (PKERNEL_OBJECT)WaitObject;

    if (!WaitObject) {
        return FALSE;
    }

    // �����������Ƿ�Ϊ�Ϸ���ͬ������
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

// �ȴ���������
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

    // У��ȴ���������
    if (!TmValidateWaitObject(WaitObject)) {
        return STATUS_INVALID_OBJECT_TYPE;
    }

    TRACE_DEBUG("[TM] Thread %u waiting for object %p (type: %d)\n",
        currentThread->ThreadId, WaitObject,
        ((PKERNEL_OBJECT)WaitObject)->Type);

    KeAcquireSpinLock(&g_ThreadManager.ThreadListLock, &oldIrql);

    // ��¼�ȴ������̵߳ĳ��ж����б����ڰ�ȫ��ֹ��
    PKERNEL_OBJECT kernelObj = (PKERNEL_OBJECT)WaitObject;
    InsertTailList(&currentThread->OwnedObjectsList, &kernelObj->OwnedListEntry);
    kernelObj->OwnerThread = currentThread;

    // �����߳�״̬
    currentThread->State = THREAD_STATE_WAITING;
    currentThread->WaitObject = WaitObject;
    currentThread->WaitReason = WaitReasonExecutive;

    // �ӵ��������������Ƴ�
    KeRemoveThreadFromReadyQueue(currentThread);

    // ����ȴ�����
    InsertTailList(&((PKWAIT_BLOCK)WaitObject)->WaitList, &currentThread->WaitListEntry);

    // ����ͳ����Ϣ
    TmUpdateStatistics(ThreadStateChange, currentThread);

    KeReleaseSpinLock(&g_ThreadManager.ThreadListLock, oldIrql);

    // �������µ���
    KeSchedule();

    TRACE_DEBUG("[TM] Thread %u resumed from wait\n", currentThread->ThreadId);
    return STATUS_SUCCESS;
}

// ֪ͨ���󣨻��ѵȴ��̣߳�
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

    // У��ȴ���������
    if (!TmValidateWaitObject(WaitObject)) {
        return STATUS_INVALID_OBJECT_TYPE;
    }

    TRACE_DEBUG("[TM] Signaling object %p (type: %d)\n",
        WaitObject, ((PKERNEL_OBJECT)WaitObject)->Type);

    KeAcquireSpinLock(&g_ThreadManager.ThreadListLock, &oldIrql);

    // �����ȴ����У��������еȴ��ö�����߳�
    LIST_ENTRY* waitList = &((PKWAIT_BLOCK)WaitObject)->WaitList;

    while (!IsListEmpty(waitList)) {
        entry = RemoveHeadList(waitList);
        thread = CONTAINING_RECORD(entry, THREAD_CONTROL_BLOCK, WaitListEntry);

        // ���̵߳ĳ��ж����б����Ƴ�
        PKERNEL_OBJECT kernelObj = (PKERNEL_OBJECT)WaitObject;
        RemoveEntryList(&kernelObj->OwnedListEntry);
        kernelObj->OwnerThread = NULL;

        // �ָ��߳�״̬
        thread->State = THREAD_STATE_READY;
        thread->WaitObject = NULL;
        thread->WaitReason = WaitReasonNone;

        // �����������������
        KeAddThreadToReadyQueue(thread);

        // ����ͳ����Ϣ
        TmUpdateStatistics(ThreadStateChange, thread);
    }

    KeReleaseSpinLock(&g_ThreadManager.ThreadListLock, oldIrql);

    // �������µ���
    KeSchedule();

    return STATUS_SUCCESS;
}

// �ͷ��̳߳��е�����ͬ������
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

    // �����̳߳��еĶ����б�
    while (!IsListEmpty(&Thread->OwnedObjectsList)) {
        entry = RemoveHeadList(&Thread->OwnedObjectsList);
        object = CONTAINING_RECORD(entry, KERNEL_OBJECT, OwnedListEntry);

        // ���ݶ������ͽ����ʵ����ͷŲ���
        switch (object->Type) {
        case KERNEL_OBJECT_MUTEX:
            // ���ڻ���������Ҫ�ͷŲ����ѵȴ���
            status = KeReleaseMutex((PMUTEX)object, TRUE);
            if (!NT_SUCCESS(status)) {
                TRACE_WARNING("[TM] Failed to release mutex %p: 0x%X\n", object, status);
            }
            break;

        case KERNEL_OBJECT_SEMAPHORE:
            // �ź�������Ҫ���⴦���ȴ��߻��Զ�����
            break;

        case KERNEL_OBJECT_EVENT:
            // �¼����������ź�״̬
            KeSetEvent((PEVENT)object, IO_NO_INCREMENT, FALSE);
            break;

        default:
            TRACE_WARNING("[TM] Unknown object type %d in owned list\n", object->Type);
            break;
        }

        // ��������������
        object->OwnerThread = NULL;
    }

    KeReleaseSpinLock(&g_ThreadManager.ThreadListLock, oldIrql);

    return status;
}

// �����߳�״̬
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

    // ����״̬����
    if (oldState < THREAD_STATE_MAX) {
        g_ThreadManager.ThreadsInState[oldState]--;
    }
    if (NewState < THREAD_STATE_MAX) {
        g_ThreadManager.ThreadsInState[NewState]++;
    }

    KeReleaseSpinLock(&g_ThreadManager.ThreadListLock, oldIrql);

    // ����ͳ����Ϣ
    TmUpdateStatistics(ThreadStateChange, Thread);

    TRACE_DEBUG("[TM] Thread %u state changed: %d -> %d\n",
        Thread->ThreadId, oldState, NewState);

    return STATUS_SUCCESS;
}

// ��ȡ�߳�״̬
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

// �Ż���TLS���亯��
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

    // ���TLS����δ��ʼ�������ʼ��
    if (!Thread->TlsArray) {
        Thread->TlsSize = TLS_INITIAL_SLOTS * TLS_SLOT_SIZE;
        Thread->TlsArray = ExAllocatePool(PagedPool, Thread->TlsSize);
        if (!Thread->TlsArray) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        RtlZeroMemory(Thread->TlsArray, Thread->TlsSize);
        Thread->MaxTlsIndex = 0; // ��¼�����Ч����
        Thread->LastTlsSearchIndex = 0;
    }

    currentSlots = Thread->TlsSize / TLS_SLOT_SIZE;

    // ���ҿ��в�λ�����ϴη���λ�ÿ�ʼ����߾ֲ��ԣ�
    for (i = Thread->LastTlsSearchIndex; i < currentSlots; i++) {
        if (((PVOID*)Thread->TlsArray)[i] == NULL) {
            *TlsIndex = i;
            Thread->LastTlsSearchIndex = i + 1; // ����������ʼλ��
            Thread->MaxTlsIndex = max(Thread->MaxTlsIndex, i);
            TmUpdateStatistics(ThreadTlsAllocation, Thread);
            return STATUS_SUCCESS;
        }
    }

    // ��ͷ��ʼ����
    for (i = 0; i < Thread->LastTlsSearchIndex; i++) {
        if (((PVOID*)Thread->TlsArray)[i] == NULL) {
            *TlsIndex = i;
            Thread->LastTlsSearchIndex = i + 1;
            Thread->MaxTlsIndex = max(Thread->MaxTlsIndex, i);
            TmUpdateStatistics(ThreadTlsAllocation, Thread);
            return STATUS_SUCCESS;
        }
    }

    // û�п��в�λ�����̶�������չ
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

    // ���ƾ����ݲ���ʼ���¿ռ�
    RtlCopyMemory(newArray, Thread->TlsArray, Thread->TlsSize);
    RtlZeroMemory((PUCHAR)newArray + Thread->TlsSize,
        (newSlots * TLS_SLOT_SIZE) - Thread->TlsSize);

    ExFreePool(Thread->TlsArray);
    Thread->TlsArray = newArray;
    Thread->TlsSize = newSlots * TLS_SLOT_SIZE;

    // �����һ���²�λ
    *TlsIndex = currentSlots;
    Thread->LastTlsSearchIndex = currentSlots + 1;
    Thread->MaxTlsIndex = currentSlots;
    TmUpdateStatistics(ThreadTlsAllocation, Thread);

    TRACE_DEBUG("[TM] Expanded TLS for thread %u to %u slots\n",
        Thread->ThreadId, newSlots);

    return STATUS_SUCCESS;
}

// ��ȡTLSֵ
PVOID TmGetTlsValue(
    _In_ PTHREAD_CONTROL_BLOCK Thread,
    _In_ ULONG TlsIndex
)
{
    if (!Thread || !Thread->TlsArray || TlsIndex > Thread->MaxTlsIndex) {
        return NULL;
    }

    // ֱ��ʹ��Ԥ����ı߽��飬�������
    return ((PVOID*)Thread->TlsArray)[TlsIndex];
}

// ����TLSֵ
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

    // �������������������õ�ֵ��֮ǰδʹ�õĲ�λ��
    if (Value != NULL && TlsIndex > Thread->MaxTlsIndex) {
        Thread->MaxTlsIndex = TlsIndex;
    }

    return STATUS_SUCCESS;
}

// �ͷ�TLS��λ
NTSTATUS TmFreeTls(
    _In_ PTHREAD_CONTROL_BLOCK Thread,
    _In_ ULONG TlsIndex
)
{
    if (!Thread || !Thread->TlsArray || TlsIndex > Thread->MaxTlsIndex) {
        return STATUS_INVALID_PARAMETER;
    }

    ((PVOID*)Thread->TlsArray)[TlsIndex] = NULL;

    // ����ͷŵ��������������Ҫ����MaxTlsIndex
    if (TlsIndex == Thread->MaxTlsIndex) {
        // �����ҵ�һ���ǿղ�λ
        while (Thread->MaxTlsIndex > 0 &&
            ((PVOID*)Thread->TlsArray)[Thread->MaxTlsIndex - 1] == NULL) {
            Thread->MaxTlsIndex--;
        }
    }

    // ����������ʼλ�ã��Ż��´η��䣩
    if (TlsIndex < Thread->LastTlsSearchIndex) {
        Thread->LastTlsSearchIndex = TlsIndex;
    }

    return STATUS_SUCCESS;
}

// �����߳����ȼ�
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
    Thread->Priority = Priority; // ��ʵ�֣�ʵ�ʿ����ж�̬����

    KeReleaseSpinLock(&g_ThreadManager.ThreadListLock, oldIrql);

    // ֪ͨ�������������ȼ�
    KeUpdateThreadPriority(Thread);

    return STATUS_SUCCESS;
}

// ����ID��ȡ�߳�
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

    // ����ȫ���߳�����
    LIST_FOR_EACH(entry, &g_ThreadManager.GlobalThreadList) {
        current = CONTAINING_RECORD(entry, THREAD_CONTROL_BLOCK, ThreadListEntry);

        if (current->ThreadId == ThreadId && current->State != THREAD_STATE_TERMINATED) {
            // �������ü���
            ObReferenceObject(&current->Header);
            *Thread = current;
            break;
        }
    }

    KeReleaseSpinLock(&g_ThreadManager.ThreadListLock, oldIrql);

    return *Thread ? STATUS_SUCCESS : STATUS_NOT_FOUND;
}

// ö�ٽ����е��߳�
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

    // ���������߳�����
    LIST_FOR_EACH(entry, &Process->ThreadListHead) {
        thread = CONTAINING_RECORD(entry, THREAD_CONTROL_BLOCK, ThreadListEntry);

        // �������ü���
        ObReferenceObject(&thread->Header);

        // ���ûص�����
        continueEnum = Callback(thread, Context);

        // �������ü���
        ObDereferenceObject(&thread->Header);

        if (!continueEnum) {
            break;
        }
    }

    KeReleaseSpinLock(&Process->ProcessLock, oldIrql);

    return STATUS_SUCCESS;
}

// ��ȡ��ǰ�߳�
PTHREAD_CONTROL_BLOCK TmGetCurrentThread(VOID)
{
    return g_CurrentThread;
}

// ���õ�ǰ�̣߳��ɵ��������ã�
VOID TmSetCurrentThread(
    _In_ PTHREAD_CONTROL_BLOCK Thread
)
{
    if (g_CurrentThread != Thread) {
        // �����������л�ͳ��
        if (g_CurrentThread) {
            TmUpdateStatistics(ThreadContextSwitch, g_CurrentThread);
        }

        g_CurrentThread = Thread;
    }

    if (Thread) {
        // �����߳�״̬Ϊ����
        TmSetThreadState(Thread, THREAD_STATE_RUNNING);
    }
}

// ����ͳ����Ϣ
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

// ��ȡ�̹߳�����ͳ����Ϣ
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

    // ����״̬��������
    for (ULONG i = 0; i < THREAD_STATE_MAX; i++) {
        Stats->ThreadsInState[i] = g_ThreadManager.ThreadsInState[i];
    }

    KeReleaseSpinLock(&g_ThreadManager.ThreadListLock, oldIrql);
}

// �߳���Ϣת������
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

    // ����еȴ�������ʾ��ϸ��Ϣ
    if (Thread->WaitObject && TmValidateWaitObject(Thread->WaitObject)) {
        PKERNEL_OBJECT obj = (PKERNEL_OBJECT)Thread->WaitObject;
        TRACE_INFO("  Wait Object Type: %d\n", obj->Type);
    }

    // ��ʾ���еĶ�������
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

// ת�������߳���Ϣ�������ã�
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
