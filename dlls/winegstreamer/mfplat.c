/*
 * Copyright 2019 Nikolay Sivov for CodeWeavers
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

#include "config.h"
#include <gst/gst.h>

#include "gst_private.h"

#include <stdarg.h>

#include "gst_private.h"
#include "mfapi.h"
#include "mfidl.h"

#include "wine/debug.h"
#include "wine/heap.h"

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

    TRACE("%p, refcount %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI video_processor_Release(IMFTransform *iface)
{
    struct video_processor *transform = impl_video_processor_from_IMFTransform(iface);
    ULONG refcount = InterlockedDecrement(&transform->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    if (!refcount)
    {
        if (transform->attributes)
            IMFAttributes_Release(transform->attributes);
        if (transform->output_attributes)
            IMFAttributes_Release(transform->output_attributes);
        heap_free(transform);
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

    TRACE("%p, %u, %p.\n", iface, id, attributes);

    *attributes = transform->output_attributes;
    IMFAttributes_AddRef(*attributes);

    return S_OK;
}

static HRESULT WINAPI video_processor_DeleteInputStream(IMFTransform *iface, DWORD id)
{
    TRACE("%p, %u.\n", iface, id);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_processor_AddInputStreams(IMFTransform *iface, DWORD streams, DWORD *ids)
{
    TRACE("%p, %u, %p.\n", iface, streams, ids);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_processor_GetInputAvailableType(IMFTransform *iface, DWORD id, DWORD index,
        IMFMediaType **type)
{
    FIXME("%p, %u, %u, %p.\n", iface, id, index, type);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_processor_GetOutputAvailableType(IMFTransform *iface, DWORD id, DWORD index,
        IMFMediaType **type)
{
    FIXME("%p, %u, %u, %p.\n", iface, id, index, type);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_processor_SetInputType(IMFTransform *iface, DWORD id, IMFMediaType *type, DWORD flags)
{
    FIXME("%p, %u, %p, %#x.\n", iface, id, type, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_processor_SetOutputType(IMFTransform *iface, DWORD id, IMFMediaType *type, DWORD flags)
{
    FIXME("%p, %u, %p, %#x.\n", iface, id, type, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_processor_GetInputCurrentType(IMFTransform *iface, DWORD id, IMFMediaType **type)
{
    FIXME("%p, %u, %p.\n", iface, id, type);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_processor_GetOutputCurrentType(IMFTransform *iface, DWORD id, IMFMediaType **type)
{
    FIXME("%p, %u, %p.\n", iface, id, type);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_processor_GetInputStatus(IMFTransform *iface, DWORD id, DWORD *flags)
{
    FIXME("%p, %u, %p.\n", iface, id, flags);

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
    TRACE("%p, %u, %p.\n", iface, id, event);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_processor_ProcessMessage(IMFTransform *iface, MFT_MESSAGE_TYPE message, ULONG_PTR param)
{
    FIXME("%p, %u.\n", iface, message);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_processor_ProcessInput(IMFTransform *iface, DWORD id, IMFSample *sample, DWORD flags)
{
    FIXME("%p, %u, %p, %#x.\n", iface, id, sample, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_processor_ProcessOutput(IMFTransform *iface, DWORD flags, DWORD count,
        MFT_OUTPUT_DATA_BUFFER *samples, DWORD *status)
{
    FIXME("%p, %#x, %u, %p, %p.\n", iface, flags, count, samples, status);

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
        heap_free(factory);

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

    if (dolock)
        InterlockedIncrement(&object_locks);
    else
        InterlockedDecrement(&object_locks);

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

    if (!(object = heap_alloc_zero(sizeof(*object))))
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

static const struct class_object
{
    const GUID *clsid;
    HRESULT (*create_instance)(REFIID riid, void **obj);
}
class_objects[] =
{
    { &CLSID_VideoProcessorMFT, &video_processor_create },
    { &CLSID_GStreamerByteStreamHandler, &winegstreamer_stream_handler_create },
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
            if (!(factory = heap_alloc(sizeof(*factory))))
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

static const struct
{
    const GUID *subtype;
    GstVideoFormat format;
}
uncompressed_video_formats[] =
{
    {&MFVideoFormat_ARGB32,  GST_VIDEO_FORMAT_BGRA},
    {&MFVideoFormat_RGB32,   GST_VIDEO_FORMAT_BGRx},
    {&MFVideoFormat_RGB24,   GST_VIDEO_FORMAT_BGR},
    {&MFVideoFormat_RGB565,  GST_VIDEO_FORMAT_BGR16},
    {&MFVideoFormat_RGB555,  GST_VIDEO_FORMAT_BGR15},
};

/* returns NULL if doesn't match exactly */
IMFMediaType *mf_media_type_from_caps(const GstCaps *caps)
{
    IMFMediaType *media_type;
    GstStructure *info;
    const char *mime_type;

    if (TRACE_ON(mfplat))
    {
        gchar *human_readable = gst_caps_to_string(caps);
        TRACE("caps = %s\n", debugstr_a(human_readable));
        g_free(human_readable);
    }

    if (FAILED(MFCreateMediaType(&media_type)))
        return NULL;

    info = gst_caps_get_structure(caps, 0);
    mime_type = gst_structure_get_name(info);

    if (!strncmp(mime_type, "video", 5))
    {
        GstVideoInfo video_info;

        if (!gst_video_info_from_caps(&video_info, caps))
        {
            return NULL;
        }

        IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);

        IMFMediaType_SetUINT64(media_type, &MF_MT_FRAME_SIZE, ((UINT64)video_info.width << 32) | video_info.height);

        IMFMediaType_SetUINT64(media_type, &MF_MT_FRAME_RATE, ((UINT64)video_info.fps_n << 32) | video_info.fps_d);

        if (!strcmp(mime_type, "video/x-raw"))
        {
            GUID fourcc_subtype = MFVideoFormat_Base;
            unsigned int i;

            IMFMediaType_SetUINT32(media_type, &MF_MT_COMPRESSED, FALSE);

            /* First try FOURCC */
            if ((fourcc_subtype.Data1 = gst_video_format_to_fourcc(video_info.finfo->format)))
            {
                IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &fourcc_subtype);
            }
            else
            {
                for (i = 0; i < ARRAY_SIZE(uncompressed_video_formats); i++)
                {
                    if (uncompressed_video_formats[i].format == video_info.finfo->format)
                    {
                        IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, uncompressed_video_formats[i].subtype);
                        break;
                    }
                }
                if (i == ARRAY_SIZE(uncompressed_video_formats))
                {
                    FIXME("Unrecognized uncompressed video format %s\n", gst_video_format_to_string(video_info.finfo->format));
                    IMFMediaType_Release(media_type);
                    return NULL;
                }
            }
        }
        else
        {
            FIXME("Unrecognized video format %s\n", mime_type);
            return NULL;
        }
    }
    else if (!strncmp(mime_type, "audio", 5))
    {
        gint rate, channels, bitrate;
        guint64 channel_mask;
        IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);

        if (gst_structure_get_int(info, "rate", &rate))
            IMFMediaType_SetUINT32(media_type, &MF_MT_AUDIO_SAMPLES_PER_SECOND, rate);

        if (gst_structure_get_int(info, "channels", &channels))
            IMFMediaType_SetUINT32(media_type, &MF_MT_AUDIO_NUM_CHANNELS, channels);

        if (gst_structure_get(info, "channel-mask", GST_TYPE_BITMASK, &channel_mask, NULL))
            IMFMediaType_SetUINT32(media_type, &MF_MT_AUDIO_CHANNEL_MASK, channel_mask);

        if (gst_structure_get_int(info, "bitrate", &bitrate))
            IMFMediaType_SetUINT32(media_type, &MF_MT_AVG_BITRATE, bitrate);

        if (!strcmp(mime_type, "audio/x-raw"))
        {
            GstAudioInfo audio_info;
            DWORD depth;

            if (!gst_audio_info_from_caps(&audio_info, caps))
            {
                ERR("Failed to get caps audio info\n");
                IMFMediaType_Release(media_type);
                return NULL;
            }

            depth = GST_AUDIO_INFO_DEPTH(&audio_info);

            /* validation */
            if ((audio_info.finfo->flags & GST_AUDIO_FORMAT_FLAG_INTEGER && depth > 8) ||
                (audio_info.finfo->flags & GST_AUDIO_FORMAT_FLAG_SIGNED && depth <= 8) ||
                (audio_info.finfo->endianness != G_LITTLE_ENDIAN && depth > 8))
            {
                IMFMediaType_Release(media_type);
                return NULL;
            }

            /* conversion */
            switch (audio_info.finfo->flags)
            {
                case GST_AUDIO_FORMAT_FLAG_FLOAT:
                    IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFAudioFormat_Float);
                    break;
                case GST_AUDIO_FORMAT_FLAG_INTEGER:
                case GST_AUDIO_FORMAT_FLAG_SIGNED:
                    IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFAudioFormat_PCM);
                    break;
                default:
                    FIXME("Unrecognized audio format %x\n", audio_info.finfo->format);
                    IMFMediaType_Release(media_type);
                    return NULL;
            }

            IMFMediaType_SetUINT32(media_type, &MF_MT_AUDIO_BITS_PER_SAMPLE, depth);
        }
        else
        {
            FIXME("Unrecognized audio format %s\n", mime_type);
            IMFMediaType_Release(media_type);
            return NULL;
        }
    }
    else
    {
        IMFMediaType_Release(media_type);
        return NULL;
    }

    return media_type;
}

GstCaps *caps_from_mf_media_type(IMFMediaType *type)
{
    GUID major_type;
    GUID subtype;
    GstCaps *output = NULL;

    if (FAILED(IMFMediaType_GetMajorType(type, &major_type)))
        return NULL;
    if (FAILED(IMFMediaType_GetGUID(type, &MF_MT_SUBTYPE, &subtype)))
        return NULL;

    if (IsEqualGUID(&major_type, &MFMediaType_Video))
    {
        UINT64 frame_rate = 0, frame_size = 0;
        DWORD width, height;
        GstVideoFormat format = GST_VIDEO_FORMAT_UNKNOWN;
        GUID subtype_base;
        GstVideoInfo info;
        unsigned int i;

        if (FAILED(IMFMediaType_GetUINT64(type, &MF_MT_FRAME_SIZE, &frame_size)))
            return NULL;
        width = frame_size >> 32;
        height = frame_size;

        output = gst_caps_new_empty_simple("video/x-raw");

        for (i = 0; i < ARRAY_SIZE(uncompressed_video_formats); i++)
        {
            if (IsEqualGUID(uncompressed_video_formats[i].subtype, &subtype))
            {
                format = uncompressed_video_formats[i].format;
                break;
            }
        }

        subtype_base = subtype;
        subtype_base.Data1 = 0;
        if (format == GST_VIDEO_FORMAT_UNKNOWN && IsEqualGUID(&MFVideoFormat_Base, &subtype_base))
            format = gst_video_format_from_fourcc(subtype.Data1);

        if (format == GST_VIDEO_FORMAT_UNKNOWN)
        {
            FIXME("Unrecognized format %s\n", debugstr_guid(&subtype));
            return NULL;
        }

        gst_video_info_set_format(&info, format, width, height);
        output = gst_video_info_to_caps(&info);

        if (frame_size)
        {
            gst_caps_set_simple(output, "width", G_TYPE_INT, width, NULL);
            gst_caps_set_simple(output, "height", G_TYPE_INT, height, NULL);
        }
        if (SUCCEEDED(IMFMediaType_GetUINT64(type, &MF_MT_FRAME_RATE, &frame_rate)))
        {
            /* Darksiders: Warmastered Edition uses a MF_MT_FRAME_RATE of 0,
               and gstreamer won't accept an undefined number as the framerate. */
            if (!(DWORD32)frame_rate)
                frame_rate = 1;
            gst_caps_set_simple(output, "framerate", GST_TYPE_FRACTION, (DWORD32)(frame_rate >> 32), (DWORD32) frame_rate, NULL);
        }
        return output;
    }
    else if (IsEqualGUID(&major_type, &MFMediaType_Audio))
    {
        DWORD rate, channels, channel_mask, bitrate;

        if (IsEqualGUID(&subtype, &MFAudioFormat_Float))
        {
            output = gst_caps_new_empty_simple("audio/x-raw");

            gst_caps_set_simple(output, "format", G_TYPE_STRING, "F32LE", NULL);
            gst_caps_set_simple(output, "layout", G_TYPE_STRING, "interleaved", NULL);
        }
        else if (IsEqualGUID(&subtype, &MFAudioFormat_PCM))
        {
            DWORD bits_per_sample;

            if (SUCCEEDED(IMFMediaType_GetUINT32(type, &MF_MT_AUDIO_BITS_PER_SAMPLE, &bits_per_sample)))
            {
                char format[6];
                char type;

                type = bits_per_sample > 8 ? 'S' : 'U';

                output = gst_caps_new_empty_simple("audio/x-raw");

                sprintf(format, "%c%u%s", type, bits_per_sample, bits_per_sample > 8 ? "LE" : "");

                gst_caps_set_simple(output, "format", G_TYPE_STRING, format, NULL);
            }
            else
            {
                ERR("Bits per sample not set.\n");
                return NULL;
            }
        }
        else
        {
            FIXME("Unrecognized subtype %s\n", debugstr_guid(&subtype));
            return NULL;
        }

        if (SUCCEEDED(IMFMediaType_GetUINT32(type, &MF_MT_AUDIO_SAMPLES_PER_SECOND, &rate)))
        {
            gst_caps_set_simple(output, "rate", G_TYPE_INT, rate, NULL);
        }
        if (SUCCEEDED(IMFMediaType_GetUINT32(type, &MF_MT_AUDIO_NUM_CHANNELS, &channels)))
        {
            gst_caps_set_simple(output, "channels", G_TYPE_INT, channels, NULL);
        }
        if (SUCCEEDED(IMFMediaType_GetUINT32(type, &MF_MT_AUDIO_CHANNEL_MASK, &channel_mask)))
        {
            gst_caps_set_simple(output, "channel-mask", GST_TYPE_BITMASK, (guint64) channel_mask, NULL);
        }

        if (SUCCEEDED(IMFMediaType_GetUINT32(type, &MF_MT_AVG_BITRATE, &bitrate)))
        {
            gst_caps_set_simple(output, "bitrate", G_TYPE_INT, bitrate, NULL);
        }

        return output;
    }

    FIXME("Unrecognized major type %s\n", debugstr_guid(&major_type));
    return NULL;
}
