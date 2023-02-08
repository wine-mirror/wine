/*
 * MS SQL Unicode Definitions
 *
 * Copyright (C) 2017 Daniel Lehman
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

#ifndef __SQLUCODE_H
#define __SQLUCODE_H

#include <sqlext.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SQL_WCHAR             (-8)
#define SQL_C_WCHAR           SQL_WCHAR
#define SQL_WVARCHAR          (-9)
#define SQL_WLONGVARCHAR      (-10)

SQLRETURN WINAPI SQLColAttributesW(SQLHSTMT hstmt, SQLUSMALLINT icol, SQLUSMALLINT fDescType,
            SQLPOINTER rgbDesc, SQLSMALLINT cbDescMax, SQLSMALLINT *pcbDesc, SQLLEN *pfDesc);

SQLRETURN WINAPI SQLConnectW(SQLHDBC ConnectionHandle,
           SQLWCHAR *ServerName, SQLSMALLINT NameLength1,
           SQLWCHAR *UserName, SQLSMALLINT NameLength2,
           SQLWCHAR *Authentication, SQLSMALLINT NameLength3);

SQLRETURN WINAPI SQLDescribeColW(SQLHSTMT StatementHandle,
           SQLUSMALLINT ColumnNumber, SQLWCHAR *ColumnName,
           SQLSMALLINT BufferLength, SQLSMALLINT *NameLength,
           SQLSMALLINT *DataType, SQLULEN *ColumnSize,
           SQLSMALLINT *DecimalDigits, SQLSMALLINT *Nullable);

SQLRETURN WINAPI SQLDriverConnectW(SQLHDBC ConnectionHandle,
           SQLHWND WindowHandle, WCHAR *InConnectionString,
           SQLSMALLINT Length, WCHAR *OutConnectionString, SQLSMALLINT BufferLength,
           SQLSMALLINT *Length2, SQLUSMALLINT DriverCompletion);

SQLRETURN WINAPI SQLExecDirectW(SQLHSTMT StatementHandle,
           SQLWCHAR *StatementText, SQLINTEGER TextLength);

SQLRETURN WINAPI SQLGetDiagRecW(SQLSMALLINT HandleType, SQLHANDLE Handle,
           SQLSMALLINT RecNumber, SQLWCHAR *Sqlstate,
           SQLINTEGER *NativeError, SQLWCHAR *MessageText,
           SQLSMALLINT BufferLength, SQLSMALLINT *TextLength);

SQLRETURN WINAPI SQLGetInfoW(SQLHDBC ConnectionHandle,
           SQLUSMALLINT InfoType, SQLPOINTER InfoValue,
           SQLSMALLINT BufferLength, SQLSMALLINT *StringLength);

SQLRETURN WINAPI SQLPrepareW(SQLHSTMT StatementHandle,
           SQLWCHAR *StatementText, SQLINTEGER TextLength);

SQLRETURN WINAPI SQLSetStmtAttrW(SQLHSTMT StatementHandle,
           SQLINTEGER Attribute, SQLPOINTER Value,
           SQLINTEGER StringLength);

SQLRETURN WINAPI SQLGetDiagFieldW(SQLSMALLINT type, SQLHANDLE handle,
           SQLSMALLINT record, SQLSMALLINT field, SQLPOINTER info,
           SQLSMALLINT info_len, SQLSMALLINT *ret_len);

#ifdef __cplusplus
}
#endif

#endif
