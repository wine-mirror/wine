/*
 * Resources
 *
 * Copyright 1993 Robert J. Amstadt
 * Copyright 1995, 2003 Alexandre Julliard
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include "windef.h"
#include "winternl.h"
#include "winbase.h"
#include "wownt32.h"
#include "wine/winbase16.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(resource);

/* handle conversions */
#define HRSRC_32(h16)   ((HRSRC)(ULONG_PTR)(h16))
#define HRSRC_16(h32)   (LOWORD(h32))
#define HGLOBAL_32(h16) ((HGLOBAL)(ULONG_PTR)(h16))
#define HGLOBAL_16(h32) (LOWORD(h32))
#define HMODULE_16(h32) (LOWORD(h32))


/* retrieve the resource name to pass to the ntdll functions */
static NTSTATUS get_res_nameA( LPCSTR name, UNICODE_STRING *str )
{
    if (!HIWORD(name))
    {
        str->Buffer = (LPWSTR)name;
        return STATUS_SUCCESS;
    }
    if (name[0] == '#')
    {
        ULONG value;
        if (RtlCharToInteger( name, 10, &value ) != STATUS_SUCCESS || HIWORD(value))
            return STATUS_INVALID_PARAMETER;
        str->Buffer = (LPWSTR)value;
        return STATUS_SUCCESS;
    }
    RtlCreateUnicodeStringFromAsciiz( str, name );
    RtlUpcaseUnicodeString( str, str, FALSE );
    return STATUS_SUCCESS;
}

/* retrieve the resource name to pass to the ntdll functions */
static NTSTATUS get_res_nameW( LPCWSTR name, UNICODE_STRING *str )
{
    if (!HIWORD(name))
    {
        str->Buffer = (LPWSTR)name;
        return STATUS_SUCCESS;
    }
    if (name[0] == '#')
    {
        ULONG value;
        RtlInitUnicodeString( str, name );
        if (RtlUnicodeStringToInteger( str, 10, &value ) != STATUS_SUCCESS || HIWORD(value))
            return STATUS_INVALID_PARAMETER;
        str->Buffer = (LPWSTR)value;
        return STATUS_SUCCESS;
    }
    RtlCreateUnicodeString( str, name );
    RtlUpcaseUnicodeString( str, str, FALSE );
    return STATUS_SUCCESS;
}

/**********************************************************************
 *	    FindResourceExA  (KERNEL32.@)
 */
HRSRC WINAPI FindResourceExA( HMODULE hModule, LPCSTR type, LPCSTR name, WORD lang )
{
    NTSTATUS status;
    UNICODE_STRING nameW, typeW;
    LDR_RESOURCE_INFO info;
    const IMAGE_RESOURCE_DATA_ENTRY *entry = NULL;

    TRACE( "%p %s %s %04x\n", hModule, debugstr_a(type), debugstr_a(name), lang );

    if (!hModule) hModule = GetModuleHandleW(0);
    else if (!HIWORD(hModule))
    {
        return HRSRC_32( FindResource16( HMODULE_16(hModule), name, type ) );
    }

    nameW.Buffer = typeW.Buffer = NULL;
    if ((status = get_res_nameA( name, &nameW )) != STATUS_SUCCESS) goto done;
    if ((status = get_res_nameA( type, &typeW )) != STATUS_SUCCESS) goto done;
    info.Type = (ULONG)typeW.Buffer;
    info.Name = (ULONG)nameW.Buffer;
    info.Language = lang;
    status = LdrFindResource_U( hModule, &info, 3, &entry );
done:
    if (HIWORD(nameW.Buffer)) HeapFree( GetProcessHeap(), 0, nameW.Buffer );
    if (HIWORD(typeW.Buffer)) HeapFree( GetProcessHeap(), 0, typeW.Buffer );
    if (status != STATUS_SUCCESS) SetLastError( RtlNtStatusToDosError(status) );
    return (HRSRC)entry;
}


/**********************************************************************
 *	    FindResourceA    (KERNEL32.@)
 */
HRSRC WINAPI FindResourceA( HMODULE hModule, LPCSTR name, LPCSTR type )
{
    return FindResourceExA( hModule, type, name, MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL ) );
}


/**********************************************************************
 *	    FindResourceExW  (KERNEL32.@)
 */
HRSRC WINAPI FindResourceExW( HMODULE hModule, LPCWSTR type, LPCWSTR name, WORD lang )
{
    NTSTATUS status;
    UNICODE_STRING nameW, typeW;
    LDR_RESOURCE_INFO info;
    const IMAGE_RESOURCE_DATA_ENTRY *entry = NULL;

    TRACE( "%p %s %s %04x\n", hModule, debugstr_w(type), debugstr_w(name), lang );

    if (!hModule) hModule = GetModuleHandleW(0);
    else if (!HIWORD(hModule))
    {
        LPSTR nameA, typeA;
        HRSRC16 ret;

        if (HIWORD(name))
        {
            DWORD len = WideCharToMultiByte( CP_ACP, 0, name, -1, NULL, 0, NULL, NULL );
            nameA = HeapAlloc( GetProcessHeap(), 0, len );
            if (nameA) WideCharToMultiByte( CP_ACP, 0, name, -1, nameA, len, NULL, NULL );
        }
        else nameA = (LPSTR)name;

        if (HIWORD(type))
        {
            DWORD len = WideCharToMultiByte( CP_ACP, 0, type, -1, NULL, 0, NULL, NULL );
            typeA = HeapAlloc( GetProcessHeap(), 0, len );
            if (typeA) WideCharToMultiByte( CP_ACP, 0, type, -1, typeA, len, NULL, NULL );
        }
        else typeA = (LPSTR)type;

        ret = FindResource16( HMODULE_16(hModule), nameA, typeA );
        if (HIWORD(nameA)) HeapFree( GetProcessHeap(), 0, nameA );
        if (HIWORD(typeA)) HeapFree( GetProcessHeap(), 0, typeA );
        return HRSRC_32(ret);
    }

    nameW.Buffer = typeW.Buffer = NULL;
    if ((status = get_res_nameW( name, &nameW )) != STATUS_SUCCESS) goto done;
    if ((status = get_res_nameW( type, &typeW )) != STATUS_SUCCESS) goto done;
    info.Type = (ULONG)typeW.Buffer;
    info.Name = (ULONG)nameW.Buffer;
    info.Language = lang;
    status = LdrFindResource_U( hModule, &info, 3, &entry );
done:
    if (HIWORD(nameW.Buffer)) HeapFree( GetProcessHeap(), 0, nameW.Buffer );
    if (HIWORD(typeW.Buffer)) HeapFree( GetProcessHeap(), 0, typeW.Buffer );
    if (status != STATUS_SUCCESS) SetLastError( RtlNtStatusToDosError(status) );
    return (HRSRC)entry;
}


/**********************************************************************
 *	    FindResourceW    (KERNEL32.@)
 */
HRSRC WINAPI FindResourceW( HINSTANCE hModule, LPCWSTR name, LPCWSTR type )
{
    return FindResourceExW( hModule, type, name, MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL ) );
}


/**********************************************************************
 *	    LoadResource     (KERNEL32.@)
 */
HGLOBAL WINAPI LoadResource( HINSTANCE hModule, HRSRC hRsrc )
{
    NTSTATUS status;
    void *ret = NULL;

    TRACE( "%p %p\n", hModule, hRsrc );

    if (hModule && !HIWORD(hModule))
        /* FIXME: should convert return to 32-bit resource */
        return HGLOBAL_32( LoadResource16( HMODULE_16(hModule), HRSRC_16(hRsrc) ) );

    if (!hRsrc) return 0;
    if (!hModule) hModule = GetModuleHandleA( NULL );
    status = LdrAccessResource( hModule, (IMAGE_RESOURCE_DATA_ENTRY *)hRsrc, &ret, NULL );
    if (status != STATUS_SUCCESS) SetLastError( RtlNtStatusToDosError(status) );
    return ret;
}


/**********************************************************************
 *	    LockResource     (KERNEL32.@)
 */
LPVOID WINAPI LockResource( HGLOBAL handle )
{
    TRACE("(%p)\n", handle );

    if (HIWORD( handle ))  /* 32-bit memory handle */
        return (LPVOID)handle;

    /* 16-bit memory handle */
    return LockResource16( HGLOBAL_16(handle) );
}


/**********************************************************************
 *	    FreeResource     (KERNEL32.@)
 */
BOOL WINAPI FreeResource( HGLOBAL handle )
{
    if (HIWORD(handle)) return 0; /* 32-bit memory handle: nothing to do */
    return FreeResource16( HGLOBAL_16(handle) );
}


/**********************************************************************
 *	    SizeofResource   (KERNEL32.@)
 */
DWORD WINAPI SizeofResource( HINSTANCE hModule, HRSRC hRsrc )
{
    if (hModule && !HIWORD(hModule))
        return SizeofResource16( HMODULE_16(hModule), HRSRC_16(hRsrc) );

    if (!hRsrc) return 0;
    return ((PIMAGE_RESOURCE_DATA_ENTRY)hRsrc)->Size;
}
