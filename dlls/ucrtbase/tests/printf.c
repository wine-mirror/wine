/*
 * Conformance tests for *printf functions.
 *
 * Copyright 2002 Uwe Bonnes
 * Copyright 2004 Aneurin Price
 * Copyright 2005 Mike McCormack
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

#include <stdio.h>
#include <errno.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"

#include "wine/test.h"

static int (__cdecl *p_vfprintf)(unsigned __int64 options, FILE *file, const char *format,
                                 void *locale, __ms_va_list valist);
static int (__cdecl *p_vsprintf)(unsigned __int64 options, char *str, size_t len, const char *format,
                                 void *locale, __ms_va_list valist);
static int (__cdecl *p_vsnprintf_s)(unsigned __int64 options, char *str, size_t sizeOfBuffer, size_t count, const char *format,
                                    void *locale, __ms_va_list valist);
static int (__cdecl *p_vsprintf_s)(unsigned __int64 options, char *str, size_t count, const char *format,
                                    void *locale, __ms_va_list valist);
static int (__cdecl *p_vswprintf)(unsigned __int64 options, wchar_t *str, size_t len, const wchar_t *format,
                                  void *locale, __ms_va_list valist);

static FILE *(__cdecl *p_fopen)(const char *name, const char *mode);
static int (__cdecl *p_fclose)(FILE *file);
static long (__cdecl *p_ftell)(FILE *file);
static char *(__cdecl *p_fgets)(char *str, int size, FILE *file);

static BOOL init( void )
{
    HMODULE hmod = LoadLibraryA("ucrtbase.dll");

    if (!hmod)
    {
        win_skip("ucrtbase.dll not installed\n");
        return FALSE;
    }

    p_vfprintf = (void *)GetProcAddress(hmod, "__stdio_common_vfprintf");
    p_vsprintf = (void *)GetProcAddress(hmod, "__stdio_common_vsprintf");
    p_vsnprintf_s = (void *)GetProcAddress(hmod, "__stdio_common_vsnprintf_s");
    p_vsprintf_s = (void *)GetProcAddress(hmod, "__stdio_common_vsprintf_s");
    p_vswprintf = (void *)GetProcAddress(hmod, "__stdio_common_vswprintf");

    p_fopen = (void *)GetProcAddress(hmod, "fopen");
    p_fclose = (void *)GetProcAddress(hmod, "fclose");
    p_ftell = (void *)GetProcAddress(hmod, "ftell");
    p_fgets = (void *)GetProcAddress(hmod, "fgets");
    return TRUE;
}

static int __cdecl vsprintf_wrapper(unsigned __int64 options, char *str,
                                    size_t len, const char *format, ...)
{
    int ret;
    __ms_va_list valist;
    __ms_va_start(valist, format);
    ret = p_vsprintf(options, str, len, format, NULL, valist);
    __ms_va_end(valist);
    return ret;
}

static void test_snprintf (void)
{
    struct snprintf_test {
        const char *format;
        int expected;
    };
    /* Legacy behaviour, not C99 compliant. */
    const struct snprintf_test tests_legacy[] = {{"short", 5},
                                                 {"justfit", 7},
                                                 {"justfits", 8},
                                                 {"muchlonger", -1}};
    /* New C99 compliant behaviour. */
    const struct snprintf_test tests_standard[] = {{"short", 5},
                                                   {"justfit", 7},
                                                   {"justfits", 8},
                                                   {"muchlonger", 10}};
    char buffer[8];
    const int bufsiz = sizeof buffer;
    unsigned int i;

    for (i = 0; i < sizeof tests_legacy / sizeof tests_legacy[0]; i++) {
        const char *fmt  = tests_legacy[i].format;
        const int expect = tests_legacy[i].expected;
        const int n      = vsprintf_wrapper (1, buffer, bufsiz, fmt);
        const int valid  = n < 0 ? bufsiz : (n == bufsiz ? n : n+1);

        ok (n == expect, "\"%s\": expected %d, returned %d\n",
            fmt, expect, n);
        ok (!memcmp (fmt, buffer, valid),
            "\"%s\": rendered \"%.*s\"\n", fmt, valid, buffer);
    }

    for (i = 0; i < sizeof tests_standard / sizeof tests_standard[0]; i++) {
        const char *fmt  = tests_standard[i].format;
        const int expect = tests_standard[i].expected;
        const int n      = vsprintf_wrapper (2, buffer, bufsiz, fmt);
        const int valid  = n >= bufsiz ? bufsiz - 1 : n < 0 ? 0 : n;

        ok (n == expect, "\"%s\": expected %d, returned %d\n",
            fmt, expect, n);
        ok (!memcmp (fmt, buffer, valid),
            "\"%s\": rendered \"%.*s\"\n", fmt, valid, buffer);
        ok (buffer[valid] == '\0',
            "\"%s\": Missing null termination (ret %d) - is %d\n", fmt, n, buffer[valid]);
    }
}

static int __cdecl vswprintf_wrapper(unsigned __int64 options, wchar_t *str,
                                     size_t len, const wchar_t *format, ...)
{
    int ret;
    __ms_va_list valist;
    __ms_va_start(valist, format);
    ret = p_vswprintf(options, str, len, format, NULL, valist);
    __ms_va_end(valist);
    return ret;
}
static void test_swprintf (void)
{
    struct swprintf_test {
        const wchar_t *format;
        int expected;
    };
    const wchar_t str_short[]      = {'s','h','o','r','t',0};
    const wchar_t str_justfit[]    = {'j','u','s','t','f','i','t',0};
    const wchar_t str_justfits[]   = {'j','u','s','t','f','i','t','s',0};
    const wchar_t str_muchlonger[] = {'m','u','c','h','l','o','n','g','e','r',0};
    /* Legacy behaviour, not C99 compliant. */
    const struct swprintf_test tests_legacy[] = {{str_short, 5},
                                                 {str_justfit, 7},
                                                 {str_justfits, 8},
                                                 {str_muchlonger, -1}};
    /* New C99 compliant behaviour. */
    const struct swprintf_test tests_standard[] = {{str_short, 5},
                                                   {str_justfit, 7},
                                                   {str_justfits, 8},
                                                   {str_muchlonger, 10}};
    wchar_t buffer[8];
    char narrow[8], narrow_fmt[16];
    const int bufsiz = sizeof buffer / sizeof buffer[0];
    unsigned int i;

    for (i = 0; i < sizeof tests_legacy / sizeof tests_legacy[0]; i++) {
        const wchar_t *fmt = tests_legacy[i].format;
        const int expect   = tests_legacy[i].expected;
        const int n        = vswprintf_wrapper (1, buffer, bufsiz, fmt);
        const int valid    = n < 0 ? bufsiz : (n == bufsiz ? n : n+1);

        WideCharToMultiByte (CP_ACP, 0, buffer, -1, narrow, sizeof(narrow), NULL, NULL);
        WideCharToMultiByte (CP_ACP, 0, fmt, -1, narrow_fmt, sizeof(narrow_fmt), NULL, NULL);
        ok (n == expect, "\"%s\": expected %d, returned %d\n",
            narrow_fmt, expect, n);
        ok (!memcmp (fmt, buffer, valid * sizeof(wchar_t)),
            "\"%s\": rendered \"%.*s\"\n", narrow_fmt, valid, narrow);
    }

    for (i = 0; i < sizeof tests_standard / sizeof tests_standard[0]; i++) {
        const wchar_t *fmt = tests_standard[i].format;
        const int expect   = tests_standard[i].expected;
        const int n        = vswprintf_wrapper (2, buffer, bufsiz, fmt);
        const int valid    = n >= bufsiz ? bufsiz - 1 : n < 0 ? 0 : n;

        WideCharToMultiByte (CP_ACP, 0, buffer, -1, narrow, sizeof(narrow), NULL, NULL);
        WideCharToMultiByte (CP_ACP, 0, fmt, -1, narrow_fmt, sizeof(narrow_fmt), NULL, NULL);
        ok (n == expect, "\"%s\": expected %d, returned %d\n",
            narrow_fmt, expect, n);
        ok (!memcmp (fmt, buffer, valid * sizeof(wchar_t)),
            "\"%s\": rendered \"%.*s\"\n", narrow_fmt, valid, narrow);
        ok (buffer[valid] == '\0',
            "\"%s\": Missing null termination (ret %d) - is %d\n", narrow_fmt, n, buffer[valid]);
    }
}

static int __cdecl vfprintf_wrapper(FILE *file,
                                    const char *format, ...)
{
    int ret;
    __ms_va_list valist;
    __ms_va_start(valist, format);
    ret = p_vfprintf(0, file, format, NULL, valist);
    __ms_va_end(valist);
    return ret;
}

static void test_fprintf(void)
{
    static const char file_name[] = "fprintf.tst";

    FILE *fp = p_fopen(file_name, "wb");
    char buf[1024];
    int ret;

    ret = vfprintf_wrapper(fp, "simple test\n");
    ok(ret == 12, "ret = %d\n", ret);
    ret = p_ftell(fp);
    ok(ret == 12, "ftell returned %d\n", ret);

    ret = vfprintf_wrapper(fp, "contains%cnull\n", '\0');
    ok(ret == 14, "ret = %d\n", ret);
    ret = p_ftell(fp);
    ok(ret == 26, "ftell returned %d\n", ret);

    p_fclose(fp);

    fp = p_fopen(file_name, "rb");
    p_fgets(buf, sizeof(buf), fp);
    ret = p_ftell(fp);
    ok(ret == 12, "ftell returned %d\n", ret);
    ok(!strcmp(buf, "simple test\n"), "buf = %s\n", buf);

    p_fgets(buf, sizeof(buf), fp);
    ret = p_ftell(fp);
    ok(ret == 26, "ret = %d\n", ret);
    ok(!memcmp(buf, "contains\0null\n", 14), "buf = %s\n", buf);

    p_fclose(fp);

    fp = p_fopen(file_name, "wt");

    ret = vfprintf_wrapper(fp, "simple test\n");
    ok(ret == 12, "ret = %d\n", ret);
    ret = p_ftell(fp);
    ok(ret == 13, "ftell returned %d\n", ret);

    ret = vfprintf_wrapper(fp, "contains%cnull\n", '\0');
    ok(ret == 14, "ret = %d\n", ret);
    ret = p_ftell(fp);
    ok(ret == 28, "ftell returned %d\n", ret);

    p_fclose(fp);

    fp = p_fopen(file_name, "rb");
    p_fgets(buf, sizeof(buf), fp);
    ret = p_ftell(fp);
    ok(ret == 13, "ftell returned %d\n", ret);
    ok(!strcmp(buf, "simple test\r\n"), "buf = %s\n", buf);

    p_fgets(buf, sizeof(buf), fp);
    ret = p_ftell(fp);
    ok(ret == 28, "ret = %d\n", ret);
    ok(!memcmp(buf, "contains\0null\r\n", 15), "buf = %s\n", buf);

    p_fclose(fp);
    unlink(file_name);
}

static int __cdecl _vsnprintf_s_wrapper(char *str, size_t sizeOfBuffer,
                                        size_t count, const char *format, ...)
{
    int ret;
    __ms_va_list valist;
    __ms_va_start(valist, format);
    ret = p_vsnprintf_s(0, str, sizeOfBuffer, count, format, NULL, valist);
    __ms_va_end(valist);
    return ret;
}

static void test_vsnprintf_s(void)
{
    const char format[] = "AB%uC";
    const char out7[] = "AB123C";
    const char out6[] = "AB123";
    const char out2[] = "A";
    const char out1[] = "";
    char buffer[14] = { 0 };
    int exp, got;

    /* Enough room. */
    exp = strlen(out7);

    got = _vsnprintf_s_wrapper(buffer, 14, _TRUNCATE, format, 123);
    ok( exp == got, "length wrong, expect=%d, got=%d\n", exp, got);
    ok( !strcmp(out7, buffer), "buffer wrong, got=%s\n", buffer);

    got = _vsnprintf_s_wrapper(buffer, 12, _TRUNCATE, format, 123);
    ok( exp == got, "length wrong, expect=%d, got=%d\n", exp, got);
    ok( !strcmp(out7, buffer), "buffer wrong, got=%s\n", buffer);

    got = _vsnprintf_s_wrapper(buffer, 7, _TRUNCATE, format, 123);
    ok( exp == got, "length wrong, expect=%d, got=%d\n", exp, got);
    ok( !strcmp(out7, buffer), "buffer wrong, got=%s\n", buffer);

    /* Not enough room. */
    exp = -1;

    got = _vsnprintf_s_wrapper(buffer, 6, _TRUNCATE, format, 123);
    ok( exp == got, "length wrong, expect=%d, got=%d\n", exp, got);
    ok( !strcmp(out6, buffer), "buffer wrong, got=%s\n", buffer);

    got = _vsnprintf_s_wrapper(buffer, 2, _TRUNCATE, format, 123);
    ok( exp == got, "length wrong, expect=%d, got=%d\n", exp, got);
    ok( !strcmp(out2, buffer), "buffer wrong, got=%s\n", buffer);

    got = _vsnprintf_s_wrapper(buffer, 1, _TRUNCATE, format, 123);
    ok( exp == got, "length wrong, expect=%d, got=%d\n", exp, got);
    ok( !strcmp(out1, buffer), "buffer wrong, got=%s\n", buffer);
}

START_TEST(printf)
{
    if (!init()) return;

    test_snprintf();
    test_swprintf();
    test_fprintf();
    test_vsnprintf_s();
}
