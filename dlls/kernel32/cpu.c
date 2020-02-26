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
