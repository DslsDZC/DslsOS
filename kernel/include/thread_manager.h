/**
 * DslsOS Thread Manager Header
 * �̹߳����������ӿڶ���
 */

#pragma once

#include <kernel/types.h>
#include <hal/arch_thread.h>

 // �߳�״̬
typedef enum _THREAD_STATE {
    THREAD_STATE_INVALID = 0,
    THREAD_STATE_CREATED,      // �Ѵ�����δ��ʼ�����
    THREAD_STATE_READY,        // �������ȴ�����
    THREAD_STATE_RUNNING,      // ��������
    THREAD_STATE_WAITING,      // �ȴ���ͬ������I/O�ȣ�
    THREAD_STATE_SUSPENDED,    // ����
    THREAD_STATE_TERMINATED,   // ����ֹ
    THREAD_STATE_MAX
} THREAD_STATE;

// �ȴ�ԭ��
typedef enum _WAIT_REASON {
    WaitReasonNone = 0,
    WaitReasonExecutive,       // ִ�������ȴ�
    WaitReasonUserRequest,     // �û�����ȴ�
    WaitReasonSynchronization, // ͬ������ȴ�
    WaitReasonIoCompletion,    // I/O��ɵȴ�
    WaitReasonPageFault,       // ҳ�����ȴ�
    WaitReasonMax
} WAIT_REASON;

// �߳����ȼ�����
#define THREAD_PRIORITY_MIN        0
#define THREAD_PRIORITY_LOW        8
#define THREAD_PRIORITY_NORMAL     16
#define THREAD_PRIORITY_HIGH       24
#define THREAD_PRIORITY_MAX        31

// ջ��С����
#define KERNEL_STACK_SIZE          (16 * 1024)    // 16KB�ں�ջ
#define USER_STACK_SIZE            (64 * 1024)    // 64KB�û�ջ

// TLS������
#define TLS_SLOT_SIZE           sizeof(PVOID)
#define TLS_INITIAL_SLOTS       64      // ��ʼ64����λ
#define TLS_EXPANSION_STEP      16      // ÿ����չ16����λ
#define TLS_MAX_SLOTS           (1024)  // ���1024����λ

// �߳̿��ƿ飨ǰ������������������kernel.h�У�
typedef struct _THREAD_CONTROL_BLOCK THREAD_CONTROL_BLOCK, * PTHREAD_CONTROL_BLOCK;

// �߳�ö�ٻص���������
typedef BOOLEAN(*PENUM_THREADS_CALLBACK)(
    _In_ PTHREAD_CONTROL_BLOCK Thread,
    _In_ PVOID Context
    );

// �̲߳������ͣ�����ͳ�ƣ�
typedef enum _THREAD_OPERATION {
    ThreadCreate,
    ThreadTerminate,
    ThreadContextSwitch,
    ThreadTlsAllocation,
    ThreadStateChange
} THREAD_OPERATION;

// �̹߳�����ͳ����Ϣ
typedef struct _THREAD_MANAGER_STATISTICS {
    ULONG TotalThreads;
    ULONG ActiveThreads;
    ULONG ThreadsInState[THREAD_STATE_MAX]; // ��״̬�̼߳���
    ULONG TotalContextSwitches;
    ULONG TotalTlsAllocations;
    ULONG PeakThreadCount;
    LARGE_INTEGER LastResetTime;
    BOOLEAN Initialized;
} THREAD_MANAGER_STATISTICS, * PTHREAD_MANAGER_STATISTICS;

// ����API����
NTSTATUS TmInitialize(VOID);
VOID TmCleanup(VOID);

NTSTATUS TmCreateThreadInternal(
    _In_ PPROCESS_CONTROL_BLOCK Process,
    _In_ PVOID StartAddress,
    _In_ PVOID Parameter,
    _In_ BOOLEAN CreateSuspended,
    _Out_ PTHREAD_CONTROL_BLOCK* Thread
);

NTSTATUS TmTerminateThread(_In_ PTHREAD_CONTROL_BLOCK Thread);
NTSTATUS TmWaitForSingleObject(_In_ PVOID WaitObject, _In_ ULONG Timeout);
NTSTATUS TmSignalObject(_In_ PVOID WaitObject);

NTSTATUS TmSetThreadState(_In_ PTHREAD_CONTROL_BLOCK Thread, _In_ THREAD_STATE NewState);
THREAD_STATE TmGetThreadState(_In_ PTHREAD_CONTROL_BLOCK Thread);

NTSTATUS TmAllocateTls(_In_ PTHREAD_CONTROL_BLOCK Thread, _Out_ PULONG TlsIndex);
PVOID TmGetTlsValue(_In_ PTHREAD_CONTROL_BLOCK Thread, _In_ ULONG TlsIndex);
NTSTATUS TmSetTlsValue(_In_ PTHREAD_CONTROL_BLOCK Thread, _In_ ULONG TlsIndex, _In_ PVOID Value);
NTSTATUS TmFreeTls(_In_ PTHREAD_CONTROL_BLOCK Thread, _In_ ULONG TlsIndex);

NTSTATUS TmSetThreadPriority(_In_ PTHREAD_CONTROL_BLOCK Thread, _In_ LONG Priority);

NTSTATUS TmGetThreadById(_In_ THREAD_ID ThreadId, _Out_ PTHREAD_CONTROL_BLOCK* Thread);
NTSTATUS TmEnumThreads(
    _In_ PPROCESS_CONTROL_BLOCK Process,
    _In_ PENUM_THREADS_CALLBACK Callback,
    _In_ PVOID Context
);

PTHREAD_CONTROL_BLOCK TmGetCurrentThread(VOID);
VOID TmSetCurrentThread(_In_ PTHREAD_CONTROL_BLOCK Thread);

VOID TmGetStatistics(_Out_ PTHREAD_MANAGER_STATISTICS Stats);

// �������Ժ���
NTSTATUS TmDumpThread(_In_ PTHREAD_CONTROL_BLOCK Thread);
NTSTATUS TmDumpAllThreads(VOID);

// �ڲ�ͳ�Ƹ��º�����������ģ����ã�
VOID TmUpdateStatistics(_In_ THREAD_OPERATION Operation, _In_opt_ PTHREAD_CONTROL_BLOCK Thread);

// ������������
__forceinline BOOLEAN TmIsThreadAlive(_In_ PTHREAD_CONTROL_BLOCK Thread) {
    return Thread && Thread->State != THREAD_STATE_TERMINATED && Thread->State != THREAD_STATE_INVALID;
}

__forceinline THREAD_ID TmGetCurrentThreadId(VOID) {
    PTHREAD_CONTROL_BLOCK current = TmGetCurrentThread();
    return current ? current->ThreadId : 0;
}

// ���ܼ�غ�
#ifdef DEBUG
#define TM_PERF_START() LARGE_INTEGER _perfStart; KeQueryPerformanceCounter(&_perfStart)
#define TM_PERF_END(Operation) \
    do { \
        LARGE_INTEGER _perfEnd; \
        KeQueryPerformanceCounter(&_perfEnd); \
        TRACE_DEBUG("[TM-Perf] %s took %I64d cycles\n", Operation, _perfEnd.QuadPart - _perfStart.QuadPart); \
    } while(0)
#else
#define TM_PERF_START()
#define TM_PERF_END(Operation)
#endif