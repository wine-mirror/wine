/*
 *    MSHTML Class Factory
 *
 * Copyright 2002 Lionel Ulmer
 * Copyright 2003 Mike McCormack
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"

#include "uuids.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

extern HRESULT HTMLDocument_create(IUnknown *pUnkOuter, LPVOID *ppObj);

/* For the moment, do nothing here. */
BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
    switch(fdwReason) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hInstDLL);
	    break;
	case DLL_PROCESS_DETACH:
	    break;
    }
    return TRUE;
}

/******************************************************************************
 * MSHTML ClassFactory
 */
typedef struct {
    IClassFactory ITF_IClassFactory;

    DWORD ref;
    HRESULT (*pfnCreateInstance)(IUnknown *pUnkOuter, LPVOID *ppObj);
} IClassFactoryImpl;

struct object_creation_info
{
    const CLSID *clsid;
    LPCSTR szClassName;
    HRESULT (*pfnCreateInstance)(IUnknown *pUnkOuter, LPVOID *ppObj);
};

static const struct object_creation_info object_creation[] =
{
    { &CLSID_HTMLDocument, "HTMLDocument", HTMLDocument_create },
};

static HRESULT WINAPI
HTMLCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj)
{
    ICOM_THIS(IClassFactoryImpl,iface);

    if (IsEqualGUID(riid, &IID_IUnknown)
	|| IsEqualGUID(riid, &IID_IClassFactory))
    {
	IClassFactory_AddRef(iface);
	*ppobj = This;
	return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI HTMLCF_AddRef(LPCLASSFACTORY iface) {
    ICOM_THIS(IClassFactoryImpl,iface);
    return ++(This->ref);
}

static ULONG WINAPI HTMLCF_Release(LPCLASSFACTORY iface) {
    ICOM_THIS(IClassFactoryImpl,iface);

    ULONG ref = --This->ref;

    if (ref == 0)
	HeapFree(GetProcessHeap(), 0, This);

    return ref;
}


static HRESULT WINAPI HTMLCF_CreateInstance(LPCLASSFACTORY iface, LPUNKNOWN pOuter,
					  REFIID riid, LPVOID *ppobj) {
    ICOM_THIS(IClassFactoryImpl,iface);
    HRESULT hres;
    LPUNKNOWN punk;
    
    TRACE("(%p)->(%p,%s,%p)\n",This,pOuter,debugstr_guid(riid),ppobj);

    hres = This->pfnCreateInstance(pOuter, (LPVOID *) &punk);
    if (FAILED(hres)) {
        *ppobj = NULL;
        return hres;
    }
    hres = IUnknown_QueryInterface(punk, riid, ppobj);
    if (FAILED(hres)) {
        *ppobj = NULL;
	return hres;
    }
    IUnknown_Release(punk);
    return hres;
}

static HRESULT WINAPI HTMLCF_LockServer(LPCLASSFACTORY iface,BOOL dolock) {
    ICOM_THIS(IClassFactoryImpl,iface);
    FIXME("(%p)->(%d),stub!\n",This,dolock);
    return S_OK;
}

static ICOM_VTABLE(IClassFactory) HTMLCF_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    HTMLCF_QueryInterface,
    HTMLCF_AddRef,
    HTMLCF_Release,
    HTMLCF_CreateInstance,
    HTMLCF_LockServer
};


HRESULT WINAPI MSHTML_DllGetClassObject(REFCLSID rclsid, REFIID iid, LPVOID *ppv)
{
    int i;
    IClassFactoryImpl *factory;

    TRACE("%s %s %p\n",debugstr_guid(rclsid), debugstr_guid(iid), ppv);
    
    if ( !IsEqualGUID( &IID_IClassFactory, iid )
	 && ! IsEqualGUID( &IID_IUnknown, iid) )
	return E_NOINTERFACE;

    for (i=0; i < sizeof(object_creation)/sizeof(object_creation[0]); i++)
    {
	if (IsEqualGUID(object_creation[i].clsid, rclsid))
	    break;
    }

    if (i == sizeof(object_creation)/sizeof(object_creation[0]))
    {
	FIXME("%s: no class found.\n", debugstr_guid(rclsid));
	return CLASS_E_CLASSNOTAVAILABLE;
    }

    TRACE("Creating a class factory for %s\n",object_creation[i].szClassName);

    factory = HeapAlloc(GetProcessHeap(), 0, sizeof(*factory));
    if (factory == NULL) return E_OUTOFMEMORY;

    factory->ITF_IClassFactory.lpVtbl = &HTMLCF_Vtbl;
    factory->ref = 1;

    factory->pfnCreateInstance = object_creation[i].pfnCreateInstance;

    *ppv = &(factory->ITF_IClassFactory);

    TRACE("(%p) <- %p\n", ppv, &(factory->ITF_IClassFactory) );

    return S_OK;
}

HRESULT WINAPI MSHTML_DllCanUnloadNow(void)
{
    FIXME("\n");
    return S_FALSE;
}

/* appears to have the same prototype as WinMain */
INT WINAPI RunHTMLApplication( HINSTANCE hinst, HINSTANCE hPrevInst,
                               LPCSTR szCmdLine, INT nCmdShow )
{
    FIXME("%p %p %s %d\n", hinst, hPrevInst, debugstr_a(szCmdLine), nCmdShow );
    return 0;
}
