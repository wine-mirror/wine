/*
 * Win32 advapi functions
 *
 * Copyright 1995 Sven Verdoolaege
 * Copyright 1998 Juergen Schmied
 * Copyright 2003 Mike Hearn
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

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winternl.h"
#include "wmistr.h"
#include "evntrace.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(advapi);
WINE_DECLARE_DEBUG_CHANNEL(eventlog);

/******************************************************************************
 * BackupEventLogA [ADVAPI32.@]
 *
 * Saves the event log to a backup file.
 *
 * PARAMS
 *  hEventLog        [I] Handle to event log to backup.
 *  lpBackupFileName [I] Name of the backup file.
 *
 * RETURNS
 *  Success: nonzero. File lpBackupFileName will contain the contents of
 *           hEvenLog.
 *  Failure: zero.
 */
BOOL WINAPI BackupEventLogA( HANDLE hEventLog, LPCSTR lpBackupFileName )
{
	FIXME("(%p,%s) stub\n", hEventLog, debugstr_a(lpBackupFileName));
	return TRUE;
}

/******************************************************************************
 * BackupEventLogW [ADVAPI32.@]
 *
 * See BackupEventLogA.
 */
BOOL WINAPI BackupEventLogW( HANDLE hEventLog, LPCWSTR lpBackupFileName )
{
	FIXME("(%p,%s) stub\n", hEventLog, debugstr_w(lpBackupFileName));
	return TRUE;
}

/******************************************************************************
 * ClearEventLogA [ADVAPI32.@]
 *
 * Clears the event log and/or saves the log to a backup file.
 *
 * PARAMS
 *  hEvenLog         [I] Handle to event log to clear.
 *  lpBackupFileName [I] Name of the backup file.
 *
 * RETURNS
 *  Success: nonzero. if lpBackupFileName != NULL, lpBackupFileName will 
 *           contain the contents of hEvenLog and the log will be cleared.
 *  Failure: zero. Fails if the event log is empty or if lpBackupFileName
 *           exists.
 */
BOOL WINAPI ClearEventLogA( HANDLE hEventLog, LPCSTR lpBackupFileName )
{
	FIXME("(%p,%s) stub\n", hEventLog, debugstr_a(lpBackupFileName));
	return TRUE;
}

/******************************************************************************
 * ClearEventLogW [ADVAPI32.@]
 *
 * See ClearEventLogA.
 */
BOOL WINAPI ClearEventLogW( HANDLE hEventLog, LPCWSTR lpBackupFileName )
{
	FIXME("(%p,%s) stub\n", hEventLog, debugstr_w(lpBackupFileName));
	return TRUE;
}

/******************************************************************************
 * CloseEventLog [ADVAPI32.@]
 *
 * Closes a read handle to the event log.
 *
 * PARAMS
 *  hEventLog [I/O] Handle of the event log to close.
 *
 * RETURNS
 *  Success: nonzero
 *  Failure: zero
 */
BOOL WINAPI CloseEventLog( HANDLE hEventLog )
{
	FIXME("(%p) stub\n", hEventLog);
	return TRUE;
}

/******************************************************************************
 * DeregisterEventSource [ADVAPI32.@]
 * 
 * Closes a write handle to an event log
 *
 * PARAMS
 *  hEventLog [I/O] Handle of the event log.
 *
 * RETURNS
 *  Success: nonzero
 *  Failure: zero
 */
BOOL WINAPI DeregisterEventSource( HANDLE hEventLog )
{
    FIXME("(%p) stub\n", hEventLog);
    return TRUE;
}

/******************************************************************************
 * GetNumberOfEventLogRecords [ADVAPI32.@]
 *
 * Retrieves the number of records in an event log.
 *
 * PARAMS
 *  hEventLog       [I] Handle to an open event log.
 *  NumberOfRecords [O] Number of records in the log.
 *
 * RETURNS
 *  Success: nonzero. NumberOfRecords will contain the number of records in
 *           the log.
 *  Failure: zero
 */
BOOL WINAPI GetNumberOfEventLogRecords( HANDLE hEventLog, PDWORD NumberOfRecords )
{
    FIXME("(%p,%p) stub\n", hEventLog, NumberOfRecords);

    if (!NumberOfRecords) return FALSE;
    *NumberOfRecords = 0;

    return TRUE;
}

/******************************************************************************
 * GetOldestEventLogRecord [ADVAPI32.@]
 *
 * Retrieves the absolute record number of the oldest record in an even log.
 *
 * PARAMS
 *  hEventLog    [I] Handle to an open event log.
 *  OldestRecord [O] Absolute record number of the oldest record.
 *
 * RETURNS
 *  Success: nonzero. OldestRecord contains the record number of the oldest
 *           record in the log.
 *  Failure: zero 
 */
BOOL WINAPI GetOldestEventLogRecord( HANDLE hEventLog, PDWORD OldestRecord )
{
    FIXME("(%p,%p) stub\n", hEventLog, OldestRecord);

    if (!OldestRecord) return FALSE;
    *OldestRecord = 0;

    return TRUE;
}

/******************************************************************************
 * NotifyChangeEventLog [ADVAPI32.@]
 *
 * Enables an application to receive notification when an event is written
 * to an event log.
 *
 * PARAMS
 *  hEventLog [I] Handle to an event log.
 *  hEvent    [I] Handle to a manual-reset event object.
 *
 * RETURNS
 *  Success: nonzero
 *  Failure: zero
 */
BOOL WINAPI NotifyChangeEventLog( HANDLE hEventLog, HANDLE hEvent )
{
	FIXME("(%p,%p) stub\n", hEventLog, hEvent);
	return TRUE;
}

/******************************************************************************
 * OpenBackupEventLogA [ADVAPI32.@]
 *
 * Opens a handle to a backup event log.
 *
 * PARAMS
 *  lpUNCServerName [I] Universal Naming Convention name of the server on which
 *                      this will be performed.
 *  lpFileName      [I] Specifies the name of the backup file.
 *
 * RETURNS
 *  Success: Handle to the backup event log.
 *  Failure: NULL
 */
HANDLE WINAPI OpenBackupEventLogA( LPCSTR lpUNCServerName, LPCSTR lpFileName )
{
	FIXME("(%s,%s) stub\n", debugstr_a(lpUNCServerName), debugstr_a(lpFileName));
	return (HANDLE)0xcafe4242;
}

/******************************************************************************
 * OpenBackupEventLogW [ADVAPI32.@]
 *
 * See OpenBackupEventLogA.
 */
HANDLE WINAPI OpenBackupEventLogW( LPCWSTR lpUNCServerName, LPCWSTR lpFileName )
{
	FIXME("(%s,%s) stub\n", debugstr_w(lpUNCServerName), debugstr_w(lpFileName));
	return (HANDLE)0xcafe4242;
}

/******************************************************************************
 * OpenEventLogA [ADVAPI32.@]
 *
 * Opens a handle to the specified event log.
 *
 * PARAMS
 *  lpUNCServerName [I] UNC name of the server on which the event log is
 *                      opened.
 *  lpSourceName    [I] Name of the log.
 *
 * RETURNS
 *  Success: Handle to an event log.
 *  Failure: NULL
 */
HANDLE WINAPI OpenEventLogA( LPCSTR uncname, LPCSTR source )
{
	FIXME("(%s,%s) stub\n", debugstr_a(uncname), debugstr_a(source));
	return (HANDLE)0xcafe4242;
}

/******************************************************************************
 * OpenEventLogW [ADVAPI32.@]
 *
 * See OpenEventLogA.
 */
HANDLE WINAPI OpenEventLogW( LPCWSTR uncname, LPCWSTR source )
{
	FIXME("(%s,%s) stub\n", debugstr_w(uncname), debugstr_w(source));
	return (HANDLE)0xcafe4242;
}

/******************************************************************************
 * ReadEventLogA [ADVAPI32.@]
 *
 * Reads a whole number of entries from an event log.
 *
 * PARAMS
 *  hEventLog                [I] Handle of the event log to read.
 *  dwReadFlags              [I] see MSDN doc.
 *  dwRecordOffset           [I] Log-entry record number to start at.
 *  lpBuffer                 [O] Buffer for the data read.
 *  nNumberOfBytesToRead     [I] Size of lpBuffer.
 *  pnBytesRead              [O] Receives number of bytes read.
 *  pnMinNumberOfBytesNeeded [O] Receives number of bytes required for the
 *                               next log entry.
 *
 * RETURNS
 *  Success: nonzero
 *  Failure: zero
 */
BOOL WINAPI ReadEventLogA( HANDLE hEventLog, DWORD dwReadFlags, DWORD dwRecordOffset,
    LPVOID lpBuffer, DWORD nNumberOfBytesToRead, DWORD *pnBytesRead, DWORD *pnMinNumberOfBytesNeeded )
{
    FIXME("(%p,0x%08x,0x%08x,%p,0x%08x,%p,%p) stub\n", hEventLog, dwReadFlags,
          dwRecordOffset, lpBuffer, nNumberOfBytesToRead, pnBytesRead, pnMinNumberOfBytesNeeded);
    return FALSE;
}

/******************************************************************************
 * ReadEventLogW [ADVAPI32.@]
 *
 * See ReadEventLogA.
 */
BOOL WINAPI ReadEventLogW( HANDLE hEventLog, DWORD dwReadFlags, DWORD dwRecordOffset,
    LPVOID lpBuffer, DWORD nNumberOfBytesToRead, DWORD *pnBytesRead, DWORD *pnMinNumberOfBytesNeeded )
{
    FIXME("(%p,0x%08x,0x%08x,%p,0x%08x,%p,%p) stub\n", hEventLog, dwReadFlags,
          dwRecordOffset, lpBuffer, nNumberOfBytesToRead, pnBytesRead, pnMinNumberOfBytesNeeded);
    return FALSE;
}

/******************************************************************************
 * RegisterEventSourceA [ADVAPI32.@]
 *
 * Returns a registered handle to an event log.
 *
 * PARAMS
 *  lpUNCServerName [I] UNC name of the source server.
 *  lpSourceName    [I] Specifies the name of the event source to retrieve.
 *
 * RETURNS
 *  Success: Handle to the event log.
 *  Failure: NULL. Returns ERROR_INVALID_HANDLE if lpSourceName specifies the
 *           Security event log.
 */
HANDLE WINAPI RegisterEventSourceA( LPCSTR lpUNCServerName, LPCSTR lpSourceName )
{
    UNICODE_STRING lpUNCServerNameW;
    UNICODE_STRING lpSourceNameW;
    HANDLE ret;

    FIXME("(%s,%s): stub\n", debugstr_a(lpUNCServerName), debugstr_a(lpSourceName));

    RtlCreateUnicodeStringFromAsciiz(&lpUNCServerNameW, lpUNCServerName);
    RtlCreateUnicodeStringFromAsciiz(&lpSourceNameW, lpSourceName);
    ret = RegisterEventSourceW(lpUNCServerNameW.Buffer,lpSourceNameW.Buffer);
    RtlFreeUnicodeString (&lpUNCServerNameW);
    RtlFreeUnicodeString (&lpSourceNameW);
    return ret;
}

/******************************************************************************
 * RegisterEventSourceW [ADVAPI32.@]
 *
 * See RegisterEventSourceA.
 */
HANDLE WINAPI RegisterEventSourceW( LPCWSTR lpUNCServerName, LPCWSTR lpSourceName )
{
    FIXME("(%s,%s): stub\n", debugstr_w(lpUNCServerName), debugstr_w(lpSourceName));
    return (HANDLE)0xcafe4242;
}

/******************************************************************************
 * ReportEventA [ADVAPI32.@]
 *
 * Writes an entry at the end of an event log.
 *
 * PARAMS
 *  hEventLog   [I] Handle of an event log.
 *  wType       [I] See MSDN doc.
 *  wCategory   [I] Event category.
 *  dwEventID   [I] Event identifier.
 *  lpUserSid   [I] Current user's security identifier.
 *  wNumStrings [I] Number of insert strings in lpStrings.
 *  dwDataSize  [I] Size of event-specific raw data to write.
 *  lpStrings   [I] Buffer containing an array of string to be merged.
 *  lpRawData   [I] Buffer containing the binary data.
 *
 * RETURNS
 *  Success: nonzero. Entry was written to the log.
 *  Failure: zero.
 *
 * NOTES
 *  The ReportEvent function adds the time, the entry's length, and the
 *  offsets before storing the entry in the log. If lpUserSid != NULL, the
 *  username is also logged.
 */
BOOL WINAPI ReportEventA ( HANDLE hEventLog, WORD wType, WORD wCategory, DWORD dwEventID,
    PSID lpUserSid, WORD wNumStrings, DWORD dwDataSize, LPCSTR *lpStrings, LPVOID lpRawData)
{
    LPWSTR *wideStrArray;
    UNICODE_STRING str;
    int i;
    BOOL ret;

    FIXME("(%p,0x%04x,0x%04x,0x%08x,%p,0x%04x,0x%08x,%p,%p): stub\n", hEventLog,
          wType, wCategory, dwEventID, lpUserSid, wNumStrings, dwDataSize, lpStrings, lpRawData);

    if (wNumStrings == 0) return TRUE;
    if (!lpStrings) return TRUE;

    wideStrArray = HeapAlloc(GetProcessHeap(), 0, sizeof(LPWSTR) * wNumStrings);
    for (i = 0; i < wNumStrings; i++)
    {
        RtlCreateUnicodeStringFromAsciiz(&str, lpStrings[i]);
        wideStrArray[i] = str.Buffer;
    }
    ret = ReportEventW(hEventLog, wType, wCategory, dwEventID, lpUserSid,
                       wNumStrings, dwDataSize, (LPCWSTR *)wideStrArray, lpRawData);
    for (i = 0; i < wNumStrings; i++)
    {
        HeapFree( GetProcessHeap(), 0, wideStrArray[i] );
    }
    HeapFree(GetProcessHeap(), 0, wideStrArray);
    return ret;
}

/******************************************************************************
 * ReportEventW [ADVAPI32.@]
 *
 * See ReportEventA.
 */
BOOL WINAPI ReportEventW( HANDLE hEventLog, WORD wType, WORD wCategory, DWORD dwEventID,
    PSID lpUserSid, WORD wNumStrings, DWORD dwDataSize, LPCWSTR *lpStrings, LPVOID lpRawData )
{
    int i;

    FIXME("(%p,0x%04x,0x%04x,0x%08x,%p,0x%04x,0x%08x,%p,%p): stub\n", hEventLog,
          wType, wCategory, dwEventID, lpUserSid, wNumStrings, dwDataSize, lpStrings, lpRawData);

    /* partial stub */

    if (wNumStrings == 0) return TRUE;
    if (!lpStrings) return TRUE;

    for (i = 0; i < wNumStrings; i++)
    {
        switch (wType)
        {
        case EVENTLOG_SUCCESS:
            TRACE_(eventlog)("%s\n", debugstr_w(lpStrings[i]));
            break;
        case EVENTLOG_ERROR_TYPE:
            ERR_(eventlog)("%s\n", debugstr_w(lpStrings[i]));
            break;
        case EVENTLOG_WARNING_TYPE:
            WARN_(eventlog)("%s\n", debugstr_w(lpStrings[i]));
            break;
        default:
            TRACE_(eventlog)("%s\n", debugstr_w(lpStrings[i]));
            break;
        }
    }
    return TRUE;
}

/******************************************************************************
 * RegisterTraceGuidsW [ADVAPI32.@]
 *
 * Register an event trace provider and the event trace classes that it uses
 * to generate events.
 *
 * PARAMS
 *  RequestAddress     [I]   ControlCallback function
 *  RequestContext     [I]   Optional provider-defined context
 *  ControlGuid        [I]   GUID of the registering provider
 *  GuidCount          [I]   Number of elements in the TraceGuidReg array
 *  TraceGuidReg       [I/O] Array of TRACE_GUID_REGISTRATION structures
 *  MofImagePath       [I]   not supported, set to NULL
 *  MofResourceNmae    [I]   not supported, set to NULL
 *  RegistrationHandle [O]   Provider's registration handle
 *
 * RETURNS
 *  Success: ERROR_SUCCESS
 *  Failure: System error code
 *
 * FIXME
 *  Stub.
 */
ULONG WINAPI RegisterTraceGuidsW( WMIDPREQUEST RequestAddress,
                PVOID RequestContext, LPCGUID ControlGuid, ULONG GuidCount,
                PTRACE_GUID_REGISTRATION TraceGuidReg, LPCWSTR MofImagePath,
                LPCWSTR MofResourceName, PTRACEHANDLE RegistrationHandle )
{
    FIXME("%p %p %p %u %p %s %s %p\n", RequestAddress, RequestContext,
          ControlGuid, GuidCount, TraceGuidReg, debugstr_w(MofImagePath),
          debugstr_w(MofResourceName), RegistrationHandle);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/******************************************************************************
 * RegisterTraceGuidsA [ADVAPI32.@]
 *
 * See RegisterTraceGuidsW.
 *
 * FIXME
 *  Stub.
 */
ULONG WINAPI RegisterTraceGuidsA( WMIDPREQUEST RequestAddress,
                PVOID RequestContext, LPCGUID ControlGuid, ULONG GuidCount,
                PTRACE_GUID_REGISTRATION TraceGuidReg, LPCSTR MofImagePath,
                LPCSTR MofResourceName, PTRACEHANDLE RegistrationHandle )
{
    FIXME("%p %p %p %u %p %s %s %p\n", RequestAddress, RequestContext,
          ControlGuid, GuidCount, TraceGuidReg, debugstr_a(MofImagePath),
          debugstr_a(MofResourceName), RegistrationHandle);
    return ERROR_CALL_NOT_IMPLEMENTED;
}
