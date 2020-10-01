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
 *
 * NOTES:
 *   Proxy ODBC driver manager.  This manager delegates all ODBC
 *   calls to a real ODBC driver manager named by the environment
 *   variable LIB_ODBC_DRIVER_MANAGER, or to libodbc.so if the
 *   variable is not set.
 */

#if 0
#pragma makedep unix
#endif

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <stdlib.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "wine/debug.h"
#include "sql.h"
#include "sqltypes.h"
#include "sqlext.h"

#include "unixlib.h"
#include "wine/debug.h"

WINE_DECLARE_DEBUG_CHANNEL(winediag);

static SQLRETURN (*pSQLAllocConnect)(SQLHENV,SQLHDBC*);
static SQLRETURN (*pSQLAllocEnv)(SQLHENV*);
static SQLRETURN (*pSQLAllocHandle)(SQLSMALLINT,SQLHANDLE,SQLHANDLE*);
static SQLRETURN (*pSQLAllocHandleStd)(SQLSMALLINT,SQLHANDLE,SQLHANDLE*);
static SQLRETURN (*pSQLAllocStmt)(SQLHDBC,SQLHSTMT*);
static SQLRETURN (*pSQLBindCol)(SQLHSTMT,SQLUSMALLINT,SQLSMALLINT,SQLPOINTER,SQLLEN,SQLLEN*);
static SQLRETURN (*pSQLBindParam)(SQLHSTMT,SQLUSMALLINT,SQLSMALLINT,SQLSMALLINT,SQLULEN,SQLSMALLINT,SQLPOINTER,SQLLEN*);
static SQLRETURN (*pSQLBindParameter)(SQLHSTMT,SQLUSMALLINT,SQLSMALLINT,SQLSMALLINT,SQLSMALLINT,SQLULEN,SQLSMALLINT,SQLPOINTER,SQLLEN,SQLLEN*);
static SQLRETURN (*pSQLBrowseConnect)(SQLHDBC,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLBrowseConnectW)(SQLHDBC,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLBulkOperations)(SQLHSTMT,SQLSMALLINT);
static SQLRETURN (*pSQLCancel)(SQLHSTMT);
static SQLRETURN (*pSQLCloseCursor)(SQLHSTMT);
static SQLRETURN (*pSQLColAttribute)(SQLHSTMT,SQLUSMALLINT,SQLUSMALLINT,SQLPOINTER,SQLSMALLINT,SQLSMALLINT*,SQLLEN*);
static SQLRETURN (*pSQLColAttributeW)(SQLHSTMT,SQLUSMALLINT,SQLUSMALLINT,SQLPOINTER,SQLSMALLINT,SQLSMALLINT*,SQLLEN*);
static SQLRETURN (*pSQLColAttributes)(SQLHSTMT,SQLUSMALLINT,SQLUSMALLINT,SQLPOINTER,SQLSMALLINT,SQLSMALLINT*,SQLLEN*);
static SQLRETURN (*pSQLColAttributesW)(SQLHSTMT,SQLUSMALLINT,SQLUSMALLINT,SQLPOINTER,SQLSMALLINT,SQLSMALLINT*,SQLLEN*);
static SQLRETURN (*pSQLColumnPrivileges)(SQLHSTMT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLColumnPrivilegesW)(SQLHSTMT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLColumns)(SQLHSTMT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLColumnsW)(SQLHSTMT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLConnect)(SQLHDBC,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLConnectW)(SQLHDBC,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLCopyDesc)(SQLHDESC,SQLHDESC);
static SQLRETURN (*pSQLDataSources)(SQLHENV,SQLUSMALLINT,SQLCHAR*,SQLSMALLINT,SQLSMALLINT*,SQLCHAR*,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLDataSourcesA)(SQLHENV,SQLUSMALLINT,SQLCHAR*,SQLSMALLINT,SQLSMALLINT*,SQLCHAR*,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLDataSourcesW)(SQLHENV,SQLUSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLSMALLINT*,SQLWCHAR*,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLDescribeCol)(SQLHSTMT,SQLUSMALLINT,SQLCHAR*,SQLSMALLINT,SQLSMALLINT*,SQLSMALLINT*,SQLULEN*,SQLSMALLINT*,SQLSMALLINT*);
static SQLRETURN (*pSQLDescribeColW)(SQLHSTMT,SQLUSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLSMALLINT*,SQLSMALLINT*,SQLULEN*,SQLSMALLINT*,SQLSMALLINT*);
static SQLRETURN (*pSQLDescribeParam)(SQLHSTMT,SQLUSMALLINT,SQLSMALLINT*,SQLULEN*,SQLSMALLINT*,SQLSMALLINT*);
static SQLRETURN (*pSQLDisconnect)(SQLHDBC);
static SQLRETURN (*pSQLDriverConnect)(SQLHDBC,SQLHWND,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLSMALLINT*,SQLUSMALLINT);
static SQLRETURN (*pSQLDriverConnectW)(SQLHDBC,SQLHWND,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLSMALLINT*,SQLUSMALLINT);
static SQLRETURN (*pSQLDrivers)(SQLHENV,SQLUSMALLINT,SQLCHAR*,SQLSMALLINT,SQLSMALLINT*,SQLCHAR*,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLDriversW)(SQLHENV,SQLUSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLSMALLINT*,SQLWCHAR*,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLEndTran)(SQLSMALLINT,SQLHANDLE,SQLSMALLINT);
static SQLRETURN (*pSQLError)(SQLHENV,SQLHDBC,SQLHSTMT,SQLCHAR*,SQLINTEGER*,SQLCHAR*,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLErrorW)(SQLHENV,SQLHDBC,SQLHSTMT,SQLWCHAR*,SQLINTEGER*,SQLWCHAR*,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLExecDirect)(SQLHSTMT,SQLCHAR*,SQLINTEGER);
static SQLRETURN (*pSQLExecDirectW)(SQLHSTMT,SQLWCHAR*,SQLINTEGER);
static SQLRETURN (*pSQLExecute)(SQLHSTMT);
static SQLRETURN (*pSQLExtendedFetch)(SQLHSTMT,SQLUSMALLINT,SQLLEN,SQLULEN*,SQLUSMALLINT*);
static SQLRETURN (*pSQLFetch)(SQLHSTMT);
static SQLRETURN (*pSQLFetchScroll)(SQLHSTMT,SQLSMALLINT,SQLLEN);
static SQLRETURN (*pSQLForeignKeys)(SQLHSTMT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLForeignKeysW)(SQLHSTMT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT);
static SQLRETURN (*pSQLFreeConnect)(SQLHDBC);
static SQLRETURN (*pSQLFreeEnv)(SQLHENV);
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
static SQLRETURN (*pSQLGetDescRec)(SQLHDESC,SQLSMALLINT,SQLCHAR*,SQLSMALLINT,SQLSMALLINT*,SQLSMALLINT*,SQLSMALLINT*,SQLLEN*,SQLSMALLINT*,SQLSMALLINT*,SQLSMALLINT*);
static SQLRETURN (*pSQLGetDescRecW)(SQLHDESC,SQLSMALLINT,SQLWCHAR*,SQLSMALLINT,SQLSMALLINT*,SQLSMALLINT*,SQLSMALLINT*,SQLLEN*,SQLSMALLINT*,SQLSMALLINT*,SQLSMALLINT*);
static SQLRETURN (*pSQLGetDiagField)(SQLSMALLINT,SQLHANDLE,SQLSMALLINT,SQLSMALLINT,SQLPOINTER,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLGetDiagFieldW)(SQLSMALLINT,SQLHANDLE,SQLSMALLINT,SQLSMALLINT,SQLPOINTER,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLGetDiagRec)(SQLSMALLINT,SQLHANDLE,SQLSMALLINT,SQLCHAR*,SQLINTEGER*,SQLCHAR*,SQLSMALLINT,SQLSMALLINT*);
static SQLRETURN (*pSQLGetDiagRecA)(SQLSMALLINT,SQLHANDLE,SQLSMALLINT,SQLCHAR*,SQLINTEGER*, SQLCHAR*,SQLSMALLINT,SQLSMALLINT*);
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


static SQLRETURN WINAPI wrap_SQLAllocConnect(SQLHENV EnvironmentHandle, SQLHDBC *ConnectionHandle)
{
    return pSQLAllocConnect(EnvironmentHandle, ConnectionHandle);
}

static SQLRETURN WINAPI wrap_SQLAllocEnv(SQLHENV *EnvironmentHandle)
{
    return pSQLAllocEnv(EnvironmentHandle);
}

static SQLRETURN WINAPI wrap_SQLAllocHandle(SQLSMALLINT HandleType, SQLHANDLE InputHandle, SQLHANDLE *OutputHandle)
{
    return pSQLAllocHandle(HandleType, InputHandle, OutputHandle);
}

static SQLRETURN WINAPI wrap_SQLAllocHandleStd(SQLSMALLINT HandleType, SQLHANDLE InputHandle, SQLHANDLE *OutputHandle)
{
    return pSQLAllocHandleStd(HandleType, InputHandle, OutputHandle);
}

static SQLRETURN WINAPI wrap_SQLAllocStmt(SQLHDBC ConnectionHandle, SQLHSTMT *StatementHandle)
{
    return pSQLAllocStmt(ConnectionHandle, StatementHandle);
}

static SQLRETURN WINAPI wrap_SQLBindCol(SQLHSTMT StatementHandle, SQLUSMALLINT ColumnNumber, SQLSMALLINT TargetType,
                                        SQLPOINTER TargetValue, SQLLEN BufferLength, SQLLEN *StrLen_or_Ind)
{
    return pSQLBindCol(StatementHandle, ColumnNumber, TargetType, TargetValue, BufferLength, StrLen_or_Ind);
}

static SQLRETURN WINAPI wrap_SQLBindParam(SQLHSTMT StatementHandle, SQLUSMALLINT ParameterNumber, SQLSMALLINT ValueType,
                                          SQLSMALLINT ParameterType, SQLULEN LengthPrecision, SQLSMALLINT ParameterScale,
                                          SQLPOINTER ParameterValue, SQLLEN *StrLen_or_Ind)
{
    return pSQLBindParam(StatementHandle, ParameterNumber, ValueType, ParameterType, LengthPrecision, ParameterScale,
                         ParameterValue, StrLen_or_Ind);
}

static SQLRETURN WINAPI wrap_SQLBindParameter(SQLHSTMT hstmt, SQLUSMALLINT ipar, SQLSMALLINT fParamType,
                                              SQLSMALLINT fCType, SQLSMALLINT fSqlType, SQLULEN cbColDef,
                                              SQLSMALLINT ibScale, SQLPOINTER rgbValue, SQLLEN cbValueMax,
                                              SQLLEN *pcbValue)
{
    return pSQLBindParameter(hstmt, ipar, fParamType, fCType, fSqlType, cbColDef, ibScale, rgbValue, cbValueMax,
                             pcbValue);
}

static SQLRETURN WINAPI wrap_SQLBrowseConnect(SQLHDBC hdbc, SQLCHAR *szConnStrIn, SQLSMALLINT cbConnStrIn,
                                              SQLCHAR *szConnStrOut, SQLSMALLINT cbConnStrOutMax,
                                              SQLSMALLINT *pcbConnStrOut)
{
    return pSQLBrowseConnect(hdbc, szConnStrIn, cbConnStrIn, szConnStrOut, cbConnStrOutMax, pcbConnStrOut);
}

static SQLRETURN WINAPI wrap_SQLBrowseConnectW(SQLHDBC hdbc, SQLWCHAR *szConnStrIn, SQLSMALLINT cbConnStrIn,
                                               SQLWCHAR *szConnStrOut, SQLSMALLINT cbConnStrOutMax,
                                               SQLSMALLINT *pcbConnStrOut)
{
    return pSQLBrowseConnectW(hdbc, szConnStrIn, cbConnStrIn, szConnStrOut, cbConnStrOutMax, pcbConnStrOut);
}

static SQLRETURN WINAPI wrap_SQLBulkOperations(SQLHSTMT StatementHandle, SQLSMALLINT Operation)
{
    return pSQLBulkOperations(StatementHandle, Operation);
}

static SQLRETURN WINAPI wrap_SQLCancel(SQLHSTMT StatementHandle)
{
    return pSQLCancel(StatementHandle);
}

static SQLRETURN WINAPI wrap_SQLCloseCursor(SQLHSTMT StatementHandle)
{
    return pSQLCloseCursor(StatementHandle);
}

static SQLRETURN WINAPI wrap_SQLColAttribute(SQLHSTMT StatementHandle, SQLUSMALLINT ColumnNumber,
                                             SQLUSMALLINT FieldIdentifier, SQLPOINTER CharacterAttribute,
                                             SQLSMALLINT BufferLength, SQLSMALLINT *StringLength,
                                             SQLLEN *NumericAttribute)
{
    return pSQLColAttribute(StatementHandle, ColumnNumber, FieldIdentifier, CharacterAttribute, BufferLength,
                            StringLength, NumericAttribute);
}

static SQLRETURN WINAPI wrap_SQLColAttributeW(SQLHSTMT StatementHandle, SQLUSMALLINT ColumnNumber,
                                              SQLUSMALLINT FieldIdentifier, SQLPOINTER CharacterAttribute,
                                              SQLSMALLINT BufferLength, SQLSMALLINT *StringLength,
                                              SQLLEN *NumericAttribute)
{
    return pSQLColAttributeW(StatementHandle, ColumnNumber, FieldIdentifier, CharacterAttribute, BufferLength,
                             StringLength, NumericAttribute);
}

static SQLRETURN WINAPI wrap_SQLColAttributes(SQLHSTMT hstmt, SQLUSMALLINT icol, SQLUSMALLINT fDescType,
                                              SQLPOINTER rgbDesc, SQLSMALLINT cbDescMax, SQLSMALLINT *pcbDesc,
                                              SQLLEN *pfDesc)
{
    return pSQLColAttributes(hstmt, icol, fDescType, rgbDesc, cbDescMax, pcbDesc, pfDesc);
}

static SQLRETURN WINAPI wrap_SQLColAttributesW(SQLHSTMT hstmt, SQLUSMALLINT icol, SQLUSMALLINT fDescType,
                                               SQLPOINTER rgbDesc, SQLSMALLINT cbDescMax, SQLSMALLINT *pcbDesc,
                                               SQLLEN *pfDesc)
{
    return pSQLColAttributesW(hstmt, icol, fDescType, rgbDesc, cbDescMax, pcbDesc, pfDesc);
}

static SQLRETURN WINAPI wrap_SQLColumnPrivileges(SQLHSTMT hstmt, SQLCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
                                                 SQLCHAR *szSchemaName, SQLSMALLINT cbSchemaName, SQLCHAR *szTableName,
                                                 SQLSMALLINT cbTableName, SQLCHAR *szColumnName, SQLSMALLINT cbColumnName)
{
    return pSQLColumnPrivileges(hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName,
                                szTableName, cbTableName, szColumnName, cbColumnName);
}

static SQLRETURN WINAPI wrap_SQLColumnPrivilegesW(SQLHSTMT hstmt, SQLWCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
                                                  SQLWCHAR *szSchemaName, SQLSMALLINT cbSchemaName, SQLWCHAR *szTableName,
                                                  SQLSMALLINT cbTableName, SQLWCHAR *szColumnName, SQLSMALLINT cbColumnName)
{
    return pSQLColumnPrivilegesW(hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName, szTableName,
                                 cbTableName, szColumnName, cbColumnName);
}

static SQLRETURN WINAPI wrap_SQLColumns(SQLHSTMT StatementHandle, SQLCHAR *CatalogName, SQLSMALLINT NameLength1,
                                        SQLCHAR *SchemaName, SQLSMALLINT NameLength2, SQLCHAR *TableName,
                                        SQLSMALLINT NameLength3, SQLCHAR *ColumnName, SQLSMALLINT NameLength4)
{
    return pSQLColumns(StatementHandle, CatalogName, NameLength1, SchemaName, NameLength2, TableName,
                       NameLength3, ColumnName, NameLength4);
}

static SQLRETURN WINAPI wrap_SQLColumnsW(SQLHSTMT StatementHandle, WCHAR *CatalogName, SQLSMALLINT NameLength1,
                                         WCHAR *SchemaName, SQLSMALLINT NameLength2, WCHAR *TableName,
                                         SQLSMALLINT NameLength3, WCHAR *ColumnName, SQLSMALLINT NameLength4)
{
    return pSQLColumnsW(StatementHandle, CatalogName, NameLength1, SchemaName, NameLength2, TableName, NameLength3,
                        ColumnName, NameLength4);
}

static SQLRETURN WINAPI wrap_SQLConnect(SQLHDBC ConnectionHandle, SQLCHAR *ServerName, SQLSMALLINT NameLength1,
                                        SQLCHAR *UserName, SQLSMALLINT NameLength2, SQLCHAR *Authentication,
                                        SQLSMALLINT NameLength3)
{
    return pSQLConnect(ConnectionHandle, ServerName, NameLength1, UserName, NameLength2, Authentication, NameLength3);
}

static SQLRETURN WINAPI wrap_SQLConnectW(SQLHDBC ConnectionHandle, WCHAR *ServerName, SQLSMALLINT NameLength1,
                                         WCHAR *UserName, SQLSMALLINT NameLength2, WCHAR *Authentication,
                                         SQLSMALLINT NameLength3)
{
    return pSQLConnectW(ConnectionHandle, ServerName, NameLength1, UserName, NameLength2, Authentication, NameLength3);
}

static SQLRETURN WINAPI wrap_SQLCopyDesc(SQLHDESC SourceDescHandle, SQLHDESC TargetDescHandle)
{
    return pSQLCopyDesc(SourceDescHandle, TargetDescHandle);
}

static SQLRETURN WINAPI wrap_SQLDataSources(SQLHENV EnvironmentHandle, SQLUSMALLINT Direction, SQLCHAR *ServerName,
                                            SQLSMALLINT BufferLength1, SQLSMALLINT *NameLength1, SQLCHAR *Description,
                                            SQLSMALLINT BufferLength2, SQLSMALLINT *NameLength2)
{
    return pSQLDataSources(EnvironmentHandle, Direction, ServerName, BufferLength1, NameLength1, Description,
                           BufferLength2, NameLength2);
}

static SQLRETURN WINAPI wrap_SQLDataSourcesA(SQLHENV EnvironmentHandle, SQLUSMALLINT Direction, SQLCHAR *ServerName,
                                             SQLSMALLINT BufferLength1, SQLSMALLINT *NameLength1, SQLCHAR *Description,
                                             SQLSMALLINT BufferLength2, SQLSMALLINT *NameLength2)
{
    return pSQLDataSourcesA(EnvironmentHandle, Direction, ServerName, BufferLength1, NameLength1, Description,
                            BufferLength2, NameLength2);
}

static SQLRETURN WINAPI wrap_SQLDataSourcesW(SQLHENV EnvironmentHandle, SQLUSMALLINT Direction, WCHAR *ServerName,
                                             SQLSMALLINT BufferLength1, SQLSMALLINT *NameLength1, WCHAR *Description,
                                             SQLSMALLINT BufferLength2, SQLSMALLINT *NameLength2)
{
    return pSQLDataSourcesW(EnvironmentHandle, Direction, ServerName, BufferLength1, NameLength1, Description,
                            BufferLength2, NameLength2);
}

static SQLRETURN WINAPI wrap_SQLDescribeCol(SQLHSTMT StatementHandle, SQLUSMALLINT ColumnNumber, SQLCHAR *ColumnName,
                                            SQLSMALLINT BufferLength, SQLSMALLINT *NameLength, SQLSMALLINT *DataType,
                                            SQLULEN *ColumnSize, SQLSMALLINT *DecimalDigits, SQLSMALLINT *Nullable)
{
    return pSQLDescribeCol(StatementHandle, ColumnNumber, ColumnName, BufferLength, NameLength, DataType, ColumnSize,
                           DecimalDigits, Nullable);
}

static SQLRETURN WINAPI wrap_SQLDescribeColW(SQLHSTMT StatementHandle, SQLUSMALLINT ColumnNumber, WCHAR *ColumnName,
                                             SQLSMALLINT BufferLength, SQLSMALLINT *NameLength, SQLSMALLINT *DataType,
                                             SQLULEN *ColumnSize, SQLSMALLINT *DecimalDigits, SQLSMALLINT *Nullable)
{
    return pSQLDescribeColW(StatementHandle, ColumnNumber, ColumnName, BufferLength, NameLength, DataType, ColumnSize,
                            DecimalDigits, Nullable);
}

static SQLRETURN WINAPI wrap_SQLDescribeParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, SQLSMALLINT *pfSqlType,
                                              SQLULEN *pcbParamDef, SQLSMALLINT *pibScale, SQLSMALLINT *pfNullable)
{
    return pSQLDescribeParam(hstmt, ipar, pfSqlType, pcbParamDef, pibScale, pfNullable);
}

static SQLRETURN WINAPI wrap_SQLDisconnect(SQLHDBC ConnectionHandle)
{
    return pSQLDisconnect(ConnectionHandle);
}

static SQLRETURN WINAPI wrap_SQLDriverConnect(SQLHDBC hdbc, SQLHWND hwnd, SQLCHAR *ConnectionString, SQLSMALLINT Length,
                                              SQLCHAR *conn_str_out, SQLSMALLINT conn_str_out_max,
                                              SQLSMALLINT *ptr_conn_str_out, SQLUSMALLINT driver_completion)
{
    return pSQLDriverConnect(hdbc, hwnd, ConnectionString, Length, conn_str_out, conn_str_out_max,
                             ptr_conn_str_out, driver_completion);
}

static SQLRETURN WINAPI wrap_SQLDriverConnectW(SQLHDBC ConnectionHandle, SQLHWND WindowHandle, WCHAR *InConnectionString,
                                               SQLSMALLINT Length, WCHAR *OutConnectionString, SQLSMALLINT BufferLength,
                                               SQLSMALLINT *Length2, SQLUSMALLINT DriverCompletion)
{
    return pSQLDriverConnectW(ConnectionHandle, WindowHandle, InConnectionString, Length, OutConnectionString,
                              BufferLength, Length2, DriverCompletion);
}

static SQLRETURN WINAPI wrap_SQLDrivers(SQLHENV EnvironmentHandle, SQLUSMALLINT fDirection, SQLCHAR *szDriverDesc,
                                        SQLSMALLINT cbDriverDescMax, SQLSMALLINT *pcbDriverDesc,
                                        SQLCHAR *szDriverAttributes, SQLSMALLINT cbDriverAttrMax,
                                        SQLSMALLINT *pcbDriverAttr)
{
    return pSQLDrivers(EnvironmentHandle, fDirection, szDriverDesc, cbDriverDescMax, pcbDriverDesc,
                       szDriverAttributes, cbDriverAttrMax, pcbDriverAttr);
}

static SQLRETURN WINAPI wrap_SQLDriversW(SQLHENV EnvironmentHandle, SQLUSMALLINT fDirection, SQLWCHAR *szDriverDesc,
                                         SQLSMALLINT cbDriverDescMax, SQLSMALLINT *pcbDriverDesc,
                                         SQLWCHAR *szDriverAttributes, SQLSMALLINT cbDriverAttrMax,
                                         SQLSMALLINT *pcbDriverAttr)
{
    return pSQLDriversW(EnvironmentHandle, fDirection, szDriverDesc, cbDriverDescMax, pcbDriverDesc,
                        szDriverAttributes, cbDriverAttrMax, pcbDriverAttr);
}

static SQLRETURN WINAPI wrap_SQLEndTran(SQLSMALLINT HandleType, SQLHANDLE Handle, SQLSMALLINT CompletionType)
{
    return pSQLEndTran(HandleType, Handle, CompletionType);
}

static SQLRETURN WINAPI wrap_SQLError(SQLHENV EnvironmentHandle, SQLHDBC ConnectionHandle, SQLHSTMT StatementHandle,
                                      SQLCHAR *Sqlstate, SQLINTEGER *NativeError, SQLCHAR *MessageText,
                                      SQLSMALLINT BufferLength, SQLSMALLINT *TextLength)
{
    return pSQLError(EnvironmentHandle, ConnectionHandle, StatementHandle, Sqlstate, NativeError, MessageText,
                     BufferLength, TextLength);
}

static SQLRETURN WINAPI wrap_SQLErrorW(SQLHENV EnvironmentHandle, SQLHDBC ConnectionHandle, SQLHSTMT StatementHandle,
                                       WCHAR *Sqlstate, SQLINTEGER *NativeError, WCHAR *MessageText,
                                       SQLSMALLINT BufferLength, SQLSMALLINT *TextLength)
{
    return pSQLErrorW(EnvironmentHandle, ConnectionHandle, StatementHandle, Sqlstate, NativeError, MessageText,
                      BufferLength, TextLength);
}

static SQLRETURN WINAPI wrap_SQLExecDirect(SQLHSTMT StatementHandle, SQLCHAR *StatementText, SQLINTEGER TextLength)
{
    return pSQLExecDirect(StatementHandle, StatementText, TextLength);
}

static SQLRETURN WINAPI wrap_SQLExecDirectW(SQLHSTMT StatementHandle, WCHAR *StatementText, SQLINTEGER TextLength)
{
    return pSQLExecDirectW(StatementHandle, StatementText, TextLength);
}

static SQLRETURN WINAPI wrap_SQLExecute(SQLHSTMT StatementHandle)
{
    return pSQLExecute(StatementHandle);
}

static SQLRETURN WINAPI wrap_SQLExtendedFetch(SQLHSTMT hstmt, SQLUSMALLINT fFetchType, SQLLEN irow, SQLULEN *pcrow,
                                              SQLUSMALLINT *rgfRowStatus)
{
    return pSQLExtendedFetch(hstmt, fFetchType, irow, pcrow, rgfRowStatus);
}

static SQLRETURN WINAPI wrap_SQLFetch(SQLHSTMT StatementHandle)
{
    return pSQLFetch(StatementHandle);
}

static SQLRETURN WINAPI wrap_SQLFetchScroll(SQLHSTMT StatementHandle, SQLSMALLINT FetchOrientation, SQLLEN FetchOffset)
{
    return pSQLFetchScroll(StatementHandle, FetchOrientation, FetchOffset);
}

static SQLRETURN WINAPI wrap_SQLForeignKeys(SQLHSTMT hstmt, SQLCHAR *szPkCatalogName, SQLSMALLINT cbPkCatalogName,
                                            SQLCHAR *szPkSchemaName, SQLSMALLINT cbPkSchemaName, SQLCHAR *szPkTableName,
                                            SQLSMALLINT cbPkTableName, SQLCHAR *szFkCatalogName,
                                            SQLSMALLINT cbFkCatalogName, SQLCHAR *szFkSchemaName,
                                            SQLSMALLINT cbFkSchemaName, SQLCHAR *szFkTableName, SQLSMALLINT cbFkTableName)
{
    return pSQLForeignKeys(hstmt, szPkCatalogName, cbPkCatalogName, szPkSchemaName, cbPkSchemaName, szPkTableName,
                           cbPkTableName, szFkCatalogName, cbFkCatalogName, szFkSchemaName, cbFkSchemaName,
                           szFkTableName, cbFkTableName);
}

static SQLRETURN WINAPI wrap_SQLForeignKeysW(SQLHSTMT hstmt, SQLWCHAR *szPkCatalogName, SQLSMALLINT cbPkCatalogName,
                                             SQLWCHAR *szPkSchemaName, SQLSMALLINT cbPkSchemaName, SQLWCHAR *szPkTableName,
                                             SQLSMALLINT cbPkTableName, SQLWCHAR *szFkCatalogName,
                                             SQLSMALLINT cbFkCatalogName, SQLWCHAR *szFkSchemaName,
                                             SQLSMALLINT cbFkSchemaName, SQLWCHAR *szFkTableName, SQLSMALLINT cbFkTableName)
{
    return pSQLForeignKeysW(hstmt, szPkCatalogName, cbPkCatalogName, szPkSchemaName, cbPkSchemaName, szPkTableName,
                            cbPkTableName, szFkCatalogName, cbFkCatalogName, szFkSchemaName, cbFkSchemaName,
                            szFkTableName, cbFkTableName);
}

static SQLRETURN WINAPI wrap_SQLFreeConnect(SQLHDBC ConnectionHandle)
{
    return pSQLFreeConnect(ConnectionHandle);
}

static SQLRETURN WINAPI wrap_SQLFreeEnv(SQLHENV EnvironmentHandle)
{
    return pSQLFreeEnv(EnvironmentHandle);
}

static SQLRETURN WINAPI wrap_SQLFreeHandle(SQLSMALLINT HandleType, SQLHANDLE Handle)
{
    return pSQLFreeHandle(HandleType, Handle);
}

static SQLRETURN WINAPI wrap_SQLFreeStmt(SQLHSTMT StatementHandle, SQLUSMALLINT Option)
{
    return pSQLFreeStmt(StatementHandle, Option);
}

static SQLRETURN WINAPI wrap_SQLGetConnectAttr(SQLHDBC ConnectionHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                               SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
    return pSQLGetConnectAttr(ConnectionHandle, Attribute, Value, BufferLength, StringLength);
}

static SQLRETURN WINAPI wrap_SQLGetConnectAttrW(SQLHDBC ConnectionHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                                SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
    return pSQLGetConnectAttrW(ConnectionHandle, Attribute, Value, BufferLength, StringLength);
}

static SQLRETURN WINAPI wrap_SQLGetConnectOption(SQLHDBC ConnectionHandle, SQLUSMALLINT Option, SQLPOINTER Value)
{
    return pSQLGetConnectOption(ConnectionHandle, Option, Value);
}

static SQLRETURN WINAPI wrap_SQLGetConnectOptionW(SQLHDBC ConnectionHandle, SQLUSMALLINT Option, SQLPOINTER Value)
{
    return pSQLGetConnectOptionW(ConnectionHandle, Option, Value);
}

static SQLRETURN WINAPI wrap_SQLGetCursorName(SQLHSTMT StatementHandle, SQLCHAR *CursorName, SQLSMALLINT BufferLength,
                                              SQLSMALLINT *NameLength)
{
    return pSQLGetCursorName(StatementHandle, CursorName, BufferLength, NameLength);
}

static SQLRETURN WINAPI wrap_SQLGetCursorNameW(SQLHSTMT StatementHandle, WCHAR *CursorName, SQLSMALLINT BufferLength,
                                               SQLSMALLINT *NameLength)
{
    return pSQLGetCursorNameW(StatementHandle, CursorName, BufferLength, NameLength);
}

static SQLRETURN WINAPI wrap_SQLGetData(SQLHSTMT StatementHandle, SQLUSMALLINT ColumnNumber, SQLSMALLINT TargetType,
                                        SQLPOINTER TargetValue, SQLLEN BufferLength, SQLLEN *StrLen_or_Ind)
{
    return pSQLGetData(StatementHandle, ColumnNumber, TargetType, TargetValue, BufferLength, StrLen_or_Ind);
}

static SQLRETURN WINAPI wrap_SQLGetDescField(SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber, SQLSMALLINT FieldIdentifier,
                                             SQLPOINTER Value, SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
    return pSQLGetDescField(DescriptorHandle, RecNumber, FieldIdentifier, Value, BufferLength, StringLength);
}

static SQLRETURN WINAPI wrap_SQLGetDescFieldW(SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber, SQLSMALLINT FieldIdentifier,
                                              SQLPOINTER Value, SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
    return pSQLGetDescFieldW(DescriptorHandle, RecNumber, FieldIdentifier, Value, BufferLength, StringLength);
}

static SQLRETURN WINAPI wrap_SQLGetDescRec(SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber, SQLCHAR *Name,
                                           SQLSMALLINT BufferLength, SQLSMALLINT *StringLength, SQLSMALLINT *Type,
                                           SQLSMALLINT *SubType, SQLLEN *Length, SQLSMALLINT *Precision,
                                           SQLSMALLINT *Scale, SQLSMALLINT *Nullable)
{
    return pSQLGetDescRec(DescriptorHandle, RecNumber, Name, BufferLength, StringLength, Type, SubType, Length,
                          Precision, Scale, Nullable);
}

static SQLRETURN WINAPI wrap_SQLGetDescRecW(SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber, WCHAR *Name,
                                            SQLSMALLINT BufferLength, SQLSMALLINT *StringLength, SQLSMALLINT *Type,
                                            SQLSMALLINT *SubType, SQLLEN *Length, SQLSMALLINT *Precision,
                                            SQLSMALLINT *Scale, SQLSMALLINT *Nullable)
{
    return pSQLGetDescRecW(DescriptorHandle, RecNumber, Name, BufferLength, StringLength, Type, SubType, Length,
                           Precision, Scale, Nullable);
}

static SQLRETURN WINAPI wrap_SQLGetDiagField(SQLSMALLINT HandleType, SQLHANDLE Handle, SQLSMALLINT RecNumber,
                                             SQLSMALLINT DiagIdentifier, SQLPOINTER DiagInfo, SQLSMALLINT BufferLength,
                                             SQLSMALLINT *StringLength)
{
    return pSQLGetDiagField(HandleType, Handle, RecNumber, DiagIdentifier, DiagInfo, BufferLength, StringLength);
}

static SQLRETURN WINAPI wrap_SQLGetDiagFieldW(SQLSMALLINT HandleType, SQLHANDLE Handle, SQLSMALLINT RecNumber,
                                              SQLSMALLINT DiagIdentifier, SQLPOINTER DiagInfo, SQLSMALLINT BufferLength,
                                              SQLSMALLINT *StringLength)
{
    return pSQLGetDiagFieldW(HandleType, Handle, RecNumber, DiagIdentifier, DiagInfo, BufferLength, StringLength);
}

static SQLRETURN WINAPI wrap_SQLGetDiagRec(SQLSMALLINT HandleType, SQLHANDLE Handle, SQLSMALLINT RecNumber,
                                           SQLCHAR *Sqlstate, SQLINTEGER *NativeError, SQLCHAR *MessageText,
                                           SQLSMALLINT BufferLength, SQLSMALLINT *TextLength)
{
    return pSQLGetDiagRec(HandleType, Handle, RecNumber, Sqlstate, NativeError, MessageText, BufferLength, TextLength);
}

static SQLRETURN WINAPI wrap_SQLGetDiagRecA(SQLSMALLINT HandleType, SQLHANDLE Handle, SQLSMALLINT RecNumber,
                                            SQLCHAR *Sqlstate, SQLINTEGER *NativeError, SQLCHAR *MessageText,
                                            SQLSMALLINT BufferLength, SQLSMALLINT *TextLength)
{
    return pSQLGetDiagRecA(HandleType, Handle, RecNumber, Sqlstate, NativeError, MessageText, BufferLength, TextLength);
}

static SQLRETURN WINAPI wrap_SQLGetDiagRecW(SQLSMALLINT HandleType, SQLHANDLE Handle, SQLSMALLINT RecNumber,
                                            WCHAR *Sqlstate, SQLINTEGER *NativeError, WCHAR *MessageText,
                                            SQLSMALLINT BufferLength, SQLSMALLINT *TextLength)
{
    return pSQLGetDiagRecW(HandleType, Handle, RecNumber, Sqlstate, NativeError, MessageText, BufferLength, TextLength);
}

static SQLRETURN WINAPI wrap_SQLGetEnvAttr(SQLHENV EnvironmentHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                           SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
    return pSQLGetEnvAttr(EnvironmentHandle, Attribute, Value, BufferLength, StringLength);
}

static SQLRETURN WINAPI wrap_SQLGetFunctions(SQLHDBC ConnectionHandle, SQLUSMALLINT FunctionId, SQLUSMALLINT *Supported)
{
    return pSQLGetFunctions(ConnectionHandle, FunctionId, Supported);
}

static SQLRETURN WINAPI wrap_SQLGetInfo(SQLHDBC ConnectionHandle, SQLUSMALLINT InfoType, SQLPOINTER InfoValue,
                                        SQLSMALLINT BufferLength, SQLSMALLINT *StringLength)
{
    return pSQLGetInfo(ConnectionHandle, InfoType, InfoValue, BufferLength, StringLength);
}

static SQLRETURN WINAPI wrap_SQLGetInfoW(SQLHDBC ConnectionHandle, SQLUSMALLINT InfoType, SQLPOINTER InfoValue,
                                         SQLSMALLINT BufferLength, SQLSMALLINT *StringLength)
{
    return pSQLGetInfoW(ConnectionHandle, InfoType, InfoValue, BufferLength, StringLength);
}

static SQLRETURN WINAPI wrap_SQLGetStmtAttr(SQLHSTMT StatementHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                            SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
    return pSQLGetStmtAttr(StatementHandle, Attribute, Value, BufferLength, StringLength);
}

static SQLRETURN WINAPI wrap_SQLGetStmtAttrW(SQLHSTMT StatementHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                             SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
    return pSQLGetStmtAttrW(StatementHandle, Attribute, Value, BufferLength, StringLength);
}

static SQLRETURN WINAPI wrap_SQLGetStmtOption(SQLHSTMT StatementHandle, SQLUSMALLINT Option, SQLPOINTER Value)
{
    return pSQLGetStmtOption(StatementHandle, Option, Value);
}

static SQLRETURN WINAPI wrap_SQLGetTypeInfo(SQLHSTMT StatementHandle, SQLSMALLINT DataType)
{
    return pSQLGetTypeInfo(StatementHandle, DataType);
}

static SQLRETURN WINAPI wrap_SQLGetTypeInfoW(SQLHSTMT StatementHandle, SQLSMALLINT DataType)
{
    return pSQLGetTypeInfoW(StatementHandle, DataType);
}

static SQLRETURN WINAPI wrap_SQLMoreResults(SQLHSTMT StatementHandle)
{
    return pSQLMoreResults(StatementHandle);
}

static SQLRETURN WINAPI wrap_SQLNativeSql(SQLHDBC hdbc, SQLCHAR *szSqlStrIn, SQLINTEGER cbSqlStrIn, SQLCHAR *szSqlStr,
                                          SQLINTEGER cbSqlStrMax, SQLINTEGER *pcbSqlStr)
{
    return pSQLNativeSql(hdbc, szSqlStrIn, cbSqlStrIn, szSqlStr, cbSqlStrMax, pcbSqlStr);
}

static SQLRETURN WINAPI wrap_SQLNativeSqlW(SQLHDBC hdbc, SQLWCHAR *szSqlStrIn, SQLINTEGER cbSqlStrIn, SQLWCHAR *szSqlStr,
                                           SQLINTEGER cbSqlStrMax, SQLINTEGER *pcbSqlStr)
{
    return pSQLNativeSqlW(hdbc, szSqlStrIn, cbSqlStrIn, szSqlStr, cbSqlStrMax, pcbSqlStr);
}

static SQLRETURN WINAPI wrap_SQLNumParams(SQLHSTMT hstmt, SQLSMALLINT *pcpar)
{
    return pSQLNumParams(hstmt, pcpar);
}

static SQLRETURN WINAPI wrap_SQLNumResultCols(SQLHSTMT StatementHandle, SQLSMALLINT *ColumnCount)
{
    return pSQLNumResultCols(StatementHandle, ColumnCount);
}

static SQLRETURN WINAPI wrap_SQLParamData(SQLHSTMT StatementHandle, SQLPOINTER *Value)
{
    return pSQLParamData(StatementHandle, Value);
}

static SQLRETURN WINAPI wrap_SQLParamOptions(SQLHSTMT hstmt, SQLULEN crow, SQLULEN *pirow)
{
    return pSQLParamOptions(hstmt, crow, pirow);
}

static SQLRETURN WINAPI wrap_SQLPrepare(SQLHSTMT StatementHandle, SQLCHAR *StatementText, SQLINTEGER TextLength)
{
    return pSQLPrepare(StatementHandle, StatementText, TextLength);
}

static SQLRETURN WINAPI wrap_SQLPrepareW(SQLHSTMT StatementHandle, WCHAR *StatementText, SQLINTEGER TextLength)
{
    return pSQLPrepareW(StatementHandle, StatementText, TextLength);
}

static SQLRETURN WINAPI wrap_SQLPrimaryKeys(SQLHSTMT hstmt, SQLCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
                                            SQLCHAR *szSchemaName, SQLSMALLINT cbSchemaName, SQLCHAR *szTableName,
                                            SQLSMALLINT cbTableName)
{
    return pSQLPrimaryKeys(hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName, szTableName, cbTableName);
}

static SQLRETURN WINAPI wrap_SQLPrimaryKeysW(SQLHSTMT hstmt, SQLWCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
                                             SQLWCHAR *szSchemaName, SQLSMALLINT cbSchemaName, SQLWCHAR *szTableName,
                                             SQLSMALLINT cbTableName)
{
    return pSQLPrimaryKeysW(hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName, szTableName, cbTableName);
}

static SQLRETURN WINAPI wrap_SQLProcedureColumns(SQLHSTMT hstmt, SQLCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
                                                 SQLCHAR *szSchemaName, SQLSMALLINT cbSchemaName, SQLCHAR *szProcName,
                                                 SQLSMALLINT cbProcName, SQLCHAR *szColumnName, SQLSMALLINT cbColumnName)
{
    return pSQLProcedureColumns(hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName, szProcName,
                                cbProcName, szColumnName, cbColumnName);
}

static SQLRETURN WINAPI wrap_SQLProcedureColumnsW(SQLHSTMT hstmt, SQLWCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
                                                  SQLWCHAR *szSchemaName, SQLSMALLINT cbSchemaName, SQLWCHAR *szProcName,
                                                  SQLSMALLINT cbProcName, SQLWCHAR *szColumnName, SQLSMALLINT cbColumnName)
{
    return pSQLProcedureColumnsW(hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName, szProcName,
                                 cbProcName, szColumnName, cbColumnName);
}

static SQLRETURN WINAPI wrap_SQLProcedures(SQLHSTMT hstmt, SQLCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
                                           SQLCHAR *szSchemaName, SQLSMALLINT cbSchemaName, SQLCHAR *szProcName,
                                           SQLSMALLINT cbProcName)
{
    return pSQLProcedures(hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName, szProcName, cbProcName);
}

static SQLRETURN WINAPI wrap_SQLProceduresW(SQLHSTMT hstmt, SQLWCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
                                            SQLWCHAR *szSchemaName, SQLSMALLINT cbSchemaName, SQLWCHAR *szProcName,
                                            SQLSMALLINT cbProcName)
{
    return pSQLProceduresW(hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName, szProcName, cbProcName);
}

static SQLRETURN WINAPI wrap_SQLPutData(SQLHSTMT StatementHandle, SQLPOINTER Data, SQLLEN StrLen_or_Ind)
{
    return pSQLPutData(StatementHandle, Data, StrLen_or_Ind);
}

static SQLRETURN WINAPI wrap_SQLRowCount(SQLHSTMT StatementHandle, SQLLEN *RowCount)
{
    return pSQLRowCount(StatementHandle, RowCount);
}

static SQLRETURN WINAPI wrap_SQLSetConnectAttr(SQLHDBC ConnectionHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                               SQLINTEGER StringLength)
{
    return pSQLSetConnectAttr(ConnectionHandle, Attribute, Value, StringLength);
}

static SQLRETURN WINAPI wrap_SQLSetConnectAttrW(SQLHDBC ConnectionHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                                SQLINTEGER StringLength)
{
    return pSQLSetConnectAttrW(ConnectionHandle, Attribute, Value, StringLength);
}

static SQLRETURN WINAPI wrap_SQLSetConnectOption(SQLHDBC ConnectionHandle, SQLUSMALLINT Option, SQLULEN Value)
{
    return pSQLSetConnectOption(ConnectionHandle, Option, Value);
}

static SQLRETURN WINAPI wrap_SQLSetConnectOptionW(SQLHDBC ConnectionHandle, SQLUSMALLINT Option, SQLULEN Value)
{
    return pSQLSetConnectOptionW(ConnectionHandle, Option, Value);
}

static SQLRETURN WINAPI wrap_SQLSetCursorName(SQLHSTMT StatementHandle, SQLCHAR *CursorName, SQLSMALLINT NameLength)
{
    return pSQLSetCursorName(StatementHandle, CursorName, NameLength);
}

static SQLRETURN WINAPI wrap_SQLSetCursorNameW(SQLHSTMT StatementHandle, WCHAR *CursorName, SQLSMALLINT NameLength)
{
    return pSQLSetCursorNameW(StatementHandle, CursorName, NameLength);
}

static SQLRETURN WINAPI wrap_SQLSetDescField(SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber, SQLSMALLINT FieldIdentifier,
                                             SQLPOINTER Value, SQLINTEGER BufferLength)
{
    return pSQLSetDescField(DescriptorHandle, RecNumber, FieldIdentifier, Value, BufferLength);
}

static SQLRETURN WINAPI wrap_SQLSetDescFieldW(SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber, SQLSMALLINT FieldIdentifier,
                                              SQLPOINTER Value, SQLINTEGER BufferLength)
{
    return pSQLSetDescFieldW(DescriptorHandle, RecNumber, FieldIdentifier, Value, BufferLength);
}

static SQLRETURN WINAPI wrap_SQLSetDescRec(SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber, SQLSMALLINT Type,
                                           SQLSMALLINT SubType, SQLLEN Length, SQLSMALLINT Precision, SQLSMALLINT Scale,
                                           SQLPOINTER Data, SQLLEN *StringLength, SQLLEN *Indicator)
{
    return pSQLSetDescRec(DescriptorHandle, RecNumber, Type, SubType, Length, Precision, Scale, Data,
                          StringLength, Indicator);
}

static SQLRETURN WINAPI wrap_SQLSetEnvAttr(SQLHENV EnvironmentHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                           SQLINTEGER StringLength)
{
    return pSQLSetEnvAttr(EnvironmentHandle, Attribute, Value, StringLength);
}

static SQLRETURN WINAPI wrap_SQLSetParam(SQLHSTMT StatementHandle, SQLUSMALLINT ParameterNumber, SQLSMALLINT ValueType,
                                         SQLSMALLINT ParameterType, SQLULEN LengthPrecision, SQLSMALLINT ParameterScale,
                                         SQLPOINTER ParameterValue, SQLLEN *StrLen_or_Ind)
{
    return pSQLSetParam(StatementHandle, ParameterNumber, ValueType, ParameterType, LengthPrecision,
                        ParameterScale, ParameterValue, StrLen_or_Ind);
}

static SQLRETURN WINAPI wrap_SQLSetPos(SQLHSTMT hstmt, SQLSETPOSIROW irow, SQLUSMALLINT fOption, SQLUSMALLINT fLock)
{
    return pSQLSetPos(hstmt, irow, fOption, fLock);
}

static SQLRETURN WINAPI wrap_SQLSetScrollOptions(SQLHSTMT statement_handle, SQLUSMALLINT f_concurrency, SQLLEN crow_keyset,
                                                 SQLUSMALLINT crow_rowset)
{
    return pSQLSetScrollOptions(statement_handle, f_concurrency, crow_keyset, crow_rowset);
}

static SQLRETURN WINAPI wrap_SQLSetStmtAttr(SQLHSTMT StatementHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                            SQLINTEGER StringLength)
{
    return pSQLSetStmtAttr(StatementHandle, Attribute, Value, StringLength);
}

static SQLRETURN WINAPI wrap_SQLSetStmtAttrW(SQLHSTMT StatementHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                             SQLINTEGER StringLength)
{
    return pSQLSetStmtAttrW(StatementHandle, Attribute, Value, StringLength);
}

static SQLRETURN WINAPI wrap_SQLSetStmtOption(SQLHSTMT StatementHandle, SQLUSMALLINT Option, SQLULEN Value)
{
    return pSQLSetStmtOption(StatementHandle, Option, Value);
}

static SQLRETURN WINAPI wrap_SQLSpecialColumns(SQLHSTMT StatementHandle, SQLUSMALLINT IdentifierType, SQLCHAR *CatalogName,
                                               SQLSMALLINT NameLength1, SQLCHAR *SchemaName, SQLSMALLINT NameLength2,
                                               SQLCHAR *TableName, SQLSMALLINT NameLength3, SQLUSMALLINT Scope,
                                               SQLUSMALLINT Nullable)
{
    return pSQLSpecialColumns(StatementHandle, IdentifierType, CatalogName, NameLength1, SchemaName,
                              NameLength2, TableName, NameLength3, Scope, Nullable);
}

static SQLRETURN WINAPI wrap_SQLSpecialColumnsW(SQLHSTMT StatementHandle, SQLUSMALLINT IdentifierType,
                                                SQLWCHAR *CatalogName, SQLSMALLINT NameLength1, SQLWCHAR *SchemaName,
                                                SQLSMALLINT NameLength2, SQLWCHAR *TableName, SQLSMALLINT NameLength3,
                                                SQLUSMALLINT Scope, SQLUSMALLINT Nullable)
{
    return pSQLSpecialColumnsW(StatementHandle, IdentifierType, CatalogName, NameLength1, SchemaName,
                               NameLength2, TableName, NameLength3, Scope, Nullable);
}

static SQLRETURN WINAPI wrap_SQLStatistics(SQLHSTMT StatementHandle, SQLCHAR *CatalogName, SQLSMALLINT NameLength1,
                                           SQLCHAR *SchemaName, SQLSMALLINT NameLength2, SQLCHAR *TableName,
                                           SQLSMALLINT NameLength3, SQLUSMALLINT Unique, SQLUSMALLINT Reserved)
{
    return pSQLStatistics(StatementHandle, CatalogName, NameLength1, SchemaName, NameLength2, TableName,
                          NameLength3, Unique, Reserved);
}

static SQLRETURN WINAPI wrap_SQLStatisticsW(SQLHSTMT StatementHandle, SQLWCHAR *CatalogName, SQLSMALLINT NameLength1,
                                            SQLWCHAR *SchemaName, SQLSMALLINT NameLength2, SQLWCHAR *TableName,
                                            SQLSMALLINT NameLength3, SQLUSMALLINT Unique, SQLUSMALLINT Reserved)
{
    return pSQLStatisticsW(StatementHandle, CatalogName, NameLength1, SchemaName, NameLength2, TableName,
                           NameLength3, Unique, Reserved);
}

static SQLRETURN WINAPI wrap_SQLTablePrivileges(SQLHSTMT hstmt, SQLCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
                                                SQLCHAR *szSchemaName, SQLSMALLINT cbSchemaName, SQLCHAR *szTableName,
                                                SQLSMALLINT cbTableName)
{
    return pSQLTablePrivileges(hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName, szTableName,
                               cbTableName);
}

static SQLRETURN WINAPI wrap_SQLTablePrivilegesW(SQLHSTMT hstmt, SQLWCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
                                                 SQLWCHAR *szSchemaName, SQLSMALLINT cbSchemaName, SQLWCHAR *szTableName,
                                                 SQLSMALLINT cbTableName)
{
    return pSQLTablePrivilegesW(hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName, szTableName,
                                cbTableName);
}

static SQLRETURN WINAPI wrap_SQLTables(SQLHSTMT StatementHandle, SQLCHAR *CatalogName, SQLSMALLINT NameLength1,
                                       SQLCHAR *SchemaName, SQLSMALLINT NameLength2, SQLCHAR *TableName,
                                       SQLSMALLINT NameLength3, SQLCHAR *TableType, SQLSMALLINT NameLength4)
{
    return pSQLTables(StatementHandle, CatalogName, NameLength1, SchemaName, NameLength2, TableName, NameLength3,
                      TableType, NameLength4);
}

static SQLRETURN WINAPI wrap_SQLTablesW(SQLHSTMT StatementHandle, SQLWCHAR *CatalogName, SQLSMALLINT NameLength1,
                                        SQLWCHAR *SchemaName, SQLSMALLINT NameLength2, SQLWCHAR *TableName,
                                        SQLSMALLINT NameLength3, SQLWCHAR *TableType, SQLSMALLINT NameLength4)
{
    return pSQLTablesW(StatementHandle, CatalogName, NameLength1, SchemaName, NameLength2, TableName, NameLength3,
                       TableType, NameLength4);
}

static SQLRETURN WINAPI wrap_SQLTransact(SQLHENV EnvironmentHandle, SQLHDBC ConnectionHandle, SQLUSMALLINT CompletionType)
{
    return pSQLTransact(EnvironmentHandle, ConnectionHandle, CompletionType);
}

static void *libodbc;

static NTSTATUS load_odbc( struct sql_funcs *funcs )
{
   const char *s = getenv("LIB_ODBC_DRIVER_MANAGER");

#ifdef SONAME_LIBODBC
   if (!s || !s[0]) s = SONAME_LIBODBC;
#endif
   if (!s || !s[0] || !(libodbc = dlopen( s, RTLD_LAZY | RTLD_GLOBAL )))
   {
       ERR_(winediag)("failed to open library %s: %s\n", debugstr_a(s), dlerror());
       return STATUS_DLL_NOT_FOUND;
   }

#define LOAD_FUNC(name) if ((p##name = dlsym( libodbc, #name ))) funcs->p##name = wrap_##name

    LOAD_FUNC(SQLAllocConnect);
    LOAD_FUNC(SQLAllocEnv);
    LOAD_FUNC(SQLAllocHandle);
    LOAD_FUNC(SQLAllocHandleStd);
    LOAD_FUNC(SQLAllocStmt);
    LOAD_FUNC(SQLBindCol);
    LOAD_FUNC(SQLBindParam);
    LOAD_FUNC(SQLBindParameter);
    LOAD_FUNC(SQLBrowseConnect);
    LOAD_FUNC(SQLBrowseConnectW);
    LOAD_FUNC(SQLBulkOperations);
    LOAD_FUNC(SQLCancel);
    LOAD_FUNC(SQLCloseCursor);
    LOAD_FUNC(SQLColAttribute);
    LOAD_FUNC(SQLColAttributeW);
    LOAD_FUNC(SQLColAttributes);
    LOAD_FUNC(SQLColAttributesW);
    LOAD_FUNC(SQLColumnPrivileges);
    LOAD_FUNC(SQLColumnPrivilegesW);
    LOAD_FUNC(SQLColumns);
    LOAD_FUNC(SQLColumnsW);
    LOAD_FUNC(SQLConnect);
    LOAD_FUNC(SQLConnectW);
    LOAD_FUNC(SQLCopyDesc);
    LOAD_FUNC(SQLDataSources);
    LOAD_FUNC(SQLDataSourcesA);
    LOAD_FUNC(SQLDataSourcesW);
    LOAD_FUNC(SQLDescribeCol);
    LOAD_FUNC(SQLDescribeColW);
    LOAD_FUNC(SQLDescribeParam);
    LOAD_FUNC(SQLDisconnect);
    LOAD_FUNC(SQLDriverConnect);
    LOAD_FUNC(SQLDriverConnectW);
    LOAD_FUNC(SQLDrivers);
    LOAD_FUNC(SQLDriversW);
    LOAD_FUNC(SQLEndTran);
    LOAD_FUNC(SQLError);
    LOAD_FUNC(SQLErrorW);
    LOAD_FUNC(SQLExecDirect);
    LOAD_FUNC(SQLExecDirectW);
    LOAD_FUNC(SQLExecute);
    LOAD_FUNC(SQLExtendedFetch);
    LOAD_FUNC(SQLFetch);
    LOAD_FUNC(SQLFetchScroll);
    LOAD_FUNC(SQLForeignKeys);
    LOAD_FUNC(SQLForeignKeysW);
    LOAD_FUNC(SQLFreeConnect);
    LOAD_FUNC(SQLFreeEnv);
    LOAD_FUNC(SQLFreeHandle);
    LOAD_FUNC(SQLFreeStmt);
    LOAD_FUNC(SQLGetConnectAttr);
    LOAD_FUNC(SQLGetConnectAttrW);
    LOAD_FUNC(SQLGetConnectOption);
    LOAD_FUNC(SQLGetConnectOptionW);
    LOAD_FUNC(SQLGetCursorName);
    LOAD_FUNC(SQLGetCursorNameW);
    LOAD_FUNC(SQLGetData);
    LOAD_FUNC(SQLGetDescField);
    LOAD_FUNC(SQLGetDescFieldW);
    LOAD_FUNC(SQLGetDescRec);
    LOAD_FUNC(SQLGetDescRecW);
    LOAD_FUNC(SQLGetDiagField);
    LOAD_FUNC(SQLGetDiagFieldW);
    LOAD_FUNC(SQLGetDiagRec);
    LOAD_FUNC(SQLGetDiagRecA);
    LOAD_FUNC(SQLGetDiagRecW);
    LOAD_FUNC(SQLGetEnvAttr);
    LOAD_FUNC(SQLGetFunctions);
    LOAD_FUNC(SQLGetInfo);
    LOAD_FUNC(SQLGetInfoW);
    LOAD_FUNC(SQLGetStmtAttr);
    LOAD_FUNC(SQLGetStmtAttrW);
    LOAD_FUNC(SQLGetStmtOption);
    LOAD_FUNC(SQLGetTypeInfo);
    LOAD_FUNC(SQLGetTypeInfoW);
    LOAD_FUNC(SQLMoreResults);
    LOAD_FUNC(SQLNativeSql);
    LOAD_FUNC(SQLNativeSqlW);
    LOAD_FUNC(SQLNumParams);
    LOAD_FUNC(SQLNumResultCols);
    LOAD_FUNC(SQLParamData);
    LOAD_FUNC(SQLParamOptions);
    LOAD_FUNC(SQLPrepare);
    LOAD_FUNC(SQLPrepareW);
    LOAD_FUNC(SQLPrimaryKeys);
    LOAD_FUNC(SQLPrimaryKeysW);
    LOAD_FUNC(SQLProcedureColumns);
    LOAD_FUNC(SQLProcedureColumnsW);
    LOAD_FUNC(SQLProcedures);
    LOAD_FUNC(SQLProceduresW);
    LOAD_FUNC(SQLPutData);
    LOAD_FUNC(SQLRowCount);
    LOAD_FUNC(SQLSetConnectAttr);
    LOAD_FUNC(SQLSetConnectAttrW);
    LOAD_FUNC(SQLSetConnectOption);
    LOAD_FUNC(SQLSetConnectOptionW);
    LOAD_FUNC(SQLSetCursorName);
    LOAD_FUNC(SQLSetCursorNameW);
    LOAD_FUNC(SQLSetDescField);
    LOAD_FUNC(SQLSetDescFieldW);
    LOAD_FUNC(SQLSetDescRec);
    LOAD_FUNC(SQLSetEnvAttr);
    LOAD_FUNC(SQLSetParam);
    LOAD_FUNC(SQLSetPos);
    LOAD_FUNC(SQLSetScrollOptions);
    LOAD_FUNC(SQLSetStmtAttr);
    LOAD_FUNC(SQLSetStmtAttrW);
    LOAD_FUNC(SQLSetStmtOption);
    LOAD_FUNC(SQLSpecialColumns);
    LOAD_FUNC(SQLSpecialColumnsW);
    LOAD_FUNC(SQLStatistics);
    LOAD_FUNC(SQLStatisticsW);
    LOAD_FUNC(SQLTablePrivileges);
    LOAD_FUNC(SQLTablePrivilegesW);
    LOAD_FUNC(SQLTables);
    LOAD_FUNC(SQLTablesW);
    LOAD_FUNC(SQLTransact);
#undef LOAD_FUNC
    return STATUS_SUCCESS;
}

NTSTATUS CDECL __wine_init_unix_lib( HMODULE module, DWORD reason, const void *ptr_in, void *ptr_out )
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        if (!load_odbc( ptr_out )) return STATUS_DLL_NOT_FOUND;
        break;
    case DLL_PROCESS_DETACH:
        if (libodbc) dlclose( libodbc );
        libodbc = NULL;
        break;
    }
    return STATUS_SUCCESS;
}
