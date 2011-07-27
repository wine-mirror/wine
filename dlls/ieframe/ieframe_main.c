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

#include "ieframe.h"

#include "shlguid.h"
#include "isguids.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ieframe);

LONG module_ref = 0;
HINSTANCE ieframe_instance;

const char *debugstr_variant(const VARIANT *v)
{
    if(!v)
        return "(null)";

    switch(V_VT(v)) {
    case VT_EMPTY:
        return "{VT_EMPTY}";
    case VT_NULL:
        return "{VT_NULL}";
    case VT_I4:
        return wine_dbg_sprintf("{VT_I4: %d}", V_I4(v));
    case VT_R8:
        return wine_dbg_sprintf("{VT_R8: %lf}", V_R8(v));
    case VT_BSTR:
        return wine_dbg_sprintf("{VT_BSTR: %s}", debugstr_w(V_BSTR(v)));
    case VT_DISPATCH:
        return wine_dbg_sprintf("{VT_DISPATCH: %p}", V_DISPATCH(v));
    case VT_BOOL:
        return wine_dbg_sprintf("{VT_BOOL: %x}", V_BOOL(v));
    default:
        return wine_dbg_sprintf("{vt %d}", V_VT(v));
    }
}

static ITypeInfo *wb_typeinfo = NULL;

HRESULT get_typeinfo(ITypeInfo **typeinfo)
{
    ITypeLib *typelib;
    HRESULT hres;

    if(wb_typeinfo) {
        *typeinfo = wb_typeinfo;
        return S_OK;
    }

    hres = LoadRegTypeLib(&LIBID_SHDocVw, 1, 1, LOCALE_SYSTEM_DEFAULT, &typelib);
    if(FAILED(hres)) {
        ERR("LoadRegTypeLib failed: %08x\n", hres);
        return hres;
    }

    hres = ITypeLib_GetTypeInfoOfGuid(typelib, &IID_IWebBrowser2, &wb_typeinfo);
    ITypeLib_Release(typelib);

    *typeinfo = wb_typeinfo;
    return hres;
}

static HRESULT WINAPI ClassFactory_QueryInterface(IClassFactory *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", iface, ppv);
        *ppv = iface;
    }else if(IsEqualGUID(&IID_IClassFactory, riid)) {
        TRACE("(%p)->(IID_IClassFactory %p)\n", iface, ppv);
        *ppv = iface;
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    FIXME("(%p)->(%s %p)\n", iface, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI ClassFactory_AddRef(IClassFactory *iface)
{
    TRACE("(%p)\n", iface);
    return 2;
}

static ULONG WINAPI ClassFactory_Release(IClassFactory *iface)
{
    TRACE("(%p)\n", iface);
    return 1;
}

static HRESULT WINAPI ClassFactory_LockServer(IClassFactory *iface, BOOL fLock)
{
    TRACE("(%p)->(%x)\n", iface, fLock);
    return S_OK;
}

static const IClassFactoryVtbl WebBrowserFactoryVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    WebBrowser_Create,
    ClassFactory_LockServer
};

static IClassFactory WebBrowserFactory = { &WebBrowserFactoryVtbl };

static const IClassFactoryVtbl WebBrowserV1FactoryVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    WebBrowserV1_Create,
    ClassFactory_LockServer
};

static IClassFactory WebBrowserV1Factory = { &WebBrowserV1FactoryVtbl };

static const IClassFactoryVtbl InternetShortcutFactoryVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    InternetShortcut_Create,
    ClassFactory_LockServer
};

static IClassFactory InternetShortcutFactory = { &InternetShortcutFactoryVtbl };

static const IClassFactoryVtbl CUrlHistoryFactoryVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    CUrlHistory_Create,
    ClassFactory_LockServer
};

static IClassFactory CUrlHistoryFactory = { &CUrlHistoryFactoryVtbl };

static const IClassFactoryVtbl TaskbarListFactoryVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    TaskbarList_Create,
    ClassFactory_LockServer
};

static IClassFactory TaskbarListFactory = { &TaskbarListFactoryVtbl };

/******************************************************************
 *              DllMain (ieframe.@)
 */
BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
    TRACE("(%p %d %p)\n", hInstDLL, fdwReason, lpv);

    switch(fdwReason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE;  /* prefer native version */
    case DLL_PROCESS_ATTACH:
        ieframe_instance = hInstDLL;
        register_iewindow_class();
        DisableThreadLibraryCalls(ieframe_instance);
        break;
    case DLL_PROCESS_DETACH:
        unregister_iewindow_class();
        if(wb_typeinfo)
            ITypeInfo_Release(wb_typeinfo);
    }

    return TRUE;
}

/***********************************************************************
 *		DllGetClassObject	(ieframe.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    if(IsEqualGUID(&CLSID_WebBrowser, rclsid)) {
        TRACE("(CLSID_WebBrowser %s %p)\n", debugstr_guid(riid), ppv);
        return IClassFactory_QueryInterface(&WebBrowserFactory, riid, ppv);
    }

    if(IsEqualGUID(&CLSID_WebBrowser_V1, rclsid)) {
        TRACE("(CLSID_WebBrowser_V1 %s %p)\n", debugstr_guid(riid), ppv);
        return IClassFactory_QueryInterface(&WebBrowserV1Factory, riid, ppv);
    }


    if(IsEqualGUID(rclsid, &CLSID_InternetShortcut)) {
        TRACE("(CLSID_InternetShortcut %s %p)\n", debugstr_guid(riid), ppv);
        return IClassFactory_QueryInterface(&InternetShortcutFactory, riid, ppv);
    }

    if(IsEqualGUID(&CLSID_CUrlHistory, rclsid)) {
        TRACE("(CLSID_CUrlHistory %s %p)\n", debugstr_guid(riid), ppv);
        return IClassFactory_QueryInterface(&CUrlHistoryFactory, riid, ppv);
    }

    if(IsEqualGUID(&CLSID_TaskbarList, rclsid)) {
        TRACE("(CLSID_TaskbarList %s %p)\n", debugstr_guid(riid), ppv);
        return IClassFactory_QueryInterface(&TaskbarListFactory, riid, ppv);
    }


    FIXME("%s %s %p\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}

static const IClassFactoryVtbl InternetExplorerFactoryVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    InternetExplorer_Create,
    ClassFactory_LockServer
};

static IClassFactory InternetExplorerFactory = { &InternetExplorerFactoryVtbl };

HRESULT register_class_object(BOOL do_reg)
{
    HRESULT hres;

    static DWORD cookie;

    if(do_reg) {
        hres = CoRegisterClassObject(&CLSID_InternetExplorer,
                (IUnknown*)&InternetExplorerFactory, CLSCTX_SERVER,
                REGCLS_MULTIPLEUSE|REGCLS_SUSPENDED, &cookie);
        if (FAILED(hres)) {
            ERR("failed to register object %08x\n", hres);
            return hres;
        }

        hres = CoResumeClassObjects();
        if(SUCCEEDED(hres))
            return hres;

        ERR("failed to resume object %08x\n", hres);
    }

    return CoRevokeClassObject(cookie);
}

/***********************************************************************
 *          DllCanUnloadNow (ieframe.@)
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    TRACE("()\n");
    return module_ref ? S_FALSE : S_OK;
}

/***********************************************************************
 *          DllRegisterServer (ieframe.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    FIXME("()\n");
    return S_OK;
}

/***********************************************************************
 *          DllUnregisterServer (ieframe.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    FIXME("()\n");
    return S_OK;
}
