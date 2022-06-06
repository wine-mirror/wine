/*
 * Copyright 2019 Nikolay Sivov for CodeWeavers
 * Copyright 2020 Zebediah Figura for CodeWeavers
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

#include "gst_private.h"

#include "ks.h"
#include "ksmedia.h"
#include "wmcodecdsp.h"
#include "initguid.h"
#include "mfapi.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

struct video_processor
{
    IMFTransform IMFTransform_iface;
    LONG refcount;
    IMFAttributes *attributes;
    IMFAttributes *output_attributes;
};

static struct video_processor *impl_video_processor_from_IMFTransform(IMFTransform *iface)
{
    return CONTAINING_RECORD(iface, struct video_processor, IMFTransform_iface);
}

static HRESULT WINAPI video_processor_QueryInterface(IMFTransform *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IMFTransform) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFTransform_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI video_processor_AddRef(IMFTransform *iface)
{
    struct video_processor *transform = impl_video_processor_from_IMFTransform(iface);
    ULONG refcount = InterlockedIncrement(&transform->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI video_processor_Release(IMFTransform *iface)
{
    struct video_processor *transform = impl_video_processor_from_IMFTransform(iface);
    ULONG refcount = InterlockedDecrement(&transform->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        if (transform->attributes)
            IMFAttributes_Release(transform->attributes);
        if (transform->output_attributes)
            IMFAttributes_Release(transform->output_attributes);
        free(transform);
    }

    return refcount;
}

static HRESULT WINAPI video_processor_GetStreamLimits(IMFTransform *iface, DWORD *input_minimum, DWORD *input_maximum,
        DWORD *output_minimum, DWORD *output_maximum)
{
    TRACE("%p, %p, %p, %p, %p.\n", iface, input_minimum, input_maximum, output_minimum, output_maximum);

    *input_minimum = *input_maximum = *output_minimum = *output_maximum = 1;

    return S_OK;
}

static HRESULT WINAPI video_processor_GetStreamCount(IMFTransform *iface, DWORD *inputs, DWORD *outputs)
{
    TRACE("%p, %p, %p.\n", iface, inputs, outputs);

    *inputs = *outputs = 1;

    return S_OK;
}

static HRESULT WINAPI video_processor_GetStreamIDs(IMFTransform *iface, DWORD input_size, DWORD *inputs,
        DWORD output_size, DWORD *outputs)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI video_processor_GetInputStreamInfo(IMFTransform *iface, DWORD id, MFT_INPUT_STREAM_INFO *info)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI video_processor_GetOutputStreamInfo(IMFTransform *iface, DWORD id, MFT_OUTPUT_STREAM_INFO *info)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI video_processor_GetAttributes(IMFTransform *iface, IMFAttributes **attributes)
{
    struct video_processor *transform = impl_video_processor_from_IMFTransform(iface);

    TRACE("%p, %p.\n", iface, attributes);

    *attributes = transform->attributes;
    IMFAttributes_AddRef(*attributes);

    return S_OK;
}

static HRESULT WINAPI video_processor_GetInputStreamAttributes(IMFTransform *iface, DWORD id,
        IMFAttributes **attributes)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI video_processor_GetOutputStreamAttributes(IMFTransform *iface, DWORD id,
        IMFAttributes **attributes)
{
    struct video_processor *transform = impl_video_processor_from_IMFTransform(iface);

    TRACE("%p, %lu, %p.\n", iface, id, attributes);

    *attributes = transform->output_attributes;
    IMFAttributes_AddRef(*attributes);

    return S_OK;
}

static HRESULT WINAPI video_processor_DeleteInputStream(IMFTransform *iface, DWORD id)
{
    TRACE("%p, %lu.\n", iface, id);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_processor_AddInputStreams(IMFTransform *iface, DWORD streams, DWORD *ids)
{
    TRACE("%p, %lu, %p.\n", iface, streams, ids);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_processor_GetInputAvailableType(IMFTransform *iface, DWORD id, DWORD index,
        IMFMediaType **type)
{
    FIXME("%p, %lu, %lu, %p.\n", iface, id, index, type);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_processor_GetOutputAvailableType(IMFTransform *iface, DWORD id, DWORD index,
        IMFMediaType **type)
{
    FIXME("%p, %lu, %lu, %p.\n", iface, id, index, type);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_processor_SetInputType(IMFTransform *iface, DWORD id, IMFMediaType *type, DWORD flags)
{
    FIXME("%p, %lu, %p, %#lx.\n", iface, id, type, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_processor_SetOutputType(IMFTransform *iface, DWORD id, IMFMediaType *type, DWORD flags)
{
    FIXME("%p, %lu, %p, %#lx.\n", iface, id, type, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_processor_GetInputCurrentType(IMFTransform *iface, DWORD id, IMFMediaType **type)
{
    FIXME("%p, %lu, %p.\n", iface, id, type);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_processor_GetOutputCurrentType(IMFTransform *iface, DWORD id, IMFMediaType **type)
{
    FIXME("%p, %lu, %p.\n", iface, id, type);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_processor_GetInputStatus(IMFTransform *iface, DWORD id, DWORD *flags)
{
    FIXME("%p, %lu, %p.\n", iface, id, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_processor_GetOutputStatus(IMFTransform *iface, DWORD *flags)
{
    FIXME("%p, %p.\n", iface, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_processor_SetOutputBounds(IMFTransform *iface, LONGLONG lower, LONGLONG upper)
{
    FIXME("%p, %s, %s.\n", iface, wine_dbgstr_longlong(lower), wine_dbgstr_longlong(upper));

    return E_NOTIMPL;
}

static HRESULT WINAPI video_processor_ProcessEvent(IMFTransform *iface, DWORD id, IMFMediaEvent *event)
{
    TRACE("%p, %lu, %p.\n", iface, id, event);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_processor_ProcessMessage(IMFTransform *iface, MFT_MESSAGE_TYPE message, ULONG_PTR param)
{
    FIXME("%p, %u, %#Ix.\n", iface, message, param);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_processor_ProcessInput(IMFTransform *iface, DWORD id, IMFSample *sample, DWORD flags)
{
    FIXME("%p, %lu, %p, %#lx.\n", iface, id, sample, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_processor_ProcessOutput(IMFTransform *iface, DWORD flags, DWORD count,
        MFT_OUTPUT_DATA_BUFFER *samples, DWORD *status)
{
    FIXME("%p, %#lx, %lu, %p, %p.\n", iface, flags, count, samples, status);

    return E_NOTIMPL;
}

static const IMFTransformVtbl video_processor_vtbl =
{
    video_processor_QueryInterface,
    video_processor_AddRef,
    video_processor_Release,
    video_processor_GetStreamLimits,
    video_processor_GetStreamCount,
    video_processor_GetStreamIDs,
    video_processor_GetInputStreamInfo,
    video_processor_GetOutputStreamInfo,
    video_processor_GetAttributes,
    video_processor_GetInputStreamAttributes,
    video_processor_GetOutputStreamAttributes,
    video_processor_DeleteInputStream,
    video_processor_AddInputStreams,
    video_processor_GetInputAvailableType,
    video_processor_GetOutputAvailableType,
    video_processor_SetInputType,
    video_processor_SetOutputType,
    video_processor_GetInputCurrentType,
    video_processor_GetOutputCurrentType,
    video_processor_GetInputStatus,
    video_processor_GetOutputStatus,
    video_processor_SetOutputBounds,
    video_processor_ProcessEvent,
    video_processor_ProcessMessage,
    video_processor_ProcessInput,
    video_processor_ProcessOutput,
};

struct class_factory
{
    IClassFactory IClassFactory_iface;
    LONG refcount;
    HRESULT (*create_instance)(REFIID riid, void **obj);
};

static struct class_factory *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, struct class_factory, IClassFactory_iface);
}

static HRESULT WINAPI class_factory_QueryInterface(IClassFactory *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualGUID(riid, &IID_IClassFactory) ||
            IsEqualGUID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IClassFactory_AddRef(iface);
        return S_OK;
    }

    WARN("%s is not supported.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI class_factory_AddRef(IClassFactory *iface)
{
    struct class_factory *factory = impl_from_IClassFactory(iface);
    return InterlockedIncrement(&factory->refcount);
}

static ULONG WINAPI class_factory_Release(IClassFactory *iface)
{
    struct class_factory *factory = impl_from_IClassFactory(iface);
    ULONG refcount = InterlockedDecrement(&factory->refcount);

    if (!refcount)
        free(factory);

    return refcount;
}

static HRESULT WINAPI class_factory_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **obj)
{
    struct class_factory *factory = impl_from_IClassFactory(iface);

    TRACE("%p, %p, %s, %p.\n", iface, outer, debugstr_guid(riid), obj);

    if (outer)
    {
        *obj = NULL;
        return CLASS_E_NOAGGREGATION;
    }

    return factory->create_instance(riid, obj);
}

static HRESULT WINAPI class_factory_LockServer(IClassFactory *iface, BOOL dolock)
{
    TRACE("%p, %d.\n", iface, dolock);
    return S_OK;
}

static const IClassFactoryVtbl class_factory_vtbl =
{
    class_factory_QueryInterface,
    class_factory_AddRef,
    class_factory_Release,
    class_factory_CreateInstance,
    class_factory_LockServer,
};

static HRESULT video_processor_create(REFIID riid, void **ret)
{
    struct video_processor *object;
    HRESULT hr;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IMFTransform_iface.lpVtbl = &video_processor_vtbl;
    object->refcount = 1;

    if (FAILED(hr = MFCreateAttributes(&object->attributes, 0)))
        goto failed;

    if (FAILED(hr = MFCreateAttributes(&object->output_attributes, 0)))
        goto failed;

    *ret = &object->IMFTransform_iface;
    return S_OK;

failed:

    IMFTransform_Release(&object->IMFTransform_iface);
    return hr;
}

static const GUID CLSID_GStreamerByteStreamHandler = {0x317df618, 0x5e5a, 0x468a, {0x9f, 0x15, 0xd8, 0x27, 0xa9, 0xa0, 0x81, 0x62}};

static const GUID CLSID_WINEAudioConverter = {0x6a170414,0xaad9,0x4693,{0xb8,0x06,0x3a,0x0c,0x47,0xc5,0x70,0xd6}};

static const struct class_object
{
    const GUID *clsid;
    HRESULT (*create_instance)(REFIID riid, void **obj);
}
class_objects[] =
{
    { &CLSID_VideoProcessorMFT, &video_processor_create },
    { &CLSID_GStreamerByteStreamHandler, &winegstreamer_stream_handler_create },
    { &CLSID_WINEAudioConverter, &audio_converter_create },
    { &CLSID_MSH264DecoderMFT, &h264_decoder_create },
};

HRESULT mfplat_get_class_object(REFCLSID rclsid, REFIID riid, void **obj)
{
    struct class_factory *factory;
    unsigned int i;
    HRESULT hr;

    for (i = 0; i < ARRAY_SIZE(class_objects); ++i)
    {
        if (IsEqualGUID(class_objects[i].clsid, rclsid))
        {
            if (!(factory = malloc(sizeof(*factory))))
                return E_OUTOFMEMORY;

            factory->IClassFactory_iface.lpVtbl = &class_factory_vtbl;
            factory->refcount = 1;
            factory->create_instance = class_objects[i].create_instance;

            hr = IClassFactory_QueryInterface(&factory->IClassFactory_iface, riid, obj);
            IClassFactory_Release(&factory->IClassFactory_iface);
            return hr;
        }
    }

    return CLASS_E_CLASSNOTAVAILABLE;
}

HRESULT mfplat_DllRegisterServer(void)
{
    MFT_REGISTER_TYPE_INFO audio_converter_supported_types[] =
    {
        {MFMediaType_Audio, MFAudioFormat_PCM},
        {MFMediaType_Audio, MFAudioFormat_Float},
    };

    MFT_REGISTER_TYPE_INFO wma_decoder_input_types[] =
    {
        {MFMediaType_Audio, MEDIASUBTYPE_MSAUDIO1},
        {MFMediaType_Audio, MFAudioFormat_WMAudioV8},
        {MFMediaType_Audio, MFAudioFormat_WMAudioV9},
        {MFMediaType_Audio, MFAudioFormat_WMAudio_Lossless},
    };
    MFT_REGISTER_TYPE_INFO wma_decoder_output_types[] =
    {
        {MFMediaType_Audio, MFAudioFormat_PCM},
        {MFMediaType_Audio, MFAudioFormat_Float},
    };

    MFT_REGISTER_TYPE_INFO h264_decoder_input_types[] =
    {
        {MFMediaType_Video, MFVideoFormat_H264},
        {MFMediaType_Video, MFVideoFormat_H264_ES},
    };
    MFT_REGISTER_TYPE_INFO h264_decoder_output_types[] =
    {
        {MFMediaType_Video, MFVideoFormat_NV12},
        {MFMediaType_Video, MFVideoFormat_YV12},
        {MFMediaType_Video, MFVideoFormat_IYUV},
        {MFMediaType_Video, MFVideoFormat_I420},
        {MFMediaType_Video, MFVideoFormat_YUY2},
    };

    MFT_REGISTER_TYPE_INFO video_processor_input_types[] =
    {
        {MFMediaType_Video, MFVideoFormat_IYUV},
        {MFMediaType_Video, MFVideoFormat_YV12},
        {MFMediaType_Video, MFVideoFormat_NV12},
        {MFMediaType_Video, MFVideoFormat_YUY2},
        {MFMediaType_Video, MFVideoFormat_ARGB32},
        {MFMediaType_Video, MFVideoFormat_RGB32},
        {MFMediaType_Video, MFVideoFormat_NV11},
        {MFMediaType_Video, MFVideoFormat_AYUV},
        {MFMediaType_Video, MFVideoFormat_UYVY},
        {MFMediaType_Video, MEDIASUBTYPE_P208},
        {MFMediaType_Video, MFVideoFormat_RGB24},
        {MFMediaType_Video, MFVideoFormat_RGB555},
        {MFMediaType_Video, MFVideoFormat_RGB565},
        {MFMediaType_Video, MFVideoFormat_RGB8},
        {MFMediaType_Video, MFVideoFormat_I420},
        {MFMediaType_Video, MFVideoFormat_Y216},
        {MFMediaType_Video, MFVideoFormat_v410},
        {MFMediaType_Video, MFVideoFormat_Y41P},
        {MFMediaType_Video, MFVideoFormat_Y41T},
        {MFMediaType_Video, MFVideoFormat_Y42T},
        {MFMediaType_Video, MFVideoFormat_YVYU},
        {MFMediaType_Video, MFVideoFormat_420O},
    };
    MFT_REGISTER_TYPE_INFO video_processor_output_types[] =
    {
        {MFMediaType_Video, MFVideoFormat_IYUV},
        {MFMediaType_Video, MFVideoFormat_YV12},
        {MFMediaType_Video, MFVideoFormat_NV12},
        {MFMediaType_Video, MFVideoFormat_YUY2},
        {MFMediaType_Video, MFVideoFormat_ARGB32},
        {MFMediaType_Video, MFVideoFormat_RGB32},
        {MFMediaType_Video, MFVideoFormat_NV11},
        {MFMediaType_Video, MFVideoFormat_AYUV},
        {MFMediaType_Video, MFVideoFormat_UYVY},
        {MFMediaType_Video, MEDIASUBTYPE_P208},
        {MFMediaType_Video, MFVideoFormat_RGB24},
        {MFMediaType_Video, MFVideoFormat_RGB555},
        {MFMediaType_Video, MFVideoFormat_RGB565},
        {MFMediaType_Video, MFVideoFormat_RGB8},
        {MFMediaType_Video, MFVideoFormat_I420},
        {MFMediaType_Video, MFVideoFormat_Y216},
        {MFMediaType_Video, MFVideoFormat_v410},
        {MFMediaType_Video, MFVideoFormat_Y41P},
        {MFMediaType_Video, MFVideoFormat_Y41T},
        {MFMediaType_Video, MFVideoFormat_Y42T},
        {MFMediaType_Video, MFVideoFormat_YVYU},
    };

    struct mft
    {
        GUID clsid;
        GUID category;
        WCHAR name[MAX_PATH];
        UINT32 flags;
        UINT32 input_types_count;
        MFT_REGISTER_TYPE_INFO *input_types;
        UINT32 output_types_count;
        MFT_REGISTER_TYPE_INFO *output_types;
    }
    mfts[] =
    {
        {
            CLSID_WINEAudioConverter,
            MFT_CATEGORY_AUDIO_EFFECT,
            L"Audio Converter",
            MFT_ENUM_FLAG_SYNCMFT,
            ARRAY_SIZE(audio_converter_supported_types),
            audio_converter_supported_types,
            ARRAY_SIZE(audio_converter_supported_types),
            audio_converter_supported_types,
        },
        {
            CLSID_WMADecMediaObject,
            MFT_CATEGORY_AUDIO_DECODER,
            L"WMAudio Decoder MFT",
            MFT_ENUM_FLAG_SYNCMFT,
            ARRAY_SIZE(wma_decoder_input_types),
            wma_decoder_input_types,
            ARRAY_SIZE(wma_decoder_output_types),
            wma_decoder_output_types,
        },
        {
            CLSID_MSH264DecoderMFT,
            MFT_CATEGORY_VIDEO_DECODER,
            L"Microsoft H264 Video Decoder MFT",
            MFT_ENUM_FLAG_SYNCMFT,
            ARRAY_SIZE(h264_decoder_input_types),
            h264_decoder_input_types,
            ARRAY_SIZE(h264_decoder_output_types),
            h264_decoder_output_types,
        },
        {
            CLSID_VideoProcessorMFT,
            MFT_CATEGORY_VIDEO_PROCESSOR,
            L"Microsoft Video Processor MFT",
            MFT_ENUM_FLAG_SYNCMFT,
            ARRAY_SIZE(video_processor_input_types),
            video_processor_input_types,
            ARRAY_SIZE(video_processor_output_types),
            video_processor_output_types,
        },
    };

    unsigned int i;
    HRESULT hr;

    for (i = 0; i < ARRAY_SIZE(mfts); i++)
    {
        hr = MFTRegister(mfts[i].clsid, mfts[i].category, mfts[i].name, mfts[i].flags, mfts[i].input_types_count,
                    mfts[i].input_types, mfts[i].output_types_count, mfts[i].output_types, NULL);

        if (FAILED(hr))
        {
            FIXME("Failed to register MFT, hr %#lx.\n", hr);
            return hr;
        }
    }

    return S_OK;
}

static const struct
{
    const GUID *subtype;
    enum wg_video_format format;
}
video_formats[] =
{
    {&MFVideoFormat_ARGB32, WG_VIDEO_FORMAT_BGRA},
    {&MFVideoFormat_RGB32,  WG_VIDEO_FORMAT_BGRx},
    {&MFVideoFormat_RGB24,  WG_VIDEO_FORMAT_BGR},
    {&MFVideoFormat_RGB555, WG_VIDEO_FORMAT_RGB15},
    {&MFVideoFormat_RGB565, WG_VIDEO_FORMAT_RGB16},
    {&MFVideoFormat_AYUV,   WG_VIDEO_FORMAT_AYUV},
    {&MFVideoFormat_I420,   WG_VIDEO_FORMAT_I420},
    {&MFVideoFormat_IYUV,   WG_VIDEO_FORMAT_I420},
    {&MFVideoFormat_NV12,   WG_VIDEO_FORMAT_NV12},
    {&MFVideoFormat_UYVY,   WG_VIDEO_FORMAT_UYVY},
    {&MFVideoFormat_YUY2,   WG_VIDEO_FORMAT_YUY2},
    {&MFVideoFormat_YV12,   WG_VIDEO_FORMAT_YV12},
    {&MFVideoFormat_YVYU,   WG_VIDEO_FORMAT_YVYU},
};

static const struct
{
    const GUID *subtype;
    UINT32 depth;
    enum wg_audio_format format;
}
audio_formats[] =
{
    {&MFAudioFormat_PCM,     8, WG_AUDIO_FORMAT_U8},
    {&MFAudioFormat_PCM,    16, WG_AUDIO_FORMAT_S16LE},
    {&MFAudioFormat_PCM,    24, WG_AUDIO_FORMAT_S24LE},
    {&MFAudioFormat_PCM,    32, WG_AUDIO_FORMAT_S32LE},
    {&MFAudioFormat_Float,  32, WG_AUDIO_FORMAT_F32LE},
    {&MFAudioFormat_Float,  64, WG_AUDIO_FORMAT_F64LE},
};

static inline UINT64 make_uint64(UINT32 high, UINT32 low)
{
    return ((UINT64)high << 32) | low;
}

static IMFMediaType *mf_media_type_from_wg_format_audio(const struct wg_format *format)
{
    IMFMediaType *type;
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(audio_formats); ++i)
    {
        if (format->u.audio.format == audio_formats[i].format)
        {
            if (FAILED(MFCreateMediaType(&type)))
                return NULL;

            IMFMediaType_SetGUID(type, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
            IMFMediaType_SetGUID(type, &MF_MT_SUBTYPE, audio_formats[i].subtype);
            IMFMediaType_SetUINT32(type, &MF_MT_AUDIO_BITS_PER_SAMPLE, audio_formats[i].depth);
            IMFMediaType_SetUINT32(type, &MF_MT_AUDIO_SAMPLES_PER_SECOND, format->u.audio.rate);
            IMFMediaType_SetUINT32(type, &MF_MT_AUDIO_NUM_CHANNELS, format->u.audio.channels);
            IMFMediaType_SetUINT32(type, &MF_MT_AUDIO_CHANNEL_MASK, format->u.audio.channel_mask);
            IMFMediaType_SetUINT32(type, &MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);
            IMFMediaType_SetUINT32(type, &MF_MT_AUDIO_BLOCK_ALIGNMENT, format->u.audio.channels * audio_formats[i].depth / 8);

            return type;
        }
    }

    return NULL;
}

static IMFMediaType *mf_media_type_from_wg_format_video(const struct wg_format *format)
{
    IMFMediaType *type;
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(video_formats); ++i)
    {
        if (format->u.video.format == video_formats[i].format)
        {
            if (FAILED(MFCreateMediaType(&type)))
                return NULL;

            IMFMediaType_SetGUID(type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
            IMFMediaType_SetGUID(type, &MF_MT_SUBTYPE, video_formats[i].subtype);
            IMFMediaType_SetUINT64(type, &MF_MT_FRAME_SIZE,
                    make_uint64(format->u.video.width, format->u.video.height));
            IMFMediaType_SetUINT64(type, &MF_MT_FRAME_RATE,
                    make_uint64(format->u.video.fps_n, format->u.video.fps_d));
            IMFMediaType_SetUINT32(type, &MF_MT_COMPRESSED, FALSE);
            IMFMediaType_SetUINT32(type, &MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);
            IMFMediaType_SetUINT32(type, &MF_MT_VIDEO_ROTATION, MFVideoRotationFormat_0);

            if (!IsRectEmpty(&format->u.video.padding))
            {
                MFVideoArea aperture =
                {
                    .OffsetX = {.value = format->u.video.padding.left},
                    .OffsetY = {.value = format->u.video.padding.top},
                    .Area.cx = format->u.video.width - format->u.video.padding.right - format->u.video.padding.left,
                    .Area.cy = format->u.video.height - format->u.video.padding.bottom - format->u.video.padding.top,
                };

                IMFMediaType_SetBlob(type, &MF_MT_MINIMUM_DISPLAY_APERTURE,
                        (BYTE *)&aperture, sizeof(aperture));
            }

            return type;
        }
    }

    return NULL;
}

IMFMediaType *mf_media_type_from_wg_format(const struct wg_format *format)
{
    switch (format->major_type)
    {
        case WG_MAJOR_TYPE_H264:
        case WG_MAJOR_TYPE_WMA:
        case WG_MAJOR_TYPE_MPEG1_AUDIO:
            FIXME("Format %u not implemented!\n", format->major_type);
            /* fallthrough */
        case WG_MAJOR_TYPE_UNKNOWN:
            return NULL;

        case WG_MAJOR_TYPE_AUDIO:
            return mf_media_type_from_wg_format_audio(format);

        case WG_MAJOR_TYPE_VIDEO:
            return mf_media_type_from_wg_format_video(format);
    }

    assert(0);
    return NULL;
}

static void mf_media_type_to_wg_format_audio(IMFMediaType *type, const GUID *subtype, struct wg_format *format)
{
    UINT32 rate, channels, channel_mask, depth;
    unsigned int i;

    if (FAILED(IMFMediaType_GetUINT32(type, &MF_MT_AUDIO_SAMPLES_PER_SECOND, &rate)))
    {
        FIXME("Sample rate is not set.\n");
        return;
    }
    if (FAILED(IMFMediaType_GetUINT32(type, &MF_MT_AUDIO_NUM_CHANNELS, &channels)))
    {
        FIXME("Channel count is not set.\n");
        return;
    }
    if (FAILED(IMFMediaType_GetUINT32(type, &MF_MT_AUDIO_BITS_PER_SAMPLE, &depth)))
    {
        FIXME("Depth is not set.\n");
        return;
    }
    if (FAILED(IMFMediaType_GetUINT32(type, &MF_MT_AUDIO_CHANNEL_MASK, &channel_mask)))
    {
        if (channels == 1)
            channel_mask = KSAUDIO_SPEAKER_MONO;
        else if (channels == 2)
            channel_mask = KSAUDIO_SPEAKER_STEREO;
        else
        {
            FIXME("Channel mask is not set.\n");
            return;
        }
    }

    format->major_type = WG_MAJOR_TYPE_AUDIO;
    format->u.audio.channels = channels;
    format->u.audio.channel_mask = channel_mask;
    format->u.audio.rate = rate;

    for (i = 0; i < ARRAY_SIZE(audio_formats); ++i)
    {
        if (IsEqualGUID(subtype, audio_formats[i].subtype) && depth == audio_formats[i].depth)
        {
            format->u.audio.format = audio_formats[i].format;
            return;
        }
    }
    FIXME("Unrecognized audio subtype %s, depth %u.\n", debugstr_guid(subtype), depth);
}

static void mf_media_type_to_wg_format_video(IMFMediaType *type, const GUID *subtype, struct wg_format *format)
{
    UINT64 frame_rate, frame_size;
    MFVideoArea aperture;
    unsigned int i;
    UINT32 size;

    if (FAILED(IMFMediaType_GetUINT64(type, &MF_MT_FRAME_SIZE, &frame_size)))
    {
        FIXME("Frame size is not set.\n");
        return;
    }

    format->major_type = WG_MAJOR_TYPE_VIDEO;
    format->u.video.width = (UINT32)(frame_size >> 32);
    format->u.video.height = (UINT32)frame_size;
    format->u.video.fps_n = 1;
    format->u.video.fps_d = 1;

    if (SUCCEEDED(IMFMediaType_GetBlob(type, &MF_MT_MINIMUM_DISPLAY_APERTURE, (BYTE *)&aperture,
            sizeof(aperture), &size)) && size == sizeof(aperture))
    {
        format->u.video.padding.left = aperture.OffsetX.value;
        format->u.video.padding.top = aperture.OffsetY.value;
        format->u.video.padding.right = format->u.video.width - aperture.Area.cx - aperture.OffsetX.value;
        format->u.video.padding.bottom = format->u.video.height - aperture.Area.cy - aperture.OffsetY.value;
    }

    if (SUCCEEDED(IMFMediaType_GetUINT64(type, &MF_MT_FRAME_RATE, &frame_rate)) && (UINT32)frame_rate)
    {
        format->u.video.fps_n = (UINT32)(frame_rate >> 32);
        format->u.video.fps_d = (UINT32)frame_rate;
    }

    for (i = 0; i < ARRAY_SIZE(video_formats); ++i)
    {
        if (IsEqualGUID(subtype, video_formats[i].subtype))
        {
            format->u.video.format = video_formats[i].format;
            return;
        }
    }
    FIXME("Unrecognized video subtype %s.\n", debugstr_guid(subtype));
}

static void mf_media_type_to_wg_format_wma(IMFMediaType *type, const GUID *subtype, struct wg_format *format)
{
    UINT32 rate, depth, channels, block_align, bytes_per_second, codec_data_len;
    BYTE codec_data[64];
    UINT32 version;

    if (FAILED(IMFMediaType_GetUINT32(type, &MF_MT_AUDIO_SAMPLES_PER_SECOND, &rate)))
    {
        FIXME("Sample rate is not set.\n");
        return;
    }
    if (FAILED(IMFMediaType_GetUINT32(type, &MF_MT_AUDIO_NUM_CHANNELS, &channels)))
    {
        FIXME("Channel count is not set.\n");
        return;
    }
    if (FAILED(IMFMediaType_GetUINT32(type, &MF_MT_AUDIO_BLOCK_ALIGNMENT, &block_align)))
    {
        FIXME("Block alignment is not set.\n");
        return;
    }
    if (FAILED(IMFMediaType_GetUINT32(type, &MF_MT_AUDIO_BITS_PER_SAMPLE, &depth)))
    {
        FIXME("Depth is not set.\n");
        return;
    }
    if (FAILED(IMFMediaType_GetBlob(type, &MF_MT_USER_DATA, codec_data, sizeof(codec_data), &codec_data_len)))
    {
        FIXME("Codec data is not set.\n");
        return;
    }
    if (FAILED(IMFMediaType_GetUINT32(type, &MF_MT_AUDIO_AVG_BYTES_PER_SECOND, &bytes_per_second)))
    {
        FIXME("Bitrate is not set.\n");
        bytes_per_second = 0;
    }

    if (IsEqualGUID(subtype, &MEDIASUBTYPE_MSAUDIO1))
        version = 1;
    else if (IsEqualGUID(subtype, &MFAudioFormat_WMAudioV8))
        version = 2;
    else if (IsEqualGUID(subtype, &MFAudioFormat_WMAudioV9))
        version = 3;
    else if (IsEqualGUID(subtype, &MFAudioFormat_WMAudio_Lossless))
        version = 4;
    else
    {
        assert(0);
        return;
    }

    format->major_type = WG_MAJOR_TYPE_WMA;
    format->u.wma.version = version;
    format->u.wma.bitrate = bytes_per_second * 8;
    format->u.wma.rate = rate;
    format->u.wma.depth = depth;
    format->u.wma.channels = channels;
    format->u.wma.block_align = block_align;
    format->u.wma.codec_data_len = codec_data_len;
    memcpy(format->u.wma.codec_data, codec_data, codec_data_len);
}

static void mf_media_type_to_wg_format_h264(IMFMediaType *type, struct wg_format *format)
{
    UINT64 frame_rate, frame_size;
    UINT32 profile, level;

    memset(format, 0, sizeof(*format));
    format->major_type = WG_MAJOR_TYPE_H264;

    if (SUCCEEDED(IMFMediaType_GetUINT64(type, &MF_MT_FRAME_SIZE, &frame_size)))
    {
        format->u.h264.width = frame_size >> 32;
        format->u.h264.height = (UINT32)frame_size;
    }

    if (SUCCEEDED(IMFMediaType_GetUINT64(type, &MF_MT_FRAME_RATE, &frame_rate)) && (UINT32)frame_rate)
    {
        format->u.h264.fps_n = frame_rate >> 32;
        format->u.h264.fps_d = (UINT32)frame_rate;
    }
    else
    {
        format->u.h264.fps_n = 1;
        format->u.h264.fps_d = 1;
    }

    if (SUCCEEDED(IMFMediaType_GetUINT32(type, &MF_MT_MPEG2_PROFILE, &profile)))
        format->u.h264.profile = profile;

    if (SUCCEEDED(IMFMediaType_GetUINT32(type, &MF_MT_MPEG2_LEVEL, &level)))
        format->u.h264.level = level;
}

void mf_media_type_to_wg_format(IMFMediaType *type, struct wg_format *format)
{
    GUID major_type, subtype;

    memset(format, 0, sizeof(*format));

    if (FAILED(IMFMediaType_GetMajorType(type, &major_type)))
    {
        FIXME("Major type is not set.\n");
        return;
    }
    if (FAILED(IMFMediaType_GetGUID(type, &MF_MT_SUBTYPE, &subtype)))
    {
        FIXME("Subtype is not set.\n");
        return;
    }

    if (IsEqualGUID(&major_type, &MFMediaType_Audio))
    {
        if (IsEqualGUID(&subtype, &MEDIASUBTYPE_MSAUDIO1) ||
                IsEqualGUID(&subtype, &MFAudioFormat_WMAudioV8) ||
                IsEqualGUID(&subtype, &MFAudioFormat_WMAudioV9) ||
                IsEqualGUID(&subtype, &MFAudioFormat_WMAudio_Lossless))
            mf_media_type_to_wg_format_wma(type, &subtype, format);
        else
            mf_media_type_to_wg_format_audio(type, &subtype, format);
    }
    else if (IsEqualGUID(&major_type, &MFMediaType_Video))
    {
        if (IsEqualGUID(&subtype, &MFVideoFormat_H264))
            mf_media_type_to_wg_format_h264(type, format);
        else
            mf_media_type_to_wg_format_video(type, &subtype, format);
    }
    else
        FIXME("Unrecognized major type %s.\n", debugstr_guid(&major_type));
}

struct mf_sample
{
    IMFSample *sample;
    IMFMediaBuffer *media_buffer;
    struct wg_sample wg_sample;
};

HRESULT mf_create_wg_sample(IMFSample *sample, struct wg_sample **out)
{
    DWORD current_length, max_length;
    struct mf_sample *mf_sample;
    BYTE *buffer;
    HRESULT hr;

    if (!(mf_sample = calloc(1, sizeof(*mf_sample))))
        return E_OUTOFMEMORY;
    if (FAILED(hr = IMFSample_ConvertToContiguousBuffer(sample, &mf_sample->media_buffer)))
        goto out;
    if (FAILED(hr = IMFMediaBuffer_Lock(mf_sample->media_buffer, &buffer, &max_length, &current_length)))
        goto out;

    IMFSample_AddRef((mf_sample->sample = sample));
    mf_sample->wg_sample.data = buffer;
    mf_sample->wg_sample.size = current_length;
    mf_sample->wg_sample.max_size = max_length;

    TRACE("Created mf_sample %p for sample %p.\n", mf_sample, sample);
    *out = &mf_sample->wg_sample;
    return S_OK;

out:
    if (mf_sample->media_buffer)
        IMFMediaBuffer_Release(mf_sample->media_buffer);
    free(mf_sample);
    return hr;
}

void mf_destroy_wg_sample(struct wg_sample *wg_sample)
{
    struct mf_sample *mf_sample = CONTAINING_RECORD(wg_sample, struct mf_sample, wg_sample);

    IMFMediaBuffer_Unlock(mf_sample->media_buffer);
    IMFMediaBuffer_Release(mf_sample->media_buffer);
    IMFSample_Release(mf_sample->sample);
    free(mf_sample);
}

HRESULT wg_transform_push_mf(struct wg_transform *transform, struct wg_sample *sample)
{
    struct mf_sample *mf_sample = CONTAINING_RECORD(sample, struct mf_sample, wg_sample);
    LONGLONG time, duration;
    UINT32 value;

    if (SUCCEEDED(IMFSample_GetSampleTime(mf_sample->sample, &time)))
    {
        mf_sample->wg_sample.flags |= WG_SAMPLE_FLAG_HAS_PTS;
        mf_sample->wg_sample.pts = time;
    }
    if (SUCCEEDED(IMFSample_GetSampleDuration(mf_sample->sample, &duration)))
    {
        mf_sample->wg_sample.flags |= WG_SAMPLE_FLAG_HAS_DURATION;
        mf_sample->wg_sample.duration = duration;
    }
    if (SUCCEEDED(IMFSample_GetUINT32(mf_sample->sample, &MFSampleExtension_CleanPoint, &value)) && value)
        mf_sample->wg_sample.flags |= WG_SAMPLE_FLAG_SYNC_POINT;

    return wg_transform_push_data(transform, sample);
}

HRESULT wg_transform_read_mf(struct wg_transform *transform, struct wg_sample *wg_sample,
        struct wg_format *format)
{
    struct mf_sample *mf_sample = CONTAINING_RECORD(wg_sample, struct mf_sample, wg_sample);
    HRESULT hr;

    if (FAILED(hr = wg_transform_read_data(transform, wg_sample, format)))
        return hr;

    IMFMediaBuffer_SetCurrentLength(mf_sample->media_buffer, wg_sample->size);

    if (wg_sample->flags & WG_SAMPLE_FLAG_HAS_PTS)
        IMFSample_SetSampleTime(mf_sample->sample, wg_sample->pts);
    if (wg_sample->flags & WG_SAMPLE_FLAG_HAS_DURATION)
        IMFSample_SetSampleDuration(mf_sample->sample, wg_sample->duration);
    if (wg_sample->flags & WG_SAMPLE_FLAG_SYNC_POINT)
        IMFSample_SetUINT32(mf_sample->sample, &MFSampleExtension_CleanPoint, 1);

    return S_OK;
}
