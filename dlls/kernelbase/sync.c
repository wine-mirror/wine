/*
 * Kernel synchronization objects
 *
 * Copyright 1998 Alexandre Julliard
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
#include <string.h>
#include <stdio.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#define NONAMELESSUNION
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winnls.h"
#include "winternl.h"
#include "winioctl.h"
#include "ddk/wdm.h"

#include "kernelbase.h"
#include "wine/asm.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(sync);

/* check if current version is NT or Win95 */
static inline BOOL is_version_nt(void)
{
    return !(GetVersion() & 0x80000000);
}


/***********************************************************************
 *              BaseGetNamedObjectDirectory  (kernelbase.@)
 */
NTSTATUS WINAPI BaseGetNamedObjectDirectory( HANDLE *dir )
{
    static HANDLE handle;
    static const WCHAR basenameW[] = {'\\','S','e','s','s','i','o','n','s','\\','%','u',
                                      '\\','B','a','s','e','N','a','m','e','d','O','b','j','e','c','t','s',0};
    WCHAR buffer[64];
    UNICODE_STRING str;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status = STATUS_SUCCESS;

    if (!handle)
    {
        HANDLE dir;

        swprintf( buffer, ARRAY_SIZE(buffer), basenameW, NtCurrentTeb()->Peb->SessionId );
        RtlInitUnicodeString( &str, buffer );
        InitializeObjectAttributes(&attr, &str, 0, 0, NULL);
        status = NtOpenDirectoryObject( &dir, DIRECTORY_CREATE_OBJECT|DIRECTORY_TRAVERSE, &attr );
        if (!status && InterlockedCompareExchangePointer( &handle, dir, 0 ) != 0)
        {
            /* someone beat us here... */
            CloseHandle( dir );
        }
    }
    *dir = handle;
    return status;
}

static void get_create_object_attributes( OBJECT_ATTRIBUTES *attr, UNICODE_STRING *nameW,
                                          SECURITY_ATTRIBUTES *sa, const WCHAR *name )
{
    attr->Length                   = sizeof(*attr);
    attr->RootDirectory            = 0;
    attr->ObjectName               = NULL;
    attr->Attributes               = OBJ_OPENIF | ((sa && sa->bInheritHandle) ? OBJ_INHERIT : 0);
    attr->SecurityDescriptor       = sa ? sa->lpSecurityDescriptor : NULL;
    attr->SecurityQualityOfService = NULL;
    if (name)
    {
        RtlInitUnicodeString( nameW, name );
        attr->ObjectName = nameW;
        BaseGetNamedObjectDirectory( &attr->RootDirectory );
    }
}

static BOOL get_open_object_attributes( OBJECT_ATTRIBUTES *attr, UNICODE_STRING *nameW,
                                        BOOL inherit, const WCHAR *name )
{
    HANDLE dir;

    if (!name)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    RtlInitUnicodeString( nameW, name );
    BaseGetNamedObjectDirectory( &dir );
    InitializeObjectAttributes( attr, nameW, inherit ? OBJ_INHERIT : 0, dir, NULL );
    return TRUE;
}


/***********************************************************************
 * Events
 ***********************************************************************/


/***********************************************************************
 *           CreateEventA    (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateEventA( SECURITY_ATTRIBUTES *sa, BOOL manual_reset,
                                              BOOL initial_state, LPCSTR name )
{
    DWORD flags = 0;

    if (manual_reset) flags |= CREATE_EVENT_MANUAL_RESET;
    if (initial_state) flags |= CREATE_EVENT_INITIAL_SET;
    return CreateEventExA( sa, name, flags, EVENT_ALL_ACCESS );
}


/***********************************************************************
 *           CreateEventW    (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateEventW( SECURITY_ATTRIBUTES *sa, BOOL manual_reset,
                                              BOOL initial_state, LPCWSTR name )
{
    DWORD flags = 0;

    if (manual_reset) flags |= CREATE_EVENT_MANUAL_RESET;
    if (initial_state) flags |= CREATE_EVENT_INITIAL_SET;
    return CreateEventExW( sa, name, flags, EVENT_ALL_ACCESS );
}


/***********************************************************************
 *           CreateEventExA    (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateEventExA( SECURITY_ATTRIBUTES *sa, LPCSTR name,
                                                DWORD flags, DWORD access )
{
    WCHAR buffer[MAX_PATH];

    if (!name) return CreateEventExW( sa, NULL, flags, access );

    if (!MultiByteToWideChar( CP_ACP, 0, name, -1, buffer, MAX_PATH ))
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    return CreateEventExW( sa, buffer, flags, access );
}


/***********************************************************************
 *           CreateEventExW    (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateEventExW( SECURITY_ATTRIBUTES *sa, LPCWSTR name,
                                                DWORD flags, DWORD access )
{
    HANDLE ret = 0;
    UNICODE_STRING nameW;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status;

    /* one buggy program needs this
     * ("Van Dale Groot woordenboek der Nederlandse taal")
     */
    if (sa && IsBadReadPtr(sa,sizeof(SECURITY_ATTRIBUTES)))
    {
        ERR("Bad security attributes pointer %p\n",sa);
        SetLastError( ERROR_INVALID_PARAMETER);
        return 0;
    }

    get_create_object_attributes( &attr, &nameW, sa, name );

    status = NtCreateEvent( &ret, access, &attr,
                            (flags & CREATE_EVENT_MANUAL_RESET) ? NotificationEvent : SynchronizationEvent,
                            (flags & CREATE_EVENT_INITIAL_SET) != 0 );
    if (status == STATUS_OBJECT_NAME_EXISTS)
        SetLastError( ERROR_ALREADY_EXISTS );
    else
        SetLastError( RtlNtStatusToDosError(status) );
    return ret;
}


/***********************************************************************
 *           OpenEventA    (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH OpenEventA( DWORD access, BOOL inherit, LPCSTR name )
{
    WCHAR buffer[MAX_PATH];

    if (!name) return OpenEventW( access, inherit, NULL );

    if (!MultiByteToWideChar( CP_ACP, 0, name, -1, buffer, MAX_PATH ))
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    return OpenEventW( access, inherit, buffer );
}


/***********************************************************************
 *           OpenEventW    (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH OpenEventW( DWORD access, BOOL inherit, LPCWSTR name )
{
    HANDLE ret;
    UNICODE_STRING nameW;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status;

    if (!is_version_nt()) access = EVENT_ALL_ACCESS;

    if (!get_open_object_attributes( &attr, &nameW, inherit, name )) return 0;

    status = NtOpenEvent( &ret, access, &attr );
    if (status != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return 0;
    }
    return ret;
}

/***********************************************************************
 *           PulseEvent    (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH PulseEvent( HANDLE handle )
{
    return set_ntstatus( NtPulseEvent( handle, NULL ));
}


/***********************************************************************
 *           SetEvent    (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetEvent( HANDLE handle )
{
    return set_ntstatus( NtSetEvent( handle, NULL ));
}


/***********************************************************************
 *           ResetEvent    (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH ResetEvent( HANDLE handle )
{
    return set_ntstatus( NtResetEvent( handle, NULL ));
}
