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

#include "wine/test.h"

DEFINE_GUID(CLSID_WshShell, 0x72c24dd5, 0xd70a, 0x438b, 0x8a,0x42, 0x98,0x42,0x4b,0x88,0xaf,0xb8);

DEFINE_GUID(IID_IWshShell3, 0x41904400, 0xbe18, 0x11d3, 0xa2,0x8b, 0x00,0x10,0x4b,0xd3,0x50,0x90);

static void test_wshshell(void)
{
    IDispatch *disp;
    IUnknown *shell;
    HRESULT hres;

    hres = CoCreateInstance(&CLSID_WshShell, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IDispatch, (void**)&disp);
    if(FAILED(hres)) {
        win_skip("Could not create WshShell object: %08x\n", hres);
        return;
    }

    hres = IDispatch_QueryInterface(disp, &IID_IWshShell3, (void**)&shell);
    IDispatch_Release(disp);
    ok(hres == S_OK, "Could not get IWshShell3 iface: %08x\n", hres);

    IUnknown_Release(shell);
}

START_TEST(wshom)
{
    CoInitialize(NULL);

    test_wshshell();

    CoUninitialize();
}
