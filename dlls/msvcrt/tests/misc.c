/*
 * Unit tests for miscellaneous msvcrt functions
 *
 * Copyright 2010 Andrew Nguyen
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

#include "wine/test.h"
#include <errno.h>
#include "msvcrt.h"

static int (__cdecl *prand_s)(unsigned int *);
static int (__cdecl *memcpy_s)(void *, MSVCRT_size_t, void*, MSVCRT_size_t);

static void init(void)
{
    HMODULE hmod = GetModuleHandleA("msvcrt.dll");

    prand_s = (void *)GetProcAddress(hmod, "rand_s");
    memcpy_s = (void*)GetProcAddress(hmod, "memcpy_s");
}

static void test_rand_s(void)
{
    int ret;
    unsigned int rand;

    if (!prand_s)
    {
        win_skip("rand_s is not available\n");
        return;
    }

    errno = EBADF;
    ret = prand_s(NULL);
    ok(ret == EINVAL, "Expected rand_s to return EINVAL, got %d\n", ret);
    ok(errno == EINVAL, "Expected errno to return EINVAL, got %d\n", errno);

    ret = prand_s(&rand);
    ok(ret == 0, "Expected rand_s to return 0, got %d\n", ret);
}

static void test_memcpy_s(void)
{
    static char data[] = "data\0to\0be\0copied";
    static char dest[32];
    int ret;

    if(!memcpy_s)
    {
        win_skip("memcpy_s in not available\n");
        return;
    }

    errno = 0xdeadbeef;
    ret = memcpy_s(NULL, 0, NULL, 0);
    ok(ret == 0, "ret = %x\n", ret);
    ok(errno == 0xdeadbeef, "errno = %x\n", errno);

    errno = 0xdeadbeef;
    dest[0] = 'x';
    ret = memcpy_s(dest, 10, NULL, 0);
    ok(ret == 0, "ret = %x\n", ret);
    ok(errno == 0xdeadbeef, "errno = %x\n", errno);
    ok(dest[0] == 'x', "dest[0] != \'x\'\n");

    errno = 0xdeadbeef;
    ret = memcpy_s(NULL, 10, data, 10);
    ok(ret == EINVAL, "ret = %x\n", ret);
    ok(errno == EINVAL, "errno = %x\n", errno);

    errno = 0xdeadbeef;
    dest[7] = 'x';
    ret = memcpy_s(dest, 10, data, 5);
    ok(ret == 0, "ret = %x\n", ret);
    ok(errno == 0xdeadbeef, "errno = %x\n", errno);
    ok(memcmp(dest, data, 10), "All data copied\n");
    ok(!memcmp(dest, data, 5), "First five bytes are different\n");

    errno = 0xdeadbeef;
    ret = memcpy_s(data, 10, data, 10);
    ok(ret == 0, "ret = %x\n", ret);
    ok(errno == 0xdeadbeef, "errno = %x\n", errno);
    ok(!memcmp(dest, data, 5), "data was destroyed during overwritting\n");

    errno = 0xdeadbeef;
    dest[0] = 'x';
    ret = memcpy_s(dest, 5, data, 10);
    ok(ret == ERANGE, "ret = %x\n", ret);
    ok(errno == ERANGE, "errno = %x\n", errno);
    ok(dest[0] == '\0', "dest[0] != \'\\0\'\n");
}

START_TEST(misc)
{
    init();

    test_rand_s();
    test_memcpy_s();
}
