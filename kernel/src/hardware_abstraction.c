/**
 * @file hardware_abstraction.c
 * @brief Hardware abstraction layer implementation
 * @author DslsOS Team
 * @version 1.0
 * @date 2024
 */

#include "../include/kernel.h"
#include "../include/dslos.h"

// Hardware state
typedef struct _HARDWARE_STATE {
    BOOLEAN Initialized;
    ULONG CpuCount;
    ULONG ActiveCpuMask;
    ULONG PageSize;
    ULONG AllocationGranularity;
    LARGE_INTEGER SystemTime;
    LARGE_INTEGER InterruptTime;
    LARGE_INTEGER PerformanceCounter;
} HARDWARE_STATE;

static HARDWARE_STATE g_HardwareState = {0};

// Interrupt controller interface
typedef struct _INTERRUPT_CONTROLLER {
    VOID (*Initialize)(VOID);
    VOID (*EnableInterrupt)(ULONG Vector);
    VOID (*DisableInterrupt)(ULONG Vector);
    VOID (*SendEndOfInterrupt)(ULONG Vector);
    VOID (*MaskInterrupt)(ULONG Vector);
    VOID (*UnmaskInterrupt)(ULONG Vector);
} INTERRUPT_CONTROLLER;

// Timer interface
typedef struct _TIMER_CONTROLLER {
    VOID (*Initialize)(VOID);
    VOID (*StartTimer)(ULONG TimerId, ULONG Milliseconds);
    VOID (*StopTimer)(ULONG TimerId);
    ULONG (*GetElapsedTime)(ULONG TimerId);
    VOID (*SetPeriodicTimer)(ULONG TimerId, ULONG Interval);
} TIMER_CONTROLLER;

// Static interrupt controller implementation
static INTERRUPT_CONTROLLER g_InterruptController = {0};
static TIMER_CONTROLLER g_TimerController = {0};

/**
 * @brief Initialize processor
 */
VOID HalInitializeProcessor(VOID)
{
    if (g_HardwareState.Initialized) {
        return;
    }

#ifdef DSLOS_ARCH_X64
    // x86_64 specific initialization
    HalInitializeProcessorX64();
#elif defined(DSLOS_ARCH_X86)
    // x86 specific initialization
    HalInitializeProcessorX86();
#elif defined(DSLOS_ARCH_ARM64)
    // ARM64 specific initialization
    HalInitializeProcessorArm64();
#elif defined(DSLOS_ARCH_ARM)
    // ARM specific initialization
    HalInitializeProcessorArm();
#endif

    // Detect CPU information
    HalDetectCpuInformation();

    // Initialize floating point unit
    HalInitializeFpu();

    g_HardwareState.Initialized = TRUE;
}

/**
 * @brief Initialize interrupts
 */
VOID HalInitializeInterrupts(VOID)
{
    if (g_InterruptController.Initialize != NULL) {
        g_InterruptController.Initialize();
    }
}

/**
 * @brief Initialize timers
 */
VOID HalInitializeTimers(VOID)
{
    if (g_TimerController.Initialize != NULL) {
        g_TimerController.Initialize();
    }
}

/**
 * @brief Disable interrupts
 */
VOID HalDisableInterrupts(VOID)
{
#ifdef DSLOS_ARCH_X64
    __asm__ __volatile__("cli");
#elif defined(_WIN64)
    _disable();
#endif
}

/**
 * @brief Enable interrupts
 */
VOID HalEnableInterrupts(VOID)
{
#ifdef DSLOS_ARCH_X64
    __asm__ __volatile__("sti");
#elif defined(_WIN64)
    _enable();
#endif
}

/**
 * @brief Halt system
 */
VOID HalHaltSystem(VOID)
{
    HalDisableInterrupts();

#ifdef DSLOS_ARCH_X64
    while (1) {
        __asm__ __volatile__("hlt");
    }
#elif defined(_WIN64)
    while (1) {
        __halt();
    }
#endif
}

/**
 * @brief Get system time
 * @return Current system time
 */
LARGE_INTEGER HalGetSystemTime(VOID)
{
    LARGE_INTEGER time;

#ifdef _WIN64
    KeQuerySystemTime(&time);
#else
    // Platform-specific implementation
    time.QuadPart = 0;
#endif

    return time;
}

/**
 * @brief Get performance counter
 * @return Performance counter value
 */
LARGE_INTEGER HalGetPerformanceCounter(VOID)
{
    LARGE_INTEGER counter;

#ifdef _WIN64
    KeQueryPerformanceCounter(&counter);
#else
    // Platform-specific implementation
    counter.QuadPart = 0;
#endif

    return counter;
}

/**
 * @brief Read port byte
 * @param Port Port number
 * @return Byte read from port
 */
UINT8 HalReadPortByte(USHORT Port)
{
    UINT8 value;

#ifdef DSLOS_ARCH_X64
    __asm__ __volatile__("inb %1, %0" : "=a"(value) : "Nd"(Port));
#elif defined(_WIN64)
    value = __inbyte(Port);
#endif

    return value;
}

/**
 * @brief Write port byte
 * @param Port Port number
 * @param Value Value to write
 */
VOID HalWritePortByte(USHORT Port, UINT8 Value)
{
#ifdef DSLOS_ARCH_X64
    __asm__ __volatile__("outb %0, %1" : : "a"(Value), "Nd"(Port));
#elif defined(_WIN64)
    __outbyte(Port, Value);
#endif
}

/**
 * @brief Read port word
 * @param Port Port number
 * @return Word read from port
 */
UINT16 HalReadPortWord(USHORT Port)
{
    UINT16 value;

#ifdef DSLOS_ARCH_X64
    __asm__ __volatile__("inw %1, %0" : "=a"(value) : "Nd"(Port));
#elif defined(_WIN64)
    value = __inword(Port);
#endif

    return value;
}

/**
 * @brief Write port word
 * @param Port Port number
 * @param Value Value to write
 */
VOID HalWritePortWord(USHORT Port, UINT16 Value)
{
#ifdef DSLOS_ARCH_X64
    __asm__ __volatile__("outw %0, %1" : : "a"(Value), "Nd"(Port));
#elif defined(_WIN64)
    __outword(Port, Value);
#endif
}

/**
 * @brief Read port dword
 * @param Port Port number
 * @return Dword read from port
 */
UINT32 HalReadPortDword(USHORT Port)
{
    UINT32 value;

#ifdef DSLOS_ARCH_X64
    __asm__ __volatile__("inl %1, %0" : "=a"(value) : "Nd"(Port));
#elif defined(_WIN64)
    __indword(Port);
#endif

    return value;
}

/**
 * @brief Write port dword
 * @param Port Port number
 * @param Value Value to write
 */
VOID HalWritePortDword(USHORT Port, UINT32 Value)
{
#ifdef DSLOS_ARCH_X64
    __asm__ __volatile__("outl %0, %1" : : "a"(Value), "Nd"(Port));
#elif defined(_WIN64)
    __outdword(Port, Value);
#endif
}

/**
 * @brief Read memory byte
 * @param Address Memory address
 * @return Byte read from memory
 */
UINT8 HalReadMemoryByte(PVOID Address)
{
    return *((volatile UINT8*)Address);
}

/**
 * @brief Write memory byte
 * @param Address Memory address
 * @param Value Value to write
 */
VOID HalWriteMemoryByte(PVOID Address, UINT8 Value)
{
    *((volatile UINT8*)Address) = Value;
}

/**
 * @brief Flush CPU cache
 */
VOID HalFlushCpuCache(VOID)
{
#ifdef DSLOS_ARCH_X64
    __asm__ __volatile__("wbinvd");
#elif defined(_WIN64)
    __wbinvd();
#endif
}

/**
 * @brief Invalidate TLB entry
 * @param VirtualAddress Virtual address to invalidate
 */
VOID HalInvalidateTlbEntry(PVOID VirtualAddress)
{
#ifdef DSLOS_ARCH_X64
    __asm__ __volatile__("invlpg (%0)" : : "r"(VirtualAddress));
#elif defined(_WIN64)
    __invlpg(VirtualAddress);
#endif
}

/**
 * @brief Get CPU flags
 * @return Current CPU flags
 */
ULONG HalGetCpuFlags(VOID)
{
    ULONG flags;

#ifdef DSLOS_ARCH_X64
    __asm__ __volatile__(
        "pushfq\n\t"
        "popq %0"
        : "=r"(flags)
    );
#elif defined(_WIN64)
    flags = __readeflags();
#endif

    return flags;
}

/**
 * @brief Set CPU flags
 * @param Flags CPU flags to set
 */
VOID HalSetCpuFlags(ULONG Flags)
{
#ifdef DSLOS_ARCH_X64
    __asm__ __volatile__(
        "pushq %0\n\t"
        "popfq"
        : : "r"(Flags)
    );
#elif defined(_WIN64)
    __writeeflags(Flags);
#endif
}

/**
 * @brief Get CR3 register (page directory base)
 * @return CR3 register value
 */
UINT_PTR HalGetCr3(VOID)
{
    UINT_PTR cr3;

#ifdef DSLOS_ARCH_X64
    __asm__ __volatile__("movq %%cr3, %0" : "=r"(cr3));
#elif defined(_WIN64)
    cr3 = __readcr3();
#endif

    return cr3;
}

/**
 * @brief Set CR3 register
 * @param Cr3Value CR3 value to set
 */
VOID HalSetCr3(UINT_PTR Cr3Value)
{
#ifdef DSLOS_ARCH_X64
    __asm__ __volatile__("movq %0, %%cr3" : : "r"(Cr3Value));
#elif defined(_WIN64)
    __writecr3(Cr3Value);
#endif
}

/**
 * @brief Get current processor number
 * @return Current processor number
 */
ULONG HalGetCurrentProcessorNumber(VOID)
{
#ifdef _WIN64
    return KeGetCurrentProcessorNumber();
#else
    return 0; // Simplified for single processor
#endif
}

/**
 * @brief Get processor affinity mask
 * @return Processor affinity mask
 */
ULONG_PTR HalGetProcessorAffinityMask(VOID)
{
#ifdef _WIN64
    return KeQueryActiveProcessors();
#else
    return 1; // Simplified for single processor
#endif
}

/**
 * @brief Get page size
 * @return System page size
 */
ULONG HalGetPageSize(VOID)
{
    if (g_HardwareState.PageSize == 0) {
        SYSTEM_INFO info;
        KeGetSystemInfo(&info);
        g_HardwareState.PageSize = info.dwPageSize;
    }
    return g_HardwareState.PageSize;
}

/**
 * @brief Get allocation granularity
 * @return Memory allocation granularity
 */
ULONG HalGetAllocationGranularity(VOID)
{
    if (g_HardwareState.AllocationGranularity == 0) {
        SYSTEM_INFO info;
        KeGetSystemInfo(&info);
        g_HardwareState.AllocationGranularity = info.dwAllocationGranularity;
    }
    return g_HardwareState.AllocationGranularity;
}

/**
 * @brief Detect CPU information
 */
static VOID HalDetectCpuInformation(VOID)
{
    SYSTEM_INFO info;
    KeGetSystemInfo(&info);

    g_HardwareState.CpuCount = info.dwNumberOfProcessors;
    g_HardwareState.ActiveCpuMask = (ULONG)info.dwActiveProcessorMask;
    g_HardwareState.PageSize = info.dwPageSize;
    g_HardwareState.AllocationGranularity = info.dwAllocationGranularity;
}

/**
 * @brief Initialize floating point unit
 */
static VOID HalInitializeFpu(VOID)
{
#ifdef DSLOS_ARCH_X64
    // Enable SSE and AVX
    ULONG cr0 = HalGetCr0();
    cr0 &= ~(1 << 2); // Clear EM (Emulation)
    cr0 |= (1 << 1);   // Set MP (Math Present)
    HalSetCr0(cr0);

    ULONG cr4 = HalGetCr4();
    cr4 |= (1 << 9);   // Set OSFXSR (Operating System FXSAVE/FXRSTOR support)
    cr4 |= (1 << 10);  // Set OSXMMEXCPT (Operating System Unmasked Exception support)
    HalSetCr4(cr4);
#endif
}

// Architecture-specific initialization functions
#ifdef DSLOS_ARCH_X64
static VOID HalInitializeProcessorX64(VOID)
{
    // x86_64 specific initialization
    // Set up page tables, GDT, IDT, etc.
}

#elif defined(DSLOS_ARCH_X86)
static VOID HalInitializeProcessorX86(VOID)
{
    // x86 specific initialization
}

#elif defined(DSLOS_ARCH_ARM64)
static VOID HalInitializeProcessorArm64(VOID)
{
    // ARM64 specific initialization
}

#elif defined(DSLOS_ARCH_ARM)
static VOID HalInitializeProcessorArm(VOID)
{
    // ARM specific initialization
}
#endif

/**
 * @brief Register interrupt controller
 * @param Controller Interrupt controller interface
 */
VOID HalRegisterInterruptController(PINTERRUPT_CONTROLLER Controller)
{
    if (Controller != NULL) {
        RtlCopyMemory(&g_InterruptController, Controller, sizeof(INTERRUPT_CONTROLLER));
    }
}

/**
 * @brief Register timer controller
 * @param Controller Timer controller interface
 */
VOID HalRegisterTimerController(PTIMER_CONTROLLER Controller)
{
    if (Controller != NULL) {
        RtlCopyMemory(&g_TimerController, Controller, sizeof(TIMER_CONTROLLER));
    }
}

/**
 * @brief Get interrupt controller interface
 * @return Pointer to interrupt controller interface
 */
PINTERRUPT_CONTROLLER HalGetInterruptController(VOID)
{
    return &g_InterruptController;
}

/**
 * @brief Get timer controller interface
 * @return Pointer to timer controller interface
 */
PTIMER_CONTROLLER HalGetTimerController(VOID)
{
    return &g_TimerController;
}