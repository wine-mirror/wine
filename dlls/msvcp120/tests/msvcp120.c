/*
 * Copyright 2014 Yifu Wang for ESRI
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
#include "winbase.h"

typedef int MSVCRT_long;

/* xtime */
typedef struct {
    __time64_t sec;
    MSVCRT_long nsec;
} xtime;

static MSVCRT_long (__cdecl *p__Xtime_diff_to_millis2)(const xtime*, const xtime*);
static int (__cdecl *p_xtime_get)(xtime*, int);

static HMODULE msvcp;

static BOOL init(void)
{
    msvcp = LoadLibraryA("msvcp120.dll");
    if(!msvcp)
    {
        win_skip("msvcp120.dll not installed\n");
        return FALSE;
    }

    p__Xtime_diff_to_millis2 = (void*)GetProcAddress(msvcp, "_Xtime_diff_to_millis2");
    p_xtime_get = (void*)GetProcAddress(msvcp, "xtime_get");

    return TRUE;
}

static void test__Xtime_diff_to_millis2(void)
{
    struct {
        __time64_t sec_before;
        MSVCRT_long nsec_before;
        __time64_t sec_after;
        MSVCRT_long nsec_after;
        MSVCRT_long expect;
    } tests[] = {
        {1, 0, 2, 0, 1000},
        {0, 1000000000, 0, 2000000000, 1000},
        {1, 100000000, 2, 100000000, 1000},
        {1, 100000000, 1, 200000000, 100},
        {0, 0, 0, 1000000000, 1000},
        {0, 0, 0, 1200000000, 1200},
        {0, 0, 0, 1230000000, 1230},
        {0, 0, 0, 1234000000, 1234},
        {0, 0, 0, 1234100000, 1235},
        {0, 0, 0, 1234900000, 1235},
        {0, 0, 0, 1234010000, 1235},
        {0, 0, 0, 1234090000, 1235},
        {0, 0, 0, 1234000001, 1235},
        {0, 0, 0, 1234000009, 1235},
        {0, 0, -1, 0, 0},
        {0, 0, 0, -10000000, 0},
        {0, 0, -1, -100000000, 0},
        {-1, 0, 0, 0, 1000},
        {0, -100000000, 0, 0, 100},
        {-1, -100000000, 0, 0, 1100},
        {0, 0, -1, 2000000000, 1000},
        {0, 0, -2, 2000000000, 0},
        {0, 0, -2, 2100000000, 100}
    };
    int i;
    MSVCRT_long ret;
    xtime t1, t2;

    for(i = 0; i < sizeof(tests) / sizeof(tests[0]); ++ i)
    {
        t1.sec = tests[i].sec_before;
        t1.nsec = tests[i].nsec_before;
        t2.sec = tests[i].sec_after;
        t2.nsec = tests[i].nsec_after;
        ret = p__Xtime_diff_to_millis2(&t2, &t1);
        ok(ret == tests[i].expect,
                "_Xtime_diff_to_millis2(): test: %d expect: %d, got: %d\n",
                i, tests[i].expect, ret);
    }
}

static void test_xtime_get(void)
{
    static const MSVCRT_long tests[] = {1, 50, 100, 200, 500};
    MSVCRT_long diff;
    xtime before, after;
    int i;

    for(i = 0; i < sizeof(tests) / sizeof(tests[0]); i ++)
    {
        p_xtime_get(&before, 1);
        Sleep(tests[i]);
        p_xtime_get(&after, 1);

        diff = p__Xtime_diff_to_millis2(&after, &before);

        ok(diff >= tests[i],
                "xtime_get() not functioning correctly, test: %d, expect: ge %d, got: %d\n",
                i, tests[i], diff);
    }

    /* Test parameter and return value */
    before.sec = 0xdeadbeef, before.nsec = 0xdeadbeef;
    i = p_xtime_get(&before, 0);
    ok(i == 0, "expect xtime_get() to return 0, got: %d\n", i);
    ok(before.sec == 0xdeadbeef && before.nsec == 0xdeadbeef,
            "xtime_get() shouldn't have modified the xtime struct with the given option\n");

    before.sec = 0xdeadbeef, before.nsec = 0xdeadbeef;
    i = p_xtime_get(&before, 1);
    ok(i == 1, "expect xtime_get() to return 1, got: %d\n", i);
    ok(before.sec != 0xdeadbeef && before.nsec != 0xdeadbeef,
            "xtime_get() should have modified the xtime struct with the given option\n");
}

START_TEST(msvcp120)
{
    if(!init()) return;
    test__Xtime_diff_to_millis2();
    test_xtime_get();
}
