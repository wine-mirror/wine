/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2004 Mike McCormack for CodeWeavers
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


/* below is the query interface to a table */

typedef struct tagMSIINSERTVIEW
{
    MSIVIEW          view;
    MSIDATABASE     *db;
    LPWSTR           name;
    BOOL             bIsTemp;
    string_list     *cols;
    value_list      *vals;
} MSIINSERTVIEW;

static UINT INSERT_fetch_int( struct tagMSIVIEW *view, UINT row, UINT col, UINT *val )
{
    MSIINSERTVIEW *iv = (MSIINSERTVIEW*)view;

    TRACE("%p %d %d %p\n", iv, row, col, val );

    return ERROR_FUNCTION_FAILED;
}

static UINT INSERT_execute( struct tagMSIVIEW *view, MSIHANDLE record )
{
#if 0
    MSIINSERTVIEW *iv = (MSIINSERTVIEW*)view;
    create_col_info *col;
    UINT r, nField, row, table_val, column_val;
    const WCHAR szTables[] =  { '_','T','a','b','l','e','s',0 };
    const WCHAR szColumns[] = { '_','C','o','l','u','m','n','s',0 };
    MSIVIEW *tv = NULL;

    TRACE("%p Table %s (%s)\n", iv, debugstr_w(iv->name), 
          iv->bIsTemp?"temporary":"permanent");

    /* only add tables that don't exist already */
    if( TABLE_Exists(iv->db, iv->name ) )
        return ERROR_BAD_QUERY_SYNTAX;

    /* add the name to the _Tables table */
    table_val = msi_addstringW( iv->db->strings, 0, iv->name, -1, 1 );
    TRACE("New string %s -> %d\n", debugstr_w( iv->name ), table_val );
    if( table_val < 0 )
        return ERROR_FUNCTION_FAILED;

    r = TABLE_CreateView( iv->db, szTables, &tv );
    TRACE("CreateView returned %x\n", r);
    if( r )
        return r;

    r = tv->ops->execute( tv, 0 );
    TRACE("tv execute returned %x\n", r);
    if( r )
        return r;

    row = -1;
    r = tv->ops->insert_row( tv, &row );
    TRACE("insert_row returned %x\n", r);
    if( r )
        goto err;

    r = tv->ops->set_int( tv, row, 1, table_val );
    if( r )
        goto err;
    tv->ops->delete( tv );
    tv = NULL;

    /* add each column to the _Columns table */
    r = TABLE_CreateView( iv->db, szColumns, &tv );
    if( r )
        return r;

    r = tv->ops->execute( tv, 0 );
    TRACE("tv execute returned %x\n", r);
    if( r )
        return r;

    /*
     * need to set the table, column number, col name and type
     * for each column we enter in the table
     */
    nField = 1;
    for( col = iv->col_info; col; col = col->next )
    {
        row = -1;
        r = tv->ops->insert_row( tv, &row );
        if( r )
            goto err;

        column_val = msi_addstringW( iv->db->strings, 0, col->colname, -1, 1 );
        TRACE("New string %s -> %d\n", debugstr_w( col->colname ), column_val );
        if( column_val < 0 )
            break;

        r = tv->ops->set_int( tv, row, 1, table_val );
        if( r )
            break;

        r = tv->ops->set_int( tv, row, 2, 0x8000|nField );
        if( r )
            break;

        r = tv->ops->set_int( tv, row, 3, column_val );
        if( r )
            break;

        r = tv->ops->set_int( tv, row, 4, 0x8000|col->type );
        if( r )
            break;
    }
    if( !col )
        r = ERROR_SUCCESS;

err:
    /* FIXME: remove values from the string table on error */
    if( tv )
        tv->ops->delete( tv );
    return r;
#else
    return ERROR_FUNCTION_FAILED;
#endif
}

static UINT INSERT_close( struct tagMSIVIEW *view )
{
    MSIINSERTVIEW *iv = (MSIINSERTVIEW*)view;

    TRACE("%p\n", iv);

    return ERROR_SUCCESS;
}

static UINT INSERT_get_dimensions( struct tagMSIVIEW *view, UINT *rows, UINT *cols )
{
    MSIINSERTVIEW *iv = (MSIINSERTVIEW*)view;

    TRACE("%p %p %p\n", iv, rows, cols );

    return ERROR_FUNCTION_FAILED;
}

static UINT INSERT_get_column_info( struct tagMSIVIEW *view,
                UINT n, LPWSTR *name, UINT *type )
{
    MSIINSERTVIEW *iv = (MSIINSERTVIEW*)view;

    TRACE("%p %d %p %p\n", iv, n, name, type );

    return ERROR_FUNCTION_FAILED;
}

static UINT INSERT_modify( struct tagMSIVIEW *view, MSIMODIFY eModifyMode, MSIHANDLE hrec)
{
    MSIINSERTVIEW *iv = (MSIINSERTVIEW*)view;

    TRACE("%p %d %ld\n", iv, eModifyMode, hrec );

    return ERROR_FUNCTION_FAILED;
}

static UINT INSERT_delete( struct tagMSIVIEW *view )
{
    MSIINSERTVIEW *iv = (MSIINSERTVIEW*)view;

    TRACE("%p\n", iv );

    delete_string_list( iv->cols ); 
    delete_value_list( iv->vals );
    HeapFree( GetProcessHeap(), 0, iv->name );
    HeapFree( GetProcessHeap(), 0, iv );

    return ERROR_SUCCESS;
}


MSIVIEWOPS insert_ops =
{
    INSERT_fetch_int,
    NULL,
    NULL,
    INSERT_execute,
    INSERT_close,
    INSERT_get_dimensions,
    INSERT_get_column_info,
    INSERT_modify,
    INSERT_delete
};

UINT INSERT_CreateView( MSIDATABASE *db, MSIVIEW **view, LPWSTR table,
                        string_list *columns, value_list *values, BOOL temp )
{
    MSIINSERTVIEW *iv = NULL;

    TRACE("%p\n", iv );

    iv = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof *iv );
    if( !iv )
        return ERROR_FUNCTION_FAILED;
    
    /* fill the structure */
    iv->view.ops = &insert_ops;
    iv->db = db;
    iv->name = table;  /* FIXME: strdupW it? */
    iv->cols = columns;
    iv->vals = values;
    iv->bIsTemp = temp;
    *view = (MSIVIEW*) iv;

    return ERROR_SUCCESS;
}
