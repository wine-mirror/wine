/*
 * Win32 VxD functions
 *
 * Copyright 1998 Marcus Meissner
 * Copyright 1998 Ulrich Weigand
 * Copyright 1998 Patrik Stridvall
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

#include <stdlib.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <sys/types.h>
#include <string.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winerror.h"
#include "winnls.h"
#include "ntstatus.h"
#include "winnt.h"
#include "winternl.h"
#include "wine/winbase16.h"
#include "kernel_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(vxd);

/*
 * VMM VxDCall service names are (mostly) taken from Stan Mitchell's
 * "Inside the Windows 95 File System"
 */

#define N_VMM_SERVICE 41

static const char * const VMM_Service_Name[N_VMM_SERVICE] =
{
    "PageReserve",            /* 0x0000 */
    "PageCommit",             /* 0x0001 */
    "PageDecommit",           /* 0x0002 */
    "PagerRegister",          /* 0x0003 */
    "PagerQuery",             /* 0x0004 */
    "HeapAllocate",           /* 0x0005 */
    "ContextCreate",          /* 0x0006 */
    "ContextDestroy",         /* 0x0007 */
    "PageAttach",             /* 0x0008 */
    "PageFlush",              /* 0x0009 */
    "PageFree",               /* 0x000A */
    "ContextSwitch",          /* 0x000B */
    "HeapReAllocate",         /* 0x000C */
    "PageModifyPermissions",  /* 0x000D */
    "PageQuery",              /* 0x000E */
    "GetCurrentContext",      /* 0x000F */
    "HeapFree",               /* 0x0010 */
    "RegOpenKey",             /* 0x0011 */
    "RegCreateKey",           /* 0x0012 */
    "RegCloseKey",            /* 0x0013 */
    "RegDeleteKey",           /* 0x0014 */
    "RegSetValue",            /* 0x0015 */
    "RegDeleteValue",         /* 0x0016 */
    "RegQueryValue",          /* 0x0017 */
    "RegEnumKey",             /* 0x0018 */
    "RegEnumValue",           /* 0x0019 */
    "RegQueryValueEx",        /* 0x001A */
    "RegSetValueEx",          /* 0x001B */
    "RegFlushKey",            /* 0x001C */
    "RegQueryInfoKey",        /* 0x001D */
    "GetDemandPageInfo",      /* 0x001E */
    "BlockOnID",              /* 0x001F */
    "SignalID",               /* 0x0020 */
    "RegLoadKey",             /* 0x0021 */
    "RegUnLoadKey",           /* 0x0022 */
    "RegSaveKey",             /* 0x0023 */
    "RegRemapPreDefKey",      /* 0x0024 */
    "PageChangePager",        /* 0x0025 */
    "RegQueryMultipleValues", /* 0x0026 */
    "RegReplaceKey",          /* 0x0027 */
    "<KERNEL32.101>"          /* 0x0028 -- What does this do??? */
};

/* PageReserve arena values */
#define PR_PRIVATE    0x80000400 /* anywhere in private arena */
#define PR_SHARED     0x80060000 /* anywhere in shared arena */
#define PR_SYSTEM     0x80080000 /* anywhere in system arena */

/* PageReserve flags */
#define PR_FIXED      0x00000008 /* don't move during PageReAllocate */
#define PR_4MEG       0x00000001 /* allocate on 4mb boundary */
#define PR_STATIC     0x00000010 /* see PageReserve documentation */

/* PageCommit default pager handle values */
#define PD_ZEROINIT   0x00000001 /* swappable zero-initialized pages */
#define PD_NOINIT     0x00000002 /* swappable uninitialized pages */
#define PD_FIXEDZERO  0x00000003 /* fixed zero-initialized pages */
#define PD_FIXED      0x00000004 /* fixed uninitialized pages */

/* PageCommit flags */
#define PC_FIXED      0x00000008 /* pages are permanently locked */
#define PC_LOCKED     0x00000080 /* pages are made present and locked */
#define PC_LOCKEDIFDP 0x00000100 /* pages are locked if swap via DOS */
#define PC_WRITEABLE  0x00020000 /* make the pages writeable */
#define PC_USER       0x00040000 /* make the pages ring 3 accessible */
#define PC_INCR       0x40000000 /* increment "pagerdata" each page */
#define PC_PRESENT    0x80000000 /* make pages initially present */
#define PC_STATIC     0x20000000 /* allow commit in PR_STATIC object */
#define PC_DIRTY      0x08000000 /* make pages initially dirty */
#define PC_CACHEDIS   0x00100000 /* Allocate uncached pages - new for WDM */
#define PC_CACHEWT    0x00080000 /* Allocate write through cache pages - new for WDM */
#define PC_PAGEFLUSH  0x00008000 /* Touch device mapped pages on alloc - new for WDM */

/* PageCommitContig additional flags */
#define PCC_ZEROINIT  0x00000001 /* zero-initialize new pages */
#define PCC_NOLIN     0x10000000 /* don't map to any linear address */


/* Pop a DWORD from the 32-bit stack */
static inline DWORD stack32_pop( CONTEXT86 *context )
{
    DWORD ret = *(DWORD *)context->Esp;
    context->Esp += sizeof(DWORD);
    return ret;
}


/******************************************************************************
 * The following is a massive duplication of the advapi32 code.
 * Unfortunately sharing the code is not possible since the native
 * Win95 advapi32 depends on it. Someday we should probably stop
 * supporting native Win95 advapi32 altogether...
 */


#define HKEY_SPECIAL_ROOT_FIRST   HKEY_CLASSES_ROOT
#define HKEY_SPECIAL_ROOT_LAST    HKEY_DYN_DATA
#define NB_SPECIAL_ROOT_KEYS      ((UINT)HKEY_SPECIAL_ROOT_LAST - (UINT)HKEY_SPECIAL_ROOT_FIRST + 1)

static HKEY special_root_keys[NB_SPECIAL_ROOT_KEYS];

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
     'H','a','r','d','w','a','r','e','P','r','o','f','i','l','e','s','\\',
     'C','u','r','r','e','n','t',0};
static const WCHAR name_DYN_DATA[] =
    {'D','y','n','D','a','t','a',0};

#define DECL_STR(key) { sizeof(name_##key)-sizeof(WCHAR), sizeof(name_##key), (LPWSTR)name_##key }
static UNICODE_STRING root_key_names[NB_SPECIAL_ROOT_KEYS] =
{
    DECL_STR(CLASSES_ROOT),
    { 0, 0, NULL },         /* HKEY_CURRENT_USER is determined dynamically */
    DECL_STR(LOCAL_MACHINE),
    DECL_STR(USERS),
    DECL_STR(PERFORMANCE_DATA),
    DECL_STR(CURRENT_CONFIG),
    DECL_STR(DYN_DATA)
};
#undef DECL_STR


/* check if value type needs string conversion (Ansi<->Unicode) */
inline static int is_string( DWORD type )
{
    return (type == REG_SZ) || (type == REG_EXPAND_SZ) || (type == REG_MULTI_SZ);
}

/* create one of the HKEY_* special root keys */
static HKEY create_special_root_hkey( HKEY hkey, DWORD access )
{
    HKEY ret = 0;
    int idx = (UINT)hkey - (UINT)HKEY_SPECIAL_ROOT_FIRST;

    if (hkey == HKEY_CURRENT_USER)
    {
        if (RtlOpenCurrentUser( access, &hkey )) return 0;
    }
    else
    {
        OBJECT_ATTRIBUTES attr;

        attr.Length = sizeof(attr);
        attr.RootDirectory = 0;
        attr.ObjectName = &root_key_names[idx];
        attr.Attributes = 0;
        attr.SecurityDescriptor = NULL;
        attr.SecurityQualityOfService = NULL;
        if (NtCreateKey( &hkey, access, &attr, 0, NULL, 0, NULL )) return 0;
    }

    if (!(ret = InterlockedCompareExchangePointer( (PVOID) &special_root_keys[idx], hkey, 0 )))
        ret = hkey;
    else
        NtClose( hkey );  /* somebody beat us to it */
    return ret;
}

/* map the hkey from special root to normal key if necessary */
inline static HKEY get_special_root_hkey( HKEY hkey )
{
    HKEY ret = hkey;

    if ((hkey >= HKEY_SPECIAL_ROOT_FIRST) && (hkey <= HKEY_SPECIAL_ROOT_LAST))
    {
        if (!(ret = special_root_keys[(UINT)hkey - (UINT)HKEY_SPECIAL_ROOT_FIRST]))
            ret = create_special_root_hkey( hkey, KEY_ALL_ACCESS );
    }
    return ret;
}


/******************************************************************************
 *           VMM_RegCreateKeyA
 */
static DWORD VMM_RegCreateKeyA( HKEY hkey, LPCSTR name, PHKEY retkey )
{
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    ANSI_STRING nameA;
    NTSTATUS status;

    if (!(hkey = get_special_root_hkey( hkey ))) return ERROR_INVALID_HANDLE;

    attr.Length = sizeof(attr);
    attr.RootDirectory = hkey;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    RtlInitAnsiString( &nameA, name );

    if (!(status = RtlAnsiStringToUnicodeString( &nameW, &nameA, TRUE )))
    {
        status = NtCreateKey( retkey, KEY_ALL_ACCESS, &attr, 0, NULL,
                              REG_OPTION_NON_VOLATILE, NULL );
        RtlFreeUnicodeString( &nameW );
    }
    return RtlNtStatusToDosError( status );
}


/******************************************************************************
 *           VMM_RegOpenKeyExA
 */
DWORD WINAPI VMM_RegOpenKeyExA(HKEY hkey, LPCSTR name, DWORD reserved, REGSAM access, PHKEY retkey)
{
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    STRING nameA;
    NTSTATUS status;

    if (!(hkey = get_special_root_hkey( hkey ))) return ERROR_INVALID_HANDLE;

    attr.Length = sizeof(attr);
    attr.RootDirectory = hkey;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    RtlInitAnsiString( &nameA, name );
    if (!(status = RtlAnsiStringToUnicodeString( &nameW, &nameA, TRUE )))
    {
        status = NtOpenKey( retkey, access, &attr );
        RtlFreeUnicodeString( &nameW );
    }
    return RtlNtStatusToDosError( status );
}


/******************************************************************************
 *           VMM_RegCloseKey
 */
static DWORD VMM_RegCloseKey( HKEY hkey )
{
    if (!hkey || hkey >= (HKEY)0x80000000) return ERROR_SUCCESS;
    return RtlNtStatusToDosError( NtClose( hkey ) );
}


/******************************************************************************
 *           VMM_RegDeleteKeyA
 */
static DWORD VMM_RegDeleteKeyA( HKEY hkey, LPCSTR name )
{
    DWORD ret;
    HKEY tmp;

    if (!(hkey = get_special_root_hkey( hkey ))) return ERROR_INVALID_HANDLE;

    if (!name || !*name) return RtlNtStatusToDosError( NtDeleteKey( hkey ) );
    if (!(ret = VMM_RegOpenKeyExA( hkey, name, 0, 0, &tmp )))
    {
        ret = RtlNtStatusToDosError( NtDeleteKey( tmp ) );
        NtClose( tmp );
    }
    return ret;
}


/******************************************************************************
 *           VMM_RegSetValueExA
 */
static DWORD VMM_RegSetValueExA( HKEY hkey, LPCSTR name, DWORD reserved, DWORD type,
                                 CONST BYTE *data, DWORD count )
{
    UNICODE_STRING nameW;
    ANSI_STRING nameA;
    WCHAR *dataW = NULL;
    NTSTATUS status;

    if (!(hkey = get_special_root_hkey( hkey ))) return ERROR_INVALID_HANDLE;

    if (is_string(type))
    {
        DWORD lenW;

        if (count)
        {
            /* if user forgot to count terminating null, add it (yes NT does this) */
            if (data[count-1] && !data[count]) count++;
        }
        RtlMultiByteToUnicodeSize( &lenW, data, count );
        if (!(dataW = HeapAlloc( GetProcessHeap(), 0, lenW ))) return ERROR_OUTOFMEMORY;
        RtlMultiByteToUnicodeN( dataW, lenW, NULL, data, count );
        count = lenW;
        data = (BYTE *)dataW;
    }

    RtlInitAnsiString( &nameA, name );
    if (!(status = RtlAnsiStringToUnicodeString( &nameW, &nameA, TRUE )))
    {
        status = NtSetValueKey( hkey, &nameW, 0, type, data, count );
        RtlFreeUnicodeString( &nameW );
    }
    if (dataW) HeapFree( GetProcessHeap(), 0, dataW );
    return RtlNtStatusToDosError( status );
}


/******************************************************************************
 *           VMM_RegSetValueA
 */
static DWORD VMM_RegSetValueA( HKEY hkey, LPCSTR name, DWORD type, LPCSTR data, DWORD count )
{
    HKEY subkey = hkey;
    DWORD ret;

    if (type != REG_SZ) return ERROR_INVALID_PARAMETER;

    if (name && name[0])  /* need to create the subkey */
    {
        if ((ret = VMM_RegCreateKeyA( hkey, name, &subkey )) != ERROR_SUCCESS) return ret;
    }
    ret = VMM_RegSetValueExA( subkey, NULL, 0, REG_SZ, (LPBYTE)data, strlen(data)+1 );
    if (subkey != hkey) NtClose( subkey );
    return ret;
}


/******************************************************************************
 *           VMM_RegDeleteValueA
 */
static DWORD VMM_RegDeleteValueA( HKEY hkey, LPCSTR name )
{
    UNICODE_STRING nameW;
    STRING nameA;
    NTSTATUS status;

    if (!(hkey = get_special_root_hkey( hkey ))) return ERROR_INVALID_HANDLE;

    RtlInitAnsiString( &nameA, name );
    if (!(status = RtlAnsiStringToUnicodeString( &nameW, &nameA, TRUE )))
    {
        status = NtDeleteValueKey( hkey, &nameW );
        RtlFreeUnicodeString( &nameW );
    }
    return RtlNtStatusToDosError( status );
}


/******************************************************************************
 *           VMM_RegQueryValueExA
 */
static DWORD VMM_RegQueryValueExA( HKEY hkey, LPCSTR name, LPDWORD reserved, LPDWORD type,
                                   LPBYTE data, LPDWORD count )
{
    NTSTATUS status;
    ANSI_STRING nameA;
    UNICODE_STRING nameW;
    DWORD total_size;
    char buffer[256], *buf_ptr = buffer;
    KEY_VALUE_PARTIAL_INFORMATION *info = (KEY_VALUE_PARTIAL_INFORMATION *)buffer;
    static const int info_size = offsetof( KEY_VALUE_PARTIAL_INFORMATION, Data );

    if ((data && !count) || reserved) return ERROR_INVALID_PARAMETER;
    if (!(hkey = get_special_root_hkey( hkey ))) return ERROR_INVALID_HANDLE;

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
            if (buf_ptr != buffer) HeapFree( GetProcessHeap(), 0, buf_ptr );
            if (!(buf_ptr = HeapAlloc( GetProcessHeap(), 0, total_size )))
            {
                status = STATUS_NO_MEMORY;
                goto done;
            }
            info = (KEY_VALUE_PARTIAL_INFORMATION *)buf_ptr;
            status = NtQueryValueKey( hkey, &nameW, KeyValuePartialInformation,
                                      buf_ptr, total_size, &total_size );
        }

        if (!status)
        {
            if (is_string(info->Type))
            {
                DWORD len = WideCharToMultiByte( CP_ACP, 0, (WCHAR *)(buf_ptr + info_size),
                                                 (total_size - info_size) /sizeof(WCHAR),
                                                 NULL, 0, NULL, NULL );
                if (data && len)
                {
                    if (len > *count) status = STATUS_BUFFER_OVERFLOW;
                    else
                    {
                        WideCharToMultiByte( CP_ACP, 0, (WCHAR *)(buf_ptr + info_size),
                                             (total_size - info_size) /sizeof(WCHAR),
                                             data, len, NULL, NULL );
                        /* if the type is REG_SZ and data is not 0-terminated
                         * and there is enough space in the buffer NT appends a \0 */
                        if (len < *count && data[len-1]) data[len] = 0;
                    }
                }
                total_size = len + info_size;
            }
            else if (data)
            {
                if (total_size - info_size > *count) status = STATUS_BUFFER_OVERFLOW;
                else memcpy( data, buf_ptr + info_size, total_size - info_size );
            }
        }
        else if (status != STATUS_BUFFER_OVERFLOW) goto done;
    }

    if (type) *type = info->Type;
    if (count) *count = total_size - info_size;

 done:
    if (buf_ptr != buffer) HeapFree( GetProcessHeap(), 0, buf_ptr );
    RtlFreeUnicodeString( &nameW );
    return RtlNtStatusToDosError(status);
}


/******************************************************************************
 *           VMM_RegQueryValueA
 */
static DWORD VMM_RegQueryValueA( HKEY hkey, LPCSTR name, LPSTR data, LPLONG count )
{
    DWORD ret;
    HKEY subkey = hkey;

    if (name && name[0])
    {
        if ((ret = VMM_RegOpenKeyExA( hkey, name, 0, KEY_ALL_ACCESS, &subkey )) != ERROR_SUCCESS)
            return ret;
    }
    ret = VMM_RegQueryValueExA( subkey, NULL, NULL, NULL, (LPBYTE)data, count );
    if (subkey != hkey) NtClose( subkey );
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
 *           VMM_RegEnumValueA
 */
static DWORD VMM_RegEnumValueA( HKEY hkey, DWORD index, LPSTR value, LPDWORD val_count,
                                LPDWORD reserved, LPDWORD type, LPBYTE data, LPDWORD count )
{
    NTSTATUS status;
    DWORD total_size;
    char buffer[256], *buf_ptr = buffer;
    KEY_VALUE_FULL_INFORMATION *info = (KEY_VALUE_FULL_INFORMATION *)buffer;
    static const int info_size = offsetof( KEY_VALUE_FULL_INFORMATION, Name );

    TRACE("(%p,%ld,%p,%p,%p,%p,%p,%p)\n",
          hkey, index, value, val_count, reserved, type, data, count );

    /* NT only checks count, not val_count */
    if ((data && !count) || reserved) return ERROR_INVALID_PARAMETER;
    if (!(hkey = get_special_root_hkey( hkey ))) return ERROR_INVALID_HANDLE;

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
            if (buf_ptr != buffer) HeapFree( GetProcessHeap(), 0, buf_ptr );
            if (!(buf_ptr = HeapAlloc( GetProcessHeap(), 0, total_size )))
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
                    RtlUnicodeToMultiByteN( data, len, NULL, (WCHAR *)(buf_ptr + info->DataOffset),
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
    if (buf_ptr != buffer) HeapFree( GetProcessHeap(), 0, buf_ptr );
    return RtlNtStatusToDosError(status);
}


/******************************************************************************
 *           VMM_RegEnumKeyA
 */
static DWORD VMM_RegEnumKeyA( HKEY hkey, DWORD index, LPSTR name, DWORD name_len )
{
    NTSTATUS status;
    char buffer[256], *buf_ptr = buffer;
    KEY_NODE_INFORMATION *info = (KEY_NODE_INFORMATION *)buffer;
    DWORD total_size;

    if (!(hkey = get_special_root_hkey( hkey ))) return ERROR_INVALID_HANDLE;

    status = NtEnumerateKey( hkey, index, KeyNodeInformation,
                             buffer, sizeof(buffer), &total_size );

    while (status == STATUS_BUFFER_OVERFLOW)
    {
        /* retry with a dynamically allocated buffer */
        if (buf_ptr != buffer) HeapFree( GetProcessHeap(), 0, buf_ptr );
        if (!(buf_ptr = HeapAlloc( GetProcessHeap(), 0, total_size )))
            return ERROR_NOT_ENOUGH_MEMORY;
        info = (KEY_NODE_INFORMATION *)buf_ptr;
        status = NtEnumerateKey( hkey, index, KeyNodeInformation,
                                 buf_ptr, total_size, &total_size );
    }

    if (!status)
    {
        DWORD len;

        RtlUnicodeToMultiByteSize( &len, info->Name, info->NameLength );
        if (len >= name_len) status = STATUS_BUFFER_OVERFLOW;
        else
        {
            RtlUnicodeToMultiByteN( name, len, NULL, info->Name, info->NameLength );
            name[len] = 0;
        }
    }

    if (buf_ptr != buffer) HeapFree( GetProcessHeap(), 0, buf_ptr );
    return RtlNtStatusToDosError( status );
}


/******************************************************************************
 *           VMM_RegQueryInfoKeyA
 *
 * NOTE: This VxDCall takes only a subset of the parameters that the
 * corresponding Win32 API call does. The implementation in Win95
 * ADVAPI32 sets all output parameters not mentioned here to zero.
 */
static DWORD VMM_RegQueryInfoKeyA( HKEY hkey, LPDWORD subkeys, LPDWORD max_subkey,
                                   LPDWORD values, LPDWORD max_value, LPDWORD max_data )
{
    NTSTATUS status;
    KEY_FULL_INFORMATION info;
    DWORD total_size;

    if (!(hkey = get_special_root_hkey( hkey ))) return ERROR_INVALID_HANDLE;

    status = NtQueryKey( hkey, KeyFullInformation, &info, sizeof(info), &total_size );
    if (status && status != STATUS_BUFFER_OVERFLOW) return RtlNtStatusToDosError( status );

    if (subkeys) *subkeys = info.SubKeys;
    if (max_subkey) *max_subkey = info.MaxNameLen;
    if (values) *values = info.Values;
    if (max_value) *max_value = info.MaxValueNameLen;
    if (max_data) *max_data = info.MaxValueDataLen;
    return ERROR_SUCCESS;
}


/***********************************************************************
 *           VxDCall_VMM
 */
static DWORD VxDCall_VMM( DWORD service, CONTEXT86 *context )
{
    switch ( LOWORD(service) )
    {
    case 0x0011:  /* RegOpenKey */
    {
        HKEY    hkey       = (HKEY)  stack32_pop( context );
        LPCSTR  lpszSubKey = (LPCSTR)stack32_pop( context );
        PHKEY   retkey     = (PHKEY)stack32_pop( context );
        return VMM_RegOpenKeyExA( hkey, lpszSubKey, 0, KEY_ALL_ACCESS, retkey );
    }

    case 0x0012:  /* RegCreateKey */
    {
        HKEY    hkey       = (HKEY)  stack32_pop( context );
        LPCSTR  lpszSubKey = (LPCSTR)stack32_pop( context );
        PHKEY   retkey     = (PHKEY)stack32_pop( context );
        return VMM_RegCreateKeyA( hkey, lpszSubKey, retkey );
    }

    case 0x0013:  /* RegCloseKey */
    {
        HKEY    hkey       = (HKEY)stack32_pop( context );
        return VMM_RegCloseKey( hkey );
    }

    case 0x0014:  /* RegDeleteKey */
    {
        HKEY    hkey       = (HKEY)  stack32_pop( context );
        LPCSTR  lpszSubKey = (LPCSTR)stack32_pop( context );
        return VMM_RegDeleteKeyA( hkey, lpszSubKey );
    }

    case 0x0015:  /* RegSetValue */
    {
        HKEY    hkey       = (HKEY)  stack32_pop( context );
        LPCSTR  lpszSubKey = (LPCSTR)stack32_pop( context );
        DWORD   dwType     = (DWORD) stack32_pop( context );
        LPCSTR  lpszData   = (LPCSTR)stack32_pop( context );
        DWORD   cbData     = (DWORD) stack32_pop( context );
        return VMM_RegSetValueA( hkey, lpszSubKey, dwType, lpszData, cbData );
    }

    case 0x0016:  /* RegDeleteValue */
    {
        HKEY    hkey       = (HKEY) stack32_pop( context );
        LPSTR   lpszValue  = (LPSTR)stack32_pop( context );
        return VMM_RegDeleteValueA( hkey, lpszValue );
    }

    case 0x0017:  /* RegQueryValue */
    {
        HKEY    hkey       = (HKEY)   stack32_pop( context );
        LPSTR   lpszSubKey = (LPSTR)  stack32_pop( context );
        LPSTR   lpszData   = (LPSTR)  stack32_pop( context );
        LPDWORD lpcbData   = (LPDWORD)stack32_pop( context );
        return VMM_RegQueryValueA( hkey, lpszSubKey, lpszData, lpcbData );
    }

    case 0x0018:  /* RegEnumKey */
    {
        HKEY    hkey       = (HKEY) stack32_pop( context );
        DWORD   iSubkey    = (DWORD)stack32_pop( context );
        LPSTR   lpszName   = (LPSTR)stack32_pop( context );
        DWORD   lpcchName  = (DWORD)stack32_pop( context );
        return VMM_RegEnumKeyA( hkey, iSubkey, lpszName, lpcchName );
    }

    case 0x0019:  /* RegEnumValue */
    {
        HKEY    hkey       = (HKEY)   stack32_pop( context );
        DWORD   iValue     = (DWORD)  stack32_pop( context );
        LPSTR   lpszValue  = (LPSTR)  stack32_pop( context );
        LPDWORD lpcchValue = (LPDWORD)stack32_pop( context );
        LPDWORD lpReserved = (LPDWORD)stack32_pop( context );
        LPDWORD lpdwType   = (LPDWORD)stack32_pop( context );
        LPBYTE  lpbData    = (LPBYTE) stack32_pop( context );
        LPDWORD lpcbData   = (LPDWORD)stack32_pop( context );
        return VMM_RegEnumValueA( hkey, iValue, lpszValue, lpcchValue,
                                  lpReserved, lpdwType, lpbData, lpcbData );
    }

    case 0x001A:  /* RegQueryValueEx */
    {
        HKEY    hkey       = (HKEY)   stack32_pop( context );
        LPSTR   lpszValue  = (LPSTR)  stack32_pop( context );
        LPDWORD lpReserved = (LPDWORD)stack32_pop( context );
        LPDWORD lpdwType   = (LPDWORD)stack32_pop( context );
        LPBYTE  lpbData    = (LPBYTE) stack32_pop( context );
        LPDWORD lpcbData   = (LPDWORD)stack32_pop( context );
        return VMM_RegQueryValueExA( hkey, lpszValue, lpReserved,
                                     lpdwType, lpbData, lpcbData );
    }

    case 0x001B:  /* RegSetValueEx */
    {
        HKEY    hkey       = (HKEY)  stack32_pop( context );
        LPSTR   lpszValue  = (LPSTR) stack32_pop( context );
        DWORD   dwReserved = (DWORD) stack32_pop( context );
        DWORD   dwType     = (DWORD) stack32_pop( context );
        LPBYTE  lpbData    = (LPBYTE)stack32_pop( context );
        DWORD   cbData     = (DWORD) stack32_pop( context );
        return VMM_RegSetValueExA( hkey, lpszValue, dwReserved,
                                   dwType, lpbData, cbData );
    }

    case 0x001C:  /* RegFlushKey */
    {
        HKEY hkey = (HKEY)stack32_pop( context );
        FIXME( "RegFlushKey(%p): stub\n", hkey );
        return ERROR_SUCCESS;
    }

    case 0x001D:  /* RegQueryInfoKey */
    {
        /* NOTE: This VxDCall takes only a subset of the parameters that the
                 corresponding Win32 API call does. The implementation in Win95
                 ADVAPI32 sets all output parameters not mentioned here to zero. */

        HKEY    hkey              = (HKEY)   stack32_pop( context );
        LPDWORD lpcSubKeys        = (LPDWORD)stack32_pop( context );
        LPDWORD lpcchMaxSubKey    = (LPDWORD)stack32_pop( context );
        LPDWORD lpcValues         = (LPDWORD)stack32_pop( context );
        LPDWORD lpcchMaxValueName = (LPDWORD)stack32_pop( context );
        LPDWORD lpcchMaxValueData = (LPDWORD)stack32_pop( context );
        return VMM_RegQueryInfoKeyA( hkey, lpcSubKeys, lpcchMaxSubKey,
                                     lpcValues, lpcchMaxValueName, lpcchMaxValueData );
    }

    case 0x0021:  /* RegLoadKey */
    {
        HKEY    hkey       = (HKEY)  stack32_pop( context );
        LPCSTR  lpszSubKey = (LPCSTR)stack32_pop( context );
        LPCSTR  lpszFile   = (LPCSTR)stack32_pop( context );
        FIXME("RegLoadKey(%p,%s,%s): stub\n",hkey, debugstr_a(lpszSubKey), debugstr_a(lpszFile));
        return ERROR_SUCCESS;
    }

    case 0x0022:  /* RegUnLoadKey */
    {
        HKEY    hkey       = (HKEY)  stack32_pop( context );
        LPCSTR  lpszSubKey = (LPCSTR)stack32_pop( context );
        FIXME("RegUnLoadKey(%p,%s): stub\n",hkey, debugstr_a(lpszSubKey));
        return ERROR_SUCCESS;
    }

    case 0x0023:  /* RegSaveKey */
    {
        HKEY    hkey       = (HKEY)  stack32_pop( context );
        LPCSTR  lpszFile   = (LPCSTR)stack32_pop( context );
        LPSECURITY_ATTRIBUTES sa = (LPSECURITY_ATTRIBUTES)stack32_pop( context );
        FIXME("RegSaveKey(%p,%s,%p): stub\n",hkey, debugstr_a(lpszFile),sa);
        return ERROR_SUCCESS;
    }

#if 0 /* Functions are not yet implemented in misc/registry.c */
    case 0x0024:  /* RegRemapPreDefKey */
    case 0x0026:  /* RegQueryMultipleValues */
#endif

    case 0x0027:  /* RegReplaceKey */
    {
        HKEY    hkey       = (HKEY)  stack32_pop( context );
        LPCSTR  lpszSubKey = (LPCSTR)stack32_pop( context );
        LPCSTR  lpszNewFile= (LPCSTR)stack32_pop( context );
        LPCSTR  lpszOldFile= (LPCSTR)stack32_pop( context );
        FIXME("RegReplaceKey(%p,%s,%s,%s): stub\n", hkey, debugstr_a(lpszSubKey),
              debugstr_a(lpszNewFile),debugstr_a(lpszOldFile));
        return ERROR_SUCCESS;
    }

    case 0x0000: /* PageReserve */
      {
	LPVOID address;
	LPVOID ret;
	DWORD psize = getpagesize();
	ULONG page   = (ULONG) stack32_pop( context );
	ULONG npages = (ULONG) stack32_pop( context );
	ULONG flags  = (ULONG) stack32_pop( context );

	TRACE("PageReserve: page: %08lx, npages: %08lx, flags: %08lx partial stub!\n",
	      page, npages, flags );

	if ( page == PR_SYSTEM ) {
	  ERR("Can't reserve ring 1 memory\n");
	  return -1;
	}
	/* FIXME: This has to be handled separately for the separate
	   address-spaces we now have */
	if ( page == PR_PRIVATE || page == PR_SHARED ) page = 0;
	/* FIXME: Handle flags in some way */
	address = (LPVOID )(page * psize);
	ret = VirtualAlloc ( address, ( npages * psize ), MEM_RESERVE, 0 );
	TRACE("PageReserve: returning: %08lx\n", (DWORD )ret );
	if ( ret == NULL )
	  return -1;
	else
	  return (DWORD )ret;
      }

    case 0x0001: /* PageCommit */
      {
	LPVOID address;
	LPVOID ret;
	DWORD virt_perm;
	DWORD psize = getpagesize();
	ULONG page   = (ULONG) stack32_pop( context );
	ULONG npages = (ULONG) stack32_pop( context );
	ULONG hpd  = (ULONG) stack32_pop( context );
	ULONG pagerdata   = (ULONG) stack32_pop( context );
	ULONG flags  = (ULONG) stack32_pop( context );

	TRACE("PageCommit: page: %08lx, npages: %08lx, hpd: %08lx pagerdata: "
	      "%08lx, flags: %08lx partial stub\n",
	      page, npages, hpd, pagerdata, flags );

	if ( flags & PC_USER )
	  if ( flags & PC_WRITEABLE )
	    virt_perm = PAGE_EXECUTE_READWRITE;
	  else
	    virt_perm = PAGE_EXECUTE_READ;
	else
	  virt_perm = PAGE_NOACCESS;

	address = (LPVOID )(page * psize);
	ret = VirtualAlloc ( address, ( npages * psize ), MEM_COMMIT, virt_perm );
	TRACE("PageCommit: Returning: %08lx\n", (DWORD )ret );
	return (DWORD )ret;

      }
    case 0x0002: /* PageDecommit */
      {
	LPVOID address;
	BOOL ret;
	DWORD psize = getpagesize();
	ULONG page = (ULONG) stack32_pop( context );
	ULONG npages = (ULONG) stack32_pop( context );
	ULONG flags = (ULONG) stack32_pop( context );

	TRACE("PageDecommit: page: %08lx, npages: %08lx, flags: %08lx partial stub\n",
	      page, npages, flags );
	address = (LPVOID )( page * psize );
	ret = VirtualFree ( address, ( npages * psize ), MEM_DECOMMIT );
	TRACE("PageDecommit: Returning: %s\n", ret ? "TRUE" : "FALSE" );
	return ret;
      }
    case 0x000d: /* PageModifyPermissions */
      {
	DWORD pg_old_perm;
	DWORD pg_new_perm;
	DWORD virt_old_perm;
	DWORD virt_new_perm;
	MEMORY_BASIC_INFORMATION mbi;
	LPVOID address;
	DWORD psize = getpagesize();
	ULONG page = stack32_pop ( context );
	ULONG npages = stack32_pop ( context );
	ULONG permand = stack32_pop ( context );
	ULONG permor = stack32_pop ( context );

	TRACE("PageModifyPermissions %08lx %08lx %08lx %08lx partial stub\n",
	      page, npages, permand, permor );
	address = (LPVOID )( page * psize );

	VirtualQuery ( address, &mbi, sizeof ( MEMORY_BASIC_INFORMATION ));
	virt_old_perm = mbi.Protect;

	switch ( virt_old_perm & mbi.Protect ) {
	case PAGE_READONLY:
	case PAGE_EXECUTE:
	case PAGE_EXECUTE_READ:
	  pg_old_perm = PC_USER;
	  break;
	case PAGE_READWRITE:
	case PAGE_WRITECOPY:
	case PAGE_EXECUTE_READWRITE:
	case PAGE_EXECUTE_WRITECOPY:
	  pg_old_perm = PC_USER | PC_WRITEABLE;
	  break;
	case PAGE_NOACCESS:
	default:
	  pg_old_perm = 0;
	  break;
	}
	pg_new_perm = pg_old_perm;
	pg_new_perm &= permand & ~PC_STATIC;
	pg_new_perm |= permor  & ~PC_STATIC;

	virt_new_perm = ( virt_old_perm )  & ~0xff;
	if ( pg_new_perm & PC_USER )
	{
	  if ( pg_new_perm & PC_WRITEABLE )
	    virt_new_perm |= PAGE_EXECUTE_READWRITE;
	  else
	    virt_new_perm |= PAGE_EXECUTE_READ;
	}

	if ( ! VirtualProtect ( address, ( npages * psize ), virt_new_perm, &virt_old_perm ) ) {
	  ERR("Can't change page permissions for %08lx\n", (DWORD )address );
	  return 0xffffffff;
	}
	TRACE("Returning: %08lx\n", pg_old_perm );
	return pg_old_perm;
      }
    case 0x000a: /* PageFree */
      {
	BOOL ret;
	LPVOID hmem = (LPVOID) stack32_pop( context );
	DWORD flags = (DWORD ) stack32_pop( context );

	TRACE("PageFree: hmem: %08lx, flags: %08lx partial stub\n",
	      (DWORD )hmem, flags );

	ret = VirtualFree ( hmem, 0, MEM_RELEASE );
	context->Eax = ret;
	TRACE("Returning: %d\n", ret );

	return 0;
      }
    case 0x001e: /* GetDemandPageInfo */
      {
	 DWORD dinfo = (DWORD)stack32_pop( context );
         DWORD flags = (DWORD)stack32_pop( context );

	 /* GetDemandPageInfo is supposed to fill out the struct at
          * "dinfo" with various low-level memory management information.
          * Apps are certainly not supposed to call this, although it's
          * demoed and documented by Pietrek on pages 441-443 of "Windows
          * 95 System Programming Secrets" if any program needs a real
          * implementation of this.
	  */

	 FIXME("GetDemandPageInfo(%08lx %08lx): stub!\n", dinfo, flags);

	 return 0;
      }
    default:
        if (LOWORD(service) < N_VMM_SERVICE)
            FIXME( "Unimplemented service %s (%08lx)\n",
                          VMM_Service_Name[LOWORD(service)], service);
        else
            FIXME( "Unknown service %08lx\n", service);
        break;
    }

    return 0xffffffff;  /* FIXME */
}


/********************************************************************************
 *      VxDCall_VWin32
 *
 *  Service numbers taken from page 448 of Pietrek's "Windows 95 System
 *  Programming Secrets".  Parameters from experimentation on real Win98.
 *
 */

static DWORD VxDCall_VWin32( DWORD service, CONTEXT86 *context )
{
    switch ( LOWORD(service) )
    {
    case 0x0000: /* GetVersion */
    {
        DWORD vers = GetVersion();
        return (LOBYTE(vers) << 8) | HIBYTE(vers);
    }
    break;

    case 0x0020: /* Get VMCPD Version */
    {
	DWORD parm = (DWORD) stack32_pop(context);

	FIXME("Get VMCPD Version(%08lx): partial stub!\n", parm);

	/* FIXME: This is what Win98 returns, it may
         *        not be correct in all situations.
         *        It makes Bleem! happy though.
         */

	return 0x0405;
    }

    case 0x0029: /* Int31/DPMI dispatch */
    {
	DWORD callnum = (DWORD) stack32_pop(context);
        DWORD parm    = (DWORD) stack32_pop(context);

        TRACE("Int31/DPMI dispatch(%08lx)\n", callnum);

        context->Eax = callnum;
        context->Ecx = parm;
        INSTR_CallBuiltinHandler( context, 0x31 );

	return LOWORD(context->Eax);
    }
    break;

    case 0x002a: /* Int41 dispatch - parm = int41 service number */
    {
	DWORD callnum = (DWORD) stack32_pop(context);

	return callnum; /* FIXME: should really call INT_Int41Handler() */
    }
    break;

    default:
      FIXME("Unknown VWin32 service %08lx\n", service);
      break;
    }

    return 0xffffffff;
}


/***********************************************************************
 *		VxDCall0 (KERNEL32.1)
 *		VxDCall1 (KERNEL32.2)
 *		VxDCall2 (KERNEL32.3)
 *		VxDCall3 (KERNEL32.4)
 *		VxDCall4 (KERNEL32.5)
 *		VxDCall5 (KERNEL32.6)
 *		VxDCall6 (KERNEL32.7)
 *		VxDCall7 (KERNEL32.8)
 *		VxDCall8 (KERNEL32.9)
 */
void VxDCall( DWORD service, CONTEXT86 *context )
{
    DWORD ret;

    TRACE( "(%08lx, ...)\n", service);

    switch(HIWORD(service))
    {
    case 0x0001:  /* VMM */
        ret = VxDCall_VMM( service, context );
        break;
    case 0x002a:  /* VWIN32 */
        ret = VxDCall_VWin32( service, context );
        break;
    default:
        FIXME( "Unknown/unimplemented VxD (%08lx)\n", service);
        ret = 0xffffffff; /* FIXME */
        break;
    }
    context->Eax = ret;
}


/***********************************************************************
 *		OpenVxDHandle (KERNEL32.@)
 *
 *	This function is supposed to return the corresponding Ring 0
 *	("kernel") handle for a Ring 3 handle in Win9x.
 *	Evidently, Wine will have problems with this. But we try anyway,
 *	maybe it helps...
 */
HANDLE WINAPI OpenVxDHandle(HANDLE hHandleRing3)
{
    FIXME( "(%p), stub! (returning Ring 3 handle instead of Ring 0)\n", hHandleRing3);
    return hHandleRing3;
}
