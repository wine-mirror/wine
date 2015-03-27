/*
 * WiaDevMgr functions
 *
 * Copyright 2009 Damjan Jovanovic
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

#include "objbase.h"
#include "wia_lh.h"

#include "wiaservc_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wia);

static inline wiadevmgr *impl_from_IWiaDevMgr(IWiaDevMgr *iface)
{
    return CONTAINING_RECORD(iface, wiadevmgr, IWiaDevMgr_iface);
}

static HRESULT WINAPI wiadevmgr_QueryInterface(IWiaDevMgr *iface, REFIID riid, void **ppvObject)
{
    wiadevmgr *This = impl_from_IWiaDevMgr(iface);

    TRACE("(%p, %s, %p)\n", This, debugstr_guid(riid), ppvObject);

    if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IWiaDevMgr))
        *ppvObject = iface;
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }
    IUnknown_AddRef((IUnknown*) *ppvObject);
    return S_OK;
}

static ULONG WINAPI wiadevmgr_AddRef(IWiaDevMgr *iface)
{
    wiadevmgr *This = impl_from_IWiaDevMgr(iface);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI wiadevmgr_Release(IWiaDevMgr *iface)
{
    ULONG ref;
    wiadevmgr *This = impl_from_IWiaDevMgr(iface);

    ref = InterlockedDecrement(&This->ref);
    if (ref == 0)
        HeapFree(GetProcessHeap(), 0, This);
    return ref;
}

static HRESULT WINAPI wiadevmgr_EnumDeviceInfo(IWiaDevMgr *iface, LONG lFlag, IEnumWIA_DEV_INFO **ppIEnum)
{
    wiadevmgr *This = impl_from_IWiaDevMgr(iface);
    FIXME("(%p, %d, %p): stub\n", This, lFlag, ppIEnum);
    return E_NOTIMPL;
}

static HRESULT WINAPI wiadevmgr_CreateDevice(IWiaDevMgr *iface, BSTR bstrDeviceID, IWiaItem **ppWiaItemRoot)
{
    wiadevmgr *This = impl_from_IWiaDevMgr(iface);
    FIXME("(%p, %s, %p): stub\n", This, debugstr_w(bstrDeviceID), ppWiaItemRoot);
    return E_NOTIMPL;
}

static HRESULT WINAPI wiadevmgr_SelectDeviceDlg(IWiaDevMgr *iface, HWND hwndParent, LONG lDeviceType,
                                                LONG lFlags, BSTR *pbstrDeviceID, IWiaItem **ppItemRoot)
{
    wiadevmgr *This = impl_from_IWiaDevMgr(iface);
    FIXME("(%p, %p, %d, 0x%x, %p, %p): stub\n", This, hwndParent, lDeviceType, lFlags, pbstrDeviceID, ppItemRoot);
    return E_NOTIMPL;
}

static HRESULT WINAPI wiadevmgr_SelectDeviceDlgID(IWiaDevMgr *iface, HWND hwndParent, LONG lDeviceType,
                                                  LONG lFlags, BSTR *pbstrDeviceID)
{
    wiadevmgr *This = impl_from_IWiaDevMgr(iface);
    FIXME("(%p, %p, %d, 0x%x, %p): stub\n", This, hwndParent, lDeviceType, lFlags, pbstrDeviceID);
    return E_NOTIMPL;
}

static HRESULT WINAPI wiadevmgr_GetImageDlg(IWiaDevMgr *iface, HWND hwndParent, LONG lDeviceType,
                                            LONG lFlags, LONG lIntent, IWiaItem *pItemRoot,
                                            BSTR bstrFilename, GUID *pguidFormat)
{
    wiadevmgr *This = impl_from_IWiaDevMgr(iface);
    FIXME("(%p, %p, %d, 0x%x, %d, %p, %s, %s): stub\n", This, hwndParent, lDeviceType, lFlags,
        lIntent, pItemRoot, debugstr_w(bstrFilename), debugstr_guid(pguidFormat));
    return E_NOTIMPL;
}

static HRESULT WINAPI wiadevmgr_RegisterEventCallbackProgram(IWiaDevMgr *iface, LONG lFlags, BSTR bstrDeviceID,
                                                             const GUID *pEventGUID, BSTR bstrCommandline,
                                                             BSTR bstrName, BSTR bstrDescription, BSTR bstrIcon)
{
    wiadevmgr *This = impl_from_IWiaDevMgr(iface);
    FIXME("(%p, 0x%x, %s, %s, %s, %s, %s, %s): stub\n", This, lFlags, debugstr_w(bstrDeviceID),
        debugstr_guid(pEventGUID), debugstr_w(bstrCommandline), debugstr_w(bstrName),
        debugstr_w(bstrDescription), debugstr_w(bstrIcon));
    return E_NOTIMPL;
}

static HRESULT WINAPI wiadevmgr_RegisterEventCallbackInterface(IWiaDevMgr *iface, LONG lFlags, BSTR bstrDeviceID,
                                                               const GUID *pEventGUID, IWiaEventCallback *pIWiaEventCallback,
                                                               IUnknown **pEventObject)
{
    wiadevmgr *This = impl_from_IWiaDevMgr(iface);
    FIXME("(%p, 0x%x, %s, %s, %p, %p): stub\n", This, lFlags, debugstr_w(bstrDeviceID),
        debugstr_guid(pEventGUID), pIWiaEventCallback, pEventObject);
    return E_NOTIMPL;
}

static HRESULT WINAPI wiadevmgr_RegisterEventCallbackCLSID(IWiaDevMgr *iface, LONG lFlags, BSTR bstrDeviceID,
                                                           const GUID *pEventGUID, const GUID *pClsID, BSTR bstrName,
                                                           BSTR bstrDescription, BSTR bstrIcon)
{
    wiadevmgr *This = impl_from_IWiaDevMgr(iface);
    FIXME("(%p, 0x%x, %s, %s, %s, %s, %s, %s): stub\n", This, lFlags, debugstr_w(bstrDeviceID),
        debugstr_guid(pEventGUID), debugstr_guid(pClsID), debugstr_w(bstrName),
        debugstr_w(bstrDescription), debugstr_w(bstrIcon));
    return E_NOTIMPL;
}

static HRESULT WINAPI wiadevmgr_AddDeviceDlg(IWiaDevMgr *iface, HWND hwndParent, LONG lFlags)
{
    wiadevmgr *This = impl_from_IWiaDevMgr(iface);
    FIXME("(%p, %p, 0x%x): stub\n", This, hwndParent, lFlags);
    return E_NOTIMPL;
}

static const IWiaDevMgrVtbl WIASERVC_IWiaDevMgr_Vtbl =
{
    wiadevmgr_QueryInterface,
    wiadevmgr_AddRef,
    wiadevmgr_Release,
    wiadevmgr_EnumDeviceInfo,
    wiadevmgr_CreateDevice,
    wiadevmgr_SelectDeviceDlg,
    wiadevmgr_SelectDeviceDlgID,
    wiadevmgr_GetImageDlg,
    wiadevmgr_RegisterEventCallbackProgram,
    wiadevmgr_RegisterEventCallbackInterface,
    wiadevmgr_RegisterEventCallbackCLSID,
    wiadevmgr_AddDeviceDlg
};

HRESULT wiadevmgr_Constructor(IWiaDevMgr **ppObj)
{
    wiadevmgr *This;
    TRACE("(%p)\n", ppObj);
    This = HeapAlloc(GetProcessHeap(), 0, sizeof(wiadevmgr));
    if (This)
    {
        This->IWiaDevMgr_iface.lpVtbl = &WIASERVC_IWiaDevMgr_Vtbl;
        This->ref = 1;
        *ppObj = &This->IWiaDevMgr_iface;
        return S_OK;
    }
    *ppObj = NULL;
    return E_OUTOFMEMORY;
}
