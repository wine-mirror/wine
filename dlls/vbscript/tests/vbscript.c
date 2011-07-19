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
#include <activscp.h>

#include "wine/test.h"

DEFINE_GUID(CLSID_VBScript, 0xb54f3741, 0x5b07, 0x11cf, 0xa4,0xb0, 0x00,0xaa,0x00,0x4a,0x55,0xe8);

static void test_vbscript(void)
{
    IActiveScriptParse *parser;
    IActiveScript *vbscript;
    HRESULT hres;

    hres = CoCreateInstance(&CLSID_VBScript, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IActiveScript, (void**)&vbscript);
    ok(hres == S_OK, "CoCreateInstance failed: %08x\n", hres);

    hres = IActiveScript_QueryInterface(vbscript, &IID_IActiveScriptParse, (void**)&parser);
    ok(hres == S_OK, "Could not get IActiveScriptParse iface: %08x\n", hres);

    IActiveScriptParse64_Release(parser);
    IActiveScript_Release(vbscript);
}

static BOOL check_vbscript(void)
{
    IActiveScript *vbscript;
    HRESULT hres;

    hres = CoCreateInstance(&CLSID_VBScript, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IActiveScript, (void**)&vbscript);
    if(SUCCEEDED(hres))
        IActiveScript_Release(vbscript);

    return hres == S_OK;
}


START_TEST(vbscript)
{
    CoInitialize(NULL);

    if(check_vbscript())
        test_vbscript();
    else
        win_skip("VBScript engine not available\n");

    CoUninitialize();
}
