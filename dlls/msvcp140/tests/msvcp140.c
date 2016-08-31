/*
 * Copyright 2016 Daniel Lehman
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

#include "wine/test.h"
#include "winbase.h"

static unsigned int (__cdecl *p__Thrd_id)(void);

static HMODULE msvcp;
#define SETNOFAIL(x,y) x = (void*)GetProcAddress(msvcp,y)
#define SET(x,y) do { SETNOFAIL(x,y); ok(x != NULL, "Export '%s' not found\n", y); } while(0)
static BOOL init(void)
{
    msvcp = LoadLibraryA("msvcp140.dll");
    if(!msvcp)
    {
        win_skip("msvcp140.dll not installed\n");
        return FALSE;
    }

    SET(p__Thrd_id, "_Thrd_id");

    return TRUE;
}

static void test_thrd(void)
{
    ok(p__Thrd_id() == GetCurrentThreadId(),
        "expected same id, got _Thrd_id %u GetCurrentThreadId %u\n",
        p__Thrd_id(), GetCurrentThreadId());
}

START_TEST(msvcp140)
{
    if(!init()) return;
    test_thrd();
    FreeLibrary(msvcp);
}
