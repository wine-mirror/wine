/*
 * Copyright 2010 Detlef Riekenberg
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
#include <stdlib.h>
#include <stdio.h>

#include <windef.h>
#include <winbase.h>
#include <errno.h>
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

static _invalid_parameter_handler (__cdecl *p_set_invalid_parameter_handler)(_invalid_parameter_handler);
typedef int (__cdecl *_INITTERM_E_FN)(void);
static int (__cdecl *p_initterm_e)(_INITTERM_E_FN *table, _INITTERM_E_FN *end);
static void* (__cdecl *p_encode_pointer)(void *);
static void* (__cdecl *p_decode_pointer)(void *);
static void* (__cdecl *p_encoded_null)(void);
static int *p_sys_nerr;
static int* (__cdecl *p__sys_nerr)(void);
static char **p_sys_errlist;
static char** (__cdecl *p__sys_errlist)(void);
static __int64 (__cdecl *p_strtoi64)(const char *, char **, int);
static unsigned __int64 (__cdecl *p_strtoui64)(const char *, char **, int);
static errno_t (__cdecl *p_itoa_s)(int,char*,size_t,int);
static int (__cdecl *p_wcsncat_s)(wchar_t *dst, size_t elem, const wchar_t *src, size_t count);
static void (__cdecl *p_qsort_s)(void *, size_t, size_t, int (__cdecl *)(void *, const void *, const void *), void *);

static void* (WINAPI *pEncodePointer)(void *);

static int cb_called[4];
static int g_qsort_s_context_counter;

/* ########## */

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

static int __cdecl initterm_cb0(void)
{
    cb_called[0]++;
    return 0;
}

static int __cdecl initterm_cb1(void)
{
    cb_called[1]++;
    return 1;
}

static int __cdecl initterm_cb2(void)
{
    cb_called[2]++;
    return 2;
}


static void test__initterm_e(void)
{
    _INITTERM_E_FN table[4];
    int res;

    if (!p_initterm_e) {
        skip("_initterm_e not found\n");
        return;
    }

    memset(table, 0, sizeof(table));

    memset(cb_called, 0, sizeof(cb_called));
    errno = 0xdeadbeef;
    res = p_initterm_e(table, table);
    ok( !res && !cb_called[0] && !cb_called[1] && !cb_called[2],
        "got %d with 0x%x {%d, %d, %d}\n",
        res, errno, cb_called[0], cb_called[1], cb_called[2]);

    memset(cb_called, 0, sizeof(cb_called));
    errno = 0xdeadbeef;
    res = p_initterm_e(table, NULL);
    ok( !res && !cb_called[0] && !cb_called[1] && !cb_called[2],
        "got %d with 0x%x {%d, %d, %d}\n",
        res, errno, cb_called[0], cb_called[1], cb_called[2]);

    if (0) {
        /* this crash on Windows */
        errno = 0xdeadbeef;
        res = p_initterm_e(NULL, table);
        trace("got %d with 0x%x\n", res, errno);
    }

    table[0] = initterm_cb0;
    memset(cb_called, 0, sizeof(cb_called));
    errno = 0xdeadbeef;
    res = p_initterm_e(table, &table[1]);
    ok( !res && (cb_called[0] == 1) && !cb_called[1] && !cb_called[2],
        "got %d with 0x%x {%d, %d, %d}\n",
        res, errno, cb_called[0], cb_called[1], cb_called[2]);


    /* init-function returning failure */
    table[1] = initterm_cb1;
    memset(cb_called, 0, sizeof(cb_called));
    errno = 0xdeadbeef;
    res = p_initterm_e(table, &table[3]);
    ok( (res == 1) && (cb_called[0] == 1) && (cb_called[1] == 1) && !cb_called[2],
        "got %d with 0x%x {%d, %d, %d}\n",
        res, errno, cb_called[0], cb_called[1], cb_called[2]);

    /* init-function not called, when end < start */
    memset(cb_called, 0, sizeof(cb_called));
    errno = 0xdeadbeef;
    res = p_initterm_e(&table[3], table);
    ok( !res && !cb_called[0] && !cb_called[1] && !cb_called[2],
        "got %d with 0x%x {%d, %d, %d}\n",
        res, errno, cb_called[0], cb_called[1], cb_called[2]);

    /* initialization stop after first non-zero result */
    table[2] = initterm_cb0;
    memset(cb_called, 0, sizeof(cb_called));
    errno = 0xdeadbeef;
    res = p_initterm_e(table, &table[3]);
    ok( (res == 1) && (cb_called[0] == 1) && (cb_called[1] == 1) && !cb_called[2],
        "got %d with 0x%x {%d, %d, %d}\n",
        res, errno, cb_called[0], cb_called[1], cb_called[2]);

    /* NULL pointer in the array are skipped */
    table[1] = NULL;
    table[2] = initterm_cb2;
    memset(cb_called, 0, sizeof(cb_called));
    errno = 0xdeadbeef;
    res = p_initterm_e(table, &table[3]);
    ok( (res == 2) && (cb_called[0] == 1) && !cb_called[1] && (cb_called[2] == 1),
        "got %d with 0x%x {%d, %d, %d}\n",
        res, errno, cb_called[0], cb_called[1], cb_called[2]);

}

/* Beware that _encode_pointer is a NOP before XP
   (the parameter is returned unchanged) */
static void test__encode_pointer(void)
{
    void *ptr, *res;

    if(!p_encode_pointer || !p_decode_pointer || !p_encoded_null) {
        win_skip("_encode_pointer, _decode_pointer or _encoded_null not found\n");
        return;
    }

    ptr = (void*)0xdeadbeef;
    res = p_encode_pointer(ptr);
    res = p_decode_pointer(res);
    ok(res == ptr, "Pointers are different after encoding and decoding\n");

    ok(p_encoded_null() == p_encode_pointer(NULL), "Error encoding null\n");

    ptr = p_encode_pointer(p_encode_pointer);
    ok(p_decode_pointer(ptr) == p_encode_pointer, "Error decoding pointer\n");

    /* Not present before XP */
    if (!pEncodePointer) {
        win_skip("EncodePointer not found\n");
        return;
    }

    res = pEncodePointer(p_encode_pointer);
    ok(ptr == res, "_encode_pointer produced different result than EncodePointer\n");

}

static void test_error_messages(void)
{
    int *size, size_copy;

    if(!p_sys_nerr || !p__sys_nerr || !p_sys_errlist || !p__sys_errlist) {
        win_skip("Skipping test_error_messages tests\n");
        return;
    }

    size = p__sys_nerr();
    size_copy = *size;
    ok(*p_sys_nerr == *size, "_sys_nerr = %u, size = %u\n", *p_sys_nerr, *size);

    *size = 20;
    ok(*p_sys_nerr == *size, "_sys_nerr = %u, size = %u\n", *p_sys_nerr, *size);

    *size = size_copy;

    ok(*p_sys_errlist == *(p__sys_errlist()), "p_sys_errlist != p__sys_errlist()\n");
}

static void test__strtoi64(void)
{
    __int64 res;
    unsigned __int64 ures;

    if(!p_strtoi64 || !p_strtoui64) {
        win_skip("_strtoi64 or _strtoui64 not found\n");
        return;
    }

    if(!p_set_invalid_parameter_handler) {
        win_skip("_set_invalid_parameter_handler not found\n");
        return;
    }

    errno = 0xdeadbeef;
    SET_EXPECT(invalid_parameter_handler);
    res = p_strtoi64(NULL, NULL, 10);
    ok(res == 0, "res != 0\n");
    CHECK_CALLED(invalid_parameter_handler);

    SET_EXPECT(invalid_parameter_handler);
    res = p_strtoi64("123", NULL, 1);
    ok(res == 0, "res != 0\n");
    CHECK_CALLED(invalid_parameter_handler);

    SET_EXPECT(invalid_parameter_handler);
    res = p_strtoi64("123", NULL, 37);
    ok(res == 0, "res != 0\n");
    CHECK_CALLED(invalid_parameter_handler);

    SET_EXPECT(invalid_parameter_handler);
    ures = p_strtoui64(NULL, NULL, 10);
    ok(ures == 0, "res = %d\n", (int)ures);
    CHECK_CALLED(invalid_parameter_handler);

    SET_EXPECT(invalid_parameter_handler);
    ures = p_strtoui64("123", NULL, 1);
    ok(ures == 0, "res = %d\n", (int)ures);
    CHECK_CALLED(invalid_parameter_handler);

    SET_EXPECT(invalid_parameter_handler);
    ures = p_strtoui64("123", NULL, 37);
    ok(ures == 0, "res = %d\n", (int)ures);
    CHECK_CALLED(invalid_parameter_handler);
    ok(errno == 0xdeadbeef, "errno = %x\n", errno);
}

static void test__itoa_s(void)
{
    errno_t ret;
    char buffer[33];

    if (!p_itoa_s)
    {
        win_skip("Skipping _itoa_s tests\n");
        return;
    }

    if(!p_set_invalid_parameter_handler) {
        win_skip("_set_invalid_parameter_handler not found\n");
        return;
    }

    /* _itoa_s (on msvcr90) doesn't set errno (in case of errors) while msvcrt does
     * as we always set errno in our msvcrt implementation, don't test here that errno
     * isn't changed
     */
    SET_EXPECT(invalid_parameter_handler);
    ret = p_itoa_s(0, NULL, 0, 0);
    ok(ret == EINVAL, "Expected _itoa_s to return EINVAL, got %d\n", ret);
    CHECK_CALLED(invalid_parameter_handler);

    memset(buffer, 'X', sizeof(buffer));
    SET_EXPECT(invalid_parameter_handler);
    ret = p_itoa_s(0, buffer, 0, 0);
    ok(ret == EINVAL, "Expected _itoa_s to return EINVAL, got %d\n", ret);
    ok(buffer[0] == 'X', "Expected the output buffer to be untouched\n");
    CHECK_CALLED(invalid_parameter_handler);

    memset(buffer, 'X', sizeof(buffer));
    SET_EXPECT(invalid_parameter_handler);
    ret = p_itoa_s(0, buffer, sizeof(buffer), 0);
    ok(ret == EINVAL, "Expected _itoa_s to return EINVAL, got %d\n", ret);
    ok(buffer[0] == '\0', "Expected the output buffer to be null terminated\n");
    CHECK_CALLED(invalid_parameter_handler);

    memset(buffer, 'X', sizeof(buffer));
    SET_EXPECT(invalid_parameter_handler);
    ret = p_itoa_s(0, buffer, sizeof(buffer), 64);
    ok(ret == EINVAL, "Expected _itoa_s to return EINVAL, got %d\n", ret);
    ok(buffer[0] == '\0', "Expected the output buffer to be null terminated\n");
    CHECK_CALLED(invalid_parameter_handler);

    memset(buffer, 'X', sizeof(buffer));
    SET_EXPECT(invalid_parameter_handler);
    ret = p_itoa_s(12345678, buffer, 4, 10);
    ok(ret == ERANGE, "Expected _itoa_s to return ERANGE, got %d\n", ret);
    ok(!memcmp(buffer, "\000765", 4),
       "Expected the output buffer to be null terminated with truncated output\n");
    CHECK_CALLED(invalid_parameter_handler);

    memset(buffer, 'X', sizeof(buffer));
    SET_EXPECT(invalid_parameter_handler);
    ret = p_itoa_s(12345678, buffer, 8, 10);
    ok(ret == ERANGE, "Expected _itoa_s to return ERANGE, got %d\n", ret);
    ok(!memcmp(buffer, "\0007654321", 8),
       "Expected the output buffer to be null terminated with truncated output\n");
    CHECK_CALLED(invalid_parameter_handler);

    memset(buffer, 'X', sizeof(buffer));
    SET_EXPECT(invalid_parameter_handler);
    ret = p_itoa_s(-12345678, buffer, 9, 10);
    ok(ret == ERANGE, "Expected _itoa_s to return ERANGE, got %d\n", ret);
    ok(!memcmp(buffer, "\00087654321", 9),
       "Expected the output buffer to be null terminated with truncated output\n");
    CHECK_CALLED(invalid_parameter_handler);

    ret = p_itoa_s(12345678, buffer, 9, 10);
    ok(ret == 0, "Expected _itoa_s to return 0, got %d\n", ret);
    ok(!strcmp(buffer, "12345678"),
       "Expected output buffer string to be \"12345678\", got \"%s\"\n",
       buffer);

    ret = p_itoa_s(43690, buffer, sizeof(buffer), 2);
    ok(ret == 0, "Expected _itoa_s to return 0, got %d\n", ret);
    ok(!strcmp(buffer, "1010101010101010"),
       "Expected output buffer string to be \"1010101010101010\", got \"%s\"\n",
       buffer);

    ret = p_itoa_s(1092009, buffer, sizeof(buffer), 36);
    ok(ret == 0, "Expected _itoa_s to return 0, got %d\n", ret);
    ok(!strcmp(buffer, "nell"),
       "Expected output buffer string to be \"nell\", got \"%s\"\n",
       buffer);

    ret = p_itoa_s(5704, buffer, sizeof(buffer), 18);
    ok(ret == 0, "Expected _itoa_s to return 0, got %d\n", ret);
    ok(!strcmp(buffer, "hag"),
       "Expected output buffer string to be \"hag\", got \"%s\"\n",
       buffer);

    ret = p_itoa_s(-12345678, buffer, sizeof(buffer), 10);
    ok(ret == 0, "Expected _itoa_s to return 0, got %d\n", ret);
    ok(!strcmp(buffer, "-12345678"),
       "Expected output buffer string to be \"-12345678\", got \"%s\"\n",
       buffer);
}

static void test_wcsncat_s(void)
{
    static wchar_t abcW[] = {'a','b','c',0};
    int ret;
    wchar_t dst[4];
    wchar_t src[4];

    if (!p_wcsncat_s)
    {
        win_skip("skipping wcsncat_s tests\n");
        return;
    }

    if(!p_set_invalid_parameter_handler) {
        win_skip("_set_invalid_parameter_handler not found\n");
        return;
    }

    memcpy(src, abcW, sizeof(abcW));
    dst[0] = 0;
    SET_EXPECT(invalid_parameter_handler);
    ret = p_wcsncat_s(NULL, 4, src, 4);
    ok(ret == EINVAL, "err = %d\n", ret);
    CHECK_CALLED(invalid_parameter_handler);

    SET_EXPECT(invalid_parameter_handler);
    ret = p_wcsncat_s(dst, 0, src, 4);
    ok(ret == EINVAL, "err = %d\n", ret);
    CHECK_CALLED(invalid_parameter_handler);

    SET_EXPECT(invalid_parameter_handler);
    ret = p_wcsncat_s(dst, 0, src, _TRUNCATE);
    ok(ret == EINVAL, "err = %d\n", ret);
    CHECK_CALLED(invalid_parameter_handler);

    ret = p_wcsncat_s(dst, 4, NULL, 0);
    ok(ret == 0, "err = %d\n", ret);

    dst[0] = 0;
    SET_EXPECT(invalid_parameter_handler);
    ret = p_wcsncat_s(dst, 2, src, 4);
    ok(ret == ERANGE, "err = %d\n", ret);
    CHECK_CALLED(invalid_parameter_handler);

    dst[0] = 0;
    ret = p_wcsncat_s(dst, 2, src, _TRUNCATE);
    ok(ret == STRUNCATE, "err = %d\n", ret);
    ok(dst[0] == 'a' && dst[1] == 0, "dst is %s\n", wine_dbgstr_w(dst));

    memcpy(dst, abcW, sizeof(abcW));
    dst[3] = 'd';
    SET_EXPECT(invalid_parameter_handler);
    ret = p_wcsncat_s(dst, 4, src, 4);
    ok(ret == EINVAL, "err = %d\n", ret);
    CHECK_CALLED(invalid_parameter_handler);
}

/* Based on dlls/ntdll/tests/string.c */
static __cdecl int intcomparefunc(void *context, const void *a, const void*b)
{
    ok (a != b, "must never get the same pointer\n");
    ++*(int *) context;
    return (*(int*)a) - (*(int*)b);
}

static __cdecl int charcomparefunc(void *context, const void *a, const void*b)
{
    ok (a != b, "must never get the same pointer\n");
    ++*(int *) context;
    return (*(char*)a) - (*(char*)b);
}

static __cdecl int strcomparefunc(void *context, const void *a, const void*b)
{
    ok (a != b, "must never get the same pointer\n");
    ++*(int *) context;
    return lstrcmpA(*(char**)a,*(char**)b);
}

static void test_qsort_s(void)
{
    int arr[5] = { 23, 42, 8, 4, 16 };
    int arr2[5] = { 23, 42, 8, 4, 16 };
    char carr[5] = { 42, 23, 4, 8, 16 };
    const char *strarr[7] = {
    "Hello",
    "Wine",
    "World",
    "!",
    "Hopefully",
    "Sorted",
    "."
    };

    if(!p_qsort_s) {
        win_skip("qsort_s not found\n");
        return;
    }

    SET_EXPECT(invalid_parameter_handler);
    p_qsort_s(NULL, 0, 0, NULL, NULL);
    CHECK_CALLED(invalid_parameter_handler);

    SET_EXPECT(invalid_parameter_handler);
    p_qsort_s(NULL, 0, 0, intcomparefunc, NULL);
    CHECK_CALLED(invalid_parameter_handler);

    SET_EXPECT(invalid_parameter_handler);
    p_qsort_s(NULL, 0, sizeof(int), NULL, NULL);
    CHECK_CALLED(invalid_parameter_handler);

    SET_EXPECT(invalid_parameter_handler);
    p_qsort_s(NULL, 1, sizeof(int), intcomparefunc, NULL);
    CHECK_CALLED(invalid_parameter_handler);

    g_qsort_s_context_counter = 0;
    p_qsort_s(NULL, 0, sizeof(int), intcomparefunc, NULL);
    ok(g_qsort_s_context_counter == 0, "callback shouldn't have been called\n");

    /* overflow without side effects, other overflow values crash */
    g_qsort_s_context_counter = 0;
    p_qsort_s((void*)arr2, (1 << (8*sizeof(size_t) - 1)) + 1, sizeof(int), intcomparefunc, &g_qsort_s_context_counter);
    ok(g_qsort_s_context_counter == 0, "callback shouldn't have been called\n");
    ok(arr2[0] == 23, "should remain unsorted, arr2[0] is %d\n", arr2[0]);
    ok(arr2[1] == 42, "should remain unsorted, arr2[1] is %d\n", arr2[1]);
    ok(arr2[2] == 8,  "should remain unsorted, arr2[2] is %d\n", arr2[2]);
    ok(arr2[3] == 4,  "should remain unsorted, arr2[3] is %d\n", arr2[3]);

    g_qsort_s_context_counter = 0;
    p_qsort_s((void*)arr, 0, sizeof(int), intcomparefunc, &g_qsort_s_context_counter);
    ok(g_qsort_s_context_counter == 0, "callback shouldn't have been called\n");
    ok(arr[0] == 23, "badly sorted, nmemb=0, arr[0] is %d\n", arr[0]);
    ok(arr[1] == 42, "badly sorted, nmemb=0, arr[1] is %d\n", arr[1]);
    ok(arr[2] == 8,  "badly sorted, nmemb=0, arr[2] is %d\n", arr[2]);
    ok(arr[3] == 4,  "badly sorted, nmemb=0, arr[3] is %d\n", arr[3]);
    ok(arr[4] == 16, "badly sorted, nmemb=0, arr[4] is %d\n", arr[4]);

    g_qsort_s_context_counter = 0;
    p_qsort_s((void*)arr, 1, sizeof(int), intcomparefunc, &g_qsort_s_context_counter);
    ok(g_qsort_s_context_counter == 0, "callback shouldn't have been called\n");
    ok(arr[0] == 23, "badly sorted, nmemb=1, arr[0] is %d\n", arr[0]);
    ok(arr[1] == 42, "badly sorted, nmemb=1, arr[1] is %d\n", arr[1]);
    ok(arr[2] == 8,  "badly sorted, nmemb=1, arr[2] is %d\n", arr[2]);
    ok(arr[3] == 4,  "badly sorted, nmemb=1, arr[3] is %d\n", arr[3]);
    ok(arr[4] == 16, "badly sorted, nmemb=1, arr[4] is %d\n", arr[4]);

    SET_EXPECT(invalid_parameter_handler);
    g_qsort_s_context_counter = 0;
    p_qsort_s((void*)arr, 5, 0, intcomparefunc, &g_qsort_s_context_counter);
    ok(g_qsort_s_context_counter == 0, "callback shouldn't have been called\n");
    ok(arr[0] == 23, "badly sorted, size=0, arr[0] is %d\n", arr[0]);
    ok(arr[1] == 42, "badly sorted, size=0, arr[1] is %d\n", arr[1]);
    ok(arr[2] == 8,  "badly sorted, size=0, arr[2] is %d\n", arr[2]);
    ok(arr[3] == 4,  "badly sorted, size=0, arr[3] is %d\n", arr[3]);
    ok(arr[4] == 16, "badly sorted, size=0, arr[4] is %d\n", arr[4]);
    CHECK_CALLED(invalid_parameter_handler);

    g_qsort_s_context_counter = 0;
    p_qsort_s((void*)arr, 5, sizeof(int), intcomparefunc, &g_qsort_s_context_counter);
    ok(g_qsort_s_context_counter > 0, "callback wasn't called\n");
    ok(arr[0] == 4,  "badly sorted, arr[0] is %d\n", arr[0]);
    ok(arr[1] == 8,  "badly sorted, arr[1] is %d\n", arr[1]);
    ok(arr[2] == 16, "badly sorted, arr[2] is %d\n", arr[2]);
    ok(arr[3] == 23, "badly sorted, arr[3] is %d\n", arr[3]);
    ok(arr[4] == 42, "badly sorted, arr[4] is %d\n", arr[4]);

    g_qsort_s_context_counter = 0;
    p_qsort_s((void*)carr, 5, sizeof(char), charcomparefunc, &g_qsort_s_context_counter);
    ok(g_qsort_s_context_counter > 0, "callback wasn't called\n");
    ok(carr[0] == 4,  "badly sorted, carr[0] is %d\n", carr[0]);
    ok(carr[1] == 8,  "badly sorted, carr[1] is %d\n", carr[1]);
    ok(carr[2] == 16, "badly sorted, carr[2] is %d\n", carr[2]);
    ok(carr[3] == 23, "badly sorted, carr[3] is %d\n", carr[3]);
    ok(carr[4] == 42, "badly sorted, carr[4] is %d\n", carr[4]);

    g_qsort_s_context_counter = 0;
    p_qsort_s((void*)strarr, 7, sizeof(char*), strcomparefunc, &g_qsort_s_context_counter);
    ok(g_qsort_s_context_counter > 0, "callback wasn't called\n");
    ok(!strcmp(strarr[0],"!"),  "badly sorted, strarr[0] is %s\n", strarr[0]);
    ok(!strcmp(strarr[1],"."),  "badly sorted, strarr[1] is %s\n", strarr[1]);
    ok(!strcmp(strarr[2],"Hello"),  "badly sorted, strarr[2] is %s\n", strarr[2]);
    ok(!strcmp(strarr[3],"Hopefully"),  "badly sorted, strarr[3] is %s\n", strarr[3]);
    ok(!strcmp(strarr[4],"Sorted"),  "badly sorted, strarr[4] is %s\n", strarr[4]);
    ok(!strcmp(strarr[5],"Wine"),  "badly sorted, strarr[5] is %s\n", strarr[5]);
    ok(!strcmp(strarr[6],"World"),  "badly sorted, strarr[6] is %s\n", strarr[6]);
}

/* ########## */

START_TEST(msvcr90)
{
    HMODULE hcrt;
    HMODULE hkernel32;

    SetLastError(0xdeadbeef);
    hcrt = LoadLibraryA("msvcr90.dll");
    if (!hcrt) {
        win_skip("msvcr90.dll not installed (got %d)\n", GetLastError());
        return;
    }

    p_set_invalid_parameter_handler = (void *) GetProcAddress(hcrt, "_set_invalid_parameter_handler");
    if(p_set_invalid_parameter_handler)
        ok(p_set_invalid_parameter_handler(test_invalid_parameter_handler) == NULL,
                "Invalid parameter handler was already set\n");

    p_initterm_e = (void *) GetProcAddress(hcrt, "_initterm_e");
    p_encode_pointer = (void *) GetProcAddress(hcrt, "_encode_pointer");
    p_decode_pointer = (void *) GetProcAddress(hcrt, "_decode_pointer");
    p_encoded_null = (void *) GetProcAddress(hcrt, "_encoded_null");
    p_sys_nerr = (void *) GetProcAddress(hcrt, "_sys_nerr");
    p__sys_nerr = (void *) GetProcAddress(hcrt, "__sys_nerr");
    p_sys_errlist = (void *) GetProcAddress(hcrt, "_sys_errlist");
    p__sys_errlist = (void *) GetProcAddress(hcrt, "__sys_errlist");
    p_strtoi64 = (void *) GetProcAddress(hcrt, "_strtoi64");
    p_strtoui64 = (void *) GetProcAddress(hcrt, "_strtoui64");
    p_itoa_s = (void *)GetProcAddress(hcrt, "_itoa_s");
    p_wcsncat_s = (void *)GetProcAddress( hcrt,"wcsncat_s" );
    p_qsort_s = (void *) GetProcAddress(hcrt, "qsort_s");

    hkernel32 = GetModuleHandleA("kernel32.dll");
    pEncodePointer = (void *) GetProcAddress(hkernel32, "EncodePointer");

    test__initterm_e();
    test__encode_pointer();
    test_error_messages();
    test__strtoi64();
    test__itoa_s();
    test_wcsncat_s();
    test_qsort_s();
}
