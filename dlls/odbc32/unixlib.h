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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"

#include "sql.h"
#include "sqltypes.h"
#include "sqlext.h"

#include "wine/unixlib.h"

enum sql_funcs
{
    process_attach,
    process_detach,
    unix_SQLAllocConnect,
    unix_SQLAllocEnv,
    unix_SQLAllocHandle,
    unix_SQLAllocHandleStd,
    unix_SQLAllocStmt,
    unix_SQLBindCol,
    unix_SQLBindParam,
    unix_SQLBindParameter,
    unix_SQLBrowseConnect,
    unix_SQLBrowseConnectW,
    unix_SQLBulkOperations,
    unix_SQLCancel,
    unix_SQLCloseCursor,
    unix_SQLColAttribute,
    unix_SQLColAttributeW,
    unix_SQLColAttributes,
    unix_SQLColAttributesW,
    unix_SQLColumnPrivileges,
    unix_SQLColumnPrivilegesW,
    unix_SQLColumns,
    unix_SQLColumnsW,
    unix_SQLConnect,
    unix_SQLConnectW,
    unix_SQLCopyDesc,
    unix_SQLDataSources,
    unix_SQLDataSourcesA,
    unix_SQLDataSourcesW,
    unix_SQLDescribeCol,
    unix_SQLDescribeColW,
    unix_SQLDescribeParam,
    unix_SQLDisconnect,
    unix_SQLDriverConnect,
    unix_SQLDriverConnectW,
    unix_SQLDrivers,
    unix_SQLDriversW,
    unix_SQLEndTran,
    unix_SQLError,
    unix_SQLErrorW,
    unix_SQLExecDirect,
    unix_SQLExecDirectW,
    unix_SQLExecute,
    unix_SQLExtendedFetch,
    unix_SQLFetch,
    unix_SQLFetchScroll,
    unix_SQLForeignKeys,
    unix_SQLForeignKeysW,
    unix_SQLFreeConnect,
    unix_SQLFreeEnv,
    unix_SQLFreeHandle,
    unix_SQLFreeStmt,
    unix_SQLGetConnectAttr,
    unix_SQLGetConnectAttrW,
    unix_SQLGetConnectOption,
    unix_SQLGetConnectOptionW,
    unix_SQLGetCursorName,
    unix_SQLGetCursorNameW,
    unix_SQLGetData,
    unix_SQLGetDescField,
    unix_SQLGetDescFieldW,
    unix_SQLGetDescRec,
    unix_SQLGetDescRecW,
    unix_SQLGetDiagField,
    unix_SQLGetDiagFieldW,
    unix_SQLGetDiagRec,
    unix_SQLGetDiagRecA,
    unix_SQLGetDiagRecW,
    unix_SQLGetEnvAttr,
    unix_SQLGetFunctions,
    unix_SQLGetInfo,
    unix_SQLGetInfoW,
    unix_SQLGetStmtAttr,
    unix_SQLGetStmtAttrW,
    unix_SQLGetStmtOption,
    unix_SQLGetTypeInfo,
    unix_SQLGetTypeInfoW,
    unix_SQLMoreResults,
    unix_SQLNativeSql,
    unix_SQLNativeSqlW,
    unix_SQLNumParams,
    unix_SQLNumResultCols,
    unix_SQLParamData,
    unix_SQLParamOptions,
    unix_SQLPrepare,
    unix_SQLPrepareW,
    unix_SQLPrimaryKeys,
    unix_SQLPrimaryKeysW,
    unix_SQLProcedureColumns,
    unix_SQLProcedureColumnsW,
    unix_SQLProcedures,
    unix_SQLProceduresW,
    unix_SQLPutData,
    unix_SQLRowCount,
    unix_SQLSetConnectAttr,
    unix_SQLSetConnectAttrW,
    unix_SQLSetConnectOption,
    unix_SQLSetConnectOptionW,
    unix_SQLSetCursorName,
    unix_SQLSetCursorNameW,
    unix_SQLSetDescField,
    unix_SQLSetDescFieldW,
    unix_SQLSetDescRec,
    unix_SQLSetEnvAttr,
    unix_SQLSetParam,
    unix_SQLSetPos,
    unix_SQLSetScrollOptions,
    unix_SQLSetStmtAttr,
    unix_SQLSetStmtAttrW,
    unix_SQLSetStmtOption,
    unix_SQLSpecialColumns,
    unix_SQLSpecialColumnsW,
    unix_SQLStatistics,
    unix_SQLStatisticsW,
    unix_SQLTablePrivileges,
    unix_SQLTablePrivilegesW,
    unix_SQLTables,
    unix_SQLTablesW,
    unix_SQLTransact,
    NB_ODBC_FUNCS
};

struct SQLAllocConnect_params { SQLHENV EnvironmentHandle; SQLHDBC *ConnectionHandle; };
struct SQLAllocEnv_params { SQLHENV *EnvironmentHandle; };
struct SQLAllocHandle_params { SQLSMALLINT HandleType; SQLHANDLE InputHandle; SQLHANDLE *OutputHandle; };
struct SQLAllocHandleStd_params { SQLSMALLINT HandleType; SQLHANDLE InputHandle; SQLHANDLE *OutputHandle; };
struct SQLAllocStmt_params { SQLHDBC ConnectionHandle; SQLHSTMT *StatementHandle; };
struct SQLBindCol_params { SQLHSTMT StatementHandle; SQLUSMALLINT ColumnNumber; SQLSMALLINT TargetType; SQLPOINTER TargetValue; SQLLEN BufferLength; SQLLEN *StrLen_or_Ind; };
struct SQLBindParam_params { SQLHSTMT StatementHandle; SQLUSMALLINT ParameterNumber; SQLSMALLINT ValueType; SQLSMALLINT ParameterType; SQLULEN LengthPrecision; SQLSMALLINT ParameterScale; SQLPOINTER ParameterValue; SQLLEN *StrLen_or_Ind; };
struct SQLBindParameter_params { SQLHSTMT hstmt; SQLUSMALLINT ipar; SQLSMALLINT fParamType; SQLSMALLINT fCType; SQLSMALLINT fSqlType; SQLULEN cbColDef; SQLSMALLINT ibScale; SQLPOINTER rgbValue; SQLLEN cbValueMax; SQLLEN *pcbValue; };
struct SQLBrowseConnect_params { SQLHDBC hdbc; SQLCHAR *szConnStrIn; SQLSMALLINT cbConnStrIn; SQLCHAR *szConnStrOut; SQLSMALLINT cbConnStrOutMax; SQLSMALLINT *pcbConnStrOut; };
struct SQLBrowseConnectW_params { SQLHDBC hdbc; SQLWCHAR *szConnStrIn; SQLSMALLINT cbConnStrIn; SQLWCHAR *szConnStrOut; SQLSMALLINT cbConnStrOutMax; SQLSMALLINT *pcbConnStrOut; };
struct SQLBulkOperations_params { SQLHSTMT StatementHandle; SQLSMALLINT Operation; };
struct SQLCancel_params { SQLHSTMT StatementHandle; };
struct SQLCloseCursor_params { SQLHSTMT StatementHandle; };
struct SQLColAttribute_params { SQLHSTMT StatementHandle; SQLUSMALLINT ColumnNumber; SQLUSMALLINT FieldIdentifier; SQLPOINTER CharacterAttribute; SQLSMALLINT BufferLength; SQLSMALLINT *StringLength; SQLLEN *NumericAttribute; };
struct SQLColAttributeW_params { SQLHSTMT StatementHandle; SQLUSMALLINT ColumnNumber; SQLUSMALLINT FieldIdentifier; SQLPOINTER CharacterAttribute; SQLSMALLINT BufferLength; SQLSMALLINT *StringLength; SQLLEN *NumericAttribute; };
struct SQLColAttributes_params { SQLHSTMT hstmt; SQLUSMALLINT icol; SQLUSMALLINT fDescType; SQLPOINTER rgbDesc; SQLSMALLINT cbDescMax; SQLSMALLINT *pcbDesc; SQLLEN *pfDesc; };
struct SQLColAttributesW_params { SQLHSTMT hstmt; SQLUSMALLINT icol; SQLUSMALLINT fDescType; SQLPOINTER rgbDesc; SQLSMALLINT cbDescMax; SQLSMALLINT *pcbDesc; SQLLEN *pfDesc; };
struct SQLColumnPrivileges_params { SQLHSTMT hstmt; SQLCHAR *szCatalogName; SQLSMALLINT cbCatalogName; SQLCHAR *szSchemaName; SQLSMALLINT cbSchemaName; SQLCHAR *szTableName; SQLSMALLINT cbTableName; SQLCHAR *szColumnName; SQLSMALLINT cbColumnName; };
struct SQLColumnPrivilegesW_params { SQLHSTMT hstmt; SQLWCHAR *szCatalogName; SQLSMALLINT cbCatalogName; SQLWCHAR *szSchemaName; SQLSMALLINT cbSchemaName; SQLWCHAR *szTableName; SQLSMALLINT cbTableName; SQLWCHAR *szColumnName; SQLSMALLINT cbColumnName; };
struct SQLColumns_params { SQLHSTMT StatementHandle; SQLCHAR *CatalogName; SQLSMALLINT NameLength1; SQLCHAR *SchemaName; SQLSMALLINT NameLength2; SQLCHAR *TableName; SQLSMALLINT NameLength3; SQLCHAR *ColumnName; SQLSMALLINT NameLength4; };
struct SQLColumnsW_params { SQLHSTMT StatementHandle; WCHAR *CatalogName; SQLSMALLINT NameLength1; WCHAR *SchemaName; SQLSMALLINT NameLength2; WCHAR *TableName; SQLSMALLINT NameLength3; WCHAR *ColumnName; SQLSMALLINT NameLength4; };
struct SQLConnect_params { SQLHDBC ConnectionHandle; SQLCHAR *ServerName; SQLSMALLINT NameLength1; SQLCHAR *UserName; SQLSMALLINT NameLength2; SQLCHAR *Authentication; SQLSMALLINT NameLength3; };
struct SQLConnectW_params { SQLHDBC ConnectionHandle; WCHAR *ServerName; SQLSMALLINT NameLength1; WCHAR *UserName; SQLSMALLINT NameLength2; WCHAR *Authentication; SQLSMALLINT NameLength3; };
struct SQLCopyDesc_params { SQLHDESC SourceDescHandle; SQLHDESC TargetDescHandle; };
struct SQLDataSources_params { SQLHENV EnvironmentHandle; SQLUSMALLINT Direction; SQLCHAR *ServerName; SQLSMALLINT BufferLength1; SQLSMALLINT *NameLength1; SQLCHAR *Description; SQLSMALLINT BufferLength2; SQLSMALLINT *NameLength2; };
struct SQLDataSourcesA_params { SQLHENV EnvironmentHandle; SQLUSMALLINT Direction; SQLCHAR *ServerName; SQLSMALLINT BufferLength1; SQLSMALLINT *NameLength1; SQLCHAR *Description; SQLSMALLINT BufferLength2; SQLSMALLINT *NameLength2; };
struct SQLDataSourcesW_params { SQLHENV EnvironmentHandle; SQLUSMALLINT Direction; WCHAR *ServerName; SQLSMALLINT BufferLength1; SQLSMALLINT *NameLength1; WCHAR *Description; SQLSMALLINT BufferLength2; SQLSMALLINT *NameLength2; };
struct SQLDescribeCol_params { SQLHSTMT StatementHandle; SQLUSMALLINT ColumnNumber; SQLCHAR *ColumnName; SQLSMALLINT BufferLength; SQLSMALLINT *NameLength; SQLSMALLINT *DataType; SQLULEN *ColumnSize; SQLSMALLINT *DecimalDigits; SQLSMALLINT *Nullable; };
struct SQLDescribeColW_params { SQLHSTMT StatementHandle; SQLUSMALLINT ColumnNumber; WCHAR *ColumnName; SQLSMALLINT BufferLength; SQLSMALLINT *NameLength; SQLSMALLINT *DataType; SQLULEN *ColumnSize; SQLSMALLINT *DecimalDigits; SQLSMALLINT *Nullable; };
struct SQLDescribeParam_params { SQLHSTMT hstmt; SQLUSMALLINT ipar; SQLSMALLINT *pfSqlType; SQLULEN *pcbParamDef; SQLSMALLINT *pibScale; SQLSMALLINT *pfNullable; };
struct SQLDisconnect_params { SQLHDBC ConnectionHandle; };
struct SQLDriverConnect_params { SQLHDBC hdbc; SQLHWND hwnd; SQLCHAR *ConnectionString; SQLSMALLINT Length; SQLCHAR *conn_str_out; SQLSMALLINT conn_str_out_max; SQLSMALLINT *ptr_conn_str_out; SQLUSMALLINT driver_completion; };
struct SQLDriverConnectW_params { SQLHDBC ConnectionHandle; SQLHWND WindowHandle; WCHAR *InConnectionString; SQLSMALLINT Length; WCHAR *OutConnectionString; SQLSMALLINT BufferLength; SQLSMALLINT *Length2; SQLUSMALLINT DriverCompletion; };
struct SQLDrivers_params { SQLHENV EnvironmentHandle; SQLUSMALLINT fDirection; SQLCHAR *szDriverDesc; SQLSMALLINT cbDriverDescMax; SQLSMALLINT *pcbDriverDesc; SQLCHAR *szDriverAttributes; SQLSMALLINT cbDriverAttrMax; SQLSMALLINT *pcbDriverAttr; };
struct SQLDriversW_params { SQLHENV EnvironmentHandle; SQLUSMALLINT fDirection; SQLWCHAR *szDriverDesc; SQLSMALLINT cbDriverDescMax; SQLSMALLINT *pcbDriverDesc; SQLWCHAR *szDriverAttributes; SQLSMALLINT cbDriverAttrMax; SQLSMALLINT *pcbDriverAttr; };
struct SQLEndTran_params { SQLSMALLINT HandleType; SQLHANDLE Handle; SQLSMALLINT CompletionType; };
struct SQLError_params { SQLHENV EnvironmentHandle; SQLHDBC ConnectionHandle; SQLHSTMT StatementHandle; SQLCHAR *Sqlstate; SQLINTEGER *NativeError; SQLCHAR *MessageText; SQLSMALLINT BufferLength; SQLSMALLINT *TextLength; };
struct SQLErrorW_params { SQLHENV EnvironmentHandle; SQLHDBC ConnectionHandle; SQLHSTMT StatementHandle; WCHAR *Sqlstate; SQLINTEGER *NativeError; WCHAR *MessageText; SQLSMALLINT BufferLength; SQLSMALLINT *TextLength; };
struct SQLExecDirect_params { SQLHSTMT StatementHandle; SQLCHAR *StatementText; SQLINTEGER TextLength; };
struct SQLExecDirectW_params { SQLHSTMT StatementHandle; WCHAR *StatementText; SQLINTEGER TextLength; };
struct SQLExecute_params { SQLHSTMT StatementHandle; };
struct SQLExtendedFetch_params { SQLHSTMT hstmt; SQLUSMALLINT fFetchType; SQLLEN irow; SQLULEN *pcrow; SQLUSMALLINT *rgfRowStatus; };
struct SQLFetch_params { SQLHSTMT StatementHandle; };
struct SQLFetchScroll_params { SQLHSTMT StatementHandle; SQLSMALLINT FetchOrientation; SQLLEN FetchOffset; };
struct SQLForeignKeys_params { SQLHSTMT hstmt; SQLCHAR *szPkCatalogName; SQLSMALLINT cbPkCatalogName; SQLCHAR *szPkSchemaName; SQLSMALLINT cbPkSchemaName; SQLCHAR *szPkTableName; SQLSMALLINT cbPkTableName; SQLCHAR *szFkCatalogName; SQLSMALLINT cbFkCatalogName; SQLCHAR *szFkSchemaName; SQLSMALLINT cbFkSchemaName; SQLCHAR *szFkTableName; SQLSMALLINT cbFkTableName; };
struct SQLForeignKeysW_params { SQLHSTMT hstmt; SQLWCHAR *szPkCatalogName; SQLSMALLINT cbPkCatalogName; SQLWCHAR *szPkSchemaName; SQLSMALLINT cbPkSchemaName; SQLWCHAR *szPkTableName; SQLSMALLINT cbPkTableName; SQLWCHAR *szFkCatalogName; SQLSMALLINT cbFkCatalogName; SQLWCHAR *szFkSchemaName; SQLSMALLINT cbFkSchemaName; SQLWCHAR *szFkTableName; SQLSMALLINT cbFkTableName; };
struct SQLFreeConnect_params { SQLHDBC ConnectionHandle; };
struct SQLFreeEnv_params { SQLHENV EnvironmentHandle; };
struct SQLFreeHandle_params { SQLSMALLINT HandleType; SQLHANDLE Handle; };
struct SQLFreeStmt_params { SQLHSTMT StatementHandle; SQLUSMALLINT Option; };
struct SQLGetConnectAttr_params { SQLHDBC ConnectionHandle; SQLINTEGER Attribute; SQLPOINTER Value; SQLINTEGER BufferLength; SQLINTEGER *StringLength; };
struct SQLGetConnectAttrW_params { SQLHDBC ConnectionHandle; SQLINTEGER Attribute; SQLPOINTER Value; SQLINTEGER BufferLength; SQLINTEGER *StringLength; };
struct SQLGetConnectOption_params { SQLHDBC ConnectionHandle; SQLUSMALLINT Option; SQLPOINTER Value; };
struct SQLGetConnectOptionW_params { SQLHDBC ConnectionHandle; SQLUSMALLINT Option; SQLPOINTER Value; };
struct SQLGetCursorName_params { SQLHSTMT StatementHandle; SQLCHAR *CursorName; SQLSMALLINT BufferLength; SQLSMALLINT *NameLength; };
struct SQLGetCursorNameW_params { SQLHSTMT StatementHandle; WCHAR *CursorName; SQLSMALLINT BufferLength; SQLSMALLINT *NameLength; };
struct SQLGetData_params { SQLHSTMT StatementHandle; SQLUSMALLINT ColumnNumber; SQLSMALLINT TargetType; SQLPOINTER TargetValue; SQLLEN BufferLength; SQLLEN *StrLen_or_Ind; };
struct SQLGetDescField_params { SQLHDESC DescriptorHandle; SQLSMALLINT RecNumber; SQLSMALLINT FieldIdentifier; SQLPOINTER Value; SQLINTEGER BufferLength; SQLINTEGER *StringLength; };
struct SQLGetDescFieldW_params { SQLHDESC DescriptorHandle; SQLSMALLINT RecNumber; SQLSMALLINT FieldIdentifier; SQLPOINTER Value; SQLINTEGER BufferLength; SQLINTEGER *StringLength; };
struct SQLGetDescRec_params { SQLHDESC DescriptorHandle; SQLSMALLINT RecNumber; SQLCHAR *Name; SQLSMALLINT BufferLength; SQLSMALLINT *StringLength; SQLSMALLINT *Type; SQLSMALLINT *SubType; SQLLEN *Length; SQLSMALLINT *Precision; SQLSMALLINT *Scale; SQLSMALLINT *Nullable; };
struct SQLGetDescRecW_params { SQLHDESC DescriptorHandle; SQLSMALLINT RecNumber; WCHAR *Name; SQLSMALLINT BufferLength; SQLSMALLINT *StringLength; SQLSMALLINT *Type; SQLSMALLINT *SubType; SQLLEN *Length; SQLSMALLINT *Precision; SQLSMALLINT *Scale; SQLSMALLINT *Nullable; };
struct SQLGetDiagField_params { SQLSMALLINT HandleType; SQLHANDLE Handle; SQLSMALLINT RecNumber; SQLSMALLINT DiagIdentifier; SQLPOINTER DiagInfo; SQLSMALLINT BufferLength; SQLSMALLINT *StringLength; };
struct SQLGetDiagFieldW_params { SQLSMALLINT HandleType; SQLHANDLE Handle; SQLSMALLINT RecNumber; SQLSMALLINT DiagIdentifier; SQLPOINTER DiagInfo; SQLSMALLINT BufferLength; SQLSMALLINT *StringLength; };
struct SQLGetDiagRec_params { SQLSMALLINT HandleType; SQLHANDLE Handle; SQLSMALLINT RecNumber; SQLCHAR *Sqlstate; SQLINTEGER *NativeError; SQLCHAR *MessageText; SQLSMALLINT BufferLength; SQLSMALLINT *TextLength; };
struct SQLGetDiagRecA_params { SQLSMALLINT HandleType; SQLHANDLE Handle; SQLSMALLINT RecNumber; SQLCHAR *Sqlstate; SQLINTEGER *NativeError; SQLCHAR *MessageText; SQLSMALLINT BufferLength; SQLSMALLINT *TextLength; };
struct SQLGetDiagRecW_params { SQLSMALLINT HandleType; SQLHANDLE Handle; SQLSMALLINT RecNumber; WCHAR *Sqlstate; SQLINTEGER *NativeError; WCHAR *MessageText; SQLSMALLINT BufferLength; SQLSMALLINT *TextLength; };
struct SQLGetEnvAttr_params { SQLHENV EnvironmentHandle; SQLINTEGER Attribute; SQLPOINTER Value; SQLINTEGER BufferLength; SQLINTEGER *StringLength; };
struct SQLGetFunctions_params { SQLHDBC ConnectionHandle; SQLUSMALLINT FunctionId; SQLUSMALLINT *Supported; };
struct SQLGetInfo_params { SQLHDBC ConnectionHandle; SQLUSMALLINT InfoType; SQLPOINTER InfoValue; SQLSMALLINT BufferLength; SQLSMALLINT *StringLength; };
struct SQLGetInfoW_params { SQLHDBC ConnectionHandle; SQLUSMALLINT InfoType; SQLPOINTER InfoValue; SQLSMALLINT BufferLength; SQLSMALLINT *StringLength; };
struct SQLGetStmtAttr_params { SQLHSTMT StatementHandle; SQLINTEGER Attribute; SQLPOINTER Value; SQLINTEGER BufferLength; SQLINTEGER *StringLength; };
struct SQLGetStmtAttrW_params { SQLHSTMT StatementHandle; SQLINTEGER Attribute; SQLPOINTER Value; SQLINTEGER BufferLength; SQLINTEGER *StringLength; };
struct SQLGetStmtOption_params { SQLHSTMT StatementHandle; SQLUSMALLINT Option; SQLPOINTER Value; };
struct SQLGetTypeInfo_params { SQLHSTMT StatementHandle; SQLSMALLINT DataType; };
struct SQLGetTypeInfoW_params { SQLHSTMT StatementHandle; SQLSMALLINT DataType; };
struct SQLMoreResults_params { SQLHSTMT StatementHandle; };
struct SQLNativeSql_params { SQLHDBC hdbc; SQLCHAR *szSqlStrIn; SQLINTEGER cbSqlStrIn; SQLCHAR *szSqlStr; SQLINTEGER cbSqlStrMax; SQLINTEGER *pcbSqlStr; };
struct SQLNativeSqlW_params { SQLHDBC hdbc; SQLWCHAR *szSqlStrIn; SQLINTEGER cbSqlStrIn; SQLWCHAR *szSqlStr; SQLINTEGER cbSqlStrMax; SQLINTEGER *pcbSqlStr; };
struct SQLNumParams_params { SQLHSTMT hstmt; SQLSMALLINT *pcpar; };
struct SQLNumResultCols_params { SQLHSTMT StatementHandle; SQLSMALLINT *ColumnCount; };
struct SQLParamData_params { SQLHSTMT StatementHandle; SQLPOINTER *Value; };
struct SQLParamOptions_params { SQLHSTMT hstmt; SQLULEN crow; SQLULEN *pirow; };
struct SQLPrepare_params { SQLHSTMT StatementHandle; SQLCHAR *StatementText; SQLINTEGER TextLength; };
struct SQLPrepareW_params { SQLHSTMT StatementHandle; WCHAR *StatementText; SQLINTEGER TextLength; };
struct SQLPrimaryKeys_params { SQLHSTMT hstmt; SQLCHAR *szCatalogName; SQLSMALLINT cbCatalogName; SQLCHAR *szSchemaName; SQLSMALLINT cbSchemaName; SQLCHAR *szTableName; SQLSMALLINT cbTableName; };
struct SQLPrimaryKeysW_params { SQLHSTMT hstmt; SQLWCHAR *szCatalogName; SQLSMALLINT cbCatalogName; SQLWCHAR *szSchemaName; SQLSMALLINT cbSchemaName; SQLWCHAR *szTableName; SQLSMALLINT cbTableName; };
struct SQLProcedureColumns_params { SQLHSTMT hstmt; SQLCHAR *szCatalogName; SQLSMALLINT cbCatalogName; SQLCHAR *szSchemaName; SQLSMALLINT cbSchemaName; SQLCHAR *szProcName; SQLSMALLINT cbProcName; SQLCHAR *szColumnName; SQLSMALLINT cbColumnName; };
struct SQLProcedureColumnsW_params { SQLHSTMT hstmt; SQLWCHAR *szCatalogName; SQLSMALLINT cbCatalogName; SQLWCHAR *szSchemaName; SQLSMALLINT cbSchemaName; SQLWCHAR *szProcName; SQLSMALLINT cbProcName; SQLWCHAR *szColumnName; SQLSMALLINT cbColumnName; };
struct SQLProcedures_params { SQLHSTMT hstmt; SQLCHAR *szCatalogName; SQLSMALLINT cbCatalogName; SQLCHAR *szSchemaName; SQLSMALLINT cbSchemaName; SQLCHAR *szProcName; SQLSMALLINT cbProcName; };
struct SQLProceduresW_params { SQLHSTMT hstmt; SQLWCHAR *szCatalogName; SQLSMALLINT cbCatalogName; SQLWCHAR *szSchemaName; SQLSMALLINT cbSchemaName; SQLWCHAR *szProcName; SQLSMALLINT cbProcName; };
struct SQLPutData_params { SQLHSTMT StatementHandle; SQLPOINTER Data; SQLLEN StrLen_or_Ind; };
struct SQLRowCount_params { SQLHSTMT StatementHandle; SQLLEN *RowCount; };
struct SQLSetConnectAttr_params { SQLHDBC ConnectionHandle; SQLINTEGER Attribute; SQLPOINTER Value; SQLINTEGER StringLength; };
struct SQLSetConnectAttrW_params { SQLHDBC ConnectionHandle; SQLINTEGER Attribute; SQLPOINTER Value; SQLINTEGER StringLength; };
struct SQLSetConnectOption_params { SQLHDBC ConnectionHandle; SQLUSMALLINT Option; SQLULEN Value; };
struct SQLSetConnectOptionW_params { SQLHDBC ConnectionHandle; SQLUSMALLINT Option; SQLULEN Value; };
struct SQLSetCursorName_params { SQLHSTMT StatementHandle; SQLCHAR *CursorName; SQLSMALLINT NameLength; };
struct SQLSetCursorNameW_params { SQLHSTMT StatementHandle; WCHAR *CursorName; SQLSMALLINT NameLength; };
struct SQLSetDescField_params { SQLHDESC DescriptorHandle; SQLSMALLINT RecNumber; SQLSMALLINT FieldIdentifier; SQLPOINTER Value; SQLINTEGER BufferLength; };
struct SQLSetDescFieldW_params { SQLHDESC DescriptorHandle; SQLSMALLINT RecNumber; SQLSMALLINT FieldIdentifier; SQLPOINTER Value; SQLINTEGER BufferLength; };
struct SQLSetDescRec_params { SQLHDESC DescriptorHandle; SQLSMALLINT RecNumber; SQLSMALLINT Type; SQLSMALLINT SubType; SQLLEN Length; SQLSMALLINT Precision; SQLSMALLINT Scale; SQLPOINTER Data; SQLLEN *StringLength; SQLLEN *Indicator; };
struct SQLSetEnvAttr_params { SQLHENV EnvironmentHandle; SQLINTEGER Attribute; SQLPOINTER Value; SQLINTEGER StringLength; };
struct SQLSetParam_params { SQLHSTMT StatementHandle; SQLUSMALLINT ParameterNumber; SQLSMALLINT ValueType; SQLSMALLINT ParameterType; SQLULEN LengthPrecision; SQLSMALLINT ParameterScale; SQLPOINTER ParameterValue; SQLLEN *StrLen_or_Ind; };
struct SQLSetPos_params { SQLHSTMT hstmt; SQLSETPOSIROW irow; SQLUSMALLINT fOption; SQLUSMALLINT fLock; };
struct SQLSetScrollOptions_params { SQLHSTMT statement_handle; SQLUSMALLINT f_concurrency; SQLLEN crow_keyset; SQLUSMALLINT crow_rowset; };
struct SQLSetStmtAttr_params { SQLHSTMT StatementHandle; SQLINTEGER Attribute; SQLPOINTER Value; SQLINTEGER StringLength; };
struct SQLSetStmtAttrW_params { SQLHSTMT StatementHandle; SQLINTEGER Attribute; SQLPOINTER Value; SQLINTEGER StringLength; };
struct SQLSetStmtOption_params { SQLHSTMT StatementHandle; SQLUSMALLINT Option; SQLULEN Value; };
struct SQLSpecialColumns_params { SQLHSTMT StatementHandle; SQLUSMALLINT IdentifierType; SQLCHAR *CatalogName; SQLSMALLINT NameLength1; SQLCHAR *SchemaName; SQLSMALLINT NameLength2; SQLCHAR *TableName; SQLSMALLINT NameLength3; SQLUSMALLINT Scope; SQLUSMALLINT Nullable; };
struct SQLSpecialColumnsW_params { SQLHSTMT StatementHandle; SQLUSMALLINT IdentifierType; SQLWCHAR *CatalogName; SQLSMALLINT NameLength1; SQLWCHAR *SchemaName; SQLSMALLINT NameLength2; SQLWCHAR *TableName; SQLSMALLINT NameLength3; SQLUSMALLINT Scope; SQLUSMALLINT Nullable; };
struct SQLStatistics_params { SQLHSTMT StatementHandle; SQLCHAR *CatalogName; SQLSMALLINT NameLength1; SQLCHAR *SchemaName; SQLSMALLINT NameLength2; SQLCHAR *TableName; SQLSMALLINT NameLength3; SQLUSMALLINT Unique; SQLUSMALLINT Reserved; };
struct SQLStatisticsW_params { SQLHSTMT StatementHandle; SQLWCHAR *CatalogName; SQLSMALLINT NameLength1; SQLWCHAR *SchemaName; SQLSMALLINT NameLength2; SQLWCHAR *TableName; SQLSMALLINT NameLength3; SQLUSMALLINT Unique; SQLUSMALLINT Reserved; };
struct SQLTablePrivileges_params { SQLHSTMT hstmt; SQLCHAR *szCatalogName; SQLSMALLINT cbCatalogName; SQLCHAR *szSchemaName; SQLSMALLINT cbSchemaName; SQLCHAR *szTableName; SQLSMALLINT cbTableName; };
struct SQLTablePrivilegesW_params { SQLHSTMT hstmt; SQLWCHAR *szCatalogName; SQLSMALLINT cbCatalogName; SQLWCHAR *szSchemaName; SQLSMALLINT cbSchemaName; SQLWCHAR *szTableName; SQLSMALLINT cbTableName; };
struct SQLTables_params { SQLHSTMT StatementHandle; SQLCHAR *CatalogName; SQLSMALLINT NameLength1; SQLCHAR *SchemaName; SQLSMALLINT NameLength2; SQLCHAR *TableName; SQLSMALLINT NameLength3; SQLCHAR *TableType; SQLSMALLINT NameLength4; };
struct SQLTablesW_params { SQLHSTMT StatementHandle; SQLWCHAR *CatalogName; SQLSMALLINT NameLength1; SQLWCHAR *SchemaName; SQLSMALLINT NameLength2; SQLWCHAR *TableName; SQLSMALLINT NameLength3; SQLWCHAR *TableType; SQLSMALLINT NameLength4; };
struct SQLTransact_params { SQLHENV EnvironmentHandle; SQLHDBC ConnectionHandle; SQLUSMALLINT CompletionType; };
