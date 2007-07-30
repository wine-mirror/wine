/* 
 * DirectPlay
 * 
 * Copyright 2004 Raphael Junqueira
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
 *
 */

#include "config.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "objbase.h"
#include "wine/debug.h"

#include "dplay8.h"
#include "dplobby8.h"
 /*
 *#include "dplay8sp.h"
 */
#include "dpnet_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dpnet);

/* At process attach */
BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
  TRACE("%p,%x,%p\n", hInstDLL, fdwReason, lpvReserved);
  if (fdwReason == DLL_PROCESS_ATTACH) {
    DisableThreadLibraryCalls(hInstDLL);    
  }
  return TRUE;
}

/***********************************************************************
 *             DirectPlay8Create (DPNET.@)
 */
HRESULT WINAPI DirectPlay8Create(REFGUID lpGUID, LPVOID *ppvInt, LPUNKNOWN punkOuter)
{
    TRACE("(%s, %p, %p): stub\n", debugstr_guid(lpGUID), ppvInt, punkOuter);
    return S_OK;
}

/*******************************************************************************
 * DirectPlay ClassFactory
 */
typedef struct
{
  /* IUnknown fields */
  const IClassFactoryVtbl *lpVtbl;
  LONG       ref; 
  REFCLSID   rclsid;
  HRESULT   (*pfnCreateInstanceFactory)(LPCLASSFACTORY iface, LPUNKNOWN punkOuter, REFIID riid, LPVOID *ppobj);
} IClassFactoryImpl;

static HRESULT WINAPI DICF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj) {
  IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
  
  FIXME("(%p)->(%s,%p),stub!\n",This,debugstr_guid(riid),ppobj);
  return E_NOINTERFACE;
}

static ULONG WINAPI DICF_AddRef(LPCLASSFACTORY iface) {
  IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
  return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI DICF_Release(LPCLASSFACTORY iface) {
  IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
  /* static class, won't be  freed */
  return InterlockedDecrement(&This->ref);
}

static HRESULT WINAPI DICF_CreateInstance(LPCLASSFACTORY iface,LPUNKNOWN pOuter,REFIID riid,LPVOID *ppobj) {
  IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
  
  TRACE("(%p)->(%p,%s,%p)\n",This,pOuter,debugstr_guid(riid),ppobj);
  return This->pfnCreateInstanceFactory(iface, pOuter, riid, ppobj);
}

static HRESULT WINAPI DICF_LockServer(LPCLASSFACTORY iface,BOOL dolock) {
  IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
  FIXME("(%p)->(%d),stub!\n",This,dolock);
  return S_OK;
}

static const IClassFactoryVtbl DICF_Vtbl = {
  DICF_QueryInterface,
  DICF_AddRef,
  DICF_Release,
  DICF_CreateInstance,
  DICF_LockServer
};

static IClassFactoryImpl DPNET_CFS[] = {
  { &DICF_Vtbl, 1, &CLSID_DirectPlay8Client,  DPNET_CreateDirectPlay8Client },
  { &DICF_Vtbl, 1, &CLSID_DirectPlay8Server,  DPNET_CreateDirectPlay8Server },
  { &DICF_Vtbl, 1, &CLSID_DirectPlay8Peer,    DPNET_CreateDirectPlay8Peer },
  { &DICF_Vtbl, 1, &CLSID_DirectPlay8Address, DPNET_CreateDirectPlay8Address },
  { &DICF_Vtbl, 1, &CLSID_DirectPlay8LobbiedApplication, (void *)DPNET_CreateDirectPlay8LobbiedApp },
  { NULL, 0, NULL, NULL }
};

/***********************************************************************
 *             DllCanUnloadNow (DPNET.@)
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    return S_FALSE;
}

/***********************************************************************
 *		DllGetClassObject (DPNET.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    int i = 0;

    TRACE("(%p,%p,%p)\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    /*
    if ( IsEqualCLSID( &IID_IClassFactory, riid ) ) {
    	*ppv = (LPVOID)&DPNET_CF;
	IClassFactory_AddRef((IClassFactory*)*ppv);
	return S_OK;
    }
    */
    while (NULL != DPNET_CFS[i].rclsid) {
      if (IsEqualGUID(rclsid, DPNET_CFS[i].rclsid)) {
	DICF_AddRef((IClassFactory*) &DPNET_CFS[i]);
	*ppv = &DPNET_CFS[i];
	return S_OK;
      }
      ++i;
    }

    FIXME("(%p,%p,%p): no interface found.\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}
