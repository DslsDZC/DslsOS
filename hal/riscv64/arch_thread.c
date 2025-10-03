/**
 * DslsOS RISC-V 64-bit Architecture Thread Implementation
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
    PRISCV64_CONTEXT context;

    TRACE_DEBUG("[HAL-RISCV64] Initializing thread context for thread %u\n", Thread->ThreadId);

    // 计算上下文在栈中的位置
    context = (PRISCV64_CONTEXT)((PUCHAR)Thread->KernelStack + KERNEL_STACK_SIZE - sizeof(RISCV64_CONTEXT));

    // 清零上下文
    RtlZeroMemory(context, sizeof(RISCV64_CONTEXT));

    // 设置程序计数器
    context->Pc = (ULONG64)StartAddress;

    // 设置参数寄存器（RISC-V使用A0传递第一个参数）
    context->A0 = (ULONG64)Parameter;

    // 设置栈指针
    if (UserThread && Thread->UserStack) {
        context->Sp = (ULONG64)Thread->UserStack + USER_STACK_SIZE - sizeof(ULONG64);
        // 用户模式状态设置
        context->Sstatus = RISCV64_SSTATUS_SPIE | RISCV64_SSTATUS_UPIE;
        TRACE_DEBUG("[HAL-RISCV64] User thread stack: %p -> %p\n",
            Thread->UserStack, (PVOID)context->Sp);
    }
    else {
        context->Sp = (ULONG64)Thread->KernelStack + KERNEL_STACK_SIZE - sizeof(ULONG64);
        // 监管者模式状态设置
        context->Sstatus = RISCV64_SSTATUS_SPP | RISCV64_SSTATUS_SPIE;
        TRACE_DEBUG("[HAL-RISCV64] Kernel thread stack: %p -> %p\n",
            Thread->KernelStack, (PVOID)context->Sp);
    }

    // 设置返回地址（线程退出时跳转到线程退出处理函数）
    context->Ra = (ULONG64)PsTerminateThread;

    // 设置页表基址
    if (Thread->Process && Thread->Process->PageDirectory) {
        context->Satp = (ULONG64)Thread->Process->PageDirectory;
        TRACE_DEBUG("[HAL-RISCV64] Page table base: %p\n", (PVOID)context->Satp);
    }

    // 设置线程指针（用于TLS）
    if (Thread->TlsArray) {
        context->Tp = (ULONG64)Thread->TlsArray;
        TRACE_DEBUG("[HAL-RISCV64] TLS pointer: %p\n", Thread->TlsArray);
    }

    // 设置全局指针（通常指向数据段）
    context->Gp = 0; // 需要在链接时确定

    // 初始化其他寄存器为0
    context->A1 = context->A2 = context->A3 = context->A4 = 0;
    context->A5 = context->A6 = context->A7 = 0;
    context->S0 = context->Sp; // 帧指针指向栈顶

    // 保存上下文指针到TCB
    Thread->InstructionPointer = StartAddress;
    Thread->Context = context;

    TRACE_SUCCESS("[HAL-RISCV64] Thread context initialized successfully\n");
    return STATUS_SUCCESS;
}

NTSTATUS ArchSwitchContext(
    _In_ PCONTEXT OldContext,
    _In_ PCONTEXT NewContext
)
{
    // RISC-V上下文切换由汇编代码实现
    TRACE_DEBUG("[HAL-RISCV64] Context switch: %p -> %p\n", OldContext, NewContext);

    if (OldContext && NewContext) {
        TRACE_DEBUG("[HAL-RISCV64] Switching PC: %p -> %p\n",
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
        RtlCopyMemory(Context, Thread->Context, sizeof(RISCV64_CONTEXT));
        TRACE_DEBUG("[HAL-RISCV64] Retrieved context for thread %u\n", Thread->ThreadId);
    }
}

VOID ArchSetThreadContext(
    _In_ PTHREAD_CONTROL_BLOCK Thread,
    _In_ PCONTEXT Context
)
{
    if (Thread && Thread->Context && Context) {
        RtlCopyMemory(Thread->Context, Context, sizeof(RISCV64_CONTEXT));
        TRACE_DEBUG("[HAL-RISCV64] Updated context for thread %u\n", Thread->ThreadId);
    }
}

ULONG_PTR ArchGetStackPointer(VOID)
{
    ULONG64 sp;
    __asm__ __volatile__("mv %0, sp" : "=r" (sp));
    return sp;
}

VOID ArchSetStackPointer(_In_ ULONG_PTR StackPointer)
{
    __asm__ __volatile__("mv sp, %0" : : "r" (StackPointer));
}

ULONG ArchGetCurrentArchitecture(VOID)
{
    return ARCH_RISCV64;
}

PCSTR ArchGetArchitectureName(VOID)
{
    return "RISC-V64";
}