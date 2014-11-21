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

#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <wchar.h>
#include <stdio.h>

#include <windef.h>
#include <winbase.h>
#include "wine/test.h"

#include <locale.h>

struct MSVCRT_lconv
{
    char* decimal_point;
    char* thousands_sep;
    char* grouping;
    char* int_curr_symbol;
    char* currency_symbol;
    char* mon_decimal_point;
    char* mon_thousands_sep;
    char* mon_grouping;
    char* positive_sign;
    char* negative_sign;
    char int_frac_digits;
    char frac_digits;
    char p_cs_precedes;
    char p_sep_by_space;
    char n_cs_precedes;
    char n_sep_by_space;
    char p_sign_posn;
    char n_sign_posn;
    wchar_t* _W_decimal_point;
    wchar_t* _W_thousands_sep;
    wchar_t* _W_int_curr_symbol;
    wchar_t* _W_currency_symbol;
    wchar_t* _W_mon_decimal_point;
    wchar_t* _W_mon_thousands_sep;
    wchar_t* _W_positive_sign;
    wchar_t* _W_negative_sign;
};

static char* (CDECL *p_setlocale)(int category, const char* locale);
static struct MSVCRT_lconv* (CDECL *p_localeconv)(void);
static size_t (CDECL *p_wcstombs_s)(size_t *ret, char* dest, size_t sz, const wchar_t* src, size_t max);

static BOOL init(void)
{
    HMODULE module;

    module = LoadLibraryA("msvcr120.dll");
    if (!module)
    {
        win_skip("msvcr120.dll not installed\n");
        return FALSE;
    }

    p_setlocale = (void*)GetProcAddress(module, "setlocale");
    p_localeconv = (void*)GetProcAddress(module, "localeconv");
    p_wcstombs_s = (void*)GetProcAddress(module, "wcstombs_s");
    return TRUE;
}

void test_lconv_helper(const char *locstr)
{
    struct MSVCRT_lconv *lconv;
    char mbs[256];
    size_t i;

    if(!p_setlocale(LC_ALL, locstr))
    {
        win_skip("locale %s not available\n", locstr);
        return;
    }

    lconv = p_localeconv();

    /* If multi-byte version available, asserts that wide char version also available.
     * If wide char version can be converted to a multi-byte string , asserts that the
     * conversion result is the same as the multi-byte version.
     */
    if(strlen(lconv->decimal_point) > 0)
        ok(wcslen(lconv->_W_decimal_point) > 0, "%s: decimal_point\n", locstr);
    if(p_wcstombs_s(&i, mbs, 256, lconv->_W_decimal_point, 256) == 0)
        ok(strcmp(mbs, lconv->decimal_point) == 0, "%s: decimal_point\n", locstr);

    if(strlen(lconv->thousands_sep) > 0)
        ok(wcslen(lconv->_W_thousands_sep) > 0, "%s: thousands_sep\n", locstr);
    if(p_wcstombs_s(&i, mbs, 256, lconv->_W_thousands_sep, 256) == 0)
        ok(strcmp(mbs, lconv->thousands_sep) == 0, "%s: thousands_sep\n", locstr);

    if(strlen(lconv->int_curr_symbol) > 0)
        ok(wcslen(lconv->_W_int_curr_symbol) > 0, "%s: int_curr_symbol\n", locstr);
    if(p_wcstombs_s(&i, mbs, 256, lconv->_W_int_curr_symbol, 256) == 0)
        ok(strcmp(mbs, lconv->int_curr_symbol) == 0, "%s: int_curr_symbol\n", locstr);

    if(strlen(lconv->currency_symbol) > 0)
        ok(wcslen(lconv->_W_currency_symbol) > 0, "%s: currency_symbol\n", locstr);
    if(p_wcstombs_s(&i, mbs, 256, lconv->_W_currency_symbol, 256) == 0)
        ok(strcmp(mbs, lconv->currency_symbol) == 0, "%s: currency_symbol\n", locstr);

    if(strlen(lconv->mon_decimal_point) > 0)
        ok(wcslen(lconv->_W_mon_decimal_point) > 0, "%s: decimal_point\n", locstr);
    if(p_wcstombs_s(&i, mbs, 256, lconv->_W_mon_decimal_point, 256) == 0)
        ok(strcmp(mbs, lconv->mon_decimal_point) == 0, "%s: decimal_point\n", locstr);

    if(strlen(lconv->positive_sign) > 0)
        ok(wcslen(lconv->_W_positive_sign) > 0, "%s: positive_sign\n", locstr);
    if(p_wcstombs_s(&i, mbs, 256, lconv->_W_positive_sign, 256) == 0)
        ok(strcmp(mbs, lconv->positive_sign) == 0, "%s: positive_sign\n", locstr);

    if(strlen(lconv->negative_sign) > 0)
        ok(wcslen(lconv->_W_negative_sign) > 0, "%s: negative_sign\n", locstr);
    if(p_wcstombs_s(&i, mbs, 256, lconv->_W_negative_sign, 256) == 0)
        ok(strcmp(mbs, lconv->negative_sign) == 0, "%s: negative_sign\n", locstr);
}

void test_lconv(void)
{
    int i;
    const char *locstrs[] =
    {
        "American", "Belgian", "Chinese",
        "Dutch", "English", "French",
        "German", "Hungarian", "Icelandic",
        "Japanese", "Korean", "Spanish"
    };

    for(i = 0; i < sizeof(locstrs) / sizeof(char *); i ++)
        test_lconv_helper(locstrs[i]);
}

START_TEST(msvcr120)
{
    if (!init()) return;
    test_lconv();
}
