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
#include <limits.h>
#include <math.h>
#include <float.h>
#include "d2d1_1.h"
#include "d3d11.h"
#include "wincrypt.h"
#include "wine/test.h"
#include "initguid.h"
#include "dwrite.h"
#include "wincodec.h"
#include "wine/heap.h"

static HRESULT (WINAPI *pD2D1CreateDevice)(IDXGIDevice *dxgi_device,
        const D2D1_CREATION_PROPERTIES *properties, ID2D1Device **device);
static void (WINAPI *pD2D1SinCos)(float angle, float *s, float *c);
static float (WINAPI *pD2D1Tan)(float angle);
static float (WINAPI *pD2D1Vec3Length)(float x, float y, float z);
static D2D1_COLOR_F (WINAPI *pD2D1ConvertColorSpace)(D2D1_COLOR_SPACE src_colour_space,
        D2D1_COLOR_SPACE dst_colour_space, const D2D1_COLOR_F *colour);

static BOOL use_mt = TRUE;

static struct test_entry
{
    void (*test)(BOOL d3d11);
    BOOL d3d11;
} *mt_tests;
size_t mt_tests_size, mt_test_count;

struct d2d1_test_context
{
    BOOL d3d11;
    IDXGIDevice *device;
    HWND window;
    IDXGISwapChain *swapchain;
    IDXGISurface *surface;
    ID2D1RenderTarget *rt;
};

struct resource_readback
{
    BOOL d3d11;
    union
    {
        ID3D10Resource *d3d10_resource;
        ID3D11Resource *d3d11_resource;
    } u;
    unsigned int pitch, width, height;
    void *data;
};

struct figure
{
    unsigned int *spans;
    unsigned int spans_size;
    unsigned int span_count;
};

struct geometry_sink
{
    ID2D1SimplifiedGeometrySink ID2D1SimplifiedGeometrySink_iface;

    struct geometry_figure
    {
        D2D1_FIGURE_BEGIN begin;
        D2D1_FIGURE_END end;
        D2D1_POINT_2F start_point;
        struct geometry_segment
        {
            enum
            {
                SEGMENT_BEZIER,
                SEGMENT_LINE,
            } type;
            union
            {
                D2D1_BEZIER_SEGMENT bezier;
                D2D1_POINT_2F line;
            } u;
            DWORD flags;
        } *segments;
        unsigned int segments_size;
        unsigned int segment_count;
    } *figures;
    unsigned int figures_size;
    unsigned int figure_count;

    D2D1_FILL_MODE fill_mode;
    DWORD segment_flags;
    BOOL closed;
};

struct expected_geometry_figure
{
    D2D1_FIGURE_BEGIN begin;
    D2D1_FIGURE_END end;
    D2D1_POINT_2F start_point;
    unsigned int segment_count;
    const struct geometry_segment *segments;
};

static void queue_d3d1x_test(void (*test)(BOOL d3d11), BOOL d3d11)
{
    if (mt_test_count >= mt_tests_size)
    {
        mt_tests_size = max(16, mt_tests_size * 2);
        mt_tests = heap_realloc(mt_tests, mt_tests_size * sizeof(*mt_tests));
    }
    mt_tests[mt_test_count].test = test;
    mt_tests[mt_test_count++].d3d11 = d3d11;
}

static void queue_d3d10_test(void (*test)(BOOL d3d11))
{
    queue_d3d1x_test(test, FALSE);
}

static void queue_test(void (*test)(BOOL d3d11))
{
    queue_d3d1x_test(test, FALSE);
    queue_d3d1x_test(test, TRUE);
}

static DWORD WINAPI thread_func(void *ctx)
{
    LONG *i = ctx, j;

    while (*i < mt_test_count)
    {
        j = *i;
        if (InterlockedCompareExchange(i, j + 1, j) == j)
            mt_tests[j].test(mt_tests[j].d3d11);
    }

    return 0;
}

static void run_queued_tests(void)
{
    unsigned int thread_count, i;
    HANDLE *threads;
    SYSTEM_INFO si;
    LONG test_idx;

    if (!use_mt)
    {
        for (i = 0; i < mt_test_count; ++i)
        {
            mt_tests[i].test(mt_tests[i].d3d11);
        }

        return;
    }

    GetSystemInfo(&si);
    thread_count = si.dwNumberOfProcessors;
    threads = heap_calloc(thread_count, sizeof(*threads));
    for (i = 0, test_idx = 0; i < thread_count; ++i)
    {
        threads[i] = CreateThread(NULL, 0, thread_func, &test_idx, 0, NULL);
        ok(!!threads[i], "Failed to create thread %u.\n", i);
    }
    WaitForMultipleObjects(thread_count, threads, TRUE, INFINITE);
    for (i = 0; i < thread_count; ++i)
    {
        CloseHandle(threads[i]);
    }
    heap_free(threads);
}

static void set_point(D2D1_POINT_2F *point, float x, float y)
{
    point->x = x;
    point->y = y;
}

static void set_quadratic(D2D1_QUADRATIC_BEZIER_SEGMENT *quadratic, float x1, float y1, float x2, float y2)
{
    quadratic->point1.x = x1;
    quadratic->point1.y = y1;
    quadratic->point2.x = x2;
    quadratic->point2.y = y2;
}

static void set_rect(D2D1_RECT_F *rect, float left, float top, float right, float bottom)
{
    rect->left = left;
    rect->top = top;
    rect->right = right;
    rect->bottom = bottom;
}

static void set_rounded_rect(D2D1_ROUNDED_RECT *rect, float left, float top, float right, float bottom,
        float radius_x, float radius_y)
{
    set_rect(&rect->rect, left, top, right, bottom);
    rect->radiusX = radius_x;
    rect->radiusY = radius_y;
}

static void set_rect_u(D2D1_RECT_U *rect, UINT32 left, UINT32 top, UINT32 right, UINT32 bottom)
{
    rect->left = left;
    rect->top = top;
    rect->right = right;
    rect->bottom = bottom;
}

static void set_ellipse(D2D1_ELLIPSE *ellipse, float x, float y, float rx, float ry)
{
    set_point(&ellipse->point, x, y);
    ellipse->radiusX = rx;
    ellipse->radiusY = ry;
}

static void set_color(D2D1_COLOR_F *color, float r, float g, float b, float a)
{
    color->r = r;
    color->g = g;
    color->b = b;
    color->a = a;
}

static void set_size_u(D2D1_SIZE_U *size, unsigned int w, unsigned int h)
{
    size->width = w;
    size->height = h;
}

static void set_size_f(D2D1_SIZE_F *size, float w, float h)
{
    size->width = w;
    size->height = h;
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

static void skew_matrix(D2D1_MATRIX_3X2_F *matrix, float x, float y)
{
    float tmp_11, tmp_12;

    tmp_11 = matrix->_11;
    tmp_12 = matrix->_12;

    matrix->_11 += y * matrix->_21;
    matrix->_12 += y * matrix->_22;
    matrix->_21 += x * tmp_11;
    matrix->_22 += x * tmp_12;
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

static void line_to(ID2D1GeometrySink *sink, float x, float y)
{
    D2D1_POINT_2F point;

    set_point(&point, x,  y);
    ID2D1GeometrySink_AddLine(sink, point);
}

static void quadratic_to(ID2D1GeometrySink *sink, float x1, float y1, float x2, float y2)
{
    D2D1_QUADRATIC_BEZIER_SEGMENT quadratic;

    set_quadratic(&quadratic, x1, y1, x2, y2);
    ID2D1GeometrySink_AddQuadraticBezier(sink, &quadratic);
}

static void cubic_to(ID2D1GeometrySink *sink, float x1, float y1, float x2, float y2, float x3, float y3)
{
    D2D1_BEZIER_SEGMENT b;

    b.point1.x = x1;
    b.point1.y = y1;
    b.point2.x = x2;
    b.point2.y = y2;
    b.point3.x = x3;
    b.point3.y = y3;
    ID2D1GeometrySink_AddBezier(sink, &b);
}

static void get_d3d10_surface_readback(IDXGISurface *surface, struct resource_readback *rb)
{
    D3D10_TEXTURE2D_DESC texture_desc;
    D3D10_MAPPED_TEXTURE2D map_desc;
    DXGI_SURFACE_DESC surface_desc;
    ID3D10Resource *src_resource;
    ID3D10Device *device;
    HRESULT hr;

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
    hr = ID3D10Device_CreateTexture2D(device, &texture_desc, NULL, (ID3D10Texture2D **)&rb->u.d3d10_resource);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);

    rb->width = texture_desc.Width;
    rb->height = texture_desc.Height;

    ID3D10Device_CopyResource(device, rb->u.d3d10_resource, src_resource);
    ID3D10Resource_Release(src_resource);
    ID3D10Device_Release(device);

    hr = ID3D10Texture2D_Map((ID3D10Texture2D *)rb->u.d3d10_resource, 0, D3D10_MAP_READ, 0, &map_desc);
    ok(SUCCEEDED(hr), "Failed to map texture, hr %#x.\n", hr);

    rb->pitch = map_desc.RowPitch;
    rb->data = map_desc.pData;
}

static void get_d3d11_surface_readback(IDXGISurface *surface, struct resource_readback *rb)
{
    D3D11_TEXTURE2D_DESC texture_desc;
    D3D11_MAPPED_SUBRESOURCE map_desc;
    DXGI_SURFACE_DESC surface_desc;
    ID3D11Resource *src_resource;
    ID3D11DeviceContext *context;
    ID3D11Device *device;
    HRESULT hr;

    hr = IDXGISurface_GetDevice(surface, &IID_ID3D11Device, (void **)&device);
    ok(SUCCEEDED(hr), "Failed to get device, hr %#x.\n", hr);
    hr = IDXGISurface_QueryInterface(surface, &IID_ID3D11Resource, (void **)&src_resource);
    ok(SUCCEEDED(hr), "Failed to query resource interface, hr %#x.\n", hr);

    hr = IDXGISurface_GetDesc(surface, &surface_desc);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    texture_desc.Width = surface_desc.Width;
    texture_desc.Height = surface_desc.Height;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = surface_desc.Format;
    texture_desc.SampleDesc = surface_desc.SampleDesc;
    texture_desc.Usage = D3D11_USAGE_STAGING;
    texture_desc.BindFlags = 0;
    texture_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    texture_desc.MiscFlags = 0;
    hr = ID3D11Device_CreateTexture2D(device, &texture_desc, NULL, (ID3D11Texture2D **)&rb->u.d3d11_resource);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);

    rb->width = texture_desc.Width;
    rb->height = texture_desc.Height;

    ID3D11Device_GetImmediateContext(device, &context);
    ID3D11DeviceContext_CopyResource(context, rb->u.d3d11_resource, src_resource);
    ID3D11Resource_Release(src_resource);
    ID3D11Device_Release(device);

    hr = ID3D11DeviceContext_Map(context, rb->u.d3d11_resource, 0, D3D11_MAP_READ, 0, &map_desc);
    ok(SUCCEEDED(hr), "Failed to map texture, hr %#x.\n", hr);
    ID3D11DeviceContext_Release(context);

    rb->pitch = map_desc.RowPitch;
    rb->data = map_desc.pData;
}

static void get_surface_readback(struct d2d1_test_context *ctx, struct resource_readback *rb)
{
    if ((rb->d3d11 = ctx->d3d11))
        get_d3d11_surface_readback(ctx->surface, rb);
    else
        get_d3d10_surface_readback(ctx->surface, rb);
}

static void release_d3d10_resource_readback(struct resource_readback *rb)
{
    ID3D10Texture2D_Unmap((ID3D10Texture2D *)rb->u.d3d10_resource, 0);
    ID3D10Resource_Release(rb->u.d3d10_resource);
}

static void release_d3d11_resource_readback(struct resource_readback *rb)
{
    ID3D11DeviceContext *context;
    ID3D11Device *device;

    ID3D11Resource_GetDevice(rb->u.d3d11_resource, &device);
    ID3D11Device_GetImmediateContext(device, &context);
    ID3D11Device_Release(device);

    ID3D11DeviceContext_Unmap(context, rb->u.d3d11_resource, 0);
    ID3D11Resource_Release(rb->u.d3d11_resource);
    ID3D11DeviceContext_Release(context);
}

static void release_resource_readback(struct resource_readback *rb)
{
    if (rb->d3d11)
        release_d3d11_resource_readback(rb);
    else
        release_d3d10_resource_readback(rb);
}

static DWORD get_readback_colour(struct resource_readback *rb, unsigned int x, unsigned int y)
{
    return ((DWORD *)((BYTE *)rb->data + y * rb->pitch))[x];
}

static float clamp_float(float f, float lower, float upper)
{
    return f < lower ? lower : f > upper ? upper : f;
}

static BOOL compare_uint(unsigned int x, unsigned int y, unsigned int max_diff)
{
    unsigned int diff = x > y ? x - y : y - x;

    return diff <= max_diff;
}

static BOOL compare_colour(DWORD c1, DWORD c2, BYTE max_diff)
{
    return compare_uint(c1 & 0xff, c2 & 0xff, max_diff)
            && compare_uint((c1 >> 8) & 0xff, (c2 >> 8) & 0xff, max_diff)
            && compare_uint((c1 >> 16) & 0xff, (c2 >> 16) & 0xff, max_diff)
            && compare_uint((c1 >> 24) & 0xff, (c2 >> 24) & 0xff, max_diff);
}

static BOOL compare_float(float f, float g, unsigned int ulps)
{
    int x = *(int *)&f;
    int y = *(int *)&g;

    if (x < 0)
        x = INT_MIN - x;
    if (y < 0)
        y = INT_MIN - y;

    if (abs(x - y) > ulps)
        return FALSE;

    return TRUE;
}

static BOOL compare_colour_f(const D2D1_COLOR_F *colour, float r, float g, float b, float a, unsigned int ulps)
{
    return compare_float(colour->r, r, ulps)
            && compare_float(colour->g, g, ulps)
            && compare_float(colour->b, b, ulps)
            && compare_float(colour->a, a, ulps);
}

static BOOL compare_point(const D2D1_POINT_2F *point, float x, float y, unsigned int ulps)
{
    return compare_float(point->x, x, ulps)
            && compare_float(point->y, y, ulps);
}

static BOOL compare_rect(const D2D1_RECT_F *rect, float left, float top, float right, float bottom, unsigned int ulps)
{
    return compare_float(rect->left, left, ulps)
            && compare_float(rect->top, top, ulps)
            && compare_float(rect->right, right, ulps)
            && compare_float(rect->bottom, bottom, ulps);
}

static BOOL compare_bezier_segment(const D2D1_BEZIER_SEGMENT *b, float x1, float y1,
        float x2, float y2, float x3, float y3, unsigned int ulps)
{
    return compare_point(&b->point1, x1, y1, ulps)
            && compare_point(&b->point2, x2, y2, ulps)
            && compare_point(&b->point3, x3, y3, ulps);
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

static BOOL compare_surface(struct d2d1_test_context *ctx, const char *ref_sha1)
{
    struct resource_readback rb;
    BOOL ret;

    get_surface_readback(ctx, &rb);
    ret = compare_sha1(rb.data, rb.pitch, 4, rb.width, rb.height, ref_sha1);
    release_resource_readback(&rb);

    return ret;
}

static BOOL compare_wic_bitmap(IWICBitmap *bitmap, const char *ref_sha1)
{
    UINT stride, width, height, buffer_size;
    IWICBitmapLock *lock;
    BYTE *data;
    HRESULT hr;
    BOOL ret;

    hr = IWICBitmap_Lock(bitmap, NULL, WICBitmapLockRead, &lock);
    ok(SUCCEEDED(hr), "Failed to lock bitmap, hr %#x.\n", hr);

    hr = IWICBitmapLock_GetDataPointer(lock, &buffer_size, &data);
    ok(SUCCEEDED(hr), "Failed to get bitmap data, hr %#x.\n", hr);

    hr = IWICBitmapLock_GetStride(lock, &stride);
    ok(SUCCEEDED(hr), "Failed to get bitmap stride, hr %#x.\n", hr);

    hr = IWICBitmapLock_GetSize(lock, &width, &height);
    ok(SUCCEEDED(hr), "Failed to get bitmap size, hr %#x.\n", hr);

    ret = compare_sha1(data, stride, 4, width, height, ref_sha1);

    IWICBitmapLock_Release(lock);

    return ret;
}

static void serialize_figure(struct figure *figure)
{
    static const char lookup[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    unsigned int i, j, k, span;
    char output[76];
    char t[3];
    char *p;

    for (i = 0, j = 0, k = 0, p = output; i < figure->span_count; ++i)
    {
        span = figure->spans[i];
        while (span)
        {
            t[j] = span & 0x7f;
            if (span > 0x7f)
                t[j] |= 0x80;
            span >>= 7;
            if (++j == 3)
            {
                p[0] = lookup[(t[0] & 0xfc) >> 2];
                p[1] = lookup[((t[0] & 0x03) << 4) | ((t[1] & 0xf0) >> 4)];
                p[2] = lookup[((t[1] & 0x0f) << 2) | ((t[2] & 0xc0) >> 6)];
                p[3] = lookup[t[2] & 0x3f];
                p += 4;
                if (++k == 19)
                {
                    trace("%.76s\n", output);
                    p = output;
                    k = 0;
                }
                j = 0;
            }
        }
    }
    if (j)
    {
        for (i = j; i < 3; ++i)
            t[i] = 0;
        p[0] = lookup[(t[0] & 0xfc) >> 2];
        p[1] = lookup[((t[0] & 0x03) << 4) | ((t[1] & 0xf0) >> 4)];
        p[2] = lookup[((t[1] & 0x0f) << 2) | ((t[2] & 0xc0) >> 6)];
        p[3] = lookup[t[2] & 0x3f];
        ++k;
    }
    if (k)
        trace("%.*s\n", k * 4, output);
}

static void figure_add_span(struct figure *figure, unsigned int span)
{
    if (figure->span_count == figure->spans_size)
    {
        figure->spans_size *= 2;
        figure->spans = HeapReAlloc(GetProcessHeap(), 0, figure->spans,
                figure->spans_size * sizeof(*figure->spans));
    }

    figure->spans[figure->span_count++] = span;
}

static void deserialize_span(struct figure *figure, unsigned int *current, unsigned int *shift, unsigned int c)
{
    *current |= (c & 0x7f) << *shift;
    if (c & 0x80)
    {
        *shift += 7;
        return;
    }

    if (*current)
        figure_add_span(figure, *current);
    *current = 0;
    *shift = 0;
}

static void deserialize_figure(struct figure *figure, const BYTE *s)
{
    static const BYTE lookup[] =
    {
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x3e, 0xff, 0xff, 0xff, 0x3f,
        0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
        0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
        0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0xff, 0xff, 0xff, 0xff, 0xff,
    };
    unsigned int current = 0, shift = 0;
    const BYTE *ptr;
    BYTE x, y;

    figure->span_count = 0;
    figure->spans_size = 64;
    figure->spans = HeapAlloc(GetProcessHeap(), 0, figure->spans_size * sizeof(*figure->spans));

    for (ptr = s; *ptr; ptr += 4)
    {
        x = lookup[ptr[0]];
        y = lookup[ptr[1]];
        deserialize_span(figure, &current, &shift, ((x & 0x3f) << 2) | ((y & 0x3f) >> 4));
        x = lookup[ptr[2]];
        deserialize_span(figure, &current, &shift, ((y & 0x0f) << 4) | ((x & 0x3f) >> 2));
        y = lookup[ptr[3]];
        deserialize_span(figure, &current, &shift, ((x & 0x03) << 6) | (y & 0x3f));
    }
}

static void read_figure(struct figure *figure, BYTE *data, unsigned int pitch,
        unsigned int x, unsigned int y,  unsigned int w, unsigned int h, DWORD prev)
{
    unsigned int i, j, span;

    figure->span_count = 0;
    for (i = 0, span = 0; i < h; ++i)
    {
        const DWORD *row = (DWORD *)&data[(y + i) * pitch + x * 4];
        for (j = 0; j < w; ++j, ++span)
        {
            if ((i || j) && prev != row[j])
            {
                figure_add_span(figure, span);
                prev = row[j];
                span = 0;
            }
        }
    }
    if (span)
        figure_add_span(figure, span);
}

static BOOL compare_figure(struct d2d1_test_context *ctx, unsigned int x, unsigned int y,
        unsigned int w, unsigned int h, DWORD prev, unsigned int max_diff, const char *ref)
{
    struct figure ref_figure, figure;
    unsigned int i, j, span, diff;
    struct resource_readback rb;

    get_surface_readback(ctx, &rb);

    figure.span_count = 0;
    figure.spans_size = 64;
    figure.spans = HeapAlloc(GetProcessHeap(), 0, figure.spans_size * sizeof(*figure.spans));

    read_figure(&figure, rb.data, rb.pitch, x, y, w, h, prev);

    deserialize_figure(&ref_figure, (BYTE *)ref);
    span = w * h;
    for (i = 0; i < ref_figure.span_count; ++i)
    {
        span -= ref_figure.spans[i];
    }
    if (span)
        figure_add_span(&ref_figure, span);

    for (i = 0, j = 0, diff = 0; i < figure.span_count && j < ref_figure.span_count;)
    {
        if (figure.spans[i] == ref_figure.spans[j])
        {
            if ((i ^ j) & 1)
                diff += ref_figure.spans[j];
            ++i;
            ++j;
        }
        else if (figure.spans[i] > ref_figure.spans[j])
        {
            if ((i ^ j) & 1)
                diff += ref_figure.spans[j];
            figure.spans[i] -= ref_figure.spans[j];
            ++j;
        }
        else
        {
            if ((i ^ j) & 1)
                diff += figure.spans[i];
            ref_figure.spans[j] -= figure.spans[i];
            ++i;
        }
    }
    if (diff > max_diff)
    {
        trace("diff %u > max_diff %u.\n", diff, max_diff);
        read_figure(&figure, rb.data, rb.pitch, x, y, w, h, prev);
        serialize_figure(&figure);
    }

    HeapFree(GetProcessHeap(), 0, ref_figure.spans);
    HeapFree(GetProcessHeap(), 0, figure.spans);
    release_resource_readback(&rb);

    return diff <= max_diff;
}

static ID3D10Device1 *create_d3d10_device(void)
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

static ID3D11Device *create_d3d11_device(void)
{
    DWORD level = D3D_FEATURE_LEVEL_11_0;
    ID3D11Device *device;

    if (SUCCEEDED(D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL,
            D3D10_CREATE_DEVICE_BGRA_SUPPORT, &level, 1, D3D11_SDK_VERSION, &device, NULL, NULL)))
        return device;
    if (SUCCEEDED(D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_WARP, NULL,
            D3D10_CREATE_DEVICE_BGRA_SUPPORT, &level, 1, D3D11_SDK_VERSION, &device, NULL, NULL)))
        return device;
    if (SUCCEEDED(D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_REFERENCE, NULL,
            D3D10_CREATE_DEVICE_BGRA_SUPPORT, &level, 1, D3D11_SDK_VERSION, &device, NULL, NULL)))
        return device;

    return NULL;
}

static IDXGIDevice *create_device(BOOL d3d11)
{
    ID3D10Device1 *d3d10_device;
    ID3D11Device *d3d11_device;
    IDXGIDevice *device;
    HRESULT hr;

    if (d3d11)
    {
        if (!(d3d11_device = create_d3d11_device()))
            return NULL;
        hr = ID3D11Device_QueryInterface(d3d11_device, &IID_IDXGIDevice, (void **)&device);
        ID3D11Device_Release(d3d11_device);
    }
    else
    {
        if (!(d3d10_device = create_d3d10_device()))
            return NULL;
        hr = ID3D10Device1_QueryInterface(d3d10_device, &IID_IDXGIDevice, (void **)&device);
        ID3D10Device1_Release(d3d10_device);
    }

    ok(SUCCEEDED(hr), "Failed to get DXGI device, hr %#x.\n", hr);

    return device;
}

static HWND create_window(void)
{
    RECT r = {0, 0, 640, 480};

    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW | WS_VISIBLE, FALSE);

    return CreateWindowA("static", "d2d1_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, r.right - r.left, r.bottom - r.top, NULL, NULL, NULL, NULL);
}

static IDXGISwapChain *create_swapchain(IDXGIDevice *device, HWND window, BOOL windowed)
{
    IDXGISwapChain *swapchain;
    DXGI_SWAP_CHAIN_DESC desc;
    IDXGIAdapter *adapter;
    IDXGIFactory *factory;
    HRESULT hr;

    hr = IDXGIDevice_GetAdapter(device, &adapter);
    ok(SUCCEEDED(hr), "Failed to get adapter, hr %#x.\n", hr);
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

static IDXGISwapChain *create_d3d10_swapchain(ID3D10Device1 *device, HWND window, BOOL windowed)
{
    IDXGISwapChain *swapchain;
    IDXGIDevice *dxgi_device;
    HRESULT hr;

    hr = ID3D10Device1_QueryInterface(device, &IID_IDXGIDevice, (void **)&dxgi_device);
    ok(SUCCEEDED(hr), "Failed to get DXGI device, hr %#x.\n", hr);
    swapchain = create_swapchain(dxgi_device, window, windowed);
    IDXGIDevice_Release(dxgi_device);
    return swapchain;
}

static ID2D1RenderTarget *create_render_target_desc(IDXGISurface *surface,
        const D2D1_RENDER_TARGET_PROPERTIES *desc, BOOL d3d11)
{
    ID2D1RenderTarget *render_target;
    ID2D1Factory *factory;
    HRESULT hr;

    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &IID_ID2D1Factory, NULL, (void **)&factory);
    ok(SUCCEEDED(hr), "Failed to create factory, hr %#x.\n", hr);
    hr = ID2D1Factory_CreateDxgiSurfaceRenderTarget(factory, surface, desc, &render_target);
    ok(SUCCEEDED(hr), "Failed to create render target, hr %#x.\n", hr);
    ID2D1Factory_Release(factory);

    return render_target;
}

static ID2D1RenderTarget *create_render_target(IDXGISurface *surface, BOOL d3d11)
{
    D2D1_RENDER_TARGET_PROPERTIES desc;

    desc.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
    desc.pixelFormat.format = DXGI_FORMAT_UNKNOWN;
    desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    desc.dpiX = 0.0f;
    desc.dpiY = 0.0f;
    desc.usage = D2D1_RENDER_TARGET_USAGE_NONE;
    desc.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;

    return create_render_target_desc(surface, &desc, d3d11);
}

#define release_test_context(ctx) release_test_context_(__LINE__, ctx)
static void release_test_context_(unsigned int line, struct d2d1_test_context *ctx)
{
    ID2D1Factory *factory;
    ULONG ref;

    ID2D1RenderTarget_GetFactory(ctx->rt, &factory);
    ID2D1RenderTarget_Release(ctx->rt);
    ref = ID2D1Factory_Release(factory);
    ok_(__FILE__, line)(!ref, "Factory has %u references left.\n", ref);

    IDXGISurface_Release(ctx->surface);
    IDXGISwapChain_Release(ctx->swapchain);
    DestroyWindow(ctx->window);
    IDXGIDevice_Release(ctx->device);
}

#define init_test_context(ctx, d3d11) init_test_context_(__LINE__, ctx, d3d11)
static BOOL init_test_context_(unsigned int line, struct d2d1_test_context *ctx, BOOL d3d11)
{
    HRESULT hr;

    memset(ctx, 0, sizeof(*ctx));

    ctx->d3d11 = d3d11;
    if (!(ctx->device = create_device(d3d11)))
    {
        skip_(__FILE__, line)("Failed to create device, skipping tests.\n");
        return FALSE;
    }

    ctx->window = create_window();
    ok_(__FILE__, line)(!!ctx->window, "Failed to create test window.\n");
    ctx->swapchain = create_swapchain(ctx->device, ctx->window, TRUE);
    ok_(__FILE__, line)(!!ctx->swapchain, "Failed to create swapchain.\n");
    hr = IDXGISwapChain_GetBuffer(ctx->swapchain, 0, &IID_IDXGISurface, (void **)&ctx->surface);
    ok_(__FILE__, line)(SUCCEEDED(hr), "Failed to get buffer, hr %#x.\n", hr);

    ctx->rt = create_render_target(ctx->surface, d3d11);
    if (!ctx->rt && d3d11)
    {
        todo_wine win_skip_(__FILE__, line)("Skipping d3d11 tests.\n");

        IDXGISurface_Release(ctx->surface);
        IDXGISwapChain_Release(ctx->swapchain);
        DestroyWindow(ctx->window);
        IDXGIDevice_Release(ctx->device);

        return FALSE;
    }
    ok_(__FILE__, line)(!!ctx->rt, "Failed to create render target.\n");

    return TRUE;
}

#define check_bitmap_surface(b, s, o) check_bitmap_surface_(__LINE__, b, s, o)
static void check_bitmap_surface_(unsigned int line, ID2D1Bitmap *bitmap, BOOL has_surface, DWORD expected_options)
{
    D2D1_BITMAP_OPTIONS options;
    IDXGISurface *surface;
    ID2D1Bitmap1 *bitmap1;
    HRESULT hr;

    hr = ID2D1Bitmap_QueryInterface(bitmap, &IID_ID2D1Bitmap1, (void **)&bitmap1);
    if (FAILED(hr))
        return;

    options = ID2D1Bitmap1_GetOptions(bitmap1);
    ok_(__FILE__, line)(options == expected_options, "Unexpected bitmap options %#x, expected %#x.\n",
            options, expected_options);

    surface = (void *)0xdeadbeef;
    hr = ID2D1Bitmap1_GetSurface(bitmap1, &surface);
    if (has_surface)
    {
        D3D10_TEXTURE2D_DESC desc;
        ID3D10Texture2D *texture;
        D2D1_SIZE_U pixel_size;
        DWORD bind_flags = 0;

        ok_(__FILE__, line)(SUCCEEDED(hr), "Failed to get bitmap surface, hr %#x.\n", hr);
        ok_(__FILE__, line)(!!surface, "Expected surface instance.\n");

        /* Correlate with resource configuration. */
        hr = IDXGISurface_QueryInterface(surface, &IID_ID3D10Texture2D, (void **)&texture);
        ok_(__FILE__, line)(SUCCEEDED(hr), "Failed to get texture pointer, hr %#x.\n", hr);

        ID3D10Texture2D_GetDesc(texture, &desc);
        ok_(__FILE__, line)(desc.Usage == 0, "Unexpected usage %#x.\n", desc.Usage);

        if (options & D2D1_BITMAP_OPTIONS_TARGET)
            bind_flags |= D3D10_BIND_RENDER_TARGET;
        if (!(options & D2D1_BITMAP_OPTIONS_CANNOT_DRAW))
            bind_flags |= D3D10_BIND_SHADER_RESOURCE;

        ok_(__FILE__, line)(desc.BindFlags == bind_flags, "Unexpected bind flags %#x for bitmap options %#x.\n",
                desc.BindFlags, options);
        ok_(__FILE__, line)(!desc.CPUAccessFlags, "Unexpected cpu access flags %#x.\n", desc.CPUAccessFlags);
        ok_(__FILE__, line)(!desc.MiscFlags, "Unexpected misc flags %#x.\n", desc.MiscFlags);

        pixel_size = ID2D1Bitmap_GetPixelSize(bitmap);
        if (!pixel_size.width || !pixel_size.height)
            pixel_size.width = pixel_size.height = 1;
        ok_(__FILE__, line)(desc.Width == pixel_size.width, "Got width %u, expected %u.\n",
                desc.Width, pixel_size.width);
        ok_(__FILE__, line)(desc.Height == pixel_size.height, "Got height %u, expected %u.\n",
                desc.Height, pixel_size.height);

        ID3D10Texture2D_Release(texture);

        IDXGISurface_Release(surface);
    }
    else
    {
        ok_(__FILE__, line)(hr == D2DERR_INVALID_CALL, "Unexpected hr %#x.\n", hr);
        ok_(__FILE__, line)(!surface, "Unexpected surface instance.\n");
    }

    ID2D1Bitmap1_Release(bitmap1);
}

static inline struct geometry_sink *impl_from_ID2D1SimplifiedGeometrySink(ID2D1SimplifiedGeometrySink *iface)
{
    return CONTAINING_RECORD(iface, struct geometry_sink, ID2D1SimplifiedGeometrySink_iface);
}

static HRESULT STDMETHODCALLTYPE geometry_sink_QueryInterface(ID2D1SimplifiedGeometrySink *iface,
        REFIID iid, void **out)
{
    if (IsEqualGUID(iid, &IID_ID2D1SimplifiedGeometrySink)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        *out = iface;
        return S_OK;
    }

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE geometry_sink_AddRef(ID2D1SimplifiedGeometrySink *iface)
{
    return 0;
}

static ULONG STDMETHODCALLTYPE geometry_sink_Release(ID2D1SimplifiedGeometrySink *iface)
{
    return 0;
}

static void STDMETHODCALLTYPE geometry_sink_SetFillMode(ID2D1SimplifiedGeometrySink *iface, D2D1_FILL_MODE mode)
{
    struct geometry_sink *sink = impl_from_ID2D1SimplifiedGeometrySink(iface);

    sink->fill_mode = mode;
}

static void STDMETHODCALLTYPE geometry_sink_SetSegmentFlags(ID2D1SimplifiedGeometrySink *iface,
        D2D1_PATH_SEGMENT flags)
{
    struct geometry_sink *sink = impl_from_ID2D1SimplifiedGeometrySink(iface);

    sink->segment_flags = flags;
}

static void STDMETHODCALLTYPE geometry_sink_BeginFigure(ID2D1SimplifiedGeometrySink *iface,
        D2D1_POINT_2F start_point, D2D1_FIGURE_BEGIN figure_begin)
{
    struct geometry_sink *sink = impl_from_ID2D1SimplifiedGeometrySink(iface);
    struct geometry_figure *figure;

    if (sink->figure_count == sink->figures_size)
    {
        sink->figures_size *= 2;
        sink->figures = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sink->figures,
                sink->figures_size * sizeof(*sink->figures));
    }
    figure = &sink->figures[sink->figure_count++];

    figure->begin = figure_begin;
    figure->start_point = start_point;
    figure->segments_size = 4;
    figure->segments = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            figure->segments_size * sizeof(*figure->segments));
}

static struct geometry_segment *geometry_figure_add_segment(struct geometry_figure *figure)
{
    if (figure->segment_count == figure->segments_size)
    {
        figure->segments_size *= 2;
        figure->segments = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, figure->segments,
                figure->segments_size * sizeof(*figure->segments));
    }
    return &figure->segments[figure->segment_count++];
}

static void STDMETHODCALLTYPE geometry_sink_AddLines(ID2D1SimplifiedGeometrySink *iface,
        const D2D1_POINT_2F *points, UINT32 count)
{
    struct geometry_sink *sink = impl_from_ID2D1SimplifiedGeometrySink(iface);
    struct geometry_figure *figure = &sink->figures[sink->figure_count - 1];
    struct geometry_segment *segment;
    unsigned int i;

    for (i = 0; i < count; ++i)
    {
        segment = geometry_figure_add_segment(figure);
        segment->type = SEGMENT_LINE;
        segment->u.line = points[i];
        segment->flags = sink->segment_flags;
    }
}

static void STDMETHODCALLTYPE geometry_sink_AddBeziers(ID2D1SimplifiedGeometrySink *iface,
        const D2D1_BEZIER_SEGMENT *beziers, UINT32 count)
{
    struct geometry_sink *sink = impl_from_ID2D1SimplifiedGeometrySink(iface);
    struct geometry_figure *figure = &sink->figures[sink->figure_count - 1];
    struct geometry_segment *segment;
    unsigned int i;

    for (i = 0; i < count; ++i)
    {
        segment = geometry_figure_add_segment(figure);
        segment->type = SEGMENT_BEZIER;
        segment->u.bezier = beziers[i];
        segment->flags = sink->segment_flags;
    }
}

static void STDMETHODCALLTYPE geometry_sink_EndFigure(ID2D1SimplifiedGeometrySink *iface,
        D2D1_FIGURE_END figure_end)
{
    struct geometry_sink *sink = impl_from_ID2D1SimplifiedGeometrySink(iface);
    struct geometry_figure *figure = &sink->figures[sink->figure_count - 1];

    figure->end = figure_end;
}

static HRESULT STDMETHODCALLTYPE geometry_sink_Close(ID2D1SimplifiedGeometrySink *iface)
{
    struct geometry_sink *sink = impl_from_ID2D1SimplifiedGeometrySink(iface);

    sink->closed = TRUE;

    return S_OK;
}

static const struct ID2D1SimplifiedGeometrySinkVtbl geometry_sink_vtbl =
{
    geometry_sink_QueryInterface,
    geometry_sink_AddRef,
    geometry_sink_Release,
    geometry_sink_SetFillMode,
    geometry_sink_SetSegmentFlags,
    geometry_sink_BeginFigure,
    geometry_sink_AddLines,
    geometry_sink_AddBeziers,
    geometry_sink_EndFigure,
    geometry_sink_Close,
};

static void geometry_sink_init(struct geometry_sink *sink)
{
    memset(sink, 0, sizeof(*sink));
    sink->ID2D1SimplifiedGeometrySink_iface.lpVtbl = &geometry_sink_vtbl;
    sink->figures_size = 4;
    sink->figures = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            sink->figures_size * sizeof(*sink->figures));
}

static void geometry_sink_cleanup(struct geometry_sink *sink)
{
    unsigned int i;

    for (i = 0; i < sink->figure_count; ++i)
    {
        HeapFree(GetProcessHeap(), 0, sink->figures[i].segments);
    }
    HeapFree(GetProcessHeap(), 0, sink->figures);
}

#define geometry_sink_check(a, b, c, d, e) geometry_sink_check_(__LINE__, a, b, c, d, e)
static void geometry_sink_check_(unsigned int line, const struct geometry_sink *sink, D2D1_FILL_MODE fill_mode,
        unsigned int figure_count, const struct expected_geometry_figure *expected_figures, unsigned int ulps)
{
    const struct geometry_segment *segment, *expected_segment;
    const struct expected_geometry_figure *expected_figure;
    const struct geometry_figure *figure;
    unsigned int i, j;
    BOOL match;

    ok_(__FILE__, line)(sink->fill_mode == fill_mode,
            "Got unexpected fill mode %#x.\n", sink->fill_mode);
    ok_(__FILE__, line)(sink->figure_count == figure_count,
            "Got unexpected figure count %u, expected %u.\n", sink->figure_count, figure_count);
    ok_(__FILE__, line)(!sink->closed, "Sink is closed.\n");

    for (i = 0; i < figure_count; ++i)
    {
        expected_figure = &expected_figures[i];
        figure = &sink->figures[i];

        ok_(__FILE__, line)(figure->begin == expected_figure->begin,
                "Got unexpected figure %u begin %#x, expected %#x.\n",
                i, figure->begin, expected_figure->begin);
        ok_(__FILE__, line)(figure->end == expected_figure->end,
                "Got unexpected figure %u end %#x, expected %#x.\n",
                i, figure->end, expected_figure->end);
        match = compare_point(&figure->start_point,
                expected_figure->start_point.x, expected_figure->start_point.y, ulps);
        ok_(__FILE__, line)(match, "Got unexpected figure %u start point {%.8e, %.8e}, expected {%.8e, %.8e}.\n",
                i, figure->start_point.x, figure->start_point.y,
                expected_figure->start_point.x, expected_figure->start_point.y);
        ok_(__FILE__, line)(figure->segment_count == expected_figure->segment_count,
                "Got unexpected figure %u segment count %u, expected %u.\n",
                i, figure->segment_count, expected_figure->segment_count);

        for (j = 0; j < figure->segment_count; ++j)
        {
            expected_segment = &expected_figure->segments[j];
            segment = &figure->segments[j];
            ok_(__FILE__, line)(segment->type == expected_segment->type,
                    "Got unexpected figure %u, segment %u type %#x, expected %#x.\n",
                    i, j, segment->type, expected_segment->type);
            ok_(__FILE__, line)(segment->flags == expected_segment->flags,
                    "Got unexpected figure %u, segment %u flags %#x, expected %#x.\n",
                    i, j, segment->flags, expected_segment->flags);
            switch (segment->type)
            {
                case SEGMENT_LINE:
                    match = compare_point(&segment->u.line,
                            expected_segment->u.line.x, expected_segment->u.line.y, ulps);
                    ok_(__FILE__, line)(match, "Got unexpected figure %u segment %u {%.8e, %.8e}, "
                            "expected {%.8e, %.8e}.\n",
                            i, j, segment->u.line.x, segment->u.line.y,
                            expected_segment->u.line.x, expected_segment->u.line.y);
                    break;

                case SEGMENT_BEZIER:
                    match = compare_bezier_segment(&segment->u.bezier,
                            expected_segment->u.bezier.point1.x, expected_segment->u.bezier.point1.y,
                            expected_segment->u.bezier.point2.x, expected_segment->u.bezier.point2.y,
                            expected_segment->u.bezier.point3.x, expected_segment->u.bezier.point3.y,
                            ulps);
                    ok_(__FILE__, line)(match, "Got unexpected figure %u segment %u "
                            "{%.8e, %.8e, %.8e, %.8e, %.8e, %.8e}, "
                            "expected {%.8e, %.8e, %.8e, %.8e, %.8e, %.8e}.\n",
                            i, j, segment->u.bezier.point1.x, segment->u.bezier.point1.y,
                            segment->u.bezier.point2.x, segment->u.bezier.point2.y,
                            segment->u.bezier.point3.x, segment->u.bezier.point3.y,
                            expected_segment->u.bezier.point1.x, expected_segment->u.bezier.point1.y,
                            expected_segment->u.bezier.point2.x, expected_segment->u.bezier.point2.y,
                            expected_segment->u.bezier.point3.x, expected_segment->u.bezier.point3.y);
                    break;
            }
        }
    }
}

static void test_clip(BOOL d3d11)
{
    struct d2d1_test_context ctx;
    D2D1_MATRIX_3X2_F matrix;
    D2D1_SIZE_U pixel_size;
    ID2D1RenderTarget *rt;
    D2D1_POINT_2F point;
    D2D1_COLOR_F color;
    float dpi_x, dpi_y;
    D2D1_RECT_F rect;
    D2D1_SIZE_F size;
    HRESULT hr;
    BOOL match;
    static const D2D1_MATRIX_3X2_F identity =
    {{{
        1.0f, 0.0f,
        0.0f, 1.0f,
        0.0f, 0.0f,
    }}};

    if (!init_test_context(&ctx, d3d11))
        return;

    rt = ctx.rt;
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

    ID2D1RenderTarget_SetDpi(rt, 192.0f, 192.0f);
    ID2D1RenderTarget_SetDpi(rt, 0.0f, 96.0f);
    ID2D1RenderTarget_GetDpi(rt, &dpi_x, &dpi_y);
    ok(dpi_x == 192.0f, "Got unexpected dpi_x %.8e.\n", dpi_x);
    ok(dpi_y == 192.0f, "Got unexpected dpi_y %.8e.\n", dpi_y);

    ID2D1RenderTarget_SetDpi(rt, -10.0f, 96.0f);
    ID2D1RenderTarget_GetDpi(rt, &dpi_x, &dpi_y);
    ok(dpi_x == 192.0f, "Got unexpected dpi_x %.8e.\n", dpi_x);
    ok(dpi_y == 192.0f, "Got unexpected dpi_y %.8e.\n", dpi_y);

    ID2D1RenderTarget_SetDpi(rt, 96.0f, -10.0f);
    ID2D1RenderTarget_GetDpi(rt, &dpi_x, &dpi_y);
    ok(dpi_x == 192.0f, "Got unexpected dpi_x %.8e.\n", dpi_x);
    ok(dpi_y == 192.0f, "Got unexpected dpi_y %.8e.\n", dpi_y);

    ID2D1RenderTarget_SetDpi(rt, 96.0f, 0.0f);
    ID2D1RenderTarget_GetDpi(rt, &dpi_x, &dpi_y);
    ok(dpi_x == 192.0f, "Got unexpected dpi_x %.8e.\n", dpi_x);
    ok(dpi_y == 192.0f, "Got unexpected dpi_y %.8e.\n", dpi_y);

    ID2D1RenderTarget_SetDpi(rt, 96.0f, 96.0f);

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
    match = compare_surface(&ctx, "035a44d4198d6e422e9de6185b5b2c2bac5e33c9");
    ok(match, "Surface does not match.\n");

    /* Fractional clip rectangle coordinates, aliased mode. */
    set_matrix_identity(&matrix);
    ID2D1RenderTarget_SetTransform(rt, &matrix);
    ID2D1RenderTarget_SetDpi(rt, 96.0f, 96.0f);

    ID2D1RenderTarget_BeginDraw(rt);

    set_color(&color, 0.0f, 0.0f, 0.0f, 1.0f);
    ID2D1RenderTarget_Clear(rt, &color);

    scale_matrix(&matrix, 2.0f, 2.0f);
    ID2D1RenderTarget_SetTransform(rt, &matrix);
    set_rect(&rect, 0.0f, 0.5f, 200.0f, 100.5f);
    set_color(&color, 1.0f, 0.0f, 1.0f, 1.0f);
    ID2D1RenderTarget_PushAxisAlignedClip(rt, &rect, D2D1_ANTIALIAS_MODE_ALIASED);
    ID2D1RenderTarget_Clear(rt, &color);
    ID2D1RenderTarget_PopAxisAlignedClip(rt);

    set_matrix_identity(&matrix);
    ID2D1RenderTarget_SetTransform(rt, &matrix);
    set_rect(&rect, 0.0f, 0.5f, 100.0f, 200.5f);
    set_color(&color, 1.0f, 0.0f, 0.0f, 1.0f);
    ID2D1RenderTarget_PushAxisAlignedClip(rt, &rect, D2D1_ANTIALIAS_MODE_ALIASED);
    ID2D1RenderTarget_Clear(rt, &color);
    ID2D1RenderTarget_PopAxisAlignedClip(rt);

    ID2D1RenderTarget_SetTransform(rt, &matrix);
    set_rect(&rect, 0.5f, 250.0f, 100.5f, 300.0f);
    set_color(&color, 1.0f, 1.0f, 0.0f, 1.0f);
    ID2D1RenderTarget_PushAxisAlignedClip(rt, &rect, D2D1_ANTIALIAS_MODE_ALIASED);
    ID2D1RenderTarget_Clear(rt, &color);
    ID2D1RenderTarget_PopAxisAlignedClip(rt);

    translate_matrix(&matrix, 0.1f, 0.0f);
    ID2D1RenderTarget_SetTransform(rt, &matrix);
    set_rect(&rect, 110.0f, 250.25f, 150.0f, 300.25f);
    set_color(&color, 0.0f, 1.0f, 0.0f, 1.0f);
    ID2D1RenderTarget_PushAxisAlignedClip(rt, &rect, D2D1_ANTIALIAS_MODE_ALIASED);
    ID2D1RenderTarget_Clear(rt, &color);
    ID2D1RenderTarget_PopAxisAlignedClip(rt);

    set_rect(&rect, 160.0f, 250.75f, 200.0f, 300.75f);
    set_color(&color, 0.0f, 0.0f, 1.0f, 1.0f);
    ID2D1RenderTarget_PushAxisAlignedClip(rt, &rect, D2D1_ANTIALIAS_MODE_ALIASED);
    ID2D1RenderTarget_Clear(rt, &color);
    ID2D1RenderTarget_PopAxisAlignedClip(rt);

    ID2D1RenderTarget_SetDpi(rt, 48.0f, 192.0f);
    set_rect(&rect, 160.25f, 0.0f, 200.25f, 100.0f);
    set_color(&color, 1.0f, 0.0f, 1.0f, 1.0f);
    ID2D1RenderTarget_PushAxisAlignedClip(rt, &rect, D2D1_ANTIALIAS_MODE_ALIASED);
    ID2D1RenderTarget_Clear(rt, &color);
    ID2D1RenderTarget_PopAxisAlignedClip(rt);

    ID2D1RenderTarget_SetDpi(rt, 192.0f, 48.0f);
    set_rect(&rect, 160.75f, 100.0f, 200.75f, 120.0f);
    set_color(&color, 0.0f, 1.0f, 1.0f, 1.0f);
    ID2D1RenderTarget_PushAxisAlignedClip(rt, &rect, D2D1_ANTIALIAS_MODE_ALIASED);
    ID2D1RenderTarget_Clear(rt, &color);
    ID2D1RenderTarget_PopAxisAlignedClip(rt);

    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to end draw, hr %#x.\n", hr);
    match = compare_surface(&ctx, "cb418ec4a7c8407b5e36db06fc6292a06bb8476c");
    ok(match, "Surface does not match.\n");

    release_test_context(&ctx);
}

static void test_state_block(BOOL d3d11)
{
    IDWriteRenderingParams *text_rendering_params1, *text_rendering_params2;
    D2D1_DRAWING_STATE_DESCRIPTION drawing_state;
    ID2D1DrawingStateBlock *state_block;
    IDWriteFactory *dwrite_factory;
    struct d2d1_test_context ctx;
    ID2D1Factory1 *factory1;
    ID2D1RenderTarget *rt;
    ID2D1Factory *factory;
    ULONG refcount;
    HRESULT hr;
    static const D2D1_MATRIX_3X2_F identity =
    {{{
        1.0f, 0.0f,
        0.0f, 1.0f,
        0.0f, 0.0f,
    }}};
    static const D2D1_MATRIX_3X2_F transform1 =
    {{{
        1.0f, 2.0f,
        3.0f, 4.0f,
        5.0f, 6.0f,
    }}};
    static const D2D1_MATRIX_3X2_F transform2 =
    {{{
        7.0f,  8.0f,
        9.0f,  10.0f,
        11.0f, 12.0f,
    }}};

    if (!init_test_context(&ctx, d3d11))
        return;

    rt = ctx.rt;
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
    ok(!drawing_state.tag1 && !drawing_state.tag2, "Got unexpected tags %s:%s.\n",
            wine_dbgstr_longlong(drawing_state.tag1), wine_dbgstr_longlong(drawing_state.tag2));
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
    ok(!drawing_state.tag1 && !drawing_state.tag2, "Got unexpected tags %s:%s.\n",
            wine_dbgstr_longlong(drawing_state.tag1), wine_dbgstr_longlong(drawing_state.tag2));
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
    ok(drawing_state.tag1 == 0xdead && drawing_state.tag2 == 0xbeef, "Got unexpected tags %s:%s.\n",
            wine_dbgstr_longlong(drawing_state.tag1), wine_dbgstr_longlong(drawing_state.tag2));
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
    ok(drawing_state.tag1 == 0xdead && drawing_state.tag2 == 0xbeef, "Got unexpected tags %s:%s.\n",
            wine_dbgstr_longlong(drawing_state.tag1), wine_dbgstr_longlong(drawing_state.tag2));
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
    ok(drawing_state.tag1 == 1 && drawing_state.tag2 == 2, "Got unexpected tags %s:%s.\n",
            wine_dbgstr_longlong(drawing_state.tag1), wine_dbgstr_longlong(drawing_state.tag2));
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
    ok(drawing_state.tag1 == 1 && drawing_state.tag2 == 2, "Got unexpected tags %s:%s.\n",
            wine_dbgstr_longlong(drawing_state.tag1), wine_dbgstr_longlong(drawing_state.tag2));
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
    ok(drawing_state.tag1 == 3 && drawing_state.tag2 == 4, "Got unexpected tags %s:%s.\n",
            wine_dbgstr_longlong(drawing_state.tag1), wine_dbgstr_longlong(drawing_state.tag2));
    ok(!memcmp(&drawing_state.transform, &transform1, sizeof(drawing_state.transform)),
            "Got unexpected matrix {%.8e, %.8e, %.8e, %.8e, %.8e, %.8e}.\n",
            drawing_state.transform._11, drawing_state.transform._12, drawing_state.transform._21,
            drawing_state.transform._22, drawing_state.transform._31, drawing_state.transform._32);
    ID2D1DrawingStateBlock_GetTextRenderingParams(state_block, &text_rendering_params2);
    ok(text_rendering_params2 == text_rendering_params1, "Got unexpected text rendering params %p, expected %p.\n",
            text_rendering_params2, text_rendering_params1);
    IDWriteRenderingParams_Release(text_rendering_params2);

    if (SUCCEEDED(ID2D1Factory_QueryInterface(factory, &IID_ID2D1Factory1, (void **)&factory1)))
    {
        D2D1_DRAWING_STATE_DESCRIPTION1 drawing_state1;
        ID2D1DrawingStateBlock1 *state_block1;

        hr = ID2D1DrawingStateBlock_QueryInterface(state_block, &IID_ID2D1DrawingStateBlock1, (void **)&state_block1);
        ok(SUCCEEDED(hr), "Failed to get ID2D1DrawingStateBlock1 interface, hr %#x.\n", hr);

        ID2D1DrawingStateBlock1_GetDescription(state_block1, &drawing_state1);
        ok(drawing_state1.antialiasMode == D2D1_ANTIALIAS_MODE_ALIASED,
                "Got unexpected antialias mode %#x.\n", drawing_state1.antialiasMode);
        ok(drawing_state1.textAntialiasMode == D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE,
                "Got unexpected text antialias mode %#x.\n", drawing_state1.textAntialiasMode);
        ok(drawing_state1.tag1 == 3 && drawing_state1.tag2 == 4, "Got unexpected tags %s:%s.\n",
                wine_dbgstr_longlong(drawing_state1.tag1), wine_dbgstr_longlong(drawing_state1.tag2));
        ok(!memcmp(&drawing_state1.transform, &transform1, sizeof(drawing_state1.transform)),
                "Got unexpected matrix {%.8e, %.8e, %.8e, %.8e, %.8e, %.8e}.\n",
                drawing_state1.transform._11, drawing_state1.transform._12, drawing_state1.transform._21,
                drawing_state1.transform._22, drawing_state1.transform._31, drawing_state1.transform._32);
        ok(drawing_state1.primitiveBlend == D2D1_PRIMITIVE_BLEND_SOURCE_OVER,
                "Got unexpected primitive blend mode %#x.\n", drawing_state1.primitiveBlend);
        ok(drawing_state1.unitMode == D2D1_UNIT_MODE_DIPS, "Got unexpected unit mode %#x.\n", drawing_state1.unitMode);
        ID2D1DrawingStateBlock1_GetTextRenderingParams(state_block1, &text_rendering_params2);
        ok(text_rendering_params2 == text_rendering_params1, "Got unexpected text rendering params %p, expected %p.\n",
                text_rendering_params2, text_rendering_params1);
        IDWriteRenderingParams_Release(text_rendering_params2);

        drawing_state1.primitiveBlend = D2D1_PRIMITIVE_BLEND_COPY;
        drawing_state1.unitMode = D2D1_UNIT_MODE_PIXELS;
        ID2D1DrawingStateBlock1_SetDescription(state_block1, &drawing_state1);
        ID2D1DrawingStateBlock1_GetDescription(state_block1, &drawing_state1);
        ok(drawing_state1.primitiveBlend == D2D1_PRIMITIVE_BLEND_COPY,
                "Got unexpected primitive blend mode %#x.\n", drawing_state1.primitiveBlend);
        ok(drawing_state1.unitMode == D2D1_UNIT_MODE_PIXELS,
                "Got unexpected unit mode %#x.\n", drawing_state1.unitMode);

        ID2D1DrawingStateBlock_SetDescription(state_block, &drawing_state);
        ID2D1DrawingStateBlock1_GetDescription(state_block1, &drawing_state1);
        ok(drawing_state1.primitiveBlend == D2D1_PRIMITIVE_BLEND_COPY,
                "Got unexpected primitive blend mode %#x.\n", drawing_state1.primitiveBlend);
        ok(drawing_state1.unitMode == D2D1_UNIT_MODE_PIXELS,
                "Got unexpected unit mode %#x.\n", drawing_state1.unitMode);

        ID2D1DrawingStateBlock1_Release(state_block1);

        hr = ID2D1Factory1_CreateDrawingStateBlock(factory1, NULL, NULL, &state_block1);
        ok(SUCCEEDED(hr), "Failed to create drawing state block, hr %#x\n", hr);
        ID2D1DrawingStateBlock1_GetDescription(state_block1, &drawing_state1);
        ok(drawing_state1.antialiasMode == D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
                "Got unexpected antialias mode %#x.\n", drawing_state1.antialiasMode);
        ok(drawing_state1.textAntialiasMode == D2D1_TEXT_ANTIALIAS_MODE_DEFAULT,
                "Got unexpected text antialias mode %#x.\n", drawing_state1.textAntialiasMode);
        ok(drawing_state1.tag1 == 0 && drawing_state1.tag2 == 0, "Got unexpected tags %s:%s.\n",
                wine_dbgstr_longlong(drawing_state1.tag1), wine_dbgstr_longlong(drawing_state1.tag2));
        ok(!memcmp(&drawing_state1.transform, &identity, sizeof(drawing_state1.transform)),
                "Got unexpected matrix {%.8e, %.8e, %.8e, %.8e, %.8e, %.8e}.\n",
                drawing_state1.transform._11, drawing_state1.transform._12, drawing_state1.transform._21,
                drawing_state1.transform._22, drawing_state1.transform._31, drawing_state1.transform._32);
        ok(drawing_state1.primitiveBlend == D2D1_PRIMITIVE_BLEND_SOURCE_OVER,
                "Got unexpected primitive blend mode %#x.\n", drawing_state1.primitiveBlend);
        ok(drawing_state1.unitMode == D2D1_UNIT_MODE_DIPS, "Got unexpected unit mode %#x.\n", drawing_state1.unitMode);
        ID2D1DrawingStateBlock1_GetTextRenderingParams(state_block1, &text_rendering_params2);
        ok(!text_rendering_params2, "Got unexpected text rendering params %p.\n", text_rendering_params2);
        ID2D1DrawingStateBlock1_Release(state_block1);

        ID2D1Factory1_Release(factory1);
    }

    ID2D1DrawingStateBlock_Release(state_block);

    refcount = IDWriteRenderingParams_Release(text_rendering_params1);
    ok(!refcount, "Rendering params %u references left.\n", refcount);
    ID2D1Factory_Release(factory);
    release_test_context(&ctx);
}

static void test_color_brush(BOOL d3d11)
{
    D2D1_MATRIX_3X2_F matrix, tmp_matrix;
    D2D1_BRUSH_PROPERTIES brush_desc;
    D2D1_COLOR_F color, tmp_color;
    struct d2d1_test_context ctx;
    ID2D1SolidColorBrush *brush;
    ID2D1RenderTarget *rt;
    D2D1_RECT_F rect;
    float opacity;
    HRESULT hr;
    BOOL match;

    if (!init_test_context(&ctx, d3d11))
        return;

    rt = ctx.rt;
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
    match = compare_surface(&ctx, "6d1218fca5e21fb7e287b3a439d60dbc251f5ceb");
    ok(match, "Surface does not match.\n");

    ID2D1SolidColorBrush_Release(brush);
    release_test_context(&ctx);
}

static void test_bitmap_brush(BOOL d3d11)
{
    D2D1_BITMAP_INTERPOLATION_MODE interpolation_mode;
    ID2D1TransformedGeometry *transformed_geometry;
    ID2D1RectangleGeometry *rectangle_geometry;
    D2D1_MATRIX_3X2_F matrix, tmp_matrix;
    D2D1_BITMAP_PROPERTIES bitmap_desc;
    ID2D1Bitmap *bitmap, *tmp_bitmap;
    D2D1_RECT_F src_rect, dst_rect;
    struct d2d1_test_context ctx;
    D2D1_EXTEND_MODE extend_mode;
    ID2D1BitmapBrush1 *brush1;
    ID2D1BitmapBrush *brush;
    D2D1_SIZE_F image_size;
    ID2D1RenderTarget *rt;
    ID2D1Factory *factory;
    D2D1_COLOR_F color;
    ID2D1Image *image;
    D2D1_SIZE_U size;
    unsigned int i;
    ULONG refcount;
    float opacity;
    HRESULT hr;
    BOOL match;

    static const struct
    {
        D2D1_EXTEND_MODE extend_mode_x;
        D2D1_EXTEND_MODE extend_mode_y;
        float translate_x;
        float translate_y;
        D2D1_RECT_F rect;
    }
    extend_mode_tests[] =
    {
        {D2D1_EXTEND_MODE_MIRROR, D2D1_EXTEND_MODE_MIRROR, -7.0f, 1.0f, {-4.0f,  0.0f, -8.0f,  4.0f}},
        {D2D1_EXTEND_MODE_WRAP,   D2D1_EXTEND_MODE_MIRROR, -3.0f, 1.0f, {-4.0f,  4.0f,  0.0f,  0.0f}},
        {D2D1_EXTEND_MODE_CLAMP,  D2D1_EXTEND_MODE_MIRROR,  1.0f, 1.0f, { 4.0f,  0.0f,  0.0f,  4.0f}},
        {D2D1_EXTEND_MODE_MIRROR, D2D1_EXTEND_MODE_WRAP,   -7.0f, 5.0f, {-8.0f,  8.0f, -4.0f,  4.0f}},
        {D2D1_EXTEND_MODE_WRAP,   D2D1_EXTEND_MODE_WRAP,   -3.0f, 5.0f, { 0.0f,  4.0f, -4.0f,  8.0f}},
        {D2D1_EXTEND_MODE_CLAMP,  D2D1_EXTEND_MODE_WRAP,    1.0f, 5.0f, { 0.0f,  8.0f,  4.0f,  4.0f}},
        {D2D1_EXTEND_MODE_MIRROR, D2D1_EXTEND_MODE_CLAMP,  -7.0f, 9.0f, {-4.0f,  8.0f, -8.0f, 12.0f}},
        {D2D1_EXTEND_MODE_WRAP,   D2D1_EXTEND_MODE_CLAMP,  -3.0f, 9.0f, {-4.0f, 12.0f,  0.0f,  8.0f}},
        {D2D1_EXTEND_MODE_CLAMP,  D2D1_EXTEND_MODE_CLAMP,   1.0f, 9.0f, { 4.0f,  8.0f,  0.0f, 12.0f}},
    };
    static const DWORD bitmap_data[] =
    {
        0xffff0000, 0xffffff00, 0xff00ff00, 0xff00ffff,
        0xff0000ff, 0xffff00ff, 0xff000000, 0xff7f7f7f,
        0xffffffff, 0xffffffff, 0xffffffff, 0xff000000,
        0xffffffff, 0xff000000, 0xff000000, 0xff000000,
    };

    if (!init_test_context(&ctx, d3d11))
        return;

    rt = ctx.rt;
    ID2D1RenderTarget_SetDpi(rt, 192.0f, 48.0f);
    ID2D1RenderTarget_SetAntialiasMode(rt, D2D1_ANTIALIAS_MODE_ALIASED);

    set_size_u(&size, 4, 4);
    bitmap_desc.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    bitmap_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;
    bitmap_desc.dpiX = 96.0f;
    bitmap_desc.dpiY = 96.0f;
    hr = ID2D1RenderTarget_CreateBitmap(rt, size, bitmap_data, 4 * sizeof(*bitmap_data), &bitmap_desc, &bitmap);
    ok(SUCCEEDED(hr), "Failed to create bitmap, hr %#x.\n", hr);
    image_size = ID2D1Bitmap_GetSize(bitmap);

    hr = ID2D1Bitmap_QueryInterface(bitmap, &IID_ID2D1Image, (void **)&image);
    ok(SUCCEEDED(hr) || broken(hr == E_NOINTERFACE) /* Vista */, "Failed to get ID2D1Image, hr %#x.\n", hr);
    if (hr == S_OK)
    {
        ID2D1DeviceContext *context;
        D2D1_POINT_2F offset;
        D2D1_RECT_F src_rect;

        hr = ID2D1RenderTarget_QueryInterface(rt, &IID_ID2D1DeviceContext, (void **)&context);
        ok(hr == S_OK, "Unexpected hr %#x.\n", hr);

        ID2D1RenderTarget_BeginDraw(rt);
        set_color(&color, 0.0f, 0.0f, 1.0f, 1.0f);
        ID2D1RenderTarget_Clear(rt, &color);

        ID2D1RenderTarget_GetTransform(rt, &tmp_matrix);
        set_matrix_identity(&matrix);
        translate_matrix(&matrix, 20.0f, 12.0f);
        scale_matrix(&matrix, 2.0f, 6.0f);
        ID2D1RenderTarget_SetTransform(rt, &matrix);

        /* Crash on Windows 7+ */
        if (0)
        {
            ID2D1DeviceContext_DrawImage(context, NULL, NULL, NULL, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
                    D2D1_COMPOSITE_MODE_SOURCE_OVER);
        }

        ID2D1DeviceContext_DrawImage(context, image, NULL, NULL, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
                D2D1_COMPOSITE_MODE_SOURCE_OVER);

        set_rect(&src_rect, 0.0f, 0.0f, image_size.width, image_size.height);

        ID2D1DeviceContext_DrawImage(context, image, NULL, &src_rect, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
                D2D1_COMPOSITE_MODE_SOURCE_OVER);

        offset.x = -1;
        offset.y = -1;
        ID2D1DeviceContext_DrawImage(context, image, &offset, NULL, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
                D2D1_COMPOSITE_MODE_SOURCE_OVER);

        offset.x = image_size.width * 2;
        offset.y = image_size.height;
        ID2D1DeviceContext_DrawImage(context, image, &offset, NULL, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
                D2D1_COMPOSITE_MODE_SOURCE_OVER);

        offset.x = image_size.width * 3;
        set_rect(&src_rect, image_size.width / 2, image_size.height / 2, image_size.width, image_size.height);
        ID2D1DeviceContext_DrawImage(context, image, &offset, &src_rect, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
                D2D1_COMPOSITE_MODE_SOURCE_OVER);

        offset.x = image_size.width * 4;
        set_rect(&src_rect, 0.0f, 0.0f, image_size.width * 2, image_size.height * 2);
        ID2D1DeviceContext_DrawImage(context, image, &offset, &src_rect, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
                D2D1_COMPOSITE_MODE_SOURCE_OVER);

        offset.x = image_size.width * 5;
        set_rect(&src_rect, image_size.width, image_size.height, 0.0f, 0.0f);
        ID2D1DeviceContext_DrawImage(context, image, &offset, &src_rect, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
                D2D1_COMPOSITE_MODE_SOURCE_OVER);

        hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
        ok(SUCCEEDED(hr), "Failed to end draw, hr %#x.\n", hr);
        match = compare_surface(&ctx, "95675fbc4a16404c9568d41b14e8f6be64240998");
        ok(match, "Surface does not match.\n");

        ID2D1RenderTarget_BeginDraw(rt);

        offset.x = image_size.width * 6;
        set_rect(&src_rect, 1.0f, 0.0f, 1.0f, image_size.height);
        ID2D1DeviceContext_DrawImage(context, image, &offset, &src_rect, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
                D2D1_COMPOSITE_MODE_SOURCE_OVER);

        hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
        ok(SUCCEEDED(hr), "Failed to end draw, hr %#x.\n", hr);
        match = compare_surface(&ctx, "95675fbc4a16404c9568d41b14e8f6be64240998");
        ok(match, "Surface does not match.\n");

        ID2D1RenderTarget_SetTransform(rt, &tmp_matrix);
        ID2D1DeviceContext_Release(context);
        ID2D1Image_Release(image);
    }

    /* Creating a brush with a NULL bitmap crashes on Vista, but works fine on
     * Windows 7+. */
    hr = ID2D1RenderTarget_CreateBitmapBrush(rt, bitmap, NULL, NULL, &brush);
    ok(SUCCEEDED(hr), "Failed to create brush, hr %#x.\n", hr);
    ID2D1BitmapBrush_GetBitmap(brush, &tmp_bitmap);
    ok(tmp_bitmap == bitmap, "Got unexpected bitmap %p, expected %p.\n", tmp_bitmap, bitmap);
    ID2D1Bitmap_Release(tmp_bitmap);
    opacity = ID2D1BitmapBrush_GetOpacity(brush);
    ok(opacity == 1.0f, "Got unexpected opacity %.8e.\n", opacity);
    set_matrix_identity(&matrix);
    ID2D1BitmapBrush_GetTransform(brush, &tmp_matrix);
    ok(!memcmp(&tmp_matrix, &matrix, sizeof(matrix)),
            "Got unexpected matrix {%.8e, %.8e, %.8e, %.8e, %.8e, %.8e}.\n",
            tmp_matrix._11, tmp_matrix._12, tmp_matrix._21,
            tmp_matrix._22, tmp_matrix._31, tmp_matrix._32);
    extend_mode = ID2D1BitmapBrush_GetExtendModeX(brush);
    ok(extend_mode == D2D1_EXTEND_MODE_CLAMP, "Got unexpected extend mode %#x.\n", extend_mode);
    extend_mode = ID2D1BitmapBrush_GetExtendModeY(brush);
    ok(extend_mode == D2D1_EXTEND_MODE_CLAMP, "Got unexpected extend mode %#x.\n", extend_mode);
    interpolation_mode = ID2D1BitmapBrush_GetInterpolationMode(brush);
    ok(interpolation_mode == D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
            "Got unexpected interpolation mode %#x.\n", interpolation_mode);
    ID2D1BitmapBrush_Release(brush);

    hr = ID2D1RenderTarget_CreateBitmapBrush(rt, bitmap, NULL, NULL, &brush);
    ok(SUCCEEDED(hr), "Failed to create brush, hr %#x.\n", hr);
    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 40.0f, 120.0f);
    scale_matrix(&matrix, 20.0f, 60.0f);
    ID2D1BitmapBrush_SetTransform(brush, &matrix);
    ID2D1BitmapBrush_SetInterpolationMode(brush, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR);

    ID2D1RenderTarget_BeginDraw(rt);

    set_color(&color, 0.0f, 0.0f, 1.0f, 1.0f);
    ID2D1RenderTarget_Clear(rt, &color);

    set_rect(&dst_rect, 40.0f, 120.0f, 120.0f, 360.0f);
    ID2D1RenderTarget_FillRectangle(rt, &dst_rect, (ID2D1Brush *)brush);

    set_matrix_identity(&matrix);
    scale_matrix(&matrix, 0.5f, 2.0f);
    translate_matrix(&matrix, 320.0f, 240.0f);
    rotate_matrix(&matrix, M_PI / 4.0f);
    ID2D1RenderTarget_SetTransform(rt, &matrix);
    set_matrix_identity(&matrix);
    translate_matrix(&matrix, -80.0f, -60.0f);
    scale_matrix(&matrix, 64.0f, 32.0f);
    ID2D1BitmapBrush_SetTransform(brush, &matrix);
    ID2D1BitmapBrush_SetOpacity(brush, 0.75f);
    set_rect(&dst_rect, -80.0f, -60.0f, 80.0f, 60.0f);
    ID2D1RenderTarget_FillRectangle(rt, &dst_rect, (ID2D1Brush *)brush);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 200.0f, 120.0f);
    scale_matrix(&matrix, 20.0f, 60.0f);
    ID2D1RenderTarget_SetTransform(rt, &matrix);
    ID2D1RenderTarget_DrawBitmap(rt, bitmap, NULL, 0.25f,
            D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, NULL);
    set_rect(&dst_rect, -4.0f, 12.0f, -8.0f, 8.0f);
    ID2D1RenderTarget_DrawBitmap(rt, bitmap, &dst_rect, 0.75f,
            D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, NULL);
    set_rect(&dst_rect, 0.0f, 8.0f, 4.0f, 12.0f);
    set_rect(&src_rect, 2.0f, 1.0f, 4.0f, 3.0f);
    ID2D1RenderTarget_DrawBitmap(rt, bitmap, &dst_rect, 1.0f,
            D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, &src_rect);
    set_rect(&dst_rect, 4.0f, 12.0f, 12.0f, 20.0f);
    set_rect(&src_rect, 0.0f, 0.0f, image_size.width * 2, image_size.height * 2);
    ID2D1RenderTarget_DrawBitmap(rt, bitmap, &dst_rect, 1.0f,
            D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, &src_rect);
    set_rect(&dst_rect, 4.0f, 8.0f, 12.0f, 12.0f);
    set_rect(&src_rect, image_size.width / 2, image_size.height / 2, image_size.width, image_size.height);
    ID2D1RenderTarget_DrawBitmap(rt, bitmap, &dst_rect, 1.0f,
            D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, &src_rect);
    set_rect(&dst_rect, 0.0f, 4.0f, 4.0f, 8.0f);
    set_rect(&src_rect, image_size.width, 0.0f, 0.0f, image_size.height);
    ID2D1RenderTarget_DrawBitmap(rt, bitmap, &dst_rect, 1.0f,
            D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, &src_rect);

    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to end draw, hr %#x.\n", hr);
    match = compare_surface(&ctx, "f5d039c280fa33ba05496c9883192a34108efbbe");
    ok(match, "Surface does not match.\n");

    /* Invalid interpolation mode. */
    ID2D1RenderTarget_BeginDraw(rt);

    set_rect(&dst_rect, 4.0f, 8.0f, 8.0f, 12.0f);
    set_rect(&src_rect, 0.0f, 1.0f, image_size.width, 1.0f);
    ID2D1RenderTarget_DrawBitmap(rt, bitmap, &dst_rect, 1.0f,
            D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, &src_rect);

    set_rect(&dst_rect, 1.0f, 8.0f, 4.0f, 12.0f);
    set_rect(&src_rect, 2.0f, 1.0f, 4.0f, 3.0f);
    ID2D1RenderTarget_DrawBitmap(rt, bitmap, &dst_rect, 1.0f,
            D2D1_BITMAP_INTERPOLATION_MODE_LINEAR + 1, &src_rect);

    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);
    match = compare_surface(&ctx, "f5d039c280fa33ba05496c9883192a34108efbbe");
    ok(match, "Surface does not match.\n");

    ID2D1RenderTarget_BeginDraw(rt);
    ID2D1RenderTarget_Clear(rt, &color);

    set_rect(&src_rect, image_size.width, 0.0f, 0.0f, image_size.height);
    ID2D1RenderTarget_DrawBitmap(rt, bitmap, NULL, 1.0f,
            D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, &src_rect);

    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to end draw, hr %#x.\n", hr);
    match = compare_surface(&ctx, "59043096393570ad800dbcbfdd644394b79493bd");
    ok(match, "Surface does not match.\n");

    ID2D1RenderTarget_BeginDraw(rt);

    ID2D1RenderTarget_Clear(rt, &color);

    ID2D1BitmapBrush_SetOpacity(brush, 1.0f);
    for (i = 0; i < ARRAY_SIZE(extend_mode_tests); ++i)
    {
        ID2D1BitmapBrush_SetExtendModeX(brush, extend_mode_tests[i].extend_mode_x);
        extend_mode = ID2D1BitmapBrush_GetExtendModeX(brush);
        ok(extend_mode == extend_mode_tests[i].extend_mode_x,
                "Test %u: Got unexpected extend mode %#x, expected %#x.\n",
                i, extend_mode, extend_mode_tests[i].extend_mode_x);
        ID2D1BitmapBrush_SetExtendModeY(brush, extend_mode_tests[i].extend_mode_y);
        extend_mode = ID2D1BitmapBrush_GetExtendModeY(brush);
        ok(extend_mode == extend_mode_tests[i].extend_mode_y,
                "Test %u: Got unexpected extend mode %#x, expected %#x.\n",
                i, extend_mode, extend_mode_tests[i].extend_mode_y);
        set_matrix_identity(&matrix);
        translate_matrix(&matrix, extend_mode_tests[i].translate_x, extend_mode_tests[i].translate_y);
        scale_matrix(&matrix, 0.5f, 0.5f);
        ID2D1BitmapBrush_SetTransform(brush, &matrix);
        ID2D1RenderTarget_FillRectangle(rt, &extend_mode_tests[i].rect, (ID2D1Brush *)brush);
    }

    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to end draw, hr %#x.\n", hr);
    match = compare_surface(&ctx, "b4b775afecdae2d26642001f4faff73663bb8b31");
    ok(match, "Surface does not match.\n");

    ID2D1Bitmap_Release(bitmap);
    bitmap_desc.dpiX = 96.0f / 20.0f;
    bitmap_desc.dpiY = 96.0f / 60.0f;
    hr = ID2D1RenderTarget_CreateBitmap(rt, size, bitmap_data, 4 * sizeof(*bitmap_data), &bitmap_desc, &bitmap);
    ok(SUCCEEDED(hr), "Failed to create bitmap, hr %#x.\n", hr);
    ID2D1BitmapBrush_SetBitmap(brush, bitmap);

    ID2D1RenderTarget_BeginDraw(rt);

    ID2D1RenderTarget_Clear(rt, &color);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 40.0f, 120.0f);
    skew_matrix(&matrix, 0.125f, 2.0f);
    ID2D1RenderTarget_SetTransform(rt, &matrix);
    set_matrix_identity(&matrix);
    ID2D1BitmapBrush_SetTransform(brush, &matrix);
    set_rect(&dst_rect, 0.0f, 0.0f, 80.0f, 240.0f);
    ID2D1RenderTarget_FillRectangle(rt, &dst_rect, (ID2D1Brush *)brush);

    ID2D1RenderTarget_GetFactory(rt, &factory);

    set_rect(&dst_rect, -1.0f, -1.0f, 1.0f, 1.0f);
    hr = ID2D1Factory_CreateRectangleGeometry(factory, &dst_rect, &rectangle_geometry);
    ok(SUCCEEDED(hr), "Failed to create geometry, hr %#x.\n", hr);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 240.0f, 720.0f);
    scale_matrix(&matrix, 40.0f, 120.0f);
    hr = ID2D1Factory_CreateTransformedGeometry(factory, (ID2D1Geometry *)rectangle_geometry,
            &matrix, &transformed_geometry);
    ok(SUCCEEDED(hr), "Failed to create geometry, hr %#x.\n", hr);
    ID2D1RectangleGeometry_Release(rectangle_geometry);

    set_matrix_identity(&matrix);
    ID2D1RenderTarget_SetTransform(rt, &matrix);
    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 200.0f, 600.0f);
    ID2D1BitmapBrush_SetTransform(brush, &matrix);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)transformed_geometry, (ID2D1Brush *)brush, NULL);
    ID2D1TransformedGeometry_Release(transformed_geometry);

    ID2D1Factory_Release(factory);

    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to end draw, hr %#x.\n", hr);
    match = compare_surface(&ctx, "cf7b90ba7b139fdfbe9347e1907d635cfb4ed197");
    ok(match, "Surface does not match.\n");

    if (SUCCEEDED(ID2D1BitmapBrush_QueryInterface(brush, &IID_ID2D1BitmapBrush1, (void **)&brush1)))
    {
        D2D1_INTERPOLATION_MODE interpolation_mode1;

        interpolation_mode = ID2D1BitmapBrush1_GetInterpolationMode(brush1);
        ok(interpolation_mode == D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
                "Unexpected interpolation mode %#x.\n", interpolation_mode);

        interpolation_mode1 = ID2D1BitmapBrush1_GetInterpolationMode1(brush1);
        ok(interpolation_mode1 == D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
                "Unexpected interpolation mode %#x.\n", interpolation_mode1);

        ID2D1BitmapBrush1_SetInterpolationMode1(brush1, D2D1_INTERPOLATION_MODE_CUBIC);
        interpolation_mode = ID2D1BitmapBrush1_GetInterpolationMode(brush1);
        ok(interpolation_mode == D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
                "Unexpected interpolation mode %#x.\n", interpolation_mode);

        interpolation_mode1 = ID2D1BitmapBrush1_GetInterpolationMode1(brush1);
        ok(interpolation_mode1 == D2D1_INTERPOLATION_MODE_CUBIC,
                "Unexpected interpolation mode %#x.\n", interpolation_mode1);

        ID2D1BitmapBrush1_SetInterpolationMode1(brush1, 100);
        interpolation_mode1 = ID2D1BitmapBrush1_GetInterpolationMode1(brush1);
        ok(interpolation_mode1 == D2D1_INTERPOLATION_MODE_CUBIC,
                "Unexpected interpolation mode %#x.\n", interpolation_mode1);

        ID2D1BitmapBrush1_SetInterpolationMode(brush1, 100);
        interpolation_mode1 = ID2D1BitmapBrush1_GetInterpolationMode1(brush1);
        ok(interpolation_mode1 == D2D1_INTERPOLATION_MODE_CUBIC,
                "Unexpected interpolation mode %#x.\n", interpolation_mode1);

        ID2D1BitmapBrush1_SetInterpolationMode(brush1, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
        interpolation_mode = ID2D1BitmapBrush1_GetInterpolationMode(brush1);
        ok(interpolation_mode == D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
                "Unexpected interpolation mode %#x.\n", interpolation_mode);

        interpolation_mode1 = ID2D1BitmapBrush1_GetInterpolationMode1(brush1);
        ok(interpolation_mode1 == D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
                "Unexpected interpolation mode %#x.\n", interpolation_mode1);

        ID2D1BitmapBrush1_Release(brush1);
    }

    ID2D1BitmapBrush_Release(brush);
    refcount = ID2D1Bitmap_Release(bitmap);
    ok(!refcount, "Bitmap has %u references left.\n", refcount);
    release_test_context(&ctx);
}

static void test_linear_brush(BOOL d3d11)
{
    D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES gradient_properties;
    ID2D1GradientStopCollection *gradient, *tmp_gradient;
    ID2D1TransformedGeometry *transformed_geometry;
    ID2D1RectangleGeometry *rectangle_geometry;
    D2D1_MATRIX_3X2_F matrix, tmp_matrix;
    ID2D1LinearGradientBrush *brush;
    struct d2d1_test_context ctx;
    struct resource_readback rb;
    ID2D1RenderTarget *rt;
    ID2D1Factory *factory;
    D2D1_COLOR_F colour;
    D2D1_POINT_2F p;
    unsigned int i;
    ULONG refcount;
    D2D1_RECT_F r;
    float opacity;
    HRESULT hr;

    static const D2D1_GRADIENT_STOP stops[] =
    {
        {0.0f, {1.0f, 0.0f, 0.0f, 1.0f}},
        {0.5f, {0.0f, 1.0f, 0.0f, 1.0f}},
        {1.0f, {0.0f, 0.0f, 1.0f, 1.0f}},
    };
    static const struct
    {
        unsigned int x, y;
        DWORD colour;
    }
    test1[] =
    {
        {80,  80, 0xff857a00}, {240,  80, 0xff926d00}, {400,  80, 0xff9f6000}, {560,  80, 0xffac5300},
        {80, 240, 0xff00eb14}, {240, 240, 0xff00f807}, {400, 240, 0xff06f900}, {560, 240, 0xff13ec00},
        {80, 400, 0xff0053ac}, {240, 400, 0xff005fa0}, {400, 400, 0xff006c93}, {560, 400, 0xff007986},
    },
    test2[] =
    {
        { 40,  30, 0xff005ba4}, {120,  30, 0xffffffff}, { 40,  60, 0xffffffff}, { 80,  60, 0xff00b44b},
        {120,  60, 0xff006c93}, {200,  60, 0xffffffff}, { 40,  90, 0xffffffff}, {120,  90, 0xff0ef100},
        {160,  90, 0xff00c53a}, {200,  90, 0xffffffff}, { 80, 120, 0xffffffff}, {120, 120, 0xffaf5000},
        {160, 120, 0xff679800}, {200, 120, 0xff1fe000}, {240, 120, 0xffffffff}, {160, 150, 0xffffffff},
        {200, 150, 0xffc03e00}, {240, 150, 0xffffffff}, {280, 150, 0xffffffff}, {320, 150, 0xffffffff},
        {240, 180, 0xffffffff}, {280, 180, 0xffff4040}, {320, 180, 0xffff4040}, {380, 180, 0xffffffff},
        {200, 210, 0xffffffff}, {240, 210, 0xffa99640}, {280, 210, 0xffb28d40}, {320, 210, 0xffbb8440},
        {360, 210, 0xffc47b40}, {400, 210, 0xffffffff}, {200, 240, 0xffffffff}, {280, 240, 0xff41fe40},
        {320, 240, 0xff49f540}, {360, 240, 0xff52ec40}, {440, 240, 0xffffffff}, {240, 270, 0xffffffff},
        {280, 270, 0xff408eb0}, {320, 270, 0xff4097a7}, {360, 270, 0xff40a19e}, {440, 270, 0xffffffff},
        {280, 300, 0xffffffff}, {320, 300, 0xff4040ff}, {360, 300, 0xff4040ff}, {400, 300, 0xff406ad4},
        {440, 300, 0xff4061de}, {480, 300, 0xff4057e7}, {520, 300, 0xff404ef1}, {280, 330, 0xffffffff},
        {360, 330, 0xffffffff}, {400, 330, 0xff40c17e}, {440, 330, 0xff40b788}, {480, 330, 0xff40ae91},
        {520, 330, 0xff40a49b}, {400, 360, 0xff57e740}, {440, 360, 0xff4ef140}, {480, 360, 0xff44fb40},
        {520, 360, 0xff40fa45}, {400, 390, 0xffae9140}, {440, 390, 0xffa49b40}, {480, 390, 0xff9aa540},
        {520, 390, 0xff90ae40},
    };

    if (!init_test_context(&ctx, d3d11))
        return;

    rt = ctx.rt;
    ID2D1RenderTarget_SetDpi(rt, 192.0f, 48.0f);
    ID2D1RenderTarget_SetAntialiasMode(rt, D2D1_ANTIALIAS_MODE_ALIASED);

    hr = ID2D1RenderTarget_CreateGradientStopCollection(rt, stops, ARRAY_SIZE(stops),
            D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, &gradient);
    ok(SUCCEEDED(hr), "Failed to create stop collection, hr %#x.\n", hr);

    set_point(&gradient_properties.startPoint, 320.0f, 0.0f);
    set_point(&gradient_properties.endPoint, 0.0f, 960.0f);
    hr = ID2D1RenderTarget_CreateLinearGradientBrush(rt, &gradient_properties, NULL, gradient, &brush);
    ok(SUCCEEDED(hr), "Failed to create brush, hr %#x.\n", hr);

    opacity = ID2D1LinearGradientBrush_GetOpacity(brush);
    ok(opacity == 1.0f, "Got unexpected opacity %.8e.\n", opacity);
    set_matrix_identity(&matrix);
    ID2D1LinearGradientBrush_GetTransform(brush, &tmp_matrix);
    ok(!memcmp(&tmp_matrix, &matrix, sizeof(matrix)),
            "Got unexpected matrix {%.8e, %.8e, %.8e, %.8e, %.8e, %.8e}.\n",
            tmp_matrix._11, tmp_matrix._12, tmp_matrix._21,
            tmp_matrix._22, tmp_matrix._31, tmp_matrix._32);
    p = ID2D1LinearGradientBrush_GetStartPoint(brush);
    ok(compare_point(&p, 320.0f, 0.0f, 0), "Got unexpected start point {%.8e, %.8e}.\n", p.x, p.y);
    p = ID2D1LinearGradientBrush_GetEndPoint(brush);
    ok(compare_point(&p, 0.0f, 960.0f, 0), "Got unexpected end point {%.8e, %.8e}.\n", p.x, p.y);
    ID2D1LinearGradientBrush_GetGradientStopCollection(brush, &tmp_gradient);
    ok(tmp_gradient == gradient, "Got unexpected gradient %p, expected %p.\n", tmp_gradient, gradient);
    ID2D1GradientStopCollection_Release(tmp_gradient);

    ID2D1RenderTarget_BeginDraw(rt);

    set_color(&colour, 1.0f, 1.0f, 1.0f, 1.0f);
    ID2D1RenderTarget_Clear(rt, &colour);

    set_rect(&r, 0.0f, 0.0f, 320.0f, 960.0f);
    ID2D1RenderTarget_FillRectangle(rt, &r, (ID2D1Brush *)brush);

    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to end draw, hr %#x.\n", hr);

    get_surface_readback(&ctx, &rb);
    for (i = 0; i < ARRAY_SIZE(test1); ++i)
    {
        DWORD colour;

        colour = get_readback_colour(&rb, test1[i].x, test1[i].y);
        ok(compare_colour(colour, test1[i].colour, 1),
                "Got unexpected colour 0x%08x at position {%u, %u}.\n",
                colour, test1[i].x, test1[i].y);
    }
    release_resource_readback(&rb);

    ID2D1RenderTarget_BeginDraw(rt);

    ID2D1RenderTarget_Clear(rt, &colour);

    set_matrix_identity(&matrix);
    skew_matrix(&matrix, 0.2146f, 1.575f);
    ID2D1RenderTarget_SetTransform(rt, &matrix);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 0.0f, 240.0f);
    scale_matrix(&matrix, 0.25f, -0.25f);
    ID2D1LinearGradientBrush_SetTransform(brush, &matrix);

    set_rect(&r, 0.0f, 0.0f, 80.0f, 240.0f);
    ID2D1RenderTarget_FillRectangle(rt, &r, (ID2D1Brush *)brush);

    set_matrix_identity(&matrix);
    scale_matrix(&matrix, 0.5f, 2.0f);
    translate_matrix(&matrix, 320.0f, 240.0f);
    rotate_matrix(&matrix, M_PI / 4.0f);
    ID2D1RenderTarget_SetTransform(rt, &matrix);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 0.0f, -50.0f);
    scale_matrix(&matrix, 0.1f, 0.1f);
    rotate_matrix(&matrix, -M_PI / 3.0f);
    ID2D1LinearGradientBrush_SetTransform(brush, &matrix);

    ID2D1LinearGradientBrush_SetOpacity(brush, 0.75f);
    set_rect(&r, -80.0f, -60.0f, 80.0f, 60.0f);
    ID2D1RenderTarget_FillRectangle(rt, &r, (ID2D1Brush *)brush);

    ID2D1RenderTarget_GetFactory(rt, &factory);

    set_rect(&r, -1.0f, -1.0f, 1.0f, 1.0f);
    hr = ID2D1Factory_CreateRectangleGeometry(factory, &r, &rectangle_geometry);
    ok(SUCCEEDED(hr), "Failed to create geometry, hr %#x.\n", hr);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 228.5f, 714.0f);
    scale_matrix(&matrix, 40.0f, 120.0f);
    hr = ID2D1Factory_CreateTransformedGeometry(factory, (ID2D1Geometry *)rectangle_geometry,
            &matrix, &transformed_geometry);
    ok(SUCCEEDED(hr), "Failed to create geometry, hr %#x.\n", hr);
    ID2D1RectangleGeometry_Release(rectangle_geometry);

    set_matrix_identity(&matrix);
    ID2D1RenderTarget_SetTransform(rt, &matrix);
    ID2D1LinearGradientBrush_SetTransform(brush, &matrix);
    set_point(&p, 188.5f, 834.0f);
    ID2D1LinearGradientBrush_SetStartPoint(brush, p);
    set_point(&p, 268.5f, 594.0f);
    ID2D1LinearGradientBrush_SetEndPoint(brush, p);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)transformed_geometry, (ID2D1Brush *)brush, NULL);
    ID2D1TransformedGeometry_Release(transformed_geometry);

    ID2D1Factory_Release(factory);

    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to end draw, hr %#x.\n", hr);

    get_surface_readback(&ctx, &rb);
    for (i = 0; i < ARRAY_SIZE(test2); ++i)
    {
        DWORD colour;

        colour = get_readback_colour(&rb, test2[i].x, test2[i].y);
        ok(compare_colour(colour, test2[i].colour, 1),
                "Got unexpected colour 0x%08x at position {%u, %u}.\n",
                colour, test2[i].x, test2[i].y);
    }
    release_resource_readback(&rb);

    ID2D1LinearGradientBrush_Release(brush);
    refcount = ID2D1GradientStopCollection_Release(gradient);
    ok(!refcount, "Gradient has %u references left.\n", refcount);
    release_test_context(&ctx);
}

static void test_radial_brush(BOOL d3d11)
{
    D2D1_RADIAL_GRADIENT_BRUSH_PROPERTIES gradient_properties;
    ID2D1GradientStopCollection *gradient, *tmp_gradient;
    ID2D1TransformedGeometry *transformed_geometry;
    ID2D1RectangleGeometry *rectangle_geometry;
    D2D1_MATRIX_3X2_F matrix, tmp_matrix;
    ID2D1RadialGradientBrush *brush;
    struct d2d1_test_context ctx;
    struct resource_readback rb;
    ID2D1RenderTarget *rt;
    ID2D1Factory *factory;
    D2D1_COLOR_F colour;
    D2D1_POINT_2F p;
    unsigned int i;
    ULONG refcount;
    D2D1_RECT_F r;
    HRESULT hr;
    float f;

    static const D2D1_GRADIENT_STOP stops[] =
    {
        {0.0f, {1.0f, 0.0f, 0.0f, 1.0f}},
        {0.5f, {0.0f, 1.0f, 0.0f, 1.0f}},
        {1.0f, {0.0f, 0.0f, 1.0f, 1.0f}},
    };
    static const struct
    {
        unsigned int x, y;
        DWORD colour;
    }
    test1[] =
    {
        {80,  80, 0xff0000ff}, {240,  80, 0xff00a857}, {400,  80, 0xff00d728}, {560,  80, 0xff0000ff},
        {80, 240, 0xff006699}, {240, 240, 0xff29d600}, {400, 240, 0xff966900}, {560, 240, 0xff00a55a},
        {80, 400, 0xff0000ff}, {240, 400, 0xff006e91}, {400, 400, 0xff007d82}, {560, 400, 0xff0000ff},
    },
    test2[] =
    {
        { 40,  30, 0xff000df2}, {120,  30, 0xffffffff}, { 40,  60, 0xffffffff}, { 80,  60, 0xff00b04f},
        {120,  60, 0xff007689}, {200,  60, 0xffffffff}, { 40,  90, 0xffffffff}, {120,  90, 0xff47b800},
        {160,  90, 0xff00c13e}, {200,  90, 0xffffffff}, { 80, 120, 0xffffffff}, {120, 120, 0xff0000ff},
        {160, 120, 0xff6f9000}, {200, 120, 0xff00718e}, {240, 120, 0xffffffff}, {160, 150, 0xffffffff},
        {200, 150, 0xff00609f}, {240, 150, 0xffffffff}, {280, 150, 0xffffffff}, {320, 150, 0xffffffff},
        {240, 180, 0xffffffff}, {280, 180, 0xff4040ff}, {320, 180, 0xff40b788}, {380, 180, 0xffffffff},
        {200, 210, 0xffffffff}, {240, 210, 0xff4040ff}, {280, 210, 0xff4040ff}, {320, 210, 0xff76c940},
        {360, 210, 0xff40cc73}, {400, 210, 0xffffffff}, {200, 240, 0xffffffff}, {280, 240, 0xff4061de},
        {320, 240, 0xff9fa040}, {360, 240, 0xff404af5}, {440, 240, 0xffffffff}, {240, 270, 0xffffffff},
        {280, 270, 0xff40aa95}, {320, 270, 0xff4ef140}, {360, 270, 0xff4040ff}, {440, 270, 0xffffffff},
        {280, 300, 0xffffffff}, {320, 300, 0xff4093ac}, {360, 300, 0xff4040ff}, {400, 300, 0xff4040ff},
        {440, 300, 0xff404af5}, {480, 300, 0xff4045fa}, {520, 300, 0xff4040ff}, {280, 330, 0xffffffff},
        {360, 330, 0xffffffff}, {400, 330, 0xff4069d6}, {440, 330, 0xff40c579}, {480, 330, 0xff40e956},
        {520, 330, 0xff4072cd}, {400, 360, 0xff408ab4}, {440, 360, 0xff49f540}, {480, 360, 0xffb98640},
        {520, 360, 0xff40dc62}, {400, 390, 0xff405ee1}, {440, 390, 0xff40d56a}, {480, 390, 0xff62dd40},
        {520, 390, 0xff4059e6},
    };

    if (!init_test_context(&ctx, d3d11))
        return;

    rt = ctx.rt;
    ID2D1RenderTarget_SetDpi(rt, 192.0f, 48.0f);
    ID2D1RenderTarget_SetAntialiasMode(rt, D2D1_ANTIALIAS_MODE_ALIASED);

    hr = ID2D1RenderTarget_CreateGradientStopCollection(rt, stops, ARRAY_SIZE(stops),
            D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, &gradient);
    ok(SUCCEEDED(hr), "Failed to create stop collection, hr %#x.\n", hr);

    set_point(&gradient_properties.center, 160.0f, 480.0f);
    set_point(&gradient_properties.gradientOriginOffset, 40.0f, -120.0f);
    gradient_properties.radiusX = 160.0f;
    gradient_properties.radiusY = 480.0f;
    hr = ID2D1RenderTarget_CreateRadialGradientBrush(rt, &gradient_properties, NULL, gradient, &brush);
    ok(SUCCEEDED(hr), "Failed to create brush, hr %#x.\n", hr);

    f = ID2D1RadialGradientBrush_GetOpacity(brush);
    ok(f == 1.0f, "Got unexpected opacity %.8e.\n", f);
    set_matrix_identity(&matrix);
    ID2D1RadialGradientBrush_GetTransform(brush, &tmp_matrix);
    ok(!memcmp(&tmp_matrix, &matrix, sizeof(matrix)),
            "Got unexpected matrix {%.8e, %.8e, %.8e, %.8e, %.8e, %.8e}.\n",
            tmp_matrix._11, tmp_matrix._12, tmp_matrix._21,
            tmp_matrix._22, tmp_matrix._31, tmp_matrix._32);
    p = ID2D1RadialGradientBrush_GetCenter(brush);
    ok(compare_point(&p, 160.0f, 480.0f, 0), "Got unexpected center {%.8e, %.8e}.\n", p.x, p.y);
    p = ID2D1RadialGradientBrush_GetGradientOriginOffset(brush);
    ok(compare_point(&p, 40.0f, -120.0f, 0), "Got unexpected origin offset {%.8e, %.8e}.\n", p.x, p.y);
    f = ID2D1RadialGradientBrush_GetRadiusX(brush);
    ok(compare_float(f, 160.0f, 0), "Got unexpected x-radius %.8e.\n", f);
    f = ID2D1RadialGradientBrush_GetRadiusY(brush);
    ok(compare_float(f, 480.0f, 0), "Got unexpected y-radius %.8e.\n", f);
    ID2D1RadialGradientBrush_GetGradientStopCollection(brush, &tmp_gradient);
    ok(tmp_gradient == gradient, "Got unexpected gradient %p, expected %p.\n", tmp_gradient, gradient);
    ID2D1GradientStopCollection_Release(tmp_gradient);

    ID2D1RenderTarget_BeginDraw(rt);

    set_color(&colour, 1.0f, 1.0f, 1.0f, 1.0f);
    ID2D1RenderTarget_Clear(rt, &colour);

    set_rect(&r, 0.0f, 0.0f, 320.0f, 960.0f);
    ID2D1RenderTarget_FillRectangle(rt, &r, (ID2D1Brush *)brush);

    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to end draw, hr %#x.\n", hr);

    get_surface_readback(&ctx, &rb);
    for (i = 0; i < ARRAY_SIZE(test1); ++i)
    {
        DWORD colour;

        colour = get_readback_colour(&rb, test1[i].x, test1[i].y);
        ok(compare_colour(colour, test1[i].colour, 1),
                "Got unexpected colour 0x%08x at position {%u, %u}.\n",
                colour, test1[i].x, test1[i].y);
    }
    release_resource_readback(&rb);

    ID2D1RenderTarget_BeginDraw(rt);

    ID2D1RenderTarget_Clear(rt, &colour);

    set_matrix_identity(&matrix);
    skew_matrix(&matrix, 0.2146f, 1.575f);
    ID2D1RenderTarget_SetTransform(rt, &matrix);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 0.0f, 240.0f);
    scale_matrix(&matrix, 0.25f, -0.25f);
    ID2D1RadialGradientBrush_SetTransform(brush, &matrix);

    set_rect(&r, 0.0f, 0.0f, 80.0f, 240.0f);
    ID2D1RenderTarget_FillRectangle(rt, &r, (ID2D1Brush *)brush);

    set_matrix_identity(&matrix);
    scale_matrix(&matrix, 0.5f, 2.0f);
    translate_matrix(&matrix, 320.0f, 240.0f);
    rotate_matrix(&matrix, M_PI / 4.0f);
    ID2D1RenderTarget_SetTransform(rt, &matrix);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, -75.0f, -50.0f);
    scale_matrix(&matrix, 0.15f, 0.5f);
    rotate_matrix(&matrix, -M_PI / 3.0f);
    ID2D1RadialGradientBrush_SetTransform(brush, &matrix);

    ID2D1RadialGradientBrush_SetOpacity(brush, 0.75f);
    set_rect(&r, -80.0f, -60.0f, 80.0f, 60.0f);
    ID2D1RenderTarget_FillRectangle(rt, &r, (ID2D1Brush *)brush);

    ID2D1RenderTarget_GetFactory(rt, &factory);

    set_rect(&r, -1.0f, -1.0f, 1.0f, 1.0f);
    hr = ID2D1Factory_CreateRectangleGeometry(factory, &r, &rectangle_geometry);
    ok(SUCCEEDED(hr), "Failed to create geometry, hr %#x.\n", hr);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 228.5f, 714.0f);
    scale_matrix(&matrix, 40.0f, 120.0f);
    hr = ID2D1Factory_CreateTransformedGeometry(factory, (ID2D1Geometry *)rectangle_geometry,
            &matrix, &transformed_geometry);
    ok(SUCCEEDED(hr), "Failed to create geometry, hr %#x.\n", hr);
    ID2D1RectangleGeometry_Release(rectangle_geometry);

    set_matrix_identity(&matrix);
    ID2D1RenderTarget_SetTransform(rt, &matrix);
    ID2D1RadialGradientBrush_SetTransform(brush, &matrix);
    set_point(&p, 228.5f, 714.0f);
    ID2D1RadialGradientBrush_SetCenter(brush, p);
    ID2D1RadialGradientBrush_SetRadiusX(brush, -40.0f);
    ID2D1RadialGradientBrush_SetRadiusY(brush, 120.0f);
    set_point(&p, 20.0f, 30.0f);
    ID2D1RadialGradientBrush_SetGradientOriginOffset(brush, p);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)transformed_geometry, (ID2D1Brush *)brush, NULL);
    ID2D1TransformedGeometry_Release(transformed_geometry);

    ID2D1Factory_Release(factory);

    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to end draw, hr %#x.\n", hr);

    get_surface_readback(&ctx, &rb);
    for (i = 0; i < ARRAY_SIZE(test2); ++i)
    {
        DWORD colour;

        colour = get_readback_colour(&rb, test2[i].x, test2[i].y);
        ok(compare_colour(colour, test2[i].colour, 1),
                "Got unexpected colour 0x%08x at position {%u, %u}.\n",
                colour, test2[i].x, test2[i].y);
    }
    release_resource_readback(&rb);

    ID2D1RadialGradientBrush_Release(brush);
    refcount = ID2D1GradientStopCollection_Release(gradient);
    ok(!refcount, "Gradient has %u references left.\n", refcount);
    release_test_context(&ctx);
}

static void fill_geometry_sink(ID2D1GeometrySink *sink, unsigned int hollow_count)
{
    D2D1_FIGURE_BEGIN begin;
    unsigned int idx = 0;
    D2D1_POINT_2F point;

    set_point(&point, 15.0f,  20.0f);
    begin = idx++ < hollow_count ? D2D1_FIGURE_BEGIN_HOLLOW : D2D1_FIGURE_BEGIN_FILLED;
    ID2D1GeometrySink_BeginFigure(sink, point, begin);
    line_to(sink, 55.0f,  20.0f);
    line_to(sink, 55.0f, 220.0f);
    line_to(sink, 25.0f, 220.0f);
    line_to(sink, 25.0f, 100.0f);
    line_to(sink, 75.0f, 100.0f);
    line_to(sink, 75.0f, 300.0f);
    line_to(sink,  5.0f, 300.0f);
    line_to(sink,  5.0f,  60.0f);
    line_to(sink, 45.0f,  60.0f);
    line_to(sink, 45.0f, 180.0f);
    line_to(sink, 35.0f, 180.0f);
    line_to(sink, 35.0f, 140.0f);
    line_to(sink, 65.0f, 140.0f);
    line_to(sink, 65.0f, 260.0f);
    line_to(sink, 15.0f, 260.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 155.0f, 300.0f);
    begin = idx++ < hollow_count ? D2D1_FIGURE_BEGIN_HOLLOW : D2D1_FIGURE_BEGIN_FILLED;
    ID2D1GeometrySink_BeginFigure(sink, point, begin);
    line_to(sink, 155.0f, 160.0f);
    line_to(sink,  85.0f, 160.0f);
    line_to(sink,  85.0f, 300.0f);
    line_to(sink, 120.0f, 300.0f);
    line_to(sink, 120.0f,  20.0f);
    line_to(sink, 155.0f,  20.0f);
    line_to(sink, 155.0f, 160.0f);
    line_to(sink,  85.0f, 160.0f);
    line_to(sink,  85.0f,  20.0f);
    line_to(sink, 120.0f,  20.0f);
    line_to(sink, 120.0f, 300.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 165.0f,  20.0f);
    begin = idx++ < hollow_count ? D2D1_FIGURE_BEGIN_HOLLOW : D2D1_FIGURE_BEGIN_FILLED;
    ID2D1GeometrySink_BeginFigure(sink, point, begin);
    line_to(sink, 165.0f, 300.0f);
    line_to(sink, 235.0f, 300.0f);
    line_to(sink, 235.0f,  20.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);
    set_point(&point, 225.0f,  60.0f);
    begin = idx++ < hollow_count ? D2D1_FIGURE_BEGIN_HOLLOW : D2D1_FIGURE_BEGIN_FILLED;
    ID2D1GeometrySink_BeginFigure(sink, point, begin);
    line_to(sink, 225.0f, 260.0f);
    line_to(sink, 175.0f, 260.0f);
    line_to(sink, 175.0f,  60.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);
    set_point(&point, 215.0f, 220.0f);
    begin = idx++ < hollow_count ? D2D1_FIGURE_BEGIN_HOLLOW : D2D1_FIGURE_BEGIN_FILLED;
    ID2D1GeometrySink_BeginFigure(sink, point, begin);
    line_to(sink, 185.0f, 220.0f);
    line_to(sink, 185.0f, 100.0f);
    line_to(sink, 215.0f, 100.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);
    set_point(&point, 195.0f, 180.0f);
    begin = idx++ < hollow_count ? D2D1_FIGURE_BEGIN_HOLLOW : D2D1_FIGURE_BEGIN_FILLED;
    ID2D1GeometrySink_BeginFigure(sink, point, begin);
    line_to(sink, 205.0f, 180.0f);
    line_to(sink, 205.0f, 140.0f);
    line_to(sink, 195.0f, 140.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);
}

static void fill_geometry_sink_bezier(ID2D1GeometrySink *sink, unsigned int hollow_count)
{
    D2D1_FIGURE_BEGIN begin;
    unsigned int idx = 0;
    D2D1_POINT_2F point;

    set_point(&point, 5.0f, 160.0f);
    begin = idx++ < hollow_count ? D2D1_FIGURE_BEGIN_HOLLOW : D2D1_FIGURE_BEGIN_FILLED;
    ID2D1GeometrySink_BeginFigure(sink, point, begin);
    quadratic_to(sink, 40.0f, 160.0f, 40.0f,  20.0f);
    quadratic_to(sink, 40.0f, 160.0f, 75.0f, 160.0f);
    quadratic_to(sink, 40.0f, 160.0f, 40.0f, 300.0f);
    quadratic_to(sink, 40.0f, 160.0f,  5.0f, 160.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 20.0f, 160.0f);
    begin = idx++ < hollow_count ? D2D1_FIGURE_BEGIN_HOLLOW : D2D1_FIGURE_BEGIN_FILLED;
    ID2D1GeometrySink_BeginFigure(sink, point, begin);
    quadratic_to(sink, 20.0f,  80.0f, 40.0f,  80.0f);
    quadratic_to(sink, 60.0f,  80.0f, 60.0f, 160.0f);
    quadratic_to(sink, 60.0f, 240.0f, 40.0f, 240.0f);
    quadratic_to(sink, 20.0f, 240.0f, 20.0f, 160.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 5.0f, 612.0f);
    begin = idx++ < hollow_count ? D2D1_FIGURE_BEGIN_HOLLOW : D2D1_FIGURE_BEGIN_FILLED;
    ID2D1GeometrySink_BeginFigure(sink, point, begin);
    quadratic_to(sink, 40.0f, 612.0f, 40.0f, 752.0f);
    quadratic_to(sink, 40.0f, 612.0f, 75.0f, 612.0f);
    quadratic_to(sink, 40.0f, 612.0f, 40.0f, 472.0f);
    quadratic_to(sink, 40.0f, 612.0f,  5.0f, 612.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 20.0f, 612.0f);
    begin = idx++ < hollow_count ? D2D1_FIGURE_BEGIN_HOLLOW : D2D1_FIGURE_BEGIN_FILLED;
    ID2D1GeometrySink_BeginFigure(sink, point, begin);
    quadratic_to(sink, 20.0f, 692.0f, 40.0f, 692.0f);
    quadratic_to(sink, 60.0f, 692.0f, 60.0f, 612.0f);
    quadratic_to(sink, 60.0f, 532.0f, 40.0f, 532.0f);
    quadratic_to(sink, 20.0f, 532.0f, 20.0f, 612.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);
}

static void test_path_geometry(BOOL d3d11)
{
    ID2D1TransformedGeometry *transformed_geometry;
    D2D1_MATRIX_3X2_F matrix, tmp_matrix;
    ID2D1GeometrySink *sink, *tmp_sink;
    struct geometry_sink simplify_sink;
    D2D1_POINT_2F point = {0.0f, 0.0f};
    struct d2d1_test_context ctx;
    ID2D1SolidColorBrush *brush;
    ID2D1PathGeometry *geometry;
    ID2D1Geometry *tmp_geometry;
    ID2D1RenderTarget *rt;
    ID2D1Factory *factory;
    BOOL match, contains;
    D2D1_COLOR_F color;
    D2D1_RECT_F rect;
    UINT32 count;
    HRESULT hr;

    static const struct geometry_segment expected_segments[] =
    {
        /* Figure 0. */
        {SEGMENT_LINE,   {{{ 55.0f,  20.0f}}}},
        {SEGMENT_LINE,   {{{ 55.0f, 220.0f}}}},
        {SEGMENT_LINE,   {{{ 25.0f, 220.0f}}}},
        {SEGMENT_LINE,   {{{ 25.0f, 100.0f}}}},
        {SEGMENT_LINE,   {{{ 75.0f, 100.0f}}}},
        {SEGMENT_LINE,   {{{ 75.0f, 300.0f}}}},
        {SEGMENT_LINE,   {{{  5.0f, 300.0f}}}},
        {SEGMENT_LINE,   {{{  5.0f,  60.0f}}}},
        {SEGMENT_LINE,   {{{ 45.0f,  60.0f}}}},
        {SEGMENT_LINE,   {{{ 45.0f, 180.0f}}}},
        {SEGMENT_LINE,   {{{ 35.0f, 180.0f}}}},
        {SEGMENT_LINE,   {{{ 35.0f, 140.0f}}}},
        {SEGMENT_LINE,   {{{ 65.0f, 140.0f}}}},
        {SEGMENT_LINE,   {{{ 65.0f, 260.0f}}}},
        {SEGMENT_LINE,   {{{ 15.0f, 260.0f}}}},
        /* Figure 1. */
        {SEGMENT_LINE,   {{{155.0f, 160.0f}}}},
        {SEGMENT_LINE,   {{{ 85.0f, 160.0f}}}},
        {SEGMENT_LINE,   {{{ 85.0f, 300.0f}}}},
        {SEGMENT_LINE,   {{{120.0f, 300.0f}}}},
        {SEGMENT_LINE,   {{{120.0f,  20.0f}}}},
        {SEGMENT_LINE,   {{{155.0f,  20.0f}}}},
        {SEGMENT_LINE,   {{{155.0f, 160.0f}}}},
        {SEGMENT_LINE,   {{{ 85.0f, 160.0f}}}},
        {SEGMENT_LINE,   {{{ 85.0f,  20.0f}}}},
        {SEGMENT_LINE,   {{{120.0f,  20.0f}}}},
        {SEGMENT_LINE,   {{{120.0f, 300.0f}}}},
        /* Figure 2. */
        {SEGMENT_LINE,   {{{165.0f, 300.0f}}}},
        {SEGMENT_LINE,   {{{235.0f, 300.0f}}}},
        {SEGMENT_LINE,   {{{235.0f,  20.0f}}}},
        /* Figure 3. */
        {SEGMENT_LINE,   {{{225.0f, 260.0f}}}},
        {SEGMENT_LINE,   {{{175.0f, 260.0f}}}},
        {SEGMENT_LINE,   {{{175.0f,  60.0f}}}},
        /* Figure 4. */
        {SEGMENT_LINE,   {{{185.0f, 220.0f}}}},
        {SEGMENT_LINE,   {{{185.0f, 100.0f}}}},
        {SEGMENT_LINE,   {{{215.0f, 100.0f}}}},
        /* Figure 5. */
        {SEGMENT_LINE,   {{{205.0f, 180.0f}}}},
        {SEGMENT_LINE,   {{{205.0f, 140.0f}}}},
        {SEGMENT_LINE,   {{{195.0f, 140.0f}}}},
        /* Figure 6. */
        {SEGMENT_LINE,   {{{135.0f, 620.0f}}}},
        {SEGMENT_LINE,   {{{135.0f, 420.0f}}}},
        {SEGMENT_LINE,   {{{105.0f, 420.0f}}}},
        {SEGMENT_LINE,   {{{105.0f, 540.0f}}}},
        {SEGMENT_LINE,   {{{155.0f, 540.0f}}}},
        {SEGMENT_LINE,   {{{155.0f, 340.0f}}}},
        {SEGMENT_LINE,   {{{ 85.0f, 340.0f}}}},
        {SEGMENT_LINE,   {{{ 85.0f, 580.0f}}}},
        {SEGMENT_LINE,   {{{125.0f, 580.0f}}}},
        {SEGMENT_LINE,   {{{125.0f, 460.0f}}}},
        {SEGMENT_LINE,   {{{115.0f, 460.0f}}}},
        {SEGMENT_LINE,   {{{115.0f, 500.0f}}}},
        {SEGMENT_LINE,   {{{145.0f, 500.0f}}}},
        {SEGMENT_LINE,   {{{145.0f, 380.0f}}}},
        {SEGMENT_LINE,   {{{ 95.0f, 380.0f}}}},
        /* Figure 7. */
        {SEGMENT_LINE,   {{{235.0f, 480.0f}}}},
        {SEGMENT_LINE,   {{{165.0f, 480.0f}}}},
        {SEGMENT_LINE,   {{{165.0f, 340.0f}}}},
        {SEGMENT_LINE,   {{{200.0f, 340.0f}}}},
        {SEGMENT_LINE,   {{{200.0f, 620.0f}}}},
        {SEGMENT_LINE,   {{{235.0f, 620.0f}}}},
        {SEGMENT_LINE,   {{{235.0f, 480.0f}}}},
        {SEGMENT_LINE,   {{{165.0f, 480.0f}}}},
        {SEGMENT_LINE,   {{{165.0f, 620.0f}}}},
        {SEGMENT_LINE,   {{{200.0f, 620.0f}}}},
        {SEGMENT_LINE,   {{{200.0f, 340.0f}}}},
        /* Figure 8. */
        {SEGMENT_LINE,   {{{245.0f, 340.0f}}}},
        {SEGMENT_LINE,   {{{315.0f, 340.0f}}}},
        {SEGMENT_LINE,   {{{315.0f, 620.0f}}}},
        /* Figure 9. */
        {SEGMENT_LINE,   {{{305.0f, 380.0f}}}},
        {SEGMENT_LINE,   {{{255.0f, 380.0f}}}},
        {SEGMENT_LINE,   {{{255.0f, 580.0f}}}},
        /* Figure 10. */
        {SEGMENT_LINE,   {{{265.0f, 420.0f}}}},
        {SEGMENT_LINE,   {{{265.0f, 540.0f}}}},
        {SEGMENT_LINE,   {{{295.0f, 540.0f}}}},
        /* Figure 11. */
        {SEGMENT_LINE,   {{{285.0f, 460.0f}}}},
        {SEGMENT_LINE,   {{{285.0f, 500.0f}}}},
        {SEGMENT_LINE,   {{{275.0f, 500.0f}}}},
        /* Figure 12. */
        {SEGMENT_BEZIER, {{{2.83333340e+01f, 1.60000000e+02f},
                           {4.00000000e+01f, 1.13333336e+02f},
                           {4.00000000e+01f, 2.00000000e+01f}}}},
        {SEGMENT_BEZIER, {{{4.00000000e+01f, 1.13333336e+02f},
                           {5.16666641e+01f, 1.60000000e+02f},
                           {7.50000000e+01f, 1.60000000e+02f}}}},
        {SEGMENT_BEZIER, {{{5.16666641e+01f, 1.60000000e+02f},
                           {4.00000000e+01f, 2.06666656e+02f},
                           {4.00000000e+01f, 3.00000000e+02f}}}},
        {SEGMENT_BEZIER, {{{4.00000000e+01f, 2.06666656e+02f},
                           {2.83333340e+01f, 1.60000000e+02f},
                           {5.00000000e+00f, 1.60000000e+02f}}}},
        /* Figure 13. */
        {SEGMENT_BEZIER, {{{2.00000000e+01f, 1.06666664e+02f},
                           {2.66666660e+01f, 8.00000000e+01f},
                           {4.00000000e+01f, 8.00000000e+01f}}}},
        {SEGMENT_BEZIER, {{{5.33333321e+01f, 8.00000000e+01f},
                           {6.00000000e+01f, 1.06666664e+02f},
                           {6.00000000e+01f, 1.60000000e+02f}}}},
        {SEGMENT_BEZIER, {{{6.00000000e+01f, 2.13333328e+02f},
                           {5.33333321e+01f, 2.40000000e+02f},
                           {4.00000000e+01f, 2.40000000e+02f}}}},
        {SEGMENT_BEZIER, {{{2.66666660e+01f, 2.40000000e+02f},
                           {2.00000000e+01f, 2.13333328e+02f},
                           {2.00000000e+01f, 1.60000000e+02f}}}},
        /* Figure 14. */
        {SEGMENT_BEZIER, {{{2.83333340e+01f, 6.12000000e+02f},
                           {4.00000000e+01f, 6.58666687e+02f},
                           {4.00000000e+01f, 7.52000000e+02f}}}},
        {SEGMENT_BEZIER, {{{4.00000000e+01f, 6.58666687e+02f},
                           {5.16666641e+01f, 6.12000000e+02f},
                           {7.50000000e+01f, 6.12000000e+02f}}}},
        {SEGMENT_BEZIER, {{{5.16666641e+01f, 6.12000000e+02f},
                           {4.00000000e+01f, 5.65333313e+02f},
                           {4.00000000e+01f, 4.72000000e+02f}}}},
        {SEGMENT_BEZIER, {{{4.00000000e+01f, 5.65333313e+02f},
                           {2.83333340e+01f, 6.12000000e+02f},
                           {5.00000000e+00f, 6.12000000e+02f}}}},
        /* Figure 15. */
        {SEGMENT_BEZIER, {{{2.00000000e+01f, 6.65333313e+02f},
                           {2.66666660e+01f, 6.92000000e+02f},
                           {4.00000000e+01f, 6.92000000e+02f}}}},
        {SEGMENT_BEZIER, {{{5.33333321e+01f, 6.92000000e+02f},
                           {6.00000000e+01f, 6.65333313e+02f},
                           {6.00000000e+01f, 6.12000000e+02f}}}},
        {SEGMENT_BEZIER, {{{6.00000000e+01f, 5.58666687e+02f},
                           {5.33333321e+01f, 5.32000000e+02f},
                           {4.00000000e+01f, 5.32000000e+02f}}}},
        {SEGMENT_BEZIER, {{{2.66666660e+01f, 5.32000000e+02f},
                           {2.00000000e+01f, 5.58666687e+02f},
                           {2.00000000e+01f, 6.12000000e+02f}}}},
        /* Figure 16. */
        {SEGMENT_BEZIER, {{{1.91750427e+02f, 1.27275856e+02f},
                           {2.08249573e+02f, 1.27275856e+02f},
                           {2.24748734e+02f, 6.12792168e+01f}}}},
        {SEGMENT_BEZIER, {{{2.08249573e+02f, 1.27275856e+02f},
                           {2.08249573e+02f, 1.93272476e+02f},
                           {2.24748734e+02f, 2.59269104e+02f}}}},
        {SEGMENT_BEZIER, {{{2.08249573e+02f, 1.93272476e+02f},
                           {1.91750427e+02f, 1.93272476e+02f},
                           {1.75251266e+02f, 2.59269104e+02f}}}},
        {SEGMENT_BEZIER, {{{1.91750427e+02f, 1.93272476e+02f},
                           {1.91750427e+02f, 1.27275856e+02f},
                           {1.75251266e+02f, 6.12792168e+01f}}}},
        /* Figure 17. */
        {SEGMENT_BEZIER, {{{1.95285950e+02f, 6.59932632e+01f},
                           {2.04714050e+02f, 6.59932632e+01f},
                           {2.14142136e+02f, 1.03705627e+02f}}}},
        {SEGMENT_BEZIER, {{{2.23570221e+02f, 1.41417984e+02f},
                           {2.23570221e+02f, 1.79130356e+02f},
                           {2.14142136e+02f, 2.16842712e+02f}}}},
        {SEGMENT_BEZIER, {{{2.04714050e+02f, 2.54555069e+02f},
                           {1.95285950e+02f, 2.54555069e+02f},
                           {1.85857864e+02f, 2.16842712e+02f}}}},
        {SEGMENT_BEZIER, {{{1.76429779e+02f, 1.79130356e+02f},
                           {1.76429779e+02f, 1.41417984e+02f},
                           {1.85857864e+02f, 1.03705627e+02f}}}},
        /* Figure 18. */
        {SEGMENT_BEZIER, {{{1.11847351e+02f, 4.46888092e+02f},
                           {1.11847351e+02f, 5.12884705e+02f},
                           {9.53481979e+01f, 5.78881348e+02f}}}},
        {SEGMENT_BEZIER, {{{1.11847351e+02f, 5.12884705e+02f},
                           {1.28346512e+02f, 5.12884705e+02f},
                           {1.44845673e+02f, 5.78881348e+02f}}}},
        {SEGMENT_BEZIER, {{{1.28346512e+02f, 5.12884705e+02f},
                           {1.28346512e+02f, 4.46888092e+02f},
                           {1.44845673e+02f, 3.80891479e+02f}}}},
        {SEGMENT_BEZIER, {{{1.28346512e+02f, 4.46888092e+02f},
                           {1.11847351e+02f, 4.46888092e+02f},
                           {9.53481979e+01f, 3.80891479e+02f}}}},
        /* Figure 19. */
        {SEGMENT_BEZIER, {{{9.65267105e+01f, 4.61030243e+02f},
                           {9.65267105e+01f, 4.98742584e+02f},
                           {1.05954803e+02f, 5.36454956e+02f}}}},
        {SEGMENT_BEZIER, {{{1.15382889e+02f, 5.74167297e+02f},
                           {1.24810982e+02f, 5.74167297e+02f},
                           {1.34239075e+02f, 5.36454956e+02f}}}},
        {SEGMENT_BEZIER, {{{1.43667160e+02f, 4.98742584e+02f},
                           {1.43667160e+02f, 4.61030243e+02f},
                           {1.34239075e+02f, 4.23317871e+02f}}}},
        {SEGMENT_BEZIER, {{{1.24810982e+02f, 3.85605499e+02f},
                           {1.15382889e+02f, 3.85605499e+02f},
                           {1.05954803e+02f, 4.23317871e+02f}}}},
        /* Figure 20. */
        {SEGMENT_LINE,   {{{ 40.0f,  20.0f}}}},
        {SEGMENT_LINE,   {{{ 75.0f, 160.0f}}}},
        {SEGMENT_LINE,   {{{ 40.0f, 300.0f}}}},
        {SEGMENT_LINE,   {{{  5.0f, 160.0f}}}},
        /* Figure 21. */
        {SEGMENT_LINE,   {{{ 40.0f,  80.0f}}}},
        {SEGMENT_LINE,   {{{ 60.0f, 160.0f}}}},
        {SEGMENT_LINE,   {{{ 40.0f, 240.0f}}}},
        {SEGMENT_LINE,   {{{ 20.0f, 160.0f}}}},
        /* Figure 22. */
        {SEGMENT_LINE,   {{{ 40.0f, 752.0f}}}},
        {SEGMENT_LINE,   {{{ 75.0f, 612.0f}}}},
        {SEGMENT_LINE,   {{{ 40.0f, 472.0f}}}},
        {SEGMENT_LINE,   {{{  5.0f, 612.0f}}}},
        /* Figure 23. */
        {SEGMENT_LINE,   {{{ 40.0f, 692.0f}}}},
        {SEGMENT_LINE,   {{{ 60.0f, 612.0f}}}},
        {SEGMENT_LINE,   {{{ 40.0f, 532.0f}}}},
        {SEGMENT_LINE,   {{{ 20.0f, 612.0f}}}},
        /* Figure 24. */
        {SEGMENT_LINE,   {{{2.03125019e+01f, 1.51250000e+02f}}}},
        {SEGMENT_LINE,   {{{3.12500019e+01f, 1.25000008e+02f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        {SEGMENT_LINE,   {{{3.78125000e+01f, 8.12500076e+01f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        {SEGMENT_LINE,   {{{4.00000000e+01f, 2.00000000e+01f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        {SEGMENT_LINE,   {{{4.21875000e+01f, 8.12500076e+01f}}}},
        {SEGMENT_LINE,   {{{4.87500000e+01f, 1.25000008e+02f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        {SEGMENT_LINE,   {{{5.96875000e+01f, 1.51250000e+02f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        {SEGMENT_LINE,   {{{7.50000000e+01f, 1.60000000e+02f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        {SEGMENT_LINE,   {{{5.96875000e+01f, 1.68750000e+02f}}}},
        {SEGMENT_LINE,   {{{4.87500000e+01f, 1.95000000e+02f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        {SEGMENT_LINE,   {{{4.21875000e+01f, 2.38750000e+02f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        {SEGMENT_LINE,   {{{4.00000000e+01f, 3.00000000e+02f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        {SEGMENT_LINE,   {{{3.78125000e+01f, 2.38750000e+02f}}}},
        {SEGMENT_LINE,   {{{3.12500019e+01f, 1.95000000e+02f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        {SEGMENT_LINE,   {{{2.03125019e+01f, 1.68750000e+02f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        {SEGMENT_LINE,   {{{5.00000000e+00f, 1.60000000e+02f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        /* Figure 25. */
        {SEGMENT_LINE,   {{{2.50000000e+01f, 1.00000000e+02f}}}},
        {SEGMENT_LINE,   {{{4.00000000e+01f, 8.00000000e+01f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        {SEGMENT_LINE,   {{{5.50000000e+01f, 1.00000000e+02f}}}},
        {SEGMENT_LINE,   {{{6.00000000e+01f, 1.60000000e+02f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        {SEGMENT_LINE,   {{{5.50000000e+01f, 2.20000000e+02f}}}},
        {SEGMENT_LINE,   {{{4.00000000e+01f, 2.40000000e+02f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        {SEGMENT_LINE,   {{{2.50000000e+01f, 2.20000000e+02f}}}},
        {SEGMENT_LINE,   {{{2.00000000e+01f, 1.60000000e+02f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        /* Figure 26. */
        {SEGMENT_LINE,   {{{2.03125019e+01f, 6.20750000e+02f}}}},
        {SEGMENT_LINE,   {{{3.12500019e+01f, 6.47000000e+02f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        {SEGMENT_LINE,   {{{3.78125000e+01f, 6.90750000e+02f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        {SEGMENT_LINE,   {{{4.00000000e+01f, 7.52000000e+02f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        {SEGMENT_LINE,   {{{4.21875000e+01f, 6.90750000e+02f}}}},
        {SEGMENT_LINE,   {{{4.87500000e+01f, 6.47000000e+02f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        {SEGMENT_LINE,   {{{5.96875000e+01f, 6.20750000e+02f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        {SEGMENT_LINE,   {{{7.50000000e+01f, 6.12000000e+02f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        {SEGMENT_LINE,   {{{5.96875000e+01f, 6.03250000e+02f}}}},
        {SEGMENT_LINE,   {{{4.87500000e+01f, 5.77000000e+02f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        {SEGMENT_LINE,   {{{4.21875000e+01f, 5.33250000e+02f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        {SEGMENT_LINE,   {{{4.00000000e+01f, 4.72000000e+02f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        {SEGMENT_LINE,   {{{3.78125000e+01f, 5.33250000e+02f}}}},
        {SEGMENT_LINE,   {{{3.12500019e+01f, 5.77000000e+02f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        {SEGMENT_LINE,   {{{2.03125019e+01f, 6.03250000e+02f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        {SEGMENT_LINE,   {{{5.00000000e+00f, 6.12000000e+02f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        /* Figure 27. */
        {SEGMENT_LINE,   {{{2.50000000e+01f, 6.72000000e+02f}}}},
        {SEGMENT_LINE,   {{{4.00000000e+01f, 6.92000000e+02f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        {SEGMENT_LINE,   {{{5.50000000e+01f, 6.72000000e+02f}}}},
        {SEGMENT_LINE,   {{{6.00000000e+01f, 6.12000000e+02f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        {SEGMENT_LINE,   {{{5.50000000e+01f, 5.52000000e+02f}}}},
        {SEGMENT_LINE,   {{{4.00000000e+01f, 5.32000000e+02f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        {SEGMENT_LINE,   {{{2.50000000e+01f, 5.52000000e+02f}}}},
        {SEGMENT_LINE,   {{{2.00000000e+01f, 6.12000000e+02f}}}, D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN},
        /* Figure 28. */
        {SEGMENT_LINE,   {{{ 75.0f, 300.0f}}}},
        {SEGMENT_LINE,   {{{  5.0f, 300.0f}}}},
    };
    static const struct expected_geometry_figure expected_figures[] =
    {
        /* 0 */
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, { 15.0f,  20.0f}, 15, &expected_segments[0]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, {155.0f, 300.0f}, 11, &expected_segments[15]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, {165.0f,  20.0f},  3, &expected_segments[26]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, {225.0f,  60.0f},  3, &expected_segments[29]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, {215.0f, 220.0f},  3, &expected_segments[32]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, {195.0f, 180.0f},  3, &expected_segments[35]},
        /* 6 */
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, { 95.0f, 620.0f}, 15, &expected_segments[38]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, {235.0f, 340.0f}, 11, &expected_segments[53]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, {245.0f, 620.0f},  3, &expected_segments[64]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, {305.0f, 580.0f},  3, &expected_segments[67]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, {295.0f, 420.0f},  3, &expected_segments[70]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, {275.0f, 460.0f},  3, &expected_segments[73]},
        /* 12 */
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, {  5.0f, 160.0f},  4, &expected_segments[76]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, { 20.0f, 160.0f},  4, &expected_segments[80]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, {  5.0f, 612.0f},  4, &expected_segments[84]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, { 20.0f, 612.0f},  4, &expected_segments[88]},
        /* 16 */
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED,
                {1.75251266e+02f, 6.12792168e+01f}, 4, &expected_segments[92]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED,
                {1.85857864e+02f, 1.03705627e+02f}, 4, &expected_segments[96]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED,
                {9.53481979e+01f, 3.80891479e+02f}, 4, &expected_segments[100]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED,
                {1.05954803e+02f, 4.23317871e+02f}, 4, &expected_segments[104]},
        /* 20 */
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, {  5.0f, 160.0f},  4, &expected_segments[108]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, { 20.0f, 160.0f},  4, &expected_segments[112]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, {  5.0f, 612.0f},  4, &expected_segments[116]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, { 20.0f, 612.0f},  4, &expected_segments[120]},
        /* 24 */
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, {  5.0f, 160.0f}, 16, &expected_segments[124]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, { 20.0f, 160.0f},  8, &expected_segments[140]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, {  5.0f, 612.0f}, 16, &expected_segments[148]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, { 20.0f, 612.0f},  8, &expected_segments[164]},
        /* 28 */
        {D2D1_FIGURE_BEGIN_HOLLOW, D2D1_FIGURE_END_OPEN,   { 40.0f,  20.0f},  2, &expected_segments[172]},
    };

    if (!init_test_context(&ctx, d3d11))
        return;

    rt = ctx.rt;
    ID2D1RenderTarget_GetFactory(rt, &factory);

    ID2D1RenderTarget_SetDpi(rt, 192.0f, 48.0f);
    ID2D1RenderTarget_SetAntialiasMode(rt, D2D1_ANTIALIAS_MODE_ALIASED);
    set_color(&color, 0.890f, 0.851f, 0.600f, 1.0f);
    hr = ID2D1RenderTarget_CreateSolidColorBrush(rt, &color, NULL, &brush);
    ok(SUCCEEDED(hr), "Failed to create brush, hr %#x.\n", hr);

    /* Close() when closed. */
    hr = ID2D1Factory_CreatePathGeometry(factory, &geometry);
    ok(SUCCEEDED(hr), "Failed to create path geometry, hr %#x.\n", hr);
    hr = ID2D1PathGeometry_GetFigureCount(geometry, &count);
    ok(hr == D2DERR_WRONG_STATE, "Got unexpected hr %#x.\n", hr);
    hr = ID2D1PathGeometry_GetSegmentCount(geometry, &count);
    ok(hr == D2DERR_WRONG_STATE, "Got unexpected hr %#x.\n", hr);
    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(SUCCEEDED(hr), "Failed to open geometry sink, hr %#x.\n", hr);
    hr = ID2D1PathGeometry_GetFigureCount(geometry, &count);
    ok(hr == D2DERR_WRONG_STATE, "Got unexpected hr %#x.\n", hr);
    hr = ID2D1PathGeometry_GetSegmentCount(geometry, &count);
    ok(hr == D2DERR_WRONG_STATE, "Got unexpected hr %#x.\n", hr);
    hr = ID2D1GeometrySink_Close(sink);
    ok(SUCCEEDED(hr), "Failed to close geometry sink, hr %#x.\n", hr);
    hr = ID2D1PathGeometry_GetFigureCount(geometry, &count);
    ok(SUCCEEDED(hr), "Failed to get figure count, hr %#x.\n", hr);
    ok(!count, "Got unexpected figure count %u.\n", count);
    hr = ID2D1PathGeometry_GetSegmentCount(geometry, &count);
    ok(SUCCEEDED(hr), "Failed to get segment count, hr %#x.\n", hr);
    ok(!count, "Got unexpected segment count %u.\n", count);
    hr = ID2D1GeometrySink_Close(sink);
    ok(hr == D2DERR_WRONG_STATE, "Got unexpected hr %#x.\n", hr);
    ID2D1GeometrySink_Release(sink);
    hr = ID2D1PathGeometry_GetFigureCount(geometry, &count);
    ok(SUCCEEDED(hr), "Failed to get figure count, hr %#x.\n", hr);
    ok(!count, "Got unexpected figure count %u.\n", count);
    hr = ID2D1PathGeometry_GetSegmentCount(geometry, &count);
    ok(SUCCEEDED(hr), "Failed to get segment count, hr %#x.\n", hr);
    ok(!count, "Got unexpected segment count %u.\n", count);
    ID2D1PathGeometry_Release(geometry);

    /* Open() when closed. */
    hr = ID2D1Factory_CreatePathGeometry(factory, &geometry);
    ok(SUCCEEDED(hr), "Failed to create path geometry, hr %#x.\n", hr);
    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(SUCCEEDED(hr), "Failed to open geometry sink, hr %#x.\n", hr);
    hr = ID2D1GeometrySink_Close(sink);
    ok(SUCCEEDED(hr), "Failed to close geometry sink, hr %#x.\n", hr);
    ID2D1GeometrySink_Release(sink);
    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(hr == D2DERR_WRONG_STATE, "Got unexpected hr %#x.\n", hr);
    hr = ID2D1PathGeometry_GetFigureCount(geometry, &count);
    ok(SUCCEEDED(hr), "Failed to get figure count, hr %#x.\n", hr);
    ok(!count, "Got unexpected figure count %u.\n", count);
    hr = ID2D1PathGeometry_GetSegmentCount(geometry, &count);
    ok(SUCCEEDED(hr), "Failed to get segment count, hr %#x.\n", hr);
    ok(!count, "Got unexpected segment count %u.\n", count);
    ID2D1PathGeometry_Release(geometry);

    /* Open() when open. */
    hr = ID2D1Factory_CreatePathGeometry(factory, &geometry);
    ok(SUCCEEDED(hr), "Failed to create path geometry, hr %#x.\n", hr);
    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(SUCCEEDED(hr), "Failed to open geometry sink, hr %#x.\n", hr);
    hr = ID2D1PathGeometry_Open(geometry, &tmp_sink);
    ok(hr == D2DERR_WRONG_STATE, "Got unexpected hr %#x.\n", hr);
    hr = ID2D1GeometrySink_Close(sink);
    ok(SUCCEEDED(hr), "Failed to close geometry sink, hr %#x.\n", hr);
    ID2D1GeometrySink_Release(sink);
    hr = ID2D1PathGeometry_GetFigureCount(geometry, &count);
    ok(SUCCEEDED(hr), "Failed to get figure count, hr %#x.\n", hr);
    ok(!count, "Got unexpected figure count %u.\n", count);
    hr = ID2D1PathGeometry_GetSegmentCount(geometry, &count);
    ok(SUCCEEDED(hr), "Failed to get segment count, hr %#x.\n", hr);
    ok(!count, "Got unexpected segment count %u.\n", count);
    ID2D1PathGeometry_Release(geometry);

    /* BeginFigure() without EndFigure(). */
    hr = ID2D1Factory_CreatePathGeometry(factory, &geometry);
    ok(SUCCEEDED(hr), "Failed to create path geometry, hr %#x.\n", hr);
    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(SUCCEEDED(hr), "Failed to open geometry sink, hr %#x.\n", hr);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    hr = ID2D1GeometrySink_Close(sink);
    ok(hr == D2DERR_WRONG_STATE, "Got unexpected hr %#x.\n", hr);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);
    hr = ID2D1GeometrySink_Close(sink);
    ok(hr == D2DERR_WRONG_STATE, "Got unexpected hr %#x.\n", hr);
    ID2D1GeometrySink_Release(sink);
    hr = ID2D1PathGeometry_GetFigureCount(geometry, &count);
    ok(hr == D2DERR_WRONG_STATE, "Got unexpected hr %#x.\n", hr);
    hr = ID2D1PathGeometry_GetSegmentCount(geometry, &count);
    ok(hr == D2DERR_WRONG_STATE, "Got unexpected hr %#x.\n", hr);
    ID2D1PathGeometry_Release(geometry);

    /* EndFigure() without BeginFigure(). */
    hr = ID2D1Factory_CreatePathGeometry(factory, &geometry);
    ok(SUCCEEDED(hr), "Failed to create path geometry, hr %#x.\n", hr);
    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(SUCCEEDED(hr), "Failed to open geometry sink, hr %#x.\n", hr);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);
    hr = ID2D1GeometrySink_Close(sink);
    ok(hr == D2DERR_WRONG_STATE, "Got unexpected hr %#x.\n", hr);
    ID2D1GeometrySink_Release(sink);
    hr = ID2D1PathGeometry_GetFigureCount(geometry, &count);
    ok(hr == D2DERR_WRONG_STATE, "Got unexpected hr %#x.\n", hr);
    hr = ID2D1PathGeometry_GetSegmentCount(geometry, &count);
    ok(hr == D2DERR_WRONG_STATE, "Got unexpected hr %#x.\n", hr);
    ID2D1PathGeometry_Release(geometry);

    /* BeginFigure()/EndFigure() mismatch. */
    hr = ID2D1Factory_CreatePathGeometry(factory, &geometry);
    ok(SUCCEEDED(hr), "Failed to create path geometry, hr %#x.\n", hr);
    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(SUCCEEDED(hr), "Failed to open geometry sink, hr %#x.\n", hr);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);
    hr = ID2D1GeometrySink_Close(sink);
    ok(hr == D2DERR_WRONG_STATE, "Got unexpected hr %#x.\n", hr);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);
    hr = ID2D1GeometrySink_Close(sink);
    ok(hr == D2DERR_WRONG_STATE, "Got unexpected hr %#x.\n", hr);
    ID2D1GeometrySink_Release(sink);
    ID2D1PathGeometry_Release(geometry);

    /* AddLine() outside BeginFigure()/EndFigure(). */
    hr = ID2D1Factory_CreatePathGeometry(factory, &geometry);
    ok(SUCCEEDED(hr), "Failed to create path geometry, hr %#x.\n", hr);
    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(SUCCEEDED(hr), "Failed to open geometry sink, hr %#x.\n", hr);
    ID2D1GeometrySink_AddLine(sink, point);
    hr = ID2D1GeometrySink_Close(sink);
    ok(hr == D2DERR_WRONG_STATE, "Got unexpected hr %#x.\n", hr);
    ID2D1GeometrySink_AddLine(sink, point);
    ID2D1GeometrySink_Release(sink);
    hr = ID2D1PathGeometry_GetFigureCount(geometry, &count);
    ok(hr == D2DERR_WRONG_STATE, "Got unexpected hr %#x.\n", hr);
    hr = ID2D1PathGeometry_GetSegmentCount(geometry, &count);
    ok(hr == D2DERR_WRONG_STATE, "Got unexpected hr %#x.\n", hr);
    ID2D1PathGeometry_Release(geometry);

    /* Empty figure. */
    hr = ID2D1Factory_CreatePathGeometry(factory, &geometry);
    ok(SUCCEEDED(hr), "Failed to create path geometry, hr %#x.\n", hr);
    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(SUCCEEDED(hr), "Failed to open geometry sink, hr %#x.\n", hr);
    set_point(&point, 123.0f, 456.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);
    hr = ID2D1GeometrySink_Close(sink);
    ok(SUCCEEDED(hr), "Failed to close geometry sink, hr %#x.\n", hr);
    ID2D1GeometrySink_Release(sink);
    hr = ID2D1PathGeometry_GetFigureCount(geometry, &count);
    ok(SUCCEEDED(hr), "Failed to get figure count, hr %#x.\n", hr);
    ok(count == 1, "Got unexpected figure count %u.\n", count);
    hr = ID2D1PathGeometry_GetSegmentCount(geometry, &count);
    ok(SUCCEEDED(hr), "Failed to get segment count, hr %#x.\n", hr);
    ok(count == 1, "Got unexpected segment count %u.\n", count);

    set_rect(&rect, 0.0f, 0.0f, 0.0f, 0.0f);
    hr = ID2D1PathGeometry_GetBounds(geometry, NULL, &rect);
    ok(SUCCEEDED(hr), "Failed to get geometry bounds, hr %#x.\n", hr);
    match = compare_rect(&rect, 123.0f, 456.0f, 123.0f, 456.0f, 0);
    ok(match, "Got unexpected rectangle {%.8e, %.8e, %.8e, %.8e}.\n",
            rect.left, rect.top, rect.right, rect.bottom);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 80.0f, 640.0f);
    scale_matrix(&matrix, 2.0f, 0.5f);
    hr = ID2D1PathGeometry_GetBounds(geometry, &matrix, &rect);
    ok(SUCCEEDED(hr), "Failed to get geometry bounds, hr %#x.\n", hr);
    match = compare_rect(&rect, 326.0f, 868.0f, 326.0f, 868.0f, 0);
    ok(match, "Got unexpected rectangle {%.8e, %.8e, %.8e, %.8e}.\n",
            rect.left, rect.top, rect.right, rect.bottom);

    ID2D1PathGeometry_Release(geometry);

    /* Close right after Open(). */
    hr = ID2D1Factory_CreatePathGeometry(factory, &geometry);
    ok(SUCCEEDED(hr), "Failed to create path geometry, hr %#x.\n", hr);

    /* Not open yet. */
    set_rect(&rect, 1.0f, 2.0f, 3.0f, 4.0f);
    hr = ID2D1PathGeometry_GetBounds(geometry, NULL, &rect);
    ok(hr == D2DERR_WRONG_STATE, "Unexpected hr %#x.\n", hr);
    match = compare_rect(&rect, 1.0f, 2.0f, 3.0f, 4.0f, 0);
    ok(match, "Got unexpected rectangle {%.8e, %.8e, %.8e, %.8e}.\n",
            rect.left, rect.top, rect.right, rect.bottom);

    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(SUCCEEDED(hr), "Failed to open geometry sink, hr %#x.\n", hr);

    /* Open, not closed. */
    set_rect(&rect, 1.0f, 2.0f, 3.0f, 4.0f);
    hr = ID2D1PathGeometry_GetBounds(geometry, NULL, &rect);
    ok(hr == D2DERR_WRONG_STATE, "Unexpected hr %#x.\n", hr);
    match = compare_rect(&rect, 1.0f, 2.0f, 3.0f, 4.0f, 0);
    ok(match, "Got unexpected rectangle {%.8e, %.8e, %.8e, %.8e}.\n",
            rect.left, rect.top, rect.right, rect.bottom);

    hr = ID2D1GeometrySink_Close(sink);
    ok(SUCCEEDED(hr), "Failed to close geometry sink, hr %#x.\n", hr);
    ID2D1GeometrySink_Release(sink);
    hr = ID2D1PathGeometry_GetFigureCount(geometry, &count);
    ok(SUCCEEDED(hr), "Failed to get figure count, hr %#x.\n", hr);
    ok(count == 0, "Got unexpected figure count %u.\n", count);
    hr = ID2D1PathGeometry_GetSegmentCount(geometry, &count);
    ok(SUCCEEDED(hr), "Failed to get segment count, hr %#x.\n", hr);
    ok(count == 0, "Got unexpected segment count %u.\n", count);

    set_rect(&rect, 0.0f, 0.0f, 0.0f, 0.0f);
    hr = ID2D1PathGeometry_GetBounds(geometry, NULL, &rect);
    ok(SUCCEEDED(hr), "Failed to get geometry bounds, hr %#x.\n", hr);
    ok(rect.left > rect.right && rect.top > rect.bottom,
            "Got unexpected rectangle {%.8e, %.8e, %.8e, %.8e}.\n", rect.left, rect.top, rect.right, rect.bottom);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 10.0f, 20.0f);
    scale_matrix(&matrix, 10.0f, 20.0f);
    set_rect(&rect, 0.0f, 0.0f, 0.0f, 0.0f);
    hr = ID2D1PathGeometry_GetBounds(geometry, &matrix, &rect);
    ok(SUCCEEDED(hr), "Failed to get geometry bounds, hr %#x.\n", hr);
    ok(rect.left > rect.right && rect.top > rect.bottom,
            "Got unexpected rectangle {%.8e, %.8e, %.8e, %.8e}.\n", rect.left, rect.top, rect.right, rect.bottom);

    ID2D1PathGeometry_Release(geometry);

    /* GetBounds() with bezier segments. */
    hr = ID2D1Factory_CreatePathGeometry(factory, &geometry);
    ok(SUCCEEDED(hr), "Failed to create path geometry, hr %#x.\n", hr);
    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(SUCCEEDED(hr), "Failed to open geometry sink, hr %#x.\n", hr);
    fill_geometry_sink_bezier(sink, 0);
    hr = ID2D1GeometrySink_Close(sink);
    ok(SUCCEEDED(hr), "Failed to close geometry sink, hr %#x.\n", hr);
    ID2D1GeometrySink_Release(sink);

    set_rect(&rect, 0.0f, 0.0f, 0.0f, 0.0f);
    hr = ID2D1PathGeometry_GetBounds(geometry, NULL, &rect);
    ok(SUCCEEDED(hr), "Failed to get geometry bounds, hr %#x.\n", hr);
    match = compare_rect(&rect, 5.0f, 20.0f, 75.0f, 752.0f, 0);
    ok(match, "Got unexpected rectangle {%.8e, %.8e, %.8e, %.8e}.\n",
            rect.left, rect.top, rect.right, rect.bottom);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 80.0f, 640.0f);
    scale_matrix(&matrix, 2.0f, 0.5f);
    set_rect(&rect, 0.0f, 0.0f, 0.0f, 0.0f);
    hr = ID2D1PathGeometry_GetBounds(geometry, &matrix, &rect);
    ok(SUCCEEDED(hr), "Failed to get geometry bounds, hr %#x.\n", hr);
    match = compare_rect(&rect, 90.0f, 650.0f, 230.0f, 1016.0f, 0);
    ok(match, "Got unexpected rectangle {%.8e, %.8e, %.8e, %.8e}.\n",
            rect.left, rect.top, rect.right, rect.bottom);

    ID2D1PathGeometry_Release(geometry);

    /* GetBounds() with bezier segments and some hollow components. */
    hr = ID2D1Factory_CreatePathGeometry(factory, &geometry);
    ok(SUCCEEDED(hr), "Failed to create path geometry, hr %#x.\n", hr);
    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(SUCCEEDED(hr), "Failed to open geometry sink, hr %#x.\n", hr);
    fill_geometry_sink_bezier(sink, 2);
    hr = ID2D1GeometrySink_Close(sink);
    ok(SUCCEEDED(hr), "Failed to close geometry sink, hr %#x.\n", hr);
    ID2D1GeometrySink_Release(sink);

    set_rect(&rect, 0.0f, 0.0f, 0.0f, 0.0f);
    hr = ID2D1PathGeometry_GetBounds(geometry, NULL, &rect);
    ok(SUCCEEDED(hr), "Failed to get geometry bounds, hr %#x.\n", hr);
    match = compare_rect(&rect, 5.0f, 472.0f, 75.0f, 752.0f, 0);
    ok(match, "Got unexpected rectangle {%.8e, %.8e, %.8e, %.8e}.\n",
            rect.left, rect.top, rect.right, rect.bottom);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 80.0f, 640.0f);
    scale_matrix(&matrix, 2.0f, 0.5f);
    set_rect(&rect, 0.0f, 0.0f, 0.0f, 0.0f);
    hr = ID2D1PathGeometry_GetBounds(geometry, &matrix, &rect);
    ok(SUCCEEDED(hr), "Failed to get geometry bounds, hr %#x.\n", hr);
    match = compare_rect(&rect, 90.0f, 876.0f, 230.0f, 1016.0f, 0);
    ok(match, "Got unexpected rectangle {%.8e, %.8e, %.8e, %.8e}.\n",
            rect.left, rect.top, rect.right, rect.bottom);

    ID2D1PathGeometry_Release(geometry);

    /* GetBounds() with bezier segments and all hollow components. */
    hr = ID2D1Factory_CreatePathGeometry(factory, &geometry);
    ok(SUCCEEDED(hr), "Failed to create path geometry, hr %#x.\n", hr);
    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(SUCCEEDED(hr), "Failed to open geometry sink, hr %#x.\n", hr);
    fill_geometry_sink_bezier(sink, 4);
    hr = ID2D1GeometrySink_Close(sink);
    ok(SUCCEEDED(hr), "Failed to close geometry sink, hr %#x.\n", hr);
    ID2D1GeometrySink_Release(sink);

    set_rect(&rect, 0.0f, 0.0f, 0.0f, 0.0f);
    hr = ID2D1PathGeometry_GetBounds(geometry, NULL, &rect);
    ok(SUCCEEDED(hr), "Failed to get geometry bounds, hr %#x.\n", hr);
    match = compare_rect(&rect, INFINITY, INFINITY, FLT_MAX, FLT_MAX, 0);
    ok(match, "Got unexpected rectangle {%.8e, %.8e, %.8e, %.8e}.\n",
            rect.left, rect.top, rect.right, rect.bottom);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 80.0f, 640.0f);
    scale_matrix(&matrix, 2.0f, 0.5f);
    set_rect(&rect, 0.0f, 0.0f, 0.0f, 0.0f);
    hr = ID2D1PathGeometry_GetBounds(geometry, &matrix, &rect);
    ok(SUCCEEDED(hr), "Failed to get geometry bounds, hr %#x.\n", hr);
    match = compare_rect(&rect, INFINITY, INFINITY, FLT_MAX, FLT_MAX, 0);
    ok(match, "Got unexpected rectangle {%.8e, %.8e, %.8e, %.8e}.\n",
            rect.left, rect.top, rect.right, rect.bottom);

    ID2D1PathGeometry_Release(geometry);

    hr = ID2D1Factory_CreatePathGeometry(factory, &geometry);
    ok(SUCCEEDED(hr), "Failed to create path geometry, hr %#x.\n", hr);
    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(SUCCEEDED(hr), "Failed to open geometry sink, hr %#x.\n", hr);
    /* The fillmode that's used is the last one set before the sink is closed. */
    ID2D1GeometrySink_SetFillMode(sink, D2D1_FILL_MODE_WINDING);
    fill_geometry_sink(sink, 0);
    ID2D1GeometrySink_SetFillMode(sink, D2D1_FILL_MODE_ALTERNATE);
    hr = ID2D1GeometrySink_Close(sink);
    ok(SUCCEEDED(hr), "Failed to close geometry sink, hr %#x.\n", hr);
    hr = ID2D1PathGeometry_GetFigureCount(geometry, &count);
    ok(SUCCEEDED(hr), "Failed to get figure count, hr %#x.\n", hr);
    ok(count == 6, "Got unexpected figure count %u.\n", count);
    hr = ID2D1PathGeometry_GetSegmentCount(geometry, &count);
    ok(SUCCEEDED(hr), "Failed to get segment count, hr %#x.\n", hr);
    /* Intersections don't create extra segments. */
    ok(count == 44, "Got unexpected segment count %u.\n", count);
    ID2D1GeometrySink_SetFillMode(sink, D2D1_FILL_MODE_WINDING);
    ID2D1GeometrySink_Release(sink);

    geometry_sink_init(&simplify_sink);
    hr = ID2D1PathGeometry_Simplify(geometry, D2D1_GEOMETRY_SIMPLIFICATION_OPTION_LINES,
            NULL, 0.0f, &simplify_sink.ID2D1SimplifiedGeometrySink_iface);
    ok(SUCCEEDED(hr), "Failed to simplify geometry, hr %#x.\n", hr);
    geometry_sink_check(&simplify_sink, D2D1_FILL_MODE_ALTERNATE, 6, &expected_figures[0], 0);
    geometry_sink_cleanup(&simplify_sink);
    geometry_sink_init(&simplify_sink);
    hr = ID2D1PathGeometry_Simplify(geometry, D2D1_GEOMETRY_SIMPLIFICATION_OPTION_CUBICS_AND_LINES,
            NULL, 0.0f, &simplify_sink.ID2D1SimplifiedGeometrySink_iface);
    ok(SUCCEEDED(hr), "Failed to simplify geometry, hr %#x.\n", hr);
    geometry_sink_check(&simplify_sink, D2D1_FILL_MODE_ALTERNATE, 6, &expected_figures[0], 0);
    geometry_sink_cleanup(&simplify_sink);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 80.0f, 640.0f);
    scale_matrix(&matrix, 1.0f, -1.0f);
    hr = ID2D1Factory_CreateTransformedGeometry(factory, (ID2D1Geometry *)geometry, &matrix, &transformed_geometry);
    ok(SUCCEEDED(hr), "Failed to create transformed geometry, hr %#x.\n", hr);

    geometry_sink_init(&simplify_sink);
    hr = ID2D1TransformedGeometry_Simplify(transformed_geometry, D2D1_GEOMETRY_SIMPLIFICATION_OPTION_LINES,
            NULL, 0.0f, &simplify_sink.ID2D1SimplifiedGeometrySink_iface);
    ok(SUCCEEDED(hr), "Failed to simplify geometry, hr %#x.\n", hr);
    geometry_sink_check(&simplify_sink, D2D1_FILL_MODE_ALTERNATE, 6, &expected_figures[6], 0);
    geometry_sink_cleanup(&simplify_sink);

    ID2D1TransformedGeometry_GetSourceGeometry(transformed_geometry, &tmp_geometry);
    ok(tmp_geometry == (ID2D1Geometry *)geometry,
            "Got unexpected source geometry %p, expected %p.\n", tmp_geometry, geometry);
    ID2D1TransformedGeometry_GetTransform(transformed_geometry, &tmp_matrix);
    ok(!memcmp(&tmp_matrix, &matrix, sizeof(matrix)),
            "Got unexpected matrix {%.8e, %.8e, %.8e, %.8e, %.8e, %.8e}.\n",
            tmp_matrix._11, tmp_matrix._12, tmp_matrix._21,
            tmp_matrix._22, tmp_matrix._31, tmp_matrix._32);
    geometry_sink_init(&simplify_sink);
    hr = ID2D1Geometry_Simplify(tmp_geometry, D2D1_GEOMETRY_SIMPLIFICATION_OPTION_LINES,
            &tmp_matrix, 0.0f, &simplify_sink.ID2D1SimplifiedGeometrySink_iface);
    ok(SUCCEEDED(hr), "Failed to simplify geometry, hr %#x.\n", hr);
    geometry_sink_check(&simplify_sink, D2D1_FILL_MODE_ALTERNATE, 6, &expected_figures[6], 0);
    geometry_sink_cleanup(&simplify_sink);
    ID2D1Geometry_Release(tmp_geometry);

    ID2D1RenderTarget_BeginDraw(rt);
    set_color(&color, 0.396f, 0.180f, 0.537f, 1.0f);
    ID2D1RenderTarget_Clear(rt, &color);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)geometry, (ID2D1Brush *)brush, NULL);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)transformed_geometry, (ID2D1Brush *)brush, NULL);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to end draw, hr %#x.\n", hr);
    match = compare_surface(&ctx, "3aace1b22aae111cb577614fed16e4eb1650dba5");
    ok(match, "Surface does not match.\n");

    /* Edge test. */
    set_point(&point, 94.0f, 620.0f);
    contains = TRUE;
    hr = ID2D1TransformedGeometry_FillContainsPoint(transformed_geometry, point, NULL, 0.0f, &contains);
    ok(hr == S_OK, "FillContainsPoint failed, hr %#x.\n", hr);
    ok(!contains, "Got unexpected contains %#x.\n", contains);

    set_point(&point, 95.0f, 620.0f);
    contains = FALSE;
    hr = ID2D1TransformedGeometry_FillContainsPoint(transformed_geometry, point, NULL, 0.0f, &contains);
    ok(hr == S_OK, "FillContainsPoint failed, hr %#x.\n", hr);
    ok(contains == TRUE, "Got unexpected contains %#x.\n", contains);

    /* With transformation matrix. */
    set_matrix_identity(&matrix);
    translate_matrix(&matrix, -10.0f, 0.0f);
    set_point(&point, 85.0f, 620.0f);
    contains = FALSE;
    hr = ID2D1TransformedGeometry_FillContainsPoint(transformed_geometry, point, &matrix, 0.0f, &contains);
    ok(hr == S_OK, "FillContainsPoint failed, hr %#x.\n", hr);
    ok(contains == TRUE, "Got unexpected contains %#x.\n", contains);

    ID2D1TransformedGeometry_Release(transformed_geometry);
    ID2D1PathGeometry_Release(geometry);

    hr = ID2D1Factory_CreatePathGeometry(factory, &geometry);
    ok(SUCCEEDED(hr), "Failed to create path geometry, hr %#x.\n", hr);
    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(SUCCEEDED(hr), "Failed to open geometry sink, hr %#x.\n", hr);
    fill_geometry_sink(sink, 0);
    ID2D1GeometrySink_SetFillMode(sink, D2D1_FILL_MODE_WINDING);
    hr = ID2D1GeometrySink_Close(sink);
    ok(SUCCEEDED(hr), "Failed to close geometry sink, hr %#x.\n", hr);
    hr = ID2D1PathGeometry_GetFigureCount(geometry, &count);
    ok(SUCCEEDED(hr), "Failed to get figure count, hr %#x.\n", hr);
    ok(count == 6, "Got unexpected figure count %u.\n", count);
    hr = ID2D1PathGeometry_GetSegmentCount(geometry, &count);
    ok(SUCCEEDED(hr), "Failed to get segment count, hr %#x.\n", hr);
    ok(count == 44, "Got unexpected segment count %u.\n", count);
    ID2D1GeometrySink_Release(sink);

    set_rect(&rect, 0.0f, 0.0f, 0.0f, 0.0f);
    hr = ID2D1PathGeometry_GetBounds(geometry, NULL, &rect);
    ok(SUCCEEDED(hr), "Failed to get geometry bounds, hr %#x.\n", hr);
    match = compare_rect(&rect, 5.0f, 20.0f, 235.0f, 300.0f, 0);
    ok(match, "Got unexpected rectangle {%.8e, %.8e, %.8e, %.8e}.\n",
            rect.left, rect.top, rect.right, rect.bottom);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 100.0f, 50.0f);
    scale_matrix(&matrix, 2.0f, 1.5f);
    rotate_matrix(&matrix, M_PI / 4.0f);
    set_rect(&rect, 0.0f, 0.0f, 0.0f, 0.0f);
    hr = ID2D1PathGeometry_GetBounds(geometry, &matrix, &rect);
    ok(SUCCEEDED(hr), "Failed to get geometry bounds, hr %#x.\n", hr);
    match = compare_rect(&rect, -3.17192993e+02f, 8.71231079e+01f, 4.04055908e+02f, 6.17453125e+02f, 1);
    ok(match, "Got unexpected rectangle {%.8e, %.8e, %.8e, %.8e}.\n",
            rect.left, rect.top, rect.right, rect.bottom);

    geometry_sink_init(&simplify_sink);
    hr = ID2D1PathGeometry_Simplify(geometry, D2D1_GEOMETRY_SIMPLIFICATION_OPTION_LINES,
            NULL, 0.0f, &simplify_sink.ID2D1SimplifiedGeometrySink_iface);
    ok(SUCCEEDED(hr), "Failed to simplify geometry, hr %#x.\n", hr);
    geometry_sink_check(&simplify_sink, D2D1_FILL_MODE_WINDING, 6, &expected_figures[0], 0);
    geometry_sink_cleanup(&simplify_sink);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 320.0f, 320.0f);
    scale_matrix(&matrix, -1.0f, 1.0f);
    hr = ID2D1Factory_CreateTransformedGeometry(factory, (ID2D1Geometry *)geometry, &matrix, &transformed_geometry);
    ok(SUCCEEDED(hr), "Failed to create transformed geometry, hr %#x.\n", hr);

    ID2D1RenderTarget_BeginDraw(rt);
    ID2D1RenderTarget_Clear(rt, &color);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)geometry, (ID2D1Brush *)brush, NULL);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)transformed_geometry, (ID2D1Brush *)brush, NULL);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to end draw, hr %#x.\n", hr);
    match = compare_surface(&ctx, "bfb40a1f007694fa07dbd3b854f3f5d9c3e1d76b");
    ok(match, "Surface does not match.\n");
    ID2D1TransformedGeometry_Release(transformed_geometry);
    ID2D1PathGeometry_Release(geometry);

    hr = ID2D1Factory_CreatePathGeometry(factory, &geometry);
    ok(SUCCEEDED(hr), "Failed to create path geometry, hr %#x.\n", hr);
    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(SUCCEEDED(hr), "Failed to open geometry sink, hr %#x.\n", hr);
    fill_geometry_sink_bezier(sink, 0);
    hr = ID2D1GeometrySink_Close(sink);
    ok(SUCCEEDED(hr), "Failed to close geometry sink, hr %#x.\n", hr);
    hr = ID2D1PathGeometry_GetFigureCount(geometry, &count);
    ok(SUCCEEDED(hr), "Failed to get figure count, hr %#x.\n", hr);
    ok(count == 4, "Got unexpected figure count %u.\n", count);
    hr = ID2D1PathGeometry_GetSegmentCount(geometry, &count);
    ok(SUCCEEDED(hr), "Failed to get segment count, hr %#x.\n", hr);
    ok(count == 20, "Got unexpected segment count %u.\n", count);
    ID2D1GeometrySink_Release(sink);

    geometry_sink_init(&simplify_sink);
    hr = ID2D1PathGeometry_Simplify(geometry, D2D1_GEOMETRY_SIMPLIFICATION_OPTION_CUBICS_AND_LINES,
            NULL, 0.0f, &simplify_sink.ID2D1SimplifiedGeometrySink_iface);
    ok(SUCCEEDED(hr), "Failed to simplify geometry, hr %#x.\n", hr);
    geometry_sink_check(&simplify_sink, D2D1_FILL_MODE_ALTERNATE, 4, &expected_figures[12], 1);
    geometry_sink_cleanup(&simplify_sink);
    geometry_sink_init(&simplify_sink);
    hr = ID2D1PathGeometry_Simplify(geometry, D2D1_GEOMETRY_SIMPLIFICATION_OPTION_LINES,
            NULL, 100.0f, &simplify_sink.ID2D1SimplifiedGeometrySink_iface);
    ok(SUCCEEDED(hr), "Failed to simplify geometry, hr %#x.\n", hr);
    geometry_sink_check(&simplify_sink, D2D1_FILL_MODE_ALTERNATE, 4, &expected_figures[20], 1);
    geometry_sink_cleanup(&simplify_sink);
    geometry_sink_init(&simplify_sink);
    hr = ID2D1PathGeometry_Simplify(geometry, D2D1_GEOMETRY_SIMPLIFICATION_OPTION_LINES,
            NULL, 10.0f, &simplify_sink.ID2D1SimplifiedGeometrySink_iface);
    ok(SUCCEEDED(hr), "Failed to simplify geometry, hr %#x.\n", hr);
    geometry_sink_check(&simplify_sink, D2D1_FILL_MODE_ALTERNATE, 4, &expected_figures[24], 1);
    geometry_sink_cleanup(&simplify_sink);

    set_matrix_identity(&matrix);
    scale_matrix(&matrix, 0.5f, 2.0f);
    translate_matrix(&matrix, 400.0f, -33.0f);
    rotate_matrix(&matrix, M_PI / 4.0f);
    scale_matrix(&matrix, 2.0f, 0.5f);
    hr = ID2D1Factory_CreateTransformedGeometry(factory, (ID2D1Geometry *)geometry, &matrix, &transformed_geometry);
    ok(SUCCEEDED(hr), "Failed to create transformed geometry, hr %#x.\n", hr);

    geometry_sink_init(&simplify_sink);
    hr = ID2D1TransformedGeometry_Simplify(transformed_geometry, D2D1_GEOMETRY_SIMPLIFICATION_OPTION_CUBICS_AND_LINES,
            NULL, 0.0f, &simplify_sink.ID2D1SimplifiedGeometrySink_iface);
    ok(SUCCEEDED(hr), "Failed to simplify geometry, hr %#x.\n", hr);
    geometry_sink_check(&simplify_sink, D2D1_FILL_MODE_ALTERNATE, 4, &expected_figures[16], 4);
    geometry_sink_cleanup(&simplify_sink);

    ID2D1RenderTarget_BeginDraw(rt);
    ID2D1RenderTarget_Clear(rt, &color);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)geometry, (ID2D1Brush *)brush, NULL);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)transformed_geometry, (ID2D1Brush *)brush, NULL);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to end draw, hr %#x.\n", hr);
    match = compare_figure(&ctx, 0, 0, 160, 160, 0xff652e89, 64,
            "7xoCngECngECngECngECngECngECngECnQEEnAEEnAEEnAEEnAEEmwEGmgEGmgEGmgEGmQEImAEI"
            "lAEECASLAQgKCIEBDQoMew8KD3YQDBByEgwSbhMOEmwUDhRpFBAUZxUQFWUVEhVjFhIWYRYUFl8X"
            "FBddFxYWXRYYFlsXGBdaFhoWWRYcFlgVHhVXFSAVVhQiFFUUIxRVEyYTVBIoElQRKhFUECwQUxAu"
            "EFIOMg5SDTQNUgs4C1IJPAlRCEAIUAZEBlAESARQAU4BTgJQAkgGUAY/C1ALMhNQEyoTUBMyC1AL"
            "PwZQBkgCUAJOAU4BUARIBFAGRAZQCEAIUQk8CVILOAtSDTQNUg4yDlIQLhBTECwQVBEqEVQSKBJU"
            "EyYTVBQjFFYUIhRWFSAVVxUeFVgWHBZZFhoWWhcYF1sWGBZcFxYWXhcUF18WFBZhFhIWYxUSFWUV"
            "EBVnFBAUaRQOFGsTDhJvEgwSchAMEHYPCg96DQoMggEICgiLAQQIBJQBCJgBCJkBBpoBBpoBBpoB"
            "BpsBBJwBBJwBBJwBBJwBBJ0BAp4BAp4BAp4BAp4BAp4BAp4BAp4BAgAA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 0, 226, 160, 160, 0xff652e89, 64,
            "7xoCngECngECngECngECngECngECngECnQEEnAEEnAEEnAEEnAEEmwEGmgEGmgEGmgEGmQEImAEI"
            "lAEECASLAQgKCIEBDQoMew8KD3YQDBByEgwSbhMOEmwUDhRpFBAUZxUQFWUVEhVjFhIWYRYUFl8X"
            "FBddFxYWXRYYFlsXGBdaFhoWWRYcFlgVHhVXFSAVVhQiFFUUIxRVEyYTVBIoElQRKhFUECwQUxAu"
            "EFIOMg5SDTQNUgs4C1IJPAlRCEAIUAZEBlAESARQAU4BTgJQAkgGUAY/C1ALMhNQEyoTUBMyC1AL"
            "PwZQBkgCUAJOAU4BUARIBFAGRAZQCEAIUQk8CVILOAtSDTQNUg4yDlIQLhBTECwQVBEqEVQSKBJU"
            "EyYTVBQjFFYUIhRWFSAVVxUeFVgWHBZZFhoWWhcYF1sWGBZcFxYWXhcUF18WFBZhFhIWYxUSFWUV"
            "EBVnFBAUaRQOFGsTDhJvEgwSchAMEHYPCg96DQoMggEICgiLAQQIBJQBCJgBCJkBBpoBBpoBBpoB"
            "BpsBBJwBBJwBBJwBBJwBBJ0BAp4BAp4BAp4BAp4BAp4BAp4BAp4BAgAA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160, 0, 320, 160, 0xff652e89, 64,
            "gVQBwAIBWgHlAQFYAecBAVYB6QEBVAHrAQEjDCMB7AECHhQeAu0BAxoYGgPvAQMWHhYD8QEDFCAU"
            "A/MBBBAkEAT0AQUOJw0F9QEGCioKBvcBBggsCAb4AQgFLgUI+QEJATIBCfsBCAIwAgj8AQcFLAUH"
            "/QEFCCgIBf4BBAwiDAT/AQIQHBAClwISlwIBPgGAAgI8Av8BAzwD/QEEPAT7AQY6BvkBBzoH+AEI"
            "OAj3AQk4CfYBCTgK9AELNgvzAQw2DPIBDDYM8QEONA7wAQ40DvABDjQO7wEPNA/uAQ80D+4BEDIQ"
            "7QERMhHsAREyEewBETIR7AERMhHsAREyEewBETIR7AERMhHsAREyEewBETIR7AERMhHsAREyEewB"
            "ETIR7AERMhHsAREyEe0BEDIQ7gEQMw/uAQ80D+4BDzQP7wEONA7wAQ40DvEBDDYM8gEMNgzzAQs2"
            "C/QBCzcK9QEJOAn3AQg4CfcBBzoH+QEGOgb7AQU6BfwBBDwE/QEDPAP/AQE+AZkCDpkCAhIYEgKA"
            "AgMNIA0D/wEFCSYJBf4BBgYqBgf8AQgDLgMI+wFG+gEIAzADCPkBBwYuBgf3AQYKKgoG9gEFDCgM"
            "BfUBBBAlDwTzAQQSIhIE8QEDFh4WA/ABAhkaGQLvAQIcFhwC7QECIBAgAusBASgEKAHpAQFWAecB"
            "AVgB5QEBWgHAAgHhUgAA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160, 160, 320, 160, 0xff652e89, 64,
            "/VUB5QEBWAHnAQFWAekBAVQB6wECIQ8hAe0BAh0VHQLuAQIZGhkD7wEDFh4WA/EBBBMhEwPzAQQQ"
            "JQ8F9AEFDCgNBfUBBgoqCgb3AQcHLQcG+QEIBC8ECPkBPAEJ+wEIAy8CCP0BBgYrBQf9AQUJJgkF"
            "/wEDDSANBP8BAhEaEQKYAhAXAYACAT4BgAICPQL+AQM8BPwBBTsE+wEGOgb6AQc5B/gBCDgJ9gEJ"
            "OAn2AQo3CvQBCzcK8wEMNgzyAQ01DPIBDTUN8AEONA7wAQ40D+4BDzQP7gEQMw/uARAzEO0BEDIR"
            "7AERMhHsAREyEewBETIR7AERMhLrAREyEusBETIS6wERMhLrAREyEusBETIS6wERMhHsAREyEewB"
            "ETIR7QEQMhHtARAzEO0BEDMP7gEPNA/vAQ40D+8BDjQO8QENNQ3xAQ01DPMBCzYM8wELNwr1AQo3"
            "CvUBCTgJ9wEIOAn4AQc5B/kBBjoG+wEFOwT9AQM8BP4BAj0C/wEBPgGYAhAXAYACAhEaEQKAAgMN"
            "IA0E/gEFCSYJBf4BBgYrBQf8AQgDLwII+wE8AQn6AQgELwQI+AEHBy0HBvcBBgoqCgb2AQUNJw0F"
            "9AEEECQQBfIBBBMhEwPxAQMWHhYD8AECGRoZA+4BAh0VHQLsAQIhDiIB6wEBVAHpAQFWAecBAVgB"
            "wAIBwlYA");
    ok(match, "Figure does not match.\n");
    ID2D1TransformedGeometry_Release(transformed_geometry);
    ID2D1PathGeometry_Release(geometry);

    hr = ID2D1Factory_CreatePathGeometry(factory, &geometry);
    ok(SUCCEEDED(hr), "Failed to create path geometry, hr %#x.\n", hr);
    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(SUCCEEDED(hr), "Failed to open geometry sink, hr %#x.\n", hr);
    fill_geometry_sink_bezier(sink, 0);
    ID2D1GeometrySink_SetFillMode(sink, D2D1_FILL_MODE_WINDING);
    hr = ID2D1GeometrySink_Close(sink);
    ok(SUCCEEDED(hr), "Failed to close geometry sink, hr %#x.\n", hr);
    hr = ID2D1PathGeometry_GetFigureCount(geometry, &count);
    ok(SUCCEEDED(hr), "Failed to get figure count, hr %#x.\n", hr);
    ok(count == 4, "Got unexpected figure count %u.\n", count);
    hr = ID2D1PathGeometry_GetSegmentCount(geometry, &count);
    ok(SUCCEEDED(hr), "Failed to get segment count, hr %#x.\n", hr);
    ok(count == 20, "Got unexpected segment count %u.\n", count);
    ID2D1GeometrySink_Release(sink);

    geometry_sink_init(&simplify_sink);
    hr = ID2D1PathGeometry_Simplify(geometry, D2D1_GEOMETRY_SIMPLIFICATION_OPTION_CUBICS_AND_LINES,
            NULL, 0.0f, &simplify_sink.ID2D1SimplifiedGeometrySink_iface);
    ok(SUCCEEDED(hr), "Failed to simplify geometry, hr %#x.\n", hr);
    geometry_sink_check(&simplify_sink, D2D1_FILL_MODE_WINDING, 4, &expected_figures[12], 1);
    geometry_sink_cleanup(&simplify_sink);
    geometry_sink_init(&simplify_sink);
    hr = ID2D1PathGeometry_Simplify(geometry, D2D1_GEOMETRY_SIMPLIFICATION_OPTION_LINES,
            NULL, 100.0f, &simplify_sink.ID2D1SimplifiedGeometrySink_iface);
    ok(SUCCEEDED(hr), "Failed to simplify geometry, hr %#x.\n", hr);
    geometry_sink_check(&simplify_sink, D2D1_FILL_MODE_WINDING, 4, &expected_figures[20], 1);
    geometry_sink_cleanup(&simplify_sink);
    geometry_sink_init(&simplify_sink);
    hr = ID2D1PathGeometry_Simplify(geometry, D2D1_GEOMETRY_SIMPLIFICATION_OPTION_LINES,
            NULL, 10.0f, &simplify_sink.ID2D1SimplifiedGeometrySink_iface);
    ok(SUCCEEDED(hr), "Failed to simplify geometry, hr %#x.\n", hr);
    geometry_sink_check(&simplify_sink, D2D1_FILL_MODE_WINDING, 4, &expected_figures[24], 1);
    geometry_sink_cleanup(&simplify_sink);

    set_matrix_identity(&matrix);
    scale_matrix(&matrix, 0.5f, 2.0f);
    translate_matrix(&matrix, 127.0f, 80.0f);
    rotate_matrix(&matrix, M_PI / -4.0f);
    scale_matrix(&matrix, 2.0f, 0.5f);
    hr = ID2D1Factory_CreateTransformedGeometry(factory, (ID2D1Geometry *)geometry, &matrix, &transformed_geometry);
    ok(SUCCEEDED(hr), "Failed to create transformed geometry, hr %#x.\n", hr);

    ID2D1RenderTarget_BeginDraw(rt);
    ID2D1RenderTarget_Clear(rt, &color);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)geometry, (ID2D1Brush *)brush, NULL);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)transformed_geometry, (ID2D1Brush *)brush, NULL);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to end draw, hr %#x.\n", hr);
    match = compare_figure(&ctx, 0, 0, 160, 160, 0xff652e89, 64,
            "7xoCngECngECngECngECngECngECngECnQEEnAEEnAEEnAEEnAEEmwEGmgEGmgEGmgEGmQEImAEI"
            "lAEQiwEagQEjeyh2LHIwbjNsNmk4ZzplPGM+YUBfQl1DXURbRlpGWUhYSFdKVkpVS1VMVExUTFRM"
            "U05STlJOUk5STlFQUFBQUFBQTlRIXD9mMnYqdjJmP1xIVE5QUFBQUFBQUU5STlJOUk5STlNMVExU"
            "TFRMVEtWSlZKV0hYSFlGWkZbRFxDXkJfQGE+YzxlOmc4aTZrM28wcix2KHojggEaiwEQlAEImAEI"
            "mQEGmgEGmgEGmgEGmwEEnAEEnAEEnAEEnAEEnQECngECngECngECngECngECngECngEC");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 0, 226, 160, 160, 0xff652e89, 64,
            "7xoCngECngECngECngECngECngECngECnQEEnAEEnAEEnAEEnAEEmwEGmgEGmgEGmgEGmQEImAEI"
            "lAEQiwEagQEjeyh2LHIwbjNsNmk4ZzplPGM+YUBfQl1DXURbRlpGWUhYSFdKVkpVS1VMVExUTFRM"
            "U05STlJOUk5STlFQUFBQUFBQTlRIXD9mMnYqdjJmP1xIVE5QUFBQUFBQUU5STlJOUk5STlNMVExU"
            "TFRMVEtWSlZKV0hYSFlGWkZbRFxDXkJfQGE+YzxlOmc4aTZrM28wcix2KHojggEaiwEQlAEImAEI"
            "mQEGmgEGmgEGmgEGmwEEnAEEnAEEnAEEnAEEnQECngECngECngECngECngECngECngEC");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160, 0, 320, 160, 0xff652e89, 64,
            "4VIBwAIBWgHlAQFYAecBAVYB6QEBVAHrAQIhDiIB7QECHRUdAu4BAhkaGQPvAQMWHhYD8QEEEyET"
            "A/MBBBAkEAT1AQUMKA0F9QEGCioKBvcBBwctBwb5AQgELwQI+QEJATIBCfsBRP0BQ/0BQv8BQf8B"
            "QIECP4ACQIACQf4BQ/wBRPsBRvoBR/gBSPcBSvYBS/QBTPMBTvIBTvIBT/ABUPABUe4BUu4BUu4B"
            "U+0BU+wBVOwBVOwBVOwBVOwBVesBVesBVesBVesBVOwBVOwBVOwBVO0BU+0BU+0BUu4BUu8BUe8B"
            "UPEBT/EBTvIBTvMBTPUBS/UBSvcBSfcBSPkBRvsBRP0BQ/4BQf8BQIECP4ACQIACQf4BQv4BQ/wB"
            "RPsBCQEyAQn6AQgELwQI+AEHBy0GB/cBBgoqCgb2AQUMKA0F9AEEECUPBPMBBBIiEwPxAQMWHhYD"
            "8AECGRoZA+4BAh0VHQLsAQIhDiIB6wEBVAHpAQFWAecBAVgB5QEBWgHAAgEA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160, 160, 320, 160, 0xff652e89, 64,
            "gVQBXAHjAQFaAeUBAVgB5wEBVgHpAQEpAikB6wECIBAgAu0BAhwWHALvAQIZGhkC8AEDFh4WA/EB"
            "BBIiEgTzAQQPJRAE9QEFDCgMBfYBBgoqCgb3AQcGLgYH+QEIAzADCPoBRvsBRPwBRP0BQv8BQIAC"
            "QIECPoECQP8BQv0BRPwBRPsBRvkBSPgBSPcBSvUBTPQBTPMBTvIBTvEBUPABUO8BUu4BUu4BUu4B"
            "Uu0BVOwBVOwBVOwBVOwBVOwBVOwBVOwBVOwBVOwBVOwBVOwBVOwBVOwBVO0BUu4BUu4BUu8BUPAB"
            "UPABUPEBTvIBTvMBTPQBS/YBSvcBSPgBSPkBRvsBRP0BQv8BQIACQIECPoECQP8BQv4BQv0BRPwB"
            "RPsBCQEyAQn5AQgFLgUI+AEGCCwIBvcBBgoqCgb1AQUNJw4F9AEEECQQBPMBAxQgFAPxAQMWHhYD"
            "7wEDGhgaA+0BAh4UHgLsAQEjDCMB6wEBVAHpAQFWAecBAVgB5QEBWgGiVQAA");
    ok(match, "Figure does not match.\n");
    ID2D1TransformedGeometry_Release(transformed_geometry);
    ID2D1PathGeometry_Release(geometry);

    hr = ID2D1Factory_CreatePathGeometry(factory, &geometry);
    ok(SUCCEEDED(hr), "Failed to create path geometry, hr %#x.\n", hr);
    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(SUCCEEDED(hr), "Failed to open geometry sink, hr %#x.\n", hr);

    set_point(&point, 40.0f,   20.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    line_to(sink, 75.0f,  300.0f);
    line_to(sink,  5.0f,  300.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 40.0f,  290.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    line_to(sink, 55.0f,  160.0f);
    line_to(sink, 25.0f,  160.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    hr = ID2D1GeometrySink_Close(sink);
    ok(SUCCEEDED(hr), "Failed to close geometry sink, hr %#x.\n", hr);
    hr = ID2D1PathGeometry_GetFigureCount(geometry, &count);
    ok(SUCCEEDED(hr), "Failed to get figure count, hr %#x.\n", hr);
    ok(count == 2, "Got unexpected figure count %u.\n", count);
    hr = ID2D1PathGeometry_GetSegmentCount(geometry, &count);
    ok(SUCCEEDED(hr), "Failed to get segment count, hr %#x.\n", hr);
    ok(count == 6, "Got unexpected segment count %u.\n", count);
    ID2D1GeometrySink_Release(sink);

    ID2D1RenderTarget_BeginDraw(rt);
    ID2D1RenderTarget_Clear(rt, &color);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)geometry, (ID2D1Brush *)brush, NULL);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to end draw, hr %#x.\n", hr);
    match = compare_surface(&ctx, "a875e68e0cb9c055927b1b50b879f90b24e38470");
    ok(match, "Surface does not match.\n");
    ID2D1PathGeometry_Release(geometry);

    hr = ID2D1Factory_CreatePathGeometry(factory, &geometry);
    ok(SUCCEEDED(hr), "Failed to create path geometry, hr %#x.\n", hr);
    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(SUCCEEDED(hr), "Failed to open geometry sink, hr %#x.\n", hr);
    set_point(&point, 40.0f,   20.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_HOLLOW);
    line_to(sink, 75.0f,  300.0f);
    line_to(sink,  5.0f,  300.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_OPEN);
    hr = ID2D1GeometrySink_Close(sink);
    ok(SUCCEEDED(hr), "Failed to close geometry sink, hr %#x.\n", hr);
    ID2D1GeometrySink_Release(sink);
    hr = ID2D1PathGeometry_GetSegmentCount(geometry, &count);
    ok(SUCCEEDED(hr), "Failed to get segment count, hr %#x.\n", hr);
    ok(count == 2, "Got unexpected segment count %u.\n", count);

    geometry_sink_init(&simplify_sink);
    hr = ID2D1PathGeometry_Simplify(geometry, D2D1_GEOMETRY_SIMPLIFICATION_OPTION_LINES,
            NULL, 0.0f, &simplify_sink.ID2D1SimplifiedGeometrySink_iface);
    ok(SUCCEEDED(hr), "Failed to simplify geometry, hr %#x.\n", hr);
    geometry_sink_check(&simplify_sink, D2D1_FILL_MODE_ALTERNATE, 1, &expected_figures[28], 1);
    geometry_sink_cleanup(&simplify_sink);

    ID2D1PathGeometry_Release(geometry);

    ID2D1SolidColorBrush_Release(brush);
    ID2D1Factory_Release(factory);
    release_test_context(&ctx);
}

static void test_rectangle_geometry(BOOL d3d11)
{
    ID2D1TransformedGeometry *transformed_geometry;
    ID2D1RectangleGeometry *geometry;
    struct geometry_sink sink;
    D2D1_MATRIX_3X2_F matrix;
    D2D1_RECT_F rect, rect2;
    ID2D1Factory *factory;
    D2D1_POINT_2F point;
    BOOL contains;
    HRESULT hr;
    BOOL match;

    static const struct geometry_segment expected_segments[] =
    {
        /* Figure 0. */
        {SEGMENT_LINE, {{{10.0f,  0.0f}}}},
        {SEGMENT_LINE, {{{10.0f, 20.0f}}}},
        {SEGMENT_LINE, {{{ 0.0f, 20.0f}}}},
        /* Figure 1. */
        {SEGMENT_LINE, {{{4.42705116e+01f, 1.82442951e+01f}}}},
        {SEGMENT_LINE, {{{7.95376282e+01f, 5.06049728e+01f}}}},
        {SEGMENT_LINE, {{{5.52671127e+01f, 6.23606796e+01f}}}},
        /* Figure 2. */
        {SEGMENT_LINE, {{{25.0f, 15.0f}}}},
        {SEGMENT_LINE, {{{25.0f, 55.0f}}}},
        {SEGMENT_LINE, {{{25.0f, 55.0f}}}},
        /* Figure 3. */
        {SEGMENT_LINE, {{{35.0f, 45.0f}}}},
        {SEGMENT_LINE, {{{35.0f, 45.0f}}}},
        {SEGMENT_LINE, {{{30.0f, 45.0f}}}},
        /* Figure 4. */
        {SEGMENT_LINE, {{{ 1.07179585e+01f, 2.23205078e+02f}}}},
        {SEGMENT_LINE, {{{-5.85640755e+01f, 2.73205078e+02f}}}},
        {SEGMENT_LINE, {{{-7.85640717e+01f, 2.29903809e+02f}}}},
        /* Figure 5. */
        {SEGMENT_LINE, {{{40.0f, 20.0f}}}},
        {SEGMENT_LINE, {{{40.0f, 40.0f}}}},
        {SEGMENT_LINE, {{{30.0f, 40.0f}}}},
        /* Figure 6. */
        {SEGMENT_LINE, {{{ 2.14359169e+01f, 0.0f}}}},
        {SEGMENT_LINE, {{{-1.17128151e+02f, 0.0f}}}},
        {SEGMENT_LINE, {{{-1.57128143e+02f, 0.0f}}}},
        /* Figure 7. */
        {SEGMENT_LINE, {{{0.0f, 1.11602539e+02f}}}},
        {SEGMENT_LINE, {{{0.0f, 1.36602539e+02f}}}},
        {SEGMENT_LINE, {{{0.0f, 1.14951904e+02f}}}},
    };
    static const struct expected_geometry_figure expected_figures[] =
    {
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, { 0.0f,  0.0f}, 3, &expected_segments[0]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, {20.0f, 30.0f}, 3, &expected_segments[3]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, {25.0f, 15.0f}, 3, &expected_segments[6]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, {30.0f, 45.0f}, 3, &expected_segments[9]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, {-9.28203964e+00f, 1.79903809e+02f},
                3, &expected_segments[12]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, {30.0f, 20.0f}, 3, &expected_segments[15]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, {-1.85640793e+01f, 0.0f}, 3, &expected_segments[18]},
        {D2D1_FIGURE_BEGIN_FILLED, D2D1_FIGURE_END_CLOSED, {0.0f, 8.99519043e+01f}, 3, &expected_segments[21]},
    };

    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &IID_ID2D1Factory, NULL, (void **)&factory);
    ok(SUCCEEDED(hr), "Failed to create factory, hr %#x.\n", hr);

    set_rect(&rect, 0.0f, 0.0f, 0.0f, 0.0f);
    hr = ID2D1Factory_CreateRectangleGeometry(factory, &rect, &geometry);
    ok(SUCCEEDED(hr), "Failed to create geometry, hr %#x.\n", hr);
    ID2D1RectangleGeometry_GetRect(geometry, &rect2);
    match = compare_rect(&rect2, 0.0f, 0.0f, 0.0f, 0.0f, 0);
    ok(match, "Got unexpected rectangle {%.8e, %.8e, %.8e, %.8e}.\n",
            rect2.left, rect2.top, rect2.right, rect2.bottom);
    ID2D1RectangleGeometry_Release(geometry);

    set_rect(&rect, 50.0f, 0.0f, 40.0f, 100.0f);
    hr = ID2D1Factory_CreateRectangleGeometry(factory, &rect, &geometry);
    ok(SUCCEEDED(hr), "Failed to create geometry, hr %#x.\n", hr);
    ID2D1RectangleGeometry_GetRect(geometry, &rect2);
    match = compare_rect(&rect2, 50.0f, 0.0f, 40.0f, 100.0f, 0);
    ok(match, "Got unexpected rectangle {%.8e, %.8e, %.8e, %.8e}.\n",
            rect2.left, rect2.top, rect2.right, rect2.bottom);
    ID2D1RectangleGeometry_Release(geometry);

    set_rect(&rect, 0.0f, 100.0f, 40.0f, 50.0f);
    hr = ID2D1Factory_CreateRectangleGeometry(factory, &rect, &geometry);
    ok(SUCCEEDED(hr), "Failed to create geometry, hr %#x.\n", hr);
    ID2D1RectangleGeometry_GetRect(geometry, &rect2);
    match = compare_rect(&rect2, 0.0f, 100.0f, 40.0f, 50.0f, 0);
    ok(match, "Got unexpected rectangle {%.8e, %.8e, %.8e, %.8e}.\n",
            rect2.left, rect2.top, rect2.right, rect2.bottom);
    ID2D1RectangleGeometry_Release(geometry);

    set_rect(&rect, 50.0f, 100.0f, 40.0f, 50.0f);
    hr = ID2D1Factory_CreateRectangleGeometry(factory, &rect, &geometry);
    ok(SUCCEEDED(hr), "Failed to create geometry, hr %#x.\n", hr);
    ID2D1RectangleGeometry_GetRect(geometry, &rect2);
    match = compare_rect(&rect2, 50.0f, 100.0f, 40.0f, 50.0f, 0);
    ok(match, "Got unexpected rectangle {%.8e, %.8e, %.8e, %.8e}.\n",
            rect2.left, rect2.top, rect2.right, rect2.bottom);
    ID2D1RectangleGeometry_Release(geometry);

    set_rect(&rect, 0.0f, 0.0f, 10.0f, 20.0f);
    hr = ID2D1Factory_CreateRectangleGeometry(factory, &rect, &geometry);
    ok(SUCCEEDED(hr), "Failed to create geometry, hr %#x.\n", hr);

    /* Edge. */
    contains = FALSE;
    set_point(&point, 0.0f, 0.0f);
    hr = ID2D1RectangleGeometry_FillContainsPoint(geometry, point, NULL, 0.0f, &contains);
    ok(SUCCEEDED(hr), "FillContainsPoint() failed, hr %#x.\n", hr);
    ok(!!contains, "Got wrong hit test result %d.\n", contains);

    /* Within tolerance limit around corner. */
    contains = TRUE;
    set_point(&point, -D2D1_DEFAULT_FLATTENING_TOLERANCE, 0.0f);
    hr = ID2D1RectangleGeometry_FillContainsPoint(geometry, point, NULL, 0.0f, &contains);
    ok(SUCCEEDED(hr), "FillContainsPoint() failed, hr %#x.\n", hr);
    ok(!contains, "Got wrong hit test result %d.\n", contains);

    contains = FALSE;
    set_point(&point, -D2D1_DEFAULT_FLATTENING_TOLERANCE + 0.01f, 0.0f);
    hr = ID2D1RectangleGeometry_FillContainsPoint(geometry, point, NULL, 0.0f, &contains);
    ok(SUCCEEDED(hr), "FillContainsPoint() failed, hr %#x.\n", hr);
    ok(!!contains, "Got wrong hit test result %d.\n", contains);

    contains = TRUE;
    set_point(&point, -D2D1_DEFAULT_FLATTENING_TOLERANCE - 0.01f, 0.0f);
    hr = ID2D1RectangleGeometry_FillContainsPoint(geometry, point, NULL, 0.0f, &contains);
    ok(SUCCEEDED(hr), "FillContainsPoint() failed, hr %#x.\n", hr);
    ok(!contains, "Got wrong hit test result %d.\n", contains);

    contains = TRUE;
    set_point(&point, -D2D1_DEFAULT_FLATTENING_TOLERANCE, -D2D1_DEFAULT_FLATTENING_TOLERANCE);
    hr = ID2D1RectangleGeometry_FillContainsPoint(geometry, point, NULL, 0.0f, &contains);
    ok(SUCCEEDED(hr), "FillContainsPoint() failed, hr %#x.\n", hr);
    ok(!contains, "Got wrong hit test result %d.\n", contains);

    /* Inside. */
    contains = FALSE;
    set_point(&point, 5.0f, 5.0f);
    hr = ID2D1RectangleGeometry_FillContainsPoint(geometry, point, NULL, 0.0f, &contains);
    ok(SUCCEEDED(hr), "FillContainsPoint() failed, hr %#x.\n", hr);
    ok(!!contains, "Got wrong hit test result %d.\n", contains);

    /* Test GetBounds() and Simplify(). */
    hr = ID2D1RectangleGeometry_GetBounds(geometry, NULL, &rect);
    ok(SUCCEEDED(hr), "Failed to get bounds.\n");
    match = compare_rect(&rect, 0.0f, 0.0f, 10.0f, 20.0f, 0);
    ok(match, "Got unexpected bounds {%.8e, %.8e, %.8e, %.8e}.\n",
            rect.left, rect.top, rect.right, rect.bottom);
    geometry_sink_init(&sink);
    hr = ID2D1RectangleGeometry_Simplify(geometry, D2D1_GEOMETRY_SIMPLIFICATION_OPTION_CUBICS_AND_LINES,
            NULL, 0.0f, &sink.ID2D1SimplifiedGeometrySink_iface);
    ok(SUCCEEDED(hr), "Failed to simplify geometry, hr %#x.\n", hr);
    geometry_sink_check(&sink, D2D1_FILL_MODE_ALTERNATE, 1, &expected_figures[0], 0);
    geometry_sink_cleanup(&sink);
    geometry_sink_init(&sink);
    hr = ID2D1RectangleGeometry_Simplify(geometry, D2D1_GEOMETRY_SIMPLIFICATION_OPTION_LINES,
            NULL, 0.0f, &sink.ID2D1SimplifiedGeometrySink_iface);
    ok(SUCCEEDED(hr), "Failed to simplify geometry, hr %#x.\n", hr);
    geometry_sink_check(&sink, D2D1_FILL_MODE_ALTERNATE, 1, &expected_figures[0], 0);
    geometry_sink_cleanup(&sink);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 20.0f, 30.0f);
    scale_matrix(&matrix, 3.0f, 2.0f);
    rotate_matrix(&matrix, M_PI / -5.0f);
    hr = ID2D1RectangleGeometry_GetBounds(geometry, &matrix, &rect);
    ok(SUCCEEDED(hr), "Failed to get bounds.\n");
    match = compare_rect(&rect, 2.00000000e+01f, 1.82442951e+01f, 7.95376282e+01f, 6.23606796e+01f, 0);
    ok(match, "Got unexpected bounds {%.8e, %.8e, %.8e, %.8e}.\n",
            rect.left, rect.top, rect.right, rect.bottom);
    geometry_sink_init(&sink);
    hr = ID2D1RectangleGeometry_Simplify(geometry, D2D1_GEOMETRY_SIMPLIFICATION_OPTION_CUBICS_AND_LINES,
            &matrix, 0.0f, &sink.ID2D1SimplifiedGeometrySink_iface);
    ok(SUCCEEDED(hr), "Failed to simplify geometry, hr %#x.\n", hr);
    geometry_sink_check(&sink, D2D1_FILL_MODE_ALTERNATE, 1, &expected_figures[1], 1);
    geometry_sink_cleanup(&sink);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 25.0f, 15.0f);
    scale_matrix(&matrix, 0.0f, 2.0f);
    hr = ID2D1RectangleGeometry_GetBounds(geometry, &matrix, &rect);
    ok(SUCCEEDED(hr), "Failed to get bounds.\n");
    match = compare_rect(&rect, 25.0f, 15.0f, 25.0f, 55.0f, 0);
    ok(match, "Got unexpected bounds {%.8e, %.8e, %.8e, %.8e}.\n",
            rect.left, rect.top, rect.right, rect.bottom);
    geometry_sink_init(&sink);
    hr = ID2D1RectangleGeometry_Simplify(geometry, D2D1_GEOMETRY_SIMPLIFICATION_OPTION_LINES,
            &matrix, 0.0f, &sink.ID2D1SimplifiedGeometrySink_iface);
    ok(SUCCEEDED(hr), "Failed to simplify geometry, hr %#x.\n", hr);
    geometry_sink_check(&sink, D2D1_FILL_MODE_ALTERNATE, 1, &expected_figures[2], 0);
    geometry_sink_cleanup(&sink);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 30.0f, 45.0f);
    scale_matrix(&matrix, 0.5f, 0.0f);
    hr = ID2D1RectangleGeometry_GetBounds(geometry, &matrix, &rect);
    ok(SUCCEEDED(hr), "Failed to get bounds.\n");
    match = compare_rect(&rect, 30.0f, 45.0f, 35.0f, 45.0f, 0);
    ok(match, "Got unexpected bounds {%.8e, %.8e, %.8e, %.8e}.\n",
            rect.left, rect.top, rect.right, rect.bottom);
    geometry_sink_init(&sink);
    hr = ID2D1RectangleGeometry_Simplify(geometry, D2D1_GEOMETRY_SIMPLIFICATION_OPTION_LINES,
            &matrix, 0.0f, &sink.ID2D1SimplifiedGeometrySink_iface);
    ok(SUCCEEDED(hr), "Failed to simplify geometry, hr %#x.\n", hr);
    geometry_sink_check(&sink, D2D1_FILL_MODE_ALTERNATE, 1, &expected_figures[3], 0);
    geometry_sink_cleanup(&sink);

    set_matrix_identity(&matrix);
    scale_matrix(&matrix, 4.0f, 5.0f);
    rotate_matrix(&matrix, M_PI / 3.0f);
    translate_matrix(&matrix, 30.0f, 20.0f);
    hr = ID2D1Factory_CreateTransformedGeometry(factory, (ID2D1Geometry *)geometry, &matrix, &transformed_geometry);
    ok(SUCCEEDED(hr), "Failed to create transformed geometry, hr %#x.\n", hr);

    ID2D1TransformedGeometry_GetBounds(transformed_geometry, NULL, &rect);
    ok(SUCCEEDED(hr), "Failed to get bounds.\n");
    match = compare_rect(&rect, -7.85640717e+01f, 1.79903809e+02f, 1.07179594e+01f, 2.73205078e+02f, 1);
    ok(match, "Got unexpected bounds {%.8e, %.8e, %.8e, %.8e}.\n",
            rect.left, rect.top, rect.right, rect.bottom);
    geometry_sink_init(&sink);
    hr = ID2D1TransformedGeometry_Simplify(transformed_geometry, D2D1_GEOMETRY_SIMPLIFICATION_OPTION_CUBICS_AND_LINES,
            NULL, 0.0f, &sink.ID2D1SimplifiedGeometrySink_iface);
    ok(SUCCEEDED(hr), "Failed to simplify geometry, hr %#x.\n", hr);
    geometry_sink_check(&sink, D2D1_FILL_MODE_ALTERNATE, 1, &expected_figures[4], 1);
    geometry_sink_cleanup(&sink);
    geometry_sink_init(&sink);
    hr = ID2D1TransformedGeometry_Simplify(transformed_geometry, D2D1_GEOMETRY_SIMPLIFICATION_OPTION_LINES,
            NULL, 0.0f, &sink.ID2D1SimplifiedGeometrySink_iface);
    ok(SUCCEEDED(hr), "Failed to simplify geometry, hr %#x.\n", hr);
    geometry_sink_check(&sink, D2D1_FILL_MODE_ALTERNATE, 1, &expected_figures[4], 1);
    geometry_sink_cleanup(&sink);

    set_matrix_identity(&matrix);
    rotate_matrix(&matrix, M_PI / -3.0f);
    scale_matrix(&matrix, 0.25f, 0.2f);
    ID2D1TransformedGeometry_GetBounds(transformed_geometry, &matrix, &rect);
    ok(SUCCEEDED(hr), "Failed to get bounds.\n");
    match = compare_rect(&rect, 30.0f, 20.0f, 40.0f, 40.0f, 2);
    ok(match, "Got unexpected bounds {%.8e, %.8e, %.8e, %.8e}.\n",
            rect.left, rect.top, rect.right, rect.bottom);
    geometry_sink_init(&sink);
    hr = ID2D1TransformedGeometry_Simplify(transformed_geometry, D2D1_GEOMETRY_SIMPLIFICATION_OPTION_LINES,
            &matrix, 0.0f, &sink.ID2D1SimplifiedGeometrySink_iface);
    ok(SUCCEEDED(hr), "Failed to simplify geometry, hr %#x.\n", hr);
    geometry_sink_check(&sink, D2D1_FILL_MODE_ALTERNATE, 1, &expected_figures[5], 4);
    geometry_sink_cleanup(&sink);

    set_matrix_identity(&matrix);
    scale_matrix(&matrix, 2.0f, 0.0f);
    ID2D1TransformedGeometry_GetBounds(transformed_geometry, &matrix, &rect);
    ok(SUCCEEDED(hr), "Failed to get bounds.\n");
    match = compare_rect(&rect, -1.57128143e+02f, 0.00000000e+00f, 2.14359188e+01f, 0.00000000e+00f, 1);
    ok(match, "Got unexpected bounds {%.8e, %.8e, %.8e, %.8e}.\n",
            rect.left, rect.top, rect.right, rect.bottom);
    geometry_sink_init(&sink);
    hr = ID2D1TransformedGeometry_Simplify(transformed_geometry, D2D1_GEOMETRY_SIMPLIFICATION_OPTION_CUBICS_AND_LINES,
            &matrix, 0.0f, &sink.ID2D1SimplifiedGeometrySink_iface);
    ok(SUCCEEDED(hr), "Failed to simplify geometry, hr %#x.\n", hr);
    geometry_sink_check(&sink, D2D1_FILL_MODE_ALTERNATE, 1, &expected_figures[6], 1);
    geometry_sink_cleanup(&sink);

    set_matrix_identity(&matrix);
    scale_matrix(&matrix, 0.0f, 0.5f);
    ID2D1TransformedGeometry_GetBounds(transformed_geometry, &matrix, &rect);
    ok(SUCCEEDED(hr), "Failed to get bounds.\n");
    match = compare_rect(&rect, 0.00000000e+00f, 8.99519043e+01f, 0.00000000e+00, 1.36602539e+02f, 1);
    ok(match, "Got unexpected bounds {%.8e, %.8e, %.8e, %.8e}.\n",
            rect.left, rect.top, rect.right, rect.bottom);
    geometry_sink_init(&sink);
    hr = ID2D1TransformedGeometry_Simplify(transformed_geometry, D2D1_GEOMETRY_SIMPLIFICATION_OPTION_CUBICS_AND_LINES,
            &matrix, 0.0f, &sink.ID2D1SimplifiedGeometrySink_iface);
    ok(SUCCEEDED(hr), "Failed to simplify geometry, hr %#x.\n", hr);
    geometry_sink_check(&sink, D2D1_FILL_MODE_ALTERNATE, 1, &expected_figures[7], 1);
    geometry_sink_cleanup(&sink);

    ID2D1TransformedGeometry_Release(transformed_geometry);
    ID2D1RectangleGeometry_Release(geometry);
    ID2D1Factory_Release(factory);
}

static void test_rounded_rectangle_geometry(BOOL d3d11)
{
    ID2D1RoundedRectangleGeometry *geometry;
    D2D1_ROUNDED_RECT rect, rect2;
    ID2D1Factory *factory;
    HRESULT hr;

    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &IID_ID2D1Factory, NULL, (void **)&factory);
    ok(SUCCEEDED(hr), "Failed to create factory, hr %#x.\n", hr);

    set_rounded_rect(&rect, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    hr = ID2D1Factory_CreateRoundedRectangleGeometry(factory, &rect, &geometry);
    ok(SUCCEEDED(hr), "Failed to create geometry, hr %#x.\n", hr);

    ID2D1RoundedRectangleGeometry_GetRoundedRect(geometry, &rect2);
    ok(!memcmp(&rect, &rect2, sizeof(rect)), "Got unexpected rectangle {%.8e, %.8e, %.8e, %.8e, %.8e, %.8e}.\n",
            rect2.rect.left, rect2.rect.top, rect2.rect.right, rect2.rect.bottom, rect2.radiusX, rect2.radiusY);
    ID2D1RoundedRectangleGeometry_Release(geometry);

    /* X radius larger than half width. */
    set_rounded_rect(&rect, 0.0f, 0.0f, 50.0f, 40.0f, 30.0f, 5.0f);
    hr = ID2D1Factory_CreateRoundedRectangleGeometry(factory, &rect, &geometry);
    ID2D1RoundedRectangleGeometry_GetRoundedRect(geometry, &rect2);
    ok(!memcmp(&rect, &rect2, sizeof(rect)), "Got unexpected rectangle {%.8e, %.8e, %.8e, %.8e, %.8e, %.8e}.\n",
            rect2.rect.left, rect2.rect.top, rect2.rect.right, rect2.rect.bottom, rect2.radiusX, rect2.radiusY);
    ID2D1RoundedRectangleGeometry_Release(geometry);

    /* Y radius larger than half height. */
    set_rounded_rect(&rect, 0.0f, 0.0f, 50.0f, 40.0f, 5.0f, 30.0f);
    hr = ID2D1Factory_CreateRoundedRectangleGeometry(factory, &rect, &geometry);
    ID2D1RoundedRectangleGeometry_GetRoundedRect(geometry, &rect2);
    ok(!memcmp(&rect, &rect2, sizeof(rect)), "Got unexpected rectangle {%.8e, %.8e, %.8e, %.8e, %.8e, %.8e}.\n",
            rect2.rect.left, rect2.rect.top, rect2.rect.right, rect2.rect.bottom, rect2.radiusX, rect2.radiusY);
    ID2D1RoundedRectangleGeometry_Release(geometry);

    /* Both exceed rectangle size. */
    set_rounded_rect(&rect, 0.0f, 0.0f, 50.0f, 40.0f, 30.0f, 25.0f);
    hr = ID2D1Factory_CreateRoundedRectangleGeometry(factory, &rect, &geometry);
    ID2D1RoundedRectangleGeometry_GetRoundedRect(geometry, &rect2);
    ok(!memcmp(&rect, &rect2, sizeof(rect)), "Got unexpected rectangle {%.8e, %.8e, %.8e, %.8e, %.8e, %.8e}.\n",
            rect2.rect.left, rect2.rect.top, rect2.rect.right, rect2.rect.bottom, rect2.radiusX, rect2.radiusY);
    ID2D1RoundedRectangleGeometry_Release(geometry);

    ID2D1Factory_Release(factory);
}

static void test_bitmap_formats(BOOL d3d11)
{
    D2D1_BITMAP_PROPERTIES bitmap_desc;
    struct d2d1_test_context ctx;
    D2D1_SIZE_U size = {4, 4};
    ID2D1RenderTarget *rt;
    ID2D1Bitmap *bitmap;
    unsigned int i, j;
    HRESULT hr;

    static const struct
    {
        DXGI_FORMAT format;
        DWORD mask;
    }
    bitmap_formats[] =
    {
        {DXGI_FORMAT_R32G32B32A32_FLOAT,    0x8a},
        {DXGI_FORMAT_R16G16B16A16_FLOAT,    0x8a},
        {DXGI_FORMAT_R16G16B16A16_UNORM,    0x8a},
        {DXGI_FORMAT_R8G8B8A8_TYPELESS,     0x00},
        {DXGI_FORMAT_R8G8B8A8_UNORM,        0x0a},
        {DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,   0x8a},
        {DXGI_FORMAT_R8G8B8A8_UINT,         0x00},
        {DXGI_FORMAT_R8G8B8A8_SNORM,        0x00},
        {DXGI_FORMAT_R8G8B8A8_SINT,         0x00},
        {DXGI_FORMAT_A8_UNORM,              0x06},
        {DXGI_FORMAT_B8G8R8A8_UNORM,        0x0a},
        {DXGI_FORMAT_B8G8R8X8_UNORM,        0x88},
        {DXGI_FORMAT_B8G8R8A8_TYPELESS,     0x00},
        {DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,   0x8a},
    };

    if (!init_test_context(&ctx, d3d11))
        return;

    rt = ctx.rt;
    bitmap_desc.dpiX = 96.0f;
    bitmap_desc.dpiY = 96.0f;
    for (i = 0; i < ARRAY_SIZE(bitmap_formats); ++i)
    {
        for (j = 0; j < 4; ++j)
        {
            if ((bitmap_formats[i].mask & (0x80 | (1u << j))) == (0x80 | (1u << j)))
                continue;

            bitmap_desc.pixelFormat.format = bitmap_formats[i].format;
            bitmap_desc.pixelFormat.alphaMode = j;
            hr = ID2D1RenderTarget_CreateBitmap(rt, size, NULL, 0, &bitmap_desc, &bitmap);
            if (bitmap_formats[i].mask & (1u << j))
                ok(hr == S_OK, "Got unexpected hr %#x, for format %#x/%#x.\n",
                        hr, bitmap_formats[i].format, j);
            else
                ok(hr == D2DERR_UNSUPPORTED_PIXEL_FORMAT, "Got unexpected hr %#x, for format %#x/%#x.\n",
                        hr, bitmap_formats[i].format, j);
            if (SUCCEEDED(hr))
                ID2D1Bitmap_Release(bitmap);
        }
    }

    release_test_context(&ctx);
}

static void test_alpha_mode(BOOL d3d11)
{
    D2D1_RENDER_TARGET_PROPERTIES rt_desc;
    D2D1_BITMAP_PROPERTIES bitmap_desc;
    ID2D1SolidColorBrush *color_brush;
    ID2D1BitmapBrush *bitmap_brush;
    struct d2d1_test_context ctx;
    ID2D1RenderTarget *rt;
    ID2D1Bitmap *bitmap;
    D2D1_COLOR_F color;
    D2D1_RECT_F rect;
    D2D1_SIZE_U size;
    ULONG refcount;
    HRESULT hr;
    BOOL match;

    static const DWORD bitmap_data[] =
    {
        0x7f7f0000, 0x7f7f7f00, 0x7f007f00, 0x7f007f7f,
        0x7f00007f, 0x7f7f007f, 0x7f000000, 0x7f404040,
        0x7f7f7f7f, 0x7f7f7f7f, 0x7f7f7f7f, 0x7f000000,
        0x7f7f7f7f, 0x7f000000, 0x7f000000, 0x7f000000,
    };

    if (!init_test_context(&ctx, d3d11))
        return;

    rt = ctx.rt;
    ID2D1RenderTarget_SetAntialiasMode(rt, D2D1_ANTIALIAS_MODE_ALIASED);

    set_size_u(&size, 4, 4);
    bitmap_desc.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    bitmap_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;
    bitmap_desc.dpiX = 96.0f / 40.0f;
    bitmap_desc.dpiY = 96.0f / 30.0f;
    hr = ID2D1RenderTarget_CreateBitmap(rt, size, bitmap_data, 4 * sizeof(*bitmap_data), &bitmap_desc, &bitmap);
    ok(SUCCEEDED(hr), "Failed to create bitmap, hr %#x.\n", hr);

    hr = ID2D1RenderTarget_CreateBitmapBrush(rt, bitmap, NULL, NULL, &bitmap_brush);
    ok(SUCCEEDED(hr), "Failed to create brush, hr %#x.\n", hr);
    ID2D1BitmapBrush_SetInterpolationMode(bitmap_brush, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
    ID2D1BitmapBrush_SetExtendModeX(bitmap_brush, D2D1_EXTEND_MODE_WRAP);
    ID2D1BitmapBrush_SetExtendModeY(bitmap_brush, D2D1_EXTEND_MODE_WRAP);

    set_color(&color, 0.0f, 1.0f, 0.0f, 0.75f);
    hr = ID2D1RenderTarget_CreateSolidColorBrush(rt, &color, NULL, &color_brush);
    ok(SUCCEEDED(hr), "Failed to create brush, hr %#x.\n", hr);

    ID2D1RenderTarget_BeginDraw(rt);
    ID2D1RenderTarget_Clear(rt, NULL);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to end draw, hr %#x.\n", hr);
    match = compare_surface(&ctx, "48c41aff3a130a17ee210866b2ab7d36763934d5");
    ok(match, "Surface does not match.\n");

    ID2D1RenderTarget_BeginDraw(rt);
    set_color(&color, 1.0f, 0.0f, 0.0f, 0.25f);
    ID2D1RenderTarget_Clear(rt, &color);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to end draw, hr %#x.\n", hr);
    match = compare_surface(&ctx, "6487e683730fb5a77c1911388d00b04664c5c4e4");
    ok(match, "Surface does not match.\n");

    ID2D1RenderTarget_BeginDraw(rt);
    set_color(&color, 0.0f, 0.0f, 1.0f, 0.75f);
    ID2D1RenderTarget_Clear(rt, &color);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to end draw, hr %#x.\n", hr);
    match = compare_surface(&ctx, "7a35ba09e43cbaf591388ff1ef8de56157630c98");
    ok(match, "Surface does not match.\n");

    ID2D1RenderTarget_BeginDraw(rt);

    set_rect(&rect,   0.0f,   0.0f, 160.0f, 120.0f);
    ID2D1RenderTarget_FillRectangle(rt, &rect, (ID2D1Brush *)bitmap_brush);
    set_rect(&rect, 160.0f,   0.0f, 320.0f, 120.0f);
    ID2D1BitmapBrush_SetOpacity(bitmap_brush, 0.75f);
    ID2D1RenderTarget_FillRectangle(rt, &rect, (ID2D1Brush *)bitmap_brush);
    set_rect(&rect, 320.0f,   0.0f, 480.0f, 120.0f);
    ID2D1BitmapBrush_SetOpacity(bitmap_brush, 0.25f);
    ID2D1RenderTarget_FillRectangle(rt, &rect, (ID2D1Brush *)bitmap_brush);

    ID2D1Bitmap_Release(bitmap);
    bitmap_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    hr = ID2D1RenderTarget_CreateBitmap(rt, size, bitmap_data, 4 * sizeof(*bitmap_data), &bitmap_desc, &bitmap);
    ok(SUCCEEDED(hr), "Failed to create bitmap, hr %#x.\n", hr);
    ID2D1BitmapBrush_SetBitmap(bitmap_brush, bitmap);

    set_rect(&rect,   0.0f, 120.0f, 160.0f, 240.0f);
    ID2D1BitmapBrush_SetOpacity(bitmap_brush, 1.0f);
    ID2D1RenderTarget_FillRectangle(rt, &rect, (ID2D1Brush *)bitmap_brush);
    set_rect(&rect, 160.0f, 120.0f, 320.0f, 240.0f);
    ID2D1BitmapBrush_SetOpacity(bitmap_brush, 0.75f);
    ID2D1RenderTarget_FillRectangle(rt, &rect, (ID2D1Brush *)bitmap_brush);
    set_rect(&rect, 320.0f, 120.0f, 480.0f, 240.0f);
    ID2D1BitmapBrush_SetOpacity(bitmap_brush, 0.25f);
    ID2D1RenderTarget_FillRectangle(rt, &rect, (ID2D1Brush *)bitmap_brush);

    set_rect(&rect,   0.0f, 240.0f, 160.0f, 360.0f);
    ID2D1RenderTarget_FillRectangle(rt, &rect, (ID2D1Brush *)color_brush);
    set_rect(&rect, 160.0f, 240.0f, 320.0f, 360.0f);
    ID2D1SolidColorBrush_SetOpacity(color_brush, 0.75f);
    ID2D1RenderTarget_FillRectangle(rt, &rect, (ID2D1Brush *)color_brush);
    set_rect(&rect, 320.0f, 240.0f, 480.0f, 360.0f);
    ID2D1SolidColorBrush_SetOpacity(color_brush, 0.25f);
    ID2D1RenderTarget_FillRectangle(rt, &rect, (ID2D1Brush *)color_brush);

    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to end draw, hr %#x.\n", hr);
    match = compare_surface(&ctx, "14f8ac64b70966c7c3c6281c59aaecdb17c3b16a");
    ok(match, "Surface does not match.\n");

    rt_desc.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
    rt_desc.pixelFormat.format = DXGI_FORMAT_UNKNOWN;
    rt_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;
    rt_desc.dpiX = 0.0f;
    rt_desc.dpiY = 0.0f;
    rt_desc.usage = D2D1_RENDER_TARGET_USAGE_NONE;
    rt_desc.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;
    rt = create_render_target_desc(ctx.surface, &rt_desc, d3d11);
    ok(!!rt, "Failed to create render target.\n");

    ID2D1RenderTarget_SetAntialiasMode(rt, D2D1_ANTIALIAS_MODE_ALIASED);

    ID2D1Bitmap_Release(bitmap);
    bitmap_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;
    hr = ID2D1RenderTarget_CreateBitmap(rt, size, bitmap_data, 4 * sizeof(*bitmap_data), &bitmap_desc, &bitmap);
    ok(SUCCEEDED(hr), "Failed to create bitmap, hr %#x.\n", hr);
    ID2D1BitmapBrush_SetBitmap(bitmap_brush, bitmap);

    ID2D1BitmapBrush_Release(bitmap_brush);
    hr = ID2D1RenderTarget_CreateBitmapBrush(rt, bitmap, NULL, NULL, &bitmap_brush);
    ok(SUCCEEDED(hr), "Failed to create brush, hr %#x.\n", hr);
    ID2D1BitmapBrush_SetInterpolationMode(bitmap_brush, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
    ID2D1BitmapBrush_SetExtendModeX(bitmap_brush, D2D1_EXTEND_MODE_WRAP);
    ID2D1BitmapBrush_SetExtendModeY(bitmap_brush, D2D1_EXTEND_MODE_WRAP);

    ID2D1SolidColorBrush_Release(color_brush);
    set_color(&color, 0.0f, 1.0f, 0.0f, 0.75f);
    hr = ID2D1RenderTarget_CreateSolidColorBrush(rt, &color, NULL, &color_brush);
    ok(SUCCEEDED(hr), "Failed to create brush, hr %#x.\n", hr);

    ID2D1RenderTarget_BeginDraw(rt);
    ID2D1RenderTarget_Clear(rt, NULL);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to end draw, hr %#x.\n", hr);
    match = compare_surface(&ctx, "b44510bf2d2e61a8d7c0ad862de49a471f1fd13f");
    ok(match, "Surface does not match.\n");

    ID2D1RenderTarget_BeginDraw(rt);
    set_color(&color, 1.0f, 0.0f, 0.0f, 0.25f);
    ID2D1RenderTarget_Clear(rt, &color);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to end draw, hr %#x.\n", hr);
    match = compare_surface(&ctx, "2184f4a9198fc1de09ac85301b7a03eebadd9b81");
    ok(match, "Surface does not match.\n");

    ID2D1RenderTarget_BeginDraw(rt);
    set_color(&color, 0.0f, 0.0f, 1.0f, 0.75f);
    ID2D1RenderTarget_Clear(rt, &color);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to end draw, hr %#x.\n", hr);
    match = compare_surface(&ctx, "6527ec83b4039c895b50f9b3e144fe0cf90d1889");
    ok(match, "Surface does not match.\n");

    ID2D1RenderTarget_BeginDraw(rt);

    set_rect(&rect,   0.0f,   0.0f, 160.0f, 120.0f);
    ID2D1RenderTarget_FillRectangle(rt, &rect, (ID2D1Brush *)bitmap_brush);
    set_rect(&rect, 160.0f,   0.0f, 320.0f, 120.0f);
    ID2D1BitmapBrush_SetOpacity(bitmap_brush, 0.75f);
    ID2D1RenderTarget_FillRectangle(rt, &rect, (ID2D1Brush *)bitmap_brush);
    set_rect(&rect, 320.0f,   0.0f, 480.0f, 120.0f);
    ID2D1BitmapBrush_SetOpacity(bitmap_brush, 0.25f);
    ID2D1RenderTarget_FillRectangle(rt, &rect, (ID2D1Brush *)bitmap_brush);

    ID2D1Bitmap_Release(bitmap);
    bitmap_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    hr = ID2D1RenderTarget_CreateBitmap(rt, size, bitmap_data, 4 * sizeof(*bitmap_data), &bitmap_desc, &bitmap);
    ok(SUCCEEDED(hr), "Failed to create bitmap, hr %#x.\n", hr);
    ID2D1BitmapBrush_SetBitmap(bitmap_brush, bitmap);

    set_rect(&rect,   0.0f, 120.0f, 160.0f, 240.0f);
    ID2D1BitmapBrush_SetOpacity(bitmap_brush, 1.0f);
    ID2D1RenderTarget_FillRectangle(rt, &rect, (ID2D1Brush *)bitmap_brush);
    set_rect(&rect, 160.0f, 120.0f, 320.0f, 240.0f);
    ID2D1BitmapBrush_SetOpacity(bitmap_brush, 0.75f);
    ID2D1RenderTarget_FillRectangle(rt, &rect, (ID2D1Brush *)bitmap_brush);
    set_rect(&rect, 320.0f, 120.0f, 480.0f, 240.0f);
    ID2D1BitmapBrush_SetOpacity(bitmap_brush, 0.25f);
    ID2D1RenderTarget_FillRectangle(rt, &rect, (ID2D1Brush *)bitmap_brush);

    set_rect(&rect,   0.0f, 240.0f, 160.0f, 360.0f);
    ID2D1RenderTarget_FillRectangle(rt, &rect, (ID2D1Brush *)color_brush);
    set_rect(&rect, 160.0f, 240.0f, 320.0f, 360.0f);
    ID2D1SolidColorBrush_SetOpacity(color_brush, 0.75f);
    ID2D1RenderTarget_FillRectangle(rt, &rect, (ID2D1Brush *)color_brush);
    set_rect(&rect, 320.0f, 240.0f, 480.0f, 360.0f);
    ID2D1SolidColorBrush_SetOpacity(color_brush, 0.25f);
    ID2D1RenderTarget_FillRectangle(rt, &rect, (ID2D1Brush *)color_brush);

    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to end draw, hr %#x.\n", hr);
    match = compare_surface(&ctx, "465f5a3190d7bde408b3206b4be939fb22f8a3d6");
    ok(match, "Surface does not match.\n");

    refcount = ID2D1Bitmap_Release(bitmap);
    ok(refcount == 1, "Bitmap has %u references left.\n", refcount);
    ID2D1SolidColorBrush_Release(color_brush);
    ID2D1BitmapBrush_Release(bitmap_brush);
    ID2D1RenderTarget_Release(rt);
    release_test_context(&ctx);
}

static void test_shared_bitmap(BOOL d3d11)
{
    IWICBitmap *wic_bitmap1, *wic_bitmap2;
    ID2D1GdiInteropRenderTarget *interop;
    D2D1_RENDER_TARGET_PROPERTIES desc;
    D2D1_BITMAP_PROPERTIES bitmap_desc;
    ID2D1RenderTarget *rt1, *rt2, *rt3;
    IDXGISurface *surface2;
    ID2D1Factory *factory1, *factory2;
    IWICImagingFactory *wic_factory;
    ID2D1Bitmap *bitmap1, *bitmap2;
    DXGI_SURFACE_DESC surface_desc;
    D2D1_PIXEL_FORMAT pixel_format;
    struct d2d1_test_context ctx;
    IDXGISwapChain *swapchain2;
    D2D1_SIZE_U size = {4, 4};
    IDXGISurface1 *surface3;
    IDXGIDevice *device2;
    HWND window2;
    HRESULT hr;

    if (!init_test_context(&ctx, d3d11))
        return;

    window2 = create_window();
    swapchain2 = create_swapchain(ctx.device, window2, TRUE);
    hr = IDXGISwapChain_GetBuffer(swapchain2, 0, &IID_IDXGISurface, (void **)&surface2);
    ok(SUCCEEDED(hr), "Failed to get buffer, hr %#x.\n", hr);

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
            &IID_IWICImagingFactory, (void **)&wic_factory);
    ok(SUCCEEDED(hr), "Failed to create WIC imaging factory, hr %#x.\n", hr);
    hr = IWICImagingFactory_CreateBitmap(wic_factory, 640, 480,
            &GUID_WICPixelFormat32bppPBGRA, WICBitmapCacheOnDemand, &wic_bitmap1);
    ok(SUCCEEDED(hr), "Failed to create bitmap, hr %#x.\n", hr);
    hr = IWICImagingFactory_CreateBitmap(wic_factory, 640, 480,
            &GUID_WICPixelFormat32bppPBGRA, WICBitmapCacheOnDemand, &wic_bitmap2);
    ok(SUCCEEDED(hr), "Failed to create bitmap, hr %#x.\n", hr);
    IWICImagingFactory_Release(wic_factory);

    desc.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
    desc.pixelFormat.format = DXGI_FORMAT_UNKNOWN;
    desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    desc.dpiX = 0.0f;
    desc.dpiY = 0.0f;
    desc.usage = D2D1_RENDER_TARGET_USAGE_NONE;
    desc.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;

    bitmap_desc.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    bitmap_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    bitmap_desc.dpiX = 96.0f;
    bitmap_desc.dpiY = 96.0f;

    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &IID_ID2D1Factory, NULL, (void **)&factory1);
    ok(SUCCEEDED(hr), "Failed to create factory, hr %#x.\n", hr);
    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &IID_ID2D1Factory, NULL, (void **)&factory2);
    ok(SUCCEEDED(hr), "Failed to create factory, hr %#x.\n", hr);

    /* DXGI surface render targets with the same device and factory. */
    hr = ID2D1Factory_CreateDxgiSurfaceRenderTarget(factory1, ctx.surface, &desc, &rt1);
    ok(SUCCEEDED(hr), "Failed to create render target, hr %#x.\n", hr);
    hr = ID2D1RenderTarget_CreateBitmap(rt1, size, NULL, 0, &bitmap_desc, &bitmap1);
    check_bitmap_surface(bitmap1, TRUE, 0);
    ok(SUCCEEDED(hr), "Failed to create bitmap, hr %#x.\n", hr);

    hr = ID2D1Factory_CreateDxgiSurfaceRenderTarget(factory1, surface2, &desc, &rt2);
    ok(SUCCEEDED(hr), "Failed to create render target, hr %#x.\n", hr);
    hr = ID2D1RenderTarget_CreateSharedBitmap(rt2, &IID_ID2D1Bitmap, bitmap1, NULL, &bitmap2);
    ok(SUCCEEDED(hr), "Failed to create bitmap, hr %#x.\n", hr);
    check_bitmap_surface(bitmap2, TRUE, 0);
    ID2D1Bitmap_Release(bitmap2);
    hr = ID2D1RenderTarget_CreateSharedBitmap(rt2, &IID_IUnknown, bitmap1, NULL, &bitmap2);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    ID2D1RenderTarget_Release(rt2);

    /* DXGI surface render targets with the same device but different factories. */
    hr = ID2D1Factory_CreateDxgiSurfaceRenderTarget(factory2, surface2, &desc, &rt2);
    ok(SUCCEEDED(hr), "Failed to create render target, hr %#x.\n", hr);
    hr = ID2D1RenderTarget_CreateSharedBitmap(rt2, &IID_ID2D1Bitmap, bitmap1, NULL, &bitmap2);
    ok(hr == D2DERR_WRONG_FACTORY, "Got unexpected hr %#x.\n", hr);
    ID2D1RenderTarget_Release(rt2);

    /* DXGI surface render targets with different devices but the same factory. */
    IDXGISurface_Release(surface2);
    IDXGISwapChain_Release(swapchain2);
    device2 = create_device(d3d11);
    ok(!!device2, "Failed to create device.\n");
    swapchain2 = create_swapchain(device2, window2, TRUE);
    hr = IDXGISwapChain_GetBuffer(swapchain2, 0, &IID_IDXGISurface, (void **)&surface2);
    ok(SUCCEEDED(hr), "Failed to get buffer, hr %#x.\n", hr);

    hr = ID2D1Factory_CreateDxgiSurfaceRenderTarget(factory1, surface2, &desc, &rt2);
    ok(SUCCEEDED(hr), "Failed to create render target, hr %#x.\n", hr);
    hr = ID2D1RenderTarget_CreateSharedBitmap(rt2, &IID_ID2D1Bitmap, bitmap1, NULL, &bitmap2);
    ok(hr == D2DERR_UNSUPPORTED_OPERATION, "Got unexpected hr %#x.\n", hr);
    ID2D1RenderTarget_Release(rt2);

    /* DXGI surface render targets with different devices and different factories. */
    hr = ID2D1Factory_CreateDxgiSurfaceRenderTarget(factory2, surface2, &desc, &rt2);
    ok(SUCCEEDED(hr), "Failed to create render target, hr %#x.\n", hr);
    hr = ID2D1RenderTarget_CreateSharedBitmap(rt2, &IID_ID2D1Bitmap, bitmap1, NULL, &bitmap2);
    ok(hr == D2DERR_WRONG_FACTORY, "Got unexpected hr %#x.\n", hr);
    ID2D1RenderTarget_Release(rt2);

    /* DXGI surface render target and WIC bitmap render target, same factory. */
    hr = ID2D1Factory_CreateWicBitmapRenderTarget(factory1, wic_bitmap2, &desc, &rt2);
    ok(SUCCEEDED(hr), "Failed to create render target, hr %#x.\n", hr);
    hr = ID2D1RenderTarget_CreateSharedBitmap(rt2, &IID_ID2D1Bitmap, bitmap1, NULL, &bitmap2);
    ok(hr == D2DERR_UNSUPPORTED_OPERATION, "Got unexpected hr %#x.\n", hr);
    ID2D1RenderTarget_Release(rt2);

    /* WIC bitmap render targets on different D2D factories. */
    ID2D1Bitmap_Release(bitmap1);
    ID2D1RenderTarget_Release(rt1);
    hr = ID2D1Factory_CreateWicBitmapRenderTarget(factory1, wic_bitmap1, &desc, &rt1);
    ok(SUCCEEDED(hr), "Failed to create render target, hr %#x.\n", hr);
    hr = ID2D1RenderTarget_CreateBitmap(rt1, size, NULL, 0, &bitmap_desc, &bitmap1);
    ok(SUCCEEDED(hr), "Failed to create bitmap, hr %#x.\n", hr);

    hr = ID2D1RenderTarget_QueryInterface(rt1, &IID_ID2D1GdiInteropRenderTarget, (void **)&interop);
    ok(SUCCEEDED(hr), "Failed to get interop target, hr %#x.\n", hr);
    hr = ID2D1GdiInteropRenderTarget_QueryInterface(interop, &IID_ID2D1RenderTarget, (void **)&rt3);
    ok(SUCCEEDED(hr), "Failed to get render target back, %#x.\n", hr);
    ok(rt3 == rt1, "Unexpected render target\n");
    ID2D1RenderTarget_Release(rt3);
    ID2D1GdiInteropRenderTarget_Release(interop);

    hr = ID2D1Factory_CreateWicBitmapRenderTarget(factory2, wic_bitmap2, &desc, &rt2);
    ok(SUCCEEDED(hr), "Failed to create render target, hr %#x.\n", hr);
    hr = ID2D1RenderTarget_CreateSharedBitmap(rt2, &IID_ID2D1Bitmap, bitmap1, NULL, &bitmap2);
    ok(hr == D2DERR_WRONG_FACTORY, "Got unexpected hr %#x.\n", hr);
    ID2D1RenderTarget_Release(rt2);

    /* WIC bitmap render targets on the same D2D factory. */
    hr = ID2D1Factory_CreateWicBitmapRenderTarget(factory1, wic_bitmap2, &desc, &rt2);
    ok(SUCCEEDED(hr), "Failed to create render target, hr %#x.\n", hr);
    hr = ID2D1RenderTarget_CreateSharedBitmap(rt2, &IID_ID2D1Bitmap, bitmap1, NULL, &bitmap2);
    ok(SUCCEEDED(hr), "Failed to create bitmap, hr %#x.\n", hr);
    check_bitmap_surface(bitmap2, FALSE, 0);
    ID2D1Bitmap_Release(bitmap2);
    ID2D1RenderTarget_Release(rt2);

    /* Shared DXGI surface. */
    desc.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
    desc.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    desc.dpiX = 0.0f;
    desc.dpiY = 0.0f;
    desc.usage = D2D1_RENDER_TARGET_USAGE_NONE;
    desc.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;

    hr = ID2D1Factory_CreateDxgiSurfaceRenderTarget(factory1, surface2, &desc, &rt2);
    ok(SUCCEEDED(hr), "Failed to create render target, hr %#x.\n", hr);

    bitmap_desc.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    bitmap_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    bitmap_desc.dpiX = 0.0f;
    bitmap_desc.dpiY = 0.0f;

    hr = ID2D1RenderTarget_CreateSharedBitmap(rt2, &IID_IDXGISurface, surface2, &bitmap_desc, &bitmap2);
    ok(SUCCEEDED(hr) || broken(hr == E_INVALIDARG) /* vista */, "Failed to create bitmap, hr %#x.\n", hr);

    if (SUCCEEDED(hr))
    {
        static const struct bitmap_format_test
        {
            D2D1_PIXEL_FORMAT original;
            D2D1_PIXEL_FORMAT result;
            HRESULT hr;
        }
        bitmap_format_tests[] =
        {
            { { DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED },
              { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED } },

            { { DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_IGNORE },
              { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE } },

            { { DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_UNKNOWN }, { 0 }, WINCODEC_ERR_UNSUPPORTEDPIXELFORMAT },

            { { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_UNKNOWN }, { 0 }, WINCODEC_ERR_UNSUPPORTEDPIXELFORMAT },

            { { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE },
              { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE } },

            { { DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_STRAIGHT }, { 0 }, WINCODEC_ERR_UNSUPPORTEDPIXELFORMAT },
            { { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_STRAIGHT }, { 0 }, WINCODEC_ERR_UNSUPPORTEDPIXELFORMAT },
        };
        unsigned int i;

        size = ID2D1Bitmap_GetPixelSize(bitmap2);
        hr = IDXGISurface_GetDesc(surface2, &surface_desc);
        ok(SUCCEEDED(hr), "Failed to get surface description, hr %#x.\n", hr);
        ok(size.width == surface_desc.Width && size.height == surface_desc.Height, "Got wrong bitmap size.\n");

        check_bitmap_surface(bitmap2, TRUE, D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW);

        ID2D1Bitmap_Release(bitmap2);

        /* IDXGISurface1 is supported too. */
        if (IDXGISurface_QueryInterface(surface2, &IID_IDXGISurface1, (void **)&surface3) == S_OK)
        {
            hr = ID2D1RenderTarget_CreateSharedBitmap(rt2, &IID_IDXGISurface1, surface3, &bitmap_desc, &bitmap2);
            ok(SUCCEEDED(hr), "Failed to create bitmap, hr %#x.\n", hr);

            ID2D1Bitmap_Release(bitmap2);
            IDXGISurface1_Release(surface3);
        }

        for (i = 0; i < ARRAY_SIZE(bitmap_format_tests); ++i)
        {
            bitmap_desc.pixelFormat = bitmap_format_tests[i].original;

            hr = ID2D1RenderTarget_CreateSharedBitmap(rt2, &IID_IDXGISurface, surface2, &bitmap_desc, &bitmap2);
        todo_wine_if(i == 2 || i == 3 || i == 5 || i == 6)
            ok(hr == bitmap_format_tests[i].hr, "%u: unexpected hr %#x.\n", i, hr);

            if (SUCCEEDED(bitmap_format_tests[i].hr))
            {
                pixel_format = ID2D1Bitmap_GetPixelFormat(bitmap2);
                ok(pixel_format.format == bitmap_format_tests[i].result.format, "%u: unexpected pixel format %#x.\n",
                        i, pixel_format.format);
                ok(pixel_format.alphaMode == bitmap_format_tests[i].result.alphaMode, "%u: unexpected alpha mode %d.\n",
                        i, pixel_format.alphaMode);

                ID2D1Bitmap_Release(bitmap2);
            }
        }
    }

    ID2D1RenderTarget_Release(rt2);

    ID2D1Bitmap_Release(bitmap1);
    ID2D1RenderTarget_Release(rt1);
    ID2D1Factory_Release(factory2);
    ID2D1Factory_Release(factory1);
    IWICBitmap_Release(wic_bitmap2);
    IWICBitmap_Release(wic_bitmap1);
    IDXGISurface_Release(surface2);
    IDXGISwapChain_Release(swapchain2);
    IDXGIDevice_Release(device2);
    release_test_context(&ctx);
    DestroyWindow(window2);
    CoUninitialize();
}

static void test_bitmap_updates(BOOL d3d11)
{
    D2D1_BITMAP_PROPERTIES bitmap_desc;
    struct d2d1_test_context ctx;
    ID2D1RenderTarget *rt;
    D2D1_RECT_U dst_rect;
    ID2D1Bitmap *bitmap;
    D2D1_COLOR_F color;
    D2D1_RECT_F rect;
    D2D1_SIZE_U size;
    HRESULT hr;
    BOOL match;

    static const DWORD bitmap_data[] =
    {
        0xffff0000, 0xffffff00, 0xff00ff00, 0xff00ffff,
        0xff0000ff, 0xffff00ff, 0xff000000, 0xff7f7f7f,
        0xffffffff, 0xffffffff, 0xffffffff, 0xff000000,
        0xffffffff, 0xff000000, 0xff000000, 0xff000000,
    };

    if (!init_test_context(&ctx, d3d11))
        return;

    rt = ctx.rt;
    ID2D1RenderTarget_SetAntialiasMode(rt, D2D1_ANTIALIAS_MODE_ALIASED);

    ID2D1RenderTarget_BeginDraw(rt);
    set_color(&color, 0.0f, 0.0f, 1.0f, 1.0f);
    ID2D1RenderTarget_Clear(rt, &color);

    set_size_u(&size, 4, 4);
    bitmap_desc.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    bitmap_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    bitmap_desc.dpiX = 96.0f;
    bitmap_desc.dpiY = 96.0f;
    hr = ID2D1RenderTarget_CreateBitmap(rt, size, NULL, 0, &bitmap_desc, &bitmap);
    ok(SUCCEEDED(hr), "Failed to create bitmap, hr %#x.\n", hr);

    set_rect(&rect, 0.0f, 0.0f, 320.0f, 240.0f);
    ID2D1RenderTarget_DrawBitmap(rt, bitmap, &rect, 1.0f,
            D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, NULL);

    ID2D1Bitmap_Release(bitmap);

    bitmap_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;
    hr = ID2D1RenderTarget_CreateBitmap(rt, size, NULL, 0, &bitmap_desc, &bitmap);
    ok(SUCCEEDED(hr), "Failed to create bitmap, hr %#x.\n", hr);

    set_rect(&rect, 0.0f, 240.0f, 320.0f, 480.0f);
    ID2D1RenderTarget_DrawBitmap(rt, bitmap, &rect, 1.0f,
            D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, NULL);

    set_rect_u(&dst_rect, 1, 1, 3, 3);
    hr = ID2D1Bitmap_CopyFromMemory(bitmap, &dst_rect, bitmap_data, 4 * sizeof(*bitmap_data));
    ok(SUCCEEDED(hr), "Failed to update bitmap, hr %#x.\n", hr);
    set_rect_u(&dst_rect, 0, 3, 3, 4);
    hr = ID2D1Bitmap_CopyFromMemory(bitmap, &dst_rect, &bitmap_data[6], 4 * sizeof(*bitmap_data));
    ok(SUCCEEDED(hr), "Failed to update bitmap, hr %#x.\n", hr);
    set_rect_u(&dst_rect, 0, 0, 4, 1);
    hr = ID2D1Bitmap_CopyFromMemory(bitmap, &dst_rect, &bitmap_data[10], 4 * sizeof(*bitmap_data));
    ok(SUCCEEDED(hr), "Failed to update bitmap, hr %#x.\n", hr);
    set_rect_u(&dst_rect, 0, 1, 1, 3);
    hr = ID2D1Bitmap_CopyFromMemory(bitmap, &dst_rect, &bitmap_data[2], sizeof(*bitmap_data));
    ok(SUCCEEDED(hr), "Failed to update bitmap, hr %#x.\n", hr);
    set_rect_u(&dst_rect, 4, 4, 3, 1);
    hr = ID2D1Bitmap_CopyFromMemory(bitmap, &dst_rect, bitmap_data, sizeof(*bitmap_data));
    ok(SUCCEEDED(hr), "Failed to update bitmap, hr %#x.\n", hr);
    set_rect(&rect, 320.0f, 240.0f, 640.0f, 480.0f);
    ID2D1RenderTarget_DrawBitmap(rt, bitmap, &rect, 1.0f,
            D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, NULL);

    hr = ID2D1Bitmap_CopyFromMemory(bitmap, NULL, bitmap_data, 4 * sizeof(*bitmap_data));
    ok(SUCCEEDED(hr), "Failed to update bitmap, hr %#x.\n", hr);
    set_rect(&rect, 320.0f, 0.0f, 640.0f, 240.0f);
    ID2D1RenderTarget_DrawBitmap(rt, bitmap, &rect, 1.0f,
            D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, NULL);

    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to end draw, hr %#x.\n", hr);

    match = compare_surface(&ctx, "cb8136c91fbbdc76bb83b8c09edc1907b0a5d0a6");
    ok(match, "Surface does not match.\n");

    ID2D1Bitmap_Release(bitmap);
    release_test_context(&ctx);
}

static void test_opacity_brush(BOOL d3d11)
{
    ID2D1BitmapBrush *bitmap_brush, *opacity_brush;
    D2D1_BITMAP_PROPERTIES bitmap_desc;
    ID2D1RectangleGeometry *geometry;
    ID2D1SolidColorBrush *color_brush;
    struct d2d1_test_context ctx;
    D2D1_MATRIX_3X2_F matrix;
    ID2D1RenderTarget *rt;
    ID2D1Factory *factory;
    ID2D1Bitmap *bitmap;
    D2D1_COLOR_F color;
    D2D1_RECT_F rect;
    D2D1_SIZE_U size;
    ULONG refcount;
    HRESULT hr;
    BOOL match;

    static const DWORD bitmap_data[] =
    {
        0xffff0000, 0x40ffff00, 0x4000ff00, 0xff00ffff,
        0x7f0000ff, 0x00ff00ff, 0x00000000, 0x7f7f7f7f,
        0x7fffffff, 0x00ffffff, 0x00ffffff, 0x7f000000,
        0xffffffff, 0x40000000, 0x40000000, 0xff000000,
    };

    if (!init_test_context(&ctx, d3d11))
        return;

    rt = ctx.rt;
    ID2D1RenderTarget_GetFactory(rt, &factory);

    ID2D1RenderTarget_SetDpi(rt, 192.0f, 48.0f);
    ID2D1RenderTarget_SetAntialiasMode(rt, D2D1_ANTIALIAS_MODE_ALIASED);

    set_color(&color, 0.0f, 1.0f, 0.0f, 0.8f);
    hr = ID2D1RenderTarget_CreateSolidColorBrush(rt, &color, NULL, &color_brush);
    ok(SUCCEEDED(hr), "Failed to create color brush, hr %#x.\n", hr);

    set_size_u(&size, 4, 4);
    bitmap_desc.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    bitmap_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    bitmap_desc.dpiX = 96.0f;
    bitmap_desc.dpiY = 96.0f;
    hr = ID2D1RenderTarget_CreateBitmap(rt, size, bitmap_data, 4 * sizeof(*bitmap_data), &bitmap_desc, &bitmap);
    ok(SUCCEEDED(hr), "Failed to create bitmap, hr %#x.\n", hr);
    hr = ID2D1RenderTarget_CreateBitmapBrush(rt, bitmap, NULL, NULL, &opacity_brush);
    ok(SUCCEEDED(hr), "Failed to create bitmap brush, hr %#x.\n", hr);
    ID2D1BitmapBrush_SetInterpolationMode(opacity_brush, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
    refcount = ID2D1Bitmap_Release(bitmap);
    ok(refcount == 1, "Got unexpected refcount %u.\n", refcount);

    set_size_u(&size, 1, 1);
    bitmap_desc.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    bitmap_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;
    bitmap_desc.dpiX = 96.0f;
    bitmap_desc.dpiY = 96.0f;
    hr = ID2D1RenderTarget_CreateBitmap(rt, size, &bitmap_data[2], sizeof(*bitmap_data), &bitmap_desc, &bitmap);
    ok(SUCCEEDED(hr), "Failed to create bitmap, hr %#x.\n", hr);
    hr = ID2D1RenderTarget_CreateBitmapBrush(rt, bitmap, NULL, NULL, &bitmap_brush);
    ok(SUCCEEDED(hr), "Failed to create bitmap brush, hr %#x.\n", hr);
    ID2D1BitmapBrush_SetInterpolationMode(bitmap_brush, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
    refcount = ID2D1Bitmap_Release(bitmap);
    ok(refcount == 1, "Got unexpected refcount %u.\n", refcount);

    ID2D1RenderTarget_BeginDraw(rt);

    set_color(&color, 0.0f, 0.0f, 1.0f, 1.0f);
    ID2D1RenderTarget_Clear(rt, &color);

    set_rect(&rect, 40.0f, 120.0f, 120.0f, 360.0f);
    ID2D1RenderTarget_FillRectangle(rt, &rect, (ID2D1Brush *)color_brush);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 120.0f, 120.0f);
    scale_matrix(&matrix, 20.0f, 60.0f);
    ID2D1BitmapBrush_SetTransform(opacity_brush, &matrix);
    set_rect(&rect, 120.0f, 120.0f, 200.0f, 360.0f);
    ID2D1RenderTarget_FillRectangle(rt, &rect, (ID2D1Brush *)opacity_brush);

    set_rect(&rect, 200.0f, 120.0f, 280.0f, 360.0f);
    ID2D1RenderTarget_FillRectangle(rt, &rect, (ID2D1Brush *)bitmap_brush);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 40.0f, 360.0f);
    scale_matrix(&matrix, 20.0f, 60.0f);
    ID2D1BitmapBrush_SetTransform(opacity_brush, &matrix);
    set_rect(&rect, 40.0f, 360.0f, 120.0f, 600.0f);
    ID2D1Factory_CreateRectangleGeometry(factory, &rect, &geometry);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)geometry,
            (ID2D1Brush *)opacity_brush, (ID2D1Brush *)color_brush);
    ID2D1RectangleGeometry_Release(geometry);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 120.0f, 360.0f);
    scale_matrix(&matrix, 20.0f, 60.0f);
    ID2D1BitmapBrush_SetTransform(opacity_brush, &matrix);
    set_rect(&rect, 120.0f, 360.0f, 200.0f, 600.0f);
    ID2D1Factory_CreateRectangleGeometry(factory, &rect, &geometry);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)geometry,
            (ID2D1Brush *)color_brush, (ID2D1Brush *)opacity_brush);
    ID2D1RectangleGeometry_Release(geometry);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 200.0f, 360.0f);
    scale_matrix(&matrix, 20.0f, 60.0f);
    ID2D1BitmapBrush_SetTransform(opacity_brush, &matrix);
    set_rect(&rect, 200.0f, 360.0f, 280.0f, 600.0f);
    ID2D1Factory_CreateRectangleGeometry(factory, &rect, &geometry);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)geometry,
            (ID2D1Brush *)bitmap_brush, (ID2D1Brush *)opacity_brush);

    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == D2DERR_INCOMPATIBLE_BRUSH_TYPES, "Got unexpected hr %#x.\n", hr);
    match = compare_surface(&ctx, "7141c6c7b3decb91196428efb1856bcbf9872935");
    ok(match, "Surface does not match.\n");
    ID2D1RenderTarget_BeginDraw(rt);

    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)geometry,
            (ID2D1Brush *)bitmap_brush, (ID2D1Brush *)opacity_brush);
    ID2D1RectangleGeometry_Release(geometry);

    ID2D1SolidColorBrush_SetOpacity(color_brush, 0.5f);
    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 40.0f, 600.0f);
    scale_matrix(&matrix, 20.0f, 60.0f);
    ID2D1BitmapBrush_SetTransform(opacity_brush, &matrix);
    set_rect(&rect, 40.0f, 600.0f, 120.0f, 840.0f);
    ID2D1Factory_CreateRectangleGeometry(factory, &rect, &geometry);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)geometry,
            (ID2D1Brush *)opacity_brush, (ID2D1Brush *)color_brush);
    ID2D1RectangleGeometry_Release(geometry);

    ID2D1BitmapBrush_SetOpacity(opacity_brush, 0.8f);
    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 120.0f, 600.0f);
    scale_matrix(&matrix, 20.0f, 60.0f);
    ID2D1BitmapBrush_SetTransform(opacity_brush, &matrix);
    set_rect(&rect, 120.0f, 600.0f, 200.0f, 840.0f);
    ID2D1Factory_CreateRectangleGeometry(factory, &rect, &geometry);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)geometry,
            (ID2D1Brush *)opacity_brush, (ID2D1Brush *)bitmap_brush);
    ID2D1RectangleGeometry_Release(geometry);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 200.0f, 600.0f);
    scale_matrix(&matrix, 20.0f, 60.0f);
    ID2D1BitmapBrush_SetTransform(opacity_brush, &matrix);
    set_rect(&rect, 200.0f, 600.0f, 280.0f, 840.0f);
    ID2D1Factory_CreateRectangleGeometry(factory, &rect, &geometry);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)geometry,
            (ID2D1Brush *)bitmap_brush, (ID2D1Brush *)opacity_brush);
    ID2D1RectangleGeometry_Release(geometry);

    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to end draw, hr %#x.\n", hr);
    match = compare_surface(&ctx, "c3a5802d1750efa3e9122c1a92f6064df3872732");
    ok(match, "Surface does not match.\n");

    ID2D1BitmapBrush_Release(bitmap_brush);
    ID2D1BitmapBrush_Release(opacity_brush);
    ID2D1SolidColorBrush_Release(color_brush);
    ID2D1Factory_Release(factory);
    release_test_context(&ctx);
}

static void test_create_target(BOOL d3d11)
{
    struct d2d1_test_context ctx;
    ID2D1Factory *factory;
    ID2D1RenderTarget *rt;
    HRESULT hr;
    static const struct
    {
        float dpi_x, dpi_y;
        float rt_dpi_x, rt_dpi_y;
        HRESULT hr;
    }
    create_dpi_tests[] =
    {
        {   0.0f,   0.0f, 96.0f, 96.0f, S_OK },
        { 192.0f,   0.0f, 96.0f, 96.0f, E_INVALIDARG },
        {   0.0f, 192.0f, 96.0f, 96.0f, E_INVALIDARG },
        { 192.0f, -10.0f, 96.0f, 96.0f, E_INVALIDARG },
        { -10.0f, 192.0f, 96.0f, 96.0f, E_INVALIDARG },
        {  48.0f,  96.0f, 48.0f, 96.0f, S_OK },
    };
    unsigned int i;

    if (!init_test_context(&ctx, d3d11))
        return;

    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &IID_ID2D1Factory, NULL, (void **)&factory);
    ok(SUCCEEDED(hr), "Failed to create factory, hr %#x.\n", hr);

    for (i = 0; i < ARRAY_SIZE(create_dpi_tests); ++i)
    {
        ID2D1GdiInteropRenderTarget *interop;
        D2D1_RENDER_TARGET_PROPERTIES desc;
        ID2D1RenderTarget *rt2;
        float dpi_x, dpi_y;
        IUnknown *unk;

        desc.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
        desc.pixelFormat.format = DXGI_FORMAT_UNKNOWN;
        desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
        desc.dpiX = create_dpi_tests[i].dpi_x;
        desc.dpiY = create_dpi_tests[i].dpi_y;
        desc.usage = D2D1_RENDER_TARGET_USAGE_NONE;
        desc.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;

        hr = ID2D1Factory_CreateDxgiSurfaceRenderTarget(factory, ctx.surface, &desc, &rt);
        ok(hr == create_dpi_tests[i].hr, "Wrong return code, hr %#x, expected %#x, test %u.\n", hr,
            create_dpi_tests[i].hr, i);

        if (FAILED(hr))
            continue;

        hr = ID2D1RenderTarget_QueryInterface(rt, &IID_IUnknown, (void **)&unk);
        ok(SUCCEEDED(hr), "Failed to get IUnknown, hr %#x.\n", hr);
        ok(unk == (IUnknown *)rt, "Expected same interface pointer.\n");
        IUnknown_Release(unk);

        hr = ID2D1RenderTarget_QueryInterface(rt, &IID_ID2D1GdiInteropRenderTarget, (void **)&interop);
        ok(SUCCEEDED(hr), "Failed to get interop target, hr %#x.\n", hr);
        hr = ID2D1GdiInteropRenderTarget_QueryInterface(interop, &IID_ID2D1RenderTarget, (void **)&rt2);
        ok(SUCCEEDED(hr), "Failed to get render target back, %#x.\n", hr);
        ok(rt2 == rt, "Unexpected render target\n");
        ID2D1RenderTarget_Release(rt2);
        ID2D1GdiInteropRenderTarget_Release(interop);

        ID2D1RenderTarget_GetDpi(rt, &dpi_x, &dpi_y);
        ok(dpi_x == create_dpi_tests[i].rt_dpi_x, "Wrong dpi_x %.8e, expected %.8e, test %u\n",
            dpi_x, create_dpi_tests[i].rt_dpi_x, i);
        ok(dpi_y == create_dpi_tests[i].rt_dpi_y, "Wrong dpi_y %.8e, expected %.8e, test %u\n",
            dpi_y, create_dpi_tests[i].rt_dpi_y, i);

        ID2D1RenderTarget_Release(rt);
    }

    ID2D1Factory_Release(factory);
    release_test_context(&ctx);
}

static void test_draw_text_layout(BOOL d3d11)
{
    static const struct
    {
        D2D1_TEXT_ANTIALIAS_MODE aa_mode;
        DWRITE_RENDERING_MODE rendering_mode;
        HRESULT hr;
    }
    antialias_mode_tests[] =
    {
        { D2D1_TEXT_ANTIALIAS_MODE_DEFAULT, DWRITE_RENDERING_MODE_ALIASED },
        { D2D1_TEXT_ANTIALIAS_MODE_ALIASED, DWRITE_RENDERING_MODE_ALIASED },
        { D2D1_TEXT_ANTIALIAS_MODE_ALIASED, DWRITE_RENDERING_MODE_DEFAULT },
        { D2D1_TEXT_ANTIALIAS_MODE_ALIASED, DWRITE_RENDERING_MODE_OUTLINE },
        { D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE, DWRITE_RENDERING_MODE_DEFAULT },
        { D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE, DWRITE_RENDERING_MODE_OUTLINE },
        { D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE, DWRITE_RENDERING_MODE_CLEARTYPE_NATURAL },
        { D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE, DWRITE_RENDERING_MODE_CLEARTYPE_NATURAL_SYMMETRIC },
        { D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE, DWRITE_RENDERING_MODE_CLEARTYPE_GDI_NATURAL },
        { D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE, DWRITE_RENDERING_MODE_CLEARTYPE_GDI_CLASSIC },
        { D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE, DWRITE_RENDERING_MODE_DEFAULT },
        { D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE, DWRITE_RENDERING_MODE_CLEARTYPE_NATURAL },
        { D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE, DWRITE_RENDERING_MODE_CLEARTYPE_NATURAL_SYMMETRIC },
        { D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE, DWRITE_RENDERING_MODE_CLEARTYPE_GDI_NATURAL },
        { D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE, DWRITE_RENDERING_MODE_CLEARTYPE_GDI_CLASSIC },
        { D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE, DWRITE_RENDERING_MODE_OUTLINE, E_INVALIDARG },
        { D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE, DWRITE_RENDERING_MODE_ALIASED, E_INVALIDARG },
        { D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE, DWRITE_RENDERING_MODE_ALIASED, E_INVALIDARG },
        { D2D1_TEXT_ANTIALIAS_MODE_ALIASED, DWRITE_RENDERING_MODE_CLEARTYPE_NATURAL, E_INVALIDARG },
        { D2D1_TEXT_ANTIALIAS_MODE_ALIASED, DWRITE_RENDERING_MODE_CLEARTYPE_NATURAL_SYMMETRIC, E_INVALIDARG },
        { D2D1_TEXT_ANTIALIAS_MODE_ALIASED, DWRITE_RENDERING_MODE_CLEARTYPE_GDI_NATURAL, E_INVALIDARG },
        { D2D1_TEXT_ANTIALIAS_MODE_ALIASED, DWRITE_RENDERING_MODE_CLEARTYPE_GDI_CLASSIC, E_INVALIDARG },
    };
    D2D1_RENDER_TARGET_PROPERTIES desc;
    ID2D1Factory *factory, *factory2;
    ID2D1RenderTarget *rt, *rt2;
    HRESULT hr;
    IDWriteFactory *dwrite_factory;
    IDWriteTextFormat *text_format;
    IDWriteTextLayout *text_layout;
    struct d2d1_test_context ctx;
    D2D1_POINT_2F origin;
    DWRITE_TEXT_RANGE range;
    D2D1_COLOR_F color;
    ID2D1SolidColorBrush *brush, *brush2;
    ID2D1RectangleGeometry *geometry;
    D2D1_RECT_F rect;
    unsigned int i;

    if (!init_test_context(&ctx, d3d11))
        return;


    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &IID_ID2D1Factory, NULL, (void **)&factory);
    ok(SUCCEEDED(hr), "Failed to create factory, hr %#x.\n", hr);

    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &IID_ID2D1Factory, NULL, (void **)&factory2);
    ok(SUCCEEDED(hr), "Failed to create factory, hr %#x.\n", hr);
    ok(factory != factory2, "got same factory\n");

    desc.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
    desc.pixelFormat.format = DXGI_FORMAT_UNKNOWN;
    desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    desc.dpiX = 0.0f;
    desc.dpiY = 0.0f;
    desc.usage = D2D1_RENDER_TARGET_USAGE_NONE;
    desc.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;

    hr = ID2D1Factory_CreateDxgiSurfaceRenderTarget(factory, ctx.surface, &desc, &rt);
    ok(SUCCEEDED(hr), "Failed to create a target, hr %#x.\n", hr);

    hr = ID2D1Factory_CreateDxgiSurfaceRenderTarget(factory2, ctx.surface, &desc, &rt2);
    ok(SUCCEEDED(hr), "Failed to create a target, hr %#x.\n", hr);

    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, &IID_IDWriteFactory, (IUnknown **)&dwrite_factory);
    ok(SUCCEEDED(hr), "Failed to create factory, hr %#x.\n", hr);

    hr = IDWriteFactory_CreateTextFormat(dwrite_factory, L"Tahoma", NULL, DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 10.0f, L"", &text_format);
    ok(SUCCEEDED(hr), "Failed to create text format, hr %#x.\n", hr);

    hr = IDWriteFactory_CreateTextLayout(dwrite_factory, L"text", 4, text_format, 100.0f, 100.0f, &text_layout);
    ok(SUCCEEDED(hr), "Failed to create text layout, hr %#x.\n", hr);

    set_color(&color, 0.0f, 0.0f, 0.0f, 0.0f);
    hr = ID2D1RenderTarget_CreateSolidColorBrush(rt, &color, NULL, &brush);
    ok(SUCCEEDED(hr), "Failed to create brush, hr %#x.\n", hr);

    hr = ID2D1RenderTarget_CreateSolidColorBrush(rt2, &color, NULL, &brush2);
    ok(SUCCEEDED(hr), "Failed to create brush, hr %#x.\n", hr);

    /* effect brush is created from different factory */
    range.startPosition = 0;
    range.length = 4;
    hr = IDWriteTextLayout_SetDrawingEffect(text_layout, (IUnknown*)brush2, range);
    ok(SUCCEEDED(hr), "Failed to set drawing effect, hr %#x.\n", hr);

    ID2D1RenderTarget_BeginDraw(rt);

    origin.x = origin.y = 0.0f;
    ID2D1RenderTarget_DrawTextLayout(rt, origin, text_layout, (ID2D1Brush*)brush, D2D1_DRAW_TEXT_OPTIONS_NONE);

    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
todo_wine
    ok(hr == D2DERR_WRONG_FACTORY, "Unexpected hr %#x.\n", hr);

    /* Effect is d2d resource, but not a brush. */
    set_rect(&rect, 0.0f, 0.0f, 10.0f, 10.0f);
    hr = ID2D1Factory_CreateRectangleGeometry(factory, &rect, &geometry);
    ok(SUCCEEDED(hr), "Failed to geometry, hr %#x.\n", hr);

    range.startPosition = 0;
    range.length = 4;
    hr = IDWriteTextLayout_SetDrawingEffect(text_layout, (IUnknown*)geometry, range);
    ok(SUCCEEDED(hr), "Failed to set drawing effect, hr %#x.\n", hr);
    ID2D1RectangleGeometry_Release(geometry);

    ID2D1RenderTarget_BeginDraw(rt);

    origin.x = origin.y = 0.0f;
    ID2D1RenderTarget_DrawTextLayout(rt, origin, text_layout, (ID2D1Brush*)brush, D2D1_DRAW_TEXT_OPTIONS_NONE);

    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(SUCCEEDED(hr), "EndDraw() failed, hr %#x.\n", hr);

    for (i = 0; i < ARRAY_SIZE(antialias_mode_tests); ++i)
    {
        IDWriteRenderingParams *rendering_params;

        ID2D1RenderTarget_SetTextAntialiasMode(rt, antialias_mode_tests[i].aa_mode);

        hr = IDWriteFactory_CreateCustomRenderingParams(dwrite_factory, 2.0f, 1.0f, 0.0f, DWRITE_PIXEL_GEOMETRY_FLAT,
                antialias_mode_tests[i].rendering_mode, &rendering_params);
        ok(SUCCEEDED(hr), "Failed to create custom rendering params, hr %#x.\n", hr);

        ID2D1RenderTarget_SetTextRenderingParams(rt, rendering_params);

        ID2D1RenderTarget_BeginDraw(rt);

        ID2D1RenderTarget_DrawTextLayout(rt, origin, text_layout, (ID2D1Brush *)brush, D2D1_DRAW_TEXT_OPTIONS_NONE);

        hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
        ok(hr == antialias_mode_tests[i].hr, "%u: unexpected hr %#x.\n", i, hr);

        IDWriteRenderingParams_Release(rendering_params);
    }

    IDWriteTextFormat_Release(text_format);
    IDWriteTextLayout_Release(text_layout);
    IDWriteFactory_Release(dwrite_factory);
    ID2D1RenderTarget_Release(rt);
    ID2D1RenderTarget_Release(rt2);

    ID2D1Factory_Release(factory);
    ID2D1Factory_Release(factory2);
    release_test_context(&ctx);
}

static void create_target_dibsection(HDC hdc, UINT32 width, UINT32 height)
{
    char bmibuf[FIELD_OFFSET(BITMAPINFO, bmiColors[256])];
    BITMAPINFO *bmi = (BITMAPINFO*)bmibuf;
    HBITMAP hbm;

    memset(bmi, 0, sizeof(bmibuf));
    bmi->bmiHeader.biSize = sizeof(bmi->bmiHeader);
    bmi->bmiHeader.biHeight = -height;
    bmi->bmiHeader.biWidth = width;
    bmi->bmiHeader.biBitCount = 32;
    bmi->bmiHeader.biPlanes = 1;
    bmi->bmiHeader.biCompression = BI_RGB;

    hbm = CreateDIBSection(hdc, bmi, DIB_RGB_COLORS, NULL, NULL, 0);
    ok(hbm != NULL, "Failed to create a dib section.\n");

    DeleteObject(SelectObject(hdc, hbm));
}

static void test_dc_target(BOOL d3d11)
{
    static const D2D1_PIXEL_FORMAT invalid_formats[] =
    {
        { DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED },
        { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_UNKNOWN },
        { DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED },
    };
    D2D1_TEXT_ANTIALIAS_MODE text_aa_mode;
    ID2D1GdiInteropRenderTarget *interop;
    D2D1_RENDER_TARGET_PROPERTIES desc;
    D2D1_MATRIX_3X2_F matrix, matrix2;
    ID2D1DCRenderTarget *rt, *rt2;
    struct d2d1_test_context ctx;
    D2D1_ANTIALIAS_MODE aa_mode;
    ID2D1SolidColorBrush *brush;
    ID2D1RenderTarget *rt3;
    ID2D1Factory *factory;
    FLOAT dpi_x, dpi_y;
    D2D1_COLOR_F color;
    D2D1_SIZE_U sizeu;
    D2D1_SIZE_F size;
    D2D1_TAG t1, t2;
    unsigned int i;
    HDC hdc, hdc2;
    D2D_RECT_F r;
    COLORREF clr;
    HRESULT hr;
    RECT rect;

    if (!init_test_context(&ctx, d3d11))
        return;
    release_test_context(&ctx);

    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &IID_ID2D1Factory, NULL, (void **)&factory);
    ok(SUCCEEDED(hr), "Failed to create factory, hr %#x.\n", hr);

    for (i = 0; i < ARRAY_SIZE(invalid_formats); ++i)
    {
        desc.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
        desc.pixelFormat = invalid_formats[i];
        desc.dpiX = 96.0f;
        desc.dpiY = 96.0f;
        desc.usage = D2D1_RENDER_TARGET_USAGE_NONE;
        desc.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;

        hr = ID2D1Factory_CreateDCRenderTarget(factory, &desc, &rt);
        ok(hr == D2DERR_UNSUPPORTED_PIXEL_FORMAT, "Got unexpected hr %#x.\n", hr);
    }

    desc.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
    desc.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    desc.dpiX = 96.0f;
    desc.dpiY = 96.0f;
    desc.usage = D2D1_RENDER_TARGET_USAGE_NONE;
    desc.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;
    hr = ID2D1Factory_CreateDCRenderTarget(factory, &desc, &rt);
    ok(SUCCEEDED(hr), "Failed to create target, hr %#x.\n", hr);

    hr = ID2D1DCRenderTarget_QueryInterface(rt, &IID_ID2D1GdiInteropRenderTarget, (void **)&interop);
    ok(SUCCEEDED(hr), "Failed to get interop target, hr %#x.\n", hr);
    hr = ID2D1GdiInteropRenderTarget_QueryInterface(interop, &IID_ID2D1RenderTarget, (void **)&rt3);
    ok(SUCCEEDED(hr), "Failed to get render target back, %#x.\n", hr);
    ok(rt3 == (ID2D1RenderTarget *)rt, "Unexpected render target\n");
    ID2D1RenderTarget_Release(rt3);
    hr = ID2D1GdiInteropRenderTarget_QueryInterface(interop, &IID_ID2D1DCRenderTarget, (void **)&rt2);
    ok(SUCCEEDED(hr), "Failed to get render target back, %#x.\n", hr);
    ok(rt2 == rt, "Unexpected render target\n");
    ID2D1DCRenderTarget_Release(rt2);
    ID2D1GdiInteropRenderTarget_Release(interop);

    size = ID2D1DCRenderTarget_GetSize(rt);
    ok(size.width == 0.0f, "got width %.08e.\n", size.width);
    ok(size.height == 0.0f, "got height %.08e.\n", size.height);

    sizeu = ID2D1DCRenderTarget_GetPixelSize(rt);
    ok(sizeu.width == 0, "got width %u.\n", sizeu.width);
    ok(sizeu.height == 0, "got height %u.\n", sizeu.height);

    /* object creation methods work without BindDC() */
    set_color(&color, 0.0f, 0.0f, 0.0f, 0.0f);
    hr = ID2D1DCRenderTarget_CreateSolidColorBrush(rt, &color, NULL, &brush);
    ok(SUCCEEDED(hr), "Failed to create a brush, hr %#x.\n", hr);
    ID2D1SolidColorBrush_Release(brush);

    ID2D1DCRenderTarget_BeginDraw(rt);
    hr = ID2D1DCRenderTarget_EndDraw(rt, NULL, NULL);
    ok(hr == D2DERR_WRONG_STATE, "Got unexpected hr %#x.\n", hr);

    ID2D1DCRenderTarget_Release(rt);

    /* BindDC() */
    hr = ID2D1Factory_CreateDCRenderTarget(factory, &desc, &rt);
    ok(SUCCEEDED(hr), "Failed to create target, hr %#x.\n", hr);

    aa_mode = ID2D1DCRenderTarget_GetAntialiasMode(rt);
    ok(aa_mode == D2D1_ANTIALIAS_MODE_PER_PRIMITIVE, "Got wrong default aa mode %d.\n", aa_mode);
    text_aa_mode = ID2D1DCRenderTarget_GetTextAntialiasMode(rt);
    ok(text_aa_mode == D2D1_TEXT_ANTIALIAS_MODE_DEFAULT, "Got wrong default text aa mode %d.\n", text_aa_mode);

    ID2D1DCRenderTarget_GetDpi(rt, &dpi_x, &dpi_y);
    ok(dpi_x == 96.0f && dpi_y == 96.0f, "Got dpi_x %f, dpi_y %f.\n", dpi_x, dpi_y);

    hdc = CreateCompatibleDC(NULL);
    ok(hdc != NULL, "Failed to create an HDC.\n");

    create_target_dibsection(hdc, 16, 16);

    SetRect(&rect, 0, 0, 32, 32);
    hr = ID2D1DCRenderTarget_BindDC(rt, NULL, &rect);
    ok(hr == E_INVALIDARG, "BindDC() returned %#x.\n", hr);

    /* Target properties are retained during BindDC() */
    ID2D1DCRenderTarget_SetTags(rt, 1, 2);
    ID2D1DCRenderTarget_SetAntialiasMode(rt, D2D1_ANTIALIAS_MODE_ALIASED);
    ID2D1DCRenderTarget_SetTextAntialiasMode(rt, D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 200.0f, 600.0f);
    ID2D1DCRenderTarget_SetTransform(rt, &matrix);

    hr = ID2D1DCRenderTarget_BindDC(rt, hdc, &rect);
    ok(hr == S_OK, "BindDC() returned %#x.\n", hr);

    ID2D1DCRenderTarget_GetTags(rt, &t1, &t2);
    ok(t1 == 1 && t2 == 2, "Got wrong tags.\n");

    aa_mode = ID2D1DCRenderTarget_GetAntialiasMode(rt);
    ok(aa_mode == D2D1_ANTIALIAS_MODE_ALIASED, "Got wrong aa mode %d.\n", aa_mode);

    text_aa_mode = ID2D1DCRenderTarget_GetTextAntialiasMode(rt);
    ok(text_aa_mode == D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE, "Got wrong text aa mode %d.\n", text_aa_mode);

    ID2D1DCRenderTarget_GetTransform(rt, &matrix2);
    ok(!memcmp(&matrix, &matrix2, sizeof(matrix)), "Got wrong target transform.\n");

    set_matrix_identity(&matrix);
    ID2D1DCRenderTarget_SetTransform(rt, &matrix);

    /* target size comes from specified dimensions, not from selected bitmap size */
    size = ID2D1DCRenderTarget_GetSize(rt);
    ok(size.width == 32.0f, "got width %.08e.\n", size.width);
    ok(size.height == 32.0f, "got height %.08e.\n", size.height);

    /* clear one HDC to red, switch to another one, partially fill it and test contents */
    ID2D1DCRenderTarget_BeginDraw(rt);

    set_color(&color, 1.0f, 0.0f, 0.0f, 1.0f);
    ID2D1DCRenderTarget_Clear(rt, &color);

    hr = ID2D1DCRenderTarget_EndDraw(rt, NULL, NULL);
    ok(SUCCEEDED(hr), "EndDraw() failed, hr %#x.\n", hr);

    clr = GetPixel(hdc, 0, 0);
    ok(clr == RGB(255, 0, 0), "Unexpected color 0x%08x.\n", clr);

    hdc2 = CreateCompatibleDC(NULL);
    ok(hdc2 != NULL, "Failed to create an HDC.\n");

    create_target_dibsection(hdc2, 16, 16);

    hr = ID2D1DCRenderTarget_BindDC(rt, hdc2, &rect);
    ok(hr == S_OK, "BindDC() returned %#x.\n", hr);

    clr = GetPixel(hdc2, 0, 0);
    ok(clr == 0, "Unexpected color 0x%08x.\n", clr);

    set_color(&color, 0.0f, 1.0f, 0.0f, 1.0f);
    hr = ID2D1DCRenderTarget_CreateSolidColorBrush(rt, &color, NULL, &brush);
    ok(SUCCEEDED(hr), "Failed to create brush, hr %#x.\n", hr);

    ID2D1DCRenderTarget_BeginDraw(rt);

    r.left = r.top = 0.0f;
    r.bottom = 16.0f;
    r.right = 8.0f;
    ID2D1DCRenderTarget_FillRectangle(rt, &r, (ID2D1Brush*)brush);

    hr = ID2D1DCRenderTarget_EndDraw(rt, NULL, NULL);
    ok(SUCCEEDED(hr), "EndDraw() failed, hr %#x.\n", hr);

    ID2D1SolidColorBrush_Release(brush);

    clr = GetPixel(hdc2, 0, 0);
    ok(clr == RGB(0, 255, 0), "Unexpected color 0x%08x.\n", clr);

    clr = GetPixel(hdc2, 10, 0);
    ok(clr == 0, "Unexpected color 0x%08x.\n", clr);

    /* Invalid DC. */
    hr = ID2D1DCRenderTarget_BindDC(rt, (HDC)0xdeadbeef, &rect);
todo_wine
    ok(hr == E_INVALIDARG, "BindDC() returned %#x.\n", hr);

    ID2D1DCRenderTarget_BeginDraw(rt);

    set_color(&color, 1.0f, 0.0f, 0.0f, 1.0f);
    ID2D1DCRenderTarget_Clear(rt, &color);

    hr = ID2D1DCRenderTarget_EndDraw(rt, NULL, NULL);
    ok(SUCCEEDED(hr), "EndDraw() failed, hr %#x.\n", hr);

    clr = GetPixel(hdc2, 0, 0);
todo_wine
    ok(clr == RGB(255, 0, 0), "Unexpected color 0x%08x.\n", clr);

    hr = ID2D1DCRenderTarget_BindDC(rt, NULL, &rect);
    ok(hr == E_INVALIDARG, "BindDC() returned %#x.\n", hr);

    ID2D1DCRenderTarget_BeginDraw(rt);

    set_color(&color, 0.0f, 0.0f, 1.0f, 1.0f);
    ID2D1DCRenderTarget_Clear(rt, &color);

    hr = ID2D1DCRenderTarget_EndDraw(rt, NULL, NULL);
    ok(SUCCEEDED(hr), "EndDraw() failed, hr %#x.\n", hr);

    clr = GetPixel(hdc2, 0, 0);
todo_wine
    ok(clr == RGB(0, 0, 255), "Unexpected color 0x%08x.\n", clr);

    DeleteDC(hdc);
    DeleteDC(hdc2);
    ID2D1DCRenderTarget_Release(rt);
    ID2D1Factory_Release(factory);
}

static void test_hwnd_target(BOOL d3d11)
{
    D2D1_HWND_RENDER_TARGET_PROPERTIES hwnd_rt_desc;
    ID2D1GdiInteropRenderTarget *interop;
    D2D1_RENDER_TARGET_PROPERTIES desc;
    ID2D1HwndRenderTarget *rt, *rt2;
    struct d2d1_test_context ctx;
    ID2D1RenderTarget *rt3;
    ID2D1Factory *factory;
    D2D1_SIZE_U size;
    HRESULT hr;

    if (!init_test_context(&ctx, d3d11))
        return;
    release_test_context(&ctx);

    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &IID_ID2D1Factory, NULL, (void **)&factory);
    ok(SUCCEEDED(hr), "Failed to create factory, hr %#x.\n", hr);

    desc.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
    desc.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    desc.dpiX = 0.0f;
    desc.dpiY = 0.0f;
    desc.usage = D2D1_RENDER_TARGET_USAGE_NONE;
    desc.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;

    hwnd_rt_desc.hwnd = NULL;
    hwnd_rt_desc.pixelSize.width = 64;
    hwnd_rt_desc.pixelSize.height = 64;
    hwnd_rt_desc.presentOptions = D2D1_PRESENT_OPTIONS_NONE;

    hr = ID2D1Factory_CreateHwndRenderTarget(factory, &desc, &hwnd_rt_desc, &rt);
    ok(FAILED(hr), "Target creation should fail, hr %#x.\n", hr);

    hwnd_rt_desc.hwnd = (HWND)0xdeadbeef;
    hr = ID2D1Factory_CreateHwndRenderTarget(factory, &desc, &hwnd_rt_desc, &rt);
    ok(FAILED(hr), "Target creation should fail, hr %#x.\n", hr);

    hwnd_rt_desc.hwnd = CreateWindowA("static", "d2d_test", 0, 0, 0, 0, 0, 0, 0, 0, 0);
    ok(!!hwnd_rt_desc.hwnd, "Failed to create target window.\n");
    hr = ID2D1Factory_CreateHwndRenderTarget(factory, &desc, &hwnd_rt_desc, &rt);
    ok(SUCCEEDED(hr), "Failed to create render target, hr %#x.\n", hr);

    hr = ID2D1HwndRenderTarget_QueryInterface(rt, &IID_ID2D1GdiInteropRenderTarget, (void **)&interop);
    ok(SUCCEEDED(hr), "Failed to get interop target, hr %#x.\n", hr);
    hr = ID2D1GdiInteropRenderTarget_QueryInterface(interop, &IID_ID2D1RenderTarget, (void **)&rt3);
    ok(SUCCEEDED(hr), "Failed to get render target back, %#x.\n", hr);
    ok(rt3 == (ID2D1RenderTarget *)rt, "Unexpected render target\n");
    ID2D1RenderTarget_Release(rt3);
    hr = ID2D1GdiInteropRenderTarget_QueryInterface(interop, &IID_ID2D1HwndRenderTarget, (void **)&rt2);
    ok(SUCCEEDED(hr), "Failed to get render target back, %#x.\n", hr);
    ok(rt2 == rt, "Unexpected render target\n");
    ID2D1HwndRenderTarget_Release(rt2);
    ID2D1GdiInteropRenderTarget_Release(interop);

    size.width = 128;
    size.height = 64;
    hr = ID2D1HwndRenderTarget_Resize(rt, &size);
    ok(SUCCEEDED(hr), "Failed to resize render target, hr %#x.\n", hr);

    ID2D1HwndRenderTarget_Release(rt);

    DestroyWindow(hwnd_rt_desc.hwnd);
    ID2D1Factory_Release(factory);
}

#define test_compatible_target_size(r) test_compatible_target_size_(__LINE__, r)
static void test_compatible_target_size_(unsigned int line, ID2D1RenderTarget *rt)
{
    static const D2D1_SIZE_F size_1_0 = { 1.0f, 0.0f };
    static const D2D1_SIZE_F size_1_1 = { 1.0f, 1.0f };
    static const D2D1_SIZE_U px_size_1_1 = { 1, 1 };
    static const D2D1_SIZE_U zero_px_size;
    static const D2D1_SIZE_F zero_size;
    static const struct size_test
    {
        const D2D1_SIZE_U *pixel_size;
        const D2D1_SIZE_F *size;
    }
    size_tests[] =
    {
        { &zero_px_size, NULL },
        { &zero_px_size, &zero_size },
        { NULL, &zero_size },
        { NULL, &size_1_0 },
        { &px_size_1_1, &size_1_1 },
    };
    float dpi_x, dpi_y, rt_dpi_x, rt_dpi_y;
    D2D1_SIZE_U pixel_size, expected_size;
    ID2D1BitmapRenderTarget *bitmap_rt;
    ID2D1DeviceContext *context;
    unsigned int i;
    HRESULT hr;

    ID2D1RenderTarget_GetDpi(rt, &rt_dpi_x, &rt_dpi_y);

    for (i = 0; i < ARRAY_SIZE(size_tests); ++i)
    {
        hr = ID2D1RenderTarget_CreateCompatibleRenderTarget(rt, size_tests[i].size, size_tests[i].pixel_size,
                NULL, D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS_NONE, &bitmap_rt);
        ok_(__FILE__, line)(SUCCEEDED(hr), "%u: Failed to create render target, hr %#x.\n", i, hr);

        if (size_tests[i].pixel_size)
        {
            expected_size = *size_tests[i].pixel_size;
        }
        else if (size_tests[i].size)
        {
            expected_size.width = ceilf((size_tests[i].size->width * rt_dpi_x) / 96.0f);
            expected_size.height = ceilf((size_tests[i].size->height * rt_dpi_y) / 96.0f);
        }
        else
        {
            expected_size = ID2D1RenderTarget_GetPixelSize(rt);
        }

        pixel_size = ID2D1BitmapRenderTarget_GetPixelSize(bitmap_rt);
        ok_(__FILE__, line)(!memcmp(&pixel_size, &expected_size, sizeof(pixel_size)),
                "%u: unexpected target size %ux%u.\n", i, pixel_size.width, pixel_size.height);

        ID2D1BitmapRenderTarget_GetDpi(bitmap_rt, &dpi_x, &dpi_y);
        if (size_tests[i].pixel_size && size_tests[i].size && size_tests[i].size->width != 0.0f
                && size_tests[i].size->height != 0.0f)
        {
            ok_(__FILE__, line)(dpi_x == pixel_size.width * 96.0f / size_tests[i].size->width
                    && dpi_y == pixel_size.height * 96.0f / size_tests[i].size->height,
                    "%u: unexpected target dpi %.8ex%.8e.\n", i, dpi_x, dpi_y);
        }
        else
            ok_(__FILE__, line)(dpi_x == rt_dpi_x && dpi_y == rt_dpi_y,
                    "%u: unexpected target dpi %.8ex%.8e.\n", i, dpi_x, dpi_y);
        ID2D1BitmapRenderTarget_Release(bitmap_rt);
    }

    pixel_size.height = pixel_size.width = 0;
    hr = ID2D1RenderTarget_CreateCompatibleRenderTarget(rt, NULL, &pixel_size, NULL,
            D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS_NONE, &bitmap_rt);
    ok_(__FILE__, line)(SUCCEEDED(hr), "Failed to create render target, hr %#x.\n", hr);

    if (SUCCEEDED(ID2D1BitmapRenderTarget_QueryInterface(bitmap_rt, &IID_ID2D1DeviceContext, (void **)&context)))
    {
        ID2D1Bitmap *bitmap;

        pixel_size = ID2D1DeviceContext_GetPixelSize(context);
        ok_(__FILE__, line)(!pixel_size.width && !pixel_size.height, "Unexpected target size %ux%u.\n",
                pixel_size.width, pixel_size.height);

        ID2D1DeviceContext_GetTarget(context, (ID2D1Image **)&bitmap);
        pixel_size = ID2D1Bitmap_GetPixelSize(bitmap);
        ok_(__FILE__, line)(!pixel_size.width && !pixel_size.height, "Unexpected target size %ux%u.\n",
                pixel_size.width, pixel_size.height);
        ID2D1Bitmap_Release(bitmap);

        ID2D1DeviceContext_Release(context);
    }

    ID2D1BitmapRenderTarget_Release(bitmap_rt);
}

static void test_bitmap_target(BOOL d3d11)
{
    D2D1_HWND_RENDER_TARGET_PROPERTIES hwnd_rt_desc;
    ID2D1GdiInteropRenderTarget *interop;
    D2D1_SIZE_U pixel_size, pixel_size2;
    D2D1_RENDER_TARGET_PROPERTIES desc;
    ID2D1BitmapRenderTarget *rt, *rt2;
    ID2D1HwndRenderTarget *hwnd_rt;
    ID2D1Bitmap *bitmap, *bitmap2;
    struct d2d1_test_context ctx;
    ID2D1DCRenderTarget *dc_rt;
    D2D1_SIZE_F size, size2;
    ID2D1RenderTarget *rt3;
    ID2D1Factory *factory;
    float dpi[2], dpi2[2];
    D2D1_COLOR_F color;
    ULONG refcount;
    HRESULT hr;

    if (!init_test_context(&ctx, d3d11))
        return;
    release_test_context(&ctx);

    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &IID_ID2D1Factory, NULL, (void **)&factory);
    ok(SUCCEEDED(hr), "Failed to create factory, hr %#x.\n", hr);

    desc.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
    desc.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    desc.dpiX = 96.0f;
    desc.dpiY = 192.0f;
    desc.usage = D2D1_RENDER_TARGET_USAGE_NONE;
    desc.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;

    hwnd_rt_desc.hwnd = CreateWindowA("static", "d2d_test", 0, 0, 0, 0, 0, 0, 0, 0, 0);
    ok(!!hwnd_rt_desc.hwnd, "Failed to create target window.\n");
    hwnd_rt_desc.pixelSize.width = 64;
    hwnd_rt_desc.pixelSize.height = 64;
    hwnd_rt_desc.presentOptions = D2D1_PRESENT_OPTIONS_NONE;

    hr = ID2D1Factory_CreateHwndRenderTarget(factory, &desc, &hwnd_rt_desc, &hwnd_rt);
    ok(SUCCEEDED(hr), "Failed to create render target, hr %#x.\n", hr);

    test_compatible_target_size((ID2D1RenderTarget *)hwnd_rt);

    hr = ID2D1HwndRenderTarget_CreateCompatibleRenderTarget(hwnd_rt, NULL, NULL, NULL,
            D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS_NONE, &rt);
    ok(SUCCEEDED(hr), "Failed to create render target, hr %#x.\n", hr);

    hr = ID2D1BitmapRenderTarget_QueryInterface(rt, &IID_ID2D1GdiInteropRenderTarget, (void **)&interop);
    ok(SUCCEEDED(hr), "Failed to get interop target, hr %#x.\n", hr);
    hr = ID2D1GdiInteropRenderTarget_QueryInterface(interop, &IID_ID2D1RenderTarget, (void **)&rt3);
    ok(SUCCEEDED(hr), "Failed to get render target back, %#x.\n", hr);
    ok(rt3 == (ID2D1RenderTarget *)rt, "Unexpected render target\n");
    ID2D1RenderTarget_Release(rt3);
    hr = ID2D1GdiInteropRenderTarget_QueryInterface(interop, &IID_ID2D1BitmapRenderTarget, (void **)&rt2);
    ok(SUCCEEDED(hr), "Failed to get render target back, %#x.\n", hr);
    ok(rt2 == rt, "Unexpected render target\n");
    ID2D1BitmapRenderTarget_Release(rt2);
    ID2D1GdiInteropRenderTarget_Release(interop);

    /* See if parent target is referenced. */
    ID2D1HwndRenderTarget_AddRef(hwnd_rt);
    refcount = ID2D1HwndRenderTarget_Release(hwnd_rt);
    ok(refcount == 1, "Target should not have been referenced, got %u.\n", refcount);

    /* Size was not specified, should match parent. */
    pixel_size = ID2D1HwndRenderTarget_GetPixelSize(hwnd_rt);
    pixel_size2 = ID2D1BitmapRenderTarget_GetPixelSize(rt);
    ok(!memcmp(&pixel_size, &pixel_size2, sizeof(pixel_size)), "Got target pixel size mismatch.\n");

    size = ID2D1HwndRenderTarget_GetSize(hwnd_rt);
    size2 = ID2D1BitmapRenderTarget_GetSize(rt);
    ok(!memcmp(&size, &size2, sizeof(size)), "Got target DIP size mismatch.\n");

    ID2D1HwndRenderTarget_GetDpi(hwnd_rt, dpi, dpi + 1);
    ID2D1BitmapRenderTarget_GetDpi(rt, dpi2, dpi2 + 1);
    ok(!memcmp(dpi, dpi2, sizeof(dpi)), "Got dpi mismatch.\n");

    ID2D1BitmapRenderTarget_Release(rt);

    /* Pixel size specified. */
    set_size_u(&pixel_size, 32, 32);
    hr = ID2D1HwndRenderTarget_CreateCompatibleRenderTarget(hwnd_rt, NULL, &pixel_size, NULL,
            D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS_NONE, &rt);
    ok(SUCCEEDED(hr), "Failed to create render target, hr %#x.\n", hr);

    pixel_size2 = ID2D1BitmapRenderTarget_GetPixelSize(rt);
    ok(!memcmp(&pixel_size, &pixel_size2, sizeof(pixel_size)), "Got target pixel size mismatch.\n");

    ID2D1BitmapRenderTarget_GetDpi(rt, dpi2, dpi2 + 1);
    ok(!memcmp(dpi, dpi2, sizeof(dpi)), "Got dpi mismatch.\n");

    ID2D1BitmapRenderTarget_Release(rt);

    /* Both pixel size and DIP size are specified. */
    set_size_u(&pixel_size, 128, 128);
    hr = ID2D1HwndRenderTarget_CreateCompatibleRenderTarget(hwnd_rt, &size, &pixel_size, NULL,
            D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS_NONE, &rt);
    ok(SUCCEEDED(hr), "Failed to create render target, hr %#x.\n", hr);

    /* Doubled pixel size dimensions with the same DIP size give doubled dpi. */
    ID2D1BitmapRenderTarget_GetDpi(rt, dpi2, dpi2 + 1);
    ok(dpi[0] == dpi2[0] / 2.0f && dpi[1] == dpi2[1] / 2.0f, "Got dpi mismatch.\n");

    ID2D1BitmapRenderTarget_Release(rt);

    /* DIP size is specified, fractional. */
    set_size_f(&size, 70.1f, 70.4f);
    hr = ID2D1HwndRenderTarget_CreateCompatibleRenderTarget(hwnd_rt, &size, NULL, NULL,
            D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS_NONE, &rt);
    ok(SUCCEEDED(hr), "Failed to create render target, hr %#x.\n", hr);

    ID2D1BitmapRenderTarget_GetDpi(rt, dpi2, dpi2 + 1);

    pixel_size = ID2D1BitmapRenderTarget_GetPixelSize(rt);
    ok(pixel_size.width == ceilf(size.width * dpi[0] / 96.0f)
            && pixel_size.height == ceilf(size.height * dpi[1] / 96.0f), "Wrong pixel size %ux%u\n",
            pixel_size.width, pixel_size.height);

    dpi[0] *= (pixel_size.width / size.width) * (96.0f / dpi[0]);
    dpi[1] *= (pixel_size.height / size.height) * (96.0f / dpi[1]);

    ok(compare_float(dpi[0], dpi2[0], 1) && compare_float(dpi[1], dpi2[1], 1), "Got dpi mismatch.\n");

    ID2D1HwndRenderTarget_Release(hwnd_rt);

    /* Check if GetBitmap() returns same instance. */
    hr = ID2D1BitmapRenderTarget_GetBitmap(rt, &bitmap);
    ok(SUCCEEDED(hr), "GetBitmap() failed, hr %#x.\n", hr);
    hr = ID2D1BitmapRenderTarget_GetBitmap(rt, &bitmap2);
    ok(SUCCEEDED(hr), "GetBitmap() failed, hr %#x.\n", hr);
    ok(bitmap == bitmap2, "Got different bitmap instances.\n");

    /* Draw something, see if bitmap instance is retained. */
    ID2D1BitmapRenderTarget_BeginDraw(rt);
    set_color(&color, 1.0f, 1.0f, 0.0f, 1.0f);
    ID2D1BitmapRenderTarget_Clear(rt, &color);
    hr = ID2D1BitmapRenderTarget_EndDraw(rt, NULL, NULL);
    ok(SUCCEEDED(hr), "EndDraw() failed, hr %#x.\n", hr);

    ID2D1Bitmap_Release(bitmap2);
    hr = ID2D1BitmapRenderTarget_GetBitmap(rt, &bitmap2);
    ok(SUCCEEDED(hr), "GetBitmap() failed, hr %#x.\n", hr);
    ok(bitmap == bitmap2, "Got different bitmap instances.\n");

    ID2D1Bitmap_Release(bitmap);
    ID2D1Bitmap_Release(bitmap2);

    refcount = ID2D1BitmapRenderTarget_Release(rt);
    ok(!refcount, "Target should be released, got %u.\n", refcount);

    DestroyWindow(hwnd_rt_desc.hwnd);

    /* Compatible target created from a DC target without associated HDC */
    desc.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
    desc.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    desc.dpiX = 192.0f;
    desc.dpiY = 96.0f;
    desc.usage = D2D1_RENDER_TARGET_USAGE_NONE;
    desc.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;
    hr = ID2D1Factory_CreateDCRenderTarget(factory, &desc, &dc_rt);
    ok(SUCCEEDED(hr), "Failed to create target, hr %#x.\n", hr);

    test_compatible_target_size((ID2D1RenderTarget *)dc_rt);

    hr = ID2D1DCRenderTarget_CreateCompatibleRenderTarget(dc_rt, NULL, NULL, NULL,
            D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS_NONE, &rt);
    ok(SUCCEEDED(hr), "Failed to create render target, hr %#x.\n", hr);

    pixel_size = ID2D1BitmapRenderTarget_GetPixelSize(rt);
    ok(pixel_size.width == 0 && pixel_size.height == 0, "Got wrong size\n");

    hr = ID2D1BitmapRenderTarget_GetBitmap(rt, &bitmap);
    ok(SUCCEEDED(hr), "GetBitmap() failed, hr %#x.\n", hr);
    pixel_size = ID2D1Bitmap_GetPixelSize(bitmap);
    ok(pixel_size.width == 0 && pixel_size.height == 0, "Got wrong size\n");
    ID2D1Bitmap_Release(bitmap);

    ID2D1BitmapRenderTarget_Release(rt);
    ID2D1DCRenderTarget_Release(dc_rt);

    ID2D1Factory_Release(factory);
}

static void test_desktop_dpi(BOOL d3d11)
{
    ID2D1Factory *factory;
    float dpi_x, dpi_y;
    HRESULT hr;

    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &IID_ID2D1Factory, NULL, (void **)&factory);
    ok(SUCCEEDED(hr), "Failed to create factory, hr %#x.\n", hr);

    dpi_x = dpi_y = 0.0f;
    ID2D1Factory_GetDesktopDpi(factory, &dpi_x, &dpi_y);
    ok(dpi_x > 0.0f && dpi_y > 0.0f, "Got wrong dpi %f x %f.\n", dpi_x, dpi_y);

    ID2D1Factory_Release(factory);
}

static void test_stroke_style(BOOL d3d11)
{
    static const struct
    {
        D2D1_DASH_STYLE dash_style;
        UINT32 dash_count;
        float dashes[6];
    }
    dash_style_tests[] =
    {
        {D2D1_DASH_STYLE_SOLID,        0},
        {D2D1_DASH_STYLE_DASH,         2, {2.0f, 2.0f}},
        {D2D1_DASH_STYLE_DOT,          2, {0.0f, 2.0f}},
        {D2D1_DASH_STYLE_DASH_DOT,     4, {2.0f, 2.0f, 0.0f, 2.0f}},
        {D2D1_DASH_STYLE_DASH_DOT_DOT, 6, {2.0f, 2.0f, 0.0f, 2.0f, 0.0f, 2.0f}},
    };
    D2D1_STROKE_STYLE_PROPERTIES desc;
    ID2D1StrokeStyle *style;
    ID2D1Factory *factory;
    UINT32 count;
    HRESULT hr;
    D2D1_CAP_STYLE cap_style;
    D2D1_LINE_JOIN line_join;
    float miter_limit, dash_offset;
    D2D1_DASH_STYLE dash_style;
    unsigned int i;
    float dashes[2];

    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &IID_ID2D1Factory, NULL, (void **)&factory);
    ok(SUCCEEDED(hr), "Failed to create factory, hr %#x.\n", hr);

    desc.startCap = D2D1_CAP_STYLE_SQUARE;
    desc.endCap = D2D1_CAP_STYLE_ROUND;
    desc.dashCap = D2D1_CAP_STYLE_TRIANGLE;
    desc.lineJoin = D2D1_LINE_JOIN_BEVEL;
    desc.miterLimit = 1.5f;
    desc.dashStyle = D2D1_DASH_STYLE_DOT;
    desc.dashOffset = -1.0f;

    hr = ID2D1Factory_CreateStrokeStyle(factory, &desc, NULL, 0, &style);
    ok(SUCCEEDED(hr), "Failed to create stroke style, %#x.\n", hr);

    cap_style = ID2D1StrokeStyle_GetStartCap(style);
    ok(cap_style == D2D1_CAP_STYLE_SQUARE, "Unexpected cap style %d.\n", cap_style);
    cap_style = ID2D1StrokeStyle_GetEndCap(style);
    ok(cap_style == D2D1_CAP_STYLE_ROUND, "Unexpected cap style %d.\n", cap_style);
    cap_style = ID2D1StrokeStyle_GetDashCap(style);
    ok(cap_style == D2D1_CAP_STYLE_TRIANGLE, "Unexpected cap style %d.\n", cap_style);
    line_join = ID2D1StrokeStyle_GetLineJoin(style);
    ok(line_join == D2D1_LINE_JOIN_BEVEL, "Unexpected line joind %d.\n", line_join);
    miter_limit = ID2D1StrokeStyle_GetMiterLimit(style);
    ok(miter_limit == 1.5f, "Unexpected miter limit %f.\n", miter_limit);
    dash_style = ID2D1StrokeStyle_GetDashStyle(style);
    ok(dash_style == D2D1_DASH_STYLE_DOT, "Unexpected dash style %d.\n", dash_style);
    dash_offset = ID2D1StrokeStyle_GetDashOffset(style);
    ok(dash_offset == -1.0f, "Unexpected dash offset %f.\n", dash_offset);

    /* Custom dash pattern, no dashes data specified. */
    desc.startCap = D2D1_CAP_STYLE_SQUARE;
    desc.endCap = D2D1_CAP_STYLE_ROUND;
    desc.dashCap = D2D1_CAP_STYLE_TRIANGLE;
    desc.lineJoin = D2D1_LINE_JOIN_BEVEL;
    desc.miterLimit = 1.5f;
    desc.dashStyle = D2D1_DASH_STYLE_CUSTOM;
    desc.dashOffset = 0.0f;

    ID2D1StrokeStyle_Release(style);

    hr = ID2D1Factory_CreateStrokeStyle(factory, &desc, NULL, 0, &style);
    ok(hr == E_INVALIDARG, "Unexpected return value, %#x.\n", hr);

    hr = ID2D1Factory_CreateStrokeStyle(factory, &desc, dashes, 0, &style);
    ok(hr == E_INVALIDARG, "Unexpected return value, %#x.\n", hr);

    hr = ID2D1Factory_CreateStrokeStyle(factory, &desc, dashes, 1, &style);
    ok(hr == S_OK, "Unexpected return value, %#x.\n", hr);
    ID2D1StrokeStyle_Release(style);

    /* Builtin style, dashes are specified. */
    desc.dashStyle = D2D1_DASH_STYLE_DOT;
    hr = ID2D1Factory_CreateStrokeStyle(factory, &desc, dashes, 1, &style);
    ok(hr == E_INVALIDARG, "Unexpected return value, %#x.\n", hr);

    /* Invalid style. */
    desc.dashStyle = 100;
    hr = ID2D1Factory_CreateStrokeStyle(factory, &desc, NULL, 0, &style);
    ok(hr == E_INVALIDARG, "Unexpected return value, %#x.\n", hr);

    /* Test returned dash pattern for builtin styles. */
    desc.startCap = D2D1_CAP_STYLE_SQUARE;
    desc.endCap = D2D1_CAP_STYLE_ROUND;
    desc.dashCap = D2D1_CAP_STYLE_TRIANGLE;
    desc.lineJoin = D2D1_LINE_JOIN_BEVEL;
    desc.miterLimit = 1.5f;
    desc.dashOffset = 0.0f;

    for (i = 0; i < ARRAY_SIZE(dash_style_tests); ++i)
    {
        float dashes[10];
        UINT dash_count;

        desc.dashStyle = dash_style_tests[i].dash_style;

        hr = ID2D1Factory_CreateStrokeStyle(factory, &desc, NULL, 0, &style);
        ok(SUCCEEDED(hr), "Failed to create stroke style, %#x.\n", hr);

        dash_count = ID2D1StrokeStyle_GetDashesCount(style);
        ok(dash_count == dash_style_tests[i].dash_count, "%u: unexpected dash count %u, expected %u.\n",
                i, dash_count, dash_style_tests[i].dash_count);
        ok(dash_count < ARRAY_SIZE(dashes), "%u: unexpectedly large dash count %u.\n", i, dash_count);
        if (dash_count == dash_style_tests[i].dash_count)
        {
            unsigned int j;

            ID2D1StrokeStyle_GetDashes(style, dashes, dash_count);
            ok(!memcmp(dashes, dash_style_tests[i].dashes, sizeof(*dashes) * dash_count),
                    "%u: unexpected dash array.\n", i);

            /* Ask for more dashes than style actually has. */
            memset(dashes, 0xcc, sizeof(dashes));
            ID2D1StrokeStyle_GetDashes(style, dashes, ARRAY_SIZE(dashes));
            ok(!memcmp(dashes, dash_style_tests[i].dashes, sizeof(*dashes) * dash_count),
                    "%u: unexpected dash array.\n", i);

            for (j = dash_count; j < ARRAY_SIZE(dashes); ++j)
                ok(dashes[j] == 0.0f, "%u: unexpected dash value at %u.\n", i, j);
        }

        ID2D1StrokeStyle_Release(style);
    }

    /* NULL dashes array, non-zero length. */
    memset(&desc, 0, sizeof(desc));
    hr = ID2D1Factory_CreateStrokeStyle(factory, &desc, NULL, 1, &style);
    ok(SUCCEEDED(hr), "Failed to create stroke style, %#x.\n", hr);

    count = ID2D1StrokeStyle_GetDashesCount(style);
    ok(count == 0, "Unexpected dashes count %u.\n", count);

    ID2D1StrokeStyle_Release(style);

    ID2D1Factory_Release(factory);
}

static void test_gradient(BOOL d3d11)
{
    ID2D1GradientStopCollection *gradient;
    D2D1_GRADIENT_STOP stops[3], stops2[3];
    struct d2d1_test_context ctx;
    ID2D1RenderTarget *rt;
    D2D1_COLOR_F color;
    unsigned int i;
    UINT32 count;
    HRESULT hr;

    if (!init_test_context(&ctx, d3d11))
        return;

    rt = ctx.rt;
    stops2[0].position = 0.5f;
    set_color(&stops2[0].color, 1.0f, 1.0f, 0.0f, 1.0f);
    stops2[1] = stops2[0];
    hr = ID2D1RenderTarget_CreateGradientStopCollection(rt, stops2, 2, D2D1_GAMMA_2_2,
            D2D1_EXTEND_MODE_CLAMP, &gradient);
    ok(SUCCEEDED(hr), "Failed to create stop collection, hr %#x.\n", hr);

    count = ID2D1GradientStopCollection_GetGradientStopCount(gradient);
    ok(count == 2, "Unexpected stop count %u.\n", count);

    /* Request more stops than collection has. */
    stops[0].position = 123.4f;
    set_color(&stops[0].color, 1.0f, 0.5f, 0.4f, 1.0f);
    color = stops[0].color;
    stops[2] = stops[1] = stops[0];
    ID2D1GradientStopCollection_GetGradientStops(gradient, stops, ARRAY_SIZE(stops));
    ok(!memcmp(stops, stops2, sizeof(*stops) * count), "Unexpected gradient stops array.\n");
    for (i = count; i < ARRAY_SIZE(stops); ++i)
    {
        ok(stops[i].position == 123.4f, "%u: unexpected stop position %f.\n", i, stops[i].position);
        ok(!memcmp(&stops[i].color, &color, sizeof(color)), "%u: unexpected stop color.\n", i);
    }

    ID2D1GradientStopCollection_Release(gradient);

    release_test_context(&ctx);
}

static void test_draw_geometry(BOOL d3d11)
{
    ID2D1TransformedGeometry *transformed_geometry[4];
    ID2D1RectangleGeometry *rect_geometry[2];
    D2D1_POINT_2F point = {0.0f, 0.0f};
    D2D1_ROUNDED_RECT rounded_rect;
    struct d2d1_test_context ctx;
    ID2D1SolidColorBrush *brush;
    ID2D1PathGeometry *geometry;
    D2D1_MATRIX_3X2_F matrix;
    ID2D1GeometrySink *sink;
    ID2D1RenderTarget *rt;
    ID2D1Factory *factory;
    D2D1_POINT_2F p0, p1;
    D2D1_ELLIPSE ellipse;
    D2D1_COLOR_F color;
    D2D1_RECT_F rect;
    HRESULT hr;
    BOOL match;

    if (!init_test_context(&ctx, d3d11))
        return;

    rt = ctx.rt;
    ID2D1RenderTarget_GetFactory(rt, &factory);

    ID2D1RenderTarget_SetDpi(rt, 192.0f, 48.0f);
    ID2D1RenderTarget_SetAntialiasMode(rt, D2D1_ANTIALIAS_MODE_ALIASED);
    set_color(&color, 0.890f, 0.851f, 0.600f, 1.0f);
    hr = ID2D1RenderTarget_CreateSolidColorBrush(rt, &color, NULL, &brush);
    ok(SUCCEEDED(hr), "Failed to create brush, hr %#x.\n", hr);

    ID2D1RenderTarget_BeginDraw(rt);
    set_color(&color, 0.396f, 0.180f, 0.537f, 1.0f);
    ID2D1RenderTarget_Clear(rt, &color);

    set_point(&p0, 40.0f, 160.0f);
    ID2D1RenderTarget_DrawLine(rt, p0, p0, (ID2D1Brush *)brush, 10.0f, NULL);
    set_point(&p0, 100.0f, 160.0f);
    set_point(&p1, 140.0f, 160.0f);
    ID2D1RenderTarget_DrawLine(rt, p0, p1, (ID2D1Brush *)brush, 10.0f, NULL);
    set_point(&p0, 200.0f,  80.0f);
    set_point(&p1, 200.0f, 240.0f);
    ID2D1RenderTarget_DrawLine(rt, p0, p1, (ID2D1Brush *)brush, 10.0f, NULL);
    set_point(&p0, 260.0f, 240.0f);
    set_point(&p1, 300.0f,  80.0f);
    ID2D1RenderTarget_DrawLine(rt, p0, p1, (ID2D1Brush *)brush, 10.0f, NULL);

    set_rect(&rect, 40.0f, 480.0f, 40.0f, 480.0f);
    ID2D1RenderTarget_DrawRectangle(rt, &rect, (ID2D1Brush *)brush, 10.0f, NULL);
    set_rect(&rect, 100.0f, 480.0f, 140.0f, 480.0f);
    ID2D1RenderTarget_DrawRectangle(rt, &rect, (ID2D1Brush *)brush, 10.0f, NULL);
    set_rect(&rect, 200.0f, 400.0f, 200.0f, 560.0f);
    ID2D1RenderTarget_DrawRectangle(rt, &rect, (ID2D1Brush *)brush, 10.0f, NULL);
    set_rect(&rect, 260.0f, 560.0f, 300.0f, 400.0f);
    ID2D1RenderTarget_DrawRectangle(rt, &rect, (ID2D1Brush *)brush, 10.0f, NULL);

    set_ellipse(&ellipse,  40.0f, 800.0f,  0.0f,  0.0f);
    ID2D1RenderTarget_DrawEllipse(rt, &ellipse, (ID2D1Brush *)brush, 10.0f, NULL);
    set_ellipse(&ellipse, 120.0f, 800.0f, 20.0f,  0.0f);
    ID2D1RenderTarget_DrawEllipse(rt, &ellipse, (ID2D1Brush *)brush, 10.0f, NULL);
    set_ellipse(&ellipse, 200.0f, 800.0f,  0.0f, 80.0f);
    ID2D1RenderTarget_DrawEllipse(rt, &ellipse, (ID2D1Brush *)brush, 10.0f, NULL);
    set_ellipse(&ellipse, 280.0f, 800.0f, 20.0f, 80.0f);
    ID2D1RenderTarget_DrawEllipse(rt, &ellipse, (ID2D1Brush *)brush, 10.0f, NULL);

    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to end draw, hr %#x.\n", hr);

    match = compare_figure(&ctx,   0,   0, 160, 160, 0xff652e89, 0, "");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160,   0, 160, 160, 0xff652e89, 0, "yGBQUFBQUFBQUFDoYQAA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 320,   0, 160, 160, 0xff652e89, 0,
            "xjIUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUxjIA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 480,   0, 160, 160, 0xff652e89, 2,
            "zjECnQETjAEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEV"
            "igEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEV"
            "igEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEV"
            "igEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEVigEV"
            "igEVigEVigEVigEVjAETnQECzjEA");
    ok(match, "Figure does not match.\n");

    match = compare_figure(&ctx,   0, 160, 160, 160, 0xff652e89, 0, "5mAUjAEUjAEUjAEUjAEUhmIA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160, 160, 160, 160, 0xff652e89, 0, "vmBkPGQ8ZDxkPGTeYQAA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 320, 160, 160, 160, 0xff652e89, 0,
            "5i4UjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUhjAA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 480, 160, 160, 160, 0xff652e89, 0,
            "vi5kPGQ8ZDxkPGQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8"
            "FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8"
            "FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8ZDxkPGQ8ZDxk3i8A");
    ok(match, "Figure does not match.\n");

    match = compare_figure(&ctx,   0, 320, 160, 160, 0xff652e89, 0, "iGIQjgEUjAEUjgEQiGIA");
    todo_wine ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160, 320, 160, 160, 0xff652e89, 0, "yGBQSGA+ZDxkPmDgYQAA");
    todo_wine ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 320, 320, 160, 160, 0xff652e89, 0,
            "iDAQjgEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjgEQiDAA");
    todo_wine ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 480, 320, 160, 160, 0xff652e89, 10,
            "hDAYgwEieyh1LnAybBcIF2gWDhZkFhIWYRUWFV4VGhVbFRwVWRUeFVcVIBVVFCQUUxQmFFEUKBRP"
            "FSgVTRUqFUwULBRLFC4USRQwFEgUMBRHFDIURhQyFEUUNBREFDQUQxQ2FEIUNhRBFDgUQBQ4FEAU"
            "OBQ/FDoUPhQ6FD4UOhQ+FDoUPhQ6FD0UPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8"
            "FDwUPBQ8FDwUPBQ8FD0UOhQ+FDoUPhQ6FD4UOhQ+FDoUPxQ4FEAUOBRAFDgUQRQ2FEIUNhRDFDQU"
            "RBQ0FEUUMhRGFDIURxQwFEgUMBRJFC4USxQsFEwVKhVNFSgVTxQoFFEUJhRTFCQUVRUgFVcVHhVZ"
            "FRwVWxUaFV4VFhVhFhIWZBYOFmgXCBdsMnAudSh7IoMBGIQw");
    todo_wine ok(match, "Figure does not match.\n");

    ID2D1RenderTarget_BeginDraw(rt);
    set_color(&color, 0.396f, 0.180f, 0.537f, 1.0f);
    ID2D1RenderTarget_Clear(rt, &color);

    set_rounded_rect(&rounded_rect, 40.0f, 160.0f, 40.0f, 160.0f, 10.0f, 10.0f);
    ID2D1RenderTarget_DrawRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush, 10.0f, NULL);
    set_rounded_rect(&rounded_rect, 100.0f, 160.0f, 140.0f, 160.0f, 10.0f, 10.0f);
    ID2D1RenderTarget_DrawRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush, 10.0f, NULL);
    set_rounded_rect(&rounded_rect, 200.0f, 80.0f, 200.0f, 240.0f, 10.0f, 10.0f);
    ID2D1RenderTarget_DrawRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush, 10.0f, NULL);
    set_rounded_rect(&rounded_rect, 260.0f, 240.0f, 300.0f, 80.0f, 10.0f, 10.0f);
    ID2D1RenderTarget_DrawRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush, 10.0f, NULL);

    set_rounded_rect(&rounded_rect, 40.0f, 480.0f, 40.0f, 480.0f, 10.0f, 20.0f);
    ID2D1RenderTarget_DrawRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush, 10.0f, NULL);
    set_rounded_rect(&rounded_rect, 100.0f, 480.0f, 140.0f, 480.0f, 10.0f, 20.0f);
    ID2D1RenderTarget_DrawRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush, 10.0f, NULL);
    set_rounded_rect(&rounded_rect, 200.0f, 400.0f, 200.0f, 560.0f, 10.0f, 20.0f);
    ID2D1RenderTarget_DrawRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush, 10.0f, NULL);
    set_rounded_rect(&rounded_rect, 260.0f, 560.0f, 300.0f, 400.0f, 10.0f, 20.0f);
    ID2D1RenderTarget_DrawRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush, 10.0f, NULL);

    set_rounded_rect(&rounded_rect, 40.0f, 800.0f, 40.0f, 800.0f, 10.0f, 5.0f);
    ID2D1RenderTarget_DrawRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush, 10.0f, NULL);
    set_rounded_rect(&rounded_rect, 100.0f, 800.0f, 140.0f, 800.0f, 10.0f, 5.0f);
    ID2D1RenderTarget_DrawRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush, 10.0f, NULL);
    set_rounded_rect(&rounded_rect, 200.0f, 720.0f, 200.0f, 880.0f, 10.0f, 5.0f);
    ID2D1RenderTarget_DrawRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush, 10.0f, NULL);
    set_rounded_rect(&rounded_rect, 260.0f, 880.0f, 300.0f, 720.0f, 10.0f, 5.0f);
    ID2D1RenderTarget_DrawRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush, 10.0f, NULL);

    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to end draw, hr %#x.\n", hr);

    match = compare_figure(&ctx,   0,   0, 160, 160, 0xff652e89, 0, "iGIQjgEUjAEUjgEQiGIA");
    todo_wine ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160,   0, 160, 160, 0xff652e89, 0, "yGBQSGA+ZDxkPmDgYQAA");
    todo_wine ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 320,   0, 160, 160, 0xff652e89, 0,
            "iDAQjgEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjgEQiDAA");
    todo_wine ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 480,   0, 160, 160, 0xff652e89, 0,
            "3C4oaUZVUExYRlxCHCgcPxU4FT0UPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8"
            "FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8"
            "FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FD0VOBU/YEJcRlhMUFVG7S8A");
    todo_wine ok(match, "Figure does not match.\n");

    match = compare_figure(&ctx,   0, 160, 160, 160, 0xff652e89, 0, "iGIQjgEUjAEUjgEQiGIA");
    todo_wine ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160, 160, 160, 160, 0xff652e89, 0, "yGBQSGA+ZDxkPmDgYQAA");
    todo_wine ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 320, 160, 160, 160, 0xff652e89, 0,
            "iDAQjgEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjgEQiDAA");
    todo_wine ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 480, 160, 160, 160, 0xff652e89, 8,
            "3C4obT5dSFRQTlRKGCgYRhYwFkMVNBVBFTYVPxU5FD4UOhQ9FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8"
            "FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8"
            "FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPRQ6FD4UOhQ/FTYVQRU0FUMWMBZGWEpVTVBTSltA8C8A");
    todo_wine ok(match, "Figure does not match.\n");

    match = compare_figure(&ctx,   0, 320, 160, 160, 0xff652e89, 0, "iGIQjgEUjAEUjgEQiGIA");
    todo_wine ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160, 320, 160, 160, 0xff652e89, 0, "yGBQSGA+ZDxkPmDgYQAA");
    todo_wine ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 320, 320, 160, 160, 0xff652e89, 0,
            "iDAQjgEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjgEQiDAA");
    todo_wine ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 480, 320, 160, 160, 0xff652e89, 0,
            "3C4oZU5NWERgP2I9HigePBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8"
            "FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8"
            "FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8ZD1iP2BEWE1O6S8A");
    todo_wine ok(match, "Figure does not match.\n");

    ID2D1RenderTarget_BeginDraw(rt);
    set_color(&color, 0.396f, 0.180f, 0.537f, 1.0f);
    ID2D1RenderTarget_Clear(rt, &color);

    set_rounded_rect(&rounded_rect, 40.0f, 160.0f, 40.0f, 160.0f, 1000.0f, 1000.0f);
    ID2D1RenderTarget_DrawRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush, 10.0f, NULL);
    set_rounded_rect(&rounded_rect, 100.0f, 160.0f, 140.0f, 160.0f, 1000.0f, 1000.0f);
    ID2D1RenderTarget_DrawRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush, 10.0f, NULL);
    set_rounded_rect(&rounded_rect, 200.0f, 80.0f, 200.0f, 240.0f, 1000.0f, 1000.0f);
    ID2D1RenderTarget_DrawRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush, 10.0f, NULL);
    set_rounded_rect(&rounded_rect, 260.0f, 240.0f, 300.0f, 80.0f, 1000.0f, 1000.0f);
    ID2D1RenderTarget_DrawRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush, 10.0f, NULL);

    set_rounded_rect(&rounded_rect, 40.0f, 480.0f, 40.0f, 480.0f, 10.0f, 1000.0f);
    ID2D1RenderTarget_DrawRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush, 10.0f, NULL);
    set_rounded_rect(&rounded_rect, 100.0f, 480.0f, 140.0f, 480.0f, 10.0f, 1000.0f);
    ID2D1RenderTarget_DrawRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush, 10.0f, NULL);
    set_rounded_rect(&rounded_rect, 200.0f, 400.0f, 200.0f, 560.0f, 10.0f, 1000.0f);
    ID2D1RenderTarget_DrawRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush, 10.0f, NULL);
    set_rounded_rect(&rounded_rect, 260.0f, 560.0f, 300.0f, 400.0f, 10.0f, 1000.0f);
    ID2D1RenderTarget_DrawRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush, 10.0f, NULL);

    set_rounded_rect(&rounded_rect, 40.0f, 800.0f, 40.0f, 800.0f, 1000.0f, 10.0f);
    ID2D1RenderTarget_DrawRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush, 10.0f, NULL);
    set_rounded_rect(&rounded_rect, 100.0f, 800.0f, 140.0f, 800.0f, 1000.0f, 10.0f);
    ID2D1RenderTarget_DrawRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush, 10.0f, NULL);
    set_rounded_rect(&rounded_rect, 200.0f, 720.0f, 200.0f, 880.0f, 1000.0f, 10.0f);
    ID2D1RenderTarget_DrawRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush, 10.0f, NULL);
    set_rounded_rect(&rounded_rect, 260.0f, 880.0f, 300.0f, 720.0f, 1000.0f, 10.0f);
    ID2D1RenderTarget_DrawRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush, 10.0f, NULL);

    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to end draw, hr %#x.\n", hr);

    match = compare_figure(&ctx,   0,   0, 160, 160, 0xff652e89, 0, "iGIQjgEUjAEUjgEQiGIA");
    todo_wine ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160,   0, 160, 160, 0xff652e89, 0, "yGBQSGA+ZDxkPmDgYQAA");
    todo_wine ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 320,   0, 160, 160, 0xff652e89, 0,
            "iDAQjgEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjgEQiDAA");
    todo_wine ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 480,   0, 160, 160, 0xff652e89, 16,
            "hDAYgwEieyh1LnAybBcIF2gWDhZkFhIWYRUWFV4WGRVbFRwVWRUeFVcVIBVVFSMUUxQmFFEVJxRP"
            "FSgVTRUqFUwULBRLFC4USRUvFEgUMBRHFDIURhQyFEUUNBREFDQUQxQ2FEIUNhRBFDgUQBQ4FEAU"
            "OBQ/FTkUPhQ6FD4UOhQ+FDoUPhQ6FD0UPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8"
            "FDwUPBQ8FDwUPBQ8FD0UOhQ+FDoUPhQ6FD4UOhQ+FDoUPxQ4FEAUOBRAFDgUQRQ2FEIUNhRDFDQU"
            "RBQ0FEUUMhRGFDIURxQwFEgUMBRJFC4USxQsFEwVKhVNFSgVTxQoFFEUJhRTFCQUVRUgFVcVHhVZ"
            "FRwVWxUaFV4VFhVhFhIWZBYOFmgWChZsMnAudCp6IoMBGIQw");
    todo_wine ok(match, "Figure does not match.\n");

    match = compare_figure(&ctx,   0, 160, 160, 160, 0xff652e89, 0, "iGIQjgEUjAEUjgEQiGIA");
    todo_wine ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160, 160, 160, 160, 0xff652e89, 0, "yGBQSGA+ZDxkPmDgYQAA");
    todo_wine ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 320, 160, 160, 160, 0xff652e89, 0,
            "iDAQjgEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjgEQiDAA");
    todo_wine ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 480, 160, 160, 160, 0xff652e89, 16,
            "3C4obzpjQF5EWkhXFSAVVRQkFFMUJhRRFCgUTxQqFE0VKhVMFCwUSxQuFEoULhVIFDAUSBQwFUYU"
            "MhRGFDIURRQ0FEQUNBRDFTQVQhQ2FEIUNhRCFDYUQRQ4FEAUOBRAFDgUQBQ4FD8UOhQ+FDoUPhQ6"
            "FD4UOhQ+FDoUPhQ6FD0VOxQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBU7FD0UOhQ+FDoUPhQ6FD4UOhQ+FDoUPhQ6FD8UOBRA"
            "FDgUQBQ4FEAUOBRBFDYUQhQ2FEIUNhRCFTQVQxQ0FEQUNBRFFDIURhQyFEYVMBVHFDAUSBUuFUkU"
            "LhRLFCwUTBUrFE0UKhRPFCgUURQmFFMUJBRVSldIWUZdQWI78i8A");
    todo_wine ok(match, "Figure does not match.\n");

    match = compare_figure(&ctx,   0, 320, 160, 160, 0xff652e89, 0, "iGIQjgEUjAEUjgEQiGIA");
    todo_wine ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160, 320, 160, 160, 0xff652e89, 0, "yGBQSGA+ZDxkPmDgYQAA");
    todo_wine ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 320, 320, 160, 160, 0xff652e89, 0,
            "iDAQjgEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEUjAEU"
            "jAEUjAEUjAEUjAEUjAEUjAEUjAEUjgEQiDAA");
    todo_wine ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 480, 320, 160, 160, 0xff652e89, 8,
            "9i80ZERWUExYRV5AHCocPRY4FjwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8"
            "FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8"
            "FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FToVPRssG0BeRFpLUFVGYzT2LwAA");
    todo_wine ok(match, "Figure does not match.\n");

    hr = ID2D1Factory_CreatePathGeometry(factory, &geometry);
    ok(SUCCEEDED(hr), "Failed to create path geometry, hr %#x.\n", hr);
    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(SUCCEEDED(hr), "Failed to open geometry sink, hr %#x.\n", hr);

    set_point(&point, 40.0f, 160.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_OPEN);

    set_point(&point, 120.0f, 160.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    line_to(sink, 120.0f, 160.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_OPEN);

    set_point(&point, 200.0f, 160.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 280.0f, 160.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    line_to(sink, 280.0f, 160.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 20.0f, 480.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    line_to(sink, 60.0f, 480.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_OPEN);

    set_point(&point, 120.0f, 400.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    line_to(sink, 120.0f, 560.0f);
    line_to(sink, 120.0f, 400.0f);
    line_to(sink, 120.0f, 560.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_OPEN);

    set_point(&point, 180.0f, 480.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    line_to(sink, 220.0f, 480.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 280.0f, 400.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    line_to(sink, 280.0f, 560.0f);
    line_to(sink, 280.0f, 400.0f);
    line_to(sink, 280.0f, 560.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 20.0f, 880.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    line_to(sink, 40.0f, 720.0f);
    line_to(sink, 60.0f, 880.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_OPEN);

    set_point(&point, 100.0f, 720.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    line_to(sink, 120.0f, 880.0f);
    line_to(sink, 140.0f, 720.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_OPEN);

    set_point(&point, 180.0f, 880.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    line_to(sink, 200.0f, 720.0f);
    line_to(sink, 220.0f, 880.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 260.0f, 720.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    line_to(sink, 280.0f, 880.0f);
    line_to(sink, 300.0f, 720.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    ID2D1GeometrySink_SetFillMode(sink, D2D1_FILL_MODE_ALTERNATE);
    hr = ID2D1GeometrySink_Close(sink);
    ok(SUCCEEDED(hr), "Failed to close geometry sink, hr %#x.\n", hr);
    ID2D1GeometrySink_Release(sink);

    ID2D1RenderTarget_BeginDraw(rt);
    ID2D1RenderTarget_Clear(rt, &color);
    ID2D1RenderTarget_DrawGeometry(rt, (ID2D1Geometry *)geometry, (ID2D1Brush *)brush, 5.0f, NULL);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to end draw, hr %#x.\n", hr);
    ID2D1PathGeometry_Release(geometry);

    match = compare_figure(&ctx,   0,   0, 160, 160, 0xff652e89, 0, "");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160,   0, 160, 160, 0xff652e89, 0, "");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 320,   0, 160, 160, 0xff652e89, 0, "");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 480,   0, 160, 160, 0xff652e89, 0, "q2MKlgEKq2MA");
    todo_wine ok(match, "Figure does not match.\n");

    match = compare_figure(&ctx,   0, 160, 160, 160, 0xff652e89, 0, "iGNQUFCIYwAA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160, 160, 160, 160, 0xff652e89, 0,
            "qyIKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEK"
            "lgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEK"
            "lgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKQQpLCkEKSwqWAQqW"
            "AQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqW"
            "AQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqW"
            "AQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQrLIwAA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 320, 160, 160, 160, 0xff652e89, 0, "4GLAAuBi");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 480, 160, 160, 160, 0xff652e89, 0,
            "qyIKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEK"
            "lgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEK"
            "lgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKlgEKSwpBCksKQQqWAQqWAQqW"
            "AQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqW"
            "AQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqW"
            "AQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQqWAQrLIwAA");
    ok(match, "Figure does not match.\n");

    match = compare_figure(&ctx,   0, 320, 160, 160, 0xff652e89, 0,
            "rycCngECnQEEnAEEmwEGmgEGmQEImAEIlwEKlgEKlQEMlAEMkwEOkgEOkQEQkAEQjwESjgESjQEU"
            "jAEUiwEKAgqKAQoCCokBCgQKiAEKBAqHAQoGCoYBCgYKhQEKCAqEAQoICoMBCgoKggEKCgqBAQoM"
            "CoABCgwKfwoOCn4KDgp9ChAKfAoQCnsKEgp6ChIKeQoUCngKFAp3ChYKdgoWCnUKGAp0ChgKcwoa"
            "CnIKGgpxChwKcAocCm8KHgpuCh4KbQogCmwKIAprCiIKagoiCmkKJApoCiQKZwomCmYKJgplCigK"
            "ZAooCmMKKgpiCioKYQosCmAKLApfCi4KXgouCl0KMApcCjAKWwoyCloKMgpZCjQKWAo0ClcKNgpW"
            "CjYKVQo4ClQKOApTCjoKUgo6ClEKPApQCjwKTwo+Ck4KPgpNCkAKTApACksKQgpKCkIKSQpECkgK"
            "RApHCkYKozIA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160, 320, 160, 160, 0xff652e89, 0,
            "ozIKRgpHCkQKSApECkkKQgpKCkIKSwpACkwKQApNCj4KTgo+Ck8KPApQCjwKUQo6ClIKOgpTCjgK"
            "VAo4ClUKNgpWCjYKVwo0ClgKNApZCjIKWgoyClsKMApcCjAKXQouCl4KLgpfCiwKYAosCmEKKgpi"
            "CioKYwooCmQKKAplCiYKZgomCmcKJApoCiQKaQoiCmoKIgprCiAKbAogCm0KHgpuCh4KbwocCnAK"
            "HApxChoKcgoaCnMKGAp0ChgKdQoWCnYKFgp3ChQKeAoUCnkKEgp6ChIKewoQCnwKEAp9Cg4KfgoO"
            "Cn8KDAqAAQoMCoEBCgoKggEKCgqDAQoICoQBCggKhQEKBgqGAQoGCocBCgQKiAEKBAqJAQoCCooB"
            "CgIKiwEUjAEUjQESjgESjwEQkAEQkQEOkgEOkwEMlAEMlQEKlgEKlwEImAEImQEGmgEGmwEEnAEE"
            "nQECngECrycA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 320, 320, 160, 160, 0xff652e89, 0,
            "rycCngECnQEEnAEEmwEGmgEGmQEImAEIlwEKlgEKlQEMlAEMkwEOkgEOkQEQkAEQjwESjgESjQEU"
            "jAEUiwEKAgqKAQoCCokBCgQKiAEKBAqHAQoGCoYBCgYKhQEKCAqEAQoICoMBCgoKggEKCgqBAQoM"
            "CoABCgwKfwoOCn4KDgp9ChAKfAoQCnsKEgp6ChIKeQoUCngKFAp3ChYKdgoWCnUKGAp0ChgKcwoa"
            "CnIKGgpxChwKcAocCm8KHgpuCh4KbQogCmwKIAprCiIKagoiCmkKJApoCiQKZwomCmYKJgplCigK"
            "ZAooCmMKKgpiCioKYQosCmAKLApfCi4KXgouCl0KMApcCjAKWwoyCloKMgpZCjQKWAo0ClcKNgpW"
            "CjYKVQo4ClQKOApTCjoKUgo6ClEKPApQCjwKTwo+Ck4KPgpNCkAKTApACksKQgpKCkIKSQpECkgK"
            "RApHWkZagzEA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 480, 320, 160, 160, 0xff652e89, 0,
            "gzFaRlpHCkQKSApECkkKQgpKCkIKSwpACkwKQApNCj4KTgo+Ck8KPApQCjwKUQo6ClIKOgpTCjgK"
            "VAo4ClUKNgpWCjYKVwo0ClgKNApZCjIKWgoyClsKMApcCjAKXQouCl4KLgpfCiwKYAosCmEKKgpi"
            "CioKYwooCmQKKAplCiYKZgomCmcKJApoCiQKaQoiCmoKIgprCiAKbAogCm0KHgpuCh4KbwocCnAK"
            "HApxChoKcgoaCnMKGAp0ChgKdQoWCnYKFgp3ChQKeAoUCnkKEgp6ChIKewoQCnwKEAp9Cg4KfgoO"
            "Cn8KDAqAAQoMCoEBCgoKggEKCgqDAQoICoQBCggKhQEKBgqGAQoGCocBCgQKiAEKBAqJAQoCCooB"
            "CgIKiwEUjAEUjQESjgESjwEQkAEQkQEOkgEOkwEMlAEMlQEKlgEKlwEImAEImQEGmgEGmwEEnAEE"
            "nQECngECrycA");
    ok(match, "Figure does not match.\n");

    set_rect(&rect, 20.0f, 80.0f, 60.0f, 240.0f);
    hr = ID2D1Factory_CreateRectangleGeometry(factory, &rect, &rect_geometry[0]);
    ok(SUCCEEDED(hr), "Failed to create geometry, hr %#x.\n", hr);

    set_rect(&rect, -1.0f, -1.0f, 1.0f, 1.0f);
    hr = ID2D1Factory_CreateRectangleGeometry(factory, &rect, &rect_geometry[1]);
    ok(SUCCEEDED(hr), "Failed to create geometry, hr %#x.\n", hr);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 160.0f, 640.0f);
    scale_matrix(&matrix, 40.0f, 160.0f);
    rotate_matrix(&matrix, M_PI / -5.0f);
    hr = ID2D1Factory_CreateTransformedGeometry(factory,
            (ID2D1Geometry *)rect_geometry[1], &matrix, &transformed_geometry[0]);
    ok(SUCCEEDED(hr), "Failed to create geometry, hr %#x.\n", hr);

    set_matrix_identity(&matrix);
    scale_matrix(&matrix, 0.5f, 1.0f);
    translate_matrix(&matrix, -80.0f, 0.0f);
    hr = ID2D1Factory_CreateTransformedGeometry(factory,
            (ID2D1Geometry *)transformed_geometry[0], &matrix, &transformed_geometry[1]);
    ok(SUCCEEDED(hr), "Failed to create geometry, hr %#x.\n", hr);

    set_matrix_identity(&matrix);
    rotate_matrix(&matrix, M_PI / 2.0f);
    translate_matrix(&matrix, 80.0f, -320.0f);
    scale_matrix(&matrix, 2.0f, 0.25f);
    hr = ID2D1Factory_CreateTransformedGeometry(factory,
            (ID2D1Geometry *)transformed_geometry[1], &matrix, &transformed_geometry[2]);
    ok(SUCCEEDED(hr), "Failed to create geometry, hr %#x.\n", hr);

    ID2D1RenderTarget_BeginDraw(rt);
    ID2D1RenderTarget_Clear(rt, &color);
    ID2D1RenderTarget_DrawGeometry(rt, (ID2D1Geometry *)rect_geometry[0], (ID2D1Brush *)brush, 10.0f, NULL);
    ID2D1RenderTarget_DrawGeometry(rt, (ID2D1Geometry *)transformed_geometry[0], (ID2D1Brush *)brush, 10.0f, NULL);
    ID2D1RenderTarget_DrawGeometry(rt, (ID2D1Geometry *)transformed_geometry[1], (ID2D1Brush *)brush, 5.0f, NULL);
    ID2D1RenderTarget_DrawGeometry(rt, (ID2D1Geometry *)transformed_geometry[2], (ID2D1Brush *)brush, 15.0f, NULL);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to end draw, hr %#x.\n", hr);
    ID2D1TransformedGeometry_Release(transformed_geometry[2]);
    ID2D1TransformedGeometry_Release(transformed_geometry[1]);
    ID2D1TransformedGeometry_Release(transformed_geometry[0]);
    ID2D1RectangleGeometry_Release(rect_geometry[1]);
    ID2D1RectangleGeometry_Release(rect_geometry[0]);

    match = compare_figure(&ctx,   0,   0, 160, 160, 0xff652e89, 0,
            "vi5kPGQ8ZDxkPGQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8"
            "FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8"
            "FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8ZDxkPGQ8ZDxk3i8A");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160,   0, 320, 160, 0xff652e89, 32,
            "8XYGtQIOrAIXpAIfmwIokwIwigI4gwJA+gFJ8gFR6QEzAiXhATMKJdgBMxMl0AEzGyXHATMkJb8B"
            "MysmtgEzNCWvATM8JaYBM0UlngEzTSWVATNWJY0BM14lhAEzZyV8M28lczN4JWszgAElYjOIASZa"
            "M5ABJVgtmQElWCWhASVYJaEBJVgloQElWCWhASVYJaEBJVgloQElWCWhASVYJaEBJVglmQEtWCWQ"
            "ATNaJogBM2IlgAEzayV4M3MlbzN8JWczhAElXjONASVWM5UBJU0zngElRTOmASU8M68BJTQztgEm"
            "KzO/ASUkM8cBJRsz0AElEzPYASUKM+EBJQIz6QFR8gFJ+gFAgwI4igIwkwIomwIfpAIXrAIOtQIG"
            "8XYA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx,   0, 160, 160, 320, 0xff652e89, 32,
            "ujEBngECnQEDnQEEmwEFmgEHmQEHmAEIlwEKlgEKlQELlAENkwENkgEOkQEQjwERjwESjQETjAEU"
            "jAEKAQqKAQoCCokBCgMKiQEKBAqHAQoFCoYBCgYKhgEKBwqEAQoICoMBCgkKgwEKCgqBAQoLCoAB"
            "Cg0KfgsNCn4KDgp9ChAKewsQCnsKEQp6ChMKeAoUCngKFAp3ChYKdQoXCnUKGApzChkKcgoaCnIK"
            "GwpwChwKbwodCm4LHgptCh8KbAogCmsLIQpqCiIKaQokCmcKJQpnCiUKZgonCmQKKApkCigKYwoq"
            "CmEKKwphCisKYAotCl4KLgpdCy8KXAowClsKMQpaCzIKWQozClgKNApXCjYKVgo2ClUKNwpUCjkK"
            "Uwo5ClIKOwpQCjwKTws8Ck8KPgpNCj8KTAs/CkwKQQpKCkIKSQtCCkkKRApHCkUKRgpHCkUKRwpE"
            "CkgKQwpKCkIKSgpBCksKQApNCj4LTQo+Ck4KPQpQCjsLUAo7ClIKOQpTCjgLUwo4ClUKNgpWCjUK"
            "Vwo1ClgKMwpZCjQKWAo0ClkKMwpZCjQKWQozClkKMwpZCjQKWQozClkKMwpZCjQKWQozClkKNApY"
            "CjQKWQozClkKNApZCjMKWQozClkKNApZCjMKWQozClkKNApZCjMKWQo0ClgKNApZCjMKWQo0ClkK"
            "MwpZCjMKWQo0ClkKMwpZCjMKWQo0ClkKMwpZCjQKWAo0ClkKMwpYCjUKVwo1ClYKNgpVCjgKUws4"
            "ClMKOQpSCjsKUAs7ClAKPQpOCj4KTQs+Ck0KQApLCkEKSgpCCkoKQwpICkQKRwpFCkcKRgpFCkcK"
            "RApJCkILSQpCCkoKQQpMCj8LTAo/Ck0KPgpPCjwLTwo8ClAKOwpSCjkKUwo5ClQKNwpVCjYKVgo2"
            "ClcKNApYCjMKWQoyC1oKMQpbCjAKXAovC10KLgpeCi0KYAorCmEKKwphCioKYwooCmQKKApkCicK"
            "ZgolCmcKJQpnCiQKaQoiCmoKIQtrCiAKbAofCm0KHgtuCh0KbwocCnAKGwpyChoKcgoZCnMKGAp1"
            "ChcKdQoWCncKFAp4ChQKeAoTCnoKEQp7ChALewoQCn0KDgp+Cg0LfgoNCoABCgsKgQEKCgqDAQoJ"
            "CoMBCggKhAEKBwqGAQoGCoYBCgUKhwEKBAqJAQoDCokBCgIKigEKAQqMARSMARONARKPARGPARCR"
            "AQ6SAQ2TAQ2UAQuVAQqWAQqXAQiYAQeZAQeaAQWbAQSdAQOdAQKeAQG6MQAA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160, 160, 320, 320, 0xff652e89, 64,
            "82ICvQIEugIHuAIJtgIKtAINsgIPsAIRrQITrAIVqQIYpwIZpgIbowIeoQIgnwIhnQIkmwImmAIp"
            "lgIVARSVAhUDFJICFQUVkAIVBxSPAhUJFIwCFQwUigIVDRWHAhYPFIYCFRIUhAIVFBSBAhUWFf8B"
            "FRgU/gEVGhT7ARUcFfkBFR4U9wEWIBT1ARUjFPMBFSQV8AEVJxTvARUpFOwBFisU6gEVLRXoARUv"
            "FOYBFjEU5AEVMxXiARU1FOABFTgU3gEVOhTbARY7FdkBFT4U2AEVQBTVARZCFNMBFUQV0QEVRhTP"
            "ARVJFM0BFUoVygEWTBTJARVPFMcBFVEUxAEVUxXCARVVFMEBFVcUvgEVWRW8ARVbFbkBFl0UuAEV"
            "YBS2ARVhFbMBFWQUsgEVZhSwARVoFK0BFWoVqwEVbBSpARZuFKcBFXAVpQEVchWiARV1FKEBFXcU"
            "nwEVeBWcARV7FJsBFX0UmAEWfxSWARWBARWUARWDARSSARWGARSQARWHARWOARWJARWLARWMARSK"
            "ARWOARSHARaPARWFARWSARSEARWUARSBARWXARR/FZgBFX0VmgEUexWdARR5FZ4BFXYWoAEVdBWj"
            "ARRzFaUBFHAVpwEVbhWpARRtFasBFGoVrgEUaBWvARVmFbEBFGcUsgEUZxSxARVmFbEBFWYUsgEU"
            "ZxSyARRnFLEBFWYVsQEUZxSyARRnFLIBFGcUsQEVZhWxARRnFLIBFGcUsQEVZhWxARVmFLIBFGcU"
            "sgEUZxSxARVmFbEBFGcUsgEUZxSyARRmFbEBFWYVsQEUZxSyARRnFLEBFWYVsQEUZxSyARRnFLIB"
            "FGcUsQEVZhWxARRnFLIBFGcUsgEUZhWxARVmFbEBFGcUsgEUZxSxARVmFa8BFWgUrgEVahSrARVt"
            "FKkBFW4VpwEVcBSlARVzFKMBFXQVoAEWdhWeARV5FJ0BFXsUmgEVfRWYARV/FJcBFYEBFJQBFYQB"
            "FJIBFYUBFY8BFocBFI4BFYoBFIwBFYsBFYkBFY4BFYcBFZABFIYBFZIBFIMBFZQBFYEBFZYBFH8W"
            "mAEUfRWbARR7FZwBFXgVnwEUdxWhARR1FaIBFXIVpQEVcBWnARRuFqkBFGwVqwEVahWtARRoFbAB"
            "FGYVsgEUZBWzARVhFbYBFGAVuAEUXRa5ARVbFbwBFVkVvgEUVxXBARRVFcIBFVMVxAEUURXHARRP"
            "FckBFEwWygEVShXNARRJFc8BFEYV0QEVRBXTARRCFtUBFEAV2AEUPhXZARU7FtsBFDoV3gEUOBXg"
            "ARQ1FeIBFTMV5AEUMRbmARQvFegBFS0V6gEUKxbsARQpFe8BFCcV8AEVJBXzARQjFfUBFCAW9wEU"
            "HhX5ARUcFfsBFBoV/gEUGBX/ARUWFYECFBQVhAIUEhWGAhQPFocCFQ0VigIUDBWMAhQJFY8CFAcV"
            "kAIVBRWSAhQDFZUCFAEVlgIpmAImmwIknQIhnwIgoQIeowIbpgIZpwIYqQIVrAITrQIRsAIPsgIN"
            "tAIKtgIJuAIHugIEvQIC82IA");
    ok(match, "Figure does not match.\n");

    hr = ID2D1Factory_CreatePathGeometry(factory, &geometry);
    ok(SUCCEEDED(hr), "Failed to create path geometry, hr %#x.\n", hr);
    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(SUCCEEDED(hr), "Failed to open geometry sink, hr %#x.\n", hr);

    set_point(&point, 20.0f, 80.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_HOLLOW);
    quadratic_to(sink, 40.0f,  80.0f, 60.0f,  80.0f);
    quadratic_to(sink, 60.0f, 160.0f, 60.0f, 240.0f);
    quadratic_to(sink, 40.0f, 240.0f, 20.0f, 240.0f);
    quadratic_to(sink, 20.0f, 160.0f, 20.0f,  80.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 100.0f, 80.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_HOLLOW);
    quadratic_to(sink, 105.0f,  80.0f, 140.0f,  80.0f);
    quadratic_to(sink, 140.0f, 100.0f, 140.0f, 240.0f);
    quadratic_to(sink, 135.0f, 240.0f, 100.0f, 240.0f);
    quadratic_to(sink, 100.0f, 220.0f, 100.0f,  80.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 180.0f, 80.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_HOLLOW);
    quadratic_to(sink, 215.0f,  80.0f, 220.0f,  80.0f);
    quadratic_to(sink, 220.0f, 220.0f, 220.0f, 240.0f);
    quadratic_to(sink, 185.0f, 240.0f, 180.0f, 240.0f);
    quadratic_to(sink, 180.0f, 100.0f, 180.0f,  80.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 260.0f, 80.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_HOLLOW);
    quadratic_to(sink, 280.0f,  80.0f, 300.0f,  80.0f);
    quadratic_to(sink, 300.0f, 160.0f, 300.0f, 240.0f);
    quadratic_to(sink, 280.0f, 240.0f, 260.0f, 240.0f);
    quadratic_to(sink, 260.0f, 160.0f, 260.0f,  80.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_OPEN);

    set_point(&point, 20.0f, 400.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_HOLLOW);
    quadratic_to(sink, 40.0f, 420.0f, 60.0f, 400.0f);
    quadratic_to(sink, 55.0f, 480.0f, 60.0f, 560.0f);
    quadratic_to(sink, 40.0f, 540.0f, 20.0f, 560.0f);
    quadratic_to(sink, 25.0f, 480.0f, 20.0f, 400.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 100.0f, 400.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_HOLLOW);
    quadratic_to(sink, 105.0f, 420.0f, 140.0f, 400.0f);
    quadratic_to(sink, 135.0f, 420.0f, 140.0f, 560.0f);
    quadratic_to(sink, 135.0f, 540.0f, 100.0f, 560.0f);
    quadratic_to(sink, 105.0f, 540.0f, 100.0f, 400.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 180.0f, 400.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_HOLLOW);
    quadratic_to(sink, 215.0f, 420.0f, 220.0f, 400.0f);
    quadratic_to(sink, 215.0f, 540.0f, 220.0f, 560.0f);
    quadratic_to(sink, 185.0f, 540.0f, 180.0f, 560.0f);
    quadratic_to(sink, 185.0f, 420.0f, 180.0f, 400.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 260.0f, 400.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_HOLLOW);
    quadratic_to(sink, 280.0f, 420.0f, 300.0f, 400.0f);
    quadratic_to(sink, 295.0f, 480.0f, 300.0f, 560.0f);
    quadratic_to(sink, 280.0f, 540.0f, 260.0f, 560.0f);
    quadratic_to(sink, 265.0f, 480.0f, 260.0f, 400.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_OPEN);

    set_point(&point, 20.0f, 720.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_HOLLOW);
    quadratic_to(sink, 40.0f, 700.0f, 60.0f, 720.0f);
    quadratic_to(sink, 65.0f, 800.0f, 60.0f, 880.0f);
    quadratic_to(sink, 40.0f, 900.0f, 20.0f, 880.0f);
    quadratic_to(sink, 15.0f, 800.0f, 20.0f, 720.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 100.0f, 720.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_HOLLOW);
    quadratic_to(sink, 105.0f, 700.0f, 140.0f, 720.0f);
    quadratic_to(sink, 145.0f, 740.0f, 140.0f, 880.0f);
    quadratic_to(sink, 135.0f, 900.0f, 100.0f, 880.0f);
    quadratic_to(sink,  95.0f, 860.0f, 100.0f, 720.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 180.0f, 720.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_HOLLOW);
    quadratic_to(sink, 215.0f, 700.0f, 220.0f, 720.0f);
    quadratic_to(sink, 225.0f, 860.0f, 220.0f, 880.0f);
    quadratic_to(sink, 185.0f, 900.0f, 180.0f, 880.0f);
    quadratic_to(sink, 175.0f, 740.0f, 180.0f, 720.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 260.0f, 720.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_HOLLOW);
    quadratic_to(sink, 280.0f, 700.0f, 300.0f, 720.0f);
    quadratic_to(sink, 305.0f, 800.0f, 300.0f, 880.0f);
    quadratic_to(sink, 280.0f, 900.0f, 260.0f, 880.0f);
    quadratic_to(sink, 255.0f, 800.0f, 260.0f, 720.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_OPEN);

    hr = ID2D1GeometrySink_Close(sink);
    ok(SUCCEEDED(hr), "Failed to close geometry sink, hr %#x.\n", hr);
    ID2D1GeometrySink_Release(sink);

    ID2D1RenderTarget_BeginDraw(rt);
    ID2D1RenderTarget_Clear(rt, &color);
    ID2D1RenderTarget_DrawGeometry(rt, (ID2D1Geometry *)geometry, (ID2D1Brush *)brush, 10.0f, NULL);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to end draw, hr %#x.\n", hr);
    ID2D1PathGeometry_Release(geometry);

    match = compare_figure(&ctx,   0,   0, 160, 160, 0xff652e89, 0,
            "vi5kPGQ8ZDxkPGQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8"
            "FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8"
            "FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8ZDxkPGQ8ZDxk3i8A");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160,   0, 160, 160, 0xff652e89, 0,
            "vi5kPGQ8ZDxkPGQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8"
            "FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8"
            "FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8ZDxkPGQ8ZDxk3i8A");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 320,   0, 160, 160, 0xff652e89, 0,
            "vi5kPGQ8ZDxkPGQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8"
            "FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8"
            "FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8ZDxkPGQ8ZDxk3i8A");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 480,   0, 160, 160, 0xff652e89, 0,
            "yC5aRlpGWjxkPGQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8"
            "FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8"
            "FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwU"
            "PBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8FDwUPBQ8ZDxkPGQ8ZDxk3i8A");
    ok(match, "Figure does not match.\n");

    match = compare_figure(&ctx,   0, 160, 160, 160, 0xff652e89, 64,
            "3SoDYAM6B1gHOgtQCzoPSA87EkASPBc2FzwcLBw8IiAiPWI+Yj5iPhQBOAEUPhQKJgoUPxQ4FEAU"
            "OBRAFDgUQBQ4FEAUOBRBFDYUQhQ2FEIUNhRCFDYUQhQ2FEIUNhRDFDQURBQ0FEQUNBREFDQURBQ0"
            "FEQUNBREFDQURBQ0FEQUNBREFDQURRQyFEYUMhRGFDIURhQyFEYUMhRGFDIURhQyFEYUMhRGFDIU"
            "RhQyFEYUMhRGFDIURhQyFEYUMhRGFDIURhQyFEYUMhRGFDIURhQyFEYUMhRGFDIURhQyFEYUMhRG"
            "FDIURRQ0FEQUNBREFDQURBQ0FEQUNBREFDQURBQ0FEQUNBREFDQURBQ0FEMUNhRCFDYUQhQ2FEIU"
            "NhRCFDYUQhQ2FEEUOBRAFDgUQBQ4FEAUOBRAFDgUPxQKJgoUPhQBOAEUPmI+Yj5iPSIgIjwcLBw8"
            "FzYXPBJAEjsPSA86C1ALOgdYBzoDYAPdKgAA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160, 160, 160, 160, 0xff652e89, 1024,
            "uxUBnwECngEDnQEEnAEFmwEGmwEGmgEHmQEImAEJlwEKlgELlQEMlQEMlAENkwEOkgEPkQEQkAER"
            "VQQ2Ek0KOBJFEDkTPRY6FDUcOxUrJDwYHi09Yj5iP2BAQwkUQDgUFEAUOBRAFDcUQRQ3FEEUNxRC"
            "FDYUQhQ2FEIUNhRCFDUUQxQ1FEMUNRRDFDUUQxQ1FEQUNBREFDQURBQ0FEQUNBREFDQURBQ0FEQU"
            "NBREFDQURBQ0FEQUNBRFFDMURRQzFEUUMxRFFDMURRQzFEUUMxRFFDMURRQzFEUUMxRFFDMURRQ0"
            "FEQUNBRFFDMURRQzFEUUMxRFFDMURRQzFEUUMxRFFDMURRQzFEUUMxRFFDMURRQ0FEQUNBREFDQU"
            "RBQ0FEQUNBREFDQURBQ0FEQUNBREFDQURBQ0FEQUNRRDFDUUQxQ1FEMUNRRDFDUUQhQ2FEIUNhRC"
            "FDYUQhQ3FEEUNxRBFDcUQBQ4FEAUFDhAFAlDQGA/Yj5iPS0eGDwkKxU7HDUUOhY9EzkQRRI4Ck0S"
            "NgRVEZABEJEBD5IBDpMBDZQBDJUBDJUBC5YBCpcBCZgBCJkBB5oBBpsBBpsBBZwBBJ0BA54BAp8B"
            "AbsV");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 320, 160, 160, 160, 0xff652e89, 1024,
            "pBYBngECnQEDnAEEmwEFmgEGmQEGmQEHmAEIlwEJlgEKlQELlAEMkwEMkwENkgEOkQEPkAEQNgRV"
            "ETcKTRI4EEUSOhY9EzscNRQ8JCsVPS0eGD5iPmI/YEAUCUNAFBQ4QBQ4FEEUNxRBFDcUQRQ3FEEU"
            "NhRCFDYUQhQ2FEMUNRRDFDUUQxQ1FEMUNRRDFDUUQxQ0FEQUNBREFDQURBQ0FEQUNBREFDQURBQ0"
            "FEQUNBREFDQURBQ0FEQUMxRFFDMURRQzFEUUMxRFFDMURRQzFEUUMxRFFDMURRQzFEUUMxREFDQU"
            "RBQ0FEQUMxRFFDMURRQzFEUUMxRFFDMURRQzFEUUMxRFFDMURRQzFEUUMxREFDQURBQ0FEQUNBRE"
            "FDQURBQ0FEQUNBREFDQURBQ0FEQUNBREFDQUQxQ1FEMUNRRDFDUUQxQ1FEMUNRRDFDYUQhQ2FEIU"
            "NhRBFDcUQRQ3FEEUNxRBFDgUQDgUFEBDCRRAYD9iPmI+GB4tPRUrJDwUNRw7Ez0WOhJFEDgSTQo3"
            "EVUENhCQAQ+RAQ6SAQ2TAQyTAQyUAQuVAQqWAQmXAQiYAQeZAQaZAQaaAQWbAQScAQOdAQKeAQGk"
            "FgAA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 480, 160, 160, 160, 0xff652e89, 64,
            "wCsDmQEHlQELkQEPSwJAEkgLNhc8HCwcPCIgIj1iPmI+Yj4UATgBFD4UCiYKFD8UOBRAFDgUQBQ4"
            "FEAUOBRAFDgUQRQ2FEIUNhRCFDYUQhQ2FEIUNhRCFDYUQxQ0FEQUNBREFDQURBQ0FEQUNBREFDQU"
            "RBQ0FEQUNBREFDQURBQ0FEUUMhRGFDIURhQyFEYUMhRGFDIURhQyFEYUMhRGFDIURhQyFEYUMhRG"
            "FDIURhQyFEYUMhRGFDIURhQyFEYUMhRGFDIURhQyFEYUMhRGFDIURhQyFEYUMhRGFDIURhQyFEUU"
            "NBREFDQURBQ0FEQUNBREFDQURBQ0FEQUNBREFDQURBQ0FEQUNBRDFDYUQhQ2FEIUNhRCFDYUQhQ2"
            "FEIUNhRBFDgUQBQ4FEAUOBRAFDgUQBQ4FD8UCiYKFD4UATgBFD5iPmI+Yj0iICI8HCwcPBc2FzwS"
            "QBI7D0gPOgtQCzoHWAc6A2AD3SoA");
    ok(match, "Figure does not match.\n");

    match = compare_figure(&ctx,   0, 320, 160, 160, 0xff652e89, 64,
            "3SkmcThiRFdOTVhEICAgPhwsHDwXNhc8FDwUOxQ+FDoUPhQ6FD4UOhQ+FDoUPhQ5FEAUOBRAFDgU"
            "QBQ4FEAUOBRAFDcUQhQ2FEIUNhRCFDYUQhQ2FEIUNhRCFDUURBQ0FEQUNBREFDQURBQ0FEQUNBRE"
            "FDQURBQ0FEQUNBREFDQURBQzFEYUMhRGFDIURhQyFEYUMhRGFDIURhQyFEYUMhRGFDIURhQyFEYU"
            "MhRGFDIURhQyFEYUMhRGFDIURhQyFEYUMhRGFDIURhQyFEYUMhRGFDIURhQyFEYUMhRGFDIURhQz"
            "FEQUNBREFDQURBQ0FEQUNBREFDQURBQ0FEQUNBREFDQURBQ0FEQUNRRCFDYUQhQ2FEIUNhRCFDYU"
            "QhQ2FEIUNxRAFDgUQBQ4FEAUOBRAFDgUQBQ5FD4UOhQ+FDoUPhQ6FD4UOhQ+FDsUPBQ8FzYXPBws"
            "HD4gICBEWE1OV0RiOHEm3SkA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160, 320, 160, 160, 0xff652e89, 1024,
            "zykoczhkRVhQTlpEFx4tPRUrJDwUNRw7FDwVOxQ+FDoUPhQ5FEAUOBRAFDgUQBQ4FEAUOBRBFDcU"
            "QRQ3FEEUNhRCFDYUQhQ2FEIUNhRDFDUUQxQ1FEMUNRRDFDUUQxQ0FEQUNBREFDQURBQ0FEQUNBRE"
            "FDQURBQ0FEQUNBREFDQURBQ0FEQUMxRFFDMURRQzFEUUMxRFFDMURRQzFEUUMxRFFDMURRQzFEUU"
            "MxRFFDMURBQ0FEQUMxRFFDMURRQzFEUUMxRFFDMURRQzFEUUMxRFFDMURRQzFEUUMxRFFDMURBQ0"
            "FEQUNBREFDQURBQ0FEQUNBREFDQURBQ0FEQUNBREFDQURBQ0FEMUNRRDFDUUQxQ1FEMUNRRDFDYU"
            "QhQ2FEIUNhRCFDYUQRQ3FEEUNxRBFDgUQBQ4FEAUOBRAFDgUQBQ5FD4UOhQ+FDsVPBQ7HDUUPCQr"
            "FT0tHhdEWk5QWEVkOHMozykA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 320, 320, 160, 160, 0xff652e89, 1024,
            "6SkobThfRVNQSFpALR4XPSQrFTscNRQ7FTwUOhQ+FDoUPhQ5FEAUOBRAFDgUQBQ4FEAUNxRBFDcU"
            "QRQ3FEEUNxRCFDYUQhQ2FEIUNRRDFDUUQxQ1FEMUNRRDFDUUQxQ1FEQUNBREFDQURBQ0FEQUNBRE"
            "FDQURBQ0FEQUNBREFDQURBQ0FEQUNBRFFDMURRQzFEUUMxRFFDMURRQzFEUUMxRFFDMURRQzFEUU"
            "MxRFFDQURBQ0FEQUNBRFFDMURRQzFEUUMxRFFDMURRQzFEUUMxRFFDMURRQzFEUUMxRFFDQURBQ0"
            "FEQUNBREFDQURBQ0FEQUNBREFDQURBQ0FEQUNBREFDQURBQ1FEMUNRRDFDUUQxQ1FEMUNRRDFDUU"
            "QhQ2FEIUNhRCFDcUQRQ3FEEUNxRBFDcUQBQ4FEAUOBRAFDgUQBQ5FD4UOhQ+FDoUPBU7FDUcOxUr"
            "JD0XHi1AWkhQU0VfOG0o6SkA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 480, 320, 160, 160, 0xff652e89, 64,
            "3SkmcThiRFdOTVhGHiAgRhQsHDwXNhc8FDwUOxQ+FDoUPhQ6FD4UOhQ+FDoUPhQ5FEAUOBRAFDgU"
            "QBQ4FEAUOBRAFDcUQhQ2FEIUNhRCFDYUQhQ2FEIUNhRCFDUURBQ0FEQUNBREFDQURBQ0FEQUNBRE"
            "FDQURBQ0FEQUNBREFDQURBQzFEYUMhRGFDIURhQyFEYUMhRGFDIURhQyFEYUMhRGFDIURhQyFEYU"
            "MhRGFDIURhQyFEYUMhRGFDIURhQyFEYUMhRGFDIURhQyFEYUMhRGFDIURhQyFEYUMhRGFDIURhQz"
            "FEQUNBREFDQURBQ0FEQUNBREFDQURBQ0FEQUNBREFDQURBQ0FEQUNRRCFDYUQhQ2FEIUNhRCFDYU"
            "QhQ2FEIUNxRAFDgUQBQ4FEAUOBRAFDgUQBQ5FD4UOhQ+FDoUPhQ6FD4UOhQ+FDsUPBQ8FzYXPBws"
            "HD4gICBEWE1OV0RiOHEm3SkA");
    ok(match, "Figure does not match.\n");

    hr = ID2D1Factory_CreatePathGeometry(factory, &geometry);
    ok(SUCCEEDED(hr), "Failed to create path geometry, hr %#x.\n", hr);
    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(SUCCEEDED(hr), "Failed to open geometry sink, hr %#x.\n", hr);

    set_point(&point, -0.402914f, 0.915514f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_HOLLOW);
    quadratic_to(sink, -0.310379f,  0.882571f, -0.116057f,  0.824000f);
    quadratic_to(sink,  0.008350f,  0.693614f, -0.052343f,  0.448886f);
    quadratic_to(sink, -0.154236f,  0.246072f, -0.279229f,  0.025343f);
    quadratic_to(sink, -0.370064f, -0.588586f, -0.383029f, -0.924114f);
    quadratic_to(sink, -0.295479f, -0.958764f, -0.017086f, -0.988400f);
    quadratic_to(sink,  0.208836f, -0.954157f,  0.272200f, -0.924114f);
    quadratic_to(sink,  0.295614f, -0.569071f,  0.230143f,  0.022886f);
    quadratic_to(sink,  0.101664f,  0.220643f,  0.012057f,  0.451571f);
    quadratic_to(sink, -0.028764f,  0.709014f,  0.104029f,  0.833943f);
    quadratic_to(sink,  0.319414f,  0.913057f,  0.403229f,  0.942628f);
    quadratic_to(sink,  0.317721f,  1.023450f, -0.017086f,  1.021771f);
    quadratic_to(sink, -0.310843f,  1.007472f, -0.402914f,  0.915514f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    hr = ID2D1GeometrySink_Close(sink);
    ok(SUCCEEDED(hr), "Failed to close geometry sink, hr %#x.\n", hr);
    ID2D1GeometrySink_Release(sink);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 40.0f, 160.0f);
    scale_matrix(&matrix, 20.0f, 80.0f);
    hr = ID2D1Factory_CreateTransformedGeometry(factory,
            (ID2D1Geometry *)geometry, &matrix, &transformed_geometry[0]);
    ok(SUCCEEDED(hr), "Failed to create geometry, hr %#x.\n", hr);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 160.0f, 640.0f);
    scale_matrix(&matrix, 40.0f, 160.0f);
    rotate_matrix(&matrix, M_PI / -5.0f);
    hr = ID2D1Factory_CreateTransformedGeometry(factory,
            (ID2D1Geometry *)geometry, &matrix, &transformed_geometry[1]);
    ok(SUCCEEDED(hr), "Failed to create geometry, hr %#x.\n", hr);
    ID2D1PathGeometry_Release(geometry);

    set_matrix_identity(&matrix);
    scale_matrix(&matrix, 0.5f, 1.0f);
    translate_matrix(&matrix, -80.0f, 0.0f);
    hr = ID2D1Factory_CreateTransformedGeometry(factory,
            (ID2D1Geometry *)transformed_geometry[1], &matrix, &transformed_geometry[2]);
    ok(SUCCEEDED(hr), "Failed to create geometry, hr %#x.\n", hr);

    set_matrix_identity(&matrix);
    rotate_matrix(&matrix, M_PI / 2.0f);
    translate_matrix(&matrix, 80.0f, -320.0f);
    scale_matrix(&matrix, 2.0f, 0.25f);
    hr = ID2D1Factory_CreateTransformedGeometry(factory,
            (ID2D1Geometry *)transformed_geometry[2], &matrix, &transformed_geometry[3]);
    ok(SUCCEEDED(hr), "Failed to create geometry, hr %#x.\n", hr);

    ID2D1RenderTarget_BeginDraw(rt);
    ID2D1RenderTarget_Clear(rt, &color);
    ID2D1RenderTarget_DrawGeometry(rt, (ID2D1Geometry *)transformed_geometry[0], (ID2D1Brush *)brush, 2.0f, NULL);
    ID2D1RenderTarget_DrawGeometry(rt, (ID2D1Geometry *)transformed_geometry[1], (ID2D1Brush *)brush, 10.0f, NULL);
    ID2D1RenderTarget_DrawGeometry(rt, (ID2D1Geometry *)transformed_geometry[2], (ID2D1Brush *)brush, 5.0f, NULL);
    ID2D1RenderTarget_DrawGeometry(rt, (ID2D1Geometry *)transformed_geometry[3], (ID2D1Brush *)brush, 15.0f, NULL);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to end draw, hr %#x.\n", hr);
    ID2D1TransformedGeometry_Release(transformed_geometry[3]);
    ID2D1TransformedGeometry_Release(transformed_geometry[2]);
    ID2D1TransformedGeometry_Release(transformed_geometry[1]);
    ID2D1TransformedGeometry_Release(transformed_geometry[0]);

    match = compare_figure(&ctx,   0,   0, 160, 160, 0xff652e89, 128,
            "yjIJkQEHBwaIAQUSBYMBBBYEggEEFgSCAQQWBIIBBBYEggEEFgSCAQQWBIIBBBYEggEEFgSCAQQW"
            "BIIBBBYEggEEFgSDAQQVBIMBBBUEgwEEFQSDAQQVBIMBBBUEgwEEFQSDAQQVBIMBBBUEgwEEFQSD"
            "AQQVBIQBBBQEhAEEFASEAQQTBIUBBBMEhQEEEwSFAQQTBIUBBBMEhQEEEwSGAQQSBIYBBBIEhgEE"
            "EgSGAQQSBIYBBBIEhgEEEgSGAQQRBIgBBBAEiAEEEASIAQQQBIkBBA4EigEEDgSLAQQMBIwBBAwE"
            "jQEECgSOAQQJBJABBAgEkAEFBgSSAQQGBJMBBAQElAEEBASVAQQDBJUBBAIElwEEAQSXAQiZAQeZ"
            "AQaaAQaaAQaaAQabAQWbAQWbAQWbAQWaAQeZAQeZAQeZAQiXAQQBBJYBBAMElQEEAwWRAQUGBY0B"
            "BQwFhwEFEgSCAQUXBYABBBoFfgUYBIIBBhEFiAEUpTEA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160,   0, 320, 160, 0xff652e89, 512,
            "yJIBArkCDa4CGKMCIZoCK5ECM4gCO4ECQ/gBS/EBUesBLAYl5QEsDiPeASwWIdkBLBwh0wEsISHO"
            "ASsgKMsBKR4vyAEnHDPIASUaNMsBIxg1mQEFMCIUN54BCygiDzijAREhIgY9qAEYGWGuAR4RXbMB"
            "JAhbuQGAAcABesYBc84Ba9YBTvQBP4MCOIoCNI4CM5ACMZICL5QCLZYCK5kCKJsCJ54CI6MCHq8C"
            "EraSAQAA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx,   0, 160, 160, 320, 0xff652e89, 512,
            "xWkCmwEFmAEJlQELlAENkgEOkQEPjwESjQETjAEVigELAQqJAQsCCogBCwQKhwEKBQqGAQoGCoYB"
            "CgcKhAEKCAqEAQoIC4IBCgoKggEKCgqBAQoMCoABCgwKfwoNCn8KDgp9Cg8KfQoPCnwKEQp7ChEK"
            "egoSCnoKEwp4ChQKeAoUCncLFQp2ChYKdgoWCnYKFwp2ChYKdgoWCncKFgp2ChYKdgoWCncKFQt2"
            "ChYKdwoVCncKFQp4ChUKdwoVCncKFQp4ChUKdwoVCngKFAp4ChUKeAoUCngKFAp4CxMKeQoUCngK"
            "FAp5ChMKeQoUCnkKEwp5ChMKegoSC3kKEwp6ChIKegoSCnoLEgp6ChIKegoSCnsKEQp7ChEKfAoQ"
            "CnwKEAp9Cg8KfQoPCn4KDgp+Cg4KfwoOCn4KDgp/Cg0KfwoNCoABCgwKgAEKDAqBAQoLCoEBCgsK"
            "gQELCgqCAQoKCoIBCwkKgwEKCQqDAQoJCoQBCggKhAEKCQqEAQsHCoUBCwYKhgELBQqHAQsECogB"
            "CwMKiQELAgqLAQoBCowBFI0BE44BE44BEo8BEZABEJEBD5IBDpMBDpMBDZMBDZQBDJQBDZQBDJQB"
            "DBUCfgwSBH4MEQV/DA4GgAEMDAiAAQ0KCYEBDAgLgQENBQ2BAQ0EDoIBDQEPgwEdgwEdgwEdgwEc"
            "hAEKAgQCCoUBCgYKhgEKBgqGAQoFC4YBCgUKhwEKBAqIAQoECogBCgMKiQEKAwqIAQoDCokBCgMK"
            "iQEKAgqJAQoCCooBCgIKiQEKAgqKAQoBCosBCgEKigEKAQqLARSMARSLARSMAROMARONARKOARGO"
            "ARGPARCQAQ6RAQ2YAQTEZAAA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160, 160, 320, 320, 0xff652e89, 1024,
            "ytABA7gCCbICD60CFKkCF6cCGqMCHqACIZ0CJJoCJpgCKZUCFgIUkgIWBBWPAhYHFI4CFQoUjAIV"
            "DBSKAhUNFYgCFQ8UhwIVERSFAhUTFIMCFRQVgQIUFxSAAhQZFP4BFBoV/AEUHBT7ARQeFPkBFB8V"
            "9wEUIRT2ARQjFPQBFSMV8gEVJRTxARUnFPABFCgV7gEUKhTtARQsFOwBFCwV7AEULBTsARUsFOwB"
            "FSsV7AEULBTtARQsFO0BFCsU7QEVKxTtARUqFe0BFSoU7gEUKxTuARQqFe4BFCoU7wEUKhTuARUp"
            "FO8BFSkU7wEVKBXvARUoFPABFCkU8AEUKBTxARQoFPEBFCcV8QEUJxTxARUnFPEBFSYU8gEVJhTy"
            "ARUlFfIBFSUU8wEUJRXzARQlFPQBFCUU9AEUJBT1ARQkFPUBFCMU9gEUIhT2ARUhFPcBFSAU+AEV"
            "HxT5ARUeFPoBFR4U+gEVHRT7ARUcFPwBFRsU/QEVGhT+ARUZFP8BFBkUgAIUGBSBAhQXFIICFBcU"
            "ggIUFhSDAhQVFIQCFBQUhQIUExSGAhQSFIcCFBIUhwIUERSIAhUPFIkCFg0UigIXCxSNAhYJFI8C"
            "FggUkAIXBRSSAhcDFJQCFwEUlgIrlwIpmgImnAIkngIjnwIhoQIfowIepAIcpgIbpgIaqAIZqAIZ"
            "qAIYKwP7ARgnBf0BGCMI/QEZHgz+ARgbD/8BGBcSgAIYEhaAAhoNGIICGggcgwIaBB+DAjyEAjyF"
            "AjqGAjmIAjiIAiECFIkCFAIIBBSKAhQNFIsCFAwUjAIUCxSNAhQKFI4CFAkUjwIUBxWQAhQGFZEC"
            "FAUVkQIUBRWRAhQFFZECFQMVkwIUAxWTAhQDFZMCFAIVlAIVARWVAiqVAimWAimWAiiYAiaZAiaZ"
            "AiWaAiScAiKdAiGeAh+hAhyjAhmuAg3GxgEA");
    ok(match, "Figure does not match.\n");

    ID2D1SolidColorBrush_Release(brush);
    ID2D1Factory_Release(factory);
    release_test_context(&ctx);
}

static void test_fill_geometry(BOOL d3d11)
{
    ID2D1TransformedGeometry *transformed_geometry[4];
    ID2D1RectangleGeometry *rect_geometry[2];
    D2D1_POINT_2F point = {0.0f, 0.0f};
    D2D1_ROUNDED_RECT rounded_rect;
    struct d2d1_test_context ctx;
    ID2D1SolidColorBrush *brush;
    ID2D1PathGeometry *geometry;
    D2D1_MATRIX_3X2_F matrix;
    ID2D1GeometrySink *sink;
    ID2D1RenderTarget *rt;
    ID2D1Factory *factory;
    D2D1_ELLIPSE ellipse;
    D2D1_COLOR_F color;
    D2D1_RECT_F rect;
    HRESULT hr;
    BOOL match;

    if (!init_test_context(&ctx, d3d11))
        return;

    rt = ctx.rt;
    ID2D1RenderTarget_GetFactory(rt, &factory);

    ID2D1RenderTarget_SetDpi(rt, 192.0f, 48.0f);
    ID2D1RenderTarget_SetAntialiasMode(rt, D2D1_ANTIALIAS_MODE_ALIASED);
    set_color(&color, 0.890f, 0.851f, 0.600f, 1.0f);
    hr = ID2D1RenderTarget_CreateSolidColorBrush(rt, &color, NULL, &brush);
    ok(SUCCEEDED(hr), "Failed to create brush, hr %#x.\n", hr);

    ID2D1RenderTarget_BeginDraw(rt);
    set_color(&color, 0.396f, 0.180f, 0.537f, 1.0f);
    ID2D1RenderTarget_Clear(rt, &color);

    set_rect(&rect, 40.0f, 480.0f, 40.0f, 480.0f);
    ID2D1RenderTarget_FillRectangle(rt, &rect, (ID2D1Brush *)brush);
    set_rect(&rect, 100.0f, 480.0f, 140.0f, 480.0f);
    ID2D1RenderTarget_FillRectangle(rt, &rect, (ID2D1Brush *)brush);
    set_rect(&rect, 200.0f, 400.0f, 200.0f, 560.0f);
    ID2D1RenderTarget_FillRectangle(rt, &rect, (ID2D1Brush *)brush);
    set_rect(&rect, 260.0f, 560.0f, 300.0f, 400.0f);
    ID2D1RenderTarget_FillRectangle(rt, &rect, (ID2D1Brush *)brush);

    set_ellipse(&ellipse,  40.0f, 800.0f,  0.0f,  0.0f);
    ID2D1RenderTarget_FillEllipse(rt, &ellipse, (ID2D1Brush *)brush);
    set_ellipse(&ellipse, 120.0f, 800.0f, 20.0f,  0.0f);
    ID2D1RenderTarget_FillEllipse(rt, &ellipse, (ID2D1Brush *)brush);
    set_ellipse(&ellipse, 200.0f, 800.0f,  0.0f, 80.0f);
    ID2D1RenderTarget_FillEllipse(rt, &ellipse, (ID2D1Brush *)brush);
    set_ellipse(&ellipse, 280.0f, 800.0f, 20.0f, 80.0f);
    ID2D1RenderTarget_FillEllipse(rt, &ellipse, (ID2D1Brush *)brush);

    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to end draw, hr %#x.\n", hr);

    match = compare_figure(&ctx,   0, 160, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160, 160, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 320, 160, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 480, 160, 160, 160, 0xff652e89, 0,
            "qDJQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQ"
            "UFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQ"
            "UFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFCoMgAA");
    ok(match, "Figure does not match.\n");

    match = compare_figure(&ctx,   0, 320, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160, 320, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 320, 320, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 480, 320, 160, 160, 0xff652e89, 8,
            "yjIMjwEWhwEcggEgfiR6KHYscy5xMG40azZpOGc6ZTxjPmI+YUBfQl1EXERbRlpGWUhYSFdKVkpV"
            "TFRMVExTTlJOUk5STlJOUVBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUU5STlJOUk5STlNMVExUTFVK"
            "VkpXSFhIWUZaRltEXERdQl9AYT5iPmM8ZTpnOGk2azRuMHEucyx2KHokfiCCARyHARaPAQzKMgAA");
     ok(match, "Figure does not match.\n");

    ID2D1RenderTarget_BeginDraw(rt);
    set_color(&color, 0.396f, 0.180f, 0.537f, 1.0f);
    ID2D1RenderTarget_Clear(rt, &color);

    set_rounded_rect(&rounded_rect, 40.0f, 160.0f, 40.0f, 160.0f, 10.0f, 10.0f);
    ID2D1RenderTarget_FillRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush);
    set_rounded_rect(&rounded_rect, 100.0f, 160.0f, 140.0f, 160.0f, 10.0f, 10.0f);
    ID2D1RenderTarget_FillRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush);
    set_rounded_rect(&rounded_rect, 200.0f, 80.0f, 200.0f, 240.0f, 10.0f, 10.0f);
    ID2D1RenderTarget_FillRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush);
    set_rounded_rect(&rounded_rect, 260.0f, 240.0f, 300.0f, 80.0f, 10.0f, 10.0f);
    ID2D1RenderTarget_FillRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush);

    set_rounded_rect(&rounded_rect, 40.0f, 480.0f, 40.0f, 480.0f, 10.0f, 20.0f);
    ID2D1RenderTarget_FillRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush);
    set_rounded_rect(&rounded_rect, 100.0f, 480.0f, 140.0f, 480.0f, 10.0f, 20.0f);
    ID2D1RenderTarget_FillRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush);
    set_rounded_rect(&rounded_rect, 200.0f, 400.0f, 200.0f, 560.0f, 10.0f, 20.0f);
    ID2D1RenderTarget_FillRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush);
    set_rounded_rect(&rounded_rect, 260.0f, 560.0f, 300.0f, 400.0f, 10.0f, 20.0f);
    ID2D1RenderTarget_FillRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush);

    set_rounded_rect(&rounded_rect, 40.0f, 800.0f, 40.0f, 800.0f, 10.0f, 5.0f);
    ID2D1RenderTarget_FillRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush);
    set_rounded_rect(&rounded_rect, 100.0f, 800.0f, 140.0f, 800.0f, 10.0f, 5.0f);
    ID2D1RenderTarget_FillRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush);
    set_rounded_rect(&rounded_rect, 200.0f, 720.0f, 200.0f, 880.0f, 10.0f, 5.0f);
    ID2D1RenderTarget_FillRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush);
    set_rounded_rect(&rounded_rect, 260.0f, 880.0f, 300.0f, 720.0f, 10.0f, 5.0f);
    ID2D1RenderTarget_FillRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush);

    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to end draw, hr %#x.\n", hr);

    match = compare_figure(&ctx,   0,   0, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160,   0, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 320,   0, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 480,   0, 160, 160, 0xff652e89, 0,
            "szI6YURZSlROUVBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQ"
            "UFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQ"
            "UFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUU5USllEYTqzMgAA");
    ok(match, "Figure does not match.\n");

    match = compare_figure(&ctx,   0, 160, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160, 160, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 320, 160, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 480, 160, 160, 160, 0xff652e89, 2,
            "tjI0aDxhQlxGWEpVTFNOUk5RUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQ"
            "UFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQ"
            "UFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFFOUk5TTFVKWEZcQmA+ZzS2MgAA");
    ok(match, "Figure does not match.\n");

    match = compare_figure(&ctx,   0, 320, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160, 320, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 320, 320, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 480, 320, 160, 160, 0xff652e89, 0,
            "sDJAWkxSUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQ"
            "UFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQ"
            "UFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFJMWkCwMgAA");
    ok(match, "Figure does not match.\n");

    ID2D1RenderTarget_BeginDraw(rt);
    set_color(&color, 0.396f, 0.180f, 0.537f, 1.0f);
    ID2D1RenderTarget_Clear(rt, &color);

    set_rounded_rect(&rounded_rect, 40.0f, 160.0f, 40.0f, 160.0f, 1000.0f, 1000.0f);
    ID2D1RenderTarget_FillRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush);
    set_rounded_rect(&rounded_rect, 100.0f, 160.0f, 140.0f, 160.0f, 1000.0f, 1000.0f);
    ID2D1RenderTarget_FillRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush);
    set_rounded_rect(&rounded_rect, 200.0f, 80.0f, 200.0f, 240.0f, 1000.0f, 1000.0f);
    ID2D1RenderTarget_FillRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush);
    set_rounded_rect(&rounded_rect, 260.0f, 240.0f, 300.0f, 80.0f, 1000.0f, 1000.0f);
    ID2D1RenderTarget_FillRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush);

    set_rounded_rect(&rounded_rect, 40.0f, 480.0f, 40.0f, 480.0f, 10.0f, 1000.0f);
    ID2D1RenderTarget_FillRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush);
    set_rounded_rect(&rounded_rect, 100.0f, 480.0f, 140.0f, 480.0f, 10.0f, 1000.0f);
    ID2D1RenderTarget_FillRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush);
    set_rounded_rect(&rounded_rect, 200.0f, 400.0f, 200.0f, 560.0f, 10.0f, 1000.0f);
    ID2D1RenderTarget_FillRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush);
    set_rounded_rect(&rounded_rect, 260.0f, 560.0f, 300.0f, 400.0f, 10.0f, 1000.0f);
    ID2D1RenderTarget_FillRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush);

    set_rounded_rect(&rounded_rect, 40.0f, 800.0f, 40.0f, 800.0f, 1000.0f, 10.0f);
    ID2D1RenderTarget_FillRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush);
    set_rounded_rect(&rounded_rect, 100.0f, 800.0f, 140.0f, 800.0f, 1000.0f, 10.0f);
    ID2D1RenderTarget_FillRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush);
    set_rounded_rect(&rounded_rect, 200.0f, 720.0f, 200.0f, 880.0f, 1000.0f, 10.0f);
    ID2D1RenderTarget_FillRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush);
    set_rounded_rect(&rounded_rect, 260.0f, 880.0f, 300.0f, 720.0f, 1000.0f, 10.0f);
    ID2D1RenderTarget_FillRoundedRectangle(rt, &rounded_rect, (ID2D1Brush *)brush);

    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to end draw, hr %#x.\n", hr);

    match = compare_figure(&ctx,   0,   0, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160,   0, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 320,   0, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 480,   0, 160, 160, 0xff652e89, 10,
            "yjIMjwEWhwEcggEgfiR6KHYscy5xMG40azZpOGc6ZTxjPmI+YUBfQl1EXERbRlpGWUhYSFdKVkpV"
            "TFRMVExTTlJOUk5STlJOUVBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUU5STlJOUk5STlNMVExUTFVK"
            "VkpXSFhIWUZaRltEXERdQl9AYT5iPmM8ZTpnOGk2azRuMHEucyx2KHokfiCCARyHARaPAQzKMgAA");
    ok(match, "Figure does not match.\n");

    match = compare_figure(&ctx,   0, 160, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160, 160, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 320, 160, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 480, 160, 160, 160, 0xff652e89, 10,
            "uTIucDJsNmk4ZzplPGM+YUBgQF9CXkJdRFxEW0ZaRllIWEhXSlZKVkpWSlVMVExUTFRMU05STlJO"
            "Uk5STlJOUk9QUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFFPUU5STlJOUk5STlJOU0xU"
            "TFRMVExVSlZKVkpWSldIWEhZRlpGW0RcRF1CXkJfQGBAYT5jPGU6ZzhpNmwycC65MgAA");
    ok(match, "Figure does not match.\n");

    match = compare_figure(&ctx,   0, 320, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160, 320, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 320, 320, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 480, 320, 160, 160, 0xff652e89, 10,
            "vzIiczhhRldMUlBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQ"
            "UFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQ"
            "UFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUkxXRmA6cSS+MgAA");
    ok(match, "Figure does not match.\n");

    hr = ID2D1Factory_CreatePathGeometry(factory, &geometry);
    ok(SUCCEEDED(hr), "Failed to create path geometry, hr %#x.\n", hr);
    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(SUCCEEDED(hr), "Failed to open geometry sink, hr %#x.\n", hr);

    set_point(&point, 40.0f, 160.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_OPEN);

    set_point(&point, 120.0f, 160.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    line_to(sink, 120.0f, 160.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_OPEN);

    set_point(&point, 200.0f, 160.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 280.0f, 160.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    line_to(sink, 280.0f, 160.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 20.0f, 480.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    line_to(sink, 60.0f, 480.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_OPEN);

    set_point(&point, 120.0f, 400.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    line_to(sink, 120.0f, 560.0f);
    line_to(sink, 120.0f, 400.0f);
    line_to(sink, 120.0f, 560.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_OPEN);

    set_point(&point, 180.0f, 480.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    line_to(sink, 220.0f, 480.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 280.0f, 400.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    line_to(sink, 280.0f, 560.0f);
    line_to(sink, 280.0f, 400.0f);
    line_to(sink, 280.0f, 560.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 20.0f, 880.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    line_to(sink, 40.0f, 720.0f);
    line_to(sink, 60.0f, 880.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_OPEN);

    set_point(&point, 100.0f, 720.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    line_to(sink, 120.0f, 880.0f);
    line_to(sink, 140.0f, 720.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_OPEN);

    set_point(&point, 180.0f, 880.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    line_to(sink, 200.0f, 720.0f);
    line_to(sink, 220.0f, 880.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 260.0f, 720.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    line_to(sink, 280.0f, 880.0f);
    line_to(sink, 300.0f, 720.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    ID2D1GeometrySink_SetFillMode(sink, D2D1_FILL_MODE_ALTERNATE);
    hr = ID2D1GeometrySink_Close(sink);
    ok(SUCCEEDED(hr), "Failed to close geometry sink, hr %#x.\n", hr);
    ID2D1GeometrySink_Release(sink);

    ID2D1RenderTarget_BeginDraw(rt);
    ID2D1RenderTarget_Clear(rt, &color);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)geometry, (ID2D1Brush *)brush, NULL);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to end draw, hr %#x.\n", hr);
    ID2D1PathGeometry_Release(geometry);

    match = compare_figure(&ctx,   0,   0, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160,   0, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 320,   0, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 480,   0, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");

    match = compare_figure(&ctx,   0, 160, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160, 160, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 320, 160, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 480, 160, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");

    match = compare_figure(&ctx,   0, 320, 160, 160, 0xff652e89, 0,
            "7zMCngECnQEEnAEEmwEGmgEGmQEImAEIlwEKlgEKlQEMlAEMkwEOkgEOkQEQkAEQjwESjgESjQEU"
            "jAEUiwEWigEWiQEYiAEYhwEahgEahQEchAEcgwEeggEegQEggAEgfyJ+In0kfCR7JnomeSh4KHcq"
            "dip1LHQscy5yLnEwcDBvMm4ybTRsNGs2ajZpOGg4ZzpmOmU8ZDxjPmI+YUBgQF9CXkJdRFxEW0Za"
            "RllIWEhXSlZKVUxUTFNOUk5RUKgy");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160, 320, 160, 160, 0xff652e89, 0,
            "qDJQUU5STlNMVExVSlZKV0hYSFlGWkZbRFxEXUJeQl9AYEBhPmI+YzxkPGU6ZjpnOGg4aTZqNms0"
            "bDRtMm4ybzBwMHEuci5zLHQsdSp2KncoeCh5JnomeyR8JH0ifiJ/IIABIIEBHoIBHoMBHIQBHIUB"
            "GoYBGocBGIgBGIkBFooBFosBFIwBFI0BEo4BEo8BEJABEJEBDpIBDpMBDJQBDJUBCpYBCpcBCJgB"
            "CJkBBpoBBpsBBJwBBJ0BAp4BAu8z");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 320, 320, 160, 160, 0xff652e89, 0,
            "7zMCngECnQEEnAEEmwEGmgEGmQEImAEIlwEKlgEKlQEMlAEMkwEOkgEOkQEQkAEQjwESjgESjQEU"
            "jAEUiwEWigEWiQEYiAEYhwEahgEahQEchAEcgwEeggEegQEggAEgfyJ+In0kfCR7JnomeSh4KHcq"
            "dip1LHQscy5yLnEwcDBvMm4ybTRsNGs2ajZpOGg4ZzpmOmU8ZDxjPmI+YUBgQF9CXkJdRFxEW0Za"
            "RllIWEhXSlZKVUxUTFNOUk5RUKgy");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 480, 320, 160, 160, 0xff652e89, 0,
            "qDJQUU5STlNMVExVSlZKV0hYSFlGWkZbRFxEXUJeQl9AYEBhPmI+YzxkPGU6ZjpnOGg4aTZqNms0"
            "bDRtMm4ybzBwMHEuci5zLHQsdSp2KncoeCh5JnomeyR8JH0ifiJ/IIABIIEBHoIBHoMBHIQBHIUB"
            "GoYBGocBGIgBGIkBFooBFosBFIwBFI0BEo4BEo8BEJABEJEBDpIBDpMBDJQBDJUBCpYBCpcBCJgB"
            "CJkBBpoBBpsBBJwBBJ0BAp4BAu8z");
    ok(match, "Figure does not match.\n");

    set_rect(&rect, 20.0f, 80.0f, 60.0f, 240.0f);
    hr = ID2D1Factory_CreateRectangleGeometry(factory, &rect, &rect_geometry[0]);
    ok(SUCCEEDED(hr), "Failed to create geometry, hr %#x.\n", hr);

    set_rect(&rect, -1.0f, -1.0f, 1.0f, 1.0f);
    hr = ID2D1Factory_CreateRectangleGeometry(factory, &rect, &rect_geometry[1]);
    ok(SUCCEEDED(hr), "Failed to create geometry, hr %#x.\n", hr);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 160.0f, 640.0f);
    scale_matrix(&matrix, 40.0f, 160.0f);
    rotate_matrix(&matrix, M_PI / -5.0f);
    hr = ID2D1Factory_CreateTransformedGeometry(factory,
            (ID2D1Geometry *)rect_geometry[1], &matrix, &transformed_geometry[0]);
    ok(SUCCEEDED(hr), "Failed to create geometry, hr %#x.\n", hr);

    set_matrix_identity(&matrix);
    scale_matrix(&matrix, 0.5f, 1.0f);
    translate_matrix(&matrix, -80.0f, 0.0f);
    hr = ID2D1Factory_CreateTransformedGeometry(factory,
            (ID2D1Geometry *)transformed_geometry[0], &matrix, &transformed_geometry[1]);
    ok(SUCCEEDED(hr), "Failed to create geometry, hr %#x.\n", hr);

    set_matrix_identity(&matrix);
    rotate_matrix(&matrix, M_PI / 2.0f);
    translate_matrix(&matrix, 80.0f, -320.0f);
    scale_matrix(&matrix, 2.0f, 0.25f);
    hr = ID2D1Factory_CreateTransformedGeometry(factory,
            (ID2D1Geometry *)transformed_geometry[1], &matrix, &transformed_geometry[2]);
    ok(SUCCEEDED(hr), "Failed to create geometry, hr %#x.\n", hr);

    ID2D1RenderTarget_BeginDraw(rt);
    ID2D1RenderTarget_Clear(rt, &color);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)rect_geometry[0], (ID2D1Brush *)brush, NULL);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)transformed_geometry[0], (ID2D1Brush *)brush, NULL);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)transformed_geometry[1], (ID2D1Brush *)brush, NULL);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)transformed_geometry[2], (ID2D1Brush *)brush, NULL);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to end draw, hr %#x.\n", hr);
    ID2D1TransformedGeometry_Release(transformed_geometry[2]);
    ID2D1TransformedGeometry_Release(transformed_geometry[1]);
    ID2D1TransformedGeometry_Release(transformed_geometry[0]);
    ID2D1RectangleGeometry_Release(rect_geometry[1]);
    ID2D1RectangleGeometry_Release(rect_geometry[0]);

    match = compare_figure(&ctx,   0,   0, 160, 160, 0xff652e89, 0,
            "qDJQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQ"
            "UFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQ"
            "UFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFCoMgAA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160,   0, 320, 160, 0xff652e89, 32,
            "sIMBA7cCDK8CFKYCHZ4CJJYCLY4CNYUCPv0BRvQBT+wBV+MBYNsBaNIBccoBecEBgQG6AYkBsQGS"
            "AakBmgGgAaMBmAGrAY8BtAGHAbwBfsUBfcYBfcYBfcUBfsUBfcYBfcYBfcYBfcYBfcUBfr0BhgG0"
            "AY8BrAGXAaMBoAGbAagBkgGwAYsBuAGCAcEBeskBcdIBadoBYOMBWOsBT/QBR/wBPoUCNowCLpUC"
            "Jp0CHaYCFa4CDLcCBK+DAQAA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx,   0, 160, 160, 320, 0xff652e89, 32,
            "+D0BngEDnQEDnAEEmwEGmgEGmQEHmAEJlwEJlgELlAEMkwENkwEOkQEPkAEQkAERjgESjQETjQEU"
            "iwEVigEXiQEXiAEYhwEahgEahQEbhAEdggEeggEegQEgfyF/In0jfCR8JXomeSd5KHcpdip2K3Qs"
            "cy5xL3EvcDFuMm4ybTRrNWs1ajdoOGg5ZjplO2U8Yz1iPmFAYEBfQV5DXUNcRVpGWkZZSFdJV0lW"
            "S1RMVExTTlFPUFFPUU5STVRMVEtVSldJV0hYR1pGWkVcQ11CXkJfQGA/YT9iPWM+Yj5jPWM+Yz1j"
            "PWM+Yz1jPmI+Yz1jPmI+Yz1jPmM9Yz1jPmM9Yz5iPmM9Yz5iPmM9Yz5jPWM9Yz5jPWM+Yj5jPWM+"
            "Yj5jPWI/YT9gQF9CXkJdRFtFW0VaR1hIV0lXSlVLVExUTVJOUVBQUE9RTlNNU0xUS1ZKVklXSFlG"
            "WkZaRVxDXUNeQV9AYEBhPmI9Yz1kO2U6ZjpnOGg3ajVrNWs0bTJuMm4xcC9xL3Eucyx0LHUqdil3"
            "KXgneSZ6JXwkfCN9In8hfyCBAR6CAR6CAR2EARuFARuFARqHARiIAReJAReKARWLARSNARONARKO"
            "ARGQARCQAQ+RAQ6TAQ2TAQyUAQuWAQqWAQmYAQeZAQaaAQabAQScAQOdAQOeAQH4PQAA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160, 160, 320, 320, 0xff652e89, 32,
            "sXkBvgIDvAIEugIHuAIJtgILswINsgIPrwISrQITrAIVqQIYpwIapQIbowIeoQIgngIjnAIkmwIm"
            "mAIplgIqlQIskgIvkAIxjQIzjAI1igI3hwI5hgI7hAI9gQJA/wFB/QFE+wFG+QFI9gFK9QFM8wFO"
            "8AFQ7wFS7AFV6gFX6AFY5gFb5AFd4gFf3wFh3gFj2wFm2QFn2AFp1QFs0wFu0QFvzwFyzQF0ygF3"
            "yAF4xwF6xAF9wgF+wAGBAb4BgwG8AYUBuQGHAbgBiQG2AYsBswGOAbEBjwGvAZIBrQGUAasBlQGp"
            "AZgBpwGaAaUBnAGiAZ4BoQGgAZ4BowGcAaUBmgGmAZgBqQGWAasBlAGsAZIBrwGQAbEBjQG0AYsB"
            "tQGKAbcBhwG6AYUBvAGDAb0BgQHAAX/CAXzEAXvGAXvGAXvGAXvFAXvGAXvGAXvGAXvFAXvGAXvG"
            "AXvFAXvGAXvGAXvGAXvFAXvGAXvGAXvFAXzFAXvGAXvGAXvFAXvGAXvGAXvGAXvFAXvGAXvGAXvF"
            "AXzFAXvGAXvGAXvFAXvGAXvGAXvGAXvEAXzCAX/AAYEBvgGCAbwBhQG6AYcBtwGKAbUBiwG0AY0B"
            "sQGQAa8BkgGtAZMBqwGWAakBmAGmAZoBpQGcAaMBngGgAaEBngGiAZ0BpAGaAacBmAGpAZUBqwGU"
            "Aa0BkgGvAY8BsQGOAbMBjAG1AYkBuAGHAbkBhQG8AYMBvgGBAcABfsIBfcQBe8YBeMgBd8oBdM0B"
            "cs8BcNABbtMBbNUBatcBZ9kBZtsBY94BYd8BYOEBXeQBW+YBWOgBV+oBVewBUu8BUPABT/IBTPUB"
            "SvYBSPkBRvsBRP0BQf8BQIECPoMCO4YCOYcCN4oCNYwCM40CMZACL5ICLZQCKpYCKZgCJpsCJJ0C"
            "Ip4CIKECHqMCHKQCGqcCGKkCFawCE60CEq8CD7ICDbMCDLUCCbgCB7oCBLwCA74CAbF5");
    ok(match, "Figure does not match.\n");

    hr = ID2D1Factory_CreatePathGeometry(factory, &geometry);
    ok(SUCCEEDED(hr), "Failed to create path geometry, hr %#x.\n", hr);
    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(SUCCEEDED(hr), "Failed to open geometry sink, hr %#x.\n", hr);

    set_point(&point, 20.0f, 80.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    quadratic_to(sink, 40.0f,  80.0f, 60.0f,  80.0f);
    quadratic_to(sink, 60.0f, 160.0f, 60.0f, 240.0f);
    quadratic_to(sink, 40.0f, 240.0f, 20.0f, 240.0f);
    quadratic_to(sink, 20.0f, 160.0f, 20.0f,  80.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 100.0f, 80.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    quadratic_to(sink, 105.0f,  80.0f, 140.0f,  80.0f);
    quadratic_to(sink, 140.0f, 100.0f, 140.0f, 240.0f);
    quadratic_to(sink, 135.0f, 240.0f, 100.0f, 240.0f);
    quadratic_to(sink, 100.0f, 220.0f, 100.0f,  80.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 180.0f, 80.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    quadratic_to(sink, 215.0f,  80.0f, 220.0f,  80.0f);
    quadratic_to(sink, 220.0f, 220.0f, 220.0f, 240.0f);
    quadratic_to(sink, 185.0f, 240.0f, 180.0f, 240.0f);
    quadratic_to(sink, 180.0f, 100.0f, 180.0f,  80.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 260.0f, 80.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    quadratic_to(sink, 280.0f,  80.0f, 300.0f,  80.0f);
    quadratic_to(sink, 300.0f, 160.0f, 300.0f, 240.0f);
    quadratic_to(sink, 280.0f, 240.0f, 260.0f, 240.0f);
    quadratic_to(sink, 260.0f, 160.0f, 260.0f,  80.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_OPEN);

    set_point(&point, 20.0f, 400.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    quadratic_to(sink, 40.0f, 420.0f, 60.0f, 400.0f);
    quadratic_to(sink, 55.0f, 480.0f, 60.0f, 560.0f);
    quadratic_to(sink, 40.0f, 540.0f, 20.0f, 560.0f);
    quadratic_to(sink, 25.0f, 480.0f, 20.0f, 400.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 100.0f, 400.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    quadratic_to(sink, 105.0f, 420.0f, 140.0f, 400.0f);
    quadratic_to(sink, 135.0f, 420.0f, 140.0f, 560.0f);
    quadratic_to(sink, 135.0f, 540.0f, 100.0f, 560.0f);
    quadratic_to(sink, 105.0f, 540.0f, 100.0f, 400.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 180.0f, 400.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    quadratic_to(sink, 215.0f, 420.0f, 220.0f, 400.0f);
    quadratic_to(sink, 215.0f, 540.0f, 220.0f, 560.0f);
    quadratic_to(sink, 185.0f, 540.0f, 180.0f, 560.0f);
    quadratic_to(sink, 185.0f, 420.0f, 180.0f, 400.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 260.0f, 400.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    quadratic_to(sink, 280.0f, 420.0f, 300.0f, 400.0f);
    quadratic_to(sink, 295.0f, 480.0f, 300.0f, 560.0f);
    quadratic_to(sink, 280.0f, 540.0f, 260.0f, 560.0f);
    quadratic_to(sink, 265.0f, 480.0f, 260.0f, 400.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_OPEN);

    set_point(&point, 20.0f, 720.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    quadratic_to(sink, 40.0f, 700.0f, 60.0f, 720.0f);
    quadratic_to(sink, 65.0f, 800.0f, 60.0f, 880.0f);
    quadratic_to(sink, 40.0f, 900.0f, 20.0f, 880.0f);
    quadratic_to(sink, 15.0f, 800.0f, 20.0f, 720.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 100.0f, 720.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    quadratic_to(sink, 105.0f, 700.0f, 140.0f, 720.0f);
    quadratic_to(sink, 145.0f, 740.0f, 140.0f, 880.0f);
    quadratic_to(sink, 135.0f, 900.0f, 100.0f, 880.0f);
    quadratic_to(sink,  95.0f, 860.0f, 100.0f, 720.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 180.0f, 720.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    quadratic_to(sink, 215.0f, 700.0f, 220.0f, 720.0f);
    quadratic_to(sink, 225.0f, 860.0f, 220.0f, 880.0f);
    quadratic_to(sink, 185.0f, 900.0f, 180.0f, 880.0f);
    quadratic_to(sink, 175.0f, 740.0f, 180.0f, 720.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 260.0f, 720.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    quadratic_to(sink, 280.0f, 700.0f, 300.0f, 720.0f);
    quadratic_to(sink, 305.0f, 800.0f, 300.0f, 880.0f);
    quadratic_to(sink, 280.0f, 900.0f, 260.0f, 880.0f);
    quadratic_to(sink, 255.0f, 800.0f, 260.0f, 720.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_OPEN);

    hr = ID2D1GeometrySink_Close(sink);
    ok(SUCCEEDED(hr), "Failed to close geometry sink, hr %#x.\n", hr);
    ID2D1GeometrySink_Release(sink);

    ID2D1RenderTarget_BeginDraw(rt);
    ID2D1RenderTarget_Clear(rt, &color);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)geometry, (ID2D1Brush *)brush, NULL);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to end draw, hr %#x.\n", hr);
    ID2D1PathGeometry_Release(geometry);

    match = compare_figure(&ctx,   0,   0, 160, 160, 0xff652e89, 0,
            "qDJQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQ"
            "UFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQ"
            "UFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFCoMgAA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160,   0, 160, 160, 0xff652e89, 0,
            "qDJQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQ"
            "UFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQ"
            "UFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFCoMgAA");
    match = compare_figure(&ctx, 320,   0, 160, 160, 0xff652e89, 0,
            "qDJQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQ"
            "UFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQ"
            "UFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFCoMgAA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 480,   0, 160, 160, 0xff652e89, 0,
            "qDJQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQ"
            "UFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQ"
            "UFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFBQUFCoMgAA");
    ok(match, "Figure does not match.\n");

    match = compare_figure(&ctx,   0, 160, 160, 160, 0xff652e89, 16,
            "qDICTAJQB0IHUQs4C1IRLBFSGxgbUk5STlNMVExUTFRMVExVSlZKVkpWSlZKVkpXSFhIWEhYSFhI"
            "WEhYSFhIWEhYSFlGWkZaRlpGWkZaRlpGWkZaRlpGWkZaRlpGWkZaRlpGWkZaRlpGWkZaRlpGWkZa"
            "RllIWEhYSFhIWEhYSFhIWEhYSFhIV0pWSlZKVkpWSlZKVUxUTFRMVExUTFNOUk5SGxgbUhEsEVIL"
            "OAtRB0IHUAJMAqgy");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160, 160, 160, 160, 0xff652e89, 16,
            "qDIBSwRQAkMKUQQ5EVIIKxtTDRkmVExUTFRMVEtVS1VLVkpWSlZKVklXSVdJV0lXSVhIWEhYSFhI"
            "WEhYSFhIWEhYSFhIWUdZR1lHWUdZR1lHWUdZR1lHWUdZSFhIWUdZR1lHWUdZR1lHWUdZR1lHWUdZ"
            "SFhIWEhYSFhIWEhYSFhIWEhYSFhJV0lXSVdJV0lWSlZKVkpWS1VLVUtUTFRMVExUJhkNUxsrCFIR"
            "OQRRCkMCUARLAagy");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 320, 160, 160, 160, 0xff652e89, 16,
            "qDIESwFRCkMCUhE5BFIbKwhTJhkNVExUTFRMVUtVS1VLVUpWSlZKV0lXSVdJV0lXSVdIWEhYSFhI"
            "WEhYSFhIWEhYSFhIWEdZR1lHWUdZR1lHWUdZR1lHWUdYSFhIWEdZR1lHWUdZR1lHWUdZR1lHWUdY"
            "SFhIWEhYSFhIWEhYSFhIWEhYSFdJV0lXSVdJV0lXSlZKVkpVS1VLVUtVTFRMVExUDRkmUwgrG1IE"
            "ORFSAkMKUQFLBKgy");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 480, 160, 160, 160, 0xff652e89, 16,
            "qDICTAJQB0IHUQs4C1IRLBFSGxgbUk5STlNMVExUTFRMVExVSlZKVkpWSlZKVkpXSFhIWEhYSFhI"
            "WEhYSFhIWEhYSFlGWkZaRlpGWkZaRlpGWkZaRlpGWkZaRlpGWkZaRlpGWkZaRlpGWkZaRlpGWkZa"
            "RllIWEhYSFhIWEhYSFhIWEhYSFhIV0pWSlZKVkpWSlZKVUxUTFRMVExUTFNOUk5SGxgbUhEsEVIL"
            "OAtRB0IHUAJMAqgy");
    ok(match, "Figure does not match.\n");

    match = compare_figure(&ctx,   0, 320, 160, 160, 0xff652e89, 16,
            "pCwYfixuOGNCWUxSUFBQT1JOUk5STlJOUk1UTFRMVExUTFRLVkpWSlZKVkpWSlZJWEhYSFhIWEhY"
            "SFhIWEhYSFhIWEdaRlpGWkZaRlpGWkZaRlpGWkZaRlpGWkZaRlpGWkZaRlpGWkZaRlpGWkZaRlpG"
            "WkdYSFhIWEhYSFhIWEhYSFhIWEhYSVZKVkpWSlZKVkpWS1RMVExUTFRMVE1STlJOUk5STlJPUFBQ"
            "UkxZQmM4bix+GKQs");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160, 320, 160, 160, 0xff652e89, 16,
            "liwZgQErcTllQ1xLVFBQUU9STlJNVExUTFRMVExVS1VLVUpWSlZKVkpXSVdJV0lXSVdIWEhYSFhI"
            "WEhYSFhIWEhYSFhIWEdZR1lHWUdZR1lHWUdZR1lHWUdZR1hIWEdZR1lHWUdZR1lHWUdZR1lHWUdZ"
            "R1hIWEhYSFhIWEhYSFhIWEhYSFhIV0lXSVdJV0lXSlZKVkpWSlVLVUtVTFRMVExUTFRNUk5ST1FQ"
            "UFRLXENlOXErgQEZliwA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 320, 320, 160, 160, 0xff652e89, 16,
            "sSwZeytrOV9DVktRUE9RTlJOUk1UTFRMVExUS1VLVUtVS1ZKVkpWSVdJV0lXSVdJV0lYSFhIWEhY"
            "SFhIWEhYSFhIWEhYSFlHWUdZR1lHWUdZR1lHWUdZR1lIWEhYSFlHWUdZR1lHWUdZR1lHWUdZR1lI"
            "WEhYSFhIWEhYSFhIWEhYSFhIWElXSVdJV0lXSVdJVkpWSlZLVUtVS1VLVExUTFRMVE1STlJOUU9Q"
            "UUtWQ185ayt7GbEs");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 480, 320, 160, 160, 0xff652e89, 16,
            "pCwYfixuOGNCWUxSUFBQT1JOUk5STlJOUk1UTFRMVExUTFRLVkpWSlZKVkpWSlZJWEhYSFhIWEhY"
            "SFhIWEhYSFhIWEdaRlpGWkZaRlpGWkZaRlpGWkZaRlpGWkZaRlpGWkZaRlpGWkZaRlpGWkZaRlpG"
            "WkdYSFhIWEhYSFhIWEhYSFhIWEhYSVZKVkpWSlZKVkpWS1RMVExUTFRMVE1STlJOUk5STlJPUFBQ"
            "UkxZQmM4bix+GKQs");
    ok(match, "Figure does not match.\n");

    hr = ID2D1Factory_CreatePathGeometry(factory, &geometry);
    ok(SUCCEEDED(hr), "Failed to create path geometry, hr %#x.\n", hr);
    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(SUCCEEDED(hr), "Failed to open geometry sink, hr %#x.\n", hr);

    set_point(&point, -0.402914f, 0.915514f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    quadratic_to(sink, -0.310379f,  0.882571f, -0.116057f,  0.824000f);
    quadratic_to(sink,  0.008350f,  0.693614f, -0.052343f,  0.448886f);
    quadratic_to(sink, -0.154236f,  0.246072f, -0.279229f,  0.025343f);
    quadratic_to(sink, -0.370064f, -0.588586f, -0.383029f, -0.924114f);
    quadratic_to(sink, -0.295479f, -0.958764f, -0.017086f, -0.988400f);
    quadratic_to(sink,  0.208836f, -0.954157f,  0.272200f, -0.924114f);
    quadratic_to(sink,  0.295614f, -0.569071f,  0.230143f,  0.022886f);
    quadratic_to(sink,  0.101664f,  0.220643f,  0.012057f,  0.451571f);
    quadratic_to(sink, -0.028764f,  0.709014f,  0.104029f,  0.833943f);
    quadratic_to(sink,  0.319414f,  0.913057f,  0.403229f,  0.942628f);
    quadratic_to(sink,  0.317721f,  1.023450f, -0.017086f,  1.021771f);
    quadratic_to(sink, -0.310843f,  1.007472f, -0.402914f,  0.915514f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    hr = ID2D1GeometrySink_Close(sink);
    ok(SUCCEEDED(hr), "Failed to close geometry sink, hr %#x.\n", hr);
    ID2D1GeometrySink_Release(sink);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 40.0f, 160.0f);
    scale_matrix(&matrix, 20.0f, 80.0f);
    hr = ID2D1Factory_CreateTransformedGeometry(factory,
            (ID2D1Geometry *)geometry, &matrix, &transformed_geometry[0]);
    ok(SUCCEEDED(hr), "Failed to create geometry, hr %#x.\n", hr);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 160.0f, 640.0f);
    scale_matrix(&matrix, 40.0f, 160.0f);
    rotate_matrix(&matrix, M_PI / -5.0f);
    hr = ID2D1Factory_CreateTransformedGeometry(factory,
            (ID2D1Geometry *)geometry, &matrix, &transformed_geometry[1]);
    ok(SUCCEEDED(hr), "Failed to create geometry, hr %#x.\n", hr);
    ID2D1PathGeometry_Release(geometry);

    set_matrix_identity(&matrix);
    scale_matrix(&matrix, 0.5f, 1.0f);
    translate_matrix(&matrix, -80.0f, 0.0f);
    hr = ID2D1Factory_CreateTransformedGeometry(factory,
            (ID2D1Geometry *)transformed_geometry[1], &matrix, &transformed_geometry[2]);
    ok(SUCCEEDED(hr), "Failed to create geometry, hr %#x.\n", hr);

    set_matrix_identity(&matrix);
    rotate_matrix(&matrix, M_PI / 2.0f);
    translate_matrix(&matrix, 80.0f, -320.0f);
    scale_matrix(&matrix, 2.0f, 0.25f);
    hr = ID2D1Factory_CreateTransformedGeometry(factory,
            (ID2D1Geometry *)transformed_geometry[2], &matrix, &transformed_geometry[3]);
    ok(SUCCEEDED(hr), "Failed to create geometry, hr %#x.\n", hr);

    ID2D1RenderTarget_BeginDraw(rt);
    ID2D1RenderTarget_Clear(rt, &color);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)transformed_geometry[0], (ID2D1Brush *)brush, NULL);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)transformed_geometry[1], (ID2D1Brush *)brush, NULL);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)transformed_geometry[2], (ID2D1Brush *)brush, NULL);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)transformed_geometry[3], (ID2D1Brush *)brush, NULL);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to end draw, hr %#x.\n", hr);
    ID2D1TransformedGeometry_Release(transformed_geometry[3]);
    ID2D1TransformedGeometry_Release(transformed_geometry[2]);
    ID2D1TransformedGeometry_Release(transformed_geometry[1]);
    ID2D1TransformedGeometry_Release(transformed_geometry[0]);

    match = compare_figure(&ctx,   0,   0, 160, 160, 0xff652e89, 32,
            "6DMNjgEWiAEahgEahgEahgEahgEahgEahgEahgEahgEahgEahgEahwEZhwEZhwEZhwEZhwEZhwEZ"
            "hwEZhwEZhwEZiAEYiAEYiAEYiAEYiAEXiQEXiQEXiQEXigEWigEWigEWigEWigEWigEWigEWiwEU"
            "jAEUjAEUjAEUjQESjgESjwEQkAEQkQEOkgENlAEMlQEKlgEKlwEImAEImQEHmQEGmwEFmwEEnQED"
            "nQECngECngECnwEBnwEBnwEBnwEBnwEBnwECnQEDnQEDnQEEmwEFmgEHmQEHlwELkQERjAEXhgEd"
            "hAEfgwEchgEXjwEMqTEA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160,   0, 320, 160, 0xff652e89, 32,
            "h58BBrYCDq0CF6QCIJwCKJMCMIwCNoUCPf8BQ/kBSPQBTu4BTe8BTPEBSfUBRvgBQf0BPYECOYUC"
            "NIoCMI4CK+UBAS0W/AEHIwiPAgsaBZcCEAwIngIepAIaqAIWrAITsAIRsgIPtQIMtwILugIHwAIB"
            "ypwB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx,   0, 160, 160, 320, 0xff652e89, 32,
            "wW4DnAEEmwEFmgEHmAEIlwEKlQELlAEMkwEOkQEPkAEQkAERjgESjgETjAEUjAEUiwEWigEWiQEX"
            "iQEYhwEZhwEZhgEbhQEbhAEchAEdggEeggEeggEfgAEggAEggAEhgAEggAEggQEggAEggAEggQEg"
            "gAEggQEfgQEfggEfgQEfgQEfggEfgQEfggEeggEfggEeggEegwEdgwEeggEegwEdgwEegwEdgwEd"
            "hAEchAEdhAEchAEchAEdhAEchAEchQEbhQEbhgEahgEahwEZhwEZiAEYiAEYiQEYiAEYiQEXiQEX"
            "igEWigEWiwEViwEViwEVjAEUjAEUjQETjQETjgESjgETjgESjwERkAEQkQEPkwENlAEMlQELlgEK"
            "lwEKlwEJmAEImQEHmgEGmwEFnAEEnQEEnQEDnQEDngECngEDngECngECnwECngECnwECngECngED"
            "ngECEgGLAQMQAosBAw4EjAEDCwaMAQQJBo0BBQYIjQEHAgqNARKOARKPARCQARCQARCQAQ+RAQ6S"
            "AQ6SAQ2TAQ2SAQ2TAQ2TAQyTAQyUAQyUAQuUAQuVAQuUAQuVAQqWAQmWAQqWAQmXAQiXAQiYAQeY"
            "AQeZAQWbAQSDZwAA");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160, 160, 320, 320, 0xff652e89, 32,
            "g90BBLkCCLYCC7ICDrACEa0CFKoCF6cCGqQCHKMCHqECIJ8CIpwCJJsCJpkCKJcCKZYCK5QCLZIC"
            "L5ACMI8CMo0CNIsCNYoCN4gCOYcCOYYCO4QCPYICPoECQIACQYACQIECQIACQIECQIECQIECP4IC"
            "P4ICP4ECP4ICP4ICPoMCPoMCPoMCPYQCPYMCPYQCPYQCPYQCPIUCPIUCPIUCO4YCO4YCOoYCO4YC"
            "OocCOocCOocCOYgCOYgCOIkCOIkCN4oCNosCNYwCNI0CM44CMo4CM44CMo8CMZACMJECL5ICLpMC"
            "LZQCLJUCK5YCK5YCKpcCKZgCKJkCJ5oCJpsCJpsCJZwCJJ4CIqACIKICH6MCHaUCG6cCGakCF6wC"
            "Fa0CE68CEbECD7MCDrQCDLYCCrgCCbkCB7sCBrsCBbwCBbwCBL0CBL0CBL0CBL0CA70CBL0CBL0C"
            "BLwCBSUBlgIFIQSXAgYbCJcCBxcKmQIIEQ6ZAgoMEJoCDQUTnAIknAIjnQIingIhnwIgoAIfoQIe"
            "ogIdowIcpAIbpQIapQIZpgIZpgIZpwIYpwIXqAIXqAIXqQIVqgIVqgIUqwITrQISrQIRrgIQsAIO"
            "sQIMswILtQIIhs4B");
    ok(match, "Figure does not match.\n");

    hr = ID2D1Factory_CreatePathGeometry(factory, &geometry);
    ok(SUCCEEDED(hr), "Failed to create path geometry, hr %#x.\n", hr);
    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(SUCCEEDED(hr), "Failed to open geometry sink, hr %#x.\n", hr);

    set_point(&point, -0.402914f, 0.915514f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_HOLLOW);
    quadratic_to(sink, -0.310379f,  0.882571f, -0.116057f,  0.824000f);
    quadratic_to(sink,  0.008350f,  0.693614f, -0.052343f,  0.448886f);
    quadratic_to(sink, -0.154236f,  0.246072f, -0.279229f,  0.025343f);
    quadratic_to(sink, -0.370064f, -0.588586f, -0.383029f, -0.924114f);
    quadratic_to(sink, -0.295479f, -0.958764f, -0.017086f, -0.988400f);
    quadratic_to(sink,  0.208836f, -0.954157f,  0.272200f, -0.924114f);
    quadratic_to(sink,  0.295614f, -0.569071f,  0.230143f,  0.022886f);
    quadratic_to(sink,  0.101664f,  0.220643f,  0.012057f,  0.451571f);
    quadratic_to(sink, -0.028764f,  0.709014f,  0.104029f,  0.833943f);
    quadratic_to(sink,  0.319414f,  0.913057f,  0.403229f,  0.942628f);
    quadratic_to(sink,  0.317721f,  1.023450f, -0.017086f,  1.021771f);
    quadratic_to(sink, -0.310843f,  1.007472f, -0.402914f,  0.915514f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    hr = ID2D1GeometrySink_Close(sink);
    ok(SUCCEEDED(hr), "Failed to close geometry sink, hr %#x.\n", hr);
    ID2D1GeometrySink_Release(sink);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 40.0f, 160.0f);
    scale_matrix(&matrix, 20.0f, 80.0f);
    hr = ID2D1Factory_CreateTransformedGeometry(factory,
            (ID2D1Geometry *)geometry, &matrix, &transformed_geometry[0]);
    ok(SUCCEEDED(hr), "Failed to create geometry, hr %#x.\n", hr);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 160.0f, 640.0f);
    scale_matrix(&matrix, 40.0f, 160.0f);
    rotate_matrix(&matrix, M_PI / -5.0f);
    hr = ID2D1Factory_CreateTransformedGeometry(factory,
            (ID2D1Geometry *)geometry, &matrix, &transformed_geometry[1]);
    ok(SUCCEEDED(hr), "Failed to create geometry, hr %#x.\n", hr);
    ID2D1PathGeometry_Release(geometry);

    set_matrix_identity(&matrix);
    scale_matrix(&matrix, 0.5f, 1.0f);
    translate_matrix(&matrix, -80.0f, 0.0f);
    hr = ID2D1Factory_CreateTransformedGeometry(factory,
            (ID2D1Geometry *)transformed_geometry[1], &matrix, &transformed_geometry[2]);
    ok(SUCCEEDED(hr), "Failed to create geometry, hr %#x.\n", hr);

    set_matrix_identity(&matrix);
    rotate_matrix(&matrix, M_PI / 2.0f);
    translate_matrix(&matrix, 80.0f, -320.0f);
    scale_matrix(&matrix, 2.0f, 0.25f);
    hr = ID2D1Factory_CreateTransformedGeometry(factory,
            (ID2D1Geometry *)transformed_geometry[2], &matrix, &transformed_geometry[3]);
    ok(SUCCEEDED(hr), "Failed to create geometry, hr %#x.\n", hr);

    ID2D1RenderTarget_BeginDraw(rt);
    ID2D1RenderTarget_Clear(rt, &color);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)transformed_geometry[0], (ID2D1Brush *)brush, NULL);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)transformed_geometry[1], (ID2D1Brush *)brush, NULL);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)transformed_geometry[2], (ID2D1Brush *)brush, NULL);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)transformed_geometry[3], (ID2D1Brush *)brush, NULL);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to end draw, hr %#x.\n", hr);
    ID2D1TransformedGeometry_Release(transformed_geometry[3]);
    ID2D1TransformedGeometry_Release(transformed_geometry[2]);
    ID2D1TransformedGeometry_Release(transformed_geometry[1]);
    ID2D1TransformedGeometry_Release(transformed_geometry[0]);

    match = compare_figure(&ctx,   0,   0, 160, 160, 0xff652e89, 0, "gMgB");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160,   0, 320, 160, 0xff652e89, 0, "gJAD");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx,   0, 160, 160, 320, 0xff652e89, 0, "gJAD");
    ok(match, "Figure does not match.\n");
    match = compare_figure(&ctx, 160, 160, 320, 320, 0xff652e89, 0, "gKAG");
    ok(match, "Figure does not match.\n");

    ID2D1SolidColorBrush_Release(brush);
    ID2D1Factory_Release(factory);
    release_test_context(&ctx);
}

static void test_gdi_interop(BOOL d3d11)
{
    ID2D1GdiInteropRenderTarget *interop;
    D2D1_RENDER_TARGET_PROPERTIES desc;
    IWICImagingFactory *wic_factory;
    struct d2d1_test_context ctx;
    IWICBitmapLock *wic_lock;
    IWICBitmap *wic_bitmap;
    ID2D1RenderTarget *rt;
    ID2D1Factory *factory;
    D2D1_COLOR_F color;
    HRESULT hr;
    BOOL match;
    RECT rect;
    HDC dc;

    if (!init_test_context(&ctx, d3d11))
        return;

    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &IID_ID2D1Factory, NULL, (void **)&factory);
    ok(SUCCEEDED(hr), "Failed to create factory, hr %#x.\n", hr);

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
            &IID_IWICImagingFactory, (void **)&wic_factory);
    ok(SUCCEEDED(hr), "Failed to create WIC imaging factory, hr %#x.\n", hr);
    hr = IWICImagingFactory_CreateBitmap(wic_factory, 16, 16,
            &GUID_WICPixelFormat32bppPBGRA, WICBitmapCacheOnDemand, &wic_bitmap);
    ok(SUCCEEDED(hr), "Failed to create bitmap, hr %#x.\n", hr);
    IWICImagingFactory_Release(wic_factory);

    /* WIC target, default usage */
    desc.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
    desc.pixelFormat.format = DXGI_FORMAT_UNKNOWN;
    desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    desc.dpiX = 0.0f;
    desc.dpiY = 0.0f;
    desc.usage = D2D1_RENDER_TARGET_USAGE_NONE;
    desc.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;

    hr = ID2D1Factory_CreateWicBitmapRenderTarget(factory, wic_bitmap, &desc, &rt);
    ok(SUCCEEDED(hr), "Failed to create render target, hr %#x.\n", hr);

    hr = ID2D1RenderTarget_QueryInterface(rt, &IID_ID2D1GdiInteropRenderTarget, (void **)&interop);
    ok(SUCCEEDED(hr), "Failed to get gdi interop interface, hr %#x.\n", hr);

    ID2D1RenderTarget_BeginDraw(rt);
    dc = (void *)0xdeadbeef;
    hr = ID2D1GdiInteropRenderTarget_GetDC(interop, D2D1_DC_INITIALIZE_MODE_COPY, &dc);
    ok(FAILED(hr), "GetDC() was expected to fail, hr %#x.\n", hr);
todo_wine
    ok(dc == NULL, "Expected NULL dc, got %p.\n", dc);
    ID2D1GdiInteropRenderTarget_Release(interop);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to end draw, hr %#x.\n", hr);

    ID2D1RenderTarget_Release(rt);

    /* WIC target, gdi compatible */
    desc.usage = D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE;

    hr = ID2D1Factory_CreateWicBitmapRenderTarget(factory, wic_bitmap, &desc, &rt);
    ok(SUCCEEDED(hr), "Failed to create render target, hr %#x.\n", hr);

    hr = ID2D1RenderTarget_QueryInterface(rt, &IID_ID2D1GdiInteropRenderTarget, (void **)&interop);
    ok(SUCCEEDED(hr), "Failed to get gdi interop interface, hr %#x.\n", hr);

    ID2D1RenderTarget_BeginDraw(rt);
    dc = NULL;
    hr = ID2D1GdiInteropRenderTarget_GetDC(interop, D2D1_DC_INITIALIZE_MODE_COPY, &dc);
    ok(SUCCEEDED(hr), "GetDC() was expected to succeed, hr %#x.\n", hr);
    ok(dc != NULL, "Expected NULL dc, got %p.\n", dc);
    ID2D1GdiInteropRenderTarget_ReleaseDC(interop, NULL);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(SUCCEEDED(hr), "EndDraw() failed, hr %#x.\n", hr);

    ID2D1RenderTarget_BeginDraw(rt);
    set_color(&color, 1.0f, 0.0f, 0.0f, 1.0f);
    ID2D1RenderTarget_Clear(rt, &color);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(SUCCEEDED(hr), "EndDraw() failed, hr %#x.\n", hr);

    match = compare_wic_bitmap(wic_bitmap, "54034063dbc1c1bb61cb60ec57e4498678dc2b13");
    ok(match, "Bitmap does not match.\n");

    /* Do solid fill using GDI */
    ID2D1RenderTarget_BeginDraw(rt);

    hr = ID2D1GdiInteropRenderTarget_GetDC(interop, D2D1_DC_INITIALIZE_MODE_COPY, &dc);
    ok(SUCCEEDED(hr), "GetDC() was expected to succeed, hr %#x.\n", hr);

    SetRect(&rect, 0, 0, 16, 16);
    FillRect(dc, &rect, GetStockObject(BLACK_BRUSH));
    ID2D1GdiInteropRenderTarget_ReleaseDC(interop, NULL);

    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(SUCCEEDED(hr), "EndDraw() failed, hr %#x.\n", hr);

    match = compare_wic_bitmap(wic_bitmap, "60cacbf3d72e1e7834203da608037b1bf83b40e8");
    ok(match, "Bitmap does not match.\n");

    /* Bitmap is locked at BeginDraw(). */
    hr = IWICBitmap_Lock(wic_bitmap, NULL, WICBitmapLockRead, &wic_lock);
    ok(SUCCEEDED(hr), "Expected bitmap to be unlocked, hr %#x.\n", hr);
    IWICBitmapLock_Release(wic_lock);

    ID2D1RenderTarget_BeginDraw(rt);
    hr = IWICBitmap_Lock(wic_bitmap, NULL, WICBitmapLockRead, &wic_lock);
todo_wine
    ok(hr == WINCODEC_ERR_ALREADYLOCKED, "Expected bitmap to be locked, hr %#x.\n", hr);
    if (SUCCEEDED(hr))
        IWICBitmapLock_Release(wic_lock);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(SUCCEEDED(hr), "EndDraw() failed, hr %#x.\n", hr);

    /* Lock before BeginDraw(). */
    hr = IWICBitmap_Lock(wic_bitmap, NULL, WICBitmapLockRead, &wic_lock);
    ok(SUCCEEDED(hr), "Expected bitmap to be unlocked, hr %#x.\n", hr);
    ID2D1RenderTarget_BeginDraw(rt);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
todo_wine
    ok(hr == WINCODEC_ERR_ALREADYLOCKED, "Unexpected hr %#x.\n", hr);
    IWICBitmapLock_Release(wic_lock);

    ID2D1GdiInteropRenderTarget_Release(interop);
    ID2D1RenderTarget_Release(rt);

    IWICBitmap_Release(wic_bitmap);
    ID2D1Factory_Release(factory);
    release_test_context(&ctx);
}

static void test_layer(BOOL d3d11)
{
    ID2D1Factory *factory, *layer_factory;
    struct d2d1_test_context ctx;
    ID2D1RenderTarget *rt;
    ID2D1Layer *layer;
    D2D1_SIZE_F size;
    HRESULT hr;

    if (!init_test_context(&ctx, d3d11))
        return;

    rt = ctx.rt;
    ID2D1RenderTarget_GetFactory(rt, &factory);

    ID2D1RenderTarget_SetDpi(rt, 192.0f, 48.0f);
    ID2D1RenderTarget_SetAntialiasMode(rt, D2D1_ANTIALIAS_MODE_ALIASED);

    hr = ID2D1RenderTarget_CreateLayer(rt, NULL, &layer);
    ok(SUCCEEDED(hr), "Failed to create layer, hr %#x.\n", hr);
    ID2D1Layer_GetFactory(layer, &layer_factory);
    ok(layer_factory == factory, "Got unexpected layer factory %p, expected %p.\n", layer_factory, factory);
    ID2D1Factory_Release(layer_factory);
    size = ID2D1Layer_GetSize(layer);
    ok(size.width == 0.0f, "Got unexpected width %.8e.\n", size.width);
    ok(size.height == 0.0f, "Got unexpected height %.8e.\n", size.height);
    ID2D1Layer_Release(layer);

    set_size_f(&size, 800.0f, 600.0f);
    hr = ID2D1RenderTarget_CreateLayer(rt, &size, &layer);
    ok(SUCCEEDED(hr), "Failed to create layer, hr %#x.\n", hr);
    size = ID2D1Layer_GetSize(layer);
    ok(size.width == 800.0f, "Got unexpected width %.8e.\n", size.width);
    ok(size.height == 600.0f, "Got unexpected height %.8e.\n", size.height);
    ID2D1Layer_Release(layer);

    ID2D1Factory_Release(factory);
    release_test_context(&ctx);
}

static void test_bezier_intersect(BOOL d3d11)
{
    D2D1_POINT_2F point = {0.0f, 0.0f};
    struct d2d1_test_context ctx;
    ID2D1SolidColorBrush *brush;
    ID2D1PathGeometry *geometry;
    ID2D1GeometrySink *sink;
    ID2D1RenderTarget *rt;
    ID2D1Factory *factory;
    D2D1_COLOR_F color;
    HRESULT hr;
    BOOL match;

    if (!init_test_context(&ctx, d3d11))
        return;

    rt = ctx.rt;
    ID2D1RenderTarget_GetFactory(rt, &factory);

    ID2D1RenderTarget_SetDpi(rt, 192.0f, 48.0f);
    ID2D1RenderTarget_SetAntialiasMode(rt, D2D1_ANTIALIAS_MODE_ALIASED);
    set_color(&color, 0.890f, 0.851f, 0.600f, 1.0f);
    hr = ID2D1RenderTarget_CreateSolidColorBrush(rt, &color, NULL, &brush);
    ok(SUCCEEDED(hr), "Failed to create brush, hr %#x.\n", hr);

    hr = ID2D1Factory_CreatePathGeometry(factory, &geometry);
    ok(SUCCEEDED(hr), "Failed to create path geometry, hr %#x.\n", hr);
    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(SUCCEEDED(hr), "Failed to open geometry sink, hr %#x.\n", hr);

    set_point(&point, 160.0f, 720.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    cubic_to(sink, 119.0f, 720.0f,  83.0f, 600.0f,  80.0f, 474.0f);
    cubic_to(sink,  78.0f, 349.0f, 108.0f, 245.0f, 135.0f, 240.0f);
    cubic_to(sink, 163.0f, 235.0f, 180.0f, 318.0f, 176.0f, 370.0f);
    cubic_to(sink, 171.0f, 422.0f, 149.0f, 422.0f, 144.0f, 370.0f);
    cubic_to(sink, 140.0f, 318.0f, 157.0f, 235.0f, 185.0f, 240.0f);
    cubic_to(sink, 212.0f, 245.0f, 242.0f, 349.0f, 240.0f, 474.0f);
    cubic_to(sink, 238.0f, 600.0f, 201.0f, 720.0f, 160.0f, 720.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    set_point(&point, 160.0f, 240.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    line_to(sink, 240.0f, 240.0f);
    line_to(sink, 240.0f, 720.0f);
    line_to(sink, 160.0f, 720.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    hr = ID2D1GeometrySink_Close(sink);
    ok(SUCCEEDED(hr), "Failed to close geometry sink, hr %#x.\n", hr);
    ID2D1GeometrySink_Release(sink);

    ID2D1RenderTarget_BeginDraw(rt);
    set_color(&color, 0.396f, 0.180f, 0.537f, 1.0f);
    ID2D1RenderTarget_Clear(rt, &color);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)geometry, (ID2D1Brush *)brush, NULL);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to end draw, hr %#x.\n", hr);
    ID2D1PathGeometry_Release(geometry);

    match = compare_figure(&ctx, 160, 120, 320, 240, 0xff652e89, 2048,
            "aRQjIxRpYiIcHCJiXSwXFyxdWTQTEzRZVTsQEDtVUkIMDEJST0cKCkdPTUsICEtNSlEFBVFKSFUD"
            "A1VIRlkBAVlGRFsBAVtEQlwCAlxCQFwEBFxAPl0FBV0+PF0HB108Ol4ICF46OV0KCl05N14LC143"
            "Nl4MDF42NF8NDV80M14PD14zMV8QEF8xMF8REV8wL18SEl8vLWATE2AtLGAUFGAsK2EUFGErKWIV"
            "FWIpKGIWFmIoJ2IXF2InJmIYGGImJWMYGGMlJGMZGWMkI2MaGmMjImQaGmQiIWQbG2QhIGQcHGQg"
            "H2UcHGUfHmUdHWUeHWYdHWYdHGcdHWccG2ceHmcbGmgeHmgaGWgfH2gZGWgfH2gZGGkfH2kYF2kg"
            "IGkXFmogIGoWFmogIGoWFWsgIGsVFGshIWsUE2whIWwTE2whIWwTEm0hIW0SEW4hIW4REW4hIW4R"
            "EG8hIW8QD3AhIXAPD3AhIXAPDnEhIXEODnEhIXEODXIhIXINDHQgIHQMDHQgIHQMC3UgIHULC3Yf"
            "H3YLCncfH3cKCngeHngKCXkeHnkJCXodHXoJCXscHHsJCHwcHHwICH0bG30IB38aGn8HB4ABGRmA"
            "AQcHgQEYGIEBBwaEARYWhAEGBoUBFRWFAQYFiAETE4gBBQWKARERigEFBYwBDw+MAQUEkAEMDJAB"
            "BASTAQkJkwEEBJwBnAEEA50BnQEDA50BnQEDA50BnQEDA50BnQEDAp4BngECAp4BngECAp4BngEC"
            "Ap4BngECAp4BngECAZ8BnwEBAZ8BnwEBAZ8BnwEBAZ8BnwEBAZ8BnwEBAZ8BnwEBAZ8BnwGhAaAB"
            "oAGgAaABoAGgAaABoAGgAaABoAGgAaABoAGgAaABoAGgAaABoAGgAaABoAGgAaABoAGgAaABoAGg"
            "AaABoAGgAaABoAGgAaABoAGhAZ8BoQGfAZ8BAQGfAZ8BAQGfAZ8BAQGfAZ8BAQGfAZ8BAQKeAZ8B"
            "AQKeAZ4BAgKeAZ4BAgKeAZ4BAgOdAZ4BAgOdAZ4BAgOdAZ0BAwScAZ0BAwScAZ0BAwScAZ0BAwSc"
            "AZwBBAWbAZwBBAWbAZwBBAWbAZsBBQaaAZsBBQaaAZoBBgeZAZoBBgeZAZoBBgeZAZkBBwiYAZkB"
            "BwiYAZgBCAmXAZgBCAmXAZgBCAmXAZcBCQqWAZcBCQqWAZYBCguVAZYBCguVAZUBCwyUAZUBCw2T"
            "AZQBDA2TAZQBDA6SAZMBDQ6SAZMBDQ+RAZIBDg+RAZIBDhCQAZEBDxCQAZABEBGPAZABEBKOAY8B"
            "ERONAY4BEhONAY4BEhSMAY0BExWLAYwBFBWLAYwBFBaKAYsBFReJAYoBFheJAYoBFhiIAYkBFxmH"
            "AYgBGBqGAYcBGRuFAYYBGhuFAYUBGxyEAYUBGx2DAYQBHB6CAYMBHR+BAYIBHiCAAYEBHyF/gAEg"
            "In5/ISJ+fiIjfX0jJHx8JCV7eyUmenomJ3l5Jyh4eCgpd3cpK3V2Kix0dSstc3QsLnJzLS9xci4w"
            "cHAwMm5vMTNtbjI0bG0zNWtrNTdpajY4aGk3OmZnOTtlZjo8ZGQ8PmJjPT9hYj5BX2BAQl5eQkRc"
            "XUNGWltFR1lZR0lXWEhLVVZKTVNUTE9RUk5RT1BQUk5OUlRMTFRWSkpWWUdIWFtFRVteQkNdYEBA"
            "YGI+PmJlOztlaDg4aGs1NWtuMjJuci4vcXUrK3V6JiZ6fiIifoMBHR2DAYsBFRWLAZUBCwuVAQAA");
    ok(match, "Figure does not match.\n");

    hr = ID2D1Factory_CreatePathGeometry(factory, &geometry);
    ok(SUCCEEDED(hr), "Failed to create path geometry, hr %#x.\n", hr);
    hr = ID2D1PathGeometry_Open(geometry, &sink);
    ok(SUCCEEDED(hr), "Failed to open geometry sink, hr %#x.\n", hr);

    set_point(&point, 240.0f, 720.0f);
    ID2D1GeometrySink_BeginFigure(sink, point, D2D1_FIGURE_BEGIN_FILLED);
    cubic_to(sink, 152.0f, 720.0f,  80.0f, 613.0f,  80.0f, 480.0f);
    cubic_to(sink,  80.0f, 347.0f, 152.0f, 240.0f, 240.0f, 240.0f);
    cubic_to(sink, 152.0f, 339.0f, 134.0f, 528.0f, 200.0f, 660.0f);
    cubic_to(sink, 212.0f, 683.0f, 225.0f, 703.0f, 240.0f, 720.0f);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_CLOSED);

    hr = ID2D1GeometrySink_Close(sink);
    ok(SUCCEEDED(hr), "Failed to close geometry sink, hr %#x.\n", hr);
    ID2D1GeometrySink_Release(sink);

    ID2D1RenderTarget_BeginDraw(rt);
    ID2D1RenderTarget_Clear(rt, &color);
    ID2D1RenderTarget_FillGeometry(rt, (ID2D1Geometry *)geometry, (ID2D1Brush *)brush, NULL);
    hr = ID2D1RenderTarget_EndDraw(rt, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to end draw, hr %#x.\n", hr);
    ID2D1PathGeometry_Release(geometry);

    match = compare_figure(&ctx, 160, 120, 320, 240, 0xff652e89, 2048,
            "pQIZkgIrhAI5/QE/9gFH7wFO6wFS5wFW4gFb3gFf2wFi2AFl1gFn1AFp0gFszwFuzQFxywFyyQF1"
            "xwF2xgF4xAF5xAF6wgF8wAF+vwF+vwF/vQGBAbwBggG7AYMBugGEAbkBhQG4AYYBtwGHAbcBiAG1"
            "AYkBtAGKAbQBigGzAYsBswGMAbEBjQGxAY0BsQGOAa8BjwGvAZABrgGQAa4BkQGtAZEBrQGSAawB"
            "kgGsAZMBqwGTAasBlAGrAZQBqgGVAakBlQGqAZUBqQGWAagBlwGoAZYBqAGXAagBlwGnAZgBpwGY"
            "AaYBmQGmAZkBpgGZAaUBmgGlAZoBpQGaAaUBmgGkAZsBpAGbAaQBmwGkAZsBpAGcAaMBnAGjAZwB"
            "owGcAaMBnAGjAZ0BogGdAaIBnQGiAZ4BoQGeAaEBngGiAZ4BoQGeAaEBnwGgAZ8BoQGeAaEBnwGh"
            "AZ4BoQGfAaABnwGhAZ8BoAGgAaABnwGgAaABoAGfAaABoAGgAaABnwGgAaABoAGgAaABnwGgAaAB"
            "oAGgAaABnwGhAZ8BoQGfAaABoAGgAaABoAGfAaEBnwGhAZ8BoQGfAaEBnwGhAZ8BoQGfAaABoAGg"
            "AaABoAGgAaEBnwGhAZ8BoQGfAaEBnwGhAaABoAGgAaABoAGgAaABoAGgAaEBoAGgAaABoAGgAaAB"
            "oQGgAaABoAGgAaABoQGfAaEBoAGhAZ8BoQGfAaIBnwGhAZ8BogGfAaEBnwGiAZ8BogGeAaIBnwGi"
            "AZ4BogGfAaIBngGjAZ4BowGdAaMBngGjAZ4BowGdAaQBnQGkAZ0BpAGcAaUBnAGlAZwBpQGcAaUB"
            "mwGmAZsBpgGbAaYBmwGmAZsBpgGbAacBmgGnAZkBqAGZAagBmQGpAZgBqQGZAagBmQGpAZgBqQGY"
            "AaoBlwGqAZcBqwGWAasBlgGsAZUBrQGVAawBlQGtAZQBrgGUAa0BlAGuAZMBrwGTAa8BkgGwAZEB"
            "sQGRAbEBkAGyAZABsgGPAbMBjwG0AY4BtAGNAbUBjQG2AYwBtgGLAbgBigG4AYoBuQGJAboBhwG7"
            "AYcBvAGGAb0BhQG+AYQBvwGDAcABggHBAYIBwgGAAcMBf8QBfsYBfMgBe8gBesoBeMwBd80BddAB"
            "c9EBcdQBb9YBbNkBatsBaN0BZeEBYuQBX+gBW+0BVvEBUvUBTvwBR4QCQIoCOZgCK6oCGQIA");
    ok(match, "Figure does not match.\n");

    ID2D1SolidColorBrush_Release(brush);
    ID2D1Factory_Release(factory);
    release_test_context(&ctx);
}

static void test_create_device(BOOL d3d11)
{
    D2D1_CREATION_PROPERTIES properties = {0};
    struct d2d1_test_context ctx;
    ID2D1Factory1 *factory;
    ID2D1Factory *factory2;
    ID2D1Device *device;
    ULONG refcount;
    HRESULT hr;

    if (!init_test_context(&ctx, d3d11))
        return;

    if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &IID_ID2D1Factory1, NULL, (void **)&factory)))
    {
        win_skip("ID2D1Factory1 is not supported.\n");
        release_test_context(&ctx);
        return;
    }

    hr = ID2D1Factory1_CreateDevice(factory, ctx.device, &device);
    ok(SUCCEEDED(hr), "Failed to get ID2D1Device, hr %#x.\n", hr);

    ID2D1Device_GetFactory(device, &factory2);
    ok(factory2 == (ID2D1Factory *)factory, "Got unexpected factory %p, expected %p.\n", factory2, factory);
    ID2D1Factory_Release(factory2);
    ID2D1Device_Release(device);

    if (pD2D1CreateDevice)
    {
        hr = pD2D1CreateDevice(ctx.device, NULL, &device);
        ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
        ID2D1Device_Release(device);

        hr = pD2D1CreateDevice(ctx.device, &properties, &device);
        ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
        ID2D1Device_Release(device);
    }
    else
        win_skip("D2D1CreateDevice() is unavailable.\n");

    release_test_context(&ctx);

    refcount = ID2D1Factory1_Release(factory);
    ok(!refcount, "Factory has %u references left.\n", refcount);
}

#define check_rt_bitmap_surface(r, s, o) check_rt_bitmap_surface_(__LINE__, r, s, o)
static void check_rt_bitmap_surface_(unsigned int line, ID2D1RenderTarget *rt, BOOL has_surface, DWORD options)
{
    ID2D1BitmapRenderTarget *compatible_rt;
    D2D1_BITMAP_PROPERTIES bitmap_desc;
    IWICImagingFactory *wic_factory;
    ID2D1Bitmap *bitmap, *bitmap2;
    ID2D1DeviceContext *context;
    ID2D1DCRenderTarget *dc_rt;
    IWICBitmap *wic_bitmap;
    ID2D1Image *target;
    D2D1_SIZE_U size;
    HRESULT hr;

    static const DWORD bitmap_data[] =
    {
        0x7f7f0000,
    };

    /* Raw data bitmap. */
    set_size_u(&size, 1, 1);
    bitmap_desc.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    bitmap_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;
    bitmap_desc.dpiX = 96.0f;
    bitmap_desc.dpiY = 96.0f;
    hr = ID2D1RenderTarget_CreateBitmap(rt, size, bitmap_data, sizeof(*bitmap_data), &bitmap_desc, &bitmap);
    ok_(__FILE__, line)(SUCCEEDED(hr), "Failed to create bitmap, hr %#x.\n", hr);

    check_bitmap_surface_(line, bitmap, has_surface, options);

    ID2D1Bitmap_Release(bitmap);

    /* Zero sized bitmaps. */
    set_size_u(&size, 0, 0);
    hr = ID2D1RenderTarget_CreateBitmap(rt, size, NULL, 0, &bitmap_desc, &bitmap);
    ok_(__FILE__, line)(SUCCEEDED(hr), "Failed to create bitmap, hr %#x.\n", hr);
    check_bitmap_surface_(line, bitmap, has_surface, options);
    ID2D1Bitmap_Release(bitmap);

    set_size_u(&size, 2, 0);
    hr = ID2D1RenderTarget_CreateBitmap(rt, size, NULL, 0, &bitmap_desc, &bitmap);
    ok_(__FILE__, line)(SUCCEEDED(hr), "Failed to create bitmap, hr %#x.\n", hr);
    check_bitmap_surface_(line, bitmap, has_surface, options);
    ID2D1Bitmap_Release(bitmap);

    set_size_u(&size, 0, 2);
    hr = ID2D1RenderTarget_CreateBitmap(rt, size, NULL, 0, &bitmap_desc, &bitmap);
    ok_(__FILE__, line)(SUCCEEDED(hr), "Failed to create bitmap, hr %#x.\n", hr);
    check_bitmap_surface_(line, bitmap, has_surface, options);
    ID2D1Bitmap_Release(bitmap);

    /* WIC bitmap. */
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
            &IID_IWICImagingFactory, (void **)&wic_factory);
    ok_(__FILE__, line)(SUCCEEDED(hr), "Failed to create WIC imaging factory, hr %#x.\n", hr);

    hr = IWICImagingFactory_CreateBitmap(wic_factory, 16, 16,
            &GUID_WICPixelFormat32bppPBGRA, WICBitmapCacheOnDemand, &wic_bitmap);
    ok_(__FILE__, line)(SUCCEEDED(hr), "Failed to create WIC bitmap, hr %#x.\n", hr);
    IWICImagingFactory_Release(wic_factory);

    hr = ID2D1RenderTarget_CreateBitmapFromWicBitmap(rt, (IWICBitmapSource *)wic_bitmap, NULL, &bitmap);
    ok_(__FILE__, line)(SUCCEEDED(hr), "Failed to create bitmap from WIC source, hr %#x.\n", hr);

    check_bitmap_surface_(line, bitmap, has_surface, options);

    ID2D1Bitmap_Release(bitmap);

    CoUninitialize();

    /* Compatible target follows its parent. */
    hr = ID2D1RenderTarget_QueryInterface(rt, &IID_ID2D1DeviceContext, (void **)&context);
    ok_(__FILE__, line)(SUCCEEDED(hr), "Failed to get device context, hr %#x.\n", hr);

    dc_rt = NULL;
    ID2D1RenderTarget_QueryInterface(rt, &IID_ID2D1DCRenderTarget, (void **)&dc_rt);

    bitmap = NULL;
    target = NULL;
    ID2D1DeviceContext_GetTarget(context, &target);
    if (target && FAILED(ID2D1Image_QueryInterface(target, &IID_ID2D1Bitmap, (void **)&bitmap)))
    {
        ID2D1Image_Release(target);
        target = NULL;
    }
    if (bitmap)
    {
        D2D1_PIXEL_FORMAT rt_format, bitmap_format;

        rt_format = ID2D1RenderTarget_GetPixelFormat(rt);
        bitmap_format = ID2D1Bitmap_GetPixelFormat(bitmap);
        ok_(__FILE__, line)(!memcmp(&rt_format, &bitmap_format, sizeof(rt_format)), "Unexpected bitmap format.\n");

        ID2D1Bitmap_Release(bitmap);
    }

    /* Pixel format is not defined until target is set, for DC target it's specified on creation. */
    if (target || dc_rt)
    {
        ID2D1Device *device, *device2;
        ID2D1DeviceContext *context2;

        ID2D1DeviceContext_GetDevice(context, &device);

        hr = ID2D1RenderTarget_CreateCompatibleRenderTarget(rt, NULL, NULL, NULL,
                D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS_NONE, &compatible_rt);
        ok_(__FILE__, line)(SUCCEEDED(hr), "Failed to create compatible render target, hr %#x.\n", hr);

        hr = ID2D1BitmapRenderTarget_QueryInterface(compatible_rt, &IID_ID2D1DeviceContext, (void **)&context2);
        ok_(__FILE__, line)(SUCCEEDED(hr), "Failed to get device context, hr %#x.\n", hr);

        ID2D1DeviceContext_GetDevice(context2, &device2);
        ok_(__FILE__, line)(device == device2, "Unexpected device.\n");

        ID2D1Device_Release(device);
        ID2D1Device_Release(device2);

        hr = ID2D1BitmapRenderTarget_CreateBitmap(compatible_rt, size,
                bitmap_data, sizeof(*bitmap_data), &bitmap_desc, &bitmap);
        ok_(__FILE__, line)(SUCCEEDED(hr), "Failed to create bitmap, hr %#x.\n", hr);
        check_bitmap_surface_(line, bitmap, has_surface, options);
        ID2D1Bitmap_Release(bitmap);

        hr = ID2D1BitmapRenderTarget_GetBitmap(compatible_rt, &bitmap);
        ok_(__FILE__, line)(SUCCEEDED(hr), "Failed to get compatible target bitmap, hr %#x.\n", hr);

        bitmap2 = NULL;
        ID2D1DeviceContext_GetTarget(context2, (ID2D1Image **)&bitmap2);
        ok_(__FILE__, line)(bitmap2 == bitmap, "Unexpected bitmap.\n");

        check_bitmap_surface_(line, bitmap, has_surface, D2D1_BITMAP_OPTIONS_TARGET);
        ID2D1Bitmap_Release(bitmap2);
        ID2D1Bitmap_Release(bitmap);

        ID2D1BitmapRenderTarget_Release(compatible_rt);
        ID2D1DeviceContext_Release(context2);
    }
    else
    {
        hr = ID2D1RenderTarget_CreateCompatibleRenderTarget(rt, NULL, NULL, NULL,
                 D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS_NONE, &compatible_rt);
        ok_(__FILE__, line)(hr == WINCODEC_ERR_UNSUPPORTEDPIXELFORMAT, "Unexpected hr %#x.\n", hr);
    }

    ID2D1DeviceContext_Release(context);
    if (target)
        ID2D1Image_Release(target);
    if (dc_rt)
        ID2D1DCRenderTarget_Release(dc_rt);
}

static IDXGISurface *create_surface(IDXGIDevice *dxgi_device, DXGI_FORMAT format)
{
    D3D10_TEXTURE2D_DESC texture_desc;
    ID3D10Texture2D *texture;
    ID3D10Device *d3d_device;
    IDXGISurface *surface;
    HRESULT hr;

    texture_desc.Width = 1;
    texture_desc.Height = 1;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = format;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.SampleDesc.Quality = 0;
    texture_desc.Usage = D3D10_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
    texture_desc.CPUAccessFlags = 0;
    texture_desc.MiscFlags = 0;

    hr = IDXGIDevice_QueryInterface(dxgi_device, &IID_ID3D10Device, (void **)&d3d_device);
    ok(SUCCEEDED(hr), "Failed to get device interface, hr %#x.\n", hr);

    hr = ID3D10Device_CreateTexture2D(d3d_device, &texture_desc, NULL, &texture);
    ok(SUCCEEDED(hr), "Failed to create a texture, hr %#x.\n", hr);

    hr = ID3D10Texture2D_QueryInterface(texture, &IID_IDXGISurface, (void **)&surface);
    ok(SUCCEEDED(hr), "Failed to get surface interface, hr %#x.\n", hr);

    ID3D10Device_Release(d3d_device);
    ID3D10Texture2D_Release(texture);

    return surface;
}

static void test_bitmap_surface(BOOL d3d11)
{
    static const struct bitmap_format_test
    {
        D2D1_PIXEL_FORMAT original;
        D2D1_PIXEL_FORMAT result;
        HRESULT hr;
    }
    bitmap_format_tests[] =
    {
        { { DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED },
          { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED } },

        { { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_STRAIGHT }, { 0 }, WINCODEC_ERR_UNSUPPORTEDPIXELFORMAT },

        { { DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_IGNORE },
          { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE } },

        { { DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_UNKNOWN }, { 0 }, WINCODEC_ERR_UNSUPPORTEDPIXELFORMAT },

        { { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_UNKNOWN }, { 0 }, WINCODEC_ERR_UNSUPPORTEDPIXELFORMAT },

        { { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE },
          { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE } },

        { { DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_STRAIGHT }, { 0 }, WINCODEC_ERR_UNSUPPORTEDPIXELFORMAT },
    };
    D2D1_HWND_RENDER_TARGET_PROPERTIES hwnd_rt_desc;
    D2D1_RENDER_TARGET_PROPERTIES rt_desc;
    D2D1_BITMAP_PROPERTIES1 bitmap_desc;
    ID2D1DeviceContext *device_context;
    IDXGISurface *surface2;
    D2D1_PIXEL_FORMAT pixel_format;
    struct d2d1_test_context ctx;
    ID2D1Factory1 *factory;
    ID2D1RenderTarget *rt;
    ID2D1Bitmap1 *bitmap;
    ID2D1Device *device;
    ID2D1Image *target;
    D2D1_SIZE_U size;
    D2D1_TAG t1, t2;
    unsigned int i;
    HRESULT hr;

    IWICBitmap *wic_bitmap;
    IWICImagingFactory *wic_factory;

    if (!init_test_context(&ctx, d3d11))
        return;

    if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &IID_ID2D1Factory1, NULL, (void **)&factory)))
    {
        win_skip("ID2D1Factory1 is not supported.\n");
        release_test_context(&ctx);
        return;
    }

    /* DXGI target */
    hr = ID2D1RenderTarget_QueryInterface(ctx.rt, &IID_ID2D1DeviceContext, (void **)&device_context);
    ok(SUCCEEDED(hr), "Failed to get device context, hr %#x.\n", hr);

    bitmap = NULL;
    ID2D1DeviceContext_GetTarget(device_context, (ID2D1Image **)&bitmap);
    ok(!!bitmap, "Unexpected target.\n");
    check_bitmap_surface((ID2D1Bitmap *)bitmap, TRUE, D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW);
    ID2D1Bitmap1_Release(bitmap);

    check_rt_bitmap_surface(ctx.rt, TRUE, D2D1_BITMAP_OPTIONS_NONE);

    ID2D1DeviceContext_Release(device_context);

    /* Bitmap created from DXGI surface. */
    hr = ID2D1Factory1_CreateDevice(factory, ctx.device, &device);
    ok(SUCCEEDED(hr), "Failed to get ID2D1Device, hr %#x.\n", hr);

    hr = ID2D1Device_CreateDeviceContext(device, D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &device_context);
    ok(SUCCEEDED(hr), "Failed to create device context, hr %#x.\n", hr);

    for (i = 0; i < ARRAY_SIZE(bitmap_format_tests); ++i)
    {
        memset(&bitmap_desc, 0, sizeof(bitmap_desc));
        bitmap_desc.pixelFormat = bitmap_format_tests[i].original;
        bitmap_desc.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;

        hr = ID2D1DeviceContext_CreateBitmapFromDxgiSurface(device_context, ctx.surface, &bitmap_desc, &bitmap);
    todo_wine_if(bitmap_format_tests[i].hr == WINCODEC_ERR_UNSUPPORTEDPIXELFORMAT)
        ok(hr == bitmap_format_tests[i].hr, "%u: unexpected hr %#x.\n", i, hr);

        if (SUCCEEDED(bitmap_format_tests[i].hr))
        {
            pixel_format = ID2D1Bitmap1_GetPixelFormat(bitmap);

            ok(pixel_format.format == bitmap_format_tests[i].result.format, "%u: unexpected pixel format %#x.\n",
                    i, pixel_format.format);
            ok(pixel_format.alphaMode == bitmap_format_tests[i].result.alphaMode, "%u: unexpected alpha mode %d.\n",
                    i, pixel_format.alphaMode);

            ID2D1Bitmap1_Release(bitmap);
        }
    }

    /* A8 surface */
    surface2 = create_surface(ctx.device, DXGI_FORMAT_A8_UNORM);

    hr = ID2D1DeviceContext_CreateBitmapFromDxgiSurface(device_context, surface2, NULL, &bitmap);
    ok(SUCCEEDED(hr) || broken(hr == WINCODEC_ERR_UNSUPPORTEDPIXELFORMAT) /* Win7 */,
            "Failed to create a bitmap, hr %#x.\n", hr);

    if (SUCCEEDED(hr))
    {
        pixel_format = ID2D1Bitmap1_GetPixelFormat(bitmap);
        ok(pixel_format.alphaMode == D2D1_ALPHA_MODE_PREMULTIPLIED,
                "Unexpected alpha mode %#x.\n", pixel_format.alphaMode);

        ID2D1Bitmap1_Release(bitmap);
    }

    IDXGISurface_Release(surface2);

    hr = ID2D1DeviceContext_CreateBitmapFromDxgiSurface(device_context, ctx.surface, NULL, &bitmap);
    ok(SUCCEEDED(hr), "Failed to create a bitmap, hr %#x.\n", hr);

    pixel_format = ID2D1Bitmap1_GetPixelFormat(bitmap);
    ok(pixel_format.alphaMode == D2D1_ALPHA_MODE_PREMULTIPLIED,
            "Unexpected alpha mode %#x.\n", pixel_format.alphaMode);

    check_bitmap_surface((ID2D1Bitmap *)bitmap, TRUE, D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW);
    check_rt_bitmap_surface((ID2D1RenderTarget *)device_context, TRUE, D2D1_BITMAP_OPTIONS_NONE);

    ID2D1DeviceContext_SetTarget(device_context, (ID2D1Image *)bitmap);
    ID2D1DeviceContext_GetTarget(device_context, &target);
    ok(target == (ID2D1Image *)bitmap, "Unexpected target.\n");

    check_rt_bitmap_surface((ID2D1RenderTarget *)device_context, TRUE, D2D1_BITMAP_OPTIONS_NONE);

    ID2D1Bitmap1_Release(bitmap);

    /* Without D2D1_BITMAP_OPTIONS_TARGET. */
    memset(&bitmap_desc, 0, sizeof(bitmap_desc));
    bitmap_desc.pixelFormat = ID2D1DeviceContext_GetPixelFormat(device_context);
    size.width = size.height = 4;
    hr = ID2D1DeviceContext_CreateBitmap(device_context, size, NULL, 0, &bitmap_desc, &bitmap);
    ok(SUCCEEDED(hr), "Failed to create a bitmap, hr %#x.\n", hr);
    check_bitmap_surface((ID2D1Bitmap *)bitmap, TRUE, D2D1_BITMAP_OPTIONS_NONE);
    ID2D1DeviceContext_SetTags(device_context, 1, 2);

    ID2D1DeviceContext_BeginDraw(device_context);
    ID2D1DeviceContext_SetTarget(device_context, (ID2D1Image *)bitmap);
    hr = ID2D1DeviceContext_EndDraw(device_context, &t1, &t2);
    ok(hr == D2DERR_INVALID_TARGET, "Unexpected hr %#x.\n", hr);
    ok(t1 == 1 && t2 == 2, "Unexpected tags %s:%s.\n", wine_dbgstr_longlong(t1), wine_dbgstr_longlong(t2));

    ID2D1Bitmap1_Release(bitmap);

    ID2D1DeviceContext_GetTarget(device_context, (ID2D1Image **)&bitmap);
    ok(!!bitmap, "Expected target bitmap.\n");
    ID2D1Bitmap1_Release(bitmap);

    bitmap_desc.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET;
    hr = ID2D1DeviceContext_CreateBitmap(device_context, size, NULL, 0, &bitmap_desc, &bitmap);
    ok(SUCCEEDED(hr), "Failed to create a bitmap, hr %#x.\n", hr);
    check_bitmap_surface((ID2D1Bitmap *)bitmap, TRUE, D2D1_BITMAP_OPTIONS_TARGET);
    ID2D1DeviceContext_SetTarget(device_context, NULL);
    ID2D1DeviceContext_SetTags(device_context, 3, 4);

    ID2D1DeviceContext_BeginDraw(device_context);
    ID2D1DeviceContext_SetTarget(device_context, (ID2D1Image *)bitmap);
    hr = ID2D1DeviceContext_EndDraw(device_context, &t1, &t2);
    ok(SUCCEEDED(hr), "Failed to end draw, hr %#x.\n", hr);
    ok(!t1 && !t2, "Unexpected tags %s:%s.\n", wine_dbgstr_longlong(t1), wine_dbgstr_longlong(t2));

    ID2D1Bitmap1_Release(bitmap);

    ID2D1DeviceContext_Release(device_context);

    ID2D1Device_Release(device);

    /* DC target */
    rt_desc.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
    rt_desc.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    rt_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    rt_desc.dpiX = 96.0f;
    rt_desc.dpiY = 96.0f;
    rt_desc.usage = D2D1_RENDER_TARGET_USAGE_NONE;
    rt_desc.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;
    hr = ID2D1Factory1_CreateDCRenderTarget(factory, &rt_desc, (ID2D1DCRenderTarget **)&rt);
    ok(SUCCEEDED(hr), "Failed to create target, hr %#x.\n", hr);

    hr = ID2D1RenderTarget_QueryInterface(rt, &IID_ID2D1DeviceContext, (void **)&device_context);
    ok(SUCCEEDED(hr), "Failed to get device context, hr %#x.\n", hr);

    ID2D1DeviceContext_GetTarget(device_context, (ID2D1Image **)&bitmap);
    ok(!bitmap, "Unexpected target.\n");

    ID2D1DeviceContext_Release(device_context);
    ID2D1RenderTarget_Release(rt);

    /* HWND target */
    hwnd_rt_desc.hwnd = NULL;
    hwnd_rt_desc.pixelSize.width = 64;
    hwnd_rt_desc.pixelSize.height = 64;
    hwnd_rt_desc.presentOptions = D2D1_PRESENT_OPTIONS_NONE;
    hwnd_rt_desc.hwnd = CreateWindowA("static", "d2d_test", 0, 0, 0, 0, 0, 0, 0, 0, 0);
    ok(!!hwnd_rt_desc.hwnd, "Failed to create target window.\n");

    hr = ID2D1Factory1_CreateHwndRenderTarget(factory, &rt_desc, &hwnd_rt_desc, (ID2D1HwndRenderTarget **)&rt);
    ok(SUCCEEDED(hr), "Failed to create render target, hr %#x.\n", hr);

    check_rt_bitmap_surface(rt, FALSE, D2D1_BITMAP_OPTIONS_NONE);
    ID2D1RenderTarget_Release(rt);
    DestroyWindow(hwnd_rt_desc.hwnd);

    /* WIC target */
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
            &IID_IWICImagingFactory, (void **)&wic_factory);
    ok(SUCCEEDED(hr), "Failed to create WIC imaging factory, hr %#x.\n", hr);
    hr = IWICImagingFactory_CreateBitmap(wic_factory, 16, 16,
            &GUID_WICPixelFormat32bppPBGRA, WICBitmapCacheOnDemand, &wic_bitmap);
    ok(SUCCEEDED(hr), "Failed to create bitmap, hr %#x.\n", hr);
    IWICImagingFactory_Release(wic_factory);

    hr = ID2D1Factory1_CreateWicBitmapRenderTarget(factory, wic_bitmap, &rt_desc, &rt);
    ok(SUCCEEDED(hr), "Failed to create render target, hr %#x.\n", hr);

    check_rt_bitmap_surface(rt, FALSE, D2D1_BITMAP_OPTIONS_NONE);
    ID2D1RenderTarget_Release(rt);

    CoUninitialize();

    ID2D1Factory1_Release(factory);
    release_test_context(&ctx);
}

static void test_device_context(BOOL d3d11)
{
    D2D1_HWND_RENDER_TARGET_PROPERTIES hwnd_rt_desc;
    D2D1_RENDER_TARGET_PROPERTIES rt_desc;
    ID2D1DeviceContext *device_context;
    IDXGISurface *surface, *surface2;
    ID2D1Device *device, *device2;
    struct d2d1_test_context ctx;
    D2D1_BITMAP_OPTIONS options;
    ID2D1DCRenderTarget *dc_rt;
    D2D1_UNIT_MODE unit_mode;
    ID2D1Factory1 *factory;
    ID2D1RenderTarget *rt;
    ID2D1Bitmap1 *bitmap;
    ID2D1Image *target;
    HRESULT hr;
    RECT rect;
    HDC hdc;

    IWICBitmap *wic_bitmap;
    IWICImagingFactory *wic_factory;

    if (!init_test_context(&ctx, d3d11))
        return;

    if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &IID_ID2D1Factory1, NULL, (void **)&factory)))
    {
        win_skip("ID2D1Factory1 is not supported.\n");
        release_test_context(&ctx);
        return;
    }

    hr = ID2D1Factory1_CreateDevice(factory, ctx.device, &device);
    ok(SUCCEEDED(hr), "Failed to get ID2D1Device, hr %#x.\n", hr);

    hr = ID2D1Device_CreateDeviceContext(device, D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &device_context);
    ok(SUCCEEDED(hr), "Failed to create device context, hr %#x.\n", hr);

    ID2D1DeviceContext_GetDevice(device_context, &device2);
    ok(device2 == device, "Unexpected device instance.\n");
    ID2D1Device_Release(device2);

    target = (void *)0xdeadbeef;
    ID2D1DeviceContext_GetTarget(device_context, &target);
    ok(target == NULL, "Unexpected target instance %p.\n", target);

    unit_mode = ID2D1DeviceContext_GetUnitMode(device_context);
    ok(unit_mode == D2D1_UNIT_MODE_DIPS, "Unexpected unit mode %d.\n", unit_mode);

    ID2D1DeviceContext_Release(device_context);

    /* DXGI target */
    rt = ctx.rt;
    hr = ID2D1RenderTarget_QueryInterface(rt, &IID_ID2D1DeviceContext, (void **)&device_context);
    ok(SUCCEEDED(hr), "Failed to get device context interface, hr %#x.\n", hr);
    ID2D1DeviceContext_GetTarget(device_context, (ID2D1Image **)&bitmap);
    options = ID2D1Bitmap1_GetOptions(bitmap);
    ok(options == (D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW),
            "Unexpected bitmap options %#x.\n", options);
    hr = ID2D1Bitmap1_GetSurface(bitmap, &surface2);
    ok(SUCCEEDED(hr), "Failed to get bitmap surface, hr %#x.\n", hr);
    ok(surface2 == ctx.surface, "Unexpected surface instance.\n");
    IDXGISurface_Release(surface2);

    ID2D1DeviceContext_BeginDraw(device_context);
    hr = ID2D1Bitmap1_GetSurface(bitmap, &surface2);
    ok(SUCCEEDED(hr), "Failed to get bitmap surface, hr %#x.\n", hr);
    ok(surface2 == ctx.surface, "Unexpected surface instance.\n");
    IDXGISurface_Release(surface2);
    ID2D1DeviceContext_EndDraw(device_context, NULL, NULL);
    ID2D1Bitmap1_Release(bitmap);

    ID2D1DeviceContext_SetTarget(device_context, NULL);
    ID2D1DeviceContext_GetTarget(device_context, (ID2D1Image **)&bitmap);
    ok(bitmap == NULL, "Unexpected target instance.\n");

    ID2D1DeviceContext_Release(device_context);

    /* WIC target */
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
            &IID_IWICImagingFactory, (void **)&wic_factory);
    ok(SUCCEEDED(hr), "Failed to create WIC imaging factory, hr %#x.\n", hr);
    hr = IWICImagingFactory_CreateBitmap(wic_factory, 16, 16,
            &GUID_WICPixelFormat32bppPBGRA, WICBitmapCacheOnDemand, &wic_bitmap);
    ok(SUCCEEDED(hr), "Failed to create bitmap, hr %#x.\n", hr);
    IWICImagingFactory_Release(wic_factory);

    rt_desc.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
    rt_desc.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    rt_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    rt_desc.dpiX = 96.0f;
    rt_desc.dpiY = 96.0f;
    rt_desc.usage = D2D1_RENDER_TARGET_USAGE_NONE;
    rt_desc.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;
    hr = ID2D1Factory1_CreateWicBitmapRenderTarget(factory, wic_bitmap, &rt_desc, &rt);
    ok(SUCCEEDED(hr), "Failed to create render target, hr %#x.\n", hr);

    hr = ID2D1RenderTarget_QueryInterface(rt, &IID_ID2D1DeviceContext, (void **)&device_context);
    ok(SUCCEEDED(hr), "Failed to get device context interface, hr %#x.\n", hr);
    ID2D1DeviceContext_GetTarget(device_context, (ID2D1Image **)&bitmap);
    options = ID2D1Bitmap1_GetOptions(bitmap);
    ok(options == (D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW),
            "Unexpected bitmap options %#x.\n", options);
    hr = ID2D1Bitmap1_GetSurface(bitmap, &surface);
todo_wine
    ok(hr == E_FAIL, "Unexpected hr %#x.\n", hr);
    ID2D1Bitmap1_Release(bitmap);

    ID2D1DeviceContext_SetTarget(device_context, NULL);
    ID2D1DeviceContext_GetTarget(device_context, (ID2D1Image **)&bitmap);
    ok(bitmap == NULL, "Unexpected target instance.\n");

    ID2D1DeviceContext_Release(device_context);
    ID2D1RenderTarget_Release(rt);

    CoUninitialize();

    /* HWND target */
    hwnd_rt_desc.hwnd = NULL;
    hwnd_rt_desc.pixelSize.width = 64;
    hwnd_rt_desc.pixelSize.height = 64;
    hwnd_rt_desc.presentOptions = D2D1_PRESENT_OPTIONS_NONE;
    hwnd_rt_desc.hwnd = CreateWindowA("static", "d2d_test", 0, 0, 0, 0, 0, 0, 0, 0, 0);
    ok(!!hwnd_rt_desc.hwnd, "Failed to create target window.\n");

    hr = ID2D1Factory1_CreateHwndRenderTarget(factory, &rt_desc, &hwnd_rt_desc, (ID2D1HwndRenderTarget **)&rt);
    ok(SUCCEEDED(hr), "Failed to create render target, hr %#x.\n", hr);

    hr = ID2D1RenderTarget_QueryInterface(rt, &IID_ID2D1DeviceContext, (void **)&device_context);
    ok(SUCCEEDED(hr), "Failed to get device context interface, hr %#x.\n", hr);
    ID2D1DeviceContext_GetTarget(device_context, (ID2D1Image **)&bitmap);
    options = ID2D1Bitmap1_GetOptions(bitmap);
    ok(options == (D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW),
            "Unexpected bitmap options %#x.\n", options);
    hr = ID2D1Bitmap1_GetSurface(bitmap, &surface);
    ok(hr == D2DERR_INVALID_CALL, "Unexpected hr %#x.\n", hr);
    ID2D1Bitmap1_Release(bitmap);

    ID2D1DeviceContext_SetTarget(device_context, NULL);
    ID2D1DeviceContext_GetTarget(device_context, (ID2D1Image **)&bitmap);
    ok(bitmap == NULL, "Unexpected target instance.\n");

    ID2D1DeviceContext_Release(device_context);
    ID2D1RenderTarget_Release(rt);
    DestroyWindow(hwnd_rt_desc.hwnd);

    /* DC target */
    hr = ID2D1Factory1_CreateDCRenderTarget(factory, &rt_desc, &dc_rt);
    ok(SUCCEEDED(hr), "Failed to create target, hr %#x.\n", hr);

    hr = ID2D1DCRenderTarget_QueryInterface(dc_rt, &IID_ID2D1DeviceContext, (void **)&device_context);
    ok(SUCCEEDED(hr), "Failed to get device context interface, hr %#x.\n", hr);
    ID2D1DeviceContext_GetTarget(device_context, (ID2D1Image **)&bitmap);
    ok(bitmap == NULL, "Unexpected bitmap instance.\n");

    hdc = CreateCompatibleDC(NULL);
    ok(hdc != NULL, "Failed to create an HDC.\n");

    create_target_dibsection(hdc, 16, 16);

    SetRect(&rect, 0, 0, 16, 16);
    hr = ID2D1DCRenderTarget_BindDC(dc_rt, hdc, &rect);
    ok(SUCCEEDED(hr), "BindDC() failed, hr %#x.\n", hr);

    ID2D1DeviceContext_GetTarget(device_context, (ID2D1Image **)&bitmap);
    options = ID2D1Bitmap1_GetOptions(bitmap);
    ok(options == (D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW | D2D1_BITMAP_OPTIONS_GDI_COMPATIBLE),
            "Unexpected bitmap options %#x.\n", options);
    hr = ID2D1Bitmap1_GetSurface(bitmap, &surface);
    ok(hr == D2DERR_INVALID_CALL, "Unexpected hr %#x.\n", hr);
    ID2D1Bitmap1_Release(bitmap);

    ID2D1DeviceContext_SetTarget(device_context, NULL);
    ID2D1DeviceContext_GetTarget(device_context, (ID2D1Image **)&bitmap);
    ok(bitmap == NULL, "Unexpected target instance.\n");

    ID2D1DeviceContext_Release(device_context);
    ID2D1DCRenderTarget_Release(dc_rt);
    DeleteDC(hdc);

    ID2D1Device_Release(device);
    ID2D1Factory1_Release(factory);
    release_test_context(&ctx);
}

static void test_invert_matrix(BOOL d3d11)
{
    static const struct
    {
        D2D1_MATRIX_3X2_F matrix;
        D2D1_MATRIX_3X2_F inverse;
        BOOL invertible;
    }
    invert_tests[] =
    {
        { {{{ 0 }}}, {{{ 0 }}}, FALSE },
        {
            {{{
               1.0f, 2.0f,
               1.0f, 2.0f,
               4.0f, 8.0f
            }}},
            {{{
               1.0f, 2.0f,
               1.0f, 2.0f,
               4.0f, 8.0f
            }}},
            FALSE
        },
        {
            {{{
               2.0f, 0.0f,
               0.0f, 2.0f,
               4.0f, 8.0f
            }}},
            {{{
               0.5f, -0.0f,
              -0.0f,  0.5f,
              -2.0f, -4.0f
            }}},
            TRUE
        },
        {
            {{{
               2.0f, 1.0f,
               2.0f, 2.0f,
               4.0f, 8.0f
            }}},
            {{{
               1.0f, -0.5f,
              -1.0f,  1.0f,
               4.0f, -6.0f
            }}},
            TRUE
        },
        {
            {{{
               2.0f, 1.0f,
               3.0f, 1.0f,
               4.0f, 8.0f
            }}},
            {{{
              -1.0f,  1.0f,
               3.0f, -2.0f,
             -20.0f, 12.0f
            }}},
            TRUE
        },
    };
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(invert_tests); ++i)
    {
        D2D1_MATRIX_3X2_F m;
        BOOL ret;

        m = invert_tests[i].matrix;
        ret = D2D1InvertMatrix(&m);
        ok(ret == invert_tests[i].invertible, "%u: unexpected return value %d.\n", i, ret);
        ok(!memcmp(&m, &invert_tests[i].inverse, sizeof(m)),
                "%u: unexpected matrix value {%.8e, %.8e, %.8e, %.8e, %.8e, %.8e}.\n", i,
                m._11, m._12, m._21, m._22, m._31, m._32);

        ret = D2D1IsMatrixInvertible(&invert_tests[i].matrix);
        ok(ret == invert_tests[i].invertible, "%u: unexpected return value %d.\n", i, ret);
    }
}

static void test_skew_matrix(BOOL d3d11)
{
    static const struct
    {
        float angle_x;
        float angle_y;
        D2D1_POINT_2F center;
        D2D1_MATRIX_3X2_F matrix;
    }
    skew_tests[] =
    {
        { 0.0f, 0.0f, { 0.0f, 0.0f }, {{{ 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f }}} },
        { 45.0f, 0.0f, { 0.0f, 0.0f }, {{{ 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f }}} },
        { 0.0f, 0.0f, { 10.0f, -3.0f }, {{{ 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f }}} },
        { -45.0f, 45.0f, { 0.1f, 0.5f }, {{{ 1.0f, 1.0f, -1.0f, 1.0f, 0.5f, -0.1f }}} },
        { -45.0f, 45.0f, { 1.0f, 2.0f }, {{{ 1.0f, 1.0f, -1.0f, 1.0f, 2.0f, -1.0f }}} },
        { 45.0f, -45.0f, { 1.0f, 2.0f }, {{{ 1.0f, -1.0f, 1.0f, 1.0f, -2.0f, 1.0f }}} },
        { 30.0f, -60.0f, { 12.0f, -5.0f }, {{{ 1.0f, -1.7320509f, 0.577350259f, 1.0f, 2.88675117f, 20.7846107f }}} },
    };
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(skew_tests); ++i)
    {
        const D2D1_MATRIX_3X2_F *expected = &skew_tests[i].matrix;
        D2D1_MATRIX_3X2_F m;
        BOOL ret;

        D2D1MakeSkewMatrix(skew_tests[i].angle_x, skew_tests[i].angle_y, skew_tests[i].center, &m);
        ret = compare_float(m._11, expected->_11, 3) && compare_float(m._12, expected->_12, 3)
                && compare_float(m._21, expected->_21, 3) && compare_float(m._22, expected->_22, 3)
                && compare_float(m._31, expected->_31, 3) && compare_float(m._32, expected->_32, 3);

        ok(ret, "%u: unexpected matrix value {%.8e, %.8e, %.8e, %.8e, %.8e, %.8e}, expected "
                "{%.8e, %.8e, %.8e, %.8e, %.8e, %.8e}.\n", i, m._11, m._12, m._21, m._22, m._31, m._32,
                expected->_11, expected->_12, expected->_21, expected->_22, expected->_31, expected->_32);
    }
}

static ID2D1DeviceContext *create_device_context(ID2D1Factory1 *factory, IDXGIDevice *dxgi_device, BOOL d3d11)
{
    ID2D1DeviceContext *device_context;
    ID2D1Device *device;
    HRESULT hr;

    hr = ID2D1Factory1_CreateDevice(factory, dxgi_device, &device);
    ok(SUCCEEDED(hr), "Failed to get ID2D1Device, hr %#x.\n", hr);

    hr = ID2D1Device_CreateDeviceContext(device, D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &device_context);
    ok(SUCCEEDED(hr), "Failed to create device context, hr %#x.\n", hr);
    ID2D1Device_Release(device);

    return device_context;
}

static void test_command_list(BOOL d3d11)
{
    static const DWORD bitmap_data[] =
    {
        0xffff0000, 0xffffff00, 0xff00ff00, 0xff00ffff,
    };
    static const D2D1_GRADIENT_STOP stops[] =
    {
        {0.0f, {1.0f, 0.0f, 0.0f, 1.0f}},
        {0.5f, {0.0f, 1.0f, 0.0f, 1.0f}},
        {1.0f, {0.0f, 0.0f, 1.0f, 1.0f}},
    };
    D2D1_RADIAL_GRADIENT_BRUSH_PROPERTIES radial_gradient_properties;
    D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES linear_gradient_properties;
    ID2D1DeviceContext *device_context, *device_context2;
    D2D1_STROKE_STYLE_PROPERTIES stroke_desc;
    ID2D1GradientStopCollection *gradient;
    D2D1_BITMAP_PROPERTIES bitmap_desc;
    ID2D1StrokeStyle *stroke_style;
    ID2D1CommandList *command_list;
    struct d2d1_test_context ctx;
    ID2D1Geometry *geometry;
    ID2D1Factory1 *factory;
    ID2D1RenderTarget *rt;
    D2D1_POINT_2F p0, p1;
    ID2D1Bitmap *bitmap;
    ID2D1Image *target;
    D2D1_COLOR_F color;
    ID2D1Brush *brush;
    D2D1_RECT_F rect;
    D2D_SIZE_U size;
    ULONG refcount;
    HRESULT hr;

    if (!init_test_context(&ctx, d3d11))
        return;

    if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &IID_ID2D1Factory1, NULL, (void **)&factory)))
    {
        win_skip("ID2D1Factory1 is not supported.\n");
        release_test_context(&ctx);
        return;
    }

    device_context = create_device_context(factory, ctx.device, d3d11);
    ok(device_context != NULL, "Failed to create device context.\n");

    hr = ID2D1DeviceContext_CreateCommandList(device_context, &command_list);
todo_wine
    ok(SUCCEEDED(hr), "Failed to create command list, hr %#x.\n", hr);

    if (FAILED(hr))
    {
        ID2D1DeviceContext_Release(device_context);
        ID2D1Factory1_Release(factory);
        return;
    }

    ID2D1DeviceContext_SetTarget(device_context, (ID2D1Image *)command_list);
    ID2D1DeviceContext_BeginDraw(device_context);

    hr = ID2D1DeviceContext_QueryInterface(device_context, &IID_ID2D1RenderTarget, (void **)&rt);
    ok(SUCCEEDED(hr), "Failed to get rt interface, hr %#x.\n", hr);

    /* Test how resources are referenced by the list. */

    /* Bitmap. */
    set_size_u(&size, 4, 1);
    bitmap_desc.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    bitmap_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;
    bitmap_desc.dpiX = 96.0f;
    bitmap_desc.dpiY = 96.0f;
    hr = ID2D1RenderTarget_CreateBitmap(rt, size, bitmap_data, 4 * sizeof(*bitmap_data), &bitmap_desc, &bitmap);
    ok(SUCCEEDED(hr), "Failed to create bitmap, hr %#x.\n", hr);

    ID2D1RenderTarget_DrawBitmap(rt, bitmap, NULL, 0.25f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, NULL);

    refcount = ID2D1Bitmap_Release(bitmap);
    ok(refcount == 1, "Got unexpected refcount %u.\n", refcount);

    /* Solid color brush. */
    set_color(&color, 0.0f, 0.0f, 0.0f, 0.0f);
    hr = ID2D1RenderTarget_CreateSolidColorBrush(rt, &color, NULL, (ID2D1SolidColorBrush **)&brush);
    ok(SUCCEEDED(hr), "Failed to create a brush, hr %#x.\n", hr);

    set_rect(&rect, 0.0f, 0.0f, 16.0f, 16.0f);
    ID2D1RenderTarget_FillRectangle(rt, &rect, brush);

    refcount = ID2D1Brush_Release(brush);
    ok(refcount == 0, "Got unexpected refcount %u.\n", refcount);

    /* Bitmap brush. */
    hr = ID2D1RenderTarget_CreateBitmap(rt, size, bitmap_data, 4 * sizeof(*bitmap_data), &bitmap_desc, &bitmap);
    ok(SUCCEEDED(hr), "Failed to create bitmap, hr %#x.\n", hr);

    hr = ID2D1RenderTarget_CreateBitmapBrush(rt, bitmap, NULL, NULL, (ID2D1BitmapBrush **)&brush);
    ok(SUCCEEDED(hr), "Failed to create bitmap brush, hr %#x.\n", hr);

    set_rect(&rect, 0.0f, 0.0f, 16.0f, 16.0f);
    ID2D1RenderTarget_FillRectangle(rt, &rect, brush);

    refcount = ID2D1Brush_Release(brush);
    ok(refcount == 0, "Got unexpected refcount %u.\n", refcount);

    refcount = ID2D1Bitmap_Release(bitmap);
    ok(refcount == 1, "Got unexpected refcount %u.\n", refcount);

    /* Linear gradient brush. */
    hr = ID2D1RenderTarget_CreateGradientStopCollection(rt, stops, ARRAY_SIZE(stops),
            D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, &gradient);
    ok(SUCCEEDED(hr), "Failed to create stop collection, hr %#x.\n", hr);

    set_point(&linear_gradient_properties.startPoint, 320.0f, 0.0f);
    set_point(&linear_gradient_properties.endPoint, 0.0f, 960.0f);
    hr = ID2D1RenderTarget_CreateLinearGradientBrush(rt, &linear_gradient_properties, NULL, gradient,
            (ID2D1LinearGradientBrush **)&brush);
    ok(SUCCEEDED(hr), "Failed to create brush, hr %#x.\n", hr);

    set_rect(&rect, -1.0f, -1.0f, 1.0f, 1.0f);
    hr = ID2D1Factory1_CreateRectangleGeometry(factory, &rect, (ID2D1RectangleGeometry **)&geometry);
    ok(SUCCEEDED(hr), "Failed to create geometry, hr %#x.\n", hr);

    ID2D1RenderTarget_FillGeometry(rt, geometry, brush, NULL);

    refcount = ID2D1Brush_Release(brush);
    ok(refcount == 0, "Got unexpected refcount %u.\n", refcount);

    refcount = ID2D1Geometry_Release(geometry);
    ok(refcount == 1, "Got unexpected refcount %u.\n", refcount);

    refcount = ID2D1GradientStopCollection_Release(gradient);
    ok(refcount == 1, "Got unexpected refcount %u.\n", refcount);

    /* Radial gradient brush. */
    hr = ID2D1RenderTarget_CreateGradientStopCollection(rt, stops, ARRAY_SIZE(stops),
            D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, &gradient);
    ok(SUCCEEDED(hr), "Failed to create stop collection, hr %#x.\n", hr);

    set_point(&radial_gradient_properties.center, 160.0f, 480.0f);
    set_point(&radial_gradient_properties.gradientOriginOffset, 40.0f, -120.0f);
    radial_gradient_properties.radiusX = 160.0f;
    radial_gradient_properties.radiusY = 480.0f;
    hr = ID2D1RenderTarget_CreateRadialGradientBrush(rt, &radial_gradient_properties, NULL, gradient,
            (ID2D1RadialGradientBrush **)&brush);
    ok(SUCCEEDED(hr), "Failed to create brush, hr %#x.\n", hr);

    set_rect(&rect, -1.0f, -1.0f, 1.0f, 1.0f);
    hr = ID2D1Factory1_CreateRectangleGeometry(factory, &rect, (ID2D1RectangleGeometry **)&geometry);
    ok(SUCCEEDED(hr), "Failed to create geometry, hr %#x.\n", hr);

    ID2D1RenderTarget_FillGeometry(rt, geometry, brush, NULL);

    refcount = ID2D1Brush_Release(brush);
    ok(refcount == 0, "Got unexpected refcount %u.\n", refcount);

    refcount = ID2D1Geometry_Release(geometry);
    ok(refcount == 1, "Got unexpected refcount %u.\n", refcount);

    refcount = ID2D1GradientStopCollection_Release(gradient);
    ok(refcount == 1, "Got unexpected refcount %u.\n", refcount);

    /* Geometry. */
    set_rect(&rect, -1.0f, -1.0f, 1.0f, 1.0f);
    hr = ID2D1Factory1_CreateRectangleGeometry(factory, &rect, (ID2D1RectangleGeometry **)&geometry);
    ok(SUCCEEDED(hr), "Failed to create geometry, hr %#x.\n", hr);

    set_color(&color, 0.0f, 0.0f, 0.0f, 0.0f);
    hr = ID2D1RenderTarget_CreateSolidColorBrush(rt, &color, NULL, (ID2D1SolidColorBrush **)&brush);
    ok(SUCCEEDED(hr), "Failed to create a brush, hr %#x.\n", hr);

    ID2D1RenderTarget_FillGeometry(rt, geometry, brush, NULL);

    refcount = ID2D1Brush_Release(brush);
    ok(refcount == 0, "Got unexpected refcount %u.\n", refcount);

    refcount = ID2D1Geometry_Release(geometry);
    ok(refcount == 1, "Got unexpected refcount %u.\n", refcount);

    /* Stroke style. */
    stroke_desc.startCap = D2D1_CAP_STYLE_SQUARE;
    stroke_desc.endCap = D2D1_CAP_STYLE_ROUND;
    stroke_desc.dashCap = D2D1_CAP_STYLE_TRIANGLE;
    stroke_desc.lineJoin = D2D1_LINE_JOIN_BEVEL;
    stroke_desc.miterLimit = 1.5f;
    stroke_desc.dashStyle = D2D1_DASH_STYLE_DOT;
    stroke_desc.dashOffset = -1.0f;

    hr = ID2D1Factory_CreateStrokeStyle((ID2D1Factory *)factory, &stroke_desc, NULL, 0, &stroke_style);
    ok(SUCCEEDED(hr), "Failed to create stroke style, %#x.\n", hr);

    set_color(&color, 0.0f, 0.0f, 0.0f, 0.0f);
    hr = ID2D1RenderTarget_CreateSolidColorBrush(rt, &color, NULL, (ID2D1SolidColorBrush **)&brush);
    ok(SUCCEEDED(hr), "Failed to create a brush, hr %#x.\n", hr);

    set_point(&p0, 100.0f, 160.0f);
    set_point(&p1, 140.0f, 160.0f);
    ID2D1RenderTarget_DrawLine(rt, p0, p1, brush, 1.0f, stroke_style);

    refcount = ID2D1Brush_Release(brush);
    ok(refcount == 0, "Got unexpected refcount %u.\n", refcount);

    refcount = ID2D1StrokeStyle_Release(stroke_style);
    ok(refcount == 1, "Got unexpected refcount %u.\n", refcount);

    /* Close on attached list. */
    ID2D1DeviceContext_GetTarget(device_context, &target);
    ok(target == (ID2D1Image *)command_list, "Unexpected context target.\n");
    ID2D1Image_Release(target);

    hr = ID2D1CommandList_Close(command_list);
    ok(SUCCEEDED(hr), "Failed to close a list, hr %#x.\n", hr);

    ID2D1DeviceContext_GetTarget(device_context, &target);
    ok(target == NULL, "Unexpected context target.\n");

    hr = ID2D1CommandList_Close(command_list);
    ok(hr == D2DERR_WRONG_STATE, "Unexpected hr %#x.\n", hr);

    ID2D1CommandList_Release(command_list);

    /* Close empty list. */
    hr = ID2D1DeviceContext_CreateCommandList(device_context, &command_list);
    ok(SUCCEEDED(hr), "Failed to create command list, hr %#x.\n", hr);

    hr = ID2D1CommandList_Close(command_list);
    ok(SUCCEEDED(hr), "Failed to close a list, hr %#x.\n", hr);

    ID2D1CommandList_Release(command_list);

    /* List created with different context. */
    device_context2 = create_device_context(factory, ctx.device, d3d11);
    ok(device_context2 != NULL, "Failed to create device context.\n");

    hr = ID2D1DeviceContext_CreateCommandList(device_context, &command_list);
    ok(SUCCEEDED(hr), "Failed to create command list, hr %#x.\n", hr);

    ID2D1DeviceContext_SetTarget(device_context2, (ID2D1Image *)command_list);
    ID2D1DeviceContext_GetTarget(device_context2, &target);
    ok(target == NULL, "Unexpected target.\n");

    ID2D1CommandList_Release(command_list);
    ID2D1DeviceContext_Release(device_context2);

    ID2D1RenderTarget_Release(rt);
    ID2D1DeviceContext_Release(device_context);
    ID2D1Factory1_Release(factory);
    release_test_context(&ctx);
}

static void test_max_bitmap_size(BOOL d3d11)
{
    D2D1_RENDER_TARGET_PROPERTIES desc;
    D2D1_BITMAP_PROPERTIES bitmap_desc;
    IDXGISwapChain *swapchain;
    ID2D1Factory *factory;
    IDXGISurface *surface;
    ID2D1RenderTarget *rt;
    ID3D10Device1 *device;
    ID2D1Bitmap *bitmap;
    UINT32 bitmap_size;
    unsigned int i, j;
    HWND window;
    HRESULT hr;

    static const struct
    {
        const char *name;
        DWORD type;
    }
    device_types[] =
    {
        { "HW",   D3D10_DRIVER_TYPE_HARDWARE },
        { "WARP", D3D10_DRIVER_TYPE_WARP },
        { "REF",  D3D10_DRIVER_TYPE_REFERENCE },
    };
    static const struct
    {
        const char *name;
        DWORD type;
    }
    target_types[] =
    {
        { "DEFAULT", D2D1_RENDER_TARGET_TYPE_DEFAULT },
        { "HW",      D2D1_RENDER_TARGET_TYPE_HARDWARE },
    };

    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &IID_ID2D1Factory, NULL, (void **)&factory);
    ok(SUCCEEDED(hr), "Failed to create factory, hr %#x.\n", hr);

    for (i = 0; i < ARRAY_SIZE(device_types); ++i)
    {
        if (FAILED(hr = D3D10CreateDevice1(NULL, device_types[i].type, NULL, D3D10_CREATE_DEVICE_BGRA_SUPPORT,
                D3D10_FEATURE_LEVEL_10_0, D3D10_1_SDK_VERSION, &device)))
        {
            skip("Failed to create %s d3d device, hr %#x.\n", device_types[i].name, hr);
            continue;
        }

        window = create_window();
        swapchain = create_d3d10_swapchain(device, window, TRUE);
        hr = IDXGISwapChain_GetBuffer(swapchain, 0, &IID_IDXGISurface, (void **)&surface);
        ok(SUCCEEDED(hr), "Failed to get buffer, hr %#x.\n", hr);

        for (j = 0; j < ARRAY_SIZE(target_types); ++j)
        {
            D3D10_TEXTURE2D_DESC texture_desc;
            ID3D10Texture2D *texture;
            D2D1_SIZE_U size;

            desc.type = target_types[j].type;
            desc.pixelFormat.format = DXGI_FORMAT_UNKNOWN;
            desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
            desc.dpiX = 0.0f;
            desc.dpiY = 0.0f;
            desc.usage = D2D1_RENDER_TARGET_USAGE_NONE;
            desc.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;

            hr = ID2D1Factory_CreateDxgiSurfaceRenderTarget(factory, surface, &desc, &rt);
            ok(SUCCEEDED(hr), "%s/%s: failed to create render target, hr %#x.\n", device_types[i].name,
                    target_types[j].name, hr);

            bitmap_size = ID2D1RenderTarget_GetMaximumBitmapSize(rt);
            ok(bitmap_size >= D3D10_REQ_TEXTURE2D_U_OR_V_DIMENSION, "%s/%s: unexpected bitmap size %u.\n",
                    device_types[i].name, target_types[j].name, bitmap_size);

            bitmap_desc.dpiX = 96.0f;
            bitmap_desc.dpiY = 96.0f;
            bitmap_desc.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
            bitmap_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;

            size.width = bitmap_size;
            size.height = 1;
            hr = ID2D1RenderTarget_CreateBitmap(rt, size, NULL, 0, &bitmap_desc, &bitmap);
            ok(SUCCEEDED(hr), "Failed to create a bitmap, hr %#x.\n", hr);
            ID2D1Bitmap_Release(bitmap);

            ID2D1RenderTarget_Release(rt);

            texture_desc.Width = bitmap_size;
            texture_desc.Height = 1;
            texture_desc.MipLevels = 1;
            texture_desc.ArraySize = 1;
            texture_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            texture_desc.SampleDesc.Count = 1;
            texture_desc.SampleDesc.Quality = 0;
            texture_desc.Usage = D3D10_USAGE_DEFAULT;
            texture_desc.BindFlags = 0;
            texture_desc.CPUAccessFlags = 0;
            texture_desc.MiscFlags = 0;

            hr = ID3D10Device1_CreateTexture2D(device, &texture_desc, NULL, &texture);
            ok(SUCCEEDED(hr) || broken(hr == E_INVALIDARG && device_types[i].type == D3D10_DRIVER_TYPE_WARP) /* Vista */,
                    "%s/%s: failed to create texture, hr %#x.\n", device_types[i].name, target_types[j].name, hr);
            if (SUCCEEDED(hr))
                ID3D10Texture2D_Release(texture);
        }

        IDXGISurface_Release(surface);
        IDXGISwapChain_Release(swapchain);
        DestroyWindow(window);

        ID3D10Device1_Release(device);
    }

    ID2D1Factory_Release(factory);
}

static void test_dpi(BOOL d3d11)
{
    D2D1_BITMAP_PROPERTIES1 bitmap_desc;
    ID2D1DeviceContext *device_context;
    IWICImagingFactory *wic_factory;
    struct d2d1_test_context ctx;
    ID2D1Factory1 *factory;
    ID2D1Bitmap1 *bitmap;
    float dpi_x, dpi_y;
    HRESULT hr;

    static const struct
    {
        float dpi_x, dpi_y;
        HRESULT hr;
    }
    create_dpi_tests[] =
    {
        {  0.0f,   0.0f, S_OK},
        {192.0f,   0.0f, E_INVALIDARG},
        {  0.0f, 192.0f, E_INVALIDARG},
        {192.0f, -10.0f, E_INVALIDARG},
        {-10.0f, 192.0f, E_INVALIDARG},
        {-10.0f, -10.0f, E_INVALIDARG},
        { 48.0f,  96.0f, S_OK},
        { 96.0f,  48.0f, S_OK},
    };
    static const float init_dpi_x = 60.0f, init_dpi_y = 288.0f;
    static const float dc_dpi_x = 120.0f, dc_dpi_y = 144.0f;
    unsigned int i;

    if (!init_test_context(&ctx, d3d11))
        return;

    if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &IID_ID2D1Factory1, NULL, (void **)&factory)))
    {
        win_skip("ID2D1Factory1 is not supported.\n");
        release_test_context(&ctx);
        return;
    }


    device_context = create_device_context(factory, ctx.device, d3d11);
    ok(!!device_context, "Failed to create device context.\n");

    ID2D1DeviceContext_GetDpi(device_context, &dpi_x, &dpi_y);
    ok(dpi_x == 96.0f, "Got unexpected dpi_x %.8e.\n", dpi_x);
    ok(dpi_y == 96.0f, "Got unexpected dpi_y %.8e.\n", dpi_y);

    /* DXGI surface */
    for (i = 0; i < ARRAY_SIZE(create_dpi_tests); ++i)
    {
        ID2D1DeviceContext_SetDpi(device_context, init_dpi_x, init_dpi_y);

        bitmap_desc.pixelFormat.format = DXGI_FORMAT_UNKNOWN;
        bitmap_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
        bitmap_desc.dpiX = create_dpi_tests[i].dpi_x;
        bitmap_desc.dpiY = create_dpi_tests[i].dpi_y;
        bitmap_desc.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;
        bitmap_desc.colorContext = NULL;
        hr = ID2D1DeviceContext_CreateBitmapFromDxgiSurface(device_context, ctx.surface, &bitmap_desc, &bitmap);
        /* Native accepts negative DPI values for DXGI surface bitmap. */
        ok(hr == S_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);

        ID2D1DeviceContext_SetDpi(device_context, dc_dpi_x, dc_dpi_y);

        ID2D1Bitmap1_GetDpi(bitmap, &dpi_x, &dpi_y);
        todo_wine_if(bitmap_desc.dpiX == 0.0f)
            ok(dpi_x == bitmap_desc.dpiX, "Test %u: Got unexpected dpi_x %.8e, expected %.8e.\n",
                    i, dpi_x, bitmap_desc.dpiX);
        todo_wine_if(bitmap_desc.dpiY == 0.0f)
            ok(dpi_y == bitmap_desc.dpiY, "Test %u: Got unexpected dpi_y %.8e, expected %.8e.\n",
                    i, dpi_y, bitmap_desc.dpiY);

        ID2D1DeviceContext_BeginDraw(device_context);
        ID2D1DeviceContext_SetTarget(device_context, (ID2D1Image *)bitmap);
        hr = ID2D1DeviceContext_EndDraw(device_context, NULL, NULL);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);

        /* Device context DPI values aren't updated by SetTarget. */
        ID2D1DeviceContext_GetDpi(device_context, &dpi_x, &dpi_y);
        ok(dpi_x == dc_dpi_x, "Test %u: Got unexpected dpi_x %.8e, expected %.8e.\n", i, dpi_x, dc_dpi_x);
        ok(dpi_y == dc_dpi_y, "Test %u: Got unexpected dpi_y %.8e, expected %.8e.\n", i, dpi_y, dc_dpi_y);

        ID2D1Bitmap1_Release(bitmap);
    }

    /* WIC bitmap */
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
            &IID_IWICImagingFactory, (void **)&wic_factory);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    for (i = 0; i < ARRAY_SIZE(create_dpi_tests); ++i)
    {
        IWICBitmapSource *wic_bitmap_src;
        IWICBitmap *wic_bitmap;

        ID2D1DeviceContext_SetDpi(device_context, init_dpi_x, init_dpi_y);

        hr = IWICImagingFactory_CreateBitmap(wic_factory, 16, 16,
                &GUID_WICPixelFormat32bppPBGRA, WICBitmapCacheOnDemand, &wic_bitmap);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);
        hr = IWICBitmap_QueryInterface(wic_bitmap, &IID_IWICBitmapSource, (void **)&wic_bitmap_src);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);
        IWICBitmap_Release(wic_bitmap);

        bitmap_desc.pixelFormat.format = DXGI_FORMAT_UNKNOWN;
        bitmap_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
        bitmap_desc.dpiX = create_dpi_tests[i].dpi_x;
        bitmap_desc.dpiY = create_dpi_tests[i].dpi_y;
        bitmap_desc.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;
        bitmap_desc.colorContext = NULL;
        hr = ID2D1DeviceContext_CreateBitmapFromWicBitmap(device_context, wic_bitmap_src, &bitmap_desc, &bitmap);
        ok(hr == create_dpi_tests[i].hr, "Test %u: Got unexpected hr %#x, expected %#x.\n",
                i, hr, create_dpi_tests[i].hr);
        IWICBitmapSource_Release(wic_bitmap_src);

        if (FAILED(hr))
            continue;

        ID2D1DeviceContext_SetDpi(device_context, dc_dpi_x, dc_dpi_y);

        ID2D1Bitmap1_GetDpi(bitmap, &dpi_x, &dpi_y);
        if (bitmap_desc.dpiX == 0.0f && bitmap_desc.dpiY == 0.0f)
        {
            /* Bitmap DPI values are inherited at creation time. */
            ok(dpi_x == init_dpi_x, "Test %u: Got unexpected dpi_x %.8e, expected %.8e.\n", i, dpi_x, init_dpi_x);
            ok(dpi_y == init_dpi_y, "Test %u: Got unexpected dpi_y %.8e, expected %.8e.\n", i, dpi_y, init_dpi_y);
        }
        else
        {
            ok(dpi_x == bitmap_desc.dpiX, "Test %u: Got unexpected dpi_x %.8e, expected %.8e.\n",
                    i, dpi_x, bitmap_desc.dpiX);
            ok(dpi_y == bitmap_desc.dpiY, "Test %u: Got unexpected dpi_y %.8e, expected %.8e.\n",
                    i, dpi_y, bitmap_desc.dpiY);
        }

        ID2D1DeviceContext_BeginDraw(device_context);
        ID2D1DeviceContext_SetTarget(device_context, (ID2D1Image *)bitmap);
        hr = ID2D1DeviceContext_EndDraw(device_context, NULL, NULL);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);

        /* Device context DPI values aren't updated by SetTarget. */
        ID2D1DeviceContext_GetDpi(device_context, &dpi_x, &dpi_y);
        ok(dpi_x == dc_dpi_x, "Test %u: Got unexpected dpi_x %.8e, expected %.8e.\n", i, dpi_x, dc_dpi_x);
        ok(dpi_y == dc_dpi_y, "Test %u: Got unexpected dpi_y %.8e, expected %.8e.\n", i, dpi_y, dc_dpi_y);

        ID2D1Bitmap1_Release(bitmap);
    }
    IWICImagingFactory_Release(wic_factory);
    CoUninitialize();

    /* D2D bitmap */
    for (i = 0; i < ARRAY_SIZE(create_dpi_tests); ++i)
    {
        const D2D1_SIZE_U size = {16, 16};

        ID2D1DeviceContext_SetDpi(device_context, init_dpi_x, init_dpi_y);

        bitmap_desc.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
        bitmap_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
        bitmap_desc.dpiX = create_dpi_tests[i].dpi_x;
        bitmap_desc.dpiY = create_dpi_tests[i].dpi_y;
        bitmap_desc.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;
        bitmap_desc.colorContext = NULL;
        hr = ID2D1DeviceContext_CreateBitmap(device_context, size, NULL, 0, &bitmap_desc, &bitmap);
        ok(hr == create_dpi_tests[i].hr, "Test %u: Got unexpected hr %#x, expected %#x.\n",
                i, hr, create_dpi_tests[i].hr);

        if (FAILED(hr))
            continue;

        ID2D1DeviceContext_SetDpi(device_context, dc_dpi_x, dc_dpi_y);

        ID2D1Bitmap1_GetDpi(bitmap, &dpi_x, &dpi_y);
        if (bitmap_desc.dpiX == 0.0f && bitmap_desc.dpiY == 0.0f)
        {
            /* Bitmap DPI values are inherited at creation time. */
            ok(dpi_x == init_dpi_x, "Test %u: Got unexpected dpi_x %.8e, expected %.8e.\n", i, dpi_x, init_dpi_x);
            ok(dpi_y == init_dpi_y, "Test %u: Got unexpected dpi_y %.8e, expected %.8e.\n", i, dpi_y, init_dpi_y);
        }
        else
        {
            ok(dpi_x == bitmap_desc.dpiX, "Test %u: Got unexpected dpi_x %.8e, expected %.8e.\n",
                    i, dpi_x, bitmap_desc.dpiX);
            ok(dpi_y == bitmap_desc.dpiY, "Test %u: Got unexpected dpi_y %.8e, expected %.8e.\n",
                    i, dpi_y, bitmap_desc.dpiY);
        }

        ID2D1DeviceContext_BeginDraw(device_context);
        ID2D1DeviceContext_SetTarget(device_context, (ID2D1Image *)bitmap);
        hr = ID2D1DeviceContext_EndDraw(device_context, NULL, NULL);
        ok(hr == S_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);

        /* Device context DPI values aren't updated by SetTarget. */
        ID2D1DeviceContext_GetDpi(device_context, &dpi_x, &dpi_y);
        ok(dpi_x == dc_dpi_x, "Test %u: Got unexpected dpi_x %.8e, expected %.8e.\n", i, dpi_x, dc_dpi_x);
        ok(dpi_y == dc_dpi_y, "Test %u: Got unexpected dpi_y %.8e, expected %.8e.\n", i, dpi_y, dc_dpi_y);

        ID2D1Bitmap1_Release(bitmap);
    }

    ID2D1DeviceContext_SetTarget(device_context, NULL);
    ID2D1DeviceContext_GetDpi(device_context, &dpi_x, &dpi_y);
    ok(dpi_x == dc_dpi_x, "Got unexpected dpi_x %.8e, expected %.8e.\n", dpi_x, dc_dpi_x);
    ok(dpi_y == dc_dpi_y, "Got unexpected dpi_y %.8e, expected %.8e.\n", dpi_y, dc_dpi_y);

    ID2D1DeviceContext_Release(device_context);
    ID2D1Factory1_Release(factory);
    release_test_context(&ctx);
}

static void test_wic_bitmap_format(BOOL d3d11)
{
    IWICImagingFactory *wic_factory;
    struct d2d1_test_context ctx;
    D2D1_PIXEL_FORMAT format;
    IWICBitmap *wic_bitmap;
    ID2D1RenderTarget *rt;
    ID2D1Bitmap *bitmap;
    unsigned int i;
    HRESULT hr;

    static const struct
    {
        const WICPixelFormatGUID *wic;
        D2D1_PIXEL_FORMAT d2d;
    }
    tests[] =
    {
        {&GUID_WICPixelFormat32bppPBGRA, {DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED}},
        {&GUID_WICPixelFormat32bppPRGBA, {DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED}},
        {&GUID_WICPixelFormat32bppBGR,   {DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE}},
    };

    if (!init_test_context(&ctx, d3d11))
        return;

    rt = ctx.rt;
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
            &IID_IWICImagingFactory, (void **)&wic_factory);
    ok(hr == S_OK, "Failed to create WIC imaging factory, hr %#x.\n", hr);

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        hr = IWICImagingFactory_CreateBitmap(wic_factory, 16, 16,
                tests[i].wic, WICBitmapCacheOnDemand, &wic_bitmap);
        ok(hr == S_OK, "%s: Failed to create WIC bitmap, hr %#x.\n", debugstr_guid(tests[i].wic), hr);

        hr = ID2D1RenderTarget_CreateBitmapFromWicBitmap(rt, (IWICBitmapSource *)wic_bitmap, NULL, &bitmap);
        ok(hr == S_OK, "%s: Failed to create bitmap from WIC source, hr %#x.\n", debugstr_guid(tests[i].wic), hr);

        format = ID2D1Bitmap_GetPixelFormat(bitmap);
        ok(format.format == tests[i].d2d.format, "%s: Got unexpected DXGI format %#x.\n",
                debugstr_guid(tests[i].wic), format.format);
        ok(format.alphaMode == tests[i].d2d.alphaMode, "%s: Got unexpected alpha mode %#x.\n",
                debugstr_guid(tests[i].wic), format.alphaMode);

        ID2D1Bitmap_Release(bitmap);
        IWICBitmap_Release(wic_bitmap);
    }

    IWICImagingFactory_Release(wic_factory);
    CoUninitialize();
    release_test_context(&ctx);
}

static void test_math(BOOL d3d11)
{
    float s, c, t, l;
    unsigned int i;

    static const struct
    {
        float x;
        float s;
        float c;
    }
    sc_data[] =
    {
        {0.0f,         0.0f,              1.0f},
        {1.0f,         8.41470957e-001f,  5.40302277e-001f},
        {2.0f,         9.09297407e-001f, -4.16146845e-001f},
        {M_PI / 2.0f,  1.0f,             -4.37113883e-008f},
        {M_PI,        -8.74227766e-008f, -1.0f},
    };

    static const struct
    {
        float x;
        float t;
    }
    t_data[] =
    {
        {0.0f,         0.0f},
        {1.0f,         1.55740774f},
        {2.0f,        -2.18503976f},
        {M_PI / 2.0f, -2.28773320e+007f},
        {M_PI,         8.74227766e-008f},
    };

    static const struct
    {
        float x;
        float y;
        float z;
        float l;
    }
    l_data[] =
    {
        {0.0f, 0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f, 1.0f},
        {0.0f, 1.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f, 1.0f},
        {1.0f, 1.0f, 1.0f, 1.73205078f},
        {1.0f, 2.0f, 2.0f, 3.0f},
        {1.0f, 2.0f, 3.0f, 3.74165750f},
    };

    if (!pD2D1SinCos || !pD2D1Tan || !pD2D1Vec3Length)
    {
        win_skip("D2D1SinCos/D2D1Tan/D2D1Vec3Length not available, skipping test.\n");
        return;
    }

    for (i = 0; i < ARRAY_SIZE(sc_data); ++i)
    {
        pD2D1SinCos(sc_data[i].x, &s, &c);
        ok(compare_float(s, sc_data[i].s, 0),
                "Test %u: Got unexpected sin %.8e, expected %.8e.\n", i, s, sc_data[i].s);
        ok(compare_float(c, sc_data[i].c, 0),
                "Test %u: Got unexpected cos %.8e, expected %.8e.\n", i, c, sc_data[i].c);
    }

    for (i = 0; i < ARRAY_SIZE(t_data); ++i)
    {
        t = pD2D1Tan(t_data[i].x);
        ok(compare_float(t, t_data[i].t, 1),
                "Test %u: Got unexpected tan %.8e, expected %.8e.\n", i, t, t_data[i].t);
    }

    for (i = 0; i < ARRAY_SIZE(l_data); ++i)
    {
        l = pD2D1Vec3Length(l_data[i].x, l_data[i].y, l_data[i].z);
        ok(compare_float(l, l_data[i].l, 0),
                "Test %u: Got unexpected length %.8e, expected %.8e.\n", i, l, l_data[i].l);
    }
}

static void test_colour_space(BOOL d3d11)
{
    D2D1_COLOR_F src_colour, dst_colour, expected;
    D2D1_COLOR_SPACE src_space, dst_space;
    unsigned i, j, k;

    static const D2D1_COLOR_SPACE colour_spaces[] =
    {
        D2D1_COLOR_SPACE_CUSTOM,
        D2D1_COLOR_SPACE_SRGB,
        D2D1_COLOR_SPACE_SCRGB,
    };
    static struct
    {
        D2D1_COLOR_F srgb;
        D2D1_COLOR_F scrgb;
    }
    const test_data[] =
    {
        {{0.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 0.0f}},
        {{0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f, 1.0f}},
        {{1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
        /* Samples in the non-linear region. */
        {{0.2f, 0.4f, 0.6f, 0.8f}, {0.0331047624f, 0.132868335f, 0.318546832f, 0.8f}},
        {{0.3f, 0.5f, 0.7f, 0.9f}, {0.0732389688f, 0.214041144f, 0.447988421f, 0.9f}},
        /* Samples in the linear region. */
        {{0.0002f, 0.0004f, 0.0006f, 0.0008f}, {1.54798763e-005f, 3.09597526e-005f, 4.64396289e-005f, 0.0008f}},
        {{0.0003f, 0.0005f, 0.0007f, 0.0009f}, {2.32198145e-005f, 3.86996908e-005f, 5.41795634e-005f, 0.0009f}},
        /* Out of range samples */
        {{-0.3f,  1.5f, -0.7f,  2.0f}, { 0.0f,  1.0f,  0.0f,  2.0f}},
        {{ 1.5f, -0.3f,  2.0f, -0.7f}, { 1.0f,  0.0f,  1.0f, -0.7f}},
        {{ 0.0f,  1.0f,  0.0f,  1.5f}, {-0.7f,  2.0f, -0.3f,  1.5f}},
        {{ 1.0f,  0.0f,  1.0f, -0.3f}, { 2.0f, -0.7f,  1.5f, -0.3f}},
    };

    if (!pD2D1ConvertColorSpace)
    {
        win_skip("D2D1ConvertColorSpace() not available, skipping test.\n");
        return;
    }

    for (i = 0; i < ARRAY_SIZE(colour_spaces); ++i)
    {
        src_space = colour_spaces[i];
        for (j = 0; j < ARRAY_SIZE(colour_spaces); ++j)
        {
            dst_space = colour_spaces[j];
            for (k = 0; k < ARRAY_SIZE(test_data); ++k)
            {
                if (src_space == D2D1_COLOR_SPACE_SCRGB)
                    src_colour = test_data[k].scrgb;
                else
                    src_colour = test_data[k].srgb;

                if (dst_space == D2D1_COLOR_SPACE_SCRGB)
                    expected = test_data[k].scrgb;
                else
                    expected = test_data[k].srgb;

                if (src_space == D2D1_COLOR_SPACE_CUSTOM || dst_space == D2D1_COLOR_SPACE_CUSTOM)
                {
                    set_color(&expected, 0.0f, 0.0f, 0.0f, 0.0f);
                }
                else if (src_space != dst_space)
                {
                    expected.r = clamp_float(expected.r, 0.0f, 1.0f);
                    expected.g = clamp_float(expected.g, 0.0f, 1.0f);
                    expected.b = clamp_float(expected.b, 0.0f, 1.0f);
                }

                dst_colour = pD2D1ConvertColorSpace(src_space, dst_space, &src_colour);
                ok(compare_colour_f(&dst_colour, expected.r, expected.g, expected.b, expected.a, 1),
                        "Got unexpected destination colour {%.8e, %.8e, %.8e, %.8e}, "
                        "expected destination colour {%.8e, %.8e, %.8e, %.8e} for "
                        "source colour {%.8e, %.8e, %.8e, %.8e}, "
                        "source colour space %#x, destination colour space %#x.\n",
                        dst_colour.r, dst_colour.g, dst_colour.b, dst_colour.a,
                        expected.r, expected.g, expected.b, expected.a,
                        src_colour.r, src_colour.g, src_colour.b, src_colour.a,
                        src_space, dst_space);
            }
        }
    }
}

static void test_geometry_group(BOOL d3d11)
{
    ID2D1Factory *factory;
    ID2D1GeometryGroup *group;
    ID2D1Geometry *geometries[2];
    D2D1_RECT_F rect;
    HRESULT hr;
    D2D1_MATRIX_3X2_F matrix;
    BOOL match;

    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &IID_ID2D1Factory, NULL, (void **)&factory);
    ok(SUCCEEDED(hr), "Failed to create factory, hr %#x.\n", hr);

    set_rect(&rect, -1.0f, -1.0f, 1.0f, 1.0f);
    hr = ID2D1Factory_CreateRectangleGeometry(factory, &rect, (ID2D1RectangleGeometry **)&geometries[0]);
    ok(SUCCEEDED(hr), "Failed to create geometry, hr %#x.\n", hr);

    set_rect(&rect, -2.0f, -2.0f, 0.0f, 2.0f);
    hr = ID2D1Factory_CreateRectangleGeometry(factory, &rect, (ID2D1RectangleGeometry **)&geometries[1]);
    ok(SUCCEEDED(hr), "Failed to create geometry, hr %#x.\n", hr);

    hr = ID2D1Factory_CreateGeometryGroup(factory, D2D1_FILL_MODE_ALTERNATE, geometries, 2, &group);
    ok(SUCCEEDED(hr), "Failed to create geometry group, hr %#x.\n", hr);

    set_rect(&rect, 0.0f, 0.0f, 0.0f, 0.0f);
    hr = ID2D1GeometryGroup_GetBounds(group, NULL, &rect);
    ok(SUCCEEDED(hr), "Failed to get geometry group bounds, hr %#x.\n", hr);
    match = compare_rect(&rect, -2.0f, -2.0f, 1.0f, 2.0f, 0);
    ok(match, "Got unexpected rectangle {%.8e, %.8e, %.8e, %.8e}.\n",
            rect.left, rect.top, rect.right, rect.bottom);

    set_matrix_identity(&matrix);
    translate_matrix(&matrix, 80.0f, 640.0f);
    scale_matrix(&matrix, 2.0f, 0.5f);
    hr = ID2D1GeometryGroup_GetBounds(group, &matrix, &rect);
    ok(SUCCEEDED(hr), "Failed to get geometry group bounds, hr %#x.\n", hr);
    match = compare_rect(&rect, 76.0f, 639.0f, 82.0f, 641.0f, 0);
    ok(match, "Got unexpected rectangle {%.8e, %.8e, %.8e, %.8e}.\n",
            rect.left, rect.top, rect.right, rect.bottom);

    ID2D1GeometryGroup_Release(group);

    ID2D1Geometry_Release(geometries[0]);
    ID2D1Geometry_Release(geometries[1]);

    ID2D1Factory_Release(factory);
}

static DWORD WINAPI mt_factory_test_thread_func(void *param)
{
    ID2D1Multithread *multithread = param;

    ID2D1Multithread_Enter(multithread);

    return 0;
}

static void test_mt_factory(BOOL d3d11)
{
    ID2D1Multithread *multithread;
    ID2D1Factory *factory;
    HANDLE thread;
    HRESULT hr;
    DWORD ret;

    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED + 1, &IID_ID2D1Factory, NULL, (void **)&factory);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &IID_ID2D1Factory, NULL, (void **)&factory);
    ok(SUCCEEDED(hr), "Failed to create factory, hr %#x.\n", hr);

    hr = ID2D1Factory_QueryInterface(factory, &IID_ID2D1Multithread, (void **)&multithread);
    if (hr == E_NOINTERFACE)
    {
        win_skip("ID2D1Multithread is not supported.\n");
        ID2D1Factory_Release(factory);
        return;
    }
    ok(SUCCEEDED(hr), "Failed to get interface, hr %#x.\n", hr);

    ret = ID2D1Multithread_GetMultithreadProtected(multithread);
    ok(!ret, "Unexpected return value.\n");

    ID2D1Multithread_Enter(multithread);
    thread = CreateThread(NULL, 0, mt_factory_test_thread_func, multithread, 0, NULL);
    ok(!!thread, "Failed to create a thread.\n");
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);

    ID2D1Multithread_Release(multithread);
    ID2D1Factory_Release(factory);

    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, &IID_ID2D1Factory, NULL, (void **)&factory);
    ok(SUCCEEDED(hr), "Failed to create factory, hr %#x.\n", hr);

    hr = ID2D1Factory_QueryInterface(factory, &IID_ID2D1Multithread, (void **)&multithread);
    ok(SUCCEEDED(hr), "Failed to get interface, hr %#x.\n", hr);

    ret = ID2D1Multithread_GetMultithreadProtected(multithread);
    ok(!!ret, "Unexpected return value.\n");

    ID2D1Multithread_Enter(multithread);
    thread = CreateThread(NULL, 0, mt_factory_test_thread_func, multithread, 0, NULL);
    ok(!!thread, "Failed to create a thread.\n");
    ret = WaitForSingleObject(thread, 10);
    ok(ret == WAIT_TIMEOUT, "Expected timeout.\n");
    ID2D1Multithread_Leave(multithread);
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);

    ID2D1Multithread_Release(multithread);

    ID2D1Factory_Release(factory);
}

static void test_effect(BOOL d3d11)
{
    unsigned int i, min_inputs, max_inputs;
    D2D1_BUFFER_PRECISION precision;
    ID2D1Image *image_a, *image_b;
    struct d2d1_test_context ctx;
    ID2D1DeviceContext *context;
    ID2D1Factory1 *factory;
    ID2D1Effect *effect;
    BOOL cached;
    CLSID clsid;
    HRESULT hr;

    const struct effect_test
    {
        const CLSID *clsid;
        UINT32 min_inputs;
        UINT32 max_inputs;
    }
    effect_tests[] =
    {
        {&CLSID_D2D12DAffineTransform,       1, 1},
        {&CLSID_D2D13DPerspectiveTransform,  1, 1},
        {&CLSID_D2D1Composite,               1, 0xffffffff},
    };

    if (!init_test_context(&ctx, d3d11))
        return;

    if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &IID_ID2D1Factory1, NULL, (void **)&factory)))
    {
        win_skip("ID2D1Factory1 is not supported.\n");
        release_test_context(&ctx);
        return;
    }

    hr = ID2D1RenderTarget_QueryInterface(ctx.rt, &IID_ID2D1DeviceContext, (void **)&context);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    for (i = 0; i < ARRAY_SIZE(effect_tests); ++i)
    {
        const struct effect_test *test = effect_tests + i;

        winetest_push_context("Test %u", i);

        hr = ID2D1DeviceContext_CreateEffect(context, test->clsid, &effect);
        ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

        hr = ID2D1Effect_QueryInterface(effect, &IID_ID2D1Image, (void **)&image_a);
        ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
        ID2D1Effect_GetOutput(effect, &image_b);
        ok(image_b == image_a, "Got unexpected image_b %p, expected %p.\n", image_b, image_a);
        ID2D1Image_Release(image_b);
        ID2D1Image_Release(image_a);

        todo_wine
        {
        hr = ID2D1Effect_GetValue(effect, D2D1_PROPERTY_CLSID,
                D2D1_PROPERTY_TYPE_CLSID, (BYTE *)&clsid, sizeof(clsid));
        ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
        if (hr == S_OK)
            ok(IsEqualGUID(&clsid, test->clsid), "Got unexpected clsid %s, expected %s.\n",
                    debugstr_guid(&clsid), debugstr_guid(test->clsid));

        hr = ID2D1Effect_GetValue(effect, D2D1_PROPERTY_CACHED,
                D2D1_PROPERTY_TYPE_BOOL, (BYTE *)&cached, sizeof(cached));
        ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
        if (hr == S_OK)
            ok(cached == FALSE, "Got unexpected cached %d.\n", cached);

        hr = ID2D1Effect_GetValue(effect, D2D1_PROPERTY_PRECISION,
                D2D1_PROPERTY_TYPE_ENUM, (BYTE *)&precision, sizeof(precision));
        ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
        if (hr == S_OK)
            ok(precision == D2D1_BUFFER_PRECISION_UNKNOWN, "Got unexpected precision %u.\n", precision);

        hr = ID2D1Effect_GetValue(effect, D2D1_PROPERTY_MIN_INPUTS,
                D2D1_PROPERTY_TYPE_UINT32, (BYTE *)&min_inputs, sizeof(min_inputs));
        ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
        if (hr == S_OK)
            ok(min_inputs == test->min_inputs, "Got unexpected min inputs %u, expected %u.\n",
                    min_inputs, test->min_inputs);

        hr = ID2D1Effect_GetValue(effect, D2D1_PROPERTY_MAX_INPUTS,
                D2D1_PROPERTY_TYPE_UINT32, (BYTE *)&max_inputs, sizeof(max_inputs));
        ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
        if (hr == S_OK)
            ok(max_inputs == test->max_inputs, "Got unexpected max inputs %u, expected %u.\n",
                    max_inputs, test->max_inputs);
        }

        ID2D1Effect_Release(effect);
        winetest_pop_context();
    }

    ID2D1DeviceContext_Release(context);
    ID2D1Factory1_Release(factory);
    release_test_context(&ctx);
}

START_TEST(d2d1)
{
    HMODULE d2d1_dll = GetModuleHandleA("d2d1.dll");
    unsigned int argc, i;
    char **argv;

    pD2D1CreateDevice = (void *)GetProcAddress(d2d1_dll, "D2D1CreateDevice");
    pD2D1SinCos = (void *)GetProcAddress(d2d1_dll, "D2D1SinCos");
    pD2D1Tan = (void *)GetProcAddress(d2d1_dll, "D2D1Tan");
    pD2D1Vec3Length = (void *)GetProcAddress(d2d1_dll, "D2D1Vec3Length");
    pD2D1ConvertColorSpace = (void *)GetProcAddress(d2d1_dll, "D2D1ConvertColorSpace");

    use_mt = !getenv("WINETEST_NO_MT_D3D");

    argc = winetest_get_mainargs(&argv);
    for (i = 2; i < argc; ++i)
    {
        if (!strcmp(argv[i], "--single"))
            use_mt = FALSE;
    }

    queue_test(test_clip);
    queue_test(test_state_block);
    queue_test(test_color_brush);
    queue_test(test_bitmap_brush);
    queue_test(test_linear_brush);
    queue_test(test_radial_brush);
    queue_test(test_path_geometry);
    queue_d3d10_test(test_rectangle_geometry);
    queue_d3d10_test(test_rounded_rectangle_geometry);
    queue_test(test_bitmap_formats);
    queue_test(test_alpha_mode);
    queue_test(test_shared_bitmap);
    queue_test(test_bitmap_updates);
    queue_test(test_opacity_brush);
    queue_test(test_create_target);
    queue_test(test_draw_text_layout);
    queue_test(test_dc_target);
    queue_test(test_hwnd_target);
    queue_test(test_bitmap_target);
    queue_d3d10_test(test_desktop_dpi);
    queue_d3d10_test(test_stroke_style);
    queue_test(test_gradient);
    queue_test(test_draw_geometry);
    queue_test(test_fill_geometry);
    queue_test(test_gdi_interop);
    queue_test(test_layer);
    queue_test(test_bezier_intersect);
    queue_test(test_create_device);
    queue_test(test_bitmap_surface);
    queue_test(test_device_context);
    queue_d3d10_test(test_invert_matrix);
    queue_d3d10_test(test_skew_matrix);
    queue_test(test_command_list);
    queue_d3d10_test(test_max_bitmap_size);
    queue_test(test_dpi);
    queue_test(test_wic_bitmap_format);
    queue_d3d10_test(test_math);
    queue_d3d10_test(test_colour_space);
    queue_test(test_geometry_group);
    queue_test(test_mt_factory);
    queue_test(test_effect);

    run_queued_tests();
}
