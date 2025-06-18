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
    IWSDAddress           *localAddress;
    IWSDAddress           *remoteAddress;
} IWSDMessageParametersImpl;

typedef struct IWSDUdpMessageParametersImpl {
    IWSDMessageParametersImpl base;
    WSDUdpRetransmitParams    retransmitParams;
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

static ULONG IWSDMessageParametersImpl_AddRef(IWSDMessageParameters *iface)
{
    IWSDMessageParametersImpl *This = impl_from_IWSDMessageParameters(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);
    return ref;
}

static ULONG IWSDMessageParametersImpl_Release(IWSDMessageParameters *iface)
{
    IWSDMessageParametersImpl *This = impl_from_IWSDMessageParameters(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if (ref == 0)
    {
        if (This->localAddress != NULL)
        {
            IWSDAddress_Release(This->localAddress);
        }

        if (This->remoteAddress != NULL)
        {
            IWSDAddress_Release(This->remoteAddress);
        }

        free(This);
    }

    return ref;
}

static HRESULT IWSDMessageParametersImpl_GetLocalAddress(IWSDMessageParameters *This, IWSDAddress **ppAddress)
{
    IWSDMessageParametersImpl *impl = impl_from_IWSDMessageParameters(This);

    TRACE("(%p, %p)\n", impl, ppAddress);

    if (ppAddress == NULL)
    {
        return E_POINTER;
    }

    if (impl->localAddress == NULL)
    {
        return E_ABORT;
    }

    *ppAddress = impl->localAddress;
    IWSDAddress_AddRef(*ppAddress);

    return S_OK;
}

static HRESULT IWSDMessageParametersImpl_SetLocalAddress(IWSDMessageParameters *This, IWSDAddress *pAddress)
{
    IWSDMessageParametersImpl *impl = impl_from_IWSDMessageParameters(This);

    TRACE("(%p, %p)\n", impl, pAddress);

    if (pAddress == NULL)
    {
        return E_POINTER;
    }

    if (impl->localAddress != NULL)
    {
        IWSDAddress_Release(impl->localAddress);
    }

    impl->localAddress = pAddress;
    IWSDAddress_AddRef(pAddress);

    return S_OK;
}

static HRESULT IWSDMessageParametersImpl_GetRemoteAddress(IWSDMessageParameters *This, IWSDAddress **ppAddress)
{
    IWSDMessageParametersImpl *impl = impl_from_IWSDMessageParameters(This);

    TRACE("(%p, %p)\n", impl, ppAddress);

    if (ppAddress == NULL)
    {
        return E_POINTER;
    }

    if (impl->remoteAddress == NULL)
    {
        return E_ABORT;
    }

    *ppAddress = impl->remoteAddress;
    IWSDAddress_AddRef(*ppAddress);

    return S_OK;
}

static HRESULT IWSDMessageParametersImpl_SetRemoteAddress(IWSDMessageParameters *This, IWSDAddress *pAddress)
{
    IWSDMessageParametersImpl *impl = impl_from_IWSDMessageParameters(This);

    TRACE("(%p, %p)\n", impl, pAddress);

    if (pAddress == NULL)
    {
        return E_POINTER;
    }

    if (impl->remoteAddress != NULL)
    {
        IWSDAddress_Release(impl->remoteAddress);
    }

    impl->remoteAddress = pAddress;
    IWSDAddress_AddRef(pAddress);

    return S_OK;
}

static HRESULT IWSDMessageParametersImpl_GetLowerParameters(IWSDMessageParameters *This, IWSDMessageParameters **ppTxParams)
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
    IWSDUdpMessageParametersImpl *impl = impl_from_IWSDUdpMessageParameters(This);

    TRACE("(%p, %p)\n", impl, pParams);

    if (pParams == NULL)
    {
        return E_INVALIDARG;
    }

    impl->retransmitParams = *pParams;
    return S_OK;
}

static HRESULT WINAPI IWSDUdpMessageParametersImpl_GetRetransmitParams(IWSDUdpMessageParameters *This, WSDUdpRetransmitParams *pParams)
{
    IWSDUdpMessageParametersImpl *impl = impl_from_IWSDUdpMessageParameters(This);

    TRACE("(%p, %p)\n", impl, pParams);

    if (pParams == NULL)
    {
        return E_POINTER;
    }

    * pParams = impl->retransmitParams;
    return S_OK;
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

    obj = calloc(1, sizeof(*obj));
    if (!obj) return E_OUTOFMEMORY;

    obj->base.IWSDMessageParameters_iface.lpVtbl = (IWSDMessageParametersVtbl *)&udpMsgParamsVtbl;
    obj->base.ref = 1;

    /* Populate default retransmit parameters */
    obj->retransmitParams.ulSendDelay = 0;
    obj->retransmitParams.ulRepeat = 1;
    obj->retransmitParams.ulRepeatMinDelay = 50;
    obj->retransmitParams.ulRepeatMaxDelay = 250;
    obj->retransmitParams.ulRepeatUpperDelay = 450;

    *ppTxParams = (IWSDUdpMessageParameters *)&obj->base.IWSDMessageParameters_iface;
    TRACE("Returning iface %p\n", *ppTxParams);

    return S_OK;
}
