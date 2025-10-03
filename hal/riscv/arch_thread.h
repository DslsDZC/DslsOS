/**
 * DslsOS RISC-V 32-bit Architecture Thread Context Definitions
 */

#pragma once

#include <kernel/types.h>

 // RISC-V 32位状态寄存器
#define RISCV_SSTATUS_SPP   (1UL << 8)   // 先前特权模式
#define RISCV_SSTATUS_SPIE  (1UL << 5)   // 先前中断使能
#define RISCV_SSTATUS_UPIE  (1UL << 4)   // 用户模式先前中断使能
#define RISCV_SSTATUS_SIE   (1UL << 1)   // 监管者模式中断使能
#define RISCV_SSTATUS_UIE   (1UL << 0)   // 用户模式中断使能

// RISC-V 32位上下文结构
typedef struct _RISCV_CONTEXT {
    // 整数寄存器
    ULONG Zero;     // x0 - 硬连线零
    ULONG Ra;       // x1 - 返回地址
    ULONG Sp;       // x2 - 栈指针
    ULONG Gp;       // x3 - 全局指针
    ULONG Tp;       // x4 - 线程指针
    ULONG T0;       // x5 - 临时寄存器
    ULONG T1;       // x6 - 临时寄存器
    ULONG T2;       // x7 - 临时寄存器
    ULONG S0;       // x8 - 保存寄存器/帧指针
    ULONG S1;       // x9 - 保存寄存器
    ULONG A0;       // x10 - 函数参数/返回值
    ULONG A1;       // x11 - 函数参数
    ULONG A2;       // x12 - 函数参数
    ULONG A3;       // x13 - 函数参数
    ULONG A4;       // x14 - 函数参数
    ULONG A5;       // x15 - 函数参数
    ULONG A6;       // x16 - 函数参数
    ULONG A7;       // x17 - 函数参数
    ULONG S2;       // x18 - 保存寄存器
    ULONG S3;       // x19 - 保存寄存器
    ULONG S4;       // x20 - 保存寄存器
    ULONG S5;       // x21 - 保存寄存器
    ULONG S6;       // x22 - 保存寄存器
    ULONG S7;       // x23 - 保存寄存器
    ULONG S8;       // x24 - 保存寄存器
    ULONG S9;       // x25 - 保存寄存器
    ULONG S10;      // x26 - 保存寄存器
    ULONG S11;      // x27 - 保存寄存器
    ULONG T3;       // x28 - 临时寄存器
    ULONG T4;       // x29 - 临时寄存器
    ULONG T5;       // x30 - 临时寄存器
    ULONG T6;       // x31 - 临时寄存器

    // 程序计数器
    ULONG Pc;

    // 状态寄存器
    ULONG Sstatus;
    ULONG Sie;
    ULONG Stvec;
    ULONG Sscratch;
    ULONG Sepc;
    ULONG Stval;
    ULONG Sip;
    ULONG Satp;     // 页表基址

    // 浮点寄存器（可选）
    ULONG Fregs[32];
    ULONG Fcsr;
} RISCV_CONTEXT, * PRISCV_CONTEXT;

#define CONTEXT RISCV_CONTEXT
#define PCONTEXT PRISCV_CONTEXT
