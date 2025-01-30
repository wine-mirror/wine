/*
 * Unit test suite for mfplat.
 *
 * Copyright 2015 Michael Müller
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
#include <string.h>
#include <limits.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "ole2.h"
#include "ks.h"
#include "ksmedia.h"
#include "amvideo.h"
#include "mfapi.h"
#include "mfidl.h"
#include "mferror.h"
#include "mfreadwrite.h"
#include "propvarutil.h"
#include "strsafe.h"
#include "uuids.h"
#include "evr.h"
#include "mfmediaengine.h"
#include "codecapi.h"
#include "rtworkq.h"

#include "wine/test.h"

#define D3D11_INIT_GUID
#include "initguid.h"
#include "d3d11_4.h"
#include "d3d9.h"
#include "d3d9types.h"
#include "ks.h"
#include "ksmedia.h"
#include "dxva2api.h"
#include "d3d12.h"
#undef EXTERN_GUID
#define EXTERN_GUID DEFINE_GUID
#include "mfd3d12.h"
#include "wmcodecdsp.h"
#include "dvdmedia.h"

DEFINE_GUID(DUMMY_CLSID, 0x12345678,0x1234,0x1234,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19);
DEFINE_GUID(DUMMY_GUID1, 0x12345678,0x1234,0x1234,0x21,0x21,0x21,0x21,0x21,0x21,0x21,0x21);
DEFINE_GUID(DUMMY_GUID2, 0x12345678,0x1234,0x1234,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22);
DEFINE_GUID(DUMMY_GUID3, 0x12345678,0x1234,0x1234,0x23,0x23,0x23,0x23,0x23,0x23,0x23,0x23);

extern const CLSID CLSID_FileSchemePlugin;

DEFINE_MEDIATYPE_GUID(MEDIASUBTYPE_Base,0);
DEFINE_GUID(MEDIASUBTYPE_ABGR32,D3DFMT_A8B8G8R8,0x524f,0x11ce,0x9f,0x53,0x00,0x20,0xaf,0x0b,0xa7,0x70);

DEFINE_MEDIATYPE_GUID(MFVideoFormat_RGB1, D3DFMT_A1);
DEFINE_MEDIATYPE_GUID(MFVideoFormat_RGB4, MAKEFOURCC('4','P','x','x'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_ABGR32, D3DFMT_A8B8G8R8);
DEFINE_MEDIATYPE_GUID(MFVideoFormat_ARGB1555, D3DFMT_A1R5G5B5);
DEFINE_MEDIATYPE_GUID(MFVideoFormat_ARGB4444, D3DFMT_A4R4G4B4);
/* SDK MFVideoFormat_A2R10G10B10 uses D3DFMT_A2B10G10R10, let's name it the other way */
DEFINE_MEDIATYPE_GUID(MFVideoFormat_A2B10G10R10, D3DFMT_A2R10G10B10);
DEFINE_MEDIATYPE_GUID(MFAudioFormat_RAW_AAC,WAVE_FORMAT_RAW_AAC1);

DEFINE_MEDIATYPE_GUID(MEDIASUBTYPE_h264,MAKEFOURCC('h','2','6','4'));
DEFINE_MEDIATYPE_GUID(MEDIASUBTYPE_MP3,WAVE_FORMAT_MPEGLAYER3);

DEFINE_MEDIATYPE_GUID(MFVideoFormat_IMC1, MAKEFOURCC('I','M','C','1'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_IMC2, MAKEFOURCC('I','M','C','2'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_IMC3, MAKEFOURCC('I','M','C','3'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_IMC4, MAKEFOURCC('I','M','C','4'));

DEFINE_MEDIATYPE_GUID(MEDIASUBTYPE_AVC1,MAKEFOURCC('A','V','C','1'));
DEFINE_MEDIATYPE_GUID(MEDIASUBTYPE_MP42,MAKEFOURCC('M','P','4','2'));
DEFINE_MEDIATYPE_GUID(MEDIASUBTYPE_mp42,MAKEFOURCC('m','p','4','2'));
DEFINE_MEDIATYPE_GUID(MEDIASUBTYPE_MPG4,MAKEFOURCC('M','P','G','4'));
DEFINE_MEDIATYPE_GUID(MEDIASUBTYPE_mpg4,MAKEFOURCC('m','p','g','4'));
DEFINE_MEDIATYPE_GUID(MEDIASUBTYPE_P422,MAKEFOURCC('P','4','2','2'));
DEFINE_MEDIATYPE_GUID(MEDIASUBTYPE_wmva,MAKEFOURCC('w','m','v','a'));
DEFINE_MEDIATYPE_GUID(MEDIASUBTYPE_WMVB,MAKEFOURCC('W','M','V','B'));
DEFINE_MEDIATYPE_GUID(MEDIASUBTYPE_wmvb,MAKEFOURCC('w','m','v','b'));
DEFINE_MEDIATYPE_GUID(MEDIASUBTYPE_wmvp,MAKEFOURCC('w','m','v','p'));
DEFINE_MEDIATYPE_GUID(MEDIASUBTYPE_wmvr,MAKEFOURCC('w','m','v','r'));
DEFINE_MEDIATYPE_GUID(MEDIASUBTYPE_wvp2,MAKEFOURCC('w','v','p','2'));
DEFINE_MEDIATYPE_GUID(MEDIASUBTYPE_X264,MAKEFOURCC('X','2','6','4'));
DEFINE_MEDIATYPE_GUID(MEDIASUBTYPE_x264,MAKEFOURCC('x','2','6','4'));

static BOOL is_win8_plus;

#define EXPECT_REF(obj,ref) _expect_ref((IUnknown*)obj, ref, __LINE__)
static void _expect_ref(IUnknown *obj, ULONG ref, int line)
{
    ULONG rc;
    IUnknown_AddRef(obj);
    rc = IUnknown_Release(obj);
    ok_(__FILE__,line)(rc == ref, "Unexpected refcount %ld, expected %ld.\n", rc, ref);
}

static ULONG get_refcount(void *iface)
{
    IUnknown *unknown = iface;
    IUnknown_AddRef(unknown);
    return IUnknown_Release(unknown);
}

#define check_interface(a, b, c) check_interface_(__LINE__, a, b, c)
static void check_interface_(unsigned int line, void *iface_ptr, REFIID iid, BOOL supported)
{
    IUnknown *iface = iface_ptr;
    HRESULT hr, expected_hr;
    IUnknown *unk;

    expected_hr = supported ? S_OK : E_NOINTERFACE;

    hr = IUnknown_QueryInterface(iface, iid, (void **)&unk);
    ok_(__FILE__, line)(hr == expected_hr, "Got hr %#lx, expected %#lx.\n", hr, expected_hr);
    if (SUCCEEDED(hr))
        IUnknown_Release(unk);
}

#define check_service_interface(a, b, c, d) check_service_interface_(__LINE__, a, b, c, d)
static void check_service_interface_(unsigned int line, void *iface_ptr, REFGUID service, REFIID iid, BOOL supported)
{
    IUnknown *iface = iface_ptr;
    HRESULT hr, expected_hr;
    IMFGetService *gs;
    IUnknown *unk;

    expected_hr = supported ? S_OK : E_NOINTERFACE;

    if (SUCCEEDED(hr = IUnknown_QueryInterface(iface, &IID_IMFGetService, (void **)&gs)))
    {
        hr = IMFGetService_GetService(gs, service, iid, (void **)&unk);
        IMFGetService_Release(gs);
    }
    ok_(__FILE__, line)(hr == expected_hr, "Got hr %#lx, expected %#lx.\n", hr, expected_hr);
    if (SUCCEEDED(hr))
        IUnknown_Release(unk);
}

struct attribute_desc
{
    const GUID *key;
    const char *name;
    PROPVARIANT value;
    BOOL ratio;
    BOOL required;
    BOOL todo;
    BOOL todo_value;
};

#define ATTR_GUID(k, g, ...)      {.key = &k, .name = #k, {.vt = VT_CLSID, .puuid = (GUID *)&g}, __VA_ARGS__ }
#define ATTR_UINT32(k, v, ...)    {.key = &k, .name = #k, {.vt = VT_UI4, .ulVal = v}, __VA_ARGS__ }
#define ATTR_BLOB(k, p, n, ...)   {.key = &k, .name = #k, {.vt = VT_VECTOR | VT_UI1, .caub = {.pElems = (void *)p, .cElems = n}}, __VA_ARGS__ }
#define ATTR_RATIO(k, n, d, ...)  {.key = &k, .name = #k, {.vt = VT_UI8, .uhVal = {.HighPart = n, .LowPart = d}}, .ratio = TRUE, __VA_ARGS__ }
#define ATTR_UINT64(k, v, ...)    {.key = &k, .name = #k, {.vt = VT_UI8, .uhVal = {.QuadPart = v}}, __VA_ARGS__ }

#define check_media_type(a, b, c) check_attributes_(__FILE__, __LINE__, (IMFAttributes *)a, b, c)

static void check_attributes_(const char *file, int line, IMFAttributes *attributes,
        const struct attribute_desc *desc, ULONG limit)
{
    char buffer[1024], *buf = buffer;
    PROPVARIANT value;
    int i, j, ret;
    HRESULT hr;

    for (i = 0; i < limit && desc[i].key; ++i)
    {
        hr = IMFAttributes_GetItem(attributes, desc[i].key, &value);
        todo_wine_if(desc[i].todo)
        ok_(file, line)(hr == S_OK, "%s missing, hr %#lx\n", debugstr_a(desc[i].name), hr);
        if (hr != S_OK) continue;

        switch (value.vt)
        {
        default: sprintf(buffer, "??"); break;
        case VT_CLSID: sprintf(buffer, "%s", debugstr_guid(value.puuid)); break;
        case VT_UI4: sprintf(buffer, "%lu", value.ulVal); break;
        case VT_UI8:
            if (desc[i].ratio)
                sprintf(buffer, "%lu:%lu", value.uhVal.HighPart, value.uhVal.LowPart);
            else
                sprintf(buffer, "%I64u", value.uhVal.QuadPart);
            break;
        case VT_VECTOR | VT_UI1:
            buf += sprintf(buf, "size %lu, data {", value.caub.cElems);
            for (j = 0; j < 128 && j < value.caub.cElems; ++j)
                buf += sprintf(buf, "0x%02x,", value.caub.pElems[j]);
            if (value.caub.cElems > 128)
                buf += sprintf(buf, "...}");
            else
                buf += sprintf(buf - (j ? 1 : 0), "}");
            break;
        }

        ret = PropVariantCompareEx(&value, &desc[i].value, 0, 0);
        todo_wine_if(desc[i].todo_value)
        ok_(file, line)(ret == 0, "%s mismatch, type %u, value %s\n",
                debugstr_a(desc[i].name), value.vt, buffer);
        PropVariantClear(&value);
    }
}

#define check_platform_lock_count(a) check_platform_lock_count_(__LINE__, a)
static void check_platform_lock_count_(unsigned int line, unsigned int expected)
{
    int i, count = 0;
    BOOL unexpected;
    DWORD queue;
    HRESULT hr;

    for (;;)
    {
        if (FAILED(hr = MFAllocateWorkQueue(&queue)))
        {
            unexpected = hr != MF_E_SHUTDOWN;
            break;
        }
        MFUnlockWorkQueue(queue);

        hr = MFUnlockPlatform();
        if ((unexpected = FAILED(hr)))
            break;

        ++count;
    }

    for (i = 0; i < count; ++i)
        MFLockPlatform();

    if (unexpected)
        count = -1;

    ok_(__FILE__, line)(count == expected, "Unexpected lock count %d.\n", count);
}

struct d3d9_surface_readback
{
    IDirect3DSurface9 *surface, *readback_surface;
    D3DLOCKED_RECT map_desc;
    D3DSURFACE_DESC surf_desc;
};

static void get_d3d9_surface_readback(IDirect3DSurface9 *surface, struct d3d9_surface_readback *rb)
{
    IDirect3DDevice9 *device;
    HRESULT hr;

    rb->surface = surface;

    hr = IDirect3DSurface9_GetDevice(surface, &device);
    ok(hr == D3D_OK, "Failed to get device, hr %#lx.\n", hr);

    hr = IDirect3DSurface9_GetDesc(surface, &rb->surf_desc);
    ok(hr == D3D_OK, "Failed to get surface desc, hr %#lx.\n", hr);
    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, rb->surf_desc.Width, rb->surf_desc.Height,
            rb->surf_desc.Format, D3DPOOL_SYSTEMMEM, &rb->readback_surface, NULL);
    ok(hr == D3D_OK, "Failed to create surface, hr %#lx.\n", hr);

    hr = IDirect3DDevice9_GetRenderTargetData(device, surface, rb->readback_surface);
    ok(hr == D3D_OK, "Failed to get render target data, hr %#lx.\n", hr);

    hr = IDirect3DSurface9_LockRect(rb->readback_surface, &rb->map_desc, NULL, 0);
    ok(hr == D3D_OK, "Failed to lock surface, hr %#lx.\n", hr);

    IDirect3DDevice9_Release(device);
}

static void release_d3d9_surface_readback(struct d3d9_surface_readback *rb, BOOL upload)
{
    ULONG refcount;
    HRESULT hr;

    hr = IDirect3DSurface9_UnlockRect(rb->readback_surface);
    ok(hr == D3D_OK, "Failed to unlock surface, hr %#lx.\n", hr);

    if (upload)
    {
        IDirect3DDevice9 *device;

        IDirect3DSurface9_GetDevice(rb->surface, &device);
        ok(hr == D3D_OK, "Failed to get device, hr %#lx.\n", hr);

        hr = IDirect3DDevice9_UpdateSurface(device, rb->readback_surface, NULL, rb->surface, NULL);
        ok(hr == D3D_OK, "Failed to update surface, hr %#lx.\n", hr);

        IDirect3DDevice9_Release(device);
    }

    refcount = IDirect3DSurface9_Release(rb->readback_surface);
    ok(refcount == 0, "Readback surface still has references.\n");
}

static void *get_d3d9_readback_data(struct d3d9_surface_readback *rb,
        unsigned int x, unsigned int y, unsigned byte_width)
{
    return (BYTE *)rb->map_desc.pBits + y * rb->map_desc.Pitch + x * byte_width;
}

static DWORD get_d3d9_readback_u32(struct d3d9_surface_readback *rb, unsigned int x, unsigned int y)
{
    return *(DWORD *)get_d3d9_readback_data(rb, x, y, sizeof(DWORD));
}

static DWORD get_d3d9_readback_color(struct d3d9_surface_readback *rb, unsigned int x, unsigned int y)
{
    return get_d3d9_readback_u32(rb, x, y);
}

static DWORD get_d3d9_surface_color(IDirect3DSurface9 *surface, unsigned int x, unsigned int y)
{
    struct d3d9_surface_readback rb;
    DWORD color;

    get_d3d9_surface_readback(surface, &rb);
    color = get_d3d9_readback_color(&rb, x, y);
    release_d3d9_surface_readback(&rb, FALSE);

    return color;
}

static void put_d3d9_readback_u32(struct d3d9_surface_readback *rb, unsigned int x, unsigned int y, DWORD color)
{
    *(DWORD *)get_d3d9_readback_data(rb, x, y, sizeof(DWORD)) = color;
}

static void put_d3d9_readback_color(struct d3d9_surface_readback *rb, unsigned int x, unsigned int y, DWORD color)
{
    put_d3d9_readback_u32(rb, x, y, color);
}

static void put_d3d9_surface_color(IDirect3DSurface9 *surface,
        unsigned int x, unsigned int y, DWORD color)
{
    struct d3d9_surface_readback rb;

    get_d3d9_surface_readback(surface, &rb);
    put_d3d9_readback_color(&rb, x, y, color);
    release_d3d9_surface_readback(&rb, TRUE);
}

struct d3d11_resource_readback
{
    ID3D11Resource *orig_resource, *resource;
    D3D11_MAPPED_SUBRESOURCE map_desc;
    ID3D11DeviceContext *immediate_context;
    unsigned int width, height, depth, sub_resource_idx;
};

static void init_d3d11_resource_readback(ID3D11Resource *resource, ID3D11Resource *readback_resource,
        unsigned int width, unsigned int height, unsigned int depth, unsigned int sub_resource_idx,
        ID3D11Device *device, struct d3d11_resource_readback *rb)
{
    HRESULT hr;

    rb->orig_resource = resource;
    rb->resource = readback_resource;
    rb->width = width;
    rb->height = height;
    rb->depth = depth;
    rb->sub_resource_idx = sub_resource_idx;

    ID3D11Device_GetImmediateContext(device, &rb->immediate_context);

    ID3D11DeviceContext_CopyResource(rb->immediate_context, rb->resource, resource);
    if (FAILED(hr = ID3D11DeviceContext_Map(rb->immediate_context,
            rb->resource, sub_resource_idx, D3D11_MAP_READ_WRITE, 0, &rb->map_desc)))
    {
        trace("Failed to map resource, hr %#lx.\n", hr);
        ID3D11Resource_Release(rb->resource);
        rb->resource = NULL;
        ID3D11DeviceContext_Release(rb->immediate_context);
        rb->immediate_context = NULL;
    }
}

static void get_d3d11_texture2d_readback(ID3D11Texture2D *texture, unsigned int sub_resource_idx,
        struct d3d11_resource_readback *rb)
{
    D3D11_TEXTURE2D_DESC texture_desc;
    ID3D11Resource *rb_texture;
    unsigned int miplevel;
    ID3D11Device *device;
    HRESULT hr;

    memset(rb, 0, sizeof(*rb));

    ID3D11Texture2D_GetDevice(texture, &device);

    ID3D11Texture2D_GetDesc(texture, &texture_desc);
    texture_desc.Usage = D3D11_USAGE_STAGING;
    texture_desc.BindFlags = 0;
    texture_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
    texture_desc.MiscFlags = 0;
    if (FAILED(hr = ID3D11Device_CreateTexture2D(device, &texture_desc, NULL, (ID3D11Texture2D **)&rb_texture)))
    {
        trace("Failed to create texture, hr %#lx.\n", hr);
        ID3D11Device_Release(device);
        return;
    }

    miplevel = sub_resource_idx % texture_desc.MipLevels;
    init_d3d11_resource_readback((ID3D11Resource *)texture, rb_texture,
            max(1, texture_desc.Width >> miplevel),
            max(1, texture_desc.Height >> miplevel),
            1, sub_resource_idx, device, rb);

    ID3D11Device_Release(device);
}

static void release_d3d11_resource_readback(struct d3d11_resource_readback *rb, BOOL upload)
{
    ID3D11DeviceContext_Unmap(rb->immediate_context, rb->resource, rb->sub_resource_idx);

    if (upload)
    {
        ID3D11DeviceContext_CopyResource(rb->immediate_context, rb->orig_resource, rb->resource);
    }

    ID3D11Resource_Release(rb->resource);
    ID3D11DeviceContext_Release(rb->immediate_context);
}

static void *get_d3d11_readback_data(struct d3d11_resource_readback *rb,
        unsigned int x, unsigned int y, unsigned int z, unsigned byte_width)
{
    return (BYTE *)rb->map_desc.pData + z * rb->map_desc.DepthPitch + y * rb->map_desc.RowPitch + x * byte_width;
}

static DWORD get_d3d11_readback_u32(struct d3d11_resource_readback *rb, unsigned int x, unsigned int y, unsigned int z)
{
    return *(DWORD *)get_d3d11_readback_data(rb, x, y, z, sizeof(DWORD));
}

static DWORD get_d3d11_readback_color(struct d3d11_resource_readback *rb, unsigned int x, unsigned int y, unsigned int z)
{
    return get_d3d11_readback_u32(rb, x, y, z);
}

static DWORD get_d3d11_texture_color(ID3D11Texture2D *texture, unsigned int x, unsigned int y)
{
    struct d3d11_resource_readback rb;
    DWORD color;

    get_d3d11_texture2d_readback(texture, 0, &rb);
    color = get_d3d11_readback_color(&rb, x, y, 0);
    release_d3d11_resource_readback(&rb, FALSE);

    return color;
}

static void put_d3d11_readback_u32(struct d3d11_resource_readback *rb,
        unsigned int x, unsigned int y, unsigned int z, DWORD color)
{
    *(DWORD *)get_d3d11_readback_data(rb, x, y, z, sizeof(DWORD)) = color;
}

static void put_d3d11_readback_color(struct d3d11_resource_readback *rb,
        unsigned int x, unsigned int y, unsigned int z, DWORD color)
{
    put_d3d11_readback_u32(rb, x, y, z, color);
}

static void put_d3d11_texture_color(ID3D11Texture2D *texture, unsigned int x, unsigned int y, DWORD color)
{
    struct d3d11_resource_readback rb;

    get_d3d11_texture2d_readback(texture, 0, &rb);
    put_d3d11_readback_color(&rb, x, y, 0, color);
    release_d3d11_resource_readback(&rb, TRUE);
}

static HRESULT (WINAPI *pD3D11CreateDevice)(IDXGIAdapter *adapter, D3D_DRIVER_TYPE driver_type, HMODULE swrast, UINT flags,
        const D3D_FEATURE_LEVEL *feature_levels, UINT levels, UINT sdk_version, ID3D11Device **device_out,
        D3D_FEATURE_LEVEL *obtained_feature_level, ID3D11DeviceContext **immediate_context);
static HRESULT (WINAPI *pD3D12CreateDevice)(IUnknown *adapter, D3D_FEATURE_LEVEL minimum_feature_level,
        REFIID iid, void **device);

static HRESULT (WINAPI *pCoGetApartmentType)(APTTYPE *type, APTTYPEQUALIFIER *qualifier);

static HRESULT (WINAPI *pMFCopyImage)(BYTE *dest, LONG deststride, const BYTE *src, LONG srcstride,
        DWORD width, DWORD lines);
static HRESULT (WINAPI *pMFCreateDXGIDeviceManager)(UINT *token, IMFDXGIDeviceManager **manager);
static HRESULT (WINAPI *pMFCreateSourceResolver)(IMFSourceResolver **resolver);
static HRESULT (WINAPI *pMFCreateMFByteStreamOnStream)(IStream *stream, IMFByteStream **bytestream);
static HRESULT (WINAPI *pMFPutWaitingWorkItem)(HANDLE event, LONG priority, IMFAsyncResult *result, MFWORKITEM_KEY *key);
static HRESULT (WINAPI *pMFAllocateSerialWorkQueue)(DWORD queue, DWORD *serial_queue);
static HRESULT (WINAPI *pMFAddPeriodicCallback)(MFPERIODICCALLBACK callback, IUnknown *context, DWORD *key);
static HRESULT (WINAPI *pMFRemovePeriodicCallback)(DWORD key);
static HRESULT (WINAPI *pMFRegisterLocalByteStreamHandler)(const WCHAR *extension, const WCHAR *mime,
        IMFActivate *activate);
static HRESULT (WINAPI *pMFRegisterLocalSchemeHandler)(const WCHAR *scheme, IMFActivate *activate);
static HRESULT (WINAPI *pMFCreateTransformActivate)(IMFActivate **activate);
static HRESULT (WINAPI *pMFTRegisterLocal)(IClassFactory *factory, REFGUID category, LPCWSTR name,
        UINT32 flags, UINT32 cinput, const MFT_REGISTER_TYPE_INFO *input_types, UINT32 coutput,
        const MFT_REGISTER_TYPE_INFO* output_types);
static HRESULT (WINAPI *pMFTRegisterLocalByCLSID)(REFCLSID clsid, REFGUID category, LPCWSTR name, UINT32 flags,
        UINT32 input_count, const MFT_REGISTER_TYPE_INFO *input_types, UINT32 output_count,
        const MFT_REGISTER_TYPE_INFO *output_types);
static HRESULT (WINAPI *pMFTUnregisterLocal)(IClassFactory *factory);
static HRESULT (WINAPI *pMFTUnregisterLocalByCLSID)(CLSID clsid);
static HRESULT (WINAPI *pMFAllocateWorkQueueEx)(MFASYNC_WORKQUEUE_TYPE queue_type, DWORD *queue);
static HRESULT (WINAPI *pMFTEnumEx)(GUID category, UINT32 flags, const MFT_REGISTER_TYPE_INFO *input_type,
        const MFT_REGISTER_TYPE_INFO *output_type, IMFActivate ***activate, UINT32 *count);
static HRESULT (WINAPI *pMFGetPlaneSize)(DWORD format, DWORD width, DWORD height, DWORD *size);
static HRESULT (WINAPI *pMFGetStrideForBitmapInfoHeader)(DWORD format, DWORD width, LONG *stride);
static HRESULT (WINAPI *pMFCreate2DMediaBuffer)(DWORD width, DWORD height, DWORD fourcc, BOOL bottom_up,
        IMFMediaBuffer **buffer);
static HRESULT (WINAPI *pMFCreateMediaBufferFromMediaType)(IMFMediaType *media_type, LONGLONG duration, DWORD min_length,
        DWORD min_alignment, IMFMediaBuffer **buffer);
static HRESULT (WINAPI *pMFCreateLegacyMediaBufferOnMFMediaBuffer)(IMFSample *sample, IMFMediaBuffer *media_buffer, DWORD offset,
        IMediaBuffer **obj);
static HRESULT (WINAPI *pMFCreatePathFromURL)(const WCHAR *url, WCHAR **path);
static HRESULT (WINAPI *pMFCreateDXSurfaceBuffer)(REFIID riid, IUnknown *surface, BOOL bottom_up, IMFMediaBuffer **buffer);
static HRESULT (WINAPI *pMFCreateTrackedSample)(IMFTrackedSample **sample);
static DWORD (WINAPI *pMFMapDXGIFormatToDX9Format)(DXGI_FORMAT dxgi_format);
static DXGI_FORMAT (WINAPI *pMFMapDX9FormatToDXGIFormat)(DWORD format);
static HRESULT (WINAPI *pMFCreateVideoSampleAllocatorEx)(REFIID riid, void **allocator);
static HRESULT (WINAPI *pMFCreateDXGISurfaceBuffer)(REFIID riid, IUnknown *surface, UINT subresource, BOOL bottomup,
        IMFMediaBuffer **buffer);
static HRESULT (WINAPI *pMFCreateVideoMediaTypeFromSubtype)(const GUID *subtype, IMFVideoMediaType **media_type);
static HRESULT (WINAPI *pMFLockSharedWorkQueue)(const WCHAR *name, LONG base_priority, DWORD *taskid, DWORD *queue);
static HRESULT (WINAPI *pMFLockDXGIDeviceManager)(UINT *token, IMFDXGIDeviceManager **manager);
static HRESULT (WINAPI *pMFUnlockDXGIDeviceManager)(void);
static HRESULT (WINAPI *pMFInitVideoFormat_RGB)(MFVIDEOFORMAT *format, DWORD width, DWORD height, DWORD d3dformat);

static HRESULT (WINAPI *pRtwqStartup)(void);
static HRESULT (WINAPI *pRtwqShutdown)(void);
static HRESULT (WINAPI *pRtwqLockPlatform)(void);
static HRESULT (WINAPI *pRtwqUnlockPlatform)(void);

static HWND create_window(void)
{
    RECT r = {0, 0, 640, 480};

    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW | WS_VISIBLE, FALSE);

    return CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, r.right - r.left, r.bottom - r.top, NULL, NULL, NULL, NULL);
}

static IDirect3DDevice9 *create_d3d9_device(IDirect3D9 *d3d9, HWND focus_window)
{
    D3DPRESENT_PARAMETERS present_parameters = {0};
    IDirect3DDevice9 *device = NULL;

    present_parameters.BackBufferWidth = 640;
    present_parameters.BackBufferHeight = 480;
    present_parameters.BackBufferFormat = D3DFMT_A8R8G8B8;
    present_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
    present_parameters.hDeviceWindow = focus_window;
    present_parameters.Windowed = TRUE;
    present_parameters.EnableAutoDepthStencil = TRUE;
    present_parameters.AutoDepthStencilFormat = D3DFMT_D24S8;
    present_parameters.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;

    IDirect3D9_CreateDevice(d3d9, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, focus_window,
            D3DCREATE_HARDWARE_VERTEXPROCESSING, &present_parameters, &device);

    return device;
}

static const WCHAR fileschemeW[] = L"file://";

static WCHAR *load_resource(const WCHAR *name)
{
    static WCHAR pathW[MAX_PATH];
    DWORD written;
    HANDLE file;
    HRSRC res;
    void *ptr;

    GetTempPathW(ARRAY_SIZE(pathW), pathW);
    lstrcatW(pathW, name);

    file = CreateFileW(pathW, GENERIC_READ|GENERIC_WRITE, 0,
                       NULL, CREATE_ALWAYS, 0, 0);
    ok(file != INVALID_HANDLE_VALUE, "file creation failed, at %s, error %ld\n",
       wine_dbgstr_w(pathW), GetLastError());

    res = FindResourceW(NULL, name, (LPCWSTR)RT_RCDATA);
    ok(res != 0, "couldn't find resource\n");
    ptr = LockResource(LoadResource(GetModuleHandleA(NULL), res));
    WriteFile(file, ptr, SizeofResource(GetModuleHandleA(NULL), res),
               &written, NULL);
    ok(written == SizeofResource(GetModuleHandleA(NULL), res),
       "couldn't write resource\n" );
    CloseHandle(file);

    return pathW;
}

static BOOL is_MEDIASUBTYPE_RGB(const GUID *subtype)
{
    return IsEqualGUID(subtype, &MEDIASUBTYPE_RGB8)
            || IsEqualGUID(subtype, &MEDIASUBTYPE_RGB555)
            || IsEqualGUID(subtype, &MEDIASUBTYPE_RGB565)
            || IsEqualGUID(subtype, &MEDIASUBTYPE_RGB24)
            || IsEqualGUID(subtype, &MEDIASUBTYPE_RGB32);
}

struct test_callback
{
    IMFAsyncCallback IMFAsyncCallback_iface;
    LONG refcount;
    HANDLE event;
    DWORD param;
    IMFMediaEvent *media_event;
    IMFAsyncResult *result;
};

static struct test_callback *impl_from_IMFAsyncCallback(IMFAsyncCallback *iface)
{
    return CONTAINING_RECORD(iface, struct test_callback, IMFAsyncCallback_iface);
}

static HRESULT WINAPI testcallback_QueryInterface(IMFAsyncCallback *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IMFAsyncCallback) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFAsyncCallback_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI testcallback_AddRef(IMFAsyncCallback *iface)
{
    struct test_callback *callback = impl_from_IMFAsyncCallback(iface);
    return InterlockedIncrement(&callback->refcount);
}

static ULONG WINAPI testcallback_Release(IMFAsyncCallback *iface)
{
    struct test_callback *callback = impl_from_IMFAsyncCallback(iface);
    ULONG refcount = InterlockedDecrement(&callback->refcount);

    if (!refcount)
    {
        CloseHandle(callback->event);
        free(callback);
    }

    return refcount;
}

static HRESULT WINAPI testcallback_GetParameters(IMFAsyncCallback *iface, DWORD *flags, DWORD *queue)
{
    ok(flags != NULL && queue != NULL, "Unexpected arguments.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_async_callback_result_Invoke(IMFAsyncCallback *iface, IMFAsyncResult *result)
{
    struct test_callback *callback = impl_from_IMFAsyncCallback(iface);

    callback->result = result;
    IMFAsyncResult_AddRef(callback->result);
    SetEvent(callback->event);

    return S_OK;
}

static const IMFAsyncCallbackVtbl test_async_callback_result_vtbl =
{
    testcallback_QueryInterface,
    testcallback_AddRef,
    testcallback_Release,
    testcallback_GetParameters,
    test_async_callback_result_Invoke,
};

static DWORD wait_async_callback_result(IMFAsyncCallback *iface, DWORD timeout, IMFAsyncResult **result)
{
    struct test_callback *callback = impl_from_IMFAsyncCallback(iface);
    DWORD res = WaitForSingleObject(callback->event, timeout);

    *result = callback->result;
    callback->result = NULL;

    return res;
}

static BOOL check_clsid(CLSID *clsids, UINT32 count)
{
    int i;
    for (i = 0; i < count; i++)
    {
        if (IsEqualGUID(&clsids[i], &DUMMY_CLSID))
            return TRUE;
    }
    return FALSE;
}

static void test_register(void)
{
    MFT_REGISTER_TYPE_INFO *in_types, *out_types;
    WCHAR name[] = L"Wine test";
    MFT_REGISTER_TYPE_INFO input[] =
    {
        { DUMMY_CLSID, DUMMY_GUID1 }
    };
    MFT_REGISTER_TYPE_INFO output[] =
    {
        { DUMMY_CLSID, DUMMY_GUID2 }
    };
    UINT32 count, in_count, out_count;
    IMFAttributes *attributes;
    WCHAR *mft_name;
    CLSID *clsids;
    HRESULT hr, ret;

    ret = MFTRegister(DUMMY_CLSID, MFT_CATEGORY_OTHER, name, 0, 1, input, 1, output, NULL);
    if (ret == E_ACCESSDENIED)
    {
        win_skip("Not enough permissions to register a transform.\n");
        return;
    }
    ok(ret == S_OK, "Failed to register dummy transform, hr %#lx.\n", ret);

if(0)
{
    /* NULL name crashes on windows */
    ret = MFTRegister(DUMMY_CLSID, MFT_CATEGORY_OTHER, NULL, 0, 1, input, 1, output, NULL);
    ok(ret == E_INVALIDARG, "Unexpected hr %#lx.\n", ret);
}

    ret = MFTRegister(DUMMY_CLSID, MFT_CATEGORY_OTHER, name, 0, 0, NULL, 0, NULL, NULL);
    ok(ret == S_OK, "Failed to register dummy filter: %lx\n", ret);

    ret = MFTRegister(DUMMY_CLSID, MFT_CATEGORY_OTHER, name, 0, 1, NULL, 0, NULL, NULL);
    ok(ret == S_OK, "Failed to register dummy filter: %lx\n", ret);

    ret = MFTRegister(DUMMY_CLSID, MFT_CATEGORY_OTHER, name, 0, 0, NULL, 1, NULL, NULL);
    ok(ret == S_OK, "Failed to register dummy filter: %lx\n", ret);

if(0)
{
    /* NULL clsids/count crashes on windows (vista) */
    count = 0;
    ret = MFTEnum(MFT_CATEGORY_OTHER, 0, NULL, NULL, NULL, NULL, &count);
    ok(ret == E_POINTER, "Failed to enumerate filters: %lx\n", ret);
    ok(count == 0, "Expected count == 0\n");

    clsids = NULL;
    ret = MFTEnum(MFT_CATEGORY_OTHER, 0, NULL, NULL, NULL, &clsids, NULL);
    ok(ret == E_POINTER, "Failed to enumerate filters: %lx\n", ret);
}
    hr = MFTGetInfo(DUMMY_CLSID, &mft_name, NULL, NULL, NULL, NULL, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(mft_name, L"Wine test"), "Unexpected name %s.\n", wine_dbgstr_w(mft_name));
    CoTaskMemFree(mft_name);

    hr = MFTGetInfo(DUMMY_CLSID, NULL, NULL, NULL, NULL, NULL, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    in_count = out_count = 1;
    hr = MFTGetInfo(DUMMY_CLSID, NULL, NULL, &in_count, NULL, &out_count, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!in_count, "Unexpected count %u.\n", in_count);
    ok(!out_count, "Unexpected count %u.\n", out_count);

    hr = MFTGetInfo(DUMMY_CLSID, NULL, NULL, NULL, NULL, NULL, &attributes);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!attributes, "Unexpected attributes.\n");
    IMFAttributes_Release(attributes);

    hr = MFTGetInfo(DUMMY_CLSID, &mft_name, &in_types, &in_count, &out_types, &out_count, &attributes);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(mft_name, L"Wine test"), "Unexpected name %s.\n", wine_dbgstr_w(mft_name));
    ok(!!in_types, "Unexpected pointer.\n");
    ok(!!out_types, "Unexpected pointer.\n");
    ok(in_count == 1, "Unexpected count %u.\n", in_count);
    ok(out_count == 1, "Unexpected count %u.\n", out_count);
    ok(IsEqualGUID(&in_types->guidMajorType, &DUMMY_CLSID), "Unexpected type guid %s.\n",
            wine_dbgstr_guid(&in_types->guidMajorType));
    ok(IsEqualGUID(&in_types->guidSubtype, &DUMMY_GUID1), "Unexpected type guid %s.\n",
            wine_dbgstr_guid(&in_types->guidSubtype));
    ok(IsEqualGUID(&out_types->guidMajorType, &DUMMY_CLSID), "Unexpected type guid %s.\n",
            wine_dbgstr_guid(&out_types->guidMajorType));
    ok(IsEqualGUID(&out_types->guidSubtype, &DUMMY_GUID2), "Unexpected type guid %s.\n",
            wine_dbgstr_guid(&out_types->guidSubtype));
    ok(!!attributes, "Unexpected attributes.\n");
    count = 1;
    hr = IMFAttributes_GetCount(attributes, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!count, "Unexpected count %u.\n", count);
    CoTaskMemFree(mft_name);
    CoTaskMemFree(in_types);
    CoTaskMemFree(out_types);
    IMFAttributes_Release(attributes);

    count = 0;
    clsids = NULL;
    ret = MFTEnum(MFT_CATEGORY_OTHER, 0, NULL, NULL, NULL, &clsids, &count);
    ok(ret == S_OK, "Failed to enumerate filters: %lx\n", ret);
    ok(count > 0, "Expected count > 0\n");
    ok(clsids != NULL, "Expected clsids != NULL\n");
    ok(check_clsid(clsids, count), "Filter was not part of enumeration\n");
    CoTaskMemFree(clsids);

    count = 0;
    clsids = NULL;
    ret = MFTEnum(MFT_CATEGORY_OTHER, 0, input, NULL, NULL, &clsids, &count);
    ok(ret == S_OK, "Failed to enumerate filters: %lx\n", ret);
    ok(count > 0, "Expected count > 0\n");
    ok(clsids != NULL, "Expected clsids != NULL\n");
    ok(check_clsid(clsids, count), "Filter was not part of enumeration\n");
    CoTaskMemFree(clsids);

    count = 0;
    clsids = NULL;
    ret = MFTEnum(MFT_CATEGORY_OTHER, 0, NULL, output, NULL, &clsids, &count);
    ok(ret == S_OK, "Failed to enumerate filters: %lx\n", ret);
    ok(count > 0, "Expected count > 0\n");
    ok(clsids != NULL, "Expected clsids != NULL\n");
    ok(check_clsid(clsids, count), "Filter was not part of enumeration\n");
    CoTaskMemFree(clsids);

    count = 0;
    clsids = NULL;
    ret = MFTEnum(MFT_CATEGORY_OTHER, 0, input, output, NULL, &clsids, &count);
    ok(ret == S_OK, "Failed to enumerate filters: %lx\n", ret);
    ok(count > 0, "Expected count > 0\n");
    ok(clsids != NULL, "Expected clsids != NULL\n");
    ok(check_clsid(clsids, count), "Filter was not part of enumeration\n");
    CoTaskMemFree(clsids);

    /* exchange input and output */
    count = 0;
    clsids = NULL;
    ret = MFTEnum(MFT_CATEGORY_OTHER, 0, output, input, NULL, &clsids, &count);
    ok(ret == S_OK, "Failed to enumerate filters: %lx\n", ret);
    ok(!count, "got %d\n", count);
    ok(clsids == NULL, "Expected clsids == NULL\n");

    ret = MFTUnregister(DUMMY_CLSID);
    ok(ret == S_OK ||
       /* w7pro64 */
       broken(ret == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)), "Unexpected hr %#lx.\n", ret);

    ret = MFTUnregister(DUMMY_CLSID);
    ok(ret == S_OK || broken(ret == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)), "Unexpected hr %#lx.\n", ret);
}

static HRESULT WINAPI test_create_from_url_callback_Invoke(IMFAsyncCallback *iface, IMFAsyncResult *result)
{
    struct test_callback *callback = impl_from_IMFAsyncCallback(iface);
    IMFSourceResolver *resolver;
    IUnknown *object, *object2;
    MF_OBJECT_TYPE obj_type;
    HRESULT hr;

    ok(!!result, "Unexpected result object.\n");

    resolver = (IMFSourceResolver *)IMFAsyncResult_GetStateNoAddRef(result);

    object = NULL;
    hr = IMFSourceResolver_EndCreateObjectFromURL(resolver, result, &obj_type, &object);
    ok(hr == S_OK, "Failed to create an object, hr %#lx.\n", hr);

    hr = IMFAsyncResult_GetObject(result, &object2);
    ok(hr == S_OK, "Failed to get result object, hr %#lx.\n", hr);
    ok(object2 == object, "Unexpected object.\n");

    if (object)
        IUnknown_Release(object);
    IUnknown_Release(object2);

    SetEvent(callback->event);

    return S_OK;
}

static const IMFAsyncCallbackVtbl test_create_from_url_callback_vtbl =
{
    testcallback_QueryInterface,
    testcallback_AddRef,
    testcallback_Release,
    testcallback_GetParameters,
    test_create_from_url_callback_Invoke,
};

static HRESULT WINAPI test_create_from_file_handler_callback_Invoke(IMFAsyncCallback *iface, IMFAsyncResult *result)
{
    struct test_callback *callback = impl_from_IMFAsyncCallback(iface);
    IMFSchemeHandler *handler;
    IUnknown *object, *object2;
    MF_OBJECT_TYPE obj_type;
    HRESULT hr;

    ok(!!result, "Unexpected result object.\n");

    handler = (IMFSchemeHandler *)IMFAsyncResult_GetStateNoAddRef(result);

    hr = IMFSchemeHandler_EndCreateObject(handler, result, &obj_type, &object);
    ok(hr == S_OK, "Failed to create an object, hr %#lx.\n", hr);
    todo_wine ok(obj_type == MF_OBJECT_BYTESTREAM, "Got object type %#x.\n", obj_type);

    hr = IMFAsyncResult_GetObject(result, (IUnknown **)&object2);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    if (obj_type == MF_OBJECT_MEDIASOURCE)
    {
        IMFMediaSource *media_source;

        hr = IUnknown_QueryInterface(object, &IID_IMFMediaSource, (void **)&media_source);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IMFMediaSource_Shutdown(media_source);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        IMFMediaSource_Release(media_source);
    }

    IUnknown_Release(object);

    SetEvent(callback->event);

    return S_OK;
}

static const IMFAsyncCallbackVtbl test_create_from_file_handler_callback_vtbl =
{
    testcallback_QueryInterface,
    testcallback_AddRef,
    testcallback_Release,
    testcallback_GetParameters,
    test_create_from_file_handler_callback_Invoke,
};

static HRESULT WINAPI source_events_callback_Invoke(IMFAsyncCallback *iface, IMFAsyncResult *result)
{
    struct test_callback *callback = impl_from_IMFAsyncCallback(iface);
    IMFMediaEventGenerator *generator;
    HRESULT hr;

    ok(!!result, "Unexpected result object.\n");

    generator = (IMFMediaEventGenerator *)IMFAsyncResult_GetStateNoAddRef(result);

    hr = IMFMediaEventGenerator_EndGetEvent(generator, result, &callback->media_event);
    ok(hr == S_OK, "Failed to create an object, hr %#lx.\n", hr);

    SetEvent(callback->event);

    return S_OK;
}

static const IMFAsyncCallbackVtbl events_callback_vtbl =
{
    testcallback_QueryInterface,
    testcallback_AddRef,
    testcallback_Release,
    testcallback_GetParameters,
    source_events_callback_Invoke,
};

static const IMFAsyncCallbackVtbl testcallbackvtbl;

static struct test_callback * create_test_callback(const IMFAsyncCallbackVtbl *vtbl)
{
    struct test_callback *callback = calloc(1, sizeof(*callback));

    callback->IMFAsyncCallback_iface.lpVtbl = vtbl ? vtbl : &testcallbackvtbl;
    callback->refcount = 1;
    callback->event = CreateEventA(NULL, FALSE, FALSE, NULL);

    return callback;
}

static BOOL get_event(IMFMediaEventGenerator *generator, MediaEventType expected_event_type, PROPVARIANT *value)
{
    struct test_callback *callback;
    MediaEventType event_type;
    BOOL ret = FALSE;
    HRESULT hr;

    callback = create_test_callback(&events_callback_vtbl);

    for (;;)
    {
        hr = IMFMediaEventGenerator_BeginGetEvent(generator, &callback->IMFAsyncCallback_iface,
                (IUnknown *)generator);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        if (WaitForSingleObject(callback->event, 1000) == WAIT_TIMEOUT)
        {
            ok(0, "timeout\n");
            break;
        }

        Sleep(10);

        hr = IMFMediaEvent_GetType(callback->media_event, &event_type);
        ok(hr == S_OK, "Failed to event type, hr %#lx.\n", hr);

        if ((ret = (event_type == expected_event_type)))
        {
            if (value)
            {
                hr = IMFMediaEvent_GetValue(callback->media_event, value);
                ok(hr == S_OK, "Failed to get value of event, hr %#lx.\n", hr);
            }

            break;
        }
    }

    if (callback->media_event)
        IMFMediaEvent_Release(callback->media_event);
    IMFAsyncCallback_Release(&callback->IMFAsyncCallback_iface);

    return ret;
}

static const IMFByteStreamVtbl *bytestream_vtbl_orig;

static int bytestream_closed = 0;
static HRESULT WINAPI bytestream_wrapper_Close(IMFByteStream *iface)
{
    bytestream_closed = 1;
    return bytestream_vtbl_orig->Close(iface);
}

static void test_compressed_media_types(IMFSourceResolver *resolver)
{
    static const BYTE aac_data[] =
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x12, 0x08, 0x56, 0xe5, 0x00,
    };
    static const BYTE wma2_data[] = {0, 0, 0, 0, 1, 0, 0, 0, 0, 0};

    static const struct
    {
        const WCHAR *filename;
        const WCHAR *mime;
        struct attribute_desc type[20];
    }
    tests[] =
    {
        {
            L"test-h264.mp4",
            L"video/mp4",
            {
                ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
                ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_H264, .todo_value = TRUE),
                ATTR_GUID(MF_MT_AM_FORMAT_TYPE, FORMAT_MPEG2Video, .todo = TRUE),
                ATTR_RATIO(MF_MT_FRAME_SIZE, 320, 240),
                ATTR_RATIO(MF_MT_FRAME_RATE, 30, 1),
                ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 1, 1, .todo = TRUE),
                ATTR_UINT32(MF_MT_MPEG2_PROFILE, eAVEncH264VProfile_High, .todo = TRUE),
                ATTR_UINT32(MF_MT_MPEG2_LEVEL, eAVEncH264VLevel2, .todo = TRUE),
            },
        },
        {
            L"test-aac.mp4",
            L"video/mp4",
            {
                ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
                ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_AAC, .todo_value = TRUE),
                ATTR_GUID(MF_MT_AM_FORMAT_TYPE, FORMAT_WaveFormatEx, .todo = TRUE),
                ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 1),
                ATTR_UINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16, .todo_value = TRUE),
                ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 44100),
                ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 9180, .todo_value = TRUE),
                ATTR_UINT32(MF_MT_AUDIO_PREFER_WAVEFORMATEX, 1, .todo = TRUE),
                ATTR_UINT32(MF_MT_AVG_BITRATE, 73440, .todo = TRUE),
                ATTR_UINT32(MF_MT_AAC_AUDIO_PROFILE_LEVEL_INDICATION, 0, .todo = TRUE),
                ATTR_UINT32(MF_MT_AAC_PAYLOAD_TYPE, 0, .todo = TRUE),
                ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 1, .todo = TRUE),
                ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
                ATTR_BLOB(MF_MT_USER_DATA, aac_data, sizeof(aac_data), .todo = TRUE),
            },
        },
        {
            L"test-wmv1.wmv",
            L"video/x-ms-wmv",
            {
                ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
                ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_WMV1, .todo_value = TRUE),
                ATTR_RATIO(MF_MT_FRAME_SIZE, 320, 240),
                ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 1, 1, .todo = TRUE),
            },
        },
        {
            L"test-wma2.wmv",
            L"video/x-ms-wmv",
            {
                ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
                ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_WMAudioV8, .todo_value = TRUE),
                ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 1),
                ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 44100),
                ATTR_UINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 16000, .todo_value = TRUE),
                ATTR_UINT32(MF_MT_AUDIO_PREFER_WAVEFORMATEX, 1, .todo = TRUE),
                ATTR_UINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 743, .todo_value = TRUE),
                ATTR_BLOB(MF_MT_USER_DATA, wma2_data, sizeof(wma2_data), .todo = TRUE),
            },
        },
        {
            L"test-mp3.mp4",
            L"video/mp4",
            {
                ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio),
                ATTR_GUID(MF_MT_SUBTYPE, MFAudioFormat_MP3, .todo_value = TRUE),
                ATTR_GUID(MF_MT_AM_FORMAT_TYPE, FORMAT_WaveFormatEx, .todo = TRUE),
                ATTR_UINT32(MF_MT_AUDIO_NUM_CHANNELS, 1),
                ATTR_UINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 44100),
                ATTR_UINT32(MF_MT_AUDIO_PREFER_WAVEFORMATEX, 1, .todo = TRUE),
                ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 1, .todo = TRUE),
                ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
            },
        },
        {
            L"test-i420.avi",
            L"video/avi",
            {
                ATTR_GUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
                ATTR_GUID(MF_MT_SUBTYPE, MFVideoFormat_I420),
                ATTR_RATIO(MF_MT_FRAME_SIZE, 32, 24),
                ATTR_RATIO(MF_MT_FRAME_RATE, 30, 1),
                ATTR_RATIO(MF_MT_PIXEL_ASPECT_RATIO, 1, 1, .todo = TRUE),
                ATTR_UINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1),
                ATTR_UINT32(MF_MT_DEFAULT_STRIDE, 32),
                ATTR_UINT32(MF_MT_FIXED_SIZE_SAMPLES, 1, .todo = TRUE),
            },
        },
    };

    for (unsigned int i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        IMFPresentationDescriptor *descriptor;
        IMFMediaTypeHandler *handler;
        IMFAttributes *attributes;
        IMFMediaType *media_type;
        IMFStreamDescriptor *sd;
        MF_OBJECT_TYPE obj_type;
        IMFMediaSource *source;
        IMFByteStream *stream;
        BOOL ret, selected;
        WCHAR *filename;
        DWORD count;
        HRESULT hr;

        winetest_push_context("%s", debugstr_w(tests[i].filename));

        filename = load_resource(tests[i].filename);

        hr = MFCreateFile(MF_ACCESSMODE_READ, MF_OPENMODE_FAIL_IF_NOT_EXIST, MF_FILEFLAGS_NONE, filename, &stream);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        hr = IMFByteStream_QueryInterface(stream, &IID_IMFAttributes, (void **)&attributes);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        hr = IMFAttributes_SetString(attributes, &MF_BYTESTREAM_CONTENT_TYPE, tests[i].mime);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        IMFAttributes_Release(attributes);

        hr = IMFSourceResolver_CreateObjectFromByteStream(resolver, stream, NULL,
                MF_RESOLUTION_MEDIASOURCE, NULL, &obj_type, (IUnknown **)&source);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ok(obj_type == MF_OBJECT_MEDIASOURCE, "Got type %#x.\n", obj_type);

        hr = IMFMediaSource_CreatePresentationDescriptor(source, &descriptor);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        hr = IMFPresentationDescriptor_GetStreamDescriptorCount(descriptor, &count);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ok(count == 1, "Got count %lu.\n", count);

        hr = IMFPresentationDescriptor_GetStreamDescriptorByIndex(descriptor, 0, &selected, &sd);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ok(selected == TRUE, "Got selected %d.\n", selected);

        hr = IMFStreamDescriptor_GetMediaTypeHandler(sd, &handler);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        hr = IMFMediaTypeHandler_GetCurrentMediaType(handler, &media_type);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);

        check_media_type(media_type, tests[i].type, -1);

        IMFMediaType_Release(media_type);

        hr = IMFMediaTypeHandler_GetMediaTypeCount(handler, &count);
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        todo_wine ok(count == 1, "Got type count %lu.\n", count);

        IMFMediaTypeHandler_Release(handler);
        IMFStreamDescriptor_Release(sd);

        IMFPresentationDescriptor_Release(descriptor);
        IMFMediaSource_Release(source);
        IMFByteStream_Release(stream);

        ret = DeleteFileW(filename);
        ok(ret, "Failed to delete %s, error %lu.\n", debugstr_w(filename), GetLastError());

        winetest_pop_context();
    }
}

static void test_source_resolver(void)
{
    struct test_callback *callback, *callback2;
    IMFByteStreamVtbl bytestream_vtbl_wrapper;
    IMFSourceResolver *resolver, *resolver2;
    IMFPresentationDescriptor *descriptor;
    IMFSchemeHandler *scheme_handler;
    IMFMediaStream *video_stream;
    IMFAttributes *attributes;
    IMFMediaSource *mediasource;
    IMFMediaTypeHandler *handler;
    IMFMediaType *media_type;
    BOOL selected, do_uninit;
    MF_OBJECT_TYPE obj_type;
    IMFStreamDescriptor *sd;
    IUnknown *cancel_cookie;
    IMFByteStream *stream, *tmp_stream;
    IMFGetService *get_service;
    IMFRateSupport *rate_support;
    WCHAR pathW[MAX_PATH];
    int i, sample_count;
    DWORD size, flags;
    BYTE buffer[1024];
    WCHAR *filename;
    PROPVARIANT var;
    QWORD length;
    HRESULT hr;
    GUID guid;
    float rate;
    UINT32 rotation;
    ULONG refcount;
    BOOL ret;

    if (!pMFCreateSourceResolver)
    {
        win_skip("MFCreateSourceResolver() not found\n");
        return;
    }

    callback = create_test_callback(&test_create_from_url_callback_vtbl);
    callback2 = create_test_callback(&test_create_from_file_handler_callback_vtbl);

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = pMFCreateSourceResolver(NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = pMFCreateSourceResolver(&resolver);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = pMFCreateSourceResolver(&resolver2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(resolver != resolver2, "Expected new instance\n");

    IMFSourceResolver_Release(resolver2);

    filename = load_resource(L"test.mp4");

    hr = MFCreateFile(MF_ACCESSMODE_READ, MF_OPENMODE_FAIL_IF_NOT_EXIST, MF_FILEFLAGS_NONE, filename, &stream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFSourceResolver_CreateObjectFromByteStream(
        resolver, NULL, NULL, MF_RESOLUTION_MEDIASOURCE, NULL,
        &obj_type, (IUnknown **)&mediasource);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFSourceResolver_CreateObjectFromByteStream(resolver, stream, NULL, MF_RESOLUTION_MEDIASOURCE, NULL,
            NULL, (IUnknown **)&mediasource);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFSourceResolver_CreateObjectFromByteStream(resolver, stream, NULL, MF_RESOLUTION_MEDIASOURCE, NULL,
            &obj_type, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    IMFByteStream_Release(stream);

    /* Create from URL. */

    hr = IMFSourceResolver_CreateObjectFromURL(resolver, L"nonexisting.mp4", MF_RESOLUTION_BYTESTREAM, NULL, &obj_type,
            (IUnknown **)&stream);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), "Unexpected hr %#lx.\n", hr);

    hr = IMFSourceResolver_CreateObjectFromURL(resolver, filename, MF_RESOLUTION_BYTESTREAM, NULL, &obj_type,
            (IUnknown **)&stream);
    ok(hr == S_OK, "Failed to resolve url, hr %#lx.\n", hr);
    IMFByteStream_Release(stream);

    hr = IMFSourceResolver_BeginCreateObjectFromURL(resolver, filename, MF_RESOLUTION_BYTESTREAM, NULL,
            &cancel_cookie, &callback->IMFAsyncCallback_iface, (IUnknown *)resolver);
    ok(hr == S_OK, "Create request failed, hr %#lx.\n", hr);
    ok(cancel_cookie != NULL, "Unexpected cancel object.\n");
    IUnknown_Release(cancel_cookie);

    if (SUCCEEDED(hr))
        WaitForSingleObject(callback->event, INFINITE);

    /* With explicit scheme. */
    lstrcpyW(pathW, fileschemeW);
    lstrcatW(pathW, filename);

    hr = IMFSourceResolver_CreateObjectFromURL(resolver, pathW, MF_RESOLUTION_BYTESTREAM, NULL, &obj_type,
            (IUnknown **)&stream);
    ok(hr == S_OK, "Failed to resolve url, hr %#lx.\n", hr);
    IMFByteStream_Release(stream);

    /* We have to create a new bytestream here, because all following
     * calls to CreateObjectFromByteStream will fail. */
    hr = MFCreateFile(MF_ACCESSMODE_READ, MF_OPENMODE_FAIL_IF_NOT_EXIST, MF_FILEFLAGS_NONE, filename, &stream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFByteStream_QueryInterface(stream, &IID_IMFAttributes, (void **)&attributes);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFAttributes_SetString(attributes, &MF_BYTESTREAM_CONTENT_TYPE, L"video/mp4");
    ok(hr == S_OK, "Failed to set string value, hr %#lx.\n", hr);
    IMFAttributes_Release(attributes);

    /* Start of gstreamer dependent tests */

    hr = IMFSourceResolver_CreateObjectFromByteStream(resolver, stream, NULL, MF_RESOLUTION_MEDIASOURCE, NULL,
            &obj_type, (IUnknown **)&mediasource);
    if (strcmp(winetest_platform, "wine"))
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (FAILED(hr))
    {
        IMFByteStream_Release(stream);
        IMFSourceResolver_Release(resolver);

        hr = MFShutdown();
        ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);

        DeleteFileW(filename);
        return;
    }
    ok(mediasource != NULL, "got %p\n", mediasource);
    ok(obj_type == MF_OBJECT_MEDIASOURCE, "got %d\n", obj_type);

    refcount = IMFMediaSource_Release(mediasource);
    todo_wine
    ok(!refcount, "Unexpected refcount %ld\n", refcount);
    IMFByteStream_Release(stream);

    /* test that bytestream position doesn't matter */
    hr = MFCreateFile(MF_ACCESSMODE_READ, MF_OPENMODE_FAIL_IF_NOT_EXIST, MF_FILEFLAGS_NONE, filename, &stream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFByteStream_SetCurrentPosition(stream, 1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFSourceResolver_CreateObjectFromByteStream(resolver, stream, NULL, MF_RESOLUTION_MEDIASOURCE, NULL,
            &obj_type, (IUnknown **)&mediasource);
    ok(hr == S_OK || broken(hr == MF_E_UNSUPPORTED_BYTESTREAM_TYPE) /* w7 || w8 */, "Unexpected hr %#lx.\n", hr);
    if (hr == S_OK) IMFMediaSource_Release(mediasource);
    IMFByteStream_Release(stream);

    hr = MFCreateFile(MF_ACCESSMODE_READ, MF_OPENMODE_FAIL_IF_NOT_EXIST, MF_FILEFLAGS_NONE, filename, &stream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFByteStream_GetLength(stream, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFByteStream_SetCurrentPosition(stream, length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFSourceResolver_CreateObjectFromByteStream(resolver, stream, NULL, MF_RESOLUTION_MEDIASOURCE, NULL,
            &obj_type, (IUnknown **)&mediasource);
    ok(hr == S_OK || broken(hr == MF_E_UNSUPPORTED_BYTESTREAM_TYPE) /* w7 || w8 */, "Unexpected hr %#lx.\n", hr);
    if (hr == S_OK) IMFMediaSource_Release(mediasource);
    IMFByteStream_Release(stream);

    /* stream must have a valid header, media cannot start in the middle of a stream */
    hr = MFCreateFile(MF_ACCESSMODE_READ, MF_OPENMODE_FAIL_IF_NOT_EXIST, MF_FILEFLAGS_NONE, filename, &tmp_stream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFCreateTempFile(MF_ACCESSMODE_READ | MF_ACCESSMODE_WRITE, MF_OPENMODE_FAIL_IF_EXIST, MF_FILEFLAGS_NONE, &stream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    memset(buffer, 0xcd, sizeof(buffer));
    hr = IMFByteStream_Write(stream, buffer, sizeof(buffer), &size);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    do
    {
        hr = IMFByteStream_Read(tmp_stream, buffer, sizeof(buffer), &size);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        hr = IMFByteStream_Write(stream, buffer, size, &size);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    }
    while (size >= sizeof(buffer));
    IMFByteStream_Release(tmp_stream);
    hr = IMFByteStream_SetCurrentPosition(stream, sizeof(buffer));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    flags = MF_RESOLUTION_MEDIASOURCE;
    flags |= MF_RESOLUTION_KEEP_BYTE_STREAM_ALIVE_ON_FAIL;
    hr = IMFSourceResolver_CreateObjectFromByteStream(resolver, stream, NULL, flags, NULL,
            &obj_type, (IUnknown **)&mediasource);
    todo_wine ok(hr == MF_E_UNSUPPORTED_BYTESTREAM_TYPE, "Unexpected hr %#lx.\n", hr);

    flags |= MF_RESOLUTION_CONTENT_DOES_NOT_HAVE_TO_MATCH_EXTENSION_OR_MIME_TYPE;
    hr = IMFSourceResolver_CreateObjectFromByteStream(resolver, stream, NULL, flags, NULL,
            &obj_type, (IUnknown **)&mediasource);
    todo_wine ok(hr == MF_E_SOURCERESOLVER_MUTUALLY_EXCLUSIVE_FLAGS, "Unexpected hr %#lx.\n", hr);

    flags = MF_RESOLUTION_MEDIASOURCE;
    flags |= MF_RESOLUTION_CONTENT_DOES_NOT_HAVE_TO_MATCH_EXTENSION_OR_MIME_TYPE;
    hr = IMFSourceResolver_CreateObjectFromByteStream(resolver, stream, NULL, flags, NULL,
            &obj_type, (IUnknown **)&mediasource);
    todo_wine ok(hr == MF_E_UNSUPPORTED_BYTESTREAM_TYPE, "Unexpected hr %#lx.\n", hr);
    IMFByteStream_Release(stream);

    /* We have to create a new bytestream here, because all following
     * calls to CreateObjectFromByteStream will fail. */
    hr = MFCreateFile(MF_ACCESSMODE_READ, MF_OPENMODE_FAIL_IF_NOT_EXIST, MF_FILEFLAGS_NONE, filename, &stream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Wrap ::Close to test when the media source calls it */
    bytestream_vtbl_orig = stream->lpVtbl;
    bytestream_vtbl_wrapper = *bytestream_vtbl_orig;
    bytestream_vtbl_wrapper.Close = bytestream_wrapper_Close;
    stream->lpVtbl = &bytestream_vtbl_wrapper;

    hr = IMFByteStream_QueryInterface(stream, &IID_IMFAttributes, (void **)&attributes);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFAttributes_SetString(attributes, &MF_BYTESTREAM_CONTENT_TYPE, L"video/mp4");
    ok(hr == S_OK, "Failed to set string value, hr %#lx.\n", hr);
    IMFAttributes_Release(attributes);

    hr = IMFSourceResolver_CreateObjectFromByteStream(resolver, stream, NULL, MF_RESOLUTION_MEDIASOURCE, NULL,
            &obj_type, (IUnknown **)&mediasource);
    ok(mediasource != NULL, "got %p\n", mediasource);
    ok(obj_type == MF_OBJECT_MEDIASOURCE, "got %d\n", obj_type);

    check_interface(mediasource, &IID_IMFGetService, TRUE);
    check_service_interface(mediasource, &MF_RATE_CONTROL_SERVICE, &IID_IMFRateSupport, TRUE);

    hr = IMFMediaSource_QueryInterface(mediasource, &IID_IMFGetService, (void**)&get_service);
    ok(hr == S_OK, "Failed to get service interface, hr %#lx.\n", hr);

    hr = IMFGetService_GetService(get_service, &MF_RATE_CONTROL_SERVICE, &IID_IMFRateSupport, (void**)&rate_support);
    ok(hr == S_OK, "Failed to get rate support interface, hr %#lx.\n", hr);

    hr = IMFRateSupport_GetFastestRate(rate_support, MFRATE_FORWARD, FALSE, &rate);
    ok(hr == S_OK, "Failed to query fastest rate, hr %#lx.\n", hr);
    ok(rate == 1e6f, "Unexpected fastest rate %f.\n", rate);
    hr = IMFRateSupport_GetFastestRate(rate_support, MFRATE_FORWARD, TRUE, &rate);
    ok(hr == S_OK, "Failed to query fastest rate, hr %#lx.\n", hr);
    ok(rate == 1e6f, "Unexpected fastest rate %f.\n", rate);
    hr = IMFRateSupport_GetFastestRate(rate_support, MFRATE_REVERSE, FALSE, &rate);
    ok(hr == S_OK, "Failed to query fastest rate, hr %#lx.\n", hr);
    ok(rate == -1e6f, "Unexpected fastest rate %f.\n", rate);
    hr = IMFRateSupport_GetFastestRate(rate_support, MFRATE_REVERSE, TRUE, &rate);
    ok(hr == S_OK, "Failed to query fastest rate, hr %#lx.\n", hr);
    ok(rate == -1e6f, "Unexpected fastest rate %f.\n", rate);

    hr = IMFRateSupport_GetSlowestRate(rate_support, MFRATE_FORWARD, FALSE, &rate);
    ok(hr == S_OK, "Failed to query slowest rate, hr %#lx.\n", hr);
    ok(rate == 0.0f, "Unexpected slowest rate %f.\n", rate);
    hr = IMFRateSupport_GetSlowestRate(rate_support, MFRATE_FORWARD, TRUE, &rate);
    ok(hr == S_OK, "Failed to query slowest rate, hr %#lx.\n", hr);
    ok(rate == 0.0f, "Unexpected slowest rate %f.\n", rate);
    hr = IMFRateSupport_GetSlowestRate(rate_support, MFRATE_REVERSE, FALSE, &rate);
    ok(hr == S_OK, "Failed to query slowest rate, hr %#lx.\n", hr);
    ok(rate == 0.0f, "Unexpected slowest rate %f.\n", rate);
    hr = IMFRateSupport_GetSlowestRate(rate_support, MFRATE_REVERSE, TRUE, &rate);
    ok(hr == S_OK, "Failed to query slowest rate, hr %#lx.\n", hr);
    ok(rate == 0.0f, "Unexpected slowest rate %f.\n", rate);

    hr = IMFRateSupport_IsRateSupported(rate_support, FALSE, 0.0f, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFRateSupport_IsRateSupported(rate_support, FALSE, 0.0f, &rate);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(rate == 0.0f, "Unexpected rate %f.\n", rate);

    hr = IMFRateSupport_IsRateSupported(rate_support, FALSE, 1.0f, &rate);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(rate == 1.0f, "Unexpected rate %f.\n", rate);
    hr = IMFRateSupport_IsRateSupported(rate_support, FALSE, -1.0f, &rate);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(rate == -1.0f, "Unexpected rate %f.\n", rate);
    hr = IMFRateSupport_IsRateSupported(rate_support, FALSE, 1e6f + 1.0f, &rate);
    ok(hr == MF_E_UNSUPPORTED_RATE, "Unexpected hr %#lx.\n", hr);
    ok(rate == 1e6f + 1.0f || broken(rate == 1e6f) /* Win7 */, "Unexpected %f.\n", rate);
    hr = IMFRateSupport_IsRateSupported(rate_support, FALSE, -1e6f, &rate);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(rate == -1e6f, "Unexpected rate %f.\n", rate);

    hr = IMFRateSupport_IsRateSupported(rate_support, FALSE, -1e6f - 1.0f, &rate);
    ok(hr == MF_E_UNSUPPORTED_RATE, "Unexpected hr %#lx.\n", hr);
    ok(rate == -1e6f - 1.0f || broken(rate == -1e6f) /* Win7 */, "Unexpected rate %f.\n", rate);

    check_service_interface(mediasource, &MF_RATE_CONTROL_SERVICE, &IID_IMFRateControl, TRUE);
    hr = IMFMediaSource_CreatePresentationDescriptor(mediasource, &descriptor);
    ok(hr == S_OK, "Failed to get presentation descriptor, hr %#lx.\n", hr);
    ok(descriptor != NULL, "got %p\n", descriptor);

    hr = IMFPresentationDescriptor_GetStreamDescriptorByIndex(descriptor, 0, &selected, &sd);
    ok(hr == S_OK, "Failed to get stream descriptor, hr %#lx.\n", hr);

    hr = IMFStreamDescriptor_GetMediaTypeHandler(sd, &handler);
    ok(hr == S_OK, "Failed to get type handler, hr %#lx.\n", hr);
    IMFStreamDescriptor_Release(sd);

    hr = IMFMediaTypeHandler_GetMajorType(handler, &guid);
    ok(hr == S_OK, "Failed to get stream major type, hr %#lx.\n", hr);

    /* Check major/minor type for the test media. */
    ok(IsEqualGUID(&guid, &MFMediaType_Video), "Unexpected major type %s.\n", debugstr_guid(&guid));

    hr = IMFMediaTypeHandler_GetCurrentMediaType(handler, &media_type);
    ok(hr == S_OK, "Failed to get current media type, hr %#lx.\n", hr);
    hr = IMFMediaType_GetGUID(media_type, &MF_MT_SUBTYPE, &guid);
    ok(hr == S_OK, "Failed to get media sub type, hr %#lx.\n", hr);
    todo_wine
    ok(IsEqualGUID(&guid, &MFVideoFormat_M4S2), "Unexpected sub type %s.\n", debugstr_guid(&guid));

    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_VIDEO_ROTATION, &rotation);
    ok(hr == S_OK || broken(hr == MF_E_ATTRIBUTENOTFOUND) /* Win7 */, "Failed to get rotation, hr %#lx.\n", hr);
    if (hr == S_OK)
        ok(rotation == MFVideoRotationFormat_0, "Got wrong rotation %u.\n", rotation);

    IMFMediaType_Release(media_type);

    hr = IMFPresentationDescriptor_SelectStream(descriptor, 0);
    ok(hr == S_OK, "Failed to select video stream, hr %#lx.\n", hr);

    var.vt = VT_EMPTY;
    hr = IMFMediaSource_Start(mediasource, descriptor, &GUID_NULL, &var);
    ok(hr == S_OK, "Failed to start media source, hr %#lx.\n", hr);

    video_stream = NULL;
    if (get_event((IMFMediaEventGenerator *)mediasource, MENewStream, &var))
    {
        ok(var.vt == VT_UNKNOWN, "Unexpected value type.\n");
        video_stream = (IMFMediaStream *)var.punkVal;
    }

    hr = IMFMediaSource_Pause(mediasource);
    ok(hr == S_OK, "Failed to pause media source, hr %#lx.\n", hr);
    if (get_event((IMFMediaEventGenerator *)mediasource, MESourcePaused, &var))
        ok(var.vt == VT_EMPTY, "Unexpected value type.\n");

    var.vt = VT_EMPTY;
    hr = IMFMediaSource_Start(mediasource, descriptor, &GUID_NULL, &var);
    ok(hr == S_OK, "Failed to start media source, hr %#lx.\n", hr);

    if (get_event((IMFMediaEventGenerator *)mediasource, MESourceStarted, &var))
        ok(var.vt == VT_EMPTY, "Unexpected value type.\n");

    hr = IMFMediaSource_Pause(mediasource);
    ok(hr == S_OK, "Failed to pause media source, hr %#lx.\n", hr);
    if (get_event((IMFMediaEventGenerator *)mediasource, MESourcePaused, &var))
        ok(var.vt == VT_EMPTY, "Unexpected value type.\n");

    var.vt = VT_I8;
    var.uhVal.QuadPart = 0;
    hr = IMFMediaSource_Start(mediasource, descriptor, &GUID_NULL, &var);
    ok(hr == S_OK, "Failed to start media source, hr %#lx.\n", hr);

    if (get_event((IMFMediaEventGenerator *)mediasource, MESourceSeeked, &var))
        ok(var.vt == VT_I8, "Unexpected value type.\n");

    hr = IMFMediaSource_Stop(mediasource);
    ok(hr == S_OK, "Failed to pause media source, hr %#lx.\n", hr);
    if (get_event((IMFMediaEventGenerator *)mediasource, MESourceStopped, &var))
        ok(var.vt == VT_EMPTY, "Unexpected value type.\n");

    var.vt = VT_I8;
    var.uhVal.QuadPart = 0;
    hr = IMFMediaSource_Start(mediasource, descriptor, &GUID_NULL, &var);
    ok(hr == S_OK, "Failed to start media source, hr %#lx.\n", hr);

    if (get_event((IMFMediaEventGenerator *)mediasource, MESourceStarted, &var))
        ok(var.vt == VT_I8, "Unexpected value type.\n");

    sample_count = 10;

    for (i = 0; i < sample_count; ++i)
    {
        hr = IMFMediaStream_RequestSample(video_stream, NULL);
        ok(hr == S_OK, "Failed to request sample %u, hr %#lx.\n", i + 1, hr);
        if (hr != S_OK)
            break;
    }

    for (i = 0; i < sample_count; ++i)
    {
        static const LONGLONG MILLI_TO_100_NANO = 10000;
        LONGLONG duration, time;
        DWORD buffer_count;
        IMFSample *sample;
        BOOL ret;

        ret = get_event((IMFMediaEventGenerator *)video_stream, MEMediaSample, &var);
        ok(ret, "Sample %u not received.\n", i + 1);
        if (!ret)
            break;

        ok(var.vt == VT_UNKNOWN, "Unexpected value type %u from MEMediaSample event.\n", var.vt);
        sample = (IMFSample *)var.punkVal;

        hr = IMFSample_GetBufferCount(sample, &buffer_count);
        ok(hr == S_OK, "Failed to get buffer count, hr %#lx.\n", hr);
        ok(buffer_count == 1, "Unexpected buffer count %lu.\n", buffer_count);

        hr = IMFSample_GetSampleDuration(sample, &duration);
        ok(hr == S_OK, "Failed to get sample duration, hr %#lx.\n", hr);
        ok(duration == 40 * MILLI_TO_100_NANO, "Unexpected duration %s.\n", wine_dbgstr_longlong(duration));

        hr = IMFSample_GetSampleTime(sample, &time);
        ok(hr == S_OK, "Failed to get sample time, hr %#lx.\n", hr);
        ok(time == i * 40 * MILLI_TO_100_NANO, "Unexpected time %s.\n", wine_dbgstr_longlong(time));

        IMFSample_Release(sample);
    }

    if (i == sample_count)
    {
        IMFMediaEvent *event;

        /* MEEndOfStream isn't queued until after a one request beyond the last frame is submitted */
        Sleep(100);
        hr = IMFMediaEventGenerator_GetEvent((IMFMediaEventGenerator *)video_stream, MF_EVENT_FLAG_NO_WAIT, &event);
        ok (hr == MF_E_NO_EVENTS_AVAILABLE, "Unexpected hr %#lx.\n", hr);

        hr = IMFMediaStream_RequestSample(video_stream, NULL);
        ok (hr == S_OK || hr == MF_E_END_OF_STREAM, "Unexpected hr %#lx.\n", hr);
        get_event((IMFMediaEventGenerator *)video_stream, MEEndOfStream, NULL);
    }


    hr = IMFMediaStream_RequestSample(video_stream, NULL);
    ok(hr == MF_E_END_OF_STREAM, "Unexpected hr %#lx.\n", hr);

    get_event((IMFMediaEventGenerator *)mediasource, MEEndOfPresentation, NULL);

    IMFMediaStream_Release(video_stream);
    IMFMediaTypeHandler_Release(handler);
    IMFPresentationDescriptor_Release(descriptor);

    ok(!bytestream_closed, "IMFByteStream::Close called unexpectedly\n");

    hr = IMFMediaSource_Shutdown(mediasource);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ok(bytestream_closed, "Missing IMFByteStream::Close call\n");

    hr = IMFMediaSource_CreatePresentationDescriptor(mediasource, NULL);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    IMFRateSupport_Release(rate_support);
    IMFGetService_Release(get_service);
    IMFMediaSource_Release(mediasource);
    IMFByteStream_Release(stream);

    /* Create directly through scheme handler. */
    hr = CoInitialize(NULL);
    ok(SUCCEEDED(hr), "Failed to initialize, hr %#lx.\n", hr);
    do_uninit = hr == S_OK;

    hr = CoCreateInstance(&CLSID_FileSchemePlugin, NULL, CLSCTX_INPROC_SERVER, &IID_IMFSchemeHandler,
            (void **)&scheme_handler);
    ok(hr == S_OK, "Failed to create handler object, hr %#lx.\n", hr);

    cancel_cookie = NULL;
    hr = IMFSchemeHandler_BeginCreateObject(scheme_handler, pathW, MF_RESOLUTION_MEDIASOURCE, NULL, &cancel_cookie,
            &callback2->IMFAsyncCallback_iface, (IUnknown *)scheme_handler);
    ok(hr == S_OK, "Create request failed, hr %#lx.\n", hr);
    ok(!!cancel_cookie, "Unexpected cancel object.\n");
    IUnknown_Release(cancel_cookie);

    WaitForSingleObject(callback2->event, INFINITE);

    IMFSchemeHandler_Release(scheme_handler);

    ret = DeleteFileW(filename);
    ok(ret, "Failed to delete %s, error %lu.\n", debugstr_w(filename), GetLastError());

    test_compressed_media_types(resolver);

    if (do_uninit)
        CoUninitialize();

    IMFSourceResolver_Release(resolver);

    hr = MFShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);

    IMFAsyncCallback_Release(&callback->IMFAsyncCallback_iface);
    IMFAsyncCallback_Release(&callback2->IMFAsyncCallback_iface);
}

static void init_functions(void)
{
    HMODULE mod = GetModuleHandleA("mfplat.dll");

#define X(f) p##f = (void*)GetProcAddress(mod, #f)
    X(MFAddPeriodicCallback);
    X(MFAllocateSerialWorkQueue);
    X(MFAllocateWorkQueueEx);
    X(MFCopyImage);
    X(MFCreate2DMediaBuffer);
    X(MFCreateLegacyMediaBufferOnMFMediaBuffer);
    X(MFCreateDXGIDeviceManager);
    X(MFCreateDXGISurfaceBuffer);
    X(MFCreateDXSurfaceBuffer);
    X(MFCreateSourceResolver);
    X(MFCreateMediaBufferFromMediaType);
    X(MFCreateMFByteStreamOnStream);
    X(MFCreatePathFromURL);
    X(MFCreateTrackedSample);
    X(MFCreateTransformActivate);
    X(MFCreateVideoMediaTypeFromSubtype);
    X(MFCreateVideoSampleAllocatorEx);
    X(MFGetPlaneSize);
    X(MFGetStrideForBitmapInfoHeader);
    X(MFInitVideoFormat_RGB);
    X(MFLockDXGIDeviceManager);
    X(MFLockSharedWorkQueue);
    X(MFMapDX9FormatToDXGIFormat);
    X(MFMapDXGIFormatToDX9Format);
    X(MFPutWaitingWorkItem);
    X(MFRegisterLocalByteStreamHandler);
    X(MFRegisterLocalSchemeHandler);
    X(MFRemovePeriodicCallback);
    X(MFTEnumEx);
    X(MFTRegisterLocal);
    X(MFTRegisterLocalByCLSID);
    X(MFTUnregisterLocal);
    X(MFTUnregisterLocalByCLSID);
    X(MFUnlockDXGIDeviceManager);

    if ((mod = LoadLibraryA("d3d11.dll")))
    {
        X(D3D11CreateDevice);
    }

    if ((mod = LoadLibraryA("d3d12.dll")))
    {
        X(D3D12CreateDevice);
    }

    mod = GetModuleHandleA("ole32.dll");

    X(CoGetApartmentType);

    if ((mod = LoadLibraryA("rtworkq.dll")))
    {
        X(RtwqStartup);
        X(RtwqShutdown);
        X(RtwqUnlockPlatform);
        X(RtwqLockPlatform);
    }
#undef X

    is_win8_plus = pMFPutWaitingWorkItem != NULL;
}

static void test_media_type(void)
{
    IMFMediaType *mediatype, *mediatype2;
    IMFVideoMediaType *video_type;
    IUnknown *unk, *unk2;
    BOOL compressed;
    DWORD flags;
    UINT count;
    HRESULT hr;
    GUID guid;

if(0)
{
    /* Crash on Windows Vista/7 */
    hr = MFCreateMediaType(NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
}

    hr = MFCreateMediaType(&mediatype);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaType_GetMajorType(mediatype, &guid);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    compressed = FALSE;
    hr = IMFMediaType_IsCompressedFormat(mediatype, &compressed);
    ok(hr == S_OK, "Failed to get media type property, hr %#lx.\n", hr);
    ok(compressed, "Unexpected value %d.\n", compressed);

    hr = IMFMediaType_SetUINT32(mediatype, &MF_MT_ALL_SAMPLES_INDEPENDENT, 0);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    compressed = FALSE;
    hr = IMFMediaType_IsCompressedFormat(mediatype, &compressed);
    ok(hr == S_OK, "Failed to get media type property, hr %#lx.\n", hr);
    ok(compressed, "Unexpected value %d.\n", compressed);

    hr = IMFMediaType_SetUINT32(mediatype, &MF_MT_COMPRESSED, 0);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    compressed = FALSE;
    hr = IMFMediaType_IsCompressedFormat(mediatype, &compressed);
    ok(hr == S_OK, "Failed to get media type property, hr %#lx.\n", hr);
    ok(compressed, "Unexpected value %d.\n", compressed);

    hr = IMFMediaType_SetUINT32(mediatype, &MF_MT_ALL_SAMPLES_INDEPENDENT, 1);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    hr = IMFMediaType_SetUINT32(mediatype, &MF_MT_COMPRESSED, 1);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    compressed = TRUE;
    hr = IMFMediaType_IsCompressedFormat(mediatype, &compressed);
    ok(hr == S_OK, "Failed to get media type property, hr %#lx.\n", hr);
    ok(!compressed, "Unexpected value %d.\n", compressed);

    hr = IMFMediaType_DeleteItem(mediatype, &MF_MT_COMPRESSED);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaType_SetGUID(mediatype, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Failed to set GUID value, hr %#lx.\n", hr);

    hr = IMFMediaType_GetMajorType(mediatype, &guid);
    ok(hr == S_OK, "Failed to get major type, hr %#lx.\n", hr);
    ok(IsEqualGUID(&guid, &MFMediaType_Video), "Unexpected major type.\n");

    /* IsEqual() */
    hr = MFCreateMediaType(&mediatype2);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);

    flags = 0xdeadbeef;
    hr = IMFMediaType_IsEqual(mediatype, mediatype2, &flags);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(flags == 0, "Unexpected flags %#lx.\n", flags);

    /* Different major types. */
    hr = IMFMediaType_SetGUID(mediatype2, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Failed to set major type, hr %#lx.\n", hr);

    flags = 0;
    hr = IMFMediaType_IsEqual(mediatype, mediatype2, &flags);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(flags == (MF_MEDIATYPE_EQUAL_FORMAT_TYPES | MF_MEDIATYPE_EQUAL_FORMAT_USER_DATA),
            "Unexpected flags %#lx.\n", flags);

    /* Same major types, different subtypes. */
    hr = IMFMediaType_SetGUID(mediatype2, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Failed to set major type, hr %#lx.\n", hr);

    flags = 0;
    hr = IMFMediaType_IsEqual(mediatype, mediatype2, &flags);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(flags == (MF_MEDIATYPE_EQUAL_MAJOR_TYPES | MF_MEDIATYPE_EQUAL_FORMAT_TYPES | MF_MEDIATYPE_EQUAL_FORMAT_DATA
            | MF_MEDIATYPE_EQUAL_FORMAT_USER_DATA), "Unexpected flags %#lx.\n", flags);

    /* Different user data. */
    hr = IMFMediaType_SetBlob(mediatype, &MF_MT_USER_DATA, (const UINT8 *)&flags, sizeof(flags));
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    flags = 0;
    hr = IMFMediaType_IsEqual(mediatype, mediatype2, &flags);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(flags == (MF_MEDIATYPE_EQUAL_MAJOR_TYPES | MF_MEDIATYPE_EQUAL_FORMAT_TYPES | MF_MEDIATYPE_EQUAL_FORMAT_DATA),
            "Unexpected flags %#lx.\n", flags);

    hr = IMFMediaType_DeleteItem(mediatype, &MF_MT_USER_DATA);
    ok(hr == S_OK, "Failed to delete item, hr %#lx.\n", hr);

    hr = IMFMediaType_SetGUID(mediatype, &MF_MT_SUBTYPE, &MFVideoFormat_RGB32);
    ok(hr == S_OK, "Failed to set subtype, hr %#lx.\n", hr);

    flags = 0;
    hr = IMFMediaType_IsEqual(mediatype, mediatype2, &flags);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
    ok(flags == (MF_MEDIATYPE_EQUAL_MAJOR_TYPES | MF_MEDIATYPE_EQUAL_FORMAT_DATA | MF_MEDIATYPE_EQUAL_FORMAT_USER_DATA),
            "Unexpected flags %#lx.\n", flags);

    IMFMediaType_Release(mediatype2);
    IMFMediaType_Release(mediatype);

    /* IMFVideoMediaType */
    hr = MFCreateMediaType(&mediatype);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    check_interface(mediatype, &IID_IMFVideoMediaType, FALSE);

    hr = IMFMediaType_QueryInterface(mediatype, &IID_IUnknown, (void **)&unk);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(unk == (IUnknown *)mediatype, "Unexpected pointer.\n");
    IUnknown_Release(unk);

    hr = IMFMediaType_SetGUID(mediatype, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Failed to set GUID value, hr %#lx.\n", hr);

    hr = IMFMediaType_QueryInterface(mediatype, &IID_IMFVideoMediaType, (void **)&unk);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IUnknown_QueryInterface(unk, &IID_IUnknown, (void **)&unk2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(unk2 == (IUnknown *)mediatype, "Unexpected pointer.\n");
    IUnknown_Release(unk2);

    hr = IUnknown_QueryInterface(unk, &IID_IMFAttributes, (void **)&unk2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(unk2 == (IUnknown *)mediatype, "Unexpected pointer.\n");
    IUnknown_Release(unk2);

    hr = IUnknown_QueryInterface(unk, &IID_IMFMediaType, (void **)&unk2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(unk2 == (IUnknown *)mediatype, "Unexpected pointer.\n");
    IUnknown_Release(unk2);

    IUnknown_Release(unk);
    IMFMediaType_Release(mediatype);

    if (pMFCreateVideoMediaTypeFromSubtype)
    {
        hr = pMFCreateVideoMediaTypeFromSubtype(&MFVideoFormat_RGB555, &video_type);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        check_interface(video_type, &IID_IMFMediaType, TRUE);
        check_interface(video_type, &IID_IMFVideoMediaType, TRUE);

        /* Major and subtype are set on creation. */
        hr = IMFVideoMediaType_GetCount(video_type, &count);
        ok(count == 2, "Unexpected attribute count %#lx.\n", hr);

        hr = IMFVideoMediaType_DeleteAllItems(video_type);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IMFVideoMediaType_GetCount(video_type, &count);
        ok(!count, "Unexpected attribute count %#lx.\n", hr);

        check_interface(video_type, &IID_IMFVideoMediaType, FALSE);

        IMFVideoMediaType_Release(video_type);
    }
    else
        win_skip("MFCreateVideoMediaTypeFromSubtype() is not available.\n");

    /* IMFAudioMediaType */
    hr = MFCreateMediaType(&mediatype);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    check_interface(mediatype, &IID_IMFAudioMediaType, FALSE);

    hr = IMFMediaType_QueryInterface(mediatype, &IID_IUnknown, (void **)&unk);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(unk == (IUnknown *)mediatype, "Unexpected pointer.\n");
    IUnknown_Release(unk);

    hr = IMFMediaType_SetGUID(mediatype, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Failed to set GUID value, hr %#lx.\n", hr);

    hr = IMFMediaType_QueryInterface(mediatype, &IID_IMFAudioMediaType, (void **)&unk);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IUnknown_QueryInterface(unk, &IID_IUnknown, (void **)&unk2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(unk2 == (IUnknown *)mediatype, "Unexpected pointer.\n");
    IUnknown_Release(unk2);

    hr = IUnknown_QueryInterface(unk, &IID_IMFAttributes, (void **)&unk2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(unk2 == (IUnknown *)mediatype, "Unexpected pointer.\n");
    IUnknown_Release(unk2);

    hr = IUnknown_QueryInterface(unk, &IID_IMFMediaType, (void **)&unk2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(unk2 == (IUnknown *)mediatype, "Unexpected pointer.\n");
    IUnknown_Release(unk2);

    IUnknown_Release(unk);

    IMFMediaType_Release(mediatype);
}

static void test_MFCreateMediaEvent(void)
{
    HRESULT hr;
    IMFMediaEvent *mediaevent;

    MediaEventType type;
    GUID extended_type;
    HRESULT status;
    PROPVARIANT value;

    PropVariantInit(&value);
    value.vt = VT_UNKNOWN;

    hr = MFCreateMediaEvent(MEError, &GUID_NULL, E_FAIL, &value, &mediaevent);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    PropVariantClear(&value);

    hr = IMFMediaEvent_GetType(mediaevent, &type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(type == MEError, "got %#lx\n", type);

    hr = IMFMediaEvent_GetExtendedType(mediaevent, &extended_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&extended_type, &GUID_NULL), "got %s\n",
       wine_dbgstr_guid(&extended_type));

    hr = IMFMediaEvent_GetStatus(mediaevent, &status);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(status == E_FAIL, "Unexpected hr %#lx.\n", status);

    PropVariantInit(&value);
    hr = IMFMediaEvent_GetValue(mediaevent, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value.vt == VT_UNKNOWN, "got %#x\n", value.vt);
    PropVariantClear(&value);

    IMFMediaEvent_Release(mediaevent);

    hr = MFCreateMediaEvent(MEUnknown, &DUMMY_GUID1, S_OK, NULL, &mediaevent);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaEvent_GetType(mediaevent, &type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(type == MEUnknown, "got %#lx\n", type);

    hr = IMFMediaEvent_GetExtendedType(mediaevent, &extended_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&extended_type, &DUMMY_GUID1), "got %s\n",
       wine_dbgstr_guid(&extended_type));

    hr = IMFMediaEvent_GetStatus(mediaevent, &status);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(status == S_OK, "Unexpected hr %#lx.\n", status);

    PropVariantInit(&value);
    hr = IMFMediaEvent_GetValue(mediaevent, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value.vt == VT_EMPTY, "got %#x\n", value.vt);
    PropVariantClear(&value);

    IMFMediaEvent_Release(mediaevent);
}

#define CHECK_ATTR_COUNT(obj, expected) check_attr_count(obj, expected, __LINE__)
static void check_attr_count(IMFAttributes* obj, UINT32 expected, int line)
{
    UINT32 count = expected + 1;
    HRESULT hr = IMFAttributes_GetCount(obj, &count);
    ok_(__FILE__, line)(hr == S_OK, "Failed to get attributes count, hr %#lx.\n", hr);
    ok_(__FILE__, line)(count == expected, "Unexpected count %u, expected %u.\n", count, expected);
}

#define CHECK_ATTR_TYPE(obj, key, expected) check_attr_type(obj, key, expected, __LINE__)
static void check_attr_type(IMFAttributes *obj, const GUID *key, MF_ATTRIBUTE_TYPE expected, int line)
{
    MF_ATTRIBUTE_TYPE type;
    HRESULT hr;

    hr = IMFAttributes_GetItemType(obj, key, &type);
    ok_(__FILE__, line)(hr == S_OK, "Failed to get item type, hr %#lx.\n", hr);
    ok_(__FILE__, line)(type == expected, "Unexpected item type %d, expected %d.\n", type, expected);
}

static void test_attributes(void)
{
    static const WCHAR stringW[] = L"Wine";
    static const UINT8 blob[] = {0,1,2,3,4,5};
    IMFAttributes *attributes, *attributes1;
    UINT8 blob_value[256], *blob_buf = NULL;
    MF_ATTRIBUTES_MATCH_TYPE match_type;
    UINT32 value, string_length, size;
    PROPVARIANT propvar, ret_propvar;
    MF_ATTRIBUTE_TYPE type;
    double double_value;
    IUnknown *unk_value;
    WCHAR bufferW[256];
    UINT64 value64;
    WCHAR *string;
    BOOL result;
    HRESULT hr;
    GUID key;

    hr = MFCreateAttributes( &attributes, 3 );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFAttributes_GetItemType(attributes, &GUID_NULL, &type);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    CHECK_ATTR_COUNT(attributes, 0);
    hr = IMFAttributes_SetUINT32(attributes, &MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, 123);
    ok(hr == S_OK, "Failed to set UINT32 value, hr %#lx.\n", hr);
    CHECK_ATTR_COUNT(attributes, 1);
    CHECK_ATTR_TYPE(attributes, &MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, MF_ATTRIBUTE_UINT32);

    value = 0xdeadbeef;
    hr = IMFAttributes_GetUINT32(attributes, &MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, &value);
    ok(hr == S_OK, "Failed to get UINT32 value, hr %#lx.\n", hr);
    ok(value == 123, "Unexpected value %u, expected: 123.\n", value);

    value64 = 0xdeadbeef;
    hr = IMFAttributes_GetUINT64(attributes, &MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, &value64);
    ok(hr == MF_E_INVALIDTYPE, "Unexpected hr %#lx.\n", hr);
    ok(value64 == 0xdeadbeef, "Unexpected value.\n");

    hr = IMFAttributes_SetUINT64(attributes, &MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, 65536);
    ok(hr == S_OK, "Failed to set UINT64 value, hr %#lx.\n", hr);
    CHECK_ATTR_COUNT(attributes, 1);
    CHECK_ATTR_TYPE(attributes, &MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, MF_ATTRIBUTE_UINT64);

    hr = IMFAttributes_GetUINT64(attributes, &MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, &value64);
    ok(hr == S_OK, "Failed to get UINT64 value, hr %#lx.\n", hr);
    ok(value64 == 65536, "Unexpected value.\n");

    value = 0xdeadbeef;
    hr = IMFAttributes_GetUINT32(attributes, &MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, &value);
    ok(hr == MF_E_INVALIDTYPE, "Unexpected hr %#lx.\n", hr);
    ok(value == 0xdeadbeef, "Unexpected value.\n");

    IMFAttributes_Release(attributes);

    hr = MFCreateAttributes(&attributes, 0);
    ok(hr == S_OK, "Failed to create attributes object, hr %#lx.\n", hr);

    PropVariantInit(&propvar);
    propvar.vt = MF_ATTRIBUTE_UINT32;
    propvar.ulVal = 123;
    hr = IMFAttributes_SetItem(attributes, &DUMMY_GUID1, &propvar);
    ok(hr == S_OK, "Failed to set item, hr %#lx.\n", hr);
    PropVariantInit(&ret_propvar);
    ret_propvar.vt = MF_ATTRIBUTE_UINT32;
    ret_propvar.ulVal = 0xdeadbeef;
    hr = IMFAttributes_GetItem(attributes, &DUMMY_GUID1, &ret_propvar);
    ok(hr == S_OK, "Failed to get item, hr %#lx.\n", hr);
    ok(!PropVariantCompareEx(&propvar, &ret_propvar, 0, 0), "Unexpected item value.\n");
    PropVariantClear(&ret_propvar);
    CHECK_ATTR_COUNT(attributes, 1);

    hr = IMFAttributes_GetItem(attributes, &DUMMY_GUID1, NULL);
    ok(hr == S_OK, "Item check failed, hr %#lx.\n", hr);

    hr = IMFAttributes_GetItem(attributes, &DUMMY_GUID2, NULL);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    PropVariantInit(&ret_propvar);
    ret_propvar.vt = MF_ATTRIBUTE_STRING;
    ret_propvar.pwszVal = NULL;
    hr = IMFAttributes_GetItem(attributes, &DUMMY_GUID1, &ret_propvar);
    ok(hr == S_OK, "Failed to get item, hr %#lx.\n", hr);
    ok(!PropVariantCompareEx(&propvar, &ret_propvar, 0, 0), "Unexpected item value.\n");
    PropVariantClear(&ret_propvar);

    PropVariantClear(&propvar);

    PropVariantInit(&propvar);
    propvar.vt = MF_ATTRIBUTE_UINT64;
    propvar.uhVal.QuadPart = 65536;
    hr = IMFAttributes_SetItem(attributes, &DUMMY_GUID1, &propvar);
    ok(hr == S_OK, "Failed to set item, hr %#lx.\n", hr);
    PropVariantInit(&ret_propvar);
    ret_propvar.vt = MF_ATTRIBUTE_UINT32;
    ret_propvar.ulVal = 0xdeadbeef;
    hr = IMFAttributes_GetItem(attributes, &DUMMY_GUID1, &ret_propvar);
    ok(hr == S_OK, "Failed to get item, hr %#lx.\n", hr);
    ok(!PropVariantCompareEx(&propvar, &ret_propvar, 0, 0), "Unexpected item value.\n");
    PropVariantClear(&ret_propvar);
    PropVariantClear(&propvar);
    CHECK_ATTR_COUNT(attributes, 1);

    PropVariantInit(&propvar);
    propvar.vt = VT_I4;
    propvar.lVal = 123;
    hr = IMFAttributes_SetItem(attributes, &DUMMY_GUID2, &propvar);
    ok(hr == MF_E_INVALIDTYPE, "Failed to set item, hr %#lx.\n", hr);
    PropVariantInit(&ret_propvar);
    ret_propvar.vt = MF_ATTRIBUTE_UINT32;
    ret_propvar.lVal = 0xdeadbeef;
    hr = IMFAttributes_GetItem(attributes, &DUMMY_GUID2, &ret_propvar);
    ok(hr == S_OK, "Failed to get item, hr %#lx.\n", hr);
    PropVariantClear(&propvar);
    ok(!PropVariantCompareEx(&propvar, &ret_propvar, 0, 0), "Unexpected item value.\n");
    PropVariantClear(&ret_propvar);

    PropVariantInit(&propvar);
    propvar.vt = MF_ATTRIBUTE_UINT32;
    propvar.ulVal = 123;
    hr = IMFAttributes_SetItem(attributes, &DUMMY_GUID3, &propvar);
    ok(hr == S_OK, "Failed to set item, hr %#lx.\n", hr);

    hr = IMFAttributes_DeleteItem(attributes, &DUMMY_GUID2);
    ok(hr == S_OK, "Failed to delete item, hr %#lx.\n", hr);
    CHECK_ATTR_COUNT(attributes, 2);

    hr = IMFAttributes_DeleteItem(attributes, &DUMMY_GUID2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    CHECK_ATTR_COUNT(attributes, 2);

    hr = IMFAttributes_GetItem(attributes, &DUMMY_GUID3, &ret_propvar);
    ok(hr == S_OK, "Failed to get item, hr %#lx.\n", hr);
    ok(!PropVariantCompareEx(&propvar, &ret_propvar, 0, 0), "Unexpected item value.\n");
    PropVariantClear(&ret_propvar);
    PropVariantClear(&propvar);

    propvar.vt = MF_ATTRIBUTE_UINT64;
    propvar.uhVal.QuadPart = 65536;

    hr = IMFAttributes_GetItem(attributes, &DUMMY_GUID1, &ret_propvar);
    ok(hr == S_OK, "Failed to get item, hr %#lx.\n", hr);
    ok(!PropVariantCompareEx(&propvar, &ret_propvar, 0, 0), "Unexpected item value.\n");
    PropVariantClear(&ret_propvar);
    PropVariantClear(&propvar);

    /* Item ordering is not consistent across Windows version. */
    hr = IMFAttributes_GetItemByIndex(attributes, 0, &key, &ret_propvar);
    ok(hr == S_OK, "Failed to get item, hr %#lx.\n", hr);
    PropVariantClear(&ret_propvar);

    hr = IMFAttributes_GetItemByIndex(attributes, 100, &key, &ret_propvar);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    PropVariantClear(&ret_propvar);

    hr = IMFAttributes_SetDouble(attributes, &GUID_NULL, 22.0);
    ok(hr == S_OK, "Failed to set double value, hr %#lx.\n", hr);
    CHECK_ATTR_COUNT(attributes, 3);
    CHECK_ATTR_TYPE(attributes, &GUID_NULL, MF_ATTRIBUTE_DOUBLE);

    double_value = 0xdeadbeef;
    hr = IMFAttributes_GetDouble(attributes, &GUID_NULL, &double_value);
    ok(hr == S_OK, "Failed to get double value, hr %#lx.\n", hr);
    ok(double_value == 22.0, "Unexpected value: %f, expected: 22.0.\n", double_value);

    propvar.vt = MF_ATTRIBUTE_UINT64;
    propvar.uhVal.QuadPart = 22;
    hr = IMFAttributes_CompareItem(attributes, &GUID_NULL, &propvar, &result);
    ok(hr == S_OK, "Failed to compare items, hr %#lx.\n", hr);
    ok(!result, "Unexpected result.\n");

    propvar.vt = MF_ATTRIBUTE_DOUBLE;
    propvar.dblVal = 22.0;
    hr = IMFAttributes_CompareItem(attributes, &GUID_NULL, &propvar, &result);
    ok(hr == S_OK, "Failed to compare items, hr %#lx.\n", hr);
    ok(result, "Unexpected result.\n");

    hr = IMFAttributes_SetString(attributes, &DUMMY_GUID1, stringW);
    ok(hr == S_OK, "Failed to set string attribute, hr %#lx.\n", hr);
    CHECK_ATTR_COUNT(attributes, 3);
    CHECK_ATTR_TYPE(attributes, &DUMMY_GUID1, MF_ATTRIBUTE_STRING);

    hr = IMFAttributes_GetStringLength(attributes, &DUMMY_GUID1, &string_length);
    ok(hr == S_OK, "Failed to get string length, hr %#lx.\n", hr);
    ok(string_length == lstrlenW(stringW), "Unexpected length %u.\n", string_length);

    hr = IMFAttributes_GetAllocatedString(attributes, &DUMMY_GUID1, &string, NULL);
    ok(hr == S_OK, "Failed to get allocated string, hr %#lx.\n", hr);
    ok(!lstrcmpW(string, stringW), "Unexpected string %s.\n", wine_dbgstr_w(string));
    CoTaskMemFree(string);

    string_length = 0xdeadbeef;
    hr = IMFAttributes_GetAllocatedString(attributes, &DUMMY_GUID1, &string, &string_length);
    ok(hr == S_OK, "Failed to get allocated string, hr %#lx.\n", hr);
    ok(!lstrcmpW(string, stringW), "Unexpected string %s.\n", wine_dbgstr_w(string));
    ok(string_length == lstrlenW(stringW), "Unexpected length %u.\n", string_length);
    CoTaskMemFree(string);

    string_length = 0xdeadbeef;
    hr = IMFAttributes_GetString(attributes, &DUMMY_GUID1, bufferW, ARRAY_SIZE(bufferW), &string_length);
    ok(hr == S_OK, "Failed to get string value, hr %#lx.\n", hr);
    ok(!lstrcmpW(bufferW, stringW), "Unexpected string %s.\n", wine_dbgstr_w(bufferW));
    ok(string_length == lstrlenW(stringW), "Unexpected length %u.\n", string_length);
    memset(bufferW, 0, sizeof(bufferW));

    hr = IMFAttributes_GetString(attributes, &DUMMY_GUID1, bufferW, ARRAY_SIZE(bufferW), NULL);
    ok(hr == S_OK, "Failed to get string value, hr %#lx.\n", hr);
    ok(!lstrcmpW(bufferW, stringW), "Unexpected string %s.\n", wine_dbgstr_w(bufferW));
    memset(bufferW, 0, sizeof(bufferW));

    string_length = 0;
    hr = IMFAttributes_GetString(attributes, &DUMMY_GUID1, bufferW, 1, &string_length);
    ok(hr == STRSAFE_E_INSUFFICIENT_BUFFER, "Unexpected hr %#lx.\n", hr);
    ok(!bufferW[0], "Unexpected string %s.\n", wine_dbgstr_w(bufferW));
    ok(string_length, "Unexpected length.\n");

    string_length = 0xdeadbeef;
    hr = IMFAttributes_GetStringLength(attributes, &GUID_NULL, &string_length);
    ok(hr == MF_E_INVALIDTYPE, "Unexpected hr %#lx.\n", hr);
    ok(string_length == 0xdeadbeef, "Unexpected length %u.\n", string_length);

    /* VT_UNKNOWN */
    hr = IMFAttributes_SetUnknown(attributes, &DUMMY_GUID2, (IUnknown *)attributes);
    ok(hr == S_OK, "Failed to set value, hr %#lx.\n", hr);
    CHECK_ATTR_COUNT(attributes, 4);
    CHECK_ATTR_TYPE(attributes, &DUMMY_GUID2, MF_ATTRIBUTE_IUNKNOWN);

    hr = IMFAttributes_GetUnknown(attributes, &DUMMY_GUID2, &IID_IUnknown, (void **)&unk_value);
    ok(hr == S_OK, "Failed to get value, hr %#lx.\n", hr);
    IUnknown_Release(unk_value);

    hr = IMFAttributes_GetUnknown(attributes, &DUMMY_GUID2, &IID_IMFAttributes, (void **)&unk_value);
    ok(hr == S_OK, "Failed to get value, hr %#lx.\n", hr);
    IUnknown_Release(unk_value);

    hr = IMFAttributes_GetUnknown(attributes, &DUMMY_GUID2, &IID_IStream, (void **)&unk_value);
    ok(hr == E_NOINTERFACE, "Unexpected hr %#lx.\n", hr);

    hr = IMFAttributes_SetUnknown(attributes, &DUMMY_CLSID, NULL);
    ok(hr == S_OK, "Failed to set value, hr %#lx.\n", hr);
    CHECK_ATTR_COUNT(attributes, 5);

    unk_value = NULL;
    hr = IMFAttributes_GetUnknown(attributes, &DUMMY_CLSID, &IID_IUnknown, (void **)&unk_value);
    ok(hr == MF_E_INVALIDTYPE, "Unexpected hr %#lx.\n", hr);

    /* CopyAllItems() */
    hr = MFCreateAttributes(&attributes1, 0);
    ok(hr == S_OK, "Failed to create attributes object, hr %#lx.\n", hr);
    hr = IMFAttributes_CopyAllItems(attributes, attributes1);
    ok(hr == S_OK, "Failed to copy items, hr %#lx.\n", hr);
    CHECK_ATTR_COUNT(attributes, 5);
    CHECK_ATTR_COUNT(attributes1, 5);

    hr = IMFAttributes_DeleteAllItems(attributes1);
    ok(hr == S_OK, "Failed to delete items, hr %#lx.\n", hr);
    CHECK_ATTR_COUNT(attributes1, 0);

    propvar.vt = MF_ATTRIBUTE_UINT64;
    propvar.uhVal.QuadPart = 22;
    hr = IMFAttributes_CompareItem(attributes, &GUID_NULL, &propvar, &result);
    ok(hr == S_OK, "Failed to compare items, hr %#lx.\n", hr);
    ok(!result, "Unexpected result.\n");

    hr = IMFAttributes_CopyAllItems(attributes1, attributes);
    ok(hr == S_OK, "Failed to copy items, hr %#lx.\n", hr);
    CHECK_ATTR_COUNT(attributes, 0);

    /* Blob */
    hr = IMFAttributes_SetBlob(attributes, &DUMMY_GUID1, blob, sizeof(blob));
    ok(hr == S_OK, "Failed to set blob attribute, hr %#lx.\n", hr);
    CHECK_ATTR_COUNT(attributes, 1);
    CHECK_ATTR_TYPE(attributes, &DUMMY_GUID1, MF_ATTRIBUTE_BLOB);
    hr = IMFAttributes_GetBlobSize(attributes, &DUMMY_GUID1, &size);
    ok(hr == S_OK, "Failed to get blob size, hr %#lx.\n", hr);
    ok(size == sizeof(blob), "Unexpected blob size %u.\n", size);

    hr = IMFAttributes_GetBlobSize(attributes, &DUMMY_GUID2, &size);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    size = 0;
    hr = IMFAttributes_GetBlob(attributes, &DUMMY_GUID1, blob_value, sizeof(blob_value), &size);
    ok(hr == S_OK, "Failed to get blob, hr %#lx.\n", hr);
    ok(size == sizeof(blob), "Unexpected blob size %u.\n", size);
    ok(!memcmp(blob_value, blob, size), "Unexpected blob.\n");

    hr = IMFAttributes_GetBlob(attributes, &DUMMY_GUID2, blob_value, sizeof(blob_value), &size);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    memset(blob_value, 0, sizeof(blob_value));
    size = 0;
    hr = IMFAttributes_GetAllocatedBlob(attributes, &DUMMY_GUID1, &blob_buf, &size);
    ok(hr == S_OK, "Failed to get allocated blob, hr %#lx.\n", hr);
    ok(size == sizeof(blob), "Unexpected blob size %u.\n", size);
    ok(!memcmp(blob_buf, blob, size), "Unexpected blob.\n");
    CoTaskMemFree(blob_buf);

    hr = IMFAttributes_GetAllocatedBlob(attributes, &DUMMY_GUID1, &blob_buf, NULL);
    ok(hr == S_OK, "Failed to get allocated blob, hr %#lx.\n", hr);
    ok(!memcmp(blob_buf, blob, size), "Unexpected blob.\n");
    CoTaskMemFree(blob_buf);

    hr = IMFAttributes_GetAllocatedBlob(attributes, &DUMMY_GUID2, &blob_buf, &size);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    hr = IMFAttributes_GetBlob(attributes, &DUMMY_GUID1, blob_value, sizeof(blob) - 1, NULL);
    ok(hr == E_NOT_SUFFICIENT_BUFFER, "Unexpected hr %#lx.\n", hr);

    IMFAttributes_Release(attributes);
    IMFAttributes_Release(attributes1);

    /* Compare() */
    hr = MFCreateAttributes(&attributes, 0);
    ok(hr == S_OK, "Failed to create attributes object, hr %#lx.\n", hr);
    hr = MFCreateAttributes(&attributes1, 0);
    ok(hr == S_OK, "Failed to create attributes object, hr %#lx.\n", hr);

    hr = IMFAttributes_Compare(attributes, attributes, MF_ATTRIBUTES_MATCH_SMALLER + 1, &result);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    for (match_type = MF_ATTRIBUTES_MATCH_OUR_ITEMS; match_type <= MF_ATTRIBUTES_MATCH_SMALLER; ++match_type)
    {
        result = FALSE;
        hr = IMFAttributes_Compare(attributes, attributes, match_type, &result);
        ok(hr == S_OK, "Failed to compare, hr %#lx.\n", hr);
        ok(result, "Unexpected result %d.\n", result);

        result = FALSE;
        hr = IMFAttributes_Compare(attributes, attributes1, match_type, &result);
        ok(hr == S_OK, "Failed to compare, hr %#lx.\n", hr);
        ok(result, "Unexpected result %d.\n", result);
    }

    hr = IMFAttributes_SetUINT32(attributes, &DUMMY_GUID1, 1);
    ok(hr == S_OK, "Failed to set value, hr %#lx.\n", hr);

    result = TRUE;
    hr = IMFAttributes_Compare(attributes, attributes1, MF_ATTRIBUTES_MATCH_OUR_ITEMS, &result);
    ok(hr == S_OK, "Failed to compare, hr %#lx.\n", hr);
    ok(!result, "Unexpected result %d.\n", result);

    result = TRUE;
    hr = IMFAttributes_Compare(attributes, attributes1, MF_ATTRIBUTES_MATCH_ALL_ITEMS, &result);
    ok(hr == S_OK, "Failed to compare, hr %#lx.\n", hr);
    ok(!result, "Unexpected result %d.\n", result);

    result = FALSE;
    hr = IMFAttributes_Compare(attributes, attributes1, MF_ATTRIBUTES_MATCH_INTERSECTION, &result);
    ok(hr == S_OK, "Failed to compare, hr %#lx.\n", hr);
    ok(result, "Unexpected result %d.\n", result);

    result = FALSE;
    hr = IMFAttributes_Compare(attributes, attributes1, MF_ATTRIBUTES_MATCH_SMALLER, &result);
    ok(hr == S_OK, "Failed to compare, hr %#lx.\n", hr);
    ok(result, "Unexpected result %d.\n", result);

    hr = IMFAttributes_SetUINT32(attributes1, &DUMMY_GUID1, 2);
    ok(hr == S_OK, "Failed to set value, hr %#lx.\n", hr);

    result = TRUE;
    hr = IMFAttributes_Compare(attributes, attributes1, MF_ATTRIBUTES_MATCH_ALL_ITEMS, &result);
    ok(hr == S_OK, "Failed to compare, hr %#lx.\n", hr);
    ok(!result, "Unexpected result %d.\n", result);

    result = TRUE;
    hr = IMFAttributes_Compare(attributes, attributes1, MF_ATTRIBUTES_MATCH_OUR_ITEMS, &result);
    ok(hr == S_OK, "Failed to compare, hr %#lx.\n", hr);
    ok(!result, "Unexpected result %d.\n", result);

    result = TRUE;
    hr = IMFAttributes_Compare(attributes, attributes1, MF_ATTRIBUTES_MATCH_THEIR_ITEMS, &result);
    ok(hr == S_OK, "Failed to compare, hr %#lx.\n", hr);
    ok(!result, "Unexpected result %d.\n", result);

    result = TRUE;
    hr = IMFAttributes_Compare(attributes, attributes1, MF_ATTRIBUTES_MATCH_INTERSECTION, &result);
    ok(hr == S_OK, "Failed to compare, hr %#lx.\n", hr);
    ok(!result, "Unexpected result %d.\n", result);

    result = TRUE;
    hr = IMFAttributes_Compare(attributes, attributes1, MF_ATTRIBUTES_MATCH_SMALLER, &result);
    ok(hr == S_OK, "Failed to compare, hr %#lx.\n", hr);
    ok(!result, "Unexpected result %d.\n", result);

    result = TRUE;
    hr = IMFAttributes_Compare(attributes1, attributes, MF_ATTRIBUTES_MATCH_ALL_ITEMS, &result);
    ok(hr == S_OK, "Failed to compare, hr %#lx.\n", hr);
    ok(!result, "Unexpected result %d.\n", result);

    result = TRUE;
    hr = IMFAttributes_Compare(attributes1, attributes, MF_ATTRIBUTES_MATCH_OUR_ITEMS, &result);
    ok(hr == S_OK, "Failed to compare, hr %#lx.\n", hr);
    ok(!result, "Unexpected result %d.\n", result);

    result = TRUE;
    hr = IMFAttributes_Compare(attributes1, attributes, MF_ATTRIBUTES_MATCH_THEIR_ITEMS, &result);
    ok(hr == S_OK, "Failed to compare, hr %#lx.\n", hr);
    ok(!result, "Unexpected result %d.\n", result);

    result = TRUE;
    hr = IMFAttributes_Compare(attributes1, attributes, MF_ATTRIBUTES_MATCH_INTERSECTION, &result);
    ok(hr == S_OK, "Failed to compare, hr %#lx.\n", hr);
    ok(!result, "Unexpected result %d.\n", result);

    result = TRUE;
    hr = IMFAttributes_Compare(attributes1, attributes, MF_ATTRIBUTES_MATCH_SMALLER, &result);
    ok(hr == S_OK, "Failed to compare, hr %#lx.\n", hr);
    ok(!result, "Unexpected result %d.\n", result);

    hr = IMFAttributes_SetUINT32(attributes1, &DUMMY_GUID1, 1);
    ok(hr == S_OK, "Failed to set value, hr %#lx.\n", hr);

    result = FALSE;
    hr = IMFAttributes_Compare(attributes, attributes1, MF_ATTRIBUTES_MATCH_ALL_ITEMS, &result);
    ok(hr == S_OK, "Failed to compare, hr %#lx.\n", hr);
    ok(result, "Unexpected result %d.\n", result);

    result = FALSE;
    hr = IMFAttributes_Compare(attributes, attributes1, MF_ATTRIBUTES_MATCH_THEIR_ITEMS, &result);
    ok(hr == S_OK, "Failed to compare, hr %#lx.\n", hr);
    ok(result, "Unexpected result %d.\n", result);

    result = FALSE;
    hr = IMFAttributes_Compare(attributes, attributes1, MF_ATTRIBUTES_MATCH_INTERSECTION, &result);
    ok(hr == S_OK, "Failed to compare, hr %#lx.\n", hr);
    ok(result, "Unexpected result %d.\n", result);

    result = FALSE;
    hr = IMFAttributes_Compare(attributes, attributes1, MF_ATTRIBUTES_MATCH_SMALLER, &result);
    ok(hr == S_OK, "Failed to compare, hr %#lx.\n", hr);
    ok(result, "Unexpected result %d.\n", result);

    result = FALSE;
    hr = IMFAttributes_Compare(attributes1, attributes, MF_ATTRIBUTES_MATCH_ALL_ITEMS, &result);
    ok(hr == S_OK, "Failed to compare, hr %#lx.\n", hr);
    ok(result, "Unexpected result %d.\n", result);

    result = FALSE;
    hr = IMFAttributes_Compare(attributes1, attributes, MF_ATTRIBUTES_MATCH_THEIR_ITEMS, &result);
    ok(hr == S_OK, "Failed to compare, hr %#lx.\n", hr);
    ok(result, "Unexpected result %d.\n", result);

    result = FALSE;
    hr = IMFAttributes_Compare(attributes1, attributes, MF_ATTRIBUTES_MATCH_INTERSECTION, &result);
    ok(hr == S_OK, "Failed to compare, hr %#lx.\n", hr);
    ok(result, "Unexpected result %d.\n", result);

    result = FALSE;
    hr = IMFAttributes_Compare(attributes1, attributes, MF_ATTRIBUTES_MATCH_SMALLER, &result);
    ok(hr == S_OK, "Failed to compare, hr %#lx.\n", hr);
    ok(result, "Unexpected result %d.\n", result);

    hr = IMFAttributes_SetUINT32(attributes1, &DUMMY_GUID2, 2);
    ok(hr == S_OK, "Failed to set value, hr %#lx.\n", hr);

    result = TRUE;
    hr = IMFAttributes_Compare(attributes, attributes1, MF_ATTRIBUTES_MATCH_ALL_ITEMS, &result);
    ok(hr == S_OK, "Failed to compare, hr %#lx.\n", hr);
    ok(!result, "Unexpected result %d.\n", result);

    result = TRUE;
    hr = IMFAttributes_Compare(attributes, attributes1, MF_ATTRIBUTES_MATCH_THEIR_ITEMS, &result);
    ok(hr == S_OK, "Failed to compare, hr %#lx.\n", hr);
    ok(!result, "Unexpected result %d.\n", result);

    result = FALSE;
    hr = IMFAttributes_Compare(attributes, attributes1, MF_ATTRIBUTES_MATCH_OUR_ITEMS, &result);
    ok(hr == S_OK, "Failed to compare, hr %#lx.\n", hr);
    ok(result, "Unexpected result %d.\n", result);

    result = FALSE;
    hr = IMFAttributes_Compare(attributes, attributes1, MF_ATTRIBUTES_MATCH_INTERSECTION, &result);
    ok(hr == S_OK, "Failed to compare, hr %#lx.\n", hr);
    ok(result, "Unexpected result %d.\n", result);

    result = FALSE;
    hr = IMFAttributes_Compare(attributes, attributes1, MF_ATTRIBUTES_MATCH_SMALLER, &result);
    ok(hr == S_OK, "Failed to compare, hr %#lx.\n", hr);
    ok(result, "Unexpected result %d.\n", result);

    result = TRUE;
    hr = IMFAttributes_Compare(attributes1, attributes, MF_ATTRIBUTES_MATCH_ALL_ITEMS, &result);
    ok(hr == S_OK, "Failed to compare, hr %#lx.\n", hr);
    ok(!result, "Unexpected result %d.\n", result);

    result = FALSE;
    hr = IMFAttributes_Compare(attributes1, attributes, MF_ATTRIBUTES_MATCH_THEIR_ITEMS, &result);
    ok(hr == S_OK, "Failed to compare, hr %#lx.\n", hr);
    ok(result, "Unexpected result %d.\n", result);

    result = TRUE;
    hr = IMFAttributes_Compare(attributes1, attributes, MF_ATTRIBUTES_MATCH_OUR_ITEMS, &result);
    ok(hr == S_OK, "Failed to compare, hr %#lx.\n", hr);
    ok(!result, "Unexpected result %d.\n", result);

    result = FALSE;
    hr = IMFAttributes_Compare(attributes1, attributes, MF_ATTRIBUTES_MATCH_INTERSECTION, &result);
    ok(hr == S_OK, "Failed to compare, hr %#lx.\n", hr);
    ok(result, "Unexpected result %d.\n", result);

    result = FALSE;
    hr = IMFAttributes_Compare(attributes1, attributes, MF_ATTRIBUTES_MATCH_SMALLER, &result);
    ok(hr == S_OK, "Failed to compare, hr %#lx.\n", hr);
    ok(result, "Unexpected result %d.\n", result);

    IMFAttributes_Release(attributes);
    IMFAttributes_Release(attributes1);
}

struct test_stream
{
    IStream IStream_iface;
    LONG refcount;

    HANDLE read_event;
    HANDLE done_event;
};

static struct test_stream *impl_from_IStream(IStream *iface)
{
    return CONTAINING_RECORD(iface, struct test_stream, IStream_iface);
}

static HRESULT WINAPI test_stream_QueryInterface(IStream *iface, REFIID iid, void **out)
{
    if (IsEqualIID(iid, &IID_IUnknown)
            || IsEqualIID(iid, &IID_IStream))
    {
        *out = iface;
        IStream_AddRef(iface);
        return S_OK;
    }

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI test_stream_AddRef(IStream *iface)
{
    struct test_stream *stream = impl_from_IStream(iface);
    return InterlockedIncrement(&stream->refcount);
}

static ULONG WINAPI test_stream_Release(IStream *iface)
{
    struct test_stream *stream = impl_from_IStream(iface);
    ULONG ref = InterlockedDecrement(&stream->refcount);

    if (!ref)
    {
        CloseHandle(stream->read_event);
        CloseHandle(stream->done_event);
        free(stream);
    }

    return ref;
}

static HRESULT WINAPI test_stream_Read(IStream *iface, void *data, ULONG size, ULONG *ret_size)
{
    struct test_stream *stream = impl_from_IStream(iface);
    DWORD res;

    SetEvent(stream->read_event);
    res = WaitForSingleObject(stream->done_event, 1000);
    ok(res == 0, "got %#lx\n", res);

    *ret_size = size;
    return S_OK;
}

static HRESULT WINAPI test_stream_Write(IStream *iface, const void *data, ULONG size, ULONG *ret_size)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_stream_Seek(IStream *iface, LARGE_INTEGER offset, DWORD method, ULARGE_INTEGER *ret_offset)
{
    return S_OK;
}

static HRESULT WINAPI test_stream_SetSize(IStream *iface, ULARGE_INTEGER size)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_stream_CopyTo(IStream *iface, IStream *dest, ULARGE_INTEGER size,
        ULARGE_INTEGER *read_size, ULARGE_INTEGER *write_size)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_stream_Commit(IStream *iface, DWORD flags)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_stream_Revert(IStream *iface)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_stream_LockRegion(IStream *iface, ULARGE_INTEGER offset, ULARGE_INTEGER size, DWORD type)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_stream_UnlockRegion(IStream *iface, ULARGE_INTEGER offset, ULARGE_INTEGER size, DWORD type)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_stream_Stat(IStream *iface, STATSTG *stat, DWORD flags)
{
    memset(stat, 0, sizeof(STATSTG));
    stat->pwcsName = NULL;
    stat->type = STGTY_STREAM;
    stat->cbSize.QuadPart = 16;
    return S_OK;
}

static HRESULT WINAPI test_stream_Clone(IStream *iface, IStream **out)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static const IStreamVtbl test_stream_vtbl =
{
    test_stream_QueryInterface,
    test_stream_AddRef,
    test_stream_Release,
    test_stream_Read,
    test_stream_Write,
    test_stream_Seek,
    test_stream_SetSize,
    test_stream_CopyTo,
    test_stream_Commit,
    test_stream_Revert,
    test_stream_LockRegion,
    test_stream_UnlockRegion,
    test_stream_Stat,
    test_stream_Clone,
};

static HRESULT test_stream_create(IStream **out)
{
    struct test_stream *stream;

    if (!(stream = calloc(1, sizeof(*stream))))
        return E_OUTOFMEMORY;

    stream->IStream_iface.lpVtbl = &test_stream_vtbl;
    stream->refcount = 1;

    stream->read_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    stream->done_event = CreateEventW(NULL, FALSE, FALSE, NULL);

    *out = &stream->IStream_iface;
    return S_OK;
}

static DWORD test_stream_wait_read(IStream *iface, DWORD timeout)
{
    struct test_stream *stream = impl_from_IStream(iface);
    return WaitForSingleObject(stream->read_event, timeout);
}

static void test_stream_complete_read(IStream *iface)
{
    struct test_stream *stream = impl_from_IStream(iface);
    SetEvent(stream->done_event);
}

static void test_MFCreateMFByteStreamOnStream(void)
{
    struct test_callback *read_callback, *test_callback;
    IMFByteStream *bytestream;
    IMFByteStream *bytestream2;
    IStream *stream;
    IMFAttributes *attributes = NULL;
    IMFAsyncResult *result;
    DWORD caps, written;
    IUnknown *unknown;
    BYTE buffer[16];
    ULONG ref, size;
    DWORD res, len;
    HRESULT hr;
    UINT count;
    QWORD to;

    if(!pMFCreateMFByteStreamOnStream)
    {
        win_skip("MFCreateMFByteStreamOnStream() not found\n");
        return;
    }

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    caps = 0xffff0000;
    hr = IStream_Write(stream, &caps, sizeof(caps), &written);
    ok(hr == S_OK, "Failed to write, hr %#lx.\n", hr);

    hr = pMFCreateMFByteStreamOnStream(stream, &bytestream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFByteStream_QueryInterface(bytestream, &IID_IUnknown,
                                 (void **)&unknown);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok((void *)unknown == (void *)bytestream, "got %p\n", unknown);
    ref = IUnknown_Release(unknown);
    ok(ref == 1, "got %lu\n", ref);

    hr = IUnknown_QueryInterface(unknown, &IID_IMFByteStream,
                                 (void **)&bytestream2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(bytestream2 == bytestream, "got %p\n", bytestream2);
    ref = IMFByteStream_Release(bytestream2);
    ok(ref == 1, "got %lu\n", ref);

    hr = IMFByteStream_QueryInterface(bytestream, &IID_IMFAttributes,
                                 (void **)&attributes);
    ok(hr == S_OK ||
       /* w7pro64 */
       broken(hr == E_NOINTERFACE), "Unexpected hr %#lx.\n", hr);

    if (hr != S_OK)
    {
        win_skip("Cannot retrieve IMFAttributes interface from IMFByteStream\n");
        IStream_Release(stream);
        IMFByteStream_Release(bytestream);
        return;
    }

    ok(attributes != NULL, "got NULL\n");
    hr = IMFAttributes_GetCount(attributes, &count);
    ok(hr == S_OK, "Failed to get attributes count, hr %#lx.\n", hr);
    ok(count == 0, "Unexpected attributes count %u.\n", count);

    hr = IMFAttributes_QueryInterface(attributes, &IID_IUnknown,
                                 (void **)&unknown);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok((void *)unknown == (void *)bytestream, "got %p\n", unknown);
    ref = IUnknown_Release(unknown);
    ok(ref == 2, "got %lu\n", ref);

    hr = IMFAttributes_QueryInterface(attributes, &IID_IMFByteStream,
                                 (void **)&bytestream2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(bytestream2 == bytestream, "got %p\n", bytestream2);
    ref = IMFByteStream_Release(bytestream2);
    ok(ref == 2, "got %lu\n", ref);

    check_interface(bytestream, &IID_IMFByteStreamBuffering, FALSE);
    check_interface(bytestream, &IID_IMFByteStreamCacheControl, FALSE);
    check_interface(bytestream, &IID_IMFMediaEventGenerator, FALSE);
    check_interface(bytestream, &IID_IMFGetService, FALSE);

    hr = IMFByteStream_GetCapabilities(bytestream, &caps);
    ok(hr == S_OK, "Failed to get stream capabilities, hr %#lx.\n", hr);
    ok(caps == (MFBYTESTREAM_IS_READABLE | MFBYTESTREAM_IS_SEEKABLE), "Unexpected caps %#lx.\n", caps);

    hr = IMFByteStream_Close(bytestream);
    ok(hr == S_OK, "Failed to close, hr %#lx.\n", hr);

    hr = IMFByteStream_Close(bytestream);
    ok(hr == S_OK, "Failed to close, hr %#lx.\n", hr);

    hr = IMFByteStream_GetCapabilities(bytestream, &caps);
    ok(hr == S_OK, "Failed to get stream capabilities, hr %#lx.\n", hr);
    ok(caps == (MFBYTESTREAM_IS_READABLE | MFBYTESTREAM_IS_SEEKABLE), "Unexpected caps %#lx.\n", caps);

    /* IMFByteStream maintains position separately from IStream */
    caps = 0;
    hr = IStream_Read(stream, &caps, sizeof(caps), &size);
    ok(hr == S_OK, "Failed to read from raw stream, hr %#lx.\n", hr);
    ok(size == 4, "Unexpected size.\n");
    ok(caps == 0xffff0000, "Unexpected content.\n");

    caps = 0;
    hr = IMFByteStream_Read(bytestream, (BYTE *)&caps, sizeof(caps), &size);
    ok(hr == S_OK, "Failed to read from stream, hr %#lx.\n", hr);
    ok(size == 4, "Unexpected size.\n");
    ok(caps == 0xffff0000, "Unexpected content.\n");

    caps = 0;
    hr = IStream_Read(stream, &caps, sizeof(caps), &size);
    ok(hr == S_OK, "Failed to read from raw stream, hr %#lx.\n", hr);
    ok(size == 0, "Unexpected size.\n");
    ok(caps == 0, "Unexpected content.\n");

    hr = IMFByteStream_Seek(bytestream, msoBegin, 0, 0, &to);
    ok(hr == S_OK, "Failed to read from stream, hr %#lx.\n", hr);

    hr = IStream_Read(stream, &caps, sizeof(caps), &size);
    ok(hr == S_OK, "Failed to read from raw stream, hr %#lx.\n", hr);
    ok(size == 0, "Unexpected size.\n");
    ok(caps == 0, "Unexpected content.\n");

    caps = 0;
    hr = IMFByteStream_Read(bytestream, (BYTE *)&caps, sizeof(caps), &size);
    ok(hr == S_OK, "Failed to read from stream, hr %#lx.\n", hr);
    ok(size == 4, "Unexpected size.\n");
    ok(caps == 0xffff0000, "Unexpected content.\n");

    caps = 0;
    hr = IMFByteStream_Read(bytestream, (BYTE *)&caps, sizeof(caps), &size);
    ok(hr == S_OK, "Failed to read from stream, hr %#lx.\n", hr);
    ok(size == 0, "Unexpected size.\n");
    ok(caps == 0, "Unexpected content.\n");

    IMFAttributes_Release(attributes);
    IMFByteStream_Release(bytestream);
    IStream_Release(stream);


    /* test that BeginRead doesn't use MFASYNC_CALLBACK_QUEUE_STANDARD */

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    read_callback = create_test_callback(&test_async_callback_result_vtbl);
    test_callback = create_test_callback(&test_async_callback_result_vtbl);

    hr = test_stream_create(&stream);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = pMFCreateMFByteStreamOnStream(stream, &bytestream);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IMFByteStream_BeginRead(bytestream, buffer, 1, &read_callback->IMFAsyncCallback_iface, NULL);
    ok(hr == S_OK, "got %#lx\n", hr);

    res = test_stream_wait_read(stream, 1000);
    ok(res == 0, "got %#lx\n", res);
    res = wait_async_callback_result(&read_callback->IMFAsyncCallback_iface, 10, &result);
    ok(res == WAIT_TIMEOUT, "got %#lx\n", res);

    /* MFASYNC_CALLBACK_QUEUE_STANDARD is not blocked */
    hr = MFPutWorkItem(MFASYNC_CALLBACK_QUEUE_STANDARD, &test_callback->IMFAsyncCallback_iface, NULL);
    ok(hr == S_OK, "got %#lx\n", hr);
    res = wait_async_callback_result(&test_callback->IMFAsyncCallback_iface, 100, &result);
    ok(res == 0, "got %#lx\n", res);
    IMFAsyncResult_Release(result);

    test_stream_complete_read(stream);
    res = wait_async_callback_result(&read_callback->IMFAsyncCallback_iface, 1000, &result);
    ok(res == 0, "got %#lx\n", res);
    hr = IMFByteStream_EndRead(bytestream, result, &len);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(len == 1, "got %#lx\n", len);
    IMFAsyncResult_Release(result);

    IMFByteStream_Release(bytestream);
    IStream_Release(stream);

    IMFAsyncCallback_Release(&read_callback->IMFAsyncCallback_iface);
    IMFAsyncCallback_Release(&test_callback->IMFAsyncCallback_iface);

    hr = MFShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);
}

static void test_file_stream(void)
{
    static const WCHAR newfilename[] = L"new.mp4";
    IMFByteStream *bytestream, *bytestream2;
    QWORD bytestream_length, position;
    IMFAttributes *attributes = NULL;
    MF_ATTRIBUTE_TYPE item_type;
    WCHAR pathW[MAX_PATH];
    WCHAR *filename;
    HRESULT hr;
    WCHAR *str;
    DWORD caps;
    UINT count;
    BOOL eos;

    filename = load_resource(L"test.mp4");

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateFile(MF_ACCESSMODE_READ, MF_OPENMODE_FAIL_IF_NOT_EXIST,
                      MF_FILEFLAGS_NONE, filename, &bytestream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    check_interface(bytestream, &IID_IMFByteStreamBuffering, FALSE);
    check_interface(bytestream, &IID_IMFByteStreamCacheControl, FALSE);
    check_interface(bytestream, &IID_IMFMediaEventGenerator, FALSE);
    check_interface(bytestream, &IID_IMFGetService, TRUE);

    hr = IMFByteStream_GetCapabilities(bytestream, &caps);
    ok(hr == S_OK, "Failed to get stream capabilities, hr %#lx.\n", hr);
    if (is_win8_plus)
    {
        ok(caps == (MFBYTESTREAM_IS_READABLE | MFBYTESTREAM_IS_SEEKABLE | MFBYTESTREAM_DOES_NOT_USE_NETWORK),
            "Unexpected caps %#lx.\n", caps);
    }
    else
        ok(caps == (MFBYTESTREAM_IS_READABLE | MFBYTESTREAM_IS_SEEKABLE), "Unexpected caps %#lx.\n", caps);

    hr = IMFByteStream_QueryInterface(bytestream, &IID_IMFAttributes,
                                 (void **)&attributes);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(attributes != NULL, "got NULL\n");

    hr = IMFAttributes_GetCount(attributes, &count);
    ok(hr == S_OK, "Failed to get attributes count, hr %#lx.\n", hr);
    ok(count == 2, "Unexpected attributes count %u.\n", count);

    /* Original file name. */
    hr = IMFAttributes_GetAllocatedString(attributes, &MF_BYTESTREAM_ORIGIN_NAME, &str, &count);
    ok(hr == S_OK, "Failed to get attribute, hr %#lx.\n", hr);
    ok(!lstrcmpW(str, filename), "Unexpected name %s.\n", wine_dbgstr_w(str));
    CoTaskMemFree(str);

    /* Modification time. */
    hr = IMFAttributes_GetItemType(attributes, &MF_BYTESTREAM_LAST_MODIFIED_TIME, &item_type);
    ok(hr == S_OK, "Failed to get item type, hr %#lx.\n", hr);
    ok(item_type == MF_ATTRIBUTE_BLOB, "Unexpected item type.\n");

    IMFAttributes_Release(attributes);

    /* Length. */
    hr = IMFByteStream_GetLength(bytestream, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    bytestream_length = 0;
    hr = IMFByteStream_GetLength(bytestream, &bytestream_length);
    ok(hr == S_OK, "Failed to get bytestream length, hr %#lx.\n", hr);
    ok(bytestream_length > 0, "Unexpected bytestream length %s.\n", wine_dbgstr_longlong(bytestream_length));

    hr = IMFByteStream_SetCurrentPosition(bytestream, bytestream_length);
    ok(hr == S_OK, "Failed to set bytestream position, hr %#lx.\n", hr);

    hr = IMFByteStream_IsEndOfStream(bytestream, &eos);
    ok(hr == S_OK, "Failed query end of stream, hr %#lx.\n", hr);
    ok(eos == TRUE, "Unexpected IsEndOfStream result, %u.\n", eos);

    hr = IMFByteStream_SetCurrentPosition(bytestream, 2 * bytestream_length);
    ok(hr == S_OK, "Failed to set bytestream position, hr %#lx.\n", hr);

    hr = IMFByteStream_GetCurrentPosition(bytestream, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IMFByteStream_GetCurrentPosition(bytestream, &position);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(position == 2 * bytestream_length, "Unexpected position.\n");

    hr = MFCreateFile(MF_ACCESSMODE_READ, MF_OPENMODE_FAIL_IF_NOT_EXIST,
                      MF_FILEFLAGS_NONE, filename, &bytestream2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFByteStream_Release(bytestream2);

    hr = MFCreateFile(MF_ACCESSMODE_WRITE, MF_OPENMODE_FAIL_IF_NOT_EXIST, MF_FILEFLAGS_NONE, filename, &bytestream2);
    ok(hr == HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION), "Unexpected hr %#lx.\n", hr);

    hr = MFCreateFile(MF_ACCESSMODE_READWRITE, MF_OPENMODE_FAIL_IF_NOT_EXIST, MF_FILEFLAGS_NONE, filename, &bytestream2);
    ok(hr == HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION), "Unexpected hr %#lx.\n", hr);

    IMFByteStream_Release(bytestream);

    hr = MFCreateFile(MF_ACCESSMODE_READ, MF_OPENMODE_FAIL_IF_NOT_EXIST,
                      MF_FILEFLAGS_NONE, newfilename, &bytestream);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), "Unexpected hr %#lx.\n", hr);

    hr = MFCreateFile(MF_ACCESSMODE_WRITE, MF_OPENMODE_FAIL_IF_EXIST,
                      MF_FILEFLAGS_NONE, filename, &bytestream);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_EXISTS), "Unexpected hr %#lx.\n", hr);

    hr = MFCreateFile(MF_ACCESSMODE_WRITE, MF_OPENMODE_FAIL_IF_EXIST,
                      MF_FILEFLAGS_NONE, newfilename, &bytestream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateFile(MF_ACCESSMODE_READ, MF_OPENMODE_FAIL_IF_NOT_EXIST, MF_FILEFLAGS_NONE, newfilename, &bytestream2);
    ok(hr == HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION), "Unexpected hr %#lx.\n", hr);

    hr = MFCreateFile(MF_ACCESSMODE_WRITE, MF_OPENMODE_FAIL_IF_NOT_EXIST, MF_FILEFLAGS_NONE, newfilename, &bytestream2);
    ok(hr == HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION), "Unexpected hr %#lx.\n", hr);

    hr = MFCreateFile(MF_ACCESSMODE_WRITE, MF_OPENMODE_FAIL_IF_NOT_EXIST, MF_FILEFLAGS_ALLOW_WRITE_SHARING,
            newfilename, &bytestream2);
    ok(hr == HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION), "Unexpected hr %#lx.\n", hr);

    IMFByteStream_Release(bytestream);

    hr = MFCreateFile(MF_ACCESSMODE_WRITE, MF_OPENMODE_FAIL_IF_NOT_EXIST,
                      MF_FILEFLAGS_ALLOW_WRITE_SHARING, newfilename, &bytestream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Opening the file again fails even though MF_FILEFLAGS_ALLOW_WRITE_SHARING is set. */
    hr = MFCreateFile(MF_ACCESSMODE_WRITE, MF_OPENMODE_FAIL_IF_NOT_EXIST, MF_FILEFLAGS_ALLOW_WRITE_SHARING,
            newfilename, &bytestream2);
    ok(hr == HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION), "Unexpected hr %#lx.\n", hr);

    IMFByteStream_Release(bytestream);

    /* Explicit file: scheme */
    lstrcpyW(pathW, fileschemeW);
    lstrcatW(pathW, filename);
    hr = MFCreateFile(MF_ACCESSMODE_READ, MF_OPENMODE_FAIL_IF_NOT_EXIST, MF_FILEFLAGS_NONE, pathW, &bytestream);
    ok(FAILED(hr), "Unexpected hr %#lx.\n", hr);

    hr = MFCreateFile(MF_ACCESSMODE_READ, MF_OPENMODE_FAIL_IF_NOT_EXIST, MF_FILEFLAGS_NONE, filename, &bytestream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(DeleteFileW(filename), "failed to delete file\n");
    IMFByteStream_Release(bytestream);

    hr = MFShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);

    DeleteFileW(newfilename);
}

static void test_system_memory_buffer(void)
{
    IMFMediaBuffer *buffer;
    HRESULT hr;
    DWORD length, max;
    BYTE *data, *data2;

    hr = MFCreateMemoryBuffer(1024, NULL);
    ok(hr == E_INVALIDARG || hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateMemoryBuffer(0, &buffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if(buffer)
    {
        hr = IMFMediaBuffer_GetMaxLength(buffer, &length);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(length == 0, "got %lu\n", length);

        IMFMediaBuffer_Release(buffer);
    }

    hr = MFCreateMemoryBuffer(1024, &buffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    check_interface(buffer, &IID_IMFGetService, FALSE);

    hr = IMFMediaBuffer_GetMaxLength(buffer, NULL);
    ok(hr == E_INVALIDARG || hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaBuffer_GetMaxLength(buffer, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(length == 1024, "got %lu\n", length);

    hr = IMFMediaBuffer_SetCurrentLength(buffer, 1025);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaBuffer_SetCurrentLength(buffer, 10);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaBuffer_GetCurrentLength(buffer, NULL);
    ok(hr == E_INVALIDARG || hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaBuffer_GetCurrentLength(buffer, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(length == 10, "got %lu\n", length);

    length = 0;
    max = 0;
    hr = IMFMediaBuffer_Lock(buffer, NULL, &length, &max);
    ok(hr == E_INVALIDARG || hr == E_POINTER, "Unexpected hr %#lx.\n", hr);
    ok(length == 0, "got %lu\n", length);
    ok(max == 0, "got %lu\n", length);

    hr = IMFMediaBuffer_Lock(buffer, &data, &max, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(length == 10, "got %lu\n", length);
    ok(max == 1024, "got %lu\n", max);

    /* Attempt to lock the buffer twice */
    hr = IMFMediaBuffer_Lock(buffer, &data2, &max, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(data == data2, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaBuffer_Unlock(buffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaBuffer_Unlock(buffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaBuffer_Lock(buffer, &data, NULL, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaBuffer_Unlock(buffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Extra Unlock */
    hr = IMFMediaBuffer_Unlock(buffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IMFMediaBuffer_Release(buffer);
}

static void test_system_memory_aligned_buffer(void)
{
    static const DWORD alignments[] =
    {
        MF_16_BYTE_ALIGNMENT,
        MF_32_BYTE_ALIGNMENT,
        MF_64_BYTE_ALIGNMENT,
        MF_128_BYTE_ALIGNMENT,
        MF_256_BYTE_ALIGNMENT,
        MF_512_BYTE_ALIGNMENT,
    };
    IMFMediaBuffer *buffer;
    DWORD length, max;
    unsigned int i;
    BYTE *data;
    HRESULT hr;

    hr = MFCreateAlignedMemoryBuffer(16, MF_8_BYTE_ALIGNMENT, NULL);
    ok(FAILED(hr), "Unexpected hr %#lx.\n", hr);

    hr = MFCreateAlignedMemoryBuffer(201, MF_8_BYTE_ALIGNMENT, &buffer);
    ok(hr == S_OK, "Failed to create memory buffer, hr %#lx.\n", hr);

    hr = IMFMediaBuffer_GetCurrentLength(buffer, &length);
    ok(hr == S_OK, "Failed to get current length, hr %#lx.\n", hr);
    ok(length == 0, "Unexpected current length %lu.\n", length);

    hr = IMFMediaBuffer_SetCurrentLength(buffer, 1);
    ok(hr == S_OK, "Failed to set current length, hr %#lx.\n", hr);
    hr = IMFMediaBuffer_GetCurrentLength(buffer, &length);
    ok(hr == S_OK, "Failed to get current length, hr %#lx.\n", hr);
    ok(length == 1, "Unexpected current length %lu.\n", length);

    hr = IMFMediaBuffer_GetMaxLength(buffer, &length);
    ok(hr == S_OK, "Failed to get max length, hr %#lx.\n", hr);
    ok(length == 201, "Unexpected max length %lu.\n", length);

    hr = IMFMediaBuffer_SetCurrentLength(buffer, 202);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaBuffer_GetMaxLength(buffer, &length);
    ok(hr == S_OK, "Failed to get max length, hr %#lx.\n", hr);
    ok(length == 201, "Unexpected max length %lu.\n", length);
    hr = IMFMediaBuffer_SetCurrentLength(buffer, 10);
    ok(hr == S_OK, "Failed to set current length, hr %#lx.\n", hr);

    hr = IMFMediaBuffer_Lock(buffer, &data, &max, &length);
    ok(hr == S_OK, "Failed to lock, hr %#lx.\n", hr);
    ok(max == 201 && length == 10, "Unexpected length.\n");
    hr = IMFMediaBuffer_Unlock(buffer);
    ok(hr == S_OK, "Failed to unlock, hr %#lx.\n", hr);

    IMFMediaBuffer_Release(buffer);

    for (i = 0; i < ARRAY_SIZE(alignments); ++i)
    {
        hr = MFCreateAlignedMemoryBuffer(200, alignments[i], &buffer);
        ok(hr == S_OK, "Failed to create memory buffer, hr %#lx.\n", hr);

        hr = IMFMediaBuffer_Lock(buffer, &data, &max, &length);
        ok(hr == S_OK, "Failed to lock, hr %#lx.\n", hr);
        ok(max == 200 && !length, "Unexpected length.\n");
        ok(!((uintptr_t)data & alignments[i]), "Data at %p is misaligned.\n", data);
        hr = IMFMediaBuffer_Unlock(buffer);
        ok(hr == S_OK, "Failed to unlock, hr %#lx.\n", hr);

        IMFMediaBuffer_Release(buffer);
    }

    hr = MFCreateAlignedMemoryBuffer(200, 0, &buffer);
    ok(hr == S_OK, "Failed to create memory buffer, hr %#lx.\n", hr);
    IMFMediaBuffer_Release(buffer);
}

static UINT expect_locks;
static UINT expect_unlocks;

static HRESULT WINAPI test_buffer_QueryInterface(IMFMediaBuffer *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IMFMediaBuffer))
    {
        IMFMediaBuffer_AddRef(iface);
        *obj = iface;
        return S_OK;
    }

    ok(IsEqualIID(riid, &IID_IMF2DBuffer) || IsEqualIID(riid, &IID_IMF2DBuffer2),
            "Unexpected riid %s\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI test_buffer_AddRef(IMFMediaBuffer *iface)
{
    return 2;
}

static ULONG WINAPI test_buffer_Release(IMFMediaBuffer *iface)
{
    return 1;
}

static HRESULT WINAPI test_buffer_Lock(IMFMediaBuffer *iface, BYTE **data, DWORD *maxlength, DWORD *length)
{
    ok(expect_locks--, "Unexpected call\n");
    ok(!maxlength, "got maxlength\n");
    todo_wine ok(!length, "got length\n");
    *data = (BYTE *)0xdeadbeef;
    return S_OK;
}

static HRESULT WINAPI test_buffer_Unlock(IMFMediaBuffer *iface)
{
    ok(expect_unlocks--, "Unexpected call\n");
    return S_OK;
}

static HRESULT WINAPI test_buffer_GetCurrentLength(IMFMediaBuffer *iface, DWORD *length)
{
    *length = 0;
    return S_OK;
}

static HRESULT WINAPI test_buffer_SetCurrentLength(IMFMediaBuffer *iface, DWORD length)
{
    ok(0, "Unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_buffer_GetMaxLength(IMFMediaBuffer *iface, DWORD *length)
{
    *length = 0;
    return S_OK;
}

static const IMFMediaBufferVtbl test_buffer_vtbl =
{
    test_buffer_QueryInterface,
    test_buffer_AddRef,
    test_buffer_Release,
    test_buffer_Lock,
    test_buffer_Unlock,
    test_buffer_GetCurrentLength,
    test_buffer_SetCurrentLength,
    test_buffer_GetMaxLength,
};

static IMFMediaBuffer test_buffer = {.lpVtbl = &test_buffer_vtbl};

static void test_MFCreateLegacyMediaBufferOnMFMediaBuffer(void)
{
    IMediaBuffer *media_buffer;
    IMFMediaBuffer *buffer;
    IMFSample *sample;
    ULONG length;
    BYTE *data;
    HRESULT hr;

    if (!pMFCreateLegacyMediaBufferOnMFMediaBuffer)
    {
        win_skip("MFCreateLegacyMediaBufferOnMFMediaBuffer() is not available.\n");
        return;
    }

    hr = pMFCreateLegacyMediaBufferOnMFMediaBuffer(NULL, NULL, 0, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    if (0) /* crashes */
    {
        hr = MFCreateSample(&sample);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        hr = pMFCreateLegacyMediaBufferOnMFMediaBuffer(sample, NULL, 0, &media_buffer);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        IMFSample_Release(sample);
    }

    hr = pMFCreateLegacyMediaBufferOnMFMediaBuffer(NULL, &test_buffer, 0, &media_buffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    expect_locks = 1;
    hr = IMediaBuffer_GetBufferAndLength(media_buffer, &data, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(expect_locks == 0, "Unexpected Lock calls\n");

    expect_locks = 1;
    hr = IMediaBuffer_GetBufferAndLength(media_buffer, &data, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(expect_locks == 1, "Unexpected Lock calls\n");

    expect_unlocks = 1;
    IMediaBuffer_Release(media_buffer);
    ok(expect_unlocks == 0, "Unexpected Unlock calls\n");

    hr = MFCreateMemoryBuffer(0, &buffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = pMFCreateLegacyMediaBufferOnMFMediaBuffer(NULL, buffer, 0, &media_buffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFMediaBuffer_Release(buffer);
    check_interface(media_buffer, &IID_IMFMediaBuffer, FALSE);
    check_interface(media_buffer, &IID_IMFGetService, FALSE);
    check_interface(media_buffer, &IID_IMF2DBuffer, FALSE);
    check_interface(media_buffer, &IID_IMFSample, FALSE);
    IMediaBuffer_Release(media_buffer);

    hr = MFCreateSample(&sample);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFCreateMemoryBuffer(0, &buffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = pMFCreateLegacyMediaBufferOnMFMediaBuffer(sample, buffer, 0, &media_buffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFMediaBuffer_Release(buffer);
    IMFSample_Release(sample);
    check_interface(media_buffer, &IID_IMFSample, TRUE);
    IMediaBuffer_Release(media_buffer);

    if (pMFCreate2DMediaBuffer)
    {
        hr = MFCreateSample(&sample);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        hr = pMFCreate2DMediaBuffer(1, 1, MAKEFOURCC('N','V','1','2'), FALSE, &buffer);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        hr = pMFCreateLegacyMediaBufferOnMFMediaBuffer(sample, buffer, 0, &media_buffer);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        IMFMediaBuffer_Release(buffer);
        IMFSample_Release(sample);
        check_interface(media_buffer, &IID_IMFMediaBuffer, FALSE);
        check_interface(media_buffer, &IID_IMF2DBuffer, TRUE);
        check_interface(media_buffer, &IID_IMF2DBuffer2, TRUE);
        check_interface(media_buffer, &IID_IMFSample, TRUE);
        IMediaBuffer_Release(media_buffer);
    }
}

static void test_sample(void)
{
    static const DWORD test_pattern = 0x22222222;
    IMFMediaBuffer *buffer, *buffer2, *buffer3;
    DWORD count, flags, length;
    IMFAttributes *attributes;
    IMFSample *sample;
    LONGLONG time;
    HRESULT hr;
    BYTE *data;

    hr = MFCreateSample( &sample );
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFSample_QueryInterface(sample, &IID_IMFAttributes, (void **)&attributes);
    ok(hr == S_OK, "Failed to get attributes interface, hr %#lx.\n", hr);

    CHECK_ATTR_COUNT(attributes, 0);

    hr = IMFSample_GetBufferCount(sample, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IMFSample_GetBufferCount(sample, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 0, "got %ld\n", count);

    hr = IMFSample_GetSampleFlags(sample, &flags);
    ok(hr == S_OK, "Failed to get sample flags, hr %#lx.\n", hr);
    ok(!flags, "Unexpected flags %#lx.\n", flags);

    hr = IMFSample_SetSampleFlags(sample, 0x123);
    ok(hr == S_OK, "Failed to set sample flags, hr %#lx.\n", hr);
    hr = IMFSample_GetSampleFlags(sample, &flags);
    ok(hr == S_OK, "Failed to get sample flags, hr %#lx.\n", hr);
    ok(flags == 0x123, "Unexpected flags %#lx.\n", flags);

    hr = IMFSample_GetSampleTime(sample, &time);
    ok(hr == MF_E_NO_SAMPLE_TIMESTAMP, "Unexpected hr %#lx.\n", hr);

    hr = IMFSample_GetSampleDuration(sample, &time);
    ok(hr == MF_E_NO_SAMPLE_DURATION, "Unexpected hr %#lx.\n", hr);

    hr = IMFSample_GetBufferByIndex(sample, 0, &buffer);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IMFSample_RemoveBufferByIndex(sample, 0);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IMFSample_RemoveAllBuffers(sample);
    ok(hr == S_OK, "Failed to remove all, hr %#lx.\n", hr);

    hr = IMFSample_GetTotalLength(sample, &length);
    ok(hr == S_OK, "Failed to get total length, hr %#lx.\n", hr);
    ok(!length, "Unexpected total length %lu.\n", length);

    hr = MFCreateMemoryBuffer(16, &buffer);
    ok(hr == S_OK, "Failed to create buffer, hr %#lx.\n", hr);

    hr = IMFSample_AddBuffer(sample, buffer);
    ok(hr == S_OK, "Failed to add buffer, hr %#lx.\n", hr);

    hr = IMFSample_AddBuffer(sample, buffer);
    ok(hr == S_OK, "Failed to add buffer, hr %#lx.\n", hr);

    hr = IMFSample_GetBufferCount(sample, &count);
    ok(hr == S_OK, "Failed to get buffer count, hr %#lx.\n", hr);
    ok(count == 2, "Unexpected buffer count %lu.\n", count);

    hr = IMFSample_GetBufferByIndex(sample, 0, &buffer2);
    ok(hr == S_OK, "Failed to get buffer, hr %#lx.\n", hr);
    ok(buffer2 == buffer, "Unexpected object.\n");
    IMFMediaBuffer_Release(buffer2);

    hr = IMFSample_GetTotalLength(sample, &length);
    ok(hr == S_OK, "Failed to get total length, hr %#lx.\n", hr);
    ok(!length, "Unexpected total length %lu.\n", length);

    hr = IMFMediaBuffer_SetCurrentLength(buffer, 2);
    ok(hr == S_OK, "Failed to set current length, hr %#lx.\n", hr);

    hr = IMFSample_GetTotalLength(sample, &length);
    ok(hr == S_OK, "Failed to get total length, hr %#lx.\n", hr);
    ok(length == 4, "Unexpected total length %lu.\n", length);

    hr = IMFSample_RemoveBufferByIndex(sample, 1);
    ok(hr == S_OK, "Failed to remove buffer, hr %#lx.\n", hr);

    hr = IMFSample_GetTotalLength(sample, &length);
    ok(hr == S_OK, "Failed to get total length, hr %#lx.\n", hr);
    ok(length == 2, "Unexpected total length %lu.\n", length);

    IMFMediaBuffer_Release(buffer);

    /* Duration */
    hr = IMFSample_SetSampleDuration(sample, 10);
    ok(hr == S_OK, "Failed to set duration, hr %#lx.\n", hr);
    CHECK_ATTR_COUNT(attributes, 0);
    hr = IMFSample_GetSampleDuration(sample, &time);
    ok(hr == S_OK, "Failed to get sample duration, hr %#lx.\n", hr);
    ok(time == 10, "Unexpected duration.\n");

    /* Timestamp */
    hr = IMFSample_SetSampleTime(sample, 1);
    ok(hr == S_OK, "Failed to set timestamp, hr %#lx.\n", hr);
    CHECK_ATTR_COUNT(attributes, 0);
    hr = IMFSample_GetSampleTime(sample, &time);
    ok(hr == S_OK, "Failed to get sample time, hr %#lx.\n", hr);
    ok(time == 1, "Unexpected timestamp.\n");

    IMFAttributes_Release(attributes);
    IMFSample_Release(sample);

    /* CopyToBuffer() */
    hr = MFCreateSample(&sample);
    ok(hr == S_OK, "Failed to create a sample, hr %#lx.\n", hr);

    hr = MFCreateMemoryBuffer(16, &buffer2);
    ok(hr == S_OK, "Failed to create a buffer, hr %#lx.\n", hr);

    /* Sample with no buffers. */
    hr = IMFMediaBuffer_SetCurrentLength(buffer2, 1);
    ok(hr == S_OK, "Failed to set current length, hr %#lx.\n", hr);
    hr = IMFSample_CopyToBuffer(sample, buffer2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaBuffer_GetCurrentLength(buffer2, &length);
    ok(hr == S_OK, "Failed to get current length, hr %#lx.\n", hr);
    ok(!length, "Unexpected length %lu.\n", length);

    /* Single buffer, larger destination. */
    hr = MFCreateMemoryBuffer(8, &buffer);
    ok(hr == S_OK, "Failed to create a buffer, hr %#lx.\n", hr);

    hr = IMFMediaBuffer_Lock(buffer, &data, NULL, NULL);
    ok(hr == S_OK, "Failed to lock buffer, hr %#lx.\n", hr);
    *(DWORD *)data = 0x11111111;
    hr = IMFMediaBuffer_Unlock(buffer);
    ok(hr == S_OK, "Failed to unlock, hr %#lx.\n", hr);
    hr = IMFMediaBuffer_SetCurrentLength(buffer, 4);
    ok(hr == S_OK, "Failed to set current length, hr %#lx.\n", hr);

    hr = IMFSample_AddBuffer(sample, buffer);
    ok(hr == S_OK, "Failed to add buffer, hr %#lx.\n", hr);

    /* Existing content is overwritten. */
    hr = IMFMediaBuffer_SetCurrentLength(buffer2, 8);
    ok(hr == S_OK, "Failed to set length, hr %#lx.\n", hr);

    hr = IMFSample_CopyToBuffer(sample, buffer2);
    ok(hr == S_OK, "Failed to copy to buffer, hr %#lx.\n", hr);

    hr = IMFMediaBuffer_GetCurrentLength(buffer2, &length);
    ok(hr == S_OK, "Failed to get length, hr %#lx.\n", hr);
    ok(length == 4, "Unexpected buffer length %lu.\n", length);

    /* Multiple buffers, matching total size. */
    hr = IMFSample_AddBuffer(sample, buffer);
    ok(hr == S_OK, "Failed to add buffer, hr %#lx.\n", hr);

    hr = IMFSample_GetBufferCount(sample, &count);
    ok(hr == S_OK, "Failed to get buffer count, hr %#lx.\n", hr);
    ok(count == 2, "Unexpected buffer count %lu.\n", count);

    hr = IMFMediaBuffer_SetCurrentLength(buffer, 8);
    ok(hr == S_OK, "Failed to set current length, hr %#lx.\n", hr);

    hr = IMFSample_CopyToBuffer(sample, buffer2);
    ok(hr == S_OK, "Failed to copy to buffer, hr %#lx.\n", hr);

    hr = IMFMediaBuffer_GetCurrentLength(buffer2, &length);
    ok(hr == S_OK, "Failed to get length, hr %#lx.\n", hr);
    ok(length == 16, "Unexpected buffer length %lu.\n", length);

    hr = IMFSample_AddBuffer(sample, buffer);
    ok(hr == S_OK, "Failed to add buffer, hr %#lx.\n", hr);

    hr = IMFMediaBuffer_SetCurrentLength(buffer2, 1);
    ok(hr == S_OK, "Failed to set buffer length, hr %#lx.\n", hr);

    hr = IMFMediaBuffer_Lock(buffer2, &data, NULL, NULL);
    ok(hr == S_OK, "Failed to lock buffer, hr %#lx.\n", hr);
    *(DWORD *)data = test_pattern;
    hr = IMFMediaBuffer_Unlock(buffer2);
    ok(hr == S_OK, "Failed to unlock buffer, hr %#lx.\n", hr);

    hr = IMFSample_CopyToBuffer(sample, buffer2);
    ok(hr == MF_E_BUFFERTOOSMALL, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaBuffer_Lock(buffer2, &data, NULL, NULL);
    ok(hr == S_OK, "Failed to lock buffer, hr %#lx.\n", hr);
    ok(!memcmp(data, &test_pattern, sizeof(test_pattern)), "Unexpected contents, %#lx\n", *(DWORD *)data);
    hr = IMFMediaBuffer_Unlock(buffer2);
    ok(hr == S_OK, "Failed to unlock buffer, hr %#lx.\n", hr);

    hr = IMFMediaBuffer_GetCurrentLength(buffer2, &length);
    ok(hr == S_OK, "Failed to get length, hr %#lx.\n", hr);
    ok(!length, "Unexpected buffer length %lu.\n", length);

    IMFMediaBuffer_Release(buffer2);
    IMFSample_Release(sample);

    /* ConvertToContiguousBuffer() */
    hr = MFCreateSample(&sample);
    ok(hr == S_OK, "Failed to create a sample, hr %#lx.\n", hr);

    hr = IMFSample_ConvertToContiguousBuffer(sample, &buffer);
    ok(hr == E_UNEXPECTED, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateMemoryBuffer(16, &buffer);
    ok(hr == S_OK, "Failed to create a buffer, hr %#lx.\n", hr);

    hr = IMFSample_AddBuffer(sample, buffer);
    ok(hr == S_OK, "Failed to add buffer, hr %#lx.\n", hr);

    hr = IMFSample_ConvertToContiguousBuffer(sample, &buffer2);
    ok(hr == S_OK, "Failed to convert, hr %#lx.\n", hr);
    ok(buffer2 == buffer, "Unexpected buffer instance.\n");
    IMFMediaBuffer_Release(buffer2);

    hr = IMFSample_ConvertToContiguousBuffer(sample, NULL);
    ok(hr == S_OK, "Failed to convert, hr %#lx.\n", hr);

    hr = IMFSample_ConvertToContiguousBuffer(sample, &buffer2);
    ok(hr == S_OK, "Failed to convert, hr %#lx.\n", hr);
    ok(buffer2 == buffer, "Unexpected buffer instance.\n");
    IMFMediaBuffer_Release(buffer2);

    hr = IMFMediaBuffer_SetCurrentLength(buffer, 3);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateMemoryBuffer(16, &buffer2);
    ok(hr == S_OK, "Failed to create a buffer, hr %#lx.\n", hr);

    hr = IMFMediaBuffer_SetCurrentLength(buffer2, 4);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFSample_AddBuffer(sample, buffer2);
    ok(hr == S_OK, "Failed to add buffer, hr %#lx.\n", hr);
    IMFMediaBuffer_Release(buffer2);

    hr = IMFSample_GetBufferCount(sample, &count);
    ok(hr == S_OK, "Failed to get buffer count, hr %#lx.\n", hr);
    ok(count == 2, "Unexpected buffer count %lu.\n", count);

    hr = IMFSample_ConvertToContiguousBuffer(sample, &buffer3);
    ok(hr == S_OK, "Failed to convert, hr %#lx.\n", hr);

    hr = IMFMediaBuffer_GetMaxLength(buffer3, &length);
    ok(hr == S_OK, "Failed to get maximum length, hr %#lx.\n", hr);
    ok(length == 7, "Unexpected length %lu.\n", length);

    hr = IMFMediaBuffer_GetCurrentLength(buffer3, &length);
    ok(hr == S_OK, "Failed to get maximum length, hr %#lx.\n", hr);
    ok(length == 7, "Unexpected length %lu.\n", length);

    IMFMediaBuffer_Release(buffer3);

    hr = IMFSample_GetBufferCount(sample, &count);
    ok(hr == S_OK, "Failed to get buffer count, hr %#lx.\n", hr);
    ok(count == 1, "Unexpected buffer count %lu.\n", count);

    hr = IMFSample_AddBuffer(sample, buffer);
    ok(hr == S_OK, "Failed to add buffer, hr %#lx.\n", hr);

    hr = IMFSample_GetBufferCount(sample, &count);
    ok(hr == S_OK, "Failed to get buffer count, hr %#lx.\n", hr);
    ok(count == 2, "Unexpected buffer count %lu.\n", count);

    hr = IMFSample_ConvertToContiguousBuffer(sample, NULL);
    ok(hr == S_OK, "Failed to convert, hr %#lx.\n", hr);

    hr = IMFSample_GetBufferCount(sample, &count);
    ok(hr == S_OK, "Failed to get buffer count, hr %#lx.\n", hr);
    ok(count == 1, "Unexpected buffer count %lu.\n", count);

    IMFMediaBuffer_Release(buffer);

    IMFSample_Release(sample);
}

static HRESULT WINAPI testcallback_Invoke(IMFAsyncCallback *iface, IMFAsyncResult *result)
{
    struct test_callback *callback = impl_from_IMFAsyncCallback(iface);
    IMFMediaEventQueue *queue;
    IUnknown *state, *obj;
    HRESULT hr;

    ok(result != NULL, "Unexpected result object.\n");

    state = IMFAsyncResult_GetStateNoAddRef(result);
    if (state && SUCCEEDED(IUnknown_QueryInterface(state, &IID_IMFMediaEventQueue, (void **)&queue)))
    {
        IMFMediaEvent *event = NULL, *event2;

        if (is_win8_plus)
        {
            hr = IMFMediaEventQueue_GetEvent(queue, MF_EVENT_FLAG_NO_WAIT, &event);
            ok(hr == MF_E_MULTIPLE_SUBSCRIBERS, "Failed to get event, hr %#lx.\n", hr);

            hr = IMFMediaEventQueue_GetEvent(queue, 0, &event);
            ok(hr == MF_E_MULTIPLE_SUBSCRIBERS, "Failed to get event, hr %#lx.\n", hr);

            hr = IMFMediaEventQueue_EndGetEvent(queue, result, &event);
            ok(hr == S_OK, "Failed to finalize GetEvent, hr %#lx.\n", hr);

            hr = IMFMediaEventQueue_EndGetEvent(queue, result, &event2);
            ok(hr == E_FAIL, "Unexpected result, hr %#lx.\n", hr);

            if (event)
                IMFMediaEvent_Release(event);
        }

        hr = IMFAsyncResult_GetObject(result, &obj);
        ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

        IMFMediaEventQueue_Release(queue);

        SetEvent(callback->event);
    }

    return E_NOTIMPL;
}

static const IMFAsyncCallbackVtbl testcallbackvtbl =
{
    testcallback_QueryInterface,
    testcallback_AddRef,
    testcallback_Release,
    testcallback_GetParameters,
    testcallback_Invoke,
};

static void test_MFCreateAsyncResult(void)
{
    IMFAsyncResult *result, *result2;
    struct test_callback *callback;
    IUnknown *state, *object;
    MFASYNCRESULT *data;
    ULONG refcount;
    HANDLE event;
    DWORD flags;
    HRESULT hr;
    BOOL ret;

    callback = create_test_callback(NULL);

    hr = MFCreateAsyncResult(NULL, NULL, NULL, NULL);
    ok(FAILED(hr), "Unexpected hr %#lx.\n", hr);

    hr = MFCreateAsyncResult(NULL, NULL, NULL, &result);
    ok(hr == S_OK, "Failed to create object, hr %#lx.\n", hr);

    data = (MFASYNCRESULT *)result;
    ok(data->pCallback == NULL, "Unexpected callback value.\n");
    ok(data->hrStatusResult == S_OK, "Unexpected status %#lx.\n", data->hrStatusResult);
    ok(data->dwBytesTransferred == 0, "Unexpected byte length %lu.\n", data->dwBytesTransferred);
    ok(data->hEvent == NULL, "Unexpected event.\n");

    hr = IMFAsyncResult_GetState(result, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    state = (void *)0xdeadbeef;
    hr = IMFAsyncResult_GetState(result, &state);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);
    ok(state == (void *)0xdeadbeef, "Unexpected state.\n");

    hr = IMFAsyncResult_GetStatus(result);
    ok(hr == S_OK, "Unexpected status %#lx.\n", hr);

    data->hrStatusResult = 123;
    hr = IMFAsyncResult_GetStatus(result);
    ok(hr == 123, "Unexpected status %#lx.\n", hr);

    hr = IMFAsyncResult_SetStatus(result, E_FAIL);
    ok(hr == S_OK, "Failed to set status, hr %#lx.\n", hr);
    ok(data->hrStatusResult == E_FAIL, "Unexpected status %#lx.\n", hr);

    hr = IMFAsyncResult_GetObject(result, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    object = (void *)0xdeadbeef;
    hr = IMFAsyncResult_GetObject(result, &object);
    ok(hr == E_POINTER, "Failed to get object, hr %#lx.\n", hr);
    ok(object == (void *)0xdeadbeef, "Unexpected object.\n");

    state = IMFAsyncResult_GetStateNoAddRef(result);
    ok(state == NULL, "Unexpected state.\n");

    /* Object. */
    hr = MFCreateAsyncResult((IUnknown *)result, &callback->IMFAsyncCallback_iface, NULL, &result2);
    ok(hr == S_OK, "Failed to create object, hr %#lx.\n", hr);

    data = (MFASYNCRESULT *)result2;
    ok(data->pCallback == &callback->IMFAsyncCallback_iface, "Unexpected callback value.\n");
    ok(data->hrStatusResult == S_OK, "Unexpected status %#lx.\n", data->hrStatusResult);
    ok(data->dwBytesTransferred == 0, "Unexpected byte length %lu.\n", data->dwBytesTransferred);
    ok(data->hEvent == NULL, "Unexpected event.\n");

    object = NULL;
    hr = IMFAsyncResult_GetObject(result2, &object);
    ok(hr == S_OK, "Failed to get object, hr %#lx.\n", hr);
    ok(object == (IUnknown *)result, "Unexpected object.\n");
    IUnknown_Release(object);

    IMFAsyncResult_Release(result2);

    /* State object. */
    hr = MFCreateAsyncResult(NULL, &callback->IMFAsyncCallback_iface, (IUnknown *)result, &result2);
    ok(hr == S_OK, "Failed to create object, hr %#lx.\n", hr);

    data = (MFASYNCRESULT *)result2;
    ok(data->pCallback == &callback->IMFAsyncCallback_iface, "Unexpected callback value.\n");
    ok(data->hrStatusResult == S_OK, "Unexpected status %#lx.\n", data->hrStatusResult);
    ok(data->dwBytesTransferred == 0, "Unexpected byte length %lu.\n", data->dwBytesTransferred);
    ok(data->hEvent == NULL, "Unexpected event.\n");

    state = NULL;
    hr = IMFAsyncResult_GetState(result2, &state);
    ok(hr == S_OK, "Failed to get state object, hr %#lx.\n", hr);
    ok(state == (IUnknown *)result, "Unexpected state.\n");
    IUnknown_Release(state);

    state = IMFAsyncResult_GetStateNoAddRef(result2);
    ok(state == (IUnknown *)result, "Unexpected state.\n");

    refcount = IMFAsyncResult_Release(result2);
    ok(!refcount, "Unexpected refcount %lu.\n", refcount);
    refcount = IMFAsyncResult_Release(result);
    ok(!refcount, "Unexpected refcount %lu.\n", refcount);

    /* Event handle is closed on release. */
    hr = MFCreateAsyncResult(NULL, NULL, NULL, &result);
    ok(hr == S_OK, "Failed to create object, hr %#lx.\n", hr);

    data = (MFASYNCRESULT *)result;
    data->hEvent = event = CreateEventA(NULL, FALSE, FALSE, NULL);
    ok(data->hEvent != NULL, "Failed to create event.\n");
    ret = GetHandleInformation(event, &flags);
    ok(ret, "Failed to get handle info.\n");

    refcount = IMFAsyncResult_Release(result);
    ok(!refcount, "Unexpected refcount %lu.\n", refcount);
    ret = GetHandleInformation(event, &flags);
    ok(!ret, "Expected handle to be closed.\n");

    hr = MFCreateAsyncResult(NULL, &callback->IMFAsyncCallback_iface, NULL, &result);
    ok(hr == S_OK, "Failed to create object, hr %#lx.\n", hr);

    data = (MFASYNCRESULT *)result;
    data->hEvent = event = CreateEventA(NULL, FALSE, FALSE, NULL);
    ok(data->hEvent != NULL, "Failed to create event.\n");
    ret = GetHandleInformation(event, &flags);
    ok(ret, "Failed to get handle info.\n");

    refcount = IMFAsyncResult_Release(result);
    ok(!refcount, "Unexpected refcount %lu.\n", refcount);
    ret = GetHandleInformation(event, &flags);
    ok(!ret, "Expected handle to be closed.\n");

    IMFAsyncCallback_Release(&callback->IMFAsyncCallback_iface);
}

static void test_startup(void)
{
    struct test_callback *callback;
    IMFAsyncResult *result;
    DWORD queue;
    HRESULT hr;

    hr = MFStartup(MAKELONG(MF_API_VERSION, 0xdead), MFSTARTUP_FULL);
    ok(hr == MF_E_BAD_STARTUP_VERSION, "Unexpected hr %#lx.\n", hr);

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Failed to start up, hr %#lx.\n", hr);

    check_platform_lock_count(1);

    hr = MFShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);

    hr = MFAllocateWorkQueue(&queue);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    /* Already shut down, has no effect. */
    hr = MFShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Failed to start up, hr %#lx.\n", hr);

    check_platform_lock_count(1);

    hr = MFShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);

    /* Platform lock. */
    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Failed to start up, hr %#lx.\n", hr);

    check_platform_lock_count(1);

    /* Unlocking implies shutdown. */
    hr = MFUnlockPlatform();
    ok(hr == S_OK, "Failed to unlock, %#lx.\n", hr);

    hr = MFAllocateWorkQueue(&queue);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = MFLockPlatform();
    ok(hr == S_OK, "Failed to lock, %#lx.\n", hr);

    check_platform_lock_count(1);

    hr = MFShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);

    if (!pRtwqStartup)
    {
        win_skip("RtwqStartup() not found.\n");
        return;
    }

    /* Rtwq equivalence */

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Failed to start up, hr %#lx.\n", hr);

    hr = pRtwqStartup();
    ok(hr == S_OK, "Failed to start up, hr %#lx.\n", hr);
    hr = pRtwqShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);
    check_platform_lock_count(1);

    /* Matching MFStartup() with RtwqShutdown() causes shutdown. */
    hr = pRtwqShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);

    hr = MFAllocateWorkQueue(&queue);
    ok(hr == MF_E_SHUTDOWN, "Failed to allocate a queue, hr %#lx.\n", hr);

    hr = MFShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);

    /* RtwqStartup() enables MF functions */
    hr = pRtwqStartup();
    ok(hr == S_OK, "Failed to start up, hr %#lx.\n", hr);

    check_platform_lock_count(1);

    callback = create_test_callback(NULL);

    /* MF platform lock is the Rtwq lock */
    hr = pRtwqUnlockPlatform();
    ok(hr == S_OK, "Failed to unlock platform, hr %#lx.\n", hr);

    hr = MFAllocateWorkQueue(&queue);
    ok(hr == MF_E_SHUTDOWN, "Failed to allocate a queue, hr %#lx.\n", hr);

    hr = MFCreateAsyncResult(NULL, &callback->IMFAsyncCallback_iface, NULL, &result);
    ok(hr == S_OK, "Failed to create result, hr %#lx.\n", hr);
    check_platform_lock_count(1);

    hr = pRtwqLockPlatform();
    ok(hr == S_OK, "Failed to lock platform, hr %#lx.\n", hr);
    check_platform_lock_count(2);

    IMFAsyncResult_Release(result);

    hr = pRtwqShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);

    hr = MFAllocateWorkQueue(&queue);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    IMFAsyncCallback_Release(&callback->IMFAsyncCallback_iface);
}

void test_startup_counts(void)
{
    IMFAsyncResult *result, *result2, *callback_result;
    struct test_callback *callback;
    MFWORKITEM_KEY key, key2;
    DWORD res, queue;
    LONG refcount;
    HRESULT hr;

    hr = MFLockPlatform();
    ok(hr == S_OK, "Failed to lock, %#lx.\n", hr);
    hr = MFAllocateWorkQueue(&queue);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);
    hr = MFUnlockPlatform();
    ok(hr == S_OK, "Failed to unlock, %#lx.\n", hr);

    callback = create_test_callback(&test_async_callback_result_vtbl);

    /* Create async results without startup. */
    hr = MFCreateAsyncResult(NULL, &callback->IMFAsyncCallback_iface, NULL, &result);
    ok(hr == S_OK, "Failed to create result, hr %#lx.\n", hr);
    hr = MFCreateAsyncResult(NULL, &callback->IMFAsyncCallback_iface, NULL, &result2);
    ok(hr == S_OK, "Failed to create result, hr %#lx.\n", hr);
    IMFAsyncResult_Release(result);
    IMFAsyncResult_Release(result2);

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Failed to start up, hr %#lx.\n", hr);
    /* Before startup the platform lock count does not track the maximum AsyncResult count. */
    check_platform_lock_count(1);

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Failed to start up, hr %#lx.\n", hr);
    /* Startup only locks once. */
    check_platform_lock_count(1);
    hr = MFShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);

    hr = MFCreateAsyncResult(NULL, &callback->IMFAsyncCallback_iface, NULL, &result);
    ok(hr == S_OK, "Failed to create result, hr %#lx.\n", hr);
    /* Platform locked by the AsyncResult object. */
    check_platform_lock_count(2);

    hr = MFCreateAsyncResult(NULL, &callback->IMFAsyncCallback_iface, NULL, &result2);
    ok(hr == S_OK, "Failed to create result, hr %#lx.\n", hr);
    check_platform_lock_count(3);

    IMFAsyncResult_Release(result);
    IMFAsyncResult_Release(result2);
    /* Platform lock count for AsyncResult objects does not decrease
     * unless the platform is in shutdown state. */
    todo_wine
    check_platform_lock_count(3);

    hr = MFCreateAsyncResult(NULL, &callback->IMFAsyncCallback_iface, NULL, &result);
    ok(hr == S_OK, "Failed to create result, hr %#lx.\n", hr);
    /* Platform lock count tracks the maximum AsyncResult count plus one for startup. */
    todo_wine
    check_platform_lock_count(3);
    IMFAsyncResult_Release(result);

    hr = MFPutWorkItem(MFASYNC_CALLBACK_QUEUE_STANDARD, &callback->IMFAsyncCallback_iface, NULL);
    ok(hr == S_OK, "got %#lx\n", hr);
    res = wait_async_callback_result(&callback->IMFAsyncCallback_iface, 100, &callback_result);
    ok(res == 0, "got %#lx\n", res);
    refcount = IMFAsyncResult_Release(callback_result);
    /* Release of an internal lock occurs in a worker thread. */
    flaky_wine
    ok(!refcount, "Unexpected refcount %ld.\n", refcount);
    todo_wine
    check_platform_lock_count(3);

    hr = MFLockPlatform();
    ok(hr == S_OK, "Failed to lock, %#lx.\n", hr);
    hr = MFLockPlatform();
    ok(hr == S_OK, "Failed to lock, %#lx.\n", hr);
    todo_wine
    check_platform_lock_count(5);

    hr = MFShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);

    /* Platform is in shutdown state if either the lock count or the startup count is <= 0. */
    hr = MFAllocateWorkQueue(&queue);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    /* Platform can be unlocked after shutdown. */
    hr = MFUnlockPlatform();
    ok(hr == S_OK, "Failed to unlock, %#lx.\n", hr);

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Failed to start up, hr %#lx.\n", hr);

    /* Platform locks for AsyncResult objects were released on shutdown, but the explicit lock was not. */
    check_platform_lock_count(2);
    hr = MFUnlockPlatform();
    ok(hr == S_OK, "Failed to unlock, %#lx.\n", hr);
    check_platform_lock_count(1);

    hr = MFUnlockPlatform();
    ok(hr == S_OK, "Failed to unlock, %#lx.\n", hr);
    /* Zero lock count. */
    hr = MFAllocateWorkQueue(&queue);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);
    hr = MFUnlockPlatform();
    ok(hr == S_OK, "Failed to unlock, %#lx.\n", hr);
    /* Negative lock count. */
    hr = MFAllocateWorkQueue(&queue);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);
    hr = MFLockPlatform();
    ok(hr == S_OK, "Failed to lock, %#lx.\n", hr);
    hr = MFLockPlatform();
    ok(hr == S_OK, "Failed to lock, %#lx.\n", hr);
    check_platform_lock_count(1);

    hr = MFCreateAsyncResult(NULL, &callback->IMFAsyncCallback_iface, NULL, &result);
    ok(hr == S_OK, "Failed to create result, hr %#lx.\n", hr);
    check_platform_lock_count(2);

    hr = MFShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);

    /* Release an AsyncResult object after shutdown. Lock count tracks the AsyncResult count.
     * It's not possible to show if unlock occurs immedately or on the next startup. */
    IMFAsyncResult_Release(result);
    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Failed to start up, hr %#lx.\n", hr);
    check_platform_lock_count(1);

    hr = MFCreateAsyncResult(NULL, &callback->IMFAsyncCallback_iface, NULL, &result);
    ok(hr == S_OK, "Failed to create result, hr %#lx.\n", hr);
    hr = MFShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);
    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Failed to start up, hr %#lx.\n", hr);
    check_platform_lock_count(2);
    /* Release an AsyncResult object after shutdown and startup */
    IMFAsyncResult_Release(result);
    todo_wine
    check_platform_lock_count(2);

    hr = MFScheduleWorkItem(&callback->IMFAsyncCallback_iface, NULL, -5000, &key);
    ok(hr == S_OK, "Failed to schedule item, hr %#lx.\n", hr);
    /* The AsyncResult created for the item locks the platform */
    check_platform_lock_count(2);

    hr = MFScheduleWorkItem(&callback->IMFAsyncCallback_iface, NULL, -5000, &key2);
    ok(hr == S_OK, "Failed to schedule item, hr %#lx.\n", hr);
    check_platform_lock_count(3);

    /* Platform locks for scheduled items are not released */
    hr = MFCancelWorkItem(key);
    ok(hr == S_OK, "Failed to cancel item, hr %#lx.\n", hr);
    hr = MFCancelWorkItem(key2);
    ok(hr == S_OK, "Failed to cancel item, hr %#lx.\n", hr);
    todo_wine
    check_platform_lock_count(3);

    hr = MFScheduleWorkItem(&callback->IMFAsyncCallback_iface, NULL, -5000, &key);
    ok(hr == S_OK, "Failed to schedule item, hr %#lx.\n", hr);
    todo_wine
    check_platform_lock_count(3);

    hr = MFShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);

    hr = MFAllocateWorkQueue(&queue);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = MFCancelWorkItem(key);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    res = wait_async_callback_result(&callback->IMFAsyncCallback_iface, 0, &result);
    ok(res == WAIT_TIMEOUT, "got res %#lx\n", res);

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Failed to start up, hr %#lx.\n", hr);

    /* Shutdown while a scheduled item is pending leaks the internal AsyncResult. */
    check_platform_lock_count(2);

    hr = MFShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);

    IMFAsyncCallback_Release(&callback->IMFAsyncCallback_iface);
}

static void test_allocate_queue(void)
{
    DWORD queue, queue2;
    HRESULT hr;

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Failed to start up, hr %#lx.\n", hr);

    hr = MFAllocateWorkQueue(&queue);
    ok(hr == S_OK, "Failed to allocate a queue, hr %#lx.\n", hr);
    ok(queue & MFASYNC_CALLBACK_QUEUE_PRIVATE_MASK, "Unexpected queue id.\n");

    hr = MFUnlockWorkQueue(queue);
    ok(hr == S_OK, "Failed to unlock the queue, hr %#lx.\n", hr);

    hr = MFUnlockWorkQueue(queue);
    ok(FAILED(hr), "Unexpected hr %#lx.\n", hr);

    hr = MFAllocateWorkQueue(&queue2);
    ok(hr == S_OK, "Failed to allocate a queue, hr %#lx.\n", hr);
    ok(queue2 & MFASYNC_CALLBACK_QUEUE_PRIVATE_MASK, "Unexpected queue id.\n");

    hr = MFUnlockWorkQueue(queue2);
    ok(hr == S_OK, "Failed to unlock the queue, hr %#lx.\n", hr);

    /* Unlock in system queue range. */
    hr = MFUnlockWorkQueue(MFASYNC_CALLBACK_QUEUE_STANDARD);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = MFUnlockWorkQueue(MFASYNC_CALLBACK_QUEUE_UNDEFINED);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = MFUnlockWorkQueue(0x20);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = MFShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);
}

static void test_MFCopyImage(void)
{
    DWORD dest[4], src[4];
    HRESULT hr;

    if (!pMFCopyImage)
    {
        win_skip("MFCopyImage() is not available.\n");
        return;
    }

    memset(dest, 0xaa, sizeof(dest));
    memset(src, 0x11, sizeof(src));

    hr = pMFCopyImage((BYTE *)dest, 8, (const BYTE *)src, 8, 4, 1);
    ok(hr == S_OK, "Failed to copy image %#lx.\n", hr);
    ok(dest[0] == src[0] && dest[1] == 0xaaaaaaaa, "Unexpected buffer contents.\n");

    /* Negative destination stride. */
    memset(dest, 0xaa, sizeof(dest));

    src[0] = 0x11111111;
    src[1] = 0x22222222;
    src[2] = 0x33333333;
    src[3] = 0x44444444;

    hr = pMFCopyImage((BYTE *)(dest + 2), -8, (const BYTE *)src, 8, 4, 2);
    ok(hr == S_OK, "Failed to copy image %#lx.\n", hr);
    ok(dest[0] == 0x33333333, "Unexpected buffer contents %#lx.\n", dest[0]);
    ok(dest[1] == 0xaaaaaaaa, "Unexpected buffer contents %#lx.\n", dest[1]);
    ok(dest[2] == 0x11111111, "Unexpected buffer contents %#lx.\n", dest[2]);
    ok(dest[3] == 0xaaaaaaaa, "Unexpected buffer contents %#lx.\n", dest[3]);

    memset(dest, 0xaa, sizeof(dest));
    memset(src, 0x11, sizeof(src));

    hr = pMFCopyImage((BYTE *)dest, 8, (const BYTE *)src, 8, 16, 1);
    ok(hr == S_OK, "Failed to copy image %#lx.\n", hr);
    ok(!memcmp(dest, src, 16), "Unexpected buffer contents.\n");

    memset(dest, 0xaa, sizeof(dest));
    memset(src, 0x11, sizeof(src));

    hr = pMFCopyImage((BYTE *)dest, 8, (const BYTE *)src, 8, 8, 2);
    ok(hr == S_OK, "Failed to copy image %#lx.\n", hr);
    ok(!memcmp(dest, src, 16), "Unexpected buffer contents.\n");
}

static void test_MFCreateCollection(void)
{
    IMFCollection *collection;
    IUnknown *element;
    DWORD count;
    HRESULT hr;

    hr = MFCreateCollection(NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateCollection(&collection);
    ok(hr == S_OK, "Failed to create collection, hr %#lx.\n", hr);

    hr = IMFCollection_GetElementCount(collection, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    count = 1;
    hr = IMFCollection_GetElementCount(collection, &count);
    ok(hr == S_OK, "Failed to get element count, hr %#lx.\n", hr);
    ok(count == 0, "Unexpected count %lu.\n", count);

    hr = IMFCollection_GetElement(collection, 0, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    element = (void *)0xdeadbeef;
    hr = IMFCollection_GetElement(collection, 0, &element);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(element == (void *)0xdeadbeef, "Unexpected pointer.\n");

    hr = IMFCollection_RemoveElement(collection, 0, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    element = (void *)0xdeadbeef;
    hr = IMFCollection_RemoveElement(collection, 0, &element);
    ok(hr == E_INVALIDARG, "Failed to remove element, hr %#lx.\n", hr);
    ok(element == (void *)0xdeadbeef, "Unexpected pointer.\n");

    hr = IMFCollection_RemoveAllElements(collection);
    ok(hr == S_OK, "Failed to clear, hr %#lx.\n", hr);

    hr = IMFCollection_AddElement(collection, (IUnknown *)collection);
    ok(hr == S_OK, "Failed to add element, hr %#lx.\n", hr);

    count = 0;
    hr = IMFCollection_GetElementCount(collection, &count);
    ok(hr == S_OK, "Failed to get element count, hr %#lx.\n", hr);
    ok(count == 1, "Unexpected count %lu.\n", count);

    hr = IMFCollection_AddElement(collection, NULL);
    ok(hr == S_OK, "Failed to add element, hr %#lx.\n", hr);

    count = 0;
    hr = IMFCollection_GetElementCount(collection, &count);
    ok(hr == S_OK, "Failed to get element count, hr %#lx.\n", hr);
    ok(count == 2, "Unexpected count %lu.\n", count);

    hr = IMFCollection_InsertElementAt(collection, 10, (IUnknown *)collection);
    ok(hr == S_OK, "Failed to insert element, hr %#lx.\n", hr);

    count = 0;
    hr = IMFCollection_GetElementCount(collection, &count);
    ok(hr == S_OK, "Failed to get element count, hr %#lx.\n", hr);
    ok(count == 11, "Unexpected count %lu.\n", count);

    hr = IMFCollection_GetElement(collection, 0, &element);
    ok(hr == S_OK, "Failed to get element, hr %#lx.\n", hr);
    ok(element == (IUnknown *)collection, "Unexpected element.\n");
    IUnknown_Release(element);

    hr = IMFCollection_GetElement(collection, 1, &element);
    ok(hr == E_UNEXPECTED, "Unexpected hr %#lx.\n", hr);
    ok(!element, "Unexpected element.\n");

    hr = IMFCollection_GetElement(collection, 2, &element);
    ok(hr == E_UNEXPECTED, "Unexpected hr %#lx.\n", hr);
    ok(!element, "Unexpected element.\n");

    hr = IMFCollection_GetElement(collection, 10, &element);
    ok(hr == S_OK, "Failed to get element, hr %#lx.\n", hr);
    ok(element == (IUnknown *)collection, "Unexpected element.\n");
    IUnknown_Release(element);

    hr = IMFCollection_InsertElementAt(collection, 0, NULL);
    ok(hr == S_OK, "Failed to insert element, hr %#lx.\n", hr);

    hr = IMFCollection_GetElement(collection, 0, &element);
    ok(hr == E_UNEXPECTED, "Unexpected hr %#lx.\n", hr);

    hr = IMFCollection_RemoveAllElements(collection);
    ok(hr == S_OK, "Failed to clear, hr %#lx.\n", hr);

    count = 1;
    hr = IMFCollection_GetElementCount(collection, &count);
    ok(hr == S_OK, "Failed to get element count, hr %#lx.\n", hr);
    ok(count == 0, "Unexpected count %lu.\n", count);

    hr = IMFCollection_InsertElementAt(collection, 0, NULL);
    ok(hr == S_OK, "Failed to insert element, hr %#lx.\n", hr);

    IMFCollection_Release(collection);
}

static void test_MFHeapAlloc(void)
{
    void *res;

    res = MFHeapAlloc(16, 0, NULL, 0, eAllocationTypeIgnore);
    ok(res != NULL, "MFHeapAlloc failed.\n");

    MFHeapFree(res);
}

static void test_scheduled_items(void)
{
    struct test_callback *callback;
    IMFAsyncResult *result;
    MFWORKITEM_KEY key, key2;
    HRESULT hr;
    ULONG refcount;

    callback = create_test_callback(NULL);

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Failed to start up, hr %#lx.\n", hr);

    hr = MFScheduleWorkItem(&callback->IMFAsyncCallback_iface, NULL, -5000, &key);
    ok(hr == S_OK, "Failed to schedule item, hr %#lx.\n", hr);

    hr = MFCancelWorkItem(key);
    ok(hr == S_OK, "Failed to cancel item, hr %#lx.\n", hr);

    refcount = IMFAsyncCallback_Release(&callback->IMFAsyncCallback_iface);
    ok(refcount == 0, "Unexpected refcount %lu.\n", refcount);

    hr = MFCancelWorkItem(key);
    ok(hr == MF_E_NOT_FOUND || broken(hr == S_OK) /* < win10 */, "Unexpected hr %#lx.\n", hr);

    if (!pMFPutWaitingWorkItem)
    {
        win_skip("Waiting items are not supported.\n");
        return;
    }

    callback = create_test_callback(NULL);

    hr = MFCreateAsyncResult(NULL, &callback->IMFAsyncCallback_iface, NULL, &result);
    ok(hr == S_OK, "Failed to create result, hr %#lx.\n", hr);

    hr = pMFPutWaitingWorkItem(NULL, 0, result, &key);
    ok(hr == S_OK, "Failed to add waiting item, hr %#lx.\n", hr);

    hr = pMFPutWaitingWorkItem(NULL, 0, result, &key2);
    ok(hr == S_OK, "Failed to add waiting item, hr %#lx.\n", hr);

    hr = MFCancelWorkItem(key);
    ok(hr == S_OK, "Failed to cancel item, hr %#lx.\n", hr);

    hr = MFCancelWorkItem(key2);
    ok(hr == S_OK, "Failed to cancel item, hr %#lx.\n", hr);

    IMFAsyncResult_Release(result);

    hr = MFScheduleWorkItem(&callback->IMFAsyncCallback_iface, NULL, -5000, &key);
    ok(hr == S_OK, "Failed to schedule item, hr %#lx.\n", hr);

    hr = MFCancelWorkItem(key);
    ok(hr == S_OK, "Failed to cancel item, hr %#lx.\n", hr);

    hr = MFShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);

    IMFAsyncCallback_Release(&callback->IMFAsyncCallback_iface);
}

static void test_serial_queue(void)
{
    static const DWORD queue_ids[] =
    {
        MFASYNC_CALLBACK_QUEUE_STANDARD,
        MFASYNC_CALLBACK_QUEUE_RT,
        MFASYNC_CALLBACK_QUEUE_IO,
        MFASYNC_CALLBACK_QUEUE_TIMER,
        MFASYNC_CALLBACK_QUEUE_MULTITHREADED,
        MFASYNC_CALLBACK_QUEUE_LONG_FUNCTION,
    };
    DWORD queue, serial_queue;
    unsigned int i;
    HRESULT hr;

    if (!pMFAllocateSerialWorkQueue)
    {
        win_skip("Serial queues are not supported.\n");
        return;
    }

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Failed to start up, hr %#lx.\n", hr);

    for (i = 0; i < ARRAY_SIZE(queue_ids); ++i)
    {
        BOOL broken_types = queue_ids[i] == MFASYNC_CALLBACK_QUEUE_TIMER ||
                queue_ids[i] == MFASYNC_CALLBACK_QUEUE_LONG_FUNCTION;

        hr = pMFAllocateSerialWorkQueue(queue_ids[i], &serial_queue);
        ok(hr == S_OK || broken(broken_types && hr == E_INVALIDARG) /* Win8 */,
                "%u: failed to allocate a queue, hr %#lx.\n", i, hr);

        if (SUCCEEDED(hr))
        {
            hr = MFUnlockWorkQueue(serial_queue);
            ok(hr == S_OK, "%u: failed to unlock the queue, hr %#lx.\n", i, hr);
        }
    }

    /* Chain them together. */
    hr = pMFAllocateSerialWorkQueue(MFASYNC_CALLBACK_QUEUE_STANDARD, &serial_queue);
    ok(hr == S_OK, "Failed to allocate a queue, hr %#lx.\n", hr);

    hr = pMFAllocateSerialWorkQueue(serial_queue, &queue);
    ok(hr == S_OK, "Failed to allocate a queue, hr %#lx.\n", hr);

    hr = MFUnlockWorkQueue(serial_queue);
    ok(hr == S_OK, "Failed to unlock the queue, hr %#lx.\n", hr);

    hr = MFUnlockWorkQueue(queue);
    ok(hr == S_OK, "Failed to unlock the queue, hr %#lx.\n", hr);

    hr = MFShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);
}

static LONG periodic_counter;
static void CALLBACK periodic_callback(IUnknown *context)
{
    InterlockedIncrement(&periodic_counter);
}

static void test_periodic_callback(void)
{
    DWORD period, key;
    HRESULT hr;
    struct test_callback *context = create_test_callback(NULL);

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Failed to start up, hr %#lx.\n", hr);

    period = 0;
    hr = MFGetTimerPeriodicity(&period);
    ok(hr == S_OK, "Failed to get timer perdiod, hr %#lx.\n", hr);
    ok(period == 10, "Unexpected period %lu.\n", period);

    if (!pMFAddPeriodicCallback)
    {
        win_skip("Periodic callbacks are not supported.\n");
        hr = MFShutdown();
        ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);
        return;
    }

    ok(periodic_counter == 0, "Unexpected counter value %lu.\n", periodic_counter);

    hr = pMFAddPeriodicCallback(periodic_callback, NULL, &key);
    ok(hr == S_OK, "Failed to add periodic callback, hr %#lx.\n", hr);
    ok(key != 0, "Unexpected key %#lx.\n", key);

    Sleep(10 * period);

    hr = pMFRemovePeriodicCallback(key);
    ok(hr == S_OK, "Failed to remove callback, hr %#lx.\n", hr);

    ok(periodic_counter > 0, "Unexpected counter value %lu.\n", periodic_counter);

    hr= pMFAddPeriodicCallback(periodic_callback, (IUnknown *)&context->IMFAsyncCallback_iface, &key);
    ok(hr == S_OK, "Failed to add periodic callback, hr %#lx.\n", hr);
    EXPECT_REF(&context->IMFAsyncCallback_iface, 2);

    hr = pMFRemovePeriodicCallback(key);
    ok(hr == S_OK, "Failed to remove callback, hr %#lx.\n", hr);
    Sleep(500);
    EXPECT_REF(&context->IMFAsyncCallback_iface, 1);
    IMFAsyncCallback_Release(&context->IMFAsyncCallback_iface);

    hr = MFShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);
}

static void test_event_queue(void)
{
    struct test_callback *callback, *callback2;
    IMFMediaEvent *event, *event2;
    IMFMediaEventQueue *queue;
    IMFAsyncResult *result;
    HRESULT hr;
    DWORD ret;

    callback = create_test_callback(NULL);
    callback2 = create_test_callback(NULL);

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Failed to start up, hr %#lx.\n", hr);

    hr = MFCreateEventQueue(&queue);
    ok(hr == S_OK, "Failed to create event queue, hr %#lx.\n", hr);

    hr = IMFMediaEventQueue_GetEvent(queue, MF_EVENT_FLAG_NO_WAIT, &event);
    ok(hr == MF_E_NO_EVENTS_AVAILABLE, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateMediaEvent(MEError, &GUID_NULL, E_FAIL, NULL, &event);
    ok(hr == S_OK, "Failed to create event object, hr %#lx.\n", hr);

    if (is_win8_plus)
    {
        hr = IMFMediaEventQueue_QueueEvent(queue, event);
        ok(hr == S_OK, "Failed to queue event, hr %#lx.\n", hr);

        hr = IMFMediaEventQueue_GetEvent(queue, MF_EVENT_FLAG_NO_WAIT, &event2);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(event2 == event, "Unexpected event object.\n");
        IMFMediaEvent_Release(event2);

        hr = IMFMediaEventQueue_QueueEvent(queue, event);
        ok(hr == S_OK, "Failed to queue event, hr %#lx.\n", hr);

        hr = IMFMediaEventQueue_GetEvent(queue, 0, &event2);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        IMFMediaEvent_Release(event2);
    }

    /* Async case. */
    hr = IMFMediaEventQueue_BeginGetEvent(queue, NULL, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaEventQueue_BeginGetEvent(queue, &callback->IMFAsyncCallback_iface, (IUnknown *)queue);
    ok(hr == S_OK, "Failed to Begin*, hr %#lx.\n", hr);

    /* Same callback, same state. */
    hr = IMFMediaEventQueue_BeginGetEvent(queue, &callback->IMFAsyncCallback_iface, (IUnknown *)queue);
    ok(hr == MF_S_MULTIPLE_BEGIN, "Unexpected hr %#lx.\n", hr);

    /* Same callback, different state. */
    hr = IMFMediaEventQueue_BeginGetEvent(queue, &callback->IMFAsyncCallback_iface, (IUnknown *)&callback->IMFAsyncCallback_iface);
    ok(hr == MF_E_MULTIPLE_BEGIN, "Unexpected hr %#lx.\n", hr);

    /* Different callback, same state. */
    hr = IMFMediaEventQueue_BeginGetEvent(queue, &callback2->IMFAsyncCallback_iface, (IUnknown *)queue);
    ok(hr == MF_E_MULTIPLE_SUBSCRIBERS, "Unexpected hr %#lx.\n", hr);

    /* Different callback, different state. */
    hr = IMFMediaEventQueue_BeginGetEvent(queue, &callback2->IMFAsyncCallback_iface, (IUnknown *)&callback->IMFAsyncCallback_iface);
    ok(hr == MF_E_MULTIPLE_SUBSCRIBERS, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaEventQueue_QueueEvent(queue, event);
    ok(hr == S_OK, "Failed to queue event, hr %#lx.\n", hr);

    ret = WaitForSingleObject(callback->event, 500);
    ok(ret == WAIT_OBJECT_0, "Unexpected return value %#lx.\n", ret);

    IMFMediaEvent_Release(event);

    hr = MFCreateAsyncResult(NULL, &callback->IMFAsyncCallback_iface, NULL, &result);
    ok(hr == S_OK, "Failed to create result, hr %#lx.\n", hr);

    hr = IMFMediaEventQueue_EndGetEvent(queue, result, &event);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);

    /* Shutdown behavior. */
    hr = IMFMediaEventQueue_Shutdown(queue);
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);

    hr = IMFMediaEventQueue_GetEvent(queue, MF_EVENT_FLAG_NO_WAIT, &event);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateMediaEvent(MEError, &GUID_NULL, E_FAIL, NULL, &event);
    ok(hr == S_OK, "Failed to create event object, hr %#lx.\n", hr);
    hr = IMFMediaEventQueue_QueueEvent(queue, event);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);
    IMFMediaEvent_Release(event);

    hr = IMFMediaEventQueue_QueueEventParamUnk(queue, MEError, &GUID_NULL, E_FAIL, NULL);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaEventQueue_QueueEventParamVar(queue, MEError, &GUID_NULL, E_FAIL, NULL);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaEventQueue_BeginGetEvent(queue, &callback->IMFAsyncCallback_iface, NULL);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaEventQueue_BeginGetEvent(queue, NULL, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaEventQueue_EndGetEvent(queue, result, &event);
    ok(hr == MF_E_SHUTDOWN, "Unexpected hr %#lx.\n", hr);
    IMFAsyncResult_Release(result);

    /* Already shut down. */
    hr = IMFMediaEventQueue_Shutdown(queue);
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);

    IMFMediaEventQueue_Release(queue);
    IMFAsyncCallback_Release(&callback->IMFAsyncCallback_iface);

    /* Release while subscribed. */
    callback = create_test_callback(NULL);

    hr = MFCreateEventQueue(&queue);
    ok(hr == S_OK, "Failed to create event queue, hr %#lx.\n", hr);

    hr = IMFMediaEventQueue_BeginGetEvent(queue, &callback->IMFAsyncCallback_iface, NULL);
    ok(hr == S_OK, "Failed to Begin*, hr %#lx.\n", hr);
    EXPECT_REF(&callback->IMFAsyncCallback_iface, 2);

    IMFMediaEventQueue_Release(queue);
    ret = get_refcount(&callback->IMFAsyncCallback_iface);
    ok(ret == 1 || broken(ret == 2) /* Vista */,
       "Unexpected refcount %ld, expected 1.\n", ret);
    IMFAsyncCallback_Release(&callback->IMFAsyncCallback_iface);

    hr = MFShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);
}

static void test_presentation_descriptor(void)
{
    IMFStreamDescriptor *stream_desc[2], *stream_desc2;
    IMFPresentationDescriptor *pd, *pd2;
    IMFMediaType *media_type;
    unsigned int i;
    BOOL selected;
    UINT64 value;
    DWORD count;
    HRESULT hr;

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);

    for (i = 0; i < ARRAY_SIZE(stream_desc); ++i)
    {
        hr = MFCreateStreamDescriptor(0, 1, &media_type, &stream_desc[i]);
        ok(hr == S_OK, "Failed to create descriptor, hr %#lx.\n", hr);
    }

    hr = MFCreatePresentationDescriptor(ARRAY_SIZE(stream_desc), stream_desc, &pd);
    ok(hr == S_OK, "Failed to create presentation descriptor, hr %#lx.\n", hr);

    hr = IMFPresentationDescriptor_GetStreamDescriptorCount(pd, &count);
    ok(count == ARRAY_SIZE(stream_desc), "Unexpected count %lu.\n", count);

    for (i = 0; i < count; ++i)
    {
        hr = IMFPresentationDescriptor_GetStreamDescriptorByIndex(pd, i, &selected, &stream_desc2);
        ok(hr == S_OK, "Failed to get stream descriptor, hr %#lx.\n", hr);
        ok(!selected, "Unexpected selected state.\n");
        ok(stream_desc[i] == stream_desc2, "Unexpected object.\n");
        IMFStreamDescriptor_Release(stream_desc2);
    }

    hr = IMFPresentationDescriptor_GetStreamDescriptorByIndex(pd, 10, &selected, &stream_desc2);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IMFPresentationDescriptor_SelectStream(pd, 10);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IMFPresentationDescriptor_SelectStream(pd, 0);
    ok(hr == S_OK, "Failed to select a stream, hr %#lx.\n", hr);

    hr = IMFPresentationDescriptor_GetStreamDescriptorByIndex(pd, 0, &selected, &stream_desc2);
    ok(hr == S_OK, "Failed to get stream descriptor, hr %#lx.\n", hr);
    ok(!!selected, "Unexpected selected state.\n");
    IMFStreamDescriptor_Release(stream_desc2);

    hr = IMFPresentationDescriptor_SetUINT64(pd, &MF_PD_TOTAL_FILE_SIZE, 1);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    hr = IMFPresentationDescriptor_Clone(pd, &pd2);
    ok(hr == S_OK, "Failed to clone, hr %#lx.\n", hr);

    hr = IMFPresentationDescriptor_GetStreamDescriptorByIndex(pd2, 0, &selected, &stream_desc2);
    ok(hr == S_OK, "Failed to get stream descriptor, hr %#lx.\n", hr);
    ok(!!selected, "Unexpected selected state.\n");
    ok(stream_desc2 == stream_desc[0], "Unexpected stream descriptor.\n");
    IMFStreamDescriptor_Release(stream_desc2);

    value = 0;
    hr = IMFPresentationDescriptor_GetUINT64(pd2, &MF_PD_TOTAL_FILE_SIZE, &value);
    ok(hr == S_OK, "Failed to get attribute, hr %#lx.\n", hr);
    ok(value == 1, "Unexpected attribute value.\n");

    IMFPresentationDescriptor_Release(pd2);
    IMFPresentationDescriptor_Release(pd);

    for (i = 0; i < ARRAY_SIZE(stream_desc); ++i)
    {
        IMFStreamDescriptor_Release(stream_desc[i]);
    }

    /* Partially initialized array. */
    hr = MFCreateStreamDescriptor(0, 1, &media_type, &stream_desc[1]);
    ok(hr == S_OK, "Failed to create descriptor, hr %#lx.\n", hr);
    stream_desc[0] = NULL;

    hr = MFCreatePresentationDescriptor(ARRAY_SIZE(stream_desc), stream_desc, &pd);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    IMFStreamDescriptor_Release(stream_desc[1]);
    IMFMediaType_Release(media_type);
}

enum clock_action
{
    CLOCK_START,
    CLOCK_STOP,
    CLOCK_PAUSE,
    CLOCK_RESTART,
};

static void test_system_time_source(void)
{
    static const struct clock_state_test
    {
        enum clock_action action;
        MFCLOCK_STATE state;
        BOOL is_invalid;
    }
    clock_state_change[] =
    {
        { CLOCK_STOP, MFCLOCK_STATE_INVALID },
        { CLOCK_RESTART, MFCLOCK_STATE_INVALID, TRUE },
        { CLOCK_PAUSE, MFCLOCK_STATE_PAUSED },
        { CLOCK_PAUSE, MFCLOCK_STATE_PAUSED, TRUE },
        { CLOCK_STOP, MFCLOCK_STATE_STOPPED },
        { CLOCK_STOP, MFCLOCK_STATE_STOPPED },
        { CLOCK_RESTART, MFCLOCK_STATE_STOPPED, TRUE },
        { CLOCK_START, MFCLOCK_STATE_RUNNING },
        { CLOCK_START, MFCLOCK_STATE_RUNNING },
        { CLOCK_RESTART, MFCLOCK_STATE_RUNNING, TRUE },
        { CLOCK_PAUSE, MFCLOCK_STATE_PAUSED },
        { CLOCK_START, MFCLOCK_STATE_RUNNING },
        { CLOCK_PAUSE, MFCLOCK_STATE_PAUSED },
        { CLOCK_RESTART, MFCLOCK_STATE_RUNNING },
        { CLOCK_RESTART, MFCLOCK_STATE_RUNNING, TRUE },
        { CLOCK_STOP, MFCLOCK_STATE_STOPPED },
        { CLOCK_PAUSE, MFCLOCK_STATE_STOPPED, TRUE },
    };
    IMFPresentationTimeSource *time_source, *time_source2;
    IMFClockStateSink *statesink;
    IMFClock *clock, *clock2;
    MFCLOCK_PROPERTIES props;
    MFCLOCK_STATE state;
    unsigned int i;
    MFTIME systime;
    LONGLONG time;
    DWORD value;
    HRESULT hr;

    hr = MFCreateSystemTimeSource(&time_source);
    ok(hr == S_OK, "Failed to create time source, hr %#lx.\n", hr);

    hr = IMFPresentationTimeSource_GetClockCharacteristics(time_source, &value);
    ok(hr == S_OK, "Failed to get flags, hr %#lx.\n", hr);
    ok(value == (MFCLOCK_CHARACTERISTICS_FLAG_FREQUENCY_10MHZ | MFCLOCK_CHARACTERISTICS_FLAG_IS_SYSTEM_CLOCK),
            "Unexpected flags %#lx.\n", value);

    value = 1;
    hr = IMFPresentationTimeSource_GetContinuityKey(time_source, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value == 0, "Unexpected value %lu.\n", value);

    hr = IMFPresentationTimeSource_GetState(time_source, 0, &state);
    ok(hr == S_OK, "Failed to get state, hr %#lx.\n", hr);
    ok(state == MFCLOCK_STATE_INVALID, "Unexpected state %d.\n", state);

    hr = IMFPresentationTimeSource_QueryInterface(time_source, &IID_IMFClockStateSink, (void **)&statesink);
    ok(hr == S_OK, "Failed to get state sink, hr %#lx.\n", hr);

    /* State changes. */
    for (i = 0; i < ARRAY_SIZE(clock_state_change); ++i)
    {
        switch (clock_state_change[i].action)
        {
            case CLOCK_STOP:
                hr = IMFClockStateSink_OnClockStop(statesink, 0);
                break;
            case CLOCK_RESTART:
                hr = IMFClockStateSink_OnClockRestart(statesink, 0);
                break;
            case CLOCK_PAUSE:
                hr = IMFClockStateSink_OnClockPause(statesink, 0);
                break;
            case CLOCK_START:
                hr = IMFClockStateSink_OnClockStart(statesink, 0, 0);
                break;
            default:
                ;
        }
        ok(hr == (clock_state_change[i].is_invalid ? MF_E_INVALIDREQUEST : S_OK), "%u: unexpected hr %#lx.\n", i, hr);
        hr = IMFPresentationTimeSource_GetState(time_source, 0, &state);
        ok(hr == S_OK, "%u: failed to get state, hr %#lx.\n", i, hr);
        ok(state == clock_state_change[i].state, "%u: unexpected state %d.\n", i, state);
    }

    IMFClockStateSink_Release(statesink);

    /* Properties. */
    hr = IMFPresentationTimeSource_GetProperties(time_source, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFPresentationTimeSource_GetProperties(time_source, &props);
    ok(hr == S_OK, "Failed to get clock properties, hr %#lx.\n", hr);

    ok(props.qwCorrelationRate == 0, "Unexpected correlation rate %s.\n",
            wine_dbgstr_longlong(props.qwCorrelationRate));
    ok(IsEqualGUID(&props.guidClockId, &GUID_NULL), "Unexpected clock id %s.\n", wine_dbgstr_guid(&props.guidClockId));
    ok(props.dwClockFlags == 0, "Unexpected flags %#lx.\n", props.dwClockFlags);
    ok(props.qwClockFrequency == MFCLOCK_FREQUENCY_HNS, "Unexpected frequency %s.\n",
            wine_dbgstr_longlong(props.qwClockFrequency));
    ok(props.dwClockTolerance == MFCLOCK_TOLERANCE_UNKNOWN, "Unexpected tolerance %lu.\n", props.dwClockTolerance);
    ok(props.dwClockJitter == 1, "Unexpected jitter %lu.\n", props.dwClockJitter);

    /* Underlying clock. */
    hr = MFCreateSystemTimeSource(&time_source2);
    ok(hr == S_OK, "Failed to create time source, hr %#lx.\n", hr);
    EXPECT_REF(time_source2, 1);
    hr = IMFPresentationTimeSource_GetUnderlyingClock(time_source2, &clock2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(time_source2, 1);
    EXPECT_REF(clock2, 2);

    EXPECT_REF(time_source, 1);
    hr = IMFPresentationTimeSource_GetUnderlyingClock(time_source, &clock);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(time_source, 1);
    EXPECT_REF(clock, 2);

    ok(clock != clock2, "Unexpected clock instance.\n");

    IMFPresentationTimeSource_Release(time_source2);
    IMFClock_Release(clock2);

    hr = IMFClock_GetClockCharacteristics(clock, &value);
    ok(hr == S_OK, "Failed to get clock flags, hr %#lx.\n", hr);
    ok(value == (MFCLOCK_CHARACTERISTICS_FLAG_FREQUENCY_10MHZ | MFCLOCK_CHARACTERISTICS_FLAG_ALWAYS_RUNNING |
            MFCLOCK_CHARACTERISTICS_FLAG_IS_SYSTEM_CLOCK), "Unexpected flags %#lx.\n", value);

    hr = IMFClock_GetContinuityKey(clock, &value);
    ok(hr == S_OK, "Failed to get clock key, hr %#lx.\n", hr);
    ok(value == 0, "Unexpected key value %lu.\n", value);

    hr = IMFClock_GetState(clock, 0, &state);
    ok(hr == S_OK, "Failed to get clock state, hr %#lx.\n", hr);
    ok(state == MFCLOCK_STATE_RUNNING, "Unexpected state %d.\n", state);

    hr = IMFClock_GetProperties(clock, &props);
    ok(hr == S_OK, "Failed to get clock properties, hr %#lx.\n", hr);

    ok(props.qwCorrelationRate == 0, "Unexpected correlation rate %s.\n",
            wine_dbgstr_longlong(props.qwCorrelationRate));
    ok(IsEqualGUID(&props.guidClockId, &GUID_NULL), "Unexpected clock id %s.\n", wine_dbgstr_guid(&props.guidClockId));
    ok(props.dwClockFlags == 0, "Unexpected flags %#lx.\n", props.dwClockFlags);
    ok(props.qwClockFrequency == MFCLOCK_FREQUENCY_HNS, "Unexpected frequency %s.\n",
            wine_dbgstr_longlong(props.qwClockFrequency));
    ok(props.dwClockTolerance == MFCLOCK_TOLERANCE_UNKNOWN, "Unexpected tolerance %lu.\n", props.dwClockTolerance);
    ok(props.dwClockJitter == 1, "Unexpected jitter %lu.\n", props.dwClockJitter);

    hr = IMFClock_GetCorrelatedTime(clock, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get clock time, hr %#lx.\n", hr);
    ok(time == systime, "Unexpected time %s, %s.\n", wine_dbgstr_longlong(time), wine_dbgstr_longlong(systime));

    IMFClock_Release(clock);

    /* Test returned time regarding specified rate and offset.  */
    hr = IMFPresentationTimeSource_QueryInterface(time_source, &IID_IMFClockStateSink, (void **)&statesink);
    ok(hr == S_OK, "Failed to get sink interface, hr %#lx.\n", hr);

    hr = IMFPresentationTimeSource_GetState(time_source, 0, &state);
    ok(hr == S_OK, "Failed to get state %#lx.\n", hr);
    ok(state == MFCLOCK_STATE_STOPPED, "Unexpected state %d.\n", state);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#lx.\n", hr);
    ok(time == 0, "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time), wine_dbgstr_longlong(systime));

    hr = IMFClockStateSink_OnClockStart(statesink, 0, 0);
    ok(hr == S_OK, "Failed to start source, hr %#lx.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#lx.\n", hr);
    ok(time == systime, "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time), wine_dbgstr_longlong(systime));

    hr = IMFClockStateSink_OnClockStart(statesink, 0, 1);
    ok(hr == S_OK, "Failed to start source, hr %#lx.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#lx.\n", hr);
    ok(time == systime + 1, "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time),
            wine_dbgstr_longlong(systime));

    hr = IMFClockStateSink_OnClockPause(statesink, 2);
    ok(hr == S_OK, "Failed to pause source, hr %#lx.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#lx.\n", hr);
    ok(time == 3, "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time), wine_dbgstr_longlong(systime));

    hr = IMFClockStateSink_OnClockRestart(statesink, 5);
    ok(hr == S_OK, "Failed to restart source, hr %#lx.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#lx.\n", hr);
    ok(time == systime - 2, "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time),
            wine_dbgstr_longlong(systime));

    hr = IMFClockStateSink_OnClockPause(statesink, 0);
    ok(hr == S_OK, "Failed to pause source, hr %#lx.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#lx.\n", hr);
    ok(time == -2, "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time),
            wine_dbgstr_longlong(systime));

    hr = IMFClockStateSink_OnClockStop(statesink, 123);
    ok(hr == S_OK, "Failed to stop source, hr %#lx.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#lx.\n", hr);
    ok(time == 0, "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time), wine_dbgstr_longlong(systime));

    /* Increased rate. */
    hr = IMFClockStateSink_OnClockSetRate(statesink, 0, 2.0f);
    ok(hr == S_OK, "Failed to set rate, hr %#lx.\n", hr);

    hr = IMFClockStateSink_OnClockStart(statesink, 0, 0);
    ok(hr == S_OK, "Failed to start source, hr %#lx.\n", hr);

    hr = IMFClockStateSink_OnClockSetRate(statesink, 5, 2.0f);
    ok(hr == S_OK, "Failed to set rate, hr %#lx.\n", hr);

    hr = IMFClockStateSink_OnClockPause(statesink, 6);
    ok(hr == S_OK, "Failed to pause source, hr %#lx.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#lx.\n", hr);
    ok(time == 12 && !!systime, "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time),
            wine_dbgstr_longlong(systime));

    hr = IMFClockStateSink_OnClockRestart(statesink, 7);
    ok(hr == S_OK, "Failed to restart source, hr %#lx.\n", hr);

    hr = IMFClockStateSink_OnClockPause(statesink, 8);
    ok(hr == S_OK, "Failed to pause source, hr %#lx.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#lx.\n", hr);
    ok(time == 14 && !!systime, "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time),
            wine_dbgstr_longlong(systime));

    hr = IMFClockStateSink_OnClockStart(statesink, 0, 0);
    ok(hr == S_OK, "Failed to start source, hr %#lx.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#lx.\n", hr);
    ok(time == 2 * systime, "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time),
            wine_dbgstr_longlong(2 * systime));

    hr = IMFClockStateSink_OnClockStart(statesink, 0, 10);
    ok(hr == S_OK, "Failed to start source, hr %#lx.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#lx.\n", hr);
    ok(time == 2 * systime + 10, "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time),
            wine_dbgstr_longlong(2 * systime));

    hr = IMFClockStateSink_OnClockPause(statesink, 2);
    ok(hr == S_OK, "Failed to pause source, hr %#lx.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#lx.\n", hr);
    ok(time == 10 + 2 * 2, "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time),
            wine_dbgstr_longlong(systime));

    hr = IMFClockStateSink_OnClockRestart(statesink, 5);
    ok(hr == S_OK, "Failed to restart source, hr %#lx.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#lx.\n", hr);
    ok(time == 2 * systime + 14 - 5 * 2, "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time),
            wine_dbgstr_longlong(systime));

    hr = IMFClockStateSink_OnClockPause(statesink, 0);
    ok(hr == S_OK, "Failed to pause source, hr %#lx.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#lx.\n", hr);
    ok(time == 4, "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time),
            wine_dbgstr_longlong(systime));

    hr = IMFClockStateSink_OnClockStop(statesink, 123);
    ok(hr == S_OK, "Failed to stop source, hr %#lx.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#lx.\n", hr);
    ok(time == 0, "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time), wine_dbgstr_longlong(systime));

    hr = IMFClockStateSink_OnClockStart(statesink, 10, 0);
    ok(hr == S_OK, "Failed to start source, hr %#lx.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#lx.\n", hr);
    ok(time == 2 * systime - 2 * 10, "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time),
            wine_dbgstr_longlong(2 * systime));

    hr = IMFClockStateSink_OnClockStop(statesink, 123);
    ok(hr == S_OK, "Failed to stop source, hr %#lx.\n", hr);

    hr = IMFClockStateSink_OnClockStart(statesink, 10, 20);
    ok(hr == S_OK, "Failed to start source, hr %#lx.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#lx.\n", hr);
    ok(time == 2 * systime, "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time),
            wine_dbgstr_longlong(2 * systime));

    hr = IMFClockStateSink_OnClockPause(statesink, 2);
    ok(hr == S_OK, "Failed to pause source, hr %#lx.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#lx.\n", hr);
    ok(time == 2 * 2, "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time),
            wine_dbgstr_longlong(systime));

    hr = IMFClockStateSink_OnClockRestart(statesink, 5);
    ok(hr == S_OK, "Failed to restart source, hr %#lx.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#lx.\n", hr);
    ok(time == 2 * systime + 4 - 5 * 2, "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time),
            wine_dbgstr_longlong(systime));

    hr = IMFClockStateSink_OnClockPause(statesink, 0);
    ok(hr == S_OK, "Failed to pause source, hr %#lx.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#lx.\n", hr);
    ok(time == -6, "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time),
            wine_dbgstr_longlong(systime));

    IMFClockStateSink_Release(statesink);
    IMFPresentationTimeSource_Release(time_source);

    /* PRESENTATION_CURRENT_POSITION */
    hr = MFCreateSystemTimeSource(&time_source);
    ok(hr == S_OK, "Failed to create time source, hr %#lx.\n", hr);

    hr = IMFPresentationTimeSource_QueryInterface(time_source, &IID_IMFClockStateSink, (void **)&statesink);
    ok(hr == S_OK, "Failed to get sink interface, hr %#lx.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#lx.\n", hr);
    ok(!time && systime, "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time),
            wine_dbgstr_longlong(systime));

    /* INVALID -> RUNNING */
    hr = IMFClockStateSink_OnClockStart(statesink, 10, PRESENTATION_CURRENT_POSITION);
    ok(hr == S_OK, "Failed to start source, hr %#lx.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#lx.\n", hr);
    ok(time == systime - 10, "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time),
            wine_dbgstr_longlong(systime));

    /* RUNNING -> RUNNING */
    hr = IMFClockStateSink_OnClockStart(statesink, 20, PRESENTATION_CURRENT_POSITION);
    ok(hr == S_OK, "Failed to start source, hr %#lx.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#lx.\n", hr);
    ok(time == systime - 10, "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time),
            wine_dbgstr_longlong(systime));

    hr = IMFClockStateSink_OnClockStart(statesink, 0, PRESENTATION_CURRENT_POSITION);
    ok(hr == S_OK, "Failed to start source, hr %#lx.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#lx.\n", hr);
    ok(time == systime - 10, "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time),
            wine_dbgstr_longlong(systime));

    hr = IMFClockStateSink_OnClockStart(statesink, 0, 0);
    ok(hr == S_OK, "Failed to start source, hr %#lx.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#lx.\n", hr);
    ok(time == systime, "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time),
            wine_dbgstr_longlong(systime));

    hr = IMFClockStateSink_OnClockStart(statesink, 30, PRESENTATION_CURRENT_POSITION);
    ok(hr == S_OK, "Failed to start source, hr %#lx.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#lx.\n", hr);
    ok(time == systime, "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time),
            wine_dbgstr_longlong(systime));

    /* STOPPED -> RUNNING */
    hr = IMFClockStateSink_OnClockStop(statesink, 567);
    ok(hr == S_OK, "Failed to stop source, hr %#lx.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#lx.\n", hr);
    ok(!time && systime != 0, "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time),
            wine_dbgstr_longlong(systime));

    hr = IMFClockStateSink_OnClockStart(statesink, 30, PRESENTATION_CURRENT_POSITION);
    ok(hr == S_OK, "Failed to start source, hr %#lx.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#lx.\n", hr);
    ok(time == systime - 30, "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time),
            wine_dbgstr_longlong(systime));

    /* PAUSED -> RUNNING */
    hr = IMFClockStateSink_OnClockPause(statesink, 8);
    ok(hr == S_OK, "Failed to pause source, hr %#lx.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#lx.\n", hr);
    ok(time == (-30 + 8) && systime != 0, "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time),
            wine_dbgstr_longlong(systime));

    hr = IMFClockStateSink_OnClockStart(statesink, 40, PRESENTATION_CURRENT_POSITION);
    ok(hr == S_OK, "Failed to start source, hr %#lx.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#lx.\n", hr);
    ok(time == systime + (-30 + 8 - 40), "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time),
            wine_dbgstr_longlong(systime));

    hr = IMFClockStateSink_OnClockPause(statesink, 7);
    ok(hr == S_OK, "Failed to pause source, hr %#lx.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#lx.\n", hr);
    ok(time == (-30 + 8 - 40 + 7) && systime != 0, "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time),
            wine_dbgstr_longlong(systime));

    hr = IMFClockStateSink_OnClockStart(statesink, 50, 7);
    ok(hr == S_OK, "Failed to start source, hr %#lx.\n", hr);

    hr = IMFPresentationTimeSource_GetCorrelatedTime(time_source, 0, &time, &systime);
    ok(hr == S_OK, "Failed to get time %#lx.\n", hr);
    ok(time == systime + (-50 + 7), "Unexpected time stamp %s, %s.\n", wine_dbgstr_longlong(time),
            wine_dbgstr_longlong(systime));

    IMFClockStateSink_Release(statesink);
    IMFPresentationTimeSource_Release(time_source);
}

static void test_MFInvokeCallback(void)
{
    struct test_callback *callback;
    IMFAsyncResult *result;
    MFASYNCRESULT *data;
    ULONG refcount;
    HRESULT hr;
    DWORD ret;

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Failed to start up, hr %#lx.\n", hr);

    callback = create_test_callback(NULL);

    hr = MFCreateAsyncResult(NULL, &callback->IMFAsyncCallback_iface, NULL, &result);
    ok(hr == S_OK, "Failed to create object, hr %#lx.\n", hr);

    data = (MFASYNCRESULT *)result;
    data->hEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
    ok(data->hEvent != NULL, "Failed to create event.\n");

    hr = MFInvokeCallback(result);
    ok(hr == S_OK, "Failed to invoke, hr %#lx.\n", hr);

    ret = WaitForSingleObject(data->hEvent, 100);
    ok(ret == WAIT_TIMEOUT, "Expected timeout, ret %#lx.\n", ret);

    refcount = IMFAsyncResult_Release(result);
    ok(!refcount, "Unexpected refcount %lu.\n", refcount);

    IMFAsyncCallback_Release(&callback->IMFAsyncCallback_iface);

    hr = MFShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);
}

static void test_stream_descriptor(void)
{
    IMFMediaType *media_types[2], *media_type, *media_type2, *media_type3;
    IMFMediaTypeHandler *type_handler;
    IMFStreamDescriptor *stream_desc;
    GUID major_type;
    DWORD id, count;
    unsigned int i;
    HRESULT hr;

    hr = MFCreateStreamDescriptor(123, 0, NULL, &stream_desc);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    for (i = 0; i < ARRAY_SIZE(media_types); ++i)
    {
        hr = MFCreateMediaType(&media_types[i]);
        ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);
    }

    hr = MFCreateStreamDescriptor(123, 0, media_types, &stream_desc);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateStreamDescriptor(123, ARRAY_SIZE(media_types), media_types, &stream_desc);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFStreamDescriptor_GetStreamIdentifier(stream_desc, &id);
    ok(hr == S_OK, "Failed to get descriptor id, hr %#lx.\n", hr);
    ok(id == 123, "Unexpected id %#lx.\n", id);

    hr = IMFStreamDescriptor_GetMediaTypeHandler(stream_desc, &type_handler);
    ok(hr == S_OK, "Failed to get type handler, hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_GetMediaTypeCount(type_handler, &count);
    ok(hr == S_OK, "Failed to get type count, hr %#lx.\n", hr);
    ok(count == ARRAY_SIZE(media_types), "Unexpected type count.\n");

    hr = IMFMediaTypeHandler_GetCurrentMediaType(type_handler, &media_type);
    ok(hr == MF_E_NOT_INITIALIZED, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_GetMajorType(type_handler, &major_type);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    for (i = 0; i < ARRAY_SIZE(media_types); ++i)
    {
        hr = IMFMediaTypeHandler_GetMediaTypeByIndex(type_handler, i, &media_type);
        ok(hr == S_OK, "Failed to get media type, hr %#lx.\n", hr);
        ok(media_type == media_types[i], "Unexpected object.\n");

        if (SUCCEEDED(hr))
            IMFMediaType_Release(media_type);
    }

    hr = IMFMediaTypeHandler_GetMediaTypeByIndex(type_handler, 2, &media_type);
    ok(hr == MF_E_NO_MORE_TYPES, "Unexpected hr %#lx.\n", hr);

    /* IsMediaTypeSupported() */

    hr = IMFMediaTypeHandler_IsMediaTypeSupported(type_handler, NULL, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_IsMediaTypeSupported(type_handler, NULL, &media_type2);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);

    hr = MFCreateMediaType(&media_type3);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);

    media_type2 = (void *)0xdeadbeef;
    hr = IMFMediaTypeHandler_IsMediaTypeSupported(type_handler, media_type, &media_type2);
    ok(hr == MF_E_INVALIDMEDIATYPE, "Unexpected hr %#lx.\n", hr);
    ok(!media_type2, "Unexpected pointer.\n");

    hr = IMFMediaTypeHandler_SetCurrentMediaType(type_handler, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_SetCurrentMediaType(type_handler, media_type);
    ok(hr == S_OK, "Failed to set current type, hr %#lx.\n", hr);

    media_type2 = (void *)0xdeadbeef;
    hr = IMFMediaTypeHandler_IsMediaTypeSupported(type_handler, media_type, &media_type2);
    ok(hr == MF_E_INVALIDMEDIATYPE, "Unexpected hr %#lx.\n", hr);
    ok(!media_type2, "Unexpected pointer.\n");

    hr = IMFMediaTypeHandler_GetMajorType(type_handler, &major_type);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Failed to set major type, hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_GetMajorType(type_handler, &major_type);
    ok(hr == S_OK, "Failed to get major type, hr %#lx.\n", hr);
    ok(IsEqualGUID(&major_type, &MFMediaType_Audio), "Unexpected major type.\n");

    /* Mismatching major types. */
    hr = IMFMediaType_SetGUID(media_type3, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Failed to set major type, hr %#lx.\n", hr);

    media_type2 = (void *)0xdeadbeef;
    hr = IMFMediaTypeHandler_IsMediaTypeSupported(type_handler, media_type3, &media_type2);
    ok(hr == MF_E_INVALIDMEDIATYPE, "Unexpected hr %#lx.\n", hr);
    ok(!media_type2, "Unexpected pointer.\n");

    /* Subtype missing. */
    hr = IMFMediaType_SetGUID(media_type3, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFAudioFormat_PCM);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    media_type2 = (void *)0xdeadbeef;
    hr = IMFMediaTypeHandler_IsMediaTypeSupported(type_handler, media_type3, &media_type2);
    ok(hr == MF_E_INVALIDMEDIATYPE, "Unexpected hr %#lx.\n", hr);
    ok(!media_type2, "Unexpected pointer.\n");

    hr = IMFMediaType_SetGUID(media_type3, &MF_MT_SUBTYPE, &MFAudioFormat_PCM);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    media_type2 = (void *)0xdeadbeef;
    hr = IMFMediaTypeHandler_IsMediaTypeSupported(type_handler, media_type3, &media_type2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!media_type2, "Unexpected pointer.\n");

    /* Mismatching subtype. */
    hr = IMFMediaType_SetGUID(media_type3, &MF_MT_SUBTYPE, &MFAudioFormat_MP3);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    media_type2 = (void *)0xdeadbeef;
    hr = IMFMediaTypeHandler_IsMediaTypeSupported(type_handler, media_type3, &media_type2);
    ok(hr == MF_E_INVALIDMEDIATYPE, "Unexpected hr %#lx.\n", hr);
    ok(!media_type2, "Unexpected pointer.\n");

    hr = IMFMediaTypeHandler_GetMediaTypeCount(type_handler, &count);
    ok(hr == S_OK, "Failed to get type count, hr %#lx.\n", hr);
    ok(count == ARRAY_SIZE(media_types), "Unexpected type count.\n");

    IMFMediaTypeHandler_Release(type_handler);
    IMFStreamDescriptor_Release(stream_desc);

    /* IsMediaTypeSupported() for unset current type. */
    hr = MFCreateStreamDescriptor(123, ARRAY_SIZE(media_types), media_types, &stream_desc);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFStreamDescriptor_GetMediaTypeHandler(stream_desc, &type_handler);
    ok(hr == S_OK, "Failed to get type handler, hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_IsMediaTypeSupported(type_handler, media_type3, NULL);
    ok(hr == MF_E_INVALIDMEDIATYPE, "Unexpected hr %#lx.\n", hr);

    /* Initialize one from initial type set. */
    hr = IMFMediaType_CopyAllItems(media_type3, (IMFAttributes *)media_types[0]);
    ok(hr == S_OK, "Failed to copy attributes, hr %#lx.\n", hr);

    media_type2 = (void *)0xdeadbeef;
    hr = IMFMediaTypeHandler_IsMediaTypeSupported(type_handler, media_type3, &media_type2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!media_type2, "Unexpected pointer.\n");

    hr = IMFMediaType_SetGUID(media_type3, &MF_MT_SUBTYPE, &MFAudioFormat_PCM);
    ok(hr == S_OK, "Failed to copy attributes, hr %#lx.\n", hr);

    media_type2 = (void *)0xdeadbeef;
    hr = IMFMediaTypeHandler_IsMediaTypeSupported(type_handler, media_type3, &media_type2);
    ok(hr == MF_E_INVALIDMEDIATYPE, "Unexpected hr %#lx.\n", hr);
    ok(!media_type2, "Unexpected pointer.\n");

    hr = IMFMediaType_SetGUID(media_type3, &MF_MT_SUBTYPE, &MFAudioFormat_MP3);
    ok(hr == S_OK, "Failed to copy attributes, hr %#lx.\n", hr);

    media_type2 = (void *)0xdeadbeef;
    hr = IMFMediaTypeHandler_IsMediaTypeSupported(type_handler, media_type3, &media_type2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!media_type2, "Unexpected pointer.\n");

    /* Now set current type that's not compatible. */
    hr = IMFMediaType_SetGUID(media_type3, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Failed to copy attributes, hr %#lx.\n", hr);

    hr = IMFMediaType_SetGUID(media_type3, &MF_MT_SUBTYPE, &MFVideoFormat_RGB8);
    ok(hr == S_OK, "Failed to copy attributes, hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_SetCurrentMediaType(type_handler, media_type3);
    ok(hr == S_OK, "Failed to set current type, hr %#lx.\n", hr);

    media_type2 = (void *)0xdeadbeef;
    hr = IMFMediaTypeHandler_IsMediaTypeSupported(type_handler, media_type3, &media_type2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!media_type2, "Unexpected pointer.\n");

    hr = IMFMediaType_CopyAllItems(media_types[0], (IMFAttributes *)media_type);
    ok(hr == S_OK, "Failed to copy attributes, hr %#lx.\n", hr);

    media_type2 = (void *)0xdeadbeef;
    hr = IMFMediaTypeHandler_IsMediaTypeSupported(type_handler, media_type, &media_type2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!media_type2, "Unexpected pointer.\n");

    IMFMediaType_Release(media_type);
    IMFMediaType_Release(media_type3);

    IMFMediaTypeHandler_Release(type_handler);

    IMFStreamDescriptor_Release(stream_desc);

    /* Major type is returned for first entry. */
    hr = MFCreateMediaType(&media_types[0]);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFCreateMediaType(&media_types[1]);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaType_SetGUID(media_types[0], &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_types[1], &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateStreamDescriptor(0, 2, media_types, &stream_desc);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFStreamDescriptor_GetMediaTypeHandler(stream_desc, &type_handler);
    ok(hr == S_OK, "Failed to get type handler, hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_GetMajorType(type_handler, &major_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&major_type, &MFMediaType_Audio), "Unexpected major type %s.\n", wine_dbgstr_guid(&major_type));

    hr = IMFMediaType_SetGUID(media_types[0], &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_types[1], &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaTypeHandler_GetMajorType(type_handler, &major_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&major_type, &MFMediaType_Video), "Unexpected major type %s.\n", wine_dbgstr_guid(&major_type));

    IMFMediaType_Release(media_types[0]);
    IMFMediaType_Release(media_types[1]);

    IMFMediaTypeHandler_Release(type_handler);
    IMFStreamDescriptor_Release(stream_desc);
}

static const struct image_size_test
{
    const GUID *subtype;
    UINT32 width;
    UINT32 height;
    UINT32 size;
    UINT32 plane_size; /* Matches image size when 0. */
    UINT32 max_length;
    UINT32 contiguous_length;
    UINT32 pitch;
}
image_size_tests[] =
{
    /* RGB */
    { &MFVideoFormat_RGB8, 3, 5, 20, 0, 320, 20, 64 },
    { &MFVideoFormat_RGB8, 1, 1, 4, 0, 64, 4, 64 },
    { &MFVideoFormat_RGB8, 320, 240, 76800, 0, 76800, 76800, 320 },
    { &MFVideoFormat_RGB555, 3, 5, 40, 0, 320, 40, 64 },
    { &MFVideoFormat_RGB555, 1, 1, 4, 0, 64, 4, 64 },
    { &MFVideoFormat_RGB555, 320, 240, 153600, 0, 153600, 153600, 640 },
    { &MFVideoFormat_RGB565, 3, 5, 40, 0, 320, 40, 64 },
    { &MFVideoFormat_RGB565, 1, 1, 4, 0, 64, 4, 64 },
    { &MFVideoFormat_RGB565, 320, 240, 153600, 0, 153600, 153600, 640 },
    { &MFVideoFormat_RGB24, 3, 5, 60, 0, 320, 60, 64 },
    { &MFVideoFormat_RGB24, 1, 1, 4, 0, 64, 4, 64 },
    { &MFVideoFormat_RGB24, 4, 3, 36, 0, 192, 36, 64 },
    { &MFVideoFormat_RGB24, 320, 240, 230400, 0, 230400, 230400, 960 },
    { &MFVideoFormat_RGB32, 3, 5, 60, 0, 320, 60, 64 },
    { &MFVideoFormat_RGB32, 1, 1, 4, 0, 64, 4, 64 },
    { &MFVideoFormat_RGB32, 320, 240, 307200, 0, 307200, 307200, 1280 },
    { &MFVideoFormat_ARGB32, 3, 5, 60, 0, 320, 60, 64 },
    { &MFVideoFormat_ARGB32, 1, 1, 4, 0, 64, 4, 64 },
    { &MFVideoFormat_ARGB32, 320, 240, 307200, 0, 307200, 307200, 1280 },
    { &MFVideoFormat_ABGR32, 3, 5, 60, 0, 320, 60, 64 },
    { &MFVideoFormat_ABGR32, 1, 1, 4, 0, 64, 4, 64 },
    { &MFVideoFormat_ABGR32, 320, 240, 307200, 0, 307200, 307200, 1280 },
    { &MFVideoFormat_A2R10G10B10, 3, 5, 60, 0, 320, 60, 64 },
    { &MFVideoFormat_A2R10G10B10, 1, 1, 4, 0, 64, 4, 64 },
    { &MFVideoFormat_A2R10G10B10, 320, 240, 307200, 0, 307200, 307200, 1280 },
    { &MFVideoFormat_A2B10G10R10, 3, 5, 60, 0, 320, 60, 64 },
    { &MFVideoFormat_A2B10G10R10, 1, 1, 4, 0, 64, 4, 64 },
    { &MFVideoFormat_A2B10G10R10, 320, 240, 307200, 0, 307200, 307200, 1280 },
    { &MFVideoFormat_A16B16G16R16F, 3, 5, 120, 0, 320, 120, 64 },
    { &MFVideoFormat_A16B16G16R16F, 1, 1, 8, 0, 64, 8, 64 },
    { &MFVideoFormat_A16B16G16R16F, 320, 240, 614400, 0, 614400, 614400, 2560 },

    { &MEDIASUBTYPE_RGB8,   3, 5, 20 },
    { &MEDIASUBTYPE_RGB8,   1, 1, 4  },
    { &MEDIASUBTYPE_RGB555, 3, 5, 40 },
    { &MEDIASUBTYPE_RGB555, 1, 1, 4  },
    { &MEDIASUBTYPE_RGB565, 3, 5, 40 },
    { &MEDIASUBTYPE_RGB565, 1, 1, 4  },
    { &MEDIASUBTYPE_RGB24,  3, 5, 60 },
    { &MEDIASUBTYPE_RGB24,  1, 1, 4  },
    { &MEDIASUBTYPE_RGB24,  4, 3, 36 },
    { &MEDIASUBTYPE_RGB32,  3, 5, 60 },
    { &MEDIASUBTYPE_RGB32,  1, 1, 4  },

    /* YUV 4:4:4, 32 bpp, packed */
    { &MFVideoFormat_AYUV, 1, 1, 4, 0, 64, 4, 64 },
    { &MFVideoFormat_AYUV, 2, 1, 8, 0, 64, 8, 64 },
    { &MFVideoFormat_AYUV, 1, 2, 8, 0, 128, 8, 64 },
    { &MFVideoFormat_AYUV, 4, 3, 48, 0, 192, 48, 64 },
    { &MFVideoFormat_AYUV, 320, 240, 307200, 0, 307200, 307200, 1280 },

    /* YUV 4:2:2, 16 bpp, packed */
    { &MFVideoFormat_YUY2, 2, 1, 4, 0, 64, 4, 64 },
    { &MFVideoFormat_YUY2, 4, 3, 24, 0, 192, 24, 64 },
    { &MFVideoFormat_YUY2, 128, 128, 32768, 0, 32768, 32768, 256 },
    { &MFVideoFormat_YUY2, 320, 240, 153600, 0, 153600, 153600, 640 },

    { &MFVideoFormat_UYVY, 2, 1, 4, 0, 64, 4, 64 },
    { &MFVideoFormat_UYVY, 4, 3, 24, 0, 192, 24, 64 },
    { &MFVideoFormat_UYVY, 128, 128, 32768, 0, 32768, 32768, 256 },
    { &MFVideoFormat_UYVY, 320, 240, 153600, 0, 153600, 153600, 640 },

    /* YUV 4:2:0, 16 bpp, planar (the secondary plane has the same
     * height, half the width and the same stride as the primary
     * one) */
    { &MFVideoFormat_IMC1, 1, 1, 4, 0, 256, 8, 128 },
    { &MFVideoFormat_IMC1, 2, 1, 4, 0, 256, 8, 128 },
    { &MFVideoFormat_IMC1, 1, 2, 8, 0, 512, 16, 128 },
    { &MFVideoFormat_IMC1, 2, 2, 8, 0, 512, 16, 128 },
    { &MFVideoFormat_IMC1, 2, 4, 16, 0, 1024, 32, 128 },
    { &MFVideoFormat_IMC1, 4, 2, 16, 0, 512, 32, 128 },
    { &MFVideoFormat_IMC1, 4, 3, 24, 0, 768, 48, 128 },
    { &MFVideoFormat_IMC1, 320, 240, 153600, 0, 307200, 307200, 640 },

    { &MFVideoFormat_IMC3, 1, 1, 4, 0, 256, 8, 128 },
    { &MFVideoFormat_IMC3, 2, 1, 4, 0, 256, 8, 128 },
    { &MFVideoFormat_IMC3, 1, 2, 8, 0, 512, 16, 128 },
    { &MFVideoFormat_IMC3, 2, 2, 8, 0, 512, 16, 128 },
    { &MFVideoFormat_IMC3, 2, 4, 16, 0, 1024, 32, 128 },
    { &MFVideoFormat_IMC3, 4, 2, 16, 0, 512, 32, 128 },
    { &MFVideoFormat_IMC3, 4, 3, 24, 0, 768, 48, 128 },
    { &MFVideoFormat_IMC3, 320, 240, 153600, 0, 307200, 307200, 640 },

    /* YUV 4:2:0, 12 bpp, planar, full stride (the secondary plane has
     * half the height, the same width and the same stride as the
     * primary one) */
    { &MFVideoFormat_NV12, 1, 3, 9, 4, 288, 4, 64 },
    { &MFVideoFormat_NV12, 1, 2, 6, 3, 192, 3, 64 },
    { &MFVideoFormat_NV12, 2, 2, 6, 6, 192, 6, 64 },
    { &MFVideoFormat_NV12, 2, 4, 12, 0, 384, 12, 64 },
    { &MFVideoFormat_NV12, 3, 2, 12, 9, 192, 9, 64 },
    { &MFVideoFormat_NV12, 4, 2, 12, 0, 192, 12, 64 },
    { &MFVideoFormat_NV12, 320, 240, 115200, 0, 115200, 115200, 320 },

    /* YUV 4:2:0, 12 bpp, planar, half stride (the secondary plane has
     * the same height, half the width and half the stride of the
     * primary one) */
    { &MFVideoFormat_IMC2, 1, 1, 3, 1, 192, 1, 128 },
    { &MFVideoFormat_IMC2, 1, 2, 6, 3, 384, 2, 128 },
    { &MFVideoFormat_IMC2, 1, 3, 9, 4, 576, 3, 128 },
    { &MFVideoFormat_IMC2, 2, 1, 3, 0, 192, 3, 128 },
    { &MFVideoFormat_IMC2, 2, 2, 6, 6, 384, 6, 128 },
    { &MFVideoFormat_IMC2, 2, 4, 12, 0, 768, 12, 128 },
    { &MFVideoFormat_IMC2, 3, 2, 12, 9, 384, 8, 128 },
    { &MFVideoFormat_IMC2, 3, 5, 30, 22, 960, 20, 128 },
    { &MFVideoFormat_IMC2, 4, 2, 12, 0, 384, 12, 128 },
    { &MFVideoFormat_IMC2, 4, 3, 18, 0, 576, 18, 128 },
    { &MFVideoFormat_IMC2, 320, 240, 115200, 0, 138240, 115200, 384 },

    { &MFVideoFormat_IMC4, 1, 1, 3, 1, 192, 1, 128 },
    { &MFVideoFormat_IMC4, 1, 2, 6, 3, 384, 2, 128 },
    { &MFVideoFormat_IMC4, 1, 3, 9, 4, 576, 3, 128 },
    { &MFVideoFormat_IMC4, 2, 1, 3, 0, 192, 3, 128 },
    { &MFVideoFormat_IMC4, 2, 2, 6, 6, 384, 6, 128 },
    { &MFVideoFormat_IMC4, 2, 4, 12, 0, 768, 12, 128 },
    { &MFVideoFormat_IMC4, 3, 2, 12, 9, 384, 8, 128 },
    { &MFVideoFormat_IMC4, 3, 5, 30, 22, 960, 20, 128 },
    { &MFVideoFormat_IMC4, 4, 2, 12, 0, 384, 12, 128 },
    { &MFVideoFormat_IMC4, 4, 3, 18, 0, 576, 18, 128 },
    { &MFVideoFormat_IMC4, 320, 240, 115200, 0, 138240, 115200, 384 },

    /* YUV 4:1:1, 12 bpp, semi-planar */
    { &MFVideoFormat_NV11, 1,   3,   18,     4,  576,    3,      128 },
    { &MFVideoFormat_NV11, 1,   2,   12,     3,  384,    2,      128 },
    { &MFVideoFormat_NV11, 2,   2,   12,     6,  384,    6,      128 },
    { &MFVideoFormat_NV11, 2,   4,   24,     12, 768,    12,     128 },
    { &MFVideoFormat_NV11, 3,   2,   12,     9,  384,    8,      128 },
    { &MFVideoFormat_NV11, 4,   2,   12,     0,  384,    12,     128 },
    { &MFVideoFormat_NV11, 320, 240, 115200, 0,  138240, 115200, 384 },

    { &MFVideoFormat_YV12, 1, 1, 3, 1, 192, 1, 128 },
    { &MFVideoFormat_YV12, 1, 2, 6, 3, 384, 2, 128 },
    { &MFVideoFormat_YV12, 1, 3, 9, 4, 576, 3, 128 },
    { &MFVideoFormat_YV12, 2, 1, 3, 0, 192, 3, 128 },
    { &MFVideoFormat_YV12, 2, 2, 6, 6, 384, 6, 128 },
    { &MFVideoFormat_YV12, 2, 4, 12, 0, 768, 12, 128 },
    { &MFVideoFormat_YV12, 3, 2, 12, 9, 384, 8, 128 },
    { &MFVideoFormat_YV12, 3, 5, 30, 22, 960, 20, 128 },
    { &MFVideoFormat_YV12, 4, 2, 12, 0, 384, 12, 128 },
    { &MFVideoFormat_YV12, 4, 3, 18, 0, 576, 18, 128 },
    { &MFVideoFormat_YV12, 320, 240, 115200, 0, 138240, 115200, 384 },

    { &MFVideoFormat_I420, 1, 1, 3, 1, 192, 1, 128 },
    { &MFVideoFormat_I420, 1, 2, 6, 3, 384, 2, 128 },
    { &MFVideoFormat_I420, 1, 3, 9, 4, 576, 3, 128 },
    { &MFVideoFormat_I420, 2, 1, 3, 0, 192, 3, 128 },
    { &MFVideoFormat_I420, 2, 2, 6, 6, 384, 6, 128 },
    { &MFVideoFormat_I420, 2, 4, 12, 0, 768, 12, 128 },
    { &MFVideoFormat_I420, 3, 2, 12, 9, 384, 8, 128 },
    { &MFVideoFormat_I420, 3, 5, 30, 22, 960, 20, 128 },
    { &MFVideoFormat_I420, 4, 2, 12, 0, 384, 12, 128 },
    { &MFVideoFormat_I420, 4, 3, 18, 0, 576, 18, 128 },
    { &MFVideoFormat_I420, 320, 240, 115200, 0, 138240, 115200, 384 },

    { &MFVideoFormat_IYUV, 1, 1, 3, 1, 192, 1, 128 },
    { &MFVideoFormat_IYUV, 1, 2, 6, 3, 384, 2, 128 },
    { &MFVideoFormat_IYUV, 1, 3, 9, 4, 576, 3, 128 },
    { &MFVideoFormat_IYUV, 2, 1, 3, 0, 192, 3, 128 },
    { &MFVideoFormat_IYUV, 2, 2, 6, 6, 384, 6, 128 },
    { &MFVideoFormat_IYUV, 2, 4, 12, 0, 768, 12, 128 },
    { &MFVideoFormat_IYUV, 3, 2, 12, 9, 384, 8, 128 },
    { &MFVideoFormat_IYUV, 3, 5, 30, 22, 960, 20, 128 },
    { &MFVideoFormat_IYUV, 4, 2, 12, 0, 384, 12, 128 },
    { &MFVideoFormat_IYUV, 4, 3, 18, 0, 576, 18, 128 },
    { &MFVideoFormat_IYUV, 320, 240, 115200, 0, 138240, 115200, 384 },
};

static void test_MFCalculateImageSize(void)
{
    unsigned int i;
    UINT32 size;
    HRESULT hr;

    size = 1;
    hr = MFCalculateImageSize(&IID_IUnknown, 1, 1, &size);
    ok(hr == E_INVALIDARG || broken(hr == S_OK) /* Vista */, "Unexpected hr %#lx.\n", hr);
    ok(size == 0, "Unexpected size %u.\n", size);

    for (i = 0; i < ARRAY_SIZE(image_size_tests); ++i)
    {
        const struct image_size_test *ptr = &image_size_tests[i];

        /* Those are supported since Win10. */
        BOOL is_broken = IsEqualGUID(ptr->subtype, &MFVideoFormat_A16B16G16R16F) ||
                IsEqualGUID(ptr->subtype, &MFVideoFormat_A2R10G10B10) ||
                IsEqualGUID(ptr->subtype, &MFVideoFormat_ABGR32);

        hr = MFCalculateImageSize(ptr->subtype, ptr->width, ptr->height, &size);
        ok(hr == S_OK || broken(is_broken && hr == E_INVALIDARG), "%u: failed to calculate image size, hr %#lx.\n", i, hr);
        if (hr == S_OK)
        {
            ok(size == ptr->size, "%u: unexpected image size %u, expected %u. Size %u x %u, format %s.\n", i, size, ptr->size,
                    ptr->width, ptr->height, wine_dbgstr_an((char *)&ptr->subtype->Data1, 4));
        }
    }
}

static void test_MFGetPlaneSize(void)
{
    unsigned int i;
    DWORD size;
    HRESULT hr;

    if (!pMFGetPlaneSize)
    {
        win_skip("MFGetPlaneSize() is not available.\n");
        return;
    }

    size = 1;
    hr = pMFGetPlaneSize(0xdeadbeef, 64, 64, &size);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(size == 0, "Unexpected size %lu.\n", size);

    for (i = 0; i < ARRAY_SIZE(image_size_tests); ++i)
    {
        const struct image_size_test *ptr = &image_size_tests[i];
        unsigned int plane_size = ptr->plane_size ? ptr->plane_size : ptr->size;
        if ((is_MEDIASUBTYPE_RGB(ptr->subtype)))
            continue;

        hr = pMFGetPlaneSize(ptr->subtype->Data1, ptr->width, ptr->height, &size);
        ok(hr == S_OK, "%u: failed to get plane size, hr %#lx.\n", i, hr);
        ok(size == plane_size, "%u: unexpected plane size %lu, expected %u. Size %u x %u, format %s.\n", i, size, plane_size,
                ptr->width, ptr->height, wine_dbgstr_an((char*)&ptr->subtype->Data1, 4));
    }
}

static void test_MFCompareFullToPartialMediaType(void)
{
    IMFMediaType *full_type, *partial_type;
    HRESULT hr;
    BOOL ret;

    hr = MFCreateMediaType(&full_type);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);

    hr = MFCreateMediaType(&partial_type);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);

    ret = MFCompareFullToPartialMediaType(full_type, partial_type);
    ok(!ret, "Unexpected result %d.\n", ret);

    hr = IMFMediaType_SetGUID(full_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Failed to set major type, hr %#lx.\n", hr);

    hr = IMFMediaType_SetGUID(partial_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Failed to set major type, hr %#lx.\n", hr);

    ret = MFCompareFullToPartialMediaType(full_type, partial_type);
    ok(ret, "Unexpected result %d.\n", ret);

    hr = IMFMediaType_SetGUID(full_type, &MF_MT_SUBTYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Failed to set major type, hr %#lx.\n", hr);

    ret = MFCompareFullToPartialMediaType(full_type, partial_type);
    ok(ret, "Unexpected result %d.\n", ret);

    hr = IMFMediaType_SetGUID(partial_type, &MF_MT_SUBTYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Failed to set major type, hr %#lx.\n", hr);

    ret = MFCompareFullToPartialMediaType(full_type, partial_type);
    ok(!ret, "Unexpected result %d.\n", ret);

    IMFMediaType_Release(full_type);
    IMFMediaType_Release(partial_type);
}

static void test_attributes_serialization(void)
{
    static const UINT8 blob[] = {1,2,3};
    IMFAttributes *attributes, *dest;
    UINT32 size, count, value32;
    double value_dbl;
    UINT64 value64;
    UINT8 *buffer;
    IUnknown *obj;
    HRESULT hr;
    WCHAR *str;
    GUID guid;

    hr = MFCreateAttributes(&attributes, 0);
    ok(hr == S_OK, "Failed to create object, hr %#lx.\n", hr);

    hr = MFCreateAttributes(&dest, 0);
    ok(hr == S_OK, "Failed to create object, hr %#lx.\n", hr);

    hr = MFGetAttributesAsBlobSize(attributes, &size);
    ok(hr == S_OK, "Failed to get blob size, hr %#lx.\n", hr);
    ok(size == 8, "Got size %u.\n", size);

    buffer = malloc(size);

    hr = MFGetAttributesAsBlob(attributes, buffer, size);
    ok(hr == S_OK, "Failed to serialize, hr %#lx.\n", hr);

    hr = MFGetAttributesAsBlob(attributes, buffer, size - 1);
    ok(hr == MF_E_BUFFERTOOSMALL, "Unexpected hr %#lx.\n", hr);

    hr = MFInitAttributesFromBlob(dest, buffer, size - 1);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IMFAttributes_SetUINT32(dest, &MF_MT_MAJOR_TYPE, 1);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    hr = MFInitAttributesFromBlob(dest, buffer, size);
    ok(hr == S_OK, "Failed to deserialize, hr %#lx.\n", hr);

    /* Previous items are cleared. */
    hr = IMFAttributes_GetCount(dest, &count);
    ok(hr == S_OK, "Failed to get attribute count, hr %#lx.\n", hr);
    ok(count == 0, "Unexpected count %u.\n", count);

    free(buffer);

    /* Set some attributes of various types. */
    IMFAttributes_SetUINT32(attributes, &MF_MT_MAJOR_TYPE, 456);
    IMFAttributes_SetUINT64(attributes, &MF_MT_SUBTYPE, 123);
    IMFAttributes_SetDouble(attributes, &IID_IUnknown, 0.5);
    IMFAttributes_SetUnknown(attributes, &IID_IMFAttributes, (IUnknown *)attributes);
    IMFAttributes_SetGUID(attributes, &GUID_NULL, &IID_IUnknown);
    IMFAttributes_SetString(attributes, &DUMMY_CLSID, L"Text");
    IMFAttributes_SetBlob(attributes, &DUMMY_GUID1, blob, sizeof(blob));

    hr = MFGetAttributesAsBlobSize(attributes, &size);
    ok(hr == S_OK, "Failed to get blob size, hr %#lx.\n", hr);
    ok(size > 8, "Got unexpected size %u.\n", size);

    buffer = malloc(size);
    hr = MFGetAttributesAsBlob(attributes, buffer, size);
    ok(hr == S_OK, "Failed to serialize, hr %#lx.\n", hr);
    hr = MFInitAttributesFromBlob(dest, buffer, size);
    ok(hr == S_OK, "Failed to deserialize, hr %#lx.\n", hr);
    free(buffer);

    hr = IMFAttributes_GetUINT32(dest, &MF_MT_MAJOR_TYPE, &value32);
    ok(hr == S_OK, "Failed to get get uint32 value, hr %#lx.\n", hr);
    ok(value32 == 456, "Unexpected value %u.\n", value32);
    hr = IMFAttributes_GetUINT64(dest, &MF_MT_SUBTYPE, &value64);
    ok(hr == S_OK, "Failed to get get uint64 value, hr %#lx.\n", hr);
    ok(value64 == 123, "Unexpected value.\n");
    hr = IMFAttributes_GetDouble(dest, &IID_IUnknown, &value_dbl);
    ok(hr == S_OK, "Failed to get get double value, hr %#lx.\n", hr);
    ok(value_dbl == 0.5, "Unexpected value.\n");
    hr = IMFAttributes_GetUnknown(dest, &IID_IMFAttributes, &IID_IUnknown, (void **)&obj);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    hr = IMFAttributes_GetGUID(dest, &GUID_NULL, &guid);
    ok(hr == S_OK, "Failed to get guid value, hr %#lx.\n", hr);
    ok(IsEqualGUID(&guid, &IID_IUnknown), "Unexpected guid.\n");
    hr = IMFAttributes_GetAllocatedString(dest, &DUMMY_CLSID, &str, &size);
    ok(hr == S_OK, "Failed to get string value, hr %#lx.\n", hr);
    ok(!lstrcmpW(str, L"Text"), "Unexpected string.\n");
    CoTaskMemFree(str);
    hr = IMFAttributes_GetAllocatedBlob(dest, &DUMMY_GUID1, &buffer, &size);
    ok(hr == S_OK, "Failed to get blob value, hr %#lx.\n", hr);
    ok(!memcmp(buffer, blob, sizeof(blob)), "Unexpected blob.\n");
    CoTaskMemFree(buffer);

    IMFAttributes_Release(attributes);
    IMFAttributes_Release(dest);
}

static void test_wrapped_media_type(void)
{
    IMFMediaType *mediatype, *mediatype2;
    MF_ATTRIBUTE_TYPE type;
    UINT32 count;
    HRESULT hr;
    GUID guid;

    hr = MFCreateMediaType(&mediatype);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);

    hr = MFUnwrapMediaType(mediatype, &mediatype2);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaType_SetUINT32(mediatype, &GUID_NULL, 1);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(mediatype, &DUMMY_GUID1, 2);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    hr = IMFMediaType_SetGUID(mediatype, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Failed to set GUID value, hr %#lx.\n", hr);

    hr = MFWrapMediaType(mediatype, &MFMediaType_Audio, &IID_IUnknown, &mediatype2);
    ok(hr == S_OK, "Failed to create wrapped media type, hr %#lx.\n", hr);

    hr = IMFMediaType_GetGUID(mediatype2, &MF_MT_MAJOR_TYPE, &guid);
    ok(hr == S_OK, "Failed to get major type, hr %#lx.\n", hr);
    ok(IsEqualGUID(&guid, &MFMediaType_Audio), "Unexpected major type.\n");

    hr = IMFMediaType_GetGUID(mediatype2, &MF_MT_SUBTYPE, &guid);
    ok(hr == S_OK, "Failed to get subtype, hr %#lx.\n", hr);
    ok(IsEqualGUID(&guid, &IID_IUnknown), "Unexpected major type.\n");

    hr = IMFMediaType_GetCount(mediatype2, &count);
    ok(hr == S_OK, "Failed to get item count, hr %#lx.\n", hr);
    ok(count == 3, "Unexpected count %u.\n", count);

    hr = IMFMediaType_GetItemType(mediatype2, &MF_MT_WRAPPED_TYPE, &type);
    ok(hr == S_OK, "Failed to get item type, hr %#lx.\n", hr);
    ok(type == MF_ATTRIBUTE_BLOB, "Unexpected item type.\n");

    IMFMediaType_Release(mediatype);

    hr = MFUnwrapMediaType(mediatype2, &mediatype);
    ok(hr == S_OK, "Failed to unwrap, hr %#lx.\n", hr);

    hr = IMFMediaType_GetGUID(mediatype, &MF_MT_MAJOR_TYPE, &guid);
    ok(hr == S_OK, "Failed to get major type, hr %#lx.\n", hr);
    ok(IsEqualGUID(&guid, &MFMediaType_Video), "Unexpected major type.\n");

    hr = IMFMediaType_GetGUID(mediatype, &MF_MT_SUBTYPE, &guid);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    IMFMediaType_Release(mediatype);
    IMFMediaType_Release(mediatype2);
}

static void test_MFCreateWaveFormatExFromMFMediaType(void)
{
    static const struct wave_fmt_test
    {
        const GUID *subtype;
        WORD format_tag;
    }
    wave_fmt_tests[] =
    {
        { &MFAudioFormat_PCM,   WAVE_FORMAT_PCM, },
        { &MFAudioFormat_Float, WAVE_FORMAT_IEEE_FLOAT, },
    };
    WAVEFORMATEXTENSIBLE *format_ext;
    IMFMediaType *mediatype;
    WAVEFORMATEX *format;
    UINT32 size, i;
    HRESULT hr;

    hr = MFCreateMediaType(&mediatype);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);

    hr = MFCreateWaveFormatExFromMFMediaType(mediatype, &format, &size, MFWaveFormatExConvertFlag_Normal);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaType_SetGUID(mediatype, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    hr = MFCreateWaveFormatExFromMFMediaType(mediatype, &format, &size, MFWaveFormatExConvertFlag_Normal);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaType_SetGUID(mediatype, &MF_MT_SUBTYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    hr = IMFMediaType_SetGUID(mediatype, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    for (i = 0; i < ARRAY_SIZE(wave_fmt_tests); ++i)
    {
        hr = IMFMediaType_SetGUID(mediatype, &MF_MT_SUBTYPE, wave_fmt_tests[i].subtype);
        ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

        hr = MFCreateWaveFormatExFromMFMediaType(mediatype, &format, &size, MFWaveFormatExConvertFlag_Normal);
        ok(hr == S_OK, "Failed to create format, hr %#lx.\n", hr);
        ok(format != NULL, "Expected format structure.\n");
        ok(size == sizeof(*format), "Unexpected size %u.\n", size);
        ok(format->wFormatTag == wave_fmt_tests[i].format_tag, "Expected tag %u, got %u.\n", wave_fmt_tests[i].format_tag, format->wFormatTag);
        ok(format->nChannels == 0, "Unexpected number of channels, %u.\n", format->nChannels);
        ok(format->nSamplesPerSec == 0, "Unexpected sample rate, %lu.\n", format->nSamplesPerSec);
        ok(format->nAvgBytesPerSec == 0, "Unexpected average data rate rate, %lu.\n", format->nAvgBytesPerSec);
        ok(format->nBlockAlign == 0, "Unexpected alignment, %u.\n", format->nBlockAlign);
        ok(format->wBitsPerSample == 0, "Unexpected sample size, %u.\n", format->wBitsPerSample);
        ok(format->cbSize == 0, "Unexpected size field, %u.\n", format->cbSize);
        CoTaskMemFree(format);

        hr = MFCreateWaveFormatExFromMFMediaType(mediatype, (WAVEFORMATEX **)&format_ext, &size,
                MFWaveFormatExConvertFlag_ForceExtensible);
        ok(hr == S_OK, "Failed to create format, hr %#lx.\n", hr);
        ok(format_ext != NULL, "Expected format structure.\n");
        ok(size == sizeof(*format_ext), "Unexpected size %u.\n", size);
        ok(format_ext->Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE, "Unexpected tag.\n");
        ok(format_ext->Format.nChannels == 0, "Unexpected number of channels, %u.\n", format_ext->Format.nChannels);
        ok(format_ext->Format.nSamplesPerSec == 0, "Unexpected sample rate, %lu.\n", format_ext->Format.nSamplesPerSec);
        ok(format_ext->Format.nAvgBytesPerSec == 0, "Unexpected average data rate rate, %lu.\n",
                format_ext->Format.nAvgBytesPerSec);
        ok(format_ext->Format.nBlockAlign == 0, "Unexpected alignment, %u.\n", format_ext->Format.nBlockAlign);
        ok(format_ext->Format.wBitsPerSample == 0, "Unexpected sample size, %u.\n", format_ext->Format.wBitsPerSample);
        ok(format_ext->Format.cbSize == sizeof(*format_ext) - sizeof(format_ext->Format), "Unexpected size field, %u.\n",
                format_ext->Format.cbSize);
        CoTaskMemFree(format_ext);

        hr = MFCreateWaveFormatExFromMFMediaType(mediatype, &format, &size, MFWaveFormatExConvertFlag_ForceExtensible + 1);
        ok(hr == S_OK, "Failed to create format, hr %#lx.\n", hr);
        ok(size == sizeof(*format), "Unexpected size %u.\n", size);
        CoTaskMemFree(format);
    }

    IMFMediaType_Release(mediatype);
}

static HRESULT WINAPI test_create_file_callback_Invoke(IMFAsyncCallback *iface, IMFAsyncResult *result)
{
    struct test_callback *callback = impl_from_IMFAsyncCallback(iface);
    IMFByteStream *stream;
    IUnknown *object;
    HRESULT hr;

    ok(!!result, "Unexpected result object.\n");

    ok((IUnknown *)iface == IMFAsyncResult_GetStateNoAddRef(result), "Unexpected result state.\n");

    hr = IMFAsyncResult_GetObject(result, &object);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = MFEndCreateFile(result, &stream);
    ok(hr == S_OK, "Failed to get file stream, hr %#lx.\n", hr);
    IMFByteStream_Release(stream);

    SetEvent(callback->event);

    return S_OK;
}

static const IMFAsyncCallbackVtbl test_create_file_callback_vtbl =
{
    testcallback_QueryInterface,
    testcallback_AddRef,
    testcallback_Release,
    testcallback_GetParameters,
    test_create_file_callback_Invoke,
};

static void test_async_create_file(void)
{
    WCHAR pathW[MAX_PATH], fileW[MAX_PATH];
    struct test_callback *callback;
    IUnknown *cancel_cookie;
    HRESULT hr;
    BOOL ret;

    callback = create_test_callback(&test_create_file_callback_vtbl);

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Fail to start up, hr %#lx.\n", hr);

    GetTempPathW(ARRAY_SIZE(pathW), pathW);
    GetTempFileNameW(pathW, NULL, 0, fileW);

    hr = MFBeginCreateFile(MF_ACCESSMODE_READWRITE, MF_OPENMODE_DELETE_IF_EXIST, MF_FILEFLAGS_NONE, fileW,
            &callback->IMFAsyncCallback_iface, (IUnknown *)&callback->IMFAsyncCallback_iface, &cancel_cookie);
    ok(hr == S_OK, "Async create request failed, hr %#lx.\n", hr);
    ok(cancel_cookie != NULL, "Unexpected cancellation object.\n");

    WaitForSingleObject(callback->event, INFINITE);

    IUnknown_Release(cancel_cookie);

    hr = MFShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);

    IMFAsyncCallback_Release(&callback->IMFAsyncCallback_iface);

    ret = DeleteFileW(fileW);
    ok(ret, "Failed to delete test file.\n");
}

struct activate_object
{
    IMFActivate IMFActivate_iface;
    LONG refcount;
};

static HRESULT WINAPI activate_object_QueryInterface(IMFActivate *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IMFActivate) ||
            IsEqualIID(riid, &IID_IMFAttributes) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFActivate_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI activate_object_AddRef(IMFActivate *iface)
{
    return 2;
}

static ULONG WINAPI activate_object_Release(IMFActivate *iface)
{
    return 1;
}

static HRESULT WINAPI activate_object_GetItem(IMFActivate *iface, REFGUID key, PROPVARIANT *value)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_GetItemType(IMFActivate *iface, REFGUID key, MF_ATTRIBUTE_TYPE *type)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_CompareItem(IMFActivate *iface, REFGUID key, REFPROPVARIANT value, BOOL *result)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_Compare(IMFActivate *iface, IMFAttributes *theirs, MF_ATTRIBUTES_MATCH_TYPE type,
        BOOL *result)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_GetUINT32(IMFActivate *iface, REFGUID key, UINT32 *value)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_GetUINT64(IMFActivate *iface, REFGUID key, UINT64 *value)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_GetDouble(IMFActivate *iface, REFGUID key, double *value)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_GetGUID(IMFActivate *iface, REFGUID key, GUID *value)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_GetStringLength(IMFActivate *iface, REFGUID key, UINT32 *length)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_GetString(IMFActivate *iface, REFGUID key, WCHAR *value,
        UINT32 size, UINT32 *length)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_GetAllocatedString(IMFActivate *iface, REFGUID key,
        WCHAR **value, UINT32 *length)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_GetBlobSize(IMFActivate *iface, REFGUID key, UINT32 *size)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_GetBlob(IMFActivate *iface, REFGUID key, UINT8 *buf,
        UINT32 bufsize, UINT32 *blobsize)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_GetAllocatedBlob(IMFActivate *iface, REFGUID key, UINT8 **buf, UINT32 *size)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_GetUnknown(IMFActivate *iface, REFGUID key, REFIID riid, void **ppv)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_SetItem(IMFActivate *iface, REFGUID key, REFPROPVARIANT value)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_DeleteItem(IMFActivate *iface, REFGUID key)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_DeleteAllItems(IMFActivate *iface)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_SetUINT32(IMFActivate *iface, REFGUID key, UINT32 value)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_SetUINT64(IMFActivate *iface, REFGUID key, UINT64 value)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_SetDouble(IMFActivate *iface, REFGUID key, double value)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_SetGUID(IMFActivate *iface, REFGUID key, REFGUID value)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_SetString(IMFActivate *iface, REFGUID key, const WCHAR *value)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_SetBlob(IMFActivate *iface, REFGUID key, const UINT8 *buf, UINT32 size)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_SetUnknown(IMFActivate *iface, REFGUID key, IUnknown *unknown)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_LockStore(IMFActivate *iface)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_UnlockStore(IMFActivate *iface)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_GetCount(IMFActivate *iface, UINT32 *count)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_GetItemByIndex(IMFActivate *iface, UINT32 index, GUID *key, PROPVARIANT *value)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_CopyAllItems(IMFActivate *iface, IMFAttributes *dest)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_ActivateObject(IMFActivate *iface, REFIID riid, void **obj)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_ShutdownObject(IMFActivate *iface)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI activate_object_DetachObject(IMFActivate *iface)
{
    return E_NOTIMPL;
}

static const IMFActivateVtbl activate_object_vtbl =
{
    activate_object_QueryInterface,
    activate_object_AddRef,
    activate_object_Release,
    activate_object_GetItem,
    activate_object_GetItemType,
    activate_object_CompareItem,
    activate_object_Compare,
    activate_object_GetUINT32,
    activate_object_GetUINT64,
    activate_object_GetDouble,
    activate_object_GetGUID,
    activate_object_GetStringLength,
    activate_object_GetString,
    activate_object_GetAllocatedString,
    activate_object_GetBlobSize,
    activate_object_GetBlob,
    activate_object_GetAllocatedBlob,
    activate_object_GetUnknown,
    activate_object_SetItem,
    activate_object_DeleteItem,
    activate_object_DeleteAllItems,
    activate_object_SetUINT32,
    activate_object_SetUINT64,
    activate_object_SetDouble,
    activate_object_SetGUID,
    activate_object_SetString,
    activate_object_SetBlob,
    activate_object_SetUnknown,
    activate_object_LockStore,
    activate_object_UnlockStore,
    activate_object_GetCount,
    activate_object_GetItemByIndex,
    activate_object_CopyAllItems,
    activate_object_ActivateObject,
    activate_object_ShutdownObject,
    activate_object_DetachObject,
};

static void test_local_handlers(void)
{
    IMFActivate local_activate = { &activate_object_vtbl };
    static const WCHAR localW[] = L"local";
    HRESULT hr;

    if (!pMFRegisterLocalSchemeHandler)
    {
        win_skip("Local handlers are not supported.\n");
        return;
    }

    hr = pMFRegisterLocalSchemeHandler(NULL, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = pMFRegisterLocalSchemeHandler(localW, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = pMFRegisterLocalSchemeHandler(NULL, &local_activate);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = pMFRegisterLocalSchemeHandler(localW, &local_activate);
    ok(hr == S_OK, "Failed to register scheme handler, hr %#lx.\n", hr);

    hr = pMFRegisterLocalSchemeHandler(localW, &local_activate);
    ok(hr == S_OK, "Failed to register scheme handler, hr %#lx.\n", hr);

    hr = pMFRegisterLocalByteStreamHandler(NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = pMFRegisterLocalByteStreamHandler(NULL, NULL, &local_activate);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = pMFRegisterLocalByteStreamHandler(NULL, localW, &local_activate);
    ok(hr == S_OK, "Failed to register stream handler, hr %#lx.\n", hr);

    hr = pMFRegisterLocalByteStreamHandler(localW, NULL, &local_activate);
    ok(hr == S_OK, "Failed to register stream handler, hr %#lx.\n", hr);

    hr = pMFRegisterLocalByteStreamHandler(localW, localW, &local_activate);
    ok(hr == S_OK, "Failed to register stream handler, hr %#lx.\n", hr);
}

static void test_create_property_store(void)
{
    static const PROPERTYKEY test_pkey = {{0x12345678}, 9};
    IPropertyStore *store, *store2;
    PROPVARIANT value = {0};
    PROPERTYKEY key;
    ULONG refcount;
    DWORD count;
    HRESULT hr;

    hr = CreatePropertyStore(NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = CreatePropertyStore(&store);
    ok(hr == S_OK, "Failed to create property store, hr %#lx.\n", hr);

    hr = CreatePropertyStore(&store2);
    ok(hr == S_OK, "Failed to create property store, hr %#lx.\n", hr);
    ok(store2 != store, "Expected different store objects.\n");
    IPropertyStore_Release(store2);

    check_interface(store, &IID_IPropertyStoreCache, FALSE);
    check_interface(store, &IID_IPersistSerializedPropStorage, FALSE);

    hr = IPropertyStore_GetCount(store, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    count = 0xdeadbeef;
    hr = IPropertyStore_GetCount(store, &count);
    ok(hr == S_OK, "Failed to get count, hr %#lx.\n", hr);
    ok(!count, "Unexpected count %lu.\n", count);

    hr = IPropertyStore_Commit(store);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);

    hr = IPropertyStore_GetAt(store, 0, &key);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IPropertyStore_GetValue(store, NULL, &value);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);

    hr = IPropertyStore_GetValue(store, &test_pkey, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IPropertyStore_GetValue(store, &test_pkey, &value);
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);

    memset(&value, 0, sizeof(PROPVARIANT));
    value.vt = VT_I4;
    value.lVal = 0xdeadbeef;
    hr = IPropertyStore_SetValue(store, &test_pkey, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

if (0)
{
    /* crashes on Windows */
    hr = IPropertyStore_SetValue(store, NULL, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
}

    hr = IPropertyStore_GetCount(store, &count);
    ok(hr == S_OK, "Failed to get count, hr %#lx.\n", hr);
    ok(count == 1, "Unexpected count %lu.\n", count);

    hr = IPropertyStore_Commit(store);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);

    hr = IPropertyStore_GetAt(store, 0, &key);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!memcmp(&key, &test_pkey, sizeof(PROPERTYKEY)), "Keys didn't match.\n");

    hr = IPropertyStore_GetAt(store, 1, &key);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    memset(&value, 0xcc, sizeof(PROPVARIANT));
    hr = IPropertyStore_GetValue(store, &test_pkey, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value.vt == VT_I4, "Unexpected type %u.\n", value.vt);
    ok(value.lVal == 0xdeadbeef, "Unexpected value %#lx.\n", value.lVal);

    memset(&value, 0, sizeof(PROPVARIANT));
    hr = IPropertyStore_SetValue(store, &test_pkey, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IPropertyStore_GetCount(store, &count);
    ok(hr == S_OK, "Failed to get count, hr %#lx.\n", hr);
    ok(count == 1, "Unexpected count %lu.\n", count);

    memset(&value, 0xcc, sizeof(PROPVARIANT));
    hr = IPropertyStore_GetValue(store, &test_pkey, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value.vt == VT_EMPTY, "Unexpected type %u.\n", value.vt);
    ok(!value.lVal, "Unexpected value %#lx.\n", value.lVal);

    refcount = IPropertyStore_Release(store);
    ok(!refcount, "Unexpected refcount %lu.\n", refcount);
}

struct test_thread_param
{
    IMFDXGIDeviceManager *manager;
    HANDLE handle;
    BOOL lock;
};

static DWORD WINAPI test_device_manager_thread(void *arg)
{
    struct test_thread_param *param = arg;
    ID3D11Device *device;
    HRESULT hr;

    if (param->lock)
    {
        hr = IMFDXGIDeviceManager_LockDevice(param->manager, param->handle, &IID_ID3D11Device,
                (void **)&device, FALSE);
        if (SUCCEEDED(hr))
            ID3D11Device_Release(device);
    }
    else
        hr = IMFDXGIDeviceManager_UnlockDevice(param->manager, param->handle, FALSE);

    return hr;
}

static void test_dxgi_device_manager(void)
{
    IMFDXGIDeviceManager *manager, *manager2;
    ID3D11Device *device, *d3d11_dev, *d3d11_dev2;
    struct test_thread_param param;
    HANDLE handle1, handle, thread;
    UINT token, token2;
    IUnknown *unk;
    HRESULT hr;

    if (!pMFCreateDXGIDeviceManager)
    {
        win_skip("MFCreateDXGIDeviceManager not found.\n");
        return;
    }

    hr = pD3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, D3D11_CREATE_DEVICE_VIDEO_SUPPORT,
                           NULL, 0, D3D11_SDK_VERSION, &d3d11_dev, NULL, NULL);
    if (FAILED(hr))
    {
        skip("Failed to create D3D11 device object.\n");
        return;
    }
    ok(hr == S_OK, "D3D11CreateDevice failed: %#lx.\n", hr);
    EXPECT_REF(d3d11_dev, 1);

    hr = pMFCreateDXGIDeviceManager(NULL, &manager);
    ok(hr == E_POINTER, "MFCreateDXGIDeviceManager should failed: %#lx.\n", hr);

    token = 0;
    hr = pMFCreateDXGIDeviceManager(&token, NULL);
    ok(hr == E_POINTER, "MFCreateDXGIDeviceManager should failed: %#lx.\n", hr);
    ok(!token, "got wrong token: %u.\n", token);

    hr = pMFCreateDXGIDeviceManager(&token, &manager);
    ok(hr == S_OK, "MFCreateDXGIDeviceManager failed: %#lx.\n", hr);
    EXPECT_REF(manager, 1);
    ok(!!token, "got wrong token: %u.\n", token);

    Sleep(50);
    token2 = 0;
    hr = pMFCreateDXGIDeviceManager(&token2, &manager2);
    ok(hr == S_OK, "MFCreateDXGIDeviceManager failed: %#lx.\n", hr);
    EXPECT_REF(manager2, 1);
    ok(token2 && token2 != token, "got wrong token: %u, %u.\n", token2, token);
    ok(manager != manager2, "got wrong pointer: %p.\n", manager2);
    EXPECT_REF(manager, 1);

    hr = IMFDXGIDeviceManager_GetVideoService(manager, NULL, &IID_ID3D11Device, (void **)&unk);
    ok(hr == MF_E_DXGI_DEVICE_NOT_INITIALIZED, "Unexpected hr %#lx.\n", hr);

    hr = IMFDXGIDeviceManager_OpenDeviceHandle(manager, &handle);
    ok(hr == MF_E_DXGI_DEVICE_NOT_INITIALIZED, "Unexpected hr %#lx.\n", hr);

    hr = IMFDXGIDeviceManager_CloseDeviceHandle(manager, 0);
    ok(hr == E_HANDLE, "Unexpected hr %#lx.\n", hr);

    hr = IMFDXGIDeviceManager_ResetDevice(manager, (IUnknown *)d3d11_dev, token - 1);
    ok(hr == E_INVALIDARG, "IMFDXGIDeviceManager_ResetDevice should failed: %#lx.\n", hr);
    EXPECT_REF(d3d11_dev, 1);

    hr = IMFDXGIDeviceManager_ResetDevice(manager, NULL, token);
    ok(hr == E_INVALIDARG, "IMFDXGIDeviceManager_ResetDevice should failed: %#lx.\n", hr);

    hr = IMFDXGIDeviceManager_ResetDevice(manager, (IUnknown *)d3d11_dev, token);
    ok(hr == S_OK, "IMFDXGIDeviceManager_ResetDevice failed: %#lx.\n", hr);
    EXPECT_REF(manager, 1);
    EXPECT_REF(d3d11_dev, 2);

    hr = IMFDXGIDeviceManager_ResetDevice(manager, (IUnknown *)manager2, token);
    ok(hr == E_INVALIDARG, "IMFDXGIDeviceManager_ResetDevice should failed: %#lx.\n", hr);
    EXPECT_REF(manager2, 1);
    EXPECT_REF(d3d11_dev, 2);

    hr = IMFDXGIDeviceManager_ResetDevice(manager, (IUnknown *)d3d11_dev, token);
    ok(hr == S_OK, "IMFDXGIDeviceManager_ResetDevice failed: %#lx.\n", hr);
    EXPECT_REF(manager, 1);
    EXPECT_REF(d3d11_dev, 2);

    /* GetVideoService() on device change. */
    handle = NULL;
    hr = IMFDXGIDeviceManager_OpenDeviceHandle(manager, &handle);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!handle, "Unexpected handle value %p.\n", handle);

    hr = pD3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0,
                           NULL, 0, D3D11_SDK_VERSION, &d3d11_dev2, NULL, NULL);
    ok(hr == S_OK, "D3D11CreateDevice failed: %#lx.\n", hr);
    EXPECT_REF(d3d11_dev2, 1);
    hr = IMFDXGIDeviceManager_ResetDevice(manager, (IUnknown *)d3d11_dev2, token);
    ok(hr == S_OK, "IMFDXGIDeviceManager_ResetDevice failed: %#lx.\n", hr);
    EXPECT_REF(manager, 1);
    EXPECT_REF(d3d11_dev2, 2);
    EXPECT_REF(d3d11_dev, 1);

    hr = IMFDXGIDeviceManager_GetVideoService(manager, handle, &IID_ID3D11Device, (void **)&unk);
    ok(hr == MF_E_DXGI_NEW_VIDEO_DEVICE, "Unexpected hr %#lx.\n", hr);

    hr = IMFDXGIDeviceManager_CloseDeviceHandle(manager, handle);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    handle = NULL;
    hr = IMFDXGIDeviceManager_OpenDeviceHandle(manager, &handle);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!handle, "Unexpected handle value %p.\n", handle);

    hr = IMFDXGIDeviceManager_GetVideoService(manager, NULL, &IID_ID3D11Device, (void **)&unk);
    ok(hr == E_HANDLE, "Unexpected hr %#lx.\n", hr);

    hr = IMFDXGIDeviceManager_GetVideoService(manager, handle, &IID_ID3D11Device, (void **)&unk);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IUnknown_Release(unk);

    hr = IMFDXGIDeviceManager_GetVideoService(manager, handle, &IID_IUnknown, (void **)&unk);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IUnknown_Release(unk);

    hr = IMFDXGIDeviceManager_GetVideoService(manager, handle, &IID_IDXGIDevice, (void **)&unk);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IUnknown_Release(unk);

    handle1 = NULL;
    hr = IMFDXGIDeviceManager_OpenDeviceHandle(manager, &handle1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(handle != handle1, "Unexpected handle.\n");

    hr = IMFDXGIDeviceManager_CloseDeviceHandle(manager, handle);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Already closed. */
    hr = IMFDXGIDeviceManager_CloseDeviceHandle(manager, handle);
    ok(hr == E_HANDLE, "Unexpected hr %#lx.\n", hr);

    handle = NULL;
    hr = IMFDXGIDeviceManager_OpenDeviceHandle(manager, &handle);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFDXGIDeviceManager_CloseDeviceHandle(manager, handle1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFDXGIDeviceManager_TestDevice(manager, handle1);
    ok(hr == E_HANDLE, "Unexpected hr %#lx.\n", hr);

    hr = IMFDXGIDeviceManager_LockDevice(manager, handle, &IID_ID3D11Device, (void **)&device, FALSE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(device == d3d11_dev2, "Unexpected device pointer.\n");
    ID3D11Device_Release(device);

    hr = IMFDXGIDeviceManager_UnlockDevice(manager, handle, FALSE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFDXGIDeviceManager_UnlockDevice(manager, handle, FALSE);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IMFDXGIDeviceManager_UnlockDevice(manager, UlongToHandle(100), FALSE);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);

    /* Locked with one handle, unlock with another. */
    hr = IMFDXGIDeviceManager_OpenDeviceHandle(manager, &handle1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFDXGIDeviceManager_LockDevice(manager, handle, &IID_ID3D11Device, (void **)&device, FALSE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFDXGIDeviceManager_UnlockDevice(manager, handle1, FALSE);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    ID3D11Device_Release(device);

    /* Closing unlocks the device. */
    hr = IMFDXGIDeviceManager_CloseDeviceHandle(manager, handle);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFDXGIDeviceManager_LockDevice(manager, handle1, &IID_ID3D11Device, (void **)&device, FALSE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ID3D11Device_Release(device);

    hr = IMFDXGIDeviceManager_CloseDeviceHandle(manager, handle1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Open two handles. */
    hr = IMFDXGIDeviceManager_OpenDeviceHandle(manager, &handle);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFDXGIDeviceManager_OpenDeviceHandle(manager, &handle1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFDXGIDeviceManager_LockDevice(manager, handle, &IID_ID3D11Device, (void **)&device, FALSE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ID3D11Device_Release(device);

    hr = IMFDXGIDeviceManager_LockDevice(manager, handle1, &IID_ID3D11Device, (void **)&device, FALSE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ID3D11Device_Release(device);

    hr = IMFDXGIDeviceManager_UnlockDevice(manager, handle, FALSE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFDXGIDeviceManager_UnlockDevice(manager, handle, FALSE);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    param.manager = manager;
    param.handle = handle;
    param.lock = TRUE;
    thread = CreateThread(NULL, 0, test_device_manager_thread, &param, 0, NULL);
    ok(!WaitForSingleObject(thread, 1000), "Wait for a test thread failed.\n");
    GetExitCodeThread(thread, (DWORD *)&hr);
    ok(hr == MF_E_DXGI_VIDEO_DEVICE_LOCKED, "Unexpected hr %#lx.\n", hr);
    CloseHandle(thread);

    hr = IMFDXGIDeviceManager_UnlockDevice(manager, handle1, FALSE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFDXGIDeviceManager_UnlockDevice(manager, handle1, FALSE);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    /* Lock on main thread, unlock on another. */
    hr = IMFDXGIDeviceManager_LockDevice(manager, handle, &IID_ID3D11Device, (void **)&device, FALSE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ID3D11Device_Release(device);

    param.manager = manager;
    param.handle = handle;
    param.lock = FALSE;
    thread = CreateThread(NULL, 0, test_device_manager_thread, &param, 0, NULL);
    ok(!WaitForSingleObject(thread, 1000), "Wait for a test thread failed.\n");
    GetExitCodeThread(thread, (DWORD *)&hr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    CloseHandle(thread);

    hr = IMFDXGIDeviceManager_CloseDeviceHandle(manager, handle1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFDXGIDeviceManager_CloseDeviceHandle(manager, handle);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IMFDXGIDeviceManager_Release(manager);
    EXPECT_REF(d3d11_dev2, 1);
    ID3D11Device_Release(d3d11_dev);
    ID3D11Device_Release(d3d11_dev2);
    IMFDXGIDeviceManager_Release(manager2);
}

static void test_MFCreateTransformActivate(void)
{
    IMFActivate *activate;
    UINT32 count;
    HRESULT hr;

    if (!pMFCreateTransformActivate)
    {
        win_skip("MFCreateTransformActivate() is not available.\n");
        return;
    }

    hr = pMFCreateTransformActivate(&activate);
    ok(hr == S_OK, "Failed to create activator, hr %#lx.\n", hr);

    hr = IMFActivate_GetCount(activate, &count);
    ok(hr == S_OK, "Failed to get count, hr %#lx.\n", hr);
    ok(!count, "Unexpected attribute count %u.\n", count);

    IMFActivate_Release(activate);
}

static HRESULT WINAPI test_mft_factory_QueryInterface(IClassFactory *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IClassFactory) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IClassFactory_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI test_mft_factory_AddRef(IClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI test_mft_factory_Release(IClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI test_mft_factory_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **obj)
{
    ok(0, "Unexpected call.\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI test_mft_factory_LockServer(IClassFactory *iface, BOOL fLock)
{
    return S_OK;
}

static const IClassFactoryVtbl test_mft_factory_vtbl =
{
    test_mft_factory_QueryInterface,
    test_mft_factory_AddRef,
    test_mft_factory_Release,
    test_mft_factory_CreateInstance,
    test_mft_factory_LockServer,
};

static void test_MFTRegisterLocal(void)
{
    IClassFactory test_factory = { &test_mft_factory_vtbl };
    MFT_REGISTER_TYPE_INFO input_types[1], *in_types, *out_types;
    IMFAttributes *attributes;
    IMFActivate **activate;
    UINT32 count, count2;
    WCHAR *name;
    HRESULT hr;

    if (!pMFTRegisterLocal)
    {
        win_skip("MFTRegisterLocal() is not available.\n");
        return;
    }

    input_types[0].guidMajorType = MFMediaType_Audio;
    input_types[0].guidSubtype = MFAudioFormat_PCM;
    hr = pMFTRegisterLocal(&test_factory, &MFT_CATEGORY_OTHER, L"Local MFT name", 0, 1, input_types, 0, NULL);
    ok(hr == S_OK, "Failed to register MFT, hr %#lx.\n", hr);

    hr = pMFTRegisterLocal(&test_factory, &MFT_CATEGORY_OTHER, L"Local MFT name", 0, 1, input_types, 0, NULL);
    ok(hr == S_OK, "Failed to register MFT, hr %#lx.\n", hr);

    hr = pMFTEnumEx(MFT_CATEGORY_OTHER, MFT_ENUM_FLAG_LOCALMFT, NULL, NULL, &activate, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count > 0, "Unexpected count %u.\n", count);
    CoTaskMemFree(activate);

    hr = pMFTUnregisterLocal(&test_factory);
    ok(hr == S_OK, "Failed to unregister MFT, hr %#lx.\n", hr);

    hr = pMFTEnumEx(MFT_CATEGORY_OTHER, MFT_ENUM_FLAG_LOCALMFT, NULL, NULL, &activate, &count2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count2 < count, "Unexpected count %u.\n", count2);
    CoTaskMemFree(activate);

    hr = pMFTUnregisterLocal(&test_factory);
    ok(hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND), "Unexpected hr %#lx.\n", hr);

    hr = pMFTUnregisterLocal(NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = pMFTRegisterLocalByCLSID(&MFT_CATEGORY_OTHER, &MFT_CATEGORY_OTHER, L"Local MFT name 2", 0, 1, input_types,
            0, NULL);
    ok(hr == S_OK, "Failed to register MFT, hr %#lx.\n", hr);

    hr = MFTGetInfo(MFT_CATEGORY_OTHER, &name, &in_types, &count, &out_types, &count2, &attributes);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), "Unexpected hr %#lx.\n", hr);

    hr = pMFTUnregisterLocal(NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = pMFTUnregisterLocalByCLSID(MFT_CATEGORY_OTHER);
    ok(hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND), "Unexpected hr %#lx.\n", hr);

    hr = pMFTRegisterLocalByCLSID(&MFT_CATEGORY_OTHER, &MFT_CATEGORY_OTHER, L"Local MFT name 2", 0, 1, input_types,
            0, NULL);
    ok(hr == S_OK, "Failed to register MFT, hr %#lx.\n", hr);

    hr = pMFTUnregisterLocalByCLSID(MFT_CATEGORY_OTHER);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
}

static void test_queue_com(void)
{
    static int system_queues[] =
    {
        MFASYNC_CALLBACK_QUEUE_STANDARD,
        MFASYNC_CALLBACK_QUEUE_RT,
        MFASYNC_CALLBACK_QUEUE_IO,
        MFASYNC_CALLBACK_QUEUE_TIMER,
        MFASYNC_CALLBACK_QUEUE_MULTITHREADED,
        MFASYNC_CALLBACK_QUEUE_LONG_FUNCTION,
    };

    static int user_queues[] =
    {
        MF_STANDARD_WORKQUEUE,
        MF_WINDOW_WORKQUEUE,
        MF_MULTITHREADED_WORKQUEUE,
    };

    char path_name[MAX_PATH];
    PROCESS_INFORMATION info;
    STARTUPINFOA startup;
    char **argv;
    int i;

    if (!pCoGetApartmentType)
    {
        win_skip("CoGetApartmentType() is not available.\n");
        return;
    }

    winetest_get_mainargs(&argv);

    for (i = 0; i < ARRAY_SIZE(system_queues); ++i)
    {
        memset(&startup, 0, sizeof(startup));
        startup.cb = sizeof(startup);
        sprintf(path_name, "%s mfplat s%d", argv[0], system_queues[i]);
        ok(CreateProcessA( NULL, path_name, NULL, NULL, FALSE, 0, NULL, NULL, &startup, &info),
                "CreateProcess failed.\n" );
        wait_child_process(info.hProcess);
        CloseHandle(info.hProcess);
        CloseHandle(info.hThread);
    }

    for (i = 0; i < ARRAY_SIZE(user_queues); ++i)
    {
        memset(&startup, 0, sizeof(startup));
        startup.cb = sizeof(startup);
        sprintf(path_name, "%s mfplat u%d", argv[0], user_queues[i]);
        ok(CreateProcessA( NULL, path_name, NULL, NULL, FALSE, 0, NULL, NULL, &startup, &info),
                "CreateProcess failed.\n" );
        wait_child_process(info.hProcess);
        CloseHandle(info.hProcess);
        CloseHandle(info.hThread);
    }
}

static HRESULT WINAPI test_queue_com_state_callback_Invoke(IMFAsyncCallback *iface, IMFAsyncResult *result)
{
    struct test_callback *callback = impl_from_IMFAsyncCallback(iface);
    APTTYPEQUALIFIER qualifier;
    APTTYPE com_type;
    HRESULT hr;

    hr = pCoGetApartmentType(&com_type, &qualifier);
    ok(SUCCEEDED(hr), "Failed to get apartment type, hr %#lx.\n", hr);
    if (SUCCEEDED(hr))
    {
    todo_wine {
        if (callback->param == MFASYNC_CALLBACK_QUEUE_LONG_FUNCTION)
            ok(com_type == APTTYPE_MAINSTA && qualifier == APTTYPEQUALIFIER_NONE,
                "%#lx: unexpected type %u, qualifier %u.\n", callback->param, com_type, qualifier);
        else
            ok(com_type == APTTYPE_MTA && qualifier == APTTYPEQUALIFIER_NONE,
                "%#lx: unexpected type %u, qualifier %u.\n", callback->param, com_type, qualifier);
    }
    }

    SetEvent(callback->event);
    return S_OK;
}

static const IMFAsyncCallbackVtbl test_queue_com_state_callback_vtbl =
{
    testcallback_QueryInterface,
    testcallback_AddRef,
    testcallback_Release,
    testcallback_GetParameters,
    test_queue_com_state_callback_Invoke,
};

static void test_queue_com_state(const char *name)
{
    struct test_callback *callback;
    DWORD queue, queue_type;
    HRESULT hr;

    callback = create_test_callback(&test_queue_com_state_callback_vtbl);

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Failed to start up, hr %#lx.\n", hr);

    if (name[0] == 's')
    {
        callback->param = name[1] - '0';
        hr = MFPutWorkItem(callback->param, &callback->IMFAsyncCallback_iface, NULL);
        ok(SUCCEEDED(hr), "Failed to queue work item, hr %#lx.\n", hr);
        WaitForSingleObject(callback->event, INFINITE);
    }
    else if (name[0] == 'u')
    {
        queue_type = name[1] - '0';

        hr = pMFAllocateWorkQueueEx(queue_type, &queue);
        ok(hr == S_OK || broken(queue_type == MF_MULTITHREADED_WORKQUEUE && hr == E_INVALIDARG) /* Win7 */,
                "Failed to allocate a queue of type %lu, hr %#lx.\n", queue_type, hr);

        if (SUCCEEDED(hr))
        {
            callback->param = queue;
            hr = MFPutWorkItem(queue, &callback->IMFAsyncCallback_iface, NULL);
            ok(SUCCEEDED(hr), "Failed to queue work item, hr %#lx.\n", hr);
            WaitForSingleObject(callback->event, INFINITE);

            hr = MFUnlockWorkQueue(queue);
            ok(hr == S_OK, "Failed to unlock the queue, hr %#lx.\n", hr);
        }
    }

    hr = MFShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);

    IMFAsyncCallback_Release(&callback->IMFAsyncCallback_iface);
}

static void test_MFGetStrideForBitmapInfoHeader(void)
{
    static const struct stride_test
    {
        const GUID *subtype;
        unsigned int width;
        LONG stride;
    }
    stride_tests[] =
    {
        { &MFVideoFormat_RGB8, 3, -4 },
        { &MFVideoFormat_RGB8, 1, -4 },
        { &MFVideoFormat_RGB555, 3, -8 },
        { &MFVideoFormat_RGB555, 1, -4 },
        { &MFVideoFormat_RGB565, 3, -8 },
        { &MFVideoFormat_RGB565, 1, -4 },
        { &MFVideoFormat_RGB24, 3, -12 },
        { &MFVideoFormat_RGB24, 1, -4 },
        { &MFVideoFormat_RGB32, 3, -12 },
        { &MFVideoFormat_RGB32, 1, -4 },
        { &MFVideoFormat_ARGB32, 3, -12 },
        { &MFVideoFormat_ARGB32, 1, -4 },
        { &MFVideoFormat_ABGR32, 3, -12 },
        { &MFVideoFormat_ABGR32, 1, -4 },
        { &MFVideoFormat_A2R10G10B10, 3, -12 },
        { &MFVideoFormat_A2R10G10B10, 1, -4 },
        { &MFVideoFormat_A2B10G10R10, 3, -12 },
        { &MFVideoFormat_A2B10G10R10, 1, -4 },
        { &MFVideoFormat_A16B16G16R16F, 3, -24 },
        { &MFVideoFormat_A16B16G16R16F, 1, -8 },

        /* YUV */
        { &MFVideoFormat_NV12, 1, 1 },
        { &MFVideoFormat_NV12, 2, 2 },
        { &MFVideoFormat_NV12, 3, 3 },
        { &MFVideoFormat_AYUV, 1, 4 },
        { &MFVideoFormat_AYUV, 4, 16 },
        { &MFVideoFormat_AYUV, 5, 20 },
        { &MFVideoFormat_IMC1, 1, 4 },
        { &MFVideoFormat_IMC1, 2, 4 },
        { &MFVideoFormat_IMC1, 3, 8 },
        { &MFVideoFormat_IMC3, 1, 4 },
        { &MFVideoFormat_IMC3, 2, 4 },
        { &MFVideoFormat_IMC3, 3, 8 },
        { &MFVideoFormat_IMC2, 1, 1 },
        { &MFVideoFormat_IMC2, 2, 2 },
        { &MFVideoFormat_IMC2, 3, 3 },
        { &MFVideoFormat_IMC4, 1, 1 },
        { &MFVideoFormat_IMC4, 2, 2 },
        { &MFVideoFormat_IMC4, 3, 3 },
        { &MFVideoFormat_YV12, 1, 1 },
        { &MFVideoFormat_YV12, 2, 2 },
        { &MFVideoFormat_YV12, 3, 3 },
        { &MFVideoFormat_YV12, 320, 320 },
        { &MFVideoFormat_I420, 1, 1 },
        { &MFVideoFormat_I420, 2, 2 },
        { &MFVideoFormat_I420, 3, 3 },
        { &MFVideoFormat_I420, 320, 320 },
        { &MFVideoFormat_IYUV, 1, 1 },
        { &MFVideoFormat_IYUV, 2, 2 },
        { &MFVideoFormat_IYUV, 3, 3 },
        { &MFVideoFormat_IYUV, 320, 320 },
        { &MFVideoFormat_NV11, 1, 1 },
        { &MFVideoFormat_NV11, 2, 2 },
        { &MFVideoFormat_NV11, 3, 3 },
    };
    unsigned int i;
    LONG stride;
    HRESULT hr;

    if (!pMFGetStrideForBitmapInfoHeader)
    {
        win_skip("MFGetStrideForBitmapInfoHeader() is not available.\n");
        return;
    }

    hr = pMFGetStrideForBitmapInfoHeader(MAKEFOURCC('H','2','6','4'), 1, &stride);
    ok(hr == MF_E_INVALIDMEDIATYPE, "Unexpected hr %#lx.\n", hr);

    for (i = 0; i < ARRAY_SIZE(stride_tests); ++i)
    {
        hr = pMFGetStrideForBitmapInfoHeader(stride_tests[i].subtype->Data1, stride_tests[i].width, &stride);
        ok(hr == S_OK, "%u: failed to get stride, hr %#lx.\n", i, hr);
        ok(stride == stride_tests[i].stride, "%u: format %s, unexpected stride %ld, expected %ld.\n", i,
                wine_dbgstr_an((char *)&stride_tests[i].subtype->Data1, 4), stride, stride_tests[i].stride);
    }
}

static void test_MFCreate2DMediaBuffer(void)
{
    static const char two_aas[] = { 0xaa, 0xaa };
    static const char eight_bbs[] = { 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb };
    DWORD max_length, length, length2;
    BYTE *buffer_start, *data, *data2;
    LONG pitch, pitch2, stride;
    IMF2DBuffer2 *_2dbuffer2;
    IMF2DBuffer *_2dbuffer;
    IMFMediaBuffer *buffer;
    int i, j, k;
    HRESULT hr;
    BOOL ret;

    if (!pMFCreate2DMediaBuffer)
    {
        win_skip("MFCreate2DMediaBuffer() is not available.\n");
        return;
    }

    hr = pMFCreate2DMediaBuffer(2, 3, MAKEFOURCC('H','2','6','4'), FALSE, &buffer);
    ok(hr == MF_E_INVALIDMEDIATYPE, "Unexpected hr %#lx.\n", hr);

    hr = pMFCreate2DMediaBuffer(2, 3, MAKEFOURCC('N','V','1','2'), FALSE, NULL);
    ok(FAILED(hr), "Unexpected hr %#lx.\n", hr);

    /* YUV formats can't be bottom-up. */
    hr = pMFCreate2DMediaBuffer(2, 3, MAKEFOURCC('N','V','1','2'), TRUE, &buffer);
    ok(hr == MF_E_INVALIDMEDIATYPE, "Unexpected hr %#lx.\n", hr);

    hr = pMFCreate2DMediaBuffer(2, 3, MAKEFOURCC('N','V','1','2'), FALSE, &buffer);
    ok(hr == S_OK, "Failed to create a buffer, hr %#lx.\n", hr);

    check_interface(buffer, &IID_IMFGetService, TRUE);
    check_interface(buffer, &IID_IMF2DBuffer, TRUE);

    /* Full backing buffer size, with 64 bytes per row alignment.  */
    hr = IMFMediaBuffer_GetMaxLength(buffer, &max_length);
    ok(hr == S_OK, "Failed to get length, hr %#lx.\n", hr);
    ok(max_length > 0, "Unexpected length %lu.\n", max_length);

    hr = IMFMediaBuffer_GetCurrentLength(buffer, &length);
    ok(hr == S_OK, "Failed to get current length, hr %#lx.\n", hr);
    ok(!length, "Unexpected length.\n");

    hr = IMFMediaBuffer_SetCurrentLength(buffer, 10);
    ok(hr == S_OK, "Failed to set current length, hr %#lx.\n", hr);

    hr = IMFMediaBuffer_GetCurrentLength(buffer, &length);
    ok(hr == S_OK, "Failed to get current length, hr %#lx.\n", hr);
    ok(length == 10, "Unexpected length.\n");

    /* Linear lock/unlock. */

    hr = IMFMediaBuffer_Lock(buffer, NULL, &max_length, &length);
    ok(FAILED(hr), "Unexpected hr %#lx.\n", hr);

    /* Linear locking call returns plane size.*/
    hr = IMFMediaBuffer_Lock(buffer, &data, &max_length, &length);
    ok(hr == S_OK, "Failed to lock buffer, hr %#lx.\n", hr);
    ok(max_length == length, "Unexpected length.\n");

    memset(data, 0xaa, length);

    length = 0;
    pMFGetPlaneSize(MAKEFOURCC('N','V','1','2'), 2, 3, &length);
    ok(max_length == length && length == 9, "Unexpected length %lu.\n", length);

    /* Already locked */
    hr = IMFMediaBuffer_Lock(buffer, &data2, NULL, NULL);
    ok(hr == S_OK, "Failed to lock buffer, hr %#lx.\n", hr);
    ok(data2 == data, "Unexpected pointer.\n");

    hr = IMFMediaBuffer_Unlock(buffer);
    ok(hr == S_OK, "Failed to unlock buffer, hr %#lx.\n", hr);

    hr = IMFMediaBuffer_QueryInterface(buffer, &IID_IMF2DBuffer, (void **)&_2dbuffer);
    ok(hr == S_OK, "Failed to get interface, hr %#lx.\n", hr);

    hr = IMF2DBuffer_GetContiguousLength(_2dbuffer, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMF2DBuffer_GetContiguousLength(_2dbuffer, &length);
    ok(hr == S_OK, "Failed to get length, hr %#lx.\n", hr);
    ok(length == 9, "Unexpected length %lu.\n", length);

    hr = IMF2DBuffer_IsContiguousFormat(_2dbuffer, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    /* 2D lock. */
    hr = IMF2DBuffer_Lock2D(_2dbuffer, &data, &pitch);
    ok(hr == MF_E_UNEXPECTED, "Unexpected hr %#lx.\n", hr);

    hr = IMF2DBuffer_GetScanline0AndPitch(_2dbuffer, &data, &pitch);
    ok(hr == HRESULT_FROM_WIN32(ERROR_WAS_UNLOCKED), "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaBuffer_Unlock(buffer);
    ok(hr == S_OK, "Failed to unlock buffer, hr %#lx.\n", hr);

    hr = IMF2DBuffer_GetScanline0AndPitch(_2dbuffer, &data, &pitch);
    ok(hr == HRESULT_FROM_WIN32(ERROR_WAS_UNLOCKED), "Unexpected hr %#lx.\n", hr);

    hr = IMF2DBuffer_Lock2D(_2dbuffer, NULL, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMF2DBuffer_Lock2D(_2dbuffer, &data, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMF2DBuffer_Lock2D(_2dbuffer, NULL, &pitch);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMF2DBuffer_Lock2D(_2dbuffer, &data, &pitch);
    ok(hr == S_OK, "Failed to lock buffer, hr %#lx.\n", hr);
    ok(!!data, "Expected data pointer.\n");
    ok(pitch == 64, "Unexpected pitch %ld.\n", pitch);

    for (i = 0; i < 4; i++)
        ok(memcmp(&data[64 * i], two_aas, sizeof(two_aas)) == 0, "Invalid data instead of 0xaa.\n");
    memset(data, 0xbb, 194);

    hr = IMF2DBuffer_Lock2D(_2dbuffer, &data2, &pitch);
    ok(hr == S_OK, "Failed to lock buffer, hr %#lx.\n", hr);
    ok(data == data2, "Expected data pointer.\n");

    hr = IMF2DBuffer_GetScanline0AndPitch(_2dbuffer, NULL, &pitch);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMF2DBuffer_GetScanline0AndPitch(_2dbuffer, &data, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    /* Active 2D lock */
    hr = IMFMediaBuffer_Lock(buffer, &data2, NULL, NULL);
    ok(hr == MF_E_INVALIDREQUEST, "Unexpected hr %#lx.\n", hr);

    hr = IMF2DBuffer_Unlock2D(_2dbuffer);
    ok(hr == S_OK, "Failed to unlock buffer, hr %#lx.\n", hr);

    hr = IMFMediaBuffer_Lock(buffer, &data2, NULL, NULL);
    ok(hr == MF_E_INVALIDREQUEST, "Unexpected hr %#lx.\n", hr);

    hr = IMF2DBuffer_Unlock2D(_2dbuffer);
    ok(hr == S_OK, "Failed to unlock buffer, hr %#lx.\n", hr);

    hr = IMF2DBuffer_Unlock2D(_2dbuffer);
    ok(hr == HRESULT_FROM_WIN32(ERROR_WAS_UNLOCKED), "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaBuffer_Lock(buffer, &data, NULL, NULL);
    ok(hr == S_OK, "Failed to lock buffer, hr %#lx.\n", hr);

    ok(memcmp(data, eight_bbs, sizeof(eight_bbs)) == 0, "Invalid data instead of 0xbb.\n");

    hr = IMFMediaBuffer_Unlock(buffer);
    ok(hr == S_OK, "Failed to unlock buffer, hr %#lx.\n", hr);

    hr = IMFMediaBuffer_QueryInterface(buffer, &IID_IMF2DBuffer2, (void **)&_2dbuffer2);
    ok(hr == S_OK || broken(hr == E_NOINTERFACE), "Failed to get interface, hr %#lx.\n", hr);

    if (SUCCEEDED(hr))
    {
        hr = IMF2DBuffer_Lock2D(_2dbuffer, &data, &pitch);
        ok(hr == S_OK, "Failed to lock buffer, hr %#lx.\n", hr);

        hr = IMF2DBuffer2_Lock2DSize(_2dbuffer2, MF2DBuffer_LockFlags_Read, &data2, &pitch, &buffer_start, &length);
        ok(hr == S_OK, "Failed to lock buffer, hr %#lx.\n", hr);

        hr = IMF2DBuffer_Unlock2D(_2dbuffer);
        ok(hr == S_OK, "Failed to unlock buffer, hr %#lx.\n", hr);

        hr = IMF2DBuffer_Unlock2D(_2dbuffer);
        ok(hr == S_OK, "Failed to unlock buffer, hr %#lx.\n", hr);

        /* Flags are ignored. */
        hr = IMF2DBuffer2_Lock2DSize(_2dbuffer2, MF2DBuffer_LockFlags_Read, &data2, &pitch, &buffer_start, &length);
        ok(hr == S_OK, "Failed to lock buffer, hr %#lx.\n", hr);

        hr = IMF2DBuffer2_Lock2DSize(_2dbuffer2, MF2DBuffer_LockFlags_Write, &data2, &pitch, &buffer_start, &length);
        ok(hr == S_OK, "Failed to lock buffer, hr %#lx.\n", hr);

        hr = IMF2DBuffer_Unlock2D(_2dbuffer);
        ok(hr == S_OK, "Failed to unlock buffer, hr %#lx.\n", hr);

        hr = IMF2DBuffer_Unlock2D(_2dbuffer);
        ok(hr == S_OK, "Failed to unlock buffer, hr %#lx.\n", hr);

        hr = IMF2DBuffer2_Lock2DSize(_2dbuffer2, MF2DBuffer_LockFlags_Write, &data2, &pitch, NULL, &length);
        ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

        hr = IMF2DBuffer2_Lock2DSize(_2dbuffer2, MF2DBuffer_LockFlags_Write, &data2, &pitch, &buffer_start, NULL);
        ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

        IMF2DBuffer2_Release(_2dbuffer2);
    }
    else
        win_skip("IMF2DBuffer2 is not supported.\n");

    IMF2DBuffer_Release(_2dbuffer);

    IMFMediaBuffer_Release(buffer);

    for (i = 0; i < ARRAY_SIZE(image_size_tests); ++i)
    {
        const struct image_size_test *ptr = &image_size_tests[i];

        if (is_MEDIASUBTYPE_RGB(ptr->subtype))
            continue;

        winetest_push_context("%u, %u x %u, format %s", i, ptr->width, ptr->height, wine_dbgstr_guid(ptr->subtype));

        hr = pMFCreate2DMediaBuffer(ptr->width, ptr->height, ptr->subtype->Data1, FALSE, &buffer);
        ok(hr == S_OK, "Failed to create a buffer, hr %#lx.\n", hr);

        hr = IMFMediaBuffer_GetMaxLength(buffer, &length);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(length == ptr->max_length, "Unexpected maximum length %lu.\n", length);

        hr = IMFMediaBuffer_QueryInterface(buffer, &IID_IMF2DBuffer, (void **)&_2dbuffer);
        ok(hr == S_OK, "Failed to get interface, hr %#lx.\n", hr);

        hr = IMFMediaBuffer_QueryInterface(buffer, &IID_IMF2DBuffer2, (void **)&_2dbuffer2);
        ok(hr == S_OK, "Failed to get interface, hr %#lx.\n", hr);

        hr = IMF2DBuffer_GetContiguousLength(_2dbuffer, &length);
        ok(hr == S_OK, "Failed to get length, hr %#lx.\n", hr);
        ok(length == ptr->contiguous_length, "Unexpected contiguous length %lu.\n", length);

        data2 = malloc(ptr->contiguous_length + 16);
        ok(!!data2, "Failed to allocate buffer.\n");

        for (j = 0; j < ptr->contiguous_length + 16; j++)
            data2[j] = j & 0x7f;

        hr = IMF2DBuffer2_ContiguousCopyFrom(_2dbuffer2, data2, ptr->contiguous_length - 1);
        ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

        hr = IMFMediaBuffer_Lock(buffer, &data, &length2, NULL);
        ok(hr == S_OK, "Failed to lock buffer, hr %#lx.\n", hr);
        ok(length2 == ptr->contiguous_length, "Unexpected linear buffer length %lu.\n", length2);

        memset(data, 0xff, length2);

        hr = IMFMediaBuffer_Unlock(buffer);
        ok(hr == S_OK, "Failed to unlock buffer, hr %#lx.\n", hr);

        hr = IMF2DBuffer2_ContiguousCopyFrom(_2dbuffer2, data2, ptr->contiguous_length + 16);
        ok(hr == S_OK, "Failed to copy from contiguous buffer, hr %#lx.\n", hr);

        hr = IMFMediaBuffer_Lock(buffer, &data, &length2, NULL);
        ok(hr == S_OK, "Failed to lock buffer, hr %#lx.\n", hr);
        ok(length2 == ptr->contiguous_length, "%d: unexpected linear buffer length %lu for %u x %u, format %s.\n",
                i, length2, ptr->width, ptr->height, wine_dbgstr_guid(ptr->subtype));

        for (j = 0; j < ptr->contiguous_length; j++)
        {
            if (IsEqualGUID(ptr->subtype, &MFVideoFormat_IMC1) || IsEqualGUID(ptr->subtype, &MFVideoFormat_IMC3))
            {
                if (j < ptr->height * ptr->pitch && j % ptr->pitch >= ptr->width)
                    continue;
                if (j >= ptr->height * ptr->pitch && j % ptr->pitch >= ptr->width / 2)
                    continue;
            }
            if (data[j] != (j & 0x7f))
                break;
        }
        ok(j == ptr->contiguous_length, "Unexpected byte %02x instead of %02x at position %u.\n", data[j], j & 0x7f, j);

        memset(data, 0xff, length2);

        hr = IMFMediaBuffer_Unlock(buffer);
        ok(hr == S_OK, "Failed to unlock buffer, hr %#lx.\n", hr);

        hr = IMF2DBuffer2_ContiguousCopyFrom(_2dbuffer2, data2, ptr->contiguous_length);
        ok(hr == S_OK, "Failed to copy from contiguous buffer, hr %#lx.\n", hr);

        hr = IMFMediaBuffer_Lock(buffer, &data, &length2, NULL);
        ok(hr == S_OK, "Failed to lock buffer, hr %#lx.\n", hr);
        ok(length2 == ptr->contiguous_length, "%d: unexpected linear buffer length %lu for %u x %u, format %s.\n",
                i, length2, ptr->width, ptr->height, wine_dbgstr_guid(ptr->subtype));

        for (j = 0; j < ptr->contiguous_length; j++)
        {
            if (IsEqualGUID(ptr->subtype, &MFVideoFormat_IMC1) || IsEqualGUID(ptr->subtype, &MFVideoFormat_IMC3))
            {
                if (j < ptr->height * ptr->pitch && j % ptr->pitch >= ptr->width)
                    continue;
                if (j >= ptr->height * ptr->pitch && j % ptr->pitch >= ptr->width / 2)
                    continue;
            }
            if (data[j] != (j & 0x7f))
                break;
        }
        ok(j == ptr->contiguous_length, "Unexpected byte %02x instead of %02x at position %u.\n", data[j], j & 0x7f, j);

        hr = IMFMediaBuffer_Unlock(buffer);
        ok(hr == S_OK, "Failed to unlock buffer, hr %#lx.\n", hr);

        hr = IMF2DBuffer2_ContiguousCopyTo(_2dbuffer2, data2, ptr->contiguous_length - 1);
        ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

        memset(data2, 0xff, ptr->contiguous_length + 16);

        hr = IMF2DBuffer2_ContiguousCopyTo(_2dbuffer2, data2, ptr->contiguous_length + 16);
        ok(hr == S_OK, "Failed to copy to contiguous buffer, hr %#lx.\n", hr);

        for (j = 0; j < ptr->contiguous_length; j++)
        {
            if (IsEqualGUID(ptr->subtype, &MFVideoFormat_IMC1) || IsEqualGUID(ptr->subtype, &MFVideoFormat_IMC3))
            {
                if (j < ptr->height * ptr->pitch && j % ptr->pitch >= ptr->width)
                    continue;
                if (j >= ptr->height * ptr->pitch && j % ptr->pitch >= ptr->width / 2)
                    continue;
            }
            if (data2[j] != (j & 0x7f))
                break;
        }
        ok(j == ptr->contiguous_length, "Unexpected byte %02x instead of %02x at position %u.\n", data2[j], j & 0x7f, j);

        memset(data2, 0xff, ptr->contiguous_length + 16);

        hr = IMF2DBuffer2_ContiguousCopyTo(_2dbuffer2, data2, ptr->contiguous_length);
        ok(hr == S_OK, "Failed to copy to contiguous buffer, hr %#lx.\n", hr);

        for (j = 0; j < ptr->contiguous_length; j++)
        {
            if (IsEqualGUID(ptr->subtype, &MFVideoFormat_IMC1) || IsEqualGUID(ptr->subtype, &MFVideoFormat_IMC3))
            {
                if (j < ptr->height * ptr->pitch && j % ptr->pitch >= ptr->width)
                    continue;
                if (j >= ptr->height * ptr->pitch && j % ptr->pitch >= ptr->width / 2)
                    continue;
            }
            if (data2[j] != (j & 0x7f))
                break;
        }
        ok(j == ptr->contiguous_length, "Unexpected byte %02x instead of %02x at position %u.\n", data2[j], j & 0x7f, j);

        free(data2);

        hr = IMFMediaBuffer_Lock(buffer, &data, &length2, NULL);
        ok(hr == S_OK, "Failed to lock buffer, hr %#lx.\n", hr);
        ok(length2 == ptr->contiguous_length, "%d: unexpected linear buffer length %lu for %u x %u, format %s.\n",
                i, length2, ptr->width, ptr->height, wine_dbgstr_guid(ptr->subtype));

        hr = pMFGetStrideForBitmapInfoHeader(ptr->subtype->Data1, ptr->width, &stride);
        ok(hr == S_OK, "Failed to get stride, hr %#lx.\n", hr);
        stride = abs(stride);

        /* primary plane */
        ok(ptr->height * stride <= length2, "Insufficient buffer space: expected at least %lu bytes, got only %lu\n",
                ptr->height * stride, length2);
        for (j = 0; j < ptr->height; j++)
            for (k = 0; k < stride; k++)
                data[j * stride + k] = ((j % 16) << 4) + (k % 16);

        data += ptr->height * stride;

        /* secondary planes */
        switch (ptr->subtype->Data1)
        {
            case MAKEFOURCC('I','M','C','1'):
            case MAKEFOURCC('I','M','C','3'):
                ok(2 * ptr->height * stride <= length2, "Insufficient buffer space: expected at least %lu bytes, got only %lu\n",
                        2 * ptr->height * stride, length2);
                for (j = 0; j < ptr->height; j++)
                    for (k = 0; k < stride / 2; k++)
                        data[j * stride + k] = (((j + ptr->height) % 16) << 4) + (k % 16);
                break;

            case MAKEFOURCC('I','M','C','2'):
            case MAKEFOURCC('I','M','C','4'):
            case MAKEFOURCC('N','V','1','1'):
            case MAKEFOURCC('Y','V','1','2'):
            case MAKEFOURCC('I','4','2','0'):
            case MAKEFOURCC('I','Y','U','V'):
                ok(stride * 3 / 2 * ptr->height <= length2, "Insufficient buffer space: expected at least %lu bytes, got only %lu\n",
                        stride * 3 / 2 * ptr->height, length2);
                for (j = 0; j < ptr->height; j++)
                    for (k = 0; k < stride / 2; k++)
                        data[j * (stride / 2) + k] = (((j + ptr->height) % 16) << 4) + (k % 16);
                break;

            case MAKEFOURCC('N','V','1','2'):
                ok(stride * ptr->height * 3 / 2 <= length2, "Insufficient buffer space: expected at least %lu bytes, got only %lu\n",
                        stride * ptr->height * 3 / 2, length2);
                for (j = 0; j < ptr->height / 2; j++)
                    for (k = 0; k < stride; k++)
                        data[j * stride + k] = (((j + ptr->height) % 16) << 4) + (k % 16);
                break;
        }

        hr = IMFMediaBuffer_Unlock(buffer);
        ok(hr == S_OK, "Failed to unlock buffer, hr %#lx.\n", hr);

        hr = IMF2DBuffer_Lock2D(_2dbuffer, &data, &pitch);
        ok(hr == S_OK, "Failed to lock buffer, hr %#lx.\n", hr);

        hr = IMF2DBuffer_GetScanline0AndPitch(_2dbuffer, &data2, &pitch2);
        ok(hr == S_OK, "Failed to get scanline, hr %#lx.\n", hr);
        ok(data2 == data, "Unexpected data pointer.\n");
        ok(pitch == pitch2, "Unexpected pitch.\n");

        /* primary plane */
        for (j = 0; j < ptr->height; j++)
            for (k = 0; k < stride; k++)
                ok(data[j * pitch + k] == ((j % 16) << 4) + (k % 16),
                        "Unexpected byte %02x instead of %02x at row %d column %d.\n",
                        data[j * pitch + k], ((j % 16) << 4) + (k % 16), j, k);

        data += ptr->height * pitch;

        /* secondary planes */
        switch (ptr->subtype->Data1)
        {
            case MAKEFOURCC('I','M','C','1'):
            case MAKEFOURCC('I','M','C','3'):
                for (j = 0; j < ptr->height; j++)
                    for (k = 0; k < stride / 2; k++)
                        ok(data[j * pitch + k] == (((j + ptr->height) % 16) << 4) + (k % 16),
                                "Unexpected byte %02x instead of %02x at row %d column %d.\n",
                                data[j * pitch + k], (((j + ptr->height) % 16) << 4) + (k % 16), j + ptr->height, k);
                break;

            case MAKEFOURCC('I','M','C','2'):
            case MAKEFOURCC('I','M','C','4'):
            case MAKEFOURCC('N','V','1','1'):
            case MAKEFOURCC('Y','V','1','2'):
            case MAKEFOURCC('I','4','2','0'):
            case MAKEFOURCC('I','Y','U','V'):
                for (j = 0; j < ptr->height; j++)
                    for (k = 0; k < stride / 2; k++)
                        ok(data[j * (pitch / 2) + k] == (((j + ptr->height) % 16) << 4) + (k % 16),
                                "Unexpected byte %02x instead of %02x at row %d column %d.\n",
                                data[j * (pitch / 2) + k], (((j + ptr->height) % 16) << 4) + (k % 16), j + ptr->height, k);
                break;

            case MAKEFOURCC('N','V','1','2'):
                for (j = 0; j < ptr->height / 2; j++)
                    for (k = 0; k < stride; k++)
                        ok(data[j * pitch + k] == (((j + ptr->height) % 16) << 4) + (k % 16),
                                "Unexpected byte %02x instead of %02x at row %d column %d.\n",
                                data[j * pitch + k], (((j + ptr->height) % 16) << 4) + (k % 16), j + ptr->height, k);
                break;

            default:
                ;
        }

        hr = IMF2DBuffer_Unlock2D(_2dbuffer);
        ok(hr == S_OK, "Failed to unlock buffer, hr %#lx.\n", hr);

        ok(pitch == ptr->pitch, "Unexpected pitch %ld, expected %d.\n", pitch, ptr->pitch);

        ret = TRUE;
        hr = IMF2DBuffer_IsContiguousFormat(_2dbuffer, &ret);
        ok(hr == S_OK, "Failed to get format flag, hr %#lx.\n", hr);
        ok(!ret, "%d: unexpected format flag %d.\n", i, ret);

        IMF2DBuffer_Release(_2dbuffer);
        IMF2DBuffer2_Release(_2dbuffer2);

        IMFMediaBuffer_Release(buffer);

        winetest_pop_context();
    }

    /* Alignment tests */
    for (i = 0; i < ARRAY_SIZE(image_size_tests); ++i)
    {
        const struct image_size_test *ptr = &image_size_tests[i];

        if (is_MEDIASUBTYPE_RGB(ptr->subtype))
            continue;

        hr = pMFCreate2DMediaBuffer(ptr->width, ptr->height, ptr->subtype->Data1, FALSE, &buffer);
        ok(hr == S_OK, "Failed to create a buffer, hr %#lx.\n", hr);

        hr = IMFMediaBuffer_QueryInterface(buffer, &IID_IMF2DBuffer, (void **)&_2dbuffer);
        ok(hr == S_OK, "Failed to get interface, hr %#lx.\n", hr);

        hr = IMF2DBuffer_Lock2D(_2dbuffer, &data, &pitch);
        ok(hr == S_OK, "Failed to lock buffer, hr %#lx.\n", hr);
        ok(((uintptr_t)data & MF_64_BYTE_ALIGNMENT) == 0, "Misaligned data at %p.\n", data);

        hr = IMF2DBuffer_Unlock2D(_2dbuffer);
        ok(hr == S_OK, "Failed to unlock buffer, hr %#lx.\n", hr);

        IMF2DBuffer_Release(_2dbuffer);
        IMFMediaBuffer_Release(buffer);
    }
}

static void test_MFCreateMediaBufferFromMediaType(void)
{
    static struct audio_buffer_test
    {
        unsigned int duration;
        unsigned int min_length;
        unsigned int min_alignment;
        unsigned int block_alignment;
        unsigned int bytes_per_second;
        unsigned int buffer_length;
    } audio_tests[] =
    {
        { 0,  0,   0,  4,  0, 20 },
        { 0, 16,   0,  4,  0, 20 },
        { 0,  0,  32,  4,  0, 36 },
        { 0, 64,  32,  4,  0, 64 },
        { 1,  0,   0,  4, 16, 36 },
        { 2,  0,   0,  4, 16, 52 },
        { 2,  0,  64,  4, 16, 68 },
        { 2,  0, 128,  4, 16,132 },
    };
    IMFMediaType *media_type, *media_type2;
    unsigned int i, alignment;
    IMF2DBuffer2 *buffer_2d2;
    IMFMediaBuffer *buffer;
    IMF2DBuffer *buffer_2d;
    BYTE *data, *scanline0;
    DWORD length, max;
    LONG pitch;
    HRESULT hr;

    if (!pMFCreateMediaBufferFromMediaType)
    {
        win_skip("MFCreateMediaBufferFromMediaType() is not available.\n");
        return;
    }

    hr = pMFCreateMediaBufferFromMediaType(NULL, 0, 0, 0, &buffer);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);

    hr = pMFCreateMediaBufferFromMediaType(media_type, 0, 0, 0, &buffer);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &GUID_NULL);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);
    hr = pMFCreateMediaBufferFromMediaType(media_type, 0, 0, 0, &buffer);
    todo_wine ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    hr = pMFCreateMediaBufferFromMediaType(media_type, 0, 16, 0, &buffer);
    todo_wine ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (hr == S_OK)
    {
    hr = IMFMediaBuffer_GetMaxLength(buffer, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(length == 16, "Got length %#lx.\n", length);
    IMFMediaBuffer_Release(buffer);
    }

    hr = MFCreateMediaType(&media_type2);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

    hr = IMFMediaType_CopyAllItems(media_type, (IMFAttributes *)media_type2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    for (i = 0; i < ARRAY_SIZE(audio_tests); ++i)
    {
        const struct audio_buffer_test *ptr = &audio_tests[i];

        hr = IMFMediaType_SetUINT32(media_type, &MF_MT_AUDIO_BLOCK_ALIGNMENT, ptr->block_alignment);
        ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

        hr = IMFMediaType_SetUINT32(media_type, &MF_MT_AUDIO_AVG_BYTES_PER_SECOND, ptr->bytes_per_second);
        ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

        hr = pMFCreateMediaBufferFromMediaType(media_type, ptr->duration * 10000000, ptr->min_length,
                ptr->min_alignment, &buffer);
        ok(hr == S_OK || broken(FAILED(hr)) /* Win8 */, "Unexpected hr %#lx.\n", hr);
        if (FAILED(hr))
            break;

        check_interface(buffer, &IID_IMFGetService, FALSE);

        hr = IMFMediaBuffer_GetMaxLength(buffer, &length);
        ok(hr == S_OK, "Failed to get length, hr %#lx.\n", hr);
        ok(ptr->buffer_length == length, "%d: unexpected buffer length %lu, expected %u.\n", i, length, ptr->buffer_length);

        alignment = ptr->min_alignment ? ptr->min_alignment - 1 : MF_16_BYTE_ALIGNMENT;
        hr = IMFMediaBuffer_Lock(buffer, &data, &max, &length);
        ok(hr == S_OK, "Failed to lock, hr %#lx.\n", hr);
        ok(ptr->buffer_length == max && !length, "Unexpected length.\n");
        ok(!((uintptr_t)data & alignment), "%u: data at %p is misaligned.\n", i, data);
        hr = IMFMediaBuffer_Unlock(buffer);
        ok(hr == S_OK, "Failed to unlock, hr %#lx.\n", hr);

        IMFMediaBuffer_Release(buffer);

        /* Only major type is set. */
        hr = pMFCreateMediaBufferFromMediaType(media_type2, ptr->duration * 10000000, ptr->min_length,
                ptr->min_alignment, &buffer);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IMFMediaBuffer_GetMaxLength(buffer, &length);
        ok(hr == S_OK, "Failed to get length, hr %#lx.\n", hr);
        ok(ptr->min_length == length, "%u: unexpected buffer length %lu, expected %u.\n", i, length, ptr->min_length);

        hr = IMFMediaBuffer_Lock(buffer, &data, &max, &length);
        ok(hr == S_OK, "Failed to lock, hr %#lx.\n", hr);
        ok(ptr->min_length == max && !length, "Unexpected length.\n");
        ok(!((uintptr_t)data & alignment), "%u: data at %p is misaligned.\n", i, data);
        hr = IMFMediaBuffer_Unlock(buffer);
        ok(hr == S_OK, "Failed to unlock, hr %#lx.\n", hr);

        IMFMediaBuffer_Release(buffer);
    }

    IMFMediaType_Release(media_type2);


    hr = IMFMediaType_DeleteAllItems(media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* MF_MT_SUBTYPE is required unless min length is provided */
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);
    hr = pMFCreateMediaBufferFromMediaType(media_type, 0, 0, 0, &buffer);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    hr = pMFCreateMediaBufferFromMediaType(media_type, 0, 16, 0, &buffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaBuffer_Lock(buffer, &data, &max, &length);
    ok(hr == S_OK, "Failed to lock, hr %#lx.\n", hr);
    ok(max == 16, "Unexpected max length.\n");
    ok(length == 0, "Unexpected length.\n");
    ok(!((uintptr_t)data & 0xf), "%u: data at %p is misaligned.\n", i, data);
    hr = IMFMediaBuffer_Unlock(buffer);
    ok(hr == S_OK, "Failed to unlock, hr %#lx.\n", hr);
    IMFMediaBuffer_Release(buffer);

    /* MF_MT_FRAME_SIZE is required  unless min length is provided */
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFVideoFormat_RGB32);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);
    hr = pMFCreateMediaBufferFromMediaType(media_type, 0, 0, 0, &buffer);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    hr = pMFCreateMediaBufferFromMediaType(media_type, 0, 16, 0, &buffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaBuffer_Lock(buffer, &data, &max, &length);
    ok(hr == S_OK, "Failed to lock, hr %#lx.\n", hr);
    ok(max == 16, "Unexpected max length.\n");
    ok(length == 0, "Unexpected length.\n");
    ok(!((uintptr_t)data & 0xf), "%u: data at %p is misaligned.\n", i, data);
    hr = IMFMediaBuffer_Unlock(buffer);
    ok(hr == S_OK, "Failed to unlock, hr %#lx.\n", hr);
    IMFMediaBuffer_Release(buffer);

    /* MF_MT_SAMPLE_SIZE / MF_MT_FIXED_SIZE_SAMPLES / MF_MT_COMPRESSED don't have any effect */
    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_SAMPLE_SIZE, 1024);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);
    hr = pMFCreateMediaBufferFromMediaType(media_type, 0, 0, 0, &buffer);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_FIXED_SIZE_SAMPLES, 1);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);
    hr = pMFCreateMediaBufferFromMediaType(media_type, 0, 0, 0, &buffer);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_COMPRESSED, 0);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);
    hr = pMFCreateMediaBufferFromMediaType(media_type, 0, 0, 0, &buffer);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    /* MF_MT_FRAME_SIZE forces the buffer size, regardless of min length */
    hr = IMFMediaType_SetUINT64(media_type, &MF_MT_FRAME_SIZE, (UINT64)7 << 32 | 8);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);
    hr = pMFCreateMediaBufferFromMediaType(media_type, 0, 0, 0, &buffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaBuffer_Lock(buffer, &data, &max, &length);
    ok(hr == S_OK, "Failed to lock, hr %#lx.\n", hr);
    ok(max == 224, "Unexpected max length.\n");
    ok(length == 224, "Unexpected length.\n");
    hr = IMFMediaBuffer_Unlock(buffer);
    ok(hr == S_OK, "Failed to unlock, hr %#lx.\n", hr);

    /* Video media type buffers implement IMF2DBuffer */
    hr = IMFMediaBuffer_QueryInterface(buffer, &IID_IMF2DBuffer, (void **)&buffer_2d);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMF2DBuffer_Lock2D(buffer_2d, &data, &pitch);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(pitch == 64, "got pitch %ld.\n", pitch);
    hr = IMF2DBuffer_Unlock2D(buffer_2d);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMF2DBuffer_Release(buffer_2d);

    /* IMF2DBuffer2 is also implemented */
    hr = IMFMediaBuffer_QueryInterface(buffer, &IID_IMF2DBuffer2, (void **)&buffer_2d2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMF2DBuffer2_Lock2DSize(buffer_2d2, MF2DBuffer_LockFlags_Read, &scanline0, &pitch, &data, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(pitch == 64, "got pitch %ld.\n", pitch);
    ok(length == 512, "Unexpected length.\n");
    ok(scanline0 == data, "Unexpected scanline0.\n");
    hr = IMF2DBuffer2_Unlock2D(buffer_2d2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMF2DBuffer_Release(buffer_2d);

    IMFMediaBuffer_Release(buffer);

    /* minimum buffer size value is ignored */
    hr = pMFCreateMediaBufferFromMediaType(media_type, 0, 4096, 0, &buffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaBuffer_Lock(buffer, &data, &max, &length);
    ok(hr == S_OK, "Failed to lock, hr %#lx.\n", hr);
    ok(max == 224, "got max %lu.\n", max);
    ok(length == 224, "got length %lu.\n", length);
    hr = IMFMediaBuffer_Unlock(buffer);
    ok(hr == S_OK, "Failed to unlock, hr %#lx.\n", hr);

    hr = IMFMediaBuffer_QueryInterface(buffer, &IID_IMF2DBuffer, (void **)&buffer_2d);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMF2DBuffer_Lock2D(buffer_2d, &data, &pitch);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(pitch == 64, "got pitch %ld.\n", pitch);
    hr = IMF2DBuffer_Unlock2D(buffer_2d);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMF2DBuffer_Release(buffer_2d);

    IMFMediaBuffer_Release(buffer);

    /* minimum alignment value is ignored */
    hr = pMFCreateMediaBufferFromMediaType(media_type, 0, 0, 0x1fff, &buffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaBuffer_Lock(buffer, &data, &max, &length);
    ok(hr == S_OK, "Failed to lock, hr %#lx.\n", hr);
    ok(max == 224, "got max %lu.\n", max);
    ok(length == 224, "got length %lu.\n", length);
    hr = IMFMediaBuffer_Unlock(buffer);
    ok(hr == S_OK, "Failed to unlock, hr %#lx.\n", hr);

    hr = IMFMediaBuffer_QueryInterface(buffer, &IID_IMF2DBuffer, (void **)&buffer_2d);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMF2DBuffer_Lock2D(buffer_2d, &data, &pitch);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(pitch == 64, "got pitch %ld.\n", pitch);
    hr = IMF2DBuffer_Unlock2D(buffer_2d);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMF2DBuffer_Release(buffer_2d);

    IMFMediaBuffer_Release(buffer);

    /* MF_MT_DEFAULT_STRIDE value is ignored */
    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_DEFAULT_STRIDE, -123456);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);
    hr = pMFCreateMediaBufferFromMediaType(media_type, 0, 0, 0, &buffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaBuffer_Lock(buffer, &data, &max, &length);
    ok(hr == S_OK, "Failed to lock, hr %#lx.\n", hr);
    ok(max == 224, "got max %lu.\n", max);
    ok(length == 224, "got length %lu.\n", length);
    hr = IMFMediaBuffer_Unlock(buffer);
    ok(hr == S_OK, "Failed to unlock, hr %#lx.\n", hr);

    hr = IMFMediaBuffer_QueryInterface(buffer, &IID_IMF2DBuffer, (void **)&buffer_2d);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMF2DBuffer_Lock2D(buffer_2d, &data, &pitch);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(pitch == -64, "got pitch %ld.\n", pitch);
    hr = IMF2DBuffer_Unlock2D(buffer_2d);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMF2DBuffer_Release(buffer_2d);

    IMFMediaBuffer_Release(buffer);

    /* MF_MT_DEFAULT_STRIDE sign controls bottom-up */
    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_DEFAULT_STRIDE, 123456);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);
    hr = pMFCreateMediaBufferFromMediaType(media_type, 0, 0, 0, &buffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaBuffer_QueryInterface(buffer, &IID_IMF2DBuffer2, (void **)&buffer_2d2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMF2DBuffer2_Lock2DSize(buffer_2d2, MF2DBuffer_LockFlags_Read, &scanline0, &pitch, &data, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(pitch == 64, "got pitch %ld.\n", pitch);
    ok(length == 64 * 8, "Unexpected length.\n");
    ok(scanline0 == data, "Unexpected scanline0.\n");
    hr = IMF2DBuffer2_Unlock2D(buffer_2d2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMF2DBuffer2_Release(buffer_2d2);

    IMFMediaBuffer_Release(buffer);

    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_DEFAULT_STRIDE, -123456);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);
    hr = pMFCreateMediaBufferFromMediaType(media_type, 0, 0, 0, &buffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaBuffer_QueryInterface(buffer, &IID_IMF2DBuffer2, (void **)&buffer_2d2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMF2DBuffer2_Lock2DSize(buffer_2d2, MF2DBuffer_LockFlags_Read, &scanline0, &pitch, &data, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(pitch == -64, "got pitch %ld.\n", pitch);
    ok(length == 64 * 8, "Unexpected length.\n");
    ok(scanline0 == data - pitch * 7, "Unexpected scanline0.\n");
    hr = IMF2DBuffer2_Unlock2D(buffer_2d2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMF2DBuffer2_Release(buffer_2d2);

    IMFMediaBuffer_Release(buffer);

    /* MF_MT_FRAME_SIZE doesn't work with compressed formats */
    hr = IMFMediaType_DeleteItem(media_type, &MF_MT_FRAME_SIZE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFVideoFormat_H264);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT64(media_type, &MF_MT_FRAME_SIZE, (UINT64)16 << 32 | 32);
    ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);
    hr = pMFCreateMediaBufferFromMediaType(media_type, 0, 0, 0, &buffer);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    IMFMediaType_Release(media_type);
}

static void validate_media_type(IMFMediaType *mediatype, const WAVEFORMATEX *format)
{
    GUID guid, subtype;
    UINT32 value;
    HRESULT hr;

    hr = IMFMediaType_GetMajorType(mediatype, &guid);
    ok(hr == S_OK, "Failed to get major type, hr %#lx.\n", hr);
    ok(IsEqualGUID(&guid, &MFMediaType_Audio), "Unexpected major type %s.\n", wine_dbgstr_guid(&guid));

    hr = IMFMediaType_GetGUID(mediatype, &MF_MT_SUBTYPE, &guid);
    ok(hr == S_OK, "Failed to get subtype, hr %#lx.\n", hr);

    if (format->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
    {
        const WAVEFORMATEXTENSIBLE *fex = (const WAVEFORMATEXTENSIBLE *)format;
        ok(IsEqualGUID(&guid, &fex->SubFormat), "Unexpected subtype %s.\n", wine_dbgstr_guid(&guid));

        if (fex->dwChannelMask)
        {
            hr = IMFMediaType_GetUINT32(mediatype, &MF_MT_AUDIO_CHANNEL_MASK, &value);
            ok(hr == S_OK, "Failed to get attribute, hr %#lx.\n", hr);
            ok(value == fex->dwChannelMask, "Unexpected CHANNEL_MASK %#x.\n", value);
        }

        if (format->wBitsPerSample && fex->Samples.wValidBitsPerSample)
        {
            hr = IMFMediaType_GetUINT32(mediatype, &MF_MT_AUDIO_VALID_BITS_PER_SAMPLE, &value);
            ok(hr == S_OK, "Failed to get attribute, hr %#lx.\n", hr);
            ok(value == fex->Samples.wValidBitsPerSample, "Unexpected VALID_BITS_PER_SAMPLE %#x.\n", value);
        }
    }
    else
    {
        memcpy(&subtype, &MFAudioFormat_Base, sizeof(subtype));
        subtype.Data1 = format->wFormatTag;
        ok(IsEqualGUID(&guid, &subtype), "Unexpected subtype %s.\n", wine_dbgstr_guid(&guid));

        hr = IMFMediaType_GetUINT32(mediatype, &MF_MT_AUDIO_PREFER_WAVEFORMATEX, &value);
        ok(hr == S_OK, "Failed to get attribute, hr %#lx.\n", hr);
        ok(value, "Unexpected value.\n");
    }

    if (format->nChannels)
    {
        hr = IMFMediaType_GetUINT32(mediatype, &MF_MT_AUDIO_NUM_CHANNELS, &value);
        ok(hr == S_OK, "Failed to get attribute, hr %#lx.\n", hr);
        ok(value == format->nChannels, "Unexpected NUM_CHANNELS %u.\n", value);
    }

    if (format->nSamplesPerSec)
    {
        hr = IMFMediaType_GetUINT32(mediatype, &MF_MT_AUDIO_SAMPLES_PER_SECOND, &value);
        ok(hr == S_OK, "Failed to get attribute, hr %#lx.\n", hr);
        ok(value == format->nSamplesPerSec, "Unexpected SAMPLES_PER_SECOND %u.\n", value);
    }

    if (format->nAvgBytesPerSec)
    {
        hr = IMFMediaType_GetUINT32(mediatype, &MF_MT_AUDIO_AVG_BYTES_PER_SECOND, &value);
        ok(hr == S_OK, "Failed to get attribute, hr %#lx.\n", hr);
        ok(value == format->nAvgBytesPerSec, "Unexpected AVG_BYTES_PER_SECOND %u.\n", value);
    }

    if (format->nBlockAlign)
    {
        hr = IMFMediaType_GetUINT32(mediatype, &MF_MT_AUDIO_BLOCK_ALIGNMENT, &value);
        ok(hr == S_OK, "Failed to get attribute, hr %#lx.\n", hr);
        ok(value == format->nBlockAlign, "Unexpected BLOCK_ALIGNMENT %u.\n", value);
    }

    if (format->wBitsPerSample)
    {
        hr = IMFMediaType_GetUINT32(mediatype, &MF_MT_AUDIO_BITS_PER_SAMPLE, &value);
        ok(hr == S_OK, "Failed to get attribute, hr %#lx.\n", hr);
        ok(value == format->wBitsPerSample, "Unexpected BITS_PER_SAMPLE %u.\n", value);
    }

    /* Only set for uncompressed formats. */
    hr = IMFMediaType_GetUINT32(mediatype, &MF_MT_ALL_SAMPLES_INDEPENDENT, &value);
    if (IsEqualGUID(&guid, &MFAudioFormat_Float) ||
            IsEqualGUID(&guid, &MFAudioFormat_PCM))
    {
        ok(hr == S_OK, "Failed to get attribute, hr %#lx.\n", hr);
        ok(value, "Unexpected ALL_SAMPLES_INDEPENDENT value.\n");
    }
    else
        ok(FAILED(hr), "Unexpected ALL_SAMPLES_INDEPENDENT.\n");

    hr = IMFMediaType_GetUINT32(mediatype, &MF_MT_FIXED_SIZE_SAMPLES, &value);
    ok(FAILED(hr), "Unexpected FIXED_SIZE_SAMPLES.\n");
    hr = IMFMediaType_GetUINT32(mediatype, &MF_MT_COMPRESSED, &value);
    ok(FAILED(hr), "Unexpected COMPRESSED.\n");
}

static void test_MFInitMediaTypeFromWaveFormatEx(void)
{
    static const WAVEFORMATEX waveformatex_tests[] =
    {
        { WAVE_FORMAT_PCM, 2, 44100, 0, 2, 8 },
        { WAVE_FORMAT_PCM, 2, 44100, 1, 2, 8 },
        { WAVE_FORMAT_PCM, 0, 44100, 0, 0, 0 },
        { WAVE_FORMAT_PCM, 0,     0, 0, 0, 0 },
        { WAVE_FORMAT_IEEE_FLOAT, 2, 44100, 1, 2, 8 },
        { 1234, 0,     0, 0, 0, 0 },
        { WAVE_FORMAT_ALAW },
        { WAVE_FORMAT_CREATIVE_ADPCM },
        { WAVE_FORMAT_MPEGLAYER3 },
        { WAVE_FORMAT_MPEG_ADTS_AAC },
        { WAVE_FORMAT_ALAC },
        { WAVE_FORMAT_AMR_NB },
        { WAVE_FORMAT_AMR_WB },
        { WAVE_FORMAT_AMR_WP },
        { WAVE_FORMAT_DOLBY_AC3_SPDIF },
        { WAVE_FORMAT_DRM },
        { WAVE_FORMAT_DTS },
        { WAVE_FORMAT_FLAC },
        { WAVE_FORMAT_MPEG },
        { WAVE_FORMAT_WMAVOICE9 },
        { WAVE_FORMAT_OPUS },
        { WAVE_FORMAT_WMAUDIO2 },
        { WAVE_FORMAT_WMAUDIO3 },
        { WAVE_FORMAT_WMAUDIO_LOSSLESS },
        { WAVE_FORMAT_WMASPDIF },
    };

    static const BYTE aac_codec_data[] =
    {
        0x12, 0x00,
        0x34, 0x00,
        0x00, 0x00,
        0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x12, 0x08,
    };
    UINT8 buff[1024];
    WAVEFORMATEXTENSIBLE waveformatext;
    MPEGLAYER3WAVEFORMAT mp3format;
    WAVEFORMATEXTENSIBLE *format;
    HEAACWAVEFORMAT aacformat;
    IMFMediaType *mediatype;
    unsigned int i, size;
    WAVEFORMATEX *wfx;
    UINT32 value;
    HRESULT hr;

    hr = MFCreateMediaType(&mediatype);
    ok(hr == S_OK, "Failed to create mediatype, hr %#lx.\n", hr);

    for (i = 0; i < ARRAY_SIZE(waveformatex_tests); ++i)
    {
        hr = MFInitMediaTypeFromWaveFormatEx(mediatype, &waveformatex_tests[i], sizeof(waveformatex_tests[i]));
        ok(hr == S_OK, "%d: format %#x, failed to initialize media type, hr %#lx.\n", i, waveformatex_tests[i].wFormatTag, hr);

        validate_media_type(mediatype, &waveformatex_tests[i]);

        waveformatext.Format = waveformatex_tests[i];
        waveformatext.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
        waveformatext.Format.cbSize = sizeof(waveformatext) - sizeof(waveformatext.Format);
        waveformatext.Samples.wSamplesPerBlock = 123;
        waveformatext.dwChannelMask = 0x8;
        memcpy(&waveformatext.SubFormat, &MFAudioFormat_Base, sizeof(waveformatext.SubFormat));
        waveformatext.SubFormat.Data1 = waveformatex_tests[i].wFormatTag;

        hr = MFInitMediaTypeFromWaveFormatEx(mediatype, &waveformatext.Format, sizeof(waveformatext));
        ok(hr == S_OK, "Failed to initialize media type, hr %#lx.\n", hr);

        hr = IMFMediaType_GetItem(mediatype, &MF_MT_USER_DATA, NULL);
        ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

        validate_media_type(mediatype, &waveformatext.Format);
    }

    /* MPEGLAYER3WAVEFORMAT */
    mp3format.wfx.wFormatTag = WAVE_FORMAT_MPEGLAYER3;
    mp3format.wfx.nChannels = 2;
    mp3format.wfx.nSamplesPerSec = 44100;
    mp3format.wfx.nAvgBytesPerSec = 16000;
    mp3format.wfx.nBlockAlign = 1;
    mp3format.wfx.wBitsPerSample = 0;
    mp3format.wfx.cbSize = MPEGLAYER3_WFX_EXTRA_BYTES;
    mp3format.wID = MPEGLAYER3_ID_MPEG;
    mp3format.fdwFlags = 0;
    mp3format.nBlockSize = 417;
    mp3format.nFramesPerBlock = 0;
    mp3format.nCodecDelay = 0;

    hr = MFInitMediaTypeFromWaveFormatEx(mediatype, (WAVEFORMATEX *)&mp3format, sizeof(mp3format));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    validate_media_type(mediatype, &mp3format.wfx);
    hr = IMFMediaType_GetBlob(mediatype, &MF_MT_USER_DATA, buff, sizeof(buff), &size);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(size == mp3format.wfx.cbSize, "Unexpected size %u.\n", size);
    ok(!memcmp(buff, (WAVEFORMATEX *)&mp3format + 1, size), "Unexpected user data.\n");

    /* HEAACWAVEFORMAT */
    aacformat.wfInfo.wfx.wFormatTag = WAVE_FORMAT_MPEG_HEAAC;
    aacformat.wfInfo.wfx.nChannels = 2;
    aacformat.wfInfo.wfx.nSamplesPerSec = 44100;
    aacformat.wfInfo.wfx.nAvgBytesPerSec = 16000;
    aacformat.wfInfo.wfx.nBlockAlign = 1;
    aacformat.wfInfo.wfx.wBitsPerSample = 0;
    aacformat.wfInfo.wPayloadType = 2;
    aacformat.wfInfo.wAudioProfileLevelIndication = 1;
    aacformat.pbAudioSpecificConfig[0] = 0xcd;

    /* test with invalid format size */
    aacformat.wfInfo.wfx.cbSize = sizeof(aacformat) - 2 - sizeof(WAVEFORMATEX);
    hr = MFInitMediaTypeFromWaveFormatEx(mediatype, (WAVEFORMATEX *)&aacformat, sizeof(aacformat) - 2);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    aacformat.wfInfo.wfx.cbSize = sizeof(aacformat) - sizeof(WAVEFORMATEX);
    hr = MFInitMediaTypeFromWaveFormatEx(mediatype, (WAVEFORMATEX *)&aacformat, sizeof(aacformat));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    validate_media_type(mediatype, &aacformat.wfInfo.wfx);

    value = 0xdeadbeef;
    hr = IMFMediaType_GetUINT32(mediatype, &MF_MT_AAC_AUDIO_PROFILE_LEVEL_INDICATION, &value);
    ok(hr == S_OK, "Failed to get attribute, hr %#lx.\n", hr);
    ok(value == aacformat.wfInfo.wAudioProfileLevelIndication, "Unexpected AAC_AUDIO_PROFILE_LEVEL_INDICATION %u.\n", value);
    value = 0xdeadbeef;
    hr = IMFMediaType_GetUINT32(mediatype, &MF_MT_AAC_PAYLOAD_TYPE, &value);
    ok(hr == S_OK, "Failed to get attribute, hr %#lx.\n", hr);
    ok(value == aacformat.wfInfo.wPayloadType, "Unexpected AAC_PAYLOAD_TYPE %u.\n", value);

    hr = IMFMediaType_GetBlob(mediatype, &MF_MT_USER_DATA, buff, sizeof(buff), &size);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(size == aacformat.wfInfo.wfx.cbSize, "Unexpected size %u.\n", size);
    ok(!memcmp(buff, (WAVEFORMATEX *)&aacformat + 1, size), "Unexpected user data.\n");

    /* check that we get an HEAACWAVEFORMAT by default */
    hr = MFCreateWaveFormatExFromMFMediaType(mediatype, (WAVEFORMATEX **)&wfx, &size, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(wfx->wFormatTag == WAVE_FORMAT_MPEG_HEAAC, "got wFormatTag %#x\n", wfx->wFormatTag);
    ok(wfx->cbSize == sizeof(HEAACWAVEFORMAT) - sizeof(WAVEFORMATEX), "got cbSize %u\n", wfx->cbSize);
    ok(!memcmp(wfx + 1, &aacformat.wfInfo.wfx + 1, aacformat.wfInfo.wfx.cbSize), "Unexpected user data.\n");
    CoTaskMemFree(wfx);

    /* MFWaveFormatExConvertFlag_ForceExtensible can force a WAVEFORMATEXTENSIBLE */
    hr = MFCreateWaveFormatExFromMFMediaType(mediatype, (WAVEFORMATEX **)&format, &size, MFWaveFormatExConvertFlag_ForceExtensible);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(format->Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE, "got wFormatTag %#x\n", format->Format.wFormatTag);
    ok(format->Format.cbSize == aacformat.wfInfo.wfx.cbSize + sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX),
            "got cbSize %u\n", format->Format.cbSize);
    ok(IsEqualGUID(&format->SubFormat, &MFAudioFormat_AAC), "got SubFormat %s\n", debugstr_guid(&format->SubFormat));
    ok(format->dwChannelMask == 3, "got dwChannelMask %#lx\n", format->dwChannelMask);
    ok(format->Samples.wSamplesPerBlock == 0, "got wSamplesPerBlock %u\n", format->Samples.wSamplesPerBlock);
    ok(!memcmp(format + 1, &aacformat.wfInfo.wfx + 1, aacformat.wfInfo.wfx.cbSize), "Unexpected user data.\n");
    CoTaskMemFree(format);

    /* adding more channels has no immediate effect */
    hr = IMFMediaType_SetUINT32(mediatype, &MF_MT_AUDIO_NUM_CHANNELS, 6);
    ok(hr == S_OK, "Failed to get attribute, hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(mediatype, &MF_MT_AUDIO_CHANNEL_MASK, 63);
    ok(hr == S_OK, "Failed to get attribute, hr %#lx.\n", hr);
    hr = MFCreateWaveFormatExFromMFMediaType(mediatype, (WAVEFORMATEX **)&wfx, &size, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(wfx->wFormatTag == WAVE_FORMAT_MPEG_HEAAC, "got wFormatTag %#x\n", wfx->wFormatTag);
    ok(wfx->cbSize == sizeof(HEAACWAVEFORMAT) - sizeof(WAVEFORMATEX), "got cbSize %u\n", wfx->cbSize);
    ok(!memcmp(wfx + 1, &aacformat.wfInfo.wfx + 1, aacformat.wfInfo.wfx.cbSize), "Unexpected user data.\n");
    CoTaskMemFree(wfx);

    /* but adding MF_MT_AUDIO_SAMPLES_PER_BLOCK as well forces the WAVEFORMATEXTENSIBLE format */
    hr = IMFMediaType_SetUINT32(mediatype, &MF_MT_AUDIO_SAMPLES_PER_BLOCK, 4);
    ok(hr == S_OK, "Failed to get attribute, hr %#lx.\n", hr);
    hr = MFCreateWaveFormatExFromMFMediaType(mediatype, (WAVEFORMATEX **)&format, &size, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(format->Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE, "got wFormatTag %#x\n", format->Format.wFormatTag);
    ok(format->Format.cbSize == aacformat.wfInfo.wfx.cbSize + sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX),
            "got cbSize %u\n", format->Format.cbSize);
    ok(IsEqualGUID(&format->SubFormat, &MFAudioFormat_AAC), "got SubFormat %s\n", debugstr_guid(&format->SubFormat));
    ok(format->dwChannelMask == 63, "got dwChannelMask %#lx\n", format->dwChannelMask);
    ok(format->Samples.wSamplesPerBlock == 4, "got wSamplesPerBlock %u\n", format->Samples.wSamplesPerBlock);
    ok(!memcmp(format + 1, &aacformat.wfInfo.wfx + 1, aacformat.wfInfo.wfx.cbSize), "Unexpected user data.\n");

    /* test initializing media type from an WAVE_FORMAT_EXTENSIBLE AAC format */
    IMFMediaType_DeleteAllItems(mediatype);
    hr = MFInitMediaTypeFromWaveFormatEx(mediatype, (WAVEFORMATEX *)format, sizeof(WAVEFORMATEX) + format->Format.cbSize);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    CoTaskMemFree(format);

    value = 0xdeadbeef;
    hr = IMFMediaType_GetUINT32(mediatype, &MF_MT_AAC_AUDIO_PROFILE_LEVEL_INDICATION, &value);
    todo_wine
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Failed to get attribute, hr %#lx.\n", hr);
    todo_wine
    ok(value == 0xdeadbeef, "Unexpected AAC_AUDIO_PROFILE_LEVEL_INDICATION %u.\n", value);
    value = 0xdeadbeef;
    hr = IMFMediaType_GetUINT32(mediatype, &MF_MT_AAC_PAYLOAD_TYPE, &value);
    todo_wine
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Failed to get attribute, hr %#lx.\n", hr);
    todo_wine
    ok(value == 0xdeadbeef, "Unexpected AAC_PAYLOAD_TYPE %u.\n", value);

    hr = IMFMediaType_GetBlob(mediatype, &MF_MT_USER_DATA, buff, sizeof(buff), &size);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(size == aacformat.wfInfo.wfx.cbSize, "Unexpected size %u.\n", size);
    ok(!memcmp(buff, (WAVEFORMATEX *)&aacformat + 1, size), "Unexpected user data.\n");


    /* test with invalid format size */
    aacformat.wfInfo.wfx.cbSize = 1;
    hr = IMFMediaType_SetBlob(mediatype, &MF_MT_USER_DATA, buff, aacformat.wfInfo.wfx.cbSize);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFCreateWaveFormatExFromMFMediaType(mediatype, (WAVEFORMATEX **)&format, &size, MFWaveFormatExConvertFlag_ForceExtensible);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(format->Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE, "got wFormatTag %#x\n", format->Format.wFormatTag);
    ok(format->Format.cbSize == aacformat.wfInfo.wfx.cbSize + sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX),
            "got cbSize %u\n", format->Format.cbSize);
    ok(IsEqualGUID(&format->SubFormat, &MFAudioFormat_AAC), "got SubFormat %s\n", debugstr_guid(&format->SubFormat));
    ok(format->dwChannelMask == 63, "got dwChannelMask %#lx\n", format->dwChannelMask);
    todo_wine
    ok(format->Samples.wSamplesPerBlock == 4, "got wSamplesPerBlock %u\n", format->Samples.wSamplesPerBlock);
    ok(!memcmp(format + 1, &aacformat.wfInfo.wfx + 1, aacformat.wfInfo.wfx.cbSize), "Unexpected user data.\n");
    CoTaskMemFree(format);

    IMFMediaType_DeleteAllItems(mediatype);


    /* check that HEAACWAVEFORMAT extra fields are copied directly from MF_MT_USER_DATA */
    aacformat.wfInfo.wfx.cbSize = sizeof(aacformat) - sizeof(WAVEFORMATEX);
    hr = MFInitMediaTypeFromWaveFormatEx(mediatype, (WAVEFORMATEX *)&aacformat, sizeof(aacformat));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaType_DeleteItem(mediatype, &MF_MT_USER_DATA);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFCreateWaveFormatExFromMFMediaType(mediatype, (WAVEFORMATEX **)&wfx, &size, 0);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (hr == S_OK)
    {
        ok(wfx->wFormatTag == WAVE_FORMAT_MPEG_HEAAC, "got wFormatTag %#x\n", wfx->wFormatTag);
        ok(wfx->cbSize == 0, "got cbSize %u\n", wfx->cbSize);
        CoTaskMemFree(wfx);
    }

    hr = IMFMediaType_DeleteItem(mediatype, &MF_MT_AAC_AUDIO_PROFILE_LEVEL_INDICATION);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_DeleteItem(mediatype, &MF_MT_AAC_PAYLOAD_TYPE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetBlob(mediatype, &MF_MT_USER_DATA, aac_codec_data, sizeof(aac_codec_data));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateWaveFormatExFromMFMediaType(mediatype, (WAVEFORMATEX **)&wfx, &size, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(wfx->wFormatTag == WAVE_FORMAT_MPEG_HEAAC, "got wFormatTag %#x\n", wfx->wFormatTag);
    ok(wfx->cbSize == sizeof(aac_codec_data), "got cbSize %u\n", wfx->cbSize);
    memcpy(&aacformat, wfx, sizeof(aacformat));
    ok(aacformat.wfInfo.wPayloadType == 0x12, "got %u\n", aacformat.wfInfo.wPayloadType);
    ok(aacformat.wfInfo.wAudioProfileLevelIndication == 0x34, "got %u\n", aacformat.wfInfo.wAudioProfileLevelIndication);

    hr = MFInitMediaTypeFromWaveFormatEx(mediatype, wfx, sizeof(*wfx) + wfx->cbSize);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    value = 0xdeadbeef;
    hr = IMFMediaType_GetUINT32(mediatype, &MF_MT_AAC_PAYLOAD_TYPE, &value);
    ok(hr == S_OK, "Failed to get attribute, hr %#lx.\n", hr);
    ok(value == 0x12, "Unexpected AAC_PAYLOAD_TYPE %u.\n", value);
    value = 0xdeadbeef;
    hr = IMFMediaType_GetUINT32(mediatype, &MF_MT_AAC_AUDIO_PROFILE_LEVEL_INDICATION, &value);
    ok(hr == S_OK, "Failed to get attribute, hr %#lx.\n", hr);
    ok(value == 0x34, "Unexpected AAC_AUDIO_PROFILE_LEVEL_INDICATION %u.\n", value);

    CoTaskMemFree(wfx);


    /* check that RAW AAC doesn't have MF_MT_AAC_* attributes */
    aacformat.wfInfo.wfx.cbSize = sizeof(aacformat) - sizeof(WAVEFORMATEX);
    hr = MFInitMediaTypeFromWaveFormatEx(mediatype, (WAVEFORMATEX *)&aacformat, sizeof(aacformat));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(mediatype, &MF_MT_SUBTYPE, &MFAudioFormat_RAW_AAC);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaType_DeleteItem(mediatype, &MF_MT_AAC_AUDIO_PROFILE_LEVEL_INDICATION);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_DeleteItem(mediatype, &MF_MT_AAC_PAYLOAD_TYPE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetBlob(mediatype, &MF_MT_USER_DATA, aac_codec_data, sizeof(aac_codec_data));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateWaveFormatExFromMFMediaType(mediatype, (WAVEFORMATEX **)&wfx, &size, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(wfx->wFormatTag == WAVE_FORMAT_RAW_AAC1, "got wFormatTag %#x\n", wfx->wFormatTag);
    ok(wfx->cbSize == sizeof(aac_codec_data), "got cbSize %u\n", wfx->cbSize);

    hr = MFInitMediaTypeFromWaveFormatEx(mediatype, wfx, sizeof(*wfx) + wfx->cbSize);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT32(mediatype, &MF_MT_AAC_PAYLOAD_TYPE, &value);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Failed to get attribute, hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT32(mediatype, &MF_MT_AAC_AUDIO_PROFILE_LEVEL_INDICATION, &value);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Failed to get attribute, hr %#lx.\n", hr);

    CoTaskMemFree(wfx);


    IMFMediaType_Release(mediatype);
}

static void test_MFCreateMFVideoFormatFromMFMediaType(void)
{
    MFVIDEOFORMAT *video_format;
    IMFMediaType *media_type;
    UINT32 size, expect_size;
    PALETTEENTRY palette[64];
    BYTE codec_data[32];
    HRESULT hr;


    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);

    expect_size = sizeof(*video_format);
    hr = MFCreateMFVideoFormatFromMFMediaType(media_type, &video_format, &size);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!video_format, "Unexpected format.\n");
    ok(size == expect_size, "Unexpected size %u.\n", size);
    ok(video_format->dwSize == size, "Unexpected size %u.\n", size);
    CoTaskMemFree(video_format);


    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFVideoFormat_RGB32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT64(media_type, &MF_MT_FRAME_SIZE, (UINT64)123 << 32 | 456);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_SAMPLE_SIZE, 123 * 456 * 4);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFCreateMFVideoFormatFromMFMediaType(media_type, &video_format, &size);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(video_format->videoInfo.dwWidth == 123, "got %lu.\n", video_format->videoInfo.dwWidth);
    ok(video_format->videoInfo.dwHeight == 456, "got %lu.\n", video_format->videoInfo.dwHeight);
    ok(video_format->videoInfo.VideoFlags == 0, "got %#I64x.\n", video_format->videoInfo.VideoFlags);
    CoTaskMemFree(video_format);

    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_DEFAULT_STRIDE, 123 * 4);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFCreateMFVideoFormatFromMFMediaType(media_type, &video_format, &size);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(video_format->videoInfo.VideoFlags == 0, "got %#I64x.\n", video_format->videoInfo.VideoFlags);
    CoTaskMemFree(video_format);

    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_DEFAULT_STRIDE, -123 * 4);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFCreateMFVideoFormatFromMFMediaType(media_type, &video_format, &size);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(video_format->videoInfo.VideoFlags == MFVideoFlag_BottomUpLinearRep, "got %#I64x.\n", video_format->videoInfo.VideoFlags);
    CoTaskMemFree(video_format);


    memset(palette, 0xa5, sizeof(palette));
    expect_size = offsetof(MFVIDEOFORMAT, surfaceInfo.Palette[ARRAY_SIZE(palette) + 1]);
    hr = IMFMediaType_SetBlob(media_type, &MF_MT_PALETTE, (BYTE *)palette, sizeof(palette));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFCreateMFVideoFormatFromMFMediaType(media_type, &video_format, &size);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!video_format, "Unexpected format.\n");
    ok(size == expect_size, "Unexpected size %u.\n", size);
    ok(video_format->dwSize == size, "Unexpected size %u.\n", size);
    CoTaskMemFree(video_format);

    memset(codec_data, 0xcd, sizeof(codec_data));
    expect_size += sizeof(codec_data);
    hr = IMFMediaType_SetBlob(media_type, &MF_MT_USER_DATA, codec_data, sizeof(codec_data));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFCreateMFVideoFormatFromMFMediaType(media_type, &video_format, &size);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!video_format, "Unexpected format.\n");
    ok(size == expect_size, "Unexpected size %u.\n", size);
    ok(video_format->dwSize == size, "Unexpected size %u.\n", size);
    CoTaskMemFree(video_format);

    IMFMediaType_Release(media_type);
}

static void test_MFInitAMMediaTypeFromMFMediaType(void)
{
    static const MFVideoArea aperture = {.OffsetX = {.fract = 1, .value = 2}, .OffsetY = {.fract = 3, .value = 4}, .Area={56,78}};
    static const BYTE dummy_mpeg_sequence[] = {0x04,0x05,0x06,0x07,0x08};
    static const BYTE dummy_user_data[] = {0x01,0x02,0x03};

    WAVEFORMATEXTENSIBLE *wave_format_ext;
    VIDEOINFOHEADER *video_info;
    WAVEFORMATEX *wave_format;
    MPEG1VIDEOINFO *mpeg1_info;
    MPEG2VIDEOINFO *mpeg2_info;
    IMFMediaType *media_type, *other_type;
    AM_MEDIA_TYPE am_type;
    MFVideoArea *area;
    UINT32 value32;
    HRESULT hr;

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);

    /* test basic media type attributes mapping and valid formats */

    hr = MFInitAMMediaTypeFromMFMediaType(media_type, GUID_NULL, &am_type);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFInitAMMediaTypeFromMFMediaType(media_type, GUID_NULL, &am_type);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFInitAMMediaTypeFromMFMediaType(media_type, GUID_NULL, &am_type);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFAudioFormat_PCM);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFInitAMMediaTypeFromMFMediaType(media_type, FORMAT_VideoInfo, &am_type);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    hr = MFInitAMMediaTypeFromMFMediaType(media_type, GUID_NULL, &am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&am_type.majortype, &MFMediaType_Audio), "got %s.\n", debugstr_guid(&am_type.majortype));
    ok(IsEqualGUID(&am_type.subtype, &MFAudioFormat_PCM), "got %s.\n", debugstr_guid(&am_type.subtype));
    ok(am_type.bFixedSizeSamples == 0, "got %u\n", am_type.bFixedSizeSamples);
    ok(am_type.bTemporalCompression == 1, "got %u\n", am_type.bTemporalCompression);
    ok(am_type.lSampleSize == 0, "got %lu\n", am_type.lSampleSize);
    ok(IsEqualGUID(&am_type.formattype, &FORMAT_WaveFormatEx), "got %s.\n", debugstr_guid(&am_type.formattype));
    ok(am_type.cbFormat == sizeof(WAVEFORMATEX), "got %lu\n", am_type.cbFormat);
    CoTaskMemFree(am_type.pbFormat);
    hr = MFInitAMMediaTypeFromMFMediaType(media_type, AM_MEDIA_TYPE_REPRESENTATION, &am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&am_type.majortype, &MFMediaType_Audio), "got %s.\n", debugstr_guid(&am_type.majortype));
    ok(IsEqualGUID(&am_type.subtype, &MFAudioFormat_PCM), "got %s.\n", debugstr_guid(&am_type.subtype));
    ok(IsEqualGUID(&am_type.formattype, &GUID_NULL), "got %s.\n", debugstr_guid(&am_type.formattype));
    ok(am_type.cbFormat == 0, "got %lu\n", am_type.cbFormat);
    CoTaskMemFree(am_type.pbFormat);
    IMFMediaType_DeleteAllItems(media_type);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFVideoFormat_RGB32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFInitAMMediaTypeFromMFMediaType(media_type, FORMAT_WaveFormatEx, &am_type);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    hr = MFInitAMMediaTypeFromMFMediaType(media_type, GUID_NULL, &am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&am_type.majortype, &MFMediaType_Video), "got %s.\n", debugstr_guid(&am_type.majortype));
    ok(IsEqualGUID(&am_type.subtype, &MEDIASUBTYPE_RGB32), "got %s.\n", debugstr_guid(&am_type.subtype));
    ok(IsEqualGUID(&am_type.formattype, &FORMAT_VideoInfo), "got %s.\n", debugstr_guid(&am_type.formattype));
    ok(am_type.cbFormat == sizeof(VIDEOINFOHEADER), "got %lu\n", am_type.cbFormat);
    CoTaskMemFree(am_type.pbFormat);
    hr = MFInitAMMediaTypeFromMFMediaType(media_type, AM_MEDIA_TYPE_REPRESENTATION, &am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&am_type.majortype, &MFMediaType_Video), "got %s.\n", debugstr_guid(&am_type.majortype));
    ok(IsEqualGUID(&am_type.subtype, &MFVideoFormat_RGB32), "got %s.\n", debugstr_guid(&am_type.subtype));
    ok(IsEqualGUID(&am_type.formattype, &GUID_NULL), "got %s.\n", debugstr_guid(&am_type.formattype));
    ok(am_type.cbFormat == 0, "got %lu\n", am_type.cbFormat);
    CoTaskMemFree(am_type.pbFormat);
    hr = MFInitAMMediaTypeFromMFMediaType(media_type, FORMAT_VideoInfo, &am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&am_type.majortype, &MFMediaType_Video), "got %s.\n", debugstr_guid(&am_type.majortype));
    ok(IsEqualGUID(&am_type.subtype, &MEDIASUBTYPE_RGB32), "got %s.\n", debugstr_guid(&am_type.subtype));
    ok(am_type.bFixedSizeSamples == 1, "got %u\n", am_type.bFixedSizeSamples);
    ok(am_type.bTemporalCompression == 0, "got %u\n", am_type.bTemporalCompression);
    ok(am_type.lSampleSize == 0, "got %lu\n", am_type.lSampleSize);
    ok(IsEqualGUID(&am_type.formattype, &FORMAT_VideoInfo), "got %s.\n", debugstr_guid(&am_type.formattype));
    ok(am_type.cbFormat == sizeof(VIDEOINFOHEADER), "got %lu\n", am_type.cbFormat);
    CoTaskMemFree(am_type.pbFormat);
    hr = MFInitAMMediaTypeFromMFMediaType(media_type, FORMAT_VideoInfo2, &am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&am_type.majortype, &MFMediaType_Video), "got %s.\n", debugstr_guid(&am_type.majortype));
    ok(IsEqualGUID(&am_type.subtype, &MEDIASUBTYPE_RGB32), "got %s.\n", debugstr_guid(&am_type.subtype));
    ok(am_type.bFixedSizeSamples == 1, "got %u\n", am_type.bFixedSizeSamples);
    ok(am_type.bTemporalCompression == 0, "got %u\n", am_type.bTemporalCompression);
    ok(am_type.lSampleSize == 0, "got %lu\n", am_type.lSampleSize);
    ok(IsEqualGUID(&am_type.formattype, &FORMAT_VideoInfo2), "got %s.\n", debugstr_guid(&am_type.formattype));
    ok(am_type.cbFormat == sizeof(VIDEOINFOHEADER2), "got %lu\n", am_type.cbFormat);
    CoTaskMemFree(am_type.pbFormat);
    hr = MFInitAMMediaTypeFromMFMediaType(media_type, FORMAT_MFVideoFormat, &am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&am_type.majortype, &MFMediaType_Video), "got %s.\n", debugstr_guid(&am_type.majortype));
    ok(IsEqualGUID(&am_type.subtype, &MFVideoFormat_RGB32), "got %s.\n", debugstr_guid(&am_type.subtype));
    ok(am_type.bFixedSizeSamples == 0, "got %u\n", am_type.bFixedSizeSamples);
    ok(am_type.bTemporalCompression == 1, "got %u\n", am_type.bTemporalCompression);
    ok(am_type.lSampleSize == 0, "got %lu\n", am_type.lSampleSize);
    ok(IsEqualGUID(&am_type.formattype, &FORMAT_MFVideoFormat), "got %s.\n", debugstr_guid(&am_type.formattype));
    ok(am_type.cbFormat == sizeof(MFVIDEOFORMAT), "got %lu\n", am_type.cbFormat);
    CoTaskMemFree(am_type.pbFormat);
    hr = MFInitAMMediaTypeFromMFMediaType(media_type, FORMAT_MPEGVideo, &am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&am_type.majortype, &MFMediaType_Video), "got %s.\n", debugstr_guid(&am_type.majortype));
    ok(IsEqualGUID(&am_type.subtype, &MEDIASUBTYPE_RGB32), "got %s.\n", debugstr_guid(&am_type.subtype));
    ok(am_type.bFixedSizeSamples == 1, "got %u\n", am_type.bFixedSizeSamples);
    ok(am_type.bTemporalCompression == 0, "got %u\n", am_type.bTemporalCompression);
    ok(am_type.lSampleSize == 0, "got %lu\n", am_type.lSampleSize);
    ok(IsEqualGUID(&am_type.formattype, &FORMAT_MPEGVideo), "got %s.\n", debugstr_guid(&am_type.formattype));
    ok(am_type.cbFormat == sizeof(MPEG1VIDEOINFO), "got %lu\n", am_type.cbFormat);
    CoTaskMemFree(am_type.pbFormat);
    hr = MFInitAMMediaTypeFromMFMediaType(media_type, FORMAT_MPEG2Video, &am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&am_type.majortype, &MFMediaType_Video), "got %s.\n", debugstr_guid(&am_type.majortype));
    ok(IsEqualGUID(&am_type.subtype, &MEDIASUBTYPE_RGB32), "got %s.\n", debugstr_guid(&am_type.subtype));
    ok(IsEqualGUID(&am_type.formattype, &FORMAT_MPEG2Video), "got %s.\n", debugstr_guid(&am_type.formattype));
    ok(am_type.cbFormat == sizeof(MPEG2VIDEOINFO), "got %lu\n", am_type.cbFormat);
    CoTaskMemFree(am_type.pbFormat);


    /* test WAVEFORMATEX mapping */

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFAudioFormat_PCM);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetBlob(media_type, &MF_MT_USER_DATA, dummy_user_data, sizeof(dummy_user_data));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFInitAMMediaTypeFromMFMediaType(media_type, GUID_NULL, &am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(am_type.cbFormat == sizeof(WAVEFORMATEX) + sizeof(dummy_user_data), "got %lu\n", am_type.cbFormat);
    wave_format = (WAVEFORMATEX *)am_type.pbFormat;
    ok(wave_format->wFormatTag == WAVE_FORMAT_PCM, "got %u\n", wave_format->wFormatTag);
    ok(wave_format->nChannels == 0, "got %u\n", wave_format->nChannels);
    ok(wave_format->nSamplesPerSec == 0, "got %lu\n", wave_format->nSamplesPerSec);
    ok(wave_format->nAvgBytesPerSec == 0, "got %lu\n", wave_format->nAvgBytesPerSec);
    ok(wave_format->nBlockAlign == 0, "got %u\n", wave_format->nBlockAlign);
    ok(wave_format->wBitsPerSample == 0, "got %u\n", wave_format->wBitsPerSample);
    ok(wave_format->cbSize == sizeof(dummy_user_data), "got %u\n", wave_format->cbSize);
    ok(!memcmp(wave_format + 1, dummy_user_data, sizeof(dummy_user_data)), "got unexpected data\n");
    CoTaskMemFree(am_type.pbFormat);
    IMFMediaType_DeleteAllItems(media_type);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFAudioFormat_PCM);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_AUDIO_NUM_CHANNELS, 2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFInitAMMediaTypeFromMFMediaType(media_type, GUID_NULL, &am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(am_type.cbFormat == sizeof(WAVEFORMATEX), "got %lu\n", am_type.cbFormat);
    wave_format = (WAVEFORMATEX *)am_type.pbFormat;
    ok(wave_format->wFormatTag == WAVE_FORMAT_PCM, "got %u\n", wave_format->wFormatTag);
    ok(wave_format->nChannels == 2, "got %u\n", wave_format->nChannels);
    ok(wave_format->cbSize == 0, "got %u\n", wave_format->cbSize);
    CoTaskMemFree(am_type.pbFormat);
    IMFMediaType_DeleteAllItems(media_type);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFAudioFormat_PCM);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_AUDIO_NUM_CHANNELS, 2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_AUDIO_BITS_PER_SAMPLE, 16);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFInitAMMediaTypeFromMFMediaType(media_type, GUID_NULL, &am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(am_type.cbFormat == sizeof(WAVEFORMATEX), "got %lu\n", am_type.cbFormat);
    wave_format = (WAVEFORMATEX *)am_type.pbFormat;
    ok(wave_format->wFormatTag == WAVE_FORMAT_PCM, "got %u\n", wave_format->wFormatTag);
    ok(wave_format->wBitsPerSample == 16, "got %u\n", wave_format->wBitsPerSample);
    ok(wave_format->nBlockAlign == 0, "got %u\n", wave_format->nBlockAlign);
    ok(wave_format->cbSize == 0, "got %u\n", wave_format->cbSize);
    CoTaskMemFree(am_type.pbFormat);
    IMFMediaType_DeleteAllItems(media_type);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFAudioFormat_PCM);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_AUDIO_BLOCK_ALIGNMENT, 16);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFInitAMMediaTypeFromMFMediaType(media_type, GUID_NULL, &am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(am_type.cbFormat == sizeof(WAVEFORMATEX), "got %lu\n", am_type.cbFormat);
    wave_format = (WAVEFORMATEX *)am_type.pbFormat;
    ok(wave_format->wFormatTag == WAVE_FORMAT_PCM, "got %u\n", wave_format->wFormatTag);
    ok(wave_format->nBlockAlign == 16, "got %u\n", wave_format->nBlockAlign);
    ok(wave_format->cbSize == 0, "got %u\n", wave_format->cbSize);
    CoTaskMemFree(am_type.pbFormat);
    IMFMediaType_DeleteAllItems(media_type);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFAudioFormat_PCM);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_AUDIO_SAMPLES_PER_SECOND, 44100);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFInitAMMediaTypeFromMFMediaType(media_type, GUID_NULL, &am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(am_type.cbFormat == sizeof(WAVEFORMATEX), "got %lu\n", am_type.cbFormat);
    wave_format = (WAVEFORMATEX *)am_type.pbFormat;
    ok(wave_format->wFormatTag == WAVE_FORMAT_PCM, "got %u\n", wave_format->wFormatTag);
    ok(wave_format->nSamplesPerSec == 44100, "got %lu\n", wave_format->nSamplesPerSec);
    ok(wave_format->cbSize == 0, "got %u\n", wave_format->cbSize);
    CoTaskMemFree(am_type.pbFormat);
    IMFMediaType_DeleteAllItems(media_type);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFAudioFormat_PCM);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 176400);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFInitAMMediaTypeFromMFMediaType(media_type, GUID_NULL, &am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(am_type.cbFormat == sizeof(WAVEFORMATEX), "got %lu\n", am_type.cbFormat);
    wave_format = (WAVEFORMATEX *)am_type.pbFormat;
    ok(wave_format->wFormatTag == WAVE_FORMAT_PCM, "got %u\n", wave_format->wFormatTag);
    ok(wave_format->nAvgBytesPerSec == 176400, "got %lu\n", wave_format->nAvgBytesPerSec);
    ok(wave_format->cbSize == 0, "got %u\n", wave_format->cbSize);
    CoTaskMemFree(am_type.pbFormat);
    IMFMediaType_DeleteAllItems(media_type);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFAudioFormat_PCM);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_AUDIO_NUM_CHANNELS, 3);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFInitAMMediaTypeFromMFMediaType(media_type, GUID_NULL, &am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(am_type.cbFormat == sizeof(WAVEFORMATEXTENSIBLE), "got %lu\n", am_type.cbFormat);
    wave_format_ext = (WAVEFORMATEXTENSIBLE *)am_type.pbFormat;
    ok(wave_format_ext->Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE, "got %u\n", wave_format_ext->Format.wFormatTag);
    ok(wave_format_ext->Format.nChannels == 3, "got %u\n", wave_format_ext->Format.nChannels);
    ok(IsEqualGUID(&wave_format_ext->SubFormat, &MFAudioFormat_PCM),
            "got %s\n", debugstr_guid(&wave_format_ext->SubFormat));
    ok(wave_format_ext->dwChannelMask == 7, "got %#lx\n", wave_format_ext->dwChannelMask);
    ok(wave_format_ext->Samples.wSamplesPerBlock == 0, "got %u\n",
            wave_format_ext->Samples.wSamplesPerBlock);
    ok(wave_format_ext->Format.cbSize == sizeof(*wave_format_ext) - sizeof(*wave_format),
            "got %u\n", wave_format_ext->Format.cbSize);
    CoTaskMemFree(am_type.pbFormat);
    IMFMediaType_DeleteAllItems(media_type);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFAudioFormat_PCM);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_AUDIO_CHANNEL_MASK, 3);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFInitAMMediaTypeFromMFMediaType(media_type, GUID_NULL, &am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(am_type.cbFormat == sizeof(WAVEFORMATEXTENSIBLE), "got %lu\n", am_type.cbFormat);
    wave_format_ext = (WAVEFORMATEXTENSIBLE *)am_type.pbFormat;
    ok(wave_format_ext->Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE,
            "got %u\n", wave_format_ext->Format.wFormatTag);
    ok(IsEqualGUID(&wave_format_ext->SubFormat, &MFAudioFormat_PCM),
            "got %s\n", debugstr_guid(&wave_format_ext->SubFormat));
    ok(wave_format_ext->dwChannelMask == 3, "got %#lx\n", wave_format_ext->dwChannelMask);
    ok(wave_format_ext->Samples.wSamplesPerBlock == 0, "got %u\n",
            wave_format_ext->Samples.wSamplesPerBlock);
    ok(wave_format_ext->Format.cbSize == sizeof(*wave_format_ext) - sizeof(*wave_format),
            "got %u\n", wave_format_ext->Format.cbSize);
    CoTaskMemFree(am_type.pbFormat);
    IMFMediaType_DeleteAllItems(media_type);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFAudioFormat_PCM);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_AUDIO_VALID_BITS_PER_SAMPLE, 3);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_AUDIO_SAMPLES_PER_BLOCK, 4);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFInitAMMediaTypeFromMFMediaType(media_type, GUID_NULL, &am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(am_type.cbFormat == sizeof(WAVEFORMATEXTENSIBLE), "got %lu\n", am_type.cbFormat);
    wave_format_ext = (WAVEFORMATEXTENSIBLE *)am_type.pbFormat;
    ok(wave_format_ext->Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE,
            "got %u\n", wave_format_ext->Format.wFormatTag);
    ok(IsEqualGUID(&wave_format_ext->SubFormat, &MFAudioFormat_PCM),
            "got %s\n", debugstr_guid(&wave_format_ext->SubFormat));
    ok(wave_format_ext->dwChannelMask == 0, "got %#lx\n", wave_format_ext->dwChannelMask);
    ok(wave_format_ext->Samples.wSamplesPerBlock == 4, "got %u\n",
            wave_format_ext->Samples.wSamplesPerBlock);
    ok(wave_format_ext->Format.cbSize == sizeof(*wave_format_ext) - sizeof(*wave_format),
            "got %u\n", wave_format_ext->Format.cbSize);
    CoTaskMemFree(am_type.pbFormat);
    IMFMediaType_DeleteAllItems(media_type);


    /* test VIDEOINFOHEADER mapping */

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFVideoFormat_RGB32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetBlob(media_type, &MF_MT_USER_DATA, dummy_user_data, sizeof(dummy_user_data));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFInitAMMediaTypeFromMFMediaType(media_type, GUID_NULL, &am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(am_type.cbFormat == sizeof(VIDEOINFOHEADER) + sizeof(dummy_user_data), "got %lu\n", am_type.cbFormat);
    video_info = (VIDEOINFOHEADER *)am_type.pbFormat;
    ok(video_info->rcSource.right == 0, "got %lu\n", video_info->rcSource.right);
    ok(video_info->rcSource.bottom == 0, "got %lu\n", video_info->rcSource.bottom);
    ok(video_info->rcTarget.right == 0, "got %lu\n", video_info->rcTarget.right);
    ok(video_info->rcTarget.bottom == 0, "got %lu\n", video_info->rcTarget.bottom);
    ok(video_info->dwBitRate == 0, "got %lu\n", video_info->dwBitRate);
    ok(video_info->dwBitErrorRate == 0, "got %lu\n", video_info->dwBitErrorRate);
    ok(video_info->AvgTimePerFrame == 0, "got %I64d\n", video_info->AvgTimePerFrame);
    ok(video_info->bmiHeader.biSize == sizeof(BITMAPINFOHEADER) + sizeof(dummy_user_data),
            "got %lu\n", video_info->bmiHeader.biSize);
    ok(video_info->bmiHeader.biWidth == 0, "got %lu\n", video_info->bmiHeader.biWidth);
    ok(video_info->bmiHeader.biHeight == 0, "got %lu\n", video_info->bmiHeader.biHeight);
    ok(video_info->bmiHeader.biPlanes == 1, "got %u\n", video_info->bmiHeader.biPlanes);
    ok(video_info->bmiHeader.biBitCount == 32, "got %u\n", video_info->bmiHeader.biBitCount);
    ok(video_info->bmiHeader.biCompression == BI_RGB, "got %lu\n", video_info->bmiHeader.biCompression);
    ok(video_info->bmiHeader.biSizeImage == 0, "got %lu\n", video_info->bmiHeader.biSizeImage);
    ok(!memcmp(video_info + 1, dummy_user_data, sizeof(dummy_user_data)),
            "got unexpected data\n");
    CoTaskMemFree(am_type.pbFormat);
    IMFMediaType_DeleteAllItems(media_type);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFAudioFormat_PCM);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_FIXED_SIZE_SAMPLES, 1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFInitAMMediaTypeFromMFMediaType(media_type, GUID_NULL, &am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(am_type.bFixedSizeSamples == 1, "got %u.\n", am_type.bFixedSizeSamples);
    CoTaskMemFree(am_type.pbFormat);
    IMFMediaType_DeleteAllItems(media_type);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFAudioFormat_PCM);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_SAMPLE_SIZE, 123456);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFInitAMMediaTypeFromMFMediaType(media_type, GUID_NULL, &am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(am_type.lSampleSize == 123456, "got %lu.\n", am_type.lSampleSize);
    CoTaskMemFree(am_type.pbFormat);
    IMFMediaType_DeleteAllItems(media_type);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFVideoFormat_RGB32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFInitAMMediaTypeFromMFMediaType(media_type, GUID_NULL, &am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    video_info = (VIDEOINFOHEADER *)am_type.pbFormat;
    ok(video_info->bmiHeader.biSize == sizeof(BITMAPINFOHEADER), "got %lu\n", video_info->bmiHeader.biSize);
    ok(video_info->bmiHeader.biBitCount == 32, "got %u\n", video_info->bmiHeader.biBitCount);
    ok(video_info->bmiHeader.biCompression == BI_RGB, "got %lu\n", video_info->bmiHeader.biCompression);
    CoTaskMemFree(am_type.pbFormat);
    IMFMediaType_DeleteAllItems(media_type);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFVideoFormat_RGB565);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFInitAMMediaTypeFromMFMediaType(media_type, GUID_NULL, &am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    video_info = (VIDEOINFOHEADER *)am_type.pbFormat;
    ok(video_info->bmiHeader.biSize == sizeof(BITMAPINFOHEADER), "got %lu\n", video_info->bmiHeader.biSize);
    ok(video_info->bmiHeader.biBitCount == 16, "got %u\n", video_info->bmiHeader.biBitCount);
    ok(video_info->bmiHeader.biCompression == BI_BITFIELDS, "got %lu\n", video_info->bmiHeader.biCompression);
    CoTaskMemFree(am_type.pbFormat);
    IMFMediaType_DeleteAllItems(media_type);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFVideoFormat_NV12);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFInitAMMediaTypeFromMFMediaType(media_type, GUID_NULL, &am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    video_info = (VIDEOINFOHEADER *)am_type.pbFormat;
    ok(video_info->bmiHeader.biSize == sizeof(BITMAPINFOHEADER), "got %lu\n", video_info->bmiHeader.biSize);
    ok(video_info->bmiHeader.biBitCount == 12, "got %u\n", video_info->bmiHeader.biBitCount);
    ok(video_info->bmiHeader.biCompression == MFVideoFormat_NV12.Data1, "got %lu\n", video_info->bmiHeader.biCompression);
    CoTaskMemFree(am_type.pbFormat);
    IMFMediaType_DeleteAllItems(media_type);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFVideoFormat_WMV2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFInitAMMediaTypeFromMFMediaType(media_type, GUID_NULL, &am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    video_info = (VIDEOINFOHEADER *)am_type.pbFormat;
    ok(video_info->bmiHeader.biSize == sizeof(BITMAPINFOHEADER), "got %lu\n", video_info->bmiHeader.biSize);
    ok(video_info->bmiHeader.biBitCount == 0, "got %u\n", video_info->bmiHeader.biBitCount);
    ok(video_info->bmiHeader.biCompression == MFVideoFormat_WMV2.Data1, "got %lu\n", video_info->bmiHeader.biCompression);
    CoTaskMemFree(am_type.pbFormat);
    IMFMediaType_DeleteAllItems(media_type);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFVideoFormat_NV12);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT64(media_type, &MF_MT_FRAME_RATE, (UINT64)123 << 32 | 456);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFInitAMMediaTypeFromMFMediaType(media_type, GUID_NULL, &am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    video_info = (VIDEOINFOHEADER *)am_type.pbFormat;
    ok(video_info->AvgTimePerFrame == 37073171, "got %I64d\n", video_info->AvgTimePerFrame);
    CoTaskMemFree(am_type.pbFormat);
    IMFMediaType_DeleteAllItems(media_type);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFVideoFormat_NV12);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_AVG_BITRATE, 123456);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_AVG_BIT_ERROR_RATE, 654321);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFInitAMMediaTypeFromMFMediaType(media_type, GUID_NULL, &am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    video_info = (VIDEOINFOHEADER *)am_type.pbFormat;
    ok(video_info->dwBitRate == 123456, "got %lu\n", video_info->dwBitRate);
    ok(video_info->dwBitErrorRate == 654321, "got %lu\n", video_info->dwBitErrorRate);
    CoTaskMemFree(am_type.pbFormat);
    IMFMediaType_DeleteAllItems(media_type);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFVideoFormat_NV12);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_SAMPLE_SIZE, 123456);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFInitAMMediaTypeFromMFMediaType(media_type, GUID_NULL, &am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    video_info = (VIDEOINFOHEADER *)am_type.pbFormat;
    ok(video_info->bmiHeader.biSizeImage == 123456, "got %lu\n", video_info->bmiHeader.biSizeImage);
    CoTaskMemFree(am_type.pbFormat);
    IMFMediaType_DeleteAllItems(media_type);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFVideoFormat_RGB32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT64(media_type, &MF_MT_FRAME_SIZE, (UINT64)123 << 32 | 456);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFInitAMMediaTypeFromMFMediaType(media_type, GUID_NULL, &am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    video_info = (VIDEOINFOHEADER *)am_type.pbFormat;
    ok(video_info->bmiHeader.biWidth == 123, "got %lu\n", video_info->bmiHeader.biWidth);
    ok(video_info->bmiHeader.biHeight == 456, "got %lu\n", video_info->bmiHeader.biHeight);
    ok(video_info->bmiHeader.biSizeImage == 123 * 456 * 4, "got %lu\n", video_info->bmiHeader.biSizeImage);
    CoTaskMemFree(am_type.pbFormat);
    IMFMediaType_DeleteAllItems(media_type);

    /* MF_MT_MINIMUM_DISPLAY_APERTURE has no effect */
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFVideoFormat_RGB32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT64(media_type, &MF_MT_FRAME_SIZE, (UINT64)123 << 32 | 456);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetBlob(media_type, &MF_MT_MINIMUM_DISPLAY_APERTURE, (BYTE *)&aperture, sizeof(aperture));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_SAMPLE_SIZE, 12345678);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFInitAMMediaTypeFromMFMediaType(media_type, GUID_NULL, &am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    video_info = (VIDEOINFOHEADER *)am_type.pbFormat;
    ok(video_info->rcSource.left == 0, "got %lu\n", video_info->rcSource.left);
    ok(video_info->rcSource.right == 0, "got %lu\n", video_info->rcSource.right);
    ok(video_info->rcSource.top == 0, "got %lu\n", video_info->rcSource.top);
    ok(video_info->rcSource.bottom == 0, "got %lu\n", video_info->rcSource.bottom);
    ok(video_info->rcTarget.left == 0, "got %lu\n", video_info->rcTarget.left);
    ok(video_info->rcTarget.right == 0, "got %lu\n", video_info->rcTarget.right);
    ok(video_info->rcTarget.top == 0, "got %lu\n", video_info->rcTarget.top);
    ok(video_info->rcTarget.bottom == 0, "got %lu\n", video_info->rcTarget.bottom);
    ok(video_info->bmiHeader.biWidth == 123, "got %lu\n", video_info->bmiHeader.biWidth);
    ok(video_info->bmiHeader.biHeight == 456, "got %lu\n", video_info->bmiHeader.biHeight);
    ok(video_info->bmiHeader.biSizeImage == 123 * 456 * 4, "got %lu\n", video_info->bmiHeader.biSizeImage);
    CoTaskMemFree(am_type.pbFormat);
    IMFMediaType_DeleteAllItems(media_type);

    /* MF_MT_DEFAULT_STRIDE / MF_MT_FRAME_SIZE mismatch is translated into rcSource / rcTarget */
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFVideoFormat_RGB32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT64(media_type, &MF_MT_FRAME_SIZE, (UINT64)123 << 32 | 456);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_DEFAULT_STRIDE, -246 * 4);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_SAMPLE_SIZE, 12345678);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetBlob(media_type, &MF_MT_MINIMUM_DISPLAY_APERTURE, (BYTE *)&aperture, sizeof(aperture));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetBlob(media_type, &MF_MT_GEOMETRIC_APERTURE, (BYTE *)&aperture, sizeof(aperture));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetBlob(media_type, &MF_MT_PAN_SCAN_APERTURE, (BYTE *)&aperture, sizeof(aperture));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_PAN_SCAN_ENABLED, 1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFInitAMMediaTypeFromMFMediaType(media_type, GUID_NULL, &am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    video_info = (VIDEOINFOHEADER *)am_type.pbFormat;
    ok(video_info->rcSource.left == 0, "got %lu\n", video_info->rcSource.left);
    ok(video_info->rcSource.right == 123, "got %lu\n", video_info->rcSource.right);
    ok(video_info->rcSource.top == 0, "got %lu\n", video_info->rcSource.top);
    ok(video_info->rcSource.bottom == 456, "got %lu\n", video_info->rcSource.bottom);
    ok(video_info->rcTarget.left == 0, "got %lu\n", video_info->rcTarget.left);
    ok(video_info->rcTarget.right == 123, "got %lu\n", video_info->rcTarget.right);
    ok(video_info->rcTarget.top == 0, "got %lu\n", video_info->rcTarget.top);
    ok(video_info->rcTarget.bottom == 456, "got %lu\n", video_info->rcTarget.bottom);
    ok(video_info->bmiHeader.biWidth == 246, "got %lu\n", video_info->bmiHeader.biWidth);
    ok(video_info->bmiHeader.biHeight == 456, "got %ld\n", video_info->bmiHeader.biHeight);
    ok(video_info->bmiHeader.biSizeImage == 246 * 456 * 4, "got %lu\n", video_info->bmiHeader.biSizeImage);
    CoTaskMemFree(am_type.pbFormat);

    /* positive stride only changes biHeight */
    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_DEFAULT_STRIDE, 246 * 4);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFInitAMMediaTypeFromMFMediaType(media_type, GUID_NULL, &am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    video_info = (VIDEOINFOHEADER *)am_type.pbFormat;
    ok(video_info->rcSource.left == 0, "got %lu\n", video_info->rcSource.left);
    ok(video_info->rcSource.right == 123, "got %lu\n", video_info->rcSource.right);
    ok(video_info->rcSource.top == 0, "got %lu\n", video_info->rcSource.top);
    ok(video_info->rcSource.bottom == 456, "got %lu\n", video_info->rcSource.bottom);
    ok(video_info->rcTarget.left == 0, "got %lu\n", video_info->rcTarget.left);
    ok(video_info->rcTarget.right == 123, "got %lu\n", video_info->rcTarget.right);
    ok(video_info->rcTarget.top == 0, "got %lu\n", video_info->rcTarget.top);
    ok(video_info->rcTarget.bottom == 456, "got %lu\n", video_info->rcTarget.bottom);
    ok(video_info->bmiHeader.biWidth == 246, "got %lu\n", video_info->bmiHeader.biWidth);
    ok(video_info->bmiHeader.biHeight == -456, "got %ld\n", video_info->bmiHeader.biHeight);
    ok(video_info->bmiHeader.biSizeImage == 246 * 456 * 4, "got %lu\n", video_info->bmiHeader.biSizeImage);
    CoTaskMemFree(am_type.pbFormat);
    IMFMediaType_DeleteAllItems(media_type);

    /* same thing happens with other formats */
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFVideoFormat_NV12);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT64(media_type, &MF_MT_FRAME_SIZE, (UINT64)123 << 32 | 456);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_DEFAULT_STRIDE, 246);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_SAMPLE_SIZE, 12345678);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFInitAMMediaTypeFromMFMediaType(media_type, GUID_NULL, &am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    video_info = (VIDEOINFOHEADER *)am_type.pbFormat;
    ok(video_info->rcSource.left == 0, "got %lu\n", video_info->rcSource.left);
    ok(video_info->rcSource.right == 123, "got %lu\n", video_info->rcSource.right);
    ok(video_info->rcSource.top == 0, "got %lu\n", video_info->rcSource.top);
    ok(video_info->rcSource.bottom == 456, "got %lu\n", video_info->rcSource.bottom);
    ok(video_info->rcTarget.left == 0, "got %lu\n", video_info->rcTarget.left);
    ok(video_info->rcTarget.right == 123, "got %lu\n", video_info->rcTarget.right);
    ok(video_info->rcTarget.top == 0, "got %lu\n", video_info->rcTarget.top);
    ok(video_info->rcTarget.bottom == 456, "got %lu\n", video_info->rcTarget.bottom);
    ok(video_info->bmiHeader.biWidth == 246, "got %lu\n", video_info->bmiHeader.biWidth);
    ok(video_info->bmiHeader.biHeight == 456, "got %ld\n", video_info->bmiHeader.biHeight);
    ok(video_info->bmiHeader.biSizeImage == 246 * 456 * 3 / 2, "got %lu\n", video_info->bmiHeader.biSizeImage);
    CoTaskMemFree(am_type.pbFormat);
    IMFMediaType_DeleteAllItems(media_type);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFVideoFormat_RGB32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT64(media_type, &MF_MT_FRAME_SIZE, (UINT64)123 << 32 | 456);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_DEFAULT_STRIDE, -246 * 4);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFInitAMMediaTypeFromMFMediaType(media_type, GUID_NULL, &am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    video_info = (VIDEOINFOHEADER *)am_type.pbFormat;
    ok(video_info->bmiHeader.biWidth == 246, "got %lu\n", video_info->bmiHeader.biWidth);
    ok(video_info->bmiHeader.biHeight == 456, "got %ld\n", video_info->bmiHeader.biHeight);
    ok(video_info->bmiHeader.biSizeImage == 246 * 456 * 4, "got %lu\n", video_info->bmiHeader.biSizeImage);
    CoTaskMemFree(am_type.pbFormat);
    IMFMediaType_DeleteAllItems(media_type);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFVideoFormat_RGB32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT64(media_type, &MF_MT_FRAME_SIZE, (UINT64)123 << 32 | 456);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_DEFAULT_STRIDE, 246 * 4);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFInitAMMediaTypeFromMFMediaType(media_type, GUID_NULL, &am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    video_info = (VIDEOINFOHEADER *)am_type.pbFormat;
    ok(video_info->bmiHeader.biWidth == 246, "got %lu\n", video_info->bmiHeader.biWidth);
    ok(video_info->bmiHeader.biHeight == -456, "got %ld\n", video_info->bmiHeader.biHeight);
    ok(video_info->bmiHeader.biSizeImage == 246 * 456 * 4, "got %lu\n", video_info->bmiHeader.biSizeImage);
    CoTaskMemFree(am_type.pbFormat);
    IMFMediaType_DeleteAllItems(media_type);

    /* aperture is lost with VIDEOINFOHEADER(2), preserved with MFVIDEOFORMAT */
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFVideoFormat_NV12);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT64(media_type, &MF_MT_FRAME_SIZE, (UINT64)1920 << 32 | 1088);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetBlob(media_type, &MF_MT_MINIMUM_DISPLAY_APERTURE, (BYTE *)&aperture, sizeof(aperture));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateMediaType(&other_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = MFInitAMMediaTypeFromMFMediaType(media_type, FORMAT_VideoInfo, &am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFInitMediaTypeFromAMMediaType(other_type, &am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetAllocatedBlob(other_type, &MF_MT_MINIMUM_DISPLAY_APERTURE, (BYTE **)&area, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_DeleteAllItems(other_type);
    CoTaskMemFree(am_type.pbFormat);

    hr = MFInitAMMediaTypeFromMFMediaType(media_type, FORMAT_VideoInfo2, &am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFInitMediaTypeFromAMMediaType(other_type, &am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetAllocatedBlob(other_type, &MF_MT_MINIMUM_DISPLAY_APERTURE, (BYTE **)&area, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_DeleteAllItems(other_type);
    CoTaskMemFree(am_type.pbFormat);

    hr = MFInitAMMediaTypeFromMFMediaType(media_type, FORMAT_MFVideoFormat, &am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFInitMediaTypeFromAMMediaType(other_type, &am_type);
    todo_wine ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetAllocatedBlob(other_type, &MF_MT_MINIMUM_DISPLAY_APERTURE, (BYTE **)&area, &value32);
    todo_wine ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (hr == S_OK) CoTaskMemFree(area);
    IMFMediaType_DeleteAllItems(other_type);
    CoTaskMemFree(am_type.pbFormat);

    IMFMediaType_Release(other_type);
    IMFMediaType_DeleteAllItems(media_type);


    /* MEDIASUBTYPE_MPEG1Packet and MEDIASUBTYPE_MPEG1Payload use FORMAT_MPEGVideo by default */

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MEDIASUBTYPE_MPEG1Packet);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFInitAMMediaTypeFromMFMediaType(media_type, GUID_NULL, &am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&am_type.majortype, &MFMediaType_Video), "got %s.\n", debugstr_guid(&am_type.majortype));
    ok(IsEqualGUID(&am_type.subtype, &MEDIASUBTYPE_MPEG1Packet), "got %s.\n", debugstr_guid(&am_type.subtype));
    ok(am_type.bFixedSizeSamples == 0, "got %u\n", am_type.bFixedSizeSamples);
    ok(am_type.bTemporalCompression == 1, "got %u\n", am_type.bTemporalCompression);
    ok(am_type.lSampleSize == 0, "got %lu\n", am_type.lSampleSize);
    ok(IsEqualGUID(&am_type.formattype, &FORMAT_MPEGVideo), "got %s.\n", debugstr_guid(&am_type.formattype));
    ok(am_type.cbFormat == sizeof(MPEG1VIDEOINFO), "got %lu\n", am_type.cbFormat);
    CoTaskMemFree(am_type.pbFormat);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MEDIASUBTYPE_MPEG1Payload);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFInitAMMediaTypeFromMFMediaType(media_type, GUID_NULL, &am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&am_type.majortype, &MFMediaType_Video), "got %s.\n", debugstr_guid(&am_type.majortype));
    ok(IsEqualGUID(&am_type.subtype, &MEDIASUBTYPE_MPEG1Payload), "got %s.\n", debugstr_guid(&am_type.subtype));
    ok(am_type.bFixedSizeSamples == 0, "got %u\n", am_type.bFixedSizeSamples);
    ok(am_type.bTemporalCompression == 1, "got %u\n", am_type.bTemporalCompression);
    ok(am_type.lSampleSize == 0, "got %lu\n", am_type.lSampleSize);
    ok(IsEqualGUID(&am_type.formattype, &FORMAT_MPEGVideo), "got %s.\n", debugstr_guid(&am_type.formattype));
    ok(am_type.cbFormat == sizeof(MPEG1VIDEOINFO), "got %lu\n", am_type.cbFormat);
    ok(am_type.cbFormat == sizeof(MPEG1VIDEOINFO), "got %lu\n", am_type.cbFormat);
    mpeg1_info = (MPEG1VIDEOINFO *)am_type.pbFormat;
    ok(mpeg1_info->cbSequenceHeader == 0, "got %lu\n", mpeg1_info->cbSequenceHeader);
    ok(mpeg1_info->dwStartTimeCode == 0, "got %lu\n", mpeg1_info->dwStartTimeCode);
    ok(mpeg1_info->hdr.bmiHeader.biPlanes == 1, "got %u\n", mpeg1_info->hdr.bmiHeader.biPlanes);
    ok(mpeg1_info->hdr.bmiHeader.biBitCount == 0, "got %u\n", mpeg1_info->hdr.bmiHeader.biBitCount);
    todo_wine ok(mpeg1_info->hdr.bmiHeader.biCompression == 0, "got %lu\n", mpeg1_info->hdr.bmiHeader.biCompression);
    CoTaskMemFree(am_type.pbFormat);

    hr = IMFMediaType_SetUINT64(media_type, &MF_MT_FRAME_SIZE, (UINT64)12 << 32 | 34);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFInitAMMediaTypeFromMFMediaType(media_type, GUID_NULL, &am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(am_type.cbFormat == sizeof(MPEG1VIDEOINFO), "got %lu\n", am_type.cbFormat);
    mpeg1_info = (MPEG1VIDEOINFO *)am_type.pbFormat;
    ok(mpeg1_info->hdr.bmiHeader.biPlanes == 1, "got %u\n", mpeg1_info->hdr.bmiHeader.biPlanes);
    ok(mpeg1_info->hdr.bmiHeader.biBitCount == 0, "got %u\n", mpeg1_info->hdr.bmiHeader.biBitCount);
    ok(mpeg1_info->hdr.bmiHeader.biWidth == 12, "got %lu\n", mpeg1_info->hdr.bmiHeader.biWidth);
    ok(mpeg1_info->hdr.bmiHeader.biHeight == 34, "got %lu\n", mpeg1_info->hdr.bmiHeader.biHeight);
    ok(mpeg1_info->hdr.bmiHeader.biSizeImage == 0, "got %lu\n", mpeg1_info->hdr.bmiHeader.biSizeImage);
    ok(mpeg1_info->hdr.bmiHeader.biXPelsPerMeter == 0, "got %lu\n", mpeg1_info->hdr.bmiHeader.biXPelsPerMeter);
    ok(mpeg1_info->hdr.bmiHeader.biYPelsPerMeter == 0, "got %lu\n", mpeg1_info->hdr.bmiHeader.biYPelsPerMeter);
    CoTaskMemFree(am_type.pbFormat);

    hr = IMFMediaType_SetUINT64(media_type, &MF_MT_PIXEL_ASPECT_RATIO, (UINT64)12 << 32 | 34);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFInitAMMediaTypeFromMFMediaType(media_type, GUID_NULL, &am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(am_type.cbFormat == sizeof(MPEG1VIDEOINFO), "got %lu\n", am_type.cbFormat);
    mpeg1_info = (MPEG1VIDEOINFO *)am_type.pbFormat;
    ok(mpeg1_info->hdr.bmiHeader.biXPelsPerMeter == 0, "got %lu\n", mpeg1_info->hdr.bmiHeader.biXPelsPerMeter);
    ok(mpeg1_info->hdr.bmiHeader.biYPelsPerMeter == 0, "got %lu\n", mpeg1_info->hdr.bmiHeader.biYPelsPerMeter);
    CoTaskMemFree(am_type.pbFormat);

    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_MPEG_START_TIME_CODE, 1234);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFInitAMMediaTypeFromMFMediaType(media_type, GUID_NULL, &am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(am_type.cbFormat == sizeof(MPEG1VIDEOINFO), "got %lu\n", am_type.cbFormat);
    mpeg1_info = (MPEG1VIDEOINFO *)am_type.pbFormat;
    ok(mpeg1_info->dwStartTimeCode == 1234, "got %lu\n", mpeg1_info->dwStartTimeCode);
    CoTaskMemFree(am_type.pbFormat);

    /* MF_MT_USER_DATA is ignored */

    hr = IMFMediaType_SetBlob(media_type, &MF_MT_USER_DATA, (BYTE *)dummy_user_data, sizeof(dummy_user_data));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFInitAMMediaTypeFromMFMediaType(media_type, GUID_NULL, &am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(am_type.cbFormat == sizeof(MPEG1VIDEOINFO), "got %lu\n", am_type.cbFormat);
    mpeg1_info = (MPEG1VIDEOINFO *)am_type.pbFormat;
    ok(mpeg1_info->hdr.bmiHeader.biSize == 0, "got %lu\n", mpeg1_info->hdr.bmiHeader.biSize);
    ok(mpeg1_info->cbSequenceHeader == 0, "got %lu\n", mpeg1_info->cbSequenceHeader);
    CoTaskMemFree(am_type.pbFormat);

    /* MF_MT_MPEG_SEQUENCE_HEADER is used instead in MPEG1VIDEOINFO */

    hr = IMFMediaType_SetBlob(media_type, &MF_MT_MPEG_SEQUENCE_HEADER, (BYTE *)dummy_mpeg_sequence, sizeof(dummy_mpeg_sequence));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFInitAMMediaTypeFromMFMediaType(media_type, GUID_NULL, &am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(am_type.cbFormat == sizeof(MPEG1VIDEOINFO) + sizeof(dummy_mpeg_sequence), "got %lu\n", am_type.cbFormat);
    mpeg1_info = (MPEG1VIDEOINFO *)am_type.pbFormat;
    ok(mpeg1_info->hdr.bmiHeader.biSize == 0, "got %lu\n", mpeg1_info->hdr.bmiHeader.biSize);
    ok(mpeg1_info->cbSequenceHeader == sizeof(dummy_mpeg_sequence), "got %lu\n", mpeg1_info->cbSequenceHeader);
    ok(!memcmp(mpeg1_info->bSequenceHeader, dummy_mpeg_sequence, mpeg1_info->cbSequenceHeader), "got wrong data\n");
    CoTaskMemFree(am_type.pbFormat);

    /* MFVIDEOFORMAT loses MF_MT_MPEG_SEQUENCE_HEADER */

    hr = IMFMediaType_DeleteItem(media_type, &MF_MT_USER_DATA);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFInitAMMediaTypeFromMFMediaType(media_type, FORMAT_MFVideoFormat, &am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(am_type.cbFormat == sizeof(MFVIDEOFORMAT), "got %lu\n", am_type.cbFormat);
    CoTaskMemFree(am_type.pbFormat);

    IMFMediaType_DeleteAllItems(media_type);


    /* MEDIASUBTYPE_MPEG2_VIDEO uses FORMAT_MPEG2Video by default */

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MEDIASUBTYPE_MPEG2_VIDEO);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFInitAMMediaTypeFromMFMediaType(media_type, GUID_NULL, &am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&am_type.majortype, &MFMediaType_Video), "got %s.\n", debugstr_guid(&am_type.majortype));
    ok(IsEqualGUID(&am_type.subtype, &MEDIASUBTYPE_MPEG2_VIDEO), "got %s.\n", debugstr_guid(&am_type.subtype));
    ok(IsEqualGUID(&am_type.formattype, &FORMAT_MPEG2Video), "got %s.\n", debugstr_guid(&am_type.formattype));
    ok(am_type.cbFormat == sizeof(MPEG2VIDEOINFO), "got %lu\n", am_type.cbFormat);
    mpeg2_info = (MPEG2VIDEOINFO *)am_type.pbFormat;
    ok(mpeg2_info->dwStartTimeCode == 0, "got %lu\n", mpeg2_info->dwStartTimeCode);
    ok(mpeg2_info->dwProfile == 0, "got %lu\n", mpeg2_info->dwProfile);
    ok(mpeg2_info->dwLevel == 0, "got %lu\n", mpeg2_info->dwLevel);
    ok(mpeg2_info->dwFlags == 0, "got %lu\n", mpeg2_info->dwFlags);
    ok(mpeg2_info->cbSequenceHeader == 0, "got %lu\n", mpeg2_info->cbSequenceHeader);
    CoTaskMemFree(am_type.pbFormat);

    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_MPEG_START_TIME_CODE, 1234);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_MPEG2_PROFILE, 6);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_MPEG2_LEVEL, 7);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_MPEG2_FLAGS, 8910);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFInitAMMediaTypeFromMFMediaType(media_type, GUID_NULL, &am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(am_type.cbFormat == sizeof(MPEG2VIDEOINFO), "got %lu\n", am_type.cbFormat);
    mpeg2_info = (MPEG2VIDEOINFO *)am_type.pbFormat;
    ok(mpeg2_info->dwStartTimeCode == 1234, "got %lu\n", mpeg2_info->dwStartTimeCode);
    ok(mpeg2_info->dwProfile == 6, "got %lu\n", mpeg2_info->dwProfile);
    ok(mpeg2_info->dwLevel == 7, "got %lu\n", mpeg2_info->dwLevel);
    ok(mpeg2_info->dwFlags == 8910, "got %lu\n", mpeg2_info->dwFlags);
    CoTaskMemFree(am_type.pbFormat);

    /* MF_MT_USER_DATA is ignored */

    hr = IMFMediaType_SetBlob(media_type, &MF_MT_USER_DATA, (BYTE *)dummy_user_data, sizeof(dummy_user_data));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFInitAMMediaTypeFromMFMediaType(media_type, GUID_NULL, &am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(am_type.cbFormat == sizeof(MPEG2VIDEOINFO), "got %lu\n", am_type.cbFormat);
    mpeg2_info = (MPEG2VIDEOINFO *)am_type.pbFormat;
    ok(mpeg2_info->hdr.bmiHeader.biSize == sizeof(mpeg2_info->hdr.bmiHeader), "got %lu\n", mpeg2_info->hdr.bmiHeader.biSize);
    ok(mpeg2_info->cbSequenceHeader == 0, "got %lu\n", mpeg2_info->cbSequenceHeader);
    CoTaskMemFree(am_type.pbFormat);

    /* MF_MT_MPEG_SEQUENCE_HEADER is used instead in MPEG2VIDEOINFO */

    hr = IMFMediaType_SetBlob(media_type, &MF_MT_MPEG_SEQUENCE_HEADER, (BYTE *)dummy_mpeg_sequence, sizeof(dummy_mpeg_sequence));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFInitAMMediaTypeFromMFMediaType(media_type, GUID_NULL, &am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(am_type.cbFormat == sizeof(MPEG2VIDEOINFO) + sizeof(dummy_mpeg_sequence), "got %lu\n", am_type.cbFormat);
    mpeg2_info = (MPEG2VIDEOINFO *)am_type.pbFormat;
    ok(mpeg2_info->hdr.bmiHeader.biSize == sizeof(mpeg2_info->hdr.bmiHeader), "got %lu\n", mpeg2_info->hdr.bmiHeader.biSize);
    ok(mpeg2_info->cbSequenceHeader == sizeof(dummy_mpeg_sequence), "got %lu\n", mpeg2_info->cbSequenceHeader);
    ok(!memcmp(mpeg2_info->dwSequenceHeader, dummy_mpeg_sequence, mpeg2_info->cbSequenceHeader), "got wrong data\n");
    CoTaskMemFree(am_type.pbFormat);

    /* MFVIDEOFORMAT loses MF_MT_MPEG_SEQUENCE_HEADER */

    hr = IMFMediaType_DeleteItem(media_type, &MF_MT_USER_DATA);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFInitAMMediaTypeFromMFMediaType(media_type, FORMAT_MFVideoFormat, &am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(am_type.cbFormat == sizeof(MFVIDEOFORMAT), "got %lu\n", am_type.cbFormat);
    CoTaskMemFree(am_type.pbFormat);

    IMFMediaType_DeleteAllItems(media_type);


    IMFMediaType_Release(media_type);
}

static void test_MFCreateAMMediaTypeFromMFMediaType(void)
{
    IMFMediaType *media_type;
    AM_MEDIA_TYPE *am_type;
    HRESULT hr;

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);

    hr = MFCreateAMMediaTypeFromMFMediaType(media_type, GUID_NULL, &am_type);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFCreateAMMediaTypeFromMFMediaType(media_type, GUID_NULL, &am_type);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFCreateAMMediaTypeFromMFMediaType(media_type, GUID_NULL, &am_type);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFAudioFormat_PCM);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = MFCreateAMMediaTypeFromMFMediaType(media_type, GUID_NULL, &am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&am_type->majortype, &MFMediaType_Audio), "got %s.\n", debugstr_guid(&am_type->majortype));
    ok(IsEqualGUID(&am_type->subtype, &MFAudioFormat_PCM), "got %s.\n", debugstr_guid(&am_type->subtype));
    ok(IsEqualGUID(&am_type->formattype, &FORMAT_WaveFormatEx), "got %s.\n", debugstr_guid(&am_type->formattype));
    ok(am_type->cbFormat == sizeof(WAVEFORMATEX), "got %lu\n", am_type->cbFormat);
    CoTaskMemFree(am_type->pbFormat);
    CoTaskMemFree(am_type);

    IMFMediaType_Release(media_type);
}

static void test_MFInitMediaTypeFromMFVideoFormat(void)
{
    static const MFPaletteEntry expect_palette[] = {{{1}},{{2}},{{3}},{{4}},{{5}},{{6}},{{7}},{{8}}};
    static const BYTE expect_user_data[] = {6,5,4,3,2,1};
    MFPaletteEntry palette[ARRAY_SIZE(expect_palette)];
    BYTE user_data[sizeof(expect_user_data)];
    char buffer[sizeof(MFVIDEOFORMAT) + sizeof(palette) + sizeof(user_data)];
    MFVIDEOFORMAT format, *format_buf = (MFVIDEOFORMAT *)buffer;
    IMFMediaType *media_type;
    MFVideoArea aperture;
    UINT32 i, value32;
    UINT64 value64;
    HRESULT hr;
    GUID guid;

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = MFInitMediaTypeFromMFVideoFormat(media_type, NULL, 0);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    memset(&format, 0, sizeof(format));
    format.dwSize = sizeof(format) - 1;
    hr = MFInitMediaTypeFromMFVideoFormat(media_type, &format, sizeof(format) - 1);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    format.dwSize = sizeof(format);
    hr = MFInitMediaTypeFromMFVideoFormat(media_type, &format, sizeof(format) - 1);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = MFInitMediaTypeFromMFVideoFormat(media_type, &format, sizeof(format));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    memset(&guid, 0xcd, sizeof(guid));
    hr = IMFMediaType_GetGUID(media_type, &MF_MT_MAJOR_TYPE, &guid);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&guid, &MFMediaType_Video), "got %s.\n", debugstr_guid(&guid));
    hr = IMFMediaType_GetGUID(media_type, &MF_MT_SUBTYPE, &guid);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_DeleteAllItems(media_type);

    format.guidFormat = MFVideoFormat_H264;
    hr = MFInitMediaTypeFromMFVideoFormat(media_type, &format, sizeof(format));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    memset(&guid, 0xcd, sizeof(guid));
    hr = IMFMediaType_GetGUID(media_type, &MF_MT_SUBTYPE, &guid);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&guid, &MFVideoFormat_H264), "got %s.\n", debugstr_guid(&guid));
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_FIXED_SIZE_SAMPLES, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_ALL_SAMPLES_INDEPENDENT, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_DeleteAllItems(media_type);

    format.guidFormat = MFVideoFormat_RGB32;
    hr = MFInitMediaTypeFromMFVideoFormat(media_type, &format, sizeof(format));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    memset(&guid, 0xcd, sizeof(guid));
    hr = IMFMediaType_GetGUID(media_type, &MF_MT_SUBTYPE, &guid);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&guid, &MFVideoFormat_RGB32), "got %s.\n", debugstr_guid(&guid));
    value32 = 0xdeadbeef;
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_FIXED_SIZE_SAMPLES, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == 1, "got %u.\n", value32);
    value32 = 0xdeadbeef;
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_ALL_SAMPLES_INDEPENDENT, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == 1, "got %u.\n", value32);
    hr = IMFMediaType_GetUINT64(media_type, &MF_MT_FRAME_SIZE, &value64);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_DEFAULT_STRIDE, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_SAMPLE_SIZE, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_DeleteAllItems(media_type);

    format.videoInfo.dwWidth = -123;
    hr = MFInitMediaTypeFromMFVideoFormat(media_type, &format, sizeof(format));
    todo_wine
    ok(hr == HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW), "Unexpected hr %#lx.\n", hr);
    format.videoInfo.dwWidth = 123;
    hr = MFInitMediaTypeFromMFVideoFormat(media_type, &format, sizeof(format));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT64(media_type, &MF_MT_FRAME_SIZE, &value64);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_DeleteAllItems(media_type);
    format.videoInfo.dwHeight = -456;
    hr = MFInitMediaTypeFromMFVideoFormat(media_type, &format, sizeof(format));
    todo_wine
    ok(hr == HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW), "Unexpected hr %#lx.\n", hr);
    format.videoInfo.dwHeight = 456;
    hr = MFInitMediaTypeFromMFVideoFormat(media_type, &format, sizeof(format));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    value64 = 0xdeadbeef;
    hr = IMFMediaType_GetUINT64(media_type, &MF_MT_FRAME_SIZE, &value64);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value64 == (((UINT64)123 << 32) | 456), "got %#I64x.\n", value64);
    value32 = 0xdeadbeef;
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_DEFAULT_STRIDE, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == 123 * 4, "got %u.\n", value32);
    value32 = 0xdeadbeef;
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_SAMPLE_SIZE, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == 123 * 4 * 456, "got %u.\n", value32);
    hr = IMFMediaType_GetUINT64(media_type, &MF_MT_PIXEL_ASPECT_RATIO, &value64);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_DeleteAllItems(media_type);

    /* MFVideoFlag_BottomUpLinearRep flag inverts the stride */
    format.videoInfo.VideoFlags = MFVideoFlag_BottomUpLinearRep;
    hr = MFInitMediaTypeFromMFVideoFormat(media_type, &format, sizeof(format));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    value32 = 0xdeadbeef;
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_DEFAULT_STRIDE, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == -123 * 4, "got %u.\n", value32);
    value32 = 0xdeadbeef;
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_SAMPLE_SIZE, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == 123 * 4 * 456, "got %u.\n", value32);
    IMFMediaType_DeleteAllItems(media_type);

    /* MFVideoFlag_BottomUpLinearRep flag only works with RGB formats */
    format.guidFormat = MFVideoFormat_H264;
    hr = MFInitMediaTypeFromMFVideoFormat(media_type, &format, sizeof(format));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_DEFAULT_STRIDE, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_SAMPLE_SIZE, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_DeleteAllItems(media_type);

    format.guidFormat = MFVideoFormat_NV12;
    hr = MFInitMediaTypeFromMFVideoFormat(media_type, &format, sizeof(format));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    value32 = 0xdeadbeef;
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_DEFAULT_STRIDE, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine
    ok(value32 == 124, "got %u.\n", value32);
    value32 = 0xdeadbeef;
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_SAMPLE_SIZE, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == 124 * 456 * 3 / 2, "got %u.\n", value32);
    IMFMediaType_DeleteAllItems(media_type);

    format.videoInfo.PixelAspectRatio.Numerator = 7;
    hr = MFInitMediaTypeFromMFVideoFormat(media_type, &format, sizeof(format));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT64(media_type, &MF_MT_PIXEL_ASPECT_RATIO, &value64);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_DeleteAllItems(media_type);
    format.videoInfo.PixelAspectRatio.Denominator = 8;
    hr = MFInitMediaTypeFromMFVideoFormat(media_type, &format, sizeof(format));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    value64 = 0xdeadbeef;
    hr = IMFMediaType_GetUINT64(media_type, &MF_MT_PIXEL_ASPECT_RATIO, &value64);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value64 == (((UINT64)7 << 32) | 8), "got %#I64x.\n", value64);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_VIDEO_CHROMA_SITING, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_DeleteAllItems(media_type);

    format.videoInfo.SourceChromaSubsampling = MFVideoChromaSubsampling_MPEG2;
    hr = MFInitMediaTypeFromMFVideoFormat(media_type, &format, sizeof(format));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    value32 = 0xdeadbeef;
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_VIDEO_CHROMA_SITING, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == MFVideoChromaSubsampling_MPEG2, "got %u.\n", value32);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_INTERLACE_MODE, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_DeleteAllItems(media_type);

    format.videoInfo.InterlaceMode = MFVideoInterlace_MixedInterlaceOrProgressive;
    hr = MFInitMediaTypeFromMFVideoFormat(media_type, &format, sizeof(format));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    value32 = 0xdeadbeef;
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_INTERLACE_MODE, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == MFVideoInterlace_MixedInterlaceOrProgressive, "got %u.\n", value32);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_TRANSFER_FUNCTION, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_DeleteAllItems(media_type);

    format.videoInfo.TransferFunction = MFVideoTransFunc_709;
    hr = MFInitMediaTypeFromMFVideoFormat(media_type, &format, sizeof(format));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    value32 = 0xdeadbeef;
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_TRANSFER_FUNCTION, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == MFVideoTransFunc_709, "got %u.\n", value32);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_VIDEO_PRIMARIES, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_DeleteAllItems(media_type);

    format.videoInfo.ColorPrimaries = MFVideoPrimaries_BT709;
    hr = MFInitMediaTypeFromMFVideoFormat(media_type, &format, sizeof(format));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    value32 = 0xdeadbeef;
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_VIDEO_PRIMARIES, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == MFVideoPrimaries_BT709, "got %u.\n", value32);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_YUV_MATRIX, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_DeleteAllItems(media_type);

    format.videoInfo.TransferMatrix = MFVideoTransferMatrix_BT709;
    hr = MFInitMediaTypeFromMFVideoFormat(media_type, &format, sizeof(format));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    value32 = 0xdeadbeef;
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_YUV_MATRIX, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == MFVideoTransferMatrix_BT709, "got %u.\n", value32);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_VIDEO_LIGHTING, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_DeleteAllItems(media_type);

    format.videoInfo.SourceLighting = MFVideoLighting_bright;
    hr = MFInitMediaTypeFromMFVideoFormat(media_type, &format, sizeof(format));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    value32 = 0xdeadbeef;
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_VIDEO_LIGHTING, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == MFVideoLighting_bright, "got %u.\n", value32);
    hr = IMFMediaType_GetUINT64(media_type, &MF_MT_FRAME_RATE, &value64);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_DeleteAllItems(media_type);

    format.videoInfo.FramesPerSecond.Numerator = 30000;
    hr = MFInitMediaTypeFromMFVideoFormat(media_type, &format, sizeof(format));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT64(media_type, &MF_MT_FRAME_RATE, &value64);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_DeleteAllItems(media_type);
    format.videoInfo.FramesPerSecond.Denominator = 1001;
    hr = MFInitMediaTypeFromMFVideoFormat(media_type, &format, sizeof(format));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    value64 = 0xdeadbeef;
    hr = IMFMediaType_GetUINT64(media_type, &MF_MT_FRAME_RATE, &value64);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value64 == (((UINT64)30000 << 32) | 1001), "got %#I64x.\n", value64);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_VIDEO_NOMINAL_RANGE, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_DeleteAllItems(media_type);

    format.videoInfo.NominalRange = MFNominalRange_Wide;
    hr = MFInitMediaTypeFromMFVideoFormat(media_type, &format, sizeof(format));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    value32 = 0xdeadbeef;
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_VIDEO_NOMINAL_RANGE, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == MFNominalRange_Wide, "got %u.\n", value32);
    hr = IMFMediaType_GetBlobSize(media_type, &MF_MT_GEOMETRIC_APERTURE, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_DeleteAllItems(media_type);

    format.videoInfo.GeometricAperture.OffsetX.value = 1;
    format.videoInfo.GeometricAperture.OffsetX.fract = 2;
    format.videoInfo.GeometricAperture.OffsetY.value = 3;
    format.videoInfo.GeometricAperture.OffsetY.fract = 4;
    format.videoInfo.GeometricAperture.Area.cx = -120;
    hr = MFInitMediaTypeFromMFVideoFormat(media_type, &format, sizeof(format));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetBlob(media_type, &MF_MT_GEOMETRIC_APERTURE, (BYTE *)&aperture, sizeof(aperture), &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    format.videoInfo.GeometricAperture.Area.cy = -450;
    hr = MFInitMediaTypeFromMFVideoFormat(media_type, &format, sizeof(format));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    value32 = 0xdeadbeef;
    memset(&aperture, 0xcd, sizeof(aperture));
    hr = IMFMediaType_GetBlob(media_type, &MF_MT_GEOMETRIC_APERTURE, (BYTE *)&aperture, sizeof(aperture), &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == sizeof(aperture), "got %u.\n", value32);
    ok(!memcmp(&format.videoInfo.GeometricAperture, &aperture, sizeof(aperture)), "Unexpected aperture.\n");
    hr = IMFMediaType_GetBlobSize(media_type, &MF_MT_MINIMUM_DISPLAY_APERTURE, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_DeleteAllItems(media_type);

    format.videoInfo.MinimumDisplayAperture.OffsetX.value = 1;
    format.videoInfo.MinimumDisplayAperture.OffsetX.fract = 2;
    format.videoInfo.MinimumDisplayAperture.OffsetY.value = 3;
    format.videoInfo.MinimumDisplayAperture.OffsetY.fract = 4;
    format.videoInfo.MinimumDisplayAperture.Area.cx = 120;
    hr = MFInitMediaTypeFromMFVideoFormat(media_type, &format, sizeof(format));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetBlob(media_type, &MF_MT_MINIMUM_DISPLAY_APERTURE, (BYTE *)&aperture, sizeof(aperture), &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    format.videoInfo.MinimumDisplayAperture.Area.cy = 450;
    hr = MFInitMediaTypeFromMFVideoFormat(media_type, &format, sizeof(format));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    value32 = 0xdeadbeef;
    memset(&aperture, 0xcd, sizeof(aperture));
    hr = IMFMediaType_GetBlob(media_type, &MF_MT_MINIMUM_DISPLAY_APERTURE, (BYTE *)&aperture, sizeof(aperture), &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == sizeof(aperture), "got %u.\n", value32);
    ok(!memcmp(&format.videoInfo.MinimumDisplayAperture, &aperture, sizeof(aperture)), "Unexpected aperture.\n");
    hr = IMFMediaType_GetBlobSize(media_type, &MF_MT_PAN_SCAN_APERTURE, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_DeleteAllItems(media_type);

    format.videoInfo.PanScanAperture.OffsetX.value = 1;
    format.videoInfo.PanScanAperture.OffsetX.fract = 2;
    format.videoInfo.PanScanAperture.OffsetY.value = 3;
    format.videoInfo.PanScanAperture.OffsetY.fract = 4;
    format.videoInfo.PanScanAperture.Area.cx = 120;
    hr = MFInitMediaTypeFromMFVideoFormat(media_type, &format, sizeof(format));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetBlob(media_type, &MF_MT_PAN_SCAN_APERTURE, (BYTE *)&aperture, sizeof(aperture), &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    format.videoInfo.PanScanAperture.Area.cy = 450;
    hr = MFInitMediaTypeFromMFVideoFormat(media_type, &format, sizeof(format));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    value32 = 0xdeadbeef;
    memset(&aperture, 0xcd, sizeof(aperture));
    hr = IMFMediaType_GetBlob(media_type, &MF_MT_PAN_SCAN_APERTURE, (BYTE *)&aperture, sizeof(aperture), &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == sizeof(aperture), "got %u.\n", value32);
    ok(!memcmp(&format.videoInfo.PanScanAperture, &aperture, sizeof(aperture)), "Unexpected aperture.\n");
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_PAN_SCAN_ENABLED, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_DeleteAllItems(media_type);

    format.videoInfo.VideoFlags |= MFVideoFlag_PanScanEnabled;
    hr = MFInitMediaTypeFromMFVideoFormat(media_type, &format, sizeof(format));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    value32 = 0xdeadbeef;
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_PAN_SCAN_ENABLED, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == 1, "got %u.\n", value32);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_PAD_CONTROL_FLAGS, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_DeleteAllItems(media_type);

    format.videoInfo.VideoFlags |= MFVideoFlag_PAD_TO_16x9;
    hr = MFInitMediaTypeFromMFVideoFormat(media_type, &format, sizeof(format));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    value32 = 0xdeadbeef;
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_PAD_CONTROL_FLAGS, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == 2, "got %u.\n", value32);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_SOURCE_CONTENT_HINT, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_DeleteAllItems(media_type);

    format.videoInfo.VideoFlags |= MFVideoFlag_SrcContentHint16x9;
    hr = MFInitMediaTypeFromMFVideoFormat(media_type, &format, sizeof(format));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    value32 = 0xdeadbeef;
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_SOURCE_CONTENT_HINT, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == 1, "got %u.\n", value32);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_DRM_FLAGS, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_DeleteAllItems(media_type);

    format.videoInfo.VideoFlags |= MFVideoFlag_DigitallyProtected;
    hr = MFInitMediaTypeFromMFVideoFormat(media_type, &format, sizeof(format));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    value32 = 0xdeadbeef;
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_DRM_FLAGS, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == 2, "got %u.\n", value32);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_AVG_BITRATE, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_DeleteAllItems(media_type);

    format.compressedInfo.AvgBitrate = 123456;
    hr = MFInitMediaTypeFromMFVideoFormat(media_type, &format, sizeof(format));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    value32 = 0xdeadbeef;
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_AVG_BITRATE, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == 123456, "got %u.\n", value32);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_AVG_BIT_ERROR_RATE, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_DeleteAllItems(media_type);

    format.compressedInfo.AvgBitErrorRate = 654321;
    hr = MFInitMediaTypeFromMFVideoFormat(media_type, &format, sizeof(format));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    value32 = 0xdeadbeef;
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_AVG_BIT_ERROR_RATE, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == 654321, "got %u.\n", value32);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_MAX_KEYFRAME_SPACING, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_DeleteAllItems(media_type);

    format.compressedInfo.MaxKeyFrameSpacing = -123;
    hr = MFInitMediaTypeFromMFVideoFormat(media_type, &format, sizeof(format));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    value32 = 0xdeadbeef;
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_MAX_KEYFRAME_SPACING, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == -123, "got %u.\n", value32);
    hr = IMFMediaType_GetBlobSize(media_type, &MF_MT_PALETTE, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_DeleteAllItems(media_type);

    /* any subtype works here */
    format.guidFormat = MFVideoFormat_H264;
    format.surfaceInfo.Format = MFVideoFormat_H264.Data1;
    hr = MFInitMediaTypeFromMFVideoFormat(media_type, &format, sizeof(format));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetBlobSize(media_type, &MF_MT_PALETTE, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_DeleteAllItems(media_type);

    *format_buf = format;
    for (i = 0; i < ARRAY_SIZE(expect_palette); i++)
        format_buf->surfaceInfo.Palette[i] = expect_palette[i];
    format_buf->surfaceInfo.PaletteEntries = ARRAY_SIZE(expect_palette);

    /* format sizes needs to include an extra palette entry */
    format_buf->dwSize = offsetof(MFVIDEOFORMAT, surfaceInfo.Palette[ARRAY_SIZE(expect_palette)]);
    hr = MFInitMediaTypeFromMFVideoFormat(media_type, format_buf, sizeof(format));
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    format_buf->dwSize = offsetof(MFVIDEOFORMAT, surfaceInfo.Palette[ARRAY_SIZE(expect_palette) + 1]);
    hr = MFInitMediaTypeFromMFVideoFormat(media_type, format_buf, format_buf->dwSize);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    value32 = 0xdeadbeef;
    memset(&palette, 0xcd, sizeof(palette));
    hr = IMFMediaType_GetBlob(media_type, &MF_MT_PALETTE, (BYTE *)&palette, sizeof(palette), &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == sizeof(expect_palette), "got %u.\n", value32);
    ok(!memcmp(palette, expect_palette, value32), "Unexpected palette.\n");
    hr = IMFMediaType_GetBlobSize(media_type, &MF_MT_USER_DATA, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_DeleteAllItems(media_type);

    memcpy(buffer + format_buf->dwSize, expect_user_data, sizeof(expect_user_data));
    format_buf->dwSize += sizeof(expect_user_data);
    hr = MFInitMediaTypeFromMFVideoFormat(media_type, format_buf, format_buf->dwSize);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    value32 = 0xdeadbeef;
    memset(&user_data, 0xcd, sizeof(user_data));
    hr = IMFMediaType_GetBlob(media_type, &MF_MT_USER_DATA, (BYTE *)user_data, sizeof(user_data), &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == sizeof(expect_user_data), "got %u.\n", value32);
    ok(!memcmp(user_data, expect_user_data, value32), "Unexpected user data.\n");
    IMFMediaType_DeleteAllItems(media_type);

    /* check that user data follows MFVIDEOFORMAT struct, which is padded, when no palette is present */
    format_buf->surfaceInfo.PaletteEntries = 0;
    memmove(format_buf + 1, expect_user_data, sizeof(expect_user_data));
    format_buf->dwSize = sizeof(*format_buf) + sizeof(expect_user_data);
    hr = MFInitMediaTypeFromMFVideoFormat(media_type, format_buf, format_buf->dwSize);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    value32 = 0xdeadbeef;
    memset(&user_data, 0xcd, sizeof(user_data));
    hr = IMFMediaType_GetBlob(media_type, &MF_MT_USER_DATA, (BYTE *)user_data, sizeof(user_data), &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == sizeof(expect_user_data), "got %u.\n", value32);
    ok(!memcmp(user_data, expect_user_data, value32), "Unexpected user data.\n");
    IMFMediaType_DeleteAllItems(media_type);

    IMFMediaType_Release(media_type);
}

static void test_IMFMediaType_GetRepresentation(void)
{
    WAVEFORMATEX wfx = {.wFormatTag = WAVE_FORMAT_PCM};
    IMFMediaType *media_type;
    AM_MEDIA_TYPE *am_type;
    HRESULT hr;

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Failed to create media type, hr %#lx.\n", hr);

    hr = IMFMediaType_GetRepresentation(media_type, GUID_NULL, (void **)&am_type);
    ok(hr == MF_E_UNSUPPORTED_REPRESENTATION, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetRepresentation(media_type, AM_MEDIA_TYPE_REPRESENTATION, (void **)&am_type);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetRepresentation(media_type, AM_MEDIA_TYPE_REPRESENTATION, (void **)&am_type);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetRepresentation(media_type, AM_MEDIA_TYPE_REPRESENTATION, (void **)&am_type);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaType_SetGUID(media_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Audio);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetGUID(media_type, &MF_MT_SUBTYPE, &MFAudioFormat_PCM);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetRepresentation(media_type, FORMAT_VideoInfo, (void **)&am_type);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetRepresentation(media_type, AM_MEDIA_TYPE_REPRESENTATION, (void **)&am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&am_type->majortype, &MFMediaType_Audio), "got %s.\n", debugstr_guid(&am_type->majortype));
    ok(IsEqualGUID(&am_type->subtype, &MFAudioFormat_PCM), "got %s.\n", debugstr_guid(&am_type->subtype));
    ok(IsEqualGUID(&am_type->formattype, &FORMAT_WaveFormatEx), "got %s.\n", debugstr_guid(&am_type->formattype));
    ok(am_type->cbFormat == sizeof(WAVEFORMATEX), "got %lu\n", am_type->cbFormat);
    hr = IMFMediaType_FreeRepresentation(media_type, IID_IUnknown /* invalid format */, am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_Release(media_type);

    hr = MFCreateAudioMediaType(&wfx, (IMFAudioMediaType **)&media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetRepresentation(media_type, GUID_NULL, (void **)&am_type);
    ok(hr == MF_E_UNSUPPORTED_REPRESENTATION, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetRepresentation(media_type, FORMAT_VideoInfo, (void **)&am_type);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetRepresentation(media_type, AM_MEDIA_TYPE_REPRESENTATION, (void **)&am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&am_type->majortype, &MFMediaType_Audio), "got %s.\n", debugstr_guid(&am_type->majortype));
    ok(IsEqualGUID(&am_type->subtype, &MFAudioFormat_PCM), "got %s.\n", debugstr_guid(&am_type->subtype));
    ok(IsEqualGUID(&am_type->formattype, &FORMAT_WaveFormatEx), "got %s.\n", debugstr_guid(&am_type->formattype));
    ok(am_type->cbFormat == sizeof(WAVEFORMATEX), "got %lu\n", am_type->cbFormat);
    hr = IMFMediaType_FreeRepresentation(media_type, AM_MEDIA_TYPE_REPRESENTATION, am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFMediaType_Release(media_type);

    hr = MFCreateVideoMediaTypeFromSubtype(&MFVideoFormat_RGB32, (IMFVideoMediaType **)&media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetRepresentation(media_type, GUID_NULL, (void **)&am_type);
    ok(hr == MF_E_UNSUPPORTED_REPRESENTATION, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetRepresentation(media_type, FORMAT_WaveFormatEx, (void **)&am_type);
    ok(hr == MF_E_UNSUPPORTED_REPRESENTATION, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetRepresentation(media_type, AM_MEDIA_TYPE_REPRESENTATION, (void **)&am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&am_type->majortype, &MFMediaType_Video), "got %s.\n", debugstr_guid(&am_type->majortype));
    ok(IsEqualGUID(&am_type->subtype, &MEDIASUBTYPE_RGB32), "got %s.\n", debugstr_guid(&am_type->subtype));
    ok(IsEqualGUID(&am_type->formattype, &FORMAT_VideoInfo), "got %s.\n", debugstr_guid(&am_type->formattype));
    ok(am_type->cbFormat == sizeof(VIDEOINFOHEADER), "got %lu\n", am_type->cbFormat);
    hr = IMFMediaType_FreeRepresentation(media_type, AM_MEDIA_TYPE_REPRESENTATION, am_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IMFMediaType_Release(media_type);
}

static void test_MFCreateMediaTypeFromRepresentation(void)
{
    IMFMediaType *media_type;
    AM_MEDIA_TYPE amt = {0};
    WAVEFORMATEX wfx = {0};
    HRESULT hr;
    GUID guid;

    hr = MFCreateMediaTypeFromRepresentation(GUID_NULL, &amt, &media_type);
    ok(hr == MF_E_UNSUPPORTED_REPRESENTATION, "Unexpected hr %#lx.\n", hr);
    hr = MFCreateMediaTypeFromRepresentation(AM_MEDIA_TYPE_REPRESENTATION, NULL, &media_type);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    hr = MFCreateMediaTypeFromRepresentation(AM_MEDIA_TYPE_REPRESENTATION, &amt, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateMediaTypeFromRepresentation(AM_MEDIA_TYPE_REPRESENTATION, &amt, &media_type);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (hr == S_OK)
    {
        memset(&guid, 0xcd, sizeof(guid));
        hr = IMFMediaType_GetGUID(media_type, &MF_MT_MAJOR_TYPE, &guid);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(IsEqualGUID(&guid, &GUID_NULL), "got %s.\n", debugstr_guid(&guid));
        memset(&guid, 0xcd, sizeof(guid));
        hr = IMFMediaType_GetGUID(media_type, &MF_MT_SUBTYPE, &guid);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(IsEqualGUID(&guid, &GUID_NULL), "got %s.\n", debugstr_guid(&guid));
        IMFMediaType_Release(media_type);
    }

    amt.formattype = FORMAT_WaveFormatEx;
    amt.majortype = MFMediaType_Audio;
    amt.subtype = MFAudioFormat_PCM;
    amt.pbFormat = (BYTE *)&wfx;
    amt.cbFormat = sizeof(wfx);
    hr = MFCreateMediaTypeFromRepresentation(AM_MEDIA_TYPE_REPRESENTATION, &amt, &media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    memset(&guid, 0xcd, sizeof(guid));
    hr = IMFMediaType_GetGUID(media_type, &MF_MT_MAJOR_TYPE, &guid);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&guid, &MFMediaType_Audio), "got %s.\n", debugstr_guid(&guid));
    memset(&guid, 0xcd, sizeof(guid));
    hr = IMFMediaType_GetGUID(media_type, &MF_MT_SUBTYPE, &guid);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine
    ok(IsEqualGUID(&guid, &MFAudioFormat_PCM), "got %s.\n", debugstr_guid(&guid));
    IMFMediaType_Release(media_type);
}

static void test_MFCreateDXSurfaceBuffer(void)
{
    IDirect3DSurface9 *backbuffer = NULL, *surface;
    IDirect3DSwapChain9 *swapchain;
    D3DLOCKED_RECT locked_rect;
    DWORD length, max_length;
    IDirect3DDevice9 *device;
    IMF2DBuffer2 *_2dbuffer2;
    BOOL value, broken_test;
    IMFMediaBuffer *buffer;
    IMF2DBuffer *_2dbuffer;
    BYTE *data, *data2;
    IMFGetService *gs;
    IDirect3D9 *d3d;
    DWORD color;
    HWND window;
    HRESULT hr;
    LONG pitch;

    if (!pMFCreateDXSurfaceBuffer)
    {
        win_skip("MFCreateDXSurfaceBuffer is not available.\n");
        return;
    }

    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!d3d)
    {
        skip("Failed to create a D3D9 object, skipping tests.\n");
        return;
    }
    window = create_window();
    if (!(device = create_d3d9_device(d3d, window)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice9_GetSwapChain(device, 0, &swapchain);
    ok(SUCCEEDED(hr), "Failed to get the implicit swapchain (%08lx)\n", hr);

    hr = IDirect3DSwapChain9_GetBackBuffer(swapchain, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer);
    ok(SUCCEEDED(hr), "Failed to get the back buffer (%08lx)\n", hr);
    ok(backbuffer != NULL, "The back buffer is NULL\n");

    IDirect3DSwapChain9_Release(swapchain);

    hr = pMFCreateDXSurfaceBuffer(&IID_IUnknown, (IUnknown *)backbuffer, FALSE, &buffer);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = pMFCreateDXSurfaceBuffer(&IID_IDirect3DSurface9, (IUnknown *)backbuffer, FALSE, &buffer);
    ok(hr == S_OK, "Failed to create a buffer, hr %#lx.\n", hr);

    check_interface(buffer, &IID_IMF2DBuffer, TRUE);
    check_interface(buffer, &IID_IMF2DBuffer2, TRUE);
    check_interface(buffer, &IID_IMFGetService, TRUE);

    /* Surface is accessible. */
    hr = IMFMediaBuffer_QueryInterface(buffer, &IID_IMFGetService, (void **)&gs);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFGetService_GetService(gs, &MR_BUFFER_SERVICE, &IID_IDirect3DSurface9, (void **)&surface);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(surface == backbuffer, "Unexpected surface pointer.\n");
    IDirect3DSurface9_Release(surface);
    IMFGetService_Release(gs);

    max_length = 0;
    hr = IMFMediaBuffer_GetMaxLength(buffer, &max_length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!max_length, "Unexpected length %lu.\n", max_length);

    hr = IMFMediaBuffer_GetCurrentLength(buffer, &length);
    ok(hr == S_OK, "Failed to get length, hr %#lx.\n", hr);
    ok(!length, "Unexpected length %lu.\n", length);

    hr = IMFMediaBuffer_SetCurrentLength(buffer, 2 * max_length);
    ok(hr == S_OK, "Failed to get length, hr %#lx.\n", hr);

    hr = IMFMediaBuffer_GetCurrentLength(buffer, &length);
    ok(hr == S_OK, "Failed to get length, hr %#lx.\n", hr);
    ok(length == 2 * max_length, "Unexpected length %lu.\n", length);

    hr = IDirect3DSurface9_LockRect(backbuffer, &locked_rect, NULL, 0);
    ok(hr == S_OK, "Failed to lock back buffer, hr %#lx.\n", hr);

    /* Cannot lock while the surface is locked. */
    hr = IMFMediaBuffer_Lock(buffer, &data, NULL, &length);
    ok(hr == D3DERR_INVALIDCALL, "Unexpected hr %#lx.\n", hr);

    hr = IDirect3DSurface9_UnlockRect(backbuffer);
    ok(hr == S_OK, "Failed to unlock back buffer, hr %#lx.\n", hr);

    hr = IMFMediaBuffer_Lock(buffer, &data, NULL, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    /* Broken on Windows 8 and 10 v1507 */
    broken_test = length == 0;
    ok(length == max_length || broken(broken_test), "Unexpected length %lu instead of %lu.\n", length, max_length);

    /* You can lock the surface while the media buffer is locked. */
    hr = IDirect3DSurface9_LockRect(backbuffer, &locked_rect, NULL, 0);
    ok(hr == S_OK, "Failed to lock back buffer, hr %#lx.\n", hr);

    hr = IMFMediaBuffer_Lock(buffer, &data, NULL, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaBuffer_Unlock(buffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirect3DSurface9_UnlockRect(backbuffer);
    ok(hr == S_OK, "Failed to unlock back buffer, hr %#lx.\n", hr);

    /* Unlock twice. */
    hr = IMFMediaBuffer_Unlock(buffer);
    ok(hr == S_OK || broken(broken_test), "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaBuffer_Unlock(buffer);
    ok(hr == HRESULT_FROM_WIN32(ERROR_WAS_UNLOCKED), "Unexpected hr %#lx.\n", hr);

    /* Lock twice. */
    hr = IMFMediaBuffer_Lock(buffer, &data, NULL, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaBuffer_Lock(buffer, &data2, NULL, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(data == data2, "Unexpected pointer.\n");

    hr = IMFMediaBuffer_Unlock(buffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaBuffer_Unlock(buffer);
    ok(hr == S_OK || broken(broken_test), "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaBuffer_QueryInterface(buffer, &IID_IMF2DBuffer, (void **)&_2dbuffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Unlocked. */
    hr = IMF2DBuffer_GetScanline0AndPitch(_2dbuffer, &data, &pitch);
    ok(hr == HRESULT_FROM_WIN32(ERROR_WAS_UNLOCKED), "Unexpected hr %#lx.\n", hr);

    hr = IDirect3DSurface9_LockRect(backbuffer, &locked_rect, NULL, 0);
    ok(hr == S_OK, "Failed to lock back buffer, hr %#lx.\n", hr);

    /* Cannot lock the buffer while the surface is locked. */
    hr = IMF2DBuffer_Lock2D(_2dbuffer, &data, &pitch);
    ok(hr == D3DERR_INVALIDCALL, "Unexpected hr %#lx.\n", hr);

    hr = IDirect3DSurface9_UnlockRect(backbuffer);
    ok(hr == S_OK, "Failed to unlock back buffer, hr %#lx.\n", hr);

    hr = IMF2DBuffer_Lock2D(_2dbuffer, &data, &pitch);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Cannot lock the surface once the buffer is locked. */
    hr = IDirect3DSurface9_LockRect(backbuffer, &locked_rect, NULL, 0);
    ok(hr == D3DERR_INVALIDCALL, "Unexpected hr %#lx.\n", hr);

    hr = IMF2DBuffer_GetScanline0AndPitch(_2dbuffer, &data, &pitch);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaBuffer_Lock(buffer, &data2, NULL, NULL);
    ok(hr == MF_E_INVALIDREQUEST, "Unexpected hr %#lx.\n", hr);

    hr = IMF2DBuffer_Lock2D(_2dbuffer, &data, &pitch);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMF2DBuffer_Unlock2D(_2dbuffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMF2DBuffer_Unlock2D(_2dbuffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMF2DBuffer_Unlock2D(_2dbuffer);
    ok(hr == HRESULT_FROM_WIN32(ERROR_WAS_UNLOCKED), "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaBuffer_Lock(buffer, &data, NULL, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMF2DBuffer_Lock2D(_2dbuffer, &data, &pitch);
    ok(hr == MF_E_UNEXPECTED, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaBuffer_Unlock(buffer);
    ok(hr == S_OK || broken(broken_test), "Unexpected hr %#lx.\n", hr);

    hr = IMF2DBuffer_IsContiguousFormat(_2dbuffer, &value);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!value, "Unexpected return value %d.\n", value);

    hr = IMF2DBuffer_GetContiguousLength(_2dbuffer, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);
    hr = IMF2DBuffer_GetContiguousLength(_2dbuffer, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(length == max_length, "Unexpected length %lu.\n", length);

    IMF2DBuffer_Release(_2dbuffer);

    hr = IMFMediaBuffer_QueryInterface(buffer, &IID_IMF2DBuffer2, (void **)&_2dbuffer2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Lock flags are ignored, so writing is allowed when locking for
     * reading and viceversa. */
    put_d3d9_surface_color(backbuffer, 0, 0, 0xcdcdcdcd);
    hr = IMF2DBuffer2_Lock2DSize(_2dbuffer2, MF2DBuffer_LockFlags_Read, &data, &pitch, &data2, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(data == data2, "Unexpected scanline pointer.\n");
    ok(data[0] == 0xcd, "Unexpected leading byte.\n");
    memset(data, 0xab, 4);
    IMF2DBuffer2_Unlock2D(_2dbuffer2);

    color = get_d3d9_surface_color(backbuffer, 0, 0);
    ok(color == 0xabababab, "Unexpected leading dword.\n");
    put_d3d9_surface_color(backbuffer, 0, 0, 0xefefefef);

    hr = IMF2DBuffer2_Lock2DSize(_2dbuffer2, MF2DBuffer_LockFlags_Write, &data, &pitch, &data2, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(data[0] == 0xef, "Unexpected leading byte.\n");
    memset(data, 0x89, 4);
    IMF2DBuffer2_Unlock2D(_2dbuffer2);

    color = get_d3d9_surface_color(backbuffer, 0, 0);
    ok(color == 0x89898989, "Unexpected leading dword.\n");

    /* Also, flags incompatibilities are not taken into account even
     * if a buffer is already locked. */
    hr = IMF2DBuffer2_Lock2DSize(_2dbuffer2, MF2DBuffer_LockFlags_ReadWrite, &data, &pitch, &data2, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMF2DBuffer2_Lock2DSize(_2dbuffer2, MF2DBuffer_LockFlags_ReadWrite, &data, &pitch, &data2, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMF2DBuffer2_Lock2DSize(_2dbuffer2, MF2DBuffer_LockFlags_Read, &data, &pitch, &data2, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMF2DBuffer2_Lock2DSize(_2dbuffer2, MF2DBuffer_LockFlags_Write, &data, &pitch, &data2, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMF2DBuffer_Lock2D(_2dbuffer, &data, &pitch);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMF2DBuffer2_Unlock2D(_2dbuffer2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMF2DBuffer2_Unlock2D(_2dbuffer2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMF2DBuffer2_Unlock2D(_2dbuffer2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMF2DBuffer2_Unlock2D(_2dbuffer2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMF2DBuffer2_Unlock2D(_2dbuffer2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMF2DBuffer2_Lock2DSize(_2dbuffer2, MF2DBuffer_LockFlags_Read, &data, &pitch, &data2, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMF2DBuffer2_Lock2DSize(_2dbuffer2, MF2DBuffer_LockFlags_Read, &data, &pitch, &data2, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMF2DBuffer2_Lock2DSize(_2dbuffer2, MF2DBuffer_LockFlags_ReadWrite, &data, &pitch, &data2, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMF2DBuffer2_Lock2DSize(_2dbuffer2, MF2DBuffer_LockFlags_Write, &data, &pitch, &data2, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMF2DBuffer_Lock2D(_2dbuffer, &data, &pitch);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMF2DBuffer2_Unlock2D(_2dbuffer2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMF2DBuffer2_Unlock2D(_2dbuffer2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMF2DBuffer2_Unlock2D(_2dbuffer2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMF2DBuffer2_Unlock2D(_2dbuffer2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMF2DBuffer2_Unlock2D(_2dbuffer2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Except when originally locking for writing. */
    hr = IMF2DBuffer2_Lock2DSize(_2dbuffer2, MF2DBuffer_LockFlags_Write, &data, &pitch, &data2, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMF2DBuffer2_Lock2DSize(_2dbuffer2, MF2DBuffer_LockFlags_Write, &data, &pitch, &data2, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMF2DBuffer2_Lock2DSize(_2dbuffer2, MF2DBuffer_LockFlags_ReadWrite, &data, &pitch, &data2, &length);
    ok(hr == HRESULT_FROM_WIN32(ERROR_WAS_LOCKED), "Unexpected hr %#lx.\n", hr);
    hr = IMF2DBuffer2_Lock2DSize(_2dbuffer2, MF2DBuffer_LockFlags_Read, &data, &pitch, &data2, &length);
    ok(hr == HRESULT_FROM_WIN32(ERROR_WAS_LOCKED), "Unexpected hr %#lx.\n", hr);
    hr = IMF2DBuffer_Lock2D(_2dbuffer, &data, &pitch);
    ok(hr == HRESULT_FROM_WIN32(ERROR_WAS_LOCKED), "Unexpected hr %#lx.\n", hr);
    hr = IMF2DBuffer2_Unlock2D(_2dbuffer2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMF2DBuffer2_Unlock2D(_2dbuffer2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMF2DBuffer2_Unlock2D(_2dbuffer2);
    ok(hr == HRESULT_FROM_WIN32(ERROR_WAS_UNLOCKED), "Unexpected hr %#lx.\n", hr);

    IMF2DBuffer2_Release(_2dbuffer2);

    IMFMediaBuffer_Release(buffer);
    IDirect3DDevice9_Release(device);

done:
    if (backbuffer)
        IDirect3DSurface9_Release(backbuffer);
    ok(!IDirect3D9_Release(d3d), "Unexpected refcount.\n");
    DestroyWindow(window);
}

static void test_MFCreateTrackedSample(void)
{
    IMFTrackedSample *tracked_sample;
    IMFSample *sample;
    IUnknown *unk;
    HRESULT hr;

    if (!pMFCreateTrackedSample)
    {
        win_skip("MFCreateTrackedSample() is not available.\n");
        return;
    }

    hr = pMFCreateTrackedSample(&tracked_sample);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* It's actually a sample. */
    hr = IMFTrackedSample_QueryInterface(tracked_sample, &IID_IMFSample, (void **)&sample);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFTrackedSample_QueryInterface(tracked_sample, &IID_IUnknown, (void **)&unk);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(unk == (IUnknown *)sample, "Unexpected pointer.\n");
    IUnknown_Release(unk);

    IMFSample_Release(sample);

    check_interface(tracked_sample, &IID_IMFDesiredSample, FALSE);

    IMFTrackedSample_Release(tracked_sample);
}

static void test_MFFrameRateToAverageTimePerFrame(void)
{
    static const struct frame_rate_test
    {
        unsigned int numerator;
        unsigned int denominator;
        UINT64 avgtime;
    } frame_rate_tests[] =
    {
        { 60000, 1001, 166833 },
        { 30000, 1001, 333667 },
        { 24000, 1001, 417188 },
        { 60, 1, 166667 },
        { 30, 1, 333333 },
        { 50, 1, 200000 },
        { 25, 1, 400000 },
        { 24, 1, 416667 },

        { 39, 1, 256410 },
        { 120, 1, 83333 },
    };
    unsigned int i;
    UINT64 avgtime;
    HRESULT hr;

    avgtime = 1;
    hr = MFFrameRateToAverageTimePerFrame(0, 0, &avgtime);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!avgtime, "Unexpected frame time.\n");

    avgtime = 1;
    hr = MFFrameRateToAverageTimePerFrame(0, 1001, &avgtime);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!avgtime, "Unexpected frame time.\n");

    for (i = 0; i < ARRAY_SIZE(frame_rate_tests); ++i)
    {
        avgtime = 0;
        hr = MFFrameRateToAverageTimePerFrame(frame_rate_tests[i].numerator,
                frame_rate_tests[i].denominator, &avgtime);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(avgtime == frame_rate_tests[i].avgtime, "%u: unexpected frame time %s, expected %s.\n",
                i, wine_dbgstr_longlong(avgtime), wine_dbgstr_longlong(frame_rate_tests[i].avgtime));
    }
}

static void test_MFAverageTimePerFrameToFrameRate(void)
{
    static const struct frame_rate_test
    {
        unsigned int numerator;
        unsigned int denominator;
        UINT64 avgtime;
    } frame_rate_tests[] =
    {
        { 60000, 1001, 166863 },
        { 60000, 1001, 166833 },
        { 60000, 1001, 166803 },

        { 30000, 1001, 333697 },
        { 30000, 1001, 333667 },
        { 30000, 1001, 333637 },

        { 24000, 1001, 417218 },
        { 24000, 1001, 417188 },
        { 24000, 1001, 417158 },

        { 60, 1, 166697 },
        { 60, 1, 166667 },
        { 60, 1, 166637 },

        { 30, 1, 333363 },
        { 30, 1, 333333 },
        { 30, 1, 333303 },

        { 50, 1, 200030 },
        { 50, 1, 200000 },
        { 50, 1, 199970 },

        { 25, 1, 400030 },
        { 25, 1, 400000 },
        { 25, 1, 399970 },

        { 24, 1, 416697 },
        { 24, 1, 416667 },
        { 24, 1, 416637 },

        { 1000000, 25641, 256410 },
        { 10000000, 83333, 83333 },
        { 1, 10, 100000000 },
        { 1, 10, 100000001 },
        { 1, 10, 200000000 },
        { 1,  1,  10000000 },
        { 1,  2,  20000000 },
        { 5,  1,   2000000 },
        { 10, 1,   1000000 },
    };
    unsigned int i, numerator, denominator;
    HRESULT hr;

    numerator = denominator = 1;
    hr = MFAverageTimePerFrameToFrameRate(0, &numerator, &denominator);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!numerator && !denominator, "Unexpected output %u/%u.\n", numerator, denominator);

    for (i = 0; i < ARRAY_SIZE(frame_rate_tests); ++i)
    {
        numerator = denominator = 12345;
        hr = MFAverageTimePerFrameToFrameRate(frame_rate_tests[i].avgtime, &numerator, &denominator);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(numerator == frame_rate_tests[i].numerator && denominator == frame_rate_tests[i].denominator,
                "%u: unexpected %u/%u, expected %u/%u.\n", i, numerator, denominator, frame_rate_tests[i].numerator,
                frame_rate_tests[i].denominator);
    }
}

static void test_MFMapDXGIFormatToDX9Format(void)
{
    static const struct format_pair
    {
        DXGI_FORMAT dxgi_format;
        DWORD d3d9_format;
        BOOL broken;
    }
    formats_map[] =
    {
        { DXGI_FORMAT_R32G32B32A32_FLOAT, D3DFMT_A32B32G32R32F },
        { DXGI_FORMAT_R16G16B16A16_FLOAT, D3DFMT_A16B16G16R16F },
        { DXGI_FORMAT_R16G16B16A16_UNORM, D3DFMT_A16B16G16R16 },
        { DXGI_FORMAT_R16G16B16A16_SNORM, D3DFMT_Q16W16V16U16 },
        { DXGI_FORMAT_R32G32_FLOAT, D3DFMT_G32R32F },
        { DXGI_FORMAT_R10G10B10A2_UNORM, D3DFMT_A2B10G10R10 },
        { DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, D3DFMT_A8R8G8B8 },
        { DXGI_FORMAT_R8G8B8A8_SNORM, D3DFMT_Q8W8V8U8 },
        { DXGI_FORMAT_R16G16_FLOAT, D3DFMT_G16R16F },
        { DXGI_FORMAT_R16G16_UNORM, D3DFMT_G16R16 },
        { DXGI_FORMAT_R16G16_SNORM, D3DFMT_V16U16 },
        { DXGI_FORMAT_D32_FLOAT, D3DFMT_D32F_LOCKABLE },
        { DXGI_FORMAT_R32_FLOAT, D3DFMT_R32F },
        { DXGI_FORMAT_D24_UNORM_S8_UINT, D3DFMT_D24S8 },
        { DXGI_FORMAT_R8G8_SNORM, D3DFMT_V8U8 },
        { DXGI_FORMAT_R16_FLOAT, D3DFMT_R16F },
        { DXGI_FORMAT_R16_UNORM, D3DFMT_L16 },
        { DXGI_FORMAT_R8_UNORM, D3DFMT_L8 },
        { DXGI_FORMAT_A8_UNORM, D3DFMT_A8 },
        { DXGI_FORMAT_BC1_UNORM, D3DFMT_DXT1 },
        { DXGI_FORMAT_BC1_UNORM_SRGB, D3DFMT_DXT1 },
        { DXGI_FORMAT_BC2_UNORM, D3DFMT_DXT2 },
        { DXGI_FORMAT_BC2_UNORM_SRGB, D3DFMT_DXT2 },
        { DXGI_FORMAT_BC3_UNORM, D3DFMT_DXT4 },
        { DXGI_FORMAT_BC3_UNORM_SRGB, D3DFMT_DXT4 },
        { DXGI_FORMAT_B8G8R8A8_UNORM, D3DFMT_A8R8G8B8 },
        { DXGI_FORMAT_B8G8R8X8_UNORM, D3DFMT_X8R8G8B8 },
        { DXGI_FORMAT_B8G8R8A8_UNORM_SRGB, D3DFMT_A8R8G8B8 },
        { DXGI_FORMAT_B8G8R8X8_UNORM_SRGB, D3DFMT_X8R8G8B8 },
        { DXGI_FORMAT_R8G8B8A8_UNORM, D3DFMT_A8B8G8R8, .broken = TRUE /* <= w1064v1507 */ },
        { DXGI_FORMAT_AYUV, MAKEFOURCC('A','Y','U','V') },
        { DXGI_FORMAT_Y410, MAKEFOURCC('Y','4','1','0') },
        { DXGI_FORMAT_Y416, MAKEFOURCC('Y','4','1','6') },
        { DXGI_FORMAT_NV12, MAKEFOURCC('N','V','1','2') },
        { DXGI_FORMAT_P010, MAKEFOURCC('P','0','1','0') },
        { DXGI_FORMAT_P016, MAKEFOURCC('P','0','1','6') },
        { DXGI_FORMAT_420_OPAQUE, MAKEFOURCC('4','2','0','O') },
        { DXGI_FORMAT_YUY2, D3DFMT_YUY2 },
        { DXGI_FORMAT_Y210, MAKEFOURCC('Y','2','1','0') },
        { DXGI_FORMAT_Y216, MAKEFOURCC('Y','2','1','6') },
        { DXGI_FORMAT_NV11, MAKEFOURCC('N','V','1','1') },
        { DXGI_FORMAT_AI44, MAKEFOURCC('A','I','4','4') },
        { DXGI_FORMAT_IA44, MAKEFOURCC('I','A','4','4') },
        { DXGI_FORMAT_P8, D3DFMT_P8 },
        { DXGI_FORMAT_A8P8, D3DFMT_A8P8 },
    };
    unsigned int i;
    DWORD format;

    if (!pMFMapDXGIFormatToDX9Format)
    {
        win_skip("MFMapDXGIFormatToDX9Format is not available.\n");
        return;
    }

    for (i = 0; i < ARRAY_SIZE(formats_map); ++i)
    {
        format = pMFMapDXGIFormatToDX9Format(formats_map[i].dxgi_format);
        ok(format == formats_map[i].d3d9_format || broken(formats_map[i].broken && format == 0),
                "Unexpected d3d9 format %#lx, dxgi format %#x.\n", format, formats_map[i].dxgi_format);
    }
}

static void test_MFMapDX9FormatToDXGIFormat(void)
{
    static const struct format_pair
    {
        DXGI_FORMAT dxgi_format;
        DWORD d3d9_format;
        BOOL broken;
    }
    formats_map[] =
    {
        { DXGI_FORMAT_R32G32B32A32_FLOAT, D3DFMT_A32B32G32R32F },
        { DXGI_FORMAT_R16G16B16A16_FLOAT, D3DFMT_A16B16G16R16F },
        { DXGI_FORMAT_R16G16B16A16_UNORM, D3DFMT_A16B16G16R16 },
        { DXGI_FORMAT_R16G16B16A16_SNORM, D3DFMT_Q16W16V16U16 },
        { DXGI_FORMAT_R32G32_FLOAT, D3DFMT_G32R32F },
        { DXGI_FORMAT_R10G10B10A2_UNORM, D3DFMT_A2B10G10R10 },
        { 0, D3DFMT_A2R10G10B10 }, /* doesn't map to DXGI */
        { DXGI_FORMAT_R8G8B8A8_SNORM, D3DFMT_Q8W8V8U8 },
        { DXGI_FORMAT_R16G16_FLOAT, D3DFMT_G16R16F },
        { DXGI_FORMAT_R16G16_UNORM, D3DFMT_G16R16 },
        { DXGI_FORMAT_R16G16_SNORM, D3DFMT_V16U16 },
        { DXGI_FORMAT_D32_FLOAT, D3DFMT_D32F_LOCKABLE },
        { DXGI_FORMAT_R32_FLOAT, D3DFMT_R32F },
        { DXGI_FORMAT_D24_UNORM_S8_UINT, D3DFMT_D24S8 },
        { DXGI_FORMAT_R8G8_SNORM, D3DFMT_V8U8 },
        { DXGI_FORMAT_R16_FLOAT, D3DFMT_R16F },
        { DXGI_FORMAT_R16_UNORM, D3DFMT_L16 },
        { DXGI_FORMAT_R8_UNORM, D3DFMT_L8 },
        { DXGI_FORMAT_A8_UNORM, D3DFMT_A8 },
        { DXGI_FORMAT_BC1_UNORM, D3DFMT_DXT1 },
        { DXGI_FORMAT_BC2_UNORM, D3DFMT_DXT2 },
        { DXGI_FORMAT_BC3_UNORM, D3DFMT_DXT4 },
        { DXGI_FORMAT_B8G8R8A8_UNORM, D3DFMT_A8R8G8B8 },
        { DXGI_FORMAT_B8G8R8X8_UNORM, D3DFMT_X8R8G8B8 },
        { DXGI_FORMAT_R8G8B8A8_UNORM, D3DFMT_A8B8G8R8, .broken = TRUE },
        { DXGI_FORMAT_AYUV, MAKEFOURCC('A','Y','U','V') },
        { DXGI_FORMAT_Y410, MAKEFOURCC('Y','4','1','0') },
        { DXGI_FORMAT_Y416, MAKEFOURCC('Y','4','1','6') },
        { DXGI_FORMAT_NV12, MAKEFOURCC('N','V','1','2') },
        { DXGI_FORMAT_P010, MAKEFOURCC('P','0','1','0') },
        { DXGI_FORMAT_P016, MAKEFOURCC('P','0','1','6') },
        { DXGI_FORMAT_420_OPAQUE, MAKEFOURCC('4','2','0','O') },
        { DXGI_FORMAT_YUY2, D3DFMT_YUY2 },
        { DXGI_FORMAT_Y210, MAKEFOURCC('Y','2','1','0') },
        { DXGI_FORMAT_Y216, MAKEFOURCC('Y','2','1','6') },
        { DXGI_FORMAT_NV11, MAKEFOURCC('N','V','1','1') },
        { DXGI_FORMAT_AI44, MAKEFOURCC('A','I','4','4') },
        { DXGI_FORMAT_IA44, MAKEFOURCC('I','A','4','4') },
        { DXGI_FORMAT_P8, D3DFMT_P8 },
        { DXGI_FORMAT_A8P8, D3DFMT_A8P8 },
    };
    DXGI_FORMAT format;
    unsigned int i;

    if (!pMFMapDX9FormatToDXGIFormat)
    {
        win_skip("MFMapDX9FormatToDXGIFormat() is not available.\n");
        return;
    }

    for (i = 0; i < ARRAY_SIZE(formats_map); ++i)
    {
        format = pMFMapDX9FormatToDXGIFormat(formats_map[i].d3d9_format);
        ok(format == formats_map[i].dxgi_format || broken(formats_map[i].broken && format == 0),
                "Unexpected DXGI format %#x, d3d9 format %#lx.\n", format, formats_map[i].d3d9_format);
    }
}

static HRESULT WINAPI test_notify_callback_QueryInterface(IMFVideoSampleAllocatorNotify *iface,
        REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IMFVideoSampleAllocatorNotify) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFVideoSampleAllocatorNotify_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI test_notify_callback_AddRef(IMFVideoSampleAllocatorNotify *iface)
{
    return 2;
}

static ULONG WINAPI test_notify_callback_Release(IMFVideoSampleAllocatorNotify *iface)
{
    return 1;
}

static HRESULT WINAPI test_notify_callback_NotifyRelease(IMFVideoSampleAllocatorNotify *iface)
{
    return E_NOTIMPL;
}

static const IMFVideoSampleAllocatorNotifyVtbl test_notify_callback_vtbl =
{
    test_notify_callback_QueryInterface,
    test_notify_callback_AddRef,
    test_notify_callback_Release,
    test_notify_callback_NotifyRelease,
};

static IMFMediaType * create_video_type(const GUID *subtype)
{
    IMFMediaType *video_type;
    HRESULT hr;

    hr = MFCreateMediaType(&video_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaType_SetGUID(video_type, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaType_SetGUID(video_type, &MF_MT_SUBTYPE, subtype);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    return video_type;
}

static ID3D11Device *create_d3d11_device(void)
{
    static const D3D_FEATURE_LEVEL default_feature_level[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    const D3D_FEATURE_LEVEL *feature_level;
    unsigned int feature_level_count;
    ID3D11Device *device;

    feature_level = default_feature_level;
    feature_level_count = ARRAY_SIZE(default_feature_level);

    if (SUCCEEDED(pD3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0,
            feature_level, feature_level_count, D3D11_SDK_VERSION, &device, NULL, NULL)))
        return device;
    if (SUCCEEDED(pD3D11CreateDevice(NULL, D3D_DRIVER_TYPE_WARP, NULL, 0,
            feature_level, feature_level_count, D3D11_SDK_VERSION, &device, NULL, NULL)))
        return device;
    if (SUCCEEDED(pD3D11CreateDevice(NULL, D3D_DRIVER_TYPE_REFERENCE, NULL, 0,
            feature_level, feature_level_count, D3D11_SDK_VERSION, &device, NULL, NULL)))
        return device;

    return NULL;
}

static void update_d3d11_texture(ID3D11Texture2D *texture, unsigned int sub_resource_idx,
        const BYTE *data, unsigned int src_pitch)
{
    ID3D11DeviceContext *immediate_context;
    ID3D11Device *device;

    ID3D11Texture2D_GetDevice(texture, &device);
    ID3D11Device_GetImmediateContext(device, &immediate_context);

    ID3D11DeviceContext_UpdateSubresource(immediate_context, (ID3D11Resource *)texture,
            sub_resource_idx, NULL, data, src_pitch, 0);

    ID3D11DeviceContext_Release(immediate_context);
    ID3D11Device_Release(device);
}

static ID3D12Device *create_d3d12_device(void)
{
    ID3D12Device *device;
    HRESULT hr;

    if (!pD3D12CreateDevice) return NULL;

    hr = pD3D12CreateDevice(NULL, D3D_FEATURE_LEVEL_11_0, &IID_ID3D12Device, (void **)&device);
    if (FAILED(hr))
        return NULL;

    return device;
}

static void test_d3d11_surface_buffer(void)
{
    DWORD max_length, cur_length, length, color;
    BYTE *data, *data2, *buffer_start;
    IMFDXGIBuffer *dxgi_buffer;
    D3D11_TEXTURE2D_DESC desc;
    IMF2DBuffer2 *_2dbuffer2;
    ID3D11Texture2D *texture;
    IMF2DBuffer *_2d_buffer;
    IMFMediaBuffer *buffer;
    ID3D11Device *device;
    BYTE buff[64 * 64 * 4];
    LONG pitch, pitch2;
    UINT index, size;
    IUnknown *obj;
    HRESULT hr;

    if (!pMFCreateDXGISurfaceBuffer)
    {
        win_skip("MFCreateDXGISurfaceBuffer() is not available.\n");
        return;
    }

    /* d3d11 */
    if (!(device = create_d3d11_device()))
    {
        skip("Failed to create a D3D11 device, skipping tests.\n");
        return;
    }

    memset(&desc, 0, sizeof(desc));
    desc.Width = 64;
    desc.Height = 64;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;

    hr = ID3D11Device_CreateTexture2D(device, &desc, NULL, &texture);
    ok(hr == S_OK, "Failed to create a texture, hr %#lx.\n", hr);

    hr = pMFCreateDXGISurfaceBuffer(&IID_ID3D11Texture2D, (IUnknown *)texture, 0, FALSE, &buffer);
    ok(hr == S_OK, "Failed to create a buffer, hr %#lx.\n", hr);

    check_interface(buffer, &IID_IMF2DBuffer, TRUE);
    check_interface(buffer, &IID_IMF2DBuffer2, TRUE);
    check_interface(buffer, &IID_IMFDXGIBuffer, TRUE);
    check_interface(buffer, &IID_IMFGetService, FALSE);

    max_length = 0;
    hr = IMFMediaBuffer_GetMaxLength(buffer, &max_length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!max_length, "Unexpected length %lu.\n", max_length);

    hr = IMFMediaBuffer_GetCurrentLength(buffer, &cur_length);
    ok(hr == S_OK, "Failed to get length, hr %#lx.\n", hr);
    ok(!cur_length, "Unexpected length %lu.\n", cur_length);

    hr = IMFMediaBuffer_SetCurrentLength(buffer, 2 * max_length);
    ok(hr == S_OK, "Failed to get length, hr %#lx.\n", hr);

    hr = IMFMediaBuffer_GetCurrentLength(buffer, &cur_length);
    ok(hr == S_OK, "Failed to get length, hr %#lx.\n", hr);
    ok(cur_length == 2 * max_length, "Unexpected length %lu.\n", cur_length);

    hr = IMFMediaBuffer_QueryInterface(buffer, &IID_IMF2DBuffer, (void **)&_2d_buffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMF2DBuffer_GetContiguousLength(_2d_buffer, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);
    hr = IMF2DBuffer_GetContiguousLength(_2d_buffer, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(length == max_length, "Unexpected length %lu.\n", length);
    IMF2DBuffer_Release(_2d_buffer);

    hr = IMFMediaBuffer_QueryInterface(buffer, &IID_IMFDXGIBuffer, (void **)&dxgi_buffer);
    ok(hr == S_OK, "Failed to get interface, hr %#lx.\n", hr);

    EXPECT_REF(texture, 2);
    hr = IMFDXGIBuffer_GetResource(dxgi_buffer, &IID_ID3D11Texture2D, (void **)&obj);
    ok(hr == S_OK, "Failed to get resource, hr %#lx.\n", hr);
    EXPECT_REF(texture, 3);
    ok(obj == (IUnknown *)texture, "Unexpected resource pointer.\n");
    IUnknown_Release(obj);

    hr = IMFDXGIBuffer_GetSubresourceIndex(dxgi_buffer, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMFDXGIBuffer_GetSubresourceIndex(dxgi_buffer, &index);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(index == 0, "Unexpected subresource index.\n");

    hr = IMFDXGIBuffer_SetUnknown(dxgi_buffer, &IID_IMFDXGIBuffer, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFDXGIBuffer_SetUnknown(dxgi_buffer, &IID_IMFDXGIBuffer, (void *)device);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFDXGIBuffer_SetUnknown(dxgi_buffer, &IID_IMFDXGIBuffer, (void *)device);
    ok(hr == HRESULT_FROM_WIN32(ERROR_OBJECT_ALREADY_EXISTS), "Unexpected hr %#lx.\n", hr);

    hr = ID3D11Texture2D_GetPrivateData(texture, &IID_IMFDXGIBuffer, &size, &data);
    ok(hr == DXGI_ERROR_NOT_FOUND, "Unexpected hr %#lx.\n", hr);

    hr = IMFDXGIBuffer_GetUnknown(dxgi_buffer, &IID_IMFDXGIBuffer, &IID_ID3D11Device, (void **)&obj);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(obj == (IUnknown *)device, "Unexpected pointer.\n");
    IUnknown_Release(obj);

    hr = IMFDXGIBuffer_SetUnknown(dxgi_buffer, &IID_IMFDXGIBuffer, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFDXGIBuffer_GetUnknown(dxgi_buffer, &IID_IMFDXGIBuffer, &IID_IUnknown, (void **)&obj);
    ok(hr == MF_E_NOT_FOUND, "Unexpected hr %#lx.\n", hr);

    IMFDXGIBuffer_Release(dxgi_buffer);

    /* Texture updates. */
    color = get_d3d11_texture_color(texture, 0, 0);
    ok(!color, "Unexpected texture color %#lx.\n", color);

    max_length = cur_length = 0;
    data = NULL;
    hr = IMFMediaBuffer_Lock(buffer, &data, &max_length, &cur_length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(max_length && max_length == cur_length, "Unexpected length %lu.\n", max_length);
    if (data) *(DWORD *)data = ~0u;

    color = get_d3d11_texture_color(texture, 0, 0);
    ok(!color, "Unexpected texture color %#lx.\n", color);

    hr = IMFMediaBuffer_Unlock(buffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    color = get_d3d11_texture_color(texture, 0, 0);
    ok(color == ~0u, "Unexpected texture color %#lx.\n", color);

    hr = IMFMediaBuffer_Lock(buffer, &data, &max_length, &cur_length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(*(DWORD *)data == ~0u, "Unexpected buffer %#lx.\n", *(DWORD *)data);

    hr = IMFMediaBuffer_Unlock(buffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Lock2D()/Unlock2D() */
    hr = IMFMediaBuffer_QueryInterface(buffer, &IID_IMF2DBuffer, (void **)&_2d_buffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMF2DBuffer_GetScanline0AndPitch(_2d_buffer, &data2, &pitch2);
    ok(hr == HRESULT_FROM_WIN32(ERROR_WAS_UNLOCKED), "Unexpected hr %#lx.\n", hr);

    hr = IMF2DBuffer_Lock2D(_2d_buffer, &data, &pitch);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!data && pitch == desc.Width * 4, "Unexpected pitch %ld.\n", pitch);

    hr = IMF2DBuffer_Lock2D(_2d_buffer, &data, &pitch);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!data && pitch == desc.Width * 4, "Unexpected pitch %ld.\n", pitch);

    hr = IMF2DBuffer_GetScanline0AndPitch(_2d_buffer, &data2, &pitch2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(data2 == data && pitch2 == pitch, "Unexpected data/pitch.\n");

    hr = IMFMediaBuffer_Lock(buffer, &data, &max_length, &cur_length);
    ok(hr == MF_E_INVALIDREQUEST, "Unexpected hr %#lx.\n", hr);

    hr = IMF2DBuffer_Unlock2D(_2d_buffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMF2DBuffer_Unlock2D(_2d_buffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMF2DBuffer_Unlock2D(_2d_buffer);
    ok(hr == HRESULT_FROM_WIN32(ERROR_WAS_UNLOCKED), "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaBuffer_Lock(buffer, &data, NULL, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMF2DBuffer_Lock2D(_2d_buffer, &data, &pitch);
    ok(hr == MF_E_UNEXPECTED, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaBuffer_Unlock(buffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IMF2DBuffer_Release(_2d_buffer);

    hr = IMFMediaBuffer_QueryInterface(buffer, &IID_IMF2DBuffer2, (void **)&_2dbuffer2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Lock flags are honored, so reads and writes are discarded if
     * the flags are not correct. Also, previous content is discarded
     * when locking for writing and not for reading. */
    put_d3d11_texture_color(texture, 0, 0, 0xcdcdcdcd);
    hr = IMF2DBuffer2_Lock2DSize(_2dbuffer2, MF2DBuffer_LockFlags_Read, &data, &pitch, &data2, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(data == data2, "Unexpected scanline pointer.\n");
    ok(*(DWORD *)data == 0xcdcdcdcd, "Unexpected leading dword %#lx.\n", *(DWORD *)data);
    memset(data, 0xab, 4);
    IMF2DBuffer2_Unlock2D(_2dbuffer2);

    color = get_d3d11_texture_color(texture, 0, 0);
    ok(color == 0xcdcdcdcd, "Unexpected leading dword %#lx.\n", color);
    put_d3d11_texture_color(texture, 0, 0, 0xefefefef);

    hr = IMF2DBuffer2_Lock2DSize(_2dbuffer2, MF2DBuffer_LockFlags_Write, &data, &pitch, &data2, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(*(DWORD *)data != 0xefefefef, "Unexpected leading dword.\n");
    IMF2DBuffer2_Unlock2D(_2dbuffer2);

    color = get_d3d11_texture_color(texture, 0, 0);
    ok(color != 0xefefefef, "Unexpected leading dword.\n");

    hr = IMF2DBuffer2_Lock2DSize(_2dbuffer2, MF2DBuffer_LockFlags_Write, &data, &pitch, &data2, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(*(DWORD *)data != 0xefefefef, "Unexpected leading dword.\n");
    memset(data, 0x89, 4);
    IMF2DBuffer2_Unlock2D(_2dbuffer2);

    color = get_d3d11_texture_color(texture, 0, 0);
    ok(color == 0x89898989, "Unexpected leading dword %#lx.\n", color);

    /* When relocking for writing, stores are committed even if they
     * were issued before relocking. */
    put_d3d11_texture_color(texture, 0, 0, 0xcdcdcdcd);
    hr = IMF2DBuffer2_Lock2DSize(_2dbuffer2, MF2DBuffer_LockFlags_Read, &data, &pitch, &data2, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    memset(data, 0xab, 4);
    hr = IMF2DBuffer2_Lock2DSize(_2dbuffer2, MF2DBuffer_LockFlags_Write, &data, &pitch, &data2, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMF2DBuffer2_Unlock2D(_2dbuffer2);
    IMF2DBuffer2_Unlock2D(_2dbuffer2);

    color = get_d3d11_texture_color(texture, 0, 0);
    ok(color == 0xabababab, "Unexpected leading dword %#lx.\n", color);

    /* Flags incompatibilities. */
    hr = IMF2DBuffer2_Lock2DSize(_2dbuffer2, MF2DBuffer_LockFlags_ReadWrite, &data, &pitch, &data2, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMF2DBuffer2_Lock2DSize(_2dbuffer2, MF2DBuffer_LockFlags_ReadWrite, &data, &pitch, &data2, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMF2DBuffer2_Lock2DSize(_2dbuffer2, MF2DBuffer_LockFlags_Read, &data, &pitch, &data2, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMF2DBuffer2_Lock2DSize(_2dbuffer2, MF2DBuffer_LockFlags_Write, &data, &pitch, &data2, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMF2DBuffer_Lock2D(_2d_buffer, &data, &pitch);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMF2DBuffer2_Unlock2D(_2dbuffer2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMF2DBuffer2_Unlock2D(_2dbuffer2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMF2DBuffer2_Unlock2D(_2dbuffer2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMF2DBuffer2_Unlock2D(_2dbuffer2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMF2DBuffer2_Unlock2D(_2dbuffer2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMF2DBuffer2_Lock2DSize(_2dbuffer2, MF2DBuffer_LockFlags_Read, &data, &pitch, &data2, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMF2DBuffer2_Lock2DSize(_2dbuffer2, MF2DBuffer_LockFlags_Read, &data, &pitch, &data2, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMF2DBuffer2_Lock2DSize(_2dbuffer2, MF2DBuffer_LockFlags_ReadWrite, &data, &pitch, &data2, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMF2DBuffer2_Lock2DSize(_2dbuffer2, MF2DBuffer_LockFlags_Write, &data, &pitch, &data2, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMF2DBuffer_Lock2D(_2d_buffer, &data, &pitch);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMF2DBuffer2_Unlock2D(_2dbuffer2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMF2DBuffer2_Unlock2D(_2dbuffer2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMF2DBuffer2_Unlock2D(_2dbuffer2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMF2DBuffer2_Unlock2D(_2dbuffer2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMF2DBuffer2_Unlock2D(_2dbuffer2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Except when originally locking for writing. */
    hr = IMF2DBuffer2_Lock2DSize(_2dbuffer2, MF2DBuffer_LockFlags_Write, &data, &pitch, &data2, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMF2DBuffer2_Lock2DSize(_2dbuffer2, MF2DBuffer_LockFlags_Write, &data, &pitch, &data2, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMF2DBuffer2_Lock2DSize(_2dbuffer2, MF2DBuffer_LockFlags_ReadWrite, &data, &pitch, &data2, &length);
    ok(hr == HRESULT_FROM_WIN32(ERROR_WAS_LOCKED), "Unexpected hr %#lx.\n", hr);
    hr = IMF2DBuffer2_Lock2DSize(_2dbuffer2, MF2DBuffer_LockFlags_Read, &data, &pitch, &data2, &length);
    ok(hr == HRESULT_FROM_WIN32(ERROR_WAS_LOCKED), "Unexpected hr %#lx.\n", hr);
    hr = IMF2DBuffer_Lock2D(_2d_buffer, &data, &pitch);
    ok(hr == HRESULT_FROM_WIN32(ERROR_WAS_LOCKED), "Unexpected hr %#lx.\n", hr);
    hr = IMF2DBuffer2_Unlock2D(_2dbuffer2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMF2DBuffer2_Unlock2D(_2dbuffer2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMF2DBuffer2_Unlock2D(_2dbuffer2);
    ok(hr == HRESULT_FROM_WIN32(ERROR_WAS_UNLOCKED), "Unexpected hr %#lx.\n", hr);

    IMF2DBuffer2_Release(_2dbuffer2);
    IMFMediaBuffer_Release(buffer);

    /* Bottom up. */
    hr = pMFCreateDXGISurfaceBuffer(&IID_ID3D11Texture2D, (IUnknown *)texture, 0, TRUE, &buffer);
    ok(hr == S_OK, "Failed to create a buffer, hr %#lx.\n", hr);

    hr = IMFMediaBuffer_QueryInterface(buffer, &IID_IMF2DBuffer, (void **)&_2d_buffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMF2DBuffer_Lock2D(_2d_buffer, &data, &pitch);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!data && pitch == desc.Width * 4, "Unexpected pitch %ld.\n", pitch);

    hr = IMF2DBuffer_GetScanline0AndPitch(_2d_buffer, &data2, &pitch2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(data2 == data && pitch2 == pitch, "Unexpected data/pitch.\n");

    hr = IMF2DBuffer_Unlock2D(_2d_buffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IMF2DBuffer_Release(_2d_buffer);
    IMFMediaBuffer_Release(buffer);

    ID3D11Texture2D_Release(texture);

    memset(&desc, 0, sizeof(desc));
    desc.Width = 64;
    desc.Height = 64;
    desc.ArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_NV12;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;

    hr = ID3D11Device_CreateTexture2D(device, &desc, NULL, &texture);
    if (SUCCEEDED(hr))
    {
        hr = pMFCreateDXGISurfaceBuffer(&IID_ID3D11Texture2D, (IUnknown *)texture, 0, FALSE, &buffer);
        ok(hr == S_OK, "got %#lx.\n", hr);
        hr = IMFMediaBuffer_QueryInterface(buffer, &IID_IMF2DBuffer2, (void **)&_2dbuffer2);
        ok(hr == S_OK, "got %#lx.\n", hr);

        hr = IMF2DBuffer2_Lock2DSize(_2dbuffer2, MF2DBuffer_LockFlags_Read, &data, &pitch, &buffer_start, &length);
        ok(hr == S_OK, "got %#lx.\n", hr);

        ok(pitch >= desc.Width, "got %ld.\n", pitch);
        ok(length == pitch * desc.Height * 3 / 2, "got %lu.\n", length);

        hr = IMF2DBuffer2_Unlock2D(_2dbuffer2);
        ok(hr == S_OK, "got %#lx.\n", hr);

        IMF2DBuffer2_Release(_2dbuffer2);
        IMFMediaBuffer_Release(buffer);
        ID3D11Texture2D_Release(texture);
    }
    else
    {
        win_skip("Failed to create NV12 texture, hr %#lx, skipping test.\n", hr);
        ID3D11Device_Release(device);
        return;
    }

    /* Subresource index 1.
     * When WARP d3d11 device is used, this test leaves the device in a broken state, so it should
     * be kept last. */
    memset(&desc, 0, sizeof(desc));
    desc.Width = 64;
    desc.Height = 64;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;

    hr = ID3D11Device_CreateTexture2D(device, &desc, NULL, &texture);
    ok(hr == S_OK, "Failed to create a texture, hr %#lx.\n", hr);

    hr = pMFCreateDXGISurfaceBuffer(&IID_ID3D11Texture2D, (IUnknown *)texture, 1, FALSE, &buffer);
    ok(hr == S_OK, "Failed to create a buffer, hr %#lx.\n", hr);

    hr = IMFMediaBuffer_QueryInterface(buffer, &IID_IMF2DBuffer, (void **)&_2d_buffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Pitch reflects top level. */
    memset(buff, 0, sizeof(buff));
    *(DWORD *)buff = 0xff00ff00;
    update_d3d11_texture(texture, 1, buff, 64 * 4);

    hr = IMF2DBuffer_Lock2D(_2d_buffer, &data, &pitch);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(pitch == desc.Width * 4, "Unexpected pitch %ld.\n", pitch);
    ok(*(DWORD *)data == 0xff00ff00, "Unexpected color %#lx.\n", *(DWORD *)data);

    hr = IMF2DBuffer_Unlock2D(_2d_buffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IMF2DBuffer_Release(_2d_buffer);
    IMFMediaBuffer_Release(buffer);
    ID3D11Texture2D_Release(texture);

    ID3D11Device_Release(device);
}

static void test_d3d12_surface_buffer(void)
{
    IMFDXGIBuffer *dxgi_buffer;
    D3D12_HEAP_PROPERTIES heap_props;
    D3D12_RESOURCE_DESC desc;
    ID3D12Resource *resource;
    IMFMediaBuffer *buffer;
    unsigned int refcount;
    ID3D12Device *device;
    IUnknown *obj;
    HRESULT hr;

    /* d3d12 */
    if (!(device = create_d3d12_device()))
    {
        skip("Failed to create a D3D12 device, skipping tests.\n");
        return;
    }

    memset(&heap_props, 0, sizeof(heap_props));
    heap_props.Type = D3D12_HEAP_TYPE_DEFAULT;

    memset(&desc, 0, sizeof(desc));
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Alignment = 0;
    desc.Width = 32;
    desc.Height = 32;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    hr = ID3D12Device_CreateCommittedResource(device, &heap_props, D3D12_HEAP_FLAG_NONE,
            &desc, D3D12_RESOURCE_STATE_RENDER_TARGET, NULL, &IID_ID3D12Resource, (void **)&resource);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = pMFCreateDXGISurfaceBuffer(&IID_ID3D12Resource, (IUnknown *)resource, 0, FALSE, &buffer);
    if (hr == E_INVALIDARG)
    {
        todo_wine
        win_skip("D3D12 resource buffers are not supported.\n");
        goto notsupported;
    }
    ok(hr == S_OK, "Failed to create a buffer, hr %#lx.\n", hr);

if (SUCCEEDED(hr))
{
    check_interface(buffer, &IID_IMF2DBuffer, TRUE);
    check_interface(buffer, &IID_IMF2DBuffer2, TRUE);
    check_interface(buffer, &IID_IMFDXGIBuffer, TRUE);
    check_interface(buffer, &IID_IMFGetService, FALSE);

    hr = IMFMediaBuffer_QueryInterface(buffer, &IID_IMFDXGIBuffer, (void **)&dxgi_buffer);
    ok(hr == S_OK, "Failed to get interface, hr %#lx.\n", hr);

    hr = IMFDXGIBuffer_GetResource(dxgi_buffer, &IID_ID3D12Resource, (void **)&obj);
    ok(hr == S_OK, "Failed to get resource, hr %#lx.\n", hr);
    ok(obj == (IUnknown *)resource, "Unexpected resource pointer.\n");
    IUnknown_Release(obj);

    IMFDXGIBuffer_Release(dxgi_buffer);
    IMFMediaBuffer_Release(buffer);
}

notsupported:
    ID3D12Resource_Release(resource);
    refcount = ID3D12Device_Release(device);
    ok(!refcount, "Unexpected device refcount %u.\n", refcount);
}

static void test_sample_allocator_sysmem(void)
{
    IMFVideoSampleAllocatorNotify test_notify = { &test_notify_callback_vtbl };
    IMFMediaType *media_type, *video_type, *video_type2;
    IMFVideoSampleAllocatorCallback *allocator_cb;
    IMFVideoSampleAllocatorEx *allocatorex;
    IMFVideoSampleAllocator *allocator;
    IMFSample *sample, *sample2;
    IMFAttributes *attributes;
    IMFMediaBuffer *buffer;
    LONG refcount, count;
    DWORD buffer_count;
    IUnknown *unk;
    HRESULT hr;

    if (!pMFCreateVideoSampleAllocatorEx)
        return;

    hr = pMFCreateVideoSampleAllocatorEx(&IID_IUnknown, (void **)&unk);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    check_interface(unk, &IID_IMFVideoSampleAllocator, TRUE);
    check_interface(unk, &IID_IMFVideoSampleAllocatorEx, TRUE);
    check_interface(unk, &IID_IMFVideoSampleAllocatorCallback, TRUE);

    IUnknown_Release(unk);

    hr = pMFCreateVideoSampleAllocatorEx(&IID_IMFVideoSampleAllocator, (void **)&allocator);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoSampleAllocator_QueryInterface(allocator, &IID_IMFVideoSampleAllocatorCallback, (void **)&allocator_cb);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoSampleAllocatorCallback_SetCallback(allocator_cb, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoSampleAllocatorCallback_SetCallback(allocator_cb, &test_notify);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoSampleAllocatorCallback_SetCallback(allocator_cb, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoSampleAllocatorCallback_GetFreeSampleCount(allocator_cb, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    count = 10;
    hr = IMFVideoSampleAllocatorCallback_GetFreeSampleCount(allocator_cb, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!count, "Unexpected count %ld.\n", count);

    hr = IMFVideoSampleAllocator_UninitializeSampleAllocator(allocator);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoSampleAllocator_AllocateSample(allocator, &sample);
    ok(hr == MF_E_NOT_INITIALIZED, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoSampleAllocator_SetDirectXManager(allocator, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoSampleAllocator_InitializeSampleAllocator(allocator, 2, media_type);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    video_type = create_video_type(&MFVideoFormat_RGB32);
    video_type2 = create_video_type(&MFVideoFormat_RGB32);

    hr = IMFVideoSampleAllocator_InitializeSampleAllocator(allocator, 2, video_type);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    /* Frame size is required. */
    hr = IMFMediaType_SetUINT64(video_type, &MF_MT_FRAME_SIZE, (UINT64) 320 << 32 | 240);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaType_SetUINT64(video_type2, &MF_MT_FRAME_SIZE, (UINT64) 320 << 32 | 240);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoSampleAllocator_InitializeSampleAllocator(allocator, 0, video_type);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    EXPECT_REF(video_type, 1);
    hr = IMFVideoSampleAllocator_InitializeSampleAllocator(allocator, 1, video_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(video_type, 2);

    hr = IMFMediaType_SetUINT64(video_type2, &IID_IUnknown, (UINT64) 320 << 32 | 240);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Setting identical type does not replace it. */
    hr = IMFVideoSampleAllocator_InitializeSampleAllocator(allocator, 1, video_type2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(video_type, 2);
    EXPECT_REF(video_type2, 1);

    hr = IMFMediaType_SetUINT64(video_type2, &MF_MT_FRAME_SIZE, (UINT64) 64 << 32 | 64);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoSampleAllocator_InitializeSampleAllocator(allocator, 1, video_type2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(video_type2, 2);
    EXPECT_REF(video_type, 1);

    /* Modify referenced type. */
    hr = IMFMediaType_SetUINT64(video_type2, &MF_MT_FRAME_SIZE, (UINT64) 320 << 32 | 64);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoSampleAllocator_InitializeSampleAllocator(allocator, 1, video_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(video_type, 2);
    EXPECT_REF(video_type2, 1);

    count = 0;
    hr = IMFVideoSampleAllocatorCallback_GetFreeSampleCount(allocator_cb, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 1, "Unexpected count %ld.\n", count);

    sample = NULL;
    hr = IMFVideoSampleAllocator_AllocateSample(allocator, &sample);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    refcount = get_refcount(sample);

    hr = IMFVideoSampleAllocatorCallback_GetFreeSampleCount(allocator_cb, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!count, "Unexpected count %ld.\n", count);

    hr = IMFVideoSampleAllocator_AllocateSample(allocator, &sample2);
    ok(hr == MF_E_SAMPLEALLOCATOR_EMPTY, "Unexpected hr %#lx.\n", hr);

    /* Reinitialize with active sample. */
    hr = IMFVideoSampleAllocator_InitializeSampleAllocator(allocator, 2, video_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(refcount == get_refcount(sample), "Unexpected refcount %lu.\n", get_refcount(sample));
    EXPECT_REF(video_type, 2);

    hr = IMFVideoSampleAllocatorCallback_GetFreeSampleCount(allocator_cb, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine
    ok(!count, "Unexpected count %ld.\n", count);

    check_interface(sample, &IID_IMFTrackedSample, TRUE);
    check_interface(sample, &IID_IMFDesiredSample, FALSE);

    hr = IMFSample_GetBufferByIndex(sample, 0, &buffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    check_interface(buffer, &IID_IMF2DBuffer, TRUE);
    check_interface(buffer, &IID_IMF2DBuffer2, TRUE);
    check_interface(buffer, &IID_IMFGetService, TRUE);
    check_interface(buffer, &IID_IMFDXGIBuffer, FALSE);

    IMFMediaBuffer_Release(buffer);

    hr = IMFSample_GetBufferCount(sample, &buffer_count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(buffer_count == 1, "Unexpected buffer count %lu.\n", buffer_count);

    IMFSample_Release(sample);

    hr = IMFVideoSampleAllocator_UninitializeSampleAllocator(allocator);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine
    EXPECT_REF(video_type, 2);

    hr = IMFVideoSampleAllocatorCallback_GetFreeSampleCount(allocator_cb, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!count, "Unexpected count %ld.\n", count);

    hr = IMFVideoSampleAllocator_AllocateSample(allocator, &sample);
    ok(hr == MF_E_NOT_INITIALIZED, "Unexpected hr %#lx.\n", hr);

    IMFVideoSampleAllocatorCallback_Release(allocator_cb);
    IMFVideoSampleAllocator_Release(allocator);

    /* IMFVideoSampleAllocatorEx */
    hr = MFCreateAttributes(&attributes, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = pMFCreateVideoSampleAllocatorEx(&IID_IMFVideoSampleAllocatorEx, (void **)&allocatorex);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoSampleAllocatorEx_QueryInterface(allocatorex, &IID_IMFVideoSampleAllocatorCallback, (void **)&allocator_cb);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoSampleAllocatorEx_InitializeSampleAllocatorEx(allocatorex, 1, 0, NULL, video_type);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IMFAttributes_SetUINT32(attributes, &MF_SA_BUFFERS_PER_SAMPLE, 2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoSampleAllocatorEx_AllocateSample(allocatorex, &sample);
    ok(hr == MF_E_NOT_INITIALIZED, "Unexpected hr %#lx.\n", hr);

    EXPECT_REF(attributes, 1);
    hr = IMFVideoSampleAllocatorEx_InitializeSampleAllocatorEx(allocatorex, 0, 0, attributes, video_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(attributes, 2);

    count = 0;
    hr = IMFVideoSampleAllocatorCallback_GetFreeSampleCount(allocator_cb, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count == 1, "Unexpected count %ld.\n", count);

    hr = IMFVideoSampleAllocatorEx_AllocateSample(allocatorex, &sample);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFSample_GetBufferCount(sample, &buffer_count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(buffer_count == 2, "Unexpected buffer count %lu.\n", buffer_count);

    hr = IMFVideoSampleAllocatorEx_AllocateSample(allocatorex, &sample2);
    ok(hr == MF_E_SAMPLEALLOCATOR_EMPTY, "Unexpected hr %#lx.\n", hr);

    /* Reinitialize with already allocated samples. */
    hr = IMFVideoSampleAllocatorEx_InitializeSampleAllocatorEx(allocatorex, 0, 0, NULL, video_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(attributes, 1);

    hr = IMFVideoSampleAllocatorEx_AllocateSample(allocatorex, &sample2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMFSample_Release(sample2);

    IMFSample_Release(sample);

    IMFVideoSampleAllocatorCallback_Release(allocator_cb);
    IMFVideoSampleAllocatorEx_Release(allocatorex);
    IMFAttributes_Release(attributes);
}

static void test_sample_allocator_d3d9(void)
{
    IDirect3DDeviceManager9 *d3d9_manager;
    IMFVideoSampleAllocator *allocator;
    IDirect3DDevice9 *d3d9_device;
    IMFMediaType *video_type;
    IMFMediaBuffer *buffer;
    unsigned int token;
    IMFSample *sample;
    IDirect3D9 *d3d9;
    HWND window;
    HRESULT hr;

    if (!pMFCreateVideoSampleAllocatorEx)
        return;

    d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    if (!d3d9)
    {
        skip("Failed to create a D3D9 object, skipping tests.\n");
        return;
    }
    window = create_window();
    if (!(d3d9_device = create_d3d9_device(d3d9, window)))
    {
        skip("Failed to create a D3D9 device, skipping tests.\n");
        goto done;
    }

    hr = DXVA2CreateDirect3DDeviceManager9(&token, &d3d9_manager);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirect3DDeviceManager9_ResetDevice(d3d9_manager, d3d9_device, token);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = pMFCreateVideoSampleAllocatorEx(&IID_IMFVideoSampleAllocator, (void **)&allocator);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoSampleAllocator_SetDirectXManager(allocator, (IUnknown *)d3d9_manager);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    video_type = create_video_type(&MFVideoFormat_RGB32);

    /* Frame size is required. */
    hr = IMFMediaType_SetUINT64(video_type, &MF_MT_FRAME_SIZE, (UINT64) 64 << 32 | 64);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoSampleAllocator_InitializeSampleAllocator(allocator, 1, video_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoSampleAllocator_AllocateSample(allocator, &sample);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    check_interface(sample, &IID_IMFTrackedSample, TRUE);
    check_interface(sample, &IID_IMFDesiredSample, FALSE);

    hr = IMFSample_GetBufferByIndex(sample, 0, &buffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    check_interface(buffer, &IID_IMF2DBuffer, TRUE);
    check_interface(buffer, &IID_IMF2DBuffer2, TRUE);
    check_interface(buffer, &IID_IMFGetService, TRUE);
    check_interface(buffer, &IID_IMFDXGIBuffer, FALSE);

    IMFSample_Release(sample);
    IMFMediaBuffer_Release(buffer);

    IMFVideoSampleAllocator_Release(allocator);
    IMFMediaType_Release(video_type);
    IDirect3DDeviceManager9_Release(d3d9_manager);
    IDirect3DDevice9_Release(d3d9_device);

done:
    IDirect3D9_Release(d3d9);
    DestroyWindow(window);
}

static void test_sample_allocator_d3d11(void)
{
    IMFMediaType *video_type;
    IMFVideoSampleAllocatorEx *allocatorex;
    IMFVideoSampleAllocator *allocator;
    unsigned int i, token;
    IMFDXGIDeviceManager *manager;
    IMFSample *sample;
    IMFDXGIBuffer *dxgi_buffer;
    IMFAttributes *attributes;
    D3D11_TEXTURE2D_DESC desc;
    ID3D11Texture2D *texture;
    IMFMediaBuffer *buffer;
    ID3D11Device *device;
    HRESULT hr;
    BYTE *data;
    static const unsigned int usage[] =
    {
        D3D11_USAGE_DEFAULT,
        D3D11_USAGE_IMMUTABLE,
        D3D11_USAGE_DYNAMIC,
        D3D11_USAGE_STAGING,
        D3D11_USAGE_STAGING + 1,
    };
    static const unsigned int sharing[] =
    {
        D3D11_RESOURCE_MISC_SHARED | D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX,
        D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX,
        D3D11_RESOURCE_MISC_SHARED,
    };

    if (!pMFCreateVideoSampleAllocatorEx)
        return;

    if (!(device = create_d3d11_device()))
    {
        skip("Failed to create a D3D11 device, skipping tests.\n");
        return;
    }

    hr = pMFCreateDXGIDeviceManager(&token, &manager);
    ok(hr == S_OK, "Failed to create device manager, hr %#lx.\n", hr);

    hr = IMFDXGIDeviceManager_ResetDevice(manager, (IUnknown *)device, token);
    ok(hr == S_OK, "Failed to set a device, hr %#lx.\n", hr);

    hr = pMFCreateVideoSampleAllocatorEx(&IID_IMFVideoSampleAllocator, (void **)&allocator);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    EXPECT_REF(manager, 1);
    hr = IMFVideoSampleAllocator_SetDirectXManager(allocator, (IUnknown *)manager);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(manager, 2);

    video_type = create_video_type(&MFVideoFormat_RGB32);
    hr = IMFMediaType_SetUINT64(video_type, &MF_MT_FRAME_SIZE, (UINT64) 64 << 32 | 64);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoSampleAllocator_InitializeSampleAllocator(allocator, 1, video_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoSampleAllocator_AllocateSample(allocator, &sample);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFSample_GetBufferByIndex(sample, 0, &buffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    check_interface(buffer, &IID_IMF2DBuffer, TRUE);
    check_interface(buffer, &IID_IMF2DBuffer2, TRUE);
    check_interface(buffer, &IID_IMFDXGIBuffer, TRUE);
    check_interface(buffer, &IID_IMFGetService, FALSE);

    hr = IMFMediaBuffer_QueryInterface(buffer, &IID_IMFDXGIBuffer, (void **)&dxgi_buffer);
    ok(hr == S_OK, "Failed to get interface, hr %#lx.\n", hr);

    hr = IMFDXGIBuffer_GetResource(dxgi_buffer, &IID_ID3D11Texture2D, (void **)&texture);
    ok(hr == S_OK, "Failed to get resource, hr %#lx.\n", hr);

    ID3D11Texture2D_GetDesc(texture, &desc);
    ok(desc.Width == 64, "Unexpected width %u.\n", desc.Width);
    ok(desc.Height == 64, "Unexpected height %u.\n", desc.Height);
    ok(desc.MipLevels == 1, "Unexpected miplevels %u.\n", desc.MipLevels);
    ok(desc.ArraySize == 1, "Unexpected array size %u.\n", desc.ArraySize);
    ok(desc.Format == DXGI_FORMAT_B8G8R8X8_UNORM, "Unexpected format %u.\n", desc.Format);
    ok(desc.SampleDesc.Count == 1, "Unexpected sample count %u.\n", desc.SampleDesc.Count);
    ok(!desc.SampleDesc.Quality, "Unexpected sample quality %u.\n", desc.SampleDesc.Quality);
    ok(desc.Usage == D3D11_USAGE_DEFAULT, "Unexpected usage %u.\n", desc.Usage);
    ok(desc.BindFlags == (D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET), "Unexpected bind flags %#x.\n",
            desc.BindFlags);
    ok(!desc.CPUAccessFlags, "Unexpected CPU access flags %#x.\n", desc.CPUAccessFlags);
    ok(!desc.MiscFlags, "Unexpected misc flags %#x.\n", desc.MiscFlags);

    ID3D11Texture2D_Release(texture);
    IMFDXGIBuffer_Release(dxgi_buffer);

    hr = IMFMediaBuffer_Lock(buffer, &data, NULL, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaBuffer_Unlock(buffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IMFMediaBuffer_Release(buffer);
    IMFSample_Release(sample);

    IMFVideoSampleAllocator_Release(allocator);

    /* MF_SA_D3D11_USAGE */
    hr = MFCreateAttributes(&attributes, 1);
    ok(hr == S_OK, "Failed to create attributes, hr %#lx.\n", hr);

    for (i = 0; i < ARRAY_SIZE(usage); ++i)
    {
        hr = pMFCreateVideoSampleAllocatorEx(&IID_IMFVideoSampleAllocatorEx, (void **)&allocatorex);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IMFVideoSampleAllocatorEx_SetDirectXManager(allocatorex, (IUnknown *)manager);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IMFAttributes_SetUINT32(attributes, &MF_SA_D3D11_USAGE, usage[i]);
        ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

        hr = IMFVideoSampleAllocatorEx_InitializeSampleAllocatorEx(allocatorex, 0, 0, attributes, video_type);
        if (usage[i] == D3D11_USAGE_IMMUTABLE || usage[i] > D3D11_USAGE_STAGING)
        {
            ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
            IMFVideoSampleAllocatorEx_Release(allocatorex);
            continue;
        }
        ok(hr == S_OK, "%u: Unexpected hr %#lx.\n", usage[i], hr);

        hr = IMFAttributes_SetUINT32(attributes, &MF_SA_D3D11_USAGE, D3D11_USAGE_DEFAULT);
        ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

        hr = IMFVideoSampleAllocatorEx_AllocateSample(allocatorex, &sample);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IMFSample_GetBufferByIndex(sample, 0, &buffer);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IMFMediaBuffer_QueryInterface(buffer, &IID_IMFDXGIBuffer, (void **)&dxgi_buffer);
        ok(hr == S_OK, "Failed to get interface, hr %#lx.\n", hr);

        hr = IMFDXGIBuffer_GetResource(dxgi_buffer, &IID_ID3D11Texture2D, (void **)&texture);
        ok(hr == S_OK, "Failed to get resource, hr %#lx.\n", hr);

        ID3D11Texture2D_GetDesc(texture, &desc);
        ok(desc.Usage == usage[i], "Unexpected usage %u.\n", desc.Usage);
        if (usage[i] == D3D11_USAGE_DEFAULT)
        {
            ok(desc.BindFlags == (D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET), "Unexpected bind flags %#x.\n",
                desc.BindFlags);
            ok(!desc.CPUAccessFlags, "Unexpected CPU access flags %#x.\n", desc.CPUAccessFlags);
        }
        else if (usage[i] == D3D11_USAGE_DYNAMIC)
        {
            ok(desc.BindFlags == D3D11_BIND_SHADER_RESOURCE, "Unexpected bind flags %#x.\n", desc.BindFlags);
            ok(desc.CPUAccessFlags == D3D11_CPU_ACCESS_WRITE, "Unexpected CPU access flags %#x.\n", desc.CPUAccessFlags);
        }
        else if (usage[i] == D3D11_USAGE_STAGING)
        {
            ok(!desc.BindFlags, "Unexpected bind flags %#x.\n", desc.BindFlags);
            ok(desc.CPUAccessFlags == (D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ), "Unexpected CPU access flags %#x.\n",
                    desc.CPUAccessFlags);
        }
        ok(!desc.MiscFlags, "Unexpected misc flags %#x.\n", desc.MiscFlags);

        ID3D11Texture2D_Release(texture);
        IMFDXGIBuffer_Release(dxgi_buffer);
        IMFMediaBuffer_Release(buffer);

        IMFSample_Release(sample);

        IMFVideoSampleAllocatorEx_Release(allocatorex);
    }

    /* MF_SA_D3D11_SHARED, MF_SA_D3D11_SHARED_WITHOUT_MUTEX */
    for (i = 0; i < ARRAY_SIZE(sharing); ++i)
    {
        hr = pMFCreateVideoSampleAllocatorEx(&IID_IMFVideoSampleAllocatorEx, (void **)&allocatorex);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IMFVideoSampleAllocatorEx_SetDirectXManager(allocatorex, (IUnknown *)manager);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IMFAttributes_DeleteAllItems(attributes);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IMFAttributes_SetUINT32(attributes, &MF_SA_D3D11_USAGE, D3D11_USAGE_DEFAULT);
        ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);

        if (sharing[i] & D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX)
        {
            hr = IMFAttributes_SetUINT32(attributes, &MF_SA_D3D11_SHARED, TRUE);
            ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);
        }

        if (sharing[i] & D3D11_RESOURCE_MISC_SHARED)
        {
            hr = IMFAttributes_SetUINT32(attributes, &MF_SA_D3D11_SHARED_WITHOUT_MUTEX, TRUE);
            ok(hr == S_OK, "Failed to set attribute, hr %#lx.\n", hr);
        }

        hr = IMFVideoSampleAllocatorEx_InitializeSampleAllocatorEx(allocatorex, 0, 0, attributes, video_type);
        if (sharing[i] == (D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX | D3D11_RESOURCE_MISC_SHARED))
        {
            todo_wine
            ok(hr == E_INVALIDARG, "%u: Unexpected hr %#lx.\n", i, hr);
            IMFVideoSampleAllocatorEx_Release(allocatorex);
            continue;
        }
        ok(hr == S_OK, "%u: Unexpected hr %#lx.\n", i, hr);

        hr = IMFVideoSampleAllocatorEx_AllocateSample(allocatorex, &sample);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IMFSample_GetBufferByIndex(sample, 0, &buffer);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IMFMediaBuffer_QueryInterface(buffer, &IID_IMFDXGIBuffer, (void **)&dxgi_buffer);
        ok(hr == S_OK, "Failed to get interface, hr %#lx.\n", hr);

        hr = IMFDXGIBuffer_GetResource(dxgi_buffer, &IID_ID3D11Texture2D, (void **)&texture);
        ok(hr == S_OK, "Failed to get resource, hr %#lx.\n", hr);

        ID3D11Texture2D_GetDesc(texture, &desc);
        ok(desc.MiscFlags == sharing[i], "%u: unexpected misc flags %#x.\n", i, desc.MiscFlags);

        ID3D11Texture2D_Release(texture);
        IMFDXGIBuffer_Release(dxgi_buffer);
        IMFMediaBuffer_Release(buffer);

        IMFSample_Release(sample);

        IMFVideoSampleAllocatorEx_Release(allocatorex);
    }

    IMFAttributes_Release(attributes);

    IMFDXGIDeviceManager_Release(manager);
    ID3D11Device_Release(device);
}

static void test_sample_allocator_d3d12(void)
{
    IMFVideoSampleAllocator *allocator = NULL;
    D3D12_HEAP_PROPERTIES heap_props;
    IMFDXGIDeviceManager *manager;
    D3D12_HEAP_FLAGS heap_flags;
    IMFDXGIBuffer *dxgi_buffer;
    IMFMediaType *video_type;
    ID3D12Resource *resource;
    D3D12_RESOURCE_DESC desc;
    IMFMediaBuffer *buffer;
    ID3D12Device *device;
    unsigned int token;
    IMFSample *sample;
    HRESULT hr;

    if (!(device = create_d3d12_device()))
    {
        skip("Failed to create a D3D12 device, skipping tests.\n");
        return;
    }

    hr = pMFCreateDXGIDeviceManager(&token, &manager);
    ok(hr == S_OK, "Failed to create device manager, hr %#lx.\n", hr);

    hr = IMFDXGIDeviceManager_ResetDevice(manager, (IUnknown *)device, token);
    if (FAILED(hr))
    {
        win_skip("Device manager does not support D3D12 devices.\n");
        goto done;
    }
    ok(hr == S_OK, "Failed to set a device, hr %#lx.\n", hr);

    hr = pMFCreateVideoSampleAllocatorEx(&IID_IMFVideoSampleAllocator, (void **)&allocator);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    EXPECT_REF(manager, 1);
    hr = IMFVideoSampleAllocator_SetDirectXManager(allocator, (IUnknown *)manager);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(manager, 2);

    video_type = create_video_type(&MFVideoFormat_RGB32);
    hr = IMFMediaType_SetUINT64(video_type, &MF_MT_FRAME_SIZE, (UINT64) 64 << 32 | 64);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_SetUINT32(video_type, &MF_MT_D3D_RESOURCE_VERSION, MF_D3D12_RESOURCE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoSampleAllocator_InitializeSampleAllocator(allocator, 1, video_type);
    todo_wine
    ok(hr == S_OK || broken(hr == MF_E_UNEXPECTED) /* Some Win10 versions fail. */, "Unexpected hr %#lx.\n", hr);
    if (FAILED(hr)) goto done;

    hr = IMFVideoSampleAllocator_AllocateSample(allocator, &sample);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFSample_GetBufferByIndex(sample, 0, &buffer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    check_interface(buffer, &IID_IMF2DBuffer, TRUE);
    check_interface(buffer, &IID_IMF2DBuffer2, TRUE);
    check_interface(buffer, &IID_IMFDXGIBuffer, TRUE);
    check_interface(buffer, &IID_IMFGetService, FALSE);

    hr = IMFMediaBuffer_QueryInterface(buffer, &IID_IMFDXGIBuffer, (void **)&dxgi_buffer);
    ok(hr == S_OK, "Failed to get interface, hr %#lx.\n", hr);

    hr = IMFDXGIBuffer_GetResource(dxgi_buffer, &IID_ID3D12Resource, (void **)&resource);
    ok(hr == S_OK, "Failed to get resource, hr %#lx.\n", hr);

    resource->lpVtbl->GetDesc(resource, &desc);
    ok(desc.Width == 64, "Unexpected width.\n");
    ok(desc.Height == 64, "Unexpected height.\n");
    ok(desc.DepthOrArraySize == 1, "Unexpected array size %u.\n", desc.DepthOrArraySize);
    ok(desc.MipLevels == 1, "Unexpected miplevels %u.\n", desc.MipLevels);
    ok(desc.Format == DXGI_FORMAT_B8G8R8X8_UNORM, "Unexpected format %u.\n", desc.Format);
    ok(desc.SampleDesc.Count == 1, "Unexpected sample count %u.\n", desc.SampleDesc.Count);
    ok(!desc.SampleDesc.Quality, "Unexpected sample quality %u.\n", desc.SampleDesc.Quality);
    ok(!desc.Layout, "Unexpected layout %u.\n", desc.Layout);
    ok(!desc.Flags, "Unexpected flags %#x.\n", desc.Flags);

    hr = ID3D12Resource_GetHeapProperties(resource, &heap_props, &heap_flags);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(heap_props.Type == D3D12_HEAP_TYPE_DEFAULT, "Unexpected heap type %u.\n", heap_props.Type);
    ok(heap_props.CPUPageProperty == D3D12_CPU_PAGE_PROPERTY_UNKNOWN, "Unexpected page property %u.\n",
            heap_props.CPUPageProperty);
    ok(!heap_props.MemoryPoolPreference, "Unexpected pool preference %u.\n", heap_props.MemoryPoolPreference);
    ok(heap_flags == D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES, "Unexpected heap flags %#x.\n", heap_flags);

    ID3D12Resource_Release(resource);
    IMFDXGIBuffer_Release(dxgi_buffer);
    IMFMediaBuffer_Release(buffer);
    IMFSample_Release(sample);

done:
    if (allocator)
        IMFVideoSampleAllocator_Release(allocator);
    IMFDXGIDeviceManager_Release(manager);
    ID3D12Device_Release(device);
}

static void test_MFLockSharedWorkQueue(void)
{
    DWORD taskid, queue, queue2;
    HRESULT hr;

    if (!pMFLockSharedWorkQueue)
    {
        win_skip("MFLockSharedWorkQueue() is not available.\n");
        return;
    }

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Failed to start up, hr %#lx.\n", hr);

    hr = pMFLockSharedWorkQueue(NULL, 0, &taskid, &queue);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = pMFLockSharedWorkQueue(NULL, 0, NULL, &queue);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    taskid = 0;
    hr = pMFLockSharedWorkQueue(L"", 0, &taskid, &queue);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    queue = 0;
    hr = pMFLockSharedWorkQueue(L"", 0, NULL, &queue);
    ok(queue & MFASYNC_CALLBACK_QUEUE_PRIVATE_MASK, "Unexpected queue id.\n");

    queue2 = 0;
    hr = pMFLockSharedWorkQueue(L"", 0, NULL, &queue2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(queue == queue2, "Unexpected queue %#lx.\n", queue2);

    hr = MFUnlockWorkQueue(queue2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = MFUnlockWorkQueue(queue);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = MFShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);
}

static void test_MFllMulDiv(void)
{
    /* (a * b + d) / c */
    static const struct muldivtest
    {
        LONGLONG a;
        LONGLONG b;
        LONGLONG c;
        LONGLONG d;
        LONGLONG result;
    }
    muldivtests[] =
    {
        { 0, 0, 0, 0, _I64_MAX },
        { 1000000, 1000000, 2, 0, 500000000000 },
        { _I64_MAX, 3, _I64_MAX, 0, 3 },
        { _I64_MAX, 3, _I64_MAX, 1, 3 },
        { -10000, 3, 100, 0, -300 },
        { 2, 0, 3, 5, 1 },
        { 2, 1, 1, -3, -1 },
        /* a * b product does not fit in uint64_t */
        { _I64_MAX, 4, 8, 0, _I64_MAX / 2 },
        /* Large a * b product, large denominator */
        { _I64_MAX, 4, 0x100000000, 0, 0x1ffffffff },
    };
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(muldivtests); ++i)
    {
        LONGLONG result;

        result = MFllMulDiv(muldivtests[i].a, muldivtests[i].b, muldivtests[i].c, muldivtests[i].d);
        ok(result == muldivtests[i].result, "%u: unexpected result %s, expected %s.\n", i,
                wine_dbgstr_longlong(result), wine_dbgstr_longlong(muldivtests[i].result));
    }
}

static void test_shared_dxgi_device_manager(void)
{
    IMFDXGIDeviceManager *manager;
    HRESULT hr;
    UINT token;

    if (!pMFLockDXGIDeviceManager)
    {
        win_skip("Shared DXGI device manager is not supported.\n");
        return;
    }

    hr = pMFUnlockDXGIDeviceManager();
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    manager = NULL;
    hr = pMFLockDXGIDeviceManager(NULL, &manager);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!manager, "Unexpected instance.\n");

    hr = pMFLockDXGIDeviceManager(&token, &manager);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    EXPECT_REF(manager, 3);

    hr = pMFUnlockDXGIDeviceManager();
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    EXPECT_REF(manager, 2);

    hr = pMFUnlockDXGIDeviceManager();
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
}

static void check_video_format(const MFVIDEOFORMAT *format, unsigned int width, unsigned int height,
        DWORD d3dformat)
{
    unsigned int transfer_function;
    GUID guid;

    if (!d3dformat) d3dformat = D3DFMT_X8R8G8B8;

    switch (d3dformat)
    {
        case D3DFMT_X8R8G8B8:
        case D3DFMT_R8G8B8:
        case D3DFMT_A8R8G8B8:
        case D3DFMT_R5G6B5:
        case D3DFMT_X1R5G5B5:
        case D3DFMT_A2B10G10R10:
        case D3DFMT_A2R10G10B10:
        case D3DFMT_P8:
            transfer_function = MFVideoTransFunc_sRGB;
            break;
        default:
            transfer_function = MFVideoTransFunc_10;
    }

    memcpy(&guid, &MFVideoFormat_Base, sizeof(guid));
    guid.Data1 = d3dformat;

    ok(format->dwSize == sizeof(*format), "Unexpected format size.\n");
    ok(format->videoInfo.dwWidth == width, "Unexpected width %lu.\n", format->videoInfo.dwWidth);
    ok(format->videoInfo.dwHeight == height, "Unexpected height %lu.\n", format->videoInfo.dwHeight);
    ok(format->videoInfo.PixelAspectRatio.Numerator == 1 &&
            format->videoInfo.PixelAspectRatio.Denominator == 1, "Unexpected PAR.\n");
    ok(format->videoInfo.SourceChromaSubsampling == MFVideoChromaSubsampling_Unknown, "Unexpected chroma subsampling.\n");
    ok(format->videoInfo.InterlaceMode == MFVideoInterlace_Progressive, "Unexpected interlace mode %u.\n",
            format->videoInfo.InterlaceMode);
    ok(format->videoInfo.TransferFunction == transfer_function, "Unexpected transfer function %u.\n",
            format->videoInfo.TransferFunction);
    ok(format->videoInfo.ColorPrimaries == MFVideoPrimaries_BT709, "Unexpected color primaries %u.\n",
            format->videoInfo.ColorPrimaries);
    ok(format->videoInfo.TransferMatrix == MFVideoTransferMatrix_Unknown, "Unexpected transfer matrix.\n");
    ok(format->videoInfo.SourceLighting == MFVideoLighting_office, "Unexpected source lighting %u.\n",
            format->videoInfo.SourceLighting);
    ok(format->videoInfo.FramesPerSecond.Numerator == 60 &&
            format->videoInfo.FramesPerSecond.Denominator == 1, "Unexpected frame rate %lu/%lu.\n",
            format->videoInfo.FramesPerSecond.Numerator, format->videoInfo.FramesPerSecond.Denominator);
    ok(format->videoInfo.NominalRange == MFNominalRange_Normal, "Unexpected nominal range %u.\n",
            format->videoInfo.NominalRange);
    ok(format->videoInfo.GeometricAperture.Area.cx == width && format->videoInfo.GeometricAperture.Area.cy == height,
            "Unexpected geometric aperture.\n");
    ok(!memcmp(&format->videoInfo.GeometricAperture, &format->videoInfo.MinimumDisplayAperture, sizeof(MFVideoArea)),
            "Unexpected minimum display aperture.\n");
    ok(format->videoInfo.PanScanAperture.Area.cx == 0 && format->videoInfo.PanScanAperture.Area.cy == 0,
            "Unexpected geometric aperture.\n");
    ok(format->videoInfo.VideoFlags == 0, "Unexpected video flags.\n");
    ok(IsEqualGUID(&format->guidFormat, &guid), "Unexpected format guid %s.\n", wine_dbgstr_guid(&format->guidFormat));
    ok(format->compressedInfo.AvgBitrate == 0, "Unexpected bitrate.\n");
    ok(format->compressedInfo.AvgBitErrorRate == 0, "Unexpected error bitrate.\n");
    ok(format->compressedInfo.MaxKeyFrameSpacing == 0, "Unexpected MaxKeyFrameSpacing.\n");
    ok(format->surfaceInfo.Format == d3dformat, "Unexpected format %lu.\n", format->surfaceInfo.Format);
    ok(format->surfaceInfo.PaletteEntries == 0, "Unexpected palette size %lu.\n", format->surfaceInfo.PaletteEntries);
}

static void test_MFInitVideoFormat_RGB(void)
{
    static const DWORD formats[] =
    {
        0, /* same D3DFMT_X8R8G8B8 */
        D3DFMT_X8R8G8B8,
        D3DFMT_R8G8B8,
        D3DFMT_A8R8G8B8,
        D3DFMT_R5G6B5,
        D3DFMT_X1R5G5B5,
        D3DFMT_A2B10G10R10,
        D3DFMT_A2R10G10B10,
        D3DFMT_P8,
        D3DFMT_L8,
        D3DFMT_YUY2,
        D3DFMT_DXT1,
        D3DFMT_D16,
        D3DFMT_L16,
        D3DFMT_A16B16G16R16F,
    };
    MFVIDEOFORMAT format;
    unsigned int i;
    HRESULT hr;

    if (!pMFInitVideoFormat_RGB)
    {
        win_skip("MFInitVideoFormat_RGB is not available.\n");
        return;
    }

    hr = pMFInitVideoFormat_RGB(NULL, 64, 32, 0);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    for (i = 0; i < ARRAY_SIZE(formats); ++i)
    {
        memset(&format, 0, sizeof(format));
        hr = pMFInitVideoFormat_RGB(&format, 64, 32, formats[i]);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        if (SUCCEEDED(hr))
            check_video_format(&format, 64, 32, formats[i]);
    }
}

static void test_MFCreateVideoMediaTypeFromVideoInfoHeader(void)
{
    IMFVideoMediaType *media_type;
    KS_VIDEOINFOHEADER vih;
    UINT32 value32;
    UINT64 value64;
    HRESULT hr;
    GUID guid;

    hr = MFCreateVideoMediaTypeFromVideoInfoHeader(NULL, 0, 0, 0, MFVideoInterlace_Unknown, 0, NULL, &media_type);
    todo_wine
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    memset(&vih, 0, sizeof(vih));
    hr = MFCreateVideoMediaTypeFromVideoInfoHeader(&vih, 0, 0, 0, MFVideoInterlace_Unknown, 0, NULL, &media_type);
    todo_wine
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    hr = MFCreateVideoMediaTypeFromVideoInfoHeader(&vih, sizeof(vih), 0, 0, MFVideoInterlace_Unknown, 0, NULL, &media_type);
    todo_wine
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    vih.bmiHeader.biSize = sizeof(vih.bmiHeader);
    hr = MFCreateVideoMediaTypeFromVideoInfoHeader(&vih, sizeof(vih), 0, 0, MFVideoInterlace_Unknown, 0, NULL, &media_type);
    todo_wine
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    vih.bmiHeader.biSize = sizeof(vih.bmiHeader);
    vih.bmiHeader.biPlanes = 1;
    vih.bmiHeader.biWidth = 16;
    vih.bmiHeader.biHeight = 32;
    vih.bmiHeader.biBitCount = 32;

    hr = MFCreateVideoMediaTypeFromVideoInfoHeader(&vih, sizeof(vih), 3, 2, MFVideoInterlace_Progressive,
            MFVideoFlag_AnalogProtected, &GUID_NULL, &media_type);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (FAILED(hr)) return;
    IMFVideoMediaType_Release(media_type);

    hr = MFCreateVideoMediaTypeFromVideoInfoHeader(&vih, sizeof(vih), 3, 2, MFVideoInterlace_Progressive,
            MFVideoFlag_AnalogProtected, NULL, &media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFVideoMediaType_GetGUID(media_type, &MF_MT_MAJOR_TYPE, &guid);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&guid, &MFMediaType_Video), "Unexpected guid %s.\n", debugstr_guid(&guid));
    hr = IMFVideoMediaType_GetGUID(media_type, &MF_MT_SUBTYPE, &guid);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&guid, &MFVideoFormat_RGB32), "Unexpected guid %s.\n", debugstr_guid(&guid));
    hr = IMFVideoMediaType_GetUINT64(media_type, &MF_MT_FRAME_SIZE, &value64);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value64 == ((UINT64)16 << 32 | 32), "Unexpected value %#I64x.\n", value64);
    hr = IMFVideoMediaType_GetUINT64(media_type, &MF_MT_PIXEL_ASPECT_RATIO, &value64);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value64 == ((UINT64)3 << 32 | 2), "Unexpected value %#I64x.\n", value64);
    hr = IMFVideoMediaType_GetUINT32(media_type, &MF_MT_DRM_FLAGS, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == MFVideoDRMFlag_AnalogProtected, "Unexpected value %#x.\n", value32);
    hr = IMFVideoMediaType_GetUINT32(media_type, &MF_MT_INTERLACE_MODE, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == MFVideoInterlace_Progressive, "Unexpected value %#x.\n", value32);
    hr = IMFVideoMediaType_GetUINT32(media_type, &MF_MT_SAMPLE_SIZE, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == 2048, "Unexpected value %u.\n", value32);
    hr = IMFVideoMediaType_GetUINT32(media_type, &MF_MT_DEFAULT_STRIDE, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == -64, "Unexpected value %d.\n", value32);
    hr = IMFVideoMediaType_GetUINT32(media_type, &MF_MT_FIXED_SIZE_SAMPLES, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!value32, "Unexpected value %#x.\n", value32);
    hr = IMFVideoMediaType_GetUINT32(media_type, &MF_MT_ALL_SAMPLES_INDEPENDENT, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!value32, "Unexpected value %#x.\n", value32);

    IMFVideoMediaType_Release(media_type);

    /* Negative height. */
    vih.bmiHeader.biHeight = -32;
    hr = MFCreateVideoMediaTypeFromVideoInfoHeader(&vih, sizeof(vih), 3, 2, MFVideoInterlace_Progressive,
            MFVideoFlag_AnalogProtected, NULL, &media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFVideoMediaType_GetUINT64(media_type, &MF_MT_FRAME_SIZE, &value64);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value64 == ((UINT64)16 << 32 | 32), "Unexpected value %#I64x.\n", value64);
    hr = IMFVideoMediaType_GetUINT32(media_type, &MF_MT_DEFAULT_STRIDE, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == 64, "Unexpected value %d.\n", value32);
    IMFVideoMediaType_Release(media_type);
}

static void test_MFInitMediaTypeFromVideoInfoHeader(void)
{
    static const MFVideoArea expect_aperture = {.OffsetX = {.value = 1}, .OffsetY = {.value = 2}, .Area = {.cx = 3, .cy = 5}};
    static const RECT source = {1, 2, 4, 7}, target = {3, 2, 12, 9};
    static const PALETTEENTRY expect_palette[] = {{1},{2},{3},{4},{5},{6},{7},{8}};
    static const BYTE expect_user_data[] = {6,5,4,3,2,1};
    PALETTEENTRY palette[ARRAY_SIZE(expect_palette)];
    BYTE user_data[sizeof(expect_user_data)];
    char buffer[sizeof(VIDEOINFOHEADER2) + sizeof(user_data) + sizeof(palette)];
    IMFMediaType *media_type;
    MFVideoArea aperture;
    VIDEOINFOHEADER vih, *vih_buf = (VIDEOINFOHEADER *)buffer;
    UINT32 value32;
    UINT64 value64;
    HRESULT hr;
    GUID guid;

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    memset(&vih, 0, sizeof(vih));
    hr = MFInitMediaTypeFromVideoInfoHeader(media_type, &vih, 0, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    hr = MFInitMediaTypeFromVideoInfoHeader(media_type, &vih, sizeof(vih), NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    /* bmiHeader.biSize and the size parameter are checked together */
    vih.bmiHeader.biSize = 1;
    hr = MFInitMediaTypeFromVideoInfoHeader(media_type, &vih, sizeof(vih), &GUID_NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    vih.bmiHeader.biSize = sizeof(vih.bmiHeader) - 1;
    hr = MFInitMediaTypeFromVideoInfoHeader(media_type, &vih, sizeof(vih), &GUID_NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    vih.bmiHeader.biSize = sizeof(vih.bmiHeader);
    hr = MFInitMediaTypeFromVideoInfoHeader(media_type, &vih, sizeof(vih), &GUID_NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    vih.bmiHeader.biSize = sizeof(vih.bmiHeader) - 1;
    hr = MFInitMediaTypeFromVideoInfoHeader(media_type, &vih, sizeof(vih) - 1, &GUID_NULL);
    todo_wine ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    *vih_buf = vih;
    vih_buf->bmiHeader.biSize = sizeof(vih.bmiHeader) + 1;
    hr = MFInitMediaTypeFromVideoInfoHeader(media_type, vih_buf, sizeof(*vih_buf) + 1, &GUID_NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    vih.bmiHeader.biSize = 0;
    hr = MFInitMediaTypeFromVideoInfoHeader(media_type, &vih, sizeof(vih), &GUID_NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetGUID(media_type, &MF_MT_MAJOR_TYPE, &guid);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&guid, &MFMediaType_Video), "Unexpected guid %s.\n", debugstr_guid(&guid));
    hr = IMFMediaType_GetGUID(media_type, &MF_MT_SUBTYPE, &guid);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&guid, &GUID_NULL), "Unexpected guid %s.\n", debugstr_guid(&guid));
    hr = IMFMediaType_GetUINT64(media_type, &MF_MT_FRAME_SIZE, &value64);
    todo_wine
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    vih.bmiHeader.biWidth = 16;
    hr = MFInitMediaTypeFromVideoInfoHeader(media_type, &vih, sizeof(vih), &GUID_NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT64(media_type, &MF_MT_FRAME_SIZE, &value64);
    todo_wine
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT64(media_type, &MF_MT_PIXEL_ASPECT_RATIO, &value64);
    todo_wine
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_INTERLACE_MODE, &value32);
    todo_wine
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    vih.bmiHeader.biHeight = -32;
    hr = MFInitMediaTypeFromVideoInfoHeader(media_type, &vih, sizeof(vih), &GUID_NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT64(media_type, &MF_MT_FRAME_SIZE, &value64);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value64 == ((UINT64)16 << 32 | 32), "Unexpected value %#I64x.\n", value64);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_DEFAULT_STRIDE, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    vih.bmiHeader.biHeight = 32;
    hr = MFInitMediaTypeFromVideoInfoHeader(media_type, &vih, sizeof(vih), &GUID_NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT64(media_type, &MF_MT_FRAME_SIZE, &value64);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value64 == ((UINT64)16 << 32 | 32), "Unexpected value %#I64x.\n", value64);
    hr = IMFMediaType_GetUINT64(media_type, &MF_MT_PIXEL_ASPECT_RATIO, &value64);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value64 == ((UINT64)1 << 32 | 1), "Unexpected value %#I64x.\n", value64);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_INTERLACE_MODE, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == MFVideoInterlace_Progressive, "Unexpected value %#x.\n", value32);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_DEFAULT_STRIDE, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_SAMPLE_SIZE, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    value32 = 0xdeadbeef;
    vih.bmiHeader.biSizeImage = 12345;
    hr = MFInitMediaTypeFromVideoInfoHeader(media_type, &vih, sizeof(vih), &GUID_NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_SAMPLE_SIZE, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == 12345, "Unexpected value %#x.\n", value32);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_AVG_BITRATE, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    value32 = 0xdeadbeef;
    vih.dwBitRate = 678910;
    hr = MFInitMediaTypeFromVideoInfoHeader(media_type, &vih, sizeof(vih), &GUID_NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_AVG_BITRATE, &value32);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine
    ok(value32 == 678910, "Unexpected value %#x.\n", value32);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_AVG_BIT_ERROR_RATE, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    value32 = 0xdeadbeef;
    vih.dwBitErrorRate = 11121314;
    hr = MFInitMediaTypeFromVideoInfoHeader(media_type, &vih, sizeof(vih), &GUID_NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_AVG_BIT_ERROR_RATE, &value32);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine
    ok(value32 == 11121314, "Unexpected value %#x.\n", value32);
    hr = IMFMediaType_GetUINT64(media_type, &MF_MT_FRAME_RATE, &value64);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    value64 = 0xdeadbeef;
    vih.AvgTimePerFrame = 1151617;
    hr = MFInitMediaTypeFromVideoInfoHeader(media_type, &vih, sizeof(vih), &GUID_NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT64(media_type, &MF_MT_FRAME_RATE, &value64);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value64 == ((UINT64)10000000 << 32 | 1151617), "Unexpected value %#I64x.\n", value64);

    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_FIXED_SIZE_SAMPLES, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_ALL_SAMPLES_INDEPENDENT, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaType_GetBlob(media_type, &MF_MT_MINIMUM_DISPLAY_APERTURE, (BYTE *)&aperture, sizeof(aperture), &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetBlob(media_type, &MF_MT_GEOMETRIC_APERTURE, (BYTE *)&aperture, sizeof(aperture), &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetBlob(media_type, &MF_MT_PAN_SCAN_APERTURE, (BYTE *)&aperture, sizeof(aperture), &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_PAN_SCAN_ENABLED, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    /* only rcSource is considered, translated into both MF_MT_MINIMUM_DISPLAY_APERTURE and MF_MT_PAN_SCAN_APERTURE */
    value32 = 0xdeadbeef;
    vih.rcSource = source;
    vih.rcTarget = target;
    hr = MFInitMediaTypeFromVideoInfoHeader(media_type, &vih, sizeof(vih), &GUID_NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT64(media_type, &MF_MT_FRAME_SIZE, &value64);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value64 == ((UINT64)16 << 32 | 32), "Unexpected value %#I64x.\n", value64);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_SAMPLE_SIZE, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == 12345, "Unexpected value %#x.\n", value32);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_DEFAULT_STRIDE, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    value32 = 0xdeadbeef;
    memset(&aperture, 0xcd, sizeof(aperture));
    hr = IMFMediaType_GetBlob(media_type, &MF_MT_MINIMUM_DISPLAY_APERTURE, (BYTE *)&aperture, sizeof(aperture), &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == sizeof(aperture), "got %d.\n", value32);
    ok(!memcmp(&aperture, &expect_aperture, sizeof(aperture)), "unexpected aperture\n");

    value32 = 0xdeadbeef;
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_PAN_SCAN_ENABLED, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == 1, "got %d.\n", (UINT32)value32);

    value32 = 0xdeadbeef;
    memset(&aperture, 0xcd, sizeof(aperture));
    hr = IMFMediaType_GetBlob(media_type, &MF_MT_PAN_SCAN_APERTURE, (BYTE *)&aperture, sizeof(aperture), &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == sizeof(aperture), "got %d.\n", value32);
    ok(!memcmp(&aperture, &expect_aperture, sizeof(aperture)), "unexpected aperture\n");

    hr = IMFMediaType_GetItem(media_type, &MF_MT_GEOMETRIC_APERTURE, NULL);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);


    hr = MFInitMediaTypeFromVideoInfoHeader(media_type, &vih, sizeof(vih), &MFVideoFormat_NV12);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_FIXED_SIZE_SAMPLES, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == 1, "Unexpected value %#x.\n", value32);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_ALL_SAMPLES_INDEPENDENT, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == 1, "Unexpected value %#x.\n", value32);


    /* biBitCount is used for implicit RGB format if GUID is NULL */
    hr = MFInitMediaTypeFromVideoInfoHeader(media_type, &vih, sizeof(vih), NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    for (vih.bmiHeader.biBitCount = 1; vih.bmiHeader.biBitCount <= 32; vih.bmiHeader.biBitCount++)
    {
        winetest_push_context("%u", vih.bmiHeader.biBitCount);

        hr = MFInitMediaTypeFromVideoInfoHeader(media_type, &vih, sizeof(vih), NULL);
        if (vih.bmiHeader.biBitCount != 1 && vih.bmiHeader.biBitCount != 4 && vih.bmiHeader.biBitCount != 8
                && vih.bmiHeader.biBitCount != 16 && vih.bmiHeader.biBitCount != 24 && vih.bmiHeader.biBitCount != 32)
            ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
        else
        {
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

            memset(&guid, 0xcd, sizeof(guid));
            hr = IMFMediaType_GetGUID(media_type, &MF_MT_SUBTYPE, &guid);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            if (vih.bmiHeader.biBitCount == 32)
                ok(IsEqualGUID(&guid, &MFVideoFormat_RGB32), "Unexpected guid %s.\n", debugstr_guid(&guid));
            else if (vih.bmiHeader.biBitCount == 24)
                ok(IsEqualGUID(&guid, &MFVideoFormat_RGB24), "Unexpected guid %s.\n", debugstr_guid(&guid));
            else if (vih.bmiHeader.biBitCount == 16)
                ok(IsEqualGUID(&guid, &MFVideoFormat_RGB555), "Unexpected guid %s.\n", debugstr_guid(&guid));
            else if (vih.bmiHeader.biBitCount == 8)
                ok(IsEqualGUID(&guid, &MFVideoFormat_RGB8), "Unexpected guid %s.\n", debugstr_guid(&guid));
            else if (vih.bmiHeader.biBitCount == 4)
                ok(IsEqualGUID(&guid, &MFVideoFormat_RGB4), "Unexpected guid %s.\n", debugstr_guid(&guid));
            else if (vih.bmiHeader.biBitCount == 1)
                ok(IsEqualGUID(&guid, &MFVideoFormat_RGB1), "Unexpected guid %s.\n", debugstr_guid(&guid));

            value32 = 0xdeadbeef;
            hr = IMFMediaType_GetUINT32(media_type, &MF_MT_DEFAULT_STRIDE, &value32);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            if (vih.bmiHeader.biBitCount > 1)
                ok(value32 == 16 * vih.bmiHeader.biBitCount / 8, "Unexpected value %#x.\n", value32);
            else
                todo_wine ok(value32 == -4, "Unexpected value %#x.\n", value32);

            hr = IMFMediaType_GetItem(media_type, &MF_MT_PALETTE, NULL);
            if (vih.bmiHeader.biBitCount > 1)
                ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
            else
                todo_wine ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

            value32 = 0xdeadbeef;
            hr = IMFMediaType_GetUINT32(media_type, &MF_MT_FIXED_SIZE_SAMPLES, &value32);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            ok(value32 == 1, "Unexpected value %#x.\n", value32);
            value32 = 0xdeadbeef;
            hr = IMFMediaType_GetUINT32(media_type, &MF_MT_ALL_SAMPLES_INDEPENDENT, &value32);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            ok(value32 == 1, "Unexpected value %#x.\n", value32);

            value32 = 0xdeadbeef;
            vih.bmiHeader.biHeight = 32;
            hr = MFInitMediaTypeFromVideoInfoHeader(media_type, &vih, sizeof(vih), NULL);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            hr = IMFMediaType_GetUINT32(media_type, &MF_MT_DEFAULT_STRIDE, &value32);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            if (vih.bmiHeader.biBitCount > 1)
                ok(value32 == -16 * vih.bmiHeader.biBitCount / 8, "Unexpected value %#x.\n", value32);
            else
                todo_wine ok(value32 == -4, "Unexpected value %#x.\n", value32);

            vih.bmiHeader.biHeight = -32;
        }

        winetest_pop_context();
    }

    hr = IMFMediaType_GetItem(media_type, &MF_MT_USER_DATA, NULL);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetItem(media_type, &MF_MT_PALETTE, NULL);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    *vih_buf = vih;
    vih_buf->bmiHeader.biCompression = BI_RGB;
    vih_buf->bmiHeader.biBitCount = 24;
    hr = MFInitMediaTypeFromVideoInfoHeader(media_type, vih_buf, sizeof(*vih_buf), NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    memcpy( vih_buf + 1, expect_palette, sizeof(expect_palette) );
    vih_buf->bmiHeader.biClrUsed = ARRAY_SIZE(expect_palette);

    /* palette only works with 8bit RGB format */
    vih_buf->bmiHeader.biSize = sizeof(vih_buf->bmiHeader) + sizeof(expect_palette);
    hr = MFInitMediaTypeFromVideoInfoHeader(media_type, vih_buf, sizeof(*vih_buf) + sizeof(expect_palette), NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetItem(media_type, &MF_MT_PALETTE, NULL);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    value32 = 0xdeadbeef;
    memset(palette, 0xcd, sizeof(palette));
    hr = IMFMediaType_GetBlob(media_type, &MF_MT_USER_DATA, (BYTE *)palette, sizeof(palette), &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == sizeof(expect_palette), "got %u.\n", value32);
    ok(!memcmp(palette, expect_palette, value32), "Unexpected user data.\n");

    vih_buf->bmiHeader.biBitCount = 16;
    hr = MFInitMediaTypeFromVideoInfoHeader(media_type, vih_buf, sizeof(*vih_buf) + sizeof(expect_palette), NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetItem(media_type, &MF_MT_PALETTE, NULL);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    value32 = 0xdeadbeef;
    memset(palette, 0xcd, sizeof(palette));
    hr = IMFMediaType_GetBlob(media_type, &MF_MT_USER_DATA, (BYTE *)palette, sizeof(palette), &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == sizeof(expect_palette), "got %u.\n", value32);
    ok(!memcmp(palette, expect_palette, value32), "Unexpected user data.\n");

    /* palette shouldn't be accounted for in the header size */
    vih_buf->bmiHeader.biBitCount = 8;
    hr = MFInitMediaTypeFromVideoInfoHeader(media_type, vih_buf, sizeof(*vih_buf) + sizeof(expect_palette), NULL);
    todo_wine ok(hr == MF_E_INVALIDMEDIATYPE, "Unexpected hr %#lx.\n", hr);
    vih_buf->bmiHeader.biSize = sizeof(vih_buf->bmiHeader);
    hr = MFInitMediaTypeFromVideoInfoHeader(media_type, vih_buf, sizeof(*vih_buf) + sizeof(expect_palette), NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetItem(media_type, &MF_MT_USER_DATA, NULL);
    todo_wine ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    value32 = 0xdeadbeef;
    memset(palette, 0xcd, sizeof(palette));
    hr = IMFMediaType_GetBlob(media_type, &MF_MT_PALETTE, (BYTE *)palette, sizeof(palette), &value32);
    todo_wine ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine ok(value32 == sizeof(expect_palette), "got %u.\n", value32);
    todo_wine ok(!memcmp(palette, expect_palette, value32), "Unexpected palette.\n");

    /* cannot have both user data and palette */
    memcpy( vih_buf + 1, expect_user_data, sizeof(expect_user_data) );
    memcpy( (BYTE *)(vih_buf + 1) + sizeof(expect_user_data), expect_palette, sizeof(expect_palette) );
    vih_buf->bmiHeader.biSize = sizeof(vih_buf->bmiHeader) + sizeof(expect_user_data);
    hr = MFInitMediaTypeFromVideoInfoHeader(media_type, vih_buf, sizeof(*vih_buf) + sizeof(expect_user_data) + sizeof(expect_palette), NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetItem(media_type, &MF_MT_USER_DATA, NULL);
    todo_wine ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    value32 = 0xdeadbeef;
    memset(palette, 0xcd, sizeof(palette));
    hr = IMFMediaType_GetBlob(media_type, &MF_MT_PALETTE, (BYTE *)palette, sizeof(palette), &value32);
    todo_wine ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine ok(value32 == sizeof(expect_palette), "got %u.\n", value32);
    todo_wine ok(!memcmp(palette, expect_palette, value32), "Unexpected palette.\n");

    IMFMediaType_Release(media_type);
}

static void test_MFInitMediaTypeFromVideoInfoHeader2(void)
{
    static const MFVideoArea expect_aperture = {.OffsetX = {.value = 1}, .OffsetY = {.value = 2}, .Area = {.cx = 3, .cy = 5}};
    static const RECT source = {1, 2, 4, 7}, target = {3, 2, 12, 9};
    static const RGBQUAD expect_palette[] = {{1},{2},{3},{4},{5},{6},{7},{8}};
    static const BYTE expect_user_data[] = {6,5,4,3,2,1};
    RGBQUAD palette[ARRAY_SIZE(expect_palette)];
    BYTE user_data[sizeof(expect_user_data)];
    char buffer[sizeof(VIDEOINFOHEADER2) + sizeof(palette) + sizeof(user_data)];
    IMFMediaType *media_type;
    VIDEOINFOHEADER2 vih, *vih_buf = (VIDEOINFOHEADER2 *)buffer;
    MFVideoArea aperture;
    UINT32 value32;
    UINT64 value64;
    HRESULT hr;
    GUID guid;

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    memset(&vih, 0, sizeof(vih));
    hr = MFInitMediaTypeFromVideoInfoHeader2(media_type, &vih, 0, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    hr = MFInitMediaTypeFromVideoInfoHeader2(media_type, &vih, sizeof(vih), NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    /* bmiHeader.biSize and the size parameter are checked together */
    vih.bmiHeader.biSize = 1;
    hr = MFInitMediaTypeFromVideoInfoHeader2(media_type, &vih, sizeof(vih), &GUID_NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    vih.bmiHeader.biSize = sizeof(vih.bmiHeader) - 1;
    hr = MFInitMediaTypeFromVideoInfoHeader2(media_type, &vih, sizeof(vih), &GUID_NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    vih.bmiHeader.biSize = sizeof(vih.bmiHeader);
    hr = MFInitMediaTypeFromVideoInfoHeader2(media_type, &vih, sizeof(vih), &GUID_NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    vih.bmiHeader.biSize = sizeof(vih.bmiHeader) - 1;
    hr = MFInitMediaTypeFromVideoInfoHeader2(media_type, &vih, sizeof(vih) - 1, &GUID_NULL);
    todo_wine ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    *vih_buf = vih;
    vih_buf->bmiHeader.biSize = sizeof(vih.bmiHeader) + 1;
    hr = MFInitMediaTypeFromVideoInfoHeader2(media_type, vih_buf, sizeof(*vih_buf) + 1, &GUID_NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    vih.bmiHeader.biSize = 0;
    hr = MFInitMediaTypeFromVideoInfoHeader2(media_type, &vih, sizeof(vih), &GUID_NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetGUID(media_type, &MF_MT_MAJOR_TYPE, &guid);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&guid, &MFMediaType_Video), "Unexpected guid %s.\n", debugstr_guid(&guid));
    hr = IMFMediaType_GetGUID(media_type, &MF_MT_SUBTYPE, &guid);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&guid, &GUID_NULL), "Unexpected guid %s.\n", debugstr_guid(&guid));
    hr = IMFMediaType_GetUINT64(media_type, &MF_MT_FRAME_SIZE, &value64);
    todo_wine
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    vih.bmiHeader.biWidth = 16;
    hr = MFInitMediaTypeFromVideoInfoHeader2(media_type, &vih, sizeof(vih), &GUID_NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT64(media_type, &MF_MT_FRAME_SIZE, &value64);
    todo_wine
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT64(media_type, &MF_MT_PIXEL_ASPECT_RATIO, &value64);
    todo_wine
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_INTERLACE_MODE, &value32);
    todo_wine
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    vih.bmiHeader.biHeight = -32;
    hr = MFInitMediaTypeFromVideoInfoHeader2(media_type, &vih, sizeof(vih), &GUID_NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT64(media_type, &MF_MT_FRAME_SIZE, &value64);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value64 == ((UINT64)16 << 32 | 32), "Unexpected value %#I64x.\n", value64);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_DEFAULT_STRIDE, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    vih.bmiHeader.biHeight = 32;
    hr = MFInitMediaTypeFromVideoInfoHeader2(media_type, &vih, sizeof(vih), &GUID_NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT64(media_type, &MF_MT_FRAME_SIZE, &value64);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value64 == ((UINT64)16 << 32 | 32), "Unexpected value %#I64x.\n", value64);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_INTERLACE_MODE, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == MFVideoInterlace_Progressive, "Unexpected value %#x.\n", value32);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_DEFAULT_STRIDE, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT64(media_type, &MF_MT_PIXEL_ASPECT_RATIO, &value64);
    todo_wine
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_SAMPLE_SIZE, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    value32 = 0xdeadbeef;
    vih.bmiHeader.biSizeImage = 12345;
    hr = MFInitMediaTypeFromVideoInfoHeader2(media_type, &vih, sizeof(vih), &GUID_NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_SAMPLE_SIZE, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == 12345, "Unexpected value %#x.\n", value32);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_AVG_BITRATE, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    value32 = 0xdeadbeef;
    vih.dwBitRate = 678910;
    hr = MFInitMediaTypeFromVideoInfoHeader2(media_type, &vih, sizeof(vih), &GUID_NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_AVG_BITRATE, &value32);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine
    ok(value32 == 678910, "Unexpected value %#x.\n", value32);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_AVG_BIT_ERROR_RATE, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    value32 = 0xdeadbeef;
    vih.dwBitErrorRate = 11121314;
    hr = MFInitMediaTypeFromVideoInfoHeader2(media_type, &vih, sizeof(vih), &GUID_NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_AVG_BIT_ERROR_RATE, &value32);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine
    ok(value32 == 11121314, "Unexpected value %#x.\n", value32);
    hr = IMFMediaType_GetUINT64(media_type, &MF_MT_FRAME_RATE, &value64);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    value64 = 0xdeadbeef;
    vih.AvgTimePerFrame = 1151617;
    hr = MFInitMediaTypeFromVideoInfoHeader2(media_type, &vih, sizeof(vih), &GUID_NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT64(media_type, &MF_MT_FRAME_RATE, &value64);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value64 == ((UINT64)10000000 << 32 | 1151617), "Unexpected value %#I64x.\n", value64);

    value32 = 0xdeadbeef;
    vih.dwInterlaceFlags = AMINTERLACE_IsInterlaced | AMINTERLACE_DisplayModeBobOrWeave;
    hr = MFInitMediaTypeFromVideoInfoHeader2(media_type, &vih, sizeof(vih), &GUID_NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_INTERLACE_MODE, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == MFVideoInterlace_MixedInterlaceOrProgressive
            || broken(value32 == MFVideoInterlace_FieldInterleavedLowerFirst) /* Win7 */,
            "Unexpected value %#x.\n", value32);

    vih.dwPictAspectRatioX = 123;
    hr = MFInitMediaTypeFromVideoInfoHeader2(media_type, &vih, sizeof(vih), &GUID_NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT64(media_type, &MF_MT_PIXEL_ASPECT_RATIO, &value64);
    todo_wine
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    value64 = 0xdeadbeef;
    vih.dwPictAspectRatioY = 456;
    hr = MFInitMediaTypeFromVideoInfoHeader2(media_type, &vih, sizeof(vih), &GUID_NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT64(media_type, &MF_MT_PIXEL_ASPECT_RATIO, &value64);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine
    ok(value64 == ((UINT64)41 << 32 | 76), "Unexpected value %#I64x.\n", value64);

    vih.dwControlFlags = AMCONTROL_COLORINFO_PRESENT;
    hr = MFInitMediaTypeFromVideoInfoHeader2(media_type, &vih, sizeof(vih), &GUID_NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_YUV_MATRIX, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    value32 = 0xdeadbeef;
    vih.dwControlFlags = AMCONTROL_COLORINFO_PRESENT | (MFVideoTransferMatrix_SMPTE240M << 15);
    hr = MFInitMediaTypeFromVideoInfoHeader2(media_type, &vih, sizeof(vih), &GUID_NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_YUV_MATRIX, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == MFVideoTransferMatrix_SMPTE240M, "Unexpected value %#x.\n", value32);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_VIDEO_NOMINAL_RANGE, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    value32 = 0xdeadbeef;
    vih.dwControlFlags = AMCONTROL_COLORINFO_PRESENT | (MFVideoTransferMatrix_SMPTE240M << 15) | (MFNominalRange_Wide << 12);
    hr = MFInitMediaTypeFromVideoInfoHeader2(media_type, &vih, sizeof(vih), &GUID_NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_VIDEO_NOMINAL_RANGE, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == MFNominalRange_Wide, "Unexpected value %#x.\n", value32);

    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_FIXED_SIZE_SAMPLES, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_ALL_SAMPLES_INDEPENDENT, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    hr = MFInitMediaTypeFromVideoInfoHeader2(media_type, &vih, sizeof(vih), &MFVideoFormat_NV12);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_FIXED_SIZE_SAMPLES, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == 1, "Unexpected value %#x.\n", value32);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_ALL_SAMPLES_INDEPENDENT, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == 1, "Unexpected value %#x.\n", value32);

    hr = IMFMediaType_GetBlob(media_type, &MF_MT_MINIMUM_DISPLAY_APERTURE, (BYTE *)&aperture, sizeof(aperture), &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetBlob(media_type, &MF_MT_GEOMETRIC_APERTURE, (BYTE *)&aperture, sizeof(aperture), &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetBlob(media_type, &MF_MT_PAN_SCAN_APERTURE, (BYTE *)&aperture, sizeof(aperture), &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_PAN_SCAN_ENABLED, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    /* only rcSource is considered, translated into both MF_MT_MINIMUM_DISPLAY_APERTURE and MF_MT_PAN_SCAN_APERTURE */
    value32 = 0xdeadbeef;
    vih.rcSource = source;
    vih.rcTarget = target;
    hr = MFInitMediaTypeFromVideoInfoHeader2(media_type, &vih, sizeof(vih), &GUID_NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT64(media_type, &MF_MT_FRAME_SIZE, &value64);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value64 == ((UINT64)16 << 32 | 32), "Unexpected value %#I64x.\n", value64);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_SAMPLE_SIZE, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == 12345, "Unexpected value %#x.\n", value32);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_DEFAULT_STRIDE, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    value32 = 0xdeadbeef;
    memset(&aperture, 0xcd, sizeof(aperture));
    hr = IMFMediaType_GetBlob(media_type, &MF_MT_MINIMUM_DISPLAY_APERTURE, (BYTE *)&aperture, sizeof(aperture), &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == sizeof(aperture), "got %d.\n", value32);
    ok(!memcmp(&aperture, &expect_aperture, sizeof(aperture)), "unexpected aperture\n");

    value32 = 0xdeadbeef;
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_PAN_SCAN_ENABLED, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == 1, "got %d.\n", (UINT32)value32);

    value32 = 0xdeadbeef;
    memset(&aperture, 0xcd, sizeof(aperture));
    hr = IMFMediaType_GetBlob(media_type, &MF_MT_PAN_SCAN_APERTURE, (BYTE *)&aperture, sizeof(aperture), &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == sizeof(aperture), "got %d.\n", value32);
    ok(!memcmp(&aperture, &expect_aperture, sizeof(aperture)), "unexpected aperture\n");

    hr = IMFMediaType_GetItem(media_type, &MF_MT_GEOMETRIC_APERTURE, NULL);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);


    /* biBitCount is used for implicit RGB format if GUID is NULL */
    hr = MFInitMediaTypeFromVideoInfoHeader2(media_type, &vih, sizeof(vih), NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    for (vih.bmiHeader.biBitCount = 1; vih.bmiHeader.biBitCount <= 32; vih.bmiHeader.biBitCount++)
    {
        winetest_push_context("%u", vih.bmiHeader.biBitCount);

        hr = MFInitMediaTypeFromVideoInfoHeader2(media_type, &vih, sizeof(vih), NULL);
        if (vih.bmiHeader.biBitCount != 1 && vih.bmiHeader.biBitCount != 4 && vih.bmiHeader.biBitCount != 8
                && vih.bmiHeader.biBitCount != 16 && vih.bmiHeader.biBitCount != 24 && vih.bmiHeader.biBitCount != 32)
            ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
        else
        {
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

            memset(&guid, 0xcd, sizeof(guid));
            hr = IMFMediaType_GetGUID(media_type, &MF_MT_SUBTYPE, &guid);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            if (vih.bmiHeader.biBitCount == 32)
                ok(IsEqualGUID(&guid, &MFVideoFormat_RGB32), "Unexpected guid %s.\n", debugstr_guid(&guid));
            else if (vih.bmiHeader.biBitCount == 24)
                ok(IsEqualGUID(&guid, &MFVideoFormat_RGB24), "Unexpected guid %s.\n", debugstr_guid(&guid));
            else if (vih.bmiHeader.biBitCount == 16)
                ok(IsEqualGUID(&guid, &MFVideoFormat_RGB555), "Unexpected guid %s.\n", debugstr_guid(&guid));
            else if (vih.bmiHeader.biBitCount == 8)
                ok(IsEqualGUID(&guid, &MFVideoFormat_RGB8), "Unexpected guid %s.\n", debugstr_guid(&guid));
            else if (vih.bmiHeader.biBitCount == 4)
                ok(IsEqualGUID(&guid, &MFVideoFormat_RGB4), "Unexpected guid %s.\n", debugstr_guid(&guid));
            else if (vih.bmiHeader.biBitCount == 1)
                ok(IsEqualGUID(&guid, &MFVideoFormat_RGB1), "Unexpected guid %s.\n", debugstr_guid(&guid));

            value32 = 0xdeadbeef;
            hr = IMFMediaType_GetUINT32(media_type, &MF_MT_DEFAULT_STRIDE, &value32);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            if (vih.bmiHeader.biBitCount > 1)
                ok(value32 == 16 * vih.bmiHeader.biBitCount / 8, "Unexpected value %#x.\n", value32);
            else
                todo_wine ok(value32 == -4, "Unexpected value %#x.\n", value32);

            hr = IMFMediaType_GetItem(media_type, &MF_MT_PALETTE, NULL);
            if (vih.bmiHeader.biBitCount > 1)
                ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
            else
                todo_wine ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

            value32 = 0xdeadbeef;
            hr = IMFMediaType_GetUINT32(media_type, &MF_MT_FIXED_SIZE_SAMPLES, &value32);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            ok(value32 == 1, "Unexpected value %#x.\n", value32);
            value32 = 0xdeadbeef;
            hr = IMFMediaType_GetUINT32(media_type, &MF_MT_ALL_SAMPLES_INDEPENDENT, &value32);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            ok(value32 == 1, "Unexpected value %#x.\n", value32);

            value32 = 0xdeadbeef;
            vih.bmiHeader.biHeight = 32;
            hr = MFInitMediaTypeFromVideoInfoHeader2(media_type, &vih, sizeof(vih), NULL);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            hr = IMFMediaType_GetUINT32(media_type, &MF_MT_DEFAULT_STRIDE, &value32);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            if (vih.bmiHeader.biBitCount > 1)
                ok(value32 == -16 * vih.bmiHeader.biBitCount / 8, "Unexpected value %#x.\n", value32);
            else
                todo_wine ok(value32 == -4, "Unexpected value %#x.\n", value32);

            vih.bmiHeader.biHeight = -32;
        }

        winetest_pop_context();
    }

    hr = IMFMediaType_GetItem(media_type, &MF_MT_USER_DATA, NULL);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetItem(media_type, &MF_MT_PALETTE, NULL);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    *vih_buf = vih;
    vih_buf->bmiHeader.biCompression = BI_RGB;
    vih_buf->bmiHeader.biBitCount = 24;
    hr = MFInitMediaTypeFromVideoInfoHeader2(media_type, vih_buf, sizeof(*vih_buf), NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    memcpy( vih_buf + 1, expect_palette, sizeof(expect_palette) );
    vih_buf->bmiHeader.biClrUsed = ARRAY_SIZE(expect_palette);

    /* palette only works with 8bit RGB format */
    vih_buf->bmiHeader.biSize = sizeof(vih_buf->bmiHeader) + sizeof(expect_palette);
    hr = MFInitMediaTypeFromVideoInfoHeader2(media_type, vih_buf, sizeof(*vih_buf) + sizeof(expect_palette), NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetItem(media_type, &MF_MT_PALETTE, NULL);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    value32 = 0xdeadbeef;
    memset(palette, 0xcd, sizeof(palette));
    hr = IMFMediaType_GetBlob(media_type, &MF_MT_USER_DATA, (BYTE *)palette, sizeof(palette), &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == sizeof(expect_palette), "got %u.\n", value32);
    ok(!memcmp(palette, expect_palette, value32), "Unexpected user data.\n");

    vih_buf->bmiHeader.biBitCount = 16;
    hr = MFInitMediaTypeFromVideoInfoHeader2(media_type, vih_buf, sizeof(*vih_buf) + sizeof(expect_palette), NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetItem(media_type, &MF_MT_PALETTE, NULL);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    value32 = 0xdeadbeef;
    memset(palette, 0xcd, sizeof(palette));
    hr = IMFMediaType_GetBlob(media_type, &MF_MT_USER_DATA, (BYTE *)palette, sizeof(palette), &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == sizeof(expect_palette), "got %u.\n", value32);
    ok(!memcmp(palette, expect_palette, value32), "Unexpected user data.\n");

    /* palette shouldn't be accounted for in the header size */
    vih_buf->bmiHeader.biBitCount = 8;
    hr = MFInitMediaTypeFromVideoInfoHeader2(media_type, vih_buf, sizeof(*vih_buf) + sizeof(expect_palette), NULL);
    todo_wine ok(hr == MF_E_INVALIDMEDIATYPE, "Unexpected hr %#lx.\n", hr);
    vih_buf->bmiHeader.biSize = sizeof(vih_buf->bmiHeader);
    hr = MFInitMediaTypeFromVideoInfoHeader2(media_type, vih_buf, sizeof(*vih_buf) + sizeof(expect_palette), NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetItem(media_type, &MF_MT_USER_DATA, NULL);
    todo_wine ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    value32 = 0xdeadbeef;
    memset(palette, 0xcd, sizeof(palette));
    hr = IMFMediaType_GetBlob(media_type, &MF_MT_PALETTE, (BYTE *)palette, sizeof(palette), &value32);
    todo_wine ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine ok(value32 == sizeof(expect_palette), "got %u.\n", value32);
    todo_wine ok(!memcmp(palette, expect_palette, value32), "Unexpected palette.\n");

    /* cannot have both user data and palette */
    memcpy( vih_buf + 1, expect_user_data, sizeof(expect_user_data) );
    memcpy( (BYTE *)(vih_buf + 1) + sizeof(expect_user_data), expect_palette, sizeof(expect_palette) );
    vih_buf->bmiHeader.biSize = sizeof(vih_buf->bmiHeader) + sizeof(expect_user_data);
    hr = MFInitMediaTypeFromVideoInfoHeader2(media_type, vih_buf, sizeof(*vih_buf) + sizeof(expect_user_data) + sizeof(expect_palette), NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetItem(media_type, &MF_MT_USER_DATA, NULL);
    todo_wine ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    value32 = 0xdeadbeef;
    memset(palette, 0xcd, sizeof(palette));
    hr = IMFMediaType_GetBlob(media_type, &MF_MT_PALETTE, (BYTE *)palette, sizeof(palette), &value32);
    todo_wine ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine ok(value32 == sizeof(expect_palette), "got %u.\n", value32);
    todo_wine ok(!memcmp(palette, expect_palette, value32), "Unexpected palette.\n");

    IMFMediaType_Release(media_type);
}

static void test_MFInitMediaTypeFromMPEG1VideoInfo(void)
{
    IMFMediaType *media_type;
    MPEG1VIDEOINFO vih;
    BYTE buffer[64];
    UINT32 value32;
    UINT64 value64;
    HRESULT hr;
    GUID guid;

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    memset(&vih, 0, sizeof(vih));
    hr = MFInitMediaTypeFromMPEG1VideoInfo(media_type, &vih, 0, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    hr = MFInitMediaTypeFromMPEG1VideoInfo(media_type, &vih, sizeof(vih), NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = MFInitMediaTypeFromMPEG1VideoInfo(media_type, &vih, sizeof(vih), &GUID_NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetGUID(media_type, &MF_MT_MAJOR_TYPE, &guid);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&guid, &MFMediaType_Video), "Unexpected guid %s.\n", debugstr_guid(&guid));
    hr = IMFMediaType_GetGUID(media_type, &MF_MT_SUBTYPE, &guid);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&guid, &GUID_NULL), "Unexpected guid %s.\n", debugstr_guid(&guid));
    hr = IMFMediaType_GetUINT64(media_type, &MF_MT_FRAME_SIZE, &value64);
    todo_wine
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    vih.hdr.bmiHeader.biWidth = 16;
    hr = MFInitMediaTypeFromMPEG1VideoInfo(media_type, &vih, sizeof(vih), &GUID_NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT64(media_type, &MF_MT_FRAME_SIZE, &value64);
    todo_wine
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT64(media_type, &MF_MT_PIXEL_ASPECT_RATIO, &value64);
    todo_wine
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_INTERLACE_MODE, &value32);
    todo_wine
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    vih.hdr.bmiHeader.biHeight = -32;
    hr = MFInitMediaTypeFromMPEG1VideoInfo(media_type, &vih, sizeof(vih), &GUID_NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT64(media_type, &MF_MT_FRAME_SIZE, &value64);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value64 == ((UINT64)16 << 32 | 32), "Unexpected value %#I64x.\n", value64);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_DEFAULT_STRIDE, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    vih.hdr.bmiHeader.biHeight = 32;
    hr = MFInitMediaTypeFromMPEG1VideoInfo(media_type, &vih, sizeof(vih), &GUID_NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT64(media_type, &MF_MT_FRAME_SIZE, &value64);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value64 == ((UINT64)16 << 32 | 32), "Unexpected value %#I64x.\n", value64);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_INTERLACE_MODE, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == MFVideoInterlace_Progressive, "Unexpected value %#x.\n", value32);
    hr = IMFMediaType_GetUINT64(media_type, &MF_MT_PIXEL_ASPECT_RATIO, &value64);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value64 == ((UINT64)1 << 32 | 1), "Unexpected value %#I64x.\n", value64);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_DEFAULT_STRIDE, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_SAMPLE_SIZE, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    vih.hdr.bmiHeader.biXPelsPerMeter = 2;
    vih.hdr.bmiHeader.biYPelsPerMeter = 3;
    hr = MFInitMediaTypeFromMPEG1VideoInfo(media_type, &vih, sizeof(vih), &GUID_NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT64(media_type, &MF_MT_PIXEL_ASPECT_RATIO, &value64);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value64 == ((UINT64)1 << 32 | 1), "Unexpected value %#I64x.\n", value64);

    value32 = 0xdeadbeef;
    vih.hdr.bmiHeader.biSizeImage = 12345;
    hr = MFInitMediaTypeFromMPEG1VideoInfo(media_type, &vih, sizeof(vih), &GUID_NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_SAMPLE_SIZE, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == 12345, "Unexpected value %#x.\n", value32);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_MPEG_START_TIME_CODE, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    value32 = 0xdeadbeef;
    vih.dwStartTimeCode = 1234;
    hr = MFInitMediaTypeFromMPEG1VideoInfo(media_type, &vih, sizeof(vih), &GUID_NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_MPEG_START_TIME_CODE, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == 1234, "Unexpected value %#x.\n", value32);
    hr = IMFMediaType_GetItem(media_type, &MF_MT_MPEG_SEQUENCE_HEADER, NULL);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    value32 = 0xdeadbeef;
    memset(buffer, 0xcd, sizeof(buffer));
    vih.cbSequenceHeader = 1;
    vih.bSequenceHeader[0] = 0xad;
    hr = MFInitMediaTypeFromMPEG1VideoInfo(media_type, &vih, sizeof(vih), &GUID_NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetBlob(media_type, &MF_MT_MPEG_SEQUENCE_HEADER, buffer, sizeof(buffer), &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == 1, "Unexpected value %#x.\n", value32);
    ok(buffer[0] == 0xad, "Unexpected value %#x.\n", buffer[0]);

    IMFMediaType_Release(media_type);
}

static void test_MFInitMediaTypeFromMPEG2VideoInfo(void)
{
    IMFMediaType *media_type;
    MPEG2VIDEOINFO vih;
    DWORD buffer[64];
    UINT32 value32;
    UINT64 value64;
    HRESULT hr;
    GUID guid;

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    memset(&vih, 0, sizeof(vih));
    hr = MFInitMediaTypeFromMPEG2VideoInfo(media_type, &vih, 0, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    hr = MFInitMediaTypeFromMPEG2VideoInfo(media_type, &vih, sizeof(vih), NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = MFInitMediaTypeFromMPEG2VideoInfo(media_type, &vih, sizeof(vih), &GUID_NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetGUID(media_type, &MF_MT_MAJOR_TYPE, &guid);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&guid, &MFMediaType_Video), "Unexpected guid %s.\n", debugstr_guid(&guid));
    hr = IMFMediaType_GetGUID(media_type, &MF_MT_SUBTYPE, &guid);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&guid, &GUID_NULL), "Unexpected guid %s.\n", debugstr_guid(&guid));
    hr = IMFMediaType_GetUINT64(media_type, &MF_MT_FRAME_SIZE, &value64);
    todo_wine
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    vih.hdr.bmiHeader.biWidth = 16;
    hr = MFInitMediaTypeFromMPEG2VideoInfo(media_type, &vih, sizeof(vih), &GUID_NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT64(media_type, &MF_MT_FRAME_SIZE, &value64);
    todo_wine
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT64(media_type, &MF_MT_PIXEL_ASPECT_RATIO, &value64);
    todo_wine
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_INTERLACE_MODE, &value32);
    todo_wine
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    vih.hdr.bmiHeader.biHeight = -32;
    hr = MFInitMediaTypeFromMPEG2VideoInfo(media_type, &vih, sizeof(vih), &GUID_NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT64(media_type, &MF_MT_FRAME_SIZE, &value64);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value64 == ((UINT64)16 << 32 | 32), "Unexpected value %#I64x.\n", value64);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_DEFAULT_STRIDE, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    vih.hdr.bmiHeader.biHeight = 32;
    hr = MFInitMediaTypeFromMPEG2VideoInfo(media_type, &vih, sizeof(vih), &GUID_NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT64(media_type, &MF_MT_FRAME_SIZE, &value64);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value64 == ((UINT64)16 << 32 | 32), "Unexpected value %#I64x.\n", value64);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_INTERLACE_MODE, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == MFVideoInterlace_Progressive, "Unexpected value %#x.\n", value32);
    hr = IMFMediaType_GetUINT64(media_type, &MF_MT_PIXEL_ASPECT_RATIO, &value64);
    todo_wine
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_DEFAULT_STRIDE, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_SAMPLE_SIZE, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    vih.hdr.bmiHeader.biXPelsPerMeter = 2;
    vih.hdr.bmiHeader.biYPelsPerMeter = 3;
    hr = MFInitMediaTypeFromMPEG2VideoInfo(media_type, &vih, sizeof(vih), &GUID_NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT64(media_type, &MF_MT_PIXEL_ASPECT_RATIO, &value64);
    todo_wine
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    value32 = 0xdeadbeef;
    vih.hdr.bmiHeader.biSizeImage = 12345;
    hr = MFInitMediaTypeFromMPEG2VideoInfo(media_type, &vih, sizeof(vih), &GUID_NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_SAMPLE_SIZE, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == 12345, "Unexpected value %#x.\n", value32);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_MPEG_START_TIME_CODE, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    value32 = 0xdeadbeef;
    vih.dwStartTimeCode = 1234;
    hr = MFInitMediaTypeFromMPEG2VideoInfo(media_type, &vih, sizeof(vih), &GUID_NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_MPEG_START_TIME_CODE, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == 1234, "Unexpected value %#x.\n", value32);
    hr = IMFMediaType_GetItem(media_type, &MF_MT_MPEG2_PROFILE, NULL);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    value32 = 0xdeadbeef;
    vih.dwProfile = 1234;
    hr = MFInitMediaTypeFromMPEG2VideoInfo(media_type, &vih, sizeof(vih), &GUID_NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_MPEG2_PROFILE, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == 1234, "Unexpected value %#x.\n", value32);
    hr = IMFMediaType_GetItem(media_type, &MF_MT_MPEG2_LEVEL, NULL);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    value32 = 0xdeadbeef;
    vih.dwLevel = 1234;
    hr = MFInitMediaTypeFromMPEG2VideoInfo(media_type, &vih, sizeof(vih), &GUID_NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_MPEG2_LEVEL, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == 1234, "Unexpected value %#x.\n", value32);
    hr = IMFMediaType_GetItem(media_type, &MF_MT_MPEG2_FLAGS, NULL);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    value32 = 0xdeadbeef;
    vih.dwFlags = 1234;
    hr = MFInitMediaTypeFromMPEG2VideoInfo(media_type, &vih, sizeof(vih), &GUID_NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_MPEG2_FLAGS, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == 1234, "Unexpected value %#x.\n", value32);
    hr = IMFMediaType_GetItem(media_type, &MF_MT_MPEG_SEQUENCE_HEADER, NULL);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    value32 = 0xdeadbeef;
    memset(buffer, 0xcd, sizeof(buffer));
    vih.cbSequenceHeader = 3;
    vih.dwSequenceHeader[0] = 0xabcdef;
    hr = MFInitMediaTypeFromMPEG2VideoInfo(media_type, &vih, sizeof(vih), &GUID_NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetBlob(media_type, &MF_MT_MPEG_SEQUENCE_HEADER, (BYTE *)buffer, sizeof(buffer), &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == 3, "Unexpected value %#x.\n", value32);
    ok(buffer[0] == 0xcdabcdef, "Unexpected value %#lx.\n", buffer[0]);

    IMFMediaType_Release(media_type);
}

static void test_MFInitMediaTypeFromAMMediaType(void)
{
    static const MFVideoArea expect_aperture = {.OffsetX = {.value = 13}, .OffsetY = {.value = 46}, .Area = {.cx = 110, .cy = 410}};
    static const RECT source = {13, 46, 123, 456}, target = {25, 34, 107, 409};
    IMFMediaType *media_type;
    AM_MEDIA_TYPE mt;
    UINT32 value32;
    UINT64 value64;
    HRESULT hr;
    GUID guid;
    VIDEOINFOHEADER vih =
    {
        {0}, {0}, 0, 0, 0,
        {sizeof(BITMAPINFOHEADER), 32, 24, 1, 0, 0xdeadbeef}
    };
    static const struct guid_type_pair
    {
        const GUID *am_type;
        const GUID *mf_type;
    } guid_types[] =
    {
        /* these RGB formats are converted, MEDIASUBTYPE variant isn't
         * defined using DEFINE_MEDIATYPE_GUID */
        { &MEDIASUBTYPE_RGB1, &MFVideoFormat_RGB1 },
        { &MEDIASUBTYPE_RGB4, &MFVideoFormat_RGB4 },
        { &MEDIASUBTYPE_RGB8, &MFVideoFormat_RGB8 },
        { &MEDIASUBTYPE_RGB555, &MFVideoFormat_RGB555 },
        { &MEDIASUBTYPE_RGB565, &MFVideoFormat_RGB565 },
        { &MEDIASUBTYPE_RGB24, &MFVideoFormat_RGB24 },
        { &MEDIASUBTYPE_RGB32, &MFVideoFormat_RGB32 },
        { &MEDIASUBTYPE_ARGB1555, &MFVideoFormat_ARGB1555 },
        { &MEDIASUBTYPE_ARGB4444, &MFVideoFormat_ARGB4444 },
        { &MEDIASUBTYPE_ARGB32, &MFVideoFormat_ARGB32 },
        { &MEDIASUBTYPE_A2R10G10B10, &MFVideoFormat_A2B10G10R10 },
        { &MEDIASUBTYPE_A2B10G10R10, &MFVideoFormat_A2R10G10B10 },

        /* any other GUID is passed through */
        { &MEDIASUBTYPE_I420, &MFVideoFormat_I420 },
        { &MEDIASUBTYPE_AYUV, &MFVideoFormat_AYUV },
        { &MEDIASUBTYPE_YV12, &MFVideoFormat_YV12 },
        { &MEDIASUBTYPE_YUY2, &MFVideoFormat_YUY2 },
        { &MEDIASUBTYPE_UYVY, &MFVideoFormat_UYVY },
        { &MEDIASUBTYPE_YVYU, &MFVideoFormat_YVYU },
        { &MEDIASUBTYPE_NV12, &MFVideoFormat_NV12 },

        /* even formats that don't exist in MF */
        { &DUMMY_GUID3, &DUMMY_GUID3 },
        { &MEDIASUBTYPE_NV24, &MEDIASUBTYPE_NV24 },
        { &MEDIASUBTYPE_P208, &MEDIASUBTYPE_P208 },

        /* if the mapping is ambiguous, it is not corrected */
        { &MEDIASUBTYPE_h264, &MEDIASUBTYPE_h264 },
        { &MEDIASUBTYPE_H264, &MFVideoFormat_H264 },
    };
    static const GUID *audio_types[] =
    {
        &MEDIASUBTYPE_MP3,
        &MEDIASUBTYPE_MSAUDIO1,
        &MEDIASUBTYPE_WMAUDIO2,
        &MEDIASUBTYPE_WMAUDIO3,
        &MEDIASUBTYPE_WMAUDIO_LOSSLESS,
        &MEDIASUBTYPE_PCM,
        &MEDIASUBTYPE_IEEE_FLOAT,
        &DUMMY_CLSID,
    };
    MFVideoArea aperture;
    unsigned int i;

    hr = MFCreateMediaType(&media_type);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    memset(&mt, 0, sizeof(mt));
    mt.majortype = MEDIATYPE_Audio;

    for (i = 0; i < ARRAY_SIZE(audio_types); i++)
    {
        mt.subtype = *audio_types[i];

        hr = MFInitMediaTypeFromAMMediaType(media_type, &mt);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IMFMediaType_GetCount(media_type, &value32);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(value32 == 4, "Unexpected value %#x.\n", value32);

        hr = IMFMediaType_GetGUID(media_type, &MF_MT_MAJOR_TYPE, &guid);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(IsEqualGUID(&guid, &MFMediaType_Audio), "Unexpected guid %s.\n", debugstr_guid(&guid));
        hr = IMFMediaType_GetGUID(media_type, &MF_MT_SUBTYPE, &guid);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(IsEqualGUID(&guid, audio_types[i]), "Unexpected guid %s.\n", debugstr_guid(&guid));
        hr = IMFMediaType_GetUINT32(media_type, &MF_MT_ALL_SAMPLES_INDEPENDENT, &value32);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(value32 == 1, "Unexpected value %#x.\n", value32);
        hr = IMFMediaType_GetGUID(media_type, &MF_MT_AM_FORMAT_TYPE, &guid);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(IsEqualGUID(&guid, &GUID_NULL), "Unexpected guid %s.\n", debugstr_guid(&guid));
    }

    memset(&mt, 0, sizeof(mt));
    mt.majortype = MEDIATYPE_Video;
    mt.formattype = FORMAT_VideoInfo;
    mt.cbFormat = sizeof(VIDEOINFOHEADER);
    mt.pbFormat = (BYTE *)&vih;

    hr = IMFMediaType_SetUINT32(media_type, &MF_MT_DEFAULT_STRIDE, 123);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = MFInitMediaTypeFromAMMediaType(media_type, &mt);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaType_GetGUID(media_type, &MF_MT_MAJOR_TYPE, &guid);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&guid, &MFMediaType_Video), "Unexpected guid %s.\n", debugstr_guid(&guid));
    hr = IMFMediaType_GetGUID(media_type, &MF_MT_SUBTYPE, &guid);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&guid, &GUID_NULL), "Unexpected guid %s.\n", debugstr_guid(&guid));
    hr = IMFMediaType_GetUINT64(media_type, &MF_MT_FRAME_SIZE, &value64);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value64 == ((UINT64)32 << 32 | 24), "Unexpected value %#I64x.\n", value64);
    hr = IMFMediaType_GetUINT64(media_type, &MF_MT_PIXEL_ASPECT_RATIO, &value64);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value64 == ((UINT64)1 << 32 | 1), "Unexpected value %#I64x.\n", value64);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_INTERLACE_MODE, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == MFVideoInterlace_Progressive, "Unexpected value %#x.\n", value32);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_ALL_SAMPLES_INDEPENDENT, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!value32, "Unexpected value %#x.\n", value32);

    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_DEFAULT_STRIDE, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_FIXED_SIZE_SAMPLES, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_SAMPLE_SIZE, &value32);
    ok(hr == MF_E_ATTRIBUTENOTFOUND, "Unexpected hr %#lx.\n", hr);

    vih.bmiHeader.biHeight = -24;

    hr = MFInitMediaTypeFromAMMediaType(media_type, &mt);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMFMediaType_GetUINT64(media_type, &MF_MT_FRAME_SIZE, &value64);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value64 == ((UINT64)32 << 32 | 24), "Unexpected value %#I64x.\n", value64);

    memcpy(&mt.subtype, &MEDIASUBTYPE_RGB32, sizeof(GUID));
    vih.bmiHeader.biHeight = 24;

    hr = MFInitMediaTypeFromAMMediaType(media_type, &mt);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaType_GetGUID(media_type, &MF_MT_MAJOR_TYPE, &guid);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&guid, &MFMediaType_Video), "Unexpected guid %s.\n", debugstr_guid(&guid));
    hr = IMFMediaType_GetGUID(media_type, &MF_MT_SUBTYPE, &guid);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&guid, &MFVideoFormat_RGB32), "Unexpected guid %s.\n", debugstr_guid(&guid));
    hr = IMFMediaType_GetUINT64(media_type, &MF_MT_FRAME_SIZE, &value64);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value64 == ((UINT64)32 << 32 | 24), "Unexpected value %#I64x.\n", value64);
    hr = IMFMediaType_GetUINT64(media_type, &MF_MT_PIXEL_ASPECT_RATIO, &value64);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value64 == ((UINT64)1 << 32 | 1), "Unexpected value %#I64x.\n", value64);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_INTERLACE_MODE, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == MFVideoInterlace_Progressive, "Unexpected value %#x.\n", value32);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_SAMPLE_SIZE, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == 3072, "Unexpected value %u.\n", value32);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_ALL_SAMPLES_INDEPENDENT, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!value32, "Unexpected value %#x.\n", value32);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_FIXED_SIZE_SAMPLES, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!value32, "Unexpected value %u.\n", value32);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_DEFAULT_STRIDE, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == -128, "Unexpected value %d.\n", value32);

    /* Negative height. */
    vih.bmiHeader.biHeight = -24;

    hr = MFInitMediaTypeFromAMMediaType(media_type, &mt);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaType_GetGUID(media_type, &MF_MT_MAJOR_TYPE, &guid);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&guid, &MFMediaType_Video), "Unexpected guid %s.\n", debugstr_guid(&guid));
    hr = IMFMediaType_GetGUID(media_type, &MF_MT_SUBTYPE, &guid);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&guid, &MFVideoFormat_RGB32), "Unexpected guid %s.\n", debugstr_guid(&guid));
    hr = IMFMediaType_GetUINT64(media_type, &MF_MT_FRAME_SIZE, &value64);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value64 == ((UINT64)32 << 32 | 24), "Unexpected value %#I64x.\n", value64);
    hr = IMFMediaType_GetUINT64(media_type, &MF_MT_PIXEL_ASPECT_RATIO, &value64);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value64 == ((UINT64)1 << 32 | 1), "Unexpected value %#I64x.\n", value64);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_INTERLACE_MODE, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == MFVideoInterlace_Progressive, "Unexpected value %#x.\n", value32);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_SAMPLE_SIZE, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == 3072, "Unexpected value %u.\n", value32);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_ALL_SAMPLES_INDEPENDENT, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!value32, "Unexpected value %#x.\n", value32);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_FIXED_SIZE_SAMPLES, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!value32, "Unexpected value %u.\n", value32);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_DEFAULT_STRIDE, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == 128, "Unexpected value %d.\n", value32);

    /* only rcSource is considered, lSampleSize is ignored if biSizeImage was present */
    memcpy(&mt.subtype, &MEDIASUBTYPE_RGB32, sizeof(GUID));
    vih.rcSource = source;
    vih.rcTarget = target;
    vih.bmiHeader.biWidth = 432;
    vih.bmiHeader.biHeight = -654;
    vih.bmiHeader.biSizeImage = 12345678;
    mt.lSampleSize = 87654321;
    hr = MFInitMediaTypeFromAMMediaType(media_type, &mt);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMFMediaType_GetUINT64(media_type, &MF_MT_FRAME_SIZE, &value64);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok((UINT32)(value64 >> 32) == 432, "got %d.\n", (UINT32)(value64 >> 32));
    ok((UINT32)value64 == 654, "got %d.\n", (UINT32)value64);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_DEFAULT_STRIDE, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == 432 * 4, "got %d.\n", (UINT32)value32);
    hr = IMFMediaType_GetUINT32(media_type, &MF_MT_SAMPLE_SIZE, &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == 12345678, "got %d.\n", (UINT32)value32);
    hr = IMFMediaType_GetBlob(media_type, &MF_MT_MINIMUM_DISPLAY_APERTURE, (BYTE *)&aperture, sizeof(aperture), &value32);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(value32 == sizeof(aperture), "got %d.\n", value32);
    ok(!memcmp(&aperture, &expect_aperture, sizeof(aperture)), "unexpected aperture\n");

    vih.bmiHeader.biHeight = 24;
    for (i = 0; i < ARRAY_SIZE(guid_types); ++i)
    {
        winetest_push_context("%s", debugstr_guid(guid_types[i].am_type));
        memcpy(&mt.subtype, guid_types[i].am_type, sizeof(GUID));

        hr = MFInitMediaTypeFromAMMediaType(media_type, &mt);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IMFMediaType_GetGUID(media_type, &MF_MT_MAJOR_TYPE, &guid);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(IsEqualGUID(&guid, &MFMediaType_Video), "Unexpected guid %s.\n", debugstr_guid(&guid));
        hr = IMFMediaType_GetGUID(media_type, &MF_MT_SUBTYPE, &guid);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(IsEqualGUID(&guid, guid_types[i].mf_type), "Unexpected guid %s.\n", debugstr_guid(&guid));
        winetest_pop_context();
    }

    IMFMediaType_Release(media_type);
}

static void test_MFCreatePathFromURL(void)
{
    static const struct
    {
        const WCHAR *url;
        const WCHAR *path;
        HRESULT hr;
    }
    tests[] =
    {
        /* 0 leading slash */
        { L"file:c:/foo/bar", L"c:\\foo\\bar" },
        { L"file:c|/foo/bar", L"c:\\foo\\bar" },
        { L"file:cx|/foo/bar", L"cx|\\foo\\bar" },
        { L"file:c:foo/bar", L"c:foo\\bar" },
        { L"file:c|foo/bar", L"c:foo\\bar" },
        { L"file:c:/foo%20ba%2fr", L"c:\\foo ba/r" },
        { L"file:foo%20ba%2fr", L"foo ba/r" },
        { L"file:foo/bar/", L"foo\\bar\\" },

        /* 1 leading (back)slash */
        { L"file:/c:/foo/bar", L"c:\\foo\\bar" },
        { L"file:\\c:/foo/bar", L"c:\\foo\\bar" },
        { L"file:/c|/foo/bar", L"c:\\foo\\bar" },
        { L"file:/cx|/foo/bar", L"\\cx|\\foo\\bar" },
        { L"file:/c:foo/bar", L"c:foo\\bar" },
        { L"file:/c|foo/bar", L"c:foo\\bar" },
        { L"file:/c:/foo%20ba%2fr", L"c:\\foo ba/r" },
        { L"file:/foo%20ba%2fr", L"\\foo ba/r" },
        { L"file:/foo/bar/", L"\\foo\\bar\\" },

        /* 2 leading (back)slashes */
        { L"file://c:/foo/bar", L"c:\\foo\\bar" },
        { L"file://c:/d:/foo/bar", L"c:\\d:\\foo\\bar" },
        { L"file://c|/d|/foo/bar", L"c:\\d|\\foo\\bar" },
        { L"file://cx|/foo/bar", L"\\\\cx|\\foo\\bar" },
        { L"file://c:foo/bar", L"c:foo\\bar" },
        { L"file://c|foo/bar", L"c:foo\\bar" },
        { L"file://c:/foo%20ba%2fr", L"c:\\foo%20ba%2fr" },
        { L"file://c%3a/foo/../bar", L"\\\\c:\\foo\\..\\bar" },
        { L"file://c%7c/foo/../bar", L"\\\\c|\\foo\\..\\bar" },
        { L"file://foo%20ba%2fr", L"\\\\foo ba/r" },
        { L"file://localhost/c:/foo/bar", L"c:\\foo\\bar" },
        { L"file://localhost/c:/foo%20ba%5Cr", L"c:\\foo ba\\r" },
        { L"file://LocalHost/c:/foo/bar", L"c:\\foo\\bar" },
        { L"file:\\\\localhost\\c:\\foo\\bar", L"c:\\foo\\bar" },
        { L"file://incomplete", L"\\\\incomplete" },

        /* 3 leading (back)slashes (omitting hostname) */
        { L"file:///c:/foo/bar", L"c:\\foo\\bar" },
        { L"File:///c:/foo/bar", L"c:\\foo\\bar" },
        { L"file:///c:/foo%20ba%2fr", L"c:\\foo ba/r" },
        { L"file:///foo%20ba%2fr", L"\\foo ba/r" },
        { L"file:///foo/bar/", L"\\foo\\bar\\" },
        { L"file:///localhost/c:/foo/bar", L"\\localhost\\c:\\foo\\bar" },

        /* 4 leading (back)slashes */
        { L"file:////c:/foo/bar", L"c:\\foo\\bar" },
        { L"file:////c:/foo%20ba%2fr", L"c:\\foo%20ba%2fr" },
        { L"file:////foo%20ba%2fr", L"\\\\foo%20ba%2fr" },

        /* 5 and more leading (back)slashes */
        { L"file://///c:/foo/bar", L"\\\\c:\\foo\\bar" },
        { L"file://///c:/foo%20ba%2fr", L"\\\\c:\\foo ba/r" },
        { L"file://///foo%20ba%2fr", L"\\\\foo ba/r" },
        { L"file://////c:/foo/bar", L"\\\\c:\\foo\\bar" },

        /* Leading (back)slashes cannot be escaped */
        { L"file:%2f%2flocalhost%2fc:/foo/bar", L"//localhost/c:\\foo\\bar" },
        { L"file:%5C%5Clocalhost%5Cc:/foo/bar", L"\\\\localhost\\c:\\foo\\bar" },

        /* Hostname handling */
        { L"file://l%6fcalhost/c:/foo/bar", L"\\\\localhostc:\\foo\\bar" },
        { L"file://localhost:80/c:/foo/bar", L"\\\\localhost:80c:\\foo\\bar" },
        { L"file://host/c:/foo/bar", L"\\\\hostc:\\foo\\bar" },
        { L"file://host//c:/foo/bar", L"\\\\host\\\\c:\\foo\\bar" },
        { L"file://host/\\c:/foo/bar", L"\\\\host\\\\c:\\foo\\bar" },
        { L"file://host/c:foo/bar", L"\\\\hostc:foo\\bar" },
        { L"file://host/foo/bar", L"\\\\host\\foo\\bar" },
        { L"file:\\\\host\\c:\\foo\\bar", L"\\\\hostc:\\foo\\bar" },
        { L"file:\\\\host\\ca\\foo\\bar", L"\\\\host\\ca\\foo\\bar" },
        { L"file:\\\\host\\c|\\foo\\bar", L"\\\\hostc|\\foo\\bar" },
        { L"file:\\%5Chost\\c:\\foo\\bar", L"\\\\host\\c:\\foo\\bar" },
        { L"file:\\\\host\\cx:\\foo\\bar", L"\\\\host\\cx:\\foo\\bar" },
        { L"file:///host/c:/foo/bar", L"\\host\\c:\\foo\\bar" },

        /* Not file URLs */
        { L"c:\\foo\\bar", NULL, E_INVALIDARG },
        { L"foo/bar", NULL, E_INVALIDARG },
        { L"http://foo/bar", NULL, E_INVALIDARG },
    };
    unsigned int i;
    WCHAR *path;
    HRESULT hr;

    if (!pMFCreatePathFromURL)
    {
        win_skip("MFCreatePathFromURL() is not available.\n");
        return;
    }

    hr = pMFCreatePathFromURL(NULL, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    path = (void *)0xdeadbeef;
    hr = pMFCreatePathFromURL(NULL, &path);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);
    ok(path == (void *)0xdeadbeef, "Unexpected pointer %p.\n", path);

    hr = pMFCreatePathFromURL(L"file://foo", NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        hr = pMFCreatePathFromURL(tests[i].url, &path);
        ok(hr == tests[i].hr, "Unexpected hr %#lx, expected %#lx.\n", hr, tests[i].hr);
        if (SUCCEEDED(hr))
        {
            ok(!wcscmp(path, tests[i].path), "Unexpected path %s, expected %s.\n",
                    debugstr_w(path), debugstr_w(tests[i].path));
            CoTaskMemFree(path);
        }
    }
}

#define check_reset_data(a, b, c, d, e) check_reset_data_(__LINE__, a, b, c, d, e)
static void check_reset_data_(unsigned int line, IMF2DBuffer2 *buffer2d, const BYTE *data, BOOL bottom_up,
        DWORD width, DWORD height)
{
    BYTE *scanline0, *buffer_start;
    DWORD length, max_length;
    IMFMediaBuffer *buffer;
    LONG pitch;
    BYTE *lock;
    HRESULT hr;
    int i;

    hr = IMF2DBuffer2_QueryInterface(buffer2d, &IID_IMFMediaBuffer, (void **)&buffer);
    ok(hr == S_OK, "got hr %#lx.\n", hr);

    hr = IMF2DBuffer2_Lock2DSize(buffer2d, MF2DBuffer_LockFlags_Read, &scanline0, &pitch, &buffer_start, &length);
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    if (bottom_up)
    {
        ok(pitch < 0, "got pitch %ld.\n", pitch);
        ok(buffer_start == scanline0 + pitch * (LONG)(height - 1), "buffer start mismatch.\n");
    }
    else
    {
        ok(pitch > 0, "got pitch %ld.\n", pitch);
        ok(buffer_start == scanline0, "buffer start mismatch.\n");
    }
    for (i = 0; i < height; ++i)
        ok_(__FILE__,line)(!memcmp(buffer_start + abs(pitch) * i, data + width * i * 4, width * 4),
                "2D Data mismatch, scaline %d.\n", i);
    hr = IMF2DBuffer2_Unlock2D(buffer2d);
    ok(hr == S_OK, "got hr %#lx.\n", hr);

    hr = IMFMediaBuffer_Lock(buffer, &lock, &max_length, &length);
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    ok_(__FILE__,line)(max_length == width * height * 4, "got max_length %lu.\n", max_length);
    ok_(__FILE__,line)(length == width * height * 4, "got length %lu.\n", length);
    ok_(__FILE__,line)(!memcmp(lock, data, length), "contiguous data mismatch.\n");
    memset(lock, 0xcc, length);
    hr =  IMFMediaBuffer_Unlock(buffer);
    ok(hr == S_OK, "got hr %#lx.\n", hr);

    IMFMediaBuffer_Release(buffer);
}

static void test_2dbuffer_copy_(IMFMediaBuffer *buffer, BOOL bottom_up, DWORD width, DWORD height)
{
    static const unsigned int test_data[] =
    {
        0x01010101, 0x01010101,
        0x02020202, 0x02020202,
    };

    BYTE data[sizeof(test_data)];
    IMFMediaBuffer *src_buffer;
    DWORD length, max_length;
    IMF2DBuffer2 *buffer2d;
    IMFSample *sample;
    BYTE *lock;
    HRESULT hr;
    ULONG ref;

    hr = IMFMediaBuffer_QueryInterface(buffer, &IID_IMF2DBuffer2, (void **)&buffer2d);
    ok(hr == S_OK, "got hr %#lx.\n", hr);

    hr = MFCreateSample(&sample);
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    hr = MFCreateMemoryBuffer(sizeof(test_data) * 2, &src_buffer);
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    hr = IMFSample_AddBuffer(sample, src_buffer);
    ok(hr == S_OK, "got hr %#lx.\n", hr);

    hr = IMFMediaBuffer_Lock(src_buffer, &lock, &max_length, &length);
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    ok(max_length == sizeof(test_data) * 2, "got %lu.\n", max_length);
    memcpy(lock, test_data, sizeof(test_data));
    hr =  IMFMediaBuffer_Unlock(src_buffer);
    ok(hr == S_OK, "got hr %#lx.\n", hr);

    hr = IMFMediaBuffer_Lock(buffer, &lock, &max_length, &length);
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    ok(max_length == 16, "got %lu.\n", max_length);
    ok(length == 16, "got %lu.\n", length);
    memset(lock, 0xcc, length);
    hr =  IMFMediaBuffer_Unlock(buffer);
    ok(hr == S_OK, "got hr %#lx.\n", hr);

    hr = IMFMediaBuffer_SetCurrentLength(src_buffer, 1);
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    hr = IMFSample_CopyToBuffer(sample, buffer);
    ok(hr == S_OK, "got hr %#lx.\n", hr);

    memset(data, 0xcc, sizeof(data));
    data[0] = ((BYTE *)test_data)[0];
    check_reset_data(buffer2d, data, bottom_up, width, height);

    hr = IMF2DBuffer2_ContiguousCopyFrom(buffer2d, (BYTE *)test_data, sizeof(test_data));
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    hr = IMF2DBuffer2_ContiguousCopyTo(buffer2d, data, sizeof(data));
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    ok(!memcmp(data, test_data, sizeof(data)), "data mismatch.\n");

    check_reset_data(buffer2d, (const BYTE *)test_data, bottom_up, width, height);

    hr = IMFMediaBuffer_SetCurrentLength(src_buffer, sizeof(test_data) + 1);
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    hr = IMFSample_CopyToBuffer(sample, buffer);
    ok(hr == MF_E_BUFFERTOOSMALL, "got hr %#lx.\n", hr);

    hr = IMFMediaBuffer_SetCurrentLength(src_buffer, sizeof(test_data));
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    hr = IMFSample_CopyToBuffer(sample, buffer);
    ok(hr == S_OK, "got hr %#lx.\n", hr);

    check_reset_data(buffer2d, (const BYTE *)test_data, bottom_up, width, height);

    IMF2DBuffer2_Release(buffer2d);
    ref = IMFSample_Release(sample);
    ok(!ref, "got %lu.\n", ref);
    ref = IMFMediaBuffer_Release(src_buffer);
    ok(!ref, "got %lu.\n", ref);
}

static void test_2dbuffer_copy(void)
{
    D3D11_TEXTURE2D_DESC desc;
    ID3D11Texture2D *texture;
    IMFMediaBuffer *buffer;
    ID3D11Device *device;
    HRESULT hr;
    ULONG ref;

    if (!pMFCreate2DMediaBuffer)
    {
        win_skip("MFCreate2DMediaBuffer() is not available.\n");
        return;
    }

    winetest_push_context("top down");
    hr = pMFCreate2DMediaBuffer(2, 2, D3DFMT_A8R8G8B8, FALSE, &buffer);
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    test_2dbuffer_copy_(buffer, FALSE, 2, 2);
    ref = IMFMediaBuffer_Release(buffer);
    ok(!ref, "got %lu.\n", ref);
    winetest_pop_context();

    winetest_push_context("bottom up");
    hr = pMFCreate2DMediaBuffer(2, 2, D3DFMT_A8R8G8B8, TRUE, &buffer);
    ok(hr == S_OK, "got hr %#lx.\n", hr);
    test_2dbuffer_copy_(buffer, TRUE, 2, 2);
    ref = IMFMediaBuffer_Release(buffer);
    ok(!ref, "got %lu.\n", ref);
    winetest_pop_context();

    if (!pMFCreateDXGISurfaceBuffer)
    {
        win_skip("MFCreateDXGISurfaceBuffer() is not available.\n");
        return;
    }

    if (!(device = create_d3d11_device()))
    {
        skip("Failed to create a D3D11 device, skipping tests.\n");
        return;
    }

    memset(&desc, 0, sizeof(desc));
    desc.Width = 2;
    desc.Height = 2;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    hr = ID3D11Device_CreateTexture2D(device, &desc, NULL, &texture);
    ok(hr == S_OK, "Failed to create a texture, hr %#lx.\n", hr);

    hr = pMFCreateDXGISurfaceBuffer(&IID_ID3D11Texture2D, (IUnknown *)texture, 0, FALSE, &buffer);
    ok(hr == S_OK, "Failed to create a buffer, hr %#lx.\n", hr);
    test_2dbuffer_copy_(buffer, FALSE, 2, 2);

    ID3D11Texture2D_Release(texture);
    ref = IMFMediaBuffer_Release(buffer);
    ok(!ref, "got %lu.\n", ref);
    ID3D11Device_Release(device);
}

static void test_undefined_queue_id(void)
{
    struct test_callback *callback;
    IMFAsyncResult *result;
    HRESULT hr;
    DWORD res;

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "Failed to start up, hr %#lx.\n", hr);

    callback = create_test_callback(&test_async_callback_result_vtbl);

    hr = MFPutWorkItem(MFASYNC_CALLBACK_QUEUE_UNDEFINED, &callback->IMFAsyncCallback_iface, NULL);
    ok(hr == S_OK, "got %#lx\n", hr);
    res = wait_async_callback_result(&callback->IMFAsyncCallback_iface, 100, &result);
    ok(res == 0, "got %#lx\n", res);
    IMFAsyncResult_Release(result);

    hr = MFPutWorkItem(MFASYNC_CALLBACK_QUEUE_PRIVATE_MASK, &callback->IMFAsyncCallback_iface, NULL);
    ok(hr == S_OK, "got %#lx\n", hr);
    res = wait_async_callback_result(&callback->IMFAsyncCallback_iface, 100, &result);
    ok(res == 0, "got %#lx\n", res);
    IMFAsyncResult_Release(result);

    hr = MFPutWorkItem(MFASYNC_CALLBACK_QUEUE_PRIVATE_MASK & (MFASYNC_CALLBACK_QUEUE_PRIVATE_MASK - 1),
            &callback->IMFAsyncCallback_iface, NULL);
    ok(hr == S_OK, "got %#lx\n", hr);
    res = wait_async_callback_result(&callback->IMFAsyncCallback_iface, 100, &result);
    ok(res == 0, "got %#lx\n", res);
    IMFAsyncResult_Release(result);

    IMFAsyncCallback_Release(&callback->IMFAsyncCallback_iface);

    hr = MFShutdown();
    ok(hr == S_OK, "Failed to shut down, hr %#lx.\n", hr);
}

START_TEST(mfplat)
{
    char **argv;
    int argc;

    init_functions();

    argc = winetest_get_mainargs(&argv);
    if (argc >= 3)
    {
        test_queue_com_state(argv[2]);
        return;
    }

    if (!pMFCreateVideoSampleAllocatorEx)
        win_skip("MFCreateVideoSampleAllocatorEx() is not available. Some tests will be skipped.\n");

    if (!pD3D12CreateDevice)
        skip("Missing d3d12 support, some tests will be skipped.\n");

    CoInitialize(NULL);

    test_startup();
    test_startup_counts();
    test_register();
    test_media_type();
    test_MFCreateMediaEvent();
    test_attributes();
    test_sample();
    test_file_stream();
    test_MFCreateMFByteStreamOnStream();
    test_system_memory_buffer();
    test_system_memory_aligned_buffer();
    test_MFCreateLegacyMediaBufferOnMFMediaBuffer();
    test_source_resolver();
    test_MFCreateAsyncResult();
    test_allocate_queue();
    test_MFLockSharedWorkQueue();
    test_MFCopyImage();
    test_MFCreateCollection();
    test_MFHeapAlloc();
    test_scheduled_items();
    test_serial_queue();
    test_periodic_callback();
    test_event_queue();
    test_presentation_descriptor();
    test_system_time_source();
    test_MFInvokeCallback();
    test_stream_descriptor();
    test_MFCalculateImageSize();
    test_MFGetPlaneSize();
    test_MFCompareFullToPartialMediaType();
    test_attributes_serialization();
    test_wrapped_media_type();
    test_MFCreateWaveFormatExFromMFMediaType();
    test_async_create_file();
    test_local_handlers();
    test_create_property_store();
    test_dxgi_device_manager();
    test_MFCreateTransformActivate();
    test_MFTRegisterLocal();
    test_queue_com();
    test_MFGetStrideForBitmapInfoHeader();
    test_MFCreate2DMediaBuffer();
    test_MFCreateMediaBufferFromMediaType();
    test_MFInitMediaTypeFromWaveFormatEx();
    test_MFCreateMFVideoFormatFromMFMediaType();
    test_MFInitAMMediaTypeFromMFMediaType();
    test_MFCreateAMMediaTypeFromMFMediaType();
    test_MFInitMediaTypeFromMFVideoFormat();
    test_IMFMediaType_GetRepresentation();
    test_MFCreateMediaTypeFromRepresentation();
    test_MFCreateDXSurfaceBuffer();
    test_MFCreateTrackedSample();
    test_MFFrameRateToAverageTimePerFrame();
    test_MFAverageTimePerFrameToFrameRate();
    test_MFMapDXGIFormatToDX9Format();
    test_d3d11_surface_buffer();
    test_d3d12_surface_buffer();
    test_sample_allocator_sysmem();
    test_sample_allocator_d3d9();
    test_sample_allocator_d3d11();
    test_sample_allocator_d3d12();
    test_MFMapDX9FormatToDXGIFormat();
    test_MFllMulDiv();
    test_shared_dxgi_device_manager();
    test_MFInitVideoFormat_RGB();
    test_MFCreateVideoMediaTypeFromVideoInfoHeader();
    test_MFInitMediaTypeFromVideoInfoHeader();
    test_MFInitMediaTypeFromVideoInfoHeader2();
    test_MFInitMediaTypeFromMPEG1VideoInfo();
    test_MFInitMediaTypeFromMPEG2VideoInfo();
    test_MFInitMediaTypeFromAMMediaType();
    test_MFCreatePathFromURL();
    test_2dbuffer_copy();
    test_undefined_queue_id();

    CoUninitialize();
}
