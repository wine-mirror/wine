/*
 * Internet Security Manager
 *
 * Copyright (c) 2004 Huw D M Davies
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

#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wine/debug.h"
#include "ole2.h"
#include "wine/unicode.h"
#include "urlmon.h"
#include "urlmon_main.h"

WINE_DEFAULT_DEBUG_CHANNEL(urlmon);

typedef struct SecManagerImpl{

    IInternetSecurityManagerVtbl*  lpvtbl1;  /* VTable relative to the IInternetSecurityManager interface.*/

    ULONG ref; /* reference counter for this object */

} SecManagerImpl;

/* IUnknown prototype functions */
static HRESULT WINAPI SecManagerImpl_QueryInterface(IInternetSecurityManager* iface,REFIID riid,void** ppvObject);
static ULONG   WINAPI SecManagerImpl_AddRef(IInternetSecurityManager* iface);
static ULONG   WINAPI SecManagerImpl_Release(IInternetSecurityManager* iface);

static HRESULT WINAPI SecManagerImpl_SetSecuritySite(IInternetSecurityManager *iface,
                                                     IInternetSecurityMgrSite *pSite);
static HRESULT WINAPI SecManagerImpl_GetSecuritySite(IInternetSecurityManager *iface,
                                                     IInternetSecurityMgrSite **ppSite);
static HRESULT WINAPI SecManagerImpl_MapUrlToZone(IInternetSecurityManager *iface,
                                                  LPCWSTR pwszUrl, DWORD *pdwZone,
                                                  DWORD dwFlags);
static HRESULT WINAPI SecManagerImpl_GetSecurityId(IInternetSecurityManager *iface, 
                                                   LPCWSTR pwszUrl,
                                                   BYTE *pbSecurityId, DWORD *pcbSecurityId,
                                                   DWORD dwReserved);
static HRESULT WINAPI SecManagerImpl_ProcessUrlAction(IInternetSecurityManager *iface,
                                                      LPCWSTR pwszUrl, DWORD dwAction,
                                                      BYTE *pPolicy, DWORD cbPolicy,
                                                      BYTE *pContext, DWORD cbContext,
                                                      DWORD dwFlags, DWORD dwReserved);
static HRESULT WINAPI SecManagerImpl_QueryCustomPolicy(IInternetSecurityManager *iface,
                                                       LPCWSTR pwszUrl, REFGUID guidKey,
                                                       BYTE **ppPolicy, DWORD *pcbPolicy,
                                                       BYTE *pContext, DWORD cbContext,
                                                       DWORD dwReserved);
static HRESULT WINAPI SecManagerImpl_SetZoneMapping(IInternetSecurityManager *iface,
                                                    DWORD dwZone, LPCWSTR pwszPattern, DWORD dwFlags);
static HRESULT WINAPI SecManagerImpl_GetZoneMappings(IInternetSecurityManager *iface,
                                                     DWORD dwZone, IEnumString **ppenumString, DWORD dwFlags);

static HRESULT SecManagerImpl_Destroy(SecManagerImpl* This);
HRESULT SecManagerImpl_Construct(IUnknown *pUnkOuter, LPVOID *ppobj);

static IInternetSecurityManagerVtbl VT_SecManagerImpl =
{
    SecManagerImpl_QueryInterface,
    SecManagerImpl_AddRef,
    SecManagerImpl_Release,
    SecManagerImpl_SetSecuritySite,
    SecManagerImpl_GetSecuritySite,
    SecManagerImpl_MapUrlToZone,
    SecManagerImpl_GetSecurityId,
    SecManagerImpl_ProcessUrlAction,
    SecManagerImpl_QueryCustomPolicy,
    SecManagerImpl_SetZoneMapping,
    SecManagerImpl_GetZoneMappings
};

static HRESULT WINAPI SecManagerImpl_QueryInterface(IInternetSecurityManager* iface,REFIID riid,void** ppvObject)
{
    SecManagerImpl *This = (SecManagerImpl *)iface;

    TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(riid),ppvObject);

    /* Perform a sanity check on the parameters.*/
    if ( (This==0) || (ppvObject==0) )
	return E_INVALIDARG;

    /* Initialize the return parameter */
    *ppvObject = 0;

    /* Compare the riid with the interface IDs implemented by this object.*/
    if (IsEqualIID(&IID_IUnknown, riid) ||
        IsEqualIID(&IID_IInternetSecurityManager, riid))
        *ppvObject = iface;

    /* Check that we obtained an interface.*/
    if ((*ppvObject)==0)
        return E_NOINTERFACE;

    /* Query Interface always increases the reference count by one when it is successful */
    SecManagerImpl_AddRef(iface);

    return S_OK;
}

static ULONG WINAPI SecManagerImpl_AddRef(IInternetSecurityManager* iface)
{
    SecManagerImpl *This = (SecManagerImpl *)iface;

    TRACE("(%p)\n",This);

    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI SecManagerImpl_Release(IInternetSecurityManager* iface)
{
    SecManagerImpl *This = (SecManagerImpl *)iface;
    ULONG ref;
    TRACE("(%p)\n",This);

    ref = InterlockedDecrement(&This->ref);

    /* destroy the object if there's no more reference on it */
    if (ref==0){

        SecManagerImpl_Destroy(This);

        return 0;
    }
    return ref;
}

static HRESULT SecManagerImpl_Destroy(SecManagerImpl* This)
{
    TRACE("(%p)\n",This);

    HeapFree(GetProcessHeap(),0,This);

    return S_OK;
}

HRESULT SecManagerImpl_Construct(IUnknown *pUnkOuter, LPVOID *ppobj)
{
    SecManagerImpl *This;

    TRACE("(%p,%p)\n",pUnkOuter,ppobj);
    This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));

    /* Initialize the virtual function table. */
    This->lpvtbl1      = &VT_SecManagerImpl;
    This->ref          = 1;

    *ppobj = This;
    return S_OK;
}

static HRESULT WINAPI SecManagerImpl_SetSecuritySite(IInternetSecurityManager *iface,
                                                     IInternetSecurityMgrSite *pSite)
{
    FIXME("(%p)->(%p)\n", iface, pSite);
    return E_NOTIMPL;
}

static HRESULT WINAPI SecManagerImpl_GetSecuritySite(IInternetSecurityManager *iface,
                                                     IInternetSecurityMgrSite **ppSite)
{
    FIXME("(%p)->( %p)\n", iface, ppSite);
    return E_NOTIMPL;
}

static HRESULT WINAPI SecManagerImpl_MapUrlToZone(IInternetSecurityManager *iface,
                                                  LPCWSTR pwszUrl, DWORD *pdwZone,
                                                  DWORD dwFlags)
{
    FIXME("(%p)->(%s %p %08lx)\n", iface, debugstr_w(pwszUrl), pdwZone, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI SecManagerImpl_GetSecurityId(IInternetSecurityManager *iface, 
                                                   LPCWSTR pwszUrl,
                                                   BYTE *pbSecurityId, DWORD *pcbSecurityId,
                                                   DWORD dwReserved)
{
    FIXME("(%p)->(%s %p %p %08lx)\n", iface, debugstr_w(pwszUrl), pbSecurityId, pcbSecurityId,
          dwReserved);
    return E_NOTIMPL;
}


static HRESULT WINAPI SecManagerImpl_ProcessUrlAction(IInternetSecurityManager *iface,
                                                      LPCWSTR pwszUrl, DWORD dwAction,
                                                      BYTE *pPolicy, DWORD cbPolicy,
                                                      BYTE *pContext, DWORD cbContext,
                                                      DWORD dwFlags, DWORD dwReserved)
{
    FIXME("(%p)->(%s %08lx %p %08lx %p %08lx %08lx %08lx)\n", iface, debugstr_w(pwszUrl), dwAction,
          pPolicy, cbPolicy, pContext, cbContext, dwFlags, dwReserved);
    return E_NOTIMPL;
}
                                               

static HRESULT WINAPI SecManagerImpl_QueryCustomPolicy(IInternetSecurityManager *iface,
                                                       LPCWSTR pwszUrl, REFGUID guidKey,
                                                       BYTE **ppPolicy, DWORD *pcbPolicy,
                                                       BYTE *pContext, DWORD cbContext,
                                                       DWORD dwReserved)
{
    FIXME("(%p)->(%s %s %p %p %p %08lx %08lx )\n", iface, debugstr_w(pwszUrl), debugstr_guid(guidKey),
          ppPolicy, pcbPolicy, pContext, cbContext, dwReserved);
    return E_NOTIMPL;
}

static HRESULT WINAPI SecManagerImpl_SetZoneMapping(IInternetSecurityManager *iface,
                                                    DWORD dwZone, LPCWSTR pwszPattern, DWORD dwFlags)
{
    FIXME("(%p)->(%08lx %s %08lx)\n", iface, dwZone, debugstr_w(pwszPattern),dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI SecManagerImpl_GetZoneMappings(IInternetSecurityManager *iface,
                                                     DWORD dwZone, IEnumString **ppenumString, DWORD dwFlags)
{
    FIXME("(%p)->(%08lx %p %08lx)\n", iface, dwZone, ppenumString,dwFlags);
    return E_NOTIMPL;
}

/***********************************************************************
 *           CoInternetCreateSecurityManager (URLMON.@)
 *
 */
HRESULT WINAPI CoInternetCreateSecurityManager( IServiceProvider *pSP,
    IInternetSecurityManager **ppSM, DWORD dwReserved )
{
    TRACE("%p %p %ld\n", pSP, ppSM, dwReserved );
    return SecManagerImpl_Construct(NULL, (void**) ppSM);
}
