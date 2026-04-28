/*
 * Unit test suite for winsqlite3
 *
 * Copyright 2026 Paul Gofman for CodeWeavers
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
 */

#include <stdarg.h>
#include <stdint.h>
#include "windef.h"
#include "winbase.h"
#include "winsqlite/winsqlite3.h"
#include "wine/test.h"

#define DECL_FUNC(name) typeof(name) *p_##name;
DECL_FUNC(sqlite3_open)
DECL_FUNC(sqlite3_exec)
DECL_FUNC(sqlite3_close)
#undef DECL_FUNC
const char *p_sqlite3_version;

HMODULE hsqlite3;

static void load_funcs(void)
{
#define GET_FUNC(name) p_##name = (void *)GetProcAddress(hsqlite3, #name); ok(!!p_##name, "%s not found.\n", #name);
GET_FUNC(sqlite3_open)
GET_FUNC(sqlite3_exec)
GET_FUNC(sqlite3_close)
GET_FUNC(sqlite3_version)
#undef GET_FUNC
}

static int SQLITE_CALLBACK select_callback(void *call_count, int col_count, char **value, char **col_name)
{
    int *count = call_count;

    ++*count;
    ok(col_count == 2, "got %d.\n", col_count);
    ok(!strcmp(col_name[0], "ID"), "got %s.\n", debugstr_a(col_name[0]));
    ok(!strcmp(col_name[1], "NAME"), "got %s.\n", debugstr_a(col_name[1]));
    ok(!strcmp(value[0], "1"), "got %s.\n", debugstr_a(value[0]));
    ok(!strcmp(value[1], "Test1"), "got %s.\n", debugstr_a(value[1]));
    return 0;
}

static void test_exec(void)
{
    int callback_call_count;
    char *err_msg;
    sqlite3 *db;
    BOOL bret;
    int ret;

    DeleteFileA("test.db");
    ret = p_sqlite3_open("test.db", &db);
    ok(!ret, "got %d.\n", ret);
    ret = p_sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS TEST(ID INTEGER PRIMARY KEY, NAME TEXT);", 0, 0, &err_msg);
    ok(!ret, "got %d.\n", ret);
    ret = p_sqlite3_exec(db, "INSERT INTO TEST VALUES (1, 'Test1');", 0, 0, &err_msg);
    ok(!ret, "got %d.\n", ret);
    callback_call_count = 0;
    ret = p_sqlite3_exec(db, "SELECT * FROM TEST;", select_callback, &callback_call_count, &err_msg);
    ok(!ret, "got %d.\n", ret);
    ok(callback_call_count == 1, "got %d.\n", callback_call_count);
    ret = p_sqlite3_close(db);
    ok(!ret, "got %d.\n", ret);
    bret = DeleteFileA("test.db");
    ok(bret, "got error %ld.\n", GetLastError());
}

START_TEST(winsqlite3)
{
    hsqlite3 = LoadLibraryA("winsqlite3.dll");
    if (!hsqlite3)
    {
        /* Supported since Win10. */
        win_skip("winsqlite3.dll not found.\n");
        return;
    }
    load_funcs();
    trace("version %s.\n", p_sqlite3_version);

    test_exec();
}
