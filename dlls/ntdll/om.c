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

WINE_DEFAULT_DEBUG_CHANNEL(ntdll);


/*
 *	Generic object functions
 */

/******************************************************************************
 *  NtQuerySecurityObject	[NTDLL.@]
 *
 * An ntdll analogue to GetKernelObjectSecurity().
 *
 */
NTSTATUS WINAPI
NtQuerySecurityObject(
	IN HANDLE Object,
	IN SECURITY_INFORMATION RequestedInformation,
	OUT PSECURITY_DESCRIPTOR pSecurityDescriptor,
	IN ULONG Length,
	OUT PULONG ResultLength)
{
    PISECURITY_DESCRIPTOR_RELATIVE psd = pSecurityDescriptor;
    NTSTATUS status;
    unsigned int buffer_size = 512;
    BOOLEAN need_more_memory;

    TRACE("(%p,0x%08x,%p,0x%08x,%p)\n",
	Object, RequestedInformation, pSecurityDescriptor, Length, ResultLength);

    do
    {
        char *buffer = RtlAllocateHeap(GetProcessHeap(), 0, buffer_size);
        if (!buffer)
            return STATUS_NO_MEMORY;

        need_more_memory = FALSE;

        SERVER_START_REQ( get_security_object )
        {
            req->handle = wine_server_obj_handle( Object );
            req->security_info = RequestedInformation;
            wine_server_set_reply( req, buffer, buffer_size );
            status = wine_server_call( req );
            if (status == STATUS_SUCCESS)
            {
                struct security_descriptor *sd = (struct security_descriptor *)buffer;
                if (reply->sd_len)
                {
                    *ResultLength = sizeof(SECURITY_DESCRIPTOR_RELATIVE) +
                        sd->owner_len + sd->group_len + sd->sacl_len + sd->dacl_len;
                    if (Length >= *ResultLength)
                    {
                        psd->Revision = SECURITY_DESCRIPTOR_REVISION;
                        psd->Sbz1 = 0;
                        psd->Control = sd->control | SE_SELF_RELATIVE;
                        psd->Owner = sd->owner_len ? sizeof(SECURITY_DESCRIPTOR_RELATIVE) : 0;
                        psd->Group = sd->group_len ? sizeof(SECURITY_DESCRIPTOR_RELATIVE) + sd->owner_len : 0;
                        psd->Sacl = sd->sacl_len ? sizeof(SECURITY_DESCRIPTOR_RELATIVE) + sd->owner_len + sd->group_len : 0;
                        psd->Dacl = sd->dacl_len ? sizeof(SECURITY_DESCRIPTOR_RELATIVE) + sd->owner_len + sd->group_len + sd->sacl_len : 0;
                        /* owner, group, sacl and dacl are the same type as in the server
                         * and in the same order so we copy the memory in one block */
                        memcpy((char *)pSecurityDescriptor + sizeof(SECURITY_DESCRIPTOR_RELATIVE),
                               buffer + sizeof(struct security_descriptor),
                               sd->owner_len + sd->group_len + sd->sacl_len + sd->dacl_len);
                    }
                    else
                        status = STATUS_BUFFER_TOO_SMALL;
                }
                else
                {
                    *ResultLength = sizeof(SECURITY_DESCRIPTOR_RELATIVE);
                    if (Length >= *ResultLength)
                    {
                        memset(psd, 0, sizeof(*psd));
                        psd->Revision = SECURITY_DESCRIPTOR_REVISION;
                        psd->Control = SE_SELF_RELATIVE;
                    }
                    else
                        status = STATUS_BUFFER_TOO_SMALL;
                }
            }
            else if (status == STATUS_BUFFER_TOO_SMALL)
            {
                buffer_size = reply->sd_len;
                need_more_memory = TRUE;
            }
        }
        SERVER_END_REQ;
        RtlFreeHeap(GetProcessHeap(), 0, buffer);
    } while (need_more_memory);

    return status;
}


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

/******************************************************************************
 *  NtAllocateUuids   [NTDLL.@]
 */
NTSTATUS WINAPI NtAllocateUuids( ULARGE_INTEGER *time, ULONG *delta, ULONG *sequence, UCHAR *seed )
{
    FIXME("(%p,%p,%p,%p), stub.\n", time, delta, sequence, seed);
    return STATUS_SUCCESS;
}
