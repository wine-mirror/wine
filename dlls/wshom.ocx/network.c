/*
 * Copyright 2022 Robert Wilhelm
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

#include "wshom_private.h"
#include "wshom.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wshom);

static HRESULT WINAPI WshNetwork2_QueryInterface(IWshNetwork2 *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(riid, &IID_IUnknown)) {
        TRACE("(IID_IUnknown %p)\n", ppv);
        *ppv = iface;
    }else if(IsEqualGUID(riid, &IID_IDispatch)) {
        TRACE("(IID_IDispatch %p)\n", ppv);
        *ppv = iface;
    }else if(IsEqualGUID(riid, &IID_IWshNetwork2)) {
        TRACE("(IID_IWshNetwork2 %p)\n", ppv);
        *ppv = iface;
    }else {
        FIXME("Unknown iface %s\n", debugstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI WshNetwork2_AddRef(IWshNetwork2 *iface)
{
    TRACE("()\n");
    return 2;
}

static ULONG WINAPI WshNetwork2_Release(IWshNetwork2 *iface)
{
    TRACE("()\n");
    return 2;
}

static HRESULT WINAPI WshNetwork2_GetTypeInfoCount(IWshNetwork2 *iface, UINT *pctinfo)
{
    TRACE("(%p)\n", pctinfo);

    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI WshNetwork2_GetTypeInfo(IWshNetwork2 *iface, UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    FIXME("%p, %u, %lx, %p.\n", iface, iTInfo, lcid, ppTInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI WshNetwork2_GetIDsOfNames(IWshNetwork2 *iface, REFIID riid, LPOLESTR *rgszNames,
        UINT cNames, LCID lcid, DISPID *rgDispId)
{
    FIXME("%p, %s, %p, %u, %lx, %p.\n", iface, debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);
    return E_NOTIMPL;
}

static HRESULT WINAPI WshNetwork2_Invoke(IWshNetwork2 *iface, DISPID dispIdMember, REFIID riid, LCID lcid,
        WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    FIXME("%p, %ld, %s, %lx, %d, %p, %p, %p, %p.\n", iface, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    return E_NOTIMPL;
}

static HRESULT WINAPI WshNetwork2_get_UserDomain(IWshNetwork2 *iface, BSTR *UserDomain)
{
    FIXME("(%p)\n", UserDomain);
    return E_NOTIMPL;
}

static const IWshNetwork2Vtbl WshNetwork2Vtbl = {
    WshNetwork2_QueryInterface,
    WshNetwork2_AddRef,
    WshNetwork2_Release,
    WshNetwork2_GetTypeInfoCount,
    WshNetwork2_GetTypeInfo,
    WshNetwork2_GetIDsOfNames,
    WshNetwork2_Invoke,
    WshNetwork2_get_UserDomain,
};

static IWshNetwork2 WshNetwork2 = { &WshNetwork2Vtbl };

HRESULT WINAPI WshNetworkFactory_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **ppv)
{
    FIXME("(%p %s %p)\n", outer, debugstr_guid(riid), ppv);

    return IWshNetwork2_QueryInterface(&WshNetwork2, riid, ppv);
}
