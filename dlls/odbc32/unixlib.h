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

#include "wine/list.h"
#include "wine/unixlib.h"

static inline BOOL SUCCESS( SQLRETURN ret ) { return ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO; }

enum sql_funcs
{
    process_attach,
    process_detach,
    unix_SQLAllocHandle,
    unix_SQLAllocHandleStd,
    unix_SQLBindCol,
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
    unix_SQLDescribeCol,
    unix_SQLDescribeColW,
    unix_SQLDescribeParam,
    unix_SQLDisconnect,
    unix_SQLDriverConnect,
    unix_SQLDriverConnectW,
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
    unix_funcs_count
};

struct bind_col_args
{
    INT16 target_type;
    void *target_value;
    INT64 buffer_length;
};

struct bind_parameter_args
{
    INT16  input_output_type;
    INT16  value_type;
    INT16  parameter_type;
    UINT64 column_size;
    INT16  decimal_digits;
    void  *parameter_value;
    INT64  buffer_length;
};

struct param
{
    INT16 type;
    union
    {
        struct bind_col_args col;
        struct bind_parameter_args parameter;
    };
    UINT8 *len;  /* result length array stored in Unix lib */
    void  *ptr;  /* result length ptr passed by client */
};

struct param_binding
{
    UINT32 count;
    struct param *param;
};

struct object
{
    UINT32 type;
    UINT64 unix_handle;
    void  *win32_handle;
    const struct win32_funcs *win32_funcs;
    struct object *parent;
    struct list entry;
    struct list children;
    CRITICAL_SECTION cs;
    BOOL closed;
};

struct environment
{
    struct object hdr;
    /* attributes */
    UINT32 attr_version;
    /* drivers and data sources */
    UINT32 drivers_idx;
    void  *drivers_key;
    UINT32 sources_idx;
    void  *sources_key;
    BOOL   sources_system;
};

struct connection
{
    struct object hdr;
    /* attributes */
    UINT32 attr_con_timeout;
    UINT32 attr_login_timeout;
};

struct statement
{
    struct object hdr;
    /* descriptors */
    struct descriptor *desc[4];
    /* parameter bindings */
    struct param_binding bind_col;
    struct param_binding bind_parameter;
    UINT32 row_count;   /* number of rows returned by SQLFetch() */
};

struct descriptor
{
    struct object hdr;
};

struct SQLAllocHandle_params
{
    INT16   HandleType;
    UINT64  InputHandle;
    UINT64 *OutputHandle;
};

struct SQLAllocHandleStd_params
{
    INT16   HandleType;
    UINT64  InputHandle;
    UINT64 *OutputHandle;
};

struct SQLBindCol_params
{
    UINT64 StatementHandle;
    UINT16 ColumnNumber;
    INT16  TargetType;
    void  *TargetValue;
    INT64  BufferLength;
    void  *StrLen_or_Ind;
};

struct SQLBindParameter_params
{
    UINT64 StatementHandle;
    UINT16 ParameterNumber;
    INT16  InputOutputType;
    INT16  ValueType;
    INT16  ParameterType;
    UINT64 ColumnSize;
    INT16  DecimalDigits;
    void  *ParameterValue;
    INT64  BufferLength;
    void  *StrLen_or_Ind;
};

struct SQLBrowseConnect_params
{
    UINT64 ConnectionHandle;
    UCHAR *InConnectionString;
    INT16  StringLength1;
    UCHAR *OutConnectionString;
    INT16  BufferLength;
    INT16 *StringLength2;
};

struct SQLBrowseConnectW_params
{
    UINT64 ConnectionHandle;
    WCHAR *InConnectionString;
    INT16  StringLength1;
    WCHAR *OutConnectionString;
    INT16  BufferLength;
    INT16 *StringLength2;
};

struct SQLBulkOperations_params
{
    UINT64 StatementHandle;
    INT16  Operation;
};

struct SQLCancel_params
{
    UINT64 StatementHandle;
};

struct SQLCloseCursor_params
{
    UINT64 StatementHandle;
};

struct SQLColAttribute_params
{
    UINT64 StatementHandle;
    UINT16 ColumnNumber;
    UINT16 FieldIdentifier;
    void  *CharacterAttribute;
    INT16  BufferLength;
    INT16 *StringLength;
    INT64 *NumericAttribute;
};

struct SQLColAttributeW_params
{
    UINT64 StatementHandle;
    UINT16 ColumnNumber;
    UINT16 FieldIdentifier;
    void  *CharacterAttribute;
    INT16  BufferLength;
    INT16 *StringLength;
    INT64 *NumericAttribute;
};

struct SQLColAttributes_params
{
    UINT64 StatementHandle;
    UINT16 ColumnNumber;
    UINT16 FieldIdentifier;
    void  *CharacterAttributes;
    INT16  BufferLength;
    INT16 *StringLength;
    INT64 *NumericAttributes;
};

struct SQLColAttributesW_params
{
    UINT64 StatementHandle;
    UINT16 ColumnNumber;
    UINT16 FieldIdentifier;
    void  *CharacterAttributes;
    INT16  BufferLength;
    INT16 *StringLength;
    INT64 *NumericAttributes;
};

struct SQLColumnPrivileges_params
{
    UINT64 StatementHandle;
    UCHAR *CatalogName;
    INT16  NameLength1;
    UCHAR *SchemaName;
    INT16  NameLength2;
    UCHAR *TableName;
    INT16  NameLength3;
    UCHAR *ColumnName;
    INT16  NameLength4;
};

struct SQLColumnPrivilegesW_params
{
    UINT64 StatementHandle;
    WCHAR *CatalogName;
    INT16  NameLength1;
    WCHAR *SchemaName;
    INT16  NameLength2;
    WCHAR *TableName;
    INT16  NameLength3;
    WCHAR *ColumnName;
    INT16  NameLength4;
};

struct SQLColumns_params
{
    UINT64 StatementHandle;
    UCHAR *CatalogName;
    INT16  NameLength1;
    UCHAR *SchemaName;
    INT16  NameLength2;
    UCHAR *TableName;
    INT16  NameLength3;
    UCHAR *ColumnName;
    INT16  NameLength4;
};

struct SQLColumnsW_params
{
    UINT64 StatementHandle;
    WCHAR *CatalogName;
    INT16  NameLength1;
    WCHAR *SchemaName;
    INT16  NameLength2;
    WCHAR *TableName;
    INT16  NameLength3;
    WCHAR *ColumnName;
    INT16  NameLength4;
};

struct SQLConnect_params
{
    UINT64 ConnectionHandle;
    UCHAR *ServerName;
    INT16  NameLength1;
    UCHAR *UserName;
    INT16  NameLength2;
    UCHAR *Authentication;
    INT16  NameLength3;
};

struct SQLConnectW_params
{
    UINT64 ConnectionHandle;
    WCHAR *ServerName;
    INT16  NameLength1;
    WCHAR *UserName;
    INT16  NameLength2;
    WCHAR *Authentication;
    INT16  NameLength3;
};

struct SQLCopyDesc_params
{
    UINT64 SourceDescHandle;
    UINT64 TargetDescHandle;
};

struct SQLDescribeCol_params
{
    UINT64  StatementHandle;
    UINT16  ColumnNumber;
    UCHAR  *ColumnName;
    INT16   BufferLength;
    INT16  *NameLength;
    INT16  *DataType;
    UINT64 *ColumnSize;
    INT16  *DecimalDigits;
    INT16  *Nullable;
};

struct SQLDescribeColW_params
{
    UINT64  StatementHandle;
    UINT16  ColumnNumber;
    WCHAR  *ColumnName;
    INT16   BufferLength;
    INT16  *NameLength;
    INT16  *DataType;
    UINT64 *ColumnSize;
    INT16  *DecimalDigits;
    INT16  *Nullable;
};

struct SQLDescribeParam_params
{
    UINT64  StatementHandle;
    UINT16  ParameterNumber;
    INT16  *DataType;
    UINT64 *ParameterSize;
    INT16  *DecimalDigits;
    INT16  *Nullable;
};

struct SQLDisconnect_params
{
    UINT64 ConnectionHandle;
};

struct SQLDriverConnect_params
{
    UINT64 ConnectionHandle;
    void  *WindowHandle;
    UCHAR *ConnectionString;
    INT16  Length;
    UCHAR *OutConnectionString;
    INT16  BufferLength;
    INT16 *Length2;
    UINT16 DriverCompletion;
};

struct SQLDriverConnectW_params
{
    UINT64 ConnectionHandle;
    void  *WindowHandle;
    WCHAR *InConnectionString;
    INT16  Length;
    WCHAR *OutConnectionString;
    INT16  BufferLength;
    INT16 *Length2;
    UINT16 DriverCompletion;
};

struct SQLEndTran_params
{
    INT16  HandleType;
    UINT64 Handle;
    INT16  CompletionType;
};

struct SQLError_params
{
    UINT64 EnvironmentHandle;
    UINT64 ConnectionHandle;
    UINT64 StatementHandle;
    UCHAR *SqlState;
    INT32 *NativeError;
    UCHAR *MessageText;
    INT16  BufferLength;
    INT16 *TextLength;
};

struct SQLErrorW_params
{
    UINT64 EnvironmentHandle;
    UINT64 ConnectionHandle;
    UINT64 StatementHandle;
    WCHAR *SqlState;
    INT32 *NativeError;
    WCHAR *MessageText;
    INT16  BufferLength;
    INT16 *TextLength;
};

struct SQLExecDirect_params
{
    UINT64 StatementHandle;
    UCHAR *StatementText;
    INT32  TextLength;
};

struct SQLExecDirectW_params
{
    UINT64 StatementHandle;
    WCHAR *StatementText;
    INT32  TextLength;
};

struct SQLExecute_params
{
    UINT64 StatementHandle;
};

struct SQLExtendedFetch_params
{
    UINT64  StatementHandle;
    UINT16  FetchOrientation;
    INT64   FetchOffset;
    UINT64 *RowCount;
    UINT16 *RowStatusArray;
};

struct SQLFetch_params
{
    UINT64 StatementHandle;
};

struct SQLFetchScroll_params
{
    UINT64 StatementHandle;
    INT16  FetchOrientation;
    INT64  FetchOffset;
};

struct SQLForeignKeys_params
{
    UINT64 StatementHandle;
    UCHAR *PkCatalogName;
    INT16  NameLength1;
    UCHAR *PkSchemaName;
    INT16  NameLength2;
    UCHAR *PkTableName;
    INT16  NameLength3;
    UCHAR *FkCatalogName;
    INT16  NameLength4;
    UCHAR *FkSchemaName;
    INT16  NameLength5;
    UCHAR *FkTableName;
    INT16  NameLength6;
};

struct SQLForeignKeysW_params
{
    UINT64 StatementHandle;
    WCHAR *PkCatalogName;
    INT16  NameLength1;
    WCHAR *PkSchemaName;
    INT16  NameLength2;
    WCHAR *PkTableName;
    INT16  NameLength3;
    WCHAR *FkCatalogName;
    INT16  NameLength4;
    WCHAR *FkSchemaName;
    INT16  NameLength5;
    WCHAR *FkTableName;
    INT16  NameLength6;
};

struct SQLFreeHandle_params
{
    INT16  HandleType;
    UINT64 Handle;
};

struct SQLFreeStmt_params
{
    UINT64 StatementHandle;
    UINT16 Option;
};

struct SQLGetConnectAttr_params
{
    UINT64 ConnectionHandle;
    INT32  Attribute;
    void  *Value;
    INT32  BufferLength;
    INT32 *StringLength;
};

struct SQLGetConnectAttrW_params
{
    UINT64 ConnectionHandle;
    INT32  Attribute;
    void  *Value;
    INT32  BufferLength;
    INT32 *StringLength;
};

struct SQLGetConnectOption_params
{
    UINT64 ConnectionHandle;
    UINT16 Option;
    void  *Value;
};

struct SQLGetConnectOptionW_params
{
    UINT64 ConnectionHandle;
    INT16  Option;
    void  *Value;
};

struct SQLGetCursorName_params
{
    UINT64 StatementHandle;
    UCHAR *CursorName;
    INT16  BufferLength;
    INT16 *NameLength;
};

struct SQLGetCursorNameW_params
{
    UINT64 StatementHandle;
    WCHAR *CursorName;
    INT16  BufferLength;
    INT16 *NameLength;
};

struct SQLGetData_params
{
    UINT64 StatementHandle;
    UINT16 ColumnNumber;
    INT16  TargetType;
    void  *TargetValue;
    INT64  BufferLength;
    INT64 *StrLen_or_Ind;
};

struct SQLGetDescField_params
{
    UINT64 DescriptorHandle;
    INT16  RecNumber;
    INT16  FieldIdentifier;
    void  *Value;
    INT32  BufferLength;
    INT32 *StringLength;
}
;
struct SQLGetDescFieldW_params
{
    UINT64 DescriptorHandle;
    INT16  RecNumber;
    INT16  FieldIdentifier;
    void  *Value;
    INT32  BufferLength;
    INT32 *StringLength;
};

struct SQLGetDescRec_params
{
    UINT64 DescriptorHandle;
    INT16  RecNumber;
    UCHAR *Name;
    INT16  BufferLength;
    INT16 *StringLength;
    INT16 *Type;
    INT16 *SubType;
    INT64 *Length;
    INT16 *Precision;
    INT16 *Scale;
    INT16 *Nullable;
};

struct SQLGetDescRecW_params
{
    UINT64 DescriptorHandle;
    INT16  RecNumber;
    WCHAR *Name;
    INT16  BufferLength;
    INT16 *StringLength;
    INT16 *Type;
    INT16 *SubType;
    INT64 *Length;
    INT16 *Precision;
    INT16 *Scale;
    INT16 *Nullable;
};

struct SQLGetDiagField_params
{
    INT16  HandleType;
    UINT64 Handle;
    INT16  RecNumber;
    INT16  DiagIdentifier;
    void  *DiagInfo;
    INT16  BufferLength;
    INT16 *StringLength;
};

struct SQLGetDiagFieldW_params
{
    INT16  HandleType;
    UINT64 Handle;
    INT16  RecNumber;
    INT16  DiagIdentifier;
    void  *DiagInfo;
    INT16  BufferLength;
    INT16 *StringLength;
};

struct SQLGetDiagRec_params
{
    INT16  HandleType;
    UINT64 Handle;
    INT16  RecNumber;
    UCHAR *SqlState;
    INT32 *NativeError;
    UCHAR *MessageText;
    INT16  BufferLength;
    INT16 *TextLength;
};

struct SQLGetDiagRecW_params
{
    INT16  HandleType;
    UINT64 Handle;
    INT16  RecNumber;
    WCHAR *SqlState;
    INT32 *NativeError;
    WCHAR *MessageText;
    INT16  BufferLength;
    INT16 *TextLength;
};

struct SQLGetEnvAttr_params
{
    UINT64 EnvironmentHandle;
    INT32  Attribute;
    void  *Value;
    INT32  BufferLength;
    INT32 *StringLength;
};

struct SQLGetFunctions_params
{
    UINT64  ConnectionHandle;
    UINT16  FunctionId;
    UINT16 *Supported;
};

struct SQLGetInfo_params
{
    UINT64 ConnectionHandle;
    UINT16 InfoType;
    void  *InfoValue;
    INT16  BufferLength;
    INT16 *StringLength;
};

struct SQLGetInfoW_params
{
    UINT64 ConnectionHandle;
    UINT16 InfoType;
    void  *InfoValue;
    INT16  BufferLength;
    INT16 *StringLength;
};

struct SQLGetStmtAttr_params
{
    UINT64 StatementHandle;
    INT32  Attribute;
    void  *Value;
    INT32  BufferLength;
    INT32 *StringLength;
};

struct SQLGetStmtAttrW_params
{
    UINT64 StatementHandle;
    INT32  Attribute;
    void  *Value;
    INT32  BufferLength;
    INT32 *StringLength;
};

struct SQLGetStmtOption_params
{
    UINT64 StatementHandle;
    UINT16 Option;
    void  *Value;
};

struct SQLGetTypeInfo_params
{
    UINT64 StatementHandle;
    INT16  DataType;
};

struct SQLGetTypeInfoW_params
{
    UINT64 StatementHandle;
    INT16  DataType;
};

struct SQLMoreResults_params
{
    UINT64 StatementHandle;
};

struct SQLNativeSql_params
{
    UINT64 ConnectionHandle;
    UCHAR *InStatementText;
    INT32  TextLength1;
    UCHAR *OutStatementText;
    INT32  BufferLength;
    INT32 *TextLength2;
};

struct SQLNativeSqlW_params
{
    UINT64 ConnectionHandle;
    WCHAR *InStatementText;
    INT32  TextLength1;
    WCHAR *OutStatementText;
    INT32  BufferLength;
    INT32 *TextLength2;
};

struct SQLNumParams_params
{
    UINT64 StatementHandle;
    INT16 *ParameterCount;
};

struct SQLNumResultCols_params
{
    UINT64 StatementHandle;
    INT16 *ColumnCount;
};

struct SQLParamData_params
{
    UINT64 StatementHandle;
    void  *Value;
};

struct SQLParamOptions_params
{
    UINT64  StatementHandle;
    UINT64  RowCount;
    UINT64 *RowNumber;
};

struct SQLPrepare_params
{
    UINT64 StatementHandle;
    UCHAR *StatementText;
    INT32  TextLength;
};

struct SQLPrepareW_params
{
    UINT64 StatementHandle;
    WCHAR *StatementText;
    INT32  TextLength;
};

struct SQLPrimaryKeys_params
{
    UINT64 StatementHandle;
    UCHAR *CatalogName;
    INT16  NameLength1;
    UCHAR *SchemaName;
    INT16  NameLength2;
    UCHAR *TableName;
    INT16  NameLength3;
};

struct SQLPrimaryKeysW_params
{
    UINT64 StatementHandle;
    WCHAR *CatalogName;
    INT16  NameLength1;
    WCHAR *SchemaName;
    INT16  NameLength2;
    WCHAR *TableName;
    INT16  NameLength3;
};

struct SQLProcedureColumns_params
{
    UINT64 StatementHandle;
    UCHAR *CatalogName;
    INT16  NameLength1;
    UCHAR *SchemaName;
    INT16  NameLength2;
    UCHAR *ProcName;
    INT16  NameLength3;
    UCHAR *ColumnName;
    INT16  NameLength4;
};

struct SQLProcedureColumnsW_params
{
    UINT64 StatementHandle;
    WCHAR *CatalogName;
    INT16  NameLength1;
    WCHAR *SchemaName;
    INT16  NameLength2;
    WCHAR *ProcName;
    INT16  NameLength3;
    WCHAR *ColumnName;
    INT16  NameLength4;
};

struct SQLProcedures_params
{
    UINT64 StatementHandle;
    UCHAR *CatalogName;
    INT16  NameLength1;
    UCHAR *SchemaName;
    INT16  NameLength2;
    UCHAR *ProcName;
    INT16  NameLength3;
};

struct SQLProceduresW_params
{
    UINT64 StatementHandle;
    WCHAR *CatalogName;
    INT16  NameLength1;
    WCHAR *SchemaName;
    INT16  NameLength2;
    WCHAR *ProcName;
    INT16  NameLength3;
};

struct SQLPutData_params
{
    UINT64 StatementHandle;
    void  *Data;
    INT64  StrLen_or_Ind;
};

struct SQLRowCount_params
{
    UINT64 StatementHandle;
    INT64 *RowCount;
};

struct SQLSetConnectAttr_params
{
    UINT64 ConnectionHandle;
    INT32  Attribute;
    void  *Value;
    INT32  StringLength;
};

struct SQLSetConnectAttrW_params
{
    UINT64 ConnectionHandle;
    INT32  Attribute;
    void  *Value;
    INT32  StringLength;
};

struct SQLSetConnectOption_params
{
    UINT64 ConnectionHandle;
    UINT16 Option;
    UINT64 Value;
};

struct SQLSetConnectOptionW_params
{
    UINT64 ConnectionHandle;
    UINT16 Option;
    UINT64 Value;
};

struct SQLSetCursorName_params
{
    UINT64 StatementHandle;
    UCHAR *CursorName;
    INT16  NameLength;
};

struct SQLSetCursorNameW_params
{
    UINT64 StatementHandle;
    WCHAR *CursorName;
    INT16  NameLength;
};

struct SQLSetDescField_params
{
    UINT64 DescriptorHandle;
    INT16  RecNumber;
    INT16  FieldIdentifier;
    void  *Value;
    INT32  BufferLength;
};

struct SQLSetDescFieldW_params
{
    UINT64 DescriptorHandle;
    INT16  RecNumber;
    INT16  FieldIdentifier;
    void  *Value;
    INT32  BufferLength;
};

struct SQLSetDescRec_params
{
    UINT64 DescriptorHandle;
    INT16  RecNumber;
    INT16  Type;
    INT16  SubType;
    INT64  Length;
    INT16  Precision;
    INT16  Scale;
    void  *Data;
    INT64 *StringLength;
    INT64 *Indicator;
};

struct SQLSetEnvAttr_params
{
    UINT64 EnvironmentHandle;
    INT32  Attribute;
    void  *Value;
    INT32  StringLength;
};

struct SQLSetParam_params
{
    UINT64 StatementHandle;
    UINT16 ParameterNumber;
    INT16  ValueType;
    INT16  ParameterType;
    UINT64 LengthPrecision;
    INT16  ParameterScale;
    void  *ParameterValue;
    INT64 *StrLen_or_Ind;
};

struct SQLSetPos_params
{
    UINT64 StatementHandle;
    UINT64 RowNumber;
    UINT16 Operation;
    UINT16 LockType;
};

struct SQLSetScrollOptions_params
{
    UINT64 StatementHandle;
    UINT16 Concurrency;
    INT64  KeySetSize;
    UINT16 RowSetSize;
};

struct SQLSetStmtAttr_params
{
    UINT64 StatementHandle;
    INT32  Attribute;
    void  *Value;
    INT32  StringLength;
};

struct SQLSetStmtAttrW_params
{
    UINT64 StatementHandle;
    INT32  Attribute;
    void  *Value;
    INT32  StringLength;
};

struct SQLSetStmtOption_params
{
    UINT64 StatementHandle;
    UINT16 Option;
    UINT64 Value;
};

struct SQLSpecialColumns_params
{
    UINT64 StatementHandle;
    UINT16 IdentifierType;
    UCHAR *CatalogName;
    INT16  NameLength1;
    UCHAR *SchemaName;
    INT16  NameLength2;
    UCHAR *TableName;
    INT16  NameLength3;
    UINT16 Scope;
    UINT16 Nullable;
};

struct SQLSpecialColumnsW_params
{
    UINT64 StatementHandle;
    UINT16 IdentifierType;
    WCHAR *CatalogName;
    INT16  NameLength1;
    WCHAR *SchemaName;
    INT16  NameLength2;
    WCHAR *TableName;
    INT16  NameLength3;
    UINT16 Scope;
    UINT16 Nullable;
};

struct SQLStatistics_params
{
    UINT64 StatementHandle;
    UCHAR *CatalogName;
    INT16  NameLength1;
    UCHAR *SchemaName;
    INT16  NameLength2;
    UCHAR *TableName;
    INT16  NameLength3;
    UINT16 Unique;
    UINT16 Reserved;
};

struct SQLStatisticsW_params
{
    UINT64 StatementHandle;
    WCHAR *CatalogName;
    INT16  NameLength1;
    WCHAR *SchemaName;
    INT16  NameLength2;
    WCHAR *TableName;
    INT16  NameLength3;
    UINT16 Unique;
    UINT16 Reserved;
};

struct SQLTablePrivileges_params
{
    UINT64 StatementHandle;
    UCHAR *CatalogName;
    INT16  NameLength1;
    UCHAR *SchemaName;
    INT16  NameLength2;
    UCHAR *TableName;
    INT16  NameLength3;
};

struct SQLTablePrivilegesW_params
{
    UINT64 StatementHandle;
    WCHAR *CatalogName;
    INT16  NameLength1;
    WCHAR *SchemaName;
    INT16  NameLength2;
    WCHAR *TableName;
    INT16  NameLength3;
};

struct SQLTables_params
{
    UINT64 StatementHandle;
    UCHAR *CatalogName;
    INT16  NameLength1;
    UCHAR *SchemaName;
    INT16  NameLength2;
    UCHAR *TableName;
    INT16  NameLength3;
    UCHAR *TableType;
    INT16  NameLength4;
};

struct SQLTablesW_params
{
    UINT64 StatementHandle;
    WCHAR *CatalogName;
    INT16  NameLength1;
    WCHAR *SchemaName;
    INT16  NameLength2;
    WCHAR *TableName;
    INT16  NameLength3;
    WCHAR *TableType;
    INT16  NameLength4;
};

struct SQLTransact_params
{
    UINT64 EnvironmentHandle;
    UINT64 ConnectionHandle;
    UINT16 CompletionType;
};
