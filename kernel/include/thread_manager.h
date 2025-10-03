/**
 * DslsOS Thread Manager Header
 * 线程管理器公共接口定义
 */

#pragma once

#include <kernel/types.h>
#include <hal/arch_thread.h>

 // 线程状态
typedef enum _THREAD_STATE {
    THREAD_STATE_INVALID = 0,
    THREAD_STATE_CREATED,      // 已创建但未初始化完成
    THREAD_STATE_READY,        // 就绪，等待调度
    THREAD_STATE_RUNNING,      // 正在运行
    THREAD_STATE_WAITING,      // 等待（同步对象、I/O等）
    THREAD_STATE_SUSPENDED,    // 挂起
    THREAD_STATE_TERMINATED,   // 已终止
    THREAD_STATE_MAX
} THREAD_STATE;

// 等待原因
typedef enum _WAIT_REASON {
    WaitReasonNone = 0,
    WaitReasonExecutive,       // 执行体对象等待
    WaitReasonUserRequest,     // 用户请求等待
    WaitReasonSynchronization, // 同步对象等待
    WaitReasonIoCompletion,    // I/O完成等待
    WaitReasonPageFault,       // 页面错误等待
    WaitReasonMax
} WAIT_REASON;

// 线程优先级常量
#define THREAD_PRIORITY_MIN        0
#define THREAD_PRIORITY_LOW        8
#define THREAD_PRIORITY_NORMAL     16
#define THREAD_PRIORITY_HIGH       24
#define THREAD_PRIORITY_MAX        31

// 栈大小常量
#define KERNEL_STACK_SIZE          (16 * 1024)    // 16KB内核栈
#define USER_STACK_SIZE            (64 * 1024)    // 64KB用户栈

// TLS管理常量
#define TLS_SLOT_SIZE           sizeof(PVOID)
#define TLS_INITIAL_SLOTS       64      // 初始64个槽位
#define TLS_EXPANSION_STEP      16      // 每次扩展16个槽位
#define TLS_MAX_SLOTS           (1024)  // 最大1024个槽位

// 线程控制块（前向声明，完整定义在kernel.h中）
typedef struct _THREAD_CONTROL_BLOCK THREAD_CONTROL_BLOCK, * PTHREAD_CONTROL_BLOCK;

// 线程枚举回调函数类型
typedef BOOLEAN(*PENUM_THREADS_CALLBACK)(
    _In_ PTHREAD_CONTROL_BLOCK Thread,
    _In_ PVOID Context
    );

// 线程操作类型（用于统计）
typedef enum _THREAD_OPERATION {
    ThreadCreate,
    ThreadTerminate,
    ThreadContextSwitch,
    ThreadTlsAllocation,
    ThreadStateChange
} THREAD_OPERATION;

// 线程管理器统计信息
typedef struct _THREAD_MANAGER_STATISTICS {
    ULONG TotalThreads;
    ULONG ActiveThreads;
    ULONG ThreadsInState[THREAD_STATE_MAX]; // 各状态线程计数
    ULONG TotalContextSwitches;
    ULONG TotalTlsAllocations;
    ULONG PeakThreadCount;
    LARGE_INTEGER LastResetTime;
    BOOLEAN Initialized;
} THREAD_MANAGER_STATISTICS, * PTHREAD_MANAGER_STATISTICS;

// 公共API函数
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

// 新增调试函数
NTSTATUS TmDumpThread(_In_ PTHREAD_CONTROL_BLOCK Thread);
NTSTATUS TmDumpAllThreads(VOID);

// 内部统计更新函数（供其他模块调用）
VOID TmUpdateStatistics(_In_ THREAD_OPERATION Operation, _In_opt_ PTHREAD_CONTROL_BLOCK Thread);

// 内联辅助函数
__forceinline BOOLEAN TmIsThreadAlive(_In_ PTHREAD_CONTROL_BLOCK Thread) {
    return Thread && Thread->State != THREAD_STATE_TERMINATED && Thread->State != THREAD_STATE_INVALID;
}

__forceinline THREAD_ID TmGetCurrentThreadId(VOID) {
    PTHREAD_CONTROL_BLOCK current = TmGetCurrentThread();
    return current ? current->ThreadId : 0;
}

// 性能监控宏
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