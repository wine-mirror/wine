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

#ifndef __WINE_MSI_PRIVATE__
#define __WINE_MSI_PRIVATE__

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "msi.h"
#include "msiquery.h"
#include "objidl.h"

#define MSI_DATASIZEMASK 0x00ff
#define MSITYPE_VALID    0x0100
#define MSITYPE_STRING   0x0800
#define MSITYPE_NULLABLE 0x1000
#define MSITYPE_KEY      0x2000

struct tagMSITABLE;
typedef struct tagMSITABLE MSITABLE;

typedef struct pool_data_tag
{
    USHORT *data;
    UINT size;
} pool_data;

typedef struct string_data_tag
{
    CHAR *data;
    UINT size;
} string_data;

typedef struct string_table_tag
{
    pool_data   pool;
    string_data info;
} string_table;

typedef struct tagMSIDATABASE
{
    IStorage *storage;
    string_table strings;
    MSITABLE *first_table, *last_table;
} MSIDATABASE;

struct tagMSIVIEW;

typedef struct tagMSIVIEWOPS
{
    /*
     * fetch_int - reads one integer from {row,col} in the table
     *
     *  This function should be called after the execute method.
     *  Data returned by the function should not change until 
     *   close or delete is called.
     *  To get a string value, query the database's string table with
     *   the integer value returned from this function.
     */
    UINT (*fetch_int)( struct tagMSIVIEW *, UINT row, UINT col, UINT *val );

    /*
     * execute - loads the underlying data into memory so it can be read
     */
    UINT (*execute)( struct tagMSIVIEW *, MSIHANDLE );

    /*
     * close - clears the data read by execute from memory
     */
    UINT (*close)( struct tagMSIVIEW * );

    /*
     * get_dimensions - returns the number of rows or columns in a table.
     *
     *  The number of rows can only be queried after the execute method
     *   is called. The number of columns can be queried at any time.
     */
    UINT (*get_dimensions)( struct tagMSIVIEW *, UINT *rows, UINT *cols );

    /*
     * get_column_info - returns the name and type of a specific column
     *
     *  The name is HeapAlloc'ed by this function and should be freed by
     *   the caller.
     *  The column information can be queried at any time.
     */
    UINT (*get_column_info)( struct tagMSIVIEW *, UINT n, LPWSTR *name, UINT *type );

    /*
     * modify - not yet implemented properly
     */
    UINT (*modify)( struct tagMSIVIEW *, MSIMODIFY, MSIHANDLE );

    /*
     * delete - destroys the structure completely
     */
    UINT (*delete)( struct tagMSIVIEW * );

} MSIVIEWOPS;

typedef struct tagMSIVIEW
{
    MSIVIEWOPS   *ops;
} MSIVIEW;

typedef struct tagMSISUMMARYINFO
{
    IPropertyStorage *propstg;
} MSISUMMARYINFO;

typedef VOID (*msihandledestructor)( VOID * );

typedef struct tagMSIHANDLEINFO
{
    UINT magic;
    UINT type;
    msihandledestructor destructor;
    struct tagMSIHANDLEINFO *next;
    struct tagMSIHANDLEINFO *prev;
} MSIHANDLEINFO;

#define MSIHANDLETYPE_ANY 0
#define MSIHANDLETYPE_DATABASE 1
#define MSIHANDLETYPE_SUMMARYINFO 2
#define MSIHANDLETYPE_VIEW 3
#define MSIHANDLETYPE_RECORD 4

#define MSI_MAJORVERSION 1
#define MSI_MINORVERSION 10
#define MSI_BUILDNUMBER 1029

#define GUID_SIZE 39

#define MSIHANDLE_MAGIC 0x4d434923
#define MSIMAXHANDLES 0x80

#define MSISUMINFO_OFFSET 0x30LL

extern void *msihandle2msiinfo(MSIHANDLE handle, UINT type);

MSIHANDLE alloc_msihandle(UINT type, UINT extra, msihandledestructor destroy);

/* add this table to the list of cached tables in the database */
extern void add_table(MSIDATABASE *db, MSITABLE *table);
extern void remove_table( MSIDATABASE *db, MSITABLE *table );
extern void free_table( MSIDATABASE *db, MSITABLE *table );
extern void free_cached_tables( MSIDATABASE *db );
extern UINT find_cached_table(MSIDATABASE *db, LPCWSTR name, MSITABLE **table);
extern UINT get_table(MSIDATABASE *db, LPCWSTR name, MSITABLE **table);
extern UINT dump_string_table(MSIDATABASE *db);
extern UINT load_string_table( MSIDATABASE *db, string_table *pst);
extern UINT msi_id2string( string_table *st, UINT string_no, LPWSTR buffer, UINT *sz );
extern LPWSTR MSI_makestring( MSIDATABASE *db, UINT stringid);

UINT VIEW_find_column( MSIVIEW *view, LPWSTR name, UINT *n );

/* FIXME! should get rid of that */
#include "winnls.h"
inline static LPWSTR HEAP_strdupAtoW( HANDLE heap, DWORD flags, LPCSTR str )
{
    LPWSTR ret;
    INT len;

    if (!str) return NULL;
    len = MultiByteToWideChar( CP_ACP, 0, str, -1, NULL, 0 );
    ret = HeapAlloc( heap, flags, len * sizeof(WCHAR) );
    if (ret) MultiByteToWideChar( CP_ACP, 0, str, -1, ret, len );
    return ret;
}

#endif /* __WINE_MSI_PRIVATE__ */
