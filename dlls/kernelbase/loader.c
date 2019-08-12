/*
 * Module loader
 *
 * Copyright 1993 Robert J. Amstadt
 * Copyright 2006 Mike McCormack
 * Copyright 1995, 2003, 2019 Alexandre Julliard
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

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winternl.h"
#include "kernelbase.h"
#include "wine/debug.h"
#include "wine/exception.h"

WINE_DEFAULT_DEBUG_CHANNEL(module);


/***********************************************************************
 * Modules
 ***********************************************************************/


/****************************************************************************
 *	DisableThreadLibraryCalls   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH DisableThreadLibraryCalls( HMODULE module )
{
    return set_ntstatus( LdrDisableThreadCalloutsForDll( module ));
}


/***********************************************************************
 *	GetModuleFileNameA   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetModuleFileNameA( HMODULE module, LPSTR filename, DWORD size )
{
    LPWSTR filenameW = HeapAlloc( GetProcessHeap(), 0, size * sizeof(WCHAR) );
    DWORD len;

    if (!filenameW)
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return 0;
    }
    if ((len = GetModuleFileNameW( module, filenameW, size )))
    {
    	len = file_name_WtoA( filenameW, len, filename, size );
        if (len < size)
            filename[len] = 0;
        else
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
    }
    HeapFree( GetProcessHeap(), 0, filenameW );
    return len;
}


/***********************************************************************
 *	GetModuleFileNameW   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetModuleFileNameW( HMODULE module, LPWSTR filename, DWORD size )
{
    ULONG len = 0;
    ULONG_PTR magic;
    LDR_MODULE *pldr;
    WIN16_SUBSYSTEM_TIB *win16_tib;

    if (!module && ((win16_tib = NtCurrentTeb()->Tib.SubSystemTib)) && win16_tib->exe_name)
    {
        len = min( size, win16_tib->exe_name->Length / sizeof(WCHAR) );
        memcpy( filename, win16_tib->exe_name->Buffer, len * sizeof(WCHAR) );
        if (len < size) filename[len] = 0;
        goto done;
    }

    LdrLockLoaderLock( 0, NULL, &magic );

    if (!module) module = NtCurrentTeb()->Peb->ImageBaseAddress;
    if (set_ntstatus( LdrFindEntryForAddress( module, &pldr )))
    {
        len = min( size, pldr->FullDllName.Length / sizeof(WCHAR) );
        memcpy( filename, pldr->FullDllName.Buffer, len * sizeof(WCHAR) );
        if (len < size)
        {
            filename[len] = 0;
            SetLastError( 0 );
        }
        else SetLastError( ERROR_INSUFFICIENT_BUFFER );
    }

    LdrUnlockLoaderLock( 0, magic );
done:
    TRACE( "%s\n", debugstr_wn(filename, len) );
    return len;
}


/***********************************************************************
 *	GetModuleHandleA   (kernelbase.@)
 */
HMODULE WINAPI DECLSPEC_HOTPATCH GetModuleHandleA( LPCSTR module )
{
    HMODULE ret;

    GetModuleHandleExA( GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, module, &ret );
    return ret;
}


/***********************************************************************
 *	GetModuleHandleW   (kernelbase.@)
 */
HMODULE WINAPI DECLSPEC_HOTPATCH GetModuleHandleW( LPCWSTR module )
{
    HMODULE ret;

    GetModuleHandleExW( GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, module, &ret );
    return ret;
}


/***********************************************************************
 *	GetModuleHandleExA   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetModuleHandleExA( DWORD flags, LPCSTR name, HMODULE *module )
{
    WCHAR *nameW;

    if (!name || (flags & GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS))
        return GetModuleHandleExW( flags, (LPCWSTR)name, module );

    if (!(nameW = file_name_AtoW( name, FALSE ))) return FALSE;
    return GetModuleHandleExW( flags, nameW, module );
}


/***********************************************************************
 *	GetModuleHandleExW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetModuleHandleExW( DWORD flags, LPCWSTR name, HMODULE *module )
{
    NTSTATUS status = STATUS_SUCCESS;
    HMODULE ret = NULL;
    ULONG_PTR magic;
    BOOL lock;

    if (!module)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    /* if we are messing with the refcount, grab the loader lock */
    lock = (flags & GET_MODULE_HANDLE_EX_FLAG_PIN) || !(flags & GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT);
    if (lock) LdrLockLoaderLock( 0, NULL, &magic );

    if (!name)
    {
        ret = NtCurrentTeb()->Peb->ImageBaseAddress;
    }
    else if (flags & GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS)
    {
        void *dummy;
        if (!(ret = RtlPcToFileHeader( (void *)name, &dummy ))) status = STATUS_DLL_NOT_FOUND;
    }
    else
    {
        UNICODE_STRING wstr;
        RtlInitUnicodeString( &wstr, name );
        status = LdrGetDllHandle( NULL, 0, &wstr, &ret );
    }

    if (status == STATUS_SUCCESS)
    {
        if (flags & GET_MODULE_HANDLE_EX_FLAG_PIN)
            LdrAddRefDll( LDR_ADDREF_DLL_PIN, ret );
        else if (!(flags & GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT))
            LdrAddRefDll( 0, ret );
    }

    if (lock) LdrUnlockLoaderLock( 0, magic );

    *module = ret;
    return set_ntstatus( status );
}


/***********************************************************************
 * Resources
 ***********************************************************************/


#define IS_INTRESOURCE(x)   (((ULONG_PTR)(x) >> 16) == 0)

/* retrieve the resource name to pass to the ntdll functions */
static NTSTATUS get_res_nameA( LPCSTR name, UNICODE_STRING *str )
{
    if (IS_INTRESOURCE(name))
    {
        str->Buffer = ULongToPtr( LOWORD(name) );
        return STATUS_SUCCESS;
    }
    if (name[0] == '#')
    {
        ULONG value;
        if (RtlCharToInteger( name + 1, 10, &value ) != STATUS_SUCCESS || HIWORD(value))
            return STATUS_INVALID_PARAMETER;
        str->Buffer = ULongToPtr(value);
        return STATUS_SUCCESS;
    }
    RtlCreateUnicodeStringFromAsciiz( str, name );
    RtlUpcaseUnicodeString( str, str, FALSE );
    return STATUS_SUCCESS;
}

/* retrieve the resource name to pass to the ntdll functions */
static NTSTATUS get_res_nameW( LPCWSTR name, UNICODE_STRING *str )
{
    if (IS_INTRESOURCE(name))
    {
        str->Buffer = ULongToPtr( LOWORD(name) );
        return STATUS_SUCCESS;
    }
    if (name[0] == '#')
    {
        ULONG value;
        RtlInitUnicodeString( str, name + 1 );
        if (RtlUnicodeStringToInteger( str, 10, &value ) != STATUS_SUCCESS || HIWORD(value))
            return STATUS_INVALID_PARAMETER;
        str->Buffer = ULongToPtr(value);
        return STATUS_SUCCESS;
    }
    RtlCreateUnicodeString( str, name );
    RtlUpcaseUnicodeString( str, str, FALSE );
    return STATUS_SUCCESS;
}


/**********************************************************************
 *	EnumResourceLanguagesExA	(kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumResourceLanguagesExA( HMODULE module, LPCSTR type, LPCSTR name,
                                                        ENUMRESLANGPROCA func, LONG_PTR param,
                                                        DWORD flags, LANGID lang )
{
    int i;
    BOOL ret = FALSE;
    NTSTATUS status;
    UNICODE_STRING typeW, nameW;
    LDR_RESOURCE_INFO info;
    const IMAGE_RESOURCE_DIRECTORY *basedir, *resdir;
    const IMAGE_RESOURCE_DIRECTORY_ENTRY *et;

    TRACE( "%p %s %s %p %lx %x %d\n", module, debugstr_a(type), debugstr_a(name),
           func, param, flags, lang );

    if (flags & (RESOURCE_ENUM_MUI | RESOURCE_ENUM_MUI_SYSTEM | RESOURCE_ENUM_VALIDATE))
        FIXME( "unimplemented flags: %x\n", flags );

    if (!flags) flags = RESOURCE_ENUM_LN | RESOURCE_ENUM_MUI;
    if (!(flags & RESOURCE_ENUM_LN)) return ret;

    if (!module) module = GetModuleHandleW( 0 );
    typeW.Buffer = nameW.Buffer = NULL;
    if ((status = LdrFindResourceDirectory_U( module, NULL, 0, &basedir )) != STATUS_SUCCESS)
        goto done;
    if ((status = get_res_nameA( type, &typeW )) != STATUS_SUCCESS)
        goto done;
    if ((status = get_res_nameA( name, &nameW )) != STATUS_SUCCESS)
        goto done;
    info.Type = (ULONG_PTR)typeW.Buffer;
    info.Name = (ULONG_PTR)nameW.Buffer;
    if ((status = LdrFindResourceDirectory_U( module, &info, 2, &resdir )) != STATUS_SUCCESS)
        goto done;

    et = (const IMAGE_RESOURCE_DIRECTORY_ENTRY *)(resdir + 1);
    __TRY
    {
        for (i = 0; i < resdir->NumberOfNamedEntries + resdir->NumberOfIdEntries; i++)
        {
            ret = func( module, type, name, et[i].u.Id, param );
            if (!ret) break;
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        ret = FALSE;
        status = STATUS_ACCESS_VIOLATION;
    }
    __ENDTRY
done:
    if (!IS_INTRESOURCE(typeW.Buffer)) HeapFree( GetProcessHeap(), 0, typeW.Buffer );
    if (!IS_INTRESOURCE(nameW.Buffer)) HeapFree( GetProcessHeap(), 0, nameW.Buffer );
    if (status != STATUS_SUCCESS) SetLastError( RtlNtStatusToDosError(status) );
    return ret;
}


/**********************************************************************
 *	EnumResourceLanguagesExW	(kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumResourceLanguagesExW( HMODULE module, LPCWSTR type, LPCWSTR name,
                                                        ENUMRESLANGPROCW func, LONG_PTR param,
                                                        DWORD flags, LANGID lang )
{
    int i;
    BOOL ret = FALSE;
    NTSTATUS status;
    UNICODE_STRING typeW, nameW;
    LDR_RESOURCE_INFO info;
    const IMAGE_RESOURCE_DIRECTORY *basedir, *resdir;
    const IMAGE_RESOURCE_DIRECTORY_ENTRY *et;

    TRACE( "%p %s %s %p %lx %x %d\n", module, debugstr_w(type), debugstr_w(name),
           func, param, flags, lang );

    if (flags & (RESOURCE_ENUM_MUI | RESOURCE_ENUM_MUI_SYSTEM | RESOURCE_ENUM_VALIDATE))
        FIXME( "unimplemented flags: %x\n", flags );

    if (!flags) flags = RESOURCE_ENUM_LN | RESOURCE_ENUM_MUI;
    if (!(flags & RESOURCE_ENUM_LN)) return ret;

    if (!module) module = GetModuleHandleW( 0 );
    typeW.Buffer = nameW.Buffer = NULL;
    if ((status = LdrFindResourceDirectory_U( module, NULL, 0, &basedir )) != STATUS_SUCCESS)
        goto done;
    if ((status = get_res_nameW( type, &typeW )) != STATUS_SUCCESS)
        goto done;
    if ((status = get_res_nameW( name, &nameW )) != STATUS_SUCCESS)
        goto done;
    info.Type = (ULONG_PTR)typeW.Buffer;
    info.Name = (ULONG_PTR)nameW.Buffer;
    if ((status = LdrFindResourceDirectory_U( module, &info, 2, &resdir )) != STATUS_SUCCESS)
        goto done;

    et = (const IMAGE_RESOURCE_DIRECTORY_ENTRY *)(resdir + 1);
    __TRY
    {
        for (i = 0; i < resdir->NumberOfNamedEntries + resdir->NumberOfIdEntries; i++)
        {
            ret = func( module, type, name, et[i].u.Id, param );
            if (!ret) break;
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        ret = FALSE;
        status = STATUS_ACCESS_VIOLATION;
    }
    __ENDTRY
done:
    if (!IS_INTRESOURCE(typeW.Buffer)) HeapFree( GetProcessHeap(), 0, typeW.Buffer );
    if (!IS_INTRESOURCE(nameW.Buffer)) HeapFree( GetProcessHeap(), 0, nameW.Buffer );
    if (status != STATUS_SUCCESS) SetLastError( RtlNtStatusToDosError(status) );
    return ret;
}


/**********************************************************************
 *	EnumResourceNamesExA	(kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumResourceNamesExA( HMODULE module, LPCSTR type, ENUMRESNAMEPROCA func,
                                                    LONG_PTR param, DWORD flags, LANGID lang )
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

    TRACE( "%p %s %p %lx\n", module, debugstr_a(type), func, param );

    if (flags & (RESOURCE_ENUM_MUI | RESOURCE_ENUM_MUI_SYSTEM | RESOURCE_ENUM_VALIDATE))
        FIXME( "unimplemented flags: %x\n", flags );

    if (!flags) flags = RESOURCE_ENUM_LN | RESOURCE_ENUM_MUI;
    if (!(flags & RESOURCE_ENUM_LN)) return ret;

    if (!module) module = GetModuleHandleW( 0 );
    typeW.Buffer = NULL;
    if ((status = LdrFindResourceDirectory_U( module, NULL, 0, &basedir )) != STATUS_SUCCESS)
        goto done;
    if ((status = get_res_nameA( type, &typeW )) != STATUS_SUCCESS)
        goto done;
    info.Type = (ULONG_PTR)typeW.Buffer;
    if ((status = LdrFindResourceDirectory_U( module, &info, 1, &resdir )) != STATUS_SUCCESS)
        goto done;

    et = (const IMAGE_RESOURCE_DIRECTORY_ENTRY *)(resdir + 1);
    __TRY
    {
        for (i = 0; i < resdir->NumberOfNamedEntries+resdir->NumberOfIdEntries; i++)
        {
            if (et[i].u.s.NameIsString)
            {
                str = (const IMAGE_RESOURCE_DIR_STRING_U *)((const BYTE *)basedir + et[i].u.s.NameOffset);
                newlen = WideCharToMultiByte(CP_ACP, 0, str->NameString, str->Length, NULL, 0, NULL, NULL);
                if (newlen + 1 > len)
                {
                    len = newlen + 1;
                    HeapFree( GetProcessHeap(), 0, name );
                    if (!(name = HeapAlloc( GetProcessHeap(), 0, len + 1 )))
                    {
                        ret = FALSE;
                        break;
                    }
                }
                WideCharToMultiByte( CP_ACP, 0, str->NameString, str->Length, name, len, NULL, NULL );
                name[newlen] = 0;
                ret = func( module, type, name, param );
            }
            else
            {
                ret = func( module, type, UIntToPtr(et[i].u.Id), param );
            }
            if (!ret) break;
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        ret = FALSE;
        status = STATUS_ACCESS_VIOLATION;
    }
    __ENDTRY

done:
    HeapFree( GetProcessHeap(), 0, name );
    if (!IS_INTRESOURCE(typeW.Buffer)) HeapFree( GetProcessHeap(), 0, typeW.Buffer );
    if (status != STATUS_SUCCESS) SetLastError( RtlNtStatusToDosError(status) );
    return ret;
}


/**********************************************************************
 *	EnumResourceNamesExW	(kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumResourceNamesExW( HMODULE module, LPCWSTR type, ENUMRESNAMEPROCW func,
                                                    LONG_PTR param, DWORD flags, LANGID lang )
{
    int i, len = 0;
    BOOL ret = FALSE;
    LPWSTR name = NULL;
    NTSTATUS status;
    UNICODE_STRING typeW;
    LDR_RESOURCE_INFO info;
    const IMAGE_RESOURCE_DIRECTORY *basedir, *resdir;
    const IMAGE_RESOURCE_DIRECTORY_ENTRY *et;
    const IMAGE_RESOURCE_DIR_STRING_U *str;

    TRACE( "%p %s %p %lx\n", module, debugstr_w(type), func, param );

    if (flags & (RESOURCE_ENUM_MUI | RESOURCE_ENUM_MUI_SYSTEM | RESOURCE_ENUM_VALIDATE))
        FIXME( "unimplemented flags: %x\n", flags );

    if (!flags) flags = RESOURCE_ENUM_LN | RESOURCE_ENUM_MUI;
    if (!(flags & RESOURCE_ENUM_LN)) return ret;

    if (!module) module = GetModuleHandleW( 0 );
    typeW.Buffer = NULL;
    if ((status = LdrFindResourceDirectory_U( module, NULL, 0, &basedir )) != STATUS_SUCCESS)
        goto done;
    if ((status = get_res_nameW( type, &typeW )) != STATUS_SUCCESS)
        goto done;
    info.Type = (ULONG_PTR)typeW.Buffer;
    if ((status = LdrFindResourceDirectory_U( module, &info, 1, &resdir )) != STATUS_SUCCESS)
        goto done;

    et = (const IMAGE_RESOURCE_DIRECTORY_ENTRY *)(resdir + 1);
    __TRY
    {
        for (i = 0; i < resdir->NumberOfNamedEntries+resdir->NumberOfIdEntries; i++)
        {
            if (et[i].u.s.NameIsString)
            {
                str = (const IMAGE_RESOURCE_DIR_STRING_U *)((const BYTE *)basedir + et[i].u.s.NameOffset);
                if (str->Length + 1 > len)
                {
                    len = str->Length + 1;
                    HeapFree( GetProcessHeap(), 0, name );
                    if (!(name = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) )))
                    {
                        ret = FALSE;
                        break;
                    }
                }
                memcpy(name, str->NameString, str->Length * sizeof (WCHAR));
                name[str->Length] = 0;
                ret = func( module, type, name, param );
            }
            else
            {
                ret = func( module, type, UIntToPtr(et[i].u.Id), param );
            }
            if (!ret) break;
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        ret = FALSE;
        status = STATUS_ACCESS_VIOLATION;
    }
    __ENDTRY
done:
    HeapFree( GetProcessHeap(), 0, name );
    if (!IS_INTRESOURCE(typeW.Buffer)) HeapFree( GetProcessHeap(), 0, typeW.Buffer );
    if (status != STATUS_SUCCESS) SetLastError( RtlNtStatusToDosError(status) );
    return ret;
}


/**********************************************************************
 *	EnumResourceNamesW	(kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumResourceNamesW( HMODULE module, LPCWSTR type,
                                                  ENUMRESNAMEPROCW func, LONG_PTR param )
{
    return EnumResourceNamesExW( module, type, func, param, 0, 0 );
}


/**********************************************************************
 *	EnumResourceTypesExA	(kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumResourceTypesExA( HMODULE module, ENUMRESTYPEPROCA func, LONG_PTR param,
                                                    DWORD flags, LANGID lang )
{
    int i;
    BOOL ret = FALSE;
    LPSTR type = NULL;
    DWORD len = 0, newlen;
    NTSTATUS status;
    const IMAGE_RESOURCE_DIRECTORY *resdir;
    const IMAGE_RESOURCE_DIRECTORY_ENTRY *et;
    const IMAGE_RESOURCE_DIR_STRING_U *str;

    TRACE( "%p %p %lx\n", module, func, param );

    if (flags & (RESOURCE_ENUM_MUI | RESOURCE_ENUM_MUI_SYSTEM | RESOURCE_ENUM_VALIDATE))
        FIXME( "unimplemented flags: %x\n", flags );

    if (!flags) flags = RESOURCE_ENUM_LN | RESOURCE_ENUM_MUI;
    if (!(flags & RESOURCE_ENUM_LN)) return ret;

    if (!module) module = GetModuleHandleW( 0 );

    if ((status = LdrFindResourceDirectory_U( module, NULL, 0, &resdir )) != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }
    et = (const IMAGE_RESOURCE_DIRECTORY_ENTRY *)(resdir + 1);
    for (i = 0; i < resdir->NumberOfNamedEntries+resdir->NumberOfIdEntries; i++)
    {
        if (et[i].u.s.NameIsString)
        {
            str = (const IMAGE_RESOURCE_DIR_STRING_U *)((const BYTE *)resdir + et[i].u.s.NameOffset);
            newlen = WideCharToMultiByte( CP_ACP, 0, str->NameString, str->Length, NULL, 0, NULL, NULL);
            if (newlen + 1 > len)
            {
                len = newlen + 1;
                HeapFree( GetProcessHeap(), 0, type );
                if (!(type = HeapAlloc( GetProcessHeap(), 0, len ))) return FALSE;
            }
            WideCharToMultiByte( CP_ACP, 0, str->NameString, str->Length, type, len, NULL, NULL);
            type[newlen] = 0;
            ret = func( module, type, param );
        }
        else
        {
            ret = func( module, UIntToPtr(et[i].u.Id), param );
        }
        if (!ret) break;
    }
    HeapFree( GetProcessHeap(), 0, type );
    return ret;
}


/**********************************************************************
 *	EnumResourceTypesExW	(kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumResourceTypesExW( HMODULE module, ENUMRESTYPEPROCW func, LONG_PTR param,
                                                    DWORD flags, LANGID lang )
{
    int i, len = 0;
    BOOL ret = FALSE;
    LPWSTR type = NULL;
    NTSTATUS status;
    const IMAGE_RESOURCE_DIRECTORY *resdir;
    const IMAGE_RESOURCE_DIRECTORY_ENTRY *et;
    const IMAGE_RESOURCE_DIR_STRING_U *str;

    TRACE( "%p %p %lx\n", module, func, param );

    if (!flags) flags = RESOURCE_ENUM_LN | RESOURCE_ENUM_MUI;
    if (!(flags & RESOURCE_ENUM_LN)) return ret;

    if (!module) module = GetModuleHandleW( 0 );

    if ((status = LdrFindResourceDirectory_U( module, NULL, 0, &resdir )) != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }
    et = (const IMAGE_RESOURCE_DIRECTORY_ENTRY *)(resdir + 1);
    for (i = 0; i < resdir->NumberOfNamedEntries + resdir->NumberOfIdEntries; i++)
    {
        if (et[i].u.s.NameIsString)
        {
            str = (const IMAGE_RESOURCE_DIR_STRING_U *)((const BYTE *)resdir + et[i].u.s.NameOffset);
            if (str->Length + 1 > len)
            {
                len = str->Length + 1;
                HeapFree( GetProcessHeap(), 0, type );
                if (!(type = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) ))) return FALSE;
            }
            memcpy(type, str->NameString, str->Length * sizeof (WCHAR));
            type[str->Length] = 0;
            ret = func( module, type, param );
        }
        else
        {
            ret = func( module, UIntToPtr(et[i].u.Id), param );
        }
        if (!ret) break;
    }
    HeapFree( GetProcessHeap(), 0, type );
    return ret;
}


/**********************************************************************
 *	    FindResourceExW  (kernelbase.@)
 */
HRSRC WINAPI DECLSPEC_HOTPATCH FindResourceExW( HMODULE module, LPCWSTR type, LPCWSTR name, WORD lang )
{
    NTSTATUS status;
    UNICODE_STRING nameW, typeW;
    LDR_RESOURCE_INFO info;
    const IMAGE_RESOURCE_DATA_ENTRY *entry = NULL;

    TRACE( "%p %s %s %04x\n", module, debugstr_w(type), debugstr_w(name), lang );

    if (!module) module = GetModuleHandleW( 0 );
    nameW.Buffer = typeW.Buffer = NULL;

    __TRY
    {
        if ((status = get_res_nameW( name, &nameW )) != STATUS_SUCCESS) goto done;
        if ((status = get_res_nameW( type, &typeW )) != STATUS_SUCCESS) goto done;
        info.Type = (ULONG_PTR)typeW.Buffer;
        info.Name = (ULONG_PTR)nameW.Buffer;
        info.Language = lang;
        status = LdrFindResource_U( module, &info, 3, &entry );
    done:
        if (status != STATUS_SUCCESS) SetLastError( RtlNtStatusToDosError(status) );
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError( ERROR_INVALID_PARAMETER );
    }
    __ENDTRY

    if (!IS_INTRESOURCE(nameW.Buffer)) HeapFree( GetProcessHeap(), 0, nameW.Buffer );
    if (!IS_INTRESOURCE(typeW.Buffer)) HeapFree( GetProcessHeap(), 0, typeW.Buffer );
    return (HRSRC)entry;
}


/**********************************************************************
 *	    FindResourceW    (kernelbase.@)
 */
HRSRC WINAPI DECLSPEC_HOTPATCH FindResourceW( HINSTANCE module, LPCWSTR name, LPCWSTR type )
{
    return FindResourceExW( module, type, name, MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL ) );
}


/**********************************************************************
 *	    FreeResource     (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH FreeResource( HGLOBAL handle )
{
    return FALSE;
}


/**********************************************************************
 *	    LoadResource     (kernelbase.@)
 */
HGLOBAL WINAPI DECLSPEC_HOTPATCH LoadResource( HINSTANCE module, HRSRC rsrc )
{
    NTSTATUS status;
    void *ret = NULL;

    TRACE( "%p %p\n", module, rsrc );

    if (!rsrc) return 0;
    if (!module) module = GetModuleHandleW( 0 );
    status = LdrAccessResource( module, (IMAGE_RESOURCE_DATA_ENTRY *)rsrc, &ret, NULL );
    if (status != STATUS_SUCCESS) SetLastError( RtlNtStatusToDosError(status) );
    return ret;
}


/**********************************************************************
 *	    LockResource     (kernelbase.@)
 */
LPVOID WINAPI DECLSPEC_HOTPATCH LockResource( HGLOBAL handle )
{
    return handle;
}


/**********************************************************************
 *	    SizeofResource   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH SizeofResource( HINSTANCE module, HRSRC rsrc )
{
    if (!rsrc) return 0;
    return ((IMAGE_RESOURCE_DATA_ENTRY *)rsrc)->Size;
}


/***********************************************************************
 * Activation contexts
 ***********************************************************************/


/***********************************************************************
 *          ActivateActCtx    (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH ActivateActCtx( HANDLE context, ULONG_PTR *cookie )
{
    return set_ntstatus( RtlActivateActivationContext( 0, context, cookie ));
}


/***********************************************************************
 *          AddRefActCtx    (kernelbase.@)
 */
void WINAPI DECLSPEC_HOTPATCH AddRefActCtx( HANDLE context )
{
    RtlAddRefActivationContext( context );
}


/***********************************************************************
 *          CreateActCtxW    (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateActCtxW( PCACTCTXW ctx )
{
    NTSTATUS status;
    HANDLE context;

    TRACE( "%p %08x\n", ctx, ctx ? ctx->dwFlags : 0 );

    if ((status = RtlCreateActivationContext( &context, ctx )))
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return INVALID_HANDLE_VALUE;
    }
    return context;
}


/***********************************************************************
 *          DeactivateActCtx    (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH DeactivateActCtx( DWORD flags, ULONG_PTR cookie )
{
    RtlDeactivateActivationContext( flags, cookie );
    return TRUE;
}


/***********************************************************************
 *          FindActCtxSectionGuid    (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH FindActCtxSectionGuid( DWORD flags, const GUID *ext_guid, ULONG id,
                                                     const GUID *guid, PACTCTX_SECTION_KEYED_DATA info )
{
    return set_ntstatus( RtlFindActivationContextSectionGuid( flags, ext_guid, id, guid, info ));
}


/***********************************************************************
 *          FindActCtxSectionStringW    (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH FindActCtxSectionStringW( DWORD flags, const GUID *ext_guid, ULONG id,
                                                        LPCWSTR str, PACTCTX_SECTION_KEYED_DATA info )
{
    UNICODE_STRING us;

    if (!info)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    RtlInitUnicodeString( &us, str );
    return set_ntstatus( RtlFindActivationContextSectionString( flags, ext_guid, id, &us, info ));
}


/***********************************************************************
 *          GetCurrentActCtx    (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetCurrentActCtx( HANDLE *pcontext )
{
    return set_ntstatus( RtlGetActiveActivationContext( pcontext ));
}


/***********************************************************************
 *          QueryActCtxSettingsW    (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH QueryActCtxSettingsW( DWORD flags, HANDLE ctx, const WCHAR *ns,
                                                    const WCHAR *settings, WCHAR *buffer, SIZE_T size,
                                                    SIZE_T *written )
{
    return set_ntstatus( RtlQueryActivationContextApplicationSettings( flags, ctx, ns, settings,
                                                                       buffer, size, written ));
}


/***********************************************************************
 *          QueryActCtxW    (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH QueryActCtxW( DWORD flags, HANDLE context, PVOID inst, ULONG class,
                                            PVOID buffer, SIZE_T size, SIZE_T *written )
{
    return set_ntstatus( RtlQueryInformationActivationContext( flags, context, inst, class,
                                                               buffer, size, written ));
}


/***********************************************************************
 *          ReleaseActCtx    (kernelbase.@)
 */
void WINAPI DECLSPEC_HOTPATCH ReleaseActCtx( HANDLE context )
{
    RtlReleaseActivationContext( context );
}


/***********************************************************************
 *          ZombifyActCtx    (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH ZombifyActCtx( HANDLE context )
{
    return set_ntstatus( RtlZombifyActivationContext( context ));
}
