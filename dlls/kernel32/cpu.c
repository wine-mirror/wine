/*
 * What processor?
 *
 * Copyright 1995,1997 Morten Welinder
 * Copyright 1997-1998 Marcus Meissner
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

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif


#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winnt.h"
#include "winternl.h"
#include "psapi.h"
#include "ddk/wdm.h"
#include "wine/unicode.h"
#include "kernel_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(reg);

/***********************************************************************
 * 			GetSystemInfo            	[KERNEL32.@]
 *
 * Get information about the system.
 *
 * RETURNS
 *  Nothing.
 */
VOID WINAPI GetSystemInfo(
	LPSYSTEM_INFO si	/* [out] Destination for system information, may not be NULL */)
{
    NTSTATUS                 nts;
    SYSTEM_CPU_INFORMATION   sci;

    TRACE("si=0x%p\n", si);

    if ((nts = NtQuerySystemInformation( SystemCpuInformation, &sci, sizeof(sci), NULL )) != STATUS_SUCCESS)
    {
        SetLastError(RtlNtStatusToDosError(nts));
        return;
    }

    si->u.s.wProcessorArchitecture  = sci.Architecture;
    si->u.s.wReserved               = 0;
    si->dwPageSize                  = system_info.PageSize;
    si->lpMinimumApplicationAddress = system_info.LowestUserAddress;
    si->lpMaximumApplicationAddress = system_info.HighestUserAddress;
    si->dwActiveProcessorMask       = system_info.ActiveProcessorsAffinityMask;
    si->dwNumberOfProcessors        = system_info.NumberOfProcessors;

    switch (sci.Architecture)
    {
    case PROCESSOR_ARCHITECTURE_INTEL:
        switch (sci.Level)
        {
        case 3:  si->dwProcessorType = PROCESSOR_INTEL_386;     break;
        case 4:  si->dwProcessorType = PROCESSOR_INTEL_486;     break;
        case 5:
        case 6:  si->dwProcessorType = PROCESSOR_INTEL_PENTIUM; break;
        default: si->dwProcessorType = PROCESSOR_INTEL_PENTIUM; break;
        }
        break;
    case PROCESSOR_ARCHITECTURE_PPC:
        switch (sci.Level)
        {
        case 1:  si->dwProcessorType = PROCESSOR_PPC_601;       break;
        case 3:
        case 6:  si->dwProcessorType = PROCESSOR_PPC_603;       break;
        case 4:  si->dwProcessorType = PROCESSOR_PPC_604;       break;
        case 9:  si->dwProcessorType = PROCESSOR_PPC_604;       break;
        case 20: si->dwProcessorType = PROCESSOR_PPC_620;       break;
        default: si->dwProcessorType = 0;
        }
        break;
    case PROCESSOR_ARCHITECTURE_AMD64:
        si->dwProcessorType = PROCESSOR_AMD_X8664;
        break;
    case PROCESSOR_ARCHITECTURE_ARM:
        switch (sci.Level)
        {
        case 4:  si->dwProcessorType = PROCESSOR_ARM_7TDMI;     break;
        default: si->dwProcessorType = PROCESSOR_ARM920;
        }
        break;
    case PROCESSOR_ARCHITECTURE_ARM64:
        si->dwProcessorType = 0;
        break;
    default:
        FIXME("Unknown processor architecture %x\n", sci.Architecture);
        si->dwProcessorType = 0;
    }
    si->dwAllocationGranularity     = system_info.AllocationGranularity;
    si->wProcessorLevel             = sci.Level;
    si->wProcessorRevision          = sci.Revision;
}


/***********************************************************************
 * 			GetNativeSystemInfo            	[KERNEL32.@]
 */
VOID WINAPI GetNativeSystemInfo(
    LPSYSTEM_INFO si	/* [out] Destination for system information, may not be NULL */)
{
    BOOL is_wow64;

    GetSystemInfo(si); 

    IsWow64Process(GetCurrentProcess(), &is_wow64);
    if (is_wow64)
    {
        if (si->u.s.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
        {
            si->u.s.wProcessorArchitecture = PROCESSOR_ARCHITECTURE_AMD64;
            si->dwProcessorType = PROCESSOR_AMD_X8664;
        }
        else
        {
            FIXME("Add the proper information for %d in wow64 mode\n",
                  si->u.s.wProcessorArchitecture);
        }
    }
}

/***********************************************************************
 *           K32GetPerformanceInfo (KERNEL32.@)
 */
BOOL WINAPI K32GetPerformanceInfo(PPERFORMANCE_INFORMATION info, DWORD size)
{
    SYSTEM_PERFORMANCE_INFORMATION perf;
    SYSTEM_BASIC_INFORMATION basic;
    SYSTEM_PROCESS_INFORMATION *process, *spi;
    DWORD info_size;
    NTSTATUS status;

    TRACE( "(%p, %d)\n", info, size );

    if (size < sizeof(*info))
    {
        SetLastError( ERROR_BAD_LENGTH );
        return FALSE;
    }

    status = NtQuerySystemInformation( SystemPerformanceInformation, &perf, sizeof(perf), NULL );
    if (status) goto err;
    status = NtQuerySystemInformation( SystemBasicInformation, &basic, sizeof(basic), NULL );
    if (status) goto err;

    info->cb                 = sizeof(*info);
    info->CommitTotal        = perf.TotalCommittedPages;
    info->CommitLimit        = perf.TotalCommitLimit;
    info->CommitPeak         = perf.PeakCommitment;
    info->PhysicalTotal      = basic.MmNumberOfPhysicalPages;
    info->PhysicalAvailable  = perf.AvailablePages;
    info->SystemCache        = 0;
    info->KernelTotal        = perf.PagedPoolUsage + perf.NonPagedPoolUsage;
    info->KernelPaged        = perf.PagedPoolUsage;
    info->KernelNonpaged     = perf.NonPagedPoolUsage;
    info->PageSize           = basic.PageSize;

    /* fields from SYSTEM_PROCESS_INFORMATION */
    NtQuerySystemInformation( SystemProcessInformation, NULL, 0, &info_size );
    for (;;)
    {
        process = HeapAlloc( GetProcessHeap(), 0, info_size );
        if (!process)
        {
            SetLastError( ERROR_OUTOFMEMORY );
            return FALSE;
        }
        status = NtQuerySystemInformation( SystemProcessInformation, process, info_size, &info_size );
        if (!status) break;
        HeapFree( GetProcessHeap(), 0, process );
        if (status != STATUS_INFO_LENGTH_MISMATCH)
            goto err;
    }

    info->HandleCount = info->ProcessCount = info->ThreadCount = 0;
    spi = process;
    for (;;)
    {
        info->ProcessCount++;
        info->HandleCount += spi->HandleCount;
        info->ThreadCount += spi->dwThreadCount;
        if (spi->NextEntryOffset == 0) break;
        spi = (SYSTEM_PROCESS_INFORMATION *)((char *)spi + spi->NextEntryOffset);
    }
    HeapFree( GetProcessHeap(), 0, process );
    return TRUE;

err:
    SetLastError( RtlNtStatusToDosError( status ) );
    return FALSE;
}

/***********************************************************************
 *           GetLargePageMinimum (KERNEL32.@)
 */
SIZE_T WINAPI GetLargePageMinimum(void)
{
#if defined(__i386__) || defined(__x86_64__) || defined(__arm__)
    return 2 * 1024 * 1024;
#endif
    FIXME("Not implemented on your platform/architecture.\n");
    return 0;
}

/***********************************************************************
 *           GetActiveProcessorGroupCount (KERNEL32.@)
 */
WORD WINAPI GetActiveProcessorGroupCount(void)
{
    FIXME("semi-stub, always returning 1\n");
    return 1;
}

/***********************************************************************
 *           GetActiveProcessorCount (KERNEL32.@)
 */
DWORD WINAPI GetActiveProcessorCount(WORD group)
{
    SYSTEM_INFO si;
    DWORD cpus;

    GetSystemInfo( &si );
    cpus = si.dwNumberOfProcessors;

    FIXME("semi-stub, returning %u\n", cpus);
    return cpus;
}

/***********************************************************************
 *           GetMaximumProcessorCount (KERNEL32.@)
 */
DWORD WINAPI GetMaximumProcessorCount(WORD group)
{
    SYSTEM_INFO si;
    DWORD cpus;

    GetSystemInfo( &si );
    cpus = si.dwNumberOfProcessors;

    FIXME("semi-stub, returning %u\n", cpus);
    return cpus;
}

/***********************************************************************
 *           GetEnabledXStateFeatures (KERNEL32.@)
 */
DWORD64 WINAPI GetEnabledXStateFeatures(void)
{
    FIXME("\n");
    return 0;
}

/***********************************************************************
 *           GetSystemFirmwareTable (KERNEL32.@)
 */
UINT WINAPI GetSystemFirmwareTable(DWORD provider, DWORD id, void *buffer, DWORD size)
{
    ULONG buffer_size = FIELD_OFFSET(SYSTEM_FIRMWARE_TABLE_INFORMATION, TableBuffer) + size;
    SYSTEM_FIRMWARE_TABLE_INFORMATION *sfti = HeapAlloc(GetProcessHeap(), 0, buffer_size);
    NTSTATUS status;

    TRACE("(0x%08x, 0x%08x, %p, %d)\n", provider, id, buffer, size);

    if (!sfti)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return 0;
    }

    sfti->ProviderSignature = provider;
    sfti->Action = SystemFirmwareTable_Get;
    sfti->TableID = id;

    status = NtQuerySystemInformation(SystemFirmwareTableInformation, sfti, buffer_size, &buffer_size);
    buffer_size -= FIELD_OFFSET(SYSTEM_FIRMWARE_TABLE_INFORMATION, TableBuffer);
    if (buffer_size <= size)
        memcpy(buffer, sfti->TableBuffer, buffer_size);

    if (status) SetLastError(RtlNtStatusToDosError(status));
    HeapFree(GetProcessHeap(), 0, sfti);
    return buffer_size;
}


/***********************************************************************
 *           EnumSystemFirmwareTables (KERNEL32.@)
 */
UINT WINAPI EnumSystemFirmwareTables(DWORD provider, void *buffer, DWORD size)
{
    FIXME("(0x%08x, %p, %d)\n", provider, buffer, size);
    return 0;
}
