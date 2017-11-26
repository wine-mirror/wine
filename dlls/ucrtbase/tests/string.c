/*
 * Copyright 2015 Martin Storsjo
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

#include <windef.h>
#include <winbase.h>
#include "wine/test.h"

#include <math.h>

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

DEFINE_EXPECT(invalid_parameter_handler);

static _invalid_parameter_handler (__cdecl *p_set_invalid_parameter_handler)(_invalid_parameter_handler);

#ifndef INFINITY
static inline float __port_infinity(void)
{
    static const unsigned __inf_bytes = 0x7f800000;
    return *(const float *)&__inf_bytes;
}
#define INFINITY __port_infinity()
#endif

#ifndef NAN
static inline float __port_nan(void)
{
    static const unsigned __nan_bytes = 0x7fc00000;
    return *(const float *)&__nan_bytes;
}
#define NAN __port_nan()
#endif

static void __cdecl test_invalid_parameter_handler(const wchar_t *expression,
        const wchar_t *function, const wchar_t *file,
        unsigned line, uintptr_t arg)
{
    CHECK_EXPECT(invalid_parameter_handler);
    ok(expression == NULL, "expression is not NULL\n");
    ok(function == NULL, "function is not NULL\n");
    ok(file == NULL, "file is not NULL\n");
    ok(line == 0, "line = %u\n", line);
    ok(arg == 0, "arg = %lx\n", (UINT_PTR)arg);
}

static double (__cdecl *p_strtod)(const char*, char** end);
static int (__cdecl *p__memicmp)(const char*, const char*, size_t);
static int (__cdecl *p__memicmp_l)(const char*, const char*, size_t,_locale_t);

static BOOL init(void)
{
    HMODULE module;

    module = LoadLibraryA("ucrtbase.dll");
    if (!module)
    {
        win_skip("ucrtbase.dll not installed\n");
        return FALSE;
    }

    p_set_invalid_parameter_handler = (void*)GetProcAddress(module, "_set_invalid_parameter_handler");
    p_strtod = (void*)GetProcAddress(module, "strtod");
    p__memicmp = (void*)GetProcAddress(module, "_memicmp");
    p__memicmp_l = (void*)GetProcAddress(module, "_memicmp_l");
    return TRUE;
}

static BOOL local_isnan(double d)
{
    return d != d;
}

#define test_strtod_str(string, value, length) _test_strtod_str(__LINE__, string, value, length)
static void _test_strtod_str(int line, const char* string, double value, int length)
{
    char *end;
    double d;
    d = p_strtod(string, &end);
    if (local_isnan(value))
        ok_(__FILE__, line)(local_isnan(d), "d = %lf (\"%s\")\n", d, string);
    else
        ok_(__FILE__, line)(d == value, "d = %lf (\"%s\")\n", d, string);
    ok_(__FILE__, line)(end == string + length, "incorrect end (%d, \"%s\")\n", (int)(end - string), string);
}

static void test_strtod(void)
{
    test_strtod_str("infinity", INFINITY, 8);
    test_strtod_str("INFINITY", INFINITY, 8);
    test_strtod_str("InFiNiTy", INFINITY, 8);
    test_strtod_str("INF", INFINITY, 3);
    test_strtod_str("-inf", -INFINITY, 4);
    test_strtod_str("inf42", INFINITY, 3);
    test_strtod_str("inffoo", INFINITY, 3);
    test_strtod_str("infini", INFINITY, 3);

    test_strtod_str("NAN", NAN, 3);
    test_strtod_str("nan", NAN, 3);
    test_strtod_str("NaN", NAN, 3);

    test_strtod_str("0x42", 66, 4);
    test_strtod_str("0X42", 66, 4);
    test_strtod_str("-0x42", -66, 5);
    test_strtod_str("0x1p1", 2, 5);
    test_strtod_str("0x1P1", 2, 5);
    test_strtod_str("0x1p+1", 2, 6);
    test_strtod_str("0x2p-1", 1, 6);
    test_strtod_str("0xA", 10, 3);
    test_strtod_str("0xa", 10, 3);
    test_strtod_str("0xABCDEF", 11259375, 8);
    test_strtod_str("0Xabcdef", 11259375, 8);

    test_strtod_str("0x1.1", 1.0625, 5);
    test_strtod_str("0x1.1p1", 2.125, 7);
    test_strtod_str("0x1.A", 1.625, 5);
    test_strtod_str("0x1p1a", 2, 5);
}

static void test__memicmp(void)
{
    static const char *s1 = "abc";
    static const char *s2 = "aBd";
    int ret;

    ok(p_set_invalid_parameter_handler(test_invalid_parameter_handler) == NULL,
            "Invalid parameter handler was already set\n");

    ret = p__memicmp(NULL, NULL, 0);
    ok(!ret, "got %d\n", ret);

    SET_EXPECT(invalid_parameter_handler);
    errno = 0xdeadbeef;
    ret = p__memicmp(NULL, NULL, 1);
    ok(ret == _NLSCMPERROR, "got %d\n", ret);
    ok(errno == 0xdeadbeef, "Unexpected errno = %d\n", errno);
    CHECK_CALLED(invalid_parameter_handler);

    SET_EXPECT(invalid_parameter_handler);
    errno = 0xdeadbeef;
    ret = p__memicmp(s1, NULL, 1);
    ok(ret == _NLSCMPERROR, "got %d\n", ret);
    ok(errno == 0xdeadbeef, "Unexpected errno = %d\n", errno);
    CHECK_CALLED(invalid_parameter_handler);

    SET_EXPECT(invalid_parameter_handler);
    errno = 0xdeadbeef;
    ret = p__memicmp(NULL, s2, 1);
    ok(ret == _NLSCMPERROR, "got %d\n", ret);
    ok(errno == 0xdeadbeef, "Unexpected errno = %d\n", errno);
    CHECK_CALLED(invalid_parameter_handler);

    ret = p__memicmp(s1, s2, 2);
    ok(!ret, "got %d\n", ret);

    ret = p__memicmp(s1, s2, 3);
    ok(ret == -1, "got %d\n", ret);

    ok(p_set_invalid_parameter_handler(NULL) == test_invalid_parameter_handler,
            "Cannot reset invalid parameter handler\n");
}

static void test__memicmp_l(void)
{
    static const char *s1 = "abc";
    static const char *s2 = "aBd";
    int ret;

    ok(p_set_invalid_parameter_handler(test_invalid_parameter_handler) == NULL,
            "Invalid parameter handler was already set\n");

    ret = p__memicmp_l(NULL, NULL, 0, NULL);
    ok(!ret, "got %d\n", ret);

    SET_EXPECT(invalid_parameter_handler);
    errno = 0xdeadbeef;
    ret = p__memicmp_l(NULL, NULL, 1, NULL);
    ok(ret == _NLSCMPERROR, "got %d\n", ret);
    ok(errno == 0xdeadbeef, "Unexpected errno = %d\n", errno);
    CHECK_CALLED(invalid_parameter_handler);

    SET_EXPECT(invalid_parameter_handler);
    errno = 0xdeadbeef;
    ret = p__memicmp_l(s1, NULL, 1, NULL);
    ok(ret == _NLSCMPERROR, "got %d\n", ret);
    ok(errno == 0xdeadbeef, "Unexpected errno = %d\n", errno);
    CHECK_CALLED(invalid_parameter_handler);

    SET_EXPECT(invalid_parameter_handler);
    errno = 0xdeadbeef;
    ret = p__memicmp_l(NULL, s2, 1, NULL);
    ok(ret == _NLSCMPERROR, "got %d\n", ret);
    ok(errno == 0xdeadbeef, "Unexpected errno = %d\n", errno);
    CHECK_CALLED(invalid_parameter_handler);

    ret = p__memicmp_l(s1, s2, 2, NULL);
    ok(!ret, "got %d\n", ret);

    ret = p__memicmp_l(s1, s2, 3, NULL);
    ok(ret == -1, "got %d\n", ret);

    ok(p_set_invalid_parameter_handler(NULL) == test_invalid_parameter_handler,
            "Cannot reset invalid parameter handler\n");
}
START_TEST(string)
{
    if (!init()) return;
    test_strtod();
    test__memicmp();
    test__memicmp_l();
}
