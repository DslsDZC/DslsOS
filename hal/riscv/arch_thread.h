/**
 * DslsOS RISC-V 32-bit Architecture Thread Context Definitions
 */

#pragma once

#include <kernel/types.h>

 // RISC-V 32λ״̬�Ĵ���
#define RISCV_SSTATUS_SPP   (1UL << 8)   // ��ǰ��Ȩģʽ
#define RISCV_SSTATUS_SPIE  (1UL << 5)   // ��ǰ�ж�ʹ��
#define RISCV_SSTATUS_UPIE  (1UL << 4)   // �û�ģʽ��ǰ�ж�ʹ��
#define RISCV_SSTATUS_SIE   (1UL << 1)   // �����ģʽ�ж�ʹ��
#define RISCV_SSTATUS_UIE   (1UL << 0)   // �û�ģʽ�ж�ʹ��

// RISC-V 32λ�����Ľṹ
typedef struct _RISCV_CONTEXT {
    // �����Ĵ���
    ULONG Zero;     // x0 - Ӳ������
    ULONG Ra;       // x1 - ���ص�ַ
    ULONG Sp;       // x2 - ջָ��
    ULONG Gp;       // x3 - ȫ��ָ��
    ULONG Tp;       // x4 - �߳�ָ��
    ULONG T0;       // x5 - ��ʱ�Ĵ���
    ULONG T1;       // x6 - ��ʱ�Ĵ���
    ULONG T2;       // x7 - ��ʱ�Ĵ���
    ULONG S0;       // x8 - ����Ĵ���/ָ֡��
    ULONG S1;       // x9 - ����Ĵ���
    ULONG A0;       // x10 - ��������/����ֵ
    ULONG A1;       // x11 - ��������
    ULONG A2;       // x12 - ��������
    ULONG A3;       // x13 - ��������
    ULONG A4;       // x14 - ��������
    ULONG A5;       // x15 - ��������
    ULONG A6;       // x16 - ��������
    ULONG A7;       // x17 - ��������
    ULONG S2;       // x18 - ����Ĵ���
    ULONG S3;       // x19 - ����Ĵ���
    ULONG S4;       // x20 - ����Ĵ���
    ULONG S5;       // x21 - ����Ĵ���
    ULONG S6;       // x22 - ����Ĵ���
    ULONG S7;       // x23 - ����Ĵ���
    ULONG S8;       // x24 - ����Ĵ���
    ULONG S9;       // x25 - ����Ĵ���
    ULONG S10;      // x26 - ����Ĵ���
    ULONG S11;      // x27 - ����Ĵ���
    ULONG T3;       // x28 - ��ʱ�Ĵ���
    ULONG T4;       // x29 - ��ʱ�Ĵ���
    ULONG T5;       // x30 - ��ʱ�Ĵ���
    ULONG T6;       // x31 - ��ʱ�Ĵ���

    // ���������
    ULONG Pc;

    // ״̬�Ĵ���
    ULONG Sstatus;
    ULONG Sie;
    ULONG Stvec;
    ULONG Sscratch;
    ULONG Sepc;
    ULONG Stval;
    ULONG Sip;
    ULONG Satp;     // ҳ���ַ

    // ����Ĵ�������ѡ��
    ULONG Fregs[32];
    ULONG Fcsr;
} RISCV_CONTEXT, * PRISCV_CONTEXT;

#define CONTEXT RISCV_CONTEXT
#define PCONTEXT PRISCV_CONTEXT
