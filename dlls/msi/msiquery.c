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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wine/debug.h"
#include "msi.h"
#include "msiquery.h"
#include "objbase.h"
#include "objidl.h"
#include "msipriv.h"
#include "winnls.h"

#include "query.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

typedef struct tagMSIQUERY
{
    MSIVIEW *view;
    UINT row;
    MSIDATABASE *db;
} MSIQUERY;

UINT WINAPI MsiDatabaseIsTablePersistentA(
              MSIHANDLE hDatabase, LPSTR szTableName)
{
    FIXME("%lx %s\n", hDatabase, debugstr_a(szTableName));
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiDatabaseIsTablePersistentW(
              MSIHANDLE hDatabase, LPWSTR szTableName)
{
    FIXME("%lx %s\n", hDatabase, debugstr_w(szTableName));
    return ERROR_CALL_NOT_IMPLEMENTED;
}

void MSI_CloseView( VOID *arg )
{
    MSIQUERY *query = arg;

    if( query->view && query->view->ops->delete )
        query->view->ops->delete( query->view );
}

UINT VIEW_find_column( MSIVIEW *table, LPWSTR name, UINT *n )
{
    LPWSTR col_name;
    UINT i, count, r;

    r = table->ops->get_dimensions( table, NULL, &count );
    if( r != ERROR_SUCCESS )
        return r;

    for( i=1; i<=count; i++ )
    {
        INT x;

        col_name = NULL;
        r = table->ops->get_column_info( table, i, &col_name, NULL );
        if( r != ERROR_SUCCESS )
            return r;
        x = lstrcmpW( name, col_name );
        HeapFree( GetProcessHeap(), 0, col_name );
        if( !x )
        {
            *n = i;
            return ERROR_SUCCESS;
        }
    }

    return ERROR_INVALID_PARAMETER;
}

UINT WINAPI MsiDatabaseOpenViewA(MSIHANDLE hdb,
              LPCSTR szQuery, MSIHANDLE *phView)
{
    UINT r;
    LPCWSTR szwQuery;

    TRACE("%ld %s %p\n", hdb, debugstr_a(szQuery), phView);

    if( szQuery )
    {
        szwQuery = HEAP_strdupAtoW( GetProcessHeap(), 0, szQuery );
        if( !szwQuery )
            return ERROR_FUNCTION_FAILED;
    }
    else
        szwQuery = NULL;

    r = MsiDatabaseOpenViewW( hdb, szwQuery, phView);

    return r;
}

UINT WINAPI MsiDatabaseOpenViewW(MSIHANDLE hdb,
              LPCWSTR szQuery, MSIHANDLE *phView)
{
    MSIDATABASE *db;
    MSIHANDLE handle;
    MSIQUERY *query;
    UINT r;

    TRACE("%s %p\n", debugstr_w(szQuery), phView);

    if( !szQuery)
        return ERROR_INVALID_PARAMETER;

    db = msihandle2msiinfo( hdb, MSIHANDLETYPE_DATABASE );
    if( !db )
        return ERROR_INVALID_HANDLE;

    /* pre allocate a handle to hold a pointer to the view */
    handle = alloc_msihandle( MSIHANDLETYPE_VIEW, sizeof (MSIQUERY), MSI_CloseView );
    if( !handle )
        return ERROR_FUNCTION_FAILED;
    query = msihandle2msiinfo( handle, MSIHANDLETYPE_VIEW );
    if( !query )
        return ERROR_FUNCTION_FAILED;

    query->row = 0;
    query->db = db;
    query->view = NULL;

    r = MSI_ParseSQL( db, szQuery, &query->view );
    if( r != ERROR_SUCCESS )
    {
        MsiCloseHandle( handle );
        return r;
    }

    *phView = handle;

    return ERROR_SUCCESS;
}

UINT WINAPI MsiViewFetch(MSIHANDLE hView, MSIHANDLE *record)
{
    MSIQUERY *query;
    MSIVIEW *view;
    MSIHANDLE handle;
    UINT row_count = 0, col_count = 0, i, ival, ret, type;

    TRACE("%ld %p\n", hView, record);

    query = msihandle2msiinfo( hView, MSIHANDLETYPE_VIEW );
    if( !query )
        return ERROR_INVALID_HANDLE;
    view = query->view;
    if( !view )
        return ERROR_FUNCTION_FAILED;

    ret = view->ops->get_dimensions( view, &row_count, &col_count );
    if( ret )
        return ret;
    if( !col_count )
        return ERROR_INVALID_PARAMETER;

    if( query->row >= row_count )
        return ERROR_NO_MORE_ITEMS;

    handle = MsiCreateRecord( col_count );
    if( !handle )
        return ERROR_FUNCTION_FAILED;

    for( i=1; i<=col_count; i++ )
    {
        ret = view->ops->get_column_info( view, i, NULL, &type );
        if( ret )
        {
            ERR("Error getting column type for %d\n", i );
            continue;
        }
        ret = view->ops->fetch_int( view, query->row, i, &ival );
        if( ret )
        {
            ERR("Error fetching data for %d\n", i );
            continue;
        }
        if( ! (type & MSITYPE_VALID ) )
            ERR("Invalid type!\n");

        /* check if it's nul (0) - if so, don't set anything */
        if( !ival )
            continue;

        if( type & MSITYPE_STRING )
        {
            LPWSTR sval = MSI_makestring( query->db, ival );
            MsiRecordSetStringW( handle, i, sval );
            HeapFree( GetProcessHeap(), 0, sval );
        }
        else
        {
            if( (type & MSI_DATASIZEMASK) == 2 )
                MsiRecordSetInteger( handle, i, ival - (1<<15) );
            else
                MsiRecordSetInteger( handle, i, ival - (1<<31) );
        }
    }
    query->row ++;

    *record = handle;

    return ERROR_SUCCESS;
}

UINT WINAPI MsiViewClose(MSIHANDLE hView)
{
    MSIQUERY *query;
    MSIVIEW *view;

    TRACE("%ld\n", hView );

    query = msihandle2msiinfo( hView, MSIHANDLETYPE_VIEW );
    if( !query )
        return ERROR_INVALID_HANDLE;

    view = query->view;
    if( !view )
        return ERROR_FUNCTION_FAILED;
    if( !view->ops->close )
        return ERROR_FUNCTION_FAILED;

    return view->ops->close( view );
}

UINT WINAPI MsiViewExecute(MSIHANDLE hView, MSIHANDLE hRec)
{
    MSIQUERY *query;
    MSIVIEW *view;

    TRACE("%ld %ld\n", hView, hRec);

    query = msihandle2msiinfo( hView, MSIHANDLETYPE_VIEW );
    if( !query )
        return ERROR_INVALID_HANDLE;

    view = query->view;
    if( !view )
        return ERROR_FUNCTION_FAILED;
    if( !view->ops->execute )
        return ERROR_FUNCTION_FAILED;
    query->row = 0;

    return view->ops->execute( view, hRec );
}

UINT WINAPI MsiViewGetColumnInfo(MSIHANDLE hView, MSICOLINFO info, MSIHANDLE *hRec)
{
    MSIVIEW *view;
    MSIQUERY *query;
    MSIHANDLE handle;
    UINT ret, i, count = 0, type;
    LPWSTR name;

    TRACE("%ld %d %p\n", hView, info, hRec);

    query = msihandle2msiinfo( hView, MSIHANDLETYPE_VIEW );
    if( !query )
        return ERROR_INVALID_HANDLE;

    view = query->view;
    if( !view )
        return ERROR_FUNCTION_FAILED;

    if( !view->ops->get_dimensions )
        return ERROR_FUNCTION_FAILED;

    ret = view->ops->get_dimensions( view, NULL, &count );
    if( ret )
        return ret;
    if( !count )
        return ERROR_INVALID_PARAMETER;

    handle = MsiCreateRecord( count );
    if( !handle )
        return ERROR_FUNCTION_FAILED;

    for( i=0; i<count; i++ )
    {
        name = NULL;
        ret = view->ops->get_column_info( view, i+1, &name, &type );
        if( ret != ERROR_SUCCESS )
            continue;
        MsiRecordSetStringW( handle, i+1, name );
        HeapFree( GetProcessHeap(), 0, name );
    }

    *hRec = handle;

    return ERROR_SUCCESS;
}


UINT WINAPI MsiDoActionA( MSIHANDLE hInstall, LPCSTR szAction )
{
    FIXME("%ld %s\n", hInstall, debugstr_a(szAction) );
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiDoActionW( MSIHANDLE hInstall, LPCWSTR szAction )
{
    FIXME("%ld %s\n", hInstall, debugstr_w(szAction) );
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiDatabaseApplyTransformA( MSIHANDLE hdb, 
                 LPCSTR szTransformFile, int iErrorCond)
{
    FIXME("%ld %s %d\n", hdb, debugstr_a(szTransformFile), iErrorCond);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiDatabaseApplyTransformW( MSIHANDLE hdb, 
                 LPCWSTR szTransformFile, int iErrorCond)
{
    FIXME("%ld %s %d\n", hdb, debugstr_w(szTransformFile), iErrorCond);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiDatabaseGenerateTransformA( MSIHANDLE hdb, MSIHANDLE hdbref,
                 LPCSTR szTransformFile, int iReserved1, int iReserved2 )
{
    FIXME("%ld %ld %s %d %d\n", hdb, hdbref, 
           debugstr_a(szTransformFile), iReserved1, iReserved2);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiDatabaseGenerateTransformW( MSIHANDLE hdb, MSIHANDLE hdbref,
                 LPCWSTR szTransformFile, int iReserved1, int iReserved2 )
{
    FIXME("%ld %ld %s %d %d\n", hdb, hdbref, 
           debugstr_w(szTransformFile), iReserved1, iReserved2);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiDatabaseCommit( MSIHANDLE hdb )
{
    FIXME("%ld\n", hdb);
    return ERROR_SUCCESS;
}

UINT WINAPI MsiDatabaseGetPrimaryKeysA(MSIHANDLE hdb, 
                    LPCSTR table, MSIHANDLE* rec)
{
    FIXME("%ld %s %p\n", hdb, debugstr_a(table), rec);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiDatabaseGetPrimaryKeysW(MSIHANDLE hdb,
                    LPCWSTR table, MSIHANDLE* rec)
{
    FIXME("%ld %s %p\n", hdb, debugstr_w(table), rec);
    return ERROR_CALL_NOT_IMPLEMENTED;
}
