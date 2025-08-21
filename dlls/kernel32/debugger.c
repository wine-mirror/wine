/*
 * Win32 debugger functions
 *
 * Copyright (C) 1999 Alexandre Julliard
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

#include <stdio.h>
#include <string.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winternl.h"
#include "kernel_private.h"
#include "wine/asm.h"
#include "wine/debug.h"
#include "wine/exception.h"

WINE_DEFAULT_DEBUG_CHANNEL(debugstr);

void *dummy = RtlUnwind;  /* force importing RtlUnwind from ntdll */

static LONG WINAPI debug_exception_handler( EXCEPTION_POINTERS *eptr )
{
    EXCEPTION_RECORD *rec = eptr->ExceptionRecord;
    return (rec->ExceptionCode == DBG_PRINTEXCEPTION_C) ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH;
}

/***********************************************************************
 *           OutputDebugStringA   (KERNEL32.@)
 *
 * Duplicate since IMVU doesn't like it if we call kernelbase.OutputDebugStringA.
 */
void WINAPI DECLSPEC_HOTPATCH OutputDebugStringA( LPCSTR str )
{
    static HANDLE DBWinMutex = NULL;
    static BOOL mutex_inited = FALSE;
    BOOL caught_by_dbg = TRUE;
    DWORD last_error = GetLastError();

    if (!str) str = "";
    WARN( "%s\n", debugstr_a(str) );

    /* raise exception, WaitForDebugEvent() will generate a corresponding debug event */
    __TRY
    {
        ULONG_PTR args[2];
        args[0] = strlen(str) + 1;
        args[1] = (ULONG_PTR)str;
        RaiseException( DBG_PRINTEXCEPTION_C, 0, 2, args );
    }
    __EXCEPT(debug_exception_handler)
    {
        caught_by_dbg = FALSE;
    }
    __ENDTRY
    if (caught_by_dbg) return;

    /* send string to a system-wide monitor */
    if (!mutex_inited)
    {
        /* first call to OutputDebugString, initialize mutex handle */
        HANDLE mutex = CreateMutexExW( NULL, L"DBWinMutex", 0, SYNCHRONIZE );
        if (mutex)
        {
            if (InterlockedCompareExchangePointer( &DBWinMutex, mutex, 0 ) != 0)
                /* someone beat us here... */
                CloseHandle( mutex );
        }
        mutex_inited = TRUE;
    }

    if (DBWinMutex)
    {
        HANDLE mapping;

        mapping = OpenFileMappingW( FILE_MAP_WRITE, FALSE, L"DBWIN_BUFFER" );
        if (mapping)
        {
            LPVOID buffer;
            HANDLE eventbuffer, eventdata;

            buffer = MapViewOfFile( mapping, FILE_MAP_WRITE, 0, 0, 0 );
            eventbuffer = OpenEventW( SYNCHRONIZE, FALSE, L"DBWIN_BUFFER_READY" );
            eventdata = OpenEventW( EVENT_MODIFY_STATE, FALSE, L"DBWIN_DATA_READY" );

            if (buffer && eventbuffer && eventdata)
            {
                /* monitor is present, synchronize with other OutputDebugString invocations */
                WaitForSingleObject( DBWinMutex, INFINITE );

                /* acquire control over the buffer */
                if (WaitForSingleObject( eventbuffer, 10000 ) == WAIT_OBJECT_0)
                {
                    int str_len = strlen( str );
                    struct _mon_buffer_t
                    {
                        DWORD pid;
                        char buffer[1];
                    } *mon_buffer = (struct _mon_buffer_t*) buffer;

                    if (str_len > (4096 - sizeof(DWORD) - 1)) str_len = 4096 - sizeof(DWORD) - 1;
                    mon_buffer->pid = GetCurrentProcessId();
                    memcpy( mon_buffer->buffer, str, str_len );
                    mon_buffer->buffer[str_len] = 0;

                    /* signal data ready */
                    SetEvent( eventdata );
                }
                ReleaseMutex( DBWinMutex );
            }

            if (buffer) UnmapViewOfFile( buffer );
            if (eventbuffer) CloseHandle( eventbuffer );
            if (eventdata) CloseHandle( eventdata );
            CloseHandle( mapping );
        }
    }
    SetLastError( last_error );
}


/***********************************************************************
 *           DebugBreakProcess   (KERNEL32.@)
 *
 *  Raises an exception so that a debugger (if attached)
 *  can take some action. Same as DebugBreak, but applies to any process.
 *
 * PARAMS
 *  hProc [I] Process to break into.
 *
 * RETURNS
 *
 *  True if successful.
 */
BOOL WINAPI DebugBreakProcess(HANDLE process)
{
    return set_ntstatus( DbgUiIssueRemoteBreakin( process ));
}


/***********************************************************************
 *           DebugSetProcessKillOnExit                    (KERNEL32.@)
 *
 * Let a debugger decide whether a debuggee will be killed upon debugger
 * termination.
 *
 * PARAMS
 *  kill [I] If set to true then kill the process on exit.
 *
 * RETURNS
 *  True if successful, false otherwise.
 */
BOOL WINAPI DebugSetProcessKillOnExit(BOOL kill)
{
    ULONG flag = kill ? DEBUG_KILL_ON_CLOSE : 0;

    return set_ntstatus( NtSetInformationDebugObject( DbgUiGetThreadDebugObject(),
                                                      DebugObjectKillProcessOnExitInformation,
                                                      &flag, sizeof(flag), NULL ));
}
