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
#include "winversion.h"
#include "file.h"
#include "heap.h"
#include "server.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(reg);


/* Ansi->Unicode conversion without string delimiters */
static LPWSTR memcpyAtoW( LPWSTR dst, LPCSTR src, INT n )
{
    LPWSTR p = dst;
    while (n-- > 0) *p++ = (WCHAR)*src++;
    return dst;
}

/* Unicode->Ansi conversion without string delimiters */
static LPSTR memcpyWtoA( LPSTR dst, LPCWSTR src, INT n )
{
    LPSTR p = dst;
    while (n-- > 0) *p++ = (CHAR)*src++;
    return dst;
}

/* check if value type needs string conversion (Ansi<->Unicode) */
static inline int is_string( DWORD type )
{
    return (type == REG_SZ) || (type == REG_EXPAND_SZ) || (type == REG_MULTI_SZ);
}

/* copy a key name into the request buffer */
static inline DWORD copy_nameW( LPWSTR dest, LPCWSTR name )
{
    if (name)
    {
        if (lstrlenW(name) > MAX_PATH) return ERROR_MORE_DATA;
        lstrcpyW( dest, name );
    }
    else dest[0] = 0;
    return ERROR_SUCCESS;
}

/* copy a key name into the request buffer */
static inline DWORD copy_nameAtoW( LPWSTR dest, LPCSTR name )
{
    if (name)
    {
        if (strlen(name) > MAX_PATH) return ERROR_MORE_DATA;
        lstrcpyAtoW( dest, name );
    }
    else dest[0] = 0;
    return ERROR_SUCCESS;
}

/* do a server call without setting the last error code */
static inline int reg_server_call( enum request req )
{
    unsigned int res = server_call_noerr( req );
    if (res) res = RtlNtStatusToDosError(res);
    return res;
}

/******************************************************************************
 *           RegCreateKeyExW   [ADVAPI32.131]
 *
 * PARAMS
 *    hkey       [I] Handle of an open key
 *    name       [I] Address of subkey name
 *    reserved   [I] Reserved - must be 0
 *    class      [I] Address of class string
 *    options    [I] Special options flag
 *    access     [I] Desired security access
 *    sa         [I] Address of key security structure
 *    retkey     [O] Address of buffer for opened handle
 *    dispos     [O] Receives REG_CREATED_NEW_KEY or REG_OPENED_EXISTING_KEY
 *
 * NOTES
 *  in case of failing retkey remains untouched
 */
DWORD WINAPI RegCreateKeyExW( HKEY hkey, LPCWSTR name, DWORD reserved, LPWSTR class,
                              DWORD options, REGSAM access, SECURITY_ATTRIBUTES *sa, 
                              LPHKEY retkey, LPDWORD dispos )
{
    DWORD ret;
    struct create_key_request *req = get_req_buffer();

    TRACE( "(0x%x,%s,%ld,%s,%lx,%lx,%p,%p,%p)\n", hkey, debugstr_w(name), reserved,
           debugstr_w(class), options, access, sa, retkey, dispos );

    if (reserved) return ERROR_INVALID_PARAMETER;
    if (!(access & KEY_ALL_ACCESS) || (access & ~KEY_ALL_ACCESS)) return ERROR_ACCESS_DENIED;

    req->parent  = hkey;
    req->access  = access;
    req->options = options;
    req->modif   = time(NULL);
    if ((ret = copy_nameW( req->name, name )) != ERROR_SUCCESS) return ret;
    if (req->name[0] == '\\') return ERROR_BAD_PATHNAME;
    lstrcpynW( req->class, class ? class : (LPWSTR)"\0\0",
               server_remaining(req->class) / sizeof(WCHAR) );
    if ((ret = reg_server_call( REQ_CREATE_KEY )) == ERROR_SUCCESS)
    {
        *retkey = req->hkey;
        if (dispos) *dispos = req->created ? REG_CREATED_NEW_KEY : REG_OPENED_EXISTING_KEY;
    }
    return ret;
}


/******************************************************************************
 *           RegCreateKeyExA   [ADVAPI32.130]
 */
DWORD WINAPI RegCreateKeyExA( HKEY hkey, LPCSTR name, DWORD reserved, LPSTR class,
                              DWORD options, REGSAM access, SECURITY_ATTRIBUTES *sa, 
                              LPHKEY retkey, LPDWORD dispos )
{
    DWORD ret;
    struct create_key_request *req = get_req_buffer();

    TRACE( "(0x%x,%s,%ld,%s,%lx,%lx,%p,%p,%p)\n", hkey, debugstr_a(name), reserved,
           debugstr_a(class), options, access, sa, retkey, dispos );

    if (reserved) return ERROR_INVALID_PARAMETER;
    if (!(access & KEY_ALL_ACCESS) || (access & ~KEY_ALL_ACCESS)) return ERROR_ACCESS_DENIED;

    req->parent  = hkey;
    req->access  = access;
    req->options = options;
    req->modif   = time(NULL);
    if ((ret = copy_nameAtoW( req->name, name )) != ERROR_SUCCESS) return ret;
    if (req->name[0] == '\\') return ERROR_BAD_PATHNAME;
    lstrcpynAtoW( req->class, class ? class : "",
                  server_remaining(req->class) / sizeof(WCHAR) );
    if ((ret = reg_server_call( REQ_CREATE_KEY )) == ERROR_SUCCESS)
    {
        *retkey = req->hkey;
        if (dispos) *dispos = req->created ? REG_CREATED_NEW_KEY : REG_OPENED_EXISTING_KEY;
    }
    return ret;
}


/******************************************************************************
 *           RegCreateKeyW   [ADVAPI32.132]
 */
DWORD WINAPI RegCreateKeyW( HKEY hkey, LPCWSTR name, LPHKEY retkey )
{
    /* FIXME: previous implementation converted ERROR_INVALID_HANDLE to ERROR_BADKEY, */
    /* but at least my version of NT (4.0 SP5) doesn't do this.  -- AJ */
    return RegCreateKeyExW( hkey, name, 0, NULL, REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS, NULL, retkey, NULL );
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
 *           RegOpenKeyExW   [ADVAPI32.150]
 *
 * Opens the specified key
 *
 * Unlike RegCreateKeyEx, this does not create the key if it does not exist.
 *
 * PARAMS
 *    hkey       [I] Handle of open key
 *    name       [I] Name of subkey to open
 *    reserved   [I] Reserved - must be zero
 *    access     [I] Security access mask
 *    retkey     [O] Handle to open key
 *
 * RETURNS
 *    Success: ERROR_SUCCESS
 *    Failure: Error code
 *
 * NOTES
 *  in case of failing is retkey = 0
 */
DWORD WINAPI RegOpenKeyExW( HKEY hkey, LPCWSTR name, DWORD reserved, REGSAM access, LPHKEY retkey )
{
    DWORD ret;
    struct open_key_request *req = get_req_buffer();

    TRACE( "(0x%x,%s,%ld,%lx,%p)\n", hkey, debugstr_w(name), reserved, access, retkey );

    if (!retkey) return ERROR_INVALID_PARAMETER;
    *retkey = 0;

    req->parent = hkey;
    req->access = access;
    if ((ret = copy_nameW( req->name, name )) != ERROR_SUCCESS) return ret;
    if (req->name[0] == '\\') return ERROR_BAD_PATHNAME;
    if ((ret = reg_server_call( REQ_OPEN_KEY )) == ERROR_SUCCESS) *retkey = req->hkey;
    return ret;
}


/******************************************************************************
 *           RegOpenKeyExA   [ADVAPI32.149]
 */
DWORD WINAPI RegOpenKeyExA( HKEY hkey, LPCSTR name, DWORD reserved, REGSAM access, LPHKEY retkey )
{
    DWORD ret;
    struct open_key_request *req = get_req_buffer();

    TRACE( "(0x%x,%s,%ld,%lx,%p)\n", hkey, debugstr_a(name), reserved, access, retkey );

    if (!retkey) return ERROR_INVALID_PARAMETER;
    *retkey = 0;

    req->parent = hkey;
    req->access = access;
    if ((ret = copy_nameAtoW( req->name, name )) != ERROR_SUCCESS) return ret;
    if (req->name[0] == '\\') return ERROR_BAD_PATHNAME;
    if ((ret = reg_server_call( REQ_OPEN_KEY )) == ERROR_SUCCESS) *retkey = req->hkey;
    return ret;
}


/******************************************************************************
 *           RegOpenKeyW   [ADVAPI32.151]
 *
 * PARAMS
 *    hkey    [I] Handle of open key
 *    name    [I] Address of name of subkey to open
 *    retkey  [O] Handle to open key
 *
 * RETURNS
 *    Success: ERROR_SUCCESS
 *    Failure: Error code
 *
 * NOTES
 *  in case of failing is retkey = 0
 */
DWORD WINAPI RegOpenKeyW( HKEY hkey, LPCWSTR name, LPHKEY retkey )
{
    return RegOpenKeyExW( hkey, name, 0, KEY_ALL_ACCESS, retkey );
}


/******************************************************************************
 *           RegOpenKeyA   [ADVAPI32.148]
 */
DWORD WINAPI RegOpenKeyA( HKEY hkey, LPCSTR name, LPHKEY retkey )
{
    return RegOpenKeyExA( hkey, name, 0, KEY_ALL_ACCESS, retkey );
}



/******************************************************************************
 *           RegEnumKeyExW   [ADVAPI32.139]
 *
 * PARAMS
 *    hkey         [I] Handle to key to enumerate
 *    index        [I] Index of subkey to enumerate
 *    name         [O] Buffer for subkey name
 *    name_len     [O] Size of subkey buffer
 *    reserved     [I] Reserved
 *    class        [O] Buffer for class string
 *    class_len    [O] Size of class buffer
 *    ft           [O] Time key last written to
 */
DWORD WINAPI RegEnumKeyExW( HKEY hkey, DWORD index, LPWSTR name, LPDWORD name_len,
                            LPDWORD reserved, LPWSTR class, LPDWORD class_len, FILETIME *ft )
{
    DWORD ret, len, cls_len;
    struct enum_key_request *req = get_req_buffer();

    TRACE( "(0x%x,%ld,%p,%p(%ld),%p,%p,%p,%p)\n", hkey, index, name, name_len,
           name_len ? *name_len : -1, reserved, class, class_len, ft );

    if (reserved) return ERROR_INVALID_PARAMETER;

    req->hkey = hkey;
    req->index = index;
    if ((ret = reg_server_call( REQ_ENUM_KEY )) != ERROR_SUCCESS) return ret;

    len = lstrlenW( req->name ) + 1;
    cls_len = lstrlenW( req->class ) + 1;
    if (len > *name_len) return ERROR_MORE_DATA;
    if (class_len && (cls_len > *class_len)) return ERROR_MORE_DATA;

    memcpy( name, req->name, len * sizeof(WCHAR) );
    *name_len = len - 1;
    if (class_len)
    {
        if (class) memcpy( class, req->class, cls_len * sizeof(WCHAR) );
        *class_len = cls_len - 1;
    }
    if (ft) DOSFS_UnixTimeToFileTime( req->modif, ft, 0 );
    return ERROR_SUCCESS;
}


/******************************************************************************
 *           RegEnumKeyExA   [ADVAPI32.138]
 */
DWORD WINAPI RegEnumKeyExA( HKEY hkey, DWORD index, LPSTR name, LPDWORD name_len,
                            LPDWORD reserved, LPSTR class, LPDWORD class_len, FILETIME *ft )
{
    DWORD ret, len, cls_len;
    struct enum_key_request *req = get_req_buffer();

    TRACE( "(0x%x,%ld,%p,%p(%ld),%p,%p,%p,%p)\n", hkey, index, name, name_len,
           name_len ? *name_len : -1, reserved, class, class_len, ft );

    if (reserved) return ERROR_INVALID_PARAMETER;

    req->hkey = hkey;
    req->index = index;
    if ((ret = reg_server_call( REQ_ENUM_KEY )) != ERROR_SUCCESS) return ret;

    len = lstrlenW( req->name ) + 1;
    cls_len = lstrlenW( req->class ) + 1;
    if (len > *name_len) return ERROR_MORE_DATA;
    if (class_len && (cls_len > *class_len)) return ERROR_MORE_DATA;

    memcpyWtoA( name, req->name, len );
    *name_len = len - 1;
    if (class_len)
    {
        if (class) memcpyWtoA( class, req->class, cls_len );
        *class_len = cls_len - 1;
    }
    if (ft) DOSFS_UnixTimeToFileTime( req->modif, ft, 0 );
    return ERROR_SUCCESS;
}


/******************************************************************************
 *           RegEnumKeyW   [ADVAPI32.140]
 */
DWORD WINAPI RegEnumKeyW( HKEY hkey, DWORD index, LPWSTR name, DWORD name_len )
{
    return RegEnumKeyExW( hkey, index, name, &name_len, NULL, NULL, NULL, NULL );
}


/******************************************************************************
 *           RegEnumKeyA   [ADVAPI32.137]
 */
DWORD WINAPI RegEnumKeyA( HKEY hkey, DWORD index, LPSTR name, DWORD name_len )
{
    return RegEnumKeyExA( hkey, index, name, &name_len, NULL, NULL, NULL, NULL );
}


/******************************************************************************
 *           RegQueryInfoKeyW   [ADVAPI32.153]
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
 * - win95 allows class to be valid and class_len to be NULL 
 * - winnt returns ERROR_INVALID_PARAMETER if class is valid and class_len is NULL
 * - both allow class to be NULL and class_len to be NULL 
 * (it's hard to test validity, so test !NULL instead)
 */
DWORD WINAPI RegQueryInfoKeyW( HKEY hkey, LPWSTR class, LPDWORD class_len, LPDWORD reserved,
                               LPDWORD subkeys, LPDWORD max_subkey, LPDWORD max_class,
                               LPDWORD values, LPDWORD max_value, LPDWORD max_data,
                               LPDWORD security, FILETIME *modif )
{
    DWORD ret;
    struct query_key_info_request *req = get_req_buffer();


    TRACE( "(0x%x,%p,%ld,%p,%p,%p,%p,%p,%p,%p,%p)\n", hkey, class, class_len ? *class_len : 0,
           reserved, subkeys, max_subkey, values, max_value, max_data, security, modif );

    if (class && !class_len && (VERSION_GetVersion() == NT40))
        return ERROR_INVALID_PARAMETER;

    req->hkey = hkey;
    if ((ret = reg_server_call( REQ_QUERY_KEY_INFO )) != ERROR_SUCCESS) return ret;

    if (class)
    {
        if (class_len && (lstrlenW(req->class) + 1 > *class_len))
        {
            *class_len = lstrlenW(req->class);
            return ERROR_MORE_DATA;
        }
        lstrcpyW( class, req->class );
    }
    if (class_len) *class_len = lstrlenW( req->class );
    if (subkeys) *subkeys = req->subkeys;
    if (max_subkey) *max_subkey = req->max_subkey;
    if (max_class) *max_class = req->max_class;
    if (values) *values = req->values;
    if (max_value) *max_value = req->max_value;
    if (max_data) *max_data = req->max_data;
    if (modif) DOSFS_UnixTimeToFileTime( req->modif, modif, 0 );
    return ERROR_SUCCESS;
}


/******************************************************************************
 *           RegQueryInfoKeyA   [ADVAPI32.152]
 */
DWORD WINAPI RegQueryInfoKeyA( HKEY hkey, LPSTR class, LPDWORD class_len, LPDWORD reserved,
                               LPDWORD subkeys, LPDWORD max_subkey, LPDWORD max_class,
                               LPDWORD values, LPDWORD max_value, LPDWORD max_data,
                               LPDWORD security, FILETIME *modif )
{
    DWORD ret;
    struct query_key_info_request *req = get_req_buffer();


    TRACE( "(0x%x,%p,%ld,%p,%p,%p,%p,%p,%p,%p,%p)\n", hkey, class, class_len ? *class_len : 0,
           reserved, subkeys, max_subkey, values, max_value, max_data, security, modif );

    if (class && !class_len && (VERSION_GetVersion() == NT40))
        return ERROR_INVALID_PARAMETER;

    req->hkey = hkey;
    if ((ret = reg_server_call( REQ_QUERY_KEY_INFO )) != ERROR_SUCCESS) return ret;

    if (class)
    {
        if (class_len && (lstrlenW(req->class) + 1 > *class_len))
        {
            *class_len = lstrlenW(req->class);
            return ERROR_MORE_DATA;
        }
        lstrcpyWtoA( class, req->class );
    }
    if (class_len) *class_len = lstrlenW( req->class );
    if (subkeys) *subkeys = req->subkeys;
    if (max_subkey) *max_subkey = req->max_subkey;
    if (max_class) *max_class = req->max_class;
    if (values) *values = req->values;
    if (max_value) *max_value = req->max_value;
    if (max_data) *max_data = req->max_data;
    if (modif) DOSFS_UnixTimeToFileTime( req->modif, modif, 0 );
    return ERROR_SUCCESS;
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
    struct close_key_request *req = get_req_buffer();
    TRACE( "(0x%x)\n", hkey );
    req->hkey = hkey;
    return reg_server_call( REQ_CLOSE_KEY );
}


/******************************************************************************
 *           RegDeleteKeyW   [ADVAPI32.134]
 *
 * PARAMS
 *    hkey   [I] Handle to open key
 *    name   [I] Name of subkey to delete
 *
 * RETURNS
 *    Success: ERROR_SUCCESS
 *    Failure: Error code
 */
DWORD WINAPI RegDeleteKeyW( HKEY hkey, LPCWSTR name )
{
    DWORD ret;
    struct delete_key_request *req = get_req_buffer();

    TRACE( "(0x%x,%s)\n", hkey, debugstr_w(name) );

    req->hkey = hkey;
    if ((ret = copy_nameW( req->name, name )) != ERROR_SUCCESS) return ret;
    if (req->name[0] == '\\') return ERROR_BAD_PATHNAME;
    return reg_server_call( REQ_DELETE_KEY );
}


/******************************************************************************
 *           RegDeleteKeyA   [ADVAPI32.133]
 */
DWORD WINAPI RegDeleteKeyA( HKEY hkey, LPCSTR name )
{
    DWORD ret;
    struct delete_key_request *req = get_req_buffer();

    TRACE( "(0x%x,%s)\n", hkey, debugstr_a(name) );

    req->hkey = hkey;
    if ((ret = copy_nameAtoW( req->name, name )) != ERROR_SUCCESS) return ret;
    if (req->name[0] == '\\') return ERROR_BAD_PATHNAME;
    return reg_server_call( REQ_DELETE_KEY );
}



/******************************************************************************
 *           RegSetValueExW   [ADVAPI32.170]
 *
 * Sets the data and type of a value under a register key
 *
 * PARAMS
 *    hkey       [I] Handle of key to set value for
 *    name       [I] Name of value to set
 *    reserved   [I] Reserved - must be zero
 *    type       [I] Flag for value type
 *    data       [I] Address of value data
 *    count      [I] Size of value data
 *
 * RETURNS
 *    Success: ERROR_SUCCESS
 *    Failure: Error code
 *
 * NOTES
 *   win95 does not care about count for REG_SZ and finds out the len by itself (js) 
 *   NT does definitely care (aj)
 */
DWORD WINAPI RegSetValueExW( HKEY hkey, LPCWSTR name, DWORD reserved,
                             DWORD type, CONST BYTE *data, DWORD count )
{
    DWORD ret;
    struct set_key_value_request *req = get_req_buffer();
    unsigned int max, pos;

    TRACE( "(0x%x,%s,%ld,%ld,%p,%ld)\n", hkey, debugstr_w(name), reserved, type, data, count );

    if (reserved) return ERROR_INVALID_PARAMETER;

    if (count && type == REG_SZ)
    {
        LPCWSTR str = (LPCWSTR)data;
        /* if user forgot to count terminating null, add it (yes NT does this) */
        if (str[count / sizeof(WCHAR) - 1] && !str[count / sizeof(WCHAR)])
            count += sizeof(WCHAR);
    }

    req->hkey = hkey;
    req->type = type;
    req->total = count;
    if ((ret = copy_nameW( req->name, name )) != ERROR_SUCCESS) return ret;

    max = server_remaining( req->data );
    pos = 0;
    while (pos < count)
    {
        unsigned int len = count - pos;
        if (len > max) len = max;
        req->offset = pos;
        req->len    = len;
        memcpy( req->data, data + pos, len );
        if ((ret = reg_server_call( REQ_SET_KEY_VALUE )) != ERROR_SUCCESS) break;
        pos += len;
    }
    return ret;
}


/******************************************************************************
 *           RegSetValueExA   [ADVAPI32.169]
 */
DWORD WINAPI RegSetValueExA( HKEY hkey, LPCSTR name, DWORD reserved, DWORD type,
			     CONST BYTE *data, DWORD count )
{
    DWORD ret;
    struct set_key_value_request *req = get_req_buffer();
    unsigned int max, pos;

    TRACE( "(0x%x,%s,%ld,%ld,%p,%ld)\n", hkey, debugstr_a(name), reserved, type, data, count );

    if (reserved) return ERROR_INVALID_PARAMETER;

    if (count && type == REG_SZ)
    {
        /* if user forgot to count terminating null, add it (yes NT does this) */
        if (data[count-1] && !data[count]) count++;
    }
    if (is_string( type )) /* need to convert to Unicode */
        count *= sizeof(WCHAR);

    req->hkey = hkey;
    req->type = type;
    req->total = count;
    if ((ret = copy_nameAtoW( req->name, name )) != ERROR_SUCCESS) return ret;

    max = server_remaining( req->data );
    pos = 0;
    while (pos < count)
    {
        unsigned int len = count - pos;
        if (len > max) len = max;
        req->offset = pos;
        req->len    = len;

        if (is_string( type ))
            memcpyAtoW( (LPWSTR)req->data, data + pos/sizeof(WCHAR), len/sizeof(WCHAR) );
        else
            memcpy( req->data, data + pos, len );
        if ((ret = reg_server_call( REQ_SET_KEY_VALUE )) != ERROR_SUCCESS) break;
        pos += len;
    }
    return ret;
}


/******************************************************************************
 *           RegSetValueW   [ADVAPI32.171]
 */
DWORD WINAPI RegSetValueW( HKEY hkey, LPCWSTR name, DWORD type, LPCWSTR data, DWORD count )
{
    HKEY subkey = hkey;
    DWORD ret;

    TRACE("(0x%x,%s,%ld,%s,%ld)\n", hkey, debugstr_w(name), type, debugstr_w(data), count );

    if (type != REG_SZ) return ERROR_INVALID_PARAMETER;

    if (name && name[0])  /* need to create the subkey */
    {
        if ((ret = RegCreateKeyW( hkey, name, &subkey )) != ERROR_SUCCESS) return ret;
    }

    ret = RegSetValueExW( subkey, NULL, 0, REG_SZ, (LPBYTE)data,
                          (lstrlenW( data ) + 1) * sizeof(WCHAR) );
    if (subkey != hkey) RegCloseKey( subkey );
    return ret;
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
 *           RegQueryValueExW   [ADVAPI32.158]
 *
 * Retrieves type and data for a specified name associated with an open key
 *
 * PARAMS
 *    hkey      [I]   Handle of key to query
 *    name      [I]   Name of value to query
 *    reserved  [I]   Reserved - must be NULL
 *    type      [O]   Address of buffer for value type.  If NULL, the type
 *                        is not required.
 *    data      [O]   Address of data buffer.  If NULL, the actual data is
 *                        not required.
 *    count     [I/O] Address of data buffer size
 *
 * RETURNS 
 *    ERROR_SUCCESS:   Success
 *    ERROR_MORE_DATA: !!! if the specified buffer is not big enough to hold the data
 * 		       buffer is left untouched. The MS-documentation is wrong (js) !!!
 */
DWORD WINAPI RegQueryValueExW( HKEY hkey, LPCWSTR name, LPDWORD reserved, LPDWORD type,
			       LPBYTE data, LPDWORD count )
{
    DWORD ret;
    struct get_key_value_request *req = get_req_buffer();

    TRACE("(0x%x,%s,%p,%p,%p,%p=%ld)\n",
          hkey, debugstr_w(name), reserved, type, data, count, count ? *count : 0 );

    if ((data && !count) || reserved) return ERROR_INVALID_PARAMETER;

    req->hkey = hkey;
    req->offset = 0;
    if ((ret = copy_nameW( req->name, name )) != ERROR_SUCCESS) return ret;
    if ((ret = reg_server_call( REQ_GET_KEY_VALUE )) != ERROR_SUCCESS) return ret;

    if (data)
    {
        if (*count < req->len) ret = ERROR_MORE_DATA;
        else
        {
            /* copy the data */
            unsigned int max = server_remaining( req->data );
            unsigned int pos = 0;
            while (pos < req->len)
            {
                unsigned int len = min( req->len - pos, max );
                memcpy( data + pos, req->data, len );
                if ((pos += len) >= req->len) break;
                req->offset = pos;
                if ((ret = copy_nameW( req->name, name )) != ERROR_SUCCESS) return ret;
                if ((ret = reg_server_call( REQ_GET_KEY_VALUE )) != ERROR_SUCCESS) return ret;
            }
        }
        /* if the type is REG_SZ and data is not 0-terminated
         * and there is enough space in the buffer NT appends a \0 */
        if (req->len && is_string(req->type) &&
            (req->len < *count) && ((WCHAR *)data)[req->len-1]) ((WCHAR *)data)[req->len] = 0;
    }
    if (type) *type = req->type;
    if (count) *count = req->len;
    return ret;
}


/******************************************************************************
 *           RegQueryValueExA   [ADVAPI32.157]
 *
 * NOTES:
 * the documentation is wrong: if the buffer is to small it remains untouched 
 */
DWORD WINAPI RegQueryValueExA( HKEY hkey, LPCSTR name, LPDWORD reserved, LPDWORD type,
			       LPBYTE data, LPDWORD count )
{
    DWORD ret, total_len;
    struct get_key_value_request *req = get_req_buffer();

    TRACE("(0x%x,%s,%p,%p,%p,%p=%ld)\n",
          hkey, debugstr_a(name), reserved, type, data, count, count ? *count : 0 );

    if ((data && !count) || reserved) return ERROR_INVALID_PARAMETER;

    req->hkey = hkey;
    req->offset = 0;
    if ((ret = copy_nameAtoW( req->name, name )) != ERROR_SUCCESS) return ret;
    if ((ret = reg_server_call( REQ_GET_KEY_VALUE )) != ERROR_SUCCESS) return ret;

    total_len = is_string( req->type ) ? req->len/sizeof(WCHAR) : req->len;

    if (data)
    {
        if (*count < total_len) ret = ERROR_MORE_DATA;
        else
        {
            /* copy the data */
            unsigned int max = server_remaining( req->data );
            unsigned int pos = 0;
            while (pos < req->len)
            {
                unsigned int len = min( req->len - pos, max );
                if (is_string( req->type ))
                    memcpyWtoA( data + pos/sizeof(WCHAR), (WCHAR *)req->data, len/sizeof(WCHAR) );
                else
                    memcpy( data + pos, req->data, len );
                if ((pos += len) >= req->len) break;
                req->offset = pos;
                if ((ret = copy_nameAtoW( req->name, name )) != ERROR_SUCCESS) return ret;
                if ((ret = reg_server_call( REQ_GET_KEY_VALUE )) != ERROR_SUCCESS) return ret;
            }
        }
        /* if the type is REG_SZ and data is not 0-terminated
         * and there is enough space in the buffer NT appends a \0 */
        if (total_len && is_string(req->type) && (total_len < *count) && data[total_len-1])
            data[total_len] = 0;
    }

    if (count) *count = total_len;
    if (type) *type = req->type;
    return ret;
}


/******************************************************************************
 *           RegQueryValueW   [ADVAPI32.159]
 */
DWORD WINAPI RegQueryValueW( HKEY hkey, LPCWSTR name, LPWSTR data, LPLONG count )
{
    DWORD ret;
    HKEY subkey = hkey;

    TRACE("(%x,%s,%p,%ld)\n", hkey, debugstr_w(name), data, count ? *count : 0 );

    if (name && name[0])
    {
        if ((ret = RegOpenKeyW( hkey, name, &subkey )) != ERROR_SUCCESS) return ret;
    }
    ret = RegQueryValueExW( subkey, NULL, NULL, NULL, (LPBYTE)data, count );
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
 *           RegEnumValueW   [ADVAPI32.142]
 *
 * PARAMS
 *    hkey       [I] Handle to key to query
 *    index      [I] Index of value to query
 *    value      [O] Value string
 *    val_count  [I/O] Size of value buffer (in wchars)
 *    reserved   [I] Reserved
 *    type       [O] Type code
 *    data       [O] Value data
 *    count      [I/O] Size of data buffer (in bytes)
 */

DWORD WINAPI RegEnumValueW( HKEY hkey, DWORD index, LPWSTR value, LPDWORD val_count,
                            LPDWORD reserved, LPDWORD type, LPBYTE data, LPDWORD count )
{
    DWORD ret, len;
    struct enum_key_value_request *req = get_req_buffer();

    TRACE("(%x,%ld,%p,%p,%p,%p,%p,%p)\n",
          hkey, index, value, val_count, reserved, type, data, count );

    /* NT only checks count, not val_count */
    if ((data && !count) || reserved) return ERROR_INVALID_PARAMETER;

    req->hkey = hkey;
    req->index = index;
    req->offset = 0;
    if ((ret = reg_server_call( REQ_ENUM_KEY_VALUE )) != ERROR_SUCCESS) return ret;

    len = lstrlenW( req->name ) + 1;
    if (len > *val_count) return ERROR_MORE_DATA;
    memcpy( value, req->name, len * sizeof(WCHAR) );
    *val_count = len - 1;

    if (data)
    {
        if (*count < req->len) ret = ERROR_MORE_DATA;
        else
        {
            /* copy the data */
            unsigned int max = server_remaining( req->data );
            unsigned int pos = 0;
            while (pos < req->len)
            {
                unsigned int len = min( req->len - pos, max );
                memcpy( data + pos, req->data, len );
                if ((pos += len) >= req->len) break;
                req->offset = pos;
                if ((ret = reg_server_call( REQ_ENUM_KEY_VALUE )) != ERROR_SUCCESS) return ret;
            }
        }
        /* if the type is REG_SZ and data is not 0-terminated
         * and there is enough space in the buffer NT appends a \0 */
        if (req->len && is_string(req->type) &&
            (req->len < *count) && ((WCHAR *)data)[req->len-1]) ((WCHAR *)data)[req->len] = 0;
    }
    if (type) *type = req->type;
    if (count) *count = req->len;
    return ret;
}


/******************************************************************************
 *           RegEnumValueA   [ADVAPI32.141]
 */
DWORD WINAPI RegEnumValueA( HKEY hkey, DWORD index, LPSTR value, LPDWORD val_count,
                            LPDWORD reserved, LPDWORD type, LPBYTE data, LPDWORD count )
{
    DWORD ret, len, total_len;
    struct enum_key_value_request *req = get_req_buffer();

    TRACE("(%x,%ld,%p,%p,%p,%p,%p,%p)\n",
          hkey, index, value, val_count, reserved, type, data, count );

    /* NT only checks count, not val_count */
    if ((data && !count) || reserved) return ERROR_INVALID_PARAMETER;

    req->hkey = hkey;
    req->index = index;
    req->offset = 0;
    if ((ret = reg_server_call( REQ_ENUM_KEY_VALUE )) != ERROR_SUCCESS) return ret;

    len = lstrlenW( req->name ) + 1;
    if (len > *val_count) return ERROR_MORE_DATA;
    memcpyWtoA( value, req->name, len );
    *val_count = len - 1;

    total_len = is_string( req->type ) ? req->len/sizeof(WCHAR) : req->len;

    if (data)
    {
        if (*count < total_len) ret = ERROR_MORE_DATA;
        else
        {
            /* copy the data */
            unsigned int max = server_remaining( req->data );
            unsigned int pos = 0;
            while (pos < req->len)
            {
                unsigned int len = min( req->len - pos, max );
                if (is_string( req->type ))
                    memcpyWtoA( data + pos/sizeof(WCHAR), (WCHAR *)req->data, len/sizeof(WCHAR) );
                else
                    memcpy( data + pos, req->data, len );
                if ((pos += len) >= req->len) break;
                req->offset = pos;
                if ((ret = reg_server_call( REQ_ENUM_KEY_VALUE )) != ERROR_SUCCESS) return ret;
            }
        }
        /* if the type is REG_SZ and data is not 0-terminated
         * and there is enough space in the buffer NT appends a \0 */
        if (total_len && is_string(req->type) && (total_len < *count) && data[total_len-1])
            data[total_len] = 0;
    }

    if (count) *count = total_len;
    if (type) *type = req->type;
    return ret;
}



/******************************************************************************
 *           RegDeleteValueW   [ADVAPI32.136]
 *
 * PARAMS
 *    hkey   [I] handle to key
 *    name   [I] name of value to delete
 *
 * RETURNS
 *    error status
 */
DWORD WINAPI RegDeleteValueW( HKEY hkey, LPCWSTR name )
{
    DWORD ret;
    struct delete_key_value_request *req = get_req_buffer();

    TRACE( "(0x%x,%s)\n", hkey, debugstr_w(name) );

    req->hkey = hkey;
    if ((ret = copy_nameW( req->name, name )) != ERROR_SUCCESS) return ret;
    return reg_server_call( REQ_DELETE_KEY_VALUE );
}


/******************************************************************************
 *           RegDeleteValueA   [ADVAPI32.135]
 */
DWORD WINAPI RegDeleteValueA( HKEY hkey, LPCSTR name )
{
    DWORD ret;
    struct delete_key_value_request *req = get_req_buffer();

    TRACE( "(0x%x,%s)\n", hkey, debugstr_a(name) );

    req->hkey = hkey;
    if ((ret = copy_nameAtoW( req->name, name )) != ERROR_SUCCESS) return ret;
    return reg_server_call( REQ_DELETE_KEY_VALUE );
}


/******************************************************************************
 *           RegLoadKeyW   [ADVAPI32.185]
 *
 * PARAMS
 *    hkey      [I] Handle of open key
 *    subkey    [I] Address of name of subkey
 *    filename  [I] Address of filename for registry information
 */
LONG WINAPI RegLoadKeyW( HKEY hkey, LPCWSTR subkey, LPCWSTR filename )
{
    struct load_registry_request *req = get_req_buffer();
    HANDLE file;
    DWORD ret, err = GetLastError();

    TRACE( "(%x,%s,%s)\n", hkey, debugstr_w(subkey), debugstr_w(filename) );

    if (!filename || !*filename) return ERROR_INVALID_PARAMETER;
    if (!subkey || !*subkey) return ERROR_INVALID_PARAMETER;

    if ((file = CreateFileW( filename, GENERIC_READ, 0, NULL, OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL, -1 )) == INVALID_HANDLE_VALUE)
    {
        ret = GetLastError();
        goto done;
    }
    req->hkey  = hkey;
    req->file  = file;
    if ((ret = copy_nameW( req->name, subkey )) != ERROR_SUCCESS) goto done;
    ret = reg_server_call( REQ_LOAD_REGISTRY );
    CloseHandle( file );

 done:
    SetLastError( err );  /* restore the last error code */
    return ret;
}


/******************************************************************************
 *           RegLoadKeyA   [ADVAPI32.184]
 */
LONG WINAPI RegLoadKeyA( HKEY hkey, LPCSTR subkey, LPCSTR filename )
{
    struct load_registry_request *req = get_req_buffer();
    HANDLE file;
    DWORD ret, err = GetLastError();

    TRACE( "(%x,%s,%s)\n", hkey, debugstr_a(subkey), debugstr_a(filename) );

    if (!filename || !*filename) return ERROR_INVALID_PARAMETER;
    if (!subkey || !*subkey) return ERROR_INVALID_PARAMETER;

    if ((file = CreateFileA( filename, GENERIC_READ, 0, NULL, OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL, -1 )) == INVALID_HANDLE_VALUE)
    {
        ret = GetLastError();
        goto done;
    }
    req->hkey  = hkey;
    req->file  = file;
    if ((ret = copy_nameAtoW( req->name, subkey )) != ERROR_SUCCESS) goto done;
    ret = reg_server_call( REQ_LOAD_REGISTRY );
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
    struct save_registry_request *req = get_req_buffer();
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

    req->hkey = hkey;
    req->file = handle;
    ret = reg_server_call( REQ_SAVE_REGISTRY );
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


/******************************************************************************
 *           RegSaveKeyW   [ADVAPI32.166]
 */
LONG WINAPI RegSaveKeyW( HKEY hkey, LPCWSTR file, LPSECURITY_ATTRIBUTES sa )
{
    LPSTR fileA = HEAP_strdupWtoA( GetProcessHeap(), 0, file );
    DWORD ret = RegSaveKeyA( hkey, fileA, sa );
    if (fileA) HeapFree( GetProcessHeap(), 0, fileA );
    return ret;
}
