/*
 * Copyright 2005 Jacek Caban for CodeWeavers
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

#include "wine/debug.h"
#include "shdocvw.h"

WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);

#define CLIENTSITE_THIS(iface) DEFINE_THIS(WebBrowser, OleClientSite, iface)

static HRESULT WINAPI ClientSite_QueryInterface(IOleClientSite *iface, REFIID riid, void **ppv)
{
    WebBrowser *This = CLIENTSITE_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = CLIENTSITE(This);
    }else if(IsEqualGUID(&IID_IOleClientSite, riid)) {
        TRACE("(%p)->(IID_IOleClientSite %p)\n", This, ppv);
        *ppv = CLIENTSITE(This);
    }

    if(*ppv) {
        IWebBrowser2_AddRef(WEBBROWSER(This));
        return S_OK;
    }

    WARN("Unsupported intrface %s\n", debugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI ClientSite_AddRef(IOleClientSite *iface)
{
    WebBrowser *This = CLIENTSITE_THIS(iface);
    return IWebBrowser2_AddRef(WEBBROWSER(This));
}

static ULONG WINAPI ClientSite_Release(IOleClientSite *iface)
{
    WebBrowser *This = CLIENTSITE_THIS(iface);
    return IWebBrowser2_Release(WEBBROWSER(This));
}

static HRESULT WINAPI ClientSite_SaveObject(IOleClientSite *iface)
{
    WebBrowser *This = CLIENTSITE_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ClientSite_GetMoniker(IOleClientSite *iface, DWORD dwAssign,
                                            DWORD dwWhichMoniker, IMoniker **ppmk)
{
    WebBrowser *This = CLIENTSITE_THIS(iface);
    FIXME("(%p)->(%ld %ld %p)\n", This, dwAssign, dwWhichMoniker, ppmk);
    return E_NOTIMPL;
}

static HRESULT WINAPI ClientSite_GetContainer(IOleClientSite *iface, IOleContainer **ppContainer)
{
    WebBrowser *This = CLIENTSITE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, ppContainer);
    return E_NOTIMPL;
}

static HRESULT WINAPI ClientSite_ShowObject(IOleClientSite *iface)
{
    WebBrowser *This = CLIENTSITE_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ClientSite_OnShowWindow(IOleClientSite *iface, BOOL fShow)
{
    WebBrowser *This = CLIENTSITE_THIS(iface);
    FIXME("(%p)->(%x)\n", This, fShow);
    return E_NOTIMPL;
}

static HRESULT WINAPI ClientSite_RequestNewObjectLayout(IOleClientSite *iface)
{
    WebBrowser *This = CLIENTSITE_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

#undef CLIENTSITE_THIS

static const IOleClientSiteVtbl OleClientSiteVtbl = {
    ClientSite_QueryInterface,
    ClientSite_AddRef,
    ClientSite_Release,
    ClientSite_SaveObject,
    ClientSite_GetMoniker,
    ClientSite_GetContainer,
    ClientSite_ShowObject,
    ClientSite_OnShowWindow,
    ClientSite_RequestNewObjectLayout
};

void WebBrowser_ClientSite_Init(WebBrowser *This)
{
    This->lpOleClientSiteVtbl = &OleClientSiteVtbl;
}
