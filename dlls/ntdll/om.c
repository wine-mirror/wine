/*
 *	Object management functions
 *
 * Copyright 1999, 2000 Juergen Schmied
 * Copyright 2005 Vitaliy Margolen
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
#include <stdlib.h>
#include <string.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "wine/debug.h"
#include "windef.h"
#include "winternl.h"
#include "ntdll_misc.h"
#include "wine/server.h"
#include "wine/exception.h"


/*
 *	Generic object functions
 */

static LONG WINAPI invalid_handle_exception_handler( EXCEPTION_POINTERS *eptr )
{
    EXCEPTION_RECORD *rec = eptr->ExceptionRecord;
    return (rec->ExceptionCode == EXCEPTION_INVALID_HANDLE) ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH;
}

/* Everquest 2 / Pirates of the Burning Sea hooks NtClose, so we need a wrapper */
NTSTATUS close_handle( HANDLE handle )
{
    DWORD_PTR debug_port;
    NTSTATUS ret = unix_funcs->NtClose( handle );

    if (ret == STATUS_INVALID_HANDLE && handle && NtCurrentTeb()->Peb->BeingDebugged &&
        !NtQueryInformationProcess( NtCurrentProcess(), ProcessDebugPort, &debug_port,
                                    sizeof(debug_port), NULL) && debug_port)
    {
        __TRY
        {
            EXCEPTION_RECORD record;
            record.ExceptionCode    = EXCEPTION_INVALID_HANDLE;
            record.ExceptionFlags   = 0;
            record.ExceptionRecord  = NULL;
            record.ExceptionAddress = NULL;
            record.NumberParameters = 0;
            RtlRaiseException( &record );
        }
        __EXCEPT(invalid_handle_exception_handler)
        {
        }
        __ENDTRY
    }

    return ret;
}

/**************************************************************************
 *                 NtClose				[NTDLL.@]
 *
 * Close a handle reference to an object.
 * 
 * PARAMS
 *  Handle [I] handle to close
 *
 * RETURNS
 *  Success: ERROR_SUCCESS.
 *  Failure: An NTSTATUS error code.
 */
NTSTATUS WINAPI NtClose( HANDLE Handle )
{
    return close_handle( Handle );
}
