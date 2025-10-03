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

    // 计算上下文在栈中的位置
    context = (PRISCV_CONTEXT)((PUCHAR)Thread->KernelStack + KERNEL_STACK_SIZE - sizeof(RISCV_CONTEXT));

    // 清零上下文
    RtlZeroMemory(context, sizeof(RISCV_CONTEXT));

    // 设置程序计数器
    context->Pc = (ULONG)StartAddress;

    // 设置参数寄存器（RISC-V使用A0传递第一个参数）
    context->A0 = (ULONG)Parameter;

    // 设置栈指针
    if (UserThread && Thread->UserStack) {
        context->Sp = (ULONG)Thread->UserStack + USER_STACK_SIZE - sizeof(ULONG);
        // 用户模式状态设置
        context->Sstatus = RISCV_SSTATUS_SPIE | RISCV_SSTATUS_UPIE;
        TRACE_DEBUG("[HAL-RISCV] User thread stack: %p -> %p\n",
            Thread->UserStack, (PVOID)context->Sp);
    }
    else {
        context->Sp = (ULONG)Thread->KernelStack + KERNEL_STACK_SIZE - sizeof(ULONG);
        // 监管者模式状态设置
        context->Sstatus = RISCV_SSTATUS_SPP | RISCV_SSTATUS_SPIE;
        TRACE_DEBUG("[HAL-RISCV] Kernel thread stack: %p -> %p\n",
            Thread->KernelStack, (PVOID)context->Sp);
    }

    // 设置返回地址（线程退出时跳转到线程退出处理函数）
    context->Ra = (ULONG)PsTerminateThread;

    // 设置页表基址
    if (Thread->Process && Thread->Process->PageDirectory) {
        context->Satp = (ULONG)Thread->Process->PageDirectory;
        TRACE_DEBUG("[HAL-RISCV] Page table base: %p\n", (PVOID)context->Satp);
    }

    // 设置线程指针（用于TLS）
    if (Thread->TlsArray) {
        context->Tp = (ULONG)Thread->TlsArray;
        TRACE_DEBUG("[HAL-RISCV] TLS pointer: %p\n", Thread->TlsArray);
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

    TRACE_SUCCESS("[HAL-RISCV] Thread context initialized successfully\n");
    return STATUS_SUCCESS;
}

NTSTATUS ArchSwitchContext(
    _In_ PCONTEXT OldContext,
    _In_ PCONTEXT NewContext
)
{
    // RISC-V上下文切换由汇编代码实现
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
