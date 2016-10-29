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
#include <io.h>
#include <sys/stat.h>
#include <share.h>
#include <fcntl.h>

#include <windef.h>
#include <winbase.h>
#include "wine/test.h"

#define DEFINE_EXPECT(func) \
    static BOOL expect_ ## func = FALSE, called_ ## func = FALSE

#define SET_EXPECT(func) \
    expect_ ## func = TRUE

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

DEFINE_EXPECT(global_invalid_parameter_handler);
DEFINE_EXPECT(thread_invalid_parameter_handler);

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
static _invalid_parameter_handler (CDECL *p__set_invalid_parameter_handler)(_invalid_parameter_handler);
static _invalid_parameter_handler (CDECL *p__get_invalid_parameter_handler)(void);
static _invalid_parameter_handler (CDECL *p__set_thread_local_invalid_parameter_handler)(_invalid_parameter_handler);
static _invalid_parameter_handler (CDECL *p__get_thread_local_invalid_parameter_handler)(void);
static int (CDECL *p__ltoa_s)(LONG, char*, size_t, int);
static char* (CDECL *p__get_narrow_winmain_command_line)(void);
static int (CDECL *p_sopen_dispatch)(const char *, int, int, int, int *, int);
static int (CDECL *p_sopen_s)(int *, const char *, int, int, int);

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

static void __cdecl global_invalid_parameter_handler(
        const wchar_t *expression, const wchar_t *function,
        const wchar_t *file, unsigned line, uintptr_t arg)
{
    CHECK_EXPECT(global_invalid_parameter_handler);
}

static void __cdecl thread_invalid_parameter_handler(
        const wchar_t *expression, const wchar_t *function,
        const wchar_t *file, unsigned line, uintptr_t arg)
{
    CHECK_EXPECT(thread_invalid_parameter_handler);
}

static void test_invalid_parameter_handler(void)
{
    _invalid_parameter_handler ret;

    ret = p__get_invalid_parameter_handler();
    ok(!ret, "ret != NULL\n");

    ret = p__get_thread_local_invalid_parameter_handler();
    ok(!ret, "ret != NULL\n");

    ret = p__set_thread_local_invalid_parameter_handler(thread_invalid_parameter_handler);
    ok(!ret, "ret != NULL\n");

    ret = p__get_thread_local_invalid_parameter_handler();
    ok(ret == thread_invalid_parameter_handler, "ret = %p\n", ret);

    ret = p__get_invalid_parameter_handler();
    ok(!ret, "ret != NULL\n");

    ret = p__set_invalid_parameter_handler(global_invalid_parameter_handler);
    ok(!ret, "ret != NULL\n");

    ret = p__get_invalid_parameter_handler();
    ok(ret == global_invalid_parameter_handler, "ret = %p\n", ret);

    ret = p__get_thread_local_invalid_parameter_handler();
    ok(ret == thread_invalid_parameter_handler, "ret = %p\n", ret);

    SET_EXPECT(thread_invalid_parameter_handler);
    p__ltoa_s(0, NULL, 0, 0);
    CHECK_CALLED(thread_invalid_parameter_handler);

    ret = p__set_thread_local_invalid_parameter_handler(NULL);
    ok(ret == thread_invalid_parameter_handler, "ret = %p\n", ret);

    SET_EXPECT(global_invalid_parameter_handler);
    p__ltoa_s(0, NULL, 0, 0);
    CHECK_CALLED(global_invalid_parameter_handler);

    ret = p__set_invalid_parameter_handler(NULL);
    ok(ret == global_invalid_parameter_handler, "ret = %p\n", ret);
}

static void test__get_narrow_winmain_command_line(char *path)
{
    PROCESS_INFORMATION proc;
    STARTUPINFOA startup;
    char cmd[MAX_PATH+32];
    char *ret, *cmdline, *name;
    int len;

    ret = p__get_narrow_winmain_command_line();
    cmdline = GetCommandLineA();
    len = strlen(cmdline);
    ok(ret>cmdline && ret<cmdline+len, "ret = %p, cmdline = %p (len = %d)\n", ret, cmdline, len);

    if(!path) {
        ok(!lstrcmpA(ret, "\"misc\" cmd"), "ret = %s\n", ret);
        return;
    }

    for(len = strlen(path); len>0; len--)
        if(path[len-1]=='\\' || path[len-1]=='/') break;
    if(len) name = path+len;
    else name = path;

    sprintf(cmd, "\"\"%c\"\"\"%s\" \t \"misc\" cmd", name[0], name+1);
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    CreateProcessA(path, cmd, NULL, NULL, TRUE,
            CREATE_DEFAULT_ERROR_MODE|NORMAL_PRIORITY_CLASS,
            NULL, NULL, &startup, &proc);
    winetest_wait_child_process(proc.hProcess);
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
    p__set_invalid_parameter_handler = (void*)GetProcAddress(module, "_set_invalid_parameter_handler");
    p__get_invalid_parameter_handler = (void*)GetProcAddress(module, "_get_invalid_parameter_handler");
    p__set_thread_local_invalid_parameter_handler = (void*)GetProcAddress(module, "_set_thread_local_invalid_parameter_handler");
    p__get_thread_local_invalid_parameter_handler = (void*)GetProcAddress(module, "_get_thread_local_invalid_parameter_handler");
    p__ltoa_s = (void*)GetProcAddress(module, "_ltoa_s");
    p__get_narrow_winmain_command_line = (void*)GetProcAddress(GetModuleHandleA("ucrtbase.dll"), "_get_narrow_winmain_command_line");
    p_sopen_dispatch = (void*)GetProcAddress(module, "_sopen_dispatch");
    p_sopen_s = (void*)GetProcAddress(module, "_sopen_s");

    return TRUE;
}

static void test__sopen_dispatch(void)
{
    int ret, fd;
    char *tempf;

    tempf = _tempnam(".", "wne");

    fd = 0;
    ret = p_sopen_dispatch(tempf, _O_CREAT, _SH_DENYWR, 0xff, &fd, 0);
    ok(!ret, "got %d\n", ret);
    ok(fd > 0, "got fd %d\n", fd);
    _close(fd);
    unlink(tempf);

    p__set_invalid_parameter_handler(global_invalid_parameter_handler);

    SET_EXPECT(global_invalid_parameter_handler);
    fd = 0;
    ret = p_sopen_dispatch(tempf, _O_CREAT, _SH_DENYWR, 0xff, &fd, 1);
todo_wine {
    ok(ret == EINVAL, "got %d\n", ret);
    ok(fd == -1, "got fd %d\n", fd);
    CHECK_CALLED(global_invalid_parameter_handler);
}
    if (fd > 0)
    {
        _close(fd);
        unlink(tempf);
    }

    p__set_invalid_parameter_handler(NULL);

    free(tempf);
}

static void test__sopen_s(void)
{
    int ret, fd;
    char *tempf;

    tempf = _tempnam(".", "wne");

    fd = 0;
    ret = p_sopen_s(&fd, tempf, _O_CREAT, _SH_DENYWR, 0);
    ok(!ret, "got %d\n", ret);
    ok(fd > 0, "got fd %d\n", fd);
    _close(fd);
    unlink(tempf);

    /* _open() does not validate pmode */
    fd = _open(tempf, _O_CREAT, 0xff);
    ok(fd > 0, "got fd %d\n", fd);
    _close(fd);
    unlink(tempf);

    p__set_invalid_parameter_handler(global_invalid_parameter_handler);

    /* _sopen_s() invokes invalid parameter handler on invalid pmode */
    SET_EXPECT(global_invalid_parameter_handler);
    fd = 0;
    ret = p_sopen_s(&fd, tempf, _O_CREAT, _SH_DENYWR, 0xff);
todo_wine {
    ok(ret == EINVAL, "got %d\n", ret);
    ok(fd == -1, "got fd %d\n", fd);
    CHECK_CALLED(global_invalid_parameter_handler);
}
    p__set_invalid_parameter_handler(NULL);

    free(tempf);
}

START_TEST(misc)
{
    int arg_c;
    char** arg_v;

    if(!init())
        return;

    arg_c = winetest_get_mainargs(&arg_v);
    if(arg_c == 3) {
        test__get_narrow_winmain_command_line(NULL);
        return;
    }

    test_invalid_parameter_handler();
    test__initialize_onexit_table();
    test__register_onexit_function();
    test__execute_onexit_table();
    test___fpe_flt_rounds();
    test__get_narrow_winmain_command_line(arg_v[0]);
    test__sopen_dispatch();
    test__sopen_s();
}
