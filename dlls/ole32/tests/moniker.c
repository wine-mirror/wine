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

START_TEST(moniker)
{
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    /* FIXME: test moniker creation funcs and parsing other moniker formats */
    test_MkParseDisplayName();

    CoUninitialize();
}
