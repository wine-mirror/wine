/*
 * Unit test suite for locale functions.
 *
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

#include <locale.h>

#include "wine/test.h"
#include "winnls.h"

static BOOL (__cdecl *p__crtGetStringTypeW)(DWORD, DWORD, const wchar_t*, int, WORD*);
static int (__cdecl *pmemcpy_s)(void *, size_t, void*, size_t);

static void init(void)
{
    HMODULE hmod = GetModuleHandleA("msvcrt.dll");

    p__crtGetStringTypeW = (void*)GetProcAddress(hmod, "__crtGetStringTypeW");
    pmemcpy_s = (void*)GetProcAddress(hmod, "memcpy_s");
}

static void test_setlocale(void)
{
    static const char lc_all[] = "LC_COLLATE=C;LC_CTYPE=C;"
        "LC_MONETARY=Greek_Greece.1253;LC_NUMERIC=Polish_Poland.1250;LC_TIME=C";

    char *ret, buf[100];

    ret = setlocale(20, "C");
    ok(ret == NULL, "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "");
    ok(ret != NULL, "ret == NULL\n");

    ret = setlocale(LC_ALL, "C");
    ok(!strcmp(ret, "C"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, NULL);
    todo_wine ok(!strcmp(ret, "C"), "ret = %s\n", ret);

    if(!setlocale(LC_NUMERIC, "Polish")
            || !setlocale(LC_NUMERIC, "Greek")
            || !setlocale(LC_NUMERIC, "German")
            || !setlocale(LC_NUMERIC, "English")) {
        win_skip("System with limited locales\n");
        return;
    }

    ret = setlocale(LC_NUMERIC, "Polish");
    ok(!strcmp(ret, "Polish_Poland.1250"), "ret = %s\n", ret);

    ret = setlocale(LC_MONETARY, "Greek");
    ok(!strcmp(ret, "Greek_Greece.1253"), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, NULL);
    ok(!strcmp(ret, lc_all), "ret = %s\n", ret);

    strcpy(buf, ret);
    ret = setlocale(LC_ALL, buf);
    todo_wine ok(!strcmp(ret, lc_all), "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "German");
    todo_wine ok(!strcmp(ret, "German_Germany.1252"), "ret = %s\n", ret);

    /* This test shows that _country_synonims table is incorrect */
    /* It translates "America" to "US" */
    ret = setlocale(LC_ALL, "America");
    ok(ret == NULL, "ret = %s\n", ret);

    ret = setlocale(LC_ALL, "US");
    todo_wine ok(ret != NULL, "ret == NULL\n");
    if(ret)
        ok(!strcmp(ret, "English_United States.1252"), "ret = %s\n", ret);
}

static void test_crtGetStringTypeW(void)
{
    static const wchar_t str0[] = { '0', '\0' };
    static const wchar_t strA[] = { 'A', '\0' };
    static const wchar_t str_space[] = { ' ', '\0' };
    static const wchar_t str_null[] = { '\0', '\0' };
    static const wchar_t str_rand[] = { 1234, '\0' };

    const wchar_t *str[] = { str0, strA, str_space, str_null, str_rand };

    WORD out_crt, out;
    BOOL ret_crt, ret;
    int i;

    if(!p__crtGetStringTypeW) {
        win_skip("Skipping __crtGetStringTypeW tests\n");
        return;
    }

    if(!pmemcpy_s) {
        win_skip("To old version of msvcrt.dll\n");
        return;
    }

    for(i=0; i<sizeof(str)/sizeof(*str); i++) {
        ret_crt = p__crtGetStringTypeW(0, CT_CTYPE1, str[i], 1, &out_crt);
        ret = GetStringTypeW(CT_CTYPE1, str[i], 1, &out);
        ok(ret == ret_crt, "%d) ret_crt = %d\n", i, (int)ret_crt);
        ok(out == out_crt, "%d) out_crt = %x, expected %x\n", i, (int)out_crt, (int)out);

        ret_crt = p__crtGetStringTypeW(0, CT_CTYPE2, str[i], 1, &out_crt);
        ret = GetStringTypeW(CT_CTYPE2, str[i], 1, &out);
        ok(ret == ret_crt, "%d) ret_crt = %d\n", i, (int)ret_crt);
        ok(out == out_crt, "%d) out_crt = %x, expected %x\n", i, (int)out_crt, (int)out);

        ret_crt = p__crtGetStringTypeW(0, CT_CTYPE3, str[i], 1, &out_crt);
        ret = GetStringTypeW(CT_CTYPE3, str[i], 1, &out);
        ok(ret == ret_crt, "%d) ret_crt = %d\n", i, (int)ret_crt);
        ok(out == out_crt, "%d) out_crt = %x, expected %x\n", i, (int)out_crt, (int)out);
    }

    ret = p__crtGetStringTypeW(0, 3, str[0], 1, &out);
    ok(!ret, "ret == TRUE\n");
}

START_TEST(locale)
{
    init();

    test_crtGetStringTypeW();
    test_setlocale();
}
