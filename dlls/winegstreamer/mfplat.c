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
#include "d3d9types.h"
#include "mfapi.h"
#include "mmreg.h"

#include "wine/debug.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

DEFINE_GUID(DMOVideoFormat_RGB32,D3DFMT_X8R8G8B8,0x524f,0x11ce,0x9f,0x53,0x00,0x20,0xaf,0x0b,0xa7,0x70);
DEFINE_GUID(DMOVideoFormat_RGB24,D3DFMT_R8G8B8,0x524f,0x11ce,0x9f,0x53,0x00,0x20,0xaf,0x0b,0xa7,0x70);
DEFINE_GUID(DMOVideoFormat_RGB565,D3DFMT_R5G6B5,0x524f,0x11ce,0x9f,0x53,0x00,0x20,0xaf,0x0b,0xa7,0x70);
DEFINE_GUID(DMOVideoFormat_RGB555,D3DFMT_X1R5G5B5,0x524f,0x11ce,0x9f,0x53,0x00,0x20,0xaf,0x0b,0xa7,0x70);
DEFINE_GUID(DMOVideoFormat_RGB8,D3DFMT_P8,0x524f,0x11ce,0x9f,0x53,0x00,0x20,0xaf,0x0b,0xa7,0x70);
DEFINE_MEDIATYPE_GUID(MFAudioFormat_RAW_AAC,WAVE_FORMAT_RAW_AAC1);
DEFINE_MEDIATYPE_GUID(MFVideoFormat_VC1S,MAKEFOURCC('V','C','1','S'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_IV50,MAKEFOURCC('I','V','5','0'));

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

static const GUID CLSID_GStreamerByteStreamHandler = {0x317df618, 0x5e5a, 0x468a, {0x9f, 0x15, 0xd8, 0x27, 0xa9, 0xa0, 0x81, 0x62}};

static const struct class_object
{
    const GUID *clsid;
    HRESULT (*create_instance)(REFIID riid, void **obj);
}
class_objects[] =
{
    { &CLSID_VideoProcessorMFT, &video_processor_create },
    { &CLSID_GStreamerByteStreamHandler, &gstreamer_byte_stream_handler_create },
    { &CLSID_MSAACDecMFT, &aac_decoder_create },
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
    MFT_REGISTER_TYPE_INFO resampler_types[] =
    {
        {MFMediaType_Audio, MFAudioFormat_PCM},
        {MFMediaType_Audio, MFAudioFormat_Float},
    };

    MFT_REGISTER_TYPE_INFO aac_decoder_input_types[] =
    {
        {MFMediaType_Audio, MFAudioFormat_AAC},
        {MFMediaType_Audio, MFAudioFormat_RAW_AAC},
        {MFMediaType_Audio, MFAudioFormat_ADTS},
    };
    MFT_REGISTER_TYPE_INFO aac_decoder_output_types[] =
    {
        {MFMediaType_Audio, MFAudioFormat_Float},
        {MFMediaType_Audio, MFAudioFormat_PCM},
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

    MFT_REGISTER_TYPE_INFO wmv_decoder_input_types[] =
    {
        {MFMediaType_Video, MFVideoFormat_WMV1},
        {MFMediaType_Video, MFVideoFormat_WMV2},
        {MFMediaType_Video, MFVideoFormat_WMV3},
        {MFMediaType_Video, MEDIASUBTYPE_WMVP},
        {MFMediaType_Video, MEDIASUBTYPE_WVP2},
        {MFMediaType_Video, MEDIASUBTYPE_WMVR},
        {MFMediaType_Video, MEDIASUBTYPE_WMVA},
        {MFMediaType_Video, MFVideoFormat_WVC1},
        {MFMediaType_Video, MFVideoFormat_VC1S},
    };
    MFT_REGISTER_TYPE_INFO wmv_decoder_output_types[] =
    {
        {MFMediaType_Video, MFVideoFormat_YV12},
        {MFMediaType_Video, MFVideoFormat_YUY2},
        {MFMediaType_Video, MFVideoFormat_UYVY},
        {MFMediaType_Video, MFVideoFormat_YVYU},
        {MFMediaType_Video, MFVideoFormat_NV11},
        {MFMediaType_Video, MFVideoFormat_NV12},
        {MFMediaType_Video, DMOVideoFormat_RGB32},
        {MFMediaType_Video, DMOVideoFormat_RGB24},
        {MFMediaType_Video, DMOVideoFormat_RGB565},
        {MFMediaType_Video, DMOVideoFormat_RGB555},
        {MFMediaType_Video, DMOVideoFormat_RGB8},
    };

    MFT_REGISTER_TYPE_INFO color_convert_input_types[] =
    {
        {MFMediaType_Video, MFVideoFormat_YV12},
        {MFMediaType_Video, MFVideoFormat_YUY2},
        {MFMediaType_Video, MFVideoFormat_UYVY},
        {MFMediaType_Video, MFVideoFormat_AYUV},
        {MFMediaType_Video, MFVideoFormat_NV12},
        {MFMediaType_Video, DMOVideoFormat_RGB32},
        {MFMediaType_Video, DMOVideoFormat_RGB565},
        {MFMediaType_Video, MFVideoFormat_I420},
        {MFMediaType_Video, MFVideoFormat_IYUV},
        {MFMediaType_Video, MFVideoFormat_YVYU},
        {MFMediaType_Video, DMOVideoFormat_RGB24},
        {MFMediaType_Video, DMOVideoFormat_RGB555},
        {MFMediaType_Video, DMOVideoFormat_RGB8},
        {MFMediaType_Video, MEDIASUBTYPE_V216},
        {MFMediaType_Video, MEDIASUBTYPE_V410},
        {MFMediaType_Video, MFVideoFormat_NV11},
        {MFMediaType_Video, MFVideoFormat_Y41P},
        {MFMediaType_Video, MFVideoFormat_Y41T},
        {MFMediaType_Video, MFVideoFormat_Y42T},
        {MFMediaType_Video, MFVideoFormat_YVU9},
    };
    MFT_REGISTER_TYPE_INFO color_convert_output_types[] =
    {
        {MFMediaType_Video, MFVideoFormat_YV12},
        {MFMediaType_Video, MFVideoFormat_YUY2},
        {MFMediaType_Video, MFVideoFormat_UYVY},
        {MFMediaType_Video, MFVideoFormat_AYUV},
        {MFMediaType_Video, MFVideoFormat_NV12},
        {MFMediaType_Video, DMOVideoFormat_RGB32},
        {MFMediaType_Video, DMOVideoFormat_RGB565},
        {MFMediaType_Video, MFVideoFormat_I420},
        {MFMediaType_Video, MFVideoFormat_IYUV},
        {MFMediaType_Video, MFVideoFormat_YVYU},
        {MFMediaType_Video, DMOVideoFormat_RGB24},
        {MFMediaType_Video, DMOVideoFormat_RGB555},
        {MFMediaType_Video, DMOVideoFormat_RGB8},
        {MFMediaType_Video, MEDIASUBTYPE_V216},
        {MFMediaType_Video, MEDIASUBTYPE_V410},
        {MFMediaType_Video, MFVideoFormat_NV11},
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
            CLSID_MSAACDecMFT,
            MFT_CATEGORY_AUDIO_DECODER,
            L"Microsoft AAC Audio Decoder MFT",
            MFT_ENUM_FLAG_SYNCMFT,
            ARRAY_SIZE(aac_decoder_input_types),
            aac_decoder_input_types,
            ARRAY_SIZE(aac_decoder_output_types),
            aac_decoder_output_types,
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
            CLSID_WMVDecoderMFT,
            MFT_CATEGORY_VIDEO_DECODER,
            L"WMVideo Decoder MFT",
            MFT_ENUM_FLAG_SYNCMFT,
            ARRAY_SIZE(wmv_decoder_input_types),
            wmv_decoder_input_types,
            ARRAY_SIZE(wmv_decoder_output_types),
            wmv_decoder_output_types,
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
        {
            CLSID_CResamplerMediaObject,
            MFT_CATEGORY_AUDIO_EFFECT,
            L"Resampler MFT",
            MFT_ENUM_FLAG_SYNCMFT,
            ARRAY_SIZE(resampler_types),
            resampler_types,
            ARRAY_SIZE(resampler_types),
            resampler_types,
        },
        {
            CLSID_CColorConvertDMO,
            MFT_CATEGORY_VIDEO_EFFECT,
            L"Color Converter MFT",
            MFT_ENUM_FLAG_SYNCMFT,
            ARRAY_SIZE(color_convert_input_types),
            color_convert_input_types,
            ARRAY_SIZE(color_convert_output_types),
            color_convert_output_types,
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
    unsigned int i, block_align;
    IMFMediaType *type;

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

            block_align = format->u.audio.channels * audio_formats[i].depth / 8;
            IMFMediaType_SetUINT32(type, &MF_MT_AUDIO_BLOCK_ALIGNMENT, block_align);
            IMFMediaType_SetUINT32(type, &MF_MT_AUDIO_AVG_BYTES_PER_SECOND, block_align * format->u.audio.rate);

            return type;
        }
    }

    FIXME("Unknown audio format %#x.\n", format->u.audio.format);
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
            unsigned int stride = wg_format_get_stride(format);
            int32_t height = abs(format->u.video.height);
            int32_t width = format->u.video.width;

            if (FAILED(MFCreateMediaType(&type)))
                return NULL;

            IMFMediaType_SetGUID(type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
            IMFMediaType_SetGUID(type, &MF_MT_SUBTYPE, video_formats[i].subtype);
            IMFMediaType_SetUINT64(type, &MF_MT_FRAME_SIZE, make_uint64(width, height));
            IMFMediaType_SetUINT64(type, &MF_MT_FRAME_RATE,
                    make_uint64(format->u.video.fps_n, format->u.video.fps_d));
            IMFMediaType_SetUINT32(type, &MF_MT_COMPRESSED, FALSE);
            IMFMediaType_SetUINT32(type, &MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);
            IMFMediaType_SetUINT32(type, &MF_MT_VIDEO_ROTATION, MFVideoRotationFormat_0);

            if (format->u.video.height < 0)
                stride = -stride;
            IMFMediaType_SetUINT32(type, &MF_MT_DEFAULT_STRIDE, stride);

            if (format->u.video.padding.left || format->u.video.padding.right
                || format->u.video.padding.top || format->u.video.padding.bottom)
            {
                MFVideoArea aperture =
                {
                    .OffsetX = {.value = format->u.video.padding.left},
                    .OffsetY = {.value = format->u.video.padding.top},
                    .Area.cx = width - format->u.video.padding.right - format->u.video.padding.left,
                    .Area.cy = height - format->u.video.padding.bottom - format->u.video.padding.top,
                };

                IMFMediaType_SetBlob(type, &MF_MT_MINIMUM_DISPLAY_APERTURE,
                        (BYTE *)&aperture, sizeof(aperture));
            }

            return type;
        }
    }

    FIXME("Unknown video format %#x.\n", format->u.video.format);
    return NULL;
}

IMFMediaType *mf_media_type_from_wg_format(const struct wg_format *format)
{
    switch (format->major_type)
    {
        case WG_MAJOR_TYPE_AUDIO_MPEG1:
        case WG_MAJOR_TYPE_AUDIO_MPEG4:
        case WG_MAJOR_TYPE_AUDIO_WMA:
        case WG_MAJOR_TYPE_VIDEO_CINEPAK:
        case WG_MAJOR_TYPE_VIDEO_H264:
        case WG_MAJOR_TYPE_VIDEO_WMV:
        case WG_MAJOR_TYPE_VIDEO_INDEO:
        case WG_MAJOR_TYPE_VIDEO_MPEG1:
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

static void mf_media_type_to_wg_format_audio_mpeg4(IMFMediaType *type, const GUID *subtype, struct wg_format *format)
{
    BYTE buffer[sizeof(HEAACWAVEFORMAT) + 64];
    HEAACWAVEFORMAT *wfx = (HEAACWAVEFORMAT *)buffer;
    UINT32 codec_data_size;
    BOOL raw_aac;

    wfx->wfInfo.wfx.cbSize = sizeof(buffer) - sizeof(wfx->wfInfo.wfx);
    if (FAILED(IMFMediaType_GetBlob(type, &MF_MT_USER_DATA, (BYTE *)(&wfx->wfInfo.wfx + 1),
            wfx->wfInfo.wfx.cbSize, &codec_data_size)))
    {
        FIXME("Codec data is not set.\n");
        return;
    }

    raw_aac = IsEqualGUID(subtype, &MFAudioFormat_RAW_AAC);
    if (!raw_aac)
        codec_data_size -= min(codec_data_size, sizeof(HEAACWAVEINFO) - sizeof(WAVEFORMATEX));
    if (codec_data_size > sizeof(format->u.audio_mpeg4.codec_data))
    {
        FIXME("Codec data needs %u bytes.\n", codec_data_size);
        return;
    }
    if (raw_aac)
        memcpy(format->u.audio_mpeg4.codec_data, (BYTE *)(&wfx->wfInfo.wfx + 1), codec_data_size);
    else
        memcpy(format->u.audio_mpeg4.codec_data, wfx->pbAudioSpecificConfig, codec_data_size);

    format->major_type = WG_MAJOR_TYPE_AUDIO_MPEG4;

    if (FAILED(IMFMediaType_GetUINT32(type, &MF_MT_AAC_PAYLOAD_TYPE, &format->u.audio_mpeg4.payload_type)))
        format->u.audio_mpeg4.payload_type = 0;

    format->u.audio_mpeg4.codec_data_len = codec_data_size;
}

static enum wg_video_format mf_video_format_to_wg(const GUID *subtype)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(video_formats); ++i)
    {
        if (IsEqualGUID(subtype, video_formats[i].subtype))
            return video_formats[i].format;
    }
    FIXME("Unrecognized video subtype %s.\n", debugstr_guid(subtype));
    return WG_VIDEO_FORMAT_UNKNOWN;
}

static void mf_media_type_to_wg_format_video(IMFMediaType *type, const GUID *subtype, struct wg_format *format)
{
    UINT64 frame_rate, frame_size;
    MFVideoArea aperture;
    UINT32 size, stride;

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

    format->u.video.format = mf_video_format_to_wg(subtype);

    if (SUCCEEDED(IMFMediaType_GetUINT32(type, &MF_MT_DEFAULT_STRIDE, &stride)))
    {
        if ((int)stride < 0)
            format->u.video.height = -format->u.video.height;
    }
    else if (wg_video_format_is_rgb(format->u.video.format))
    {
        format->u.video.height = -format->u.video.height;
    }
}

static void mf_media_type_to_wg_format_audio_wma(IMFMediaType *type, const GUID *subtype, struct wg_format *format)
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

    format->major_type = WG_MAJOR_TYPE_AUDIO_WMA;
    format->u.audio_wma.version = version;
    format->u.audio_wma.bitrate = bytes_per_second * 8;
    format->u.audio_wma.rate = rate;
    format->u.audio_wma.depth = depth;
    format->u.audio_wma.channels = channels;
    format->u.audio_wma.block_align = block_align;
    format->u.audio_wma.codec_data_len = codec_data_len;
    memcpy(format->u.audio_wma.codec_data, codec_data, codec_data_len);
}

static void mf_media_type_to_wg_format_video_h264(IMFMediaType *type, struct wg_format *format)
{
    UINT32 profile, level, codec_data_len;
    UINT64 frame_rate, frame_size;
    BYTE *codec_data;

    memset(format, 0, sizeof(*format));
    format->major_type = WG_MAJOR_TYPE_VIDEO_H264;

    if (SUCCEEDED(IMFMediaType_GetUINT64(type, &MF_MT_FRAME_SIZE, &frame_size)))
    {
        format->u.video_h264.width = frame_size >> 32;
        format->u.video_h264.height = (UINT32)frame_size;
    }

    if (SUCCEEDED(IMFMediaType_GetUINT64(type, &MF_MT_FRAME_RATE, &frame_rate)) && (UINT32)frame_rate)
    {
        format->u.video_h264.fps_n = frame_rate >> 32;
        format->u.video_h264.fps_d = (UINT32)frame_rate;
    }
    else
    {
        format->u.video_h264.fps_n = 1;
        format->u.video_h264.fps_d = 1;
    }

    if (SUCCEEDED(IMFMediaType_GetUINT32(type, &MF_MT_MPEG2_PROFILE, &profile)))
        format->u.video_h264.profile = profile;

    if (SUCCEEDED(IMFMediaType_GetUINT32(type, &MF_MT_MPEG2_LEVEL, &level)))
        format->u.video_h264.level = level;

    if (SUCCEEDED(IMFMediaType_GetAllocatedBlob(type, &MF_MT_MPEG_SEQUENCE_HEADER, &codec_data, &codec_data_len)))
    {
        if (codec_data_len <= sizeof(format->u.video_h264.codec_data))
        {
            format->u.video_h264.codec_data_len = codec_data_len;
            memcpy(format->u.video_h264.codec_data, codec_data, codec_data_len);
        }
        else
        {
            WARN("Codec data buffer too small, codec data size %u.\n", codec_data_len);
        }
        CoTaskMemFree(codec_data);
    }
}

static void mf_media_type_to_wg_format_video_indeo(IMFMediaType *type, uint32_t version, struct wg_format *format)
{
    UINT64 frame_rate, frame_size;

    memset(format, 0, sizeof(*format));
    format->major_type = WG_MAJOR_TYPE_VIDEO_INDEO;

    if (SUCCEEDED(IMFMediaType_GetUINT64(type, &MF_MT_FRAME_SIZE, &frame_size)))
    {
        format->u.video_indeo.width = frame_size >> 32;
        format->u.video_indeo.height = (UINT32)frame_size;
    }

    if (SUCCEEDED(IMFMediaType_GetUINT64(type, &MF_MT_FRAME_RATE, &frame_rate)) && (UINT32)frame_rate)
    {
        format->u.video_indeo.fps_n = frame_rate >> 32;
        format->u.video_indeo.fps_d = (UINT32)frame_rate;
    }
    else
    {
        format->u.video_indeo.fps_n = 1;
        format->u.video_indeo.fps_d = 1;
    }

    format->u.video_indeo.version = version;
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
            mf_media_type_to_wg_format_audio_wma(type, &subtype, format);
        else if (IsEqualGUID(&subtype, &MFAudioFormat_AAC) || IsEqualGUID(&subtype, &MFAudioFormat_RAW_AAC))
            mf_media_type_to_wg_format_audio_mpeg4(type, &subtype, format);
        else
            mf_media_type_to_wg_format_audio(type, &subtype, format);
    }
    else if (IsEqualGUID(&major_type, &MFMediaType_Video))
    {
        if (IsEqualGUID(&subtype, &MFVideoFormat_H264))
            mf_media_type_to_wg_format_video_h264(type, format);
        else if (IsEqualGUID(&subtype, &MFVideoFormat_IV50))
            mf_media_type_to_wg_format_video_indeo(type, 5, format);
        else
            mf_media_type_to_wg_format_video(type, &subtype, format);
    }
    else
        FIXME("Unrecognized major type %s.\n", debugstr_guid(&major_type));
}
