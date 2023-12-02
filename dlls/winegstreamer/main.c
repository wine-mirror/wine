/* GStreamer Base Functions
 *
 * Copyright 2002 Lionel Ulmer
 * Copyright 2010 Aric Stewart, CodeWeavers
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

#define WINE_NO_NAMELESS_EXTENSION

#define EXTERN_GUID DEFINE_GUID

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "initguid.h"
#include "gst_private.h"
#include "winternl.h"
#include "rpcproxy.h"
#include "dmoreg.h"
#include "gst_guids.h"
#include "wmcodecdsp.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

DEFINE_GUID(GUID_NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
DEFINE_GUID(MEDIASUBTYPE_VC1S,MAKEFOURCC('V','C','1','S'),0x0000,0x0010,0x80,0x00,0x00,0xaa,0x00,0x38,0x9b,0x71);

static const GUID MEDIASUBTYPE_MP3 = {0x00000055, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};

bool array_reserve(void **elements, size_t *capacity, size_t count, size_t size)
{
    unsigned int new_capacity, max_capacity;
    void *new_elements;

    if (count <= *capacity)
        return TRUE;

    max_capacity = ~(SIZE_T)0 / size;
    if (count > max_capacity)
        return FALSE;

    new_capacity = max(4, *capacity);
    while (new_capacity < count && new_capacity <= max_capacity / 2)
        new_capacity *= 2;
    if (new_capacity < count)
        new_capacity = max_capacity;

    if (!(new_elements = realloc(*elements, new_capacity * size)))
        return FALSE;

    *elements = new_elements;
    *capacity = new_capacity;

    return TRUE;
}

wg_parser_t wg_parser_create(enum wg_parser_type type, bool output_compressed)
{
    struct wg_parser_create_params params =
    {
        .type = type,
        .output_compressed = output_compressed,
        .err_on = ERR_ON(quartz),
        .warn_on = WARN_ON(quartz),
    };

    TRACE("type %#x.\n", type);

    if (WINE_UNIX_CALL(unix_wg_parser_create, &params))
        return 0;

    TRACE("Returning parser %#I64x.\n", params.parser);

    return params.parser;
}

void wg_parser_destroy(wg_parser_t parser)
{
    TRACE("parser %#I64x.\n", parser);

    WINE_UNIX_CALL(unix_wg_parser_destroy, &parser);
}

HRESULT wg_parser_connect(wg_parser_t parser, uint64_t file_size)
{
    struct wg_parser_connect_params params =
    {
        .parser = parser,
        .file_size = file_size,
    };

    TRACE("parser %#I64x, file_size %I64u.\n", parser, file_size);

    return WINE_UNIX_CALL(unix_wg_parser_connect, &params);
}

void wg_parser_disconnect(wg_parser_t parser)
{
    TRACE("parser %#I64x.\n", parser);

    WINE_UNIX_CALL(unix_wg_parser_disconnect, &parser);
}

bool wg_parser_get_next_read_offset(wg_parser_t parser, uint64_t *offset, uint32_t *size)
{
    struct wg_parser_get_next_read_offset_params params =
    {
        .parser = parser,
    };

    TRACE("parser %#I64x, offset %p, size %p.\n", parser, offset, size);

    if (WINE_UNIX_CALL(unix_wg_parser_get_next_read_offset, &params))
        return false;
    *offset = params.offset;
    *size = params.size;
    return true;
}

void wg_parser_push_data(wg_parser_t parser, const void *data, uint32_t size)
{
    struct wg_parser_push_data_params params =
    {
        .parser = parser,
        .data = data,
        .size = size,
    };

    TRACE("parser %#I64x, data %p, size %u.\n", parser, data, size);

    WINE_UNIX_CALL(unix_wg_parser_push_data, &params);
}

uint32_t wg_parser_get_stream_count(wg_parser_t parser)
{
    struct wg_parser_get_stream_count_params params =
    {
        .parser = parser,
    };

    TRACE("parser %#I64x.\n", parser);

    WINE_UNIX_CALL(unix_wg_parser_get_stream_count, &params);
    return params.count;
}

wg_parser_stream_t wg_parser_get_stream(wg_parser_t parser, uint32_t index)
{
    struct wg_parser_get_stream_params params =
    {
        .parser = parser,
        .index = index,
    };

    TRACE("parser %#I64x, index %u.\n", parser, index);

    WINE_UNIX_CALL(unix_wg_parser_get_stream, &params);

    TRACE("Returning stream %#I64x.\n", params.stream);
    return params.stream;
}

void wg_parser_stream_get_preferred_format(wg_parser_stream_t stream, struct wg_format *format)
{
    struct wg_parser_stream_get_preferred_format_params params =
    {
        .stream = stream,
        .format = format,
    };

    TRACE("stream %#I64x, format %p.\n", stream, format);

    WINE_UNIX_CALL(unix_wg_parser_stream_get_preferred_format, &params);
}

void wg_parser_stream_get_codec_format(wg_parser_stream_t stream, struct wg_format *format)
{
    struct wg_parser_stream_get_codec_format_params params =
    {
        .stream = stream,
        .format = format,
    };

    TRACE("stream %#I64x, format %p.\n", stream, format);

    WINE_UNIX_CALL(unix_wg_parser_stream_get_codec_format, &params);
}

void wg_parser_stream_enable(wg_parser_stream_t stream, const struct wg_format *format)
{
    struct wg_parser_stream_enable_params params =
    {
        .stream = stream,
        .format = format,
    };

    TRACE("stream %#I64x, format %p.\n", stream, format);

    WINE_UNIX_CALL(unix_wg_parser_stream_enable, &params);
}

void wg_parser_stream_disable(wg_parser_stream_t stream)
{
    TRACE("stream %#I64x.\n", stream);

    WINE_UNIX_CALL(unix_wg_parser_stream_disable, &stream);
}

bool wg_parser_stream_get_buffer(wg_parser_t parser, wg_parser_stream_t stream,
        struct wg_parser_buffer *buffer)
{
    struct wg_parser_stream_get_buffer_params params =
    {
        .parser = parser,
        .stream = stream,
        .buffer = buffer,
    };

    TRACE("parser %#I64x, stream %#I64x, buffer %p.\n", parser, stream, buffer);

    return !WINE_UNIX_CALL(unix_wg_parser_stream_get_buffer, &params);
}

bool wg_parser_stream_copy_buffer(wg_parser_stream_t stream,
        void *data, uint32_t offset, uint32_t size)
{
    struct wg_parser_stream_copy_buffer_params params =
    {
        .stream = stream,
        .data = data,
        .offset = offset,
        .size = size,
    };

    TRACE("stream %#I64x, data %p, offset %u, size %u.\n", stream, data, offset, size);

    return !WINE_UNIX_CALL(unix_wg_parser_stream_copy_buffer, &params);
}

void wg_parser_stream_release_buffer(wg_parser_stream_t stream)
{
    TRACE("stream %#I64x.\n", stream);

    WINE_UNIX_CALL(unix_wg_parser_stream_release_buffer, &stream);
}

void wg_parser_stream_notify_qos(wg_parser_stream_t stream,
        bool underflow, double proportion, int64_t diff, uint64_t timestamp)
{
    struct wg_parser_stream_notify_qos_params params =
    {
        .stream = stream,
        .underflow = underflow,
        .proportion = proportion,
        .diff = diff,
        .timestamp = timestamp,
    };

    TRACE("stream %#I64x, underflow %d, proportion %.16e, diff %I64d, timestamp %I64u.\n",
            stream, underflow, proportion, diff, timestamp);

    WINE_UNIX_CALL(unix_wg_parser_stream_notify_qos, &params);
}

uint64_t wg_parser_stream_get_duration(wg_parser_stream_t stream)
{
    struct wg_parser_stream_get_duration_params params =
    {
        .stream = stream,
    };

    TRACE("stream %#I64x.\n", stream);

    WINE_UNIX_CALL(unix_wg_parser_stream_get_duration, &params);

    TRACE("Returning duration %I64u.\n", params.duration);
    return params.duration;
}

char *wg_parser_stream_get_tag(wg_parser_stream_t stream, enum wg_parser_tag tag)
{
    uint32_t size = 0;
    struct wg_parser_stream_get_tag_params params =
    {
        .stream = stream,
        .tag = tag,
        .size = &size,
    };
    char *buffer;

    if (WINE_UNIX_CALL(unix_wg_parser_stream_get_tag, &params) != STATUS_BUFFER_TOO_SMALL)
        return NULL;
    if (!(buffer = malloc(size)))
    {
        ERR("No memory.\n");
        return NULL;
    }
    params.buffer = buffer;
    if (WINE_UNIX_CALL(unix_wg_parser_stream_get_tag, &params))
    {
        ERR("wg_parser_stream_get_tag failed unexpectedly.\n");
        free(buffer);
        return NULL;
    }
    return buffer;
}

void wg_parser_stream_seek(wg_parser_stream_t stream, double rate,
        uint64_t start_pos, uint64_t stop_pos, DWORD start_flags, DWORD stop_flags)
{
    struct wg_parser_stream_seek_params params =
    {
        .stream = stream,
        .rate = rate,
        .start_pos = start_pos,
        .stop_pos = stop_pos,
        .start_flags = start_flags,
        .stop_flags = stop_flags,
    };

    TRACE("stream %#I64x, rate %.16e, start_pos %I64u, stop_pos %I64u, start_flags %#lx, stop_flags %#lx.\n",
            stream, rate, start_pos, stop_pos, start_flags, stop_flags);

    WINE_UNIX_CALL(unix_wg_parser_stream_seek, &params);
}

wg_transform_t wg_transform_create(const struct wg_format *input_format,
        const struct wg_format *output_format, const struct wg_transform_attrs *attrs)
{
    struct wg_transform_create_params params =
    {
        .input_format = input_format,
        .output_format = output_format,
        .attrs = attrs,
    };

    TRACE("input_format %p, output_format %p.\n", input_format, output_format);

    if (WINE_UNIX_CALL(unix_wg_transform_create, &params))
        return 0;

    TRACE("Returning transform %#I64x.\n", params.transform);
    return params.transform;
}

void wg_transform_destroy(wg_transform_t transform)
{
    TRACE("transform %#I64x.\n", transform);

    WINE_UNIX_CALL(unix_wg_transform_destroy, &transform);
}

HRESULT wg_transform_push_data(wg_transform_t transform, struct wg_sample *sample)
{
    struct wg_transform_push_data_params params =
    {
        .transform = transform,
        .sample = sample,
    };
    NTSTATUS status;

    TRACE("transform %#I64x, sample %p.\n", transform, sample);

    if ((status = WINE_UNIX_CALL(unix_wg_transform_push_data, &params)))
        return HRESULT_FROM_NT(status);

    return params.result;
}

HRESULT wg_transform_read_data(wg_transform_t transform, struct wg_sample *sample,
        struct wg_format *format)
{
    struct wg_transform_read_data_params params =
    {
        .transform = transform,
        .sample = sample,
        .format = format,
    };
    NTSTATUS status;

    TRACE("transform %#I64x, sample %p, format %p.\n", transform, sample, format);

    if ((status = WINE_UNIX_CALL(unix_wg_transform_read_data, &params)))
        return HRESULT_FROM_NT(status);

    return params.result;
}

bool wg_transform_get_status(wg_transform_t transform, bool *accepts_input)
{
    struct wg_transform_get_status_params params =
    {
        .transform = transform,
    };

    TRACE("transform %#I64x, accepts_input %p.\n", transform, accepts_input);

    if (WINE_UNIX_CALL(unix_wg_transform_get_status, &params))
        return false;

    *accepts_input = params.accepts_input;
    return true;
}

bool wg_transform_set_output_format(wg_transform_t transform, struct wg_format *format)
{
    struct wg_transform_set_output_format_params params =
    {
        .transform = transform,
        .format = format,
    };

    TRACE("transform %#I64x, format %p.\n", transform, format);

    return !WINE_UNIX_CALL(unix_wg_transform_set_output_format, &params);
}

HRESULT wg_transform_drain(wg_transform_t transform)
{
    NTSTATUS status;

    TRACE("transform %#I64x.\n", transform);

    if ((status = WINE_UNIX_CALL(unix_wg_transform_drain, &transform)))
    {
        WARN("wg_transform_drain returned status %#lx\n", status);
        return HRESULT_FROM_NT(status);
    }

    return S_OK;
}

HRESULT wg_transform_flush(wg_transform_t transform)
{
    NTSTATUS status;

    TRACE("transform %#I64x.\n", transform);

    if ((status = WINE_UNIX_CALL(unix_wg_transform_flush, &transform)))
    {
        WARN("wg_transform_flush returned status %#lx\n", status);
        return HRESULT_FROM_NT(status);
    }

    return S_OK;
}

void wg_transform_notify_qos(wg_transform_t transform,
        bool underflow, double proportion, int64_t diff, uint64_t timestamp)
{
    struct wg_transform_notify_qos_params params =
    {
        .transform = transform,
        .underflow = underflow,
        .proportion = proportion,
        .diff = diff,
        .timestamp = timestamp,
    };

    TRACE("transform %#I64x, underflow %d, proportion %.16e, diff %I64d, timestamp %I64u.\n",
            transform, underflow, proportion, diff, timestamp);

    WINE_UNIX_CALL(unix_wg_transform_notify_qos, &params);
}

HRESULT wg_muxer_create(const char *format, wg_muxer_t *muxer)
{
    struct wg_muxer_create_params params =
    {
        .format = format,
    };
    NTSTATUS status;

    TRACE("format %p, muxer %p.\n", format, muxer);

    if (!(status = WINE_UNIX_CALL(unix_wg_muxer_create, &params)))
    {
        *muxer = params.muxer;
        TRACE("Created wg_muxer %#I64x.\n", params.muxer);
    }
    else
    {
        WARN("Failed to create muxer, status %#lx.\n", status);
    }

    return HRESULT_FROM_NT(status);
}

void wg_muxer_destroy(wg_muxer_t muxer)
{
    TRACE("muxer %#I64x.\n", muxer);

    WINE_UNIX_CALL(unix_wg_muxer_destroy, &muxer);
}

HRESULT wg_muxer_add_stream(wg_muxer_t muxer, UINT32 stream_id, const struct wg_format *format)
{
    struct wg_muxer_add_stream_params params =
    {
        .muxer = muxer,
        .stream_id = stream_id,
        .format = format,
    };
    NTSTATUS status;

    TRACE("muxer %#I64x, stream_id %u, format %p.\n", muxer, stream_id, format);

    if ((status = WINE_UNIX_CALL(unix_wg_muxer_add_stream, &params)))
    {
        WARN("Failed to add stream, status %#lx.\n", status);
        return HRESULT_FROM_NT(status);
    }

    return S_OK;
}

HRESULT wg_muxer_start(wg_muxer_t muxer)
{
    NTSTATUS status;

    TRACE("muxer %#I64x.\n",  muxer);

    if ((status = WINE_UNIX_CALL(unix_wg_muxer_start, &muxer)))
    {
        WARN("Failed to start muxer, status %#lx.\n", status);
        return HRESULT_FROM_NT(status);
    }

    return S_OK;
}

HRESULT wg_muxer_push_sample(wg_muxer_t muxer, struct wg_sample *sample, UINT32 steam_id)
{
    struct wg_muxer_push_sample_params params =
    {
        .muxer = muxer,
        .sample = sample,
        .stream_id = steam_id,
    };
    NTSTATUS status;

    TRACE("muxer %#I64x, sample %p.\n", muxer, sample);

    if ((status = WINE_UNIX_CALL(unix_wg_muxer_push_sample, &params)))
    {
        WARN("Failed to push sample, status %#lx.\n", status);
        return HRESULT_FROM_NT(status);
    }

    return S_OK;
}

HRESULT wg_muxer_read_data(wg_muxer_t muxer, void *buffer, UINT32 *size, UINT64 *offset)
{
    struct wg_muxer_read_data_params params =
    {
        .muxer = muxer,
        .buffer = buffer,
        .size = *size,
        .offset = UINT64_MAX,
    };
    NTSTATUS status;

    TRACE("muxer %#I64x, buffer %p, size %u.\n", muxer, buffer, *size);

    if (SUCCEEDED(status = WINE_UNIX_CALL(unix_wg_muxer_read_data, &params)))
    {
        *size = params.size;
        *offset = params.offset;
        TRACE("Read %u bytes, offset %#I64x.\n", *size, *offset);
    }

    return HRESULT_FROM_NT(status);
}

HRESULT wg_muxer_finalize(wg_muxer_t muxer)
{
    NTSTATUS status;

    TRACE("muxer %#I64x.\n", muxer);

    if ((status = WINE_UNIX_CALL(unix_wg_muxer_finalize, &muxer)))
    {
        WARN("Failed to finalize, status %#lx.\n", status);
        return HRESULT_FROM_NT(status);
    }

    return S_OK;
}

#define ALIGN(n, alignment) (((n) + (alignment) - 1) & ~((alignment) - 1))

unsigned int wg_format_get_stride(const struct wg_format *format)
{
    const unsigned int width = format->u.video.width;

    switch (format->u.video.format)
    {
        case WG_VIDEO_FORMAT_AYUV:
            return width * 4;

        case WG_VIDEO_FORMAT_BGRA:
        case WG_VIDEO_FORMAT_BGRx:
            return width * 4;

        case WG_VIDEO_FORMAT_BGR:
            return ALIGN(width * 3, 4);

        case WG_VIDEO_FORMAT_UYVY:
        case WG_VIDEO_FORMAT_YUY2:
        case WG_VIDEO_FORMAT_YVYU:
            return ALIGN(width * 2, 4);

        case WG_VIDEO_FORMAT_RGB15:
        case WG_VIDEO_FORMAT_RGB16:
            return ALIGN(width * 2, 4);

        case WG_VIDEO_FORMAT_I420:
        case WG_VIDEO_FORMAT_NV12:
        case WG_VIDEO_FORMAT_YV12:
            return ALIGN(width, 4); /* Y plane */

        case WG_VIDEO_FORMAT_UNKNOWN:
            FIXME("Cannot calculate stride for unknown video format.\n");
    }

    return 0;
}

bool wg_video_format_is_rgb(enum wg_video_format format)
{
    switch (format)
    {
        case WG_VIDEO_FORMAT_BGRA:
        case WG_VIDEO_FORMAT_BGRx:
        case WG_VIDEO_FORMAT_BGR:
        case WG_VIDEO_FORMAT_RGB15:
        case WG_VIDEO_FORMAT_RGB16:
            return true;

        case WG_VIDEO_FORMAT_AYUV:
        case WG_VIDEO_FORMAT_I420:
        case WG_VIDEO_FORMAT_NV12:
        case WG_VIDEO_FORMAT_UYVY:
        case WG_VIDEO_FORMAT_YUY2:
        case WG_VIDEO_FORMAT_YV12:
        case WG_VIDEO_FORMAT_YVYU:
        case WG_VIDEO_FORMAT_UNKNOWN:
            break;
    }

    return false;
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, void *reserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(instance);
        __wine_init_unix_call();
    }
    return TRUE;
}

struct class_factory
{
    IClassFactory IClassFactory_iface;
    HRESULT (*create_instance)(IUnknown *outer, IUnknown **out);
};

static inline struct class_factory *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, struct class_factory, IClassFactory_iface);
}

static HRESULT WINAPI class_factory_QueryInterface(IClassFactory *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) || IsEqualGUID(iid, &IID_IClassFactory))
    {
        *out = iface;
        IClassFactory_AddRef(iface);
        return S_OK;
    }

    *out = NULL;
    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI class_factory_AddRef(IClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI class_factory_Release(IClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI class_factory_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID iid, void **out)
{
    struct class_factory *factory = impl_from_IClassFactory(iface);
    IUnknown *unk;
    HRESULT hr;

    TRACE("iface %p, outer %p, iid %s, out %p.\n", iface, outer, debugstr_guid(iid), out);

    if (outer && !IsEqualGUID(iid, &IID_IUnknown))
        return E_NOINTERFACE;

    *out = NULL;
    if (SUCCEEDED(hr = factory->create_instance(outer, &unk)))
    {
        hr = IUnknown_QueryInterface(unk, iid, out);
        IUnknown_Release(unk);
    }
    return hr;
}

static HRESULT WINAPI class_factory_LockServer(IClassFactory *iface, BOOL lock)
{
    TRACE("iface %p, lock %d.\n", iface, lock);
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

static struct class_factory avi_splitter_cf = {{&class_factory_vtbl}, avi_splitter_create};
static struct class_factory decodebin_parser_cf = {{&class_factory_vtbl}, decodebin_parser_create};
static struct class_factory mpeg_audio_codec_cf = {{&class_factory_vtbl}, mpeg_audio_codec_create};
static struct class_factory mpeg_video_codec_cf = {{&class_factory_vtbl}, mpeg_video_codec_create};
static struct class_factory mpeg_layer3_decoder_cf = {{&class_factory_vtbl}, mpeg_layer3_decoder_create};
static struct class_factory mpeg_splitter_cf = {{&class_factory_vtbl}, mpeg_splitter_create};
static struct class_factory wave_parser_cf = {{&class_factory_vtbl}, wave_parser_create};
static struct class_factory wma_decoder_cf = {{&class_factory_vtbl}, wma_decoder_create};
static struct class_factory wmv_decoder_cf = {{&class_factory_vtbl}, wmv_decoder_create};
static struct class_factory resampler_cf = {{&class_factory_vtbl}, resampler_create};
static struct class_factory color_convert_cf = {{&class_factory_vtbl}, color_convert_create};
static struct class_factory mp3_sink_class_factory_cf = {{&class_factory_vtbl}, mp3_sink_class_factory_create};
static struct class_factory mpeg4_sink_class_factory_cf = {{&class_factory_vtbl}, mpeg4_sink_class_factory_create};

HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID iid, void **out)
{
    struct class_factory *factory;
    HRESULT hr;

    TRACE("clsid %s, iid %s, out %p.\n", debugstr_guid(clsid), debugstr_guid(iid), out);

    if (!init_gstreamer())
        return CLASS_E_CLASSNOTAVAILABLE;

    if (SUCCEEDED(hr = mfplat_get_class_object(clsid, iid, out)))
        return hr;

    if (IsEqualGUID(clsid, &CLSID_AviSplitter))
        factory = &avi_splitter_cf;
    else if (IsEqualGUID(clsid, &CLSID_decodebin_parser))
        factory = &decodebin_parser_cf;
    else if (IsEqualGUID(clsid, &CLSID_CMpegAudioCodec))
        factory = &mpeg_audio_codec_cf;
    else if (IsEqualGUID(clsid, &CLSID_CMpegVideoCodec))
        factory = &mpeg_video_codec_cf;
    else if (IsEqualGUID(clsid, &CLSID_mpeg_layer3_decoder))
        factory = &mpeg_layer3_decoder_cf;
    else if (IsEqualGUID(clsid, &CLSID_MPEG1Splitter))
        factory = &mpeg_splitter_cf;
    else if (IsEqualGUID(clsid, &CLSID_WAVEParser))
        factory = &wave_parser_cf;
    else if (IsEqualGUID(clsid, &CLSID_WMADecMediaObject))
        factory = &wma_decoder_cf;
    else if (IsEqualGUID(clsid, &CLSID_WMVDecoderMFT))
        factory = &wmv_decoder_cf;
    else if (IsEqualGUID(clsid, &CLSID_CResamplerMediaObject))
        factory = &resampler_cf;
    else if (IsEqualGUID(clsid, &CLSID_CColorConvertDMO))
        factory = &color_convert_cf;
    else if (IsEqualGUID(clsid, &CLSID_MFMP3SinkClassFactory))
        factory = &mp3_sink_class_factory_cf;
    else if (IsEqualGUID(clsid, &CLSID_MFMPEG4SinkClassFactory))
        factory = &mpeg4_sink_class_factory_cf;
    else
    {
        FIXME("%s not implemented, returning CLASS_E_CLASSNOTAVAILABLE.\n", debugstr_guid(clsid));
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    return IClassFactory_QueryInterface(&factory->IClassFactory_iface, iid, out);
}

static BOOL CALLBACK init_gstreamer_proc(INIT_ONCE *once, void *param, void **ctx)
{
    HINSTANCE handle;

    if (WINE_UNIX_CALL(unix_wg_init_gstreamer, NULL))
        return FALSE;

    /* Unloading glib is a bad idea.. it installs atexit handlers,
     * so never unload the dll after loading */
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
            (LPCWSTR)init_gstreamer_proc, &handle);
    if (!handle)
        ERR("Failed to pin module.\n");

    return TRUE;
}

BOOL init_gstreamer(void)
{
    static INIT_ONCE once = INIT_ONCE_STATIC_INIT;

    InitOnceExecuteOnce(&once, init_gstreamer_proc, NULL, NULL);

    return TRUE;
}

static const REGPINTYPES reg_audio_mt = {&MEDIATYPE_Audio, &GUID_NULL};
static const REGPINTYPES reg_stream_mt = {&MEDIATYPE_Stream, &GUID_NULL};
static const REGPINTYPES reg_video_mt = {&MEDIATYPE_Video, &GUID_NULL};

static const REGPINTYPES reg_avi_splitter_sink_mt = {&MEDIATYPE_Stream, &MEDIASUBTYPE_Avi};

static const REGFILTERPINS2 reg_avi_splitter_pins[2] =
{
    {
        .nMediaTypes = 1,
        .lpMediaType = &reg_avi_splitter_sink_mt,
    },
    {
        .dwFlags = REG_PINFLAG_B_OUTPUT,
        .nMediaTypes = 1,
        .lpMediaType = &reg_video_mt,
    },
};

static const REGFILTER2 reg_avi_splitter =
{
    .dwVersion = 2,
    .dwMerit = MERIT_NORMAL,
    .u.s2.cPins2 = 2,
    .u.s2.rgPins2 = reg_avi_splitter_pins,
};

static const REGPINTYPES reg_mpeg_audio_codec_sink_mts[3] =
{
    {&MEDIATYPE_Audio, &MEDIASUBTYPE_MPEG1Packet},
    {&MEDIATYPE_Audio, &MEDIASUBTYPE_MPEG1Payload},
    {&MEDIATYPE_Audio, &MEDIASUBTYPE_MPEG1AudioPayload},
};

static const REGPINTYPES reg_mpeg_audio_codec_source_mts[1] =
{
    {&MEDIATYPE_Audio, &MEDIASUBTYPE_PCM},
};

static const REGFILTERPINS2 reg_mpeg_audio_codec_pins[2] =
{
    {
        .nMediaTypes = 3,
        .lpMediaType = reg_mpeg_audio_codec_sink_mts,
    },
    {
        .dwFlags = REG_PINFLAG_B_OUTPUT,
        .nMediaTypes = 1,
        .lpMediaType = reg_mpeg_audio_codec_source_mts,
    },
};

static const REGFILTER2 reg_mpeg_audio_codec =
{
    .dwVersion = 2,
    .dwMerit = 0x03680001,
    .u.s2.cPins2 = 2,
    .u.s2.rgPins2 = reg_mpeg_audio_codec_pins,
};

static const REGPINTYPES reg_mpeg_video_codec_sink_mts[2] =
{
    {&MEDIATYPE_Video, &MEDIASUBTYPE_MPEG1Packet},
    {&MEDIATYPE_Video, &MEDIASUBTYPE_MPEG1Payload},
};

static const REGPINTYPES reg_mpeg_video_codec_source_mts[1] =
{
    {&MEDIATYPE_Video, &GUID_NULL},
};

static const REGFILTERPINS2 reg_mpeg_video_codec_pins[2] =
{
    {
        .nMediaTypes = 2,
        .lpMediaType = reg_mpeg_video_codec_sink_mts,
    },
    {
        .dwFlags = REG_PINFLAG_B_OUTPUT,
        .nMediaTypes = 1,
        .lpMediaType = reg_mpeg_video_codec_source_mts,
    },
};

static const REGFILTER2 reg_mpeg_video_codec =
{
    .dwVersion = 2,
    .dwMerit = 0x40000001,
    .u.s2.cPins2 = 2,
    .u.s2.rgPins2 = reg_mpeg_video_codec_pins,
};

static const REGPINTYPES reg_mpeg_layer3_decoder_sink_mts[1] =
{
    {&MEDIATYPE_Audio, &MEDIASUBTYPE_MP3},
};

static const REGPINTYPES reg_mpeg_layer3_decoder_source_mts[1] =
{
    {&MEDIATYPE_Audio, &MEDIASUBTYPE_PCM},
};

static const REGFILTERPINS2 reg_mpeg_layer3_decoder_pins[2] =
{
    {
        .nMediaTypes = 1,
        .lpMediaType = reg_mpeg_layer3_decoder_sink_mts,
    },
    {
        .dwFlags = REG_PINFLAG_B_OUTPUT,
        .nMediaTypes = 1,
        .lpMediaType = reg_mpeg_layer3_decoder_source_mts,
    },
};

static const REGFILTER2 reg_mpeg_layer3_decoder =
{
    .dwVersion = 2,
    .dwMerit = 0x00810000,
    .u.s2.cPins2 = 2,
    .u.s2.rgPins2 = reg_mpeg_layer3_decoder_pins,
};

static const REGPINTYPES reg_mpeg_splitter_sink_mts[4] =
{
    {&MEDIATYPE_Stream, &MEDIASUBTYPE_MPEG1Audio},
    {&MEDIATYPE_Stream, &MEDIASUBTYPE_MPEG1Video},
    {&MEDIATYPE_Stream, &MEDIASUBTYPE_MPEG1System},
    {&MEDIATYPE_Stream, &MEDIASUBTYPE_MPEG1VideoCD},
};

static const REGPINTYPES reg_mpeg_splitter_audio_mts[2] =
{
    {&MEDIATYPE_Audio, &MEDIASUBTYPE_MPEG1Packet},
    {&MEDIATYPE_Audio, &MEDIASUBTYPE_MPEG1AudioPayload},
};

static const REGPINTYPES reg_mpeg_splitter_video_mts[2] =
{
    {&MEDIATYPE_Video, &MEDIASUBTYPE_MPEG1Packet},
    {&MEDIATYPE_Video, &MEDIASUBTYPE_MPEG1Payload},
};

static const REGFILTERPINS2 reg_mpeg_splitter_pins[3] =
{
    {
        .nMediaTypes = 4,
        .lpMediaType = reg_mpeg_splitter_sink_mts,
    },
    {
        .dwFlags = REG_PINFLAG_B_ZERO | REG_PINFLAG_B_OUTPUT,
        .nMediaTypes = 2,
        .lpMediaType = reg_mpeg_splitter_audio_mts,
    },
    {
        .dwFlags = REG_PINFLAG_B_ZERO | REG_PINFLAG_B_OUTPUT,
        .nMediaTypes = 2,
        .lpMediaType = reg_mpeg_splitter_video_mts,
    },
};

static const REGFILTER2 reg_mpeg_splitter =
{
    .dwVersion = 2,
    .dwMerit = MERIT_NORMAL,
    .u.s2.cPins2 = 3,
    .u.s2.rgPins2 = reg_mpeg_splitter_pins,
};

static const REGPINTYPES reg_wave_parser_sink_mts[3] =
{
    {&MEDIATYPE_Stream, &MEDIASUBTYPE_WAVE},
    {&MEDIATYPE_Stream, &MEDIASUBTYPE_AU},
    {&MEDIATYPE_Stream, &MEDIASUBTYPE_AIFF},
};

static const REGFILTERPINS2 reg_wave_parser_pins[2] =
{
    {
        .nMediaTypes = 3,
        .lpMediaType = reg_wave_parser_sink_mts,
    },
    {
        .dwFlags = REG_PINFLAG_B_OUTPUT,
        .nMediaTypes = 1,
        .lpMediaType = &reg_audio_mt,
    },
};

static const REGFILTER2 reg_wave_parser =
{
    .dwVersion = 2,
    .dwMerit = MERIT_UNLIKELY,
    .u.s2.cPins2 = 2,
    .u.s2.rgPins2 = reg_wave_parser_pins,
};

static const REGFILTERPINS2 reg_decodebin_parser_pins[3] =
{
    {
        .nMediaTypes = 1,
        .lpMediaType = &reg_stream_mt,
    },
    {
        .dwFlags = REG_PINFLAG_B_OUTPUT,
        .nMediaTypes = 1,
        .lpMediaType = &reg_audio_mt,
    },
    {
        .dwFlags = REG_PINFLAG_B_OUTPUT,
        .nMediaTypes = 1,
        .lpMediaType = &reg_video_mt,
    },
};

static const REGFILTER2 reg_decodebin_parser =
{
    .dwVersion = 2,
    .dwMerit = MERIT_PREFERRED,
    .u.s2.cPins2 = 3,
    .u.s2.rgPins2 = reg_decodebin_parser_pins,
};

HRESULT WINAPI DllRegisterServer(void)
{
    DMO_PARTIAL_MEDIATYPE audio_convert_types[2] =
    {
        {.type = MEDIATYPE_Audio, .subtype = MEDIASUBTYPE_PCM},
        {.type = MEDIATYPE_Audio, .subtype = MEDIASUBTYPE_IEEE_FLOAT},
    };
    DMO_PARTIAL_MEDIATYPE wma_decoder_output[2] =
    {
        {.type = MEDIATYPE_Audio, .subtype = MEDIASUBTYPE_PCM},
        {.type = MEDIATYPE_Audio, .subtype = MEDIASUBTYPE_IEEE_FLOAT},
    };
    DMO_PARTIAL_MEDIATYPE wma_decoder_input[4] =
    {
        {.type = MEDIATYPE_Audio, .subtype = MEDIASUBTYPE_MSAUDIO1},
        {.type = MEDIATYPE_Audio, .subtype = MEDIASUBTYPE_WMAUDIO2},
        {.type = MEDIATYPE_Audio, .subtype = MEDIASUBTYPE_WMAUDIO3},
        {.type = MEDIATYPE_Audio, .subtype = MEDIASUBTYPE_WMAUDIO_LOSSLESS},
    };
    DMO_PARTIAL_MEDIATYPE wmv_decoder_output[11] =
    {
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_YV12},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_YUY2},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_UYVY},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_YVYU},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_NV11},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_NV12},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_RGB32},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_RGB24},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_RGB565},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_RGB555},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_RGB8},
    };
    DMO_PARTIAL_MEDIATYPE wmv_decoder_input[8] =
    {
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_WMV1},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_WMV2},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_WMV3},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_WMVA},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_WVC1},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_WMVP},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_WVP2},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_VC1S},
    };
    DMO_PARTIAL_MEDIATYPE color_convert_input[20] =
    {
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_YV12},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_YUY2},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_UYVY},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_AYUV},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_NV12},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_RGB32},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_RGB565},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_I420},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_IYUV},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_YVYU},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_RGB24},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_RGB555},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_RGB8},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_V216},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_V410},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_NV11},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_Y41P},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_Y41T},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_Y42T},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_YVU9},
    };
    DMO_PARTIAL_MEDIATYPE color_convert_output[16] =
    {
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_YV12},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_YUY2},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_UYVY},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_AYUV},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_NV12},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_RGB32},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_RGB565},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_I420},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_IYUV},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_YVYU},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_RGB24},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_RGB555},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_RGB8},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_V216},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_V410},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_NV11},
    };

    IFilterMapper2 *mapper;
    HRESULT hr;

    TRACE(".\n");

    if (FAILED(hr = __wine_register_resources()))
        return hr;

    if (FAILED(hr = CoCreateInstance(&CLSID_FilterMapper2, NULL, CLSCTX_INPROC_SERVER,
            &IID_IFilterMapper2, (void **)&mapper)))
        return hr;

    IFilterMapper2_RegisterFilter(mapper, &CLSID_AviSplitter, L"AVI Splitter", NULL, NULL, NULL, &reg_avi_splitter);
    IFilterMapper2_RegisterFilter(mapper, &CLSID_decodebin_parser,
            L"GStreamer splitter filter", NULL, NULL, NULL, &reg_decodebin_parser);
    IFilterMapper2_RegisterFilter(mapper, &CLSID_CMpegAudioCodec,
            L"MPEG Audio Decoder", NULL, NULL, NULL, &reg_mpeg_audio_codec);
    IFilterMapper2_RegisterFilter(mapper, &CLSID_CMpegVideoCodec,
            L"MPEG Video Decoder", NULL, NULL, NULL, &reg_mpeg_video_codec);
    IFilterMapper2_RegisterFilter(mapper, &CLSID_mpeg_layer3_decoder,
            L"MPEG Layer-3 Decoder", NULL, NULL, NULL, &reg_mpeg_layer3_decoder);
    IFilterMapper2_RegisterFilter(mapper, &CLSID_MPEG1Splitter,
            L"MPEG-I Stream Splitter", NULL, NULL, NULL, &reg_mpeg_splitter);
    IFilterMapper2_RegisterFilter(mapper, &CLSID_WAVEParser, L"Wave Parser", NULL, NULL, NULL, &reg_wave_parser);

    IFilterMapper2_Release(mapper);

    if (FAILED(hr = DMORegister(L"WMAudio Decoder DMO", &CLSID_WMADecMediaObject, &DMOCATEGORY_AUDIO_DECODER,
            0, ARRAY_SIZE(wma_decoder_input), wma_decoder_input, ARRAY_SIZE(wma_decoder_output), wma_decoder_output)))
        return hr;
    if (FAILED(hr = DMORegister(L"WMVideo Decoder DMO", &CLSID_WMVDecoderMFT, &DMOCATEGORY_VIDEO_DECODER,
            0, ARRAY_SIZE(wmv_decoder_input), wmv_decoder_input, ARRAY_SIZE(wmv_decoder_output), wmv_decoder_output)))
        return hr;
    if (FAILED(hr = DMORegister(L"Resampler DMO", &CLSID_CResamplerMediaObject, &DMOCATEGORY_AUDIO_EFFECT,
            0, ARRAY_SIZE(audio_convert_types), audio_convert_types, ARRAY_SIZE(audio_convert_types), audio_convert_types)))
        return hr;
    if (FAILED(hr = DMORegister(L"Color Converter DMO", &CLSID_CColorConvertDMO, &DMOCATEGORY_VIDEO_EFFECT,
            0, ARRAY_SIZE(color_convert_input), color_convert_input, ARRAY_SIZE(color_convert_output), color_convert_output)))
        return hr;

    return mfplat_DllRegisterServer();
}

HRESULT WINAPI DllUnregisterServer(void)
{
    IFilterMapper2 *mapper;
    HRESULT hr;

    TRACE(".\n");

    if (FAILED(hr = __wine_unregister_resources()))
        return hr;

    if (FAILED(hr = CoCreateInstance(&CLSID_FilterMapper2, NULL, CLSCTX_INPROC_SERVER,
            &IID_IFilterMapper2, (void **)&mapper)))
        return hr;

    IFilterMapper2_UnregisterFilter(mapper, NULL, NULL, &CLSID_AviSplitter);
    IFilterMapper2_UnregisterFilter(mapper, NULL, NULL, &CLSID_decodebin_parser);
    IFilterMapper2_UnregisterFilter(mapper, NULL, NULL, &CLSID_CMpegAudioCodec);
    IFilterMapper2_UnregisterFilter(mapper, NULL, NULL, &CLSID_CMpegVideoCodec);
    IFilterMapper2_UnregisterFilter(mapper, NULL, NULL, &CLSID_mpeg_layer3_decoder);
    IFilterMapper2_UnregisterFilter(mapper, NULL, NULL, &CLSID_MPEG1Splitter);
    IFilterMapper2_UnregisterFilter(mapper, NULL, NULL, &CLSID_WAVEParser);

    IFilterMapper2_Release(mapper);

    if (FAILED(hr = DMOUnregister(&CLSID_CColorConvertDMO, &DMOCATEGORY_VIDEO_EFFECT)))
        return hr;
    if (FAILED(hr = DMOUnregister(&CLSID_CResamplerMediaObject, &DMOCATEGORY_AUDIO_EFFECT)))
        return hr;
    if (FAILED(hr = DMOUnregister(&CLSID_WMADecMediaObject, &DMOCATEGORY_AUDIO_DECODER)))
        return hr;
    if (FAILED(hr = DMOUnregister(&CLSID_WMVDecoderMFT, &DMOCATEGORY_VIDEO_DECODER)))
        return hr;

    return S_OK;
}
