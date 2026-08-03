/*
 * Copyright 2019 Jacek Caban for CodeWeavers
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

#include <ole2.h>
#include <dispex.h>
#include <activscp.h>
#include <activdbg.h>
#include <objsafe.h>
#include <mshtmhst.h>

#include "wine/test.h"

static HRESULT (WINAPI *pDllInstall)(BOOL, const WCHAR *);

static const CLSID CLSID_WineTest = {0xa8a83483,0xa6ac,0x474d,{0xb2,0x2a,0x9a,0x5f,0x2d,0x68,0xcb,0x7f}};
#define CLSID_STR "{a8a83483-a6ac-474d-b22a-9a5f2d68cb7f}"

#define TESTSCRIPT_CLSID "{178fc164-f585-4e24-9c13-4bb7faf80746}"
static const GUID CLSID_TestScript =
    {0x178fc164,0xf585,0x4e24,{0x9c,0x13,0x4b,0xb7,0xfa,0xf8,0x07,0x46}};

#ifdef _WIN64

#define CTXARG_T DWORDLONG
#define IActiveScriptParseVtbl IActiveScriptParse64Vtbl
#define IActiveScriptSiteDebug_Release IActiveScriptSiteDebug64_Release

#else

#define CTXARG_T DWORD
#define IActiveScriptParseVtbl IActiveScriptParse32Vtbl
#define IActiveScriptSiteDebug_Release IActiveScriptSiteDebug32_Release

#endif

#define DEFINE_EXPECT(func) \
    static BOOL expect_ ## func = FALSE, called_ ## func = FALSE

#define SET_EXPECT(func) \
    do { called_ ## func = FALSE; expect_ ## func = TRUE; } while(0)

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

#define CHECK_CALLED_BROKEN(func) \
    do { \
        ok(called_ ## func || broken(!called_ ## func), "expected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

#define CHECK_NOT_CALLED(func) \
    do { \
        ok(!called_ ## func, "unexpected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

#define CLEAR_CALLED(func) \
    expect_ ## func = called_ ## func = FALSE

DEFINE_EXPECT(CreateInstance);
DEFINE_EXPECT(InitNew);
DEFINE_EXPECT(Close);
DEFINE_EXPECT(SetScriptSite);
DEFINE_EXPECT(QI_IActiveScriptParse);
DEFINE_EXPECT(SetScriptState_INITIALIZED);
DEFINE_EXPECT(SetScriptState_UNINITIALIZED);
DEFINE_EXPECT(SetScriptState_STARTED);
DEFINE_EXPECT(AddNamedItem_scriptlet);
DEFINE_EXPECT(AddNamedItem_globals);
DEFINE_EXPECT(ParseScriptText);
DEFINE_EXPECT(Clone);
DEFINE_EXPECT(GetScriptDispatch);
DEFINE_EXPECT(GetDispID_vbAddOne);
DEFINE_EXPECT(GetDispID_wtTest);
DEFINE_EXPECT(GetDispID_get_gsProp);
DEFINE_EXPECT(GetDispID_put_gsProp);
DEFINE_EXPECT(InvokeEx);
DEFINE_EXPECT(InvokeEx_get_gsProp);
DEFINE_EXPECT(InvokeEx_put_gsProp);

#define DISPID_WTTEST      100
#define DISPID_GET_GSPROP  101
#define DISPID_PUT_GSPROP  102

static DWORD parse_flags;
static BOOL support_clone;

static IActiveScriptSite *site;
static SCRIPTSTATE state;

static HRESULT WINAPI DispatchEx_QueryInterface(IDispatchEx *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if (IsEqualGUID(riid, &IID_IUnknown)
       || IsEqualGUID(riid, &IID_IDispatch)
       || IsEqualGUID(riid, &IID_IDispatchEx))
        *ppv = iface;
    else
        return E_NOINTERFACE;

    return S_OK;
}

static ULONG WINAPI DispatchEx_AddRef(IDispatchEx *iface)
{
    return 2;
}

static ULONG WINAPI DispatchEx_Release(IDispatchEx *iface)
{
    return 1;
}

static HRESULT WINAPI DispatchEx_GetTypeInfoCount(IDispatchEx *iface, UINT *pctinfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetTypeInfo(IDispatchEx *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetIDsOfNames(IDispatchEx *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_Invoke(IDispatchEx *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetDispID(IDispatchEx *iface, BSTR name, DWORD grfdex, DISPID *pid)
{
    if (!wcscmp(name, L"vbAddOne"))
    {
        CHECK_EXPECT(GetDispID_vbAddOne);
        ok(grfdex == fdexNameCaseInsensitive, "grfdex = %lx\n", grfdex);
        return DISP_E_UNKNOWNNAME;
    }
    if (!wcscmp(name, L"wtTest"))
    {
        CHECK_EXPECT(GetDispID_wtTest);
        ok(!grfdex, "grfdex = %lx\n", grfdex);
        *pid = DISPID_WTTEST;
        return S_OK;
    }
    if (!wcscmp(name, L"get_gsProp"))
    {
        CHECK_EXPECT(GetDispID_get_gsProp);
        ok(!grfdex, "grfdex = %lx\n", grfdex);
        *pid = DISPID_GET_GSPROP;
        return S_OK;
    }
    if (!wcscmp(name, L"put_gsProp"))
    {
        CHECK_EXPECT(GetDispID_put_gsProp);
        ok(!grfdex, "grfdex = %lx\n", grfdex);
        *pid = DISPID_PUT_GSPROP;
        return S_OK;
    }
    ok(0, "unexpected name %s\n", wine_dbgstr_w(name));
    return DISP_E_UNKNOWNNAME;
}

static HRESULT WINAPI DispatchEx_InvokeEx(IDispatchEx *iface, DISPID id, LCID lcid, WORD flags, DISPPARAMS *pdp,
        VARIANT *res, EXCEPINFO *pei, IServiceProvider *caller)
{
    switch (id)
    {
    case DISPID_WTTEST:
        CHECK_EXPECT(InvokeEx);
        ok(lcid == 0x100, "lcid = %lx\n", lcid);
        ok(flags == DISPATCH_METHOD, "flags = %x\n", flags);
        ok(caller == (void*)0xdeadbeef, "called = %p\n", caller);
        V_VT(res) = VT_BOOL;
        V_BOOL(res) = VARIANT_TRUE;
        return S_OK;
    case DISPID_GET_GSPROP:
        CHECK_EXPECT(InvokeEx_get_gsProp);
        ok(flags == DISPATCH_METHOD, "flags = %x\n", flags);
        ok(pdp->cArgs == 1, "cArgs = %d\n", pdp->cArgs);
        V_VT(res) = VT_BOOL;
        V_BOOL(res) = VARIANT_TRUE;
        return S_OK;
    case DISPID_PUT_GSPROP:
        CHECK_EXPECT(InvokeEx_put_gsProp);
        ok(flags == DISPATCH_METHOD, "flags = %x\n", flags);
        ok(pdp->cArgs == 1, "cArgs = %d\n", pdp->cArgs);
        ok(pdp->cNamedArgs == 0, "cNamedArgs = %d\n", pdp->cNamedArgs);
        V_VT(res) = VT_BOOL;
        V_BOOL(res) = VARIANT_FALSE;
        return S_OK;
    }

    ok(0, "unexpected id %lu\n", id);
    return E_FAIL;
}

static HRESULT WINAPI DispatchEx_DeleteMemberByName(IDispatchEx *iface, BSTR name, DWORD grfdex)
{
    ok(0, "unexpected call %s %lx\n", wine_dbgstr_w(name), grfdex);
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_DeleteMemberByDispID(IDispatchEx *iface, DISPID id)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetMemberProperties(IDispatchEx *iface, DISPID id, DWORD grfdexFetch, DWORD *pgrfdex)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetMemberName(IDispatchEx *iface, DISPID id, BSTR *name)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetNextDispID(IDispatchEx *iface, DWORD grfdex, DISPID id, DISPID *pid)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetNameSpaceParent(IDispatchEx *iface, IUnknown **ppunk)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static IDispatchExVtbl DispatchExVtbl = {
    DispatchEx_QueryInterface,
    DispatchEx_AddRef,
    DispatchEx_Release,
    DispatchEx_GetTypeInfoCount,
    DispatchEx_GetTypeInfo,
    DispatchEx_GetIDsOfNames,
    DispatchEx_Invoke,
    DispatchEx_GetDispID,
    DispatchEx_InvokeEx,
    DispatchEx_DeleteMemberByName,
    DispatchEx_DeleteMemberByDispID,
    DispatchEx_GetMemberProperties,
    DispatchEx_GetMemberName,
    DispatchEx_GetNextDispID,
    DispatchEx_GetNameSpaceParent
};

static IDispatchEx script_global = { &DispatchExVtbl };

static HRESULT WINAPI ActiveScriptParse_QueryInterface(IActiveScriptParse *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;
    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI ActiveScriptParse_AddRef(IActiveScriptParse *iface)
{
    return 2;
}

static ULONG WINAPI ActiveScriptParse_Release(IActiveScriptParse *iface)
{
    return 1;
}

static HRESULT WINAPI ActiveScriptParse_InitNew(IActiveScriptParse *iface)
{
    CHECK_EXPECT(InitNew);
    return S_OK;
}

static HRESULT WINAPI ActiveScriptParse_AddScriptlet(IActiveScriptParse *iface,
        LPCOLESTR pstrDefaultName, LPCOLESTR pstrCode, LPCOLESTR pstrItemName,
        LPCOLESTR pstrSubItemName, LPCOLESTR pstrEventName, LPCOLESTR pstrDelimiter,
        CTXARG_T dwSourceContextCookie, ULONG ulStartingLineNumber, DWORD dwFlags,
        BSTR *pbstrName, EXCEPINFO *pexcepinfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptParse_ParseScriptText(IActiveScriptParse *iface,
        LPCOLESTR code, LPCOLESTR item_name, IUnknown *context,
        LPCOLESTR delimiter, CTXARG_T dwSourceContextCookie, ULONG ulStartingLine,
        DWORD flags, VARIANT *result, EXCEPINFO *excepinfo)
{
    CHECK_EXPECT(ParseScriptText);
    ok(!wcscmp(code, L"WTTest body"), "unexpected code %s\n", wine_dbgstr_w(code));
    ok(!item_name, "pstrItemName = %s\n", wine_dbgstr_w(item_name));
    ok(!context, "punkContext = %p\n", context);
    ok(!delimiter, "pstrDelimiter = %s\n", wine_dbgstr_w(delimiter));
    ok(flags == parse_flags, "dwFlags = %lx\n", flags);
    ok(!result, "pvarResult = NULL\n");
    ok(!excepinfo, "pexcepinfo = %p\n", excepinfo);
    return S_OK;
}

static const IActiveScriptParseVtbl ActiveScriptParseVtbl = {
    ActiveScriptParse_QueryInterface,
    ActiveScriptParse_AddRef,
    ActiveScriptParse_Release,
    ActiveScriptParse_InitNew,
    ActiveScriptParse_AddScriptlet,
    ActiveScriptParse_ParseScriptText
};

static IActiveScriptParse ActiveScriptParse = { &ActiveScriptParseVtbl };

static HRESULT WINAPI ActiveScript_QueryInterface(IActiveScript *iface, REFIID riid, void **ppv)
{
    if (IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IActiveScript, riid))
    {
        *ppv = iface;
        return S_OK;
    }

    if (IsEqualGUID(&IID_IActiveScriptParse, riid))
    {
        CHECK_EXPECT(QI_IActiveScriptParse);
        *ppv = &ActiveScriptParse;
        return S_OK;
    }

    ok(0, "unexpected riid %s\n", wine_dbgstr_guid(riid));
    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI ActiveScript_AddRef(IActiveScript *iface)
{
    return 2;
}

static ULONG WINAPI ActiveScript_Release(IActiveScript *iface)
{
    return 1;
}

static HRESULT WINAPI ActiveScript_SetScriptSite(IActiveScript *iface, IActiveScriptSite *pass)
{
    IActiveScriptSiteInterruptPoll *poll;
    IActiveScriptSiteWindow *window;
    IActiveScriptSiteDebug *debug;
    IServiceProvider *service;
    ICanHandleException *canexception;
    LCID lcid;
    HRESULT hres;

    CHECK_EXPECT(SetScriptSite);

    ok(pass != NULL, "pass == NULL\n");

    hres = IActiveScriptSite_QueryInterface(pass, &IID_IActiveScriptSiteInterruptPoll, (void**)&poll);
    ok(hres == E_NOINTERFACE, "Got IActiveScriptSiteInterruptPoll interface: %08lx\n", hres);

    hres = IActiveScriptSite_GetLCID(pass, &lcid);
    ok(hres == S_OK, "GetLCID failed: %08lx\n", hres);

    hres = IActiveScriptSite_OnStateChange(pass, (state = SCRIPTSTATE_INITIALIZED));
    ok(hres == E_NOTIMPL, "OnStateChange failed: %08lx\n", hres);

    hres = IActiveScriptSite_QueryInterface(pass, &IID_IActiveScriptSiteDebug, (void**)&debug);
    todo_wine
    ok(hres == S_OK, "IActiveScriptSiteDebug not supported: %08lx\n", hres);
    if (SUCCEEDED(hres))
        IActiveScriptSiteDebug_Release(debug);

    hres = IActiveScriptSite_QueryInterface(pass, &IID_ICanHandleException, (void**)&canexception);
    ok(hres == E_NOINTERFACE, "Got IID_ICanHandleException interface: %08lx\n", hres);

    hres = IActiveScriptSite_QueryInterface(pass, &IID_IServiceProvider, (void**)&service);
    ok(hres == S_OK, "Could not get IServiceProvider interface: %08lx\n", hres);
    if (SUCCEEDED(hres))
        IServiceProvider_Release(service);

    hres = IActiveScriptSite_QueryInterface(pass, &IID_IActiveScriptSiteWindow, (void**)&window);
    ok(hres == S_OK, "Could not get IActiveScriptSiteWindow interface: %08lx\n", hres);
    if (window)
        IActiveScriptSiteWindow_Release(window);

    if (site)
        IActiveScriptSite_Release(site);
    site = pass;
    IActiveScriptSite_AddRef(site);

    return S_OK;
}

static HRESULT WINAPI ActiveScript_GetScriptSite(IActiveScript *iface, REFIID riid, void **ppv)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScript_SetScriptState(IActiveScript *iface, SCRIPTSTATE ss)
{
    switch(ss)
    {
    case SCRIPTSTATE_INITIALIZED:
        CHECK_EXPECT(SetScriptState_INITIALIZED);
        break;
    case SCRIPTSTATE_UNINITIALIZED:
        CHECK_EXPECT(SetScriptState_UNINITIALIZED);
        break;
    case SCRIPTSTATE_STARTED:
        CHECK_EXPECT(SetScriptState_STARTED);
        break;
    default:
        ok(0, "unexpected call, state %u\n", ss);
    }

    return S_OK;
}

static HRESULT WINAPI ActiveScript_GetScriptState(IActiveScript *iface, SCRIPTSTATE *pssState)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScript_Close(IActiveScript *iface)
{
    CHECK_EXPECT(Close);
    if (site)
    {
        IActiveScriptSite_Release(site);
        site = NULL;
    }
    return S_OK;
}

static HRESULT WINAPI ActiveScript_AddNamedItem(IActiveScript *iface, LPCOLESTR name, DWORD flags)
{
    if (!wcscmp(name, L"scriptlet"))
    {
        CHECK_EXPECT(AddNamedItem_scriptlet);
        ok(flags == (SCRIPTITEM_ISVISIBLE|SCRIPTITEM_GLOBALMEMBERS), "got flags %#lx\n", flags);
    }
    else if (!wcscmp(name, L"globals"))
    {
        CHECK_EXPECT(AddNamedItem_globals);
        ok(flags == SCRIPTITEM_ISVISIBLE, "got flags %#lx\n", flags);
    }
    else
    {
        ok(0, "got name %s\n", wine_dbgstr_w(name));
        return E_FAIL;
    }
    return S_OK;
}

static HRESULT WINAPI ActiveScript_AddTypeLib(IActiveScript *iface, REFGUID rguidTypeLib,
        DWORD dwMajor, DWORD dwMinor, DWORD dwFlags)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScript_GetScriptDispatch(IActiveScript *iface, LPCOLESTR item_name,
        IDispatch **ppdisp)
{
    CHECK_EXPECT(GetScriptDispatch);
    ok(!item_name, "item name %s\n", wine_dbgstr_w(item_name));
    IDispatch_AddRef(*ppdisp = (IDispatch*)&script_global);
    return S_OK;
}

static HRESULT WINAPI ActiveScript_GetCurrentScriptThreadID(IActiveScript *iface,
        SCRIPTTHREADID *pstridThread)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScript_GetScriptThreadID(IActiveScript *iface,
        DWORD dwWin32ThreadId, SCRIPTTHREADID *pstidThread)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScript_GetScriptThreadState(IActiveScript *iface,
        SCRIPTTHREADID stidThread, SCRIPTTHREADSTATE *pstsState)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScript_InterruptScriptThread(IActiveScript *iface,
        SCRIPTTHREADID stidThread, const EXCEPINFO *pexcepinfo, DWORD dwFlags)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScript_Clone(IActiveScript *iface, IActiveScript **ppscript)
{
    CHECK_EXPECT(Clone);

    if (support_clone)
    {
        *ppscript = iface;
        return S_OK;
    }

    if (parse_flags == SCRIPTTEXT_ISVISIBLE) return E_NOTIMPL;

    CHECK_CALLED(CreateInstance);
    CHECK_CALLED(QI_IActiveScriptParse);
    CHECK_CALLED(InitNew);
    CHECK_CALLED(SetScriptSite);
    CHECK_CALLED(ParseScriptText);
    CHECK_CALLED(SetScriptState_UNINITIALIZED);

    SET_EXPECT(CreateInstance);
    SET_EXPECT(QI_IActiveScriptParse);
    SET_EXPECT(InitNew);
    SET_EXPECT(SetScriptSite);
    SET_EXPECT(AddNamedItem_scriptlet);
    SET_EXPECT(AddNamedItem_globals);
    SET_EXPECT(GetScriptDispatch);
    SET_EXPECT(GetDispID_vbAddOne);
    SET_EXPECT(GetDispID_wtTest);
    SET_EXPECT(GetDispID_get_gsProp);
    SET_EXPECT(GetDispID_put_gsProp);
    SET_EXPECT(SetScriptState_STARTED);
    SET_EXPECT(ParseScriptText);

    parse_flags = SCRIPTTEXT_ISVISIBLE;
    return E_NOTIMPL;
}

static const IActiveScriptVtbl ActiveScriptVtbl = {
    ActiveScript_QueryInterface,
    ActiveScript_AddRef,
    ActiveScript_Release,
    ActiveScript_SetScriptSite,
    ActiveScript_GetScriptSite,
    ActiveScript_SetScriptState,
    ActiveScript_GetScriptState,
    ActiveScript_Close,
    ActiveScript_AddNamedItem,
    ActiveScript_AddTypeLib,
    ActiveScript_GetScriptDispatch,
    ActiveScript_GetCurrentScriptThreadID,
    ActiveScript_GetScriptThreadID,
    ActiveScript_GetScriptThreadState,
    ActiveScript_InterruptScriptThread,
    ActiveScript_Clone
};

static IActiveScript ActiveScript = { &ActiveScriptVtbl };

static HRESULT WINAPI ClassFactory_QueryInterface(IClassFactory *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IClassFactory, riid)) {
        *ppv = iface;
        return S_OK;
    }

    if(IsEqualGUID(&IID_IMarshal, riid) || IsEqualGUID(&IID_IClassFactoryEx, riid))
        return E_NOINTERFACE;

    ok(0, "unexpected riid %s\n", wine_dbgstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI ClassFactory_AddRef(IClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI ClassFactory_Release(IClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI ClassFactory_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **ppv)
{
    CHECK_EXPECT(CreateInstance);

    ok(!outer, "outer = %p\n", outer);
    ok(IsEqualGUID(&IID_IUnknown, riid), "unexpected riid %s\n", wine_dbgstr_guid(riid));
    *ppv = &ActiveScript;
    return S_OK;
}

static HRESULT WINAPI ClassFactory_LockServer(IClassFactory *iface, BOOL dolock)
{
    ok(0, "unexpected call\n");
    return S_OK;
}

static const IClassFactoryVtbl ClassFactoryVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    ClassFactory_CreateInstance,
    ClassFactory_LockServer
};

static IClassFactory script_cf = { &ClassFactoryVtbl };

static BOOL init_key(const char *key_name, const char *def_value, BOOL init)
{
    HKEY hkey;
    DWORD res;

    if(!init) {
        RegDeleteKeyA(HKEY_CLASSES_ROOT, key_name);
        return TRUE;
    }

    res = RegCreateKeyA(HKEY_CLASSES_ROOT, key_name, &hkey);
    if(res != ERROR_SUCCESS)
        return FALSE;

    if(def_value)
        res = RegSetValueA(hkey, NULL, REG_SZ, def_value, strlen(def_value));

    RegCloseKey(hkey);

    return res == ERROR_SUCCESS;
}

static BOOL register_script_engine(BOOL init)
{
    DWORD regid;
    HRESULT hres;

    if (!init_key("WTScript\\CLSID", TESTSCRIPT_CLSID, init)) return FALSE;
    if (!init) return TRUE;

    hres = CoRegisterClassObject(&CLSID_TestScript, (IUnknown *)&script_cf,
                                 CLSCTX_INPROC_SERVER, REGCLS_MULTIPLEUSE, &regid);
    ok(hres == S_OK, "Could not register script engine: %08lx\n", hres);
    return TRUE;
}

static WCHAR *get_test_file(const char *res_name)
{
    WCHAR path[MAX_PATH], buffer[MAX_PATH], *ret;
    const char *data;
    HANDLE handle;
    DWORD size;
    HRSRC src;
    BOOL res;

    src = FindResourceA(NULL, res_name, (const char *)40);
    ok(src != NULL, "Could not find resource %s\n", res_name);
    size = SizeofResource(NULL, src);
    data = LoadResource(NULL, src);

    GetTempPathW(ARRAY_SIZE(path), path);
    GetTempFileNameW(path, L"wsc", 0, buffer);
    handle = CreateFileW(buffer, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(handle != INVALID_HANDLE_VALUE, "failed to create temp file\n");

    res = WriteFile(handle, data, size, &size, NULL);
    ok(res, "WriteFile failed: %lu\n", GetLastError());

    res = CloseHandle(handle);
    ok(res, "CloseHandle failed: %lu\n", GetLastError());

    size = (wcslen(buffer) + 1) * sizeof(WCHAR);
    ret = malloc(size);
    memcpy(ret, buffer, size);
    return ret;
}

#define test_key_value(a,b) test_key_value_(a,b,__LINE__)
static void test_key_value_(const char *key_name, const char *expect, unsigned line)
{
    char buf[512];
    LONG size = sizeof(buf);
    LSTATUS status;
    status = RegQueryValueA(HKEY_CLASSES_ROOT, key_name, buf, &size);
    ok_(__FILE__,line)(!status, "RegQueryValueA failed: %lu\n", status);
    if (*expect == '*')
        ok_(__FILE__,line)(size >= strlen(expect) + 1 && !strcmp(buf + size - strlen(expect), expect + 1),
                           "Unexpected value \"%s\", expected \"%s\"\n", buf, expect);
    else
        ok_(__FILE__,line)(size == strlen(expect) + 1 && !memicmp(buf, expect, size),
                           "Unexpected value \"%s\", expected \"%s\"\n", buf, expect);
}

static void test_key_deleted(const char *key_name)
{
    HKEY key;
    LSTATUS status;
    status = RegOpenKeyA(HKEY_CLASSES_ROOT, key_name, &key);
    ok(status == ERROR_FILE_NOT_FOUND, "RegOpenKey(\"%s\") returned %lu\n", key_name, status);
}

static void register_script_object(BOOL do_register, const WCHAR *file_name)
{
    HRESULT hres;

    parse_flags = SCRIPTTEXT_ISPERSISTENT | SCRIPTTEXT_ISVISIBLE;

    SET_EXPECT(CreateInstance);
    SET_EXPECT(QI_IActiveScriptParse);
    SET_EXPECT(InitNew);
    SET_EXPECT(SetScriptSite);
    SET_EXPECT(ParseScriptText);
    SET_EXPECT(SetScriptState_UNINITIALIZED);
    SET_EXPECT(Close);
    hres = pDllInstall(do_register, file_name);
    CHECK_CALLED(CreateInstance);
    CHECK_CALLED(QI_IActiveScriptParse);
    CHECK_CALLED(InitNew);
    CHECK_CALLED(SetScriptSite);
    CHECK_CALLED(ParseScriptText);
    CHECK_CALLED(SetScriptState_UNINITIALIZED);
    CHECK_CALLED(Close);
    ok(hres == S_OK, "DllInstall failed: %08lx\n", hres);

    if (do_register)
    {
        test_key_value("CLSID\\" CLSID_STR, "WineTest object");
        test_key_value("CLSID\\" CLSID_STR "\\InprocServer32", "*\\scrobj.dll");
        test_key_value("CLSID\\" CLSID_STR "\\ProgID", "WineTest.1.23");
        test_key_value("CLSID\\" CLSID_STR "\\VersionIndependentProgID", "WineTest");
        test_key_value("WineTest", "WineTest object");
        test_key_value("WineTest\\CLSID", CLSID_STR);
        test_key_value("WineTest.1.23", "WineTest object");
        test_key_value("WineTest.1.23\\CLSID", CLSID_STR);
    }
    else
    {
        test_key_deleted("CLSID\\" CLSID_STR);
        test_key_deleted("WineTest");
        test_key_deleted("WineTest.1.23");
    }
}

static void test_create_object(void)
{
    DISPID vb_add_one_id, js_add_two_id, wt_test_id, wt_gsprop_id, id;
    DISPID id_propput = DISPID_PROPERTYPUT;
    IDispatchEx *dispex;
    IClassFactory *cf;
    IDispatch *disp;
    IUnknown *unk;
    DISPPARAMS dp;
    EXCEPINFO ei;
    VARIANT v, r;
    BSTR str;
    HRESULT hres;

    hres = CoGetClassObject(&CLSID_WineTest, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER, NULL,
                            &IID_IClassFactory, (void**)&cf);
    ok(hres == S_OK, "Could not get class factory: %08lx\n", hres);

    parse_flags = SCRIPTTEXT_ISPERSISTENT | SCRIPTTEXT_ISVISIBLE;

    /* CreateInstance creates two script engine instances. The first one is for validation
     * and is created once per class factry. Then, when object instance is created, it tries
     * to clone that or create a new instance from scratch.
     * We don't implement Clone in tests, but we use it to check already called functions
     * and set new expected once.
     */
    SET_EXPECT(CreateInstance);
    SET_EXPECT(QI_IActiveScriptParse);
    SET_EXPECT(InitNew);
    SET_EXPECT(SetScriptSite);
    SET_EXPECT(ParseScriptText);
    SET_EXPECT(SetScriptState_UNINITIALIZED);
    SET_EXPECT(Clone);
    hres = IClassFactory_CreateInstance(cf, NULL, &IID_IUnknown, (void**)&unk);
    ok(hres == S_OK, "Could not create scriptlet instance: %08lx\n", hres);
    CHECK_CALLED(Clone);
    CHECK_CALLED(CreateInstance);
    CHECK_CALLED(QI_IActiveScriptParse);
    CHECK_CALLED(InitNew);
    CHECK_CALLED(SetScriptSite);
    CHECK_CALLED(AddNamedItem_scriptlet);
    CHECK_CALLED(AddNamedItem_globals);
    CHECK_CALLED(GetScriptDispatch);
    todo_wine
    CHECK_CALLED(GetDispID_vbAddOne);
    CHECK_CALLED(GetDispID_wtTest);
    CHECK_CALLED(GetDispID_get_gsProp);
    CHECK_CALLED(GetDispID_put_gsProp);
    CHECK_CALLED(SetScriptState_STARTED);
    CHECK_CALLED(ParseScriptText);

    hres = IUnknown_QueryInterface(unk, &IID_IDispatch, (void**)&disp);
    ok(hres == S_OK, "Could not get IDispatch iface: %08lx\n", hres);

    IDispatch_Release(disp);

    hres = IUnknown_QueryInterface(unk, &IID_IDispatchEx, (void**)&dispex);
    ok(hres == S_OK, "Could not get IDispatch iface: %08lx\n", hres);

    str = SysAllocString(L"vbAddOne");
    hres = IDispatchEx_GetDispID(dispex, str, fdexNameCaseSensitive, &vb_add_one_id);
    ok(hres == S_OK, "Could not get vkAddOne id: %08lx\n", hres);
    SysFreeString(str);

    str = SysAllocString(L"jsAddTwo");
    hres = IDispatchEx_GetDispID(dispex, str, fdexNameCaseSensitive, &js_add_two_id);
    ok(hres == S_OK, "Could not get jsAddTwo id: %08lx\n", hres);
    SysFreeString(str);

    str = SysAllocString(L"wtTest");
    hres = IDispatchEx_GetDispID(dispex, str, fdexNameCaseSensitive, &wt_test_id);
    ok(hres == S_OK, "Could not get wtTest id: %08lx\n", hres);
    SysFreeString(str);

    str = SysAllocString(L"gsProp");
    hres = IDispatchEx_GetDispID(dispex, str, fdexNameCaseSensitive, &wt_gsprop_id);
    ok(hres == S_OK, "Could not get wtTest id: %08lx\n", hres);
    SysFreeString(str);

    str = SysAllocString(L"vbaddone");
    hres = IDispatchEx_GetDispID(dispex, str, fdexNameCaseSensitive, &id);
    ok(hres == DISP_E_UNKNOWNNAME, "invalid case returned: %08lx\n", hres);
    SysFreeString(str);

    str = SysAllocString(L"vbaddone");
    hres = IDispatchEx_GetDispID(dispex, str, 0, &id);
    ok(hres == DISP_E_UNKNOWNNAME, "invalid case returned: %08lx\n", hres);
    SysFreeString(str);

    str = SysAllocString(L"vbaddone");
    hres = IDispatchEx_GetDispID(dispex, str, fdexNameCaseInsensitive, &id);
    ok(hres == S_OK, "case insensitive returned: %08lx\n", hres);
    ok(id == vb_add_one_id, "id = %lu, expected %lu\n", id, vb_add_one_id);
    SysFreeString(str);

    memset(&ei, 0, sizeof(ei));
    memset(&dp, 0, sizeof(dp));
    V_VT(&v) = VT_I4;
    V_I4(&v) = 2;
    V_VT(&r) = VT_ERROR;
    dp.cArgs = 1;
    dp.rgvarg = &v;
    hres = IDispatchEx_InvokeEx(dispex, vb_add_one_id, 0, DISPATCH_PROPERTYGET|DISPATCH_METHOD, &dp, &r, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    ok(V_VT(&r) == VT_I4, "V_VT(r) = %d\n", V_VT(&r));
    ok(V_I4(&r) == 3, "V_I4(r) = %ld\n", V_I4(&r));

    memset(&ei, 0, sizeof(ei));
    memset(&dp, 0, sizeof(dp));
    V_VT(&v) = VT_I4;
    V_I4(&v) = 4;
    V_VT(&r) = VT_ERROR;
    dp.cArgs = 1;
    dp.rgvarg = &v;
    hres = IDispatchEx_InvokeEx(dispex, js_add_two_id, 0, DISPATCH_PROPERTYGET|DISPATCH_METHOD, &dp, &r, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    ok(V_VT(&r) == VT_I4, "V_VT(r) = %d\n", V_VT(&r));
    ok(V_I4(&r) == 6, "V_I4(r) = %ld\n", V_I4(&r));

    memset(&ei, 0, sizeof(ei));
    memset(&dp, 0, sizeof(dp));
    V_VT(&v) = VT_I4;
    V_I4(&v) = 4;
    V_VT(&r) = VT_ERROR;
    dp.cArgs = 1;
    dp.rgvarg = &v;
    SET_EXPECT(InvokeEx);
    hres = IDispatchEx_InvokeEx(dispex, wt_test_id, 0x100, DISPATCH_PROPERTYGET|DISPATCH_METHOD, &dp, &r, &ei, (void*)0xdeadbeef);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    CHECK_CALLED(InvokeEx);
    ok(V_VT(&r) == VT_BOOL, "V_VT(r) = %d\n", V_VT(&r));
    ok(V_BOOL(&r) == VARIANT_TRUE, "V_I4(r) = %ld\n", V_I4(&r));

    memset(&ei, 0, sizeof(ei));
    memset(&dp, 0, sizeof(dp));
    V_VT(&r) = VT_ERROR;
    SET_EXPECT(InvokeEx);
    hres = IDispatchEx_InvokeEx(dispex, wt_test_id, 0x100, DISPATCH_METHOD, &dp, &r, &ei, (void*)0xdeadbeef);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    CHECK_CALLED(InvokeEx);
    ok(V_VT(&r) == VT_BOOL, "V_VT(r) = %d\n", V_VT(&r));
    ok(V_BOOL(&r) == VARIANT_TRUE, "V_I4(r) = %ld\n", V_I4(&r));

    memset(&ei, 0, sizeof(ei));
    memset(&dp, 0, sizeof(dp));
    V_VT(&v) = VT_I4;
    V_I4(&v) = 4;
    V_VT(&r) = VT_ERROR;
    dp.cArgs = 1;
    dp.rgvarg = &v;
    SET_EXPECT(InvokeEx_get_gsProp);
    hres = IDispatchEx_InvokeEx(dispex, wt_gsprop_id, 0, DISPATCH_PROPERTYGET|DISPATCH_METHOD, &dp, &r, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    CHECK_CALLED(InvokeEx_get_gsProp);
    ok(V_VT(&r) == VT_BOOL, "V_VT(r) = %d\n", V_VT(&r));
    ok(V_BOOL(&r) == VARIANT_TRUE, "V_I4(r) = %ld\n", V_I4(&r));

    memset(&ei, 0, sizeof(ei));
    memset(&dp, 0, sizeof(dp));
    V_VT(&v) = VT_I4;
    V_I4(&v) = 4;
    V_VT(&r) = VT_ERROR;
    dp.cArgs = 1;
    dp.rgdispidNamedArgs = &id_propput;
    dp.rgvarg = &v;
    dp.cNamedArgs = 1;
    SET_EXPECT(InvokeEx_put_gsProp);
    hres = IDispatchEx_InvokeEx(dispex, wt_gsprop_id, 0, DISPATCH_PROPERTYPUT, &dp, &r, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    CHECK_CALLED(InvokeEx_put_gsProp);
    ok(V_VT(&r) == VT_BOOL, "V_VT(r) = %d\n", V_VT(&r));
    ok(V_BOOL(&r) == VARIANT_FALSE, "V_I4(r) = %ld\n", V_I4(&r));

    hres = IDispatchEx_InvokeEx(dispex, wt_test_id, 0x100, DISPATCH_PROPERTYGET, &dp, &r, &ei, (void*)0xdeadbeef);
    ok(hres == DISP_E_MEMBERNOTFOUND, "InvokeEx returned: %08lx\n", hres);

    hres = IDispatchEx_InvokeEx(dispex, 0xdeadbeef, 0, DISPATCH_METHOD, &dp, &r, &ei, (void*)0xdeadbeef);
    ok(hres == DISP_E_MEMBERNOTFOUND, "InvokeEx returned: %08lx\n", hres);

    hres = IDispatchEx_InvokeEx(dispex, DISPID_VALUE, 0, DISPATCH_METHOD, &dp, &r, &ei, (void*)0xdeadbeef);
    ok(hres == DISP_E_MEMBERNOTFOUND, "InvokeEx returned: %08lx\n", hres);

    IDispatchEx_Release(dispex);

    SET_EXPECT(SetScriptState_UNINITIALIZED);
    SET_EXPECT(Close);
    IUnknown_Release(unk);
    CHECK_CALLED(SetScriptState_UNINITIALIZED);
    CHECK_CALLED(Close);

    parse_flags = SCRIPTTEXT_ISVISIBLE;

    SET_EXPECT(Clone);
    SET_EXPECT(CreateInstance);
    SET_EXPECT(QI_IActiveScriptParse);
    SET_EXPECT(InitNew);
    SET_EXPECT(SetScriptSite);
    SET_EXPECT(AddNamedItem_scriptlet);
    SET_EXPECT(AddNamedItem_globals);
    SET_EXPECT(GetScriptDispatch);
    SET_EXPECT(GetDispID_vbAddOne);
    SET_EXPECT(GetDispID_wtTest);
    SET_EXPECT(GetDispID_get_gsProp);
    SET_EXPECT(GetDispID_put_gsProp);
    SET_EXPECT(SetScriptState_STARTED);
    SET_EXPECT(ParseScriptText);
    hres = IClassFactory_CreateInstance(cf, NULL, &IID_IUnknown, (void**)&unk);
    ok(hres == S_OK, "Could not create scriptlet instance: %08lx\n", hres);
    CHECK_CALLED(Clone);
    CHECK_CALLED(CreateInstance);
    CHECK_CALLED(QI_IActiveScriptParse);
    CHECK_CALLED(InitNew);
    CHECK_CALLED(SetScriptSite);
    CHECK_CALLED(AddNamedItem_scriptlet);
    CHECK_CALLED(AddNamedItem_globals);
    CHECK_CALLED(GetScriptDispatch);
    todo_wine
    CHECK_CALLED(GetDispID_vbAddOne);
    CHECK_CALLED(GetDispID_wtTest);
    CHECK_CALLED(GetDispID_get_gsProp);
    CHECK_CALLED(GetDispID_put_gsProp);
    CHECK_CALLED(SetScriptState_STARTED);
    CHECK_CALLED(ParseScriptText);

    SET_EXPECT(SetScriptState_UNINITIALIZED);
    SET_EXPECT(Close);
    IUnknown_Release(unk);
    CHECK_CALLED(SetScriptState_UNINITIALIZED);
    CHECK_CALLED(Close);

    support_clone = TRUE;
    SET_EXPECT(Clone);
    SET_EXPECT(QI_IActiveScriptParse);
    SET_EXPECT(SetScriptSite);
    SET_EXPECT(AddNamedItem_scriptlet);
    SET_EXPECT(AddNamedItem_globals);
    SET_EXPECT(GetScriptDispatch);
    SET_EXPECT(GetDispID_vbAddOne);
    SET_EXPECT(GetDispID_wtTest);
    SET_EXPECT(GetDispID_get_gsProp);
    SET_EXPECT(GetDispID_put_gsProp);
    SET_EXPECT(SetScriptState_STARTED);
    hres = IClassFactory_CreateInstance(cf, NULL, &IID_IUnknown, (void**)&unk);
    ok(hres == S_OK, "Could not create scriptlet instance: %08lx\n", hres);
    CHECK_CALLED(Clone);
    CHECK_CALLED(QI_IActiveScriptParse);
    CHECK_CALLED(SetScriptSite);
    CHECK_CALLED(AddNamedItem_scriptlet);
    CHECK_CALLED(AddNamedItem_globals);
    CHECK_CALLED(GetScriptDispatch);
    todo_wine
    CHECK_CALLED(GetDispID_vbAddOne);
    CHECK_CALLED(GetDispID_wtTest);
    CHECK_CALLED(GetDispID_get_gsProp);
    CHECK_CALLED(GetDispID_put_gsProp);
    CHECK_CALLED(SetScriptState_STARTED);

    SET_EXPECT(SetScriptState_UNINITIALIZED);
    SET_EXPECT(Close);
    IUnknown_Release(unk);
    CHECK_CALLED(SetScriptState_UNINITIALIZED);
    CHECK_CALLED(Close);

    SET_EXPECT(Close);
    IClassFactory_Release(cf);
    CHECK_CALLED(Close);
}

START_TEST(scrobj)
{
    HMODULE scrobj_module;
    WCHAR *test_file;
    HRESULT hres;

    hres = CoInitialize(NULL);
    ok(hres == S_OK, "CoInitialize failed: %08lx\n", hres);

    scrobj_module = LoadLibraryA("scrobj.dll");
    ok(scrobj_module != NULL, "Could not load scrobj.dll\n");

    pDllInstall = (void *)GetProcAddress(scrobj_module, "DllInstall");
    ok(pDllInstall != NULL, "DllInstall not found in scrobj.dll\n");

    if (register_script_engine(TRUE))
    {
        test_file = get_test_file("scrobj.wsc");

        register_script_object(TRUE, test_file);
        test_create_object();
        register_script_object(FALSE, test_file);

        register_script_engine(FALSE);
        free(test_file);
    }
    else
        skip("Could not register script engine\n");

    CoUninitialize();
}
