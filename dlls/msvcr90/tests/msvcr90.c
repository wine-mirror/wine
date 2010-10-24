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

static void* (WINAPI *pEncodePointer)(void *);

static int cb_called[4];

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

    hkernel32 = GetModuleHandleA("kernel32.dll");
    pEncodePointer = (void *) GetProcAddress(hkernel32, "EncodePointer");

    test__initterm_e();
    test__encode_pointer();
    test_error_messages();
    test__strtoi64();
}
