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
    BOOLEAN ReplaceIfExists;
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
    NTSTATUS  ExitStatus;
    ULONG     PebBaseAddress;
    ULONG     AffinityMask;
    LONG      BasePriority;
    ULONG     UniqueProcessId;
    ULONG     InheritedFromUniqueProcessId;
} PROCESS_BASIC_INFORMATION32;

typedef struct
{
    ULONG Version;
    ULONG Reserved;
    ULONG Callback;
} PROCESS_INSTRUMENTATION_CALLBACK_INFORMATION32;

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
    UNICODE_STRING32 Description;
} THREAD_DESCRIPTION_INFORMATION32;

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

#endif /* __WOW64_STRUCT32_H */
