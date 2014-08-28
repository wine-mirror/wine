/*
 * Registry management
 *
 * Copyright (C) 1999 Alexandre Julliard
 *
 * Based on misc/registry.c code
 * Copyright (C) 1996 Marcus Meissner
 * Copyright (C) 1998 Matthew Becker
 * Copyright (C) 1999 Sylvain St-Germain
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

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winerror.h"
#include "winternl.h"
#include "winuser.h"
#include "advapi32_misc.h"

#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(reg);

#define HKEY_SPECIAL_ROOT_FIRST   HKEY_CLASSES_ROOT
#define HKEY_SPECIAL_ROOT_LAST    HKEY_DYN_DATA

static const WCHAR name_CLASSES_ROOT[] =
    {'M','a','c','h','i','n','e','\\',
     'S','o','f','t','w','a','r','e','\\',
     'C','l','a','s','s','e','s',0};
static const WCHAR name_LOCAL_MACHINE[] =
    {'M','a','c','h','i','n','e',0};
static const WCHAR name_USERS[] =
    {'U','s','e','r',0};
static const WCHAR name_PERFORMANCE_DATA[] =
    {'P','e','r','f','D','a','t','a',0};
static const WCHAR name_CURRENT_CONFIG[] =
    {'M','a','c','h','i','n','e','\\',
     'S','y','s','t','e','m','\\',
     'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
     'H','a','r','d','w','a','r','e',' ','P','r','o','f','i','l','e','s','\\',
     'C','u','r','r','e','n','t',0};
static const WCHAR name_DYN_DATA[] =
    {'D','y','n','D','a','t','a',0};

static const WCHAR * const root_key_names[] =
{
    name_CLASSES_ROOT,
    NULL,         /* HKEY_CURRENT_USER is determined dynamically */
    name_LOCAL_MACHINE,
    name_USERS,
    name_PERFORMANCE_DATA,
    name_CURRENT_CONFIG,
    name_DYN_DATA
};

#define NB_SPECIAL_ROOT_KEYS   (sizeof(root_key_names)/sizeof(root_key_names[0]))

static HKEY special_root_keys[NB_SPECIAL_ROOT_KEYS];
static BOOL hkcu_cache_disabled;

static const BOOL is_win64 = (sizeof(void *) > sizeof(int));

/* check if value type needs string conversion (Ansi<->Unicode) */
static inline BOOL is_string( DWORD type )
{
    return (type == REG_SZ) || (type == REG_EXPAND_SZ) || (type == REG_MULTI_SZ);
}

/* check if current version is NT or Win95 */
static inline BOOL is_version_nt(void)
{
    return !(GetVersion() & 0x80000000);
}

static BOOL is_wow6432node( const UNICODE_STRING *name )
{
    static const WCHAR wow6432nodeW[] = {'W','o','w','6','4','3','2','N','o','d','e'};

    return (name->Length == sizeof(wow6432nodeW) &&
            !memicmpW( name->Buffer, wow6432nodeW, sizeof(wow6432nodeW)/sizeof(WCHAR) ));
}

/* open the Wow6432Node subkey of the specified key */
static HANDLE open_wow6432node( HANDLE key, const UNICODE_STRING *name )
{
    static const WCHAR wow6432nodeW[] = {'W','o','w','6','4','3','2','N','o','d','e',0};
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    HANDLE ret;

    attr.Length = sizeof(attr);
    attr.RootDirectory = key;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    RtlInitUnicodeString( &nameW, wow6432nodeW );
    if (NtOpenKey( &ret, MAXIMUM_ALLOWED, &attr )) ret = 0;
    return ret;
}

/* wrapper for NtCreateKey that creates the key recursively if necessary */
static NTSTATUS create_key( HKEY *retkey, ACCESS_MASK access, OBJECT_ATTRIBUTES *attr,
                            const UNICODE_STRING *class, ULONG options, PULONG dispos )
{
    BOOL force_wow32 = is_win64 && (access & KEY_WOW64_32KEY);
    NTSTATUS status = STATUS_OBJECT_NAME_NOT_FOUND;

    if (!force_wow32) status = NtCreateKey( (HANDLE *)retkey, access, attr, 0, class, options, dispos );

    if (status == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        HANDLE subkey, root = attr->RootDirectory;
        WCHAR *buffer = attr->ObjectName->Buffer;
        DWORD attrs, pos = 0, i = 0, len = attr->ObjectName->Length / sizeof(WCHAR);
        UNICODE_STRING str;

        while (i < len && buffer[i] != '\\') i++;
        if (i == len && !force_wow32) return status;

        attrs = attr->Attributes;
        attr->ObjectName = &str;

        for (;;)
        {
            str.Buffer = buffer + pos;
            str.Length = (i - pos) * sizeof(WCHAR);
            if (force_wow32 && pos)
            {
                if (is_wow6432node( &str )) force_wow32 = FALSE;
                else if ((subkey = open_wow6432node( attr->RootDirectory, &str )))
                {
                    if (attr->RootDirectory != root) NtClose( attr->RootDirectory );
                    attr->RootDirectory = subkey;
                    force_wow32 = FALSE;
                }
            }
            if (i == len)
            {
                attr->Attributes = attrs;
                status = NtCreateKey( (PHANDLE)retkey, access, attr, 0, class, options, dispos );
            }
            else
            {
                attr->Attributes = attrs & ~OBJ_OPENLINK;
                status = NtCreateKey( &subkey, access, attr, 0, class,
                                      options & ~REG_OPTION_CREATE_LINK, dispos );
            }
            if (attr->RootDirectory != root) NtClose( attr->RootDirectory );
            if (status) return status;
            if (i == len) break;
            attr->RootDirectory = subkey;
            while (i < len && buffer[i] == '\\') i++;
            pos = i;
            while (i < len && buffer[i] != '\\') i++;
        }
    }
    return status;
}

/* wrapper for NtOpenKey to handle Wow6432 nodes */
static NTSTATUS open_key( HKEY *retkey, ACCESS_MASK access, OBJECT_ATTRIBUTES *attr )
{
    NTSTATUS status;
    BOOL force_wow32 = is_win64 && (access & KEY_WOW64_32KEY);
    HANDLE subkey, root = attr->RootDirectory;
    WCHAR *buffer = attr->ObjectName->Buffer;
    DWORD attrs, pos = 0, i = 0, len = attr->ObjectName->Length / sizeof(WCHAR);
    UNICODE_STRING str;

    if (!force_wow32) return NtOpenKey( (HANDLE *)retkey, access, attr );

    if (len && buffer[0] == '\\') return STATUS_OBJECT_PATH_INVALID;
    while (i < len && buffer[i] != '\\') i++;
    attrs = attr->Attributes;
    attr->ObjectName = &str;

    for (;;)
    {
        str.Buffer = buffer + pos;
        str.Length = (i - pos) * sizeof(WCHAR);
        if (force_wow32 && pos)
        {
            if (is_wow6432node( &str )) force_wow32 = FALSE;
            else if ((subkey = open_wow6432node( attr->RootDirectory, &str )))
            {
                if (attr->RootDirectory != root) NtClose( attr->RootDirectory );
                attr->RootDirectory = subkey;
                force_wow32 = FALSE;
            }
        }
        if (i == len)
        {
            attr->Attributes = attrs;
            status = NtOpenKey( (PHANDLE)retkey, access, attr );
        }
        else
        {
            attr->Attributes = attrs & ~OBJ_OPENLINK;
            status = NtOpenKey( &subkey, access, attr );
        }
        if (attr->RootDirectory != root) NtClose( attr->RootDirectory );
        if (status) return status;
        if (i == len) break;
        attr->RootDirectory = subkey;
        while (i < len && buffer[i] == '\\') i++;
        pos = i;
        while (i < len && buffer[i] != '\\') i++;
    }
    return status;
}

/* create one of the HKEY_* special root keys */
static HKEY create_special_root_hkey( HKEY hkey, DWORD access )
{
    HKEY ret = 0;
    int idx = HandleToUlong(hkey) - HandleToUlong(HKEY_SPECIAL_ROOT_FIRST);

    if (HandleToUlong(hkey) == HandleToUlong(HKEY_CURRENT_USER))
    {
        if (RtlOpenCurrentUser( access, (HANDLE *)&hkey )) return 0;
        TRACE( "HKEY_CURRENT_USER -> %p\n", hkey );

        /* don't cache the key in the table if caching is disabled */
        if (hkcu_cache_disabled)
            return hkey;
    }
    else
    {
        OBJECT_ATTRIBUTES attr;
        UNICODE_STRING name;

        attr.Length = sizeof(attr);
        attr.RootDirectory = 0;
        attr.ObjectName = &name;
        attr.Attributes = 0;
        attr.SecurityDescriptor = NULL;
        attr.SecurityQualityOfService = NULL;
        RtlInitUnicodeString( &name, root_key_names[idx] );
        if (create_key( &hkey, access, &attr, NULL, 0, NULL )) return 0;
        TRACE( "%s -> %p\n", debugstr_w(attr.ObjectName->Buffer), hkey );
    }

    if (!(access & (KEY_WOW64_64KEY | KEY_WOW64_32KEY)))
    {
        if (!(ret = InterlockedCompareExchangePointer( (void **)&special_root_keys[idx], hkey, 0 )))
            ret = hkey;
        else
            NtClose( hkey );  /* somebody beat us to it */
    }
    else
        ret = hkey;
    return ret;
}

/* map the hkey from special root to normal key if necessary */
static inline HKEY get_special_root_hkey( HKEY hkey, REGSAM access )
{
    HKEY ret = hkey;

    if ((HandleToUlong(hkey) >= HandleToUlong(HKEY_SPECIAL_ROOT_FIRST))
            && (HandleToUlong(hkey) <= HandleToUlong(HKEY_SPECIAL_ROOT_LAST)))
    {
        REGSAM mask = 0;

        if (HandleToUlong(hkey) == HandleToUlong(HKEY_CLASSES_ROOT))
            mask = KEY_WOW64_32KEY | KEY_WOW64_64KEY;

        if ((access & mask) ||
                !(ret = special_root_keys[HandleToUlong(hkey) - HandleToUlong(HKEY_SPECIAL_ROOT_FIRST)]))
            ret = create_special_root_hkey( hkey, MAXIMUM_ALLOWED | (access & mask) );
    }
    return ret;
}


/******************************************************************************
 * RegOverridePredefKey   [ADVAPI32.@]
 */
LSTATUS WINAPI RegOverridePredefKey( HKEY hkey, HKEY override )
{
    HKEY old_key;
    int idx;

    TRACE("(%p %p)\n", hkey, override);

    if ((HandleToUlong(hkey) < HandleToUlong(HKEY_SPECIAL_ROOT_FIRST))
            || (HandleToUlong(hkey) > HandleToUlong(HKEY_SPECIAL_ROOT_LAST)))
        return ERROR_INVALID_PARAMETER;
    idx = HandleToUlong(hkey) - HandleToUlong(HKEY_SPECIAL_ROOT_FIRST);

    if (override)
    {
        NTSTATUS status = NtDuplicateObject( GetCurrentProcess(), override,
                                             GetCurrentProcess(), (HANDLE *)&override,
                                             0, 0, DUPLICATE_SAME_ACCESS );
        if (status) return RtlNtStatusToDosError( status );
    }

    old_key = InterlockedExchangePointer( (void **)&special_root_keys[idx], override );
    if (old_key) NtClose( old_key );
    return ERROR_SUCCESS;
}


/******************************************************************************
 * RegCreateKeyExW   [ADVAPI32.@]
 *
 * See RegCreateKeyExA.
 */
LSTATUS WINAPI RegCreateKeyExW( HKEY hkey, LPCWSTR name, DWORD reserved, LPWSTR class,
                             DWORD options, REGSAM access, SECURITY_ATTRIBUTES *sa,
                             PHKEY retkey, LPDWORD dispos )
{
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW, classW;

    if (reserved) return ERROR_INVALID_PARAMETER;
    if (!(hkey = get_special_root_hkey( hkey, access ))) return ERROR_INVALID_HANDLE;

    attr.Length = sizeof(attr);
    attr.RootDirectory = hkey;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    if (options & REG_OPTION_OPEN_LINK) attr.Attributes |= OBJ_OPENLINK;
    RtlInitUnicodeString( &nameW, name );
    RtlInitUnicodeString( &classW, class );

    return RtlNtStatusToDosError( create_key( retkey, access, &attr, &classW, options, dispos ) );
}


/******************************************************************************
 * RegCreateKeyExA   [ADVAPI32.@]
 *
 * Open a registry key, creating it if it doesn't exist.
 *
 * PARAMS
 *  hkey       [I] Handle of the parent registry key
 *  name       [I] Name of the new key to open or create
 *  reserved   [I] Reserved, pass 0
 *  class      [I] The object type of the new key
 *  options    [I] Flags controlling the key creation (REG_OPTION_* flags from "winnt.h")
 *  access     [I] Access level desired
 *  sa         [I] Security attributes for the key
 *  retkey     [O] Destination for the resulting handle
 *  dispos     [O] Receives REG_CREATED_NEW_KEY or REG_OPENED_EXISTING_KEY
 *
 * RETURNS
 *  Success: ERROR_SUCCESS.
 *  Failure: A standard Win32 error code. retkey remains untouched.
 *
 * FIXME
 *  MAXIMUM_ALLOWED in access mask not supported by server
 */
LSTATUS WINAPI RegCreateKeyExA( HKEY hkey, LPCSTR name, DWORD reserved, LPSTR class,
                             DWORD options, REGSAM access, SECURITY_ATTRIBUTES *sa,
                             PHKEY retkey, LPDWORD dispos )
{
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING classW;
    ANSI_STRING nameA, classA;
    NTSTATUS status;

    if (reserved) return ERROR_INVALID_PARAMETER;
    if (!is_version_nt())
    {
        access = MAXIMUM_ALLOWED;  /* Win95 ignores the access mask */
        if (name && *name == '\\') name++; /* win9x,ME ignores one (and only one) beginning backslash */
    }
    if (!(hkey = get_special_root_hkey( hkey, access ))) return ERROR_INVALID_HANDLE;

    attr.Length = sizeof(attr);
    attr.RootDirectory = hkey;
    attr.ObjectName = &NtCurrentTeb()->StaticUnicodeString;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    if (options & REG_OPTION_OPEN_LINK) attr.Attributes |= OBJ_OPENLINK;
    RtlInitAnsiString( &nameA, name );
    RtlInitAnsiString( &classA, class );

    if (!(status = RtlAnsiStringToUnicodeString( &NtCurrentTeb()->StaticUnicodeString,
                                                 &nameA, FALSE )))
    {
        if (!(status = RtlAnsiStringToUnicodeString( &classW, &classA, TRUE )))
        {
            status = create_key( retkey, access, &attr, &classW, options, dispos );
            RtlFreeUnicodeString( &classW );
        }
    }
    return RtlNtStatusToDosError( status );
}


/******************************************************************************
 * RegCreateKeyW   [ADVAPI32.@]
 *
 * Creates the specified reg key.
 *
 * PARAMS
 *  hKey      [I] Handle to an open key.
 *  lpSubKey  [I] Name of a key that will be opened or created.
 *  phkResult [O] Receives a handle to the opened or created key.
 *
 * RETURNS
 *  Success: ERROR_SUCCESS
 *  Failure: nonzero error code defined in Winerror.h
 */
LSTATUS WINAPI RegCreateKeyW( HKEY hkey, LPCWSTR lpSubKey, PHKEY phkResult )
{
    /* FIXME: previous implementation converted ERROR_INVALID_HANDLE to ERROR_BADKEY, */
    /* but at least my version of NT (4.0 SP5) doesn't do this.  -- AJ */
    return RegCreateKeyExW( hkey, lpSubKey, 0, NULL, REG_OPTION_NON_VOLATILE,
                            MAXIMUM_ALLOWED, NULL, phkResult, NULL );
}


/******************************************************************************
 * RegCreateKeyA   [ADVAPI32.@]
 *
 * See RegCreateKeyW.
 */
LSTATUS WINAPI RegCreateKeyA( HKEY hkey, LPCSTR lpSubKey, PHKEY phkResult )
{
    return RegCreateKeyExA( hkey, lpSubKey, 0, NULL, REG_OPTION_NON_VOLATILE,
                            MAXIMUM_ALLOWED, NULL, phkResult, NULL );
}



/******************************************************************************
 * RegOpenKeyExW   [ADVAPI32.@]
 * 
 * See RegOpenKeyExA.
 */
LSTATUS WINAPI RegOpenKeyExW( HKEY hkey, LPCWSTR name, DWORD options, REGSAM access, PHKEY retkey )
{
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;

    /* NT+ allows beginning backslash for HKEY_CLASSES_ROOT */
    if (HandleToUlong(hkey) == HandleToUlong(HKEY_CLASSES_ROOT) && name && *name == '\\') name++;

    if (!retkey) return ERROR_INVALID_PARAMETER;
    if (!(hkey = get_special_root_hkey( hkey, access ))) return ERROR_INVALID_HANDLE;

    attr.Length = sizeof(attr);
    attr.RootDirectory = hkey;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    if (options & REG_OPTION_OPEN_LINK) attr.Attributes |= OBJ_OPENLINK;
    RtlInitUnicodeString( &nameW, name );
    return RtlNtStatusToDosError( open_key( retkey, access, &attr ) );
}


/******************************************************************************
 * RegOpenKeyExA   [ADVAPI32.@]
 *
 * Open a registry key.
 *
 * PARAMS
 *  hkey       [I] Handle of open key
 *  name       [I] Name of subkey to open
 *  options    [I] Open options (can be set to REG_OPTION_OPEN_LINK)
 *  access     [I] Security access mask
 *  retkey     [O] Handle to open key
 *
 * RETURNS
 *  Success: ERROR_SUCCESS
 *  Failure: A standard Win32 error code. retkey is set to 0.
 *
 * NOTES
 *  Unlike RegCreateKeyExA(), this function will not create the key if it
 *  does not exist.
 */
LSTATUS WINAPI RegOpenKeyExA( HKEY hkey, LPCSTR name, DWORD options, REGSAM access, PHKEY retkey )
{
    OBJECT_ATTRIBUTES attr;
    STRING nameA;
    NTSTATUS status;

    if (!is_version_nt()) access = MAXIMUM_ALLOWED;  /* Win95 ignores the access mask */
    else
    {
        /* NT+ allows beginning backslash for HKEY_CLASSES_ROOT */
        if (HandleToUlong(hkey) == HandleToUlong(HKEY_CLASSES_ROOT) && name && *name == '\\') name++;
    }

    if (!(hkey = get_special_root_hkey( hkey, access ))) return ERROR_INVALID_HANDLE;

    attr.Length = sizeof(attr);
    attr.RootDirectory = hkey;
    attr.ObjectName = &NtCurrentTeb()->StaticUnicodeString;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    if (options & REG_OPTION_OPEN_LINK) attr.Attributes |= OBJ_OPENLINK;

    RtlInitAnsiString( &nameA, name );
    if (!(status = RtlAnsiStringToUnicodeString( &NtCurrentTeb()->StaticUnicodeString,
                                                 &nameA, FALSE )))
    {
        status = open_key( retkey, access, &attr );
    }
    return RtlNtStatusToDosError( status );
}


/******************************************************************************
 * RegOpenKeyW   [ADVAPI32.@]
 *
 * See RegOpenKeyA.
 */
LSTATUS WINAPI RegOpenKeyW( HKEY hkey, LPCWSTR name, PHKEY retkey )
{
    if (!retkey)
        return ERROR_INVALID_PARAMETER;

    if (!name || !*name)
    {
        *retkey = hkey;
        return ERROR_SUCCESS;
    }
    return RegOpenKeyExW( hkey, name, 0, MAXIMUM_ALLOWED, retkey );
}


/******************************************************************************
 * RegOpenKeyA   [ADVAPI32.@]
 *           
 * Open a registry key.
 *
 * PARAMS
 *  hkey    [I] Handle of parent key to open the new key under
 *  name    [I] Name of the key under hkey to open
 *  retkey  [O] Destination for the resulting Handle
 *
 * RETURNS
 *  Success: ERROR_SUCCESS
 *  Failure: A standard Win32 error code. When retkey is valid, *retkey is set to 0.
 */
LSTATUS WINAPI RegOpenKeyA( HKEY hkey, LPCSTR name, PHKEY retkey )
{
    if (!retkey)
        return ERROR_INVALID_PARAMETER;

    if (!name || !*name)
    {
        *retkey = hkey;
        return ERROR_SUCCESS;
    }
    return RegOpenKeyExA( hkey, name, 0, MAXIMUM_ALLOWED, retkey );
}


/******************************************************************************
 * RegOpenCurrentUser   [ADVAPI32.@]
 *
 * Get a handle to the HKEY_CURRENT_USER key for the user 
 * the current thread is impersonating.
 *
 * PARAMS
 *  access [I] Desired access rights to the key
 *  retkey [O] Handle to the opened key
 *
 * RETURNS
 *  Success: ERROR_SUCCESS
 *  Failure: nonzero error code from Winerror.h
 *
 * FIXME
 *  This function is supposed to retrieve a handle to the
 *  HKEY_CURRENT_USER for the user the current thread is impersonating.
 *  Since Wine does not currently allow threads to impersonate other users,
 *  this stub should work fine.
 */
LSTATUS WINAPI RegOpenCurrentUser( REGSAM access, PHKEY retkey )
{
    return RegOpenKeyExA( HKEY_CURRENT_USER, "", 0, access, retkey );
}



/******************************************************************************
 * RegEnumKeyExW   [ADVAPI32.@]
 *
 * Enumerate subkeys of the specified open registry key.
 *
 * PARAMS
 *  hkey         [I] Handle to key to enumerate
 *  index        [I] Index of subkey to enumerate
 *  name         [O] Buffer for subkey name
 *  name_len     [O] Size of subkey buffer
 *  reserved     [I] Reserved
 *  class        [O] Buffer for class string
 *  class_len    [O] Size of class buffer
 *  ft           [O] Time key last written to
 *
 * RETURNS
 *  Success: ERROR_SUCCESS
 *  Failure: System error code. If there are no more subkeys available, the
 *           function returns ERROR_NO_MORE_ITEMS.
 */
LSTATUS WINAPI RegEnumKeyExW( HKEY hkey, DWORD index, LPWSTR name, LPDWORD name_len,
                           LPDWORD reserved, LPWSTR class, LPDWORD class_len, FILETIME *ft )
{
    NTSTATUS status;
    char buffer[256], *buf_ptr = buffer;
    KEY_NODE_INFORMATION *info = (KEY_NODE_INFORMATION *)buffer;
    DWORD total_size;

    TRACE( "(%p,%d,%p,%p(%u),%p,%p,%p,%p)\n", hkey, index, name, name_len,
           name_len ? *name_len : 0, reserved, class, class_len, ft );

    if (reserved) return ERROR_INVALID_PARAMETER;
    if (!(hkey = get_special_root_hkey( hkey, 0 ))) return ERROR_INVALID_HANDLE;

    status = NtEnumerateKey( hkey, index, KeyNodeInformation,
                             buffer, sizeof(buffer), &total_size );

    while (status == STATUS_BUFFER_OVERFLOW)
    {
        /* retry with a dynamically allocated buffer */
        if (buf_ptr != buffer) heap_free( buf_ptr );
        if (!(buf_ptr = heap_alloc( total_size )))
            return ERROR_NOT_ENOUGH_MEMORY;
        info = (KEY_NODE_INFORMATION *)buf_ptr;
        status = NtEnumerateKey( hkey, index, KeyNodeInformation,
                                 buf_ptr, total_size, &total_size );
    }

    if (!status)
    {
        DWORD len = info->NameLength / sizeof(WCHAR);
        DWORD cls_len = info->ClassLength / sizeof(WCHAR);

        if (ft) *ft = *(FILETIME *)&info->LastWriteTime;

        if (len >= *name_len || (class && class_len && (cls_len >= *class_len)))
            status = STATUS_BUFFER_OVERFLOW;
        else
        {
            *name_len = len;
            memcpy( name, info->Name, info->NameLength );
            name[len] = 0;
            if (class_len)
            {
                *class_len = cls_len;
                if (class)
                {
                    memcpy( class, buf_ptr + info->ClassOffset, info->ClassLength );
                    class[cls_len] = 0;
                }
            }
        }
    }

    if (buf_ptr != buffer) heap_free( buf_ptr );
    return RtlNtStatusToDosError( status );
}


/******************************************************************************
 * RegEnumKeyExA   [ADVAPI32.@]
 *
 * See RegEnumKeyExW.
 */
LSTATUS WINAPI RegEnumKeyExA( HKEY hkey, DWORD index, LPSTR name, LPDWORD name_len,
                           LPDWORD reserved, LPSTR class, LPDWORD class_len, FILETIME *ft )
{
    NTSTATUS status;
    char buffer[256], *buf_ptr = buffer;
    KEY_NODE_INFORMATION *info = (KEY_NODE_INFORMATION *)buffer;
    DWORD total_size;

    TRACE( "(%p,%d,%p,%p(%u),%p,%p,%p,%p)\n", hkey, index, name, name_len,
           name_len ? *name_len : 0, reserved, class, class_len, ft );

    if (reserved) return ERROR_INVALID_PARAMETER;
    if (!(hkey = get_special_root_hkey( hkey, 0 ))) return ERROR_INVALID_HANDLE;

    status = NtEnumerateKey( hkey, index, KeyNodeInformation,
                             buffer, sizeof(buffer), &total_size );

    while (status == STATUS_BUFFER_OVERFLOW)
    {
        /* retry with a dynamically allocated buffer */
        if (buf_ptr != buffer) heap_free( buf_ptr );
        if (!(buf_ptr = heap_alloc( total_size )))
            return ERROR_NOT_ENOUGH_MEMORY;
        info = (KEY_NODE_INFORMATION *)buf_ptr;
        status = NtEnumerateKey( hkey, index, KeyNodeInformation,
                                 buf_ptr, total_size, &total_size );
    }

    if (!status)
    {
        DWORD len, cls_len;

        RtlUnicodeToMultiByteSize( &len, info->Name, info->NameLength );
        RtlUnicodeToMultiByteSize( &cls_len, (WCHAR *)(buf_ptr + info->ClassOffset),
                                   info->ClassLength );
        if (ft) *ft = *(FILETIME *)&info->LastWriteTime;

        if (len >= *name_len || (class && class_len && (cls_len >= *class_len)))
            status = STATUS_BUFFER_OVERFLOW;
        else
        {
            *name_len = len;
            RtlUnicodeToMultiByteN( name, len, NULL, info->Name, info->NameLength );
            name[len] = 0;
            if (class_len)
            {
                *class_len = cls_len;
                if (class)
                {
                    RtlUnicodeToMultiByteN( class, cls_len, NULL,
                                            (WCHAR *)(buf_ptr + info->ClassOffset),
                                            info->ClassLength );
                    class[cls_len] = 0;
                }
            }
        }
    }

    if (buf_ptr != buffer) heap_free( buf_ptr );
    return RtlNtStatusToDosError( status );
}


/******************************************************************************
 * RegEnumKeyW   [ADVAPI32.@]
 *
 * Enumerates subkeys of the specified open reg key.
 *
 * PARAMS
 *  hKey    [I] Handle to an open key.
 *  dwIndex [I] Index of the subkey of hKey to retrieve.
 *  lpName  [O] Name of the subkey.
 *  cchName [I] Size of lpName in TCHARS.
 *
 * RETURNS
 *  Success: ERROR_SUCCESS
 *  Failure: system error code. If there are no more subkeys available, the
 *           function returns ERROR_NO_MORE_ITEMS.
 */
LSTATUS WINAPI RegEnumKeyW( HKEY hkey, DWORD index, LPWSTR name, DWORD name_len )
{
    return RegEnumKeyExW( hkey, index, name, &name_len, NULL, NULL, NULL, NULL );
}


/******************************************************************************
 * RegEnumKeyA   [ADVAPI32.@]
 *
 * See RegEnumKeyW.
 */
LSTATUS WINAPI RegEnumKeyA( HKEY hkey, DWORD index, LPSTR name, DWORD name_len )
{
    return RegEnumKeyExA( hkey, index, name, &name_len, NULL, NULL, NULL, NULL );
}


/******************************************************************************
 * RegQueryInfoKeyW   [ADVAPI32.@]
 *
 * Retrieves information about the specified registry key.
 *
 * PARAMS
 *    hkey       [I] Handle to key to query
 *    class      [O] Buffer for class string
 *    class_len  [O] Size of class string buffer
 *    reserved   [I] Reserved
 *    subkeys    [O] Buffer for number of subkeys
 *    max_subkey [O] Buffer for longest subkey name length
 *    max_class  [O] Buffer for longest class string length
 *    values     [O] Buffer for number of value entries
 *    max_value  [O] Buffer for longest value name length
 *    max_data   [O] Buffer for longest value data length
 *    security   [O] Buffer for security descriptor length
 *    modif      [O] Modification time
 *
 * RETURNS
 *  Success: ERROR_SUCCESS
 *  Failure: system error code.
 *
 * NOTES
 *  - win95 allows class to be valid and class_len to be NULL
 *  - winnt returns ERROR_INVALID_PARAMETER if class is valid and class_len is NULL
 *  - both allow class to be NULL and class_len to be NULL
 *    (it's hard to test validity, so test !NULL instead)
 */
LSTATUS WINAPI RegQueryInfoKeyW( HKEY hkey, LPWSTR class, LPDWORD class_len, LPDWORD reserved,
                              LPDWORD subkeys, LPDWORD max_subkey, LPDWORD max_class,
                              LPDWORD values, LPDWORD max_value, LPDWORD max_data,
                              LPDWORD security, FILETIME *modif )
{
    NTSTATUS status;
    char buffer[256], *buf_ptr = buffer;
    KEY_FULL_INFORMATION *info = (KEY_FULL_INFORMATION *)buffer;
    DWORD total_size;

    TRACE( "(%p,%p,%d,%p,%p,%p,%p,%p,%p,%p,%p)\n", hkey, class, class_len ? *class_len : 0,
           reserved, subkeys, max_subkey, values, max_value, max_data, security, modif );

    if (class && !class_len && is_version_nt()) return ERROR_INVALID_PARAMETER;
    if (!(hkey = get_special_root_hkey( hkey, 0 ))) return ERROR_INVALID_HANDLE;

    status = NtQueryKey( hkey, KeyFullInformation, buffer, sizeof(buffer), &total_size );
    if (status && status != STATUS_BUFFER_OVERFLOW) goto done;

    if (class)
    {
        /* retry with a dynamically allocated buffer */
        while (status == STATUS_BUFFER_OVERFLOW)
        {
            if (buf_ptr != buffer) heap_free( buf_ptr );
            if (!(buf_ptr = heap_alloc( total_size )))
                return ERROR_NOT_ENOUGH_MEMORY;
            info = (KEY_FULL_INFORMATION *)buf_ptr;
            status = NtQueryKey( hkey, KeyFullInformation, buf_ptr, total_size, &total_size );
        }

        if (status) goto done;

        if (class_len && (info->ClassLength/sizeof(WCHAR) + 1 > *class_len))
        {
            status = STATUS_BUFFER_OVERFLOW;
        }
        else
        {
            memcpy( class, buf_ptr + info->ClassOffset, info->ClassLength );
            class[info->ClassLength/sizeof(WCHAR)] = 0;
        }
    }
    else status = STATUS_SUCCESS;

    if (class_len) *class_len = info->ClassLength / sizeof(WCHAR);
    if (subkeys) *subkeys = info->SubKeys;
    if (max_subkey) *max_subkey = info->MaxNameLen;
    if (max_class) *max_class = info->MaxClassLen;
    if (values) *values = info->Values;
    if (max_value) *max_value = info->MaxValueNameLen;
    if (max_data) *max_data = info->MaxValueDataLen;
    if (modif) *modif = *(FILETIME *)&info->LastWriteTime;

 done:
    if (buf_ptr != buffer) heap_free( buf_ptr );
    return RtlNtStatusToDosError( status );
}


/******************************************************************************
 * RegQueryMultipleValuesA   [ADVAPI32.@]
 *
 * Retrieves the type and data for a list of value names associated with a key.
 *
 * PARAMS
 *  hKey       [I] Handle to an open key.
 *  val_list   [O] Array of VALENT structures that describes the entries.
 *  num_vals   [I] Number of elements in val_list.
 *  lpValueBuf [O] Pointer to a buffer that receives the data for each value.
 *  ldwTotsize [I/O] Size of lpValueBuf.
 *
 * RETURNS
 *  Success: ERROR_SUCCESS. ldwTotsize contains num bytes copied.
 *  Failure: nonzero error code from Winerror.h ldwTotsize contains num needed
 *           bytes.
 */
LSTATUS WINAPI RegQueryMultipleValuesA( HKEY hkey, PVALENTA val_list, DWORD num_vals,
                                     LPSTR lpValueBuf, LPDWORD ldwTotsize )
{
    unsigned int i;
    DWORD maxBytes = *ldwTotsize;
    HRESULT status;
    LPSTR bufptr = lpValueBuf;
    *ldwTotsize = 0;

    TRACE("(%p,%p,%d,%p,%p=%d)\n", hkey, val_list, num_vals, lpValueBuf, ldwTotsize, *ldwTotsize);

    for(i=0; i < num_vals; ++i)
    {

        val_list[i].ve_valuelen=0;
        status = RegQueryValueExA(hkey, val_list[i].ve_valuename, NULL, NULL, NULL, &val_list[i].ve_valuelen);
        if(status != ERROR_SUCCESS)
        {
            return status;
        }

        if(lpValueBuf != NULL && *ldwTotsize + val_list[i].ve_valuelen <= maxBytes)
        {
            status = RegQueryValueExA(hkey, val_list[i].ve_valuename, NULL, &val_list[i].ve_type,
                                      (LPBYTE)bufptr, &val_list[i].ve_valuelen);
            if(status != ERROR_SUCCESS)
            {
                return status;
            }

            val_list[i].ve_valueptr = (DWORD_PTR)bufptr;

            bufptr += val_list[i].ve_valuelen;
        }

        *ldwTotsize += val_list[i].ve_valuelen;
    }
    return lpValueBuf != NULL && *ldwTotsize <= maxBytes ? ERROR_SUCCESS : ERROR_MORE_DATA;
}


/******************************************************************************
 * RegQueryMultipleValuesW   [ADVAPI32.@]
 *
 * See RegQueryMultipleValuesA.
 */
LSTATUS WINAPI RegQueryMultipleValuesW( HKEY hkey, PVALENTW val_list, DWORD num_vals,
                                     LPWSTR lpValueBuf, LPDWORD ldwTotsize )
{
    unsigned int i;
    DWORD maxBytes = *ldwTotsize;
    HRESULT status;
    LPSTR bufptr = (LPSTR)lpValueBuf;
    *ldwTotsize = 0;

    TRACE("(%p,%p,%d,%p,%p=%d)\n", hkey, val_list, num_vals, lpValueBuf, ldwTotsize, *ldwTotsize);

    for(i=0; i < num_vals; ++i)
    {
        val_list[i].ve_valuelen=0;
        status = RegQueryValueExW(hkey, val_list[i].ve_valuename, NULL, NULL, NULL, &val_list[i].ve_valuelen);
        if(status != ERROR_SUCCESS)
        {
            return status;
        }

        if(lpValueBuf != NULL && *ldwTotsize + val_list[i].ve_valuelen <= maxBytes)
        {
            status = RegQueryValueExW(hkey, val_list[i].ve_valuename, NULL, &val_list[i].ve_type,
                                      (LPBYTE)bufptr, &val_list[i].ve_valuelen);
            if(status != ERROR_SUCCESS)
            {
                return status;
            }

            val_list[i].ve_valueptr = (DWORD_PTR)bufptr;

            bufptr += val_list[i].ve_valuelen;
        }

        *ldwTotsize += val_list[i].ve_valuelen;
    }
    return lpValueBuf != NULL && *ldwTotsize <= maxBytes ? ERROR_SUCCESS : ERROR_MORE_DATA;
}

/******************************************************************************
 * RegQueryInfoKeyA   [ADVAPI32.@]
 *
 * Retrieves information about a registry key.
 *
 * PARAMS
 *  hKey                   [I] Handle to an open key.
 *  lpClass                [O] Class string of the key.
 *  lpcClass               [I/O] size of lpClass.
 *  lpReserved             [I] Reserved; must be NULL.
 *  lpcSubKeys             [O] Number of subkeys contained by the key.
 *  lpcMaxSubKeyLen        [O] Size of the key's subkey with the longest name.
 *  lpcMaxClassLen         [O] Size of the longest string specifying a subkey
 *                             class in TCHARS.
 *  lpcValues              [O] Number of values associated with the key.
 *  lpcMaxValueNameLen     [O] Size of the key's longest value name in TCHARS.
 *  lpcMaxValueLen         [O] Longest data component among the key's values
 *  lpcbSecurityDescriptor [O] Size of the key's security descriptor.
 *  lpftLastWriteTime      [O] FILETIME structure that is the last write time.
 *
 *  RETURNS
 *   Success: ERROR_SUCCESS
 *   Failure: nonzero error code from Winerror.h
 */
LSTATUS WINAPI RegQueryInfoKeyA( HKEY hkey, LPSTR class, LPDWORD class_len, LPDWORD reserved,
                              LPDWORD subkeys, LPDWORD max_subkey, LPDWORD max_class,
                              LPDWORD values, LPDWORD max_value, LPDWORD max_data,
                              LPDWORD security, FILETIME *modif )
{
    NTSTATUS status;
    char buffer[256], *buf_ptr = buffer;
    KEY_FULL_INFORMATION *info = (KEY_FULL_INFORMATION *)buffer;
    DWORD total_size, len;

    TRACE( "(%p,%p,%d,%p,%p,%p,%p,%p,%p,%p,%p)\n", hkey, class, class_len ? *class_len : 0,
           reserved, subkeys, max_subkey, values, max_value, max_data, security, modif );

    if (class && !class_len && is_version_nt()) return ERROR_INVALID_PARAMETER;
    if (!(hkey = get_special_root_hkey( hkey, 0 ))) return ERROR_INVALID_HANDLE;

    status = NtQueryKey( hkey, KeyFullInformation, buffer, sizeof(buffer), &total_size );
    if (status && status != STATUS_BUFFER_OVERFLOW) goto done;

    if (class || class_len)
    {
        /* retry with a dynamically allocated buffer */
        while (status == STATUS_BUFFER_OVERFLOW)
        {
            if (buf_ptr != buffer) heap_free( buf_ptr );
            if (!(buf_ptr = heap_alloc( total_size )))
                return ERROR_NOT_ENOUGH_MEMORY;
            info = (KEY_FULL_INFORMATION *)buf_ptr;
            status = NtQueryKey( hkey, KeyFullInformation, buf_ptr, total_size, &total_size );
        }

        if (status) goto done;

        RtlUnicodeToMultiByteSize( &len, (WCHAR *)(buf_ptr + info->ClassOffset), info->ClassLength);
        if (class_len)
        {
            if (len + 1 > *class_len) status = STATUS_BUFFER_OVERFLOW;
            *class_len = len;
        }
        if (class && !status)
        {
            RtlUnicodeToMultiByteN( class, len, NULL, (WCHAR *)(buf_ptr + info->ClassOffset),
                                    info->ClassLength );
            class[len] = 0;
        }
    }
    else status = STATUS_SUCCESS;

    if (subkeys) *subkeys = info->SubKeys;
    if (max_subkey) *max_subkey = info->MaxNameLen;
    if (max_class) *max_class = info->MaxClassLen;
    if (values) *values = info->Values;
    if (max_value) *max_value = info->MaxValueNameLen;
    if (max_data) *max_data = info->MaxValueDataLen;
    if (modif) *modif = *(FILETIME *)&info->LastWriteTime;

 done:
    if (buf_ptr != buffer) heap_free( buf_ptr );
    return RtlNtStatusToDosError( status );
}


/******************************************************************************
 * RegCloseKey   [ADVAPI32.@]
 *
 * Close an open registry key.
 *
 * PARAMS
 *  hkey [I] Handle of key to close
 *
 * RETURNS
 *  Success: ERROR_SUCCESS
 *  Failure: Error code
 */
LSTATUS WINAPI RegCloseKey( HKEY hkey )
{
    if (!hkey) return ERROR_INVALID_HANDLE;
    if (hkey >= (HKEY)0x80000000) return ERROR_SUCCESS;
    return RtlNtStatusToDosError( NtClose( hkey ) );
}


/******************************************************************************
 * RegDeleteKeyExW   [ADVAPI32.@]
 */
LSTATUS WINAPI RegDeleteKeyExW( HKEY hkey, LPCWSTR name, REGSAM access, DWORD reserved )
{
    DWORD ret;
    HKEY tmp;

    if (!name) return ERROR_INVALID_PARAMETER;

    if (!(hkey = get_special_root_hkey( hkey, access ))) return ERROR_INVALID_HANDLE;

    access &= KEY_WOW64_64KEY | KEY_WOW64_32KEY;
    if (!(ret = RegOpenKeyExW( hkey, name, 0, access | DELETE, &tmp )))
    {
        ret = RtlNtStatusToDosError( NtDeleteKey( tmp ) );
        RegCloseKey( tmp );
    }
    TRACE("%s ret=%08x\n", debugstr_w(name), ret);
    return ret;
}


/******************************************************************************
 * RegDeleteKeyW   [ADVAPI32.@]
 *
 * See RegDeleteKeyA.
 */
LSTATUS WINAPI RegDeleteKeyW( HKEY hkey, LPCWSTR name )
{
    return RegDeleteKeyExW( hkey, name, 0, 0 );
}


/******************************************************************************
 * RegDeleteKeyExA   [ADVAPI32.@]
 */
LSTATUS WINAPI RegDeleteKeyExA( HKEY hkey, LPCSTR name, REGSAM access, DWORD reserved )
{
    DWORD ret;
    HKEY tmp;

    if (!name) return ERROR_INVALID_PARAMETER;

    if (!(hkey = get_special_root_hkey( hkey, access ))) return ERROR_INVALID_HANDLE;

    access &= KEY_WOW64_64KEY | KEY_WOW64_32KEY;
    if (!(ret = RegOpenKeyExA( hkey, name, 0, access | DELETE, &tmp )))
    {
        if (!is_version_nt()) /* win95 does recursive key deletes */
        {
            CHAR sub[MAX_PATH];

            while(!RegEnumKeyA(tmp, 0, sub, sizeof(sub)))
            {
                if(RegDeleteKeyExA(tmp, sub, access, reserved))  /* recurse */
                    break;
            }
        }
        ret = RtlNtStatusToDosError( NtDeleteKey( tmp ) );
        RegCloseKey( tmp );
    }
    TRACE("%s ret=%08x\n", debugstr_a(name), ret);
    return ret;
}


/******************************************************************************
 * RegDeleteKeyA   [ADVAPI32.@]
 *
 * Delete a registry key.
 *
 * PARAMS
 *  hkey   [I] Handle to parent key containing the key to delete
 *  name   [I] Name of the key user hkey to delete
 *
 * NOTES
 *
 * MSDN is wrong when it says that hkey must be opened with the DELETE access
 * right. In reality, it opens a new handle with DELETE access.
 *
 * RETURNS
 *  Success: ERROR_SUCCESS
 *  Failure: Error code
 */
LSTATUS WINAPI RegDeleteKeyA( HKEY hkey, LPCSTR name )
{
    return RegDeleteKeyExA( hkey, name, 0, 0 );
}



/******************************************************************************
 * RegSetValueExW   [ADVAPI32.@]
 *
 * Set the data and contents of a registry value.
 *
 * PARAMS
 *  hkey       [I] Handle of key to set value for
 *  name       [I] Name of value to set
 *  reserved   [I] Reserved, must be zero
 *  type       [I] Type of the value being set
 *  data       [I] The new contents of the value to set
 *  count      [I] Size of data
 *
 * RETURNS
 *  Success: ERROR_SUCCESS
 *  Failure: Error code
 */
LSTATUS WINAPI RegSetValueExW( HKEY hkey, LPCWSTR name, DWORD reserved,
                            DWORD type, const BYTE *data, DWORD count )
{
    UNICODE_STRING nameW;

    /* no need for version check, not implemented on win9x anyway */

    if (data && ((ULONG_PTR)data >> 16) == 0) return ERROR_NOACCESS;

    if (count && is_string(type))
    {
        LPCWSTR str = (LPCWSTR)data;
        /* if user forgot to count terminating null, add it (yes NT does this) */
        if (str[count / sizeof(WCHAR) - 1] && !str[count / sizeof(WCHAR)])
            count += sizeof(WCHAR);
    }
    if (!(hkey = get_special_root_hkey( hkey, 0 ))) return ERROR_INVALID_HANDLE;

    RtlInitUnicodeString( &nameW, name );
    return RtlNtStatusToDosError( NtSetValueKey( hkey, &nameW, 0, type, data, count ) );
}


/******************************************************************************
 * RegSetValueExA   [ADVAPI32.@]
 *
 * See RegSetValueExW.
 *
 * NOTES
 *  win95 does not care about count for REG_SZ and finds out the len by itself (js)
 *  NT does definitely care (aj)
 */
LSTATUS WINAPI RegSetValueExA( HKEY hkey, LPCSTR name, DWORD reserved, DWORD type,
                            const BYTE *data, DWORD count )
{
    ANSI_STRING nameA;
    UNICODE_STRING nameW;
    WCHAR *dataW = NULL;
    NTSTATUS status;

    if (!is_version_nt())  /* win95 */
    {
        if (type == REG_SZ)
        {
            if (!data) return ERROR_INVALID_PARAMETER;
            count = strlen((const char *)data) + 1;
        }
    }
    else if (count && is_string(type))
    {
        /* if user forgot to count terminating null, add it (yes NT does this) */
        if (data[count-1] && !data[count]) count++;
    }

    if (!(hkey = get_special_root_hkey( hkey, 0 ))) return ERROR_INVALID_HANDLE;

    if (is_string( type )) /* need to convert to Unicode */
    {
        DWORD lenW;
        RtlMultiByteToUnicodeSize( &lenW, (const char *)data, count );
        if (!(dataW = heap_alloc( lenW ))) return ERROR_OUTOFMEMORY;
        RtlMultiByteToUnicodeN( dataW, lenW, NULL, (const char *)data, count );
        count = lenW;
        data = (BYTE *)dataW;
    }

    RtlInitAnsiString( &nameA, name );
    if (!(status = RtlAnsiStringToUnicodeString( &nameW, &nameA, TRUE )))
    {
        status = NtSetValueKey( hkey, &nameW, 0, type, data, count );
        RtlFreeUnicodeString( &nameW );
    }
    heap_free( dataW );
    return RtlNtStatusToDosError( status );
}


/******************************************************************************
 * RegSetValueW   [ADVAPI32.@]
 *
 * Sets the data for the default or unnamed value of a reg key.
 *
 * PARAMS
 *  hkey     [I] Handle to an open key.
 *  subkey   [I] Name of a subkey of hKey.
 *  type     [I] Type of information to store.
 *  data     [I] String that contains the data to set for the default value.
 *  count    [I] Ignored.
 *
 * RETURNS
 *  Success: ERROR_SUCCESS
 *  Failure: nonzero error code from Winerror.h
 */
LSTATUS WINAPI RegSetValueW( HKEY hkey, LPCWSTR subkey, DWORD type, LPCWSTR data, DWORD count )
{
    TRACE("(%p,%s,%d,%s,%d)\n", hkey, debugstr_w(subkey), type, debugstr_w(data), count );

    if (type != REG_SZ || !data) return ERROR_INVALID_PARAMETER;

    return RegSetKeyValueW( hkey, subkey, NULL, type, data, (strlenW(data) + 1)*sizeof(WCHAR) );
}

/******************************************************************************
 * RegSetValueA   [ADVAPI32.@]
 *
 * See RegSetValueW.
 */
LSTATUS WINAPI RegSetValueA( HKEY hkey, LPCSTR subkey, DWORD type, LPCSTR data, DWORD count )
{
    TRACE("(%p,%s,%d,%s,%d)\n", hkey, debugstr_a(subkey), type, debugstr_a(data), count );

    if (type != REG_SZ || !data) return ERROR_INVALID_PARAMETER;

    return RegSetKeyValueA( hkey, subkey, NULL, type, data, strlen(data) + 1 );
}

/******************************************************************************
 * RegSetKeyValueW   [ADVAPI32.@]
 */
LONG WINAPI RegSetKeyValueW( HKEY hkey, LPCWSTR subkey, LPCWSTR name, DWORD type, const void *data, DWORD len )
{
    HKEY hsubkey = NULL;
    DWORD ret;

    TRACE("(%p,%s,%s,%d,%p,%d)\n", hkey, debugstr_w(subkey), debugstr_w(name), type, data, len );

    if (subkey && subkey[0])  /* need to create the subkey */
    {
        if ((ret = RegCreateKeyW( hkey, subkey, &hsubkey )) != ERROR_SUCCESS) return ret;
        hkey = hsubkey;
    }

    ret = RegSetValueExW( hkey, name, 0, type, (const BYTE*)data, len );
    if (hsubkey) RegCloseKey( hsubkey );
    return ret;
}

/******************************************************************************
 * RegSetKeyValueA   [ADVAPI32.@]
 */
LONG WINAPI RegSetKeyValueA( HKEY hkey, LPCSTR subkey, LPCSTR name, DWORD type, const void *data, DWORD len )
{
    HKEY hsubkey = NULL;
    DWORD ret;

    TRACE("(%p,%s,%s,%d,%p,%d)\n", hkey, debugstr_a(subkey), debugstr_a(name), type, data, len );

    if (subkey && subkey[0])  /* need to create the subkey */
    {
        if ((ret = RegCreateKeyA( hkey, subkey, &hsubkey )) != ERROR_SUCCESS) return ret;
        hkey = hsubkey;
    }

    ret = RegSetValueExA( hkey, name, 0, type, (const BYTE*)data, len );
    if (hsubkey) RegCloseKey( hsubkey );
    return ret;
}

/******************************************************************************
 * RegQueryValueExW   [ADVAPI32.@]
 *
 * See RegQueryValueExA.
 */
LSTATUS WINAPI RegQueryValueExW( HKEY hkey, LPCWSTR name, LPDWORD reserved, LPDWORD type,
                              LPBYTE data, LPDWORD count )
{
    NTSTATUS status;
    UNICODE_STRING name_str;
    DWORD total_size;
    char buffer[256], *buf_ptr = buffer;
    KEY_VALUE_PARTIAL_INFORMATION *info = (KEY_VALUE_PARTIAL_INFORMATION *)buffer;
    static const int info_size = offsetof( KEY_VALUE_PARTIAL_INFORMATION, Data );

    TRACE("(%p,%s,%p,%p,%p,%p=%d)\n",
          hkey, debugstr_w(name), reserved, type, data, count,
          (count && data) ? *count : 0 );

    if ((data && !count) || reserved) return ERROR_INVALID_PARAMETER;
    if (!(hkey = get_special_root_hkey( hkey, 0 ))) return ERROR_INVALID_HANDLE;

    RtlInitUnicodeString( &name_str, name );

    if (data) total_size = min( sizeof(buffer), *count + info_size );
    else
    {
        total_size = info_size;
        if (count) *count = 0;
    }

    status = NtQueryValueKey( hkey, &name_str, KeyValuePartialInformation,
                              buffer, total_size, &total_size );
    if (status && status != STATUS_BUFFER_OVERFLOW) goto done;

    if (data)
    {
        /* retry with a dynamically allocated buffer */
        while (status == STATUS_BUFFER_OVERFLOW && total_size - info_size <= *count)
        {
            if (buf_ptr != buffer) heap_free( buf_ptr );
            if (!(buf_ptr = heap_alloc( total_size )))
                return ERROR_NOT_ENOUGH_MEMORY;
            info = (KEY_VALUE_PARTIAL_INFORMATION *)buf_ptr;
            status = NtQueryValueKey( hkey, &name_str, KeyValuePartialInformation,
                                      buf_ptr, total_size, &total_size );
        }

        if (!status)
        {
            memcpy( data, buf_ptr + info_size, total_size - info_size );
            /* if the type is REG_SZ and data is not 0-terminated
             * and there is enough space in the buffer NT appends a \0 */
            if (total_size - info_size <= *count-sizeof(WCHAR) && is_string(info->Type))
            {
                WCHAR *ptr = (WCHAR *)(data + total_size - info_size);
                if (ptr > (WCHAR *)data && ptr[-1]) *ptr = 0;
            }
        }
        else if (status != STATUS_BUFFER_OVERFLOW) goto done;
    }
    else status = STATUS_SUCCESS;

    if (type) *type = info->Type;
    if (count) *count = total_size - info_size;

 done:
    if (buf_ptr != buffer) heap_free( buf_ptr );
    return RtlNtStatusToDosError(status);
}


/******************************************************************************
 * RegQueryValueExA   [ADVAPI32.@]
 *
 * Get the type and contents of a specified value under with a key.
 *
 * PARAMS
 *  hkey      [I]   Handle of the key to query
 *  name      [I]   Name of value under hkey to query
 *  reserved  [I]   Reserved - must be NULL
 *  type      [O]   Destination for the value type, or NULL if not required
 *  data      [O]   Destination for the values contents, or NULL if not required
 *  count     [I/O] Size of data, updated with the number of bytes returned
 *
 * RETURNS
 *  Success: ERROR_SUCCESS. *count is updated with the number of bytes copied to data.
 *  Failure: ERROR_INVALID_HANDLE, if hkey is invalid.
 *           ERROR_INVALID_PARAMETER, if any other parameter is invalid.
 *           ERROR_MORE_DATA, if on input *count is too small to hold the contents.
 *                     
 * NOTES
 *   MSDN states that if data is too small it is partially filled. In reality 
 *   it remains untouched.
 */
LSTATUS WINAPI RegQueryValueExA( HKEY hkey, LPCSTR name, LPDWORD reserved, LPDWORD type,
                              LPBYTE data, LPDWORD count )
{
    NTSTATUS status;
    ANSI_STRING nameA;
    UNICODE_STRING nameW;
    DWORD total_size, datalen = 0;
    char buffer[256], *buf_ptr = buffer;
    KEY_VALUE_PARTIAL_INFORMATION *info = (KEY_VALUE_PARTIAL_INFORMATION *)buffer;
    static const int info_size = offsetof( KEY_VALUE_PARTIAL_INFORMATION, Data );

    TRACE("(%p,%s,%p,%p,%p,%p=%d)\n",
          hkey, debugstr_a(name), reserved, type, data, count, count ? *count : 0 );

    if ((data && !count) || reserved) return ERROR_INVALID_PARAMETER;
    if (!(hkey = get_special_root_hkey( hkey, 0 ))) return ERROR_INVALID_HANDLE;

    if (count) datalen = *count;
    if (!data && count) *count = 0;

    /* this matches Win9x behaviour - NT sets *type to a random value */
    if (type) *type = REG_NONE;

    RtlInitAnsiString( &nameA, name );
    if ((status = RtlAnsiStringToUnicodeString( &nameW, &nameA, TRUE )))
        return RtlNtStatusToDosError(status);

    status = NtQueryValueKey( hkey, &nameW, KeyValuePartialInformation,
                              buffer, sizeof(buffer), &total_size );
    if (status && status != STATUS_BUFFER_OVERFLOW) goto done;

    /* we need to fetch the contents for a string type even if not requested,
     * because we need to compute the length of the ASCII string. */
    if (data || is_string(info->Type))
    {
        /* retry with a dynamically allocated buffer */
        while (status == STATUS_BUFFER_OVERFLOW)
        {
            if (buf_ptr != buffer) heap_free( buf_ptr );
            if (!(buf_ptr = heap_alloc( total_size )))
            {
                status = STATUS_NO_MEMORY;
                goto done;
            }
            info = (KEY_VALUE_PARTIAL_INFORMATION *)buf_ptr;
            status = NtQueryValueKey( hkey, &nameW, KeyValuePartialInformation,
                                      buf_ptr, total_size, &total_size );
        }

        if (status) goto done;

        if (is_string(info->Type))
        {
            DWORD len;

            RtlUnicodeToMultiByteSize( &len, (WCHAR *)(buf_ptr + info_size),
                                       total_size - info_size );
            if (data && len)
            {
                if (len > datalen) status = STATUS_BUFFER_OVERFLOW;
                else
                {
                    RtlUnicodeToMultiByteN( (char*)data, len, NULL, (WCHAR *)(buf_ptr + info_size),
                                            total_size - info_size );
                    /* if the type is REG_SZ and data is not 0-terminated
                     * and there is enough space in the buffer NT appends a \0 */
                    if (len < datalen && data[len-1]) data[len] = 0;
                }
            }
            total_size = len + info_size;
        }
        else if (data)
        {
            if (total_size - info_size > datalen) status = STATUS_BUFFER_OVERFLOW;
            else memcpy( data, buf_ptr + info_size, total_size - info_size );
        }
    }
    else status = STATUS_SUCCESS;

    if (type) *type = info->Type;
    if (count) *count = total_size - info_size;

 done:
    if (buf_ptr != buffer) heap_free( buf_ptr );
    RtlFreeUnicodeString( &nameW );
    return RtlNtStatusToDosError(status);
}


/******************************************************************************
 * RegQueryValueW   [ADVAPI32.@]
 *
 * Retrieves the data associated with the default or unnamed value of a key.
 *
 * PARAMS
 *  hkey      [I] Handle to an open key.
 *  name      [I] Name of the subkey of hKey.
 *  data      [O] Receives the string associated with the default value
 *                of the key.
 *  count     [I/O] Size of lpValue in bytes.
 *
 *  RETURNS
 *   Success: ERROR_SUCCESS
 *   Failure: nonzero error code from Winerror.h
 */
LSTATUS WINAPI RegQueryValueW( HKEY hkey, LPCWSTR name, LPWSTR data, LPLONG count )
{
    DWORD ret;
    HKEY subkey = hkey;

    TRACE("(%p,%s,%p,%d)\n", hkey, debugstr_w(name), data, count ? *count : 0 );

    if (name && name[0])
    {
        if ((ret = RegOpenKeyW( hkey, name, &subkey )) != ERROR_SUCCESS) return ret;
    }
    ret = RegQueryValueExW( subkey, NULL, NULL, NULL, (LPBYTE)data, (LPDWORD)count );
    if (subkey != hkey) RegCloseKey( subkey );
    if (ret == ERROR_FILE_NOT_FOUND)
    {
        /* return empty string if default value not found */
        if (data) *data = 0;
        if (count) *count = sizeof(WCHAR);
        ret = ERROR_SUCCESS;
    }
    return ret;
}


/******************************************************************************
 * RegQueryValueA   [ADVAPI32.@]
 *
 * See RegQueryValueW.
 */
LSTATUS WINAPI RegQueryValueA( HKEY hkey, LPCSTR name, LPSTR data, LPLONG count )
{
    DWORD ret;
    HKEY subkey = hkey;

    TRACE("(%p,%s,%p,%d)\n", hkey, debugstr_a(name), data, count ? *count : 0 );

    if (name && name[0])
    {
        if ((ret = RegOpenKeyA( hkey, name, &subkey )) != ERROR_SUCCESS) return ret;
    }
    ret = RegQueryValueExA( subkey, NULL, NULL, NULL, (LPBYTE)data, (LPDWORD)count );
    if (subkey != hkey) RegCloseKey( subkey );
    if (ret == ERROR_FILE_NOT_FOUND)
    {
        /* return empty string if default value not found */
        if (data) *data = 0;
        if (count) *count = 1;
        ret = ERROR_SUCCESS;
    }
    return ret;
}


/******************************************************************************
 * ADVAPI_ApplyRestrictions   [internal]
 *
 * Helper function for RegGetValueA/W.
 */
static VOID ADVAPI_ApplyRestrictions( DWORD dwFlags, DWORD dwType,
                                      DWORD cbData, PLONG ret )
{
    /* Check if the type is restricted by the passed flags */
    if (*ret == ERROR_SUCCESS || *ret == ERROR_MORE_DATA)
    {
        DWORD dwMask = 0;

        switch (dwType)
        {
        case REG_NONE: dwMask = RRF_RT_REG_NONE; break;
        case REG_SZ: dwMask = RRF_RT_REG_SZ; break;
        case REG_EXPAND_SZ: dwMask = RRF_RT_REG_EXPAND_SZ; break;
        case REG_MULTI_SZ: dwMask = RRF_RT_REG_MULTI_SZ; break;
        case REG_BINARY: dwMask = RRF_RT_REG_BINARY; break;
        case REG_DWORD: dwMask = RRF_RT_REG_DWORD; break;
        case REG_QWORD: dwMask = RRF_RT_REG_QWORD; break;
        }

        if (dwFlags & dwMask)
        {
            /* Type is not restricted, check for size mismatch */
            if (dwType == REG_BINARY)
            {
                DWORD cbExpect = 0;

                if ((dwFlags & RRF_RT_ANY) == RRF_RT_DWORD)
                    cbExpect = 4;
                else if ((dwFlags & RRF_RT_ANY) == RRF_RT_QWORD)
                    cbExpect = 8;

                if (cbExpect && cbData != cbExpect)
                    *ret = ERROR_DATATYPE_MISMATCH;
            }
        }
        else *ret = ERROR_UNSUPPORTED_TYPE;
    }
}


/******************************************************************************
 * RegGetValueW   [ADVAPI32.@]
 *
 * Retrieves the type and data for a value name associated with a key,
 * optionally expanding its content and restricting its type.
 *
 * PARAMS
 *  hKey      [I] Handle to an open key.
 *  pszSubKey [I] Name of the subkey of hKey.
 *  pszValue  [I] Name of value under hKey/szSubKey to query.
 *  dwFlags   [I] Flags restricting the value type to retrieve.
 *  pdwType   [O] Destination for the values type, may be NULL.
 *  pvData    [O] Destination for the values content, may be NULL.
 *  pcbData   [I/O] Size of pvData, updated with the size in bytes required to
 *                  retrieve the whole content, including the trailing '\0'
 *                  for strings.
 *
 * RETURNS
 *  Success: ERROR_SUCCESS
 *  Failure: nonzero error code from Winerror.h
 *
 * NOTES
 *  - Unless RRF_NOEXPAND is specified, REG_EXPAND_SZ values are automatically
 *    expanded and pdwType is set to REG_SZ instead.
 *  - Restrictions are applied after expanding, using RRF_RT_REG_EXPAND_SZ 
 *    without RRF_NOEXPAND is thus not allowed.
 *    An exception is the case where RRF_RT_ANY is specified, because then
 *    RRF_NOEXPAND is allowed.
 */
LSTATUS WINAPI RegGetValueW( HKEY hKey, LPCWSTR pszSubKey, LPCWSTR pszValue,
                          DWORD dwFlags, LPDWORD pdwType, PVOID pvData,
                          LPDWORD pcbData )
{
    DWORD dwType, cbData = pcbData ? *pcbData : 0;
    PVOID pvBuf = NULL;
    LONG ret;

    TRACE("(%p,%s,%s,%d,%p,%p,%p=%d)\n", 
          hKey, debugstr_w(pszSubKey), debugstr_w(pszValue), dwFlags, pdwType,
          pvData, pcbData, cbData);

    if (pvData && !pcbData)
        return ERROR_INVALID_PARAMETER;
    if ((dwFlags & RRF_RT_REG_EXPAND_SZ) && !(dwFlags & RRF_NOEXPAND) &&
            ((dwFlags & RRF_RT_ANY) != RRF_RT_ANY))
        return ERROR_INVALID_PARAMETER;

    if (pszSubKey && pszSubKey[0])
    {
        ret = RegOpenKeyExW(hKey, pszSubKey, 0, KEY_QUERY_VALUE, &hKey);
        if (ret != ERROR_SUCCESS) return ret;
    }

    ret = RegQueryValueExW(hKey, pszValue, NULL, &dwType, pvData, &cbData);
    
    /* If we are going to expand we need to read in the whole the value even
     * if the passed buffer was too small as the expanded string might be
     * smaller than the unexpanded one and could fit into cbData bytes. */
    if ((ret == ERROR_SUCCESS || ret == ERROR_MORE_DATA) &&
        dwType == REG_EXPAND_SZ && !(dwFlags & RRF_NOEXPAND))
    {
        do {
            heap_free(pvBuf);
            
            pvBuf = heap_alloc(cbData);
            if (!pvBuf)
            {
                ret = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            if (ret == ERROR_MORE_DATA || !pvData)
                ret = RegQueryValueExW(hKey, pszValue, NULL, 
                                       &dwType, pvBuf, &cbData);
            else
            {
                /* Even if cbData was large enough we have to copy the 
                 * string since ExpandEnvironmentStrings can't handle
                 * overlapping buffers. */
                CopyMemory(pvBuf, pvData, cbData);
            }

            /* Both the type or the value itself could have been modified in
             * between so we have to keep retrying until the buffer is large
             * enough or we no longer have to expand the value. */
        } while (dwType == REG_EXPAND_SZ && ret == ERROR_MORE_DATA);

        if (ret == ERROR_SUCCESS)
        {
            /* Recheck dwType in case it changed since the first call */
            if (dwType == REG_EXPAND_SZ)
            {
                cbData = ExpandEnvironmentStringsW(pvBuf, pvData,
                                                   pcbData ? *pcbData : 0) * sizeof(WCHAR);
                dwType = REG_SZ;
                if(pvData && pcbData && cbData > *pcbData)
                    ret = ERROR_MORE_DATA;
            }
            else if (pvData)
                CopyMemory(pvData, pvBuf, *pcbData);
        }

        heap_free(pvBuf);
    }

    if (pszSubKey && pszSubKey[0])
        RegCloseKey(hKey);

    ADVAPI_ApplyRestrictions(dwFlags, dwType, cbData, &ret);

    if (pvData && ret != ERROR_SUCCESS && (dwFlags & RRF_ZEROONFAILURE))
        ZeroMemory(pvData, *pcbData);

    if (pdwType) *pdwType = dwType;
    if (pcbData) *pcbData = cbData;

    return ret;
}


/******************************************************************************
 * RegGetValueA   [ADVAPI32.@]
 *
 * See RegGetValueW.
 */
LSTATUS WINAPI RegGetValueA( HKEY hKey, LPCSTR pszSubKey, LPCSTR pszValue,
                          DWORD dwFlags, LPDWORD pdwType, PVOID pvData, 
                          LPDWORD pcbData )
{
    DWORD dwType, cbData = pcbData ? *pcbData : 0;
    PVOID pvBuf = NULL;
    LONG ret;

    TRACE("(%p,%s,%s,%d,%p,%p,%p=%d)\n", 
          hKey, debugstr_a(pszSubKey), debugstr_a(pszValue), dwFlags,
          pdwType, pvData, pcbData, cbData);

    if (pvData && !pcbData)
        return ERROR_INVALID_PARAMETER;
    if ((dwFlags & RRF_RT_REG_EXPAND_SZ) && !(dwFlags & RRF_NOEXPAND) &&
            ((dwFlags & RRF_RT_ANY) != RRF_RT_ANY))
        return ERROR_INVALID_PARAMETER;

    if (pszSubKey && pszSubKey[0])
    {
        ret = RegOpenKeyExA(hKey, pszSubKey, 0, KEY_QUERY_VALUE, &hKey);
        if (ret != ERROR_SUCCESS) return ret;
    }

    ret = RegQueryValueExA(hKey, pszValue, NULL, &dwType, pvData, &cbData);

    /* If we are going to expand we need to read in the whole the value even
     * if the passed buffer was too small as the expanded string might be
     * smaller than the unexpanded one and could fit into cbData bytes. */
    if ((ret == ERROR_SUCCESS || ret == ERROR_MORE_DATA) &&
        dwType == REG_EXPAND_SZ && !(dwFlags & RRF_NOEXPAND))
    {
        do {
            heap_free(pvBuf);

            pvBuf = heap_alloc(cbData);
            if (!pvBuf)
            {
                ret = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            if (ret == ERROR_MORE_DATA || !pvData)
                ret = RegQueryValueExA(hKey, pszValue, NULL, 
                                       &dwType, pvBuf, &cbData);
            else
            {
                /* Even if cbData was large enough we have to copy the 
                 * string since ExpandEnvironmentStrings can't handle
                 * overlapping buffers. */
                CopyMemory(pvBuf, pvData, cbData);
            }

            /* Both the type or the value itself could have been modified in
             * between so we have to keep retrying until the buffer is large
             * enough or we no longer have to expand the value. */
        } while (dwType == REG_EXPAND_SZ && ret == ERROR_MORE_DATA);

        if (ret == ERROR_SUCCESS)
        {
            /* Recheck dwType in case it changed since the first call */
            if (dwType == REG_EXPAND_SZ)
            {
                cbData = ExpandEnvironmentStringsA(pvBuf, pvData,
                                                   pcbData ? *pcbData : 0);
                dwType = REG_SZ;
                if(pvData && pcbData && cbData > *pcbData)
                    ret = ERROR_MORE_DATA;
            }
            else if (pvData)
                CopyMemory(pvData, pvBuf, *pcbData);
        }

        heap_free(pvBuf);
    }

    if (pszSubKey && pszSubKey[0])
        RegCloseKey(hKey);

    ADVAPI_ApplyRestrictions(dwFlags, dwType, cbData, &ret);

    if (pvData && ret != ERROR_SUCCESS && (dwFlags & RRF_ZEROONFAILURE))
        ZeroMemory(pvData, *pcbData);

    if (pdwType) *pdwType = dwType;
    if (pcbData) *pcbData = cbData;

    return ret;
}


/******************************************************************************
 * RegEnumValueW   [ADVAPI32.@]
 *
 * Enumerates the values for the specified open registry key.
 *
 * PARAMS
 *  hkey       [I] Handle to key to query
 *  index      [I] Index of value to query
 *  value      [O] Value string
 *  val_count  [I/O] Size of value buffer (in wchars)
 *  reserved   [I] Reserved
 *  type       [O] Type code
 *  data       [O] Value data
 *  count      [I/O] Size of data buffer (in bytes)
 *
 * RETURNS
 *  Success: ERROR_SUCCESS
 *  Failure: nonzero error code from Winerror.h
 */

LSTATUS WINAPI RegEnumValueW( HKEY hkey, DWORD index, LPWSTR value, LPDWORD val_count,
                           LPDWORD reserved, LPDWORD type, LPBYTE data, LPDWORD count )
{
    NTSTATUS status;
    DWORD total_size;
    char buffer[256], *buf_ptr = buffer;
    KEY_VALUE_FULL_INFORMATION *info = (KEY_VALUE_FULL_INFORMATION *)buffer;
    static const int info_size = offsetof( KEY_VALUE_FULL_INFORMATION, Name );

    TRACE("(%p,%d,%p,%p,%p,%p,%p,%p)\n",
          hkey, index, value, val_count, reserved, type, data, count );

    /* NT only checks count, not val_count */
    if ((data && !count) || reserved) return ERROR_INVALID_PARAMETER;
    if (!(hkey = get_special_root_hkey( hkey, 0 ))) return ERROR_INVALID_HANDLE;

    total_size = info_size + (MAX_PATH + 1) * sizeof(WCHAR);
    if (data) total_size += *count;
    total_size = min( sizeof(buffer), total_size );

    status = NtEnumerateValueKey( hkey, index, KeyValueFullInformation,
                                  buffer, total_size, &total_size );
    if (status && status != STATUS_BUFFER_OVERFLOW) goto done;

    if (value || data)
    {
        /* retry with a dynamically allocated buffer */
        while (status == STATUS_BUFFER_OVERFLOW)
        {
            if (buf_ptr != buffer) heap_free( buf_ptr );
            if (!(buf_ptr = heap_alloc( total_size )))
                return ERROR_NOT_ENOUGH_MEMORY;
            info = (KEY_VALUE_FULL_INFORMATION *)buf_ptr;
            status = NtEnumerateValueKey( hkey, index, KeyValueFullInformation,
                                          buf_ptr, total_size, &total_size );
        }

        if (status) goto done;

        if (value)
        {
            if (info->NameLength/sizeof(WCHAR) >= *val_count)
            {
                status = STATUS_BUFFER_OVERFLOW;
                goto overflow;
            }
            memcpy( value, info->Name, info->NameLength );
            *val_count = info->NameLength / sizeof(WCHAR);
            value[*val_count] = 0;
        }

        if (data)
        {
            if (total_size - info->DataOffset > *count)
            {
                status = STATUS_BUFFER_OVERFLOW;
                goto overflow;
            }
            memcpy( data, buf_ptr + info->DataOffset, total_size - info->DataOffset );
            if (total_size - info->DataOffset <= *count-sizeof(WCHAR) && is_string(info->Type))
            {
                /* if the type is REG_SZ and data is not 0-terminated
                 * and there is enough space in the buffer NT appends a \0 */
                WCHAR *ptr = (WCHAR *)(data + total_size - info->DataOffset);
                if (ptr > (WCHAR *)data && ptr[-1]) *ptr = 0;
            }
        }
    }
    else status = STATUS_SUCCESS;

 overflow:
    if (type) *type = info->Type;
    if (count) *count = info->DataLength;

 done:
    if (buf_ptr != buffer) heap_free( buf_ptr );
    return RtlNtStatusToDosError(status);
}


/******************************************************************************
 * RegEnumValueA   [ADVAPI32.@]
 *
 * See RegEnumValueW.
 */
LSTATUS WINAPI RegEnumValueA( HKEY hkey, DWORD index, LPSTR value, LPDWORD val_count,
                           LPDWORD reserved, LPDWORD type, LPBYTE data, LPDWORD count )
{
    NTSTATUS status;
    DWORD total_size;
    char buffer[256], *buf_ptr = buffer;
    KEY_VALUE_FULL_INFORMATION *info = (KEY_VALUE_FULL_INFORMATION *)buffer;
    static const int info_size = offsetof( KEY_VALUE_FULL_INFORMATION, Name );

    TRACE("(%p,%d,%p,%p,%p,%p,%p,%p)\n",
          hkey, index, value, val_count, reserved, type, data, count );

    /* NT only checks count, not val_count */
    if ((data && !count) || reserved) return ERROR_INVALID_PARAMETER;
    if (!(hkey = get_special_root_hkey( hkey, 0 ))) return ERROR_INVALID_HANDLE;

    total_size = info_size + (MAX_PATH + 1) * sizeof(WCHAR);
    if (data) total_size += *count;
    total_size = min( sizeof(buffer), total_size );

    status = NtEnumerateValueKey( hkey, index, KeyValueFullInformation,
                                  buffer, total_size, &total_size );
    if (status && status != STATUS_BUFFER_OVERFLOW) goto done;

    /* we need to fetch the contents for a string type even if not requested,
     * because we need to compute the length of the ASCII string. */
    if (value || data || is_string(info->Type))
    {
        /* retry with a dynamically allocated buffer */
        while (status == STATUS_BUFFER_OVERFLOW)
        {
            if (buf_ptr != buffer) heap_free( buf_ptr );
            if (!(buf_ptr = heap_alloc( total_size )))
                return ERROR_NOT_ENOUGH_MEMORY;
            info = (KEY_VALUE_FULL_INFORMATION *)buf_ptr;
            status = NtEnumerateValueKey( hkey, index, KeyValueFullInformation,
                                          buf_ptr, total_size, &total_size );
        }

        if (status) goto done;

        if (is_string(info->Type))
        {
            DWORD len;
            RtlUnicodeToMultiByteSize( &len, (WCHAR *)(buf_ptr + info->DataOffset),
                                       total_size - info->DataOffset );
            if (data && len)
            {
                if (len > *count) status = STATUS_BUFFER_OVERFLOW;
                else
                {
                    RtlUnicodeToMultiByteN( (char*)data, len, NULL, (WCHAR *)(buf_ptr + info->DataOffset),
                                            total_size - info->DataOffset );
                    /* if the type is REG_SZ and data is not 0-terminated
                     * and there is enough space in the buffer NT appends a \0 */
                    if (len < *count && data[len-1]) data[len] = 0;
                }
            }
            info->DataLength = len;
        }
        else if (data)
        {
            if (total_size - info->DataOffset > *count) status = STATUS_BUFFER_OVERFLOW;
            else memcpy( data, buf_ptr + info->DataOffset, total_size - info->DataOffset );
        }

        if (value && !status)
        {
            DWORD len;

            RtlUnicodeToMultiByteSize( &len, info->Name, info->NameLength );
            if (len >= *val_count)
            {
                status = STATUS_BUFFER_OVERFLOW;
                if (*val_count)
                {
                    len = *val_count - 1;
                    RtlUnicodeToMultiByteN( value, len, NULL, info->Name, info->NameLength );
                    value[len] = 0;
                }
            }
            else
            {
                RtlUnicodeToMultiByteN( value, len, NULL, info->Name, info->NameLength );
                value[len] = 0;
                *val_count = len;
            }
        }
    }
    else status = STATUS_SUCCESS;

    if (type) *type = info->Type;
    if (count) *count = info->DataLength;

 done:
    if (buf_ptr != buffer) heap_free( buf_ptr );
    return RtlNtStatusToDosError(status);
}

/******************************************************************************
 * RegDeleteValueW   [ADVAPI32.@]
 *
 * See RegDeleteValueA.
 */
LSTATUS WINAPI RegDeleteValueW( HKEY hkey, LPCWSTR name )
{
    return RegDeleteKeyValueW( hkey, NULL, name );
}

/******************************************************************************
 * RegDeleteValueA   [ADVAPI32.@]
 *
 * Delete a value from the registry.
 *
 * PARAMS
 *  hkey [I] Registry handle of the key holding the value
 *  name [I] Name of the value under hkey to delete
 *
 * RETURNS
 *  Success: ERROR_SUCCESS
 *  Failure: nonzero error code from Winerror.h
 */
LSTATUS WINAPI RegDeleteValueA( HKEY hkey, LPCSTR name )
{
    return RegDeleteKeyValueA( hkey, NULL, name );
}

/******************************************************************************
 * RegDeleteKeyValueW   [ADVAPI32.@]
 */
LONG WINAPI RegDeleteKeyValueW( HKEY hkey, LPCWSTR subkey, LPCWSTR name )
{
    UNICODE_STRING nameW;
    HKEY hsubkey = 0;
    LONG ret;

    if (!(hkey = get_special_root_hkey( hkey, 0 ))) return ERROR_INVALID_HANDLE;

    if (subkey)
    {
        if ((ret = RegOpenKeyExW( hkey, subkey, 0, KEY_SET_VALUE, &hsubkey )))
            return ret;
        hkey = hsubkey;
    }

    RtlInitUnicodeString( &nameW, name );
    ret = RtlNtStatusToDosError( NtDeleteValueKey( hkey, &nameW ) );
    if (hsubkey) RegCloseKey( hsubkey );
    return ret;
}

/******************************************************************************
 * RegDeleteKeyValueA   [ADVAPI32.@]
 */
LONG WINAPI RegDeleteKeyValueA( HKEY hkey, LPCSTR subkey, LPCSTR name )
{
    UNICODE_STRING nameW;
    HKEY hsubkey = 0;
    ANSI_STRING nameA;
    NTSTATUS status;

    if (!(hkey = get_special_root_hkey( hkey, 0 ))) return ERROR_INVALID_HANDLE;

    if (subkey)
    {
        LONG ret = RegOpenKeyExA( hkey, subkey, 0, KEY_SET_VALUE, &hsubkey );
        if (ret)
            return ret;
        hkey = hsubkey;
    }

    RtlInitAnsiString( &nameA, name );
    if (!(status = RtlAnsiStringToUnicodeString( &nameW, &nameA, TRUE )))
    {
        status = NtDeleteValueKey( hkey, &nameW );
        RtlFreeUnicodeString( &nameW );
    }

    if (hsubkey) RegCloseKey( hsubkey );
    return RtlNtStatusToDosError( status );
}

/******************************************************************************
 * RegLoadKeyW   [ADVAPI32.@]
 *
 * Create a subkey under HKEY_USERS or HKEY_LOCAL_MACHINE and store
 * registration information from a specified file into that subkey.
 *
 * PARAMS
 *  hkey      [I] Handle of open key
 *  subkey    [I] Address of name of subkey
 *  filename  [I] Address of filename for registry information
 *
 * RETURNS
 *  Success: ERROR_SUCCESS
 *  Failure: nonzero error code from Winerror.h
 */
LSTATUS WINAPI RegLoadKeyW( HKEY hkey, LPCWSTR subkey, LPCWSTR filename )
{
    OBJECT_ATTRIBUTES destkey, file;
    UNICODE_STRING subkeyW, filenameW;
    NTSTATUS status;

    if (!(hkey = get_special_root_hkey(hkey, 0))) return ERROR_INVALID_HANDLE;

    destkey.Length = sizeof(destkey);
    destkey.RootDirectory = hkey;               /* root key: HKLM or HKU */
    destkey.ObjectName = &subkeyW;              /* name of the key */
    destkey.Attributes = 0;
    destkey.SecurityDescriptor = NULL;
    destkey.SecurityQualityOfService = NULL;
    RtlInitUnicodeString(&subkeyW, subkey);

    file.Length = sizeof(file);
    file.RootDirectory = NULL;
    file.ObjectName = &filenameW;               /* file containing the hive */
    file.Attributes = OBJ_CASE_INSENSITIVE;
    file.SecurityDescriptor = NULL;
    file.SecurityQualityOfService = NULL;
    RtlDosPathNameToNtPathName_U(filename, &filenameW, NULL, NULL);

    status = NtLoadKey(&destkey, &file);
    RtlFreeUnicodeString(&filenameW);
    return RtlNtStatusToDosError( status );
}


/******************************************************************************
 * RegLoadKeyA   [ADVAPI32.@]
 *
 * See RegLoadKeyW.
 */
LSTATUS WINAPI RegLoadKeyA( HKEY hkey, LPCSTR subkey, LPCSTR filename )
{
    UNICODE_STRING subkeyW, filenameW;
    STRING subkeyA, filenameA;
    NTSTATUS status;
    LONG ret;

    RtlInitAnsiString(&subkeyA, subkey);
    RtlInitAnsiString(&filenameA, filename);

    RtlInitUnicodeString(&subkeyW, NULL);
    RtlInitUnicodeString(&filenameW, NULL);
    if (!(status = RtlAnsiStringToUnicodeString(&subkeyW, &subkeyA, TRUE)) &&
        !(status = RtlAnsiStringToUnicodeString(&filenameW, &filenameA, TRUE)))
    {
        ret = RegLoadKeyW(hkey, subkeyW.Buffer, filenameW.Buffer);
    }
    else ret = RtlNtStatusToDosError(status);
    RtlFreeUnicodeString(&subkeyW);
    RtlFreeUnicodeString(&filenameW);
    return ret;
}


/******************************************************************************
 * RegSaveKeyW   [ADVAPI32.@]
 *
 * Save a key and all of its subkeys and values to a new file in the standard format.
 *
 * PARAMS
 *  hkey   [I] Handle of key where save begins
 *  lpFile [I] Address of filename to save to
 *  sa     [I] Address of security structure
 *
 * RETURNS
 *  Success: ERROR_SUCCESS
 *  Failure: nonzero error code from Winerror.h
 */
LSTATUS WINAPI RegSaveKeyW( HKEY hkey, LPCWSTR file, LPSECURITY_ATTRIBUTES sa )
{
    static const WCHAR format[] =
        {'r','e','g','%','0','4','x','.','t','m','p',0};
    WCHAR buffer[MAX_PATH];
    int count = 0;
    LPWSTR nameW;
    DWORD ret, err;
    HANDLE handle;

    TRACE( "(%p,%s,%p)\n", hkey, debugstr_w(file), sa );

    if (!file || !*file) return ERROR_INVALID_PARAMETER;
    if (!(hkey = get_special_root_hkey( hkey, 0 ))) return ERROR_INVALID_HANDLE;

    err = GetLastError();
    GetFullPathNameW( file, sizeof(buffer)/sizeof(WCHAR), buffer, &nameW );

    for (;;)
    {
        snprintfW( nameW, 16, format, count++ );
        handle = CreateFileW( buffer, GENERIC_WRITE, 0, NULL,
                            CREATE_NEW, FILE_ATTRIBUTE_NORMAL, 0 );
        if (handle != INVALID_HANDLE_VALUE) break;
        if ((ret = GetLastError()) != ERROR_FILE_EXISTS) goto done;

        /* Something gone haywire ? Please report if this happens abnormally */
        if (count >= 100)
            MESSAGE("Wow, we are already fiddling with a temp file %s with an ordinal as high as %d !\nYou might want to delete all corresponding temp files in that directory.\n", debugstr_w(buffer), count);
    }

    ret = RtlNtStatusToDosError(NtSaveKey(hkey, handle));

    CloseHandle( handle );
    if (!ret)
    {
        if (!MoveFileExW( buffer, file, MOVEFILE_REPLACE_EXISTING ))
        {
            ERR( "Failed to move %s to %s\n", debugstr_w(buffer),
	        debugstr_w(file) );
            ret = GetLastError();
        }
    }
    if (ret) DeleteFileW( buffer );

done:
    SetLastError( err );  /* restore last error code */
    return ret;
}


/******************************************************************************
 * RegSaveKeyA  [ADVAPI32.@]
 *
 * See RegSaveKeyW.
 */
LSTATUS WINAPI RegSaveKeyA( HKEY hkey, LPCSTR file, LPSECURITY_ATTRIBUTES sa )
{
    UNICODE_STRING *fileW = &NtCurrentTeb()->StaticUnicodeString;
    NTSTATUS status;
    STRING fileA;

    RtlInitAnsiString(&fileA, file);
    if ((status = RtlAnsiStringToUnicodeString(fileW, &fileA, FALSE)))
        return RtlNtStatusToDosError( status );
    return RegSaveKeyW(hkey, fileW->Buffer, sa);
}


/******************************************************************************
 * RegRestoreKeyW [ADVAPI32.@]
 *
 * Read the registry information from a file and copy it over a key.
 *
 * PARAMS
 *  hkey    [I] Handle of key where restore begins
 *  lpFile  [I] Address of filename containing saved tree
 *  dwFlags [I] Optional flags
 *
 * RETURNS
 *  Success: ERROR_SUCCESS
 *  Failure: nonzero error code from Winerror.h
 */
LSTATUS WINAPI RegRestoreKeyW( HKEY hkey, LPCWSTR lpFile, DWORD dwFlags )
{
    TRACE("(%p,%s,%d)\n",hkey,debugstr_w(lpFile),dwFlags);

    /* It seems to do this check before the hkey check */
    if (!lpFile || !*lpFile)
        return ERROR_INVALID_PARAMETER;

    FIXME("(%p,%s,%d): stub\n",hkey,debugstr_w(lpFile),dwFlags);

    /* Check for file existence */

    return ERROR_SUCCESS;
}


/******************************************************************************
 * RegRestoreKeyA [ADVAPI32.@]
 *
 * See RegRestoreKeyW.
 */
LSTATUS WINAPI RegRestoreKeyA( HKEY hkey, LPCSTR lpFile, DWORD dwFlags )
{
    UNICODE_STRING lpFileW;
    LONG ret;

    RtlCreateUnicodeStringFromAsciiz( &lpFileW, lpFile );
    ret = RegRestoreKeyW( hkey, lpFileW.Buffer, dwFlags );
    RtlFreeUnicodeString( &lpFileW );
    return ret;
}


/******************************************************************************
 * RegUnLoadKeyW [ADVAPI32.@]
 *
 * Unload a registry key and its subkeys from the registry.
 *
 * PARAMS
 *  hkey     [I] Handle of open key
 *  lpSubKey [I] Address of name of subkey to unload
 *
 * RETURNS
 *  Success: ERROR_SUCCESS
 *  Failure: nonzero error code from Winerror.h
 */
LSTATUS WINAPI RegUnLoadKeyW( HKEY hkey, LPCWSTR lpSubKey )
{
    DWORD ret;
    HKEY shkey;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING subkey;

    TRACE("(%p,%s)\n",hkey, debugstr_w(lpSubKey));

    ret = RegOpenKeyW(hkey,lpSubKey,&shkey);
    if( ret )
        return ERROR_INVALID_PARAMETER;

    RtlInitUnicodeString(&subkey, lpSubKey);
    InitializeObjectAttributes(&attr, &subkey, OBJ_CASE_INSENSITIVE, shkey, NULL);
    ret = RtlNtStatusToDosError(NtUnloadKey(&attr));

    RegCloseKey(shkey);

    return ret;
}


/******************************************************************************
 * RegUnLoadKeyA [ADVAPI32.@]
 *
 * See RegUnLoadKeyW.
 */
LSTATUS WINAPI RegUnLoadKeyA( HKEY hkey, LPCSTR lpSubKey )
{
    UNICODE_STRING lpSubKeyW;
    LONG ret;

    RtlCreateUnicodeStringFromAsciiz( &lpSubKeyW, lpSubKey );
    ret = RegUnLoadKeyW( hkey, lpSubKeyW.Buffer );
    RtlFreeUnicodeString( &lpSubKeyW );
    return ret;
}


/******************************************************************************
 * RegReplaceKeyW [ADVAPI32.@]
 *
 * Replace the file backing a registry key and all its subkeys with another file.
 *
 * PARAMS
 *  hkey      [I] Handle of open key
 *  lpSubKey  [I] Address of name of subkey
 *  lpNewFile [I] Address of filename for file with new data
 *  lpOldFile [I] Address of filename for backup file
 *
 * RETURNS
 *  Success: ERROR_SUCCESS
 *  Failure: nonzero error code from Winerror.h
 */
LSTATUS WINAPI RegReplaceKeyW( HKEY hkey, LPCWSTR lpSubKey, LPCWSTR lpNewFile,
                              LPCWSTR lpOldFile )
{
    FIXME("(%p,%s,%s,%s): stub\n", hkey, debugstr_w(lpSubKey),
          debugstr_w(lpNewFile),debugstr_w(lpOldFile));
    return ERROR_SUCCESS;
}


/******************************************************************************
 * RegReplaceKeyA [ADVAPI32.@]
 *
 * See RegReplaceKeyW.
 */
LSTATUS WINAPI RegReplaceKeyA( HKEY hkey, LPCSTR lpSubKey, LPCSTR lpNewFile,
                              LPCSTR lpOldFile )
{
    UNICODE_STRING lpSubKeyW;
    UNICODE_STRING lpNewFileW;
    UNICODE_STRING lpOldFileW;
    LONG ret;

    RtlCreateUnicodeStringFromAsciiz( &lpSubKeyW, lpSubKey );
    RtlCreateUnicodeStringFromAsciiz( &lpOldFileW, lpOldFile );
    RtlCreateUnicodeStringFromAsciiz( &lpNewFileW, lpNewFile );
    ret = RegReplaceKeyW( hkey, lpSubKeyW.Buffer, lpNewFileW.Buffer, lpOldFileW.Buffer );
    RtlFreeUnicodeString( &lpOldFileW );
    RtlFreeUnicodeString( &lpNewFileW );
    RtlFreeUnicodeString( &lpSubKeyW );
    return ret;
}


/******************************************************************************
 * RegSetKeySecurity [ADVAPI32.@]
 *
 * Set the security of an open registry key.
 *
 * PARAMS
 *  hkey          [I] Open handle of key to set
 *  SecurityInfo  [I] Descriptor contents
 *  pSecurityDesc [I] Address of descriptor for key
 *
 * RETURNS
 *  Success: ERROR_SUCCESS
 *  Failure: nonzero error code from Winerror.h
 */
LSTATUS WINAPI RegSetKeySecurity( HKEY hkey, SECURITY_INFORMATION SecurityInfo,
                               PSECURITY_DESCRIPTOR pSecurityDesc )
{
    TRACE("(%p,%d,%p)\n",hkey,SecurityInfo,pSecurityDesc);

    /* It seems to perform this check before the hkey check */
    if ((SecurityInfo & OWNER_SECURITY_INFORMATION) ||
        (SecurityInfo & GROUP_SECURITY_INFORMATION) ||
        (SecurityInfo & DACL_SECURITY_INFORMATION) ||
        (SecurityInfo & SACL_SECURITY_INFORMATION)) {
        /* Param OK */
    } else
        return ERROR_INVALID_PARAMETER;

    if (!pSecurityDesc)
        return ERROR_INVALID_PARAMETER;

    if (!(hkey = get_special_root_hkey( hkey, 0 ))) return ERROR_INVALID_HANDLE;

    return RtlNtStatusToDosError( NtSetSecurityObject( hkey, SecurityInfo, pSecurityDesc ) );
}


/******************************************************************************
 * RegGetKeySecurity [ADVAPI32.@]
 *
 * Get a copy of the security descriptor for a given registry key.
 *
 * PARAMS
 *  hkey                   [I]   Open handle of key to set
 *  SecurityInformation    [I]   Descriptor contents
 *  pSecurityDescriptor    [O]   Address of descriptor for key
 *  lpcbSecurityDescriptor [I/O] Address of size of buffer and description
 *
 * RETURNS
 *  Success: ERROR_SUCCESS
 *  Failure: Error code
 */
LSTATUS WINAPI RegGetKeySecurity( HKEY hkey, SECURITY_INFORMATION SecurityInformation,
                               PSECURITY_DESCRIPTOR pSecurityDescriptor,
                               LPDWORD lpcbSecurityDescriptor )
{
    TRACE("(%p,%d,%p,%d)\n",hkey,SecurityInformation,pSecurityDescriptor,
          *lpcbSecurityDescriptor);

    if (!(hkey = get_special_root_hkey( hkey, 0 ))) return ERROR_INVALID_HANDLE;

    return RtlNtStatusToDosError( NtQuerySecurityObject( hkey,
                SecurityInformation, pSecurityDescriptor,
                *lpcbSecurityDescriptor, lpcbSecurityDescriptor ) );
}


/******************************************************************************
 * RegFlushKey [ADVAPI32.@]
 * 
 * Immediately write a registry key to registry.
 *
 * PARAMS
 *  hkey [I] Handle of key to write
 *
 * RETURNS
 *  Success: ERROR_SUCCESS
 *  Failure: Error code
 */
LSTATUS WINAPI RegFlushKey( HKEY hkey )
{
    hkey = get_special_root_hkey( hkey, 0 );
    if (!hkey) return ERROR_INVALID_HANDLE;

    return RtlNtStatusToDosError( NtFlushKey( hkey ) );
}


/******************************************************************************
 * RegConnectRegistryW [ADVAPI32.@]
 *
 * Establish a connection to a predefined registry key on another computer.
 *
 * PARAMS
 *  lpMachineName [I] Address of name of remote computer
 *  hHey          [I] Predefined registry handle
 *  phkResult     [I] Address of buffer for remote registry handle
 *
 * RETURNS
 *  Success: ERROR_SUCCESS
 *  Failure: nonzero error code from Winerror.h
 */
LSTATUS WINAPI RegConnectRegistryW( LPCWSTR lpMachineName, HKEY hKey,
                                   PHKEY phkResult )
{
    LONG ret;

    TRACE("(%s,%p,%p)\n",debugstr_w(lpMachineName), hKey, phkResult);

    if (!lpMachineName || !*lpMachineName) {
        /* Use the local machine name */
        ret = RegOpenKeyW( hKey, NULL, phkResult );
    }
    else {
        WCHAR compName[MAX_COMPUTERNAME_LENGTH + 1];
        DWORD len = sizeof(compName) / sizeof(WCHAR);

        /* MSDN says lpMachineName must start with \\ : not so */
        if( lpMachineName[0] == '\\' &&  lpMachineName[1] == '\\')
            lpMachineName += 2;
        if (GetComputerNameW(compName, &len))
        {
            if (!strcmpiW(lpMachineName, compName))
                ret = RegOpenKeyW(hKey, NULL, phkResult);
            else
            {
                FIXME("Connect to %s is not supported.\n",debugstr_w(lpMachineName));
                ret = ERROR_BAD_NETPATH;
            }
        }
        else
            ret = GetLastError();
    }
    return ret;
}


/******************************************************************************
 * RegConnectRegistryA [ADVAPI32.@]
 *
 * See RegConnectRegistryW.
 */
LSTATUS WINAPI RegConnectRegistryA( LPCSTR machine, HKEY hkey, PHKEY reskey )
{
    UNICODE_STRING machineW;
    LONG ret;

    RtlCreateUnicodeStringFromAsciiz( &machineW, machine );
    ret = RegConnectRegistryW( machineW.Buffer, hkey, reskey );
    RtlFreeUnicodeString( &machineW );
    return ret;
}


/******************************************************************************
 * RegNotifyChangeKeyValue [ADVAPI32.@]
 *
 * Notify the caller about changes to the attributes or contents of a registry key.
 *
 * PARAMS
 *  hkey            [I] Handle of key to watch
 *  fWatchSubTree   [I] Flag for subkey notification
 *  fdwNotifyFilter [I] Changes to be reported
 *  hEvent          [I] Handle of signaled event
 *  fAsync          [I] Flag for asynchronous reporting
 *
 * RETURNS
 *  Success: ERROR_SUCCESS
 *  Failure: nonzero error code from Winerror.h
 */
LSTATUS WINAPI RegNotifyChangeKeyValue( HKEY hkey, BOOL fWatchSubTree,
                                     DWORD fdwNotifyFilter, HANDLE hEvent,
                                     BOOL fAsync )
{
    NTSTATUS status;
    IO_STATUS_BLOCK iosb;

    hkey = get_special_root_hkey( hkey, 0 );
    if (!hkey) return ERROR_INVALID_HANDLE;

    TRACE("(%p,%i,%d,%p,%i)\n", hkey, fWatchSubTree, fdwNotifyFilter,
          hEvent, fAsync);

    status = NtNotifyChangeKey( hkey, hEvent, NULL, NULL, &iosb,
                                fdwNotifyFilter, fAsync, NULL, 0,
                                fWatchSubTree);

    if (status && status != STATUS_TIMEOUT)
        return RtlNtStatusToDosError( status );

    return ERROR_SUCCESS;
}

/******************************************************************************
 * RegOpenUserClassesRoot [ADVAPI32.@]
 *
 * Open the HKEY_CLASSES_ROOT key for a user.
 *
 * PARAMS
 *  hToken     [I] Handle of token representing the user
 *  dwOptions  [I] Reserved, must be 0
 *  samDesired [I] Desired access rights
 *  phkResult  [O] Destination for the resulting key handle
 *
 * RETURNS
 *  Success: ERROR_SUCCESS
 *  Failure: nonzero error code from Winerror.h
 * 
 * NOTES
 *  On Windows 2000 and upwards the HKEY_CLASSES_ROOT key is a view of the
 *  "HKEY_LOCAL_MACHINE\Software\Classes" and the
 *  "HKEY_CURRENT_USER\Software\Classes" keys merged together.
 */
LSTATUS WINAPI RegOpenUserClassesRoot(
    HANDLE hToken,
    DWORD dwOptions,
    REGSAM samDesired,
    PHKEY phkResult
)
{
    FIXME("(%p, 0x%x, 0x%x, %p) semi-stub\n", hToken, dwOptions, samDesired, phkResult);

    *phkResult = HKEY_CLASSES_ROOT;
    return ERROR_SUCCESS;
}

/******************************************************************************
 * load_string [Internal]
 *
 * This is basically a copy of user32/resource.c's LoadStringW. Necessary to
 * avoid importing user32, which is higher level than advapi32. Helper for
 * RegLoadMUIString.
 */
static int load_string(HINSTANCE hModule, UINT resId, LPWSTR pwszBuffer, INT cMaxChars)
{
    HGLOBAL hMemory;
    HRSRC hResource;
    WCHAR *pString;
    int idxString;

    /* Negative values have to be inverted. */
    if (HIWORD(resId) == 0xffff)
        resId = (UINT)(-((INT)resId));

    /* Load the resource into memory and get a pointer to it. */
    hResource = FindResourceW(hModule, MAKEINTRESOURCEW(LOWORD(resId >> 4) + 1), (LPWSTR)RT_STRING);
    if (!hResource) return 0;
    hMemory = LoadResource(hModule, hResource);
    if (!hMemory) return 0;
    pString = LockResource(hMemory);

    /* Strings are length-prefixed. Lowest nibble of resId is an index. */
    idxString = resId & 0xf;
    while (idxString--) pString += *pString + 1;

    /* If no buffer is given, return length of the string. */
    if (!pwszBuffer) return *pString;

    /* Else copy over the string, respecting the buffer size. */
    cMaxChars = (*pString < cMaxChars) ? *pString : (cMaxChars - 1);
    if (cMaxChars >= 0) {
        memcpy(pwszBuffer, pString+1, cMaxChars * sizeof(WCHAR));
        pwszBuffer[cMaxChars] = '\0';
    }

    return cMaxChars;
}

/******************************************************************************
 * RegLoadMUIStringW [ADVAPI32.@]
 *
 * Load the localized version of a string resource from some PE, respective 
 * id and path of which are given in the registry value in the format 
 * @[path]\dllname,-resourceId
 *
 * PARAMS
 *  hKey       [I] Key, of which to load the string value from.
 *  pszValue   [I] The value to be loaded (Has to be of REG_EXPAND_SZ or REG_SZ type).
 *  pszBuffer  [O] Buffer to store the localized string in. 
 *  cbBuffer   [I] Size of the destination buffer in bytes.
 *  pcbData    [O] Number of bytes written to pszBuffer (optional, may be NULL).
 *  dwFlags    [I] None supported yet.
 *  pszBaseDir [I] Not supported yet.
 *
 * RETURNS
 *  Success: ERROR_SUCCESS,
 *  Failure: nonzero error code from winerror.h
 *
 * NOTES
 *  This is an API of Windows Vista, which wasn't available at the time this code
 *  was written. We have to check for the correct behaviour once it's available. 
 */
LSTATUS WINAPI RegLoadMUIStringW(HKEY hKey, LPCWSTR pwszValue, LPWSTR pwszBuffer, DWORD cbBuffer,
    LPDWORD pcbData, DWORD dwFlags, LPCWSTR pwszBaseDir)
{
    DWORD dwValueType, cbData;
    LPWSTR pwszTempBuffer = NULL, pwszExpandedBuffer = NULL;
    LONG result;
        
    TRACE("(hKey = %p, pwszValue = %s, pwszBuffer = %p, cbBuffer = %d, pcbData = %p, "
          "dwFlags = %d, pwszBaseDir = %s)\n", hKey, debugstr_w(pwszValue), pwszBuffer,
          cbBuffer, pcbData, dwFlags, debugstr_w(pwszBaseDir));

    /* Parameter sanity checks. */
    if (!hKey || !pwszBuffer)
        return ERROR_INVALID_PARAMETER;

    if (pwszBaseDir && *pwszBaseDir) {
        FIXME("BaseDir parameter not yet supported!\n");
        return ERROR_INVALID_PARAMETER;
    }

    /* Check for value existence and correctness of its type, allocate a buffer and load it. */
    result = RegQueryValueExW(hKey, pwszValue, NULL, &dwValueType, NULL, &cbData);
    if (result != ERROR_SUCCESS) goto cleanup;
    if (!(dwValueType == REG_SZ || dwValueType == REG_EXPAND_SZ) || !cbData) {
        result = ERROR_FILE_NOT_FOUND;
        goto cleanup;
    }
    pwszTempBuffer = heap_alloc(cbData);
    if (!pwszTempBuffer) {
        result = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }
    result = RegQueryValueExW(hKey, pwszValue, NULL, &dwValueType, (LPBYTE)pwszTempBuffer, &cbData);
    if (result != ERROR_SUCCESS) goto cleanup;

    /* Expand environment variables, if appropriate, or copy the original string over. */
    if (dwValueType == REG_EXPAND_SZ) {
        cbData = ExpandEnvironmentStringsW(pwszTempBuffer, NULL, 0) * sizeof(WCHAR);
        if (!cbData) goto cleanup;
        pwszExpandedBuffer = heap_alloc(cbData);
        if (!pwszExpandedBuffer) {
            result = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        ExpandEnvironmentStringsW(pwszTempBuffer, pwszExpandedBuffer, cbData);
    } else {
        pwszExpandedBuffer = heap_alloc(cbData);
        memcpy(pwszExpandedBuffer, pwszTempBuffer, cbData);
    }

    /* If the value references a resource based string, parse the value and load the string.
     * Else just copy over the original value. */
    result = ERROR_SUCCESS;
    if (*pwszExpandedBuffer != '@') { /* '@' is the prefix for resource based string entries. */
        lstrcpynW(pwszBuffer, pwszExpandedBuffer, cbBuffer / sizeof(WCHAR));
    } else {
        WCHAR *pComma = strrchrW(pwszExpandedBuffer, ',');
        UINT uiStringId;
        HMODULE hModule;

        /* Format of the expanded value is 'path_to_dll,-resId' */
        if (!pComma || pComma[1] != '-') {
            result = ERROR_BADKEY;
            goto cleanup;
        }
 
        uiStringId = atoiW(pComma+2);
        *pComma = '\0';

        hModule = LoadLibraryW(pwszExpandedBuffer + 1);
        if (!hModule || !load_string(hModule, uiStringId, pwszBuffer, cbBuffer/sizeof(WCHAR)))
            result = ERROR_BADKEY;
        FreeLibrary(hModule);
    }
 
cleanup:
    heap_free(pwszTempBuffer);
    heap_free(pwszExpandedBuffer);
    return result;
}

/******************************************************************************
 * RegLoadMUIStringA [ADVAPI32.@]
 *
 * See RegLoadMUIStringW
 */
LSTATUS WINAPI RegLoadMUIStringA(HKEY hKey, LPCSTR pszValue, LPSTR pszBuffer, DWORD cbBuffer,
    LPDWORD pcbData, DWORD dwFlags, LPCSTR pszBaseDir)
{
    UNICODE_STRING valueW, baseDirW;
    WCHAR *pwszBuffer;
    DWORD cbData = cbBuffer * sizeof(WCHAR);
    LONG result;

    valueW.Buffer = baseDirW.Buffer = pwszBuffer = NULL;
    if (!RtlCreateUnicodeStringFromAsciiz(&valueW, pszValue) ||
        !RtlCreateUnicodeStringFromAsciiz(&baseDirW, pszBaseDir) ||
        !(pwszBuffer = heap_alloc(cbData)))
    {
        result = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    result = RegLoadMUIStringW(hKey, valueW.Buffer, pwszBuffer, cbData, NULL, dwFlags, 
                               baseDirW.Buffer);
 
    if (result == ERROR_SUCCESS) {
        cbData = WideCharToMultiByte(CP_ACP, 0, pwszBuffer, -1, pszBuffer, cbBuffer, NULL, NULL);
        if (pcbData)
            *pcbData = cbData;
    }

cleanup:
    heap_free(pwszBuffer);
    RtlFreeUnicodeString(&baseDirW);
    RtlFreeUnicodeString(&valueW);
 
    return result;
}

/******************************************************************************
 * RegDisablePredefinedCache [ADVAPI32.@]
 *
 * Disables the caching of the HKEY_CLASSES_ROOT key for the process.
 *
 * PARAMS
 *  None.
 *
 * RETURNS
 *  Success: ERROR_SUCCESS
 *  Failure: nonzero error code from Winerror.h
 * 
 * NOTES
 *  This is useful for services that use impersonation.
 */
LSTATUS WINAPI RegDisablePredefinedCache(void)
{
    HKEY hkey_current_user;
    int idx = HandleToUlong(HKEY_CURRENT_USER) - HandleToUlong(HKEY_SPECIAL_ROOT_FIRST);

    /* prevent caching of future requests */
    hkcu_cache_disabled = TRUE;

    hkey_current_user = InterlockedExchangePointer( (void **)&special_root_keys[idx], NULL );

    if (hkey_current_user)
        NtClose( hkey_current_user );

    return ERROR_SUCCESS;
}

/******************************************************************************
 * RegDeleteTreeW [ADVAPI32.@]
 *
 */
LSTATUS WINAPI RegDeleteTreeW(HKEY hKey, LPCWSTR lpszSubKey)
{
    LONG ret;
    DWORD dwMaxSubkeyLen, dwMaxValueLen;
    DWORD dwMaxLen, dwSize;
    WCHAR szNameBuf[MAX_PATH], *lpszName = szNameBuf;
    HKEY hSubKey = hKey;

    TRACE("(hkey=%p,%p %s)\n", hKey, lpszSubKey, debugstr_w(lpszSubKey));

    if(lpszSubKey)
    {
        ret = RegOpenKeyExW(hKey, lpszSubKey, 0, KEY_READ, &hSubKey);
        if (ret) return ret;
    }

    /* Get highest length for keys, values */
    ret = RegQueryInfoKeyW(hSubKey, NULL, NULL, NULL, NULL,
            &dwMaxSubkeyLen, NULL, NULL, &dwMaxValueLen, NULL, NULL, NULL);
    if (ret) goto cleanup;

    dwMaxSubkeyLen++;
    dwMaxValueLen++;
    dwMaxLen = max(dwMaxSubkeyLen, dwMaxValueLen);
    if (dwMaxLen > sizeof(szNameBuf)/sizeof(WCHAR))
    {
        /* Name too big: alloc a buffer for it */
        if (!(lpszName = heap_alloc( dwMaxLen*sizeof(WCHAR))))
        {
            ret = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
    }


    /* Recursively delete all the subkeys */
    while (TRUE)
    {
        dwSize = dwMaxLen;
        if (RegEnumKeyExW(hSubKey, 0, lpszName, &dwSize, NULL,
                          NULL, NULL, NULL)) break;

        ret = RegDeleteTreeW(hSubKey, lpszName);
        if (ret) goto cleanup;
    }

    if (lpszSubKey)
        ret = RegDeleteKeyW(hKey, lpszSubKey);
    else
        while (TRUE)
        {
            dwSize = dwMaxLen;
            if (RegEnumValueW(hKey, 0, lpszName, &dwSize,
                  NULL, NULL, NULL, NULL)) break;

            ret = RegDeleteValueW(hKey, lpszName);
            if (ret) goto cleanup;
        }

cleanup:
    /* Free buffer if allocated */
    if (lpszName != szNameBuf)
        heap_free( lpszName);
    if(lpszSubKey)
        RegCloseKey(hSubKey);
    return ret;
}

/******************************************************************************
 * RegDeleteTreeA [ADVAPI32.@]
 *
 */
LSTATUS WINAPI RegDeleteTreeA(HKEY hKey, LPCSTR lpszSubKey)
{
    LONG ret;
    UNICODE_STRING lpszSubKeyW;

    if (lpszSubKey) RtlCreateUnicodeStringFromAsciiz( &lpszSubKeyW, lpszSubKey);
    else lpszSubKeyW.Buffer = NULL;
    ret = RegDeleteTreeW( hKey, lpszSubKeyW.Buffer);
    RtlFreeUnicodeString( &lpszSubKeyW );
    return ret;
}

/******************************************************************************
 * RegDisableReflectionKey [ADVAPI32.@]
 *
 */
LONG WINAPI RegDisableReflectionKey(HKEY base)
{
    FIXME("%p: stub\n", base);
    return ERROR_SUCCESS;
}
