/*
 * ODBC test driver
 *
 * Copyright 2026 Piotr Caban
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
#pragma makedep testdll
#endif

#include "windows.h"
#include "sql.h"

#include "driver.h"

static struct driver_funcs *driver_funcs;

void WINAPI init_funcs( struct driver_funcs *funcs )
{
    driver_funcs = funcs;
}

SQLRETURN WINAPI SQLAllocHandle( SQLSMALLINT type, SQLHANDLE input_handle, SQLHANDLE *out )
{
    return driver_funcs->SQLAllocHandle( type, input_handle, out );
}

SQLRETURN WINAPI SQLFreeHandle( SQLSMALLINT type, SQLHANDLE handle )
{
    return driver_funcs->SQLFreeHandle( type, handle );
}

SQLRETURN WINAPI SQLGetEnvAttr( SQLHENV env, SQLINTEGER attr, SQLPOINTER val,
        SQLINTEGER len, SQLINTEGER *out_len )
{
    return driver_funcs->SQLGetEnvAttr( env, attr, val, len, out_len );
}

SQLRETURN WINAPI SQLSetEnvAttr( SQLHENV env, SQLINTEGER attr, SQLPOINTER val, SQLINTEGER len )
{
    return driver_funcs->SQLSetEnvAttr( env, attr, val, len );
}

SQLRETURN WINAPI SQLGetConnectAttr( SQLHDBC con, SQLINTEGER attr, SQLPOINTER val,
        SQLINTEGER len, SQLINTEGER *out_len )
{
    return driver_funcs->SQLGetConnectAttr( con, attr, val, len, out_len );
}

SQLRETURN WINAPI SQLSetConnectAttr( SQLHDBC con, SQLINTEGER attr, SQLPOINTER val,
        SQLINTEGER len )
{
    return driver_funcs->SQLSetConnectAttr( con, attr, val, len );
}

SQLRETURN WINAPI SQLGetInfo( SQLHDBC con, SQLUSMALLINT type, SQLPOINTER info,
        SQLSMALLINT len, SQLSMALLINT *out_len )
{
    return driver_funcs->SQLGetInfo( con, type, info, len, out_len );
}

SQLRETURN WINAPI SQLConnect( SQLHDBC con, SQLCHAR *server, SQLSMALLINT server_len,
        SQLCHAR *user, SQLSMALLINT user_len, SQLCHAR *auth, SQLSMALLINT auth_len )
{
    return driver_funcs->SQLConnect( con, server, server_len, user, user_len, auth, auth_len );
}

SQLRETURN WINAPI SQLDriverConnect( SQLHDBC con, SQLHWND win, SQLCHAR *in_con,
        SQLSMALLINT in_con_len, SQLCHAR *out_con, SQLSMALLINT out_con_max_len,
        SQLSMALLINT *out_con_len, SQLUSMALLINT completion )
{
    return driver_funcs->SQLDriverConnect( con, win, in_con, in_con_len,
            out_con, out_con_max_len, out_con_len, completion );
}

SQLRETURN WINAPI SQLBrowseConnect( SQLHDBC con, SQLCHAR *in_con, SQLSMALLINT in_con_len,
        SQLCHAR *out_con, SQLSMALLINT out_con_max_len, SQLSMALLINT *out_con_len )
{
    return driver_funcs->SQLBrowseConnect( con, in_con, in_con_len,
            out_con, out_con_max_len, out_con_len );
}

SQLRETURN WINAPI SQLDisconnect( SQLHDBC con )
{
    return driver_funcs->SQLDisconnect( con );
}

SQLRETURN WINAPI SQLGetStmtAttr( SQLHSTMT stmt, SQLINTEGER attr,
        SQLPOINTER val, SQLINTEGER max_len, SQLINTEGER *len )
{
    return driver_funcs->SQLGetStmtAttr( stmt, attr, val, max_len, len );
}

SQLRETURN WINAPI SQLSetStmtAttr( SQLHSTMT stmt, SQLINTEGER attr, SQLPOINTER val, SQLINTEGER len )
{
    return driver_funcs->SQLSetStmtAttr( stmt, attr, val, len );
}

SQLRETURN WINAPI SQLGetDiagField( SQLSMALLINT type, SQLHANDLE handle, SQLSMALLINT rec,
        SQLSMALLINT identifier, SQLPOINTER info, SQLSMALLINT max_len, SQLSMALLINT *len )
{
    return driver_funcs->SQLGetDiagField( type, handle, rec, identifier, info, max_len, len );
}

SQLRETURN WINAPI SQLGetDiagRec( SQLSMALLINT type, SQLHANDLE handle,
        SQLSMALLINT rec, SQLCHAR *state, SQLINTEGER *err, SQLCHAR *msg,
        SQLSMALLINT max_len, SQLSMALLINT *len )
{
    return driver_funcs->SQLGetDiagRec( type, handle, rec, state, err, msg, max_len, len );
}

SQLRETURN WINAPI SQLExecDirect( SQLHSTMT stmt, SQLCHAR *cmd, SQLINTEGER len )
{
    return driver_funcs->SQLExecDirect( stmt, cmd, len );
}

SQLRETURN WINAPI SQLRowCount( SQLHSTMT stmt, SQLLEN *count )
{
    return driver_funcs->SQLRowCount( stmt, count );
}

SQLRETURN WINAPI SQLFetch( SQLHSTMT stmt )
{
    return driver_funcs->SQLFetch( stmt );
}

SQLRETURN WINAPI SQLGetData( SQLHSTMT stmt, SQLUSMALLINT col, SQLSMALLINT type,
        SQLPOINTER val, SQLLEN max_len, SQLLEN *len )
{
    return driver_funcs->SQLGetData( stmt, col, type, val, max_len, len );
}

SQLRETURN WINAPI SQLBindCol( SQLHSTMT stmt, SQLUSMALLINT col, SQLSMALLINT type,
        SQLPOINTER val, SQLLEN max_len, SQLLEN *len )
{
    return driver_funcs->SQLBindCol( stmt, col, type, val, max_len, len );
}

SQLRETURN WINAPI SQLPrepare( SQLHSTMT stmt, SQLCHAR *cmd, SQLINTEGER len )
{
    return driver_funcs->SQLPrepare( stmt, cmd, len );
}

SQLRETURN WINAPI SQLBindParameter( SQLHSTMT stmt, SQLUSMALLINT param,
        SQLSMALLINT param_type, SQLSMALLINT ctype, SQLSMALLINT type, SQLULEN size,
        SQLSMALLINT decimal_digits, SQLPOINTER val, SQLLEN max_len, SQLLEN *len )
{
    return driver_funcs->SQLBindParameter( stmt, param, param_type, ctype,
            type, size, decimal_digits, val, max_len, len );
}

SQLRETURN WINAPI SQLExecute( SQLHSTMT stmt )
{
    return driver_funcs->SQLExecute( stmt );
}

SQLRETURN WINAPI SQLSetDescField( SQLHDESC desc, SQLSMALLINT rec,
        SQLSMALLINT field, SQLPOINTER val, SQLINTEGER len )
{
    return driver_funcs->SQLSetDescField( desc, rec, field, val, len );
}

SQLRETURN WINAPI SQLFreeStmt( SQLHSTMT stmt, SQLUSMALLINT option )
{
    return driver_funcs->SQLFreeStmt( stmt, option );
}

SQLRETURN WINAPI SQLDescribeCol( SQLHSTMT stmt, SQLUSMALLINT col,
        SQLCHAR *name, SQLSMALLINT max_len, SQLSMALLINT *len, SQLSMALLINT *type,
        SQLULEN *size, SQLSMALLINT *dec_digits, SQLSMALLINT *nullable )
{
    return driver_funcs->SQLDescribeCol( stmt, col, name, max_len, len,
            type, size, dec_digits, nullable );
}
