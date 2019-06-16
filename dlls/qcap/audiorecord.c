/* Implementation of the Audio Capture Filter (CLSID_AudioRecord)
 *
 * Copyright 2015 Damjan Jovanovic
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
#include "wtypes.h"
#include "wingdi.h"
#include "winuser.h"
#include "dshow.h"

#include "qcap_main.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(qcap);

typedef struct {
    BaseFilter filter;
    IPersistPropertyBag IPersistPropertyBag_iface;
    BaseOutputPin *output;
} AudioRecord;

static inline AudioRecord *impl_from_BaseFilter(BaseFilter *filter)
{
    return CONTAINING_RECORD(filter, AudioRecord, filter);
}

static inline AudioRecord *impl_from_IBaseFilter(IBaseFilter *iface)
{
    BaseFilter *filter = CONTAINING_RECORD(iface, BaseFilter, IBaseFilter_iface);
    return impl_from_BaseFilter(filter);
}

static inline AudioRecord *impl_from_IPersistPropertyBag(IPersistPropertyBag *iface)
{
    return CONTAINING_RECORD(iface, AudioRecord, IPersistPropertyBag_iface);
}

static HRESULT WINAPI AudioRecord_Stop(IBaseFilter *iface)
{
    AudioRecord *This = impl_from_IBaseFilter(iface);
    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI AudioRecord_Pause(IBaseFilter *iface)
{
    AudioRecord *This = impl_from_IBaseFilter(iface);
    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI AudioRecord_Run(IBaseFilter *iface, REFERENCE_TIME tStart)
{
    AudioRecord *This = impl_from_IBaseFilter(iface);
    FIXME("(%p, %s): stub\n", This, wine_dbgstr_longlong(tStart));
    return E_NOTIMPL;
}

static const IBaseFilterVtbl AudioRecordVtbl = {
    BaseFilterImpl_QueryInterface,
    BaseFilterImpl_AddRef,
    BaseFilterImpl_Release,
    BaseFilterImpl_GetClassID,
    AudioRecord_Stop,
    AudioRecord_Pause,
    AudioRecord_Run,
    BaseFilterImpl_GetState,
    BaseFilterImpl_SetSyncSource,
    BaseFilterImpl_GetSyncSource,
    BaseFilterImpl_EnumPins,
    BaseFilterImpl_FindPin,
    BaseFilterImpl_QueryFilterInfo,
    BaseFilterImpl_JoinFilterGraph,
    BaseFilterImpl_QueryVendorInfo
};

static IPin *audio_record_get_pin(BaseFilter *iface, unsigned int index)
{
    FIXME("iface %p, index %u, stub!\n", iface, index);
    return NULL;
}

static void audio_record_destroy(BaseFilter *iface)
{
    AudioRecord *filter = impl_from_BaseFilter(iface);

    strmbase_filter_cleanup(&filter->filter);
    CoTaskMemFree(filter);
}

static HRESULT audio_record_query_interface(BaseFilter *iface, REFIID iid, void **out)
{
    AudioRecord *filter = impl_from_BaseFilter(iface);

    if (IsEqualGUID(iid, &IID_IPersistPropertyBag))
        *out = &filter->IPersistPropertyBag_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static const BaseFilterFuncTable AudioRecordFuncs = {
    .filter_get_pin = audio_record_get_pin,
    .filter_destroy = audio_record_destroy,
    .filter_query_interface = audio_record_query_interface,
};

static HRESULT WINAPI PPB_QueryInterface(IPersistPropertyBag *iface, REFIID riid, LPVOID *ppv)
{
    AudioRecord *This = impl_from_IPersistPropertyBag(iface);
    return IUnknown_QueryInterface(This->filter.outer_unk, riid, ppv);
}

static ULONG WINAPI PPB_AddRef(IPersistPropertyBag *iface)
{
    AudioRecord *This = impl_from_IPersistPropertyBag(iface);
    return IUnknown_AddRef(This->filter.outer_unk);
}

static ULONG WINAPI PPB_Release(IPersistPropertyBag *iface)
{
    AudioRecord *This = impl_from_IPersistPropertyBag(iface);
    return IUnknown_Release(This->filter.outer_unk);
}

static HRESULT WINAPI PPB_GetClassID(IPersistPropertyBag *iface, CLSID *pClassID)
{
    AudioRecord *This = impl_from_IPersistPropertyBag(iface);
    TRACE("(%p/%p)->(%p)\n", iface, This, pClassID);
    return IBaseFilter_GetClassID(&This->filter.IBaseFilter_iface, pClassID);
}

static HRESULT WINAPI PPB_InitNew(IPersistPropertyBag *iface)
{
    AudioRecord *This = impl_from_IPersistPropertyBag(iface);
    FIXME("(%p/%p)->(): stub\n", iface, This);
    return E_NOTIMPL;
}

static HRESULT WINAPI PPB_Load(IPersistPropertyBag *iface, IPropertyBag *pPropBag,
        IErrorLog *pErrorLog)
{
    AudioRecord *This = impl_from_IPersistPropertyBag(iface);
    HRESULT hr;
    VARIANT var;
    static const WCHAR WaveInIDW[] = {'W','a','v','e','I','n','I','D',0};

    TRACE("(%p/%p)->(%p, %p)\n", iface, This, pPropBag, pErrorLog);

    V_VT(&var) = VT_I4;
    hr = IPropertyBag_Read(pPropBag, WaveInIDW, &var, pErrorLog);
    if (SUCCEEDED(hr))
    {
        FIXME("FIXME: implement opening waveIn device %d\n", V_I4(&var));
    }

    return hr;
}

static HRESULT WINAPI PPB_Save(IPersistPropertyBag *iface, IPropertyBag *pPropBag,
        BOOL fClearDirty, BOOL fSaveAllProperties)
{
    AudioRecord *This = impl_from_IPersistPropertyBag(iface);
    FIXME("(%p/%p)->(%p, %u, %u): stub\n", iface, This, pPropBag, fClearDirty, fSaveAllProperties);
    return E_NOTIMPL;
}

static const IPersistPropertyBagVtbl PersistPropertyBagVtbl =
{
    PPB_QueryInterface,
    PPB_AddRef,
    PPB_Release,
    PPB_GetClassID,
    PPB_InitNew,
    PPB_Load,
    PPB_Save
};

IUnknown* WINAPI QCAP_createAudioCaptureFilter(IUnknown *outer, HRESULT *phr)
{
    AudioRecord *This = NULL;

    FIXME("(%p, %p): the entire CLSID_AudioRecord implementation is just stubs\n", outer, phr);

    This = CoTaskMemAlloc(sizeof(*This));
    if (This == NULL) {
        *phr = E_OUTOFMEMORY;
        return NULL;
    }
    memset(This, 0, sizeof(*This));
    This->IPersistPropertyBag_iface.lpVtbl = &PersistPropertyBagVtbl;

    strmbase_filter_init(&This->filter, &AudioRecordVtbl, outer, &CLSID_AudioRecord,
            (DWORD_PTR)(__FILE__ ": AudioRecord.csFilter"), &AudioRecordFuncs);

    *phr = S_OK;
    return &This->filter.IUnknown_inner;
}
