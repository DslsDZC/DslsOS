/**
 * DslsOS x86_64 Architecture Thread Implementation
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
    PX86_64_CONTEXT context;

    TRACE_DEBUG("[HAL-x86_64] Initializing thread context for thread %u\n", Thread->ThreadId);

    // ������������ջ�е�λ��
    context = (PX86_64_CONTEXT)((PUCHAR)Thread->KernelStack + KERNEL_STACK_SIZE - sizeof(X86_64_CONTEXT));

    // ����������
    RtlZeroMemory(context, sizeof(X86_64_CONTEXT));

    // ����ָ��ָ��
    context->Rip = (ULONG64)StartAddress;

    // ���ò����Ĵ�����x86_64 System V ABIʹ��RDI���ݵ�һ��������
    context->Rdi = (ULONG64)Parameter;

    // ����ջָ��
    if (UserThread && Thread->UserStack) {
        context->Rsp = (ULONG64)Thread->UserStack + USER_STACK_SIZE - sizeof(ULONG64);
        context->SegCs = X86_64_USER_CS;
        context->SegSs = X86_64_USER_DS;
        context->EFlags = 0x200;   // �����ж�
        TRACE_DEBUG("[HAL-x86_64] User thread stack: %p -> %p\n",
            Thread->UserStack, (PVOID)context->Rsp);
    }
    else {
        context->Rsp = (ULONG64)Thread->KernelStack + KERNEL_STACK_SIZE - sizeof(ULONG64);
        context->SegCs = X86_64_KERNEL_CS;
        context->SegSs = X86_64_KERNEL_DS;
        context->EFlags = 0x200;   // �����ж�
        TRACE_DEBUG("[HAL-x86_64] Kernel thread stack: %p -> %p\n",
            Thread->KernelStack, (PVOID)context->Rsp);
    }

    // ���������μĴ���
    context->SegDs = context->SegSs;
    context->SegEs = context->SegSs;
    context->SegFs = context->SegSs;
    context->SegGs = context->SegSs;

    // ����ҳĿ¼
    if (Thread->Process && Thread->Process->PageDirectory) {
        context->Cr3 = (ULONG64)Thread->Process->PageDirectory;
        TRACE_DEBUG("[HAL-x86_64] Page directory: %p\n", (PVOID)context->Cr3);
    }

    // ����TLS��ַ
    if (Thread->TlsArray) {
        context->FsBase = (ULONG64)Thread->TlsArray;
        TRACE_DEBUG("[HAL-x86_64] TLS base: %p\n", Thread->TlsArray);
    }

    // ����������ָ�뵽TCB
    Thread->InstructionPointer = StartAddress;
    Thread->Context = context;

    TRACE_SUCCESS("[HAL-x86_64] Thread context initialized successfully\n");
    return STATUS_SUCCESS;
}

NTSTATUS ArchSwitchContext(
    _In_ PCONTEXT OldContext,
    _In_ PCONTEXT NewContext
)
{
    // x86_64�������л��ɻ�����ʵ��
    TRACE_DEBUG("[HAL-x86_64] Context switch: %p -> %p\n", OldContext, NewContext);

    if (OldContext && NewContext) {
        TRACE_DEBUG("[HAL-x86_64] Switching RIP: %p -> %p\n",
            (PVOID)OldContext->Rip, (PVOID)NewContext->Rip);
    }

    return STATUS_SUCCESS;
}

VOID ArchGetThreadContext(
    _In_ PTHREAD_CONTROL_BLOCK Thread,
    _Out_ PCONTEXT Context
)
{
    if (Thread && Thread->Context && Context) {
        RtlCopyMemory(Context, Thread->Context, sizeof(X86_64_CONTEXT));
        TRACE_DEBUG("[HAL-x86_64] Retrieved context for thread %u\n", Thread->ThreadId);
    }
}

VOID ArchSetThreadContext(
    _In_ PTHREAD_CONTROL_BLOCK Thread,
    _In_ PCONTEXT Context
)
{
    if (Thread && Thread->Context && Context) {
        RtlCopyMemory(Thread->Context, Context, sizeof(X86_64_CONTEXT));
        TRACE_DEBUG("[HAL-x86_64] Updated context for thread %u\n", Thread->ThreadId);
    }
}

ULONG_PTR ArchGetStackPointer(VOID)
{
    ULONG64 rsp;
    __asm__ __volatile__("mov %%rsp, %0" : "=r" (rsp));
    return rsp;
}

VOID ArchSetStackPointer(_In_ ULONG_PTR StackPointer)
{
    __asm__ __volatile__("mov %0, %%rsp" : : "r" (StackPointer));
}

ULONG ArchGetCurrentArchitecture(VOID)
{
    return ARCH_X86_64;
}

PCSTR ArchGetArchitectureName(VOID)
{
    return "x86_64";
}
