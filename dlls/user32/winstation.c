/*
 * Window stations and desktops
 *
 * Copyright 2002 Alexandre Julliard
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

#include "ntstatus.h"
#define WIN32_NO_STATUS

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winerror.h"
#include "wingdi.h"
#include "winuser.h"
#include "winternl.h"
#include "ddk/wdm.h"
#include "wine/debug.h"
#include "user_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(winstation);


/* callback for enumeration functions */
struct enum_proc_lparam
{
    NAMEENUMPROCA func;
    LPARAM        lparam;
};

static BOOL CALLBACK enum_names_WtoA( LPWSTR name, LPARAM lparam )
{
    struct enum_proc_lparam *data = (struct enum_proc_lparam *)lparam;
    char buffer[MAX_PATH];

    if (!WideCharToMultiByte( CP_ACP, 0, name, -1, buffer, sizeof(buffer), NULL, NULL ))
        return FALSE;
    return data->func( buffer, data->lparam );
}

static BOOL enum_names( HWINSTA handle, NAMEENUMPROCW func, LPARAM lparam )
{
    struct ntuser_name_list *list;
    ULONG i, size = offsetof( struct ntuser_name_list, strings[256] );
    WCHAR *name;
    NTSTATUS status;
    BOOL ret = TRUE;

    while (size)
    {
        if (!(list = HeapAlloc( GetProcessHeap(), 0, size ))) break;
        if (!(status = NtUserBuildNameList( handle, size, list, &size )))
        {
            for (i = 0, name = list->strings; ret && i < list->count; i++, name += wcslen( name ) + 1)
                ret = func( name, lparam );
        }
        HeapFree( GetProcessHeap(), 0, list );
        if (status != STATUS_BUFFER_TOO_SMALL) break;
    }

    if (status)
    {
        SetLastError( RtlNtStatusToDosError( status ) );
        return FALSE;
    }
    return ret;
}

/* return a handle to the directory where window station objects are created */
static HANDLE get_winstations_dir_handle(void)
{
    static HANDLE handle = NULL;
    WCHAR buffer[64];
    UNICODE_STRING str;
    OBJECT_ATTRIBUTES attr;

    if (!handle)
    {
        HANDLE dir;

        swprintf( buffer, ARRAY_SIZE(buffer), L"\\Sessions\\%u\\Windows\\WindowStations", NtCurrentTeb()->Peb->SessionId );
        RtlInitUnicodeString( &str, buffer );
        InitializeObjectAttributes( &attr, &str, 0, 0, NULL );
        NtOpenDirectoryObject( &dir, DIRECTORY_CREATE_OBJECT | DIRECTORY_TRAVERSE, &attr );
        if (InterlockedCompareExchangePointer( &handle, dir, 0 ) != 0) /* someone beat us here */
            CloseHandle( dir );
    }
    return handle;
}

static WCHAR default_name[29];

static BOOL WINAPI winstation_default_name_once( INIT_ONCE *once, void *param, void **context )
{
    TOKEN_STATISTICS stats;
    BOOL ret;

    ret = GetTokenInformation( GetCurrentProcessToken(), TokenStatistics, &stats, sizeof(stats), NULL );
    if (ret)
        swprintf( default_name, ARRAY_SIZE(default_name), L"Service-0x%x-%x$",
                  stats.AuthenticationId.HighPart, stats.AuthenticationId.LowPart );

    return ret;
}

static const WCHAR *get_winstation_default_name( void )
{
    static INIT_ONCE once = INIT_ONCE_STATIC_INIT;
    BOOL ret;

    ret = InitOnceExecuteOnce( &once, winstation_default_name_once, NULL, NULL );
    return ret ? default_name : NULL;
}

/***********************************************************************
 *              CreateWindowStationA  (USER32.@)
 */
HWINSTA WINAPI CreateWindowStationA( LPCSTR name, DWORD flags, ACCESS_MASK access,
                                     LPSECURITY_ATTRIBUTES sa )
{
    WCHAR buffer[MAX_PATH];

    if (!name) return CreateWindowStationW( NULL, flags, access, sa );

    if (!MultiByteToWideChar( CP_ACP, 0, name, -1, buffer, MAX_PATH ))
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    return CreateWindowStationW( buffer, flags, access, sa );
}


/***********************************************************************
 *              CreateWindowStationW  (USER32.@)
 */
HWINSTA WINAPI CreateWindowStationW( LPCWSTR name, DWORD flags, ACCESS_MASK access,
                                     LPSECURITY_ATTRIBUTES sa )
{
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING str;

    RtlInitUnicodeString( &str, name );
    if (!str.Length) RtlInitUnicodeString( &str, get_winstation_default_name() );

    InitializeObjectAttributes( &attr, &str, OBJ_CASE_INSENSITIVE,
                                get_winstations_dir_handle(), sa );
    if (!(flags & CWF_CREATE_ONLY)) attr.Attributes |= OBJ_OPENIF;
    if (sa && sa->bInheritHandle) attr.Attributes |= OBJ_INHERIT;

    return NtUserCreateWindowStation( &attr, access, 0, 0, 0, 0, 0 );
}


/******************************************************************************
 *              OpenWindowStationA  (USER32.@)
 */
HWINSTA WINAPI OpenWindowStationA( LPCSTR name, BOOL inherit, ACCESS_MASK access )
{
    WCHAR buffer[MAX_PATH];

    if (!name) return OpenWindowStationW( NULL, inherit, access );

    if (!MultiByteToWideChar( CP_ACP, 0, name, -1, buffer, MAX_PATH ))
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    return OpenWindowStationW( buffer, inherit, access );
}


/******************************************************************************
 *              OpenWindowStationW  (USER32.@)
 */
HWINSTA WINAPI OpenWindowStationW( LPCWSTR name, BOOL inherit, ACCESS_MASK access )
{
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING str;

    RtlInitUnicodeString( &str, name );
    if (!str.Length) RtlInitUnicodeString( &str, get_winstation_default_name() );

    InitializeObjectAttributes( &attr, &str, OBJ_CASE_INSENSITIVE,
                                get_winstations_dir_handle(), NULL );
    if (inherit) attr.Attributes |= OBJ_INHERIT;

    return NtUserOpenWindowStation( &attr, access );
}


/******************************************************************************
 *              EnumWindowStationsA  (USER32.@)
 */
BOOL WINAPI EnumWindowStationsA( WINSTAENUMPROCA func, LPARAM lparam )
{
    struct enum_proc_lparam data;
    data.func   = func;
    data.lparam = lparam;
    return EnumWindowStationsW( enum_names_WtoA, (LPARAM)&data );
}


/******************************************************************************
 *              EnumWindowStationsW  (USER32.@)
 */
BOOL WINAPI EnumWindowStationsW( WINSTAENUMPROCW func, LPARAM lparam )
{
    return enum_names( 0, func, lparam );
}


/***********************************************************************
 *              CreateDesktopA   (USER32.@)
 */
HDESK WINAPI CreateDesktopA( LPCSTR name, LPCSTR device, LPDEVMODEA devmode,
                             DWORD flags, ACCESS_MASK access, LPSECURITY_ATTRIBUTES sa )
{
    WCHAR buffer[MAX_PATH];

    if (device || devmode)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (!name) return CreateDesktopW( NULL, NULL, NULL, flags, access, sa );

    if (!MultiByteToWideChar( CP_ACP, 0, name, -1, buffer, MAX_PATH ))
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    return CreateDesktopW( buffer, NULL, NULL, flags, access, sa );
}


/***********************************************************************
 *              CreateDesktopW   (USER32.@)
 */
HDESK WINAPI CreateDesktopW( LPCWSTR name, LPCWSTR device, LPDEVMODEW devmode,
                             DWORD flags, ACCESS_MASK access, LPSECURITY_ATTRIBUTES sa )
{
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING str;

    if (device || (devmode && !(flags & DF_WINE_VIRTUAL_DESKTOP))
        || (flags & DF_WINE_ROOT_DESKTOP && flags & DF_WINE_VIRTUAL_DESKTOP))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    RtlInitUnicodeString( &str, name );
    InitializeObjectAttributes( &attr, &str, OBJ_CASE_INSENSITIVE | OBJ_OPENIF,
                                get_winstations_dir_handle(), NULL );
    if (sa && sa->bInheritHandle) attr.Attributes |= OBJ_INHERIT;
    return NtUserCreateDesktopEx( &attr, NULL, devmode, flags, access, 0 );
}


/******************************************************************************
 *              OpenDesktopA   (USER32.@)
 */
HDESK WINAPI OpenDesktopA( LPCSTR name, DWORD flags, BOOL inherit, ACCESS_MASK access )
{
    WCHAR buffer[MAX_PATH];

    if (!name) return OpenDesktopW( NULL, flags, inherit, access );

    if (!MultiByteToWideChar( CP_ACP, 0, name, -1, buffer, MAX_PATH ))
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    return OpenDesktopW( buffer, flags, inherit, access );
}


HDESK open_winstation_desktop( HWINSTA hwinsta, LPCWSTR name, DWORD flags, BOOL inherit, ACCESS_MASK access )
{
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING str;

    RtlInitUnicodeString( &str, name );
    InitializeObjectAttributes( &attr, &str, OBJ_CASE_INSENSITIVE, hwinsta, NULL );
    if (inherit) attr.Attributes |= OBJ_INHERIT;
    return NtUserOpenDesktop( &attr, flags, access );
}


/******************************************************************************
 *              OpenDesktopW   (USER32.@)
 */
HDESK WINAPI OpenDesktopW( LPCWSTR name, DWORD flags, BOOL inherit, ACCESS_MASK access )
{
    return open_winstation_desktop( NULL, name, flags, inherit, access );
}


/******************************************************************************
 *              EnumDesktopsA   (USER32.@)
 */
BOOL WINAPI EnumDesktopsA( HWINSTA winsta, DESKTOPENUMPROCA func, LPARAM lparam )
{
    struct enum_proc_lparam data;
    data.func   = func;
    data.lparam = lparam;
    return EnumDesktopsW( winsta, enum_names_WtoA, (LPARAM)&data );
}


/******************************************************************************
 *              EnumDesktopsW   (USER32.@)
 */
BOOL WINAPI EnumDesktopsW( HWINSTA winsta, DESKTOPENUMPROCW func, LPARAM lparam )
{
    if (!winsta)
        winsta = NtUserGetProcessWindowStation();

    return enum_names( winsta, func, lparam );
}


/***********************************************************************
 *              GetUserObjectInformationA   (USER32.@)
 */
BOOL WINAPI GetUserObjectInformationA( HANDLE handle, INT index, LPVOID info, DWORD len, LPDWORD needed )
{
    /* check for information types returning strings */
    if (index == UOI_TYPE || index == UOI_NAME)
    {
        WCHAR buffer[MAX_PATH];
        DWORD lenA, lenW;

        if (!NtUserGetObjectInformation( handle, index, buffer, sizeof(buffer), &lenW )) return FALSE;
        lenA = WideCharToMultiByte( CP_ACP, 0, buffer, -1, NULL, 0, NULL, NULL );
        if (needed) *needed = lenA;
        if (lenA > len)
        {
            /* If the buffer length supplied by the caller is insufficient, Windows returns a
               'needed' length based upon the Unicode byte length, so we should do similarly. */
            if (needed) *needed = lenW;

            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }
        if (info) WideCharToMultiByte( CP_ACP, 0, buffer, -1, info, len, NULL, NULL );
        return TRUE;
    }
    return NtUserGetObjectInformation( handle, index, info, len, needed );
}


/******************************************************************************
 *              SetUserObjectInformationA   (USER32.@)
 */
BOOL WINAPI SetUserObjectInformationA( HANDLE handle, INT index, LPVOID info, DWORD len )
{
    return NtUserSetObjectInformation( handle, index, info, len );
}


/***********************************************************************
 *              GetUserObjectSecurity   (USER32.@)
 */
BOOL WINAPI GetUserObjectSecurity( HANDLE handle, PSECURITY_INFORMATION info,
                                   PSECURITY_DESCRIPTOR sid, DWORD len, LPDWORD needed )
{
    FIXME( "(%p %p %p len=%ld %p),stub!\n", handle, info, sid, len, needed );
    if (needed)
        *needed = sizeof(SECURITY_DESCRIPTOR);
    if (len < sizeof(SECURITY_DESCRIPTOR))
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
    }
    return InitializeSecurityDescriptor(sid, SECURITY_DESCRIPTOR_REVISION);
}

/***********************************************************************
 *              SetUserObjectSecurity   (USER32.@)
 */
BOOL WINAPI SetUserObjectSecurity( HANDLE handle, PSECURITY_INFORMATION info,
                                   PSECURITY_DESCRIPTOR sid )
{
    FIXME( "(%p,%p,%p),stub!\n", handle, info, sid );
    return TRUE;
}
