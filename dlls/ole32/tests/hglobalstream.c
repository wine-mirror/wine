/*
 * Stream on HGLOBAL Tests
 *
 * Copyright 2006 Robert Shearman (for CodeWeavers)
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

#define COBJMACROS

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "objbase.h"

#include "wine/test.h"

#define ok_ole_success(hr, func) ok(hr == S_OK, func " failed with error 0x%08lx\n", hr)

static void test_streamonhglobal(IStream *pStream)
{
    const char data[] = "Test String";
    ULARGE_INTEGER ull;
    LARGE_INTEGER ll;
    char buffer[128];
    ULONG read;
    STATSTG statstg;
    HRESULT hr;

    ull.QuadPart = sizeof(data);
    hr = IStream_SetSize(pStream, ull);
    ok_ole_success(hr, "IStream_SetSize");

    hr = IStream_Write(pStream, data, sizeof(data), NULL);
    ok_ole_success(hr, "IStream_Write");

    ll.QuadPart = 0;
    hr = IStream_Seek(pStream, ll, STREAM_SEEK_SET, NULL);
    ok_ole_success(hr, "IStream_Seek");

    /* should return S_OK, not S_FALSE */
    hr = IStream_Read(pStream, buffer, sizeof(buffer), &read);
    todo_wine {
    ok_ole_success(hr, "IStream_Read");
    }
    ok(read == sizeof(data), "IStream_Read returned read %ld instead of %d\n", read, sizeof(data));

    /* ignores HighPart */
    ull.HighPart = -1;
    ull.LowPart = 0;
    hr = IStream_SetSize(pStream, ull);
    todo_wine {
    ok_ole_success(hr, "IStream_SetSize");
    }

    hr = IStream_Commit(pStream, STGC_DEFAULT);
    ok_ole_success(hr, "IStream_Commit");

    hr = IStream_Revert(pStream);
    ok_ole_success(hr, "IStream_Revert");

    hr = IStream_LockRegion(pStream, ull, ull, LOCK_WRITE);
    todo_wine {
    ok(hr == STG_E_INVALIDFUNCTION, "IStream_LockRegion should have returned STG_E_INVALIDFUNCTION instead of 0x%08lx\n", hr);
    }

    hr = IStream_Stat(pStream, &statstg, STATFLAG_DEFAULT);
    ok_ole_success(hr, "IStream_Stat");
    ok(statstg.type == STGTY_STREAM, "statstg.type should have been STGTY_STREAM instead of %ld\n", statstg.type);

    /* test OOM condition */
    ull.HighPart = -1;
    ull.LowPart = -1;
    hr = IStream_SetSize(pStream, ull);
    todo_wine {
    ok(hr == E_OUTOFMEMORY, "IStream_SetSize with large size should have returned E_OUTOFMEMORY instead of 0x%08lx\n", hr);
    }
}

START_TEST(hglobalstream)
{
    HRESULT hr;
    IStream *pStream;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, "CreateStreamOnHGlobal");

    test_streamonhglobal(pStream);
}
