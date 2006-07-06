/*
 * Copyright 2006 Jacek Caban for CodeWeavers
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

#include <wine/test.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "ole2.h"
#include "exdisp.h"

static void test_ClassInfo(IUnknown *unk)
{
    IProvideClassInfo2 *class_info;
    GUID guid;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IProvideClassInfo2, (void**)&class_info);
    ok(hres == S_OK, "QueryInterface(IID_IProvideClassInfo)  failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    hres = IProvideClassInfo2_GetGUID(class_info, GUIDKIND_DEFAULT_SOURCE_DISP_IID, &guid);
    ok(hres == S_OK, "GetGUID failed: %08lx\n", hres);
    ok(IsEqualGUID(&DIID_DWebBrowserEvents2, &guid), "wrong guid\n");

    hres = IProvideClassInfo2_GetGUID(class_info, 0, &guid);
    ok(hres == E_FAIL, "GetGUID failed: %08lx, expected E_FAIL\n", hres);
    ok(IsEqualGUID(&IID_NULL, &guid), "wrong guid\n");

    hres = IProvideClassInfo2_GetGUID(class_info, 2, &guid);
    ok(hres == E_FAIL, "GetGUID failed: %08lx, expected E_FAIL\n", hres);
    ok(IsEqualGUID(&IID_NULL, &guid), "wrong guid\n");

    hres = IProvideClassInfo2_GetGUID(class_info, GUIDKIND_DEFAULT_SOURCE_DISP_IID, NULL);
    ok(hres == E_POINTER, "GetGUID failed: %08lx, expected E_POINTER\n", hres);

    hres = IProvideClassInfo2_GetGUID(class_info, 0, NULL);
    ok(hres == E_POINTER, "GetGUID failed: %08lx, expected E_POINTER\n", hres);

    IProvideClassInfo2_Release(class_info);
}

static void test_WebBrowser(void)
{
    IUnknown *unk = NULL;
    ULONG ref;
    HRESULT hres;

    hres = CoCreateInstance(&CLSID_WebBrowser, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IUnknown, (void**)&unk);
    ok(hres == S_OK, "CoCreateInterface failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    test_ClassInfo(unk);

    ref = IUnknown_Release(unk);
    ok(ref == 0, "ref=%ld, expected 0\n", ref);
}

START_TEST(webbrowser)
{
    OleInitialize(NULL);

    test_WebBrowser();

    OleUninitialize();
}
