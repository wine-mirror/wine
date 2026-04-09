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
