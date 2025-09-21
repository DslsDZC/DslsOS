/**
 * @file dslos.h
 * @brief Main header file for DslsOS
 * @author DslsOS Team
 * @version 1.0
 * @date 2024
 */

#ifndef DSLOS_H
#define DSLOS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Platform detection
#ifdef _WIN64
#define DSLOS_PLATFORM_WINDOWS 1
#define DSLOS_ARCH_X64 1
#elif defined(_WIN32)
#define DSLOS_PLATFORM_WINDOWS 1
#define DSLOS_ARCH_X86 1
#elif defined(__linux__)
#define DSLOS_PLATFORM_LINUX 1
#if defined(__x86_64__)
#define DSLOS_ARCH_X64 1
#elif defined(__i386__)
#define DSLOS_ARCH_X86 1
#elif defined(__aarch64__)
#define DSLOS_ARCH_ARM64 1
#elif defined(__arm__)
#define DSLOS_ARCH_ARM 1
#endif
#endif

// Version information
#define DSLOS_VERSION_MAJOR 1
#define DSLOS_VERSION_MINOR 0
#define DSLOS_VERSION_PATCH 0
#define DSLOS_VERSION_STRING "1.0.0"

// Export macros
#ifdef _WIN32
#define DSLOS_EXPORT __declspec(dllexport)
#define DSLOS_IMPORT __declspec(dllimport)
#else
#define DSLOS_EXPORT __attribute__((visibility("default")))
#define DSLOS_IMPORT __attribute__((visibility("default")))
#endif

#ifdef DSLOS_BUILD_DLL
#define DSLOS_API DSLOS_EXPORT
#else
#define DSLOS_API DSLOS_IMPORT
#endif

// Basic types
typedef uint8_t     UINT8;
typedef uint16_t    UINT16;
typedef uint32_t    UINT32;
typedef uint64_t    UINT64;
typedef int8_t      INT8;
typedef int16_t     INT16;
typedef int32_t     INT32;
typedef int64_t     INT64;
typedef uintptr_t   UINT_PTR;
typedef intptr_t    INT_PTR;
typedef size_t      SIZE_T;
typedef ptrdiff_t   SSIZE_T;

// Status codes
typedef UINT32 NTSTATUS;
#define STATUS_SUCCESS                   0x00000000
#define STATUS_UNSUCCESSFUL             0xC0000001
#define STATUS_INVALID_PARAMETER        0xC000000D
#define STATUS_ACCESS_DENIED            0xC0000022
#define STATUS_INSUFFICIENT_RESOURCES   0xC000009A
#define STATUS_NOT_IMPLEMENTED          0xC0000002
#define STATUS_DEVICE_NOT_READY         0xC00000A3
#define STATUS_IO_DEVICE_ERROR          0xC0000185
#define STATUS_DEVICE_NOT_CONNECTED     0xC000009D

// Handle types
typedef void* HANDLE;
typedef void* PVOID;
typedef const void* PCVOID;
typedef HANDLE* PHANDLE;
typedef PVOID* PPVOID;

// String types
typedef char CHAR;
typedef wchar_t WCHAR;
typedef CHAR* PCHAR;
typedef WCHAR* PWCHAR;
typedef const CHAR* PCCHAR;
typedef const WCHAR* PCWCHAR;
typedef PCHAR PSTR;
typedef PWCHAR PWSTR;
typedef PCCHAR PCSTR;
typedef PCWCHAR PCWSTR;

// Unicode strings
typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;
typedef const UNICODE_STRING* PCUNICODE_STRING;

// Object attributes
typedef struct _OBJECT_ATTRIBUTES {
    ULONG Length;
    HANDLE RootDirectory;
    PUNICODE_STRING ObjectName;
    ULONG Attributes;
    PVOID SecurityDescriptor;
    PVOID SecurityQualityOfService;
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;
typedef const OBJECT_ATTRIBUTES* PCOBJECT_ATTRIBUTES;

// Large integer
typedef union _LARGE_INTEGER {
    struct {
        ULONG LowPart;
        LONG HighPart;
    };
    struct {
        ULONG LowPart;
        LONG HighPart;
    } u;
    LONGLONG QuadPart;
} LARGE_INTEGER, *PLARGE_INTEGER;

// Client ID
typedef struct _CLIENT_ID {
    HANDLE UniqueProcess;
    HANDLE UniqueThread;
} CLIENT_ID, *PCLIENT_ID;

// Basic definitions
#define TRUE  1
#define FALSE 0
#define NULL  ((void*)0)

// Memory alignment
#define DSLOS_ALIGN(x, a) (((x) + (a) - 1) & ~((a) - 1))
#define DSLOS_PAGE_SIZE 4096
#define DSLOS_PAGE_MASK (DSLOS_PAGE_SIZE - 1)

// Array size macro
#define DSLOS_ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

// Min/Max macros
#define DSLOS_MIN(a, b) (((a) < (b)) ? (a) : (b))
#define DSLOS_MAX(a, b) (((a) > (b)) ? (a) : (b))

// Container macros
#define DSLOS_CONTAINER_OF(ptr, type, member) \
    ((type*)((char*)(ptr) - offsetof(type, member)))

// Forward declarations
struct _PROCESS_CONTROL_BLOCK;
struct _THREAD_CONTROL_BLOCK;
struct _KERNEL_OBJECT;

// Type definitions
typedef struct _PROCESS_CONTROL_BLOCK PROCESS_CONTROL_BLOCK, *PPROCESS_CONTROL_BLOCK;
typedef struct _THREAD_CONTROL_BLOCK THREAD_CONTROL_BLOCK, *PTHREAD_CONTROL_BLOCK;
typedef struct _KERNEL_OBJECT KERNEL_OBJECT, *PKERNEL_OBJECT;

// Process and Thread IDs
typedef HANDLE PROCESS_ID;
typedef HANDLE THREAD_ID;

// Access masks
typedef ULONG ACCESS_MASK;

// System information
typedef struct _SYSTEM_INFO {
    ULONG dwOemId;
    ULONG dwPageSize;
    PVOID lpMinimumApplicationAddress;
    PVOID lpMaximumApplicationAddress;
    ULONG_PTR dwActiveProcessorMask;
    ULONG dwNumberOfProcessors;
    ULONG dwProcessorType;
    ULONG dwAllocationGranularity;
    USHORT wProcessorLevel;
    USHORT wProcessorRevision;
} SYSTEM_INFO, *PSYSTEM_INFO;

// Function declarations
DSLOS_API NTSTATUS DslsInitializeSystem(VOID);
DSLOS_API NTSTATUS DslsShutdownSystem(ULONG Flags);
DSLOS_API VOID DslsGetSystemInfo(PSYSTEM_INFO SystemInfo);
DSLOS_API ULONG DslsGetLastError(VOID);
DSLOS_API VOID DslsSetLastError(ULONG Error);

#endif // DSLOS_H