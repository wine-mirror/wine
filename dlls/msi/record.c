/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2002 Mike McCormack for Codeweavers
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

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winerror.h"
#include "wine/debug.h"
#include "msi.h"
#include "msiquery.h"
#include "msipriv.h"
#include "objidl.h"
#include "winnls.h"

#include "query.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

#define MSIFIELD_NULL   0
#define MSIFIELD_INT    1
#define MSIFIELD_STR    2
#define MSIFIELD_WSTR   3
#define MSIFIELD_STREAM 4

/* maybe we can use a Variant instead of doing it ourselves? */
typedef struct tagMSIFIELD
{
    UINT type;
    union
    {
        INT iVal;
        LPSTR szVal;
        LPWSTR szwVal;
        IStream *stream;
    } u;
} MSIFIELD;

typedef struct tagMSIRECORD
{
    UINT count;       /* as passed to MsiCreateRecord */
    MSIFIELD fields[1]; /* nb. array size is count+1 */
} MSIRECORD;

void MSI_FreeField( MSIFIELD *field )
{
    switch( field->type )
    {
    case MSIFIELD_NULL:
    case MSIFIELD_INT:
        break;
    case MSIFIELD_STR:
        HeapFree( GetProcessHeap(), 0, field->u.szVal);
        break;
    case MSIFIELD_WSTR:
        HeapFree( GetProcessHeap(), 0, field->u.szwVal);
        break;
    case MSIFIELD_STREAM:
        IStream_Release( field->u.stream );
        break;
    default:
        ERR("Invalid field type %d\n", field->type);
    }
}

void MSI_CloseRecord( VOID *arg )
{
    MSIRECORD *rec = (MSIRECORD *) arg;
    UINT i;

    for( i=0; i<rec->count; i++ )
        MSI_FreeField( &rec->fields[i] );
}

MSIHANDLE WINAPI MsiCreateRecord( unsigned int cParams )
{
    MSIHANDLE handle = 0;
    UINT sz;
    MSIRECORD *rec;

    TRACE("%d\n", cParams);

    sz = sizeof (MSIRECORD) + sizeof(MSIFIELD)*(cParams+1) ;
    handle = alloc_msihandle( MSIHANDLETYPE_RECORD, sz, MSI_CloseRecord );
    if( !handle )
        return 0;

    rec = msihandle2msiinfo( handle, MSIHANDLETYPE_RECORD );
    if( !rec )
        return 0;

    rec->count = cParams;

    return handle;
}

unsigned int WINAPI MsiRecordGetFieldCount( MSIHANDLE handle )
{
    MSIRECORD *rec;

    TRACE("%ld\n", handle );

    rec = msihandle2msiinfo( handle, MSIHANDLETYPE_RECORD );
    if( !rec )
    {
        ERR("Record not found!\n");
        return 0;
    }

    return rec->count;
}

static BOOL string2intA( LPCSTR str, int *out )
{
    int x = 0;
    LPCSTR p = str;

    if( *p == '-' ) /* skip the minus sign */
        p++;
    while ( *p )
    {
        if( (*p < '0') || (*p > '9') )
            return FALSE;
        x *= 10;
        x += (*p - '0');
        p++;
    }

    if( str[0] == '-' ) /* check if it's negative */
        x = -x;
    *out = x; 

    return TRUE;
}

static BOOL string2intW( LPCWSTR str, int *out )
{
    int x = 0;
    LPCWSTR p = str;

    if( *p == '-' ) /* skip the minus sign */
        p++;
    while ( *p )
    {
        if( (*p < '0') || (*p > '9') )
            return FALSE;
        x *= 10;
        x += (*p - '0');
        p++;
    }

    if( str[0] == '-' ) /* check if it's negative */
        x = -x;
    *out = x; 

    return TRUE;
}

int WINAPI MsiRecordGetInteger( MSIHANDLE handle, unsigned int iField)
{
    MSIRECORD *rec;
    int ret = 0;

    TRACE("%ld %d\n", handle, iField );

    rec = msihandle2msiinfo( handle, MSIHANDLETYPE_RECORD );
    if( !rec )
        return MSI_NULL_INTEGER;

    if( iField > rec->count )
        return MSI_NULL_INTEGER;

    switch( rec->fields[iField].type )
    {
    case MSIFIELD_INT:
        return rec->fields[iField].u.iVal;
    case MSIFIELD_STR:
        if( string2intA( rec->fields[iField].u.szVal, &ret ) )
            return ret;
        return MSI_NULL_INTEGER;
    case MSIFIELD_WSTR:
        if( string2intW( rec->fields[iField].u.szwVal, &ret ) )
            return ret;
        return MSI_NULL_INTEGER;
    default:
        break;
    }

    return MSI_NULL_INTEGER;
}

UINT WINAPI MsiRecordClearData( MSIHANDLE handle )
{
    MSIRECORD *rec;
    UINT i;

    TRACE("%ld\n", handle );

    rec = msihandle2msiinfo( handle, MSIHANDLETYPE_RECORD );
    if( !rec )
        return ERROR_INVALID_HANDLE;

    for( i=0; i<=rec->count; i++)
    {
        MSI_FreeField( &rec->fields[i] );
        rec->fields[i].type = MSIFIELD_NULL;
        rec->fields[i].u.iVal = 0;
    }

    return ERROR_SUCCESS;
}

UINT WINAPI MsiRecordSetInteger( MSIHANDLE handle, unsigned int iField, int iVal )
{
    MSIRECORD *rec;

    TRACE("%ld %u %d\n", handle,iField, iVal);
    
    rec = msihandle2msiinfo( handle, MSIHANDLETYPE_RECORD );
    if( !rec )
        return ERROR_INVALID_HANDLE;

    if( iField > rec->count )
        return ERROR_INVALID_FIELD;

    MSI_FreeField( &rec->fields[iField] );
    rec->fields[iField].type = MSIFIELD_INT;
    rec->fields[iField].u.iVal = iVal;

    return ERROR_SUCCESS;
}

BOOL WINAPI MsiRecordIsNull( MSIHANDLE handle, unsigned int iField )
{
    MSIRECORD *rec;

    TRACE("%ld %d\n", handle,iField );

    rec = msihandle2msiinfo( handle, MSIHANDLETYPE_RECORD );
    if( !rec )
        return ERROR_INVALID_HANDLE;

    if( iField > rec->count ) 
        return TRUE;

    if( rec->fields[iField].type == MSIFIELD_NULL )
        return TRUE;

    return FALSE;
}

UINT WINAPI MsiRecordGetStringA(MSIHANDLE handle, unsigned int iField, 
               LPSTR szValue, DWORD *pcchValue)
{
    MSIRECORD *rec;
    UINT len=0, ret;
    CHAR buffer[16];

    TRACE("%ld %d %p %p\n", handle, iField, szValue, pcchValue);

    rec = msihandle2msiinfo( handle, MSIHANDLETYPE_RECORD );
    if( !rec )
        return ERROR_INVALID_HANDLE;

    if( iField > rec->count )
        return ERROR_INVALID_PARAMETER;

    ret = ERROR_SUCCESS;
    switch( rec->fields[iField].type )
    {
    case MSIFIELD_INT:
        wsprintfA(buffer, "%d", rec->fields[iField].u.iVal);
        len = lstrlenA( buffer );
        lstrcpynA(szValue, buffer, *pcchValue);
        break;
    case MSIFIELD_STR:
        len = lstrlenA( rec->fields[iField].u.szVal );
        lstrcpynA(szValue, rec->fields[iField].u.szVal, *pcchValue);
        break;
    case MSIFIELD_WSTR:
        len = WideCharToMultiByte( CP_ACP, 0, rec->fields[iField].u.szwVal, -1,
                             NULL, 0 , NULL, NULL);
        WideCharToMultiByte( CP_ACP, 0, rec->fields[iField].u.szwVal, -1,
                             szValue, *pcchValue, NULL, NULL);
        break;
    default:
        ret = ERROR_INVALID_PARAMETER;
        break;
    }

    if( *pcchValue < len )
        ret = ERROR_MORE_DATA;
    *pcchValue = len;

    return ret;
}

UINT WINAPI MsiRecordGetStringW(MSIHANDLE handle, unsigned int iField,
               LPWSTR szValue, DWORD *pcchValue)
{
    MSIRECORD *rec;
    UINT len=0, ret;
    WCHAR buffer[16];
    const WCHAR szFormat[] = { '%','d',0 };

    TRACE("%ld %d %p %p\n", handle, iField, szValue, pcchValue);

    rec = msihandle2msiinfo( handle, MSIHANDLETYPE_RECORD );
    if( !rec )
        return ERROR_INVALID_HANDLE;

    if( iField > rec->count )
        return ERROR_INVALID_PARAMETER;

    ret = ERROR_SUCCESS;
    switch( rec->fields[iField].type )
    {
    case MSIFIELD_INT:
        wsprintfW(buffer, szFormat, rec->fields[iField].u.iVal);
        len = lstrlenW( buffer );
        lstrcpynW(szValue, buffer, *pcchValue);
        break;
    case MSIFIELD_WSTR:
        len = lstrlenW( rec->fields[iField].u.szwVal );
        lstrcpynW(szValue, rec->fields[iField].u.szwVal, *pcchValue);
        break;
    case MSIFIELD_STR:
        len = MultiByteToWideChar( CP_ACP, 0, rec->fields[iField].u.szVal, -1,
                             NULL, 0 );
        MultiByteToWideChar( CP_ACP, 0, rec->fields[iField].u.szVal, -1,
                             szValue, *pcchValue);
        break;
    default:
        break;
    }

    if( *pcchValue < len )
        ret = ERROR_MORE_DATA;
    *pcchValue = len;

    return ret;
}

UINT WINAPI MsiRecordDataSize(MSIHANDLE hRecord, unsigned int iField)
{
    FIXME("%ld %d\n", hRecord, iField);
    return 0;
}

UINT WINAPI MsiRecordSetStringA( MSIHANDLE handle, unsigned int iField, LPCSTR szValue )
{
    MSIRECORD *rec;
    LPSTR str;

    TRACE("%ld %d %s\n", handle, iField, debugstr_a(szValue));

    rec = msihandle2msiinfo( handle, MSIHANDLETYPE_RECORD );
    if( !rec )
        return ERROR_INVALID_HANDLE;

    if( iField > rec->count )
        return ERROR_INVALID_FIELD;

    str = HeapAlloc( GetProcessHeap(), 0, (lstrlenA(szValue) + 1)*sizeof str[0]);
    lstrcpyA( str, szValue );

    MSI_FreeField( &rec->fields[iField] );
    rec->fields[iField].type = MSIFIELD_STR;
    rec->fields[iField].u.szVal = str;

    return 0;
}

UINT WINAPI MsiRecordSetStringW( MSIHANDLE handle, unsigned int iField, LPCWSTR szValue )
{
    MSIRECORD *rec;
    LPWSTR str;

    TRACE("%ld %d %s\n", handle, iField, debugstr_w(szValue));

    rec = msihandle2msiinfo( handle, MSIHANDLETYPE_RECORD );
    if( !rec )
        return ERROR_INVALID_HANDLE;

    if( iField > rec->count )
        return ERROR_INVALID_FIELD;

    str = HeapAlloc( GetProcessHeap(), 0, (lstrlenW(szValue) + 1)*sizeof str[0]);
    lstrcpyW( str, szValue );

    MSI_FreeField( &rec->fields[iField] );
    rec->fields[iField].type = MSIFIELD_WSTR;
    rec->fields[iField].u.szwVal = str;

    return 0;
}

UINT WINAPI MsiFormatRecordA(MSIHANDLE hInstall, MSIHANDLE hRecord, LPSTR szResult, DWORD *sz)
{
    FIXME("%ld %ld %p %p\n", hInstall, hRecord, szResult, sz);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiFormatRecordW(MSIHANDLE hInstall, MSIHANDLE hRecord, LPWSTR szResult, DWORD *sz)
{
    FIXME("%ld %ld %p %p\n", hInstall, hRecord, szResult, sz);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiRecordSetStreamA(MSIHANDLE hRecord, unsigned int iField, LPCSTR szFilename)
{
    FIXME("%ld %d %s\n", hRecord, iField, debugstr_a(szFilename));
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiRecordSetStreamW(MSIHANDLE hRecord, unsigned int iField, LPCWSTR szFilename)
{
    FIXME("%ld %d %s\n", hRecord, iField, debugstr_w(szFilename));
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiRecordReadStream(MSIHANDLE handle, unsigned int iField, char *buf, DWORD *sz)
{
    FIXME("%ld %d %p %p\n",handle,iField,buf,sz);
    return ERROR_CALL_NOT_IMPLEMENTED;
}
