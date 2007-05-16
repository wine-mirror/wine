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

#define NONAMELESSSTRUCT
#define NONAMELESSUNION
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

#include "pin.h"
#include "capture.h"
#include "uuids.h"
#include "vfwmsgs.h"
#include "amvideo.h"
#include "strmif.h"
#include "ddraw.h"
#include "ocidl.h"
#include "oleauto.h"

WINE_DEFAULT_DEBUG_CHANNEL(qcap);

#define ICOM_THIS_MULTI(impl,field,iface) \
    impl* const This=(impl*)((char*)(iface) - offsetof(impl,field))

static const IBaseFilterVtbl VfwCapture_Vtbl;
static const IAMStreamConfigVtbl IAMStreamConfig_VTable;
static const IAMVideoProcAmpVtbl IAMVideoProcAmp_VTable;
static const IPersistPropertyBagVtbl IPersistPropertyBag_VTable;
static const IPinVtbl VfwPin_Vtbl;

static HRESULT VfwPin_Construct( IBaseFilter *, LPCRITICAL_SECTION, IPin ** );

typedef struct VfwCapture
{
    const IBaseFilterVtbl * lpVtbl;
    const IAMStreamConfigVtbl * IAMStreamConfig_vtbl;
    const IAMVideoProcAmpVtbl * IAMVideoProcAmp_vtbl;
    const IPersistPropertyBagVtbl * IPersistPropertyBag_vtbl;

    BOOL init;
    Capture *driver_info;
    LONG refCount;
    FILTER_INFO filterInfo;
    FILTER_STATE state;
    CRITICAL_SECTION csFilter;

    IPin * pOutputPin;
} VfwCapture;

/* VfwPin implementation */
typedef struct VfwPinImpl
{
    OutputPin pin;
    Capture *driver_info;
    const IKsPropertySetVtbl * KSP_VT;
} VfwPinImpl;


IUnknown * WINAPI QCAP_createVFWCaptureFilter(IUnknown *pUnkOuter, HRESULT *phr)
{
    VfwCapture *pVfwCapture;
    HRESULT hr;

    TRACE("%p - %p\n", pUnkOuter, phr);

    *phr = CLASS_E_NOAGGREGATION;
    if (pUnkOuter)
        return NULL;
    *phr = E_OUTOFMEMORY;

    pVfwCapture = CoTaskMemAlloc( sizeof(VfwCapture) );

    if (!pVfwCapture)
        return NULL;

    pVfwCapture->lpVtbl = &VfwCapture_Vtbl;
    pVfwCapture->IAMStreamConfig_vtbl = &IAMStreamConfig_VTable;
    pVfwCapture->IAMVideoProcAmp_vtbl = &IAMVideoProcAmp_VTable;
    pVfwCapture->IPersistPropertyBag_vtbl = &IPersistPropertyBag_VTable;
    pVfwCapture->refCount = 1;
    pVfwCapture->filterInfo.achName[0] = '\0';
    pVfwCapture->filterInfo.pGraph = NULL;
    pVfwCapture->state = State_Stopped;
    pVfwCapture->init = FALSE;
    InitializeCriticalSection(&pVfwCapture->csFilter);
    pVfwCapture->csFilter.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": VfwCapture.csFilter");
    hr = VfwPin_Construct((IBaseFilter *)&pVfwCapture->lpVtbl,
                   &pVfwCapture->csFilter, &pVfwCapture->pOutputPin);
    if (!SUCCEEDED(hr))
    {
        CoTaskMemFree(pVfwCapture);
        return NULL;
    }
    TRACE("-- created at %p\n", pVfwCapture);

    ObjectRefCount(TRUE);
    *phr = S_OK;
    return (IUnknown *)pVfwCapture;
}

static HRESULT WINAPI VfwCapture_QueryInterface(IBaseFilter * iface, REFIID riid, LPVOID * ppv)
{
    VfwCapture *This = (VfwCapture *)iface;
    TRACE("(%s, %p)\n", debugstr_guid(riid), ppv);
    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IPersist) ||
        IsEqualIID(riid, &IID_IMediaFilter) ||
        IsEqualIID(riid, &IID_IBaseFilter))
    {
        *ppv = This;
    }
    else if (IsEqualIID(riid, &IID_IAMStreamConfig))
        *ppv = &(This->IAMStreamConfig_vtbl);
    else if (IsEqualIID(riid, &IID_IAMVideoProcAmp))
        *ppv = &(This->IAMVideoProcAmp_vtbl);
    else if (IsEqualIID(riid, &IID_IPersistPropertyBag))
        *ppv = &(This->IPersistPropertyBag_vtbl);

    if (!IsEqualIID(riid, &IID_IUnknown) &&
        !IsEqualIID(riid, &IID_IPersist) &&
        !IsEqualIID(riid, &IID_IPersistPropertyBag) &&
        !This->init)
    {
        FIXME("Capture system not initialised when looking for %s, "
              "trying it on primary device now\n", debugstr_guid(riid));
        This->driver_info = qcap_driver_init( This->pOutputPin, 0 );
        if (!This->driver_info)
        {
            ERR("VfwCapture initialisation failed\n");
            return E_UNEXPECTED;
        }
        This->init = TRUE;
    }

    if (*ppv)
    {
        TRACE("Returning %s interface\n", debugstr_guid(riid));
        IUnknown_AddRef((IUnknown *)(*ppv));
        return S_OK;
    }

    FIXME("No interface for %s!\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI VfwCapture_AddRef(IBaseFilter * iface)
{
    VfwCapture *This = (VfwCapture *)iface;
    ULONG refCount = InterlockedIncrement(&This->refCount);

    TRACE("%p->() New refcount: %d\n", This, refCount);

    return refCount;
}

static ULONG WINAPI VfwCapture_Release(IBaseFilter * iface)
{
    VfwCapture *This = (VfwCapture *)iface;
    ULONG refCount = InterlockedDecrement(&This->refCount);

    TRACE("%p->() New refcount: %d\n", This, refCount);

    if (!refCount)
    {
        IPinImpl *pin;

        TRACE("destroying everything\n");
        if (This->init)
        {
            if (This->state != State_Stopped)
                qcap_driver_stop(This->driver_info, &This->state);
            qcap_driver_destroy(This->driver_info);
        }
        pin = (IPinImpl*) This->pOutputPin;
        if (pin->pConnectedTo != NULL)
        {
            IPin_Disconnect(pin->pConnectedTo);
            IPin_Disconnect(This->pOutputPin);
        }
        IPin_Release(This->pOutputPin);
        This->csFilter.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->csFilter);
        This->lpVtbl = NULL;
        CoTaskMemFree(This);
        ObjectRefCount(FALSE);
    }
    return refCount;
}

/** IPersist methods **/

static HRESULT WINAPI VfwCapture_GetClassID(IBaseFilter * iface, CLSID * pClsid)
{
    TRACE("(%p)\n", pClsid);
    *pClsid = CLSID_VfwCapture;
    return S_OK;
}

/** IMediaFilter methods **/

static HRESULT WINAPI VfwCapture_Stop(IBaseFilter * iface)
{
    VfwCapture *This = (VfwCapture *)iface;

    TRACE("()\n");
    return qcap_driver_stop(This->driver_info, &This->state);
}

static HRESULT WINAPI VfwCapture_Pause(IBaseFilter * iface)
{
    VfwCapture *This = (VfwCapture *)iface;

    TRACE("()\n");
    return qcap_driver_pause(This->driver_info, &This->state);
}

static HRESULT WINAPI VfwCapture_Run(IBaseFilter * iface, REFERENCE_TIME tStart)
{
    VfwCapture *This = (VfwCapture *)iface;
    TRACE("(%x%08x)\n", (ULONG)(tStart >> 32), (ULONG)tStart);
    return qcap_driver_run(This->driver_info, &This->state);
}

static HRESULT WINAPI
VfwCapture_GetState( IBaseFilter * iface, DWORD dwMilliSecsTimeout,
                     FILTER_STATE *pState )
{
    VfwCapture *This = (VfwCapture *)iface;

    TRACE("(%u, %p)\n", dwMilliSecsTimeout, pState);

    *pState = This->state;
    return S_OK;
}

static HRESULT WINAPI
VfwCapture_SetSyncSource(IBaseFilter * iface, IReferenceClock *pClock)
{
    TRACE("(%p)\n", pClock);

    return S_OK;
}

static HRESULT WINAPI
VfwCapture_GetSyncSource(IBaseFilter * iface, IReferenceClock **ppClock)
{
    TRACE("(%p)\n", ppClock);

    return S_OK;
}

/** IBaseFilter methods **/

static HRESULT WINAPI
VfwCapture_EnumPins(IBaseFilter * iface, IEnumPins **ppEnum)
{
    ENUMPINDETAILS epd;
    VfwCapture *This = (VfwCapture *)iface;

    TRACE("(%p)\n", ppEnum);

    epd.cPins = 1;
    epd.ppPins = &This->pOutputPin;
    return IEnumPinsImpl_Construct(&epd, ppEnum);
}

static HRESULT WINAPI VfwCapture_FindPin(IBaseFilter * iface, LPCWSTR Id, IPin **ppPin)
{
    FIXME("(%s, %p) - stub\n", debugstr_w(Id), ppPin);
    return E_NOTIMPL;
}

static HRESULT WINAPI VfwCapture_QueryFilterInfo(IBaseFilter * iface, FILTER_INFO *pInfo)
{
    VfwCapture *This = (VfwCapture *)iface;

    TRACE("(%p)\n", pInfo);

    lstrcpyW(pInfo->achName, This->filterInfo.achName);
    pInfo->pGraph = This->filterInfo.pGraph;

    if (pInfo->pGraph)
        IFilterGraph_AddRef(pInfo->pGraph);
    return S_OK;
}

static HRESULT WINAPI
VfwCapture_JoinFilterGraph( IBaseFilter * iface, IFilterGraph *pGraph, LPCWSTR pName )
{
    VfwCapture *This = (VfwCapture *)iface;

    TRACE("(%p, %s)\n", pGraph, debugstr_w(pName));

    if (pName)
        lstrcpyW(This->filterInfo.achName, pName);
    else
        *This->filterInfo.achName = 0;
    This->filterInfo.pGraph = pGraph; /* NOTE: do NOT increase ref. count */

    return S_OK;
}

static HRESULT WINAPI
VfwCapture_QueryVendorInfo(IBaseFilter * iface, LPWSTR *pVendorInfo)
{
    FIXME("(%p) - stub\n", pVendorInfo);
    return E_NOTIMPL;
}

static const IBaseFilterVtbl VfwCapture_Vtbl =
{
    VfwCapture_QueryInterface,
    VfwCapture_AddRef,
    VfwCapture_Release,
    VfwCapture_GetClassID,
    VfwCapture_Stop,
    VfwCapture_Pause,
    VfwCapture_Run,
    VfwCapture_GetState,
    VfwCapture_SetSyncSource,
    VfwCapture_GetSyncSource,
    VfwCapture_EnumPins,
    VfwCapture_FindPin,
    VfwCapture_QueryFilterInfo,
    VfwCapture_JoinFilterGraph,
    VfwCapture_QueryVendorInfo
};

/* AMStreamConfig interface, we only need to implement {G,S}etFormat */
static HRESULT WINAPI
AMStreamConfig_QueryInterface( IAMStreamConfig * iface, REFIID riid, LPVOID * ppv )
{
    ICOM_THIS_MULTI(VfwCapture, IAMStreamConfig_vtbl, iface);

    TRACE("%p --> %s\n", This, debugstr_guid(riid));

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IAMStreamConfig))
    {
        IAMStreamConfig_AddRef(iface);
        *ppv = iface;
        return S_OK;
    }

    FIXME("No interface for iid %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI AMStreamConfig_AddRef( IAMStreamConfig * iface )
{
    ICOM_THIS_MULTI(VfwCapture, IAMStreamConfig_vtbl, iface);

    TRACE("%p --> Forwarding to VfwCapture (%p)\n", iface, This);
    return IUnknown_AddRef((IUnknown *)This);
}

static ULONG WINAPI AMStreamConfig_Release( IAMStreamConfig * iface )
{
    ICOM_THIS_MULTI(VfwCapture, IAMStreamConfig_vtbl, iface);

    TRACE("%p --> Forwarding to VfwCapture (%p)\n", iface, This);
    return IUnknown_Release((IUnknown *)This);
}

static HRESULT WINAPI
AMStreamConfig_SetFormat(IAMStreamConfig *iface, AM_MEDIA_TYPE *pmt)
{
    HRESULT hr;
    ICOM_THIS_MULTI(VfwCapture, IAMStreamConfig_vtbl, iface);
    IPinImpl *pin;

    TRACE("(%p): %p->%p\n", iface, pmt, pmt->pbFormat);

    if (This->state != State_Stopped)
    {
        TRACE("Returning not stopped error\n");
        return VFW_E_NOT_STOPPED;
    }

    dump_AM_MEDIA_TYPE(pmt);

    pin = (IPinImpl *)This->pOutputPin;
    if (pin->pConnectedTo != NULL)
    {
        hr = IPin_QueryAccept(pin->pConnectedTo, pmt);
        TRACE("Would accept: %d\n", hr);
        if (hr == S_FALSE)
            return VFW_E_INVALIDMEDIATYPE;
    }

    hr = qcap_driver_set_format(This->driver_info, pmt);
    if (SUCCEEDED(hr) && This->filterInfo.pGraph && pin->pConnectedTo )
    {
        hr = IFilterGraph_Reconnect(This->filterInfo.pGraph, This->pOutputPin);
        if (SUCCEEDED(hr))
            TRACE("Reconnection completed, with new media format..\n");
    }
    TRACE("Returning: %d\n", hr);
    return hr;
}

static HRESULT WINAPI
AMStreamConfig_GetFormat( IAMStreamConfig *iface, AM_MEDIA_TYPE **pmt )
{
    ICOM_THIS_MULTI(VfwCapture, IAMStreamConfig_vtbl, iface);

    TRACE("%p -> (%p)\n", iface, pmt);
    return qcap_driver_get_format(This->driver_info, pmt);
}

static HRESULT WINAPI
AMStreamConfig_GetNumberOfCapabilities( IAMStreamConfig *iface, int *piCount,
                                        int *piSize )
{
    FIXME("%p: %p %p - stub, intentional\n", iface, piCount, piSize);
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

static HRESULT WINAPI
AMVideoProcAmp_QueryInterface( IAMVideoProcAmp * iface, REFIID riid,
                               LPVOID * ppv )
{
    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IAMVideoProcAmp))
    {
        *ppv = iface;
        IAMVideoProcAmp_AddRef( iface );
        return S_OK;
    }

    FIXME("No interface for iid %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI AMVideoProcAmp_AddRef(IAMVideoProcAmp * iface)
{
    ICOM_THIS_MULTI(VfwCapture, IAMVideoProcAmp_vtbl, iface);

    return IUnknown_AddRef((IUnknown *)This);
}

static ULONG WINAPI AMVideoProcAmp_Release(IAMVideoProcAmp * iface)
{
    ICOM_THIS_MULTI(VfwCapture, IAMVideoProcAmp_vtbl, iface);

    return IUnknown_Release((IUnknown *)This);
}

static HRESULT WINAPI
AMVideoProcAmp_GetRange( IAMVideoProcAmp * iface, long Property, long *pMin,
        long *pMax, long *pSteppingDelta, long *pDefault, long *pCapsFlags )
{
    ICOM_THIS_MULTI(VfwCapture, IAMVideoProcAmp_vtbl, iface);

    return qcap_driver_get_prop_range( This->driver_info, Property, pMin, pMax,
                   pSteppingDelta, pDefault, pCapsFlags );
}

static HRESULT WINAPI
AMVideoProcAmp_Set( IAMVideoProcAmp * iface, long Property, long lValue,
                    long Flags )
{
    ICOM_THIS_MULTI(VfwCapture, IAMVideoProcAmp_vtbl, iface);

    return qcap_driver_set_prop(This->driver_info, Property, lValue, Flags);
}

static HRESULT WINAPI
AMVideoProcAmp_Get( IAMVideoProcAmp * iface, long Property, long *lValue,
                    long *Flags )
{
    ICOM_THIS_MULTI(VfwCapture, IAMVideoProcAmp_vtbl, iface);

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

static HRESULT WINAPI
PPB_QueryInterface( IPersistPropertyBag * iface, REFIID riid, LPVOID * ppv )
{
    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IPersist) ||
        IsEqualIID(riid, &IID_IPersistPropertyBag))
    {
        IPersistPropertyBag_AddRef(iface);
        *ppv = iface;
        return S_OK;
    }
    if (IsEqualIID(riid, &IID_IBaseFilter))
    {
        /* FIXME: native devenum asks for IBaseFilter, should we return it? */
        IPersistPropertyBag_AddRef(iface);
        *ppv = iface;
        return S_OK;
    }

    FIXME("No interface for iid %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI PPB_AddRef(IPersistPropertyBag * iface)
{
    ICOM_THIS_MULTI(VfwCapture, IPersistPropertyBag_vtbl, iface);

    TRACE("%p --> Forwarding to VfwCapture (%p)\n", iface, This);

    return IUnknown_AddRef((IUnknown *)This);
}

static ULONG WINAPI PPB_Release(IPersistPropertyBag * iface)
{
    ICOM_THIS_MULTI(VfwCapture, IPersistPropertyBag_vtbl, iface);

    TRACE("%p --> Forwarding to VfwCapture (%p)\n", iface, This);

    return IUnknown_Release((IUnknown *)This);
}

static HRESULT WINAPI
PPB_GetClassID( IPersistPropertyBag * iface, CLSID * pClassID )
{
    ICOM_THIS_MULTI(VfwCapture, IPersistPropertyBag_vtbl, iface);

    FIXME("%p - stub\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI PPB_InitNew(IPersistPropertyBag * iface)
{
    ICOM_THIS_MULTI(VfwCapture, IPersistPropertyBag_vtbl, iface);

    FIXME("%p - stub\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI
PPB_Load( IPersistPropertyBag * iface, IPropertyBag *pPropBag,
          IErrorLog *pErrorLog )
{
    ICOM_THIS_MULTI(VfwCapture, IPersistPropertyBag_vtbl, iface);
    HRESULT hr;
    VARIANT var;
    const OLECHAR VFWIndex[] = {'V','F','W','I','n','d','e','x',0};

    TRACE("%p/%p-> (%p, %p)\n", iface, This, pPropBag, pErrorLog);

    V_VT(&var) = VT_I4;
    hr = IPropertyBag_Read(pPropBag, (LPCOLESTR)VFWIndex, &var, pErrorLog);

    if (SUCCEEDED(hr))
    {
        VfwPinImpl *pin;

        This->driver_info = qcap_driver_init( This->pOutputPin,
               var.__VARIANT_NAME_1.__VARIANT_NAME_2.__VARIANT_NAME_3.ulVal );
        if (This->driver_info)
        {
            pin = (VfwPinImpl *)This->pOutputPin;
            pin->driver_info = This->driver_info;
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
    ICOM_THIS_MULTI(VfwCapture, IPersistPropertyBag_vtbl, iface);
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
static HRESULT WINAPI
KSP_QueryInterface( IKsPropertySet * iface, REFIID riid, LPVOID * ppv )
{
    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IKsPropertySet))
    {
        *ppv = (LPVOID)iface;
        IKsPropertySet_AddRef( iface );
        return S_OK;
    }

    FIXME("No interface for iid %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI KSP_AddRef(IKsPropertySet * iface)
{
    ICOM_THIS_MULTI(VfwPinImpl, KSP_VT, iface);

    TRACE("%p --> Forwarding to VfwPin (%p)\n", iface, This);

    return IUnknown_AddRef((IUnknown *)This);
}

static ULONG WINAPI KSP_Release(IKsPropertySet * iface)
{
    ICOM_THIS_MULTI(VfwPinImpl, KSP_VT, iface);

    TRACE("%p --> Forwarding to VfwPin (%p)\n", iface, This);

    return IUnknown_Release((IUnknown *)This);
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
    *pGuid = PIN_CATEGORY_PREVIEW;
    FIXME("() Not adding a pin with PIN_CATEGORY_CAPTURE\n");
    return S_OK;
}

static HRESULT WINAPI
KSP_QuerySupported( IKsPropertySet * iface, REFGUID guidPropSet,
                    DWORD dwPropID, DWORD *pTypeSupport )
{
   FIXME("%p: stub\n", iface);
   return E_NOTIMPL;
}

static const IKsPropertySetVtbl KSP_VTable =
{
   KSP_QueryInterface,
   KSP_AddRef,
   KSP_Release,
   KSP_Set,
   KSP_Get,
   KSP_QuerySupported
};

static HRESULT
VfwPin_Construct( IBaseFilter * pBaseFilter, LPCRITICAL_SECTION pCritSec,
                  IPin ** ppPin )
{
    static const WCHAR wszOutputPinName[] = { 'O','u','t','p','u','t',0 };
    ALLOCATOR_PROPERTIES ap;
    VfwPinImpl * pPinImpl;
    PIN_INFO piOutput;
    HRESULT hr;

    pPinImpl = CoTaskMemAlloc( sizeof(*pPinImpl) );
    if (!pPinImpl)
        return E_OUTOFMEMORY;

    /* What we put here doesn't matter, the
       driver function should override it then commit */
    ap.cBuffers = 3;
    ap.cbBuffer = 230400;
    ap.cbAlign = 1;
    ap.cbPrefix = 0;

    piOutput.dir = PINDIR_OUTPUT;
    piOutput.pFilter = pBaseFilter;
    lstrcpyW(piOutput.achName, wszOutputPinName);
    ObjectRefCount(TRUE);

    hr = OutputPin_Init(&piOutput, &ap, pBaseFilter, NULL, pCritSec, &pPinImpl->pin);
    if (SUCCEEDED(hr))
    {
        pPinImpl->KSP_VT = &KSP_VTable;
        pPinImpl->pin.pin.lpVtbl = &VfwPin_Vtbl;
        *ppPin = (IPin *)(&pPinImpl->pin.pin.lpVtbl);
        return S_OK;
    }
    return E_FAIL;
}

static HRESULT WINAPI VfwPin_QueryInterface(IPin * iface, REFIID riid, LPVOID * ppv)
{
    VfwPinImpl *This = (VfwPinImpl *)iface;

    TRACE("%s %p\n", debugstr_guid(riid), ppv);

    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IPin))
        *ppv = (LPVOID)This;
    else if (IsEqualIID(riid, &IID_IKsPropertySet))
        *ppv = (LPVOID)&(This->KSP_VT);

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)(*ppv));
        return S_OK;
    }

    FIXME("No interface for %s!\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI VfwPin_AddRef(IPin * iface)
{
    VfwPinImpl *This = (VfwPinImpl *)iface;
    ULONG refCount = InterlockedIncrement(&This->pin.pin.refCount);

    TRACE("() -> new refcount: %u\n", refCount);

    return refCount;
}

static ULONG WINAPI
VfwPin_Release(IPin * iface)
{
   VfwPinImpl *This = (VfwPinImpl *)iface;
   ULONG refCount = InterlockedDecrement(&This->pin.pin.refCount);

   TRACE("() -> new refcount: %u\n", refCount);

   if (!refCount)
   {
      CoTaskMemFree(This);
      ObjectRefCount(FALSE);
   }
   return refCount;
}

static HRESULT WINAPI
VfwPin_EnumMediaTypes(IPin * iface, IEnumMediaTypes ** ppEnum)
{
    ENUMMEDIADETAILS emd;
    AM_MEDIA_TYPE *pmt;
    HRESULT hr;

    VfwPinImpl *This = (VfwPinImpl *)iface;
    emd.cMediaTypes = 1;
    hr = qcap_driver_get_format(This->driver_info, &pmt);
    emd.pMediaTypes = pmt;
    if (SUCCEEDED(hr))
        hr = IEnumMediaTypesImpl_Construct(&emd, ppEnum);
    TRACE("%p -- %x\n", This, hr);
    DeleteMediaType(pmt);
    return hr;
}

static HRESULT WINAPI
VfwPin_QueryInternalConnections(IPin * iface, IPin ** apPin, ULONG * cPin)
{
    TRACE("(%p)->(%p, %p)\n", iface, apPin, cPin);
    return E_NOTIMPL;
}

static HRESULT WINAPI VfwPin_EndOfStream(IPin * iface)
{
    TRACE("()\n");
    return E_UNEXPECTED;
}

static HRESULT WINAPI VfwPin_BeginFlush(IPin * iface)
{
    TRACE("(%p)->()\n", iface);
    return E_UNEXPECTED;
}

static HRESULT WINAPI VfwPin_EndFlush(IPin * iface)
{
    TRACE("(%p)->()\n", iface);
    return E_UNEXPECTED;
}

static HRESULT WINAPI
VfwPin_NewSegment(IPin * iface, REFERENCE_TIME tStart,
                  REFERENCE_TIME tStop, double dRate)
{
    TRACE("(%p)->(%s, %s, %e)\n", iface, wine_dbgstr_longlong(tStart),
           wine_dbgstr_longlong(tStop), dRate);
    return E_UNEXPECTED;
}

static const IPinVtbl VfwPin_Vtbl =
{
    VfwPin_QueryInterface,
    VfwPin_AddRef,
    VfwPin_Release,
    OutputPin_Connect,
    OutputPin_ReceiveConnection,
    OutputPin_Disconnect,
    IPinImpl_ConnectedTo,
    IPinImpl_ConnectionMediaType,
    IPinImpl_QueryPinInfo,
    IPinImpl_QueryDirection,
    IPinImpl_QueryId,
    IPinImpl_QueryAccept,
    VfwPin_EnumMediaTypes,
    VfwPin_QueryInternalConnections,
    VfwPin_EndOfStream,
    VfwPin_BeginFlush,
    VfwPin_EndFlush,
    VfwPin_NewSegment
};
