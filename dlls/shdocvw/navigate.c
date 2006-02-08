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
#include "wine/unicode.h"

#include "shdocvw.h"

WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);

#define HLINKFRAME_THIS(iface) DEFINE_THIS(WebBrowser, HlinkFrame, iface)

static HRESULT WINAPI HlinkFrame_QueryInterface(IHlinkFrame *iface, REFIID riid, void **ppv)
{
    WebBrowser *This = HLINKFRAME_THIS(iface);
    return IWebBrowser2_QueryInterface(WEBBROWSER2(This), riid, ppv);
}

static ULONG WINAPI HlinkFrame_AddRef(IHlinkFrame *iface)
{
    WebBrowser *This = HLINKFRAME_THIS(iface);
    return IWebBrowser2_AddRef(WEBBROWSER2(This));
}

static ULONG WINAPI HlinkFrame_Release(IHlinkFrame *iface)
{
    WebBrowser *This = HLINKFRAME_THIS(iface);
    return IWebBrowser2_Release(WEBBROWSER2(This));
}

static HRESULT WINAPI HlinkFrame_SetBrowseContext(IHlinkFrame *iface,
                                                  IHlinkBrowseContext *pihlbc)
{
    WebBrowser *This = HLINKFRAME_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pihlbc);
    return E_NOTIMPL;
}

static HRESULT WINAPI HlinkFrame_GetBrowseContext(IHlinkFrame *iface,
                                                  IHlinkBrowseContext **ppihlbc)
{
    WebBrowser *This = HLINKFRAME_THIS(iface);
    FIXME("(%p)->(%p)\n", This, ppihlbc);
    return E_NOTIMPL;
}

static HRESULT WINAPI HlinkFrame_Navigate(IHlinkFrame *iface, DWORD grfHLNF, LPBC pbc,
                                          IBindStatusCallback *pibsc, IHlink *pihlNavigate)
{
    WebBrowser *This = HLINKFRAME_THIS(iface);
    FIXME("(%p)->(%08lx %p %p %p)\n", This, grfHLNF, pbc, pibsc, pihlNavigate);
    return E_NOTIMPL;
}

static HRESULT WINAPI HlinkFrame_OnNavigate(IHlinkFrame *iface, DWORD grfHLNF,
        IMoniker *pimkTarget, LPCWSTR pwzLocation, LPCWSTR pwzFriendlyName, DWORD dwreserved)
{
    WebBrowser *This = HLINKFRAME_THIS(iface);
    FIXME("(%p)->(%08lx %p %s %s %ld)\n", This, grfHLNF, pimkTarget, debugstr_w(pwzLocation),
          debugstr_w(pwzFriendlyName), dwreserved);
    return E_NOTIMPL;
}

static HRESULT WINAPI HlinkFrame_UpdateHlink(IHlinkFrame *iface, ULONG uHLID,
        IMoniker *pimkTarget, LPCWSTR pwzLocation, LPCWSTR pwzFriendlyName)
{
    WebBrowser *This = HLINKFRAME_THIS(iface);
    FIXME("(%p)->(%lu %p %s %s)\n", This, uHLID, pimkTarget, debugstr_w(pwzLocation),
          debugstr_w(pwzFriendlyName));
    return E_NOTIMPL;
}

#undef HLINKFRAME_THIS

static const IHlinkFrameVtbl HlinkFrameVtbl = {
    HlinkFrame_QueryInterface,
    HlinkFrame_AddRef,
    HlinkFrame_Release,
    HlinkFrame_SetBrowseContext,
    HlinkFrame_GetBrowseContext,
    HlinkFrame_Navigate,
    HlinkFrame_OnNavigate,
    HlinkFrame_UpdateHlink
};

void WebBrowser_HlinkFrame_Init(WebBrowser *This)
{
    This->lpHlinkFrameVtbl = &HlinkFrameVtbl;
}
