/*
 *    ITSS Class Factory
 *
 * Copyright 2002 Lionel Ulmer
 * Copyright 2004 Mike McCormack
 *
 *  see http://bonedaddy.net/pabs3/hhm/#chmspec
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

#include "config.h"

#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winnls.h"
#include "winreg.h"
#include "ole2.h"

#include "uuids.h"

#include "wine/unicode.h"
#include "wine/debug.h"

#include "itsstor.h"

#include "initguid.h"
#include "wine/itss.h"

WINE_DEFAULT_DEBUG_CHANNEL(itss);

static HRESULT ITSS_create(IUnknown *pUnkOuter, LPVOID *ppObj);

LONG dll_count = 0;

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
 * ITSS ClassFactory
 */
typedef struct {
    IClassFactory ITF_IClassFactory;

    LONG ref;
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
    { &CLSID_ITStorage, "ITStorage", ITSS_create },
    { &CLSID_ITSProtocol, "ITSProtocol", ITS_IParseDisplayName_create },
};

static HRESULT WINAPI
ITSSCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;

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

static ULONG WINAPI ITSSCF_AddRef(LPCLASSFACTORY iface)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI ITSSCF_Release(LPCLASSFACTORY iface)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;

    ULONG ref = InterlockedDecrement(&This->ref);

    if (ref == 0) {
        HeapFree(GetProcessHeap(), 0, This);
        InterlockedDecrement(&dll_count);
    }

    return ref;
}


static HRESULT WINAPI ITSSCF_CreateInstance(LPCLASSFACTORY iface, LPUNKNOWN pOuter,
					  REFIID riid, LPVOID *ppobj)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
    HRESULT hres;
    LPUNKNOWN punk;
    
    TRACE("(%p)->(%p,%s,%p)\n",This,pOuter,debugstr_guid(riid),ppobj);

    *ppobj = NULL;
    hres = This->pfnCreateInstance(pOuter, (LPVOID *) &punk);
    if (SUCCEEDED(hres)) {
        hres = IUnknown_QueryInterface(punk, riid, ppobj);
        IUnknown_Release(punk);
    }
    return hres;
}

static HRESULT WINAPI ITSSCF_LockServer(LPCLASSFACTORY iface, BOOL dolock)
{
    TRACE("(%p)->(%d)\n", iface, dolock);

    if(dolock)
        InterlockedIncrement(&dll_count);
    else
        InterlockedDecrement(&dll_count);

    return S_OK;
}

static const IClassFactoryVtbl ITSSCF_Vtbl =
{
    ITSSCF_QueryInterface,
    ITSSCF_AddRef,
    ITSSCF_Release,
    ITSSCF_CreateInstance,
    ITSSCF_LockServer
};


/***********************************************************************
 *		DllGetClassObject	(ITSS.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID iid, LPVOID *ppv)
{
    DWORD i;
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

    factory->ITF_IClassFactory.lpVtbl = &ITSSCF_Vtbl;
    factory->ref = 1;

    factory->pfnCreateInstance = object_creation[i].pfnCreateInstance;

    *ppv = &(factory->ITF_IClassFactory);
    InterlockedIncrement(&dll_count);

    TRACE("(%p) <- %p\n", ppv, &(factory->ITF_IClassFactory) );

    return S_OK;
}

/*****************************************************************************/

typedef struct {
    const IITStorageVtbl *vtbl_IITStorage;
    LONG ref;
} ITStorageImpl;


static HRESULT WINAPI ITStorageImpl_QueryInterface(
    IITStorage* iface,
    REFIID riid,
    void** ppvObject)
{
    ITStorageImpl *This = (ITStorageImpl *)iface;
    if (IsEqualGUID(riid, &IID_IUnknown)
	|| IsEqualGUID(riid, &IID_IITStorage))
    {
	IClassFactory_AddRef(iface);
	*ppvObject = This;
	return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppvObject);
    return E_NOINTERFACE;
}

static ULONG WINAPI ITStorageImpl_AddRef(
    IITStorage* iface)
{
    ITStorageImpl *This = (ITStorageImpl *)iface;
    TRACE("%p\n", This);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI ITStorageImpl_Release(
    IITStorage* iface)
{
    ITStorageImpl *This = (ITStorageImpl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    if (ref == 0) {
        HeapFree(GetProcessHeap(), 0, This);
        InterlockedDecrement(&dll_count);
    }

    return ref;
}

static HRESULT WINAPI ITStorageImpl_StgCreateDocfile(
    IITStorage* iface,
    const WCHAR* pwcsName,
    DWORD grfMode,
    DWORD reserved,
    IStorage** ppstgOpen)
{
    ITStorageImpl *This = (ITStorageImpl *)iface;

    TRACE("%p %s %lu %lu %p\n", This,
          debugstr_w(pwcsName), grfMode, reserved, ppstgOpen );

    return ITSS_StgOpenStorage( pwcsName, NULL, grfMode,
                                0, reserved, ppstgOpen);
}

static HRESULT WINAPI ITStorageImpl_StgCreateDocfileOnILockBytes(
    IITStorage* iface,
    ILockBytes* plkbyt,
    DWORD grfMode,
    DWORD reserved,
    IStorage** ppstgOpen)
{
    ITStorageImpl *This = (ITStorageImpl *)iface;
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ITStorageImpl_StgIsStorageFile(
    IITStorage* iface,
    const WCHAR* pwcsName)
{
    ITStorageImpl *This = (ITStorageImpl *)iface;
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ITStorageImpl_StgIsStorageILockBytes(
    IITStorage* iface,
    ILockBytes* plkbyt)
{
    ITStorageImpl *This = (ITStorageImpl *)iface;
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ITStorageImpl_StgOpenStorage(
    IITStorage* iface,
    const WCHAR* pwcsName,
    IStorage* pstgPriority,
    DWORD grfMode,
    SNB snbExclude,
    DWORD reserved,
    IStorage** ppstgOpen)
{
    ITStorageImpl *This = (ITStorageImpl *)iface;

    TRACE("%p %s %p %ld %p\n", This, debugstr_w( pwcsName ),
           pstgPriority, grfMode, snbExclude );

    return ITSS_StgOpenStorage( pwcsName, pstgPriority, grfMode,
                                snbExclude, reserved, ppstgOpen);
}

static HRESULT WINAPI ITStorageImpl_StgOpenStorageOnILockBytes(
    IITStorage* iface,
    ILockBytes* plkbyt,
    IStorage* pStgPriority,
    DWORD grfMode,
    SNB snbExclude,
    DWORD reserved,
    IStorage** ppstgOpen)
{
    ITStorageImpl *This = (ITStorageImpl *)iface;
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ITStorageImpl_StgSetTimes(
    IITStorage* iface,
    WCHAR* lpszName,
    FILETIME* pctime,
    FILETIME* patime,
    FILETIME* pmtime)
{
    ITStorageImpl *This = (ITStorageImpl *)iface;
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ITStorageImpl_SetControlData(
    IITStorage* iface,
    PITS_Control_Data pControlData)
{
    ITStorageImpl *This = (ITStorageImpl *)iface;
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ITStorageImpl_DefaultControlData(
    IITStorage* iface,
    PITS_Control_Data* ppControlData)
{
    ITStorageImpl *This = (ITStorageImpl *)iface;
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ITStorageImpl_Compact(
    IITStorage* iface,
    const WCHAR* pwcsName,
    ECompactionLev iLev)
{
    ITStorageImpl *This = (ITStorageImpl *)iface;
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static const IITStorageVtbl ITStorageImpl_Vtbl =
{
    ITStorageImpl_QueryInterface,
    ITStorageImpl_AddRef,
    ITStorageImpl_Release,
    ITStorageImpl_StgCreateDocfile,
    ITStorageImpl_StgCreateDocfileOnILockBytes,
    ITStorageImpl_StgIsStorageFile,
    ITStorageImpl_StgIsStorageILockBytes,
    ITStorageImpl_StgOpenStorage,
    ITStorageImpl_StgOpenStorageOnILockBytes,
    ITStorageImpl_StgSetTimes,
    ITStorageImpl_SetControlData,
    ITStorageImpl_DefaultControlData,
    ITStorageImpl_Compact,
};

static HRESULT ITSS_create(IUnknown *pUnkOuter, LPVOID *ppObj)
{
    ITStorageImpl *its;

    if( pUnkOuter )
        return CLASS_E_NOAGGREGATION;

    its = HeapAlloc( GetProcessHeap(), 0, sizeof(ITStorageImpl) );
    its->vtbl_IITStorage = &ITStorageImpl_Vtbl;
    its->ref = 1;

    TRACE("-> %p\n", its);
    *ppObj = (LPVOID) its;
    InterlockedIncrement(&dll_count);

    return S_OK;
}

/*****************************************************************************/

HRESULT WINAPI DllCanUnloadNow(void)
{
    TRACE("dll_count = %lu\n", dll_count);
    return dll_count ? S_FALSE : S_OK;
}
