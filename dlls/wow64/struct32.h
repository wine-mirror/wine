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
