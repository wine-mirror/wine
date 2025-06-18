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

typedef struct
{
    IEnumWIA_DEV_INFO IEnumWIA_DEV_INFO_iface;
    LONG ref;
} enumwiadevinfo;

static inline wiadevmgr *impl_from_IWiaDevMgr(IWiaDevMgr *iface)
{
    return CONTAINING_RECORD(iface, wiadevmgr, IWiaDevMgr_iface);
}

static inline enumwiadevinfo *impl_from_IEnumWIA_DEV_INFO(IEnumWIA_DEV_INFO *iface)
{
    return CONTAINING_RECORD(iface, enumwiadevinfo, IEnumWIA_DEV_INFO_iface);
}

static HRESULT WINAPI enumwiadevinfo_QueryInterface(IEnumWIA_DEV_INFO *iface, REFIID riid, void **obj)
{
    enumwiadevinfo *This = impl_from_IEnumWIA_DEV_INFO(iface);

    TRACE("(%p, %s, %p)\n", This, debugstr_guid(riid), obj);

    if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IEnumWIA_DEV_INFO))
        *obj = iface;
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        *obj = NULL;
        return E_NOINTERFACE;
    }
    IUnknown_AddRef((IUnknown*)*obj);
    return S_OK;
}

static ULONG WINAPI enumwiadevinfo_AddRef(IEnumWIA_DEV_INFO *iface)
{
    enumwiadevinfo *This = impl_from_IEnumWIA_DEV_INFO(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%lu)\n", This, ref);
    return ref;
}

static ULONG WINAPI enumwiadevinfo_Release(IEnumWIA_DEV_INFO *iface)
{
    enumwiadevinfo *This = impl_from_IEnumWIA_DEV_INFO(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%lu)\n", This, ref);

    if (ref == 0)
        HeapFree(GetProcessHeap(), 0, This);
    return ref;
}

static HRESULT WINAPI enumwiadevinfo_Next(IEnumWIA_DEV_INFO *iface, ULONG count, IWiaPropertyStorage **elem, ULONG *fetched)
{
    enumwiadevinfo *This = impl_from_IEnumWIA_DEV_INFO(iface);

    FIXME("(%p, %ld, %p, %p): stub\n", This, count, elem, fetched);

    *fetched = 0;
    return E_NOTIMPL;
}

static HRESULT WINAPI enumwiadevinfo_Skip(IEnumWIA_DEV_INFO *iface, ULONG count)
{
    enumwiadevinfo *This = impl_from_IEnumWIA_DEV_INFO(iface);

    FIXME("(%p, %lu): stub\n", This, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI enumwiadevinfo_Reset(IEnumWIA_DEV_INFO *iface)
{
    enumwiadevinfo *This = impl_from_IEnumWIA_DEV_INFO(iface);

    FIXME("(%p): stub\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI enumwiadevinfo_Clone(IEnumWIA_DEV_INFO *iface, IEnumWIA_DEV_INFO **ret)
{
    enumwiadevinfo *This = impl_from_IEnumWIA_DEV_INFO(iface);

    FIXME("(%p, %p): stub\n", This, ret);

    return E_NOTIMPL;
}

static HRESULT WINAPI enumwiadevinfo_GetCount(IEnumWIA_DEV_INFO *iface, ULONG *count)
{
    enumwiadevinfo *This = impl_from_IEnumWIA_DEV_INFO(iface);

    FIXME("(%p, %p): stub\n", This, count);

    *count = 0;
    return E_NOTIMPL;
}

static const IEnumWIA_DEV_INFOVtbl EnumWIA_DEV_INFOVtbl =
{
    enumwiadevinfo_QueryInterface,
    enumwiadevinfo_AddRef,
    enumwiadevinfo_Release,
    enumwiadevinfo_Next,
    enumwiadevinfo_Skip,
    enumwiadevinfo_Reset,
    enumwiadevinfo_Clone,
    enumwiadevinfo_GetCount
};

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

static HRESULT WINAPI wiadevmgr_EnumDeviceInfo(IWiaDevMgr *iface, LONG flag, IEnumWIA_DEV_INFO **ret)
{
    wiadevmgr *This = impl_from_IWiaDevMgr(iface);
    enumwiadevinfo *enuminfo;

    TRACE("(%p)->(%lx, %p)\n", This, flag, ret);

    *ret = NULL;

    enuminfo = HeapAlloc(GetProcessHeap(), 0, sizeof(*enuminfo));
    if (!enuminfo)
        return E_OUTOFMEMORY;

    enuminfo->IEnumWIA_DEV_INFO_iface.lpVtbl = &EnumWIA_DEV_INFOVtbl;
    enuminfo->ref = 1;

    *ret = &enuminfo->IEnumWIA_DEV_INFO_iface;
    return S_OK;
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
    FIXME("(%p, %p, %ld, 0x%lx, %p, %p): stub\n", This, hwndParent, lDeviceType, lFlags, pbstrDeviceID, ppItemRoot);
    return E_NOTIMPL;
}

static HRESULT WINAPI wiadevmgr_SelectDeviceDlgID(IWiaDevMgr *iface, HWND hwndParent, LONG lDeviceType,
                                                  LONG lFlags, BSTR *pbstrDeviceID)
{
    wiadevmgr *This = impl_from_IWiaDevMgr(iface);
    FIXME("(%p, %p, %ld, 0x%lx, %p): stub\n", This, hwndParent, lDeviceType, lFlags, pbstrDeviceID);
    return E_NOTIMPL;
}

static HRESULT WINAPI wiadevmgr_GetImageDlg(IWiaDevMgr *iface, HWND hwndParent, LONG lDeviceType,
                                            LONG lFlags, LONG lIntent, IWiaItem *pItemRoot,
                                            BSTR bstrFilename, GUID *pguidFormat)
{
    wiadevmgr *This = impl_from_IWiaDevMgr(iface);
    FIXME("(%p, %p, %ld, 0x%lx, %ld, %p, %s, %s): stub\n", This, hwndParent, lDeviceType, lFlags,
        lIntent, pItemRoot, debugstr_w(bstrFilename), debugstr_guid(pguidFormat));
    return E_NOTIMPL;
}

static HRESULT WINAPI wiadevmgr_RegisterEventCallbackProgram(IWiaDevMgr *iface, LONG lFlags, BSTR bstrDeviceID,
                                                             const GUID *pEventGUID, BSTR bstrCommandline,
                                                             BSTR bstrName, BSTR bstrDescription, BSTR bstrIcon)
{
    wiadevmgr *This = impl_from_IWiaDevMgr(iface);
    FIXME("(%p, 0x%lx, %s, %s, %s, %s, %s, %s): stub\n", This, lFlags, debugstr_w(bstrDeviceID),
        debugstr_guid(pEventGUID), debugstr_w(bstrCommandline), debugstr_w(bstrName),
        debugstr_w(bstrDescription), debugstr_w(bstrIcon));
    return E_NOTIMPL;
}

static HRESULT WINAPI wiadevmgr_RegisterEventCallbackInterface(IWiaDevMgr *iface, LONG lFlags, BSTR bstrDeviceID,
                                                               const GUID *pEventGUID, IWiaEventCallback *pIWiaEventCallback,
                                                               IUnknown **pEventObject)
{
    wiadevmgr *This = impl_from_IWiaDevMgr(iface);
    FIXME("(%p, 0x%lx, %s, %s, %p, %p): stub\n", This, lFlags, debugstr_w(bstrDeviceID),
        debugstr_guid(pEventGUID), pIWiaEventCallback, pEventObject);
    return E_NOTIMPL;
}

static HRESULT WINAPI wiadevmgr_RegisterEventCallbackCLSID(IWiaDevMgr *iface, LONG lFlags, BSTR bstrDeviceID,
                                                           const GUID *pEventGUID, const GUID *pClsID, BSTR bstrName,
                                                           BSTR bstrDescription, BSTR bstrIcon)
{
    wiadevmgr *This = impl_from_IWiaDevMgr(iface);
    FIXME("(%p, 0x%lx, %s, %s, %s, %s, %s, %s): stub\n", This, lFlags, debugstr_w(bstrDeviceID),
        debugstr_guid(pEventGUID), debugstr_guid(pClsID), debugstr_w(bstrName),
        debugstr_w(bstrDescription), debugstr_w(bstrIcon));
    return E_NOTIMPL;
}

static HRESULT WINAPI wiadevmgr_AddDeviceDlg(IWiaDevMgr *iface, HWND hwndParent, LONG lFlags)
{
    wiadevmgr *This = impl_from_IWiaDevMgr(iface);
    FIXME("(%p, %p, 0x%lx): stub\n", This, hwndParent, lFlags);
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
