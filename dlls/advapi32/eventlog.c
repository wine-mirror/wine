/*
 * Win32 advapi functions
 *
 * Copyright 1995 Sven Verdoolaege, 1998 Juergen Schmied
 */

#include "wintypes.h"
#include "winreg.h"
#include "winerror.h"
#include "heap.h"

#include "debug.h"

/******************************************************************************
 * BackupEventLog32A [ADVAPI32.15]
 */
BOOL32 WINAPI BackupEventLog32A( HANDLE32 hEventLog, LPCSTR lpBackupFileName )
{
	FIXME(advapi,"stub\n");
	return TRUE;
}

/******************************************************************************
 * BackupEventLog32W [ADVAPI32.16]
 *
 * PARAMS
 *   hEventLog        []
 *   lpBackupFileName []
 */
BOOL32 WINAPI
BackupEventLog32W( HANDLE32 hEventLog, LPCWSTR lpBackupFileName )
{
	FIXME(advapi,"stub\n");
	return TRUE;
}

/******************************************************************************
 * ClearEventLog32A [ADVAPI32.19]
 */
BOOL32 WINAPI ClearEventLog32A ( HANDLE32 hEventLog, LPCSTR lpBackupFileName )
{
	FIXME(advapi,"stub\n");
	return TRUE;
}

/******************************************************************************
 * ClearEventLog32W [ADVAPI32.20]
 */
BOOL32 WINAPI ClearEventLog32W ( HANDLE32 hEventLog, LPCWSTR lpBackupFileName )
{
	FIXME(advapi,"stub\n");
	return TRUE;
}

/******************************************************************************
 * CloseEventLog32 [ADVAPI32.21]
 */
BOOL32 WINAPI CloseEventLog32 ( HANDLE32 hEventLog )
{
	FIXME(advapi,"stub\n");
	return TRUE;
}

/******************************************************************************
 * DeregisterEventSource32 [ADVAPI32.32]
 * Closes a handle to the specified event log
 *
 * PARAMS
 *    hEventLog [I] Handle to event log
 *
 * RETURNS STD
 */
BOOL32 WINAPI DeregisterEventSource32( HANDLE32 hEventLog )
{
    FIXME(advapi, "(%d): stub\n",hEventLog);
    return TRUE;
}

/******************************************************************************
 * GetNumberOfEventLogRecords32 [ADVAPI32.49]
 *
 * PARAMS
 *   hEventLog       []
 *   NumberOfRecords []
 */
BOOL32 WINAPI
GetNumberOfEventLogRecords32( HANDLE32 hEventLog, PDWORD NumberOfRecords )
{
	FIXME(advapi,"stub\n");
	return TRUE;
}

/******************************************************************************
 * GetOldestEventLogRecord32 [ADVAPI32.50]
 *
 * PARAMS
 *   hEventLog    []
 *   OldestRecord []
 */
BOOL32 WINAPI
GetOldestEventLogRecord32( HANDLE32 hEventLog, PDWORD OldestRecord )
{
	FIXME(advapi,":stub\n");
	return TRUE;
}

/******************************************************************************
 * NotifyChangeEventLog32 [ADVAPI32.98]
 *
 * PARAMS
 *   hEventLog []
 *   hEvent    []
 */
BOOL32 WINAPI NotifyChangeEventLog32( HANDLE32 hEventLog, HANDLE32 hEvent )
{
	FIXME(advapi,"stub\n");
	return TRUE;
}

/******************************************************************************
 * OpenBackupEventLog32A [ADVAPI32.105]
 */
HANDLE32 WINAPI
OpenBackupEventLog32A( LPCSTR lpUNCServerName, LPCSTR lpFileName )
{
	FIXME(advapi,"stub\n");
	return TRUE;
}

/******************************************************************************
 * OpenBackupEventLog32W [ADVAPI32.106]
 *
 * PARAMS
 *   lpUNCServerName []
 *   lpFileName      []
 */
HANDLE32 WINAPI
OpenBackupEventLog32W( LPCWSTR lpUNCServerName, LPCWSTR lpFileName )
{
	FIXME(advapi,"stub\n");
	return TRUE;
}

/******************************************************************************
 * OpenEventLog32A [ADVAPI32.107]
 */
HANDLE32 WINAPI OpenEventLog32A(LPCSTR uncname,LPCSTR source) 
{
	FIXME(advapi,"(%s,%s),stub!\n",uncname,source);
	return 0xcafe4242;
}

/******************************************************************************
 * OpenEventLog32W [ADVAPI32.108]
 *
 * PARAMS
 *   uncname []
 *   source  []
 */
HANDLE32 WINAPI
OpenEventLog32W( LPCWSTR uncname, LPCWSTR source )
{
	FIXME(advapi,"stub\n");
	return TRUE;
}

/******************************************************************************
 * ReadEventLog32A [ADVAPI32.124]
 */
BOOL32 WINAPI ReadEventLog32A( HANDLE32 hEventLog, DWORD dwReadFlags, DWORD dwRecordOffset,
    LPVOID lpBuffer, DWORD nNumberOfBytesToRead, DWORD *pnBytesRead, DWORD *pnMinNumberOfBytesNeeded )
{
	FIXME(advapi,"stub\n");
	return TRUE;
}

/******************************************************************************
 * ReadEventLog32W [ADVAPI32.125]
 *
 * PARAMS
 *   hEventLog                []
 *   dwReadFlags              []
 *   dwRecordOffset           []
 *   lpBuffer                 []
 *   nNumberOfBytesToRead     []
 *   pnBytesRead              []
 *   pnMinNumberOfBytesNeeded []
 */
BOOL32 WINAPI
ReadEventLog32W( HANDLE32 hEventLog, DWORD dwReadFlags, DWORD dwRecordOffset,
                 LPVOID lpBuffer, DWORD nNumberOfBytesToRead, 
                 DWORD *pnBytesRead, DWORD *pnMinNumberOfBytesNeeded )
{
	FIXME(advapi,"stub\n");
	return TRUE;
}

/******************************************************************************
 * RegisterEventSource32A [ADVAPI32.174]
 */
HANDLE32 WINAPI RegisterEventSource32A( LPCSTR lpUNCServerName, LPCSTR lpSourceName )
{
    LPWSTR lpUNCServerNameW = HEAP_strdupAtoW(GetProcessHeap(),0,lpUNCServerName);
    LPWSTR lpSourceNameW = HEAP_strdupAtoW(GetProcessHeap(),0,lpSourceName);
    HANDLE32 ret = RegisterEventSource32W(lpUNCServerNameW,lpSourceNameW);
    HeapFree(GetProcessHeap(),0,lpSourceNameW);
    HeapFree(GetProcessHeap(),0,lpUNCServerNameW);
    return ret;
}

/******************************************************************************
 * RegisterEventSource32W [ADVAPI32.175]
 * Returns a registered handle to an event log
 *
 * PARAMS
 *   lpUNCServerName [I] Server name for source
 *   lpSourceName    [I] Source name for registered handle
 *
 * RETURNS
 *    Success: Handle
 *    Failure: NULL
 */
HANDLE32 WINAPI
RegisterEventSource32W( LPCWSTR lpUNCServerName, LPCWSTR lpSourceName )
{
    FIXME(advapi, "(%s,%s): stub\n", debugstr_w(lpUNCServerName),
          debugstr_w(lpSourceName));
    return 1;
}

/******************************************************************************
 * ReportEvent32A [ADVAPI32.178]
 */
BOOL32 WINAPI ReportEvent32A ( HANDLE32 hEventLog, WORD wType, WORD wCategory, DWORD dwEventID,
    PSID lpUserSid, WORD wNumStrings, DWORD dwDataSize, LPCSTR *lpStrings, LPVOID lpRawData)
{
	FIXME(advapi,"stub\n");
	return TRUE;
}

/******************************************************************************
 * ReportEvent32W [ADVAPI32.179]
 *
 * PARAMS
 *   hEventLog   []
 *   wType       []
 *   wCategory   []
 *   dwEventID   []
 *   lpUserSid   []
 *   wNumStrings []
 *   dwDataSize  []
 *   lpStrings   []
 *   lpRawData   []
 */
BOOL32 WINAPI
ReportEvent32W( HANDLE32 hEventLog, WORD wType, WORD wCategory, 
                DWORD dwEventID, PSID lpUserSid, WORD wNumStrings, 
                DWORD dwDataSize, LPCWSTR *lpStrings, LPVOID lpRawData )
{
	FIXME(advapi,"stub\n");
	return TRUE;
}
