/*
 * 32-bit version of ntdll structures
 *
 * Copyright 2021 Alexandre Julliard
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

#ifndef __WOW64_STRUCT32_H
#define __WOW64_STRUCT32_H

typedef struct
{
    ULONG Length;
    ULONG RootDirectory;
    ULONG ObjectName;
    ULONG Attributes;
    ULONG SecurityDescriptor;
    ULONG SecurityQualityOfService;
} OBJECT_ATTRIBUTES32;

typedef struct
{
    union
    {
        NTSTATUS Status;
        ULONG Pointer;
    };
    ULONG Information;
} IO_STATUS_BLOCK32;

typedef struct
{
    UNICODE_STRING32 Name;
} OBJECT_NAME_INFORMATION32;

typedef struct
{
    UNICODE_STRING32 TypeName;
    ULONG            TotalNumberOfObjects;
    ULONG            TotalNumberOfHandles;
    ULONG            TotalPagedPoolUsage;
    ULONG            TotalNonPagedPoolUsage;
    ULONG            TotalNamePoolUsage;
    ULONG            TotalHandleTableUsage;
    ULONG            HighWaterNumberOfObjects;
    ULONG            HighWaterNumberOfHandles;
    ULONG            HighWaterPagedPoolUsage;
    ULONG            HighWaterNonPagedPoolUsage;
    ULONG            HighWaterNamePoolUsage;
    ULONG            HighWaterHandleTableUsage;
    ULONG            InvalidAttributes;
    GENERIC_MAPPING  GenericMapping;
    ULONG            ValidAccessMask;
    BOOLEAN          SecurityRequired;
    BOOLEAN          MaintainHandleCount;
    UCHAR            TypeIndex;
    CHAR             ReservedByte;
    ULONG            PoolType;
    ULONG            DefaultPagedPoolCharge;
    ULONG            DefaultNonPagedPoolCharge;
} OBJECT_TYPE_INFORMATION32;

typedef struct
{
    WORD  Level;
    WORD  Sbz;
    ULONG ObjectType;
} OBJECT_TYPE_LIST32;

typedef struct
{
    CONTEXT_CHUNK All;
    CONTEXT_CHUNK Legacy;
    CONTEXT_CHUNK XState;
} CONTEXT_EX32;

typedef struct
{
    UNICODE_STRING32 ObjectName;
    UNICODE_STRING32 ObjectTypeName;
} DIRECTORY_BASIC_INFORMATION32;

typedef struct
{
    ULONG CompletionPort;
    ULONG CompletionKey;
} FILE_COMPLETION_INFORMATION32;

typedef struct
{
    ULONG             CompletionKey;
    ULONG             CompletionValue;
    IO_STATUS_BLOCK32 IoStatusBlock;
} FILE_IO_COMPLETION_INFORMATION32;

typedef struct
{
    union
    {
        BOOLEAN ReplaceIfExists;
        ULONG Flags;
    };
    ULONG   RootDirectory;
    ULONG   FileNameLength;
    WCHAR   FileName[1];
} FILE_RENAME_INFORMATION32;

typedef struct
{
    ULONG Mask;
    WORD  Group;
    WORD  Reserved[3];
} GROUP_AFFINITY32;

typedef struct
{
    DWORD NumberOfAssignedProcesses;
    DWORD NumberOfProcessIdsInList;
    ULONG ProcessIdList[1];
} JOBOBJECT_BASIC_PROCESS_ID_LIST32;

typedef struct
{
    LARGE_INTEGER PerProcessUserTimeLimit;
    LARGE_INTEGER PerJobUserTimeLimit;
    DWORD         LimitFlags;
    ULONG         MinimumWorkingSetSize;
    ULONG         MaximumWorkingSetSize;
    DWORD         ActiveProcessLimit;
    ULONG         Affinity;
    DWORD         PriorityClass;
    DWORD         SchedulingClass;
} JOBOBJECT_BASIC_LIMIT_INFORMATION32;

typedef struct
{
    JOBOBJECT_BASIC_LIMIT_INFORMATION32 BasicLimitInformation;
    IO_COUNTERS                         IoInfo;
    ULONG                               ProcessMemoryLimit;
    ULONG                               JobMemoryLimit;
    ULONG                               PeakProcessMemoryUsed;
    ULONG                               PeakJobMemoryUsed;
} JOBOBJECT_EXTENDED_LIMIT_INFORMATION32;

typedef struct
{
    ULONG CompletionKey;
    LONG  CompletionPort;
} JOBOBJECT_ASSOCIATE_COMPLETION_PORT32;

typedef struct
{
    ULONG    BaseAddress;
    ULONG    AllocationBase;
    DWORD    AllocationProtect;
    ULONG    RegionSize;
    DWORD    State;
    DWORD    Protect;
    DWORD    Type;
} MEMORY_BASIC_INFORMATION32;

typedef struct
{
    ULONG AllocationBase;
    ULONG AllocationProtect;
    ULONG RegionType;
    ULONG RegionSize;
    ULONG CommitSize;
    ULONG PartitionId;
    ULONG NodePreference;
} MEMORY_REGION_INFORMATION32;

typedef struct
{
    UNICODE_STRING32 SectionFileName;
} MEMORY_SECTION_NAME32;

typedef union
{
    ULONG Flags;
    struct {
        ULONG Valid : 1;
        ULONG ShareCount : 3;
        ULONG Win32Protection : 11;
        ULONG Shared : 1;
        ULONG Node : 6;
        ULONG Locked : 1;
        ULONG LargePage : 1;
    };
} MEMORY_WORKING_SET_EX_BLOCK32;

typedef struct
{
    ULONG                         VirtualAddress;
    MEMORY_WORKING_SET_EX_BLOCK32 VirtualAttributes;
} MEMORY_WORKING_SET_EX_INFORMATION32;

typedef struct
{
    ULONG ImageBase;
    ULONG SizeOfImage;
    union
    {
        ULONG ImageFlags;
        struct
        {
            ULONG ImagePartialMap : 1;
            ULONG ImageNotExecutable : 1;
            ULONG ImageSigningLevel : 4;
            ULONG Reserved : 26;
        };
    };
} MEMORY_IMAGE_INFORMATION32;

typedef struct
{
    NTSTATUS  ExitStatus;
    ULONG     PebBaseAddress;
    ULONG     AffinityMask;
    LONG      BasePriority;
    ULONG     UniqueProcessId;
    ULONG     InheritedFromUniqueProcessId;
} PROCESS_BASIC_INFORMATION32;

typedef struct
{
    ULONG ReserveSize;
    ULONG ZeroBits;
    ULONG StackBase;
} PROCESS_STACK_ALLOCATION_INFORMATION32;

typedef struct
{
    ULONG                                  PreferredNode;
    ULONG                                  Reserved0;
    ULONG                                  Reserved1;
    ULONG                                  Reserved2;
    PROCESS_STACK_ALLOCATION_INFORMATION32 AllocInfo;
} PROCESS_STACK_ALLOCATION_INFORMATION_EX32;

typedef struct
{
    ULONG Size;
    PS_CREATE_STATE State;
    union
    {
        struct
        {
            ULONG InitFlags;
            ACCESS_MASK AdditionalFileAccess;
        } InitState;
        struct
        {
            ULONG FileHandle;
        } FailSection;
        struct
        {
            USHORT DllCharacteristics;
        } ExeFormat;
        struct
        {
            ULONG IFEOKey;
        } ExeName;
        struct
        {
            ULONG OutputFlags;
            ULONG FileHandle;
            ULONG SectionHandle;
            ULONGLONG UserProcessParametersNative;
            ULONG UserProcessParametersWow64;
            ULONG CurrentParameterFlags;
            ULONGLONG PebAddressNative;
            ULONG PebAddressWow64;
            ULONGLONG ManifestAddress;
            ULONG ManifestSize;
        } SuccessState;
    };
} PS_CREATE_INFO32;

typedef struct
{
    ULONG Attribute;
    ULONG Size;
    ULONG Value;
    ULONG ReturnLength;
} PS_ATTRIBUTE32;

typedef struct
{
    ULONG TotalLength;
    PS_ATTRIBUTE32 Attributes[1];
} PS_ATTRIBUTE_LIST32;

typedef struct
{
    ULONG  Section;
    ULONG  MappedBaseAddress;
    ULONG  ImageBaseAddress;
    ULONG  ImageSize;
    ULONG  Flags;
    USHORT LoadOrderIndex;
    USHORT InitOrderIndex;
    USHORT LoadCount;
    USHORT NameOffset;
    BYTE   Name[MAXIMUM_FILENAME_LENGTH];
} RTL_PROCESS_MODULE_INFORMATION32;

typedef struct
{
    ULONG                            ModulesCount;
    RTL_PROCESS_MODULE_INFORMATION32 Modules[1];
} RTL_PROCESS_MODULES32;

typedef struct
{
    USHORT                           NextOffset;
    RTL_PROCESS_MODULE_INFORMATION32 BaseInfo;
    ULONG                            ImageCheckSum;
    ULONG                            TimeDateStamp;
    ULONG                            DefaultBase;
} RTL_PROCESS_MODULE_INFORMATION_EX32;

typedef struct
{
    ULONG         BaseAddress;
    ULONG         Attributes;
    LARGE_INTEGER Size;
} SECTION_BASIC_INFORMATION32;

typedef struct
{
    ULONG   TransferAddress;
    ULONG   ZeroBits;
    ULONG   MaximumStackSize;
    ULONG   CommittedStackSize;
    ULONG   SubSystemType;
    USHORT  MinorSubsystemVersion;
    USHORT  MajorSubsystemVersion;
    USHORT  MajorOperatingSystemVersion;
    USHORT  MinorOperatingSystemVersion;
    USHORT  ImageCharacteristics;
    USHORT  DllCharacteristics;
    USHORT  Machine;
    BOOLEAN ImageContainsCode;
    UCHAR   ImageFlags;
    ULONG   LoaderFlags;
    ULONG   ImageFileSize;
    ULONG   CheckSum;
} SECTION_IMAGE_INFORMATION32;

typedef struct
{
    ULONG Sid;
    DWORD Attributes;
} SID_AND_ATTRIBUTES32;

typedef struct
{
    SID_AND_ATTRIBUTES32 Label;
} TOKEN_MANDATORY_LABEL32;

typedef struct
{
    ULONG DefaultDacl;
} TOKEN_DEFAULT_DACL32;

typedef struct
{
    DWORD                GroupCount;
    SID_AND_ATTRIBUTES32 Groups[1];
} TOKEN_GROUPS32;

typedef struct
{
    ULONG Owner;
} TOKEN_OWNER32;

typedef struct
{
    ULONG PrimaryGroup;
} TOKEN_PRIMARY_GROUP32;

typedef struct
{
    SID_AND_ATTRIBUTES32 User;
} TOKEN_USER32;

typedef struct
{
    NTSTATUS    ExitStatus;
    ULONG       TebBaseAddress;
    CLIENT_ID32 ClientId;
    ULONG       AffinityMask;
    LONG        Priority;
    LONG        BasePriority;
} THREAD_BASIC_INFORMATION32;

typedef struct
{
    UNICODE_STRING32 ThreadName;
} THREAD_NAME_INFORMATION32;

typedef struct
{
    ULONG PeakVirtualSize;
    ULONG VirtualSize;
    ULONG PageFaultCount;
    ULONG PeakWorkingSetSize;
    ULONG WorkingSetSize;
    ULONG QuotaPeakPagedPoolUsage;
    ULONG QuotaPagedPoolUsage;
    ULONG QuotaPeakNonPagedPoolUsage;
    ULONG QuotaNonPagedPoolUsage;
    ULONG PagefileUsage;
    ULONG PeakPagefileUsage;
} VM_COUNTERS32;

typedef struct
{
    ULONG PeakVirtualSize;
    ULONG VirtualSize;
    ULONG PageFaultCount;
    ULONG PeakWorkingSetSize;
    ULONG WorkingSetSize;
    ULONG QuotaPeakPagedPoolUsage;
    ULONG QuotaPagedPoolUsage;
    ULONG QuotaPeakNonPagedPoolUsage;
    ULONG QuotaNonPagedPoolUsage;
    ULONG PagefileUsage;
    ULONG PeakPagefileUsage;
    ULONG PrivateUsage;
} VM_COUNTERS_EX32;

typedef struct
{
    DBG_STATE   NewState;
    CLIENT_ID32 AppClientId;
    union
    {
        struct
        {
            EXCEPTION_RECORD32 ExceptionRecord;
            ULONG FirstChance;
        } Exception;
        struct
        {
            ULONG HandleToThread;
            struct
            {
                ULONG SubSystemKey;
                ULONG StartAddress;
            } NewThread;
        } CreateThread;
        struct
        {
            ULONG HandleToProcess;
            ULONG HandleToThread;
            struct
            {
                ULONG SubSystemKey;
                ULONG FileHandle;
                ULONG BaseOfImage;
                ULONG DebugInfoFileOffset;
                ULONG DebugInfoSize;
                struct
                {
                    ULONG SubSystemKey;
                    ULONG StartAddress;
                } InitialThread;
            } NewProcess;
        } CreateProcessInfo;
        struct
        {
            NTSTATUS ExitStatus;
        } ExitProcess, ExitThread;
        struct
        {
            ULONG FileHandle;
            ULONG BaseOfDll;
            ULONG DebugInfoFileOffset;
            ULONG DebugInfoSize;
            ULONG NamePointer;
        } LoadDll;
        struct
        {
            ULONG BaseAddress;
        } UnloadDll;
    } StateInfo;
} DBGUI_WAIT_STATE_CHANGE32;

typedef struct
{
    ULONG unknown;
    ULONG KeMaximumIncrement;
    ULONG PageSize;
    ULONG MmNumberOfPhysicalPages;
    ULONG MmLowestPhysicalPage;
    ULONG MmHighestPhysicalPage;
    ULONG AllocationGranularity;
    ULONG LowestUserAddress;
    ULONG HighestUserAddress;
    ULONG ActiveProcessorsAffinityMask;
    BYTE  NumberOfProcessors;
} SYSTEM_BASIC_INFORMATION32;

typedef struct
{
    ULONG CurrentSize;
    ULONG PeakSize;
    ULONG PageFaultCount;
    ULONG MinimumWorkingSet;
    ULONG MaximumWorkingSet;
    ULONG CurrentSizeIncludingTransitionInPages;
    ULONG PeakSizeIncludingTransitionInPages;
    ULONG TransitionRePurposeCount;
    ULONG Flags;
} SYSTEM_CACHE_INFORMATION32;

typedef struct
{
    ULONG ProcessId;
    UNICODE_STRING32 ImageName;
} SYSTEM_PROCESS_ID_INFORMATION32;

typedef struct
{
    ULONG  OwnerPid;
    BYTE   ObjectType;
    BYTE   HandleFlags;
    USHORT HandleValue;
    ULONG  ObjectPointer;
    ULONG  AccessMask;
} SYSTEM_HANDLE_ENTRY32;

typedef struct
{
    ULONG                 Count;
    SYSTEM_HANDLE_ENTRY32 Handle[1];
} SYSTEM_HANDLE_INFORMATION32;

typedef struct
{
    ULONG RegistryQuotaAllowed;
    ULONG RegistryQuotaUsed;
    ULONG Reserved1;
} SYSTEM_REGISTRY_QUOTA_INFORMATION32;

typedef struct
{
    ULONG  Object;
    ULONG  UniqueProcessId;
    ULONG  HandleValue;
    ULONG  GrantedAccess;
    USHORT CreatorBackTraceIndex;
    USHORT ObjectTypeIndex;
    ULONG  HandleAttributes;
    ULONG  Reserved;
} SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX32;

typedef struct
{
    ULONG  NumberOfHandles;
    ULONG  Reserved;
    SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX32 Handles[1];
} SYSTEM_HANDLE_INFORMATION_EX32;

typedef struct
{
    ULONG ProcessorMask;
    LOGICAL_PROCESSOR_RELATIONSHIP Relationship;
    union
    {
        struct
        {
            BYTE Flags;
        } ProcessorCore;
        struct
        {
            DWORD NodeNumber;
        } NumaNode;
        CACHE_DESCRIPTOR Cache;
        ULONGLONG Reserved[2];
    };
} SYSTEM_LOGICAL_PROCESSOR_INFORMATION32;

typedef struct
{
    BYTE Flags;
    BYTE EfficiencyClass;
    BYTE Reserved[20];
    WORD GroupCount;
    GROUP_AFFINITY32 GroupMask[ANYSIZE_ARRAY];
} PROCESSOR_RELATIONSHIP32;

typedef struct
{
    DWORD NodeNumber;
    BYTE Reserved[20];
    GROUP_AFFINITY32 GroupMask;
} NUMA_NODE_RELATIONSHIP32;

typedef struct
{
    BYTE Level;
    BYTE Associativity;
    WORD LineSize;
    DWORD CacheSize;
    PROCESSOR_CACHE_TYPE Type;
    BYTE Reserved[20];
    GROUP_AFFINITY32 GroupMask;
} CACHE_RELATIONSHIP32;

typedef struct
{
    BYTE MaximumProcessorCount;
    BYTE ActiveProcessorCount;
    BYTE Reserved[38];
    ULONG ActiveProcessorMask;
} PROCESSOR_GROUP_INFO32;

typedef struct
{
    WORD MaximumGroupCount;
    WORD ActiveGroupCount;
    BYTE Reserved[20];
    PROCESSOR_GROUP_INFO32 GroupInfo[ANYSIZE_ARRAY];
} GROUP_RELATIONSHIP32;

typedef struct
{
    LOGICAL_PROCESSOR_RELATIONSHIP Relationship;
    DWORD Size;
    union
    {
        PROCESSOR_RELATIONSHIP32 Processor;
        NUMA_NODE_RELATIONSHIP32 NumaNode;
        CACHE_RELATIONSHIP32 Cache;
        GROUP_RELATIONSHIP32 Group;
    };
} SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX32;

typedef struct
{
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER CreateTime;
    DWORD         dwTickCount;
    ULONG         StartAddress;
    CLIENT_ID32   ClientId;
    DWORD         dwCurrentPriority;
    DWORD         dwBasePriority;
    DWORD         dwContextSwitches;
    DWORD         dwThreadState;
    DWORD         dwWaitReason;
    DWORD         dwUnknown;
} SYSTEM_THREAD_INFORMATION32;

typedef struct
{
    ULONG            NextEntryOffset;
    DWORD            dwThreadCount;
    LARGE_INTEGER    WorkingSetPrivateSize;
    ULONG            HardFaultCount;
    ULONG            NumberOfThreadsHighWatermark;
    ULONGLONG        CycleTime;
    LARGE_INTEGER    CreationTime;
    LARGE_INTEGER    UserTime;
    LARGE_INTEGER    KernelTime;
    UNICODE_STRING32 ProcessName;
    DWORD            dwBasePriority;
    ULONG            UniqueProcessId;
    ULONG            ParentProcessId;
    ULONG            HandleCount;
    ULONG            SessionId;
    ULONG            UniqueProcessKey;
    VM_COUNTERS_EX32 vmCounters;
    IO_COUNTERS      ioCounters;
    SYSTEM_THREAD_INFORMATION32 ti[1];
} SYSTEM_PROCESS_INFORMATION32;

typedef struct
{
    SYSTEM_THREAD_INFORMATION32 ThreadInfo;
    ULONG                       StackBase;
    ULONG                       StackLimit;
    ULONG                       Win32StartAddress;
    ULONG                       TebBase;
    ULONG                       Reserved2;
    ULONG                       Reserved3;
    ULONG                       Reserved4;
} SYSTEM_EXTENDED_THREAD_INFORMATION32;

typedef struct
{
    ULONG VirtualAddress;
    ULONG NumberOfBytes;
} MEMORY_RANGE_ENTRY32;

typedef struct
{
    ULONG LowestStartingAddress;
    ULONG HighestEndingAddress;
    ULONG Alignment;
} MEM_ADDRESS_REQUIREMENTS32;

typedef struct DECLSPEC_ALIGN(8)
{
    struct
    {
        DWORD64 Type : MEM_EXTENDED_PARAMETER_TYPE_BITS;
        DWORD64 Reserved : 64 - MEM_EXTENDED_PARAMETER_TYPE_BITS;
    };
    union
    {
        DWORD64 ULong64;
        ULONG   Pointer;
        ULONG   Size;
        ULONG   Handle;
        ULONG   ULong;
    };
} MEM_EXTENDED_PARAMETER32;

typedef struct
{
    ULONG Token;
    ULONG Thread;
} PROCESS_ACCESS_TOKEN32;

typedef struct
{
    ULONG PagedPoolLimit;
    ULONG NonPagedPoolLimit;
    ULONG MinimumWorkingSetSize;
    ULONG MaximumWorkingSetSize;
    ULONG PagefileLimit;
    LARGE_INTEGER TimeLimit;
} QUOTA_LIMITS32;

#endif /* __WOW64_STRUCT32_H */
