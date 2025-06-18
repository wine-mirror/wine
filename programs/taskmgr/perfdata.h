/*
 *  ReactOS Task Manager
 *
 *  perfdata.h
 *
 *  Copyright (C) 1999 - 2001  Brian Palmer  <brianp@reactos.org>
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
	
#ifndef __PERFDATA_H
#define __PERFDATA_H

#include "winternl.h"

typedef ULARGE_INTEGER TIME;

typedef struct _PERFDATA
{
	WCHAR				ImageName[MAX_PATH];
	ULONG				ProcessId;
	WCHAR				UserName[MAX_PATH];
	ULONG				SessionId;
	ULONG				CPUUsage;
	TIME				CPUTime;
	ULONG				BasePriority;
	ULONG				HandleCount;
	ULONG				ThreadCount;
	ULONG				USERObjectCount;
	ULONG				GDIObjectCount;
	SIZE_T				WorkingSetSizeDelta;
	ULONG				PageFaultCountDelta;
	VM_COUNTERS                     vmCounters;
	IO_COUNTERS			IOCounters;

	TIME				UserTime;
	TIME				KernelTime;
	BOOL				Wow64Process;
} PERFDATA, *PPERFDATA;

/* SystemPageFileInformation (18) */
typedef
struct _SYSTEM_PAGEFILE_INFORMATION
{
	ULONG		RelativeOffset;
	ULONG		CurrentSizePages;
	ULONG		TotalUsedPages;
	ULONG		PeakUsedPages;
	UNICODE_STRING	PagefileFileName;
} SYSTEM_PAGEFILE_INFORMATION, *PSYSTEM_PAGEFILE_INFORMATION;

#define Li2Double(x) ((double)((x).QuadPart))

#define GR_GDIOBJECTS     0       /* Count of GDI objects */
#define GR_USEROBJECTS    1       /* Count of USER objects */

BOOL	PerfDataInitialize(void);
void	PerfDataRefresh(void);

ULONG	PerfDataGetProcessCount(void);
ULONG	PerfDataGetProcessorUsage(void);
ULONG	PerfDataGetProcessorSystemUsage(void);

BOOL	PerfDataGetImageName(ULONG Index, LPWSTR lpImageName, int nMaxCount);
ULONG	PerfDataGetProcessId(ULONG Index);
BOOL	PerfDataGetUserName(ULONG Index, LPWSTR lpUserName, int nMaxCount);
ULONG	PerfDataGetSessionId(ULONG Index);
ULONG	PerfDataGetCPUUsage(ULONG Index);
TIME	PerfDataGetCPUTime(ULONG Index);
ULONG	PerfDataGetWorkingSetSizeBytes(ULONG Index);
ULONG	PerfDataGetPeakWorkingSetSizeBytes(ULONG Index);
ULONG	PerfDataGetWorkingSetSizeDelta(ULONG Index);
ULONG	PerfDataGetPageFaultCount(ULONG Index);
ULONG	PerfDataGetPageFaultCountDelta(ULONG Index);
ULONG	PerfDataGetVirtualMemorySizeBytes(ULONG Index);
ULONG	PerfDataGetPagedPoolUsagePages(ULONG Index);
ULONG	PerfDataGetNonPagedPoolUsagePages(ULONG Index);
ULONG	PerfDataGetBasePriority(ULONG Index);
ULONG	PerfDataGetHandleCount(ULONG Index);
ULONG	PerfDataGetThreadCount(ULONG Index);
ULONG	PerfDataGetUSERObjectCount(ULONG Index);
ULONG	PerfDataGetGDIObjectCount(ULONG Index);
BOOL	PerfDataGetIOCounters(ULONG Index, PIO_COUNTERS pIoCounters);

ULONG	PerfDataGetCommitChargeTotalK(void);
ULONG	PerfDataGetCommitChargeLimitK(void);
ULONG	PerfDataGetCommitChargePeakK(void);

ULONG	PerfDataGetKernelMemoryTotalK(void);
ULONG	PerfDataGetKernelMemoryPagedK(void);
ULONG	PerfDataGetKernelMemoryNonPagedK(void);

ULONG	PerfDataGetPhysicalMemoryTotalK(void);
ULONG	PerfDataGetPhysicalMemoryAvailableK(void);
ULONG	PerfDataGetPhysicalMemorySystemCacheK(void);

ULONG	PerfDataGetSystemHandleCount(void);

ULONG	PerfDataGetTotalThreadCount(void);

#endif /* __PERFDATA_H */
