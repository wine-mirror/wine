/*
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

#include "config.h"

#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"

#include "wine/debug.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

#define CONPOINT(x) ((IConnectionPoint*) &(x)->lpConnectionPointVtbl);

void call_property_onchanged(ConnectionPoint *This, DISPID dispid)
{
    DWORD i;

    for(i=0; i<This->sinks_size; i++) {
        if(This->sinks[i].propnotif)
            IPropertyNotifySink_OnChanged(This->sinks[i].propnotif, dispid);
    }
}

#define CONPOINT_THIS(iface) DEFINE_THIS(ConnectionPoint, ConnectionPoint, iface)

static HRESULT WINAPI ConnectionPoint_QueryInterface(IConnectionPoint *iface,
                                                     REFIID riid, LPVOID *ppv)
{
    ConnectionPoint *This = CONPOINT_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = CONPOINT(This);
    }else if(IsEqualGUID(&IID_IConnectionPoint, riid)) {
        TRACE("(%p)->(IID_IConnectionPoint %p)\n", This, ppv);
        *ppv = CONPOINT(This);
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    WARN("Unsupported interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI ConnectionPoint_AddRef(IConnectionPoint *iface)
{
    ConnectionPoint *This = CONPOINT_THIS(iface);
    return IHTMLDocument2_AddRef(HTMLDOC(This->doc));
}

static ULONG WINAPI ConnectionPoint_Release(IConnectionPoint *iface)
{
    ConnectionPoint *This = CONPOINT_THIS(iface);
    return IHTMLDocument2_Release(HTMLDOC(This->doc));
}

static HRESULT WINAPI ConnectionPoint_GetConnectionInterface(IConnectionPoint *iface, IID *pIID)
{
    ConnectionPoint *This = CONPOINT_THIS(iface);

    TRACE("(%p)->(%p)\n", This, pIID);

    if(!pIID)
        return E_POINTER;

    memcpy(pIID, &This->iid, sizeof(IID));
    return S_OK;
}

static HRESULT WINAPI ConnectionPoint_GetConnectionPointContainer(IConnectionPoint *iface,
        IConnectionPointContainer **ppCPC)
{
    ConnectionPoint *This = CONPOINT_THIS(iface);

    TRACE("(%p)->(%p)\n", This, ppCPC);

    if(!ppCPC)
        return E_POINTER;

    *ppCPC = CONPTCONT(This->doc);
    IConnectionPointContainer_AddRef(*ppCPC);
    return S_OK;
}

static HRESULT WINAPI ConnectionPoint_Advise(IConnectionPoint *iface, IUnknown *pUnkSink,
                                             DWORD *pdwCookie)
{
    ConnectionPoint *This = CONPOINT_THIS(iface);
    IUnknown *sink;
    DWORD i;
    HRESULT hres;

    TRACE("(%p)->(%p %p)\n", This, pUnkSink, pdwCookie);

    hres = IUnknown_QueryInterface(pUnkSink, &This->iid, (void**)&sink);
    if(FAILED(hres) && !IsEqualGUID(&IID_IPropertyNotifySink, &This->iid))
        hres = IUnknown_QueryInterface(pUnkSink, &IID_IDispatch, (void**)&sink);
    if(FAILED(hres))
        return CONNECT_E_CANNOTCONNECT;

    if(This->sinks) {
        for(i=0; i<This->sinks_size; i++) {
            if(!This->sinks[i].unk)
                break;
        }

        if(i == This->sinks_size)
            This->sinks = mshtml_realloc(This->sinks,(++This->sinks_size)*sizeof(*This->sinks));
    }else {
        This->sinks = mshtml_alloc(sizeof(*This->sinks));
        This->sinks_size = 1;
        i = 0;
    }

    This->sinks[i].unk = sink;
    *pdwCookie = i+1;

    return S_OK;
}

static HRESULT WINAPI ConnectionPoint_Unadvise(IConnectionPoint *iface, DWORD dwCookie)
{
    ConnectionPoint *This = CONPOINT_THIS(iface);
    TRACE("(%p)->(%d)\n", This, dwCookie);

    if(!dwCookie || dwCookie > This->sinks_size || !This->sinks[dwCookie-1].unk)
        return CONNECT_E_NOCONNECTION;

    IUnknown_Release(This->sinks[dwCookie-1].unk);
    This->sinks[dwCookie-1].unk = NULL;

    return S_OK;
}

static HRESULT WINAPI ConnectionPoint_EnumConnections(IConnectionPoint *iface,
                                                      IEnumConnections **ppEnum)
{
    ConnectionPoint *This = CONPOINT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, ppEnum);
    return E_NOTIMPL;
}

#undef CONPOINT_THIS

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

static void ConnectionPoint_Init(HTMLDocument *doc, REFIID riid, ConnectionPoint *cp)
{
    cp->lpConnectionPointVtbl = &ConnectionPointVtbl;
    cp->doc = doc;
    cp->sinks = NULL;
    cp->sinks_size = 0;
    cp->iid = *riid;
}

static void ConnectionPoint_Destroy(ConnectionPoint *This)
{
    int i;

    for(i=0; i<This->sinks_size; i++) {
        if(This->sinks[i].unk)
            IUnknown_Release(This->sinks[i].unk);
    }

    mshtml_free(This->sinks);
}

#define CONPTCONT_THIS(iface) DEFINE_THIS(HTMLDocument, ConnectionPointContainer, iface)

static HRESULT WINAPI ConnectionPointContainer_QueryInterface(IConnectionPointContainer *iface,
                                                              REFIID riid, void **ppv)
{
    HTMLDocument *This = CONPTCONT_THIS(iface);
    return IHTMLDocument2_QueryInterface(HTMLDOC(This), riid, ppv);
}

static ULONG WINAPI ConnectionPointContainer_AddRef(IConnectionPointContainer *iface)
{
    HTMLDocument *This = CONPTCONT_THIS(iface);
    return IHTMLDocument2_AddRef(HTMLDOC(This));
}

static ULONG WINAPI ConnectionPointContainer_Release(IConnectionPointContainer *iface)
{
    HTMLDocument *This = CONPTCONT_THIS(iface);
    return IHTMLDocument2_Release(HTMLDOC(This));
}

static HRESULT WINAPI ConnectionPointContainer_EnumConnectionPoints(IConnectionPointContainer *iface,
        IEnumConnectionPoints **ppEnum)
{
    HTMLDocument *This = CONPTCONT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, ppEnum);
    return E_NOTIMPL;
}

static HRESULT WINAPI ConnectionPointContainer_FindConnectionPoint(IConnectionPointContainer *iface,
        REFIID riid, IConnectionPoint **ppCP)
{
    HTMLDocument *This = CONPTCONT_THIS(iface);

    *ppCP = NULL;

    if(IsEqualGUID(&DIID_HTMLDocumentEvents, riid)) {
        TRACE("(%p)->(DIID_HTMLDocumentEvents %p)\n", This, ppCP);
        *ppCP = CONPOINT(&This->cp_htmldocevents);
    }else if(IsEqualGUID(&DIID_HTMLDocumentEvents2, riid)) {
        TRACE("(%p)->(DIID_HTMLDocumentEvents2 %p)\n", This, ppCP);
        *ppCP = CONPOINT(&This->cp_htmldocevents2);
    }else if(IsEqualGUID(&IID_IPropertyNotifySink, riid)) {
        TRACE("(%p)->(IID_IPropertyNotifySink %p)\n", This, ppCP);
        *ppCP = CONPOINT(&This->cp_propnotif);
    }

    if(*ppCP) {
        IConnectionPoint_AddRef(*ppCP);
        return S_OK;
    }

    FIXME("(%p)->(%s %p) unsupported riid\n", This, debugstr_guid(riid), ppCP);
    return CONNECT_E_NOCONNECTION;
}

static const IConnectionPointContainerVtbl ConnectionPointContainerVtbl = {
    ConnectionPointContainer_QueryInterface,
    ConnectionPointContainer_AddRef,
    ConnectionPointContainer_Release,
    ConnectionPointContainer_EnumConnectionPoints,
    ConnectionPointContainer_FindConnectionPoint
};

#undef CONPTCONT_THIS

void HTMLDocument_ConnectionPoints_Init(HTMLDocument *This)
{
    This->lpConnectionPointContainerVtbl = &ConnectionPointContainerVtbl;

    ConnectionPoint_Init(This, &IID_IPropertyNotifySink, &This->cp_propnotif);
    ConnectionPoint_Init(This, &DIID_HTMLDocumentEvents, &This->cp_htmldocevents);
    ConnectionPoint_Init(This, &DIID_HTMLDocumentEvents2, &This->cp_htmldocevents2);
}

void HTMLDocument_ConnectionPoints_Destroy(HTMLDocument *This)
{
    ConnectionPoint_Destroy(&This->cp_propnotif);
    ConnectionPoint_Destroy(&This->cp_htmldocevents);
    ConnectionPoint_Destroy(&This->cp_htmldocevents2);
}
