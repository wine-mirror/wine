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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "wine/test.h"
#include <winbase.h>
#include <winerror.h>
#include <lmcons.h>
#include <lmerr.h>
#include <lmapibuf.h>
#include <lmaccess.h>

void run_apibuf_tests(void)
{
    VOID *p;
    DWORD dwSize;

    /* test normal logic */
    ok(NetApiBufferAllocate(1024, (LPVOID *)&p) == NERR_Success,
       "Reserved memory");
    ok(NetApiBufferSize(p, &dwSize) == NERR_Success, "Got size");
    ok(dwSize >= 1024, "The size is correct");

    ok(NetApiBufferReallocate(p, 1500, (LPVOID *) &p) == NERR_Success,
       "Reallocated");
    ok(NetApiBufferSize(p, &dwSize) == NERR_Success, "Got size");
    ok(dwSize >= 1500, "The size is correct");

    ok(NetApiBufferFree(p) == NERR_Success, "Freed");

    /* test errors handling */
    ok(NetApiBufferFree(p) == NERR_Success, "Freed");

    ok(NetApiBufferSize(p, &dwSize) == NERR_Success, "Got size");
    ok(dwSize >= 0, "The size");
    ok(NetApiBufferSize(NULL, &dwSize) == ERROR_INVALID_PARAMETER, "Error for NULL pointer");

   /* 0-length buffer */
    ok(NetApiBufferAllocate(0, (LPVOID *)&p) == NERR_Success,
       "Reserved memory");
    ok(NetApiBufferSize(p, &dwSize) == NERR_Success, "Got size");
    ok((dwSize >= 0) && (dwSize < 0xFFFFFFFF),"The size of the 0-length buffer");
    ok(NetApiBufferFree(p) == NERR_Success, "Freed");
}

START_TEST(apibuf)
{
    run_apibuf_tests();
}
