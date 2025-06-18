/*
 * Copyright 2002 Andriy Palamarchuk
 *
 * Conformance test of the network buffer function.
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

#include <stdarg.h>

#include "wine/test.h"
#include <windef.h>
#include <winbase.h>
#include <winerror.h>
#include <lmcons.h>
#include <lmerr.h>
#include <lmapibuf.h>
#include <lmaccess.h>

static void run_apibuf_tests(void)
{
    VOID *p;
    DWORD dwSize;
    NET_API_STATUS res;

    /* test normal logic */
    ok(NetApiBufferAllocate(1024, &p) == NERR_Success,
       "Reserved memory\n");
    ok(NetApiBufferSize(p, &dwSize) == NERR_Success, "Got size\n");
    ok(dwSize >= 1024, "The size is correct\n");

    ok(NetApiBufferReallocate(p, 1500, &p) == NERR_Success,
       "Reallocated\n");
    ok(NetApiBufferSize(p, &dwSize) == NERR_Success, "Got size\n");
    ok(dwSize >= 1500, "The size is correct\n");

    ok(NetApiBufferFree(p) == NERR_Success, "Freed\n");

    ok(NetApiBufferSize(NULL, &dwSize) == ERROR_INVALID_PARAMETER, "Error for NULL pointer\n");

    /* border reallocate cases */
    ok(NetApiBufferReallocate(0, 1500, &p) == NERR_Success, "Reallocate with OldBuffer = NULL failed\n");
    ok(p != NULL, "No memory got allocated\n");
    ok(NetApiBufferFree(p) == NERR_Success, "NetApiBufferFree failed\n");

    ok(NetApiBufferAllocate(1024, &p) == NERR_Success, "Memory not reserved\n");
    ok(NetApiBufferReallocate(p, 0, &p) == NERR_Success, "Not freed\n");
    ok(p == NULL, "Pointer not cleared\n");

    /* 0-length buffer */
    ok(NetApiBufferAllocate(0, &p) == NERR_Success,
       "Reserved memory\n");
    ok(NetApiBufferSize(p, &dwSize) == NERR_Success, "Got size\n");
    ok(dwSize < 0xFFFFFFFF, "The size of the 0-length buffer\n");
    ok(NetApiBufferFree(p) == NERR_Success, "Freed\n");

    /* NULL-Pointer */
    /* NT: ERROR_INVALID_PARAMETER, lasterror is untouched) */
    SetLastError(0xdeadbeef);
    res = NetApiBufferAllocate(0, NULL);
    ok( (res == ERROR_INVALID_PARAMETER) && (GetLastError() == 0xdeadbeef),
        "returned %ld with 0x%lx (expected ERROR_INVALID_PARAMETER with "
        "0xdeadbeef)\n", res, GetLastError());

    SetLastError(0xdeadbeef);
    res = NetApiBufferAllocate(1024, NULL);
    ok( (res == ERROR_INVALID_PARAMETER) && (GetLastError() == 0xdeadbeef),
        "returned %ld with 0x%lx (expected ERROR_INVALID_PARAMETER with "
        "0xdeadbeef)\n", res, GetLastError());
}

START_TEST(apibuf)
{
    run_apibuf_tests();
}
