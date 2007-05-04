/*
 * Copyright 2005-2006 Jacek Caban for CodeWeavers
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
#include "wine/unicode.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

/**********************************************************
 * IHlinkTarget implementation
 */

#define HLINKTRG_THIS(iface) DEFINE_THIS(HTMLDocument, HlinkTarget, iface)

static HRESULT WINAPI HlinkTarget_QueryInterface(IHlinkTarget *iface, REFIID riid, void **ppv)
{
    HTMLDocument *This = HLINKTRG_THIS(iface);
    return IHTMLDocument2_QueryInterface(HTMLDOC(This), riid, ppv);
}

static ULONG WINAPI HlinkTarget_AddRef(IHlinkTarget *iface)
{
    HTMLDocument *This = HLINKTRG_THIS(iface);
    return IHTMLDocument2_AddRef(HTMLDOC(This));
}

static ULONG WINAPI HlinkTarget_Release(IHlinkTarget *iface)
{
    HTMLDocument *This = HLINKTRG_THIS(iface);
    return IHTMLDocument2_Release(HTMLDOC(This));
}

static HRESULT WINAPI HlinkTarget_SetBrowseContext(IHlinkTarget *iface, IHlinkBrowseContext *pihlbc)
{
    HTMLDocument *This = HLINKTRG_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pihlbc);
    return E_NOTIMPL;
}

static HRESULT WINAPI HlinkTarget_GetBrowseContext(IHlinkTarget *iface, IHlinkBrowseContext **ppihlbc)
{
    HTMLDocument *This = HLINKTRG_THIS(iface);
    FIXME("(%p)->(%p)\n", This, ppihlbc);
    return E_NOTIMPL;
}

static HRESULT WINAPI HlinkTarget_Navigate(IHlinkTarget *iface, DWORD grfHLNF, LPCWSTR pwzJumpLocation)
{
    HTMLDocument *This = HLINKTRG_THIS(iface);

    TRACE("(%p)->(%08x %s)\n", This, grfHLNF, debugstr_w(pwzJumpLocation));

    if(grfHLNF)
        FIXME("Unsupported grfHLNF=%08x\n", grfHLNF);
    if(pwzJumpLocation)
        FIXME("JumpLocation not supported\n");

    return IOleObject_DoVerb(OLEOBJ(This), OLEIVERB_SHOW, NULL, NULL, -1, NULL, NULL);
}

static HRESULT WINAPI HlinkTarget_GetMoniker(IHlinkTarget *iface, LPCWSTR pwzLocation, DWORD dwAssign,
        IMoniker **ppimkLocation)
{
    HTMLDocument *This = HLINKTRG_THIS(iface);
    FIXME("(%p)->(%s %08x %p)\n", This, debugstr_w(pwzLocation), dwAssign, ppimkLocation);
    return E_NOTIMPL;
}

static HRESULT WINAPI HlinkTarget_GetFriendlyName(IHlinkTarget *iface, LPCWSTR pwzLocation,
        LPWSTR *ppwzFriendlyName)
{
    HTMLDocument *This = HLINKTRG_THIS(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(pwzLocation), ppwzFriendlyName);
    return E_NOTIMPL;
}

static const IHlinkTargetVtbl HlinkTargetVtbl = {
    HlinkTarget_QueryInterface,
    HlinkTarget_AddRef,
    HlinkTarget_Release,
    HlinkTarget_SetBrowseContext,
    HlinkTarget_GetBrowseContext,
    HlinkTarget_Navigate,
    HlinkTarget_GetMoniker,
    HlinkTarget_GetFriendlyName
};

void HTMLDocument_Hlink_Init(HTMLDocument *This)
{
    This->lpHlinkTargetVtbl = &HlinkTargetVtbl;
}

typedef struct {
    const IHlinkVtbl  *lpHlinkVtbl;

    LONG ref;

    IMoniker *mon;
    LPWSTR location;
} Hlink;

#define HLINK(x)  ((IHlink*)  &(x)->lpHlinkVtbl)

#define HLINK_THIS(iface) DEFINE_THIS(Hlink, Hlink, iface)

static HRESULT WINAPI Hlink_QueryInterface(IHlink *iface, REFIID riid, void **ppv)
{
    Hlink *This = HLINK_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = HLINK(This);
    }else if(IsEqualGUID(&IID_IHlink, riid)) {
        TRACE("(%p)->(IID_IHlink %p)\n", This, ppv);
        *ppv = HLINK(This);
    }

    if(*ppv) {
        IHlink_AddRef(HLINK(This));
        return S_OK;
    }

    WARN("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI Hlink_AddRef(IHlink *iface)
{
    Hlink *This = HLINK_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI Hlink_Release(IHlink *iface)
{
    Hlink *This = HLINK_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        if(This->mon)
            IMoniker_Release(This->mon);
        mshtml_free(This->location);
        mshtml_free(This);
    }

    return ref;
}

static HRESULT WINAPI Hlink_SetHlinkSite(IHlink *iface, IHlinkSite *pihlSite, DWORD dwSiteData)
{
    Hlink *This = HLINK_THIS(iface);
    FIXME("(%p)->(%p %d)\n", This, pihlSite, dwSiteData);
    return E_NOTIMPL;
}

static HRESULT WINAPI Hlink_GetHlinkSite(IHlink *iface, IHlinkSite **ppihlSite,
                                         DWORD *pdwSiteData)
{
    Hlink *This = HLINK_THIS(iface);

    TRACE("(%p)->(%p %p)\n", This, ppihlSite, pdwSiteData);

    *ppihlSite = NULL;
    return S_OK;
}

static HRESULT WINAPI Hlink_SetMonikerReference(IHlink *iface, DWORD grfHLSETF,
                                                IMoniker *pimkTarget, LPCWSTR pwzLocation)
{
    Hlink *This = HLINK_THIS(iface);

    TRACE("(%p)->(%08x %p %s)\n", This, grfHLSETF, pimkTarget, debugstr_w(pwzLocation));

    if(grfHLSETF)
        FIXME("unsupported grfHLSETF=%08x\n", grfHLSETF);

    if(This->mon)
        IMoniker_Release(This->mon);
    mshtml_free(This->location);

    if(pimkTarget)
        IMoniker_AddRef(pimkTarget);
    This->mon = pimkTarget;

    if(pwzLocation) {
        DWORD len = strlenW(pwzLocation)+1;

        This->location = mshtml_alloc(len*sizeof(WCHAR));
        memcpy(This->location, pwzLocation, len*sizeof(WCHAR));
    }else {
        This->location = NULL;
    }

    return S_OK;
}

static HRESULT WINAPI Hlink_GetMonikerReference(IHlink *iface, DWORD dwWhichRef,
                                                IMoniker **ppimkTarget, LPWSTR *ppwzLocation)
{
    Hlink *This = HLINK_THIS(iface);

    TRACE("(%p)->(%d %p %p)\n", This, dwWhichRef, ppimkTarget, ppwzLocation);

    if(dwWhichRef != 1)
        FIXME("upsupported dwWhichRef = %d\n", dwWhichRef);

    if(This->mon)
        IMoniker_AddRef(This->mon);
    *ppimkTarget = This->mon;

    if(This->location) {
        DWORD len = strlenW(This->location)+1;

        *ppwzLocation = CoTaskMemAlloc(len*sizeof(WCHAR));
        memcpy(*ppwzLocation, This->location, len*sizeof(WCHAR));
    }else {
        *ppwzLocation = NULL;
    }

    return S_OK;
}

static HRESULT WINAPI Hlink_SetStringReference(IHlink *iface, DWORD grfHLSETF,
                                               LPCWSTR pwzTarget, LPCWSTR pwzLocation)
{
    Hlink *This = HLINK_THIS(iface);
    FIXME("(%p)->(%08x %s %s)\n", This, grfHLSETF, debugstr_w(pwzTarget),
          debugstr_w(pwzLocation));
    return E_NOTIMPL;
}

static HRESULT WINAPI Hlink_GetStringReference(IHlink *iface, DWORD dwWhichRef,
                                               LPWSTR *ppwzTarget, LPWSTR *ppwzLocation)
{
    Hlink *This = HLINK_THIS(iface);
    FIXME("(%p)->(%d %p %p)\n", This, dwWhichRef, ppwzTarget, ppwzLocation);
    return E_NOTIMPL;
}

static HRESULT WINAPI Hlink_SetFriendlyName(IHlink *iface, LPCWSTR pwzFriendlyName)
{
    Hlink *This = HLINK_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(pwzFriendlyName));
    return E_NOTIMPL;
}

static HRESULT WINAPI Hlink_GetFriendlyName(IHlink *iface, DWORD grfHLNAMEF,
                                            LPWSTR *ppwzFriendlyName)
{
    Hlink *This = HLINK_THIS(iface);

    TRACE("(%p)->(%08x %p)\n", This, grfHLNAMEF, ppwzFriendlyName);

    *ppwzFriendlyName = NULL;
    return S_FALSE;
}

static HRESULT WINAPI Hlink_SetTargetFrameName(IHlink *iface, LPCWSTR pwzTargetFrameName)
{
    Hlink *This = HLINK_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(pwzTargetFrameName));
    return E_NOTIMPL;
}

static HRESULT WINAPI Hlink_GetTargetFrameName(IHlink *iface, LPWSTR *ppwzTargetFrameName)
{
    Hlink *This = HLINK_THIS(iface);

    TRACE("(%p)->(%p)\n", This, ppwzTargetFrameName);

    *ppwzTargetFrameName = NULL;
    return S_FALSE;
}

static HRESULT WINAPI Hlink_GetMiscStatus(IHlink *iface, DWORD *pdwStatus)
{
    Hlink *This = HLINK_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pdwStatus);
    return E_NOTIMPL;
}

static HRESULT WINAPI Hlink_Navigate(IHlink *iface, DWORD grfHLNF, LPBC pibc,
        IBindStatusCallback *pibsc, IHlinkBrowseContext *pihlbc)
{
    Hlink *This = HLINK_THIS(iface);
    FIXME("(%p)->(%08x %p %p %p)\n", This, grfHLNF, pibc, pibsc, pihlbc);
    return E_NOTIMPL;
}

static HRESULT WINAPI Hlink_SetAdditionalParams(IHlink *iface, LPCWSTR pwzAdditionalParams)
{
    Hlink *This = HLINK_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(pwzAdditionalParams));
    return E_NOTIMPL;
}

static HRESULT WINAPI Hlink_GetAdditionalParams(IHlink *iface, LPWSTR *ppwzAdditionalParams)
{
    Hlink *This = HLINK_THIS(iface);
    FIXME("(%p)->(%p)\n", This, ppwzAdditionalParams);
    return E_NOTIMPL;
}

#undef HLINK_THIS

static const IHlinkVtbl HlinkVtbl = {
    Hlink_QueryInterface,
    Hlink_AddRef,
    Hlink_Release,
    Hlink_SetHlinkSite,
    Hlink_GetHlinkSite,
    Hlink_SetMonikerReference,
    Hlink_GetMonikerReference,
    Hlink_SetStringReference,
    Hlink_GetStringReference,
    Hlink_SetFriendlyName,
    Hlink_GetFriendlyName,
    Hlink_SetTargetFrameName,
    Hlink_GetTargetFrameName,
    Hlink_GetMiscStatus,
    Hlink_Navigate,
    Hlink_SetAdditionalParams,
    Hlink_GetAdditionalParams
};

IHlink *Hlink_Create(void)
{
    Hlink *ret = mshtml_alloc(sizeof(Hlink));

    ret->lpHlinkVtbl = &HlinkVtbl;
    ret->ref = 1;
    ret->mon = NULL;
    ret->location = NULL;

    return HLINK(ret);
}
