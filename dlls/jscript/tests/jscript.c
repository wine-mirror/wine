/*
 * Copyright 2008 Jacek Caban for CodeWeavers
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

#include <ole2.h>
#include <activscp.h>
#include <objsafe.h>

#include "wine/test.h"

static const CLSID CLSID_JScript =
    {0xf414c260,0x6ac0,0x11cf,{0xb6,0xd1,0x00,0xaa,0x00,0xbb,0xbb,0x58}};

static void test_safety(IUnknown *unk)
{
    IObjectSafety *safety;
    DWORD supported, enabled;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IObjectSafety, (void**)&safety);
    ok(hres == S_OK, "Could not get IObjectSafety: %08x\n", hres);
    if(FAILED(hres))
        return;

    hres = IObjectSafety_GetInterfaceSafetyOptions(safety, &IID_NULL, &supported, NULL);
    ok(hres == E_POINTER, "GetInterfaceSafetyOptions failed: %08x, expected E_POINTER\n", hres);
    hres = IObjectSafety_GetInterfaceSafetyOptions(safety, &IID_NULL, NULL, &enabled);
    ok(hres == E_POINTER, "GetInterfaceSafetyOptions failed: %08x, expected E_POINTER\n", hres);

    supported = enabled = 0xdeadbeef;
    hres = IObjectSafety_GetInterfaceSafetyOptions(safety, &IID_NULL, &supported, &enabled);
    ok(hres == S_OK, "GetInterfaceSafetyOptions failed: %08x\n", hres);
    ok(supported == (INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER),
       "supported=%x\n", supported);
    ok(enabled == INTERFACE_USES_DISPEX, "enabled=%x\n", enabled);

    supported = enabled = 0xdeadbeef;
    hres = IObjectSafety_GetInterfaceSafetyOptions(safety, &IID_IActiveScript, &supported, &enabled);
    ok(hres == S_OK, "GetInterfaceSafetyOptions failed: %08x\n", hres);
    ok(supported == (INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER),
       "supported=%x\n", supported);
    ok(enabled == INTERFACE_USES_DISPEX, "enabled=%x\n", enabled);

    supported = enabled = 0xdeadbeef;
    hres = IObjectSafety_GetInterfaceSafetyOptions(safety, &IID_IActiveScriptParse, &supported, &enabled);
    ok(hres == S_OK, "GetInterfaceSafetyOptions failed: %08x\n", hres);
    ok(supported == (INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER),
       "supported=%x\n", supported);
    ok(enabled == INTERFACE_USES_DISPEX, "enabled=%x\n", enabled);

    hres = IObjectSafety_SetInterfaceSafetyOptions(safety, &IID_IActiveScriptParse,
            INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER
                |INTERFACESAFE_FOR_UNTRUSTED_CALLER,
            INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER);
    ok(hres == E_FAIL, "SetInterfaceSafetyOptions failed: %08x, expected E_FAIL\n", hres);

    hres = IObjectSafety_SetInterfaceSafetyOptions(safety, &IID_IActiveScriptParse,
            INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER,
            INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER);
    ok(hres == S_OK, "SetInterfaceSafetyOptions failed: %08x\n", hres);

    supported = enabled = 0xdeadbeef;
    hres = IObjectSafety_GetInterfaceSafetyOptions(safety, &IID_IActiveScriptParse, &supported, &enabled);
    ok(hres == S_OK, "GetInterfaceSafetyOptions failed: %08x\n", hres);
    ok(supported == (INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER),
       "supported=%x\n", supported);
    ok(enabled == (INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER),
       "enabled=%x\n", enabled);

    IObjectSafety_Release(safety);
}

static void test_jscript(void)
{
    IActiveScriptParse *parse;
    IActiveScript *script;
    IUnknown *unk;
    HRESULT hres;

    hres = CoCreateInstance(&CLSID_JScript, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IUnknown, (void**)&unk);
    ok(hres == S_OK, "CoCreateInstance failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    hres = IUnknown_QueryInterface(unk, &IID_IActiveScript, (void**)&script);
    ok(hres == S_OK, "Could not get IActiveScript: %08x\n", hres);

    hres = IUnknown_QueryInterface(unk, &IID_IActiveScriptParse, (void**)&parse);
    ok(hres == S_OK, "Could not get IActiveScriptParse: %08x\n", hres);

    test_safety(unk);

    IActiveScriptParse_Release(parse);
    IActiveScript_Release(script);
    IUnknown_Release(unk);
}

START_TEST(jscript)
{
    CoInitialize(NULL);

    test_jscript();

    CoUninitialize();
}
