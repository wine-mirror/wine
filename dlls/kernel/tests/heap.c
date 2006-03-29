/*
 * Unit test suite for heap functions
 *
 * Copyright 2003 Dimitrie O. Paun
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "wine/test.h"

#define MAGIC_DEAD 0xdeadbeef

static SIZE_T resize_9x(SIZE_T size)
{
    DWORD dwSizeAligned = (size + 3) & ~3;
    return max(dwSizeAligned, 12); /* at least 12 bytes */
}

START_TEST(heap)
{
    void *mem;
    UINT    flags;
    HGLOBAL gbl;
    HGLOBAL hsecond;
    SIZE_T  size;

    /* Heap*() functions */
    mem = HeapAlloc(GetProcessHeap(), 0, 0);
    ok(mem != NULL, "memory not allocated for size 0\n");

    mem = HeapReAlloc(GetProcessHeap(), 0, NULL, 10);
    ok(mem == NULL, "memory allocated by HeapReAlloc\n");

    for (size = 0; size <= 256; size++)
    {
        SIZE_T heap_size;
        mem = HeapAlloc(GetProcessHeap(), 0, size);
        heap_size = HeapSize(GetProcessHeap(), 0, mem);
        ok(heap_size == size || heap_size == resize_9x(size), 
            "HeapSize returned %lu instead of %lu or %lu\n", heap_size, size, resize_9x(size));
        HeapFree(GetProcessHeap(), 0, mem);
    }

    /* Global*() functions */
    gbl = GlobalAlloc(GMEM_MOVEABLE, 0);
    ok(gbl != NULL, "global memory not allocated for size 0\n");

    gbl = GlobalReAlloc(gbl, 10, GMEM_MOVEABLE);
    ok(gbl != NULL, "Can't realloc global memory\n");
    size = GlobalSize(gbl);
    ok(size >= 10 && size <= 16, "Memory not resized to size 10, instead size=%ld\n", size);

    gbl = GlobalReAlloc(gbl, 0, GMEM_MOVEABLE);
    ok(gbl != NULL, "GlobalReAlloc should not fail on size 0\n");

    size = GlobalSize(gbl);
    ok(size == 0, "Memory not resized to size 0, instead size=%ld\n", size);
    ok(GlobalFree(gbl) == NULL, "Memory not freed\n");
    size = GlobalSize(gbl);
    ok(size == 0, "Memory should have been freed, size=%ld\n", size);

    gbl = GlobalReAlloc(0, 10, GMEM_MOVEABLE);
    ok(gbl == NULL, "global realloc allocated memory\n");

    /* invalid handles are catched in windows */
    gbl = GlobalAlloc(GMEM_MOVEABLE, 256);
    GlobalFree(gbl);
    SetLastError(MAGIC_DEAD);
    hsecond = GlobalFree(gbl);      /* invalid handle: free memory twice */
    ok( (hsecond == gbl) && (GetLastError() == ERROR_INVALID_HANDLE),
        "returned %p with 0x%08lx (expected %p with ERROR_INVALID_HANDLE)\n",
        hsecond, GetLastError(), gbl);
    SetLastError(MAGIC_DEAD);
    flags = GlobalFlags(gbl);
    ok( (flags == GMEM_INVALID_HANDLE) && (GetLastError() == ERROR_INVALID_HANDLE),
        "returned 0x%04x with 0x%08lx (expected GMEM_INVALID_HANDLE with " \
        "ERROR_INVALID_HANDLE)\n", flags, GetLastError());


    /* Local*() functions */
    gbl = LocalAlloc(LMEM_MOVEABLE, 0);
    ok(gbl != NULL, "local memory not allocated for size 0\n");

    gbl = LocalReAlloc(gbl, 10, LMEM_MOVEABLE);
    ok(gbl != NULL, "Can't realloc local memory\n");
    size = LocalSize(gbl);
    ok(size >= 10 && size <= 16, "Memory not resized to size 10, instead size=%ld\n", size);

    gbl = LocalReAlloc(gbl, 0, LMEM_MOVEABLE);
    ok(gbl != NULL, "LocalReAlloc should not fail on size 0\n");

    size = LocalSize(gbl);
    ok(size == 0, "Memory not resized to size 0, instead size=%ld\n", size);
    ok(LocalFree(gbl) == NULL, "Memory not freed\n");
    size = LocalSize(gbl);
    ok(size == 0, "Memory should have been freed, size=%ld\n", size);

    gbl = LocalReAlloc(0, 10, LMEM_MOVEABLE);
    ok(gbl == NULL, "local realloc allocated memory\n");

    /* invalid handles are catched in windows */
    gbl = LocalAlloc(LMEM_MOVEABLE, 256);
    LocalFree(gbl);
    SetLastError(MAGIC_DEAD);
    hsecond = LocalFree(gbl);       /* invalid handle: free memory twice */
    ok( (hsecond == gbl) && (GetLastError() == ERROR_INVALID_HANDLE),
        "returned %p with 0x%08lx (expected %p with ERROR_INVALID_HANDLE)\n",
        hsecond, GetLastError(), gbl);
    SetLastError(MAGIC_DEAD);
    flags = LocalFlags(gbl);
    ok( (flags == LMEM_INVALID_HANDLE) && (GetLastError() == ERROR_INVALID_HANDLE),
        "returned 0x%04x with 0x%08lx (expected LMEM_INVALID_HANDLE with " \
        "ERROR_INVALID_HANDLE)\n", flags, GetLastError());


    /* trying to lock empty memory should give an error */
    gbl = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT,0);
    ok(gbl != NULL, "returned NULL\n");
    SetLastError(0xdeadbeef);
    mem = GlobalLock(gbl);
    ok( GetLastError() == ERROR_DISCARDED, "should return an error\n");
    ok( mem == NULL, "should return NULL\n");
    GlobalFree(gbl);
}
