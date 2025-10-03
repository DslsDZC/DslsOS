/**
 * DslsOS RISC-V 64-bit Architecture Thread Context Definitions
 */

#pragma once

#include <kernel/types.h>

 // RISC-V状态寄存器
#define RISCV64_SSTATUS_SPP   (1UL << 8)   // 先前特权模式
#define RISCV64_SSTATUS_SPIE  (1UL << 5)   // 先前中断使能
#define RISCV64_SSTATUS_UPIE  (1UL << 4)   // 用户模式先前中断使能
#define RISCV64_SSTATUS_SIE   (1UL << 1)   // 监管者模式中断使能
#define RISCV64_SSTATUS_UIE   (1UL << 0)   // 用户模式中断使能

// RISC-V 64位上下文结构
typedef struct _RISCV64_CONTEXT {
    // 整数寄存器
    ULONG64 Zero;   // x0 - 硬连线零
    ULONG64 Ra;     // x1 - 返回地址
    ULONG64 Sp;     // x2 - 栈指针
    ULONG64 Gp;     // x3 - 全局指针
    ULONG64 Tp;     // x4 - 线程指针
    ULONG64 T0;     // x5 - 临时寄存器
    ULONG64 T1;     // x6 - 临时寄存器
    ULONG64 T2;     // x7 - 临时寄存器
    ULONG64 S0;     // x8 - 保存寄存器/帧指针
    ULONG64 S1;     // x9 - 保存寄存器
    ULONG64 A0;     // x10 - 函数参数/返回值
    ULONG64 A1;     // x11 - 函数参数
    ULONG64 A2;     // x12 - 函数参数
    ULONG64 A3;     // x13 - 函数参数
    ULONG64 A4;     // x14 - 函数参数
    ULONG64 A5;     // x15 - 函数参数
    ULONG64 A6;     // x16 - 函数参数
    ULONG64 A7;     // x17 - 函数参数
    ULONG64 S2;     // x18 - 保存寄存器
    ULONG64 S3;     // x19 - 保存寄存器
    ULONG64 S4;     // x20 - 保存寄存器
    ULONG64 S5;     // x21 - 保存寄存器
    ULONG64 S6;     // x22 - 保存寄存器
    ULONG64 S7;     // x23 - 保存寄存器
    ULONG64 S8;     // x24 - 保存寄存器
    ULONG64 S9;     // x25 - 保存寄存器
    ULONG64 S10;    // x26 - 保存寄存器
    ULONG64 S11;    // x27 - 保存寄存器
    ULONG64 T3;     // x28 - 临时寄存器
    ULONG64 T4;     // x29 - 临时寄存器
    ULONG64 T5;     // x30 - 临时寄存器
    ULONG64 T6;     // x31 - 临时寄存器

    // 程序计数器
    ULONG64 Pc;

    // 状态寄存器
    ULONG64 Sstatus;
    ULONG64 Sie;
    ULONG64 Stvec;
    ULONG64 Sscratch;
    ULONG64 Sepc;
    ULONG64 Stval;
    ULONG64 Sip;
    ULONG64 Satp;   // 页表基址

    // 浮点寄存器（可选）
    ULONG64 Fregs[32];
    ULONG64 Fcsr;
} RISCV64_CONTEXT, * PRISCV64_CONTEXT;

#define CONTEXT RISCV64_CONTEXT
#define PCONTEXT PRISCV64_CONTEXT
