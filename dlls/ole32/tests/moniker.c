/*
 * Moniker Tests
 *
 * Copyright 2004 Robert Shearman
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

#define _WIN32_DCOM
#define COBJMACROS

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "comcat.h"

#include "wine/test.h"

#define ok_ole_success(hr, func) ok(hr == S_OK, #func " failed with error 0x%08lx\n", hr)

static void test_MkParseDisplayName()
{
    IBindCtx * pbc = NULL;
    HRESULT hr;
    IMoniker * pmk = NULL;
    ULONG eaten;
    IUnknown * object = NULL;
    /* CLSID of My Computer */
    static const WCHAR wszDisplayName[] = {'c','l','s','i','d',':',
        '2','0','D','0','4','F','E','0','-','3','A','E','A','-','1','0','6','9','-','A','2','D','8','-','0','8','0','0','2','B','3','0','3','0','9','D',':',0};

    hr = CreateBindCtx(0, &pbc);
    ok_ole_success(hr, CreateBindCtx);

    hr = MkParseDisplayName(pbc, wszDisplayName, &eaten, &pmk);
    todo_wine { ok_ole_success(hr, MkParseDisplayName); }

    if (object)
    {
        hr = IMoniker_BindToObject(pmk, pbc, NULL, &IID_IUnknown, (LPVOID*)&object);
        ok_ole_success(hr, IMoniker_BindToObject);

        IUnknown_Release(object);
    }
    IBindCtx_Release(pbc);
}

static const BYTE expected_moniker_data[] =
{
	0x4d,0x45,0x4f,0x57,0x04,0x00,0x00,0x00,
	0x0f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
	0x1a,0x03,0x00,0x00,0x00,0x00,0x00,0x00,
	0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
	0x00,0x00,0x00,0x00,0x14,0x00,0x00,0x00,
	0x05,0xe0,0x02,0x00,0x00,0x00,0x00,0x00,
	0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
	0x00,0x00,0x00,0x00,
};

static const LARGE_INTEGER llZero;

static void test_class_moniker()
{
    IStream * stream;
    IMoniker * moniker;
    HRESULT hr;
    HGLOBAL hglobal;
    LPBYTE moniker_data;
    DWORD moniker_size;
    DWORD i;
    BOOL same = TRUE;

    hr = CreateClassMoniker(&CLSID_StdComponentCategoriesMgr, &moniker);
    todo_wine { ok_ole_success(hr, CreateClassMoniker); }

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);

    hr = CoMarshalInterface(stream, &IID_IMoniker, (IUnknown *)moniker, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    todo_wine { ok_ole_success(hr, CoMarshalInterface); }

    hr = GetHGlobalFromStream(stream, &hglobal);
    ok_ole_success(hr, GetHGlobalFromStream);

    moniker_size = GlobalSize(hglobal);

    moniker_data = GlobalLock(hglobal);

    /* first check we have the right amount of data */
    todo_wine {
    ok(moniker_size == sizeof(expected_moniker_data),
        "Size of marshaled data differs (expected %d, actual %ld)\n",
        sizeof(expected_moniker_data), moniker_size);
    }

    /* then do a byte-by-byte comparison */
    for (i = 0; i < min(moniker_size, sizeof(expected_moniker_data)); i++)
    {
        if (expected_moniker_data[i] != moniker_data[i])
        {
            same = FALSE;
            break;
        }
    }

    ok(same, "Marshaled data differs\n");
    if (!same)
    {
        trace("Dumping marshaled moniker data:\n");
        for (i = 0; i < moniker_size; i++)
        {
            trace("0x%02x,", moniker_data[i]);
            if (i % 8 == 7) trace("\n");
            if (i % 8 == 0) trace("     ");
        }
    }

    GlobalUnlock(hglobal);

    IStream_Seek(stream, llZero, STREAM_SEEK_SET, NULL);
    hr = CoReleaseMarshalData(stream);
    todo_wine { ok_ole_success(hr, CoReleaseMarshalData); }

    IStream_Release(stream);
    if (moniker) IMoniker_Release(moniker);
}

START_TEST(moniker)
{
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    test_MkParseDisplayName();
    test_class_moniker();
    /* FIXME: test moniker creation funcs and parsing other moniker formats */

    CoUninitialize();
}
