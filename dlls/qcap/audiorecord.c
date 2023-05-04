/*
 * Audio capture filter
 *
 * Copyright 2015 Damjan Jovanovic
 * Copyright 2023 Zeb Figura for CodeWeavers
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

#include "qcap_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

struct audio_record
{
    struct strmbase_filter filter;
    IPersistPropertyBag IPersistPropertyBag_iface;

    struct strmbase_source source;
    IAMStreamConfig IAMStreamConfig_iface;
};

static struct audio_record *impl_from_strmbase_filter(struct strmbase_filter *filter)
{
    return CONTAINING_RECORD(filter, struct audio_record, filter);
}

static HRESULT audio_record_source_query_interface(struct strmbase_pin *iface, REFIID iid, void **out)
{
    struct audio_record *filter = impl_from_strmbase_filter(iface->filter);

    if (IsEqualGUID(iid, &IID_IAMStreamConfig))
        *out = &filter->IAMStreamConfig_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static HRESULT audio_record_source_query_accept(struct strmbase_pin *iface, const AM_MEDIA_TYPE *mt)
{
    if (!IsEqualGUID(&mt->majortype, &MEDIATYPE_Audio))
        return S_FALSE;

    if (!IsEqualGUID(&mt->formattype, &FORMAT_WaveFormatEx))
        return S_FALSE;

    return S_OK;
}

static const struct
{
    unsigned int rate;
    unsigned int depth;
    unsigned int channels;
}
audio_formats[] =
{
    {44100, 16, 2},
    {44100, 16, 1},
    {32000, 16, 2},
    {32000, 16, 1},
    {22050, 16, 2},
    {22050, 16, 1},
    {11025, 16, 2},
    {11025, 16, 1},
    { 8000, 16, 2},
    { 8000, 16, 1},
    {44100,  8, 2},
    {44100,  8, 1},
    {22050,  8, 2},
    {22050,  8, 1},
    {11025,  8, 2},
    {11025,  8, 1},
    { 8000,  8, 2},
    { 8000,  8, 1},
    {48000, 16, 2},
    {48000, 16, 1},
    {96000, 16, 2},
    {96000, 16, 1},
};

static HRESULT fill_media_type(unsigned int index, AM_MEDIA_TYPE *mt)
{
    WAVEFORMATEX *format;

    if (index >= ARRAY_SIZE(audio_formats))
        return VFW_S_NO_MORE_ITEMS;

    if (!(format = CoTaskMemAlloc(sizeof(*format))))
        return E_OUTOFMEMORY;

    memset(format, 0, sizeof(WAVEFORMATEX));
    format->wFormatTag = WAVE_FORMAT_PCM;
    format->nChannels = audio_formats[index].channels;
    format->nSamplesPerSec = audio_formats[index].rate;
    format->wBitsPerSample = audio_formats[index].depth;
    format->nBlockAlign = audio_formats[index].channels * audio_formats[index].depth / 8;
    format->nAvgBytesPerSec = format->nBlockAlign * format->nSamplesPerSec;

    memset(mt, 0, sizeof(AM_MEDIA_TYPE));
    mt->majortype = MEDIATYPE_Audio;
    mt->subtype = MEDIASUBTYPE_PCM;
    mt->bFixedSizeSamples = TRUE;
    mt->lSampleSize = format->nBlockAlign;
    mt->formattype = FORMAT_WaveFormatEx;
    mt->cbFormat = sizeof(WAVEFORMATEX);
    mt->pbFormat = (BYTE *)format;

    return S_OK;
}

static HRESULT audio_record_source_get_media_type(struct strmbase_pin *iface,
        unsigned int index, AM_MEDIA_TYPE *mt)
{
    return fill_media_type(index, mt);
}

static HRESULT WINAPI audio_record_source_DecideBufferSize(struct strmbase_source *iface,
        IMemAllocator *allocator, ALLOCATOR_PROPERTIES *props)
{
    ALLOCATOR_PROPERTIES ret_props;

    return IMemAllocator_SetProperties(allocator, props, &ret_props);
}

static const struct strmbase_source_ops source_ops =
{
    .base.pin_get_media_type = audio_record_source_get_media_type,
    .base.pin_query_accept = audio_record_source_query_accept,
    .base.pin_query_interface = audio_record_source_query_interface,
    .pfnAttemptConnection = BaseOutputPinImpl_AttemptConnection,
    .pfnDecideAllocator = BaseOutputPinImpl_DecideAllocator,
    .pfnDecideBufferSize = audio_record_source_DecideBufferSize,
};

static struct audio_record *impl_from_IAMStreamConfig(IAMStreamConfig *iface)
{
    return CONTAINING_RECORD(iface, struct audio_record, IAMStreamConfig_iface);
}

static HRESULT WINAPI stream_config_QueryInterface(IAMStreamConfig *iface, REFIID iid, void **out)
{
    struct audio_record *filter = impl_from_IAMStreamConfig(iface);
    return IPin_QueryInterface(&filter->source.pin.IPin_iface, iid, out);
}

static ULONG WINAPI stream_config_AddRef(IAMStreamConfig *iface)
{
    struct audio_record *filter = impl_from_IAMStreamConfig(iface);
    return IPin_AddRef(&filter->source.pin.IPin_iface);
}

static ULONG WINAPI stream_config_Release(IAMStreamConfig *iface)
{
    struct audio_record *filter = impl_from_IAMStreamConfig(iface);
    return IPin_Release(&filter->source.pin.IPin_iface);
}

static HRESULT WINAPI stream_config_SetFormat(IAMStreamConfig *iface, AM_MEDIA_TYPE *mt)
{
    FIXME("iface %p, mt %p, stub!\n", iface, mt);
    strmbase_dump_media_type(mt);
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_config_GetFormat(IAMStreamConfig *iface, AM_MEDIA_TYPE **mt)
{
    FIXME("iface %p, mt %p, stub!\n", iface, mt);
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_config_GetNumberOfCapabilities(IAMStreamConfig *iface,
        int *count, int *size)
{
    struct audio_record *filter = impl_from_IAMStreamConfig(iface);

    TRACE("filter %p, count %p, size %p.\n", filter, count, size);

    *count = ARRAY_SIZE(audio_formats);
    *size = sizeof(AUDIO_STREAM_CONFIG_CAPS);
    return S_OK;
}

static HRESULT WINAPI stream_config_GetStreamCaps(IAMStreamConfig *iface,
        int index, AM_MEDIA_TYPE **ret_mt, BYTE *caps)
{
    struct audio_record *filter = impl_from_IAMStreamConfig(iface);
    AUDIO_STREAM_CONFIG_CAPS *audio_caps = (void *)caps;
    AM_MEDIA_TYPE *mt;
    HRESULT hr;

    TRACE("filter %p, index %d, ret_mt %p, caps %p.\n", filter, index, ret_mt, caps);

    if (index >= ARRAY_SIZE(audio_formats))
        return S_FALSE;

    if (!(mt = CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE))))
        return E_OUTOFMEMORY;

    if ((hr = fill_media_type(index, mt)) != S_OK)
    {
        CoTaskMemFree(mt);
        return hr;
    }

    *ret_mt = mt;

    audio_caps->guid = MEDIATYPE_Audio;
    audio_caps->MinimumChannels = 1;
    audio_caps->MaximumChannels = 2;
    audio_caps->ChannelsGranularity = 1;
    audio_caps->MinimumBitsPerSample = 8;
    audio_caps->MaximumBitsPerSample = 16;
    audio_caps->BitsPerSampleGranularity = 8;
    audio_caps->MinimumSampleFrequency = 11025;
    audio_caps->MaximumSampleFrequency = 44100;
    audio_caps->SampleFrequencyGranularity = 11025;

    return S_OK;
}

static const IAMStreamConfigVtbl stream_config_vtbl =
{
    stream_config_QueryInterface,
    stream_config_AddRef,
    stream_config_Release,
    stream_config_SetFormat,
    stream_config_GetFormat,
    stream_config_GetNumberOfCapabilities,
    stream_config_GetStreamCaps,
};

static struct audio_record *impl_from_IPersistPropertyBag(IPersistPropertyBag *iface)
{
    return CONTAINING_RECORD(iface, struct audio_record, IPersistPropertyBag_iface);
}

static struct strmbase_pin *audio_record_get_pin(struct strmbase_filter *iface, unsigned int index)
{
    struct audio_record *filter = impl_from_strmbase_filter(iface);

    if (!index)
        return &filter->source.pin;
    return NULL;
}

static void audio_record_destroy(struct strmbase_filter *iface)
{
    struct audio_record *filter = impl_from_strmbase_filter(iface);

    strmbase_source_cleanup(&filter->source);
    strmbase_filter_cleanup(&filter->filter);
    free(filter);
}

static HRESULT audio_record_query_interface(struct strmbase_filter *iface, REFIID iid, void **out)
{
    struct audio_record *filter = impl_from_strmbase_filter(iface);

    if (IsEqualGUID(iid, &IID_IPersistPropertyBag))
        *out = &filter->IPersistPropertyBag_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static const struct strmbase_filter_ops filter_ops =
{
    .filter_get_pin = audio_record_get_pin,
    .filter_destroy = audio_record_destroy,
    .filter_query_interface = audio_record_query_interface,
};

static HRESULT WINAPI PPB_QueryInterface(IPersistPropertyBag *iface, REFIID riid, LPVOID *ppv)
{
    struct audio_record *filter = impl_from_IPersistPropertyBag(iface);

    return IUnknown_QueryInterface(filter->filter.outer_unk, riid, ppv);
}

static ULONG WINAPI PPB_AddRef(IPersistPropertyBag *iface)
{
    struct audio_record *filter = impl_from_IPersistPropertyBag(iface);

    return IUnknown_AddRef(filter->filter.outer_unk);
}

static ULONG WINAPI PPB_Release(IPersistPropertyBag *iface)
{
    struct audio_record *filter = impl_from_IPersistPropertyBag(iface);

    return IUnknown_Release(filter->filter.outer_unk);
}

static HRESULT WINAPI PPB_GetClassID(IPersistPropertyBag *iface, CLSID *pClassID)
{
    struct audio_record *This = impl_from_IPersistPropertyBag(iface);
    TRACE("(%p/%p)->(%p)\n", iface, This, pClassID);
    return IBaseFilter_GetClassID(&This->filter.IBaseFilter_iface, pClassID);
}

static HRESULT WINAPI PPB_InitNew(IPersistPropertyBag *iface)
{
    struct audio_record *This = impl_from_IPersistPropertyBag(iface);
    FIXME("(%p/%p)->(): stub\n", iface, This);
    return E_NOTIMPL;
}

static HRESULT WINAPI PPB_Load(IPersistPropertyBag *iface, IPropertyBag *pPropBag,
        IErrorLog *pErrorLog)
{
    struct audio_record *This = impl_from_IPersistPropertyBag(iface);
    HRESULT hr;
    VARIANT var;

    TRACE("(%p/%p)->(%p, %p)\n", iface, This, pPropBag, pErrorLog);

    V_VT(&var) = VT_I4;
    hr = IPropertyBag_Read(pPropBag, L"WaveInID", &var, pErrorLog);
    if (SUCCEEDED(hr))
    {
        FIXME("FIXME: implement opening waveIn device %ld\n", V_I4(&var));
    }

    return hr;
}

static HRESULT WINAPI PPB_Save(IPersistPropertyBag *iface, IPropertyBag *pPropBag,
        BOOL fClearDirty, BOOL fSaveAllProperties)
{
    struct audio_record *This = impl_from_IPersistPropertyBag(iface);
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

HRESULT audio_record_create(IUnknown *outer, IUnknown **out)
{
    struct audio_record *object;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IPersistPropertyBag_iface.lpVtbl = &PersistPropertyBagVtbl;
    strmbase_filter_init(&object->filter, outer, &CLSID_AudioRecord, &filter_ops);

    strmbase_source_init(&object->source, &object->filter, L"Capture", &source_ops);
    object->IAMStreamConfig_iface.lpVtbl = &stream_config_vtbl;

    TRACE("Created audio recorder %p.\n", object);
    *out = &object->filter.IUnknown_inner;
    return S_OK;
}
