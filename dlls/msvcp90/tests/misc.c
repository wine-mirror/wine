/*
 * Copyright 2010 Piotr Caban for CodeWeavers
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

#include <windef.h>
#include <winbase.h>
#include "wine/test.h"

static void* (__cdecl *p_set_invalid_parameter_handler)(void*);

static void (__cdecl *p_char_assign)(void*, const void*);
static void (__cdecl *p_wchar_assign)(void*, const void*);
static void (__cdecl *p_short_assign)(void*, const void*);

static BYTE (__cdecl *p_char_eq)(const void*, const void*);
static BYTE (__cdecl *p_wchar_eq)(const void*, const void*);
static BYTE (__cdecl *p_short_eq)(const void*, const void*);

static char* (__cdecl *p_Copy_s)(char*, unsigned int, const char*, unsigned int);

static int invalid_parameter = 0;
static void __cdecl test_invalid_parameter_handler(const wchar_t *expression,
        const wchar_t *function, const wchar_t *file,
        unsigned line, uintptr_t arg)
{
    ok(expression == NULL, "expression is not NULL\n");
    ok(function == NULL, "function is not NULL\n");
    ok(file == NULL, "file is not NULL\n");
    ok(line == 0, "line = %u\n", line);
    ok(arg == 0, "arg = %lx\n", (UINT_PTR)arg);
    invalid_parameter++;
}

static BOOL init(void)
{
    HMODULE msvcr = LoadLibraryA("msvcr90.dll");
    HMODULE msvcp = LoadLibraryA("msvcp90.dll");
    if(!msvcr || !msvcp) {
        win_skip("msvcp90.dll or msvcrt90.dll not installed\n");
        return FALSE;
    }

    p_set_invalid_parameter_handler = (void*)GetProcAddress(msvcr, "_set_invalid_parameter_handler");
    if(!p_set_invalid_parameter_handler) {
        win_skip("Error setting tests environment\n");
        return FALSE;
    }

    p_set_invalid_parameter_handler(test_invalid_parameter_handler);

    p_char_assign = (void*)GetProcAddress(msvcp, "?assign@?$char_traits@D@std@@SAXAADABD@Z");
    p_wchar_assign = (void*)GetProcAddress(msvcp, "?assign@?$char_traits@_W@std@@SAXAA_WAB_W@Z");
    p_short_assign = (void*)GetProcAddress(msvcp, "?assign@?$char_traits@G@std@@SAXAAGABG@Z");

    p_char_eq = (void*)GetProcAddress(msvcp, "?eq@?$char_traits@D@std@@SA_NABD0@Z");
    p_wchar_eq = (void*)GetProcAddress(msvcp, "?eq@?$char_traits@_W@std@@SA_NAB_W0@Z");
    p_short_eq = (void*)GetProcAddress(msvcp, "?eq@?$char_traits@G@std@@SA_NABG0@Z");

    p_Copy_s = (void*)GetProcAddress(msvcp, "?_Copy_s@?$char_traits@D@std@@SAPADPADIPBDI@Z");

    return TRUE;
}

static void test_assign(void)
{
    const char in[] = "abc";
    char out[4];

    if(!p_char_assign || !p_wchar_assign || !p_short_assign) {
        win_skip("assign tests skipped\n");
        return;
    }

    out[1] = '#';
    p_char_assign(out, in);
    ok(out[0] == in[0], "out[0] = %c\n", out[0]);
    ok(out[1] == '#', "out[1] = %c\n", out[1]);

    out[2] = '#';
    p_wchar_assign(out, in);
    ok(*((char*)out)==in[0] && *((char*)out+1)==in[1],
            "out[0] = %d, out[1] = %d\n", (int)out[0], (int)out[1]);
    ok(out[2] == '#', "out[2] = %c\n", out[2]);

    out[2] = '#';
    p_short_assign(out, in);
    ok(*((char*)out)==in[0] && *((char*)out+1)==in[1],
            "out[0] = %d, out[1] = %d\n", (int)out[0], (int)out[1]);
    ok(out[2] == '#', "out[2] = %c\n", out[2]);
}

static void test_equal(void)
{
    static const char in1[] = "abc";
    static const char in2[] = "ab";
    static const char in3[] = "a";
    static const char in4[] = "b";
    BYTE ret;

    if(!p_char_eq || !p_wchar_eq || !p_short_eq) {
        win_skip("equal tests skipped\n");
        return;
    }

    ret = p_char_eq(in1, in2);
    ok(ret == TRUE, "ret = %d\n", (int)ret);
    ret = p_char_eq(in1, in3);
    ok(ret == TRUE, "ret = %d\n", (int)ret);
    ret = p_char_eq(in1, in4);
    ok(ret == FALSE, "ret = %d\n", (int)ret);

    ret = p_wchar_eq(in1, in2);
    ok(ret == TRUE, "ret = %d\n", (int)ret);
    ret = p_wchar_eq(in1, in3);
    ok(ret == FALSE, "ret = %d\n", (int)ret);
    ret = p_wchar_eq(in1, in4);
    ok(ret == FALSE, "ret = %d\n", (int)ret);

    ret = p_short_eq(in1, in2);
    ok(ret == TRUE, "ret = %d\n", (int)ret);
    ret = p_short_eq(in1, in3);
    ok(ret == FALSE, "ret = %d\n", (int)ret);
    ret = p_short_eq(in1, in4);
    ok(ret == FALSE, "ret = %d\n", (int)ret);
}

static void test_Copy_s(void)
{
    static const char src[] = "abcd";
    char dest[32], *ret;

    if(!p_Copy_s) {
        win_skip("Copy_s tests skipped\n");
        return;
    }

    dest[4] = '#';
    dest[5] = '\0';
    ret = p_Copy_s(dest, 4, src, 4);
    ok(ret == dest, "ret != dest\n");
    ok(dest[4] == '#', "dest[4] != '#'\n");
    ok(!memcmp(dest, src, sizeof(char[4])), "dest = %s\n", dest);

    ret = p_Copy_s(dest, 32, src, 4);
    ok(ret == dest, "ret != dest\n");
    ok(dest[4] == '#', "dest[4] != '#'\n");
    ok(!memcmp(dest, src, sizeof(char[4])), "dest = %s\n", dest);

    errno = 0xdeadbeef;
    dest[0] = '#';
    ret = p_Copy_s(dest, 3, src, 4);
    ok(ret == dest, "ret != dest\n");
    ok(dest[0] == '\0', "dest[0] != 0\n");
    ok(invalid_parameter==1, "invalid_parameter = %d\n",
            invalid_parameter);
    invalid_parameter = 0;
    ok(errno == 0xdeadbeef, "errno = %d\n", errno);

    errno = 0xdeadbeef;
    p_Copy_s(NULL, 32, src, 4);
    ok(invalid_parameter==1, "invalid_parameter = %d\n",
            invalid_parameter);
    invalid_parameter = 0;
    ok(errno == 0xdeadbeef, "errno = %d\n", errno);

    errno = 0xdeadbeef;
    p_Copy_s(dest, 32, NULL, 4);
    ok(invalid_parameter==1, "invalid_parameter = %d\n",
            invalid_parameter);
    invalid_parameter = 0;
    ok(errno == 0xdeadbeef, "errno = %d\n", errno);
}

START_TEST(misc)
{
    if(!init())
        return;

    test_assign();
    test_equal();
    test_Copy_s();

    ok(!invalid_parameter, "invalid_parameter_handler was invoked too many times\n");
}
