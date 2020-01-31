/* Video For Windows Steering structure
 *
 * Copyright 2005 Maarten Lankhorst
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
 *
 */

#define COBJMACROS

#include "config.h"
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wtypes.h"
#include "wingdi.h"
#include "winuser.h"
#include "dshow.h"

#include "qcap_main.h"
#include "wine/debug.h"

#include "capture.h"
#include "uuids.h"
#include "vfwmsgs.h"
#include "amvideo.h"
#include "strmif.h"
#include "ddraw.h"
#include "ocidl.h"
#include "oleauto.h"

WINE_DEFAULT_DEBUG_CHANNEL(qcap);

typedef struct VfwCapture
{
    struct strmbase_filter filter;
    IAMStreamConfig IAMStreamConfig_iface;
    IAMVideoProcAmp IAMVideoProcAmp_iface;
    IPersistPropertyBag IPersistPropertyBag_iface;
    BOOL init;
    Capture *driver_info;

    struct strmbase_source source;
    IKsPropertySet IKsPropertySet_iface;
} VfwCapture;

static inline VfwCapture *impl_from_strmbase_filter(struct strmbase_filter *iface)
{
    return CONTAINING_RECORD(iface, VfwCapture, filter);
}

static inline VfwCapture *impl_from_IAMStreamConfig(IAMStreamConfig *iface)
{
    return CONTAINING_RECORD(iface, VfwCapture, IAMStreamConfig_iface);
}

static inline VfwCapture *impl_from_IAMVideoProcAmp(IAMVideoProcAmp *iface)
{
    return CONTAINING_RECORD(iface, VfwCapture, IAMVideoProcAmp_iface);
}

static inline VfwCapture *impl_from_IPersistPropertyBag(IPersistPropertyBag *iface)
{
    return CONTAINING_RECORD(iface, VfwCapture, IPersistPropertyBag_iface);
}

static struct strmbase_pin *vfw_capture_get_pin(struct strmbase_filter *iface, unsigned int index)
{
    VfwCapture *This = impl_from_strmbase_filter(iface);

    if (index >= 1)
        return NULL;

    return &This->source.pin;
}

static void vfw_capture_destroy(struct strmbase_filter *iface)
{
    VfwCapture *filter = impl_from_strmbase_filter(iface);

    if (filter->init)
    {
        if (filter->filter.state != State_Stopped)
            qcap_driver_stop_stream(filter->driver_info);
        qcap_driver_destroy(filter->driver_info);
    }

    if (filter->source.pin.peer)
    {
        IPin_Disconnect(filter->source.pin.peer);
        IPin_Disconnect(&filter->source.pin.IPin_iface);
    }
    strmbase_source_cleanup(&filter->source);
    strmbase_filter_cleanup(&filter->filter);
    CoTaskMemFree(filter);
    ObjectRefCount(FALSE);
}

static HRESULT vfw_capture_query_interface(struct strmbase_filter *iface, REFIID iid, void **out)
{
    VfwCapture *filter = impl_from_strmbase_filter(iface);

    if (IsEqualGUID(iid, &IID_IPersistPropertyBag))
        *out = &filter->IPersistPropertyBag_iface;
    else if (IsEqualGUID(iid, &IID_IAMVideoProcAmp))
        *out = &filter->IAMVideoProcAmp_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static HRESULT vfw_capture_init_stream(struct strmbase_filter *iface)
{
    VfwCapture *filter = impl_from_strmbase_filter(iface);

    qcap_driver_init_stream(filter->driver_info);
    return VFW_S_CANT_CUE;
}

static HRESULT vfw_capture_start_stream(struct strmbase_filter *iface, REFERENCE_TIME time)
{
    VfwCapture *filter = impl_from_strmbase_filter(iface);

    qcap_driver_start_stream(filter->driver_info);
    return S_OK;
}

static HRESULT vfw_capture_stop_stream(struct strmbase_filter *iface)
{
    VfwCapture *filter = impl_from_strmbase_filter(iface);

    qcap_driver_stop_stream(filter->driver_info);
    return VFW_S_CANT_CUE;
}

static HRESULT vfw_capture_cleanup_stream(struct strmbase_filter *iface)
{
    VfwCapture *filter = impl_from_strmbase_filter(iface);

    qcap_driver_cleanup_stream(filter->driver_info);
    return S_OK;
}

static const struct strmbase_filter_ops filter_ops =
{
    .filter_get_pin = vfw_capture_get_pin,
    .filter_destroy = vfw_capture_destroy,
    .filter_query_interface = vfw_capture_query_interface,
    .filter_init_stream = vfw_capture_init_stream,
    .filter_start_stream = vfw_capture_start_stream,
    .filter_stop_stream = vfw_capture_stop_stream,
    .filter_cleanup_stream = vfw_capture_cleanup_stream,
};

/* AMStreamConfig interface, we only need to implement {G,S}etFormat */
static HRESULT WINAPI AMStreamConfig_QueryInterface(IAMStreamConfig *iface, REFIID iid, void **out)
{
    VfwCapture *filter = impl_from_IAMStreamConfig(iface);
    return IPin_QueryInterface(&filter->source.pin.IPin_iface, iid, out);
}

static ULONG WINAPI AMStreamConfig_AddRef(IAMStreamConfig *iface)
{
    VfwCapture *filter = impl_from_IAMStreamConfig(iface);
    return IPin_AddRef(&filter->source.pin.IPin_iface);
}

static ULONG WINAPI AMStreamConfig_Release(IAMStreamConfig *iface)
{
    VfwCapture *filter = impl_from_IAMStreamConfig(iface);
    return IPin_Release(&filter->source.pin.IPin_iface);
}

static HRESULT WINAPI
AMStreamConfig_SetFormat(IAMStreamConfig *iface, AM_MEDIA_TYPE *pmt)
{
    HRESULT hr;
    VfwCapture *This = impl_from_IAMStreamConfig(iface);

    TRACE("filter %p, mt %p.\n", This, pmt);
    strmbase_dump_media_type(pmt);

    if (This->filter.state != State_Stopped)
    {
        TRACE("Returning not stopped error\n");
        return VFW_E_NOT_STOPPED;
    }

    if (!pmt)
    {
        TRACE("pmt is NULL\n");
        return E_POINTER;
    }

    if (This->source.pin.peer)
    {
        hr = IPin_QueryAccept(This->source.pin.peer, pmt);
        TRACE("Would accept: %d\n", hr);
        if (hr == S_FALSE)
            return VFW_E_INVALIDMEDIATYPE;
    }

    hr = qcap_driver_set_format(This->driver_info, pmt);
    if (SUCCEEDED(hr) && This->filter.graph && This->source.pin.peer)
    {
        hr = IFilterGraph_Reconnect(This->filter.graph, &This->source.pin.IPin_iface);
        if (SUCCEEDED(hr))
            TRACE("Reconnection completed, with new media format..\n");
    }
    TRACE("Returning: %d\n", hr);
    return hr;
}

static HRESULT WINAPI
AMStreamConfig_GetFormat( IAMStreamConfig *iface, AM_MEDIA_TYPE **pmt )
{
    VfwCapture *This = impl_from_IAMStreamConfig(iface);
    HRESULT hr;

    TRACE("%p -> (%p)\n", iface, pmt);
    hr = qcap_driver_get_format(This->driver_info, pmt);
    if (SUCCEEDED(hr))
        strmbase_dump_media_type(*pmt);
    return hr;
}

static HRESULT WINAPI
AMStreamConfig_GetNumberOfCapabilities( IAMStreamConfig *iface, int *piCount,
                                        int *piSize )
{
    FIXME("%p: %p %p - stub, intentional\n", iface, piCount, piSize);
    *piCount = 0;
    return E_NOTIMPL; /* Not implemented for this interface */
}

static HRESULT WINAPI
AMStreamConfig_GetStreamCaps( IAMStreamConfig *iface, int iIndex,
                              AM_MEDIA_TYPE **pmt, BYTE *pSCC )
{
    FIXME("%p: %d %p %p - stub, intentional\n", iface, iIndex, pmt, pSCC);
    return E_NOTIMPL; /* Not implemented for this interface */
}

static const IAMStreamConfigVtbl IAMStreamConfig_VTable =
{
    AMStreamConfig_QueryInterface,
    AMStreamConfig_AddRef,
    AMStreamConfig_Release,
    AMStreamConfig_SetFormat,
    AMStreamConfig_GetFormat,
    AMStreamConfig_GetNumberOfCapabilities,
    AMStreamConfig_GetStreamCaps
};

static HRESULT WINAPI AMVideoProcAmp_QueryInterface(IAMVideoProcAmp *iface, REFIID riid,
        void **ret_iface)
{
    VfwCapture *This = impl_from_IAMVideoProcAmp(iface);

    return IUnknown_QueryInterface(This->filter.outer_unk, riid, ret_iface);
}

static ULONG WINAPI AMVideoProcAmp_AddRef(IAMVideoProcAmp * iface)
{
    VfwCapture *This = impl_from_IAMVideoProcAmp(iface);

    return IUnknown_AddRef(This->filter.outer_unk);
}

static ULONG WINAPI AMVideoProcAmp_Release(IAMVideoProcAmp * iface)
{
    VfwCapture *This = impl_from_IAMVideoProcAmp(iface);

    return IUnknown_Release(This->filter.outer_unk);
}

static HRESULT WINAPI
AMVideoProcAmp_GetRange( IAMVideoProcAmp * iface, LONG Property, LONG *pMin,
        LONG *pMax, LONG *pSteppingDelta, LONG *pDefault, LONG *pCapsFlags )
{
    VfwCapture *This = impl_from_IAMVideoProcAmp(iface);

    return qcap_driver_get_prop_range( This->driver_info, Property, pMin, pMax,
                   pSteppingDelta, pDefault, pCapsFlags );
}

static HRESULT WINAPI
AMVideoProcAmp_Set( IAMVideoProcAmp * iface, LONG Property, LONG lValue,
                    LONG Flags )
{
    VfwCapture *This = impl_from_IAMVideoProcAmp(iface);

    return qcap_driver_set_prop(This->driver_info, Property, lValue, Flags);
}

static HRESULT WINAPI
AMVideoProcAmp_Get( IAMVideoProcAmp * iface, LONG Property, LONG *lValue,
                    LONG *Flags )
{
    VfwCapture *This = impl_from_IAMVideoProcAmp(iface);

    return qcap_driver_get_prop(This->driver_info, Property, lValue, Flags);
}

static const IAMVideoProcAmpVtbl IAMVideoProcAmp_VTable =
{
    AMVideoProcAmp_QueryInterface,
    AMVideoProcAmp_AddRef,
    AMVideoProcAmp_Release,
    AMVideoProcAmp_GetRange,
    AMVideoProcAmp_Set,
    AMVideoProcAmp_Get,
};

static HRESULT WINAPI PPB_QueryInterface(IPersistPropertyBag *iface, REFIID riid, void **ret_iface)
{
    VfwCapture *This = impl_from_IPersistPropertyBag(iface);

    return IUnknown_QueryInterface(This->filter.outer_unk, riid, ret_iface);
}

static ULONG WINAPI PPB_AddRef(IPersistPropertyBag * iface)
{
    VfwCapture *This = impl_from_IPersistPropertyBag(iface);

    return IUnknown_AddRef(This->filter.outer_unk);
}

static ULONG WINAPI PPB_Release(IPersistPropertyBag * iface)
{
    VfwCapture *This = impl_from_IPersistPropertyBag(iface);

    return IUnknown_Release(This->filter.outer_unk);
}

static HRESULT WINAPI
PPB_GetClassID( IPersistPropertyBag * iface, CLSID * pClassID )
{
    VfwCapture *This = impl_from_IPersistPropertyBag(iface);

    FIXME("%p - stub\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI PPB_InitNew(IPersistPropertyBag * iface)
{
    VfwCapture *This = impl_from_IPersistPropertyBag(iface);

    FIXME("%p - stub\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI
PPB_Load( IPersistPropertyBag * iface, IPropertyBag *pPropBag,
          IErrorLog *pErrorLog )
{
    static const OLECHAR VFWIndex[] = {'V','F','W','I','n','d','e','x',0};
    VfwCapture *This = impl_from_IPersistPropertyBag(iface);
    HRESULT hr;
    VARIANT var;

    TRACE("%p/%p-> (%p, %p)\n", iface, This, pPropBag, pErrorLog);

    V_VT(&var) = VT_I4;
    hr = IPropertyBag_Read(pPropBag, VFWIndex, &var, pErrorLog);

    if (SUCCEEDED(hr))
    {
        This->driver_info = qcap_driver_init(&This->source, V_I4(&var));
        if (This->driver_info)
        {
            This->init = TRUE;
            hr = S_OK;
        }
        else
            hr = E_FAIL;
    }

    return hr;
}

static HRESULT WINAPI
PPB_Save( IPersistPropertyBag * iface, IPropertyBag *pPropBag,
          BOOL fClearDirty, BOOL fSaveAllProperties )
{
    VfwCapture *This = impl_from_IPersistPropertyBag(iface);
    FIXME("%p - stub\n", This);
    return E_NOTIMPL;
}

static const IPersistPropertyBagVtbl IPersistPropertyBag_VTable =
{
    PPB_QueryInterface,
    PPB_AddRef,
    PPB_Release,
    PPB_GetClassID,
    PPB_InitNew,
    PPB_Load,
    PPB_Save
};

/* IKsPropertySet interface */
static inline VfwCapture *impl_from_IKsPropertySet(IKsPropertySet *iface)
{
    return CONTAINING_RECORD(iface, VfwCapture, IKsPropertySet_iface);
}

static HRESULT WINAPI KSP_QueryInterface(IKsPropertySet * iface, REFIID riid, void **ret_iface)
{
    VfwCapture *filter = impl_from_IKsPropertySet(iface);
    return IPin_QueryInterface(&filter->source.pin.IPin_iface, riid, ret_iface);
}

static ULONG WINAPI KSP_AddRef(IKsPropertySet * iface)
{
    VfwCapture *filter = impl_from_IKsPropertySet(iface);
    return IPin_AddRef(&filter->source.pin.IPin_iface);
}

static ULONG WINAPI KSP_Release(IKsPropertySet * iface)
{
    VfwCapture *filter = impl_from_IKsPropertySet(iface);
    return IPin_Release(&filter->source.pin.IPin_iface);
}

static HRESULT WINAPI
KSP_Set( IKsPropertySet * iface, REFGUID guidPropSet, DWORD dwPropID,
         LPVOID pInstanceData, DWORD cbInstanceData, LPVOID pPropData,
         DWORD cbPropData )
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI
KSP_Get( IKsPropertySet * iface, REFGUID guidPropSet, DWORD dwPropID,
         LPVOID pInstanceData, DWORD cbInstanceData, LPVOID pPropData,
         DWORD cbPropData, DWORD *pcbReturned )
{
    LPGUID pGuid;

    TRACE("()\n");

    if (!IsEqualIID(guidPropSet, &AMPROPSETID_Pin))
        return E_PROP_SET_UNSUPPORTED;
    if (pPropData == NULL && pcbReturned == NULL)
        return E_POINTER;
    if (pcbReturned)
        *pcbReturned = sizeof(GUID);
    if (pPropData == NULL)
        return S_OK;
    if (cbPropData < sizeof(GUID))
        return E_UNEXPECTED;
    pGuid = pPropData;
    *pGuid = PIN_CATEGORY_CAPTURE;
    FIXME("() Not adding a pin with PIN_CATEGORY_PREVIEW\n");
    return S_OK;
}

static HRESULT WINAPI
KSP_QuerySupported( IKsPropertySet * iface, REFGUID guidPropSet,
                    DWORD dwPropID, DWORD *pTypeSupport )
{
   FIXME("%p: stub\n", iface);
   return E_NOTIMPL;
}

static const IKsPropertySetVtbl IKsPropertySet_VTable =
{
   KSP_QueryInterface,
   KSP_AddRef,
   KSP_Release,
   KSP_Set,
   KSP_Get,
   KSP_QuerySupported
};

static inline VfwCapture *impl_from_strmbase_pin(struct strmbase_pin *pin)
{
    return CONTAINING_RECORD(pin, VfwCapture, source.pin);
}

static HRESULT source_query_accept(struct strmbase_pin *pin, const AM_MEDIA_TYPE *mt)
{
    VfwCapture *filter = impl_from_strmbase_pin(pin);
    return qcap_driver_check_format(filter->driver_info, mt);
}

static HRESULT source_get_media_type(struct strmbase_pin *pin,
        unsigned int iPosition, AM_MEDIA_TYPE *pmt)
{
    VfwCapture *filter = impl_from_strmbase_pin(pin);
    AM_MEDIA_TYPE *vfw_pmt;
    HRESULT hr;

    if (iPosition > 0)
        return VFW_S_NO_MORE_ITEMS;

    hr = qcap_driver_get_format(filter->driver_info, &vfw_pmt);
    if (SUCCEEDED(hr)) {
        CopyMediaType(pmt, vfw_pmt);
        DeleteMediaType(vfw_pmt);
    }
    return hr;
}

static HRESULT source_query_interface(struct strmbase_pin *iface, REFIID iid, void **out)
{
    VfwCapture *filter = impl_from_strmbase_pin(iface);

    if (IsEqualGUID(iid, &IID_IKsPropertySet))
        *out = &filter->IKsPropertySet_iface;
    else if (IsEqualGUID(iid, &IID_IAMStreamConfig))
        *out = &filter->IAMStreamConfig_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static HRESULT WINAPI VfwPin_DecideBufferSize(struct strmbase_source *iface,
        IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *ppropInputRequest)
{
    ALLOCATOR_PROPERTIES actual;

    /* What we put here doesn't matter, the
       driver function should override it then commit */
    if (!ppropInputRequest->cBuffers)
        ppropInputRequest->cBuffers = 3;
    if (!ppropInputRequest->cbBuffer)
        ppropInputRequest->cbBuffer = 230400;
    if (!ppropInputRequest->cbAlign)
        ppropInputRequest->cbAlign = 1;

    return IMemAllocator_SetProperties(pAlloc, ppropInputRequest, &actual);
}

static const struct strmbase_source_ops source_ops =
{
    .base.pin_query_accept = source_query_accept,
    .base.pin_get_media_type = source_get_media_type,
    .base.pin_query_interface = source_query_interface,
    .pfnAttemptConnection = BaseOutputPinImpl_AttemptConnection,
    .pfnDecideBufferSize = VfwPin_DecideBufferSize,
    .pfnDecideAllocator = BaseOutputPinImpl_DecideAllocator,
};

IUnknown * WINAPI QCAP_createVFWCaptureFilter(IUnknown *outer, HRESULT *phr)
{
    static const WCHAR source_name[] = {'O','u','t','p','u','t',0};
    VfwCapture *object;

    if (!(object = CoTaskMemAlloc(sizeof(*object))))
    {
        *phr = E_OUTOFMEMORY;
        return NULL;
    }

    strmbase_filter_init(&object->filter, outer, &CLSID_VfwCapture, &filter_ops);

    object->IAMStreamConfig_iface.lpVtbl = &IAMStreamConfig_VTable;
    object->IAMVideoProcAmp_iface.lpVtbl = &IAMVideoProcAmp_VTable;
    object->IPersistPropertyBag_iface.lpVtbl = &IPersistPropertyBag_VTable;
    object->init = FALSE;

    strmbase_source_init(&object->source, &object->filter, source_name, &source_ops);

    object->IKsPropertySet_iface.lpVtbl = &IKsPropertySet_VTable;

    TRACE("Created VFW capture filter %p.\n", object);
    ObjectRefCount(TRUE);
    *phr = S_OK;
    return &object->filter.IUnknown_inner;
}
