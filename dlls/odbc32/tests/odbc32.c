/*
 * Copyright 2024 Hans Leidekker for CodeWeavers
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "sql.h"
#include "sqlext.h"

#include <wine/test.h>

static void test_SQLAllocHandle( void )
{
    SQLHANDLE handle;
    SQLHENV env;
    SQLHDBC con;
    SQLRETURN ret;

    handle = (void *)0xdeadbeef;
    ret = SQLAllocHandle( SQL_HANDLE_ENV, SQL_NULL_HANDLE, &handle );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );
    ok( handle != (void *)0xdeadbeef, "handle not set\n" );
    ret = SQLFreeHandle( SQL_HANDLE_ENV, handle );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );
    ret = SQLFreeHandle( SQL_HANDLE_ENV, 0 );
    ok( ret == SQL_INVALID_HANDLE, "got %d\n", ret );

    env = (void *)0xdeadbeef;
    ret = SQLAllocEnv( &env );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );
    ok( env != (void *)0xdeadbeef, "env not set\n" );

    con = (void *)0xdeadbeef;
    ret = SQLAllocConnect( env, &con );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );
    ok( con != (void *)0xdeadbeef, "con not set\n" );

    ret = SQLFreeConnect( con );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );
    ret = SQLFreeConnect( 0 );
    ok( ret == SQL_INVALID_HANDLE, "got %d\n", ret );

    ret = SQLFreeEnv( env );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );
    ret = SQLFreeEnv( 0 );
    ok( ret == SQL_INVALID_HANDLE, "got %d\n", ret );
}

static void diag( SQLHANDLE handle, SQLSMALLINT type )
{
    SQLINTEGER err;
    SQLSMALLINT len;
    SQLCHAR state[6], msg[256];
    SQLRETURN ret;

    state[0] = 0;
    msg[0] = 0;
    err = -1;
    len = 0;
    ret = SQLGetDiagRec( type, handle, 1, state, &err, msg, sizeof(msg), &len );
    if (ret == SQL_SUCCESS) trace( "state '%s' err %d msg '%s' len %d\n", state, err, msg, len );
}

static void test_SQLGetDiagRec( void )
{
    SQLHANDLE handle;
    SQLINTEGER err;
    SQLSMALLINT len;
    SQLCHAR state[6], msg[256];
    SQLRETURN ret;

    handle = (void *)0xdeadbeef;
    ret = SQLAllocHandle( SQL_HANDLE_ENV, SQL_NULL_HANDLE, &handle );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );
    ok( handle != (void *)0xdeadbeef, "handle not set\n" );

    state[0] = 0;
    msg[0] = 0;
    err = -1;
    len = 0;
    ret = SQLGetDiagRec( SQL_HANDLE_ENV, handle, 1, state, &err, msg, sizeof(msg), &len );
    ok( ret == SQL_NO_DATA, "got %d\n", ret );

    ret = SQLFreeHandle( SQL_HANDLE_ENV, handle );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );
}

static void test_SQLConnect( void )
{
    SQLHENV env;
    SQLHDBC con;
    SQLRETURN ret;
    SQLINTEGER size, version, pooling;
    SQLUINTEGER timeout;
    SQLSMALLINT len;
    char str[32];

    ret = SQLAllocEnv( &env );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );

    version = -1;
    size = -1;
    ret = SQLGetEnvAttr( env, SQL_ATTR_ODBC_VERSION, &version, sizeof(version), &size );
    if (ret == SQL_ERROR) diag( env, SQL_HANDLE_ENV );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );
    ok( version != -1, "version not set\n" );
    ok( size == -1, "size set\n" );
    trace( "ODBC version %d\n", version );

    pooling = -1;
    ret = SQLGetEnvAttr( env, SQL_ATTR_CONNECTION_POOLING, &pooling, sizeof(pooling), NULL );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );
    ok( !pooling, "got %d\n", pooling );

    ret = SQLAllocConnect( env, &con );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );

    /* env handle can't be freed before connect handle */
    ret = SQLFreeEnv( env );
    ok( ret == SQL_ERROR, "got %d\n", ret );

    ret = SQLGetEnvAttr( env, SQL_ATTR_ODBC_VERSION, &version, sizeof(version), &size );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );

    len = -1;
    ret = SQLGetInfo( con, SQL_ODBC_VER, NULL, 0, &len );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );
    ok( len != -1, "len not set\n" );

    memset( str, 0, sizeof(str) );
    ret = SQLGetInfo( con, SQL_ODBC_VER, str, sizeof(str), &len );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );
    ok( str[0], "empty string\n" );
    ok( len == strlen(str), "got %d\n", len );
    trace( "version %s\n", str );

    timeout = 0xdeadbeef;
    ret = SQLGetConnectAttr( con, SQL_ATTR_LOGIN_TIMEOUT, &timeout, sizeof(timeout), NULL );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );
    ok( timeout == 15, "wrong timeout %d\n", timeout );

    ret = SQLConnect( con, (SQLCHAR *)"winetest", SQL_NTS, (SQLCHAR *)"winetest", SQL_NTS, (SQLCHAR *)"winetest",
                      SQL_NTS );
    if (ret == SQL_ERROR) diag( con, SQL_HANDLE_DBC );
    if (ret != SQL_SUCCESS)
    {
        SQLFreeConnect( con );
        SQLFreeEnv( env );
        skip( "data source winetest not available\n" );
        return;
    }

    timeout = 0xdeadbeef;
    ret = SQLGetConnectAttr( con, SQL_ATTR_LOGIN_TIMEOUT, &timeout, sizeof(timeout), NULL );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );
    todo_wine ok( !timeout, "wrong timeout %d\n", timeout );

    timeout = 0xdeadbeef;
    size = -1;
    ret = SQLGetConnectAttr( con, SQL_ATTR_CONNECTION_TIMEOUT, &timeout, sizeof(timeout), &size );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );
    ok( timeout != 0xdeadbeef, "timeout not set\n" );
    ok( size == -1, "size set\n" );

    ret = SQLTransact( NULL, NULL, SQL_COMMIT );
    ok( ret == SQL_INVALID_HANDLE, "got %d\n", ret );

    ret = SQLTransact( env, NULL, SQL_COMMIT );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );

    ret = SQLTransact( NULL, con, SQL_COMMIT );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );

    ret = SQLTransact( env, con, SQL_COMMIT );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );

    ret = SQLDisconnect( con );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );

    ret = SQLFreeConnect( con );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );

    ret = SQLFreeEnv( env );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );
}

static void test_SQLDriverConnect( void )
{
    SQLHENV env;
    SQLHDBC con;
    SQLRETURN ret;
    SQLSMALLINT len;
    SQLCHAR str[256];

    ret = SQLAllocEnv( &env );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );

    ret = SQLAllocConnect( env, &con );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );

    len = 0;
    str[0] = 0;
    ret = SQLDriverConnect( con, NULL, (SQLCHAR *)"dsn=winetest;UID=winetest", strlen("dsn=winetest;UID=winetest"),
                            str, sizeof(str), &len, 0 );
    if (ret == SQL_ERROR) diag( con, SQL_HANDLE_DBC );
    if (ret != SQL_SUCCESS)
    {
        SQLFreeConnect( con );
        SQLFreeEnv( env );
        skip( "data source winetest not available\n" );
        return;
    }
    ok( ret == SQL_SUCCESS, "got %d\n", ret );
    todo_wine {
    ok( !strcmp( (const char *)str, "DSN=winetest;UID=winetest;" ), "got '%s'\n", str );
    ok( len == 26, "got %d\n", len );
    }

    ret = SQLDisconnect( con );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );

    /* trailing garbage */
    len = 0;
    str[0] = 0;
    ret = SQLDriverConnect( con, NULL, (SQLCHAR *)"DSN=winetest;er\\9.99", strlen("DSN=winetest;er\\9.99"),
                            str, sizeof(str), &len, 0 );
    if (ret == SQL_ERROR) diag( con, SQL_HANDLE_DBC );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );
    todo_wine {
    ok( !strcmp( (const char *)str, "DSN=winetest;" ), "got '%s'\n", str );
    ok( len == 13, "got %d\n", len );
    }

    ret = SQLDisconnect( con );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );

    ret = SQLFreeConnect( con );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );

    ret = SQLFreeEnv( env );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );
}

static void test_SQLBrowseConnect( void )
{
    SQLHENV env;
    SQLHDBC con;
    SQLRETURN ret;
    SQLSMALLINT len;
    SQLCHAR str[256];

    ret = SQLAllocEnv( &env );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );

    ret = SQLAllocConnect( env, &con );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );

    len = 0;
    str[0] = 0;
    ret = SQLBrowseConnect( con, (SQLCHAR *)"DSN=winetest", 12, str, sizeof(str), &len );
    if (ret == SQL_ERROR) diag( con, SQL_HANDLE_DBC );
    if (ret != SQL_SUCCESS)
    {
        SQLFreeConnect( con );
        SQLFreeEnv( env );
        skip( "data source winetest not available\n" );
        return;
    }
    ok( ret == SQL_SUCCESS, "got %d\n", ret );
    ok( !strcmp( (const char *)str, "DSN=winetest" ), "got '%s'\n", str );
    ok( len == 12, "got %d\n", len );

    ret = SQLDisconnect( con );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );

    ret = SQLFreeConnect( con );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );

    ret = SQLFreeEnv( env );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );
}

static void test_SQLDataSources( void )
{
    SQLHENV env;
    SQLRETURN ret;
    SQLSMALLINT len, len2;
    SQLCHAR server[256], desc[256];

    ret = SQLAllocEnv( &env );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );

    len = len2 = -1;
    memset( server, 0, sizeof(server) );
    memset( desc, 0, sizeof(desc) );
    ret = SQLDataSources( env, SQL_FETCH_FIRST, server, sizeof(server), &len, desc, sizeof(desc), &len2 );
    ok( ret == SQL_SUCCESS || ret == SQL_NO_DATA, "got %d\n", ret );
    while (ret == SQL_SUCCESS)
    {
        ok( len == strlen((const char *)server), "unexpected len\n" );
        ok( len2 == strlen((const char *)desc), "unexpected len\n" );
        ok( server[0], "empty string\n" );
        ok( desc[0], "empty string\n" );
        trace( "server %s len %d desc %s len %d\n", server, len, desc, len2 );

        len = len2 = -1;
        memset( server, 0, sizeof(server) );
        memset( desc, 0, sizeof(desc) );
        ret = SQLDataSources( env, SQL_FETCH_NEXT, server, sizeof(server), &len, desc, sizeof(desc), &len2 );
        ok( ret == SQL_SUCCESS || ret == SQL_NO_DATA, "got %d\n", ret );
    }

    ret = SQLFreeEnv( env );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );
}

static void test_SQLDrivers( void )
{
    SQLHENV env;
    SQLRETURN ret;
    SQLSMALLINT len, len2;
    SQLCHAR desc[256], attrs[256];

    ret = SQLAllocEnv( &env );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );

    len = len2 = 0;
    memset( desc, 0, sizeof(desc) );
    memset( attrs, 0, sizeof(attrs) );
    ret = SQLDrivers( env, SQL_FETCH_FIRST, desc, sizeof(desc), &len, attrs, sizeof(attrs), &len2 );
    ok( ret == SQL_SUCCESS || ret == SQL_NO_DATA, "got %d\n", ret );
    while (ret == SQL_SUCCESS)
    {
        SQLCHAR *ptr;

        trace( "desc %s len %d len2 %d\n", desc, len, len2 );
        ok( len == strlen((const char *)desc), "unexpected len %u\n", len );
        ok( desc[0], "empty string\n" );
        ptr = attrs;
        while (*ptr)
        {
            trace( " attr %s\n", ptr );
            ptr += strlen( (const char *)ptr ) + 1;
        }

        len = len2 = 0;
        memset( desc, 0, sizeof(desc) );
        memset( attrs, 0, sizeof(attrs) );
        ret = SQLDrivers( env, SQL_FETCH_NEXT, desc, sizeof(desc), &len, attrs, sizeof(attrs), &len2 );
        ok( ret == SQL_SUCCESS || ret == SQL_NO_DATA, "got %d\n", ret );
    }

    ret = SQLFreeEnv( env );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );
}

static void test_SQLExecDirect( void )
{
    SQLHENV env;
    SQLHDBC con;
    SQLHSTMT stmt;
    SQLHDESC desc;
    SQLRETURN ret;
    SQLLEN count, len_id[2], len_name[2], len_octet;
    SQLULEN rows_fetched;
    SQLINTEGER id[2], err, size;
    SQLCHAR name[32], msg[256], state[6];
    SQLSMALLINT len;

    ret = SQLAllocEnv( &env );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );

    ret = SQLAllocConnect( env, &con );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );

    ret = SQLConnect( con, (SQLCHAR *)"winetest", 8, (SQLCHAR *)"winetest", 8, (SQLCHAR *)"winetest", 8 );
    if (ret == SQL_ERROR) diag( con, SQL_HANDLE_DBC );
    if (ret != SQL_SUCCESS)
    {
        SQLFreeConnect( con );
        SQLFreeEnv( env );
        skip( "data source winetest not available\n" );
        return;
    }
    ok( ret == SQL_SUCCESS, "got %d\n", ret );

    ret = SQLAllocStmt( con, &stmt );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );

    ret = SQLError( NULL, NULL, NULL, state, &err, msg, sizeof(msg), &len );
    ok( ret == SQL_INVALID_HANDLE, "got %d\n", ret );

    ret = SQLError( env, NULL, NULL, state, &err, msg, sizeof(msg), &len );
    ok( ret == SQL_NO_DATA, "got %d\n", ret );

    ret = SQLError( env, con, NULL, state, &err, msg, sizeof(msg), &len );
    ok( ret == SQL_NO_DATA, "got %d\n", ret );

    ret = SQLError( env, con, stmt, state, &err, msg, sizeof(msg), &len );
    ok( ret == SQL_NO_DATA, "got %d\n", ret );

    SQLExecDirect( stmt, (SQLCHAR *)"USE winetest", ARRAYSIZE("USE winetest") - 1 );
    SQLExecDirect( stmt, (SQLCHAR *)"DROP TABLE winetest", ARRAYSIZE("DROP TABLE winetest") - 1 );
    ret = SQLExecDirect( stmt, (SQLCHAR *)"CREATE TABLE winetest ( Id int, Name varchar(255) )",
                         ARRAYSIZE("CREATE TABLE winetest ( Id int, Name varchar(255) )") - 1 );
    if (ret == SQL_ERROR) diag( stmt, SQL_HANDLE_STMT );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );
    if (ret == SQL_ERROR) diag( stmt, SQL_HANDLE_STMT );

    ret = SQLExecDirect( stmt, (SQLCHAR *)"INSERT INTO winetest VALUES (0, 'John')",
                         ARRAYSIZE("INSERT INTO winetest VALUES (0, 'John')") - 1 );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );
    if (ret == SQL_ERROR) diag( stmt, SQL_HANDLE_STMT );

    ret = SQLExecDirect( stmt, (SQLCHAR *)"INSERT INTO winetest VALUES (1, 'Mary')",
                         ARRAYSIZE("INSERT INTO winetest VALUES (1, 'Mary')") - 1 );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );
    if (ret == SQL_ERROR) diag( stmt, SQL_HANDLE_STMT );

    ret = SQLExecDirect( stmt, (SQLCHAR *)"SELECT * FROM winetest", ARRAYSIZE("SELECT * FROM winetest") - 1 );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );
    if (ret == SQL_ERROR) diag( stmt, SQL_HANDLE_STMT );

    count = 0xdeadbeef;
    ret = SQLRowCount( stmt, &count );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );
    ok( count == 2, "got %d\n", (int)count );

    ret = SQLFetch( stmt );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );

    id[0] = -1;
    len_id[0] = 0;
    ret = SQLGetData( stmt, 1, SQL_C_SLONG, id, sizeof(id[0]), len_id );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );
    ok( !id[0], "id not set\n" );
    ok( len_id[0] == sizeof(id[0]), "got %d\n", (int)len_id[0] );

    ret = SQLFreeStmt( stmt, SQL_DROP );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );

    ret = SQLAllocStmt( con, &stmt );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );

    ret = SQLExecDirect( stmt, (SQLCHAR *)"SELECT * FROM winetest", ARRAYSIZE("SELECT * FROM winetest") - 1 );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );
    if (ret == SQL_ERROR) diag( stmt, SQL_HANDLE_STMT );

    ret = SQLSetStmtAttr( stmt, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER)2,  0 );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );
    if (ret == SQL_ERROR) diag( stmt, SQL_HANDLE_STMT );

    rows_fetched = 0xdeadbeef;
    ret = SQLSetStmtAttr( stmt, SQL_ATTR_ROWS_FETCHED_PTR, (SQLPOINTER)&rows_fetched,  0 );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );
    if (ret == SQL_ERROR) diag( stmt, SQL_HANDLE_STMT );

    id[0] = -1;
    id[1] = -1;
    len_id[0] = 0;
    len_id[1] = 0;
    ret = SQLBindCol( stmt, 1, SQL_C_SLONG, id, sizeof(id), len_id );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );
    if (ret == SQL_ERROR) diag( stmt, SQL_HANDLE_STMT );

    memset( name, 0, sizeof(name) );
    len_name[0] = 0;
    len_name[1] = 0;
    ret = SQLBindCol( stmt, 2, SQL_C_CHAR, name, sizeof(name), len_name );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );
    if (ret == SQL_ERROR) diag( stmt, SQL_HANDLE_STMT );

    ret = SQLFetch( stmt );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );
    ok( rows_fetched == 2, "got %d\n", (int)rows_fetched );
    ok( id[0] == 0, "got %d\n", id[0] );
    ok( id[1] == 1, "got %d\n", id[1] );
    ok( len_id[0] == sizeof(id[0]), "got %d\n", (int)len_id[0] );
    ok( len_id[1] == sizeof(id[1]), "got %d\n", (int)len_id[1] );
    ok( !strcmp( (const char *)name, "John" ), "got %s\n", name );
    ok( len_name[0] == sizeof("John") - 1, "got %d\n", (int)len_name[0] );
    ok( len_name[1] == sizeof("Mary") - 1, "got %d\n", (int)len_name[1] );

    ret = SQLFreeStmt( stmt, SQL_DROP );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );

    ret = SQLAllocStmt( con, &stmt );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );

    ret = SQLPrepare( stmt, (SQLCHAR *)"SELECT * FROM winetest WHERE Id = ? AND Name = ?",
                      ARRAYSIZE("SELECT * FROM winetest WHERE Id = ? AND Name = ?") - 1 );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );
    if (ret == SQL_ERROR) diag( stmt, SQL_HANDLE_STMT );

    id[0] = 1;
    len_id[0] = sizeof(id[0]);
    ret = SQLBindParameter( stmt, 1, SQL_PARAM_INPUT, SQL_INTEGER, SQL_INTEGER, 0, 0, id, 0, len_id );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );
    if (ret == SQL_ERROR) diag( stmt, SQL_HANDLE_STMT );

    memcpy( name, "Mary", sizeof("Mary") );
    len_name[0] = sizeof( "Mary" ) - 1;
    ret = SQLBindParameter( stmt, 2, SQL_PARAM_INPUT, SQL_CHAR, SQL_VARCHAR, 0, 0, name, 0, len_name );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );
    if (ret == SQL_ERROR) diag( stmt, SQL_HANDLE_STMT );

    ret = SQLExecute( stmt );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );
    if (ret == SQL_ERROR) diag( stmt, SQL_HANDLE_STMT );

    ret = SQLFetch( stmt );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );
    if (ret == SQL_ERROR) diag( stmt, SQL_HANDLE_STMT );
    ok( id[0] == 1, "got %d\n", id[0] );
    ok( len_id[0] == sizeof(id[0]), "got %d\n", (int)len_id[0] );
    ok( !strcmp( (const char *)name, "Mary" ), "got %s\n", name );
    ok( len_name[0] == sizeof("Mary") - 1, "got %d\n", (int)len_name[0] );
    ret = SQLFreeStmt( stmt, SQL_UNBIND );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );
    ret = SQLFreeStmt( stmt, SQL_DROP );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );

    ret = SQLAllocStmt( con, &stmt );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );

    ret = SQLExecDirect( stmt, (SQLCHAR *)"DROP TABLE winetest", ARRAYSIZE("DROP TABLE winetest") - 1 );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );

    desc = (SQLHDESC)0xdeadbeef;
    size = 0xdeadbeef;
    ret = SQLGetStmtAttr( stmt, SQL_ATTR_APP_ROW_DESC, &desc, sizeof(desc), &size );
    if (ret == SQL_ERROR) diag( stmt, SQL_HANDLE_STMT );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );
    ok( desc != (SQLHDESC)0xdeadbeef, "desc not set\n" );
    ok( size == 0xdeadbeef, "got %d\n", size );

    ret = SQLSetDescField( desc, 1, SQL_DESC_OCTET_LENGTH_PTR, &len_octet, 0 );
    if (ret == SQL_ERROR) diag( stmt, SQL_HANDLE_DESC );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );

    ret = SQLSetStmtAttr( stmt, SQL_ATTR_APP_ROW_DESC, NULL, sizeof(desc) );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );

    ret = SQLSetStmtAttr( stmt, SQL_ATTR_APP_ROW_DESC, desc, 0 );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );

    ret = SQLSetStmtAttr( stmt, SQL_ATTR_IMP_ROW_DESC, NULL, sizeof(desc) );
    ok( ret == SQL_ERROR, "got %d\n", ret );

    ret = SQLFreeStmt( stmt, SQL_DROP );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );

    ret = SQLDisconnect( con );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );

    ret = SQLFreeConnect( con );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );

    ret = SQLFreeEnv( env );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );
}

static void test_SQLSetEnvAttr(void)
{
    SQLINTEGER version;
    SQLHENV env;
    SQLRETURN ret;

    ret = SQLAllocEnv( &env );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );

    ret = SQLSetEnvAttr (env, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC2, 0);
    ok( ret == SQL_SUCCESS, "got %d\n", ret );

    ret = SQLSetEnvAttr (env, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
    ok( ret == SQL_SUCCESS, "got %d\n", ret );

    version = -1;
    ret = SQLGetEnvAttr( env, SQL_ATTR_ODBC_VERSION, &version, sizeof(version), NULL );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );
    ok( version == SQL_OV_ODBC3, "wrong version %d\n", version );

    ret = SQLFreeEnv( env );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );
}

static void test_SQLSetConnectAttr(void)
{
    SQLUINTEGER timeout;
    SQLHENV env;
    SQLHDBC con;
    SQLRETURN ret;

    ret = SQLAllocEnv( &env );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );

    ret = SQLAllocConnect( env, &con );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );

    timeout = 10;
    ret = SQLSetConnectAttr( con, SQL_ATTR_LOGIN_TIMEOUT, (SQLPOINTER)(ULONG_PTR)timeout, sizeof(timeout) );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );

    timeout = 0;
    ret = SQLGetConnectAttr( con, SQL_ATTR_LOGIN_TIMEOUT, &timeout, sizeof(timeout), NULL );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );
    ok( timeout == 10, "wrong timeout %d\n", timeout );

    ret = SQLFreeConnect( con );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );

    ret = SQLFreeEnv( env );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );
}

START_TEST(odbc32)
{
    test_SQLAllocHandle();
    test_SQLGetDiagRec();
    test_SQLConnect();
    test_SQLDriverConnect();
    test_SQLBrowseConnect();
    test_SQLDataSources();
    test_SQLDrivers();
    test_SQLExecDirect();
    test_SQLSetEnvAttr();
    test_SQLSetConnectAttr();
}
