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
#include <locale.h>

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
static size_t (__cdecl *p___strncnt)(const char*, size_t);
static int (__cdecl *p_towlower)(wint_t);
static int (__cdecl *p__towlower_l)(wint_t, _locale_t);
static int (__cdecl *p_towupper)(wint_t);
static int (__cdecl *p__towupper_l)(wint_t, _locale_t);
static char* (__cdecl *p_setlocale)(int, const char*);
static _locale_t (__cdecl *p__create_locale)(int, const char*);
static void (__cdecl *p__free_locale)(_locale_t);

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
    p___strncnt = (void*)GetProcAddress(module, "__strncnt");
    p_towlower = (void*)GetProcAddress(module, "towlower");
    p__towlower_l = (void*)GetProcAddress(module, "_towlower_l");
    p_towupper = (void*)GetProcAddress(module, "towupper");
    p__towupper_l = (void*)GetProcAddress(module, "_towupper_l");
    p_setlocale = (void*)GetProcAddress(module, "setlocale");
    p__create_locale = (void*)GetProcAddress(module, "_create_locale");
    p__free_locale = (void*)GetProcAddress(module, "_free_locale");
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


static void test___strncnt(void)
{
    static const struct
    {
        const char *str;
        size_t size;
        size_t ret;
    }
    strncnt_tests[] =
    {
        { "a", 0, 0 },
        { "a", 1, 1 },
        { "a", 10, 1 },
        { "abc", 1, 1 },
    };
    unsigned int i;
    size_t ret;

    for (i = 0; i < ARRAY_SIZE(strncnt_tests); ++i)
    {
        ret = p___strncnt(strncnt_tests[i].str, strncnt_tests[i].size);
        ok(ret == strncnt_tests[i].ret, "%u: unexpected return value %u.\n", i, (int)ret);
    }

    ok(p_set_invalid_parameter_handler(test_invalid_parameter_handler) == NULL,
            "Invalid parameter handler was already set\n");

    if (0) /* crashes */
    {
        ret = p___strncnt(NULL, 0);
        ret = p___strncnt(NULL, 1);
    }

    ok(p_set_invalid_parameter_handler(NULL) == test_invalid_parameter_handler,
            "Cannot reset invalid parameter handler\n");
}

static void test_C_locale(void)
{
    int i, j;
    wint_t ret, exp;
    _locale_t locale;
    static const char *locales[] = { NULL, "C" };

    /* C locale only converts case for [a-zA-Z] */
    p_setlocale(LC_ALL, "C");
    for (i = 0; i <= 0xffff; i++)
    {
        ret = p_towlower(i);
        if (i >= 'A' && i <= 'Z')
        {
            exp = i + 'a' - 'A';
            ok(ret == exp, "expected %x, got %x for C locale\n", exp, ret);
        }
        else
            ok(ret == i, "expected self %x, got %x for C locale\n", i, ret);

        ret = p_towupper(i);
        if (i >= 'a' && i <= 'z')
        {
            exp = i + 'A' - 'a';
            ok(ret == exp, "expected %x, got %x for C locale\n", exp, ret);
        }
        else
            ok(ret == i, "expected self %x, got %x for C locale\n", i, ret);
    }

    for (i = 0; i < ARRAY_SIZE(locales); i++) {
        locale = locales[i] ? p__create_locale(LC_ALL, locales[i]) : NULL;

        for (j = 0; j <= 0xffff; j++) {
            ret = p__towlower_l(j, locale);
            if (j >= 'A' && j <= 'Z')
            {
                exp = j + 'a' - 'A';
                ok(ret == exp, "expected %x, got %x for C locale\n", exp, ret);
            }
            else
                ok(ret == j, "expected self %x, got %x for C locale\n", j, ret);

            ret = p__towupper_l(j, locale);
            if (j >= 'a' && j <= 'z')
            {
                exp = j + 'A' - 'a';
                ok(ret == exp, "expected %x, got %x for C locale\n", exp, ret);
            }
            else
                ok(ret == j, "expected self %x, got %x for C locale\n", j, ret);
        }

        p__free_locale(locale);
    }
}

START_TEST(string)
{
    if (!init()) return;
    test_strtod();
    test__memicmp();
    test__memicmp_l();
    test___strncnt();
    test_C_locale();
}
