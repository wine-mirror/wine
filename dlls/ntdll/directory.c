/*
 * NTDLL directory functions
 *
 * Copyright 1993 Erik Bos
 * Copyright 2003 Eric Pouech
 * Copyright 1996, 2004 Alexandre Julliard
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

#include <assert.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#define NONAMELESSUNION
#include "windef.h"
#include "winnt.h"
#include "winternl.h"
#include "ddk/wdm.h"
#include "ntdll_misc.h"
#include "wine/list.h"
#include "wine/debug.h"
#include "wine/exception.h"


/******************************************************************
 *		RtlWow64EnableFsRedirection   (NTDLL.@)
 */
NTSTATUS WINAPI RtlWow64EnableFsRedirection( BOOLEAN enable )
{
    if (!NtCurrentTeb64()) return STATUS_NOT_IMPLEMENTED;
    NtCurrentTeb64()->TlsSlots[WOW64_TLS_FILESYSREDIR] = !enable;
    return STATUS_SUCCESS;
}


/******************************************************************
 *		RtlWow64EnableFsRedirectionEx   (NTDLL.@)
 */
NTSTATUS WINAPI RtlWow64EnableFsRedirectionEx( ULONG disable, ULONG *old_value )
{
    if (!NtCurrentTeb64()) return STATUS_NOT_IMPLEMENTED;

    __TRY
    {
        *old_value = NtCurrentTeb64()->TlsSlots[WOW64_TLS_FILESYSREDIR];
    }
    __EXCEPT_PAGE_FAULT
    {
        return STATUS_ACCESS_VIOLATION;
    }
    __ENDTRY

    NtCurrentTeb64()->TlsSlots[WOW64_TLS_FILESYSREDIR] = disable;
    return STATUS_SUCCESS;
}


/******************************************************************
 *		RtlDoesFileExists_U   (NTDLL.@)
 */
BOOLEAN WINAPI RtlDoesFileExists_U(LPCWSTR file_name)
{
    UNICODE_STRING nt_name;
    FILE_BASIC_INFORMATION basic_info;
    OBJECT_ATTRIBUTES attr;
    BOOLEAN ret;

    if (!RtlDosPathNameToNtPathName_U( file_name, &nt_name, NULL, NULL )) return FALSE;

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &nt_name;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    ret = NtQueryAttributesFile(&attr, &basic_info) == STATUS_SUCCESS;

    RtlFreeUnicodeString( &nt_name );
    return ret;
}
