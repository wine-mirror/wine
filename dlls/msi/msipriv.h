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

struct string_table;
typedef struct string_table string_table;

typedef struct tagMSIDATABASE
{
    IStorage *storage;
    string_table *strings;
    LPWSTR mode;
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
     * get_int - sets one integer at {row,col} in the table
     *
     *  Similar semantics to fetch_int
     */
    UINT (*set_int)( struct tagMSIVIEW *, UINT row, UINT col, UINT val );

    /*
     * Inserts a new, blank row into the database
     *  *row receives the number of the new row
     */
    UINT (*insert_row)( struct tagMSIVIEW *, UINT *row );

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

DEFINE_GUID(CLSID_IMsiServer,   0x000C101C,0x0000,0x0000,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);
DEFINE_GUID(CLSID_IMsiServerX1, 0x000C103E,0x0000,0x0000,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);
DEFINE_GUID(CLSID_IMsiServerX2, 0x000C1090,0x0000,0x0000,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);
DEFINE_GUID(CLSID_IMsiServerX3, 0x000C1094,0x0000,0x0000,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);

DEFINE_GUID(CLSID_IMsiServerMessage, 0x000C101D,0x0000,0x0000,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);

extern void *msihandle2msiinfo(MSIHANDLE handle, UINT type);

MSIHANDLE alloc_msihandle(UINT type, UINT extra, msihandledestructor destroy, void **out);

/* add this table to the list of cached tables in the database */
extern void add_table(MSIDATABASE *db, MSITABLE *table);
extern void remove_table( MSIDATABASE *db, MSITABLE *table );
extern void free_table( MSIDATABASE *db, MSITABLE *table );
extern void free_cached_tables( MSIDATABASE *db );
extern UINT find_cached_table(MSIDATABASE *db, LPCWSTR name, MSITABLE **table);
extern UINT get_table(MSIDATABASE *db, LPCWSTR name, MSITABLE **table);
extern UINT load_string_table( MSIDATABASE *db );
extern UINT MSI_CommitTables( MSIDATABASE *db );
extern HRESULT init_string_table( IStorage *stg );


/* string table functions */
extern BOOL msi_addstring( string_table *st, UINT string_no, const CHAR *data, UINT len, UINT refcount );
extern BOOL msi_addstringW( string_table *st, UINT string_no, const WCHAR *data, UINT len, UINT refcount );
extern UINT msi_id2stringW( string_table *st, UINT string_no, LPWSTR buffer, UINT *sz );
extern UINT msi_id2stringA( string_table *st, UINT string_no, LPSTR buffer, UINT *sz );

extern LPWSTR MSI_makestring( MSIDATABASE *db, UINT stringid);
extern UINT msi_string2id( string_table *st, LPCWSTR buffer, UINT *id );
extern UINT msi_string2idA( string_table *st, LPCSTR str, UINT *id );
extern string_table *msi_init_stringtable( int entries );
extern VOID msi_destroy_stringtable( string_table *st );
extern UINT msi_string_count( string_table *st );
extern UINT msi_id_refcount( string_table *st, UINT i );
extern UINT msi_string_totalsize( string_table *st );

UINT VIEW_find_column( MSIVIEW *view, LPWSTR name, UINT *n );

extern BOOL TABLE_Exists( MSIDATABASE *db, LPWSTR name );

#endif /* __WINE_MSI_PRIVATE__ */
