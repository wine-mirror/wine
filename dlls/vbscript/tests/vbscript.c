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
#include <objsafe.h>
#include <dispex.h>

#include "wine/test.h"

#ifdef _WIN64

#define IActiveScriptParse_QueryInterface IActiveScriptParse64_QueryInterface
#define IActiveScriptParse_Release IActiveScriptParse64_Release
#define IActiveScriptParse_InitNew IActiveScriptParse64_InitNew
#define IActiveScriptParse_ParseScriptText IActiveScriptParse64_ParseScriptText
#define IActiveScriptParseProcedure2_Release \
    IActiveScriptParseProcedure2_64_Release

#else

#define IActiveScriptParse_QueryInterface IActiveScriptParse32_QueryInterface
#define IActiveScriptParse_Release IActiveScriptParse32_Release
#define IActiveScriptParse_InitNew IActiveScriptParse32_InitNew
#define IActiveScriptParse_ParseScriptText IActiveScriptParse32_ParseScriptText
#define IActiveScriptParseProcedure2_Release \
    IActiveScriptParseProcedure2_32_Release

#endif

DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);

#define DEFINE_EXPECT(func) \
    static BOOL expect_ ## func = FALSE, called_ ## func = FALSE

#define SET_EXPECT(func) \
    expect_ ## func = TRUE

#define CHECK_EXPECT2(func) \
    do { \
        ok(expect_ ##func, "unexpected call " #func "\n"); \
        called_ ## func = TRUE; \
    }while(0)

#define CHECK_EXPECT(func) \
    do { \
        CHECK_EXPECT2(func); \
        expect_ ## func = FALSE; \
    }while(0)

#define CHECK_CALLED(func) \
    do { \
        ok(called_ ## func, "expected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

DEFINE_EXPECT(GetLCID);
DEFINE_EXPECT(OnStateChange_UNINITIALIZED);
DEFINE_EXPECT(OnStateChange_STARTED);
DEFINE_EXPECT(OnStateChange_CONNECTED);
DEFINE_EXPECT(OnStateChange_DISCONNECTED);
DEFINE_EXPECT(OnStateChange_CLOSED);
DEFINE_EXPECT(OnStateChange_INITIALIZED);
DEFINE_EXPECT(OnEnterScript);
DEFINE_EXPECT(OnLeaveScript);

DEFINE_GUID(CLSID_VBScript, 0xb54f3741, 0x5b07, 0x11cf, 0xa4,0xb0, 0x00,0xaa,0x00,0x4a,0x55,0xe8);

#define test_state(s,ss) _test_state(__LINE__,s,ss)
static void _test_state(unsigned line, IActiveScript *script, SCRIPTSTATE exstate)
{
    SCRIPTSTATE state = -1;
    HRESULT hres;

    hres = IActiveScript_GetScriptState(script, &state);
    ok_(__FILE__,line) (hres == S_OK, "GetScriptState failed: %08x\n", hres);
    ok_(__FILE__,line) (state == exstate, "state=%d, expected %d\n", state, exstate);
}

static HRESULT WINAPI ActiveScriptSite_QueryInterface(IActiveScriptSite *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid))
        *ppv = iface;
    else if(IsEqualGUID(&IID_IActiveScriptSite, riid))
        *ppv = iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI ActiveScriptSite_AddRef(IActiveScriptSite *iface)
{
    return 2;
}

static ULONG WINAPI ActiveScriptSite_Release(IActiveScriptSite *iface)
{
    return 1;
}

static HRESULT WINAPI ActiveScriptSite_GetLCID(IActiveScriptSite *iface, LCID *plcid)
{
    CHECK_EXPECT(GetLCID);
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSite_GetItemInfo(IActiveScriptSite *iface, LPCOLESTR pstrName,
        DWORD dwReturnMask, IUnknown **ppiunkItem, ITypeInfo **ppti)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSite_GetDocVersionString(IActiveScriptSite *iface, BSTR *pbstrVersion)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSite_OnScriptTerminate(IActiveScriptSite *iface,
        const VARIANT *pvarResult, const EXCEPINFO *pexcepinfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSite_OnStateChange(IActiveScriptSite *iface, SCRIPTSTATE ssScriptState)
{
    switch(ssScriptState) {
    case SCRIPTSTATE_UNINITIALIZED:
        CHECK_EXPECT(OnStateChange_UNINITIALIZED);
        return S_OK;
    case SCRIPTSTATE_STARTED:
        CHECK_EXPECT(OnStateChange_STARTED);
        return S_OK;
    case SCRIPTSTATE_CONNECTED:
        CHECK_EXPECT(OnStateChange_CONNECTED);
        return S_OK;
    case SCRIPTSTATE_DISCONNECTED:
        CHECK_EXPECT(OnStateChange_DISCONNECTED);
        return S_OK;
    case SCRIPTSTATE_CLOSED:
        CHECK_EXPECT(OnStateChange_CLOSED);
        return S_OK;
    case SCRIPTSTATE_INITIALIZED:
        CHECK_EXPECT(OnStateChange_INITIALIZED);
        return S_OK;
    default:
        ok(0, "unexpected call %d\n", ssScriptState);
    }

    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSite_OnScriptError(IActiveScriptSite *iface, IActiveScriptError *pscripterror)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSite_OnEnterScript(IActiveScriptSite *iface)
{
    CHECK_EXPECT(OnEnterScript);
    return S_OK;
}

static HRESULT WINAPI ActiveScriptSite_OnLeaveScript(IActiveScriptSite *iface)
{
    CHECK_EXPECT(OnLeaveScript);
    return S_OK;
}

static const IActiveScriptSiteVtbl ActiveScriptSiteVtbl = {
    ActiveScriptSite_QueryInterface,
    ActiveScriptSite_AddRef,
    ActiveScriptSite_Release,
    ActiveScriptSite_GetLCID,
    ActiveScriptSite_GetItemInfo,
    ActiveScriptSite_GetDocVersionString,
    ActiveScriptSite_OnScriptTerminate,
    ActiveScriptSite_OnStateChange,
    ActiveScriptSite_OnScriptError,
    ActiveScriptSite_OnEnterScript,
    ActiveScriptSite_OnLeaveScript
};

static IActiveScriptSite ActiveScriptSite = { &ActiveScriptSiteVtbl };

static void test_safety(IActiveScript *script)
{
    IObjectSafety *safety;
    DWORD supported, enabled;
    HRESULT hres;

    hres = IActiveScript_QueryInterface(script, &IID_IObjectSafety, (void**)&safety);
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

    hres = IObjectSafety_SetInterfaceSafetyOptions(safety, &IID_IActiveScriptParse, INTERFACESAFE_FOR_UNTRUSTED_DATA, 0);
    ok(hres == S_OK, "SetInterfaceSafetyOptions failed: %08x\n", hres);

    supported = enabled = 0xdeadbeef;
    hres = IObjectSafety_GetInterfaceSafetyOptions(safety, &IID_IActiveScriptParse, &supported, &enabled);
    ok(hres == S_OK, "GetInterfaceSafetyOptions failed: %08x\n", hres);
    ok(supported == (INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER),
       "supported=%x\n", supported);
    ok(enabled == (INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER), "enabled=%x\n", enabled);

    hres = IObjectSafety_SetInterfaceSafetyOptions(safety, &IID_IActiveScriptParse,
            INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER, 0);
    ok(hres == S_OK, "SetInterfaceSafetyOptions failed: %08x\n", hres);

    supported = enabled = 0xdeadbeef;
    hres = IObjectSafety_GetInterfaceSafetyOptions(safety, &IID_IActiveScriptParse, &supported, &enabled);
    ok(hres == S_OK, "GetInterfaceSafetyOptions failed: %08x\n", hres);
    ok(supported == (INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER),
       "supported=%x\n", supported);
    ok(enabled == INTERFACE_USES_DISPEX, "enabled=%x\n", enabled);

    hres = IObjectSafety_SetInterfaceSafetyOptions(safety, &IID_IActiveScriptParse,
            INTERFACE_USES_DISPEX, 0);
    ok(hres == S_OK, "SetInterfaceSafetyOptions failed: %08x\n", hres);

    supported = enabled = 0xdeadbeef;
    hres = IObjectSafety_GetInterfaceSafetyOptions(safety, &IID_IActiveScriptParse, &supported, &enabled);
    ok(hres == S_OK, "GetInterfaceSafetyOptions failed: %08x\n", hres);
    ok(supported == (INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER),
       "supported=%x\n", supported);
    ok(enabled == INTERFACE_USES_DISPEX, "enabled=%x\n", enabled);

    IObjectSafety_Release(safety);
}

static IDispatchEx *get_script_dispatch(IActiveScript *script)
{
    IDispatchEx *dispex;
    IDispatch *disp;
    HRESULT hres;

    disp = (void*)0xdeadbeef;
    hres = IActiveScript_GetScriptDispatch(script, NULL, &disp);
    ok(hres == S_OK, "GetScriptDispatch failed: %08x\n", hres);
    if(FAILED(hres))
        return NULL;

    hres = IDispatch_QueryInterface(disp, &IID_IDispatchEx, (void**)&dispex);
    IDispatch_Release(disp);
    ok(hres == S_OK, "Could not get IDispatchEx iface: %08x\n", hres);
    return dispex;
}

static void test_no_script_dispatch(IActiveScript *script)
{
    IDispatch *disp;
    HRESULT hres;

    disp = (void*)0xdeadbeef;
    hres = IActiveScript_GetScriptDispatch(script, NULL, &disp);
    ok(hres == E_UNEXPECTED, "hres = %08x, expected E_UNEXPECTED\n", hres);
    ok(!disp, "disp != NULL\n");
}

static IActiveScript *create_vbscript(void)
{
    IActiveScript *ret;
    HRESULT hres;

    hres = CoCreateInstance(&CLSID_VBScript, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IActiveScript, (void**)&ret);
    ok(hres == S_OK, "CoCreateInstance failed: %08x\n", hres);

    return ret;
}

static void test_vbscript(void)
{
    IActiveScriptParseProcedure2 *parse_proc;
    IActiveScriptParse *parser;
    IActiveScript *vbscript;
    ULONG ref;
    HRESULT hres;

    vbscript = create_vbscript();

    hres = IActiveScript_QueryInterface(vbscript, &IID_IActiveScriptParse, (void**)&parser);
    ok(hres == S_OK, "Could not get IActiveScriptParse iface: %08x\n", hres);

    test_state(vbscript, SCRIPTSTATE_UNINITIALIZED);
    test_safety(vbscript);

    SET_EXPECT(GetLCID);
    hres = IActiveScript_SetScriptSite(vbscript, &ActiveScriptSite);
    ok(hres == S_OK, "SetScriptSite failed: %08x\n", hres);
    CHECK_CALLED(GetLCID);

    test_state(vbscript, SCRIPTSTATE_UNINITIALIZED);

    SET_EXPECT(OnStateChange_INITIALIZED);
    hres = IActiveScriptParse_InitNew(parser);
    ok(hres == S_OK, "InitNew failed: %08x\n", hres);
    CHECK_CALLED(OnStateChange_INITIALIZED);

    test_state(vbscript, SCRIPTSTATE_INITIALIZED);

    hres = IActiveScriptParse_InitNew(parser);
    ok(hres == E_UNEXPECTED, "InitNew failed: %08x, expected E_UNEXPECTED\n", hres);

    SET_EXPECT(OnStateChange_CONNECTED);
    hres = IActiveScript_SetScriptState(vbscript, SCRIPTSTATE_CONNECTED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_CONNECTED) failed: %08x\n", hres);
    CHECK_CALLED(OnStateChange_CONNECTED);

    test_state(vbscript, SCRIPTSTATE_CONNECTED);

    SET_EXPECT(OnStateChange_DISCONNECTED);
    SET_EXPECT(OnStateChange_INITIALIZED);
    SET_EXPECT(OnStateChange_CLOSED);
    hres = IActiveScript_Close(vbscript);
    ok(hres == S_OK, "Close failed: %08x\n", hres);
    CHECK_CALLED(OnStateChange_DISCONNECTED);
    CHECK_CALLED(OnStateChange_INITIALIZED);
    CHECK_CALLED(OnStateChange_CLOSED);

    test_state(vbscript, SCRIPTSTATE_CLOSED);
    test_no_script_dispatch(vbscript);

    IActiveScriptParse_Release(parser);

    hres = IActiveScript_QueryInterface(vbscript, &IID_IActiveScriptParseProcedure, (void**)&parse_proc);
    ok(hres == E_NOINTERFACE, "Got IActiveScriptParseProcedure interface, expected E_NOTIMPL\n");

    hres = IActiveScript_QueryInterface(vbscript, &IID_IActiveScriptParseProcedure2, (void**)&parse_proc);
    ok(hres == S_OK, "Could not get IActiveScriptParseProcedure2 interface\n");
    IActiveScriptParseProcedure2_Release(parse_proc);

    ref = IActiveScript_Release(vbscript);
    ok(!ref, "ref = %d\n", ref);
}

static void test_vbscript_uninitializing(void)
{
    IActiveScriptParse *parse;
    IActiveScript *script;
    IDispatchEx *dispex;
    ULONG ref;
    HRESULT hres;

    static const WCHAR script_textW[] =
        {'F','u','n','c','t','i','o','n',' ','f','\n','E','n','d',' ','F','u','n','c','t','i','o','n','\n',0};

    script = create_vbscript();

    hres = IActiveScript_QueryInterface(script, &IID_IActiveScriptParse, (void**)&parse);
    ok(hres == S_OK, "Could not get IActiveScriptParse: %08x\n", hres);

    test_state(script, SCRIPTSTATE_UNINITIALIZED);

    hres = IActiveScriptParse_InitNew(parse);
    ok(hres == S_OK, "InitNew failed: %08x\n", hres);

    SET_EXPECT(GetLCID);
    SET_EXPECT(OnStateChange_INITIALIZED);
    hres = IActiveScript_SetScriptSite(script, &ActiveScriptSite);
    ok(hres == S_OK, "SetScriptSite failed: %08x\n", hres);
    CHECK_CALLED(GetLCID);
    CHECK_CALLED(OnStateChange_INITIALIZED);

    test_state(script, SCRIPTSTATE_INITIALIZED);

    hres = IActiveScriptParse_ParseScriptText(parse, script_textW, NULL, NULL, NULL, 0, 1, 0x42, NULL, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08x\n", hres);

    hres = IActiveScript_SetScriptSite(script, &ActiveScriptSite);
    ok(hres == E_UNEXPECTED, "SetScriptSite failed: %08x, expected E_UNEXPECTED\n", hres);

    SET_EXPECT(OnStateChange_UNINITIALIZED);
    hres = IActiveScript_SetScriptState(script, SCRIPTSTATE_UNINITIALIZED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_UNINITIALIZED) failed: %08x\n", hres);
    CHECK_CALLED(OnStateChange_UNINITIALIZED);

    test_state(script, SCRIPTSTATE_UNINITIALIZED);

    hres = IActiveScript_SetScriptState(script, SCRIPTSTATE_UNINITIALIZED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_UNINITIALIZED) failed: %08x\n", hres);

    SET_EXPECT(GetLCID);
    SET_EXPECT(OnStateChange_INITIALIZED);
    hres = IActiveScript_SetScriptSite(script, &ActiveScriptSite);
    ok(hres == S_OK, "SetScriptSite failed: %08x\n", hres);
    CHECK_CALLED(GetLCID);
    CHECK_CALLED(OnStateChange_INITIALIZED);

    SET_EXPECT(OnStateChange_CONNECTED);
    SET_EXPECT(OnEnterScript);
    SET_EXPECT(OnLeaveScript);
    hres = IActiveScript_SetScriptState(script, SCRIPTSTATE_CONNECTED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_CONNECTED) failed: %08x\n", hres);
    CHECK_CALLED(OnStateChange_CONNECTED);
    CHECK_CALLED(OnEnterScript);
    CHECK_CALLED(OnLeaveScript);

    test_state(script, SCRIPTSTATE_CONNECTED);

    dispex = get_script_dispatch(script);
    ok(dispex != NULL, "dispex == NULL\n");
    if(dispex)
        IDispatchEx_Release(dispex);

    SET_EXPECT(OnStateChange_DISCONNECTED);
    SET_EXPECT(OnStateChange_INITIALIZED);
    SET_EXPECT(OnStateChange_UNINITIALIZED);
    hres = IActiveScript_SetScriptState(script, SCRIPTSTATE_UNINITIALIZED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_UNINITIALIZED) failed: %08x\n", hres);
    CHECK_CALLED(OnStateChange_DISCONNECTED);
    CHECK_CALLED(OnStateChange_INITIALIZED);
    CHECK_CALLED(OnStateChange_UNINITIALIZED);

    test_state(script, SCRIPTSTATE_UNINITIALIZED);

    hres = IActiveScript_Close(script);
    ok(hres == S_OK, "Close failed: %08x\n", hres);

    test_state(script, SCRIPTSTATE_CLOSED);

    hres = IActiveScript_SetScriptState(script, SCRIPTSTATE_UNINITIALIZED);
    ok(hres == E_UNEXPECTED, "SetScriptState(SCRIPTSTATE_UNINITIALIZED) failed: %08x, expected E_UNEXPECTED\n", hres);

    test_state(script, SCRIPTSTATE_CLOSED);

    SET_EXPECT(GetLCID);
    SET_EXPECT(OnStateChange_INITIALIZED);
    hres = IActiveScript_SetScriptSite(script, &ActiveScriptSite);
    ok(hres == S_OK, "SetScriptSite failed: %08x\n", hres);
    CHECK_CALLED(GetLCID);
    CHECK_CALLED(OnStateChange_INITIALIZED);

    test_state(script, SCRIPTSTATE_INITIALIZED);

    SET_EXPECT(OnStateChange_CLOSED);
    hres = IActiveScript_Close(script);
    ok(hres == S_OK, "Close failed: %08x\n", hres);
    CHECK_CALLED(OnStateChange_CLOSED);

    test_state(script, SCRIPTSTATE_CLOSED);

    IActiveScriptParse_Release(parse);

    ref = IActiveScript_Release(script);
    ok(!ref, "ref = %d\n", ref);
}

static void test_vbscript_release(void)
{
    IActiveScriptParse *parser;
    IActiveScript *vbscript;
    ULONG ref;
    HRESULT hres;

    vbscript = create_vbscript();

    hres = IActiveScript_QueryInterface(vbscript, &IID_IActiveScriptParse, (void**)&parser);
    ok(hres == S_OK, "Could not get IActiveScriptParse iface: %08x\n", hres);

    test_state(vbscript, SCRIPTSTATE_UNINITIALIZED);
    test_safety(vbscript);

    SET_EXPECT(GetLCID);
    hres = IActiveScript_SetScriptSite(vbscript, &ActiveScriptSite);
    ok(hres == S_OK, "SetScriptSite failed: %08x\n", hres);
    CHECK_CALLED(GetLCID);

    test_state(vbscript, SCRIPTSTATE_UNINITIALIZED);

    SET_EXPECT(OnStateChange_INITIALIZED);
    hres = IActiveScriptParse_InitNew(parser);
    ok(hres == S_OK, "InitNew failed: %08x\n", hres);
    CHECK_CALLED(OnStateChange_INITIALIZED);

    test_state(vbscript, SCRIPTSTATE_INITIALIZED);

    SET_EXPECT(OnStateChange_CONNECTED);
    hres = IActiveScript_SetScriptState(vbscript, SCRIPTSTATE_CONNECTED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_CONNECTED) failed: %08x\n", hres);
    CHECK_CALLED(OnStateChange_CONNECTED);

    test_state(vbscript, SCRIPTSTATE_CONNECTED);

    IActiveScriptParse_Release(parser);

    SET_EXPECT(OnStateChange_DISCONNECTED);
    SET_EXPECT(OnStateChange_INITIALIZED);
    SET_EXPECT(OnStateChange_CLOSED);
    ref = IActiveScript_Release(vbscript);
    ok(!ref, "ref = %d\n", ref);
    CHECK_CALLED(OnStateChange_DISCONNECTED);
    CHECK_CALLED(OnStateChange_INITIALIZED);
    CHECK_CALLED(OnStateChange_CLOSED);
}

static void test_vbscript_simplecreate(void)
{
    IActiveScript *script;
    ULONG ref;
    HRESULT hres;

    script = create_vbscript();

    hres = IActiveScript_SetScriptState(script, SCRIPTSTATE_UNINITIALIZED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_UNINITIALIZED) failed: %08x\n", hres);

    ref = IActiveScript_Release(script);
    ok(!ref, "ref = %d\n", ref);
}

static void test_vbscript_initializing(void)
{
    IActiveScriptParse *parse;
    IActiveScript *script;
    ULONG ref;
    HRESULT hres;

    script = create_vbscript();

    hres = IActiveScript_QueryInterface(script, &IID_IActiveScriptParse, (void**)&parse);
    ok(hres == S_OK, "Could not get IActiveScriptParse: %08x\n", hres);

    test_state(script, SCRIPTSTATE_UNINITIALIZED);

    SET_EXPECT(GetLCID);
    hres = IActiveScript_SetScriptSite(script, &ActiveScriptSite);
    ok(hres == S_OK, "SetScriptSite failed: %08x\n", hres);
    CHECK_CALLED(GetLCID);

    SET_EXPECT(OnStateChange_INITIALIZED);
    hres = IActiveScriptParse_InitNew(parse);
    ok(hres == S_OK, "InitNew failed: %08x\n", hres);
    CHECK_CALLED(OnStateChange_INITIALIZED);

    hres = IActiveScript_SetScriptSite(script, &ActiveScriptSite);
    ok(hres == E_UNEXPECTED, "SetScriptSite failed: %08x, expected E_UNEXPECTED\n", hres);

    SET_EXPECT(OnStateChange_CLOSED);
    hres = IActiveScript_Close(script);
    ok(hres == S_OK, "Close failed: %08x\n", hres);
    CHECK_CALLED(OnStateChange_CLOSED);

    test_state(script, SCRIPTSTATE_CLOSED);

    IActiveScriptParse_Release(parse);

    ref = IActiveScript_Release(script);
    ok(!ref, "ref = %d\n", ref);
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

    if(check_vbscript()) {
        test_vbscript();
        test_vbscript_uninitializing();
        test_vbscript_release();
        test_vbscript_simplecreate();
        test_vbscript_initializing();
    }else {
        win_skip("VBScript engine not available\n");
    }

    CoUninitialize();
}
