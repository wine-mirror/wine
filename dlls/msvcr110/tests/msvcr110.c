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
static size_t (CDECL *p___strncnt)(const char *str, size_t count);

static unsigned int (CDECL *p_CurrentScheduler_GetNumberOfVirtualProcessors)(void);
static unsigned int (CDECL *p__CurrentScheduler__GetNumberOfVirtualProcessors)(void);
static unsigned int (CDECL *p_CurrentScheduler_Id)(void);
static unsigned int (CDECL *p__CurrentScheduler__Id)(void);

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
    p___strncnt = (void*)GetProcAddress(module, "__strncnt");
    p_CurrentScheduler_GetNumberOfVirtualProcessors = (void*)GetProcAddress(module, "?GetNumberOfVirtualProcessors@CurrentScheduler@Concurrency@@SAIXZ");
    p__CurrentScheduler__GetNumberOfVirtualProcessors = (void*)GetProcAddress(module, "?_GetNumberOfVirtualProcessors@_CurrentScheduler@details@Concurrency@@SAIXZ");
    p_CurrentScheduler_Id = (void*)GetProcAddress(module, "?Id@CurrentScheduler@Concurrency@@SAIXZ");
    p__CurrentScheduler__Id = (void*)GetProcAddress(module, "?_Id@_CurrentScheduler@details@Concurrency@@SAIXZ");

    return TRUE;
}

static void test_CurrentScheduler(void)
{
    unsigned int id;
    unsigned int ncpus;
    unsigned int expect;
    SYSTEM_INFO si;

    expect = ~0;
    ncpus = p_CurrentScheduler_GetNumberOfVirtualProcessors();
    ok(ncpus == expect, "expected %x, got %x\n", expect, ncpus);
    id = p_CurrentScheduler_Id();
    ok(id == expect, "expected %u, got %u\n", expect, id);

    GetSystemInfo(&si);
    expect = si.dwNumberOfProcessors;
    /* these _CurrentScheduler calls trigger scheduler creation
       if either is commented out, the following CurrentScheduler (no _) tests will still work */
    ncpus = p__CurrentScheduler__GetNumberOfVirtualProcessors();
    id = p__CurrentScheduler__Id();
    ok(ncpus == expect, "expected %u, got %u\n", expect, ncpus);
    ok(id == 0, "expected 0, got %u\n", id);

    /* these CurrentScheduler tests assume scheduler is created */
    ncpus = p_CurrentScheduler_GetNumberOfVirtualProcessors();
    ok(ncpus == expect, "expected %u, got %u\n", expect, ncpus);
    id = p_CurrentScheduler_Id();
    ok(id == 0, "expected 0, got %u\n", id);
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

static void test___strncnt(void)
{
    static const struct
    {
        const char *str;
        size_t size;
        size_t ret;
    }
    strncnt_tests[] =
    {
        { NULL, 0, 0 },
        { "a", 0, 0 },
        { "a", 1, 1 },
        { "a", 10, 1 },
        { "abc", 1, 1 },
    };
    unsigned int i;
    size_t ret;

    if (0) /* crashes */
        ret = p___strncnt(NULL, 1);

    for (i = 0; i < ARRAY_SIZE(strncnt_tests); ++i)
    {
        ret = p___strncnt(strncnt_tests[i].str, strncnt_tests[i].size);
        ok(ret == strncnt_tests[i].ret, "%u: unexpected return value %u.\n", i, (int)ret);
    }
}

START_TEST(msvcr110)
{
    if (!init()) return;
    test_CurrentScheduler(); /* MUST be first (at least among Concurrency tests) */
    test_setlocale();
    test___strncnt();
}
