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

#include <stdarg.h>

#include "wine/test.h"
#include <windef.h>
#include <winbase.h>
#include <winerror.h>
#include <lmcons.h>
#include <lmerr.h>
#include <lmapibuf.h>
#include <lmaccess.h>

static NET_API_STATUS (WINAPI *pNetApiBufferAllocate)(DWORD,LPVOID*)=NULL;
static NET_API_STATUS (WINAPI *pNetApiBufferFree)(LPVOID)=NULL;
static NET_API_STATUS (WINAPI *pNetApiBufferReallocate)(LPVOID,DWORD,LPVOID*)=NULL;
static NET_API_STATUS (WINAPI *pNetApiBufferSize)(LPVOID,LPDWORD)=NULL;


void run_apibuf_tests(void)
{
    VOID *p;
    DWORD dwSize;

    if (!pNetApiBufferAllocate)
        return;

    /* test normal logic */
    ok(pNetApiBufferAllocate(1024, (LPVOID *)&p) == NERR_Success,
       "Reserved memory");
    ok(pNetApiBufferSize(p, &dwSize) == NERR_Success, "Got size");
    ok(dwSize >= 1024, "The size is correct");

    ok(pNetApiBufferReallocate(p, 1500, (LPVOID *) &p) == NERR_Success,
       "Reallocated");
    ok(pNetApiBufferSize(p, &dwSize) == NERR_Success, "Got size");
    ok(dwSize >= 1500, "The size is correct");

    ok(pNetApiBufferFree(p) == NERR_Success, "Freed");

    /* test errors handling */
    ok(pNetApiBufferFree(p) == NERR_Success, "Freed");

    ok(pNetApiBufferSize(p, &dwSize) == NERR_Success, "Got size");
    ok(dwSize >= 0, "The size");
    ok(pNetApiBufferSize(NULL, &dwSize) == ERROR_INVALID_PARAMETER, "Error for NULL pointer");

    /* border reallocate cases */
    ok(pNetApiBufferReallocate(0, 1500, (LPVOID *) &p) != NERR_Success, "(Re)allocated");
    ok(p == NULL, "Some memory got allocated");
    ok(pNetApiBufferAllocate(1024, (LPVOID *)&p) == NERR_Success, "Memory not reserved");
    ok(pNetApiBufferReallocate(p, 0, (LPVOID *) &p) == NERR_Success, "Not freed");
    ok(p == NULL, "Pointer not cleared");
    
    /* 0-length buffer */
    ok(pNetApiBufferAllocate(0, (LPVOID *)&p) == NERR_Success,
       "Reserved memory");
    ok(pNetApiBufferSize(p, &dwSize) == NERR_Success, "Got size");
    ok((dwSize >= 0) && (dwSize < 0xFFFFFFFF),"The size of the 0-length buffer");
    ok(pNetApiBufferFree(p) == NERR_Success, "Freed");
}

START_TEST(apibuf)
{
    HMODULE hnetapi32=LoadLibraryA("netapi32.dll");
    pNetApiBufferAllocate=(void*)GetProcAddress(hnetapi32,"NetApiBufferAllocate");
    pNetApiBufferFree=(void*)GetProcAddress(hnetapi32,"NetApiBufferFree");
    pNetApiBufferReallocate=(void*)GetProcAddress(hnetapi32,"NetApiBufferReallocate");
    pNetApiBufferSize=(void*)GetProcAddress(hnetapi32,"NetApiBufferSize");
    if (!pNetApiBufferSize)
        trace("It appears there is no netapi32 functionality on this platform\n");

    run_apibuf_tests();
}
