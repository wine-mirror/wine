/*
 * Copyright 2007 Jacek Caban for CodeWeavers
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

#include "hlink_private.h"

#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(hlink);

#define DEFINE_THIS(cls,ifc,iface) ((cls*)((BYTE*)(iface)-offsetof(cls,lp ## ifc ## Vtbl)))

typedef struct {
    const IUnknownVtbl        *lpIUnknownVtbl;
    const IAuthenticateVtbl   *lpIAuthenticateVtbl;

    LONG ref;
    IUnknown *outer;

    HWND hwnd;
    LPWSTR username;
    LPWSTR password;
} ExtensionService;

#define EXTSERVUNK(x)    ((IUnknown*)       &(x)->lpIUnknownVtbl)
#define AUTHENTICATE(x)  ((IAuthenticate*)  &(x)->lpIAuthenticateVtbl)

#define EXTSERVUNK_THIS(iface)  DEFINE_THIS(ExtensionService, IUnknown, iface)

static HRESULT WINAPI ExtServUnk_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    ExtensionService *This = EXTSERVUNK_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = EXTSERVUNK(This);
    }else if(IsEqualGUID(&IID_IAuthenticate, riid)) {
        TRACE("(%p)->(IID_IAuthenticate %p)\n", This, ppv);
        *ppv = AUTHENTICATE(This);
    }

    if(*ppv) {
        IUnknown_AddRef(EXTSERVUNK(This));
        return S_OK;
    }

    FIXME("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI ExtServUnk_AddRef(IUnknown *iface)
{
    ExtensionService *This = EXTSERVUNK_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI ExtServUnk_Release(IUnknown *iface)
{
    ExtensionService *This = EXTSERVUNK_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        hlink_free(This->username);
        hlink_free(This->password);
        hlink_free(This);
    }

    return ref;
}

#undef EXTSERVUNK_THIS

static const IUnknownVtbl ExtServUnkVtbl = {
    ExtServUnk_QueryInterface,
    ExtServUnk_AddRef,
    ExtServUnk_Release
};

#define AUTHENTICATE_THIS(iface) DEFINE_THIS(ExtensionService, IAuthenticate, iface)

static HRESULT WINAPI Authenticate_QueryInterface(IAuthenticate *iface, REFIID riid, void **ppv)
{
    ExtensionService *This = AUTHENTICATE_THIS(iface);
    return IUnknown_QueryInterface(This->outer, riid, ppv);
}

static ULONG WINAPI Authenticate_AddRef(IAuthenticate *iface)
{
    ExtensionService *This = AUTHENTICATE_THIS(iface);
    return IUnknown_AddRef(This->outer);
}

static ULONG WINAPI Authenticate_Release(IAuthenticate *iface)
{
    ExtensionService *This = AUTHENTICATE_THIS(iface);
    return IUnknown_Release(This->outer);
}

static HRESULT WINAPI Authenticate_Authenticate(IAuthenticate *iface,
        HWND *phwnd, LPWSTR *pszUsername, LPWSTR *pszPassword)
{
    ExtensionService *This = AUTHENTICATE_THIS(iface);

    TRACE("(%p)->(%p %p %p)\n", This, phwnd, pszUsername, pszPassword);

    if(!phwnd || !pszUsername || !pszPassword)
        return E_INVALIDARG;

    *phwnd = This->hwnd;
    *pszUsername = hlink_co_strdupW(This->username);
    *pszPassword = hlink_co_strdupW(This->password);

    return S_OK;
}

#undef AUTHENTICATE_THIS

static const IAuthenticateVtbl AuthenticateVtbl = {
    Authenticate_QueryInterface,
    Authenticate_AddRef,
    Authenticate_Release,
    Authenticate_Authenticate
};

HRESULT WINAPI HlinkCreateExtensionServices(LPCWSTR pwzAdditionalHeaders,
        HWND phwnd, LPCWSTR pszUsername, LPCWSTR pszPassword,
        IUnknown *punkOuter, REFIID riid, void** ppv)
{
    ExtensionService *ret;
    HRESULT hres = S_OK;

    TRACE("%s %p %s %s %p %s %p\n",debugstr_w(pwzAdditionalHeaders),
            phwnd, debugstr_w(pszUsername), debugstr_w(pszPassword),
            punkOuter, debugstr_guid(riid), ppv);

    if(pwzAdditionalHeaders)
        FIXME("Unsupported pwzAdditionalHeaders\n");

    ret = hlink_alloc(sizeof(*ret));

    ret->lpIUnknownVtbl = &ExtServUnkVtbl;
    ret->lpIAuthenticateVtbl = &AuthenticateVtbl;
    ret->ref = 1;
    ret->hwnd = phwnd;
    ret->username = hlink_strdupW(pszUsername);
    ret->password = hlink_strdupW(pszPassword);


    if(!punkOuter) {
        ret->outer = EXTSERVUNK(ret);
        hres = IUnknown_QueryInterface(EXTSERVUNK(ret), riid, ppv);
        IUnknown_Release(EXTSERVUNK(ret));
    }else if(IsEqualGUID(&IID_IUnknown, riid)) {
        ret->outer = punkOuter;
        *ppv = EXTSERVUNK(ret);
    }else {
        IUnknown_Release(EXTSERVUNK(ret));
        hres = E_INVALIDARG;
    }

    return hres;
}
