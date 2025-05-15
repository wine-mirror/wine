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

#if 0
#pragma makedep unix
#endif

#include "config.h"

#include <stdarg.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dlfcn.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "sql.h"
#include "sqlucode.h"
#include "sqlext.h"

#include "wine/debug.h"
#include "unixlib.h"

WINE_DEFAULT_DEBUG_CHANNEL(odbc);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

static void *libodbc;

static SQLRETURN (*pSQLAllocHandle)(SQLSMALLINT,SQLHANDLE,SQLHANDLE*);
static SQLRETURN (*pSQLAllocHandleStd)(SQLSMALLINT,SQLHANDLE,SQLHANDLE*);
static SQLRETURN (*pSQLBindCol)(SQLHSTMT,SQLUSMALLINT,SQLSMALLINT,SQLPOINTER,SQLLEN,SQLLEN*);
static SQLRETURN (*pSQLBindParam)(SQLHSTMT,SQLUSMALLINT,SQLSMALLINT,SQLSMALLINT,SQLULEN,SQLSMALLINT,SQLPOINTER,SQLLEN*);
static SQLRETURN (*pSQLBindParameter)(SQLHSTMT,SQLUSMALLINT,SQLSMALLINT,SQLSMALLINT,SQLSMALLINT,SQLULEN,SQLSMALLINT, SQLPOINTER,SQLLEN,SQLLEN*);
static SQLRETURN (*pSQLBrowseConnect)(SQLHDBC,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLBrowseConnectW)(SQLHDBC,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLBulkOperations)(SQLHSTMT,SQLSMALLINT);
static SQLRETURN (*pSQLCancel)(SQLHSTMT);
static SQLRETURN (*pSQLCloseCursor)(SQLHSTMT);
static SQLRETURN (*pSQLColAttribute)(SQLHSTMT,SQLUSMALLINT,SQLUSMALLINT,SQLPOINTER,SQLSMALLINT,SQLSMALLINT*,SQLLEN*);
static SQLRETURN (*pSQLColAttributeW)(SQLHSTMT,SQLUSMALLINT,SQLUSMALLINT,SQLPOINTER,SQLSMALLINT,SQLSMALLINT*,SQLLEN*);
static SQLRETURN (*pSQLColAttributes)(SQLHSTMT,SQLUSMALLINT,SQLUSMALLINT,SQLPOINTER,SQLSMALLINT,SQLSMALLINT*,SQLLEN*);
static SQLRETURN (*pSQLColAttributesW)(SQLHSTMT,SQLUSMALLINT,SQLUSMALLINT,SQLPOINTER,SQLSMALLINT,SQLSMALLINT*,SQLLEN*);
static SQLRETURN (*pSQLColumnPrivileges)(SQLHSTMT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT, SQLCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLColumnPrivilegesW)(SQLHSTMT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT, SQLWCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLColumns)(SQLHSTMT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*, SQLSMALLINT);
static SQLRETURN (*pSQLColumnsW)(SQLHSTMT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*, SQLSMALLINT);
static SQLRETURN (*pSQLConnect)(SQLHDBC,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLConnectW)(SQLHDBC,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLCopyDesc)(SQLHDESC,SQLHDESC);
static SQLRETURN (*pSQLDataSources)(SQLHENV,SQLUSMALLINT,SQLCHAR*,SQLSMALLINT,SQLSMALLINT*,SQLCHAR*,SQLSMALLINT, SQLSMALLINT*);
static SQLRETURN (*pSQLDataSourcesW)(SQLHENV,SQLUSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLSMALLINT*,SQLWCHAR*,SQLSMALLINT, SQLSMALLINT*);
static SQLRETURN (*pSQLDescribeCol)(SQLHSTMT,SQLUSMALLINT,SQLCHAR*,SQLSMALLINT,SQLSMALLINT*,SQLSMALLINT*,SQLULEN*, SQLSMALLINT*,SQLSMALLINT*);
static SQLRETURN (*pSQLDescribeColW)(SQLHSTMT,SQLUSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLSMALLINT*,SQLSMALLINT*,SQLULEN*, SQLSMALLINT*,SQLSMALLINT*);
static SQLRETURN (*pSQLDescribeParam)(SQLHSTMT,SQLUSMALLINT,SQLSMALLINT*,SQLULEN*,SQLSMALLINT*,SQLSMALLINT*);
static SQLRETURN (*pSQLDisconnect)(SQLHDBC);
static SQLRETURN (*pSQLDriverConnect)(SQLHDBC,SQLHWND,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLSMALLINT*, SQLUSMALLINT);
static SQLRETURN (*pSQLDriverConnectW)(SQLHDBC,SQLHWND,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLSMALLINT*, SQLUSMALLINT);
static SQLRETURN (*pSQLDrivers)(SQLHENV,SQLUSMALLINT,SQLCHAR*,SQLSMALLINT,SQLSMALLINT*,SQLCHAR*,SQLSMALLINT, SQLSMALLINT*);
static SQLRETURN (*pSQLDriversW)(SQLHENV,SQLUSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLSMALLINT*,SQLWCHAR*,SQLSMALLINT, SQLSMALLINT*);
static SQLRETURN (*pSQLEndTran)(SQLSMALLINT,SQLHANDLE,SQLSMALLINT);
static SQLRETURN (*pSQLError)(SQLHENV,SQLHDBC,SQLHSTMT,SQLCHAR*,SQLINTEGER*,SQLCHAR*,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLErrorW)(SQLHENV,SQLHDBC,SQLHSTMT,SQLWCHAR*,SQLINTEGER*,SQLWCHAR*,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLExecDirect)(SQLHSTMT,SQLCHAR*,SQLINTEGER);
static SQLRETURN (*pSQLExecDirectW)(SQLHSTMT,SQLWCHAR*,SQLINTEGER);
static SQLRETURN (*pSQLExecute)(SQLHSTMT);
static SQLRETURN (*pSQLExtendedFetch)(SQLHSTMT,SQLUSMALLINT,SQLLEN,SQLULEN*,SQLUSMALLINT*);
static SQLRETURN (*pSQLFetch)(SQLHSTMT);
static SQLRETURN (*pSQLFetchScroll)(SQLHSTMT,SQLSMALLINT,SQLLEN);
static SQLRETURN (*pSQLForeignKeys)(SQLHSTMT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*, SQLSMALLINT,SQLCHAR*,
                                    SQLSMALLINT,SQLCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLForeignKeysW)(SQLHSTMT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT, SQLWCHAR*,SQLSMALLINT,
                                     SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLFreeHandle)(SQLSMALLINT,SQLHANDLE);
static SQLRETURN (*pSQLFreeStmt)(SQLHSTMT,SQLUSMALLINT);
static SQLRETURN (*pSQLGetConnectAttr)(SQLHDBC,SQLINTEGER,SQLPOINTER,SQLINTEGER,SQLINTEGER*);
static SQLRETURN (*pSQLGetConnectAttrW)(SQLHDBC,SQLINTEGER,SQLPOINTER,SQLINTEGER,SQLINTEGER*);
static SQLRETURN (*pSQLGetConnectOption)(SQLHDBC,SQLUSMALLINT,SQLPOINTER);
static SQLRETURN (*pSQLGetConnectOptionW)(SQLHDBC,SQLUSMALLINT,SQLPOINTER);
static SQLRETURN (*pSQLGetCursorName)(SQLHSTMT,SQLCHAR*,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLGetCursorNameW)(SQLHSTMT,SQLWCHAR*,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLGetData)(SQLHSTMT,SQLUSMALLINT,SQLSMALLINT,SQLPOINTER,SQLLEN,SQLLEN*);
static SQLRETURN (*pSQLGetDescField)(SQLHDESC,SQLSMALLINT,SQLSMALLINT,SQLPOINTER,SQLINTEGER,SQLINTEGER*);
static SQLRETURN (*pSQLGetDescFieldW)(SQLHDESC,SQLSMALLINT,SQLSMALLINT,SQLPOINTER,SQLINTEGER,SQLINTEGER*);
static SQLRETURN (*pSQLGetDescRec)(SQLHDESC,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLSMALLINT*,SQLSMALLINT*,SQLSMALLINT*,SQLLEN*,SQLSMALLINT*,
                                   SQLSMALLINT*,SQLSMALLINT*);
static SQLRETURN (*pSQLGetDescRecW)(SQLHDESC,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLSMALLINT*,SQLSMALLINT*,SQLSMALLINT*,SQLLEN*,SQLSMALLINT*,
                                    SQLSMALLINT*,SQLSMALLINT*);
static SQLRETURN (*pSQLGetDiagField)(SQLSMALLINT,SQLHANDLE,SQLSMALLINT,SQLSMALLINT,SQLPOINTER,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLGetDiagFieldW)(SQLSMALLINT,SQLHANDLE,SQLSMALLINT,SQLSMALLINT,SQLPOINTER,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLGetDiagRec)(SQLSMALLINT,SQLHANDLE,SQLSMALLINT,SQLCHAR*,SQLINTEGER*,SQLCHAR*,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLGetDiagRecW)(SQLSMALLINT,SQLHANDLE,SQLSMALLINT,SQLWCHAR*,SQLINTEGER*,SQLWCHAR*,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLGetEnvAttr)(SQLHENV,SQLINTEGER,SQLPOINTER,SQLINTEGER,SQLINTEGER*);
static SQLRETURN (*pSQLGetFunctions)(SQLHDBC,SQLUSMALLINT,SQLUSMALLINT*);
static SQLRETURN (*pSQLGetInfo)(SQLHDBC,SQLUSMALLINT,SQLPOINTER,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLGetInfoW)(SQLHDBC,SQLUSMALLINT,SQLPOINTER,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLGetStmtAttr)(SQLHSTMT,SQLINTEGER,SQLPOINTER,SQLINTEGER,SQLINTEGER*);
static SQLRETURN (*pSQLGetStmtAttrW)(SQLHSTMT,SQLINTEGER,SQLPOINTER,SQLINTEGER,SQLINTEGER*);
static SQLRETURN (*pSQLGetStmtOption)(SQLHSTMT,SQLUSMALLINT,SQLPOINTER);
static SQLRETURN (*pSQLGetTypeInfo)(SQLHSTMT,SQLSMALLINT);
static SQLRETURN (*pSQLGetTypeInfoW)(SQLHSTMT,SQLSMALLINT);
static SQLRETURN (*pSQLMoreResults)(SQLHSTMT);
static SQLRETURN (*pSQLNativeSql)(SQLHDBC,SQLCHAR*,SQLINTEGER,SQLCHAR*,SQLINTEGER,SQLINTEGER*);
static SQLRETURN (*pSQLNativeSqlW)(SQLHDBC,SQLWCHAR*,SQLINTEGER,SQLWCHAR*,SQLINTEGER,SQLINTEGER*);
static SQLRETURN (*pSQLNumParams)(SQLHSTMT,SQLSMALLINT*);
static SQLRETURN (*pSQLNumResultCols)(SQLHSTMT,SQLSMALLINT*);
static SQLRETURN (*pSQLParamData)(SQLHSTMT,SQLPOINTER*);
static SQLRETURN (*pSQLParamOptions)(SQLHSTMT,SQLULEN,SQLULEN*);
static SQLRETURN (*pSQLPrepare)(SQLHSTMT,SQLCHAR*,SQLINTEGER);
static SQLRETURN (*pSQLPrepareW)(SQLHSTMT,SQLWCHAR*,SQLINTEGER);
static SQLRETURN (*pSQLPrimaryKeys)(SQLHSTMT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLPrimaryKeysW)(SQLHSTMT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLProcedureColumns)(SQLHSTMT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLProcedureColumnsW)(SQLHSTMT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLProcedures)(SQLHSTMT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLProceduresW)(SQLHSTMT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLPutData)(SQLHSTMT,SQLPOINTER,SQLLEN);
static SQLRETURN (*pSQLRowCount)(SQLHSTMT,SQLLEN*);
static SQLRETURN (*pSQLSetConnectAttr)(SQLHDBC,SQLINTEGER,SQLPOINTER,SQLINTEGER);
static SQLRETURN (*pSQLSetConnectAttrW)(SQLHDBC,SQLINTEGER,SQLPOINTER,SQLINTEGER);
static SQLRETURN (*pSQLSetConnectOption)(SQLHDBC,SQLUSMALLINT,SQLULEN);
static SQLRETURN (*pSQLSetConnectOptionW)(SQLHDBC,SQLUSMALLINT,SQLULEN);
static SQLRETURN (*pSQLSetCursorName)(SQLHSTMT,SQLCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLSetCursorNameW)(SQLHSTMT,SQLWCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLSetDescField)(SQLHDESC,SQLSMALLINT,SQLSMALLINT,SQLPOINTER,SQLINTEGER);
static SQLRETURN (*pSQLSetDescFieldW)(SQLHDESC,SQLSMALLINT,SQLSMALLINT,SQLPOINTER,SQLINTEGER);
static SQLRETURN (*pSQLSetDescRec)(SQLHDESC,SQLSMALLINT,SQLSMALLINT,SQLSMALLINT,SQLLEN,SQLSMALLINT,SQLSMALLINT,SQLPOINTER,SQLLEN*,SQLLEN*);
static SQLRETURN (*pSQLSetEnvAttr)(SQLHENV,SQLINTEGER,SQLPOINTER,SQLINTEGER);
static SQLRETURN (*pSQLSetParam)(SQLHSTMT,SQLUSMALLINT,SQLSMALLINT,SQLSMALLINT,SQLULEN,SQLSMALLINT,SQLPOINTER,SQLLEN*);
static SQLRETURN (*pSQLSetPos)(SQLHSTMT,SQLSETPOSIROW,SQLUSMALLINT,SQLUSMALLINT);
static SQLRETURN (*pSQLSetScrollOptions)(SQLHSTMT,SQLUSMALLINT,SQLLEN,SQLUSMALLINT);
static SQLRETURN (*pSQLSetStmtAttr)(SQLHSTMT,SQLINTEGER,SQLPOINTER,SQLINTEGER);
static SQLRETURN (*pSQLSetStmtAttrW)(SQLHSTMT,SQLINTEGER,SQLPOINTER,SQLINTEGER);
static SQLRETURN (*pSQLSetStmtOption)(SQLHSTMT,SQLUSMALLINT,SQLULEN);
static SQLRETURN (*pSQLSpecialColumns)(SQLHSTMT,SQLUSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLUSMALLINT,SQLUSMALLINT);
static SQLRETURN (*pSQLSpecialColumnsW)(SQLHSTMT,SQLUSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLUSMALLINT,SQLUSMALLINT);
static SQLRETURN (*pSQLStatistics)(SQLHSTMT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLUSMALLINT,SQLUSMALLINT);
static SQLRETURN (*pSQLStatisticsW)(SQLHSTMT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLUSMALLINT,SQLUSMALLINT);
static SQLRETURN (*pSQLTablePrivileges)(SQLHSTMT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLTablePrivilegesW)(SQLHSTMT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLTables)(SQLHSTMT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLTablesW)(SQLHSTMT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLTransact)(SQLHENV,SQLHDBC,SQLUSMALLINT);

static NTSTATUS load_odbc(void)
{
   const char *s = getenv( "LIB_ODBC_DRIVER_MANAGER" );

   if (!s || !s[0]) s = SONAME_LIBODBC;
   if (!(libodbc = dlopen( s, RTLD_NOW )))
   {
       ERR_(winediag)( "failed to open library %s: %s\n", debugstr_a(s), debugstr_a(dlerror()) );
       return STATUS_DLL_NOT_FOUND;
   }
#define LOAD_FUNC(name) \
    if (!(p##name = dlsym( libodbc, #name ))) \
    { \
        ERR( "failed to load %s\n", #name ); \
        goto fail; \
    }

    LOAD_FUNC( SQLAllocHandle );
    LOAD_FUNC( SQLAllocHandleStd );
    LOAD_FUNC( SQLBindCol );
    LOAD_FUNC( SQLBindParam );
    LOAD_FUNC( SQLBindParameter );
    LOAD_FUNC( SQLBrowseConnect );
    LOAD_FUNC( SQLBrowseConnectW );
    LOAD_FUNC( SQLBulkOperations );
    LOAD_FUNC( SQLCancel );
    LOAD_FUNC( SQLCloseCursor );
    LOAD_FUNC( SQLColAttribute );
    LOAD_FUNC( SQLColAttributeW );
    LOAD_FUNC( SQLColAttributes );
    LOAD_FUNC( SQLColAttributesW );
    LOAD_FUNC( SQLColumnPrivileges );
    LOAD_FUNC( SQLColumnPrivilegesW );
    LOAD_FUNC( SQLColumns );
    LOAD_FUNC( SQLColumnsW );
    LOAD_FUNC( SQLConnect );
    LOAD_FUNC( SQLConnectW );
    LOAD_FUNC( SQLCopyDesc );
    LOAD_FUNC( SQLDataSources );
    LOAD_FUNC( SQLDataSourcesW );
    LOAD_FUNC( SQLDescribeCol );
    LOAD_FUNC( SQLDescribeColW );
    LOAD_FUNC( SQLDescribeParam );
    LOAD_FUNC( SQLDisconnect );
    LOAD_FUNC( SQLDriverConnect );
    LOAD_FUNC( SQLDriverConnectW );
    LOAD_FUNC( SQLDrivers );
    LOAD_FUNC( SQLDriversW );
    LOAD_FUNC( SQLEndTran );
    LOAD_FUNC( SQLError );
    LOAD_FUNC( SQLErrorW );
    LOAD_FUNC( SQLExecDirect );
    LOAD_FUNC( SQLExecDirectW );
    LOAD_FUNC( SQLExecute );
    LOAD_FUNC( SQLExtendedFetch );
    LOAD_FUNC( SQLFetch );
    LOAD_FUNC( SQLFetchScroll );
    LOAD_FUNC( SQLForeignKeys );
    LOAD_FUNC( SQLForeignKeysW );
    LOAD_FUNC( SQLFreeHandle );
    LOAD_FUNC( SQLFreeStmt );
    LOAD_FUNC( SQLGetConnectAttr );
    LOAD_FUNC( SQLGetConnectAttrW );
    LOAD_FUNC( SQLGetConnectOption );
    LOAD_FUNC( SQLGetConnectOptionW );
    LOAD_FUNC( SQLGetCursorName );
    LOAD_FUNC( SQLGetCursorNameW );
    LOAD_FUNC( SQLGetData );
    LOAD_FUNC( SQLGetDescField );
    LOAD_FUNC( SQLGetDescFieldW );
    LOAD_FUNC( SQLGetDescRec );
    LOAD_FUNC( SQLGetDescRecW );
    LOAD_FUNC( SQLGetDiagField );
    LOAD_FUNC( SQLGetDiagFieldW );
    LOAD_FUNC( SQLGetDiagRec );
    LOAD_FUNC( SQLGetDiagRecW );
    LOAD_FUNC( SQLGetEnvAttr );
    LOAD_FUNC( SQLGetFunctions );
    LOAD_FUNC( SQLGetInfo );
    LOAD_FUNC( SQLGetInfoW );
    LOAD_FUNC( SQLGetStmtAttr );
    LOAD_FUNC( SQLGetStmtAttrW );
    LOAD_FUNC( SQLGetStmtOption );
    LOAD_FUNC( SQLGetTypeInfo );
    LOAD_FUNC( SQLGetTypeInfoW );
    LOAD_FUNC( SQLMoreResults );
    LOAD_FUNC( SQLNativeSql );
    LOAD_FUNC( SQLNativeSqlW );
    LOAD_FUNC( SQLNumParams );
    LOAD_FUNC( SQLNumResultCols );
    LOAD_FUNC( SQLParamData );
    LOAD_FUNC( SQLParamOptions );
    LOAD_FUNC( SQLPrepare );
    LOAD_FUNC( SQLPrepareW );
    LOAD_FUNC( SQLPrimaryKeys );
    LOAD_FUNC( SQLPrimaryKeysW );
    LOAD_FUNC( SQLProcedureColumns );
    LOAD_FUNC( SQLProcedureColumnsW );
    LOAD_FUNC( SQLProcedures );
    LOAD_FUNC( SQLProceduresW );
    LOAD_FUNC( SQLPutData );
    LOAD_FUNC( SQLRowCount );
    LOAD_FUNC( SQLSetConnectAttr );
    LOAD_FUNC( SQLSetConnectAttrW );
    LOAD_FUNC( SQLSetConnectOption );
    LOAD_FUNC( SQLSetConnectOptionW );
    LOAD_FUNC( SQLSetCursorName );
    LOAD_FUNC( SQLSetCursorNameW );
    LOAD_FUNC( SQLSetDescField );
    LOAD_FUNC( SQLSetDescFieldW );
    LOAD_FUNC( SQLSetDescRec );
    LOAD_FUNC( SQLSetEnvAttr );
    LOAD_FUNC( SQLSetParam );
    LOAD_FUNC( SQLSetPos );
    LOAD_FUNC( SQLSetScrollOptions );
    LOAD_FUNC( SQLSetStmtAttr );
    LOAD_FUNC( SQLSetStmtAttrW );
    LOAD_FUNC( SQLSetStmtOption );
    LOAD_FUNC( SQLSpecialColumns );
    LOAD_FUNC( SQLSpecialColumnsW );
    LOAD_FUNC( SQLStatistics );
    LOAD_FUNC( SQLStatisticsW );
    LOAD_FUNC( SQLTablePrivileges );
    LOAD_FUNC( SQLTablePrivilegesW );
    LOAD_FUNC( SQLTables );
    LOAD_FUNC( SQLTablesW );
    LOAD_FUNC( SQLTransact );
#undef LOAD_FUNC
    return STATUS_SUCCESS;

fail:
    dlclose( libodbc );
    libodbc = NULL;
    return STATUS_DLL_NOT_FOUND;
}

static inline void init_unicode_string( UNICODE_STRING *str, const WCHAR *data, ULONG data_size )
{
    str->Length = str->MaximumLength = data_size;
    str->Buffer = (WCHAR *)data;
}

static inline void ascii_to_unicode( WCHAR *dst, const char *src, size_t len )
{
    while (len--) *dst++ = (unsigned char)*src++;
}

static HANDLE create_hkcu_key( const WCHAR *path, ULONG path_size )
{
    NTSTATUS status;
    char buffer[512 + ARRAY_SIZE("\\Registry\\User\\")];
    WCHAR bufferW[512 + ARRAY_SIZE("\\Registry\\User\\")];
    DWORD_PTR sid_data[(sizeof(TOKEN_USER) + SECURITY_MAX_SID_SIZE) / sizeof(DWORD_PTR)];
    DWORD i, len = sizeof(sid_data);
    SID *sid;
    UNICODE_STRING str;
    OBJECT_ATTRIBUTES attr;
    HANDLE ret;

    status = NtQueryInformationToken( GetCurrentThreadEffectiveToken(), TokenUser, sid_data, len, &len );
    if (status) return NULL;

    sid = ((TOKEN_USER *)sid_data)->User.Sid;
    len = snprintf( buffer, sizeof(buffer), "\\Registry\\User\\S-%u-%u", sid->Revision,
                   MAKELONG( MAKEWORD( sid->IdentifierAuthority.Value[5], sid->IdentifierAuthority.Value[4] ),
                             MAKEWORD( sid->IdentifierAuthority.Value[3], sid->IdentifierAuthority.Value[2] )));
    for (i = 0; i < sid->SubAuthorityCount; i++)
        len += snprintf( buffer + len, sizeof(buffer) - len, "-%u", sid->SubAuthority[i] );
    buffer[len++] = '\\';

    ascii_to_unicode( bufferW, buffer, len );
    memcpy( bufferW + len, path, path_size );
    init_unicode_string( &str, bufferW, len * sizeof(WCHAR) + path_size );
    InitializeObjectAttributes( &attr, &str, OBJ_CASE_INSENSITIVE, 0, NULL );
    if (!NtCreateKey( &ret, KEY_ALL_ACCESS, &attr, 0, NULL, 0, NULL )) return ret;
    return NULL;
}

static HANDLE create_hklm_key( const WCHAR *path, ULONG path_size )
{
    static const WCHAR machineW[] = {'\\','R','e','g','i','s','t','r','y','\\','M','a','c','h','i','n','e','\\'};
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING str;
    WCHAR bufferW[256 + ARRAY_SIZE(machineW)];
    HANDLE ret;

    memcpy( bufferW, machineW, sizeof(machineW) );
    memcpy( bufferW + ARRAY_SIZE(machineW), path, path_size );
    init_unicode_string( &str, bufferW, sizeof(machineW) + path_size );
    InitializeObjectAttributes( &attr, &str, OBJ_CASE_INSENSITIVE, 0, NULL );
    if (!NtCreateKey( &ret, KEY_ALL_ACCESS, &attr, 0, NULL, 0, NULL )) return ret;
    return NULL;
}

static HANDLE create_key( HANDLE root, const WCHAR *path, ULONG path_size )
{
    UNICODE_STRING name = { path_size, path_size, (WCHAR *)path };
    OBJECT_ATTRIBUTES attr;
    HANDLE ret;

    attr.Length = sizeof(attr);
    attr.RootDirectory = root;
    attr.ObjectName = &name;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    if (NtCreateKey( &ret, MAXIMUM_ALLOWED, &attr, 0, NULL, 0, NULL )) return NULL;
    return ret;
}

static HANDLE open_key( HANDLE root, const WCHAR *path, ULONG path_size )
{
    UNICODE_STRING name = { path_size, path_size, (WCHAR *)path };
    OBJECT_ATTRIBUTES attr;
    HANDLE ret;

    attr.Length = sizeof(attr);
    attr.RootDirectory = root;
    attr.ObjectName = &name;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    if (NtOpenKey( &ret, MAXIMUM_ALLOWED, &attr )) return NULL;
    return ret;
}

static ULONG query_value( HANDLE key, const WCHAR *name, ULONG name_size, KEY_VALUE_PARTIAL_INFORMATION *info,
                          ULONG size )
{
    UNICODE_STRING str = { name_size, name_size, (WCHAR *)name };
    if (NtQueryValueKey( key, &str, KeyValuePartialInformation, info, size, &size )) return 0;
    return size - FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data);
}

static BOOL set_value( HANDLE key, const WCHAR *name, ULONG name_size, ULONG type, const void *value, ULONG count )
{
    UNICODE_STRING str = { name_size, name_size, (WCHAR *)name };
    return !NtSetValueKey( key, &str, 0, type, value, count );
}

static HANDLE open_odbcinst_key( void )
{
    static const WCHAR odbcW[] = {'S','o','f','t','w','a','r','e','\\','O','D','B','C'};
    static const WCHAR odbcinstW[] = {'O','D','B','C','I','N','S','T','.','I','N','I'};
    HANDLE ret, root = create_hklm_key( odbcW, sizeof(odbcW) );
    ret = create_key( root, odbcinstW, sizeof(odbcinstW) );
    NtClose( root );
    return ret;
}

static HANDLE open_drivers_key( void )
{
    static const WCHAR driversW[] = {'O','D','B','C',' ','D','r','i','v','e','r','s'};
    HANDLE ret, root = open_odbcinst_key();
    ret = create_key( root, driversW, sizeof(driversW) );
    NtClose( root );
    return ret;
}

/***********************************************************************
 * odbc_replicate_odbcinst_to_registry
 *
 * Utility to odbc_replicate_odbcinst_to_registry() to replicate the drivers of the
 * ODBCINST.INI settings
 *
 * The driver settings are not replicated to the registry.  If we were to
 * replicate them we would need to decide whether to replicate all settings
 * or to do some translation; whether to remove any entries present only in
 * the windows registry, etc.
 */
static void replicate_odbcinst_to_registry( SQLHENV env )
{
    HANDLE key_odbcinst, key_drivers;
    SQLRETURN ret;
    SQLUSMALLINT dir = SQL_FETCH_FIRST;
    WCHAR desc[256], attrs[1024];
    SQLSMALLINT len_desc, len_attrs;

    if (!(key_odbcinst = open_odbcinst_key())) return;
    if (!(key_drivers = open_drivers_key()))
    {
        NtClose( key_odbcinst );
        return;
    }

    while (SUCCESS((ret = pSQLDriversW( env, dir, (SQLWCHAR *)desc, ARRAY_SIZE(desc), &len_desc, attrs,
                                        ARRAY_SIZE(attrs), &len_attrs ))))
    {
        static const WCHAR installedW[] = {'I','n','s','t','a','l','l','e','d',0};
        HANDLE key_driver;
        WCHAR buffer[1024];
        KEY_VALUE_PARTIAL_INFORMATION *info = (KEY_VALUE_PARTIAL_INFORMATION *)buffer;

        dir = SQL_FETCH_NEXT;
        if (!query_value( key_drivers, desc, len_desc * sizeof(WCHAR), info, sizeof(buffer) ))
        {
            set_value( key_drivers, desc, len_desc * sizeof(WCHAR), REG_SZ, (const BYTE *)installedW,
                       sizeof(installedW) );
        }

        if ((key_driver = create_key( key_odbcinst, desc, wcslen( desc ) * sizeof(WCHAR) )))
        {
            static const WCHAR driverW[] = {'D','r','i','v','e','r'}, driver_eqW[] = {'D','r','i','v','e','r','='};
            const WCHAR *driver = NULL, *ptr = attrs;

            while (*ptr)
            {
                if (len_attrs > ARRAY_SIZE(driver_eqW) && !memcmp( driver_eqW, ptr, sizeof(driver_eqW) ))
                {
                    driver = ptr + 7;
                    break;
                }
                len_attrs -= wcslen( ptr ) + 1;
                ptr += wcslen( ptr ) + 1;
            }
            if (driver) set_value( key_driver, driverW, sizeof(driverW), REG_SZ, (const BYTE *)driver,
                                   (wcslen(driver) + 1) * sizeof(WCHAR) );
            NtClose( key_driver );
        }
    }

    NtClose( key_drivers );
    NtClose( key_odbcinst );
}

struct drivers
{
    ULONG   count;
    WCHAR **names;
};

static void get_drivers( struct drivers *drivers )
{
    ULONG idx = 0, count = 0, capacity, info_size, info_max_size;
    KEY_VALUE_BASIC_INFORMATION *info = NULL;
    WCHAR **names;
    NTSTATUS status = STATUS_SUCCESS;
    HANDLE key;

    drivers->count = 0;
    drivers->names = NULL;

    if (!(key = open_drivers_key())) return;

    capacity = 4;
    if (!(names = malloc( capacity * sizeof(*names) ))) goto done;
    info_max_size = offsetof(KEY_VALUE_BASIC_INFORMATION, Name) + 512;
    if (!(info = malloc( info_max_size ))) goto error;

    while (1)
    {
        status = NtEnumerateValueKey( key, idx, KeyValueBasicInformation, info, info_max_size, &info_size );
        while (status == STATUS_BUFFER_OVERFLOW)
        {
            info_max_size = info_size;
            if (!(info = realloc( info, info_max_size ))) goto error;
            status = NtEnumerateValueKey( key, idx, KeyValueFullInformation, info, info_max_size, &info_size );
        }

        if (status == STATUS_NO_MORE_ENTRIES)
        {
            drivers->count = count;
            drivers->names = names;
            goto done;
        }
        idx++;
        if (status) break;
        if (info->Type != REG_SZ) continue;

        if (count >= capacity)
        {
            capacity = capacity * 3 / 2;
            if (!(names = realloc( names, capacity * sizeof(*names) ))) break;
        }

        if (!(names[count] = malloc( info->NameLength + sizeof(WCHAR) ))) break;
        memcpy( names[count], info->Name, info->NameLength );
        names[count][info->NameLength / sizeof(WCHAR)] = 0;
        count++;
    }

error:
    if (names) while (count) free( names[--count] );
    free( names );

done:
    free( info );
    NtClose( key );
}

/* use the driver name to look up the driver filename */
static WCHAR *get_driver_filename( const WCHAR *name )
{
    struct drivers drivers;
    HANDLE key_odbcinst, key_driver;
    WCHAR *ret = NULL;
    ULONG i;

    get_drivers( &drivers );
    if (!drivers.count) return NULL;

    key_odbcinst = open_odbcinst_key();
    for (i = 0; i < drivers.count; i++)
    {
        static const WCHAR driverW[] = {'D','r','i','v','e','r'};
        WCHAR buffer[1024];
        KEY_VALUE_PARTIAL_INFORMATION *info = (KEY_VALUE_PARTIAL_INFORMATION *)buffer;

        if (wcscmp( name, drivers.names[i] )) continue;
        if ((key_driver = open_key( key_odbcinst, drivers.names[i], wcslen(drivers.names[i]) * sizeof(WCHAR) )))
        {
            if (query_value( key_driver, driverW, sizeof(driverW), info, sizeof(buffer) ) && info->Type == REG_SZ &&
                (ret = malloc( info->DataLength + sizeof(WCHAR) )))
            {
                memcpy( ret, info->Data, info->DataLength );
                ret[info->DataLength / sizeof(WCHAR)] = 0;
                break;
            }
            NtClose( key_driver );
        }
    }

    for (i = 0; i < drivers.count; i++) free( drivers.names[i] );
    free( drivers.names );
    NtClose( key_odbcinst );
    return ret;
}

static HANDLE open_odbcini_key( BOOL user )
{
    static const WCHAR odbcW[] = {'S','o','f','t','w','a','r','e','\\','O','D','B','C'};
    static const WCHAR odbcinstW[] = {'O','D','B','C','.','I','N','I'};
    HANDLE ret, root = user ? create_hkcu_key( odbcW, sizeof(odbcW) ) : create_hklm_key( odbcW, sizeof(odbcW) );
    ret = create_key( root, odbcinstW, sizeof(odbcinstW) );
    NtClose( root );
    return ret;
}

static HANDLE open_sources_key( BOOL user )
{
    static const WCHAR sourcesW[] = {'O','D','B','C',' ','D','a','t','a',' ','S','o','u','r','c','e','s'};
    HANDLE ret, root = open_odbcini_key( user );
    ret = create_key( root, sourcesW, sizeof(sourcesW) );
    NtClose( root );
    return ret;
}

/***********************************************************************
 * replicate_odbc_to_registry
 *
 * Utility to replicate either the USER or SYSTEM data sources.
 * This function must be called after replicate_odbcinst_to_registry().
 */
static void replicate_odbc_to_registry( BOOL is_user, SQLHENV env )
{
    HANDLE key_odbcini, key_sources;
    SQLRETURN ret;
    SQLUSMALLINT dir;
    WCHAR dsn[SQL_MAX_DSN_LENGTH + 1], desc[256];
    SQLSMALLINT len_dsn, len_desc;

    if (!(key_odbcini = open_odbcini_key( is_user ))) return;
    if (!(key_sources = open_sources_key( is_user )))
    {
        NtClose( key_odbcini );
        return;
    }

    dir = is_user ? SQL_FETCH_FIRST_USER : SQL_FETCH_FIRST_SYSTEM;
    while (SUCCESS((ret = pSQLDataSourcesW( env, dir, dsn, sizeof(dsn), &len_dsn, desc, sizeof(desc), &len_desc ))))
    {
        HANDLE key_source;
        WCHAR buffer[1024], *filename = get_driver_filename( desc );
        KEY_VALUE_PARTIAL_INFORMATION *info = (KEY_VALUE_PARTIAL_INFORMATION *)buffer;

        dir = SQL_FETCH_NEXT;
        if (!query_value( key_sources, dsn, len_dsn * sizeof(WCHAR), info, sizeof(buffer) ) && desc[0])
        {
            set_value( key_sources, dsn, len_dsn * sizeof(WCHAR), REG_SZ, (const BYTE *)desc,
                       (wcslen(desc) + 1) * sizeof(WCHAR) );
        }

        if ((key_source = create_key( key_odbcini, dsn, len_dsn * sizeof(WCHAR) )))
        {
            static const WCHAR driverW[] = {'D','r','i','v','e','r'};
            if (!query_value( key_source, driverW, sizeof(driverW), info, sizeof(buffer) ))
            {
                set_value( key_source, driverW, sizeof(driverW), REG_SZ, (const BYTE *)filename,
                           (wcslen(filename) + 1) * sizeof(WCHAR) );
            }
            NtClose( key_source );
        }
        free( filename );
    }

    NtClose( key_sources );
    NtClose( key_odbcini );
}

/***********************************************************************
 * replicate_to_registry
 *
 * Unfortunately some of the functions that Windows documents as being part
 * of the ODBC API it implements directly during compilation or something
 * in terms of registry access functions.
 * e.g. SQLGetInstalledDrivers queries the list at
 * HKEY_LOCAL_MACHINE\Software\ODBC\ODBCINST.INI\ODBC Drivers
 *
 * This function is called when the driver manager is loaded and is used
 * to replicate the appropriate details into the Wine registry
 */
static void replicate_to_registry(void)
{
    SQLHENV env;
    SQLRETURN ret;

    if (!(ret = pSQLAllocHandle( SQL_HANDLE_ENV, NULL, &env )))
    {
        pSQLSetEnvAttr( env, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC2, 0 );
        replicate_odbcinst_to_registry( env );
        replicate_odbc_to_registry( FALSE /* system dsn */, env );
        replicate_odbc_to_registry( TRUE /* user dsn */, env );
        pSQLFreeHandle( SQL_HANDLE_ENV, env );
    }
    else
    {
        TRACE( "error %d opening an SQL environment\n", ret );
        WARN( "external ODBC settings have not been replicated to the Wine registry\n" );
    }
}

static NTSTATUS odbc_process_attach( void *args )
{
    NTSTATUS status;
    if ((status = load_odbc())) return status;
    replicate_to_registry();
    return STATUS_SUCCESS;
}

static NTSTATUS odbc_process_detach( void *args )
{
    if (libodbc) dlclose( libodbc );
    libodbc = NULL;
    return STATUS_SUCCESS;
}

static NTSTATUS wrap_SQLAllocHandle( void *args )
{
    struct SQLAllocHandle_params *params = args;
    return pSQLAllocHandle( params->HandleType, (SQLHANDLE)(ULONG_PTR)params->InputHandle,
                            (SQLHANDLE *)params->OutputHandle );
}

static NTSTATUS wrap_SQLAllocHandleStd( void *args )
{
    struct SQLAllocHandleStd_params *params = args;
    return pSQLAllocHandleStd( params->HandleType, (SQLHANDLE)(ULONG_PTR)params->InputHandle,
                               (SQLHANDLE *)params->OutputHandle );
}

static NTSTATUS wrap_SQLBindCol( void *args )
{
    struct SQLBindCol_params *params = args;
    return pSQLBindCol( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->ColumnNumber, params->TargetType,
                        params->TargetValue, params->BufferLength, params->StrLen_or_Ind );
}

static NTSTATUS wrap_SQLBindParameter( void *args )
{
    struct SQLBindParameter_params *params = args;
    return pSQLBindParameter( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->ParameterNumber,
                              params->InputOutputType, params->ValueType, params->ParameterType, params->ColumnSize,
                              params->DecimalDigits, params->ParameterValue, params->BufferLength,
                              params->StrLen_or_Ind );
}

static NTSTATUS wrap_SQLBrowseConnect( void *args )
{
    struct SQLBrowseConnect_params *params = args;
    return pSQLBrowseConnect( (SQLHDBC)(ULONG_PTR)params->ConnectionHandle, params->InConnectionString,
                              params->StringLength1, params->OutConnectionString, params->BufferLength,
                              params->StringLength2 );
}

static NTSTATUS wrap_SQLBrowseConnectW( void *args )
{
    struct SQLBrowseConnectW_params *params = args;
    return pSQLBrowseConnectW( (SQLHDBC)(ULONG_PTR)params->ConnectionHandle, params->InConnectionString,
                               params->StringLength1, params->OutConnectionString, params->BufferLength,
                               params->StringLength2 );
}

static NTSTATUS wrap_SQLBulkOperations( void *args )
{
    struct SQLBulkOperations_params *params = args;
    return pSQLBulkOperations( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->Operation );
}

static NTSTATUS wrap_SQLCancel( void *args )
{
    struct SQLCancel_params *params = args;
    return pSQLCancel( (SQLHSTMT)(ULONG_PTR)params->StatementHandle );
}

static NTSTATUS wrap_SQLCloseCursor( void *args )
{
    struct SQLCloseCursor_params *params = args;
    return pSQLCloseCursor( (SQLHSTMT)(ULONG_PTR)params->StatementHandle );
}

static NTSTATUS wrap_SQLColAttribute( void *args )
{
    struct SQLColAttribute_params *params = args;
    return pSQLColAttribute( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->ColumnNumber,
                             params->FieldIdentifier, params->CharacterAttribute, params->BufferLength,
                             params->StringLength, (SQLLEN *)(ULONG_PTR)params->NumericAttribute );
}

static NTSTATUS wrap_SQLColAttributeW( void *args )
{
    struct SQLColAttributeW_params *params = args;
    return pSQLColAttributeW( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->ColumnNumber,
                              params->FieldIdentifier, params->CharacterAttribute,
                              params->BufferLength, params->StringLength,
                              (SQLLEN *)(ULONG_PTR)params->NumericAttribute );
}

static NTSTATUS wrap_SQLColAttributes( void *args )
{
    struct SQLColAttributes_params *params = args;
    return pSQLColAttributes( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->ColumnNumber,
                              params->FieldIdentifier, params->CharacterAttributes, params->BufferLength,
                              params->StringLength, (SQLLEN *)(ULONG_PTR)params->NumericAttributes );
}

static NTSTATUS wrap_SQLColAttributesW( void *args )
{
    struct SQLColAttributesW_params *params = args;
    return pSQLColAttributesW( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->ColumnNumber,
                               params->FieldIdentifier, params->CharacterAttributes, params->BufferLength,
                               params->StringLength, (SQLLEN *)(ULONG_PTR)params->NumericAttributes );
}

static NTSTATUS wrap_SQLColumnPrivileges( void *args )
{
    struct SQLColumnPrivileges_params *params = args;
    return pSQLColumnPrivileges( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->CatalogName,
                                 params->NameLength1, params->SchemaName, params->NameLength2,
                                 params->TableName, params->NameLength3, params->ColumnName, params->NameLength4 );
}

static NTSTATUS wrap_SQLColumnPrivilegesW( void *args )
{
    struct SQLColumnPrivilegesW_params *params = args;
    return pSQLColumnPrivilegesW( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->CatalogName,
                                  params->NameLength1, params->SchemaName, params->NameLength2,
                                  params->TableName, params->NameLength3, params->ColumnName, params->NameLength4 );
}

static NTSTATUS wrap_SQLColumns( void *args )
{
    struct SQLColumns_params *params = args;
    return pSQLColumns( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->CatalogName, params->NameLength1,
                        params->SchemaName, params->NameLength2, params->TableName, params->NameLength3,
                        params->ColumnName, params->NameLength4 );
}

static NTSTATUS wrap_SQLColumnsW( void *args )
{
    struct SQLColumnsW_params *params = args;
    return pSQLColumnsW( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->CatalogName, params->NameLength1,
                         params->SchemaName, params->NameLength2, params->TableName, params->NameLength3,
                         params->ColumnName, params->NameLength4 );
}

static NTSTATUS wrap_SQLConnect( void *args )
{
    struct SQLConnect_params *params = args;
    return pSQLConnect( (SQLHDBC)(ULONG_PTR)params->ConnectionHandle, params->ServerName, params->NameLength1,
                        params->UserName, params->NameLength2, params->Authentication, params->NameLength3 );
}

static NTSTATUS wrap_SQLConnectW( void *args )
{
    struct SQLConnectW_params *params = args;
    return pSQLConnectW( (SQLHDBC)(ULONG_PTR)params->ConnectionHandle, params->ServerName, params->NameLength1,
                         params->UserName, params->NameLength2, params->Authentication, params->NameLength3 );
}

static NTSTATUS wrap_SQLCopyDesc( void *args )
{
    struct SQLCopyDesc_params *params = args;
    return pSQLCopyDesc( (SQLHDESC)(ULONG_PTR)params->SourceDescHandle, (SQLHDESC)(ULONG_PTR)params->TargetDescHandle );
}

static NTSTATUS wrap_SQLDescribeCol( void *args )
{
    struct SQLDescribeCol_params *params = args;
    return pSQLDescribeCol( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->ColumnNumber, params->ColumnName,
                            params->BufferLength, params->NameLength, params->DataType,
                            (SQLULEN *)(ULONG_PTR)params->ColumnSize, params->DecimalDigits, params->Nullable );
}

static NTSTATUS wrap_SQLDescribeColW( void *args )
{
    struct SQLDescribeColW_params *params = args;
    return pSQLDescribeColW( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->ColumnNumber, params->ColumnName,
                             params->BufferLength, params->NameLength, params->DataType,
                             (SQLULEN *)(ULONG_PTR)params->ColumnSize, params->DecimalDigits, params->Nullable );
}

static NTSTATUS wrap_SQLDescribeParam( void *args )
{
    struct SQLDescribeParam_params *params = args;
    return pSQLDescribeParam( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->ParameterNumber, params->DataType,
                              (SQLULEN *)(ULONG_PTR)params->ParameterSize, params->DecimalDigits, params->Nullable );
}

static NTSTATUS wrap_SQLDisconnect( void *args )
{
    struct SQLDisconnect_params *params = args;
    return pSQLDisconnect( (SQLHDBC)(ULONG_PTR)params->ConnectionHandle );
}

static NTSTATUS wrap_SQLDriverConnect( void *args )
{
    struct SQLDriverConnect_params *params = args;
    return pSQLDriverConnect( (SQLHDBC)(ULONG_PTR)params->ConnectionHandle, (SQLHWND)(ULONG_PTR)params->WindowHandle,
                              params->ConnectionString, params->Length, params->OutConnectionString,
                              params->BufferLength, params->Length2, params->DriverCompletion );
}

static NTSTATUS wrap_SQLDriverConnectW( void *args )
{
    struct SQLDriverConnectW_params *params = args;
    return pSQLDriverConnectW( (SQLHDBC)(ULONG_PTR)params->ConnectionHandle, (SQLHWND)(ULONG_PTR)params->WindowHandle,
                               params->InConnectionString, params->Length, params->OutConnectionString,
                               params->BufferLength, params->Length2, params->DriverCompletion );
}

static NTSTATUS wrap_SQLEndTran( void *args )
{
    struct SQLEndTran_params *params = args;
    return pSQLEndTran( params->HandleType, (SQLHANDLE)(ULONG_PTR)params->Handle, params->CompletionType );
}

static NTSTATUS wrap_SQLError( void *args )
{
    struct SQLError_params *params = args;
    return pSQLError( (SQLHENV)(ULONG_PTR)params->EnvironmentHandle, (SQLHDBC)(ULONG_PTR)params->ConnectionHandle,
                      (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->SqlState, params->NativeError,
                      params->MessageText, params->BufferLength, params->TextLength );
}

static NTSTATUS wrap_SQLErrorW( void *args )
{
    struct SQLErrorW_params *params = args;
    return pSQLErrorW( (SQLHENV)(ULONG_PTR)params->EnvironmentHandle, (SQLHDBC)(ULONG_PTR)params->ConnectionHandle,
                       (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->SqlState, params->NativeError,
                       params->MessageText, params->BufferLength, params->TextLength );
}

static NTSTATUS wrap_SQLExecDirect( void *args )
{
    struct SQLExecDirect_params *params = args;
    return pSQLExecDirect( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->StatementText, params->TextLength );
}

static NTSTATUS wrap_SQLExecDirectW( void *args )
{
    struct SQLExecDirectW_params *params = args;
    return pSQLExecDirectW( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->StatementText, params->TextLength );
}

static NTSTATUS wrap_SQLExecute( void *args )
{
    struct SQLExecute_params *params = args;
    return pSQLExecute( (SQLHSTMT)(ULONG_PTR)params->StatementHandle );
}

static NTSTATUS wrap_SQLExtendedFetch( void *args )
{
    struct SQLExtendedFetch_params *params = args;
    return pSQLExtendedFetch( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->FetchOrientation,
                              params->FetchOffset, (SQLULEN *)(ULONG_PTR)params->RowCount, params->RowStatusArray );
}

static NTSTATUS wrap_SQLFetch( void *args )
{
    struct SQLFetch_params *params = args;
    return pSQLFetch( (SQLHSTMT)(ULONG_PTR)params->StatementHandle );
}

static NTSTATUS wrap_SQLFetchScroll( void *args )
{
    struct SQLFetchScroll_params *params = args;
    return pSQLFetchScroll( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->FetchOrientation,
                            params->FetchOffset );
}

static NTSTATUS wrap_SQLForeignKeys( void *args )
{
    struct SQLForeignKeys_params *params = args;
    return pSQLForeignKeys( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->PkCatalogName,
                            params->NameLength1, params->PkSchemaName, params->NameLength2,
                            params->PkTableName, params->NameLength3, params->FkCatalogName,
                            params->NameLength4, params->FkSchemaName, params->NameLength5,
                            params->FkTableName, params->NameLength6 );
}

static NTSTATUS wrap_SQLForeignKeysW( void *args )
{
    struct SQLForeignKeysW_params *params = args;
    return pSQLForeignKeysW( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->PkCatalogName,
                             params->NameLength1, params->PkSchemaName, params->NameLength2,
                             params->PkTableName, params->NameLength3, params->FkCatalogName,
                             params->NameLength4, params->FkSchemaName, params->NameLength5,
                             params->FkTableName, params->NameLength6 );
}

static NTSTATUS wrap_SQLFreeHandle( void *args )
{
    struct SQLFreeHandle_params *params = args;
    return pSQLFreeHandle( params->HandleType, (SQLHANDLE)(ULONG_PTR)params->Handle );
}

static NTSTATUS wrap_SQLFreeStmt( void *args )
{
    struct SQLFreeStmt_params *params = args;
    return pSQLFreeStmt( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->Option );
}

static NTSTATUS wrap_SQLGetConnectAttr( void *args )
{
    struct SQLGetConnectAttr_params *params = args;
    return pSQLGetConnectAttr( (SQLHDBC)(ULONG_PTR)params->ConnectionHandle, params->Attribute, params->Value,
                               params->BufferLength, params->StringLength );
}

static NTSTATUS wrap_SQLGetConnectAttrW( void *args )
{
    struct SQLGetConnectAttrW_params *params = args;
    return pSQLGetConnectAttrW( (SQLHDBC)(ULONG_PTR)params->ConnectionHandle, params->Attribute, params->Value,
                                params->BufferLength, params->StringLength );
}

static NTSTATUS wrap_SQLGetConnectOption( void *args )
{
    struct SQLGetConnectOption_params *params = args;
    return pSQLGetConnectOption( (SQLHDBC)(ULONG_PTR)params->ConnectionHandle, params->Option, params->Value );
}

static NTSTATUS wrap_SQLGetConnectOptionW( void *args )
{
    struct SQLGetConnectOptionW_params *params = args;
    return pSQLGetConnectOptionW( (SQLHDBC)(ULONG_PTR)params->ConnectionHandle, params->Option, params->Value );
}

static NTSTATUS wrap_SQLGetCursorName( void *args )
{
    struct SQLGetCursorName_params *params = args;
    return pSQLGetCursorName( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->CursorName, params->BufferLength,
                              params->NameLength );
}

static NTSTATUS wrap_SQLGetCursorNameW( void *args )
{
    struct SQLGetCursorNameW_params *params = args;
    return pSQLGetCursorNameW( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->CursorName, params->BufferLength,
                               params->NameLength );
}

static NTSTATUS wrap_SQLGetData( void *args )
{
    struct SQLGetData_params *params = args;
    return pSQLGetData( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->ColumnNumber, params->TargetType,
                        params->TargetValue, params->BufferLength, (SQLLEN *)(ULONG_PTR)params->StrLen_or_Ind );
}

static NTSTATUS wrap_SQLGetDescField( void *args )
{
    struct SQLGetDescField_params *params = args;
    return pSQLGetDescField( (SQLHDESC)(ULONG_PTR)params->DescriptorHandle, params->RecNumber, params->FieldIdentifier,
                             params->Value, params->BufferLength, params->StringLength );
}

static NTSTATUS wrap_SQLGetDescFieldW( void *args )
{
    struct SQLGetDescFieldW_params *params = args;
    return pSQLGetDescFieldW( (SQLHDESC)(ULONG_PTR)params->DescriptorHandle, params->RecNumber, params->FieldIdentifier,
                              params->Value, params->BufferLength, params->StringLength );
}

static NTSTATUS wrap_SQLGetDescRec( void *args )
{
    struct SQLGetDescRec_params *params = args;
    return pSQLGetDescRec( (SQLHDESC)(ULONG_PTR)params->DescriptorHandle, params->RecNumber, params->Name,
                           params->BufferLength, params->StringLength, params->Type, params->SubType,
                           (SQLLEN *)(ULONG_PTR)params->Length, params->Precision, params->Scale, params->Nullable );
}

static NTSTATUS wrap_SQLGetDescRecW( void *args )
{
    struct SQLGetDescRecW_params *params = args;
    return pSQLGetDescRecW( (SQLHDESC)(ULONG_PTR)params->DescriptorHandle, params->RecNumber, params->Name,
                            params->BufferLength, params->StringLength, params->Type, params->SubType,
                            (SQLLEN *)(ULONG_PTR)params->Length, params->Precision, params->Scale, params->Nullable );
}

static NTSTATUS wrap_SQLGetDiagField( void *args )
{
    struct SQLGetDiagField_params *params = args;
    return pSQLGetDiagField( params->HandleType, (SQLHANDLE)(ULONG_PTR)params->Handle, params->RecNumber,
                             params->DiagIdentifier, params->DiagInfo, params->BufferLength, params->StringLength );
}

static NTSTATUS wrap_SQLGetDiagFieldW( void *args )
{
    struct SQLGetDiagFieldW_params *params = args;
    return pSQLGetDiagFieldW( params->HandleType, (SQLHANDLE)(ULONG_PTR)params->Handle, params->RecNumber,
                              params->DiagIdentifier, params->DiagInfo, params->BufferLength, params->StringLength );
}

static NTSTATUS wrap_SQLGetDiagRec( void *args )
{
    struct SQLGetDiagRec_params *params = args;
    return pSQLGetDiagRec( params->HandleType, (SQLHANDLE)(ULONG_PTR)params->Handle, params->RecNumber, params->SqlState,
                           params->NativeError, params->MessageText, params->BufferLength, params->TextLength );
}

static NTSTATUS wrap_SQLGetDiagRecW( void *args )
{
    struct SQLGetDiagRecW_params *params = args;
    return pSQLGetDiagRecW( params->HandleType, (SQLHANDLE)(ULONG_PTR)params->Handle, params->RecNumber, params->SqlState,
                            params->NativeError, params->MessageText, params->BufferLength, params->TextLength );
}

static NTSTATUS wrap_SQLGetEnvAttr( void *args )
{
    struct SQLGetEnvAttr_params *params = args;
    return pSQLGetEnvAttr( (SQLHENV)(ULONG_PTR)params->EnvironmentHandle, params->Attribute,
                           params->Value, params->BufferLength, params->StringLength );
}

static NTSTATUS wrap_SQLGetFunctions( void *args )
{
    struct SQLGetFunctions_params *params = args;
    return pSQLGetFunctions( (SQLHDBC)(ULONG_PTR)params->ConnectionHandle, params->FunctionId, params->Supported );
}

static NTSTATUS wrap_SQLGetInfo( void *args )
{
    struct SQLGetInfo_params *params = args;
    return pSQLGetInfo( (SQLHDBC)(ULONG_PTR)params->ConnectionHandle, params->InfoType, params->InfoValue,
                        params->BufferLength, params->StringLength );
}

static NTSTATUS wrap_SQLGetInfoW( void *args )
{
    struct SQLGetInfoW_params *params = args;
    return pSQLGetInfoW( (SQLHDBC)(ULONG_PTR)params->ConnectionHandle, params->InfoType, params->InfoValue,
                         params->BufferLength, params->StringLength );
}

static NTSTATUS wrap_SQLGetStmtAttr( void *args )
{
    struct SQLGetStmtAttr_params *params = args;
    return pSQLGetStmtAttr( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->Attribute, params->Value,
                            params->BufferLength, params->StringLength );
}

static NTSTATUS wrap_SQLGetStmtAttrW( void *args )
{
    struct SQLGetStmtAttrW_params *params = args;
    return pSQLGetStmtAttrW( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->Attribute, params->Value,
                             params->BufferLength, params->StringLength );
}

static NTSTATUS wrap_SQLGetStmtOption( void *args )
{
    struct SQLGetStmtOption_params *params = args;
    return pSQLGetStmtOption( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->Option, params->Value );
}

static NTSTATUS wrap_SQLGetTypeInfo( void *args )
{
    struct SQLGetTypeInfo_params *params = args;
    return pSQLGetTypeInfo( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->DataType );
}

static NTSTATUS wrap_SQLGetTypeInfoW( void *args )
{
    struct SQLGetTypeInfoW_params *params = args;
    return pSQLGetTypeInfoW( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->DataType );
}

static NTSTATUS wrap_SQLMoreResults( void *args )
{
    struct SQLMoreResults_params *params = args;
    return pSQLMoreResults( (SQLHSTMT)(ULONG_PTR)params->StatementHandle );
}

static NTSTATUS wrap_SQLNativeSql( void *args )
{
    struct SQLNativeSql_params *params = args;
    return pSQLNativeSql( (SQLHDBC)(ULONG_PTR)params->ConnectionHandle, params->InStatementText, params->TextLength1,
                          params->OutStatementText, params->BufferLength, params->TextLength2 );
}

static NTSTATUS wrap_SQLNativeSqlW( void *args )
{
    struct SQLNativeSqlW_params *params = args;
    return pSQLNativeSqlW( (SQLHDBC)(ULONG_PTR)params->ConnectionHandle, params->InStatementText, params->TextLength1,
                           params->OutStatementText, params->BufferLength, params->TextLength2 );
}

static NTSTATUS wrap_SQLNumParams( void *args )
{
    struct SQLNumParams_params *params = args;
    return pSQLNumParams( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->ParameterCount );
}

static NTSTATUS wrap_SQLNumResultCols( void *args )
{
    struct SQLNumResultCols_params *params = args;
    return pSQLNumResultCols( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->ColumnCount );
}

static NTSTATUS wrap_SQLParamData( void *args )
{
    struct SQLParamData_params *params = args;
    return pSQLParamData( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->Value );
}

static NTSTATUS wrap_SQLParamOptions( void *args )
{
    struct SQLParamOptions_params *params = args;
    return pSQLParamOptions( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->RowCount,
                             (SQLULEN *)(ULONG_PTR)params->RowNumber );
}

static NTSTATUS wrap_SQLPrepare( void *args )
{
    struct SQLPrepare_params *params = args;
    return pSQLPrepare( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->StatementText, params->TextLength );
}

static NTSTATUS wrap_SQLPrepareW( void *args )
{
    struct SQLPrepareW_params *params = args;
    return pSQLPrepareW( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->StatementText, params->TextLength );
}

static NTSTATUS wrap_SQLPrimaryKeys( void *args )
{
    struct SQLPrimaryKeys_params *params = args;
    return pSQLPrimaryKeys( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->CatalogName, params->NameLength1,
                            params->SchemaName, params->NameLength2, params->TableName, params->NameLength3 );
}

static NTSTATUS wrap_SQLPrimaryKeysW( void *args )
{
    struct SQLPrimaryKeysW_params *params = args;
    return pSQLPrimaryKeysW( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->CatalogName, params->NameLength1,
                             params->SchemaName, params->NameLength2, params->TableName, params->NameLength3 );
}

static NTSTATUS wrap_SQLProcedureColumns( void *args )
{
    struct SQLProcedureColumns_params *params = args;
    return pSQLProcedureColumns( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->CatalogName,
                                 params->NameLength1, params->SchemaName, params->NameLength2, params->ProcName,
                                 params->NameLength3, params->ColumnName, params->NameLength4 );
}

static NTSTATUS wrap_SQLProcedureColumnsW( void *args )
{
    struct SQLProcedureColumnsW_params *params = args;
    return pSQLProcedureColumnsW( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->CatalogName,
                                  params->NameLength1, params->SchemaName, params->NameLength2, params->ProcName,
                                  params->NameLength3, params->ColumnName, params->NameLength4 );
}

static NTSTATUS wrap_SQLProcedures( void *args )
{
    struct SQLProcedures_params *params = args;
    return pSQLProcedures( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->CatalogName, params->NameLength1,
                           params->SchemaName, params->NameLength2, params->ProcName, params->NameLength3 );
}

static NTSTATUS wrap_SQLProceduresW( void *args )
{
    struct SQLProceduresW_params *params = args;
    return pSQLProceduresW( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->CatalogName, params->NameLength1,
                            params->SchemaName, params->NameLength2, params->ProcName, params->NameLength3 );
}

static NTSTATUS wrap_SQLPutData( void *args )
{
    struct SQLPutData_params *params = args;
    return pSQLPutData( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->Data, params->StrLen_or_Ind );
}

static NTSTATUS wrap_SQLRowCount( void *args )
{
    struct SQLRowCount_params *params = args;
    return pSQLRowCount( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, (SQLLEN *)(ULONG_PTR)params->RowCount );
}

static NTSTATUS wrap_SQLSetConnectAttr( void *args )
{
    struct SQLSetConnectAttr_params *params = args;
    return pSQLSetConnectAttr( (SQLHDBC)(ULONG_PTR)params->ConnectionHandle, params->Attribute, params->Value,
                               params->StringLength );
}

static NTSTATUS wrap_SQLSetConnectAttrW( void *args )
{
    struct SQLSetConnectAttrW_params *params = args;
    return pSQLSetConnectAttrW( (SQLHDBC)(ULONG_PTR)params->ConnectionHandle, params->Attribute, params->Value,
                                params->StringLength );
}

static NTSTATUS wrap_SQLSetConnectOption( void *args )
{
    struct SQLSetConnectOption_params *params = args;
    return pSQLSetConnectOption( (SQLHDBC)(ULONG_PTR)params->ConnectionHandle, params->Option, params->Value );
}

static NTSTATUS wrap_SQLSetConnectOptionW( void *args )
{
    struct SQLSetConnectOptionW_params *params = args;
    return pSQLSetConnectOptionW( (SQLHDBC)(ULONG_PTR)params->ConnectionHandle, params->Option, params->Value );
}

static NTSTATUS wrap_SQLSetCursorName( void *args )
{
    struct SQLSetCursorName_params *params = args;
    return pSQLSetCursorName( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->CursorName, params->NameLength );
}

static NTSTATUS wrap_SQLSetCursorNameW( void *args )
{
    struct SQLSetCursorNameW_params *params = args;
    return pSQLSetCursorNameW( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->CursorName, params->NameLength );
}

static NTSTATUS wrap_SQLSetDescField( void *args )
{
    struct SQLSetDescField_params *params = args;
    return pSQLSetDescField( (SQLHDESC)(ULONG_PTR)params->DescriptorHandle, params->RecNumber, params->FieldIdentifier,
                             params->Value, params->BufferLength );
}

static NTSTATUS wrap_SQLSetDescFieldW( void *args )
{
    struct SQLSetDescFieldW_params *params = args;
    return pSQLSetDescFieldW( (SQLHDESC)(ULONG_PTR)params->DescriptorHandle, params->RecNumber, params->FieldIdentifier,
                              params->Value, params->BufferLength );
}

static NTSTATUS wrap_SQLSetDescRec( void *args )
{
    struct SQLSetDescRec_params *params = args;
    return pSQLSetDescRec( (SQLHDESC)(ULONG_PTR)params->DescriptorHandle, params->RecNumber, params->Type,
                           params->SubType, params->Length, params->Precision, params->Scale,
                           params->Data, (SQLLEN *)(ULONG_PTR)params->StringLength,
                           (SQLLEN *)(ULONG_PTR)params->Indicator );
}

static NTSTATUS wrap_SQLSetEnvAttr( void *args )
{
    struct SQLSetEnvAttr_params *params = args;
    return pSQLSetEnvAttr( (SQLHENV)(ULONG_PTR)params->EnvironmentHandle, params->Attribute, params->Value,
                           params->StringLength );
}

static NTSTATUS wrap_SQLSetParam( void *args )
{
    struct SQLSetParam_params *params = args;
    return pSQLSetParam( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->ParameterNumber, params->ValueType,
                         params->ParameterType, params->LengthPrecision, params->ParameterScale,
                         params->ParameterValue, (SQLLEN *)(ULONG_PTR)params->StrLen_or_Ind );
}

static NTSTATUS wrap_SQLSetPos( void *args )
{
    struct SQLSetPos_params *params = args;
    return pSQLSetPos( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->RowNumber, params->Operation,
                       params->LockType );
}

static NTSTATUS wrap_SQLSetScrollOptions( void *args )
{
    struct SQLSetScrollOptions_params *params = args;
    return pSQLSetScrollOptions( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->Concurrency,
                                 params->KeySetSize, params->RowSetSize );
}

static NTSTATUS wrap_SQLSetStmtAttr( void *args )
{
    struct SQLSetStmtAttr_params *params = args;
    return pSQLSetStmtAttr( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->Attribute, params->Value,
                            params->StringLength );
}

static NTSTATUS wrap_SQLSetStmtAttrW( void *args )
{
    struct SQLSetStmtAttrW_params *params = args;
    return pSQLSetStmtAttrW( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->Attribute, params->Value,
                             params->StringLength );
}

static NTSTATUS wrap_SQLSetStmtOption( void *args )
{
    struct SQLSetStmtOption_params *params = args;
    return pSQLSetStmtOption( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->Option, params->Value );
}

static NTSTATUS wrap_SQLSpecialColumns( void *args )
{
    struct SQLSpecialColumns_params *params = args;
    return pSQLSpecialColumns( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->IdentifierType,
                               params->CatalogName, params->NameLength1, params->SchemaName, params->NameLength2,
                               params->TableName, params->NameLength3, params->Scope, params->Nullable );
}

static NTSTATUS wrap_SQLSpecialColumnsW( void *args )
{
    struct SQLSpecialColumnsW_params *params = args;
    return pSQLSpecialColumnsW( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->IdentifierType,
                                params->CatalogName, params->NameLength1, params->SchemaName, params->NameLength2,
                                params->TableName, params->NameLength3, params->Scope, params->Nullable );
}

static NTSTATUS wrap_SQLStatistics( void *args )
{
    struct SQLStatistics_params *params = args;
    return pSQLStatistics( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->CatalogName, params->NameLength1,
                           params->SchemaName, params->NameLength2, params->TableName,
                           params->NameLength3, params->Unique, params->Reserved );
}

static NTSTATUS wrap_SQLStatisticsW( void *args )
{
    struct SQLStatisticsW_params *params = args;
    return pSQLStatisticsW( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->CatalogName, params->NameLength1,
                            params->SchemaName, params->NameLength2, params->TableName,
                            params->NameLength3, params->Unique, params->Reserved );
}

static NTSTATUS wrap_SQLTablePrivileges( void *args )
{
    struct SQLTablePrivileges_params *params = args;
    return pSQLTablePrivileges( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->CatalogName,
                                params->NameLength1, params->SchemaName, params->NameLength2, params->TableName,
                                params->NameLength3 );
}

static NTSTATUS wrap_SQLTablePrivilegesW( void *args )
{
    struct SQLTablePrivilegesW_params *params = args;
    return pSQLTablePrivilegesW( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->CatalogName,
                                 params->NameLength1, params->SchemaName, params->NameLength2, params->TableName,
                                 params->NameLength3 );
}

static NTSTATUS wrap_SQLTables( void *args )
{
    struct SQLTables_params *params = args;
    return pSQLTables( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->CatalogName, params->NameLength1,
                       params->SchemaName, params->NameLength2, params->TableName,
                       params->NameLength3, params->TableType, params->NameLength4 );
}

static NTSTATUS wrap_SQLTablesW( void *args )
{
    struct SQLTablesW_params *params = args;
    return pSQLTablesW( (SQLHSTMT)(ULONG_PTR)params->StatementHandle, params->CatalogName, params->NameLength1,
                        params->SchemaName, params->NameLength2, params->TableName,
                        params->NameLength3, params->TableType, params->NameLength4 );
}

static NTSTATUS wrap_SQLTransact( void *args )
{
    struct SQLTransact_params *params = args;
    return pSQLTransact( (SQLHENV)(ULONG_PTR)params->EnvironmentHandle, (SQLHDBC)(ULONG_PTR)params->ConnectionHandle,
                         params->CompletionType );
}

const unixlib_entry_t __wine_unix_call_funcs[] =
{
    odbc_process_attach,
    odbc_process_detach,
    wrap_SQLAllocHandle,
    wrap_SQLAllocHandleStd,
    wrap_SQLBindCol,
    wrap_SQLBindParameter,
    wrap_SQLBrowseConnect,
    wrap_SQLBrowseConnectW,
    wrap_SQLBulkOperations,
    wrap_SQLCancel,
    wrap_SQLCloseCursor,
    wrap_SQLColAttribute,
    wrap_SQLColAttributeW,
    wrap_SQLColAttributes,
    wrap_SQLColAttributesW,
    wrap_SQLColumnPrivileges,
    wrap_SQLColumnPrivilegesW,
    wrap_SQLColumns,
    wrap_SQLColumnsW,
    wrap_SQLConnect,
    wrap_SQLConnectW,
    wrap_SQLCopyDesc,
    wrap_SQLDescribeCol,
    wrap_SQLDescribeColW,
    wrap_SQLDescribeParam,
    wrap_SQLDisconnect,
    wrap_SQLDriverConnect,
    wrap_SQLDriverConnectW,
    wrap_SQLEndTran,
    wrap_SQLError,
    wrap_SQLErrorW,
    wrap_SQLExecDirect,
    wrap_SQLExecDirectW,
    wrap_SQLExecute,
    wrap_SQLExtendedFetch,
    wrap_SQLFetch,
    wrap_SQLFetchScroll,
    wrap_SQLForeignKeys,
    wrap_SQLForeignKeysW,
    wrap_SQLFreeHandle,
    wrap_SQLFreeStmt,
    wrap_SQLGetConnectAttr,
    wrap_SQLGetConnectAttrW,
    wrap_SQLGetConnectOption,
    wrap_SQLGetConnectOptionW,
    wrap_SQLGetCursorName,
    wrap_SQLGetCursorNameW,
    wrap_SQLGetData,
    wrap_SQLGetDescField,
    wrap_SQLGetDescFieldW,
    wrap_SQLGetDescRec,
    wrap_SQLGetDescRecW,
    wrap_SQLGetDiagField,
    wrap_SQLGetDiagFieldW,
    wrap_SQLGetDiagRec,
    wrap_SQLGetDiagRecW,
    wrap_SQLGetEnvAttr,
    wrap_SQLGetFunctions,
    wrap_SQLGetInfo,
    wrap_SQLGetInfoW,
    wrap_SQLGetStmtAttr,
    wrap_SQLGetStmtAttrW,
    wrap_SQLGetStmtOption,
    wrap_SQLGetTypeInfo,
    wrap_SQLGetTypeInfoW,
    wrap_SQLMoreResults,
    wrap_SQLNativeSql,
    wrap_SQLNativeSqlW,
    wrap_SQLNumParams,
    wrap_SQLNumResultCols,
    wrap_SQLParamData,
    wrap_SQLParamOptions,
    wrap_SQLPrepare,
    wrap_SQLPrepareW,
    wrap_SQLPrimaryKeys,
    wrap_SQLPrimaryKeysW,
    wrap_SQLProcedureColumns,
    wrap_SQLProcedureColumnsW,
    wrap_SQLProcedures,
    wrap_SQLProceduresW,
    wrap_SQLPutData,
    wrap_SQLRowCount,
    wrap_SQLSetConnectAttr,
    wrap_SQLSetConnectAttrW,
    wrap_SQLSetConnectOption,
    wrap_SQLSetConnectOptionW,
    wrap_SQLSetCursorName,
    wrap_SQLSetCursorNameW,
    wrap_SQLSetDescField,
    wrap_SQLSetDescFieldW,
    wrap_SQLSetDescRec,
    wrap_SQLSetEnvAttr,
    wrap_SQLSetParam,
    wrap_SQLSetPos,
    wrap_SQLSetScrollOptions,
    wrap_SQLSetStmtAttr,
    wrap_SQLSetStmtAttrW,
    wrap_SQLSetStmtOption,
    wrap_SQLSpecialColumns,
    wrap_SQLSpecialColumnsW,
    wrap_SQLStatistics,
    wrap_SQLStatisticsW,
    wrap_SQLTablePrivileges,
    wrap_SQLTablePrivilegesW,
    wrap_SQLTables,
    wrap_SQLTablesW,
    wrap_SQLTransact,
};

C_ASSERT( ARRAYSIZE( __wine_unix_call_funcs) == unix_funcs_count );

#ifdef _WIN64

typedef ULONG PTR32;

static NTSTATUS wow64_SQLBindCol( void *args )
{
    struct
    {
        UINT64 StatementHandle;
        INT16  ColumnNumber;
        INT16  TargetType;
        PTR32  TargetValue;
        INT64  BufferLength;
        PTR32  StrLen_or_Ind;
    } const *params32 = args;

    struct SQLBindCol_params params =
    {
        params32->StatementHandle,
        params32->ColumnNumber,
        params32->TargetType,
        ULongToPtr(params32->TargetValue),
        params32->BufferLength,
        ULongToPtr(params32->StrLen_or_Ind)
    };

    return wrap_SQLBindCol( &params );
}

static NTSTATUS wow64_SQLBindParameter( void *args )
{
    struct
    {
        UINT64 StatementHandle;
        UINT16 ParameterNumber;
        INT16  InputOutputType;
        INT16  ValueType;
        INT16  ParameterType;
        UINT64 ColumnSize;
        INT16  DecimalDigits;
        PTR32  ParameterValue;
        INT64  BufferLength;
        PTR32  StrLen_or_Ind;
    } const *params32 = args;

    struct SQLBindParameter_params params =
    {
        params32->StatementHandle,
        params32->ParameterNumber,
        params32->InputOutputType,
        params32->ValueType,
        params32->ParameterType,
        params32->ColumnSize,
        params32->DecimalDigits,
        ULongToPtr(params32->ParameterValue),
        params32->BufferLength,
        ULongToPtr(params32->StrLen_or_Ind)
    };

    return wrap_SQLBindParameter( &params );
}

static NTSTATUS wow64_SQLBrowseConnect( void *args )
{
    struct
    {
        UINT64 ConnectionHandle;
        PTR32  InConnectionString;
        INT16  StringLength1;
        PTR32  OutConnectionString;
        INT16  BufferLength;
        PTR32  StringLength2;
    } const *params32 = args;

    struct SQLBrowseConnect_params params =
    {
        params32->ConnectionHandle,
        ULongToPtr(params32->InConnectionString),
        params32->StringLength1,
        ULongToPtr(params32->OutConnectionString),
        params32->BufferLength,
        ULongToPtr(params32->StringLength2)
    };

    return wrap_SQLBrowseConnect( &params );
}

static NTSTATUS wow64_SQLBrowseConnectW( void *args )
{
    struct
    {
        UINT64 ConnectionHandle;
        PTR32  InConnectionString;
        INT16  StringLength1;
        PTR32  OutConnectionString;
        INT16  BufferLength;
        PTR32  StringLength2;
    } const *params32 = args;

    struct SQLBrowseConnectW_params params =
    {
        params32->ConnectionHandle,
        ULongToPtr(params32->InConnectionString),
        params32->StringLength1,
        ULongToPtr(params32->OutConnectionString),
        params32->BufferLength,
        ULongToPtr(params32->StringLength2)
    };

    return wrap_SQLBrowseConnectW( &params );
}

static NTSTATUS wow64_SQLColAttribute( void *args )
{
    struct
    {
        UINT64 StatementHandle;
        UINT16 ColumnNumber;
        UINT16 FieldIdentifier;
        PTR32  CharacterAttribute;
        INT16  BufferLength;
        PTR32  StringLength;
        PTR32  NumericAttribute;
    } const *params32 = args;

    struct SQLColAttribute_params params =
    {
        params32->StatementHandle,
        params32->ColumnNumber,
        params32->FieldIdentifier,
        ULongToPtr(params32->CharacterAttribute),
        params32->BufferLength,
        ULongToPtr(params32->StringLength),
        ULongToPtr(params32->NumericAttribute)
    };

    return wrap_SQLColAttribute( &params );
}

static NTSTATUS wow64_SQLColAttributeW( void *args )
{
    struct
    {
        UINT64 StatementHandle;
        UINT16 ColumnNumber;
        UINT16 FieldIdentifier;
        PTR32  CharacterAttribute;
        INT16  BufferLength;
        PTR32  StringLength;
        PTR32  NumericAttribute;
    } const *params32 = args;

    struct SQLColAttributeW_params params =
    {
        params32->StatementHandle,
        params32->ColumnNumber,
        params32->FieldIdentifier,
        ULongToPtr(params32->CharacterAttribute),
        params32->BufferLength,
        ULongToPtr(params32->StringLength),
        ULongToPtr(params32->NumericAttribute)
    };

    return wrap_SQLColAttributeW( &params );
}

static NTSTATUS wow64_SQLColAttributes( void *args )
{
    struct
    {
        UINT64 StatementHandle;
        UINT16 ColumnNumber;
        UINT16 FieldIdentifier;
        PTR32  CharacterAttributes;
        INT16  BufferLength;
        PTR32  StringLength;
        PTR32  NumericAttributes;
    } const *params32 = args;

    struct SQLColAttributes_params params =
    {
        params32->StatementHandle,
        params32->ColumnNumber,
        params32->FieldIdentifier,
        ULongToPtr(params32->CharacterAttributes),
        params32->BufferLength,
        ULongToPtr(params32->StringLength),
        ULongToPtr(params32->NumericAttributes)
    };

    return wrap_SQLColAttributes( &params );
}

static NTSTATUS wow64_SQLColAttributesW( void *args )
{
    struct
    {
        UINT64 StatementHandle;
        UINT16 ColumnNumber;
        UINT16 FieldIdentifier;
        PTR32  CharacterAttributes;
        INT16  BufferLength;
        PTR32  StringLength;
        PTR32  NumericAttributes;
    } const *params32 = args;

    struct SQLColAttributesW_params params =
    {
        params32->StatementHandle,
        params32->ColumnNumber,
        params32->FieldIdentifier,
        ULongToPtr(params32->CharacterAttributes),
        params32->BufferLength,
        ULongToPtr(params32->StringLength),
        ULongToPtr(params32->NumericAttributes)
    };

    return wrap_SQLColAttributesW( &params );
}

static NTSTATUS wow64_SQLColumnPrivileges( void *args )
{
    struct
    {
        UINT64 StatementHandle;
        PTR32  CatalogName;
        INT16  NameLength1;
        PTR32  SchemaName;
        INT16  NameLength2;
        PTR32  TableName;
        INT16  NameLength3;
        PTR32  ColumnName;
        INT16  NameLength4;
    } const *params32 = args;

    struct SQLColumnPrivileges_params params =
    {
        params32->StatementHandle,
        ULongToPtr(params32->CatalogName),
        params32->NameLength1,
        ULongToPtr(params32->SchemaName),
        params32->NameLength2,
        ULongToPtr(params32->TableName),
        params32->NameLength3,
        ULongToPtr(params32->ColumnName),
        params32->NameLength4
    };

    return wrap_SQLColumnPrivileges( &params );
}

static NTSTATUS wow64_SQLColumnPrivilegesW( void *args )
{
    struct
    {
        UINT64 StatementHandle;
        PTR32  CatalogName;
        INT16  NameLength1;
        PTR32  SchemaName;
        INT16  NameLength2;
        PTR32  TableName;
        INT16  NameLength3;
        PTR32  ColumnName;
        INT16  NameLength4;
    } const *params32 = args;

    struct SQLColumnPrivilegesW_params params =
    {
        params32->StatementHandle,
        ULongToPtr(params32->CatalogName),
        params32->NameLength1,
        ULongToPtr(params32->SchemaName),
        params32->NameLength2,
        ULongToPtr(params32->TableName),
        params32->NameLength3,
        ULongToPtr(params32->ColumnName),
        params32->NameLength4
    };

    return wrap_SQLColumnPrivilegesW( &params );
}

static NTSTATUS wow64_SQLColumns( void *args )
{
    struct
    {
        UINT64 StatementHandle;
        PTR32  CatalogName;
        INT16  NameLength1;
        PTR32  SchemaName;
        INT16  NameLength2;
        PTR32  TableName;
        INT16  NameLength3;
        PTR32  ColumnName;
        INT16  NameLength4;
    } const *params32 = args;

    struct SQLColumns_params params =
    {
        params32->StatementHandle,
        ULongToPtr(params32->CatalogName),
        params32->NameLength1,
        ULongToPtr(params32->SchemaName),
        params32->NameLength2,
        ULongToPtr(params32->TableName),
        params32->NameLength3,
        ULongToPtr(params32->ColumnName),
        params32->NameLength4
    };

    return wrap_SQLColumns( &params );
}

static NTSTATUS wow64_SQLColumnsW( void *args )
{
    struct
    {
        UINT64 StatementHandle;
        PTR32  CatalogName;
        INT16  NameLength1;
        PTR32  SchemaName;
        INT16  NameLength2;
        PTR32  TableName;
        INT16  NameLength3;
        PTR32  ColumnName;
        INT16  NameLength4;
    } const *params32 = args;

    struct SQLColumnsW_params params =
    {
        params32->StatementHandle,
        ULongToPtr(params32->CatalogName),
        params32->NameLength1,
        ULongToPtr(params32->SchemaName),
        params32->NameLength2,
        ULongToPtr(params32->TableName),
        params32->NameLength3,
        ULongToPtr(params32->ColumnName),
        params32->NameLength4
    };

    return wrap_SQLColumnsW( &params );
}

static NTSTATUS wow64_SQLConnect( void *args )
{
    struct
    {
        UINT64 ConnectionHandle;
        PTR32  ServerName;
        INT16  NameLength1;
        PTR32  UserName;
        INT16  NameLength2;
        PTR32  Authentication;
        INT16  NameLength3;
    } const *params32 = args;

    struct SQLConnect_params params =
    {
        params32->ConnectionHandle,
        ULongToPtr(params32->ServerName),
        params32->NameLength1,
        ULongToPtr(params32->UserName),
        params32->NameLength2,
        ULongToPtr(params32->Authentication),
        params32->NameLength3
    };

    return wrap_SQLConnect( &params );
}

static NTSTATUS wow64_SQLConnectW( void *args )
{
    struct
    {
        UINT64 ConnectionHandle;
        PTR32  ServerName;
        INT16  NameLength1;
        PTR32  UserName;
        INT16  NameLength2;
        PTR32  Authentication;
        INT16  NameLength3;
    } const *params32 = args;

    struct SQLConnectW_params params =
    {
        params32->ConnectionHandle,
        ULongToPtr(params32->ServerName),
        params32->NameLength1,
        ULongToPtr(params32->UserName),
        params32->NameLength2,
        ULongToPtr(params32->Authentication),
        params32->NameLength3
    };

    return wrap_SQLConnectW( &params );
}

static NTSTATUS wow64_SQLDescribeCol( void *args )
{
    struct
    {
        UINT64 StatementHandle;
        INT16  ColumnNumber;
        PTR32  ColumnName;
        INT16  BufferLength;
        PTR32  NameLength;
        PTR32  DataType;
        PTR32  ColumnSize;
        PTR32  DecimalDigits;
        PTR32  Nullable;
    } const *params32 = args;

    struct SQLDescribeCol_params params =
    {
        params32->StatementHandle,
        params32->ColumnNumber,
        ULongToPtr(params32->ColumnName),
        params32->BufferLength,
        ULongToPtr(params32->NameLength),
        ULongToPtr(params32->DataType),
        ULongToPtr(params32->ColumnSize),
        ULongToPtr(params32->DecimalDigits),
        ULongToPtr(params32->Nullable)
    };

    return wrap_SQLDescribeCol( &params );
}

static NTSTATUS wow64_SQLDescribeColW( void *args )
{
    struct
    {
        UINT64 StatementHandle;
        INT16  ColumnNumber;
        PTR32  ColumnName;
        INT16  BufferLength;
        PTR32  NameLength;
        PTR32  DataType;
        PTR32  ColumnSize;
        PTR32  DecimalDigits;
        PTR32  Nullable;
    } const *params32 = args;

    struct SQLDescribeColW_params params =
    {
        params32->StatementHandle,
        params32->ColumnNumber,
        ULongToPtr(params32->ColumnName),
        params32->BufferLength,
        ULongToPtr(params32->NameLength),
        ULongToPtr(params32->DataType),
        ULongToPtr(params32->ColumnSize),
        ULongToPtr(params32->DecimalDigits),
        ULongToPtr(params32->Nullable)
    };

    return wrap_SQLDescribeColW( &params );
}

static NTSTATUS wow64_SQLDescribeParam( void *args )
{
    struct
    {
        UINT64 StatementHandle;
        INT16  ParameterNumber;
        PTR32  DataType;
        PTR32  ParameterSize;
        PTR32  DecimalDigits;
        PTR32  Nullable;
    } const *params32 = args;

    struct SQLDescribeParam_params params =
    {
        params32->StatementHandle,
        params32->ParameterNumber,
        ULongToPtr(params32->DataType),
        ULongToPtr(params32->ParameterSize),
        ULongToPtr(params32->DecimalDigits),
        ULongToPtr(params32->Nullable)
    };

    return wrap_SQLDescribeParam( &params );
}

static NTSTATUS wow64_SQLDriverConnect( void *args )
{
    struct
    {
        UINT64 ConnectionHandle;
        PTR32  WindowHandle;
        PTR32  InConnectionString;
        INT16  Length;
        PTR32  OutConnectionString;
        INT16  BufferLength;
        PTR32  Length2;
        UINT16 DriverCompletion;
    } const *params32 = args;

    struct SQLDriverConnect_params params =
    {
        params32->ConnectionHandle,
        ULongToPtr(params32->WindowHandle),
        ULongToPtr(params32->InConnectionString),
        params32->Length,
        ULongToPtr(params32->OutConnectionString),
        params32->BufferLength,
        ULongToPtr(params32->Length2),
        params32->DriverCompletion,
    };

    return wrap_SQLDriverConnect( &params );
}

static NTSTATUS wow64_SQLDriverConnectW( void *args )
{
    struct
    {
        UINT64 ConnectionHandle;
        PTR32  WindowHandle;
        PTR32  InConnectionString;
        INT16  Length;
        PTR32  OutConnectionString;
        INT16  BufferLength;
        PTR32  Length2;
        UINT16 DriverCompletion;
    } const *params32 = args;

    struct SQLDriverConnectW_params params =
    {
        params32->ConnectionHandle,
        ULongToPtr(params32->WindowHandle),
        ULongToPtr(params32->InConnectionString),
        params32->Length,
        ULongToPtr(params32->OutConnectionString),
        params32->BufferLength,
        ULongToPtr(params32->Length2),
        params32->DriverCompletion,
    };

    return wrap_SQLDriverConnectW( &params );
}

static NTSTATUS wow64_SQLError( void *args )
{
    struct
    {
        UINT64 EnvironmentHandle;
        UINT64 ConnectionHandle;
        UINT64 StatementHandle;
        PTR32  SqlState;
        PTR32  NativeError;
        PTR32  MessageText;
        INT16  BufferLength;
        PTR32  TextLength;
    } const *params32 = args;

    struct SQLError_params params =
    {
        params32->EnvironmentHandle,
        params32->ConnectionHandle,
        params32->StatementHandle,
        ULongToPtr(params32->SqlState),
        ULongToPtr(params32->NativeError),
        ULongToPtr(params32->MessageText),
        params32->BufferLength,
        ULongToPtr(params32->TextLength)
    };

    return wrap_SQLError( &params );
}

static NTSTATUS wow64_SQLErrorW( void *args )
{
    struct
    {
        UINT64 EnvironmentHandle;
        UINT64 ConnectionHandle;
        UINT64 StatementHandle;
        PTR32  SqlState;
        PTR32  NativeError;
        PTR32  MessageText;
        INT16  BufferLength;
        PTR32  TextLength;
    } const *params32 = args;

    struct SQLErrorW_params params =
    {
        params32->EnvironmentHandle,
        params32->ConnectionHandle,
        params32->StatementHandle,
        ULongToPtr(params32->SqlState),
        ULongToPtr(params32->NativeError),
        ULongToPtr(params32->MessageText),
        params32->BufferLength,
        ULongToPtr(params32->TextLength)
    };

    return wrap_SQLErrorW( &params );
}

static NTSTATUS wow64_SQLExecDirect( void *args )
{
    struct
    {
        UINT64 StatementHandle;
        PTR32  StatementText;
        INT32  TextLength;
    } const *params32 = args;

    struct SQLExecDirect_params params =
    {
        params32->StatementHandle,
        ULongToPtr(params32->StatementText),
        params32->TextLength,
    };

    return wrap_SQLExecDirect( &params );
}

static NTSTATUS wow64_SQLExecDirectW( void *args )
{
    struct
    {
        UINT64 StatementHandle;
        PTR32  StatementText;
        INT32  TextLength;
    } const *params32 = args;

    struct SQLExecDirectW_params params =
    {
        params32->StatementHandle,
        ULongToPtr(params32->StatementText),
        params32->TextLength,
    };

    return wrap_SQLExecDirectW( &params );
}

static NTSTATUS wow64_SQLExtendedFetch( void *args )
{
    struct
    {
        UINT64 StatementHandle;
        INT16  FetchOrientation;
        INT64  FetchOffset;
        UINT64 RowCount;
        PTR32  RowStatusArray;
    } const *params32 = args;

    struct SQLExtendedFetch_params params =
    {
        params32->StatementHandle,
        params32->FetchOrientation,
        params32->FetchOffset,
        ULongToPtr(params32->RowCount),
        ULongToPtr(params32->RowStatusArray)
    };

    return wrap_SQLExtendedFetch( &params );
}

static NTSTATUS wow64_SQLForeignKeys( void *args )
{
    struct
    {
        UINT64 StatementHandle;
        PTR32  PkCatalogName;
        INT16  NameLength1;
        PTR32  PkSchemaName;
        INT16  NameLength2;
        PTR32  PkTableName;
        INT16  NameLength3;
        PTR32  FkCatalogName;
        INT16  NameLength4;
        PTR32  FkSchemaName;
        INT16  NameLength5;
        PTR32  FkTableName;
        INT16  NameLength6;
    } const *params32 = args;

    struct SQLForeignKeys_params params =
    {
        params32->StatementHandle,
        ULongToPtr(params32->PkCatalogName),
        params32->NameLength1,
        ULongToPtr(params32->PkSchemaName),
        params32->NameLength2,
        ULongToPtr(params32->PkTableName),
        params32->NameLength3,
        ULongToPtr(params32->FkCatalogName),
        params32->NameLength4,
        ULongToPtr(params32->FkSchemaName),
        params32->NameLength5,
        ULongToPtr(params32->FkTableName),
        params32->NameLength6
    };

    return wrap_SQLForeignKeys( &params );
}

static NTSTATUS wow64_SQLForeignKeysW( void *args )
{
    struct
    {
        UINT64 StatementHandle;
        PTR32  PkCatalogName;
        INT16  NameLength1;
        PTR32  PkSchemaName;
        INT16  NameLength2;
        PTR32  PkTableName;
        INT16  NameLength3;
        PTR32  FkCatalogName;
        INT16  NameLength4;
        PTR32  FkSchemaName;
        INT16  NameLength5;
        PTR32  FkTableName;
        INT16  NameLength6;
    } const *params32 = args;

    struct SQLForeignKeysW_params params =
    {
        params32->StatementHandle,
        ULongToPtr(params32->PkCatalogName),
        params32->NameLength1,
        ULongToPtr(params32->PkSchemaName),
        params32->NameLength2,
        ULongToPtr(params32->PkTableName),
        params32->NameLength3,
        ULongToPtr(params32->FkCatalogName),
        params32->NameLength4,
        ULongToPtr(params32->FkSchemaName),
        params32->NameLength5,
        ULongToPtr(params32->FkTableName),
        params32->NameLength6
    };

    return wrap_SQLForeignKeysW( &params );
}

static NTSTATUS wow64_SQLGetConnectAttr( void *args )
{
    struct
    {
        UINT64 ConnectionHandle;
        INT32  Attribute;
        PTR32  Value;
        INT32  BufferLength;
        PTR32  StringLength;
    } const *params32 = args;

    struct SQLGetConnectAttr_params params =
    {
        params32->ConnectionHandle,
        params32->Attribute,
        ULongToPtr(params32->Value),
        params32->BufferLength,
        ULongToPtr(params32->StringLength)
    };

    return wrap_SQLGetConnectAttr( &params );
}

static NTSTATUS wow64_SQLGetConnectAttrW( void *args )
{
    struct
    {
        UINT64 ConnectionHandle;
        INT32  Attribute;
        PTR32  Value;
        INT32  BufferLength;
        PTR32  StringLength;
    } const *params32 = args;

    struct SQLGetConnectAttrW_params params =
    {
        params32->ConnectionHandle,
        params32->Attribute,
        ULongToPtr(params32->Value),
        params32->BufferLength,
        ULongToPtr(params32->StringLength)
    };

    return wrap_SQLGetConnectAttrW( &params );
}

static NTSTATUS wow64_SQLGetConnectOption( void *args )
{
    struct
    {
        UINT64 ConnectionHandle;
        INT16  Option;
        PTR32  Value;
    } const *params32 = args;

    struct SQLGetConnectOption_params params =
    {
        params32->ConnectionHandle,
        params32->Option,
        ULongToPtr(params32->Value)
    };

    return wrap_SQLGetConnectOption( &params );
}

static NTSTATUS wow64_SQLGetConnectOptionW( void *args )
{
    struct
    {
        UINT64 ConnectionHandle;
        INT16  Option;
        PTR32  Value;
    } const *params32 = args;

    struct SQLGetConnectOptionW_params params =
    {
        params32->ConnectionHandle,
        params32->Option,
        ULongToPtr(params32->Value)
    };

    return wrap_SQLGetConnectOptionW( &params );
}

static NTSTATUS wow64_SQLGetCursorName( void *args )
{
    struct
    {
        UINT64 StatementHandle;
        PTR32  CursorName;
        INT16  BufferLength;
        PTR32  NameLength;
    } const *params32 = args;

    struct SQLGetCursorName_params params =
    {
        params32->StatementHandle,
        ULongToPtr(params32->CursorName),
        params32->BufferLength,
        ULongToPtr(params32->NameLength)
    };

    return wrap_SQLGetCursorName( &params );
}

static NTSTATUS wow64_SQLGetCursorNameW( void *args )
{
    struct
    {
        UINT64 StatementHandle;
        PTR32  CursorName;
        INT16  BufferLength;
        PTR32  NameLength;
    } const *params32 = args;

    struct SQLGetCursorNameW_params params =
    {
        params32->StatementHandle,
        ULongToPtr(params32->CursorName),
        params32->BufferLength,
        ULongToPtr(params32->NameLength)
    };

    return wrap_SQLGetCursorNameW( &params );
}

static NTSTATUS wow64_SQLGetData( void *args )
{
    struct
    {
        UINT64 StatementHandle;
        INT16  ColumnNumber;
        INT16  TargetType;
        PTR32  TargetValue;
        INT64  BufferLength;
        PTR32  StrLen_or_Ind;
    } const *params32 = args;

    struct SQLGetData_params params =
    {
        params32->StatementHandle,
        params32->ColumnNumber,
        params32->TargetType,
        ULongToPtr(params32->TargetValue),
        params32->BufferLength,
        ULongToPtr(params32->StrLen_or_Ind)
    };

    return wrap_SQLGetData( &params );
}

static NTSTATUS wow64_SQLGetDescField( void *args )
{
    struct
    {
        UINT64 DescriptorHandle;
        INT16  RecNumber;
        INT16  FieldIdentifier;
        PTR32  Value;
        INT32  BufferLength;
        PTR32  StringLength;
    } const *params32 = args;

    struct SQLGetDescField_params params =
    {
        params32->DescriptorHandle,
        params32->RecNumber,
        params32->FieldIdentifier,
        ULongToPtr(params32->Value),
        params32->BufferLength,
        ULongToPtr(params32->StringLength)
    };

    return wrap_SQLGetDescField( &params );
}

static NTSTATUS wow64_SQLGetDescFieldW( void *args )
{
    struct
    {
        UINT64 DescriptorHandle;
        INT16  RecNumber;
        INT16  FieldIdentifier;
        PTR32  Value;
        INT32  BufferLength;
        PTR32  StringLength;
    } const *params32 = args;

    struct SQLGetDescFieldW_params params =
    {
        params32->DescriptorHandle,
        params32->RecNumber,
        params32->FieldIdentifier,
        ULongToPtr(params32->Value),
        params32->BufferLength,
        ULongToPtr(params32->StringLength)
    };

    return wrap_SQLGetDescFieldW( &params );
}

static NTSTATUS wow64_SQLGetDescRec( void *args )
{
    struct
    {
        UINT64 DescriptorHandle;
        INT16  RecNumber;
        PTR32  Name;
        INT16  BufferLength;
        PTR32  StringLength;
        PTR32  Type;
        PTR32  SubType;
        PTR32  Length;
        PTR32  Precision;
        PTR32  Scale;
        PTR32  Nullable;
    } const *params32 = args;

    struct SQLGetDescRec_params params =
    {
        params32->DescriptorHandle,
        params32->RecNumber,
        ULongToPtr(params32->Name),
        params32->BufferLength,
        ULongToPtr(params32->StringLength),
        ULongToPtr(params32->Type),
        ULongToPtr(params32->SubType),
        ULongToPtr(params32->Length),
        ULongToPtr(params32->Precision),
        ULongToPtr(params32->Scale),
        ULongToPtr(params32->Nullable)
    };

    return wrap_SQLGetDescRec( &params );
}

static NTSTATUS wow64_SQLGetDescRecW( void *args )
{
    struct
    {
        UINT64 DescriptorHandle;
        INT16  RecNumber;
        PTR32  Name;
        INT16  BufferLength;
        PTR32  StringLength;
        PTR32  Type;
        PTR32  SubType;
        PTR32  Length;
        PTR32  Precision;
        PTR32  Scale;
        PTR32  Nullable;
    } const *params32 = args;

    struct SQLGetDescRecW_params params =
    {
        params32->DescriptorHandle,
        params32->RecNumber,
        ULongToPtr(params32->Name),
        params32->BufferLength,
        ULongToPtr(params32->StringLength),
        ULongToPtr(params32->Type),
        ULongToPtr(params32->SubType),
        ULongToPtr(params32->Length),
        ULongToPtr(params32->Precision),
        ULongToPtr(params32->Scale),
        ULongToPtr(params32->Nullable)
    };

    return wrap_SQLGetDescRecW( &params );
}

static NTSTATUS wow64_SQLGetDiagRec( void *args )
{
    struct
    {
        INT16  HandleType;
        UINT64 Handle;
        INT16  RecNumber;
        PTR32  SqlState;
        PTR32  NativeError;
        PTR32  MessageText;
        INT16  BufferLength;
        PTR32  TextLength;
    } const *params32 = args;

    struct SQLGetDiagRec_params params =
    {
        params32->HandleType,
        params32->Handle,
        params32->RecNumber,
        ULongToPtr(params32->SqlState),
        ULongToPtr(params32->NativeError),
        ULongToPtr(params32->MessageText),
        params32->BufferLength,
        ULongToPtr(params32->TextLength)
    };

    return wrap_SQLGetDiagRec( &params );
}

static NTSTATUS wow64_SQLGetDiagRecW( void *args )
{
    struct
    {
        INT16  HandleType;
        UINT64 Handle;
        INT16  RecNumber;
        PTR32  SqlState;
        PTR32  NativeError;
        PTR32  MessageText;
        INT16  BufferLength;
        PTR32  TextLength;
    } const *params32 = args;

    struct SQLGetDiagRecW_params params =
    {
        params32->HandleType,
        params32->Handle,
        params32->RecNumber,
        ULongToPtr(params32->SqlState),
        ULongToPtr(params32->NativeError),
        ULongToPtr(params32->MessageText),
        params32->BufferLength,
        ULongToPtr(params32->TextLength)
    };

    return wrap_SQLGetDiagRecW( &params );
}

static NTSTATUS wow64_SQLGetDiagField( void *args )
{
    struct
    {
        INT16  HandleType;
        UINT64 Handle;
        INT16  RecNumber;
        INT16  DiagIdentifier;
        PTR32  DiagInfo;
        INT16  BufferLength;
        PTR32  StringLength;
    } const *params32 = args;

    struct SQLGetDiagField_params params =
    {
        params32->HandleType,
        params32->Handle,
        params32->RecNumber,
        params32->DiagIdentifier,
        ULongToPtr(params32->DiagInfo),
        params32->BufferLength,
        ULongToPtr(params32->StringLength)
    };

    return wrap_SQLGetDiagField( &params );
}

static NTSTATUS wow64_SQLGetDiagFieldW( void *args )
{
    struct
    {
        INT16  HandleType;
        UINT64 Handle;
        INT16  RecNumber;
        INT16  DiagIdentifier;
        PTR32  DiagInfo;
        INT16  BufferLength;
        PTR32  StringLength;
    } const *params32 = args;

    struct SQLGetDiagFieldW_params params =
    {
        params32->HandleType,
        params32->Handle,
        params32->RecNumber,
        params32->DiagIdentifier,
        ULongToPtr(params32->DiagInfo),
        params32->BufferLength,
        ULongToPtr(params32->StringLength)
    };

    return wrap_SQLGetDiagFieldW( &params );
}

static NTSTATUS wow64_SQLGetEnvAttr( void *args )
{
    struct
    {
        UINT64 EnvironmentHandle;
        INT32  Attribute;
        PTR32  Value;
        INT32  BufferLength;
        PTR32  StringLength;
    } const *params32 = args;

    struct SQLGetEnvAttr_params params =
    {
        params32->EnvironmentHandle,
        params32->Attribute,
        ULongToPtr(params32->Value),
        params32->BufferLength,
        ULongToPtr(params32->StringLength)
    };

    return wrap_SQLGetEnvAttr( &params );
}

static NTSTATUS wow64_SQLGetFunctions( void *args )
{
    struct
    {
        UINT64 ConnectionHandle;
        UINT16 FunctionId;
        PTR32  Supported;
    } const *params32 = args;

    struct SQLGetFunctions_params params =
    {
        params32->ConnectionHandle,
        params32->FunctionId,
        ULongToPtr(params32->Supported)
    };

    return wrap_SQLGetFunctions( &params );
}

static NTSTATUS wow64_SQLGetInfo( void *args )
{
    struct
    {
        UINT64 ConnectionHandle;
        UINT16 InfoType;
        PTR32  InfoValue;
        INT16  BufferLength;
        PTR32  StringLength;
    } const *params32 = args;

    struct SQLGetInfo_params params =
    {
        params32->ConnectionHandle,
        params32->InfoType,
        ULongToPtr(params32->InfoValue),
        params32->BufferLength,
        ULongToPtr(params32->StringLength)
    };

    return wrap_SQLGetInfo( &params );
}

static NTSTATUS wow64_SQLGetInfoW( void *args )
{
    struct
    {
        UINT64 ConnectionHandle;
        UINT16 InfoType;
        PTR32  InfoValue;
        INT16  BufferLength;
        PTR32  StringLength;
    } const *params32 = args;

    struct SQLGetInfoW_params params =
    {
        params32->ConnectionHandle,
        params32->InfoType,
        ULongToPtr(params32->InfoValue),
        params32->BufferLength,
        ULongToPtr(params32->StringLength)
    };

    return wrap_SQLGetInfoW( &params );
}

static NTSTATUS wow64_SQLGetStmtOption( void *args )
{
    struct
    {
        UINT64 StatementHandle;
        UINT16 Option;
        PTR32  Value;
    } const *params32 = args;

    struct SQLGetStmtOption_params params =
    {
        params32->StatementHandle,
        params32->Option,
        ULongToPtr(params32->Value)
    };

    return wrap_SQLGetStmtOption( &params );
}

static NTSTATUS wow64_SQLNativeSql( void *args )
{
    struct
    {
        UINT64 ConnectionHandle;
        PTR32  InStatementText;
        INT32  TextLength1;
        PTR32  OutStatementText;
        INT32  BufferLength;
        PTR32  TextLength2;
    } const *params32 = args;

    struct SQLNativeSql_params params =
    {
        params32->ConnectionHandle,
        ULongToPtr(params32->InStatementText),
        params32->TextLength1,
        ULongToPtr(params32->OutStatementText),
        params32->BufferLength,
        ULongToPtr(params32->TextLength2)
    };

    return wrap_SQLNativeSql( &params );
}

static NTSTATUS wow64_SQLNativeSqlW( void *args )
{
    struct
    {
        UINT64 ConnectionHandle;
        PTR32  InStatementText;
        INT32  TextLength1;
        PTR32  OutStatementText;
        INT32  BufferLength;
        PTR32  TextLength2;
    } const *params32 = args;

    struct SQLNativeSqlW_params params =
    {
        params32->ConnectionHandle,
        ULongToPtr(params32->InStatementText),
        params32->TextLength1,
        ULongToPtr(params32->OutStatementText),
        params32->BufferLength,
        ULongToPtr(params32->TextLength2)
    };

    return wrap_SQLNativeSqlW( &params );
}

static NTSTATUS wow64_SQLNumParams( void *args )
{
    struct
    {
        UINT64 StatementHandle;
        PTR32  ParameterCount;
    } const *params32 = args;

    struct SQLNumParams_params params =
    {
        params32->StatementHandle,
        ULongToPtr(params32->ParameterCount)
    };

    return wrap_SQLNumParams( &params );
}

static NTSTATUS wow64_SQLNumResultCols( void *args )
{
    struct
    {
        UINT64 StatementHandle;
        PTR32  ColumnCount;
    } const *params32 = args;

    struct SQLNumResultCols_params params =
    {
        params32->StatementHandle,
        ULongToPtr(params32->ColumnCount)
    };

    return wrap_SQLNumResultCols( &params );
}

static NTSTATUS wow64_SQLParamData( void *args )
{
    struct
    {
        UINT64 StatementHandle;
        PTR32  Value;
    } const *params32 = args;

    struct SQLParamData_params params =
    {
        params32->StatementHandle,
        ULongToPtr(params32->Value)
    };

    return wrap_SQLParamData( &params );
}

static NTSTATUS wow64_SQLParamOptions( void *args )
{
    struct
    {
        UINT64 StatementHandle;
        UINT64 RowCount;
        PTR32  RowNumber;
    } const *params32 = args;

    struct SQLParamOptions_params params =
    {
        params32->StatementHandle,
        params32->RowCount,
        ULongToPtr(params32->RowNumber)
    };

    return wrap_SQLParamOptions( &params );
}

static NTSTATUS wow64_SQLPrepare( void *args )
{
    struct
    {
        UINT64 StatementHandle;
        PTR32  StatementText;
        INT32  TextLength;
    } const *params32 = args;

    struct SQLPrepare_params params =
    {
        params32->StatementHandle,
        ULongToPtr(params32->StatementText),
        params32->TextLength,
    };

    return wrap_SQLPrepare( &params );
}

static NTSTATUS wow64_SQLPrepareW( void *args )
{
    struct
    {
        UINT64 StatementHandle;
        PTR32  StatementText;
        INT32  TextLength;
    } const *params32 = args;

    struct SQLPrepareW_params params =
    {
        params32->StatementHandle,
        ULongToPtr(params32->StatementText),
        params32->TextLength,
    };

    return wrap_SQLPrepareW( &params );
}

static NTSTATUS wow64_SQLPrimaryKeys( void *args )
{
    struct
    {
        UINT64 StatementHandle;
        PTR32  CatalogName;
        INT16  NameLength1;
        PTR32  SchemaName;
        INT16  NameLength2;
        PTR32  TableName;
        INT16  NameLength3;
    } const *params32 = args;

    struct SQLPrimaryKeys_params params =
    {
        params32->StatementHandle,
        ULongToPtr(params32->CatalogName),
        params32->NameLength1,
        ULongToPtr(params32->SchemaName),
        params32->NameLength2,
        ULongToPtr(params32->TableName),
        params32->NameLength3
    };

    return wrap_SQLPrimaryKeys( &params );
}

static NTSTATUS wow64_SQLPrimaryKeysW( void *args )
{
    struct
    {
        UINT64 StatementHandle;
        PTR32  CatalogName;
        INT16  NameLength1;
        PTR32  SchemaName;
        INT16  NameLength2;
        PTR32  TableName;
        INT16  NameLength3;
    } const *params32 = args;

    struct SQLPrimaryKeysW_params params =
    {
        params32->StatementHandle,
        ULongToPtr(params32->CatalogName),
        params32->NameLength1,
        ULongToPtr(params32->SchemaName),
        params32->NameLength2,
        ULongToPtr(params32->TableName),
        params32->NameLength3
    };

    return wrap_SQLPrimaryKeysW( &params );
}

static NTSTATUS wow64_SQLProcedureColumns( void *args )
{
    struct
    {
        UINT64 StatementHandle;
        PTR32  CatalogName;
        INT16  NameLength1;
        PTR32  SchemaName;
        INT16  NameLength2;
        PTR32  ProcName;
        INT16  NameLength3;
        PTR32  ColumnName;
        INT16  NameLength4;
    } const *params32 = args;

    struct SQLProcedureColumns_params params =
    {
        params32->StatementHandle,
        ULongToPtr(params32->CatalogName),
        params32->NameLength1,
        ULongToPtr(params32->SchemaName),
        params32->NameLength2,
        ULongToPtr(params32->ProcName),
        params32->NameLength3,
        ULongToPtr(params32->ColumnName),
        params32->NameLength4
    };

    return wrap_SQLProcedureColumns( &params );
}

static NTSTATUS wow64_SQLProcedureColumnsW( void *args )
{
    struct
    {
        UINT64 StatementHandle;
        PTR32  CatalogName;
        INT16  NameLength1;
        PTR32  SchemaName;
        INT16  NameLength2;
        PTR32  ProcName;
        INT16  NameLength3;
        PTR32  ColumnName;
        INT16  NameLength4;
    } const *params32 = args;

    struct SQLProcedureColumnsW_params params =
    {
        params32->StatementHandle,
        ULongToPtr(params32->CatalogName),
        params32->NameLength1,
        ULongToPtr(params32->SchemaName),
        params32->NameLength2,
        ULongToPtr(params32->ProcName),
        params32->NameLength3,
        ULongToPtr(params32->ColumnName),
        params32->NameLength4
    };

    return wrap_SQLProcedureColumnsW( &params );
}

static NTSTATUS wow64_SQLProcedures( void *args )
{
    struct
    {
        UINT64 StatementHandle;
        PTR32  CatalogName;
        INT16  NameLength1;
        PTR32  SchemaName;
        INT16  NameLength2;
        PTR32  ProcName;
        INT16  NameLength3;
    } const *params32 = args;

    struct SQLProcedures_params params =
    {
        params32->StatementHandle,
        ULongToPtr(params32->CatalogName),
        params32->NameLength1,
        ULongToPtr(params32->SchemaName),
        params32->NameLength2,
        ULongToPtr(params32->ProcName),
        params32->NameLength3
    };

    return wrap_SQLProcedures( &params );
}

static NTSTATUS wow64_SQLProceduresW( void *args )
{
    struct
    {
        UINT64 StatementHandle;
        PTR32  CatalogName;
        INT16  NameLength1;
        PTR32  SchemaName;
        INT16  NameLength2;
        PTR32  ProcName;
        INT16  NameLength3;
    } const *params32 = args;

    struct SQLProceduresW_params params =
    {
        params32->StatementHandle,
        ULongToPtr(params32->CatalogName),
        params32->NameLength1,
        ULongToPtr(params32->SchemaName),
        params32->NameLength2,
        ULongToPtr(params32->ProcName),
        params32->NameLength3
    };

    return wrap_SQLProceduresW( &params );
}

static NTSTATUS wow64_SQLPutData( void *args )
{
    struct
    {
        UINT64 StatementHandle;
        PTR32  Data;
        INT64  StrLen_or_Ind;
    } const *params32 = args;

    struct SQLPutData_params params =
    {
        params32->StatementHandle,
        ULongToPtr(params32->Data),
        params32->StrLen_or_Ind
    };

    return wrap_SQLPutData( &params );
}

static NTSTATUS wow64_SQLRowCount( void *args )
{
    struct
    {
        UINT64 StatementHandle;
        PTR32  RowCount;
    } const *params32 = args;

    struct SQLRowCount_params params =
    {
        params32->StatementHandle,
        ULongToPtr(params32->RowCount)
    };

    return wrap_SQLRowCount( &params );
}

static NTSTATUS wow64_SQLSetConnectAttr( void *args )
{
    struct
    {
        UINT64 ConnectionHandle;
        INT32  Attribute;
        PTR32  Value;
        INT32  StringLength;
    } const *params32 = args;

    struct SQLSetConnectAttr_params params =
    {
        params32->ConnectionHandle,
        params32->Attribute,
        ULongToPtr(params32->Value),
        params32->StringLength
    };

    return wrap_SQLSetConnectAttr( &params );
}

static NTSTATUS wow64_SQLSetConnectAttrW( void *args )
{
    struct
    {
        UINT64 ConnectionHandle;
        INT32  Attribute;
        PTR32  Value;
        INT32  StringLength;
    } const *params32 = args;

    struct SQLSetConnectAttrW_params params =
    {
        params32->ConnectionHandle,
        params32->Attribute,
        ULongToPtr(params32->Value),
        params32->StringLength
    };

    return wrap_SQLSetConnectAttrW( &params );
}

static NTSTATUS wow64_SQLSetCursorName( void *args )
{
    struct
    {
        UINT64 StatementHandle;
        PTR32  CursorName;
        INT16  NameLength;
    } const *params32 = args;

    struct SQLSetCursorName_params params =
    {
        params32->StatementHandle,
        ULongToPtr(params32->CursorName),
        params32->NameLength
    };

    return wrap_SQLSetCursorName( &params );
}

static NTSTATUS wow64_SQLSetCursorNameW( void *args )
{
    struct
    {
        UINT64 StatementHandle;
        PTR32  CursorName;
        INT16  NameLength;
    } const *params32 = args;

    struct SQLSetCursorNameW_params params =
    {
        params32->StatementHandle,
        ULongToPtr(params32->CursorName),
        params32->NameLength
    };

    return wrap_SQLSetCursorNameW( &params );
}

static NTSTATUS wow64_SQLSetDescField( void *args )
{
    struct
    {
        UINT64 DescriptorHandle;
        INT16  RecNumber;
        INT16  FieldIdentifier;
        PTR32  Value;
        INT32  BufferLength;
    } const *params32 = args;

    struct SQLSetDescField_params params =
    {
        params32->DescriptorHandle,
        params32->RecNumber,
        params32->FieldIdentifier,
        ULongToPtr(params32->Value),
        params32->BufferLength
    };

    return wrap_SQLSetDescField( &params );
}

static NTSTATUS wow64_SQLSetDescFieldW( void *args )
{
    struct
    {
        UINT64 DescriptorHandle;
        INT16  RecNumber;
        INT16  FieldIdentifier;
        PTR32  Value;
        INT32  BufferLength;
    } const *params32 = args;

    struct SQLSetDescFieldW_params params =
    {
        params32->DescriptorHandle,
        params32->RecNumber,
        params32->FieldIdentifier,
        ULongToPtr(params32->Value),
        params32->BufferLength
    };

    return wrap_SQLSetDescFieldW( &params );
}

static NTSTATUS wow64_SQLSetDescRec( void *args )
{
    struct
    {
        UINT64 DescriptorHandle;
        INT16  RecNumber;
        INT16  Type;
        INT16  SubType;
        INT64  Length;
        INT16  Precision;
        INT16  Scale;
        PTR32  Data;
        PTR32  StringLength;
        PTR32  Indicator;
    } const *params32 = args;

    struct SQLSetDescRec_params params =
    {
        params32->DescriptorHandle,
        params32->RecNumber,
        params32->Type,
        params32->SubType,
        params32->Length,
        params32->Precision,
        params32->Scale,
        ULongToPtr(params32->Data),
        ULongToPtr(params32->StringLength),
        ULongToPtr(params32->Indicator)
    };

    return wrap_SQLSetDescRec( &params );
}

static NTSTATUS wow64_SQLSetEnvAttr( void *args )
{
    struct
    {
        UINT64 EnvironmentHandle;
        INT32  Attribute;
        PTR32  Value;
        INT32  StringLength;
    } const *params32 = args;

    struct SQLSetEnvAttr_params params =
    {
        params32->EnvironmentHandle,
        params32->Attribute,
        ULongToPtr(params32->Value),
        params32->StringLength
    };

    return wrap_SQLSetEnvAttr( &params );
}

static NTSTATUS wow64_SQLSetParam( void *args )
{
    struct
    {
        UINT64 StatementHandle;
        INT16  ParameterNumber;
        INT16  ValueType;
        INT16  ParameterType;
        UINT64 LengthPrecision;
        INT16  ParameterScale;
        PTR32  ParameterValue;
        PTR32  StrLen_or_Ind;
    } const *params32 = args;

    struct SQLSetParam_params params =
    {
        params32->StatementHandle,
        params32->ParameterNumber,
        params32->ValueType,
        params32->ParameterType,
        params32->LengthPrecision,
        params32->ParameterScale,
        ULongToPtr(params32->ParameterValue),
        ULongToPtr(params32->StrLen_or_Ind)
    };

    return wrap_SQLSetParam( &params );
}

static NTSTATUS wow64_SQLSetStmtAttr( void *args )
{
    struct
    {
        UINT64 StatementHandle;
        INT32  Attribute;
        PTR32  Value;
        INT32  StringLength;
    } const *params32 = args;

    struct SQLSetStmtAttr_params params =
    {
        params32->StatementHandle,
        params32->Attribute,
        ULongToPtr(params32->Value),
        params32->StringLength
    };

    return wrap_SQLSetStmtAttr( &params );
}

static NTSTATUS wow64_SQLSetStmtAttrW( void *args )
{
    struct
    {
        UINT64 StatementHandle;
        INT32  Attribute;
        PTR32  Value;
        INT32  StringLength;
    } const *params32 = args;

    struct SQLSetStmtAttrW_params params =
    {
        params32->StatementHandle,
        params32->Attribute,
        ULongToPtr(params32->Value),
        params32->StringLength
    };

    return wrap_SQLSetStmtAttrW( &params );
}

static NTSTATUS wow64_SQLSpecialColumns( void *args )
{
    struct
    {
        UINT64 StatementHandle;
        UINT16 IdentifierType;
        PTR32  CatalogName;
        INT16  NameLength1;
        PTR32  SchemaName;
        INT16  NameLength2;
        PTR32  TableName;
        INT16  NameLength3;
        UINT16 Scope;
        UINT16 Nullable;
    } const *params32 = args;

    struct SQLSpecialColumns_params params =
    {
        params32->StatementHandle,
        params32->IdentifierType,
        ULongToPtr(params32->CatalogName),
        params32->NameLength1,
        ULongToPtr(params32->SchemaName),
        params32->NameLength2,
        ULongToPtr(params32->TableName),
        params32->NameLength3,
        params32->Scope,
        params32->Nullable
    };

    return wrap_SQLSpecialColumns( &params );
}

static NTSTATUS wow64_SQLSpecialColumnsW( void *args )
{
    struct
    {
        UINT64 StatementHandle;
        UINT16 IdentifierType;
        PTR32  CatalogName;
        INT16  NameLength1;
        PTR32  SchemaName;
        INT16  NameLength2;
        PTR32  TableName;
        INT16  NameLength3;
        UINT16 Scope;
        UINT16 Nullable;
    } const *params32 = args;

    struct SQLSpecialColumnsW_params params =
    {
        params32->StatementHandle,
        params32->IdentifierType,
        ULongToPtr(params32->CatalogName),
        params32->NameLength1,
        ULongToPtr(params32->SchemaName),
        params32->NameLength2,
        ULongToPtr(params32->TableName),
        params32->NameLength3,
        params32->Scope,
        params32->Nullable
    };

    return wrap_SQLSpecialColumnsW( &params );
}

static NTSTATUS wow64_SQLStatistics( void *args )
{
    struct
    {
        UINT64 StatementHandle;
        PTR32  CatalogName;
        INT16  NameLength1;
        PTR32  SchemaName;
        INT16  NameLength2;
        PTR32  TableName;
        INT16  NameLength3;
        UINT16 Unique;
        UINT16 Reserved;
    } const *params32 = args;

    struct SQLStatistics_params params =
    {
        params32->StatementHandle,
        ULongToPtr(params32->CatalogName),
        params32->NameLength1,
        ULongToPtr(params32->SchemaName),
        params32->NameLength2,
        ULongToPtr(params32->TableName),
        params32->NameLength3,
        params32->Unique,
        params32->Reserved
    };

    return wrap_SQLStatistics( &params );
}

static NTSTATUS wow64_SQLStatisticsW( void *args )
{
    struct
    {
        UINT64 StatementHandle;
        PTR32  CatalogName;
        INT16  NameLength1;
        PTR32  SchemaName;
        INT16  NameLength2;
        PTR32  TableName;
        INT16  NameLength3;
        UINT16 Unique;
        UINT16 Reserved;
    } const *params32 = args;

    struct SQLStatisticsW_params params =
    {
        params32->StatementHandle,
        ULongToPtr(params32->CatalogName),
        params32->NameLength1,
        ULongToPtr(params32->SchemaName),
        params32->NameLength2,
        ULongToPtr(params32->TableName),
        params32->NameLength3,
        params32->Unique,
        params32->Reserved
    };

    return wrap_SQLStatisticsW( &params );
}

static NTSTATUS wow64_SQLGetStmtAttr( void *args )
{
    struct
    {
        UINT64 StatementHandle;
        INT32  Attribute;
        PTR32  Value;
        INT32  BufferLength;
        PTR32  StringLength;
    } const *params32 = args;

    struct SQLGetStmtAttr_params params =
    {
        params32->StatementHandle,
        params32->Attribute,
        ULongToPtr(params32->Value),
        params32->BufferLength,
        ULongToPtr(params32->StringLength)
    };

    return wrap_SQLGetStmtAttr( &params );
}

static NTSTATUS wow64_SQLGetStmtAttrW( void *args )
{
    struct
    {
        UINT64 StatementHandle;
        INT32  Attribute;
        PTR32  Value;
        INT32  BufferLength;
        PTR32  StringLength;
    } const *params32 = args;

    struct SQLGetStmtAttrW_params params =
    {
        params32->StatementHandle,
        params32->Attribute,
        ULongToPtr(params32->Value),
        params32->BufferLength,
        ULongToPtr(params32->StringLength)
    };

    return wrap_SQLGetStmtAttrW( &params );
}

static NTSTATUS wow64_SQLTablePrivileges( void *args )
{
    struct
    {
        UINT64 StatementHandle;
        PTR32  CatalogName;
        INT16  NameLength1;
        PTR32  SchemaName;
        INT16  NameLength2;
        PTR32  TableName;
        INT16  NameLength3;
    } const *params32 = args;

    struct SQLTablePrivileges_params params =
    {
        params32->StatementHandle,
        ULongToPtr(params32->CatalogName),
        params32->NameLength1,
        ULongToPtr(params32->SchemaName),
        params32->NameLength2,
        ULongToPtr(params32->TableName),
        params32->NameLength3
    };

    return wrap_SQLTablePrivileges( &params );
}

static NTSTATUS wow64_SQLTablePrivilegesW( void *args )
{
    struct
    {
        UINT64 StatementHandle;
        PTR32  CatalogName;
        INT16  NameLength1;
        PTR32  SchemaName;
        INT16  NameLength2;
        PTR32  TableName;
        INT16  NameLength3;
    } const *params32 = args;

    struct SQLTablePrivilegesW_params params =
    {
        params32->StatementHandle,
        ULongToPtr(params32->CatalogName),
        params32->NameLength1,
        ULongToPtr(params32->SchemaName),
        params32->NameLength2,
        ULongToPtr(params32->TableName),
        params32->NameLength3
    };

    return wrap_SQLTablePrivilegesW( &params );
}

static NTSTATUS wow64_SQLTables( void *args )
{
    struct
    {
        UINT64 StatementHandle;
        PTR32  CatalogName;
        INT16  NameLength1;
        PTR32  SchemaName;
        INT16  NameLength2;
        PTR32  TableName;
        INT16  NameLength3;
        PTR32  TableType;
        INT16  NameLength4;
    } const *params32 = args;

    struct SQLTables_params params =
    {
        params32->StatementHandle,
        ULongToPtr(params32->CatalogName),
        params32->NameLength1,
        ULongToPtr(params32->SchemaName),
        params32->NameLength2,
        ULongToPtr(params32->TableName),
        params32->NameLength3,
        ULongToPtr(params32->TableType),
        params32->NameLength4
    };

    return wrap_SQLTables( &params );
}

static NTSTATUS wow64_SQLTablesW( void *args )
{
    struct
    {
        UINT64 StatementHandle;
        PTR32  CatalogName;
        INT16  NameLength1;
        PTR32  SchemaName;
        INT16  NameLength2;
        PTR32  TableName;
        INT16  NameLength3;
        PTR32  TableType;
        INT16  NameLength4;
    } const *params32 = args;

    struct SQLTablesW_params params =
    {
        params32->StatementHandle,
        ULongToPtr(params32->CatalogName),
        params32->NameLength1,
        ULongToPtr(params32->SchemaName),
        params32->NameLength2,
        ULongToPtr(params32->TableName),
        params32->NameLength3,
        ULongToPtr(params32->TableType),
        params32->NameLength4
    };

    return wrap_SQLTablesW( &params );
}

const unixlib_entry_t __wine_unix_call_wow64_funcs[] =
{
    odbc_process_attach,
    odbc_process_detach,
    wrap_SQLAllocHandle,
    wrap_SQLAllocHandleStd,
    wow64_SQLBindCol,
    wow64_SQLBindParameter,
    wow64_SQLBrowseConnect,
    wow64_SQLBrowseConnectW,
    wrap_SQLBulkOperations,
    wrap_SQLCancel,
    wrap_SQLCloseCursor,
    wow64_SQLColAttribute,
    wow64_SQLColAttributeW,
    wow64_SQLColAttributes,
    wow64_SQLColAttributesW,
    wow64_SQLColumnPrivileges,
    wow64_SQLColumnPrivilegesW,
    wow64_SQLColumns,
    wow64_SQLColumnsW,
    wow64_SQLConnect,
    wow64_SQLConnectW,
    wrap_SQLCopyDesc,
    wow64_SQLDescribeCol,
    wow64_SQLDescribeColW,
    wow64_SQLDescribeParam,
    wrap_SQLDisconnect,
    wow64_SQLDriverConnect,
    wow64_SQLDriverConnectW,
    wrap_SQLEndTran,
    wow64_SQLError,
    wow64_SQLErrorW,
    wow64_SQLExecDirect,
    wow64_SQLExecDirectW,
    wrap_SQLExecute,
    wow64_SQLExtendedFetch,
    wrap_SQLFetch,
    wrap_SQLFetchScroll,
    wow64_SQLForeignKeys,
    wow64_SQLForeignKeysW,
    wrap_SQLFreeHandle,
    wrap_SQLFreeStmt,
    wow64_SQLGetConnectAttr,
    wow64_SQLGetConnectAttrW,
    wow64_SQLGetConnectOption,
    wow64_SQLGetConnectOptionW,
    wow64_SQLGetCursorName,
    wow64_SQLGetCursorNameW,
    wow64_SQLGetData,
    wow64_SQLGetDescField,
    wow64_SQLGetDescFieldW,
    wow64_SQLGetDescRec,
    wow64_SQLGetDescRecW,
    wow64_SQLGetDiagField,
    wow64_SQLGetDiagFieldW,
    wow64_SQLGetDiagRec,
    wow64_SQLGetDiagRecW,
    wow64_SQLGetEnvAttr,
    wow64_SQLGetFunctions,
    wow64_SQLGetInfo,
    wow64_SQLGetInfoW,
    wow64_SQLGetStmtAttr,
    wow64_SQLGetStmtAttrW,
    wow64_SQLGetStmtOption,
    wrap_SQLGetTypeInfo,
    wrap_SQLGetTypeInfoW,
    wrap_SQLMoreResults,
    wow64_SQLNativeSql,
    wow64_SQLNativeSqlW,
    wow64_SQLNumParams,
    wow64_SQLNumResultCols,
    wow64_SQLParamData,
    wow64_SQLParamOptions,
    wow64_SQLPrepare,
    wow64_SQLPrepareW,
    wow64_SQLPrimaryKeys,
    wow64_SQLPrimaryKeysW,
    wow64_SQLProcedureColumns,
    wow64_SQLProcedureColumnsW,
    wow64_SQLProcedures,
    wow64_SQLProceduresW,
    wow64_SQLPutData,
    wow64_SQLRowCount,
    wow64_SQLSetConnectAttr,
    wow64_SQLSetConnectAttrW,
    wrap_SQLSetConnectOption,
    wrap_SQLSetConnectOptionW,
    wow64_SQLSetCursorName,
    wow64_SQLSetCursorNameW,
    wow64_SQLSetDescField,
    wow64_SQLSetDescFieldW,
    wow64_SQLSetDescRec,
    wow64_SQLSetEnvAttr,
    wow64_SQLSetParam,
    wrap_SQLSetPos,
    wrap_SQLSetScrollOptions,
    wow64_SQLSetStmtAttr,
    wow64_SQLSetStmtAttrW,
    wrap_SQLSetStmtOption,
    wow64_SQLSpecialColumns,
    wow64_SQLSpecialColumnsW,
    wow64_SQLStatistics,
    wow64_SQLStatisticsW,
    wow64_SQLTablePrivileges,
    wow64_SQLTablePrivilegesW,
    wow64_SQLTables,
    wow64_SQLTablesW,
    wrap_SQLTransact,
};

C_ASSERT( ARRAYSIZE( __wine_unix_call_wow64_funcs) == unix_funcs_count );

#endif  /* _WIN64 */
