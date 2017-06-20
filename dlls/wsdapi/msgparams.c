/*
 * Web Services on Devices
 *
 * Copyright 2017 Owen Rudge for CodeWeavers
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

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"
#include "wsdapi.h"

WINE_DEFAULT_DEBUG_CHANNEL(wsdapi);

typedef struct IWSDMessageParametersImpl {
    IWSDMessageParameters IWSDMessageParameters_iface;
    LONG                  ref;
} IWSDMessageParametersImpl;

typedef struct IWSDUdpMessageParametersImpl {
    IWSDMessageParametersImpl base;
} IWSDUdpMessageParametersImpl;

static inline IWSDMessageParametersImpl *impl_from_IWSDMessageParameters(IWSDMessageParameters *iface)
{
    return CONTAINING_RECORD(iface, IWSDMessageParametersImpl, IWSDMessageParameters_iface);
}

static inline IWSDUdpMessageParametersImpl *impl_from_IWSDUdpMessageParameters(IWSDUdpMessageParameters *iface)
{
    return CONTAINING_RECORD(iface, IWSDUdpMessageParametersImpl, base.IWSDMessageParameters_iface);
}

/* IWSDMessageParameters implementation */

static ULONG WINAPI IWSDMessageParametersImpl_AddRef(IWSDMessageParameters *iface)
{
    IWSDMessageParametersImpl *This = impl_from_IWSDMessageParameters(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);
    return ref;
}

static ULONG WINAPI IWSDMessageParametersImpl_Release(IWSDMessageParameters *iface)
{
    IWSDMessageParametersImpl *This = impl_from_IWSDMessageParameters(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if (ref == 0)
    {
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI IWSDMessageParametersImpl_GetLocalAddress(IWSDMessageParameters *This, IWSDAddress **ppAddress)
{
    FIXME("(%p, %p)\n", This, ppAddress);
    return E_NOTIMPL;
}

static HRESULT WINAPI IWSDMessageParametersImpl_SetLocalAddress(IWSDMessageParameters *This, IWSDAddress *pAddress)
{
    FIXME("(%p, %p)\n", This, pAddress);
    return E_NOTIMPL;
}

static HRESULT WINAPI IWSDMessageParametersImpl_GetRemoteAddress(IWSDMessageParameters *This, IWSDAddress **ppAddress)
{
    FIXME("(%p, %p)\n", This, ppAddress);
    return E_NOTIMPL;
}

static HRESULT WINAPI IWSDMessageParametersImpl_SetRemoteAddress(IWSDMessageParameters *This, IWSDAddress *pAddress)
{
    FIXME("(%p, %p)\n", This, pAddress);
    return E_NOTIMPL;
}

static HRESULT WINAPI IWSDMessageParametersImpl_GetLowerParameters(IWSDMessageParameters *This, IWSDMessageParameters **ppTxParams)
{
    FIXME("(%p, %p)\n", This, ppTxParams);
    return E_NOTIMPL;
}

/* IWSDUdpMessageParameters implementation */
static HRESULT WINAPI IWSDUdpMessageParametersImpl_QueryInterface(IWSDUdpMessageParameters *iface, REFIID riid, void **ppv)
{
    IWSDUdpMessageParametersImpl *This = impl_from_IWSDUdpMessageParameters(iface);

    TRACE("(%p, %s, %p)\n", This, debugstr_guid(riid), ppv);

    if (!ppv)
    {
        WARN("Invalid parameter\n");
        return E_INVALIDARG;
    }

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IWSDMessageParameters) ||
        IsEqualIID(riid, &IID_IWSDUdpMessageParameters))
    {
        *ppv = &This->base.IWSDMessageParameters_iface;
    }
    else
    {
        WARN("Unknown IID %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI IWSDUdpMessageParametersImpl_AddRef(IWSDUdpMessageParameters *iface)
{
    return IWSDMessageParametersImpl_AddRef((IWSDMessageParameters *)iface);
}

static ULONG WINAPI IWSDUdpMessageParametersImpl_Release(IWSDUdpMessageParameters *iface)
{
    return IWSDMessageParametersImpl_Release((IWSDMessageParameters *)iface);
}

static HRESULT WINAPI IWSDUdpMessageParametersImpl_GetLocalAddress(IWSDUdpMessageParameters *This, IWSDAddress **ppAddress)
{
    return IWSDMessageParametersImpl_GetLocalAddress((IWSDMessageParameters *)This, ppAddress);
}

static HRESULT WINAPI IWSDUdpMessageParametersImpl_SetLocalAddress(IWSDUdpMessageParameters *This, IWSDAddress *pAddress)
{
    return IWSDMessageParametersImpl_SetLocalAddress((IWSDMessageParameters *)This, pAddress);
}

static HRESULT WINAPI IWSDUdpMessageParametersImpl_GetRemoteAddress(IWSDUdpMessageParameters *This, IWSDAddress **ppAddress)
{
    return IWSDMessageParametersImpl_GetRemoteAddress((IWSDMessageParameters *)This, ppAddress);
}

static HRESULT WINAPI IWSDUdpMessageParametersImpl_SetRemoteAddress(IWSDUdpMessageParameters *This, IWSDAddress *pAddress)
{
    return IWSDMessageParametersImpl_SetRemoteAddress((IWSDMessageParameters *)This, pAddress);
}

static HRESULT WINAPI IWSDUdpMessageParametersImpl_GetLowerParameters(IWSDUdpMessageParameters *This, IWSDMessageParameters **ppTxParams)
{
    return IWSDMessageParametersImpl_GetLowerParameters((IWSDMessageParameters *)This, ppTxParams);
}

static HRESULT WINAPI IWSDUdpMessageParametersImpl_SetRetransmitParams(IWSDUdpMessageParameters *This, const WSDUdpRetransmitParams *pParams)
{
    FIXME("(%p, %p)\n", This, pParams);
    return E_NOTIMPL;
}

static HRESULT WINAPI IWSDUdpMessageParametersImpl_GetRetransmitParams(IWSDUdpMessageParameters *This, WSDUdpRetransmitParams *pParams)
{
    FIXME("(%p, %p)\n", This, pParams);
    return E_NOTIMPL;
}

static const IWSDUdpMessageParametersVtbl udpMsgParamsVtbl =
{
    /* IUnknown */
    IWSDUdpMessageParametersImpl_QueryInterface,
    IWSDUdpMessageParametersImpl_AddRef,
    IWSDUdpMessageParametersImpl_Release,

    /* IWSDMessageParameters */
    IWSDUdpMessageParametersImpl_GetLocalAddress,
    IWSDUdpMessageParametersImpl_SetLocalAddress,
    IWSDUdpMessageParametersImpl_GetRemoteAddress,
    IWSDUdpMessageParametersImpl_SetRemoteAddress,
    IWSDUdpMessageParametersImpl_GetLowerParameters,

    /* IWSDUdpMessageParameters */
    IWSDUdpMessageParametersImpl_SetRetransmitParams,
    IWSDUdpMessageParametersImpl_GetRetransmitParams
};

HRESULT WINAPI WSDCreateUdpMessageParameters(IWSDUdpMessageParameters **ppTxParams)
{
    IWSDUdpMessageParametersImpl *obj;

    TRACE("(%p)\n", ppTxParams);

    if (ppTxParams == NULL)
    {
        WARN("Invalid parameter: ppTxParams == NULL\n");
        return E_POINTER;
    }

    *ppTxParams = NULL;

    obj = HeapAlloc(GetProcessHeap(), 0, sizeof(*obj));
    if (!obj) return E_OUTOFMEMORY;

    obj->base.IWSDMessageParameters_iface.lpVtbl = (IWSDMessageParametersVtbl *)&udpMsgParamsVtbl;
    obj->base.ref = 1;

    *ppTxParams = (IWSDUdpMessageParameters *)&obj->base.IWSDMessageParameters_iface;
    TRACE("Returning iface %p\n", *ppTxParams);

    return S_OK;
}
