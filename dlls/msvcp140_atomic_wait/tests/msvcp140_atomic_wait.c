/*
 * Copyright 2022 Daniel Lehman
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

#include "windef.h"
#include "winbase.h"
#include "wine/test.h"

static unsigned int (__stdcall *p___std_parallel_algorithms_hw_threads)(void);

#define SETNOFAIL(x,y) x = (void*)GetProcAddress(msvcp,y)
#define SET(x,y) do { SETNOFAIL(x,y); ok(x != NULL, "Export '%s' not found\n", y); } while(0)
static HMODULE init(void)
{
    HMODULE msvcp;

    if (!(msvcp = LoadLibraryA("msvcp140_atomic_wait.dll")))
        return NULL;

    SET(p___std_parallel_algorithms_hw_threads, "__std_parallel_algorithms_hw_threads");
    return msvcp;
}

static void test___std_parallel_algorithms_hw_threads(void)
{
    SYSTEM_INFO si;
    unsigned int nthr;

    GetSystemInfo(&si);
    nthr = p___std_parallel_algorithms_hw_threads();
    ok(nthr == si.dwNumberOfProcessors, "expected %u, got %u\n", si.dwNumberOfProcessors, nthr);
}

START_TEST(msvcp140_atomic_wait)
{
    HMODULE msvcp;
    if (!(msvcp = init()))
    {
        win_skip("msvcp140_atomic_wait.dll not installed\n");
        return;
    }
    test___std_parallel_algorithms_hw_threads();
    FreeLibrary(msvcp);
}
