/**
 * @file object_manager.c
 * @brief Kernel object management implementation
 * @author DslsOS Team
 * @version 1.0
 * @date 2024
 */

#include "../include/kernel.h"
#include "../include/dslos.h"
#include <string.h>

// Object manager state
typedef struct _OBJECT_MANAGER_STATE {
    BOOLEAN Initialized;
    KSPIN_LOCK ObjectLock;
    LIST_ENTRY ObjectListHead;
    ULONG ObjectCount;
    ULONG ObjectTypeCount;

    // Object type information
    typedef struct _OBJECT_TYPE {
        UNICODE_STRING TypeName;
        ULONG TotalObjects;
        ULONG TotalHandles;
        POOL_TYPE PoolType;
        ULONG ValidAccessMask;
        PVOID DefaultObject;
        PVOID ParseProcedure;
        PVOID DeleteProcedure;
        LIST_ENTRY TypeListEntry;
    } OBJECT_TYPE, *POBJECT_TYPE;

    OBJECT_TYPE ObjectTypes[KERNEL_OBJECT_TYPE_MAX];

    // Handle table
    typedef struct _HANDLE_TABLE {
        PVOID Table;
        ULONG TableSize;
        ULONG HandleCount;
        KSPIN_LOCK TableLock;
    } HANDLE_TABLE;

    HANDLE_TABLE GlobalHandleTable;
} OBJECT_MANAGER_STATE;

static OBJECT_MANAGER_STATE g_ObjectManager = {0};

/**
 * @brief Initialize object manager
 * @return NTSTATUS Status code
 */
NTSTATUS ObInitializeObjectManager(VOID)
{
    if (g_ObjectManager.Initialized) {
        return STATUS_SUCCESS;
    }

    // Initialize object lock
    KeInitializeSpinLock(&g_ObjectManager.ObjectLock);

    // Initialize object list
    InitializeListHead(&g_ObjectManager.ObjectListHead);
    g_ObjectManager.ObjectCount = 0;
    g_ObjectManager.ObjectTypeCount = 0;

    // Initialize handle table
    NTSTATUS status = ObInitializeHandleTable(&g_ObjectManager.GlobalHandleTable);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    // Initialize object types
    status = ObInitializeObjectTypes();
    if (!NT_SUCCESS(status)) {
        return status;
    }

    g_ObjectManager.Initialized = TRUE;
    return STATUS_SUCCESS;
}

/**
 * @brief Create a kernel object
 * @param Type Object type
 * @param ObjectSize Size of object
 * @param Object Pointer to receive object pointer
 * @return NTSTATUS Status code
 */
NTSTATUS ObCreateObject(KERNEL_OBJECT_TYPE Type, SIZE_T ObjectSize, PKERNEL_OBJECT* Object)
{
    if (Type >= KERNEL_OBJECT_TYPE_MAX || ObjectSize < sizeof(KERNEL_OBJECT)) {
        return STATUS_INVALID_PARAMETER;
    }

    if (!g_ObjectManager.Initialized) {
        return STATUS_UNSUCCESSFUL;
    }

    // Allocate memory for object
    PKERNEL_OBJECT new_object = (PKERNEL_OBJECT)ExAllocatePool(NonPagedPool, ObjectSize);
    if (new_object == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Initialize object header
    RtlZeroMemory(new_object, ObjectSize);
    new_object->ObjectType = Type;
    new_object->ReferenceCount = 1;
    new_object->Flags = 0;
    new_object->SecurityDescriptor = NULL;

    // Initialize list entry
    InitializeListHead(&new_object->ObjectListEntry);

    // Add to global object list
    KIRQL old_irql;
    KeAcquireSpinLock(&g_ObjectManager.ObjectLock, &old_irql);
    InsertTailList(&g_ObjectManager.ObjectListHead, &new_object->ObjectListEntry);
    g_ObjectManager.ObjectCount++;
    KeReleaseSpinLock(&g_ObjectManager.ObjectLock, old_irql);

    // Update object type statistics
    g_ObjectManager.ObjectTypes[Type].TotalObjects++;

    *Object = new_object;
    return STATUS_SUCCESS;
}

/**
 * @brief Reference an object (increase reference count)
 * @param Object Object to reference
 */
VOID ObReferenceObject(PKERNEL_OBJECT Object)
{
    if (Object == NULL) {
        return;
    }

    InterlockedIncrement(&Object->ReferenceCount);
}

/**
 * @brief Dereference an object (decrease reference count)
 * @param Object Object to dereference
 */
VOID ObDereferenceObject(PKERNEL_OBJECT Object)
{
    if (Object == NULL) {
        return;
    }

    LONG new_count = InterlockedDecrement(&Object->ReferenceCount);
    if (new_count <= 0) {
        // Object should be deleted
        ObDeleteObject(Object);
    }
}

/**
 * @brief Delete an object
 * @param Object Object to delete
 */
VOID ObDeleteObject(PKERNEL_OBJECT Object)
{
    if (Object == NULL) {
        return;
    }

    // Remove from global object list
    KIRQL old_irql;
    KeAcquireSpinLock(&g_ObjectManager.ObjectLock, &old_irql);
    RemoveEntryList(&Object->ObjectListEntry);
    g_ObjectManager.ObjectCount--;
    KeReleaseSpinLock(&g_ObjectManager.ObjectLock, old_irql);

    // Update object type statistics
    if (Object->ObjectType < KERNEL_OBJECT_TYPE_MAX) {
        g_ObjectManager.ObjectTypes[Object->ObjectType].TotalObjects--;
    }

    // Free security descriptor if exists
    if (Object->SecurityDescriptor != NULL) {
        ExFreePool(Object->SecurityDescriptor);
    }

    // Call type-specific delete procedure if exists
    if (Object->ObjectType < KERNEL_OBJECT_TYPE_MAX &&
        g_ObjectManager.ObjectTypes[Object->ObjectType].DeleteProcedure != NULL) {
        typedef VOID (*DELETE_PROCEDURE)(PKERNEL_OBJECT);
        DELETE_PROCEDURE delete_proc = (DELETE_PROCEDURE)
            g_ObjectManager.ObjectTypes[Object->ObjectType].DeleteProcedure;
        delete_proc(Object);
    }

    // Free object memory
    ExFreePool(Object);
}

/**
 * @brief Get object by name
 * @param Name Object name
 * @param Object Pointer to receive object pointer
 * @return NTSTATUS Status code
 */
NTSTATUS ObGetObjectByName(PUNICODE_STRING Name, PKERNEL_OBJECT* Object)
{
    if (Name == NULL || Name->Buffer == NULL || Name->Length == 0 || Object == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    if (!g_ObjectManager.Initialized) {
        return STATUS_UNSUCCESSFUL;
    }

    KIRQL old_irql;
    KeAcquireSpinLock(&g_ObjectManager.ObjectLock, &old_irql);

    // Search through object list
    PLIST_ENTRY entry = g_ObjectManager.ObjectListHead.Flink;
    while (entry != &g_ObjectManager.ObjectListHead) {
        PKERNEL_OBJECT obj = CONTAINING_RECORD(entry, KERNEL_OBJECT, ObjectListEntry);

        // Check if object has a name (this is simplified)
        // In a real implementation, objects would have name fields
        entry = entry->Flink;
    }

    KeReleaseSpinLock(&g_ObjectManager.ObjectLock, old_irql);

    *Object = NULL;
    return STATUS_OBJECT_NAME_NOT_FOUND;
}

/**
 * @brief Initialize handle table
 * @param HandleTable Handle table to initialize
 * @return NTSTATUS Status code
 */
NTSTATUS ObInitializeHandleTable(PHANDLE_TABLE HandleTable)
{
    if (HandleTable == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    // Allocate initial handle table
    ULONG table_size = 1024 * sizeof(HANDLE); // Start with 1024 handles
    PVOID table = ExAllocatePool(NonPagedPool, table_size);
    if (table == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(table, table_size);

    HandleTable->Table = table;
    HandleTable->TableSize = 1024;
    HandleTable->HandleCount = 0;
    KeInitializeSpinLock(&HandleTable->TableLock);

    return STATUS_SUCCESS;
}

/**
 * @brief Create handle for object
 * @param Object Object to create handle for
 * @param DesiredAccess Desired access rights
 * @param Handle Pointer to receive handle
 * @return NTSTATUS Status code
 */
NTSTATUS ObCreateHandle(PKERNEL_OBJECT Object, ACCESS_MASK DesiredAccess, PHANDLE Handle)
{
    if (Object == NULL || Handle == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    // This is a simplified implementation
    // In a real implementation, this would:
    // - Find free slot in handle table
    // - Create handle entry with object and access rights
    // - Return handle value

    *Handle = (HANDLE)((UINT_PTR)Object | 0x3); // Simple handle encoding
    return STATUS_SUCCESS;
}

/**
 * @brief Get object from handle
 * @param Handle Handle to object
 * @param Object Pointer to receive object pointer
 * @return NTSTATUS Status code
 */
NTSTATUS ObReferenceObjectByHandle(HANDLE Handle, ACCESS_MASK DesiredAccess, PKERNEL_OBJECT* Object)
{
    if (Handle == NULL || Object == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    // Simple handle decoding
    PKERNEL_OBJECT obj = (PKERNEL_OBJECT)((UINT_PTR)Handle & ~0x3);

    // Verify object (simplified)
    if (obj == NULL) {
        return STATUS_INVALID_HANDLE;
    }

    // Reference object
    ObReferenceObject(obj);
    *Object = obj;

    return STATUS_SUCCESS;
}

/**
 * @brief Close handle
 * @param Handle Handle to close
 * @return NTSTATUS Status code
 */
NTSTATUS ObCloseHandle(HANDLE Handle)
{
    if (Handle == NULL) {
        return STATUS_INVALID_HANDLE;
    }

    PKERNEL_OBJECT obj;
    NTSTATUS status = ObReferenceObjectByHandle(Handle, 0, &obj);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    // Dereference object
    ObDereferenceObject(obj);

    return STATUS_SUCCESS;
}

/**
 * @brief Initialize object types
 * @return NTSTATUS Status code
 */
static NTSTATUS ObInitializeObjectTypes(VOID)
{
    // Initialize process object type
    RtlInitUnicodeString(&g_ObjectManager.ObjectTypes[KERNEL_OBJECT_TYPE_PROCESS].TypeName, L"Process");
    g_ObjectManager.ObjectTypes[KERNEL_OBJECT_TYPE_PROCESS].TotalObjects = 0;
    g_ObjectManager.ObjectTypes[KERNEL_OBJECT_TYPE_PROCESS].TotalHandles = 0;
    g_ObjectManager.ObjectTypes[KERNEL_OBJECT_TYPE_PROCESS].PoolType = NonPagedPool;
    g_ObjectManager.ObjectTypes[KERNEL_OBJECT_TYPE_PROCESS].ValidAccessMask = 0x1F0001;

    // Initialize thread object type
    RtlInitUnicodeString(&g_ObjectManager.ObjectTypes[KERNEL_OBJECT_TYPE_THREAD].TypeName, L"Thread");
    g_ObjectManager.ObjectTypes[KERNEL_OBJECT_TYPE_THREAD].TotalObjects = 0;
    g_ObjectManager.ObjectTypes[KERNEL_OBJECT_TYPE_THREAD].TotalHandles = 0;
    g_ObjectManager.ObjectTypes[KERNEL_OBJECT_TYPE_THREAD].PoolType = NonPagedPool;
    g_ObjectManager.ObjectTypes[KERNEL_OBJECT_TYPE_THREAD].ValidAccessMask = 0x1F03FF;

    // Initialize other object types
    for (ULONG i = KERNEL_OBJECT_TYPE_FILE; i < KERNEL_OBJECT_TYPE_MAX; i++) {
        RtlZeroMemory(&g_ObjectManager.ObjectTypes[i], sizeof(OBJECT_TYPE));
    }

    g_ObjectManager.ObjectTypeCount = KERNEL_OBJECT_TYPE_MAX;
    return STATUS_SUCCESS;
}

/**
 * @brief Query object information
 * @param Object Object to query
 * @param ObjectInformation Information buffer
 * @param InformationLength Length of information buffer
 * @param ReturnLength Returned length
 * @return NTSTATUS Status code
 */
NTSTATUS ObQueryObjectInformation(PKERNEL_OBJECT Object, PVOID ObjectInformation,
                                   ULONG InformationLength, PULONG ReturnLength)
{
    if (Object == NULL || ObjectInformation == NULL || ReturnLength == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    // This is a simplified implementation
    // In a real implementation, this would return detailed object information

    *ReturnLength = sizeof(KERNEL_OBJECT);
    if (InformationLength < sizeof(KERNEL_OBJECT)) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    RtlCopyMemory(ObjectInformation, Object, sizeof(KERNEL_OBJECT));
    return STATUS_SUCCESS;
}

/**
 * @brief Set object security
 * @param Object Object to secure
 * @param SecurityInformation Security information
 * @param SecurityDescriptor Security descriptor
 * @return NTSTATUS Status code
 */
NTSTATUS ObSetObjectSecurity(PKERNEL_OBJECT Object, ULONG SecurityInformation,
                            PSECURITY_DESCRIPTOR SecurityDescriptor)
{
    if (Object == NULL || SecurityDescriptor == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    // This is a simplified implementation
    // In a real implementation, this would handle complex security descriptor operations

    // Free old security descriptor
    if (Object->SecurityDescriptor != NULL) {
        ExFreePool(Object->SecurityDescriptor);
    }

    // Allocate and copy new security descriptor
    SIZE_T sd_size = RtlLengthSecurityDescriptor(SecurityDescriptor);
    Object->SecurityDescriptor = ExAllocatePool(NonPagedPool, sd_size);
    if (Object->SecurityDescriptor == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(Object->SecurityDescriptor, SecurityDescriptor, sd_size);
    return STATUS_SUCCESS;
}