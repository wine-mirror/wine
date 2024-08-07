/*
 * Win32 ODBC functions
 *
 * Copyright 1999 Xiang Li, Corel Corporation
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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winreg.h"
#include "winnls.h"
#include "wine/debug.h"

#include "sql.h"
#include "sqltypes.h"
#include "sqlext.h"
#include "unixlib.h"

WINE_DEFAULT_DEBUG_CHANNEL(odbc);

#define ODBC_CALL( func, params ) WINE_UNIX_CALL( unix_ ## func, params )

static BOOL is_wow64;

struct win32_funcs
{
    SQLRETURN (WINAPI *SQLAllocConnect)(SQLHENV,SQLHDBC*);
    SQLRETURN (WINAPI *SQLAllocEnv)(SQLHENV*);
    SQLRETURN (WINAPI *SQLAllocHandle)(SQLSMALLINT,SQLHANDLE,SQLHANDLE*);
    SQLRETURN (WINAPI *SQLAllocHandleStd)(SQLSMALLINT,SQLHANDLE,SQLHANDLE*);
    SQLRETURN (WINAPI *SQLAllocStmt)(SQLHDBC,SQLHSTMT*);
    SQLRETURN (WINAPI *SQLBindCol)(SQLHSTMT,SQLUSMALLINT,SQLSMALLINT,SQLPOINTER,SQLLEN,SQLLEN*);
    SQLRETURN (WINAPI *SQLBindParameter)(SQLHSTMT,SQLUSMALLINT,SQLSMALLINT,SQLSMALLINT,SQLSMALLINT,SQLULEN,
                                         SQLSMALLINT,SQLPOINTER,SQLLEN,SQLLEN*);
    SQLRETURN (WINAPI *SQLBrowseConnect)(SQLHDBC,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLSMALLINT*);
    SQLRETURN (WINAPI *SQLBrowseConnectW)(SQLHDBC,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLSMALLINT*);
    SQLRETURN (WINAPI *SQLBulkOperations)(SQLHSTMT,SQLSMALLINT);
    SQLRETURN (WINAPI *SQLCancel)(SQLHSTMT);
    SQLRETURN (WINAPI *SQLCloseCursor)(SQLHSTMT);
    SQLRETURN (WINAPI *SQLColAttribute)(SQLHSTMT,SQLUSMALLINT,SQLUSMALLINT,SQLPOINTER,SQLSMALLINT,SQLSMALLINT*,
                                        SQLLEN*);
    SQLRETURN (WINAPI *SQLColAttributeW)(SQLHSTMT,SQLUSMALLINT,SQLUSMALLINT,SQLPOINTER,SQLSMALLINT,SQLSMALLINT*,
                                         SQLLEN*);
    SQLRETURN (WINAPI *SQLColAttributes)(SQLHSTMT,SQLUSMALLINT,SQLUSMALLINT,SQLPOINTER,SQLSMALLINT,SQLSMALLINT*,
                                         SQLLEN*);
    SQLRETURN (WINAPI *SQLColAttributesW)(SQLHSTMT,SQLUSMALLINT,SQLUSMALLINT,SQLPOINTER,SQLSMALLINT,SQLSMALLINT*,
                                          SQLLEN*);
    SQLRETURN (WINAPI *SQLColumnPrivileges)(SQLHSTMT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,
                                           SQLCHAR*,SQLSMALLINT);
    SQLRETURN (WINAPI *SQLColumnPrivilegesW)(SQLHSTMT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,
                                             SQLWCHAR*,SQLSMALLINT);
    SQLRETURN (WINAPI *SQLColumns)(SQLHSTMT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,
                                   SQLSMALLINT);
    SQLRETURN (WINAPI *SQLColumnsW)(SQLHSTMT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,
                                    SQLWCHAR*, SQLSMALLINT);
    SQLRETURN (WINAPI *SQLConnect)(SQLHDBC,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT);
    SQLRETURN (WINAPI *SQLConnectW)(SQLHDBC,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT);
    SQLRETURN (WINAPI *SQLCopyDesc)(SQLHDESC,SQLHDESC);
    SQLRETURN (WINAPI *SQLDescribeCol)(SQLHSTMT,SQLUSMALLINT,SQLCHAR*,SQLSMALLINT,SQLSMALLINT*,SQLSMALLINT*,SQLULEN*,
                                       SQLSMALLINT*,SQLSMALLINT*);
    SQLRETURN (WINAPI *SQLDescribeColW)(SQLHSTMT,SQLUSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLSMALLINT*,SQLSMALLINT*,SQLULEN*,
                                        SQLSMALLINT*,SQLSMALLINT*);
    SQLRETURN (WINAPI *SQLDescribeParam)(SQLHSTMT,SQLUSMALLINT,SQLSMALLINT*,SQLULEN*,SQLSMALLINT*,SQLSMALLINT*);
    SQLRETURN (WINAPI *SQLDisconnect)(SQLHDBC);
    SQLRETURN (WINAPI *SQLDriverConnect)(SQLHDBC,SQLHWND,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLSMALLINT*,
                                         SQLUSMALLINT);
    SQLRETURN (WINAPI *SQLDriverConnectW)(SQLHDBC,SQLHWND,WCHAR*,SQLSMALLINT,WCHAR*,SQLSMALLINT,SQLSMALLINT*,
                                          SQLUSMALLINT);
    SQLRETURN (WINAPI *SQLEndTran)(SQLSMALLINT,SQLHANDLE,SQLSMALLINT);
    SQLRETURN (WINAPI *SQLError)(SQLHENV,SQLHDBC,SQLHSTMT,SQLCHAR*,SQLINTEGER*,SQLCHAR*,SQLSMALLINT,SQLSMALLINT*);
    SQLRETURN (WINAPI *SQLErrorW)(SQLHENV,SQLHDBC,SQLHSTMT,SQLWCHAR*,SQLINTEGER*,SQLWCHAR*,SQLSMALLINT,SQLSMALLINT*);
    SQLRETURN (WINAPI *SQLExecDirect)(SQLHSTMT,SQLCHAR*,SQLINTEGER);
    SQLRETURN (WINAPI *SQLExecDirectW)(SQLHSTMT,SQLWCHAR*,SQLINTEGER);
    SQLRETURN (WINAPI *SQLExecute)(SQLHSTMT);
    SQLRETURN (WINAPI *SQLExtendedFetch)(SQLHSTMT,SQLUSMALLINT,SQLLEN,SQLULEN*,SQLUSMALLINT*);
    SQLRETURN (WINAPI *SQLFetch)(SQLHSTMT);
    SQLRETURN (WINAPI *SQLFetchScroll)(SQLHSTMT,SQLSMALLINT,SQLLEN);
    SQLRETURN (WINAPI *SQLForeignKeys)(SQLHSTMT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,
                                       SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT);
    SQLRETURN (WINAPI *SQLForeignKeysW)(SQLHSTMT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,
                                        SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT);
    SQLRETURN (WINAPI *SQLFreeConnect)(SQLHDBC);
    SQLRETURN (WINAPI *SQLFreeEnv)(SQLHENV);
    SQLRETURN (WINAPI *SQLFreeHandle)(SQLSMALLINT,SQLHANDLE);
    SQLRETURN (WINAPI *SQLFreeStmt)(SQLHSTMT,SQLUSMALLINT);
    SQLRETURN (WINAPI *SQLGetConnectAttr)(SQLHDBC,SQLINTEGER,SQLPOINTER,SQLINTEGER,SQLINTEGER*);
    SQLRETURN (WINAPI *SQLGetConnectAttrW)(SQLHDBC,SQLINTEGER,SQLPOINTER,SQLINTEGER,SQLINTEGER*);
    SQLRETURN (WINAPI *SQLGetConnectOption)(SQLHDBC,SQLUSMALLINT,SQLPOINTER);
    SQLRETURN (WINAPI *SQLGetConnectOptionW)(SQLHDBC,SQLUSMALLINT,SQLPOINTER);
    SQLRETURN (WINAPI *SQLGetCursorName)(SQLHSTMT,SQLCHAR*,SQLSMALLINT,SQLSMALLINT*);
    SQLRETURN (WINAPI *SQLGetCursorNameW)(SQLHSTMT,SQLWCHAR*,SQLSMALLINT,SQLSMALLINT*);
    SQLRETURN (WINAPI *SQLGetData)(SQLHSTMT,SQLUSMALLINT,SQLSMALLINT,SQLPOINTER,SQLLEN,SQLLEN*);
    SQLRETURN (WINAPI *SQLGetDescField)(SQLHDESC,SQLSMALLINT,SQLSMALLINT,SQLPOINTER,SQLINTEGER,SQLINTEGER*);
    SQLRETURN (WINAPI *SQLGetDescFieldW)(SQLHDESC,SQLSMALLINT,SQLSMALLINT,SQLPOINTER,SQLINTEGER,SQLINTEGER*);
    SQLRETURN (WINAPI *SQLGetDescRec)(SQLHDESC,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLSMALLINT*,SQLSMALLINT*,
                                      SQLSMALLINT*,SQLLEN*,SQLSMALLINT*,SQLSMALLINT*,SQLSMALLINT*);
    SQLRETURN (WINAPI *SQLGetDescRecW)(SQLHDESC,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLSMALLINT*,SQLSMALLINT*,
                                       SQLSMALLINT*,SQLLEN*,SQLSMALLINT*,SQLSMALLINT*,SQLSMALLINT*);
    SQLRETURN (WINAPI *SQLGetDiagField)(SQLSMALLINT,SQLHANDLE,SQLSMALLINT,SQLSMALLINT,SQLPOINTER,SQLSMALLINT,
                                        SQLSMALLINT*);
    SQLRETURN (WINAPI *SQLGetDiagFieldW)(SQLSMALLINT,SQLHANDLE,SQLSMALLINT,SQLSMALLINT,SQLPOINTER,SQLSMALLINT,
                                         SQLSMALLINT*);
    SQLRETURN (WINAPI *SQLGetDiagRec)(SQLSMALLINT,SQLHANDLE,SQLSMALLINT,SQLCHAR*,SQLINTEGER*,SQLCHAR*,SQLSMALLINT,
                                      SQLSMALLINT*);
    SQLRETURN (WINAPI *SQLGetDiagRecW)(SQLSMALLINT,SQLHANDLE,SQLSMALLINT,SQLWCHAR*,SQLINTEGER*,SQLWCHAR*,SQLSMALLINT,
                                       SQLSMALLINT*);
    SQLRETURN (WINAPI *SQLGetEnvAttr)(SQLHENV,SQLINTEGER,SQLPOINTER,SQLINTEGER,SQLINTEGER*);
    SQLRETURN (WINAPI *SQLGetFunctions)(SQLHDBC,SQLUSMALLINT,SQLUSMALLINT*);
    SQLRETURN (WINAPI *SQLGetInfo)(SQLHDBC,SQLUSMALLINT,SQLPOINTER,SQLSMALLINT,SQLSMALLINT*);
    SQLRETURN (WINAPI *SQLGetInfoW)(SQLHDBC,SQLUSMALLINT,SQLPOINTER,SQLSMALLINT,SQLSMALLINT*);
    SQLRETURN (WINAPI *SQLGetStmtAttr)(SQLHSTMT,SQLINTEGER,SQLPOINTER,SQLINTEGER,SQLINTEGER*);
    SQLRETURN (WINAPI *SQLGetStmtAttrW)(SQLHSTMT,SQLINTEGER,SQLPOINTER,SQLINTEGER,SQLINTEGER*);
    SQLRETURN (WINAPI *SQLGetStmtOption)(SQLHSTMT,SQLUSMALLINT,SQLPOINTER);
    SQLRETURN (WINAPI *SQLGetTypeInfo)(SQLHSTMT,SQLSMALLINT);
    SQLRETURN (WINAPI *SQLGetTypeInfoW)(SQLHSTMT,SQLSMALLINT);
    SQLRETURN (WINAPI *SQLMoreResults)(SQLHSTMT);
    SQLRETURN (WINAPI *SQLNativeSql)(SQLHDBC,SQLCHAR*,SQLINTEGER,SQLCHAR*,SQLINTEGER,SQLINTEGER*);
    SQLRETURN (WINAPI *SQLNativeSqlW)(SQLHDBC,SQLWCHAR*,SQLINTEGER,SQLWCHAR*,SQLINTEGER,SQLINTEGER*);
    SQLRETURN (WINAPI *SQLNumParams)(SQLHSTMT,SQLSMALLINT*);
    SQLRETURN (WINAPI *SQLNumResultCols)(SQLHSTMT,SQLSMALLINT*);
    SQLRETURN (WINAPI *SQLParamData)(SQLHSTMT,SQLPOINTER*);
    SQLRETURN (WINAPI *SQLParamOptions)(SQLHSTMT,SQLULEN,SQLULEN*);
    SQLRETURN (WINAPI *SQLPrepare)(SQLHSTMT,SQLCHAR*,SQLINTEGER);
    SQLRETURN (WINAPI *SQLPrepareW)(SQLHSTMT,SQLWCHAR*,SQLINTEGER);
    SQLRETURN (WINAPI *SQLPrimaryKeys)(SQLHSTMT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT);
    SQLRETURN (WINAPI *SQLPrimaryKeysW)(SQLHSTMT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT);
    SQLRETURN (WINAPI *SQLProcedureColumns)(SQLHSTMT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,
                                            SQLCHAR*,SQLSMALLINT);
    SQLRETURN (WINAPI *SQLProcedureColumnsW)(SQLHSTMT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,
                                             SQLWCHAR*,SQLSMALLINT);
    SQLRETURN (WINAPI *SQLProcedures)(SQLHSTMT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT);
    SQLRETURN (WINAPI *SQLProceduresW)(SQLHSTMT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT);
    SQLRETURN (WINAPI *SQLPutData)(SQLHSTMT,SQLPOINTER,SQLLEN);
    SQLRETURN (WINAPI *SQLRowCount)(SQLHSTMT,SQLLEN*);
    SQLRETURN (WINAPI *SQLSetConnectAttr)(SQLHDBC,SQLINTEGER,SQLPOINTER,SQLINTEGER);
    SQLRETURN (WINAPI *SQLSetConnectAttrW)(SQLHDBC,SQLINTEGER,SQLPOINTER,SQLINTEGER);
    SQLRETURN (WINAPI *SQLSetConnectOption)(SQLHDBC,SQLUSMALLINT,SQLULEN);
    SQLRETURN (WINAPI *SQLSetConnectOptionW)(SQLHDBC,SQLUSMALLINT,SQLULEN);
    SQLRETURN (WINAPI *SQLSetCursorName)(SQLHSTMT,SQLCHAR*,SQLSMALLINT);
    SQLRETURN (WINAPI *SQLSetCursorNameW)(SQLHSTMT,SQLWCHAR*,SQLSMALLINT);
    SQLRETURN (WINAPI *SQLSetDescField)(SQLHDESC,SQLSMALLINT,SQLSMALLINT,SQLPOINTER,SQLINTEGER);
    SQLRETURN (WINAPI *SQLSetDescFieldW)(SQLHDESC,SQLSMALLINT,SQLSMALLINT,SQLPOINTER,SQLINTEGER);
    SQLRETURN (WINAPI *SQLSetDescRec)(SQLHDESC,SQLSMALLINT,SQLSMALLINT,SQLSMALLINT,SQLLEN,SQLSMALLINT,SQLSMALLINT,
                                      SQLPOINTER,SQLLEN*,SQLLEN*);
    SQLRETURN (WINAPI *SQLSetEnvAttr)(SQLHENV,SQLINTEGER,SQLPOINTER,SQLINTEGER);
    SQLRETURN (WINAPI *SQLSetParam)(SQLHSTMT,SQLUSMALLINT,SQLSMALLINT,SQLSMALLINT,SQLULEN,SQLSMALLINT,SQLPOINTER,
                                    SQLLEN*);
    SQLRETURN (WINAPI *SQLSetPos)(SQLHSTMT,SQLSETPOSIROW,SQLUSMALLINT,SQLUSMALLINT);
    SQLRETURN (WINAPI *SQLSetScrollOptions)(SQLHSTMT,SQLUSMALLINT,SQLLEN,SQLUSMALLINT);
    SQLRETURN (WINAPI *SQLSetStmtAttr)(SQLHSTMT,SQLINTEGER,SQLPOINTER,SQLINTEGER);
    SQLRETURN (WINAPI *SQLSetStmtAttrW)(SQLHSTMT,SQLINTEGER,SQLPOINTER,SQLINTEGER);
    SQLRETURN (WINAPI *SQLSetStmtOption)(SQLHSTMT,SQLUSMALLINT,SQLULEN);
    SQLRETURN (WINAPI *SQLSpecialColumns)(SQLHSTMT,SQLUSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,
                                          SQLSMALLINT,SQLUSMALLINT,SQLUSMALLINT);
    SQLRETURN (WINAPI *SQLSpecialColumnsW)(SQLHSTMT,SQLUSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,
                                           SQLSMALLINT,SQLUSMALLINT,SQLUSMALLINT);
    SQLRETURN (WINAPI *SQLStatistics)(SQLHSTMT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,
                                      SQLUSMALLINT,SQLUSMALLINT);
    SQLRETURN (WINAPI *SQLStatisticsW)(SQLHSTMT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,
                                       SQLUSMALLINT,SQLUSMALLINT);
    SQLRETURN (WINAPI *SQLTablePrivileges)(SQLHSTMT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT);
    SQLRETURN (WINAPI *SQLTablePrivilegesW)(SQLHSTMT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT);
    SQLRETURN (WINAPI *SQLTables)(SQLHSTMT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,
                                  SQLSMALLINT);
    SQLRETURN (WINAPI *SQLTablesW)(SQLHSTMT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,
                                   SQLSMALLINT);
    SQLRETURN (WINAPI *SQLTransact)(SQLHENV,SQLHDBC,SQLUSMALLINT);
};

struct win32_driver
{
    const WCHAR *filename;
    struct win32_funcs funcs;
};

static BOOL load_function_table( HMODULE module, struct win32_driver *driver )
{
#define LOAD_FUNCPTR(f) \
    if (!(driver->funcs.f = (typeof(f) *)GetProcAddress( module, #f ))) \
    { \
        TRACE( "failed to load %s\n", #f ); \
    }
    LOAD_FUNCPTR( SQLAllocConnect )
    LOAD_FUNCPTR( SQLAllocEnv )
    LOAD_FUNCPTR( SQLAllocHandle )
    LOAD_FUNCPTR( SQLAllocHandleStd )
    LOAD_FUNCPTR( SQLAllocStmt )
    LOAD_FUNCPTR( SQLBindCol )
    LOAD_FUNCPTR( SQLBindParameter )
    LOAD_FUNCPTR( SQLBrowseConnect )
    LOAD_FUNCPTR( SQLBrowseConnectW )
    LOAD_FUNCPTR( SQLBulkOperations )
    LOAD_FUNCPTR( SQLCancel )
    LOAD_FUNCPTR( SQLCloseCursor )
    LOAD_FUNCPTR( SQLColAttribute )
    LOAD_FUNCPTR( SQLColAttributeW )
    LOAD_FUNCPTR( SQLColAttributes )
    LOAD_FUNCPTR( SQLColAttributesW )
    LOAD_FUNCPTR( SQLColumnPrivileges )
    LOAD_FUNCPTR( SQLColumnPrivilegesW )
    LOAD_FUNCPTR( SQLColumns )
    LOAD_FUNCPTR( SQLColumnsW )
    LOAD_FUNCPTR( SQLConnect )
    LOAD_FUNCPTR( SQLConnectW )
    LOAD_FUNCPTR( SQLCopyDesc )
    LOAD_FUNCPTR( SQLDescribeCol )
    LOAD_FUNCPTR( SQLDescribeColW )
    LOAD_FUNCPTR( SQLDescribeParam )
    LOAD_FUNCPTR( SQLDisconnect )
    LOAD_FUNCPTR( SQLDriverConnect )
    LOAD_FUNCPTR( SQLDriverConnectW )
    LOAD_FUNCPTR( SQLEndTran )
    LOAD_FUNCPTR( SQLError )
    LOAD_FUNCPTR( SQLErrorW )
    LOAD_FUNCPTR( SQLExecDirect )
    LOAD_FUNCPTR( SQLExecDirectW )
    LOAD_FUNCPTR( SQLExecute )
    LOAD_FUNCPTR( SQLExtendedFetch )
    LOAD_FUNCPTR( SQLFetch )
    LOAD_FUNCPTR( SQLFetchScroll )
    LOAD_FUNCPTR( SQLForeignKeys )
    LOAD_FUNCPTR( SQLForeignKeysW )
    LOAD_FUNCPTR( SQLFreeConnect )
    LOAD_FUNCPTR( SQLFreeEnv )
    LOAD_FUNCPTR( SQLFreeHandle )
    LOAD_FUNCPTR( SQLFreeStmt )
    LOAD_FUNCPTR( SQLGetConnectAttr )
    LOAD_FUNCPTR( SQLGetConnectAttrW )
    LOAD_FUNCPTR( SQLGetConnectOption )
    LOAD_FUNCPTR( SQLGetConnectOptionW )
    LOAD_FUNCPTR( SQLGetCursorName )
    LOAD_FUNCPTR( SQLGetCursorNameW )
    LOAD_FUNCPTR( SQLGetData )
    LOAD_FUNCPTR( SQLGetDescField )
    LOAD_FUNCPTR( SQLGetDescFieldW )
    LOAD_FUNCPTR( SQLGetDescRec )
    LOAD_FUNCPTR( SQLGetDescRecW )
    LOAD_FUNCPTR( SQLGetDiagField )
    LOAD_FUNCPTR( SQLGetDiagFieldW )
    LOAD_FUNCPTR( SQLGetDiagRec )
    LOAD_FUNCPTR( SQLGetDiagRecW )
    LOAD_FUNCPTR( SQLGetEnvAttr )
    LOAD_FUNCPTR( SQLGetFunctions )
    LOAD_FUNCPTR( SQLGetInfo )
    LOAD_FUNCPTR( SQLGetInfoW )
    LOAD_FUNCPTR( SQLGetStmtAttr )
    LOAD_FUNCPTR( SQLGetStmtAttrW )
    LOAD_FUNCPTR( SQLGetStmtOption )
    LOAD_FUNCPTR( SQLGetTypeInfo )
    LOAD_FUNCPTR( SQLGetTypeInfoW )
    LOAD_FUNCPTR( SQLMoreResults )
    LOAD_FUNCPTR( SQLNativeSql )
    LOAD_FUNCPTR( SQLNativeSqlW )
    LOAD_FUNCPTR( SQLNumParams )
    LOAD_FUNCPTR( SQLNumResultCols )
    LOAD_FUNCPTR( SQLParamData )
    LOAD_FUNCPTR( SQLParamOptions )
    LOAD_FUNCPTR( SQLPrepare )
    LOAD_FUNCPTR( SQLPrepareW )
    LOAD_FUNCPTR( SQLPrimaryKeys )
    LOAD_FUNCPTR( SQLPrimaryKeysW )
    LOAD_FUNCPTR( SQLProcedureColumns )
    LOAD_FUNCPTR( SQLProcedureColumnsW )
    LOAD_FUNCPTR( SQLProcedures )
    LOAD_FUNCPTR( SQLProceduresW )
    LOAD_FUNCPTR( SQLPutData )
    LOAD_FUNCPTR( SQLRowCount )
    LOAD_FUNCPTR( SQLSetConnectAttr )
    LOAD_FUNCPTR( SQLSetConnectAttrW )
    LOAD_FUNCPTR( SQLSetConnectOption )
    LOAD_FUNCPTR( SQLSetConnectOptionW )
    LOAD_FUNCPTR( SQLSetCursorName )
    LOAD_FUNCPTR( SQLSetCursorNameW )
    LOAD_FUNCPTR( SQLSetDescField )
    LOAD_FUNCPTR( SQLSetDescFieldW )
    LOAD_FUNCPTR( SQLSetDescRec )
    LOAD_FUNCPTR( SQLSetEnvAttr )
    LOAD_FUNCPTR( SQLSetParam )
    LOAD_FUNCPTR( SQLSetPos )
    LOAD_FUNCPTR( SQLSetScrollOptions )
    LOAD_FUNCPTR( SQLSetStmtAttr )
    LOAD_FUNCPTR( SQLSetStmtAttrW )
    LOAD_FUNCPTR( SQLSetStmtOption )
    LOAD_FUNCPTR( SQLSpecialColumns )
    LOAD_FUNCPTR( SQLSpecialColumnsW )
    LOAD_FUNCPTR( SQLStatistics )
    LOAD_FUNCPTR( SQLStatisticsW )
    LOAD_FUNCPTR( SQLTablePrivileges )
    LOAD_FUNCPTR( SQLTablePrivilegesW )
    LOAD_FUNCPTR( SQLTables )
    LOAD_FUNCPTR( SQLTablesW )
    LOAD_FUNCPTR( SQLTransact )
    return TRUE;
#undef LOAD_FUNCPTR
}

static struct
{
    UINT32 count;
    struct win32_driver **drivers;
} win32_drivers;

static BOOL append_driver( struct win32_driver *driver )
{
    struct win32_driver **tmp;
    UINT32 new_count = win32_drivers.count + 1;

    if (!(tmp = realloc( win32_drivers.drivers, new_count * sizeof(*win32_drivers.drivers) )))
        return FALSE;

    tmp[win32_drivers.count] = driver;
    win32_drivers.drivers = tmp;
    win32_drivers.count   = new_count;
    return TRUE;
}

static const struct win32_funcs *load_driver( const WCHAR *filename )
{
    HMODULE module;
    struct win32_driver *driver;
    WCHAR *ptr;
    UINT32 i;

    for (i = 0; i < win32_drivers.count; i++)
    {
        if (!wcsicmp( filename, win32_drivers.drivers[i]->filename )) return &win32_drivers.drivers[i]->funcs;
    }

    if (!(driver = malloc( sizeof(*driver) + (wcslen(filename) + 1) * sizeof(WCHAR) ))) return NULL;
    ptr = (WCHAR *)(driver + 1);
    wcscpy( ptr, filename );
    driver->filename = ptr;

    module = LoadLibraryExW( filename, NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS );
    if (!module)
    {
        free( driver );
        return NULL;
    }

    if (!load_function_table( module, driver ) || !append_driver( driver ))
    {
        FreeLibrary( module );
        free( driver );
        return NULL;
    }

    return &driver->funcs;
}

static struct handle *create_handle( struct handle *parent )
{
    struct handle *ret;
    if (!(ret = calloc( 1, sizeof(*ret) ))) return NULL;
    ret->parent = parent;
    ret->env_attr_version = SQL_OV_ODBC2;
    ret->row_count = 1;
    return ret;
}

/*************************************************************************
 *				SQLAllocConnect           [ODBC32.001]
 */
SQLRETURN WINAPI SQLAllocConnect(SQLHENV EnvironmentHandle, SQLHDBC *ConnectionHandle)
{
    SQLRETURN ret = SQL_ERROR;

    TRACE("(EnvironmentHandle %p, ConnectionHandle %p)\n", EnvironmentHandle, ConnectionHandle);

    /* delay creating handle in lower layer until SQLConnect() is called */
    if ((*ConnectionHandle = create_handle( EnvironmentHandle ))) ret = SQL_SUCCESS;

    TRACE("Returning %d, ConnectionHandle %p\n", ret, *ConnectionHandle);
    return ret;
}

/*************************************************************************
 *				SQLAllocEnv           [ODBC32.002]
 */
SQLRETURN WINAPI SQLAllocEnv(SQLHENV *EnvironmentHandle)
{
    SQLRETURN ret = SQL_ERROR;

    TRACE("(EnvironmentHandle %p)\n", EnvironmentHandle);

    /* delay creating handle in lower layer until SQLConnect() is called */
    if ((*EnvironmentHandle = create_handle( NULL ))) ret = SQL_SUCCESS;

    TRACE("Returning %d, EnvironmentHandle %p\n", ret, *EnvironmentHandle);
    return ret;
}

/*************************************************************************
 *				SQLAllocHandle           [ODBC32.024]
 */
SQLRETURN WINAPI SQLAllocHandle(SQLSMALLINT HandleType, SQLHANDLE InputHandle, SQLHANDLE *OutputHandle)
{
    struct handle *output, *input = InputHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(HandleType %d, InputHandle %p, OutputHandle %p)\n", HandleType, InputHandle, OutputHandle);

    *OutputHandle = 0;
    if (!(output = create_handle( input ))) return SQL_ERROR;

    /* delay creating these handles in lower layer until SQLConnect() is called */
    if (HandleType == SQL_HANDLE_ENV || HandleType == SQL_HANDLE_DBC)
    {
        *OutputHandle = output;
        TRACE("Returning 0, OutputHandle %p\n", *OutputHandle);
        return SQL_SUCCESS;
    }

    if (input->unix_handle)
    {
        struct SQLAllocHandle_params params = { HandleType, input->unix_handle, &output->unix_handle };
        ret = ODBC_CALL( SQLAllocHandle, &params );
    }
    else if (input->win32_handle)
    {
        ret = input->win32_funcs->SQLAllocHandle( HandleType, input->win32_handle, &output->win32_handle );
        if (SUCCESS( ret )) output->win32_funcs = input->win32_funcs;
    }

    if (SUCCESS( ret )) *OutputHandle = output;
    else free( output );

    TRACE("Returning %d, OutputHandle %p\n", ret, *OutputHandle);
    return ret;
}

/*************************************************************************
 *				SQLAllocStmt           [ODBC32.003]
 */
SQLRETURN WINAPI SQLAllocStmt(SQLHDBC ConnectionHandle, SQLHSTMT *StatementHandle)
{
    struct handle *stmt, *con = ConnectionHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(ConnectionHandle %p, StatementHandle %p)\n", ConnectionHandle, StatementHandle);

    *StatementHandle = 0;
    if (!(stmt = create_handle( con ))) return SQL_ERROR;

    if (con->unix_handle)
    {
        struct SQLAllocStmt_params params = { con->unix_handle, &stmt->unix_handle };
        ret = ODBC_CALL( SQLAllocStmt, &params );
    }
    else if (con->win32_handle)
    {
        ret = con->win32_funcs->SQLAllocStmt( con->win32_handle, &stmt->win32_handle );
        if (SUCCESS( ret )) stmt->win32_funcs = con->win32_funcs;
    }

    if (SUCCESS( ret )) *StatementHandle = stmt;
    else free( stmt );

    TRACE ("Returning %d, StatementHandle %p\n", ret, *StatementHandle);
    return ret;
}

/*************************************************************************
 *				SQLAllocHandleStd           [ODBC32.077]
 */
SQLRETURN WINAPI SQLAllocHandleStd(SQLSMALLINT HandleType, SQLHANDLE InputHandle, SQLHANDLE *OutputHandle)
{
    struct handle *output, *input = InputHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(HandleType %d, InputHandle %p, OutputHandle %p)\n", HandleType, InputHandle, OutputHandle);

    *OutputHandle = 0;
    if (!(output = create_handle( input ))) return SQL_ERROR;

    /* delay creating these handles in lower layer until SQLConnect() is called */
    if (HandleType == SQL_HANDLE_ENV || HandleType == SQL_HANDLE_DBC)
    {
        *OutputHandle = output;
        TRACE("Returning 0, OutputHandle %p\n", *OutputHandle);
        return SQL_SUCCESS;
    }

    if (input->unix_handle)
    {
        struct SQLAllocHandleStd_params params = { HandleType, input->unix_handle, &output->unix_handle };
        ret = ODBC_CALL( SQLAllocHandleStd, &params );
    }
    else if (input->win32_handle)
    {
        ret = input->win32_funcs->SQLAllocHandleStd( HandleType, input->win32_handle, &output->win32_handle );
        if (SUCCESS( ret )) output->win32_funcs = input->win32_funcs;
    }

    if (SUCCESS( ret )) *OutputHandle = output;
    else free( output );

    TRACE ("Returning %d, OutputHandle %p\n", ret, *OutputHandle);
    return ret;
}

static const char *debugstr_sqllen( SQLLEN len )
{
#ifdef _WIN64
    return wine_dbg_sprintf( "%Id", len );
#else
    return wine_dbg_sprintf( "%d", len );
#endif
}

#define MAX_BINDING_PARAMS 1024
static BOOL alloc_binding( struct param_binding *binding, USHORT type, UINT column, UINT row_count )
{
    if (column > MAX_BINDING_PARAMS)
    {
        FIXME( "increase maximum number of parameters\n" );
        return FALSE;
    }
    if (!binding->param && !(binding->param = calloc( MAX_BINDING_PARAMS, sizeof(*binding->param)))) return FALSE;

    if (!(binding->param[column - 1].len = calloc( row_count, sizeof(UINT64) ))) return FALSE;
    binding->param[column - 1].type = type;
    binding->count = column;
    return TRUE;
}

/*************************************************************************
 *				SQLBindCol           [ODBC32.004]
 */
SQLRETURN WINAPI SQLBindCol(SQLHSTMT StatementHandle, SQLUSMALLINT ColumnNumber, SQLSMALLINT TargetType,
                            SQLPOINTER TargetValue, SQLLEN BufferLength, SQLLEN *StrLen_or_Ind)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, ColumnNumber %d, TargetType %d, TargetValue %p, BufferLength %s, StrLen_or_Ind %p)\n",
          StatementHandle, ColumnNumber, TargetType, TargetValue, debugstr_sqllen(BufferLength), StrLen_or_Ind);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        UINT i = ColumnNumber - 1;
        struct SQLBindCol_params params = { handle->unix_handle, ColumnNumber, TargetType, TargetValue, BufferLength };

        if (!ColumnNumber)
        {
            FIXME( "column 0 not handled\n" );
            return SQL_ERROR;
        }

        if (!alloc_binding( &handle->bind_col, SQL_PARAM_INPUT_OUTPUT, ColumnNumber, handle->row_count ))
            return SQL_ERROR;
        handle->bind_col.param[i].col.target_type   = TargetType;
        handle->bind_col.param[i].col.target_value  = TargetValue;
        handle->bind_col.param[i].col.buffer_length = BufferLength;

        if (StrLen_or_Ind) params.StrLen_or_Ind = handle->bind_col.param[i].len;
        if (SUCCESS(( ret = ODBC_CALL( SQLBindCol, &params )))) handle->bind_col.param[i].ptr = StrLen_or_Ind;
    }
    else if (handle->win32_handle)
    {
        ret = handle->win32_funcs->SQLBindCol( handle->win32_handle, ColumnNumber, TargetType, TargetValue,
                                               BufferLength, StrLen_or_Ind );
    }

    TRACE ("Returning %d\n", ret);
    return ret;
}

static const char *debugstr_sqlulen( SQLULEN len )
{
#ifdef _WIN64
    return wine_dbg_sprintf( "%Iu", len );
#else
    return wine_dbg_sprintf( "%u", len );
#endif
}

/*************************************************************************
 *				SQLBindParam           [ODBC32.025]
 */
SQLRETURN WINAPI SQLBindParam(SQLHSTMT StatementHandle, SQLUSMALLINT ParameterNumber, SQLSMALLINT ValueType,
                              SQLSMALLINT ParameterType, SQLULEN LengthPrecision, SQLSMALLINT ParameterScale,
                              SQLPOINTER ParameterValue, SQLLEN *StrLen_or_Ind)
{
    FIXME("(StatementHandle %p, ParameterNumber %d, ValueType %d, ParameterType %d, LengthPrecision %s,"
          " ParameterScale %d, ParameterValue %p, StrLen_or_Ind %p) stub\n", StatementHandle, ParameterNumber, ValueType,
          ParameterType, debugstr_sqlulen(LengthPrecision), ParameterScale, ParameterValue, StrLen_or_Ind);
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLCancel           [ODBC32.005]
 */
SQLRETURN WINAPI SQLCancel(SQLHSTMT StatementHandle)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p)\n", StatementHandle);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        struct SQLCancel_params params = { handle->unix_handle };
        ret = ODBC_CALL( SQLCancel, &params );
    }
    else if (handle->win32_handle)
    {
        ret = handle->win32_funcs->SQLCancel( handle->win32_handle );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLCloseCursor           [ODBC32.026]
 */
SQLRETURN WINAPI SQLCloseCursor(SQLHSTMT StatementHandle)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p)\n", StatementHandle);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        struct SQLCloseCursor_params params = { handle->unix_handle };
        ret = ODBC_CALL( SQLCloseCursor, &params );
    }
    else if (handle->win32_handle)
    {
        ret = handle->win32_funcs->SQLCloseCursor( handle->win32_handle );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

static SQLRETURN col_attribute_unix_a( struct handle *handle, SQLUSMALLINT col, SQLUSMALLINT field_id,
                                       SQLPOINTER char_attr, SQLSMALLINT buflen, SQLSMALLINT *retlen,
                                       SQLLEN *num_attr )
{
    SQLRETURN ret;
    INT64 attr;
    struct SQLColAttribute_params params = { handle->unix_handle, col, field_id, char_attr, buflen, retlen, &attr };
    if (SUCCESS((ret = ODBC_CALL( SQLColAttribute, &params ))) && num_attr) *num_attr = attr;
    return ret;
}

static SQLRETURN col_attribute_win32_a( struct handle *handle, SQLUSMALLINT col, SQLUSMALLINT field_id,
                                        SQLPOINTER char_attr, SQLSMALLINT buflen, SQLSMALLINT *retlen,
                                        SQLLEN *num_attr )
{
    SQLRETURN ret = SQL_ERROR;

    if (handle->win32_funcs->SQLColAttribute)
        return handle->win32_funcs->SQLColAttribute( handle->win32_handle, col, field_id, char_attr, buflen,
                                                     retlen, num_attr );

    if (handle->win32_funcs->SQLColAttributeW)
    {
        if (buflen < 0)
            ret = handle->win32_funcs->SQLColAttributeW( handle->win32_handle, col, field_id, char_attr, buflen,
                                                         retlen, num_attr );
        else
        {
            SQLWCHAR *strW;
            SQLSMALLINT lenW;

            if (!(strW = malloc( buflen * sizeof(WCHAR) ))) return SQL_ERROR;
            ret = handle->win32_funcs->SQLColAttributeW( handle->win32_handle, col, field_id, strW, buflen, &lenW,
                                                         num_attr );
            if (SUCCESS( ret ))
            {
                int len = WideCharToMultiByte( CP_ACP, 0, strW, -1, char_attr, buflen, NULL, NULL );
                if (retlen) *retlen = len - 1;
            }
            free( strW );
        }
    }
    return ret;
}

/*************************************************************************
 *				SQLColAttribute           [ODBC32.027]
 */
SQLRETURN WINAPI SQLColAttribute(SQLHSTMT StatementHandle, SQLUSMALLINT ColumnNumber, SQLUSMALLINT FieldIdentifier,
                                 SQLPOINTER CharacterAttribute, SQLSMALLINT BufferLength, SQLSMALLINT *StringLength,
                                 SQLLEN *NumericAttribute)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, ColumnNumber %d, FieldIdentifier %d, CharacterAttribute %p, BufferLength %d,"
          " StringLength %p, NumericAttribute %p)\n", StatementHandle, ColumnNumber, FieldIdentifier,
          CharacterAttribute, BufferLength, StringLength, NumericAttribute);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        ret = col_attribute_unix_a( handle, ColumnNumber, FieldIdentifier, CharacterAttribute, BufferLength,
                                    StringLength, NumericAttribute );
    }
    else if (handle->win32_handle)
    {
        ret = col_attribute_win32_a( handle, ColumnNumber, FieldIdentifier, CharacterAttribute, BufferLength,
                                     StringLength, NumericAttribute );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

static const char *debugstr_sqlstr( const SQLCHAR *str, SQLSMALLINT len )
{
    if (len == SQL_NTS) len = strlen( (const char *)str );
    return wine_dbgstr_an( (const char *)str, len );
}

static SQLWCHAR *strnAtoW( const SQLCHAR *str, int len )
{
    SQLWCHAR *ret;
    int lenW;

    if (!str) return NULL;

    if (len == SQL_NTS) len = strlen( (const char *)str );
    lenW = MultiByteToWideChar( CP_ACP, 0, (const char *)str, len, NULL, 0 );
    if ((ret = malloc( (lenW + 1) * sizeof(SQLWCHAR) )))
    {
        MultiByteToWideChar( CP_ACP, 0, (const char *)str, len, ret, lenW );
        ret[lenW] = 0;
    }
    return ret;
}

static SQLRETURN columns_unix_a( struct handle *handle, SQLCHAR *catalog, SQLSMALLINT len1, SQLCHAR *schema,
                                 SQLSMALLINT len2, SQLCHAR *table, SQLSMALLINT len3, SQLCHAR *column,
                                 SQLSMALLINT len4 )
{
    struct SQLColumns_params params = { handle->unix_handle, catalog, len1, schema, len2, table, len3, column, len4 };
    return ODBC_CALL( SQLColumns, &params );
}

static SQLRETURN columns_win32_a( struct handle *handle, SQLCHAR *catalog, SQLSMALLINT len1, SQLCHAR *schema,
                                  SQLSMALLINT len2, SQLCHAR *table, SQLSMALLINT len3, SQLCHAR *column,
                                  SQLSMALLINT len4 )
{
    SQLWCHAR *catalogW = NULL, *schemaW = NULL, *tableW = NULL, *columnW = NULL;
    SQLRETURN ret = SQL_ERROR;

    if (handle->win32_funcs->SQLColumns)
        return handle->win32_funcs->SQLColumns( handle->win32_handle, catalog, len1, schema, len2, table, len3,
                                                column, len4 );

    if (handle->win32_funcs->SQLColumnsW)
    {
        if (!(catalogW = strnAtoW( catalog, len1 ))) return SQL_ERROR;
        if (!(schemaW = strnAtoW( schema, len2 ))) goto done;
        if (!(tableW = strnAtoW( table, len3 ))) goto done;
        if (!(columnW = strnAtoW( column, len4 ))) goto done;
        ret = handle->win32_funcs->SQLColumnsW( handle->win32_handle, catalogW, len1, schemaW, len2, tableW, len3,
                                                columnW, len4 );
    }
done:
    free( catalogW );
    free( schemaW );
    free( tableW );
    free( columnW );
    return ret;
}

/*************************************************************************
 *				SQLColumns           [ODBC32.040]
 */
SQLRETURN WINAPI SQLColumns(SQLHSTMT StatementHandle, SQLCHAR *CatalogName, SQLSMALLINT NameLength1,
                            SQLCHAR *SchemaName, SQLSMALLINT NameLength2, SQLCHAR *TableName,
                            SQLSMALLINT NameLength3, SQLCHAR *ColumnName, SQLSMALLINT NameLength4)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d, TableName %s,"
          " NameLength3 %d, ColumnName %s, NameLength4 %d)\n", StatementHandle,
          debugstr_sqlstr(CatalogName, NameLength1), NameLength1, debugstr_sqlstr(SchemaName, NameLength2),
          NameLength2, debugstr_sqlstr(TableName, NameLength3), NameLength3,
          debugstr_sqlstr(ColumnName, NameLength4), NameLength4);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        ret = columns_unix_a( handle, CatalogName, NameLength1, SchemaName, NameLength2, TableName, NameLength3,
                              ColumnName, NameLength4 );
    }
    else if (handle->win32_handle)
    {
        ret = columns_win32_a( handle, CatalogName, NameLength1, SchemaName, NameLength2, TableName, NameLength3,
                               ColumnName, NameLength4 );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

static WCHAR *strdupAW( const char *src )
{
    int len;
    WCHAR *dst;
    if (!src) return NULL;
    len = MultiByteToWideChar( CP_ACP, 0, src, -1, NULL, 0 );
    if ((dst = malloc( len * sizeof(*dst) ))) MultiByteToWideChar( CP_ACP, 0, src, -1, dst, len );
    return dst;
}

static HKEY open_sources_key( HKEY root )
{
    static const WCHAR sourcesW[] = L"Software\\ODBC\\ODBC.INI\\ODBC Data Sources";
    HKEY key;
    if (!RegCreateKeyExW( root, sourcesW, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &key, NULL )) return key;
    return NULL;
}

static WCHAR *get_reg_value( HKEY key, const WCHAR *name )
{
    WCHAR *ret;
    DWORD len = 0;
    if (RegGetValueW( key, NULL, name, RRF_RT_REG_SZ, NULL, NULL, &len ) || !(ret = malloc( len ))) return NULL;
    if (!RegGetValueW( key, NULL, name, RRF_RT_REG_SZ, NULL, ret, &len )) return ret;
    free( ret );
    return NULL;
}

static HKEY open_odbcinst_key( void )
{
    static const WCHAR odbcinstW[] = L"Software\\ODBC\\ODBCINST.INI";
    HKEY key;
    if (!RegCreateKeyExW( HKEY_LOCAL_MACHINE, odbcinstW, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &key, NULL )) return key;
    return NULL;
}

static WCHAR *get_driver_filename( const SQLWCHAR *source )
{
    HKEY key_sources, key_odbcinst, key_driver;
    WCHAR *driver_name, *ret;

    if (!(key_sources = open_sources_key( HKEY_CURRENT_USER ))) return NULL;
    if (!(driver_name = get_reg_value( key_sources, source )))
    {
        RegCloseKey( key_sources );
        if (!(key_sources = open_sources_key( HKEY_LOCAL_MACHINE ))) return NULL;
        if (!(driver_name = get_reg_value( key_sources, source )))
        {
            RegCloseKey( key_sources );
            return NULL;
        }
    }
    RegCloseKey( key_sources );

    if (!(key_odbcinst = open_odbcinst_key()) || RegOpenKeyExW( key_odbcinst, driver_name, 0, KEY_READ, &key_driver ))
    {
        RegCloseKey( key_odbcinst );
        free( driver_name );
        return NULL;
    }

    ret = get_reg_value( key_driver, L"Driver" );

    RegCloseKey( key_driver );
    RegCloseKey( key_odbcinst );
    return ret;
}

static int has_suffix( const WCHAR *str, const WCHAR *suffix )
{
    int len = wcslen( str ), len2 = wcslen( suffix );
    return len >= len2 && !wcsicmp( str + len - len2, suffix );
}

static SQLRETURN set_env_attr( struct handle *handle, SQLINTEGER attr, SQLPOINTER value, SQLINTEGER len )
{
    SQLRETURN ret = SQL_ERROR;

    if (handle->unix_handle)
    {
        struct SQLSetEnvAttr_params params = { handle->unix_handle, attr, value, len };
        ret = ODBC_CALL( SQLSetEnvAttr, &params );
    }
    else if (handle->win32_handle)
    {
        ret = handle->win32_funcs->SQLSetEnvAttr( handle->win32_handle, attr, value, len );
    }
    return ret;
}

#define INT_PTR(val) (SQLPOINTER)(ULONG_PTR)val
static SQLRETURN prepare_env( struct handle *handle )
{
    SQLRETURN ret;
    if ((ret = set_env_attr( handle, SQL_ATTR_ODBC_VERSION, INT_PTR(handle->env_attr_version), 0 )))
        return ret;
    return SQL_SUCCESS;
}

static SQLRETURN create_env( struct handle *handle, BOOL is_unix )
{
    SQLRETURN ret;

    if (is_unix)
    {
        struct SQLAllocEnv_params params = { &handle->unix_handle };
        if ((ret = ODBC_CALL( SQLAllocEnv, &params ))) return ret;
    }
    else
    {
        if ((ret = handle->win32_funcs->SQLAllocHandle( SQL_HANDLE_ENV, NULL, &handle->win32_handle ))) return ret;
    }

    return prepare_env( handle );
}

static SQLRETURN set_con_attr( struct handle *handle, SQLINTEGER attr, SQLPOINTER value, SQLINTEGER len )
{
    SQLRETURN ret = SQL_ERROR;

    if (handle->unix_handle)
    {
        struct SQLSetConnectAttr_params params = { handle->unix_handle, attr, value, len };
        ret = ODBC_CALL( SQLSetConnectAttr, &params );
    }
    else if (handle->win32_handle)
    {
        switch (attr)
        {
        case SQL_ATTR_CURRENT_CATALOG:
        case SQL_ATTR_TRACEFILE:
        case SQL_ATTR_TRANSLATE_LIB:
            ERR( "string attribute %u not handled\n", attr );
            return SQL_ERROR;
        default:
            break;
        }
        if (handle->win32_funcs->SQLSetConnectAttrW)
            ret = handle->win32_funcs->SQLSetConnectAttrW( handle->win32_handle, attr, value, len );
        else if (handle->win32_funcs->SQLSetConnectAttr)
            ret = handle->win32_funcs->SQLSetConnectAttr( handle->win32_handle, attr, value, len );
    }
    return ret;
}

static SQLRETURN prepare_con( struct handle *handle )
{
    SQLRETURN ret;

    if ((ret = set_con_attr( handle, SQL_ATTR_CONNECTION_TIMEOUT, INT_PTR(handle->con_attr_con_timeout), 0 ))) return ret;
    if ((ret = set_con_attr( handle, SQL_ATTR_LOGIN_TIMEOUT, INT_PTR(handle->con_attr_login_timeout), 0 ))) return ret;
    return SQL_SUCCESS;
}

static SQLRETURN create_con( struct handle *handle )
{
    struct handle *parent = handle->parent;
    SQLRETURN ret;

    if (parent->unix_handle)
    {
        struct SQLAllocConnect_params params = { parent->unix_handle, &handle->unix_handle };
        if ((ret = ODBC_CALL( SQLAllocConnect, &params ))) return ret;
    }
    else
    {
        if ((ret = handle->win32_funcs->SQLAllocHandle( SQL_HANDLE_DBC, parent->win32_handle, &handle->win32_handle )))
            return ret;
    }

    return prepare_con( handle );
}

static SQLRETURN connect_win32_a( struct handle *handle, SQLCHAR *servername, SQLSMALLINT len1, SQLCHAR *username,
                                  SQLSMALLINT len2, SQLCHAR *auth, SQLSMALLINT len3 )
{
    SQLRETURN ret = SQL_ERROR;
    SQLWCHAR *servernameW = NULL, *usernameW = NULL, *authW = NULL;

    if (handle->win32_funcs->SQLConnect)
        return handle->win32_funcs->SQLConnect( handle->win32_handle, servername, len1, username, len2, auth, len3 );

    if (handle->win32_funcs->SQLConnectW)
    {
        if (!(servernameW = strnAtoW( servername, len1 ))) return SQL_ERROR;
        if (!(usernameW = strnAtoW( username, len2 ))) goto done;
        if (!(authW = strnAtoW( auth, len3 ))) goto done;
        ret = handle->win32_funcs->SQLConnectW( handle->win32_handle, servernameW, len1, usernameW, len2, authW, len3 );
    }
done:
    free( servernameW );
    free( usernameW );
    free( authW );
    return ret;
}

static SQLRETURN connect_unix_a( struct handle *handle, SQLCHAR *servername, SQLSMALLINT len1, SQLCHAR *username,
                                 SQLSMALLINT len2, SQLCHAR *authentication, SQLSMALLINT len3 )
{
    struct SQLConnect_params params = { handle->unix_handle, servername, len1, username, len2, authentication, len3 };
    return ODBC_CALL( SQLConnect, &params );
}

/*************************************************************************
 *				SQLConnect           [ODBC32.007]
 */
SQLRETURN WINAPI SQLConnect(SQLHDBC ConnectionHandle, SQLCHAR *ServerName, SQLSMALLINT NameLength1,
                            SQLCHAR *UserName, SQLSMALLINT NameLength2, SQLCHAR *Authentication,
                            SQLSMALLINT NameLength3)
{
    struct handle *handle = ConnectionHandle;
    WCHAR *filename = NULL, *servername = strdupAW( (const char *)ServerName );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(ConnectionHandle %p, ServerName %s, NameLength1 %d, UserName %s, NameLength2 %d, Authentication %s,"
          " NameLength3 %d)\n", ConnectionHandle,
          debugstr_sqlstr(ServerName, NameLength1), NameLength1, debugstr_sqlstr(UserName, NameLength2),
          NameLength2, debugstr_sqlstr(Authentication, NameLength3), NameLength3);

    if (!handle) return SQL_INVALID_HANDLE;

    if (!servername || !(filename = get_driver_filename( servername )))
    {
        WARN( "can't find driver filename\n" );
        goto done;
    }

    if (has_suffix( filename, L".dll" ))
    {
        if (!(handle->win32_funcs = handle->parent->win32_funcs = load_driver( filename )))
        {
            WARN( "failed to load driver %s\n", debugstr_w(filename) );
            goto done;
        }
        TRACE( "using Windows driver %s\n", debugstr_w(filename) );

        if (!SUCCESS((ret = create_env( handle->parent, FALSE )))) goto done;
        if (!SUCCESS((ret = create_con( handle )))) goto done;

        ret = connect_win32_a( handle, ServerName, NameLength1, UserName, NameLength2, Authentication, NameLength3 );
    }
    else
    {
        TRACE( "using Unix driver %s\n", debugstr_w(filename) );

        if (!SUCCESS((ret = create_env( handle->parent, TRUE )))) goto done;
        if (!SUCCESS((ret = create_con( handle )))) goto done;

        ret = connect_unix_a( handle, ServerName, NameLength1, UserName, NameLength2, Authentication, NameLength3 );
    }

done:
    free( servername );
    free( filename );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLCopyDesc           [ODBC32.028]
 */
SQLRETURN WINAPI SQLCopyDesc(SQLHDESC SourceDescHandle, SQLHDESC TargetDescHandle)
{
    struct handle *source = SourceDescHandle, *target = TargetDescHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(SourceDescHandle %p, TargetDescHandle %p)\n", SourceDescHandle, TargetDescHandle);

    if (!source || !target) return SQL_INVALID_HANDLE;

    if (source->unix_handle)
    {
        struct SQLCopyDesc_params params = { source->unix_handle, target->unix_handle };
        ret = ODBC_CALL( SQLCopyDesc, &params );
    }
    else if (source->win32_handle)
    {
        ret = source->win32_funcs->SQLCopyDesc( source->win32_handle, target->win32_handle );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLDataSources           [ODBC32.057]
 */
SQLRETURN WINAPI SQLDataSources(SQLHENV EnvironmentHandle, SQLUSMALLINT Direction, SQLCHAR *ServerName,
                                SQLSMALLINT BufferLength1, SQLSMALLINT *NameLength1, SQLCHAR *Description,
                                SQLSMALLINT BufferLength2, SQLSMALLINT *NameLength2)
{
    struct handle *handle = EnvironmentHandle;
    SQLRETURN ret = SQL_ERROR;
    DWORD len_source = BufferLength1, len_desc = BufferLength2;
    LONG res;

    TRACE("(EnvironmentHandle %p, Direction %d, ServerName %p, BufferLength1 %d, NameLength1 %p, Description %p,"
          " BufferLength2 %d, NameLength2 %p)\n", EnvironmentHandle, Direction, ServerName, BufferLength1,
          NameLength1, Description, BufferLength2, NameLength2);

    if (!handle) return SQL_INVALID_HANDLE;

    if (Direction == SQL_FETCH_FIRST || (Direction == SQL_FETCH_NEXT && !handle->sources_key))
    {
        handle->sources_idx = 0;
        handle->sources_system = FALSE;
        RegCloseKey( handle->sources_key );
        if (!(handle->sources_key = open_sources_key( HKEY_CURRENT_USER ))) return SQL_ERROR;
    }

    res = RegEnumValueA( handle->sources_key, handle->sources_idx, (char *)ServerName, &len_source, NULL,
                         NULL, (BYTE *)Description, &len_desc );
    if (res == ERROR_NO_MORE_ITEMS)
    {
        if (handle->sources_system)
        {
            ret = SQL_NO_DATA;
            goto done;
        }
        /* user key exhausted, continue with system key */
        RegCloseKey( handle->sources_key );
        if (!(handle->sources_key = open_sources_key( HKEY_LOCAL_MACHINE ))) goto done;
        handle->sources_idx = 0;
        handle->sources_system = TRUE;
        res = RegEnumValueA( handle->sources_key, handle->sources_idx, (char *)ServerName, &len_source, NULL,
                             NULL, (BYTE *)Description, &len_desc );
    }
    if (res == ERROR_NO_MORE_ITEMS)
    {
        ret = SQL_NO_DATA;
        goto done;
    }
    else if (res == ERROR_SUCCESS)
    {
        if (NameLength1) *NameLength1 = len_source;
        if (NameLength2) *NameLength2 = len_desc - 1;

        handle->sources_idx++;
        ret = SQL_SUCCESS;
    }

done:
    if (ret)
    {
        RegCloseKey( handle->sources_key );
        handle->sources_key = NULL;
        handle->sources_idx = 0;
    }
    TRACE("Returning %d\n", ret);
    return ret;
}

static SQLRETURN describe_col_unix_a( struct handle *handle, SQLUSMALLINT col_number, SQLCHAR *col_name,
                                      SQLSMALLINT buflen, SQLSMALLINT *retlen, SQLSMALLINT *data_type,
                                      SQLULEN *col_size, SQLSMALLINT *decimal_digits, SQLSMALLINT *nullable )
{
    SQLRETURN ret;
    SQLSMALLINT dummy;
    UINT64 size;
    struct SQLDescribeCol_params params = { handle->unix_handle, col_number, col_name, buflen, retlen, data_type,
                                            &size, decimal_digits, nullable };
    if (!retlen) params.NameLength = &dummy; /* workaround for drivers that don't accept NULL NameLength */

    if (SUCCESS((ret = ODBC_CALL( SQLDescribeCol, &params ))) && col_size) *col_size = size;
    return ret;
}

static SQLRETURN describe_col_win32_a( struct handle *handle, SQLUSMALLINT col_number, SQLCHAR *col_name,
                                       SQLSMALLINT buflen, SQLSMALLINT *retlen, SQLSMALLINT *data_type,
                                       SQLULEN *col_size, SQLSMALLINT *decimal_digits, SQLSMALLINT *nullable )
{
    SQLRETURN ret = SQL_ERROR;

    if (handle->win32_funcs->SQLDescribeCol)
        return handle->win32_funcs->SQLDescribeCol( handle->win32_handle, col_number, col_name, buflen, retlen,
                                                    data_type, col_size, decimal_digits, nullable );
    if (handle->win32_funcs->SQLDescribeColW)
    {
        SQLWCHAR *nameW;
        SQLSMALLINT lenW;

        if (!(nameW = malloc( buflen * sizeof(WCHAR) ))) return SQL_ERROR;
        ret = handle->win32_funcs->SQLDescribeColW( handle->win32_handle, col_number, nameW, buflen, &lenW,
                                                    data_type, col_size, decimal_digits, nullable );
        if (SUCCESS( ret ))
        {
            int len = WideCharToMultiByte( CP_ACP, 0, nameW, -1, (char *)col_name, buflen, NULL, NULL );
            if (retlen) *retlen = len - 1;
        }
        free( nameW );
    }
    return ret;
}

/*************************************************************************
 *				SQLDescribeCol           [ODBC32.008]
 */
SQLRETURN WINAPI SQLDescribeCol(SQLHSTMT StatementHandle, SQLUSMALLINT ColumnNumber, SQLCHAR *ColumnName,
                                SQLSMALLINT BufferLength, SQLSMALLINT *NameLength, SQLSMALLINT *DataType,
                                SQLULEN *ColumnSize, SQLSMALLINT *DecimalDigits, SQLSMALLINT *Nullable)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, ColumnNumber %d, ColumnName %p, BufferLength %d, NameLength %p, DataType %p,"
          " ColumnSize %p, DecimalDigits %p, Nullable %p)\n", StatementHandle, ColumnNumber, ColumnName,
          BufferLength, NameLength, DataType, ColumnSize, DecimalDigits, Nullable);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        ret = describe_col_unix_a( handle, ColumnNumber, ColumnName, BufferLength, NameLength, DataType,
                                   ColumnSize, DecimalDigits, Nullable );
    }
    else if (handle->win32_handle)
    {
        ret = describe_col_win32_a( handle, ColumnNumber, ColumnName, BufferLength, NameLength, DataType,
                                    ColumnSize, DecimalDigits, Nullable );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLDisconnect           [ODBC32.009]
 */
SQLRETURN WINAPI SQLDisconnect(SQLHDBC ConnectionHandle)
{
    struct handle *handle = ConnectionHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(ConnectionHandle %p)\n", ConnectionHandle);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        struct SQLDisconnect_params params = { handle->unix_handle };
        ret = ODBC_CALL( SQLDisconnect, &params );
    }
    else if (handle->win32_handle)
    {
        ret = handle->win32_funcs->SQLDisconnect( handle->win32_handle );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLEndTran           [ODBC32.029]
 */
SQLRETURN WINAPI SQLEndTran(SQLSMALLINT HandleType, SQLHANDLE Handle, SQLSMALLINT CompletionType)
{
    struct handle *handle = Handle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(HandleType %d, Handle %p, CompletionType %d)\n", HandleType, Handle, CompletionType);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        struct SQLEndTran_params params = { HandleType, handle->unix_handle, CompletionType };
        ret = ODBC_CALL( SQLEndTran, &params );
    }
    else if (handle->win32_handle)
    {
        ret = handle->win32_funcs->SQLEndTran( HandleType, handle->win32_handle, CompletionType );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

static SQLRETURN error_unix_a( struct handle *env, struct handle *con, struct handle *stmt, SQLCHAR *state,
                               SQLINTEGER *native_err, SQLCHAR *msg, SQLSMALLINT buflen, SQLSMALLINT *retlen )
{
    struct SQLError_params params = { env ? env->unix_handle : 0, con ? con->unix_handle : 0,
                                      stmt ? stmt->unix_handle : 0, state, native_err, msg, buflen, retlen };
    return ODBC_CALL( SQLError, &params );
}

static SQLRETURN error_win32_a( struct handle *env, struct handle *con, struct handle *stmt, SQLCHAR *state,
                                SQLINTEGER *native_err, SQLCHAR *msg, SQLSMALLINT buflen, SQLSMALLINT *retlen )
{
    const struct win32_funcs *win32_funcs;
    SQLRETURN ret = SQL_ERROR;

    if (env) win32_funcs = env->win32_funcs;
    else if (con) win32_funcs = con->win32_funcs;
    else win32_funcs = stmt->win32_funcs;

    if (win32_funcs->SQLError)
        return win32_funcs->SQLError( env ? env->win32_handle : NULL, con ? con->win32_handle : NULL,
                                      stmt ? stmt->win32_handle : NULL, state, native_err, msg, buflen, retlen );

    if (win32_funcs->SQLErrorW)
    {
        SQLWCHAR stateW[6], *msgW = NULL;
        SQLSMALLINT lenW;

        if (!(msgW = malloc( buflen * sizeof(SQLWCHAR) ))) return SQL_ERROR;
        ret = win32_funcs->SQLErrorW( env ? env->win32_handle : NULL, con ? con->win32_handle : NULL,
                                      stmt ? stmt->win32_handle : NULL, stateW, native_err, msgW, buflen, &lenW );
        if (SUCCESS( ret ))
        {
            int len = WideCharToMultiByte( CP_ACP, 0, msgW, -1, (char *)msg, buflen, NULL, NULL );
            if (retlen) *retlen = len - 1;
            WideCharToMultiByte( CP_ACP, 0, stateW, -1, (char *)state, 6, NULL, NULL );
        }
        free( msgW );
    }
    return ret;
}

/*************************************************************************
 *				SQLError           [ODBC32.010]
 */
SQLRETURN WINAPI SQLError(SQLHENV EnvironmentHandle, SQLHDBC ConnectionHandle, SQLHSTMT StatementHandle,
                          SQLCHAR *SqlState, SQLINTEGER *NativeError, SQLCHAR *MessageText,
                          SQLSMALLINT BufferLength, SQLSMALLINT *TextLength)
{
    struct handle *env = EnvironmentHandle, *con = ConnectionHandle, *stmt = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(EnvironmentHandle %p, ConnectionHandle %p, StatementHandle %p, SqlState %p, NativeError %p,"
          " MessageText %p, BufferLength %d, TextLength %p)\n", EnvironmentHandle, ConnectionHandle,
          StatementHandle, SqlState, NativeError, MessageText, BufferLength, TextLength);

    if (!env && !con && !stmt) return SQL_INVALID_HANDLE;

    if ((env && env->unix_handle) || (con && con->unix_handle) || (stmt && stmt->unix_handle))
    {
        ret = error_unix_a( env, con, stmt, SqlState, NativeError, MessageText, BufferLength, TextLength );
    }
    else if ((env && env->win32_handle) || (con && con->win32_handle) || (stmt && stmt->win32_handle))
    {
        ret = error_win32_a( env, con, stmt, SqlState, NativeError, MessageText, BufferLength, TextLength );
    }

    if (SUCCESS( ret ))
    {
        TRACE(" SqlState %s\n", debugstr_sqlstr(SqlState, 5));
        TRACE(" Error %d\n", *NativeError);
        TRACE(" MessageText %s\n", debugstr_sqlstr(MessageText, *TextLength));
    }
    TRACE("Returning %d\n", ret);
    return ret;
}

static SQLRETURN exec_direct_unix_a( struct handle *handle, SQLCHAR *text, SQLINTEGER len )
{
    struct SQLExecDirect_params params = { handle->unix_handle, text, len };
    return ODBC_CALL( SQLExecDirect, &params );
}

static SQLRETURN exec_direct_win32_a( struct handle *handle, SQLCHAR *text, SQLINTEGER len )
{
    SQLRETURN ret = SQL_ERROR;
    SQLWCHAR *textW;

    if (handle->win32_funcs->SQLExecDirect)
        return handle->win32_funcs->SQLExecDirect( handle->win32_handle, text, len );

    if (handle->win32_funcs->SQLExecDirectW)
    {
        if (!(textW = strnAtoW( text, len ))) return SQL_ERROR;
        ret = handle->win32_funcs->SQLExecDirectW( handle->win32_handle, textW, len );
        free( textW );
    }
    return ret;
}

/*************************************************************************
 *				SQLExecDirect           [ODBC32.011]
 */
SQLRETURN WINAPI SQLExecDirect(SQLHSTMT StatementHandle, SQLCHAR *StatementText, SQLINTEGER TextLength)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, StatementText %s, TextLength %d)\n", StatementHandle,
          debugstr_sqlstr(StatementText, TextLength), TextLength);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        ret = exec_direct_unix_a( handle, StatementText, TextLength );
    }
    else if (handle->win32_handle)
    {
        ret = exec_direct_win32_a( handle, StatementText, TextLength );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

static void len_to_user( SQLLEN *ptr, UINT8 *len, UINT row_count, UINT width )
{
    UINT i;
    for (i = 0; i < row_count; i++)
    {
        *ptr++ = *(SQLLEN *)(len + i * width);
    }
}

static void len_from_user( UINT8 *len, SQLLEN *ptr, UINT row_count, UINT width )
{
    UINT i;
    for (i = 0; i < row_count; i++)
    {
        *(SQLLEN *)(len + i * width) = *ptr++;
    }
}

static void update_result_lengths( struct handle *handle, USHORT type )
{
    UINT i, width = sizeof(void *) == 8 ? 8 : is_wow64 ? 8 : 4;

    switch (type)
    {
    case SQL_PARAM_OUTPUT:
        for (i = 0; i < handle->bind_col.count; i++)
        {
            len_to_user( handle->bind_col.param[i].ptr, handle->bind_col.param[i].len, handle->row_count, width );
        }
        for (i = 0; i < handle->bind_parameter.count; i++)
        {
            if (handle->bind_parameter.param[i].type != SQL_PARAM_OUTPUT &&
                handle->bind_parameter.param[i].type != SQL_PARAM_INPUT_OUTPUT) continue;

            len_to_user( handle->bind_parameter.param[i].ptr, handle->bind_parameter.param[i].len, handle->row_count, width );
        }
        break;

    case SQL_PARAM_INPUT:
        for (i = 0; i < handle->bind_col.count; i++)
        {
            len_from_user( handle->bind_col.param[i].len, handle->bind_col.param[i].ptr, handle->row_count, width );
        }
        for (i = 0; i < handle->bind_parameter.count; i++)
        {
            if (handle->bind_parameter.param[i].type != SQL_PARAM_INPUT &&
                handle->bind_parameter.param[i].type != SQL_PARAM_INPUT_OUTPUT) continue;

            len_from_user( handle->bind_parameter.param[i].len, handle->bind_parameter.param[i].ptr, handle->row_count, width );
        }

    default: break;
    }
}

/*************************************************************************
 *				SQLExecute           [ODBC32.012]
 */
SQLRETURN WINAPI SQLExecute(SQLHSTMT StatementHandle)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p)\n", StatementHandle);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        struct SQLExecute_params params = { handle->unix_handle };
        update_result_lengths( handle, SQL_PARAM_INPUT );
        if (SUCCESS(( ret = ODBC_CALL( SQLExecute, &params )))) update_result_lengths( handle, SQL_PARAM_OUTPUT );
    }
    else if (handle->win32_handle)
    {
        ret = handle->win32_funcs->SQLExecute( handle->win32_handle );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLFetch           [ODBC32.013]
 */
SQLRETURN WINAPI SQLFetch(SQLHSTMT StatementHandle)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p)\n", StatementHandle);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        struct SQLFetch_params params = { handle->unix_handle };
        if (SUCCESS(( ret = ODBC_CALL( SQLFetch, &params )))) update_result_lengths( handle, SQL_PARAM_OUTPUT );
    }
    else if (handle->win32_handle)
    {
        ret = handle->win32_funcs->SQLFetch( handle->win32_handle );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLFetchScroll          [ODBC32.030]
 */
SQLRETURN WINAPI SQLFetchScroll(SQLHSTMT StatementHandle, SQLSMALLINT FetchOrientation, SQLLEN FetchOffset)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, FetchOrientation %d, FetchOffset %s)\n", StatementHandle, FetchOrientation,
          debugstr_sqllen(FetchOffset));

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        struct SQLFetchScroll_params params = { handle->unix_handle, FetchOrientation, FetchOffset };
        if (SUCCESS(( ret = ODBC_CALL( SQLFetchScroll, &params )))) update_result_lengths( handle, SQL_PARAM_OUTPUT );
    }
    else if (handle->win32_handle)
    {
        ret = handle->win32_funcs->SQLFetchScroll( handle->win32_handle, FetchOrientation, FetchOffset );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLFreeConnect           [ODBC32.014]
 */
SQLRETURN WINAPI SQLFreeConnect(SQLHDBC ConnectionHandle)
{
    struct handle *handle = ConnectionHandle;
    SQLRETURN ret = SQL_SUCCESS;

    TRACE("(ConnectionHandle %p)\n", ConnectionHandle);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        struct SQLFreeHandle_params params = { SQL_HANDLE_DBC, handle->unix_handle };
        ret = ODBC_CALL( SQLFreeHandle, &params );
    }
    else if (handle->win32_handle)
    {
        ret = handle->win32_funcs->SQLFreeHandle( SQL_HANDLE_DBC, handle->win32_handle );
    }

    free( handle );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLFreeEnv           [ODBC32.015]
 */
SQLRETURN WINAPI SQLFreeEnv(SQLHENV EnvironmentHandle)
{
    struct handle *handle = EnvironmentHandle;
    SQLRETURN ret = SQL_SUCCESS;

    TRACE("(EnvironmentHandle %p)\n", EnvironmentHandle);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        struct SQLFreeHandle_params params = { SQL_HANDLE_ENV, handle->unix_handle };
        ret = ODBC_CALL( SQLFreeHandle, &params );
    }
    else if (handle->win32_handle)
    {
        ret = handle->win32_funcs->SQLFreeHandle( SQL_HANDLE_ENV, handle->win32_handle );
    }

    RegCloseKey( handle->drivers_key );
    RegCloseKey( handle->sources_key );
    free( handle );

    TRACE("Returning %d\n", ret);
    return ret;
}

static void free_col_bindings( struct handle *handle )
{
    if (handle->bind_col.param)
    {
        free( handle->bind_col.param->len );
        free( handle->bind_col.param );
        handle->bind_col.param = NULL;
    }
}

static void free_param_bindings( struct handle *handle )
{
    if (handle->bind_parameter.param)
    {
        free( handle->bind_parameter.param->len );
        free( handle->bind_parameter.param );
        handle->bind_parameter.param = NULL;
    }
}

/*************************************************************************
 *				SQLFreeHandle           [ODBC32.031]
 */
SQLRETURN WINAPI SQLFreeHandle(SQLSMALLINT HandleType, SQLHANDLE Handle)
{
    struct handle *handle = Handle;
    SQLRETURN ret = SQL_SUCCESS;

    TRACE("(HandleType %d, Handle %p)\n", HandleType, Handle);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        struct SQLFreeHandle_params params = { HandleType, handle->unix_handle };
        ret = ODBC_CALL( SQLFreeHandle, &params );
    }
    else if (handle->win32_handle)
    {
        ret = handle->win32_funcs->SQLFreeHandle( HandleType, handle->win32_handle );
    }

    RegCloseKey( handle->drivers_key );
    RegCloseKey( handle->sources_key );
    free_col_bindings( handle );
    free_param_bindings( handle );
    free( handle );

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLFreeStmt           [ODBC32.016]
 */
SQLRETURN WINAPI SQLFreeStmt(SQLHSTMT StatementHandle, SQLUSMALLINT Option)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, Option %d)\n", StatementHandle, Option);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        struct SQLFreeStmt_params params = { handle->unix_handle, Option };
        ret = ODBC_CALL( SQLFreeStmt, &params );
    }
    else if (handle->win32_handle)
    {
        ret = handle->win32_funcs->SQLFreeStmt( handle->win32_handle, Option );
    }

    switch (Option)
    {
    case SQL_CLOSE:
        break;

    case SQL_UNBIND:
        free_col_bindings( handle );
        break;

    case SQL_RESET_PARAMS:
        free_param_bindings( handle );
        break;

    case SQL_DROP:
    default:
        free_col_bindings( handle );
        free_param_bindings( handle );
        free( handle );
        break;
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

static SQLRETURN get_connect_attr_unix_a( struct handle *handle, SQLINTEGER attr, SQLPOINTER value,
                                          SQLINTEGER buflen, SQLINTEGER *retlen )
{
    struct SQLGetConnectAttr_params params = { handle->unix_handle, attr, value, buflen, retlen };
    return ODBC_CALL( SQLGetConnectAttr, &params );
}

static SQLRETURN get_connect_attr_win32_a( struct handle *handle, SQLINTEGER attr, SQLPOINTER value,
                                           SQLINTEGER buflen, SQLINTEGER *retlen )
{
    SQLRETURN ret = SQL_ERROR;
    SQLPOINTER val = value;
    SQLWCHAR *strW = NULL;

    if (handle->win32_funcs->SQLGetConnectAttr)
        return handle->win32_funcs->SQLGetConnectAttr( handle->win32_handle, attr, value, buflen, retlen );

    if (handle->win32_funcs->SQLGetConnectAttrW)
    {
        switch (attr)
        {
        case SQL_ATTR_CURRENT_CATALOG:
        case SQL_ATTR_TRACEFILE:
        case SQL_ATTR_TRANSLATE_LIB:
            if (!(val = strW = malloc( buflen * sizeof(WCHAR) ))) return SQL_ERROR;
        default:
            break;
        }

        ret = handle->win32_funcs->SQLGetConnectAttrW( handle->win32_handle, attr, val, buflen, retlen );
        if (SUCCESS( ret ) && strW)
        {
            int len = WideCharToMultiByte( CP_ACP, 0, strW, -1, (char *)value, buflen, NULL, NULL );
            if (retlen) *retlen = len - 1;
        }
        free( strW );
    }
    return ret;
}

/*************************************************************************
 *				SQLGetConnectAttr           [ODBC32.032]
 */
SQLRETURN WINAPI SQLGetConnectAttr(SQLHDBC ConnectionHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                   SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
    struct handle *handle = ConnectionHandle;
    SQLRETURN ret = SQL_SUCCESS;

    TRACE("(ConnectionHandle %p, Attribute %d, Value %p, BufferLength %d, StringLength %p)\n", ConnectionHandle,
          Attribute, Value, BufferLength, StringLength);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        ret = get_connect_attr_unix_a( handle, Attribute, Value, BufferLength, StringLength );
    }
    else if (handle->win32_handle)
    {
        ret = get_connect_attr_win32_a( handle, Attribute, Value, BufferLength, StringLength );
    }
    else
    {
        switch (Attribute)
        {
        case SQL_ATTR_CONNECTION_TIMEOUT:
            *(SQLINTEGER *)Value = handle->con_attr_con_timeout;
            break;

        case SQL_ATTR_LOGIN_TIMEOUT:
            *(SQLINTEGER *)Value = handle->con_attr_login_timeout;
            break;

        default:
            FIXME( "unhandled attribute %d\n", Attribute );
            ret = SQL_ERROR;
            break;
        }
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

static SQLRETURN get_connect_option_unix_a( struct handle *handle, SQLUSMALLINT option, SQLPOINTER value )
{
    struct SQLGetConnectOption_params params = { handle->unix_handle, option, value };
    return ODBC_CALL( SQLGetConnectOption, &params );
}

static SQLRETURN get_connect_option_win32_a( struct handle *handle, SQLUSMALLINT option, SQLPOINTER value )
{
    SQLRETURN ret = SQL_ERROR;

    if (handle->win32_funcs->SQLGetConnectOption)
        return handle->win32_funcs->SQLGetConnectOption( handle->win32_handle, option, value );

    if (handle->win32_funcs->SQLGetConnectOptionW)
    {
        switch (option)
        {
        case SQL_ATTR_CURRENT_CATALOG:
        case SQL_ATTR_TRACEFILE:
        case SQL_ATTR_TRANSLATE_LIB:
            FIXME( "option %u not handled\n", option );
            return SQL_ERROR;
        default: break;
        }
        ret = handle->win32_funcs->SQLGetConnectOptionW( handle->win32_handle, option, value );
    }
    return ret;
}

/*************************************************************************
 *				SQLGetConnectOption       [ODBC32.042]
 */
SQLRETURN WINAPI SQLGetConnectOption(SQLHDBC ConnectionHandle, SQLUSMALLINT Option, SQLPOINTER Value)
{
    struct handle *handle = ConnectionHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(ConnectionHandle %p, Option %d, Value %p)\n", ConnectionHandle, Option, Value);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        ret = get_connect_option_unix_a( handle, Option, Value );
    }
    else if (handle->win32_handle)
    {
        ret = get_connect_option_win32_a( handle, Option, Value );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

static SQLRETURN get_cursor_name_unix_a( struct handle *handle, SQLCHAR *name, SQLSMALLINT buflen,
                                         SQLSMALLINT *retlen )
{
    struct SQLGetCursorName_params params = { handle->unix_handle, name, buflen, retlen };
    return ODBC_CALL( SQLGetCursorName, &params );
}

static SQLRETURN get_cursor_name_win32_a( struct handle *handle, SQLCHAR *name, SQLSMALLINT buflen,
                                          SQLSMALLINT *retlen )
{
   SQLRETURN ret = SQL_ERROR;

    if (handle->win32_funcs->SQLGetCursorName)
        return handle->win32_funcs->SQLGetCursorName( handle->win32_handle, name, buflen, retlen );

    if (handle->win32_funcs->SQLGetCursorNameW)
    {
        SQLWCHAR *nameW;
        SQLSMALLINT lenW;

        if (!(nameW = malloc( buflen * sizeof(WCHAR) ))) return SQL_ERROR;
        ret = handle->win32_funcs->SQLGetCursorNameW( handle->win32_handle, nameW, buflen, &lenW );
        if (SUCCESS( ret ))
        {
            int len = WideCharToMultiByte( CP_ACP, 0, nameW, -1, (char *)name, buflen, NULL, NULL );
            if (retlen) *retlen = len - 1;
        }
        free( nameW );
    }
    return ret;
}

/*************************************************************************
 *				SQLGetCursorName           [ODBC32.017]
 */
SQLRETURN WINAPI SQLGetCursorName(SQLHSTMT StatementHandle, SQLCHAR *CursorName, SQLSMALLINT BufferLength,
                                  SQLSMALLINT *NameLength)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, CursorName %p, BufferLength %d, NameLength %p)\n", StatementHandle, CursorName,
          BufferLength, NameLength);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        ret = get_cursor_name_unix_a( handle, CursorName, BufferLength, NameLength );
    }
    else if (handle->win32_handle)
    {
        ret = get_cursor_name_win32_a( handle, CursorName, BufferLength, NameLength );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetData           [ODBC32.043]
 */
SQLRETURN WINAPI SQLGetData(SQLHSTMT StatementHandle, SQLUSMALLINT ColumnNumber, SQLSMALLINT TargetType,
                            SQLPOINTER TargetValue, SQLLEN BufferLength, SQLLEN *StrLen_or_Ind)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, ColumnNumber %d, TargetType %d, TargetValue %p, BufferLength %s, StrLen_or_Ind %p)\n",
          StatementHandle, ColumnNumber, TargetType, TargetValue, debugstr_sqllen(BufferLength), StrLen_or_Ind);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        INT64 len;
        struct SQLGetData_params params = { handle->unix_handle, ColumnNumber, TargetType, TargetValue,
                                            BufferLength, &len };
        if (SUCCESS((ret = ODBC_CALL( SQLGetData, &params )))) *StrLen_or_Ind = len;
    }
    else if (handle->win32_handle)
    {
        ret = handle->win32_funcs->SQLGetData( handle->win32_handle, ColumnNumber, TargetType, TargetValue,
                                               BufferLength, StrLen_or_Ind );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

static SQLRETURN get_desc_field_unix_a( struct handle *handle, SQLSMALLINT record, SQLSMALLINT field_id,
                                        SQLPOINTER value, SQLINTEGER buflen, SQLINTEGER *retlen )
{
    struct SQLGetDescField_params params = { handle->unix_handle, record, field_id, value, buflen, retlen };
    return ODBC_CALL( SQLGetDescField, &params );
}

static SQLRETURN get_desc_field_win32_a( struct handle *handle, SQLSMALLINT record, SQLSMALLINT field_id,
                                         SQLPOINTER value, SQLINTEGER buflen, SQLINTEGER *retlen )
{
    SQLRETURN ret = SQL_ERROR;
    SQLPOINTER val = value;
    SQLWCHAR *strW = NULL;

    if (handle->win32_funcs->SQLGetDescField)
        return handle->win32_funcs->SQLGetDescField( handle->win32_handle, record, field_id, value, buflen, retlen );

    if (handle->win32_funcs->SQLGetDescFieldW)
    {
        switch (field_id)
        {
        case SQL_DESC_BASE_COLUMN_NAME:
        case SQL_DESC_BASE_TABLE_NAME:
        case SQL_DESC_CATALOG_NAME:
        case SQL_DESC_LABEL:
        case SQL_DESC_LITERAL_PREFIX:
        case SQL_DESC_LITERAL_SUFFIX:
        case SQL_DESC_LOCAL_TYPE_NAME:
        case SQL_DESC_NAME:
        case SQL_DESC_SCHEMA_NAME:
        case SQL_DESC_TABLE_NAME:
        case SQL_DESC_TYPE_NAME:
            if (!(val = strW = malloc( buflen * sizeof(WCHAR) ))) return SQL_ERROR;
        default: break;
        }

        ret = handle->win32_funcs->SQLGetDescFieldW( handle->win32_handle, record, field_id, value, buflen, retlen );
        if (SUCCESS( ret ) && strW)
        {
            int len = WideCharToMultiByte( CP_ACP, 0, strW, -1, (char *)value, buflen, NULL, NULL );
            if (retlen) *retlen = len - 1;
        }
        free( strW );
    }
    return ret;
}

/*************************************************************************
 *				SQLGetDescField           [ODBC32.033]
 */
SQLRETURN WINAPI SQLGetDescField(SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber, SQLSMALLINT FieldIdentifier,
                                 SQLPOINTER Value, SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
    struct handle *handle = DescriptorHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(DescriptorHandle %p, RecNumber %d, FieldIdentifier %d, Value %p, BufferLength %d, StringLength %p)\n",
          DescriptorHandle, RecNumber, FieldIdentifier, Value, BufferLength, StringLength);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        ret = get_desc_field_unix_a( handle, RecNumber, FieldIdentifier, Value, BufferLength, StringLength );
    }
    else if (handle->win32_handle)
    {
        ret = get_desc_field_win32_a( handle, RecNumber, FieldIdentifier, Value, BufferLength, StringLength );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

static SQLRETURN get_desc_rec_unix_a( struct handle *handle, SQLSMALLINT record, SQLCHAR *name, SQLSMALLINT buflen,
                                      SQLSMALLINT *retlen, SQLSMALLINT *type, SQLSMALLINT *subtype, SQLLEN *len,
                                      SQLSMALLINT *precision, SQLSMALLINT *scale, SQLSMALLINT *nullable )
{
    SQLRETURN ret;
    INT64 len64;
    struct SQLGetDescRec_params params = { handle->unix_handle, record, name, buflen, retlen, type, subtype, &len64,
                                           precision, scale, nullable };
    if (SUCCESS((ret = ODBC_CALL( SQLGetDescRec, &params )))) *len = len64;
    return ret;
}

static SQLRETURN get_desc_rec_win32_a( struct handle *handle, SQLSMALLINT record, SQLCHAR *name, SQLSMALLINT buflen,
                                       SQLSMALLINT *retlen, SQLSMALLINT *type, SQLSMALLINT *subtype, SQLLEN *len,
                                       SQLSMALLINT *precision, SQLSMALLINT *scale, SQLSMALLINT *nullable )
{
    SQLRETURN ret = SQL_ERROR;

    if (handle->win32_funcs->SQLGetDescRec)
        return handle->win32_funcs->SQLGetDescRec( handle->win32_handle, record, name, buflen, retlen, type, subtype,
                                                   len, precision, scale, nullable );

    if (handle->win32_funcs->SQLGetDescRecW)
    {
        SQLWCHAR *nameW;

        if (!(nameW = malloc( buflen * sizeof(WCHAR) ))) return SQL_ERROR;
        ret = handle->win32_funcs->SQLGetDescRecW( handle->win32_handle, record, nameW, buflen, retlen, type, subtype,
                                                   len, precision, scale, nullable );
        if (SUCCESS( ret ))
        {
            int len = WideCharToMultiByte( CP_ACP, 0, nameW, -1, (char *)name, buflen, NULL, NULL );
            if (retlen) *retlen = len - 1;
        }
        free( nameW );
    }
    return ret;
}

/*************************************************************************
 *				SQLGetDescRec           [ODBC32.034]
 */
SQLRETURN WINAPI SQLGetDescRec(SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber, SQLCHAR *Name,
                               SQLSMALLINT BufferLength, SQLSMALLINT *StringLength, SQLSMALLINT *Type,
                               SQLSMALLINT *SubType, SQLLEN *Length, SQLSMALLINT *Precision,
                               SQLSMALLINT *Scale, SQLSMALLINT *Nullable)
{
    struct handle *handle = DescriptorHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(DescriptorHandle %p, RecNumber %d, Name %p, BufferLength %d, StringLength %p, Type %p, SubType %p,"
          " Length %p, Precision %p, Scale %p, Nullable %p)\n", DescriptorHandle, RecNumber, Name, BufferLength,
          StringLength, Type, SubType, Length, Precision, Scale, Nullable);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        ret = get_desc_rec_unix_a( handle, RecNumber, Name, BufferLength, StringLength, Type, SubType, Length,
                                   Precision, Scale, Nullable );
    }
    else if (handle->win32_handle)
    {
        ret = get_desc_rec_win32_a( handle, RecNumber, Name, BufferLength, StringLength, Type, SubType, Length,
                                    Precision, Scale, Nullable );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

static SQLRETURN get_diag_field_unix_a( SQLSMALLINT handle_type, struct handle *handle, SQLSMALLINT rec_num,
                                        SQLSMALLINT diag_id, SQLPOINTER diag_info, SQLSMALLINT buflen,
                                        SQLSMALLINT *retlen )
{

    struct SQLGetDiagField_params params = { handle_type, handle->unix_handle, rec_num, diag_id, diag_info, buflen,
                                             retlen };
    return ODBC_CALL( SQLGetDiagField, &params );
}

static SQLRETURN get_diag_field_win32_a( SQLSMALLINT handle_type, struct handle *handle, SQLSMALLINT rec_num,
                                         SQLSMALLINT diag_id, SQLPOINTER diag_info, SQLSMALLINT buflen,
                                         SQLSMALLINT *retlen )
{
    SQLRETURN ret = SQL_ERROR;

    if (handle->win32_funcs->SQLGetDiagField)
        return handle->win32_funcs->SQLGetDiagField( handle_type, handle->win32_handle, rec_num, diag_id, diag_info,
                                                     buflen, retlen );

    if (handle->win32_funcs->SQLGetDiagFieldW)
    {
        SQLWCHAR *strW;
        SQLSMALLINT lenW;

        if (buflen < 0)
            ret = handle->win32_funcs->SQLGetDiagFieldW( handle_type, handle->win32_handle, rec_num, diag_id,
                                                         diag_info, buflen, retlen );
        else
        {
            if (!(strW = malloc( buflen * sizeof(WCHAR) ))) return SQL_ERROR;
            ret = handle->win32_funcs->SQLGetDiagFieldW( handle_type, handle->win32_handle, rec_num, diag_id, strW,
                                                         buflen, &lenW );
            if (SUCCESS( ret ))
            {
                int len = WideCharToMultiByte( CP_ACP, 0, strW, -1, (char *)diag_info, buflen, NULL, NULL );
                if (retlen) *retlen = len - 1;
            }
            free( strW );
        }
    }
    return ret;
}

/*************************************************************************
 *				SQLGetDiagField           [ODBC32.035]
 */
SQLRETURN WINAPI SQLGetDiagField(SQLSMALLINT HandleType, SQLHANDLE Handle, SQLSMALLINT RecNumber,
                                 SQLSMALLINT DiagIdentifier, SQLPOINTER DiagInfo, SQLSMALLINT BufferLength,
                                 SQLSMALLINT *StringLength)
{
    struct handle *handle = Handle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(HandleType %d, Handle %p, RecNumber %d, DiagIdentifier %d, DiagInfo %p, BufferLength %d,"
          " StringLength %p)\n", HandleType, Handle, RecNumber, DiagIdentifier, DiagInfo, BufferLength, StringLength);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        ret = get_diag_field_unix_a( HandleType, handle, RecNumber, DiagIdentifier, DiagInfo, BufferLength,
                                     StringLength );
    }
    else if (handle->win32_handle)
    {
        ret = get_diag_field_win32_a( HandleType, handle, RecNumber, DiagIdentifier, DiagInfo, BufferLength,
                                      StringLength );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

static SQLRETURN get_diag_rec_unix_a( SQLSMALLINT handle_type, struct handle *handle, SQLSMALLINT rec_num,
                                      SQLCHAR *state, SQLINTEGER *native_err, SQLCHAR *msg, SQLSMALLINT buflen,
                                      SQLSMALLINT *retlen )
{
    struct SQLGetDiagRec_params params = { handle_type, handle->unix_handle, rec_num, state, native_err, msg,
                                           buflen, retlen };
    return ODBC_CALL( SQLGetDiagRec, &params );
}

static SQLRETURN get_diag_rec_win32_a( SQLSMALLINT handle_type, struct handle *handle, SQLSMALLINT rec_num,
                                       SQLCHAR *state, SQLINTEGER *native_err, SQLCHAR *msg, SQLSMALLINT buflen,
                                       SQLSMALLINT *retlen )
{
    SQLRETURN ret = SQL_ERROR;
    SQLWCHAR stateW[6], *msgW;
    SQLSMALLINT lenW;

    if (handle->win32_funcs->SQLGetDiagRec)
        return handle->win32_funcs->SQLGetDiagRec( handle_type, handle->win32_handle, rec_num, state, native_err,
                                                   msg, buflen, retlen );

    if (handle->win32_funcs->SQLGetDiagRecW)
    {
        if (!(msgW = malloc( buflen * sizeof(WCHAR) ))) return SQL_ERROR;
        ret = handle->win32_funcs->SQLGetDiagRecW( handle_type, handle->win32_handle, rec_num, stateW, native_err,
                                                   msgW, buflen, &lenW );
        if (SUCCESS( ret ))
        {
            int len = WideCharToMultiByte( CP_ACP, 0, msgW, -1, (char *)msg, buflen, NULL, NULL );
            if (retlen) *retlen = len - 1;
            WideCharToMultiByte( CP_ACP, 0, stateW, -1, (char *)state, 6, NULL, NULL );
        }
        free( msgW );
    }
    return ret;
}

/*************************************************************************
 *				SQLGetDiagRec           [ODBC32.036]
 */
SQLRETURN WINAPI SQLGetDiagRec(SQLSMALLINT HandleType, SQLHANDLE Handle, SQLSMALLINT RecNumber,
                               SQLCHAR *SqlState, SQLINTEGER *NativeError, SQLCHAR *MessageText,
                               SQLSMALLINT BufferLength, SQLSMALLINT *TextLength)
{
    struct handle *handle = Handle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(HandleType %d, Handle %p, RecNumber %d, SqlState %p, NativeError %p, MessageText %p, BufferLength %d,"
          " TextLength %p)\n", HandleType, Handle, RecNumber, SqlState, NativeError, MessageText, BufferLength,
          TextLength);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        ret = get_diag_rec_unix_a( HandleType, handle, RecNumber, SqlState, NativeError, MessageText, BufferLength,
                                   TextLength );
    }
    else if (handle->win32_handle)
    {
        ret = get_diag_rec_win32_a( HandleType, handle, RecNumber, SqlState, NativeError, MessageText, BufferLength,
                                    TextLength );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetEnvAttr           [ODBC32.037]
 */
SQLRETURN WINAPI SQLGetEnvAttr(SQLHENV EnvironmentHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                               SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
    struct handle *handle = EnvironmentHandle;
    SQLRETURN ret = SQL_SUCCESS;

    TRACE("(EnvironmentHandle %p, Attribute %d, Value %p, BufferLength %d, StringLength %p)\n",
          EnvironmentHandle, Attribute, Value, BufferLength, StringLength);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        struct SQLGetEnvAttr_params params = { handle->unix_handle, Attribute, Value, BufferLength, StringLength };
        ret = ODBC_CALL( SQLGetEnvAttr, &params );
    }
    else if (handle->win32_handle)
    {
        ret = handle->win32_funcs->SQLGetEnvAttr( handle->win32_handle, Attribute, Value, BufferLength, StringLength );
    }
    else
    {
        switch (Attribute)
        {
        case SQL_ATTR_CONNECTION_POOLING:
            *(SQLINTEGER *)Value = SQL_CP_OFF;
            break;

        case SQL_ATTR_ODBC_VERSION:
            *(SQLINTEGER *)Value = handle->env_attr_version;
            break;

        default:
            FIXME( "unhandled attribute %d\n", Attribute );
            ret = SQL_ERROR;
            break;
        }
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetFunctions           [ODBC32.044]
 */
SQLRETURN WINAPI SQLGetFunctions(SQLHDBC ConnectionHandle, SQLUSMALLINT FunctionId, SQLUSMALLINT *Supported)
{
    struct handle *handle = ConnectionHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(ConnectionHandle %p, FunctionId %d, Supported %p)\n", ConnectionHandle, FunctionId, Supported);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        struct SQLGetFunctions_params params = { handle->unix_handle, FunctionId, Supported };
        ret = ODBC_CALL( SQLGetFunctions, &params );
    }
    else if (handle->win32_handle)
    {
        ret = handle->win32_funcs->SQLGetFunctions( handle->win32_handle, FunctionId, Supported );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

static SQLRETURN get_info_unix_a( struct handle *handle, SQLUSMALLINT type, SQLPOINTER value, SQLSMALLINT buflen,
                                  SQLSMALLINT *retlen )
{
    struct SQLGetInfo_params params = { handle->unix_handle, type, value, buflen, retlen };
    return ODBC_CALL( SQLGetInfo, &params );
}

static SQLRETURN get_info_win32_a( struct handle *handle, SQLUSMALLINT type, SQLPOINTER value, SQLSMALLINT buflen,
                                   SQLSMALLINT *retlen )
{
    SQLRETURN ret = SQL_ERROR;
    WCHAR *strW = NULL;
    SQLPOINTER buf = value;

    if (handle->win32_funcs->SQLGetInfo)
        return handle->win32_funcs->SQLGetInfo( handle->win32_handle, type, value, buflen, retlen );

    if (handle->win32_funcs->SQLGetInfoW)
    {
        switch (type)
        {
        case SQL_ACCESSIBLE_PROCEDURES:
        case SQL_ACCESSIBLE_TABLES:
        case SQL_CATALOG_NAME:
        case SQL_CATALOG_NAME_SEPARATOR:
        case SQL_CATALOG_TERM:
        case SQL_COLLATION_SEQ:
        case SQL_COLUMN_ALIAS:
        case SQL_DATA_SOURCE_NAME:
        case SQL_DATA_SOURCE_READ_ONLY:
        case SQL_DATABASE_NAME:
        case SQL_DBMS_NAME:
        case SQL_DBMS_VER:
        case SQL_DESCRIBE_PARAMETER:
        case SQL_DM_VER:
        case SQL_DRIVER_NAME:
        case SQL_DRIVER_ODBC_VER:
        case SQL_DRIVER_VER:
        case SQL_EXPRESSIONS_IN_ORDERBY:
        case SQL_IDENTIFIER_QUOTE_CHAR:
        case SQL_INTEGRITY:
        case SQL_KEYWORDS:
        case SQL_LIKE_ESCAPE_CLAUSE:
        case SQL_MAX_ROW_SIZE_INCLUDES_LONG:
        case SQL_MULT_RESULT_SETS:
        case SQL_MULTIPLE_ACTIVE_TXN:
        case SQL_NEED_LONG_DATA_LEN:
        case SQL_ODBC_VER:
        case SQL_ORDER_BY_COLUMNS_IN_SELECT:
        case SQL_PROCEDURE_TERM:
        case SQL_PROCEDURES:
        case SQL_ROW_UPDATES:
        case SQL_SCHEMA_TERM:
        case SQL_SEARCH_PATTERN_ESCAPE:
        case SQL_SERVER_NAME:
        case SQL_SPECIAL_CHARACTERS:
        case SQL_TABLE_TERM:
        case SQL_USER_NAME:
        case SQL_XOPEN_CLI_YEAR:
            if (!(strW = malloc( buflen * sizeof(WCHAR) ))) return SQL_ERROR;
            buf = strW;
            break;

        default: break;
        }

        ret = SQLGetInfoW( handle->win32_handle, type, buf, buflen, retlen );
        if (SUCCESS( ret ) && strW)
        {
            int len = WideCharToMultiByte( CP_ACP, 0, strW, -1, (char *)value, buflen, NULL, NULL );
            if (retlen) *retlen = len - 1;
        }
        free( strW );
    }
    return ret;
}

/*************************************************************************
 *				SQLGetInfo           [ODBC32.045]
 */
SQLRETURN WINAPI SQLGetInfo(SQLHDBC ConnectionHandle, SQLUSMALLINT InfoType, SQLPOINTER InfoValue,
                            SQLSMALLINT BufferLength, SQLSMALLINT *StringLength)
{
    struct handle *handle = ConnectionHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(ConnectionHandle, %p, InfoType %d, InfoValue %p, BufferLength %d, StringLength %p)\n", ConnectionHandle,
          InfoType, InfoValue, BufferLength, StringLength);

    if (!handle) return SQL_INVALID_HANDLE;

    switch (InfoType)
    {
    case SQL_ODBC_VER:
    {
        const char version[] = "03.80.0000";
        int len = sizeof(version);
        char *value = InfoValue;

        if (StringLength) *StringLength = len;
        if (value && BufferLength >= len)
        {
            strcpy( value, version );
            if (StringLength) *StringLength = len - 1;
        }
        return SQL_SUCCESS;
    }
    default: break;
    }

    if (handle->unix_handle)
    {
        ret = get_info_unix_a( handle, InfoType, InfoValue, BufferLength, StringLength );
    }
    else if (handle->win32_handle)
    {
        ret = get_info_win32_a( handle, InfoType, InfoValue, BufferLength, StringLength );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

static SQLRETURN get_stmt_attr_unix_a( struct handle *handle, SQLINTEGER attr, SQLPOINTER value, SQLINTEGER buflen,
                                       SQLINTEGER *retlen )
{
    struct SQLGetStmtAttr_params params = { handle->unix_handle, attr, value, buflen, retlen };
    return ODBC_CALL( SQLGetStmtAttr, &params );
}

static SQLRETURN get_stmt_attr_win32_a( struct handle *handle, SQLINTEGER attr, SQLPOINTER value, SQLINTEGER buflen,
                                        SQLINTEGER *retlen )
{
    if (handle->win32_funcs->SQLGetStmtAttr)
        return handle->win32_funcs->SQLGetStmtAttr( handle->win32_handle, attr, value, buflen, retlen );

    if (handle->win32_funcs->SQLGetStmtAttrW)
    {
        SQLRETURN ret;
        WCHAR *strW;

        if (buflen == SQL_IS_POINTER || buflen < SQL_LEN_BINARY_ATTR_OFFSET)
            return handle->win32_funcs->SQLGetStmtAttrW( handle->win32_handle, attr, value, buflen, retlen );

        if (!(strW = malloc( buflen * sizeof(WCHAR) ))) return SQL_ERROR;
        ret = handle->win32_funcs->SQLGetStmtAttrW( handle->win32_handle, attr, strW, buflen, retlen );
        if (SUCCESS( ret ))
        {
            int len = WideCharToMultiByte( CP_ACP, 0, strW, -1, (char *)value, buflen, NULL, NULL );
            if (retlen) *retlen = len - 1;
        }
        free( strW );
    }

    return SQL_ERROR;
}

/*************************************************************************
 *				SQLGetStmtAttr           [ODBC32.038]
 */
SQLRETURN WINAPI SQLGetStmtAttr(SQLHSTMT StatementHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, Attribute %d, Value %p, BufferLength %d, StringLength %p)\n", StatementHandle,
          Attribute, Value, BufferLength, StringLength);

    if (!Value)
    {
        WARN("Unexpected NULL Value return address\n");
        return SQL_ERROR;
    }

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        ret = get_stmt_attr_unix_a( handle, Attribute, Value, BufferLength, StringLength );
    }
    else if (handle->win32_handle)
    {
        ret = get_stmt_attr_win32_a( handle, Attribute, Value, BufferLength, StringLength );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetStmtOption           [ODBC32.046]
 */
SQLRETURN WINAPI SQLGetStmtOption(SQLHSTMT StatementHandle, SQLUSMALLINT Option, SQLPOINTER Value)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, Option %d, Value %p)\n", StatementHandle, Option, Value);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        struct SQLGetStmtOption_params params = { handle->unix_handle, Option, Value };
        ret = ODBC_CALL( SQLGetStmtOption, &params );
    }
    else if (handle->win32_handle)
    {
        ret = handle->win32_funcs->SQLGetStmtOption( handle->win32_handle, Option, Value );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

static SQLRETURN get_type_info_unix_a( struct handle *handle, SQLSMALLINT type )
{
    struct SQLGetTypeInfo_params params = { handle->unix_handle, type };
    return ODBC_CALL( SQLGetTypeInfo, &params );
}

static SQLRETURN get_type_info_win32_a( struct handle *handle, SQLSMALLINT type )
{
    if (handle->win32_funcs->SQLGetTypeInfo)
        return handle->win32_funcs->SQLGetTypeInfo( handle->win32_handle, type );
    if (handle->win32_funcs->SQLGetTypeInfoW)
        return handle->win32_funcs->SQLGetTypeInfoW( handle->win32_handle, type );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLGetTypeInfo           [ODBC32.047]
 */
SQLRETURN WINAPI SQLGetTypeInfo(SQLHSTMT StatementHandle, SQLSMALLINT DataType)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, DataType %d)\n", StatementHandle, DataType);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        ret = get_type_info_unix_a( handle, DataType );
    }
    else if (handle->win32_handle)
    {
        ret = get_type_info_win32_a( handle, DataType );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLNumResultCols           [ODBC32.018]
 */
SQLRETURN WINAPI SQLNumResultCols(SQLHSTMT StatementHandle, SQLSMALLINT *ColumnCount)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, ColumnCount %p)\n", StatementHandle, ColumnCount);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        struct SQLNumResultCols_params params = { handle->unix_handle, ColumnCount };
        ret = ODBC_CALL( SQLNumResultCols, &params );
    }
    else if (handle->win32_handle)
    {
        ret = handle->win32_funcs->SQLNumResultCols( handle->win32_handle, ColumnCount );
    }

    TRACE("Returning %d ColumnCount %d\n", ret, *ColumnCount);
    return ret;
}

/*************************************************************************
 *				SQLParamData           [ODBC32.048]
 */
SQLRETURN WINAPI SQLParamData(SQLHSTMT StatementHandle, SQLPOINTER *Value)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, Value %p)\n", StatementHandle, Value);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        struct SQLParamData_params params = { handle->unix_handle, Value };
        ret = ODBC_CALL( SQLParamData, &params );
    }
    else if (handle->win32_handle)
    {
        ret = handle->win32_funcs->SQLParamData( handle->win32_handle, Value );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

static SQLRETURN prepare_unix_a( struct handle *handle, SQLCHAR *statement, SQLINTEGER len )
{
    struct SQLPrepare_params params = { handle->unix_handle, statement, len };
    return ODBC_CALL( SQLPrepare, &params );
}

static SQLRETURN prepare_win32_a( struct handle *handle, SQLCHAR *statement, SQLINTEGER len )
{
    SQLRETURN ret = SQL_ERROR;

    if (handle->win32_funcs->SQLPrepare)
        return handle->win32_funcs->SQLPrepare( handle->win32_handle, statement, len );

    if (handle->win32_funcs->SQLPrepareW)
    {
        WCHAR *strW;
        if (!(strW = strnAtoW( statement, len ))) return SQL_ERROR;
        ret = handle->win32_funcs->SQLPrepareW( handle->win32_handle, strW, len );
        free( strW );
    }

    return ret;
}

/*************************************************************************
 *				SQLPrepare           [ODBC32.019]
 */
SQLRETURN WINAPI SQLPrepare(SQLHSTMT StatementHandle, SQLCHAR *StatementText, SQLINTEGER TextLength)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, StatementText %s, TextLength %d)\n", StatementHandle,
          debugstr_sqlstr(StatementText, TextLength), TextLength);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        ret = prepare_unix_a( handle, StatementText, TextLength );
    }
    else if (handle->win32_handle)
    {
        ret = prepare_win32_a( handle, StatementText, TextLength );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLPutData           [ODBC32.049]
 */
SQLRETURN WINAPI SQLPutData(SQLHSTMT StatementHandle, SQLPOINTER Data, SQLLEN StrLen_or_Ind)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, Data %p, StrLen_or_Ind %s)\n", StatementHandle, Data, debugstr_sqllen(StrLen_or_Ind));

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        struct SQLPutData_params params = { handle->unix_handle, Data, StrLen_or_Ind };
        ret = ODBC_CALL( SQLPutData, &params );
    }
    else if (handle->win32_handle)
    {
        ret = handle->win32_funcs->SQLPutData( handle->win32_handle, Data, StrLen_or_Ind );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLRowCount           [ODBC32.020]
 */
SQLRETURN WINAPI SQLRowCount(SQLHSTMT StatementHandle, SQLLEN *RowCount)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, RowCount %p)\n", StatementHandle, RowCount);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        INT64 count;
        struct SQLRowCount_params params = { handle->unix_handle, &count };
        if (SUCCESS((ret = ODBC_CALL( SQLRowCount, &params ))) && RowCount)
        {
            *RowCount = count;
            TRACE(" RowCount %s\n", debugstr_sqllen(*RowCount));
        }
    }
    else if (handle->win32_handle)
    {
        ret = handle->win32_funcs->SQLRowCount( handle->win32_handle, RowCount );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetConnectAttr           [ODBC32.039]
 */
SQLRETURN WINAPI SQLSetConnectAttr(SQLHDBC ConnectionHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                   SQLINTEGER StringLength)
{
    struct handle *handle = ConnectionHandle;
    SQLRETURN ret = SQL_SUCCESS;

    TRACE("(ConnectionHandle %p, Attribute %d, Value %p, StringLength %d)\n", ConnectionHandle, Attribute, Value,
          StringLength);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        struct SQLSetConnectAttr_params params = { handle->unix_handle, Attribute, Value, StringLength };
        ret = ODBC_CALL( SQLSetConnectAttr, &params );
    }
    else if (handle->win32_handle)
    {
        ret = handle->win32_funcs->SQLSetConnectAttr( handle->win32_handle, Attribute, Value, StringLength );
    }
    else
    {
        switch (Attribute)
        {
        case SQL_ATTR_CONNECTION_TIMEOUT:
            handle->con_attr_con_timeout = (UINT32)(ULONG_PTR)Value;
            break;

        case SQL_ATTR_LOGIN_TIMEOUT:
            handle->con_attr_login_timeout = (UINT32)(ULONG_PTR)Value;
            break;

        default:
            FIXME( "unhandled attribute %d\n", Attribute );
            ret = SQL_ERROR;
            break;
        }
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetConnectOption           [ODBC32.050]
 */
SQLRETURN WINAPI SQLSetConnectOption(SQLHDBC ConnectionHandle, SQLUSMALLINT Option, SQLULEN Value)
{
    struct handle *handle = ConnectionHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(ConnectionHandle %p, Option %d, Value %s)\n", ConnectionHandle, Option, debugstr_sqlulen(Value));

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        struct SQLSetConnectOption_params params = { handle->unix_handle, Option, Value };
        ret = ODBC_CALL( SQLSetConnectOption, &params );
    }
    else if (handle->win32_handle)
    {
        ret = handle->win32_funcs->SQLSetConnectOption( handle->win32_handle, Option, Value );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetCursorName           [ODBC32.021]
 */
SQLRETURN WINAPI SQLSetCursorName(SQLHSTMT StatementHandle, SQLCHAR *CursorName, SQLSMALLINT NameLength)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, CursorName %s, NameLength %d)\n", StatementHandle,
          debugstr_sqlstr(CursorName, NameLength), NameLength);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        struct SQLSetCursorName_params params = { handle->unix_handle, CursorName, NameLength };
        ret = ODBC_CALL( SQLSetCursorName, &params );
    }
    else if (handle->win32_handle)
    {
        ret = handle->win32_funcs->SQLSetCursorName( handle->win32_handle, CursorName, NameLength );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetDescField           [ODBC32.073]
 */
SQLRETURN WINAPI SQLSetDescField(SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber, SQLSMALLINT FieldIdentifier,
                                 SQLPOINTER Value, SQLINTEGER BufferLength)
{
    struct handle *handle = DescriptorHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(DescriptorHandle %p, RecNumber %d, FieldIdentifier %d, Value %p, BufferLength %d)\n", DescriptorHandle,
          RecNumber, FieldIdentifier, Value, BufferLength);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        struct SQLSetDescField_params params = { handle->unix_handle, RecNumber, FieldIdentifier, Value,
                                                 BufferLength };
        ret = ODBC_CALL( SQLSetDescField, &params );
    }
    else if (handle->win32_handle)
    {
        ret = handle->win32_funcs->SQLSetDescField( handle->win32_handle, RecNumber, FieldIdentifier, Value,
                                                    BufferLength );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetDescRec           [ODBC32.074]
 */
SQLRETURN WINAPI SQLSetDescRec(SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber, SQLSMALLINT Type,
                               SQLSMALLINT SubType, SQLLEN Length, SQLSMALLINT Precision, SQLSMALLINT Scale,
                               SQLPOINTER Data, SQLLEN *StringLength, SQLLEN *Indicator)
{
    struct handle *handle = DescriptorHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(DescriptorHandle %p, RecNumber %d, Type %d, SubType %d, Length %s, Precision %d, Scale %d, Data %p,"
          " StringLength %p, Indicator %p)\n", DescriptorHandle, RecNumber, Type, SubType, debugstr_sqllen(Length),
          Precision, Scale, Data, StringLength, Indicator);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        INT64 stringlen, indicator;
        struct SQLSetDescRec_params params = { handle->unix_handle, RecNumber, Type, SubType, Length, Precision,
                                               Scale, Data, &stringlen, &indicator };
        if (SUCCESS((ret = ODBC_CALL( SQLSetDescRec, &params ))))
        {
            *StringLength = stringlen;
            *Indicator = indicator;
        }
    }
    else if (handle->win32_handle)
    {
        ret = handle->win32_funcs->SQLSetDescRec( handle->win32_handle, RecNumber, Type, SubType, Length, Precision,
                                                  Scale, Data, StringLength, Indicator );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetEnvAttr           [ODBC32.075]
 */
SQLRETURN WINAPI SQLSetEnvAttr(SQLHENV EnvironmentHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                               SQLINTEGER StringLength)
{
    struct handle *handle = EnvironmentHandle;
    SQLRETURN ret = SQL_SUCCESS;

    TRACE("(EnvironmentHandle %p, Attribute %d, Value %p, StringLength %d)\n", EnvironmentHandle, Attribute, Value,
          StringLength);

    if (handle->unix_handle)
    {
        struct SQLSetEnvAttr_params params = { handle->unix_handle, Attribute, Value, StringLength };
        ret = ODBC_CALL( SQLSetEnvAttr, &params );
    }
    else if (handle->win32_handle)
    {
        ret = handle->win32_funcs->SQLSetEnvAttr( handle->win32_handle, Attribute, Value, StringLength );
    }
    else
    {
        switch (Attribute)
        {
        case SQL_ATTR_ODBC_VERSION:
            handle->env_attr_version = (UINT32)(ULONG_PTR)Value;
            break;

        case SQL_ATTR_CONNECTION_POOLING:
            FIXME("Ignore Pooling value\n");
            break;

        default:
            FIXME( "unhandled attribute %d\n", Attribute );
            ret = SQL_ERROR;
            break;
        }
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetParam           [ODBC32.022]
 */
SQLRETURN WINAPI SQLSetParam(SQLHSTMT StatementHandle, SQLUSMALLINT ParameterNumber, SQLSMALLINT ValueType,
                             SQLSMALLINT ParameterType, SQLULEN LengthPrecision, SQLSMALLINT ParameterScale,
                             SQLPOINTER ParameterValue, SQLLEN *StrLen_or_Ind)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, ParameterNumber %d, ValueType %d, ParameterType %d, LengthPrecision %s,"
          " ParameterScale %d, ParameterValue %p, StrLen_or_Ind %p)\n", StatementHandle, ParameterNumber, ValueType,
          ParameterType, debugstr_sqlulen(LengthPrecision), ParameterScale, ParameterValue, StrLen_or_Ind);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        INT64 len;
        struct SQLSetParam_params params = { handle->unix_handle, ParameterNumber, ValueType, ParameterType,
                                             LengthPrecision, ParameterScale, ParameterValue, &len };
        if (SUCCESS((ret = ODBC_CALL( SQLSetParam, &params )))) *StrLen_or_Ind = len;
    }
    else if (handle->win32_handle)
    {
        ret = handle->win32_funcs->SQLSetParam( handle->win32_handle, ParameterNumber, ValueType, ParameterType,
                                                LengthPrecision, ParameterScale, ParameterValue, StrLen_or_Ind );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

static BOOL resize_result_lengths( struct handle *handle, UINT size )
{
    UINT i;
    for (i = 0; i < handle->bind_col.count; i++)
    {
        UINT8 *tmp;
        if (!handle->bind_col.param[i].ptr) continue;
        if (!(tmp = realloc( handle->bind_col.param[i].len, size * sizeof(UINT64) ))) return FALSE;
        if (tmp != handle->bind_col.param[i].len)
        {
            struct SQLBindCol_params params;

            params.StatementHandle = handle->unix_handle;
            params.ColumnNumber    = i + 1;
            params.TargetType      = handle->bind_col.param[i].col.target_type;
            params.TargetValue     = handle->bind_col.param[i].col.target_value;
            params.BufferLength    = handle->bind_col.param[i].col.buffer_length;
            params.StrLen_or_Ind   = tmp;
            if (!SUCCESS(ODBC_CALL( SQLBindCol, &params )))
            {
                free( tmp );
                return FALSE;
            }
        }
        handle->bind_col.param[i].len = tmp;
    }
    for (i = 0; i < handle->bind_parameter.count; i++)
    {
        UINT8 *tmp;
        if (!(tmp = realloc( handle->bind_parameter.param[i].len, size * sizeof(UINT64) ))) return FALSE;
        if (tmp != handle->bind_parameter.param[i].len)
        {
            struct SQLBindParameter_params params;

            params.StatementHandle = handle->unix_handle;
            params.ParameterNumber = i + 1;
            params.InputOutputType = handle->bind_parameter.param[i].parameter.input_output_type;
            params.ValueType       = handle->bind_parameter.param[i].parameter.value_type;
            params.ParameterType   = handle->bind_parameter.param[i].parameter.parameter_type;
            params.ColumnSize      = handle->bind_parameter.param[i].parameter.column_size;
            params.DecimalDigits   = handle->bind_parameter.param[i].parameter.decimal_digits;
            params.ParameterValue  = handle->bind_parameter.param[i].parameter.parameter_value;
            params.BufferLength    = handle->bind_parameter.param[i].parameter.buffer_length;
            params.StrLen_or_Ind   = tmp;
            if (!SUCCESS(ODBC_CALL( SQLBindParameter, &params )))
            {
                free( tmp );
                return FALSE;
            }
        }
        handle->bind_parameter.param[i].len = tmp;
    }
    return TRUE;
}

/*************************************************************************
 *				SQLSetStmtAttr           [ODBC32.076]
 */
SQLRETURN WINAPI SQLSetStmtAttr(SQLHSTMT StatementHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                SQLINTEGER StringLength)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, Attribute %d, Value %p, StringLength %d)\n", StatementHandle, Attribute, Value,
          StringLength);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        struct SQLSetStmtAttr_params params = { handle->unix_handle, Attribute, Value, StringLength };
        if (SUCCESS((ret = ODBC_CALL( SQLSetStmtAttr, &params ))))
        {
            SQLULEN row_count = (SQLULEN)Value;
            if (Attribute == SQL_ATTR_ROW_ARRAY_SIZE && row_count != handle->row_count)
            {
                TRACE( "resizing result length array\n" );
                if (!resize_result_lengths( handle, row_count )) ret = SQL_ERROR;
                else handle->row_count = row_count;
            }
        }
    }
    else if (handle->win32_handle)
    {
        ret = handle->win32_funcs->SQLSetStmtAttr( handle->win32_handle, Attribute, Value, StringLength );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetStmtOption           [ODBC32.051]
 */
SQLRETURN WINAPI SQLSetStmtOption(SQLHSTMT StatementHandle, SQLUSMALLINT Option, SQLULEN Value)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, Option %d, Value %s)\n", StatementHandle, Option, debugstr_sqlulen(Value));

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        struct SQLSetStmtOption_params params = { handle->unix_handle, Option, Value };
        ret = ODBC_CALL( SQLSetStmtOption, &params );
    }
    else if (handle->win32_handle)
    {
        ret = handle->win32_funcs->SQLSetStmtOption( handle->win32_handle, Option, Value );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSpecialColumns           [ODBC32.052]
 */
SQLRETURN WINAPI SQLSpecialColumns(SQLHSTMT StatementHandle, SQLUSMALLINT IdentifierType, SQLCHAR *CatalogName,
                                   SQLSMALLINT NameLength1, SQLCHAR *SchemaName, SQLSMALLINT NameLength2,
                                   SQLCHAR *TableName, SQLSMALLINT NameLength3, SQLUSMALLINT Scope,
                                   SQLUSMALLINT Nullable)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, IdentifierType %d, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d,"
          " TableName %s, NameLength3 %d, Scope %d, Nullable %d)\n", StatementHandle, IdentifierType,
          debugstr_sqlstr(CatalogName, NameLength1), NameLength1, debugstr_sqlstr(SchemaName, NameLength2),
          NameLength2, debugstr_sqlstr(TableName, NameLength3), NameLength3, Scope, Nullable);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        struct SQLSpecialColumns_params params = { handle->unix_handle, IdentifierType, CatalogName, NameLength1,
                                                   SchemaName, NameLength2, TableName, NameLength3, Scope, Nullable };
        ret = ODBC_CALL( SQLSpecialColumns, &params );
    }
    else if (handle->win32_handle)
    {
        ret = handle->win32_funcs->SQLSpecialColumns( handle->win32_handle, IdentifierType, CatalogName, NameLength1,
                                                      SchemaName, NameLength2, TableName, NameLength3, Scope, Nullable );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLStatistics           [ODBC32.053]
 */
SQLRETURN WINAPI SQLStatistics(SQLHSTMT StatementHandle, SQLCHAR *CatalogName, SQLSMALLINT NameLength1,
                               SQLCHAR *SchemaName, SQLSMALLINT NameLength2, SQLCHAR *TableName,
                               SQLSMALLINT NameLength3, SQLUSMALLINT Unique, SQLUSMALLINT Reserved)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d SchemaName %s, NameLength2 %d, TableName %s"
          " NameLength3 %d, Unique %d, Reserved %d)\n", StatementHandle,
          debugstr_sqlstr(CatalogName, NameLength1), NameLength1, debugstr_sqlstr(SchemaName, NameLength2),
          NameLength2, debugstr_sqlstr(TableName, NameLength3), NameLength3, Unique, Reserved);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        struct SQLStatistics_params params = { handle->unix_handle, CatalogName, NameLength1, SchemaName,
                                               NameLength2, TableName, NameLength3, Unique, Reserved };
        ret = ODBC_CALL( SQLStatistics, &params );
    }
    else if (handle->win32_handle)
    {
        ret = handle->win32_funcs->SQLStatistics( handle->win32_handle, CatalogName, NameLength1, SchemaName,
                                                  NameLength2, TableName, NameLength3, Unique, Reserved );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLTables           [ODBC32.054]
 */
SQLRETURN WINAPI SQLTables(SQLHSTMT StatementHandle, SQLCHAR *CatalogName, SQLSMALLINT NameLength1,
                           SQLCHAR *SchemaName, SQLSMALLINT NameLength2, SQLCHAR *TableName,
                           SQLSMALLINT NameLength3, SQLCHAR *TableType, SQLSMALLINT NameLength4)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d, TableName %s,"
          " NameLength3 %d, TableType %s, NameLength4 %d)\n", StatementHandle,
          debugstr_sqlstr(CatalogName, NameLength1), NameLength1, debugstr_sqlstr(SchemaName, NameLength2),
          NameLength2, debugstr_sqlstr(TableName, NameLength3), NameLength3, debugstr_sqlstr(TableType, NameLength4),
          NameLength4);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        struct SQLTables_params params = { handle->unix_handle, CatalogName, NameLength1, SchemaName, NameLength2,
                                           TableName, NameLength3, TableType, NameLength4 };
        ret = ODBC_CALL( SQLTables, &params );
    }
    else if (handle->win32_handle)
    {
        ret = handle->win32_funcs->SQLTables( handle->win32_handle, CatalogName, NameLength1, SchemaName, NameLength2,
                                              TableName, NameLength3, TableType, NameLength4 );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLTransact           [ODBC32.023]
 */
SQLRETURN WINAPI SQLTransact(SQLHENV EnvironmentHandle, SQLHDBC ConnectionHandle, SQLUSMALLINT CompletionType)
{
    struct handle *env = EnvironmentHandle, *con = ConnectionHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(EnvironmentHandle %p, ConnectionHandle %p, CompletionType %d)\n", EnvironmentHandle, ConnectionHandle,
          CompletionType);

    if (!env && !con) return SQL_INVALID_HANDLE;

    if ((env && env->unix_handle) || (con && con->unix_handle))
    {
        struct SQLTransact_params params = { env ? env->unix_handle : 0, con ? con->unix_handle : 0, CompletionType };
        ret = ODBC_CALL( SQLTransact, &params );
    }
    else if ((env && env->win32_handle) || (con && con->win32_handle))
    {
        const struct win32_funcs *win32_funcs;

        if (env) win32_funcs = env->win32_funcs;
        else win32_funcs = con->win32_funcs;

        ret = win32_funcs->SQLTransact( env ? env->win32_handle : NULL, con ? con->win32_handle : NULL, CompletionType );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

static WCHAR *get_datasource( const WCHAR *connection_string )
{
    const WCHAR *p = connection_string, *q;
    WCHAR *ret = NULL;
    unsigned int len;

    if (!p) return NULL;
    while (*p)
    {
        if (!wcsnicmp( p, L"DSN=", 4 ))
        {
            p += 4;
            q = wcschr( p, ';' );
            len = q ? (q - p) : wcslen( p );
            if ((ret = malloc( (len + 1) * sizeof(WCHAR) )))
            {
                memcpy( ret, p, len * sizeof(WCHAR) );
                ret[len] = 0;
                break;
            }
        }
        p++;
    }
    return ret;
}

static SQLRETURN browse_connect_win32_a( struct handle *handle, SQLCHAR *in_conn_str, SQLSMALLINT inlen,
                                         SQLCHAR *out_conn_str, SQLSMALLINT buflen, SQLSMALLINT *outlen )
{
    SQLRETURN ret = SQL_ERROR;
    SQLWCHAR *in = NULL, *out = NULL;
    SQLSMALLINT lenW;

    if (handle->win32_funcs->SQLBrowseConnect)
        return handle->win32_funcs->SQLBrowseConnect( handle->win32_handle, in_conn_str, inlen, out_conn_str,
                                                      buflen, outlen );
    if (handle->win32_funcs->SQLBrowseConnectW)
    {
        if (!(in = strnAtoW( in_conn_str, inlen ))) return SQL_ERROR;
        if (!(out = malloc( buflen * sizeof(WCHAR) ))) goto done;
        ret = handle->win32_funcs->SQLBrowseConnectW( handle->win32_handle, in, inlen, out, buflen, &lenW );
        if (SUCCESS( ret ))
        {
            int len = WideCharToMultiByte( CP_ACP, 0, out, -1, (char *)out_conn_str, buflen, NULL, NULL );
            if (outlen) *outlen = len - 1;
        }
    }
done:
    free( in );
    free( out );
    return ret;
}

static SQLRETURN browse_connect_unix_a( struct handle *handle, SQLCHAR *in_conn_str, SQLSMALLINT len,
                                        SQLCHAR *out_conn_str, SQLSMALLINT buflen, SQLSMALLINT *len2 )
{
    struct SQLBrowseConnect_params params = { handle->unix_handle, in_conn_str, len, out_conn_str, buflen, len2 };
    return ODBC_CALL( SQLBrowseConnect, &params );
}

/*************************************************************************
 *				SQLBrowseConnect           [ODBC32.055]
 */
SQLRETURN WINAPI SQLBrowseConnect(SQLHDBC ConnectionHandle, SQLCHAR *InConnectionString, SQLSMALLINT StringLength1,
                                  SQLCHAR *OutConnectionString, SQLSMALLINT BufferLength, SQLSMALLINT *StringLength2)
{
    struct handle *handle = ConnectionHandle;
    WCHAR *datasource = NULL, *filename = NULL, *connection_string = strdupAW( (const char *)InConnectionString );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(ConnectionHandle %p, InConnectionString %s, StringLength1 %d, OutConnectionString %p, BufferLength, %d, "
          "StringLength2 %p)\n", ConnectionHandle, debugstr_sqlstr(InConnectionString, StringLength1),
          StringLength1, OutConnectionString, BufferLength, StringLength2);

    if (!handle) return SQL_INVALID_HANDLE;

    /* FIXME: try DRIVER attribute if DSN is absent */
    if (!connection_string || !(datasource = get_datasource( connection_string )))
    {
        WARN( "can't find data source\n" );
        goto done;
    }
    if (!(filename = get_driver_filename( datasource )))
    {
        WARN( "can't find driver filename\n" );
        goto done;
    }

    if (has_suffix( filename, L".dll" ))
    {
        if (!(handle->win32_funcs = handle->parent->win32_funcs = load_driver( filename )))
        {
            WARN( "failed to load driver %s\n", debugstr_w(filename) );
            goto done;
        }
        TRACE( "using Windows driver %s\n", debugstr_w(filename) );

        if (!SUCCESS((ret = create_env( handle->parent, FALSE )))) goto done;
        if (!SUCCESS((ret = create_con( handle )))) goto done;

        ret = browse_connect_win32_a( handle, InConnectionString, StringLength1, OutConnectionString,
                                      BufferLength, StringLength2 );
    }
    else
    {
        TRACE( "using Unix driver %s\n", debugstr_w(filename) );

        if (!SUCCESS((ret = create_env( handle->parent, TRUE )))) goto done;
        if (!SUCCESS((ret = create_con( handle )))) goto done;

        ret = browse_connect_unix_a( handle, InConnectionString, StringLength1, OutConnectionString,
                                     BufferLength, StringLength2 );
    }

done:
    free( connection_string );
    free( filename );
    free( datasource );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLBulkOperations           [ODBC32.078]
 */
SQLRETURN WINAPI SQLBulkOperations(SQLHSTMT StatementHandle, SQLSMALLINT Operation)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, Operation %d)\n", StatementHandle, Operation);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        struct SQLBulkOperations_params params = { handle->unix_handle, Operation };
        if (SUCCESS(( ret = ODBC_CALL( SQLBulkOperations, &params )))) update_result_lengths( handle, SQL_PARAM_OUTPUT );
    }
    else if (handle->win32_handle)
    {
        ret = handle->win32_funcs->SQLBulkOperations( handle->win32_handle, Operation );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

static SQLRETURN col_attributes_unix_a( struct handle *handle, SQLUSMALLINT col, SQLUSMALLINT field_id,
                                        SQLPOINTER char_attrs, SQLSMALLINT buflen, SQLSMALLINT *retlen,
                                        SQLLEN *num_attrs )
{
    SQLRETURN ret;
    INT64 attrs;
    struct SQLColAttributes_params params = { handle->unix_handle, col, field_id, char_attrs, buflen, retlen, &attrs };
    if (SUCCESS((ret = ODBC_CALL( SQLColAttributes, &params )))) *num_attrs = attrs;
    return ret;
}

static SQLRETURN col_attributes_win32_a( struct handle *handle, SQLUSMALLINT col, SQLUSMALLINT field_id,
                                         SQLPOINTER char_attrs, SQLSMALLINT buflen, SQLSMALLINT *retlen,
                                         SQLLEN *num_attrs )
{
    SQLRETURN ret = SQL_ERROR;

    if (handle->win32_funcs->SQLColAttributes)
        return handle->win32_funcs->SQLColAttributes( handle->win32_handle, col, field_id, char_attrs, buflen,
                                                      retlen, num_attrs );

    if (handle->win32_funcs->SQLColAttributesW)
    {
        if (buflen < 0)
            ret = handle->win32_funcs->SQLColAttributesW( handle->win32_handle, col, field_id, char_attrs, buflen,
                                                          retlen, num_attrs );
        else
        {
            SQLWCHAR *strW;
            SQLSMALLINT lenW;

            if (!(strW = malloc( buflen * sizeof(WCHAR) ))) return SQL_ERROR;
            ret = handle->win32_funcs->SQLColAttributesW( handle->win32_handle, col, field_id, strW, buflen, &lenW,
                                                          num_attrs );
            if (SUCCESS( ret ))
            {
                int len = WideCharToMultiByte( CP_ACP, 0, strW, -1, char_attrs, buflen, NULL, NULL );
                if (retlen) *retlen = len - 1;
            }
            free( strW );
        }
    }
    return ret;
}

/*************************************************************************
 *				SQLColAttributes           [ODBC32.006]
 */
SQLRETURN WINAPI SQLColAttributes(SQLHSTMT StatementHandle, SQLUSMALLINT ColumnNumber, SQLUSMALLINT FieldIdentifier,
                                  SQLPOINTER CharacterAttributes, SQLSMALLINT BufferLength, SQLSMALLINT *StringLength,
                                  SQLLEN *NumericAttributes)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, ColumnNumber %d, FieldIdentifier %d, CharacterAttributes %p, BufferLength %d, "
          "StringLength %p, NumericAttributes %p)\n", StatementHandle, ColumnNumber, FieldIdentifier,
          CharacterAttributes, BufferLength, StringLength, NumericAttributes);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        ret = col_attributes_unix_a( handle, ColumnNumber, FieldIdentifier, CharacterAttributes, BufferLength,
                                     StringLength, NumericAttributes );
    }
    else if (handle->win32_handle)
    {
        ret = col_attributes_win32_a( handle, ColumnNumber, FieldIdentifier, CharacterAttributes, BufferLength,
                                      StringLength, NumericAttributes );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

static SQLRETURN column_privs_unix_a( struct handle *handle, SQLCHAR *catalog, SQLSMALLINT len1, SQLCHAR *schema,
                                      SQLSMALLINT len2, SQLCHAR *table, SQLSMALLINT len3, SQLCHAR *column,
                                      SQLSMALLINT len4 )
{
    struct SQLColumnPrivileges_params params = { handle->unix_handle, catalog, len1, schema, len2, table, len3,
                                                 column, len4 };
    return ODBC_CALL( SQLColumnPrivileges, &params );
}

static SQLRETURN column_privs_win32_a( struct handle *handle, SQLCHAR *catalog, SQLSMALLINT len1, SQLCHAR *schema,
                                      SQLSMALLINT len2, SQLCHAR *table, SQLSMALLINT len3, SQLCHAR *column,
                                      SQLSMALLINT len4 )
{
    SQLWCHAR *catalogW = NULL, *schemaW = NULL, *tableW = NULL, *columnW = NULL;
    SQLRETURN ret = SQL_ERROR;

    if (handle->win32_funcs->SQLColumnPrivileges)
        return handle->win32_funcs->SQLColumnPrivileges( handle->win32_handle, catalog, len1, schema, len2, table,
                                                         len3, column, len4 );

    if (handle->win32_funcs->SQLColumnPrivilegesW)
    {
        if (!(catalogW = strnAtoW( catalog, len1 ))) return SQL_ERROR;
        if (!(schemaW = strnAtoW( schema, len2 ))) goto done;
        if (!(tableW = strnAtoW( table, len3 ))) goto done;
        if (!(columnW = strnAtoW( column, len4 ))) goto done;
        ret = handle->win32_funcs->SQLColumnPrivilegesW( handle->win32_handle, catalogW, len1, schemaW, len2, tableW,
                                                         len3, columnW, len4 );
    }
done:
    free( catalogW );
    free( schemaW );
    free( tableW );
    free( columnW );
    return ret;
}

/*************************************************************************
 *				SQLColumnPrivileges           [ODBC32.056]
 */
SQLRETURN WINAPI SQLColumnPrivileges(SQLHSTMT StatementHandle, SQLCHAR *CatalogName, SQLSMALLINT NameLength1,
                                     SQLCHAR *SchemaName, SQLSMALLINT NameLength2, SQLCHAR *TableName,
                                     SQLSMALLINT NameLength3, SQLCHAR *ColumnName, SQLSMALLINT NameLength4)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d, TableName %s,"
          " NameLength3 %d, ColumnName %s, NameLength4 %d)\n", StatementHandle,
          debugstr_sqlstr(CatalogName, NameLength1), NameLength1, debugstr_sqlstr(SchemaName, NameLength2),
          NameLength2, debugstr_sqlstr(TableName, NameLength3), NameLength3, debugstr_sqlstr(ColumnName, NameLength4),
          NameLength4);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        ret = column_privs_unix_a( handle, CatalogName, NameLength1, SchemaName, NameLength2, TableName, NameLength3,
                                   ColumnName, NameLength4 );
    }
    else if (handle->win32_handle)
    {
        ret = column_privs_win32_a( handle, CatalogName, NameLength1, SchemaName, NameLength2, TableName, NameLength3,
                                    ColumnName, NameLength4 );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLDescribeParam          [ODBC32.058]
 */
SQLRETURN WINAPI SQLDescribeParam(SQLHSTMT StatementHandle, SQLUSMALLINT ParameterNumber, SQLSMALLINT *DataType,
                                  SQLULEN *ParameterSize, SQLSMALLINT *DecimalDigits, SQLSMALLINT *Nullable)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, ParameterNumber %d, DataType %p, ParameterSize %p, DecimalDigits %p, Nullable %p)\n",
          StatementHandle, ParameterNumber, DataType, ParameterSize, DecimalDigits, Nullable);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        UINT64 size;
        struct SQLDescribeParam_params params = { handle->unix_handle, ParameterNumber, DataType, &size,
                                                  DecimalDigits, Nullable };
        if (SUCCESS((ret = ODBC_CALL( SQLDescribeParam, &params )))) *ParameterSize = size;
    }
    else if (handle->win32_handle)
    {
        ret = handle->win32_funcs->SQLDescribeParam( handle->win32_handle, ParameterNumber, DataType, ParameterSize,
                                                     DecimalDigits, Nullable );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLExtendedFetch           [ODBC32.059]
 */
SQLRETURN WINAPI SQLExtendedFetch(SQLHSTMT StatementHandle, SQLUSMALLINT FetchOrientation, SQLLEN FetchOffset,
                                  SQLULEN *RowCount, SQLUSMALLINT *RowStatusArray)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, FetchOrientation %d, FetchOffset %s, RowCount %p, RowStatusArray %p)\n",
          StatementHandle, FetchOrientation, debugstr_sqllen(FetchOffset), RowCount, RowStatusArray);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        UINT64 count;
        struct SQLExtendedFetch_params params = { handle->unix_handle, FetchOrientation, FetchOffset, &count,
                                                  RowStatusArray };
        if (SUCCESS((ret = ODBC_CALL( SQLExtendedFetch, &params )))) *RowCount = count;
    }
    else if (handle->win32_handle)
    {
        ret = handle->win32_funcs->SQLExtendedFetch( handle->win32_handle, FetchOrientation, FetchOffset, RowCount,
                                                     RowStatusArray );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

static SQLRETURN foreign_keys_unix_a( struct handle *handle, SQLCHAR *pk_catalog, SQLSMALLINT len1,
                                      SQLCHAR *pk_schema, SQLSMALLINT len2, SQLCHAR *pk_table, SQLSMALLINT len3,
                                      SQLCHAR *fk_catalog, SQLSMALLINT len4, SQLCHAR *fk_schema, SQLSMALLINT len5,
                                      SQLCHAR *fk_table, SQLSMALLINT len6 )
{
    struct SQLForeignKeys_params params = { handle->unix_handle, pk_catalog, len1, pk_schema, len2, pk_table, len3,
                                            fk_catalog, len4, fk_schema, len5, fk_table, len6 };
    return ODBC_CALL( SQLForeignKeys, &params );
}

static SQLRETURN foreign_keys_win32_a( struct handle *handle, SQLCHAR *pk_catalog, SQLSMALLINT len1,
                                       SQLCHAR *pk_schema, SQLSMALLINT len2, SQLCHAR *pk_table, SQLSMALLINT len3,
                                       SQLCHAR *fk_catalog, SQLSMALLINT len4, SQLCHAR *fk_schema, SQLSMALLINT len5,
                                       SQLCHAR *fk_table, SQLSMALLINT len6 )
{
    SQLWCHAR *pk_catalogW = NULL, *pk_schemaW = NULL, *pk_tableW = NULL;
    SQLWCHAR *fk_catalogW = NULL, *fk_schemaW = NULL, *fk_tableW = NULL;
    SQLRETURN ret = SQL_ERROR;

    if (handle->win32_funcs->SQLForeignKeys)
        return handle->win32_funcs->SQLForeignKeys( handle->win32_handle, pk_catalog, len1, pk_schema, len2,
                                                    pk_table, len3, fk_catalog, len4, fk_schema, len5, fk_table,
                                                    len6 );
    if (handle->win32_funcs->SQLForeignKeysW)
    {
        if (!(pk_catalogW = strnAtoW( pk_catalog, len1 ))) return SQL_ERROR;
        if (!(pk_schemaW = strnAtoW( pk_schema, len2 ))) goto done;
        if (!(pk_tableW = strnAtoW( pk_table, len3 ))) goto done;
        if (!(fk_catalogW = strnAtoW( fk_catalog, len4 ))) goto done;
        if (!(fk_schemaW = strnAtoW( fk_schema, len5 ))) goto done;
        if (!(fk_tableW = strnAtoW( fk_table, len6 ))) goto done;
        ret = handle->win32_funcs->SQLForeignKeysW( handle->win32_handle, pk_catalogW, len1, pk_schemaW, len2,
                                                    pk_tableW, len3, fk_catalogW, len4, fk_schemaW, len5, fk_tableW,
                                                    len6 );
    }
done:
    free( pk_catalogW );
    free( pk_schemaW );
    free( pk_tableW );
    free( fk_catalogW );
    free( fk_schemaW );
    free( fk_tableW );
    return ret;
}

/*************************************************************************
 *				SQLForeignKeys           [ODBC32.060]
 */
SQLRETURN WINAPI SQLForeignKeys(SQLHSTMT StatementHandle, SQLCHAR *PkCatalogName, SQLSMALLINT NameLength1,
                                SQLCHAR *PkSchemaName, SQLSMALLINT NameLength2, SQLCHAR *PkTableName,
                                SQLSMALLINT NameLength3, SQLCHAR *FkCatalogName, SQLSMALLINT NameLength4,
                                SQLCHAR *FkSchemaName, SQLSMALLINT NameLength5, SQLCHAR *FkTableName,
                                SQLSMALLINT NameLength6)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, PkCatalogName %s, NameLength1 %d, PkSchemaName %s, NameLength2 %d,"
          " PkTableName %s, NameLength3 %d, FkCatalogName %s, NameLength4 %d, FkSchemaName %s,"
          " NameLength5 %d, FkTableName %s, NameLength6 %d)\n", StatementHandle,
          debugstr_sqlstr(PkCatalogName, NameLength1), NameLength1, debugstr_sqlstr(PkSchemaName, NameLength2),
          NameLength2, debugstr_sqlstr(PkTableName, NameLength3), NameLength3,
          debugstr_sqlstr(FkCatalogName, NameLength4), NameLength4, debugstr_sqlstr(FkSchemaName, NameLength5),
          NameLength5, debugstr_sqlstr(FkTableName, NameLength6), NameLength6);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        ret = foreign_keys_unix_a( handle, PkCatalogName, NameLength1, PkSchemaName, NameLength2, PkTableName,
                                   NameLength3, FkCatalogName, NameLength4, FkSchemaName, NameLength5, FkTableName,
                                   NameLength6 );
    }
    else if (handle->win32_handle)
    {
        ret = foreign_keys_win32_a( handle, PkCatalogName, NameLength1, PkSchemaName, NameLength2, PkTableName,
                                    NameLength3, FkCatalogName, NameLength4, FkSchemaName, NameLength5, FkTableName,
                                    NameLength6 );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLMoreResults           [ODBC32.061]
 */
SQLRETURN WINAPI SQLMoreResults(SQLHSTMT StatementHandle)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(%p)\n", StatementHandle);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        struct SQLMoreResults_params params = { handle->unix_handle };
        ret = ODBC_CALL( SQLMoreResults, &params );
    }
    else if (handle->win32_handle)
    {
        ret = handle->win32_funcs->SQLMoreResults( handle->win32_handle );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

static SQLRETURN native_sql_unix_a( struct handle *handle, SQLCHAR *in_statement, SQLINTEGER len,
                                    SQLCHAR *out_statement, SQLINTEGER buflen, SQLINTEGER *retlen )
{
    struct SQLNativeSql_params params = { handle->unix_handle, in_statement, len, out_statement, buflen, retlen };
    return ODBC_CALL( SQLNativeSql, &params );
}

static SQLRETURN native_sql_win32_a( struct handle *handle, SQLCHAR *in_statement, SQLINTEGER in_len,
                                     SQLCHAR *out_statement, SQLINTEGER buflen, SQLINTEGER *retlen )
{
    SQLRETURN ret = SQL_ERROR;

    if (handle->win32_funcs->SQLNativeSql)
        return handle->win32_funcs->SQLNativeSql( handle->win32_handle, in_statement, in_len, out_statement, buflen,
                                                  retlen );

    if (handle->win32_funcs->SQLNativeSqlW)
    {
        SQLWCHAR *inW, *outW;

        if (!(inW = malloc( in_len * sizeof(WCHAR) ))) return SQL_ERROR;
        if (!(outW = malloc( buflen * sizeof(WCHAR) )))
        {
            free( inW );
            return SQL_ERROR;
        }
        ret = handle->win32_funcs->SQLNativeSqlW( handle->win32_handle, inW, in_len, outW, buflen, retlen );
        if (SUCCESS( ret ))
        {
            int len = WideCharToMultiByte( CP_ACP, 0, outW, -1, (char *)out_statement, buflen, NULL, NULL );
            if (retlen) *retlen = len - 1;
        }
        free( inW );
        free( outW );
    }

    return ret;
}

/*************************************************************************
 *				SQLNativeSql           [ODBC32.062]
 */
SQLRETURN WINAPI SQLNativeSql(SQLHDBC ConnectionHandle, SQLCHAR *InStatementText, SQLINTEGER TextLength1,
                              SQLCHAR *OutStatementText, SQLINTEGER BufferLength, SQLINTEGER *TextLength2)
{
    struct handle *handle = ConnectionHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(ConnectionHandle %p, InStatementText %s, TextLength1 %d, OutStatementText %p, BufferLength, %d, "
          "TextLength2 %p)\n", ConnectionHandle, debugstr_sqlstr(InStatementText, TextLength1), TextLength1,
          OutStatementText, BufferLength, TextLength2);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        ret = native_sql_unix_a( handle, InStatementText, TextLength1, OutStatementText, BufferLength, TextLength2 );
    }
    else if (handle->win32_handle)
    {
        ret = native_sql_win32_a( handle, InStatementText, TextLength1, OutStatementText, BufferLength, TextLength2 );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLNumParams           [ODBC32.063]
 */
SQLRETURN WINAPI SQLNumParams(SQLHSTMT StatementHandle, SQLSMALLINT *ParameterCount)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, pcpar %p)\n", StatementHandle, ParameterCount);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        struct SQLNumParams_params params = { handle->unix_handle, ParameterCount };
        ret = ODBC_CALL( SQLNumParams, &params );
    }
    else if (handle->win32_handle)
    {
        ret = handle->win32_funcs->SQLNumParams( handle->win32_handle, ParameterCount );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLParamOptions           [ODBC32.064]
 */
SQLRETURN WINAPI SQLParamOptions(SQLHSTMT StatementHandle, SQLULEN RowCount, SQLULEN *RowNumber)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, RowCount %s, RowNumber %p)\n", StatementHandle, debugstr_sqlulen(RowCount),
          RowNumber);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        UINT64 row;
        struct SQLParamOptions_params params = { handle->unix_handle, RowCount, &row };
        if (SUCCESS((ret = ODBC_CALL( SQLParamOptions, &params )))) *RowNumber = row;
    }
    else if (handle->win32_handle)
    {
        ret = handle->win32_funcs->SQLParamOptions( handle->win32_handle, RowCount, RowNumber );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

static SQLRETURN primary_keys_unix_a( struct handle *handle, SQLCHAR *catalog, SQLSMALLINT len1, SQLCHAR *schema,
                                      SQLSMALLINT len2, SQLCHAR *table, SQLSMALLINT len3 )
{
    struct SQLPrimaryKeys_params params = { handle->unix_handle, catalog, len1, schema, len2, table, len3 };
    return ODBC_CALL( SQLPrimaryKeys, &params );
}

static SQLRETURN primary_keys_win32_a( struct handle *handle, SQLCHAR *catalog, SQLSMALLINT len1, SQLCHAR *schema,
                                       SQLSMALLINT len2, SQLCHAR *table, SQLSMALLINT len3 )
{
    WCHAR *catalogW = NULL, *schemaW = NULL, *tableW = NULL;
    SQLRETURN ret = SQL_ERROR;

    if (handle->win32_funcs->SQLPrimaryKeys)
        return handle->win32_funcs->SQLPrimaryKeys( handle->win32_handle, catalog, len1, schema, len2, table, len3 );

    if (handle->win32_funcs->SQLPrimaryKeysW)
    {
        if (!(catalogW = strnAtoW( catalog, len1 ))) goto done;
        if (!(schemaW = strnAtoW( schema, len2 ))) goto done;
        if (!(tableW = strnAtoW( table, len3 ))) goto done;
        ret = handle->win32_funcs->SQLPrimaryKeysW( handle->win32_handle, catalogW, len1, schemaW, len2, tableW, len3 );
    }

done:
    free( catalogW );
    free( schemaW );
    free( tableW );
    return ret;
}

/*************************************************************************
 *				SQLPrimaryKeys           [ODBC32.065]
 */
SQLRETURN WINAPI SQLPrimaryKeys(SQLHSTMT StatementHandle, SQLCHAR *CatalogName, SQLSMALLINT NameLength1,
                                SQLCHAR *SchemaName, SQLSMALLINT NameLength2, SQLCHAR *TableName,
                                SQLSMALLINT NameLength3)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d, TableName %s,"
          " NameLength3 %d)\n", StatementHandle,
          debugstr_sqlstr(CatalogName, NameLength1), NameLength1, debugstr_sqlstr(SchemaName, NameLength2),
          NameLength2, debugstr_sqlstr(TableName, NameLength3), NameLength3);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        ret = primary_keys_unix_a( handle, CatalogName, NameLength1, SchemaName, NameLength2, TableName, NameLength3 );
    }
    else if (handle->win32_handle)
    {
        ret = primary_keys_win32_a( handle, CatalogName, NameLength1, SchemaName, NameLength2, TableName, NameLength3 );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLProcedureColumns           [ODBC32.066]
 */
SQLRETURN WINAPI SQLProcedureColumns(SQLHSTMT StatementHandle, SQLCHAR *CatalogName, SQLSMALLINT NameLength1,
                                     SQLCHAR *SchemaName, SQLSMALLINT NameLength2, SQLCHAR *ProcName,
                                     SQLSMALLINT NameLength3, SQLCHAR *ColumnName, SQLSMALLINT NameLength4)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d, ProcName %s,"
          " NameLength3 %d, ColumnName %s, NameLength4 %d)\n", StatementHandle,
          debugstr_sqlstr(CatalogName, NameLength1), NameLength1, debugstr_sqlstr(SchemaName, NameLength2),
          NameLength2, debugstr_sqlstr(ProcName, NameLength3), NameLength3, debugstr_sqlstr(ColumnName, NameLength4),
          NameLength4);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        struct SQLProcedureColumns_params params = { handle->unix_handle, CatalogName, NameLength1, SchemaName,
                                                     NameLength2, ProcName, NameLength3, ColumnName, NameLength4 };
        ret = ODBC_CALL( SQLProcedureColumns, &params );
    }
    else if (handle->win32_handle)
    {
        ret = handle->win32_funcs->SQLProcedureColumns( handle->win32_handle, CatalogName, NameLength1, SchemaName,
                                                        NameLength2, ProcName, NameLength3, ColumnName, NameLength4 );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLProcedures           [ODBC32.067]
 */
SQLRETURN WINAPI SQLProcedures(SQLHSTMT StatementHandle, SQLCHAR *CatalogName, SQLSMALLINT NameLength1,
                               SQLCHAR *SchemaName, SQLSMALLINT NameLength2, SQLCHAR *ProcName,
                               SQLSMALLINT NameLength3)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d, ProcName %s,"
          " NameLength3 %d)\n", StatementHandle,
          debugstr_sqlstr(CatalogName, NameLength1), NameLength1, debugstr_sqlstr(SchemaName, NameLength2),
          NameLength2, debugstr_sqlstr(ProcName, NameLength3), NameLength3);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        struct SQLProcedures_params params = { handle->unix_handle, CatalogName, NameLength1, SchemaName,
                                               NameLength2, ProcName, NameLength3 };
        ret = ODBC_CALL( SQLProcedures, &params );
    }
    else if (handle->win32_handle)
    {
        ret = handle->win32_funcs->SQLProcedures( handle->win32_handle, CatalogName, NameLength1, SchemaName,
                                                  NameLength2, ProcName, NameLength3 );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetPos           [ODBC32.068]
 */
SQLRETURN WINAPI SQLSetPos(SQLHSTMT StatementHandle, SQLSETPOSIROW RowNumber, SQLUSMALLINT Operation,
                           SQLUSMALLINT LockType)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, RowNumber %s, Operation %d, LockType %d)\n", StatementHandle,
          debugstr_sqlulen(RowNumber), Operation, LockType);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        struct SQLSetPos_params params = { handle->unix_handle, RowNumber, Operation, LockType };
        if (SUCCESS(( ret = ODBC_CALL( SQLSetPos, &params ))) && Operation == SQL_REFRESH)
            update_result_lengths( handle, SQL_PARAM_OUTPUT );
    }
    else if (handle->win32_handle)
    {
        ret = handle->win32_funcs->SQLSetPos( handle->win32_handle, RowNumber, Operation, LockType );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLTablePrivileges           [ODBC32.070]
 */
SQLRETURN WINAPI SQLTablePrivileges(SQLHSTMT StatementHandle, SQLCHAR *CatalogName, SQLSMALLINT NameLength1,
                                    SQLCHAR *SchemaName, SQLSMALLINT NameLength2, SQLCHAR *TableName,
                                    SQLSMALLINT NameLength3)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d, TableName %s,"
          "NameLength3  %d)\n", StatementHandle,
          debugstr_sqlstr(CatalogName, NameLength1), NameLength1, debugstr_sqlstr(SchemaName, NameLength2),
          NameLength2, debugstr_sqlstr(TableName, NameLength3), NameLength3);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        struct SQLTablePrivileges_params params = { handle->unix_handle, CatalogName, NameLength1, SchemaName,
                                                    NameLength2, TableName, NameLength3 };
        ret = ODBC_CALL( SQLTablePrivileges, &params );
    }
    else if (handle->win32_handle)
    {
        ret = handle->win32_funcs->SQLTablePrivileges( handle->win32_handle, CatalogName, NameLength1, SchemaName,
                                                       NameLength2, TableName, NameLength3 );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

static HKEY open_drivers_key( void )
{
    static const WCHAR driversW[] = L"Software\\ODBC\\ODBCINST.INI\\ODBC Drivers";
    HKEY key;
    if (!RegCreateKeyExW( HKEY_LOCAL_MACHINE, driversW, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &key, NULL )) return key;
    return NULL;
}

/*************************************************************************
 *				SQLDrivers           [ODBC32.071]
 */
SQLRETURN WINAPI SQLDrivers(SQLHENV EnvironmentHandle, SQLUSMALLINT Direction, SQLCHAR *DriverDescription,
                            SQLSMALLINT BufferLength1, SQLSMALLINT *DescriptionLength,
                            SQLCHAR *DriverAttributes, SQLSMALLINT BufferLength2,
                            SQLSMALLINT *AttributesLength)
{
    struct handle *handle = EnvironmentHandle;
    DWORD len_desc = BufferLength1;
    SQLRETURN ret = SQL_ERROR;
    LONG res;

    TRACE("(EnvironmentHandle %p, Direction %d, DriverDescription %p, BufferLength1 %d, DescriptionLength %p,"
          " DriverAttributes %p, BufferLength2 %d, AttributesLength %p)\n", EnvironmentHandle, Direction,
          DriverDescription, BufferLength1, DescriptionLength, DriverAttributes, BufferLength2, AttributesLength);

    if (!handle) return SQL_INVALID_HANDLE;

    if (Direction == SQL_FETCH_FIRST || (Direction == SQL_FETCH_NEXT && !handle->drivers_key))
    {
        handle->drivers_idx = 0;
        RegCloseKey( handle->drivers_key );
        if (!(handle->drivers_key = open_drivers_key())) return SQL_ERROR;
    }

    res = RegEnumValueA( handle->drivers_key, handle->drivers_idx, (char *)DriverDescription, &len_desc,
                         NULL, NULL, NULL, NULL );
    if (res == ERROR_NO_MORE_ITEMS)
    {
        ret = SQL_NO_DATA;
        goto done;
    }
    else if (res == ERROR_SUCCESS)
    {
        if (DescriptionLength) *DescriptionLength = len_desc;

        handle->drivers_idx++;
        ret = SQL_SUCCESS;
    }
    else goto done;

    if (DriverAttributes)
    {
        FIXME( "read attributes from registry\n" );
        if (BufferLength2 >= 2) memset( DriverAttributes, 0, 2 );
    }
    if (AttributesLength) *AttributesLength = 2;

done:
    if (ret)
    {
        RegCloseKey( handle->drivers_key );
        handle->drivers_key = NULL;
        handle->drivers_idx = 0;
    }
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLBindParameter           [ODBC32.072]
 */
SQLRETURN WINAPI SQLBindParameter(SQLHSTMT StatementHandle, SQLUSMALLINT ParameterNumber, SQLSMALLINT InputOutputType,
                                  SQLSMALLINT ValueType, SQLSMALLINT ParameterType, SQLULEN ColumnSize,
                                  SQLSMALLINT DecimalDigits, SQLPOINTER ParameterValue, SQLLEN BufferLength,
                                  SQLLEN *StrLen_or_Ind)
{
    struct handle *handle = StatementHandle;
    UINT i = ParameterNumber - 1;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, ParameterNumber %d, InputOutputType %d, ValueType %d, ParameterType %d, "
          "ColumnSize %s, DecimalDigits %d, ParameterValue, %p, BufferLength %s, StrLen_or_Ind %p)\n",
          StatementHandle, ParameterNumber, InputOutputType, ValueType, ParameterType, debugstr_sqlulen(ColumnSize),
          DecimalDigits, ParameterValue, debugstr_sqllen(BufferLength), StrLen_or_Ind);

    if (!handle) return SQL_INVALID_HANDLE;
    if (!ParameterNumber)
    {
        FIXME( "parameter 0 not handled\n" );
        return SQL_ERROR;
    }
    if (!alloc_binding( &handle->bind_parameter, InputOutputType, ParameterNumber, handle->row_count )) return SQL_ERROR;
    handle->bind_parameter.param[i].parameter.input_output_type = InputOutputType;
    handle->bind_parameter.param[i].parameter.value_type        = ValueType;
    handle->bind_parameter.param[i].parameter.parameter_type    = ParameterType;
    handle->bind_parameter.param[i].parameter.column_size       = ColumnSize;
    handle->bind_parameter.param[i].parameter.decimal_digits    = DecimalDigits;
    handle->bind_parameter.param[i].parameter.parameter_value   = ParameterValue;
    handle->bind_parameter.param[i].parameter.buffer_length     = BufferLength;

    if (handle->unix_handle)
    {
        struct SQLBindParameter_params params = { handle->unix_handle, ParameterNumber, InputOutputType, ValueType,
                                                  ParameterType, ColumnSize, DecimalDigits, ParameterValue,
                                                  BufferLength, handle->bind_parameter.param[i].len };
        *(UINT64 *)params.StrLen_or_Ind = *StrLen_or_Ind;
        if (SUCCESS((ret = ODBC_CALL( SQLBindParameter, &params )))) handle->bind_parameter.param[i].ptr = StrLen_or_Ind;
    }
    else if (handle->win32_handle)
    {
        ret = handle->win32_funcs->SQLBindParameter( handle->win32_handle, ParameterNumber, InputOutputType, ValueType,
                                                     ParameterType, ColumnSize, DecimalDigits, ParameterValue,
                                                     BufferLength, StrLen_or_Ind );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

static SQLRETURN driver_connect_win32_a( struct handle *handle, SQLHWND window, SQLCHAR *in_conn_str,
                                         SQLSMALLINT inlen, SQLCHAR *out_conn_str, SQLSMALLINT buflen,
                                         SQLSMALLINT *outlen, SQLUSMALLINT completion )
{
    SQLRETURN ret = SQL_ERROR;
    SQLWCHAR *in = NULL, *out = NULL;
    SQLSMALLINT lenW;

    if (handle->win32_funcs->SQLDriverConnect)
        return handle->win32_funcs->SQLDriverConnect( handle->win32_handle, window, in_conn_str, inlen, out_conn_str,
                                                      buflen, outlen, completion );

    if (handle->win32_funcs->SQLDriverConnectW)
    {
        if (!(in = strnAtoW( in_conn_str, inlen ))) return SQL_ERROR;
        if (!(out = malloc( buflen * sizeof(WCHAR) ))) goto done;
        ret = handle->win32_funcs->SQLDriverConnectW( handle->win32_handle, window, in, inlen, out, buflen, &lenW,
                                                      completion );
        if (SUCCESS( ret ))
        {
            int len = WideCharToMultiByte( CP_ACP, 0, out, -1, (char *)out_conn_str, buflen, NULL, NULL );
            if (outlen) *outlen = len - 1;
        }
    }
done:
    free( in );
    free( out );
    return ret;
}

static SQLRETURN driver_connect_unix_a( struct handle *handle, SQLHWND window, SQLCHAR *in_conn_str, SQLSMALLINT len,
                                        SQLCHAR *out_conn_str, SQLSMALLINT buflen, SQLSMALLINT *len2,
                                        SQLUSMALLINT completion )
{
    struct SQLDriverConnect_params params = { handle->unix_handle, window, in_conn_str, len, out_conn_str, buflen,
                                              len2, completion };
    return ODBC_CALL( SQLDriverConnect, &params );
}

/*************************************************************************
 *				SQLDriverConnect           [ODBC32.041]
 */
SQLRETURN WINAPI SQLDriverConnect(SQLHDBC ConnectionHandle, SQLHWND WindowHandle, SQLCHAR *InConnectionString,
                                  SQLSMALLINT Length, SQLCHAR *OutConnectionString, SQLSMALLINT BufferLength,
                                  SQLSMALLINT *Length2, SQLUSMALLINT DriverCompletion)
{
    struct handle *handle = ConnectionHandle;
    WCHAR *datasource = NULL, *filename = NULL, *connection_string = strdupAW( (const char *)InConnectionString );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(ConnectionHandle %p, WindowHandle %p, InConnectionString %s, Length %d, OutConnectionString %p,"
          " BufferLength %d, Length2 %p, DriverCompletion %d)\n", ConnectionHandle, WindowHandle,
          debugstr_sqlstr(InConnectionString, Length), Length, OutConnectionString, BufferLength,
          Length2, DriverCompletion);

    if (!handle) return SQL_INVALID_HANDLE;

    /* FIXME: try DRIVER attribute if DSN is absent */
    if (!connection_string || !(datasource = get_datasource( connection_string )))
    {
        WARN( "can't find data source\n" );
        goto done;
    }
    if (!(filename = get_driver_filename( datasource )))
    {
        WARN( "can't find driver filename\n" );
        goto done;
    }

    if (has_suffix( filename, L".dll" ))
    {
        if (!(handle->win32_funcs = handle->parent->win32_funcs = load_driver( filename )))
        {
            WARN( "failed to load driver %s\n", debugstr_w(filename) );
            goto done;
        }
        TRACE( "using Windows driver %s\n", debugstr_w(filename) );

        if (!SUCCESS((ret = create_env( handle->parent, FALSE )))) goto done;
        if (!SUCCESS((ret = create_con( handle )))) goto done;

        ret = driver_connect_win32_a( handle, WindowHandle, InConnectionString, Length, OutConnectionString,
                                      BufferLength, Length2, DriverCompletion );
    }
    else
    {
        TRACE( "using Unix driver %s\n", debugstr_w(filename) );

        if (!SUCCESS((ret = create_env( handle->parent, TRUE )))) goto done;
        if (!SUCCESS((ret = create_con( handle )))) goto done;

        ret = driver_connect_unix_a( handle, WindowHandle, InConnectionString, Length, OutConnectionString,
                                     BufferLength, Length2, DriverCompletion );
    }

done:
    free( filename );
    free( datasource );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetScrollOptions           [ODBC32.069]
 */
SQLRETURN WINAPI SQLSetScrollOptions(SQLHSTMT StatementHandle, SQLUSMALLINT Concurrency, SQLLEN KeySetSize,
                                     SQLUSMALLINT RowSetSize)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, Concurrency %d, KeySetSize %s, RowSetSize %d)\n", StatementHandle,
          Concurrency, debugstr_sqllen(KeySetSize), RowSetSize);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        struct SQLSetScrollOptions_params params = { handle->unix_handle, Concurrency, KeySetSize, RowSetSize };
        ret = ODBC_CALL( SQLSetScrollOptions, &params );
    }
    else if (handle->win32_handle)
    {
        ret = handle->win32_funcs->SQLSetScrollOptions( handle->win32_handle, Concurrency, KeySetSize, RowSetSize );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

static BOOL SQLColAttributes_KnownStringAttribute(SQLUSMALLINT fDescType)
{
    static const SQLUSMALLINT attrList[] =
    {
        SQL_COLUMN_OWNER_NAME,
        SQL_COLUMN_QUALIFIER_NAME,
        SQL_COLUMN_LABEL,
        SQL_COLUMN_NAME,
        SQL_COLUMN_TABLE_NAME,
        SQL_COLUMN_TYPE_NAME,
        SQL_DESC_BASE_COLUMN_NAME,
        SQL_DESC_BASE_TABLE_NAME,
        SQL_DESC_CATALOG_NAME,
        SQL_DESC_LABEL,
        SQL_DESC_LITERAL_PREFIX,
        SQL_DESC_LITERAL_SUFFIX,
        SQL_DESC_LOCAL_TYPE_NAME,
        SQL_DESC_NAME,
        SQL_DESC_SCHEMA_NAME,
        SQL_DESC_TABLE_NAME,
        SQL_DESC_TYPE_NAME,
    };
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(attrList); i++) {
        if (attrList[i] == fDescType) return TRUE;
    }
    return FALSE;
}

static SQLRETURN col_attributes_unix_w( struct handle *handle, SQLUSMALLINT col, SQLUSMALLINT field_id,
                                        SQLPOINTER char_attrs, SQLSMALLINT buflen, SQLSMALLINT *retlen,
                                        SQLLEN *num_attrs )
{
    SQLRETURN ret;
    INT64 attrs;
    struct SQLColAttributesW_params params = { handle->unix_handle, col, field_id, char_attrs, buflen, retlen,
                                               &attrs };

    if (SUCCESS((ret = ODBC_CALL( SQLColAttributesW, &params )))) *num_attrs = attrs;

    if (ret == SQL_SUCCESS && SQLColAttributes_KnownStringAttribute(field_id) && char_attrs &&
        retlen && *retlen != wcslen( char_attrs ) * sizeof(WCHAR))
    {
        TRACE("CHEAT: resetting name length for ADO\n");
        *retlen = wcslen( char_attrs ) * sizeof(WCHAR);
    }
    return ret;
}

static SQLRETURN col_attributes_win32_w( struct handle *handle, SQLUSMALLINT col, SQLUSMALLINT field_id,
                                         SQLPOINTER char_attrs, SQLSMALLINT buflen, SQLSMALLINT *retlen,
                                         SQLLEN *num_attrs )
{
    if (handle->win32_funcs->SQLColAttributesW)
        return handle->win32_funcs->SQLColAttributesW( handle->win32_handle, col, field_id, char_attrs, buflen,
                                                       retlen, num_attrs );
    if (handle->win32_funcs->SQLColAttributes) FIXME( "Unicode to ANSI conversion not handled\n" );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLColAttributesW          [ODBC32.106]
 */
SQLRETURN WINAPI SQLColAttributesW(SQLHSTMT StatementHandle, SQLUSMALLINT ColumnNumber, SQLUSMALLINT FieldIdentifier,
                                   SQLPOINTER CharacterAttributes, SQLSMALLINT BufferLength, SQLSMALLINT *StringLength,
                                   SQLLEN *NumericAttributes)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, ColumnNumber %d, FieldIdentifier %d, CharacterAttributes %p, BufferLength %d, "
          "StringLength %p, NumericAttributes %p)\n", StatementHandle, ColumnNumber, FieldIdentifier,
          CharacterAttributes, BufferLength, StringLength, NumericAttributes);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        ret = col_attributes_unix_w( handle, ColumnNumber, FieldIdentifier, CharacterAttributes, BufferLength,
                                     StringLength, NumericAttributes );
    }
    else if (handle->win32_handle)
    {
        ret = col_attributes_win32_w( handle, ColumnNumber, FieldIdentifier, CharacterAttributes, BufferLength,
                                      StringLength, NumericAttributes );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

static const char *debugstr_sqlwstr( const SQLWCHAR *str, SQLSMALLINT len )
{
    if (len == SQL_NTS) len = wcslen( str );
    return wine_dbgstr_wn( str, len );
}

static SQLRETURN connect_win32_w( struct handle *handle, SQLWCHAR *servername, SQLSMALLINT len1, SQLWCHAR *username,
                                  SQLSMALLINT len2, SQLWCHAR *auth, SQLSMALLINT len3 )
{
    if (handle->win32_funcs->SQLConnectW)
        return handle->win32_funcs->SQLConnectW( handle->win32_handle, servername, len1, username, len2, auth, len3 );
    if (handle->win32_funcs->SQLConnect) FIXME( "Unicode to ANSI conversion not handled\n" );
    return SQL_ERROR;
}

static SQLRETURN connect_unix_w( struct handle *handle, SQLWCHAR *servername, SQLSMALLINT len1, SQLWCHAR *username,
                                 SQLSMALLINT len2, SQLWCHAR *auth, SQLSMALLINT len3 )
{
    struct SQLConnectW_params params = { handle->unix_handle, servername, len1, username, len2, auth, len3 };
    return ODBC_CALL( SQLConnectW, &params );
}

/*************************************************************************
 *				SQLConnectW          [ODBC32.107]
 */
SQLRETURN WINAPI SQLConnectW(SQLHDBC ConnectionHandle, SQLWCHAR *ServerName, SQLSMALLINT NameLength1,
                             SQLWCHAR *UserName, SQLSMALLINT NameLength2, SQLWCHAR *Authentication,
                             SQLSMALLINT NameLength3)
{
    struct handle *handle = ConnectionHandle;
    WCHAR *filename;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(ConnectionHandle %p, ServerName %s, NameLength1 %d, UserName %s, NameLength2 %d, Authentication %s,"
          " NameLength3 %d)\n", ConnectionHandle, debugstr_sqlwstr(ServerName, NameLength1), NameLength1,
          debugstr_sqlwstr(UserName, NameLength2), NameLength2, debugstr_sqlwstr(Authentication, NameLength3),
          NameLength3);

    if (!handle) return SQL_INVALID_HANDLE;

    if (!(filename = get_driver_filename( ServerName )))
    {
        WARN( "can't find driver filename\n" );
        goto done;
    }

    if (has_suffix( filename, L".dll" ))
    {
        if (!(handle->win32_funcs = handle->parent->win32_funcs = load_driver( filename )))
        {
            WARN( "failed to load driver %s\n", debugstr_w(filename) );
            goto done;
        }
        TRACE( "using Windows driver %s\n", debugstr_w(filename) );

        if (!SUCCESS((ret = create_env( handle->parent, FALSE )))) goto done;
        if (!SUCCESS((ret = create_con( handle )))) goto done;

        ret = connect_win32_w( handle, ServerName, NameLength1, UserName, NameLength2, Authentication, NameLength3 );
    }
    else
    {
        TRACE( "using Unix driver %s\n", debugstr_w(filename) );

        if (!SUCCESS((ret = create_env( handle->parent, TRUE )))) goto done;
        if (!SUCCESS((ret = create_con( handle )))) goto done;

        ret = connect_unix_w( handle, ServerName, NameLength1, UserName, NameLength2, Authentication, NameLength3 );
    }

done:
    free( filename );
    TRACE("Returning %d\n", ret);
    return ret;
}

static SQLRETURN describe_col_unix_w( struct handle *handle, SQLUSMALLINT col_number, SQLWCHAR *col_name,
                                      SQLSMALLINT buf_len, SQLSMALLINT *name_len, SQLSMALLINT *data_type,
                                      SQLULEN *col_size, SQLSMALLINT *decimal_digits, SQLSMALLINT *nullable )
{
    SQLRETURN ret;
    SQLSMALLINT dummy;
    UINT64 size;
    struct SQLDescribeColW_params params = { handle->unix_handle, col_number, col_name, buf_len, name_len,
                                             data_type, &size, decimal_digits, nullable };
    if (!name_len) params.NameLength = &dummy; /* workaround for drivers that don't accept NULL NameLength */

    if (SUCCESS((ret = ODBC_CALL( SQLDescribeColW, &params ))) && col_size) *col_size = size;
    return ret;
}

static SQLRETURN describe_col_win32_w( struct handle *handle, SQLUSMALLINT col_number, SQLWCHAR *col_name,
                                       SQLSMALLINT buf_len, SQLSMALLINT *name_len, SQLSMALLINT *data_type,
                                       SQLULEN *col_size, SQLSMALLINT *decimal_digits, SQLSMALLINT *nullable )
{
    if (handle->win32_funcs->SQLDescribeColW)
        return handle->win32_funcs->SQLDescribeColW( handle->win32_handle, col_number, col_name, buf_len, name_len,
                                                     data_type, col_size, decimal_digits, nullable );
    if (handle->win32_funcs->SQLDescribeCol) FIXME( "Unicode to ANSI conversion not handled\n" );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLDescribeColW          [ODBC32.108]
 */
SQLRETURN WINAPI SQLDescribeColW(SQLHSTMT StatementHandle, SQLUSMALLINT ColumnNumber, SQLWCHAR *ColumnName,
                                 SQLSMALLINT BufferLength, SQLSMALLINT *NameLength, SQLSMALLINT *DataType,
                                 SQLULEN *ColumnSize, SQLSMALLINT *DecimalDigits, SQLSMALLINT *Nullable)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, ColumnNumber %d, ColumnName %p, BufferLength %d, NameLength %p, DataType %p,"
          " ColumnSize %p, DecimalDigits %p, Nullable %p)\n", StatementHandle, ColumnNumber, ColumnName,
          BufferLength, NameLength, DataType, ColumnSize, DecimalDigits, Nullable);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        ret = describe_col_unix_w( handle, ColumnNumber, ColumnName, BufferLength, NameLength, DataType,
                                   ColumnSize, DecimalDigits, Nullable );
    }
    else if (handle->win32_handle)
    {
        ret = describe_col_win32_w( handle, ColumnNumber, ColumnName, BufferLength, NameLength, DataType,
                                    ColumnSize, DecimalDigits, Nullable );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

static SQLRETURN error_unix_w( struct handle *env, struct handle *con, struct handle *stmt, SQLWCHAR *state,
                               SQLINTEGER *native_err, SQLWCHAR *msg, SQLSMALLINT buflen, SQLSMALLINT *retlen )
{
    struct SQLErrorW_params params = { env ? env->unix_handle : 0, con ? con->unix_handle : 0,
                                       stmt ? stmt->unix_handle : 0, state, native_err, msg, buflen, retlen };
    return ODBC_CALL( SQLErrorW, &params );
}

static SQLRETURN error_win32_w( struct handle *env, struct handle *con, struct handle *stmt, SQLWCHAR *state,
                                SQLINTEGER *native_err, SQLWCHAR *msg, SQLSMALLINT buflen, SQLSMALLINT *retlen )
{
    const struct win32_funcs *win32_funcs;

    if (env) win32_funcs = env->win32_funcs;
    else if (con) win32_funcs = con->win32_funcs;
    else win32_funcs = stmt->win32_funcs;

    if (win32_funcs->SQLErrorW)
        return win32_funcs->SQLErrorW( env ? env->win32_handle : NULL, con ? con->win32_handle : NULL,
                                       stmt ? stmt->win32_handle : NULL, state, native_err, msg, buflen, retlen );
    if (win32_funcs->SQLError) FIXME( "Unicode to ANSI conversion not handled\n" );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLErrorW          [ODBC32.110]
 */
SQLRETURN WINAPI SQLErrorW(SQLHENV EnvironmentHandle, SQLHDBC ConnectionHandle, SQLHSTMT StatementHandle,
                           SQLWCHAR *SqlState, SQLINTEGER *NativeError, SQLWCHAR *MessageText,
                           SQLSMALLINT BufferLength, SQLSMALLINT *TextLength)
{
    struct handle *env = EnvironmentHandle, *con = ConnectionHandle, *stmt = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(EnvironmentHandle %p, ConnectionHandle %p, StatementHandle %p, SqlState %p, NativeError %p,"
          " MessageText %p, BufferLength %d, TextLength %p)\n", EnvironmentHandle, ConnectionHandle,
          StatementHandle, SqlState, NativeError, MessageText, BufferLength, TextLength);

    if (!env && !con && !stmt) return SQL_INVALID_HANDLE;

    if ((env && env->unix_handle) || (con && con->unix_handle) || (stmt && stmt->unix_handle))
    {
        ret = error_unix_w( env, con, stmt, SqlState, NativeError, MessageText, BufferLength, TextLength );
    }
    else if ((env && env->win32_handle) || (con && con->win32_handle) || (stmt && stmt->win32_handle))
    {
        ret = error_win32_w( env, con, stmt, SqlState, NativeError, MessageText, BufferLength, TextLength );
    }

    if (SUCCESS( ret ))
    {
        TRACE(" SqlState %s\n", debugstr_sqlwstr(SqlState, 5));
        TRACE(" Error %d\n", *NativeError);
        TRACE(" MessageText %s\n", debugstr_sqlwstr(MessageText, *TextLength));
    }
    TRACE("Returning %d\n", ret);
    return ret;
}

static SQLRETURN exec_direct_unix_w( struct handle *handle, SQLWCHAR *text, SQLINTEGER len )
{
    struct SQLExecDirectW_params params = { handle->unix_handle, text, len };
    return ODBC_CALL( SQLExecDirectW, &params );
}

static SQLRETURN exec_direct_win32_w( struct handle *handle, SQLWCHAR *text, SQLINTEGER len )
{
    if (handle->win32_funcs->SQLExecDirectW)
        return handle->win32_funcs->SQLExecDirectW( handle->win32_handle, text, len );
    if (handle->win32_funcs->SQLExecDirect) FIXME( "Unicode to ANSI conversion not handled\n" );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLExecDirectW          [ODBC32.111]
 */
SQLRETURN WINAPI SQLExecDirectW(SQLHSTMT StatementHandle, SQLWCHAR *StatementText, SQLINTEGER TextLength)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, StatementText %s, TextLength %d)\n", StatementHandle,
          debugstr_sqlwstr(StatementText, TextLength), TextLength);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        ret = exec_direct_unix_w( handle, StatementText, TextLength );
    }
    else if (handle->win32_handle)
    {
        ret = exec_direct_win32_w( handle, StatementText, TextLength );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

static SQLRETURN get_cursor_name_unix_w( struct handle *handle, SQLWCHAR *name, SQLSMALLINT buflen,
                                         SQLSMALLINT *retlen )
{
    struct SQLGetCursorNameW_params params = { handle->unix_handle, name, buflen, retlen };
    return ODBC_CALL( SQLGetCursorNameW, &params );
}

static SQLRETURN get_cursor_name_win32_w( struct handle *handle, SQLWCHAR *name, SQLSMALLINT buflen,
                                          SQLSMALLINT *retlen )
{
    if (handle->win32_funcs->SQLGetCursorNameW)
        return handle->win32_funcs->SQLGetCursorNameW( handle->win32_handle, name, buflen, retlen );
    if (handle->win32_funcs->SQLGetCursorName) FIXME( "Unicode to ANSI conversion not handled\n" );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLGetCursorNameW          [ODBC32.117]
 */
SQLRETURN WINAPI SQLGetCursorNameW(SQLHSTMT StatementHandle, SQLWCHAR *CursorName, SQLSMALLINT BufferLength,
                                   SQLSMALLINT *NameLength)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, CursorName %p, BufferLength %d, NameLength %p)\n", StatementHandle, CursorName,
          BufferLength, NameLength);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        ret = get_cursor_name_unix_w( handle, CursorName, BufferLength, NameLength );
    }
    else if (handle->win32_handle)
    {
        ret = get_cursor_name_win32_w( handle, CursorName, BufferLength, NameLength );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

static SQLRETURN prepare_unix_w( struct handle *handle, SQLWCHAR *statement, SQLINTEGER len )
{
    struct SQLPrepareW_params params = { handle->unix_handle, statement, len };
    return ODBC_CALL( SQLPrepareW, &params );
}

static SQLRETURN prepare_win32_w( struct handle *handle, SQLWCHAR *statement, SQLINTEGER len )
{
    if (handle->win32_funcs->SQLPrepareW)
        return handle->win32_funcs->SQLPrepareW( handle->win32_handle, statement, len );
    if (handle->win32_funcs->SQLPrepare) FIXME( "Unicode to ANSI conversion not handled\n" );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLPrepareW          [ODBC32.119]
 */
SQLRETURN WINAPI SQLPrepareW(SQLHSTMT StatementHandle, SQLWCHAR *StatementText, SQLINTEGER TextLength)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, StatementText %s, TextLength %d)\n", StatementHandle,
          debugstr_sqlwstr(StatementText, TextLength), TextLength);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        ret = prepare_unix_w( handle, StatementText, TextLength );
    }
    else if (handle->win32_handle)
    {
        ret = prepare_win32_w( handle, StatementText, TextLength );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetCursorNameW          [ODBC32.121]
 */
SQLRETURN WINAPI SQLSetCursorNameW(SQLHSTMT StatementHandle, SQLWCHAR *CursorName, SQLSMALLINT NameLength)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, CursorName %s, NameLength %d)\n", StatementHandle,
          debugstr_sqlwstr(CursorName, NameLength), NameLength);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        struct SQLSetCursorNameW_params params = { handle->unix_handle, CursorName, NameLength };
        ret = ODBC_CALL( SQLSetCursorNameW, &params );
    }
    else if (handle->win32_handle)
    {
        ret = handle->win32_funcs->SQLSetCursorNameW( handle->win32_handle, CursorName, NameLength );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

static SQLRETURN col_attribute_unix_w( struct handle *handle, SQLUSMALLINT col, SQLUSMALLINT field_id,
                                       SQLPOINTER char_attr, SQLSMALLINT buflen, SQLSMALLINT *retlen,
                                       SQLLEN *num_attr )
{
    SQLRETURN ret;
    INT64 attr;
    struct SQLColAttributeW_params params = { handle->unix_handle, col, field_id, char_attr, buflen, retlen, &attr };

    if (SUCCESS((ret = ODBC_CALL( SQLColAttributeW, &params ))) && num_attr) *num_attr = attr;

    if (ret == SQL_SUCCESS && SQLColAttributes_KnownStringAttribute(field_id) && char_attr &&
        retlen && *retlen != wcslen( char_attr ) * sizeof(WCHAR))
    {
        TRACE("CHEAT: resetting name length for ADO\n");
        *retlen = wcslen( char_attr ) * sizeof(WCHAR);
    }
    return ret;
}

static SQLRETURN col_attribute_win32_w( struct handle *handle, SQLUSMALLINT col, SQLUSMALLINT field_id,
                                        SQLPOINTER char_attr, SQLSMALLINT buflen, SQLSMALLINT *retlen,
                                        SQLLEN *num_attr )
{
    if (handle->win32_funcs->SQLColAttributeW)
        return handle->win32_funcs->SQLColAttributeW( handle->win32_handle, col, field_id, char_attr, buflen,
                                                      retlen, num_attr );
    if (handle->win32_funcs->SQLColAttribute) FIXME( "Unicode to ANSI conversion not handled\n" );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLColAttributeW          [ODBC32.127]
 */
SQLRETURN WINAPI SQLColAttributeW(SQLHSTMT StatementHandle, SQLUSMALLINT ColumnNumber,
                                  SQLUSMALLINT FieldIdentifier, SQLPOINTER CharacterAttribute,
                                  SQLSMALLINT BufferLength, SQLSMALLINT *StringLength,
                                  SQLLEN *NumericAttribute)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("StatementHandle %p ColumnNumber %d FieldIdentifier %d CharacterAttribute %p BufferLength %d"
          " StringLength %p NumericAttribute %p\n", StatementHandle, ColumnNumber, FieldIdentifier,
          CharacterAttribute, BufferLength, StringLength, NumericAttribute);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        ret = col_attribute_unix_w( handle, ColumnNumber, FieldIdentifier, CharacterAttribute, BufferLength,
                                    StringLength, NumericAttribute );
    }
    else if (handle->win32_handle)
    {
        ret = col_attribute_win32_w( handle, ColumnNumber, FieldIdentifier, CharacterAttribute, BufferLength,
                                     StringLength, NumericAttribute );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

static SQLRETURN get_connect_attr_unix_w( struct handle *handle, SQLINTEGER attr, SQLPOINTER value,
                                          SQLINTEGER buflen, SQLINTEGER *retlen )
{
    struct SQLGetConnectAttrW_params params = { handle->unix_handle, attr, value, buflen, retlen };
    return ODBC_CALL( SQLGetConnectAttrW, &params );
}

static SQLRETURN get_connect_attr_win32_w( struct handle *handle, SQLINTEGER attr, SQLPOINTER value,
                                           SQLINTEGER buflen, SQLINTEGER *retlen )
{
    if (handle->win32_funcs->SQLGetConnectAttrW)
        return handle->win32_funcs->SQLGetConnectAttrW( handle->win32_handle, attr, value, buflen, retlen );
    if (handle->win32_funcs->SQLGetConnectAttr) FIXME( "Unicode to ANSI conversion not handled\n" );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLGetConnectAttrW          [ODBC32.132]
 */
SQLRETURN WINAPI SQLGetConnectAttrW(SQLHDBC ConnectionHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                    SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
    struct handle *handle = ConnectionHandle;
    SQLRETURN ret = SQL_SUCCESS;

    TRACE("(ConnectionHandle %p, Attribute %d, Value %p, BufferLength %d, StringLength %p)\n", ConnectionHandle,
          Attribute, Value, BufferLength, StringLength);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        return get_connect_attr_unix_w( handle, Attribute, Value, BufferLength, StringLength );
    }
    else if (handle->win32_handle)
    {
        ret = get_connect_attr_win32_w( handle, Attribute, Value, BufferLength, StringLength );
    }
    else
    {
        switch (Attribute)
        {
        case SQL_ATTR_CONNECTION_TIMEOUT:
            *(SQLINTEGER *)Value = handle->con_attr_con_timeout;
            break;

        case SQL_ATTR_LOGIN_TIMEOUT:
            *(SQLINTEGER *)Value = handle->con_attr_login_timeout;
            break;

        default:
            FIXME( "unhandled attribute %d\n", Attribute );
            ret = SQL_ERROR;
            break;
        }
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

static SQLRETURN get_desc_field_unix_w( struct handle *handle, SQLSMALLINT record, SQLSMALLINT field_id,
                                        SQLPOINTER value, SQLINTEGER buflen, SQLINTEGER *retlen )
{
    struct SQLGetDescFieldW_params params = { handle->unix_handle, record, field_id, value, buflen, retlen };
    return ODBC_CALL( SQLGetDescFieldW, &params );
}

static SQLRETURN get_desc_field_win32_w( struct handle *handle, SQLSMALLINT record, SQLSMALLINT field_id,
                                         SQLPOINTER value, SQLINTEGER buflen, SQLINTEGER *retlen )
{
    if (handle->win32_funcs->SQLGetDescFieldW)
        return handle->win32_funcs->SQLGetDescFieldW( handle->win32_handle, record, field_id, value, buflen, retlen );
    if (handle->win32_funcs->SQLGetDescField) FIXME( "Unicode to ANSI conversion not handled\n" );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLGetDescFieldW          [ODBC32.133]
 */
SQLRETURN WINAPI SQLGetDescFieldW(SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber, SQLSMALLINT FieldIdentifier,
                                  SQLPOINTER Value, SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
    struct handle *handle = DescriptorHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(DescriptorHandle %p, RecNumber %d, FieldIdentifier %d, Value %p, BufferLength %d, StringLength %p)\n",
          DescriptorHandle, RecNumber, FieldIdentifier, Value, BufferLength, StringLength);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        ret = get_desc_field_unix_w( handle, RecNumber, FieldIdentifier, Value, BufferLength, StringLength );
    }
    else if (handle->win32_handle)
    {
        ret = get_desc_field_win32_w( handle, RecNumber, FieldIdentifier, Value, BufferLength, StringLength );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

static SQLRETURN get_desc_rec_unix_w( struct handle *handle, SQLSMALLINT record, SQLWCHAR *name, SQLSMALLINT buflen,
                                      SQLSMALLINT *retlen, SQLSMALLINT *type, SQLSMALLINT *subtype, SQLLEN *len,
                                      SQLSMALLINT *precision, SQLSMALLINT *scale, SQLSMALLINT *nullable )
{
    SQLRETURN ret = SQL_ERROR;
    INT64 len64;
    struct SQLGetDescRecW_params params = { handle->unix_handle, record, name, buflen, retlen, type, subtype, &len64,
                                            precision, scale, nullable };
    if (SUCCESS((ret = ODBC_CALL( SQLGetDescRecW, &params )))) *len = len64;
    return ret;
}

static SQLRETURN get_desc_rec_win32_w( struct handle *handle, SQLSMALLINT record, SQLWCHAR *name, SQLSMALLINT buflen,
                                       SQLSMALLINT *retlen, SQLSMALLINT *type, SQLSMALLINT *subtype, SQLLEN *len,
                                       SQLSMALLINT *precision, SQLSMALLINT *scale, SQLSMALLINT *nullable )
{
    if (handle->win32_funcs->SQLGetDescRecW)
        return handle->win32_funcs->SQLGetDescRecW( handle->win32_handle, record, name, buflen, retlen, type,
                                                    subtype, len, precision, scale, nullable );
    if (handle->win32_funcs->SQLGetDescRec) FIXME( "Unicode to ANSI conversion not handled\n" );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLGetDescRecW          [ODBC32.134]
 */
SQLRETURN WINAPI SQLGetDescRecW(SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber, SQLWCHAR *Name,
                                SQLSMALLINT BufferLength, SQLSMALLINT *StringLength, SQLSMALLINT *Type,
                                SQLSMALLINT *SubType, SQLLEN *Length, SQLSMALLINT *Precision,
                                SQLSMALLINT *Scale, SQLSMALLINT *Nullable)
{
    struct handle *handle = DescriptorHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(DescriptorHandle %p, RecNumber %d, Name %p, BufferLength %d, StringLength %p, Type %p, SubType %p,"
          " Length %p, Precision %p, Scale %p, Nullable %p)\n", DescriptorHandle, RecNumber, Name, BufferLength,
          StringLength, Type, SubType, Length, Precision, Scale, Nullable);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        ret = get_desc_rec_unix_w( handle, RecNumber, Name, BufferLength, StringLength, Type, SubType, Length,
                                   Precision, Scale, Nullable );
    }
    else if (handle->win32_handle)
    {
        ret = get_desc_rec_win32_w( handle, RecNumber, Name, BufferLength, StringLength, Type, SubType, Length,
                                    Precision, Scale, Nullable );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

static SQLRETURN get_diag_field_unix_w( SQLSMALLINT handle_type, struct handle *handle, SQLSMALLINT rec_num,
                                        SQLSMALLINT diag_id, SQLPOINTER diag_info, SQLSMALLINT buflen,
                                        SQLSMALLINT *retlen )
{
    struct SQLGetDiagFieldW_params params = { handle_type, handle->unix_handle, rec_num, diag_id, diag_info,
                                              buflen, retlen };
    return ODBC_CALL( SQLGetDiagFieldW, &params );
}

static SQLRETURN get_diag_field_win32_w( SQLSMALLINT handle_type, struct handle *handle, SQLSMALLINT rec_num,
                                         SQLSMALLINT diag_id, SQLPOINTER diag_info, SQLSMALLINT buflen,
                                         SQLSMALLINT *retlen )
{
    if (handle->win32_funcs->SQLGetDiagFieldW)
        return handle->win32_funcs->SQLGetDiagFieldW( handle_type, handle->win32_handle, rec_num, diag_id, diag_info,
                                                      buflen, retlen );
    if (handle->win32_funcs->SQLGetDiagField) FIXME( "Unicode to ANSI conversion not handled\n" );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLGetDiagFieldW          [ODBC32.135]
 */
SQLRETURN WINAPI SQLGetDiagFieldW(SQLSMALLINT HandleType, SQLHANDLE Handle, SQLSMALLINT RecNumber,
                                  SQLSMALLINT DiagIdentifier, SQLPOINTER DiagInfo, SQLSMALLINT BufferLength,
                                  SQLSMALLINT *StringLength)
{
    struct handle *handle = Handle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(HandleType %d, Handle %p, RecNumber %d, DiagIdentifier %d, DiagInfo %p, BufferLength %d,"
          " StringLength %p)\n", HandleType, Handle, RecNumber, DiagIdentifier, DiagInfo, BufferLength, StringLength);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        ret = get_diag_field_unix_w( HandleType, handle, RecNumber, DiagIdentifier, DiagInfo, BufferLength,
                                     StringLength );
    }
    else if (handle->win32_handle)
    {
        ret = get_diag_field_win32_w( HandleType, handle, RecNumber, DiagIdentifier, DiagInfo, BufferLength,
                                      StringLength );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

static SQLRETURN get_diag_rec_unix_w( SQLSMALLINT handle_type, struct handle *handle, SQLSMALLINT rec_num,
                                      SQLWCHAR *state, SQLINTEGER *native_err, SQLWCHAR *msg, SQLSMALLINT buflen,
                                      SQLSMALLINT *retlen )
{
    struct SQLGetDiagRecW_params params = { handle_type, handle->unix_handle, rec_num, state, native_err, msg,
                                            buflen, retlen };
    return ODBC_CALL( SQLGetDiagRecW, &params );
}

static SQLRETURN get_diag_rec_win32_w( SQLSMALLINT handle_type, struct handle *handle, SQLSMALLINT rec_num,
                                       SQLWCHAR *state, SQLINTEGER *native_err, SQLWCHAR *msg, SQLSMALLINT buflen,
                                       SQLSMALLINT *retlen )
{
    if (handle->win32_funcs->SQLGetDiagRecW)
        return handle->win32_funcs->SQLGetDiagRecW( handle_type, handle->win32_handle, rec_num, state, native_err,
                                                    msg, buflen, retlen );
    if (handle->win32_funcs->SQLGetDiagRec) FIXME( "Unicode to ANSI conversion not handled\n" );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLGetDiagRecW           [ODBC32.136]
 */
SQLRETURN WINAPI SQLGetDiagRecW(SQLSMALLINT HandleType, SQLHANDLE Handle, SQLSMALLINT RecNumber, SQLWCHAR *SqlState,
                                SQLINTEGER *NativeError, SQLWCHAR *MessageText, SQLSMALLINT BufferLength,
                                SQLSMALLINT *TextLength)
{
    struct handle *handle = Handle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(HandleType %d, Handle %p, RecNumber %d, SqlState %p, NativeError %p, MessageText %p, BufferLength %d,"
          " TextLength %p)\n", HandleType, Handle, RecNumber, SqlState, NativeError, MessageText, BufferLength,
          TextLength);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        ret = get_diag_rec_unix_w( HandleType, handle, RecNumber, SqlState, NativeError, MessageText, BufferLength,
                                   TextLength );
    }
    else if (handle->win32_handle)
    {
        ret = get_diag_rec_win32_w( HandleType, handle, RecNumber, SqlState, NativeError, MessageText, BufferLength,
                                    TextLength );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

static SQLRETURN get_stmt_attr_unix_w( struct handle *handle, SQLINTEGER attr, SQLPOINTER value, SQLINTEGER buflen,
                                       SQLINTEGER *retlen )
{
    struct SQLGetStmtAttrW_params params = { handle->unix_handle, attr, value, buflen, retlen };
    return ODBC_CALL( SQLGetStmtAttrW, &params );
}

static SQLRETURN get_stmt_attr_win32_w( struct handle *handle, SQLINTEGER attr, SQLPOINTER value, SQLINTEGER buflen,
                                        SQLINTEGER *retlen )
{
    if (handle->win32_funcs->SQLGetStmtAttrW)
        return handle->win32_funcs->SQLGetStmtAttrW( handle->win32_handle, attr, value, buflen, retlen );
    if (handle->win32_funcs->SQLGetStmtAttr) FIXME( "Unicode to ANSI conversion not handled\n" );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLGetStmtAttrW          [ODBC32.138]
 */
SQLRETURN WINAPI SQLGetStmtAttrW(SQLHSTMT StatementHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                 SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, Attribute %d, Value %p, BufferLength %d, StringLength %p)\n", StatementHandle,
          Attribute, Value, BufferLength, StringLength);

    if (!Value)
    {
        WARN("Unexpected NULL Value return address\n");
        return SQL_ERROR;
    }

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        ret = get_stmt_attr_unix_w( handle, Attribute, Value, BufferLength, StringLength );
    }
    else if (handle->win32_handle)
    {
        ret = get_stmt_attr_win32_w( handle, Attribute, Value, BufferLength, StringLength );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetConnectAttrW          [ODBC32.139]
 */
SQLRETURN WINAPI SQLSetConnectAttrW(SQLHDBC ConnectionHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                    SQLINTEGER StringLength)
{
    struct handle *handle = ConnectionHandle;
    SQLRETURN ret = SQL_SUCCESS;

    TRACE("(ConnectionHandle %p, Attribute %d, Value %p, StringLength %d)\n", ConnectionHandle, Attribute,
          Value, StringLength);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        struct SQLSetConnectAttrW_params params = { handle->unix_handle, Attribute, Value, StringLength };
        ret = ODBC_CALL( SQLSetConnectAttrW, &params );
    }
    else if (handle->win32_handle)
    {
        ret = handle->win32_funcs->SQLSetConnectAttrW( handle->win32_handle, Attribute, Value, StringLength );
    }
    else
    {
        switch (Attribute)
        {
        case SQL_ATTR_CONNECTION_TIMEOUT:
            handle->con_attr_con_timeout = (UINT32)(ULONG_PTR)Value;
            break;

        case SQL_ATTR_LOGIN_TIMEOUT:
            handle->con_attr_login_timeout = (UINT32)(ULONG_PTR)Value;
            break;

        default:
            FIXME( "unhandled attribute %d\n", Attribute );
            ret = SQL_ERROR;
            break;
        }
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

static SQLRETURN columns_unix_w( struct handle *handle, SQLWCHAR *catalog, SQLSMALLINT len1, SQLWCHAR *schema,
                                 SQLSMALLINT len2, SQLWCHAR *table, SQLSMALLINT len3, SQLWCHAR *column,
                                 SQLSMALLINT len4 )
{
    struct SQLColumnsW_params params = { handle->unix_handle, catalog, len1, schema, len2, table, len3, column, len4 };
    return ODBC_CALL( SQLColumnsW, &params );
}

static SQLRETURN columns_win32_w( struct handle *handle, SQLWCHAR *catalog, SQLSMALLINT len1, SQLWCHAR *schema,
                                  SQLSMALLINT len2, SQLWCHAR *table, SQLSMALLINT len3, SQLWCHAR *column,
                                  SQLSMALLINT len4 )
{
    if (handle->win32_funcs->SQLColumnsW)
        return handle->win32_funcs->SQLColumnsW( handle->win32_handle, catalog, len1, schema, len2, table, len3,
                                                 column, len4 );
    if (handle->win32_funcs->SQLColumns) FIXME( "Unicode to ANSI conversion not handled\n" );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLColumnsW          [ODBC32.140]
 */
SQLRETURN WINAPI SQLColumnsW(SQLHSTMT StatementHandle, SQLWCHAR *CatalogName, SQLSMALLINT NameLength1,
                             SQLWCHAR *SchemaName, SQLSMALLINT NameLength2, SQLWCHAR *TableName,
                             SQLSMALLINT NameLength3, SQLWCHAR *ColumnName, SQLSMALLINT NameLength4)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d, TableName %s,"
          " NameLength3 %d, ColumnName %s, NameLength4 %d)\n", StatementHandle,
          debugstr_sqlwstr(CatalogName, NameLength1), NameLength1, debugstr_sqlwstr(SchemaName, NameLength2),
          NameLength2, debugstr_sqlwstr(TableName, NameLength3), NameLength3, debugstr_sqlwstr(ColumnName, NameLength4),
          NameLength4);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        ret = columns_unix_w( handle, CatalogName, NameLength1, SchemaName, NameLength2, TableName, NameLength3,
                              ColumnName, NameLength4 );
    }
    else if (handle->win32_handle)
    {
        ret = columns_win32_w( handle, CatalogName, NameLength1, SchemaName, NameLength2, TableName, NameLength3,
                               ColumnName, NameLength4 );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

static SQLRETURN driver_connect_win32_w( struct handle *handle, SQLHWND window, SQLWCHAR *in_conn_str,
                                         SQLSMALLINT len, SQLWCHAR *out_conn_str, SQLSMALLINT buflen, SQLSMALLINT *len2,
                                         SQLUSMALLINT completion )
{
    if (handle->win32_funcs->SQLDriverConnectW)
        return handle->win32_funcs->SQLDriverConnectW( handle->win32_handle, window, in_conn_str, len, out_conn_str,
                                                       buflen, len2, completion );
    if (handle->win32_funcs->SQLDriverConnect) FIXME( "Unicode to ANSI conversion not handled\n" );
    return SQL_ERROR;
}

static SQLRETURN driver_connect_unix_w( struct handle *handle, SQLHWND window, SQLWCHAR *in_conn_str, SQLSMALLINT len,
                                        SQLWCHAR *out_conn_str, SQLSMALLINT buflen, SQLSMALLINT *len2,
                                        SQLUSMALLINT completion )
{
    struct SQLDriverConnectW_params params = { handle->unix_handle, window, in_conn_str, len, out_conn_str, buflen,
                                               len2, completion };
    return ODBC_CALL( SQLDriverConnectW, &params );
}

/*************************************************************************
 *				SQLDriverConnectW          [ODBC32.141]
 */
SQLRETURN WINAPI SQLDriverConnectW(SQLHDBC ConnectionHandle, SQLHWND WindowHandle, SQLWCHAR *InConnectionString,
                                   SQLSMALLINT Length, SQLWCHAR *OutConnectionString, SQLSMALLINT BufferLength,
                                   SQLSMALLINT *Length2, SQLUSMALLINT DriverCompletion)
{
    struct handle *handle = ConnectionHandle;
    WCHAR *datasource, *filename = NULL;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(ConnectionHandle %p, WindowHandle %p, InConnectionString %s, Length %d, OutConnectionString %p,"
          " BufferLength %d, Length2 %p, DriverCompletion %d)\n", ConnectionHandle, WindowHandle,
          debugstr_sqlwstr(InConnectionString, Length), Length, OutConnectionString, BufferLength, Length2,
          DriverCompletion);

    if (!handle) return SQL_INVALID_HANDLE;

    /* FIXME: try DRIVER attribute if DSN is absent */
    if (!(datasource = get_datasource( InConnectionString )))
    {
        WARN( "can't find data source\n" );
        goto done;
    }
    if (!(filename = get_driver_filename( datasource )))
    {
        WARN( "can't find driver filename\n" );
        goto done;
    }

    if (has_suffix( filename, L".dll" ))
    {
        if (!(handle->win32_funcs = handle->parent->win32_funcs = load_driver( filename )))
        {
            WARN( "failed to load driver %s\n", debugstr_w(filename) );
            goto done;
        }
        TRACE( "using Windows driver %s\n", debugstr_w(filename) );

        if (!SUCCESS((ret = create_env( handle->parent, FALSE )))) goto done;
        if (!SUCCESS((ret = create_con( handle )))) goto done;

        ret = driver_connect_win32_w( handle, WindowHandle, InConnectionString, Length, OutConnectionString,
                                      BufferLength, Length2, DriverCompletion );
    }
    else
    {
        TRACE( "using Unix driver %s\n", debugstr_w(filename) );

        if (!SUCCESS((ret = create_env( handle->parent, TRUE )))) goto done;
        if (!SUCCESS((ret = create_con( handle )))) goto done;

        ret = driver_connect_unix_w( handle, WindowHandle, InConnectionString, Length, OutConnectionString,
                                     BufferLength, Length2, DriverCompletion );
    }

done:
    free( filename );
    free( datasource );
    TRACE("Returning %d\n", ret);
    return ret;
}

static SQLRETURN get_connect_option_unix_w( struct handle *handle, SQLUSMALLINT option, SQLPOINTER value )
{
    struct SQLGetConnectOptionW_params params = { handle->unix_handle, option, value };
    return ODBC_CALL( SQLGetConnectOptionW, &params );
}

static SQLRETURN get_connect_option_win32_w( struct handle *handle, SQLUSMALLINT option, SQLPOINTER value )
{
    if (handle->win32_funcs->SQLGetConnectOptionW)
        return handle->win32_funcs->SQLGetConnectOptionW( handle->win32_handle, option, value );
    if (handle->win32_funcs->SQLGetConnectOption) FIXME( "Unicode to ANSI conversion not handled\n" );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLGetConnectOptionW      [ODBC32.142]
 */
SQLRETURN WINAPI SQLGetConnectOptionW(SQLHDBC ConnectionHandle, SQLUSMALLINT Option, SQLPOINTER Value)
{
    struct handle *handle = ConnectionHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(ConnectionHandle %p, Option %d, Value %p)\n", ConnectionHandle, Option, Value);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        ret = get_connect_option_unix_w( handle, Option, Value );
    }
    else if (handle->win32_handle)
    {
        ret = get_connect_option_win32_w( handle, Option, Value );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

static SQLRETURN get_info_unix_w( struct handle *handle, SQLUSMALLINT type, SQLPOINTER value, SQLSMALLINT buflen,
                                  SQLSMALLINT *retlen )
{
    struct SQLGetInfoW_params params = { handle->unix_handle, type, value, buflen, retlen };
    return ODBC_CALL( SQLGetInfoW, &params );
}

static SQLRETURN get_info_win32_w( struct handle *handle, SQLUSMALLINT type, SQLPOINTER value, SQLSMALLINT buflen,
                                   SQLSMALLINT *retlen )
{
    if (handle->win32_funcs->SQLGetInfoW)
        return handle->win32_funcs->SQLGetInfoW( handle->win32_handle, type, value, buflen, retlen );
    if (handle->win32_funcs->SQLGetInfo) FIXME( "Unicode to ANSI conversion not handled\n" );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLGetInfoW          [ODBC32.145]
 */
SQLRETURN WINAPI SQLGetInfoW(SQLHDBC ConnectionHandle, SQLUSMALLINT InfoType, SQLPOINTER InfoValue,
                             SQLSMALLINT BufferLength, SQLSMALLINT *StringLength)
{
    struct handle *handle = ConnectionHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(ConnectionHandle, %p, InfoType %d, InfoValue %p, BufferLength %d, StringLength %p)\n", ConnectionHandle,
          InfoType, InfoValue, BufferLength, StringLength);

    if (!handle) return SQL_INVALID_HANDLE;

    switch (InfoType)
    {
    case SQL_ODBC_VER:
    {
        const WCHAR version[] = L"03.80.0000";
        int len = ARRAY_SIZE(version);
        WCHAR *value = InfoValue;

        if (StringLength) *StringLength = len;
        if (value && BufferLength >= len)
        {
            wcscpy( value, version );
            if (StringLength) *StringLength = len - 1;
        }
        return SQL_SUCCESS;
    }
    default: break;
    }

    if (handle->unix_handle)
    {
        ret = get_info_unix_w( handle, InfoType, InfoValue, BufferLength, StringLength );
    }
    else if (handle->win32_handle)
    {
        ret = get_info_win32_w( handle, InfoType, InfoValue, BufferLength, StringLength );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

static SQLRETURN get_type_info_unix_w( struct handle *handle, SQLSMALLINT type )
{
    struct SQLGetTypeInfoW_params params = { handle->unix_handle, type };
    return ODBC_CALL( SQLGetTypeInfoW, &params );
}

static SQLRETURN get_type_info_win32_w( struct handle *handle, SQLSMALLINT type )
{
    if (handle->win32_funcs->SQLGetTypeInfoW)
        return handle->win32_funcs->SQLGetTypeInfoW( handle->win32_handle, type );
    if (handle->win32_funcs->SQLGetTypeInfo)
        return handle->win32_funcs->SQLGetTypeInfo( handle->win32_handle, type );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLGetTypeInfoW          [ODBC32.147]
 */
SQLRETURN WINAPI SQLGetTypeInfoW(SQLHSTMT StatementHandle, SQLSMALLINT DataType)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, DataType %d)\n", StatementHandle, DataType);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        ret = get_type_info_unix_w( handle, DataType );
    }
    else if (handle->win32_handle)
    {
        ret = get_type_info_win32_w( handle, DataType );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetConnectOptionW          [ODBC32.150]
 */
SQLRETURN WINAPI SQLSetConnectOptionW(SQLHDBC ConnectionHandle, SQLUSMALLINT Option, SQLULEN Value)
{
    struct handle *handle = ConnectionHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(ConnectionHandle %p, Option %d, Value %s)\n", ConnectionHandle, Option, debugstr_sqllen(Value));

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        struct SQLSetConnectOptionW_params params = { handle->unix_handle, Option, Value };
        ret = ODBC_CALL( SQLSetConnectOptionW, &params );
    }
    else if (handle->win32_handle)
    {
        ret = handle->win32_funcs->SQLSetConnectOptionW( handle->win32_handle, Option, Value );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSpecialColumnsW          [ODBC32.152]
 */
SQLRETURN WINAPI SQLSpecialColumnsW(SQLHSTMT StatementHandle, SQLUSMALLINT IdentifierType,
                                    SQLWCHAR *CatalogName, SQLSMALLINT NameLength1, SQLWCHAR *SchemaName,
                                    SQLSMALLINT NameLength2, SQLWCHAR *TableName, SQLSMALLINT NameLength3,
                                    SQLUSMALLINT Scope, SQLUSMALLINT Nullable)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, IdentifierType %d, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d,"
          " TableName %s, NameLength3 %d, Scope %d, Nullable %d)\n", StatementHandle, IdentifierType,
          debugstr_sqlwstr(CatalogName, NameLength1), NameLength1, debugstr_sqlwstr(SchemaName, NameLength2),
          NameLength2, debugstr_sqlwstr(TableName, NameLength3), NameLength3, Scope, Nullable);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        struct SQLSpecialColumnsW_params params = { handle->unix_handle, IdentifierType, CatalogName, NameLength1,
                                                    SchemaName, NameLength2, TableName, NameLength3, Scope, Nullable };
        ret = ODBC_CALL( SQLSpecialColumnsW, &params );
    }
    else if (handle->win32_handle)
    {
        ret = handle->win32_funcs->SQLSpecialColumnsW( handle->win32_handle, IdentifierType, CatalogName,
                                                       NameLength1, SchemaName, NameLength2, TableName, NameLength3,
                                                       Scope, Nullable );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLStatisticsW          [ODBC32.153]
 */
SQLRETURN WINAPI SQLStatisticsW(SQLHSTMT StatementHandle, SQLWCHAR *CatalogName, SQLSMALLINT NameLength1,
                                SQLWCHAR *SchemaName, SQLSMALLINT NameLength2, SQLWCHAR *TableName,
                                SQLSMALLINT NameLength3, SQLUSMALLINT Unique, SQLUSMALLINT Reserved)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d SchemaName %s, NameLength2 %d, TableName %s"
          " NameLength3 %d, Unique %d, Reserved %d)\n", StatementHandle,
          debugstr_sqlwstr(CatalogName, NameLength1), NameLength1, debugstr_sqlwstr(SchemaName, NameLength2),
          NameLength2, debugstr_sqlwstr(TableName, NameLength3), NameLength3, Unique, Reserved);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        struct SQLStatisticsW_params params = { handle->unix_handle, CatalogName, NameLength1, SchemaName,
                                                NameLength2, TableName, NameLength3, Unique, Reserved };
        ret = ODBC_CALL( SQLStatisticsW, &params );
    }
    else if (handle->win32_handle)
    {
        ret = handle->win32_funcs->SQLStatisticsW( handle->win32_handle, CatalogName, NameLength1, SchemaName,
                                                   NameLength2, TableName, NameLength3, Unique, Reserved );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLTablesW          [ODBC32.154]
 */
SQLRETURN WINAPI SQLTablesW(SQLHSTMT StatementHandle, SQLWCHAR *CatalogName, SQLSMALLINT NameLength1,
                            SQLWCHAR *SchemaName, SQLSMALLINT NameLength2, SQLWCHAR *TableName,
                            SQLSMALLINT NameLength3, SQLWCHAR *TableType, SQLSMALLINT NameLength4)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d, TableName %s,"
          " NameLength3 %d, TableType %s, NameLength4 %d)\n", StatementHandle,
          debugstr_sqlwstr(CatalogName, NameLength1), NameLength1, debugstr_sqlwstr(SchemaName, NameLength2),
          NameLength2, debugstr_sqlwstr(TableName, NameLength3), NameLength3, debugstr_sqlwstr(TableType, NameLength4),
          NameLength4);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        struct SQLTablesW_params params = { handle->unix_handle, CatalogName, NameLength1, SchemaName, NameLength2,
                                            TableName, NameLength3, TableType, NameLength4 };
        ret = ODBC_CALL( SQLTablesW, &params );
    }
    else if (handle->win32_handle)
    {
        ret = handle->win32_funcs->SQLTablesW( handle->win32_handle, CatalogName, NameLength1, SchemaName, NameLength2,
                                               TableName, NameLength3, TableType, NameLength4 );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

static SQLRETURN browse_connect_win32_w( struct handle *handle, SQLWCHAR *in_conn_str, SQLSMALLINT len,
                                         SQLWCHAR *out_conn_str, SQLSMALLINT buflen, SQLSMALLINT *len2 )
{
    if (handle->win32_funcs->SQLBrowseConnectW)
        return handle->win32_funcs->SQLBrowseConnectW( handle->win32_handle, in_conn_str, len, out_conn_str,
                                                       buflen, len2 );
    if (handle->win32_funcs->SQLBrowseConnect) FIXME( "Unicode to ANSI conversion not handled\n" );
    return SQL_ERROR;
}

static SQLRETURN browse_connect_unix_w( struct handle *handle, SQLWCHAR *in_conn_str, SQLSMALLINT len,
                                        SQLWCHAR *out_conn_str, SQLSMALLINT buflen, SQLSMALLINT *len2 )
{
    struct SQLBrowseConnectW_params params = { handle->unix_handle, in_conn_str, len, out_conn_str, buflen, len2 };
    return ODBC_CALL( SQLBrowseConnectW, &params );
}

/*************************************************************************
 *				SQLBrowseConnectW          [ODBC32.155]
 */
SQLRETURN WINAPI SQLBrowseConnectW(SQLHDBC ConnectionHandle, SQLWCHAR *InConnectionString, SQLSMALLINT StringLength1,
                                   SQLWCHAR *OutConnectionString, SQLSMALLINT BufferLength, SQLSMALLINT *StringLength2)
{
    struct handle *handle = ConnectionHandle;
    WCHAR *datasource, *filename = NULL;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(ConnectionHandle %p, InConnectionString %s, StringLength1 %d, OutConnectionString %p, BufferLength %d, "
          "StringLength2 %p)\n", ConnectionHandle, debugstr_sqlwstr(InConnectionString, StringLength1), StringLength1,
          OutConnectionString, BufferLength, StringLength2);

    if (!handle) return SQL_INVALID_HANDLE;

    /* FIXME: try DRIVER attribute if DSN is absent */
    if (!(datasource = get_datasource( InConnectionString )))
    {
        WARN( "can't find data source\n" );
        goto done;
    }
    if (!(filename = get_driver_filename( datasource )))
    {
        WARN( "can't find driver filename\n" );
        goto done;
    }

    if (has_suffix( filename, L".dll" ))
    {
        if (!(handle->win32_funcs = handle->parent->win32_funcs = load_driver( filename )))
        {
            WARN( "failed to load driver %s\n", debugstr_w(filename) );
            goto done;
        }
        TRACE( "using Windows driver %s\n", debugstr_w(filename) );

        if (!SUCCESS((ret = create_env( handle->parent, FALSE )))) goto done;
        if (!SUCCESS((ret = create_con( handle )))) goto done;

        ret = browse_connect_win32_w( handle, InConnectionString, StringLength1, OutConnectionString, BufferLength,
                                      StringLength2 );
    }
    else
    {
        TRACE( "using Unix driver %s\n", debugstr_w(filename) );

        if (!SUCCESS((ret = create_env( handle->parent, TRUE )))) goto done;
        if (!SUCCESS((ret = create_con( handle )))) goto done;

        ret = browse_connect_unix_w( handle, InConnectionString, StringLength1, OutConnectionString, BufferLength,
                                     StringLength2 );
    }

done:
    free( filename );
    free( datasource );
    TRACE("Returning %d\n", ret);
    return ret;
}

static SQLRETURN column_privs_unix_w( struct handle *handle, SQLWCHAR *catalog, SQLSMALLINT len1, SQLWCHAR *schema,
                                      SQLSMALLINT len2, SQLWCHAR *table, SQLSMALLINT len3, SQLWCHAR *column,
                                      SQLSMALLINT len4 )
{
    struct SQLColumnPrivilegesW_params params = { handle->unix_handle, catalog, len1, schema, len2, table, len3,
                                                  column, len4 };
    return ODBC_CALL( SQLColumnPrivilegesW, &params );
}

static SQLRETURN column_privs_win32_w( struct handle *handle, SQLWCHAR *catalog, SQLSMALLINT len1, SQLWCHAR *schema,
                                       SQLSMALLINT len2, SQLWCHAR *table, SQLSMALLINT len3, SQLWCHAR *column,
                                       SQLSMALLINT len4 )
{
    if (handle->win32_funcs->SQLColumnPrivilegesW)
        return handle->win32_funcs->SQLColumnPrivilegesW( handle->win32_handle, catalog, len1, schema, len2, table,
                                                          len3, column, len4 );
    if (handle->win32_funcs->SQLColumnPrivileges) FIXME( "Unicode to ANSI conversion not handled\n" );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLColumnPrivilegesW          [ODBC32.156]
 */
SQLRETURN WINAPI SQLColumnPrivilegesW(SQLHSTMT StatementHandle, SQLWCHAR *CatalogName, SQLSMALLINT NameLength1,
                                      SQLWCHAR *SchemaName, SQLSMALLINT NameLength2, SQLWCHAR *TableName,
                                      SQLSMALLINT NameLength3, SQLWCHAR *ColumnName, SQLSMALLINT NameLength4)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d, TableName %s,"
          " NameLength3 %d, ColumnName %s, NameLength3 %d)\n", StatementHandle,
          debugstr_sqlwstr(CatalogName, NameLength1), NameLength1, debugstr_sqlwstr(SchemaName, NameLength2),
          NameLength2, debugstr_sqlwstr(TableName, NameLength3), NameLength3, debugstr_sqlwstr(ColumnName, NameLength4),
          NameLength4);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        ret = column_privs_unix_w( handle, CatalogName, NameLength1, SchemaName, NameLength2, TableName, NameLength3,
                                   ColumnName, NameLength4 );
    }
    else if (handle->win32_handle)
    {
        ret = column_privs_win32_w( handle, CatalogName, NameLength1, SchemaName, NameLength2, TableName, NameLength3,
                                    ColumnName, NameLength4 );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLDataSourcesW          [ODBC32.157]
 */
SQLRETURN WINAPI SQLDataSourcesW(SQLHENV EnvironmentHandle, SQLUSMALLINT Direction, SQLWCHAR *ServerName,
                                 SQLSMALLINT BufferLength1, SQLSMALLINT *NameLength1, SQLWCHAR *Description,
                                 SQLSMALLINT BufferLength2, SQLSMALLINT *NameLength2)
{
    struct handle *handle = EnvironmentHandle;
    SQLRETURN ret = SQL_ERROR;
    DWORD len_source = BufferLength1, len_desc = BufferLength2;
    LONG res;

    TRACE("(EnvironmentHandle %p, Direction %d, ServerName %p, BufferLength1 %d, NameLength1 %p, Description %p,"
          " BufferLength2 %d, NameLength2 %p)\n", EnvironmentHandle, Direction, ServerName, BufferLength1,
          NameLength1, Description, BufferLength2, NameLength2);

    if (!handle) return SQL_INVALID_HANDLE;

    if (Direction == SQL_FETCH_FIRST || (Direction == SQL_FETCH_NEXT && !handle->sources_key))
    {
        handle->sources_idx = 0;
        handle->sources_system = FALSE;
        RegCloseKey( handle->sources_key );
        if (!(handle->sources_key = open_sources_key( HKEY_CURRENT_USER ))) return SQL_ERROR;
    }

    res = RegEnumValueW( handle->sources_key, handle->sources_idx, ServerName, &len_source, NULL, NULL,
                         (BYTE *)Description, &len_desc );
    if (res == ERROR_NO_MORE_ITEMS)
    {
        if (handle->sources_system)
        {
            ret = SQL_NO_DATA;
            goto done;
        }
        /* user key exhausted, continue with system key */
        RegCloseKey( handle->sources_key );
        if (!(handle->sources_key = open_sources_key( HKEY_LOCAL_MACHINE ))) goto done;
        handle->sources_idx = 0;
        handle->sources_system = TRUE;
        res = RegEnumValueW( handle->sources_key, handle->sources_idx, ServerName, &len_source, NULL, NULL,
                             (BYTE *)Description, &len_desc );
    }
    if (res == ERROR_NO_MORE_ITEMS)
    {
        ret = SQL_NO_DATA;
        goto done;
    }
    else if (res == ERROR_SUCCESS)
    {
        if (NameLength1) *NameLength1 = len_source;
        if (NameLength2) *NameLength2 = len_desc - 1;

        handle->sources_idx++;
        ret = SQL_SUCCESS;
    }

done:
    if (ret)
    {
        RegCloseKey( handle->sources_key );
        handle->sources_key = NULL;
        handle->sources_idx = 0;
    }
    TRACE("Returning %d\n", ret);
    return ret;
}

static SQLRETURN foreign_keys_unix_w( struct handle *handle, SQLWCHAR *pk_catalog, SQLSMALLINT len1,
                                      SQLWCHAR *pk_schema, SQLSMALLINT len2, SQLWCHAR *pk_table, SQLSMALLINT len3,
                                      SQLWCHAR *fk_catalog, SQLSMALLINT len4, SQLWCHAR *fk_schema, SQLSMALLINT len5,
                                      SQLWCHAR *fk_table, SQLSMALLINT len6 )
{
    struct SQLForeignKeysW_params params = { handle->unix_handle, pk_catalog, len1, pk_schema, len2, pk_table, len3,
                                             fk_catalog, len4, fk_schema, len5, fk_table, len6 };
    return ODBC_CALL( SQLForeignKeysW, &params );
}

static SQLRETURN foreign_keys_win32_w( struct handle *handle, SQLWCHAR *pk_catalog, SQLSMALLINT len1,
                                       SQLWCHAR *pk_schema, SQLSMALLINT len2, SQLWCHAR *pk_table, SQLSMALLINT len3,
                                       SQLWCHAR *fk_catalog, SQLSMALLINT len4, SQLWCHAR *fk_schema, SQLSMALLINT len5,
                                       SQLWCHAR *fk_table, SQLSMALLINT len6 )
{
    if (handle->win32_funcs->SQLForeignKeysW)
        return handle->win32_funcs->SQLForeignKeysW( handle->win32_handle, pk_catalog, len1, pk_schema, len2, pk_table,
                                                     len3, fk_catalog, len4, fk_schema, len5, fk_table, len6 );
    if (handle->win32_funcs->SQLForeignKeys) FIXME( "Unicode to ANSI conversion not handled\n" );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLForeignKeysW          [ODBC32.160]
 */
SQLRETURN WINAPI SQLForeignKeysW(SQLHSTMT StatementHandle, SQLWCHAR *PkCatalogName, SQLSMALLINT NameLength1,
                                 SQLWCHAR *PkSchemaName, SQLSMALLINT NameLength2, SQLWCHAR *PkTableName,
                                 SQLSMALLINT NameLength3, SQLWCHAR *FkCatalogName, SQLSMALLINT NameLength4,
                                 SQLWCHAR *FkSchemaName, SQLSMALLINT NameLength5, SQLWCHAR *FkTableName,
                                 SQLSMALLINT NameLength6)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, PkCatalogName %s, NameLength1 %d, PkSchemaName %s, NameLength2 %d,"
          " PkTableName %s, NameLength3 %d, FkCatalogName %s, NameLength4 %d, FkSchemaName %s,"
          " NameLength5 %d, FkTableName %s, NameLength6 %d)\n", StatementHandle,
          debugstr_sqlwstr(PkCatalogName, NameLength1), NameLength1, debugstr_sqlwstr(PkSchemaName, NameLength2),
          NameLength2, debugstr_sqlwstr(PkTableName, NameLength3), NameLength3,
          debugstr_sqlwstr(FkCatalogName, NameLength4), NameLength4, debugstr_sqlwstr(FkSchemaName, NameLength5),
          NameLength5, debugstr_sqlwstr(FkTableName, NameLength6), NameLength6);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        ret = foreign_keys_unix_w( handle, PkCatalogName, NameLength1, PkSchemaName, NameLength2, PkTableName,
                                   NameLength2, FkCatalogName, NameLength3, FkSchemaName, NameLength5, FkTableName,
                                   NameLength6 );
    }
    else if (handle->win32_handle)
    {
        ret = foreign_keys_win32_w( handle, PkCatalogName, NameLength1, PkSchemaName, NameLength2, PkTableName,
                                    NameLength3, FkCatalogName, NameLength4, FkSchemaName, NameLength5, FkTableName,
                                    NameLength6 );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

static SQLRETURN native_sql_unix_w( struct handle *handle, SQLWCHAR *in_statement, SQLINTEGER len,
                                    SQLWCHAR *out_statement, SQLINTEGER buflen, SQLINTEGER *retlen )
{
    struct SQLNativeSqlW_params params = { handle->unix_handle, in_statement, len, out_statement, buflen, retlen };
    return ODBC_CALL( SQLNativeSqlW, &params );
}

static SQLRETURN native_sql_win32_w( struct handle *handle, SQLWCHAR *in_statement, SQLINTEGER len,
                                     SQLWCHAR *out_statement, SQLINTEGER buflen, SQLINTEGER *retlen )
{
    if (handle->win32_funcs->SQLNativeSqlW)
        return handle->win32_funcs->SQLNativeSqlW( handle->win32_handle, in_statement, len, out_statement, buflen,
                                                   retlen );
    if (handle->win32_funcs->SQLNativeSql) FIXME( "Unicode to ANSI conversion not handled\n" );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLNativeSqlW          [ODBC32.162]
 */
SQLRETURN WINAPI SQLNativeSqlW(SQLHDBC ConnectionHandle, SQLWCHAR *InStatementText, SQLINTEGER TextLength1,
                               SQLWCHAR *OutStatementText, SQLINTEGER BufferLength, SQLINTEGER *TextLength2)
{
    struct handle *handle = ConnectionHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(ConnectionHandle %p, InStatementText %s, TextLength1 %d, OutStatementText %p, BufferLength %d, "
          "TextLength2 %p)\n", ConnectionHandle, debugstr_sqlwstr(InStatementText, TextLength1), TextLength1,
          OutStatementText, BufferLength, TextLength2);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        ret = native_sql_unix_w( handle, InStatementText, TextLength1, OutStatementText, BufferLength, TextLength2 );
    }
    else if (handle->win32_handle)
    {
        ret = native_sql_win32_w( handle, InStatementText, TextLength1, OutStatementText, BufferLength, TextLength2 );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

static SQLRETURN primary_keys_unix_w( struct handle *handle, SQLWCHAR *catalog, SQLSMALLINT len1, SQLWCHAR *schema,
                                      SQLSMALLINT len2, SQLWCHAR *table, SQLSMALLINT len3 )
{
    struct SQLPrimaryKeysW_params params = { handle->unix_handle, catalog, len1, schema, len2, table, len3 };
    return ODBC_CALL( SQLPrimaryKeysW, &params );
}

static SQLRETURN primary_keys_win32_w( struct handle *handle, SQLWCHAR *catalog, SQLSMALLINT len1, SQLWCHAR *schema,
                                       SQLSMALLINT len2, SQLWCHAR *table, SQLSMALLINT len3 )
{
    if (handle->win32_funcs->SQLPrimaryKeysW)
        return handle->win32_funcs->SQLPrimaryKeysW( handle->win32_handle, catalog, len1, schema, len2, table, len3 );
    if (handle->win32_funcs->SQLPrimaryKeys) FIXME( "Unicode to ANSI conversion not handled\n" );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLPrimaryKeysW          [ODBC32.165]
 */
SQLRETURN WINAPI SQLPrimaryKeysW(SQLHSTMT StatementHandle, SQLWCHAR *CatalogName, SQLSMALLINT NameLength1,
                                 SQLWCHAR *SchemaName, SQLSMALLINT NameLength2, SQLWCHAR *TableName,
                                 SQLSMALLINT NameLength3)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d, TableName %s,"
          " NameLength3 %d)\n", StatementHandle, debugstr_sqlwstr(CatalogName, NameLength1), NameLength1,
          debugstr_sqlwstr(SchemaName, NameLength2), NameLength2, debugstr_sqlwstr(TableName, NameLength3),
          NameLength3);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        ret = primary_keys_unix_w( handle, CatalogName, NameLength1, SchemaName, NameLength2, TableName, NameLength2 );
    }
    else if (handle->win32_handle)
    {
        ret = primary_keys_win32_w( handle, CatalogName, NameLength1, SchemaName, NameLength2, TableName, NameLength2 );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLProcedureColumnsW          [ODBC32.166]
 */
SQLRETURN WINAPI SQLProcedureColumnsW(SQLHSTMT StatementHandle, SQLWCHAR *CatalogName, SQLSMALLINT NameLength1,
                                      SQLWCHAR *SchemaName, SQLSMALLINT NameLength2, SQLWCHAR *ProcName,
                                      SQLSMALLINT NameLength3, SQLWCHAR *ColumnName, SQLSMALLINT NameLength4 )
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d, ProcName %s,"
          " NameLength3 %d, ColumnName %s, NameLength4 %d)\n", StatementHandle,
          debugstr_sqlwstr(CatalogName, NameLength1), NameLength1, debugstr_sqlwstr(SchemaName, NameLength2),
          NameLength2, debugstr_sqlwstr(ProcName, NameLength3), NameLength3, debugstr_sqlwstr(ColumnName, NameLength4),
          NameLength4);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        struct SQLProcedureColumnsW_params params = { handle->unix_handle, CatalogName, NameLength1, SchemaName,
                                                      NameLength2, ProcName, NameLength3, ColumnName, NameLength4 };
        ret = ODBC_CALL( SQLProcedureColumnsW, &params );
    }
    else if (handle->win32_handle)
    {
        ret = handle->win32_funcs->SQLProcedureColumnsW( handle->win32_handle, CatalogName, NameLength1, SchemaName,
                                                         NameLength2, ProcName, NameLength3, ColumnName, NameLength4 );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLProceduresW          [ODBC32.167]
 */
SQLRETURN WINAPI SQLProceduresW(SQLHSTMT StatementHandle, SQLWCHAR *CatalogName, SQLSMALLINT NameLength1,
                                SQLWCHAR *SchemaName, SQLSMALLINT NameLength2, SQLWCHAR *ProcName,
                                SQLSMALLINT NameLength3)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d, ProcName %s,"
          " NameLength3 %d)\n", StatementHandle, debugstr_sqlwstr(CatalogName, NameLength1), NameLength1,
          debugstr_sqlwstr(SchemaName, NameLength2), NameLength2, debugstr_sqlwstr(ProcName, NameLength3),
          NameLength3);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        struct SQLProceduresW_params params = { handle->unix_handle, CatalogName, NameLength1, SchemaName,
                                                NameLength2, ProcName, NameLength3 };
        ret = ODBC_CALL( SQLProceduresW, &params );
    }
    else if (handle->win32_handle)
    {
        ret = handle->win32_funcs->SQLProceduresW( handle->win32_handle, CatalogName, NameLength1, SchemaName,
                                                   NameLength2, ProcName, NameLength3 );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLTablePrivilegesW          [ODBC32.170]
 */
SQLRETURN WINAPI SQLTablePrivilegesW(SQLHSTMT StatementHandle, SQLWCHAR *CatalogName, SQLSMALLINT NameLength1,
                                     SQLWCHAR *SchemaName, SQLSMALLINT NameLength2, SQLWCHAR *TableName,
                                     SQLSMALLINT NameLength3)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d, TableName %s,"
          " NameLength3 %d)\n", StatementHandle, debugstr_sqlwstr(CatalogName, NameLength1), NameLength1,
          debugstr_sqlwstr(SchemaName, NameLength2), NameLength2, debugstr_sqlwstr(TableName, NameLength3),
          NameLength3);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        struct SQLTablePrivilegesW_params params = { handle->unix_handle, CatalogName, NameLength1, SchemaName,
                                                     NameLength2, TableName, NameLength3 };
        ret = ODBC_CALL( SQLTablePrivilegesW, &params );
    }
    else if (handle->win32_handle)
    {
        ret = handle->win32_funcs->SQLTablePrivilegesW( handle->win32_handle, CatalogName, NameLength1, SchemaName,
                                                        NameLength2, TableName, NameLength3 );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLDriversW          [ODBC32.171]
 */
SQLRETURN WINAPI SQLDriversW(SQLHENV EnvironmentHandle, SQLUSMALLINT Direction, SQLWCHAR *DriverDescription,
                             SQLSMALLINT BufferLength1, SQLSMALLINT *DescriptionLength, SQLWCHAR *DriverAttributes,
                             SQLSMALLINT BufferLength2, SQLSMALLINT *AttributesLength)
{
    struct handle *handle = EnvironmentHandle;
    DWORD len_desc = BufferLength1;
    SQLRETURN ret = SQL_ERROR;
    LONG res;

    TRACE("(EnvironmentHandle %p, Direction %d, DriverDescription %p, BufferLength1 %d, DescriptionLength %p,"
          " DriverAttributes %p, BufferLength2 %d, AttributesLength %p)\n", EnvironmentHandle, Direction,
          DriverDescription, BufferLength1, DescriptionLength, DriverAttributes, BufferLength2, AttributesLength);

    if (!handle) return SQL_INVALID_HANDLE;

    if (Direction == SQL_FETCH_FIRST || (Direction == SQL_FETCH_NEXT && !handle->drivers_key))
    {
        handle->drivers_idx = 0;
        RegCloseKey( handle->drivers_key );
        if (!(handle->drivers_key = open_drivers_key())) return SQL_ERROR;
    }

    res = RegEnumValueW( handle->drivers_key, handle->drivers_idx, DriverDescription, &len_desc,
                         NULL, NULL, NULL, NULL );
    if (res == ERROR_NO_MORE_ITEMS)
    {
        ret = SQL_NO_DATA;
        goto done;
    }
    else if (res == ERROR_SUCCESS)
    {
        if (DescriptionLength) *DescriptionLength = len_desc;

        handle->drivers_idx++;
        ret = SQL_SUCCESS;
    }
    else goto done;

    if (DriverAttributes)
    {
        FIXME( "read attributes from registry\n" );
        if (BufferLength2 >= 2) memset( DriverAttributes, 0, 2 * sizeof(WCHAR) );
    }
    if (AttributesLength) *AttributesLength = 2;

done:
    if (ret)
    {
        RegCloseKey( handle->drivers_key );
        handle->drivers_key = NULL;
        handle->drivers_idx = 0;
    }
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetDescFieldW          [ODBC32.173]
 */
SQLRETURN WINAPI SQLSetDescFieldW(SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber, SQLSMALLINT FieldIdentifier,
                                  SQLPOINTER Value, SQLINTEGER BufferLength)
{
    struct handle *handle = DescriptorHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(DescriptorHandle %p, RecNumber %d, FieldIdentifier %d, Value %p, BufferLength %d)\n", DescriptorHandle,
          RecNumber, FieldIdentifier, Value, BufferLength);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        struct SQLSetDescFieldW_params params = { handle->unix_handle, RecNumber, FieldIdentifier, Value,
                                                  BufferLength };
        ret = ODBC_CALL( SQLSetDescFieldW, &params );
    }
    else if (handle->win32_handle)
    {
        ret = handle->win32_funcs->SQLSetDescFieldW( handle->win32_handle, RecNumber, FieldIdentifier, Value,
                                                     BufferLength );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetStmtAttrW          [ODBC32.176]
 */
SQLRETURN WINAPI SQLSetStmtAttrW(SQLHSTMT StatementHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                 SQLINTEGER StringLength)
{
    struct handle *handle = StatementHandle;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, Attribute %d, Value %p, StringLength %d)\n", StatementHandle, Attribute,
          Value, StringLength);

    if (!handle) return SQL_INVALID_HANDLE;

    if (handle->unix_handle)
    {
        struct SQLSetStmtAttrW_params params = { handle->unix_handle, Attribute, Value, StringLength };
        if (SUCCESS((ret = ODBC_CALL( SQLSetStmtAttrW, &params ))))
        {
            SQLULEN row_count = (SQLULEN)Value;
            if (Attribute == SQL_ATTR_ROW_ARRAY_SIZE && row_count != handle->row_count)
            {
                TRACE( "resizing result length array\n" );
                if (!resize_result_lengths( handle, row_count )) ret = SQL_ERROR;
                else handle->row_count = row_count;
            }
        }
    }
    else if (handle->win32_handle)
    {
        ret = handle->win32_funcs->SQLSetStmtAttrW( handle->win32_handle, Attribute, Value, StringLength );
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/***********************************************************************
 * DllMain [Internal] Initializes the internal 'ODBC32.DLL'.
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD reason, LPVOID reserved)
{
    TRACE("proxy ODBC: %p,%lx,%p\n", hinstDLL, reason, reserved);

    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinstDLL);
        if (!__wine_init_unix_call())
        {
            if (WINE_UNIX_CALL( process_attach, NULL )) __wine_unixlib_handle = 0;
        }
        IsWow64Process( GetCurrentProcess(), &is_wow64 );
        break;

    case DLL_PROCESS_DETACH:
        if (reserved) break;
    }

    return TRUE;
}
