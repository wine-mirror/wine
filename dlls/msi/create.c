/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2002 Mike McCormack for CodeWeavers
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

typedef struct tagMSICREATEVIEW
{
    MSIVIEW          view;
    MSIDATABASE     *db;
    LPWSTR           name;
    BOOL             bIsTemp;
    create_col_info *col_info;
} MSICREATEVIEW;

static UINT CREATE_fetch_int( struct tagMSIVIEW *view, UINT row, UINT col, UINT *val )
{
    MSICREATEVIEW *cv = (MSICREATEVIEW*)view;

    TRACE("%p %d %d %p\n", cv, row, col, val );

    return ERROR_FUNCTION_FAILED;
}

static UINT CREATE_execute( struct tagMSIVIEW *view, MSIHANDLE record )
{
    MSICREATEVIEW *cv = (MSICREATEVIEW*)view;
    create_col_info *c;

    FIXME("%p %ld\n", cv, record);

    FIXME("Table %s (%s)\n", debugstr_w(cv->name), 
          cv->bIsTemp?"temporary":"permanent");

    for( c = cv->col_info; c; c = c->next )
    {
        FIXME("Column %s  type %04x\n", debugstr_w(c->colname), c->type );
    }

    return ERROR_SUCCESS;
    return ERROR_FUNCTION_FAILED;
}

static UINT CREATE_close( struct tagMSIVIEW *view )
{
    MSICREATEVIEW *cv = (MSICREATEVIEW*)view;

    FIXME("%p\n", cv );

    return ERROR_SUCCESS;
    return ERROR_FUNCTION_FAILED;
}

static UINT CREATE_get_dimensions( struct tagMSIVIEW *view, UINT *rows, UINT *cols )
{
    MSICREATEVIEW *cv = (MSICREATEVIEW*)view;

    TRACE("%p %p %p\n", cv, rows, cols );

    return ERROR_FUNCTION_FAILED;
}

static UINT CREATE_get_column_info( struct tagMSIVIEW *view,
                UINT n, LPWSTR *name, UINT *type )
{
    MSICREATEVIEW *cv = (MSICREATEVIEW*)view;

    TRACE("%p %d %p %p\n", cv, n, name, type );

    return ERROR_FUNCTION_FAILED;
}

static UINT CREATE_modify( struct tagMSIVIEW *view, MSIMODIFY eModifyMode, MSIHANDLE hrec)
{
    MSICREATEVIEW *cv = (MSICREATEVIEW*)view;

    TRACE("%p %d %ld\n", cv, eModifyMode, hrec );

    return ERROR_FUNCTION_FAILED;
}

static UINT CREATE_delete( struct tagMSIVIEW *view )
{
    MSICREATEVIEW *cv = (MSICREATEVIEW*)view;
    create_col_info *col;

    TRACE("%p\n", cv );

    col = cv->col_info; 
    while( col )
    {
        create_col_info *t = col;
        col = col->next;
        HeapFree( GetProcessHeap(), 0, t->colname );
        HeapFree( GetProcessHeap(), 0, t );
    }
    HeapFree( GetProcessHeap(), 0, cv->name );
    HeapFree( GetProcessHeap(), 0, cv );

    return ERROR_SUCCESS;
}


MSIVIEWOPS create_ops =
{
    CREATE_fetch_int,
    CREATE_execute,
    CREATE_close,
    CREATE_get_dimensions,
    CREATE_get_column_info,
    CREATE_modify,
    CREATE_delete
};

UINT CREATE_CreateView( MSIDATABASE *db, MSIVIEW **view, LPWSTR table,
                        create_col_info *col_info, BOOL temp )
{
    MSICREATEVIEW *cv = NULL;

    TRACE("%p\n", cv );

    cv = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof *cv );
    if( !cv )
        return ERROR_FUNCTION_FAILED;
    
    /* fill the structure */
    cv->view.ops = &create_ops;
    cv->db = db;
    cv->name = table;  /* FIXME: strdupW it? */
    cv->col_info = col_info;
    cv->bIsTemp = temp;
    *view = (MSIVIEW*) cv;

    return ERROR_SUCCESS;
}
