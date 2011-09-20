/*
 * Copyright 2011 Jacek Caban for CodeWeavers
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

static HRESULT WINAPI WshShell3_QueryInterface(IWshShell3 *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(riid, &IID_IUnknown)) {
        TRACE("(IID_IUnknown %p)\n", ppv);
        *ppv = iface;
    }else if(IsEqualGUID(riid, &IID_IDispatch)) {
        TRACE("(IID_IDispatch %p)\n", ppv);
        *ppv = iface;
    }else if(IsEqualGUID(riid, &IID_IWshShell3)) {
        TRACE("(IID_IWshShell3 %p)\n", ppv);
        *ppv = iface;
    }else {
        FIXME("Unknown iface %s\n", debugstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI WshShell3_AddRef(IWshShell3 *iface)
{
    TRACE("()\n");
    return 2;
}

static ULONG WINAPI WshShell3_Release(IWshShell3 *iface)
{
    TRACE("()\n");
    return 2;
}

static HRESULT WINAPI WshShell3_GetTypeInfoCount(IWshShell3 *iface, UINT *pctinfo)
{
    TRACE("(%p)\n", pctinfo);

    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI WshShell3_GetTypeInfo(IWshShell3 *iface, UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    FIXME("(%u %u %p)\n", iTInfo, lcid, ppTInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI WshShell3_GetIDsOfNames(IWshShell3 *iface, REFIID riid, LPOLESTR *rgszNames,
        UINT cNames, LCID lcid, DISPID *rgDispId)
{
    FIXME("(%s %p %u %u %p)\n", debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);
    return E_NOTIMPL;
}

static HRESULT WINAPI WshShell3_Invoke(IWshShell3 *iface, DISPID dispIdMember, REFIID riid, LCID lcid,
        WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    FIXME("(%d %s %d %d %p %p %p %p)\n", dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    return E_NOTIMPL;
}

static HRESULT WINAPI WshShell3_get_SpecialFolders(IWshShell3 *iface, IWshCollection **out_Folders)
{
    FIXME("(%p)\n", out_Folders);
    return E_NOTIMPL;
}

static HRESULT WINAPI WshShell3_get_Environment(IWshShell3 *iface, VARIANT *Type, IWshEnvironment **out_Env)
{
    FIXME("(%p %p)\n", Type, out_Env);
    return E_NOTIMPL;
}

static HRESULT WINAPI WshShell3_Run(IWshShell3 *iface, BSTR Command, VARIANT *WindowStyle, VARIANT *WaitOnReturn, int *out_ExitCode)
{
    FIXME("(%p %p %p)\n", WindowStyle, WaitOnReturn, out_ExitCode);
    return E_NOTIMPL;
}

static const IWshShell3Vtbl WshShell3Vtbl = {
    WshShell3_QueryInterface,
    WshShell3_AddRef,
    WshShell3_Release,
    WshShell3_GetTypeInfoCount,
    WshShell3_GetTypeInfo,
    WshShell3_GetIDsOfNames,
    WshShell3_Invoke,
    WshShell3_get_SpecialFolders,
    WshShell3_get_Environment,
    WshShell3_Run
};

static IWshShell3 WshShell3 = { &WshShell3Vtbl };

HRESULT WINAPI WshShellFactory_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **ppv)
{
    TRACE("(%p %s %p)\n", outer, debugstr_guid(riid), ppv);

    return IWshShell3_QueryInterface(&WshShell3, riid, ppv);
}
