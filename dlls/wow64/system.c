/*
 * WoW64 system functions
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

#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winnt.h"
#include "winternl.h"
#include "wow64_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wow);

static void put_system_basic_information( SYSTEM_BASIC_INFORMATION32 *info32,
                                          const SYSTEM_BASIC_INFORMATION *info )
{
    info32->unknown                      = info->unknown;
    info32->KeMaximumIncrement           = info->KeMaximumIncrement;
    info32->PageSize                     = info->PageSize;
    info32->MmNumberOfPhysicalPages      = info->MmNumberOfPhysicalPages;
    info32->MmLowestPhysicalPage         = info->MmLowestPhysicalPage;
    info32->MmHighestPhysicalPage        = info->MmHighestPhysicalPage;
    info32->AllocationGranularity        = info->AllocationGranularity;
    info32->LowestUserAddress            = PtrToUlong( info->LowestUserAddress );
    info32->HighestUserAddress           = PtrToUlong( info->HighestUserAddress );
    info32->ActiveProcessorsAffinityMask = info->ActiveProcessorsAffinityMask;
    info32->NumberOfProcessors           = info->NumberOfProcessors;
}


static void put_group_affinity( GROUP_AFFINITY32 *info32, const GROUP_AFFINITY *info )
{
    info32->Mask = info->Mask;
    info32->Group = info->Group;
}


static void put_logical_proc_info_ex( SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX32 *info32,
                                      const SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *info )
{
    ULONG i;

    info32->Relationship = info->Relationship;
    info32->Size = offsetof( SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX32, Processor );
    switch (info->Relationship)
    {
    case RelationProcessorCore:
    case RelationProcessorPackage:
        info32->Processor.Flags           = info->Processor.Flags;
        info32->Processor.EfficiencyClass = info->Processor.EfficiencyClass;
        info32->Processor.GroupCount      = info->Processor.GroupCount;
        for (i = 0; i < info->Processor.GroupCount; i++)
            put_group_affinity( &info32->Processor.GroupMask[i], &info->Processor.GroupMask[i] );
        info32->Size += offsetof( PROCESSOR_RELATIONSHIP32, GroupMask[i] );
        break;
    case RelationNumaNode:
        info32->NumaNode.NodeNumber = info->NumaNode.NodeNumber;
        put_group_affinity( &info32->NumaNode.GroupMask, &info->NumaNode.GroupMask );
        info32->Size += sizeof(info32->NumaNode);
        break;
    case RelationCache:
        info32->Cache.Level         = info->Cache.Level;
        info32->Cache.Associativity = info->Cache.Associativity;
        info32->Cache.LineSize      = info->Cache.LineSize;
        info32->Cache.CacheSize     = info->Cache.CacheSize;
        info32->Cache.Type          = info->Cache.Type;
        put_group_affinity( &info32->Cache.GroupMask, &info->Cache.GroupMask );
        info32->Size += sizeof(info32->Cache);
        break;
    case RelationGroup:
        info32->Group.MaximumGroupCount = info->Group.MaximumGroupCount;
        info32->Group.ActiveGroupCount = info->Group.ActiveGroupCount;
        for (i = 0; i < info->Group.MaximumGroupCount; i++)
        {
            info32->Group.GroupInfo[i].MaximumProcessorCount = info->Group.GroupInfo[i].MaximumProcessorCount;
            info32->Group.GroupInfo[i].ActiveProcessorCount = info->Group.GroupInfo[i].ActiveProcessorCount;
            info32->Group.GroupInfo[i].ActiveProcessorMask = info->Group.GroupInfo[i].ActiveProcessorMask;
        }
        info32->Size += offsetof( GROUP_RELATIONSHIP32, GroupInfo[i] );
        break;
    default:
        break;
    }
}


static NTSTATUS put_system_proc_info( SYSTEM_PROCESS_INFORMATION32 *info32,
                                      const SYSTEM_PROCESS_INFORMATION *info,
                                      BOOL ext_info, ULONG len, ULONG *retlen )
{
    ULONG inpos = 0, outpos = 0, i;
    SYSTEM_PROCESS_INFORMATION32 *prev = NULL;
    ULONG ti_size = (ext_info ? sizeof(SYSTEM_EXTENDED_THREAD_INFORMATION) : sizeof(SYSTEM_THREAD_INFORMATION));
    ULONG ti_size32 = (ext_info ? sizeof(SYSTEM_EXTENDED_THREAD_INFORMATION32) : sizeof(SYSTEM_THREAD_INFORMATION32));

    for (;;)
    {
        SYSTEM_EXTENDED_THREAD_INFORMATION *ti;
        SYSTEM_EXTENDED_THREAD_INFORMATION32 *ti32;
        SYSTEM_PROCESS_INFORMATION *proc = (SYSTEM_PROCESS_INFORMATION *)((char *)info + inpos);
        SYSTEM_PROCESS_INFORMATION32 *proc32 = (SYSTEM_PROCESS_INFORMATION32 *)((char *)info32 + outpos);
        ULONG proc_len = offsetof( SYSTEM_PROCESS_INFORMATION32, ti ) + proc->dwThreadCount * ti_size32;

        if (outpos + proc_len + proc->ProcessName.MaximumLength <= len)
        {
            memset( proc32, 0, proc_len );

            proc32->dwThreadCount                = proc->dwThreadCount;
            proc32->WorkingSetPrivateSize        = proc->WorkingSetPrivateSize;
            proc32->HardFaultCount               = proc->HardFaultCount;
            proc32->NumberOfThreadsHighWatermark = proc->NumberOfThreadsHighWatermark;
            proc32->CycleTime                    = proc->CycleTime;
            proc32->CreationTime                 = proc->CreationTime;
            proc32->UserTime                     = proc->UserTime;
            proc32->KernelTime                   = proc->KernelTime;
            proc32->ProcessName.Length           = proc->ProcessName.Length;
            proc32->ProcessName.MaximumLength    = proc->ProcessName.MaximumLength;
            proc32->ProcessName.Buffer           = PtrToUlong( (char *)proc32 + proc_len );
            proc32->dwBasePriority               = proc->dwBasePriority;
            proc32->UniqueProcessId              = HandleToULong( proc->UniqueProcessId );
            proc32->ParentProcessId              = HandleToULong( proc->ParentProcessId );
            proc32->HandleCount                  = proc->HandleCount;
            proc32->SessionId                    = proc->SessionId;
            proc32->UniqueProcessKey             = proc->UniqueProcessKey;
            proc32->ioCounters                   = proc->ioCounters;
            put_vm_counters( &proc32->vmCounters, &proc->vmCounters, sizeof(proc32->vmCounters) );
            for (i = 0; i < proc->dwThreadCount; i++)
            {
                ti = (SYSTEM_EXTENDED_THREAD_INFORMATION *)((char *)proc->ti + i * ti_size);
                ti32 = (SYSTEM_EXTENDED_THREAD_INFORMATION32 *)((char *)proc32->ti + i * ti_size32);
                ti32->ThreadInfo.KernelTime        = ti->ThreadInfo.KernelTime;
                ti32->ThreadInfo.UserTime          = ti->ThreadInfo.UserTime;
                ti32->ThreadInfo.CreateTime        = ti->ThreadInfo.CreateTime;
                ti32->ThreadInfo.dwTickCount       = ti->ThreadInfo.dwTickCount;
                ti32->ThreadInfo.StartAddress      = PtrToUlong( ti->ThreadInfo.StartAddress );
                ti32->ThreadInfo.dwCurrentPriority = ti->ThreadInfo.dwCurrentPriority;
                ti32->ThreadInfo.dwBasePriority    = ti->ThreadInfo.dwBasePriority;
                ti32->ThreadInfo.dwContextSwitches = ti->ThreadInfo.dwContextSwitches;
                ti32->ThreadInfo.dwThreadState     = ti->ThreadInfo.dwThreadState;
                ti32->ThreadInfo.dwWaitReason      = ti->ThreadInfo.dwWaitReason;
                put_client_id( &ti32->ThreadInfo.ClientId, &ti->ThreadInfo.ClientId );
                if (ext_info)
                {
                    ti32->StackBase         = PtrToUlong( ti->StackBase );
                    ti32->StackLimit        = PtrToUlong( ti->StackLimit );
                    ti32->Win32StartAddress = PtrToUlong( ti->Win32StartAddress );
                    ti32->TebBase           = PtrToUlong( ti->TebBase );
                    ti32->Reserved2         = ti->Reserved2;
                    ti32->Reserved3         = ti->Reserved3;
                    ti32->Reserved4         = ti->Reserved4;
                }
            }
            memcpy( (char *)proc32 + proc_len, proc->ProcessName.Buffer,
                    proc->ProcessName.MaximumLength );

            if (prev) prev->NextEntryOffset = (char *)proc32 - (char *)prev;
            prev = proc32;
        }
        outpos += proc_len + proc->ProcessName.MaximumLength;
        inpos += proc->NextEntryOffset;
        if (!proc->NextEntryOffset) break;
    }
    if (retlen) *retlen = outpos;
    if (outpos <= len) return STATUS_SUCCESS;
    else return STATUS_INFO_LENGTH_MISMATCH;
}


/**********************************************************************
 *           wow64_NtDisplayString
 */
NTSTATUS WINAPI wow64_NtDisplayString( UINT *args )
{
    const UNICODE_STRING32 *str32 = get_ptr( &args );

    UNICODE_STRING str;

    return NtDisplayString( unicode_str_32to64( &str, str32 ));
}


/**********************************************************************
 *           wow64_NtInitiatePowerAction
 */
NTSTATUS WINAPI wow64_NtInitiatePowerAction( UINT *args )
{
    POWER_ACTION action = get_ulong( &args );
    SYSTEM_POWER_STATE state = get_ulong( &args );
    ULONG flags = get_ulong( &args );
    BOOLEAN async = get_ulong( &args );

    return NtInitiatePowerAction( action, state, flags, async );
}


/**********************************************************************
 *           wow64_NtLoadDriver
 */
NTSTATUS WINAPI wow64_NtLoadDriver( UINT *args )
{
    UNICODE_STRING32 *str32 = get_ptr( &args );

    UNICODE_STRING str;

    return NtLoadDriver( unicode_str_32to64( &str, str32 ));
}


/**********************************************************************
 *           wow64_NtPowerInformation
 */
NTSTATUS WINAPI wow64_NtPowerInformation( UINT *args )
{
    POWER_INFORMATION_LEVEL level = get_ulong( &args );
    void *in_buf = get_ptr( &args );
    ULONG in_len = get_ulong( &args );
    void *out_buf = get_ptr( &args );
    ULONG out_len = get_ulong( &args );

    switch (level)
    {
    case SystemPowerCapabilities:   /* SYSTEM_POWER_CAPABILITIES */
    case SystemBatteryState:   /* SYSTEM_BATTERY_STATE */
    case SystemExecutionState:   /* ULONG */
    case ProcessorInformation:   /* PROCESSOR_POWER_INFORMATION */
        return NtPowerInformation( level, in_buf, in_len, out_buf, out_len );

    default:
        FIXME( "unsupported level %u\n", level );
        return STATUS_NOT_IMPLEMENTED;
    }
}


/**********************************************************************
 *           wow64_NtQueryLicenseValue
 */
NTSTATUS WINAPI wow64_NtQueryLicenseValue( UINT *args )
{
    UNICODE_STRING32 *str32 = get_ptr( &args );
    ULONG *type = get_ptr( &args );
    void *buffer = get_ptr( &args );
    ULONG len = get_ulong( &args );
    ULONG *retlen = get_ptr( &args );

    UNICODE_STRING str;

    return NtQueryLicenseValue( unicode_str_32to64( &str, str32 ), type, buffer, len, retlen );
}


/**********************************************************************
 *           wow64_NtQuerySystemEnvironmentValue
 */
NTSTATUS WINAPI wow64_NtQuerySystemEnvironmentValue( UINT *args )
{
    UNICODE_STRING32 *str32 = get_ptr( &args );
    WCHAR *buffer = get_ptr( &args );
    ULONG len = get_ulong( &args );
    ULONG *retlen = get_ptr( &args );

    UNICODE_STRING str;

    return NtQuerySystemEnvironmentValue( unicode_str_32to64( &str, str32 ), buffer, len, retlen );
}


/**********************************************************************
 *           wow64_NtQuerySystemEnvironmentValueEx
 */
NTSTATUS WINAPI wow64_NtQuerySystemEnvironmentValueEx( UINT *args )
{
    UNICODE_STRING32 *str32 = get_ptr( &args );
    GUID *vendor = get_ptr( &args );
    void *buffer = get_ptr( &args );
    ULONG *retlen = get_ptr( &args );
    ULONG *attrib = get_ptr( &args );

    UNICODE_STRING str;

    return NtQuerySystemEnvironmentValueEx( unicode_str_32to64( &str, str32 ),
                                            vendor, buffer, retlen, attrib );
}


/**********************************************************************
 *           wow64_NtQuerySystemInformation
 */
NTSTATUS WINAPI wow64_NtQuerySystemInformation( UINT *args )
{
    SYSTEM_INFORMATION_CLASS class = get_ulong( &args );
    void *ptr = get_ptr( &args );
    ULONG len = get_ulong( &args );
    ULONG *retlen = get_ptr( &args );

    NTSTATUS status;

    switch (class)
    {
    case SystemPerformanceInformation:  /* SYSTEM_PERFORMANCE_INFORMATION */
    case SystemTimeOfDayInformation:  /* SYSTEM_TIMEOFDAY_INFORMATION */
    case SystemProcessorPerformanceInformation:  /* SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION */
    case SystemInterruptInformation:  /* SYSTEM_INTERRUPT_INFORMATION */
    case SystemTimeAdjustmentInformation:  /* SYSTEM_TIME_ADJUSTMENT_QUERY */
    case SystemKernelDebuggerInformation:  /* SYSTEM_KERNEL_DEBUGGER_INFORMATION */
    case SystemCurrentTimeZoneInformation:   /* RTL_TIME_ZONE_INFORMATION */
    case SystemRecommendedSharedDataAlignment:  /* ULONG */
    case SystemFirmwareTableInformation:  /* SYSTEM_FIRMWARE_TABLE_INFORMATION */
    case SystemDynamicTimeZoneInformation:  /* RTL_DYNAMIC_TIME_ZONE_INFORMATION */
    case SystemCodeIntegrityInformation:  /* SYSTEM_CODEINTEGRITY_INFORMATION */
    case SystemKernelDebuggerInformationEx:  /* SYSTEM_KERNEL_DEBUGGER_INFORMATION_EX */
    case SystemCpuSetInformation:  /* SYSTEM_CPU_SET_INFORMATION */
    case SystemProcessorBrandString:  /* char[] */
    case SystemProcessorFeaturesInformation:  /* SYSTEM_PROCESSOR_FEATURES_INFORMATION */
    case SystemWineVersionInformation:  /* char[] */
        return NtQuerySystemInformation( class, ptr, len, retlen );

    case SystemCpuInformation:  /* SYSTEM_CPU_INFORMATION */
    case SystemEmulationProcessorInformation:  /* SYSTEM_CPU_INFORMATION */
        status = NtQuerySystemInformation( SystemEmulationProcessorInformation, ptr, len, retlen );
        if (!status && pBTCpuUpdateProcessorInformation) pBTCpuUpdateProcessorInformation( ptr );
        return status;

    case SystemBasicInformation:  /* SYSTEM_BASIC_INFORMATION */
    case SystemEmulationBasicInformation:  /* SYSTEM_BASIC_INFORMATION */
        if (len == sizeof(SYSTEM_BASIC_INFORMATION32))
        {
            SYSTEM_BASIC_INFORMATION info;
            SYSTEM_BASIC_INFORMATION32 *info32 = ptr;

            if (!(status = NtQuerySystemInformation( SystemEmulationBasicInformation, &info, sizeof(info), NULL )))
                put_system_basic_information( info32, &info );
        }
        else status = STATUS_INFO_LENGTH_MISMATCH;
        if (retlen) *retlen = sizeof(SYSTEM_BASIC_INFORMATION32);
        return status;

    case SystemProcessInformation:  /* SYSTEM_PROCESS_INFORMATION */
    case SystemExtendedProcessInformation:  /* SYSTEM_PROCESS_INFORMATION */
    {
        ULONG size = len * 2, retsize;
        SYSTEM_PROCESS_INFORMATION *info = Wow64AllocateTemp( size );

        status = NtQuerySystemInformation( class, info, size, &retsize );
        if (status)
        {
            if (status == STATUS_INFO_LENGTH_MISMATCH && retlen) *retlen = retsize;
            return status;
        }
        return put_system_proc_info( ptr, info, class == SystemExtendedProcessInformation, len, retlen );
    }

    case SystemModuleInformation:  /* RTL_PROCESS_MODULES */
        if (len >= sizeof(RTL_PROCESS_MODULES32))
        {
            RTL_PROCESS_MODULES *info;
            RTL_PROCESS_MODULES32 *info32 = ptr;
            ULONG count = (len - offsetof( RTL_PROCESS_MODULES32, Modules )) / sizeof(info32->Modules[0]);
            ULONG i, size = offsetof( RTL_PROCESS_MODULES, Modules[count] );

            info = Wow64AllocateTemp( size );
            if (!(status = NtQuerySystemInformation( class, info, size, retlen )))
            {
                info32->ModulesCount = info->ModulesCount;
                for (i = 0; i < info->ModulesCount; i++)
                {
                    info32->Modules[i].Section           = HandleToULong( info->Modules[i].Section );
                    info32->Modules[i].MappedBaseAddress = 0;
                    info32->Modules[i].ImageBaseAddress  = 0;
                    info32->Modules[i].ImageSize         = info->Modules[i].ImageSize;
                    info32->Modules[i].Flags             = info->Modules[i].Flags;
                    info32->Modules[i].LoadOrderIndex    = info->Modules[i].LoadOrderIndex;
                    info32->Modules[i].InitOrderIndex    = info->Modules[i].InitOrderIndex;
                    info32->Modules[i].LoadCount         = info->Modules[i].LoadCount;
                    info32->Modules[i].NameOffset        = info->Modules[i].NameOffset;
                    strcpy( (char *)info32->Modules[i].Name, (char *)info->Modules[i].Name );
                }
            }
        }
        else status = NtQuerySystemInformation( class, NULL, 0, retlen );

        if (retlen)
        {
            ULONG count = (*retlen - offsetof(RTL_PROCESS_MODULES, Modules)) / sizeof(RTL_PROCESS_MODULE_INFORMATION32);
            *retlen = offsetof( RTL_PROCESS_MODULES32, Modules[count] );
        }
        return status;

    case SystemHandleInformation:  /* SYSTEM_HANDLE_INFORMATION */
        if (len >= sizeof(SYSTEM_HANDLE_INFORMATION32))
        {
            SYSTEM_HANDLE_INFORMATION *info;
            SYSTEM_HANDLE_INFORMATION32 *info32 = ptr;
            ULONG count = (len - offsetof(SYSTEM_HANDLE_INFORMATION32, Handle)) / sizeof(SYSTEM_HANDLE_ENTRY32);
            ULONG i, size = offsetof( SYSTEM_HANDLE_INFORMATION, Handle[count] );

            info = Wow64AllocateTemp( size );
            if (!(status = NtQuerySystemInformation( class, info, size, retlen )))
            {
                info32->Count = info->Count;
                for (i = 0; i < info->Count; i++)
                {
                    info32->Handle[i].OwnerPid      = info->Handle[i].OwnerPid;
                    info32->Handle[i].ObjectType    = info->Handle[i].ObjectType;
                    info32->Handle[i].HandleFlags   = info->Handle[i].HandleFlags;
                    info32->Handle[i].HandleValue   = info->Handle[i].HandleValue;
                    info32->Handle[i].ObjectPointer = PtrToUlong( info->Handle[i].ObjectPointer );
                    info32->Handle[i].AccessMask    = info->Handle[i].AccessMask;
                }
            }
            if (retlen)
            {
                ULONG count = (*retlen - offsetof(SYSTEM_HANDLE_INFORMATION, Handle)) / sizeof(SYSTEM_HANDLE_ENTRY);
                *retlen = offsetof( SYSTEM_HANDLE_INFORMATION32, Handle[count] );
            }
            return status;
        }
        else return STATUS_INFO_LENGTH_MISMATCH;

    case SystemFileCacheInformation:   /* SYSTEM_CACHE_INFORMATION */
        if (len >= sizeof(SYSTEM_CACHE_INFORMATION32))
        {
            SYSTEM_CACHE_INFORMATION info;
            SYSTEM_CACHE_INFORMATION32 *info32 = ptr;

            if (!(status = NtQuerySystemInformation( class, &info, sizeof(info), NULL )))
            {
                info32->CurrentSize                           = info.CurrentSize;
                info32->PeakSize                              = info.PeakSize;
                info32->PageFaultCount                        = info.PageFaultCount;
                info32->MinimumWorkingSet                     = info.MinimumWorkingSet;
                info32->MaximumWorkingSet                     = info.MaximumWorkingSet;
                info32->CurrentSizeIncludingTransitionInPages = info.CurrentSizeIncludingTransitionInPages;
                info32->PeakSizeIncludingTransitionInPages    = info.PeakSizeIncludingTransitionInPages;
                info32->TransitionRePurposeCount              = info.TransitionRePurposeCount;
                info32->Flags                                 = info.Flags;
            }
        }
        else status = STATUS_INFO_LENGTH_MISMATCH;
        if (retlen) *retlen = sizeof(SYSTEM_CACHE_INFORMATION32);
        return status;

    case SystemRegistryQuotaInformation:  /* SYSTEM_REGISTRY_QUOTA_INFORMATION */
        if (len >= sizeof(SYSTEM_REGISTRY_QUOTA_INFORMATION32))
        {
            SYSTEM_REGISTRY_QUOTA_INFORMATION info;
            SYSTEM_REGISTRY_QUOTA_INFORMATION32 *info32 = ptr;

            if (!(status = NtQuerySystemInformation( class, &info, sizeof(info), NULL )))
            {
                info32->RegistryQuotaAllowed = info.RegistryQuotaAllowed;
                info32->RegistryQuotaUsed = info.RegistryQuotaUsed;
                info32->Reserved1 = PtrToUlong( info.Reserved1 );
            }
        }
        else status = STATUS_INFO_LENGTH_MISMATCH;
        if (retlen) *retlen = sizeof(SYSTEM_REGISTRY_QUOTA_INFORMATION32);
        return status;

    case SystemExtendedHandleInformation:  /* SYSTEM_HANDLE_INFORMATION_EX */
        if (len >= sizeof(SYSTEM_HANDLE_INFORMATION_EX32))
        {
            SYSTEM_HANDLE_INFORMATION_EX *info;
            SYSTEM_HANDLE_INFORMATION_EX32 *info32 = ptr;
            ULONG count = (len - offsetof(SYSTEM_HANDLE_INFORMATION_EX32, Handles)) / sizeof(info32->Handles[0]);
            ULONG i, size = offsetof( SYSTEM_HANDLE_INFORMATION_EX, Handles[count] );

            if (!ptr) return STATUS_ACCESS_VIOLATION;
            info = Wow64AllocateTemp( size );
            if (!(status = NtQuerySystemInformation( class, info, size, retlen )))
            {
                info32->NumberOfHandles = info->NumberOfHandles;
                info32->Reserved        = info->Reserved;
                for (i = 0; i < info->NumberOfHandles; i++)
                {
                    info32->Handles[i].Object                = PtrToUlong( info->Handles[i].Object );
                    info32->Handles[i].UniqueProcessId       = info->Handles[i].UniqueProcessId;
                    info32->Handles[i].HandleValue           = info->Handles[i].HandleValue;
                    info32->Handles[i].GrantedAccess         = info->Handles[i].GrantedAccess;
                    info32->Handles[i].CreatorBackTraceIndex = info->Handles[i].CreatorBackTraceIndex;
                    info32->Handles[i].ObjectTypeIndex       = info->Handles[i].ObjectTypeIndex;
                    info32->Handles[i].HandleAttributes      = info->Handles[i].HandleAttributes;
                    info32->Handles[i].Reserved              = info->Handles[i].Reserved;
                }
            }
            if (retlen)
            {
                ULONG count = (*retlen - offsetof(SYSTEM_HANDLE_INFORMATION_EX, Handles)) / sizeof(SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX);
                *retlen = offsetof( SYSTEM_HANDLE_INFORMATION_EX32, Handles[count] );
            }
            return status;
        }
        else return STATUS_INFO_LENGTH_MISMATCH;

    case SystemLogicalProcessorInformation:  /* SYSTEM_LOGICAL_PROCESSOR_INFORMATION */
        {
            SYSTEM_LOGICAL_PROCESSOR_INFORMATION *info;
            SYSTEM_LOGICAL_PROCESSOR_INFORMATION32 *info32 = ptr;
            ULONG i, size, count;

            info = Wow64AllocateTemp( len * 2 );
            status = NtQuerySystemInformation( class, info, len * 2, &size );
            if (status && status != STATUS_INFO_LENGTH_MISMATCH) return status;
            count = size / sizeof(*info);
            if (!status && len >= count * sizeof(*info32))
            {
                for (i = 0; i < count; i++)
                {
                    info32[i].ProcessorMask = info[i].ProcessorMask;
                    info32[i].Relationship  = info[i].Relationship;
                    info32[i].Reserved[0]   = info[i].Reserved[0];
                    info32[i].Reserved[1]   = info[i].Reserved[1];
                }
            }
            else status = STATUS_INFO_LENGTH_MISMATCH;
            if (retlen) *retlen = count * sizeof(*info32);
            return status;
        }

    case SystemModuleInformationEx:   /* RTL_PROCESS_MODULE_INFORMATION_EX */
        if (len >= sizeof(RTL_PROCESS_MODULE_INFORMATION_EX) + sizeof(USHORT))
        {
            RTL_PROCESS_MODULE_INFORMATION_EX *info;
            RTL_PROCESS_MODULE_INFORMATION_EX32 *info32 = ptr;
            ULONG count = (len - sizeof(info32->NextOffset)) / sizeof(*info32);
            ULONG i, size = count * sizeof(*info) + sizeof(info->NextOffset);

            info = Wow64AllocateTemp( size );
            if (!(status = NtQuerySystemInformation( class, info, size, retlen )))
            {
                RTL_PROCESS_MODULE_INFORMATION_EX *mod = info;
                for (i = 0; mod->NextOffset; i++)
                {
                    info32[i].NextOffset                 = sizeof(*info32);
                    info32[i].BaseInfo.Section           = HandleToULong( mod->BaseInfo.Section );
                    info32[i].BaseInfo.MappedBaseAddress = 0;
                    info32[i].BaseInfo.ImageBaseAddress  = 0;
                    info32[i].BaseInfo.ImageSize         = mod->BaseInfo.ImageSize;
                    info32[i].BaseInfo.Flags             = mod->BaseInfo.Flags;
                    info32[i].BaseInfo.LoadOrderIndex    = mod->BaseInfo.LoadOrderIndex;
                    info32[i].BaseInfo.InitOrderIndex    = mod->BaseInfo.InitOrderIndex;
                    info32[i].BaseInfo.LoadCount         = mod->BaseInfo.LoadCount;
                    info32[i].BaseInfo.NameOffset        = mod->BaseInfo.NameOffset;
                    info32[i].ImageCheckSum              = mod->ImageCheckSum;
                    info32[i].TimeDateStamp              = mod->TimeDateStamp;
                    info32[i].DefaultBase                = 0;
                    strcpy( (char *)info32[i].BaseInfo.Name, (char *)mod->BaseInfo.Name );
                    mod = (RTL_PROCESS_MODULE_INFORMATION_EX *)((char *)mod + mod->NextOffset);
                }
                info32[i].NextOffset = 0;
            }
        }
        else status = NtQuerySystemInformation( class, NULL, 0, retlen );

        if (retlen)
        {
            ULONG count = (*retlen - sizeof(USHORT)) / sizeof(RTL_PROCESS_MODULE_INFORMATION_EX);
            *retlen = count * sizeof(RTL_PROCESS_MODULE_INFORMATION_EX32) + sizeof(USHORT);
        }
        return status;

    case SystemNativeBasicInformation:
        return STATUS_INVALID_INFO_CLASS;

    default:
        FIXME( "unsupported class %u\n", class );
        return STATUS_INVALID_INFO_CLASS;
    }
}


/**********************************************************************
 *           wow64_NtQuerySystemInformationEx
 */
NTSTATUS WINAPI wow64_NtQuerySystemInformationEx( UINT *args )
{
    SYSTEM_INFORMATION_CLASS class = get_ulong( &args );
    void *query = get_ptr( &args );
    ULONG query_len = get_ulong( &args );
    void *ptr = get_ptr( &args );
    ULONG len = get_ulong( &args );
    ULONG *retlen = get_ptr( &args );

    HANDLE handle;
    NTSTATUS status;

    if (!query || query_len < sizeof(LONG)) return STATUS_INVALID_PARAMETER;
    handle = LongToHandle( *(LONG *)query );

    switch (class)
    {
    case SystemLogicalProcessorInformationEx:  /* SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX */
    {
        SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX32 *ex32, *info32 = ptr;
        SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *ex, *info;
        ULONG size, size32, pos = 0, pos32 = 0;

        status = NtQuerySystemInformationEx( class, &handle, sizeof(handle), NULL, 0, &size );
        if (status != STATUS_INFO_LENGTH_MISMATCH) return status;
        info = Wow64AllocateTemp( size );
        status = NtQuerySystemInformationEx( class, &handle, sizeof(handle), info, size, &size );
        if (!status)
        {
            for (pos = pos32 = 0; pos < size; pos += ex->Size, pos32 += size32)
            {
                ex = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *)((char *)info + pos);
                ex32 = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX32 *)((char *)info32 + pos32);

                switch (ex->Relationship)
                {
                case RelationProcessorCore:
                case RelationProcessorPackage:
                    size32 = offsetof( SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX32,
                                       Processor.GroupMask[ex->Processor.GroupCount] );
                    break;
                case RelationNumaNode:
                    size32 = offsetof( SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX32, NumaNode ) + sizeof( ex32->NumaNode );
                    break;
                case RelationCache:
                    size32 = offsetof( SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX32, Cache ) + sizeof( ex32->Cache );
                    break;
                case RelationGroup:
                    size32 = offsetof( SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX32,
                                       Group.GroupInfo[ex->Group.MaximumGroupCount] );
                    break;
                default:
                    size32 = 0;
                    continue;
                }
                if (pos32 + size32 <= len) put_logical_proc_info_ex( ex32, ex );
            }
            if (pos32 > len) status = STATUS_INFO_LENGTH_MISMATCH;
            size = pos32;
        }
        if (retlen) *retlen = size;
        return status;
    }

    case SystemCpuSetInformation:  /* SYSTEM_CPU_SET_INFORMATION */
    case SystemSupportedProcessorArchitectures:  /* SYSTEM_SUPPORTED_PROCESSOR_ARCHITECTURES_INFORMATION */
        return NtQuerySystemInformationEx( class, &handle, sizeof(handle), ptr, len, retlen );

    default:
        FIXME( "unsupported class %u\n", class );
        return STATUS_INVALID_INFO_CLASS;
    }
}


/**********************************************************************
 *           wow64_NtQuerySystemTime
 */
NTSTATUS WINAPI wow64_NtQuerySystemTime( UINT *args )
{
    LARGE_INTEGER *time = get_ptr( &args );

    return NtQuerySystemTime( time );
}


/**********************************************************************
 *           wow64_NtRaiseHardError
 */
NTSTATUS WINAPI wow64_NtRaiseHardError( UINT *args )
{
    NTSTATUS status = get_ulong( &args );
    ULONG count = get_ulong( &args );
    ULONG params_mask = get_ulong( &args );
    ULONG *params = get_ptr( &args );
    HARDERROR_RESPONSE_OPTION option = get_ulong( &args );
    HARDERROR_RESPONSE *response = get_ptr( &args );

    FIXME( "%08lx %lu %lx %p %u %p: stub\n", status, count, params_mask, params, option, response );
    return STATUS_NOT_IMPLEMENTED;
}


/**********************************************************************
 *           wow64_NtSetIntervalProfile
 */
NTSTATUS WINAPI wow64_NtSetIntervalProfile( UINT *args )
{
    ULONG interval = get_ulong( &args );
    KPROFILE_SOURCE source = get_ulong( &args );

    return NtSetIntervalProfile( interval, source );
}


/**********************************************************************
 *           wow64_NtSetSystemInformation
 */
NTSTATUS WINAPI wow64_NtSetSystemInformation( UINT *args )
{
    SYSTEM_INFORMATION_CLASS class = get_ulong( &args );
    void *info = get_ptr( &args );
    ULONG len = get_ulong( &args );

    return NtSetSystemInformation( class, info, len );
}


/**********************************************************************
 *           wow64_NtSetSystemTime
 */
NTSTATUS WINAPI wow64_NtSetSystemTime( UINT *args )
{
    const LARGE_INTEGER *new = get_ptr( &args );
    LARGE_INTEGER *old = get_ptr( &args );

    return NtSetSystemTime( new, old );
}


/**********************************************************************
 *           wow64_NtShutdownSystem
 */
NTSTATUS WINAPI wow64_NtShutdownSystem( UINT *args )
{
    SHUTDOWN_ACTION action = get_ulong( &args );

    return NtShutdownSystem( action );
}


/**********************************************************************
 *           wow64_NtSystemDebugControl
 */
NTSTATUS WINAPI wow64_NtSystemDebugControl( UINT *args )
{
    SYSDBG_COMMAND command = get_ulong( &args );
    void *in_buf = get_ptr( &args );
    ULONG in_len = get_ulong( &args );
    void *out_buf = get_ptr( &args );
    ULONG out_len = get_ulong( &args );
    ULONG *retlen = get_ptr( &args );

    switch (command)
    {
    case SysDbgBreakPoint:
    case SysDbgEnableKernelDebugger:
    case SysDbgDisableKernelDebugger:
    case SysDbgGetAutoKdEnable:
    case SysDbgSetAutoKdEnable:
    case SysDbgGetPrintBufferSize:
    case SysDbgSetPrintBufferSize:
    case SysDbgGetKdUmExceptionEnable:
    case SysDbgSetKdUmExceptionEnable:
    case SysDbgGetTriageDump:
    case SysDbgGetKdBlockEnable:
    case SysDbgSetKdBlockEnable:
    case SysDbgRegisterForUmBreakInfo:
    case SysDbgGetUmBreakPid:
    case SysDbgClearUmBreakPid:
    case SysDbgGetUmAttachPid:
    case SysDbgClearUmAttachPid:
        return NtSystemDebugControl( command, in_buf, in_len, out_buf, out_len, retlen );

    default:
        return STATUS_NOT_IMPLEMENTED;  /* not implemented on Windows either */
    }
}


/**********************************************************************
 *           wow64_NtUnloadDriver
 */
NTSTATUS WINAPI wow64_NtUnloadDriver( UINT *args )
{
    UNICODE_STRING32 *str32 = get_ptr( &args );

    UNICODE_STRING str;

    return NtUnloadDriver( unicode_str_32to64( &str, str32 ));
}


/**********************************************************************
 *           wow64_NtWow64GetNativeSystemInformation
 */
NTSTATUS WINAPI wow64_NtWow64GetNativeSystemInformation( UINT *args )
{
    ULONG class = get_ulong( &args );
    void *ptr = get_ptr( &args );
    ULONG len = get_ulong( &args );
    ULONG *retlen = get_ptr( &args );

    NTSTATUS status;

    switch (class)
    {
    case SystemBasicInformation:
    case SystemEmulationBasicInformation:
        if (len == sizeof(SYSTEM_BASIC_INFORMATION32))
        {
            SYSTEM_BASIC_INFORMATION info;
            SYSTEM_BASIC_INFORMATION32 *info32 = ptr;

            if (!(status = NtQuerySystemInformation( class, &info, sizeof(info), NULL )))
                put_system_basic_information( info32, &info );
        }
        else status = STATUS_INFO_LENGTH_MISMATCH;
        if (retlen) *retlen = sizeof(SYSTEM_BASIC_INFORMATION32);
        return status;

    case SystemCpuInformation:
    case SystemEmulationProcessorInformation:
    case SystemNativeBasicInformation:
        return NtQuerySystemInformation( class, ptr, len, retlen );

    default:
        return STATUS_INVALID_INFO_CLASS;
    }
}
