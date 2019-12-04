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
#include <math.h>
#include <float.h>
#include <io.h>
#include <sys/stat.h>
#include <share.h>
#include <fcntl.h>
#include <time.h>
#include <direct.h>
#include <locale.h>

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

static inline double __port_min_pos_double(void)
{
    static const UINT64 __min_pos_double = 0x10000000000000;
    return *(const double *)&__min_pos_double;
}

static inline double __port_max_double(void)
{
    static const UINT64 __max_double = 0x7FEFFFFFFFFFFFFF;
    return *(const double *)&__max_double;
}

#define M_PI_2 1.57079632679489661923

#define FE_TONEAREST 0

#define _DOMAIN         1       /* domain error in argument */
#define _SING           2       /* singularity */
#define _OVERFLOW       3       /* range overflow */
#define _UNDERFLOW      4       /* range underflow */

DEFINE_EXPECT(global_invalid_parameter_handler);
DEFINE_EXPECT(thread_invalid_parameter_handler);

typedef int (CDECL *MSVCRT__onexit_t)(void);

typedef struct MSVCRT__onexit_table_t
{
    MSVCRT__onexit_t *_first;
    MSVCRT__onexit_t *_last;
    MSVCRT__onexit_t *_end;
} MSVCRT__onexit_table_t;

typedef struct MSVCRT__lldiv_t {
    LONGLONG quot;  /* quotient */
    LONGLONG rem;   /* remainder */
} MSVCRT_lldiv_t;

typedef struct MSVCRT__exception {
    int     type;
    char*   name;
    double  arg1;
    double  arg2;
    double  retval;
} MSVCRT__exception;

typedef struct {
    const char *short_wday[7];
    const char *wday[7];
    const char *short_mon[12];
    const char *mon[12];
    const char *am;
    const char *pm;
    const char *short_date;
    const char *date;
    const char *time;
    int  unk[2];
    const wchar_t *short_wdayW[7];
    const wchar_t *wdayW[7];
    const wchar_t *short_monW[12];
    const wchar_t *monW[12];
    const wchar_t *amW;
    const wchar_t *pmW;
    const wchar_t *short_dateW;
    const wchar_t *dateW;
    const wchar_t *timeW;
    const wchar_t *locnameW;
} __lc_time_data;

typedef int (CDECL *MSVCRT_matherr_func)(struct MSVCRT__exception *);

static HMODULE module;
static LONGLONG crt_init_start, crt_init_end;

static int (CDECL *p_initialize_onexit_table)(MSVCRT__onexit_table_t *table);
static int (CDECL *p_register_onexit_function)(MSVCRT__onexit_table_t *table, MSVCRT__onexit_t func);
static int (CDECL *p_execute_onexit_table)(MSVCRT__onexit_table_t *table);
static int (CDECL *p_o__initialize_onexit_table)(MSVCRT__onexit_table_t *table);
static int (CDECL *p_o__register_onexit_function)(MSVCRT__onexit_table_t *table, MSVCRT__onexit_t func);
static int (CDECL *p_o__execute_onexit_table)(MSVCRT__onexit_table_t *table);
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
static int (WINAPIV *p__open)(const char*, int, ...);
static MSVCRT_lldiv_t* (CDECL *p_lldiv)(MSVCRT_lldiv_t*,LONGLONG,LONGLONG);
static int (CDECL *p__isctype)(int,int);
static int (CDECL *p_isblank)(int);
static int (CDECL *p__isblank_l)(int,_locale_t);
static int (CDECL *p__iswctype_l)(int,int,_locale_t);
static int (CDECL *p_iswblank)(int);
static int (CDECL *p__iswblank_l)(wint_t,_locale_t);
static int (CDECL *p_fesetround)(int);
static void (CDECL *p___setusermatherr)(MSVCRT_matherr_func);
static int* (CDECL *p_errno)(void);
static char* (CDECL *p_asctime)(const struct tm *);
static size_t (__cdecl *p_strftime)(char *, size_t, const char *, const struct tm *);
static size_t (__cdecl *p__Strftime)(char*, size_t, const char*, const struct tm*, void*);
static char* (__cdecl *p_setlocale)(int, const char*);
static struct tm*  (__cdecl *p__gmtime32)(const __time32_t*);
static void (CDECL *p_exit)(int);
static int (CDECL *p__crt_atexit)(void (CDECL*)(void));
static int (__cdecl *p_crt_at_quick_exit)(void (__cdecl *func)(void));
static void (__cdecl *p_quick_exit)(int exitcode);
static int (__cdecl *p__stat32)(const char*, struct _stat32 *buf);
static int (__cdecl *p__close)(int);
static void* (__cdecl *p__o_malloc)(size_t);
static size_t (__cdecl *p__msize)(void*);
static void (__cdecl *p_free)(void*);
static clock_t (__cdecl *p_clock)(void);

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
    ok(table2._last == table._last, "got %p, %p\n", table2._last, table._last);
    ok(table2._end == table._end, "got %p, %p\n", table2._end, table._end);

    memset(&table2, 0, sizeof(table2));
    ret = p_o__initialize_onexit_table(&table2);
    ok(ret == 0, "got %d\n", ret);
    ok(table2._first == table._first, "got %p, %p\n", table2._first, table._first);
    ok(table2._last == table._last, "got %p, %p\n", table2._last, table._last);
    ok(table2._end == table._end, "got %p, %p\n", table2._end, table._end);

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

    f = table._last;
    ret = p_o__register_onexit_function(&table, NULL);
    ok(ret == 0, "got %d\n", ret);
    ok(f != table._last, "got %p, initial %p\n", table._last, f);

    f = table._last;
    ret = p_o__register_onexit_function(&table, onexit_func);
    ok(ret == 0, "got %d\n", ret);
    ok(f != table._last, "got %p, initial %p\n", table._last, f);

    ret = p_execute_onexit_table(&table);
    ok(ret == 0, "got %d\n", ret);
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

    /* same function registered multiple times */
    ret = p_register_onexit_function(&table, onexit_func);
    ok(ret == 0, "got %d\n", ret);

    ret = p_register_onexit_function(&table, NULL);
    ok(ret == 0, "got %d\n", ret);

    ret = p_register_onexit_function(&table, onexit_func);
    ok(ret == 0, "got %d\n", ret);

    ret = p_o__register_onexit_function(&table, onexit_func);
    ok(ret == 0, "got %d\n", ret);

    ok(table._first != table._end, "got %p, %p\n", table._first, table._end);
    g_onexit_called = 0;
    ret = p_execute_onexit_table(&table);
    ok(ret == 0, "got %d\n", ret);
    ok(g_onexit_called == 3, "got %d\n", g_onexit_called);
    ok(table._first == table._end, "got %p, %p\n", table._first, table._end);

    ret = p_register_onexit_function(&table, onexit_func);
    ok(ret == 0, "got %d\n", ret);

    ret = p_register_onexit_function(&table, NULL);
    ok(ret == 0, "got %d\n", ret);

    ret = p_register_onexit_function(&table, onexit_func);
    ok(ret == 0, "got %d\n", ret);

    ret = p_o__register_onexit_function(&table, onexit_func);
    ok(ret == 0, "got %d\n", ret);

    ok(table._first != table._end, "got %p, %p\n", table._first, table._end);
    g_onexit_called = 0;
    ret = p_o__execute_onexit_table(&table);
    ok(ret == 0, "got %d\n", ret);
    ok(g_onexit_called == 3, "got %d\n", g_onexit_called);
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
    ok(ret == 2 || broken(ret == 3) /* w1064v1507 */, "__fpe_flt_rounds returned %d\n", ret);

    ok((p__controlfp(_RC_DOWN, _RC_CHOP) & _RC_CHOP) == _RC_DOWN, "_controlfp(_RC_DOWN, _RC_CHOP) failed\n");
    ret = p___fpe_flt_rounds();
    ok(ret == 3 || broken(ret == 2) /* w1064v1507 */, "__fpe_flt_rounds returned %d\n", ret);

    ok((p__controlfp(_RC_CHOP, _RC_CHOP) & _RC_CHOP) == _RC_CHOP, "_controlfp(_RC_CHOP, _RC_CHOP) failed\n");
    ret = p___fpe_flt_rounds();
    ok(ret == 0, "__fpe_flt_rounds returned %d\n", ret);
}

static void __cdecl global_invalid_parameter_handler(
        const wchar_t *expression, const wchar_t *function,
        const wchar_t *file, unsigned line, uintptr_t arg)
{
    CHECK_EXPECT2(global_invalid_parameter_handler);
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

    ret = p__set_invalid_parameter_handler(global_invalid_parameter_handler);
    ok(!ret, "ret != NULL\n");
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
    CloseHandle(proc.hProcess);
    CloseHandle(proc.hThread);
}

static BOOL init(void)
{
    FILETIME cur;

    GetSystemTimeAsFileTime(&cur);
    crt_init_start = ((LONGLONG)cur.dwHighDateTime << 32) + cur.dwLowDateTime;
    module = LoadLibraryA("ucrtbase.dll");
    GetSystemTimeAsFileTime(&cur);
    crt_init_end = ((LONGLONG)cur.dwHighDateTime << 32) + cur.dwLowDateTime;

    if(!module) {
        win_skip("ucrtbase.dll not available\n");
        return FALSE;
    }

    p_initialize_onexit_table = (void*)GetProcAddress(module, "_initialize_onexit_table");
    p_register_onexit_function = (void*)GetProcAddress(module, "_register_onexit_function");
    p_execute_onexit_table = (void*)GetProcAddress(module, "_execute_onexit_table");
    p_o__initialize_onexit_table = (void*)GetProcAddress(module, "_o__initialize_onexit_table");
    p_o__register_onexit_function = (void*)GetProcAddress(module, "_o__register_onexit_function");
    p_o__execute_onexit_table = (void*)GetProcAddress(module, "_o__execute_onexit_table");
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
    p__open = (void*)GetProcAddress(module, "_open");
    p_lldiv = (void*)GetProcAddress(module, "lldiv");
    p__isctype = (void*)GetProcAddress(module, "_isctype");
    p_isblank = (void*)GetProcAddress(module, "isblank");
    p__isblank_l = (void*)GetProcAddress(module, "_isblank_l");
    p__iswctype_l = (void*)GetProcAddress(module, "_iswctype_l");
    p_iswblank = (void*)GetProcAddress(module, "iswblank");
    p__iswblank_l = (void*)GetProcAddress(module, "_iswblank_l");
    p_fesetround = (void*)GetProcAddress(module, "fesetround");
    p___setusermatherr = (void*)GetProcAddress(module, "__setusermatherr");
    p_errno = (void*)GetProcAddress(module, "_errno");
    p_asctime = (void*)GetProcAddress(module, "asctime");
    p_strftime = (void*)GetProcAddress(module, "strftime");
    p__Strftime = (void*)GetProcAddress(module, "_Strftime");
    p_setlocale = (void*)GetProcAddress(module, "setlocale");
    p__gmtime32 = (void*)GetProcAddress(module, "_gmtime32");
    p__crt_atexit = (void*)GetProcAddress(module, "_crt_atexit");
    p_exit = (void*)GetProcAddress(module, "exit");
    p_crt_at_quick_exit = (void*)GetProcAddress(module, "_crt_at_quick_exit");
    p_quick_exit = (void*)GetProcAddress(module, "quick_exit");
    p__stat32 = (void*)GetProcAddress(module, "_stat32");
    p__close = (void*)GetProcAddress(module, "_close");
    p__o_malloc = (void*)GetProcAddress(module, "_o_malloc");
    p__msize = (void*)GetProcAddress(module, "_msize");
    p_free = (void*)GetProcAddress(module, "free");
    p_clock = (void*)GetProcAddress(module, "clock");

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
    p__close(fd);
    unlink(tempf);

    SET_EXPECT(global_invalid_parameter_handler);
    fd = 0;
    ret = p_sopen_dispatch(tempf, _O_CREAT, _SH_DENYWR, 0xff, &fd, 1);
    ok(ret == EINVAL, "got %d\n", ret);
    ok(fd == -1, "got fd %d\n", fd);
    CHECK_CALLED(global_invalid_parameter_handler);
    if (fd > 0)
    {
        p__close(fd);
        unlink(tempf);
    }

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
    p__close(fd);
    unlink(tempf);

    /* _open() does not validate pmode */
    fd = p__open(tempf, _O_CREAT, 0xff);
    ok(fd > 0, "got fd %d\n", fd);
    p__close(fd);
    unlink(tempf);

    /* _sopen_s() invokes invalid parameter handler on invalid pmode */
    SET_EXPECT(global_invalid_parameter_handler);
    fd = 0;
    ret = p_sopen_s(&fd, tempf, _O_CREAT, _SH_DENYWR, 0xff);
    ok(ret == EINVAL, "got %d\n", ret);
    ok(fd == -1, "got fd %d\n", fd);
    CHECK_CALLED(global_invalid_parameter_handler);

    free(tempf);
}

static void test_lldiv(void)
{
    MSVCRT_lldiv_t r;

    p_lldiv(&r, (LONGLONG)0x111 << 32 | 0x222, (LONGLONG)1 << 32);
    ok(r.quot == 0x111, "quot = %s\n", wine_dbgstr_longlong(r.quot));
    ok(r.rem == 0x222, "rem = %s\n", wine_dbgstr_longlong(r.rem));
}

static void test_isblank(void)
{
    int c;

    for(c = 0; c <= 0xff; c++) {
        if(c == '\t' || c == ' ') {
            if(c == '\t')
                ok(!p__isctype(c, _BLANK), "tab shouldn't be blank\n");
            else
                ok(p__isctype(c, _BLANK), "space should be blank\n");
            ok(p_isblank(c), "%d should be blank\n", c);
            ok(p__isblank_l(c, NULL), "%d should be blank\n", c);
        } else {
            ok(!p__isctype(c, _BLANK), "%d shouldn't be blank\n", c);
            ok(!p_isblank(c), "%d shouldn't be blank\n", c);
            ok(!p__isblank_l(c, NULL), "%d shouldn't be blank\n", c);
        }
    }

    for(c = 0; c <= 0xffff; c++) {
        if(c == '\t' || c == ' ' || c == 0x3000 || c == 0xfeff) {
            if(c == '\t')
                todo_wine ok(!p__iswctype_l(c, _BLANK, NULL), "tab shouldn't be blank\n");
            else
                ok(p__iswctype_l(c, _BLANK, NULL), "%d should be blank\n", c);
            ok(p_iswblank(c), "%d should be blank\n", c);
            ok(p__iswblank_l(c, NULL), "%d should be blank\n", c);
        } else {
            todo_wine_if(c == 0xa0) {
                ok(!p__iswctype_l(c, _BLANK, NULL), "%d shouldn't be blank\n", c);
                ok(!p_iswblank(c), "%d shouldn't be blank\n", c);
                ok(!p__iswblank_l(c, NULL), "%d shouldn't be blank\n", c);
            }
        }
    }
}

static struct MSVCRT__exception exception;

static int CDECL matherr_callback(struct MSVCRT__exception *e)
{
    memcpy(&exception, e, sizeof(MSVCRT__exception));
    return 0;
}

static void test_math_errors(void)
{
    const struct {
        char func[16];
        double x;
        int error;
        int exception;
    } testsd[] = {
        {"_logb", -INFINITY, -1, -1},
        {"_logb", -1, -1, -1},
        {"_logb", 0, ERANGE, _SING},
        {"_logb", INFINITY, -1, -1},
        {"acos", -INFINITY, EDOM, _DOMAIN},
        {"acos", -2, EDOM, _DOMAIN},
        {"acos", -1, -1, -1},
        {"acos", 1, -1, -1},
        {"acos", 2, EDOM, _DOMAIN},
        {"acos", INFINITY, EDOM, _DOMAIN},
        {"acosh", -INFINITY, EDOM, -1},
        {"acosh", 0, EDOM, -1},
        {"acosh", 1, -1, -1},
        {"acosh", INFINITY, -1, -1},
        {"asin", -INFINITY, EDOM, _DOMAIN},
        {"asin", -2, EDOM, _DOMAIN},
        {"asin", -1, -1, -1},
        {"asin", 1, -1, -1},
        {"asin", 2, EDOM, _DOMAIN},
        {"asin", INFINITY, EDOM, _DOMAIN},
        {"asinh", -INFINITY, -1, -1},
        {"asinh", INFINITY, -1, -1},
        {"atan", -INFINITY, -1, -1},
        {"atan", 0, -1, -1},
        {"atan", INFINITY, -1, -1},
        {"atanh", -INFINITY, EDOM, -1},
        {"atanh", -2, EDOM, -1},
        {"atanh", -1, ERANGE, -1},
        {"atanh", 1, ERANGE, -1},
        {"atanh", 2, EDOM, -1},
        {"atanh", INFINITY, EDOM, -1},
        {"cos", -INFINITY, EDOM, _DOMAIN},
        {"cos", INFINITY, EDOM, _DOMAIN},
        {"cosh", -INFINITY, -1, -1},
        {"cosh", 0, -1, -1},
        {"cosh", INFINITY, -1, -1},
        {"exp", -INFINITY, -1, -1},
        {"exp", -1e100, -1, _UNDERFLOW},
        {"exp", 1e100, ERANGE, _OVERFLOW},
        {"exp", INFINITY, -1, -1},
        {"exp2", -INFINITY, -1, -1},
        {"exp2", -1e100, -1, -1},
        {"exp2", 1e100, ERANGE, -1},
        {"exp2", INFINITY, -1, -1},
        {"expm1", -INFINITY, -1, -1},
        {"expm1", INFINITY, -1, -1},
        {"log", -INFINITY, EDOM, _DOMAIN},
        {"log", -1, EDOM, _DOMAIN},
        {"log", 0, ERANGE, _SING},
        {"log", INFINITY, -1, -1},
        {"log10", -INFINITY, EDOM, _DOMAIN},
        {"log10", -1, EDOM, _DOMAIN},
        {"log10", 0, ERANGE, _SING},
        {"log10", INFINITY, -1, -1},
        {"log1p", -INFINITY, EDOM, -1},
        {"log1p", -2, EDOM, -1},
        {"log1p", -1, ERANGE, -1},
        {"log1p", INFINITY, -1, -1},
        {"log2", INFINITY, -1, -1},
        {"sin", -INFINITY, EDOM, _DOMAIN},
        {"sin", INFINITY, EDOM, _DOMAIN},
        {"sinh", -INFINITY, -1, -1},
        {"sinh", 0, -1, -1},
        {"sinh", INFINITY, -1, -1},
        {"sqrt", -INFINITY, EDOM, _DOMAIN},
        {"sqrt", -1, EDOM, _DOMAIN},
        {"sqrt", 0, -1, -1},
        {"sqrt", INFINITY, -1, -1},
        {"tan", -INFINITY, EDOM, _DOMAIN},
        {"tan", -M_PI_2, -1, -1},
        {"tan", M_PI_2, -1, -1},
        {"tan", INFINITY, EDOM, _DOMAIN},
        {"tanh", -INFINITY, -1, -1},
        {"tanh", 0, -1, -1},
        {"tanh", INFINITY, -1, -1},
    };
    const struct {
        char func[16];
        double a;
        double b;
        int error;
        int exception;
    } tests2d[] = {
        {"atan2", -INFINITY, 0, -1, -1},
        {"atan2", 0, 0, -1, -1},
        {"atan2", INFINITY, 0, -1, -1},
        {"atan2", 0, -INFINITY, -1, -1},
        {"atan2", 0, INFINITY, -1, -1},
        {"pow", -INFINITY, -2, -1, -1},
        {"pow", -INFINITY, -1, -1, -1},
        {"pow", -INFINITY, 0, -1, -1},
        {"pow", -INFINITY, 1, -1, -1},
        {"pow", -INFINITY, 2, -1, -1},
        {"pow", -1e100, -10, -1, _UNDERFLOW},
        {"pow", -1e100, 10, ERANGE, _OVERFLOW},
        {"pow", -1, 1.5, EDOM, _DOMAIN},
        {"pow", 0, -2, ERANGE, _SING},
        {"pow", 0, -1, ERANGE, _SING},
        {"pow", 0.5, -INFINITY, -1, -1},
        {"pow", 0.5, INFINITY, -1, -1},
        {"pow", 2, -INFINITY, -1, -1},
        {"pow", 2, -1e100, -1, _UNDERFLOW},
        {"pow", 2, 1e100, ERANGE, _OVERFLOW},
        {"pow", 2, INFINITY, -1, -1},
        {"pow", 1e100, -10, -1, _UNDERFLOW},
        {"pow", 1e100, 10, ERANGE, _OVERFLOW},
        {"pow", INFINITY, -2, -1, -1},
        {"pow", INFINITY, -1, -1, -1},
        {"pow", INFINITY, 0, -1, -1},
        {"pow", INFINITY, 1, -1, -1},
        {"pow", INFINITY, 2, -1, -1},
    };
    const struct {
        char func[16];
        double a;
        double b;
        double c;
        int error;
        int exception;
    } tests3d[] = {
        /* 0 * inf --> EDOM */
        {"fma", INFINITY, 0, 0, EDOM, -1},
        {"fma", 0, INFINITY, 0, EDOM, -1},
        /* inf - inf -> EDOM */
        {"fma", INFINITY, 1, -INFINITY, EDOM, -1},
        {"fma", -INFINITY, 1, INFINITY, EDOM, -1},
        {"fma", 1, INFINITY, -INFINITY, EDOM, -1},
        {"fma", 1, -INFINITY, INFINITY, EDOM, -1},
        /* NaN */
        {"fma", NAN, 0, 0, -1, -1},
        {"fma", 0, NAN, 0, -1, -1},
        {"fma", 0, 0, NAN, -1, -1},
        /* over/underflow */
        {"fma", __port_max_double(), __port_max_double(), __port_max_double(), -1, -1},
        {"fma", __port_min_pos_double(), __port_min_pos_double(), 1, -1, -1},
    };
    const struct {
        char func[16];
        double a;
        long b;
        int error;
        int exception;
    } testsdl[] = {
        {"_scalb", -INFINITY, 1, -1, -1},
        {"_scalb", -1e100, 1, -1, -1},
        {"_scalb", 0, 1, -1, -1},
        {"_scalb", 1e100, 1, -1, -1},
        {"_scalb", INFINITY, 1, -1, -1},
        {"_scalb", 1, 1e9, ERANGE, _OVERFLOW},
        {"ldexp", -INFINITY, 1, -1, -1},
        {"ldexp", -1e100, 1, -1, -1},
        {"ldexp", 0, 1, -1, -1},
        {"ldexp", 1e100, 1, -1, -1},
        {"ldexp", INFINITY, 1, -1, -1},
        {"ldexp", 1, -1e9, -1, _UNDERFLOW},
        {"ldexp", 1, 1e9, ERANGE, _OVERFLOW},
    };
    double (CDECL *p_funcd)(double);
    double (CDECL *p_func2d)(double, double);
    double (CDECL *p_func3d)(double, double, double);
    double (CDECL *p_funcdl)(double, long);
    int i;

    p___setusermatherr(matherr_callback);

    /* necessary so that exp(1e100)==INFINITY on glibc, we can remove this if we change our implementation */
    p_fesetround(FE_TONEAREST);

    for(i = 0; i < ARRAY_SIZE(testsd); i++) {
        p_funcd = (void*)GetProcAddress(module, testsd[i].func);
        *p_errno() = -1;
        exception.type = -1;
        p_funcd(testsd[i].x);
        ok(*p_errno() == testsd[i].error,
           "%s(%f) got errno %d\n", testsd[i].func, testsd[i].x, *p_errno());
        ok(exception.type == testsd[i].exception,
           "%s(%f) got exception type %d\n", testsd[i].func, testsd[i].x, exception.type);
        if(exception.type == -1) continue;
        ok(exception.arg1 == testsd[i].x,
           "%s(%f) got exception arg1 %f\n", testsd[i].func, testsd[i].x, exception.arg1);
    }

    for(i = 0; i < ARRAY_SIZE(tests2d); i++) {
        p_func2d = (void*)GetProcAddress(module, tests2d[i].func);
        *p_errno() = -1;
        exception.type = -1;
        p_func2d(tests2d[i].a, tests2d[i].b);
        ok(*p_errno() == tests2d[i].error,
           "%s(%f, %f) got errno %d\n", tests2d[i].func, tests2d[i].a, tests2d[i].b, *p_errno());
        ok(exception.type == tests2d[i].exception,
           "%s(%f, %f) got exception type %d\n", tests2d[i].func, tests2d[i].a, tests2d[i].b, exception.type);
        if(exception.type == -1) continue;
        ok(exception.arg1 == tests2d[i].a,
           "%s(%f, %f) got exception arg1 %f\n", tests2d[i].func, tests2d[i].a, tests2d[i].b, exception.arg1);
        ok(exception.arg2 == tests2d[i].b,
           "%s(%f, %f) got exception arg2 %f\n", tests2d[i].func, tests2d[i].a, tests2d[i].b, exception.arg2);
    }

    for(i = 0; i < ARRAY_SIZE(tests3d); i++) {
        p_func3d = (void*)GetProcAddress(module, tests3d[i].func);
        *p_errno() = -1;
        exception.type = -1;
        p_func3d(tests3d[i].a, tests3d[i].b, tests3d[i].c);
        ok(*p_errno() == tests3d[i].error,
           "%s(%f, %f, %f) got errno %d\n", tests3d[i].func, tests3d[i].a, tests3d[i].b, tests3d[i].c, *p_errno());
        ok(exception.type == tests3d[i].exception,
           "%s(%f, %f, %f) got exception type %d\n", tests3d[i].func, tests3d[i].a, tests3d[i].b, tests3d[i].c, exception.type);
        if(exception.type == -1) continue;
        ok(exception.arg1 == tests3d[i].a,
           "%s(%f, %f, %f) got exception arg1 %f\n", tests3d[i].func, tests3d[i].a, tests3d[i].b, tests3d[i].c, exception.arg1);
        ok(exception.arg2 == tests3d[i].b,
           "%s(%f, %f, %f) got exception arg2 %f\n", tests3d[i].func, tests3d[i].a, tests3d[i].b, tests3d[i].c, exception.arg2);
    }

    for(i = 0; i < ARRAY_SIZE(testsdl); i++) {
        p_funcdl = (void*)GetProcAddress(module, testsdl[i].func);
        *p_errno() = -1;
        exception.type = -1;
        p_funcdl(testsdl[i].a, testsdl[i].b);
        ok(*p_errno() == testsdl[i].error,
           "%s(%f, %ld) got errno %d\n", testsdl[i].func, testsdl[i].a, testsdl[i].b, *p_errno());
        ok(exception.type == testsdl[i].exception,
           "%s(%f, %ld) got exception type %d\n", testsdl[i].func, testsdl[i].a, testsdl[i].b, exception.type);
        if(exception.type == -1) continue;
        ok(exception.arg1 == testsdl[i].a,
           "%s(%f, %ld) got exception arg1 %f\n", testsdl[i].func, testsdl[i].a, testsdl[i].b, exception.arg1);
        ok(exception.arg2 == testsdl[i].b,
           "%s(%f, %ld) got exception arg2 %f\n", testsdl[i].func, testsdl[i].a, testsdl[i].b, exception.arg2);
    }
}

static void test_asctime(void)
{
    const struct tm epoch = { 0, 0, 0, 1, 0, 70, 4, 0, 0 };
    char *ret;

    if(!p_asctime)
    {
        win_skip("asctime is not available\n");
        return;
    }

    ret = p_asctime(&epoch);
    ok(!strcmp(ret, "Thu Jan  1 00:00:00 1970\n"), "asctime returned %s\n", ret);
}

static void test_strftime(void)
{
    const struct {
       const char *format;
       const char *ret;
       struct tm tm;
       BOOL todo_value;
       BOOL todo_handler;
    } tests[] = {
        {"%C", "", { 0, 0, 0, 1, 0, -2000, 4, 0, 0 }},
        {"%C", "", { 0, 0, 0, 1, 0, -1901, 4, 0, 0 }},
        {"%C", "00", { 0, 0, 0, 1, 0, -1900, 4, 0, 0 }},
        {"%C", "18", { 0, 0, 0, 1, 0, -1, 4, 0, 0 }},
        {"%C", "19", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%C", "99", { 0, 0, 0, 1, 0, 8099, 4, 0, 0 }},
        {"%C", "", { 0, 0, 0, 1, 0, 8100, 4, 0, 0 }},
        {"%d", "", { 0, 0, 0, 0, 0, 70, 4, 0, 0 }},
        {"%d", "01", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%d", "31", { 0, 0, 0, 31, 0, 70, 4, 0, 0 }},
        {"%d", "", { 0, 0, 0, 32, 0, 70, 4, 0, 0 }},
        {"%D", "", { 0, 0, 0, 1, 0, -1901, 4, 0, 0 }},
        {"%D", "01/01/00", { 0, 0, 0, 1, 0, -1900, 4, 0, 0 }},
        {"%D", "01/01/99", { 0, 0, 0, 1, 0, -1, 4, 0, 0 }},
        {"%D", "01/01/70", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%D", "01/01/99", { 0, 0, 0, 1, 0, 8099, 4, 0, 0 }},
        {"%D", "", { 0, 0, 0, 1, 0, 8100, 4, 0, 0 }},
        {"%#D", "1/1/70", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%e", "", { 0, 0, 0, 0, 0, 70, 4, 0, 0 }},
        {"%e", " 1", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%e", "31", { 0, 0, 0, 31, 0, 70, 4, 0, 0 }},
        {"%e", "", { 0, 0, 0, 32, 0, 70, 4, 0, 0 }},
        {"%#e", "1", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%F", "1970-01-01", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%#F", "1970-1-1", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%R", "00:00", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%#R", "0:0", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%T", "00:00:00", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%#T", "0:0:0", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%u", "", { 0, 0, 0, 1, 0, 117, -1, 0, 0 }},
        {"%u", "7", { 0, 0, 0, 1, 0, 117, 0, 0, 0 }},
        {"%u", "1", { 0, 0, 0, 1, 0, 117, 1, 0, 0 }},
        {"%u", "6", { 0, 0, 0, 1, 0, 117, 6, 0, 0 }},
        {"%u", "", { 0, 0, 0, 1, 0, 117, 7, 0, 0 }},
        {"%h", "Jan", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%I", "", { 0, 0, -1, 1, 0, 70, 4, 0, 0 }},
        {"%I", "12", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%I", "01", { 0, 0, 1, 1, 0, 70, 4, 0, 0 }},
        {"%I", "11", { 0, 0, 11, 1, 0, 70, 4, 0, 0 }},
        {"%I", "12", { 0, 0, 12, 1, 0, 70, 4, 0, 0 }},
        {"%I", "01", { 0, 0, 13, 1, 0, 70, 4, 0, 0 }},
        {"%I", "11", { 0, 0, 23, 1, 0, 70, 4, 0, 0 }},
        {"%I", "", { 0, 0, 24, 1, 0, 70, 4, 0, 0 }},
        {"%n", "\n", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%r", "12:00:00 AM", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%r", "02:00:00 PM", { 0, 0, 14, 1, 0, 121, 6, 0, 0 }},
        {"%#r", "12:0:0 AM", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%#r", "2:0:0 PM", { 0, 0, 14, 1, 0, 121, 6, 0, 0 }},
        {"%t", "\t", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%g", "", { 0, 0, 0, 1, 0, -1901, 4, 0, 0 }},
        {"%g", "", { 0, 0, 0, 1, 0, -1901, 3, 364, 0 }},
        {"%g", "00", { 0, 0, 0, 1, 0, -1900, 4, 0, 0 }},
        {"%g", "70", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%g", "71", { 0, 0, 0, 2, 0, 72, 0, 1, 0 }},
        {"%g", "72", { 0, 0, 0, 3, 0, 72, 1, 2, 0 }},
        {"%g", "16", { 0, 0, 0, 1, 0, 117, 0, 0, 0 }},
        {"%g", "99", { 0, 0, 0, 1, 0, 8099, 4, 0, 0 }},
        {"%g", "00", { 0, 0, 0, 1, 0, 8099, 3, 364, 0 }},
        {"%g", "", { 0, 0, 0, 1, 0, 8100, 0, 0, 0 }},
        {"%g", "", { 0, 0, 0, 1, 0, 8100, 4, 0, 0 }},
        {"%G", "1970", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%G", "1971", { 0, 0, 0, 2, 0, 72, 0, 1, 0 }},
        {"%G", "1972", { 0, 0, 0, 3, 0, 72, 1, 2, 0 }},
        {"%G", "2016", { 0, 0, 0, 1, 0, 117, 0, 0, 0 }},
        {"%V", "01", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%V", "52", { 0, 0, 0, 1, 0, 117, 0, 0, 0 }},
        {"%V", "53", { 0, 0, 14, 1, 0, 121, 6, 0, 0 }},
        {"%y", "", { 0, 0, 0, 0, 0, -1901, 0, 0, 0 }},
        {"%y", "00", { 0, 0, 0, 0, 0, -1900, 0, 0, 0 }},
        {"%y", "99", { 0, 0, 0, 0, 0, 8099, 0, 0, 0 }},
        {"%y", "", { 0, 0, 0, 0, 0, 8100, 0, 0, 0 }},
        {"%c", "Thu Jan  1 00:00:00 1970", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%c", "Thu Feb 30 00:00:00 1970", { 0, 0, 0, 30, 1, 70, 4, 0, 0 }},
        {"%#c", "Thursday, January 01, 1970 00:00:00", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%#c", "Thursday, February 30, 1970 00:00:00", { 0, 0, 0, 30, 1, 70, 4, 0, 0 }},
        {"%x", "01/01/70", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%x", "02/30/70", { 0, 0, 0, 30, 1, 70, 4, 0, 0 }},
        {"%#x", "Thursday, January 01, 1970", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%#x", "Thursday, February 30, 1970", { 0, 0, 0, 30, 1, 70, 4, 0, 0 }},
        {"%#x", "", { 0, 0, 0, 30, 1, 70, 7, 0, 0 }},
        {"%#x", "", { 0, 0, 0, 30, 12, 70, 4, 0, 0 }},
        {"%X", "00:00:00", { 0, 0, 0, 1, 0, 70, 4, 0, 0 }},
        {"%X", "14:00:00", { 0, 0, 14, 1, 0, 70, 4, 0, 0 }},
        {"%X", "23:59:60", { 60, 59, 23, 1, 0, 70, 4, 0, 0 }},
    };

    const struct {
        const char *format;
        const char *ret;
        const wchar_t *short_date;
        const wchar_t *date;
        const wchar_t *time;
        struct tm tm;
        BOOL todo;
    } tests_td[] = {
        { "%c", "x z", L"x", L"y", L"z", { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%#c", "y z", L"x", L"y", L"z", { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%X", "M1", 0, 0, L"MMM", { 0, 0, 1, 1, 0, 70, 0, 0, 0 }},
        { "%X", "1", 0, 0, L"h", { 0, 0, 1, 1, 0, 70, 0, 0, 0 }},
        { "%X", "01", 0, 0, L"hh", { 0, 0, 1, 1, 0, 70, 0, 0, 0 }},
        { "%X", "h01", 0, 0, L"hhh", { 0, 0, 1, 1, 0, 70, 0, 0, 0 }},
        { "%X", "hh01", 0, 0, L"hhhh", { 0, 0, 1, 1, 0, 70, 0, 0, 0 }},
        { "%X", "1", 0, 0, L"H", { 0, 0, 1, 1, 0, 70, 0, 0, 0 }},
        { "%X", "01", 0, 0, L"HH", { 0, 0, 1, 1, 0, 70, 0, 0, 0 }},
        { "%X", "H13", 0, 0, L"HHH", { 0, 0, 13, 1, 0, 70, 0, 0, 0 }},
        { "%X", "0", 0, 0, L"m", { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%X", "00", 0, 0, L"mm", { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%X", "m00", 0, 0, L"mmm", { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%X", "0", 0, 0, L"s", { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%X", "00", 0, 0, L"ss", { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%X", "s00", 0, 0, L"sss", { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%X", "T", 0, 0, L"t", { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%X", "TAM", 0, 0, L"tt", { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%X", "TAM", 0, 0, L"ttttttttt", { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%X", "TAM", 0, 0, L"a", { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%X", "TAM", 0, 0, L"aaaaa", { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%X", "TAM", 0, 0, L"A", { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%X", "TAM", 0, 0, L"AAAAA", { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%x", "1", L"d", 0, 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%x", "01", L"dd", 0, 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%x", "D1", L"ddd", 0, 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%x", "Day1", L"dddd", 0, 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%x", "dDay1", L"ddddd", 0, 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%x", "1", L"M", 0, 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%x", "01", L"MM", 0, 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%x", "M1", L"MMM", 0, 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%x", "Mon1", L"MMMM", 0, 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%x", "MMon1", L"MMMMM", 0, 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%x", "y", L"y", 0, 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%x", "70", L"yy", 0, 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%x", "y70", L"yyy", 0, 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%x", "1970", L"yyyy", 0, 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%x", "y1970", L"yyyyy", 0, 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%x", "ggggggggggg", L"ggggggggggg", 0, 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%#x", "1", 0, L"d", 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%#x", "01", 0, L"dd", 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%#x", "D1", 0, L"ddd", 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%#x", "Day1", 0, L"dddd", 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%#x", "dDay1", 0, L"ddddd", 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%#x", "1", 0, L"M", 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%#x", "01", 0, L"MM", 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%#x", "M1", 0, L"MMM", 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%#x", "Mon1", 0, L"MMMM", 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%#x", "MMon1", 0, L"MMMMM", 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%#x", "y", 0, L"y", 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%#x", "70", 0, L"yy", 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%#x", "y70", 0, L"yyy", 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%#x", "1970", 0, L"yyyy", 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%#x", "y1970", 0, L"yyyyy", 0, { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
        { "%r", "z", L"x", L"y", L"z", { 0, 0, 0, 1, 0, 70, 0, 0, 0 }},
    };

    const struct {
        int year;
        int yday;
        const char *ret[7];
    } tests_yweek[] = {
        { 100, 0, { "99 52", "00 01", "00 01", "00 01", "00 01", "99 53", "99 52" }},
        { 100, 1, { "99 52", "00 01", "00 01", "00 01", "00 01", "00 01", "99 53" }},
        { 100, 2, { "99 53", "00 01", "00 01", "00 01", "00 01", "00 01", "00 01" }},
        { 100, 3, { "00 01", "00 01", "00 01", "00 01", "00 01", "00 01", "00 01" }},
        { 100, 4, { "00 01", "00 02", "00 01", "00 01", "00 01", "00 01", "00 01" }},
        { 100, 5, { "00 01", "00 02", "00 02", "00 01", "00 01", "00 01", "00 01" }},
        { 100, 6, { "00 01", "00 02", "00 02", "00 02", "00 01", "00 01", "00 01" }},
        { 100, 358, { "00 51", "00 52", "00 52", "00 52", "00 52", "00 52", "00 51" }},
        { 100, 359, { "00 51", "00 52", "00 52", "00 52", "00 52", "00 52", "00 52" }},
        { 100, 360, { "00 52", "00 52", "00 52", "00 52", "00 52", "00 52", "00 52" }},
        { 100, 361, { "00 52", "00 53", "00 52", "00 52", "00 52", "00 52", "00 52" }},
        { 100, 362, { "00 52", "00 53", "00 53", "00 52", "00 52", "00 52", "00 52" }},
        { 100, 363, { "00 52", "01 01", "00 53", "00 53", "00 52", "00 52", "00 52" }},
        { 100, 364, { "00 52", "01 01", "01 01", "00 53", "00 53", "00 52", "00 52" }},
        { 100, 365, { "00 52", "01 01", "01 01", "01 01", "00 53", "00 53", "00 52" }},
        { 101, 0, { "00 52", "01 01", "01 01", "01 01", "01 01", "00 53", "00 53" }},
        { 101, 1, { "00 53", "01 01", "01 01", "01 01", "01 01", "01 01", "00 53" }},
        { 101, 2, { "00 53", "01 01", "01 01", "01 01", "01 01", "01 01", "01 01" }},
        { 101, 3, { "01 01", "01 01", "01 01", "01 01", "01 01", "01 01", "01 01" }},
        { 101, 4, { "01 01", "01 02", "01 01", "01 01", "01 01", "01 01", "01 01" }},
        { 101, 5, { "01 01", "01 02", "01 02", "01 01", "01 01", "01 01", "01 01" }},
        { 101, 6, { "01 01", "01 02", "01 02", "01 02", "01 01", "01 01", "01 01" }},
        { 101, 358, { "01 51", "01 52", "01 52", "01 52", "01 52", "01 52", "01 51" }},
        { 101, 359, { "01 51", "01 52", "01 52", "01 52", "01 52", "01 52", "01 52" }},
        { 101, 360, { "01 52", "01 52", "01 52", "01 52", "01 52", "01 52", "01 52" }},
        { 101, 361, { "01 52", "01 53", "01 52", "01 52", "01 52", "01 52", "01 52" }},
        { 101, 362, { "01 52", "02 01", "01 53", "01 52", "01 52", "01 52", "01 52" }},
        { 101, 363, { "01 52", "02 01", "02 01", "01 53", "01 52", "01 52", "01 52" }},
        { 101, 364, { "01 52", "02 01", "02 01", "02 01", "01 53", "01 52", "01 52" }},
    };

    __lc_time_data time_data = {
        { "d1", "d2", "d3", "d4", "d5", "d6", "d7" },
        { "day1", "day2", "day3", "day4", "day5", "day6", "day7" },
        { "m1", "m2", "m3", "m4", "m5", "m6", "m7", "m8", "m9", "m10", "m11", "m12" },
        { "mon1", "mon2", "mon3", "mon4", "mon5", "mon6", "mon7", "mon8", "mon9", "mon10", "mon11", "mon12" },
        "tam", "tpm", 0, 0, 0, { 1, 0 },
        { L"D1", L"D2", L"D3", L"D4", L"D5", L"D6", L"D7" },
        { L"Day1", L"Day2", L"Day3", L"Day4", L"Day5", L"Day6", L"Day7" },
        { L"M1", L"M2", L"M3", L"M4", L"M5", L"M6", L"M7", L"M8", L"M9", L"M10", L"M11", L"M12" },
        { L"Mon1", L"Mon2", L"Mon3", L"Mon4", L"Mon5", L"Mon6", L"Mon7", L"Mon8", L"Mon9", L"Mon10", L"Mon11", L"Mon12" },
        L"TAM", L"TPM"
    };

    const struct tm epoch = { 0, 0, 0, 1, 0, 70, 4, 0, 0 };
    struct tm tm_yweek = { 0, 0, 0, 1, 0, 70, 0, 0, 0 };
    char buf[256];
    int i, ret=0;

    for (i=0; i<ARRAY_SIZE(tests); i++)
    {
        todo_wine_if(tests[i].todo_handler) {
            if (!tests[i].ret[0])
                SET_EXPECT(global_invalid_parameter_handler);
            ret = p_strftime(buf, sizeof(buf), tests[i].format, &tests[i].tm);
            if (!tests[i].ret[0])
                CHECK_CALLED(global_invalid_parameter_handler);
        }

        todo_wine_if(tests[i].todo_value) {
            ok(ret == strlen(tests[i].ret), "%d) ret = %d\n", i, ret);
            ok(!strcmp(buf, tests[i].ret), "%d) buf = \"%s\", expected \"%s\"\n",
                    i, buf, tests[i].ret);
        }
    }

    ret = p_strftime(buf, sizeof(buf), "%z", &epoch);
    ok(ret == 5, "expected 5, got %d\n", ret);
    ok((buf[0] == '+' || buf[0] == '-') &&
        isdigit(buf[1]) && isdigit(buf[2]) &&
        isdigit(buf[3]) && isdigit(buf[4]), "got %s\n", buf);

    for (i=0; i<ARRAY_SIZE(tests_td); i++)
    {
        time_data.short_dateW = tests_td[i].short_date;
        time_data.dateW = tests_td[i].date;
        time_data.timeW = tests_td[i].time;
        ret = p__Strftime(buf, sizeof(buf), tests_td[i].format, &tests_td[i].tm, &time_data);
        ok(ret == strlen(buf), "%d) ret = %d\n", i, ret);
        todo_wine_if(tests_td[i].todo) {
            ok(!strcmp(buf, tests_td[i].ret), "%d) buf = \"%s\", expected \"%s\"\n",
                    i, buf, tests_td[i].ret);
        }
    }

    for (i=0; i<ARRAY_SIZE(tests_yweek); i++)
    {
        int j;
        tm_yweek.tm_year = tests_yweek[i].year;
        tm_yweek.tm_yday = tests_yweek[i].yday;
        for (j=0; j<7; j++)
        {
            tm_yweek.tm_wday = j;
            p_strftime(buf, sizeof(buf), "%g %V", &tm_yweek);
            ok(!strcmp(buf, tests_yweek[i].ret[j]), "%d,%d) buf = \"%s\", expected \"%s\"\n",
                    i, j, buf, tests_yweek[i].ret[j]);
        }
    }

    if(!p_setlocale(LC_ALL, "fr-FR")) {
        win_skip("fr-FR locale not available\n");
        return;
    }
    ret = p_strftime(buf, sizeof(buf), "%c", &epoch);
    ok(ret == 19, "ret = %d\n", ret);
    ok(!strcmp(buf, "01/01/1970 00:00:00"), "buf = \"%s\", expected \"%s\"\n", buf, "01/01/1970 00:00:00");
    ret = p_strftime(buf, sizeof(buf), "%r", &epoch);
    ok(ret == 8, "ret = %d\n", ret);
    ok(!strcmp(buf, "00:00:00"), "buf = \"%s\", expected \"%s\"\n", buf, "00:00:00");
    p_setlocale(LC_ALL, "C");
}

static LONG* get_failures_counter(HANDLE *map)
{
    *map = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
            0, sizeof(LONG), "winetest_failures_counter");
    return MapViewOfFile(*map, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(LONG));
}

static void free_failures_counter(LONG *mem, HANDLE map)
{
    UnmapViewOfFile(mem);
    CloseHandle(map);
}

static void set_failures_counter(LONG add)
{
    HANDLE failures_map;
    LONG *failures;

    failures = get_failures_counter(&failures_map);
    *failures = add;
    free_failures_counter(failures, failures_map);
}

static void test_exit(const char *argv0)
{
    PROCESS_INFORMATION proc;
    STARTUPINFOA startup = {0};
    char path[MAX_PATH];
    HANDLE failures_map, exit_event, quick_exit_event;
    LONG *failures;
    DWORD ret;

    exit_event = CreateEventA(NULL, FALSE, FALSE, "exit_event");
    quick_exit_event = CreateEventA(NULL, FALSE, FALSE, "quick_exit_event");

    failures = get_failures_counter(&failures_map);
    sprintf(path, "%s misc exit", argv0);
    startup.cb = sizeof(startup);
    CreateProcessA(NULL, path, NULL, NULL, TRUE, 0, NULL, NULL, &startup, &proc);
    ret = WaitForSingleObject(proc.hProcess, INFINITE);
    ok(ret == WAIT_OBJECT_0, "child process wait failed\n");
    GetExitCodeProcess(proc.hProcess, &ret);
    ok(ret == 1, "child process exited with code %d\n", ret);
    CloseHandle(proc.hProcess);
    CloseHandle(proc.hThread);
    ok(!*failures, "%d tests failed in child process\n", *failures);
    free_failures_counter(failures, failures_map);


    ret = WaitForSingleObject(exit_event, 0);
    ok(ret == WAIT_OBJECT_0, "exit_event was not set (%x)\n", ret);
    ret = WaitForSingleObject(quick_exit_event, 0);
    ok(ret == WAIT_TIMEOUT, "quick_exit_event should not have be set (%x)\n", ret);

    CloseHandle(exit_event);
    CloseHandle(quick_exit_event);
}

static int atexit_called;

static void CDECL at_exit_func1(void)
{
    HANDLE exit_event = CreateEventA(NULL, FALSE, FALSE, "exit_event");

    ok(exit_event != NULL, "CreateEvent failed: %d\n", GetLastError());
    ok(atexit_called == 1, "atexit_called = %d\n", atexit_called);
    atexit_called++;
    SetEvent(exit_event);
    CloseHandle(exit_event);
    set_failures_counter(winetest_get_failures());
}

static void CDECL at_exit_func2(void)
{
    ok(!atexit_called, "atexit_called = %d\n", atexit_called);
    atexit_called++;
    set_failures_counter(winetest_get_failures());
}

static int atquick_exit_called;

static void CDECL at_quick_exit_func1(void)
{
    HANDLE quick_exit_event = CreateEventA(NULL, FALSE, FALSE, "quick_exit_event");

    ok(quick_exit_event != NULL, "CreateEvent failed: %d\n", GetLastError());
    ok(atquick_exit_called == 1, "atquick_exit_called = %d\n", atquick_exit_called);
    atquick_exit_called++;
    SetEvent(quick_exit_event);
    CloseHandle(quick_exit_event);
    set_failures_counter(winetest_get_failures());
}

static void CDECL at_quick_exit_func2(void)
{
    ok(!atquick_exit_called, "atquick_exit_called = %d\n", atquick_exit_called);
    atquick_exit_called++;
    set_failures_counter(winetest_get_failures());
}

static void test_call_exit(void)
{
    ok(!p__crt_atexit(at_exit_func1), "_crt_atexit failed\n");
    ok(!p__crt_atexit(at_exit_func2), "_crt_atexit failed\n");

    ok(!p_crt_at_quick_exit(at_quick_exit_func1), "_crt_at_quick_exit failed\n");
    ok(!p_crt_at_quick_exit(at_quick_exit_func2), "_crt_at_quick_exit failed\n");

    set_failures_counter(winetest_get_failures());
    p_exit(1);
}

static void test_call_quick_exit(void)
{
    ok(!p__crt_atexit(at_exit_func1), "_crt_atexit failed\n");
    ok(!p__crt_atexit(at_exit_func2), "_crt_atexit failed\n");

    ok(!p_crt_at_quick_exit(at_quick_exit_func1), "_crt_at_quick_exit failed\n");
    ok(!p_crt_at_quick_exit(at_quick_exit_func2), "_crt_at_quick_exit failed\n");

    set_failures_counter(winetest_get_failures());
    p_quick_exit(2);
}

static void test_quick_exit(const char *argv0)
{
    PROCESS_INFORMATION proc;
    STARTUPINFOA startup = {0};
    char path[MAX_PATH];
    HANDLE failures_map, exit_event, quick_exit_event;
    LONG *failures;
    DWORD ret;

    exit_event = CreateEventA(NULL, FALSE, FALSE, "exit_event");
    quick_exit_event = CreateEventA(NULL, FALSE, FALSE, "quick_exit_event");

    failures = get_failures_counter(&failures_map);
    sprintf(path, "%s misc quick_exit", argv0);
    startup.cb = sizeof(startup);
    CreateProcessA(NULL, path, NULL, NULL, TRUE, 0, NULL, NULL, &startup, &proc);
    ret = WaitForSingleObject(proc.hProcess, INFINITE);
    ok(ret == WAIT_OBJECT_0, "child process wait failed\n");
    GetExitCodeProcess(proc.hProcess, &ret);
    ok(ret == 2, "child process exited with code %d\n", ret);
    CloseHandle(proc.hProcess);
    CloseHandle(proc.hThread);
    ok(!*failures, "%d tests failed in child process\n", *failures);
    free_failures_counter(failures, failures_map);

    ret = WaitForSingleObject(quick_exit_event, 0);
    ok(ret == WAIT_OBJECT_0, "quick_exit_event was not set (%x)\n", ret);
    ret = WaitForSingleObject(exit_event, 0);
    ok(ret == WAIT_TIMEOUT, "exit_event should not have be set (%x)\n", ret);

    CloseHandle(exit_event);
    CloseHandle(quick_exit_event);
}

static void test__stat32(void)
{
    static const char test_file[] = "\\stat_file.tst";
    static const char test_dir[] = "\\stat_dir.tst";

    char path[2*MAX_PATH];
    struct _stat32 buf;
    int fd, ret;
    DWORD len;

    len = GetTempPathA(MAX_PATH, path);
    ok(len, "GetTempPathA failed\n");

    ret = p__stat32("c:", &buf);
    ok(ret == -1, "_stat32('c:') returned %d\n", ret);
    ret = p__stat32("c:\\", &buf);
    ok(!ret, "_stat32('c:\\') returned %d\n", ret);

    memcpy(path+len, test_file, sizeof(test_file));
    if((fd = open(path, O_WRONLY | O_CREAT | O_BINARY, _S_IREAD |_S_IWRITE)) >= 0)
    {
        ret = p__stat32(path, &buf);
        ok(!ret, "_stat32('%s') returned %d\n", path, ret);
        strcat(path, "\\");
        ret = p__stat32(path, &buf);
        todo_wine ok(ret, "_stat32('%s') returned %d\n", path, ret);
        close(fd);
        remove(path);
    }

    memcpy(path+len, test_dir, sizeof(test_dir));
    if(!mkdir(path))
    {
        ret = p__stat32(path, &buf);
        ok(!ret, "_stat32('%s') returned %d\n", path, ret);
        strcat(path, "\\");
        ret = p__stat32(path, &buf);
        ok(!ret, "_stat32('%s') returned %d\n", path, ret);
        rmdir(path);
    }
}

static void test__o_malloc(void)
{
    void *m;
    size_t s;

    m = p__o_malloc(1);
    ok(m != NULL, "p__o_malloc(1) returned NULL\n");

    s = p__msize(m);
    ok(s == 1, "_msize returned %d\n", (int)s);

    p_free(m);
}

static void test_clock(void)
{
    static const int thresh = 100;
    int c, expect_min, expect_max;
    FILETIME cur;

    GetSystemTimeAsFileTime(&cur);
    c = p_clock();

    expect_min = (((LONGLONG)cur.dwHighDateTime << 32) + cur.dwLowDateTime - crt_init_end) / 10000;
    expect_max = (((LONGLONG)cur.dwHighDateTime << 32) + cur.dwLowDateTime - crt_init_start) / 10000;
    ok(c > expect_min-thresh && c < expect_max+thresh, "clock() = %d, expected range [%d, %d]\n",
            c, expect_min, expect_max);
}

START_TEST(misc)
{
    int arg_c;
    char** arg_v;

    if(!init())
        return;

    arg_c = winetest_get_mainargs(&arg_v);
    if(arg_c == 3) {
        if(!strcmp(arg_v[2], "cmd"))
            test__get_narrow_winmain_command_line(NULL);
        else if(!strcmp(arg_v[2], "exit"))
            test_call_exit();
        else if(!strcmp(arg_v[2], "quick_exit"))
            test_call_quick_exit();
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
    test_lldiv();
    test_isblank();
    test_math_errors();
    test_asctime();
    test_strftime();
    test_exit(arg_v[0]);
    test_quick_exit(arg_v[0]);
    test__stat32();
    test__o_malloc();
    test_clock();
}
