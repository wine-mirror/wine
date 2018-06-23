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
#include <time.h>

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

static inline float __port_infinity(void)
{
    static const unsigned __inf_bytes = 0x7f800000;
    return *(const float *)&__inf_bytes;
}
#define INFINITY __port_infinity()

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

typedef int (CDECL *MSVCRT_matherr_func)(struct MSVCRT__exception *);

static HMODULE module;

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
static void (CDECL *p_exit)(int);
static int (CDECL *p__crt_atexit)(void (CDECL*)(void));

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
    module = LoadLibraryA("ucrtbase.dll");

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
    p__crt_atexit = (void*)GetProcAddress(module, "_crt_atexit");
    p_exit = (void*)GetProcAddress(module, "exit");

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
    ok(ret == EINVAL, "got %d\n", ret);
    ok(fd == -1, "got fd %d\n", fd);
    CHECK_CALLED(global_invalid_parameter_handler);
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
    ok(ret == EINVAL, "got %d\n", ret);
    ok(fd == -1, "got fd %d\n", fd);
    CHECK_CALLED(global_invalid_parameter_handler);
    p__set_invalid_parameter_handler(NULL);

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
        long b;
        int error;
        int exception;
    } testsdl[] = {
        {"_scalb", -INFINITY, 1, -1, -1},
        {"_scalb", -1e100, 1, -1, -1},
        {"_scalb", 1e100, 1, -1, -1},
        {"_scalb", INFINITY, 1, -1, -1},
        {"_scalb", 1, 1e9, ERANGE, _OVERFLOW},
        {"ldexp", -INFINITY, 1, -1, -1},
        {"ldexp", -1e100, 1, -1, -1},
        {"ldexp", 1e100, 1, -1, -1},
        {"ldexp", INFINITY, 1, -1, -1},
        {"ldexp", 1, -1e9, -1, _UNDERFLOW},
        {"ldexp", 1, 1e9, ERANGE, _OVERFLOW},
    };
    double (CDECL *p_funcd)(double);
    double (CDECL *p_func2d)(double, double);
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

static void test_exit(const char *argv0)
{
    PROCESS_INFORMATION proc;
    STARTUPINFOA startup = {0};
    char path[MAX_PATH];
    HANDLE exit_event;
    DWORD ret;

    exit_event = CreateEventA(NULL, FALSE, FALSE, "exit_event");

    sprintf(path, "%s misc exit", argv0);
    startup.cb = sizeof(startup);
    CreateProcessA(NULL, path, NULL, NULL, TRUE, 0, NULL, NULL, &startup, &proc);
    winetest_wait_child_process(proc.hProcess);

    ret = WaitForSingleObject(exit_event, 0);
    ok(ret == WAIT_OBJECT_0, "exit_event was not set (%x)\n", ret);

    CloseHandle(exit_event);
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
}

static void CDECL at_exit_func2(void)
{
    ok(!atexit_called, "atexit_called = %d\n", atexit_called);
    atexit_called++;
}

static void test_call_exit(void)
{
    ok(!p__crt_atexit(at_exit_func1), "_crt_atexit failed\n");
    ok(!p__crt_atexit(at_exit_func2), "_crt_atexit failed\n");
    p_exit(0);
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
    test_exit(arg_v[0]);
}
