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
 * This file is concerned about handle management and interaction with the Wine server.
 * Registry file I/O is in misc/registry.c.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "winbase.h"
#include "winreg.h"
#include "winerror.h"
#include "wine/winbase16.h"
#include "wine/unicode.h"
#include "heap.h"
#include "server.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(reg);


/* check if value type needs string conversion (Ansi<->Unicode) */
static inline int is_string( DWORD type )
{
    return (type == REG_SZ) || (type == REG_EXPAND_SZ) || (type == REG_MULTI_SZ);
}

/* do a server call without setting the last error code */
static inline int reg_server_call( enum request req )
{
    unsigned int res = server_call_noerr( req );
    if (res) res = RtlNtStatusToDosError(res);
    return res;
}

/******************************************************************************
 *           RegCreateKeyExA   [ADVAPI32.130]
 */
DWORD WINAPI RegCreateKeyExA( HKEY hkey, LPCSTR name, DWORD reserved, LPSTR class,
                              DWORD options, REGSAM access, SECURITY_ATTRIBUTES *sa, 
                              LPHKEY retkey, LPDWORD dispos )
{
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW, classW;
    ANSI_STRING nameA, classA;
    NTSTATUS status;

    if (reserved) return ERROR_INVALID_PARAMETER;
    if (!(access & KEY_ALL_ACCESS) || (access & ~KEY_ALL_ACCESS)) return ERROR_ACCESS_DENIED;

    attr.Length = sizeof(attr);
    attr.RootDirectory = hkey;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    RtlInitAnsiString( &nameA, name );
    RtlInitAnsiString( &classA, class );

    /* FIXME: should use Unicode buffer in TEB */
    if (!(status = RtlAnsiStringToUnicodeString( &nameW, &nameA, TRUE )))
    {
        if (!(status = RtlAnsiStringToUnicodeString( &classW, &classA, TRUE )))
        {
            status = NtCreateKey( retkey, access, &attr, 0, &classW, options, dispos );
            RtlFreeUnicodeString( &classW );
        }
        RtlFreeUnicodeString( &nameW );
    }
    return RtlNtStatusToDosError( status );
}


/******************************************************************************
 *           RegCreateKeyA   [ADVAPI32.129]
 */
DWORD WINAPI RegCreateKeyA( HKEY hkey, LPCSTR name, LPHKEY retkey )
{
    return RegCreateKeyExA( hkey, name, 0, NULL, REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS, NULL, retkey, NULL );
}



/******************************************************************************
 *           RegOpenKeyExA   [ADVAPI32.149]
 */
DWORD WINAPI RegOpenKeyExA( HKEY hkey, LPCSTR name, DWORD reserved, REGSAM access, LPHKEY retkey )
{
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    STRING nameA;
    NTSTATUS status;

    attr.Length = sizeof(attr);
    attr.RootDirectory = hkey;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    RtlInitAnsiString( &nameA, name );
    /* FIXME: should use Unicode buffer in TEB */
    if (!(status = RtlAnsiStringToUnicodeString( &nameW, &nameA, TRUE )))
    {
        status = NtOpenKey( retkey, access, &attr );
        RtlFreeUnicodeString( &nameW );
    }
    return RtlNtStatusToDosError( status );
}


/******************************************************************************
 *           RegOpenKeyA   [ADVAPI32.148]
 */
DWORD WINAPI RegOpenKeyA( HKEY hkey, LPCSTR name, LPHKEY retkey )
{
    return RegOpenKeyExA( hkey, name, 0, KEY_ALL_ACCESS, retkey );
}


/******************************************************************************
 *           RegEnumKeyExA   [ADVAPI32.138]
 */
DWORD WINAPI RegEnumKeyExA( HKEY hkey, DWORD index, LPSTR name, LPDWORD name_len,
                            LPDWORD reserved, LPSTR class, LPDWORD class_len, FILETIME *ft )
{
    NTSTATUS status;
    char buffer[256], *buf_ptr = buffer;
    KEY_NODE_INFORMATION *info = (KEY_NODE_INFORMATION *)buffer;
    DWORD total_size;

    TRACE( "(0x%x,%ld,%p,%p(%ld),%p,%p,%p,%p)\n", hkey, index, name, name_len,
           name_len ? *name_len : -1, reserved, class, class_len, ft );

    if (reserved) return ERROR_INVALID_PARAMETER;

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
        DWORD len = WideCharToMultiByte( CP_ACP, 0, info->Name, info->NameLength/sizeof(WCHAR),
                                         NULL, 0, NULL, NULL );
        DWORD cls_len = WideCharToMultiByte( CP_ACP, 0, (WCHAR *)(buf_ptr + info->ClassOffset),
                                             info->ClassLength / sizeof(WCHAR),
                                             NULL, 0, NULL, NULL );

        if (ft) *ft = info->LastWriteTime;

        if (len >= *name_len || (class_len && (cls_len >= *class_len)))
            status = STATUS_BUFFER_OVERFLOW;
        else
        {
            *name_len = len;
            WideCharToMultiByte( CP_ACP, 0, info->Name, info->NameLength/sizeof(WCHAR),
                                 name, len, NULL, NULL );
            name[len] = 0;
            if (class_len)
            {
                *class_len = cls_len;
                if (class)
                {
                    WideCharToMultiByte( CP_ACP, 0, (WCHAR *)(buf_ptr + info->ClassOffset),
                                         info->ClassLength / sizeof(WCHAR),
                                         class, cls_len, NULL, NULL );
                    class[cls_len] = 0;
                }
            }
        }
    }

    if (buf_ptr != buffer) HeapFree( GetProcessHeap(), 0, buf_ptr );
    return RtlNtStatusToDosError( status );
}


/******************************************************************************
 *           RegEnumKeyA   [ADVAPI32.137]
 */
DWORD WINAPI RegEnumKeyA( HKEY hkey, DWORD index, LPSTR name, DWORD name_len )
{
    return RegEnumKeyExA( hkey, index, name, &name_len, NULL, NULL, NULL, NULL );
}


/******************************************************************************
 *           RegQueryInfoKeyA   [ADVAPI32.152]
 */
DWORD WINAPI RegQueryInfoKeyA( HKEY hkey, LPSTR class, LPDWORD class_len, LPDWORD reserved,
                               LPDWORD subkeys, LPDWORD max_subkey, LPDWORD max_class,
                               LPDWORD values, LPDWORD max_value, LPDWORD max_data,
                               LPDWORD security, FILETIME *modif )
{
    NTSTATUS status;
    char buffer[256], *buf_ptr = buffer;
    KEY_FULL_INFORMATION *info = (KEY_FULL_INFORMATION *)buffer;
    DWORD total_size;

    TRACE( "(0x%x,%p,%ld,%p,%p,%p,%p,%p,%p,%p,%p)\n", hkey, class, class_len ? *class_len : 0,
           reserved, subkeys, max_subkey, values, max_value, max_data, security, modif );

    if (class && !class_len && !(GetVersion() & 0x80000000 /*NT*/))
        return ERROR_INVALID_PARAMETER;

    status = NtQueryKey( hkey, KeyFullInformation, buffer, sizeof(buffer), &total_size );

    if (class || class_len)
    {
        /* retry with a dynamically allocated buffer */
        while (status == STATUS_BUFFER_OVERFLOW)
        {
            if (buf_ptr != buffer) HeapFree( GetProcessHeap(), 0, buf_ptr );
            if (!(buf_ptr = HeapAlloc( GetProcessHeap(), 0, total_size )))
                return ERROR_NOT_ENOUGH_MEMORY;
            info = (KEY_FULL_INFORMATION *)buf_ptr;
            status = NtQueryKey( hkey, KeyFullInformation, buf_ptr, total_size, &total_size );
        }

        if (!status)
        {
            DWORD len = WideCharToMultiByte( CP_ACP, 0,
                                             (WCHAR *)(buf_ptr + info->ClassOffset),
                                             info->ClassLength/sizeof(WCHAR),
                                             NULL, 0, NULL, NULL );
            if (class_len)
            {
                if (len + 1 > *class_len) status = STATUS_BUFFER_OVERFLOW;
                *class_len = len;
            }
            if (class && !status)
            {
                WideCharToMultiByte( CP_ACP, 0,
                                     (WCHAR *)(buf_ptr + info->ClassOffset),
                                     info->ClassLength/sizeof(WCHAR),
                                     class, len, NULL, NULL );
                class[len] = 0;
            }
        }
    }

    if (!status || status == STATUS_BUFFER_OVERFLOW)
    {
        if (subkeys) *subkeys = info->SubKeys;
        if (max_subkey) *max_subkey = info->MaxNameLen;
        if (max_class) *max_class = info->MaxClassLen;
        if (values) *values = info->Values;
        if (max_value) *max_value = info->MaxValueNameLen;
        if (max_data) *max_data = info->MaxValueDataLen;
        if (modif) *modif = info->LastWriteTime;
    }

    if (buf_ptr != buffer) HeapFree( GetProcessHeap(), 0, buf_ptr );
    return RtlNtStatusToDosError( status );
}


/******************************************************************************
 *           RegCloseKey   [ADVAPI32.126]
 *
 * Releases the handle of the specified key
 *
 * PARAMS
 *    hkey [I] Handle of key to close
 *
 * RETURNS
 *    Success: ERROR_SUCCESS
 *    Failure: Error code
 */
DWORD WINAPI RegCloseKey( HKEY hkey )
{
    if (!hkey || hkey >= 0x80000000) return ERROR_SUCCESS;
    return RtlNtStatusToDosError( NtClose( hkey ) );
}


/******************************************************************************
 *           RegDeleteKeyA   [ADVAPI32.133]
 */
DWORD WINAPI RegDeleteKeyA( HKEY hkey, LPCSTR name )
{
    DWORD ret;
    HKEY tmp;

    if (!name || !*name) return NtDeleteKey( hkey );
    if (!(ret = RegOpenKeyExA( hkey, name, 0, 0, &tmp )))
    {
        ret = RtlNtStatusToDosError( NtDeleteKey( tmp ) );
        RegCloseKey( tmp );
    }
    return ret;
}



/******************************************************************************
 *           RegSetValueExA   [ADVAPI32.169]
 */
DWORD WINAPI RegSetValueExA( HKEY hkey, LPCSTR name, DWORD reserved, DWORD type,
                             CONST BYTE *data, DWORD count )
{
    UNICODE_STRING nameW;
    ANSI_STRING nameA;
    WCHAR *dataW = NULL;
    NTSTATUS status;

    if (count && is_string(type))
    {
        /* if user forgot to count terminating null, add it (yes NT does this) */
        if (data[count-1] && !data[count]) count++;
    }

    if (is_string( type )) /* need to convert to Unicode */
    {
        DWORD lenW = MultiByteToWideChar( CP_ACP, 0, data, count, NULL, 0 );
        if (!(dataW = HeapAlloc( GetProcessHeap(), 0, lenW*sizeof(WCHAR) )))
            return ERROR_OUTOFMEMORY;
        MultiByteToWideChar( CP_ACP, 0, data, count, dataW, lenW );
        count = lenW * sizeof(WCHAR);
        data = (BYTE *)dataW;
    }

    RtlInitAnsiString( &nameA, name );
    /* FIXME: should use Unicode buffer in TEB */
    if (!(status = RtlAnsiStringToUnicodeString( &nameW, &nameA, TRUE )))
    {
        status = NtSetValueKey( hkey, &nameW, 0, type, data, count );
        RtlFreeUnicodeString( &nameW );
    }
    if (dataW) HeapFree( GetProcessHeap(), 0, dataW );
    return RtlNtStatusToDosError( status );
}


/******************************************************************************
 *           RegSetValueA   [ADVAPI32.168]
 */
DWORD WINAPI RegSetValueA( HKEY hkey, LPCSTR name, DWORD type, LPCSTR data, DWORD count )
{
    HKEY subkey = hkey;
    DWORD ret;

    TRACE("(0x%x,%s,%ld,%s,%ld)\n", hkey, debugstr_a(name), type, debugstr_a(data), count );

    if (type != REG_SZ) return ERROR_INVALID_PARAMETER;

    if (name && name[0])  /* need to create the subkey */
    {
        if ((ret = RegCreateKeyA( hkey, name, &subkey )) != ERROR_SUCCESS) return ret;
    }
    ret = RegSetValueExA( subkey, NULL, 0, REG_SZ, (LPBYTE)data, strlen(data)+1 );
    if (subkey != hkey) RegCloseKey( subkey );
    return ret;
}



/******************************************************************************
 *           RegQueryValueExA   [ADVAPI32.157]
 *
 * NOTES:
 * the documentation is wrong: if the buffer is too small it remains untouched 
 */
DWORD WINAPI RegQueryValueExA( HKEY hkey, LPCSTR name, LPDWORD reserved, LPDWORD type,
                               LPBYTE data, LPDWORD count )
{
    NTSTATUS status;
    ANSI_STRING nameA;
    UNICODE_STRING nameW;
    DWORD total_size;
    char buffer[256], *buf_ptr = buffer;
    KEY_VALUE_PARTIAL_INFORMATION *info = (KEY_VALUE_PARTIAL_INFORMATION *)buffer;
    static const int info_size = sizeof(*info) - sizeof(info->Data);

    TRACE("(0x%x,%s,%p,%p,%p,%p=%ld)\n",
          hkey, debugstr_a(name), reserved, type, data, count, count ? *count : 0 );

    if ((data && !count) || reserved) return ERROR_INVALID_PARAMETER;

    RtlInitAnsiString( &nameA, name );
    /* FIXME: should use Unicode buffer in TEB */
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
 *           RegQueryValueA   [ADVAPI32.156]
 */
DWORD WINAPI RegQueryValueA( HKEY hkey, LPCSTR name, LPSTR data, LPLONG count )
{
    DWORD ret;
    HKEY subkey = hkey;

    TRACE("(%x,%s,%p,%ld)\n", hkey, debugstr_a(name), data, count ? *count : 0 );

    if (name && name[0])
    {
        if ((ret = RegOpenKeyA( hkey, name, &subkey )) != ERROR_SUCCESS) return ret;
    }
    ret = RegQueryValueExA( subkey, NULL, NULL, NULL, (LPBYTE)data, count );
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
 *           RegEnumValueA   [ADVAPI32.141]
 */
DWORD WINAPI RegEnumValueA( HKEY hkey, DWORD index, LPSTR value, LPDWORD val_count,
                            LPDWORD reserved, LPDWORD type, LPBYTE data, LPDWORD count )
{
    NTSTATUS status;
    DWORD total_size;
    char buffer[256], *buf_ptr = buffer;
    KEY_VALUE_FULL_INFORMATION *info = (KEY_VALUE_FULL_INFORMATION *)buffer;
    static const int info_size = sizeof(*info) - sizeof(info->Name);

    TRACE("(%x,%ld,%p,%p,%p,%p,%p,%p)\n",
          hkey, index, value, val_count, reserved, type, data, count );

    /* NT only checks count, not val_count */
    if ((data && !count) || reserved) return ERROR_INVALID_PARAMETER;

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

        if (value)
        {
            DWORD len = WideCharToMultiByte( CP_ACP, 0, info->Name, info->NameLength/sizeof(WCHAR),
                                             NULL, 0, NULL, NULL );
            if (len >= *val_count)
            {
                status = STATUS_BUFFER_OVERFLOW;
                goto done;
            }
            WideCharToMultiByte( CP_ACP, 0, info->Name, info->NameLength/sizeof(WCHAR),
                                 value, len, NULL, NULL );
            value[len] = 0;
            *val_count = len;
        }

        if (is_string(info->Type))
        {
            DWORD len = WideCharToMultiByte( CP_ACP, 0, (WCHAR *)(buf_ptr + info->DataOffset),
                                             (total_size - info->DataOffset) / sizeof(WCHAR),
                                             NULL, 0, NULL, NULL );
            if (data && len)
            {
                if (len > *count)
                {
                    status = STATUS_BUFFER_OVERFLOW;
                    goto done;
                }
                WideCharToMultiByte( CP_ACP, 0, (WCHAR *)(buf_ptr + info->DataOffset),
                                     (total_size - info->DataOffset) / sizeof(WCHAR),
                                     data, len, NULL, NULL );
                /* if the type is REG_SZ and data is not 0-terminated
                 * and there is enough space in the buffer NT appends a \0 */
                if (len < *count && data[len-1]) data[len] = 0;
            }
            info->DataLength = len;
        }
        else if (data)
        {
            if (total_size - info->DataOffset > *count) status = STATUS_BUFFER_OVERFLOW;
            else memcpy( data, buf_ptr + info->DataOffset, total_size - info->DataOffset );
        }
    }

    if (type) *type = info->Type;
    if (count) *count = info->DataLength;

 done:
    if (buf_ptr != buffer) HeapFree( GetProcessHeap(), 0, buf_ptr );
    return RtlNtStatusToDosError(status);
}



/******************************************************************************
 *           RegDeleteValueA   [ADVAPI32.135]
 */
DWORD WINAPI RegDeleteValueA( HKEY hkey, LPCSTR name )
{
    UNICODE_STRING nameW;
    STRING nameA;
    NTSTATUS status;

    RtlInitAnsiString( &nameA, name );
    /* FIXME: should use Unicode buffer in TEB */
    if (!(status = RtlAnsiStringToUnicodeString( &nameW, &nameA, TRUE )))
    {
        status = NtDeleteValueKey( hkey, &nameW );
        RtlFreeUnicodeString( &nameW );
    }
    return RtlNtStatusToDosError( status );
}


/******************************************************************************
 *           RegLoadKeyA   [ADVAPI32.184]
 */
LONG WINAPI RegLoadKeyA( HKEY hkey, LPCSTR subkey, LPCSTR filename )
{
    HANDLE file;
    DWORD ret, len, err = GetLastError();

    TRACE( "(%x,%s,%s)\n", hkey, debugstr_a(subkey), debugstr_a(filename) );

    if (!filename || !*filename) return ERROR_INVALID_PARAMETER;
    if (!subkey || !*subkey) return ERROR_INVALID_PARAMETER;

    len = MultiByteToWideChar( CP_ACP, 0, subkey, strlen(subkey), NULL, 0 ) * sizeof(WCHAR);
    if (len > MAX_PATH*sizeof(WCHAR)) return ERROR_INVALID_PARAMETER;

    if ((file = CreateFileA( filename, GENERIC_READ, 0, NULL, OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL, -1 )) == INVALID_HANDLE_VALUE)
    {
        ret = GetLastError();
        goto done;
    }

    SERVER_START_REQ
    {
        struct load_registry_request *req = server_alloc_req( sizeof(*req), len );
        req->hkey  = hkey;
        req->file  = file;
        MultiByteToWideChar( CP_ACP, 0, subkey, strlen(subkey),
                             server_data_ptr(req), len/sizeof(WCHAR) );
        ret = reg_server_call( REQ_LOAD_REGISTRY );
    }
    SERVER_END_REQ;
    CloseHandle( file );

 done:
    SetLastError( err );  /* restore the last error code */
    return ret;
}


/******************************************************************************
 *           RegSaveKeyA   [ADVAPI32.165]
 *
 * PARAMS
 *    hkey   [I] Handle of key where save begins
 *    lpFile [I] Address of filename to save to
 *    sa     [I] Address of security structure
 */
LONG WINAPI RegSaveKeyA( HKEY hkey, LPCSTR file, LPSECURITY_ATTRIBUTES sa )
{
    char buffer[1024];
    int count = 0;
    LPSTR name;
    DWORD ret, err;
    HFILE handle;

    TRACE( "(%x,%s,%p)\n", hkey, debugstr_a(file), sa );

    if (!file || !*file) return ERROR_INVALID_PARAMETER;

    err = GetLastError();
    GetFullPathNameA( file, sizeof(buffer), buffer, &name );
    for (;;)
    {
        sprintf( name, "reg%04x.tmp", count++ );
        handle = CreateFileA( buffer, GENERIC_WRITE, 0, NULL,
                            CREATE_NEW, FILE_ATTRIBUTE_NORMAL, -1 );
        if (handle != INVALID_HANDLE_VALUE) break;
        if ((ret = GetLastError()) != ERROR_ALREADY_EXISTS) goto done;

        /* Something gone haywire ? Please report if this happens abnormally */
        if (count >= 100)
            MESSAGE("Wow, we are already fiddling with a temp file %s with an ordinal as high as %d !\nYou might want to delete all corresponding temp files in that directory.\n", buffer, count);
    }

    SERVER_START_REQ
    {
        struct save_registry_request *req = server_alloc_req( sizeof(*req), 0 );
        req->hkey = hkey;
        req->file = handle;
        ret = reg_server_call( REQ_SAVE_REGISTRY );
    }
    SERVER_END_REQ;

    CloseHandle( handle );
    if (!ret)
    {
        if (!MoveFileExA( buffer, file, MOVEFILE_REPLACE_EXISTING ))
        {
            ERR( "Failed to move %s to %s\n", buffer, file );
            ret = GetLastError();
        }
    }
    if (ret) DeleteFileA( buffer );

done:
    SetLastError( err );  /* restore last error code */
    return ret;
}
