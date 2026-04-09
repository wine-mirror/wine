/*
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

struct driver_funcs
{
    SQLRETURN (WINAPI *SQLAllocHandle)(SQLSMALLINT, SQLHANDLE, SQLHANDLE*);
    SQLRETURN (WINAPI *SQLFreeHandle)(SQLSMALLINT, SQLHANDLE);
    SQLRETURN (WINAPI *SQLGetEnvAttr)(SQLHENV, SQLINTEGER, SQLPOINTER, SQLINTEGER, SQLINTEGER*);
    SQLRETURN (WINAPI *SQLSetEnvAttr)(SQLHENV, SQLINTEGER, SQLPOINTER, SQLINTEGER);
    SQLRETURN (WINAPI *SQLGetConnectAttr)(SQLHDBC, SQLINTEGER, SQLPOINTER,
            SQLINTEGER, SQLINTEGER*);
    SQLRETURN (WINAPI *SQLSetConnectAttr)(SQLHDBC, SQLINTEGER, SQLPOINTER, SQLINTEGER);
    SQLRETURN (WINAPI *SQLGetInfo)(SQLHDBC, SQLUSMALLINT, SQLPOINTER, SQLSMALLINT, SQLSMALLINT*);
    SQLRETURN (WINAPI *SQLConnect)(SQLHDBC, SQLCHAR*, SQLSMALLINT,
            SQLCHAR*, SQLSMALLINT, SQLCHAR*, SQLSMALLINT);
    SQLRETURN (WINAPI *SQLDriverConnect)(SQLHDBC, SQLHWND, SQLCHAR*, SQLSMALLINT,
            SQLCHAR*, SQLSMALLINT, SQLSMALLINT*, SQLUSMALLINT);
    SQLRETURN (WINAPI *SQLBrowseConnect)(SQLHDBC, SQLCHAR*, SQLSMALLINT,
            SQLCHAR*, SQLSMALLINT, SQLSMALLINT*);
    SQLRETURN (WINAPI *SQLDisconnect)(SQLHDBC);
};
