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
 *
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
#include "wine/debug.h"

#include "sql.h"
#include "sqltypes.h"
#include "sqlext.h"
#include "unixlib.h"

WINE_DEFAULT_DEBUG_CHANNEL(odbc);
WINE_DECLARE_DEBUG_CHANNEL(winediag);


static SQLRETURN WINAPI null_SQLAllocConnect(SQLHENV EnvironmentHandle, SQLHDBC *ConnectionHandle)
{
    *ConnectionHandle = SQL_NULL_HDBC;
    TRACE("Not ready\n");
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLAllocEnv(SQLHENV *EnvironmentHandle)
{
    *EnvironmentHandle = SQL_NULL_HENV;
    TRACE("Not ready\n");
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLAllocHandle(SQLSMALLINT HandleType, SQLHANDLE InputHandle, SQLHANDLE *OutputHandle)
{
    WARN("ProxyODBC: Cannot load ODBC driver manager library.\n");

    if (HandleType == SQL_HANDLE_ENV)
        *OutputHandle = SQL_NULL_HENV;
    else if (HandleType == SQL_HANDLE_DBC)
        *OutputHandle = SQL_NULL_HDBC;
    else if (HandleType == SQL_HANDLE_STMT)
        *OutputHandle = SQL_NULL_HSTMT;
    else if (HandleType == SQL_HANDLE_DESC)
        *OutputHandle = SQL_NULL_HDESC;

    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLAllocHandleStd(SQLSMALLINT HandleType, SQLHANDLE InputHandle, SQLHANDLE *OutputHandle)
{
    WARN("ProxyODBC: Cannot load ODBC driver manager library.\n");

    if (HandleType == SQL_HANDLE_ENV)
        *OutputHandle = SQL_NULL_HENV;
    else if (HandleType == SQL_HANDLE_DBC)
        *OutputHandle = SQL_NULL_HDBC;
    else if (HandleType == SQL_HANDLE_STMT)
        *OutputHandle = SQL_NULL_HSTMT;
    else if (HandleType == SQL_HANDLE_DESC)
        *OutputHandle = SQL_NULL_HDESC;

    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLAllocStmt(SQLHDBC ConnectionHandle, SQLHSTMT *StatementHandle)
{
    *StatementHandle = SQL_NULL_HSTMT;
    TRACE("Not ready\n");
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLBindCol(SQLHSTMT StatementHandle, SQLUSMALLINT ColumnNumber, SQLSMALLINT TargetType,
                                        SQLPOINTER TargetValue, SQLLEN BufferLength, SQLLEN *StrLen_or_Ind)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLBindParam(SQLHSTMT StatementHandle, SQLUSMALLINT ParameterNumber, SQLSMALLINT ValueType,
                                          SQLSMALLINT ParameterType, SQLULEN LengthPrecision, SQLSMALLINT ParameterScale,
                                          SQLPOINTER ParameterValue, SQLLEN *StrLen_or_Ind)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLBindParameter(SQLHSTMT hstmt, SQLUSMALLINT ipar, SQLSMALLINT fParamType,
                                              SQLSMALLINT fCType, SQLSMALLINT fSqlType, SQLULEN cbColDef,
                                              SQLSMALLINT ibScale, SQLPOINTER rgbValue, SQLLEN cbValueMax,
                                              SQLLEN *pcbValue)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLBrowseConnect(SQLHDBC hdbc, SQLCHAR *szConnStrIn, SQLSMALLINT cbConnStrIn,
                                              SQLCHAR *szConnStrOut, SQLSMALLINT cbConnStrOutMax,
                                              SQLSMALLINT *pcbConnStrOut)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLBrowseConnectW(SQLHDBC hdbc, SQLWCHAR *szConnStrIn, SQLSMALLINT cbConnStrIn,
                                               SQLWCHAR *szConnStrOut, SQLSMALLINT cbConnStrOutMax,
                                               SQLSMALLINT *pcbConnStrOut)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLBulkOperations(SQLHSTMT StatementHandle, SQLSMALLINT Operation)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLCancel(SQLHSTMT StatementHandle)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLCloseCursor(SQLHSTMT StatementHandle)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLColAttribute(SQLHSTMT StatementHandle, SQLUSMALLINT ColumnNumber,
                                             SQLUSMALLINT FieldIdentifier, SQLPOINTER CharacterAttribute,
                                             SQLSMALLINT BufferLength, SQLSMALLINT *StringLength,
                                             SQLLEN *NumericAttribute)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLColAttributeW(SQLHSTMT StatementHandle, SQLUSMALLINT ColumnNumber,
                                              SQLUSMALLINT FieldIdentifier, SQLPOINTER CharacterAttribute,
                                              SQLSMALLINT BufferLength, SQLSMALLINT *StringLength,
                                              SQLLEN *NumericAttribute)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLColAttributes(SQLHSTMT hstmt, SQLUSMALLINT icol, SQLUSMALLINT fDescType,
                                              SQLPOINTER rgbDesc, SQLSMALLINT cbDescMax, SQLSMALLINT *pcbDesc,
                                              SQLLEN *pfDesc)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLColAttributesW(SQLHSTMT hstmt, SQLUSMALLINT icol, SQLUSMALLINT fDescType,
                                               SQLPOINTER rgbDesc, SQLSMALLINT cbDescMax, SQLSMALLINT *pcbDesc,
                                               SQLLEN *pfDesc)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLColumnPrivileges(SQLHSTMT hstmt, SQLCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
                                                 SQLCHAR *szSchemaName, SQLSMALLINT cbSchemaName, SQLCHAR *szTableName,
                                                 SQLSMALLINT cbTableName, SQLCHAR *szColumnName, SQLSMALLINT cbColumnName)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLColumnPrivilegesW(SQLHSTMT hstmt, SQLWCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
                                                  SQLWCHAR *szSchemaName, SQLSMALLINT cbSchemaName, SQLWCHAR *szTableName,
                                                  SQLSMALLINT cbTableName, SQLWCHAR *szColumnName, SQLSMALLINT cbColumnName)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLColumns(SQLHSTMT StatementHandle, SQLCHAR *CatalogName, SQLSMALLINT NameLength1,
                                        SQLCHAR *SchemaName, SQLSMALLINT NameLength2, SQLCHAR *TableName,
                                        SQLSMALLINT NameLength3, SQLCHAR *ColumnName, SQLSMALLINT NameLength4)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLColumnsW(SQLHSTMT StatementHandle, WCHAR *CatalogName, SQLSMALLINT NameLength1,
                                         WCHAR *SchemaName, SQLSMALLINT NameLength2, WCHAR *TableName,
                                         SQLSMALLINT NameLength3, WCHAR *ColumnName, SQLSMALLINT NameLength4)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLConnect(SQLHDBC ConnectionHandle, SQLCHAR *ServerName, SQLSMALLINT NameLength1,
                                        SQLCHAR *UserName, SQLSMALLINT NameLength2, SQLCHAR *Authentication,
                                        SQLSMALLINT NameLength3)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLConnectW(SQLHDBC ConnectionHandle, WCHAR *ServerName, SQLSMALLINT NameLength1,
                                         WCHAR *UserName, SQLSMALLINT NameLength2, WCHAR *Authentication,
                                         SQLSMALLINT NameLength3)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLCopyDesc(SQLHDESC SourceDescHandle, SQLHDESC TargetDescHandle)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLDataSources(SQLHENV EnvironmentHandle, SQLUSMALLINT Direction, SQLCHAR *ServerName,
                                            SQLSMALLINT BufferLength1, SQLSMALLINT *NameLength1, SQLCHAR *Description,
                                            SQLSMALLINT BufferLength2, SQLSMALLINT *NameLength2)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLDataSourcesA(SQLHENV EnvironmentHandle, SQLUSMALLINT Direction, SQLCHAR *ServerName,
                                             SQLSMALLINT BufferLength1, SQLSMALLINT *NameLength1, SQLCHAR *Description,
                                             SQLSMALLINT BufferLength2, SQLSMALLINT *NameLength2)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLDataSourcesW(SQLHENV EnvironmentHandle, SQLUSMALLINT Direction, WCHAR *ServerName,
                                             SQLSMALLINT BufferLength1, SQLSMALLINT *NameLength1, WCHAR *Description,
                                             SQLSMALLINT BufferLength2, SQLSMALLINT *NameLength2)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLDescribeCol(SQLHSTMT StatementHandle, SQLUSMALLINT ColumnNumber, SQLCHAR *ColumnName,
                                            SQLSMALLINT BufferLength, SQLSMALLINT *NameLength, SQLSMALLINT *DataType,
                                            SQLULEN *ColumnSize, SQLSMALLINT *DecimalDigits, SQLSMALLINT *Nullable)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLDescribeColW(SQLHSTMT StatementHandle, SQLUSMALLINT ColumnNumber, WCHAR *ColumnName,
                                             SQLSMALLINT BufferLength, SQLSMALLINT *NameLength, SQLSMALLINT *DataType,
                                             SQLULEN *ColumnSize, SQLSMALLINT *DecimalDigits, SQLSMALLINT *Nullable)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLDescribeParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, SQLSMALLINT *pfSqlType,
                                              SQLULEN *pcbParamDef, SQLSMALLINT *pibScale, SQLSMALLINT *pfNullable)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLDisconnect(SQLHDBC ConnectionHandle)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLDriverConnect(SQLHDBC hdbc, SQLHWND hwnd, SQLCHAR *ConnectionString, SQLSMALLINT Length,
                                              SQLCHAR *conn_str_out, SQLSMALLINT conn_str_out_max,
                                              SQLSMALLINT *ptr_conn_str_out, SQLUSMALLINT driver_completion)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLDriverConnectW(SQLHDBC ConnectionHandle, SQLHWND WindowHandle, WCHAR *InConnectionString,
                                               SQLSMALLINT Length, WCHAR *OutConnectionString, SQLSMALLINT BufferLength,
                                               SQLSMALLINT *Length2, SQLUSMALLINT DriverCompletion)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLDrivers(SQLHENV EnvironmentHandle, SQLUSMALLINT fDirection, SQLCHAR *szDriverDesc,
                                        SQLSMALLINT cbDriverDescMax, SQLSMALLINT *pcbDriverDesc,
                                        SQLCHAR *szDriverAttributes, SQLSMALLINT cbDriverAttrMax,
                                        SQLSMALLINT *pcbDriverAttr)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLDriversW(SQLHENV EnvironmentHandle, SQLUSMALLINT fDirection, SQLWCHAR *szDriverDesc,
                                         SQLSMALLINT cbDriverDescMax, SQLSMALLINT *pcbDriverDesc,
                                         SQLWCHAR *szDriverAttributes, SQLSMALLINT cbDriverAttrMax,
                                         SQLSMALLINT *pcbDriverAttr)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLEndTran(SQLSMALLINT HandleType, SQLHANDLE Handle, SQLSMALLINT CompletionType)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLError(SQLHENV EnvironmentHandle, SQLHDBC ConnectionHandle, SQLHSTMT StatementHandle,
                                      SQLCHAR *Sqlstate, SQLINTEGER *NativeError, SQLCHAR *MessageText,
                                      SQLSMALLINT BufferLength, SQLSMALLINT *TextLength)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLErrorW(SQLHENV EnvironmentHandle, SQLHDBC ConnectionHandle, SQLHSTMT StatementHandle,
                                       WCHAR *Sqlstate, SQLINTEGER *NativeError, WCHAR *MessageText,
                                       SQLSMALLINT BufferLength, SQLSMALLINT *TextLength)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLExecDirect(SQLHSTMT StatementHandle, SQLCHAR *StatementText, SQLINTEGER TextLength)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLExecDirectW(SQLHSTMT StatementHandle, WCHAR *StatementText, SQLINTEGER TextLength)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLExecute(SQLHSTMT StatementHandle)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLExtendedFetch(SQLHSTMT hstmt, SQLUSMALLINT fFetchType, SQLLEN irow, SQLULEN *pcrow,
                                              SQLUSMALLINT *rgfRowStatus)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLFetch(SQLHSTMT StatementHandle)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLFetchScroll(SQLHSTMT StatementHandle, SQLSMALLINT FetchOrientation, SQLLEN FetchOffset)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLForeignKeys(SQLHSTMT hstmt, SQLCHAR *szPkCatalogName, SQLSMALLINT cbPkCatalogName,
                                            SQLCHAR *szPkSchemaName, SQLSMALLINT cbPkSchemaName, SQLCHAR *szPkTableName,
                                            SQLSMALLINT cbPkTableName, SQLCHAR *szFkCatalogName,
                                            SQLSMALLINT cbFkCatalogName, SQLCHAR *szFkSchemaName,
                                            SQLSMALLINT cbFkSchemaName, SQLCHAR *szFkTableName, SQLSMALLINT cbFkTableName)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLForeignKeysW(SQLHSTMT hstmt, SQLWCHAR *szPkCatalogName, SQLSMALLINT cbPkCatalogName,
                                             SQLWCHAR *szPkSchemaName, SQLSMALLINT cbPkSchemaName, SQLWCHAR *szPkTableName,
                                             SQLSMALLINT cbPkTableName, SQLWCHAR *szFkCatalogName,
                                             SQLSMALLINT cbFkCatalogName, SQLWCHAR *szFkSchemaName,
                                             SQLSMALLINT cbFkSchemaName, SQLWCHAR *szFkTableName, SQLSMALLINT cbFkTableName)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLFreeConnect(SQLHDBC ConnectionHandle)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLFreeEnv(SQLHENV EnvironmentHandle)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLFreeHandle(SQLSMALLINT HandleType, SQLHANDLE Handle)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLFreeStmt(SQLHSTMT StatementHandle, SQLUSMALLINT Option)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLGetConnectAttr(SQLHDBC ConnectionHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                               SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLGetConnectAttrW(SQLHDBC ConnectionHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                                SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLGetConnectOption(SQLHDBC ConnectionHandle, SQLUSMALLINT Option, SQLPOINTER Value)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLGetConnectOptionW(SQLHDBC ConnectionHandle, SQLUSMALLINT Option, SQLPOINTER Value)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLGetCursorName(SQLHSTMT StatementHandle, SQLCHAR *CursorName, SQLSMALLINT BufferLength,
                                              SQLSMALLINT *NameLength)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLGetCursorNameW(SQLHSTMT StatementHandle, WCHAR *CursorName, SQLSMALLINT BufferLength,
                                               SQLSMALLINT *NameLength)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLGetData(SQLHSTMT StatementHandle, SQLUSMALLINT ColumnNumber, SQLSMALLINT TargetType,
                                        SQLPOINTER TargetValue, SQLLEN BufferLength, SQLLEN *StrLen_or_Ind)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLGetDescField(SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber, SQLSMALLINT FieldIdentifier,
                                             SQLPOINTER Value, SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLGetDescFieldW(SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber, SQLSMALLINT FieldIdentifier,
                                              SQLPOINTER Value, SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLGetDescRec(SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber, SQLCHAR *Name,
                                           SQLSMALLINT BufferLength, SQLSMALLINT *StringLength, SQLSMALLINT *Type,
                                           SQLSMALLINT *SubType, SQLLEN *Length, SQLSMALLINT *Precision,
                                           SQLSMALLINT *Scale, SQLSMALLINT *Nullable)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLGetDescRecW(SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber, WCHAR *Name,
                                            SQLSMALLINT BufferLength, SQLSMALLINT *StringLength, SQLSMALLINT *Type,
                                            SQLSMALLINT *SubType, SQLLEN *Length, SQLSMALLINT *Precision,
                                            SQLSMALLINT *Scale, SQLSMALLINT *Nullable)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLGetDiagField(SQLSMALLINT HandleType, SQLHANDLE Handle, SQLSMALLINT RecNumber,
                                             SQLSMALLINT DiagIdentifier, SQLPOINTER DiagInfo, SQLSMALLINT BufferLength,
                                             SQLSMALLINT *StringLength)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLGetDiagFieldW(SQLSMALLINT HandleType, SQLHANDLE Handle, SQLSMALLINT RecNumber,
                                              SQLSMALLINT DiagIdentifier, SQLPOINTER DiagInfo, SQLSMALLINT BufferLength,
                                              SQLSMALLINT *StringLength)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLGetDiagRec(SQLSMALLINT HandleType, SQLHANDLE Handle, SQLSMALLINT RecNumber,
                                           SQLCHAR *Sqlstate, SQLINTEGER *NativeError, SQLCHAR *MessageText,
                                           SQLSMALLINT BufferLength, SQLSMALLINT *TextLength)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLGetDiagRecA(SQLSMALLINT HandleType, SQLHANDLE Handle, SQLSMALLINT RecNumber,
                                            SQLCHAR *Sqlstate, SQLINTEGER *NativeError, SQLCHAR *MessageText,
                                            SQLSMALLINT BufferLength, SQLSMALLINT *TextLength)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLGetDiagRecW(SQLSMALLINT HandleType, SQLHANDLE Handle, SQLSMALLINT RecNumber,
                                            WCHAR *Sqlstate, SQLINTEGER *NativeError, WCHAR *MessageText,
                                            SQLSMALLINT BufferLength, SQLSMALLINT *TextLength)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLGetEnvAttr(SQLHENV EnvironmentHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                           SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLGetFunctions(SQLHDBC ConnectionHandle, SQLUSMALLINT FunctionId, SQLUSMALLINT *Supported)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLGetInfo(SQLHDBC ConnectionHandle, SQLUSMALLINT InfoType, SQLPOINTER InfoValue,
                                        SQLSMALLINT BufferLength, SQLSMALLINT *StringLength)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLGetInfoW(SQLHDBC ConnectionHandle, SQLUSMALLINT InfoType, SQLPOINTER InfoValue,
                                         SQLSMALLINT BufferLength, SQLSMALLINT *StringLength)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLGetStmtAttr(SQLHSTMT StatementHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                            SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLGetStmtAttrW(SQLHSTMT StatementHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                             SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLGetStmtOption(SQLHSTMT StatementHandle, SQLUSMALLINT Option, SQLPOINTER Value)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLGetTypeInfo(SQLHSTMT StatementHandle, SQLSMALLINT DataType)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLGetTypeInfoW(SQLHSTMT StatementHandle, SQLSMALLINT DataType)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLMoreResults(SQLHSTMT StatementHandle)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLNativeSql(SQLHDBC hdbc, SQLCHAR *szSqlStrIn, SQLINTEGER cbSqlStrIn, SQLCHAR *szSqlStr,
                                          SQLINTEGER cbSqlStrMax, SQLINTEGER *pcbSqlStr)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLNativeSqlW(SQLHDBC hdbc, SQLWCHAR *szSqlStrIn, SQLINTEGER cbSqlStrIn, SQLWCHAR *szSqlStr,
                                           SQLINTEGER cbSqlStrMax, SQLINTEGER *pcbSqlStr)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLNumParams(SQLHSTMT hstmt, SQLSMALLINT *pcpar)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLNumResultCols(SQLHSTMT StatementHandle, SQLSMALLINT *ColumnCount)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLParamData(SQLHSTMT StatementHandle, SQLPOINTER *Value)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLParamOptions(SQLHSTMT hstmt, SQLULEN crow, SQLULEN *pirow)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLPrepare(SQLHSTMT StatementHandle, SQLCHAR *StatementText, SQLINTEGER TextLength)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLPrepareW(SQLHSTMT StatementHandle, WCHAR *StatementText, SQLINTEGER TextLength)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLPrimaryKeys(SQLHSTMT hstmt, SQLCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
                                            SQLCHAR *szSchemaName, SQLSMALLINT cbSchemaName, SQLCHAR *szTableName,
                                            SQLSMALLINT cbTableName)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLPrimaryKeysW(SQLHSTMT hstmt, SQLWCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
                                             SQLWCHAR *szSchemaName, SQLSMALLINT cbSchemaName, SQLWCHAR *szTableName,
                                             SQLSMALLINT cbTableName)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLProcedureColumns(SQLHSTMT hstmt, SQLCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
                                                 SQLCHAR *szSchemaName, SQLSMALLINT cbSchemaName, SQLCHAR *szProcName,
                                                 SQLSMALLINT cbProcName, SQLCHAR *szColumnName, SQLSMALLINT cbColumnName)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLProcedureColumnsW(SQLHSTMT hstmt, SQLWCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
                                                  SQLWCHAR *szSchemaName, SQLSMALLINT cbSchemaName, SQLWCHAR *szProcName,
                                                  SQLSMALLINT cbProcName, SQLWCHAR *szColumnName, SQLSMALLINT cbColumnName)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLProcedures(SQLHSTMT hstmt, SQLCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
                                           SQLCHAR *szSchemaName, SQLSMALLINT cbSchemaName, SQLCHAR *szProcName,
                                           SQLSMALLINT cbProcName)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLProceduresW(SQLHSTMT hstmt, SQLWCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
                                            SQLWCHAR *szSchemaName, SQLSMALLINT cbSchemaName, SQLWCHAR *szProcName,
                                            SQLSMALLINT cbProcName)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLPutData(SQLHSTMT StatementHandle, SQLPOINTER Data, SQLLEN StrLen_or_Ind)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLRowCount(SQLHSTMT StatementHandle, SQLLEN *RowCount)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLSetConnectAttr(SQLHDBC ConnectionHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                               SQLINTEGER StringLength)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLSetConnectAttrW(SQLHDBC ConnectionHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                                SQLINTEGER StringLength)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLSetConnectOption(SQLHDBC ConnectionHandle, SQLUSMALLINT Option, SQLULEN Value)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLSetConnectOptionW(SQLHDBC ConnectionHandle, SQLUSMALLINT Option, SQLULEN Value)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLSetCursorName(SQLHSTMT StatementHandle, SQLCHAR *CursorName, SQLSMALLINT NameLength)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLSetCursorNameW(SQLHSTMT StatementHandle, WCHAR *CursorName, SQLSMALLINT NameLength)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLSetDescField(SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber, SQLSMALLINT FieldIdentifier,
                                             SQLPOINTER Value, SQLINTEGER BufferLength)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLSetDescFieldW(SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber, SQLSMALLINT FieldIdentifier,
                                              SQLPOINTER Value, SQLINTEGER BufferLength)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLSetDescRec(SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber, SQLSMALLINT Type,
                                           SQLSMALLINT SubType, SQLLEN Length, SQLSMALLINT Precision, SQLSMALLINT Scale,
                                           SQLPOINTER Data, SQLLEN *StringLength, SQLLEN *Indicator)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLSetEnvAttr(SQLHENV EnvironmentHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                           SQLINTEGER StringLength)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLSetParam(SQLHSTMT StatementHandle, SQLUSMALLINT ParameterNumber, SQLSMALLINT ValueType,
                                         SQLSMALLINT ParameterType, SQLULEN LengthPrecision, SQLSMALLINT ParameterScale,
                                         SQLPOINTER ParameterValue, SQLLEN *StrLen_or_Ind)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLSetPos(SQLHSTMT hstmt, SQLSETPOSIROW irow, SQLUSMALLINT fOption, SQLUSMALLINT fLock)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLSetScrollOptions(SQLHSTMT statement_handle, SQLUSMALLINT f_concurrency, SQLLEN crow_keyset,
                                                 SQLUSMALLINT crow_rowset)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLSetStmtAttr(SQLHSTMT StatementHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                            SQLINTEGER StringLength)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLSetStmtAttrW(SQLHSTMT StatementHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                             SQLINTEGER StringLength)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLSetStmtOption(SQLHSTMT StatementHandle, SQLUSMALLINT Option, SQLULEN Value)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLSpecialColumns(SQLHSTMT StatementHandle, SQLUSMALLINT IdentifierType, SQLCHAR *CatalogName,
                                               SQLSMALLINT NameLength1, SQLCHAR *SchemaName, SQLSMALLINT NameLength2,
                                               SQLCHAR *TableName, SQLSMALLINT NameLength3, SQLUSMALLINT Scope,
                                               SQLUSMALLINT Nullable)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLSpecialColumnsW(SQLHSTMT StatementHandle, SQLUSMALLINT IdentifierType,
                                                SQLWCHAR *CatalogName, SQLSMALLINT NameLength1, SQLWCHAR *SchemaName,
                                                SQLSMALLINT NameLength2, SQLWCHAR *TableName, SQLSMALLINT NameLength3,
                                                SQLUSMALLINT Scope, SQLUSMALLINT Nullable)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLStatistics(SQLHSTMT StatementHandle, SQLCHAR *CatalogName, SQLSMALLINT NameLength1,
                                           SQLCHAR *SchemaName, SQLSMALLINT NameLength2, SQLCHAR *TableName,
                                           SQLSMALLINT NameLength3, SQLUSMALLINT Unique, SQLUSMALLINT Reserved)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLStatisticsW(SQLHSTMT StatementHandle, SQLWCHAR *CatalogName, SQLSMALLINT NameLength1,
                                            SQLWCHAR *SchemaName, SQLSMALLINT NameLength2, SQLWCHAR *TableName,
                                            SQLSMALLINT NameLength3, SQLUSMALLINT Unique, SQLUSMALLINT Reserved)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLTablePrivileges(SQLHSTMT hstmt, SQLCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
                                                SQLCHAR *szSchemaName, SQLSMALLINT cbSchemaName, SQLCHAR *szTableName,
                                                SQLSMALLINT cbTableName)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLTablePrivilegesW(SQLHSTMT hstmt, SQLWCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
                                                 SQLWCHAR *szSchemaName, SQLSMALLINT cbSchemaName, SQLWCHAR *szTableName,
                                                 SQLSMALLINT cbTableName)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLTables(SQLHSTMT StatementHandle, SQLCHAR *CatalogName, SQLSMALLINT NameLength1,
                                       SQLCHAR *SchemaName, SQLSMALLINT NameLength2, SQLCHAR *TableName,
                                       SQLSMALLINT NameLength3, SQLCHAR *TableType, SQLSMALLINT NameLength4)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLTablesW(SQLHSTMT StatementHandle, SQLWCHAR *CatalogName, SQLSMALLINT NameLength1,
                                        SQLWCHAR *SchemaName, SQLSMALLINT NameLength2, SQLWCHAR *TableName,
                                        SQLSMALLINT NameLength3, SQLWCHAR *TableType, SQLSMALLINT NameLength4)
{
    return SQL_ERROR;
}

static SQLRETURN WINAPI null_SQLTransact(SQLHENV EnvironmentHandle, SQLHDBC ConnectionHandle, SQLUSMALLINT CompletionType)
{
    return SQL_ERROR;
}

static struct sql_funcs sql_funcs =
{
    null_SQLAllocConnect,
    null_SQLAllocEnv,
    null_SQLAllocHandle,
    null_SQLAllocHandleStd,
    null_SQLAllocStmt,
    null_SQLBindCol,
    null_SQLBindParam,
    null_SQLBindParameter,
    null_SQLBrowseConnect,
    null_SQLBrowseConnectW,
    null_SQLBulkOperations,
    null_SQLCancel,
    null_SQLCloseCursor,
    null_SQLColAttribute,
    null_SQLColAttributeW,
    null_SQLColAttributes,
    null_SQLColAttributesW,
    null_SQLColumnPrivileges,
    null_SQLColumnPrivilegesW,
    null_SQLColumns,
    null_SQLColumnsW,
    null_SQLConnect,
    null_SQLConnectW,
    null_SQLCopyDesc,
    null_SQLDataSources,
    null_SQLDataSourcesA,
    null_SQLDataSourcesW,
    null_SQLDescribeCol,
    null_SQLDescribeColW,
    null_SQLDescribeParam,
    null_SQLDisconnect,
    null_SQLDriverConnect,
    null_SQLDriverConnectW,
    null_SQLDrivers,
    null_SQLDriversW,
    null_SQLEndTran,
    null_SQLError,
    null_SQLErrorW,
    null_SQLExecDirect,
    null_SQLExecDirectW,
    null_SQLExecute,
    null_SQLExtendedFetch,
    null_SQLFetch,
    null_SQLFetchScroll,
    null_SQLForeignKeys,
    null_SQLForeignKeysW,
    null_SQLFreeConnect,
    null_SQLFreeEnv,
    null_SQLFreeHandle,
    null_SQLFreeStmt,
    null_SQLGetConnectAttr,
    null_SQLGetConnectAttrW,
    null_SQLGetConnectOption,
    null_SQLGetConnectOptionW,
    null_SQLGetCursorName,
    null_SQLGetCursorNameW,
    null_SQLGetData,
    null_SQLGetDescField,
    null_SQLGetDescFieldW,
    null_SQLGetDescRec,
    null_SQLGetDescRecW,
    null_SQLGetDiagField,
    null_SQLGetDiagFieldW,
    null_SQLGetDiagRec,
    null_SQLGetDiagRecA,
    null_SQLGetDiagRecW,
    null_SQLGetEnvAttr,
    null_SQLGetFunctions,
    null_SQLGetInfo,
    null_SQLGetInfoW,
    null_SQLGetStmtAttr,
    null_SQLGetStmtAttrW,
    null_SQLGetStmtOption,
    null_SQLGetTypeInfo,
    null_SQLGetTypeInfoW,
    null_SQLMoreResults,
    null_SQLNativeSql,
    null_SQLNativeSqlW,
    null_SQLNumParams,
    null_SQLNumResultCols,
    null_SQLParamData,
    null_SQLParamOptions,
    null_SQLPrepare,
    null_SQLPrepareW,
    null_SQLPrimaryKeys,
    null_SQLPrimaryKeysW,
    null_SQLProcedureColumns,
    null_SQLProcedureColumnsW,
    null_SQLProcedures,
    null_SQLProceduresW,
    null_SQLPutData,
    null_SQLRowCount,
    null_SQLSetConnectAttr,
    null_SQLSetConnectAttrW,
    null_SQLSetConnectOption,
    null_SQLSetConnectOptionW,
    null_SQLSetCursorName,
    null_SQLSetCursorNameW,
    null_SQLSetDescField,
    null_SQLSetDescFieldW,
    null_SQLSetDescRec,
    null_SQLSetEnvAttr,
    null_SQLSetParam,
    null_SQLSetPos,
    null_SQLSetScrollOptions,
    null_SQLSetStmtAttr,
    null_SQLSetStmtAttrW,
    null_SQLSetStmtOption,
    null_SQLSpecialColumns,
    null_SQLSpecialColumnsW,
    null_SQLStatistics,
    null_SQLStatisticsW,
    null_SQLTablePrivileges,
    null_SQLTablePrivilegesW,
    null_SQLTables,
    null_SQLTablesW,
    null_SQLTransact
};


/***********************************************************************
 * ODBC_ReplicateODBCInstToRegistry
 *
 * PARAMS
 *
 * RETURNS
 *
 * Utility to ODBC_ReplicateToRegistry to replicate the drivers of the
 * ODBCINST.INI settings
 *
 * The driver settings are not replicated to the registry.  If we were to 
 * replicate them we would need to decide whether to replicate all settings
 * or to do some translation; whether to remove any entries present only in
 * the windows registry, etc.
 */

static void ODBC_ReplicateODBCInstToRegistry (SQLHENV hEnv)
{
    HKEY hODBCInst;
    LONG reg_ret;
    BOOL success;

    success = FALSE;
    TRACE ("Driver settings are not currently replicated to the registry\n");
    if ((reg_ret = RegCreateKeyExA (HKEY_LOCAL_MACHINE,
            "Software\\ODBC\\ODBCINST.INI", 0, NULL,
            REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS /* a couple more than we need */, NULL,
            &hODBCInst, NULL)) == ERROR_SUCCESS)
    {
        HKEY hDrivers;
        if ((reg_ret = RegCreateKeyExA (hODBCInst, "ODBC Drivers", 0,
                NULL, REG_OPTION_NON_VOLATILE,
                KEY_ALL_ACCESS /* overkill */, NULL, &hDrivers, NULL))
                == ERROR_SUCCESS)
        {
            SQLRETURN sql_ret;
            SQLUSMALLINT dirn;
            CHAR desc [256];
            SQLSMALLINT sizedesc;

            success = TRUE;
            dirn = SQL_FETCH_FIRST;
            while ((sql_ret = SQLDrivers (hEnv, dirn, (SQLCHAR*)desc, sizeof(desc),
                    &sizedesc, NULL, 0, NULL)) == SQL_SUCCESS ||
                    sql_ret == SQL_SUCCESS_WITH_INFO)
            {
                /* FIXME Do some proper handling of the SUCCESS_WITH_INFO */
                dirn = SQL_FETCH_NEXT;
                if (sizedesc == lstrlenA(desc))
                {
                    HKEY hThis;
                    if ((reg_ret = RegQueryValueExA (hDrivers, desc, NULL,
                            NULL, NULL, NULL)) == ERROR_FILE_NOT_FOUND)
                    {
                        if ((reg_ret = RegSetValueExA (hDrivers, desc, 0,
                                REG_SZ, (const BYTE *)"Installed", 10)) != ERROR_SUCCESS)
                        {
                            TRACE ("Error %d replicating driver %s\n",
                                    reg_ret, desc);
                            success = FALSE;
                        }
                    }
                    else if (reg_ret != ERROR_SUCCESS)
                    {
                        TRACE ("Error %d checking for %s in drivers\n",
                                reg_ret, desc);
                        success = FALSE;
                    }
                    if ((reg_ret = RegCreateKeyExA (hODBCInst, desc, 0,
                            NULL, REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS, NULL, &hThis, NULL))
                            == ERROR_SUCCESS)
                    {
                        /* FIXME This is where the settings go.
                         * I suggest that if the disposition says it 
                         * exists then we leave it alone.  Alternatively
                         * include an extra value to flag that it is 
                         * a replication of the unixODBC/iODBC/...
                         */
                        if ((reg_ret = RegCloseKey (hThis)) !=
                                ERROR_SUCCESS)
                            TRACE ("Error %d closing %s key\n", reg_ret,
                                    desc);
                    }
                    else
                    {
                        TRACE ("Error %d ensuring driver key %s\n",
                                reg_ret, desc);
                        success = FALSE;
                    }
                }
                else
                {
                    WARN ("Unusually long driver name %s not replicated\n",
                            desc);
                    success = FALSE;
                }
            }
            if (sql_ret != SQL_NO_DATA)
            {
                TRACE ("Error %d enumerating drivers\n", (int)sql_ret);
                success = FALSE;
            }
            if ((reg_ret = RegCloseKey (hDrivers)) != ERROR_SUCCESS)
            {
                TRACE ("Error %d closing hDrivers\n", reg_ret);
            }
        }
        else
        {
            TRACE ("Error %d opening HKLM\\S\\O\\OI\\Drivers\n", reg_ret);
        }
        if ((reg_ret = RegCloseKey (hODBCInst)) != ERROR_SUCCESS)
        {
            TRACE ("Error %d closing HKLM\\S\\O\\ODBCINST.INI\n", reg_ret);
        }
    }
    else
    {
        TRACE ("Error %d opening HKLM\\S\\O\\ODBCINST.INI\n", reg_ret);
    }
    if (!success)
    {
        WARN ("May not have replicated all ODBC drivers to the registry\n");
    }
}

/***********************************************************************
 * ODBC_ReplicateODBCToRegistry
 *
 * PARAMS
 *
 * RETURNS
 *
 * Utility to ODBC_ReplicateToRegistry to replicate either the USER or 
 * SYSTEM dsns
 *
 * For now simply place the "Driver description" (as returned by SQLDataSources)
 * into the registry as the driver.  This is enough to satisfy Crystal's 
 * requirement that there be a driver entry.  (It doesn't seem to care what
 * the setting is).
 * A slightly more accurate setting would be to access the registry to find
 * the actual driver library for the given description (which appears to map
 * to one of the HKLM/Software/ODBC/ODBCINST.INI keys).  (If you do this note
 * that this will add a requirement that this function be called after
 * ODBC_ReplicateODBCInstToRegistry)
 */
static void ODBC_ReplicateODBCToRegistry (BOOL is_user, SQLHENV hEnv)
{
    HKEY hODBC;
    LONG reg_ret;
    SQLRETURN sql_ret;
    SQLUSMALLINT dirn;
    CHAR dsn [SQL_MAX_DSN_LENGTH + 1];
    SQLSMALLINT sizedsn;
    CHAR desc [256];
    SQLSMALLINT sizedesc;
    BOOL success;
    const char *which = is_user ? "user" : "system";

    success = FALSE;
    if ((reg_ret = RegCreateKeyExA (
            is_user ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE,
            "Software\\ODBC\\ODBC.INI", 0, NULL, REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS /* a couple more than we need */, NULL, &hODBC,
            NULL)) == ERROR_SUCCESS)
    {
        success = TRUE;
        dirn = is_user ? SQL_FETCH_FIRST_USER : SQL_FETCH_FIRST_SYSTEM;
        while ((sql_ret = SQLDataSources (hEnv, dirn,
                (SQLCHAR*)dsn, sizeof(dsn), &sizedsn,
                (SQLCHAR*)desc, sizeof(desc), &sizedesc)) == SQL_SUCCESS
                || sql_ret == SQL_SUCCESS_WITH_INFO)
        {
            /* FIXME Do some proper handling of the SUCCESS_WITH_INFO */
            dirn = SQL_FETCH_NEXT;
            if (sizedsn == lstrlenA(dsn) && sizedesc == lstrlenA(desc))
            {
                HKEY hDSN;
                if ((reg_ret = RegCreateKeyExA (hODBC, dsn, 0,
                        NULL, REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS, NULL, &hDSN, NULL))
                        == ERROR_SUCCESS)
                {
                    static const char DRIVERKEY[] = "Driver";
                    if ((reg_ret = RegQueryValueExA (hDSN, DRIVERKEY,
                            NULL, NULL, NULL, NULL))
                            == ERROR_FILE_NOT_FOUND)
                    {
                        if ((reg_ret = RegSetValueExA (hDSN, DRIVERKEY, 0,
                                REG_SZ, (LPBYTE)desc, sizedesc)) != ERROR_SUCCESS)
                        {
                            TRACE ("Error %d replicating description of "
                                    "%s(%s)\n", reg_ret, dsn, desc);
                            success = FALSE;
                        }
                    }
                    else if (reg_ret != ERROR_SUCCESS)
                    {
                        TRACE ("Error %d checking for description of %s\n",
                                reg_ret, dsn);
                        success = FALSE;
                    }
                    if ((reg_ret = RegCloseKey (hDSN)) != ERROR_SUCCESS)
                    {
                        TRACE ("Error %d closing %s DSN key %s\n",
                                reg_ret, which, dsn);
                    }
                }
                else
                {
                    TRACE ("Error %d opening %s DSN key %s\n",
                            reg_ret, which, dsn);
                    success = FALSE;
                }
            }
            else
            {
                WARN ("Unusually long %s data source name %s (%s) not "
                        "replicated\n", which, dsn, desc);
                success = FALSE;
            }
        }
        if (sql_ret != SQL_NO_DATA)
        {
            TRACE ("Error %d enumerating %s datasources\n",
                    (int)sql_ret, which);
            success = FALSE;
        }
        if ((reg_ret = RegCloseKey (hODBC)) != ERROR_SUCCESS)
        {
            TRACE ("Error %d closing %s ODBC.INI registry key\n", reg_ret,
                    which);
        }
    }
    else
    {
        TRACE ("Error %d creating/opening %s ODBC.INI registry key\n",
                reg_ret, which);
    }
    if (!success)
    {
        WARN ("May not have replicated all %s ODBC DSNs to the registry\n",
                which);
    }
}

/***********************************************************************
 * ODBC_ReplicateToRegistry
 *
 * PARAMS
 *
 * RETURNS
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

static void ODBC_ReplicateToRegistry (void)
{
    SQLRETURN sql_ret;
    SQLHENV hEnv;

    if ((sql_ret = SQLAllocEnv(&hEnv)) == SQL_SUCCESS)
    {
        ODBC_ReplicateODBCInstToRegistry (hEnv);
        ODBC_ReplicateODBCToRegistry (FALSE /* system dsns */, hEnv);
        ODBC_ReplicateODBCToRegistry (TRUE /* user dsns */, hEnv);

        if ((sql_ret = SQLFreeEnv(hEnv)) != SQL_SUCCESS)
        {
            TRACE ("Error %d freeing the SQL environment.\n", (int)sql_ret);
        }
    }
    else
    {
        TRACE ("Error %d opening an SQL environment.\n", (int)sql_ret);
        WARN ("The external ODBC settings have not been replicated to the"
                " Wine registry\n");
    }
}

/*************************************************************************
 *				SQLAllocConnect           [ODBC32.001]
 */
SQLRETURN WINAPI SQLAllocConnect(SQLHENV EnvironmentHandle, SQLHDBC *ConnectionHandle)
{
    SQLRETURN ret;

    TRACE("(EnvironmentHandle %p, ConnectionHandle %p)\n", EnvironmentHandle, ConnectionHandle);

    ret = sql_funcs.pSQLAllocConnect(EnvironmentHandle, ConnectionHandle);
    TRACE("Returning %d, ConnectionHandle %p\n", ret, *ConnectionHandle);
    return ret;
}

/*************************************************************************
 *				SQLAllocEnv           [ODBC32.002]
 */
SQLRETURN WINAPI SQLAllocEnv(SQLHENV *EnvironmentHandle)
{
    SQLRETURN ret;

    TRACE("(EnvironmentHandle %p)\n", EnvironmentHandle);

    ret = sql_funcs.pSQLAllocEnv(EnvironmentHandle);
    TRACE("Returning %d, EnvironmentHandle %p\n", ret, *EnvironmentHandle);
    return ret;
}

/*************************************************************************
 *				SQLAllocHandle           [ODBC32.024]
 */
SQLRETURN WINAPI SQLAllocHandle(SQLSMALLINT HandleType, SQLHANDLE InputHandle, SQLHANDLE *OutputHandle)
{
    SQLRETURN ret;

    TRACE("(HandleType %d, InputHandle %p, OutputHandle %p)\n", HandleType, InputHandle, OutputHandle);

    ret = sql_funcs.pSQLAllocHandle(HandleType, InputHandle, OutputHandle);
    TRACE("Returning %d, Handle %p\n", ret, *OutputHandle);
    return ret;
}

/*************************************************************************
 *				SQLAllocStmt           [ODBC32.003]
 */
SQLRETURN WINAPI SQLAllocStmt(SQLHDBC ConnectionHandle, SQLHSTMT *StatementHandle)
{
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p, StatementHandle %p)\n", ConnectionHandle, StatementHandle);

    ret = sql_funcs.pSQLAllocStmt(ConnectionHandle, StatementHandle);
    TRACE ("Returning %d, StatementHandle %p\n", ret, *StatementHandle);
    return ret;
}

/*************************************************************************
 *				SQLAllocHandleStd           [ODBC32.077]
 */
SQLRETURN WINAPI SQLAllocHandleStd(SQLSMALLINT HandleType, SQLHANDLE InputHandle, SQLHANDLE *OutputHandle)
{
    SQLRETURN ret;

    TRACE("(HandleType %d, InputHandle %p, OutputHandle %p)\n", HandleType, InputHandle, OutputHandle);

    ret = sql_funcs.pSQLAllocHandleStd(HandleType, InputHandle, OutputHandle);
    TRACE ("Returning %d, OutputHandle %p\n", ret, *OutputHandle);
    return ret;
}

static const char *debugstr_sqllen( SQLLEN len )
{
#ifdef _WIN64
    return wine_dbg_sprintf( "%ld", len );
#else
    return wine_dbg_sprintf( "%d", len );
#endif
}

/*************************************************************************
 *				SQLBindCol           [ODBC32.004]
 */
SQLRETURN WINAPI SQLBindCol(SQLHSTMT StatementHandle, SQLUSMALLINT ColumnNumber, SQLSMALLINT TargetType,
                            SQLPOINTER TargetValue, SQLLEN BufferLength, SQLLEN *StrLen_or_Ind)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, ColumnNumber %d, TargetType %d, TargetValue %p, BufferLength %s, StrLen_or_Ind %p)\n",
          StatementHandle, ColumnNumber, TargetType, TargetValue, debugstr_sqllen(BufferLength), StrLen_or_Ind);

    ret = sql_funcs.pSQLBindCol(StatementHandle, ColumnNumber, TargetType, TargetValue, BufferLength, StrLen_or_Ind);
    TRACE ("Returning %d\n", ret);
    return ret;
}

static const char *debugstr_sqlulen( SQLULEN len )
{
#ifdef _WIN64
    return wine_dbg_sprintf( "%lu", len );
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
    SQLRETURN ret;

    TRACE("(StatementHandle %p, ParameterNumber %d, ValueType %d, ParameterType %d, LengthPrecision %s,"
          " ParameterScale %d, ParameterValue %p, StrLen_or_Ind %p)\n", StatementHandle, ParameterNumber, ValueType,
          ParameterType, debugstr_sqlulen(LengthPrecision), ParameterScale, ParameterValue, StrLen_or_Ind);

    ret = sql_funcs.pSQLBindParam(StatementHandle, ParameterNumber, ValueType, ParameterType, LengthPrecision, ParameterScale,
                                  ParameterValue, StrLen_or_Ind);
    TRACE ("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLCancel           [ODBC32.005]
 */
SQLRETURN WINAPI SQLCancel(SQLHSTMT StatementHandle)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p)\n", StatementHandle);

    ret = sql_funcs.pSQLCancel(StatementHandle);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLCloseCursor           [ODBC32.026]
 */
SQLRETURN WINAPI SQLCloseCursor(SQLHSTMT StatementHandle)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p)\n", StatementHandle);

    ret = sql_funcs.pSQLCloseCursor(StatementHandle);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLColAttribute           [ODBC32.027]
 */
SQLRETURN WINAPI SQLColAttribute(SQLHSTMT StatementHandle, SQLUSMALLINT ColumnNumber,
                                 SQLUSMALLINT FieldIdentifier, SQLPOINTER CharacterAttribute,
                                 SQLSMALLINT BufferLength, SQLSMALLINT *StringLength,
                                 SQLLEN *NumericAttribute)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, ColumnNumber %d, FieldIdentifier %d, CharacterAttribute %p, BufferLength %d,"
          " StringLength %p, NumericAttribute %p)\n", StatementHandle, ColumnNumber, FieldIdentifier,
          CharacterAttribute, BufferLength, StringLength, NumericAttribute);

    ret = sql_funcs.pSQLColAttribute(StatementHandle, ColumnNumber, FieldIdentifier, CharacterAttribute, BufferLength,
                                     StringLength, NumericAttribute);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLColumns           [ODBC32.040]
 */
SQLRETURN WINAPI SQLColumns(SQLHSTMT StatementHandle, SQLCHAR *CatalogName, SQLSMALLINT NameLength1,
                            SQLCHAR *SchemaName, SQLSMALLINT NameLength2, SQLCHAR *TableName,
                            SQLSMALLINT NameLength3, SQLCHAR *ColumnName, SQLSMALLINT NameLength4)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d, TableName %s,"
          " NameLength3 %d, ColumnName %s, NameLength4 %d)\n", StatementHandle,
          debugstr_an((const char *)CatalogName, NameLength1), NameLength1,
          debugstr_an((const char *)SchemaName, NameLength2), NameLength2,
          debugstr_an((const char *)TableName, NameLength3), NameLength3,
          debugstr_an((const char *)ColumnName, NameLength4), NameLength4);

    ret = sql_funcs.pSQLColumns(StatementHandle, CatalogName, NameLength1, SchemaName, NameLength2, TableName,
                                NameLength3, ColumnName, NameLength4);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLConnect           [ODBC32.007]
 */
SQLRETURN WINAPI SQLConnect(SQLHDBC ConnectionHandle, SQLCHAR *ServerName, SQLSMALLINT NameLength1,
                            SQLCHAR *UserName, SQLSMALLINT NameLength2, SQLCHAR *Authentication,
                            SQLSMALLINT NameLength3)
{
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p, ServerName %s, NameLength1 %d, UserName %s, NameLength2 %d, Authentication %s,"
          " NameLength3 %d)\n", ConnectionHandle,
          debugstr_an((const char *)ServerName, NameLength1), NameLength1,
          debugstr_an((const char *)UserName, NameLength2), NameLength2,
          debugstr_an((const char *)Authentication, NameLength3), NameLength3);

    ret = sql_funcs.pSQLConnect(ConnectionHandle, ServerName, NameLength1, UserName, NameLength2, Authentication, NameLength3);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLCopyDesc           [ODBC32.028]
 */
SQLRETURN WINAPI SQLCopyDesc(SQLHDESC SourceDescHandle, SQLHDESC TargetDescHandle)
{
    SQLRETURN ret;

    TRACE("(SourceDescHandle %p, TargetDescHandle %p)\n", SourceDescHandle, TargetDescHandle);

    ret = sql_funcs.pSQLCopyDesc(SourceDescHandle, TargetDescHandle);
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
    SQLRETURN ret;

    TRACE("(EnvironmentHandle %p, Direction %d, ServerName %p, BufferLength1 %d, NameLength1 %p, Description %p,"
          " BufferLength2 %d, NameLength2 %p)\n", EnvironmentHandle, Direction, ServerName, BufferLength1,
          NameLength1, Description, BufferLength2, NameLength2);

    ret = sql_funcs.pSQLDataSources(EnvironmentHandle, Direction, ServerName, BufferLength1, NameLength1, Description,
                                    BufferLength2, NameLength2);
    if (ret >= 0 && TRACE_ON(odbc))
    {
        if (ServerName && NameLength1 && *NameLength1 > 0)
            TRACE(" DataSource %s", debugstr_an((const char *)ServerName, *NameLength1));
        if (Description && NameLength2 && *NameLength2 > 0)
            TRACE(" Description %s", debugstr_an((const char *)Description, *NameLength2));
        TRACE("\n");
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

SQLRETURN WINAPI SQLDataSourcesA(SQLHENV EnvironmentHandle, SQLUSMALLINT Direction, SQLCHAR *ServerName,
                                 SQLSMALLINT BufferLength1, SQLSMALLINT *NameLength1, SQLCHAR *Description,
                                 SQLSMALLINT BufferLength2, SQLSMALLINT *NameLength2)
{
    SQLRETURN ret;

    TRACE("(EnvironmentHandle %p, Direction %d, ServerName %p, BufferLength1 %d, NameLength1 %p, Description %p,"
          " BufferLength2 %d, NameLength2 %p)\n", EnvironmentHandle, Direction, ServerName, BufferLength1,
          NameLength1, Description, BufferLength2, NameLength2);

    ret = sql_funcs.pSQLDataSourcesA(EnvironmentHandle, Direction, ServerName, BufferLength1, NameLength1, Description,
                                     BufferLength2, NameLength2);
    if (TRACE_ON(odbc))
    {
       if (ServerName && NameLength1 && *NameLength1 > 0)
            TRACE(" DataSource %s", debugstr_an((const char *)ServerName, *NameLength1));
       if (Description && NameLength2 && *NameLength2 > 0)
            TRACE(" Description %s", debugstr_an((const char *)Description, *NameLength2));
       TRACE("\n");
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLDescribeCol           [ODBC32.008]
 */
SQLRETURN WINAPI SQLDescribeCol(SQLHSTMT StatementHandle, SQLUSMALLINT ColumnNumber, SQLCHAR *ColumnName,
                                SQLSMALLINT BufferLength, SQLSMALLINT *NameLength, SQLSMALLINT *DataType,
                                SQLULEN *ColumnSize, SQLSMALLINT *DecimalDigits, SQLSMALLINT *Nullable)
{
    SQLSMALLINT dummy;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, ColumnNumber %d, ColumnName %p, BufferLength %d, NameLength %p, DataType %p,"
          " ColumnSize %p, DecimalDigits %p, Nullable %p)\n", StatementHandle, ColumnNumber, ColumnName,
          BufferLength, NameLength, DataType, ColumnSize, DecimalDigits, Nullable);

    if (!NameLength) NameLength = &dummy; /* workaround for drivers that don't accept NULL NameLength */

    ret = sql_funcs.pSQLDescribeCol(StatementHandle, ColumnNumber, ColumnName, BufferLength, NameLength, DataType, ColumnSize,
                                    DecimalDigits, Nullable);
    if (ret >= 0)
    {
        if (ColumnName && NameLength) TRACE(" ColumnName %s\n", debugstr_an((const char *)ColumnName, *NameLength));
        if (DataType) TRACE(" DataType %d\n", *DataType);
        if (ColumnSize) TRACE(" ColumnSize %s\n", debugstr_sqlulen(*ColumnSize));
        if (DecimalDigits) TRACE(" DecimalDigits %d\n", *DecimalDigits);
        if (Nullable) TRACE(" Nullable %d\n", *Nullable);
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLDisconnect           [ODBC32.009]
 */
SQLRETURN WINAPI SQLDisconnect(SQLHDBC ConnectionHandle)
{
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p)\n", ConnectionHandle);

    ret = sql_funcs.pSQLDisconnect(ConnectionHandle);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLEndTran           [ODBC32.029]
 */
SQLRETURN WINAPI SQLEndTran(SQLSMALLINT HandleType, SQLHANDLE Handle, SQLSMALLINT CompletionType)
{
    SQLRETURN ret;

    TRACE("(HandleType %d, Handle %p, CompletionType %d)\n", HandleType, Handle, CompletionType);

    ret = sql_funcs.pSQLEndTran(HandleType, Handle, CompletionType);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLError           [ODBC32.010]
 */
SQLRETURN WINAPI SQLError(SQLHENV EnvironmentHandle, SQLHDBC ConnectionHandle, SQLHSTMT StatementHandle,
                          SQLCHAR *Sqlstate, SQLINTEGER *NativeError, SQLCHAR *MessageText,
                          SQLSMALLINT BufferLength, SQLSMALLINT *TextLength)
{
    SQLRETURN ret;

    TRACE("(EnvironmentHandle %p, ConnectionHandle %p, StatementHandle %p, Sqlstate %p, NativeError %p,"
          " MessageText %p, BufferLength %d, TextLength %p)\n", EnvironmentHandle, ConnectionHandle,
          StatementHandle, Sqlstate, NativeError, MessageText, BufferLength, TextLength);

    ret = sql_funcs.pSQLError(EnvironmentHandle, ConnectionHandle, StatementHandle, Sqlstate, NativeError, MessageText,
                              BufferLength, TextLength);

    if (ret == SQL_SUCCESS)
    {
        TRACE(" SQLState %s\n", debugstr_an((const char *)Sqlstate, 5));
        TRACE(" Error %d\n", *NativeError);
        TRACE(" MessageText %s\n", debugstr_an((const char *)MessageText, *TextLength));
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLExecDirect           [ODBC32.011]
 */
SQLRETURN WINAPI SQLExecDirect(SQLHSTMT StatementHandle, SQLCHAR *StatementText, SQLINTEGER TextLength)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, StatementText %s, TextLength %d)\n", StatementHandle,
          debugstr_an((const char *)StatementText, TextLength), TextLength);

    ret = sql_funcs.pSQLExecDirect(StatementHandle, StatementText, TextLength);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLExecute           [ODBC32.012]
 */
SQLRETURN WINAPI SQLExecute(SQLHSTMT StatementHandle)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p)\n", StatementHandle);

    ret = sql_funcs.pSQLExecute(StatementHandle);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLFetch           [ODBC32.013]
 */
SQLRETURN WINAPI SQLFetch(SQLHSTMT StatementHandle)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p)\n", StatementHandle);

    ret = sql_funcs.pSQLFetch(StatementHandle);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLFetchScroll          [ODBC32.030]
 */
SQLRETURN WINAPI SQLFetchScroll(SQLHSTMT StatementHandle, SQLSMALLINT FetchOrientation, SQLLEN FetchOffset)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, FetchOrientation %d, FetchOffset %s)\n", StatementHandle, FetchOrientation,
          debugstr_sqllen(FetchOffset));

    ret = sql_funcs.pSQLFetchScroll(StatementHandle, FetchOrientation, FetchOffset);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLFreeConnect           [ODBC32.014]
 */
SQLRETURN WINAPI SQLFreeConnect(SQLHDBC ConnectionHandle)
{
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p)\n", ConnectionHandle);

    ret = sql_funcs.pSQLFreeConnect(ConnectionHandle);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLFreeEnv           [ODBC32.015]
 */
SQLRETURN WINAPI SQLFreeEnv(SQLHENV EnvironmentHandle)
{
    SQLRETURN ret;

    TRACE("(EnvironmentHandle %p)\n", EnvironmentHandle);

    ret = sql_funcs.pSQLFreeEnv(EnvironmentHandle);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLFreeHandle           [ODBC32.031]
 */
SQLRETURN WINAPI SQLFreeHandle(SQLSMALLINT HandleType, SQLHANDLE Handle)
{
    SQLRETURN ret;

    TRACE("(HandleType %d, Handle %p)\n", HandleType, Handle);

    ret = sql_funcs.pSQLFreeHandle(HandleType, Handle);
    TRACE ("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLFreeStmt           [ODBC32.016]
 */
SQLRETURN WINAPI SQLFreeStmt(SQLHSTMT StatementHandle, SQLUSMALLINT Option)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, Option %d)\n", StatementHandle, Option);

    ret = sql_funcs.pSQLFreeStmt(StatementHandle, Option);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetConnectAttr           [ODBC32.032]
 */
SQLRETURN WINAPI SQLGetConnectAttr(SQLHDBC ConnectionHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                   SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p, Attribute %d, Value %p, BufferLength %d, StringLength %p)\n", ConnectionHandle,
          Attribute, Value, BufferLength, StringLength);

    ret = sql_funcs.pSQLGetConnectAttr(ConnectionHandle, Attribute, Value, BufferLength, StringLength);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetConnectOption       [ODBC32.042]
 */
SQLRETURN WINAPI SQLGetConnectOption(SQLHDBC ConnectionHandle, SQLUSMALLINT Option, SQLPOINTER Value)
{
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p, Option %d, Value %p)\n", ConnectionHandle, Option, Value);

    ret = sql_funcs.pSQLGetConnectOption(ConnectionHandle, Option, Value);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetCursorName           [ODBC32.017]
 */
SQLRETURN WINAPI SQLGetCursorName(SQLHSTMT StatementHandle, SQLCHAR *CursorName, SQLSMALLINT BufferLength,
                                  SQLSMALLINT *NameLength)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, CursorName %p, BufferLength %d, NameLength %p)\n", StatementHandle, CursorName,
          BufferLength, NameLength);

    ret = sql_funcs.pSQLGetCursorName(StatementHandle, CursorName, BufferLength, NameLength);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetData           [ODBC32.043]
 */
SQLRETURN WINAPI SQLGetData(SQLHSTMT StatementHandle, SQLUSMALLINT ColumnNumber, SQLSMALLINT TargetType,
                            SQLPOINTER TargetValue, SQLLEN BufferLength, SQLLEN *StrLen_or_Ind)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, ColumnNumber %d, TargetType %d, TargetValue %p, BufferLength %s, StrLen_or_Ind %p)\n",
          StatementHandle, ColumnNumber, TargetType, TargetValue, debugstr_sqllen(BufferLength), StrLen_or_Ind);

    ret = sql_funcs.pSQLGetData(StatementHandle, ColumnNumber, TargetType, TargetValue, BufferLength, StrLen_or_Ind);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetDescField           [ODBC32.033]
 */
SQLRETURN WINAPI SQLGetDescField(SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber, SQLSMALLINT FieldIdentifier,
                                 SQLPOINTER Value, SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
    SQLRETURN ret;

    TRACE("(DescriptorHandle %p, RecNumber %d, FieldIdentifier %d, Value %p, BufferLength %d, StringLength %p)\n",
          DescriptorHandle, RecNumber, FieldIdentifier, Value, BufferLength, StringLength);

    ret = sql_funcs.pSQLGetDescField(DescriptorHandle, RecNumber, FieldIdentifier, Value, BufferLength, StringLength);
    TRACE("Returning %d\n", ret);
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
    SQLRETURN ret;

    TRACE("(DescriptorHandle %p, RecNumber %d, Name %p, BufferLength %d, StringLength %p, Type %p, SubType %p,"
          " Length %p, Precision %p, Scale %p, Nullable %p)\n", DescriptorHandle, RecNumber, Name, BufferLength,
          StringLength, Type, SubType, Length, Precision, Scale, Nullable);

    ret = sql_funcs.pSQLGetDescRec(DescriptorHandle, RecNumber, Name, BufferLength, StringLength, Type, SubType, Length,
                                   Precision, Scale, Nullable);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetDiagField           [ODBC32.035]
 */
SQLRETURN WINAPI SQLGetDiagField(SQLSMALLINT HandleType, SQLHANDLE Handle, SQLSMALLINT RecNumber,
                                 SQLSMALLINT DiagIdentifier, SQLPOINTER DiagInfo, SQLSMALLINT BufferLength,
                                 SQLSMALLINT *StringLength)
{
    SQLRETURN ret;

    TRACE("(HandleType %d, Handle %p, RecNumber %d, DiagIdentifier %d, DiagInfo %p, BufferLength %d,"
          " StringLength %p)\n", HandleType, Handle, RecNumber, DiagIdentifier, DiagInfo, BufferLength, StringLength);

    ret = sql_funcs.pSQLGetDiagField(HandleType, Handle, RecNumber, DiagIdentifier, DiagInfo, BufferLength, StringLength);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetDiagRec           [ODBC32.036]
 */
SQLRETURN WINAPI SQLGetDiagRec(SQLSMALLINT HandleType, SQLHANDLE Handle, SQLSMALLINT RecNumber,
                               SQLCHAR *Sqlstate, SQLINTEGER *NativeError, SQLCHAR *MessageText,
                               SQLSMALLINT BufferLength, SQLSMALLINT *TextLength)
{
    SQLRETURN ret;

    TRACE("(HandleType %d, Handle %p, RecNumber %d, Sqlstate %p, NativeError %p, MessageText %p, BufferLength %d,"
          " TextLength %p)\n", HandleType, Handle, RecNumber, Sqlstate, NativeError, MessageText, BufferLength,
          TextLength);

    ret = sql_funcs.pSQLGetDiagRec(HandleType, Handle, RecNumber, Sqlstate, NativeError, MessageText, BufferLength, TextLength);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetEnvAttr           [ODBC32.037]
 */
SQLRETURN WINAPI SQLGetEnvAttr(SQLHENV EnvironmentHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                               SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
    SQLRETURN ret;

    TRACE("(EnvironmentHandle %p, Attribute %d, Value %p, BufferLength %d, StringLength %p)\n",
          EnvironmentHandle, Attribute, Value, BufferLength, StringLength);

    ret = sql_funcs.pSQLGetEnvAttr(EnvironmentHandle, Attribute, Value, BufferLength, StringLength);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetFunctions           [ODBC32.044]
 */
SQLRETURN WINAPI SQLGetFunctions(SQLHDBC ConnectionHandle, SQLUSMALLINT FunctionId, SQLUSMALLINT *Supported)
{
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p, FunctionId %d, Supported %p)\n", ConnectionHandle, FunctionId, Supported);

    ret = sql_funcs.pSQLGetFunctions(ConnectionHandle, FunctionId, Supported);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetInfo           [ODBC32.045]
 */
SQLRETURN WINAPI SQLGetInfo(SQLHDBC ConnectionHandle, SQLUSMALLINT InfoType, SQLPOINTER InfoValue,
                            SQLSMALLINT BufferLength, SQLSMALLINT *StringLength)
{
    SQLRETURN ret;

    TRACE("(ConnectionHandle, %p, InfoType %d, InfoValue %p, BufferLength %d, StringLength %p)\n", ConnectionHandle,
          InfoType, InfoValue, BufferLength, StringLength);

    if (!InfoValue)
    {
        WARN("Unexpected NULL InfoValue address\n");
        return SQL_ERROR;
    }

    ret = sql_funcs.pSQLGetInfo(ConnectionHandle, InfoType, InfoValue, BufferLength, StringLength);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetStmtAttr           [ODBC32.038]
 */
SQLRETURN WINAPI SQLGetStmtAttr(SQLHSTMT StatementHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, Attribute %d, Value %p, BufferLength %d, StringLength %p)\n", StatementHandle,
          Attribute, Value, BufferLength, StringLength);

    if (!Value)
    {
        WARN("Unexpected NULL Value return address\n");
        return SQL_ERROR;
    }

    ret = sql_funcs.pSQLGetStmtAttr(StatementHandle, Attribute, Value, BufferLength, StringLength);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetStmtOption           [ODBC32.046]
 */
SQLRETURN WINAPI SQLGetStmtOption(SQLHSTMT StatementHandle, SQLUSMALLINT Option, SQLPOINTER Value)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, Option %d, Value %p)\n", StatementHandle, Option, Value);

    ret = sql_funcs.pSQLGetStmtOption(StatementHandle, Option, Value);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetTypeInfo           [ODBC32.047]
 */
SQLRETURN WINAPI SQLGetTypeInfo(SQLHSTMT StatementHandle, SQLSMALLINT DataType)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, DataType %d)\n", StatementHandle, DataType);

    ret = sql_funcs.pSQLGetTypeInfo(StatementHandle, DataType);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLNumResultCols           [ODBC32.018]
 */
SQLRETURN WINAPI SQLNumResultCols(SQLHSTMT StatementHandle, SQLSMALLINT *ColumnCount)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, ColumnCount %p)\n", StatementHandle, ColumnCount);

    ret = sql_funcs.pSQLNumResultCols(StatementHandle, ColumnCount);
    TRACE("Returning %d ColumnCount %d\n", ret, *ColumnCount);
    return ret;
}

/*************************************************************************
 *				SQLParamData           [ODBC32.048]
 */
SQLRETURN WINAPI SQLParamData(SQLHSTMT StatementHandle, SQLPOINTER *Value)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, Value %p)\n", StatementHandle, Value);

    ret = sql_funcs.pSQLParamData(StatementHandle, Value);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLPrepare           [ODBC32.019]
 */
SQLRETURN WINAPI SQLPrepare(SQLHSTMT StatementHandle, SQLCHAR *StatementText, SQLINTEGER TextLength)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, StatementText %s, TextLength %d)\n", StatementHandle,
          debugstr_an((const char *)StatementText, TextLength), TextLength);

    ret = sql_funcs.pSQLPrepare(StatementHandle, StatementText, TextLength);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLPutData           [ODBC32.049]
 */
SQLRETURN WINAPI SQLPutData(SQLHSTMT StatementHandle, SQLPOINTER Data, SQLLEN StrLen_or_Ind)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, Data %p, StrLen_or_Ind %s)\n", StatementHandle, Data, debugstr_sqllen(StrLen_or_Ind));

    ret = sql_funcs.pSQLPutData(StatementHandle, Data, StrLen_or_Ind);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLRowCount           [ODBC32.020]
 */
SQLRETURN WINAPI SQLRowCount(SQLHSTMT StatementHandle, SQLLEN *RowCount)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, RowCount %p)\n", StatementHandle, RowCount);

    ret = sql_funcs.pSQLRowCount(StatementHandle, RowCount);
    if (ret == SQL_SUCCESS && RowCount) TRACE(" RowCount %s\n", debugstr_sqllen(*RowCount));
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetConnectAttr           [ODBC32.039]
 */
SQLRETURN WINAPI SQLSetConnectAttr(SQLHDBC ConnectionHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                   SQLINTEGER StringLength)
{
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p, Attribute %d, Value %p, StringLength %d)\n", ConnectionHandle, Attribute, Value,
          StringLength);

    ret = sql_funcs.pSQLSetConnectAttr(ConnectionHandle, Attribute, Value, StringLength);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetConnectOption           [ODBC32.050]
 */
SQLRETURN WINAPI SQLSetConnectOption(SQLHDBC ConnectionHandle, SQLUSMALLINT Option, SQLULEN Value)
{
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p, Option %d, Value %s)\n", ConnectionHandle, Option, debugstr_sqlulen(Value));

    ret = sql_funcs.pSQLSetConnectOption(ConnectionHandle, Option, Value);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetCursorName           [ODBC32.021]
 */
SQLRETURN WINAPI SQLSetCursorName(SQLHSTMT StatementHandle, SQLCHAR *CursorName, SQLSMALLINT NameLength)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, CursorName %s, NameLength %d)\n", StatementHandle,
          debugstr_an((const char *)CursorName, NameLength), NameLength);

    ret = sql_funcs.pSQLSetCursorName(StatementHandle, CursorName, NameLength);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetDescField           [ODBC32.073]
 */
SQLRETURN WINAPI SQLSetDescField(SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber, SQLSMALLINT FieldIdentifier,
                                 SQLPOINTER Value, SQLINTEGER BufferLength)
{
    SQLRETURN ret;

    TRACE("(DescriptorHandle %p, RecNumber %d, FieldIdentifier %d, Value %p, BufferLength %d)\n", DescriptorHandle,
          RecNumber, FieldIdentifier, Value, BufferLength);

    ret = sql_funcs.pSQLSetDescField(DescriptorHandle, RecNumber, FieldIdentifier, Value, BufferLength);
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
    SQLRETURN ret;

    TRACE("(DescriptorHandle %p, RecNumber %d, Type %d, SubType %d, Length %s, Precision %d, Scale %d, Data %p,"
          " StringLength %p, Indicator %p)\n", DescriptorHandle, RecNumber, Type, SubType, debugstr_sqllen(Length),
          Precision, Scale, Data, StringLength, Indicator);

    ret = sql_funcs.pSQLSetDescRec(DescriptorHandle, RecNumber, Type, SubType, Length, Precision, Scale, Data,
                                   StringLength, Indicator);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetEnvAttr           [ODBC32.075]
 */
SQLRETURN WINAPI SQLSetEnvAttr(SQLHENV EnvironmentHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                               SQLINTEGER StringLength)
{
    SQLRETURN ret;

    TRACE("(EnvironmentHandle %p, Attribute %d, Value %p, StringLength %d)\n", EnvironmentHandle, Attribute, Value,
          StringLength);

    ret = sql_funcs.pSQLSetEnvAttr(EnvironmentHandle, Attribute, Value, StringLength);
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
    SQLRETURN ret;

    TRACE("(StatementHandle %p, ParameterNumber %d, ValueType %d, ParameterType %d, LengthPrecision %s,"
          " ParameterScale %d, ParameterValue %p, StrLen_or_Ind %p)\n", StatementHandle, ParameterNumber, ValueType,
          ParameterType, debugstr_sqlulen(LengthPrecision), ParameterScale, ParameterValue, StrLen_or_Ind);

    ret = sql_funcs.pSQLSetParam(StatementHandle, ParameterNumber, ValueType, ParameterType, LengthPrecision,
                                 ParameterScale, ParameterValue, StrLen_or_Ind);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetStmtAttr           [ODBC32.076]
 */
SQLRETURN WINAPI SQLSetStmtAttr(SQLHSTMT StatementHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                SQLINTEGER StringLength)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, Attribute %d, Value %p, StringLength %d)\n", StatementHandle, Attribute, Value,
          StringLength);

    ret = sql_funcs.pSQLSetStmtAttr(StatementHandle, Attribute, Value, StringLength);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetStmtOption           [ODBC32.051]
 */
SQLRETURN WINAPI SQLSetStmtOption(SQLHSTMT StatementHandle, SQLUSMALLINT Option, SQLULEN Value)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, Option %d, Value %s)\n", StatementHandle, Option, debugstr_sqlulen(Value));

    ret = sql_funcs.pSQLSetStmtOption(StatementHandle, Option, Value);
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
    SQLRETURN ret;

    TRACE("(StatementHandle %p, IdentifierType %d, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d,"
          " TableName %s, NameLength3 %d, Scope %d, Nullable %d)\n", StatementHandle, IdentifierType,
          debugstr_an((const char *)CatalogName, NameLength1), NameLength1,
          debugstr_an((const char *)SchemaName, NameLength2), NameLength2,
          debugstr_an((const char *)TableName, NameLength3), NameLength3, Scope, Nullable);

    ret = sql_funcs.pSQLSpecialColumns(StatementHandle, IdentifierType, CatalogName, NameLength1, SchemaName,
                                       NameLength2, TableName, NameLength3, Scope, Nullable);
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
    SQLRETURN ret;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d SchemaName %s, NameLength2 %d, TableName %s"
          " NameLength3 %d, Unique %d, Reserved %d)\n", StatementHandle,
          debugstr_an((const char *)CatalogName, NameLength1), NameLength1,
          debugstr_an((const char *)SchemaName, NameLength2), NameLength2,
          debugstr_an((const char *)TableName, NameLength3), NameLength3, Unique, Reserved);

    ret = sql_funcs.pSQLStatistics(StatementHandle, CatalogName, NameLength1, SchemaName, NameLength2, TableName,
                                   NameLength3, Unique, Reserved);
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
    SQLRETURN ret;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d, TableName %s,"
          " NameLength3 %d, TableType %s, NameLength4 %d)\n", StatementHandle,
          debugstr_an((const char *)CatalogName, NameLength1), NameLength1,
          debugstr_an((const char *)SchemaName, NameLength2), NameLength2,
          debugstr_an((const char *)TableName, NameLength3), NameLength3,
          debugstr_an((const char *)TableType, NameLength4), NameLength4);

    ret = sql_funcs.pSQLTables(StatementHandle, CatalogName, NameLength1, SchemaName, NameLength2, TableName, NameLength3,
                               TableType, NameLength4);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLTransact           [ODBC32.023]
 */
SQLRETURN WINAPI SQLTransact(SQLHENV EnvironmentHandle, SQLHDBC ConnectionHandle, SQLUSMALLINT CompletionType)
{
    SQLRETURN ret;

    TRACE("(EnvironmentHandle %p, ConnectionHandle %p, CompletionType %d)\n", EnvironmentHandle, ConnectionHandle,
          CompletionType);

    ret = sql_funcs.pSQLTransact(EnvironmentHandle, ConnectionHandle, CompletionType);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLBrowseConnect           [ODBC32.055]
 */
SQLRETURN WINAPI SQLBrowseConnect(SQLHDBC hdbc, SQLCHAR *szConnStrIn, SQLSMALLINT cbConnStrIn,
                                  SQLCHAR *szConnStrOut, SQLSMALLINT cbConnStrOutMax,
                                  SQLSMALLINT *pcbConnStrOut)
{
    SQLRETURN ret;

    TRACE("(hdbc %p, szConnStrIn %s, cbConnStrIn %d, szConnStrOut %p, cbConnStrOutMax %d, pcbConnStrOut %p)\n",
          hdbc, debugstr_an((const char *)szConnStrIn, cbConnStrIn), cbConnStrIn, szConnStrOut, cbConnStrOutMax,
          pcbConnStrOut);

    ret = sql_funcs.pSQLBrowseConnect(hdbc, szConnStrIn, cbConnStrIn, szConnStrOut, cbConnStrOutMax, pcbConnStrOut);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLBulkOperations           [ODBC32.078]
 */
SQLRETURN WINAPI SQLBulkOperations(SQLHSTMT StatementHandle, SQLSMALLINT Operation)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, Operation %d)\n", StatementHandle, Operation);

    ret = sql_funcs.pSQLBulkOperations(StatementHandle, Operation);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLColAttributes           [ODBC32.006]
 */
SQLRETURN WINAPI SQLColAttributes(SQLHSTMT hstmt, SQLUSMALLINT icol, SQLUSMALLINT fDescType,
                                  SQLPOINTER rgbDesc, SQLSMALLINT cbDescMax, SQLSMALLINT *pcbDesc,
                                  SQLLEN *pfDesc)
{
    SQLRETURN ret;

    TRACE("(hstmt %p, icol %d, fDescType %d, rgbDesc %p, cbDescMax %d, pcbDesc %p, pfDesc %p)\n", hstmt, icol,
          fDescType, rgbDesc, cbDescMax, pcbDesc, pfDesc);

    ret = sql_funcs.pSQLColAttributes(hstmt, icol, fDescType, rgbDesc, cbDescMax, pcbDesc, pfDesc);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLColumnPrivileges           [ODBC32.056]
 */
SQLRETURN WINAPI SQLColumnPrivileges(SQLHSTMT hstmt, SQLCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
                                     SQLCHAR *szSchemaName, SQLSMALLINT cbSchemaName, SQLCHAR *szTableName,
                                     SQLSMALLINT cbTableName, SQLCHAR *szColumnName, SQLSMALLINT cbColumnName)
{
    SQLRETURN ret;

    TRACE("(hstmt %p, szCatalogName %s, cbCatalogName %d, szSchemaName %s, cbSchemaName %d, szTableName %s,"
          " cbTableName %d, szColumnName %s, cbColumnName %d)\n", hstmt,
          debugstr_an((const char *)szCatalogName, cbCatalogName), cbCatalogName,
          debugstr_an((const char *)szSchemaName, cbSchemaName), cbSchemaName,
          debugstr_an((const char *)szTableName, cbTableName), cbTableName,
          debugstr_an((const char *)szColumnName, cbColumnName), cbColumnName);

    ret = sql_funcs.pSQLColumnPrivileges(hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName,
                                         szTableName, cbTableName, szColumnName, cbColumnName);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLDescribeParam          [ODBC32.058]
 */
SQLRETURN WINAPI SQLDescribeParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, SQLSMALLINT *pfSqlType,
                                  SQLULEN *pcbParamDef, SQLSMALLINT *pibScale, SQLSMALLINT *pfNullable)
{
    SQLRETURN ret;

    TRACE("(hstmt %p, ipar %d, pfSqlType %p, pcbParamDef %p, pibScale %p, pfNullable %p)\n", hstmt, ipar,
          pfSqlType, pcbParamDef, pibScale, pfNullable);

    ret = sql_funcs.pSQLDescribeParam(hstmt, ipar, pfSqlType, pcbParamDef, pibScale, pfNullable);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLExtendedFetch           [ODBC32.059]
 */
SQLRETURN WINAPI SQLExtendedFetch(SQLHSTMT hstmt, SQLUSMALLINT fFetchType, SQLLEN irow, SQLULEN *pcrow,
                                  SQLUSMALLINT *rgfRowStatus)
{
    SQLRETURN ret;

    TRACE("(hstmt %p, fFetchType %d, irow %s, pcrow %p, rgfRowStatus %p)\n", hstmt, fFetchType, debugstr_sqllen(irow),
          pcrow, rgfRowStatus);

    ret = sql_funcs.pSQLExtendedFetch(hstmt, fFetchType, irow, pcrow, rgfRowStatus);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLForeignKeys           [ODBC32.060]
 */
SQLRETURN WINAPI SQLForeignKeys(SQLHSTMT hstmt, SQLCHAR *szPkCatalogName, SQLSMALLINT cbPkCatalogName,
                                SQLCHAR *szPkSchemaName, SQLSMALLINT cbPkSchemaName, SQLCHAR *szPkTableName,
                                SQLSMALLINT cbPkTableName, SQLCHAR *szFkCatalogName,
                                SQLSMALLINT cbFkCatalogName, SQLCHAR *szFkSchemaName,
                                SQLSMALLINT cbFkSchemaName, SQLCHAR *szFkTableName, SQLSMALLINT cbFkTableName)
{
    SQLRETURN ret;

    TRACE("(hstmt %p, szPkCatalogName %s, cbPkCatalogName %d, szPkSchemaName %s, cbPkSchemaName %d,"
          " szPkTableName %s, cbPkTableName %d, szFkCatalogName %s, cbFkCatalogName %d, szFkSchemaName %s,"
          " cbFkSchemaName %d, szFkTableName %s, cbFkTableName %d)\n", hstmt,
          debugstr_an((const char *)szPkCatalogName, cbPkCatalogName), cbPkCatalogName,
          debugstr_an((const char *)szPkSchemaName, cbPkSchemaName), cbPkSchemaName,
          debugstr_an((const char *)szPkTableName, cbPkTableName), cbPkTableName,
          debugstr_an((const char *)szFkCatalogName, cbFkCatalogName), cbFkCatalogName,
          debugstr_an((const char *)szFkSchemaName, cbFkSchemaName), cbFkSchemaName,
          debugstr_an((const char *)szFkTableName, cbFkTableName), cbFkTableName);

    ret = sql_funcs.pSQLForeignKeys(hstmt, szPkCatalogName, cbPkCatalogName, szPkSchemaName, cbPkSchemaName, szPkTableName,
                                    cbPkTableName, szFkCatalogName, cbFkCatalogName, szFkSchemaName, cbFkSchemaName,
                                    szFkTableName, cbFkTableName);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLMoreResults           [ODBC32.061]
 */
SQLRETURN WINAPI SQLMoreResults(SQLHSTMT StatementHandle)
{
    SQLRETURN ret;

    TRACE("(%p)\n", StatementHandle);

    ret = sql_funcs.pSQLMoreResults(StatementHandle);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLNativeSql           [ODBC32.062]
 */
SQLRETURN WINAPI SQLNativeSql(SQLHDBC hdbc, SQLCHAR *szSqlStrIn, SQLINTEGER cbSqlStrIn, SQLCHAR *szSqlStr,
                              SQLINTEGER cbSqlStrMax, SQLINTEGER *pcbSqlStr)
{
    SQLRETURN ret;

    TRACE("(hdbc %p, szSqlStrIn %s, cbSqlStrIn %d, szSqlStr %p, cbSqlStrMax %d, pcbSqlStr %p)\n", hdbc,
          debugstr_an((const char *)szSqlStrIn, cbSqlStrIn), cbSqlStrIn, szSqlStr, cbSqlStrMax, pcbSqlStr);

    ret = sql_funcs.pSQLNativeSql(hdbc, szSqlStrIn, cbSqlStrIn, szSqlStr, cbSqlStrMax, pcbSqlStr);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLNumParams           [ODBC32.063]
 */
SQLRETURN WINAPI SQLNumParams(SQLHSTMT hstmt, SQLSMALLINT *pcpar)
{
    SQLRETURN ret;

    TRACE("(hstmt %p, pcpar %p)\n", hstmt, pcpar);

    ret = sql_funcs.pSQLNumParams(hstmt, pcpar);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLParamOptions           [ODBC32.064]
 */
SQLRETURN WINAPI SQLParamOptions(SQLHSTMT hstmt, SQLULEN crow, SQLULEN *pirow)
{
    SQLRETURN ret;

    TRACE("(hstmt %p, crow %s, pirow %p)\n", hstmt, debugstr_sqlulen(crow), pirow);

    ret = sql_funcs.pSQLParamOptions(hstmt, crow, pirow);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLPrimaryKeys           [ODBC32.065]
 */
SQLRETURN WINAPI SQLPrimaryKeys(SQLHSTMT hstmt, SQLCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
                                SQLCHAR *szSchemaName, SQLSMALLINT cbSchemaName, SQLCHAR *szTableName,
                                SQLSMALLINT cbTableName)
{
    SQLRETURN ret;

    TRACE("(hstmt %p, szCatalogName %s, cbCatalogName %d, szSchemaName %s, cbSchemaName %d, szTableName %s,"
          " cbTableName %d)\n", hstmt,
          debugstr_an((const char *)szCatalogName, cbCatalogName), cbCatalogName,
          debugstr_an((const char *)szSchemaName, cbSchemaName), cbSchemaName,
          debugstr_an((const char *)szTableName, cbTableName), cbTableName);

    ret = sql_funcs.pSQLPrimaryKeys(hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName, szTableName, cbTableName);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLProcedureColumns           [ODBC32.066]
 */
SQLRETURN WINAPI SQLProcedureColumns(SQLHSTMT hstmt, SQLCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
                                     SQLCHAR *szSchemaName, SQLSMALLINT cbSchemaName, SQLCHAR *szProcName,
                                     SQLSMALLINT cbProcName, SQLCHAR *szColumnName, SQLSMALLINT cbColumnName)
{
    SQLRETURN ret;

    TRACE("(hstmt %p, szCatalogName %s, cbCatalogName %d, szSchemaName %s, cbSchemaName %d, szProcName %s,"
          " cbProcName %d, szColumnName %s, cbColumnName %d)\n", hstmt,
          debugstr_an((const char *)szCatalogName, cbCatalogName), cbCatalogName,
          debugstr_an((const char *)szSchemaName, cbSchemaName), cbSchemaName,
          debugstr_an((const char *)szProcName, cbProcName), cbProcName,
          debugstr_an((const char *)szColumnName, cbColumnName), cbColumnName);

    ret = sql_funcs.pSQLProcedureColumns(hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName, szProcName,
                                         cbProcName, szColumnName, cbColumnName);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLProcedures           [ODBC32.067]
 */
SQLRETURN WINAPI SQLProcedures(SQLHSTMT hstmt, SQLCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
                               SQLCHAR *szSchemaName, SQLSMALLINT cbSchemaName, SQLCHAR *szProcName,
                               SQLSMALLINT cbProcName)
{
    SQLRETURN ret;

    TRACE("(hstmt %p, szCatalogName %s, cbCatalogName %d, szSchemaName %s, cbSchemaName %d, szProcName %s,"
          " cbProcName %d)\n", hstmt,
          debugstr_an((const char *)szCatalogName, cbCatalogName), cbCatalogName,
          debugstr_an((const char *)szSchemaName, cbSchemaName), cbSchemaName,
          debugstr_an((const char *)szProcName, cbProcName), cbProcName);

    ret = sql_funcs.pSQLProcedures(hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName, szProcName, cbProcName);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetPos           [ODBC32.068]
 */
SQLRETURN WINAPI SQLSetPos(SQLHSTMT hstmt, SQLSETPOSIROW irow, SQLUSMALLINT fOption, SQLUSMALLINT fLock)
{
    SQLRETURN ret;

    TRACE("(hstmt %p, irow %s, fOption %d, fLock %d)\n", hstmt, debugstr_sqlulen(irow), fOption, fLock);

    ret = sql_funcs.pSQLSetPos(hstmt, irow, fOption, fLock);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLTablePrivileges           [ODBC32.070]
 */
SQLRETURN WINAPI SQLTablePrivileges(SQLHSTMT hstmt, SQLCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
                                    SQLCHAR *szSchemaName, SQLSMALLINT cbSchemaName, SQLCHAR *szTableName,
                                    SQLSMALLINT cbTableName)
{
    SQLRETURN ret;

    TRACE("(hstmt %p, szCatalogName %s, cbCatalogName %d, szSchemaName %s, cbSchemaName %d, szTableName %s,"
          " cbTableName %d)\n", hstmt,
          debugstr_an((const char *)szCatalogName, cbCatalogName), cbCatalogName,
          debugstr_an((const char *)szSchemaName, cbSchemaName), cbSchemaName,
          debugstr_an((const char *)szTableName, cbTableName), cbTableName);

    ret = sql_funcs.pSQLTablePrivileges(hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName, szTableName,
                                        cbTableName);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLDrivers           [ODBC32.071]
 */
SQLRETURN WINAPI SQLDrivers(SQLHENV EnvironmentHandle, SQLUSMALLINT fDirection, SQLCHAR *szDriverDesc,
                            SQLSMALLINT cbDriverDescMax, SQLSMALLINT *pcbDriverDesc,
                            SQLCHAR *szDriverAttributes, SQLSMALLINT cbDriverAttrMax,
                            SQLSMALLINT *pcbDriverAttr)
{
    SQLRETURN ret;

    TRACE("(EnvironmentHandle %p, Direction %d, szDriverDesc %p, cbDriverDescMax %d, pcbDriverDesc %p,"
          " DriverAttributes %p, cbDriverAttrMax %d, pcbDriverAttr %p)\n", EnvironmentHandle, fDirection,
          szDriverDesc, cbDriverDescMax, pcbDriverDesc, szDriverAttributes, cbDriverAttrMax, pcbDriverAttr);

    ret = sql_funcs.pSQLDrivers(EnvironmentHandle, fDirection, szDriverDesc, cbDriverDescMax, pcbDriverDesc,
                                szDriverAttributes, cbDriverAttrMax, pcbDriverAttr);

    if (ret == SQL_NO_DATA && fDirection == SQL_FETCH_FIRST)
        ERR_(winediag)("No ODBC drivers could be found. Check the settings for your libodbc provider.\n");

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLBindParameter           [ODBC32.072]
 */
SQLRETURN WINAPI SQLBindParameter(SQLHSTMT hstmt, SQLUSMALLINT ipar, SQLSMALLINT fParamType,
                                  SQLSMALLINT fCType, SQLSMALLINT fSqlType, SQLULEN cbColDef,
                                  SQLSMALLINT ibScale, SQLPOINTER rgbValue, SQLLEN cbValueMax,
                                  SQLLEN *pcbValue)
{
    SQLRETURN ret;

    TRACE("(hstmt %p, ipar %d, fParamType %d, fCType %d, fSqlType %d, cbColDef %s, ibScale %d, rgbValue %p,"
          " cbValueMax %s, pcbValue %p)\n", hstmt, ipar, fParamType, fCType, fSqlType, debugstr_sqlulen(cbColDef),
          ibScale, rgbValue, debugstr_sqllen(cbValueMax), pcbValue);

    ret = sql_funcs.pSQLBindParameter(hstmt, ipar, fParamType, fCType, fSqlType, cbColDef, ibScale, rgbValue, cbValueMax,
                                      pcbValue);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLDriverConnect           [ODBC32.041]
 */
SQLRETURN WINAPI SQLDriverConnect(SQLHDBC hdbc, SQLHWND hwnd, SQLCHAR *ConnectionString, SQLSMALLINT Length,
                                  SQLCHAR *conn_str_out, SQLSMALLINT conn_str_out_max,
                                  SQLSMALLINT *ptr_conn_str_out, SQLUSMALLINT driver_completion)
{
    SQLRETURN ret;

    TRACE("(hdbc %p, hwnd %p, ConnectionString %s, Length %d, conn_str_out %p, conn_str_out_max %d,"
          " ptr_conn_str_out %p, driver_completion %d)\n", hdbc, hwnd,
          debugstr_an((const char *)ConnectionString, Length), Length, conn_str_out, conn_str_out_max,
          ptr_conn_str_out, driver_completion);

    ret = sql_funcs.pSQLDriverConnect(hdbc, hwnd, ConnectionString, Length, conn_str_out, conn_str_out_max,
                                      ptr_conn_str_out, driver_completion);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetScrollOptions           [ODBC32.069]
 */
SQLRETURN WINAPI SQLSetScrollOptions(SQLHSTMT statement_handle, SQLUSMALLINT f_concurrency, SQLLEN crow_keyset,
                                     SQLUSMALLINT crow_rowset)
{
    SQLRETURN ret;

    TRACE("(statement_handle %p, f_concurrency %d, crow_keyset %s, crow_rowset %d)\n", statement_handle,
          f_concurrency, debugstr_sqllen(crow_keyset), crow_rowset);

    ret = sql_funcs.pSQLSetScrollOptions(statement_handle, f_concurrency, crow_keyset, crow_rowset);
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

/*************************************************************************
 *				SQLColAttributesW          [ODBC32.106]
 */
SQLRETURN WINAPI SQLColAttributesW(SQLHSTMT hstmt, SQLUSMALLINT icol, SQLUSMALLINT fDescType,
                                   SQLPOINTER rgbDesc, SQLSMALLINT cbDescMax, SQLSMALLINT *pcbDesc,
                                   SQLLEN *pfDesc)
{
    SQLRETURN ret;

    TRACE("(hstmt %p, icol %d, fDescType %d, rgbDesc %p, cbDescMax %d, pcbDesc %p, pfDesc %p)\n", hstmt, icol,
          fDescType, rgbDesc, cbDescMax, pcbDesc, pfDesc);

    ret = sql_funcs.pSQLColAttributesW(hstmt, icol, fDescType, rgbDesc, cbDescMax, pcbDesc, pfDesc);

    if (ret == SQL_SUCCESS && SQLColAttributes_KnownStringAttribute(fDescType) && rgbDesc && pcbDesc &&
        *pcbDesc != lstrlenW(rgbDesc) * 2)
    {
        TRACE("CHEAT: resetting name length for ADO\n");
        *pcbDesc = lstrlenW(rgbDesc) * 2;
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLConnectW          [ODBC32.107]
 */
SQLRETURN WINAPI SQLConnectW(SQLHDBC ConnectionHandle, WCHAR *ServerName, SQLSMALLINT NameLength1,
                             WCHAR *UserName, SQLSMALLINT NameLength2, WCHAR *Authentication,
                             SQLSMALLINT NameLength3)
{
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p, ServerName %s, NameLength1 %d, UserName %s, NameLength2 %d, Authentication %s,"
          " NameLength3 %d)\n", ConnectionHandle, debugstr_wn(ServerName, NameLength1), NameLength1,
          debugstr_wn(UserName, NameLength2), NameLength2, debugstr_wn(Authentication, NameLength3), NameLength3);

    ret = sql_funcs.pSQLConnectW(ConnectionHandle, ServerName, NameLength1, UserName, NameLength2, Authentication, NameLength3);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLDescribeColW          [ODBC32.108]
 */
SQLRETURN WINAPI SQLDescribeColW(SQLHSTMT StatementHandle, SQLUSMALLINT ColumnNumber, WCHAR *ColumnName,
                                 SQLSMALLINT BufferLength, SQLSMALLINT *NameLength, SQLSMALLINT *DataType,
                                 SQLULEN *ColumnSize, SQLSMALLINT *DecimalDigits, SQLSMALLINT *Nullable)
{
    SQLSMALLINT dummy;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, ColumnNumber %d, ColumnName %p, BufferLength %d, NameLength %p, DataType %p,"
          " ColumnSize %p, DecimalDigits %p, Nullable %p)\n", StatementHandle, ColumnNumber, ColumnName,
          BufferLength, NameLength, DataType, ColumnSize, DecimalDigits, Nullable);

    if (!NameLength) NameLength = &dummy; /* workaround for drivers that don't accept NULL NameLength */

    ret = sql_funcs.pSQLDescribeColW(StatementHandle, ColumnNumber, ColumnName, BufferLength, NameLength, DataType, ColumnSize,
                                     DecimalDigits, Nullable);
    if (ret >= 0)
    {
        if (ColumnName && NameLength) TRACE("ColumnName %s\n", debugstr_wn(ColumnName, *NameLength));
        if (DataType) TRACE("DataType %d\n", *DataType);
        if (ColumnSize) TRACE("ColumnSize %s\n", debugstr_sqlulen(*ColumnSize));
        if (DecimalDigits) TRACE("DecimalDigits %d\n", *DecimalDigits);
        if (Nullable) TRACE("Nullable %d\n", *Nullable);
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLErrorW          [ODBC32.110]
 */
SQLRETURN WINAPI SQLErrorW(SQLHENV EnvironmentHandle, SQLHDBC ConnectionHandle, SQLHSTMT StatementHandle,
                           WCHAR *Sqlstate, SQLINTEGER *NativeError, WCHAR *MessageText,
                           SQLSMALLINT BufferLength, SQLSMALLINT *TextLength)
{
    SQLRETURN ret;

    TRACE("(EnvironmentHandle %p, ConnectionHandle %p, StatementHandle %p, Sqlstate %p, NativeError %p,"
          " MessageText %p, BufferLength %d, TextLength %p)\n", EnvironmentHandle, ConnectionHandle,
          StatementHandle, Sqlstate, NativeError, MessageText, BufferLength, TextLength);

    ret = sql_funcs.pSQLErrorW(EnvironmentHandle, ConnectionHandle, StatementHandle, Sqlstate, NativeError, MessageText,
                               BufferLength, TextLength);

    if (ret == SQL_SUCCESS)
    {
        TRACE(" SQLState %s\n", debugstr_wn(Sqlstate, 5));
        TRACE(" Error %d\n", *NativeError);
        TRACE(" MessageText %s\n", debugstr_wn(MessageText, *TextLength));
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLExecDirectW          [ODBC32.111]
 */
SQLRETURN WINAPI SQLExecDirectW(SQLHSTMT StatementHandle, WCHAR *StatementText, SQLINTEGER TextLength)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, StatementText %s, TextLength %d)\n", StatementHandle,
          debugstr_wn(StatementText, TextLength), TextLength);

    ret = sql_funcs.pSQLExecDirectW(StatementHandle, StatementText, TextLength);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetCursorNameW          [ODBC32.117]
 */
SQLRETURN WINAPI SQLGetCursorNameW(SQLHSTMT StatementHandle, WCHAR *CursorName, SQLSMALLINT BufferLength,
                                   SQLSMALLINT *NameLength)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, CursorName %p, BufferLength %d, NameLength %p)\n", StatementHandle, CursorName,
          BufferLength, NameLength);

    ret = sql_funcs.pSQLGetCursorNameW(StatementHandle, CursorName, BufferLength, NameLength);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLPrepareW          [ODBC32.119]
 */
SQLRETURN WINAPI SQLPrepareW(SQLHSTMT StatementHandle, WCHAR *StatementText, SQLINTEGER TextLength)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, StatementText %s, TextLength %d)\n", StatementHandle,
          debugstr_wn(StatementText, TextLength), TextLength);

    ret = sql_funcs.pSQLPrepareW(StatementHandle, StatementText, TextLength);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetCursorNameW          [ODBC32.121]
 */
SQLRETURN WINAPI SQLSetCursorNameW(SQLHSTMT StatementHandle, WCHAR *CursorName, SQLSMALLINT NameLength)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, CursorName %s, NameLength %d)\n", StatementHandle,
          debugstr_wn(CursorName, NameLength), NameLength);

    ret = sql_funcs.pSQLSetCursorNameW(StatementHandle, CursorName, NameLength);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLColAttributeW          [ODBC32.127]
 */
SQLRETURN WINAPI SQLColAttributeW(SQLHSTMT StatementHandle, SQLUSMALLINT ColumnNumber,
                                  SQLUSMALLINT FieldIdentifier, SQLPOINTER CharacterAttribute,
                                  SQLSMALLINT BufferLength, SQLSMALLINT *StringLength,
                                  SQLLEN *NumericAttribute)
{
    SQLRETURN ret;

    TRACE("StatementHandle %p ColumnNumber %d FieldIdentifier %d CharacterAttribute %p BufferLength %d"
          " StringLength %p NumericAttribute %p\n", StatementHandle, ColumnNumber, FieldIdentifier,
          CharacterAttribute, BufferLength, StringLength, NumericAttribute);

    ret = sql_funcs.pSQLColAttributeW(StatementHandle, ColumnNumber, FieldIdentifier, CharacterAttribute, BufferLength,
                                      StringLength, NumericAttribute);

    if (ret == SQL_SUCCESS && CharacterAttribute != NULL && SQLColAttributes_KnownStringAttribute(FieldIdentifier) &&
        StringLength && *StringLength != lstrlenW(CharacterAttribute) * 2)
    {
        TRACE("CHEAT: resetting name length for ADO\n");
        *StringLength = lstrlenW(CharacterAttribute) * 2;
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetConnectAttrW          [ODBC32.132]
 */
SQLRETURN WINAPI SQLGetConnectAttrW(SQLHDBC ConnectionHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                    SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p, Attribute %d, Value %p, BufferLength %d, StringLength %p)\n", ConnectionHandle,
          Attribute, Value, BufferLength, StringLength);

    ret = sql_funcs.pSQLGetConnectAttrW(ConnectionHandle, Attribute, Value, BufferLength, StringLength);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetDescFieldW          [ODBC32.133]
 */
SQLRETURN WINAPI SQLGetDescFieldW(SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber, SQLSMALLINT FieldIdentifier,
                                  SQLPOINTER Value, SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
    SQLRETURN ret;

    TRACE("(DescriptorHandle %p, RecNumber %d, FieldIdentifier %d, Value %p, BufferLength %d, StringLength %p)\n",
          DescriptorHandle, RecNumber, FieldIdentifier, Value, BufferLength, StringLength);

    ret = sql_funcs.pSQLGetDescFieldW(DescriptorHandle, RecNumber, FieldIdentifier, Value, BufferLength, StringLength);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetDescRecW          [ODBC32.134]
 */
SQLRETURN WINAPI SQLGetDescRecW(SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber, WCHAR *Name,
                                SQLSMALLINT BufferLength, SQLSMALLINT *StringLength, SQLSMALLINT *Type,
                                SQLSMALLINT *SubType, SQLLEN *Length, SQLSMALLINT *Precision,
                                SQLSMALLINT *Scale, SQLSMALLINT *Nullable)
{
    SQLRETURN ret;

    TRACE("(DescriptorHandle %p, RecNumber %d, Name %p, BufferLength %d, StringLength %p, Type %p, SubType %p,"
          " Length %p, Precision %p, Scale %p, Nullable %p)\n", DescriptorHandle, RecNumber, Name, BufferLength,
          StringLength, Type, SubType, Length, Precision, Scale, Nullable);

    ret = sql_funcs.pSQLGetDescRecW(DescriptorHandle, RecNumber, Name, BufferLength, StringLength, Type, SubType, Length,
                                    Precision, Scale, Nullable);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetDiagFieldW          [ODBC32.135]
 */
SQLRETURN WINAPI SQLGetDiagFieldW(SQLSMALLINT HandleType, SQLHANDLE Handle, SQLSMALLINT RecNumber,
                                  SQLSMALLINT DiagIdentifier, SQLPOINTER DiagInfo, SQLSMALLINT BufferLength,
                                  SQLSMALLINT *StringLength)
{
    SQLRETURN ret;

    TRACE("(HandleType %d, Handle %p, RecNumber %d, DiagIdentifier %d, DiagInfo %p, BufferLength %d,"
          " StringLength %p)\n", HandleType, Handle, RecNumber, DiagIdentifier, DiagInfo, BufferLength, StringLength);

    ret = sql_funcs.pSQLGetDiagFieldW(HandleType, Handle, RecNumber, DiagIdentifier, DiagInfo, BufferLength, StringLength);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetDiagRecW           [ODBC32.136]
 */
SQLRETURN WINAPI SQLGetDiagRecW(SQLSMALLINT HandleType, SQLHANDLE Handle, SQLSMALLINT RecNumber,
                                WCHAR *Sqlstate, SQLINTEGER *NativeError, WCHAR *MessageText,
                                SQLSMALLINT BufferLength, SQLSMALLINT *TextLength)
{
    SQLRETURN ret;

    TRACE("(HandleType %d, Handle %p, RecNumber %d, Sqlstate %p, NativeError %p, MessageText %p, BufferLength %d,"
          " TextLength %p)\n", HandleType, Handle, RecNumber, Sqlstate, NativeError, MessageText, BufferLength,
          TextLength);

    ret = sql_funcs.pSQLGetDiagRecW(HandleType, Handle, RecNumber, Sqlstate, NativeError, MessageText, BufferLength, TextLength);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetStmtAttrW          [ODBC32.138]
 */
SQLRETURN WINAPI SQLGetStmtAttrW(SQLHSTMT StatementHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                 SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, Attribute %d, Value %p, BufferLength %d, StringLength %p)\n", StatementHandle,
          Attribute, Value, BufferLength, StringLength);

    if (!Value)
    {
        WARN("Unexpected NULL Value return address\n");
        return SQL_ERROR;
    }

    ret = sql_funcs.pSQLGetStmtAttrW(StatementHandle, Attribute, Value, BufferLength, StringLength);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetConnectAttrW          [ODBC32.139]
 */
SQLRETURN WINAPI SQLSetConnectAttrW(SQLHDBC ConnectionHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                    SQLINTEGER StringLength)
{
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p, Attribute %d, Value %p, StringLength %d)\n", ConnectionHandle, Attribute, Value,
          StringLength);

    ret = sql_funcs.pSQLSetConnectAttrW(ConnectionHandle, Attribute, Value, StringLength);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLColumnsW          [ODBC32.140]
 */
SQLRETURN WINAPI SQLColumnsW(SQLHSTMT StatementHandle, WCHAR *CatalogName, SQLSMALLINT NameLength1,
                             WCHAR *SchemaName, SQLSMALLINT NameLength2, WCHAR *TableName,
                             SQLSMALLINT NameLength3, WCHAR *ColumnName, SQLSMALLINT NameLength4)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d, TableName %s,"
          " NameLength3 %d, ColumnName %s, NameLength4 %d)\n", StatementHandle,
          debugstr_wn(CatalogName, NameLength1), NameLength1, debugstr_wn(SchemaName, NameLength2), NameLength2,
          debugstr_wn(TableName, NameLength3), NameLength3, debugstr_wn(ColumnName, NameLength4), NameLength4);

    ret = sql_funcs.pSQLColumnsW(StatementHandle, CatalogName, NameLength1, SchemaName, NameLength2, TableName, NameLength3,
                                 ColumnName, NameLength4);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLDriverConnectW          [ODBC32.141]
 */
SQLRETURN WINAPI SQLDriverConnectW(SQLHDBC ConnectionHandle, SQLHWND WindowHandle, WCHAR *InConnectionString,
                                   SQLSMALLINT Length, WCHAR *OutConnectionString, SQLSMALLINT BufferLength,
                                   SQLSMALLINT *Length2, SQLUSMALLINT DriverCompletion)
{
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p, WindowHandle %p, InConnectionString %s, Length %d, OutConnectionString %p,"
          " BufferLength %d, Length2 %p, DriverCompletion %d)\n", ConnectionHandle, WindowHandle,
          debugstr_wn(InConnectionString, Length), Length, OutConnectionString, BufferLength, Length2,
          DriverCompletion);

    ret = sql_funcs.pSQLDriverConnectW(ConnectionHandle, WindowHandle, InConnectionString, Length, OutConnectionString,
                                       BufferLength, Length2, DriverCompletion);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetConnectOptionW      [ODBC32.142]
 */
SQLRETURN WINAPI SQLGetConnectOptionW(SQLHDBC ConnectionHandle, SQLUSMALLINT Option, SQLPOINTER Value)
{
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p, Option %d, Value %p)\n", ConnectionHandle, Option, Value);

    ret = sql_funcs.pSQLGetConnectOptionW(ConnectionHandle, Option, Value);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetInfoW          [ODBC32.145]
 */
SQLRETURN WINAPI SQLGetInfoW(SQLHDBC ConnectionHandle, SQLUSMALLINT InfoType, SQLPOINTER InfoValue,
                             SQLSMALLINT BufferLength, SQLSMALLINT *StringLength)
{
    SQLRETURN ret;

    TRACE("(ConnectionHandle, %p, InfoType %d, InfoValue %p, BufferLength %d, StringLength %p)\n", ConnectionHandle,
          InfoType, InfoValue, BufferLength, StringLength);

    if (!InfoValue)
    {
        WARN("Unexpected NULL InfoValue address\n");
        return SQL_ERROR;
    }

    ret = sql_funcs.pSQLGetInfoW(ConnectionHandle, InfoType, InfoValue, BufferLength, StringLength);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetTypeInfoW          [ODBC32.147]
 */
SQLRETURN WINAPI SQLGetTypeInfoW(SQLHSTMT StatementHandle, SQLSMALLINT DataType)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, DataType %d)\n", StatementHandle, DataType);

    ret = sql_funcs.pSQLGetTypeInfoW(StatementHandle, DataType);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetConnectOptionW          [ODBC32.150]
 */
SQLRETURN WINAPI SQLSetConnectOptionW(SQLHDBC ConnectionHandle, SQLUSMALLINT Option, SQLLEN Value)
{
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p, Option %d, Value %s)\n", ConnectionHandle, Option, debugstr_sqllen(Value));

    ret = sql_funcs.pSQLSetConnectOptionW(ConnectionHandle, Option, Value);
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
    SQLRETURN ret;

    TRACE("(StatementHandle %p, IdentifierType %d, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d,"
          " TableName %s, NameLength3 %d, Scope %d, Nullable %d)\n", StatementHandle, IdentifierType,
          debugstr_wn(CatalogName, NameLength1), NameLength1, debugstr_wn(SchemaName, NameLength2), NameLength2,
          debugstr_wn(TableName, NameLength3), NameLength3, Scope, Nullable);

    ret = sql_funcs.pSQLSpecialColumnsW(StatementHandle, IdentifierType, CatalogName, NameLength1, SchemaName,
                                        NameLength2, TableName, NameLength3, Scope, Nullable);
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
    SQLRETURN ret;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d SchemaName %s, NameLength2 %d, TableName %s"
          " NameLength3 %d, Unique %d, Reserved %d)\n", StatementHandle,
          debugstr_wn(CatalogName, NameLength1), NameLength1, debugstr_wn(SchemaName, NameLength2), NameLength2,
          debugstr_wn(TableName, NameLength3), NameLength3, Unique, Reserved);

    ret = sql_funcs.pSQLStatisticsW(StatementHandle, CatalogName, NameLength1, SchemaName, NameLength2, TableName,
                          NameLength3, Unique, Reserved);
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
    SQLRETURN ret;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d, TableName %s,"
          " NameLength3 %d, TableType %s, NameLength4 %d)\n", StatementHandle,
          debugstr_wn(CatalogName, NameLength1), NameLength1, debugstr_wn(SchemaName, NameLength2), NameLength2,
          debugstr_wn(TableName, NameLength3), NameLength3, debugstr_wn(TableType, NameLength4), NameLength4);

    ret = sql_funcs.pSQLTablesW(StatementHandle, CatalogName, NameLength1, SchemaName, NameLength2, TableName, NameLength3,
                      TableType, NameLength4);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLBrowseConnectW          [ODBC32.155]
 */
SQLRETURN WINAPI SQLBrowseConnectW(SQLHDBC hdbc, SQLWCHAR *szConnStrIn, SQLSMALLINT cbConnStrIn,
                                   SQLWCHAR *szConnStrOut, SQLSMALLINT cbConnStrOutMax,
                                   SQLSMALLINT *pcbConnStrOut)
{
    SQLRETURN ret;

    TRACE("(hdbc %p, szConnStrIn %s, cbConnStrIn %d, szConnStrOut %p, cbConnStrOutMax %d, pcbConnStrOut %p)\n",
          hdbc, debugstr_wn(szConnStrIn, cbConnStrIn), cbConnStrIn, szConnStrOut, cbConnStrOutMax, pcbConnStrOut);

    ret = sql_funcs.pSQLBrowseConnectW(hdbc, szConnStrIn, cbConnStrIn, szConnStrOut, cbConnStrOutMax, pcbConnStrOut);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLColumnPrivilegesW          [ODBC32.156]
 */
SQLRETURN WINAPI SQLColumnPrivilegesW(SQLHSTMT hstmt, SQLWCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
                                      SQLWCHAR *szSchemaName, SQLSMALLINT cbSchemaName, SQLWCHAR *szTableName,
                                      SQLSMALLINT cbTableName, SQLWCHAR *szColumnName, SQLSMALLINT cbColumnName)
{
    SQLRETURN ret;

    TRACE("(hstmt %p, szCatalogName %s, cbCatalogName %d, szSchemaName %s, cbSchemaName %d, szTableName %s,"
          " cbTableName %d, szColumnName %s, cbColumnName %d)\n", hstmt,
          debugstr_wn(szCatalogName, cbCatalogName), cbCatalogName,
          debugstr_wn(szSchemaName, cbSchemaName), cbSchemaName,
          debugstr_wn(szTableName, cbTableName), cbTableName,
          debugstr_wn(szColumnName, cbColumnName), cbColumnName);

    ret = sql_funcs.pSQLColumnPrivilegesW(hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName, szTableName,
                                cbTableName, szColumnName, cbColumnName);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLDataSourcesW          [ODBC32.157]
 */
SQLRETURN WINAPI SQLDataSourcesW(SQLHENV EnvironmentHandle, SQLUSMALLINT Direction, WCHAR *ServerName,
                                 SQLSMALLINT BufferLength1, SQLSMALLINT *NameLength1, WCHAR *Description,
                                 SQLSMALLINT BufferLength2, SQLSMALLINT *NameLength2)
{
    SQLRETURN ret;

    TRACE("(EnvironmentHandle %p, Direction %d, ServerName %p, BufferLength1 %d, NameLength1 %p, Description %p,"
          " BufferLength2 %d, NameLength2 %p)\n", EnvironmentHandle, Direction, ServerName, BufferLength1,
          NameLength1, Description, BufferLength2, NameLength2);

    ret = sql_funcs.pSQLDataSourcesW(EnvironmentHandle, Direction, ServerName, BufferLength1, NameLength1, Description,
                           BufferLength2, NameLength2);

    if (ret >= 0 && TRACE_ON(odbc))
    {
        if (ServerName && NameLength1 && *NameLength1 > 0)
            TRACE(" DataSource %s", debugstr_wn(ServerName, *NameLength1));
        if (Description && NameLength2 && *NameLength2 > 0)
            TRACE(" Description %s", debugstr_wn(Description, *NameLength2));
        TRACE("\n");
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLForeignKeysW          [ODBC32.160]
 */
SQLRETURN WINAPI SQLForeignKeysW(SQLHSTMT hstmt, SQLWCHAR *szPkCatalogName, SQLSMALLINT cbPkCatalogName,
                                 SQLWCHAR *szPkSchemaName, SQLSMALLINT cbPkSchemaName, SQLWCHAR *szPkTableName,
                                 SQLSMALLINT cbPkTableName, SQLWCHAR *szFkCatalogName,
                                 SQLSMALLINT cbFkCatalogName, SQLWCHAR *szFkSchemaName,
                                 SQLSMALLINT cbFkSchemaName, SQLWCHAR *szFkTableName, SQLSMALLINT cbFkTableName)
{
    SQLRETURN ret;

    TRACE("(hstmt %p, szPkCatalogName %s, cbPkCatalogName %d, szPkSchemaName %s, cbPkSchemaName %d,"
          " szPkTableName %s, cbPkTableName %d, szFkCatalogName %s, cbFkCatalogName %d, szFkSchemaName %s,"
          " cbFkSchemaName %d, szFkTableName %s, cbFkTableName %d)\n", hstmt,
          debugstr_wn(szPkCatalogName, cbPkCatalogName), cbPkCatalogName,
          debugstr_wn(szPkSchemaName, cbPkSchemaName), cbPkSchemaName,
          debugstr_wn(szPkTableName, cbPkTableName), cbPkTableName,
          debugstr_wn(szFkCatalogName, cbFkCatalogName), cbFkCatalogName,
          debugstr_wn(szFkSchemaName, cbFkSchemaName), cbFkSchemaName,
          debugstr_wn(szFkTableName, cbFkTableName), cbFkTableName);

    ret = sql_funcs.pSQLForeignKeysW(hstmt, szPkCatalogName, cbPkCatalogName, szPkSchemaName, cbPkSchemaName, szPkTableName,
                           cbPkTableName, szFkCatalogName, cbFkCatalogName, szFkSchemaName, cbFkSchemaName,
                           szFkTableName, cbFkTableName);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLNativeSqlW          [ODBC32.162]
 */
SQLRETURN WINAPI SQLNativeSqlW(SQLHDBC hdbc, SQLWCHAR *szSqlStrIn, SQLINTEGER cbSqlStrIn, SQLWCHAR *szSqlStr,
                               SQLINTEGER cbSqlStrMax, SQLINTEGER *pcbSqlStr)
{
    SQLRETURN ret;

    TRACE("(hdbc %p, szSqlStrIn %s, cbSqlStrIn %d, szSqlStr %p, cbSqlStrMax %d, pcbSqlStr %p)\n", hdbc,
          debugstr_wn(szSqlStrIn, cbSqlStrIn), cbSqlStrIn, szSqlStr, cbSqlStrMax, pcbSqlStr);

    ret = sql_funcs.pSQLNativeSqlW(hdbc, szSqlStrIn, cbSqlStrIn, szSqlStr, cbSqlStrMax, pcbSqlStr);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLPrimaryKeysW          [ODBC32.165]
 */
SQLRETURN WINAPI SQLPrimaryKeysW(SQLHSTMT hstmt, SQLWCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
                                 SQLWCHAR *szSchemaName, SQLSMALLINT cbSchemaName, SQLWCHAR *szTableName,
                                 SQLSMALLINT cbTableName)
{
    SQLRETURN ret;

    TRACE("(hstmt %p, szCatalogName %s, cbCatalogName %d, szSchemaName %s, cbSchemaName %d, szTableName %s,"
          " cbTableName %d)\n", hstmt,
          debugstr_wn(szCatalogName, cbCatalogName), cbCatalogName,
          debugstr_wn(szSchemaName, cbSchemaName), cbSchemaName,
          debugstr_wn(szTableName, cbTableName), cbTableName);

    ret = sql_funcs.pSQLPrimaryKeysW(hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName, szTableName, cbTableName);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLProcedureColumnsW          [ODBC32.166]
 */
SQLRETURN WINAPI SQLProcedureColumnsW(SQLHSTMT hstmt, SQLWCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
                                      SQLWCHAR *szSchemaName, SQLSMALLINT cbSchemaName, SQLWCHAR *szProcName,
                                      SQLSMALLINT cbProcName, SQLWCHAR *szColumnName, SQLSMALLINT cbColumnName)
{
    SQLRETURN ret;

    TRACE("(hstmt %p, szCatalogName %s, cbCatalogName %d, szSchemaName %s, cbSchemaName %d, szProcName %s,"
          " cbProcName %d, szColumnName %s, cbColumnName %d)\n", hstmt,
          debugstr_wn(szCatalogName, cbCatalogName), cbCatalogName,
          debugstr_wn(szSchemaName, cbSchemaName), cbSchemaName,
          debugstr_wn(szProcName, cbProcName), cbProcName,
          debugstr_wn(szColumnName, cbColumnName), cbColumnName);

    ret = sql_funcs.pSQLProcedureColumnsW(hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName, szProcName,
                                cbProcName, szColumnName, cbColumnName);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLProceduresW          [ODBC32.167]
 */
SQLRETURN WINAPI SQLProceduresW(SQLHSTMT hstmt, SQLWCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
                                SQLWCHAR *szSchemaName, SQLSMALLINT cbSchemaName, SQLWCHAR *szProcName,
                                SQLSMALLINT cbProcName)
{
    SQLRETURN ret;

    TRACE("(hstmt %p, szCatalogName %s, cbCatalogName %d, szSchemaName %s, cbSchemaName %d, szProcName %s,"
          " cbProcName %d)\n", hstmt, debugstr_wn(szCatalogName, cbCatalogName), cbCatalogName,
          debugstr_wn(szSchemaName, cbSchemaName), cbSchemaName, debugstr_wn(szProcName, cbProcName), cbProcName);

    ret = sql_funcs.pSQLProceduresW(hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName, szProcName, cbProcName);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLTablePrivilegesW          [ODBC32.170]
 */
SQLRETURN WINAPI SQLTablePrivilegesW(SQLHSTMT hstmt, SQLWCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
                                     SQLWCHAR *szSchemaName, SQLSMALLINT cbSchemaName, SQLWCHAR *szTableName,
                                     SQLSMALLINT cbTableName)
{
    SQLRETURN ret;

    TRACE("(hstmt %p, szCatalogName %s, cbCatalogName %d, szSchemaName %s, cbSchemaName %d, szTableName %s,"
          " cbTableName %d)\n", hstmt, debugstr_wn(szCatalogName, cbCatalogName), cbCatalogName,
          debugstr_wn(szSchemaName, cbSchemaName), cbSchemaName, debugstr_wn(szTableName, cbTableName), cbTableName);

    ret = sql_funcs.pSQLTablePrivilegesW(hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName, szTableName,
                               cbTableName);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLDriversW          [ODBC32.171]
 */
SQLRETURN WINAPI SQLDriversW(SQLHENV EnvironmentHandle, SQLUSMALLINT fDirection, SQLWCHAR *szDriverDesc,
                             SQLSMALLINT cbDriverDescMax, SQLSMALLINT *pcbDriverDesc,
                             SQLWCHAR *szDriverAttributes, SQLSMALLINT cbDriverAttrMax,
                             SQLSMALLINT *pcbDriverAttr)
{
    SQLRETURN ret;

    TRACE("(EnvironmentHandle %p, Direction %d, szDriverDesc %p, cbDriverDescMax %d, pcbDriverDesc %p,"
          " DriverAttributes %p, cbDriverAttrMax %d, pcbDriverAttr %p)\n", EnvironmentHandle, fDirection,
          szDriverDesc, cbDriverDescMax, pcbDriverDesc, szDriverAttributes, cbDriverAttrMax, pcbDriverAttr);

    ret = sql_funcs.pSQLDriversW(EnvironmentHandle, fDirection, szDriverDesc, cbDriverDescMax, pcbDriverDesc,
                       szDriverAttributes, cbDriverAttrMax, pcbDriverAttr);

    if (ret == SQL_NO_DATA && fDirection == SQL_FETCH_FIRST)
        ERR_(winediag)("No ODBC drivers could be found. Check the settings for your libodbc provider.\n");

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetDescFieldW          [ODBC32.173]
 */
SQLRETURN WINAPI SQLSetDescFieldW(SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber, SQLSMALLINT FieldIdentifier,
                                  SQLPOINTER Value, SQLINTEGER BufferLength)
{
    SQLRETURN ret;

    TRACE("(DescriptorHandle %p, RecNumber %d, FieldIdentifier %d, Value %p, BufferLength %d)\n", DescriptorHandle,
          RecNumber, FieldIdentifier, Value, BufferLength);

    ret = sql_funcs.pSQLSetDescFieldW(DescriptorHandle, RecNumber, FieldIdentifier, Value, BufferLength);
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetStmtAttrW          [ODBC32.176]
 */
SQLRETURN WINAPI SQLSetStmtAttrW(SQLHSTMT StatementHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                 SQLINTEGER StringLength)
{
    SQLRETURN ret;

    TRACE("(StatementHandle %p, Attribute %d, Value %p, StringLength %d)\n", StatementHandle, Attribute, Value,
          StringLength);

    ret = sql_funcs.pSQLSetStmtAttrW(StatementHandle, Attribute, Value, StringLength);
    if (ret == SQL_ERROR && (Attribute == SQL_ROWSET_SIZE || Attribute == SQL_ATTR_ROW_ARRAY_SIZE))
    {
        TRACE("CHEAT: returning SQL_SUCCESS to ADO\n");
        return SQL_SUCCESS;
    }

    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetDiagRecA           [ODBC32.236]
 */
SQLRETURN WINAPI SQLGetDiagRecA(SQLSMALLINT HandleType, SQLHANDLE Handle, SQLSMALLINT RecNumber,
                                SQLCHAR *Sqlstate, SQLINTEGER *NativeError, SQLCHAR *MessageText,
                                SQLSMALLINT BufferLength, SQLSMALLINT *TextLength)
{
    SQLRETURN ret;

    TRACE("(HandleType %d, Handle %p, RecNumber %d, Sqlstate %p, NativeError %p, MessageText %p, BufferLength %d,"
          " TextLength %p)\n", HandleType, Handle, RecNumber, Sqlstate, NativeError, MessageText, BufferLength,
          TextLength);

    ret = sql_funcs.pSQLGetDiagRecA(HandleType, Handle, RecNumber, Sqlstate, NativeError, MessageText, BufferLength, TextLength);
    TRACE("Returning %d\n", ret);
    return ret;
}


/***********************************************************************
 * DllMain [Internal] Initializes the internal 'ODBC32.DLL'.
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD reason, LPVOID reserved)
{
    TRACE("proxy ODBC: %p,%x,%p\n", hinstDLL, reason, reserved);

    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
       DisableThreadLibraryCalls(hinstDLL);
       if (!__wine_init_unix_lib( hinstDLL, reason, NULL, &sql_funcs )) ODBC_ReplicateToRegistry();
       break;

    case DLL_PROCESS_DETACH:
      if (reserved) break;
      __wine_init_unix_lib( hinstDLL, reason, NULL, NULL );
    }

    return TRUE;
}
