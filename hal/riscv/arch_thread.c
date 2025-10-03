/**
 * DslsOS RISC-V 32-bit Architecture Thread Implementation
 */

#include <hal/arch_thread.h>
#include <kernel/kernel.h>
#include <kernel/debug.h>

NTSTATUS ArchInitializeThreadContext(
    _In_ PTHREAD_CONTROL_BLOCK Thread,
    _In_ PVOID StartAddress,
    _In_ PVOID Parameter,
    _In_ BOOLEAN UserThread
)
{
    PRISCV_CONTEXT context;

    TRACE_DEBUG("[HAL-RISCV] Initializing thread context for thread %u\n", Thread->ThreadId);

    // ������������ջ�е�λ��
    context = (PRISCV_CONTEXT)((PUCHAR)Thread->KernelStack + KERNEL_STACK_SIZE - sizeof(RISCV_CONTEXT));

    // ����������
    RtlZeroMemory(context, sizeof(RISCV_CONTEXT));

    // ���ó��������
    context->Pc = (ULONG)StartAddress;

    // ���ò����Ĵ�����RISC-Vʹ��A0���ݵ�һ��������
    context->A0 = (ULONG)Parameter;

    // ����ջָ��
    if (UserThread && Thread->UserStack) {
        context->Sp = (ULONG)Thread->UserStack + USER_STACK_SIZE - sizeof(ULONG);
        // �û�ģʽ״̬����
        context->Sstatus = RISCV_SSTATUS_SPIE | RISCV_SSTATUS_UPIE;
        TRACE_DEBUG("[HAL-RISCV] User thread stack: %p -> %p\n",
            Thread->UserStack, (PVOID)context->Sp);
    }
    else {
        context->Sp = (ULONG)Thread->KernelStack + KERNEL_STACK_SIZE - sizeof(ULONG);
        // �����ģʽ״̬����
        context->Sstatus = RISCV_SSTATUS_SPP | RISCV_SSTATUS_SPIE;
        TRACE_DEBUG("[HAL-RISCV] Kernel thread stack: %p -> %p\n",
            Thread->KernelStack, (PVOID)context->Sp);
    }

    // ���÷��ص�ַ���߳��˳�ʱ��ת���߳��˳���������
    context->Ra = (ULONG)PsTerminateThread;

    // ����ҳ���ַ
    if (Thread->Process && Thread->Process->PageDirectory) {
        context->Satp = (ULONG)Thread->Process->PageDirectory;
        TRACE_DEBUG("[HAL-RISCV] Page table base: %p\n", (PVOID)context->Satp);
    }

    // �����߳�ָ�루����TLS��
    if (Thread->TlsArray) {
        context->Tp = (ULONG)Thread->TlsArray;
        TRACE_DEBUG("[HAL-RISCV] TLS pointer: %p\n", Thread->TlsArray);
    }

    // ����ȫ��ָ�루ͨ��ָ�����ݶΣ�
    context->Gp = 0; // ��Ҫ������ʱȷ��

    // ��ʼ�������Ĵ���Ϊ0
    context->A1 = context->A2 = context->A3 = context->A4 = 0;
    context->A5 = context->A6 = context->A7 = 0;
    context->S0 = context->Sp; // ָ֡��ָ��ջ��

    // ����������ָ�뵽TCB
    Thread->InstructionPointer = StartAddress;
    Thread->Context = context;

    TRACE_SUCCESS("[HAL-RISCV] Thread context initialized successfully\n");
    return STATUS_SUCCESS;
}

NTSTATUS ArchSwitchContext(
    _In_ PCONTEXT OldContext,
    _In_ PCONTEXT NewContext
)
{
    // RISC-V�������л��ɻ�����ʵ��
    TRACE_DEBUG("[HAL-RISCV] Context switch: %p -> %p\n", OldContext, NewContext);

    if (OldContext && NewContext) {
        TRACE_DEBUG("[HAL-RISCV] Switching PC: %p -> %p\n",
            (PVOID)OldContext->Pc, (PVOID)NewContext->Pc);
    }

    return STATUS_SUCCESS;
}

VOID ArchGetThreadContext(
    _In_ PTHREAD_CONTROL_BLOCK Thread,
    _Out_ PCONTEXT Context
)
{
    if (Thread && Thread->Context && Context) {
        RtlCopyMemory(Context, Thread->Context, sizeof(RISCV_CONTEXT));
        TRACE_DEBUG("[HAL-RISCV] Retrieved context for thread %u\n", Thread->ThreadId);
    }
}

VOID ArchSetThreadContext(
    _In_ PTHREAD_CONTROL_BLOCK Thread,
    _In_ PCONTEXT Context
)
{
    if (Thread && Thread->Context && Context) {
        RtlCopyMemory(Thread->Context, Context, sizeof(RISCV_CONTEXT));
        TRACE_DEBUG("[HAL-RISCV] Updated context for thread %u\n", Thread->ThreadId);
    }
}

ULONG_PTR ArchGetStackPointer(VOID)
{
    ULONG sp;
    __asm__ __volatile__("mv %0, sp" : "=r" (sp));
    return sp;
}

VOID ArchSetStackPointer(_In_ ULONG_PTR StackPointer)
{
    __asm__ __volatile__("mv sp, %0" : : "r" (StackPointer));
}

ULONG ArchGetCurrentArchitecture(VOID)
{
    return ARCH_RISCV;
}

PCSTR ArchGetArchitectureName(VOID)
{
    return "RISC-V";
}
