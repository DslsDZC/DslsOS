/**
 * DslsOS x86_64 Architecture Thread Context Definitions
 */

#pragma once

#include <kernel/types.h>

 // x86_64段选择子
#define X86_64_KERNEL_CS    0x08
#define X86_64_KERNEL_DS    0x10
#define X86_64_USER_CS      0x18
#define X86_64_USER_DS      0x20

// x86_64上下文结构
typedef struct _X86_64_CONTEXT {
    // 通用寄存器
    ULONG64 Rax, Rbx, Rcx, Rdx;
    ULONG64 Rsi, Rdi;
    ULONG64 Rbp;
    ULONG64 Rsp;
    ULONG64 Rip;

    // 扩展寄存器
    ULONG64 R8, R9, R10, R11, R12, R13, R14, R15;

    // 段寄存器
    ULONG SegCs, SegDs, SegEs, SegFs, SegGs, SegSs;

    // 控制寄存器
    ULONG64 EFlags;
    ULONG64 Cr3; // 页目录基址

    // SSE/AVX状态（可选）
    UCHAR FpuState[512];

    // GS/FS基址（用于TLS）
    ULONG64 GsBase;
    ULONG64 FsBase;
} X86_64_CONTEXT, * PX86_64_CONTEXT;

#define CONTEXT X86_64_CONTEXT
#define PCONTEXT PX86_64_CONTEXT
