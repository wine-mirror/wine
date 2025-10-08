/*
 * Copyright 2019 Jactry Zeng for CodeWeavers
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

#define COBJMACROS

#include <math.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"

#include "mfapi.h"
#include "mfmediaengine.h"
#include "mferror.h"
#include "dxgi.h"
#include "initguid.h"
#include "d3d11.h"
#include "wincodec.h"
#include "mmdeviceapi.h"
#include "audiosessiontypes.h"

#include "mediaengine_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

static BOOL mf_array_reserve(void **elements, size_t *capacity, size_t count, size_t size)
{
    size_t new_capacity, max_capacity;
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

/* Convert 100ns to seconds */
static double mftime_to_seconds(MFTIME time)
{
    return (double)time / 10000000.0;
}

enum media_engine_mode
{
    MEDIA_ENGINE_INVALID,
    MEDIA_ENGINE_AUDIO_MODE,
    MEDIA_ENGINE_RENDERING_MODE,
    MEDIA_ENGINE_FRAME_SERVER_MODE,
};

/* Used with create flags. */
enum media_engine_flags
{
    /* MF_MEDIA_ENGINE_CREATEFLAGS_MASK is 0x1f. */
    FLAGS_ENGINE_SHUT_DOWN = 0x20,
    FLAGS_ENGINE_AUTO_PLAY = 0x40,
    FLAGS_ENGINE_LOOP = 0x80,
    FLAGS_ENGINE_PAUSED = 0x100,
    FLAGS_ENGINE_WAITING = 0x200,
    FLAGS_ENGINE_MUTED = 0x400,
    FLAGS_ENGINE_HAS_AUDIO = 0x800,
    FLAGS_ENGINE_HAS_VIDEO = 0x1000,
    FLAGS_ENGINE_FIRST_FRAME = 0x2000,
    FLAGS_ENGINE_IS_ENDED = 0x4000,
    FLAGS_ENGINE_NEW_FRAME = 0x8000,
    FLAGS_ENGINE_SOURCE_PENDING = 0x10000,
    FLAGS_ENGINE_PLAY_PENDING = 0x20000,
    FLAGS_ENGINE_SEEKING = 0x40000,
};

struct vec3
{
    float x, y, z;
};

struct color
{
    float r, g, b, a;
};

static const struct vec3 fullquad[] =
{
    {-1.0f, -1.0f, 0.0f},
    {-1.0f,  1.0f, 0.0f},
    { 1.0f, -1.0f, 0.0f},
    { 1.0f,  1.0f, 0.0f},
};

struct rect
{
    float left, top, right, bottom;
};

struct effect
{
    IUnknown *object;
    BOOL optional;
};

struct effects
{
    struct effect *effects;
    size_t count;
    size_t capacity;
};

struct media_engine
{
    IMFMediaEngineEx IMFMediaEngineEx_iface;
    IMFGetService IMFGetService_iface;
    IMFAsyncCallback session_events;
    IMFAsyncCallback sink_events;
    IMFAsyncCallback load_handler;
    LONG refcount;
    IMFMediaEngineNotify *callback;
    IMFAttributes *attributes;
    IMFDXGIDeviceManager *device_manager;
    HANDLE device_handle;
    enum media_engine_mode mode;
    unsigned int flags;
    double playback_rate;
    double default_playback_rate;
    double volume;
    double duration;
    double next_seek;
    MF_MEDIA_ENGINE_NETWORK network_state;
    MF_MEDIA_ENGINE_ERR error_code;
    HRESULT extended_code;
    MF_MEDIA_ENGINE_READY ready_state;
    MF_MEDIA_ENGINE_PRELOAD preload;
    IMFMediaSession *session;
    IMFPresentationClock *clock;
    IMFSourceResolver *resolver;
    IMFMediaEngineExtension *extension;
    BSTR current_source;
    struct
    {
        IMFMediaSource *source;
        IMFPresentationDescriptor *pd;
        PROPVARIANT start_position;
        struct video_frame_sink *frame_sink;
    } presentation;
    struct effects video_effects;
    struct effects audio_effects;
    struct
    {
        LONGLONG pts;
        SIZE size;
        SIZE ratio;
        TOPOID node_id;
        BYTE *buffer;
        UINT buffer_size;
        DXGI_FORMAT output_format;

        struct
        {
            ID3D11Buffer *vb;
            ID3D11Buffer *ps_cb;
            ID3D11Texture2D *source;
            ID3D11ShaderResourceView *srv;
            ID3D11SamplerState *sampler;
            ID3D11InputLayout *input_layout;
            ID3D11VertexShader *vs;
            ID3D11PixelShader *ps;
            struct vec3 quad[4];
            struct
            {
                struct rect dst;
                struct rect src;
                struct color backcolor;
            } cb;
        } d3d11;
    } video_frame;
    CRITICAL_SECTION cs;
};

static void media_engine_release_video_frame_resources(struct media_engine *engine)
{
    if (engine->video_frame.d3d11.vb)
        ID3D11Buffer_Release(engine->video_frame.d3d11.vb);
    if (engine->video_frame.d3d11.ps_cb)
        ID3D11Buffer_Release(engine->video_frame.d3d11.ps_cb);
    if (engine->video_frame.d3d11.source)
        ID3D11Texture2D_Release(engine->video_frame.d3d11.source);
    if (engine->video_frame.d3d11.srv)
        ID3D11ShaderResourceView_Release(engine->video_frame.d3d11.srv);
    if (engine->video_frame.d3d11.sampler)
        ID3D11SamplerState_Release(engine->video_frame.d3d11.sampler);
    if (engine->video_frame.d3d11.input_layout)
        ID3D11InputLayout_Release(engine->video_frame.d3d11.input_layout);
    if (engine->video_frame.d3d11.vs)
        ID3D11VertexShader_Release(engine->video_frame.d3d11.vs);
    if (engine->video_frame.d3d11.ps)
        ID3D11PixelShader_Release(engine->video_frame.d3d11.ps);

    memset(&engine->video_frame.d3d11, 0, sizeof(engine->video_frame.d3d11));
    memcpy(engine->video_frame.d3d11.quad, fullquad, sizeof(fullquad));
}

static HRESULT media_engine_lock_d3d_device(struct media_engine *engine, ID3D11Device **device)
{
    HRESULT hr;

    if (!engine->device_manager)
    {
        FIXME("Device manager wasn't set.\n");
        return E_UNEXPECTED;
    }

    if (!engine->device_handle)
    {
        if (FAILED(hr = IMFDXGIDeviceManager_OpenDeviceHandle(engine->device_manager, &engine->device_handle)))
        {
            WARN("Failed to open device handle, hr %#lx.\n", hr);
            return hr;
        }
    }

    hr = IMFDXGIDeviceManager_LockDevice(engine->device_manager, engine->device_handle, &IID_ID3D11Device,
            (void **)device, TRUE);
    if (hr == MF_E_DXGI_NEW_VIDEO_DEVICE)
    {
        IMFDXGIDeviceManager_CloseDeviceHandle(engine->device_manager, engine->device_handle);
        engine->device_handle = NULL;

        media_engine_release_video_frame_resources(engine);

        if (FAILED(hr = IMFDXGIDeviceManager_OpenDeviceHandle(engine->device_manager, &engine->device_handle)))
        {
            WARN("Failed to open a device handle, hr %#lx.\n", hr);
            return hr;
        }
        hr = IMFDXGIDeviceManager_LockDevice(engine->device_manager, engine->device_handle, &IID_ID3D11Device,
                (void **)device, TRUE);
    }

    return hr;
}

static void media_engine_unlock_d3d_device(struct media_engine *engine, ID3D11Device *device)
{
    ID3D11Device_Release(device);
    IMFDXGIDeviceManager_UnlockDevice(engine->device_manager, engine->device_handle, FALSE);
}

static HRESULT media_engine_create_d3d11_video_frame_resources(struct media_engine *engine, ID3D11Device *device)
{
    static const D3D11_INPUT_ELEMENT_DESC layout_desc[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    static const DWORD vs_code[] =
    {
#if 0
        float4 main(float4 position : POSITION) : SV_POSITION
        {
            return position;
        }
#endif
        0x43425844, 0xa7a2f22d, 0x83ff2560, 0xe61638bd, 0x87e3ce90, 0x00000001, 0x000000d8, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x00000f0f, 0x49534f50, 0x4e4f4954, 0xababab00,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000001, 0x00000003,
        0x00000000, 0x0000000f, 0x505f5653, 0x5449534f, 0x004e4f49, 0x52444853, 0x0000003c, 0x00010040,
        0x0000000f, 0x0300005f, 0x001010f2, 0x00000000, 0x04000067, 0x001020f2, 0x00000000, 0x00000001,
        0x05000036, 0x001020f2, 0x00000000, 0x00101e46, 0x00000000, 0x0100003e,
    };
    static const DWORD ps_code[] =
    {
#if 0
        Texture2D t;
        SamplerState s;
        float4 dst;
        float4 src;
        float4 backcolor;

        float4 main(float4 position : SV_POSITION) : SV_TARGET
        {
            float2 p;

            if (position.x < dst.x || position.x > dst.z) return backcolor;
            if (position.y < dst.y || position.y > dst.w) return backcolor;
            p.x = (position.x - dst.x) / (dst.z - dst.x);
            p.y = (position.y - dst.y) / (dst.w - dst.y);
            p.x = src.x + p.x * (src.z - src.x);
            p.y = src.y + p.y * (src.w - src.y);
            return t.Sample(s, p);
        }
#endif
        0x43425844, 0xae2162b7, 0x0fd69625, 0x6784c41a, 0x84ae95de, 0x00000001, 0x000002f8, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000030f, 0x505f5653, 0x5449534f, 0x004e4f49,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x545f5653, 0x45475241, 0xabab0054, 0x52444853, 0x0000025c, 0x00000040,
        0x00000097, 0x04000059, 0x00208e46, 0x00000000, 0x00000003, 0x0300005a, 0x00106000, 0x00000000,
        0x04001858, 0x00107000, 0x00000000, 0x00005555, 0x04002064, 0x00101032, 0x00000000, 0x00000001,
        0x03000065, 0x001020f2, 0x00000000, 0x02000068, 0x00000002, 0x08000031, 0x00100012, 0x00000000,
        0x0010100a, 0x00000000, 0x0020800a, 0x00000000, 0x00000000, 0x08000031, 0x00100022, 0x00000000,
        0x0020802a, 0x00000000, 0x00000000, 0x0010100a, 0x00000000, 0x0700003c, 0x00100012, 0x00000000,
        0x0010001a, 0x00000000, 0x0010000a, 0x00000000, 0x09000000, 0x00100062, 0x00000000, 0x00101106,
        0x00000000, 0x80208106, 0x00000041, 0x00000000, 0x00000000, 0x0a000000, 0x00100032, 0x00000001,
        0x80208046, 0x00000041, 0x00000000, 0x00000000, 0x00208ae6, 0x00000000, 0x00000000, 0x0700000e,
        0x00100062, 0x00000000, 0x00100656, 0x00000000, 0x00100106, 0x00000001, 0x0a000000, 0x00100032,
        0x00000001, 0x80208046, 0x00000041, 0x00000000, 0x00000001, 0x00208ae6, 0x00000000, 0x00000001,
        0x0a000032, 0x00100062, 0x00000000, 0x00100656, 0x00000000, 0x00100106, 0x00000001, 0x00208106,
        0x00000000, 0x00000001, 0x09000045, 0x001000f2, 0x00000001, 0x00100596, 0x00000000, 0x00107e46,
        0x00000000, 0x00106000, 0x00000000, 0x0304001f, 0x0010000a, 0x00000000, 0x06000036, 0x001020f2,
        0x00000000, 0x00208e46, 0x00000000, 0x00000002, 0x0100003e, 0x01000015, 0x08000031, 0x00100012,
        0x00000000, 0x0010101a, 0x00000000, 0x0020801a, 0x00000000, 0x00000000, 0x08000031, 0x00100022,
        0x00000000, 0x0020803a, 0x00000000, 0x00000000, 0x0010101a, 0x00000000, 0x0700003c, 0x00100012,
        0x00000000, 0x0010001a, 0x00000000, 0x0010000a, 0x00000000, 0x0304001f, 0x0010000a, 0x00000000,
        0x06000036, 0x001020f2, 0x00000000, 0x00208e46, 0x00000000, 0x00000002, 0x0100003e, 0x01000015,
        0x05000036, 0x001020f2, 0x00000000, 0x00100e46, 0x00000001, 0x0100003e,
    };
    D3D11_SUBRESOURCE_DATA resource_data;
    D3D11_TEXTURE2D_DESC texture_desc;
    D3D11_SAMPLER_DESC sampler_desc;
    D3D11_BUFFER_DESC buffer_desc;
    HRESULT hr;

    if (engine->video_frame.d3d11.source)
        return S_OK;

    /* Default vertex buffer, updated on first transfer call. */
    buffer_desc.ByteWidth = sizeof(engine->video_frame.d3d11.quad);
    buffer_desc.Usage = D3D11_USAGE_DEFAULT;
    buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    buffer_desc.CPUAccessFlags = 0;
    buffer_desc.MiscFlags = 0;
    buffer_desc.StructureByteStride = 0;

    resource_data.pSysMem = engine->video_frame.d3d11.quad;
    resource_data.SysMemPitch = 0;
    resource_data.SysMemSlicePitch = 0;

    if (FAILED(hr = ID3D11Device_CreateBuffer(device, &buffer_desc, &resource_data, &engine->video_frame.d3d11.vb)))
    {
        WARN("Failed to create a vertex buffer, hr %#lx.\n", hr);
        goto failed;
    }

    buffer_desc.ByteWidth = sizeof(engine->video_frame.d3d11.cb);
    buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    if (FAILED(hr = ID3D11Device_CreateBuffer(device, &buffer_desc, NULL, &engine->video_frame.d3d11.ps_cb)))
    {
        WARN("Failed to create a buffer, hr %#lx.\n", hr);
        goto failed;
    }

    /* Source texture. */
    texture_desc.Width = engine->video_frame.size.cx;
    texture_desc.Height = engine->video_frame.size.cy;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = engine->video_frame.output_format;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.SampleDesc.Quality = 0;
    texture_desc.Usage = D3D11_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    texture_desc.CPUAccessFlags = 0;
    texture_desc.MiscFlags = 0;

    if (FAILED(hr = ID3D11Device_CreateTexture2D(device, &texture_desc, NULL, &engine->video_frame.d3d11.source)))
    {
        WARN("Failed to create source texture, hr %#lx.\n", hr);
        goto failed;
    }

    if (FAILED(hr = ID3D11Device_CreateShaderResourceView(device, (ID3D11Resource *)engine->video_frame.d3d11.source,
            NULL, &engine->video_frame.d3d11.srv)))
    {
        WARN("Failed to create SRV, hr %#lx.\n", hr);
        goto failed;
    }

    /* Sampler state. */
    memset(&sampler_desc, 0, sizeof(sampler_desc));
    sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;

    if (FAILED(hr = ID3D11Device_CreateSamplerState(device, &sampler_desc, &engine->video_frame.d3d11.sampler)))
    {
        WARN("Failed to create a sampler state, hr %#lx.\n", hr);
        goto failed;
    }

    /* Input layout */
    if (FAILED(hr = ID3D11Device_CreateInputLayout(device, layout_desc, ARRAY_SIZE(layout_desc), vs_code, sizeof(vs_code),
            &engine->video_frame.d3d11.input_layout)))
    {
        WARN("Failed to create input layout, hr %#lx.\n", hr);
        goto failed;
    }

    /* Shaders */
    if (FAILED(hr = ID3D11Device_CreateVertexShader(device, vs_code, sizeof(vs_code), NULL, &engine->video_frame.d3d11.vs)))
    {
        WARN("Failed to create the vertex shader, hr %#lx.\n", hr);
        goto failed;
    }

    if (FAILED(hr = ID3D11Device_CreatePixelShader(device, ps_code, sizeof(ps_code), NULL, &engine->video_frame.d3d11.ps)))
    {
        WARN("Failed to create the pixel shader, hr %#lx.\n", hr);
        goto failed;
    }

failed:

    return hr;
}

struct range
{
    double start;
    double end;
};

struct time_range
{
    IMFMediaTimeRange IMFMediaTimeRange_iface;
    LONG refcount;

    struct range *ranges;
    size_t count;
    size_t capacity;
};

static struct time_range *impl_from_IMFMediaTimeRange(IMFMediaTimeRange *iface)
{
    return CONTAINING_RECORD(iface, struct time_range, IMFMediaTimeRange_iface);
}

struct media_error
{
    IMFMediaError IMFMediaError_iface;
    LONG refcount;
    unsigned int code;
    HRESULT extended_code;
};

static struct media_error *impl_from_IMFMediaError(IMFMediaError *iface)
{
    return CONTAINING_RECORD(iface, struct media_error, IMFMediaError_iface);
}

static HRESULT WINAPI media_error_QueryInterface(IMFMediaError *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IMFMediaError) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFMediaError_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI media_error_AddRef(IMFMediaError *iface)
{
    struct media_error *me = impl_from_IMFMediaError(iface);
    ULONG refcount = InterlockedIncrement(&me->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI media_error_Release(IMFMediaError *iface)
{
    struct media_error *me = impl_from_IMFMediaError(iface);
    ULONG refcount = InterlockedDecrement(&me->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
        free(me);

    return refcount;
}

static USHORT WINAPI media_error_GetErrorCode(IMFMediaError *iface)
{
    struct media_error *me = impl_from_IMFMediaError(iface);
    TRACE("%p.\n", iface);
    return me->code;
}

static HRESULT WINAPI media_error_GetExtendedErrorCode(IMFMediaError *iface)
{
    struct media_error *me = impl_from_IMFMediaError(iface);
    TRACE("%p.\n", iface);
    return me->extended_code;
}

static HRESULT WINAPI media_error_SetErrorCode(IMFMediaError *iface, MF_MEDIA_ENGINE_ERR code)
{
    struct media_error *me = impl_from_IMFMediaError(iface);

    TRACE("%p, %u.\n", iface, code);

    if ((unsigned int)code > MF_MEDIA_ENGINE_ERR_ENCRYPTED)
        return E_INVALIDARG;

    me->code = code;

    return S_OK;
}

static HRESULT WINAPI media_error_SetExtendedErrorCode(IMFMediaError *iface, HRESULT code)
{
    struct media_error *me = impl_from_IMFMediaError(iface);

    TRACE("%p, %#lx.\n", iface, code);

    me->extended_code = code;

    return S_OK;
}

static const IMFMediaErrorVtbl media_error_vtbl =
{
    media_error_QueryInterface,
    media_error_AddRef,
    media_error_Release,
    media_error_GetErrorCode,
    media_error_GetExtendedErrorCode,
    media_error_SetErrorCode,
    media_error_SetExtendedErrorCode,
};

static HRESULT create_media_error(IMFMediaError **ret)
{
    struct media_error *object;

    *ret = NULL;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IMFMediaError_iface.lpVtbl = &media_error_vtbl;
    object->refcount = 1;

    *ret = &object->IMFMediaError_iface;

    return S_OK;
}

static HRESULT WINAPI time_range_QueryInterface(IMFMediaTimeRange *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IMFMediaTimeRange) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFMediaTimeRange_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI time_range_AddRef(IMFMediaTimeRange *iface)
{
    struct time_range *range = impl_from_IMFMediaTimeRange(iface);
    ULONG refcount = InterlockedIncrement(&range->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI time_range_Release(IMFMediaTimeRange *iface)
{
    struct time_range *range = impl_from_IMFMediaTimeRange(iface);
    ULONG refcount = InterlockedDecrement(&range->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        free(range->ranges);
        free(range);
    }

    return refcount;
}

static DWORD WINAPI time_range_GetLength(IMFMediaTimeRange *iface)
{
    struct time_range *range = impl_from_IMFMediaTimeRange(iface);

    TRACE("%p.\n", iface);

    return range->count;
}

static HRESULT WINAPI time_range_GetStart(IMFMediaTimeRange *iface, DWORD idx, double *start)
{
    struct time_range *range = impl_from_IMFMediaTimeRange(iface);

    TRACE("%p, %lu, %p.\n", iface, idx, start);

    if (idx >= range->count)
        return E_INVALIDARG;

    *start = range->ranges[idx].start;

    return S_OK;
}

static HRESULT WINAPI time_range_GetEnd(IMFMediaTimeRange *iface, DWORD idx, double *end)
{
    struct time_range *range = impl_from_IMFMediaTimeRange(iface);

    TRACE("%p, %lu, %p.\n", iface, idx, end);

    if (idx >= range->count)
        return E_INVALIDARG;

    *end = range->ranges[idx].end;

    return S_OK;
}

static BOOL WINAPI time_range_ContainsTime(IMFMediaTimeRange *iface, double time)
{
    struct time_range *range = impl_from_IMFMediaTimeRange(iface);
    size_t i;

    TRACE("%p, %.8e.\n", iface, time);

    for (i = 0; i < range->count; ++i)
    {
        if (time >= range->ranges[i].start && time <= range->ranges[i].end)
            return TRUE;
    }

    return FALSE;
}

static HRESULT WINAPI time_range_AddRange(IMFMediaTimeRange *iface, double start, double end)
{
    struct time_range *range = impl_from_IMFMediaTimeRange(iface);
    struct range *c;
    size_t i;

    TRACE("%p, %.8e, %.8e.\n", iface, start, end);

    for (i = 0; i < range->count; ++i)
    {
        c = &range->ranges[i];

        /* New range is fully contained within existing one. */
        if (c->start <= start && c->end >= end)
            return S_OK;

        /* New range fully contains existing one. */
        if (c->start >= start && c->end <= end)
        {
            c->start = start;
            c->end = end;
            return S_OK;
        }

        /* Merge if ranges intersect. */
        if ((start >= c->start && start <= c->end) ||
                (end >= c->start && end <= c->end))
        {
            c->start = min(c->start, start);
            c->end = max(c->end, end);
            return S_OK;
        }
    }

    if (!mf_array_reserve((void **)&range->ranges, &range->capacity, range->count + 1, sizeof(*range->ranges)))
        return E_OUTOFMEMORY;

    range->ranges[range->count].start = start;
    range->ranges[range->count].end = end;
    range->count++;

    return S_OK;
}

static HRESULT WINAPI time_range_Clear(IMFMediaTimeRange *iface)
{
    struct time_range *range = impl_from_IMFMediaTimeRange(iface);

    TRACE("%p.\n", iface);

    range->count = 0;

    return S_OK;
}

static const IMFMediaTimeRangeVtbl time_range_vtbl =
{
    time_range_QueryInterface,
    time_range_AddRef,
    time_range_Release,
    time_range_GetLength,
    time_range_GetStart,
    time_range_GetEnd,
    time_range_ContainsTime,
    time_range_AddRange,
    time_range_Clear,
};

static HRESULT create_time_range(IMFMediaTimeRange **range)
{
    struct time_range *object;

    object = calloc(1, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IMFMediaTimeRange_iface.lpVtbl = &time_range_vtbl;
    object->refcount = 1;

    *range = &object->IMFMediaTimeRange_iface;

    return S_OK;
}

static void media_engine_set_flag(struct media_engine *engine, unsigned int mask, BOOL value)
{
    if (value)
        engine->flags |= mask;
    else
        engine->flags &= ~mask;
}

static inline struct media_engine *impl_from_IMFMediaEngineEx(IMFMediaEngineEx *iface)
{
    return CONTAINING_RECORD(iface, struct media_engine, IMFMediaEngineEx_iface);
}

static inline struct media_engine *impl_from_IMFGetService(IMFGetService *iface)
{
    return CONTAINING_RECORD(iface, struct media_engine, IMFGetService_iface);
}

static struct media_engine *impl_from_session_events_IMFAsyncCallback(IMFAsyncCallback *iface)
{
    return CONTAINING_RECORD(iface, struct media_engine, session_events);
}

static struct media_engine *impl_from_sink_events_IMFAsyncCallback(IMFAsyncCallback *iface)
{
    return CONTAINING_RECORD(iface, struct media_engine, sink_events);
}

static struct media_engine *impl_from_load_handler_IMFAsyncCallback(IMFAsyncCallback *iface)
{
    return CONTAINING_RECORD(iface, struct media_engine, load_handler);
}

static unsigned int get_gcd(unsigned int a, unsigned int b)
{
    unsigned int m;

    while (b)
    {
        m = a % b;
        a = b;
        b = m;
    }

    return a;
}

static void media_engine_get_frame_size(struct media_engine *engine, IMFTopology *topology)
{
    IMFMediaTypeHandler *handler;
    IMFMediaType *media_type;
    IMFStreamDescriptor *sd;
    IMFTopologyNode *node;
    unsigned int gcd;
    UINT64 size;
    HRESULT hr;

    engine->video_frame.size.cx = 0;
    engine->video_frame.size.cy = 0;
    engine->video_frame.ratio.cx = 1;
    engine->video_frame.ratio.cy = 1;

    if (FAILED(IMFTopology_GetNodeByID(topology, engine->video_frame.node_id, &node)))
        return;

    hr = IMFTopologyNode_GetUnknown(node, &MF_TOPONODE_STREAM_DESCRIPTOR,
            &IID_IMFStreamDescriptor, (void **)&sd);
    IMFTopologyNode_Release(node);
    if (FAILED(hr))
        return;

    hr = IMFStreamDescriptor_GetMediaTypeHandler(sd, &handler);
    IMFStreamDescriptor_Release(sd);
    if (FAILED(hr))
        return;

    hr = IMFMediaTypeHandler_GetCurrentMediaType(handler, &media_type);
    IMFMediaTypeHandler_Release(handler);
    if (FAILED(hr))
    {
        WARN("Failed to get current media type %#lx.\n", hr);
        return;
    }

    IMFMediaType_GetUINT64(media_type, &MF_MT_FRAME_SIZE, &size);

    engine->video_frame.size.cx = size >> 32;
    engine->video_frame.size.cy = size;

    if ((gcd = get_gcd(engine->video_frame.size.cx, engine->video_frame.size.cy)))
    {
        engine->video_frame.ratio.cx = engine->video_frame.size.cx / gcd;
        engine->video_frame.ratio.cy = engine->video_frame.size.cy / gcd;
    }

    IMFMediaType_Release(media_type);
}

static void media_engine_apply_volume(const struct media_engine *engine)
{
    IMFSimpleAudioVolume *sa_volume;
    HRESULT hr;

    if (!engine->session)
        return;

    if (FAILED(MFGetService((IUnknown *)engine->session, &MR_POLICY_VOLUME_SERVICE, &IID_IMFSimpleAudioVolume, (void **)&sa_volume)))
        return;

    if (FAILED(hr = IMFSimpleAudioVolume_SetMasterVolume(sa_volume, engine->volume)))
        WARN("Failed to set master volume, hr %#lx.\n", hr);

    IMFSimpleAudioVolume_Release(sa_volume);
}

static HRESULT WINAPI media_engine_callback_QueryInterface(IMFAsyncCallback *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IMFAsyncCallback) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFAsyncCallback_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI media_engine_session_events_AddRef(IMFAsyncCallback *iface)
{
    struct media_engine *engine = impl_from_session_events_IMFAsyncCallback(iface);
    return IMFMediaEngineEx_AddRef(&engine->IMFMediaEngineEx_iface);
}

static ULONG WINAPI media_engine_session_events_Release(IMFAsyncCallback *iface)
{
    struct media_engine *engine = impl_from_session_events_IMFAsyncCallback(iface);
    return IMFMediaEngineEx_Release(&engine->IMFMediaEngineEx_iface);
}

static HRESULT WINAPI media_engine_callback_GetParameters(IMFAsyncCallback *iface, DWORD *flags, DWORD *queue)
{
    return E_NOTIMPL;
}

static HRESULT media_engine_set_current_time(struct media_engine *engine, double seektime);

static HRESULT WINAPI media_engine_session_events_Invoke(IMFAsyncCallback *iface, IMFAsyncResult *result)
{
    struct media_engine *engine = impl_from_session_events_IMFAsyncCallback(iface);
    IMFMediaEvent *event = NULL;
    MediaEventType event_type;
    HRESULT hr;

    if (FAILED(hr = IMFMediaSession_EndGetEvent(engine->session, result, &event)))
    {
        WARN("Failed to get session event, hr %#lx.\n", hr);
        goto failed;
    }

    if (FAILED(hr = IMFMediaEvent_GetType(event, &event_type)))
    {
        WARN("Failed to get event type, hr %#lx.\n", hr);
        goto failed;
    }

    switch (event_type)
    {
        case MEBufferingStarted:
        case MEBufferingStopped:

            IMFMediaEngineNotify_EventNotify(engine->callback, event_type == MEBufferingStarted ?
                    MF_MEDIA_ENGINE_EVENT_BUFFERINGSTARTED : MF_MEDIA_ENGINE_EVENT_BUFFERINGENDED, 0, 0);
            break;
        case MESessionTopologyStatus:
        {
            UINT32 topo_status = 0;
            IMFTopology *topology;
            PROPVARIANT value;

            IMFMediaEvent_GetUINT32(event, &MF_EVENT_TOPOLOGY_STATUS, &topo_status);
            if (topo_status != MF_TOPOSTATUS_READY)
                break;

            value.vt = VT_EMPTY;
            if (FAILED(IMFMediaEvent_GetValue(event, &value)))
                break;

            if (value.vt != VT_UNKNOWN)
            {
                PropVariantClear(&value);
                break;
            }

            topology = (IMFTopology *)value.punkVal;

            EnterCriticalSection(&engine->cs);

            media_engine_apply_volume(engine);

            engine->ready_state = MF_MEDIA_ENGINE_READY_HAVE_METADATA;

            media_engine_get_frame_size(engine, topology);

            IMFMediaEngineNotify_EventNotify(engine->callback, MF_MEDIA_ENGINE_EVENT_DURATIONCHANGE, 0, 0);
            IMFMediaEngineNotify_EventNotify(engine->callback, MF_MEDIA_ENGINE_EVENT_LOADEDMETADATA, 0, 0);

            engine->ready_state = MF_MEDIA_ENGINE_READY_HAVE_ENOUGH_DATA;

            IMFMediaEngineNotify_EventNotify(engine->callback, MF_MEDIA_ENGINE_EVENT_LOADEDDATA, 0, 0);
            IMFMediaEngineNotify_EventNotify(engine->callback, MF_MEDIA_ENGINE_EVENT_CANPLAY, 0, 0);

            LeaveCriticalSection(&engine->cs);

            PropVariantClear(&value);

            break;
        }
        case MESessionStarted:
            EnterCriticalSection(&engine->cs);
            if (engine->flags & FLAGS_ENGINE_SEEKING)
            {
                media_engine_set_flag(engine, FLAGS_ENGINE_SEEKING | FLAGS_ENGINE_IS_ENDED, FALSE);
                IMFMediaEngineNotify_EventNotify(engine->callback, MF_MEDIA_ENGINE_EVENT_SEEKED, 0, 0);
                IMFMediaEngineNotify_EventNotify(engine->callback, MF_MEDIA_ENGINE_EVENT_TIMEUPDATE, 0, 0);
                if (isfinite(engine->next_seek))
                    media_engine_set_current_time(engine, engine->next_seek);
            }
            LeaveCriticalSection(&engine->cs);
            IMFMediaEngineNotify_EventNotify(engine->callback, MF_MEDIA_ENGINE_EVENT_PLAYING, 0, 0);
            break;
        case MESessionEnded:

            EnterCriticalSection(&engine->cs);
            media_engine_set_flag(engine, FLAGS_ENGINE_FIRST_FRAME, FALSE);
            media_engine_set_flag(engine, FLAGS_ENGINE_IS_ENDED, TRUE);
            engine->video_frame.pts = MINLONGLONG;
            LeaveCriticalSection(&engine->cs);

            IMFMediaEngineNotify_EventNotify(engine->callback, MF_MEDIA_ENGINE_EVENT_ENDED, 0, 0);
            break;

        case MEEndOfPresentationSegment:
            video_frame_sink_notify_end_of_presentation_segment(engine->presentation.frame_sink);
            break;
    }

failed:

    if (event)
        IMFMediaEvent_Release(event);

    if (FAILED(hr = IMFMediaSession_BeginGetEvent(engine->session, iface, NULL)))
        WARN("Failed to subscribe to session events, hr %#lx.\n", hr);

    return S_OK;
}

static const IMFAsyncCallbackVtbl media_engine_session_events_vtbl =
{
    media_engine_callback_QueryInterface,
    media_engine_session_events_AddRef,
    media_engine_session_events_Release,
    media_engine_callback_GetParameters,
    media_engine_session_events_Invoke,
};

static ULONG WINAPI media_engine_sink_events_AddRef(IMFAsyncCallback *iface)
{
    struct media_engine *engine = impl_from_sink_events_IMFAsyncCallback(iface);
    return IMFMediaEngineEx_AddRef(&engine->IMFMediaEngineEx_iface);
}

static ULONG WINAPI media_engine_sink_events_Release(IMFAsyncCallback *iface)
{
    struct media_engine *engine = impl_from_sink_events_IMFAsyncCallback(iface);
    return IMFMediaEngineEx_Release(&engine->IMFMediaEngineEx_iface);
}

static HRESULT WINAPI media_engine_sink_events_Invoke(IMFAsyncCallback *iface, IMFAsyncResult *result)
{
    struct media_engine *engine = impl_from_sink_events_IMFAsyncCallback(iface);
    MF_MEDIA_ENGINE_EVENT event = IMFAsyncResult_GetStatus(result);

    EnterCriticalSection(&engine->cs);

    switch (event)
    {
        case MF_MEDIA_ENGINE_EVENT_FIRSTFRAMEREADY:
            IMFMediaEngineNotify_EventNotify(engine->callback, event, 0, 0);
            break;
        default:
            ;
    }

    LeaveCriticalSection(&engine->cs);

    return S_OK;
}

static const IMFAsyncCallbackVtbl media_engine_sink_events_vtbl =
{
    media_engine_callback_QueryInterface,
    media_engine_sink_events_AddRef,
    media_engine_sink_events_Release,
    media_engine_callback_GetParameters,
    media_engine_sink_events_Invoke,
};

static ULONG WINAPI media_engine_load_handler_AddRef(IMFAsyncCallback *iface)
{
    struct media_engine *engine = impl_from_load_handler_IMFAsyncCallback(iface);
    return IMFMediaEngineEx_AddRef(&engine->IMFMediaEngineEx_iface);
}

static ULONG WINAPI media_engine_load_handler_Release(IMFAsyncCallback *iface)
{
    struct media_engine *engine = impl_from_load_handler_IMFAsyncCallback(iface);
    return IMFMediaEngineEx_Release(&engine->IMFMediaEngineEx_iface);
}

static HRESULT media_engine_create_source_node(IMFMediaSource *source, IMFPresentationDescriptor *pd, IMFStreamDescriptor *sd,
        IMFTopologyNode **node)
{
    HRESULT hr;

    if (FAILED(hr = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, node)))
        return hr;

    IMFTopologyNode_SetUnknown(*node, &MF_TOPONODE_SOURCE, (IUnknown *)source);
    IMFTopologyNode_SetUnknown(*node, &MF_TOPONODE_PRESENTATION_DESCRIPTOR, (IUnknown *)pd);
    IMFTopologyNode_SetUnknown(*node, &MF_TOPONODE_STREAM_DESCRIPTOR, (IUnknown *)sd);

    return S_OK;
}

static HRESULT media_engine_create_effects(struct effect *effects, size_t count,
    IMFTopologyNode *src, IMFTopologyNode *sink, IMFTopology *topology)
{
    IMFTopologyNode *last = src;
    HRESULT hr = S_OK;
    size_t i;

    IMFTopologyNode_AddRef(last);

    for (i = 0; i < count; ++i)
    {
        UINT32 method = MF_CONNECT_ALLOW_DECODER;
        IMFTopologyNode *node = NULL;

        if (FAILED(hr = MFCreateTopologyNode(MF_TOPOLOGY_TRANSFORM_NODE, &node)))
        {
            WARN("Failed to create transform node, hr %#lx.\n", hr);
            break;
        }

        IMFTopologyNode_SetObject(node, (IUnknown *)effects[i].object);
        IMFTopologyNode_SetUINT32(node, &MF_TOPONODE_NOSHUTDOWN_ON_REMOVE, FALSE);

        if (effects[i].optional)
            method |= MF_CONNECT_AS_OPTIONAL;
        IMFTopologyNode_SetUINT32(node, &MF_TOPONODE_CONNECT_METHOD, method);

        IMFTopology_AddNode(topology, node);
        IMFTopologyNode_ConnectOutput(last, 0, node, 0);

        IMFTopologyNode_Release(last);
        last = node;
    }

    IMFTopologyNode_Release(last);

    if (SUCCEEDED(hr))
        hr = IMFTopologyNode_ConnectOutput(last, 0, sink, 0);

    return hr;
}

static HRESULT media_engine_create_audio_renderer(struct media_engine *engine, IMFTopologyNode **node)
{
    unsigned int category, role;
    IMFActivate *sar_activate;
    HRESULT hr;

    *node = NULL;

    if (FAILED(hr = MFCreateAudioRendererActivate(&sar_activate)))
        return hr;

    /* Configuration attributes keys differ between Engine and SAR. */
    if (SUCCEEDED(IMFAttributes_GetUINT32(engine->attributes, &MF_MEDIA_ENGINE_AUDIO_CATEGORY, &category)))
        IMFActivate_SetUINT32(sar_activate, &MF_AUDIO_RENDERER_ATTRIBUTE_STREAM_CATEGORY, category);
    if (SUCCEEDED(IMFAttributes_GetUINT32(engine->attributes, &MF_MEDIA_ENGINE_AUDIO_ENDPOINT_ROLE, &role)))
        IMFActivate_SetUINT32(sar_activate, &MF_AUDIO_RENDERER_ATTRIBUTE_ENDPOINT_ROLE, role);

    if (SUCCEEDED(hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, node)))
    {
        IMFTopologyNode_SetObject(*node, (IUnknown *)sar_activate);
        IMFTopologyNode_SetUINT32(*node, &MF_TOPONODE_NOSHUTDOWN_ON_REMOVE, FALSE);
    }

    IMFActivate_Release(sar_activate);

    return hr;
}

static HRESULT media_engine_create_video_renderer(struct media_engine *engine, IMFTopologyNode **node)
{
    IMFMediaType *media_type;
    UINT32 output_format;
    GUID subtype;
    HRESULT hr;

    *node = NULL;

    if (FAILED(IMFAttributes_GetUINT32(engine->attributes, &MF_MEDIA_ENGINE_VIDEO_OUTPUT_FORMAT, &output_format)))
    {
        WARN("Output format was not specified.\n");
        return E_FAIL;
    }

    memcpy(&subtype, &MFVideoFormat_Base, sizeof(subtype));
    if (!(subtype.Data1 = MFMapDXGIFormatToDX9Format(output_format)))
    {
        WARN("Unrecognized output format %#x.\n", output_format);
        return E_FAIL;
    }

    if (FAILED(hr = MFCreateMediaType(&media_type)))
        return hr;

    IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &subtype);

    hr = create_video_frame_sink(media_type, (IUnknown *)engine->device_manager, &engine->sink_events, &engine->presentation.frame_sink);
    IMFMediaType_Release(media_type);
    if (FAILED(hr))
        return hr;

    if (SUCCEEDED(hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, node)))
    {
        IMFStreamSink *sink;
        video_frame_sink_query_iface(engine->presentation.frame_sink, &IID_IMFStreamSink, (void **)&sink);

        IMFTopologyNode_SetObject(*node, (IUnknown *)sink);
        IMFTopologyNode_SetUINT32(*node, &MF_TOPONODE_NOSHUTDOWN_ON_REMOVE, FALSE);

        IMFStreamSink_Release(sink);
    }

    engine->video_frame.output_format = output_format;

    return hr;
}

/* must be called with engine->cs held */
static void media_engine_clear_presentation(struct media_engine *engine)
{
    if (engine->presentation.source)
    {
        /* critical section can not be held during shutdown, as shut down requires all pending
         * callbacks to complete, and some callbacks require this cs */
        LeaveCriticalSection(&engine->cs);
        IMFMediaSource_Shutdown(engine->presentation.source);
        EnterCriticalSection(&engine->cs);
        IMFMediaSource_Release(engine->presentation.source);
    }
    if (engine->presentation.pd)
        IMFPresentationDescriptor_Release(engine->presentation.pd);
    if (engine->presentation.frame_sink)
    {
        video_frame_sink_release(engine->presentation.frame_sink);
        engine->presentation.frame_sink = NULL;
    }

    memset(&engine->presentation, 0, sizeof(engine->presentation));
}

static void media_engine_clear_effects(struct effects *effects)
{
    size_t i;

    for (i = 0; i < effects->count; ++i)
    {
        if (effects->effects[i].object)
            IUnknown_Release(effects->effects[i].object);
    }

    free(effects->effects);
    memset(effects, 0, sizeof(*effects));
}

static HRESULT media_engine_create_topology(struct media_engine *engine, IMFMediaSource *source)
{
    IMFStreamDescriptor *sd_audio = NULL, *sd_video = NULL;
    IMFPresentationDescriptor *pd;
    DWORD stream_count = 0, i;
    IMFTopology *topology;
    UINT64 duration;
    HRESULT hr;

    media_engine_release_video_frame_resources(engine);
    media_engine_clear_presentation(engine);

    if (FAILED(hr = IMFMediaSource_CreatePresentationDescriptor(source, &pd)))
        return hr;

    if (FAILED(hr = IMFPresentationDescriptor_GetStreamDescriptorCount(pd, &stream_count)))
        WARN("Failed to get stream count, hr %#lx.\n", hr);

    /* Enable first video stream and first audio stream. */

    for (i = 0; i < stream_count; ++i)
    {
        IMFMediaTypeHandler *type_handler;
        IMFStreamDescriptor *sd;
        BOOL selected;

        IMFPresentationDescriptor_DeselectStream(pd, i);

        if (sd_audio && sd_video)
            continue;

        IMFPresentationDescriptor_GetStreamDescriptorByIndex(pd, i, &selected, &sd);

        if (SUCCEEDED(IMFStreamDescriptor_GetMediaTypeHandler(sd, &type_handler)))
        {
            GUID major = { 0 };

            IMFMediaTypeHandler_GetMajorType(type_handler, &major);

            if (IsEqualGUID(&major, &MFMediaType_Audio) && !sd_audio)
            {
                sd_audio = sd;
                IMFStreamDescriptor_AddRef(sd_audio);
                IMFPresentationDescriptor_SelectStream(pd, i);
            }
            else if (IsEqualGUID(&major, &MFMediaType_Video) && !sd_video && !(engine->flags & MF_MEDIA_ENGINE_AUDIOONLY))
            {
                sd_video = sd;
                IMFStreamDescriptor_AddRef(sd_video);
                IMFPresentationDescriptor_SelectStream(pd, i);
            }

            IMFMediaTypeHandler_Release(type_handler);
        }

        IMFStreamDescriptor_Release(sd);
    }

    if (!sd_video && !sd_audio)
    {
        IMFPresentationDescriptor_Release(pd);
        return E_UNEXPECTED;
    }

    engine->presentation.source = source;
    IMFMediaSource_AddRef(engine->presentation.source);
    engine->presentation.pd = pd;
    IMFPresentationDescriptor_AddRef(engine->presentation.pd);

    media_engine_set_flag(engine, FLAGS_ENGINE_HAS_VIDEO, !!sd_video);
    media_engine_set_flag(engine, FLAGS_ENGINE_HAS_AUDIO, !!sd_audio);

    /* Assume live source if duration was not provided. */
    if (SUCCEEDED(IMFPresentationDescriptor_GetUINT64(pd, &MF_PD_DURATION, &duration)))
        engine->duration = mftime_to_seconds(duration);
    else
        engine->duration = INFINITY;

    if (SUCCEEDED(hr = MFCreateTopology(&topology)))
    {
        IMFTopologyNode *sar_node = NULL, *audio_src = NULL;
        IMFTopologyNode *svr_node = NULL, *video_src = NULL;

        if (engine->flags & MF_MEDIA_ENGINE_REAL_TIME_MODE)
            IMFTopology_SetUINT32(topology, &MF_LOW_LATENCY, TRUE);

        if (sd_audio)
        {
            if (FAILED(hr = media_engine_create_source_node(source, pd, sd_audio, &audio_src)))
                WARN("Failed to create audio source node, hr %#lx.\n", hr);

            if (FAILED(hr = media_engine_create_audio_renderer(engine, &sar_node)))
                WARN("Failed to create audio renderer node, hr %#lx.\n", hr);

            if (sar_node && audio_src)
            {
                IMFTopology_AddNode(topology, audio_src);
                IMFTopology_AddNode(topology, sar_node);

                if (FAILED(hr = media_engine_create_effects(engine->audio_effects.effects, engine->audio_effects.count,
                        audio_src, sar_node, topology)))
                    WARN("Failed to create audio effect nodes, hr %#lx.\n", hr);
            }

            if (sar_node)
                IMFTopologyNode_Release(sar_node);
            if (audio_src)
                IMFTopologyNode_Release(audio_src);
        }

        if (SUCCEEDED(hr) && sd_video)
        {
            if (FAILED(hr = media_engine_create_source_node(source, pd, sd_video, &video_src)))
                WARN("Failed to create video source node, hr %#lx.\n", hr);

            if (FAILED(hr = media_engine_create_video_renderer(engine, &svr_node)))
                WARN("Failed to create simple video render node, hr %#lx.\n", hr);

            if (svr_node && video_src)
            {
                IMFTopology_AddNode(topology, video_src);
                IMFTopology_AddNode(topology, svr_node);

                if (FAILED(hr = media_engine_create_effects(engine->video_effects.effects, engine->video_effects.count,
                        video_src, svr_node, topology)))
                    WARN("Failed to create video effect nodes, hr %#lx.\n", hr);
            }

            if (SUCCEEDED(hr))
                IMFTopologyNode_GetTopoNodeID(video_src, &engine->video_frame.node_id);

            if (svr_node)
                IMFTopologyNode_Release(svr_node);
            if (video_src)
                IMFTopologyNode_Release(video_src);
        }

        IMFTopology_SetUINT32(topology, &MF_TOPOLOGY_ENUMERATE_SOURCE_TYPES, TRUE);
        IMFTopology_SetUINT32(topology, &MF_TOPOLOGY_ENABLE_XVP_FOR_PLAYBACK, TRUE);

        if (SUCCEEDED(hr))
            hr = IMFMediaSession_SetTopology(engine->session, MFSESSION_SETTOPOLOGY_IMMEDIATE, topology);
    }

    if (topology)
        IMFTopology_Release(topology);

    if (sd_video)
        IMFStreamDescriptor_Release(sd_video);
    if (sd_audio)
        IMFStreamDescriptor_Release(sd_audio);

    IMFPresentationDescriptor_Release(pd);

    return hr;
}

static void media_engine_start_playback(struct media_engine *engine)
{
    IMFMediaSession_Start(engine->session, &GUID_NULL, &engine->presentation.start_position);
    /* Reset the playback position to the current position */
    engine->presentation.start_position.vt = VT_EMPTY;
}

static HRESULT WINAPI media_engine_load_handler_Invoke(IMFAsyncCallback *iface, IMFAsyncResult *result)
{
    struct media_engine *engine = impl_from_load_handler_IMFAsyncCallback(iface);
    IUnknown *object = NULL, *state;
    unsigned int start_playback;
    MF_OBJECT_TYPE obj_type;
    IMFMediaSource *source;
    HRESULT hr;

    EnterCriticalSection(&engine->cs);

    if (engine->flags & FLAGS_ENGINE_SHUT_DOWN)
    {
        LeaveCriticalSection(&engine->cs);
        return S_OK;
    }

    engine->network_state = MF_MEDIA_ENGINE_NETWORK_LOADING;
    IMFMediaEngineNotify_EventNotify(engine->callback, MF_MEDIA_ENGINE_EVENT_LOADSTART, 0, 0);

    start_playback = engine->flags & FLAGS_ENGINE_PLAY_PENDING;
    media_engine_set_flag(engine, FLAGS_ENGINE_SOURCE_PENDING | FLAGS_ENGINE_PLAY_PENDING, FALSE);

    if (SUCCEEDED(IMFAsyncResult_GetState(result, &state)))
    {
        hr = IMFSourceResolver_EndCreateObjectFromByteStream(engine->resolver, result, &obj_type, &object);
        IUnknown_Release(state);
    }
    else
        hr = IMFSourceResolver_EndCreateObjectFromURL(engine->resolver, result, &obj_type, &object);

    if (FAILED(hr))
        WARN("Failed to create source object, hr %#lx.\n", hr);

    if (object)
    {
        if (SUCCEEDED(hr = IUnknown_QueryInterface(object, &IID_IMFMediaSource, (void **)&source)))
        {
            hr = media_engine_create_topology(engine, source);
            IMFMediaSource_Release(source);
        }
        IUnknown_Release(object);
    }

    if (SUCCEEDED(hr))
    {
        engine->network_state = MF_MEDIA_ENGINE_NETWORK_IDLE;
        if (start_playback)
            media_engine_start_playback(engine);
    }
    else
    {
        engine->network_state = MF_MEDIA_ENGINE_NETWORK_NO_SOURCE;
        engine->error_code = MF_MEDIA_ENGINE_ERR_SRC_NOT_SUPPORTED;
        engine->extended_code = hr;
        IMFMediaEngineNotify_EventNotify(engine->callback, MF_MEDIA_ENGINE_EVENT_ERROR, engine->error_code,
                engine->extended_code);
    }

    LeaveCriticalSection(&engine->cs);

    return S_OK;
}

static const IMFAsyncCallbackVtbl media_engine_load_handler_vtbl =
{
    media_engine_callback_QueryInterface,
    media_engine_load_handler_AddRef,
    media_engine_load_handler_Release,
    media_engine_callback_GetParameters,
    media_engine_load_handler_Invoke,
};

static HRESULT WINAPI media_engine_QueryInterface(IMFMediaEngineEx *iface, REFIID riid, void **obj)
{
    struct media_engine *engine = impl_from_IMFMediaEngineEx(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IMFMediaEngineEx) ||
        IsEqualIID(riid, &IID_IMFMediaEngine) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
    }
    else if (IsEqualIID(riid, &IID_IMFGetService))
    {
        *obj = &engine->IMFGetService_iface;
    }
    else
    {
        WARN("Unsupported interface %s.\n", debugstr_guid(riid));
        *obj = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*obj);
    return S_OK;
}

static ULONG WINAPI media_engine_AddRef(IMFMediaEngineEx *iface)
{
    struct media_engine *engine = impl_from_IMFMediaEngineEx(iface);
    ULONG refcount = InterlockedIncrement(&engine->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static void free_media_engine(struct media_engine *engine)
{
    if (engine->callback)
        IMFMediaEngineNotify_Release(engine->callback);
    if (engine->clock)
        IMFPresentationClock_Release(engine->clock);
    if (engine->session)
        IMFMediaSession_Release(engine->session);
    if (engine->attributes)
        IMFAttributes_Release(engine->attributes);
    if (engine->resolver)
        IMFSourceResolver_Release(engine->resolver);
    if (engine->extension)
        IMFMediaEngineExtension_Release(engine->extension);
    media_engine_clear_effects(&engine->audio_effects);
    media_engine_clear_effects(&engine->video_effects);
    media_engine_release_video_frame_resources(engine);
    media_engine_clear_presentation(engine);
    if (engine->device_manager)
    {
        IMFDXGIDeviceManager_CloseDeviceHandle(engine->device_manager, engine->device_handle);
        IMFDXGIDeviceManager_Release(engine->device_manager);
    }
    SysFreeString(engine->current_source);
    DeleteCriticalSection(&engine->cs);
    free(engine->video_frame.buffer);
    free(engine);
}

static ULONG WINAPI media_engine_Release(IMFMediaEngineEx *iface)
{
    struct media_engine *engine = impl_from_IMFMediaEngineEx(iface);
    ULONG refcount = InterlockedDecrement(&engine->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
        free_media_engine(engine);

    return refcount;
}

static HRESULT WINAPI media_engine_GetError(IMFMediaEngineEx *iface, IMFMediaError **error)
{
    struct media_engine *engine = impl_from_IMFMediaEngineEx(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, error);

    *error = NULL;

    EnterCriticalSection(&engine->cs);
    if (engine->flags & FLAGS_ENGINE_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else if (engine->error_code)
    {
        if (SUCCEEDED(hr = create_media_error(error)))
        {
            IMFMediaError_SetErrorCode(*error, engine->error_code);
            IMFMediaError_SetExtendedErrorCode(*error, engine->extended_code);
        }
    }
    LeaveCriticalSection(&engine->cs);

    return hr;
}

static HRESULT WINAPI media_engine_SetErrorCode(IMFMediaEngineEx *iface, MF_MEDIA_ENGINE_ERR code)
{
    struct media_engine *engine = impl_from_IMFMediaEngineEx(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %u.\n", iface, code);

    if ((unsigned int)code > MF_MEDIA_ENGINE_ERR_ENCRYPTED)
        return E_INVALIDARG;

    EnterCriticalSection(&engine->cs);
    if (engine->flags & FLAGS_ENGINE_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else
        engine->error_code = code;
    LeaveCriticalSection(&engine->cs);

    return hr;
}

static HRESULT WINAPI media_engine_SetSourceElements(IMFMediaEngineEx *iface, IMFMediaEngineSrcElements *elements)
{
    FIXME("(%p, %p): stub.\n", iface, elements);

    return E_NOTIMPL;
}

static HRESULT media_engine_set_source(struct media_engine *engine, IMFByteStream *bytestream, BSTR url)
{
    IPropertyStore *props = NULL;
    unsigned int flags;
    HRESULT hr = S_OK;

    SysFreeString(engine->current_source);
    engine->current_source = NULL;
    if (url)
        engine->current_source = SysAllocString(url);

    engine->ready_state = MF_MEDIA_ENGINE_READY_HAVE_NOTHING;

    IMFMediaEngineNotify_EventNotify(engine->callback, MF_MEDIA_ENGINE_EVENT_PURGEQUEUEDEVENTS, 0, 0);

    engine->network_state = MF_MEDIA_ENGINE_NETWORK_NO_SOURCE;

    if (url || bytestream)
    {
        if (engine->extension)
            FIXME("Use extension to load from.\n");

        flags = MF_RESOLUTION_MEDIASOURCE | MF_RESOLUTION_CONTENT_DOES_NOT_HAVE_TO_MATCH_EXTENSION_OR_MIME_TYPE;
        if (engine->flags & MF_MEDIA_ENGINE_DISABLE_LOCAL_PLUGINS)
            flags |= MF_RESOLUTION_DISABLE_LOCAL_PLUGINS;

        IMFAttributes_GetUnknown(engine->attributes, &MF_MEDIA_ENGINE_SOURCE_RESOLVER_CONFIG_STORE,
                &IID_IPropertyStore, (void **)&props);
        if (bytestream)
            hr = IMFSourceResolver_BeginCreateObjectFromByteStream(engine->resolver, bytestream, url, flags,
                    props, NULL, &engine->load_handler, (IUnknown *)bytestream);
        else
            hr = IMFSourceResolver_BeginCreateObjectFromURL(engine->resolver, url, flags, props, NULL,
                    &engine->load_handler, NULL);
        if (SUCCEEDED(hr))
            media_engine_set_flag(engine, FLAGS_ENGINE_SOURCE_PENDING, TRUE);

        if (props)
            IPropertyStore_Release(props);
    }

    return hr;
}

static HRESULT WINAPI media_engine_SetSource(IMFMediaEngineEx *iface, BSTR url)
{
    struct media_engine *engine = impl_from_IMFMediaEngineEx(iface);
    HRESULT hr;

    TRACE("%p, %s.\n", iface, debugstr_w(url));

    EnterCriticalSection(&engine->cs);

    if (engine->flags & FLAGS_ENGINE_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else
        hr = media_engine_set_source(engine, NULL, url);

    LeaveCriticalSection(&engine->cs);

    return hr;
}

static HRESULT WINAPI media_engine_GetCurrentSource(IMFMediaEngineEx *iface, BSTR *url)
{
    struct media_engine *engine = impl_from_IMFMediaEngineEx(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, url);

    *url = NULL;

    EnterCriticalSection(&engine->cs);

    if (engine->flags & FLAGS_ENGINE_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    if (engine->current_source)
    {
        if (!(*url = SysAllocString(engine->current_source)))
            hr = E_OUTOFMEMORY;
    }

    LeaveCriticalSection(&engine->cs);

    return hr;
}

static USHORT WINAPI media_engine_GetNetworkState(IMFMediaEngineEx *iface)
{
    struct media_engine *engine = impl_from_IMFMediaEngineEx(iface);

    TRACE("%p.\n", iface);

    return engine->network_state;
}

static MF_MEDIA_ENGINE_PRELOAD WINAPI media_engine_GetPreload(IMFMediaEngineEx *iface)
{
    struct media_engine *engine = impl_from_IMFMediaEngineEx(iface);
    MF_MEDIA_ENGINE_PRELOAD preload;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&engine->cs);
    preload = engine->preload;
    LeaveCriticalSection(&engine->cs);

    return preload;
}

static HRESULT WINAPI media_engine_SetPreload(IMFMediaEngineEx *iface, MF_MEDIA_ENGINE_PRELOAD preload)
{
    struct media_engine *engine = impl_from_IMFMediaEngineEx(iface);

    TRACE("%p, %d.\n", iface, preload);

    EnterCriticalSection(&engine->cs);
    engine->preload = preload;
    LeaveCriticalSection(&engine->cs);

    return S_OK;
}

static HRESULT WINAPI media_engine_GetBuffered(IMFMediaEngineEx *iface, IMFMediaTimeRange **range)
{
    struct media_engine *engine = impl_from_IMFMediaEngineEx(iface);
    HRESULT hr;

    TRACE("%p, %p.\n", iface, range);

    if (FAILED(hr = create_time_range(range)))
        return hr;

    EnterCriticalSection(&engine->cs);

    if (engine->flags & FLAGS_ENGINE_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else if (!isnan(engine->duration))
        hr = IMFMediaTimeRange_AddRange(*range, 0.0, engine->duration);

    LeaveCriticalSection(&engine->cs);

    return hr;
}

static HRESULT WINAPI media_engine_Load(IMFMediaEngineEx *iface)
{
    struct media_engine *engine = impl_from_IMFMediaEngineEx(iface);
    HRESULT hr = E_NOTIMPL;

    FIXME("(%p): stub.\n", iface);

    EnterCriticalSection(&engine->cs);

    if (engine->flags & FLAGS_ENGINE_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;

    LeaveCriticalSection(&engine->cs);

    return hr;
}

static HRESULT WINAPI media_engine_CanPlayType(IMFMediaEngineEx *iface, BSTR mime_type, MF_MEDIA_ENGINE_CANPLAY *answer)
{
    struct media_engine *engine = impl_from_IMFMediaEngineEx(iface);
    HRESULT hr = E_NOTIMPL;

    TRACE("%p, %s, %p.\n", iface, debugstr_w(mime_type), answer);

    EnterCriticalSection(&engine->cs);

    if (engine->flags & FLAGS_ENGINE_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else
    {
        FIXME("Check builtin supported types.\n");

        if (engine->extension)
             hr = IMFMediaEngineExtension_CanPlayType(engine->extension, !!(engine->flags & MF_MEDIA_ENGINE_AUDIOONLY),
                     mime_type, answer);
    }

    LeaveCriticalSection(&engine->cs);

    return hr;
}

static USHORT WINAPI media_engine_GetReadyState(IMFMediaEngineEx *iface)
{
    struct media_engine *engine = impl_from_IMFMediaEngineEx(iface);
    unsigned short state;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&engine->cs);
    state = engine->ready_state;
    LeaveCriticalSection(&engine->cs);

    return state;
}

static BOOL WINAPI media_engine_IsSeeking(IMFMediaEngineEx *iface)
{
    FIXME("(%p): stub.\n", iface);

    return FALSE;
}

static double WINAPI media_engine_GetCurrentTime(IMFMediaEngineEx *iface)
{
    struct media_engine *engine = impl_from_IMFMediaEngineEx(iface);
    double ret = 0.0;
    MFTIME clocktime;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&engine->cs);
    if (engine->flags & FLAGS_ENGINE_IS_ENDED)
    {
        ret = engine->duration;
    }
    else if (engine->flags & FLAGS_ENGINE_PAUSED && engine->presentation.start_position.vt == VT_I8)
    {
        ret = (double)engine->presentation.start_position.hVal.QuadPart / 10000000;
    }
    else if (SUCCEEDED(IMFPresentationClock_GetTime(engine->clock, &clocktime)))
    {
        ret = mftime_to_seconds(clocktime);
    }
    LeaveCriticalSection(&engine->cs);

    return ret;
}

static HRESULT media_engine_set_current_time(struct media_engine *engine, double seektime)
{
    PROPVARIANT position;
    DWORD caps;
    HRESULT hr;

    hr = IMFMediaSession_GetSessionCapabilities(engine->session, &caps);
    if (FAILED(hr) || !(caps & MFSESSIONCAP_SEEK))
        return hr;

    if (engine->flags & FLAGS_ENGINE_SEEKING)
    {
        engine->next_seek = seektime;
        return S_OK;
    }

    engine->next_seek = NAN;

    position.vt = VT_I8;
    position.hVal.QuadPart = min(max(0, seektime), engine->duration) * 10000000;

    if (IMFMediaEngineEx_IsPaused(&engine->IMFMediaEngineEx_iface))
    {
        engine->presentation.start_position = position;
        IMFMediaEngineNotify_EventNotify(engine->callback, MF_MEDIA_ENGINE_EVENT_SEEKING, 0, 0);
        IMFMediaEngineNotify_EventNotify(engine->callback, MF_MEDIA_ENGINE_EVENT_SEEKED, 0, 0);
        IMFMediaEngineNotify_EventNotify(engine->callback, MF_MEDIA_ENGINE_EVENT_TIMEUPDATE, 0, 0);
        return S_OK;
    }

    if (SUCCEEDED(hr = IMFMediaSession_Start(engine->session, &GUID_NULL, &position)))
    {
        media_engine_set_flag(engine, FLAGS_ENGINE_SEEKING, TRUE);
        IMFMediaEngineNotify_EventNotify(engine->callback, MF_MEDIA_ENGINE_EVENT_SEEKING, 0, 0);
    }

    return hr;
}

static HRESULT WINAPI media_engine_SetCurrentTime(IMFMediaEngineEx *iface, double time)
{
    struct media_engine *engine = impl_from_IMFMediaEngineEx(iface);
    HRESULT hr;

    TRACE("%p, %f.\n", iface, time);

    EnterCriticalSection(&engine->cs);

    if (engine->flags & FLAGS_ENGINE_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else
        hr = media_engine_set_current_time(engine, time);

    LeaveCriticalSection(&engine->cs);

    return hr;
}

static double WINAPI media_engine_GetStartTime(IMFMediaEngineEx *iface)
{
    FIXME("(%p): stub.\n", iface);

    return 0.0;
}

static double WINAPI media_engine_GetDuration(IMFMediaEngineEx *iface)
{
    struct media_engine *engine = impl_from_IMFMediaEngineEx(iface);
    double value;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&engine->cs);
    value = engine->duration;
    LeaveCriticalSection(&engine->cs);

    return value;
}

static BOOL WINAPI media_engine_IsPaused(IMFMediaEngineEx *iface)
{
    struct media_engine *engine = impl_from_IMFMediaEngineEx(iface);
    BOOL value;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&engine->cs);
    value = !!(engine->flags & FLAGS_ENGINE_PAUSED);
    LeaveCriticalSection(&engine->cs);

    return value;
}

static double WINAPI media_engine_GetDefaultPlaybackRate(IMFMediaEngineEx *iface)
{
    struct media_engine *engine = impl_from_IMFMediaEngineEx(iface);
    double rate;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&engine->cs);
    rate = engine->default_playback_rate;
    LeaveCriticalSection(&engine->cs);

    return rate;
}

static HRESULT WINAPI media_engine_SetDefaultPlaybackRate(IMFMediaEngineEx *iface, double rate)
{
    struct media_engine *engine = impl_from_IMFMediaEngineEx(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %f.\n", iface, rate);

    EnterCriticalSection(&engine->cs);
    if (engine->flags & FLAGS_ENGINE_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else if (engine->default_playback_rate != rate)
    {
        engine->default_playback_rate = rate;
        IMFMediaEngineNotify_EventNotify(engine->callback, MF_MEDIA_ENGINE_EVENT_RATECHANGE, 0, 0);
    }
    LeaveCriticalSection(&engine->cs);

    return hr;
}

static double WINAPI media_engine_GetPlaybackRate(IMFMediaEngineEx *iface)
{
    struct media_engine *engine = impl_from_IMFMediaEngineEx(iface);
    double rate;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&engine->cs);
    rate = engine->playback_rate;
    LeaveCriticalSection(&engine->cs);

    return rate;
}

static HRESULT WINAPI media_engine_SetPlaybackRate(IMFMediaEngineEx *iface, double rate)
{
    struct media_engine *engine = impl_from_IMFMediaEngineEx(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %f.\n", iface, rate);

    EnterCriticalSection(&engine->cs);
    if (engine->flags & FLAGS_ENGINE_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else if (engine->playback_rate != rate)
    {
        engine->playback_rate = rate;
        IMFMediaEngineNotify_EventNotify(engine->callback, MF_MEDIA_ENGINE_EVENT_RATECHANGE, 0, 0);
    }
    LeaveCriticalSection(&engine->cs);

    return hr;
}

static HRESULT WINAPI media_engine_GetPlayed(IMFMediaEngineEx *iface, IMFMediaTimeRange **played)
{
    struct media_engine *engine = impl_from_IMFMediaEngineEx(iface);
    HRESULT hr = E_NOTIMPL;

    FIXME("(%p, %p): stub.\n", iface, played);

    EnterCriticalSection(&engine->cs);

    if (engine->flags & FLAGS_ENGINE_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;

    LeaveCriticalSection(&engine->cs);

    return hr;
}

static HRESULT WINAPI media_engine_GetSeekable(IMFMediaEngineEx *iface, IMFMediaTimeRange **seekable)
{
    struct media_engine *engine = impl_from_IMFMediaEngineEx(iface);
    IMFMediaTimeRange *time_range = NULL;
    DWORD flags;
    HRESULT hr;

    TRACE("%p, %p.\n", iface, seekable);

    EnterCriticalSection(&engine->cs);

    if (engine->flags & FLAGS_ENGINE_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else
    {
        hr = create_time_range(&time_range);
        if (SUCCEEDED(hr) && !isnan(engine->duration) && engine->presentation.source)
        {
            hr = IMFMediaSource_GetCharacteristics(engine->presentation.source, &flags);
            if (SUCCEEDED(hr) && (flags & MFBYTESTREAM_IS_SEEKABLE))
                hr = IMFMediaTimeRange_AddRange(time_range, 0.0, engine->duration);
        }
    }

    LeaveCriticalSection(&engine->cs);

    if (FAILED(hr) && time_range)
    {
        IMFMediaTimeRange_Release(time_range);
        time_range = NULL;
    }
    *seekable = time_range;
    return hr;
}

static BOOL WINAPI media_engine_IsEnded(IMFMediaEngineEx *iface)
{
    struct media_engine *engine = impl_from_IMFMediaEngineEx(iface);
    BOOL value;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&engine->cs);
    value = !!(engine->flags & FLAGS_ENGINE_IS_ENDED);
    LeaveCriticalSection(&engine->cs);

    return value;
}

static BOOL WINAPI media_engine_GetAutoPlay(IMFMediaEngineEx *iface)
{
    struct media_engine *engine = impl_from_IMFMediaEngineEx(iface);
    BOOL value;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&engine->cs);
    value = !!(engine->flags & FLAGS_ENGINE_AUTO_PLAY);
    LeaveCriticalSection(&engine->cs);

    return value;
}

static HRESULT WINAPI media_engine_SetAutoPlay(IMFMediaEngineEx *iface, BOOL autoplay)
{
    struct media_engine *engine = impl_from_IMFMediaEngineEx(iface);

    FIXME("(%p, %d): stub.\n", iface, autoplay);

    EnterCriticalSection(&engine->cs);
    media_engine_set_flag(engine, FLAGS_ENGINE_AUTO_PLAY, autoplay);
    LeaveCriticalSection(&engine->cs);

    return S_OK;
}

static BOOL WINAPI media_engine_GetLoop(IMFMediaEngineEx *iface)
{
    struct media_engine *engine = impl_from_IMFMediaEngineEx(iface);
    BOOL value;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&engine->cs);
    value = !!(engine->flags & FLAGS_ENGINE_LOOP);
    LeaveCriticalSection(&engine->cs);

    return value;
}

static HRESULT WINAPI media_engine_SetLoop(IMFMediaEngineEx *iface, BOOL loop)
{
    struct media_engine *engine = impl_from_IMFMediaEngineEx(iface);

    FIXME("(%p, %d): stub.\n", iface, loop);

    EnterCriticalSection(&engine->cs);
    media_engine_set_flag(engine, FLAGS_ENGINE_LOOP, loop);
    LeaveCriticalSection(&engine->cs);

    return S_OK;
}

static HRESULT WINAPI media_engine_Play(IMFMediaEngineEx *iface)
{
    struct media_engine *engine = impl_from_IMFMediaEngineEx(iface);
    HRESULT hr = S_OK;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&engine->cs);

    if (engine->flags & FLAGS_ENGINE_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else
    {
        IMFMediaEngineNotify_EventNotify(engine->callback, MF_MEDIA_ENGINE_EVENT_PURGEQUEUEDEVENTS, 0, 0);

        if (!(engine->flags & FLAGS_ENGINE_WAITING))
        {
            media_engine_set_flag(engine, FLAGS_ENGINE_PAUSED | FLAGS_ENGINE_IS_ENDED, FALSE);
            IMFMediaEngineNotify_EventNotify(engine->callback, MF_MEDIA_ENGINE_EVENT_PLAY, 0, 0);

            if (!(engine->flags & FLAGS_ENGINE_SOURCE_PENDING))
                media_engine_start_playback(engine);
            else
                media_engine_set_flag(engine, FLAGS_ENGINE_PLAY_PENDING, TRUE);

            media_engine_set_flag(engine, FLAGS_ENGINE_WAITING, TRUE);
        }

        IMFMediaEngineNotify_EventNotify(engine->callback, MF_MEDIA_ENGINE_EVENT_WAITING, 0, 0);
    }

    LeaveCriticalSection(&engine->cs);

    return hr;
}

static HRESULT WINAPI media_engine_Pause(IMFMediaEngineEx *iface)
{
    struct media_engine *engine = impl_from_IMFMediaEngineEx(iface);
    HRESULT hr = S_OK;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&engine->cs);

    if (engine->flags & FLAGS_ENGINE_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else
    {
        if (!(engine->flags & FLAGS_ENGINE_PAUSED))
        {
            if (SUCCEEDED(hr = IMFMediaSession_Pause(engine->session)))
            {
                media_engine_set_flag(engine, FLAGS_ENGINE_WAITING | FLAGS_ENGINE_IS_ENDED, FALSE);
                media_engine_set_flag(engine, FLAGS_ENGINE_PAUSED, TRUE);

                IMFMediaEngineNotify_EventNotify(engine->callback, MF_MEDIA_ENGINE_EVENT_TIMEUPDATE, 0, 0);
                IMFMediaEngineNotify_EventNotify(engine->callback, MF_MEDIA_ENGINE_EVENT_PAUSE, 0, 0);
            }
        }

        IMFMediaEngineNotify_EventNotify(engine->callback, MF_MEDIA_ENGINE_EVENT_PURGEQUEUEDEVENTS, 0, 0);
    }

    LeaveCriticalSection(&engine->cs);

    return hr;
}

static BOOL WINAPI media_engine_GetMuted(IMFMediaEngineEx *iface)
{
    struct media_engine *engine = impl_from_IMFMediaEngineEx(iface);
    BOOL ret;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&engine->cs);
    ret = !!(engine->flags & FLAGS_ENGINE_MUTED);
    LeaveCriticalSection(&engine->cs);

    return ret;
}

static HRESULT WINAPI media_engine_SetMuted(IMFMediaEngineEx *iface, BOOL muted)
{
    struct media_engine *engine = impl_from_IMFMediaEngineEx(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %d.\n", iface, muted);

    EnterCriticalSection(&engine->cs);
    if (engine->flags & FLAGS_ENGINE_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else if (!!(engine->flags & FLAGS_ENGINE_MUTED) ^ !!muted)
    {
        media_engine_set_flag(engine, FLAGS_ENGINE_MUTED, muted);
        IMFMediaEngineNotify_EventNotify(engine->callback, MF_MEDIA_ENGINE_EVENT_VOLUMECHANGE, 0, 0);
    }
    LeaveCriticalSection(&engine->cs);

    return hr;
}

static double WINAPI media_engine_GetVolume(IMFMediaEngineEx *iface)
{
    struct media_engine *engine = impl_from_IMFMediaEngineEx(iface);
    double volume;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&engine->cs);
    volume = engine->volume;
    LeaveCriticalSection(&engine->cs);

    return volume;
}

static HRESULT WINAPI media_engine_SetVolume(IMFMediaEngineEx *iface, double volume)
{
    struct media_engine *engine = impl_from_IMFMediaEngineEx(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %f.\n", iface, volume);

    EnterCriticalSection(&engine->cs);
    if (engine->flags & FLAGS_ENGINE_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else if (volume != engine->volume)
    {
        engine->volume = volume;
        media_engine_apply_volume(engine);
        IMFMediaEngineNotify_EventNotify(engine->callback, MF_MEDIA_ENGINE_EVENT_VOLUMECHANGE, 0, 0);
    }
    LeaveCriticalSection(&engine->cs);

    return hr;
}

static BOOL WINAPI media_engine_HasVideo(IMFMediaEngineEx *iface)
{
    struct media_engine *engine = impl_from_IMFMediaEngineEx(iface);
    BOOL value;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&engine->cs);
    value = !!(engine->flags & FLAGS_ENGINE_HAS_VIDEO);
    LeaveCriticalSection(&engine->cs);

    return value;
}

static BOOL WINAPI media_engine_HasAudio(IMFMediaEngineEx *iface)
{
    struct media_engine *engine = impl_from_IMFMediaEngineEx(iface);
    BOOL value;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&engine->cs);
    value = !!(engine->flags & FLAGS_ENGINE_HAS_AUDIO);
    LeaveCriticalSection(&engine->cs);

    return value;
}

static HRESULT WINAPI media_engine_GetNativeVideoSize(IMFMediaEngineEx *iface, DWORD *cx, DWORD *cy)
{
    struct media_engine *engine = impl_from_IMFMediaEngineEx(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p, %p.\n", iface, cx, cy);

    if (!cx && !cy)
        return E_INVALIDARG;

    EnterCriticalSection(&engine->cs);

    if (engine->flags & FLAGS_ENGINE_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else if (!engine->video_frame.size.cx && !engine->video_frame.size.cy)
        hr = E_FAIL;
    else
    {
        if (cx) *cx = engine->video_frame.size.cx;
        if (cy) *cy = engine->video_frame.size.cy;
    }

    LeaveCriticalSection(&engine->cs);

    return hr;
}

static HRESULT WINAPI media_engine_GetVideoAspectRatio(IMFMediaEngineEx *iface, DWORD *cx, DWORD *cy)
{
    struct media_engine *engine = impl_from_IMFMediaEngineEx(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p, %p.\n", iface, cx, cy);

    if (!cx && !cy)
        return E_INVALIDARG;

    EnterCriticalSection(&engine->cs);

    if (engine->flags & FLAGS_ENGINE_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else if (!engine->video_frame.size.cx && !engine->video_frame.size.cy)
        hr = E_FAIL;
    else
    {
        if (cx) *cx = engine->video_frame.ratio.cx;
        if (cy) *cy = engine->video_frame.ratio.cy;
    }

    LeaveCriticalSection(&engine->cs);

    return hr;
}

static HRESULT WINAPI media_engine_Shutdown(IMFMediaEngineEx *iface)
{
    struct media_engine *engine = impl_from_IMFMediaEngineEx(iface);
    HRESULT hr = S_OK;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&engine->cs);
    if (engine->flags & FLAGS_ENGINE_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else
    {
        media_engine_set_flag(engine, FLAGS_ENGINE_SHUT_DOWN, TRUE);
        media_engine_clear_presentation(engine);
        /* critical section can not be held during shutdown, as shut down requires all pending
         * callbacks to complete, and some callbacks require this cs */
        LeaveCriticalSection(&engine->cs);
        IMFMediaSession_Shutdown(engine->session);
        EnterCriticalSection(&engine->cs);
    }
    LeaveCriticalSection(&engine->cs);

    return hr;
}

static void set_rect(struct rect *rect, float left, float top, float right, float bottom)
{
    rect->left = left;
    rect->top = top;
    rect->right = right;
    rect->bottom = bottom;
}

static void media_engine_adjust_destination_for_ratio(const struct media_engine *engine,
        struct rect *src_n, struct rect *dst)
{
    float dst_width = dst->right - dst->left, dst_height = dst->bottom - dst->top;
    D3D11_TEXTURE2D_DESC source_desc;
    float src_width, src_height;
    struct rect src;

    ID3D11Texture2D_GetDesc(engine->video_frame.d3d11.source, &source_desc);
    set_rect(&src, src_n->left * source_desc.Width, src_n->top * source_desc.Height,
            src_n->right * source_desc.Width, src_n->bottom * source_desc.Height);

    src_width = src.right - src.left;
    src_height = src.bottom - src.top;

    if (src_width * dst_height > dst_width * src_height)
    {
        /* src is "wider" than dst. */
        float dst_center = (dst->top + dst->bottom) / 2.0f;
        float scaled_height = src_height * dst_width / src_width;

        dst->top = dst_center - scaled_height / 2.0f;
        dst->bottom = dst->top + scaled_height;
    }
    else if (src_width * dst_height < dst_width * src_height)
    {
        /* src is "taller" than dst. */
        float dst_center = (dst->left + dst->right) / 2.0f;
        float scaled_width = src_width * dst_height / src_height;

        dst->left = dst_center - scaled_width / 2.0f;
        dst->right = dst->left + scaled_width;
    }
}

static void media_engine_update_d3d11_frame_surface(ID3D11DeviceContext *context, struct media_engine *engine)
{
    D3D11_TEXTURE2D_DESC surface_desc;
    IMFMediaBuffer *media_buffer;
    IMFSample *sample;

    if (!video_frame_sink_get_sample(engine->presentation.frame_sink, &sample))
        return;

    ID3D11Texture2D_GetDesc(engine->video_frame.d3d11.source, &surface_desc);

    switch (surface_desc.Format)
    {
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8X8_UNORM:
        surface_desc.Width *= 4;
        break;
    default:
        FIXME("Unsupported format %#x.\n", surface_desc.Format);
        surface_desc.Width = 0;
    }

    if (SUCCEEDED(IMFSample_ConvertToContiguousBuffer(sample, &media_buffer)))
    {
        BYTE *buffer;
        DWORD buffer_size;
        if (SUCCEEDED(IMFMediaBuffer_Lock(media_buffer, &buffer, NULL, &buffer_size)))
        {
            if (buffer_size == surface_desc.Width * surface_desc.Height)
            {
                ID3D11DeviceContext_UpdateSubresource(context, (ID3D11Resource *)engine->video_frame.d3d11.source,
                        0, NULL, buffer, surface_desc.Width, 0);
            }

            IMFMediaBuffer_Unlock(media_buffer);
        }
        IMFMediaBuffer_Release(media_buffer);
    }

    IMFSample_Release(sample);
}

static HRESULT get_d3d11_resource_from_sample(IMFSample *sample, ID3D11Texture2D **resource, UINT *subresource)
{
    IMFDXGIBuffer *dxgi_buffer;
    IMFMediaBuffer *buffer;
    HRESULT hr;

    *resource = NULL;
    *subresource = 0;

    if (FAILED(hr = IMFSample_GetBufferByIndex(sample, 0, &buffer)))
        return hr;

    if (SUCCEEDED(hr = IMFMediaBuffer_QueryInterface(buffer, &IID_IMFDXGIBuffer, (void **)&dxgi_buffer)))
    {
        IMFDXGIBuffer_GetSubresourceIndex(dxgi_buffer, subresource);
        hr = IMFDXGIBuffer_GetResource(dxgi_buffer, &IID_ID3D11Texture2D, (void **)resource);
        IMFDXGIBuffer_Release(dxgi_buffer);
    }

    IMFMediaBuffer_Release(buffer);
    return hr;
}

static HRESULT media_engine_transfer_d3d11(struct media_engine *engine, ID3D11Texture2D *dst_texture,
        const MFVideoNormalizedRect *src_rect, const RECT *dst_rect, const MFARGB *color)
{
    MFVideoNormalizedRect src_rect_default = {0.0, 0.0, 1.0, 1.0};
    MFARGB color_default = {0, 0, 0, 0};
    D3D11_TEXTURE2D_DESC src_desc, dst_desc;
    ID3D11DeviceContext *context;
    ID3D11Texture2D *src_texture;
    RECT dst_rect_default = {0};
    D3D11_BOX src_box = {0};
    ID3D11Device *device;
    IMFSample *sample;
    UINT subresource;
    HRESULT hr;

    if (!src_rect)
        src_rect = &src_rect_default;
    if (!dst_rect)
        dst_rect = &dst_rect_default;
    if (!color)
        color = &color_default;

    if (!video_frame_sink_get_sample(engine->presentation.frame_sink, &sample))
        return MF_E_UNEXPECTED;
    hr = get_d3d11_resource_from_sample(sample, &src_texture, &subresource);
    IMFSample_Release(sample);
    if (FAILED(hr))
        return hr;

    ID3D11Texture2D_GetDesc(src_texture, &src_desc);
    ID3D11Texture2D_GetDesc(dst_texture, &dst_desc);

    src_box.left = src_rect->left * src_desc.Width;
    src_box.top = src_rect->top * src_desc.Height;
    src_box.front = 0;
    src_box.right = src_rect->right * src_desc.Width;
    src_box.bottom = src_rect->bottom * src_desc.Height;
    src_box.back = 1;

    if (dst_rect->left + src_box.right - src_box.left > dst_desc.Width ||
            dst_rect->top + src_box.bottom - src_box.top > dst_desc.Height)
    {
        ID3D11Texture2D_Release(src_texture);
        return MF_E_UNEXPECTED;
    }

    if (FAILED(hr = media_engine_lock_d3d_device(engine, &device)))
    {
        ID3D11Texture2D_Release(src_texture);
        return hr;
    }

    ID3D11Device_GetImmediateContext(device, &context);
    ID3D11DeviceContext_CopySubresourceRegion(context, (ID3D11Resource *)dst_texture, 0,
            dst_rect->left, dst_rect->top, 0, (ID3D11Resource *)src_texture, subresource, &src_box);
    ID3D11DeviceContext_Release(context);

    media_engine_unlock_d3d_device(engine, device);
    ID3D11Texture2D_Release(src_texture);
    return hr;
}

static HRESULT media_engine_transfer_to_d3d11_texture(struct media_engine *engine, ID3D11Texture2D *texture,
        const MFVideoNormalizedRect *src_rect, const RECT *dst_rect, const MFARGB *color)
{
    static const float black[] = {0.0f, 0.0f, 0.0f, 0.0f};
    ID3D11Device *device, *dst_device;
    ID3D11DeviceContext *context;
    ID3D11RenderTargetView *rtv;
    unsigned int stride, offset;
    D3D11_TEXTURE2D_DESC desc;
    BOOL device_mismatch;
    struct vec3 quad[4];
    D3D11_VIEWPORT vp;
    struct rect src, dst;
    struct color backcolor;
    HRESULT hr;
    RECT rect;

    if (FAILED(hr = media_engine_lock_d3d_device(engine, &device)))
        return hr;

    if (FAILED(hr = media_engine_create_d3d11_video_frame_resources(engine, device)))
    {
        WARN("Failed to create d3d resources, hr %#lx.\n", hr);
        goto done;
    }

    ID3D11Texture2D_GetDevice(texture, &dst_device);
    device_mismatch = device != dst_device;
    ID3D11Device_Release(dst_device);

    if (device_mismatch)
    {
        WARN("Destination target from different device.\n");
        hr = E_UNEXPECTED;
        goto done;
    }

    ID3D11Texture2D_GetDesc(texture, &desc);

    if (FAILED(hr = ID3D11Device_CreateRenderTargetView(device, (ID3D11Resource *)texture, NULL, &rtv)))
    {
        WARN("Failed to create an rtv, hr %#lx.\n", hr);
        goto done;
    }

    ID3D11Device_GetImmediateContext(device, &context);

    /* Whole destination is cleared, regardless of specified rectangle. */
    ID3D11DeviceContext_ClearRenderTargetView(context, rtv, black);

    if (dst_rect)
    {
        rect.left = max(0, dst_rect->left);
        rect.top = max(0, dst_rect->top);
        rect.right = min(desc.Width, dst_rect->right);
        rect.bottom = min(desc.Height, dst_rect->bottom);

        quad[0].x = 2.0f * rect.left / desc.Width - 1.0f;
        quad[0].y = -2.0f * rect.bottom / desc.Height + 1.0f;
        quad[0].z = 0.0f;

        quad[1].x = quad[0].x;
        quad[1].y = -2.0f * rect.top / desc.Height + 1.0f;
        quad[1].z = 0.0f;

        quad[2].x = 2.0f * rect.right / desc.Width - 1.0f;
        quad[2].y = quad[0].y;
        quad[2].z = 0.0f;

        quad[3].x = quad[2].x;
        quad[3].y = quad[1].y;
        quad[3].z = 0.0f;

        set_rect(&dst, dst_rect->left, dst_rect->top, dst_rect->right, dst_rect->bottom);
    }
    else
    {
        memcpy(quad, fullquad, sizeof(quad));
        set_rect(&dst, 0.0f, 0.0f, desc.Width, desc.Height);
    }

    if (src_rect)
        memcpy(&src, src_rect, sizeof(src));
    else
        set_rect(&src, 0.0f, 0.0f, 1.0f, 1.0f);

    media_engine_adjust_destination_for_ratio(engine, &src, &dst);

    if (memcmp(quad, engine->video_frame.d3d11.quad, sizeof(quad)))
    {
        memcpy(engine->video_frame.d3d11.quad, quad, sizeof(quad));
        ID3D11DeviceContext_UpdateSubresource(context, (ID3D11Resource *)engine->video_frame.d3d11.vb, 0, NULL, quad, 0, 0);
    }

    if (color)
    {
        backcolor.r = color->rgbRed / 255.0f;
        backcolor.g = color->rgbGreen / 255.0f;
        backcolor.b = color->rgbBlue / 255.0f;
        backcolor.a = color->rgbAlpha / 255.0f;
    }
    else
        memcpy(&backcolor, black, sizeof(backcolor));

    if (memcmp(&dst, &engine->video_frame.d3d11.cb.dst, sizeof(dst)) ||
            memcmp(&src, &engine->video_frame.d3d11.cb.src, sizeof(src)) ||
            memcmp(&backcolor, &engine->video_frame.d3d11.cb.backcolor, sizeof(backcolor)))
    {
        memcpy(&engine->video_frame.d3d11.cb.dst, &dst, sizeof(dst));
        memcpy(&engine->video_frame.d3d11.cb.src, &src, sizeof(src));
        memcpy(&engine->video_frame.d3d11.cb.backcolor, &backcolor, sizeof(backcolor));

        ID3D11DeviceContext_UpdateSubresource(context, (ID3D11Resource *)engine->video_frame.d3d11.ps_cb, 0, NULL,
                &engine->video_frame.d3d11.cb, 0, 0);
    }

    /* Update with new frame contents */
    media_engine_update_d3d11_frame_surface(context, engine);

    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;
    vp.Width = desc.Width;
    vp.Height = desc.Height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    ID3D11DeviceContext_RSSetViewports(context, 1, &vp);

    ID3D11DeviceContext_IASetInputLayout(context, engine->video_frame.d3d11.input_layout);
    ID3D11DeviceContext_IASetPrimitiveTopology(context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    stride = sizeof(*quad);
    offset = 0;
    ID3D11DeviceContext_IASetVertexBuffers(context, 0, 1, &engine->video_frame.d3d11.vb, &stride, &offset);
    ID3D11DeviceContext_VSSetShader(context, engine->video_frame.d3d11.vs, NULL, 0);
    ID3D11DeviceContext_PSSetShader(context, engine->video_frame.d3d11.ps, NULL, 0);
    ID3D11DeviceContext_PSSetShaderResources(context, 0, 1, &engine->video_frame.d3d11.srv);
    ID3D11DeviceContext_PSSetConstantBuffers(context, 0, 1, &engine->video_frame.d3d11.ps_cb);
    ID3D11DeviceContext_PSSetSamplers(context, 0, 1, &engine->video_frame.d3d11.sampler);
    ID3D11DeviceContext_OMSetRenderTargets(context, 1, &rtv, NULL);

    ID3D11DeviceContext_Draw(context, 4, 0);

    ID3D11RenderTargetView_Release(rtv);
    ID3D11DeviceContext_Release(context);

done:
    media_engine_unlock_d3d_device(engine, device);

    return hr;
}

static HRESULT media_engine_transfer_wic(struct media_engine *engine, IWICBitmap *bitmap,
        const MFVideoNormalizedRect *src_mf_rect, const RECT *dst_rect, const MFARGB *color)
{
    UINT frame_width, frame_height, dst_width, dst_height, dst_size, src_stride, dst_stride, format_size;
    RECT src_rect = {0}, dst_rect_default = {0};
    DWORD max_length, current_length;
    IMFMediaBuffer *media_buffer;
    IWICBitmapLock *lock = NULL;
    WICPixelFormatGUID format;
    IMFSample *sample;
    WICRect wic_rect;
    BYTE *dst, *src;
    HRESULT hr;

    frame_width = engine->video_frame.size.cx;
    frame_height = engine->video_frame.size.cy;

    if (src_mf_rect)
    {
        src_rect.left = src_mf_rect->left * frame_width + 0.5f;
        src_rect.top = src_mf_rect->top * frame_height + 0.5f;
        src_rect.right = src_mf_rect->right * frame_width + 0.5f;
        src_rect.bottom = src_mf_rect->bottom * frame_height + 0.5f;
    }
    else
    {
        src_rect.right = frame_width;
        src_rect.bottom = frame_height;
    }

    if (FAILED(hr = IWICBitmap_GetPixelFormat(bitmap, &format))
            || FAILED(hr = IWICBitmap_GetSize(bitmap, &dst_width, &dst_height)))
        return hr;

    if (!dst_rect)
    {
        dst_rect = &dst_rect_default;
        dst_rect_default.right = dst_width;
        dst_rect_default.bottom = dst_height;
    }

    if (!video_frame_sink_get_sample(engine->presentation.frame_sink, &sample))
        return MF_E_UNEXPECTED;
    hr = IMFSample_ConvertToContiguousBuffer(sample, &media_buffer);
    IMFSample_Release(sample);
    if (FAILED(hr))
        return hr;

    if (dst_rect->left + src_rect.right - src_rect.left > dst_width
            || dst_rect->top + src_rect.bottom - src_rect.top > dst_height)
    {
        hr = MF_E_UNEXPECTED;
        goto done;
    }
    if (dst_rect->right - dst_rect->left != src_rect.right - src_rect.left
            || dst_rect->bottom - dst_rect->top != src_rect.bottom - src_rect.top)
    {
        FIXME("Scaling/letterboxing is not implemented.\n");
        goto done;
    }

    if (!IsEqualGUID(&format, &GUID_WICPixelFormat32bppBGR) && !IsEqualGUID(&format, &GUID_WICPixelFormat32bppBGRA))
    {
        FIXME("Unsupported format %s.\n", wine_dbgstr_guid(&format));
        goto done;
    }
    if (engine->video_frame.output_format != DXGI_FORMAT_B8G8R8A8_UNORM
            && engine->video_frame.output_format != DXGI_FORMAT_B8G8R8X8_UNORM)
    {
        FIXME("Unsupported format %#x.\n", engine->video_frame.output_format);
        goto done;
    }
    if (engine->video_frame.output_format == DXGI_FORMAT_B8G8R8A8_UNORM
            && IsEqualGUID(&format, &GUID_WICPixelFormat32bppBGR))
    {
        WARN("Dropping alpha channel.\n");
    }
    format_size = 4;

    wic_rect.X = dst_rect->left;
    wic_rect.Y = dst_rect->top;
    wic_rect.Width = dst_rect->right - dst_rect->left;
    wic_rect.Height = dst_rect->bottom - dst_rect->top;
    if (FAILED(hr = IWICBitmap_Lock(bitmap, &wic_rect, WICBitmapLockWrite, &lock)))
        goto done;

    if (FAILED(hr = IWICBitmapLock_GetStride(lock, &dst_stride))
            || FAILED(hr = IWICBitmapLock_GetDataPointer(lock, &dst_size, &dst)))
        goto done;

    if (FAILED(hr = IMFMediaBuffer_Lock(media_buffer, &src, &max_length, &current_length)))
        goto done;

    if (current_length < frame_width * frame_height * format_size)
    {
        WARN("Unexpected source length %lu.\n", current_length);
        hr = MF_E_UNEXPECTED;
        goto done;
    }

    src_stride = frame_width * format_size;
    src += src_rect.top * src_stride + src_rect.left * format_size;
    MFCopyImage(dst, dst_stride, src, src_stride, dst_stride, wic_rect.Height);

    IMFMediaBuffer_Unlock(media_buffer);

done:
    if (lock)
        IWICBitmapLock_Release(lock);
    IMFMediaBuffer_Release(media_buffer);

    return hr;
}

static HRESULT WINAPI media_engine_TransferVideoFrame(IMFMediaEngineEx *iface, IUnknown *surface,
        const MFVideoNormalizedRect *src_rect, const RECT *dst_rect, const MFARGB *color)
{
    struct media_engine *engine = impl_from_IMFMediaEngineEx(iface);
    ID3D11Texture2D *texture;
    HRESULT hr = E_NOINTERFACE;
    IWICBitmap *bitmap;

    TRACE("%p, %p, %s, %s, %p.\n", iface, surface, src_rect ? wine_dbg_sprintf("(%f,%f)-(%f,%f)",
            src_rect->left, src_rect->top, src_rect->right, src_rect->bottom) : "(null)",
            wine_dbgstr_rect(dst_rect), color);

    EnterCriticalSection(&engine->cs);

    if (SUCCEEDED(IUnknown_QueryInterface(surface, &IID_ID3D11Texture2D, (void **)&texture)))
    {
        if (!engine->device_manager || FAILED(hr = media_engine_transfer_d3d11(engine, texture, src_rect, dst_rect, color)))
            hr = media_engine_transfer_to_d3d11_texture(engine, texture, src_rect, dst_rect, color);
        ID3D11Texture2D_Release(texture);
    }
    /* Windows does not allow transfer to IWICBitmap if a device manager was set. */
    else if (!engine->device_manager && SUCCEEDED(IUnknown_QueryInterface(surface, &IID_IWICBitmap, (void **)&bitmap)))
    {
        hr = media_engine_transfer_wic(engine, bitmap, src_rect, dst_rect, color);
        IWICBitmap_Release(bitmap);
    }
    else
    {
        FIXME("Unsupported destination type.\n");
    }

    LeaveCriticalSection(&engine->cs);

    return hr;
}

static HRESULT WINAPI media_engine_OnVideoStreamTick(IMFMediaEngineEx *iface, LONGLONG *pts)
{
    struct media_engine *engine = impl_from_IMFMediaEngineEx(iface);
    HRESULT hr;

    TRACE("%p, %p.\n", iface, pts);

    EnterCriticalSection(&engine->cs);

    if (engine->flags & FLAGS_ENGINE_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else if (!pts)
        hr = E_POINTER;
    else
    {
        MFTIME clocktime;
        IMFPresentationClock_GetTime(engine->clock, &clocktime);
        hr = video_frame_sink_get_pts(engine->presentation.frame_sink, clocktime, pts);
    }

    LeaveCriticalSection(&engine->cs);

    return hr;
}

static HRESULT WINAPI media_engine_SetSourceFromByteStream(IMFMediaEngineEx *iface, IMFByteStream *bytestream, BSTR url)
{
    struct media_engine *engine = impl_from_IMFMediaEngineEx(iface);
    HRESULT hr;

    TRACE("%p, %p, %s.\n", iface, bytestream, debugstr_w(url));

    EnterCriticalSection(&engine->cs);

    if (engine->flags & FLAGS_ENGINE_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else if (!bytestream || !url)
        hr = E_POINTER;
    else
        hr = media_engine_set_source(engine, bytestream, url);

    LeaveCriticalSection(&engine->cs);

    return hr;
}

static HRESULT WINAPI media_engine_GetStatistics(IMFMediaEngineEx *iface, MF_MEDIA_ENGINE_STATISTIC stat_id, PROPVARIANT *stat)
{
    FIXME("%p, %x, %p stub.\n", iface, stat_id, stat);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_engine_UpdateVideoStream(IMFMediaEngineEx *iface, const MFVideoNormalizedRect *src,
        const RECT *dst, const MFARGB *border_color)
{
    FIXME("%p, %p, %p, %p stub.\n", iface, src, dst, border_color);

    return E_NOTIMPL;
}

static double WINAPI media_engine_GetBalance(IMFMediaEngineEx *iface)
{
    FIXME("%p stub.\n", iface);

    return 0.0;
}

static HRESULT WINAPI media_engine_SetBalance(IMFMediaEngineEx *iface, double balance)
{
    FIXME("%p, %f stub.\n", iface, balance);

    return E_NOTIMPL;
}

static BOOL WINAPI media_engine_IsPlaybackRateSupported(IMFMediaEngineEx *iface, double rate)
{
    FIXME("%p, %f stub.\n", iface, rate);

    return FALSE;
}

static HRESULT WINAPI media_engine_FrameStep(IMFMediaEngineEx *iface, BOOL forward)
{
    FIXME("%p, %d stub.\n", iface, forward);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_engine_GetResourceCharacteristics(IMFMediaEngineEx *iface, DWORD *flags)
{
    struct media_engine *engine = impl_from_IMFMediaEngineEx(iface);
    HRESULT hr = E_FAIL;

    TRACE("%p, %p.\n", iface, flags);

    EnterCriticalSection(&engine->cs);
    if (engine->flags & FLAGS_ENGINE_SHUT_DOWN)
    {
        hr = MF_E_SHUTDOWN;
    }
    else if (engine->presentation.source && flags)
    {
        if (SUCCEEDED(IMFMediaSource_GetCharacteristics(engine->presentation.source, flags)))
        {
            *flags = *flags & 0xf;
            hr = S_OK;
        }
    }
    LeaveCriticalSection(&engine->cs);

    return hr;
}

static HRESULT WINAPI media_engine_GetPresentationAttribute(IMFMediaEngineEx *iface, REFGUID attribute,
        PROPVARIANT *value)
{
    struct media_engine *engine = impl_from_IMFMediaEngineEx(iface);
    HRESULT hr = E_FAIL;

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(attribute), value);

    EnterCriticalSection(&engine->cs);
    if (engine->flags & FLAGS_ENGINE_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else if (engine->presentation.pd)
        hr = IMFPresentationDescriptor_GetItem(engine->presentation.pd, attribute, value);
    LeaveCriticalSection(&engine->cs);

    return hr;
}

static HRESULT WINAPI media_engine_GetNumberOfStreams(IMFMediaEngineEx *iface, DWORD *stream_count)
{
    struct media_engine *engine = impl_from_IMFMediaEngineEx(iface);
    HRESULT hr = E_FAIL;

    TRACE("%p, %p.\n", iface, stream_count);

    EnterCriticalSection(&engine->cs);
    if (engine->flags & FLAGS_ENGINE_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else if (engine->presentation.pd)
        hr = IMFPresentationDescriptor_GetStreamDescriptorCount(engine->presentation.pd, stream_count);
    LeaveCriticalSection(&engine->cs);

    return hr;
}

static HRESULT WINAPI media_engine_GetStreamAttribute(IMFMediaEngineEx *iface, DWORD stream_index, REFGUID attribute,
        PROPVARIANT *value)
{
    struct media_engine *engine = impl_from_IMFMediaEngineEx(iface);
    IMFStreamDescriptor *sd;
    HRESULT hr = E_FAIL;
    BOOL selected;

    TRACE("%p, %ld, %s, %p.\n", iface, stream_index, debugstr_guid(attribute), value);

    EnterCriticalSection(&engine->cs);
    if (engine->flags & FLAGS_ENGINE_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else if (engine->presentation.pd)
    {
        if (SUCCEEDED(hr = IMFPresentationDescriptor_GetStreamDescriptorByIndex(engine->presentation.pd,
                stream_index, &selected, &sd)))
        {
            hr = IMFStreamDescriptor_GetItem(sd, attribute, value);
            IMFStreamDescriptor_Release(sd);
        }
    }
    LeaveCriticalSection(&engine->cs);

    return hr;
}

static HRESULT WINAPI media_engine_GetStreamSelection(IMFMediaEngineEx *iface, DWORD stream_index, BOOL *enabled)
{
    FIXME("%p, %ld, %p stub.\n", iface, stream_index, enabled);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_engine_SetStreamSelection(IMFMediaEngineEx *iface, DWORD stream_index, BOOL enabled)
{
    FIXME("%p, %ld, %d stub.\n", iface, stream_index, enabled);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_engine_ApplyStreamSelections(IMFMediaEngineEx *iface)
{
    FIXME("%p stub.\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_engine_IsProtected(IMFMediaEngineEx *iface, BOOL *protected)
{
    FIXME("%p, %p stub.\n", iface, protected);

    return E_NOTIMPL;
}

static HRESULT media_engine_insert_effect(struct media_engine *engine, struct effects *effects, IUnknown *object, BOOL is_optional)
{
    HRESULT hr = S_OK;

    if (engine->flags & FLAGS_ENGINE_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else if (!mf_array_reserve((void **)&effects->effects, &effects->capacity, effects->count + 1, sizeof(*effects->effects)))
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        effects->effects[effects->count].object = object;
        if (object)
        {
            IUnknown_AddRef(effects->effects[effects->count].object);
        }
        effects->effects[effects->count].optional = is_optional;

        effects->count++;
    }

    return hr;
}

static HRESULT WINAPI media_engine_InsertVideoEffect(IMFMediaEngineEx *iface, IUnknown *effect, BOOL is_optional)
{
    struct media_engine *engine = impl_from_IMFMediaEngineEx(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p, %d.\n", iface, effect, is_optional);

    EnterCriticalSection(&engine->cs);
    hr = media_engine_insert_effect(engine, &engine->video_effects, effect, is_optional);
    LeaveCriticalSection(&engine->cs);

    return hr;
}

static HRESULT WINAPI media_engine_InsertAudioEffect(IMFMediaEngineEx *iface, IUnknown *effect, BOOL is_optional)
{
    struct media_engine *engine = impl_from_IMFMediaEngineEx(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p, %d.\n", iface, effect, is_optional);

    EnterCriticalSection(&engine->cs);
    hr = media_engine_insert_effect(engine, &engine->audio_effects, effect, is_optional);
    LeaveCriticalSection(&engine->cs);

    return hr;
}

static HRESULT WINAPI media_engine_RemoveAllEffects(IMFMediaEngineEx *iface)
{
    struct media_engine *engine = impl_from_IMFMediaEngineEx(iface);
    HRESULT hr = S_OK;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&engine->cs);
    if (engine->flags & FLAGS_ENGINE_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else
    {
        media_engine_clear_effects(&engine->audio_effects);
        media_engine_clear_effects(&engine->video_effects);
    }
    LeaveCriticalSection(&engine->cs);

    return hr;
}

static HRESULT WINAPI media_engine_SetTimelineMarkerTimer(IMFMediaEngineEx *iface, double timeout)
{
    FIXME("%p, %f stub.\n", iface, timeout);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_engine_GetTimelineMarkerTimer(IMFMediaEngineEx *iface, double *timeout)
{
    FIXME("%p, %p stub.\n", iface, timeout);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_engine_CancelTimelineMarkerTimer(IMFMediaEngineEx *iface)
{
    FIXME("%p stub.\n", iface);

    return E_NOTIMPL;
}

static BOOL WINAPI media_engine_IsStereo3D(IMFMediaEngineEx *iface)
{
    FIXME("%p stub.\n", iface);

    return FALSE;
}

static HRESULT WINAPI media_engine_GetStereo3DFramePackingMode(IMFMediaEngineEx *iface, MF_MEDIA_ENGINE_S3D_PACKING_MODE *mode)
{
    FIXME("%p, %p stub.\n", iface, mode);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_engine_SetStereo3DFramePackingMode(IMFMediaEngineEx *iface, MF_MEDIA_ENGINE_S3D_PACKING_MODE mode)
{
    FIXME("%p, %#x stub.\n", iface, mode);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_engine_GetStereo3DRenderMode(IMFMediaEngineEx *iface, MF3DVideoOutputType *output_type)
{
    FIXME("%p, %p stub.\n", iface, output_type);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_engine_SetStereo3DRenderMode(IMFMediaEngineEx *iface, MF3DVideoOutputType output_type)
{
    FIXME("%p, %#x stub.\n", iface, output_type);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_engine_EnableWindowlessSwapchainMode(IMFMediaEngineEx *iface, BOOL enable)
{
    FIXME("%p, %d stub.\n", iface, enable);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_engine_GetVideoSwapchainHandle(IMFMediaEngineEx *iface, HANDLE *swapchain)
{
    FIXME("%p, %p stub.\n", iface, swapchain);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_engine_EnableHorizontalMirrorMode(IMFMediaEngineEx *iface, BOOL enable)
{
    FIXME("%p, %d stub.\n", iface, enable);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_engine_GetAudioStreamCategory(IMFMediaEngineEx *iface, UINT32 *category)
{
    struct media_engine *engine = impl_from_IMFMediaEngineEx(iface);
    HRESULT hr;

    TRACE("%p, %p.\n", iface, category);

    EnterCriticalSection(&engine->cs);

    if (engine->flags & FLAGS_ENGINE_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else
        hr = IMFAttributes_GetUINT32(engine->attributes, &MF_MEDIA_ENGINE_AUDIO_CATEGORY, category);

    LeaveCriticalSection(&engine->cs);

    return hr;
}

static HRESULT WINAPI media_engine_SetAudioStreamCategory(IMFMediaEngineEx *iface, UINT32 category)
{
    struct media_engine *engine = impl_from_IMFMediaEngineEx(iface);
    HRESULT hr;

    TRACE("%p, %u.\n", iface, category);

    EnterCriticalSection(&engine->cs);

    if (engine->flags & FLAGS_ENGINE_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else
        hr = IMFAttributes_SetUINT32(engine->attributes, &MF_MEDIA_ENGINE_AUDIO_CATEGORY, category);

    LeaveCriticalSection(&engine->cs);

    return hr;
}

static HRESULT WINAPI media_engine_GetAudioEndpointRole(IMFMediaEngineEx *iface, UINT32 *role)
{
    struct media_engine *engine = impl_from_IMFMediaEngineEx(iface);
    HRESULT hr;

    TRACE("%p, %p.\n", iface, role);

    EnterCriticalSection(&engine->cs);

    if (engine->flags & FLAGS_ENGINE_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else
        hr = IMFAttributes_GetUINT32(engine->attributes, &MF_MEDIA_ENGINE_AUDIO_ENDPOINT_ROLE, role);

    LeaveCriticalSection(&engine->cs);

    return hr;
}

static HRESULT WINAPI media_engine_SetAudioEndpointRole(IMFMediaEngineEx *iface, UINT32 role)
{
    struct media_engine *engine = impl_from_IMFMediaEngineEx(iface);
    HRESULT hr;

    TRACE("%p, %u.\n", iface, role);

    EnterCriticalSection(&engine->cs);

    if (engine->flags & FLAGS_ENGINE_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else
        hr = IMFAttributes_SetUINT32(engine->attributes, &MF_MEDIA_ENGINE_AUDIO_ENDPOINT_ROLE, role);

    LeaveCriticalSection(&engine->cs);

    return hr;
}

static HRESULT WINAPI media_engine_GetRealTimeMode(IMFMediaEngineEx *iface, BOOL *enabled)
{
    struct media_engine *engine = impl_from_IMFMediaEngineEx(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, enabled);

    EnterCriticalSection(&engine->cs);
    if (engine->flags & FLAGS_ENGINE_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else
        *enabled = !!(engine->flags & MF_MEDIA_ENGINE_REAL_TIME_MODE);
    LeaveCriticalSection(&engine->cs);

    return hr;
}

static HRESULT WINAPI media_engine_SetRealTimeMode(IMFMediaEngineEx *iface, BOOL enable)
{
    struct media_engine *engine = impl_from_IMFMediaEngineEx(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %d.\n", iface, enable);

    EnterCriticalSection(&engine->cs);
    if (engine->flags & FLAGS_ENGINE_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else
        media_engine_set_flag(engine, MF_MEDIA_ENGINE_REAL_TIME_MODE, enable);
    LeaveCriticalSection(&engine->cs);

    return hr;
}

static HRESULT WINAPI media_engine_SetCurrentTimeEx(IMFMediaEngineEx *iface, double seektime, MF_MEDIA_ENGINE_SEEK_MODE mode)
{
    struct media_engine *engine = impl_from_IMFMediaEngineEx(iface);
    HRESULT hr;

    TRACE("%p, %f, %#x.\n", iface, seektime, mode);

    if (mode)
        FIXME("mode %#x is ignored.\n", mode);

    EnterCriticalSection(&engine->cs);

    if (engine->flags & FLAGS_ENGINE_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else
        hr = media_engine_set_current_time(engine, seektime);

    LeaveCriticalSection(&engine->cs);

    return hr;
}

static HRESULT WINAPI media_engine_EnableTimeUpdateTimer(IMFMediaEngineEx *iface, BOOL enable)
{
    FIXME("%p, %d stub.\n", iface, enable);

    return E_NOTIMPL;
}

static const IMFMediaEngineExVtbl media_engine_vtbl =
{
    media_engine_QueryInterface,
    media_engine_AddRef,
    media_engine_Release,
    media_engine_GetError,
    media_engine_SetErrorCode,
    media_engine_SetSourceElements,
    media_engine_SetSource,
    media_engine_GetCurrentSource,
    media_engine_GetNetworkState,
    media_engine_GetPreload,
    media_engine_SetPreload,
    media_engine_GetBuffered,
    media_engine_Load,
    media_engine_CanPlayType,
    media_engine_GetReadyState,
    media_engine_IsSeeking,
    media_engine_GetCurrentTime,
    media_engine_SetCurrentTime,
    media_engine_GetStartTime,
    media_engine_GetDuration,
    media_engine_IsPaused,
    media_engine_GetDefaultPlaybackRate,
    media_engine_SetDefaultPlaybackRate,
    media_engine_GetPlaybackRate,
    media_engine_SetPlaybackRate,
    media_engine_GetPlayed,
    media_engine_GetSeekable,
    media_engine_IsEnded,
    media_engine_GetAutoPlay,
    media_engine_SetAutoPlay,
    media_engine_GetLoop,
    media_engine_SetLoop,
    media_engine_Play,
    media_engine_Pause,
    media_engine_GetMuted,
    media_engine_SetMuted,
    media_engine_GetVolume,
    media_engine_SetVolume,
    media_engine_HasVideo,
    media_engine_HasAudio,
    media_engine_GetNativeVideoSize,
    media_engine_GetVideoAspectRatio,
    media_engine_Shutdown,
    media_engine_TransferVideoFrame,
    media_engine_OnVideoStreamTick,
    media_engine_SetSourceFromByteStream,
    media_engine_GetStatistics,
    media_engine_UpdateVideoStream,
    media_engine_GetBalance,
    media_engine_SetBalance,
    media_engine_IsPlaybackRateSupported,
    media_engine_FrameStep,
    media_engine_GetResourceCharacteristics,
    media_engine_GetPresentationAttribute,
    media_engine_GetNumberOfStreams,
    media_engine_GetStreamAttribute,
    media_engine_GetStreamSelection,
    media_engine_SetStreamSelection,
    media_engine_ApplyStreamSelections,
    media_engine_IsProtected,
    media_engine_InsertVideoEffect,
    media_engine_InsertAudioEffect,
    media_engine_RemoveAllEffects,
    media_engine_SetTimelineMarkerTimer,
    media_engine_GetTimelineMarkerTimer,
    media_engine_CancelTimelineMarkerTimer,
    media_engine_IsStereo3D,
    media_engine_GetStereo3DFramePackingMode,
    media_engine_SetStereo3DFramePackingMode,
    media_engine_GetStereo3DRenderMode,
    media_engine_SetStereo3DRenderMode,
    media_engine_EnableWindowlessSwapchainMode,
    media_engine_GetVideoSwapchainHandle,
    media_engine_EnableHorizontalMirrorMode,
    media_engine_GetAudioStreamCategory,
    media_engine_SetAudioStreamCategory,
    media_engine_GetAudioEndpointRole,
    media_engine_SetAudioEndpointRole,
    media_engine_GetRealTimeMode,
    media_engine_SetRealTimeMode,
    media_engine_SetCurrentTimeEx,
    media_engine_EnableTimeUpdateTimer,
};

static HRESULT WINAPI media_engine_gs_QueryInterface(IMFGetService *iface, REFIID riid, void **obj)
{
    struct media_engine *engine = impl_from_IMFGetService(iface);
    return IMFMediaEngineEx_QueryInterface(&engine->IMFMediaEngineEx_iface, riid, obj);
}

static ULONG WINAPI media_engine_gs_AddRef(IMFGetService *iface)
{
    struct media_engine *engine = impl_from_IMFGetService(iface);
    return IMFMediaEngineEx_AddRef(&engine->IMFMediaEngineEx_iface);
}

static ULONG WINAPI media_engine_gs_Release(IMFGetService *iface)
{
    struct media_engine *engine = impl_from_IMFGetService(iface);
    return IMFMediaEngineEx_Release(&engine->IMFMediaEngineEx_iface);
}

static HRESULT WINAPI media_engine_gs_GetService(IMFGetService *iface, REFGUID service,
        REFIID riid, void **object)
{
    FIXME("%p, %s, %s, %p stub.\n", iface, debugstr_guid(service), debugstr_guid(riid), object);

    return E_NOTIMPL;
}

static const IMFGetServiceVtbl media_engine_get_service_vtbl =
{
    media_engine_gs_QueryInterface,
    media_engine_gs_AddRef,
    media_engine_gs_Release,
    media_engine_gs_GetService,
};

static HRESULT WINAPI media_engine_factory_QueryInterface(IMFMediaEngineClassFactory *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IMFMediaEngineClassFactory) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFMediaEngineClassFactory_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI media_engine_factory_AddRef(IMFMediaEngineClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI media_engine_factory_Release(IMFMediaEngineClassFactory *iface)
{
    return 1;
}

static HRESULT init_media_engine(DWORD flags, IMFAttributes *attributes, struct media_engine *engine)
{
    UINT32 output_format;
    UINT64 playback_hwnd = 0;
    IMFClock *clock;
    HRESULT hr;

    engine->IMFMediaEngineEx_iface.lpVtbl = &media_engine_vtbl;
    engine->IMFGetService_iface.lpVtbl = &media_engine_get_service_vtbl;
    engine->session_events.lpVtbl = &media_engine_session_events_vtbl;
    engine->sink_events.lpVtbl = &media_engine_sink_events_vtbl;
    engine->load_handler.lpVtbl = &media_engine_load_handler_vtbl;
    engine->refcount = 1;
    engine->flags = (flags & MF_MEDIA_ENGINE_CREATEFLAGS_MASK) | FLAGS_ENGINE_PAUSED;
    engine->default_playback_rate = 1.0;
    engine->playback_rate = 1.0;
    engine->volume = 1.0;
    engine->duration = NAN;
    engine->next_seek = NAN;
    engine->video_frame.pts = MINLONGLONG;
    InitializeCriticalSection(&engine->cs);

    if (FAILED(hr = IMFAttributes_GetUnknown(attributes, &MF_MEDIA_ENGINE_CALLBACK, &IID_IMFMediaEngineNotify,
            (void **)&engine->callback)))
    {
        WARN("Notification callback was not provided.\n");
        return hr;
    }

    IMFAttributes_GetUnknown(attributes, &MF_MEDIA_ENGINE_EXTENSION, &IID_IMFMediaEngineExtension,
            (void **)&engine->extension);

    IMFAttributes_GetUnknown(attributes, &MF_MEDIA_ENGINE_DXGI_MANAGER, &IID_IMFDXGIDeviceManager,
            (void **)&engine->device_manager);

    if (FAILED(hr = MFCreateMediaSession(NULL, &engine->session)))
        return hr;

    if (FAILED(hr = IMFMediaSession_GetClock(engine->session, &clock)))
        return hr;

    hr = IMFClock_QueryInterface(clock, &IID_IMFPresentationClock, (void **)&engine->clock);
    IMFClock_Release(clock);
    if (FAILED(hr))
        return hr;

    if (FAILED(hr = IMFMediaSession_BeginGetEvent(engine->session, &engine->session_events, NULL)))
        return hr;

    if (FAILED(hr = MFCreateSourceResolver(&engine->resolver)))
        return hr;

    if (FAILED(hr = MFCreateAttributes(&engine->attributes, 0)))
        return hr;

    if (FAILED(hr = IMFAttributes_CopyAllItems(attributes, engine->attributes)))
        return hr;

    /* Set default audio configuration */
    if (FAILED(IMFAttributes_GetItem(engine->attributes, &MF_MEDIA_ENGINE_AUDIO_CATEGORY, NULL)))
        IMFAttributes_SetUINT32(engine->attributes, &MF_MEDIA_ENGINE_AUDIO_CATEGORY, AudioCategory_Other);
    if (FAILED(IMFAttributes_GetItem(engine->attributes, &MF_MEDIA_ENGINE_AUDIO_ENDPOINT_ROLE, NULL)))
        IMFAttributes_SetUINT32(engine->attributes, &MF_MEDIA_ENGINE_AUDIO_ENDPOINT_ROLE, eMultimedia);

    IMFAttributes_GetUINT64(attributes, &MF_MEDIA_ENGINE_PLAYBACK_HWND, &playback_hwnd);
    hr = IMFAttributes_GetUINT32(attributes, &MF_MEDIA_ENGINE_VIDEO_OUTPUT_FORMAT, &output_format);
    if (playback_hwnd) /* FIXME: handle MF_MEDIA_ENGINE_PLAYBACK_VISUAL */
        engine->mode = MEDIA_ENGINE_RENDERING_MODE;
    else
    {
        if (SUCCEEDED(hr))
            engine->mode = MEDIA_ENGINE_FRAME_SERVER_MODE;
        else
            engine->mode = MEDIA_ENGINE_AUDIO_MODE;
    }

    return S_OK;
}

static HRESULT WINAPI media_engine_factory_CreateInstance(IMFMediaEngineClassFactory *iface, DWORD flags,
                                                          IMFAttributes *attributes, IMFMediaEngine **engine)
{
    struct media_engine *object;
    HRESULT hr;

    TRACE("%p, %#lx, %p, %p.\n", iface, flags, attributes, engine);

    if (!attributes || !engine)
        return E_POINTER;

    object = calloc(1, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    hr = init_media_engine(flags, attributes, object);
    if (FAILED(hr))
    {
        free_media_engine(object);
        return hr;
    }

    *engine = (IMFMediaEngine *)&object->IMFMediaEngineEx_iface;

    return S_OK;
}

static HRESULT WINAPI media_engine_factory_CreateTimeRange(IMFMediaEngineClassFactory *iface,
        IMFMediaTimeRange **range)
{
    TRACE("%p, %p.\n", iface, range);

    return create_time_range(range);
}

static HRESULT WINAPI media_engine_factory_CreateError(IMFMediaEngineClassFactory *iface, IMFMediaError **error)
{
    TRACE("%p, %p.\n", iface, error);

    return create_media_error(error);
}

static const IMFMediaEngineClassFactoryVtbl media_engine_factory_vtbl =
{
    media_engine_factory_QueryInterface,
    media_engine_factory_AddRef,
    media_engine_factory_Release,
    media_engine_factory_CreateInstance,
    media_engine_factory_CreateTimeRange,
    media_engine_factory_CreateError,
};

static IMFMediaEngineClassFactory media_engine_factory = { &media_engine_factory_vtbl };

static HRESULT WINAPI classfactory_QueryInterface(IClassFactory *iface, REFIID riid, void **obj)
{
    TRACE("%s, %p.\n", debugstr_guid(riid), obj);

    if (IsEqualGUID(riid, &IID_IClassFactory) ||
        IsEqualGUID(riid, &IID_IUnknown))
    {
        IClassFactory_AddRef(iface);
        *obj = iface;
        return S_OK;
    }

    WARN("interface %s not implemented.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI classfactory_AddRef(IClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI classfactory_Release(IClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI classfactory_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", outer, debugstr_guid(riid), obj);

    *obj = NULL;

    if (outer)
        return CLASS_E_NOAGGREGATION;

    return IMFMediaEngineClassFactory_QueryInterface(&media_engine_factory, riid, obj);
}

static HRESULT WINAPI classfactory_LockServer(IClassFactory *iface, BOOL dolock)
{
    FIXME("(%d): stub.\n", dolock);
    return S_OK;
}

static const IClassFactoryVtbl class_factory_vtbl =
{
    classfactory_QueryInterface,
    classfactory_AddRef,
    classfactory_Release,
    classfactory_CreateInstance,
    classfactory_LockServer,
};

static IClassFactory classfactory = { &class_factory_vtbl };

HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID riid, void **obj)
{
    TRACE("%s, %s, %p.\n", debugstr_guid(clsid), debugstr_guid(riid), obj);

    if (IsEqualGUID(clsid, &CLSID_MFMediaEngineClassFactory))
        return IClassFactory_QueryInterface(&classfactory, riid, obj);

    WARN("Unsupported class %s.\n", debugstr_guid(clsid));
    *obj = NULL;
    return CLASS_E_CLASSNOTAVAILABLE;
}
