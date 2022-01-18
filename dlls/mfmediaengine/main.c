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
#include "d3d11.h"

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

struct media_engine
{
    IMFMediaEngine IMFMediaEngine_iface;
    IMFAsyncCallback session_events;
    IMFAsyncCallback load_handler;
    IMFSampleGrabberSinkCallback grabber_callback;
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
    MF_MEDIA_ENGINE_NETWORK network_state;
    MF_MEDIA_ENGINE_ERR error_code;
    HRESULT extended_code;
    MF_MEDIA_ENGINE_READY ready_state;
    MF_MEDIA_ENGINE_PRELOAD preload;
    IMFMediaSession *session;
    IMFPresentationClock *clock;
    IMFSourceResolver *resolver;
    BSTR current_source;
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
            WARN("Failed to open device handle, hr %#x.\n", hr);
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
            WARN("Failed to open a device handle, hr %#x.\n", hr);
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
            p.y = 1.0f - (position.y - dst.y) / (dst.w - dst.y);
            p.x = src.x + p.x * (src.z - src.x);
            p.y = src.y + p.y * (src.w - src.y);
            return t.Sample(s, p);
        }
#endif
        0x43425844, 0x5892e3b1, 0x24c17f7c, 0x9999f143, 0x49667872, 0x00000001, 0x0000032c, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000030f, 0x505f5653, 0x5449534f, 0x004e4f49,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x545f5653, 0x45475241, 0xabab0054, 0x52444853, 0x00000290, 0x00000040,
        0x000000a4, 0x04000059, 0x00208e46, 0x00000000, 0x00000003, 0x0300005a, 0x00106000, 0x00000000,
        0x04001858, 0x00107000, 0x00000000, 0x00005555, 0x04002064, 0x00101032, 0x00000000, 0x00000001,
        0x03000065, 0x001020f2, 0x00000000, 0x02000068, 0x00000002, 0x08000031, 0x00100012, 0x00000000,
        0x0010100a, 0x00000000, 0x0020800a, 0x00000000, 0x00000000, 0x08000031, 0x00100022, 0x00000000,
        0x0020802a, 0x00000000, 0x00000000, 0x0010100a, 0x00000000, 0x0700003c, 0x00100012, 0x00000000,
        0x0010001a, 0x00000000, 0x0010000a, 0x00000000, 0x0304001f, 0x0010000a, 0x00000000, 0x06000036,
        0x001020f2, 0x00000000, 0x00208e46, 0x00000000, 0x00000002, 0x0100003e, 0x01000015, 0x08000031,
        0x00100012, 0x00000000, 0x0010101a, 0x00000000, 0x0020801a, 0x00000000, 0x00000000, 0x08000031,
        0x00100022, 0x00000000, 0x0020803a, 0x00000000, 0x00000000, 0x0010101a, 0x00000000, 0x0700003c,
        0x00100012, 0x00000000, 0x0010001a, 0x00000000, 0x0010000a, 0x00000000, 0x0304001f, 0x0010000a,
        0x00000000, 0x06000036, 0x001020f2, 0x00000000, 0x00208e46, 0x00000000, 0x00000002, 0x0100003e,
        0x01000015, 0x09000000, 0x00100032, 0x00000000, 0x00101046, 0x00000000, 0x80208046, 0x00000041,
        0x00000000, 0x00000000, 0x0a000000, 0x001000c2, 0x00000000, 0x80208406, 0x00000041, 0x00000000,
        0x00000000, 0x00208ea6, 0x00000000, 0x00000000, 0x0700000e, 0x00100032, 0x00000000, 0x00100046,
        0x00000000, 0x00100ae6, 0x00000000, 0x08000000, 0x00100022, 0x00000000, 0x8010001a, 0x00000041,
        0x00000000, 0x00004001, 0x3f800000, 0x0a000000, 0x001000c2, 0x00000000, 0x80208406, 0x00000041,
        0x00000000, 0x00000001, 0x00208ea6, 0x00000000, 0x00000001, 0x0a000032, 0x00100012, 0x00000001,
        0x0010000a, 0x00000000, 0x0010002a, 0x00000000, 0x0020800a, 0x00000000, 0x00000001, 0x0a000032,
        0x00100022, 0x00000001, 0x0010001a, 0x00000000, 0x0010003a, 0x00000000, 0x0020801a, 0x00000000,
        0x00000001, 0x09000045, 0x001020f2, 0x00000000, 0x00100046, 0x00000001, 0x00107e46, 0x00000000,
        0x00106000, 0x00000000, 0x0100003e,
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
        WARN("Failed to create a vertex buffer, hr %#x.\n", hr);
        goto failed;
    }

    buffer_desc.ByteWidth = sizeof(engine->video_frame.d3d11.cb);
    buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    if (FAILED(hr = ID3D11Device_CreateBuffer(device, &buffer_desc, NULL, &engine->video_frame.d3d11.ps_cb)))
    {
        WARN("Failed to create a buffer, hr %#x.\n", hr);
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
        WARN("Failed to create source texture, hr %#x.\n", hr);
        goto failed;
    }

    if (FAILED(hr = ID3D11Device_CreateShaderResourceView(device, (ID3D11Resource *)engine->video_frame.d3d11.source,
            NULL, &engine->video_frame.d3d11.srv)))
    {
        WARN("Failed to create SRV, hr %#x.\n", hr);
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
        WARN("Failed to create a sampler state, hr %#x.\n", hr);
        goto failed;
    }

    /* Input layout */
    if (FAILED(hr = ID3D11Device_CreateInputLayout(device, layout_desc, ARRAY_SIZE(layout_desc), vs_code, sizeof(vs_code),
            &engine->video_frame.d3d11.input_layout)))
    {
        WARN("Failed to create input layout, hr %#x.\n", hr);
        goto failed;
    }

    /* Shaders */
    if (FAILED(hr = ID3D11Device_CreateVertexShader(device, vs_code, sizeof(vs_code), NULL, &engine->video_frame.d3d11.vs)))
    {
        WARN("Failed to create the vertex shader, hr %#x.\n", hr);
        goto failed;
    }

    if (FAILED(hr = ID3D11Device_CreatePixelShader(device, ps_code, sizeof(ps_code), NULL, &engine->video_frame.d3d11.ps)))
    {
        WARN("Failed to create the pixel shader, hr %#x.\n", hr);
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

    TRACE("%p, refcount %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI media_error_Release(IMFMediaError *iface)
{
    struct media_error *me = impl_from_IMFMediaError(iface);
    ULONG refcount = InterlockedDecrement(&me->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

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

    TRACE("%p, %#x.\n", iface, code);

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

    TRACE("%p, refcount %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI time_range_Release(IMFMediaTimeRange *iface)
{
    struct time_range *range = impl_from_IMFMediaTimeRange(iface);
    ULONG refcount = InterlockedDecrement(&range->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

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

    TRACE("%p, %u, %p.\n", iface, idx, start);

    if (idx >= range->count)
        return E_INVALIDARG;

    *start = range->ranges[idx].start;

    return S_OK;
}

static HRESULT WINAPI time_range_GetEnd(IMFMediaTimeRange *iface, DWORD idx, double *end)
{
    struct time_range *range = impl_from_IMFMediaTimeRange(iface);

    TRACE("%p, %u, %p.\n", iface, idx, end);

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

    TRACE("%p, %.8e, %.8e.\n", iface, start, end);

    if (range->count)
    {
        FIXME("Range merging is not implemented.\n");
        return E_NOTIMPL;
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

static inline struct media_engine *impl_from_IMFMediaEngine(IMFMediaEngine *iface)
{
    return CONTAINING_RECORD(iface, struct media_engine, IMFMediaEngine_iface);
}

static struct media_engine *impl_from_session_events_IMFAsyncCallback(IMFAsyncCallback *iface)
{
    return CONTAINING_RECORD(iface, struct media_engine, session_events);
}

static struct media_engine *impl_from_load_handler_IMFAsyncCallback(IMFAsyncCallback *iface)
{
    return CONTAINING_RECORD(iface, struct media_engine, load_handler);
}

static struct media_engine *impl_from_IMFSampleGrabberSinkCallback(IMFSampleGrabberSinkCallback *iface)
{
    return CONTAINING_RECORD(iface, struct media_engine, grabber_callback);
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
        WARN("Failed to get current media type %#x.\n", hr);
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
    return IMFMediaEngine_AddRef(&engine->IMFMediaEngine_iface);
}

static ULONG WINAPI media_engine_session_events_Release(IMFAsyncCallback *iface)
{
    struct media_engine *engine = impl_from_session_events_IMFAsyncCallback(iface);
    return IMFMediaEngine_Release(&engine->IMFMediaEngine_iface);
}

static HRESULT WINAPI media_engine_callback_GetParameters(IMFAsyncCallback *iface, DWORD *flags, DWORD *queue)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI media_engine_session_events_Invoke(IMFAsyncCallback *iface, IMFAsyncResult *result)
{
    struct media_engine *engine = impl_from_session_events_IMFAsyncCallback(iface);
    IMFMediaEvent *event = NULL;
    MediaEventType event_type;
    HRESULT hr;

    if (FAILED(hr = IMFMediaSession_EndGetEvent(engine->session, result, &event)))
    {
        WARN("Failed to get session event, hr %#x.\n", hr);
        goto failed;
    }

    if (FAILED(hr = IMFMediaEvent_GetType(event, &event_type)))
    {
        WARN("Failed to get event type, hr %#x.\n", hr);
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
    }

failed:

    if (event)
        IMFMediaEvent_Release(event);

    if (FAILED(hr = IMFMediaSession_BeginGetEvent(engine->session, iface, NULL)))
        WARN("Failed to subscribe to session events, hr %#x.\n", hr);

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

static ULONG WINAPI media_engine_load_handler_AddRef(IMFAsyncCallback *iface)
{
    struct media_engine *engine = impl_from_load_handler_IMFAsyncCallback(iface);
    return IMFMediaEngine_AddRef(&engine->IMFMediaEngine_iface);
}

static ULONG WINAPI media_engine_load_handler_Release(IMFAsyncCallback *iface)
{
    struct media_engine *engine = impl_from_load_handler_IMFAsyncCallback(iface);
    return IMFMediaEngine_Release(&engine->IMFMediaEngine_iface);
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
    DXGI_FORMAT output_format;
    IMFMediaType *media_type;
    IMFActivate *activate;
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

    hr = MFCreateSampleGrabberSinkActivate(media_type, &engine->grabber_callback, &activate);
    IMFMediaType_Release(media_type);
    if (FAILED(hr))
        return hr;

    if (SUCCEEDED(hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, node)))
    {
        IMFTopologyNode_SetObject(*node, (IUnknown *)activate);
        IMFTopologyNode_SetUINT32(*node, &MF_TOPONODE_NOSHUTDOWN_ON_REMOVE, FALSE);
    }

    IMFActivate_Release(activate);

    engine->video_frame.output_format = output_format;

    return hr;
}

static HRESULT media_engine_create_topology(struct media_engine *engine, IMFMediaSource *source)
{
    IMFStreamDescriptor *sd_audio = NULL, *sd_video = NULL;
    unsigned int stream_count = 0, i;
    IMFPresentationDescriptor *pd;
    IMFTopology *topology;
    UINT64 duration;
    HRESULT hr;

    media_engine_release_video_frame_resources(engine);

    if (FAILED(hr = IMFMediaSource_CreatePresentationDescriptor(source, &pd)))
        return hr;

    if (FAILED(hr = IMFPresentationDescriptor_GetStreamDescriptorCount(pd, &stream_count)))
        WARN("Failed to get stream count, hr %#x.\n", hr);

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

    media_engine_set_flag(engine, FLAGS_ENGINE_HAS_VIDEO, !!sd_video);
    media_engine_set_flag(engine, FLAGS_ENGINE_HAS_AUDIO, !!sd_audio);

    /* Assume live source if duration was not provided. */
    if (SUCCEEDED(IMFPresentationDescriptor_GetUINT64(pd, &MF_PD_DURATION, &duration)))
    {
        /* Convert 100ns to seconds. */
        engine->duration = duration / 10000000;
    }
    else
        engine->duration = INFINITY;

    if (SUCCEEDED(hr = MFCreateTopology(&topology)))
    {
        IMFTopologyNode *sar_node = NULL, *audio_src = NULL;
        IMFTopologyNode *grabber_node = NULL, *video_src = NULL;

        if (sd_audio)
        {
            if (FAILED(hr = media_engine_create_source_node(source, pd, sd_audio, &audio_src)))
                WARN("Failed to create audio source node, hr %#x.\n", hr);

            if (FAILED(hr = media_engine_create_audio_renderer(engine, &sar_node)))
                WARN("Failed to create audio renderer node, hr %#x.\n", hr);

            if (sar_node && audio_src)
            {
                IMFTopology_AddNode(topology, audio_src);
                IMFTopology_AddNode(topology, sar_node);
                IMFTopologyNode_ConnectOutput(audio_src, 0, sar_node, 0);
            }

            if (sar_node)
                IMFTopologyNode_Release(sar_node);
            if (audio_src)
                IMFTopologyNode_Release(audio_src);
        }

        if (SUCCEEDED(hr) && sd_video)
        {
            if (FAILED(hr = media_engine_create_source_node(source, pd, sd_video, &video_src)))
                WARN("Failed to create video source node, hr %#x.\n", hr);

            if (FAILED(hr = media_engine_create_video_renderer(engine, &grabber_node)))
                WARN("Failed to create video grabber node, hr %#x.\n", hr);

            if (grabber_node && video_src)
            {
                IMFTopology_AddNode(topology, video_src);
                IMFTopology_AddNode(topology, grabber_node);
                IMFTopologyNode_ConnectOutput(video_src, 0, grabber_node, 0);
            }

            if (SUCCEEDED(hr))
                IMFTopologyNode_GetTopoNodeID(video_src, &engine->video_frame.node_id);

            if (grabber_node)
                IMFTopologyNode_Release(grabber_node);
            if (video_src)
                IMFTopologyNode_Release(video_src);
        }

        IMFTopology_SetUINT32(topology, &MF_TOPOLOGY_ENUMERATE_SOURCE_TYPES, TRUE);

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
    PROPVARIANT var;

    var.vt = VT_EMPTY;
    IMFMediaSession_Start(engine->session, &GUID_NULL, &var);
}

static HRESULT WINAPI media_engine_load_handler_Invoke(IMFAsyncCallback *iface, IMFAsyncResult *result)
{
    struct media_engine *engine = impl_from_load_handler_IMFAsyncCallback(iface);
    unsigned int start_playback;
    MF_OBJECT_TYPE obj_type;
    IMFMediaSource *source;
    IUnknown *object = NULL;
    HRESULT hr;

    EnterCriticalSection(&engine->cs);

    engine->network_state = MF_MEDIA_ENGINE_NETWORK_LOADING;
    IMFMediaEngineNotify_EventNotify(engine->callback, MF_MEDIA_ENGINE_EVENT_LOADSTART, 0, 0);

    start_playback = engine->flags & FLAGS_ENGINE_PLAY_PENDING;
    media_engine_set_flag(engine, FLAGS_ENGINE_SOURCE_PENDING | FLAGS_ENGINE_PLAY_PENDING, FALSE);

    if (FAILED(hr = IMFSourceResolver_EndCreateObjectFromURL(engine->resolver, result, &obj_type, &object)))
        WARN("Failed to create source object, hr %#x.\n", hr);

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

static HRESULT WINAPI media_engine_QueryInterface(IMFMediaEngine *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IMFMediaEngine) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFMediaEngine_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI media_engine_AddRef(IMFMediaEngine *iface)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);
    ULONG refcount = InterlockedIncrement(&engine->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

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
    media_engine_release_video_frame_resources(engine);
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

static ULONG WINAPI media_engine_Release(IMFMediaEngine *iface)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);
    ULONG refcount = InterlockedDecrement(&engine->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    if (!refcount)
        free_media_engine(engine);

    return refcount;
}

static HRESULT WINAPI media_engine_GetError(IMFMediaEngine *iface, IMFMediaError **error)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);
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

static HRESULT WINAPI media_engine_SetErrorCode(IMFMediaEngine *iface, MF_MEDIA_ENGINE_ERR code)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);
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

static HRESULT WINAPI media_engine_SetSourceElements(IMFMediaEngine *iface, IMFMediaEngineSrcElements *elements)
{
    FIXME("(%p, %p): stub.\n", iface, elements);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_engine_SetSource(IMFMediaEngine *iface, BSTR url)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %s.\n", iface, debugstr_w(url));

    EnterCriticalSection(&engine->cs);

    if (engine->flags & FLAGS_ENGINE_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else
    {
        SysFreeString(engine->current_source);
        engine->current_source = NULL;
        if (url)
            engine->current_source = SysAllocString(url);

        engine->ready_state = MF_MEDIA_ENGINE_READY_HAVE_NOTHING;

        IMFMediaEngineNotify_EventNotify(engine->callback, MF_MEDIA_ENGINE_EVENT_PURGEQUEUEDEVENTS, 0, 0);

        engine->network_state = MF_MEDIA_ENGINE_NETWORK_NO_SOURCE;

        if (url)
        {
            IPropertyStore *props = NULL;
            unsigned int flags;

            flags = MF_RESOLUTION_MEDIASOURCE;
            if (engine->flags & MF_MEDIA_ENGINE_DISABLE_LOCAL_PLUGINS)
                flags |= MF_RESOLUTION_DISABLE_LOCAL_PLUGINS;

            IMFAttributes_GetUnknown(engine->attributes, &MF_MEDIA_ENGINE_SOURCE_RESOLVER_CONFIG_STORE,
                    &IID_IPropertyStore, (void **)&props);
            hr = IMFSourceResolver_BeginCreateObjectFromURL(engine->resolver, url, flags, props, NULL,
                    &engine->load_handler, NULL);
            if (SUCCEEDED(hr))
                media_engine_set_flag(engine, FLAGS_ENGINE_SOURCE_PENDING, TRUE);

            if (props)
                IPropertyStore_Release(props);
        }
    }

    LeaveCriticalSection(&engine->cs);

    return hr;
}

static HRESULT WINAPI media_engine_GetCurrentSource(IMFMediaEngine *iface, BSTR *url)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p.\n", iface, url);

    *url = NULL;

    EnterCriticalSection(&engine->cs);
    if (engine->current_source)
    {
        if (!(*url = SysAllocString(engine->current_source)))
            hr = E_OUTOFMEMORY;
    }
    LeaveCriticalSection(&engine->cs);

    return hr;
}

static USHORT WINAPI media_engine_GetNetworkState(IMFMediaEngine *iface)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);

    TRACE("%p.\n", iface);

    return engine->network_state;
}

static MF_MEDIA_ENGINE_PRELOAD WINAPI media_engine_GetPreload(IMFMediaEngine *iface)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);
    MF_MEDIA_ENGINE_PRELOAD preload;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&engine->cs);
    preload = engine->preload;
    LeaveCriticalSection(&engine->cs);

    return preload;
}

static HRESULT WINAPI media_engine_SetPreload(IMFMediaEngine *iface, MF_MEDIA_ENGINE_PRELOAD preload)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);

    TRACE("%p, %d.\n", iface, preload);

    EnterCriticalSection(&engine->cs);
    engine->preload = preload;
    LeaveCriticalSection(&engine->cs);

    return S_OK;
}

static HRESULT WINAPI media_engine_GetBuffered(IMFMediaEngine *iface, IMFMediaTimeRange **range)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);
    HRESULT hr;

    TRACE("%p, %p.\n", iface, range);

    if (FAILED(hr = create_time_range(range)))
        return hr;

    EnterCriticalSection(&engine->cs);
    if (!isnan(engine->duration))
        hr = IMFMediaTimeRange_AddRange(*range, 0.0, engine->duration);
    LeaveCriticalSection(&engine->cs);

    return hr;
}

static HRESULT WINAPI media_engine_Load(IMFMediaEngine *iface)
{
    FIXME("(%p): stub.\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_engine_CanPlayType(IMFMediaEngine *iface, BSTR type, MF_MEDIA_ENGINE_CANPLAY *answer)
{
    FIXME("(%p, %s, %p): stub.\n", iface, debugstr_w(type), answer);

    return E_NOTIMPL;
}

static USHORT WINAPI media_engine_GetReadyState(IMFMediaEngine *iface)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);
    unsigned short state;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&engine->cs);
    state = engine->ready_state;
    LeaveCriticalSection(&engine->cs);

    return state;
}

static BOOL WINAPI media_engine_IsSeeking(IMFMediaEngine *iface)
{
    FIXME("(%p): stub.\n", iface);

    return FALSE;
}

static double WINAPI media_engine_GetCurrentTime(IMFMediaEngine *iface)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);
    double ret = 0.0;
    MFTIME clocktime;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&engine->cs);
    if (engine->flags & FLAGS_ENGINE_IS_ENDED)
    {
        ret = engine->duration;
    }
    else if (SUCCEEDED(IMFPresentationClock_GetTime(engine->clock, &clocktime)))
    {
        ret = (double)clocktime / 10000000.0;
    }
    LeaveCriticalSection(&engine->cs);

    return ret;
}

static HRESULT WINAPI media_engine_SetCurrentTime(IMFMediaEngine *iface, double time)
{
    FIXME("(%p, %f): stub.\n", iface, time);

    return E_NOTIMPL;
}

static double WINAPI media_engine_GetStartTime(IMFMediaEngine *iface)
{
    FIXME("(%p): stub.\n", iface);

    return 0.0;
}

static double WINAPI media_engine_GetDuration(IMFMediaEngine *iface)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);
    double value;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&engine->cs);
    value = engine->duration;
    LeaveCriticalSection(&engine->cs);

    return value;
}

static BOOL WINAPI media_engine_IsPaused(IMFMediaEngine *iface)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);
    BOOL value;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&engine->cs);
    value = !!(engine->flags & FLAGS_ENGINE_PAUSED);
    LeaveCriticalSection(&engine->cs);

    return value;
}

static double WINAPI media_engine_GetDefaultPlaybackRate(IMFMediaEngine *iface)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);
    double rate;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&engine->cs);
    rate = engine->default_playback_rate;
    LeaveCriticalSection(&engine->cs);

    return rate;
}

static HRESULT WINAPI media_engine_SetDefaultPlaybackRate(IMFMediaEngine *iface, double rate)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);
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

static double WINAPI media_engine_GetPlaybackRate(IMFMediaEngine *iface)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);
    double rate;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&engine->cs);
    rate = engine->playback_rate;
    LeaveCriticalSection(&engine->cs);

    return rate;
}

static HRESULT WINAPI media_engine_SetPlaybackRate(IMFMediaEngine *iface, double rate)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);
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

static HRESULT WINAPI media_engine_GetPlayed(IMFMediaEngine *iface, IMFMediaTimeRange **played)
{
    FIXME("(%p, %p): stub.\n", iface, played);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_engine_GetSeekable(IMFMediaEngine *iface, IMFMediaTimeRange **seekable)
{
    FIXME("(%p, %p): stub.\n", iface, seekable);

    return E_NOTIMPL;
}

static BOOL WINAPI media_engine_IsEnded(IMFMediaEngine *iface)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);
    BOOL value;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&engine->cs);
    value = !!(engine->flags & FLAGS_ENGINE_IS_ENDED);
    LeaveCriticalSection(&engine->cs);

    return value;
}

static BOOL WINAPI media_engine_GetAutoPlay(IMFMediaEngine *iface)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);
    BOOL value;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&engine->cs);
    value = !!(engine->flags & FLAGS_ENGINE_AUTO_PLAY);
    LeaveCriticalSection(&engine->cs);

    return value;
}

static HRESULT WINAPI media_engine_SetAutoPlay(IMFMediaEngine *iface, BOOL autoplay)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);

    FIXME("(%p, %d): stub.\n", iface, autoplay);

    EnterCriticalSection(&engine->cs);
    media_engine_set_flag(engine, FLAGS_ENGINE_AUTO_PLAY, autoplay);
    LeaveCriticalSection(&engine->cs);

    return S_OK;
}

static BOOL WINAPI media_engine_GetLoop(IMFMediaEngine *iface)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);
    BOOL value;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&engine->cs);
    value = !!(engine->flags & FLAGS_ENGINE_LOOP);
    LeaveCriticalSection(&engine->cs);

    return value;
}

static HRESULT WINAPI media_engine_SetLoop(IMFMediaEngine *iface, BOOL loop)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);

    FIXME("(%p, %d): stub.\n", iface, loop);

    EnterCriticalSection(&engine->cs);
    media_engine_set_flag(engine, FLAGS_ENGINE_LOOP, loop);
    LeaveCriticalSection(&engine->cs);

    return S_OK;
}

static HRESULT WINAPI media_engine_Play(IMFMediaEngine *iface)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);

    TRACE("%p.\n", iface);

    EnterCriticalSection(&engine->cs);

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

    LeaveCriticalSection(&engine->cs);

    return S_OK;
}

static HRESULT WINAPI media_engine_Pause(IMFMediaEngine *iface)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);

    TRACE("%p.\n", iface);

    EnterCriticalSection(&engine->cs);

    if (!(engine->flags & FLAGS_ENGINE_PAUSED))
    {
        media_engine_set_flag(engine, FLAGS_ENGINE_WAITING | FLAGS_ENGINE_IS_ENDED, FALSE);
        media_engine_set_flag(engine, FLAGS_ENGINE_PAUSED, TRUE);

        IMFMediaEngineNotify_EventNotify(engine->callback, MF_MEDIA_ENGINE_EVENT_TIMEUPDATE, 0, 0);
        IMFMediaEngineNotify_EventNotify(engine->callback, MF_MEDIA_ENGINE_EVENT_PAUSE, 0, 0);
    }

    IMFMediaEngineNotify_EventNotify(engine->callback, MF_MEDIA_ENGINE_EVENT_PURGEQUEUEDEVENTS, 0, 0);

    LeaveCriticalSection(&engine->cs);

    return S_OK;
}

static BOOL WINAPI media_engine_GetMuted(IMFMediaEngine *iface)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);
    BOOL ret;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&engine->cs);
    ret = !!(engine->flags & FLAGS_ENGINE_MUTED);
    LeaveCriticalSection(&engine->cs);

    return ret;
}

static HRESULT WINAPI media_engine_SetMuted(IMFMediaEngine *iface, BOOL muted)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);
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

static double WINAPI media_engine_GetVolume(IMFMediaEngine *iface)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);
    double volume;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&engine->cs);
    volume = engine->volume;
    LeaveCriticalSection(&engine->cs);

    return volume;
}

static HRESULT WINAPI media_engine_SetVolume(IMFMediaEngine *iface, double volume)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %f.\n", iface, volume);

    EnterCriticalSection(&engine->cs);
    if (engine->flags & FLAGS_ENGINE_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else if (volume != engine->volume)
    {
        engine->volume = volume;
        IMFMediaEngineNotify_EventNotify(engine->callback, MF_MEDIA_ENGINE_EVENT_VOLUMECHANGE, 0, 0);
    }
    LeaveCriticalSection(&engine->cs);

    return hr;
}

static BOOL WINAPI media_engine_HasVideo(IMFMediaEngine *iface)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);
    BOOL value;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&engine->cs);
    value = !!(engine->flags & FLAGS_ENGINE_HAS_VIDEO);
    LeaveCriticalSection(&engine->cs);

    return value;
}

static BOOL WINAPI media_engine_HasAudio(IMFMediaEngine *iface)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);
    BOOL value;

    TRACE("%p.\n", iface);

    EnterCriticalSection(&engine->cs);
    value = !!(engine->flags & FLAGS_ENGINE_HAS_AUDIO);
    LeaveCriticalSection(&engine->cs);

    return value;
}

static HRESULT WINAPI media_engine_GetNativeVideoSize(IMFMediaEngine *iface, DWORD *cx, DWORD *cy)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);
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

static HRESULT WINAPI media_engine_GetVideoAspectRatio(IMFMediaEngine *iface, DWORD *cx, DWORD *cy)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);
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

static HRESULT WINAPI media_engine_Shutdown(IMFMediaEngine *iface)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);
    HRESULT hr = S_OK;

    FIXME("(%p): stub.\n", iface);

    EnterCriticalSection(&engine->cs);
    if (engine->flags & FLAGS_ENGINE_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else
    {
        media_engine_set_flag(engine, FLAGS_ENGINE_SHUT_DOWN, TRUE);
        IMFMediaSession_Shutdown(engine->session);
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

    if (!(engine->flags & FLAGS_ENGINE_NEW_FRAME))
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

    if (engine->video_frame.buffer_size == surface_desc.Width * surface_desc.Height)
    {
        ID3D11DeviceContext_UpdateSubresource(context, (ID3D11Resource *)engine->video_frame.d3d11.source,
                0, NULL, engine->video_frame.buffer, surface_desc.Width, 0);
    }

    media_engine_set_flag(engine, FLAGS_ENGINE_NEW_FRAME, FALSE);
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
        WARN("Failed to create d3d resources, hr %#x.\n", hr);
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
        WARN("Failed to create an rtv, hr %#x.\n", hr);
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

static HRESULT WINAPI media_engine_TransferVideoFrame(IMFMediaEngine *iface, IUnknown *surface,
        const MFVideoNormalizedRect *src_rect, const RECT *dst_rect, const MFARGB *color)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);
    ID3D11Texture2D *texture;
    HRESULT hr = E_NOINTERFACE;

    TRACE("%p, %p, %s, %s, %p.\n", iface, surface, src_rect ? wine_dbg_sprintf("(%f,%f)-(%f,%f)",
            src_rect->left, src_rect->top, src_rect->right, src_rect->bottom) : "(null)",
            wine_dbgstr_rect(dst_rect), color);

    EnterCriticalSection(&engine->cs);

    if (SUCCEEDED(IUnknown_QueryInterface(surface, &IID_ID3D11Texture2D, (void **)&texture)))
    {
        hr = media_engine_transfer_to_d3d11_texture(engine, texture, src_rect, dst_rect, color);
        ID3D11Texture2D_Release(texture);
    }
    else
    {
        FIXME("Unsupported destination type.\n");
    }

    LeaveCriticalSection(&engine->cs);

    return hr;
}

static HRESULT WINAPI media_engine_OnVideoStreamTick(IMFMediaEngine *iface, LONGLONG *pts)
{
    struct media_engine *engine = impl_from_IMFMediaEngine(iface);
    HRESULT hr;

    TRACE("%p, %p.\n", iface, pts);

    EnterCriticalSection(&engine->cs);

    if (engine->flags & FLAGS_ENGINE_SHUT_DOWN)
        hr = MF_E_SHUTDOWN;
    else if (!pts)
        hr = E_POINTER;
    else
    {
        *pts = engine->video_frame.pts;
        hr = *pts == MINLONGLONG ? S_FALSE : S_OK;
    }

    LeaveCriticalSection(&engine->cs);

    return hr;
}

static const IMFMediaEngineVtbl media_engine_vtbl =
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
};

static HRESULT WINAPI media_engine_grabber_callback_QueryInterface(IMFSampleGrabberSinkCallback *iface,
        REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IMFSampleGrabberSinkCallback) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFSampleGrabberSinkCallback_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI media_engine_grabber_callback_AddRef(IMFSampleGrabberSinkCallback *iface)
{
    struct media_engine *engine = impl_from_IMFSampleGrabberSinkCallback(iface);
    return IMFMediaEngine_AddRef(&engine->IMFMediaEngine_iface);
}

static ULONG WINAPI media_engine_grabber_callback_Release(IMFSampleGrabberSinkCallback *iface)
{
    struct media_engine *engine = impl_from_IMFSampleGrabberSinkCallback(iface);
    return IMFMediaEngine_Release(&engine->IMFMediaEngine_iface);
}

static HRESULT WINAPI media_engine_grabber_callback_OnClockStart(IMFSampleGrabberSinkCallback *iface,
        MFTIME systime, LONGLONG start_offset)
{
    return S_OK;
}

static HRESULT WINAPI media_engine_grabber_callback_OnClockStop(IMFSampleGrabberSinkCallback *iface,
        MFTIME systime)
{
    struct media_engine *engine = impl_from_IMFSampleGrabberSinkCallback(iface);

    EnterCriticalSection(&engine->cs);
    media_engine_set_flag(engine, FLAGS_ENGINE_FIRST_FRAME, FALSE);
    engine->video_frame.pts = MINLONGLONG;
    LeaveCriticalSection(&engine->cs);

    return S_OK;
}

static HRESULT WINAPI media_engine_grabber_callback_OnClockPause(IMFSampleGrabberSinkCallback *iface,
        MFTIME systime)
{
    return S_OK;
}

static HRESULT WINAPI media_engine_grabber_callback_OnClockRestart(IMFSampleGrabberSinkCallback *iface,
        MFTIME systime)
{
    return S_OK;
}

static HRESULT WINAPI media_engine_grabber_callback_OnClockSetRate(IMFSampleGrabberSinkCallback *iface,
        MFTIME systime, float rate)
{
    return S_OK;
}

static HRESULT WINAPI media_engine_grabber_callback_OnSetPresentationClock(IMFSampleGrabberSinkCallback *iface,
        IMFPresentationClock *clock)
{
    return S_OK;
}

static HRESULT WINAPI media_engine_grabber_callback_OnProcessSample(IMFSampleGrabberSinkCallback *iface,
        REFGUID major_type, DWORD sample_flags, LONGLONG sample_time, LONGLONG sample_duration,
        const BYTE *buffer, DWORD buffer_size)
{
    struct media_engine *engine = impl_from_IMFSampleGrabberSinkCallback(iface);

    EnterCriticalSection(&engine->cs);

    if (!(engine->flags & FLAGS_ENGINE_FIRST_FRAME))
    {
        IMFMediaEngineNotify_EventNotify(engine->callback, MF_MEDIA_ENGINE_EVENT_FIRSTFRAMEREADY, 0, 0);
        media_engine_set_flag(engine, FLAGS_ENGINE_FIRST_FRAME, TRUE);
    }
    engine->video_frame.pts = sample_time;
    if (engine->video_frame.buffer_size < buffer_size)
    {
        free(engine->video_frame.buffer);
        if ((engine->video_frame.buffer = malloc(buffer_size)))
            engine->video_frame.buffer_size = buffer_size;
    }
    if (engine->video_frame.buffer)
    {
        memcpy(engine->video_frame.buffer, buffer, buffer_size);
        engine->flags |= FLAGS_ENGINE_NEW_FRAME;
    }

    LeaveCriticalSection(&engine->cs);

    return S_OK;
}

static HRESULT WINAPI media_engine_grabber_callback_OnShutdown(IMFSampleGrabberSinkCallback *iface)
{
    return S_OK;
}

static const IMFSampleGrabberSinkCallbackVtbl media_engine_grabber_callback_vtbl =
{
    media_engine_grabber_callback_QueryInterface,
    media_engine_grabber_callback_AddRef,
    media_engine_grabber_callback_Release,
    media_engine_grabber_callback_OnClockStart,
    media_engine_grabber_callback_OnClockStop,
    media_engine_grabber_callback_OnClockPause,
    media_engine_grabber_callback_OnClockRestart,
    media_engine_grabber_callback_OnClockSetRate,
    media_engine_grabber_callback_OnSetPresentationClock,
    media_engine_grabber_callback_OnProcessSample,
    media_engine_grabber_callback_OnShutdown,
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
    DXGI_FORMAT output_format;
    UINT64 playback_hwnd;
    IMFClock *clock;
    HRESULT hr;

    engine->IMFMediaEngine_iface.lpVtbl = &media_engine_vtbl;
    engine->session_events.lpVtbl = &media_engine_session_events_vtbl;
    engine->load_handler.lpVtbl = &media_engine_load_handler_vtbl;
    engine->grabber_callback.lpVtbl = &media_engine_grabber_callback_vtbl;
    engine->refcount = 1;
    engine->flags = (flags & MF_MEDIA_ENGINE_CREATEFLAGS_MASK) | FLAGS_ENGINE_PAUSED;
    engine->default_playback_rate = 1.0;
    engine->playback_rate = 1.0;
    engine->volume = 1.0;
    engine->duration = NAN;
    engine->video_frame.pts = MINLONGLONG;
    InitializeCriticalSection(&engine->cs);

    hr = IMFAttributes_GetUnknown(attributes, &MF_MEDIA_ENGINE_CALLBACK, &IID_IMFMediaEngineNotify,
            (void **)&engine->callback);
    if (FAILED(hr))
        return hr;

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

    TRACE("%p, %#x, %p, %p.\n", iface, flags, attributes, engine);

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

    *engine = &object->IMFMediaEngine_iface;

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
