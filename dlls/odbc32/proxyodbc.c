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

#define ODBC_CALL( func, params ) WINE_UNIX_CALL( unix_ ## func, params )

/*************************************************************************
 *				SQLAllocConnect           [ODBC32.001]
 */
SQLRETURN WINAPI SQLAllocConnect(SQLHENV EnvironmentHandle, SQLHDBC *ConnectionHandle)
{
    struct SQLAllocConnect_params params;
    struct handle *con, *env = EnvironmentHandle;
    SQLRETURN ret;

    TRACE("(EnvironmentHandle %p, ConnectionHandle %p)\n", EnvironmentHandle, ConnectionHandle);

    *ConnectionHandle = 0;
    if (!(con = calloc( 1, sizeof(*con) ))) return SQL_ERROR;

    params.EnvironmentHandle = env->unix_handle;
    if (SUCCESS((ret = ODBC_CALL( SQLAllocConnect, &params ))))
    {
        con->unix_handle = params.ConnectionHandle;
        *ConnectionHandle = con;
    }
    else free( con );

    TRACE("Returning %d, ConnectionHandle %p\n", ret, *ConnectionHandle);
    return ret;
}

/*************************************************************************
 *				SQLAllocEnv           [ODBC32.002]
 */
SQLRETURN WINAPI SQLAllocEnv(SQLHENV *EnvironmentHandle)
{
    struct SQLAllocEnv_params params;
    struct handle *env;
    SQLRETURN ret;

    TRACE("(EnvironmentHandle %p)\n", EnvironmentHandle);

    *EnvironmentHandle = 0;
    if (!(env = calloc( 1, sizeof(*env) ))) return SQL_ERROR;

    if (SUCCESS((ret = ODBC_CALL( SQLAllocEnv, &params ))))
    {
        env->unix_handle = params.EnvironmentHandle;
        *EnvironmentHandle = env;
    }
    else free( env );

    TRACE("Returning %d, EnvironmentHandle %p\n", ret, *EnvironmentHandle);
    return ret;
}

/*************************************************************************
 *				SQLAllocHandle           [ODBC32.024]
 */
SQLRETURN WINAPI SQLAllocHandle(SQLSMALLINT HandleType, SQLHANDLE InputHandle, SQLHANDLE *OutputHandle)
{
    struct SQLAllocHandle_params params;
    struct handle *output, *input = InputHandle;
    SQLRETURN ret;

    TRACE("(HandleType %d, InputHandle %p, OutputHandle %p)\n", HandleType, InputHandle, OutputHandle);

    *OutputHandle = 0;
    if (!(output = calloc( 1, sizeof(*output) ))) return SQL_ERROR;

    params.HandleType  = HandleType;
    params.InputHandle = input ? input->unix_handle : 0;
    if (SUCCESS((ret = ODBC_CALL( SQLAllocHandle, &params ))))
    {
        output->unix_handle = params.OutputHandle;
        *OutputHandle = output;
    }
    else free( output );

    TRACE("Returning %d, OutputHandle %p\n", ret, *OutputHandle);
    return ret;
}

/*************************************************************************
 *				SQLAllocStmt           [ODBC32.003]
 */
SQLRETURN WINAPI SQLAllocStmt(SQLHDBC ConnectionHandle, SQLHSTMT *StatementHandle)
{
    struct SQLAllocStmt_params params;
    struct handle *stmt, *con = ConnectionHandle;
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p, StatementHandle %p)\n", ConnectionHandle, StatementHandle);

    *StatementHandle = 0;
    if (!(stmt = calloc( 1, sizeof(*stmt) ))) return SQL_ERROR;

    params.ConnectionHandle = con->unix_handle;
    if (SUCCESS((ret = ODBC_CALL( SQLAllocStmt, &params ))))
    {
        stmt->unix_handle = params.StatementHandle;
        *StatementHandle = stmt;
    }
    else free( stmt );

    TRACE ("Returning %d, StatementHandle %p\n", ret, *StatementHandle);
    return ret;
}

/*************************************************************************
 *				SQLAllocHandleStd           [ODBC32.077]
 */
SQLRETURN WINAPI SQLAllocHandleStd(SQLSMALLINT HandleType, SQLHANDLE InputHandle, SQLHANDLE *OutputHandle)
{
    struct SQLAllocHandleStd_params params;
    struct handle *output, *input = InputHandle;
    SQLRETURN ret;

    TRACE("(HandleType %d, InputHandle %p, OutputHandle %p)\n", HandleType, InputHandle, OutputHandle);

    *OutputHandle = 0;
    if (!(output = calloc( 1, sizeof(*output) ))) return SQL_ERROR;

    params.HandleType  = HandleType;
    params.InputHandle = input ? input->unix_handle : 0;
    if (SUCCESS((ret = ODBC_CALL( SQLAllocHandleStd, &params ))))
    {
        output->unix_handle = params.OutputHandle;
        *OutputHandle = output;
    }
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

static BOOL resize_binding( struct param_binding *binding, UINT32 count )
{
    struct param *tmp;
    UINT32 new_count = max( binding->count, count );

    if (!(tmp = realloc( binding->param, new_count * sizeof(*tmp) ))) return FALSE;
    memset( tmp + binding->count, 0, (new_count - binding->count) * sizeof(*tmp) );
    binding->param = tmp;
    binding->count = new_count;
    return TRUE;
}

/*************************************************************************
 *				SQLBindCol           [ODBC32.004]
 */
SQLRETURN WINAPI SQLBindCol(SQLHSTMT StatementHandle, SQLUSMALLINT ColumnNumber, SQLSMALLINT TargetType,
                            SQLPOINTER TargetValue, SQLLEN BufferLength, SQLLEN *StrLen_or_Ind)
{
    struct SQLBindCol_params params = { 0, ColumnNumber, TargetType, TargetValue, BufferLength };
    struct handle *handle = StatementHandle;
    UINT i = ColumnNumber - 1;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, ColumnNumber %d, TargetType %d, TargetValue %p, BufferLength %s, StrLen_or_Ind %p)\n",
          StatementHandle, ColumnNumber, TargetType, TargetValue, debugstr_sqllen(BufferLength), StrLen_or_Ind);

    if (!handle) return SQL_INVALID_HANDLE;
    if (!ColumnNumber)
    {
        FIXME( "column 0 not handled\n" );
        return SQL_ERROR;
    }
    if (!resize_binding( &handle->bind_col, ColumnNumber )) return SQL_ERROR;
    params.StatementHandle = handle->unix_handle;
    params.StrLen_or_Ind   = &handle->bind_col.param[i].len;
    if (SUCCESS(( ret = ODBC_CALL( SQLBindCol, &params )))) handle->bind_col.param[i].ptr = StrLen_or_Ind;
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
    struct SQLBindParam_params params = { 0, ParameterNumber, ValueType, ParameterType, LengthPrecision,
                                          ParameterScale, ParameterValue };
    struct handle *handle = StatementHandle;
    UINT i = ParameterNumber - 1;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, ParameterNumber %d, ValueType %d, ParameterType %d, LengthPrecision %s,"
          " ParameterScale %d, ParameterValue %p, StrLen_or_Ind %p)\n", StatementHandle, ParameterNumber, ValueType,
          ParameterType, debugstr_sqlulen(LengthPrecision), ParameterScale, ParameterValue, StrLen_or_Ind);

    if (!handle) return SQL_INVALID_HANDLE;
    if (!ParameterNumber)
    {
        FIXME( "parameter 0 not handled\n" );
        return SQL_ERROR;
    }
    if (!resize_binding( &handle->bind_param, ParameterNumber )) return SQL_ERROR;

    params.StatementHandle = handle->unix_handle;
    params.StrLen_or_Ind   = &handle->bind_param.param[i].len;
    if (SUCCESS(( ret = ODBC_CALL( SQLBindParam, &params )))) handle->bind_param.param[i].ptr = StrLen_or_Ind;
    TRACE ("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLCancel           [ODBC32.005]
 */
SQLRETURN WINAPI SQLCancel(SQLHSTMT StatementHandle)
{
    struct SQLCancel_params params;
    struct handle *handle = StatementHandle;
    SQLRETURN ret;

    TRACE("(StatementHandle %p)\n", StatementHandle);

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLCancel, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLCloseCursor           [ODBC32.026]
 */
SQLRETURN WINAPI SQLCloseCursor(SQLHSTMT StatementHandle)
{
    struct SQLCloseCursor_params params;
    struct handle *handle = StatementHandle;
    SQLRETURN ret;

    TRACE("(StatementHandle %p)\n", StatementHandle);

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLCloseCursor, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLColAttribute           [ODBC32.027]
 */
SQLRETURN WINAPI SQLColAttribute(SQLHSTMT StatementHandle, SQLUSMALLINT ColumnNumber, SQLUSMALLINT FieldIdentifier,
                                 SQLPOINTER CharacterAttribute, SQLSMALLINT BufferLength, SQLSMALLINT *StringLength,
                                 SQLLEN *NumericAttribute)
{
    struct SQLColAttribute_params params = { 0, ColumnNumber, FieldIdentifier, CharacterAttribute, BufferLength,
                                             StringLength };
    struct handle *handle = StatementHandle;
    INT64 num_attr = 0;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, ColumnNumber %d, FieldIdentifier %d, CharacterAttribute %p, BufferLength %d,"
          " StringLength %p, NumericAttribute %p)\n", StatementHandle, ColumnNumber, FieldIdentifier,
          CharacterAttribute, BufferLength, StringLength, NumericAttribute);

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle  = handle->unix_handle;
    params.NumericAttribute = &num_attr;
    if (SUCCESS(( ret = ODBC_CALL( SQLColAttribute, &params )))) *NumericAttribute = num_attr;
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
    struct SQLColumns_params params = { 0, CatalogName, NameLength1, SchemaName, NameLength2, TableName,
                                        NameLength3, ColumnName, NameLength4 };
    struct handle *handle = StatementHandle;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d, TableName %s,"
          " NameLength3 %d, ColumnName %s, NameLength4 %d)\n", StatementHandle,
          debugstr_an((const char *)CatalogName, NameLength1), NameLength1,
          debugstr_an((const char *)SchemaName, NameLength2), NameLength2,
          debugstr_an((const char *)TableName, NameLength3), NameLength3,
          debugstr_an((const char *)ColumnName, NameLength4), NameLength4);

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle  = handle->unix_handle;
    ret = ODBC_CALL( SQLColumns, &params );
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
    struct SQLConnect_params params = { 0, ServerName, NameLength1, UserName, NameLength2, Authentication, NameLength3 };
    struct handle *handle = ConnectionHandle;
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p, ServerName %s, NameLength1 %d, UserName %s, NameLength2 %d, Authentication %s,"
          " NameLength3 %d)\n", ConnectionHandle,
          debugstr_an((const char *)ServerName, NameLength1), NameLength1,
          debugstr_an((const char *)UserName, NameLength2), NameLength2,
          debugstr_an((const char *)Authentication, NameLength3), NameLength3);

    if (!handle) return SQL_INVALID_HANDLE;

    params.ConnectionHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLConnect, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLCopyDesc           [ODBC32.028]
 */
SQLRETURN WINAPI SQLCopyDesc(SQLHDESC SourceDescHandle, SQLHDESC TargetDescHandle)
{
    struct SQLCopyDesc_params params;
    struct handle *source = SourceDescHandle, *target = TargetDescHandle;
    SQLRETURN ret;

    TRACE("(SourceDescHandle %p, TargetDescHandle %p)\n", SourceDescHandle, TargetDescHandle);

    if (!source || !target) return SQL_INVALID_HANDLE;

    params.SourceDescHandle = source->unix_handle;
    params.TargetDescHandle = target->unix_handle;
    ret = ODBC_CALL( SQLCopyDesc, &params );
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
    struct SQLDataSources_params params = { 0, Direction, ServerName, BufferLength1, NameLength1, Description,
                                            BufferLength2, NameLength2 };
    struct handle *handle = EnvironmentHandle;
    SQLRETURN ret;

    TRACE("(EnvironmentHandle %p, Direction %d, ServerName %p, BufferLength1 %d, NameLength1 %p, Description %p,"
          " BufferLength2 %d, NameLength2 %p)\n", EnvironmentHandle, Direction, ServerName, BufferLength1,
          NameLength1, Description, BufferLength2, NameLength2);

    if (!handle) return SQL_INVALID_HANDLE;

    params.EnvironmentHandle = handle->unix_handle;
    if (SUCCESS((ret = ODBC_CALL( SQLDataSources, &params ))) && TRACE_ON(odbc))
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
    return SQLDataSources( EnvironmentHandle, Direction, ServerName, BufferLength1, NameLength1, Description,
                           BufferLength2, NameLength2 );
}

/*************************************************************************
 *				SQLDescribeCol           [ODBC32.008]
 */
SQLRETURN WINAPI SQLDescribeCol(SQLHSTMT StatementHandle, SQLUSMALLINT ColumnNumber, SQLCHAR *ColumnName,
                                SQLSMALLINT BufferLength, SQLSMALLINT *NameLength, SQLSMALLINT *DataType,
                                SQLULEN *ColumnSize, SQLSMALLINT *DecimalDigits, SQLSMALLINT *Nullable)
{
    struct SQLDescribeCol_params params = { 0, ColumnNumber, ColumnName, BufferLength, NameLength, DataType,
                                            NULL, DecimalDigits, Nullable };
    struct handle *handle = StatementHandle;
    UINT64 size;
    SQLSMALLINT dummy;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, ColumnNumber %d, ColumnName %p, BufferLength %d, NameLength %p, DataType %p,"
          " ColumnSize %p, DecimalDigits %p, Nullable %p)\n", StatementHandle, ColumnNumber, ColumnName,
          BufferLength, NameLength, DataType, ColumnSize, DecimalDigits, Nullable);

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle = handle->unix_handle;
    if (!params.NameLength) params.NameLength = &dummy; /* workaround for drivers that don't accept NULL NameLength */
    params.ColumnSize      = &size;
    if (SUCCESS((ret = ODBC_CALL( SQLDescribeCol, &params ))))
    {
        if (ColumnName && NameLength) TRACE(" ColumnName %s\n", debugstr_an((const char *)ColumnName, *NameLength));
        if (DataType) TRACE(" DataType %d\n", *DataType);
        if (ColumnSize)
        {
            *ColumnSize = size;
            TRACE(" ColumnSize %s\n", debugstr_sqlulen(*ColumnSize));
        }
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
    struct SQLDisconnect_params params;
    struct handle *handle = ConnectionHandle;
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p)\n", ConnectionHandle);

    if (!handle) return SQL_INVALID_HANDLE;

    params.ConnectionHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLDisconnect, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLEndTran           [ODBC32.029]
 */
SQLRETURN WINAPI SQLEndTran(SQLSMALLINT HandleType, SQLHANDLE Handle, SQLSMALLINT CompletionType)
{
    struct SQLEndTran_params params = { HandleType, 0, CompletionType };
    struct handle *handle = Handle;
    SQLRETURN ret;

    TRACE("(HandleType %d, Handle %p, CompletionType %d)\n", HandleType, Handle, CompletionType);

    if (!handle) return SQL_INVALID_HANDLE;

    params.Handle = handle->unix_handle;
    ret = ODBC_CALL( SQLEndTran, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLError           [ODBC32.010]
 */
SQLRETURN WINAPI SQLError(SQLHENV EnvironmentHandle, SQLHDBC ConnectionHandle, SQLHSTMT StatementHandle,
                          SQLCHAR *SqlState, SQLINTEGER *NativeError, SQLCHAR *MessageText,
                          SQLSMALLINT BufferLength, SQLSMALLINT *TextLength)
{
    struct SQLError_params params = { 0, 0, 0, SqlState, NativeError, MessageText, BufferLength, TextLength };
    struct handle *env = EnvironmentHandle, *con = ConnectionHandle, *stmt = StatementHandle;
    SQLRETURN ret;

    TRACE("(EnvironmentHandle %p, ConnectionHandle %p, StatementHandle %p, SqlState %p, NativeError %p,"
          " MessageText %p, BufferLength %d, TextLength %p)\n", EnvironmentHandle, ConnectionHandle,
          StatementHandle, SqlState, NativeError, MessageText, BufferLength, TextLength);

    if (env) params.EnvironmentHandle = env->unix_handle;
    if (con) params.ConnectionHandle = con->unix_handle;
    if (stmt) params.StatementHandle = stmt->unix_handle;
    if (SUCCESS((ret = ODBC_CALL( SQLError, &params ))))
    {
        TRACE(" SqlState %s\n", debugstr_an((const char *)SqlState, 5));
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
    struct SQLExecDirect_params params = { 0, StatementText, TextLength };
    struct handle *handle = StatementHandle;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, StatementText %s, TextLength %d)\n", StatementHandle,
          debugstr_an((const char *)StatementText, TextLength), TextLength);

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLExecDirect, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLExecute           [ODBC32.012]
 */
SQLRETURN WINAPI SQLExecute(SQLHSTMT StatementHandle)
{
    struct SQLExecute_params params;
    struct handle *handle = StatementHandle;
    SQLRETURN ret;

    TRACE("(StatementHandle %p)\n", StatementHandle);

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLExecute, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

static void update_result_lengths( struct handle *handle )
{
    UINT i;
    for (i = 0; i < handle->bind_col.count; i++)
    {
        if (handle->bind_col.param[i].ptr)
            *(SQLLEN *)handle->bind_col.param[i].ptr = handle->bind_col.param[i].len;
    }
    for (i = 0; i < handle->bind_param.count; i++)
    {
        if (handle->bind_param.param[i].ptr)
            *(SQLLEN *)handle->bind_param.param[i].ptr = handle->bind_param.param[i].len;
    }
    for (i = 0; i < handle->bind_parameter.count; i++)
    {
        if (handle->bind_parameter.param[i].ptr)
        {
            *(SQLLEN *)handle->bind_parameter.param[i].ptr = handle->bind_parameter.param[i].len;
        }
    }
}

/*************************************************************************
 *				SQLFetch           [ODBC32.013]
 */
SQLRETURN WINAPI SQLFetch(SQLHSTMT StatementHandle)
{
    struct SQLFetch_params params;
    struct handle *handle = StatementHandle;
    SQLRETURN ret;

    TRACE("(StatementHandle %p)\n", StatementHandle);

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle = handle->unix_handle;
    if (SUCCESS(( ret = ODBC_CALL( SQLFetch, &params )))) update_result_lengths( handle );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLFetchScroll          [ODBC32.030]
 */
SQLRETURN WINAPI SQLFetchScroll(SQLHSTMT StatementHandle, SQLSMALLINT FetchOrientation, SQLLEN FetchOffset)
{
    struct SQLFetchScroll_params params = { 0, FetchOrientation, FetchOffset };
    struct handle *handle = StatementHandle;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, FetchOrientation %d, FetchOffset %s)\n", StatementHandle, FetchOrientation,
          debugstr_sqllen(FetchOffset));

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle = handle->unix_handle;
    if (SUCCESS(( ret = ODBC_CALL( SQLFetchScroll, &params )))) update_result_lengths( handle );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLFreeConnect           [ODBC32.014]
 */
SQLRETURN WINAPI SQLFreeConnect(SQLHDBC ConnectionHandle)
{
    struct SQLFreeConnect_params params;
    struct handle *handle = ConnectionHandle;
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p)\n", ConnectionHandle);

    if (!handle) return SQL_INVALID_HANDLE;

    params.ConnectionHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLFreeConnect, &params );
    free( handle );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLFreeEnv           [ODBC32.015]
 */
SQLRETURN WINAPI SQLFreeEnv(SQLHENV EnvironmentHandle)
{
    struct SQLFreeEnv_params params;
    struct handle *handle = EnvironmentHandle;
    SQLRETURN ret;

    TRACE("(EnvironmentHandle %p)\n", EnvironmentHandle);

    if (!handle) return SQL_INVALID_HANDLE;

    params.EnvironmentHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLFreeEnv, &params );
    free( handle );
    TRACE("Returning %d\n", ret);
    return ret;
}

static void free_bindings( struct handle *handle )
{
    free( handle->bind_col.param );
    free( handle->bind_param.param );
    free( handle->bind_parameter.param );
}

/*************************************************************************
 *				SQLFreeHandle           [ODBC32.031]
 */
SQLRETURN WINAPI SQLFreeHandle(SQLSMALLINT HandleType, SQLHANDLE Handle)
{
    struct SQLFreeHandle_params params;
    struct handle *handle = Handle;
    SQLRETURN ret;

    TRACE("(HandleType %d, Handle %p)\n", HandleType, Handle);

    if (!handle) return SQL_INVALID_HANDLE;

    params.HandleType = HandleType;
    params.Handle     = handle->unix_handle;
    ret = ODBC_CALL( SQLFreeHandle, &params );
    free_bindings( handle );
    free( handle );
    TRACE ("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLFreeStmt           [ODBC32.016]
 */
SQLRETURN WINAPI SQLFreeStmt(SQLHSTMT StatementHandle, SQLUSMALLINT Option)
{
    struct SQLFreeStmt_params params;
    struct handle *handle = StatementHandle;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, Option %d)\n", StatementHandle, Option);

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle = handle->unix_handle;
    params.Option          = Option;
    ret = ODBC_CALL( SQLFreeStmt, &params );
    free_bindings( handle );
    free( handle );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetConnectAttr           [ODBC32.032]
 */
SQLRETURN WINAPI SQLGetConnectAttr(SQLHDBC ConnectionHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                   SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
    struct SQLGetConnectAttr_params params = { 0, Attribute, Value, BufferLength, StringLength };
    struct handle *handle = ConnectionHandle;
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p, Attribute %d, Value %p, BufferLength %d, StringLength %p)\n", ConnectionHandle,
          Attribute, Value, BufferLength, StringLength);

    if (!handle) return SQL_INVALID_HANDLE;

    params.ConnectionHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLGetConnectAttr, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetConnectOption       [ODBC32.042]
 */
SQLRETURN WINAPI SQLGetConnectOption(SQLHDBC ConnectionHandle, SQLUSMALLINT Option, SQLPOINTER Value)
{
    struct SQLGetConnectOption_params params = { 0, Option, Value };
    struct handle *handle = ConnectionHandle;
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p, Option %d, Value %p)\n", ConnectionHandle, Option, Value);

    if (!handle) return SQL_INVALID_HANDLE;

    params.ConnectionHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLGetConnectOption, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetCursorName           [ODBC32.017]
 */
SQLRETURN WINAPI SQLGetCursorName(SQLHSTMT StatementHandle, SQLCHAR *CursorName, SQLSMALLINT BufferLength,
                                  SQLSMALLINT *NameLength)
{
    struct SQLGetCursorName_params params = { 0, CursorName, BufferLength, NameLength };
    struct handle *handle = StatementHandle;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, CursorName %p, BufferLength %d, NameLength %p)\n", StatementHandle, CursorName,
          BufferLength, NameLength);

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLGetCursorName, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetData           [ODBC32.043]
 */
SQLRETURN WINAPI SQLGetData(SQLHSTMT StatementHandle, SQLUSMALLINT ColumnNumber, SQLSMALLINT TargetType,
                            SQLPOINTER TargetValue, SQLLEN BufferLength, SQLLEN *StrLen_or_Ind)
{
    struct SQLGetData_params params = { 0, ColumnNumber, TargetType, TargetValue, BufferLength };
    struct handle *handle = StatementHandle;
    INT64 len;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, ColumnNumber %d, TargetType %d, TargetValue %p, BufferLength %s, StrLen_or_Ind %p)\n",
          StatementHandle, ColumnNumber, TargetType, TargetValue, debugstr_sqllen(BufferLength), StrLen_or_Ind);

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle = handle->unix_handle;
    params.StrLen_or_Ind   = &len;
    if (SUCCESS((ret = ODBC_CALL( SQLGetData, &params )))) *StrLen_or_Ind = len;
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetDescField           [ODBC32.033]
 */
SQLRETURN WINAPI SQLGetDescField(SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber, SQLSMALLINT FieldIdentifier,
                                 SQLPOINTER Value, SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
    struct SQLGetDescField_params params = { 0, RecNumber, FieldIdentifier, Value, BufferLength, StringLength };
    struct handle *handle = DescriptorHandle;
    SQLRETURN ret;

    TRACE("(DescriptorHandle %p, RecNumber %d, FieldIdentifier %d, Value %p, BufferLength %d, StringLength %p)\n",
          DescriptorHandle, RecNumber, FieldIdentifier, Value, BufferLength, StringLength);

    if (!handle) return SQL_INVALID_HANDLE;

    params.DescriptorHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLGetDescField, &params );
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
    struct SQLGetDescRec_params params = { 0, RecNumber, Name, BufferLength, StringLength, Type, SubType, NULL,
                                           Precision, Scale, Nullable };
    struct handle *handle = DescriptorHandle;
    INT64 len;
    SQLRETURN ret;

    TRACE("(DescriptorHandle %p, RecNumber %d, Name %p, BufferLength %d, StringLength %p, Type %p, SubType %p,"
          " Length %p, Precision %p, Scale %p, Nullable %p)\n", DescriptorHandle, RecNumber, Name, BufferLength,
          StringLength, Type, SubType, Length, Precision, Scale, Nullable);

    if (!handle) return SQL_INVALID_HANDLE;

    params.DescriptorHandle = handle->unix_handle;
    params.Length           = &len;
    if (SUCCESS((ret = ODBC_CALL( SQLGetDescRec, &params )))) *Length = len;
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
    struct SQLGetDiagField_params params = { HandleType, 0, RecNumber, DiagIdentifier, DiagInfo, BufferLength,
                                             StringLength };
    struct handle *handle = Handle;
    SQLRETURN ret;

    TRACE("(HandleType %d, Handle %p, RecNumber %d, DiagIdentifier %d, DiagInfo %p, BufferLength %d,"
          " StringLength %p)\n", HandleType, Handle, RecNumber, DiagIdentifier, DiagInfo, BufferLength, StringLength);

    if (!handle) return SQL_INVALID_HANDLE;

    params.Handle = handle->unix_handle;
    ret = ODBC_CALL( SQLGetDiagField, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetDiagRec           [ODBC32.036]
 */
SQLRETURN WINAPI SQLGetDiagRec(SQLSMALLINT HandleType, SQLHANDLE Handle, SQLSMALLINT RecNumber,
                               SQLCHAR *SqlState, SQLINTEGER *NativeError, SQLCHAR *MessageText,
                               SQLSMALLINT BufferLength, SQLSMALLINT *TextLength)
{
    struct SQLGetDiagRec_params params = { HandleType, 0, RecNumber, SqlState, NativeError, MessageText,
                                           BufferLength, TextLength };
    struct handle *handle = Handle;
    SQLRETURN ret;

    TRACE("(HandleType %d, Handle %p, RecNumber %d, SqlState %p, NativeError %p, MessageText %p, BufferLength %d,"
          " TextLength %p)\n", HandleType, Handle, RecNumber, SqlState, NativeError, MessageText, BufferLength,
          TextLength);

    if (!handle) return SQL_INVALID_HANDLE;

    params.Handle = handle->unix_handle;
    ret = ODBC_CALL( SQLGetDiagRec, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetEnvAttr           [ODBC32.037]
 */
SQLRETURN WINAPI SQLGetEnvAttr(SQLHENV EnvironmentHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                               SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
    struct SQLGetEnvAttr_params params = { 0, Attribute, Value, BufferLength, StringLength };
    struct handle *handle = EnvironmentHandle;
    SQLRETURN ret;

    TRACE("(EnvironmentHandle %p, Attribute %d, Value %p, BufferLength %d, StringLength %p)\n",
          EnvironmentHandle, Attribute, Value, BufferLength, StringLength);

    if (!handle) return SQL_INVALID_HANDLE;

    params.EnvironmentHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLGetEnvAttr, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetFunctions           [ODBC32.044]
 */
SQLRETURN WINAPI SQLGetFunctions(SQLHDBC ConnectionHandle, SQLUSMALLINT FunctionId, SQLUSMALLINT *Supported)
{
    struct SQLGetFunctions_params params = { 0, FunctionId, Supported };
    struct handle *handle = ConnectionHandle;
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p, FunctionId %d, Supported %p)\n", ConnectionHandle, FunctionId, Supported);

    if (!handle) return SQL_INVALID_HANDLE;

    params.ConnectionHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLGetFunctions, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetInfo           [ODBC32.045]
 */
SQLRETURN WINAPI SQLGetInfo(SQLHDBC ConnectionHandle, SQLUSMALLINT InfoType, SQLPOINTER InfoValue,
                            SQLSMALLINT BufferLength, SQLSMALLINT *StringLength)
{
    struct SQLGetInfo_params params = { 0, InfoType, InfoValue, BufferLength, StringLength };
    struct handle *handle = ConnectionHandle;
    SQLRETURN ret;

    TRACE("(ConnectionHandle, %p, InfoType %d, InfoValue %p, BufferLength %d, StringLength %p)\n", ConnectionHandle,
          InfoType, InfoValue, BufferLength, StringLength);

    if (!handle) return SQL_INVALID_HANDLE;

    params.ConnectionHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLGetInfo, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetStmtAttr           [ODBC32.038]
 */
SQLRETURN WINAPI SQLGetStmtAttr(SQLHSTMT StatementHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
    struct SQLGetStmtAttr_params params = { 0, Attribute, Value, BufferLength, StringLength };
    struct handle *handle = StatementHandle;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, Attribute %d, Value %p, BufferLength %d, StringLength %p)\n", StatementHandle,
          Attribute, Value, BufferLength, StringLength);

    if (!Value)
    {
        WARN("Unexpected NULL Value return address\n");
        return SQL_ERROR;
    }

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLGetStmtAttr, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetStmtOption           [ODBC32.046]
 */
SQLRETURN WINAPI SQLGetStmtOption(SQLHSTMT StatementHandle, SQLUSMALLINT Option, SQLPOINTER Value)
{
    struct SQLGetStmtOption_params params = { 0, Option, Value };
    struct handle *handle = StatementHandle;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, Option %d, Value %p)\n", StatementHandle, Option, Value);

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLGetStmtOption, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetTypeInfo           [ODBC32.047]
 */
SQLRETURN WINAPI SQLGetTypeInfo(SQLHSTMT StatementHandle, SQLSMALLINT DataType)
{
    struct SQLGetTypeInfo_params params = { 0, DataType };
    struct handle *handle = StatementHandle;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, DataType %d)\n", StatementHandle, DataType);

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLGetTypeInfo, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLNumResultCols           [ODBC32.018]
 */
SQLRETURN WINAPI SQLNumResultCols(SQLHSTMT StatementHandle, SQLSMALLINT *ColumnCount)
{
    struct SQLNumResultCols_params params = { 0, ColumnCount };
    struct handle *handle = StatementHandle;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, ColumnCount %p)\n", StatementHandle, ColumnCount);

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLNumResultCols, &params );
    TRACE("Returning %d ColumnCount %d\n", ret, *ColumnCount);
    return ret;
}

/*************************************************************************
 *				SQLParamData           [ODBC32.048]
 */
SQLRETURN WINAPI SQLParamData(SQLHSTMT StatementHandle, SQLPOINTER *Value)
{
    struct SQLParamData_params params = { 0, Value };
    struct handle *handle = StatementHandle;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, Value %p)\n", StatementHandle, Value);

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLParamData, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLPrepare           [ODBC32.019]
 */
SQLRETURN WINAPI SQLPrepare(SQLHSTMT StatementHandle, SQLCHAR *StatementText, SQLINTEGER TextLength)
{
    struct SQLPrepare_params params = { 0, StatementText, TextLength };
    struct handle *handle = StatementHandle;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, StatementText %s, TextLength %d)\n", StatementHandle,
          debugstr_an((const char *)StatementText, TextLength), TextLength);

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLPrepare, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLPutData           [ODBC32.049]
 */
SQLRETURN WINAPI SQLPutData(SQLHSTMT StatementHandle, SQLPOINTER Data, SQLLEN StrLen_or_Ind)
{
    struct SQLPutData_params params = { 0, Data, StrLen_or_Ind };
    struct handle *handle = StatementHandle;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, Data %p, StrLen_or_Ind %s)\n", StatementHandle, Data, debugstr_sqllen(StrLen_or_Ind));

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLPutData, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLRowCount           [ODBC32.020]
 */
SQLRETURN WINAPI SQLRowCount(SQLHSTMT StatementHandle, SQLLEN *RowCount)
{
    struct SQLRowCount_params params;
    struct handle *handle = StatementHandle;
    INT64 count;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, RowCount %p)\n", StatementHandle, RowCount);

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle = handle->unix_handle;
    params.RowCount        = &count;
    if (SUCCESS((ret = ODBC_CALL( SQLRowCount, &params ))) && RowCount)
    {
        *RowCount = count;
        TRACE(" RowCount %s\n", debugstr_sqllen(*RowCount));
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
    struct SQLSetConnectAttr_params params = { 0, Attribute, Value, StringLength };
    struct handle *handle = ConnectionHandle;
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p, Attribute %d, Value %p, StringLength %d)\n", ConnectionHandle, Attribute, Value,
          StringLength);

    if (!handle) return SQL_INVALID_HANDLE;

    params.ConnectionHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLSetConnectAttr, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetConnectOption           [ODBC32.050]
 */
SQLRETURN WINAPI SQLSetConnectOption(SQLHDBC ConnectionHandle, SQLUSMALLINT Option, SQLULEN Value)
{
    struct SQLSetConnectOption_params params = { 0, Option, Value };
    struct handle *handle = ConnectionHandle;
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p, Option %d, Value %s)\n", ConnectionHandle, Option, debugstr_sqlulen(Value));

    if (!handle) return SQL_INVALID_HANDLE;

    params.ConnectionHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLSetConnectOption, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetCursorName           [ODBC32.021]
 */
SQLRETURN WINAPI SQLSetCursorName(SQLHSTMT StatementHandle, SQLCHAR *CursorName, SQLSMALLINT NameLength)
{
    struct SQLSetCursorName_params params = { 0, CursorName, NameLength };
    struct handle *handle = StatementHandle;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, CursorName %s, NameLength %d)\n", StatementHandle,
          debugstr_an((const char *)CursorName, NameLength), NameLength);

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLSetCursorName, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetDescField           [ODBC32.073]
 */
SQLRETURN WINAPI SQLSetDescField(SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber, SQLSMALLINT FieldIdentifier,
                                 SQLPOINTER Value, SQLINTEGER BufferLength)
{
    struct SQLSetDescField_params params = { 0, RecNumber, FieldIdentifier, Value, BufferLength };
    struct handle *handle = DescriptorHandle;
    SQLRETURN ret;

    TRACE("(DescriptorHandle %p, RecNumber %d, FieldIdentifier %d, Value %p, BufferLength %d)\n", DescriptorHandle,
          RecNumber, FieldIdentifier, Value, BufferLength);

    if (!handle) return SQL_INVALID_HANDLE;

    params.DescriptorHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLSetDescField, &params );
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
    struct SQLSetDescRec_params params = { 0, RecNumber, Type, SubType, Length, Precision, Scale, Data };
    struct handle *handle = DescriptorHandle;
    INT64 stringlen, indicator;
    SQLRETURN ret;

    TRACE("(DescriptorHandle %p, RecNumber %d, Type %d, SubType %d, Length %s, Precision %d, Scale %d, Data %p,"
          " StringLength %p, Indicator %p)\n", DescriptorHandle, RecNumber, Type, SubType, debugstr_sqllen(Length),
          Precision, Scale, Data, StringLength, Indicator);

    if (!handle) return SQL_INVALID_HANDLE;

    params.DescriptorHandle = handle->unix_handle;
    params.StringLength     = &stringlen;
    params.Indicator        = &indicator;
    if (SUCCESS((ret = ODBC_CALL( SQLSetDescRec, &params ))))
    {
        *StringLength = stringlen;
        *Indicator = indicator;
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
    struct SQLSetEnvAttr_params params = { 0, Attribute, Value, StringLength };
    struct handle *handle = EnvironmentHandle;
    SQLRETURN ret;

    TRACE("(EnvironmentHandle %p, Attribute %d, Value %p, StringLength %d)\n", EnvironmentHandle, Attribute, Value,
          StringLength);

    params.EnvironmentHandle = handle ? handle->unix_handle : 0;
    ret = ODBC_CALL( SQLSetEnvAttr, &params );
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
    struct SQLSetParam_params params = { 0, ParameterNumber, ValueType, ParameterType, LengthPrecision, ParameterScale,
                                         ParameterValue };
    struct handle *handle = StatementHandle;
    INT64 len;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, ParameterNumber %d, ValueType %d, ParameterType %d, LengthPrecision %s,"
          " ParameterScale %d, ParameterValue %p, StrLen_or_Ind %p)\n", StatementHandle, ParameterNumber, ValueType,
          ParameterType, debugstr_sqlulen(LengthPrecision), ParameterScale, ParameterValue, StrLen_or_Ind);

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle = handle->unix_handle;
    params.StrLen_or_Ind   = &len;
    if (SUCCESS((ret = ODBC_CALL( SQLSetParam, &params )))) *StrLen_or_Ind = len;
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetStmtAttr           [ODBC32.076]
 */
SQLRETURN WINAPI SQLSetStmtAttr(SQLHSTMT StatementHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                SQLINTEGER StringLength)
{
    struct SQLSetStmtAttr_params params = { 0, Attribute, Value, StringLength };
    struct handle *handle = StatementHandle;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, Attribute %d, Value %p, StringLength %d)\n", StatementHandle, Attribute, Value,
          StringLength);

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLSetStmtAttr, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetStmtOption           [ODBC32.051]
 */
SQLRETURN WINAPI SQLSetStmtOption(SQLHSTMT StatementHandle, SQLUSMALLINT Option, SQLULEN Value)
{
    struct SQLSetStmtOption_params params = { 0, Option, Value };
    struct handle *handle = StatementHandle;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, Option %d, Value %s)\n", StatementHandle, Option, debugstr_sqlulen(Value));

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLSetStmtOption, &params );
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
    struct SQLSpecialColumns_params params = { 0, IdentifierType, CatalogName, NameLength1, SchemaName, NameLength2,
                                               TableName, NameLength3, Scope, Nullable };
    struct handle *handle = StatementHandle;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, IdentifierType %d, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d,"
          " TableName %s, NameLength3 %d, Scope %d, Nullable %d)\n", StatementHandle, IdentifierType,
          debugstr_an((const char *)CatalogName, NameLength1), NameLength1,
          debugstr_an((const char *)SchemaName, NameLength2), NameLength2,
          debugstr_an((const char *)TableName, NameLength3), NameLength3, Scope, Nullable);

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLSpecialColumns, &params );
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
    struct SQLStatistics_params params = { 0, CatalogName, NameLength1, SchemaName, NameLength2, TableName,
                                           NameLength3, Unique, Reserved };
    struct handle *handle = StatementHandle;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d SchemaName %s, NameLength2 %d, TableName %s"
          " NameLength3 %d, Unique %d, Reserved %d)\n", StatementHandle,
          debugstr_an((const char *)CatalogName, NameLength1), NameLength1,
          debugstr_an((const char *)SchemaName, NameLength2), NameLength2,
          debugstr_an((const char *)TableName, NameLength3), NameLength3, Unique, Reserved);

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLStatistics, &params );
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
    struct SQLTables_params params = { 0, CatalogName, NameLength1, SchemaName, NameLength2, TableName,
                                       NameLength3, TableType, NameLength4 };
    struct handle *handle = StatementHandle;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d, TableName %s,"
          " NameLength3 %d, TableType %s, NameLength4 %d)\n", StatementHandle,
          debugstr_an((const char *)CatalogName, NameLength1), NameLength1,
          debugstr_an((const char *)SchemaName, NameLength2), NameLength2,
          debugstr_an((const char *)TableName, NameLength3), NameLength3,
          debugstr_an((const char *)TableType, NameLength4), NameLength4);

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLTables, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLTransact           [ODBC32.023]
 */
SQLRETURN WINAPI SQLTransact(SQLHENV EnvironmentHandle, SQLHDBC ConnectionHandle, SQLUSMALLINT CompletionType)
{
    struct SQLTransact_params params = { 0, 0, CompletionType };
    struct handle *env = EnvironmentHandle, *con = ConnectionHandle;
    SQLRETURN ret;

    TRACE("(EnvironmentHandle %p, ConnectionHandle %p, CompletionType %d)\n", EnvironmentHandle, ConnectionHandle,
          CompletionType);

    if (!env || !con) return SQL_INVALID_HANDLE;

    params.EnvironmentHandle = env->unix_handle;
    params.ConnectionHandle  = con->unix_handle;
    ret = ODBC_CALL( SQLTransact, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLBrowseConnect           [ODBC32.055]
 */
SQLRETURN WINAPI SQLBrowseConnect(SQLHDBC ConnectionHandle, SQLCHAR *InConnectionString, SQLSMALLINT StringLength1,
                                  SQLCHAR *OutConnectionString, SQLSMALLINT BufferLength, SQLSMALLINT *StringLength2)
{
    struct SQLBrowseConnect_params params = { 0, InConnectionString, StringLength1, OutConnectionString, BufferLength,
                                              StringLength2 };
    struct handle *handle = ConnectionHandle;
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p, InConnectionString %s, StringLength1 %d, OutConnectionString %p, BufferLength, %d, "
          "StringLength2 %p)\n", ConnectionHandle, debugstr_an((const char *)InConnectionString, StringLength1),
          StringLength1, OutConnectionString, BufferLength, StringLength2);

    if (!handle) return SQL_INVALID_HANDLE;

    params.ConnectionHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLBrowseConnect, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLBulkOperations           [ODBC32.078]
 */
SQLRETURN WINAPI SQLBulkOperations(SQLHSTMT StatementHandle, SQLSMALLINT Operation)
{
    struct SQLBulkOperations_params params = { 0, Operation };
    struct handle *handle = StatementHandle;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, Operation %d)\n", StatementHandle, Operation);

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle = handle->unix_handle;
    if (SUCCESS(( ret = ODBC_CALL( SQLBulkOperations, &params )))) update_result_lengths( handle );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLColAttributes           [ODBC32.006]
 */
SQLRETURN WINAPI SQLColAttributes(SQLHSTMT StatementHandle, SQLUSMALLINT ColumnNumber, SQLUSMALLINT FieldIdentifier,
                                  SQLPOINTER CharacterAttributes, SQLSMALLINT BufferLength, SQLSMALLINT *StringLength,
                                  SQLLEN *NumericAttributes)
{
    struct SQLColAttributes_params params = { 0, ColumnNumber, FieldIdentifier, CharacterAttributes, BufferLength,
                                              StringLength };
    struct handle *handle = StatementHandle;
    INT64 attrs;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, ColumnNumber %d, FieldIdentifier %d, CharacterAttributes %p, BufferLength %d, "
          "StringLength %p, NumericAttributes %p)\n", StatementHandle, ColumnNumber, FieldIdentifier,
          CharacterAttributes, BufferLength, StringLength, NumericAttributes);

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle   = handle->unix_handle;
    params.NumericAttributes = &attrs;
    if (SUCCESS((ret = ODBC_CALL( SQLColAttributes, &params )))) *NumericAttributes = attrs;
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLColumnPrivileges           [ODBC32.056]
 */
SQLRETURN WINAPI SQLColumnPrivileges(SQLHSTMT StatementHandle, SQLCHAR *CatalogName, SQLSMALLINT NameLength1,
                                     SQLCHAR *SchemaName, SQLSMALLINT NameLength2, SQLCHAR *TableName,
                                     SQLSMALLINT NameLength3, SQLCHAR *ColumnName, SQLSMALLINT NameLength4)
{
    struct SQLColumnPrivileges_params params = { 0, CatalogName, NameLength1, SchemaName, NameLength2,
                                                 TableName, NameLength3, ColumnName, NameLength4 };
    struct handle *handle = StatementHandle;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d, TableName %s,"
          " NameLength3 %d, ColumnName %s, NameLength4 %d)\n", StatementHandle,
          debugstr_an((const char *)CatalogName, NameLength1), NameLength1,
          debugstr_an((const char *)SchemaName, NameLength2), NameLength2,
          debugstr_an((const char *)TableName, NameLength3), NameLength3,
          debugstr_an((const char *)ColumnName, NameLength4), NameLength4);

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLColumnPrivileges, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLDescribeParam          [ODBC32.058]
 */
SQLRETURN WINAPI SQLDescribeParam(SQLHSTMT StatementHandle, SQLUSMALLINT ParameterNumber, SQLSMALLINT *DataType,
                                  SQLULEN *ParameterSize, SQLSMALLINT *DecimalDigits, SQLSMALLINT *Nullable)
{
    struct SQLDescribeParam_params params = { 0, ParameterNumber, DataType, NULL, DecimalDigits, Nullable };
    struct handle *handle = StatementHandle;
    UINT64 size;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, ParameterNumber %d, DataType %p, ParameterSize %p, DecimalDigits %p, Nullable %p)\n",
          StatementHandle, ParameterNumber, DataType, ParameterSize, DecimalDigits, Nullable);

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle = handle->unix_handle;
    params.ParameterSize   = &size;
    if (SUCCESS((ret = ODBC_CALL( SQLDescribeParam, &params )))) *ParameterSize = size;
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLExtendedFetch           [ODBC32.059]
 */
SQLRETURN WINAPI SQLExtendedFetch(SQLHSTMT StatementHandle, SQLUSMALLINT FetchOrientation, SQLLEN FetchOffset,
                                  SQLULEN *RowCount, SQLUSMALLINT *RowStatusArray)
{
    struct SQLExtendedFetch_params params = { 0, FetchOrientation, FetchOffset, NULL, RowStatusArray };
    struct handle *handle = StatementHandle;
    UINT64 count;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, FetchOrientation %d, FetchOffset %s, RowCount %p, RowStatusArray %p)\n",
          StatementHandle, FetchOrientation, debugstr_sqllen(FetchOffset), RowCount, RowStatusArray);

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle = handle->unix_handle;
    params.RowCount        = &count;
    if (SUCCESS((ret = ODBC_CALL( SQLExtendedFetch, &params )))) *RowCount = count;
    TRACE("Returning %d\n", ret);
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
    struct SQLForeignKeys_params params = { 0, PkCatalogName, NameLength1, PkSchemaName, NameLength2,
                                            PkTableName, NameLength3, FkCatalogName, NameLength4,
                                            FkSchemaName, NameLength5, FkTableName, NameLength6 };
    struct handle *handle = StatementHandle;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, PkCatalogName %s, NameLength1 %d, PkSchemaName %s, NameLength2 %d,"
          " PkTableName %s, NameLength3 %d, FkCatalogName %s, NameLength4 %d, FkSchemaName %s,"
          " NameLength5 %d, FkTableName %s, NameLength6 %d)\n", StatementHandle,
          debugstr_an((const char *)PkCatalogName, NameLength1), NameLength1,
          debugstr_an((const char *)PkSchemaName, NameLength2), NameLength2,
          debugstr_an((const char *)PkTableName, NameLength3), NameLength3,
          debugstr_an((const char *)FkCatalogName, NameLength4), NameLength4,
          debugstr_an((const char *)FkSchemaName, NameLength5), NameLength5,
          debugstr_an((const char *)FkTableName, NameLength6), NameLength6);

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLForeignKeys, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLMoreResults           [ODBC32.061]
 */
SQLRETURN WINAPI SQLMoreResults(SQLHSTMT StatementHandle)
{
    struct SQLMoreResults_params params;
    struct handle *handle = StatementHandle;
    SQLRETURN ret;

    TRACE("(%p)\n", StatementHandle);

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLMoreResults, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLNativeSql           [ODBC32.062]
 */
SQLRETURN WINAPI SQLNativeSql(SQLHDBC ConnectionHandle, SQLCHAR *InStatementText, SQLINTEGER TextLength1,
                              SQLCHAR *OutStatementText, SQLINTEGER BufferLength, SQLINTEGER *TextLength2)
{
    struct SQLNativeSql_params params = { 0, InStatementText, TextLength1, OutStatementText, BufferLength,
                                          TextLength2 };
    struct handle *handle = ConnectionHandle;
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p, InStatementText %s, TextLength1 %d, OutStatementText %p, BufferLength, %d, "
          "TextLength2 %p)\n", ConnectionHandle, debugstr_an((const char *)InStatementText, TextLength1),
          TextLength1, OutStatementText, BufferLength, TextLength2);

    if (!handle) return SQL_INVALID_HANDLE;

    params.ConnectionHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLNativeSql, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLNumParams           [ODBC32.063]
 */
SQLRETURN WINAPI SQLNumParams(SQLHSTMT StatementHandle, SQLSMALLINT *ParameterCount)
{
    struct SQLNumParams_params params = { 0, ParameterCount };
    struct handle *handle = StatementHandle;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, pcpar %p)\n", StatementHandle, ParameterCount);

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLNumParams, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLParamOptions           [ODBC32.064]
 */
SQLRETURN WINAPI SQLParamOptions(SQLHSTMT StatementHandle, SQLULEN RowCount, SQLULEN *RowNumber)
{
    struct SQLParamOptions_params params = { 0, RowCount };
    struct handle *handle = StatementHandle;
    UINT64 row;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, RowCount %s, RowNumber %p)\n", StatementHandle, debugstr_sqlulen(RowCount),
          RowNumber);

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle = handle->unix_handle;
    params.RowNumber       = &row;
    if (SUCCESS((ret = ODBC_CALL( SQLParamOptions, &params )))) *RowNumber = row;
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLPrimaryKeys           [ODBC32.065]
 */
SQLRETURN WINAPI SQLPrimaryKeys(SQLHSTMT StatementHandle, SQLCHAR *CatalogName, SQLSMALLINT NameLength1,
                                SQLCHAR *SchemaName, SQLSMALLINT NameLength2, SQLCHAR *TableName,
                                SQLSMALLINT NameLength3)
{
    struct SQLPrimaryKeys_params params = { 0, CatalogName, NameLength1, SchemaName, NameLength2, TableName,
                                            NameLength3 };
    struct handle *handle = StatementHandle;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d, TableName %s,"
          " NameLength3 %d)\n", StatementHandle,
          debugstr_an((const char *)CatalogName, NameLength1), NameLength1,
          debugstr_an((const char *)SchemaName, NameLength2), NameLength2,
          debugstr_an((const char *)TableName, NameLength3), NameLength3);

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLPrimaryKeys, &params );
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
    struct SQLProcedureColumns_params params = { 0, CatalogName, NameLength1, SchemaName, NameLength2,
                                                 ProcName, NameLength3, ColumnName, NameLength4 };
    struct handle *handle = StatementHandle;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d, ProcName %s,"
          " NameLength3 %d, ColumnName %s, NameLength4 %d)\n", StatementHandle,
          debugstr_an((const char *)CatalogName, NameLength1), NameLength1,
          debugstr_an((const char *)SchemaName, NameLength2), NameLength2,
          debugstr_an((const char *)ProcName, NameLength3), NameLength3,
          debugstr_an((const char *)ColumnName, NameLength4), NameLength4);

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLProcedureColumns, &params );
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
    struct SQLProcedures_params params = { 0, CatalogName, NameLength1, SchemaName, NameLength2, ProcName,
                                           NameLength3 };
    struct handle *handle = StatementHandle;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d, ProcName %s,"
          " NameLength3 %d)\n", StatementHandle,
          debugstr_an((const char *)CatalogName, NameLength1), NameLength1,
          debugstr_an((const char *)SchemaName, NameLength2), NameLength2,
          debugstr_an((const char *)ProcName, NameLength3), NameLength3);

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLProcedures, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetPos           [ODBC32.068]
 */
SQLRETURN WINAPI SQLSetPos(SQLHSTMT StatementHandle, SQLSETPOSIROW RowNumber, SQLUSMALLINT Operation,
                           SQLUSMALLINT LockType)
{
    struct SQLSetPos_params params = { 0, RowNumber, Operation, LockType };
    struct handle *handle = StatementHandle;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, RowNumber %s, Operation %d, LockType %d)\n", StatementHandle,
          debugstr_sqlulen(RowNumber), Operation, LockType);

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle = handle->unix_handle;
    if (SUCCESS(( ret = ODBC_CALL( SQLSetPos, &params ))) && Operation == SQL_REFRESH)
        update_result_lengths( handle );
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
    struct SQLTablePrivileges_params params = { 0, CatalogName, NameLength1, SchemaName, NameLength2,
                                                TableName, NameLength3 };
    struct handle *handle = StatementHandle;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d, TableName %s,"
          "NameLength3  %d)\n", StatementHandle,
          debugstr_an((const char *)CatalogName, NameLength1), NameLength1,
          debugstr_an((const char *)SchemaName, NameLength2), NameLength2,
          debugstr_an((const char *)TableName, NameLength3), NameLength3);

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLTablePrivileges, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLDrivers           [ODBC32.071]
 */
SQLRETURN WINAPI SQLDrivers(SQLHENV EnvironmentHandle, SQLUSMALLINT Direction, SQLCHAR *DriverDescription,
                            SQLSMALLINT BufferLength1, SQLSMALLINT *DescriptionLength,
                            SQLCHAR *DriverAttributes, SQLSMALLINT BufferLength2,
                            SQLSMALLINT *AttributesLength)
{
    struct SQLDrivers_params params = { 0, Direction, DriverDescription, BufferLength1, DescriptionLength,
                                        DriverAttributes, BufferLength2, AttributesLength };
    struct handle *handle = EnvironmentHandle;
    SQLRETURN ret;

    TRACE("(EnvironmentHandle %p, Direction %d, DriverDescription %p, BufferLength1 %d, DescriptionLength %p,"
          " DriverAttributes %p, BufferLength2 %d, AttributesLength %p)\n", EnvironmentHandle, Direction,
          DriverDescription, BufferLength1, DescriptionLength, DriverAttributes, BufferLength2, AttributesLength);

    if (!handle) return SQL_INVALID_HANDLE;

    params.EnvironmentHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLDrivers, &params );

    if (ret == SQL_NO_DATA && Direction == SQL_FETCH_FIRST)
        ERR_(winediag)("No ODBC drivers could be found. Check the settings for your libodbc provider.\n");

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
    struct SQLBindParameter_params params = { 0, ParameterNumber, InputOutputType, ValueType, ParameterType,
                                              ColumnSize, DecimalDigits, ParameterValue, BufferLength };
    struct handle *handle = StatementHandle;
    UINT i = ParameterNumber - 1;
    SQLRETURN ret;

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
    if (!resize_binding( &handle->bind_parameter, ParameterNumber )) return SQL_ERROR;

    params.StatementHandle = handle->unix_handle;
    params.StrLen_or_Ind   = &handle->bind_parameter.param[i].len;
    if (SUCCESS((ret = ODBC_CALL( SQLBindParameter, &params )))) handle->bind_parameter.param[i].ptr = StrLen_or_Ind;
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLDriverConnect           [ODBC32.041]
 */
SQLRETURN WINAPI SQLDriverConnect(SQLHDBC ConnectionHandle, SQLHWND WindowHandle, SQLCHAR *ConnectionString,
                                  SQLSMALLINT Length, SQLCHAR *OutConnectionString, SQLSMALLINT BufferLength,
                                  SQLSMALLINT *Length2, SQLUSMALLINT DriverCompletion)
{
    struct SQLDriverConnect_params params = { 0, WindowHandle, ConnectionString, Length, OutConnectionString,
                                              BufferLength, Length2, DriverCompletion };
    struct handle *handle = ConnectionHandle;
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p, hwnd %p, ConnectionString %s, Length %d, conn_str_out %p, conn_str_out_max %d,"
          " ptr_conn_str_out %p, driver_completion %d)\n", ConnectionHandle, WindowHandle,
          debugstr_an((const char *)ConnectionString, Length), Length, OutConnectionString, BufferLength,
          Length2, DriverCompletion);

    if (!handle) return SQL_INVALID_HANDLE;

    params.ConnectionHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLDriverConnect, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetScrollOptions           [ODBC32.069]
 */
SQLRETURN WINAPI SQLSetScrollOptions(SQLHSTMT StatementHandle, SQLUSMALLINT Concurrency, SQLLEN KeySetSize,
                                     SQLUSMALLINT RowSetSize)
{
    struct SQLSetScrollOptions_params params = { 0, Concurrency, KeySetSize, RowSetSize };
    struct handle *handle = StatementHandle;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, Concurrency %d, KeySetSize %s, RowSetSize %d)\n", StatementHandle,
          Concurrency, debugstr_sqllen(KeySetSize), RowSetSize);

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLSetScrollOptions, &params );
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
SQLRETURN WINAPI SQLColAttributesW(SQLHSTMT StatementHandle, SQLUSMALLINT ColumnNumber, SQLUSMALLINT FieldIdentifier,
                                   SQLPOINTER CharacterAttributes, SQLSMALLINT BufferLength, SQLSMALLINT *StringLength,
                                   SQLLEN *NumericAttributes)
{
    struct SQLColAttributesW_params params = { 0, ColumnNumber, FieldIdentifier, CharacterAttributes, BufferLength,
                                               StringLength };
    struct handle *handle = StatementHandle;
    INT64 attrs;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, ColumnNumber %d, FieldIdentifier %d, CharacterAttributes %p, BufferLength %d, "
          "StringLength %p, NumericAttributes %p)\n", StatementHandle, ColumnNumber, FieldIdentifier,
          CharacterAttributes, BufferLength, StringLength, NumericAttributes);

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle   = handle->unix_handle;
    params.NumericAttributes = &attrs;
    if (SUCCESS((ret = ODBC_CALL( SQLColAttributesW, &params )))) *NumericAttributes = attrs;

    if (ret == SQL_SUCCESS && SQLColAttributes_KnownStringAttribute(FieldIdentifier) && CharacterAttributes &&
        StringLength && *StringLength != wcslen(CharacterAttributes) * 2)
    {
        TRACE("CHEAT: resetting name length for ADO\n");
        *StringLength = wcslen(CharacterAttributes) * 2;
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
    struct SQLConnectW_params params = { 0, ServerName, NameLength1, UserName, NameLength2, Authentication,
                                         NameLength3 };
    struct handle *handle = ConnectionHandle;
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p, ServerName %s, NameLength1 %d, UserName %s, NameLength2 %d, Authentication %s,"
          " NameLength3 %d)\n", ConnectionHandle, debugstr_wn(ServerName, NameLength1), NameLength1,
          debugstr_wn(UserName, NameLength2), NameLength2, debugstr_wn(Authentication, NameLength3), NameLength3);

    if (!handle) return SQL_INVALID_HANDLE;

    params.ConnectionHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLConnectW, &params );
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
    struct SQLDescribeColW_params params = { 0, ColumnNumber, ColumnName, BufferLength, NameLength, DataType,
                                             NULL, DecimalDigits, Nullable };
    struct handle *handle = StatementHandle;
    SQLSMALLINT dummy;
    UINT64 size;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, ColumnNumber %d, ColumnName %p, BufferLength %d, NameLength %p, DataType %p,"
          " ColumnSize %p, DecimalDigits %p, Nullable %p)\n", StatementHandle, ColumnNumber, ColumnName,
          BufferLength, NameLength, DataType, ColumnSize, DecimalDigits, Nullable);

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle = handle->unix_handle;
    if (!NameLength) params.NameLength = &dummy; /* workaround for drivers that don't accept NULL NameLength */
    params.ColumnSize      = &size;
    if (SUCCESS((ret = ODBC_CALL( SQLDescribeColW, &params ))))
    {
        if (ColumnName && NameLength) TRACE("ColumnName %s\n", debugstr_wn(ColumnName, *NameLength));
        if (DataType) TRACE("DataType %d\n", *DataType);
        if (ColumnSize)
        {
            *ColumnSize = size;
            TRACE("ColumnSize %s\n", debugstr_sqlulen(*ColumnSize));
        }
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
                           WCHAR *SqlState, SQLINTEGER *NativeError, WCHAR *MessageText,
                           SQLSMALLINT BufferLength, SQLSMALLINT *TextLength)
{
    struct SQLErrorW_params params = { 0, 0, 0, SqlState, NativeError, MessageText, BufferLength, TextLength };
    struct handle *env = EnvironmentHandle, *con = ConnectionHandle, *stmt = StatementHandle;
    SQLRETURN ret;

    TRACE("(EnvironmentHandle %p, ConnectionHandle %p, StatementHandle %p, SqlState %p, NativeError %p,"
          " MessageText %p, BufferLength %d, TextLength %p)\n", EnvironmentHandle, ConnectionHandle,
          StatementHandle, SqlState, NativeError, MessageText, BufferLength, TextLength);

    if (env) params.EnvironmentHandle = env->unix_handle;
    if (con) params.ConnectionHandle = con->unix_handle;
    if (stmt) params.StatementHandle = stmt->unix_handle;
    if (SUCCESS((ret = ODBC_CALL( SQLErrorW, &params ))))
    {
        TRACE(" SqlState %s\n", debugstr_wn(SqlState, 5));
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
    struct SQLExecDirectW_params params = { 0, StatementText, TextLength };
    struct handle *handle = StatementHandle;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, StatementText %s, TextLength %d)\n", StatementHandle,
          debugstr_wn(StatementText, TextLength), TextLength);

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLExecDirectW, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetCursorNameW          [ODBC32.117]
 */
SQLRETURN WINAPI SQLGetCursorNameW(SQLHSTMT StatementHandle, WCHAR *CursorName, SQLSMALLINT BufferLength,
                                   SQLSMALLINT *NameLength)
{
    struct SQLGetCursorNameW_params params = { 0, CursorName, BufferLength, NameLength };
    struct handle *handle = StatementHandle;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, CursorName %p, BufferLength %d, NameLength %p)\n", StatementHandle, CursorName,
          BufferLength, NameLength);

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLGetCursorNameW, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLPrepareW          [ODBC32.119]
 */
SQLRETURN WINAPI SQLPrepareW(SQLHSTMT StatementHandle, WCHAR *StatementText, SQLINTEGER TextLength)
{
    struct SQLPrepareW_params params = { 0, StatementText, TextLength };
    struct handle *handle = StatementHandle;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, StatementText %s, TextLength %d)\n", StatementHandle,
          debugstr_wn(StatementText, TextLength), TextLength);

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLPrepareW, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetCursorNameW          [ODBC32.121]
 */
SQLRETURN WINAPI SQLSetCursorNameW(SQLHSTMT StatementHandle, WCHAR *CursorName, SQLSMALLINT NameLength)
{
    struct SQLSetCursorNameW_params params = { 0, CursorName, NameLength };
    struct handle *handle = StatementHandle;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, CursorName %s, NameLength %d)\n", StatementHandle,
          debugstr_wn(CursorName, NameLength), NameLength);

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLSetCursorNameW, &params );
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
    struct SQLColAttributeW_params params = { 0, ColumnNumber, FieldIdentifier, CharacterAttribute, BufferLength,
                                              StringLength };
    struct handle *handle = StatementHandle;
    INT64 attr;
    SQLRETURN ret;

    TRACE("StatementHandle %p ColumnNumber %d FieldIdentifier %d CharacterAttribute %p BufferLength %d"
          " StringLength %p NumericAttribute %p\n", StatementHandle, ColumnNumber, FieldIdentifier,
          CharacterAttribute, BufferLength, StringLength, NumericAttribute);

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle  = handle->unix_handle;
    params.NumericAttribute = &attr;
    if (SUCCESS((ret = ODBC_CALL( SQLColAttributeW, &params )))) *NumericAttribute = attr;

    if (ret == SQL_SUCCESS && CharacterAttribute != NULL && SQLColAttributes_KnownStringAttribute(FieldIdentifier) &&
        StringLength && *StringLength != wcslen(CharacterAttribute) * 2)
    {
        TRACE("CHEAT: resetting name length for ADO\n");
        *StringLength = wcslen(CharacterAttribute) * 2;
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
    struct SQLGetConnectAttrW_params params = { 0, Attribute, Value, BufferLength, StringLength };
    struct handle *handle = ConnectionHandle;
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p, Attribute %d, Value %p, BufferLength %d, StringLength %p)\n", ConnectionHandle,
          Attribute, Value, BufferLength, StringLength);

    if (!handle) return SQL_INVALID_HANDLE;

    params.ConnectionHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLGetConnectAttrW, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetDescFieldW          [ODBC32.133]
 */
SQLRETURN WINAPI SQLGetDescFieldW(SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber, SQLSMALLINT FieldIdentifier,
                                  SQLPOINTER Value, SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
    struct SQLGetDescFieldW_params params = { 0, RecNumber, FieldIdentifier, Value, BufferLength, StringLength };
    struct handle *handle = DescriptorHandle;
    SQLRETURN ret;

    TRACE("(DescriptorHandle %p, RecNumber %d, FieldIdentifier %d, Value %p, BufferLength %d, StringLength %p)\n",
          DescriptorHandle, RecNumber, FieldIdentifier, Value, BufferLength, StringLength);

    if (!handle) return SQL_INVALID_HANDLE;

    params.DescriptorHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLGetDescFieldW, &params );
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
    struct SQLGetDescRecW_params params = { 0, RecNumber, Name, BufferLength, StringLength, Type, SubType,
                                            NULL, Precision, Scale, Nullable };
    struct handle *handle = DescriptorHandle;
    INT64 len;
    SQLRETURN ret;

    TRACE("(DescriptorHandle %p, RecNumber %d, Name %p, BufferLength %d, StringLength %p, Type %p, SubType %p,"
          " Length %p, Precision %p, Scale %p, Nullable %p)\n", DescriptorHandle, RecNumber, Name, BufferLength,
          StringLength, Type, SubType, Length, Precision, Scale, Nullable);

    if (!handle) return SQL_INVALID_HANDLE;

    params.DescriptorHandle = handle->unix_handle;
    params.Length           = &len;
    if (SUCCESS((ret = ODBC_CALL( SQLGetDescRecW, &params )))) *Length = len;
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
    struct SQLGetDiagFieldW_params params = { HandleType, 0, RecNumber, DiagIdentifier, DiagInfo, BufferLength,
                                              StringLength };
    struct handle *handle = Handle;
    SQLRETURN ret;

    TRACE("(HandleType %d, Handle %p, RecNumber %d, DiagIdentifier %d, DiagInfo %p, BufferLength %d,"
          " StringLength %p)\n", HandleType, Handle, RecNumber, DiagIdentifier, DiagInfo, BufferLength, StringLength);

    if (!handle) return SQL_INVALID_HANDLE;

    params.Handle = handle->unix_handle;
    ret = ODBC_CALL( SQLGetDiagFieldW, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetDiagRecW           [ODBC32.136]
 */
SQLRETURN WINAPI SQLGetDiagRecW(SQLSMALLINT HandleType, SQLHANDLE Handle, SQLSMALLINT RecNumber, WCHAR *SqlState,
                                SQLINTEGER *NativeError, WCHAR *MessageText, SQLSMALLINT BufferLength,
                                SQLSMALLINT *TextLength)
{
    struct SQLGetDiagRecW_params params = { HandleType, 0, RecNumber, SqlState, NativeError, MessageText,
                                            BufferLength, TextLength };
    struct handle *handle = Handle;
    SQLRETURN ret;

    TRACE("(HandleType %d, Handle %p, RecNumber %d, SqlState %p, NativeError %p, MessageText %p, BufferLength %d,"
          " TextLength %p)\n", HandleType, Handle, RecNumber, SqlState, NativeError, MessageText, BufferLength,
          TextLength);

    if (!handle) return SQL_INVALID_HANDLE;

    params.Handle = handle->unix_handle;
    ret = ODBC_CALL( SQLGetDiagRecW, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetStmtAttrW          [ODBC32.138]
 */
SQLRETURN WINAPI SQLGetStmtAttrW(SQLHSTMT StatementHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                 SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
    struct SQLGetStmtAttrW_params params = { 0, Attribute, Value, BufferLength, StringLength };
    struct handle *handle = StatementHandle;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, Attribute %d, Value %p, BufferLength %d, StringLength %p)\n", StatementHandle,
          Attribute, Value, BufferLength, StringLength);

    if (!Value)
    {
        WARN("Unexpected NULL Value return address\n");
        return SQL_ERROR;
    }

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLGetStmtAttrW, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetConnectAttrW          [ODBC32.139]
 */
SQLRETURN WINAPI SQLSetConnectAttrW(SQLHDBC ConnectionHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                    SQLINTEGER StringLength)
{
    struct SQLSetConnectAttrW_params params = { 0, Attribute, Value, StringLength };
    struct handle *handle = ConnectionHandle;
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p, Attribute %d, Value %p, StringLength %d)\n", ConnectionHandle, Attribute,
          Value, StringLength);

    if (!handle) return SQL_INVALID_HANDLE;

    params.ConnectionHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLSetConnectAttrW, &params );
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
    struct SQLColumnsW_params params = { 0, CatalogName, NameLength1, SchemaName, NameLength2, TableName,
                                         NameLength3, ColumnName, NameLength4 };
    struct handle *handle = StatementHandle;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d, TableName %s,"
          " NameLength3 %d, ColumnName %s, NameLength4 %d)\n", StatementHandle,
          debugstr_wn(CatalogName, NameLength1), NameLength1, debugstr_wn(SchemaName, NameLength2), NameLength2,
          debugstr_wn(TableName, NameLength3), NameLength3, debugstr_wn(ColumnName, NameLength4), NameLength4);

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLColumnsW, &params );
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
    struct SQLDriverConnectW_params params = { 0, WindowHandle, InConnectionString, Length, OutConnectionString,
                                               BufferLength, Length2, DriverCompletion };
    struct handle *handle = ConnectionHandle;
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p, WindowHandle %p, InConnectionString %s, Length %d, OutConnectionString %p,"
          " BufferLength %d, Length2 %p, DriverCompletion %d)\n", ConnectionHandle, WindowHandle,
          debugstr_wn(InConnectionString, Length), Length, OutConnectionString, BufferLength, Length2,
          DriverCompletion);

    if (!handle) return SQL_INVALID_HANDLE;

    params.ConnectionHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLDriverConnectW, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetConnectOptionW      [ODBC32.142]
 */
SQLRETURN WINAPI SQLGetConnectOptionW(SQLHDBC ConnectionHandle, SQLUSMALLINT Option, SQLPOINTER Value)
{
    struct SQLGetConnectOptionW_params params = { 0, Option, Value };
    struct handle *handle = ConnectionHandle;
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p, Option %d, Value %p)\n", ConnectionHandle, Option, Value);

    if (!handle) return SQL_INVALID_HANDLE;

    params.ConnectionHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLGetConnectOptionW, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetInfoW          [ODBC32.145]
 */
SQLRETURN WINAPI SQLGetInfoW(SQLHDBC ConnectionHandle, SQLUSMALLINT InfoType, SQLPOINTER InfoValue,
                             SQLSMALLINT BufferLength, SQLSMALLINT *StringLength)
{
    struct SQLGetInfoW_params params = { 0, InfoType, InfoValue, BufferLength, StringLength };
    struct handle *handle = ConnectionHandle;
    SQLRETURN ret;

    TRACE("(ConnectionHandle, %p, InfoType %d, InfoValue %p, BufferLength %d, StringLength %p)\n", ConnectionHandle,
          InfoType, InfoValue, BufferLength, StringLength);

    if (!handle) return SQL_INVALID_HANDLE;

    params.ConnectionHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLGetInfoW, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLGetTypeInfoW          [ODBC32.147]
 */
SQLRETURN WINAPI SQLGetTypeInfoW(SQLHSTMT StatementHandle, SQLSMALLINT DataType)
{
    struct SQLGetTypeInfoW_params params = { 0, DataType };
    struct handle *handle = StatementHandle;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, DataType %d)\n", StatementHandle, DataType);

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLGetTypeInfoW, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetConnectOptionW          [ODBC32.150]
 */
SQLRETURN WINAPI SQLSetConnectOptionW(SQLHDBC ConnectionHandle, SQLUSMALLINT Option, SQLULEN Value)
{
    struct SQLSetConnectOptionW_params params = { 0, Option, Value };
    struct handle *handle = ConnectionHandle;
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p, Option %d, Value %s)\n", ConnectionHandle, Option, debugstr_sqllen(Value));

    if (!handle) return SQL_INVALID_HANDLE;

    params.ConnectionHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLSetConnectOptionW, &params );
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
    struct SQLSpecialColumnsW_params params = { 0, IdentifierType, CatalogName, NameLength1, SchemaName, NameLength2,
                                                TableName, NameLength3, Scope, Nullable };
    struct handle *handle = StatementHandle;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, IdentifierType %d, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d,"
          " TableName %s, NameLength3 %d, Scope %d, Nullable %d)\n", StatementHandle, IdentifierType,
          debugstr_wn(CatalogName, NameLength1), NameLength1, debugstr_wn(SchemaName, NameLength2), NameLength2,
          debugstr_wn(TableName, NameLength3), NameLength3, Scope, Nullable);

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLSpecialColumnsW, &params );
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
    struct SQLStatisticsW_params params = { 0, CatalogName, NameLength1, SchemaName, NameLength2, TableName,
                                            NameLength3, Unique, Reserved };
    struct handle *handle = StatementHandle;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d SchemaName %s, NameLength2 %d, TableName %s"
          " NameLength3 %d, Unique %d, Reserved %d)\n", StatementHandle,
          debugstr_wn(CatalogName, NameLength1), NameLength1, debugstr_wn(SchemaName, NameLength2), NameLength2,
          debugstr_wn(TableName, NameLength3), NameLength3, Unique, Reserved);

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLStatisticsW, &params );
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
    struct SQLTablesW_params params = { 0, CatalogName, NameLength1, SchemaName, NameLength2, TableName,
                                        NameLength3, TableType, NameLength4 };
    struct handle *handle = StatementHandle;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d, TableName %s,"
          " NameLength3 %d, TableType %s, NameLength4 %d)\n", StatementHandle,
          debugstr_wn(CatalogName, NameLength1), NameLength1, debugstr_wn(SchemaName, NameLength2), NameLength2,
          debugstr_wn(TableName, NameLength3), NameLength3, debugstr_wn(TableType, NameLength4), NameLength4);

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLTablesW, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLBrowseConnectW          [ODBC32.155]
 */
SQLRETURN WINAPI SQLBrowseConnectW(SQLHDBC ConnectionHandle, SQLWCHAR *InConnectionString, SQLSMALLINT StringLength1,
                                   SQLWCHAR *OutConnectionString, SQLSMALLINT BufferLength, SQLSMALLINT *StringLength2)
{
    struct SQLBrowseConnectW_params params = { 0, InConnectionString, StringLength1, OutConnectionString,
                                               BufferLength, StringLength2 };
    struct handle *handle = ConnectionHandle;
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p, InConnectionString %s, StringLength1 %d, OutConnectionString %p, BufferLength %d, "
          "StringLength2 %p)\n", ConnectionHandle, debugstr_wn(InConnectionString, StringLength1), StringLength1,
          OutConnectionString, BufferLength, StringLength2);

    if (!handle) return SQL_INVALID_HANDLE;

    params.ConnectionHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLBrowseConnectW, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLColumnPrivilegesW          [ODBC32.156]
 */
SQLRETURN WINAPI SQLColumnPrivilegesW(SQLHSTMT StatementHandle, SQLWCHAR *CatalogName, SQLSMALLINT NameLength1,
                                      SQLWCHAR *SchemaName, SQLSMALLINT NameLength2, SQLWCHAR *TableName,
                                      SQLSMALLINT NameLength3, SQLWCHAR *ColumnName, SQLSMALLINT NameLength4)
{
    struct SQLColumnPrivilegesW_params params = { 0, CatalogName, NameLength1, SchemaName, NameLength2,
                                                  TableName, NameLength3, ColumnName, NameLength4 };
    struct handle *handle = StatementHandle;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d, TableName %s,"
          " NameLength3 %d, ColumnName %s, NameLength3 %d)\n", StatementHandle,
          debugstr_wn(CatalogName, NameLength1), NameLength1,
          debugstr_wn(SchemaName, NameLength2), NameLength2,
          debugstr_wn(TableName, NameLength3), NameLength3,
          debugstr_wn(ColumnName, NameLength4), NameLength4);

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLColumnPrivilegesW, &params );
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
    struct SQLDataSourcesW_params params = { 0, Direction, ServerName, BufferLength1, NameLength1, Description,
                                             BufferLength2, NameLength2 };
    struct handle *handle = EnvironmentHandle;
    SQLRETURN ret;

    TRACE("(EnvironmentHandle %p, Direction %d, ServerName %p, BufferLength1 %d, NameLength1 %p, Description %p,"
          " BufferLength2 %d, NameLength2 %p)\n", EnvironmentHandle, Direction, ServerName, BufferLength1,
          NameLength1, Description, BufferLength2, NameLength2);

    if (!handle) return SQL_INVALID_HANDLE;

    params.EnvironmentHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLDataSourcesW, &params );

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
SQLRETURN WINAPI SQLForeignKeysW(SQLHSTMT StatementHandle, SQLWCHAR *PkCatalogName, SQLSMALLINT NameLength1,
                                 SQLWCHAR *PkSchemaName, SQLSMALLINT NameLength2, SQLWCHAR *PkTableName,
                                 SQLSMALLINT NameLength3, SQLWCHAR *FkCatalogName, SQLSMALLINT NameLength4,
                                 SQLWCHAR *FkSchemaName, SQLSMALLINT NameLength5, SQLWCHAR *FkTableName,
                                 SQLSMALLINT NameLength6)
{
    struct SQLForeignKeysW_params params = { 0, PkCatalogName, NameLength1, PkSchemaName, NameLength2,
                                             PkTableName, NameLength2, FkCatalogName, NameLength3,
                                             FkSchemaName, NameLength5, FkTableName, NameLength6 };
    struct handle *handle = StatementHandle;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, PkCatalogName %s, NameLength1 %d, PkSchemaName %s, NameLength2 %d,"
          " PkTableName %s, NameLength3 %d, FkCatalogName %s, NameLength4 %d, FkSchemaName %s,"
          " NameLength5 %d, FkTableName %s, NameLength6 %d)\n", StatementHandle,
          debugstr_wn(PkCatalogName, NameLength1), NameLength1,
          debugstr_wn(PkSchemaName, NameLength2), NameLength2,
          debugstr_wn(PkTableName, NameLength3), NameLength3,
          debugstr_wn(FkCatalogName, NameLength4), NameLength4,
          debugstr_wn(FkSchemaName, NameLength5), NameLength5,
          debugstr_wn(FkTableName, NameLength6), NameLength6);

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLForeignKeysW, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLNativeSqlW          [ODBC32.162]
 */
SQLRETURN WINAPI SQLNativeSqlW(SQLHDBC ConnectionHandle, SQLWCHAR *InStatementText, SQLINTEGER TextLength1,
                               SQLWCHAR *OutStatementText, SQLINTEGER BufferLength, SQLINTEGER *TextLength2)
{
    struct SQLNativeSqlW_params params = { 0, InStatementText, TextLength1, OutStatementText, BufferLength,
                                           TextLength2 };
    struct handle *handle = ConnectionHandle;
    SQLRETURN ret;

    TRACE("(ConnectionHandle %p, InStatementText %s, TextLength1 %d, OutStatementText %p, BufferLength %d, "
          "TextLength2 %p)\n", ConnectionHandle, debugstr_wn(InStatementText, TextLength1), TextLength1,
          OutStatementText, BufferLength, TextLength2);

    if (!handle) return SQL_INVALID_HANDLE;

    params.ConnectionHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLNativeSqlW, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLPrimaryKeysW          [ODBC32.165]
 */
SQLRETURN WINAPI SQLPrimaryKeysW(SQLHSTMT StatementHandle, SQLWCHAR *CatalogName, SQLSMALLINT NameLength1,
                                 SQLWCHAR *SchemaName, SQLSMALLINT NameLength2, SQLWCHAR *TableName,
                                 SQLSMALLINT NameLength3)
{
    struct SQLPrimaryKeysW_params params = { 0, CatalogName, NameLength1, SchemaName, NameLength2, TableName,
                                             NameLength2 };
    struct handle *handle = StatementHandle;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d, TableName %s,"
          " NameLength3 %d)\n", StatementHandle,
          debugstr_wn(CatalogName, NameLength1), NameLength1,
          debugstr_wn(SchemaName, NameLength2), NameLength2,
          debugstr_wn(TableName, NameLength3), NameLength3);

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLPrimaryKeysW, &params );
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
    struct SQLProcedureColumnsW_params params = { 0, CatalogName, NameLength1, SchemaName, NameLength2,
                                                  ProcName, NameLength3, ColumnName, NameLength4 };
    struct handle *handle = StatementHandle;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d, ProcName %s,"
          " NameLength3 %d, ColumnName %s, NameLength4 %d)\n", StatementHandle,
          debugstr_wn(CatalogName, NameLength1), NameLength1,
          debugstr_wn(SchemaName, NameLength2), NameLength2,
          debugstr_wn(ProcName, NameLength3), NameLength3,
          debugstr_wn(ColumnName, NameLength4), NameLength4);

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLProcedureColumnsW, &params );
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
    struct SQLProceduresW_params params = { 0, CatalogName, NameLength1, SchemaName, NameLength2, ProcName,
                                            NameLength3 };
    struct handle *handle = StatementHandle;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d, ProcName %s,"
          " NameLength3 %d)\n", StatementHandle, debugstr_wn(CatalogName, NameLength1), NameLength1,
          debugstr_wn(SchemaName, NameLength2), NameLength2, debugstr_wn(ProcName, NameLength3), NameLength3);

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLProceduresW, &params );
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
    struct SQLTablePrivilegesW_params params = { 0, CatalogName, NameLength1, SchemaName, NameLength2, TableName,
                                                 NameLength3 };
    struct handle *handle = StatementHandle;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, CatalogName %s, NameLength1 %d, SchemaName %s, NameLength2 %d, TableName %s,"
          " NameLength3 %d)\n", StatementHandle, debugstr_wn(CatalogName, NameLength1), NameLength1,
          debugstr_wn(SchemaName, NameLength2), NameLength2, debugstr_wn(TableName, NameLength3), NameLength3);

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLTablePrivilegesW, &params );
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
    struct SQLDriversW_params params = { 0, Direction, DriverDescription, BufferLength1, DescriptionLength,
                                         DriverAttributes, BufferLength2, AttributesLength };
    struct handle *handle = EnvironmentHandle;
    SQLRETURN ret;

    TRACE("(EnvironmentHandle %p, Direction %d, DriverDescription %p, BufferLength1 %d, DescriptionLength %p,"
          " DriverAttributes %p, BufferLength2 %d, AttributesLength %p)\n", EnvironmentHandle, Direction,
          DriverDescription, BufferLength1, DescriptionLength, DriverAttributes, BufferLength2, AttributesLength);

    if (!handle) return SQL_INVALID_HANDLE;

    params.EnvironmentHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLDriversW, &params );

    if (ret == SQL_NO_DATA && Direction == SQL_FETCH_FIRST)
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
    struct SQLSetDescFieldW_params params = { 0, RecNumber, FieldIdentifier, Value, BufferLength };
    struct handle *handle = DescriptorHandle;
    SQLRETURN ret;

    TRACE("(DescriptorHandle %p, RecNumber %d, FieldIdentifier %d, Value %p, BufferLength %d)\n", DescriptorHandle,
          RecNumber, FieldIdentifier, Value, BufferLength);

    if (!handle) return SQL_INVALID_HANDLE;

    params.DescriptorHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLSetDescFieldW, &params );
    TRACE("Returning %d\n", ret);
    return ret;
}

/*************************************************************************
 *				SQLSetStmtAttrW          [ODBC32.176]
 */
SQLRETURN WINAPI SQLSetStmtAttrW(SQLHSTMT StatementHandle, SQLINTEGER Attribute, SQLPOINTER Value,
                                 SQLINTEGER StringLength)
{
    struct SQLSetStmtAttrW_params params = { 0, Attribute, Value, StringLength };
    struct handle *handle = StatementHandle;
    SQLRETURN ret;

    TRACE("(StatementHandle %p, Attribute %d, Value %p, StringLength %d)\n", StatementHandle, Attribute,
          Value, StringLength);

    if (!handle) return SQL_INVALID_HANDLE;

    params.StatementHandle = handle->unix_handle;
    ret = ODBC_CALL( SQLSetStmtAttrW, &params );
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
                                SQLCHAR *SqlState, SQLINTEGER *NativeError, SQLCHAR *MessageText,
                                SQLSMALLINT BufferLength, SQLSMALLINT *TextLength)
{
    return SQLGetDiagRec( HandleType, Handle, RecNumber, SqlState, NativeError, MessageText, BufferLength,
                          TextLength );
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
        break;

    case DLL_PROCESS_DETACH:
        if (reserved) break;
    }

    return TRUE;
}
