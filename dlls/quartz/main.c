/*              DirectShow Base Functions (QUARTZ.DLL)
 *
 * Copyright 2002 Lionel Ulmer
 *
 * This file contains the (internal) driver registration functions,
 * driver enumeration APIs and DirectDraw creation functions.
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
#include "wine/debug.h"

#include "quartz_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

static DWORD dll_ref = 0;

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
 * DirectShow ClassFactory
 */
typedef struct {
    IClassFactory ITF_IClassFactory;

    DWORD ref;
    HRESULT (*pfnCreateInstance)(IUnknown *pUnkOuter, LPVOID *ppObj);
} IClassFactoryImpl;

struct object_creation_info
{
    const CLSID *clsid;
    HRESULT (*pfnCreateInstance)(IUnknown *pUnkOuter, LPVOID *ppObj);
};

static const struct object_creation_info object_creation[] =
{
    { &CLSID_FilterGraph, FILTERGRAPH_create },
    { &CLSID_FilterMapper, FilterMapper2_create },
    { &CLSID_FilterMapper2, FilterMapper2_create },
    { &CLSID_AsyncReader, AsyncReader_create },
    { &CLSID_MemoryAllocator, StdMemAllocator_create },
    { &CLSID_AviSplitter, AVISplitter_create },
};

static HRESULT WINAPI
DSCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj)
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

static ULONG WINAPI DSCF_AddRef(LPCLASSFACTORY iface) {
    ICOM_THIS(IClassFactoryImpl,iface);
    return ++(This->ref);
}

static ULONG WINAPI DSCF_Release(LPCLASSFACTORY iface) {
    ICOM_THIS(IClassFactoryImpl,iface);

    ULONG ref = --This->ref;

    if (ref == 0)
	HeapFree(GetProcessHeap(), 0, This);

    return ref;
}


static HRESULT WINAPI DSCF_CreateInstance(LPCLASSFACTORY iface, LPUNKNOWN pOuter,
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

static HRESULT WINAPI DSCF_LockServer(LPCLASSFACTORY iface,BOOL dolock) {
    ICOM_THIS(IClassFactoryImpl,iface);
    FIXME("(%p)->(%d),stub!\n",This,dolock);
    return S_OK;
}

static ICOM_VTABLE(IClassFactory) DSCF_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    DSCF_QueryInterface,
    DSCF_AddRef,
    DSCF_Release,
    DSCF_CreateInstance,
    DSCF_LockServer
};

/*******************************************************************************
 * DllGetClassObject [QUARTZ.@]
 * Retrieves class object from a DLL object
 *
 * NOTES
 *    Docs say returns STDAPI
 *
 * PARAMS
 *    rclsid [I] CLSID for the class object
 *    riid   [I] Reference to identifier of interface for class object
 *    ppv    [O] Address of variable to receive interface pointer for riid
 *
 * RETURNS
 *    Success: S_OK
 *    Failure: CLASS_E_CLASSNOTAVAILABLE, E_OUTOFMEMORY, E_INVALIDARG,
 *             E_UNEXPECTED
 */
DWORD WINAPI QUARTZ_DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    int i;
    IClassFactoryImpl *factory;
    
    TRACE("(%s,%s,%p)\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    
    if ( !IsEqualGUID( &IID_IClassFactory, riid )
	 && ! IsEqualGUID( &IID_IUnknown, riid) )
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

    factory = HeapAlloc(GetProcessHeap(), 0, sizeof(*factory));
    if (factory == NULL) return E_OUTOFMEMORY;

    factory->ITF_IClassFactory.lpVtbl = &DSCF_Vtbl;
    factory->ref = 1;

    factory->pfnCreateInstance = object_creation[i].pfnCreateInstance;

    *ppv = &(factory->ITF_IClassFactory);
    return S_OK;
}

/***********************************************************************
 *              DllCanUnloadNow (QUARTZ.@)
 */
HRESULT WINAPI QUARTZ_DllCanUnloadNow()
{
    return dll_ref != 0 ? S_FALSE : S_OK;
}


static struct {
	const GUID	riid;
	const char 	*name;
} InterfaceDesc[] =
{
    #define OUR_GUID_ENTRY(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    { { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } } , #name },
        #include <uuids.h>
	{ { 0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0} }, NULL }
};

/***********************************************************************
 *              qzdebugstr_guid (internal)
 *
 * Gives a text version of DirectShow GUIDs
 */
const char * qzdebugstr_guid( const GUID * id )
{
    int i;
    char * name = NULL;

    for (i=0;InterfaceDesc[i].name && !name;i++) {
        if (IsEqualGUID(&InterfaceDesc[i].riid, id)) return InterfaceDesc[i].name;
    }
    return debugstr_guid(id);
}

/***********************************************************************
 *              qzdebugstr_State (internal)
 *
 * Gives a text version of the FILTER_STATE enumeration
 */
const char * qzdebugstr_State(FILTER_STATE state)
{
    switch (state)
    {
    case State_Stopped:
        return "State_Stopped";
    case State_Running:
        return "State_Running";
    case State_Paused:
        return "State_Paused";
    default:
        return "State_Unknown";
    }
}
