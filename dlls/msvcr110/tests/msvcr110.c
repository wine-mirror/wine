/*
 * Copyright 2018 Daniel Lehman
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
#include <float.h>
#include <limits.h>

#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include "wine/test.h"

#include <locale.h>

static char* (CDECL *p_setlocale)(int category, const char* locale);

static BOOL init(void)
{
    HMODULE module;

    module = LoadLibraryA("msvcr110.dll");
    if (!module)
    {
        win_skip("msvcr110.dll not installed\n");
        return FALSE;
    }

    p_setlocale = (void*)GetProcAddress(module, "setlocale");
    return TRUE;
}

static void test_setlocale(void)
{
    int i;
    char *ret;
    static const char *names[] =
    {
        "en-us",
        "en-US",
        "EN-US",
        "syr-SY",
        "uz-Latn-uz",
    };

    for(i=0; i<ARRAY_SIZE(names); i++) {
        ret = p_setlocale(LC_ALL, names[i]);
        ok(ret != NULL, "expected success, but got NULL\n");
        ok(!strcmp(ret, names[i]), "expected %s, got %s\n", names[i], ret);
    }

    ret = p_setlocale(LC_ALL, "en-us.1250");
    ok(!ret, "setlocale(en-us.1250) succeeded (%s)\n", ret);

    p_setlocale(LC_ALL, "C");
}

START_TEST(msvcr110)
{
    if (!init()) return;
    test_setlocale();
}
