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

#pragma pack(push,4)

#ifdef __cplusplus
extern "C" {
#endif

#define MINIDUMP_SIGNATURE 0x504D444D /* 'MDMP' */
#define MINIDUMP_VERSION   (42899)

typedef DWORD   RVA;
typedef ULONG64 RVA64;
#define RVA_TO_ADDR(map,rva) ((void *)((ULONG_PTR)(map) + (rva)))

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

    MiniDumpWithoutAuxiliaryState          = 0x00004000,
    MiniDumpWithFullAuxiliaryState         = 0x00008000,
    MiniDumpWithPrivateWriteCopyMemory     = 0x00010000,
    MiniDumpIgnoreInaccessibleMemory       = 0x00020000,
    MiniDumpWithTokenInformation           = 0x00040000,
    MiniDumpWithModuleHeaders              = 0x00080000,
    MiniDumpFilterTriage                   = 0x00100000,
    MiniDumpWithAvxXStateContext           = 0x00200000,
    MiniDumpWithIptTrace                   = 0x00400000,
    MiniDumpValidTypeFlags                 = 0x007fffff,
} MINIDUMP_TYPE;

typedef enum _MINIDUMP_SECONDARY_FLAGS
{
    MiniSecondaryWithoutPowerInfo          = 0x00000001,
    MiniSecondaryValidFlags                = 0x00000001,
} MINIDUMP_SECONDARY_FLAGS;

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

typedef struct _MINIDUMP_EXCEPTION_INFORMATION64
{
    DWORD ThreadId;
    ULONG64 ExceptionRecord;
    ULONG64 ContextRecord;
    BOOL ClientPointers;
} MINIDUMP_EXCEPTION_INFORMATION64, *PMINIDUMP_EXCEPTION_INFORMATION64;

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

typedef struct _XSTATE_CONFIG_FEATURE_MSC_INFO
{
    ULONG32 SizeOfInfo;
    ULONG32 ContextSize;
    ULONG64 EnabledFeatures;
    XSTATE_FEATURE Features[MAXIMUM_XSTATE_FEATURES];
} XSTATE_CONFIG_FEATURE_MSC_INFO, *PXSTATE_CONFIG_FEATURE_MSC_INFO;

#define MINIDUMP_MISC1_PROCESS_ID               0x00000001
#define MINIDUMP_MISC1_PROCESS_TIMES            0x00000002
#define MINIDUMP_MISC1_PROCESSOR_POWER_INFO     0x00000004
#define MINIDUMP_MISC3_PROCESS_INTEGRITY        0x00000010
#define MINIDUMP_MISC3_PROCESS_EXECUTE_FLAGS    0x00000020
#define MINIDUMP_MISC3_TIMEZONE                 0x00000040
#define MINIDUMP_MISC3_PROTECTED_PROCESS        0x00000080
#define MINIDUMP_MISC4_BUILDSTRING              0x00000100
#define MINIDUMP_MISC5_PROCESS_COOKIE           0x00000200

typedef struct _MINIDUMP_MISC_INFO
{
    ULONG32 SizeOfInfo;
    ULONG32 Flags1;
    ULONG32 ProcessId;
    ULONG32 ProcessCreateTime;
    ULONG32 ProcessUserTime;
    ULONG32 ProcessKernelTime;
} MINIDUMP_MISC_INFO, *PMINIDUMP_MISC_INFO;

typedef struct _MINIDUMP_MISC_INFO_2
{
    ULONG32 SizeOfInfo;
    ULONG32 Flags1;
    ULONG32 ProcessId;
    ULONG32 ProcessCreateTime;
    ULONG32 ProcessUserTime;
    ULONG32 ProcessKernelTime;
    ULONG32 ProcessorMaxMhz;
    ULONG32 ProcessorCurrentMhz;
    ULONG32 ProcessorMhzLimit;
    ULONG32 ProcessorMaxIdleState;
    ULONG32 ProcessorCurrentIdleState;
} MINIDUMP_MISC_INFO_2, *PMINIDUMP_MISC_INFO_2;

typedef struct _MINIDUMP_MISC_INFO_3
{
    ULONG32 SizeOfInfo;
    ULONG32 Flags1;
    ULONG32 ProcessId;
    ULONG32 ProcessCreateTime;
    ULONG32 ProcessUserTime;
    ULONG32 ProcessKernelTime;
    ULONG32 ProcessorMaxMhz;
    ULONG32 ProcessorCurrentMhz;
    ULONG32 ProcessorMhzLimit;
    ULONG32 ProcessorMaxIdleState;
    ULONG32 ProcessorCurrentIdleState;
    ULONG32 ProcessIntegrityLevel;
    ULONG32 ProcessExecuteFlags;
    ULONG32 ProtectedProcess;
    ULONG32 TimeZoneId;
    TIME_ZONE_INFORMATION TimeZone;
} MINIDUMP_MISC_INFO_3, *PMINIDUMP_MISC_INFO_3;

typedef struct _MINIDUMP_MISC_INFO_4
{
    ULONG32 SizeOfInfo;
    ULONG32 Flags1;
    ULONG32 ProcessId;
    ULONG32 ProcessCreateTime;
    ULONG32 ProcessUserTime;
    ULONG32 ProcessKernelTime;
    ULONG32 ProcessorMaxMhz;
    ULONG32 ProcessorCurrentMhz;
    ULONG32 ProcessorMhzLimit;
    ULONG32 ProcessorMaxIdleState;
    ULONG32 ProcessorCurrentIdleState;
    ULONG32 ProcessIntegrityLevel;
    ULONG32 ProcessExecuteFlags;
    ULONG32 ProtectedProcess;
    ULONG32 TimeZoneId;
    TIME_ZONE_INFORMATION TimeZone;
    WCHAR   BuildString[MAX_PATH];
    WCHAR   DbgBldStr[40];
} MINIDUMP_MISC_INFO_4, *PMINIDUMP_MISC_INFO_4;

typedef struct _MINIDUMP_MISC_INFO_5
{
    ULONG32 SizeOfInfo;
    ULONG32 Flags1;
    ULONG32 ProcessId;
    ULONG32 ProcessCreateTime;
    ULONG32 ProcessUserTime;
    ULONG32 ProcessKernelTime;
    ULONG32 ProcessorMaxMhz;
    ULONG32 ProcessorCurrentMhz;
    ULONG32 ProcessorMhzLimit;
    ULONG32 ProcessorMaxIdleState;
    ULONG32 ProcessorCurrentIdleState;
    ULONG32 ProcessIntegrityLevel;
    ULONG32 ProcessExecuteFlags;
    ULONG32 ProtectedProcess;
    ULONG32 TimeZoneId;
    TIME_ZONE_INFORMATION TimeZone;
    WCHAR   BuildString[MAX_PATH];
    WCHAR   DbgBldStr[40];
    XSTATE_CONFIG_FEATURE_MSC_INFO XStateData;
    ULONG32 ProcessCookie;
} MINIDUMP_MISC_INFO_5, *PMINIDUMP_MISC_INFO_5;

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
    ULONG32 UserStreamCount;
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

typedef struct _MINIDUMP_HANDLE_OBJECT_INFORMATION
{
    RVA     NextInfoRva;
    ULONG32 InfoType;
    ULONG32 SizeOfInfo;
} MINIDUMP_HANDLE_OBJECT_INFORMATION;

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

typedef struct _MINIDUMP_FUNCTION_TABLE_DESCRIPTOR
{
    ULONG64 MinimumAddress;
    ULONG64 MaximumAddress;
    ULONG64 BaseAddress;
    ULONG32 EntryCount;
    ULONG32 SizeOfAlignPad;
} MINIDUMP_FUNCTION_TABLE_DESCRIPTOR, *PMINIDUMP_FUNCTION_TABLE_DESCRIPTOR;

typedef struct _MINIDUMP_FUNCTION_TABLE_STREAM
{
    ULONG32 SizeOfHeader;
    ULONG32 SizeOfDescriptor;
    ULONG32 SizeOfNativeDescriptor;
    ULONG32 SizeOfFunctionEntry;
    ULONG32 NumberOfDescriptors;
    ULONG32 SizeOfAlignPad;
} MINIDUMP_FUNCTION_TABLE_STREAM, *PMINIDUMP_FUNCTION_TABLE_STREAM;

typedef struct _MINIDUMP_MEMORY_INFO
{
    ULONG64 BaseAddress;
    ULONG64 AllocationBase;
    ULONG32 AllocationProtect;
    ULONG32 __alignment1;
    ULONG64 RegionSize;
    ULONG32 State;
    ULONG32 Protect;
    ULONG32 Type;
    ULONG32 __alignment2;
} MINIDUMP_MEMORY_INFO, *PMINIDUMP_MEMORY_INFO;

typedef struct _MINIDUMP_MEMORY_INFO_LIST
{
    ULONG SizeOfHeader;
    ULONG SizeOfEntry;
    ULONG64 NumberOfEntries;
} MINIDUMP_MEMORY_INFO_LIST, *PMINIDUMP_MEMORY_INFO_LIST;

typedef struct _MINIDUMP_THREAD_NAME
{
    ULONG ThreadId;
    RVA64 RvaOfThreadName;
} MINIDUMP_THREAD_NAME, *PMINIDUMP_THREAD_NAME;

typedef struct _MINIDUMP_THREAD_NAME_LIST
{
    ULONG NumberOfThreadNames;
    MINIDUMP_THREAD_NAME ThreadNames[1]; /* FIXME: 0-sized array not supported */
} MINIDUMP_THREAD_NAME_LIST, *PMINIDUMP_THREAD_NAME_LIST;

typedef struct _MINIDUMP_TOKEN_INFO_HEADER
{
    ULONG32 TokenSize;
    ULONG32 TokenId;
    ULONG64 TokenHandle;
} MINIDUMP_TOKEN_INFO_HEADER, *PMINIDUMP_TOKEN_INFO_HEADER;

typedef struct _MINIDUMP_TOKEN_INFO_LIST
{
    ULONG32 TokenListSize;
    ULONG32 TokenListEntries;
    ULONG32 ListHeaderSize;
    ULONG32 ElementHeaderSize;
} MINIDUMP_TOKEN_INFO_LIST, *PMINIDUMP_TOKEN_INFO_LIST;

typedef struct _MINIDUMP_PROCESS_VM_COUNTERS_1
{
    USHORT Revision;
    ULONG PageFaultCount;
    ULONG64 PeakWorkingSetSize;
    ULONG64 WorkingSetSize;
    ULONG64 QuotaPeakPagedPoolUsage;
    ULONG64 QuotaPagedPoolUsage;
    ULONG64 QuotaPeakNonPagedPoolUsage;
    ULONG64 QuotaNonPagedPoolUsage;
    ULONG64 PagefileUsage;
    ULONG64 PeakPagefileUsage;
    ULONG64 PrivateUsage;
} MINIDUMP_PROCESS_VM_COUNTERS_1, *PMINIDUMP_PROCESS_VM_COUNTERS_1;

#define MINIDUMP_PROCESS_VM_COUNTERS                0x0001
#define MINIDUMP_PROCESS_VM_COUNTERS_VIRTUALSIZE    0x0002
#define MINIDUMP_PROCESS_VM_COUNTERS_EX             0x0004
#define MINIDUMP_PROCESS_VM_COUNTERS_EX2            0x0008
#define MINIDUMP_PROCESS_VM_COUNTERS_JOB            0x0010

typedef struct _MINIDUMP_PROCESS_VM_COUNTERS_2
{
    USHORT Revision;
    USHORT Flags;
    ULONG PageFaultCount;
    ULONG64 PeakWorkingSetSize;
    ULONG64 WorkingSetSize;
    ULONG64 QuotaPeakPagedPoolUsage;
    ULONG64 QuotaPagedPoolUsage;
    ULONG64 QuotaPeakNonPagedPoolUsage;
    ULONG64 QuotaNonPagedPoolUsage;
    ULONG64 PagefileUsage;
    ULONG64 PeakPagefileUsage;
    ULONG64 PeakVirtualSize;
    ULONG64 VirtualSize;
    ULONG64 PrivateUsage;
    ULONG64 PrivateWorkingSetSize;
    ULONG64 SharedCommitUsage;

    ULONG64 JobSharedCommitUsage;
    ULONG64 JobPrivateCommitUsage;
    ULONG64 JobPeakPrivateCommitUsage;
    ULONG64 JobPrivateCommitLimit;
    ULONG64 JobTotalCommitLimit;
} MINIDUMP_PROCESS_VM_COUNTERS_2, *PMINIDUMP_PROCESS_VM_COUNTERS_2;

typedef struct _MINIDUMP_SYSTEM_BASIC_INFORMATION
{
    ULONG TimerResolution;
    ULONG PageSize;
    ULONG NumberOfPhysicalPages;
    ULONG LowestPhysicalPageNumber;
    ULONG HighestPhysicalPageNumber;
    ULONG AllocationGranularity;
    ULONG64 MinimumUserModeAddress;
    ULONG64 MaximumUserModeAddress;
    ULONG64 ActiveProcessorsAffinityMask;
    ULONG NumberOfProcessors;
} MINIDUMP_SYSTEM_BASIC_INFORMATION, *PMINIDUMP_SYSTEM_BASIC_INFORMATION;

typedef struct _MINIDUMP_SYSTEM_FILECACHE_INFORMATION
{
    ULONG64 CurrentSize;
    ULONG64 PeakSize;
    ULONG PageFaultCount;
    ULONG64 MinimumWorkingSet;
    ULONG64 MaximumWorkingSet;
    ULONG64 CurrentSizeIncludingTransitionInPages;
    ULONG64 PeakSizeIncludingTransitionInPages;
    ULONG TransitionRePurposeCount;
    ULONG Flags;
} MINIDUMP_SYSTEM_FILECACHE_INFORMATION, *PMINIDUMP_SYSTEM_FILECACHE_INFORMATION;

typedef struct _MINIDUMP_SYSTEM_BASIC_PERFORMANCE_INFORMATION
{
    ULONG64 AvailablePages;
    ULONG64 CommittedPages;
    ULONG64 CommitLimit;
    ULONG64 PeakCommitment;
} MINIDUMP_SYSTEM_BASIC_PERFORMANCE_INFORMATION, *PMINIDUMP_SYSTEM_BASIC_PERFORMANCE_INFORMATION;

typedef struct _MINIDUMP_SYSTEM_PERFORMANCE_INFORMATION {
    ULONG64 IdleProcessTime;
    ULONG64 IoReadTransferCount;
    ULONG64 IoWriteTransferCount;
    ULONG64 IoOtherTransferCount;
    ULONG IoReadOperationCount;
    ULONG IoWriteOperationCount;
    ULONG IoOtherOperationCount;
    ULONG AvailablePages;
    ULONG CommittedPages;
    ULONG CommitLimit;
    ULONG PeakCommitment;
    ULONG PageFaultCount;
    ULONG CopyOnWriteCount;
    ULONG TransitionCount;
    ULONG CacheTransitionCount;
    ULONG DemandZeroCount;
    ULONG PageReadCount;
    ULONG PageReadIoCount;
    ULONG CacheReadCount;
    ULONG CacheIoCount;
    ULONG DirtyPagesWriteCount;
    ULONG DirtyWriteIoCount;
    ULONG MappedPagesWriteCount;
    ULONG MappedWriteIoCount;
    ULONG PagedPoolPages;
    ULONG NonPagedPoolPages;
    ULONG PagedPoolAllocs;
    ULONG PagedPoolFrees;
    ULONG NonPagedPoolAllocs;
    ULONG NonPagedPoolFrees;
    ULONG FreeSystemPtes;
    ULONG ResidentSystemCodePage;
    ULONG TotalSystemDriverPages;
    ULONG TotalSystemCodePages;
    ULONG NonPagedPoolLookasideHits;
    ULONG PagedPoolLookasideHits;
    ULONG AvailablePagedPoolPages;
    ULONG ResidentSystemCachePage;
    ULONG ResidentPagedPoolPage;
    ULONG ResidentSystemDriverPage;
    ULONG CcFastReadNoWait;
    ULONG CcFastReadWait;
    ULONG CcFastReadResourceMiss;
    ULONG CcFastReadNotPossible;
    ULONG CcFastMdlReadNoWait;
    ULONG CcFastMdlReadWait;
    ULONG CcFastMdlReadResourceMiss;
    ULONG CcFastMdlReadNotPossible;
    ULONG CcMapDataNoWait;
    ULONG CcMapDataWait;
    ULONG CcMapDataNoWaitMiss;
    ULONG CcMapDataWaitMiss;
    ULONG CcPinMappedDataCount;
    ULONG CcPinReadNoWait;
    ULONG CcPinReadWait;
    ULONG CcPinReadNoWaitMiss;
    ULONG CcPinReadWaitMiss;
    ULONG CcCopyReadNoWait;
    ULONG CcCopyReadWait;
    ULONG CcCopyReadNoWaitMiss;
    ULONG CcCopyReadWaitMiss;
    ULONG CcMdlReadNoWait;
    ULONG CcMdlReadWait;
    ULONG CcMdlReadNoWaitMiss;
    ULONG CcMdlReadWaitMiss;
    ULONG CcReadAheadIos;
    ULONG CcLazyWriteIos;
    ULONG CcLazyWritePages;
    ULONG CcDataFlushes;
    ULONG CcDataPages;
    ULONG ContextSwitches;
    ULONG FirstLevelTbFills;
    ULONG SecondLevelTbFills;
    ULONG SystemCalls;

    ULONG64 CcTotalDirtyPages;
    ULONG64 CcDirtyPageThreshold;

    LONG64 ResidentAvailablePages;
    ULONG64 SharedCommittedPages;
} MINIDUMP_SYSTEM_PERFORMANCE_INFORMATION, *PMINIDUMP_SYSTEM_PERFORMANCE_INFORMATION;

#define MINIDUMP_SYSMEMINFO1_FILECACHE_TRANSITIONREPURPOSECOUNT_FLAGS      0x0001
#define MINIDUMP_SYSMEMINFO1_BASICPERF                                     0x0002
#define MINIDUMP_SYSMEMINFO1_PERF_CCTOTALDIRTYPAGES_CCDIRTYPAGETHRESHOLD   0x0004
#define MINIDUMP_SYSMEMINFO1_PERF_RESIDENTAVAILABLEPAGES_SHAREDCOMMITPAGES 0x0008

typedef struct _MINIDUMP_SYSTEM_MEMORY_INFO_1
{
    USHORT Revision;
    USHORT Flags;

    MINIDUMP_SYSTEM_BASIC_INFORMATION BasicInfo;
    MINIDUMP_SYSTEM_FILECACHE_INFORMATION FileCacheInfo;
    MINIDUMP_SYSTEM_BASIC_PERFORMANCE_INFORMATION BasicPerfInfo;
    MINIDUMP_SYSTEM_PERFORMANCE_INFORMATION PerfInfo;
} MINIDUMP_SYSTEM_MEMORY_INFO_1, *PMINIDUMP_SYSTEM_MEMORY_INFO_1;

typedef MINIDUMP_SYSTEM_MEMORY_INFO_1 MINIDUMP_SYSTEM_MEMORY_INFO_N;
typedef MINIDUMP_SYSTEM_MEMORY_INFO_N *PMINIDUMP_SYSTEM_MEMORY_INFO_N;

typedef enum _MINIDUMP_CALLBACK_TYPE
{
    ModuleCallback,
    ThreadCallback,
    ThreadExCallback,
    IncludeThreadCallback,
    IncludeModuleCallback,
    MemoryCallback,
    CancelCallback,
    WriteKernelMinidumpCallback,
    KernelMinidumpStatusCallback,
    RemoveMemoryCallback,
    IncludeVmRegionCallback,
    IoStartCallback,
    IoWriteAllCallback,
    IoFinishCallback,
    ReadMemoryFailureCallback,
    SecondaryFlagsCallback,
    IsProcessSnapshotCallback,
    VmStartCallback,
    VmQueryCallback,
    VmPreReadCallback,
    VmPostReadCallback,
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

typedef struct _MINIDUMP_IO_CALLBACK
{
    HANDLE                      Handle;
    ULONG64                     Offset;
    PVOID                       Buffer;
    ULONG                       BufferBytes;
} MINIDUMP_IO_CALLBACK, *PMINIDUMP_IO_CALLBACK;

typedef struct _MINIDUMP_READ_MEMORY_FAILURE_CALLBACK
{
    ULONG64                     Offset;
    ULONG                       Bytes;
    HRESULT                     FailureStatus;
} MINIDUMP_READ_MEMORY_FAILURE_CALLBACK, *PMINIDUMP_READ_MEMORY_FAILURE_CALLBACK;

typedef struct _MINIDUMP_VM_QUERY_CALLBACK
{
    ULONG64                     Offset;
} MINIDUMP_VM_QUERY_CALLBACK, *PMINIDUMP_VM_QUERY_CALLBACK;

typedef struct _MINIDUMP_VM_PRE_READ_CALLBACK
{
    ULONG64                     Offset;
    PVOID                       Buffer;
    ULONG                       Size;
} MINIDUMP_VM_PRE_READ_CALLBACK, *PMINIDUMP_VM_PRE_READ_CALLBACK;

typedef struct _MINIDUMP_VM_POST_READ_CALLBACK
{
    ULONG64                     Offset;
    PVOID                       Buffer;
    ULONG                       Size;
    ULONG                       Completed;
    HRESULT                     Status;
} MINIDUMP_VM_POST_READ_CALLBACK, *PMINIDUMP_VM_POST_READ_CALLBACK;

typedef struct _MINIDUMP_CALLBACK_INPUT
{
    ULONG                       ProcessId;
    HANDLE                      ProcessHandle;
    ULONG                       CallbackType;
    union
    {
        HRESULT                                 Status;
        MINIDUMP_THREAD_CALLBACK                Thread;
        MINIDUMP_THREAD_EX_CALLBACK             ThreadEx;
        MINIDUMP_MODULE_CALLBACK                Module;
        MINIDUMP_INCLUDE_THREAD_CALLBACK        IncludeThread;
        MINIDUMP_INCLUDE_MODULE_CALLBACK        IncludeModule;
        MINIDUMP_IO_CALLBACK                    Io;
        MINIDUMP_READ_MEMORY_FAILURE_CALLBACK   ReadMemoryFailure;
        ULONG                                   SecondaryFlags;
        MINIDUMP_VM_QUERY_CALLBACK              VmQuery;
        MINIDUMP_VM_PRE_READ_CALLBACK           VmPreRead;
        MINIDUMP_VM_POST_READ_CALLBACK          VmPostRead;
    };
} MINIDUMP_CALLBACK_INPUT, *PMINIDUMP_CALLBACK_INPUT;

typedef struct _MINIDUMP_CALLBACK_OUTPUT
{
    union
    {
        ULONG                   ModuleWriteFlags;
        ULONG                   ThreadWriteFlags;
        ULONG                   SecondaryFlags;
        struct
        {
            ULONG64             MemoryBase;
            ULONG               MemorySize;
        };
        struct
        {
            BOOL                CheckCancel;
            BOOL                Cancel;
        };
        HANDLE Handle;
        struct
        {
            MINIDUMP_MEMORY_INFO VmRegion;
            BOOL                 Continue;
        };
        struct
        {
            HRESULT              VmQueryStatus;
            MINIDUMP_MEMORY_INFO VmQueryResult;
        };
        struct
        {
            HRESULT             VmReadStatus;
            ULONG               VmReadBytesCompleted;
        };
        HRESULT                 Status;
    };
} MINIDUMP_CALLBACK_OUTPUT, *PMINIDUMP_CALLBACK_OUTPUT;

typedef BOOL (WINAPI* MINIDUMP_CALLBACK_ROUTINE)(PVOID, const PMINIDUMP_CALLBACK_INPUT, PMINIDUMP_CALLBACK_OUTPUT);

typedef struct _MINIDUMP_CALLBACK_INFORMATION
{
    MINIDUMP_CALLBACK_ROUTINE CallbackRoutine;
    void *CallbackParam;
} MINIDUMP_CALLBACK_INFORMATION, *PMINIDUMP_CALLBACK_INFORMATION;

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

#pragma pack(pop)

#endif  /* __WINE_MINIDUMP_H */
