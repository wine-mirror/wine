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

#include <stdarg.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "ntstatus.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winternl.h"
#include "wownt32.h"
#include "wine/winbase16.h"
#include "wine/debug.h"
#include "excpt.h"
#include "wine/exception.h"

WINE_DEFAULT_DEBUG_CHANNEL(resource);

/* handle conversions */
#define HRSRC_32(h16)   ((HRSRC)(ULONG_PTR)(h16))
#define HRSRC_16(h32)   (LOWORD(h32))
#define HGLOBAL_32(h16) ((HGLOBAL)(ULONG_PTR)(h16))
#define HGLOBAL_16(h32) (LOWORD(h32))
#define HMODULE_16(h32) (LOWORD(h32))

static WINE_EXCEPTION_FILTER(page_fault)
{
    if (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ||
        GetExceptionCode() == EXCEPTION_PRIV_INSTRUCTION)
        return EXCEPTION_EXECUTE_HANDLER;
    return EXCEPTION_CONTINUE_SEARCH;
}

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
        if (RtlCharToInteger( name + 1, 10, &value ) != STATUS_SUCCESS || HIWORD(value))
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
        RtlInitUnicodeString( str, name + 1 );
        if (RtlUnicodeStringToInteger( str, 10, &value ) != STATUS_SUCCESS || HIWORD(value))
            return STATUS_INVALID_PARAMETER;
        str->Buffer = (LPWSTR)value;
        return STATUS_SUCCESS;
    }
    RtlCreateUnicodeString( str, name );
    RtlUpcaseUnicodeString( str, str, FALSE );
    return STATUS_SUCCESS;
}

/* retrieve the resource names for the 16-bit FindResource function */
static BOOL get_res_name_type_WtoA( LPCWSTR name, LPCWSTR type, LPSTR *nameA, LPSTR *typeA )
{
    *nameA = *typeA = NULL;

    __TRY
    {
        if (HIWORD(name))
        {
            DWORD len = WideCharToMultiByte( CP_ACP, 0, name, -1, NULL, 0, NULL, NULL );
            *nameA = HeapAlloc( GetProcessHeap(), 0, len );
            if (*nameA) WideCharToMultiByte( CP_ACP, 0, name, -1, *nameA, len, NULL, NULL );
        }
        else *nameA = (LPSTR)name;

        if (HIWORD(type))
        {
            DWORD len = WideCharToMultiByte( CP_ACP, 0, type, -1, NULL, 0, NULL, NULL );
            *typeA = HeapAlloc( GetProcessHeap(), 0, len );
            if (*typeA) WideCharToMultiByte( CP_ACP, 0, type, -1, *typeA, len, NULL, NULL );
        }
        else *typeA = (LPSTR)type;
    }
    __EXCEPT(page_fault)
    {
        if (HIWORD(*nameA)) HeapFree( GetProcessHeap(), 0, *nameA );
        if (HIWORD(*typeA)) HeapFree( GetProcessHeap(), 0, *typeA );
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    __ENDTRY
    return TRUE;
}

/* implementation of FindResourceExA */
static HRSRC find_resourceA( HMODULE hModule, LPCSTR type, LPCSTR name, WORD lang )
{
    NTSTATUS status;
    UNICODE_STRING nameW, typeW;
    LDR_RESOURCE_INFO info;
    const IMAGE_RESOURCE_DATA_ENTRY *entry = NULL;

    __TRY
    {
        if ((status = get_res_nameA( name, &nameW )) != STATUS_SUCCESS) goto done;
        if ((status = get_res_nameA( type, &typeW )) != STATUS_SUCCESS) goto done;
        info.Type = (ULONG)typeW.Buffer;
        info.Name = (ULONG)nameW.Buffer;
        info.Language = lang;
        status = LdrFindResource_U( hModule, &info, 3, &entry );
    done:
        if (status != STATUS_SUCCESS) SetLastError( RtlNtStatusToDosError(status) );
    }
    __EXCEPT(page_fault)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
    }
    __ENDTRY

    if (HIWORD(nameW.Buffer)) HeapFree( GetProcessHeap(), 0, nameW.Buffer );
    if (HIWORD(typeW.Buffer)) HeapFree( GetProcessHeap(), 0, typeW.Buffer );
    return (HRSRC)entry;
}


/* implementation of FindResourceExW */
static HRSRC find_resourceW( HMODULE hModule, LPCWSTR type, LPCWSTR name, WORD lang )
{
    NTSTATUS status;
    UNICODE_STRING nameW, typeW;
    LDR_RESOURCE_INFO info;
    const IMAGE_RESOURCE_DATA_ENTRY *entry = NULL;

    nameW.Buffer = typeW.Buffer = NULL;

    __TRY
    {
        if ((status = get_res_nameW( name, &nameW )) != STATUS_SUCCESS) goto done;
        if ((status = get_res_nameW( type, &typeW )) != STATUS_SUCCESS) goto done;
        info.Type = (ULONG)typeW.Buffer;
        info.Name = (ULONG)nameW.Buffer;
        info.Language = lang;
        status = LdrFindResource_U( hModule, &info, 3, &entry );
    done:
        if (status != STATUS_SUCCESS) SetLastError( RtlNtStatusToDosError(status) );
    }
    __EXCEPT(page_fault)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
    }
    __ENDTRY

    if (HIWORD(nameW.Buffer)) HeapFree( GetProcessHeap(), 0, nameW.Buffer );
    if (HIWORD(typeW.Buffer)) HeapFree( GetProcessHeap(), 0, typeW.Buffer );
    return (HRSRC)entry;
}

/**********************************************************************
 *	    FindResourceExA  (KERNEL32.@)
 */
HRSRC WINAPI FindResourceExA( HMODULE hModule, LPCSTR type, LPCSTR name, WORD lang )
{
    TRACE( "%p %s %s %04x\n", hModule, debugstr_a(type), debugstr_a(name), lang );

    if (!hModule) hModule = GetModuleHandleW(0);
    else if (!HIWORD(hModule))
    {
        return HRSRC_32( FindResource16( HMODULE_16(hModule), name, type ) );
    }
    return find_resourceA( hModule, type, name, lang );
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
    TRACE( "%p %s %s %04x\n", hModule, debugstr_w(type), debugstr_w(name), lang );

    if (!hModule) hModule = GetModuleHandleW(0);
    else if (!HIWORD(hModule))
    {
        LPSTR nameA, typeA;
        HRSRC16 ret;

        if (!get_res_name_type_WtoA( name, type, &nameA, &typeA )) return NULL;

        ret = FindResource16( HMODULE_16(hModule), nameA, typeA );
        if (HIWORD(nameA)) HeapFree( GetProcessHeap(), 0, nameA );
        if (HIWORD(typeA)) HeapFree( GetProcessHeap(), 0, typeA );
        return HRSRC_32(ret);
    }

    return find_resourceW( hModule, type, name, lang );
}


/**********************************************************************
 *	    FindResourceW    (KERNEL32.@)
 */
HRSRC WINAPI FindResourceW( HINSTANCE hModule, LPCWSTR name, LPCWSTR type )
{
    return FindResourceExW( hModule, type, name, MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL ) );
}


/**********************************************************************
 *	EnumResourceTypesA	(KERNEL32.@)
 */
BOOL WINAPI EnumResourceTypesA( HMODULE hmod, ENUMRESTYPEPROCA lpfun, LONG_PTR lparam )
{
    int i;
    BOOL ret = FALSE;
    LPSTR type = NULL;
    DWORD len = 0, newlen;
    NTSTATUS status;
    const IMAGE_RESOURCE_DIRECTORY *resdir;
    const IMAGE_RESOURCE_DIRECTORY_ENTRY *et;
    const IMAGE_RESOURCE_DIR_STRING_U *str;

    TRACE( "%p %p %lx\n", hmod, lpfun, lparam );

    if (!hmod) hmod = GetModuleHandleA( NULL );

    if ((status = LdrFindResourceDirectory_U( hmod, NULL, 0, &resdir )) != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }
    et = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(resdir + 1);
    for (i = 0; i < resdir->NumberOfNamedEntries+resdir->NumberOfIdEntries; i++)
    {
        if (et[i].u1.s1.NameIsString)
        {
            str = (PIMAGE_RESOURCE_DIR_STRING_U) ((LPBYTE) resdir + et[i].u1.s1.NameOffset);
            newlen = WideCharToMultiByte( CP_ACP, 0, str->NameString, str->Length, NULL, 0, NULL, NULL);
            if (newlen + 1 > len)
            {
                len = newlen + 1;
                if (type) HeapFree( GetProcessHeap(), 0, type );
                if (!(type = HeapAlloc( GetProcessHeap(), 0, len ))) return FALSE;
            }
            WideCharToMultiByte( CP_ACP, 0, str->NameString, str->Length, type, len, NULL, NULL);
            type[newlen] = 0;
            ret = lpfun(hmod,type,lparam);
        }
        else
        {
            ret = lpfun( hmod, (LPSTR)(int)et[i].u1.s2.Id, lparam );
        }
        if (!ret) break;
    }
    if (type) HeapFree( GetProcessHeap(), 0, type );
    return ret;
}


/**********************************************************************
 *	EnumResourceTypesW	(KERNEL32.@)
 */
BOOL WINAPI EnumResourceTypesW( HMODULE hmod, ENUMRESTYPEPROCW lpfun, LONG_PTR lparam )
{
    int i;
    BOOL ret = FALSE;
    DWORD len = 0;
    LPWSTR type = NULL;
    NTSTATUS status;
    const IMAGE_RESOURCE_DIRECTORY *resdir;
    const IMAGE_RESOURCE_DIRECTORY_ENTRY *et;
    const IMAGE_RESOURCE_DIR_STRING_U *str;

    TRACE( "%p %p %lx\n", hmod, lpfun, lparam );

    if (!hmod) hmod = GetModuleHandleW( NULL );

    if ((status = LdrFindResourceDirectory_U( hmod, NULL, 0, &resdir )) != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }
    et = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(resdir + 1);
    for (i = 0; i < resdir->NumberOfNamedEntries + resdir->NumberOfIdEntries; i++)
    {
        if (et[i].u1.s1.NameIsString)
        {
            str = (PIMAGE_RESOURCE_DIR_STRING_U) ((LPBYTE) resdir + et[i].u1.s1.NameOffset);
            if (str->Length + 1 > len)
            {
                len = str->Length + 1;
                if (type) HeapFree( GetProcessHeap(), 0, type );
                if (!(type = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) ))) return FALSE;
            }
            memcpy(type, str->NameString, str->Length * sizeof (WCHAR));
            type[str->Length] = 0;
            ret = lpfun(hmod,type,lparam);
        }
        else
        {
            ret = lpfun( hmod, (LPWSTR)(int)et[i].u1.s2.Id, lparam );
        }
        if (!ret) break;
    }
    if (type) HeapFree( GetProcessHeap(), 0, type );
    return ret;
}


/**********************************************************************
 *	EnumResourceNamesA	(KERNEL32.@)
 */
BOOL WINAPI EnumResourceNamesA( HMODULE hmod, LPCSTR type, ENUMRESNAMEPROCA lpfun, LONG_PTR lparam )
{
    int i;
    BOOL ret = FALSE;
    DWORD len = 0, newlen;
    LPSTR name = NULL;
    NTSTATUS status;
    UNICODE_STRING typeW;
    LDR_RESOURCE_INFO info;
    const IMAGE_RESOURCE_DIRECTORY *basedir, *resdir;
    const IMAGE_RESOURCE_DIRECTORY_ENTRY *et;
    const IMAGE_RESOURCE_DIR_STRING_U *str;

    TRACE( "%p %s %p %lx\n", hmod, debugstr_a(type), lpfun, lparam );

    if (!hmod) hmod = GetModuleHandleA( NULL );
    typeW.Buffer = NULL;
    if ((status = LdrFindResourceDirectory_U( hmod, NULL, 0, &basedir )) != STATUS_SUCCESS)
        goto done;
    if ((status = get_res_nameA( type, &typeW )) != STATUS_SUCCESS)
        goto done;
    info.Type = (ULONG)typeW.Buffer;
    if ((status = LdrFindResourceDirectory_U( hmod, &info, 1, &resdir )) != STATUS_SUCCESS)
        goto done;

    et = (IMAGE_RESOURCE_DIRECTORY_ENTRY *)(resdir + 1);
    for (i = 0; i < resdir->NumberOfNamedEntries+resdir->NumberOfIdEntries; i++)
    {
        if (et[i].u1.s1.NameIsString)
        {
            str = (IMAGE_RESOURCE_DIR_STRING_U *) ((LPBYTE) basedir + et[i].u1.s1.NameOffset);
            newlen = WideCharToMultiByte(CP_ACP, 0, str->NameString, str->Length, NULL, 0, NULL, NULL);
            if (newlen + 1 > len)
            {
                len = newlen + 1;
                if (name) HeapFree( GetProcessHeap(), 0, name );
                if (!(name = HeapAlloc(GetProcessHeap(), 0, len + 1 )))
                {
                    ret = FALSE;
                    break;
                }
            }
            WideCharToMultiByte( CP_ACP, 0, str->NameString, str->Length, name, len, NULL, NULL );
            name[newlen] = 0;
            ret = lpfun(hmod,type,name,lparam);
        }
        else
        {
            ret = lpfun( hmod, type, (LPSTR)(int)et[i].u1.s2.Id, lparam );
        }
        if (!ret) break;
    }
done:
    if (name) HeapFree( GetProcessHeap(), 0, name );
    if (HIWORD(typeW.Buffer)) HeapFree( GetProcessHeap(), 0, typeW.Buffer );
    if (status != STATUS_SUCCESS) SetLastError( RtlNtStatusToDosError(status) );
    return ret;
}


/**********************************************************************
 *	EnumResourceNamesW	(KERNEL32.@)
 */
BOOL WINAPI EnumResourceNamesW( HMODULE hmod, LPCWSTR type, ENUMRESNAMEPROCW lpfun, LONG_PTR lparam )
{
    int i;
    BOOL ret = FALSE;
    LPWSTR name = NULL;
    DWORD len = 0;
    NTSTATUS status;
    UNICODE_STRING typeW;
    LDR_RESOURCE_INFO info;
    const IMAGE_RESOURCE_DIRECTORY *basedir, *resdir;
    const IMAGE_RESOURCE_DIRECTORY_ENTRY *et;
    const IMAGE_RESOURCE_DIR_STRING_U *str;

    TRACE( "%p %s %p %lx\n", hmod, debugstr_w(type), lpfun, lparam );

    if (!hmod) hmod = GetModuleHandleW( NULL );
    typeW.Buffer = NULL;
    if ((status = LdrFindResourceDirectory_U( hmod, NULL, 0, &basedir )) != STATUS_SUCCESS)
        goto done;
    if ((status = get_res_nameW( type, &typeW )) != STATUS_SUCCESS)
        goto done;
    info.Type = (ULONG)typeW.Buffer;
    if ((status = LdrFindResourceDirectory_U( hmod, &info, 1, &resdir )) != STATUS_SUCCESS)
        goto done;

    et = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(resdir + 1);
    for (i = 0; i < resdir->NumberOfNamedEntries+resdir->NumberOfIdEntries; i++)
    {
        if (et[i].u1.s1.NameIsString)
        {
            str = (IMAGE_RESOURCE_DIR_STRING_U *) ((LPBYTE) basedir + et[i].u1.s1.NameOffset);
            if (str->Length + 1 > len)
            {
                len = str->Length + 1;
                if (name) HeapFree( GetProcessHeap(), 0, name );
                if (!(name = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) )))
                {
                    ret = FALSE;
                    break;
                }
            }
            memcpy(name, str->NameString, str->Length * sizeof (WCHAR));
            name[str->Length] = 0;
            ret = lpfun(hmod,type,name,lparam);
        }
        else
        {
            ret = lpfun( hmod, type, (LPWSTR)(int)et[i].u1.s2.Id, lparam );
        }
        if (!ret) break;
    }
done:
    if (name) HeapFree( GetProcessHeap(), 0, name );
    if (HIWORD(typeW.Buffer)) HeapFree( GetProcessHeap(), 0, typeW.Buffer );
    if (status != STATUS_SUCCESS) SetLastError( RtlNtStatusToDosError(status) );
    return ret;
}


/**********************************************************************
 *	EnumResourceLanguagesA	(KERNEL32.@)
 */
BOOL WINAPI EnumResourceLanguagesA( HMODULE hmod, LPCSTR type, LPCSTR name,
                                    ENUMRESLANGPROCA lpfun, LONG_PTR lparam )
{
    int i;
    BOOL ret = FALSE;
    NTSTATUS status;
    UNICODE_STRING typeW, nameW;
    LDR_RESOURCE_INFO info;
    const IMAGE_RESOURCE_DIRECTORY *basedir, *resdir;
    const IMAGE_RESOURCE_DIRECTORY_ENTRY *et;

    TRACE( "%p %s %s %p %lx\n", hmod, debugstr_a(type), debugstr_a(name), lpfun, lparam );

    if (!hmod) hmod = GetModuleHandleA( NULL );
    typeW.Buffer = nameW.Buffer = NULL;
    if ((status = LdrFindResourceDirectory_U( hmod, NULL, 0, &basedir )) != STATUS_SUCCESS)
        goto done;
    if ((status = get_res_nameA( type, &typeW )) != STATUS_SUCCESS)
        goto done;
    if ((status = get_res_nameA( name, &nameW )) != STATUS_SUCCESS)
        goto done;
    info.Type = (ULONG)typeW.Buffer;
    info.Name = (ULONG)nameW.Buffer;
    if ((status = LdrFindResourceDirectory_U( hmod, &info, 2, &resdir )) != STATUS_SUCCESS)
        goto done;

    et = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(resdir + 1);
    for (i = 0; i < resdir->NumberOfNamedEntries + resdir->NumberOfIdEntries; i++)
    {
        ret = lpfun( hmod, type, name, et[i].u1.s2.Id, lparam );
        if (!ret) break;
    }
done:
    if (HIWORD(typeW.Buffer)) HeapFree( GetProcessHeap(), 0, typeW.Buffer );
    if (HIWORD(nameW.Buffer)) HeapFree( GetProcessHeap(), 0, nameW.Buffer );
    if (status != STATUS_SUCCESS) SetLastError( RtlNtStatusToDosError(status) );
    return ret;
}


/**********************************************************************
 *	EnumResourceLanguagesW	(KERNEL32.@)
 */
BOOL WINAPI EnumResourceLanguagesW( HMODULE hmod, LPCWSTR type, LPCWSTR name,
                                    ENUMRESLANGPROCW lpfun, LONG_PTR lparam )
{
    int i;
    BOOL ret = FALSE;
    NTSTATUS status;
    UNICODE_STRING typeW, nameW;
    LDR_RESOURCE_INFO info;
    const IMAGE_RESOURCE_DIRECTORY *basedir, *resdir;
    const IMAGE_RESOURCE_DIRECTORY_ENTRY *et;

    TRACE( "%p %s %s %p %lx\n", hmod, debugstr_w(type), debugstr_w(name), lpfun, lparam );

    if (!hmod) hmod = GetModuleHandleW( NULL );
    typeW.Buffer = nameW.Buffer = NULL;
    if ((status = LdrFindResourceDirectory_U( hmod, NULL, 0, &basedir )) != STATUS_SUCCESS)
        goto done;
    if ((status = get_res_nameW( type, &typeW )) != STATUS_SUCCESS)
        goto done;
    if ((status = get_res_nameW( name, &nameW )) != STATUS_SUCCESS)
        goto done;
    info.Type = (ULONG)typeW.Buffer;
    info.Name = (ULONG)nameW.Buffer;
    if ((status = LdrFindResourceDirectory_U( hmod, &info, 2, &resdir )) != STATUS_SUCCESS)
        goto done;

    et = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(resdir + 1);
    for (i = 0; i < resdir->NumberOfNamedEntries + resdir->NumberOfIdEntries; i++)
    {
        ret = lpfun( hmod, type, name, et[i].u1.s2.Id, lparam );
        if (!ret) break;
    }
done:
    if (HIWORD(typeW.Buffer)) HeapFree( GetProcessHeap(), 0, typeW.Buffer );
    if (HIWORD(nameW.Buffer)) HeapFree( GetProcessHeap(), 0, nameW.Buffer );
    if (status != STATUS_SUCCESS) SetLastError( RtlNtStatusToDosError(status) );
    return ret;
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


/***********************************************************************
 *          BeginUpdateResourceA                 (KERNEL32.@)
 */
HANDLE WINAPI BeginUpdateResourceA( LPCSTR pFileName, BOOL bDeleteExistingResources )
{
  FIXME("(%s,%d): stub\n",debugstr_a(pFileName),bDeleteExistingResources);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}


/***********************************************************************
 *          BeginUpdateResourceW                 (KERNEL32.@)
 */
HANDLE WINAPI BeginUpdateResourceW( LPCWSTR pFileName, BOOL bDeleteExistingResources )
{
  FIXME("(%s,%d): stub\n",debugstr_w(pFileName),bDeleteExistingResources);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}


/***********************************************************************
 *          EndUpdateResourceA                 (KERNEL32.@)
 */
BOOL WINAPI EndUpdateResourceA( HANDLE hUpdate, BOOL fDiscard )
{
  FIXME("(%p,%d): stub\n",hUpdate, fDiscard);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


/***********************************************************************
 *          EndUpdateResourceW                 (KERNEL32.@)
 */
BOOL WINAPI EndUpdateResourceW( HANDLE hUpdate, BOOL fDiscard )
{
  FIXME("(%p,%d): stub\n",hUpdate, fDiscard);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


/***********************************************************************
 *           UpdateResourceA                 (KERNEL32.@)
 */
BOOL WINAPI UpdateResourceA(
  HANDLE  hUpdate,
  LPCSTR  lpType,
  LPCSTR  lpName,
  WORD    wLanguage,
  LPVOID  lpData,
  DWORD   cbData)
{
  FIXME(": stub\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


/***********************************************************************
 *           UpdateResourceW                 (KERNEL32.@)
 */
BOOL WINAPI UpdateResourceW(
  HANDLE  hUpdate,
  LPCWSTR lpType,
  LPCWSTR lpName,
  WORD    wLanguage,
  LPVOID  lpData,
  DWORD   cbData)
{
  FIXME(": stub\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}
