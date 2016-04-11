/*
 * Copyright 2016 Daniel Lehman (Esri)
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

typedef unsigned char MSVCRT_bool;

typedef struct {
    const char  *what;
    MSVCRT_bool  dofree;
} __std_exception_data;

static void (CDECL *p___std_exception_copy)(const __std_exception_data*, __std_exception_data*);
static void (CDECL *p___std_exception_destroy)(__std_exception_data*);

static BOOL init(void)
{
    HMODULE module;

    module = LoadLibraryA("ucrtbase.dll");
    if (!module)
    {
        win_skip("ucrtbase.dll not installed\n");
        return FALSE;
    }

    p___std_exception_copy = (void*)GetProcAddress(module, "__std_exception_copy");
    p___std_exception_destroy = (void*)GetProcAddress(module, "__std_exception_destroy");
    return TRUE;
}

static void test___std_exception(void)
{
    __std_exception_data src;
    __std_exception_data dst;

    if (0) /* crash on Windows */
    {
        p___std_exception_copy(NULL, &src);
        p___std_exception_copy(&dst, NULL);

        src.what   = "invalid free";
        src.dofree = 1;
        p___std_exception_destroy(&src);
        p___std_exception_destroy(NULL);
    }

    src.what   = "what";
    src.dofree = 0;
    p___std_exception_copy(&src, &dst);
    ok(dst.what == src.what, "expected what to be same, got src %p dst %p\n", src.what, dst.what);
    ok(!dst.dofree, "expected 0, got %d\n", dst.dofree);

    src.dofree = 0x42;
    p___std_exception_copy(&src, &dst);
    ok(dst.what != src.what, "expected what to be different, got src %p dst %p\n", src.what, dst.what);
    ok(dst.dofree == 1, "expected 1, got %d\n", dst.dofree);

    p___std_exception_destroy(&dst);
    ok(!dst.what, "expected NULL, got %p\n", dst.what);
    ok(!dst.dofree, "expected 0, got %d\n", dst.dofree);

    src.what = NULL;
    src.dofree = 0;
    p___std_exception_copy(&src, &dst);
    ok(!dst.what, "dst.what != NULL\n");
    ok(!dst.dofree, "dst.dofree != FALSE\n");

    src.what = NULL;
    src.dofree = 1;
    p___std_exception_copy(&src, &dst);
    ok(!dst.what, "dst.what != NULL\n");
    ok(!dst.dofree, "dst.dofree != FALSE\n");
}

START_TEST(cpp)
{
    if (!init()) return;
    test___std_exception();
}
