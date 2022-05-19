/*
 * Copyright 2022 Nikolay Sivov for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_MINIDUMP_H
#define __WINE_MINIDUMP_H

#include <pshpack4.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MINIDUMP_SIGNATURE 0x504D444D /* 'MDMP' */
#define MINIDUMP_VERSION   (42899)

typedef DWORD   RVA;
typedef ULONG64 RVA64;

typedef enum _MINIDUMP_TYPE
{
    MiniDumpNormal                         = 0x00000000,
    MiniDumpWithDataSegs                   = 0x00000001,
    MiniDumpWithFullMemory                 = 0x00000002,
    MiniDumpWithHandleData                 = 0x00000004,
    MiniDumpFilterMemory                   = 0x00000008,
    MiniDumpScanMemory                     = 0x00000010,
    MiniDumpWithUnloadedModules            = 0x00000020,
    MiniDumpWithIndirectlyReferencedMemory = 0x00000040,
    MiniDumpFilterModulePaths              = 0x00000080,
    MiniDumpWithProcessThreadData          = 0x00000100,
    MiniDumpWithPrivateReadWriteMemory     = 0x00000200,
    MiniDumpWithoutOptionalData            = 0x00000400,
    MiniDumpWithFullMemoryInfo             = 0x00000800,
    MiniDumpWithThreadInfo                 = 0x00001000,
    MiniDumpWithCodeSegs                   = 0x00002000,
    MiniDumpValidTypeFlags                 = 0x01ffffff,
} MINIDUMP_TYPE;

typedef enum _MINIDUMP_CALLBACK_TYPE
{
    ModuleCallback,
    ThreadCallback,
    ThreadExCallback,
    IncludeThreadCallback,
    IncludeModuleCallback,
    MemoryCallback,
} MINIDUMP_CALLBACK_TYPE;

typedef struct _MINIDUMP_THREAD_CALLBACK
{
    ULONG                       ThreadId;
    HANDLE                      ThreadHandle;
#if defined(__aarch64__)
    ULONG                       Pad;
#endif
    CONTEXT                     Context;
    ULONG                       SizeOfContext;
    ULONG64                     StackBase;
    ULONG64                     StackEnd;
} MINIDUMP_THREAD_CALLBACK, *PMINIDUMP_THREAD_CALLBACK;

typedef struct _MINIDUMP_THREAD_EX_CALLBACK
{
    ULONG                       ThreadId;
    HANDLE                      ThreadHandle;
#if defined(__aarch64__)
    ULONG                       Pad;
#endif
    CONTEXT                     Context;
    ULONG                       SizeOfContext;
    ULONG64                     StackBase;
    ULONG64                     StackEnd;
    ULONG64                     BackingStoreBase;
    ULONG64                     BackingStoreEnd;
} MINIDUMP_THREAD_EX_CALLBACK, *PMINIDUMP_THREAD_EX_CALLBACK;

typedef struct _MINIDUMP_INCLUDE_THREAD_CALLBACK
{
    ULONG ThreadId;
} MINIDUMP_INCLUDE_THREAD_CALLBACK, *PMINIDUMP_INCLUDE_THREAD_CALLBACK;

typedef enum _THREAD_WRITE_FLAGS
{
    ThreadWriteThread            = 0x0001,
    ThreadWriteStack             = 0x0002,
    ThreadWriteContext           = 0x0004,
    ThreadWriteBackingStore      = 0x0008,
    ThreadWriteInstructionWindow = 0x0010,
    ThreadWriteThreadData        = 0x0020,
    ThreadWriteThreadInfo        = 0x0040
} THREAD_WRITE_FLAGS;

typedef struct _MINIDUMP_MODULE_CALLBACK
{
    PWCHAR                      FullPath;
    ULONG64                     BaseOfImage;
    ULONG                       SizeOfImage;
    ULONG                       CheckSum;
    ULONG                       TimeDateStamp;
    VS_FIXEDFILEINFO            VersionInfo;
    PVOID                       CvRecord;
    ULONG                       SizeOfCvRecord;
    PVOID                       MiscRecord;
    ULONG                       SizeOfMiscRecord;
} MINIDUMP_MODULE_CALLBACK, *PMINIDUMP_MODULE_CALLBACK;

typedef struct _MINIDUMP_INCLUDE_MODULE_CALLBACK
{
    ULONG64 BaseOfImage;
} MINIDUMP_INCLUDE_MODULE_CALLBACK, *PMINIDUMP_INCLUDE_MODULE_CALLBACK;

typedef enum _MODULE_WRITE_FLAGS
{
    ModuleWriteModule        = 0x0001,
    ModuleWriteDataSeg       = 0x0002,
    ModuleWriteMiscRecord    = 0x0004,
    ModuleWriteCvRecord      = 0x0008,
    ModuleReferencedByMemory = 0x0010,
    ModuleWriteTlsData       = 0x0020,
    ModuleWriteCodeSegs      = 0x0040,
} MODULE_WRITE_FLAGS;

typedef struct _MINIDUMP_CALLBACK_INPUT
{
    ULONG ProcessId;
    HANDLE ProcessHandle;
    ULONG CallbackType;
    union
    {
        MINIDUMP_THREAD_CALLBACK Thread;
        MINIDUMP_THREAD_EX_CALLBACK ThreadEx;
        MINIDUMP_MODULE_CALLBACK Module;
        MINIDUMP_INCLUDE_THREAD_CALLBACK IncludeThread;
        MINIDUMP_INCLUDE_MODULE_CALLBACK IncludeModule;
    } DUMMYUNIONNAME;
} MINIDUMP_CALLBACK_INPUT, *PMINIDUMP_CALLBACK_INPUT;

typedef struct _MINIDUMP_CALLBACK_OUTPUT
{
    union
    {
        ULONG ModuleWriteFlags;
        ULONG ThreadWriteFlags;
        struct
        {
            ULONG64 MemoryBase;
            ULONG MemorySize;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
} MINIDUMP_CALLBACK_OUTPUT, *PMINIDUMP_CALLBACK_OUTPUT;

typedef BOOL (WINAPI* MINIDUMP_CALLBACK_ROUTINE)(PVOID, const PMINIDUMP_CALLBACK_INPUT, PMINIDUMP_CALLBACK_OUTPUT);

typedef struct _MINIDUMP_CALLBACK_INFORMATION
{
    MINIDUMP_CALLBACK_ROUTINE CallbackRoutine;
    void *CallbackParam;
} MINIDUMP_CALLBACK_INFORMATION, *PMINIDUMP_CALLBACK_INFORMATION;

typedef struct _MINIDUMP_LOCATION_DESCRIPTOR
{
    ULONG32 DataSize;
    RVA Rva;
} MINIDUMP_LOCATION_DESCRIPTOR;

typedef struct _MINIDUMP_LOCATION_DESCRIPTOR64
{
    ULONG64 DataSize;
    RVA64 Rva;
} MINIDUMP_LOCATION_DESCRIPTOR64;

typedef struct _MINIDUMP_DIRECTORY
{
    ULONG32 StreamType;
    MINIDUMP_LOCATION_DESCRIPTOR Location;
} MINIDUMP_DIRECTORY, *PMINIDUMP_DIRECTORY;

typedef struct _MINIDUMP_EXCEPTION
{
    ULONG32 ExceptionCode;
    ULONG32 ExceptionFlags;
    ULONG64 ExceptionRecord;
    ULONG64 ExceptionAddress;
    ULONG32 NumberParameters;
    ULONG32 __unusedAlignment;
    ULONG64 ExceptionInformation[EXCEPTION_MAXIMUM_PARAMETERS];
} MINIDUMP_EXCEPTION, *PMINIDUMP_EXCEPTION;

typedef struct _MINIDUMP_EXCEPTION_INFORMATION
{
    DWORD ThreadId;
    PEXCEPTION_POINTERS ExceptionPointers;
    BOOL ClientPointers;
} MINIDUMP_EXCEPTION_INFORMATION, *PMINIDUMP_EXCEPTION_INFORMATION;

typedef struct MINIDUMP_EXCEPTION_STREAM
{
    ULONG32 ThreadId;
    ULONG32 __alignment;
    MINIDUMP_EXCEPTION ExceptionRecord;
    MINIDUMP_LOCATION_DESCRIPTOR ThreadContext;
} MINIDUMP_EXCEPTION_STREAM, *PMINIDUMP_EXCEPTION_STREAM;

typedef struct _MINIDUMP_HEADER
{
    ULONG32 Signature;
    ULONG32 Version;
    ULONG32 NumberOfStreams;
    RVA StreamDirectoryRva;
    ULONG32 CheckSum;
    union
    {
        ULONG32 Reserved;
        ULONG32 TimeDateStamp;
    } DUMMYUNIONNAME;
    ULONG64 Flags;
} MINIDUMP_HEADER, *PMINIDUMP_HEADER;

typedef struct _MINIDUMP_MEMORY_DESCRIPTOR
{
    ULONG64 StartOfMemoryRange;
    MINIDUMP_LOCATION_DESCRIPTOR Memory;
} MINIDUMP_MEMORY_DESCRIPTOR, *PMINIDUMP_MEMORY_DESCRIPTOR;

typedef struct _MINIDUMP_MEMORY_LIST
{
    ULONG32 NumberOfMemoryRanges;
    MINIDUMP_MEMORY_DESCRIPTOR  MemoryRanges[1]; /* FIXME: 0-sized array not supported */
} MINIDUMP_MEMORY_LIST, *PMINIDUMP_MEMORY_LIST;

typedef struct _MINIDUMP_MEMORY_DESCRIPTOR64
{
    ULONG64 StartOfMemoryRange;
    ULONG64 DataSize;
} MINIDUMP_MEMORY_DESCRIPTOR64, *PMINIDUMP_MEMORY_DESCRIPTOR64;

typedef struct _MINIDUMP_MEMORY64_LIST
{
    ULONG64 NumberOfMemoryRanges;
    RVA64 BaseRva;
    MINIDUMP_MEMORY_DESCRIPTOR64 MemoryRanges[1]; /* FIXME: 0-sized array not supported */
} MINIDUMP_MEMORY64_LIST, *PMINIDUMP_MEMORY64_LIST;

#define MINIDUMP_MISC1_PROCESS_ID       0x00000001
#define MINIDUMP_MISC1_PROCESS_TIMES    0x00000002

typedef struct _MINIDUMP_MISC_INFO
{
    ULONG32 SizeOfInfo;
    ULONG32 Flags1;
    ULONG32 ProcessId;
    ULONG32 ProcessCreateTime;
    ULONG32 ProcessUserTime;
    ULONG32 ProcessKernelTime;
} MINIDUMP_MISC_INFO, *PMINIDUMP_MISC_INFO;

typedef struct _MINIDUMP_MODULE
{
    ULONG64 BaseOfImage;
    ULONG32 SizeOfImage;
    ULONG32 CheckSum;
    ULONG32 TimeDateStamp;
    RVA ModuleNameRva;
    VS_FIXEDFILEINFO VersionInfo;
    MINIDUMP_LOCATION_DESCRIPTOR CvRecord;
    MINIDUMP_LOCATION_DESCRIPTOR MiscRecord;
    ULONG64 Reserved0;
    ULONG64 Reserved1;
} MINIDUMP_MODULE, *PMINIDUMP_MODULE;

typedef struct _MINIDUMP_MODULE_LIST
{
    ULONG32 NumberOfModules;
    MINIDUMP_MODULE Modules[1]; /* FIXME: 0-sized array not supported */
} MINIDUMP_MODULE_LIST, *PMINIDUMP_MODULE_LIST;

typedef struct _MINIDUMP_STRING
{
    ULONG32 Length;
    WCHAR Buffer[1]; /* FIXME: O-sized array not supported */
} MINIDUMP_STRING, *PMINIDUMP_STRING;

typedef union _CPU_INFORMATION
{
    struct
    {
        ULONG32 VendorId[3];
        ULONG32 VersionInformation;
        ULONG32 FeatureInformation;
        ULONG32 AMDExtendedCpuFeatures;
    } X86CpuInfo;
    struct
    {
        ULONG64 ProcessorFeatures[2];
    } OtherCpuInfo;
} CPU_INFORMATION, *PCPU_INFORMATION;

typedef struct _MINIDUMP_SYSTEM_INFO
{
    USHORT ProcessorArchitecture;
    USHORT ProcessorLevel;
    USHORT ProcessorRevision;
    union
    {
        USHORT Reserved0;
        struct
        {
            UCHAR NumberOfProcessors;
            UCHAR ProductType;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;

    ULONG32 MajorVersion;
    ULONG32 MinorVersion;
    ULONG32 BuildNumber;
    ULONG32 PlatformId;

    RVA CSDVersionRva;
    union
    {
        ULONG32 Reserved1;
        struct
        {
            USHORT SuiteMask;
            USHORT Reserved2;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME1;
    CPU_INFORMATION Cpu;
} MINIDUMP_SYSTEM_INFO, *PMINIDUMP_SYSTEM_INFO;

typedef struct _MINIDUMP_THREAD
{
    ULONG32 ThreadId;
    ULONG32 SuspendCount;
    ULONG32 PriorityClass;
    ULONG32 Priority;
    ULONG64 Teb;
    MINIDUMP_MEMORY_DESCRIPTOR Stack;
    MINIDUMP_LOCATION_DESCRIPTOR ThreadContext;
} MINIDUMP_THREAD, *PMINIDUMP_THREAD;

typedef struct _MINIDUMP_THREAD_LIST
{
    ULONG32 NumberOfThreads;
    MINIDUMP_THREAD Threads[1]; /* FIXME: no support of 0 sized array */
} MINIDUMP_THREAD_LIST, *PMINIDUMP_THREAD_LIST;

typedef struct _MINIDUMP_USER_STREAM
{
    ULONG32 Type;
    ULONG BufferSize;
    void *Buffer;
} MINIDUMP_USER_STREAM, *PMINIDUMP_USER_STREAM;

typedef struct _MINIDUMP_USER_STREAM_INFORMATION
{
    ULONG UserStreamCount;
    PMINIDUMP_USER_STREAM UserStreamArray;
} MINIDUMP_USER_STREAM_INFORMATION, *PMINIDUMP_USER_STREAM_INFORMATION;

typedef struct _MINIDUMP_HANDLE_DATA_STREAM
{
    ULONG32 SizeOfHeader;
    ULONG32 SizeOfDescriptor;
    ULONG32 NumberOfDescriptors;
    ULONG32 Reserved;
} MINIDUMP_HANDLE_DATA_STREAM, *PMINIDUMP_HANDLE_DATA_STREAM;

typedef struct _MINIDUMP_HANDLE_DESCRIPTOR
{
    ULONG64 Handle;
    RVA TypeNameRva;
    RVA ObjectNameRva;
    ULONG32 Attributes;
    ULONG32 GrantedAccess;
    ULONG32 HandleCount;
    ULONG32 PointerCount;
} MINIDUMP_HANDLE_DESCRIPTOR, *PMINIDUMP_HANDLE_DESCRIPTOR;

typedef struct _MINIDUMP_HANDLE_DESCRIPTOR_2
{
    ULONG64 Handle;
    RVA TypeNameRva;
    RVA ObjectNameRva;
    ULONG32 Attributes;
    ULONG32 GrantedAccess;
    ULONG32 HandleCount;
    ULONG32 PointerCount;
    RVA ObjectInfoRva;
    ULONG32 Reserved0;
} MINIDUMP_HANDLE_DESCRIPTOR_2, *PMINIDUMP_HANDLE_DESCRIPTOR_2;

typedef struct _MINIDUMP_THREAD_INFO
{
    ULONG32 ThreadId;
    ULONG32 DumpFlags;
    ULONG32 DumpError;
    ULONG32 ExitStatus;
    ULONG64 CreateTime;
    ULONG64 ExitTime;
    ULONG64 KernelTime;
    ULONG64 UserTime;
    ULONG64 StartAddress;
    ULONG64 Affinity;
} MINIDUMP_THREAD_INFO, *PMINIDUMP_THREAD_INFO;

typedef struct _MINIDUMP_THREAD_INFO_LIST
{
    ULONG SizeOfHeader;
    ULONG SizeOfEntry;
    ULONG NumberOfEntries;
} MINIDUMP_THREAD_INFO_LIST, *PMINIDUMP_THREAD_INFO_LIST;

typedef struct _MINIDUMP_UNLOADED_MODULE
{
    ULONG64 BaseOfImage;
    ULONG32 SizeOfImage;
    ULONG32 CheckSum;
    ULONG32 TimeDateStamp;
    RVA ModuleNameRva;
} MINIDUMP_UNLOADED_MODULE, *PMINIDUMP_UNLOADED_MODULE;

typedef struct _MINIDUMP_UNLOADED_MODULE_LIST
{
    ULONG32 SizeOfHeader;
    ULONG32 SizeOfEntry;
    ULONG32 NumberOfEntries;
} MINIDUMP_UNLOADED_MODULE_LIST, *PMINIDUMP_UNLOADED_MODULE_LIST;

typedef enum _MINIDUMP_STREAM_TYPE
{
    UnusedStream                = 0,
    ReservedStream0             = 1,
    ReservedStream1             = 2,
    ThreadListStream            = 3,
    ModuleListStream            = 4,
    MemoryListStream            = 5,
    ExceptionStream             = 6,
    SystemInfoStream            = 7,
    ThreadExListStream          = 8,
    Memory64ListStream          = 9,
    CommentStreamA              = 10,
    CommentStreamW              = 11,
    HandleDataStream            = 12,
    FunctionTableStream         = 13,
    UnloadedModuleListStream    = 14,
    MiscInfoStream              = 15,
    MemoryInfoListStream        = 16,
    ThreadInfoListStream        = 17,
    HandleOperationListStream   = 18,
    TokenStream                 = 19,
    JavaScriptDataStream        = 20,
    SystemMemoryInfoStream      = 21,
    ProcessVmCountersStream     = 22,
    IptTraceStream              = 23,
    ThreadNamesStream           = 24,

    LastReservedStream          = 0xffff
} MINIDUMP_STREAM_TYPE;

BOOL WINAPI MiniDumpWriteDump(HANDLE process, DWORD pid, HANDLE hfile, MINIDUMP_TYPE dumptype,
        PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
        PMINIDUMP_CALLBACK_INFORMATION CallbackParam);
BOOL WINAPI MiniDumpReadDumpStream(PVOID base, ULONG index, PMINIDUMP_DIRECTORY *dir, void **streamptr,
        ULONG *stream_size);

#ifdef __cplusplus
}
#endif

#include <poppack.h>

#endif  /* __WINE_MINIDUMP_H */
