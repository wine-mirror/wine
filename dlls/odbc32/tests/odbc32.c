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
#include "winreg.h"
#include "odbcinst.h"
#include "sql.h"
#include "sqlext.h"

#include "driver.h"

#include <wine/test.h>

#define DEFINE_EXPECT(func) \
    static BOOL expect_ ## func = FALSE, called_ ## func = FALSE

#define SET_EXPECT(func) \
    do { \
        called_ ## func = FALSE; \
        expect_ ## func = TRUE; \
    } while(0)

#define CHECK_EXPECT2(func) \
    do { \
        ok(expect_ ##func, "unexpected call " #func "\n"); \
        called_ ## func = TRUE; \
    }while(0)

#define CHECK_EXPECT(func) \
    do { \
        CHECK_EXPECT2(func); \
        expect_ ## func = FALSE; \
    }while(0)

#define CHECK_CALLED(func) \
    do { \
        ok(called_ ## func, "expected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

#define CHECK_NOT_CALLED(func) \
    do { \
        ok(!called_ ## func, "unexpected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

DEFINE_EXPECT( driver_SQLAllocHandle_env );
DEFINE_EXPECT( driver_SQLAllocHandle_con );
DEFINE_EXPECT( driver_SQLFreeHandle_env );
DEFINE_EXPECT( driver_SQLFreeHandle_con );
DEFINE_EXPECT( driver_SQLSetEnvAttr );
DEFINE_EXPECT( driver_SQLGetInfo );
DEFINE_EXPECT( driver_SQLGetInfo_SQL_DRIVER_ODBC_VER );
DEFINE_EXPECT( driver_SQLConnect );
DEFINE_EXPECT( driver_SQLGetConnectAttr );
DEFINE_EXPECT( driver_SQLDisconnect );
DEFINE_EXPECT( driver_SQLDriverConnect );
DEFINE_EXPECT( driver_SQLBrowseConnect );

static SQLRETURN WINAPI driver_SQLAllocHandle( SQLSMALLINT type,
        SQLHANDLE input_handle, SQLHANDLE *out )
{
    if (type == SQL_HANDLE_ENV) CHECK_EXPECT( driver_SQLAllocHandle_env );
    else if (type == SQL_HANDLE_DBC) CHECK_EXPECT( driver_SQLAllocHandle_con );
    else ok( 0, "SQLAllocHandle type = %d\n", type );

    *out = (SQLHANDLE)(ULONG_PTR)type;
    return SQL_SUCCESS;
}

static SQLRETURN WINAPI driver_SQLFreeHandle( SQLSMALLINT type, SQLHANDLE handle )
{
    if (type == SQL_HANDLE_ENV) CHECK_EXPECT( driver_SQLFreeHandle_env );
    else if (type == SQL_HANDLE_DBC) CHECK_EXPECT( driver_SQLFreeHandle_con );
    else ok( 0, "SQLFreeHandle type = %d\n", type );

    ok( (ULONG_PTR)handle == type, "handle = %p\n", handle );
    return SQL_SUCCESS;
}

static SQLRETURN WINAPI driver_SQLGetEnvAttr( SQLHENV env, SQLINTEGER attr,
        SQLPOINTER val, SQLINTEGER len, SQLINTEGER *out_len )
{
    ok( 0, "unexpected call\n" );
    return SQL_ERROR;
}

static SQLRETURN WINAPI driver_SQLSetEnvAttr( SQLHENV env,
        SQLINTEGER attr, SQLPOINTER val, SQLINTEGER len )
{
    CHECK_EXPECT( driver_SQLSetEnvAttr );
    ok( (ULONG_PTR)env == SQL_HANDLE_ENV, "env = %p\n", env );
    ok( attr == SQL_ATTR_ODBC_VERSION, "attr = %d\n", attr );
    ok( val == (SQLPOINTER)SQL_OV_ODBC2, "val = %p\n", val );
    ok( !len, "len = %d\n", len );
    return SQL_SUCCESS;
}

static SQLRETURN WINAPI driver_SQLGetConnectAttr( SQLHDBC con, SQLINTEGER attr,
        SQLPOINTER val, SQLINTEGER len, SQLINTEGER *out_len )
{
    CHECK_EXPECT( driver_SQLGetConnectAttr );
    ok( attr == SQL_ATTR_LOGIN_TIMEOUT || attr == SQL_ATTR_CONNECTION_TIMEOUT, "attr = %d\n", attr );
    ok( val != NULL, "val = %p\n", val );
    ok( len == sizeof(SQLINTEGER), "len = %d\n", len );
    todo_wine_if( attr == SQL_ATTR_LOGIN_TIMEOUT ) ok( out_len != NULL, "out_len = %p\n", out_len );

    *(SQLINTEGER*)val = 0;
    return SQL_SUCCESS;
}

static SQLRETURN WINAPI driver_SQLSetConnectAttr( SQLHDBC con, SQLINTEGER attr,
        SQLPOINTER val, SQLINTEGER len )
{
    todo_wine ok( 0, "unexpected call\n" );
    return SQL_ERROR;
}

static SQLRETURN WINAPI driver_SQLGetInfo( SQLHDBC con, SQLUSMALLINT type,
        SQLPOINTER info, SQLSMALLINT len, SQLSMALLINT *out_len )
{
    if (type == SQL_DRIVER_ODBC_VER)
    {
        CHECK_EXPECT( driver_SQLGetInfo_SQL_DRIVER_ODBC_VER );
        ok( info != NULL, "info = %p\n", info );
        todo_wine ok( len == 12, "len = %d\n", len );
        ok( out_len != NULL, "out_len = %p\n", out_len );

        strcpy( info, "03.00" );
        *out_len = 5;
        return SQL_SUCCESS;
    }

    CHECK_EXPECT2( driver_SQLGetInfo );
    ok( type == SQL_CURSOR_COMMIT_BEHAVIOR || type == SQL_CURSOR_ROLLBACK_BEHAVIOR
            || type == SQL_GETDATA_EXTENSIONS, "type = %d\n", type );
    return SQL_ERROR;
}

static SQLRETURN WINAPI driver_SQLConnect( SQLHDBC con, SQLCHAR *server,
        SQLSMALLINT server_len, SQLCHAR *user, SQLSMALLINT user_len,
        SQLCHAR *auth, SQLSMALLINT auth_len )
{
    CHECK_EXPECT( driver_SQLConnect );

    ok( (ULONG_PTR)con == SQL_HANDLE_DBC, "con = %p\n", con );
    todo_wine ok( server_len == 12, "server_len = %d\n", server_len );
    ok( !strncmp((char *)server, "winetest_dsn", 12), "server = %s\n",
            debugstr_an((char *)server, server_len) );
    ok( !strcmp((char *)user, "winetest"), "user = %s\n", user );
    ok( user_len == SQL_NTS, "user_len = %d\n", user_len );
    ok( !strcmp((char *)auth, "winetest"), "auth = %s\n", auth );
    ok( auth_len == SQL_NTS, "auth_len = %d\n", auth_len );
    return SQL_SUCCESS;
}

static SQLRETURN WINAPI driver_SQLDriverConnect( SQLHDBC con, SQLHWND win,
        SQLCHAR *in_con, SQLSMALLINT in_con_len, SQLCHAR *out_con,
        SQLSMALLINT out_con_max_len, SQLSMALLINT *out_con_len, SQLUSMALLINT completion )
{
    CHECK_EXPECT( driver_SQLDriverConnect );
    ok( (ULONG_PTR)con == SQL_HANDLE_DBC, "con = %p\n", con );
    ok( !win, "win = %p\n", win );
    todo_wine ok( in_con_len == SQL_NTS, "in_con_len = %d\n", in_con_len );
    todo_wine ok( !strcmp((char *)in_con, "DSN=winetest_dsn;UID=winetest;")
            || !strcmp((char *)in_con, "DSN=winetest_dsn;"), "in_con = %s\n", in_con );
    ok( out_con != NULL, "out_con = %p\n", out_con );
    ok( out_con_max_len == 256, "out_con_max_len = %d\n", out_con_max_len );
    ok( out_con_len != NULL, "out_con_len = %p\n", out_con_len );
    ok( completion == SQL_DRIVER_NOPROMPT, "completion = %d\n", completion );

    strcpy( (char *)out_con, (char *)in_con );
    *out_con_len = strlen( (char *)in_con );
    return SQL_SUCCESS;
}

static SQLRETURN WINAPI driver_SQLBrowseConnect( SQLHDBC con, SQLCHAR *in_con,
        SQLSMALLINT in_con_len, SQLCHAR *out_con, SQLSMALLINT out_con_max_len,
        SQLSMALLINT *out_con_len )
{
    CHECK_EXPECT( driver_SQLBrowseConnect );
    ok( (ULONG_PTR)con == SQL_HANDLE_DBC, "con = %p\n", con );
    todo_wine ok( in_con_len == 17, "in_con_len = %d\n", in_con_len );
    todo_wine ok( !strcmp((char *)in_con, "DSN=winetest_dsn;"), "in_con = %s\n", in_con );
    ok( out_con != NULL, "out_con = %p\n", out_con );
    ok( out_con_max_len == 256, "out_con_max_len = %d\n", out_con_max_len );
    ok( out_con_len != NULL, "out_con_len = %p\n", out_con_len );

    strcpy( (char *)out_con, (char *)in_con );
    *out_con_len = strlen( (char *)in_con );
    return SQL_SUCCESS;
}

static SQLRETURN WINAPI driver_SQLDisconnect( SQLHDBC con )
{
    CHECK_EXPECT( driver_SQLDisconnect );
    return SQL_SUCCESS;
}

struct driver_funcs driver_funcs =
{
    driver_SQLAllocHandle,
    driver_SQLFreeHandle,
    driver_SQLGetEnvAttr,
    driver_SQLSetEnvAttr,
    driver_SQLGetConnectAttr,
    driver_SQLSetConnectAttr,
    driver_SQLGetInfo,
    driver_SQLConnect,
    driver_SQLDriverConnect,
    driver_SQLBrowseConnect,
    driver_SQLDisconnect,
};

static void load_resource(const char *name, char *path)
{
    DWORD written;
    HANDLE file;
    HRSRC res;
    void *ptr;

    GetTempPathA( MAX_PATH, path );
    strcat( path, name );

    file = CreateFileA( path, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    ok( file != INVALID_HANDLE_VALUE, "file creation failed, at %s, error %lu\n", path, GetLastError() );

    res = FindResourceA( NULL, name, "TESTDLL" );
    ok( res != 0, "couldn't find resource\n" );
    ptr = LockResource( LoadResource(GetModuleHandleA(NULL), res) );
    WriteFile( file, ptr, SizeofResource(GetModuleHandleA(NULL), res), &written, NULL );
    ok( written == SizeofResource(GetModuleHandleA(NULL), res), "couldn't write resource\n" );
    CloseHandle( file );
}

static HMODULE driver_hmod;

static void cleanup_odbc_driver( char *driver_path )
{
    DWORD count;
    BOOL r;

    FreeLibrary( driver_hmod );
    r = SQLRemoveDSNFromIni( "winetest_dsn" );
    ok( r, "SQLRemoveDSNFromInit failed\n" );
    r = SQLRemoveDriver( "winetest", TRUE, &count );
    ok( r, "SQLRemoveDriver failed\n" );
    DeleteFileA( driver_path );
}

static void setup_odbc_driver( char *driver_path )
{
    void (WINAPI *p_init_funcs)(struct driver_funcs *funcs);
    char buf[512], tmp[MAX_PATH];
    WORD tmp_len;
    BOOL r;

    load_resource( "driver.dll", driver_path );
    driver_hmod = LoadLibraryA( driver_path );
    ok( driver_hmod != NULL, "LoadLibrary failed\n" );
    p_init_funcs = (void *)GetProcAddress( driver_hmod, "init_funcs" );
    p_init_funcs( &driver_funcs );

    sprintf( buf, "winetest%cdriver=%s%c", 0, driver_path, 0 );
    r = SQLInstallDriver( NULL, buf, tmp, sizeof(tmp), &tmp_len );
    ok( r, "SQLInstallDriver failed\n" );

    r = SQLWriteDSNToIni( "winetest_dsn", "winetest" );
    ok( r, "SQLWriteDSNToInit failed\n" );
}

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
    ok( timeout == SQL_LOGIN_TIMEOUT_DEFAULT, "wrong timeout %d\n", timeout );

    SET_EXPECT( driver_SQLAllocHandle_env );
    SET_EXPECT( driver_SQLSetEnvAttr );
    SET_EXPECT( driver_SQLAllocHandle_con );
    SET_EXPECT( driver_SQLGetInfo_SQL_DRIVER_ODBC_VER );
    SET_EXPECT( driver_SQLConnect );
    SET_EXPECT( driver_SQLGetInfo );
    ret = SQLConnect( con, (SQLCHAR *)"winetest_dsn", SQL_NTS, (SQLCHAR *)"winetest", SQL_NTS, (SQLCHAR *)"winetest",
                      SQL_NTS );
    CHECK_CALLED( driver_SQLAllocHandle_env );
    CHECK_CALLED( driver_SQLSetEnvAttr );
    CHECK_CALLED( driver_SQLAllocHandle_con );
    CHECK_CALLED( driver_SQLGetInfo_SQL_DRIVER_ODBC_VER );
    CHECK_CALLED( driver_SQLConnect );
    todo_wine CHECK_CALLED( driver_SQLGetInfo );
    ok (ret == SQL_SUCCESS, "got %d\n", ret );
    if (ret == SQL_ERROR) diag( con, SQL_HANDLE_DBC );

    timeout = 0xdeadbeef;
    SET_EXPECT( driver_SQLGetConnectAttr );
    ret = SQLGetConnectAttr( con, SQL_ATTR_LOGIN_TIMEOUT, &timeout, sizeof(timeout), NULL );
    CHECK_CALLED( driver_SQLGetConnectAttr );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );
    ok( !timeout, "wrong timeout %d\n", timeout );

    timeout = 0xdeadbeef;
    size = -1;
    SET_EXPECT( driver_SQLGetConnectAttr );
    ret = SQLGetConnectAttr( con, SQL_ATTR_CONNECTION_TIMEOUT, &timeout, sizeof(timeout), &size );
    CHECK_CALLED( driver_SQLGetConnectAttr );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );
    ok( timeout != 0xdeadbeef, "timeout = %d\n", timeout );
    ok( size == -1, "size set\n" );

    ret = SQLTransact( NULL, NULL, SQL_COMMIT );
    ok( ret == SQL_INVALID_HANDLE, "got %d\n", ret );

    ret = SQLTransact( env, NULL, SQL_COMMIT );
    todo_wine ok( ret == SQL_SUCCESS, "got %d\n", ret );

    ret = SQLTransact( NULL, con, SQL_COMMIT );
    todo_wine ok( ret == SQL_SUCCESS, "got %d\n", ret );

    ret = SQLTransact( env, con, SQL_COMMIT );
    todo_wine ok( ret == SQL_SUCCESS, "got %d\n", ret );

    SET_EXPECT( driver_SQLDisconnect );
    ret = SQLDisconnect( con );
    CHECK_CALLED( driver_SQLDisconnect );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );

    SET_EXPECT( driver_SQLFreeHandle_con );
    SET_EXPECT( driver_SQLFreeHandle_env );
    ret = SQLFreeConnect( con );
    CHECK_CALLED( driver_SQLFreeHandle_con );
    todo_wine CHECK_CALLED( driver_SQLFreeHandle_env );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );

    SET_EXPECT( driver_SQLFreeHandle_env );
    ret = SQLFreeEnv( env );
    todo_wine CHECK_NOT_CALLED( driver_SQLFreeHandle_env );
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
    SET_EXPECT( driver_SQLAllocHandle_env );
    SET_EXPECT( driver_SQLSetEnvAttr );
    SET_EXPECT( driver_SQLAllocHandle_con );
    SET_EXPECT( driver_SQLGetInfo_SQL_DRIVER_ODBC_VER );
    SET_EXPECT( driver_SQLDriverConnect );
    SET_EXPECT( driver_SQLGetInfo );
    ret = SQLDriverConnect( con, NULL, (SQLCHAR *)"dsn=winetest_dsn;UID=winetest",
                            strlen("dsn=winetest_dsn;UID=winetest"), str, sizeof(str), &len, 0 );
    CHECK_CALLED( driver_SQLAllocHandle_env );
    CHECK_CALLED( driver_SQLSetEnvAttr );
    CHECK_CALLED( driver_SQLAllocHandle_con );
    CHECK_CALLED( driver_SQLGetInfo_SQL_DRIVER_ODBC_VER );
    CHECK_CALLED( driver_SQLDriverConnect );
    todo_wine CHECK_CALLED( driver_SQLGetInfo );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );
    if (ret == SQL_ERROR) diag( con, SQL_HANDLE_DBC );
    todo_wine {
    ok( !strcmp( (const char *)str, "DSN=winetest_dsn;UID=winetest;" ), "got '%s'\n", str );
    ok( len == 30, "got %d\n", len );
    }

    SET_EXPECT( driver_SQLDisconnect );
    ret = SQLDisconnect( con );
    CHECK_CALLED( driver_SQLDisconnect );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );

    /* trailing garbage */
    len = 0;
    str[0] = 0;
    SET_EXPECT( driver_SQLAllocHandle_env );
    SET_EXPECT( driver_SQLSetEnvAttr );
    SET_EXPECT( driver_SQLAllocHandle_con );
    SET_EXPECT( driver_SQLGetInfo_SQL_DRIVER_ODBC_VER );
    SET_EXPECT( driver_SQLDriverConnect );
    SET_EXPECT( driver_SQLGetInfo );
    ret = SQLDriverConnect( con, NULL, (SQLCHAR *)"DSN=winetest_dsn;er\\9.99", strlen("DSN=winetest_dsn;er\\9.99"),
                            str, sizeof(str), &len, 0 );
    todo_wine CHECK_NOT_CALLED( driver_SQLAllocHandle_env );
    todo_wine CHECK_NOT_CALLED( driver_SQLSetEnvAttr );
    todo_wine CHECK_NOT_CALLED( driver_SQLAllocHandle_con );
    todo_wine CHECK_NOT_CALLED( driver_SQLGetInfo_SQL_DRIVER_ODBC_VER );
    CHECK_CALLED( driver_SQLDriverConnect );
    todo_wine CHECK_CALLED( driver_SQLGetInfo );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );
    todo_wine {
    ok( !strcmp( (const char *)str, "DSN=winetest_dsn;" ), "got '%s'\n", str );
    ok( len == 17, "got %d\n", len );
    }

    SET_EXPECT( driver_SQLDisconnect );
    ret = SQLDisconnect( con );
    CHECK_CALLED( driver_SQLDisconnect );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );

    SET_EXPECT( driver_SQLFreeHandle_con );
    SET_EXPECT( driver_SQLFreeHandle_env );
    ret = SQLFreeConnect( con );
    CHECK_CALLED( driver_SQLFreeHandle_con );
    todo_wine CHECK_CALLED( driver_SQLFreeHandle_env );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );

    SET_EXPECT( driver_SQLFreeHandle_env );
    ret = SQLFreeEnv( env );
    todo_wine CHECK_NOT_CALLED( driver_SQLFreeHandle_env );
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
    SET_EXPECT( driver_SQLAllocHandle_env );
    SET_EXPECT( driver_SQLSetEnvAttr );
    SET_EXPECT( driver_SQLAllocHandle_con );
    SET_EXPECT( driver_SQLGetInfo_SQL_DRIVER_ODBC_VER );
    SET_EXPECT( driver_SQLBrowseConnect );
    SET_EXPECT( driver_SQLGetInfo );
    ret = SQLBrowseConnect( con, (SQLCHAR *)"DSN=winetest_dsn", 16, str, sizeof(str), &len );
    CHECK_CALLED( driver_SQLAllocHandle_env );
    CHECK_CALLED( driver_SQLSetEnvAttr );
    CHECK_CALLED( driver_SQLAllocHandle_con );
    CHECK_CALLED( driver_SQLGetInfo_SQL_DRIVER_ODBC_VER );
    CHECK_CALLED( driver_SQLBrowseConnect );
    todo_wine CHECK_CALLED( driver_SQLGetInfo );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );
    if (ret == SQL_ERROR) diag( con, SQL_HANDLE_DBC );
    todo_wine ok( !strcmp( (const char *)str, "DSN=winetest_dsn;" ), "got '%s'\n", str );
    todo_wine ok( len == 17, "got %d\n", len );

    SET_EXPECT( driver_SQLDisconnect );
    ret = SQLDisconnect( con );
    CHECK_CALLED( driver_SQLDisconnect );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );

    SET_EXPECT( driver_SQLFreeHandle_con );
    SET_EXPECT( driver_SQLFreeHandle_env );
    ret = SQLFreeConnect( con );
    CHECK_CALLED( driver_SQLFreeHandle_con );
    todo_wine CHECK_CALLED( driver_SQLFreeHandle_env );
    ok( ret == SQL_SUCCESS, "got %d\n", ret );

    SET_EXPECT( driver_SQLFreeHandle_env );
    ret = SQLFreeEnv( env );
    todo_wine CHECK_NOT_CALLED( driver_SQLFreeHandle_env );
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

BOOL is_process_elevated(void)
{
    TOKEN_ELEVATION_TYPE type = TokenElevationTypeDefault;
    HANDLE token;
    DWORD size;
    BOOL ret;

    if (!OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &token) ) return FALSE;
    ret = GetTokenInformation( token, TokenElevationType, &type, sizeof(type), &size );
    CloseHandle( token );
    return ret && type == TokenElevationTypeFull;
}

static HANDLE get_admin_token(void)
{
    TOKEN_ELEVATION_TYPE type;
    TOKEN_LINKED_TOKEN linked;
    DWORD size;

    if (!GetTokenInformation( GetCurrentThreadEffectiveToken(), TokenElevationType, &type, sizeof(type), &size )
            || type == TokenElevationTypeFull)
        return NULL;

    if (!GetTokenInformation( GetCurrentThreadEffectiveToken(), TokenLinkedToken, &linked, sizeof(linked), &size ))
        return NULL;
    return linked.LinkedToken;
}

static void restart_as_admin_elevated(void)
{
    PROCESS_INFORMATION pi;
    STARTUPINFOW si;
    HANDLE token;

    if (!(token = get_admin_token())) return;

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    if (CreateProcessAsUserW( token, NULL, GetCommandLineW(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi ))
    {
        DWORD exit_code;

        WaitForSingleObject( pi.hProcess, INFINITE );
        GetExitCodeProcess( pi.hProcess, &exit_code );
        CloseHandle( pi.hProcess );
        CloseHandle( pi.hThread );
        ExitProcess( exit_code );
    }
    CloseHandle( token );
}

START_TEST(odbc32)
{
    char driver_path[MAX_PATH];

    if (!is_process_elevated()) restart_as_admin_elevated();

    if (!is_process_elevated()) skip( "process is limited\n" );
    else
    {
        setup_odbc_driver( driver_path );
        test_SQLConnect();
        test_SQLDriverConnect();
        test_SQLBrowseConnect();
        cleanup_odbc_driver( driver_path );
    }

    test_SQLAllocHandle();
    test_SQLGetDiagRec();
    test_SQLDataSources();
    test_SQLDrivers();
    test_SQLExecDirect();
    test_SQLSetEnvAttr();
    test_SQLSetConnectAttr();
}
