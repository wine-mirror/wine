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

typedef int (__cdecl *_INITTERM_E_FN)(void);
static int (__cdecl *p_initterm_e)(_INITTERM_E_FN *table, _INITTERM_E_FN *end);
static void* (__cdecl *p_encode_pointer)(void *);
static void* (__cdecl *p_decode_pointer)(void *);
int cb_called[4];

/* ########## */

static int initterm_cb0(void)
{
    cb_called[0]++;
    return 0;
}

static int initterm_cb1(void)
{
    cb_called[1]++;
    return 1;
}

static int initterm_cb2(void)
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

static void test__encode_pointer(void)
{
    void *ptr, *res;

    if(!p_encode_pointer || !p_decode_pointer) {
        win_skip("_encode_pointer or _decode_pointer not found\n");
        return;
    }

    ptr = (void*)0xdeadbeef;
    res = p_encode_pointer(ptr);
    res = p_decode_pointer(res);
    ok(res == ptr, "Pointers are different after encoding and decoding\n");

    ptr = p_encode_pointer(p_encode_pointer);
    res = EncodePointer(p_encode_pointer);
    ok(ptr == res, "_encode_pointer produced different result than EncodePointer\n");
    ok(p_decode_pointer(ptr) == p_encode_pointer, "Error decoding pointer\n");
}

/* ########## */

START_TEST(msvcr90)
{
    HMODULE hcrt;

    SetLastError(0xdeadbeef);
    hcrt = LoadLibraryA("msvcr90.dll");
    if (!hcrt) {
        win_skip("msvcr90.dll not installed (got %d)\n", GetLastError());
        return;
    }

    p_initterm_e = (void *) GetProcAddress(hcrt, "_initterm_e");
    p_encode_pointer = (void *) GetProcAddress(hcrt, "_encode_pointer");
    p_decode_pointer = (void *) GetProcAddress(hcrt, "_decode_pointer");

    test__initterm_e();
    test__encode_pointer();
}
