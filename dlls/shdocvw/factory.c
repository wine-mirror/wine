/*
 * Implementation of class factory for IE Web Browser
 *
 * Copyright 2001 John R. Sheets (for CodeWeavers)
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

#include <string.h>
#include "wine/debug.h"
#include "shdocvw.h"

WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);

/**********************************************************************
 * Implement the WebBrowser class factory
 *
 * (Based on implementation in ddraw/main.c)
 */

#define FACTORY(x) ((IClassFactory*) &(x)->lpClassFactoryVtbl)

typedef struct
{
    /* IUnknown fields */
    const IClassFactoryVtbl *lpClassFactoryVtbl;
    HRESULT (*cf)(LPUNKNOWN, REFIID, LPVOID *);
    LONG ref;
} IClassFactoryImpl;


/**********************************************************************
 * WBCF_QueryInterface (IUnknown)
 */
static HRESULT WINAPI WBCF_QueryInterface(LPCLASSFACTORY iface,
                                          REFIID riid, LPVOID *ppobj)
{
    TRACE("(%s %p)\n", debugstr_guid(riid), ppobj);

    if (!ppobj)
        return E_POINTER;

    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IClassFactory, riid)) {
        *ppobj = iface;
        return S_OK;
    }

    WARN("Not supported interface %s\n", debugstr_guid(riid));

    *ppobj = NULL;
    return E_NOINTERFACE;
}

/************************************************************************
 * WBCF_AddRef (IUnknown)
 */
static ULONG WINAPI WBCF_AddRef(LPCLASSFACTORY iface)
{
    SHDOCVW_LockModule();

    return 2; /* non-heap based object */
}

/************************************************************************
 * WBCF_Release (IUnknown)
 */
static ULONG WINAPI WBCF_Release(LPCLASSFACTORY iface)
{
    SHDOCVW_UnlockModule();

    return 1; /* non-heap based object */
}

/************************************************************************
 * WBCF_CreateInstance (IClassFactory)
 */
static HRESULT WINAPI WBCF_CreateInstance(LPCLASSFACTORY iface, LPUNKNOWN pOuter,
                                          REFIID riid, LPVOID *ppobj)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *) iface;
    return This->cf(pOuter, riid, ppobj);
}

/************************************************************************
 * WBCF_LockServer (IClassFactory)
 */
static HRESULT WINAPI WBCF_LockServer(LPCLASSFACTORY iface, BOOL dolock)
{
    TRACE("(%d)\n", dolock);

    if (dolock)
        SHDOCVW_LockModule();
    else
        SHDOCVW_UnlockModule();
    
    return S_OK;
}

static const IClassFactoryVtbl WBCF_Vtbl =
{
    WBCF_QueryInterface,
    WBCF_AddRef,
    WBCF_Release,
    WBCF_CreateInstance,
    WBCF_LockServer
};

/*************************************************************************
 *              DllGetClassObject (SHDOCVW.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    HRESULT hres;

    static IClassFactoryImpl WBClassFactory = {&WBCF_Vtbl, WebBrowser_Create};

    TRACE("\n");

    if(IsEqualGUID(&CLSID_WebBrowser, rclsid))  {
        hres = create_mozctl(riid, ppv);
        if(SUCCEEDED(hres))
            return hres;
        return IClassFactory_QueryInterface(FACTORY(&WBClassFactory), riid, ppv);
    }

    /* As a last resort, figure if the CLSID belongs to a 'Shell Instance Object' */
    return SHDOCVW_GetShellInstanceObjectClassObject(rclsid, riid, ppv);
}

HRESULT register_class_object(BOOL do_reg)
{
    HRESULT hres;

    static DWORD cookie;
    static IClassFactoryImpl IEClassFactory = {&WBCF_Vtbl, InternetExplorer_Create};

    if(do_reg) {
        hres = CoRegisterClassObject(&CLSID_InternetExplorer, (IUnknown*)FACTORY(&IEClassFactory),
                                     CLSCTX_SERVER, REGCLS_MULTIPLEUSE|REGCLS_SUSPENDED, &cookie);
        if (FAILED(hres)) {
            ERR("failed to register object %08lx\n", hres);
            return hres;
        }

        hres = CoResumeClassObjects();
        if(SUCCEEDED(hres))
            return hres;

        ERR("failed to resume object %08lx\n", hres);
    }

    return CoRevokeClassObject(cookie);
}
