/**
 * DslsOS RISC-V 64-bit Architecture Thread Context Definitions
 */

#pragma once

#include <kernel/types.h>

 // RISC-V״̬�Ĵ���
#define RISCV64_SSTATUS_SPP   (1UL << 8)   // ��ǰ��Ȩģʽ
#define RISCV64_SSTATUS_SPIE  (1UL << 5)   // ��ǰ�ж�ʹ��
#define RISCV64_SSTATUS_UPIE  (1UL << 4)   // �û�ģʽ��ǰ�ж�ʹ��
#define RISCV64_SSTATUS_SIE   (1UL << 1)   // �����ģʽ�ж�ʹ��
#define RISCV64_SSTATUS_UIE   (1UL << 0)   // �û�ģʽ�ж�ʹ��

// RISC-V 64λ�����Ľṹ
typedef struct _RISCV64_CONTEXT {
    // �����Ĵ���
    ULONG64 Zero;   // x0 - Ӳ������
    ULONG64 Ra;     // x1 - ���ص�ַ
    ULONG64 Sp;     // x2 - ջָ��
    ULONG64 Gp;     // x3 - ȫ��ָ��
    ULONG64 Tp;     // x4 - �߳�ָ��
    ULONG64 T0;     // x5 - ��ʱ�Ĵ���
    ULONG64 T1;     // x6 - ��ʱ�Ĵ���
    ULONG64 T2;     // x7 - ��ʱ�Ĵ���
    ULONG64 S0;     // x8 - ����Ĵ���/ָ֡��
    ULONG64 S1;     // x9 - ����Ĵ���
    ULONG64 A0;     // x10 - ��������/����ֵ
    ULONG64 A1;     // x11 - ��������
    ULONG64 A2;     // x12 - ��������
    ULONG64 A3;     // x13 - ��������
    ULONG64 A4;     // x14 - ��������
    ULONG64 A5;     // x15 - ��������
    ULONG64 A6;     // x16 - ��������
    ULONG64 A7;     // x17 - ��������
    ULONG64 S2;     // x18 - ����Ĵ���
    ULONG64 S3;     // x19 - ����Ĵ���
    ULONG64 S4;     // x20 - ����Ĵ���
    ULONG64 S5;     // x21 - ����Ĵ���
    ULONG64 S6;     // x22 - ����Ĵ���
    ULONG64 S7;     // x23 - ����Ĵ���
    ULONG64 S8;     // x24 - ����Ĵ���
    ULONG64 S9;     // x25 - ����Ĵ���
    ULONG64 S10;    // x26 - ����Ĵ���
    ULONG64 S11;    // x27 - ����Ĵ���
    ULONG64 T3;     // x28 - ��ʱ�Ĵ���
    ULONG64 T4;     // x29 - ��ʱ�Ĵ���
    ULONG64 T5;     // x30 - ��ʱ�Ĵ���
    ULONG64 T6;     // x31 - ��ʱ�Ĵ���

    // ���������
    ULONG64 Pc;

    // ״̬�Ĵ���
    ULONG64 Sstatus;
    ULONG64 Sie;
    ULONG64 Stvec;
    ULONG64 Sscratch;
    ULONG64 Sepc;
    ULONG64 Stval;
    ULONG64 Sip;
    ULONG64 Satp;   // ҳ���ַ

    // ����Ĵ�������ѡ��
    ULONG64 Fregs[32];
    ULONG64 Fcsr;
} RISCV64_CONTEXT, * PRISCV64_CONTEXT;

#define CONTEXT RISCV64_CONTEXT
#define PCONTEXT PRISCV64_CONTEXT
