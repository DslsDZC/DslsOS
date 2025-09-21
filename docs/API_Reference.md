# DslsOS API Reference

## Table of Contents

1. [Kernel APIs](#kernel-apis)
2. [Memory Management APIs](#memory-management-apis)
3. [Process Management APIs](#process-management-apis)
4. [Thread Management APIs](#thread-management-apis)
5. [IPC Communication APIs](#ipc-communication-apis)
6. [File System APIs](#file-system-apis)
7. [Security APIs](#security-apis)
8. [User Interface APIs](#user-interface-apis)
9. [Distributed Management APIs](#distributed-management-apis)
10. [System Test APIs](#system-test-apis)

---

## Kernel APIs

### System Initialization

#### `KiKernelMain`
```c
NTSTATUS NTAPI KiKernelMain(PBOOT_INFORMATION BootInfo);
```

**Description**: Main kernel entry point that initializes the entire system.

**Parameters**:
- `BootInfo`: Pointer to boot information structure

**Returns**: NTSTATUS code indicating success or failure

**Status Codes**:
- `STATUS_SUCCESS`: System initialized successfully
- `STATUS_INSUFFICIENT_RESOURCES`: Insufficient system resources
- `STATUS_UNSUCCESSFUL`: General initialization failure

---

### Memory Management APIs

### Pool Allocation

#### `ExAllocatePool`
```c
PVOID NTAPI ExAllocatePool(POOL_TYPE PoolType, SIZE_T NumberOfBytes);
```

**Description**: Allocates memory from the specified pool.

**Parameters**:
- `PoolType`: Type of pool (NonPagedPool, PagedPool)
- `NumberOfBytes`: Number of bytes to allocate

**Returns**: Pointer to allocated memory or NULL on failure

#### `ExAllocatePoolWithTag`
```c
PVOID NTAPI ExAllocatePoolWithTag(POOL_TYPE PoolType, SIZE_T NumberOfBytes, ULONG Tag);
```

**Description**: Allocates memory from the specified pool with a tag.

**Parameters**:
- `PoolType`: Type of pool (NonPagedPool, PagedPool)
- `NumberOfBytes`: Number of bytes to allocate
- `Tag`: 4-character tag for memory tracking

**Returns**: Pointer to allocated memory or NULL on failure

#### `ExFreePool`
```c
VOID NTAPI ExFreePool(PVOID BaseAddress);
```

**Description**: Frees memory allocated from pool.

**Parameters**:
- `BaseAddress`: Pointer to memory to free

#### `ExFreePoolWithTag`
```c
VOID NTAPI ExFreePoolWithTag(PVOID BaseAddress, ULONG Tag);
```

**Description**: Frees memory allocated from pool with tag verification.

**Parameters**:
- `BaseAddress`: Pointer to memory to free
- `Tag`: Tag used for allocation verification

### Memory Mapping

#### `MmMapMemory`
```c
NTSTATUS NTAPI MmMapMemory(
    PVOID VirtualAddress,
    PVOID PhysicalAddress,
    SIZE_T Size,
    ULONG Protection
);
```

**Description**: Maps physical memory to virtual address space.

**Parameters**:
- `VirtualAddress`: Target virtual address
- `PhysicalAddress`: Source physical address
- `Size`: Size of memory region to map
- `Protection`: Memory protection flags

**Returns**: NTSTATUS code

#### `MmUnmapMemory`
```c
NTSTATUS NTAPI MmUnmapMemory(PVOID VirtualAddress, SIZE_T Size);
```

**Description**: Unmaps memory region.

**Parameters**:
- `VirtualAddress`: Virtual address to unmap
- `Size`: Size of memory region

**Returns**: NTSTATUS code

### Page Management

#### `MmAllocatePages`
```c
NTSTATUS NTAPI MmAllocatePages(
    SIZE_T NumberOfPages,
    PVOID *BaseAddress
);
```

**Description**: Allocates physical pages.

**Parameters**:
- `NumberOfPages`: Number of pages to allocate
- `BaseAddress`: Receives base address of allocated pages

**Returns**: NTSTATUS code

#### `MmFreePages`
```c
NTSTATUS NTAPI MmFreePages(PVOID BaseAddress, SIZE_T NumberOfPages);
```

**Description**: Frees physical pages.

**Parameters**:
- `BaseAddress`: Base address of pages to free
- `NumberOfPages`: Number of pages to free

**Returns**: NTSTATUS code

---

### Process Management APIs

### Process Creation

#### `PsCreateProcess`
```c
NTSTATUS NTAPI PsCreateProcess(
    PCWSTR ProcessName,
    PCWSTR CommandLine,
    PHANDLE ProcessHandle
);
```

**Description**: Creates a new process.

**Parameters**:
- `ProcessName`: Name of the process
- `CommandLine`: Command line for the process
- `ProcessHandle`: Receives handle to the process

**Returns**: NTSTATUS code

#### `PsTerminateProcess`
```c
NTSTATUS NTAPI PsTerminateProcess(
    HANDLE ProcessHandle,
    NTSTATUS ExitStatus
);
```

**Description**: Terminates a process.

**Parameters**:
- `ProcessHandle`: Handle to the process
- `ExitStatus`: Exit status code

**Returns**: NTSTATUS code

### Process Information

#### `PsGetProcessById`
```c
PPROCESS_CONTROL_BLOCK NTAPI PsGetProcessById(PROCESS_ID ProcessId);
```

**Description**: Gets process control block by ID.

**Parameters**:
- `ProcessId`: Process ID

**Returns**: Pointer to process control block or NULL

#### `PsGetCurrentProcess`
```c
PPROCESS_CONTROL_BLOCK NTAPI PsGetCurrentProcess(VOID);
```

**Description**: Gets current process control block.

**Returns**: Pointer to current process control block

#### `PsGetProcessName`
```c
NTSTATUS NTAPI PsGetProcessName(
    HANDLE ProcessHandle,
    PUNICODE_STRING ProcessName
);
```

**Description**: Gets process name.

**Parameters**:
- `ProcessHandle`: Handle to the process
- `ProcessName`: Receives process name

**Returns**: NTSTATUS code

### Process Operations

#### `PsSuspendProcess`
```c
NTSTATUS NTAPI PsSuspendProcess(HANDLE ProcessHandle);
```

**Description**: Suspends process execution.

**Parameters**:
- `ProcessHandle`: Handle to the process

**Returns**: NTSTATUS code

#### `PsResumeProcess`
```c
NTSTATUS NTAPI PsResumeProcess(HANDLE ProcessHandle);
```

**Description**: Resumes process execution.

**Parameters**:
- `ProcessHandle`: Handle to the process

**Returns**: NTSTATUS code

#### `PsEnumProcesses`
```c
NTSTATUS NTAPI PsEnumProcesses(
    PSYSTEM_PROCESS_INFORMATION Buffer,
    ULONG BufferSize,
    PULONG ReturnLength
);
```

**Description**: Enumerates running processes.

**Parameters**:
- `Buffer`: Buffer to receive process information
- `BufferSize`: Size of buffer in bytes
- `ReturnLength`: Receives bytes written

**Returns**: NTSTATUS code

---

### Thread Management APIs

### Thread Creation

#### `PsCreateThread`
```c
NTSTATUS NTAPI PsCreateThread(
    PVOID StartAddress,
    PVOID Parameter,
    PHANDLE ThreadHandle
);
```

**Description**: Creates a new thread.

**Parameters**:
- `StartAddress`: Thread start address
- `Parameter`: Thread parameter
- `ThreadHandle`: Receives handle to the thread

**Returns**: NTSTATUS code

#### `PsTerminateThread`
```c
NTSTATUS NTAPI PsTerminateThread(
    HANDLE ThreadHandle,
    NTSTATUS ExitStatus
);
```

**Description**: Terminates a thread.

**Parameters**:
- `ThreadHandle`: Handle to the thread
- `ExitStatus`: Exit status code

**Returns**: NTSTATUS code

### Thread Operations

#### `PsSuspendThread`
```c
NTSTATUS NTAPI PsSuspendThread(HANDLE ThreadHandle);
```

**Description**: Suspends thread execution.

**Parameters**:
- `ThreadHandle`: Handle to the thread

**Returns**: NTSTATUS code

#### `PsResumeThread`
```c
NTSTATUS NTAPI PsResumeThread(HANDLE ThreadHandle);
```

**Description**: Resumes thread execution.

**Parameters**:
- `ThreadHandle`: Handle to the thread

**Returns**: NTSTATUS code

#### `PsGetThreadPriority`
```c
NTSTATUS NTAPI PsGetThreadPriority(
    HANDLE ThreadHandle,
    PTHREAD_PRIORITY Priority
);
```

**Description**: Gets thread priority.

**Parameters**:
- `ThreadHandle`: Handle to the thread
- `Priority`: Receives thread priority

**Returns**: NTSTATUS code

#### `PsSetThreadPriority`
```c
NTSTATUS NTAPI PsSetThreadPriority(
    HANDLE ThreadHandle,
    THREAD_PRIORITY Priority
);
```

**Description**: Sets thread priority.

**Parameters**:
- `ThreadHandle`: Handle to the thread
- `Priority`: New thread priority

**Returns**: NTSTATUS code

### Thread Information

#### `PsGetCurrentThread`
```c
PTHREAD_CONTROL_BLOCK NTAPI PsGetCurrentThread(VOID);
```

**Description**: Gets current thread control block.

**Returns**: Pointer to current thread control block

#### `PsGetThreadId`
```c
NTSTATUS NTAPI PsGetThreadId(
    HANDLE ThreadHandle,
    PTHREAD_ID ThreadId
);
```

**Description**: Gets thread ID.

**Parameters**:
- `ThreadHandle`: Handle to the thread
- `ThreadId`: Receives thread ID

**Returns**: NTSTATUS code

---

### IPC Communication APIs

### Port Management

#### `IpcCreatePort`
```c
NTSTATUS NTAPI IpcCreatePort(
    PHANDLE PortHandle,
    ULONG MaxConnections
);
```

**Description**: Creates an IPC port.

**Parameters**:
- `PortHandle`: Receives handle to the port
- `MaxConnections`: Maximum number of connections

**Returns**: NTSTATUS code

#### `IpcClosePort`
```c
NTSTATUS NTAPI IpcClosePort(HANDLE PortHandle);
```

**Description**: Closes an IPC port.

**Parameters**:
- `PortHandle`: Handle to the port

**Returns**: NTSTATUS code

### Message Operations

#### `IpcSendMessage`
```c
NTSTATUS NTAPI IpcSendMessage(
    HANDLE PortHandle,
    PVOID Message,
    SIZE_T MessageSize
);
```

**Description**: Sends a message through IPC port.

**Parameters**:
- `PortHandle`: Handle to the port
- `Message`: Message to send
- `MessageSize`: Size of message

**Returns**: NTSTATUS code

#### `IpcReceiveMessage`
```c
NTSTATUS NTAPI IpcReceiveMessage(
    HANDLE PortHandle,
    PVOID Message,
    SIZE_T MessageSize,
    PSIZE_T BytesReceived
);
```

**Description**: Receives a message through IPC port.

**Parameters**:
- `PortHandle`: Handle to the port
- `Message`: Buffer to receive message
- `MessageSize`: Size of buffer
- `BytesReceived`: Receives bytes received

**Returns**: NTSTATUS code

#### `IpcSendReply`
```c
NTSTATUS NTAPI IpcSendReply(
    HANDLE PortHandle,
    PVOID Message,
    SIZE_T MessageSize,
    PVOID Reply,
    SIZE_T ReplySize
);
```

**Description**: Sends a message and waits for reply.

**Parameters**:
- `PortHandle`: Handle to the port
- `Message`: Message to send
- `MessageSize`: Size of message
- `Reply`: Buffer to receive reply
- `ReplySize`: Size of reply buffer

**Returns**: NTSTATUS code

### Connection Management

#### `IpcConnectToPort`
```c
NTSTATUS NTAPI IpcConnectToPort(
    PCWSTR PortName,
    PHANDLE ConnectionHandle
);
```

**Description**: Connects to an IPC port.

**Parameters**:
- `PortName`: Name of the port
- `ConnectionHandle`: Receives connection handle

**Returns**: NTSTATUS code

#### `IpcDisconnectFromPort`
```c
NTSTATUS NTAPI IpcDisconnectFromPort(HANDLE ConnectionHandle);
```

**Description**: Disconnects from IPC port.

**Parameters**:
- `ConnectionHandle`: Connection handle

**Returns**: NTSTATUS code

---

### File System APIs

### Volume Management

#### `DslsfsCreateVolume`
```c
NTSTATUS NTAPI DslsfsCreateVolume(
    PCWSTR VolumeName,
    PCWSTR MountPoint,
    VOLUME_TYPE Type,
    PVOLUME_ID VolumeId
);
```

**Description**: Creates a new volume.

**Parameters**:
- `VolumeName`: Name of the volume
- `MountPoint`: Mount point for the volume
- `Type`: Volume type
- `VolumeId`: Receives volume ID

**Returns**: NTSTATUS code

#### `DslsfsMountVolume`
```c
NTSTATUS NTAPI DslsfsMountVolume(VOLUME_ID VolumeId);
```

**Description**: Mounts a volume.

**Parameters**:
- `VolumeId`: ID of the volume to mount

**Returns**: NTSTATUS code

#### `DslsfsUnmountVolume`
```c
NTSTATUS NTAPI DslsfsUnmountVolume(VOLUME_ID VolumeId);
```

**Description**: Unmounts a volume.

**Parameters**:
- `VolumeId`: ID of the volume to unmount

**Returns**: NTSTATUS code

#### `DslsfsDeleteVolume`
```c
NTSTATUS NTAPI DslsfsDeleteVolume(VOLUME_ID VolumeId);
```

**Description**: Deletes a volume.

**Parameters**:
- `VolumeId`: ID of the volume to delete

**Returns**: NTSTATUS code

### File Operations

#### `DslsfsCreateFile`
```c
NTSTATUS NTAPI DslsfsCreateFile(
    PCWSTR FileName,
    ULONG DesiredAccess,
    ULONG ShareMode,
    PHANDLE FileHandle
);
```

**Description**: Creates or opens a file.

**Parameters**:
- `FileName`: Path to the file
- `DesiredAccess`: Desired access rights
- `ShareMode`: Sharing mode
- `FileHandle`: Receives file handle

**Returns**: NTSTATUS code

#### `DslsfsReadFile`
```c
NTSTATUS NTAPI DslsfsReadFile(
    HANDLE FileHandle,
    PVOID Buffer,
    SIZE_T BytesToRead,
    PSIZE_T BytesRead
);
```

**Description**: Reads from a file.

**Parameters**:
- `FileHandle`: Handle to the file
- `Buffer`: Buffer to receive data
- `BytesToRead`: Number of bytes to read
- `BytesRead`: Receives bytes read

**Returns**: NTSTATUS code

#### `DslsfsWriteFile`
```c
NTSTATUS NTAPI DslsfsWriteFile(
    HANDLE FileHandle,
    PVOID Buffer,
    SIZE_T BytesToWrite,
    PSIZE_T BytesWritten
);
```

**Description**: Writes to a file.

**Parameters**:
- `FileHandle`: Handle to the file
- `Buffer`: Buffer containing data
- `BytesToWrite`: Number of bytes to write
- `BytesWritten`: Receives bytes written

**Returns**: NTSTATUS code

#### `DslsfsDeleteFile`
```c
NTSTATUS NTAPI DslsfsDeleteFile(PCWSTR FileName);
```

**Description**: Deletes a file.

**Parameters**:
- `FileName`: Path to the file

**Returns**: NTSTATUS code

### Directory Operations

#### `DslsfsCreateDirectory`
```c
NTSTATUS NTAPI DslsfsCreateDirectory(PCWSTR DirectoryName);
```

**Description**: Creates a directory.

**Parameters**:
- `DirectoryName`: Path to the directory

**Returns**: NTSTATUS code

#### `DslsfsDeleteDirectory`
```c
NTSTATUS NTAPI DslsfsDeleteDirectory(PCWSTR DirectoryName);
```

**Description**: Deletes a directory.

**Parameters**:
- `DirectoryName`: Path to the directory

**Returns**: NTSTATUS code

#### `DslsfsEnumDirectory`
```c
NTSTATUS NTAPI DslsfsEnumDirectory(
    PCWSTR DirectoryName,
    PDIRECTORY_INFORMATION Buffer,
    ULONG BufferSize,
    PULONG ReturnLength
);
```

**Description**: Enumerates directory contents.

**Parameters**:
- `DirectoryName`: Path to the directory
- `Buffer`: Buffer to receive directory information
- `BufferSize`: Size of buffer
- `ReturnLength`: Receives bytes written

**Returns**: NTSTATUS code

### File Information

#### `DslsfsGetFileSize`
```c
NTSTATUS NTAPI DslsfsGetFileSize(
    HANDLE FileHandle,
    PULONGLONG FileSize
);
```

**Description**: Gets file size.

**Parameters**:
- `FileHandle`: Handle to the file
- `FileSize`: Receives file size

**Returns**: NTSTATUS code

#### `DslsfsSetFileSize`
```c
NTSTATUS NTAPI DslsfsSetFileSize(
    HANDLE FileHandle,
    ULONGLONG FileSize
);
```

**Description**: Sets file size.

**Parameters**:
- `FileHandle`: Handle to the file
- `FileSize`: New file size

**Returns**: NTSTATUS code

#### `DslsfsGetFileAttributes`
```c
NTSTATUS NTAPI DslsfsGetFileAttributes(
    PCWSTR FileName,
    PULONG Attributes
);
```

**Description**: Gets file attributes.

**Parameters**:
- `FileName`: Path to the file
- `Attributes`: Receives file attributes

**Returns**: NTSTATUS code

---

### Security APIs

### Authentication

#### `SmAuthenticateUser`
```c
NTSTATUS NTAPI SmAuthenticateUser(
    PCWSTR Username,
    PCWSTR Password,
    PHANDLE UserToken
);
```

**Description**: Authenticates a user.

**Parameters**:
- `Username`: User name
- `Password`: Password
- `UserToken`: Receives user token

**Returns**: NTSTATUS code

#### `SmCreateUser`
```c
NTSTATUS NTAPI SmCreateUser(
    PCWSTR Username,
    PCWSTR Password,
    PCWSTR FullName,
    PUSER_ID UserId
);
```

**Description**: Creates a new user.

**Parameters**:
- `Username`: User name
- `Password`: Password
- `FullName`: Full name
- `UserId`: Receives user ID

**Returns**: NTSTATUS code

#### `SmDeleteUser`
```c
NTSTATUS NTAPI SmDeleteUser(USER_ID UserId);
```

**Description**: Deletes a user.

**Parameters**:
- `UserId`: User ID to delete

**Returns**: NTSTATUS code

### Authorization

#### `SmCheckAccess`
```c
NTSTATUS NTAPI SmCheckAccess(
    HANDLE UserToken,
    PCWSTR ResourceName,
    ULONG DesiredAccess,
    PBOOLEAN Granted
);
```

**Description**: Checks access rights.

**Parameters**:
- `UserToken`: User token
- `ResourceName`: Resource name
- `DesiredAccess`: Desired access rights
- `Granted`: Receives access granted flag

**Returns**: NTSTATUS code

#### `SmGrantAccess`
```c
NTSTATUS NTAPI SmGrantAccess(
    PCWSTR ResourceName,
    USER_ID UserId,
    ULONG AccessRights
);
```

**Description**: Grants access rights.

**Parameters**:
- `ResourceName`: Resource name
- `UserId`: User ID
- `AccessRights`: Access rights to grant

**Returns**: NTSTATUS code

#### `SmRevokeAccess`
```c
NTSTATUS NTAPI SmRevokeAccess(
    PCWSTR ResourceName,
    USER_ID UserId,
    ULONG AccessRights
);
```

**Description**: Revokes access rights.

**Parameters**:
- `ResourceName`: Resource name
- `UserId`: User ID
- `AccessRights`: Access rights to revoke

**Returns**: NTSTATUS code

### Role Management

#### `SmCreateRole`
```c
NTSTATUS NTAPI SmCreateRole(
    PCWSTR RoleName,
    PCWSTR Description,
    PROLE_ID RoleId
);
```

**Description**: Creates a new role.

**Parameters**:
- `RoleName`: Role name
- `Description`: Role description
- `RoleId`: Receives role ID

**Returns**: NTSTATUS code

#### `SmAssignRole`
```c
NTSTATUS NTAPI SmAssignRole(
    USER_ID UserId,
    ROLE_ID RoleId
);
```

**Description**: Assigns role to user.

**Parameters**:
- `UserId`: User ID
- `RoleId`: Role ID

**Returns**: NTSTATUS code

#### `SmRemoveRole`
```c
NTSTATUS NTAPI SmRemoveRole(
    USER_ID UserId,
    ROLE_ID RoleId
);
```

**Description**: Removes role from user.

**Parameters**:
- `UserId`: User ID
- `RoleId`: Role ID

**Returns**: NTSTATUS code

### Encryption

#### `SmEncryptData`
```c
NTSTATUS NTAPI SmEncryptData(
    PVOID Plaintext,
    SIZE_T PlaintextSize,
    PVOID Ciphertext,
    PSIZE_T CiphertextSize,
    ULONG Algorithm
);
```

**Description**: Encrypts data.

**Parameters**:
- `Plaintext`: Data to encrypt
- `PlaintextSize`: Size of plaintext
- `Ciphertext`: Buffer for ciphertext
- `CiphertextSize`: Size of ciphertext buffer
- `Algorithm`: Encryption algorithm

**Returns**: NTSTATUS code

#### `SmDecryptData`
```c
NTSTATUS NTAPI SmDecryptData(
    PVOID Ciphertext,
    SIZE_T CiphertextSize,
    PVOID Plaintext,
    PSIZE_T PlaintextSize,
    ULONG Algorithm
);
```

**Description**: Decrypts data.

**Parameters**:
- `Ciphertext`: Data to decrypt
- `CiphertextSize`: Size of ciphertext
- `Plaintext`: Buffer for plaintext
- `PlaintextSize`: Size of plaintext buffer
- `Algorithm`: Encryption algorithm

**Returns**: NTSTATUS code

---

### User Interface APIs

### Window Management

#### `UiCreateWindow`
```c
NTSTATUS NTAPI UiCreateWindow(
    PCWSTR WindowTitle,
    WINDOW_TYPE WindowType,
    UI_RECT Bounds,
    ULONG Style,
    PWINDOW_ID WindowId
);
```

**Description**: Creates a new window.

**Parameters**:
- `WindowTitle`: Window title
- `WindowType`: Window type
- `Bounds`: Window bounds
- `Style`: Window style
- `WindowId`: Receives window ID

**Returns**: NTSTATUS code

#### `UiShowWindow`
```c
NTSTATUS NTAPI UiShowWindow(WINDOW_ID WindowId);
```

**Description**: Shows a window.

**Parameters**:
- `WindowId`: Window ID

**Returns**: NTSTATUS code

#### `UiHideWindow`
```c
NTSTATUS NTAPI UiHideWindow(WINDOW_ID WindowId);
```

**Description**: Hides a window.

**Parameters**:
- `WindowId`: Window ID

**Returns**: NTSTATUS code

#### `UiDestroyWindow`
```c
NTSTATUS NTAPI UiDestroyWindow(WINDOW_ID WindowId);
```

**Description**: Destroys a window.

**Parameters**:
- `WindowId`: Window ID

**Returns**: NTSTATUS code

### Control Management

#### `UiCreateControl`
```c
NTSTATUS NTAPI UiCreateControl(
    WINDOW_ID WindowId,
    CONTROL_TYPE ControlType,
    PCWSTR ControlName,
    UI_RECT Bounds,
    PCONTROL_ID ControlId
);
```

**Description**: Creates a control.

**Parameters**:
- `WindowId`: Parent window ID
- `ControlType`: Control type
- `ControlName`: Control name
- `Bounds`: Control bounds
- `ControlId`: Receives control ID

**Returns**: NTSTATUS code

#### `UiDestroyControl`
```c
NTSTATUS NTAPI UiDestroyControl(CONTROL_ID ControlId);
```

**Description**: Destroys a control.

**Parameters**:
- `ControlId`: Control ID

**Returns**: NTSTATUS code

### UI Mode Management

#### `UiSetUiMode`
```c
NTSTATUS NTAPI UiSetUiMode(UI_MODE Mode);
```

**Description**: Sets UI mode.

**Parameters**:
- `Mode`: UI mode to set

**Returns**: NTSTATUS code

#### `UiGetUiMode`
```c
UI_MODE NTAPI UiGetUiMode(VOID);
```

**Description**: Gets current UI mode.

**Returns**: Current UI mode

#### `UiRunEventLoop`
```c
NTSTATUS NTAPI UiRunEventLoop(VOID);
```

**Description**: Runs UI event loop.

**Returns**: NTSTATUS code

### UI Statistics

#### `UiGetUiStatistics`
```c
NTSTATUS NTAPI UiGetUiStatistics(PUI_STATS Stats);
```

**Description**: Gets UI statistics.

**Parameters**:
- `Stats`: Receives UI statistics

**Returns**: NTSTATUS code

#### `UiIsCompositeUiInitialized`
```c
BOOLEAN NTAPI UiIsCompositeUiInitialized(VOID);
```

**Description**: Checks if UI is initialized.

**Returns**: TRUE if initialized, FALSE otherwise

---

### Distributed Management APIs

### Cluster Management

#### `DmCreateCluster`
```c
NTSTATUS NTAPI DmCreateCluster(
    PCWSTR ClusterName,
    PCWSTR ClusterDescription,
    PCLUSTER_CONFIG Config,
    PCLUSTER_ID ClusterId
);
```

**Description**: Creates a new cluster.

**Parameters**:
- `ClusterName`: Cluster name
- `ClusterDescription`: Cluster description
- `Config`: Cluster configuration
- `ClusterId`: Receives cluster ID

**Returns**: NTSTATUS code

#### `DmJoinCluster`
```c
NTSTATUS NTAPI DmJoinCluster(
    CLUSTER_ID ClusterId,
    PCWSTR NodeAddress
);
```

**Description**: Joins a cluster.

**Parameters**:
- `ClusterId`: Cluster ID
- `NodeAddress`: Node address

**Returns**: NTSTATUS code

#### `DmLeaveCluster`
```c
NTSTATUS NTAPI DmLeaveCluster(VOID);
```

**Description**: Leaves the current cluster.

**Returns**: NTSTATUS code

### Node Management

#### `DmSendHeartbeat`
```c
NTSTATUS NTAPI DmSendHeartbeat(NODE_ID NodeId);
```

**Description**: Sends heartbeat to node.

**Parameters**:
- `NodeId`: Node ID

**Returns**: NTSTATUS code

#### `DmGetNodeInfo`
```c
NTSTATUS NTAPI DmGetNodeInfo(
    NODE_ID NodeId,
    PNODE_INFO NodeInfo
);
```

**Description**: Gets node information.

**Parameters**:
- `NodeId`: Node ID
- `NodeInfo`: Receives node information

**Returns**: NTSTATUS code

### Service Management

#### `DmCreateService`
```c
NTSTATUS NTAPI DmCreateService(
    PCWSTR ServiceName,
    PCWSTR ServiceType,
    PSERVICE_CONFIG Config,
    PSERVICE_ID ServiceId
);
```

**Description**: Creates a service.

**Parameters**:
- `ServiceName`: Service name
- `ServiceType`: Service type
- `Config`: Service configuration
- `ServiceId`: Receives service ID

**Returns**: NTSTATUS code

#### `DmStartService`
```c
NTSTATUS NTAPI DmStartService(SERVICE_ID ServiceId);
```

**Description**: Starts a service.

**Parameters**:
- `ServiceId`: Service ID

**Returns**: NTSTATUS code

#### `DmStopService`
```c
NTSTATUS NTAPI DmStopService(
    SERVICE_ID ServiceId,
    BOOLEAN Force
);
```

**Description**: Stops a service.

**Parameters**:
- `ServiceId`: Service ID
- `Force`: Force stop flag

**Returns**: NTSTATUS code

#### `DmScaleService`
```c
NTSTATUS NTAPI DmScaleService(
    SERVICE_ID ServiceId,
    ULONG Replicas
);
```

**Description**: Scales a service.

**Parameters**:
- `ServiceId`: Service ID
- `Replicas`: Number of replicas

**Returns**: NTSTATUS code

### Statistics

#### `DmGetDistributedSystemStatistics`
```c
NTSTATUS NTAPI DmGetDistributedSystemStatistics(
    PDISTRIBUTED_SYSTEM_STATS Stats
);
```

**Description**: Gets distributed system statistics.

**Parameters**:
- `Stats`: Receives statistics

**Returns**: NTSTATUS code

#### `DmIsDistributedSystemInitialized`
```c
BOOLEAN NTAPI DmIsDistributedSystemInitialized(VOID);
```

**Description**: Checks if distributed system is initialized.

**Returns**: TRUE if initialized, FALSE otherwise

---

### System Test APIs

### Test Management

#### `TmInitializeTestManager`
```c
NTSTATUS TmInitializeTestManager(VOID);
```

**Description**: Initializes test manager.

**Returns**: NTSTATUS code

#### `TmRunAllTests`
```c
NTSTATUS TmRunAllTests(VOID);
```

**Description**: Runs all tests.

**Returns**: NTSTATUS code

#### `TmRunTestSuiteByName`
```c
NTSTATUS TmRunTestSuiteByName(PCWSTR SuiteName);
```

**Description**: Runs specific test suite.

**Parameters**:
- `SuiteName`: Name of test suite

**Returns**: NTSTATUS code

#### `TmRunSpecificTest`
```c
NTSTATUS TmRunSpecificTest(
    PCWSTR SuiteName,
    PCWSTR TestName
);
```

**Description**: Runs specific test.

**Parameters**:
- `SuiteName`: Name of test suite
- `TestName`: Name of test

**Returns**: NTSTATUS code

### Test Statistics

#### `TmGetTestStatistics`
```c
VOID TmGetTestStatistics(
    PULONG TotalTests,
    PULONG PassedTests,
    PULONG FailedTests
);
```

**Description**: Gets test statistics.

**Parameters**:
- `TotalTests`: Receives total tests
- `PassedTests`: Receives passed tests
- `FailedTests`: Receives failed tests

#### `TmGetTestDuration`
```c
ULONG TmGetTestDuration(VOID);
```

**Description**: Gets test duration.

**Returns**: Test duration in milliseconds

#### `TmAllTestsPassed`
```c
BOOLEAN TmAllTestsPassed(VOID);
```

**Description**: Checks if all tests passed.

**Returns**: TRUE if all passed, FALSE otherwise

---

## Error Codes

### Common NTSTATUS Codes

| Value | Name | Description |
|-------|------|-------------|
| 0x00000000 | STATUS_SUCCESS | Operation completed successfully |
| 0xC0000001 | STATUS_UNSUCCESSFUL | Operation failed |
| 0xC0000002 | STATUS_NOT_IMPLEMENTED | Not implemented |
| 0xC0000004 | STATUS_INVALID_PARAMETER | Invalid parameter |
| 0xC0000008 | STATUS_INVALID_HANDLE | Invalid handle |
| 0xC000000F | STATUS_NO_MEMORY | Insufficient memory |
| 0xC0000010 | STATUS_ACCESS_DENIED | Access denied |
| 0xC0000011 | STATUS_BUFFER_TOO_SMALL | Buffer too small |
| 0xC0000012 | STATUS_OBJECT_NAME_NOT_FOUND | Object not found |
| 0xC0000022 | STATUS_ACCESS_VIOLATION | Access violation |
| 0xC0000023 | STATUS_IN_PAGE_ERROR | In-page error |
| 0xC0000034 | STATUS_OBJECT_NAME_COLLISION | Name collision |
| 0xC0000035 | STATUS_OBJECT_PATH_INVALID | Invalid path |
| 0xC000003A | STATUS_OBJECT_TYPE_MISMATCH | Type mismatch |
| 0xC000006A | STATUS_INSUFFICIENT_RESOURCES | Insufficient resources |
| 0xC000006D | STATUS_LOGON_FAILURE | Logon failure |
| 0xC000006E | STATUS_ACCOUNT_RESTRICTION | Account restriction |
| 0xC000007A | STATUS_INVALID_DEVICE_STATE | Invalid device state |
| 0xC00000BB | STATUS_NOT_SUPPORTED | Not supported |
| 0xC00000BC | STATUS_TIMEOUT | Timeout |
| 0xC00000BA | STATUS_FILE_INVALID | File invalid |
| 0xC00000C0 | STATUS_DEVICE_NOT_READY | Device not ready |
| 0xC00000EA | STATUS_TOO_MANY_OPENED_FILES | Too many open files |
| 0xC000010A | STATUS_PIPE_BROKEN | Pipe broken |
| 0xC0000120 | STATUS_CANCELLED | Operation cancelled |
| 0xC000013A | STATUS_CONTROL_C_EXIT | Control-C exit |
| 0xC0000142 | STATUS_DLL_INIT_FAILED | DLL initialization failed |
| 0xC000015B | STATUS_IO_TIMEOUT | I/O timeout |
| 0xC0000185 | STATUS_INVALID_LOCK_SEQUENCE | Invalid lock sequence |
| 0xC00001A1 | STATUS_INVALID_IMAGE_FORMAT | Invalid image format |
| 0xC00001A2 | STATUS_IMAGE_ALREADY_LOADED | Image already loaded |
| 0xC00001B2 | STATUS_DEVICE_BUSY | Device busy |
| 0xC00001B5 | STATUS_DEVICE_DATA_ERROR | Device data error |
| 0xC00001B6 | STATUS_DEVICE_NOT_CONNECTED | Device not connected |
| 0xC00001B7 | STATUS_DEVICE_POWER_FAILURE | Device power failure |
| 0xC00001B8 | STATUS_MEDIA_WRITE_PROTECTED | Media write protected |
| 0xC00001B9 | STATUS_NO_MEDIA_IN_DEVICE | No media in device |
| 0xC00001BA | STATUS_IO_DEVICE_ERROR | I/O device error |
| 0xC00001BB | STATUS_SERIAL_NO_DEVICE | No serial device |
| 0xC00001BC | STATUS_IRQ_BUSY | IRQ busy |
| 0xC00001BD | STATUS_MORE_WRITES | More writes required |
| 0xC00001BE | STATUS_COUNTER_TIMEOUT | Counter timeout |
| 0xC00001BF | STATUS_FLOPPY_BAD_REGISTERS | Floppy bad registers |
| 0xC00001C0 | STATUS_DISK_RECALIBRATE_FAILED | Disk recalibrate failed |
| 0xC00001C1 | STATUS_DISK_OPERATION_FAILED | Disk operation failed |
| 0xC00001C2 | STATUS_DISK_RESET_FAILED | Disk reset failed |
| 0xC00001C3 | STATUS_SECTOR_NOT_FOUND | Sector not found |
| 0xC00001C4 | STATUS_OUT_OF_PAPER | Out of paper |
| 0xC00001C5 | STATUS_WRITE_PROTECT | Write protect |
| 0xC00001C6 | STATUS_MEDIA_CHANGED | Media changed |
| 0xC00001C7 | STATUS_BUS_RESET | Bus reset |
| 0xC00001C8 | STATUS_NO_MORE_EAS | No more extended attributes |
| 0xC00001C9 | STATUS_EA_TOO_LARGE | EA too large |
| 0xC00001CA | STATUS_EA_LIST_INCONSISTENT | EA list inconsistent |
| 0xC00001CB | STATUS_INVALID_EA_NAME | Invalid EA name |
| 0xC00001CC | STATUS_EA_CORRUPT_ERROR | EA corrupt error |
| 0xC00001CD | STATUS_FILE_LOCK_CONFLICT | File lock conflict |
| 0xC00001CE | STATUS_LOCK_NOT_GRANTED | Lock not granted |
| 0xC00001CF | STATUS_DELETE_PENDING | Delete pending |
| 0xC00001D0 | STATUS_FILE_DELETED | File deleted |
| 0xC00001D1 | STATUS_PRIVILEGE_NOT_HELD | Privilege not held |
| 0xC00001D2 | STATUS_NO_KERNER_MEMORY | No kernel memory |
| 0xC00001D3 | STATUS_QUOTA_EXCEEDED | Quota exceeded |

---

*This API reference covers the major components of DslsOS. For more detailed information about specific APIs, refer to the individual component documentation.*