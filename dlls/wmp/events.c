/*
 * Implementation of event-related interfaces for WMP control:
 *
 *  - IConnectionPointContainer
 *  - IConnectionPoint
 *
 * Copyright 2001 John R. Sheets (for CodeWeavers)
 * Copyright 2006 Jacek Caban for CodeWeavers
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

#include "wmp_private.h"
#include "olectl.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wmp);

static inline WindowsMediaPlayer *impl_from_IConnectionPointContainer(IConnectionPointContainer *iface)
{
    return CONTAINING_RECORD(iface, WindowsMediaPlayer, IConnectionPointContainer_iface);
}

static HRESULT WINAPI ConnectionPointContainer_QueryInterface(IConnectionPointContainer *iface,
        REFIID riid, LPVOID *ppv)
{
    WindowsMediaPlayer *This = impl_from_IConnectionPointContainer(iface);
    return IOleObject_QueryInterface(&This->IOleObject_iface, riid, ppv);
}

static ULONG WINAPI ConnectionPointContainer_AddRef(IConnectionPointContainer *iface)
{
    WindowsMediaPlayer *This = impl_from_IConnectionPointContainer(iface);
    return IOleObject_AddRef(&This->IOleObject_iface);
}

static ULONG WINAPI ConnectionPointContainer_Release(IConnectionPointContainer *iface)
{
    WindowsMediaPlayer *This = impl_from_IConnectionPointContainer(iface);
    return IOleObject_Release(&This->IOleObject_iface);
}

static HRESULT WINAPI ConnectionPointContainer_EnumConnectionPoints(IConnectionPointContainer *iface,
        IEnumConnectionPoints **ppEnum)
{
    WindowsMediaPlayer *This = impl_from_IConnectionPointContainer(iface);
    FIXME("(%p)->(%p)\n", This, ppEnum);
    return E_NOTIMPL;
}

static HRESULT WINAPI ConnectionPointContainer_FindConnectionPoint(IConnectionPointContainer *iface,
        REFIID riid, IConnectionPoint **ppCP){
    WindowsMediaPlayer *This = impl_from_IConnectionPointContainer(iface);

    if(!ppCP) {
        WARN("ppCP == NULL\n");
        return E_POINTER;
    }

    *ppCP = NULL;

    if(IsEqualGUID(&IID__WMPOCXEvents, riid)) {
        TRACE("(%p)->(IID__WMPOCXEvents %p)\n", This, ppCP);
        *ppCP = &This->wmpocx->IConnectionPoint_iface;
    }

    if(*ppCP) {
        IConnectionPoint_AddRef(*ppCP);
        return S_OK;
    }

    WARN("Unsupported IID %s\n", debugstr_guid(riid));
    return CONNECT_E_NOCONNECTION;
}

static const IConnectionPointContainerVtbl ConnectionPointContainerVtbl =
{
    ConnectionPointContainer_QueryInterface,
    ConnectionPointContainer_AddRef,
    ConnectionPointContainer_Release,
    ConnectionPointContainer_EnumConnectionPoints,
    ConnectionPointContainer_FindConnectionPoint
};

static inline ConnectionPoint *impl_from_IConnectionPoint(IConnectionPoint *iface)
{
    return CONTAINING_RECORD(iface, ConnectionPoint, IConnectionPoint_iface);
}

typedef struct {
    IEnumConnections IEnumConnections_iface;

    LONG ref;

    ConnectionPoint *cp;
    DWORD iter;
} EnumConnections;

static inline EnumConnections *impl_from_IEnumConnections(IEnumConnections *iface)
{
    return CONTAINING_RECORD(iface, EnumConnections, IEnumConnections_iface);
}

static HRESULT WINAPI EnumConnections_QueryInterface(IEnumConnections *iface, REFIID riid, void **ppv)
{
    EnumConnections *This = impl_from_IEnumConnections(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IEnumConnections_iface;
    }else if(IsEqualGUID(&IID_IEnumConnections, riid)) {
        TRACE("(%p)->(IID_IEnumConnections %p)\n", This, ppv);
        *ppv = &This->IEnumConnections_iface;
    }else {
        WARN("Unsupported interface %s\n", debugstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI EnumConnections_AddRef(IEnumConnections *iface)
{
    EnumConnections *This = impl_from_IEnumConnections(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI EnumConnections_Release(IEnumConnections *iface)
{
    EnumConnections *This = impl_from_IEnumConnections(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref) {
        IConnectionPoint_Release(&This->cp->IConnectionPoint_iface);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI EnumConnections_Next(IEnumConnections *iface, ULONG cConnections, CONNECTDATA *pgcd, ULONG *pcFetched)
{
    EnumConnections *This = impl_from_IEnumConnections(iface);
    ULONG cnt = 0;

    TRACE("(%p)->(%lu %p %p)\n", This, cConnections, pgcd, pcFetched);

    while(cConnections--) {
        while(This->iter < This->cp->sinks_size && !This->cp->sinks[This->iter])
            This->iter++;
        if(This->iter == This->cp->sinks_size)
            break;

        pgcd[cnt].pUnk = (IUnknown*)This->cp->sinks[This->iter];
        pgcd[cnt].dwCookie = cnt+1;
        This->iter++;
        cnt++;
        IUnknown_AddRef(pgcd[cnt].pUnk);
    }

    if(pcFetched)
        *pcFetched = cnt;
    return cnt ? S_OK : S_FALSE;
}

static HRESULT WINAPI EnumConnections_Skip(IEnumConnections *iface, ULONG cConnections)
{
    EnumConnections *This = impl_from_IEnumConnections(iface);
    FIXME("(%p)->(%lu)\n", This, cConnections);
    return E_NOTIMPL;
}

static HRESULT WINAPI EnumConnections_Reset(IEnumConnections *iface)
{
    EnumConnections *This = impl_from_IEnumConnections(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI EnumConnections_Clone(IEnumConnections *iface, IEnumConnections **ppEnum)
{
    EnumConnections *This = impl_from_IEnumConnections(iface);
    FIXME("(%p)->(%p)\n", This, ppEnum);
    return E_NOTIMPL;
}

static const IEnumConnectionsVtbl EnumConnectionsVtbl = {
    EnumConnections_QueryInterface,
    EnumConnections_AddRef,
    EnumConnections_Release,
    EnumConnections_Next,
    EnumConnections_Skip,
    EnumConnections_Reset,
    EnumConnections_Clone
};

static HRESULT WINAPI ConnectionPoint_QueryInterface(IConnectionPoint *iface,
                                                     REFIID riid, LPVOID *ppv)
{
    ConnectionPoint *This = impl_from_IConnectionPoint(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IConnectionPoint_iface;
    }else if(IsEqualGUID(&IID_IConnectionPoint, riid)) {
        TRACE("(%p)->(IID_IConnectionPoint %p)\n", This, ppv);
        *ppv = &This->IConnectionPoint_iface;
    }

    if(*ppv) {
        IConnectionPointContainer_AddRef(This->container);
        return S_OK;
    }

    WARN("Unsupported interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI ConnectionPoint_AddRef(IConnectionPoint *iface)
{
    ConnectionPoint *This = impl_from_IConnectionPoint(iface);
    return IConnectionPointContainer_AddRef(This->container);
}

static ULONG WINAPI ConnectionPoint_Release(IConnectionPoint *iface)
{
    ConnectionPoint *This = impl_from_IConnectionPoint(iface);
    return IConnectionPointContainer_Release(This->container);
}

static HRESULT WINAPI ConnectionPoint_GetConnectionInterface(IConnectionPoint *iface, IID *pIID)
{
    ConnectionPoint *This = impl_from_IConnectionPoint(iface);

    TRACE("(%p)->(%p)\n", This, pIID);

    *pIID = This->iid;
    return S_OK;
}

static HRESULT WINAPI ConnectionPoint_GetConnectionPointContainer(IConnectionPoint *iface,
        IConnectionPointContainer **ppCPC)
{
    ConnectionPoint *This = impl_from_IConnectionPoint(iface);

    TRACE("(%p)->(%p)\n", This, ppCPC);

    *ppCPC = This->container;
    IConnectionPointContainer_AddRef(This->container);
    return S_OK;
}

static HRESULT WINAPI ConnectionPoint_Advise(IConnectionPoint *iface, IUnknown *pUnkSink,
                                             DWORD *pdwCookie)
{
    ConnectionPoint *This = impl_from_IConnectionPoint(iface);
    IDispatch *disp;
    DWORD i;
    HRESULT hres;

    TRACE("(%p)->(%p %p)\n", This, pUnkSink, pdwCookie);

    hres = IUnknown_QueryInterface(pUnkSink, &This->iid, (void**)&disp);
    if(FAILED(hres)) {
        hres = IUnknown_QueryInterface(pUnkSink, &IID_IDispatch, (void**)&disp);
        if(FAILED(hres))
            return CONNECT_E_CANNOTCONNECT;
    }

    if(This->sinks) {
        for(i=0; i<This->sinks_size; i++) {
            if(!This->sinks[i])
                break;
        }

        if(i == This->sinks_size)
            This->sinks = realloc(This->sinks, (++This->sinks_size)*sizeof(*This->sinks));
    }else {
        This->sinks = malloc(sizeof(*This->sinks));
        This->sinks_size = 1;
        i = 0;
    }

    This->sinks[i] = disp;
    *pdwCookie = i+1;

    return S_OK;
}

static HRESULT WINAPI ConnectionPoint_Unadvise(IConnectionPoint *iface, DWORD dwCookie)
{
    ConnectionPoint *This = impl_from_IConnectionPoint(iface);

    TRACE("(%p)->(%ld)\n", This, dwCookie);

    if(!dwCookie || dwCookie > This->sinks_size || !This->sinks[dwCookie-1])
        return CONNECT_E_NOCONNECTION;

    IDispatch_Release(This->sinks[dwCookie-1]);
    This->sinks[dwCookie-1] = NULL;

    return S_OK;
}

static HRESULT WINAPI ConnectionPoint_EnumConnections(IConnectionPoint *iface,
                                                      IEnumConnections **ppEnum)
{
    ConnectionPoint *This = impl_from_IConnectionPoint(iface);
    EnumConnections *ret;

    TRACE("(%p)->(%p)\n", This, ppEnum);

    ret = malloc(sizeof(*ret));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IEnumConnections_iface.lpVtbl = &EnumConnectionsVtbl;
    ret->ref = 1;
    ret->iter = 0;

    IConnectionPoint_AddRef(&This->IConnectionPoint_iface);
    ret->cp = This;

    *ppEnum = &ret->IEnumConnections_iface;
    return S_OK;
}


static const IConnectionPointVtbl ConnectionPointVtbl =
{
    ConnectionPoint_QueryInterface,
    ConnectionPoint_AddRef,
    ConnectionPoint_Release,
    ConnectionPoint_GetConnectionInterface,
    ConnectionPoint_GetConnectionPointContainer,
    ConnectionPoint_Advise,
    ConnectionPoint_Unadvise,
    ConnectionPoint_EnumConnections
};

static void ConnectionPoint_Destroy(ConnectionPoint *This)
{
    DWORD i;

    for(i=0; i<This->sinks_size; i++) {
        if(This->sinks[i])
            IDispatch_Release(This->sinks[i]);
    }

    free(This->sinks);
    free(This);
}

static void ConnectionPoint_Create(REFIID riid, ConnectionPoint **cp,
                                   IConnectionPointContainer *container)
{
    ConnectionPoint *ret = malloc(sizeof(ConnectionPoint));

    ret->IConnectionPoint_iface.lpVtbl = &ConnectionPointVtbl;

    ret->sinks = NULL;
    ret->sinks_size = 0;
    ret->container = container;

    ret->iid = *riid;

    *cp = ret;
}

void ConnectionPointContainer_Init(WindowsMediaPlayer *wmp)
{
    wmp->IConnectionPointContainer_iface.lpVtbl = &ConnectionPointContainerVtbl;
    ConnectionPoint_Create(&IID__WMPOCXEvents, &wmp->wmpocx, &wmp->IConnectionPointContainer_iface);
}

void ConnectionPointContainer_Destroy(WindowsMediaPlayer *wmp)
{
    ConnectionPoint_Destroy(wmp->wmpocx);
}

void call_sink(ConnectionPoint *This, DISPID dispid, DISPPARAMS *dispparams)
{
    DWORD i;

    for(i=0; i<This->sinks_size; i++) {
        if(This->sinks[i])
            IDispatch_Invoke(This->sinks[i], dispid, &IID_NULL, LOCALE_SYSTEM_DEFAULT,
                    DISPATCH_METHOD, dispparams, NULL, NULL, NULL);
    }
}
