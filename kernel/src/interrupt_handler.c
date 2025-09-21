/**
 * @file interrupt_handler.c
 * @brief Interrupt handler implementation
 * @author DslsOS Team
 * @version 1.0
 * @date 2024
 */

#include "../include/kernel.h"
#include "../include/dslos.h"

// Interrupt handler state
typedef struct _INTERRUPT_HANDLER_STATE {
    BOOLEAN Initialized;
    KSPIN_LOCK InterruptLock;

    // Interrupt dispatch table
    INTERRUPT_HANDLER InterruptHandlers[256];
    INTERRUPT_HANDLER FastInterruptHandlers[32];

    // Interrupt statistics
    INTERRUPT_STATISTICS Statistics;

    // Interrupt nesting level
    volatile LONG NestingLevel;

    // Deferred Procedure Calls (DPC)
    LIST_ENTRY DpcQueueHead;
    KSPIN_LOCK DpcLock;
    ULONG DpcQueueDepth;
    BOOLEAN DpcProcessing;
} INTERRUPT_HANDLER_STATE;

static INTERRUPT_HANDLER_STATE g_InterruptHandler = {0};

// Interrupt statistics structure
typedef struct _INTERRUPT_STATISTICS {
    ULONG TotalInterrupts;
    ULONG TotalSpuriousInterrupts;
    ULONG InterruptCounts[256];
    ULONG DpcCount;
    LARGE_INTEGER TotalInterruptTime;
} INTERRUPT_STATISTICS, *PINTERRUPT_STATISTICS;

// DPC structure
typedef struct _KDPC {
    LIST_ENTRY DpcListEntry;
    PVOID DeferredRoutine;
    PVOID DeferredContext;
    ULONG Priority;
} KDPC, *PKDPC;

// Interrupt flags
#define INTERRUPT_FLAG_SPURIOUS     0x00000001
#define INTERRUPT_FLAG_MASKED       0x00000002
#define INTERRUPT_FLAG_PENDING      0x00000004
#define INTERRUPT_FLAG_IN_SERVICE   0x00000008

/**
 * @brief Initialize interrupt handler
 * @return NTSTATUS Status code
 */
NTSTATUS KeInitializeInterruptHandler(VOID)
{
    if (g_InterruptHandler.Initialized) {
        return STATUS_SUCCESS;
    }

    KeInitializeSpinLock(&g_InterruptHandler.InterruptLock);

    // Initialize interrupt dispatch table
    for (ULONG i = 0; i < 256; i++) {
        g_InterruptHandler.InterruptHandlers[i] = NULL;
    }

    // Initialize fast interrupt handlers
    for (ULONG i = 0; i < 32; i++) {
        g_InterruptHandler.FastInterruptHandlers[i] = NULL;
    }

    // Initialize statistics
    RtlZeroMemory(&g_InterruptHandler.Statistics, sizeof(INTERRUPT_STATISTICS));

    // Initialize DPC queue
    InitializeListHead(&g_InterruptHandler.DpcQueueHead);
    KeInitializeSpinLock(&g_InterruptHandler.DpcLock);
    g_InterruptHandler.DpcQueueDepth = 0;
    g_InterruptHandler.DpcProcessing = FALSE;

    // Initialize nesting level
    g_InterruptHandler.NestingLevel = 0;

    // Register default interrupt handlers
    KeRegisterDefaultHandlers();

    g_InterruptHandler.Initialized = TRUE;
    return STATUS_SUCCESS;
}

/**
 * @brief Register interrupt handler
 * @param Vector Interrupt vector
 * @param Handler Interrupt handler function
 * @param Flags Interrupt flags
 * @return NTSTATUS Status code
 */
NTSTATUS KeRegisterInterruptHandler(ULONG Vector, INTERRUPT_HANDLER Handler, ULONG Flags)
{
    if (Vector >= 256 || Handler == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    KIRQL old_irql;
    KeAcquireSpinLock(&g_InterruptHandler.InterruptLock, &old_irql);

    if (Vector < 32) {
        g_InterruptHandler.FastInterruptHandlers[Vector] = Handler;
    } else {
        g_InterruptHandler.InterruptHandlers[Vector] = Handler;
    }

    KeReleaseSpinLock(&g_InterruptHandler.InterruptLock, old_irql);

    return STATUS_SUCCESS;
}

/**
 * @brief Unregister interrupt handler
 * @param Vector Interrupt vector
 * @return NTSTATUS Status code
 */
NTSTATUS KeUnregisterInterruptHandler(ULONG Vector)
{
    if (Vector >= 256) {
        return STATUS_INVALID_PARAMETER;
    }

    KIRQL old_irql;
    KeAcquireSpinLock(&g_InterruptHandler.InterruptLock, &old_irql);

    if (Vector < 32) {
        g_InterruptHandler.FastInterruptHandlers[Vector] = NULL;
    } else {
        g_InterruptHandler.InterruptHandlers[Vector] = NULL;
    }

    KeReleaseSpinLock(&g_InterruptHandler.InterruptLock, old_irql);

    return STATUS_SUCCESS;
}

/**
 * @brief Common interrupt handler entry point
 * @param Vector Interrupt vector
 * @param Context Interrupt context
 */
VOID KeInterruptHandler(ULONG Vector, PVOID Context)
{
    UNREFERENCED_PARAMETER(Context);

    // Update statistics
    InterlockedIncrement(&g_InterruptHandler.Statistics.TotalInterrupts);
    InterlockedIncrement(&g_InterruptHandler.Statistics.InterruptCounts[Vector]);

    // Increase nesting level
    InterlockedIncrement(&g_InterruptHandler.NestingLevel);

    // Disable interrupts
    HalDisableInterrupts();

    // Call appropriate handler
    INTERRUPT_HANDLER handler;
    if (Vector < 32) {
        handler = g_InterruptHandler.FastInterruptHandlers[Vector];
    } else {
        handler = g_InterruptHandler.InterruptHandlers[Vector];
    }

    if (handler != NULL) {
        // Call the handler
        handler(Vector);
    } else {
        // No handler registered, log spurious interrupt
        InterlockedIncrement(&g_InterruptHandler.Statistics.TotalSpuriousInterrupts);
    }

    // Send end of interrupt
    HalSendEndOfInterrupt(Vector);

    // Enable interrupts
    HalEnableInterrupts();

    // Decrease nesting level
    InterlockedDecrement(&g_InterruptHandler.NestingLevel);

    // Check for pending DPCs
    if (g_InterruptHandler.NestingLevel == 0 && !IsListEmpty(&g_InterruptHandler.DpcQueueHead)) {
        KeProcessDpcQueue();
    }
}

/**
 * @brief Timer interrupt handler
 * @param Vector Interrupt vector
 */
VOID KeTimerInterruptHandler(ULONG Vector)
{
    UNREFERENCED_PARAMETER(Vector);

    // Update scheduler
    KeHandleTimerInterrupt();

    // Update system time
    KeUpdateSystemTime();
}

/**
 * @brief Keyboard interrupt handler
 * @param Vector Interrupt vector
 */
VOID KeKeyboardInterruptHandler(ULONG Vector)
{
    UNREFERENCED_PARAMETER(Vector);

    // This is a simplified implementation
    // In a real implementation, this would:
    // - Read keyboard scancode
    // - Convert to character
    // - Queue keyboard event
    // - Signal keyboard driver
}

/**
 * @brief System call interrupt handler
 * @param Vector Interrupt vector
 */
VOID KeSystemCallInterruptHandler(ULONG Vector)
{
    UNREFERENCED_PARAMETER(Vector);

    // This is a simplified implementation
    // In a real implementation, this would:
    // - Extract system call number and parameters
    // - Call appropriate system call handler
    // - Return result to user mode
}

/**
 * @brief Register default interrupt handlers
 */
static VOID KeRegisterDefaultHandlers(VOID)
{
    // Register timer interrupt handler (typically IRQ 0)
    KeRegisterInterruptHandler(32, KeTimerInterruptHandler, 0);

    // Register keyboard interrupt handler (typically IRQ 1)
    KeRegisterInterruptHandler(33, KeKeyboardInterruptHandler, 0);

    // Register system call handler (typically INT 0x80)
    KeRegisterInterruptHandler(0x80, KeSystemCallInterruptHandler, 0);
}

/**
 * @brief Queue DPC
 * @param Dpc DPC structure
 * @param DeferredRoutine DPC routine
 * @param DeferredContext DPC context
 * @param Priority DPC priority
 */
VOID KeQueueDpc(PKDPC Dpc, PVOID DeferredRoutine, PVOID DeferredContext, ULONG Priority)
{
    if (Dpc == NULL || DeferredRoutine == NULL) {
        return;
    }

    // Initialize DPC
    Dpc->DeferredRoutine = DeferredRoutine;
    Dpc->DeferredContext = DeferredContext;
    Dpc->Priority = Priority;

    // Add to DPC queue
    KIRQL old_irql;
    KeAcquireSpinLock(&g_InterruptHandler.DpcLock, &old_irql);

    InsertTailList(&g_InterruptHandler.DpcQueueHead, &Dpc->DpcListEntry);
    g_InterruptHandler.DpcQueueDepth++;

    KeReleaseSpinLock(&g_InterruptHandler.DpcLock, old_irql);

    // Request software interrupt if not already processing DPCs
    if (!g_InterruptHandler.DpcProcessing) {
        HalRequestSoftwareInterrupt();
    }
}

/**
 * @brief Process DPC queue
 */
VOID KeProcessDpcQueue(VOID)
{
    g_InterruptHandler.DpcProcessing = TRUE;

    while (TRUE) {
        PKDPC dpc;
        KIRQL old_irql;

        // Get next DPC from queue
        KeAcquireSpinLock(&g_InterruptHandler.DpcLock, &old_irql);

        if (IsListEmpty(&g_InterruptHandler.DpcQueueHead)) {
            KeReleaseSpinLock(&g_InterruptHandler.DpcLock, old_irql);
            break;
        }

        PLIST_ENTRY entry = RemoveHeadList(&g_InterruptHandler.DpcQueueHead);
        g_InterruptHandler.DpcQueueDepth--;

        KeReleaseSpinLock(&g_InterruptHandler.DpcLock, old_irql);

        dpc = CONTAINING_RECORD(entry, KDPC, DpcListEntry);

        // Call DPC routine
        typedef VOID (*DPC_ROUTINE)(PVOID Context);
        DPC_ROUTINE routine = (DPC_ROUTINE)dpc->DeferredRoutine;
        routine(dpc->DeferredContext);

        InterlockedIncrement(&g_InterruptHandler.Statistics.DpcCount);
    }

    g_InterruptHandler.DpcProcessing = FALSE;
}

/**
 * @brief Get interrupt statistics
 * @param Statistics Statistics structure to fill
 */
VOID KeGetInterruptStatistics(PINTERRUPT_STATISTICS Statistics)
{
    if (Statistics == NULL) {
        return;
    }

    KIRQL old_irql;
    KeAcquireSpinLock(&g_InterruptHandler.InterruptLock, &old_irql);
    RtlCopyMemory(Statistics, &g_InterruptHandler.Statistics, sizeof(INTERRUPT_STATISTICS));
    KeReleaseSpinLock(&g_InterruptHandler.InterruptLock, old_irql);
}

/**
 * @brief Send end of interrupt
 * @param Vector Interrupt vector
 */
VOID HalSendEndOfInterrupt(ULONG Vector)
{
    // This is a simplified implementation
    // In a real implementation, this would send EOI to the appropriate interrupt controller

    UNREFERENCED_PARAMETER(Vector);
}

/**
 * @brief Request software interrupt
 */
VOID HalRequestSoftwareInterrupt(VOID)
{
    // This is a simplified implementation
    // In a real implementation, this would trigger a software interrupt
}

/**
 * @brief Update system time
 */
VOID KeUpdateSystemTime(VOID)
{
    // This is a simplified implementation
    // In a real implementation, this would update the system time from hardware timer
}

/**
 * @brief Mask interrupt
 * @param Vector Interrupt vector
 */
VOID KeMaskInterrupt(ULONG Vector)
{
    if (Vector >= 256) {
        return;
    }

    // This is a simplified implementation
    // In a real implementation, this would mask the interrupt in the interrupt controller
}

/**
 * @brief Unmask interrupt
 * @param Vector Interrupt vector
 */
VOID KeUnmaskInterrupt(ULONG Vector)
{
    if (Vector >= 256) {
        return;
    }

    // This is a simplified implementation
    // In a real implementation, this would unmask the interrupt in the interrupt controller
}