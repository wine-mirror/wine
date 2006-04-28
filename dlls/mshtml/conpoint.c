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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

struct ConnectionPoint {
    const IConnectionPointVtbl *lpConnectionPointVtbl;

    HTMLDocument *doc;
};

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
    FIXME("(%p)->(%p)\n", This, pIID);
    return E_NOTIMPL;
}

static HRESULT WINAPI ConnectionPoint_GetConnectionPointContainer(IConnectionPoint *iface,
        IConnectionPointContainer **ppCPC)
{
    ConnectionPoint *This = CONPOINT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, ppCPC);
    return E_NOTIMPL;
}

static HRESULT WINAPI ConnectionPoint_Advise(IConnectionPoint *iface, IUnknown *pUnkSink,
                                             DWORD *pdwCookie)
{
    ConnectionPoint *This = CONPOINT_THIS(iface);
    FIXME("(%p)->(%p %p)\n", This, pUnkSink, pdwCookie);
    return E_NOTIMPL;
}

static HRESULT WINAPI ConnectionPoint_Unadvise(IConnectionPoint *iface, DWORD dwCookie)
{
    ConnectionPoint *This = CONPOINT_THIS(iface);
    FIXME("(%p)->(%ld)\n", This, dwCookie);
    return E_NOTIMPL;
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

static void ConnectionPoint_Create(HTMLDocument *doc, REFIID riid, ConnectionPoint **cp)
{
    ConnectionPoint *ret = HeapAlloc(GetProcessHeap(), 0, sizeof(ConnectionPoint));

    ret->lpConnectionPointVtbl = &ConnectionPointVtbl;
    ret->doc = doc;

    *cp = ret;
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
        *ppCP = CONPOINT(This->cp_htmldocevents);
    }else if(IsEqualGUID(&DIID_HTMLDocumentEvents2, riid)) {
        TRACE("(%p)->(DIID_HTMLDocumentEvents2 %p)\n", This, ppCP);
        *ppCP = CONPOINT(This->cp_htmldocevents2);
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

    ConnectionPoint_Create(This, &DIID_HTMLDocumentEvents, &This->cp_htmldocevents);
    ConnectionPoint_Create(This, &DIID_HTMLDocumentEvents2, &This->cp_htmldocevents2);
}
