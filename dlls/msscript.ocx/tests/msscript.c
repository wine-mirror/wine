/*
 * Copyright 2016 Nikolay Sivov for CodeWeavers
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
#include <olectl.h>
#include "dispex.h"
#include "activscp.h"
#include "activdbg.h"
#include "objsafe.h"

#include "msscript.h"
#include "wine/test.h"

#define TESTSCRIPT_CLSID "{178fc164-f585-4e24-9c13-4bb7faf80746}"
static const GUID CLSID_TestScript =
    {0x178fc164,0xf585,0x4e24,{0x9c,0x13,0x4b,0xb7,0xfa,0xf8,0x07,0x46}};
static const WCHAR vbW[] = {'V','B','S','c','r','i','p','t',0};

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
DEFINE_EXPECT(SetInterfaceSafetyOptions);
DEFINE_EXPECT(InitNew);
DEFINE_EXPECT(Close);
DEFINE_EXPECT(SetScriptSite);
DEFINE_EXPECT(QI_IActiveScriptParse);
DEFINE_EXPECT(SetScriptState_INITIALIZED);
DEFINE_EXPECT(AddNamedItem);

#define EXPECT_REF(obj,ref) _expect_ref((IUnknown*)obj, ref, __LINE__)
static void _expect_ref(IUnknown* obj, ULONG ref, int line)
{
    ULONG rc;
    IUnknown_AddRef(obj);
    rc = IUnknown_Release(obj);
    ok_(__FILE__,line)(rc == ref, "expected refcount %d, got %d\n", ref, rc);
}

static IActiveScriptSite *site;
static SCRIPTSTATE state;

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
        LPCOLESTR pstrCode, LPCOLESTR pstrItemName, IUnknown *punkContext,
        LPCOLESTR pstrDelimiter, CTXARG_T dwSourceContextCookie, ULONG ulStartingLine,
        DWORD dwFlags, VARIANT *pvarResult, EXCEPINFO *pexcepinfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
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

static HRESULT WINAPI ObjectSafety_QueryInterface(IObjectSafety *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;
    ok(0, "unexpected call %s\n", wine_dbgstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI ObjectSafety_AddRef(IObjectSafety *iface)
{
    return 2;
}

static ULONG WINAPI ObjectSafety_Release(IObjectSafety *iface)
{
    return 1;
}

static HRESULT WINAPI ObjectSafety_GetInterfaceSafetyOptions(IObjectSafety *iface, REFIID riid,
        DWORD *pdwSupportedOptions, DWORD *pdwEnabledOptions)
{
    ok(0, "unexpected riid %s\n", wine_dbgstr_guid(riid));
    return E_NOTIMPL;
}

static HRESULT WINAPI ObjectSafety_SetInterfaceSafetyOptions(IObjectSafety *iface, REFIID riid,
        DWORD mask, DWORD options)
{
    CHECK_EXPECT(SetInterfaceSafetyOptions);

    ok(IsEqualGUID(&IID_IActiveScriptParse, riid), "unexpected riid %s\n", wine_dbgstr_guid(riid));

    ok(mask == INTERFACESAFE_FOR_UNTRUSTED_DATA, "option mask = %x\n", mask);
    ok(options == 0, "options = %x\n", options);

    return S_OK;
}

static const IObjectSafetyVtbl ObjectSafetyVtbl = {
    ObjectSafety_QueryInterface,
    ObjectSafety_AddRef,
    ObjectSafety_Release,
    ObjectSafety_GetInterfaceSafetyOptions,
    ObjectSafety_SetInterfaceSafetyOptions
};

static IObjectSafety ObjectSafety = { &ObjectSafetyVtbl };

static HRESULT WINAPI ActiveScript_QueryInterface(IActiveScript *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IActiveScript, riid)) {
        *ppv = iface;
        return S_OK;
    }

    if(IsEqualGUID(&IID_IObjectSafety, riid)) {
        *ppv = &ObjectSafety;
        return S_OK;
    }

    if(IsEqualGUID(&IID_IActiveScriptParse, riid)) {
        CHECK_EXPECT(QI_IActiveScriptParse);
        *ppv = &ActiveScriptParse;
        return S_OK;
    }

    if(IsEqualGUID(&IID_IActiveScriptGarbageCollector, riid))
        return E_NOINTERFACE;

    ok(0, "unexpected riid %s\n", wine_dbgstr_guid(riid));
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
    ICanHandleException *canexpection;
    LCID lcid;
    HRESULT hres;

    CHECK_EXPECT(SetScriptSite);

    ok(pass != NULL, "pass == NULL\n");

    hres = IActiveScriptSite_QueryInterface(pass, &IID_IActiveScriptSiteInterruptPoll, (void**)&poll);
    ok(hres == E_NOINTERFACE, "Got IActiveScriptSiteInterruptPoll interface: %08x\n", hres);

    hres = IActiveScriptSite_GetLCID(pass, &lcid);
    ok(hres == S_OK, "GetLCID failed: %08x\n", hres);

    hres = IActiveScriptSite_OnStateChange(pass, (state = SCRIPTSTATE_INITIALIZED));
    ok(hres == E_NOTIMPL, "OnStateChange failed: %08x\n", hres);

    hres = IActiveScriptSite_QueryInterface(pass, &IID_IActiveScriptSiteDebug, (void**)&debug);
    ok(hres == E_NOINTERFACE, "Got IActiveScriptSiteDebug interface: %08x\n", hres);

    hres = IActiveScriptSite_QueryInterface(pass, &IID_ICanHandleException, (void**)&canexpection);
    ok(hres == E_NOINTERFACE, "Got IID_ICanHandleException interface: %08x\n", hres);

    hres = IActiveScriptSite_QueryInterface(pass, &IID_IServiceProvider, (void**)&service);
    ok(hres == S_OK, "Could not get IServiceProvider interface: %08x\n", hres);
    IServiceProvider_Release(service);

    hres = IActiveScriptSite_QueryInterface(pass, &IID_IActiveScriptSiteWindow, (void**)&window);
    ok(hres == S_OK, "Could not get IActiveScriptSiteWindow interface: %08x\n", hres);
    IActiveScriptSiteWindow_Release(window);

    site = pass;
    IActiveScriptSite_AddRef(site);
    return S_OK;
}

static HRESULT WINAPI ActiveScript_GetScriptSite(IActiveScript *iface, REFIID riid,
                                            void **ppvObject)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScript_SetScriptState(IActiveScript *iface, SCRIPTSTATE ss)
{
    if (ss == SCRIPTSTATE_INITIALIZED) {
        CHECK_EXPECT(SetScriptState_INITIALIZED);
        return S_OK;
    }
    else
        ok(0, "unexpected call, state %u\n", ss);

    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScript_GetScriptState(IActiveScript *iface, SCRIPTSTATE *pssState)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScript_Close(IActiveScript *iface)
{
    CHECK_EXPECT(Close);
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScript_AddNamedItem(IActiveScript *iface, LPCOLESTR name, DWORD flags)
{
    static const WCHAR oW[] = {'o',0};
    CHECK_EXPECT(AddNamedItem);
    ok(!lstrcmpW(name, oW), "got name %s\n", wine_dbgstr_w(name));
    ok(flags == (SCRIPTITEM_ISVISIBLE|SCRIPTITEM_ISSOURCE|SCRIPTITEM_GLOBALMEMBERS), "got flags %#x\n", flags);
    return S_OK;
}

static HRESULT WINAPI ActiveScript_AddTypeLib(IActiveScript *iface, REFGUID rguidTypeLib,
                                         DWORD dwMajor, DWORD dwMinor, DWORD dwFlags)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScript_GetScriptDispatch(IActiveScript *iface, LPCOLESTR pstrItemName,
                                                IDispatch **ppdisp)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
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
    ok(0, "unexpected call\n");
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

    if(IsEqualGUID(&IID_IMarshal, riid))
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
    ok(IsEqualGUID(&IID_IActiveScript, riid), "unexpected riid %s\n", wine_dbgstr_guid(riid));
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

static BOOL init_registry(BOOL init)
{
    return init_key("TestScript\\CLSID", TESTSCRIPT_CLSID, init)
        && init_key("CLSID\\"TESTSCRIPT_CLSID"\\Implemented Categories\\{F0B7A1A1-9847-11CF-8F20-00805F2CD064}",
                    NULL, init)
        && init_key("CLSID\\"TESTSCRIPT_CLSID"\\Implemented Categories\\{F0B7A1A2-9847-11CF-8F20-00805F2CD064}",
                    NULL, init);
}

static BOOL register_script_engine(void)
{
    DWORD regid;
    HRESULT hres;

    if(!init_registry(TRUE)) {
        init_registry(FALSE);
        return FALSE;
    }

    hres = CoRegisterClassObject(&CLSID_TestScript, (IUnknown *)&script_cf,
                                 CLSCTX_INPROC_SERVER, REGCLS_MULTIPLEUSE, &regid);
    ok(hres == S_OK, "Could not register script engine: %08x\n", hres);

    return TRUE;
}

static HRESULT WINAPI OleClientSite_QueryInterface(IOleClientSite *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IOleClientSite) || IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IOleClientSite_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI OleClientSite_AddRef(IOleClientSite *iface)
{
    return 2;
}

static ULONG WINAPI OleClientSite_Release(IOleClientSite *iface)
{
    return 1;
}

static HRESULT WINAPI OleClientSite_SaveObject(IOleClientSite *iface)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI OleClientSite_GetMoniker(IOleClientSite *iface, DWORD assign,
    DWORD which, IMoniker **moniker)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI OleClientSite_GetContainer(IOleClientSite *iface, IOleContainer **container)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI OleClientSite_ShowObject(IOleClientSite *iface)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI OleClientSite_OnShowWindow(IOleClientSite *iface, BOOL show)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI OleClientSite_RequestNewObjectLayout(IOleClientSite *iface)
{
    return E_NOTIMPL;
}

static const IOleClientSiteVtbl OleClientSiteVtbl = {
    OleClientSite_QueryInterface,
    OleClientSite_AddRef,
    OleClientSite_Release,
    OleClientSite_SaveObject,
    OleClientSite_GetMoniker,
    OleClientSite_GetContainer,
    OleClientSite_ShowObject,
    OleClientSite_OnShowWindow,
    OleClientSite_RequestNewObjectLayout
};

static IOleClientSite testclientsite = { &OleClientSiteVtbl };

static void test_oleobject(void)
{
    DWORD status, dpi_x, dpi_y;
    IOleClientSite *site;
    IOleObject *obj;
    SIZEL extent;
    HRESULT hr;
    HDC hdc;

    hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IOleObject, (void**)&obj);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    if (0) /* crashes on w2k3 */
        hr = IOleObject_GetMiscStatus(obj, DVASPECT_CONTENT, NULL);

    status = 0;
    hr = IOleObject_GetMiscStatus(obj, DVASPECT_CONTENT, &status);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(status != 0, "got 0x%08x\n", status);

    hr = IOleObject_SetClientSite(obj, &testclientsite);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    if (0) /* crashes on w2k3 */
        hr = IOleObject_GetClientSite(obj, NULL);

    hr = IOleObject_GetClientSite(obj, &site);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(site == &testclientsite, "got %p, %p\n", site, &testclientsite);
    IOleClientSite_Release(site);

    hr = IOleObject_SetClientSite(obj, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IOleObject_GetClientSite(obj, &site);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(site == NULL, "got %p\n", site);

    /* extents */
    hdc = GetDC(0);
    dpi_x = GetDeviceCaps(hdc, LOGPIXELSX);
    dpi_y = GetDeviceCaps(hdc, LOGPIXELSY);
    ReleaseDC(0, hdc);

    memset(&extent, 0, sizeof(extent));
    hr = IOleObject_GetExtent(obj, DVASPECT_CONTENT, &extent);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(extent.cx == MulDiv(38, 2540, dpi_x), "got %d\n", extent.cx);
    ok(extent.cy == MulDiv(38, 2540, dpi_y), "got %d\n", extent.cy);

    extent.cx = extent.cy = 0xdeadbeef;
    hr = IOleObject_GetExtent(obj, DVASPECT_THUMBNAIL, &extent);
    ok(hr == DV_E_DVASPECT, "got 0x%08x\n", hr);
    ok(extent.cx == 0xdeadbeef, "got %d\n", extent.cx);
    ok(extent.cy == 0xdeadbeef, "got %d\n", extent.cy);

    extent.cx = extent.cy = 0xdeadbeef;
    hr = IOleObject_GetExtent(obj, DVASPECT_ICON, &extent);
    ok(hr == DV_E_DVASPECT, "got 0x%08x\n", hr);
    ok(extent.cx == 0xdeadbeef, "got %d\n", extent.cx);
    ok(extent.cy == 0xdeadbeef, "got %d\n", extent.cy);

    extent.cx = extent.cy = 0xdeadbeef;
    hr = IOleObject_GetExtent(obj, DVASPECT_DOCPRINT, &extent);
    ok(hr == DV_E_DVASPECT, "got 0x%08x\n", hr);
    ok(extent.cx == 0xdeadbeef, "got %d\n", extent.cx);
    ok(extent.cy == 0xdeadbeef, "got %d\n", extent.cy);

    IOleObject_Release(obj);
}

static void test_persiststreaminit(void)
{
    IPersistStreamInit *init;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IPersistStreamInit, (void**)&init);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    IPersistStreamInit_Release(init);
}

static void test_olecontrol(void)
{
    IOleControl *olecontrol;
    CONTROLINFO info;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IOleControl, (void**)&olecontrol);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    memset(&info, 0xab, sizeof(info));
    info.cb = sizeof(info);
    hr = IOleControl_GetControlInfo(olecontrol, &info);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(info.hAccel == NULL, "got %p\n", info.hAccel);
    ok(info.cAccel == 0, "got %d\n", info.cAccel);
    ok(info.dwFlags == 0xabababab, "got %x\n", info.dwFlags);

    memset(&info, 0xab, sizeof(info));
    info.cb = sizeof(info) - 1;
    hr = IOleControl_GetControlInfo(olecontrol, &info);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(info.hAccel == NULL, "got %p\n", info.hAccel);
    ok(info.cAccel == 0, "got %d\n", info.cAccel);
    ok(info.dwFlags == 0xabababab, "got %x\n", info.dwFlags);

    if (0) /* crashes on win2k3 */
    {
        hr = IOleControl_GetControlInfo(olecontrol, NULL);
        ok(hr == E_POINTER, "got 0x%08x\n", hr);
    }

    IOleControl_Release(olecontrol);
}

static void test_Language(void)
{
    static const WCHAR jsW[] = {'J','S','c','r','i','p','t',0};
    static const WCHAR vb2W[] = {'v','B','s','c','r','i','p','t',0};
    static const WCHAR dummyW[] = {'d','u','m','m','y',0};
    IScriptControl *sc;
    HRESULT hr;
    BSTR str;

    hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IScriptControl, (void**)&sc);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IScriptControl_get_Language(sc, NULL);
    ok(hr == E_POINTER, "got 0x%08x\n", hr);

    str = (BSTR)0xdeadbeef;
    hr = IScriptControl_get_Language(sc, &str);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(str == NULL, "got %s\n", wine_dbgstr_w(str));

    str = SysAllocString(vbW);
    hr = IScriptControl_put_Language(sc, str);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    SysFreeString(str);

    str = SysAllocString(vb2W);
    hr = IScriptControl_put_Language(sc, str);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    SysFreeString(str);

    hr = IScriptControl_get_Language(sc, &str);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(!lstrcmpW(str, vbW), "got %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    str = SysAllocString(dummyW);
    hr = IScriptControl_put_Language(sc, str);
    ok(hr == CTL_E_INVALIDPROPERTYVALUE, "got 0x%08x\n", hr);
    SysFreeString(str);

    hr = IScriptControl_get_Language(sc, &str);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(!lstrcmpW(str, vbW), "got %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    str = SysAllocString(jsW);
    hr = IScriptControl_put_Language(sc, str);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    SysFreeString(str);

    hr = IScriptControl_get_Language(sc, &str);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(!lstrcmpW(str, jsW), "got %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    hr = IScriptControl_put_Language(sc, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IScriptControl_get_Language(sc, &str);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(str == NULL, "got %s\n", wine_dbgstr_w(str));

    IScriptControl_Release(sc);

    /* custom script engine */
    if (register_script_engine()) {
        static const WCHAR testscriptW[] = {'t','e','s','t','s','c','r','i','p','t',0};

        hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
                &IID_IScriptControl, (void**)&sc);
        ok(hr == S_OK, "got 0x%08x\n", hr);

        SET_EXPECT(CreateInstance);
        SET_EXPECT(SetInterfaceSafetyOptions);
        SET_EXPECT(SetScriptSite);
        SET_EXPECT(QI_IActiveScriptParse);
        SET_EXPECT(InitNew);

        str = SysAllocString(testscriptW);
        hr = IScriptControl_put_Language(sc, str);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        SysFreeString(str);

        CHECK_CALLED(CreateInstance);
        CHECK_CALLED(SetInterfaceSafetyOptions);
        CHECK_CALLED(SetScriptSite);
        CHECK_CALLED(QI_IActiveScriptParse);
        CHECK_CALLED(InitNew);
        hr = IScriptControl_get_Language(sc, &str);
    todo_wine
        ok(hr == S_OK, "got 0x%08x\n", hr);
     if (hr == S_OK)
        ok(!lstrcmpW(testscriptW, str), "%s\n", wine_dbgstr_w(str));
        SysFreeString(str);

        init_registry(FALSE);

        SET_EXPECT(Close);

        IScriptControl_Release(sc);

        CHECK_CALLED(Close);
    }
    else
        skip("Could not register TestScript engine\n");
}

static void test_connectionpoints(void)
{
    IConnectionPointContainer *container, *container2;
    IConnectionPoint *cp;
    IScriptControl *sc;
    HRESULT hr;
    IID iid;

    hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IScriptControl, (void**)&sc);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    EXPECT_REF(sc, 1);
    hr = IScriptControl_QueryInterface(sc, &IID_IConnectionPointContainer, (void**)&container);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    EXPECT_REF(sc, 2);
    EXPECT_REF(container, 2);

    hr = IConnectionPointContainer_FindConnectionPoint(container, &IID_IPropertyNotifySink, &cp);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    if (0) /* crashes on win2k3 */
    {
        hr = IConnectionPoint_GetConnectionPointContainer(cp, NULL);
        ok(hr == E_POINTER, "got 0x%08x\n", hr);
    }

    hr = IConnectionPoint_GetConnectionInterface(cp, &iid);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(IsEqualIID(&iid, &IID_IPropertyNotifySink), "got %s\n", wine_dbgstr_guid(&iid));

    hr = IConnectionPoint_GetConnectionPointContainer(cp, &container2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(container2 == container, "got %p, expected %p\n", container2, container);
    IConnectionPointContainer_Release(container2);

    IConnectionPoint_Release(cp);

    hr = IConnectionPointContainer_FindConnectionPoint(container, &DIID_DScriptControlSource, &cp);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    if (0) /* crashes on win2k3 */
    {
        hr = IConnectionPoint_GetConnectionPointContainer(cp, NULL);
        ok(hr == E_POINTER, "got 0x%08x\n", hr);
    }

    hr = IConnectionPoint_GetConnectionInterface(cp, &iid);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(IsEqualIID(&iid, &DIID_DScriptControlSource), "got %s\n", wine_dbgstr_guid(&iid));

    hr = IConnectionPoint_GetConnectionPointContainer(cp, &container2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(container2 == container, "got %p, expected %p\n", container2, container);
    IConnectionPointContainer_Release(container2);

    IConnectionPoint_Release(cp);

    IConnectionPointContainer_Release(container);
    IScriptControl_Release(sc);
}

static void test_quickactivate(void)
{
    IScriptControl *sc;
    IQuickActivate *qa;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IScriptControl, (void**)&sc);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IScriptControl_QueryInterface(sc, &IID_IQuickActivate, (void**)&qa);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    IQuickActivate_Release(qa);
    IScriptControl_Release(sc);
}

static void test_viewobject(void)
{
    DWORD status, aspect, flags;
    IViewObjectEx *viewex;
    IScriptControl *sc;
    IViewObject *view;
    IAdviseSink *sink;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IScriptControl, (void**)&sc);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IScriptControl_QueryInterface(sc, &IID_IViewObject, (void**)&view);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    IViewObject_Release(view);

    hr = IScriptControl_QueryInterface(sc, &IID_IViewObject2, (void**)&view);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    sink = (IAdviseSink*)0xdeadbeef;
    hr = IViewObject_GetAdvise(view, &aspect, &flags, &sink);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(aspect == DVASPECT_CONTENT, "got %u\n", aspect);
    ok(flags == 0, "got %#x\n", flags);
    ok(sink == NULL, "got %p\n", sink);

    hr = IViewObject_SetAdvise(view, DVASPECT_CONTENT, 0, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IViewObject_SetAdvise(view, DVASPECT_THUMBNAIL, 0, NULL);
    ok(hr == DV_E_DVASPECT, "got 0x%08x\n", hr);

    hr = IViewObject_SetAdvise(view, DVASPECT_ICON, 0, NULL);
    ok(hr == DV_E_DVASPECT, "got 0x%08x\n", hr);

    hr = IViewObject_SetAdvise(view, DVASPECT_DOCPRINT, 0, NULL);
    ok(hr == DV_E_DVASPECT, "got 0x%08x\n", hr);

    sink = (IAdviseSink*)0xdeadbeef;
    hr = IViewObject_GetAdvise(view, &aspect, &flags, &sink);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(aspect == DVASPECT_CONTENT, "got %u\n", aspect);
    ok(flags == 0, "got %#x\n", flags);
    ok(sink == NULL, "got %p\n", sink);

    IViewObject_Release(view);

    hr = IScriptControl_QueryInterface(sc, &IID_IViewObjectEx, (void**)&viewex);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    if (0) /* crashes */
        hr = IViewObjectEx_GetViewStatus(viewex, NULL);

    status = 0;
    hr = IViewObjectEx_GetViewStatus(viewex, &status);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(status == VIEWSTATUS_OPAQUE, "got %#x\n", status);

    IViewObjectEx_Release(viewex);

    IScriptControl_Release(sc);
}

static void test_pointerinactive(void)
{
    IPointerInactive *pi;
    IScriptControl *sc;
    DWORD policy;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IScriptControl, (void**)&sc);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IScriptControl_QueryInterface(sc, &IID_IPointerInactive, (void**)&pi);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    if (0) /* crashes w2k3 */
        hr = IPointerInactive_GetActivationPolicy(pi, NULL);

    policy = 123;
    hr = IPointerInactive_GetActivationPolicy(pi, &policy);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(policy == 0, "got %#x\n", policy);

    IPointerInactive_Release(pi);
    IScriptControl_Release(sc);
}

static void test_timeout(void)
{
    IScriptControl *sc;
    HRESULT hr;
    LONG val;
    BSTR str;

    hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IScriptControl, (void**)&sc);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IScriptControl_get_Timeout(sc, NULL);
    ok(hr == E_POINTER, "got 0x%08x\n", hr);

    val = 0;
    hr = IScriptControl_get_Timeout(sc, &val);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(val == 10000, "got %d\n", val);

    hr = IScriptControl_put_Timeout(sc, -1);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    val = 0;
    hr = IScriptControl_get_Timeout(sc, &val);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(val == -1, "got %d\n", val);

    hr = IScriptControl_put_Timeout(sc, -2);
    ok(hr == CTL_E_INVALIDPROPERTYVALUE, "got 0x%08x\n", hr);

    val = 0;
    hr = IScriptControl_get_Timeout(sc, &val);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(val == -1, "got %d\n", val);

    hr = IScriptControl_put_Timeout(sc, 0);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    val = 1;
    hr = IScriptControl_get_Timeout(sc, &val);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(val == 0, "got %d\n", val);

    str = SysAllocString(vbW);
    hr = IScriptControl_put_Language(sc, str);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    SysFreeString(str);

    val = 1;
    hr = IScriptControl_get_Timeout(sc, &val);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(val == 0, "got %d\n", val);

    hr = IScriptControl_put_Timeout(sc, 10000);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    IScriptControl_Release(sc);
}

static void test_Reset(void)
{
    IScriptControl *sc;
    HRESULT hr;
    BSTR str;

    hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IScriptControl, (void**)&sc);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IScriptControl_Reset(sc);
    ok(hr == E_FAIL, "got 0x%08x\n", hr);

    str = SysAllocString(vbW);
    hr = IScriptControl_put_Language(sc, str);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    SysFreeString(str);

    hr = IScriptControl_Reset(sc);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IScriptControl_get_Language(sc, &str);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(!lstrcmpW(str, vbW), "got %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    IScriptControl_Release(sc);

    /* custom script engine */
    if (register_script_engine()) {
        static const WCHAR testscriptW[] = {'t','e','s','t','s','c','r','i','p','t',0};

        hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
                &IID_IScriptControl, (void**)&sc);
        ok(hr == S_OK, "got 0x%08x\n", hr);

        SET_EXPECT(CreateInstance);
        SET_EXPECT(SetInterfaceSafetyOptions);
        SET_EXPECT(SetScriptSite);
        SET_EXPECT(QI_IActiveScriptParse);
        SET_EXPECT(InitNew);

        str = SysAllocString(testscriptW);
        hr = IScriptControl_put_Language(sc, str);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        SysFreeString(str);

        SET_EXPECT(SetScriptState_INITIALIZED);
        hr = IScriptControl_Reset(sc);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        CHECK_CALLED(SetScriptState_INITIALIZED);

        SET_EXPECT(SetScriptState_INITIALIZED);
        hr = IScriptControl_Reset(sc);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        CHECK_CALLED(SetScriptState_INITIALIZED);

        init_registry(FALSE);

        SET_EXPECT(Close);

        IScriptControl_Release(sc);

        CHECK_CALLED(Close);
    }
    else
        skip("Could not register TestScript engine\n");
}

static HRESULT WINAPI disp_QI(IDispatch *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IDispatch) || IsEqualIID(riid, &IID_IUnknown)) {
        *obj = iface;
        IDispatch_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI disp_AddRef(IDispatch *iface)
{
    return 2;
}

static ULONG WINAPI disp_Release(IDispatch *iface)
{
    return 1;
}

static HRESULT WINAPI disp_GetTypeInfoCount(IDispatch *iface, UINT *count)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI disp_GetTypeInfo(IDispatch *iface, UINT index, LCID lcid, ITypeInfo **ti)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI disp_GetIDsOfNames(IDispatch *iface, REFIID riid, LPOLESTR *names,
    UINT cnames, LCID lcid, DISPID *dispid)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI disp_Invoke(IDispatch *iface, DISPID dispid, REFIID riid, LCID lcid,
    WORD flags, DISPPARAMS *dispparams, VARIANT *result, EXCEPINFO *ei, UINT *arg_err)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IDispatchVtbl dispvtbl = {
    disp_QI,
    disp_AddRef,
    disp_Release,
    disp_GetTypeInfoCount,
    disp_GetTypeInfo,
    disp_GetIDsOfNames,
    disp_Invoke
};

static IDispatch testdisp = { &dispvtbl };

static void test_AddObject(void)
{
    static const WCHAR oW[] = {'o',0};
    IScriptControl *sc;
    BSTR str, objname;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IScriptControl, (void**)&sc);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    objname = SysAllocString(oW);
    hr = IScriptControl_AddObject(sc, objname, NULL, VARIANT_FALSE);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    hr = IScriptControl_AddObject(sc, objname, &testdisp, VARIANT_FALSE);
    ok(hr == E_FAIL, "got 0x%08x\n", hr);

    str = SysAllocString(vbW);
    hr = IScriptControl_put_Language(sc, str);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    SysFreeString(str);

    hr = IScriptControl_AddObject(sc, objname, &testdisp, VARIANT_TRUE);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IScriptControl_Reset(sc);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IScriptControl_AddObject(sc, objname, &testdisp, VARIANT_TRUE);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    IScriptControl_Release(sc);

    /* custom script engine */
    if (register_script_engine()) {
        static const WCHAR testscriptW[] = {'t','e','s','t','s','c','r','i','p','t',0};

        hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
                &IID_IScriptControl, (void**)&sc);
        ok(hr == S_OK, "got 0x%08x\n", hr);

        SET_EXPECT(CreateInstance);
        SET_EXPECT(SetInterfaceSafetyOptions);
        SET_EXPECT(SetScriptSite);
        SET_EXPECT(QI_IActiveScriptParse);
        SET_EXPECT(InitNew);

        str = SysAllocString(testscriptW);
        hr = IScriptControl_put_Language(sc, str);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        SysFreeString(str);

        hr = IScriptControl_AddObject(sc, objname, NULL, VARIANT_FALSE);
        ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

        SET_EXPECT(AddNamedItem);
        hr = IScriptControl_AddObject(sc, objname, &testdisp, VARIANT_TRUE);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        CHECK_CALLED(AddNamedItem);

        hr = IScriptControl_AddObject(sc, objname, &testdisp, VARIANT_TRUE);
        ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

        init_registry(FALSE);

        SET_EXPECT(Close);

        IScriptControl_Release(sc);

        CHECK_CALLED(Close);
    }
    else
        skip("Could not register TestScript engine\n");

    SysFreeString(objname);
}

static void test_AllowUI(void)
{
    IScriptControl *sc;
    VARIANT_BOOL allow;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IScriptControl, (void**)&sc);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IScriptControl_get_AllowUI(sc, NULL);
    ok(hr == E_POINTER, "got 0x%08x\n", hr);

    hr = IScriptControl_get_AllowUI(sc, &allow);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(allow == VARIANT_TRUE, "got %d\n", allow);

    hr = IScriptControl_put_AllowUI(sc, VARIANT_FALSE);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IScriptControl_get_AllowUI(sc, &allow);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(allow == VARIANT_FALSE, "got %d\n", allow);

    IScriptControl_Release(sc);
}

static void test_UseSafeSubset(void)
{
    IScriptControl *sc;
    VARIANT_BOOL use_safe_subset;
    HRESULT hr;
    BSTR str;

    hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IScriptControl, (void**)&sc);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IScriptControl_get_UseSafeSubset(sc, NULL);
    ok(hr == E_POINTER, "got 0x%08x\n", hr);

    hr = IScriptControl_get_UseSafeSubset(sc, &use_safe_subset);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(use_safe_subset == VARIANT_FALSE, "got %d\n", use_safe_subset);

    hr = IScriptControl_put_UseSafeSubset(sc, VARIANT_TRUE);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IScriptControl_get_UseSafeSubset(sc, &use_safe_subset);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(use_safe_subset == VARIANT_TRUE, "got %d\n", use_safe_subset);

    str = SysAllocString(vbW);
    hr = IScriptControl_put_Language(sc, str);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    SysFreeString(str);

    hr = IScriptControl_get_UseSafeSubset(sc, &use_safe_subset);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(use_safe_subset == VARIANT_TRUE, "got %d\n", use_safe_subset);

    IScriptControl_Release(sc);
}

static void test_State(void)
{
    IScriptControl *sc;
    ScriptControlStates state;
    HRESULT hr;
    BSTR str;

    hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IScriptControl, (void**)&sc);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IScriptControl_get_State(sc, NULL);
    ok(hr == E_POINTER, "got 0x%08x\n", hr);

    hr = IScriptControl_get_State(sc, &state);
    ok(hr == E_FAIL, "got 0x%08x\n", hr);

    hr = IScriptControl_put_State(sc, Connected);
    ok(hr == E_FAIL, "got 0x%08x\n", hr);

    str = SysAllocString(vbW);
    hr = IScriptControl_put_Language(sc, str);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    SysFreeString(str);

    hr = IScriptControl_get_State(sc, &state);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(state == Initialized, "got %d\n", state);

    hr = IScriptControl_put_State(sc, Connected);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IScriptControl_get_State(sc, &state);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(state == Connected, "got %d\n", state);

    hr = IScriptControl_put_State(sc, 2);
    ok(hr == CTL_E_INVALIDPROPERTYVALUE, "got 0x%08x\n", hr);

    hr = IScriptControl_get_State(sc, &state);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(state == Connected, "got %d\n", state);

    hr = IScriptControl_put_State(sc, -1);
    ok(hr == CTL_E_INVALIDPROPERTYVALUE, "got 0x%08x\n", hr);

    hr = IScriptControl_get_State(sc, &state);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(state == Connected, "got %d\n", state);

    IScriptControl_Release(sc);
}

START_TEST(msscript)
{
    IUnknown *unk;
    HRESULT hr;

    CoInitialize(NULL);

    hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IUnknown, (void**)&unk);
    if (FAILED(hr)) {
        win_skip("Could not create ScriptControl object: %08x\n", hr);
        return;
    }
    IUnknown_Release(unk);

    test_oleobject();
    test_persiststreaminit();
    test_olecontrol();
    test_Language();
    test_connectionpoints();
    test_quickactivate();
    test_viewobject();
    test_pointerinactive();
    test_timeout();
    test_Reset();
    test_AddObject();
    test_AllowUI();
    test_UseSafeSubset();
    test_State();

    CoUninitialize();
}
