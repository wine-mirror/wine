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
#include "wine/unicode.h"
#include "wine/debug.h"
#include "ddk/wdm.h"

WINE_DEFAULT_DEBUG_CHANNEL(reg);

#define SHARED_DATA     ((KSHARED_USER_DATA*)0x7ffe0000)

/****************************************************************************
 *		QueryPerformanceCounter (KERNEL32.@)
 *
 * Get the current value of the performance counter.
 * 
 * PARAMS
 *  counter [O] Destination for the current counter reading
 *
 * RETURNS
 *  Success: TRUE. counter contains the current reading
 *  Failure: FALSE.
 *
 * SEE ALSO
 *  See QueryPerformanceFrequency.
 */
BOOL WINAPI QueryPerformanceCounter(PLARGE_INTEGER counter)
{
    NtQueryPerformanceCounter( counter, NULL );
    return TRUE;
}


/****************************************************************************
 *		QueryPerformanceFrequency (KERNEL32.@)
 *
 * Get the resolution of the performance counter.
 *
 * PARAMS
 *  frequency [O] Destination for the counter resolution
 *
 * RETURNS
 *  Success. TRUE. Frequency contains the resolution of the counter.
 *  Failure: FALSE.
 *
 * SEE ALSO
 *  See QueryPerformanceCounter.
 */
BOOL WINAPI QueryPerformanceFrequency(PLARGE_INTEGER frequency)
{
    LARGE_INTEGER counter;
    NtQueryPerformanceCounter( &counter, frequency );
    return TRUE;
}


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
    SYSTEM_BASIC_INFORMATION sbi;
    SYSTEM_CPU_INFORMATION   sci;

    TRACE("si=0x%p\n", si);

    if ((nts = NtQuerySystemInformation( SystemBasicInformation, &sbi, sizeof(sbi), NULL )) != STATUS_SUCCESS ||
        (nts = NtQuerySystemInformation( SystemCpuInformation, &sci, sizeof(sci), NULL )) != STATUS_SUCCESS)
    {
        SetLastError(RtlNtStatusToDosError(nts));
        return;
    }

    si->u.s.wProcessorArchitecture  = sci.Architecture;
    si->u.s.wReserved               = 0;
    si->dwPageSize                  = sbi.PageSize;
    si->lpMinimumApplicationAddress = sbi.LowestUserAddress;
    si->lpMaximumApplicationAddress = sbi.HighestUserAddress;
    si->dwActiveProcessorMask       = sbi.ActiveProcessorsAffinityMask;
    si->dwNumberOfProcessors        = sbi.NumberOfProcessors;

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
    default:
        FIXME("Unknown processor architecture %x\n", sci.Architecture);
        si->dwProcessorType = 0;
    }
    si->dwAllocationGranularity     = sbi.AllocationGranularity;
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
 * 			IsProcessorFeaturePresent	[KERNEL32.@]
 *
 * Determine if the cpu supports a given feature.
 * 
 * RETURNS
 *  TRUE, If the processor supports feature,
 *  FALSE otherwise.
 */
BOOL WINAPI IsProcessorFeaturePresent (
	DWORD feature	/* [in] Feature number, (PF_ constants from "winnt.h") */) 
{
  if (feature < PROCESSOR_FEATURE_MAX)
    return SHARED_DATA->ProcessorFeatures[feature];
  else
    return FALSE;
}

/***********************************************************************
 *           K32GetPerformanceInfo (KERNEL32.@)
 */
BOOL WINAPI K32GetPerformanceInfo(PPERFORMANCE_INFORMATION info, DWORD size)
{
    NTSTATUS status;

    TRACE( "(%p, %d)\n", info, size );

    status = NtQuerySystemInformation( SystemPerformanceInformation, info, size, NULL );

    if (status)
    {
        SetLastError( RtlNtStatusToDosError( status ) );
        return FALSE;
    }
    return TRUE;
}
