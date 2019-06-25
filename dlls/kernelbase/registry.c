/*
 * Registry management
 *
 * Copyright 1996 Marcus Meissner
 * Copyright 1998 Matthew Becker
 * Copyright 1999 Sylvain St-Germain
 * Copyright 1999 Alexandre Julliard
 * Copyright 2017 Dmitry Timoshkov
 * Copyright 2019 Nikolay Sivov for CodeWeavers
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
#include "winperf.h"
#include "winuser.h"
#include "shlwapi.h"
#include "sddl.h"

#include "wine/debug.h"
#include "wine/heap.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(reg);

#define HKEY_SPECIAL_ROOT_FIRST   HKEY_CLASSES_ROOT
#define HKEY_SPECIAL_ROOT_LAST    HKEY_DYN_DATA

static const WCHAR name_CLASSES_ROOT[] =
    {'\\','R','e','g','i','s','t','r','y','\\',
     'M','a','c','h','i','n','e','\\',
     'S','o','f','t','w','a','r','e','\\',
     'C','l','a','s','s','e','s',0};
static const WCHAR name_LOCAL_MACHINE[] =
    {'\\','R','e','g','i','s','t','r','y','\\',
     'M','a','c','h','i','n','e',0};
static const WCHAR name_USERS[] =
    {'\\','R','e','g','i','s','t','r','y','\\',
     'U','s','e','r',0};
static const WCHAR name_PERFORMANCE_DATA[] =
    {'\\','R','e','g','i','s','t','r','y','\\',
     'P','e','r','f','D','a','t','a',0};
static const WCHAR name_CURRENT_CONFIG[] =
    {'\\','R','e','g','i','s','t','r','y','\\',
     'M','a','c','h','i','n','e','\\',
     'S','y','s','t','e','m','\\',
     'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
     'H','a','r','d','w','a','r','e',' ','P','r','o','f','i','l','e','s','\\',
     'C','u','r','r','e','n','t',0};
static const WCHAR name_DYN_DATA[] =
    {'\\','R','e','g','i','s','t','r','y','\\',
     'D','y','n','D','a','t','a',0};

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

static HKEY special_root_keys[ARRAY_SIZE(root_key_names)];
static BOOL cache_disabled[ARRAY_SIZE(root_key_names)];

static CRITICAL_SECTION reg_mui_cs;
static CRITICAL_SECTION_DEBUG reg_mui_cs_debug =
{
    0, 0, &reg_mui_cs,
    { &reg_mui_cs_debug.ProcessLocksList,
      &reg_mui_cs_debug.ProcessLocksList },
    0, 0, { (DWORD_PTR)(__FILE__ ": reg_mui_cs") }
};
static CRITICAL_SECTION reg_mui_cs = { &reg_mui_cs_debug, -1, 0, 0, 0, 0 };
struct mui_cache_entry {
    struct list entry;
    WCHAR *file_name; /* full path name */
    DWORD index;
    LCID  locale;
    WCHAR *text;
};
static struct list reg_mui_cache = LIST_INIT(reg_mui_cache); /* MRU */
static unsigned int reg_mui_cache_count;
#define REG_MUI_CACHE_SIZE 8

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
            !wcsnicmp( name->Buffer, wow6432nodeW, ARRAY_SIZE( wow6432nodeW )));
}

/* open the Wow6432Node subkey of the specified key */
static HANDLE open_wow6432node( HANDLE key )
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
    if (NtOpenKeyEx( &ret, MAXIMUM_ALLOWED, &attr, 0 )) ret = 0;
    return ret;
}

/* wrapper for NtCreateKey that creates the key recursively if necessary */
static NTSTATUS create_key( HKEY *retkey, ACCESS_MASK access, OBJECT_ATTRIBUTES *attr,
                            const UNICODE_STRING *class, ULONG options, PULONG dispos )
{
    BOOL force_wow32 = is_win64 && (access & KEY_WOW64_32KEY);
    NTSTATUS status = STATUS_OBJECT_NAME_NOT_FOUND;
    HANDLE subkey, root = attr->RootDirectory;

    if (!force_wow32) status = NtCreateKey( &subkey, access, attr, 0, class, options, dispos );

    if (status == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        static const WCHAR registry_root[] = {'\\','R','e','g','i','s','t','r','y','\\'};
        WCHAR *buffer = attr->ObjectName->Buffer;
        DWORD attrs, pos = 0, i = 0, len = attr->ObjectName->Length / sizeof(WCHAR);
        UNICODE_STRING str;

        /* don't try to create registry root */
        if (!attr->RootDirectory && len > ARRAY_SIZE( registry_root ) &&
            !wcsnicmp( buffer, registry_root, ARRAY_SIZE( registry_root )))
            i += ARRAY_SIZE( registry_root );

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
                else if ((subkey = open_wow6432node( attr->RootDirectory )))
                {
                    if (attr->RootDirectory != root) NtClose( attr->RootDirectory );
                    attr->RootDirectory = subkey;
                    force_wow32 = FALSE;
                }
            }
            if (i == len)
            {
                attr->Attributes = attrs;
                status = NtCreateKey( &subkey, access, attr, 0, class, options, dispos );
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
    attr->RootDirectory = subkey;
    if (force_wow32 && (subkey = open_wow6432node( attr->RootDirectory )))
    {
        if (attr->RootDirectory != root) NtClose( attr->RootDirectory );
        attr->RootDirectory = subkey;
    }
    *retkey = attr->RootDirectory;
    return status;
}

/* wrapper for NtOpenKeyEx to handle Wow6432 nodes */
static NTSTATUS open_key( HKEY *retkey, DWORD options, ACCESS_MASK access, OBJECT_ATTRIBUTES *attr )
{
    NTSTATUS status;
    BOOL force_wow32 = is_win64 && (access & KEY_WOW64_32KEY);
    HANDLE subkey, root = attr->RootDirectory;
    WCHAR *buffer = attr->ObjectName->Buffer;
    DWORD pos = 0, i = 0, len = attr->ObjectName->Length / sizeof(WCHAR);
    UNICODE_STRING str;

    *retkey = NULL;

    if (!force_wow32)
    {
        if (options & REG_OPTION_OPEN_LINK) attr->Attributes |= OBJ_OPENLINK;
        return NtOpenKeyEx( (HANDLE *)retkey, access, attr, options );
    }

    if (len && buffer[0] == '\\') return STATUS_OBJECT_PATH_INVALID;
    while (i < len && buffer[i] != '\\') i++;
    attr->ObjectName = &str;

    for (;;)
    {
        str.Buffer = buffer + pos;
        str.Length = (i - pos) * sizeof(WCHAR);
        if (force_wow32 && pos)
        {
            if (is_wow6432node( &str )) force_wow32 = FALSE;
            else if ((subkey = open_wow6432node( attr->RootDirectory )))
            {
                if (attr->RootDirectory != root) NtClose( attr->RootDirectory );
                attr->RootDirectory = subkey;
                force_wow32 = FALSE;
            }
        }
        if (i == len)
        {
            if (options & REG_OPTION_OPEN_LINK) attr->Attributes |= OBJ_OPENLINK;
            status = NtOpenKeyEx( &subkey, access, attr, options );
        }
        else
        {
            if (!(options & REG_OPTION_OPEN_LINK)) attr->Attributes &= ~OBJ_OPENLINK;
            status = NtOpenKeyEx( &subkey, access, attr, options & ~REG_OPTION_OPEN_LINK );
        }
        if (attr->RootDirectory != root) NtClose( attr->RootDirectory );
        if (status) return status;
        attr->RootDirectory = subkey;
        if (i == len) break;
        while (i < len && buffer[i] == '\\') i++;
        pos = i;
        while (i < len && buffer[i] != '\\') i++;
    }
    if (force_wow32 && (subkey = open_wow6432node( attr->RootDirectory )))
    {
        if (attr->RootDirectory != root) NtClose( attr->RootDirectory );
        attr->RootDirectory = subkey;
    }
    *retkey = attr->RootDirectory;
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

    if (!cache_disabled[idx] && !(access & (KEY_WOW64_64KEY | KEY_WOW64_32KEY)))
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
 * RemapPredefinedHandleInternal   (kernelbase.@)
 */
NTSTATUS WINAPI RemapPredefinedHandleInternal( HKEY hkey, HKEY override )
{
    HKEY old_key;
    int idx;

    TRACE("(%p %p)\n", hkey, override);

    if ((HandleToUlong(hkey) < HandleToUlong(HKEY_SPECIAL_ROOT_FIRST))
            || (HandleToUlong(hkey) > HandleToUlong(HKEY_SPECIAL_ROOT_LAST)))
        return STATUS_INVALID_HANDLE;
    idx = HandleToUlong(hkey) - HandleToUlong(HKEY_SPECIAL_ROOT_FIRST);

    if (override)
    {
        NTSTATUS status = NtDuplicateObject( GetCurrentProcess(), override,
                                             GetCurrentProcess(), (HANDLE *)&override,
                                             0, 0, DUPLICATE_SAME_ACCESS );
        if (status) return status;
    }

    old_key = InterlockedExchangePointer( (void **)&special_root_keys[idx], override );
    if (old_key) NtClose( old_key );
    return STATUS_SUCCESS;
}


/******************************************************************************
 * DisablePredefinedHandleTableInternal (kernelbase.@)
 */
NTSTATUS WINAPI DisablePredefinedHandleTableInternal( HKEY hkey )
{
    HKEY old_key;
    int idx;

    TRACE("(%p)\n", hkey);

    if ((HandleToUlong(hkey) < HandleToUlong(HKEY_SPECIAL_ROOT_FIRST))
            || (HandleToUlong(hkey) > HandleToUlong(HKEY_SPECIAL_ROOT_LAST)))
        return STATUS_INVALID_HANDLE;
    idx = HandleToUlong(hkey) - HandleToUlong(HKEY_SPECIAL_ROOT_FIRST);

    cache_disabled[idx] = TRUE;

    old_key = InterlockedExchangePointer( (void **)&special_root_keys[idx], NULL );
    if (old_key) NtClose( old_key );
    return STATUS_SUCCESS;
}


/******************************************************************************
 * RegCreateKeyExW   (kernelbase.@)
 *
 * See RegCreateKeyExA.
 */
LSTATUS WINAPI DECLSPEC_HOTPATCH RegCreateKeyExW( HKEY hkey, LPCWSTR name, DWORD reserved, LPWSTR class,
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
 * RegCreateKeyExA   (kernelbase.@)
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
LSTATUS WINAPI DECLSPEC_HOTPATCH RegCreateKeyExA( HKEY hkey, LPCSTR name, DWORD reserved, LPSTR class,
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
 * RegOpenKeyExW   (kernelbase.@)
 *
 * See RegOpenKeyExA.
 */
LSTATUS WINAPI DECLSPEC_HOTPATCH RegOpenKeyExW( HKEY hkey, LPCWSTR name, DWORD options, REGSAM access, PHKEY retkey )
{
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;

    if (retkey && (!name || !name[0]) &&
        (HandleToUlong(hkey) >= HandleToUlong(HKEY_SPECIAL_ROOT_FIRST)) &&
        (HandleToUlong(hkey) <= HandleToUlong(HKEY_SPECIAL_ROOT_LAST)))
    {
        *retkey = hkey;
        return ERROR_SUCCESS;
    }

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
    RtlInitUnicodeString( &nameW, name );
    return RtlNtStatusToDosError( open_key( retkey, options, access, &attr ) );
}


/******************************************************************************
 * RegOpenKeyExA   (kernelbase.@)
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
LSTATUS WINAPI DECLSPEC_HOTPATCH RegOpenKeyExA( HKEY hkey, LPCSTR name, DWORD options, REGSAM access, PHKEY retkey )
{
    OBJECT_ATTRIBUTES attr;
    STRING nameA;
    NTSTATUS status;

    if (retkey && (!name || !name[0]) &&
        (HandleToUlong(hkey) >= HandleToUlong(HKEY_SPECIAL_ROOT_FIRST)) &&
        (HandleToUlong(hkey) <= HandleToUlong(HKEY_SPECIAL_ROOT_LAST)))
    {
        *retkey = hkey;
        return ERROR_SUCCESS;
    }

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

    RtlInitAnsiString( &nameA, name );
    if (!(status = RtlAnsiStringToUnicodeString( &NtCurrentTeb()->StaticUnicodeString,
                                                 &nameA, FALSE )))
    {
        status = open_key( retkey, options, access, &attr );
    }
    return RtlNtStatusToDosError( status );
}


/******************************************************************************
 * RegOpenCurrentUser   (kernelbase.@)
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
    void *data[20];
    TOKEN_USER *info = (TOKEN_USER *)data;
    HANDLE token;
    DWORD len = 0;

    /* get current user SID */
    if (OpenThreadToken( GetCurrentThread(), TOKEN_QUERY, FALSE, &token ))
    {
        len = sizeof(data);
        if (!GetTokenInformation( token, TokenUser, info, len, &len )) len = 0;
        CloseHandle( token );
    }
    if (!len)
    {
        ImpersonateSelf(SecurityIdentification);
        if (OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, FALSE, &token))
        {
            len = sizeof(data);
            if (!GetTokenInformation( token, TokenUser, info, len, &len )) len = 0;
            CloseHandle( token );
        }
        RevertToSelf();
    }

    if (len)
    {
        WCHAR buffer[200];
        UNICODE_STRING string = { 0, sizeof(buffer), buffer };

        RtlConvertSidToUnicodeString( &string, info->User.Sid, FALSE );
        return RegOpenKeyExW( HKEY_USERS, string.Buffer, 0, access, retkey );
    }

    return RegOpenKeyExA( HKEY_CURRENT_USER, "", 0, access, retkey );
}



/******************************************************************************
 * RegEnumKeyExW   (kernelbase.@)
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
 * RegEnumKeyExA   (kernelbase.@)
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
 * RegQueryInfoKeyW   (kernelbase.@)
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

    if (class && class_len && *class_len)
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
            status = STATUS_BUFFER_TOO_SMALL;
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
    if (max_subkey) *max_subkey = info->MaxNameLen / sizeof(WCHAR);
    if (max_class) *max_class = info->MaxClassLen / sizeof(WCHAR);
    if (values) *values = info->Values;
    if (max_value) *max_value = info->MaxValueNameLen / sizeof(WCHAR);
    if (max_data) *max_data = info->MaxValueDataLen;
    if (modif) *modif = *(FILETIME *)&info->LastWriteTime;

    if (security)
    {
        FIXME( "security argument not supported.\n");
        *security = 0;
    }

 done:
    if (buf_ptr != buffer) heap_free( buf_ptr );
    return RtlNtStatusToDosError( status );
}


/******************************************************************************
 * RegQueryInfoKeyA   (kernelbase.@)
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

        len = 0;
        if (class && class_len) len = *class_len;
        RtlUnicodeToMultiByteN( class, len, class_len,
                                (WCHAR *)(buf_ptr + info->ClassOffset), info->ClassLength );
        if (len)
        {
            if (*class_len + 1 > len)
            {
                status = STATUS_BUFFER_OVERFLOW;
                *class_len -= 1;
            }
            class[*class_len] = 0;
        }
    }
    else status = STATUS_SUCCESS;

    if (subkeys) *subkeys = info->SubKeys;
    if (max_subkey) *max_subkey = info->MaxNameLen / sizeof(WCHAR);
    if (max_class) *max_class = info->MaxClassLen / sizeof(WCHAR);
    if (values) *values = info->Values;
    if (max_value) *max_value = info->MaxValueNameLen / sizeof(WCHAR);
    if (max_data) *max_data = info->MaxValueDataLen;
    if (modif) *modif = *(FILETIME *)&info->LastWriteTime;

    if (security)
    {
        FIXME( "security argument not supported.\n");
        *security = 0;
    }

 done:
    if (buf_ptr != buffer) heap_free( buf_ptr );
    return RtlNtStatusToDosError( status );
}

/******************************************************************************
 * RegCloseKey   (kernelbase.@)
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
LSTATUS WINAPI DECLSPEC_HOTPATCH RegCloseKey( HKEY hkey )
{
    if (!hkey) return ERROR_INVALID_HANDLE;
    if (hkey >= (HKEY)0x80000000) return ERROR_SUCCESS;
    return RtlNtStatusToDosError( NtClose( hkey ) );
}


/******************************************************************************
 * RegDeleteKeyExW   (kernelbase.@)
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
 * RegDeleteKeyExA   (kernelbase.@)
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
            DWORD len = sizeof(sub);
            while(!RegEnumKeyExA(tmp, 0, sub, &len, NULL, NULL, NULL, NULL))
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
 * RegSetValueExW   (kernelbase.@)
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
LSTATUS WINAPI DECLSPEC_HOTPATCH RegSetValueExW( HKEY hkey, LPCWSTR name, DWORD reserved,
                                                 DWORD type, const BYTE *data, DWORD count )
{
    UNICODE_STRING nameW;

    /* no need for version check, not implemented on win9x anyway */

    if ((data && ((ULONG_PTR)data >> 16) == 0) || (!data && count)) return ERROR_NOACCESS;

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
 * RegSetValueExA   (kernelbase.@)
 *
 * See RegSetValueExW.
 *
 * NOTES
 *  win95 does not care about count for REG_SZ and finds out the len by itself (js)
 *  NT does definitely care (aj)
 */
LSTATUS WINAPI DECLSPEC_HOTPATCH RegSetValueExA( HKEY hkey, LPCSTR name, DWORD reserved, DWORD type,
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
 * RegSetKeyValueW   (kernelbase.@)
 */
LONG WINAPI RegSetKeyValueW( HKEY hkey, LPCWSTR subkey, LPCWSTR name, DWORD type, const void *data, DWORD len )
{
    HKEY hsubkey = NULL;
    DWORD ret;

    TRACE("(%p,%s,%s,%d,%p,%d)\n", hkey, debugstr_w(subkey), debugstr_w(name), type, data, len );

    if (subkey && subkey[0])  /* need to create the subkey */
    {
        if ((ret = RegCreateKeyExW( hkey, subkey, 0, NULL, REG_OPTION_NON_VOLATILE,
                                    KEY_SET_VALUE, NULL, &hsubkey, NULL )) != ERROR_SUCCESS) return ret;
        hkey = hsubkey;
    }

    ret = RegSetValueExW( hkey, name, 0, type, (const BYTE*)data, len );
    if (hsubkey) RegCloseKey( hsubkey );
    return ret;
}

/******************************************************************************
 * RegSetKeyValueA   (kernelbase.@)
 */
LONG WINAPI RegSetKeyValueA( HKEY hkey, LPCSTR subkey, LPCSTR name, DWORD type, const void *data, DWORD len )
{
    HKEY hsubkey = NULL;
    DWORD ret;

    TRACE("(%p,%s,%s,%d,%p,%d)\n", hkey, debugstr_a(subkey), debugstr_a(name), type, data, len );

    if (subkey && subkey[0])  /* need to create the subkey */
    {
        if ((ret = RegCreateKeyExA( hkey, subkey, 0, NULL, REG_OPTION_NON_VOLATILE,
                                    KEY_SET_VALUE, NULL, &hsubkey, NULL )) != ERROR_SUCCESS) return ret;
        hkey = hsubkey;
    }

    ret = RegSetValueExA( hkey, name, 0, type, (const BYTE*)data, len );
    if (hsubkey) RegCloseKey( hsubkey );
    return ret;
}

struct perf_provider
{
    HMODULE perflib;
    WCHAR linkage[MAX_PATH];
    WCHAR objects[MAX_PATH];
    PM_OPEN_PROC *pOpen;
    PM_CLOSE_PROC *pClose;
    PM_COLLECT_PROC *pCollect;
};

static void *get_provider_entry(HKEY perf, HMODULE perflib, const char *name)
{
    char buf[MAX_PATH];
    DWORD err, type, len;

    len = sizeof(buf) - 1;
    err = RegQueryValueExA(perf, name, NULL, &type, (BYTE *)buf, &len);
    if (err != ERROR_SUCCESS || type != REG_SZ)
        return NULL;

    buf[len] = 0;
    TRACE("Loading function pointer for %s: %s\n", name, debugstr_a(buf));

    return GetProcAddress(perflib, buf);
}

static BOOL load_provider(HKEY root, const WCHAR *name, struct perf_provider *provider)
{
    static const WCHAR object_listW[] = { 'O','b','j','e','c','t',' ','L','i','s','t',0 };
    static const WCHAR performanceW[] = { 'P','e','r','f','o','r','m','a','n','c','e',0 };
    static const WCHAR libraryW[] = { 'L','i','b','r','a','r','y',0 };
    static const WCHAR linkageW[] = { 'L','i','n','k','a','g','e',0 };
    static const WCHAR exportW[] = { 'E','x','p','o','r','t',0 };
    WCHAR buf[MAX_PATH], buf2[MAX_PATH];
    DWORD err, type, len;
    HKEY service, perf;

    err = RegOpenKeyExW(root, name, 0, KEY_READ, &service);
    if (err != ERROR_SUCCESS)
        return FALSE;

    provider->linkage[0] = 0;
    err = RegOpenKeyExW(service, linkageW, 0, KEY_READ, &perf);
    if (err == ERROR_SUCCESS)
    {
        len = sizeof(buf) - sizeof(WCHAR);
        err = RegQueryValueExW(perf, exportW, NULL, &type, (BYTE *)buf, &len);
        if (err == ERROR_SUCCESS && (type == REG_SZ || type == REG_MULTI_SZ))
        {
            memcpy(provider->linkage, buf, len);
            provider->linkage[len / sizeof(WCHAR)] = 0;
            TRACE("Export: %s\n", debugstr_w(provider->linkage));
        }
        RegCloseKey(perf);
    }

    err = RegOpenKeyExW(service, performanceW, 0, KEY_READ, &perf);
    RegCloseKey(service);
    if (err != ERROR_SUCCESS)
        return FALSE;

    provider->objects[0] = 0;
    len = sizeof(buf) - sizeof(WCHAR);
    err = RegQueryValueExW(perf, object_listW, NULL, &type, (BYTE *)buf, &len);
    if (err == ERROR_SUCCESS && (type == REG_SZ || type == REG_MULTI_SZ))
    {
        memcpy(provider->objects, buf, len);
        provider->objects[len / sizeof(WCHAR)] = 0;
        TRACE("Object List: %s\n", debugstr_w(provider->objects));
    }

    len = sizeof(buf) - sizeof(WCHAR);
    err = RegQueryValueExW(perf, libraryW, NULL, &type, (BYTE *)buf, &len);
    if (err != ERROR_SUCCESS || !(type == REG_SZ || type == REG_EXPAND_SZ))
        goto error;

    buf[len / sizeof(WCHAR)] = 0;
    if (type == REG_EXPAND_SZ)
    {
        len = ExpandEnvironmentStringsW(buf, buf2, MAX_PATH);
        if (!len || len > MAX_PATH) goto error;
        lstrcpyW(buf, buf2);
    }

    if (!(provider->perflib = LoadLibraryW(buf)))
    {
        WARN("Failed to load %s\n", debugstr_w(buf));
        goto error;
    }

    GetModuleFileNameW(provider->perflib, buf, MAX_PATH);
    TRACE("Loaded provider %s\n", wine_dbgstr_w(buf));

    provider->pOpen = get_provider_entry(perf, provider->perflib, "Open");
    provider->pClose = get_provider_entry(perf, provider->perflib, "Close");
    provider->pCollect = get_provider_entry(perf, provider->perflib, "Collect");
    if (provider->pOpen && provider->pClose && provider->pCollect)
    {
        RegCloseKey(perf);
        return TRUE;
    }

    TRACE("Provider is missing required exports\n");
    FreeLibrary(provider->perflib);

error:
    RegCloseKey(perf);
    return FALSE;
}

static DWORD collect_data(struct perf_provider *provider, const WCHAR *query, void **data, DWORD *size, DWORD *obj_count)
{
    static const WCHAR globalW[] = { 'G','l','o','b','a','l',0 };
    WCHAR *linkage = provider->linkage[0] ? provider->linkage : NULL;
    DWORD err;

    if (!query || !query[0])
        query = globalW;

    err = provider->pOpen(linkage);
    if (err != ERROR_SUCCESS)
    {
        TRACE("Open(%s) error %u (%#x)\n", debugstr_w(linkage), err, err);
        return err;
    }

    *obj_count = 0;
    err = provider->pCollect((WCHAR *)query, data, size, obj_count);
    if (err != ERROR_SUCCESS)
    {
        TRACE("Collect error %u (%#x)\n", err, err);
        *obj_count = 0;
    }

    provider->pClose();
    return err;
}

#define MAX_SERVICE_NAME 260

static DWORD query_perf_data(const WCHAR *query, DWORD *type, void *data, DWORD *ret_size)
{
    static const WCHAR SZ_SERVICES_KEY[] = { 'S','y','s','t','e','m','\\',
        'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
        'S','e','r','v','i','c','e','s',0 };
    DWORD err, i, data_size;
    HKEY root;
    PERF_DATA_BLOCK *pdb;

    if (!ret_size)
        return ERROR_INVALID_PARAMETER;

    data_size = *ret_size;
    *ret_size = 0;

    if (type)
        *type = REG_BINARY;

    if (!data || data_size < sizeof(*pdb))
        return ERROR_MORE_DATA;

    pdb = data;

    pdb->Signature[0] = 'P';
    pdb->Signature[1] = 'E';
    pdb->Signature[2] = 'R';
    pdb->Signature[3] = 'F';
#ifdef WORDS_BIGENDIAN
    pdb->LittleEndian = FALSE;
#else
    pdb->LittleEndian = TRUE;
#endif
    pdb->Version = PERF_DATA_VERSION;
    pdb->Revision = PERF_DATA_REVISION;
    pdb->TotalByteLength = 0;
    pdb->HeaderLength = sizeof(*pdb);
    pdb->NumObjectTypes = 0;
    pdb->DefaultObject = 0;
    QueryPerformanceCounter(&pdb->PerfTime);
    QueryPerformanceFrequency(&pdb->PerfFreq);

    data = pdb + 1;
    pdb->SystemNameOffset = sizeof(*pdb);
    pdb->SystemNameLength = (data_size - sizeof(*pdb)) / sizeof(WCHAR);
    if (!GetComputerNameW(data, &pdb->SystemNameLength))
        return ERROR_MORE_DATA;

    pdb->SystemNameLength++;
    pdb->SystemNameLength *= sizeof(WCHAR);

    pdb->HeaderLength += pdb->SystemNameLength;

    /* align to 8 bytes */
    if (pdb->SystemNameLength & 7)
        pdb->HeaderLength += 8 - (pdb->SystemNameLength & 7);

    if (data_size < pdb->HeaderLength)
        return ERROR_MORE_DATA;

    pdb->TotalByteLength = pdb->HeaderLength;

    data_size -= pdb->HeaderLength;
    data = (char *)data + pdb->HeaderLength;

    err = RegOpenKeyExW(HKEY_LOCAL_MACHINE, SZ_SERVICES_KEY, 0, KEY_READ, &root);
    if (err != ERROR_SUCCESS)
        return err;

    i = 0;
    for (;;)
    {
        DWORD collected_size = data_size, obj_count = 0;
        struct perf_provider provider;
        WCHAR name[MAX_SERVICE_NAME];
        DWORD len = ARRAY_SIZE( name );
        void *collected_data = data;

        err = RegEnumKeyExW(root, i++, name, &len, NULL, NULL, NULL, NULL);
        if (err == ERROR_NO_MORE_ITEMS)
        {
            err = ERROR_SUCCESS;
            break;
        }

        if (err != ERROR_SUCCESS)
            continue;

        if (!load_provider(root, name, &provider))
            continue;

        err = collect_data(&provider, query, &collected_data, &collected_size, &obj_count);
        FreeLibrary(provider.perflib);

        if (err == ERROR_MORE_DATA)
            break;

        if (err == ERROR_SUCCESS)
        {
            PERF_OBJECT_TYPE *obj = (PERF_OBJECT_TYPE *)data;

            TRACE("Collect: obj->TotalByteLength %u, collected_size %u\n",
                obj->TotalByteLength, collected_size);

            data_size -= collected_size;
            data = collected_data;

            pdb->TotalByteLength += collected_size;
            pdb->NumObjectTypes += obj_count;
        }
    }

    RegCloseKey(root);

    if (err == ERROR_SUCCESS)
    {
        *ret_size = pdb->TotalByteLength;

        GetSystemTime(&pdb->SystemTime);
        GetSystemTimeAsFileTime((FILETIME *)&pdb->PerfTime100nSec);
    }

    return err;
}

/******************************************************************************
 * RegQueryValueExW   (kernelbase.@)
 *
 * See RegQueryValueExA.
 */
LSTATUS WINAPI DECLSPEC_HOTPATCH RegQueryValueExW( HKEY hkey, LPCWSTR name, LPDWORD reserved, LPDWORD type,
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

    if (hkey == HKEY_PERFORMANCE_DATA)
        return query_perf_data(name, type, data, count);

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
 * RegQueryValueExA   (kernelbase.@)
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
LSTATUS WINAPI DECLSPEC_HOTPATCH RegQueryValueExA( HKEY hkey, LPCSTR name, LPDWORD reserved,
                                                   LPDWORD type, LPBYTE data, LPDWORD count )
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
    if (hkey != HKEY_PERFORMANCE_DATA && !(hkey = get_special_root_hkey( hkey, 0 )))
        return ERROR_INVALID_HANDLE;

    if (count) datalen = *count;
    if (!data && count) *count = 0;

    /* this matches Win9x behaviour - NT sets *type to a random value */
    if (type) *type = REG_NONE;

    RtlInitAnsiString( &nameA, name );
    if ((status = RtlAnsiStringToUnicodeString( &nameW, &nameA, TRUE )))
        return RtlNtStatusToDosError(status);

    if (hkey == HKEY_PERFORMANCE_DATA)
    {
        DWORD ret = query_perf_data( nameW.Buffer, type, data, count );
        RtlFreeUnicodeString( &nameW );
        return ret;
    }

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
 * apply_restrictions   [internal]
 *
 * Helper function for RegGetValueA/W.
 */
static void apply_restrictions( DWORD dwFlags, DWORD dwType, DWORD cbData, PLONG ret )
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
 * RegGetValueW   (kernelbase.@)
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
    DWORD dwType, cbData = (pvData && pcbData) ? *pcbData : 0;
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

    apply_restrictions(dwFlags, dwType, cbData, &ret);

    if (pvData && ret != ERROR_SUCCESS && (dwFlags & RRF_ZEROONFAILURE))
        ZeroMemory(pvData, *pcbData);

    if (pdwType) *pdwType = dwType;
    if (pcbData) *pcbData = cbData;

    return ret;
}


/******************************************************************************
 * RegGetValueA   (kernelbase.@)
 *
 * See RegGetValueW.
 */
LSTATUS WINAPI RegGetValueA( HKEY hKey, LPCSTR pszSubKey, LPCSTR pszValue,
                             DWORD dwFlags, LPDWORD pdwType, PVOID pvData,
                             LPDWORD pcbData )
{
    DWORD dwType, cbData = (pvData && pcbData) ? *pcbData : 0;
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

    apply_restrictions(dwFlags, dwType, cbData, &ret);

    if (pvData && ret != ERROR_SUCCESS && (dwFlags & RRF_ZEROONFAILURE))
        ZeroMemory(pvData, *pcbData);

    if (pdwType) *pdwType = dwType;
    if (pcbData) *pcbData = cbData;

    return ret;
}


/******************************************************************************
 * RegEnumValueW   (kernelbase.@)
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

    if ((data && !count) || reserved || !value || !val_count)
        return ERROR_INVALID_PARAMETER;
    if (!(hkey = get_special_root_hkey( hkey, 0 ))) return ERROR_INVALID_HANDLE;

    total_size = info_size + (MAX_PATH + 1) * sizeof(WCHAR);
    if (data) total_size += *count;
    total_size = min( sizeof(buffer), total_size );

    status = NtEnumerateValueKey( hkey, index, KeyValueFullInformation,
                                  buffer, total_size, &total_size );

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

    if (info->NameLength/sizeof(WCHAR) >= *val_count)
    {
        status = STATUS_BUFFER_OVERFLOW;
        goto overflow;
    }
    memcpy( value, info->Name, info->NameLength );
    *val_count = info->NameLength / sizeof(WCHAR);
    value[*val_count] = 0;

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

 overflow:
    if (type) *type = info->Type;
    if (count) *count = info->DataLength;

 done:
    if (buf_ptr != buffer) heap_free( buf_ptr );
    return RtlNtStatusToDosError(status);
}


/******************************************************************************
 * RegEnumValueA   (kernelbase.@)
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

    if ((data && !count) || reserved || !value || !val_count)
        return ERROR_INVALID_PARAMETER;
    if (!(hkey = get_special_root_hkey( hkey, 0 ))) return ERROR_INVALID_HANDLE;

    total_size = info_size + (MAX_PATH + 1) * sizeof(WCHAR);
    if (data) total_size += *count;
    total_size = min( sizeof(buffer), total_size );

    status = NtEnumerateValueKey( hkey, index, KeyValueFullInformation,
                                  buffer, total_size, &total_size );

    /* we need to fetch the contents for a string type even if not requested,
     * because we need to compute the length of the ASCII string. */

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

    if (!status)
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

    if (type) *type = info->Type;
    if (count) *count = info->DataLength;

 done:
    if (buf_ptr != buffer) heap_free( buf_ptr );
    return RtlNtStatusToDosError(status);
}

/******************************************************************************
 * RegDeleteValueW   (kernelbase.@)
 *
 * See RegDeleteValueA.
 */
LSTATUS WINAPI RegDeleteValueW( HKEY hkey, LPCWSTR name )
{
    return RegDeleteKeyValueW( hkey, NULL, name );
}

/******************************************************************************
 * RegDeleteValueA   (kernelbase.@)
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
 * RegDeleteKeyValueW   (kernelbase.@)
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
 * RegDeleteKeyValueA   (kernelbase.@)
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
 * RegLoadKeyW   (kernelbase.@)
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
 * RegLoadKeyA   (kernelbase.@)
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
 * RegSaveKeyExW  (kernelbase.@)
 */
LSTATUS WINAPI RegSaveKeyExW( HKEY hkey, LPCWSTR file, SECURITY_ATTRIBUTES *sa, DWORD flags )
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
    GetFullPathNameW( file, ARRAY_SIZE( buffer ), buffer, &nameW );

    for (;;)
    {
        swprintf( nameW, 16, format, count++ );
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
 * RegSaveKeyExA  (kernelbase.@)
 */
LSTATUS WINAPI RegSaveKeyExA( HKEY hkey, LPCSTR file, SECURITY_ATTRIBUTES *sa, DWORD flags )
{
    UNICODE_STRING *fileW = &NtCurrentTeb()->StaticUnicodeString;
    NTSTATUS status;
    STRING fileA;

    RtlInitAnsiString(&fileA, file);
    if ((status = RtlAnsiStringToUnicodeString(fileW, &fileA, FALSE)))
        return RtlNtStatusToDosError( status );
    return RegSaveKeyExW(hkey, fileW->Buffer, sa, flags);
}


/******************************************************************************
 * RegRestoreKeyW (kernelbase.@)
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
 * RegRestoreKeyA (kernelbase.@)
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
 * RegUnLoadKeyW (kernelbase.@)
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

    ret = RegOpenKeyExW( hkey, lpSubKey, 0, MAXIMUM_ALLOWED, &shkey );
    if( ret )
        return ERROR_INVALID_PARAMETER;

    RtlInitUnicodeString(&subkey, lpSubKey);
    InitializeObjectAttributes(&attr, &subkey, OBJ_CASE_INSENSITIVE, shkey, NULL);
    ret = RtlNtStatusToDosError(NtUnloadKey(&attr));

    RegCloseKey(shkey);

    return ret;
}


/******************************************************************************
 * RegUnLoadKeyA (kernelbase.@)
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
 * RegSetKeySecurity (kernelbase.@)
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
 * RegGetKeySecurity (kernelbase.@)
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
 * RegFlushKey (kernelbase.@)
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
 * RegNotifyChangeKeyValue (kernelbase.@)
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
                                fdwNotifyFilter, fWatchSubTree, NULL, 0,
                                fAsync);

    if (status && status != STATUS_PENDING)
        return RtlNtStatusToDosError( status );

    return ERROR_SUCCESS;
}

/******************************************************************************
 * RegOpenUserClassesRoot (kernelbase.@)
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
LSTATUS WINAPI RegOpenUserClassesRoot( HANDLE hToken, DWORD dwOptions, REGSAM samDesired, PHKEY phkResult )
{
    FIXME("(%p, 0x%x, 0x%x, %p) semi-stub\n", hToken, dwOptions, samDesired, phkResult);

    *phkResult = HKEY_CLASSES_ROOT;
    return ERROR_SUCCESS;
}


static void dump_mui_cache(void)
{
    struct mui_cache_entry *ent;

    TRACE("---------- MUI Cache ----------\n");
    LIST_FOR_EACH_ENTRY( ent, &reg_mui_cache, struct mui_cache_entry, entry )
        TRACE("entry=%p, %s,-%u [%#x] => %s\n",
              ent, wine_dbgstr_w(ent->file_name), ent->index, ent->locale, wine_dbgstr_w(ent->text));
}

static inline void free_mui_cache_entry(struct mui_cache_entry *ent)
{
    heap_free(ent->file_name);
    heap_free(ent->text);
    heap_free(ent);
}

/* critical section must be held */
static int reg_mui_cache_get(const WCHAR *file_name, UINT index, WCHAR **buffer)
{
    struct mui_cache_entry *ent;

    TRACE("(%s %u %p)\n", wine_dbgstr_w(file_name), index, buffer);

    LIST_FOR_EACH_ENTRY(ent, &reg_mui_cache, struct mui_cache_entry, entry)
    {
        if (ent->index == index && ent->locale == GetThreadLocale() &&
            !wcsicmp(ent->file_name, file_name))
            goto found;
    }
    return 0;

found:
    /* move to the list head */
    if (list_prev(&reg_mui_cache, &ent->entry)) {
        list_remove(&ent->entry);
        list_add_head(&reg_mui_cache, &ent->entry);
    }

    TRACE("=> %s\n", wine_dbgstr_w(ent->text));
    *buffer = ent->text;
    return lstrlenW(ent->text);
}

/* critical section must be held */
static void reg_mui_cache_put(const WCHAR *file_name, UINT index, const WCHAR *buffer, INT size)
{
    struct mui_cache_entry *ent;
    TRACE("(%s %u %s %d)\n", wine_dbgstr_w(file_name), index, wine_dbgstr_wn(buffer, size), size);

    ent = heap_calloc(sizeof(*ent), 1);
    if (!ent)
        return;
    ent->file_name = heap_alloc((lstrlenW(file_name) + 1) * sizeof(WCHAR));
    if (!ent->file_name) {
        free_mui_cache_entry(ent);
        return;
    }
    lstrcpyW(ent->file_name, file_name);
    ent->index = index;
    ent->locale = GetThreadLocale();
    ent->text = heap_alloc((size + 1) * sizeof(WCHAR));
    if (!ent->text) {
        free_mui_cache_entry(ent);
        return;
    }
    memcpy(ent->text, buffer, size * sizeof(WCHAR));
    ent->text[size] = '\0';

    TRACE("add %p\n", ent);
    list_add_head(&reg_mui_cache, &ent->entry);
    if (reg_mui_cache_count > REG_MUI_CACHE_SIZE) {
        ent = LIST_ENTRY( list_tail( &reg_mui_cache ), struct mui_cache_entry, entry );
        TRACE("freeing %p\n", ent);
        list_remove(&ent->entry);
        free_mui_cache_entry(ent);
    }
    else
        reg_mui_cache_count++;

    if (TRACE_ON(reg))
        dump_mui_cache();
    return;
}

static LONG load_mui_string(const WCHAR *file_name, UINT res_id, WCHAR *buffer, INT max_chars, INT *req_chars, DWORD flags)
{
    HMODULE hModule = NULL;
    WCHAR *string = NULL, *full_name;
    int size;
    LONG result;

    /* Verify the file existence. i.e. We don't rely on PATH variable */
    if (GetFileAttributesW(file_name) == INVALID_FILE_ATTRIBUTES)
        return ERROR_FILE_NOT_FOUND;

    size = GetFullPathNameW(file_name, 0, NULL, NULL);
    full_name = heap_alloc(size * sizeof(WCHAR));
    if (!size)
        return GetLastError();
    GetFullPathNameW(file_name, size, full_name, NULL);

    EnterCriticalSection(&reg_mui_cs);
    size = reg_mui_cache_get(full_name, res_id, &string);
    if (!size) {
        LeaveCriticalSection(&reg_mui_cs);

        /* Load the file */
        hModule = LoadLibraryExW(full_name, NULL,
                                 LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE);
        if (!hModule)
            return GetLastError();

        size = LoadStringW(hModule, res_id, (WCHAR *)&string, 0);
        if (!size) {
            if (string) result = ERROR_NOT_FOUND;
            else result = GetLastError();
            goto cleanup;
        }

        EnterCriticalSection(&reg_mui_cs);
        reg_mui_cache_put(full_name, res_id, string, size);
        LeaveCriticalSection(&reg_mui_cs);
    }
    *req_chars = size + 1;

    /* If no buffer is given, skip copying. */
    if (!buffer) {
        result = ERROR_MORE_DATA;
        goto cleanup;
    }

    /* Else copy over the string, respecting the buffer size. */
    if (size < max_chars)
        max_chars = size;
    else {
        if (flags & REG_MUI_STRING_TRUNCATE)
            max_chars--;
        else {
            result = ERROR_MORE_DATA;
            goto cleanup;
        }
    }
    if (max_chars >= 0) {
        memcpy(buffer, string, max_chars * sizeof(WCHAR));
        buffer[max_chars] = '\0';
    }

    result = ERROR_SUCCESS;

cleanup:
    if (hModule)
        FreeLibrary(hModule);
    else
        LeaveCriticalSection(&reg_mui_cs);
    heap_free(full_name);
    return result;
}

/******************************************************************************
 * RegLoadMUIStringW (kernelbase.@)
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
 *  dwFlags    [I] Truncate output to fit the buffer if REG_MUI_STRING_TRUNCATE.
 *  pszBaseDir [I] Base directory of loading path. If NULL, use the current directory.
 *
 * RETURNS
 *  Success: ERROR_SUCCESS,
 *  Failure: nonzero error code from winerror.h
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
    if (!hKey || (!pwszBuffer && cbBuffer) || (cbBuffer % sizeof(WCHAR))
        || ((dwFlags & REG_MUI_STRING_TRUNCATE) && pcbData)
        || (dwFlags & ~REG_MUI_STRING_TRUNCATE))
        return ERROR_INVALID_PARAMETER;

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

    /* '@' is the prefix for resource based string entries. */
    if (*pwszTempBuffer != '@') {
        result = ERROR_INVALID_DATA;
        goto cleanup;
    }

    /* Expand environment variables regardless of the type. */
    cbData = ExpandEnvironmentStringsW(pwszTempBuffer, NULL, 0) * sizeof(WCHAR);
    if (!cbData) goto cleanup;
    pwszExpandedBuffer = heap_alloc(cbData);
    if (!pwszExpandedBuffer) {
        result = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }
    ExpandEnvironmentStringsW(pwszTempBuffer, pwszExpandedBuffer, cbData / sizeof(WCHAR));

    /* Parse the value and load the string. */
    {
        WCHAR *pComma = wcsrchr(pwszExpandedBuffer, ','), *pNewBuffer;
        const WCHAR backslashW[] = {'\\',0};
        UINT uiStringId;
        DWORD baseDirLen;
        int reqChars;

        /* Format of the expanded value is 'path_to_dll,-resId' */
        if (!pComma || pComma[1] != '-') {
            result = ERROR_INVALID_DATA;
            goto cleanup;
        }

        uiStringId = wcstol(pComma+2, NULL, 10);
        *pComma = '\0';

        /* Build a resource dll path. */
        baseDirLen = pwszBaseDir ? lstrlenW(pwszBaseDir) : 0;
        cbData = (baseDirLen + 1 + lstrlenW(pwszExpandedBuffer + 1) + 1) * sizeof(WCHAR);
        pNewBuffer = heap_realloc(pwszTempBuffer, cbData);
        if (!pNewBuffer) {
            result = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        pwszTempBuffer = pNewBuffer;
        pwszTempBuffer[0] = '\0';
        if (baseDirLen) {
            lstrcpyW(pwszTempBuffer, pwszBaseDir);
            if (pwszBaseDir[baseDirLen - 1] != '\\')
                lstrcatW(pwszTempBuffer, backslashW);
        }
        lstrcatW(pwszTempBuffer, pwszExpandedBuffer + 1);

        /* Load specified string from the file */
        reqChars = 0;
        result = load_mui_string(pwszTempBuffer, uiStringId, pwszBuffer, cbBuffer/sizeof(WCHAR), &reqChars, dwFlags);
        if (pcbData && (result == ERROR_SUCCESS || result == ERROR_MORE_DATA))
            *pcbData = reqChars * sizeof(WCHAR);
    }

cleanup:
    heap_free(pwszTempBuffer);
    heap_free(pwszExpandedBuffer);
    return result;
}

/******************************************************************************
 * RegLoadMUIStringA (kernelbase.@)
 *
 * Not implemented on native.
 */
LSTATUS WINAPI RegLoadMUIStringA(HKEY hKey, LPCSTR pszValue, LPSTR pszBuffer, DWORD cbBuffer,
    LPDWORD pcbData, DWORD dwFlags, LPCSTR pszBaseDir)
{
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/******************************************************************************
 * RegDeleteTreeW (kernelbase.@)
 *
 */
LSTATUS WINAPI RegDeleteTreeW( HKEY hkey, const WCHAR *subkey )
{
    static const WCHAR emptyW[] = {0};
    DWORD name_size, max_name, max_subkey;
    WCHAR *name_buf = NULL;
    LONG ret;

    TRACE( "(%p, %s)\n", hkey, debugstr_w(subkey) );

    if (subkey && *subkey)
    {
        ret = RegOpenKeyExW( hkey, subkey, 0, KEY_READ, &hkey );
        if (ret) return ret;
    }

    ret = RegQueryInfoKeyW( hkey, NULL, NULL, NULL, NULL, &max_subkey,
                            NULL, NULL, &max_name, NULL, NULL, NULL );
    if (ret)
        goto cleanup;

    max_name = max( max_subkey, max_name ) + 1;
    if (!(name_buf = heap_alloc( max_name * sizeof(WCHAR) )))
    {
        ret = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    /* Recursively delete subkeys */
    for (;;)
    {
        name_size = max_name;
        ret = RegEnumKeyExW( hkey, 0, name_buf, &name_size, NULL, NULL, NULL, NULL );
        if (ret == ERROR_NO_MORE_ITEMS) break;
        if (ret) goto cleanup;
        ret = RegDeleteTreeW( hkey, name_buf );
        if (ret) goto cleanup;
    }

    /* Delete the key itself */
    if (subkey && *subkey)
    {
        ret = RegDeleteKeyExW( hkey, emptyW, 0, 0 );
        goto cleanup;
    }

    /* Delete values */
    for (;;)
    {
        name_size = max_name;
        ret = RegEnumValueW( hkey, 0, name_buf, &name_size, NULL, NULL, NULL, NULL );
        if (ret == ERROR_NO_MORE_ITEMS) break;
        if (ret) goto cleanup;
        ret = RegDeleteValueW( hkey, name_buf );
        if (ret) goto cleanup;
    }

    ret = ERROR_SUCCESS;

cleanup:
    heap_free( name_buf );
    if (subkey && *subkey)
        RegCloseKey( hkey );
    return ret;
}


/******************************************************************************
 * RegDeleteTreeA (kernelbase.@)
 *
 */
LSTATUS WINAPI RegDeleteTreeA( HKEY hkey, const char *subkey )
{
    UNICODE_STRING subkeyW;
    LONG ret;

    if (subkey) RtlCreateUnicodeStringFromAsciiz( &subkeyW, subkey );
    else subkeyW.Buffer = NULL;
    ret = RegDeleteTreeW( hkey, subkeyW.Buffer );
    RtlFreeUnicodeString( &subkeyW );
    return ret;
}


/******************************************************************************
 * RegCopyTreeW (kernelbase.@)
 *
 */
LSTATUS WINAPI RegCopyTreeW( HKEY hsrc, const WCHAR *subkey, HKEY hdst )
{
    DWORD name_size, max_name;
    DWORD value_size, max_value;
    DWORD max_subkey, i, type;
    WCHAR *name_buf = NULL;
    BYTE *value_buf = NULL;
    HKEY hkey;
    LONG ret;

    TRACE( "(%p, %s, %p)\n", hsrc, debugstr_w(subkey), hdst );

    if (subkey)
    {
        ret = RegOpenKeyExW( hsrc, subkey, 0, KEY_READ, &hsrc );
        if (ret) return ret;
    }

    ret = RegQueryInfoKeyW( hsrc, NULL, NULL, NULL, NULL, &max_subkey,
                            NULL, NULL, &max_name, &max_value, NULL, NULL );
    if (ret)
        goto cleanup;

    max_name = max( max_subkey, max_name ) + 1;
    if (!(name_buf = heap_alloc( max_name * sizeof(WCHAR) )))
    {
        ret = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    if (!(value_buf = heap_alloc( max_value )))
    {
        ret = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    /* Copy values */
    for (i = 0;; i++)
    {
        name_size = max_name;
        value_size = max_value;
        ret = RegEnumValueW( hsrc, i, name_buf, &name_size, NULL, &type, value_buf, &value_size );
        if (ret == ERROR_NO_MORE_ITEMS) break;
        if (ret) goto cleanup;
        ret = RegSetValueExW( hdst, name_buf, 0, type, value_buf, value_size );
        if (ret) goto cleanup;
    }

    /* Recursively copy subkeys */
    for (i = 0;; i++)
    {
        name_size = max_name;
        ret = RegEnumKeyExW( hsrc, i, name_buf, &name_size, NULL, NULL, NULL, NULL );
        if (ret == ERROR_NO_MORE_ITEMS) break;
        if (ret) goto cleanup;
        ret = RegCreateKeyExW( hdst, name_buf, 0, NULL, 0, KEY_WRITE, NULL, &hkey, NULL );
        if (ret) goto cleanup;
        ret = RegCopyTreeW( hsrc, name_buf, hkey );
        RegCloseKey( hkey );
        if (ret) goto cleanup;
    }

    ret = ERROR_SUCCESS;

cleanup:
    heap_free( name_buf );
    heap_free( value_buf );
    if (subkey)
        RegCloseKey( hsrc );
    return ret;
}


/******************************************************************************
 * RegLoadAppKeyA (kernelbase.@)
 *
 */
LSTATUS WINAPI RegLoadAppKeyA(const char *file, HKEY *result, REGSAM sam, DWORD options, DWORD reserved)
{
    FIXME("%s %p %u %u %u: stub\n", wine_dbgstr_a(file), result, sam, options, reserved);

    if (!file || reserved)
        return ERROR_INVALID_PARAMETER;

    *result = (HKEY)0xdeadbeef;
    return ERROR_SUCCESS;
}

/******************************************************************************
 * RegLoadAppKeyW (kernelbase.@)
 *
 */
LSTATUS WINAPI RegLoadAppKeyW(const WCHAR *file, HKEY *result, REGSAM sam, DWORD options, DWORD reserved)
{
    FIXME("%s %p %u %u %u: stub\n", wine_dbgstr_w(file), result, sam, options, reserved);

    if (!file || reserved)
        return ERROR_INVALID_PARAMETER;

    *result = (HKEY)0xdeadbeef;
    return ERROR_SUCCESS;
}

/******************************************************************************
 *           EnumDynamicTimeZoneInformation   (kernelbase.@)
 */
DWORD WINAPI EnumDynamicTimeZoneInformation(const DWORD index,
    DYNAMIC_TIME_ZONE_INFORMATION *dtzi)
{
    static const WCHAR mui_stdW[] = { 'M','U','I','_','S','t','d',0 };
    static const WCHAR mui_dltW[] = { 'M','U','I','_','D','l','t',0 };
    WCHAR keyname[ARRAY_SIZE(dtzi->TimeZoneKeyName)];
    HKEY time_zones_key, sub_key;
    WCHAR sysdir[MAX_PATH];
    LSTATUS ret;
    DWORD size;
    struct tz_reg_data
    {
        LONG bias;
        LONG std_bias;
        LONG dlt_bias;
        SYSTEMTIME std_date;
        SYSTEMTIME dlt_date;
    } tz_data;

    if (!dtzi)
        return ERROR_INVALID_PARAMETER;

    ret = RegOpenKeyExA( HKEY_LOCAL_MACHINE,
                "Software\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones", 0,
                KEY_ENUMERATE_SUB_KEYS|KEY_QUERY_VALUE, &time_zones_key );
    if (ret) return ret;

    sub_key = NULL;
    size = ARRAY_SIZE(keyname);
    ret = RegEnumKeyExW( time_zones_key, index, keyname, &size, NULL, NULL, NULL, NULL );
    if (ret) goto cleanup;

    ret = RegOpenKeyExW( time_zones_key, keyname, 0, KEY_QUERY_VALUE, &sub_key );
    if (ret) goto cleanup;

    GetSystemDirectoryW(sysdir, ARRAY_SIZE(sysdir));
    size = sizeof(dtzi->StandardName);
    ret = RegLoadMUIStringW( sub_key, mui_stdW, dtzi->StandardName, size, NULL, 0, sysdir );
    if (ret) goto cleanup;

    size = sizeof(dtzi->DaylightName);
    ret = RegLoadMUIStringW( sub_key, mui_dltW, dtzi->DaylightName, size, NULL, 0, sysdir );
    if (ret) goto cleanup;

    size = sizeof(tz_data);
    ret = RegQueryValueExA( sub_key, "TZI", NULL, NULL, (BYTE*)&tz_data, &size );
    if (ret) goto cleanup;

    dtzi->Bias = tz_data.bias;
    dtzi->StandardBias = tz_data.std_bias;
    dtzi->DaylightBias = tz_data.dlt_bias;
    memcpy( &dtzi->StandardDate, &tz_data.std_date, sizeof(tz_data.std_date) );
    memcpy( &dtzi->DaylightDate, &tz_data.dlt_date, sizeof(tz_data.dlt_date) );
    lstrcpyW( dtzi->TimeZoneKeyName, keyname );
    dtzi->DynamicDaylightTimeDisabled = FALSE;

cleanup:
    if (sub_key) RegCloseKey( sub_key );
    RegCloseKey( time_zones_key );
    return ret;
}


struct USKEY
{
    HKEY  HKCUstart; /* Start key in CU hive */
    HKEY  HKCUkey;   /* Opened key in CU hive */
    HKEY  HKLMstart; /* Start key in LM hive */
    HKEY  HKLMkey;   /* Opened key in LM hive */
    WCHAR path[MAX_PATH];
};

LONG WINAPI SHRegCreateUSKeyA(LPCSTR path, REGSAM samDesired, HUSKEY relative_key, PHUSKEY new_uskey, DWORD flags)
{
    WCHAR *pathW;
    LONG ret;

    TRACE("%s, %#x, %p, %p, %#x\n", debugstr_a(path), samDesired, relative_key, new_uskey, flags);

    if (path)
    {
        INT len = MultiByteToWideChar(CP_ACP, 0, path, -1, NULL, 0);
        pathW = heap_alloc(len * sizeof(WCHAR));
        if (!pathW)
            return ERROR_NOT_ENOUGH_MEMORY;
        MultiByteToWideChar(CP_ACP, 0, path, -1, pathW, len);
    }
    else
        pathW = NULL;

    ret = SHRegCreateUSKeyW(pathW, samDesired, relative_key, new_uskey, flags);
    HeapFree(GetProcessHeap(), 0, pathW);
    return ret;
}

static HKEY reg_duplicate_hkey(HKEY hKey)
{
    HKEY newKey = 0;

    RegOpenKeyExW(hKey, 0, 0, MAXIMUM_ALLOWED, &newKey);
    return newKey;
}

static HKEY reg_get_hkey_from_huskey(HUSKEY hUSKey, BOOL is_hkcu)
{
    struct USKEY *mihk = hUSKey;
    HKEY test = hUSKey;

    if (test == HKEY_CLASSES_ROOT
            || test == HKEY_CURRENT_CONFIG
            || test == HKEY_CURRENT_USER
            || test == HKEY_DYN_DATA
            || test == HKEY_LOCAL_MACHINE
            || test == HKEY_PERFORMANCE_DATA
            || test == HKEY_USERS)
/* FIXME:  need to define for Win2k, ME, XP
 *    (test == HKEY_PERFORMANCE_TEXT)    ||
 *    (test == HKEY_PERFORMANCE_NLSTEXT) ||
 */
    {
        return test;
    }

    return is_hkcu ? mihk->HKCUkey : mihk->HKLMkey;
}

LONG WINAPI SHRegCreateUSKeyW(const WCHAR *path, REGSAM samDesired, HUSKEY relative_key, PHUSKEY new_uskey, DWORD flags)
{
    LONG ret = ERROR_CALL_NOT_IMPLEMENTED;
    struct USKEY *ret_key;

    TRACE("%s, %#x, %p, %p, %#x\n", debugstr_w(path), samDesired, relative_key, new_uskey, flags);

    if (!new_uskey)
        return ERROR_INVALID_PARAMETER;

    *new_uskey = NULL;

    if (flags & ~SHREGSET_FORCE_HKCU)
    {
        FIXME("unsupported flags 0x%08x\n", flags);
        return ERROR_SUCCESS;
    }

    ret_key = heap_alloc_zero(sizeof(*ret_key));
    lstrcpynW(ret_key->path, path, ARRAY_SIZE(ret_key->path));

    if (relative_key)
    {
        ret_key->HKCUstart = reg_duplicate_hkey(reg_get_hkey_from_huskey(relative_key, TRUE));
        ret_key->HKLMstart = reg_duplicate_hkey(reg_get_hkey_from_huskey(relative_key, FALSE));
    }
    else
    {
        ret_key->HKCUstart = HKEY_CURRENT_USER;
        ret_key->HKLMstart = HKEY_LOCAL_MACHINE;
    }

    if (flags & SHREGSET_FORCE_HKCU)
    {
        ret = RegCreateKeyExW(ret_key->HKCUstart, path, 0, NULL, 0, samDesired, NULL, &ret_key->HKCUkey, NULL);
        if (ret == ERROR_SUCCESS)
            *new_uskey = ret_key;
        else
            heap_free(ret_key);
    }

    return ret;
}

LONG WINAPI SHRegCloseUSKey(HUSKEY hUSKey)
{
    struct USKEY *key = hUSKey;
    LONG ret = ERROR_SUCCESS;

    if (!key)
        return ERROR_INVALID_PARAMETER;

    if (key->HKCUkey)
        ret = RegCloseKey(key->HKCUkey);
    if (key->HKCUstart && key->HKCUstart != HKEY_CURRENT_USER)
        ret = RegCloseKey(key->HKCUstart);
    if (key->HKLMkey)
        ret = RegCloseKey(key->HKLMkey);
    if (key->HKLMstart && key->HKLMstart != HKEY_LOCAL_MACHINE)
        ret = RegCloseKey(key->HKLMstart);

    heap_free(key);
    return ret;
}

LONG WINAPI SHRegDeleteEmptyUSKeyA(HUSKEY hUSKey, const char *value, SHREGDEL_FLAGS flags)
{
    FIXME("%p, %s, %#x\n", hUSKey, debugstr_a(value), flags);
    return ERROR_SUCCESS;
}

LONG WINAPI SHRegDeleteEmptyUSKeyW(HUSKEY hUSKey, const WCHAR *value, SHREGDEL_FLAGS flags)
{
    FIXME("%p, %s, %#x\n", hUSKey, debugstr_w(value), flags);
    return ERROR_SUCCESS;
}

LONG WINAPI SHRegDeleteUSValueA(HUSKEY hUSKey, const char *value, SHREGDEL_FLAGS flags)
{
    FIXME("%p, %s, %#x\n", hUSKey, debugstr_a(value), flags);
    return ERROR_SUCCESS;
}

LONG WINAPI SHRegDeleteUSValueW(HUSKEY hUSKey, const WCHAR *value, SHREGDEL_FLAGS flags)
{
    FIXME("%p, %s, %#x\n", hUSKey, debugstr_w(value), flags);
    return ERROR_SUCCESS;
}

LONG WINAPI SHRegEnumUSValueA(HUSKEY hUSKey, DWORD index, char *value_name, DWORD *value_name_len, DWORD *type,
        void *data, DWORD *data_len, SHREGENUM_FLAGS flags)
{
    HKEY dokey;

    TRACE("%p, %#x, %p, %p, %p, %p, %p, %#x\n", hUSKey, index, value_name, value_name_len, type, data, data_len, flags);

    if ((flags == SHREGENUM_HKCU || flags == SHREGENUM_DEFAULT) && (dokey = reg_get_hkey_from_huskey(hUSKey, TRUE)))
        return RegEnumValueA(dokey, index, value_name, value_name_len, NULL, type, data, data_len);

    if ((flags == SHREGENUM_HKLM || flags == SHREGENUM_DEFAULT) && (dokey = reg_get_hkey_from_huskey(hUSKey, FALSE)))
        return RegEnumValueA(dokey, index, value_name, value_name_len, NULL, type, data, data_len);

    FIXME("no support for SHREGENUM_BOTH\n");
    return ERROR_INVALID_FUNCTION;
}

LONG WINAPI SHRegEnumUSValueW(HUSKEY hUSKey, DWORD index, WCHAR *value_name, DWORD *value_name_len, DWORD *type,
        void *data, DWORD *data_len, SHREGENUM_FLAGS flags)
{
    HKEY dokey;

    TRACE("%p, %#x, %p, %p, %p, %p, %p, %#x\n", hUSKey, index, value_name, value_name_len, type, data, data_len, flags);

    if ((flags == SHREGENUM_HKCU || flags == SHREGENUM_DEFAULT) && (dokey = reg_get_hkey_from_huskey(hUSKey, TRUE)))
        return RegEnumValueW(dokey, index, value_name, value_name_len, NULL, type, data, data_len);

    if ((flags == SHREGENUM_HKLM || flags == SHREGENUM_DEFAULT) && (dokey = reg_get_hkey_from_huskey(hUSKey, FALSE)))
        return RegEnumValueW(dokey, index, value_name, value_name_len, NULL, type, data, data_len);

    FIXME("no support for SHREGENUM_BOTH\n");
    return ERROR_INVALID_FUNCTION;
}

LONG WINAPI SHRegEnumUSKeyA(HUSKEY hUSKey, DWORD index, char *name, DWORD *name_len, SHREGENUM_FLAGS flags)
{
    HKEY dokey;

    TRACE("%p, %d, %p, %p(%d), %d\n", hUSKey, index, name, name_len, *name_len, flags);

    if ((flags == SHREGENUM_HKCU || flags == SHREGENUM_DEFAULT) && (dokey = reg_get_hkey_from_huskey(hUSKey, TRUE)))
        return RegEnumKeyExA(dokey, index, name, name_len, 0, 0, 0, 0);

    if ((flags == SHREGENUM_HKLM || flags == SHREGENUM_DEFAULT) && (dokey = reg_get_hkey_from_huskey(hUSKey, FALSE)))
        return RegEnumKeyExA(dokey, index, name, name_len, 0, 0, 0, 0);

    FIXME("no support for SHREGENUM_BOTH\n");
    return ERROR_INVALID_FUNCTION;
}

LONG WINAPI SHRegEnumUSKeyW(HUSKEY hUSKey, DWORD index, WCHAR *name, DWORD *name_len, SHREGENUM_FLAGS flags)
{
    HKEY dokey;

    TRACE("%p, %d, %p, %p(%d), %d\n", hUSKey, index, name, name_len, *name_len, flags);

    if ((flags == SHREGENUM_HKCU || flags == SHREGENUM_DEFAULT) && (dokey = reg_get_hkey_from_huskey(hUSKey, TRUE)))
        return RegEnumKeyExW(dokey, index, name, name_len, 0, 0, 0, 0);

    if ((flags == SHREGENUM_HKLM || flags == SHREGENUM_DEFAULT) && (dokey = reg_get_hkey_from_huskey(hUSKey, FALSE)))
        return RegEnumKeyExW(dokey, index, name, name_len, 0, 0, 0, 0);

    FIXME("no support for SHREGENUM_BOTH\n");
    return ERROR_INVALID_FUNCTION;
}

LONG WINAPI SHRegOpenUSKeyA(const char *path, REGSAM access_mask, HUSKEY relative_key, HUSKEY *uskey, BOOL ignore_hkcu)
{
    WCHAR pathW[MAX_PATH];

    if (path)
        MultiByteToWideChar(CP_ACP, 0, path, -1, pathW, ARRAY_SIZE(pathW));

    return SHRegOpenUSKeyW(path ? pathW : NULL, access_mask, relative_key, uskey, ignore_hkcu);
}

LONG WINAPI SHRegOpenUSKeyW(const WCHAR *path, REGSAM access_mask, HUSKEY relative_key, HUSKEY *uskey, BOOL ignore_hkcu)
{
    LONG ret2, ret1 = ~ERROR_SUCCESS;
    struct USKEY *key;

    TRACE("%s, %#x, %p, %p, %d\n", debugstr_w(path), access_mask, relative_key, uskey, ignore_hkcu);

    if (uskey)
        *uskey = NULL;

    /* Create internal HUSKEY */
    key = heap_alloc_zero(sizeof(*key));
    lstrcpynW(key->path, path, ARRAY_SIZE(key->path));

    if (relative_key)
    {
        key->HKCUstart = reg_duplicate_hkey(reg_get_hkey_from_huskey(relative_key, TRUE));
        key->HKLMstart = reg_duplicate_hkey(reg_get_hkey_from_huskey(relative_key, FALSE));

        /* FIXME: if either of these keys is NULL, create the start key from
         *        the relative keys start+path
         */
    }
    else
    {
        key->HKCUstart = HKEY_CURRENT_USER;
        key->HKLMstart = HKEY_LOCAL_MACHINE;
    }

    if (!ignore_hkcu)
    {
        ret1 = RegOpenKeyExW(key->HKCUstart, key->path, 0, access_mask, &key->HKCUkey);
        if (ret1)
            key->HKCUkey = 0;
    }

    ret2 = RegOpenKeyExW(key->HKLMstart, key->path, 0, access_mask, &key->HKLMkey);
    if (ret2)
        key->HKLMkey = 0;

    if (ret1 || ret2)
        TRACE("one or more opens failed: HKCU=%d HKLM=%d\n", ret1, ret2);

    if (ret1 && ret2)
    {
        /* Neither open succeeded: fail */
        SHRegCloseUSKey(key);
        return ret2;
    }

    TRACE("HUSKEY=%p\n", key);
    if (uskey)
        *uskey = key;

    return ERROR_SUCCESS;
}

LONG WINAPI SHRegWriteUSValueA(HUSKEY hUSKey, const char *value, DWORD type, void *data, DWORD data_len, DWORD flags)
{
    WCHAR valueW[MAX_PATH];

    if (value)
        MultiByteToWideChar(CP_ACP, 0, value, -1, valueW, ARRAY_SIZE(valueW));

    return SHRegWriteUSValueW(hUSKey, value ? valueW : NULL, type, data, data_len, flags);
}

LONG WINAPI SHRegWriteUSValueW(HUSKEY hUSKey, const WCHAR *value, DWORD type, void *data, DWORD data_len, DWORD flags)
{
    struct USKEY *hKey = hUSKey;
    LONG ret = ERROR_SUCCESS;
    DWORD dummy;

    TRACE("%p, %s, %d, %p, %d, %#x\n", hUSKey, debugstr_w(value), type, data, data_len, flags);

    if (!hUSKey || IsBadWritePtr(hUSKey, sizeof(struct USKEY)) || !(flags & (SHREGSET_FORCE_HKCU|SHREGSET_FORCE_HKLM)))
        return ERROR_INVALID_PARAMETER;

    if (flags & (SHREGSET_FORCE_HKCU | SHREGSET_HKCU))
    {
        if (!hKey->HKCUkey)
        {
            /* Create the key */
            ret = RegCreateKeyExW(hKey->HKCUstart, hKey->path, 0, NULL, REG_OPTION_NON_VOLATILE,
                                  MAXIMUM_ALLOWED, NULL, &hKey->HKCUkey, NULL);
            TRACE("Creating HKCU key, ret = %d\n", ret);
            if (ret && (flags & SHREGSET_FORCE_HKCU))
            {
                hKey->HKCUkey = 0;
                return ret;
            }
        }

        if (!ret)
        {
            if ((flags & SHREGSET_FORCE_HKCU) || RegQueryValueExW(hKey->HKCUkey, value, NULL, NULL, NULL, &dummy))
            {
                /* Doesn't exist or we are forcing: Write value */
                ret = RegSetValueExW(hKey->HKCUkey, value, 0, type, data, data_len);
                TRACE("Writing HKCU value, ret = %d\n", ret);
            }
        }
    }

    if (flags & (SHREGSET_FORCE_HKLM | SHREGSET_HKLM))
    {
        if (!hKey->HKLMkey)
        {
            /* Create the key */
            ret = RegCreateKeyExW(hKey->HKLMstart, hKey->path, 0, NULL, REG_OPTION_NON_VOLATILE,
                                  MAXIMUM_ALLOWED, NULL, &hKey->HKLMkey, NULL);
            TRACE("Creating HKLM key, ret = %d\n", ret);
            if (ret && (flags & (SHREGSET_FORCE_HKLM)))
            {
                hKey->HKLMkey = 0;
                return ret;
            }
        }

        if (!ret)
        {
            if ((flags & SHREGSET_FORCE_HKLM) || RegQueryValueExW(hKey->HKLMkey, value, NULL, NULL, NULL, &dummy))
            {
                /* Doesn't exist or we are forcing: Write value */
                ret = RegSetValueExW(hKey->HKLMkey, value, 0, type, data, data_len);
                TRACE("Writing HKLM value, ret = %d\n", ret);
            }
        }
    }

    return ret;
}

LONG WINAPI SHRegSetUSValueA(const char *subkey, const char *value, DWORD type, void *data, DWORD data_len,
        DWORD flags)
{
    BOOL ignore_hkcu;
    HUSKEY hkey;
    LONG ret;

    TRACE("%s, %s, %d, %p, %d, %#x\n", debugstr_a(subkey), debugstr_a(value), type, data, data_len, flags);

    if (!data)
        return ERROR_INVALID_FUNCTION;

    ignore_hkcu = !(flags & SHREGSET_HKCU || flags & SHREGSET_FORCE_HKCU);

    ret = SHRegOpenUSKeyA(subkey, KEY_ALL_ACCESS, 0, &hkey, ignore_hkcu);
    if (ret == ERROR_SUCCESS)
    {
        ret = SHRegWriteUSValueA(hkey, value, type, data, data_len, flags);
        SHRegCloseUSKey(hkey);
    }

    return ret;
}

LONG WINAPI SHRegSetUSValueW(const WCHAR *subkey, const WCHAR *value, DWORD type, void *data, DWORD data_len,
        DWORD flags)
{
    BOOL ignore_hkcu;
    HUSKEY hkey;
    LONG ret;

    TRACE("%s, %s, %d, %p, %d, %#x\n", debugstr_w(subkey), debugstr_w(value), type, data, data_len, flags);

    if (!data)
        return ERROR_INVALID_FUNCTION;

    ignore_hkcu = !(flags & SHREGSET_HKCU || flags & SHREGSET_FORCE_HKCU);

    ret = SHRegOpenUSKeyW(subkey, KEY_ALL_ACCESS, 0, &hkey, ignore_hkcu);
    if (ret == ERROR_SUCCESS)
    {
        ret = SHRegWriteUSValueW(hkey, value, type, data, data_len, flags);
        SHRegCloseUSKey(hkey);
    }

    return ret;
}

LONG WINAPI SHRegQueryInfoUSKeyA(HUSKEY hUSKey, DWORD *subkeys, DWORD *max_subkey_len, DWORD *values,
        DWORD *max_value_name_len, SHREGENUM_FLAGS flags)
{
    HKEY dokey;
    LONG ret;

    TRACE("%p, %p, %p, %p, %p, %#x\n", hUSKey, subkeys, max_subkey_len, values, max_value_name_len, flags);

    if ((flags == SHREGENUM_HKCU || flags == SHREGENUM_DEFAULT) && (dokey = reg_get_hkey_from_huskey(hUSKey, TRUE)))
    {
        ret = RegQueryInfoKeyA(dokey, 0, 0, 0, subkeys, max_subkey_len, 0, values, max_value_name_len, 0, 0, 0);
        if (ret == ERROR_SUCCESS || flags == SHREGENUM_HKCU)
            return ret;
    }

    if ((flags == SHREGENUM_HKLM || flags == SHREGENUM_DEFAULT) && (dokey = reg_get_hkey_from_huskey(hUSKey, FALSE)))
    {
        return RegQueryInfoKeyA(dokey, 0, 0, 0, subkeys, max_subkey_len, 0, values, max_value_name_len, 0, 0, 0);
    }

    return ERROR_INVALID_FUNCTION;
}

LONG WINAPI SHRegQueryInfoUSKeyW(HUSKEY hUSKey, DWORD *subkeys, DWORD *max_subkey_len, DWORD *values,
        DWORD *max_value_name_len, SHREGENUM_FLAGS flags)
{
    HKEY dokey;
    LONG ret;

    TRACE("%p, %p, %p, %p, %p, %#x\n", hUSKey, subkeys, max_subkey_len, values, max_value_name_len, flags);

    if ((flags == SHREGENUM_HKCU || flags == SHREGENUM_DEFAULT) && (dokey = reg_get_hkey_from_huskey(hUSKey, TRUE)))
    {
        ret = RegQueryInfoKeyW(dokey, 0, 0, 0, subkeys, max_subkey_len, 0, values, max_value_name_len, 0, 0, 0);
        if (ret == ERROR_SUCCESS || flags == SHREGENUM_HKCU)
            return ret;
    }

    if ((flags == SHREGENUM_HKLM || flags == SHREGENUM_DEFAULT) && (dokey = reg_get_hkey_from_huskey(hUSKey, FALSE)))
    {
        return RegQueryInfoKeyW(dokey, 0, 0, 0, subkeys, max_subkey_len, 0, values, max_value_name_len, 0, 0, 0);
    }

    return ERROR_INVALID_FUNCTION;
}

LONG WINAPI SHRegQueryUSValueA(HUSKEY hUSKey, const char *value, DWORD *type, void *data, DWORD *data_len,
        BOOL ignore_hkcu, void *default_data, DWORD default_data_len)
{
    LONG ret = ~ERROR_SUCCESS;
    DWORD move_len;
    HKEY dokey;

    /* If user wants HKCU, and it exists, then try it */
    if (!ignore_hkcu && (dokey = reg_get_hkey_from_huskey(hUSKey, TRUE)))
    {
        ret = RegQueryValueExA(dokey, value, 0, type, data, data_len);
        TRACE("HKCU RegQueryValue returned %d\n", ret);
    }

    /* If HKCU did not work and HKLM exists, then try it */
    if ((ret != ERROR_SUCCESS) && (dokey = reg_get_hkey_from_huskey(hUSKey, FALSE)))
    {
        ret = RegQueryValueExA(dokey, value, 0, type, data, data_len);
        TRACE("HKLM RegQueryValue returned %d\n", ret);
    }

    /* If neither worked, and default data exists, then use it */
    if (ret != ERROR_SUCCESS)
    {
        if (default_data && default_data_len)
        {
            move_len = default_data_len >= *data_len ? *data_len : default_data_len;
            memmove(data, default_data, move_len);
            *data_len = move_len;
            TRACE("setting default data\n");
            ret = ERROR_SUCCESS;
        }
    }

    return ret;
}

LONG WINAPI SHRegQueryUSValueW(HUSKEY hUSKey, const WCHAR *value, DWORD *type, void *data, DWORD *data_len,
        BOOL ignore_hkcu, void *default_data, DWORD default_data_len)
{
    LONG ret = ~ERROR_SUCCESS;
    DWORD move_len;
    HKEY dokey;

    /* If user wants HKCU, and it exists, then try it */
    if (!ignore_hkcu && (dokey = reg_get_hkey_from_huskey(hUSKey, TRUE)))
    {
        ret = RegQueryValueExW(dokey, value, 0, type, data, data_len);
        TRACE("HKCU RegQueryValue returned %d\n", ret);
    }

    /* If HKCU did not work and HKLM exists, then try it */
    if ((ret != ERROR_SUCCESS) && (dokey = reg_get_hkey_from_huskey(hUSKey, FALSE)))
    {
        ret = RegQueryValueExW(dokey, value, 0, type, data, data_len);
        TRACE("HKLM RegQueryValue returned %d\n", ret);
    }

    /* If neither worked, and default data exists, then use it */
    if (ret != ERROR_SUCCESS)
    {
        if (default_data && default_data_len)
        {
            move_len = default_data_len >= *data_len ? *data_len : default_data_len;
            memmove(data, default_data, move_len);
            *data_len = move_len;
            TRACE("setting default data\n");
            ret = ERROR_SUCCESS;
        }
    }

    return ret;
}

LONG WINAPI SHRegGetUSValueA(const char *subkey, const char *value, DWORD *type, void *data, DWORD *data_len,
        BOOL ignore_hkcu, void *default_data, DWORD default_data_len)
{
    HUSKEY myhuskey;
    LONG ret;

    if (!data || !data_len)
        return ERROR_INVALID_FUNCTION; /* FIXME:wrong*/

    TRACE("%s, %s, %d\n", debugstr_a(subkey), debugstr_a(value), *data_len);

    ret = SHRegOpenUSKeyA(subkey, KEY_QUERY_VALUE, 0, &myhuskey, ignore_hkcu);
    if (!ret)
    {
        ret = SHRegQueryUSValueA(myhuskey, value, type, data, data_len, ignore_hkcu, default_data, default_data_len);
        SHRegCloseUSKey(myhuskey);
    }

    return ret;
}

LONG WINAPI SHRegGetUSValueW(const WCHAR *subkey, const WCHAR *value, DWORD *type, void *data, DWORD *data_len,
        BOOL ignore_hkcu, void *default_data, DWORD default_data_len)
{
    HUSKEY myhuskey;
    LONG ret;

    if (!data || !data_len)
        return ERROR_INVALID_FUNCTION; /* FIXME:wrong*/

    TRACE("%s, %s, %d\n", debugstr_w(subkey), debugstr_w(value), *data_len);

    ret = SHRegOpenUSKeyW(subkey, KEY_QUERY_VALUE, 0, &myhuskey, ignore_hkcu);
    if (!ret)
    {
        ret = SHRegQueryUSValueW(myhuskey, value, type, data, data_len, ignore_hkcu, default_data, default_data_len);
        SHRegCloseUSKey(myhuskey);
    }

    return ret;
}

BOOL WINAPI SHRegGetBoolUSValueA(const char *subkey, const char *value, BOOL ignore_hkcu, BOOL default_value)
{
    BOOL ret = default_value;
    DWORD type, datalen;
    char data[10];

    TRACE("%s, %s, %d\n", debugstr_a(subkey), debugstr_a(value), ignore_hkcu);

    datalen = ARRAY_SIZE(data) - 1;
    if (!SHRegGetUSValueA(subkey, value, &type, data, &datalen, ignore_hkcu, 0, 0))
    {
        switch (type)
        {
            case REG_SZ:
                data[9] = '\0';
                if (!lstrcmpiA(data, "YES") || !lstrcmpiA(data, "TRUE"))
                    ret = TRUE;
                else if (!lstrcmpiA(data, "NO") || !lstrcmpiA(data, "FALSE"))
                    ret = FALSE;
                break;
            case REG_DWORD:
                ret = *(DWORD *)data != 0;
                break;
            case REG_BINARY:
                if (datalen == 1)
                {
                    ret = !!data[0];
                    break;
                }
            default:
                FIXME("Unsupported registry data type %d\n", type);
                ret = FALSE;
        }
        TRACE("got value (type=%d), returning %d\n", type, ret);
    }
    else
        TRACE("returning default value %d\n", ret);

    return ret;
}

BOOL WINAPI SHRegGetBoolUSValueW(const WCHAR *subkey, const WCHAR *value, BOOL ignore_hkcu, BOOL default_value)
{
    static const WCHAR yesW[]= {'Y','E','S',0};
    static const WCHAR trueW[] = {'T','R','U','E',0};
    static const WCHAR noW[] = {'N','O',0};
    static const WCHAR falseW[] = {'F','A','L','S','E',0};
    BOOL ret = default_value;
    DWORD type, datalen;
    WCHAR data[10];

    TRACE("%s, %s, %d\n", debugstr_w(subkey), debugstr_w(value), ignore_hkcu);

    datalen = (ARRAY_SIZE(data) - 1) * sizeof(WCHAR);
    if (!SHRegGetUSValueW(subkey, value, &type, data, &datalen, ignore_hkcu, 0, 0))
    {
        switch (type)
        {
            case REG_SZ:
                data[9] = '\0';
                if (!lstrcmpiW(data, yesW) || !lstrcmpiW(data, trueW))
                    ret = TRUE;
                else if (!lstrcmpiW(data, noW) || !lstrcmpiW(data, falseW))
                    ret = FALSE;
                break;
            case REG_DWORD:
                ret = *(DWORD *)data != 0;
                break;
            case REG_BINARY:
                if (datalen == 1)
                {
                    ret = !!data[0];
                    break;
                }
            default:
                FIXME("Unsupported registry data type %d\n", type);
                ret = FALSE;
        }
        TRACE("got value (type=%d), returning %d\n", type, ret);
    }
    else
        TRACE("returning default value %d\n", ret);

    return ret;
}
