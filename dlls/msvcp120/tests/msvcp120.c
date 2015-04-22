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

#include <locale.h>

#include "wine/test.h"
#include "winbase.h"

typedef int MSVCRT_long;

/* xtime */
typedef struct {
    __time64_t sec;
    MSVCRT_long nsec;
} xtime;

typedef struct {
    unsigned page;
    int mb_max;
    int unk;
    BYTE isleadbyte[32];
} _Cvtvec;

static char* (__cdecl *p_setlocale)(int, const char*);
static int (__cdecl *p__setmbcp)(int);
static int (__cdecl *p_isleadbyte)(int);

static MSVCRT_long (__cdecl *p__Xtime_diff_to_millis2)(const xtime*, const xtime*);
static int (__cdecl *p_xtime_get)(xtime*, int);
static _Cvtvec* (__cdecl *p__Getcvt)(_Cvtvec*);

static HMODULE msvcp;

static BOOL init(void)
{
    HANDLE msvcr;

    msvcp = LoadLibraryA("msvcp120.dll");
    if(!msvcp)
    {
        win_skip("msvcp120.dll not installed\n");
        return FALSE;
    }

    p__Xtime_diff_to_millis2 = (void*)GetProcAddress(msvcp, "_Xtime_diff_to_millis2");
    p_xtime_get = (void*)GetProcAddress(msvcp, "xtime_get");
    p__Getcvt = (void*)GetProcAddress(msvcp, "_Getcvt");

    msvcr = GetModuleHandleA("msvcr120.dll");
    p_setlocale = (void*)GetProcAddress(msvcr, "setlocale");
    p__setmbcp = (void*)GetProcAddress(msvcr, "_setmbcp");
    p_isleadbyte = (void*)GetProcAddress(msvcr, "isleadbyte");
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

static void test__Getcvt(void)
{
    _Cvtvec cvtvec;
    int i;

    p__Getcvt(&cvtvec);
    ok(cvtvec.page == 0, "cvtvec.page = %d\n", cvtvec.page);
    ok(cvtvec.mb_max == 1, "cvtvec.mb_max = %d\n", cvtvec.mb_max);
    todo_wine ok(cvtvec.unk == 1, "cvtvec.unk = %d\n", cvtvec.unk);
    for(i=0; i<32; i++)
        ok(cvtvec.isleadbyte[i] == 0, "cvtvec.isleadbyte[%d] = %x\n", i, cvtvec.isleadbyte[i]);

    if(!p_setlocale(LC_ALL, ".936")) {
        win_skip("_Getcvt tests\n");
        return;
    }
    p__Getcvt(&cvtvec);
    ok(cvtvec.page == 936, "cvtvec.page = %d\n", cvtvec.page);
    ok(cvtvec.mb_max == 2, "cvtvec.mb_max = %d\n", cvtvec.mb_max);
    ok(cvtvec.unk == 0, "cvtvec.unk = %d\n", cvtvec.unk);
    for(i=0; i<32; i++)
        ok(cvtvec.isleadbyte[i] == 0, "cvtvec.isleadbyte[%d] = %x\n", i, cvtvec.isleadbyte[i]);

    p__setmbcp(936);
    p__Getcvt(&cvtvec);
    ok(cvtvec.page == 936, "cvtvec.page = %d\n", cvtvec.page);
    ok(cvtvec.mb_max == 2, "cvtvec.mb_max = %d\n", cvtvec.mb_max);
    ok(cvtvec.unk == 0, "cvtvec.unk = %d\n", cvtvec.unk);
    for(i=0; i<32; i++) {
        BYTE b = 0;
        int j;

        for(j=0; j<8; j++)
            b |= (p_isleadbyte(i*8+j) ? 1 : 0) << j;
        ok(cvtvec.isleadbyte[i] ==b, "cvtvec.isleadbyte[%d] = %x (%x)\n", i, cvtvec.isleadbyte[i], b);
    }
}

START_TEST(msvcp120)
{
    if(!init()) return;
    test__Xtime_diff_to_millis2();
    test_xtime_get();
    test__Getcvt();
}
