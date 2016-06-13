/*
 * Copyright 2016 Nikolay Sivov for CodeWeavers
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

#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <wchar.h>
#include <stdio.h>
#include <float.h>

#include <windef.h>
#include <winbase.h>
#include "wine/test.h"

typedef int (CDECL *MSVCRT__onexit_t)(void);

typedef struct MSVCRT__onexit_table_t
{
    MSVCRT__onexit_t *_first;
    MSVCRT__onexit_t *_last;
    MSVCRT__onexit_t *_end;
} MSVCRT__onexit_table_t;

static int (CDECL *p_initialize_onexit_table)(MSVCRT__onexit_table_t *table);
static int (CDECL *p_register_onexit_function)(MSVCRT__onexit_table_t *table, MSVCRT__onexit_t func);
static int (CDECL *p_execute_onexit_table)(MSVCRT__onexit_table_t *table);
static int (CDECL *p___fpe_flt_rounds)(void);
static unsigned int (CDECL *p__controlfp)(unsigned int, unsigned int);

static void test__initialize_onexit_table(void)
{
    MSVCRT__onexit_table_t table, table2;
    int ret;

    ret = p_initialize_onexit_table(NULL);
    ok(ret == -1, "got %d\n", ret);

    memset(&table, 0, sizeof(table));
    ret = p_initialize_onexit_table(&table);
    ok(ret == 0, "got %d\n", ret);
    ok(table._first == table._last && table._first == table._end, "got first %p, last %p, end %p\n",
        table._first, table._last, table._end);

    memset(&table2, 0, sizeof(table2));
    ret = p_initialize_onexit_table(&table2);
    ok(ret == 0, "got %d\n", ret);
    ok(table2._first == table._first, "got %p, %p\n", table2._first, table._first);

    /* uninitialized table */
    table._first = table._last = table._end = (void*)0x123;
    ret = p_initialize_onexit_table(&table);
    ok(ret == 0, "got %d\n", ret);
    ok(table._first == table._last && table._first == table._end, "got first %p, last %p, end %p\n",
        table._first, table._last, table._end);
    ok(table._first != (void*)0x123, "got %p\n", table._first);

    table._first = (void*)0x123;
    table._last = (void*)0x456;
    table._end = (void*)0x123;
    ret = p_initialize_onexit_table(&table);
    ok(ret == 0, "got %d\n", ret);
    ok(table._first == table._last && table._first == table._end, "got first %p, last %p, end %p\n",
        table._first, table._last, table._end);
    ok(table._first != (void*)0x123, "got %p\n", table._first);

    table._first = (void*)0x123;
    table._last = (void*)0x456;
    table._end = (void*)0x789;
    ret = p_initialize_onexit_table(&table);
    ok(ret == 0, "got %d\n", ret);
    ok(table._first == (void*)0x123, "got %p\n", table._first);
    ok(table._last == (void*)0x456, "got %p\n", table._last);
    ok(table._end == (void*)0x789, "got %p\n", table._end);

    table._first = NULL;
    table._last = (void*)0x456;
    table._end = NULL;
    ret = p_initialize_onexit_table(&table);
    ok(ret == 0, "got %d\n", ret);
    ok(table._first == table._last && table._first == table._end, "got first %p, last %p, end %p\n",
        table._first, table._last, table._end);
}

static int g_onexit_called;
static int CDECL onexit_func(void)
{
    g_onexit_called++;
    return 0;
}

static int CDECL onexit_func2(void)
{
    ok(g_onexit_called == 0, "got %d\n", g_onexit_called);
    g_onexit_called++;
    return 0;
}

static void test__register_onexit_function(void)
{
    MSVCRT__onexit_table_t table;
    MSVCRT__onexit_t *f;
    int ret;

    memset(&table, 0, sizeof(table));
    ret = p_initialize_onexit_table(&table);
    ok(ret == 0, "got %d\n", ret);

    ret = p_register_onexit_function(NULL, NULL);
    ok(ret == -1, "got %d\n", ret);

    ret = p_register_onexit_function(NULL, onexit_func);
    ok(ret == -1, "got %d\n", ret);

    f = table._last;
    ret = p_register_onexit_function(&table, NULL);
    ok(ret == 0, "got %d\n", ret);
    ok(f != table._last, "got %p, initial %p\n", table._last, f);

    ret = p_register_onexit_function(&table, onexit_func);
    ok(ret == 0, "got %d\n", ret);

    f = table._last;
    ret = p_register_onexit_function(&table, onexit_func);
    ok(ret == 0, "got %d\n", ret);
    ok(f != table._last, "got %p, initial %p\n", table._last, f);
}

static void test__execute_onexit_table(void)
{
    MSVCRT__onexit_table_t table;
    int ret;

    ret = p_execute_onexit_table(NULL);
    ok(ret == -1, "got %d\n", ret);

    memset(&table, 0, sizeof(table));
    ret = p_initialize_onexit_table(&table);
    ok(ret == 0, "got %d\n", ret);

    /* execute empty table */
    ret = p_execute_onexit_table(&table);
    ok(ret == 0, "got %d\n", ret);

    /* same functions registered twice */
    ret = p_register_onexit_function(&table, onexit_func);
    ok(ret == 0, "got %d\n", ret);

    ret = p_register_onexit_function(&table, NULL);
    ok(ret == 0, "got %d\n", ret);

    ret = p_register_onexit_function(&table, onexit_func);
    ok(ret == 0, "got %d\n", ret);

    ok(table._first != table._end, "got %p, %p\n", table._first, table._end);
    g_onexit_called = 0;
    ret = p_execute_onexit_table(&table);
    ok(ret == 0, "got %d\n", ret);
    ok(g_onexit_called == 2, "got %d\n", g_onexit_called);
    ok(table._first == table._end, "got %p, %p\n", table._first, table._end);

    /* execute again, table is already empty */
    g_onexit_called = 0;
    ret = p_execute_onexit_table(&table);
    ok(ret == 0, "got %d\n", ret);
    ok(g_onexit_called == 0, "got %d\n", g_onexit_called);

    /* check call order */
    memset(&table, 0, sizeof(table));
    ret = p_initialize_onexit_table(&table);
    ok(ret == 0, "got %d\n", ret);

    ret = p_register_onexit_function(&table, onexit_func);
    ok(ret == 0, "got %d\n", ret);

    ret = p_register_onexit_function(&table, onexit_func2);
    ok(ret == 0, "got %d\n", ret);

    g_onexit_called = 0;
    ret = p_execute_onexit_table(&table);
    ok(ret == 0, "got %d\n", ret);
    ok(g_onexit_called == 2, "got %d\n", g_onexit_called);
}

static void test___fpe_flt_rounds(void)
{
    unsigned int cfp = p__controlfp(0, 0);
    int ret;

    if(!cfp) {
        skip("_controlfp not supported\n");
        return;
    }

    ok((p__controlfp(_RC_NEAR, _RC_CHOP) & _RC_CHOP) == _RC_NEAR, "_controlfp(_RC_NEAR, _RC_CHOP) failed\n");
    ret = p___fpe_flt_rounds();
    ok(ret == 1, "__fpe_flt_rounds returned %d\n", ret);

    ok((p__controlfp(_RC_UP, _RC_CHOP) & _RC_CHOP) == _RC_UP, "_controlfp(_RC_UP, _RC_CHOP) failed\n");
    ret = p___fpe_flt_rounds();
    ok(ret == 2 + (sizeof(void*)>sizeof(int)), "__fpe_flt_rounds returned %d\n", ret);

    ok((p__controlfp(_RC_DOWN, _RC_CHOP) & _RC_CHOP) == _RC_DOWN, "_controlfp(_RC_DOWN, _RC_CHOP) failed\n");
    ret = p___fpe_flt_rounds();
    ok(ret == 3 - (sizeof(void*)>sizeof(int)), "__fpe_flt_rounds returned %d\n", ret);

    ok((p__controlfp(_RC_CHOP, _RC_CHOP) & _RC_CHOP) == _RC_CHOP, "_controlfp(_RC_CHOP, _RC_CHOP) failed\n");
    ret = p___fpe_flt_rounds();
    ok(ret == 0, "__fpe_flt_rounds returned %d\n", ret);
}

static BOOL init(void)
{
    HMODULE module = LoadLibraryA("ucrtbase.dll");

    if(!module) {
        win_skip("ucrtbase.dll not available\n");
        return FALSE;
    }

    p_initialize_onexit_table = (void*)GetProcAddress(module, "_initialize_onexit_table");
    p_register_onexit_function = (void*)GetProcAddress(module, "_register_onexit_function");
    p_execute_onexit_table = (void*)GetProcAddress(module, "_execute_onexit_table");
    p___fpe_flt_rounds = (void*)GetProcAddress(module, "__fpe_flt_rounds");
    p__controlfp = (void*)GetProcAddress(module, "_controlfp");

    return TRUE;
}

START_TEST(misc)
{
    if(!init())
        return;

    test__initialize_onexit_table();
    test__register_onexit_function();
    test__execute_onexit_table();
    test___fpe_flt_rounds();
}
