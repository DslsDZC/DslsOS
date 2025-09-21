/**
 * @file dslsfs.h
 * @brief DslsFS distributed file system interface
 * @author DslsOS Team
 * @version 1.0
 * @date 2024
 */

#ifndef _DSLSFS_H_
#define _DSLSFS_H_

#include "kernel.h"
#include "dslos.h"

// DslsFS constants
#define DSLSFS_MAGIC_NUMBER           0x44534C53  // "DSLS"
#define DSLSFS_VERSION                0x00010000
#define DSLSFS_DEFAULT_BLOCK_SIZE     4096
#define DSLSFS_DEFAULT_CLUSTER_SIZE   32768
#define DSLSFS_DEFAULT_REPLICATION_FACTOR 3
#define DSLSFS_DEFAULT_CONSISTENCY_LEVEL 2
#define DSLSFS_DEFAULT_CACHE_SIZE     1048576    // 1MB
#define DSLSFS_DEFAULT_JOURNAL_SIZE   104857600  // 100MB
#define DSLSFS_MAX_VOLUMES            64
#define DSLSFS_MAX_FILES              1000000
#define DSLSFS_MAX_DIRECTORIES        100000
#define DSLSFS_MAX_CONNECTIONS        1000
#define DSLSFS_TIMEOUT                30000       // 30 seconds

// Type definitions
typedef ULONG_PTR VOLUME_ID;
typedef ULONG_PTR FILE_ID;
typedef ULONG_PTR DIRECTORY_ID;
typedef ULONG_PTR INODE_ID;
typedef ULONG_PTR REPLICATION_GROUP_ID;
typedef ULONG_PTR NODE_ID;
typedef ULONG_PTR LOCK_ID;
typedef ULONG_PTR JOURNAL_ENTRY_ID;

// Forward declarations
typedef struct _DSLSFS_VOLUME DSLSFS_VOLUME, *PDSLSFS_VOLUME;
typedef struct _DSLSFS_FILE DSLSFS_FILE, *PDSLSFS_FILE;
typedef struct _DSLSFS_DIRECTORY DSLSFS_DIRECTORY, *PDSLSFS_DIRECTORY;
typedef struct _DSLSFS_INODE DSLSFS_INODE, *PDSLSFS_INODE;
typedef struct _DSLSFS_JOURNAL DSLSFS_JOURNAL, *PDSLSFS_JOURNAL;
typedef struct _DSLSFS_CACHE DSLSFS_CACHE, *PDSLSFS_CACHE;

// Volume types
typedef enum _VOLUME_TYPE {
    VolumeTypeLocal = 0,
    VolumeTypeDistributed,
    VolumeTypeReplicated,
    VolumeTypeHybrid,
    VolumeTypeMaximum
} VOLUME_TYPE;

// Volume states
typedef enum _VOLUME_STATE {
    VolumeStateUnmounted = 0,
    VolumeStateMounting,
    VolumeStateMounted,
    VolumeStateUnmounting,
    VolumeStateChecking,
    VolumeStateRepairing,
    VolumeStateFailed
} VOLUME_STATE;

// File types
typedef enum _DSLSFS_FILE_TYPE {
    FileTypeRegular = 0,
    FileTypeDirectory,
    FileTypeSymbolicLink,
    FileTypeBlockDevice,
    FileTypeCharacterDevice,
    FileTypeFifo,
    FileTypeSocket,
    FileTypeMaximum
} DSLSFS_FILE_TYPE;

// File states
typedef enum _FILE_STATE {
    FileStateClosed = 0,
    FileStateOpening,
    FileStateOpen,
    FileStateClosing,
    FileStateReading,
    FileStateWriting,
    FileStateDeleting,
    FileStateFailed
} FILE_STATE;

// Directory states
typedef enum _DIRECTORY_STATE {
    DirectoryStateClosed = 0,
    DirectoryStateOpening,
    DirectoryStateOpen,
    DirectoryStateClosing,
    DirectoryStateDeleting,
    DirectoryStateFailed
} DIRECTORY_STATE;

// Inode types
typedef enum _DSLSFS_INODE_TYPE {
    InodeTypeRegular = 0,
    InodeTypeDirectory,
    InodeTypeSymlink,
    InodeTypeBlockDevice,
    InodeTypeCharacterDevice,
    InodeTypeFifo,
    InodeTypeSocket,
    InodeTypeMaximum
} DSLSFS_INODE_TYPE;

// Journal operations
typedef enum _JOURNAL_OPERATION {
    JournalOperationCreate = 0,
    JournalOperationWrite,
    JournalOperationDelete,
    JournalOperationRename,
    JournalOperationSetAttribute,
    JournalOperationTruncate,
    JournalOperationMaximum
} JOURNAL_OPERATION;

// Volume flags
#define VOLUME_FLAG_READ_ONLY         0x00000001
#define VOLUME_FLAG_COMPRESSED        0x00000002
#define VOLUME_FLAG_ENCRYPTED         0x00000004
#define VOLUME_FLAG_DEDUPLICATED      0x00000008
#define VOLUME_FLAG_REPLICATED        0x00000010
#define VOLUME_FLAG_JOURNALING        0x00000020
#define VOLUME_FLAG_CACHED            0x00000040
#define VOLUME_FLAG_BACKUP_VOLUME     0x00000080
#define VOLUME_FLAG_SYSTEM_VOLUME     0x00000100

// File flags
#define FILE_FLAG_READ_ONLY           0x00000001
#define FILE_FLAG_HIDDEN              0x00000002
#define FILE_FLAG_SYSTEM              0x00000004
#define FILE_FLAG_ARCHIVE             0x00000008
#define FILE_FLAG_TEMPORARY           0x00000010
#define FILE_FLAG_COMPRESSED          0x00000020
#define FILE_FLAG_ENCRYPTED           0x00000040
#define FILE_FLAG_DEDUPLICATED        0x00000080
#define FILE_FLAG_REPLICATED          0x00000100
#define FILE_FLAG_CACHED              0x00000200

// File attributes
#define FILE_ATTRIBUTE_READONLY        0x00000001
#define FILE_ATTRIBUTE_HIDDEN         0x00000002
#define FILE_ATTRIBUTE_SYSTEM         0x00000004
#define FILE_ATTRIBUTE_DIRECTORY      0x00000010
#define FILE_ATTRIBUTE_ARCHIVE        0x00000020
#define FILE_ATTRIBUTE_TEMPORARY      0x00000100
#define FILE_ATTRIBUTE_COMPRESSED     0x00000800
#define FILE_ATTRIBUTE_ENCRYPTED      0x00004000

// Share modes
#define FILE_SHARE_READ               0x00000001
#define FILE_SHARE_WRITE              0x00000002
#define FILE_SHARE_DELETE             0x00000004

// Create dispositions
#define FILE_SUPERSEDE                0x00000000
#define FILE_OPEN                     0x00000001
#define FILE_CREATE                   0x00000002
#define FILE_OPEN_IF                  0x00000003
#define FILE_OVERWRITE                0x00000004
#define FILE_OVERWRITE_IF             0x00000005

// I/O control codes
#define IOCTL_MOUNT_VOLUME            CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_UNMOUNT_VOLUME          CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_CHECK_VOLUME            CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_REPAIR_VOLUME           CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)

// Superblock structure
typedef struct _DSLSFS_SUPERBLOCK {
    UNICODE_STRING VolumeLabel;
    LARGE_INTEGER VolumeSize;
    ULONG BlockSize;
    ULONG ClusterSize;
    ULONG TotalBlocks;
    ULONG FreeBlocks;
    ULONG TotalInodes;
    ULONG FreeInodes;
    ULONG MagicNumber;
    ULONG Version;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastMountTime;
    LARGE_INTEGER LastCheckTime;
    ULONG State;
    ULONG Errors;
} DSLSFS_SUPERBLOCK, *PDSLSFS_SUPERBLOCK;

// Inode structure
typedef struct _DSLSFS_INODE {
    INODE_ID InodeId;
    DSLSFS_INODE_TYPE InodeType;
    ULONG Mode;
    ULONG LinkCount;
    ULONG UserId;
    ULONG GroupId;
    LARGE_INTEGER Size;
    LARGE_INTEGER AllocationSize;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastModificationTime;
    LARGE_INTEGER LastChangeTime;
    LIST_ENTRY ExtentListHead;
    ULONG DirectBlocks[12];
    ULONG IndirectBlock;
    ULONG DoubleIndirectBlock;
    ULONG TripleIndirectBlock;
} DSLSFS_INODE, *PDSLSFS_INODE;

// Bitmap structure
typedef struct _DSLSFS_BITMAP {
    ULONG BitmapSize;
    ULONG TotalBits;
    PULONG BitmapData;
    KSPIN_LOCK BitmapLock;
} DSLSFS_BITMAP, *PDSLSFS_BITMAP;

// Journal structure
typedef struct _DSLSFS_JOURNAL {
    ULONG JournalSize;
    ULONG JournalBlockSize;
    ULONG JournalEntries;
    ULONG JournalHead;
    ULONG JournalTail;
    LIST_ENTRY JournalEntryListHead;
    KSPIN_LOCK JournalLock;
} DSLSFS_JOURNAL, *PDSLSFS_JOURNAL;

// Cache structure
typedef struct _DSLSFS_CACHE {
    ULONG CacheSize;
    ULONG CacheBlockSize;
    ULONG CacheLineSize;
    ULONG CacheAssociativity;
    volatile ULONG CacheHits;
    volatile ULONG CacheMisses;
    volatile ULONG CacheEvictions;
    LIST_ENTRY CacheLineListHead;
    KSPIN_LOCK CacheLock;
} DSLSFS_CACHE, *PDSLSFS_CACHE;

// Statistics structure
typedef struct _DSLSFS_STATISTICS {
    ULONG TotalReads;
    ULONG TotalWrites;
    ULONG TotalOpens;
    ULONG TotalCloses;
    ULONG TotalCreates;
    ULONG TotalDeletes;
    ULONG CacheHits;
    ULONG CacheMisses;
    ULONG CacheEvictions;
    ULONG JournalOperations;
    ULONG ReplicationOperations;
    ULONG FailedOperations;
    LARGE_INTEGER TotalReadBytes;
    LARGE_INTEGER TotalWriteBytes;
    LARGE_INTEGER AverageReadLatency;
    LARGE_INTEGER AverageWriteLatency;
    LARGE_INTEGER AverageCacheLatency;
} DSLSFS_STATISTICS, *PDSLSFS_STATISTICS;

// Configuration structure
typedef struct _DSLSFS_CONFIG {
    ULONG DefaultBlockSize;
    ULONG DefaultClusterSize;
    ULONG DefaultReplicationFactor;
    ULONG DefaultConsistencyLevel;
    ULONG CacheSize;
    ULONG JournalSize;
    ULONG MaxVolumes;
    ULONG MaxFiles;
    ULONG MaxDirectories;
    ULONG MaxConnections;
    ULONG Timeout;
    BOOLEAN EnableCompression;
    BOOLEAN EnableEncryption;
    BOOLEAN EnableDeduplication;
    BOOLEAN EnableJournaling;
    BOOLEAN EnableCaching;
    BOOLEAN EnableReplication;
} DSLSFS_CONFIG, *PDSLSFS_CONFIG;

// Operations structure
typedef struct _DSLSFS_OPERATIONS {
    NTSTATUS (*CreateVolume)(PCWSTR, LARGE_INTEGER, VOLUME_TYPE, PCWSTR*, ULONG, PDSLSFS_VOLUME*);
    NTSTATUS (*DeleteVolume)(PDSLSFS_VOLUME);
    NTSTATUS (*MountVolume)(PDSLSFS_VOLUME);
    NTSTATUS (*UnmountVolume)(PDSLSFS_VOLUME);
    NTSTATUS (*CheckVolume)(PDSLSFS_VOLUME, BOOLEAN);
    NTSTATUS (*RepairVolume)(PDSLSFS_VOLUME);

    NTSTATUS (*CreateFile)(PDSLSFS_VOLUME, PCWSTR, PCWSTR, ULONG, PDSLSFS_FILE*);
    NTSTATUS (*OpenFile)(PDSLSFS_VOLUME, PCWSTR, ACCESS_MASK, ULONG, ULONG, PDSLSFS_FILE*);
    NTSTATUS (*CloseFile)(PDSLSFS_FILE);
    NTSTATUS (*ReadFile)(PDSLSFS_FILE, PVOID, SIZE_T, SIZE_T*, LARGE_INTEGER);
    NTSTATUS (*WriteFile)(PDSLSFS_FILE, PVOID, SIZE_T, SIZE_T*, LARGE_INTEGER);
    NTSTATUS (*DeleteFile)(PDSLSFS_FILE);
    NTSTATUS (*RenameFile)(PDSLSFS_FILE, PCWSTR, PCWSTR);
    NTSTATUS (*SetFileAttributes)(PDSLSFS_FILE, ULONG);
    NTSTATUS (*GetFileInformation)(PDSLSFS_FILE, PDSLSFS_FILE_INFORMATION);

    NTSTATUS (*CreateDirectory)(PDSLSFS_VOLUME, PCWSTR, PCWSTR, ULONG, PDSLSFS_DIRECTORY*);
    NTSTATUS (*OpenDirectory)(PDSLSFS_VOLUME, PCWSTR, PDSLSFS_DIRECTORY*);
    NTSTATUS (*CloseDirectory)(PDSLSFS_DIRECTORY);
    NTSTATUS (*DeleteDirectory)(PDSLSFS_DIRECTORY);
    NTSTATUS (*RenameDirectory)(PDSLSFS_DIRECTORY, PCWSTR, PCWSTR);
    NTSTATUS (*ReadDirectory)(PDSLSFS_DIRECTORY, PVOID, SIZE_T, SIZE_T*, LARGE_INTEGER);
} DSLSFS_OPERATIONS, *PDSLSFS_OPERATIONS;

// Volume operations structure
typedef struct _DSLSFS_VOLUME_OPERATIONS {
    NTSTATUS (*CreateFile)(PDSLSFS_VOLUME, PCWSTR, PCWSTR, ULONG, PDSLSFS_FILE*);
    NTSTATUS (*OpenFile)(PDSLSFS_VOLUME, PCWSTR, ACCESS_MASK, ULONG, ULONG, PDSLSFS_FILE*);
    NTSTATUS (*CloseFile)(PDSLSFS_FILE);
    NTSTATUS (*ReadFile)(PDSLSFS_FILE, PVOID, SIZE_T, SIZE_T*, LARGE_INTEGER);
    NTSTATUS (*WriteFile)(PDSLSFS_FILE, PVOID, SIZE_T, SIZE_T*, LARGE_INTEGER);
    NTSTATUS (*DeleteFile)(PDSLSFS_FILE);
    NTSTATUS (*RenameFile)(PDSLSFS_FILE, PCWSTR, PCWSTR);
    NTSTATUS (*SetFileAttributes)(PDSLSFS_FILE, ULONG);
    NTSTATUS (*GetFileInformation)(PDSLSFS_FILE, PDSLSFS_FILE_INFORMATION);
    NTSTATUS (*CreateDirectory)(PDSLSFS_VOLUME, PCWSTR, PCWSTR, ULONG, PDSLSFS_DIRECTORY*);
    NTSTATUS (*OpenDirectory)(PDSLSFS_VOLUME, PCWSTR, PDSLSFS_DIRECTORY*);
    NTSTATUS (*CloseDirectory)(PDSLSFS_DIRECTORY);
    NTSTATUS (*DeleteDirectory)(PDSLSFS_DIRECTORY);
    NTSTATUS (*RenameDirectory)(PDSLSFS_DIRECTORY, PCWSTR, PCWSTR);
    NTSTATUS (*ReadDirectory)(PDSLSFS_DIRECTORY, PVOID, SIZE_T, SIZE_T*, LARGE_INTEGER);
} DSLSFS_VOLUME_OPERATIONS, *PDSLSFS_VOLUME_OPERATIONS;

// File information structure
typedef struct _DSLSFS_FILE_INFORMATION {
    FILE_ID FileId;
    UNICODE_STRING FileName;
    DSLSFS_FILE_TYPE FileType;
    LARGE_INTEGER FileSize;
    LARGE_INTEGER AllocationSize;
    ULONG Attributes;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
} DSLSFS_FILE_INFORMATION, *PDSLSFS_FILE_INFORMATION;

// Directory entry structure
typedef struct _DSLSFS_DIRECTORY_ENTRY {
    UNICODE_STRING EntryName;
    DSLSFS_ENTRY_TYPE EntryType;
    union {
        INODE_ID InodeId;
        FILE_ID FileId;
        DIRECTORY_ID DirectoryId;
    };
    ULONG Attributes;
    LARGE_INTEGER Size;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
} DSLSFS_DIRECTORY_ENTRY, *PDSLSFS_DIRECTORY_ENTRY;

// Entry types
typedef enum _DSLSFS_ENTRY_TYPE {
    EntryTypeFile = 0,
    EntryTypeDirectory,
    EntryTypeSymlink,
    EntryTypeMaximum
} DSLSFS_ENTRY_TYPE;

// DslsFS API functions

// Initialize DslsFS
NTSTATUS
NTAPI
DslsfsInitialize(
    VOID
);

// Create volume
NTSTATUS
NTAPI
DslsfsCreateVolume(
    _In_ PCWSTR VolumeName,
    _In_ LARGE_INTEGER VolumeSize,
    _In_ VOLUME_TYPE VolumeType,
    _In_reads_(DeviceCount) PCWSTR* DevicePaths,
    _In_ ULONG DeviceCount,
    _Outptr_ PDSLSFS_VOLUME* VolumeObject
);

// Delete volume
NTSTATUS
NTAPI
DslsfsDeleteVolume(
    _In_ PDSLSFS_VOLUME Volume
);

// Mount volume
NTSTATUS
NTAPI
DslsfsMountVolume(
    _In_ PDSLSFS_VOLUME Volume
);

// Unmount volume
NTSTATUS
NTAPI
DslsfsUnmountVolume(
    _In_ PDSLSFS_VOLUME Volume
);

// Check volume
NTSTATUS
NTAPI
DslsfsCheckVolume(
    _In_ PDSLSFS_VOLUME Volume,
    _In_ BOOLEAN Repair
);

// Repair volume
NTSTATUS
NTAPI
DslsfsRepairVolume(
    _In_ PDSLSFS_VOLUME Volume
);

// Create file
NTSTATUS
NTAPI
DslsfsCreateFile(
    _In_ PDSLSFS_VOLUME Volume,
    _In_ PCWSTR FileName,
    _In_opt_ PCWSTR FilePath,
    _In_ ULONG Attributes,
    _Outptr_ PDSLSFS_FILE* FileObject
);

// Open file
NTSTATUS
NTAPI
DslsfsOpenFile(
    _In_ PDSLSFS_VOLUME Volume,
    _In_ PCWSTR FilePath,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG ShareMode,
    _In_ ULONG CreateDisposition,
    _Outptr_ PDSLSFS_FILE* FileObject
);

// Close file
NTSTATUS
NTAPI
DslsfsCloseFile(
    _In_ PDSLSFS_FILE File
);

// Read file
NTSTATUS
NTAPI
DslsfsReadFile(
    _In_ PDSLSFS_FILE File,
    _Out_writes_bytes_(BufferSize) PVOID Buffer,
    _In_ SIZE_T BufferSize,
    _Out_opt_ SIZE_T* BytesRead,
    _In_ LARGE_INTEGER Offset
);

// Write file
NTSTATUS
NTAPI
DslsfsWriteFile(
    _In_ PDSLSFS_FILE File,
    _In_reads_bytes_(BufferSize) PVOID Buffer,
    _In_ SIZE_T BufferSize,
    _Out_opt_ SIZE_T* BytesWritten,
    _In_ LARGE_INTEGER Offset
);

// Delete file
NTSTATUS
NTAPI
DslsfsDeleteFile(
    _In_ PDSLSFS_FILE File
);

// Rename file
NTSTATUS
NTAPI
DslsfsRenameFile(
    _In_ PDSLSFS_FILE File,
    _In_ PCWSTR NewName,
    _In_opt_ PCWSTR NewPath
);

// Set file attributes
NTSTATUS
NTAPI
DslsfsSetFileAttributes(
    _In_ PDSLSFS_FILE File,
    _In_ ULONG Attributes
);

// Get file information
NTSTATUS
NTAPI
DslsfsGetFileInformation(
    _In_ PDSLSFS_FILE File,
    _Out_ PDSLSFS_FILE_INFORMATION FileInformation
);

// Create directory
NTSTATUS
NTAPI
DslsfsCreateDirectory(
    _In_ PDSLSFS_VOLUME Volume,
    _In_ PCWSTR DirectoryName,
    _In_opt_ PCWSTR DirectoryPath,
    _In_ ULONG Attributes,
    _Outptr_ PDSLSFS_DIRECTORY* DirectoryObject
);

// Open directory
NTSTATUS
NTAPI
DslsfsOpenDirectory(
    _In_ PDSLSFS_VOLUME Volume,
    _In_ PCWSTR DirectoryPath,
    _Outptr_ PDSLSFS_DIRECTORY* DirectoryObject
);

// Close directory
NTSTATUS
NTAPI
DslsfsCloseDirectory(
    _In_ PDSLSFS_DIRECTORY Directory
);

// Delete directory
NTSTATUS
NTAPI
DslsfsDeleteDirectory(
    _In_ PDSLSFS_DIRECTORY Directory
);

// Rename directory
NTSTATUS
NTAPI
DslsfsRenameDirectory(
    _In_ PDSLSFS_DIRECTORY Directory,
    _In_ PCWSTR NewName,
    _In_opt_ PCWSTR NewPath
);

// Read directory
NTSTATUS
NTAPI
DslsfsReadDirectory(
    _In_ PDSLSFS_DIRECTORY Directory,
    _Out_writes_bytes_(BufferSize) PVOID Buffer,
    _In_ SIZE_T BufferSize,
    _Out_opt_ SIZE_T* BytesRead,
    _In_ LARGE_INTEGER Offset
);

// Find volume by name
PDSLSFS_VOLUME
NTAPI
DslsfsFindVolumeByName(
    _In_ PCWSTR VolumeName
);

// Find file by name
PDSLSFS_FILE
NTAPI
DslsfsFindFileByName(
    _In_ PDSLSFS_VOLUME Volume,
    _In_ PCWSTR FilePath
);

// Get DslsFS statistics
VOID
NTAPI
DslsfsGetStatistics(
    _Out_ PDSLSFS_STATISTICS Statistics
);

// Set DslsFS configuration
NTSTATUS
NTAPI
DslsfsSetConfiguration(
    _In_ PDSLSFS_CONFIG Config
);

// Get DslsFS configuration
VOID
NTAPI
DslsfsGetConfiguration(
    _Out_ PDSLSFS_CONFIG Config
);

// Utility functions

// Format time string
NTSTATUS
NTAPI
DslsfsFormatTimeString(
    _In_ LARGE_INTEGER Time,
    _Out_writes_(BufferSize) PWSTR Buffer,
    _In_ SIZE_T BufferSize
);

// Format size string
NTSTATUS
NTAPI
DslsfsFormatSizeString(
    _In_ LARGE_INTEGER Size,
    _Out_writes_(BufferSize) PWSTR Buffer,
    _In_ SIZE_T BufferSize
);

// Validate file name
BOOLEAN
NTAPI
DslsfsIsValidFileName(
    _In_ PCWSTR FileName
);

// Validate file path
BOOLEAN
NTAPI
DslsfsIsValidFilePath(
    _In_ PCWSTR FilePath
);

// Extract file name from path
NTSTATUS
NTAPI
DslsfsExtractFileName(
    _In_ PCWSTR FilePath,
    _Out_writes_(BufferSize) PWSTR Buffer,
    _In_ SIZE_T BufferSize
);

// Extract directory path from file path
NTSTATUS
NTAPI
DslsfsExtractDirectoryPath(
    _In_ PCWSTR FilePath,
    _Out_writes_(BufferSize) PWSTR Buffer,
    _In_ SIZE_T BufferSize
);

#endif // _DSLSFS_H_