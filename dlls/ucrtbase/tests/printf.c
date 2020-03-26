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
#include <math.h>
#include <inttypes.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"

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

DEFINE_EXPECT(invalid_parameter_handler);

static inline float __port_ind(void)
{
    static const unsigned __ind_bytes = 0xffc00000;
    return *(const float *)&__ind_bytes;
}
#define IND __port_ind()

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

static int WINAPIV vsprintf_wrapper(unsigned __int64 options, char *str,
                                    size_t len, const char *format, ...)
{
    int ret;
    __ms_va_list valist;
    __ms_va_start(valist, format);
    ret = __stdio_common_vsprintf(options, str, len, format, NULL, valist);
    __ms_va_end(valist);
    return ret;
}

static void test_snprintf (void)
{
    const char *tests[] = {"short", "justfit", "justfits", "muchlonger"};
    char buffer[8];
    const int bufsiz = sizeof buffer;
    unsigned int i;

    /* Legacy _snprintf style termination */
    for (i = 0; i < ARRAY_SIZE(tests); i++) {
        const char *fmt  = tests[i];
        const int expect = strlen(fmt) > bufsiz ? -1 : strlen(fmt);
        const int n      = vsprintf_wrapper (_CRT_INTERNAL_PRINTF_LEGACY_VSPRINTF_NULL_TERMINATION, buffer, bufsiz, fmt);
        const int valid  = n < 0 ? bufsiz : (n == bufsiz ? n : n+1);

        ok (n == expect, "\"%s\": expected %d, returned %d\n",
            fmt, expect, n);
        ok (!memcmp (fmt, buffer, valid),
            "\"%s\": rendered \"%.*s\"\n", fmt, valid, buffer);
    }

    /* C99 snprintf style termination */
    for (i = 0; i < ARRAY_SIZE(tests); i++) {
        const char *fmt  = tests[i];
        const int expect = strlen(fmt);
        const int n      = vsprintf_wrapper (_CRT_INTERNAL_PRINTF_STANDARD_SNPRINTF_BEHAVIOR, buffer, bufsiz, fmt);
        const int valid  = n >= bufsiz ? bufsiz - 1 : n < 0 ? 0 : n;

        ok (n == expect, "\"%s\": expected %d, returned %d\n",
            fmt, expect, n);
        ok (!memcmp (fmt, buffer, valid),
            "\"%s\": rendered \"%.*s\"\n", fmt, valid, buffer);
        ok (buffer[valid] == '\0',
            "\"%s\": Missing null termination (ret %d) - is %d\n", fmt, n, buffer[valid]);
    }

    /* swprintf style termination */
    for (i = 0; i < ARRAY_SIZE(tests); i++) {
        const char *fmt  = tests[i];
        const int expect = strlen(fmt) >= bufsiz ? -2 : strlen(fmt);
        const int n      = vsprintf_wrapper (0, buffer, bufsiz, fmt);
        const int valid  = n < 0 ? bufsiz - 1 : n;

        ok (n == expect, "\"%s\": expected %d, returned %d\n",
            fmt, expect, n);
        ok (!memcmp (fmt, buffer, valid),
            "\"%s\": rendered \"%.*s\"\n", fmt, valid, buffer);
        ok (buffer[valid] == '\0',
            "\"%s\": Missing null termination (ret %d) - is %d\n", fmt, n, buffer[valid]);
    }

    ok (vsprintf_wrapper (_CRT_INTERNAL_PRINTF_STANDARD_SNPRINTF_BEHAVIOR, NULL, 0, "abcd") == 4,
        "Failure to snprintf to NULL\n");
    ok (vsprintf_wrapper (_CRT_INTERNAL_PRINTF_LEGACY_VSPRINTF_NULL_TERMINATION, NULL, 0, "abcd") == 4,
        "Failure to snprintf to NULL\n");
    ok (vsprintf_wrapper (0, NULL, 0, "abcd") == 4,
        "Failure to snprintf to NULL\n");
}

static int WINAPIV vswprintf_wrapper(unsigned __int64 options, wchar_t *str,
                                     size_t len, const wchar_t *format, ...)
{
    int ret;
    __ms_va_list valist;
    __ms_va_start(valist, format);
    ret = __stdio_common_vswprintf(options, str, len, format, NULL, valist);
    __ms_va_end(valist);
    return ret;
}

static void test_swprintf (void)
{
    const wchar_t str_short[]      = {'s','h','o','r','t',0};
    const wchar_t str_justfit[]    = {'j','u','s','t','f','i','t',0};
    const wchar_t str_justfits[]   = {'j','u','s','t','f','i','t','s',0};
    const wchar_t str_muchlonger[] = {'m','u','c','h','l','o','n','g','e','r',0};
    const wchar_t *tests[] = {str_short, str_justfit, str_justfits, str_muchlonger};

    wchar_t buffer[8];
    char narrow[8], narrow_fmt[16];
    const int bufsiz = ARRAY_SIZE(buffer);
    unsigned int i;

    /* Legacy _snprintf style termination */
    for (i = 0; i < ARRAY_SIZE(tests); i++) {
        const wchar_t *fmt = tests[i];
        const int expect   = wcslen(fmt) > bufsiz ? -1 : wcslen(fmt);
        const int n        = vswprintf_wrapper (_CRT_INTERNAL_PRINTF_LEGACY_VSPRINTF_NULL_TERMINATION, buffer, bufsiz, fmt);
        const int valid    = n < 0 ? bufsiz : (n == bufsiz ? n : n+1);

        WideCharToMultiByte (CP_ACP, 0, buffer, -1, narrow, sizeof(narrow), NULL, NULL);
        WideCharToMultiByte (CP_ACP, 0, fmt, -1, narrow_fmt, sizeof(narrow_fmt), NULL, NULL);
        ok (n == expect, "\"%s\": expected %d, returned %d\n",
            narrow_fmt, expect, n);
        ok (!memcmp (fmt, buffer, valid * sizeof(wchar_t)),
            "\"%s\": rendered \"%.*s\"\n", narrow_fmt, valid, narrow);
    }

    /* C99 snprintf style termination */
    for (i = 0; i < ARRAY_SIZE(tests); i++) {
        const wchar_t *fmt = tests[i];
        const int expect   = wcslen(fmt);
        const int n        = vswprintf_wrapper (_CRT_INTERNAL_PRINTF_STANDARD_SNPRINTF_BEHAVIOR, buffer, bufsiz, fmt);
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

    /* swprintf style termination */
    for (i = 0; i < ARRAY_SIZE(tests); i++) {
        const wchar_t *fmt = tests[i];
        const int expect   = wcslen(fmt) >= bufsiz ? -2 : wcslen(fmt);
        const int n        = vswprintf_wrapper (0, buffer, bufsiz, fmt);
        const int valid    = n < 0 ? bufsiz - 1 : n;

        WideCharToMultiByte (CP_ACP, 0, buffer, -1, narrow, sizeof(narrow), NULL, NULL);
        WideCharToMultiByte (CP_ACP, 0, fmt, -1, narrow_fmt, sizeof(narrow_fmt), NULL, NULL);
        ok (n == expect, "\"%s\": expected %d, returned %d\n",
            narrow_fmt, expect, n);
        ok (!memcmp (fmt, buffer, valid * sizeof(wchar_t)),
            "\"%s\": rendered \"%.*s\"\n", narrow_fmt, valid, narrow);
        ok (buffer[valid] == '\0',
            "\"%s\": Missing null termination (ret %d) - is %d\n", narrow_fmt, n, buffer[valid]);
    }

    ok (vswprintf_wrapper (_CRT_INTERNAL_PRINTF_STANDARD_SNPRINTF_BEHAVIOR, NULL, 0, str_short) == 5,
        "Failure to swprintf to NULL\n");
    ok (vswprintf_wrapper (_CRT_INTERNAL_PRINTF_LEGACY_VSPRINTF_NULL_TERMINATION, NULL, 0, str_short) == 5,
        "Failure to swprintf to NULL\n");
    ok (vswprintf_wrapper (0, NULL, 0, str_short) == 5,
        "Failure to swprintf to NULL\n");
}

static int WINAPIV vfprintf_wrapper(FILE *file,
                                    const char *format, ...)
{
    int ret;
    __ms_va_list valist;
    __ms_va_start(valist, format);
    ret = __stdio_common_vfprintf(0, file, format, NULL, valist);
    __ms_va_end(valist);
    return ret;
}

static void test_fprintf(void)
{
    static const char file_name[] = "fprintf.tst";

    FILE *fp = fopen(file_name, "wb");
    char buf[1024];
    int ret;

    ret = vfprintf_wrapper(fp, "simple test\n");
    ok(ret == 12, "ret = %d\n", ret);
    ret = ftell(fp);
    ok(ret == 12, "ftell returned %d\n", ret);

    ret = vfprintf_wrapper(fp, "contains%cnull\n", '\0');
    ok(ret == 14, "ret = %d\n", ret);
    ret = ftell(fp);
    ok(ret == 26, "ftell returned %d\n", ret);

    fclose(fp);

    fp = fopen(file_name, "rb");
    fgets(buf, sizeof(buf), fp);
    ret = ftell(fp);
    ok(ret == 12, "ftell returned %d\n", ret);
    ok(!strcmp(buf, "simple test\n"), "buf = %s\n", buf);

    fgets(buf, sizeof(buf), fp);
    ret = ftell(fp);
    ok(ret == 26, "ret = %d\n", ret);
    ok(!memcmp(buf, "contains\0null\n", 14), "buf = %s\n", buf);

    fclose(fp);

    fp = fopen(file_name, "wt");

    ret = vfprintf_wrapper(fp, "simple test\n");
    ok(ret == 12, "ret = %d\n", ret);
    ret = ftell(fp);
    ok(ret == 13, "ftell returned %d\n", ret);

    ret = vfprintf_wrapper(fp, "contains%cnull\n", '\0');
    ok(ret == 14, "ret = %d\n", ret);
    ret = ftell(fp);
    ok(ret == 28, "ftell returned %d\n", ret);

    fclose(fp);

    fp = fopen(file_name, "rb");
    fgets(buf, sizeof(buf), fp);
    ret = ftell(fp);
    ok(ret == 13, "ftell returned %d\n", ret);
    ok(!strcmp(buf, "simple test\r\n"), "buf = %s\n", buf);

    fgets(buf, sizeof(buf), fp);
    ret = ftell(fp);
    ok(ret == 28, "ret = %d\n", ret);
    ok(!memcmp(buf, "contains\0null\r\n", 15), "buf = %s\n", buf);

    fclose(fp);
    unlink(file_name);
}

static int WINAPIV vfwprintf_wrapper(FILE *file,
                                     const wchar_t *format, ...)
{
    int ret;
    __ms_va_list valist;
    __ms_va_start(valist, format);
    ret = __stdio_common_vfwprintf(0, file, format, NULL, valist);
    __ms_va_end(valist);
    return ret;
}

static void test_fwprintf(void)
{
    static const char file_name[] = "fprintf.tst";
    static const WCHAR simple[] = {'s','i','m','p','l','e',' ','t','e','s','t','\n',0};
    static const WCHAR cont_fmt[] = {'c','o','n','t','a','i','n','s','%','c','n','u','l','l','\n',0};
    static const WCHAR cont[] = {'c','o','n','t','a','i','n','s','\0','n','u','l','l','\n',0};

    FILE *fp = fopen(file_name, "wb");
    wchar_t bufw[1024];
    char bufa[1024];
    int ret;

    ret = vfwprintf_wrapper(fp, simple);
    ok(ret == 12, "ret = %d\n", ret);
    ret = ftell(fp);
    ok(ret == 24, "ftell returned %d\n", ret);

    ret = vfwprintf_wrapper(fp, cont_fmt, '\0');
    ok(ret == 14, "ret = %d\n", ret);
    ret = ftell(fp);
    ok(ret == 52, "ftell returned %d\n", ret);

    fclose(fp);

    fp = fopen(file_name, "rb");
    fgetws(bufw, ARRAY_SIZE(bufw), fp);
    ret = ftell(fp);
    ok(ret == 24, "ftell returned %d\n", ret);
    ok(!wcscmp(bufw, simple), "buf = %s\n", wine_dbgstr_w(bufw));

    fgetws(bufw, ARRAY_SIZE(bufw), fp);
    ret = ftell(fp);
    ok(ret == 52, "ret = %d\n", ret);
    ok(!memcmp(bufw, cont, 28), "buf = %s\n", wine_dbgstr_w(bufw));

    fclose(fp);

    fp = fopen(file_name, "wt");

    ret = vfwprintf_wrapper(fp, simple);
    ok(ret == 12, "ret = %d\n", ret);
    ret = ftell(fp);
    ok(ret == 13, "ftell returned %d\n", ret);

    ret = vfwprintf_wrapper(fp, cont_fmt, '\0');
    ok(ret == 14, "ret = %d\n", ret);
    ret = ftell(fp);
    ok(ret == 28, "ftell returned %d\n", ret);

    fclose(fp);

    fp = fopen(file_name, "rb");
    fgets(bufa, sizeof(bufa), fp);
    ret = ftell(fp);
    ok(ret == 13, "ftell returned %d\n", ret);
    ok(!strcmp(bufa, "simple test\r\n"), "buf = %s\n", bufa);

    fgets(bufa, sizeof(bufa), fp);
    ret = ftell(fp);
    ok(ret == 28, "ret = %d\n", ret);
    ok(!memcmp(bufa, "contains\0null\r\n", 15), "buf = %s\n", bufa);

    fclose(fp);
    unlink(file_name);

    /* NULL format */
    errno = 0xdeadbeef;
    SET_EXPECT(invalid_parameter_handler);
    ret = vfwprintf_wrapper(fp, NULL);
    ok(errno == EINVAL, "expected errno EINVAL, got %d\n", errno);
    ok(ret == -1, "expected ret -1, got %d\n", ret);
    CHECK_CALLED(invalid_parameter_handler);

    /* NULL file */
    errno = 0xdeadbeef;
    SET_EXPECT(invalid_parameter_handler);
    ret = vfwprintf_wrapper(NULL, simple);
    ok(errno == EINVAL, "expected errno EINVAL, got %d\n", errno);
    ok(ret == -1, "expected ret -1, got %d\n", ret);
    CHECK_CALLED(invalid_parameter_handler);

    /* format using % with NULL arglist*/
    /* crashes on Windows */
    /* ret = __stdio_common_vfwprintf(0, fp, cont_fmt, NULL, NULL); */
}

static int WINAPIV _vsnprintf_s_wrapper(char *str, size_t sizeOfBuffer,
                                        size_t count, const char *format, ...)
{
    int ret;
    __ms_va_list valist;
    __ms_va_start(valist, format);
    ret = __stdio_common_vsnprintf_s(0, str, sizeOfBuffer, count, format, NULL, valist);
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

static int WINAPIV _vsnwprintf_s_wrapper(WCHAR *str, size_t sizeOfBuffer,
                                        size_t count, const WCHAR *format, ...)
{
    int ret;
    __ms_va_list valist;
    __ms_va_start(valist, format);
    ret = __stdio_common_vsnwprintf_s(0, str, sizeOfBuffer, count, format, NULL, valist);
    __ms_va_end(valist);
    return ret;
}

static void test_vsnwprintf_s(void)
{
    const WCHAR format[] = {'A','B','%','u','C',0};
    const WCHAR out7[] = {'A','B','1','2','3','C',0};
    const WCHAR out6[] = {'A','B','1','2','3',0};
    const WCHAR out2[] = {'A',0};
    const WCHAR out1[] = {0};
    WCHAR buffer[14] = { 0 };
    int exp, got;

    /* Enough room. */
    exp = lstrlenW(out7);

    got = _vsnwprintf_s_wrapper(buffer, 14, _TRUNCATE, format, 123);
    ok( exp == got, "length wrong, expect=%d, got=%d\n", exp, got);
    ok( !lstrcmpW(out7, buffer), "buffer wrong, got=%s\n", wine_dbgstr_w(buffer));

    got = _vsnwprintf_s_wrapper(buffer, 12, _TRUNCATE, format, 123);
    ok( exp == got, "length wrong, expect=%d, got=%d\n", exp, got);
    ok( !lstrcmpW(out7, buffer), "buffer wrong, got=%s\n", wine_dbgstr_w(buffer));

    got = _vsnwprintf_s_wrapper(buffer, 7, _TRUNCATE, format, 123);
    ok( exp == got, "length wrong, expect=%d, got=%d\n", exp, got);
    ok( !lstrcmpW(out7, buffer), "buffer wrong, got=%s\n", wine_dbgstr_w(buffer));

    /* Not enough room. */
    exp = -1;

    got = _vsnwprintf_s_wrapper(buffer, 6, _TRUNCATE, format, 123);
    ok( exp == got, "length wrong, expect=%d, got=%d\n", exp, got);
    ok( !lstrcmpW(out6, buffer), "buffer wrong, got=%s\n", wine_dbgstr_w(buffer));

    got = _vsnwprintf_s_wrapper(buffer, 2, _TRUNCATE, format, 123);
    ok( exp == got, "length wrong, expect=%d, got=%d\n", exp, got);
    ok( !lstrcmpW(out2, buffer), "buffer wrong, got=%s\n", wine_dbgstr_w(buffer));

    got = _vsnwprintf_s_wrapper(buffer, 1, _TRUNCATE, format, 123);
    ok( exp == got, "length wrong, expect=%d, got=%d\n", exp, got);
    ok( !lstrcmpW(out1, buffer), "buffer wrong, got=%s\n", wine_dbgstr_w(buffer));
}

static void test_printf_legacy_wide(void)
{
    const wchar_t wide[] = {'A','B','C','D',0};
    const char narrow[] = "abcd";
    const char out[] = "abcd ABCD";
    /* The legacy wide flag doesn't affect narrow printfs, so the same
     * format should behave the same both with and without the flag. */
    const char narrow_fmt[] = "%s %ls";
    /* The standard behaviour is to use the same format as for the narrow
     * case, while the legacy case has got a different meaning for %s. */
    const wchar_t std_wide_fmt[] = {'%','s',' ','%','l','s',0};
    const wchar_t legacy_wide_fmt[] = {'%','h','s',' ','%','s',0};
    char buffer[20];
    wchar_t wbuffer[20];

    vsprintf_wrapper(0, buffer, sizeof(buffer), narrow_fmt, narrow, wide);
    ok(!strcmp(buffer, out), "buffer wrong, got=%s\n", buffer);
    vsprintf_wrapper(_CRT_INTERNAL_PRINTF_LEGACY_WIDE_SPECIFIERS, buffer, sizeof(buffer), narrow_fmt, narrow, wide);
    ok(!strcmp(buffer, out), "buffer wrong, got=%s\n", buffer);

    vswprintf_wrapper(0, wbuffer, sizeof(wbuffer), std_wide_fmt, narrow, wide);
    WideCharToMultiByte(CP_ACP, 0, wbuffer, -1, buffer, sizeof(buffer), NULL, NULL);
    ok(!strcmp(buffer, out), "buffer wrong, got=%s\n", buffer);
    vswprintf_wrapper(_CRT_INTERNAL_PRINTF_LEGACY_WIDE_SPECIFIERS, wbuffer, sizeof(wbuffer), legacy_wide_fmt, narrow, wide);
    WideCharToMultiByte(CP_ACP, 0, wbuffer, -1, buffer, sizeof(buffer), NULL, NULL);
    ok(!strcmp(buffer, out), "buffer wrong, got=%s\n", buffer);
}

static void test_printf_legacy_msvcrt(void)
{
    char buf[50];

    /* In standard mode, %F is a float format conversion, while it is a
     * length modifier in legacy msvcrt mode. In legacy mode, N is also
     * a length modifier. */
    vsprintf_wrapper(0, buf, sizeof(buf), "%F", 1.23);
    ok(!strcmp(buf, "1.230000"), "buf = %s\n", buf);
    vsprintf_wrapper(_CRT_INTERNAL_PRINTF_LEGACY_MSVCRT_COMPATIBILITY, buf, sizeof(buf), "%Fd %Nd", 123, 456);
    ok(!strcmp(buf, "123 456"), "buf = %s\n", buf);

    vsprintf_wrapper(0, buf, sizeof(buf), "%f %F %f %e %E %g %G", INFINITY, INFINITY, -INFINITY, INFINITY, INFINITY, INFINITY, INFINITY);
    ok(!strcmp(buf, "inf INF -inf inf INF inf INF"), "buf = %s\n", buf);
    vsprintf_wrapper(_CRT_INTERNAL_PRINTF_LEGACY_MSVCRT_COMPATIBILITY, buf, sizeof(buf), "%f", INFINITY);
    ok(!strcmp(buf, "1.#INF00"), "buf = %s\n", buf);
    vsprintf_wrapper(0, buf, sizeof(buf), "%f %F", NAN, NAN);
    ok(!strcmp(buf, "nan NAN"), "buf = %s\n", buf);
    vsprintf_wrapper(_CRT_INTERNAL_PRINTF_LEGACY_MSVCRT_COMPATIBILITY, buf, sizeof(buf), "%f", NAN);
    ok(!strcmp(buf, "1.#QNAN0"), "buf = %s\n", buf);
    vsprintf_wrapper(0, buf, sizeof(buf), "%f %F", IND, IND);
    ok(!strcmp(buf, "-nan(ind) -NAN(IND)"), "buf = %s\n", buf);
    vsprintf_wrapper(_CRT_INTERNAL_PRINTF_LEGACY_MSVCRT_COMPATIBILITY, buf, sizeof(buf), "%f", IND);
    ok(!strcmp(buf, "-1.#IND00"), "buf = %s\n", buf);
}

static void test_printf_legacy_three_digit_exp(void)
{
    char buf[20];

    vsprintf_wrapper(0, buf, sizeof(buf), "%E", 1.23);
    ok(!strcmp(buf, "1.230000E+00"), "buf = %s\n", buf);
    vsprintf_wrapper(_CRT_INTERNAL_PRINTF_LEGACY_THREE_DIGIT_EXPONENTS, buf, sizeof(buf), "%E", 1.23);
    ok(!strcmp(buf, "1.230000E+000"), "buf = %s\n", buf);
    vsprintf_wrapper(0, buf, sizeof(buf), "%E", 1.23e+123);
    ok(!strcmp(buf, "1.230000E+123"), "buf = %s\n", buf);
}

static void test_printf_c99(void)
{
    char buf[30];
    int i;

    /* The msvcrt compatibility flag doesn't affect whether 'z' is interpreted
     * as size_t size for integers. */
     for (i = 0; i < 2; i++) {
        unsigned __int64 options = (i == 0) ? 0 :
            _CRT_INTERNAL_PRINTF_LEGACY_MSVCRT_COMPATIBILITY;

        /* z modifier accepts size_t argument */
        vsprintf_wrapper(options, buf, sizeof(buf), "%zx %d", SIZE_MAX, 1);
        if (sizeof(size_t) == 8)
            ok(!strcmp(buf, "ffffffffffffffff 1"), "buf = %s\n", buf);
        else
            ok(!strcmp(buf, "ffffffff 1"), "buf = %s\n", buf);

        /* j modifier with signed format accepts intmax_t argument */
        vsprintf_wrapper(options, buf, sizeof(buf), "%jd %d", INTMAX_MIN, 1);
        ok(!strcmp(buf, "-9223372036854775808 1"), "buf = %s\n", buf);

        /* j modifier with unsigned format accepts uintmax_t argument */
        vsprintf_wrapper(options, buf, sizeof(buf), "%ju %d", UINTMAX_MAX, 1);
        ok(!strcmp(buf, "18446744073709551615 1"), "buf = %s\n", buf);

        /* t modifier accepts ptrdiff_t argument */
        vsprintf_wrapper(options, buf, sizeof(buf), "%td %d", PTRDIFF_MIN, 1);
        if (sizeof(ptrdiff_t) == 8)
            ok(!strcmp(buf, "-9223372036854775808 1"), "buf = %s\n", buf);
        else
            ok(!strcmp(buf, "-2147483648 1"), "buf = %s\n", buf);
    }
}

static void test_printf_natural_string(void)
{
    const wchar_t wide[] = {'A','B','C','D',0};
    const char narrow[] = "abcd";
    const char narrow_fmt[] = "%s %Ts";
    const char narrow_out[] = "abcd abcd";
    const wchar_t wide_fmt[] = {'%','s',' ','%','T','s',0};
    const wchar_t wide_out[] = {'a','b','c','d',' ','A','B','C','D',0};
    char buffer[20];
    wchar_t wbuffer[20];

    vsprintf_wrapper(0, buffer, sizeof(buffer), narrow_fmt, narrow, narrow);
    ok(!strcmp(buffer, narrow_out), "buffer wrong, got=%s\n", buffer);

    vswprintf_wrapper(0, wbuffer, sizeof(wbuffer), wide_fmt, narrow, wide);
    ok(!lstrcmpW(wbuffer, wide_out), "buffer wrong, got=%s\n", wine_dbgstr_w(wbuffer));
}

START_TEST(printf)
{
    ok(_set_invalid_parameter_handler(test_invalid_parameter_handler) == NULL,
            "Invalid parameter handler was already set\n");

    test_snprintf();
    test_swprintf();
    test_fprintf();
    test_fwprintf();
    test_vsnprintf_s();
    test_vsnwprintf_s();
    test_printf_legacy_wide();
    test_printf_legacy_msvcrt();
    test_printf_legacy_three_digit_exp();
    test_printf_c99();
    test_printf_natural_string();
}
