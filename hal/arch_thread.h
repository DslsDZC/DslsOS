/**
 * DslsOS Hardware Abstraction Layer - Thread Context Interface
 * 支持多架构的线程上下文管理
 */

#pragma once

#include <kernel/types.h>

 // 架构定义
#define ARCH_X86       1
#define ARCH_X86_64    2
#define ARCH_ARM       3
#define ARCH_ARM64     4
#define ARCH_RISCV     5
#define ARCH_RISCV64   6

// 检测当前架构
#if defined(__x86_64__)
#define CURRENT_ARCH ARCH_X86_64
#include "x86_64/arch_thread.h"
#elif defined(__i386__)
#define CURRENT_ARCH ARCH_X86
#include "x86/arch_thread.h"
#elif defined(__aarch64__)
#define CURRENT_ARCH ARCH_ARM64
#include "arm64/arch_thread.h"
#elif defined(__arm__)
#define CURRENT_ARCH ARCH_ARM
#include "arm/arch_thread.h"
#elif defined(__riscv) && (__riscv_xlen == 64)
#define CURRENT_ARCH ARCH_RISCV64
#include "riscv64/arch_thread.h"
#elif defined(__riscv) && (__riscv_xlen == 32)
#define CURRENT_ARCH ARCH_RISCV
#include "riscv/arch_thread.h"
#else
#error "Unsupported architecture"
#endif

// 统一的HAL接口函数（所有架构必须实现）
NTSTATUS ArchInitializeThreadContext(
    _In_ PTHREAD_CONTROL_BLOCK Thread,
    _In_ PVOID StartAddress,
    _In_ PVOID Parameter,
    _In_ BOOLEAN UserThread
);

NTSTATUS ArchSwitchContext(
    _In_ PCONTEXT OldContext,
    _In_ PCONTEXT NewContext
);

VOID ArchGetThreadContext(
    _In_ PTHREAD_CONTROL_BLOCK Thread,
    _Out_ PCONTEXT Context
);

VOID ArchSetThreadContext(
    _In_ PTHREAD_CONTROL_BLOCK Thread,
    _In_ PCONTEXT Context
);

// 架构相关的栈操作
ULONG_PTR ArchGetStackPointer(VOID);
VOID ArchSetStackPointer(_In_ ULONG_PTR StackPointer);

// 架构检测
ULONG ArchGetCurrentArchitecture(VOID);
PCSTR ArchGetArchitectureName(VOID);
