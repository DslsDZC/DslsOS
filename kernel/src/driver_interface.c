/**
 * @file driver_interface.c
 * @brief Driver interface implementation
 * @author DslsOS Team
 * @version 1.0
 * @date 2024
 */

#include "../include/kernel.h"
#include "../include/dslos.h"

// Driver interface state
typedef struct _DRIVER_INTERFACE_STATE {
    BOOLEAN Initialized;
    KSPIN_LOCK DriverInterfaceLock;

    // Driver entry points
    LIST_ENTRY DriverEntryListHead;
    ULONG DriverEntryCount;

    // Driver registry
    LIST_ENTRY DriverRegistryListHead;
    ULONG DriverRegistryCount;

    // Service discovery
    LIST_ENTRY ServiceListHead;
    ULONG ServiceCount;

    // Driver compatibility
    LIST_ENTRY CompatibilityListHead;
    ULONG CompatibilityCount;

    // Driver validation
    LIST_ENTRY ValidationListHead;
    ULONG ValidationCount;

    // Interface statistics
    DRIVER_INTERFACE_STATISTICS Statistics;

    // Configuration
    ULONG MaxDrivers;
    ULONG MaxServices;
    ULONG MaxCompatibilityEntries;
} DRIVER_INTERFACE_STATE;

static DRIVER_INTERFACE_STATE g_DriverInterface = {0};

// Driver entry structure
typedef struct _DRIVER_ENTRY {
    UNICODE_STRING DriverName;
    UNICODE_STRING DriverPath;
    UNICODE_STRING DriverVersion;
    UNICODE_STRING DriverDescription;
    UNICODE_STRING DriverVendor;
    UNICODE_STRING DriverSignature;
    DRIVER_TYPE DriverType;
    ULONG DriverFlags;
    PDRIVER_INITIALIZE DriverInitialize;
    PDRIVER_UNLOAD DriverUnload;
    PVOID DriverHandle;
    ULONG ReferenceCount;
    LIST_ENTRY DriverEntryListEntry;
} DRIVER_ENTRY, *PDRIVER_ENTRY;

// Driver registry entry
typedef struct _DRIVER_REGISTRY_ENTRY {
    UNICODE_STRING HardwareId;
    UNICODE_STRING CompatibleId;
    UNICODE_STRING DriverName;
    UNICODE_STRING DriverClass;
    ULONG DriverVersion;
    ULONG CompatibilityFlags;
    BOOLEAN AutoLoad;
    BOOLEAN Critical;
    LIST_ENTRY RegistryListEntry;
} DRIVER_REGISTRY_ENTRY, *PDRIVER_REGISTRY_ENTRY;

// Service entry
typedef struct _SERVICE_ENTRY {
    UNICODE_STRING ServiceName;
    UNICODE_STRING ServiceDescription;
    SERVICE_TYPE ServiceType;
    SERVICE_STATE ServiceState;
    ULONG ServiceFlags;
    PDRIVER_OBJECT DriverObject;
    PVOID ServiceContext;
    LIST_ENTRY ServiceListEntry;
} SERVICE_ENTRY, *PSERVICE_ENTRY;

// Compatibility entry
typedef struct _COMPATIBILITY_ENTRY {
    UNICODE_STRING HardwareId;
    UNICODE_STRING DriverName;
    ULONG MinimumDriverVersion;
    ULONG MaximumDriverVersion;
    ULONG CompatibilityFlags;
    BOOLEAN Compatible;
    LIST_ENTRY CompatibilityListEntry;
} COMPATIBILITY_ENTRY, *PCOMPATIBILITY_ENTRY;

// Validation entry
typedef struct _VALIDATION_ENTRY {
    UNICODE_STRING DriverName;
    UNICODE_STRING DriverHash;
    UNICODE_STRING SignatureHash;
    VALIDATION_STATUS ValidationStatus;
    LARGE_INTEGER ValidationTime;
    LIST_ENTRY ValidationListEntry;
} VALIDATION_ENTRY, *PVALIDATION_ENTRY;

// Statistics structure
typedef struct _DRIVER_INTERFACE_STATISTICS {
    ULONG TotalDriversLoaded;
    ULONG TotalDriversUnloaded;
    ULONG TotalServicesRegistered;
    ULONG TotalCompatibilityChecks;
    ULONG TotalValidations;
    ULONG FailedLoads;
    ULONG FailedValidations;
    LARGE_INTEGER TotalLoadTime;
} DRIVER_INTERFACE_STATISTICS, *PDRIVER_INTERFACE_STATISTICS;

// Service types
typedef enum _SERVICE_TYPE {
    ServiceTypeDevice = 0,
    ServiceTypeFileSystem,
    ServiceTypeNetwork,
    ServiceTypeStorage,
    ServiceTypeDisplay,
    ServiceTypeInput,
    ServiceTypePrint,
    ServiceTypeMaximum
} SERVICE_TYPE;

// Service states
typedef enum _SERVICE_STATE {
    ServiceStateStopped = 0,
    ServiceStateStarting,
    ServiceStateRunning,
    ServiceStateStopping,
    ServiceStateFailed
} SERVICE_STATE;

// Validation status
typedef enum _VALIDATION_STATUS {
    ValidationStatusPending = 0,
    ValidationStatusValid,
    ValidationStatusInvalid,
    ValidationStatusExpired,
    ValidationStatusRevoked
} VALIDATION_STATUS;

// Driver types
typedef enum _DRIVER_TYPE {
    DriverTypeKernel = 0,
    DriverTypeUser,
    DriverTypeBoot,
    DriverTypePnp,
    DriverTypeMaximum
} DRIVER_TYPE;

// Driver flags
#define DRIVER_FLAG_SIGNED           0x00000001
#define DRIVER_FLAG_BOOT_DRIVER      0x00000002
#define DRIVER_FLAG_CRITICAL         0x00000004
#define DRIVER_FLAG_AUTO_LOAD        0x00000008
#define DRIVER_FLAG_USER_LOADABLE    0x00000010
#define DRIVER_FLAG_DEBUG            0x00000020
#define DRIVER_FLAG_TESTING          0x00000040
#define DRIVER_FLAG_DEPRECATED       0x00000080

// Service flags
#define SERVICE_FLAG_AUTO_START      0x00000001
#define SERVICE_FLAG_CRITICAL        0x00000002
#define SERVICE_FLAG_INTERACTIVE      0x00000004
#define SERVICE_FLAG_SHARED          0x00000008

// Compatibility flags
#define COMPATIBILITY_FLAG_EXACT     0x00000001
#define COMPATIBILITY_FLAG_RANGE     0x00000002
#define COMPATIBILITY_FLAG_WILDCARD  0x00000004
#define COMPATIBILITY_FLAG_OPTIONAL  0x00000008

/**
 * @brief Initialize driver interface
 * @return NTSTATUS Status code
 */
NTSTATUS DiInitializeDriverInterface(VOID)
{
    if (g_DriverInterface.Initialized) {
        return STATUS_SUCCESS;
    }

    KeInitializeSpinLock(&g_DriverInterface.DriverInterfaceLock);

    // Initialize driver entry list
    InitializeListHead(&g_DriverInterface.DriverEntryListHead);
    g_DriverInterface.DriverEntryCount = 0;

    // Initialize driver registry
    InitializeListHead(&g_DriverInterface.DriverRegistryListHead);
    g_DriverInterface.DriverRegistryCount = 0;

    // Initialize service discovery
    InitializeListHead(&g_DriverInterface.ServiceListHead);
    g_DriverInterface.ServiceCount = 0;

    // Initialize driver compatibility
    InitializeListHead(&g_DriverInterface.CompatibilityListHead);
    g_DriverInterface.CompatibilityCount = 0;

    // Initialize driver validation
    InitializeListHead(&g_DriverInterface.ValidationListHead);
    g_DriverInterface.ValidationCount = 0;

    // Initialize statistics
    RtlZeroMemory(&g_DriverInterface.Statistics, sizeof(DRIVER_INTERFACE_STATISTICS));

    // Set configuration
    g_DriverInterface.MaxDrivers = 100;
    g_DriverInterface.MaxServices = 1000;
    g_DriverInterface.MaxCompatibilityEntries = 10000;

    // Load built-in driver registry
    NTSTATUS status = DiLoadDriverRegistry();
    if (!NT_SUCCESS(status)) {
        return status;
    }

    // Initialize compatibility database
    status = DiInitializeCompatibilityDatabase();
    if (!NT_SUCCESS(status)) {
        return status;
    }

    g_DriverInterface.Initialized = TRUE;
    return STATUS_SUCCESS;
}

/**
 * @brief Load driver registry
 * @return NTSTATUS Status code
 */
static NTSTATUS DiLoadDriverRegistry(VOID)
{
    // This is a simplified implementation
    // In a real implementation, this would:
    // - Load driver registry from persistent storage
    // - Parse hardware ID to driver mappings
    // - Validate registry entries
    // - Add entries to internal registry

    return STATUS_SUCCESS;
}

/**
 * @brief Initialize compatibility database
 * @return NTSTATUS Status code
 */
static NTSTATUS DiInitializeCompatibilityDatabase(VOID)
{
    // This is a simplified implementation
    // In a real implementation, this would:
    // - Load compatibility database from storage
    // - Parse driver compatibility rules
    // - Validate compatibility entries
    // - Set up compatibility lookup tables

    return STATUS_SUCCESS;
}

/**
 * @brief Register driver entry
 * @param DriverName Name of the driver
 * @param DriverPath Path to driver file
 * @param DriverVersion Version of the driver
 * @param DriverDescription Description of the driver
 * @param DriverVendor Vendor of the driver
 * @param DriverType Type of driver
 * @param DriverInitialize Driver initialization function
 * @param DriverUnload Driver unload function
 * @return NTSTATUS Status code
 */
NTSTATUS DiRegisterDriverEntry(PCWSTR DriverName, PCWSTR DriverPath, PCWSTR DriverVersion,
                               PCWSTR DriverDescription, PCWSTR DriverVendor,
                               DRIVER_TYPE DriverType, PDRIVER_INITIALIZE DriverInitialize,
                               PDRIVER_UNLOAD DriverUnload)
{
    if (DriverName == NULL || DriverPath == NULL || DriverInitialize == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    // Allocate driver entry
    PDRIVER_ENTRY driver_entry = ExAllocatePool(NonPagedPool, sizeof(DRIVER_ENTRY));
    if (driver_entry == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(driver_entry, sizeof(DRIVER_ENTRY));

    // Set driver name
    SIZE_T name_length = wcslen(DriverName);
    driver_entry->DriverName.Buffer = ExAllocatePool(NonPagedPool, (name_length + 1) * sizeof(WCHAR));
    if (driver_entry->DriverName.Buffer == NULL) {
        ExFreePool(driver_entry);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    wcscpy_s(driver_entry->DriverName.Buffer, name_length + 1, DriverName);
    driver_entry->DriverName.Length = (USHORT)(name_length * sizeof(WCHAR));
    driver_entry->DriverName.MaximumLength = (USHORT)((name_length + 1) * sizeof(WCHAR));

    // Set driver path
    SIZE_T path_length = wcslen(DriverPath);
    driver_entry->DriverPath.Buffer = ExAllocatePool(NonPagedPool, (path_length + 1) * sizeof(WCHAR));
    if (driver_entry->DriverPath.Buffer == NULL) {
        ExFreePool(driver_entry->DriverName.Buffer);
        ExFreePool(driver_entry);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    wcscpy_s(driver_entry->DriverPath.Buffer, path_length + 1, DriverPath);
    driver_entry->DriverPath.Length = (USHORT)(path_length * sizeof(WCHAR));
    driver_entry->DriverPath.MaximumLength = (USHORT)((path_length + 1) * sizeof(WCHAR));

    // Set other driver information
    if (DriverVersion != NULL) {
        SIZE_T version_length = wcslen(DriverVersion);
        driver_entry->DriverVersion.Buffer = ExAllocatePool(NonPagedPool, (version_length + 1) * sizeof(WCHAR));
        if (driver_entry->DriverVersion.Buffer != NULL) {
            wcscpy_s(driver_entry->DriverVersion.Buffer, version_length + 1, DriverVersion);
            driver_entry->DriverVersion.Length = (USHORT)(version_length * sizeof(WCHAR));
            driver_entry->DriverVersion.MaximumLength = (USHORT)((version_length + 1) * sizeof(WCHAR));
        }
    }

    if (DriverDescription != NULL) {
        SIZE_T desc_length = wcslen(DriverDescription);
        driver_entry->DriverDescription.Buffer = ExAllocatePool(NonPagedPool, (desc_length + 1) * sizeof(WCHAR));
        if (driver_entry->DriverDescription.Buffer != NULL) {
            wcscpy_s(driver_entry->DriverDescription.Buffer, desc_length + 1, DriverDescription);
            driver_entry->DriverDescription.Length = (USHORT)(desc_length * sizeof(WCHAR));
            driver_entry->DriverDescription.MaximumLength = (USHORT)((desc_length + 1) * sizeof(WCHAR));
        }
    }

    if (DriverVendor != NULL) {
        SIZE_T vendor_length = wcslen(DriverVendor);
        driver_entry->DriverVendor.Buffer = ExAllocatePool(NonPagedPool, (vendor_length + 1) * sizeof(WCHAR));
        if (driver_entry->DriverVendor.Buffer != NULL) {
            wcscpy_s(driver_entry->DriverVendor.Buffer, vendor_length + 1, DriverVendor);
            driver_entry->DriverVendor.Length = (USHORT)(vendor_length * sizeof(WCHAR));
            driver_entry->DriverVendor.MaximumLength = (USHORT)((vendor_length + 1) * sizeof(WCHAR));
        }
    }

    // Set driver entry fields
    driver_entry->DriverType = DriverType;
    driver_entry->DriverFlags = 0;
    driver_entry->DriverInitialize = DriverInitialize;
    driver_entry->DriverUnload = DriverUnload;
    driver_entry->DriverHandle = NULL;
    driver_entry->ReferenceCount = 1;

    // Add to driver entry list
    KIRQL old_irql;
    KeAcquireSpinLock(&g_DriverInterface.DriverInterfaceLock, &old_irql);

    InsertTailList(&g_DriverInterface.DriverEntryListHead, &driver_entry->DriverEntryListEntry);
    g_DriverInterface.DriverEntryCount++;

    KeReleaseSpinLock(&g_DriverInterface.DriverInterfaceLock, old_irql);

    return STATUS_SUCCESS;
}

/**
 * @brief Unregister driver entry
 * @param DriverName Name of the driver to unregister
 * @return NTSTATUS Status code
 */
NTSTATUS DiUnregisterDriverEntry(PCWSTR DriverName)
{
    if (DriverName == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    KIRQL old_irql;
    KeAcquireSpinLock(&g_DriverInterface.DriverInterfaceLock, &old_irql);

    PLIST_ENTRY entry = g_DriverInterface.DriverEntryListHead.Flink;
    while (entry != &g_DriverInterface.DriverEntryListHead) {
        PDRIVER_ENTRY driver_entry = CONTAINING_RECORD(entry, DRIVER_ENTRY, DriverEntryListEntry);

        if (wcscmp(driver_entry->DriverName.Buffer, DriverName) == 0) {
            RemoveEntryList(&driver_entry->DriverEntryListEntry);
            g_DriverInterface.DriverEntryCount--;

            KeReleaseSpinLock(&g_DriverInterface.DriverInterfaceLock, old_irql);

            // Free driver entry resources
            if (driver_entry->DriverName.Buffer != NULL) {
                ExFreePool(driver_entry->DriverName.Buffer);
            }
            if (driver_entry->DriverPath.Buffer != NULL) {
                ExFreePool(driver_entry->DriverPath.Buffer);
            }
            if (driver_entry->DriverVersion.Buffer != NULL) {
                ExFreePool(driver_entry->DriverVersion.Buffer);
            }
            if (driver_entry->DriverDescription.Buffer != NULL) {
                ExFreePool(driver_entry->DriverDescription.Buffer);
            }
            if (driver_entry->DriverVendor.Buffer != NULL) {
                ExFreePool(driver_entry->DriverVendor.Buffer);
            }
            if (driver_entry->DriverSignature.Buffer != NULL) {
                ExFreePool(driver_entry->DriverSignature.Buffer);
            }

            ExFreePool(driver_entry);
            return STATUS_SUCCESS;
        }

        entry = entry->Flink;
    }

    KeReleaseSpinLock(&g_DriverInterface.DriverInterfaceLock, old_irql);
    return STATUS_NOT_FOUND;
}

/**
 * @brief Load driver by name
 * @param DriverName Name of the driver to load
 * @param DriverObject Pointer to receive driver object
 * @return NTSTATUS Status code
 */
NTSTATUS DiLoadDriverByName(PCWSTR DriverName, PDRIVER_OBJECT* DriverObject)
{
    if (DriverName == NULL || DriverObject == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    // Find driver entry
    PDRIVER_ENTRY driver_entry = DiFindDriverEntry(DriverName);
    if (driver_entry == NULL) {
        return STATUS_NOT_FOUND;
    }

    // Validate driver signature
    NTSTATUS status = DiValidateDriverSignature(driver_entry);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    // Check compatibility
    status = DiCheckDriverCompatibility(driver_entry);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    // Load driver file
    PVOID driver_image = NULL;
    SIZE_T driver_size = 0;
    status = DiLoadDriverImage(driver_entry->DriverPath.Buffer, &driver_image, &driver_size);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    // Create driver object
    PDRIVER_OBJECT driver_obj = ExAllocatePool(NonPagedPool, sizeof(DRIVER_OBJECT));
    if (driver_obj == NULL) {
        ExFreePool(driver_image);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(driver_obj, sizeof(DRIVER_OBJECT));

    // Initialize driver object
    driver_obj->Header.ObjectType = KERNEL_OBJECT_TYPE_DRIVER;
    driver_obj->Header.ReferenceCount = 1;
    driver_obj->Header.Flags = 0;
    InitializeListHead(&driver_obj->Header.ObjectListEntry);

    // Set driver identification
    driver_obj->DriverId = g_DriverInterface.DriverEntryCount;
    driver_obj->DriverName = driver_entry->DriverName;
    driver_obj->DriverVersion = driver_entry->DriverVersion;
    driver_obj->DriverDescription = driver_entry->DriverDescription;
    driver_obj->DriverVendor = driver_entry->DriverVendor;

    // Set driver entry points
    driver_obj->DriverInitialize = driver_entry->DriverInitialize;
    driver_obj->DriverUnload = driver_entry->DriverUnload;

    // Set driver state
    driver_obj->DriverState = DriverStateLoaded;
    driver_obj->Flags = driver_entry->DriverFlags;
    driver_obj->ReferenceCount = 1;

    // Initialize driver dispatch table
    for (ULONG i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
        driver_obj->MajorFunction[i] = DiDefaultDispatch;
    }

    // Initialize driver lists
    InitializeListHead(&driver_obj->DeviceListHead);
    driver_obj->DeviceCount = 0;

    InitializeListHead(&driver_obj->ResourceListHead);
    driver_obj->ResourceCount = 0;

    // Set driver-specific data
    driver_obj->DriverExtension = NULL;
    driver_obj->DriverStart = driver_image;
    driver_obj->DriverSize = driver_size;

    // Initialize statistics
    RtlZeroMemory(&driver_obj->DriverStats, sizeof(DRIVER_SPECIFIC_STATISTICS));

    // Add to driver list
    KIRQL old_irql;
    KeAcquireSpinLock(&g_DriverInterface.DriverInterfaceLock, &old_irql);

    InsertTailList(&g_DriverInterface.DriverEntryListHead, &driver_obj->DriverListEntry);

    KeReleaseSpinLock(&g_DriverInterface.DriverInterfaceLock, old_irql);

    // Call driver initialization
    LARGE_INTEGER start_time;
    KeQuerySystemTime(&start_time);

    status = driver_obj->DriverInitialize(driver_obj, NULL);

    LARGE_INTEGER end_time;
    KeQuerySystemTime(&end_time);

    if (NT_SUCCESS(status)) {
        driver_obj->DriverState = DriverStateInitialized;
        InterlockedIncrement(&g_DriverInterface.Statistics.TotalDriversLoaded);
        g_DriverInterface.Statistics.TotalLoadTime.QuadPart += end_time.QuadPart - start_time.QuadPart;
        *DriverObject = driver_obj;
        return STATUS_SUCCESS;
    } else {
        // Initialization failed, clean up
        KeAcquireSpinLock(&g_DriverInterface.DriverInterfaceLock, &old_irql);
        RemoveEntryList(&driver_obj->DriverListEntry);
        KeReleaseSpinLock(&g_DriverInterface.DriverInterfaceLock, old_irql);

        ExFreePool(driver_obj);
        ExFreePool(driver_image);

        InterlockedIncrement(&g_DriverInterface.Statistics.FailedLoads);
        return status;
    }
}

/**
 * @brief Find driver entry by name
 * @param DriverName Name of the driver to find
 * @return Driver entry or NULL
 */
static PDRIVER_ENTRY DiFindDriverEntry(PCWSTR DriverName)
{
    KIRQL old_irql;
    KeAcquireSpinLock(&g_DriverInterface.DriverInterfaceLock, &old_irql);

    PLIST_ENTRY entry = g_DriverInterface.DriverEntryListHead.Flink;
    while (entry != &g_DriverInterface.DriverEntryListHead) {
        PDRIVER_ENTRY driver_entry = CONTAINING_RECORD(entry, DRIVER_ENTRY, DriverEntryListEntry);

        if (wcscmp(driver_entry->DriverName.Buffer, DriverName) == 0) {
            KeReleaseSpinLock(&g_DriverInterface.DriverInterfaceLock, old_irql);
            return driver_entry;
        }

        entry = entry->Flink;
    }

    KeReleaseSpinLock(&g_DriverInterface.DriverInterfaceLock, old_irql);
    return NULL;
}

/**
 * @brief Validate driver signature
 * @param DriverEntry Driver entry to validate
 * @return NTSTATUS Status code
 */
static NTSTATUS DiValidateDriverSignature(PDRIVER_ENTRY DriverEntry)
{
    if (DriverEntry == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    // This is a simplified implementation
    // In a real implementation, this would:
    // - Check digital signature
    // - Verify certificate chain
    // - Check revocation status
    // - Validate against trusted root

    // For now, assume all drivers are valid
    DriverEntry->DriverFlags |= DRIVER_FLAG_SIGNED;

    InterlockedIncrement(&g_DriverInterface.Statistics.TotalValidations);
    return STATUS_SUCCESS;
}

/**
 * @brief Check driver compatibility
 * @param DriverEntry Driver entry to check
 * @return NTSTATUS Status code
 */
static NTSTATUS DiCheckDriverCompatibility(PDRIVER_ENTRY DriverEntry)
{
    if (DriverEntry == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    // This is a simplified implementation
    // In a real implementation, this would:
    // - Check hardware compatibility
    // - Verify driver version compatibility
    // - Check system requirements
    // - Validate against known issues

    InterlockedIncrement(&g_DriverInterface.Statistics.TotalCompatibilityChecks);
    return STATUS_SUCCESS;
}

/**
 * @brief Load driver image from file
 * @param DriverPath Path to driver file
 * @param DriverImage Pointer to receive driver image
 * @param DriverSize Pointer to receive driver size
 * @return NTSTATUS Status code
 */
static NTSTATUS DiLoadDriverImage(PCWSTR DriverPath, PVOID* DriverImage, SIZE_T* DriverSize)
{
    if (DriverPath == NULL || DriverImage == NULL || DriverSize == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    // This is a simplified implementation
    // In a real implementation, this would:
    // - Open driver file
    // - Read file contents
    // - Verify file format
    // - Relocate driver code
    // - Set up memory protection

    // For now, return not implemented
    return STATUS_NOT_IMPLEMENTED;
}

/**
 * @brief Default dispatch function for unhandled IRPs
 * @param DeviceObject Device object
 * @param IoRequest I/O request
 * @return NTSTATUS Status code
 */
NTSTATUS DiDefaultDispatch(PDEVICE_OBJECT DeviceObject, PIO_REQUEST IoRequest)
{
    if (DeviceObject == NULL || IoRequest == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    // This is the default dispatch function for IRPs that don't have specific handlers
    IoRequest->Status = STATUS_INVALID_DEVICE_REQUEST;
    IoCompleteRequest(IoRequest, IoRequest->Status, 0);

    return STATUS_SUCCESS;
}

/**
 * @brief Register service
 * @param ServiceName Name of the service
 * @param ServiceDescription Description of the service
 * @param ServiceType Type of service
 * @param DriverObject Driver object providing the service
 * @param ServiceContext Service context
 * @return NTSTATUS Status code
 */
NTSTATUS DiRegisterService(PCWSTR ServiceName, PCWSTR ServiceDescription, SERVICE_TYPE ServiceType,
                         PDRIVER_OBJECT DriverObject, PVOID ServiceContext)
{
    if (ServiceName == NULL || DriverObject == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    // Allocate service entry
    PSERVICE_ENTRY service_entry = ExAllocatePool(NonPagedPool, sizeof(SERVICE_ENTRY));
    if (service_entry == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(service_entry, sizeof(SERVICE_ENTRY));

    // Set service name
    SIZE_T name_length = wcslen(ServiceName);
    service_entry->ServiceName.Buffer = ExAllocatePool(NonPagedPool, (name_length + 1) * sizeof(WCHAR));
    if (service_entry->ServiceName.Buffer == NULL) {
        ExFreePool(service_entry);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    wcscpy_s(service_entry->ServiceName.Buffer, name_length + 1, ServiceName);
    service_entry->ServiceName.Length = (USHORT)(name_length * sizeof(WCHAR));
    service_entry->ServiceName.MaximumLength = (USHORT)((name_length + 1) * sizeof(WCHAR));

    // Set service description
    if (ServiceDescription != NULL) {
        SIZE_T desc_length = wcslen(ServiceDescription);
        service_entry->ServiceDescription.Buffer = ExAllocatePool(NonPagedPool, (desc_length + 1) * sizeof(WCHAR));
        if (service_entry->ServiceDescription.Buffer != NULL) {
            wcscpy_s(service_entry->ServiceDescription.Buffer, desc_length + 1, ServiceDescription);
            service_entry->ServiceDescription.Length = (USHORT)(desc_length * sizeof(WCHAR));
            service_entry->ServiceDescription.MaximumLength = (USHORT)((desc_length + 1) * sizeof(WCHAR));
        }
    }

    // Set service entry fields
    service_entry->ServiceType = ServiceType;
    service_entry->ServiceState = ServiceStateStopped;
    service_entry->ServiceFlags = 0;
    service_entry->DriverObject = DriverObject;
    service_entry->ServiceContext = ServiceContext;

    // Add to service list
    KIRQL old_irql;
    KeAcquireSpinLock(&g_DriverInterface.DriverInterfaceLock, &old_irql);

    InsertTailList(&g_DriverInterface.ServiceListHead, &service_entry->ServiceListEntry);
    g_DriverInterface.ServiceCount++;

    KeReleaseSpinLock(&g_DriverInterface.DriverInterfaceLock, old_irql);

    InterlockedIncrement(&g_DriverInterface.Statistics.TotalServicesRegistered);
    return STATUS_SUCCESS;
}

/**
 * @brief Unregister service
 * @param ServiceName Name of the service to unregister
 * @return NTSTATUS Status code
 */
NTSTATUS DiUnregisterService(PCWSTR ServiceName)
{
    if (ServiceName == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    KIRQL old_irql;
    KeAcquireSpinLock(&g_DriverInterface.DriverInterfaceLock, &old_irql);

    PLIST_ENTRY entry = g_DriverInterface.ServiceListHead.Flink;
    while (entry != &g_DriverInterface.ServiceListHead) {
        PSERVICE_ENTRY service_entry = CONTAINING_RECORD(entry, SERVICE_ENTRY, ServiceListEntry);

        if (wcscmp(service_entry->ServiceName.Buffer, ServiceName) == 0) {
            RemoveEntryList(&service_entry->ServiceListEntry);
            g_DriverInterface.ServiceCount--;

            KeReleaseSpinLock(&g_DriverInterface.DriverInterfaceLock, old_irql);

            // Free service entry resources
            if (service_entry->ServiceName.Buffer != NULL) {
                ExFreePool(service_entry->ServiceName.Buffer);
            }
            if (service_entry->ServiceDescription.Buffer != NULL) {
                ExFreePool(service_entry->ServiceDescription.Buffer);
            }

            ExFreePool(service_entry);
            return STATUS_SUCCESS;
        }

        entry = entry->Flink;
    }

    KeReleaseSpinLock(&g_DriverInterface.DriverInterfaceLock, old_irql);
    return STATUS_NOT_FOUND;
}

/**
 * @brief Find service by name
 * @param ServiceName Name of the service to find
 * @return Service entry or NULL
 */
PSERVICE_ENTRY DiFindService(PCWSTR ServiceName)
{
    if (ServiceName == NULL) {
        return NULL;
    }

    KIRQL old_irql;
    KeAcquireSpinLock(&g_DriverInterface.DriverInterfaceLock, &old_irql);

    PLIST_ENTRY entry = g_DriverInterface.ServiceListHead.Flink;
    while (entry != &g_DriverInterface.ServiceListHead) {
        PSERVICE_ENTRY service_entry = CONTAINING_RECORD(entry, SERVICE_ENTRY, ServiceListEntry);

        if (wcscmp(service_entry->ServiceName.Buffer, ServiceName) == 0) {
            KeReleaseSpinLock(&g_DriverInterface.DriverInterfaceLock, old_irql);
            return service_entry;
        }

        entry = entry->Flink;
    }

    KeReleaseSpinLock(&g_DriverInterface.DriverInterfaceLock, old_irql);
    return NULL;
}

/**
 * @brief Get services by type
 * @param ServiceType Type of services to find
 * @param ServiceArray Array to receive service entries
 * @param ArraySize Size of service array
 * @return Number of services found
 */
ULONG DiGetServicesByType(SERVICE_TYPE ServiceType, PSERVICE_ENTRY* ServiceArray, ULONG ArraySize)
{
    if (ServiceArray == NULL || ArraySize == 0) {
        return 0;
    }

    ULONG count = 0;
    KIRQL old_irql;
    KeAcquireSpinLock(&g_DriverInterface.DriverInterfaceLock, &old_irql);

    PLIST_ENTRY entry = g_DriverInterface.ServiceListHead.Flink;
    while (entry != &g_DriverInterface.ServiceListHead && count < ArraySize) {
        PSERVICE_ENTRY service_entry = CONTAINING_RECORD(entry, SERVICE_ENTRY, ServiceListEntry);

        if (service_entry->ServiceType == ServiceType) {
            ServiceArray[count++] = service_entry;
        }

        entry = entry->Flink;
    }

    KeReleaseSpinLock(&g_DriverInterface.DriverInterfaceLock, old_irql);
    return count;
}

/**
 * @brief Get driver interface statistics
 * @param Statistics Statistics structure to fill
 */
VOID DiGetDriverInterfaceStatistics(PDRIVER_INTERFACE_STATISTICS Statistics)
{
    if (Statistics == NULL) {
        return;
    }

    KIRQL old_irql;
    KeAcquireSpinLock(&g_DriverInterface.DriverInterfaceLock, &old_irql);
    RtlCopyMemory(Statistics, &g_DriverInterface.Statistics, sizeof(DRIVER_INTERFACE_STATISTICS));
    KeReleaseSpinLock(&g_DriverInterface.DriverInterfaceLock, old_irql);
}

/**
 * @brief Add compatibility entry
 * @param HardwareId Hardware ID
 * @param DriverName Driver name
 * @param MinimumDriverVersion Minimum driver version
 * @param MaximumDriverVersion Maximum driver version
 * @param CompatibilityFlags Compatibility flags
 * @return NTSTATUS Status code
 */
NTSTATUS DiAddCompatibilityEntry(PCWSTR HardwareId, PCWSTR DriverName, ULONG MinimumDriverVersion,
                                 ULONG MaximumDriverVersion, ULONG CompatibilityFlags)
{
    if (HardwareId == NULL || DriverName == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    // Allocate compatibility entry
    PCOMPATIBILITY_ENTRY compat_entry = ExAllocatePool(NonPagedPool, sizeof(COMPATIBILITY_ENTRY));
    if (compat_entry == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(compat_entry, sizeof(COMPATIBILITY_ENTRY));

    // Set hardware ID
    SIZE_T hwid_length = wcslen(HardwareId);
    compat_entry->HardwareId.Buffer = ExAllocatePool(NonPagedPool, (hwid_length + 1) * sizeof(WCHAR));
    if (compat_entry->HardwareId.Buffer == NULL) {
        ExFreePool(compat_entry);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    wcscpy_s(compat_entry->HardwareId.Buffer, hwid_length + 1, HardwareId);
    compat_entry->HardwareId.Length = (USHORT)(hwid_length * sizeof(WCHAR));
    compat_entry->HardwareId.MaximumLength = (USHORT)((hwid_length + 1) * sizeof(WCHAR));

    // Set driver name
    SIZE_T driver_length = wcslen(DriverName);
    compat_entry->DriverName.Buffer = ExAllocatePool(NonPagedPool, (driver_length + 1) * sizeof(WCHAR));
    if (compat_entry->DriverName.Buffer == NULL) {
        ExFreePool(compat_entry->HardwareId.Buffer);
        ExFreePool(compat_entry);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    wcscpy_s(compat_entry->DriverName.Buffer, driver_length + 1, DriverName);
    compat_entry->DriverName.Length = (USHORT)(driver_length * sizeof(WCHAR));
    compat_entry->DriverName.MaximumLength = (USHORT)((driver_length + 1) * sizeof(WCHAR));

    // Set compatibility entry fields
    compat_entry->MinimumDriverVersion = MinimumDriverVersion;
    compat_entry->MaximumDriverVersion = MaximumDriverVersion;
    compat_entry->CompatibilityFlags = CompatibilityFlags;
    compat_entry->Compatible = TRUE;

    // Add to compatibility list
    KIRQL old_irql;
    KeAcquireSpinLock(&g_DriverInterface.DriverInterfaceLock, &old_irql);

    InsertTailList(&g_DriverInterface.CompatibilityListHead, &compat_entry->CompatibilityListEntry);
    g_DriverInterface.CompatibilityCount++;

    KeReleaseSpinLock(&g_DriverInterface.DriverInterfaceLock, old_irql);

    return STATUS_SUCCESS;
}

/**
 * @brief Check hardware compatibility
 * @param HardwareId Hardware ID to check
 * @param DriverVersion Driver version to check
 * @param Compatible Pointer to receive compatibility status
 * @return NTSTATUS Status code
 */
NTSTATUS DiCheckHardwareCompatibility(PCWSTR HardwareId, ULONG DriverVersion, PBOOLEAN Compatible)
{
    if (HardwareId == NULL || Compatible == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    *Compatible = FALSE;

    KIRQL old_irql;
    KeAcquireSpinLock(&g_DriverInterface.DriverInterfaceLock, &old_irql);

    PLIST_ENTRY entry = g_DriverInterface.CompatibilityListHead.Flink;
    while (entry != &g_DriverInterface.CompatibilityListHead) {
        PCOMPATIBILITY_ENTRY compat_entry = CONTAINING_RECORD(entry, COMPATIBILITY_ENTRY, CompatibilityListEntry);

        if (wcscmp(compat_entry->HardwareId.Buffer, HardwareId) == 0) {
            if (DriverVersion >= compat_entry->MinimumDriverVersion &&
                DriverVersion <= compat_entry->MaximumDriverVersion) {
                *Compatible = compat_entry->Compatible;
                KeReleaseSpinLock(&g_DriverInterface.DriverInterfaceLock, old_irql);
                return STATUS_SUCCESS;
            }
        }

        entry = entry->Flink;
    }

    KeReleaseSpinLock(&g_DriverInterface.DriverInterfaceLock, old_irql);
    return STATUS_NOT_FOUND;
}

/**
 * @brief Unload all drivers
 */
VOID DiUnloadAllDrivers(VOID)
{
    KIRQL old_irql;
    KeAcquireSpinLock(&g_DriverInterface.DriverInterfaceLock, &old_irql);

    while (!IsListEmpty(&g_DriverInterface.DriverEntryListHead)) {
        PLIST_ENTRY entry = RemoveHeadList(&g_DriverInterface.DriverEntryListHead);
        g_DriverInterface.DriverEntryCount--;

        KeReleaseSpinLock(&g_DriverInterface.DriverInterfaceLock, old_irql);

        PDRIVER_ENTRY driver_entry = CONTAINING_RECORD(entry, DRIVER_ENTRY, DriverEntryListEntry);

        // Call driver unload if available
        if (driver_entry->DriverUnload != NULL) {
            driver_entry->DriverUnload(NULL);
        }

        // Free driver entry resources
        if (driver_entry->DriverName.Buffer != NULL) {
            ExFreePool(driver_entry->DriverName.Buffer);
        }
        if (driver_entry->DriverPath.Buffer != NULL) {
            ExFreePool(driver_entry->DriverPath.Buffer);
        }
        if (driver_entry->DriverVersion.Buffer != NULL) {
            ExFreePool(driver_entry->DriverVersion.Buffer);
        }
        if (driver_entry->DriverDescription.Buffer != NULL) {
            ExFreePool(driver_entry->DriverDescription.Buffer);
        }
        if (driver_entry->DriverVendor.Buffer != NULL) {
            ExFreePool(driver_entry->DriverVendor.Buffer);
        }
        if (driver_entry->DriverSignature.Buffer != NULL) {
            ExFreePool(driver_entry->DriverSignature.Buffer);
        }

        ExFreePool(driver_entry);

        KeAcquireSpinLock(&g_DriverInterface.DriverInterfaceLock, &old_irql);
    }

    KeReleaseSpinLock(&g_DriverInterface.DriverInterfaceLock, old_irql);
    InterlockedIncrement(&g_DriverInterface.Statistics.TotalDriversUnloaded);
}