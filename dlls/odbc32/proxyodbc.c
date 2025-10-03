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

static BOOL is_wow64, is_old_wow64;

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

static CRITICAL_SECTION loader_cs;
static CRITICAL_SECTION_DEBUG loader_cs_debug =
{
    0, 0, &loader_cs,
    { &loader_cs_debug.ProcessLocksList, &loader_cs_debug.ProcessLocksList },
      0, 0, { (ULONG_PTR)(__FILE__ ": loader_cs") }
};
static CRITICAL_SECTION loader_cs = { &loader_cs_debug, -1, 0, 0, 0, 0 };

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

    EnterCriticalSection( &loader_cs );
    for (i = 0; i < win32_drivers.count; i++)
    {
        if (!wcsicmp( filename, win32_drivers.drivers[i]->filename ))
        {
            LeaveCriticalSection( &loader_cs );
            return &win32_drivers.drivers[i]->funcs;
        }
    }

    if (!(driver = malloc( sizeof(*driver) + (wcslen(filename) + 1) * sizeof(WCHAR) )))
    {
        LeaveCriticalSection( &loader_cs );
        return NULL;
    }
    ptr = (WCHAR *)(driver + 1);
    wcscpy( ptr, filename );
    driver->filename = ptr;

    module = LoadLibraryExW( filename, NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS );
    if (!module)
    {
        free( driver );
        LeaveCriticalSection( &loader_cs );
        return NULL;
    }

    if (!load_function_table( module, driver ) || !append_driver( driver ))
    {
        FreeLibrary( module );
        free( driver );
        LeaveCriticalSection( &loader_cs );
        return NULL;
    }

    LeaveCriticalSection( &loader_cs );
    return &driver->funcs;
}

static void init_object( struct object *obj, UINT32 type, struct object *parent )
{
    obj->type = type;
    obj->parent = parent;
    list_init( &obj->entry );
    list_init( &obj->children );
    if (parent) list_add_tail( &parent->children, &obj->entry );
    InitializeCriticalSectionEx( &obj->cs, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO );
    obj->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": object.cs");
}

static void destroy_object( struct object *obj )
{
    list_remove( &obj->entry );
    obj->cs.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection( &obj->cs );
    free( obj );
}

static struct object *lock_object( SQLHANDLE handle, UINT32 type )
{
    struct object *obj = handle;

    if (!obj) return NULL;
    EnterCriticalSection( &obj->cs );
    if (obj->type != type || obj->closed)
    {
        LeaveCriticalSection( &obj->cs );
        return NULL;
    }
    return obj;
}

static void unlock_object( struct object *obj )
{
    LeaveCriticalSection( &obj->cs );
}

static struct connection *create_connection( struct environment *env )
{
    struct connection *ret;
    if (!(ret = calloc( 1, sizeof(*ret) ))) return NULL;
    init_object( &ret->hdr, SQL_HANDLE_DBC, &env->hdr );
    ret->attr_login_timeout = 15;
    return ret;
}

/*************************************************************************
 *				SQLAllocConnect           [ODBC32.001]
 */
SQLRETURN WINAPI SQLAllocConnect(SQLHENV EnvironmentHandle, SQLHDBC *ConnectionHandle)
{
    SQLRETURN ret = SQL_ERROR;
    struct environment *env = (struct environment *)lock_object( EnvironmentHandle, SQL_HANDLE_ENV );

    TRACE("(EnvironmentHandle %p, ConnectionHandle %p)\n", EnvironmentHandle, ConnectionHandle);

    if (!env) return SQL_INVALID_HANDLE;

    /* delay creating handle in lower layer until SQLConnect() is called */
    if ((*ConnectionHandle = create_connection( EnvironmentHandle ))) ret = SQL_SUCCESS;

    TRACE("Returning %d, ConnectionHandle %p\n", ret, *ConnectionHandle);
    unlock_object( &env->hdr );
    return ret;
}

static struct environment *create_environment( void )
{
    struct environment *ret;
    if (!(ret = calloc( 1, sizeof(*ret) ))) return NULL;
    init_object( &ret->hdr, SQL_HANDLE_ENV, NULL );
    ret->attr_version = SQL_OV_ODBC2;
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
    if ((*EnvironmentHandle = create_environment())) ret = SQL_SUCCESS;

    TRACE("Returning %d, EnvironmentHandle %p\n", ret, *EnvironmentHandle);
    return ret;
}

static SQLRETURN alloc_handle( SQLSMALLINT type, struct object *input, struct object *output )
{
    SQLRETURN ret = SQL_ERROR;

    if (input->unix_handle)
    {
        struct SQLAllocHandle_params params = { type, input->unix_handle, &output->unix_handle };
        ret = ODBC_CALL( SQLAllocHandle, &params );
    }
    else if (input->win32_handle)
    {
        if (input->win32_funcs->SQLAllocHandle)
            ret = input->win32_funcs->SQLAllocHandle( type, input->win32_handle, &output->win32_handle );
        else
        {
            switch (type)
            {
            case SQL_HANDLE_ENV:
                if (input->win32_funcs->SQLAllocEnv)
                    ret = input->win32_funcs->SQLAllocEnv( &output->win32_handle );
                break;
            case SQL_HANDLE_DBC:
                if (input->win32_funcs->SQLAllocConnect)
                    ret = input->win32_funcs->SQLAllocConnect( input->win32_handle, &output->win32_handle );
                break;
            case SQL_HANDLE_STMT:
                if (input->win32_funcs->SQLAllocStmt)
                    ret = input->win32_funcs->SQLAllocStmt( input->win32_handle, &output->win32_handle );
                break;
            default: break;
            }
        }
        if (SUCCESS( ret )) output->win32_funcs = input->win32_funcs;
    }

    return ret;
}

static struct descriptor *create_descriptor( struct statement *stmt )
{
    struct descriptor *ret;
    if (!(ret = calloc( 1, sizeof(*ret) ))) return NULL;
    init_object( &ret->hdr, SQL_HANDLE_DESC, &stmt->hdr );
    return ret;
}

static void free_descriptors( struct statement *stmt )
{
    unsigned int i;
    for (i = 0; i < ARRAY_SIZE(stmt->desc); i++)
    {
        if (stmt->desc[i]) destroy_object( &stmt->desc[i]->hdr );
    }
}

static SQLRETURN alloc_descriptors( struct statement *stmt )
{
    unsigned int i;
    for (i = 0; i < ARRAY_SIZE(stmt->desc); i++)
    {
        if (!(stmt->desc[i] = create_descriptor( NULL )))
        {
            free_descriptors( stmt );
            return SQL_ERROR;
        }
    }
    return SQL_SUCCESS;
}

static struct statement *create_statement( struct connection *con )
{
    struct statement *ret;
    if (!(ret = calloc( 1, sizeof(*ret) ))) return NULL;
    init_object( &ret->hdr, SQL_HANDLE_STMT, &con->hdr );
    if (alloc_descriptors( ret ))
    {
        destroy_object( &ret->hdr );
        return NULL;
    }
    ret->row_count = 1;
    return ret;
}

/*************************************************************************
 *				SQLAllocHandle           [ODBC32.024]
 */
SQLRETURN WINAPI SQLAllocHandle(SQLSMALLINT HandleType, SQLHANDLE InputHandle, SQLHANDLE *OutputHandle)
{
    SQLRETURN ret = SQL_ERROR;

    TRACE("(HandleType %d, InputHandle %p, OutputHandle %p)\n", HandleType, InputHandle, OutputHandle);

    *OutputHandle = 0;
    switch (HandleType)
    {
    case SQL_HANDLE_ENV:
        if ((*OutputHandle = create_environment())) ret = SQL_SUCCESS;
        /* delay creating handle in lower layer until SQLConnect() is called */
        break;

    case SQL_HANDLE_DBC:
    {
        struct environment *env;
        if (!(env = (struct environment *)lock_object( InputHandle, SQL_HANDLE_ENV )))
        {
            ret = SQL_INVALID_HANDLE;
            break;
        }
        if ((*OutputHandle = create_connection( env ))) ret = SQL_SUCCESS;
        /* delay creating handle in lower layer until SQLConnect() is called */
        unlock_object( &env->hdr );
        break;
    }
    case SQL_HANDLE_STMT:
    {
        struct connection *con;
        struct statement *stmt;
        if (!(con = (struct connection *)lock_object( InputHandle, SQL_HANDLE_DBC )))
        {
            ret = SQL_INVALID_HANDLE;
            break;
        }
        if ((stmt = create_statement( con )))
        {
            if (!(ret = alloc_handle( SQL_HANDLE_STMT, &con->hdr, &stmt->hdr ))) *OutputHandle = stmt;
            else destroy_object( &stmt->hdr );
        }
        unlock_object( &con->hdr );
        break;
    }
    case SQL_HANDLE_DESC:
    {
        struct statement *stmt;
        struct descriptor *desc;
        if (!(stmt = (struct statement *)lock_object( InputHandle, SQL_HANDLE_STMT )))
        {
            ret = SQL_INVALID_HANDLE;
            break;
        }
        if ((desc = create_descriptor( stmt )))
        {
            if (!(ret = alloc_handle( SQL_HANDLE_DESC, &stmt->hdr, &desc->hdr ))) *OutputHandle = desc;
            else destroy_object( &desc->hdr );
        }
        unlock_object( &stmt->hdr );
        break;
    }
    default: break;
    }

    TRACE("Returning %d, OutputHandle %p\n", ret, *OutputHandle);
    return ret;
}

/*************************************************************************
 *				SQLAllocStmt           [ODBC32.003]
 */
SQLRETURN WINAPI SQLAllocStmt(SQLHDBC ConnectionHandle, SQLHSTMT *StatementHandle)
{
    struct connection *con = (struct connection *)lock_object( ConnectionHandle, SQL_HANDLE_DBC );
    struct statement *stmt;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(ConnectionHandle %p, StatementHandle %p)\n", ConnectionHandle, StatementHandle);

    *StatementHandle = 0;
    if (!con) ret = SQL_INVALID_HANDLE;
    else if ((stmt = create_statement( con )))
    {
        if (!(ret = alloc_handle( SQL_HANDLE_STMT, &con->hdr, &stmt->hdr ))) *StatementHandle = stmt;
        else destroy_object( &stmt->hdr );
    }

    TRACE ("Returning %d, StatementHandle %p\n", ret, *StatementHandle);
    unlock_object( &con->hdr );
    return ret;
}

static SQLRETURN alloc_handle_std( SQLSMALLINT type, struct object *input, struct object *output )
{
    SQLRETURN ret = SQL_ERROR;

    if (input->unix_handle)
    {
        struct SQLAllocHandleStd_params params = { type, input->unix_handle, &output->unix_handle };
        ret = ODBC_CALL( SQLAllocHandleStd, &params );
    }
    else if (input->win32_handle)
    {
        if (input->win32_funcs->SQLAllocHandleStd)
            ret = input->win32_funcs->SQLAllocHandleStd( type, input->win32_handle, &output->win32_handle );
        if (SUCCESS( ret )) output->win32_funcs = input->win32_funcs;
    }

    return ret;
}

/*************************************************************************
 *				SQLAllocHandleStd           [ODBC32.077]
 */
SQLRETURN WINAPI SQLAllocHandleStd(SQLSMALLINT HandleType, SQLHANDLE InputHandle, SQLHANDLE *OutputHandle)
{
    SQLRETURN ret = SQL_ERROR;

    TRACE("(HandleType %d, InputHandle %p, OutputHandle %p)\n", HandleType, InputHandle, OutputHandle);

    *OutputHandle = 0;
    switch (HandleType)
    {
    case SQL_HANDLE_ENV:
        if ((*OutputHandle = create_environment())) ret = SQL_SUCCESS;
        /* delay creating handle in lower layer until SQLConnect() is called */
        break;

    case SQL_HANDLE_DBC:
    {
        struct environment *env;
        if (!(env = (struct environment *)lock_object( InputHandle, SQL_HANDLE_ENV )))
        {
            ret = SQL_INVALID_HANDLE;
            break;
        }
        if ((*OutputHandle = create_connection( env ))) ret = SQL_SUCCESS;
        /* delay creating handle in lower layer until SQLConnect() is called */
        unlock_object( &env->hdr );
        break;
    }
    case SQL_HANDLE_STMT:
    {
        struct connection *con;
        struct statement *stmt;
        if (!(con = (struct connection *)lock_object( InputHandle, SQL_HANDLE_DBC )))
        {
            ret = SQL_INVALID_HANDLE;
            break;
        }
        if ((stmt = create_statement( con )))
        {
            if (!(ret = alloc_handle_std( SQL_HANDLE_STMT, &con->hdr, &stmt->hdr ))) *OutputHandle = stmt;
            else destroy_object( &stmt->hdr );
        }
        unlock_object( &con->hdr );
        break;
    }
    case SQL_HANDLE_DESC:
    {
        struct statement *stmt;
        struct descriptor *desc;
        if (!(stmt = (struct statement *)lock_object( InputHandle, SQL_HANDLE_STMT )))
        {
            ret = SQL_INVALID_HANDLE;
            break;
        }
        if ((desc = create_descriptor( stmt )))
        {
            if (!(ret = alloc_handle_std( SQL_HANDLE_DESC, &stmt->hdr, &desc->hdr ))) *OutputHandle = desc;
            else destroy_object( &desc->hdr );
        }
        unlock_object( &stmt->hdr );
        break;
    }
    default: break;
    }

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

static SQLRETURN bind_col_unix( struct statement *stmt, SQLUSMALLINT column, SQLSMALLINT type, SQLPOINTER value,
                                SQLLEN buflen, SQLLEN *retlen )
{
    if (is_wow64 && !is_old_wow64)
    {
        struct SQLBindCol_params params = { stmt->hdr.unix_handle, column, type, value, buflen };
        UINT i = column - 1;
        SQLRETURN ret;

        if (!column)
        {
            FIXME( "column 0 not handled\n" );
            return SQL_ERROR;
        }

        if (!alloc_binding( &stmt->bind_col, SQL_PARAM_INPUT_OUTPUT, column, stmt->row_count ))
            return SQL_ERROR;
        stmt->bind_col.param[i].col.target_type   = type;
        stmt->bind_col.param[i].col.target_value  = value;
        stmt->bind_col.param[i].col.buffer_length = buflen;

        if (retlen) params.StrLen_or_Ind = stmt->bind_col.param[i].len;
        if (SUCCESS(( ret = ODBC_CALL( SQLBindCol, &params )))) stmt->bind_col.param[i].ptr = retlen;
        return ret;
    }
    else
    {
        struct SQLBindCol_params params = { stmt->hdr.unix_handle, column, type, value, buflen, retlen };
        return ODBC_CALL( SQLBindCol, &params );
    }
}

static SQLRETURN bind_col_win32( struct statement *stmt, SQLUSMALLINT column, SQLSMALLINT type, SQLPOINTER value,
                                 SQLLEN buflen, SQLLEN *retlen )
{
    if (stmt->hdr.win32_funcs->SQLBindCol)
        return stmt->hdr.win32_funcs->SQLBindCol( stmt->hdr.win32_handle, column, type, value, buflen, retlen );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLBindCol           [ODBC32.004]
 */
SQLRETURN WINAPI SQLBindCol(SQLHSTMT StatementHandle, SQLUSMALLINT ColumnNumber, SQLSMALLINT TargetType,
                            SQLPOINTER TargetValue, SQLLEN BufferLength, SQLLEN *StrLen_or_Ind)
{
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, ColumnNumber %d, TargetType %d, TargetValue %p, BufferLength %s, StrLen_or_Ind %p)\n",
          StatementHandle, ColumnNumber, TargetType, TargetValue, debugstr_sqllen(BufferLength), StrLen_or_Ind);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = bind_col_unix( stmt, ColumnNumber, TargetType, TargetValue, BufferLength, StrLen_or_Ind );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = bind_col_win32( stmt, ColumnNumber, TargetType, TargetValue, BufferLength, StrLen_or_Ind );
    }

    TRACE ("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
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

static SQLRETURN cancel_unix( struct statement *stmt )
{
    struct SQLCancel_params params = { stmt->hdr.unix_handle };
    return ODBC_CALL( SQLCancel, &params );
}

static SQLRETURN cancel_win32( struct statement *stmt )
{
    if (stmt->hdr.win32_funcs->SQLCancel)
        return stmt->hdr.win32_funcs->SQLCancel( stmt->hdr.win32_handle );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLCancel           [ODBC32.005]
 */
SQLRETURN WINAPI SQLCancel(SQLHSTMT StatementHandle)
{
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p)\n", StatementHandle);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = cancel_unix( stmt );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = cancel_win32( stmt );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
    return ret;
}

static SQLRETURN close_cursor_unix( struct statement *stmt )
{
    struct SQLCloseCursor_params params = { stmt->hdr.unix_handle };
    return ODBC_CALL( SQLCloseCursor, &params );
}

static SQLRETURN close_cursor_win32( struct statement *stmt )
{
    if (stmt->hdr.win32_funcs->SQLCloseCursor)
        return stmt->hdr.win32_funcs->SQLCloseCursor( stmt->hdr.win32_handle );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLCloseCursor           [ODBC32.026]
 */
SQLRETURN WINAPI SQLCloseCursor(SQLHSTMT StatementHandle)
{
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p)\n", StatementHandle);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = close_cursor_unix( stmt );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = close_cursor_win32( stmt );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
    return ret;
}

static SQLRETURN col_attribute_unix_a( struct statement *stmt, SQLUSMALLINT col, SQLUSMALLINT field_id,
                                       SQLPOINTER char_attr, SQLSMALLINT buflen, SQLSMALLINT *retlen,
                                       SQLLEN *num_attr )
{
    SQLRETURN ret;
    INT64 attr;
    struct SQLColAttribute_params params = { stmt->hdr.unix_handle, col, field_id, char_attr, buflen, retlen, &attr };
    if (SUCCESS((ret = ODBC_CALL( SQLColAttribute, &params ))) && num_attr) *num_attr = attr;
    return ret;
}

static SQLRETURN col_attribute_win32_a( struct statement *stmt, SQLUSMALLINT col, SQLUSMALLINT field_id,
                                        SQLPOINTER char_attr, SQLSMALLINT buflen, SQLSMALLINT *retlen,
                                        SQLLEN *num_attr )
{
    SQLRETURN ret;

    if (stmt->hdr.win32_funcs->SQLColAttribute)
        return stmt->hdr.win32_funcs->SQLColAttribute( stmt->hdr.win32_handle, col, field_id, char_attr, buflen,
                                                       retlen, num_attr );

    if (stmt->hdr.win32_funcs->SQLColAttributeW)
    {
        if (buflen < 0)
            ret = stmt->hdr.win32_funcs->SQLColAttributeW( stmt->hdr.win32_handle, col, field_id, char_attr, buflen,
                                                           retlen, num_attr );
        else
        {
            SQLWCHAR *strW;
            SQLSMALLINT lenW;

            if (!(strW = malloc( buflen * sizeof(WCHAR) ))) return SQL_ERROR;
            ret = stmt->hdr.win32_funcs->SQLColAttributeW( stmt->hdr.win32_handle, col, field_id, strW, buflen, &lenW,
                                                           num_attr );
            if (SUCCESS( ret ))
            {
                int len = WideCharToMultiByte( CP_ACP, 0, strW, -1, char_attr, buflen, NULL, NULL );
                if (retlen) *retlen = len - 1;
            }
            free( strW );
        }
        return ret;
    }

    if (stmt->hdr.win32_funcs->SQLColAttributes)
    {
        if (buflen < 0) return SQL_ERROR;
        if (!col)
        {
            FIXME( "column 0 not handled\n" );
            return SQL_ERROR;
        }

        switch (field_id)
        {
        case SQL_DESC_COUNT:
            field_id = SQL_COLUMN_COUNT;
            break;

        case SQL_DESC_TYPE:
            field_id = SQL_COLUMN_TYPE;
            break;

        case SQL_DESC_LENGTH:
            field_id = SQL_COLUMN_LENGTH;
            break;

        case SQL_DESC_PRECISION:
            field_id = SQL_COLUMN_PRECISION;
            break;

        case SQL_DESC_SCALE:
            field_id = SQL_COLUMN_SCALE;
            break;

        case SQL_DESC_NULLABLE:
            field_id = SQL_COLUMN_NULLABLE;
            break;

        case SQL_DESC_NAME:
            field_id = SQL_COLUMN_NAME;
            break;

        case SQL_COLUMN_NAME:
        case SQL_COLUMN_TYPE:
        case SQL_COLUMN_DISPLAY_SIZE:
        case SQL_MAX_COLUMNS_IN_TABLE:
            break;

        default:
            FIXME( "field id %u not handled\n", field_id );
            return SQL_ERROR;
        }

        return stmt->hdr.win32_funcs->SQLColAttributes( stmt->hdr.win32_handle, col, field_id, char_attr, buflen,
                                                        retlen, num_attr );
    }

    return SQL_ERROR;
}

/*************************************************************************
 *				SQLColAttribute           [ODBC32.027]
 */
SQLRETURN WINAPI SQLColAttribute(SQLHSTMT StatementHandle, SQLUSMALLINT ColumnNumber, SQLUSMALLINT FieldIdentifier,
                                 SQLPOINTER CharacterAttribute, SQLSMALLINT BufferLength, SQLSMALLINT *StringLength,
                                 SQLLEN *NumericAttribute)
{
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, ColumnNumber %d, FieldIdentifier %d, CharacterAttribute %p, BufferLength %d,"
          " StringLength %p, NumericAttribute %p)\n", StatementHandle, ColumnNumber, FieldIdentifier,
          CharacterAttribute, BufferLength, StringLength, NumericAttribute);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = col_attribute_unix_a( stmt, ColumnNumber, FieldIdentifier, CharacterAttribute, BufferLength,
                                    StringLength, NumericAttribute );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = col_attribute_win32_a( stmt, ColumnNumber, FieldIdentifier, CharacterAttribute, BufferLength,
                                     StringLength, NumericAttribute );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
    return ret;
}

static const char *debugstr_sqlstr( const SQLCHAR *str, SQLSMALLINT len )
{
    if (len == SQL_NTS) len = -1;
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

static SQLCHAR *strnWtoA( const SQLWCHAR *str, int len )
{
    SQLCHAR *ret;
    int lenA;

    if (!str) return NULL;

    if (len == SQL_NTS) len = -1;
    lenA = WideCharToMultiByte( CP_ACP, 0, str, len, NULL, 0, NULL, NULL );

    if ((ret = malloc( (lenA + 1) * sizeof(*ret) )))
    {
        WideCharToMultiByte( CP_ACP, 0, str, len, (char *)ret, lenA, NULL, NULL );
        ret[lenA] = 0;
    }

    return ret;
}

static SQLRETURN columns_unix_a( struct statement *stmt, SQLCHAR *catalog, SQLSMALLINT len1, SQLCHAR *schema,
                                 SQLSMALLINT len2, SQLCHAR *table, SQLSMALLINT len3, SQLCHAR *column,
                                 SQLSMALLINT len4 )
{
    struct SQLColumns_params params = { stmt->hdr.unix_handle, catalog, len1, schema, len2, table, len3, column, len4 };
    return ODBC_CALL( SQLColumns, &params );
}

static SQLRETURN columns_win32_a( struct statement *stmt, SQLCHAR *catalog, SQLSMALLINT len1, SQLCHAR *schema,
                                  SQLSMALLINT len2, SQLCHAR *table, SQLSMALLINT len3, SQLCHAR *column,
                                  SQLSMALLINT len4 )
{
    SQLWCHAR *catalogW = NULL, *schemaW = NULL, *tableW = NULL, *columnW = NULL;
    SQLRETURN ret = SQL_ERROR;

    if (stmt->hdr.win32_funcs->SQLColumns)
        return stmt->hdr.win32_funcs->SQLColumns( stmt->hdr.win32_handle, catalog, len1, schema, len2, table, len3,
                                                  column, len4 );

    if (stmt->hdr.win32_funcs->SQLColumnsW)
    {
        if (!(catalogW = strnAtoW( catalog, len1 ))) return SQL_ERROR;
        if (!(schemaW = strnAtoW( schema, len2 ))) goto done;
        if (!(tableW = strnAtoW( table, len3 ))) goto done;
        if (!(columnW = strnAtoW( column, len4 ))) goto done;
        ret = stmt->hdr.win32_funcs->SQLColumnsW( stmt->hdr.win32_handle, catalogW, len1, schemaW, len2, tableW, len3,
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
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d, TableName %s,"
          " NameLength3 %d, ColumnName %s, NameLength4 %d)\n", StatementHandle,
          debugstr_sqlstr(CatalogName, NameLength1), NameLength1, debugstr_sqlstr(SchemaName, NameLength2),
          NameLength2, debugstr_sqlstr(TableName, NameLength3), NameLength3,
          debugstr_sqlstr(ColumnName, NameLength4), NameLength4);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = columns_unix_a( stmt, CatalogName, NameLength1, SchemaName, NameLength2, TableName, NameLength3,
                              ColumnName, NameLength4 );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = columns_win32_a( stmt, CatalogName, NameLength1, SchemaName, NameLength2, TableName, NameLength3,
                               ColumnName, NameLength4 );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
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

static WCHAR *get_driver_filename_from_name( const SQLWCHAR *name )
{
    HKEY key_odbcinst, key_driver;
    WCHAR *ret;

    if (!name) return NULL;
    if (!(key_odbcinst = open_odbcinst_key()) || RegOpenKeyExW( key_odbcinst, name, 0, KEY_READ, &key_driver ))
    {
        RegCloseKey( key_odbcinst );
        return NULL;
    }

    ret = get_reg_value( key_driver, L"Driver" );

    RegCloseKey( key_driver );
    RegCloseKey( key_odbcinst );
    return ret;
}

static WCHAR *get_driver_filename_from_source( const SQLWCHAR *source )
{
    HKEY key_sources;
    WCHAR *driver_name, *ret;

    if (!source) return NULL;
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

    ret = get_driver_filename_from_name( driver_name );
    free( driver_name );
    return ret;
}

static int has_suffix( const WCHAR *str, const WCHAR *suffix )
{
    int len = wcslen( str ), len2 = wcslen( suffix );
    return len >= len2 && !wcsicmp( str + len - len2, suffix );
}

static SQLRETURN set_env_attr( struct environment *env, SQLINTEGER attr, SQLPOINTER value, SQLINTEGER len )
{
    SQLRETURN ret = SQL_ERROR;

    if (env->hdr.unix_handle)
    {
        struct SQLSetEnvAttr_params params = { env->hdr.unix_handle, attr, value, len };
        ret = ODBC_CALL( SQLSetEnvAttr, &params );
    }
    else if (env->hdr.win32_handle)
    {
        if (env->hdr.win32_funcs->SQLSetEnvAttr)
            ret = env->hdr.win32_funcs->SQLSetEnvAttr( env->hdr.win32_handle, attr, value, len );
    }

    return ret;
}

static SQLRETURN alloc_env_handle( struct environment *env, BOOL is_unix )
{
    SQLRETURN ret = SQL_ERROR;

    if (is_unix)
    {
        struct SQLAllocHandle_params params = { SQL_HANDLE_ENV, 0, &env->hdr.unix_handle };
        ret = ODBC_CALL( SQLAllocHandle, &params );
    }
    else
    {
        if (env->hdr.win32_funcs->SQLAllocHandle)
            ret = env->hdr.win32_funcs->SQLAllocHandle( SQL_HANDLE_ENV, NULL, &env->hdr.win32_handle );
        else if (env->hdr.win32_funcs->SQLAllocEnv)
            ret = env->hdr.win32_funcs->SQLAllocEnv( &env->hdr.win32_handle );
    }

    return ret;
}

#define INT_PTR(val) (SQLPOINTER)(ULONG_PTR)val
static void prepare_env( struct environment *env )
{
    if (set_env_attr( env, SQL_ATTR_ODBC_VERSION, INT_PTR(env->attr_version), 0 ))
        WARN( "failed to set ODBC version\n" );
}

static SQLRETURN create_env( struct environment *env, BOOL is_unix )
{
    SQLRETURN ret;
    if ((ret = alloc_env_handle( env, is_unix ))) return ret;
    prepare_env( env );
    return SQL_SUCCESS;
}

static SQLRETURN set_con_attr( struct connection *con, SQLINTEGER attr, SQLPOINTER value, SQLINTEGER len )
{
    SQLRETURN ret = SQL_ERROR;

    if (con->hdr.unix_handle)
    {
        struct SQLSetConnectAttr_params params = { con->hdr.unix_handle, attr, value, len };
        ret = ODBC_CALL( SQLSetConnectAttr, &params );
    }
    else if (con->hdr.win32_handle)
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
        if (con->hdr.win32_funcs->SQLSetConnectAttrW)
            ret = con->hdr.win32_funcs->SQLSetConnectAttrW( con->hdr.win32_handle, attr, value, len );
        else if (con->hdr.win32_funcs->SQLSetConnectAttr)
            ret = con->hdr.win32_funcs->SQLSetConnectAttr( con->hdr.win32_handle, attr, value, len );
    }

    return ret;
}

static void prepare_con( struct connection *con )
{
    if (set_con_attr( con, SQL_ATTR_CONNECTION_TIMEOUT, INT_PTR(con->attr_con_timeout), 0 ))
        WARN( "failed to set connection timeout\n" );
    if (set_con_attr( con, SQL_ATTR_LOGIN_TIMEOUT, INT_PTR(con->attr_login_timeout), 0 ))
        WARN( "failed to set login timeout\n" );
}

static SQLRETURN create_con( struct connection *con )
{
    SQLRETURN ret;
    if ((ret = alloc_handle( SQL_HANDLE_DBC, con->hdr.parent, &con->hdr ))) return ret;
    prepare_con( con );
    return SQL_SUCCESS;
}

static SQLRETURN connect_win32_a( struct connection *con, SQLCHAR *servername, SQLSMALLINT len1, SQLCHAR *username,
                                  SQLSMALLINT len2, SQLCHAR *auth, SQLSMALLINT len3 )
{
    SQLRETURN ret = SQL_ERROR;
    SQLWCHAR *servernameW = NULL, *usernameW = NULL, *authW = NULL;

    if (con->hdr.win32_funcs->SQLConnect)
        return con->hdr.win32_funcs->SQLConnect( con->hdr.win32_handle, servername, len1, username, len2, auth, len3 );

    if (con->hdr.win32_funcs->SQLConnectW)
    {
        if (!(servernameW = strnAtoW( servername, len1 ))) return SQL_ERROR;
        if (!(usernameW = strnAtoW( username, len2 ))) goto done;
        if (!(authW = strnAtoW( auth, len3 ))) goto done;
        ret = con->hdr.win32_funcs->SQLConnectW( con->hdr.win32_handle, servernameW, len1, usernameW, len2, authW, len3 );
    }
done:
    free( servernameW );
    free( usernameW );
    free( authW );
    return ret;
}

static SQLRETURN connect_unix_a( struct connection *con, SQLCHAR *servername, SQLSMALLINT len1, SQLCHAR *username,
                                 SQLSMALLINT len2, SQLCHAR *authentication, SQLSMALLINT len3 )
{
    struct SQLConnect_params params = { con->hdr.unix_handle, servername, len1, username, len2, authentication, len3 };
    return ODBC_CALL( SQLConnect, &params );
}

/*************************************************************************
 *				SQLConnect           [ODBC32.007]
 */
SQLRETURN WINAPI SQLConnect(SQLHDBC ConnectionHandle, SQLCHAR *ServerName, SQLSMALLINT NameLength1,
                            SQLCHAR *UserName, SQLSMALLINT NameLength2, SQLCHAR *Authentication,
                            SQLSMALLINT NameLength3)
{
    struct connection *con = (struct connection *)lock_object( ConnectionHandle, SQL_HANDLE_DBC );
    WCHAR *filename = NULL, *servername = strdupAW( (const char *)ServerName );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(ConnectionHandle %p, ServerName %s, NameLength1 %d, UserName %s, NameLength2 %d, Authentication %s,"
          " NameLength3 %d)\n", ConnectionHandle,
          debugstr_sqlstr(ServerName, NameLength1), NameLength1, debugstr_sqlstr(UserName, NameLength2),
          NameLength2, debugstr_sqlstr(Authentication, NameLength3), NameLength3);

    if (!con) return SQL_INVALID_HANDLE;

    if (!(filename = get_driver_filename_from_source( servername )))
    {
        WARN( "can't find driver filename\n" );
        goto done;
    }

    if (has_suffix( filename, L".dll" ))
    {
        if (!(con->hdr.win32_funcs = con->hdr.parent->win32_funcs = load_driver( filename )))
        {
            WARN( "failed to load driver %s\n", debugstr_w(filename) );
            goto done;
        }
        TRACE( "using Windows driver %s\n", debugstr_w(filename) );

        if (!SUCCESS((ret = create_env( (struct environment *)con->hdr.parent, FALSE )))) goto done;
        if (!SUCCESS((ret = create_con( con )))) goto done;

        ret = connect_win32_a( con, ServerName, NameLength1, UserName, NameLength2, Authentication, NameLength3 );
    }
    else
    {
        TRACE( "using Unix driver %s\n", debugstr_w(filename) );

        if (!SUCCESS((ret = create_env( (struct environment *)con->hdr.parent, TRUE )))) goto done;
        if (!SUCCESS((ret = create_con( con )))) goto done;

        ret = connect_unix_a( con, ServerName, NameLength1, UserName, NameLength2, Authentication, NameLength3 );
    }

done:
    free( servername );
    free( filename );
    TRACE("Returning %d\n", ret);
    unlock_object( &con->hdr );
    return ret;
}

static SQLRETURN copy_desc_unix( struct descriptor *source, struct descriptor *target )
{
    struct SQLCopyDesc_params params = { source->hdr.unix_handle, target->hdr.unix_handle };
    return ODBC_CALL( SQLCopyDesc, &params );
}

static SQLRETURN copy_desc_win32( struct descriptor *source, struct descriptor *target )
{
    if (source->hdr.win32_funcs->SQLCopyDesc)
        return source->hdr.win32_funcs->SQLCopyDesc( source->hdr.win32_handle, target->hdr.win32_handle );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLCopyDesc           [ODBC32.028]
 */
SQLRETURN WINAPI SQLCopyDesc(SQLHDESC SourceDescHandle, SQLHDESC TargetDescHandle)
{
    struct descriptor *source = (struct descriptor *)lock_object( SourceDescHandle, SQL_HANDLE_DESC );
    struct descriptor *target = (struct descriptor *)lock_object( TargetDescHandle, SQL_HANDLE_DESC );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(SourceDescHandle %p, TargetDescHandle %p)\n", SourceDescHandle, TargetDescHandle);

    if (!source || !target)
    {
        if (source) unlock_object( &source->hdr );
        if (target) unlock_object( &target->hdr );
        return SQL_INVALID_HANDLE;
    }

    if (source->hdr.unix_handle)
    {
        ret = copy_desc_unix( source, target );
    }
    else if (source->hdr.win32_handle)
    {
        ret = copy_desc_win32( source, target );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &source->hdr );
    unlock_object( &target->hdr );
    return ret;
}

/*************************************************************************
 *				SQLDataSources           [ODBC32.057]
 */
SQLRETURN WINAPI SQLDataSources(SQLHENV EnvironmentHandle, SQLUSMALLINT Direction, SQLCHAR *ServerName,
                                SQLSMALLINT BufferLength1, SQLSMALLINT *NameLength1, SQLCHAR *Description,
                                SQLSMALLINT BufferLength2, SQLSMALLINT *NameLength2)
{
    struct environment *env = (struct environment *)lock_object( EnvironmentHandle, SQL_HANDLE_ENV );
    SQLRETURN ret = SQL_ERROR;
    DWORD len_source = BufferLength1, len_desc = BufferLength2;
    LONG res;

    TRACE("(EnvironmentHandle %p, Direction %d, ServerName %p, BufferLength1 %d, NameLength1 %p, Description %p,"
          " BufferLength2 %d, NameLength2 %p)\n", EnvironmentHandle, Direction, ServerName, BufferLength1,
          NameLength1, Description, BufferLength2, NameLength2);

    if (!env) return SQL_INVALID_HANDLE;

    if (Direction == SQL_FETCH_FIRST || (Direction == SQL_FETCH_NEXT && !env->sources_key))
    {
        env->sources_idx = 0;
        env->sources_system = FALSE;
        RegCloseKey( env->sources_key );
        if (!(env->sources_key = open_sources_key( HKEY_CURRENT_USER ))) goto done;
    }

    res = RegEnumValueA( env->sources_key, env->sources_idx, (char *)ServerName, &len_source, NULL,
                         NULL, (BYTE *)Description, &len_desc );
    if (res == ERROR_NO_MORE_ITEMS)
    {
        if (env->sources_system)
        {
            ret = SQL_NO_DATA;
            goto done;
        }
        /* user key exhausted, continue with system key */
        RegCloseKey( env->sources_key );
        if (!(env->sources_key = open_sources_key( HKEY_LOCAL_MACHINE ))) goto done;
        env->sources_idx = 0;
        env->sources_system = TRUE;
        res = RegEnumValueA( env->sources_key, env->sources_idx, (char *)ServerName, &len_source, NULL,
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

        env->sources_idx++;
        ret = SQL_SUCCESS;
    }

done:
    if (ret)
    {
        RegCloseKey( env->sources_key );
        env->sources_key = NULL;
        env->sources_idx = 0;
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &env->hdr );
    return ret;
}

static SQLRETURN describe_col_unix_a( struct statement *stmt, SQLUSMALLINT col_number, SQLCHAR *col_name,
                                      SQLSMALLINT buflen, SQLSMALLINT *retlen, SQLSMALLINT *data_type,
                                      SQLULEN *col_size, SQLSMALLINT *decimal_digits, SQLSMALLINT *nullable )
{
    SQLRETURN ret;
    SQLSMALLINT dummy;
    UINT64 size;
    struct SQLDescribeCol_params params = { stmt->hdr.unix_handle, col_number, col_name, buflen, retlen, data_type,
                                            &size, decimal_digits, nullable };
    if (!retlen) params.NameLength = &dummy; /* workaround for drivers that don't accept NULL NameLength */

    if (SUCCESS((ret = ODBC_CALL( SQLDescribeCol, &params ))) && col_size) *col_size = size;
    return ret;
}

static SQLRETURN describe_col_win32_a( struct statement *stmt, SQLUSMALLINT col_number, SQLCHAR *col_name,
                                       SQLSMALLINT buflen, SQLSMALLINT *retlen, SQLSMALLINT *data_type,
                                       SQLULEN *col_size, SQLSMALLINT *decimal_digits, SQLSMALLINT *nullable )
{
    SQLRETURN ret = SQL_ERROR;

    if (stmt->hdr.win32_funcs->SQLDescribeCol)
        return stmt->hdr.win32_funcs->SQLDescribeCol( stmt->hdr.win32_handle, col_number, col_name, buflen, retlen,
                                                      data_type, col_size, decimal_digits, nullable );

    if (stmt->hdr.win32_funcs->SQLDescribeColW)
    {
        SQLWCHAR *nameW;
        SQLSMALLINT lenW;

        if (!(nameW = malloc( buflen * sizeof(WCHAR) ))) return SQL_ERROR;
        ret = stmt->hdr.win32_funcs->SQLDescribeColW( stmt->hdr.win32_handle, col_number, nameW, buflen, &lenW,
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
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, ColumnNumber %d, ColumnName %p, BufferLength %d, NameLength %p, DataType %p,"
          " ColumnSize %p, DecimalDigits %p, Nullable %p)\n", StatementHandle, ColumnNumber, ColumnName,
          BufferLength, NameLength, DataType, ColumnSize, DecimalDigits, Nullable);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = describe_col_unix_a( stmt, ColumnNumber, ColumnName, BufferLength, NameLength, DataType,
                                   ColumnSize, DecimalDigits, Nullable );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = describe_col_win32_a( stmt, ColumnNumber, ColumnName, BufferLength, NameLength, DataType,
                                    ColumnSize, DecimalDigits, Nullable );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
    return ret;
}

static SQLRETURN disconnect_unix( struct connection *con )
{
    struct SQLDisconnect_params params = { con->hdr.unix_handle };
    return ODBC_CALL( SQLDisconnect, &params );
}

static SQLRETURN disconnect_win32( struct connection *con )
{
    if (con->hdr.win32_funcs->SQLDisconnect)
        return con->hdr.win32_funcs->SQLDisconnect( con->hdr.win32_handle );
    return SQL_ERROR;
}

static void cleanup_object( struct object *obj );

static void destroy_dependent_objects( struct connection *con )
{
    struct object *obj, *next;

    LIST_FOR_EACH_ENTRY_SAFE( obj, next, &con->hdr.children, struct object, entry)
    {
        EnterCriticalSection( &obj->cs );
        cleanup_object( obj );
        obj->closed = TRUE;
        LeaveCriticalSection( &obj->cs );

        /* Unlink from the parent object */
        destroy_object( obj );
    }
}

/*************************************************************************
 *				SQLDisconnect           [ODBC32.009]
 */
SQLRETURN WINAPI SQLDisconnect(SQLHDBC ConnectionHandle)
{
    struct connection *con = (struct connection *)lock_object( ConnectionHandle, SQL_HANDLE_DBC );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(ConnectionHandle %p)\n", ConnectionHandle);

    if (!con) return SQL_INVALID_HANDLE;

    if (con->hdr.unix_handle)
    {
        ret = disconnect_unix( con );
    }
    else if (con->hdr.win32_handle)
    {
        ret = disconnect_win32( con );
    }

    /* Driver drops allocated statements automatically. After successful disconnect
       it's possible to free connection handle right away. */
    if (!ret)
        destroy_dependent_objects( con );

    TRACE("Returning %d\n", ret);
    unlock_object( &con->hdr );
    return ret;
}

static SQLRETURN end_tran_unix( SQLSMALLINT type, struct object *obj, SQLSMALLINT completion )
{
    struct SQLEndTran_params params = { type, obj->unix_handle, completion };
    return ODBC_CALL( SQLEndTran, &params );
}

static SQLRETURN end_tran_win32( SQLSMALLINT type, struct object *obj, SQLSMALLINT completion )
{
    if (obj->win32_funcs->SQLEndTran)
        return obj->win32_funcs->SQLEndTran( type, obj->win32_handle, completion );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLEndTran           [ODBC32.029]
 */
SQLRETURN WINAPI SQLEndTran(SQLSMALLINT HandleType, SQLHANDLE Handle, SQLSMALLINT CompletionType)
{
    struct object *obj = lock_object( Handle, HandleType );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(HandleType %d, Handle %p, CompletionType %d)\n", HandleType, Handle, CompletionType);

    if (!obj) return SQL_INVALID_HANDLE;

    if (obj->unix_handle)
    {
        ret = end_tran_unix( HandleType, obj, CompletionType );
    }
    else if (obj->win32_handle)
    {
        ret = end_tran_win32( HandleType, obj, CompletionType );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( obj );
    return ret;
}

static SQLRETURN error_unix_a( struct environment *env, struct connection *con, struct statement *stmt, SQLCHAR *state,
                               SQLINTEGER *native_err, SQLCHAR *msg, SQLSMALLINT buflen, SQLSMALLINT *retlen )
{
    struct SQLError_params params = { env ? env->hdr.unix_handle : 0, con ? con->hdr.unix_handle : 0,
                                      stmt ? stmt->hdr.unix_handle : 0, state, native_err, msg, buflen, retlen };
    return ODBC_CALL( SQLError, &params );
}

static SQLRETURN error_win32_a( struct environment *env, struct connection *con, struct statement *stmt, SQLCHAR *state,
                                SQLINTEGER *native_err, SQLCHAR *msg, SQLSMALLINT buflen, SQLSMALLINT *retlen )
{
    const struct win32_funcs *win32_funcs;
    SQLRETURN ret = SQL_ERROR;

    if (env) win32_funcs = env->hdr.win32_funcs;
    else if (con) win32_funcs = con->hdr.win32_funcs;
    else win32_funcs = stmt->hdr.win32_funcs;

    if (win32_funcs->SQLError)
        return win32_funcs->SQLError( env ? env->hdr.win32_handle : NULL, con ? con->hdr.win32_handle : NULL,
                                      stmt ? stmt->hdr.win32_handle : NULL, state, native_err, msg, buflen, retlen );

    if (win32_funcs->SQLErrorW)
    {
        SQLWCHAR stateW[6], *msgW = NULL;
        SQLSMALLINT lenW;

        if (!(msgW = malloc( buflen * sizeof(SQLWCHAR) ))) return SQL_ERROR;
        ret = win32_funcs->SQLErrorW( env ? env->hdr.win32_handle : NULL, con ? con->hdr.win32_handle : NULL,
                                      stmt ? stmt->hdr.win32_handle : NULL, stateW, native_err, msgW, buflen, &lenW );
        if (SUCCESS( ret ))
        {
            int len = WideCharToMultiByte( CP_ACP, 0, msgW, -1, (char *)msg, buflen, NULL, NULL );
            if (retlen) *retlen = len - 1;
            WideCharToMultiByte( CP_ACP, 0, stateW, -1, (char *)state, 6, NULL, NULL );
        }
        free( msgW );
    }

    if (win32_funcs->SQLGetDiagRec) FIXME("Use SQLGetDiagRec\n");
    else if (win32_funcs->SQLGetDiagRecW) FIXME("Use SQLGetDiagRecW\n");

    return ret;
}

/*************************************************************************
 *				SQLError           [ODBC32.010]
 */
SQLRETURN WINAPI SQLError(SQLHENV EnvironmentHandle, SQLHDBC ConnectionHandle, SQLHSTMT StatementHandle,
                          SQLCHAR *SqlState, SQLINTEGER *NativeError, SQLCHAR *MessageText,
                          SQLSMALLINT BufferLength, SQLSMALLINT *TextLength)
{
    struct environment *env = (struct environment *)lock_object( EnvironmentHandle, SQL_HANDLE_ENV );
    struct connection *con = (struct connection *)lock_object( ConnectionHandle, SQL_HANDLE_DBC );
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(EnvironmentHandle %p, ConnectionHandle %p, StatementHandle %p, SqlState %p, NativeError %p,"
          " MessageText %p, BufferLength %d, TextLength %p)\n", EnvironmentHandle, ConnectionHandle,
          StatementHandle, SqlState, NativeError, MessageText, BufferLength, TextLength);

    if (!env && !con && !stmt) return SQL_INVALID_HANDLE;

    if ((env && env->hdr.unix_handle) || (con && con->hdr.unix_handle) || (stmt && stmt->hdr.unix_handle))
    {
        ret = error_unix_a( env, con, stmt, SqlState, NativeError, MessageText, BufferLength, TextLength );
    }
    else if ((env && env->hdr.win32_handle) || (con && con->hdr.win32_handle) || (stmt && stmt->hdr.win32_handle))
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
    if (env) unlock_object( &env->hdr );
    if (con) unlock_object( &con->hdr );
    if (stmt) unlock_object( &stmt->hdr );
    return ret;
}

static SQLRETURN exec_direct_unix_a( struct statement *stmt, SQLCHAR *text, SQLINTEGER len )
{
    struct SQLExecDirect_params params = { stmt->hdr.unix_handle, text, len };
    return ODBC_CALL( SQLExecDirect, &params );
}

static SQLRETURN exec_direct_win32_a( struct statement *stmt, SQLCHAR *text, SQLINTEGER len )
{
    SQLRETURN ret = SQL_ERROR;
    SQLWCHAR *textW;

    if (stmt->hdr.win32_funcs->SQLExecDirect)
        return stmt->hdr.win32_funcs->SQLExecDirect( stmt->hdr.win32_handle, text, len );

    if (stmt->hdr.win32_funcs->SQLExecDirectW)
    {
        if (!(textW = strnAtoW( text, len ))) return SQL_ERROR;
        ret = stmt->hdr.win32_funcs->SQLExecDirectW( stmt->hdr.win32_handle, textW, len );
        free( textW );
    }
    return ret;
}

/*************************************************************************
 *				SQLExecDirect           [ODBC32.011]
 */
SQLRETURN WINAPI SQLExecDirect(SQLHSTMT StatementHandle, SQLCHAR *StatementText, SQLINTEGER TextLength)
{
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, StatementText %s, TextLength %d)\n", StatementHandle,
          debugstr_sqlstr(StatementText, TextLength), TextLength);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = exec_direct_unix_a( stmt, StatementText, TextLength );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = exec_direct_win32_a( stmt, StatementText, TextLength );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
    return ret;
}

static void len_to_user( SQLLEN *ptr, UINT8 *len, UINT row_count, UINT width )
{
    UINT i;

    if (ptr == NULL) return;

    for (i = 0; i < row_count; i++)
    {
        *ptr++ = *(SQLLEN *)(len + i * width);
    }
}

static void len_from_user( UINT8 *len, SQLLEN *ptr, UINT row_count, UINT width )
{
    UINT i;

    if (ptr == NULL) return;

    for (i = 0; i < row_count; i++)
    {
        *(SQLLEN *)(len + i * width) = *ptr++;
    }
}

static void update_result_lengths( struct statement *stmt, USHORT type )
{
    UINT i, width = is_old_wow64 ? 4 : 8;

    if (!is_wow64 || is_old_wow64) return;

    switch (type)
    {
    case SQL_PARAM_OUTPUT:
        for (i = 0; i < stmt->bind_col.count; i++)
        {
            len_to_user( stmt->bind_col.param[i].ptr, stmt->bind_col.param[i].len, stmt->row_count, width );
        }
        for (i = 0; i < stmt->bind_parameter.count; i++)
        {
            if (stmt->bind_parameter.param[i].type != SQL_PARAM_OUTPUT &&
                stmt->bind_parameter.param[i].type != SQL_PARAM_INPUT_OUTPUT) continue;

            len_to_user( stmt->bind_parameter.param[i].ptr, stmt->bind_parameter.param[i].len, stmt->row_count, width );
        }
        break;

    case SQL_PARAM_INPUT:
        for (i = 0; i < stmt->bind_col.count; i++)
        {
            len_from_user( stmt->bind_col.param[i].len, stmt->bind_col.param[i].ptr, stmt->row_count, width );
        }
        for (i = 0; i < stmt->bind_parameter.count; i++)
        {
            if (stmt->bind_parameter.param[i].type != SQL_PARAM_INPUT &&
                stmt->bind_parameter.param[i].type != SQL_PARAM_INPUT_OUTPUT) continue;

            len_from_user( stmt->bind_parameter.param[i].len, stmt->bind_parameter.param[i].ptr, stmt->row_count, width );
        }

    default: break;
    }
}

static SQLRETURN execute_unix( struct statement *stmt )
{
    struct SQLExecute_params params = { stmt->hdr.unix_handle };
    SQLRETURN ret;

    update_result_lengths( stmt, SQL_PARAM_INPUT );
    if (SUCCESS((ret = ODBC_CALL( SQLExecute, &params )))) update_result_lengths( stmt, SQL_PARAM_OUTPUT );
    return ret;
}

static SQLRETURN execute_win32( struct statement *stmt )
{
    if (stmt->hdr.win32_funcs->SQLExecute)
        return stmt->hdr.win32_funcs->SQLExecute( stmt->hdr.win32_handle );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLExecute           [ODBC32.012]
 */
SQLRETURN WINAPI SQLExecute(SQLHSTMT StatementHandle)
{
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p)\n", StatementHandle);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = execute_unix( stmt );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = execute_win32( stmt );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
    return ret;
}

static SQLRETURN fetch_unix( struct statement *stmt )
{
    struct SQLFetch_params params = { stmt->hdr.unix_handle };
    SQLRETURN ret;

    if (SUCCESS(( ret = ODBC_CALL( SQLFetch, &params )))) update_result_lengths( stmt, SQL_PARAM_OUTPUT );
    return ret;
}

static SQLRETURN fetch_win32( struct statement *stmt )
{
    if (stmt->hdr.win32_funcs->SQLFetch)
        return stmt->hdr.win32_funcs->SQLFetch( stmt->hdr.win32_handle );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLFetch           [ODBC32.013]
 */
SQLRETURN WINAPI SQLFetch(SQLHSTMT StatementHandle)
{
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p)\n", StatementHandle);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = fetch_unix( stmt );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = fetch_win32( stmt );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
    return ret;
}

static SQLRETURN fetch_scroll_unix( struct statement *stmt, SQLSMALLINT orientation, SQLLEN offset )
{
    struct SQLFetchScroll_params params = { stmt->hdr.unix_handle, orientation, offset };
    SQLRETURN ret;

    if (SUCCESS((ret = ODBC_CALL( SQLFetchScroll, &params )))) update_result_lengths( stmt, SQL_PARAM_OUTPUT );
    return ret;
}

static SQLRETURN fetch_scroll_win32( struct statement *stmt, SQLSMALLINT orientation, SQLLEN offset )
{
    if (stmt->hdr.win32_funcs->SQLFetchScroll)
        return stmt->hdr.win32_funcs->SQLFetchScroll( stmt->hdr.win32_handle, orientation, offset );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLFetchScroll          [ODBC32.030]
 */
SQLRETURN WINAPI SQLFetchScroll(SQLHSTMT StatementHandle, SQLSMALLINT FetchOrientation, SQLLEN FetchOffset)
{
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, FetchOrientation %d, FetchOffset %s)\n", StatementHandle, FetchOrientation,
          debugstr_sqllen(FetchOffset));

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = fetch_scroll_unix( stmt, FetchOrientation, FetchOffset );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = fetch_scroll_win32( stmt, FetchOrientation, FetchOffset );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
    return ret;
}

static SQLRETURN free_handle( SQLSMALLINT type, struct object *obj )
{
    SQLRETURN ret = SQL_SUCCESS;

    if (obj->unix_handle)
    {
        struct SQLFreeHandle_params params = { type, obj->unix_handle };
        ret = ODBC_CALL( SQLFreeHandle, &params );
    }
    else if (obj->win32_handle)
    {
        if (obj->win32_funcs->SQLFreeHandle)
            ret = obj->win32_funcs->SQLFreeHandle( type, obj->win32_handle );
        else
        {
            switch (type)
            {
            case SQL_HANDLE_ENV:
                if (obj->win32_funcs->SQLFreeEnv)
                    ret = obj->win32_funcs->SQLFreeEnv( obj->win32_handle );
                break;
            case SQL_HANDLE_DBC:
                if (obj->win32_funcs->SQLFreeConnect)
                    ret = obj->win32_funcs->SQLFreeConnect( obj->win32_handle );
                break;
            case SQL_HANDLE_STMT:
                if (obj->win32_funcs->SQLFreeStmt)
                    ret = obj->win32_funcs->SQLFreeStmt( obj->win32_handle, SQL_DROP );
                break;
            default: break;
            }
        }
    }

    return ret;
}

/*************************************************************************
 *				SQLFreeConnect           [ODBC32.014]
 */
SQLRETURN WINAPI SQLFreeConnect(SQLHDBC ConnectionHandle)
{
    struct connection *con = (struct connection *)lock_object( ConnectionHandle, SQL_HANDLE_DBC );
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p)\n", ConnectionHandle);

    if (!con) return SQL_INVALID_HANDLE;

    if (!list_empty( &con->hdr.children )) ret = SQL_ERROR;
    else
    {
        ret = free_handle( SQL_HANDLE_DBC, &con->hdr );
        con->hdr.closed = TRUE;
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &con->hdr );
    if (con->hdr.closed) destroy_object( &con->hdr );
    return ret;
}

/*************************************************************************
 *				SQLFreeEnv           [ODBC32.015]
 */
SQLRETURN WINAPI SQLFreeEnv(SQLHENV EnvironmentHandle)
{
    struct environment *env = (struct environment *)lock_object( EnvironmentHandle, SQL_HANDLE_ENV );
    SQLRETURN ret;

    TRACE("(EnvironmentHandle %p)\n", EnvironmentHandle);

    if (!env) return SQL_INVALID_HANDLE;

    if (!list_empty( &env->hdr.children )) ret = SQL_ERROR;
    else
    {
        ret = free_handle( SQL_HANDLE_ENV, &env->hdr );

        RegCloseKey( env->drivers_key );
        RegCloseKey( env->sources_key );
        env->hdr.closed = TRUE;
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &env->hdr );
    if (env->hdr.closed) destroy_object( &env->hdr );
    return ret;
}

static void free_col_bindings( struct statement *stmt )
{
    if (stmt->bind_col.param)
    {
        free( stmt->bind_col.param->len );
        free( stmt->bind_col.param );
        stmt->bind_col.param = NULL;
    }
}

static void free_param_bindings( struct statement *stmt )
{
    if (stmt->bind_parameter.param)
    {
        free( stmt->bind_parameter.param->len );
        free( stmt->bind_parameter.param );
        stmt->bind_parameter.param = NULL;
    }
}

static void cleanup_object( struct object *obj )
{
    switch (obj->type)
    {
    case SQL_HANDLE_ENV:
    {
        struct environment *env = (struct environment *)obj;
        RegCloseKey( env->drivers_key );
        RegCloseKey( env->sources_key );
        env->drivers_key = env->sources_key = NULL;
        env->drivers_idx = env->sources_idx = 0;
        break;
    }
    case SQL_HANDLE_STMT:
    {
        struct statement *stmt = (struct statement *)obj;
        free_col_bindings( stmt );
        free_param_bindings( stmt );
        free_descriptors( stmt );
        break;
    }
    default: break;
    }
}

/*************************************************************************
 *				SQLFreeHandle           [ODBC32.031]
 */
SQLRETURN WINAPI SQLFreeHandle(SQLSMALLINT HandleType, SQLHANDLE Handle)
{
    struct object *obj = lock_object( Handle, HandleType );
    SQLRETURN ret;

    TRACE("(HandleType %d, Handle %p)\n", HandleType, Handle);

    if (!obj) return SQL_INVALID_HANDLE;

    if (!list_empty( &obj->children )) ret = SQL_ERROR;
    else
    {
        ret = free_handle( HandleType, obj );
        obj->closed = TRUE;

        cleanup_object( obj );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( obj );
    if (obj->closed) destroy_object( obj );
    return ret;
}

static SQLRETURN free_statement( struct statement *stmt, SQLUSMALLINT option )
{
    SQLRETURN ret = SQL_ERROR;

    if (stmt->hdr.unix_handle)
    {
        struct SQLFreeStmt_params params = { stmt->hdr.unix_handle, option };
        ret = ODBC_CALL( SQLFreeStmt, &params );
    }
    else if (stmt->hdr.win32_handle)
    {
        if (stmt->hdr.win32_funcs->SQLFreeStmt)
            ret = stmt->hdr.win32_funcs->SQLFreeStmt( stmt->hdr.win32_handle, option );
    }

    return ret;
}

/*************************************************************************
 *				SQLFreeStmt           [ODBC32.016]
 */
SQLRETURN WINAPI SQLFreeStmt(SQLHSTMT StatementHandle, SQLUSMALLINT Option)
{
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, Option %d)\n", StatementHandle, Option);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (!list_empty( &stmt->hdr.children )) ret = SQL_ERROR;
    else
    {
        ret = free_statement( stmt, Option );

        switch (Option)
        {
        case SQL_CLOSE:
            break;

        case SQL_UNBIND:
            free_col_bindings( stmt );
            break;

        case SQL_RESET_PARAMS:
            free_param_bindings( stmt );
            break;

        case SQL_DROP:
        default:
            free_col_bindings( stmt );
            free_param_bindings( stmt );
            free_descriptors( stmt );
            stmt->hdr.closed = TRUE;
            break;
        }
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
    if (stmt->hdr.closed) destroy_object( &stmt->hdr );
    return ret;
}

static SQLRETURN get_connect_attr_unix_a( struct connection *con, SQLINTEGER attr, SQLPOINTER value,
                                          SQLINTEGER buflen, SQLINTEGER *retlen )
{
    struct SQLGetConnectAttr_params params = { con->hdr.unix_handle, attr, value, buflen, retlen };
    return ODBC_CALL( SQLGetConnectAttr, &params );
}

static SQLRETURN get_connect_attr_win32_a( struct connection *con, SQLINTEGER attr, SQLPOINTER value,
                                           SQLINTEGER buflen, SQLINTEGER *retlen )
{
    SQLRETURN ret = SQL_ERROR;
    SQLPOINTER val = value;
    SQLWCHAR *strW = NULL;

    if (con->hdr.win32_funcs->SQLGetConnectAttr)
        return con->hdr.win32_funcs->SQLGetConnectAttr( con->hdr.win32_handle, attr, value, buflen, retlen );

    if (con->hdr.win32_funcs->SQLGetConnectAttrW)
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

        ret = con->hdr.win32_funcs->SQLGetConnectAttrW( con->hdr.win32_handle, attr, val, buflen, retlen );
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
    struct connection *con = (struct connection *)lock_object( ConnectionHandle, SQL_HANDLE_DBC );
    SQLRETURN ret = SQL_SUCCESS;

    TRACE("(ConnectionHandle %p, Attribute %d, Value %p, BufferLength %d, StringLength %p)\n", ConnectionHandle,
          Attribute, Value, BufferLength, StringLength);

    if (!con) return SQL_INVALID_HANDLE;

    if (con->hdr.unix_handle)
    {
        ret = get_connect_attr_unix_a( con, Attribute, Value, BufferLength, StringLength );
    }
    else if (con->hdr.win32_handle)
    {
        ret = get_connect_attr_win32_a( con, Attribute, Value, BufferLength, StringLength );
    }
    else
    {
        switch (Attribute)
        {
        case SQL_ATTR_CONNECTION_TIMEOUT:
            *(SQLINTEGER *)Value = con->attr_con_timeout;
            break;

        case SQL_ATTR_LOGIN_TIMEOUT:
            *(SQLINTEGER *)Value = con->attr_login_timeout;
            break;

        default:
            FIXME( "unhandled attribute %d\n", Attribute );
            ret = SQL_ERROR;
            break;
        }
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &con->hdr );
    return ret;
}

static SQLRETURN get_connect_option_unix_a( struct connection *con, SQLUSMALLINT option, SQLPOINTER value )
{
    struct SQLGetConnectOption_params params = { con->hdr.unix_handle, option, value };
    return ODBC_CALL( SQLGetConnectOption, &params );
}

static SQLRETURN get_connect_option_win32_a( struct connection *con, SQLUSMALLINT option, SQLPOINTER value )
{
    SQLRETURN ret = SQL_ERROR;

    if (con->hdr.win32_funcs->SQLGetConnectOption)
        return con->hdr.win32_funcs->SQLGetConnectOption( con->hdr.win32_handle, option, value );

    if (con->hdr.win32_funcs->SQLGetConnectOptionW)
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
        ret = con->hdr.win32_funcs->SQLGetConnectOptionW( con->hdr.win32_handle, option, value );
    }
    return ret;
}

/*************************************************************************
 *				SQLGetConnectOption       [ODBC32.042]
 */
SQLRETURN WINAPI SQLGetConnectOption(SQLHDBC ConnectionHandle, SQLUSMALLINT Option, SQLPOINTER Value)
{
    struct connection *con = (struct connection *)lock_object( ConnectionHandle, SQL_HANDLE_DBC );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(ConnectionHandle %p, Option %d, Value %p)\n", ConnectionHandle, Option, Value);

    if (!con) return SQL_INVALID_HANDLE;

    if (con->hdr.unix_handle)
    {
        ret = get_connect_option_unix_a( con, Option, Value );
    }
    else if (con->hdr.win32_handle)
    {
        ret = get_connect_option_win32_a( con, Option, Value );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &con->hdr );
    return ret;
}

static SQLRETURN get_cursor_name_unix_a( struct statement *stmt, SQLCHAR *name, SQLSMALLINT buflen,
                                         SQLSMALLINT *retlen )
{
    struct SQLGetCursorName_params params = { stmt->hdr.unix_handle, name, buflen, retlen };
    return ODBC_CALL( SQLGetCursorName, &params );
}

static SQLRETURN get_cursor_name_win32_a( struct statement *stmt, SQLCHAR *name, SQLSMALLINT buflen,
                                          SQLSMALLINT *retlen )
{
   SQLRETURN ret = SQL_ERROR;

    if (stmt->hdr.win32_funcs->SQLGetCursorName)
        return stmt->hdr.win32_funcs->SQLGetCursorName( stmt->hdr.win32_handle, name, buflen, retlen );

    if (stmt->hdr.win32_funcs->SQLGetCursorNameW)
    {
        SQLWCHAR *nameW;
        SQLSMALLINT lenW;

        if (!(nameW = malloc( buflen * sizeof(WCHAR) ))) return SQL_ERROR;
        ret = stmt->hdr.win32_funcs->SQLGetCursorNameW( stmt->hdr.win32_handle, nameW, buflen, &lenW );
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
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, CursorName %p, BufferLength %d, NameLength %p)\n", StatementHandle, CursorName,
          BufferLength, NameLength);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = get_cursor_name_unix_a( stmt, CursorName, BufferLength, NameLength );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = get_cursor_name_win32_a( stmt, CursorName, BufferLength, NameLength );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
    return ret;
}

static SQLRETURN get_data_unix( struct statement *stmt, SQLUSMALLINT column, SQLSMALLINT type, SQLPOINTER value,
                                SQLLEN buflen, SQLLEN *retlen )
{
    INT64 len;
    SQLRETURN ret;
    struct SQLGetData_params params = { stmt->hdr.unix_handle, column, type, value, buflen, retlen ? &len : NULL};

    if (SUCCESS((ret = ODBC_CALL( SQLGetData, &params ))) && retlen) *retlen = len;
    return ret;
}

static SQLRETURN get_data_win32( struct statement *stmt, SQLUSMALLINT column, SQLSMALLINT type, SQLPOINTER value,
                                 SQLLEN buflen, SQLLEN *retlen )
{
    if (stmt->hdr.win32_funcs->SQLGetData)
        return stmt->hdr.win32_funcs->SQLGetData( stmt->hdr.win32_handle, column, type, value, buflen, retlen );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLGetData           [ODBC32.043]
 */
SQLRETURN WINAPI SQLGetData(SQLHSTMT StatementHandle, SQLUSMALLINT ColumnNumber, SQLSMALLINT TargetType,
                            SQLPOINTER TargetValue, SQLLEN BufferLength, SQLLEN *StrLen_or_Ind)
{
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, ColumnNumber %d, TargetType %d, TargetValue %p, BufferLength %s, StrLen_or_Ind %p)\n",
          StatementHandle, ColumnNumber, TargetType, TargetValue, debugstr_sqllen(BufferLength), StrLen_or_Ind);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = get_data_unix( stmt, ColumnNumber, TargetType, TargetValue, BufferLength, StrLen_or_Ind );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = get_data_win32( stmt, ColumnNumber, TargetType, TargetValue, BufferLength, StrLen_or_Ind );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
    return ret;
}

static SQLRETURN get_desc_field_unix_a( struct descriptor *desc, SQLSMALLINT record, SQLSMALLINT field_id,
                                        SQLPOINTER value, SQLINTEGER buflen, SQLINTEGER *retlen )
{
    struct SQLGetDescField_params params = { desc->hdr.unix_handle, record, field_id, value, buflen, retlen };
    return ODBC_CALL( SQLGetDescField, &params );
}

static SQLRETURN get_desc_field_win32_a( struct descriptor *desc, SQLSMALLINT record, SQLSMALLINT field_id,
                                         SQLPOINTER value, SQLINTEGER buflen, SQLINTEGER *retlen )
{
    SQLRETURN ret = SQL_ERROR;
    SQLPOINTER val = value;
    SQLWCHAR *strW = NULL;

    if (desc->hdr.win32_funcs->SQLGetDescField)
        return desc->hdr.win32_funcs->SQLGetDescField( desc->hdr.win32_handle, record, field_id, value, buflen, retlen );

    if (desc->hdr.win32_funcs->SQLGetDescFieldW)
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

        ret = desc->hdr.win32_funcs->SQLGetDescFieldW( desc->hdr.win32_handle, record, field_id, value, buflen, retlen );
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
    struct descriptor *desc = (struct descriptor *)lock_object( DescriptorHandle, SQL_HANDLE_DESC );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(DescriptorHandle %p, RecNumber %d, FieldIdentifier %d, Value %p, BufferLength %d, StringLength %p)\n",
          DescriptorHandle, RecNumber, FieldIdentifier, Value, BufferLength, StringLength);

    if (!desc) return SQL_INVALID_HANDLE;

    if (desc->hdr.unix_handle)
    {
        ret = get_desc_field_unix_a( desc, RecNumber, FieldIdentifier, Value, BufferLength, StringLength );
    }
    else if (desc->hdr.win32_handle)
    {
        ret = get_desc_field_win32_a( desc, RecNumber, FieldIdentifier, Value, BufferLength, StringLength );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &desc->hdr );
    return ret;
}

static SQLRETURN get_desc_rec_unix_a( struct descriptor *desc, SQLSMALLINT record, SQLCHAR *name, SQLSMALLINT buflen,
                                      SQLSMALLINT *retlen, SQLSMALLINT *type, SQLSMALLINT *subtype, SQLLEN *len,
                                      SQLSMALLINT *precision, SQLSMALLINT *scale, SQLSMALLINT *nullable )
{
    SQLRETURN ret;
    INT64 len64;
    struct SQLGetDescRec_params params = { desc->hdr.unix_handle, record, name, buflen, retlen, type, subtype, &len64,
                                           precision, scale, nullable };
    if (SUCCESS((ret = ODBC_CALL( SQLGetDescRec, &params )))) *len = len64;
    return ret;
}

static SQLRETURN get_desc_rec_win32_a( struct descriptor *desc, SQLSMALLINT record, SQLCHAR *name, SQLSMALLINT buflen,
                                       SQLSMALLINT *retlen, SQLSMALLINT *type, SQLSMALLINT *subtype, SQLLEN *len,
                                       SQLSMALLINT *precision, SQLSMALLINT *scale, SQLSMALLINT *nullable )
{
    SQLRETURN ret = SQL_ERROR;

    if (desc->hdr.win32_funcs->SQLGetDescRec)
        return desc->hdr.win32_funcs->SQLGetDescRec( desc->hdr.win32_handle, record, name, buflen, retlen, type,
                                                     subtype, len, precision, scale, nullable );

    if (desc->hdr.win32_funcs->SQLGetDescRecW)
    {
        SQLWCHAR *nameW;

        if (!(nameW = malloc( buflen * sizeof(WCHAR) ))) return SQL_ERROR;
        ret = desc->hdr.win32_funcs->SQLGetDescRecW( desc->hdr.win32_handle, record, nameW, buflen, retlen, type,
                                                     subtype, len, precision, scale, nullable );
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
    struct descriptor *desc = (struct descriptor *)lock_object( DescriptorHandle, SQL_HANDLE_DESC );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(DescriptorHandle %p, RecNumber %d, Name %p, BufferLength %d, StringLength %p, Type %p, SubType %p,"
          " Length %p, Precision %p, Scale %p, Nullable %p)\n", DescriptorHandle, RecNumber, Name, BufferLength,
          StringLength, Type, SubType, Length, Precision, Scale, Nullable);

    if (!desc) return SQL_INVALID_HANDLE;

    if (desc->hdr.unix_handle)
    {
        ret = get_desc_rec_unix_a( desc, RecNumber, Name, BufferLength, StringLength, Type, SubType, Length,
                                   Precision, Scale, Nullable );
    }
    else if (desc->hdr.win32_handle)
    {
        ret = get_desc_rec_win32_a( desc, RecNumber, Name, BufferLength, StringLength, Type, SubType, Length,
                                    Precision, Scale, Nullable );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &desc->hdr );
    return ret;
}

static SQLRETURN get_diag_field_unix_a( SQLSMALLINT type, struct object *obj, SQLSMALLINT rec_num,
                                        SQLSMALLINT diag_id, SQLPOINTER diag_info, SQLSMALLINT buflen,
                                        SQLSMALLINT *retlen )
{

    struct SQLGetDiagField_params params = { type, obj->unix_handle, rec_num, diag_id, diag_info, buflen, retlen };
    return ODBC_CALL( SQLGetDiagField, &params );
}

static SQLRETURN get_diag_field_win32_a( SQLSMALLINT type, struct object *obj, SQLSMALLINT rec_num,
                                         SQLSMALLINT diag_id, SQLPOINTER diag_info, SQLSMALLINT buflen,
                                         SQLSMALLINT *retlen )
{
    SQLRETURN ret = SQL_ERROR;

    if (obj->win32_funcs->SQLGetDiagField)
        return obj->win32_funcs->SQLGetDiagField( type, obj->win32_handle, rec_num, diag_id, diag_info, buflen,
                                                  retlen );

    if (obj->win32_funcs->SQLGetDiagFieldW)
    {
        SQLWCHAR *strW;
        SQLSMALLINT lenW;

        if (buflen < 0)
            ret = obj->win32_funcs->SQLGetDiagFieldW( type, obj->win32_handle, rec_num, diag_id, diag_info, buflen,
                                                      retlen );
        else
        {
            if (!(strW = malloc( buflen * sizeof(WCHAR) ))) return SQL_ERROR;
            ret = obj->win32_funcs->SQLGetDiagFieldW( type, obj->win32_handle, rec_num, diag_id, strW, buflen, &lenW );
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
    struct object *obj = lock_object( Handle, HandleType );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(HandleType %d, Handle %p, RecNumber %d, DiagIdentifier %d, DiagInfo %p, BufferLength %d,"
          " StringLength %p)\n", HandleType, Handle, RecNumber, DiagIdentifier, DiagInfo, BufferLength, StringLength);

    if (!obj) return SQL_INVALID_HANDLE;

    if (obj->unix_handle)
    {
        ret = get_diag_field_unix_a( HandleType, obj, RecNumber, DiagIdentifier, DiagInfo, BufferLength,
                                     StringLength );
    }
    else if (obj->win32_handle)
    {
        ret = get_diag_field_win32_a( HandleType, obj, RecNumber, DiagIdentifier, DiagInfo, BufferLength,
                                      StringLength );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( obj );
    return ret;
}

static SQLRETURN get_diag_rec_unix_a( SQLSMALLINT type, struct object *obj, SQLSMALLINT rec_num,
                                      SQLCHAR *state, SQLINTEGER *native_err, SQLCHAR *msg, SQLSMALLINT buflen,
                                      SQLSMALLINT *retlen )
{
    struct SQLGetDiagRec_params params = { type, obj->unix_handle, rec_num, state, native_err, msg, buflen, retlen };
    return ODBC_CALL( SQLGetDiagRec, &params );
}

static SQLRETURN get_diag_rec_win32_a( SQLSMALLINT type, struct object *obj, SQLSMALLINT rec_num, SQLCHAR *state,
                                       SQLINTEGER *native_err, SQLCHAR *msg, SQLSMALLINT buflen, SQLSMALLINT *retlen )
{
    SQLRETURN ret;
    SQLWCHAR stateW[6], *msgW;
    SQLSMALLINT lenW;

    if (obj->win32_funcs->SQLGetDiagRec)
        return obj->win32_funcs->SQLGetDiagRec( type, obj->win32_handle, rec_num, state, native_err, msg, buflen,
                                                retlen );

    if (obj->win32_funcs->SQLGetDiagRecW)
    {
        if (!(msgW = malloc( buflen * sizeof(WCHAR) ))) return SQL_ERROR;
        ret = obj->win32_funcs->SQLGetDiagRecW( type, obj->win32_handle, rec_num, stateW, native_err, msgW, buflen,
                                                &lenW );
        if (SUCCESS( ret ))
        {
            int len = WideCharToMultiByte( CP_ACP, 0, msgW, -1, (char *)msg, buflen, NULL, NULL );
            if (retlen) *retlen = len - 1;
            WideCharToMultiByte( CP_ACP, 0, stateW, -1, (char *)state, 6, NULL, NULL );
        }
        free( msgW );
        return ret;
    }

    if (obj->win32_funcs->SQLError)
    {
        SQLHENV env = NULL;
        SQLHDBC con = NULL;
        SQLHSTMT stmt = NULL;

        if (rec_num > 1) return SQL_NO_DATA;

        switch (type)
        {
        case SQL_HANDLE_ENV:
            env = obj->win32_handle;
            break;

        case SQL_HANDLE_DBC:
            con = obj->win32_handle;
            break;

        case SQL_HANDLE_STMT:
            stmt = obj->win32_handle;
            break;

        default:
            return SQL_ERROR;
        }

        return obj->win32_funcs->SQLError( env, con, stmt, state, native_err, msg, buflen, retlen );
    }

    return SQL_ERROR;
}

/*************************************************************************
 *				SQLGetDiagRec           [ODBC32.036]
 */
SQLRETURN WINAPI SQLGetDiagRec(SQLSMALLINT HandleType, SQLHANDLE Handle, SQLSMALLINT RecNumber,
                               SQLCHAR *SqlState, SQLINTEGER *NativeError, SQLCHAR *MessageText,
                               SQLSMALLINT BufferLength, SQLSMALLINT *TextLength)
{
    struct object *obj = lock_object( Handle, HandleType );
    SQLRETURN ret = SQL_NO_DATA;

    TRACE("(HandleType %d, Handle %p, RecNumber %d, SqlState %p, NativeError %p, MessageText %p, BufferLength %d,"
          " TextLength %p)\n", HandleType, Handle, RecNumber, SqlState, NativeError, MessageText, BufferLength,
          TextLength);

    if (!obj) return SQL_INVALID_HANDLE;

    if (obj->unix_handle)
    {
        ret = get_diag_rec_unix_a( HandleType, obj, RecNumber, SqlState, NativeError, MessageText, BufferLength,
                                   TextLength );
    }
    else if (obj->win32_handle)
    {
        ret = get_diag_rec_win32_a( HandleType, obj, RecNumber, SqlState, NativeError, MessageText, BufferLength,
                                    TextLength );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( obj );
    return ret;
}

static SQLRETURN get_env_attr_unix( struct environment *env, SQLINTEGER attr, SQLPOINTER value, SQLINTEGER buflen,
                                    SQLINTEGER *retlen )
{
    struct SQLGetEnvAttr_params params = { env->hdr.unix_handle, attr, value, buflen, retlen };
    return ODBC_CALL( SQLGetEnvAttr, &params );
}

static SQLRETURN get_env_attr_win32( struct environment *env, SQLINTEGER attr, SQLPOINTER value, SQLINTEGER buflen,
                                     SQLINTEGER *retlen )
{
    if (env->hdr.win32_funcs->SQLGetEnvAttr)
        return env->hdr.win32_funcs->SQLGetEnvAttr( env->hdr.win32_handle, attr, value, buflen, retlen );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLGetEnvAttr           [ODBC32.037]
 */
SQLRETURN WINAPI SQLGetEnvAttr(SQLHENV EnvironmentHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                               SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
    struct environment *env = (struct environment *)lock_object( EnvironmentHandle, SQL_HANDLE_ENV );
    SQLRETURN ret = SQL_SUCCESS;

    TRACE("(EnvironmentHandle %p, Attribute %d, Value %p, BufferLength %d, StringLength %p)\n",
          EnvironmentHandle, Attribute, Value, BufferLength, StringLength);

    if (!env) return SQL_INVALID_HANDLE;

    if (env->hdr.unix_handle)
    {
        ret = get_env_attr_unix( env, Attribute, Value, BufferLength, StringLength );
    }
    else if (env->hdr.win32_handle)
    {
        ret = get_env_attr_win32( env, Attribute, Value, BufferLength, StringLength );
    }
    else
    {
        switch (Attribute)
        {
        case SQL_ATTR_CONNECTION_POOLING:
            *(SQLINTEGER *)Value = SQL_CP_OFF;
            break;

        case SQL_ATTR_ODBC_VERSION:
            *(SQLINTEGER *)Value = env->attr_version;
            break;

        default:
            FIXME( "unhandled attribute %d\n", Attribute );
            ret = SQL_ERROR;
            break;
        }
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &env->hdr );
    return ret;
}

static SQLRETURN get_functions_unix( struct connection *con, SQLUSMALLINT id, SQLUSMALLINT *supported )
{
    struct SQLGetFunctions_params params = { con->hdr.unix_handle, id, supported };
    return ODBC_CALL( SQLGetFunctions, &params );
}

static SQLRETURN get_functions_win32( struct connection *con, SQLUSMALLINT id, SQLUSMALLINT *supported )
{
    if (con->hdr.win32_funcs->SQLGetFunctions)
        return con->hdr.win32_funcs->SQLGetFunctions( con->hdr.win32_handle, id, supported );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLGetFunctions           [ODBC32.044]
 */
SQLRETURN WINAPI SQLGetFunctions(SQLHDBC ConnectionHandle, SQLUSMALLINT FunctionId, SQLUSMALLINT *Supported)
{
    struct connection *con = (struct connection *)lock_object( ConnectionHandle, SQL_HANDLE_DBC );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(ConnectionHandle %p, FunctionId %d, Supported %p)\n", ConnectionHandle, FunctionId, Supported);

    if (!con) return SQL_INVALID_HANDLE;

    if (con->hdr.unix_handle)
    {
        ret = get_functions_unix( con, FunctionId, Supported );
    }
    else if (con->hdr.win32_handle)
    {
        ret = get_functions_win32( con, FunctionId, Supported );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &con->hdr );
    return ret;
}

static SQLRETURN get_info_unix_a( struct connection *con, SQLUSMALLINT type, SQLPOINTER value, SQLSMALLINT buflen,
                                  SQLSMALLINT *retlen )
{
    struct SQLGetInfo_params params = { con->hdr.unix_handle, type, value, buflen, retlen };
    return ODBC_CALL( SQLGetInfo, &params );
}

static SQLRETURN get_info_win32_a( struct connection *con, SQLUSMALLINT type, SQLPOINTER value, SQLSMALLINT buflen,
                                   SQLSMALLINT *retlen )
{
    SQLRETURN ret = SQL_ERROR;
    WCHAR *strW = NULL;
    SQLPOINTER buf = value;
    BOOL strvalue = FALSE;

    if (con->hdr.win32_funcs->SQLGetInfo)
        return con->hdr.win32_funcs->SQLGetInfo( con->hdr.win32_handle, type, value, buflen, retlen );

    if (con->hdr.win32_funcs->SQLGetInfoW)
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
            if (buf)
            {
                if (!(strW = malloc( buflen * sizeof(WCHAR) ))) return SQL_ERROR;
                buf = strW;
                buflen *= sizeof(WCHAR);
            }
            strvalue = TRUE;
            break;

        default: break;
        }

        ret = con->hdr.win32_funcs->SQLGetInfoW( con->hdr.win32_handle, type, buf, buflen, retlen );
        if (SUCCESS( ret ))
        {
            if (strW)
            {
                int len = WideCharToMultiByte( CP_ACP, 0, strW, -1, (char *)value, buflen / sizeof(WCHAR), NULL, NULL );
                if (retlen) *retlen = len - 1;
            }
            else if (strvalue && retlen)
            {
                *retlen /= sizeof(WCHAR);
            }
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
    struct connection *con = (struct connection *)lock_object( ConnectionHandle, SQL_HANDLE_DBC );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(ConnectionHandle, %p, InfoType %d, InfoValue %p, BufferLength %d, StringLength %p)\n", ConnectionHandle,
          InfoType, InfoValue, BufferLength, StringLength);

    if (!con) return SQL_INVALID_HANDLE;

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
        ret = SQL_SUCCESS;
        goto done;
    }
    default: break;
    }

    if (con->hdr.unix_handle)
    {
        ret = get_info_unix_a( con, InfoType, InfoValue, BufferLength, StringLength );
    }
    else if (con->hdr.win32_handle)
    {
        ret = get_info_win32_a( con, InfoType, InfoValue, BufferLength, StringLength );
    }

done:
    TRACE("Returning %d\n", ret);
    unlock_object( &con->hdr );
    return ret;
}

static SQLRETURN get_stmt_attr_unix_a( struct statement *stmt, SQLINTEGER attr, SQLPOINTER value, SQLINTEGER buflen,
                                       SQLINTEGER *retlen )
{
    struct SQLGetStmtAttr_params params = { stmt->hdr.unix_handle, attr, value, buflen, retlen };
    return ODBC_CALL( SQLGetStmtAttr, &params );
}

static SQLRETURN get_stmt_attr_win32_a( struct statement *stmt, SQLINTEGER attr, SQLPOINTER value, SQLINTEGER buflen,
                                        SQLINTEGER *retlen )
{
    if (stmt->hdr.win32_funcs->SQLGetStmtAttr)
        return stmt->hdr.win32_funcs->SQLGetStmtAttr( stmt->hdr.win32_handle, attr, value, buflen, retlen );

    if (stmt->hdr.win32_funcs->SQLGetStmtAttrW)
    {
        SQLRETURN ret;
        WCHAR *strW;

        if (buflen == SQL_IS_POINTER || buflen < SQL_LEN_BINARY_ATTR_OFFSET)
            return stmt->hdr.win32_funcs->SQLGetStmtAttrW( stmt->hdr.win32_handle, attr, value, buflen, retlen );

        if (!(strW = malloc( buflen * sizeof(WCHAR) ))) return SQL_ERROR;
        ret = stmt->hdr.win32_funcs->SQLGetStmtAttrW( stmt->hdr.win32_handle, attr, strW, buflen, retlen );
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
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, Attribute %d, Value %p, BufferLength %d, StringLength %p)\n", StatementHandle,
          Attribute, Value, BufferLength, StringLength);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = get_stmt_attr_unix_a( stmt, Attribute, Value, BufferLength, StringLength );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = get_stmt_attr_win32_a( stmt, Attribute, Value, BufferLength, StringLength );
    }

    if (!ret)
    {
        switch (Attribute)
        {
        case SQL_ATTR_APP_ROW_DESC:
        case SQL_ATTR_APP_PARAM_DESC:
        case SQL_ATTR_IMP_ROW_DESC:
        case SQL_ATTR_IMP_PARAM_DESC:
        {
            struct descriptor *desc = stmt->desc[Attribute - SQL_ATTR_APP_ROW_DESC];
            if (stmt->hdr.unix_handle)
            {
                if (sizeof(desc->hdr.unix_handle) > sizeof(SQLHDESC))
                    ERR( "truncating descriptor handle, consider using a Windows driver\n" );
                desc->hdr.unix_handle = (ULONG_PTR)*(SQLHDESC *)Value;
            }
            else if (stmt->hdr.win32_handle)
            {
                desc->hdr.win32_handle = *(SQLHDESC *)Value;
                desc->hdr.win32_funcs = stmt->hdr.win32_funcs;
            }
            *(struct descriptor **)Value = desc;
            break;
        }
        default: break;
        }
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
    return ret;
}

static SQLRETURN get_stmt_option_unix( struct statement *stmt, SQLUSMALLINT option, SQLPOINTER value )
{
    struct SQLGetStmtOption_params params = { stmt->hdr.unix_handle, option, value };
    return ODBC_CALL( SQLGetStmtOption, &params );
}

static SQLRETURN get_stmt_option_win32( struct statement *stmt, SQLUSMALLINT option, SQLPOINTER value )
{
    if (stmt->hdr.win32_funcs->SQLGetStmtOption)
        return stmt->hdr.win32_funcs->SQLGetStmtOption( stmt->hdr.win32_handle, option, value );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLGetStmtOption           [ODBC32.046]
 */
SQLRETURN WINAPI SQLGetStmtOption(SQLHSTMT StatementHandle, SQLUSMALLINT Option, SQLPOINTER Value)
{
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, Option %d, Value %p)\n", StatementHandle, Option, Value);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = get_stmt_option_unix( stmt, Option, Value );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = get_stmt_option_win32( stmt, Option, Value );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
    return ret;
}

static SQLRETURN get_type_info_unix_a( struct statement *stmt, SQLSMALLINT type )
{
    struct SQLGetTypeInfo_params params = { stmt->hdr.unix_handle, type };
    return ODBC_CALL( SQLGetTypeInfo, &params );
}

static SQLRETURN get_type_info_win32_a( struct statement *stmt, SQLSMALLINT type )
{
    if (stmt->hdr.win32_funcs->SQLGetTypeInfo)
        return stmt->hdr.win32_funcs->SQLGetTypeInfo( stmt->hdr.win32_handle, type );
    if (stmt->hdr.win32_funcs->SQLGetTypeInfoW)
        return stmt->hdr.win32_funcs->SQLGetTypeInfoW( stmt->hdr.win32_handle, type );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLGetTypeInfo           [ODBC32.047]
 */
SQLRETURN WINAPI SQLGetTypeInfo(SQLHSTMT StatementHandle, SQLSMALLINT DataType)
{
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, DataType %d)\n", StatementHandle, DataType);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = get_type_info_unix_a( stmt, DataType );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = get_type_info_win32_a( stmt, DataType );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
    return ret;
}

static SQLRETURN get_num_result_cols_unix( struct statement *stmt, SQLSMALLINT *count )
{
    struct SQLNumResultCols_params params = { stmt->hdr.unix_handle, count };
    return ODBC_CALL( SQLNumResultCols, &params );
}

static SQLRETURN get_num_result_cols_win32( struct statement *stmt, SQLSMALLINT *count )
{
    if (stmt->hdr.win32_funcs->SQLNumResultCols)
        return stmt->hdr.win32_funcs->SQLNumResultCols( stmt->hdr.win32_handle, count );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLNumResultCols           [ODBC32.018]
 */
SQLRETURN WINAPI SQLNumResultCols(SQLHSTMT StatementHandle, SQLSMALLINT *ColumnCount)
{
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, ColumnCount %p)\n", StatementHandle, ColumnCount);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = get_num_result_cols_unix( stmt, ColumnCount );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = get_num_result_cols_win32( stmt, ColumnCount );
    }

    TRACE("Returning %d ColumnCount %d\n", ret, *ColumnCount);
    unlock_object( &stmt->hdr );
    return ret;
}

static SQLRETURN get_param_data_unix( struct statement *stmt, SQLPOINTER *value )
{
    struct SQLParamData_params params = { stmt->hdr.unix_handle, value };
    return ODBC_CALL( SQLParamData, &params );
}

static SQLRETURN get_param_data_win32( struct statement *stmt, SQLPOINTER *value )
{
    if (stmt->hdr.win32_funcs->SQLParamData)
        return stmt->hdr.win32_funcs->SQLParamData( stmt->hdr.win32_handle, value );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLParamData           [ODBC32.048]
 */
SQLRETURN WINAPI SQLParamData(SQLHSTMT StatementHandle, SQLPOINTER *Value)
{
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, Value %p)\n", StatementHandle, Value);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = get_param_data_unix( stmt, Value );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = get_param_data_win32( stmt, Value );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
    return ret;
}

static SQLRETURN prepare_unix_a( struct statement *stmt, SQLCHAR *statement, SQLINTEGER len )
{
    struct SQLPrepare_params params = { stmt->hdr.unix_handle, statement, len };
    return ODBC_CALL( SQLPrepare, &params );
}

static SQLRETURN prepare_win32_a( struct statement *stmt, SQLCHAR *statement, SQLINTEGER len )
{
    SQLRETURN ret = SQL_ERROR;

    if (stmt->hdr.win32_funcs->SQLPrepare)
        return stmt->hdr.win32_funcs->SQLPrepare( stmt->hdr.win32_handle, statement, len );

    if (stmt->hdr.win32_funcs->SQLPrepareW)
    {
        WCHAR *strW;
        if (!(strW = strnAtoW( statement, len ))) return SQL_ERROR;
        ret = stmt->hdr.win32_funcs->SQLPrepareW( stmt->hdr.win32_handle, strW, len );
        free( strW );
    }

    return ret;
}

/*************************************************************************
 *				SQLPrepare           [ODBC32.019]
 */
SQLRETURN WINAPI SQLPrepare(SQLHSTMT StatementHandle, SQLCHAR *StatementText, SQLINTEGER TextLength)
{
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, StatementText %s, TextLength %d)\n", StatementHandle,
          debugstr_sqlstr(StatementText, TextLength), TextLength);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = prepare_unix_a( stmt, StatementText, TextLength );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = prepare_win32_a( stmt, StatementText, TextLength );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
    return ret;
}

static SQLRETURN put_data_unix( struct statement *stmt, SQLPOINTER *data, SQLLEN len )
{
    struct SQLPutData_params params = { stmt->hdr.unix_handle, data, len };
    return ODBC_CALL( SQLPutData, &params );
}

static SQLRETURN put_data_win32( struct statement *stmt, SQLPOINTER *data, SQLLEN len )
{
    if (stmt->hdr.win32_funcs->SQLPutData)
        return stmt->hdr.win32_funcs->SQLPutData( stmt->hdr.win32_handle, data, len );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLPutData           [ODBC32.049]
 */
SQLRETURN WINAPI SQLPutData(SQLHSTMT StatementHandle, SQLPOINTER Data, SQLLEN StrLen_or_Ind)
{
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, Data %p, StrLen_or_Ind %s)\n", StatementHandle, Data, debugstr_sqllen(StrLen_or_Ind));

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = put_data_unix( stmt, Data, StrLen_or_Ind );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = put_data_win32( stmt, Data, StrLen_or_Ind );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
    return ret;
}

static SQLRETURN row_count_unix( struct statement *stmt, SQLLEN *count )
{
    INT64 count64;
    struct SQLRowCount_params params = { stmt->hdr.unix_handle, &count64 };
    SQLRETURN ret;

    if (SUCCESS((ret = ODBC_CALL( SQLRowCount, &params ))) && count) *count = count64;
    return ret;
}

static SQLRETURN row_count_win32( struct statement *stmt, SQLLEN *count )
{
    if (stmt->hdr.win32_funcs->SQLRowCount)
        return stmt->hdr.win32_funcs->SQLRowCount( stmt->hdr.win32_handle, count );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLRowCount           [ODBC32.020]
 */
SQLRETURN WINAPI SQLRowCount(SQLHSTMT StatementHandle, SQLLEN *RowCount)
{
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, RowCount %p)\n", StatementHandle, RowCount);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = row_count_unix( stmt, RowCount );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = row_count_win32( stmt, RowCount );
    }

    if (SUCCESS(ret) && RowCount) TRACE(" RowCount %s\n", debugstr_sqllen(*RowCount));
    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
    return ret;
}

static SQLRETURN set_connect_attr_unix_a( struct connection *con, SQLINTEGER attr, SQLPOINTER value, SQLINTEGER len )
{
    struct SQLSetConnectAttr_params params = { con->hdr.unix_handle, attr, value, len };
    return ODBC_CALL( SQLSetConnectAttr, &params );
}

static SQLRETURN set_connect_attr_win32_a( struct connection *con, SQLINTEGER attr, SQLPOINTER value, SQLINTEGER len )
{
    SQLRETURN ret = SQL_ERROR;

    if (con->hdr.win32_funcs->SQLSetConnectAttr)
        return con->hdr.win32_funcs->SQLSetConnectAttr( con->hdr.win32_handle, attr, value, len );

    if (con->hdr.win32_funcs->SQLSetConnectAttrW)
    {
        switch (attr)
        {
        case SQL_ATTR_CURRENT_CATALOG:
        case SQL_ATTR_TRACEFILE:
        case SQL_ATTR_TRANSLATE_LIB:
            FIXME( "string attribute %u not handled\n", attr );
            return SQL_ERROR;
        default: break;
        }

        ret = con->hdr.win32_funcs->SQLSetConnectAttrW( con->hdr.win32_handle, attr, value, len );
    }
    return ret;
}

/*************************************************************************
 *				SQLSetConnectAttr           [ODBC32.039]
 */
SQLRETURN WINAPI SQLSetConnectAttr(SQLHDBC ConnectionHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                   SQLINTEGER StringLength)
{
    struct connection *con = (struct connection *)lock_object( ConnectionHandle, SQL_HANDLE_DBC );
    SQLRETURN ret = SQL_SUCCESS;

    TRACE("(ConnectionHandle %p, Attribute %d, Value %p, StringLength %d)\n", ConnectionHandle, Attribute, Value,
          StringLength);

    if (!con) return SQL_INVALID_HANDLE;

    if (con->hdr.unix_handle)
    {
        ret = set_connect_attr_unix_a( con, Attribute, Value, StringLength );
    }
    else if (con->hdr.win32_handle)
    {
        ret = set_connect_attr_win32_a( con, Attribute, Value, StringLength );
    }
    else
    {
        switch (Attribute)
        {
        case SQL_ATTR_CONNECTION_TIMEOUT:
            con->attr_con_timeout = (UINT32)(ULONG_PTR)Value;
            break;

        case SQL_ATTR_LOGIN_TIMEOUT:
            con->attr_login_timeout = (UINT32)(ULONG_PTR)Value;
            break;

        default:
            FIXME( "unhandled attribute %d\n", Attribute );
            ret = SQL_ERROR;
            break;
        }
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &con->hdr );
    return ret;
}

static SQLRETURN set_connect_option_unix_a( struct connection *con, SQLUSMALLINT attr, SQLULEN value )
{
    struct SQLSetConnectOption_params params = { con->hdr.unix_handle, attr, value };
    return ODBC_CALL( SQLSetConnectOption, &params );
}

static SQLRETURN set_connect_option_win32_a( struct connection *con, SQLUSMALLINT attr, SQLULEN value )
{
    SQLRETURN ret = SQL_ERROR;

    if (con->hdr.win32_funcs->SQLSetConnectOption)
        return con->hdr.win32_funcs->SQLSetConnectOption( con->hdr.win32_handle, attr, value );

    if (con->hdr.win32_funcs->SQLSetConnectOptionW)
    {
        switch (attr)
        {
        case SQL_ATTR_CURRENT_CATALOG:
        case SQL_ATTR_TRACEFILE:
        case SQL_ATTR_TRANSLATE_LIB:
            FIXME( "string option %u not handled\n", attr );
            return SQL_ERROR;
        default: break;
        }

        ret = con->hdr.win32_funcs->SQLSetConnectOptionW( con->hdr.win32_handle, attr, value );
    }
    return ret;
}

/*************************************************************************
 *				SQLSetConnectOption           [ODBC32.050]
 */
SQLRETURN WINAPI SQLSetConnectOption(SQLHDBC ConnectionHandle, SQLUSMALLINT Option, SQLULEN Value)
{
    struct connection *con = (struct connection *)lock_object( ConnectionHandle, SQL_HANDLE_DBC );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(ConnectionHandle %p, Option %d, Value %s)\n", ConnectionHandle, Option, debugstr_sqlulen(Value));

    if (!con) return SQL_INVALID_HANDLE;

    if (con->hdr.unix_handle)
    {
        ret = set_connect_option_unix_a( con, Option, Value );
    }
    else if (con->hdr.win32_handle)
    {
        ret = set_connect_option_win32_a( con, Option, Value );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &con->hdr );
    return ret;
}

static SQLRETURN set_cursor_name_unix_a( struct statement *stmt, SQLCHAR *name, SQLSMALLINT len )
{
    struct SQLSetCursorName_params params = { stmt->hdr.unix_handle, name, len };
    return ODBC_CALL( SQLSetCursorName, &params );
}

static SQLRETURN set_cursor_name_win32_a( struct statement *stmt, SQLCHAR *name, SQLSMALLINT len )
{
    SQLRETURN ret = SQL_ERROR;

    if (stmt->hdr.win32_funcs->SQLSetCursorName)
        return stmt->hdr.win32_funcs->SQLSetCursorName( stmt->hdr.win32_handle, name, len );

    if (stmt->hdr.win32_funcs->SQLSetCursorNameW)
    {
        WCHAR *strW;
        if (!(strW = strnAtoW( name, len ))) return SQL_ERROR;
        ret = stmt->hdr.win32_funcs->SQLSetCursorNameW( stmt->hdr.win32_handle, strW, len );
        free( strW );
    }
    return ret;
}

/*************************************************************************
 *				SQLSetCursorName           [ODBC32.021]
 */
SQLRETURN WINAPI SQLSetCursorName(SQLHSTMT StatementHandle, SQLCHAR *CursorName, SQLSMALLINT NameLength)
{
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, CursorName %s, NameLength %d)\n", StatementHandle,
          debugstr_sqlstr(CursorName, NameLength), NameLength);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = set_cursor_name_unix_a( stmt, CursorName, NameLength );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = set_cursor_name_win32_a( stmt, CursorName, NameLength );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
    return ret;
}

static SQLRETURN set_desc_field_unix_a( struct descriptor *desc, SQLSMALLINT record, SQLSMALLINT id, SQLPOINTER value,
                                        SQLINTEGER len )
{
    struct SQLSetDescField_params params = { desc->hdr.unix_handle, record, id, value, len };
    return ODBC_CALL( SQLSetDescField, &params );
}

static SQLRETURN set_desc_field_win32_a( struct descriptor *desc, SQLSMALLINT record, SQLSMALLINT id, SQLPOINTER value,
                                         SQLINTEGER len )
{
    SQLRETURN ret = SQL_ERROR;

    if (desc->hdr.win32_funcs->SQLSetDescField)
        return desc->hdr.win32_funcs->SQLSetDescField( desc->hdr.win32_handle, record, id, value, len );

    if (desc->hdr.win32_funcs->SQLSetDescFieldW)
    {
        WCHAR *strW = NULL;

        switch (id)
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
            if (!(value = strW = strnAtoW( value, len ))) return SQL_ERROR;
        default: break;
        }

        ret = desc->hdr.win32_funcs->SQLSetDescFieldW( desc->hdr.win32_handle, record, id, value, len );
        free( strW );
    }

    return ret;
}

/*************************************************************************
 *				SQLSetDescField           [ODBC32.073]
 */
SQLRETURN WINAPI SQLSetDescField(SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber, SQLSMALLINT FieldIdentifier,
                                 SQLPOINTER Value, SQLINTEGER BufferLength)
{
    struct descriptor *desc = (struct descriptor *)lock_object( DescriptorHandle, SQL_HANDLE_DESC );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(DescriptorHandle %p, RecNumber %d, FieldIdentifier %d, Value %p, BufferLength %d)\n", DescriptorHandle,
          RecNumber, FieldIdentifier, Value, BufferLength);

    if (!desc) return SQL_INVALID_HANDLE;

    if (desc->hdr.unix_handle)
    {
        ret = set_desc_field_unix_a( desc, RecNumber, FieldIdentifier, Value, BufferLength );
    }
    else if (desc->hdr.win32_handle)
    {
        ret = set_desc_field_win32_a( desc, RecNumber, FieldIdentifier, Value, BufferLength );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &desc->hdr );
    return ret;
}

static SQLRETURN set_desc_rec_unix( struct descriptor *desc, SQLSMALLINT record, SQLSMALLINT type, SQLSMALLINT subtype,
                                    SQLLEN len, SQLSMALLINT precision, SQLSMALLINT scale, SQLPOINTER data,
                                    SQLLEN *retlen, SQLLEN *indicator )
{
    SQLRETURN ret;
    INT64 len64, ind64;
    struct SQLSetDescRec_params params = { desc->hdr.unix_handle, record, type, subtype, len, precision, scale, data,
                                           &len64, &ind64 };
    if (SUCCESS((ret = ODBC_CALL( SQLSetDescRec, &params ))))
    {
        *retlen = len64;
        *indicator = ind64;
    }
    return ret;
}

static SQLRETURN set_desc_rec_win32( struct descriptor *desc, SQLSMALLINT record, SQLSMALLINT type, SQLSMALLINT subtype,
                                     SQLLEN len, SQLSMALLINT precision, SQLSMALLINT scale, SQLPOINTER data,
                                     SQLLEN *retlen, SQLLEN *indicator )
{
    if (desc->hdr.win32_funcs->SQLSetDescRec)
        return desc->hdr.win32_funcs->SQLSetDescRec( desc->hdr.win32_handle, record, type, subtype, len, precision,
                                                     scale, data, retlen, indicator );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLSetDescRec           [ODBC32.074]
 */
SQLRETURN WINAPI SQLSetDescRec(SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber, SQLSMALLINT Type,
                               SQLSMALLINT SubType, SQLLEN Length, SQLSMALLINT Precision, SQLSMALLINT Scale,
                               SQLPOINTER Data, SQLLEN *StringLength, SQLLEN *Indicator)
{
    struct descriptor *desc = (struct descriptor *)lock_object( DescriptorHandle, SQL_HANDLE_DESC );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(DescriptorHandle %p, RecNumber %d, Type %d, SubType %d, Length %s, Precision %d, Scale %d, Data %p,"
          " StringLength %p, Indicator %p)\n", DescriptorHandle, RecNumber, Type, SubType, debugstr_sqllen(Length),
          Precision, Scale, Data, StringLength, Indicator);

    if (!desc) return SQL_INVALID_HANDLE;

    if (desc->hdr.unix_handle)
    {
        ret = set_desc_rec_unix( desc, RecNumber, Type, SubType, Length, Precision, Scale, Data, StringLength,
                                 Indicator );
    }
    else if (desc->hdr.win32_handle)
    {
        ret = set_desc_rec_win32( desc, RecNumber, Type, SubType, Length, Precision, Scale, Data, StringLength,
                                  Indicator );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &desc->hdr );
    return ret;
}

static SQLRETURN set_env_attr_unix( struct environment *env, SQLINTEGER attr, SQLPOINTER value, SQLINTEGER len )
{
    struct SQLSetEnvAttr_params params = { env->hdr.unix_handle, attr, value, len };
    return ODBC_CALL( SQLSetEnvAttr, &params );
}

static SQLRETURN set_env_attr_win32( struct environment *env, SQLINTEGER attr, SQLPOINTER value, SQLINTEGER len )
{
    if (env->hdr.win32_funcs->SQLSetEnvAttr)
        return env->hdr.win32_funcs->SQLSetEnvAttr( env->hdr.win32_handle, attr, value, len );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLSetEnvAttr           [ODBC32.075]
 */
SQLRETURN WINAPI SQLSetEnvAttr(SQLHENV EnvironmentHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                               SQLINTEGER StringLength)
{
    struct environment *env = (struct environment *)lock_object( EnvironmentHandle, SQL_HANDLE_ENV );
    SQLRETURN ret = SQL_SUCCESS;

    TRACE("(EnvironmentHandle %p, Attribute %d, Value %p, StringLength %d)\n", EnvironmentHandle, Attribute, Value,
          StringLength);

    if (!env && Attribute == SQL_ATTR_CONNECTION_POOLING)
    {
        FIXME("Ignoring SQL_ATTR_CONNECTION_POOLING attribute.\n");
        return SQL_SUCCESS;
    }

    if (env->hdr.unix_handle)
    {
        ret = set_env_attr_unix( env, Attribute, Value, StringLength );
    }
    else if (env->hdr.win32_handle)
    {
        ret = set_env_attr_win32( env, Attribute, Value, StringLength );
    }
    else
    {
        switch (Attribute)
        {
        case SQL_ATTR_ODBC_VERSION:
            env->attr_version = (UINT32)(ULONG_PTR)Value;
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
    unlock_object( &env->hdr );
    return ret;
}

static SQLRETURN set_param_unix( struct statement *stmt, SQLUSMALLINT param, SQLSMALLINT value_type,
                                 SQLSMALLINT param_type, SQLULEN precision, SQLSMALLINT scale, SQLPOINTER value,
                                 SQLLEN *retlen )
{
    INT64 len;
    SQLRETURN ret;
    struct SQLSetParam_params params = { stmt->hdr.unix_handle, param, value_type, param_type, precision, scale,
                                         value, &len };
    if (SUCCESS((ret = ODBC_CALL( SQLSetParam, &params )))) *retlen = len;
    return ret;
}

static SQLRETURN set_param_win32( struct statement *stmt, SQLUSMALLINT param, SQLSMALLINT value_type,
                                  SQLSMALLINT param_type, SQLULEN precision, SQLSMALLINT scale, SQLPOINTER value,
                                  SQLLEN *retlen )
{
    if (stmt->hdr.win32_funcs->SQLSetParam)
        return stmt->hdr.win32_funcs->SQLSetParam( stmt->hdr.win32_handle, param, value_type, param_type, precision,
                                                   scale, value, retlen );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLSetParam           [ODBC32.022]
 */
SQLRETURN WINAPI SQLSetParam(SQLHSTMT StatementHandle, SQLUSMALLINT ParameterNumber, SQLSMALLINT ValueType,
                             SQLSMALLINT ParameterType, SQLULEN LengthPrecision, SQLSMALLINT ParameterScale,
                             SQLPOINTER ParameterValue, SQLLEN *StrLen_or_Ind)
{
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, ParameterNumber %d, ValueType %d, ParameterType %d, LengthPrecision %s,"
          " ParameterScale %d, ParameterValue %p, StrLen_or_Ind %p)\n", StatementHandle, ParameterNumber, ValueType,
          ParameterType, debugstr_sqlulen(LengthPrecision), ParameterScale, ParameterValue, StrLen_or_Ind);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = set_param_unix( stmt, ParameterNumber, ValueType, ParameterType, LengthPrecision, ParameterScale,
                              ParameterValue, StrLen_or_Ind );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = set_param_win32( stmt, ParameterNumber, ValueType, ParameterType, LengthPrecision, ParameterScale,
                               ParameterValue, StrLen_or_Ind );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
    return ret;
}

static BOOL resize_result_lengths( struct statement *stmt, UINT size )
{
    UINT i;

    if (!is_wow64 || is_old_wow64) return TRUE;

    TRACE( "resizing result length array\n" );

    for (i = 0; i < stmt->bind_col.count; i++)
    {
        UINT8 *tmp;
        if (!stmt->bind_col.param[i].ptr) continue;
        if (!(tmp = realloc( stmt->bind_col.param[i].len, size * sizeof(UINT64) ))) return FALSE;
        if (tmp != stmt->bind_col.param[i].len)
        {
            struct SQLBindCol_params params;

            params.StatementHandle = stmt->hdr.unix_handle;
            params.ColumnNumber    = i + 1;
            params.TargetType      = stmt->bind_col.param[i].col.target_type;
            params.TargetValue     = stmt->bind_col.param[i].col.target_value;
            params.BufferLength    = stmt->bind_col.param[i].col.buffer_length;
            params.StrLen_or_Ind   = tmp;
            if (!SUCCESS(ODBC_CALL( SQLBindCol, &params )))
            {
                free( tmp );
                return FALSE;
            }
        }
        stmt->bind_col.param[i].len = tmp;
    }

    for (i = 0; i < stmt->bind_parameter.count; i++)
    {
        UINT8 *tmp;
        if (!(tmp = realloc( stmt->bind_parameter.param[i].len, size * sizeof(UINT64) ))) return FALSE;
        if (tmp != stmt->bind_parameter.param[i].len)
        {
            struct SQLBindParameter_params params;

            params.StatementHandle = stmt->hdr.unix_handle;
            params.ParameterNumber = i + 1;
            params.InputOutputType = stmt->bind_parameter.param[i].parameter.input_output_type;
            params.ValueType       = stmt->bind_parameter.param[i].parameter.value_type;
            params.ParameterType   = stmt->bind_parameter.param[i].parameter.parameter_type;
            params.ColumnSize      = stmt->bind_parameter.param[i].parameter.column_size;
            params.DecimalDigits   = stmt->bind_parameter.param[i].parameter.decimal_digits;
            params.ParameterValue  = stmt->bind_parameter.param[i].parameter.parameter_value;
            params.BufferLength    = stmt->bind_parameter.param[i].parameter.buffer_length;
            params.StrLen_or_Ind   = tmp;
            if (!SUCCESS(ODBC_CALL( SQLBindParameter, &params )))
            {
                free( tmp );
                return FALSE;
            }
        }
        stmt->bind_parameter.param[i].len = tmp;
    }
    return TRUE;
}

static SQLRETURN set_stmt_attr_unix_a( struct statement *stmt, SQLINTEGER attr, SQLPOINTER value, SQLINTEGER len )
{
    struct SQLSetStmtAttr_params params = { stmt->hdr.unix_handle, attr, value, len };
    SQLRETURN ret;

    if (SUCCESS((ret = ODBC_CALL( SQLSetStmtAttr, &params ))))
    {
        SQLULEN row_count = (SQLULEN)value;
        if (attr == SQL_ATTR_ROW_ARRAY_SIZE && row_count != stmt->row_count)
        {
            if (!resize_result_lengths( stmt, row_count )) ret = SQL_ERROR;
            else stmt->row_count = row_count;
        }
    }
    return ret;
}

static SQLRETURN set_stmt_attr_win32_a( struct statement *stmt, SQLINTEGER attr, SQLPOINTER value, SQLINTEGER len )
{
    SQLRETURN ret = SQL_ERROR;

    if (stmt->hdr.win32_funcs->SQLSetStmtAttr)
        return stmt->hdr.win32_funcs->SQLSetStmtAttr( stmt->hdr.win32_handle, attr, value, len );

    if (stmt->hdr.win32_funcs->SQLSetStmtAttrW)
    {
        BOOL stringvalue = !(len < SQL_LEN_BINARY_ATTR_OFFSET /* Binary buffer */
                || (len == SQL_IS_POINTER) /* Other pointer */
                || (len == SQL_IS_INTEGER || len == SQL_IS_UINTEGER)); /* Fixed-length */
        WCHAR *strW;

        /* Driver-defined attribute range */
        if (stringvalue && attr >= SQL_DRIVER_STMT_ATTR_BASE && attr <= 0x7fff)
        {
            if (!(strW = strnAtoW( value, len ))) return SQL_ERROR;
            ret = stmt->hdr.win32_funcs->SQLSetStmtAttrW( stmt->hdr.win32_handle, attr, strW, len );
            free( strW );
        }
        else
        {
            ret = stmt->hdr.win32_funcs->SQLSetStmtAttrW( stmt->hdr.win32_handle, attr, value, len );
        }
    }
    return ret;
}

/*************************************************************************
 *				SQLSetStmtAttr           [ODBC32.076]
 */
SQLRETURN WINAPI SQLSetStmtAttr(SQLHSTMT StatementHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                SQLINTEGER StringLength)
{
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, Attribute %d, Value %p, StringLength %d)\n", StatementHandle, Attribute, Value,
          StringLength);

    if (!stmt) return SQL_INVALID_HANDLE;

    switch (Attribute)
    {
    case SQL_ATTR_APP_ROW_DESC:
    case SQL_ATTR_APP_PARAM_DESC:
    {
        struct descriptor *desc = (struct descriptor *)lock_object( Value, SQL_HANDLE_DESC );
        if (desc)
        {
            if (stmt->hdr.unix_handle)
            {
                if (sizeof(desc->hdr.unix_handle) > sizeof(Value))
                    ERR( "truncating descriptor handle, consider using a Windows driver\n" );
                Value = (SQLPOINTER)(ULONG_PTR)desc->hdr.unix_handle;
            }
            else Value = desc->hdr.win32_handle;
            unlock_object( &desc->hdr );
        }
        break;
    }
    default: break;
    }

    if (stmt->hdr.unix_handle)
    {
        ret = set_stmt_attr_unix_a( stmt, Attribute, Value, StringLength );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = set_stmt_attr_win32_a( stmt, Attribute, Value, StringLength );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
    return ret;
}

static SQLRETURN set_stmt_option_unix( struct statement *stmt, SQLUSMALLINT option, SQLULEN value )
{
    struct SQLSetStmtOption_params params = { stmt->hdr.unix_handle, option, value };
    return ODBC_CALL( SQLSetStmtOption, &params );
}

static SQLRETURN set_stmt_option_win32( struct statement *stmt, SQLUSMALLINT option, SQLULEN value )
{
    if (stmt->hdr.win32_funcs->SQLSetStmtOption)
        return stmt->hdr.win32_funcs->SQLSetStmtOption( stmt->hdr.win32_handle, option, value );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLSetStmtOption           [ODBC32.051]
 */
SQLRETURN WINAPI SQLSetStmtOption(SQLHSTMT StatementHandle, SQLUSMALLINT Option, SQLULEN Value)
{
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, Option %d, Value %s)\n", StatementHandle, Option, debugstr_sqlulen(Value));

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = set_stmt_option_unix( stmt, Option, Value );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = set_stmt_option_win32( stmt, Option, Value );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
    return ret;
}

static SQLRETURN special_columns_unix_a( struct statement *stmt, SQLUSMALLINT id, SQLCHAR *catalog, SQLSMALLINT len1,
                                         SQLCHAR *schema, SQLSMALLINT len2, SQLCHAR *table, SQLSMALLINT len3,
                                         SQLUSMALLINT scope, SQLUSMALLINT nullable )
{
    struct SQLSpecialColumns_params params = { stmt->hdr.unix_handle, id, catalog, len1, schema, len2, table, len3,
                                               scope, nullable };
    return ODBC_CALL( SQLSpecialColumns, &params );
}

static SQLRETURN special_columns_win32_a( struct statement *stmt, SQLUSMALLINT id, SQLCHAR *catalog, SQLSMALLINT len1,
                                          SQLCHAR *schema, SQLSMALLINT len2, SQLCHAR *table, SQLSMALLINT len3,
                                          SQLUSMALLINT scope, SQLUSMALLINT nullable )
{
    SQLWCHAR *catalogW = NULL, *schemaW = NULL, *tableW = NULL;
    SQLRETURN ret = SQL_ERROR;

    if (stmt->hdr.win32_funcs->SQLSpecialColumns)
        return stmt->hdr.win32_funcs->SQLSpecialColumns( stmt->hdr.win32_handle, id, catalog, len1, schema, len2,
                                                         table, len3, scope, nullable );

    if (stmt->hdr.win32_funcs->SQLSpecialColumnsW)
    {
        if (!(catalogW = strnAtoW( catalog, len1 ))) return SQL_ERROR;
        if (!(schemaW = strnAtoW( schema, len2 ))) goto done;
        if (!(tableW = strnAtoW( table, len3 ))) goto done;
        ret = stmt->hdr.win32_funcs->SQLSpecialColumnsW( stmt->hdr.win32_handle, id, catalogW, len1, schemaW, len2,
                                                         tableW, len3, scope, nullable );
    }
done:
    free( catalogW );
    free( schemaW );
    free( tableW );
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
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, IdentifierType %d, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d,"
          " TableName %s, NameLength3 %d, Scope %d, Nullable %d)\n", StatementHandle, IdentifierType,
          debugstr_sqlstr(CatalogName, NameLength1), NameLength1, debugstr_sqlstr(SchemaName, NameLength2),
          NameLength2, debugstr_sqlstr(TableName, NameLength3), NameLength3, Scope, Nullable);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = special_columns_unix_a( stmt, IdentifierType, CatalogName, NameLength1, SchemaName, NameLength2,
                                      TableName, NameLength3, Scope, Nullable );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = special_columns_win32_a( stmt, IdentifierType, CatalogName, NameLength1, SchemaName, NameLength2,
                                       TableName, NameLength3, Scope, Nullable );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
    return ret;
}

static SQLRETURN statistics_unix_a( struct statement *stmt, SQLCHAR *catalog, SQLSMALLINT len1, SQLCHAR *schema,
                                    SQLSMALLINT len2, SQLCHAR *table, SQLSMALLINT len3, SQLUSMALLINT unique,
                                    SQLUSMALLINT reserved )
{
    struct SQLStatistics_params params = { stmt->hdr.unix_handle, catalog, len1, schema, len2, table, len3, unique,
                                           reserved };
    return ODBC_CALL( SQLStatistics, &params );
}

static SQLRETURN statistics_win32_a( struct statement *stmt, SQLCHAR *catalog, SQLSMALLINT len1, SQLCHAR *schema,
                                     SQLSMALLINT len2, SQLCHAR *table, SQLSMALLINT len3, SQLUSMALLINT unique,
                                     SQLUSMALLINT reserved )
{
    SQLWCHAR *catalogW = NULL, *schemaW = NULL, *tableW = NULL;
    SQLRETURN ret = SQL_ERROR;

    if (stmt->hdr.win32_funcs->SQLStatistics)
        return stmt->hdr.win32_funcs->SQLStatistics( stmt->hdr.win32_handle, catalog, len1, schema, len2, table, len3,
                                                     unique, reserved );

    if (stmt->hdr.win32_funcs->SQLStatisticsW)
    {
        if (!(catalogW = strnAtoW( catalog, len1 ))) return SQL_ERROR;
        if (!(schemaW = strnAtoW( schema, len2 ))) goto done;
        if (!(tableW = strnAtoW( table, len3 ))) goto done;
        ret = stmt->hdr.win32_funcs->SQLStatisticsW( stmt->hdr.win32_handle, catalogW, len1, schemaW, len2, tableW,
                                                     len3, unique, reserved );
    }
done:
    free( catalogW );
    free( schemaW );
    free( tableW );
    return ret;
}

/*************************************************************************
 *				SQLStatistics           [ODBC32.053]
 */
SQLRETURN WINAPI SQLStatistics(SQLHSTMT StatementHandle, SQLCHAR *CatalogName, SQLSMALLINT NameLength1,
                               SQLCHAR *SchemaName, SQLSMALLINT NameLength2, SQLCHAR *TableName,
                               SQLSMALLINT NameLength3, SQLUSMALLINT Unique, SQLUSMALLINT Reserved)
{
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d SchemaName %s, NameLength2 %d, TableName %s"
          " NameLength3 %d, Unique %d, Reserved %d)\n", StatementHandle,
          debugstr_sqlstr(CatalogName, NameLength1), NameLength1, debugstr_sqlstr(SchemaName, NameLength2),
          NameLength2, debugstr_sqlstr(TableName, NameLength3), NameLength3, Unique, Reserved);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = statistics_unix_a( stmt, CatalogName, NameLength1, SchemaName, NameLength2, TableName, NameLength3,
                                 Unique, Reserved );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = statistics_win32_a( stmt, CatalogName, NameLength1, SchemaName, NameLength2, TableName, NameLength3,
                                  Unique, Reserved );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
    return ret;
}

static SQLRETURN tables_unix_a( struct statement *stmt, SQLCHAR *catalog, SQLSMALLINT len1, SQLCHAR *schema,
                                SQLSMALLINT len2, SQLCHAR *table, SQLSMALLINT len3, SQLCHAR *type,
                                SQLSMALLINT len4 )
{
    struct SQLTables_params params = { stmt->hdr.unix_handle, catalog, len1, schema, len2, table, len3, type, len4 };
    return ODBC_CALL( SQLTables, &params );
}

static SQLRETURN tables_win32_a( struct statement *stmt, SQLCHAR *catalog, SQLSMALLINT len1, SQLCHAR *schema,
                                 SQLSMALLINT len2, SQLCHAR *table, SQLSMALLINT len3, SQLCHAR *type,
                                 SQLSMALLINT len4 )
{
    SQLWCHAR *catalogW = NULL, *schemaW = NULL, *tableW = NULL, *typeW = NULL;
    SQLRETURN ret = SQL_ERROR;

    if (stmt->hdr.win32_funcs->SQLTables)
        return stmt->hdr.win32_funcs->SQLTables( stmt->hdr.win32_handle, catalog, len1, schema, len2, table, len3,
                                                 type, len4 );
    if (stmt->hdr.win32_funcs->SQLTablesW)
    {
        if (!(catalogW = strnAtoW( catalog, len1 ))) return SQL_ERROR;
        if (!(schemaW = strnAtoW( schema, len2 ))) goto done;
        if (!(tableW = strnAtoW( table, len3 ))) goto done;
        if (!(typeW = strnAtoW( type, len4 ))) goto done;
        ret = stmt->hdr.win32_funcs->SQLTablesW( stmt->hdr.win32_handle, catalogW, len1, schemaW, len2, tableW, len3,
                                                 typeW, len4 );
    }
done:
    free( catalogW );
    free( schemaW );
    free( tableW );
    free( typeW );
    return ret;
}

/*************************************************************************
 *				SQLTables           [ODBC32.054]
 */
SQLRETURN WINAPI SQLTables(SQLHSTMT StatementHandle, SQLCHAR *CatalogName, SQLSMALLINT NameLength1,
                           SQLCHAR *SchemaName, SQLSMALLINT NameLength2, SQLCHAR *TableName,
                           SQLSMALLINT NameLength3, SQLCHAR *TableType, SQLSMALLINT NameLength4)
{
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d, TableName %s,"
          " NameLength3 %d, TableType %s, NameLength4 %d)\n", StatementHandle,
          debugstr_sqlstr(CatalogName, NameLength1), NameLength1, debugstr_sqlstr(SchemaName, NameLength2),
          NameLength2, debugstr_sqlstr(TableName, NameLength3), NameLength3, debugstr_sqlstr(TableType, NameLength4),
          NameLength4);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = tables_unix_a( stmt, CatalogName, NameLength1, SchemaName, NameLength2, TableName, NameLength3,
                             TableType, NameLength4 );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = tables_win32_a( stmt, CatalogName, NameLength1, SchemaName, NameLength2, TableName, NameLength3,
                              TableType, NameLength4 );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
    return ret;
}

static SQLRETURN transact_unix( struct environment *env, struct connection *con, SQLUSMALLINT completion )
{
    struct SQLTransact_params params = { env ? env->hdr.unix_handle : 0, con ? con->hdr.unix_handle : 0, completion };
    return ODBC_CALL( SQLTransact, &params );
}

static SQLRETURN transact_win32( struct environment *env, struct connection *con, SQLUSMALLINT completion )
{
    const struct win32_funcs *win32_funcs;

    if (env) win32_funcs = env->hdr.win32_funcs;
    else win32_funcs = con->hdr.win32_funcs;

    if (win32_funcs->SQLTransact)
        return win32_funcs->SQLTransact( env ? env->hdr.win32_handle : NULL, con ? con->hdr.win32_handle : NULL,
                                         completion );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLTransact           [ODBC32.023]
 */
SQLRETURN WINAPI SQLTransact(SQLHENV EnvironmentHandle, SQLHDBC ConnectionHandle, SQLUSMALLINT CompletionType)
{
    struct environment *env = (struct environment *)lock_object( EnvironmentHandle, SQL_HANDLE_ENV );
    struct connection *con = (struct connection *)lock_object( ConnectionHandle, SQL_HANDLE_DBC );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(EnvironmentHandle %p, ConnectionHandle %p, CompletionType %d)\n", EnvironmentHandle, ConnectionHandle,
          CompletionType);

    if (!env && !con) return SQL_INVALID_HANDLE;

    if ((env && env->hdr.unix_handle) || (con && con->hdr.unix_handle))
    {
        ret = transact_unix( env, con, CompletionType );
    }
    else if ((env && env->hdr.win32_handle) || (con && con->hdr.win32_handle))
    {
        ret = transact_win32( env, con, CompletionType );
    }

    TRACE("Returning %d\n", ret);
    if (env) unlock_object( &env->hdr );
    if (con) unlock_object( &con->hdr );
    return ret;
}

struct attribute
{
    WCHAR *name;
    WCHAR *value;
};

struct attribute_list
{
    UINT32 count;
    struct attribute **attrs;
};

static void free_attribute( struct attribute *attr )
{
    free( attr->name );
    free( attr->value );
    free( attr );
}

static void free_attribute_list( struct attribute_list *list )
{
    UINT32 i;
    for (i = 0; i < list->count; i++) free_attribute( list->attrs[i] );
    free( list->attrs );
    list->count = 0;
    list->attrs = NULL;
}

static BOOL append_attribute( struct attribute_list *list, struct attribute *attr )
{
    struct attribute **tmp;
    UINT32 new_count = list->count + 1;

    if (!(tmp = realloc( list->attrs, new_count * sizeof(*list->attrs) )))
        return FALSE;

    tmp[list->count] = attr;
    list->attrs = tmp;
    list->count = new_count;
    return TRUE;
}

static SQLRETURN parse_connect_string( struct attribute_list *list, const WCHAR *str )
{
    const WCHAR *p = str, *q;
    struct attribute *attr;
    int len;

    list->count = 0;
    list->attrs = NULL;
    for (;;)
    {
        if (!*p) break;
        if (!(q = wcschr( p, '=' )) || q == p) return SQL_SUCCESS;
        len = q - p;

        if (!(attr = calloc( 1, sizeof(*attr) ))) return SQL_ERROR;
        if (!(attr->name = malloc( (len + 1) * sizeof(WCHAR) )))
        {
            free_attribute( attr );
            free_attribute_list( list );
            return SQL_ERROR;
        }
        memcpy( attr->name, p, len * sizeof(WCHAR) );
        attr->name[len] = 0;

        q++; /* skip = */
        if (*q == '{')
        {
            if (!(p = wcschr( ++q, '}' )))
            {
                free_attribute( attr );
                free_attribute_list( list );
                return SQL_ERROR;
            }
        }
        else
            p = wcschr( q, ';' );

        if (p)
        {
            len = p - q;
            if (*p == '}') p++;
            p++;
        }
        else
        {
            len = wcslen( q );
            p = q + len;
        }

        if (!(attr->value = malloc( (len + 1) * sizeof(WCHAR) )))
        {
            free_attribute( attr );
            free_attribute_list( list );
            return SQL_ERROR;
        }
        memcpy( attr->value, q, len * sizeof(WCHAR) );
        attr->value[len] = 0;

        if (!append_attribute( list, attr ))
        {
            free_attribute( attr );
            free_attribute_list( list );
            return SQL_ERROR;
        }
    }

    return SQL_SUCCESS;
}

static const WCHAR *get_attribute( const struct attribute_list *list, const WCHAR *name )
{
    UINT32 i;
    for (i = 0; i < list->count; i++) if (!wcsicmp( list->attrs[i]->name, name )) return list->attrs[i]->value;
    return NULL;
}

static WCHAR *build_connect_string( struct attribute_list *list )
{
    WCHAR *ret, *ptr;
    UINT32 i, len = 1;

    for (i = 0; i < list->count; i++) len += wcslen( list->attrs[i]->name ) + wcslen( list->attrs[i]->value ) + 2;
    if (!(ptr = ret = malloc( len * sizeof(WCHAR) ))) return NULL;
    for (i = 0; i < list->count; i++)
    {
        wcscpy( ptr, list->attrs[i]->name );
        ptr += wcslen( ptr );
        *ptr++ = '=';
        wcscpy( ptr, list->attrs[i]->value );
        ptr += wcslen( ptr );
        if (i < list->count - 1) *ptr++ = ';';
    }
    *ptr = 0;
    return ret;
}

static SQLRETURN browse_connect_win32_a( struct connection *con, SQLCHAR *in_conn_str, SQLSMALLINT inlen,
                                         SQLCHAR *out_conn_str, SQLSMALLINT buflen, SQLSMALLINT *outlen )
{
    SQLRETURN ret = SQL_ERROR;
    SQLWCHAR *in = NULL, *out = NULL;
    SQLSMALLINT lenW;

    if (con->hdr.win32_funcs->SQLBrowseConnect)
        return con->hdr.win32_funcs->SQLBrowseConnect( con->hdr.win32_handle, in_conn_str, inlen, out_conn_str,
                                                       buflen, outlen );
    if (con->hdr.win32_funcs->SQLBrowseConnectW)
    {
        if (!(in = strnAtoW( in_conn_str, inlen ))) return SQL_ERROR;
        if (!(out = malloc( buflen * sizeof(WCHAR) ))) goto done;
        ret = con->hdr.win32_funcs->SQLBrowseConnectW( con->hdr.win32_handle, in, inlen, out, buflen, &lenW );
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

static SQLRETURN browse_connect_unix_a( struct connection *con, SQLCHAR *in_conn_str, SQLSMALLINT len,
                                        SQLCHAR *out_conn_str, SQLSMALLINT buflen, SQLSMALLINT *len2 )
{
    struct SQLBrowseConnect_params params = { con->hdr.unix_handle, in_conn_str, len, out_conn_str, buflen, len2 };
    return ODBC_CALL( SQLBrowseConnect, &params );
}

static char *strdupWA( const WCHAR *src )
{
    int len;
    char *dst;
    if (!src) return NULL;
    len = WideCharToMultiByte( CP_ACP, 0, src, -1, NULL, 0, NULL, NULL );
    if ((dst = malloc( len ))) WideCharToMultiByte( CP_ACP, 0, src, -1, dst, len, NULL, NULL );
    return dst;
}

/*************************************************************************
 *				SQLBrowseConnect           [ODBC32.055]
 */
SQLRETURN WINAPI SQLBrowseConnect(SQLHDBC ConnectionHandle, SQLCHAR *InConnectionString, SQLSMALLINT StringLength1,
                                  SQLCHAR *OutConnectionString, SQLSMALLINT BufferLength, SQLSMALLINT *StringLength2)
{
    struct connection *con = (struct connection *)lock_object( ConnectionHandle, SQL_HANDLE_DBC );
    const WCHAR *datasource, *drivername = NULL;
    WCHAR *filename = NULL, *connect_string = NULL, *strW = strdupAW( (const char *)InConnectionString );
    SQLCHAR *strA = NULL;
    struct attribute_list attrs;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(ConnectionHandle %p, InConnectionString %s, StringLength1 %d, OutConnectionString %p, BufferLength, %d, "
          "StringLength2 %p)\n", ConnectionHandle, debugstr_sqlstr(InConnectionString, StringLength1),
          StringLength1, OutConnectionString, BufferLength, StringLength2);

    if (!con) return SQL_INVALID_HANDLE;

    if (parse_connect_string( &attrs, strW ) || !(connect_string = build_connect_string( &attrs )) ||
        !(strA = (SQLCHAR *)strdupWA( connect_string ))) goto done;

    if (!(datasource = get_attribute( &attrs, L"DSN" )) && !(drivername = get_attribute( &attrs, L"DRIVER" )))
    {
        WARN( "can't find data source or driver name\n" );
        goto done;
    }
    if ((datasource && !(filename = get_driver_filename_from_source( datasource ))) ||
        (drivername && !(filename = get_driver_filename_from_name( drivername ))))
    {
        WARN( "can't find driver filename\n" );
        goto done;
    }

    if (has_suffix( filename, L".dll" ))
    {
        if (!(con->hdr.win32_funcs = con->hdr.parent->win32_funcs = load_driver( filename )))
        {
            WARN( "failed to load driver %s\n", debugstr_w(filename) );
            goto done;
        }
        TRACE( "using Windows driver %s\n", debugstr_w(filename) );

        if (!SUCCESS((ret = create_env( (struct environment *)con->hdr.parent, FALSE )))) goto done;
        if (!SUCCESS((ret = create_con( con )))) goto done;

        ret = browse_connect_win32_a( con, strA, StringLength1, OutConnectionString, BufferLength, StringLength2 );
    }
    else
    {
        TRACE( "using Unix driver %s\n", debugstr_w(filename) );

        if (!SUCCESS((ret = create_env( (struct environment *)con->hdr.parent, TRUE )))) goto done;
        if (!SUCCESS((ret = create_con( con )))) goto done;

        ret = browse_connect_unix_a( con, strA, StringLength1, OutConnectionString, BufferLength, StringLength2 );
    }

done:
    free( strA );
    free( strW );
    free( connect_string );
    free_attribute_list( &attrs );
    free( filename );

    TRACE("Returning %d\n", ret);
    unlock_object( &con->hdr );
    return ret;
}

static SQLRETURN bulk_operations_unix( struct statement *stmt, SQLSMALLINT operation )
{
    struct SQLBulkOperations_params params = { stmt->hdr.unix_handle, operation };
    SQLRETURN ret;

    if (SUCCESS((ret = ODBC_CALL( SQLBulkOperations, &params )))) update_result_lengths( stmt, SQL_PARAM_OUTPUT );
    return ret;
}

static SQLRETURN bulk_operations_win32( struct statement *stmt, SQLSMALLINT operation )
{
    if (stmt->hdr.win32_funcs->SQLBulkOperations)
        return stmt->hdr.win32_funcs->SQLBulkOperations( stmt->hdr.win32_handle, operation );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLBulkOperations           [ODBC32.078]
 */
SQLRETURN WINAPI SQLBulkOperations(SQLHSTMT StatementHandle, SQLSMALLINT Operation)
{
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, Operation %d)\n", StatementHandle, Operation);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = bulk_operations_unix( stmt, Operation );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = bulk_operations_win32( stmt, Operation );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
    return ret;
}

static SQLRETURN col_attributes_unix_a( struct statement *stmt, SQLUSMALLINT col, SQLUSMALLINT field_id,
                                        SQLPOINTER char_attrs, SQLSMALLINT buflen, SQLSMALLINT *retlen,
                                        SQLLEN *num_attrs )
{
    SQLRETURN ret;
    INT64 attrs;
    struct SQLColAttributes_params params = { stmt->hdr.unix_handle, col, field_id, char_attrs, buflen, retlen, &attrs };
    if (SUCCESS((ret = ODBC_CALL( SQLColAttributes, &params )))) *num_attrs = attrs;
    return ret;
}

static SQLRETURN col_attributes_win32_a( struct statement *stmt, SQLUSMALLINT col, SQLUSMALLINT field_id,
                                         SQLPOINTER char_attrs, SQLSMALLINT buflen, SQLSMALLINT *retlen,
                                         SQLLEN *num_attrs )
{
    SQLRETURN ret = SQL_ERROR;

    if (stmt->hdr.win32_funcs->SQLColAttributes)
        return stmt->hdr.win32_funcs->SQLColAttributes( stmt->hdr.win32_handle, col, field_id, char_attrs, buflen,
                                                        retlen, num_attrs );

    if (stmt->hdr.win32_funcs->SQLColAttributesW)
    {
        if (buflen < 0)
            ret = stmt->hdr.win32_funcs->SQLColAttributesW( stmt->hdr.win32_handle, col, field_id, char_attrs, buflen,
                                                            retlen, num_attrs );
        else
        {
            SQLWCHAR *strW;
            SQLSMALLINT lenW;

            if (!(strW = malloc( buflen * sizeof(WCHAR) ))) return SQL_ERROR;
            ret = stmt->hdr.win32_funcs->SQLColAttributesW( stmt->hdr.win32_handle, col, field_id, strW, buflen, &lenW,
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
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, ColumnNumber %d, FieldIdentifier %d, CharacterAttributes %p, BufferLength %d, "
          "StringLength %p, NumericAttributes %p)\n", StatementHandle, ColumnNumber, FieldIdentifier,
          CharacterAttributes, BufferLength, StringLength, NumericAttributes);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = col_attributes_unix_a( stmt, ColumnNumber, FieldIdentifier, CharacterAttributes, BufferLength,
                                     StringLength, NumericAttributes );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = col_attributes_win32_a( stmt, ColumnNumber, FieldIdentifier, CharacterAttributes, BufferLength,
                                      StringLength, NumericAttributes );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
    return ret;
}

static SQLRETURN column_privs_unix_a( struct statement *stmt, SQLCHAR *catalog, SQLSMALLINT len1, SQLCHAR *schema,
                                      SQLSMALLINT len2, SQLCHAR *table, SQLSMALLINT len3, SQLCHAR *column,
                                      SQLSMALLINT len4 )
{
    struct SQLColumnPrivileges_params params = { stmt->hdr.unix_handle, catalog, len1, schema, len2, table, len3,
                                                 column, len4 };
    return ODBC_CALL( SQLColumnPrivileges, &params );
}

static SQLRETURN column_privs_win32_a( struct statement *stmt, SQLCHAR *catalog, SQLSMALLINT len1, SQLCHAR *schema,
                                      SQLSMALLINT len2, SQLCHAR *table, SQLSMALLINT len3, SQLCHAR *column,
                                      SQLSMALLINT len4 )
{
    SQLWCHAR *catalogW = NULL, *schemaW = NULL, *tableW = NULL, *columnW = NULL;
    SQLRETURN ret = SQL_ERROR;

    if (stmt->hdr.win32_funcs->SQLColumnPrivileges)
        return stmt->hdr.win32_funcs->SQLColumnPrivileges( stmt->hdr.win32_handle, catalog, len1, schema, len2, table,
                                                           len3, column, len4 );

    if (stmt->hdr.win32_funcs->SQLColumnPrivilegesW)
    {
        if (!(catalogW = strnAtoW( catalog, len1 ))) return SQL_ERROR;
        if (!(schemaW = strnAtoW( schema, len2 ))) goto done;
        if (!(tableW = strnAtoW( table, len3 ))) goto done;
        if (!(columnW = strnAtoW( column, len4 ))) goto done;
        ret = stmt->hdr.win32_funcs->SQLColumnPrivilegesW( stmt->hdr.win32_handle, catalogW, len1, schemaW, len2,
                                                           tableW, len3, columnW, len4 );
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
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d, TableName %s,"
          " NameLength3 %d, ColumnName %s, NameLength4 %d)\n", StatementHandle,
          debugstr_sqlstr(CatalogName, NameLength1), NameLength1, debugstr_sqlstr(SchemaName, NameLength2),
          NameLength2, debugstr_sqlstr(TableName, NameLength3), NameLength3, debugstr_sqlstr(ColumnName, NameLength4),
          NameLength4);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = column_privs_unix_a( stmt, CatalogName, NameLength1, SchemaName, NameLength2, TableName, NameLength3,
                                   ColumnName, NameLength4 );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = column_privs_win32_a( stmt, CatalogName, NameLength1, SchemaName, NameLength2, TableName, NameLength3,
                                    ColumnName, NameLength4 );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
    return ret;
}

static SQLRETURN describe_param_unix( struct statement *stmt, SQLUSMALLINT param, SQLSMALLINT *type, SQLULEN *size,
                                      SQLSMALLINT *digits, SQLSMALLINT *nullable )
{
    UINT64 size64;
    struct SQLDescribeParam_params params = { stmt->hdr.unix_handle, param, type, &size64, digits, nullable };
    SQLRETURN ret;

    if (SUCCESS((ret = ODBC_CALL( SQLDescribeParam, &params )))) *size = size64;
    return ret;
}

static SQLRETURN describe_param_win32( struct statement *stmt, SQLUSMALLINT param, SQLSMALLINT *type, SQLULEN *size,
                                       SQLSMALLINT *digits, SQLSMALLINT *nullable )
{
    if (stmt->hdr.win32_funcs->SQLDescribeParam)
        return stmt->hdr.win32_funcs->SQLDescribeParam( stmt->hdr.win32_handle, param, type, size, digits, nullable );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLDescribeParam          [ODBC32.058]
 */
SQLRETURN WINAPI SQLDescribeParam(SQLHSTMT StatementHandle, SQLUSMALLINT ParameterNumber, SQLSMALLINT *DataType,
                                  SQLULEN *ParameterSize, SQLSMALLINT *DecimalDigits, SQLSMALLINT *Nullable)
{
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, ParameterNumber %d, DataType %p, ParameterSize %p, DecimalDigits %p, Nullable %p)\n",
          StatementHandle, ParameterNumber, DataType, ParameterSize, DecimalDigits, Nullable);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = describe_param_unix( stmt, ParameterNumber, DataType, ParameterSize, DecimalDigits, Nullable );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = describe_param_win32( stmt, ParameterNumber, DataType, ParameterSize, DecimalDigits, Nullable );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
    return ret;
}

static SQLRETURN extended_fetch_unix( struct statement *stmt, SQLUSMALLINT orientation, SQLLEN offset,
                                      SQLULEN *count, SQLUSMALLINT *status )
{
    UINT64 count64;
    struct SQLExtendedFetch_params params = { stmt->hdr.unix_handle, orientation, offset, &count64, status };
    SQLRETURN ret;

    if (SUCCESS((ret = ODBC_CALL( SQLExtendedFetch, &params )))) *count = count64;
    return ret;
}

static SQLRETURN extended_fetch_win32( struct statement *stmt, SQLUSMALLINT orientation, SQLLEN offset,
                                       SQLULEN *count, SQLUSMALLINT *status )
{
    if (stmt->hdr.win32_funcs->SQLExtendedFetch)
        return stmt->hdr.win32_funcs->SQLExtendedFetch( stmt->hdr.win32_handle, orientation, offset, count, status );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLExtendedFetch           [ODBC32.059]
 */
SQLRETURN WINAPI SQLExtendedFetch(SQLHSTMT StatementHandle, SQLUSMALLINT FetchOrientation, SQLLEN FetchOffset,
                                  SQLULEN *RowCount, SQLUSMALLINT *RowStatusArray)
{
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, FetchOrientation %d, FetchOffset %s, RowCount %p, RowStatusArray %p)\n",
          StatementHandle, FetchOrientation, debugstr_sqllen(FetchOffset), RowCount, RowStatusArray);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = extended_fetch_unix( stmt, FetchOrientation, FetchOffset, RowCount, RowStatusArray );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = extended_fetch_win32( stmt, FetchOrientation, FetchOffset, RowCount, RowStatusArray );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
    return ret;
}

static SQLRETURN foreign_keys_unix_a( struct statement *stmt, SQLCHAR *pk_catalog, SQLSMALLINT len1,
                                      SQLCHAR *pk_schema, SQLSMALLINT len2, SQLCHAR *pk_table, SQLSMALLINT len3,
                                      SQLCHAR *fk_catalog, SQLSMALLINT len4, SQLCHAR *fk_schema, SQLSMALLINT len5,
                                      SQLCHAR *fk_table, SQLSMALLINT len6 )
{
    struct SQLForeignKeys_params params = { stmt->hdr.unix_handle, pk_catalog, len1, pk_schema, len2, pk_table, len3,
                                            fk_catalog, len4, fk_schema, len5, fk_table, len6 };
    return ODBC_CALL( SQLForeignKeys, &params );
}

static SQLRETURN foreign_keys_win32_a( struct statement *stmt, SQLCHAR *pk_catalog, SQLSMALLINT len1,
                                       SQLCHAR *pk_schema, SQLSMALLINT len2, SQLCHAR *pk_table, SQLSMALLINT len3,
                                       SQLCHAR *fk_catalog, SQLSMALLINT len4, SQLCHAR *fk_schema, SQLSMALLINT len5,
                                       SQLCHAR *fk_table, SQLSMALLINT len6 )
{
    SQLWCHAR *pk_catalogW = NULL, *pk_schemaW = NULL, *pk_tableW = NULL;
    SQLWCHAR *fk_catalogW = NULL, *fk_schemaW = NULL, *fk_tableW = NULL;
    SQLRETURN ret = SQL_ERROR;

    if (stmt->hdr.win32_funcs->SQLForeignKeys)
    {
        return stmt->hdr.win32_funcs->SQLForeignKeys( stmt->hdr.win32_handle, pk_catalog, len1, pk_schema, len2,
                                                      pk_table, len3, fk_catalog, len4, fk_schema, len5, fk_table,
                                                      len6 );
    }
    if (stmt->hdr.win32_funcs->SQLForeignKeysW)
    {
        if (!(pk_catalogW = strnAtoW( pk_catalog, len1 ))) return SQL_ERROR;
        if (!(pk_schemaW = strnAtoW( pk_schema, len2 ))) goto done;
        if (!(pk_tableW = strnAtoW( pk_table, len3 ))) goto done;
        if (!(fk_catalogW = strnAtoW( fk_catalog, len4 ))) goto done;
        if (!(fk_schemaW = strnAtoW( fk_schema, len5 ))) goto done;
        if (!(fk_tableW = strnAtoW( fk_table, len6 ))) goto done;
        ret = stmt->hdr.win32_funcs->SQLForeignKeysW( stmt->hdr.win32_handle, pk_catalogW, len1, pk_schemaW, len2,
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
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, PkCatalogName %s, NameLength1 %d, PkSchemaName %s, NameLength2 %d,"
          " PkTableName %s, NameLength3 %d, FkCatalogName %s, NameLength4 %d, FkSchemaName %s,"
          " NameLength5 %d, FkTableName %s, NameLength6 %d)\n", StatementHandle,
          debugstr_sqlstr(PkCatalogName, NameLength1), NameLength1, debugstr_sqlstr(PkSchemaName, NameLength2),
          NameLength2, debugstr_sqlstr(PkTableName, NameLength3), NameLength3,
          debugstr_sqlstr(FkCatalogName, NameLength4), NameLength4, debugstr_sqlstr(FkSchemaName, NameLength5),
          NameLength5, debugstr_sqlstr(FkTableName, NameLength6), NameLength6);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = foreign_keys_unix_a( stmt, PkCatalogName, NameLength1, PkSchemaName, NameLength2, PkTableName,
                                   NameLength3, FkCatalogName, NameLength4, FkSchemaName, NameLength5, FkTableName,
                                   NameLength6 );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = foreign_keys_win32_a( stmt, PkCatalogName, NameLength1, PkSchemaName, NameLength2, PkTableName,
                                    NameLength3, FkCatalogName, NameLength4, FkSchemaName, NameLength5, FkTableName,
                                    NameLength6 );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
    return ret;
}

static SQLRETURN more_results_unix( struct statement *stmt )
{
    struct SQLMoreResults_params params = { stmt->hdr.unix_handle };
    return ODBC_CALL( SQLMoreResults, &params );
}

static SQLRETURN more_results_win32( struct statement *stmt )
{
    if (stmt->hdr.win32_funcs->SQLMoreResults)
        return stmt->hdr.win32_funcs->SQLMoreResults( stmt->hdr.win32_handle );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLMoreResults           [ODBC32.061]
 */
SQLRETURN WINAPI SQLMoreResults(SQLHSTMT StatementHandle)
{
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(%p)\n", StatementHandle);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = more_results_unix( stmt );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = more_results_win32( stmt );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
    return ret;
}

static SQLRETURN native_sql_unix_a( struct connection *con, SQLCHAR *in_statement, SQLINTEGER len,
                                    SQLCHAR *out_statement, SQLINTEGER buflen, SQLINTEGER *retlen )
{
    struct SQLNativeSql_params params = { con->hdr.unix_handle, in_statement, len, out_statement, buflen, retlen };
    return ODBC_CALL( SQLNativeSql, &params );
}

static SQLRETURN native_sql_win32_a( struct connection *con, SQLCHAR *in_statement, SQLINTEGER in_len,
                                     SQLCHAR *out_statement, SQLINTEGER buflen, SQLINTEGER *retlen )
{
    SQLRETURN ret = SQL_ERROR;

    if (con->hdr.win32_funcs->SQLNativeSql)
        return con->hdr.win32_funcs->SQLNativeSql( con->hdr.win32_handle, in_statement, in_len, out_statement,
                                                   buflen, retlen );

    if (con->hdr.win32_funcs->SQLNativeSqlW)
    {
        SQLWCHAR *inW, *outW;

        if (!(inW = malloc( in_len * sizeof(WCHAR) ))) return SQL_ERROR;
        if (!(outW = malloc( buflen * sizeof(WCHAR) )))
        {
            free( inW );
            return SQL_ERROR;
        }
        ret = con->hdr.win32_funcs->SQLNativeSqlW( con->hdr.win32_handle, inW, in_len, outW, buflen, retlen );
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
    struct connection *con = (struct connection *)lock_object( ConnectionHandle, SQL_HANDLE_DBC );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(ConnectionHandle %p, InStatementText %s, TextLength1 %d, OutStatementText %p, BufferLength, %d, "
          "TextLength2 %p)\n", ConnectionHandle, debugstr_sqlstr(InStatementText, TextLength1), TextLength1,
          OutStatementText, BufferLength, TextLength2);

    if (!con) return SQL_INVALID_HANDLE;

    if (con->hdr.unix_handle)
    {
        ret = native_sql_unix_a( con, InStatementText, TextLength1, OutStatementText, BufferLength, TextLength2 );
    }
    else if (con->hdr.win32_handle)
    {
        ret = native_sql_win32_a( con, InStatementText, TextLength1, OutStatementText, BufferLength, TextLength2 );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &con->hdr );
    return ret;
}

static SQLRETURN num_params_unix( struct statement *stmt, SQLSMALLINT *count )
{
    struct SQLNumParams_params params = { stmt->hdr.unix_handle, count };
    return ODBC_CALL( SQLNumParams, &params );
}

static SQLRETURN num_params_win32( struct statement *stmt, SQLSMALLINT *count )
{
    if (stmt->hdr.win32_funcs->SQLNumParams)
        return stmt->hdr.win32_funcs->SQLNumParams( stmt->hdr.win32_handle, count );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLNumParams           [ODBC32.063]
 */
SQLRETURN WINAPI SQLNumParams(SQLHSTMT StatementHandle, SQLSMALLINT *ParameterCount)
{
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, pcpar %p)\n", StatementHandle, ParameterCount);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = num_params_unix( stmt, ParameterCount );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = num_params_win32( stmt, ParameterCount );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
    return ret;
}

static SQLRETURN param_options_unix( struct statement *stmt, SQLULEN row_count, SQLULEN *row_number )
{
    UINT64 row;
    struct SQLParamOptions_params params = { stmt->hdr.unix_handle, row_count, &row };
    SQLRETURN ret;

    if (SUCCESS((ret = ODBC_CALL( SQLParamOptions, &params )))) *row_number = row;
    return ret;
}

static SQLRETURN param_options_win32( struct statement *stmt, SQLULEN row_count, SQLULEN *row_number )
{
    if (stmt->hdr.win32_funcs->SQLParamOptions)
        return stmt->hdr.win32_funcs->SQLParamOptions( stmt->hdr.win32_handle, row_count, row_number );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLParamOptions           [ODBC32.064]
 */
SQLRETURN WINAPI SQLParamOptions(SQLHSTMT StatementHandle, SQLULEN RowCount, SQLULEN *RowNumber)
{
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, RowCount %s, RowNumber %p)\n", StatementHandle, debugstr_sqlulen(RowCount),
          RowNumber);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = param_options_unix( stmt, RowCount, RowNumber );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = param_options_win32( stmt, RowCount, RowNumber );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
    return ret;
}

static SQLRETURN primary_keys_unix_a( struct statement *stmt, SQLCHAR *catalog, SQLSMALLINT len1, SQLCHAR *schema,
                                      SQLSMALLINT len2, SQLCHAR *table, SQLSMALLINT len3 )
{
    struct SQLPrimaryKeys_params params = { stmt->hdr.unix_handle, catalog, len1, schema, len2, table, len3 };
    return ODBC_CALL( SQLPrimaryKeys, &params );
}

static SQLRETURN primary_keys_win32_a( struct statement *stmt, SQLCHAR *catalog, SQLSMALLINT len1, SQLCHAR *schema,
                                       SQLSMALLINT len2, SQLCHAR *table, SQLSMALLINT len3 )
{
    WCHAR *catalogW = NULL, *schemaW = NULL, *tableW = NULL;
    SQLRETURN ret = SQL_ERROR;

    if (stmt->hdr.win32_funcs->SQLPrimaryKeys)
        return stmt->hdr.win32_funcs->SQLPrimaryKeys( stmt->hdr.win32_handle, catalog, len1, schema, len2, table,
                                                      len3 );

    if (stmt->hdr.win32_funcs->SQLPrimaryKeysW)
    {
        if (!(catalogW = strnAtoW( catalog, len1 ))) goto done;
        if (!(schemaW = strnAtoW( schema, len2 ))) goto done;
        if (!(tableW = strnAtoW( table, len3 ))) goto done;
        ret = stmt->hdr.win32_funcs->SQLPrimaryKeysW( stmt->hdr.win32_handle, catalogW, len1, schemaW, len2, tableW,
                                                      len3 );
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
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d, TableName %s,"
          " NameLength3 %d)\n", StatementHandle,
          debugstr_sqlstr(CatalogName, NameLength1), NameLength1, debugstr_sqlstr(SchemaName, NameLength2),
          NameLength2, debugstr_sqlstr(TableName, NameLength3), NameLength3);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = primary_keys_unix_a( stmt, CatalogName, NameLength1, SchemaName, NameLength2, TableName, NameLength3 );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = primary_keys_win32_a( stmt, CatalogName, NameLength1, SchemaName, NameLength2, TableName, NameLength3 );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
    return ret;
}

static SQLRETURN procedure_columns_unix_a( struct statement *stmt, SQLCHAR *catalog, SQLSMALLINT len1,
                                           SQLCHAR *schema, SQLSMALLINT len2, SQLCHAR *proc, SQLSMALLINT len3,
                                           SQLCHAR *column, SQLSMALLINT len4 )
{
    struct SQLProcedureColumns_params params = { stmt->hdr.unix_handle, catalog, len1, schema, len2, proc, len3,
                                                 column, len4 };
    return ODBC_CALL( SQLProcedureColumns, &params );
}

static SQLRETURN procedure_columns_win32_a( struct statement *stmt, SQLCHAR *catalog, SQLSMALLINT len1,
                                            SQLCHAR *schema, SQLSMALLINT len2, SQLCHAR *proc, SQLSMALLINT len3,
                                            SQLCHAR *column, SQLSMALLINT len4 )
{
    WCHAR *catalogW = NULL, *schemaW = NULL, *procW = NULL, *columnW = NULL;
    SQLRETURN ret = SQL_ERROR;

    if (stmt->hdr.win32_funcs->SQLProcedureColumns)
        return stmt->hdr.win32_funcs->SQLProcedureColumns( stmt->hdr.win32_handle, catalog, len1, schema, len2, proc,
                                                           len3, column, len4 );

    if (stmt->hdr.win32_funcs->SQLProcedureColumnsW)
    {
        if (!(catalogW = strnAtoW( catalog, len1 ))) goto done;
        if (!(schemaW = strnAtoW( schema, len2 ))) goto done;
        if (!(procW = strnAtoW( proc, len3 ))) goto done;
        if (!(columnW = strnAtoW( column, len4 ))) goto done;
        ret = stmt->hdr.win32_funcs->SQLProcedureColumnsW( stmt->hdr.win32_handle, catalogW, len1, schemaW, len2,
                                                           procW, len3, columnW, len4 );
    }

done:
    free( catalogW );
    free( schemaW );
    free( procW );
    free( columnW );
    return ret;
}

/*************************************************************************
 *				SQLProcedureColumns           [ODBC32.066]
 */
SQLRETURN WINAPI SQLProcedureColumns(SQLHSTMT StatementHandle, SQLCHAR *CatalogName, SQLSMALLINT NameLength1,
                                     SQLCHAR *SchemaName, SQLSMALLINT NameLength2, SQLCHAR *ProcName,
                                     SQLSMALLINT NameLength3, SQLCHAR *ColumnName, SQLSMALLINT NameLength4)
{
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d, ProcName %s,"
          " NameLength3 %d, ColumnName %s, NameLength4 %d)\n", StatementHandle,
          debugstr_sqlstr(CatalogName, NameLength1), NameLength1, debugstr_sqlstr(SchemaName, NameLength2),
          NameLength2, debugstr_sqlstr(ProcName, NameLength3), NameLength3, debugstr_sqlstr(ColumnName, NameLength4),
          NameLength4);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = procedure_columns_unix_a( stmt, CatalogName, NameLength1, SchemaName, NameLength2, ProcName,
                                        NameLength3, ColumnName, NameLength4 );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = procedure_columns_win32_a( stmt, CatalogName, NameLength1, SchemaName, NameLength2, ProcName,
                                         NameLength3, ColumnName, NameLength4 );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
    return ret;
}

static SQLRETURN procedures_unix_a( struct statement *stmt, SQLCHAR *catalog, SQLSMALLINT len1, SQLCHAR *schema,
                                    SQLSMALLINT len2, SQLCHAR *proc, SQLSMALLINT len3 )
{
    struct SQLProcedures_params params = { stmt->hdr.unix_handle, catalog, len1, schema, len2, proc, len3 };
    return ODBC_CALL( SQLProcedures, &params );
}

static SQLRETURN procedures_win32_a( struct statement *stmt, SQLCHAR *catalog, SQLSMALLINT len1, SQLCHAR *schema,
                                     SQLSMALLINT len2, SQLCHAR *proc, SQLSMALLINT len3 )
{
    WCHAR *catalogW = NULL, *schemaW = NULL, *procW = NULL;
    SQLRETURN ret = SQL_ERROR;

    if (stmt->hdr.win32_funcs->SQLProcedures)
        return stmt->hdr.win32_funcs->SQLProcedures( stmt->hdr.win32_handle, catalog, len1, schema, len2, proc,
                                                     len3 );

    if (stmt->hdr.win32_funcs->SQLProceduresW)
    {
        if (!(catalogW = strnAtoW( catalog, len1 ))) goto done;
        if (!(schemaW = strnAtoW( schema, len2 ))) goto done;
        if (!(procW = strnAtoW( proc, len3 ))) goto done;
        ret = stmt->hdr.win32_funcs->SQLProceduresW( stmt->hdr.win32_handle, catalogW, len1, schemaW, len2, procW,
                                                     len3 );
    }

done:
    free( catalogW );
    free( schemaW );
    free( procW );
    return ret;
}

/*************************************************************************
 *				SQLProcedures           [ODBC32.067]
 */
SQLRETURN WINAPI SQLProcedures(SQLHSTMT StatementHandle, SQLCHAR *CatalogName, SQLSMALLINT NameLength1,
                               SQLCHAR *SchemaName, SQLSMALLINT NameLength2, SQLCHAR *ProcName,
                               SQLSMALLINT NameLength3)
{
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d, ProcName %s,"
          " NameLength3 %d)\n", StatementHandle,
          debugstr_sqlstr(CatalogName, NameLength1), NameLength1, debugstr_sqlstr(SchemaName, NameLength2),
          NameLength2, debugstr_sqlstr(ProcName, NameLength3), NameLength3);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = procedures_unix_a( stmt, CatalogName, NameLength1, SchemaName, NameLength2, ProcName, NameLength3 );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = procedures_win32_a( stmt, CatalogName, NameLength1, SchemaName, NameLength2, ProcName, NameLength3 );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
    return ret;
}

static SQLRETURN set_pos_unix( struct statement *stmt, SQLSETPOSIROW row, SQLUSMALLINT op, SQLUSMALLINT lock )
{
    struct SQLSetPos_params params = { stmt->hdr.unix_handle, row, op, lock };
    SQLRETURN ret;

    if (SUCCESS(( ret = ODBC_CALL( SQLSetPos, &params ))) && op == SQL_REFRESH)
        update_result_lengths( stmt, SQL_PARAM_OUTPUT );
    return ret;
}

static SQLRETURN set_pos_win32( struct statement *stmt, SQLSETPOSIROW row, SQLUSMALLINT op, SQLUSMALLINT lock )
{
    if (stmt->hdr.win32_funcs->SQLSetPos)
        return stmt->hdr.win32_funcs->SQLSetPos( stmt->hdr.win32_handle, row, op, lock );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLSetPos           [ODBC32.068]
 */
SQLRETURN WINAPI SQLSetPos(SQLHSTMT StatementHandle, SQLSETPOSIROW RowNumber, SQLUSMALLINT Operation,
                           SQLUSMALLINT LockType)
{
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, RowNumber %s, Operation %d, LockType %d)\n", StatementHandle,
          debugstr_sqlulen(RowNumber), Operation, LockType);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = set_pos_unix( stmt, RowNumber, Operation, LockType );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = set_pos_win32( stmt, RowNumber, Operation, LockType );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
    return ret;
}

static SQLRETURN table_privileges_unix_a( struct statement *stmt, SQLCHAR *catalog, SQLSMALLINT len1, SQLCHAR *schema,
                                          SQLSMALLINT len2, SQLCHAR *table, SQLSMALLINT len3 )
{
    struct SQLTablePrivileges_params params = { stmt->hdr.unix_handle, catalog, len1, schema, len2, table, len3 };
    return ODBC_CALL( SQLTablePrivileges, &params );
}

static SQLRETURN table_privileges_win32_a( struct statement *stmt, SQLCHAR *catalog, SQLSMALLINT len1, SQLCHAR *schema,
                                           SQLSMALLINT len2, SQLCHAR *table, SQLSMALLINT len3 )
{
    WCHAR *catalogW = NULL, *schemaW = NULL, *tableW = NULL;
    SQLRETURN ret = SQL_ERROR;

    if (stmt->hdr.win32_funcs->SQLTablePrivileges)
        return stmt->hdr.win32_funcs->SQLTablePrivileges( stmt->hdr.win32_handle, catalog, len1, schema, len2, table,
                                                          len3 );
    if (stmt->hdr.win32_funcs->SQLTablePrivilegesW)
    {
        if (!(catalogW = strnAtoW( catalog, len1 ))) goto done;
        if (!(schemaW = strnAtoW( schema, len2 ))) goto done;
        if (!(tableW = strnAtoW( table, len3 ))) goto done;
        ret = stmt->hdr.win32_funcs->SQLTablePrivilegesW( stmt->hdr.win32_handle, catalogW, len1, schemaW, len2,
                                                          tableW, len3 );
    }
done:
    free( catalogW );
    free( schemaW );
    free( tableW );
    return ret;
}

/*************************************************************************
 *				SQLTablePrivileges           [ODBC32.070]
 */
SQLRETURN WINAPI SQLTablePrivileges(SQLHSTMT StatementHandle, SQLCHAR *CatalogName, SQLSMALLINT NameLength1,
                                    SQLCHAR *SchemaName, SQLSMALLINT NameLength2, SQLCHAR *TableName,
                                    SQLSMALLINT NameLength3)
{
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d, TableName %s,"
          "NameLength3  %d)\n", StatementHandle,
          debugstr_sqlstr(CatalogName, NameLength1), NameLength1, debugstr_sqlstr(SchemaName, NameLength2),
          NameLength2, debugstr_sqlstr(TableName, NameLength3), NameLength3);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = table_privileges_unix_a( stmt, CatalogName, NameLength1, SchemaName, NameLength2, TableName,
                                       NameLength3 );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = table_privileges_win32_a( stmt, CatalogName, NameLength1, SchemaName, NameLength2, TableName,
                                        NameLength3 );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
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
    struct environment *env = (struct environment *)lock_object( EnvironmentHandle, SQL_HANDLE_ENV );
    DWORD len_desc = BufferLength1;
    SQLRETURN ret = SQL_ERROR;
    LONG res;

    TRACE("(EnvironmentHandle %p, Direction %d, DriverDescription %p, BufferLength1 %d, DescriptionLength %p,"
          " DriverAttributes %p, BufferLength2 %d, AttributesLength %p)\n", EnvironmentHandle, Direction,
          DriverDescription, BufferLength1, DescriptionLength, DriverAttributes, BufferLength2, AttributesLength);

    if (!env) return SQL_INVALID_HANDLE;

    if (Direction == SQL_FETCH_FIRST || (Direction == SQL_FETCH_NEXT && !env->drivers_key))
    {
        env->drivers_idx = 0;
        RegCloseKey( env->drivers_key );
        if (!(env->drivers_key = open_drivers_key())) goto done;
    }

    res = RegEnumValueA( env->drivers_key, env->drivers_idx, (char *)DriverDescription, &len_desc,
                         NULL, NULL, NULL, NULL );
    if (res == ERROR_NO_MORE_ITEMS)
    {
        ret = SQL_NO_DATA;
        goto done;
    }
    else if (res == ERROR_SUCCESS)
    {
        if (DescriptionLength) *DescriptionLength = len_desc;

        env->drivers_idx++;
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
        RegCloseKey( env->drivers_key );
        env->drivers_key = NULL;
        env->drivers_idx = 0;
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &env->hdr );
    return ret;
}

static SQLRETURN bind_parameter_unix( struct statement *stmt, SQLUSMALLINT param, SQLSMALLINT io_type,
                                      SQLSMALLINT value_type, SQLSMALLINT param_type, SQLULEN size,
                                      SQLSMALLINT digits, SQLPOINTER value, SQLLEN buflen, SQLLEN *len )
{
    if (is_wow64 && !is_old_wow64)
    {
        struct SQLBindParameter_params params = { stmt->hdr.unix_handle, param, io_type, value_type, param_type,
                                                  size, digits, value, buflen };
        UINT i = param - 1;
        SQLRETURN ret;

        if (!param)
        {
            FIXME( "parameter 0 not handled\n" );
            return SQL_ERROR;
        }
        if (!alloc_binding( &stmt->bind_parameter, io_type, param, stmt->row_count )) return SQL_ERROR;
        stmt->bind_parameter.param[i].parameter.input_output_type = io_type;
        stmt->bind_parameter.param[i].parameter.value_type        = value_type;
        stmt->bind_parameter.param[i].parameter.parameter_type    = param_type;
        stmt->bind_parameter.param[i].parameter.column_size       = size;
        stmt->bind_parameter.param[i].parameter.decimal_digits    = digits;
        stmt->bind_parameter.param[i].parameter.parameter_value   = value;
        stmt->bind_parameter.param[i].parameter.buffer_length     = buflen;

        params.StrLen_or_Ind = stmt->bind_parameter.param[i].len;
        *(UINT64 *)params.StrLen_or_Ind = *len;
        if (SUCCESS((ret = ODBC_CALL( SQLBindParameter, &params )))) stmt->bind_parameter.param[i].ptr = len;
        return ret;
    }
    else
    {
        struct SQLBindParameter_params params = { stmt->hdr.unix_handle, param, io_type, value_type, param_type,
                                                  size, digits, value, buflen, len };
        return ODBC_CALL( SQLBindParameter, &params );
    }
}

static SQLRETURN bind_parameter_win32( struct statement *stmt, SQLUSMALLINT param, SQLSMALLINT io_type,
                                       SQLSMALLINT value_type, SQLSMALLINT param_type, SQLULEN size,
                                       SQLSMALLINT digits, SQLPOINTER value, SQLLEN buflen, SQLLEN *len )
{
    if (stmt->hdr.win32_funcs->SQLBindParameter)
        return stmt->hdr.win32_funcs->SQLBindParameter( stmt->hdr.win32_handle, param, io_type, value_type,
                                                        param_type, size, digits, value, buflen, len );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLBindParameter           [ODBC32.072]
 */
SQLRETURN WINAPI SQLBindParameter(SQLHSTMT StatementHandle, SQLUSMALLINT ParameterNumber, SQLSMALLINT InputOutputType,
                                  SQLSMALLINT ValueType, SQLSMALLINT ParameterType, SQLULEN ColumnSize,
                                  SQLSMALLINT DecimalDigits, SQLPOINTER ParameterValue, SQLLEN BufferLength,
                                  SQLLEN *StrLen_or_Ind)
{
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, ParameterNumber %d, InputOutputType %d, ValueType %d, ParameterType %d, "
          "ColumnSize %s, DecimalDigits %d, ParameterValue, %p, BufferLength %s, StrLen_or_Ind %p)\n",
          StatementHandle, ParameterNumber, InputOutputType, ValueType, ParameterType, debugstr_sqlulen(ColumnSize),
          DecimalDigits, ParameterValue, debugstr_sqllen(BufferLength), StrLen_or_Ind);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = bind_parameter_unix( stmt, ParameterNumber, InputOutputType, ValueType, ParameterType, ColumnSize,
                                   DecimalDigits, ParameterValue, BufferLength, StrLen_or_Ind );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = bind_parameter_win32( stmt, ParameterNumber, InputOutputType, ValueType, ParameterType, ColumnSize,
                                    DecimalDigits, ParameterValue, BufferLength, StrLen_or_Ind );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
    return ret;
}

static SQLRETURN driver_connect_win32_a( struct connection *con, SQLHWND window, SQLCHAR *in_conn_str,
                                         SQLSMALLINT inlen, SQLCHAR *out_conn_str, SQLSMALLINT buflen,
                                         SQLSMALLINT *outlen, SQLUSMALLINT completion )
{
    SQLRETURN ret = SQL_ERROR;
    SQLWCHAR *in = NULL, *out = NULL;
    SQLSMALLINT lenW;

    if (con->hdr.win32_funcs->SQLDriverConnect)
        return con->hdr.win32_funcs->SQLDriverConnect( con->hdr.win32_handle, window, in_conn_str, inlen, out_conn_str,
                                                       buflen, outlen, completion );

    if (con->hdr.win32_funcs->SQLDriverConnectW)
    {
        if (!(in = strnAtoW( in_conn_str, inlen ))) return SQL_ERROR;
        if (!(out = malloc( buflen * sizeof(WCHAR) ))) goto done;
        ret = con->hdr.win32_funcs->SQLDriverConnectW( con->hdr.win32_handle, window, in, inlen, out, buflen, &lenW,
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

static SQLRETURN driver_connect_unix_a( struct connection *con, SQLHWND window, SQLCHAR *in_conn_str, SQLSMALLINT len,
                                        SQLCHAR *out_conn_str, SQLSMALLINT buflen, SQLSMALLINT *len2,
                                        SQLUSMALLINT completion )
{
    struct SQLDriverConnect_params params = { con->hdr.unix_handle, window, in_conn_str, len, out_conn_str, buflen,
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
    struct connection *con = (struct connection *)lock_object( ConnectionHandle, SQL_HANDLE_DBC );
    const WCHAR *datasource, *drivername = NULL;
    WCHAR *filename = NULL, *connect_string = NULL, *strW = strdupAW( (const char *)InConnectionString );
    SQLCHAR *strA = NULL;
    struct attribute_list attrs;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(ConnectionHandle %p, WindowHandle %p, InConnectionString %s, Length %d, OutConnectionString %p,"
          " BufferLength %d, Length2 %p, DriverCompletion %d)\n", ConnectionHandle, WindowHandle,
          debugstr_sqlstr(InConnectionString, Length), Length, OutConnectionString, BufferLength,
          Length2, DriverCompletion);

    if (!con) return SQL_INVALID_HANDLE;

    if (parse_connect_string( &attrs, strW ) || !(connect_string = build_connect_string( &attrs )) ||
        !(strA = (SQLCHAR *)strdupWA( connect_string ))) goto done;

    if (!(datasource = get_attribute( &attrs, L"DSN" )) && !(drivername = get_attribute( &attrs, L"DRIVER" )))
    {
        WARN( "can't find data source or driver name\n" );
        goto done;
    }
    if ((datasource && !(filename = get_driver_filename_from_source( datasource ))) ||
        (drivername && !(filename = get_driver_filename_from_name( drivername ))))
    {
        WARN( "can't find driver filename\n" );
        goto done;
    }

    if (has_suffix( filename, L".dll" ))
    {
        if (!(con->hdr.win32_funcs = con->hdr.parent->win32_funcs = load_driver( filename )))
        {
            WARN( "failed to load driver %s\n", debugstr_w(filename) );
            goto done;
        }
        TRACE( "using Windows driver %s\n", debugstr_w(filename) );

        if (!SUCCESS((ret = create_env( (struct environment *)con->hdr.parent, FALSE )))) goto done;
        if (!SUCCESS((ret = create_con( con )))) goto done;

        ret = driver_connect_win32_a( con, WindowHandle, strA, Length, OutConnectionString, BufferLength, Length2,
                                      DriverCompletion );
    }
    else
    {
        TRACE( "using Unix driver %s\n", debugstr_w(filename) );

        if (!SUCCESS((ret = create_env( (struct environment *)con->hdr.parent, TRUE )))) goto done;
        if (!SUCCESS((ret = create_con( con )))) goto done;

        ret = driver_connect_unix_a( con, WindowHandle, strA, Length, OutConnectionString, BufferLength, Length2,
                                     DriverCompletion );
    }

done:
    free( strA );
    free( strW );
    free( connect_string );
    free_attribute_list( &attrs );
    free( filename );

    TRACE("Returning %d\n", ret);
    unlock_object( &con->hdr );
    return ret;
}

static SQLRETURN set_scroll_options_unix( struct statement *stmt, SQLUSMALLINT concurrency, SQLLEN keyset_size,
                                          SQLUSMALLINT rowset_size )
{
    struct SQLSetScrollOptions_params params = { stmt->hdr.unix_handle, concurrency, keyset_size, rowset_size };
    return ODBC_CALL( SQLSetScrollOptions, &params );
}

static SQLRETURN set_scroll_options_win32( struct statement *stmt, SQLUSMALLINT concurrency, SQLLEN keyset_size,
                                           SQLUSMALLINT rowset_size )
{
    if (stmt->hdr.win32_funcs->SQLSetScrollOptions)
        return stmt->hdr.win32_funcs->SQLSetScrollOptions( stmt->hdr.win32_handle, concurrency, keyset_size,
                                                           rowset_size );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLSetScrollOptions           [ODBC32.069]
 */
SQLRETURN WINAPI SQLSetScrollOptions(SQLHSTMT StatementHandle, SQLUSMALLINT Concurrency, SQLLEN KeySetSize,
                                     SQLUSMALLINT RowSetSize)
{
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, Concurrency %d, KeySetSize %s, RowSetSize %d)\n", StatementHandle,
          Concurrency, debugstr_sqllen(KeySetSize), RowSetSize);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = set_scroll_options_unix( stmt, Concurrency, KeySetSize, RowSetSize );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = set_scroll_options_win32( stmt, Concurrency, KeySetSize, RowSetSize );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
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

static SQLRETURN col_attributes_unix_w( struct statement *stmt, SQLUSMALLINT col, SQLUSMALLINT field_id,
                                        SQLPOINTER char_attrs, SQLSMALLINT buflen, SQLSMALLINT *retlen,
                                        SQLLEN *num_attrs )
{
    SQLRETURN ret;
    INT64 attrs;
    struct SQLColAttributesW_params params = { stmt->hdr.unix_handle, col, field_id, char_attrs, buflen, retlen,
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

static SQLRETURN col_attributes_win32_w( struct statement *stmt, SQLUSMALLINT col, SQLUSMALLINT field_id,
                                         SQLPOINTER char_attrs, SQLSMALLINT buflen, SQLSMALLINT *retlen,
                                         SQLLEN *num_attrs )
{
    if (stmt->hdr.win32_funcs->SQLColAttributesW)
        return stmt->hdr.win32_funcs->SQLColAttributesW( stmt->hdr.win32_handle, col, field_id, char_attrs, buflen,
                                                         retlen, num_attrs );
    if (stmt->hdr.win32_funcs->SQLColAttributes) FIXME( "Unicode to ANSI conversion not handled\n" );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLColAttributesW          [ODBC32.106]
 */
SQLRETURN WINAPI SQLColAttributesW(SQLHSTMT StatementHandle, SQLUSMALLINT ColumnNumber, SQLUSMALLINT FieldIdentifier,
                                   SQLPOINTER CharacterAttributes, SQLSMALLINT BufferLength, SQLSMALLINT *StringLength,
                                   SQLLEN *NumericAttributes)
{
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, ColumnNumber %d, FieldIdentifier %d, CharacterAttributes %p, BufferLength %d, "
          "StringLength %p, NumericAttributes %p)\n", StatementHandle, ColumnNumber, FieldIdentifier,
          CharacterAttributes, BufferLength, StringLength, NumericAttributes);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = col_attributes_unix_w( stmt, ColumnNumber, FieldIdentifier, CharacterAttributes, BufferLength,
                                     StringLength, NumericAttributes );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = col_attributes_win32_w( stmt, ColumnNumber, FieldIdentifier, CharacterAttributes, BufferLength,
                                      StringLength, NumericAttributes );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
    return ret;
}

static const char *debugstr_sqlwstr( const SQLWCHAR *str, SQLSMALLINT len )
{
    if (len == SQL_NTS) return wine_dbgstr_w( str );
    return wine_dbgstr_wn( str, len );
}

static SQLRETURN connect_win32_w( struct connection *con, SQLWCHAR *servername, SQLSMALLINT len1, SQLWCHAR *username,
                                  SQLSMALLINT len2, SQLWCHAR *auth, SQLSMALLINT len3 )
{
    if (con->hdr.win32_funcs->SQLConnectW)
        return con->hdr.win32_funcs->SQLConnectW( con->hdr.win32_handle, servername, len1, username, len2, auth, len3 );
    if (con->hdr.win32_funcs->SQLConnect) FIXME( "Unicode to ANSI conversion not handled\n" );
    return SQL_ERROR;
}

static SQLRETURN connect_unix_w( struct connection *con, SQLWCHAR *servername, SQLSMALLINT len1, SQLWCHAR *username,
                                 SQLSMALLINT len2, SQLWCHAR *auth, SQLSMALLINT len3 )
{
    struct SQLConnectW_params params = { con->hdr.unix_handle, servername, len1, username, len2, auth, len3 };
    return ODBC_CALL( SQLConnectW, &params );
}

/*************************************************************************
 *				SQLConnectW          [ODBC32.107]
 */
SQLRETURN WINAPI SQLConnectW(SQLHDBC ConnectionHandle, SQLWCHAR *ServerName, SQLSMALLINT NameLength1,
                             SQLWCHAR *UserName, SQLSMALLINT NameLength2, SQLWCHAR *Authentication,
                             SQLSMALLINT NameLength3)
{
    struct connection *con = (struct connection *)lock_object( ConnectionHandle, SQL_HANDLE_DBC );
    WCHAR *filename;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(ConnectionHandle %p, ServerName %s, NameLength1 %d, UserName %s, NameLength2 %d, Authentication %s,"
          " NameLength3 %d)\n", ConnectionHandle, debugstr_sqlwstr(ServerName, NameLength1), NameLength1,
          debugstr_sqlwstr(UserName, NameLength2), NameLength2, debugstr_sqlwstr(Authentication, NameLength3),
          NameLength3);

    if (!con) return SQL_INVALID_HANDLE;

    if (!(filename = get_driver_filename_from_source( ServerName )))
    {
        WARN( "can't find driver filename\n" );
        goto done;
    }

    if (has_suffix( filename, L".dll" ))
    {
        if (!(con->hdr.win32_funcs = con->hdr.parent->win32_funcs = load_driver( filename )))
        {
            WARN( "failed to load driver %s\n", debugstr_w(filename) );
            goto done;
        }
        TRACE( "using Windows driver %s\n", debugstr_w(filename) );

        if (!SUCCESS((ret = create_env( (struct environment *)con->hdr.parent, FALSE )))) goto done;
        if (!SUCCESS((ret = create_con( con )))) goto done;

        ret = connect_win32_w( con, ServerName, NameLength1, UserName, NameLength2, Authentication, NameLength3 );
    }
    else
    {
        TRACE( "using Unix driver %s\n", debugstr_w(filename) );

        if (!SUCCESS((ret = create_env( (struct environment *)con->hdr.parent, TRUE )))) goto done;
        if (!SUCCESS((ret = create_con( con )))) goto done;

        ret = connect_unix_w( con, ServerName, NameLength1, UserName, NameLength2, Authentication, NameLength3 );
    }

done:
    free( filename );

    TRACE("Returning %d\n", ret);
    unlock_object( &con->hdr );
    return ret;
}

static SQLRETURN describe_col_unix_w( struct statement *stmt, SQLUSMALLINT col_number, SQLWCHAR *col_name,
                                      SQLSMALLINT buf_len, SQLSMALLINT *name_len, SQLSMALLINT *data_type,
                                      SQLULEN *col_size, SQLSMALLINT *decimal_digits, SQLSMALLINT *nullable )
{
    SQLRETURN ret;
    SQLSMALLINT dummy;
    UINT64 size;
    struct SQLDescribeColW_params params = { stmt->hdr.unix_handle, col_number, col_name, buf_len, name_len,
                                             data_type, &size, decimal_digits, nullable };
    if (!name_len) params.NameLength = &dummy; /* workaround for drivers that don't accept NULL NameLength */

    if (SUCCESS((ret = ODBC_CALL( SQLDescribeColW, &params ))) && col_size) *col_size = size;
    return ret;
}

static SQLRETURN describe_col_win32_w( struct statement *stmt, SQLUSMALLINT col_number, SQLWCHAR *col_name,
                                       SQLSMALLINT buf_len, SQLSMALLINT *name_len, SQLSMALLINT *data_type,
                                       SQLULEN *col_size, SQLSMALLINT *decimal_digits, SQLSMALLINT *nullable )
{
    if (stmt->hdr.win32_funcs->SQLDescribeColW)
        return stmt->hdr.win32_funcs->SQLDescribeColW( stmt->hdr.win32_handle, col_number, col_name, buf_len,
                                                       name_len, data_type, col_size, decimal_digits, nullable );
    if (stmt->hdr.win32_funcs->SQLDescribeCol) FIXME( "Unicode to ANSI conversion not handled\n" );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLDescribeColW          [ODBC32.108]
 */
SQLRETURN WINAPI SQLDescribeColW(SQLHSTMT StatementHandle, SQLUSMALLINT ColumnNumber, SQLWCHAR *ColumnName,
                                 SQLSMALLINT BufferLength, SQLSMALLINT *NameLength, SQLSMALLINT *DataType,
                                 SQLULEN *ColumnSize, SQLSMALLINT *DecimalDigits, SQLSMALLINT *Nullable)
{
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, ColumnNumber %d, ColumnName %p, BufferLength %d, NameLength %p, DataType %p,"
          " ColumnSize %p, DecimalDigits %p, Nullable %p)\n", StatementHandle, ColumnNumber, ColumnName,
          BufferLength, NameLength, DataType, ColumnSize, DecimalDigits, Nullable);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = describe_col_unix_w( stmt, ColumnNumber, ColumnName, BufferLength, NameLength, DataType,
                                   ColumnSize, DecimalDigits, Nullable );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = describe_col_win32_w( stmt, ColumnNumber, ColumnName, BufferLength, NameLength, DataType,
                                    ColumnSize, DecimalDigits, Nullable );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
    return ret;
}

static SQLRETURN error_unix_w( struct environment *env, struct connection *con, struct statement *stmt, SQLWCHAR *state,
                               SQLINTEGER *native_err, SQLWCHAR *msg, SQLSMALLINT buflen, SQLSMALLINT *retlen )
{
    struct SQLErrorW_params params = { env ? env->hdr.unix_handle : 0, con ? con->hdr.unix_handle : 0,
                                       stmt ? stmt->hdr.unix_handle : 0, state, native_err, msg, buflen, retlen };
    return ODBC_CALL( SQLErrorW, &params );
}

static SQLRETURN error_win32_w( struct environment *env, struct connection *con, struct statement *stmt, SQLWCHAR *state,
                                SQLINTEGER *native_err, SQLWCHAR *msg, SQLSMALLINT buflen, SQLSMALLINT *retlen )
{
    const struct win32_funcs *win32_funcs;
    SQLRETURN ret;

    if (env) win32_funcs = env->hdr.win32_funcs;
    else if (con) win32_funcs = con->hdr.win32_funcs;
    else win32_funcs = stmt->hdr.win32_funcs;

    if (win32_funcs->SQLErrorW)
        return win32_funcs->SQLErrorW( env ? env->hdr.win32_handle : NULL, con ? con->hdr.win32_handle : NULL,
                                       stmt ? stmt->hdr.win32_handle : NULL, state, native_err, msg, buflen, retlen );
    if (win32_funcs->SQLError)
    {
        SQLCHAR stateA[6], *msgA;
        SQLSMALLINT lenA;

        if (!(msgA = malloc( buflen * sizeof(*msgA) ))) return SQL_ERROR;
        ret = win32_funcs->SQLError( env ? env->hdr.win32_handle : NULL, con ? con->hdr.win32_handle : NULL,
                stmt ? stmt->hdr.win32_handle : NULL, stateA, native_err, msgA, buflen, &lenA );
        if (SUCCESS( ret ))
        {
            int len = MultiByteToWideChar( CP_ACP, 0, (const char *)msgA, -1, msg, buflen );
            if (retlen) *retlen = len - 1;
            MultiByteToWideChar( CP_ACP, 0, (const char *)stateA, -1, state, 6 );
        }
        free( msgA );

        return ret;
    }

    if (win32_funcs->SQLGetDiagRecW) FIXME("Use SQLGetDiagRecW\n");
    else if (win32_funcs->SQLGetDiagRec) FIXME("Use SQLGetDiagRec\n");

    return SQL_ERROR;
}

/*************************************************************************
 *				SQLErrorW          [ODBC32.110]
 */
SQLRETURN WINAPI SQLErrorW(SQLHENV EnvironmentHandle, SQLHDBC ConnectionHandle, SQLHSTMT StatementHandle,
                           SQLWCHAR *SqlState, SQLINTEGER *NativeError, SQLWCHAR *MessageText,
                           SQLSMALLINT BufferLength, SQLSMALLINT *TextLength)
{
    struct environment *env = (struct environment *)lock_object( EnvironmentHandle, SQL_HANDLE_ENV );
    struct connection *con = (struct connection *)lock_object( ConnectionHandle, SQL_HANDLE_DBC );
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(EnvironmentHandle %p, ConnectionHandle %p, StatementHandle %p, SqlState %p, NativeError %p,"
          " MessageText %p, BufferLength %d, TextLength %p)\n", EnvironmentHandle, ConnectionHandle,
          StatementHandle, SqlState, NativeError, MessageText, BufferLength, TextLength);

    if (!env && !con && !stmt) return SQL_INVALID_HANDLE;

    if ((env && env->hdr.unix_handle) || (con && con->hdr.unix_handle) || (stmt && stmt->hdr.unix_handle))
    {
        ret = error_unix_w( env, con, stmt, SqlState, NativeError, MessageText, BufferLength, TextLength );
    }
    else if ((env && env->hdr.win32_handle) || (con && con->hdr.win32_handle) || (stmt && stmt->hdr.win32_handle))
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
    if (env) unlock_object( &env->hdr );
    if (con) unlock_object( &con->hdr );
    if (stmt) unlock_object( &stmt->hdr );
    return ret;
}

static SQLRETURN exec_direct_unix_w( struct statement *stmt, SQLWCHAR *text, SQLINTEGER len )
{
    struct SQLExecDirectW_params params = { stmt->hdr.unix_handle, text, len };
    return ODBC_CALL( SQLExecDirectW, &params );
}

static SQLRETURN exec_direct_win32_w( struct statement *stmt, SQLWCHAR *text, SQLINTEGER len )
{
    if (stmt->hdr.win32_funcs->SQLExecDirectW)
        return stmt->hdr.win32_funcs->SQLExecDirectW( stmt->hdr.win32_handle, text, len );
    if (stmt->hdr.win32_funcs->SQLExecDirect) FIXME( "Unicode to ANSI conversion not handled\n" );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLExecDirectW          [ODBC32.111]
 */
SQLRETURN WINAPI SQLExecDirectW(SQLHSTMT StatementHandle, SQLWCHAR *StatementText, SQLINTEGER TextLength)
{
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, StatementText %s, TextLength %d)\n", StatementHandle,
          debugstr_sqlwstr(StatementText, TextLength), TextLength);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = exec_direct_unix_w( stmt, StatementText, TextLength );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = exec_direct_win32_w( stmt, StatementText, TextLength );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
    return ret;
}

static SQLRETURN get_cursor_name_unix_w( struct statement *stmt, SQLWCHAR *name, SQLSMALLINT buflen,
                                         SQLSMALLINT *retlen )
{
    struct SQLGetCursorNameW_params params = { stmt->hdr.unix_handle, name, buflen, retlen };
    return ODBC_CALL( SQLGetCursorNameW, &params );
}

static SQLRETURN get_cursor_name_win32_w( struct statement *stmt, SQLWCHAR *name, SQLSMALLINT buflen,
                                          SQLSMALLINT *retlen )
{
    if (stmt->hdr.win32_funcs->SQLGetCursorNameW)
        return stmt->hdr.win32_funcs->SQLGetCursorNameW( stmt->hdr.win32_handle, name, buflen, retlen );
    if (stmt->hdr.win32_funcs->SQLGetCursorName) FIXME( "Unicode to ANSI conversion not handled\n" );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLGetCursorNameW          [ODBC32.117]
 */
SQLRETURN WINAPI SQLGetCursorNameW(SQLHSTMT StatementHandle, SQLWCHAR *CursorName, SQLSMALLINT BufferLength,
                                   SQLSMALLINT *NameLength)
{
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, CursorName %p, BufferLength %d, NameLength %p)\n", StatementHandle, CursorName,
          BufferLength, NameLength);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = get_cursor_name_unix_w( stmt, CursorName, BufferLength, NameLength );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = get_cursor_name_win32_w( stmt, CursorName, BufferLength, NameLength );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
    return ret;
}

static SQLRETURN prepare_unix_w( struct statement *stmt, SQLWCHAR *statement, SQLINTEGER len )
{
    struct SQLPrepareW_params params = { stmt->hdr.unix_handle, statement, len };
    return ODBC_CALL( SQLPrepareW, &params );
}

static SQLRETURN prepare_win32_w( struct statement *stmt, SQLWCHAR *statement, SQLINTEGER len )
{
    if (stmt->hdr.win32_funcs->SQLPrepareW)
        return stmt->hdr.win32_funcs->SQLPrepareW( stmt->hdr.win32_handle, statement, len );
    if (stmt->hdr.win32_funcs->SQLPrepare) FIXME( "Unicode to ANSI conversion not handled\n" );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLPrepareW          [ODBC32.119]
 */
SQLRETURN WINAPI SQLPrepareW(SQLHSTMT StatementHandle, SQLWCHAR *StatementText, SQLINTEGER TextLength)
{
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, StatementText %s, TextLength %d)\n", StatementHandle,
          debugstr_sqlwstr(StatementText, TextLength), TextLength);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = prepare_unix_w( stmt, StatementText, TextLength );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = prepare_win32_w( stmt, StatementText, TextLength );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
    return ret;
}

static SQLRETURN set_cursor_name_unix_w( struct statement *stmt, SQLWCHAR *name, SQLSMALLINT len )
{
    struct SQLSetCursorNameW_params params = { stmt->hdr.unix_handle, name, len };
    return ODBC_CALL( SQLSetCursorNameW, &params );
}

static SQLRETURN set_cursor_name_win32_w( struct statement *stmt, SQLWCHAR *name, SQLSMALLINT len )
{
    if (stmt->hdr.win32_funcs->SQLSetCursorNameW)
        return stmt->hdr.win32_funcs->SQLSetCursorNameW( stmt->hdr.win32_handle, name, len );
    if (stmt->hdr.win32_funcs->SQLSetCursorName) FIXME( "Unicode to ANSI conversion not handled\n" );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLSetCursorNameW          [ODBC32.121]
 */
SQLRETURN WINAPI SQLSetCursorNameW(SQLHSTMT StatementHandle, SQLWCHAR *CursorName, SQLSMALLINT NameLength)
{
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, CursorName %s, NameLength %d)\n", StatementHandle,
          debugstr_sqlwstr(CursorName, NameLength), NameLength);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = set_cursor_name_unix_w( stmt, CursorName, NameLength );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = set_cursor_name_win32_w( stmt, CursorName, NameLength );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
    return ret;
}

static SQLRETURN col_attribute_unix_w( struct statement *stmt, SQLUSMALLINT col, SQLUSMALLINT field_id,
                                       SQLPOINTER char_attr, SQLSMALLINT buflen, SQLSMALLINT *retlen,
                                       SQLLEN *num_attr )
{
    SQLRETURN ret;
    INT64 attr;
    struct SQLColAttributeW_params params = { stmt->hdr.unix_handle, col, field_id, char_attr, buflen, retlen, &attr };

    if (SUCCESS((ret = ODBC_CALL( SQLColAttributeW, &params ))) && num_attr) *num_attr = attr;

    if (ret == SQL_SUCCESS && SQLColAttributes_KnownStringAttribute(field_id) && char_attr &&
        retlen && *retlen != wcslen( char_attr ) * sizeof(WCHAR))
    {
        TRACE("CHEAT: resetting name length for ADO\n");
        *retlen = wcslen( char_attr ) * sizeof(WCHAR);
    }
    return ret;
}

static SQLRETURN col_attribute_win32_w( struct statement *stmt, SQLUSMALLINT col, SQLUSMALLINT field_id,
                                        SQLPOINTER char_attr, SQLSMALLINT buflen, SQLSMALLINT *retlen,
                                        SQLLEN *num_attr )
{
    if (stmt->hdr.win32_funcs->SQLColAttributeW)
        return stmt->hdr.win32_funcs->SQLColAttributeW( stmt->hdr.win32_handle, col, field_id, char_attr, buflen,
                                                       retlen, num_attr );

    if (stmt->hdr.win32_funcs->SQLColAttribute)
    {
        FIXME( "Unicode to ANSI conversion not handled\n" );
        return SQL_ERROR;
    }

    if (stmt->hdr.win32_funcs->SQLColAttributesW)
    {
        if (buflen < 0) return SQL_ERROR;
        if (!col)
        {
            FIXME( "column 0 not handled\n" );
            return SQL_ERROR;
        }

        switch (field_id)
        {
        case SQL_DESC_COUNT:
            field_id = SQL_COLUMN_COUNT;
            break;

        case SQL_DESC_TYPE:
            field_id = SQL_COLUMN_TYPE;
            break;

        case SQL_DESC_LENGTH:
            field_id = SQL_COLUMN_LENGTH;
            break;

        case SQL_DESC_PRECISION:
            field_id = SQL_COLUMN_PRECISION;
            break;

        case SQL_DESC_SCALE:
            field_id = SQL_COLUMN_SCALE;
            break;

        case SQL_DESC_NULLABLE:
            field_id = SQL_COLUMN_NULLABLE;
            break;

        case SQL_DESC_NAME:
            field_id = SQL_COLUMN_NAME;
            break;

        case SQL_COLUMN_NAME:
        case SQL_COLUMN_TYPE:
        case SQL_COLUMN_DISPLAY_SIZE:
        case SQL_MAX_COLUMNS_IN_TABLE:
            break;

        default:
            FIXME( "field id %u not handled\n", field_id );
            return SQL_ERROR;
        }

        return stmt->hdr.win32_funcs->SQLColAttributesW( stmt->hdr.win32_handle, col, field_id, char_attr, buflen,
                                                         retlen, num_attr );
    }

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
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("StatementHandle %p ColumnNumber %d FieldIdentifier %d CharacterAttribute %p BufferLength %d"
          " StringLength %p NumericAttribute %p\n", StatementHandle, ColumnNumber, FieldIdentifier,
          CharacterAttribute, BufferLength, StringLength, NumericAttribute);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = col_attribute_unix_w( stmt, ColumnNumber, FieldIdentifier, CharacterAttribute, BufferLength,
                                    StringLength, NumericAttribute );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = col_attribute_win32_w( stmt, ColumnNumber, FieldIdentifier, CharacterAttribute, BufferLength,
                                     StringLength, NumericAttribute );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
    return ret;
}

static SQLRETURN get_connect_attr_unix_w( struct connection *con, SQLINTEGER attr, SQLPOINTER value,
                                          SQLINTEGER buflen, SQLINTEGER *retlen )
{
    struct SQLGetConnectAttrW_params params = { con->hdr.unix_handle, attr, value, buflen, retlen };
    return ODBC_CALL( SQLGetConnectAttrW, &params );
}

static SQLRETURN get_connect_attr_win32_w( struct connection *con, SQLINTEGER attr, SQLPOINTER value,
                                           SQLINTEGER buflen, SQLINTEGER *retlen )
{
    if (con->hdr.win32_funcs->SQLGetConnectAttrW)
        return con->hdr.win32_funcs->SQLGetConnectAttrW( con->hdr.win32_handle, attr, value, buflen, retlen );
    if (con->hdr.win32_funcs->SQLGetConnectAttr) FIXME( "Unicode to ANSI conversion not handled\n" );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLGetConnectAttrW          [ODBC32.132]
 */
SQLRETURN WINAPI SQLGetConnectAttrW(SQLHDBC ConnectionHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                    SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
    struct connection *con = (struct connection *)lock_object( ConnectionHandle, SQL_HANDLE_DBC );
    SQLRETURN ret = SQL_SUCCESS;

    TRACE("(ConnectionHandle %p, Attribute %d, Value %p, BufferLength %d, StringLength %p)\n", ConnectionHandle,
          Attribute, Value, BufferLength, StringLength);

    if (!con) return SQL_INVALID_HANDLE;

    if (con->hdr.unix_handle)
    {
        return get_connect_attr_unix_w( con, Attribute, Value, BufferLength, StringLength );
    }
    else if (con->hdr.win32_handle)
    {
        ret = get_connect_attr_win32_w( con, Attribute, Value, BufferLength, StringLength );
    }
    else
    {
        switch (Attribute)
        {
        case SQL_ATTR_CONNECTION_TIMEOUT:
            *(SQLINTEGER *)Value = con->attr_con_timeout;
            break;

        case SQL_ATTR_LOGIN_TIMEOUT:
            *(SQLINTEGER *)Value = con->attr_login_timeout;
            break;

        default:
            FIXME( "unhandled attribute %d\n", Attribute );
            ret = SQL_ERROR;
            break;
        }
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &con->hdr );
    return ret;
}

static SQLRETURN get_desc_field_unix_w( struct descriptor *desc, SQLSMALLINT record, SQLSMALLINT field_id,
                                        SQLPOINTER value, SQLINTEGER buflen, SQLINTEGER *retlen )
{
    struct SQLGetDescFieldW_params params = { desc->hdr.unix_handle, record, field_id, value, buflen, retlen };
    return ODBC_CALL( SQLGetDescFieldW, &params );
}

static SQLRETURN get_desc_field_win32_w( struct descriptor *desc, SQLSMALLINT record, SQLSMALLINT field_id,
                                         SQLPOINTER value, SQLINTEGER buflen, SQLINTEGER *retlen )
{
    if (desc->hdr.win32_funcs->SQLGetDescFieldW)
        return desc->hdr.win32_funcs->SQLGetDescFieldW( desc->hdr.win32_handle, record, field_id, value, buflen, retlen );
    if (desc->hdr.win32_funcs->SQLGetDescField) FIXME( "Unicode to ANSI conversion not handled\n" );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLGetDescFieldW          [ODBC32.133]
 */
SQLRETURN WINAPI SQLGetDescFieldW(SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber, SQLSMALLINT FieldIdentifier,
                                  SQLPOINTER Value, SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
    struct descriptor *desc = (struct descriptor *)lock_object( DescriptorHandle, SQL_HANDLE_DESC );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(DescriptorHandle %p, RecNumber %d, FieldIdentifier %d, Value %p, BufferLength %d, StringLength %p)\n",
          DescriptorHandle, RecNumber, FieldIdentifier, Value, BufferLength, StringLength);

    if (!desc) return SQL_INVALID_HANDLE;

    if (desc->hdr.unix_handle)
    {
        ret = get_desc_field_unix_w( desc, RecNumber, FieldIdentifier, Value, BufferLength, StringLength );
    }
    else if (desc->hdr.win32_handle)
    {
        ret = get_desc_field_win32_w( desc, RecNumber, FieldIdentifier, Value, BufferLength, StringLength );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &desc->hdr );
    return ret;
}

static SQLRETURN get_desc_rec_unix_w( struct descriptor *desc, SQLSMALLINT record, SQLWCHAR *name, SQLSMALLINT buflen,
                                      SQLSMALLINT *retlen, SQLSMALLINT *type, SQLSMALLINT *subtype, SQLLEN *len,
                                      SQLSMALLINT *precision, SQLSMALLINT *scale, SQLSMALLINT *nullable )
{
    SQLRETURN ret = SQL_ERROR;
    INT64 len64;
    struct SQLGetDescRecW_params params = { desc->hdr.unix_handle, record, name, buflen, retlen, type, subtype, &len64,
                                            precision, scale, nullable };
    if (SUCCESS((ret = ODBC_CALL( SQLGetDescRecW, &params )))) *len = len64;
    return ret;
}

static SQLRETURN get_desc_rec_win32_w( struct descriptor *desc, SQLSMALLINT record, SQLWCHAR *name, SQLSMALLINT buflen,
                                       SQLSMALLINT *retlen, SQLSMALLINT *type, SQLSMALLINT *subtype, SQLLEN *len,
                                       SQLSMALLINT *precision, SQLSMALLINT *scale, SQLSMALLINT *nullable )
{
    if (desc->hdr.win32_funcs->SQLGetDescRecW)
        return desc->hdr.win32_funcs->SQLGetDescRecW( desc->hdr.win32_handle, record, name, buflen, retlen, type,
                                                      subtype, len, precision, scale, nullable );
    if (desc->hdr.win32_funcs->SQLGetDescRec) FIXME( "Unicode to ANSI conversion not handled\n" );
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
    struct descriptor *desc = (struct descriptor *)lock_object( DescriptorHandle, SQL_HANDLE_DESC );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(DescriptorHandle %p, RecNumber %d, Name %p, BufferLength %d, StringLength %p, Type %p, SubType %p,"
          " Length %p, Precision %p, Scale %p, Nullable %p)\n", DescriptorHandle, RecNumber, Name, BufferLength,
          StringLength, Type, SubType, Length, Precision, Scale, Nullable);

    if (!desc) return SQL_INVALID_HANDLE;

    if (desc->hdr.unix_handle)
    {
        ret = get_desc_rec_unix_w( desc, RecNumber, Name, BufferLength, StringLength, Type, SubType, Length,
                                   Precision, Scale, Nullable );
    }
    else if (desc->hdr.win32_handle)
    {
        ret = get_desc_rec_win32_w( desc, RecNumber, Name, BufferLength, StringLength, Type, SubType, Length,
                                    Precision, Scale, Nullable );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &desc->hdr );
    return ret;
}

static SQLRETURN get_diag_field_unix_w( SQLSMALLINT type, struct object *obj, SQLSMALLINT rec_num,
                                        SQLSMALLINT diag_id, SQLPOINTER diag_info, SQLSMALLINT buflen,
                                        SQLSMALLINT *retlen )
{
    struct SQLGetDiagFieldW_params params = { type, obj->unix_handle, rec_num, diag_id, diag_info, buflen,
                                              retlen };
    return ODBC_CALL( SQLGetDiagFieldW, &params );
}

static SQLRETURN get_diag_field_win32_w( SQLSMALLINT type, struct object *obj, SQLSMALLINT rec_num,
                                         SQLSMALLINT diag_id, SQLPOINTER diag_info, SQLSMALLINT buflen,
                                         SQLSMALLINT *retlen )
{
    if (obj->win32_funcs->SQLGetDiagFieldW)
        return obj->win32_funcs->SQLGetDiagFieldW( type, obj->win32_handle, rec_num, diag_id, diag_info, buflen,
                                                   retlen );
    if (obj->win32_funcs->SQLGetDiagField) FIXME( "Unicode to ANSI conversion not handled\n" );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLGetDiagFieldW          [ODBC32.135]
 */
SQLRETURN WINAPI SQLGetDiagFieldW(SQLSMALLINT HandleType, SQLHANDLE Handle, SQLSMALLINT RecNumber,
                                  SQLSMALLINT DiagIdentifier, SQLPOINTER DiagInfo, SQLSMALLINT BufferLength,
                                  SQLSMALLINT *StringLength)
{
    struct object *obj = lock_object( Handle, HandleType );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(HandleType %d, Handle %p, RecNumber %d, DiagIdentifier %d, DiagInfo %p, BufferLength %d,"
          " StringLength %p)\n", HandleType, Handle, RecNumber, DiagIdentifier, DiagInfo, BufferLength, StringLength);

    if (!obj) return SQL_INVALID_HANDLE;

    if (obj->unix_handle)
    {
        ret = get_diag_field_unix_w( HandleType, obj, RecNumber, DiagIdentifier, DiagInfo, BufferLength,
                                     StringLength );
    }
    else if (obj->win32_handle)
    {
        ret = get_diag_field_win32_w( HandleType, obj, RecNumber, DiagIdentifier, DiagInfo, BufferLength,
                                      StringLength );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( obj );
    return ret;
}

static SQLRETURN get_diag_rec_unix_w( SQLSMALLINT type, struct object *obj, SQLSMALLINT rec_num,
                                      SQLWCHAR *state, SQLINTEGER *native_err, SQLWCHAR *msg, SQLSMALLINT buflen,
                                      SQLSMALLINT *retlen )
{
    struct SQLGetDiagRecW_params params = { type, obj->unix_handle, rec_num, state, native_err, msg, buflen, retlen };
    return ODBC_CALL( SQLGetDiagRecW, &params );
}

static SQLRETURN get_diag_rec_win32_w( SQLSMALLINT type, struct object *obj, SQLSMALLINT rec_num,
                                       SQLWCHAR *state, SQLINTEGER *native_err, SQLWCHAR *msg, SQLSMALLINT buflen,
                                       SQLSMALLINT *retlen )
{
    if (obj->win32_funcs->SQLGetDiagRecW)
        return obj->win32_funcs->SQLGetDiagRecW( type, obj->win32_handle, rec_num, state, native_err, msg, buflen,
                                                 retlen );
    if (obj->win32_funcs->SQLGetDiagRec)
    {
        FIXME( "Unicode to ANSI conversion not handled\n" );
        return SQL_ERROR;
    }

    if (obj->win32_funcs->SQLErrorW)
    {
        SQLHENV env = NULL;
        SQLHDBC con = NULL;
        SQLHSTMT stmt = NULL;

        if (rec_num > 1) return SQL_NO_DATA;

        switch (type)
        {
        case SQL_HANDLE_ENV:
            env = obj->win32_handle;
            break;

        case SQL_HANDLE_DBC:
            con = obj->win32_handle;
            break;

        case SQL_HANDLE_STMT:
            stmt = obj->win32_handle;
            break;

        default:
            return SQL_ERROR;
        }

        return obj->win32_funcs->SQLErrorW( env, con, stmt, state, native_err, msg, buflen, retlen );
    }

    return SQL_ERROR;
}

/*************************************************************************
 *				SQLGetDiagRecW           [ODBC32.136]
 */
SQLRETURN WINAPI SQLGetDiagRecW(SQLSMALLINT HandleType, SQLHANDLE Handle, SQLSMALLINT RecNumber, SQLWCHAR *SqlState,
                                SQLINTEGER *NativeError, SQLWCHAR *MessageText, SQLSMALLINT BufferLength,
                                SQLSMALLINT *TextLength)
{
    struct object *obj = lock_object( Handle, HandleType );
    SQLRETURN ret = SQL_NO_DATA;

    TRACE("(HandleType %d, Handle %p, RecNumber %d, SqlState %p, NativeError %p, MessageText %p, BufferLength %d,"
          " TextLength %p)\n", HandleType, Handle, RecNumber, SqlState, NativeError, MessageText, BufferLength,
          TextLength);

    if (!obj) return SQL_INVALID_HANDLE;

    if (obj->unix_handle)
    {
        ret = get_diag_rec_unix_w( HandleType, obj, RecNumber, SqlState, NativeError, MessageText, BufferLength,
                                   TextLength );
    }
    else if (obj->win32_handle)
    {
        ret = get_diag_rec_win32_w( HandleType, obj, RecNumber, SqlState, NativeError, MessageText, BufferLength,
                                    TextLength );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( obj );
    return ret;
}

static SQLRETURN get_stmt_attr_unix_w( struct statement *stmt, SQLINTEGER attr, SQLPOINTER value, SQLINTEGER buflen,
                                       SQLINTEGER *retlen )
{
    struct SQLGetStmtAttrW_params params = { stmt->hdr.unix_handle, attr, value, buflen, retlen };
    return ODBC_CALL( SQLGetStmtAttrW, &params );
}

static SQLRETURN get_stmt_attr_win32_w( struct statement *stmt, SQLINTEGER attr, SQLPOINTER value, SQLINTEGER buflen,
                                        SQLINTEGER *retlen )
{
    if (stmt->hdr.win32_funcs->SQLGetStmtAttrW)
        return stmt->hdr.win32_funcs->SQLGetStmtAttrW( stmt->hdr.win32_handle, attr, value, buflen, retlen );
    if (stmt->hdr.win32_funcs->SQLGetStmtAttr) FIXME( "Unicode to ANSI conversion not handled\n" );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLGetStmtAttrW          [ODBC32.138]
 */
SQLRETURN WINAPI SQLGetStmtAttrW(SQLHSTMT StatementHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                 SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, Attribute %d, Value %p, BufferLength %d, StringLength %p)\n", StatementHandle,
          Attribute, Value, BufferLength, StringLength);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = get_stmt_attr_unix_w( stmt, Attribute, Value, BufferLength, StringLength );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = get_stmt_attr_win32_w( stmt, Attribute, Value, BufferLength, StringLength );
    }

    if (!ret)
    {
        switch (Attribute)
        {
        case SQL_ATTR_APP_ROW_DESC:
        case SQL_ATTR_APP_PARAM_DESC:
        case SQL_ATTR_IMP_ROW_DESC:
        case SQL_ATTR_IMP_PARAM_DESC:
        {
            struct descriptor *desc = stmt->desc[Attribute - SQL_ATTR_APP_ROW_DESC];
            if (stmt->hdr.unix_handle)
            {
                if (sizeof(desc->hdr.unix_handle) > sizeof(SQLHDESC))
                    ERR( "truncating descriptor handle, consider using a Windows driver\n" );
                desc->hdr.unix_handle = (ULONG_PTR)*(SQLHDESC *)Value;
            }
            else if (stmt->hdr.win32_handle)
            {
                desc->hdr.win32_handle = *(SQLHDESC *)Value;
                desc->hdr.win32_funcs = stmt->hdr.win32_funcs;
            }
            *(struct descriptor **)Value = desc;
            break;
        }
        default: break;
        }
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
    return ret;
}

static SQLRETURN set_connect_attr_unix_w( struct connection *con, SQLINTEGER attr, SQLPOINTER value, SQLINTEGER len )
{
    struct SQLSetConnectAttrW_params params = { con->hdr.unix_handle, attr, value, len };
    return ODBC_CALL( SQLSetConnectAttrW, &params );
}

static SQLRETURN set_connect_attr_win32_w( struct connection *con, SQLINTEGER attr, SQLPOINTER value, SQLINTEGER len )
{
    if (con->hdr.win32_funcs->SQLSetConnectAttrW)
        return con->hdr.win32_funcs->SQLSetConnectAttrW( con->hdr.win32_handle, attr, value, len );
    if (con->hdr.win32_funcs->SQLSetConnectAttr) FIXME( "Unicode to ANSI conversion not handled\n" );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLSetConnectAttrW          [ODBC32.139]
 */
SQLRETURN WINAPI SQLSetConnectAttrW(SQLHDBC ConnectionHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                    SQLINTEGER StringLength)
{
    struct connection *con = (struct connection *)lock_object( ConnectionHandle, SQL_HANDLE_DBC );
    SQLRETURN ret = SQL_SUCCESS;

    TRACE("(ConnectionHandle %p, Attribute %d, Value %p, StringLength %d)\n", ConnectionHandle, Attribute,
          Value, StringLength);

    if (!con) return SQL_INVALID_HANDLE;

    if (con->hdr.unix_handle)
    {
        ret = set_connect_attr_unix_w( con, Attribute, Value, StringLength );
    }
    else if (con->hdr.win32_handle)
    {
        ret = set_connect_attr_win32_w( con, Attribute, Value, StringLength );
    }
    else
    {
        switch (Attribute)
        {
        case SQL_ATTR_CONNECTION_TIMEOUT:
            con->attr_con_timeout = (UINT32)(ULONG_PTR)Value;
            break;

        case SQL_ATTR_LOGIN_TIMEOUT:
            con->attr_login_timeout = (UINT32)(ULONG_PTR)Value;
            break;

        default:
            FIXME( "unhandled attribute %d\n", Attribute );
            ret = SQL_ERROR;
            break;
        }
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &con->hdr );
    return ret;
}

static SQLRETURN columns_unix_w( struct statement *stmt, SQLWCHAR *catalog, SQLSMALLINT len1, SQLWCHAR *schema,
                                 SQLSMALLINT len2, SQLWCHAR *table, SQLSMALLINT len3, SQLWCHAR *column,
                                 SQLSMALLINT len4 )
{
    struct SQLColumnsW_params params = { stmt->hdr.unix_handle, catalog, len1, schema, len2, table, len3, column,
                                         len4 };
    return ODBC_CALL( SQLColumnsW, &params );
}

static SQLRETURN columns_win32_w( struct statement *stmt, SQLWCHAR *catalog, SQLSMALLINT len1, SQLWCHAR *schema,
                                  SQLSMALLINT len2, SQLWCHAR *table, SQLSMALLINT len3, SQLWCHAR *column,
                                  SQLSMALLINT len4 )
{
    if (stmt->hdr.win32_funcs->SQLColumnsW)
        return stmt->hdr.win32_funcs->SQLColumnsW( stmt->hdr.win32_handle, catalog, len1, schema, len2, table, len3,
                                                   column, len4 );
    if (stmt->hdr.win32_funcs->SQLColumns) FIXME( "Unicode to ANSI conversion not handled\n" );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLColumnsW          [ODBC32.140]
 */
SQLRETURN WINAPI SQLColumnsW(SQLHSTMT StatementHandle, SQLWCHAR *CatalogName, SQLSMALLINT NameLength1,
                             SQLWCHAR *SchemaName, SQLSMALLINT NameLength2, SQLWCHAR *TableName,
                             SQLSMALLINT NameLength3, SQLWCHAR *ColumnName, SQLSMALLINT NameLength4)
{
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d, TableName %s,"
          " NameLength3 %d, ColumnName %s, NameLength4 %d)\n", StatementHandle,
          debugstr_sqlwstr(CatalogName, NameLength1), NameLength1, debugstr_sqlwstr(SchemaName, NameLength2),
          NameLength2, debugstr_sqlwstr(TableName, NameLength3), NameLength3, debugstr_sqlwstr(ColumnName, NameLength4),
          NameLength4);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = columns_unix_w( stmt, CatalogName, NameLength1, SchemaName, NameLength2, TableName, NameLength3,
                              ColumnName, NameLength4 );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = columns_win32_w( stmt, CatalogName, NameLength1, SchemaName, NameLength2, TableName, NameLength3,
                               ColumnName, NameLength4 );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
    return ret;
}

static SQLRETURN driver_connect_win32_w( struct connection *con, SQLHWND window, SQLWCHAR *in_conn_str,
                                         SQLSMALLINT len1, SQLWCHAR *out_conn_str, SQLSMALLINT buflen, SQLSMALLINT *len2,
                                         SQLUSMALLINT completion )
{
    SQLRETURN ret = SQL_ERROR;

    if (con->hdr.win32_funcs->SQLDriverConnectW)
        return con->hdr.win32_funcs->SQLDriverConnectW( con->hdr.win32_handle, window, in_conn_str, len1, out_conn_str,
                                                        buflen, len2, completion );
    if (con->hdr.win32_funcs->SQLDriverConnect)
    {
        SQLCHAR *in_conn_str_a, *out_conn_str_a;
        SQLSMALLINT out_len;

        if (!(out_conn_str_a = malloc( buflen * sizeof(*out_conn_str_a) ))) return SQL_ERROR;
        in_conn_str_a = strnWtoA( in_conn_str, len1 );

        ret = con->hdr.win32_funcs->SQLDriverConnect( con->hdr.win32_handle, window, in_conn_str_a, SQL_NTS, out_conn_str_a,
                buflen, &out_len, completion );
        if (SUCCESS( ret ))
        {
            int len = MultiByteToWideChar( CP_ACP, 0, (const char *)out_conn_str_a, -1, out_conn_str, buflen );
            if (len2) *len2 = len - 1;
        }

        free(in_conn_str_a);
        free(out_conn_str_a);
    }

    return ret;
}

static SQLRETURN driver_connect_unix_w( struct connection *con, SQLHWND window, SQLWCHAR *in_conn_str, SQLSMALLINT len,
                                        SQLWCHAR *out_conn_str, SQLSMALLINT buflen, SQLSMALLINT *len2,
                                        SQLUSMALLINT completion )
{
    struct SQLDriverConnectW_params params = { con->hdr.unix_handle, window, in_conn_str, len, out_conn_str, buflen,
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
    struct connection *con = (struct connection *)lock_object( ConnectionHandle, SQL_HANDLE_DBC );
    const WCHAR *datasource, *drivername = NULL;
    WCHAR *filename = NULL, *connect_string = NULL;
    struct attribute_list attrs;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(ConnectionHandle %p, WindowHandle %p, InConnectionString %s, Length %d, OutConnectionString %p,"
          " BufferLength %d, Length2 %p, DriverCompletion %d)\n", ConnectionHandle, WindowHandle,
          debugstr_sqlwstr(InConnectionString, Length), Length, OutConnectionString, BufferLength, Length2,
          DriverCompletion);

    if (!con) return SQL_INVALID_HANDLE;

    if (parse_connect_string( &attrs, InConnectionString ) || !(connect_string = build_connect_string( &attrs )))
        goto done;

    if (!(datasource = get_attribute( &attrs, L"DSN" )) && !(drivername = get_attribute( &attrs, L"DRIVER" )))
    {
        WARN( "can't find data source or driver name\n" );
        goto done;
    }
    if ((datasource && !(filename = get_driver_filename_from_source( datasource ))) ||
        (drivername && !(filename = get_driver_filename_from_name( drivername ))))
    {
        WARN( "can't find driver filename\n" );
        goto done;
    }

    if (has_suffix( filename, L".dll" ))
    {
        if (!(con->hdr.win32_funcs = con->hdr.parent->win32_funcs = load_driver( filename )))
        {
            WARN( "failed to load driver %s\n", debugstr_w(filename) );
            goto done;
        }
        TRACE( "using Windows driver %s\n", debugstr_w(filename) );

        if (!SUCCESS((ret = create_env( (struct environment *)con->hdr.parent, FALSE )))) goto done;
        if (!SUCCESS((ret = create_con( con )))) goto done;

        ret = driver_connect_win32_w( con, WindowHandle, InConnectionString, Length, OutConnectionString,
                                      BufferLength, Length2, DriverCompletion );
    }
    else
    {
        TRACE( "using Unix driver %s\n", debugstr_w(filename) );

        if (!SUCCESS((ret = create_env( (struct environment *)con->hdr.parent, TRUE )))) goto done;
        if (!SUCCESS((ret = create_con( con )))) goto done;

        ret = driver_connect_unix_w( con, WindowHandle, InConnectionString, Length, OutConnectionString,
                                     BufferLength, Length2, DriverCompletion );
    }

done:
    free( connect_string );
    free_attribute_list( &attrs );
    free( filename );

    TRACE("Returning %d\n", ret);
    unlock_object( &con->hdr );
    return ret;
}

static SQLRETURN get_connect_option_unix_w( struct connection *con, SQLUSMALLINT option, SQLPOINTER value )
{
    struct SQLGetConnectOptionW_params params = { con->hdr.unix_handle, option, value };
    return ODBC_CALL( SQLGetConnectOptionW, &params );
}

static SQLRETURN get_connect_option_win32_w( struct connection *con, SQLUSMALLINT option, SQLPOINTER value )
{
    if (con->hdr.win32_funcs->SQLGetConnectOptionW)
        return con->hdr.win32_funcs->SQLGetConnectOptionW( con->hdr.win32_handle, option, value );
    if (con->hdr.win32_funcs->SQLGetConnectOption) FIXME( "Unicode to ANSI conversion not handled\n" );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLGetConnectOptionW      [ODBC32.142]
 */
SQLRETURN WINAPI SQLGetConnectOptionW(SQLHDBC ConnectionHandle, SQLUSMALLINT Option, SQLPOINTER Value)
{
    struct connection *con = (struct connection *)lock_object( ConnectionHandle, SQL_HANDLE_DBC );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(ConnectionHandle %p, Option %d, Value %p)\n", ConnectionHandle, Option, Value);

    if (!con) return SQL_INVALID_HANDLE;

    if (con->hdr.unix_handle)
    {
        ret = get_connect_option_unix_w( con, Option, Value );
    }
    else if (con->hdr.win32_handle)
    {
        ret = get_connect_option_win32_w( con, Option, Value );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &con->hdr );
    return ret;
}

static SQLRETURN get_info_unix_w( struct connection *con, SQLUSMALLINT type, SQLPOINTER value, SQLSMALLINT buflen,
                                  SQLSMALLINT *retlen )
{
    struct SQLGetInfoW_params params = { con->hdr.unix_handle, type, value, buflen, retlen };
    return ODBC_CALL( SQLGetInfoW, &params );
}

static SQLRETURN get_info_win32_w( struct connection *con, SQLUSMALLINT type, SQLPOINTER value, SQLSMALLINT buflen,
                                   SQLSMALLINT *retlen )
{
    SQLRETURN ret = SQL_ERROR;

    if (con->hdr.win32_funcs->SQLGetInfoW)
        return con->hdr.win32_funcs->SQLGetInfoW( con->hdr.win32_handle, type, value, buflen, retlen );

    if (con->hdr.win32_funcs->SQLGetInfo)
    {
        switch (type)
        {
        case SQL_ACTIVE_CONNECTIONS:
        case SQL_ACTIVE_STATEMENTS:
        case SQL_ODBC_API_CONFORMANCE:
        case SQL_TXN_CAPABLE:
            ret = con->hdr.win32_funcs->SQLGetInfo( con->hdr.win32_handle, type, value, buflen, retlen );
            break;
        case SQL_DRIVER_NAME:
        case SQL_DBMS_NAME:
        case SQL_DATA_SOURCE_READ_ONLY:
        {
            SQLSMALLINT lenA;
            SQLCHAR *strA;

            /* For string types sizes are in bytes. */

            buflen /= sizeof(WCHAR);
            if (!(strA = malloc(buflen))) return SQL_ERROR;

            ret = con->hdr.win32_funcs->SQLGetInfo( con->hdr.win32_handle, type, strA, buflen, &lenA );
            if (SUCCESS( ret ))
            {
                int len = MultiByteToWideChar( CP_ACP, 0, (const char *)strA, -1, (WCHAR *)value, buflen );
                if (retlen) *retlen = (len - 1) * sizeof(WCHAR);
            }
            free( strA );

            break;
        }
        default:
            FIXME( "Unicode to ANSI conversion not handled, for info type %u.\n", type );
        }
    }

    return ret;
}

/*************************************************************************
 *				SQLGetInfoW          [ODBC32.145]
 */
SQLRETURN WINAPI SQLGetInfoW(SQLHDBC ConnectionHandle, SQLUSMALLINT InfoType, SQLPOINTER InfoValue,
                             SQLSMALLINT BufferLength, SQLSMALLINT *StringLength)
{
    struct connection *con = (struct connection *)lock_object( ConnectionHandle, SQL_HANDLE_DBC );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(ConnectionHandle, %p, InfoType %d, InfoValue %p, BufferLength %d, StringLength %p)\n", ConnectionHandle,
          InfoType, InfoValue, BufferLength, StringLength);

    if (!con) return SQL_INVALID_HANDLE;

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
        ret = SQL_SUCCESS;
        goto done;
    }
    default: break;
    }

    if (con->hdr.unix_handle)
    {
        ret = get_info_unix_w( con, InfoType, InfoValue, BufferLength, StringLength );
    }
    else if (con->hdr.win32_handle)
    {
        ret = get_info_win32_w( con, InfoType, InfoValue, BufferLength, StringLength );
    }

done:
    TRACE("Returning %d\n", ret);
    unlock_object( &con->hdr );
    return ret;
}

static SQLRETURN get_type_info_unix_w( struct statement *stmt, SQLSMALLINT type )
{
    struct SQLGetTypeInfoW_params params = { stmt->hdr.unix_handle, type };
    return ODBC_CALL( SQLGetTypeInfoW, &params );
}

static SQLRETURN get_type_info_win32_w( struct statement *stmt, SQLSMALLINT type )
{
    if (stmt->hdr.win32_funcs->SQLGetTypeInfoW)
        return stmt->hdr.win32_funcs->SQLGetTypeInfoW( stmt->hdr.win32_handle, type );
    if (stmt->hdr.win32_funcs->SQLGetTypeInfo)
        return stmt->hdr.win32_funcs->SQLGetTypeInfo( stmt->hdr.win32_handle, type );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLGetTypeInfoW          [ODBC32.147]
 */
SQLRETURN WINAPI SQLGetTypeInfoW(SQLHSTMT StatementHandle, SQLSMALLINT DataType)
{
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, DataType %d)\n", StatementHandle, DataType);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = get_type_info_unix_w( stmt, DataType );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = get_type_info_win32_w( stmt, DataType );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
    return ret;
}

static SQLRETURN set_connect_option_unix_w( struct connection *con, SQLUSMALLINT option, SQLULEN value )
{
    struct SQLSetConnectOptionW_params params = { con->hdr.unix_handle, option, value };
    return ODBC_CALL( SQLSetConnectOptionW, &params );
}

static SQLRETURN set_connect_option_win32_w( struct connection *con, SQLUSMALLINT option, SQLULEN value )
{
    if (con->hdr.win32_funcs->SQLSetConnectOptionW)
        return con->hdr.win32_funcs->SQLSetConnectOptionW( con->hdr.win32_handle, option, value );
    if (con->hdr.win32_funcs->SQLSetConnectOption) FIXME( "Unicode to ANSI conversion not handled\n" );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLSetConnectOptionW          [ODBC32.150]
 */
SQLRETURN WINAPI SQLSetConnectOptionW(SQLHDBC ConnectionHandle, SQLUSMALLINT Option, SQLULEN Value)
{
    struct connection *con = (struct connection *)lock_object( ConnectionHandle, SQL_HANDLE_DBC );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(ConnectionHandle %p, Option %d, Value %s)\n", ConnectionHandle, Option, debugstr_sqllen(Value));

    if (!con) return SQL_INVALID_HANDLE;

    if (con->hdr.unix_handle)
    {
        ret = set_connect_option_unix_w( con, Option, Value );
    }
    else if (con->hdr.win32_handle)
    {
        ret = set_connect_option_win32_w( con, Option, Value );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &con->hdr );
    return ret;
}

static SQLRETURN special_columns_unix_w( struct statement *stmt, SQLUSMALLINT id, SQLWCHAR *catalog, SQLSMALLINT len1,
                                         SQLWCHAR *schema, SQLSMALLINT len2, SQLWCHAR *table, SQLSMALLINT len3,
                                         SQLUSMALLINT scope, SQLUSMALLINT nullable )
{
    struct SQLSpecialColumnsW_params params = { stmt->hdr.unix_handle, id, catalog, len1, schema, len2, table, len3,
                                                scope, nullable };
    return ODBC_CALL( SQLSpecialColumnsW, &params );
}

static SQLRETURN special_columns_win32_w( struct statement *stmt, SQLUSMALLINT id, SQLWCHAR *catalog, SQLSMALLINT len1,
                                          SQLWCHAR *schema, SQLSMALLINT len2, SQLWCHAR *table, SQLSMALLINT len3,
                                          SQLUSMALLINT scope, SQLUSMALLINT nullable )
{
    if (stmt->hdr.win32_funcs->SQLSpecialColumnsW)
        return stmt->hdr.win32_funcs->SQLSpecialColumnsW( stmt->hdr.win32_handle, id, catalog, len1, schema, len2,
                                                          table, len3, scope, nullable );
    if (stmt->hdr.win32_funcs->SQLSpecialColumns) FIXME( "Unicode to ANSI conversion not handled\n" );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLSpecialColumnsW          [ODBC32.152]
 */
SQLRETURN WINAPI SQLSpecialColumnsW(SQLHSTMT StatementHandle, SQLUSMALLINT IdentifierType,
                                    SQLWCHAR *CatalogName, SQLSMALLINT NameLength1, SQLWCHAR *SchemaName,
                                    SQLSMALLINT NameLength2, SQLWCHAR *TableName, SQLSMALLINT NameLength3,
                                    SQLUSMALLINT Scope, SQLUSMALLINT Nullable)
{
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, IdentifierType %d, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d,"
          " TableName %s, NameLength3 %d, Scope %d, Nullable %d)\n", StatementHandle, IdentifierType,
          debugstr_sqlwstr(CatalogName, NameLength1), NameLength1, debugstr_sqlwstr(SchemaName, NameLength2),
          NameLength2, debugstr_sqlwstr(TableName, NameLength3), NameLength3, Scope, Nullable);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = special_columns_unix_w( stmt, IdentifierType, CatalogName, NameLength1, SchemaName, NameLength2,
                                      TableName, NameLength3, Scope, Nullable );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = special_columns_win32_w( stmt, IdentifierType, CatalogName, NameLength1, SchemaName, NameLength2,
                                       TableName, NameLength3, Scope, Nullable );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
    return ret;
}

static SQLRETURN statistics_unix_w( struct statement *stmt, SQLWCHAR *catalog, SQLSMALLINT len1, SQLWCHAR *schema,
                                    SQLSMALLINT len2, SQLWCHAR *table, SQLSMALLINT len3, SQLUSMALLINT unique,
                                    SQLUSMALLINT reserved )
{
    struct SQLStatisticsW_params params = { stmt->hdr.unix_handle, catalog, len1, schema, len2, table, len3, unique,
                                            reserved };
    return ODBC_CALL( SQLStatisticsW, &params );
}

static SQLRETURN statistics_win32_w( struct statement *stmt, SQLWCHAR *catalog, SQLSMALLINT len1, SQLWCHAR *schema,
                                     SQLSMALLINT len2, SQLWCHAR *table, SQLSMALLINT len3, SQLUSMALLINT unique,
                                     SQLUSMALLINT reserved )
{
    if (stmt->hdr.win32_funcs->SQLStatisticsW)
        return stmt->hdr.win32_funcs->SQLStatisticsW( stmt->hdr.win32_handle, catalog, len1, schema, len2, table,
                                                      len3, unique, reserved );
    if (stmt->hdr.win32_funcs->SQLStatistics) FIXME( "Unicode to ANSI conversion not handled\n" );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLStatisticsW          [ODBC32.153]
 */
SQLRETURN WINAPI SQLStatisticsW(SQLHSTMT StatementHandle, SQLWCHAR *CatalogName, SQLSMALLINT NameLength1,
                                SQLWCHAR *SchemaName, SQLSMALLINT NameLength2, SQLWCHAR *TableName,
                                SQLSMALLINT NameLength3, SQLUSMALLINT Unique, SQLUSMALLINT Reserved)
{
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d SchemaName %s, NameLength2 %d, TableName %s"
          " NameLength3 %d, Unique %d, Reserved %d)\n", StatementHandle,
          debugstr_sqlwstr(CatalogName, NameLength1), NameLength1, debugstr_sqlwstr(SchemaName, NameLength2),
          NameLength2, debugstr_sqlwstr(TableName, NameLength3), NameLength3, Unique, Reserved);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = statistics_unix_w( stmt, CatalogName, NameLength1, SchemaName, NameLength2, TableName, NameLength3,
                                 Unique, Reserved );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = statistics_win32_w( stmt, CatalogName, NameLength1, SchemaName, NameLength2, TableName, NameLength3,
                                  Unique, Reserved );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
    return ret;
}

static SQLRETURN tables_unix_w( struct statement *stmt, SQLWCHAR *catalog, SQLSMALLINT len1, SQLWCHAR *schema,
                                SQLSMALLINT len2, SQLWCHAR *table, SQLSMALLINT len3, SQLWCHAR *type,
                                SQLSMALLINT len4 )
{
    struct SQLTablesW_params params = { stmt->hdr.unix_handle, catalog, len1, schema, len2, table, len3, type, len4 };
    return ODBC_CALL( SQLTablesW, &params );
}

static SQLRETURN tables_win32_w( struct statement *stmt, SQLWCHAR *catalog, SQLSMALLINT len1, SQLWCHAR *schema,
                                 SQLSMALLINT len2, SQLWCHAR *table, SQLSMALLINT len3, SQLWCHAR *type,
                                 SQLSMALLINT len4 )
{
    if (stmt->hdr.win32_funcs->SQLTablesW)
        return stmt->hdr.win32_funcs->SQLTablesW( stmt->hdr.win32_handle, catalog, len1, schema, len2, table, len3,
                                                  type, len4 );
    if (stmt->hdr.win32_funcs->SQLTables) FIXME( "Unicode to ANSI conversion not handled\n" );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLTablesW          [ODBC32.154]
 */
SQLRETURN WINAPI SQLTablesW(SQLHSTMT StatementHandle, SQLWCHAR *CatalogName, SQLSMALLINT NameLength1,
                            SQLWCHAR *SchemaName, SQLSMALLINT NameLength2, SQLWCHAR *TableName,
                            SQLSMALLINT NameLength3, SQLWCHAR *TableType, SQLSMALLINT NameLength4)
{
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d, TableName %s,"
          " NameLength3 %d, TableType %s, NameLength4 %d)\n", StatementHandle,
          debugstr_sqlwstr(CatalogName, NameLength1), NameLength1, debugstr_sqlwstr(SchemaName, NameLength2),
          NameLength2, debugstr_sqlwstr(TableName, NameLength3), NameLength3, debugstr_sqlwstr(TableType, NameLength4),
          NameLength4);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = tables_unix_w( stmt, CatalogName, NameLength1, SchemaName, NameLength2, TableName, NameLength3,
                             TableType, NameLength4 );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = tables_win32_w( stmt, CatalogName, NameLength1, SchemaName, NameLength2, TableName, NameLength3,
                              TableType, NameLength4 );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
    return ret;
}

static SQLRETURN browse_connect_win32_w( struct connection *con, SQLWCHAR *in_conn_str, SQLSMALLINT len,
                                         SQLWCHAR *out_conn_str, SQLSMALLINT buflen, SQLSMALLINT *len2 )
{
    if (con->hdr.win32_funcs->SQLBrowseConnectW)
        return con->hdr.win32_funcs->SQLBrowseConnectW( con->hdr.win32_handle, in_conn_str, len, out_conn_str,
                                                        buflen, len2 );
    if (con->hdr.win32_funcs->SQLBrowseConnect) FIXME( "Unicode to ANSI conversion not handled\n" );
    return SQL_ERROR;
}

static SQLRETURN browse_connect_unix_w( struct connection *con, SQLWCHAR *in_conn_str, SQLSMALLINT len,
                                        SQLWCHAR *out_conn_str, SQLSMALLINT buflen, SQLSMALLINT *len2 )
{
    struct SQLBrowseConnectW_params params = { con->hdr.unix_handle, in_conn_str, len, out_conn_str, buflen, len2 };
    return ODBC_CALL( SQLBrowseConnectW, &params );
}

/*************************************************************************
 *				SQLBrowseConnectW          [ODBC32.155]
 */
SQLRETURN WINAPI SQLBrowseConnectW(SQLHDBC ConnectionHandle, SQLWCHAR *InConnectionString, SQLSMALLINT StringLength1,
                                   SQLWCHAR *OutConnectionString, SQLSMALLINT BufferLength, SQLSMALLINT *StringLength2)
{
    struct connection *con = (struct connection *)lock_object( ConnectionHandle, SQL_HANDLE_DBC );
    const WCHAR *datasource, *drivername = NULL;
    WCHAR *filename = NULL, *connect_string = NULL;
    struct attribute_list attrs;
    SQLRETURN ret = SQL_ERROR;

    TRACE("(ConnectionHandle %p, InConnectionString %s, StringLength1 %d, OutConnectionString %p, BufferLength %d, "
          "StringLength2 %p)\n", ConnectionHandle, debugstr_sqlwstr(InConnectionString, StringLength1), StringLength1,
          OutConnectionString, BufferLength, StringLength2);

    if (!con) return SQL_INVALID_HANDLE;

    if (parse_connect_string( &attrs, InConnectionString ) || !(connect_string = build_connect_string( &attrs )))
        goto done;

    if (!(datasource = get_attribute( &attrs, L"DSN" )) && !(drivername = get_attribute( &attrs, L"DRIVER" )))
    {
        WARN( "can't find data source or driver name\n" );
        goto done;
    }
    if ((datasource && !(filename = get_driver_filename_from_source( datasource ))) ||
        (drivername && !(filename = get_driver_filename_from_name( drivername ))))
    {
        WARN( "can't find driver filename\n" );
        goto done;
    }

    if (has_suffix( filename, L".dll" ))
    {
        if (!(con->hdr.win32_funcs = con->hdr.parent->win32_funcs = load_driver( filename )))
        {
            WARN( "failed to load driver %s\n", debugstr_w(filename) );
            goto done;
        }
        TRACE( "using Windows driver %s\n", debugstr_w(filename) );

        if (!SUCCESS((ret = create_env( (struct environment *)con->hdr.parent, FALSE )))) goto done;
        if (!SUCCESS((ret = create_con( con )))) goto done;

        ret = browse_connect_win32_w( con, connect_string, StringLength1, OutConnectionString, BufferLength,
                                      StringLength2 );
    }
    else
    {
        TRACE( "using Unix driver %s\n", debugstr_w(filename) );

        if (!SUCCESS((ret = create_env( (struct environment *)con->hdr.parent, TRUE )))) goto done;
        if (!SUCCESS((ret = create_con( con )))) goto done;

        ret = browse_connect_unix_w( con, connect_string, StringLength1, OutConnectionString, BufferLength,
                                     StringLength2 );
    }

done:
    free( connect_string );
    free_attribute_list( &attrs );
    free( filename );

    TRACE("Returning %d\n", ret);
    unlock_object( &con->hdr );
    return ret;
}

static SQLRETURN column_privs_unix_w( struct statement *stmt, SQLWCHAR *catalog, SQLSMALLINT len1, SQLWCHAR *schema,
                                      SQLSMALLINT len2, SQLWCHAR *table, SQLSMALLINT len3, SQLWCHAR *column,
                                      SQLSMALLINT len4 )
{
    struct SQLColumnPrivilegesW_params params = { stmt->hdr.unix_handle, catalog, len1, schema, len2, table, len3,
                                                  column, len4 };
    return ODBC_CALL( SQLColumnPrivilegesW, &params );
}

static SQLRETURN column_privs_win32_w( struct statement *stmt, SQLWCHAR *catalog, SQLSMALLINT len1, SQLWCHAR *schema,
                                       SQLSMALLINT len2, SQLWCHAR *table, SQLSMALLINT len3, SQLWCHAR *column,
                                       SQLSMALLINT len4 )
{
    if (stmt->hdr.win32_funcs->SQLColumnPrivilegesW)
        return stmt->hdr.win32_funcs->SQLColumnPrivilegesW( stmt->hdr.win32_handle, catalog, len1, schema, len2,
                                                            table, len3, column, len4 );
    if (stmt->hdr.win32_funcs->SQLColumnPrivileges) FIXME( "Unicode to ANSI conversion not handled\n" );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLColumnPrivilegesW          [ODBC32.156]
 */
SQLRETURN WINAPI SQLColumnPrivilegesW(SQLHSTMT StatementHandle, SQLWCHAR *CatalogName, SQLSMALLINT NameLength1,
                                      SQLWCHAR *SchemaName, SQLSMALLINT NameLength2, SQLWCHAR *TableName,
                                      SQLSMALLINT NameLength3, SQLWCHAR *ColumnName, SQLSMALLINT NameLength4)
{
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d, TableName %s,"
          " NameLength3 %d, ColumnName %s, NameLength3 %d)\n", StatementHandle,
          debugstr_sqlwstr(CatalogName, NameLength1), NameLength1, debugstr_sqlwstr(SchemaName, NameLength2),
          NameLength2, debugstr_sqlwstr(TableName, NameLength3), NameLength3, debugstr_sqlwstr(ColumnName, NameLength4),
          NameLength4);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = column_privs_unix_w( stmt, CatalogName, NameLength1, SchemaName, NameLength2, TableName, NameLength3,
                                   ColumnName, NameLength4 );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = column_privs_win32_w( stmt, CatalogName, NameLength1, SchemaName, NameLength2, TableName, NameLength3,
                                    ColumnName, NameLength4 );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
    return ret;
}

/*************************************************************************
 *				SQLDataSourcesW          [ODBC32.157]
 */
SQLRETURN WINAPI SQLDataSourcesW(SQLHENV EnvironmentHandle, SQLUSMALLINT Direction, SQLWCHAR *ServerName,
                                 SQLSMALLINT BufferLength1, SQLSMALLINT *NameLength1, SQLWCHAR *Description,
                                 SQLSMALLINT BufferLength2, SQLSMALLINT *NameLength2)
{
    struct environment *env = (struct environment *)lock_object( EnvironmentHandle, SQL_HANDLE_ENV );
    SQLRETURN ret = SQL_ERROR;
    DWORD len_source = BufferLength1, len_desc = BufferLength2;
    LONG res;

    TRACE("(EnvironmentHandle %p, Direction %d, ServerName %p, BufferLength1 %d, NameLength1 %p, Description %p,"
          " BufferLength2 %d, NameLength2 %p)\n", EnvironmentHandle, Direction, ServerName, BufferLength1,
          NameLength1, Description, BufferLength2, NameLength2);

    if (!env) return SQL_INVALID_HANDLE;

    if (Direction == SQL_FETCH_FIRST || (Direction == SQL_FETCH_NEXT && !env->sources_key))
    {
        env->sources_idx = 0;
        env->sources_system = FALSE;
        RegCloseKey( env->sources_key );
        if (!(env->sources_key = open_sources_key( HKEY_CURRENT_USER ))) goto done;
    }

    res = RegEnumValueW( env->sources_key, env->sources_idx, ServerName, &len_source, NULL, NULL,
                         (BYTE *)Description, &len_desc );
    if (res == ERROR_NO_MORE_ITEMS)
    {
        if (env->sources_system)
        {
            ret = SQL_NO_DATA;
            goto done;
        }
        /* user key exhausted, continue with system key */
        RegCloseKey( env->sources_key );
        if (!(env->sources_key = open_sources_key( HKEY_LOCAL_MACHINE ))) goto done;
        env->sources_idx = 0;
        env->sources_system = TRUE;
        res = RegEnumValueW( env->sources_key, env->sources_idx, ServerName, &len_source, NULL, NULL,
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

        env->sources_idx++;
        ret = SQL_SUCCESS;
    }

done:
    if (ret)
    {
        RegCloseKey( env->sources_key );
        env->sources_key = NULL;
        env->sources_idx = 0;
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &env->hdr );
    return ret;
}

static SQLRETURN foreign_keys_unix_w( struct statement *stmt, SQLWCHAR *pk_catalog, SQLSMALLINT len1,
                                      SQLWCHAR *pk_schema, SQLSMALLINT len2, SQLWCHAR *pk_table, SQLSMALLINT len3,
                                      SQLWCHAR *fk_catalog, SQLSMALLINT len4, SQLWCHAR *fk_schema, SQLSMALLINT len5,
                                      SQLWCHAR *fk_table, SQLSMALLINT len6 )
{
    struct SQLForeignKeysW_params params = { stmt->hdr.unix_handle, pk_catalog, len1, pk_schema, len2, pk_table, len3,
                                             fk_catalog, len4, fk_schema, len5, fk_table, len6 };
    return ODBC_CALL( SQLForeignKeysW, &params );
}

static SQLRETURN foreign_keys_win32_w( struct statement *stmt, SQLWCHAR *pk_catalog, SQLSMALLINT len1,
                                       SQLWCHAR *pk_schema, SQLSMALLINT len2, SQLWCHAR *pk_table, SQLSMALLINT len3,
                                       SQLWCHAR *fk_catalog, SQLSMALLINT len4, SQLWCHAR *fk_schema, SQLSMALLINT len5,
                                       SQLWCHAR *fk_table, SQLSMALLINT len6 )
{
    if (stmt->hdr.win32_funcs->SQLForeignKeysW)
        return stmt->hdr.win32_funcs->SQLForeignKeysW( stmt->hdr.win32_handle, pk_catalog, len1, pk_schema, len2,
                                                       pk_table, len3, fk_catalog, len4, fk_schema, len5, fk_table,
                                                       len6 );
    if (stmt->hdr.win32_funcs->SQLForeignKeys) FIXME( "Unicode to ANSI conversion not handled\n" );
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
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, PkCatalogName %s, NameLength1 %d, PkSchemaName %s, NameLength2 %d,"
          " PkTableName %s, NameLength3 %d, FkCatalogName %s, NameLength4 %d, FkSchemaName %s,"
          " NameLength5 %d, FkTableName %s, NameLength6 %d)\n", StatementHandle,
          debugstr_sqlwstr(PkCatalogName, NameLength1), NameLength1, debugstr_sqlwstr(PkSchemaName, NameLength2),
          NameLength2, debugstr_sqlwstr(PkTableName, NameLength3), NameLength3,
          debugstr_sqlwstr(FkCatalogName, NameLength4), NameLength4, debugstr_sqlwstr(FkSchemaName, NameLength5),
          NameLength5, debugstr_sqlwstr(FkTableName, NameLength6), NameLength6);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = foreign_keys_unix_w( stmt, PkCatalogName, NameLength1, PkSchemaName, NameLength2, PkTableName,
                                   NameLength2, FkCatalogName, NameLength3, FkSchemaName, NameLength5, FkTableName,
                                   NameLength6 );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = foreign_keys_win32_w( stmt, PkCatalogName, NameLength1, PkSchemaName, NameLength2, PkTableName,
                                    NameLength3, FkCatalogName, NameLength4, FkSchemaName, NameLength5, FkTableName,
                                    NameLength6 );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
    return ret;
}

static SQLRETURN native_sql_unix_w( struct connection *con, SQLWCHAR *in_statement, SQLINTEGER len,
                                    SQLWCHAR *out_statement, SQLINTEGER buflen, SQLINTEGER *retlen )
{
    struct SQLNativeSqlW_params params = { con->hdr.unix_handle, in_statement, len, out_statement, buflen, retlen };
    return ODBC_CALL( SQLNativeSqlW, &params );
}

static SQLRETURN native_sql_win32_w( struct connection *con, SQLWCHAR *in_statement, SQLINTEGER len,
                                     SQLWCHAR *out_statement, SQLINTEGER buflen, SQLINTEGER *retlen )
{
    if (con->hdr.win32_funcs->SQLNativeSqlW)
        return con->hdr.win32_funcs->SQLNativeSqlW( con->hdr.win32_handle, in_statement, len, out_statement, buflen,
                                                    retlen );
    if (con->hdr.win32_funcs->SQLNativeSql) FIXME( "Unicode to ANSI conversion not handled\n" );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLNativeSqlW          [ODBC32.162]
 */
SQLRETURN WINAPI SQLNativeSqlW(SQLHDBC ConnectionHandle, SQLWCHAR *InStatementText, SQLINTEGER TextLength1,
                               SQLWCHAR *OutStatementText, SQLINTEGER BufferLength, SQLINTEGER *TextLength2)
{
    struct connection *con = (struct connection *)lock_object( ConnectionHandle, SQL_HANDLE_DBC );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(ConnectionHandle %p, InStatementText %s, TextLength1 %d, OutStatementText %p, BufferLength %d, "
          "TextLength2 %p)\n", ConnectionHandle, debugstr_sqlwstr(InStatementText, TextLength1), TextLength1,
          OutStatementText, BufferLength, TextLength2);

    if (!con) return SQL_INVALID_HANDLE;

    if (con->hdr.unix_handle)
    {
        ret = native_sql_unix_w( con, InStatementText, TextLength1, OutStatementText, BufferLength, TextLength2 );
    }
    else if (con->hdr.win32_handle)
    {
        ret = native_sql_win32_w( con, InStatementText, TextLength1, OutStatementText, BufferLength, TextLength2 );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &con->hdr );
    return ret;
}

static SQLRETURN primary_keys_unix_w( struct statement *stmt, SQLWCHAR *catalog, SQLSMALLINT len1, SQLWCHAR *schema,
                                      SQLSMALLINT len2, SQLWCHAR *table, SQLSMALLINT len3 )
{
    struct SQLPrimaryKeysW_params params = { stmt->hdr.unix_handle, catalog, len1, schema, len2, table, len3 };
    return ODBC_CALL( SQLPrimaryKeysW, &params );
}

static SQLRETURN primary_keys_win32_w( struct statement *stmt, SQLWCHAR *catalog, SQLSMALLINT len1, SQLWCHAR *schema,
                                       SQLSMALLINT len2, SQLWCHAR *table, SQLSMALLINT len3 )
{
    if (stmt->hdr.win32_funcs->SQLPrimaryKeysW)
        return stmt->hdr.win32_funcs->SQLPrimaryKeysW( stmt->hdr.win32_handle, catalog, len1, schema, len2, table,
                                                       len3 );
    if (stmt->hdr.win32_funcs->SQLPrimaryKeys) FIXME( "Unicode to ANSI conversion not handled\n" );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLPrimaryKeysW          [ODBC32.165]
 */
SQLRETURN WINAPI SQLPrimaryKeysW(SQLHSTMT StatementHandle, SQLWCHAR *CatalogName, SQLSMALLINT NameLength1,
                                 SQLWCHAR *SchemaName, SQLSMALLINT NameLength2, SQLWCHAR *TableName,
                                 SQLSMALLINT NameLength3)
{
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d, TableName %s,"
          " NameLength3 %d)\n", StatementHandle, debugstr_sqlwstr(CatalogName, NameLength1), NameLength1,
          debugstr_sqlwstr(SchemaName, NameLength2), NameLength2, debugstr_sqlwstr(TableName, NameLength3),
          NameLength3);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = primary_keys_unix_w( stmt, CatalogName, NameLength1, SchemaName, NameLength2, TableName, NameLength2 );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = primary_keys_win32_w( stmt, CatalogName, NameLength1, SchemaName, NameLength2, TableName, NameLength2 );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
    return ret;
}

static SQLRETURN procedure_columns_unix_w( struct statement *stmt, SQLWCHAR *catalog, SQLSMALLINT len1,
                                           SQLWCHAR *schema, SQLSMALLINT len2, SQLWCHAR *proc, SQLSMALLINT len3,
                                           SQLWCHAR *column, SQLSMALLINT len4 )
{
    struct SQLProcedureColumnsW_params params = { stmt->hdr.unix_handle, catalog, len1, schema, len2, proc, len3,
                                                  column, len4 };
    return ODBC_CALL( SQLProcedureColumnsW, &params );
}

static SQLRETURN procedure_columns_win32_w( struct statement *stmt, SQLWCHAR *catalog, SQLSMALLINT len1,
                                            SQLWCHAR *schema, SQLSMALLINT len2, SQLWCHAR *proc, SQLSMALLINT len3,
                                            SQLWCHAR *column, SQLSMALLINT len4 )
{
    if (stmt->hdr.win32_funcs->SQLProcedureColumnsW)
        return stmt->hdr.win32_funcs->SQLProcedureColumnsW( stmt->hdr.win32_handle, catalog, len1, schema, len2,
                                                            proc, len3, column, len4 );
    if (stmt->hdr.win32_funcs->SQLProcedureColumns) FIXME( "Unicode to ANSI conversion not handled\n" );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLProcedureColumnsW          [ODBC32.166]
 */
SQLRETURN WINAPI SQLProcedureColumnsW(SQLHSTMT StatementHandle, SQLWCHAR *CatalogName, SQLSMALLINT NameLength1,
                                      SQLWCHAR *SchemaName, SQLSMALLINT NameLength2, SQLWCHAR *ProcName,
                                      SQLSMALLINT NameLength3, SQLWCHAR *ColumnName, SQLSMALLINT NameLength4 )
{
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d, ProcName %s,"
          " NameLength3 %d, ColumnName %s, NameLength4 %d)\n", StatementHandle,
          debugstr_sqlwstr(CatalogName, NameLength1), NameLength1, debugstr_sqlwstr(SchemaName, NameLength2),
          NameLength2, debugstr_sqlwstr(ProcName, NameLength3), NameLength3, debugstr_sqlwstr(ColumnName, NameLength4),
          NameLength4);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = procedure_columns_unix_w( stmt, CatalogName, NameLength1, SchemaName, NameLength2, ProcName,
                                        NameLength3, ColumnName, NameLength4 );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = procedure_columns_win32_w( stmt, CatalogName, NameLength1, SchemaName, NameLength2, ProcName,
                                         NameLength3, ColumnName, NameLength4 );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
    return ret;
}

static SQLRETURN procedures_unix_w( struct statement *stmt, SQLWCHAR *catalog, SQLSMALLINT len1, SQLWCHAR *schema,
                                    SQLSMALLINT len2, SQLWCHAR *proc, SQLSMALLINT len3 )
{
    struct SQLProceduresW_params params = { stmt->hdr.unix_handle, catalog, len1, schema, len2, proc, len3 };
    return ODBC_CALL( SQLProceduresW, &params );
}

static SQLRETURN procedures_win32_w( struct statement *stmt, SQLWCHAR *catalog, SQLSMALLINT len1, SQLWCHAR *schema,
                                     SQLSMALLINT len2, SQLWCHAR *proc, SQLSMALLINT len3 )
{
    if (stmt->hdr.win32_funcs->SQLProceduresW)
        return stmt->hdr.win32_funcs->SQLProceduresW( stmt->hdr.win32_handle, catalog, len1, schema, len2, proc,
                                                      len3 );
    if (stmt->hdr.win32_funcs->SQLProcedures) FIXME( "Unicode to ANSI conversion not handled\n" );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLProceduresW          [ODBC32.167]
 */
SQLRETURN WINAPI SQLProceduresW(SQLHSTMT StatementHandle, SQLWCHAR *CatalogName, SQLSMALLINT NameLength1,
                                SQLWCHAR *SchemaName, SQLSMALLINT NameLength2, SQLWCHAR *ProcName,
                                SQLSMALLINT NameLength3)
{
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d, ProcName %s,"
          " NameLength3 %d)\n", StatementHandle, debugstr_sqlwstr(CatalogName, NameLength1), NameLength1,
          debugstr_sqlwstr(SchemaName, NameLength2), NameLength2, debugstr_sqlwstr(ProcName, NameLength3),
          NameLength3);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = procedures_unix_w( stmt, CatalogName, NameLength1, SchemaName, NameLength2, ProcName, NameLength3 );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = procedures_win32_w( stmt, CatalogName, NameLength1, SchemaName, NameLength2, ProcName, NameLength3 );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
    return ret;
}

static SQLRETURN table_privileges_unix_w( struct statement *stmt, SQLWCHAR *catalog, SQLSMALLINT len1,
                                          SQLWCHAR *schema, SQLSMALLINT len2, SQLWCHAR *table, SQLSMALLINT len3 )
{
    struct SQLTablePrivilegesW_params params = { stmt->hdr.unix_handle, catalog, len1, schema, len2, table, len3 };
    return ODBC_CALL( SQLTablePrivilegesW, &params );
}

static SQLRETURN table_privileges_win32_w( struct statement *stmt, SQLWCHAR *catalog, SQLSMALLINT len1,
                                           SQLWCHAR *schema, SQLSMALLINT len2, SQLWCHAR *table, SQLSMALLINT len3 )
{
    if (stmt->hdr.win32_funcs->SQLTablePrivilegesW)
        return stmt->hdr.win32_funcs->SQLTablePrivilegesW( stmt->hdr.win32_handle, catalog, len1, schema, len2,
                                                           table, len3 );
    if (stmt->hdr.win32_funcs->SQLTablePrivileges) FIXME( "Unicode to ANSI conversion not handled\n" );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLTablePrivilegesW          [ODBC32.170]
 */
SQLRETURN WINAPI SQLTablePrivilegesW(SQLHSTMT StatementHandle, SQLWCHAR *CatalogName, SQLSMALLINT NameLength1,
                                     SQLWCHAR *SchemaName, SQLSMALLINT NameLength2, SQLWCHAR *TableName,
                                     SQLSMALLINT NameLength3)
{
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d, TableName %s,"
          " NameLength3 %d)\n", StatementHandle, debugstr_sqlwstr(CatalogName, NameLength1), NameLength1,
          debugstr_sqlwstr(SchemaName, NameLength2), NameLength2, debugstr_sqlwstr(TableName, NameLength3),
          NameLength3);

    if (!stmt) return SQL_INVALID_HANDLE;

    if (stmt->hdr.unix_handle)
    {
        ret = table_privileges_unix_w( stmt, CatalogName, NameLength1, SchemaName, NameLength2, TableName,
                                       NameLength3 );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = table_privileges_win32_w( stmt, CatalogName, NameLength1, SchemaName, NameLength2, TableName,
                                        NameLength3 );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
    return ret;
}

/*************************************************************************
 *				SQLDriversW          [ODBC32.171]
 */
SQLRETURN WINAPI SQLDriversW(SQLHENV EnvironmentHandle, SQLUSMALLINT Direction, SQLWCHAR *DriverDescription,
                             SQLSMALLINT BufferLength1, SQLSMALLINT *DescriptionLength, SQLWCHAR *DriverAttributes,
                             SQLSMALLINT BufferLength2, SQLSMALLINT *AttributesLength)
{
    struct environment *env = (struct environment *)lock_object( EnvironmentHandle, SQL_HANDLE_ENV );
    DWORD len_desc = BufferLength1;
    SQLRETURN ret = SQL_ERROR;
    LONG res;

    TRACE("(EnvironmentHandle %p, Direction %d, DriverDescription %p, BufferLength1 %d, DescriptionLength %p,"
          " DriverAttributes %p, BufferLength2 %d, AttributesLength %p)\n", EnvironmentHandle, Direction,
          DriverDescription, BufferLength1, DescriptionLength, DriverAttributes, BufferLength2, AttributesLength);

    if (!env) return SQL_INVALID_HANDLE;

    if (Direction == SQL_FETCH_FIRST || (Direction == SQL_FETCH_NEXT && !env->drivers_key))
    {
        env->drivers_idx = 0;
        RegCloseKey( env->drivers_key );
        if (!(env->drivers_key = open_drivers_key())) goto done;
    }

    res = RegEnumValueW( env->drivers_key, env->drivers_idx, DriverDescription, &len_desc,
                         NULL, NULL, NULL, NULL );
    if (res == ERROR_NO_MORE_ITEMS)
    {
        ret = SQL_NO_DATA;
        goto done;
    }
    else if (res == ERROR_SUCCESS)
    {
        if (DescriptionLength) *DescriptionLength = len_desc;

        env->drivers_idx++;
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
        RegCloseKey( env->drivers_key );
        env->drivers_key = NULL;
        env->drivers_idx = 0;
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &env->hdr );
    return ret;
}

static SQLRETURN set_desc_field_unix_w( struct descriptor *desc, SQLSMALLINT record, SQLSMALLINT id, SQLPOINTER value,
                                        SQLINTEGER len )
{
    struct SQLSetDescFieldW_params params = { desc->hdr.unix_handle, record, id, value, len };
    return ODBC_CALL( SQLSetDescFieldW, &params );
}

static SQLRETURN set_desc_field_win32_w( struct descriptor *desc, SQLSMALLINT record, SQLSMALLINT id, SQLPOINTER value,
                                         SQLINTEGER len )
{
    if (desc->hdr.win32_funcs->SQLSetDescFieldW)
        return desc->hdr.win32_funcs->SQLSetDescFieldW( desc->hdr.win32_handle, record, id, value, len );
    if (desc->hdr.win32_funcs->SQLSetDescField) FIXME( "Unicode to ANSI conversion not handled\n" );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLSetDescFieldW          [ODBC32.173]
 */
SQLRETURN WINAPI SQLSetDescFieldW(SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber, SQLSMALLINT FieldIdentifier,
                                  SQLPOINTER Value, SQLINTEGER BufferLength)
{
    struct descriptor *desc = (struct descriptor *)lock_object( DescriptorHandle, SQL_HANDLE_DESC );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(DescriptorHandle %p, RecNumber %d, FieldIdentifier %d, Value %p, BufferLength %d)\n", DescriptorHandle,
          RecNumber, FieldIdentifier, Value, BufferLength);

    if (!desc) return SQL_INVALID_HANDLE;

    if (desc->hdr.unix_handle)
    {
        ret = set_desc_field_unix_w( desc, RecNumber, FieldIdentifier, Value, BufferLength );
    }
    else if (desc->hdr.win32_handle)
    {
        ret = set_desc_field_win32_w( desc, RecNumber, FieldIdentifier, Value, BufferLength );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &desc->hdr );
    return ret;
}

static SQLRETURN set_stmt_attr_unix_w( struct statement *stmt, SQLINTEGER attr, SQLPOINTER value, SQLINTEGER len )
{
    struct SQLSetStmtAttrW_params params = { stmt->hdr.unix_handle, attr, value, len };
    SQLRETURN ret;

    if (SUCCESS((ret = ODBC_CALL( SQLSetStmtAttrW, &params ))))
    {
        SQLULEN row_count = (SQLULEN)value;
        if (attr == SQL_ATTR_ROW_ARRAY_SIZE && row_count != stmt->row_count)
        {
            TRACE( "resizing result length array\n" );
            if (!resize_result_lengths( stmt, row_count )) ret = SQL_ERROR;
            else stmt->row_count = row_count;
        }
    }
    return ret;
}

static SQLRETURN set_stmt_attr_win32_w( struct statement *stmt, SQLINTEGER attr, SQLPOINTER value, SQLINTEGER len )
{
    if (stmt->hdr.win32_funcs->SQLSetStmtAttrW)
        return stmt->hdr.win32_funcs->SQLSetStmtAttrW( stmt->hdr.win32_handle, attr, value, len );
    if (stmt->hdr.win32_funcs->SQLSetStmtAttr) FIXME( "Unicode to ANSI conversion not handled\n" );
    return SQL_ERROR;
}

/*************************************************************************
 *				SQLSetStmtAttrW          [ODBC32.176]
 */
SQLRETURN WINAPI SQLSetStmtAttrW(SQLHSTMT StatementHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                 SQLINTEGER StringLength)
{
    struct statement *stmt = (struct statement *)lock_object( StatementHandle, SQL_HANDLE_STMT );
    SQLRETURN ret = SQL_ERROR;

    TRACE("(StatementHandle %p, Attribute %d, Value %p, StringLength %d)\n", StatementHandle, Attribute,
          Value, StringLength);

    if (!stmt) return SQL_INVALID_HANDLE;

    switch (Attribute)
    {
    case SQL_ATTR_APP_ROW_DESC:
    case SQL_ATTR_APP_PARAM_DESC:
    {
        struct descriptor *desc = (struct descriptor *)lock_object( Value, SQL_HANDLE_DESC );
        if (desc)
        {
            if (stmt->hdr.unix_handle)
            {
                if (sizeof(desc->hdr.unix_handle) > sizeof(Value))
                    ERR( "truncating descriptor handle, consider using a Windows driver\n" );
                Value = (SQLPOINTER)(ULONG_PTR)desc->hdr.unix_handle;
            }
            else Value = desc->hdr.win32_handle;
            unlock_object( &desc->hdr );
        }
        break;
    }
    default: break;
    }

    if (stmt->hdr.unix_handle)
    {
        ret = set_stmt_attr_unix_w( stmt, Attribute, Value, StringLength );
    }
    else if (stmt->hdr.win32_handle)
    {
        ret = set_stmt_attr_win32_w( stmt, Attribute, Value, StringLength );
    }

    TRACE("Returning %d\n", ret);
    unlock_object( &stmt->hdr );
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
        if (is_wow64)
        {
            TEB64 *teb64 = ULongToPtr( NtCurrentTeb()->GdiBatchCount );
            if (teb64)
            {
                PEB64 *peb64 = ULongToPtr( teb64->Peb );
                is_old_wow64 = !peb64->LdrData;
            }
        }
        break;

    case DLL_PROCESS_DETACH:
        if (__wine_unixlib_handle) WINE_UNIX_CALL( process_detach, NULL );
        if (reserved) break;
    }

    return TRUE;
}
