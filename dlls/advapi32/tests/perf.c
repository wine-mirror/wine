/*
 * Unit tests for Perflib functions
 *
 * Copyright (c) 2021 Paul Gofman for CodeWeavers
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

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "perflib.h"

#include "wine/test.h"

static ULONG WINAPI test_provider_callback(ULONG code, void *buffer, ULONG size)
{
    ok(0, "Provider callback called.\n");
    return ERROR_SUCCESS;
}

void test_provider_init(void)
{
    static GUID test_guid = {0xdeadbeef, 0x0001, 0x0002, {0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x00 ,0x0a}};
    PERF_PROVIDER_CONTEXT prov_context;
    HANDLE prov, prov2;
    ULONG ret;
    BOOL bret;

    prov = (HANDLE)0xdeadbeef;
    ret = PerfStartProvider(NULL, test_provider_callback, &prov);
    ok(ret == ERROR_INVALID_PARAMETER, "Got unexpected ret %u.\n", ret);
    ok(prov == (HANDLE)0xdeadbeef, "Got unexpected prov %p.\n", prov);

    prov = (HANDLE)0xdeadbeef;
    ret = PerfStartProvider(&test_guid, test_provider_callback, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "Got unexpected ret %u.\n", ret);
    ok(prov == (HANDLE)0xdeadbeef, "Got unexpected prov %p.\n", prov);

    prov = (HANDLE)0xdeadbeef;
    ret = PerfStartProvider(&test_guid, test_provider_callback, &prov);
    ok(!ret, "Got unexpected ret %u.\n", ret);
    ok(prov != (HANDLE)0xdeadbeef, "Provider handle is not set.\n");

    prov2 = prov;
    ret = PerfStartProvider(&test_guid, test_provider_callback, &prov2);
    ok(!ret, "Got unexpected ret %u.\n", ret);
    ok(prov2 != prov, "Got the same provider handle.\n");

    ret = PerfStopProvider(prov2);
    ok(!ret, "Got unexpected ret %u.\n", ret);

    if (0)
    {
        /* Access violation on Windows. */
        PerfStopProvider(prov2);
    }

    /* Provider handle is a pointer and not a kernel object handle. */
    bret = DuplicateHandle(GetCurrentProcess(), prov, GetCurrentProcess(), &prov2, 0, FALSE, DUPLICATE_SAME_ACCESS);
    ok(!bret && GetLastError() == ERROR_INVALID_HANDLE, "Got unexpected bret %d, err %u.\n", bret, GetLastError());
    bret = IsBadWritePtr(prov, 8);
    ok(!bret, "Handle does not point to the data.\n");

    ret = PerfStopProvider(prov);
    ok(!ret, "Got unexpected ret %u.\n", ret);

    memset( &prov_context, 0, sizeof(prov_context) );
    prov = (HANDLE)0xdeadbeef;
    ret = PerfStartProviderEx( &test_guid, &prov_context, &prov );
    ok(ret == ERROR_INVALID_PARAMETER, "Got unexpected ret %u.\n", ret);
    ok(prov == (HANDLE)0xdeadbeef, "Got unexpected prov %p.\n", prov);

    prov_context.ContextSize = sizeof(prov_context) + 1;
    ret = PerfStartProviderEx( &test_guid, &prov_context, &prov );
    ok(!ret, "Got unexpected ret %u.\n", ret);
    ok(prov != (HANDLE)0xdeadbeef, "Provider handle is not set.\n");

    ret = PerfStopProvider(prov);
    ok(!ret, "Got unexpected ret %u.\n", ret);
}

START_TEST(perf)
{
    test_provider_init();
}
