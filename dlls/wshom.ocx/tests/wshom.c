/*
 * Copyright 2011 Jacek Caban for CodeWeavers
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
#define CONST_VTABLE

#include <initguid.h>
#include <ole2.h>
#include <dispex.h>

#include "wshom.h"
#include "wine/test.h"

#define EXPECT_HR(hr,hr_exp) \
    ok(hr == hr_exp, "got 0x%08x, expected 0x%08x\n", hr, hr_exp)

static void test_wshshell(void)
{
    IWshShell3 *sh3;
    IDispatchEx *dispex;
    IWshCollection *coll;
    IDispatch *disp;
    IUnknown *shell;
    IFolderCollection *folders;
    ITypeInfo *ti;
    HRESULT hr;
    TYPEATTR *tattr;

    hr = CoCreateInstance(&CLSID_WshShell, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IDispatch, (void**)&disp);
    if(FAILED(hr)) {
        win_skip("Could not create WshShell object: %08x\n", hr);
        return;
    }

    hr = IDispatch_QueryInterface(disp, &IID_IWshShell3, (void**)&shell);
    EXPECT_HR(hr, S_OK);
    IDispatch_Release(disp);

    hr = IDispatch_QueryInterface(disp, &IID_IDispatchEx, (void**)&dispex);
    EXPECT_HR(hr, E_NOINTERFACE);

    hr = IDispatch_QueryInterface(shell, &IID_IWshShell3, (void**)&sh3);
    EXPECT_HR(hr, S_OK);

    hr = IWshShell3_get_SpecialFolders(sh3, &coll);
    EXPECT_HR(hr, S_OK);

    hr = IWshCollection_QueryInterface(coll, &IID_IFolderCollection, (void**)&folders);
    EXPECT_HR(hr, E_NOINTERFACE);

    hr = IWshCollection_QueryInterface(coll, &IID_IDispatch, (void**)&disp);
    EXPECT_HR(hr, S_OK);

    hr = IDispatch_GetTypeInfo(disp, 0, 0, &ti);
    EXPECT_HR(hr, S_OK);

    hr = ITypeInfo_GetTypeAttr(ti, &tattr);
    EXPECT_HR(hr, S_OK);
    ok(IsEqualIID(&tattr->guid, &IID_IWshCollection), "got \n");
    ITypeInfo_ReleaseTypeAttr(ti, tattr);

    IWshShell3_Release(sh3);

    IUnknown_Release(shell);
}

START_TEST(wshom)
{
    CoInitialize(NULL);

    test_wshshell();

    CoUninitialize();
}
