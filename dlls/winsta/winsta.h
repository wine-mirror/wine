/*
 * Copyright (C) 2015 Austin English
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

#ifndef _WINSTA_H
#define _WINSTA_H

#define WINSTATIONNAME_LENGTH 32
typedef WCHAR WINSTATIONNAME[WINSTATIONNAME_LENGTH + 1];

typedef enum _WINSTATIONSTATECLASS
{
    State_Active,
    State_Connected,
    State_ConnectQuery,
    State_Shadow,
    State_Disconnected,
    State_Idle,
    State_Listen,
    State_Reset,
    State_Down,
    State_Init
} WINSTATIONSTATECLASS;

typedef struct _SESSIONIDW
{
    union
    {
        ULONG SessionId;
        ULONG LogonId;
    } DUMMYUNIONNAME;
    WINSTATIONNAME WinStationName;
    WINSTATIONSTATECLASS State;
} SESSIONIDW, *PSESSIONIDW;

typedef struct _TS_SYS_PROCESS_INFORMATION
{
    ULONG NextEntryOffset;
    ULONG NumberOfThreads;
    LARGE_INTEGER SpareLi1;
    LARGE_INTEGER SpareLi2;
    LARGE_INTEGER SpareLi3;
    LARGE_INTEGER CreateTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER KernelTime;
    UNICODE_STRING ImageName;
    LONG BasePriority;
    ULONG UniqueProcessId;
    ULONG InheritedFromUniqueProcessId;
    ULONG HandleCount;
    ULONG SessionId;
    ULONG SpareUl3;
    SIZE_T PeakVirtualSize;
    SIZE_T VirtualSize;
    ULONG PageFaultCount;
    ULONG PeakWorkingSetSize;
    ULONG WorkingSetSize;
    SIZE_T QuotaPeakPagedPoolUsage;
    SIZE_T QuotaPagedPoolUsage;
    SIZE_T QuotaPeakNonPagedPoolUsage;
    SIZE_T QuotaNonPagedPoolUsage;
    SIZE_T PagefileUsage;
    SIZE_T PeakPagefileUsage;
    SIZE_T PrivatePageCount;
} TS_SYS_PROCESS_INFORMATION, *PTS_SYS_PROCESS_INFORMATION;

typedef struct _TS_ALL_PROCESSES_INFO
{
    PTS_SYS_PROCESS_INFORMATION pTsProcessInfo;
    DWORD SizeOfSid;
    PBYTE pSid;
} TS_ALL_PROCESSES_INFO, *PTS_ALL_PROCESSES_INFO;

BOOLEAN WINAPI WinStationEnumerateW(HANDLE,PSESSIONIDW *,PULONG);
BOOLEAN WINAPI WinStationGetAllProcesses(HANDLE,ULONG,PULONG,PTS_ALL_PROCESSES_INFO *);
BOOLEAN WINAPI WinStationGetProcessSid(HANDLE,ULONG,FILETIME,PVOID,PULONG);
BOOLEAN WINAPI WinStationQueryInformationW(HANDLE,ULONG,WINSTATIONINFOCLASS,PVOID,ULONG,PULONG);
BOOLEAN WINAPI WinStationRegisterConsoleNotification(HANDLE,HWND,ULONG);
BOOLEAN WINAPI WinStationUnRegisterConsoleNotification(HANDLE,HWND);

#endif /* _WINSTA_H */
