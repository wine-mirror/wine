/*
 * Copyright 2014 Henri Verbeet for CodeWeavers
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
#include "d2d1.h"
#include "wincrypt.h"
#include "wine/test.h"
#include "initguid.h"
#include "dwrite.h"

static void set_point(D2D1_POINT_2F *point, float x, float y)
{
    point->x = x;
    point->y = y;
}

static void set_rect(D2D1_RECT_F *rect, float left, float top, float right, float bottom)
{
    rect->left = left;
    rect->top = top;
    rect->right = right;
    rect->bottom = bottom;
}

static void set_color(D2D1_COLOR_F *color, float r, float g, float b, float a)
{
    color->r = r;
    color->g = g;
    color->b = b;
    color->a = a;
}

static void set_matrix_identity(D2D1_MATRIX_3X2_F *matrix)
{
    matrix->_11 = 1.0f;
    matrix->_12 = 0.0f;
    matrix->_21 = 0.0f;
    matrix->_22 = 1.0f;
    matrix->_31 = 0.0f;
    matrix->_32 = 0.0f;
}

static void rotate_matrix(D2D1_MATRIX_3X2_F *matrix, float theta)
{
    float sin_theta, cos_theta, tmp_11, tmp_12;

    sin_theta = sinf(theta);
    cos_theta = cosf(theta);
    tmp_11 = matrix->_11;
    tmp_12 = matrix->_12;

    matrix->_11 = cos_theta * tmp_11 + sin_theta * matrix->_21;
    matrix->_12 = cos_theta * tmp_12 + sin_theta * matrix->_22;
    matrix->_21 = -sin_theta * tmp_11 + cos_theta * matrix->_21;
    matrix->_22 = -sin_theta * tmp_12 + cos_theta * matrix->_22;
}

static void scale_matrix(D2D1_MATRIX_3X2_F *matrix, float x, float y)
{
    matrix->_11 *= x;
    matrix->_12 *= x;
    matrix->_21 *= y;
    matrix->_22 *= y;
}

static void translate_matrix(D2D1_MATRIX_3X2_F *matrix, float x, float y)
{
    matrix->_31 += x * matrix->_11 + y * matrix->_21;
    matrix->_32 += x * matrix->_12 + y * matrix->_22;
}

static BOOL compare_sha1(void *data, unsigned int pitch, unsigned int bpp,
        unsigned int w, unsigned int h, const char *ref_sha1)
{
    static const char hex_chars[] = "0123456789abcdef";
    HCRYPTPROV provider;
    BYTE hash_data[20];
    HCRYPTHASH hash;
    unsigned int i;
    char sha1[41];
    BOOL ret;

    ret = CryptAcquireContextW(&provider, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
    ok(ret, "Failed to acquire crypt context.\n");
    ret = CryptCreateHash(provider, CALG_SHA1, 0, 0, &hash);
    ok(ret, "Failed to create hash.\n");

    for (i = 0; i < h; ++i)
    {
        if (!(ret = CryptHashData(hash, (BYTE *)data + pitch * i, w * bpp, 0)))
            break;
    }
    ok(ret, "Failed to hash data.\n");

    i = sizeof(hash_data);
    ret = CryptGetHashParam(hash, HP_HASHVAL, hash_data, &i, 0);
    ok(ret, "Failed to get hash value.\n");
    ok(i == sizeof(hash_data), "Got unexpected hash size %u.\n", i);

    ret = CryptDestroyHash(hash);
    ok(ret, "Failed to destroy hash.\n");
    ret = CryptReleaseContext(provider, 0);
    ok(ret, "Failed to release crypt context.\n");

    for (i = 0; i < 20; ++i)
    {
        sha1[i * 2] = hex_chars[hash_data[i] >> 4];
        sha1[i * 2 + 1] = hex_chars[hash_data[i] & 0xf];
    }
    sha1[40] = 0;

    return !strcmp(ref_sha1, (char *)sha1);
}

static BOOL compare_surface(IDXGISurface *surface, const char *ref_sha1)
{
    D3D10_MAPPED_TEXTURE2D mapped_texture;
    D3D10_TEXTURE2D_DESC texture_desc;
    DXGI_SURFACE_DESC surface_desc;
    ID3D10Resource *src_resource;
    ID3D10Texture2D *texture;
    ID3D10Device *device;
    HRESULT hr;
    BOOL ret;

    hr = IDXGISurface_GetDevice(surface, &IID_ID3D10Device, (void **)&device);
    ok(SUCCEEDED(hr), "Failed to get device, hr %#x.\n", hr);
    hr = IDXGISurface_QueryInterface(surface, &IID_ID3D10Resource, (void **)&src_resource);
    ok(SUCCEEDED(hr), "Failed to query resource interface, hr %#x.\n", hr);

    hr = IDXGISurface_GetDesc(surface, &surface_desc);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    texture_desc.Width = surface_desc.Width;
    texture_desc.Height = surface_desc.Height;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = surface_desc.Format;
    texture_desc.SampleDesc = surface_desc.SampleDesc;
    texture_desc.Usage = D3D10_USAGE_STAGING;
    texture_desc.BindFlags = 0;
    texture_desc.CPUAccessFlags = D3D10_CPU_ACCESS_READ;
    texture_desc.MiscFlags = 0;
    hr = ID3D10Device_CreateTexture2D(device, &texture_desc, NULL, &texture);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);

    ID3D10Device_CopyResource(device, (ID3D10Resource *)texture, src_resource);
    hr = ID3D10Texture2D_Map(texture, 0, D3D10_MAP_READ, 0, &mapped_texture);
    ok(SUCCEEDED(hr), "Failed to map texture, hr %#x.\n", hr);
    ret = compare_sha1(mapped_texture.pData, mapped_texture.RowPitch, 4,
            texture_desc.Width, texture_desc.Height, ref_sha1);
    ID3D10Texture2D_Unmap(texture, 0);

    ID3D10Texture2D_Release(texture);
    ID3D10Resource_Release(src_resource);
    ID3D10Device_Release(device);

    return ret;
}

static ID3D10Device1 *create_device(void)
{
    ID3D10Device1 *device;

    if (SUCCEEDED(D3D10CreateDevice1(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL,
            D3D10_CREATE_DEVICE_BGRA_SUPPORT, D3D10_FEATURE_LEVEL_10_0, D3D10_1_SDK_VERSION, &device)))
        return device;
    if (SUCCEEDED(D3D10CreateDevice1(NULL, D3D10_DRIVER_TYPE_WARP, NULL,
            D3D10_CREATE_DEVICE_BGRA_SUPPORT, D3D10_FEATURE_LEVEL_10_0, D3D10_1_SDK_VERSION, &device)))
        return device;
    if (SUCCEEDED(D3D10CreateDevice1(NULL, D3D10_DRIVER_TYPE_REFERENCE, NULL,
            D3D10_CREATE_DEVICE_BGRA_SUPPORT, D3D10_FEATURE_LEVEL_10_0, D3D10_1_SDK_VERSION, &device)))
        return device;

    return NULL;
}

static IDXGISwapChain *create_swapchain(ID3D10Device1 *device, HWND window, BOOL windowed)
{
    IDXGISwapChain *swapchain;
    DXGI_SWAP_CHAIN_DESC desc;
    IDXGIDevice *dxgi_device;
    IDXGIAdapter *adapter;
    IDXGIFactory *factory;
    HRESULT hr;

    hr = ID3D10Device1_QueryInterface(device, &IID_IDXGIDevice, (void **)&dxgi_device);
    ok(SUCCEEDED(hr), "Failed to get DXGI device, hr %#x.\n", hr);
    hr = IDXGIDevice_GetAdapter(dxgi_device, &adapter);
    ok(SUCCEEDED(hr), "Failed to get adapter, hr %#x.\n", hr);
    IDXGIDevice_Release(dxgi_device);
    hr = IDXGIAdapter_GetParent(adapter, &IID_IDXGIFactory, (void **)&factory);
    ok(SUCCEEDED(hr), "Failed to get factory, hr %#x.\n", hr);
    IDXGIAdapter_Release(adapter);

    desc.BufferDesc.Width = 640;
    desc.BufferDesc.Height = 480;
    desc.BufferDesc.RefreshRate.Numerator = 60;
    desc.BufferDesc.RefreshRate.Denominator = 1;
    desc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = 1;
    desc.OutputWindow = window;
    desc.Windowed = windowed;
    desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    desc.Flags = 0;

    hr = IDXGIFactory_CreateSwapChain(factory, (IUnknown *)device, &desc, &swapchain);
    ok(SUCCEEDED(hr), "Failed to create swapchain, hr %#x.\n", hr);
    IDXGIFactory_Release(factory);

    return swapchain;
}

static ID2D1RenderTarget *create_render_target(IDXGISurface *surface)
{
    D2D1_RENDER_TARGET_PROPERTIES desc;
    ID2D1RenderTarget *render_target;
    ID2D1Factory *factory;
    HRESULT hr;

    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &IID_ID2D1Factory, NULL, (void **)&factory);
    ok(SUCCEEDED(hr), "Failed to create factory, hr %#x.\n", hr);

    desc.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
    desc.pixelFormat.format = DXGI_FORMAT_UNKNOWN;
    desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;
    desc.dpiX = 0.0f;
    desc.dpiY = 0.0f;
    desc.usage = D2D1_RENDER_TARGET_USAGE_NONE;
    desc.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;
    hr = ID2D1Factory_CreateDxgiSurfaceRenderTarget(factory, surface, &desc, &render_target);
    ok(SUCCEEDED(hr), "Failed to create render target, hr %#x.\n", hr);

    ID2D1Factory_Release(factory);

    return render_target;
}

static void test_clip(void)
{
    IDXGISwapChain *swapchain;
    D2D1_MATRIX_3X2_F matrix;
    D2D1_SIZE_U pixel_size;
    ID2D1RenderTarget *rt;
    ID3D10Device1 *device;
    IDXGISurface *surface;
    D2D1_POINT_2F point;
    D2D1_COLOR_F color;
    float dpi_x, dpi_y;
    D2D1_RECT_F rect;
    D2D1_SIZE_F size;
    HWND window;
    HRESULT hr;
    static const D2D1_MATRIX_3X2_F identity =
    {
        1.0f, 0.0f,
        0.0f, 1.0f,
        0.0f, 0.0f,
    };

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }
    window = CreateWindowA("static", "d2d1_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    swapchain = create_swapchain(device, window, TRUE);
    hr = IDXGISwapChain_GetBuffer(swapchain, 0, &IID_IDXGISurface, (void **)&surface);
    ok(SUCCEEDED(hr), "Failed to get buffer, hr %#x.\n", hr);
    rt = create_render_target(surface);
    ok(!!rt, "Failed to create render target.\n");

    ID2D1RenderTarget_GetDpi(rt, &dpi_x, &dpi_y);
    ok(dpi_x == 96.0f, "Got unexpected dpi_x %.8e.\n", dpi_x);
    ok(dpi_y == 96.0f, "Got unexpected dpi_x %.8e.\n", dpi_y);
    size = ID2D1RenderTarget_GetSize(rt);
    ok(size.width == 640.0f, "Got unexpected width %.8e.\n", size.width);
    ok(size.height == 480.0f, "Got unexpected height %.8e.\n", size.height);
    pixel_size = ID2D1RenderTarget_GetPixelSize(rt);
    ok(pixel_size.width == 640, "Got unexpected width %u.\n", pixel_size.width);
    ok(pixel_size.height == 480, "Got unexpected height %u.\n", pixel_size.height);

    ID2D1RenderTarget_GetTransform(rt, &matrix);
    ok(!memcmp(&matrix, &identity, sizeof(matrix)),
            "Got unexpected matrix {%.8e, %.8e, %.8e, %.8e, %.8e, %.8e}.\n",
            matrix._11, matrix._12, matrix._21, matrix._22, matrix._31, matrix._32);

    ID2D1RenderTarget_BeginDraw(rt);

    set_color(&color, 1.0f, 1.0f, 0.0f, 1.0f);
    ID2D1RenderTarget_Clear(rt, &color);

    ID2D1RenderTarget_SetDpi(rt, 48.0f, 192.0f);
    ID2D1RenderTarget_GetDpi(rt, &dpi_x, &dpi_y);
    ok(dpi_x == 48.0f, "Got unexpected dpi_x %.8e.\n", dpi_x);
    ok(dpi_y == 192.0f, "Got unexpected dpi_x %.8e.\n", dpi_y);
    size = ID2D1RenderTarget_GetSize(rt);
    ok(size.width == 1280.0f, "Got unexpected width %.8e.\n", size.width);
    ok(size.height == 240.0f, "Got unexpected height %.8e.\n", size.height);
    pixel_size = ID2D1RenderTarget_GetPixelSize(rt);
    ok(pixel_size.width == 640, "Got unexpected width %u.\n", pixel_size.width);
    ok(pixel_size.height == 480, "Got unexpected height %u.\n", pixel_size.height);

    /* The effective clip rect is the intersection of all currently pushed
     * clip rects. Clip rects are in DIPs. */
    set_rect(&rect, 0.0f, 0.0f, 1280.0f, 80.0f);
    ID2D1RenderTarget_PushAxisAlignedClip(rt, &rect, D2D1_ANTIALIAS_MODE_ALIASED);
    set_rect(&rect, 0.0f, 0.0f, 426.0f, 240.0f);
    ID2D1RenderTarget_PushAxisAlignedClip(rt, &rect, D2D1_ANTIALIAS_MODE_ALIASED);

    set_color(&color, 0.0f, 1.0f, 0.0f, 1.0f);
    ID2D1RenderTarget_Clear(rt, &color);
    ID2D1RenderTarget_PopAxisAlignedClip(rt);
    ID2D1RenderTarget_PopAxisAlignedClip(rt);

    ID2D1RenderTarget_SetDpi(rt, 0.0f, 0.0f);
    ID2D1RenderTarget_GetDpi(rt, &dpi_x, &dpi_y);
    ok(dpi_x == 96.0f, "Got unexpected dpi_x %.8e.\n", dpi_x);
    ok(dpi_y == 96.0f, "Got unexpected dpi_y %.8e.\n", dpi_y);

    /* Transformations apply to clip rects, the effective clip rect is the
     * (axis-aligned) bounding box of the transformed clip rect. */
    set_point(&point, 320.0f, 240.0f);
    D2D1MakeRotateMatrix(30.0f, point, &matrix);
    ID2D1RenderTarget_SetTransform(rt, &matrix);
    set_rect(&rect, 215.0f, 208.0f, 425.0f, 272.0f);
    ID2D1RenderTarget_PushAxisAlignedClip(rt, &rect, D2D1_ANTIALIAS_MODE_ALIASED);
    set_color(&color, 1.0f, 1.0f, 1.0f, 1.0f);
    ID2D1RenderTarget_Clear(rt, &color);
    ID2D1RenderTarget_PopAxisAlignedClip(rt);

    /* Transformations are applied when pushing the clip rect, transformations
     * set afterwards have no effect on the current clip rect. This includes
     * SetDpi(). */
    ID2D1RenderTarget_SetTransform(rt, &identity);
    set_rect(&rect, 427.0f, 320.0f, 640.0f, 480.0f);
    ID2D1RenderTarget_PushAxisAlignedClip(rt, &rect, D2D1_ANTIALIAS_MODE_ALIASED);
    ID2D1RenderTarget_SetTransform(rt, &matrix);
    ID2D1RenderTarget_SetDpi(rt, 48.0f, 192.0f);
    set_color(&color, 1.0f, 0.0f, 0.0f, 1.0f);
    ID2D1RenderTarget_Clear(rt, &color);
    ID2D1RenderTarget_PopAxisAlignedClip(rt);

    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to end draw, hr %#x.\n", hr);
    ok(compare_surface(surface, "035a44d4198d6e422e9de6185b5b2c2bac5e33c9"), "Surface does not match.\n");

    ID2D1RenderTarget_Release(rt);
    IDXGISurface_Release(surface);
    IDXGISwapChain_Release(swapchain);
    ID3D10Device1_Release(device);
    DestroyWindow(window);
}

static void test_state_block(void)
{
    IDWriteRenderingParams *text_rendering_params1, *text_rendering_params2;
    D2D1_DRAWING_STATE_DESCRIPTION drawing_state;
    ID2D1DrawingStateBlock *state_block;
    IDWriteFactory *dwrite_factory;
    IDXGISwapChain *swapchain;
    ID2D1RenderTarget *rt;
    ID3D10Device1 *device;
    IDXGISurface *surface;
    ID2D1Factory *factory;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    static const D2D1_MATRIX_3X2_F identity =
    {
        1.0f, 0.0f,
        0.0f, 1.0f,
        0.0f, 0.0f,
    };
    static const D2D1_MATRIX_3X2_F transform1 =
    {
        1.0f, 2.0f,
        3.0f, 4.0f,
        5.0f, 6.0f,
    };
    static const D2D1_MATRIX_3X2_F transform2 =
    {
        7.0f,  8.0f,
        9.0f,  10.0f,
        11.0f, 12.0f,
    };

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }
    window = CreateWindowA("static", "d2d1_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    swapchain = create_swapchain(device, window, TRUE);
    hr = IDXGISwapChain_GetBuffer(swapchain, 0, &IID_IDXGISurface, (void **)&surface);
    ok(SUCCEEDED(hr), "Failed to get buffer, hr %#x.\n", hr);
    rt = create_render_target(surface);
    ok(!!rt, "Failed to create render target.\n");
    ID2D1RenderTarget_GetFactory(rt, &factory);
    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, &IID_IDWriteFactory, (IUnknown **)&dwrite_factory);
    ok(SUCCEEDED(hr), "Failed to create dwrite factory, hr %#x.\n", hr);
    hr = IDWriteFactory_CreateRenderingParams(dwrite_factory, &text_rendering_params1);
    ok(SUCCEEDED(hr), "Failed to create dwrite rendering params, hr %#x.\n", hr);
    IDWriteFactory_Release(dwrite_factory);

    drawing_state.antialiasMode = ID2D1RenderTarget_GetAntialiasMode(rt);
    ok(drawing_state.antialiasMode == D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
            "Got unexpected antialias mode %#x.\n", drawing_state.antialiasMode);
    drawing_state.textAntialiasMode = ID2D1RenderTarget_GetTextAntialiasMode(rt);
    ok(drawing_state.textAntialiasMode == D2D1_TEXT_ANTIALIAS_MODE_DEFAULT,
            "Got unexpected text antialias mode %#x.\n", drawing_state.textAntialiasMode);
    ID2D1RenderTarget_GetTags(rt, &drawing_state.tag1, &drawing_state.tag2);
    ok(!drawing_state.tag1 && !drawing_state.tag2, "Got unexpected tags %08x%08x:%08x%08x.\n",
            (unsigned int)(drawing_state.tag1 >> 32), (unsigned int)(drawing_state.tag1),
            (unsigned int)(drawing_state.tag2 >> 32), (unsigned int)(drawing_state.tag2));
    ID2D1RenderTarget_GetTransform(rt, &drawing_state.transform);
    ok(!memcmp(&drawing_state.transform, &identity, sizeof(drawing_state.transform)),
            "Got unexpected matrix {%.8e, %.8e, %.8e, %.8e, %.8e, %.8e}.\n",
            drawing_state.transform._11, drawing_state.transform._12, drawing_state.transform._21,
            drawing_state.transform._22, drawing_state.transform._31, drawing_state.transform._32);
    ID2D1RenderTarget_GetTextRenderingParams(rt, &text_rendering_params2);
    ok(!text_rendering_params2, "Got unexpected text rendering params %p.\n", text_rendering_params2);

    hr = ID2D1Factory_CreateDrawingStateBlock(factory, NULL, NULL, &state_block);
    ok(SUCCEEDED(hr), "Failed to create drawing state block, hr %#x\n", hr);
    ID2D1DrawingStateBlock_GetDescription(state_block, &drawing_state);
    ok(drawing_state.antialiasMode == D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
            "Got unexpected antialias mode %#x.\n", drawing_state.antialiasMode);
    ok(drawing_state.textAntialiasMode == D2D1_TEXT_ANTIALIAS_MODE_DEFAULT,
            "Got unexpected text antialias mode %#x.\n", drawing_state.textAntialiasMode);
    ok(!drawing_state.tag1 && !drawing_state.tag2, "Got unexpected tags %08x%08x:%08x%08x.\n",
            (unsigned int)(drawing_state.tag1 >> 32), (unsigned int)(drawing_state.tag1),
            (unsigned int)(drawing_state.tag2 >> 32), (unsigned int)(drawing_state.tag2));
    ok(!memcmp(&drawing_state.transform, &identity, sizeof(drawing_state.transform)),
            "Got unexpected matrix {%.8e, %.8e, %.8e, %.8e, %.8e, %.8e}.\n",
            drawing_state.transform._11, drawing_state.transform._12, drawing_state.transform._21,
            drawing_state.transform._22, drawing_state.transform._31, drawing_state.transform._32);
    ID2D1DrawingStateBlock_GetTextRenderingParams(state_block, &text_rendering_params2);
    ok(!text_rendering_params2, "Got unexpected text rendering params %p.\n", text_rendering_params2);
    ID2D1DrawingStateBlock_Release(state_block);

    drawing_state.antialiasMode = D2D1_ANTIALIAS_MODE_ALIASED;
    drawing_state.textAntialiasMode = D2D1_TEXT_ANTIALIAS_MODE_ALIASED;
    drawing_state.tag1 = 0xdead;
    drawing_state.tag2 = 0xbeef;
    drawing_state.transform = transform1;
    hr = ID2D1Factory_CreateDrawingStateBlock(factory, &drawing_state, text_rendering_params1, &state_block);
    ok(SUCCEEDED(hr), "Failed to create drawing state block, hr %#x\n", hr);

    ID2D1DrawingStateBlock_GetDescription(state_block, &drawing_state);
    ok(drawing_state.antialiasMode == D2D1_ANTIALIAS_MODE_ALIASED,
            "Got unexpected antialias mode %#x.\n", drawing_state.antialiasMode);
    ok(drawing_state.textAntialiasMode == D2D1_TEXT_ANTIALIAS_MODE_ALIASED,
            "Got unexpected text antialias mode %#x.\n", drawing_state.textAntialiasMode);
    ok(drawing_state.tag1 == 0xdead && drawing_state.tag2 == 0xbeef, "Got unexpected tags %08x%08x:%08x%08x.\n",
            (unsigned int)(drawing_state.tag1 >> 32), (unsigned int)(drawing_state.tag1),
            (unsigned int)(drawing_state.tag2 >> 32), (unsigned int)(drawing_state.tag2));
    ok(!memcmp(&drawing_state.transform, &transform1, sizeof(drawing_state.transform)),
            "Got unexpected matrix {%.8e, %.8e, %.8e, %.8e, %.8e, %.8e}.\n",
            drawing_state.transform._11, drawing_state.transform._12, drawing_state.transform._21,
            drawing_state.transform._22, drawing_state.transform._31, drawing_state.transform._32);
    ID2D1DrawingStateBlock_GetTextRenderingParams(state_block, &text_rendering_params2);
    ok(text_rendering_params2 == text_rendering_params1, "Got unexpected text rendering params %p, expected %p.\n",
            text_rendering_params2, text_rendering_params1);
    IDWriteRenderingParams_Release(text_rendering_params2);

    ID2D1RenderTarget_RestoreDrawingState(rt, state_block);

    drawing_state.antialiasMode = ID2D1RenderTarget_GetAntialiasMode(rt);
    ok(drawing_state.antialiasMode == D2D1_ANTIALIAS_MODE_ALIASED,
            "Got unexpected antialias mode %#x.\n", drawing_state.antialiasMode);
    drawing_state.textAntialiasMode = ID2D1RenderTarget_GetTextAntialiasMode(rt);
    ok(drawing_state.textAntialiasMode == D2D1_TEXT_ANTIALIAS_MODE_ALIASED,
            "Got unexpected text antialias mode %#x.\n", drawing_state.textAntialiasMode);
    ID2D1RenderTarget_GetTags(rt, &drawing_state.tag1, &drawing_state.tag2);
    ok(drawing_state.tag1 == 0xdead && drawing_state.tag2 == 0xbeef, "Got unexpected tags %08x%08x:%08x%08x.\n",
            (unsigned int)(drawing_state.tag1 >> 32), (unsigned int)(drawing_state.tag1),
            (unsigned int)(drawing_state.tag2 >> 32), (unsigned int)(drawing_state.tag2));
    ID2D1RenderTarget_GetTransform(rt, &drawing_state.transform);
    ok(!memcmp(&drawing_state.transform, &transform1, sizeof(drawing_state.transform)),
            "Got unexpected matrix {%.8e, %.8e, %.8e, %.8e, %.8e, %.8e}.\n",
            drawing_state.transform._11, drawing_state.transform._12, drawing_state.transform._21,
            drawing_state.transform._22, drawing_state.transform._31, drawing_state.transform._32);
    ID2D1RenderTarget_GetTextRenderingParams(rt, &text_rendering_params2);
    ok(text_rendering_params2 == text_rendering_params1, "Got unexpected text rendering params %p, expected %p.\n",
            text_rendering_params2, text_rendering_params1);
    IDWriteRenderingParams_Release(text_rendering_params2);

    ID2D1RenderTarget_SetAntialiasMode(rt, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    ID2D1RenderTarget_SetTextAntialiasMode(rt, D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
    ID2D1RenderTarget_SetTags(rt, 1, 2);
    ID2D1RenderTarget_SetTransform(rt, &transform2);
    ID2D1RenderTarget_SetTextRenderingParams(rt, NULL);

    drawing_state.antialiasMode = ID2D1RenderTarget_GetAntialiasMode(rt);
    ok(drawing_state.antialiasMode == D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
            "Got unexpected antialias mode %#x.\n", drawing_state.antialiasMode);
    drawing_state.textAntialiasMode = ID2D1RenderTarget_GetTextAntialiasMode(rt);
    ok(drawing_state.textAntialiasMode == D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE,
            "Got unexpected text antialias mode %#x.\n", drawing_state.textAntialiasMode);
    ID2D1RenderTarget_GetTags(rt, &drawing_state.tag1, &drawing_state.tag2);
    ok(drawing_state.tag1 == 1 && drawing_state.tag2 == 2, "Got unexpected tags %08x%08x:%08x%08x.\n",
            (unsigned int)(drawing_state.tag1 >> 32), (unsigned int)(drawing_state.tag1),
            (unsigned int)(drawing_state.tag2 >> 32), (unsigned int)(drawing_state.tag2));
    ID2D1RenderTarget_GetTransform(rt, &drawing_state.transform);
    ok(!memcmp(&drawing_state.transform, &transform2, sizeof(drawing_state.transform)),
            "Got unexpected matrix {%.8e, %.8e, %.8e, %.8e, %.8e, %.8e}.\n",
            drawing_state.transform._11, drawing_state.transform._12, drawing_state.transform._21,
            drawing_state.transform._22, drawing_state.transform._31, drawing_state.transform._32);
    ID2D1RenderTarget_GetTextRenderingParams(rt, &text_rendering_params2);
    ok(!text_rendering_params2, "Got unexpected text rendering params %p.\n", text_rendering_params2);

    ID2D1RenderTarget_SaveDrawingState(rt, state_block);

    ID2D1DrawingStateBlock_GetDescription(state_block, &drawing_state);
    ok(drawing_state.antialiasMode == D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
            "Got unexpected antialias mode %#x.\n", drawing_state.antialiasMode);
    ok(drawing_state.textAntialiasMode == D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE,
            "Got unexpected text antialias mode %#x.\n", drawing_state.textAntialiasMode);
    ok(drawing_state.tag1 == 1 && drawing_state.tag2 == 2, "Got unexpected tags %08x%08x:%08x%08x.\n",
            (unsigned int)(drawing_state.tag1 >> 32), (unsigned int)(drawing_state.tag1),
            (unsigned int)(drawing_state.tag2 >> 32), (unsigned int)(drawing_state.tag2));
    ok(!memcmp(&drawing_state.transform, &transform2, sizeof(drawing_state.transform)),
            "Got unexpected matrix {%.8e, %.8e, %.8e, %.8e, %.8e, %.8e}.\n",
            drawing_state.transform._11, drawing_state.transform._12, drawing_state.transform._21,
            drawing_state.transform._22, drawing_state.transform._31, drawing_state.transform._32);
    ID2D1DrawingStateBlock_GetTextRenderingParams(state_block, &text_rendering_params2);
    ok(!text_rendering_params2, "Got unexpected text rendering params %p.\n", text_rendering_params2);

    drawing_state.antialiasMode = D2D1_ANTIALIAS_MODE_ALIASED;
    drawing_state.textAntialiasMode = D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE;
    drawing_state.tag1 = 3;
    drawing_state.tag2 = 4;
    drawing_state.transform = transform1;
    ID2D1DrawingStateBlock_SetDescription(state_block, &drawing_state);
    ID2D1DrawingStateBlock_SetTextRenderingParams(state_block, text_rendering_params1);

    ID2D1DrawingStateBlock_GetDescription(state_block, &drawing_state);
    ok(drawing_state.antialiasMode == D2D1_ANTIALIAS_MODE_ALIASED,
            "Got unexpected antialias mode %#x.\n", drawing_state.antialiasMode);
    ok(drawing_state.textAntialiasMode == D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE,
            "Got unexpected text antialias mode %#x.\n", drawing_state.textAntialiasMode);
    ok(drawing_state.tag1 == 3 && drawing_state.tag2 == 4, "Got unexpected tags %08x%08x:%08x%08x.\n",
            (unsigned int)(drawing_state.tag1 >> 32), (unsigned int)(drawing_state.tag1),
            (unsigned int)(drawing_state.tag2 >> 32), (unsigned int)(drawing_state.tag2));
    ok(!memcmp(&drawing_state.transform, &transform1, sizeof(drawing_state.transform)),
            "Got unexpected matrix {%.8e, %.8e, %.8e, %.8e, %.8e, %.8e}.\n",
            drawing_state.transform._11, drawing_state.transform._12, drawing_state.transform._21,
            drawing_state.transform._22, drawing_state.transform._31, drawing_state.transform._32);
    ID2D1DrawingStateBlock_GetTextRenderingParams(state_block, &text_rendering_params2);
    ok(text_rendering_params2 == text_rendering_params1, "Got unexpected text rendering params %p, expected %p.\n",
            text_rendering_params2, text_rendering_params1);
    IDWriteRenderingParams_Release(text_rendering_params2);

    ID2D1DrawingStateBlock_Release(state_block);

    refcount = IDWriteRenderingParams_Release(text_rendering_params1);
    ok(!refcount, "Rendering params %u references left.\n", refcount);
    ID2D1Factory_Release(factory);
    ID2D1RenderTarget_Release(rt);
    IDXGISurface_Release(surface);
    IDXGISwapChain_Release(swapchain);
    ID3D10Device1_Release(device);
    DestroyWindow(window);
}

static void test_color_brush(void)
{
    D2D1_MATRIX_3X2_F matrix, tmp_matrix;
    D2D1_BRUSH_PROPERTIES brush_desc;
    D2D1_COLOR_F color, tmp_color;
    ID2D1SolidColorBrush *brush;
    IDXGISwapChain *swapchain;
    ID2D1RenderTarget *rt;
    ID3D10Device1 *device;
    IDXGISurface *surface;
    D2D1_RECT_F rect;
    float opacity;
    HWND window;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }
    window = CreateWindowA("static", "d2d1_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    swapchain = create_swapchain(device, window, TRUE);
    hr = IDXGISwapChain_GetBuffer(swapchain, 0, &IID_IDXGISurface, (void **)&surface);
    ok(SUCCEEDED(hr), "Failed to get buffer, hr %#x.\n", hr);
    rt = create_render_target(surface);
    ok(!!rt, "Failed to create render target.\n");

    ID2D1RenderTarget_SetDpi(rt, 192.0f, 48.0f);
    ID2D1RenderTarget_SetAntialiasMode(rt, D2D1_ANTIALIAS_MODE_ALIASED);

    set_color(&color, 0.0f, 0.0f, 0.0f, 0.0f);
    hr = ID2D1RenderTarget_CreateSolidColorBrush(rt, &color, NULL, &brush);
    ok(SUCCEEDED(hr), "Failed to create brush, hr %#x.\n", hr);
    opacity = ID2D1SolidColorBrush_GetOpacity(brush);
    ok(opacity == 1.0f, "Got unexpected opacity %.8e.\n", opacity);
    set_matrix_identity(&matrix);
    ID2D1SolidColorBrush_GetTransform(brush, &tmp_matrix);
    ok(!memcmp(&tmp_matrix, &matrix, sizeof(matrix)),
            "Got unexpected matrix {%.8e, %.8e, %.8e, %.8e, %.8e, %.8e}.\n",
            tmp_matrix._11, tmp_matrix._12, tmp_matrix._21,
            tmp_matrix._22, tmp_matrix._31, tmp_matrix._32);
    tmp_color = ID2D1SolidColorBrush_GetColor(brush);
    ok(!memcmp(&tmp_color, &color, sizeof(color)),
            "Got unexpected color {%.8e, %.8e, %.8e, %.8e}.\n",
            tmp_color.r, tmp_color.g, tmp_color.b, tmp_color.a);
    ID2D1SolidColorBrush_Release(brush);

    set_color(&color, 0.0f, 1.0f, 0.0f, 0.8f);
    brush_desc.opacity = 0.3f;
    set_matrix_identity(&matrix);
    scale_matrix(&matrix, 2.0f, 2.0f);
    brush_desc.transform = matrix;
    hr = ID2D1RenderTarget_CreateSolidColorBrush(rt, &color, &brush_desc, &brush);
    ok(SUCCEEDED(hr), "Failed to create brush, hr %#x.\n", hr);
    opacity = ID2D1SolidColorBrush_GetOpacity(brush);
    ok(opacity == 0.3f, "Got unexpected opacity %.8e.\n", opacity);
    ID2D1SolidColorBrush_GetTransform(brush, &tmp_matrix);
    ok(!memcmp(&tmp_matrix, &matrix, sizeof(matrix)),
            "Got unexpected matrix {%.8e, %.8e, %.8e, %.8e, %.8e, %.8e}.\n",
            tmp_matrix._11, tmp_matrix._12, tmp_matrix._21,
            tmp_matrix._22, tmp_matrix._31, tmp_matrix._32);
    tmp_color = ID2D1SolidColorBrush_GetColor(brush);
    ok(!memcmp(&tmp_color, &color, sizeof(color)),
            "Got unexpected color {%.8e, %.8e, %.8e, %.8e}.\n",
            tmp_color.r, tmp_color.g, tmp_color.b, tmp_color.a);

    ID2D1RenderTarget_BeginDraw(rt);

    set_color(&color, 0.0f, 0.0f, 1.0f, 1.0f);
    ID2D1RenderTarget_Clear(rt, &color);

    ID2D1SolidColorBrush_SetOpacity(brush, 1.0f);
    set_rect(&rect, 40.0f, 120.0f, 120.0f, 360.0f);
    ID2D1RenderTarget_FillRectangle(rt, &rect, (ID2D1Brush *)brush);

    set_matrix_identity(&matrix);
    scale_matrix(&matrix, 0.5f, 2.0f);
    translate_matrix(&matrix, 320.0f, 240.0f);
    rotate_matrix(&matrix, M_PI / 4.0f);
    ID2D1RenderTarget_SetTransform(rt, &matrix);
    set_color(&color, 1.0f, 0.0f, 0.0f, 0.625f);
    ID2D1SolidColorBrush_SetColor(brush, &color);
    ID2D1SolidColorBrush_SetOpacity(brush, 0.75f);
    set_rect(&rect, -80.0f, -60.0f, 80.0f, 60.0f);
    ID2D1RenderTarget_FillRectangle(rt, &rect, (ID2D1Brush *)brush);

    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to end draw, hr %#x.\n", hr);
    ok(compare_surface(surface, "6d1218fca5e21fb7e287b3a439d60dbc251f5ceb"), "Surface does not match.\n");

    ID2D1SolidColorBrush_Release(brush);
    ID2D1RenderTarget_Release(rt);
    IDXGISurface_Release(surface);
    IDXGISwapChain_Release(swapchain);
    ID3D10Device1_Release(device);
    DestroyWindow(window);
}

START_TEST(d2d1)
{
    test_clip();
    test_state_block();
    test_color_brush();
}
