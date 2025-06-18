/*
 * Copyright 2012 Jacek Caban for CodeWeavers
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

#define COBJMACROS

#include "windows.h"
#include "initguid.h"
#include "msident.h"
#include "rpcproxy.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msident);

typedef struct {
    IEnumUserIdentity IEnumUserIdentity_iface;
    LONG ref;
} EnumUserIdentity;

static inline EnumUserIdentity *impl_from_IEnumUserIdentity(IEnumUserIdentity *iface)
{
    return CONTAINING_RECORD(iface, EnumUserIdentity, IEnumUserIdentity_iface);
}

static HRESULT WINAPI EnumUserIdentity_QueryInterface(IEnumUserIdentity *iface, REFIID riid, void **ppv)
{
    EnumUserIdentity *This = impl_from_IEnumUserIdentity(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(IID_IUnknown %p)\n", ppv);
        *ppv = &This->IEnumUserIdentity_iface;
    }else if(IsEqualGUID(&IID_IEnumUserIdentity, riid)) {
        TRACE("(IID_IEnumUserIdentity %p)\n", ppv);
        *ppv = &This->IEnumUserIdentity_iface;
    }else {
        WARN("(%s %p)\n", debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI EnumUserIdentity_AddRef(IEnumUserIdentity *iface)
{
    EnumUserIdentity *This = impl_from_IEnumUserIdentity(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI EnumUserIdentity_Release(IEnumUserIdentity *iface)
{
    EnumUserIdentity *This = impl_from_IEnumUserIdentity(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref)
        free(This);

    return ref;
}

static HRESULT WINAPI EnumUserIdentity_Next(IEnumUserIdentity *iface, ULONG celt, IUnknown **rgelt, ULONG *pceltFetched)
{
    EnumUserIdentity *This = impl_from_IEnumUserIdentity(iface);
    FIXME("(%p)->(%lu %p %p)\n", This, celt, rgelt, pceltFetched);
    return E_NOTIMPL;
}

static HRESULT WINAPI EnumUserIdentity_Skip(IEnumUserIdentity *iface, ULONG celt)
{
    EnumUserIdentity *This = impl_from_IEnumUserIdentity(iface);
    FIXME("(%p)->(%lu)\n", This, celt);
    return E_NOTIMPL;
}

static HRESULT WINAPI EnumUserIdentity_Reset(IEnumUserIdentity *iface)
{
    EnumUserIdentity *This = impl_from_IEnumUserIdentity(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI EnumUserIdentity_Clone(IEnumUserIdentity *iface, IEnumUserIdentity **ppenum)
{
    EnumUserIdentity *This = impl_from_IEnumUserIdentity(iface);
    FIXME("(%p)->(%p)\n", This, ppenum);
    return E_NOTIMPL;
}

static HRESULT WINAPI EnumUserIdentity_GetCount(IEnumUserIdentity *iface, ULONG *pnCount)
{
    EnumUserIdentity *This = impl_from_IEnumUserIdentity(iface);

    FIXME("(%p)->(%p)\n", This, pnCount);

    *pnCount = 0;
    return S_OK;
}

static const IEnumUserIdentityVtbl EnumUserIdentityVtbl = {
    EnumUserIdentity_QueryInterface,
    EnumUserIdentity_AddRef,
    EnumUserIdentity_Release,
    EnumUserIdentity_Next,
    EnumUserIdentity_Skip,
    EnumUserIdentity_Reset,
    EnumUserIdentity_Clone,
    EnumUserIdentity_GetCount
};

static HRESULT WINAPI UserIdentityManager_QueryInterface(IUserIdentityManager *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(IID_IUnknown %p)\n", ppv);
        *ppv = iface;
    }else if(IsEqualGUID(&IID_IUserIdentityManager, riid)) {
        TRACE("(IID_IUserIdentityManager %p)\n", ppv);
        *ppv = iface;
    }else {
        WARN("(%s %p)\n", debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI UserIdentityManager_AddRef(IUserIdentityManager *iface)
{
    TRACE("\n");
    return 2;
}

static ULONG WINAPI UserIdentityManager_Release(IUserIdentityManager *iface)
{
    TRACE("\n");
    return 1;
}

static HRESULT WINAPI UserIdentityManager_EnumIdentities(IUserIdentityManager *iface, IEnumUserIdentity **ppEnumUser)
{
    EnumUserIdentity *ret;

    TRACE("(%p)\n", ppEnumUser);

    ret = malloc(sizeof(*ret));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IEnumUserIdentity_iface.lpVtbl = &EnumUserIdentityVtbl;
    ret->ref = 1;

    *ppEnumUser = &ret->IEnumUserIdentity_iface;
    return S_OK;
}

static HRESULT WINAPI UserIdentityManager_ManageIdentities(IUserIdentityManager *iface, HWND hwndParent, DWORD dwFlags)
{
    FIXME("(%p %lx)\n", hwndParent, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI UserIdentityManager_Logon(IUserIdentityManager *iface, HWND hwndParent,
        DWORD dwFlags, IUserIdentity **ppIdentity)
{
    FIXME("(%p %lx %p)\n", hwndParent, dwFlags, ppIdentity);
    return E_USER_CANCELLED;
}

static HRESULT WINAPI UserIdentityManager_Logoff(IUserIdentityManager *iface, HWND hwndParent)
{
    FIXME("(%p)\n", hwndParent);
    return E_NOTIMPL;
}

static HRESULT WINAPI UserIdentityManager_GetIdentityByCookie(IUserIdentityManager *iface, GUID *uidCookie,
        IUserIdentity **ppIdentity)
{
    FIXME("(%p %p)\n", uidCookie, ppIdentity);
    return E_NOTIMPL;
}

static const IUserIdentityManagerVtbl UserIdentityManagerVtbl = {
    UserIdentityManager_QueryInterface,
    UserIdentityManager_AddRef,
    UserIdentityManager_Release,
    UserIdentityManager_EnumIdentities,
    UserIdentityManager_ManageIdentities,
    UserIdentityManager_Logon,
    UserIdentityManager_Logoff,
    UserIdentityManager_GetIdentityByCookie
};

static IUserIdentityManager UserIdentityManager = { &UserIdentityManagerVtbl };

static HRESULT WINAPI UserIdentityManager_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **ppv)
{
    TRACE("\n");

    return IUserIdentityManager_QueryInterface(&UserIdentityManager, riid, ppv);
}

static HRESULT WINAPI ClassFactory_QueryInterface(IClassFactory *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", iface, ppv);
        *ppv = iface;
    }else if(IsEqualGUID(&IID_IClassFactory, riid)) {
        TRACE("(%p)->(IID_IClassFactory %p)\n", iface, ppv);
        *ppv = iface;
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    FIXME("(%p)->(%s %p)\n", iface, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI ClassFactory_AddRef(IClassFactory *iface)
{
    TRACE("(%p)\n", iface);
    return 2;
}

static ULONG WINAPI ClassFactory_Release(IClassFactory *iface)
{
    TRACE("(%p)\n", iface);
    return 1;
}

static HRESULT WINAPI ClassFactory_LockServer(IClassFactory *iface, BOOL fLock)
{
    TRACE("(%p)->(%x)\n", iface, fLock);
    return S_OK;
}

static const IClassFactoryVtbl UserIdentityManagerCFVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    UserIdentityManager_CreateInstance,
    ClassFactory_LockServer
};

static IClassFactory UserIdentityManagerCF = { &UserIdentityManagerCFVtbl };

/***********************************************************************
 *		DllGetClassObject	(msident.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    if(IsEqualGUID(&CLSID_UserIdentityManager, rclsid)) {
        TRACE("CLSID_UserIdentityManager\n");
        return IClassFactory_QueryInterface(&UserIdentityManagerCF, riid, ppv);
    }

    FIXME("%s %s %p\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}
