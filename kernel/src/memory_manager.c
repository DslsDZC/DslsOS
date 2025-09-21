/**
 * @file memory_manager.c
 * @brief Memory management subsystem implementation
 * @author DslsOS Team
 * @version 1.0
 * @date 2024
 */

#include "../include/kernel.h"
#include "../include/dslos.h"
#include <string.h>

// Memory manager state
typedef struct _MEMORY_MANAGER_STATE {
    BOOLEAN Initialized;
    KSPIN_LOCK MemoryLock;

    // Physical memory management
    PHYSICAL_MEMORY_RANGE* PhysicalMemoryRanges;
    ULONG PhysicalMemoryRangeCount;
    ULONG TotalPhysicalPages;
    ULONG FreePhysicalPages;

    // Page frame management
    PHYSICAL_PAGE_FRAME* PageFrameArray;
    ULONG PageFrameArraySize;
    LIST_ENTRY FreePageListHead;
    ULONG FreePageCount;

    // Virtual memory management
    PVOID KernelBaseAddress;
    SIZE_T KernelSize;
    PVOID KernelHeapBase;
    SIZE_T KernelHeapSize;
    LIST_ENTRY KernelHeapFreeListHead;

    // Memory statistics
    MEMORY_STATISTICS Statistics;

    // Memory pools
    typedef struct _MEMORY_POOL {
        POOL_TYPE PoolType;
        PVOID PoolBase;
        SIZE_T PoolSize;
        SIZE_T PoolUsed;
        LIST_ENTRY FreeBlockListHead;
        KSPIN_LOCK PoolLock;
    } MEMORY_POOL;

    MEMORY_POOL NonPagedPool;
    MEMORY_POOL PagedPool;

    // Address space management
    LIST_ENTRY AddressSpaceListHead;
    ULONG AddressSpaceCount;
} MEMORY_MANAGER_STATE;

static MEMORY_MANAGER_STATE g_MemoryManager = {0};

// Physical page frame structure
typedef struct _PHYSICAL_PAGE_FRAME {
    ULONG_PTR PhysicalAddress;
    ULONG ReferenceCount;
    ULONG Flags;
    PVOID VirtualMapping;
    LIST_ENTRY PageListEntry;
} PHYSICAL_PAGE_FRAME, *PPHYSICAL_PAGE_FRAME;

// Physical memory range structure
typedef struct _PHYSICAL_MEMORY_RANGE {
    ULONG_PTR BaseAddress;
    SIZE_T Size;
    ULONG Type;
} PHYSICAL_MEMORY_RANGE, *PPHYSICAL_MEMORY_RANGE;

// Memory statistics structure
typedef struct _MEMORY_STATISTICS {
    ULONG TotalPhysicalPages;
    ULONG FreePhysicalPages;
    ULONG ReservedPages;
    ULONG KernelPages;
    ULONG NonPagedPoolPages;
    ULONG PagedPoolPages;
    ULONG PageFaultCount;
    ULONG PageInCount;
    ULONG PageOutCount;
} MEMORY_STATISTICS, *PMEMORY_STATISTICS;

// Memory pool block header
typedef struct _MEMORY_POOL_BLOCK {
    SIZE_T BlockSize;
    ULONG Flags;
    LIST_ENTRY ListEntry;
} MEMORY_POOL_BLOCK, *PMEMORY_POOL_BLOCK;

// Address space descriptor
typedef struct _ADDRESS_SPACE_DESCRIPTOR {
    PPROCESS_CONTROL_BLOCK Process;
    PVOID PageDirectory;
    LIST_ENTRY AddressSpaceListEntry;
    ULONG RegionCount;
} ADDRESS_SPACE_DESCRIPTOR, *PADDRESS_SPACE_DESCRIPTOR;

// Virtual memory region
typedef struct _VIRTUAL_MEMORY_REGION {
    PVOID BaseAddress;
    SIZE_T RegionSize;
    ULONG Protect;
    ULONG State;
    ULONG Type;
    LIST_ENTRY RegionListEntry;
} VIRTUAL_MEMORY_REGION, *PVIRTUAL_MEMORY_REGION;

// Memory protection flags
#define PAGE_NOACCESS          0x01
#define PAGE_READONLY          0x02
#define PAGE_READWRITE         0x04
#define PAGE_WRITECOPY         0x08
#define PAGE_EXECUTE           0x10
#define PAGE_EXECUTE_READ       0x20
#define PAGE_EXECUTE_READWRITE  0x40
#define PAGE_EXECUTE_WRITECOPY  0x80

// Memory allocation types
#define MEM_COMMIT             0x00001000
#define MEM_RESERVE            0x00002000
#define MEM_DECOMMIT           0x00004000
#define MEM_RELEASE            0x00008000
#define MEM_FREE               0x00010000
#define MEM_PRIVATE            0x00020000
#define MEM_MAPPED             0x00040000

// Memory state flags
#define MEM_STATE_FREE         0x10000
#define MEM_STATE_RESERVED     0x20000
#define MEM_STATE_COMMITTED    0x40000

/**
 * @brief Initialize memory manager
 * @return NTSTATUS Status code
 */
NTSTATUS MmInitializeMemoryManager(VOID)
{
    if (g_MemoryManager.Initialized) {
        return STATUS_SUCCESS;
    }

    KeInitializeSpinLock(&g_MemoryManager.MemoryLock);

    // Initialize physical memory manager
    NTSTATUS status = MmInitializePhysicalMemory();
    if (!NT_SUCCESS(status)) {
        return status;
    }

    // Initialize virtual memory manager
    status = MmInitializeVirtualMemory();
    if (!NT_SUCCESS(status)) {
        return status;
    }

    // Initialize memory pools
    status = MmInitializeMemoryPools();
    if (!NT_SUCCESS(status)) {
        return status;
    }

    // Initialize kernel heap
    status = MmInitializeKernelHeap();
    if (!NT_SUCCESS(status)) {
        return status;
    }

    g_MemoryManager.Initialized = TRUE;
    return STATUS_SUCCESS;
}

/**
 * @brief Initialize physical memory management
 * @return NTSTATUS Status code
 */
static NTSTATUS MmInitializePhysicalMemory(VOID)
{
    // Detect physical memory ranges
    NTSTATUS status = MmDetectPhysicalMemoryRanges();
    if (!NT_SUCCESS(status)) {
        return status;
    }

    // Initialize page frame array
    status = MmInitializePageFrameArray();
    if (!NT_SUCCESS(status)) {
        return status;
    }

    // Initialize free page list
    InitializeListHead(&g_MemoryManager.FreePageListHead);
    g_MemoryManager.FreePageCount = 0;

    // Add initial free pages
    for (ULONG i = 0; i < g_MemoryManager.PageFrameArraySize; i++) {
        PPHYSICAL_PAGE_FRAME page = &g_MemoryManager.PageFrameArray[i];
        if (page->Flags & PAGE_FLAG_AVAILABLE) {
            InsertTailList(&g_MemoryManager.FreePageListHead, &page->PageListEntry);
            g_MemoryManager.FreePageCount++;
        }
    }

    g_MemoryManager.FreePhysicalPages = g_MemoryManager.FreePageCount;

    return STATUS_SUCCESS;
}

/**
 * @brief Detect physical memory ranges
 * @return NTSTATUS Status code
 */
static NTSTATUS MmDetectPhysicalMemoryRanges(VOID)
{
    // This is a simplified implementation
    // In a real implementation, this would:
    // - Query BIOS/UEFI memory map
    // - Parse ACPI tables
    // - Detect available memory ranges

    // For demonstration, we'll create a simple memory map
    g_MemoryManager.PhysicalMemoryRangeCount = 2;
    g_MemoryManager.PhysicalMemoryRanges = ExAllocatePool(NonPagedPool,
        sizeof(PHYSICAL_MEMORY_RANGE) * g_MemoryManager.PhysicalMemoryRangeCount);

    if (g_MemoryManager.PhysicalMemoryRanges == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // First 1MB is typically reserved
    g_MemoryManager.PhysicalMemoryRanges[0].BaseAddress = 0x00000000;
    g_MemoryManager.PhysicalMemoryRanges[0].Size = 0x100000;
    g_MemoryManager.PhysicalMemoryRanges[0].Type = MEMORY_TYPE_RESERVED;

    // Available memory starting at 1MB
    g_MemoryManager.PhysicalMemoryRanges[1].BaseAddress = 0x00100000;
    g_MemoryManager.PhysicalMemoryRanges[1].Size = 0x3FF00000; // ~1GB total
    g_MemoryManager.PhysicalMemoryRanges[1].Type = MEMORY_TYPE_AVAILABLE;

    return STATUS_SUCCESS;
}

/**
 * @brief Initialize page frame array
 * @return NTSTATUS Status code
 */
static NTSTATUS MmInitializePageFrameArray(VOID)
{
    // Calculate total physical pages
    SIZE_T total_physical_memory = 0;
    for (ULONG i = 0; i < g_MemoryManager.PhysicalMemoryRangeCount; i++) {
        if (g_MemoryManager.PhysicalMemoryRanges[i].Type == MEMORY_TYPE_AVAILABLE) {
            total_physical_memory += g_MemoryManager.PhysicalMemoryRanges[i].Size;
        }
    }

    g_MemoryManager.TotalPhysicalPages = (ULONG)(total_physical_memory / DSLOS_PAGE_SIZE);
    g_MemoryManager.PageFrameArraySize = g_MemoryManager.TotalPhysicalPages;

    // Allocate page frame array
    g_MemoryManager.PageFrameArray = ExAllocatePool(NonPagedPool,
        sizeof(PHYSICAL_PAGE_FRAME) * g_MemoryManager.PageFrameArraySize);

    if (g_MemoryManager.PageFrameArray == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Initialize page frame array
    RtlZeroMemory(g_MemoryManager.PageFrameArray,
        sizeof(PHYSICAL_PAGE_FRAME) * g_MemoryManager.PageFrameArraySize);

    ULONG_PTR current_address = 0;
    for (ULONG i = 0; i < g_MemoryManager.PageFrameArraySize; i++) {
        g_MemoryManager.PageFrameArray[i].PhysicalAddress = current_address;
        g_MemoryManager.PageFrameArray[i].ReferenceCount = 0;
        g_MemoryManager.PageFrameArray[i].Flags = 0;
        g_MemoryManager.PageFrameArray[i].VirtualMapping = NULL;
        InitializeListHead(&g_MemoryManager.PageFrameArray[i].PageListEntry);

        // Determine if page is available
        BOOLEAN is_available = FALSE;
        for (ULONG j = 0; j < g_MemoryManager.PhysicalMemoryRangeCount; j++) {
            if (current_address >= g_MemoryManager.PhysicalMemoryRanges[j].BaseAddress &&
                current_address < g_MemoryManager.PhysicalMemoryRanges[j].BaseAddress +
                                  g_MemoryManager.PhysicalMemoryRanges[j].Size) {
                if (g_MemoryManager.PhysicalMemoryRanges[j].Type == MEMORY_TYPE_AVAILABLE) {
                    is_available = TRUE;
                }
                break;
            }
        }

        if (is_available) {
            g_MemoryManager.PageFrameArray[i].Flags |= PAGE_FLAG_AVAILABLE;
        }

        current_address += DSLOS_PAGE_SIZE;
    }

    return STATUS_SUCCESS;
}

/**
 * @brief Initialize virtual memory management
 * @return NTSTATUS Status code
 */
static NTSTATUS MmInitializeVirtualMemory(VOID)
{
    // Initialize address space list
    InitializeListHead(&g_MemoryManager.AddressSpaceListHead);
    g_MemoryManager.AddressSpaceCount = 0;

    return STATUS_SUCCESS;
}

/**
 * @brief Initialize memory pools
 * @return NTSTATUS Status code
 */
static NTSTATUS MmInitializeMemoryPools(VOID)
{
    // Initialize non-paged pool
    g_MemoryManager.NonPagedPool.PoolType = NonPagedPool;
    g_MemoryManager.NonPagedPool.PoolBase = ExAllocatePool(NonPagedPool, 16 * 1024 * 1024); // 16MB
    g_MemoryManager.NonPagedPool.PoolSize = 16 * 1024 * 1024;
    g_MemoryManager.NonPagedPool.PoolUsed = 0;
    InitializeListHead(&g_MemoryManager.NonPagedPool.FreeBlockListHead);
    KeInitializeSpinLock(&g_MemoryManager.NonPagedPool.PoolLock);

    // Initialize non-paged pool with one large free block
    PMEMORY_POOL_BLOCK initial_block = (PMEMORY_POOL_BLOCK)g_MemoryManager.NonPagedPool.PoolBase;
    initial_block->BlockSize = g_MemoryManager.NonPagedPool.PoolSize - sizeof(MEMORY_POOL_BLOCK);
    initial_block->Flags = 0;
    InitializeListHead(&initial_block->ListEntry);
    InsertTailList(&g_MemoryManager.NonPagedPool.FreeBlockListHead, &initial_block->ListEntry);

    // Initialize paged pool (simplified)
    g_MemoryManager.PagedPool.PoolType = PagedPool;
    g_MemoryManager.PagedPool.PoolBase = ExAllocatePool(PagedPool, 32 * 1024 * 1024); // 32MB
    g_MemoryManager.PagedPool.PoolSize = 32 * 1024 * 1024;
    g_MemoryManager.PagedPool.PoolUsed = 0;
    InitializeListHead(&g_MemoryManager.PagedPool.FreeBlockListHead);
    KeInitializeSpinLock(&g_MemoryManager.PagedPool.PoolLock);

    return STATUS_SUCCESS;
}

/**
 * @brief Initialize kernel heap
 * @return NTSTATUS Status code
 */
static NTSTATUS MmInitializeKernelHeap(VOID)
{
    // Kernel heap is part of non-paged pool
    g_MemoryManager.KernelHeapBase = g_MemoryManager.NonPagedPool.PoolBase;
    g_MemoryManager.KernelHeapSize = 8 * 1024 * 1024; // 8MB
    InitializeListHead(&g_MemoryManager.KernelHeapFreeListHead);

    return STATUS_SUCCESS;
}

/**
 * @brief Allocate physical memory
 * @param Size Size to allocate
 * @return Pointer to allocated physical memory
 */
PVOID MmAllocatePhysicalMemory(SIZE_T Size)
{
    if (Size == 0) {
        return NULL;
    }

    SIZE_T page_count = (Size + DSLOS_PAGE_SIZE - 1) / DSLOS_PAGE_SIZE;
    PVOID physical_memory = NULL;

    KIRQL old_irql;
    KeAcquireSpinLock(&g_MemoryManager.MemoryLock, &old_irql);

    if (g_MemoryManager.FreePageCount < page_count) {
        KeReleaseSpinLock(&g_MemoryManager.MemoryLock, old_irql);
        return NULL; // Out of memory
    }

    // Allocate contiguous pages (simplified)
    ULONG pages_allocated = 0;
    for (ULONG i = 0; i < g_MemoryManager.PageFrameArraySize && pages_allocated < page_count; i++) {
        PPHYSICAL_PAGE_FRAME page = &g_MemoryManager.PageFrameArray[i];
        if (page->Flags & PAGE_FLAG_AVAILABLE) {
            page->Flags &= ~PAGE_FLAG_AVAILABLE;
            page->ReferenceCount = 1;
            RemoveEntryList(&page->PageListEntry);
            g_MemoryManager.FreePageCount--;
            g_MemoryManager.FreePhysicalPages--;

            if (physical_memory == NULL) {
                physical_memory = (PVOID)page->PhysicalAddress;
            }

            pages_allocated++;
        }
    }

    KeReleaseSpinLock(&g_MemoryManager.MemoryLock, old_irql);

    return physical_memory;
}

/**
 * @brief Free physical memory
 * @param Address Address to free
 * @param Size Size to free
 */
VOID MmFreePhysicalMemory(PVOID Address, SIZE_T Size)
{
    if (Address == NULL || Size == 0) {
        return;
    }

    SIZE_T page_count = (Size + DSLOS_PAGE_SIZE - 1) / DSLOS_PAGE_SIZE;
    ULONG_PTR base_address = (ULONG_PTR)Address;

    KIRQL old_irql;
    KeAcquireSpinLock(&g_MemoryManager.MemoryLock, &old_irql);

    for (SIZE_T i = 0; i < page_count; i++) {
        ULONG_PTR page_address = base_address + (i * DSLOS_PAGE_SIZE);
        ULONG page_index = (ULONG)(page_address / DSLOS_PAGE_SIZE);

        if (page_index < g_MemoryManager.PageFrameArraySize) {
            PPHYSICAL_PAGE_FRAME page = &g_MemoryManager.PageFrameArray[page_index];
            if (page->ReferenceCount > 0) {
                page->ReferenceCount--;
                if (page->ReferenceCount == 0) {
                    page->Flags |= PAGE_FLAG_AVAILABLE;
                    page->VirtualMapping = NULL;
                    InsertTailList(&g_MemoryManager.FreePageListHead, &page->PageListEntry);
                    g_MemoryManager.FreePageCount++;
                    g_MemoryManager.FreePhysicalPages++;
                }
            }
        }
    }

    KeReleaseSpinLock(&g_MemoryManager.MemoryLock, old_irql);
}

/**
 * @brief Allocate virtual memory
 * @param Process Process to allocate for
 * @param BaseAddress Base address (can be NULL)
 * @param Size Size to allocate
 * @param Protect Memory protection flags
 * @return Pointer to allocated virtual memory
 */
PVOID MmAllocateVirtualMemory(PPROCESS_CONTROL_BLOCK Process, PVOID BaseAddress, SIZE_T Size, ULONG Protect)
{
    if (Size == 0) {
        return NULL;
    }

    // Align size to page boundary
    SIZE_T aligned_size = (Size + DSLOS_PAGE_SIZE - 1) & ~(DSLOS_PAGE_SIZE - 1);

    // Find free virtual address space (simplified)
    PVOID virtual_address = BaseAddress;
    if (virtual_address == NULL) {
        virtual_address = MmFindFreeVirtualAddress(Process, aligned_size);
        if (virtual_address == NULL) {
            return NULL; // Out of virtual address space
        }
    }

    // Allocate physical pages
    PVOID physical_memory = MmAllocatePhysicalMemory(aligned_size);
    if (physical_memory == NULL) {
        return NULL; // Out of physical memory
    }

    // Map physical pages to virtual address
    NTSTATUS status = MmMapPhysicalMemory(Process, virtual_address, physical_memory, aligned_size, Protect);
    if (!NT_SUCCESS(status)) {
        MmFreePhysicalMemory(physical_memory, aligned_size);
        return NULL;
    }

    return virtual_address;
}

/**
 * @brief Free virtual memory
 * @param Process Process to free for
 * @param Address Address to free
 * @param Size Size to free
 */
VOID MmFreeVirtualMemory(PPROCESS_CONTROL_BLOCK Process, PVOID Address, SIZE_T Size)
{
    if (Address == NULL || Size == 0) {
        return;
    }

    // Unmap virtual memory
    SIZE_T aligned_size = (Size + DSLOS_PAGE_SIZE - 1) & ~(DSLOS_PAGE_SIZE - 1);
    PVOID physical_memory = MmUnmapVirtualMemory(Process, Address, aligned_size);

    if (physical_memory != NULL) {
        MmFreePhysicalMemory(physical_memory, aligned_size);
    }
}

/**
 * @brief Find free virtual address space
 * @param Process Process to find address for
 * @param Size Size needed
 * @return Free virtual address
 */
static PVOID MmFindFreeVirtualAddress(PPROCESS_CONTROL_BLOCK Process, SIZE_T Size)
{
    // This is a simplified implementation
    // In a real implementation, this would scan the process's address space

    PVOID base_address = (PVOID)0x10000000; // Start at 256MB for user mode
    PVOID end_address = (PVOID)0x7FFFFFFF;   // End at 2GB-1 for user mode

    while ((UINT_PTR)base_address + Size <= (UINT_PTR)end_address) {
        // Check if address range is free (simplified check)
        if (MmIsAddressRangeFree(Process, base_address, Size)) {
            return base_address;
        }
        base_address = (PVOID)((UINT_PTR)base_address + DSLOS_PAGE_SIZE);
    }

    return NULL;
}

/**
 * @brief Check if address range is free
 * @param Process Process to check
 * @param BaseAddress Base address to check
 * @param Size Size to check
 * @return TRUE if free, FALSE otherwise
 */
static BOOLEAN MmIsAddressRangeFree(PPROCESS_CONTROL_BLOCK Process, PVOID BaseAddress, SIZE_T Size)
{
    // This is a simplified implementation
    // In a real implementation, this would check the process's virtual memory regions

    // For now, just return TRUE
    return TRUE;
}

/**
 * @brief Map physical memory to virtual address
 * @param Process Process to map for
 * @param VirtualAddress Virtual address
 * @param PhysicalAddress Physical address
 * @param Size Size to map
 * @param Protect Memory protection flags
 * @return NTSTATUS Status code
 */
static NTSTATUS MmMapPhysicalMemory(PPROCESS_CONTROL_BLOCK Process, PVOID VirtualAddress,
                                    PVOID PhysicalAddress, SIZE_T Size, ULONG Protect)
{
    // This is a simplified implementation
    // In a real implementation, this would:
    // - Update page tables
    // - Set proper permissions
    // - Flush TLB if necessary

    UNREFERENCED_PARAMETER(Process);
    UNREFERENCED_PARAMETER(VirtualAddress);
    UNREFERENCED_PARAMETER(PhysicalAddress);
    UNREFERENCED_PARAMETER(Size);
    UNREFERENCED_PARAMETER(Protect);

    return STATUS_SUCCESS;
}

/**
 * @brief Unmap virtual memory
 * @param Process Process to unmap for
 * @param VirtualAddress Virtual address
 * @param Size Size to unmap
 * @return Physical address that was mapped
 */
static PVOID MmUnmapVirtualMemory(PPROCESS_CONTROL_BLOCK Process, PVOID VirtualAddress, SIZE_T Size)
{
    // This is a simplified implementation
    // In a real implementation, this would:
    // - Update page tables
    // - Flush TLB if necessary
    // - Return the physical address

    UNREFERENCED_PARAMETER(Process);
    UNREFERENCED_PARAMETER(VirtualAddress);
    UNREFERENCED_PARAMETER(Size);

    return NULL;
}

/**
 * @brief Get memory statistics
 * @param Statistics Memory statistics structure
 */
VOID MmGetMemoryStatistics(PMEMORY_STATISTICS Statistics)
{
    if (Statistics == NULL) {
        return;
    }

    KIRQL old_irql;
    KeAcquireSpinLock(&g_MemoryManager.MemoryLock, &old_irql);

    RtlCopyMemory(Statistics, &g_MemoryManager.Statistics, sizeof(MEMORY_STATISTICS));

    Statistics->TotalPhysicalPages = g_MemoryManager.TotalPhysicalPages;
    Statistics->FreePhysicalPages = g_MemoryManager.FreePhysicalPages;

    KeReleaseSpinLock(&g_MemoryManager.MemoryLock, old_irql);
}

/**
 * @brief Create address space for process
 * @param Process Process to create address space for
 * @return NTSTATUS Status code
 */
NTSTATUS MmCreateAddressSpace(PPROCESS_CONTROL_BLOCK Process)
{
    if (Process == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    // Allocate page directory
    PVOID page_directory = MmAllocatePhysicalMemory(DSLOS_PAGE_SIZE);
    if (page_directory == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Initialize page directory (simplified)
    RtlZeroMemory(page_directory, DSLOS_PAGE_SIZE);

    // Create address space descriptor
    PADDRESS_SPACE_DESCRIPTOR descriptor = ExAllocatePool(NonPagedPool, sizeof(ADDRESS_SPACE_DESCRIPTOR));
    if (descriptor == NULL) {
        MmFreePhysicalMemory(page_directory, DSLOS_PAGE_SIZE);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    descriptor->Process = Process;
    descriptor->PageDirectory = page_directory;
    descriptor->RegionCount = 0;
    InitializeListHead(&descriptor->AddressSpaceListEntry);

    // Add to global address space list
    KIRQL old_irql;
    KeAcquireSpinLock(&g_MemoryManager.MemoryLock, &old_irql);
    InsertTailList(&g_MemoryManager.AddressSpaceListHead, &descriptor->AddressSpaceListEntry);
    g_MemoryManager.AddressSpaceCount++;
    KeReleaseSpinLock(&g_MemoryManager.MemoryLock, old_irql);

    Process->PageDirectory = page_directory;
    return STATUS_SUCCESS;
}

/**
 * @brief Destroy address space for process
 * @param Process Process to destroy address space for
 */
NTSTATUS MmDestroyAddressSpace(PPROCESS_CONTROL_BLOCK Process)
{
    if (Process == NULL || Process->PageDirectory == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    // Find and remove address space descriptor
    PADDRESS_SPACE_DESCRIPTOR descriptor = NULL;
    KIRQL old_irql;
    KeAcquireSpinLock(&g_MemoryManager.MemoryLock, &old_irql);

    PLIST_ENTRY entry = g_MemoryManager.AddressSpaceListHead.Flink;
    while (entry != &g_MemoryManager.AddressSpaceListHead) {
        descriptor = CONTAINING_RECORD(entry, ADDRESS_SPACE_DESCRIPTOR, AddressSpaceListEntry);
        if (descriptor->Process == Process) {
            RemoveEntryList(&descriptor->AddressSpaceListEntry);
            g_MemoryManager.AddressSpaceCount--;
            break;
        }
        entry = entry->Flink;
    }

    KeReleaseSpinLock(&g_MemoryManager.MemoryLock, old_irql);

    if (descriptor != NULL) {
        // Free page directory
        MmFreePhysicalMemory(descriptor->PageDirectory, DSLOS_PAGE_SIZE);
        ExFreePool(descriptor);
        Process->PageDirectory = NULL;
    }

    return STATUS_SUCCESS;
}