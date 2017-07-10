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

#include "config.h"
#include "wine/port.h"

#define D2D1_INIT_GUID
#include "d2d1_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d2d);

struct d2d_factory
{
    ID2D1Factory ID2D1Factory_iface;
    LONG refcount;

    ID3D10Device1 *device;

    float dpi_x;
    float dpi_y;
};

static inline struct d2d_factory *impl_from_ID2D1Factory(ID2D1Factory *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_factory, ID2D1Factory_iface);
}

static HRESULT d2d_factory_reload_sysmetrics(struct d2d_factory *factory)
{
    HDC hdc;

    if (!(hdc = GetDC(NULL)))
    {
        factory->dpi_x = factory->dpi_y = 96.0f;
        return E_FAIL;
    }

    factory->dpi_x = GetDeviceCaps(hdc, LOGPIXELSX);
    factory->dpi_y = GetDeviceCaps(hdc, LOGPIXELSY);

    ReleaseDC(NULL, hdc);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d2d_factory_QueryInterface(ID2D1Factory *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID2D1Factory)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        ID2D1Factory_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d2d_factory_AddRef(ID2D1Factory *iface)
{
    struct d2d_factory *factory = impl_from_ID2D1Factory(iface);
    ULONG refcount = InterlockedIncrement(&factory->refcount);

    TRACE("%p increasing refcount to %u.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d2d_factory_Release(ID2D1Factory *iface)
{
    struct d2d_factory *factory = impl_from_ID2D1Factory(iface);
    ULONG refcount = InterlockedDecrement(&factory->refcount);

    TRACE("%p decreasing refcount to %u.\n", iface, refcount);

    if (!refcount)
    {
        if (factory->device)
            ID3D10Device1_Release(factory->device);
        HeapFree(GetProcessHeap(), 0, factory);
    }

    return refcount;
}

static HRESULT STDMETHODCALLTYPE d2d_factory_ReloadSystemMetrics(ID2D1Factory *iface)
{
    struct d2d_factory *factory = impl_from_ID2D1Factory(iface);

    TRACE("iface %p.\n", iface);

    return d2d_factory_reload_sysmetrics(factory);
}

static void STDMETHODCALLTYPE d2d_factory_GetDesktopDpi(ID2D1Factory *iface, float *dpi_x, float *dpi_y)
{
    struct d2d_factory *factory = impl_from_ID2D1Factory(iface);

    TRACE("iface %p, dpi_x %p, dpi_y %p.\n", iface, dpi_x, dpi_y);

    *dpi_x = factory->dpi_x;
    *dpi_y = factory->dpi_y;
}

static HRESULT STDMETHODCALLTYPE d2d_factory_CreateRectangleGeometry(ID2D1Factory *iface,
        const D2D1_RECT_F *rect, ID2D1RectangleGeometry **geometry)
{
    struct d2d_geometry *object;
    HRESULT hr;

    TRACE("iface %p, rect %s, geometry %p.\n", iface, debug_d2d_rect_f(rect), geometry);

    if (!(object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = d2d_rectangle_geometry_init(object, iface, rect)))
    {
        WARN("Failed to initialize rectangle geometry, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    TRACE("Created rectangle geometry %p.\n", object);
    *geometry = (ID2D1RectangleGeometry *)&object->ID2D1Geometry_iface;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d2d_factory_CreateRoundedRectangleGeometry(ID2D1Factory *iface,
        const D2D1_ROUNDED_RECT *rect, ID2D1RoundedRectangleGeometry **geometry)
{
    FIXME("iface %p, rect %p, geometry %p stub!\n", iface, rect, geometry);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_factory_CreateEllipseGeometry(ID2D1Factory *iface,
        const D2D1_ELLIPSE *ellipse, ID2D1EllipseGeometry **geometry)
{
    FIXME("iface %p, ellipse %p, geometry %p stub!\n", iface, ellipse, geometry);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_factory_CreateGeometryGroup(ID2D1Factory *iface,
        D2D1_FILL_MODE fill_mode, ID2D1Geometry **geometries, UINT32 geometry_count, ID2D1GeometryGroup **group)
{
    FIXME("iface %p, fill_mode %#x, geometries %p, geometry_count %u, group %p stub!\n",
            iface, fill_mode, geometries, geometry_count, group);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_factory_CreateTransformedGeometry(ID2D1Factory *iface,
        ID2D1Geometry *src_geometry, const D2D1_MATRIX_3X2_F *transform,
        ID2D1TransformedGeometry **transformed_geometry)
{
    struct d2d_geometry *object;

    TRACE("iface %p, src_geometry %p, transform %p, transformed_geometry %p.\n",
            iface, src_geometry, transform, transformed_geometry);

    if (!(object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object))))
        return E_OUTOFMEMORY;

    d2d_transformed_geometry_init(object, iface, src_geometry, transform);

    TRACE("Created transformed geometry %p.\n", object);
    *transformed_geometry = (ID2D1TransformedGeometry *)&object->ID2D1Geometry_iface;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d2d_factory_CreatePathGeometry(ID2D1Factory *iface, ID2D1PathGeometry **geometry)
{
    struct d2d_geometry *object;

    TRACE("iface %p, geometry %p.\n", iface, geometry);

    if (!(object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object))))
        return E_OUTOFMEMORY;

    d2d_path_geometry_init(object, iface);

    TRACE("Created path geometry %p.\n", object);
    *geometry = (ID2D1PathGeometry *)&object->ID2D1Geometry_iface;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d2d_factory_CreateStrokeStyle(ID2D1Factory *iface,
        const D2D1_STROKE_STYLE_PROPERTIES *desc, const float *dashes, UINT32 dash_count,
        ID2D1StrokeStyle **stroke_style)
{
    struct d2d_stroke_style *object;
    HRESULT hr;

    TRACE("iface %p, desc %p, dashes %p, dash_count %u, stroke_style %p.\n",
            iface, desc, dashes, dash_count, stroke_style);

    if (!(object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = d2d_stroke_style_init(object, iface, desc, dashes, dash_count)))
    {
        WARN("Failed to initialize stroke style, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    TRACE("Created stroke style %p.\n", object);
    *stroke_style = &object->ID2D1StrokeStyle_iface;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d2d_factory_CreateDrawingStateBlock(ID2D1Factory *iface,
        const D2D1_DRAWING_STATE_DESCRIPTION *desc, IDWriteRenderingParams *text_rendering_params,
        ID2D1DrawingStateBlock **state_block)
{
    struct d2d_state_block *object;

    TRACE("iface %p, desc %p, text_rendering_params %p, state_block %p.\n",
            iface, desc, text_rendering_params, state_block);

    if (!(object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object))))
        return E_OUTOFMEMORY;

    d2d_state_block_init(object, iface, desc, text_rendering_params);

    TRACE("Created state block %p.\n", object);
    *state_block = &object->ID2D1DrawingStateBlock_iface;

    return S_OK;
}

static HRESULT d2d_factory_get_device(struct d2d_factory *factory, ID3D10Device1 **device)
{
    HRESULT hr = S_OK;

    if (!factory->device && FAILED(hr = D3D10CreateDevice1(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, D3D10_CREATE_DEVICE_BGRA_SUPPORT,
            D3D10_FEATURE_LEVEL_10_0, D3D10_1_SDK_VERSION, &factory->device)))
        WARN("Failed to create device, hr %#x.\n", hr);

    *device = factory->device;
    return hr;
}

static HRESULT STDMETHODCALLTYPE d2d_factory_CreateWicBitmapRenderTarget(ID2D1Factory *iface,
        IWICBitmap *target, const D2D1_RENDER_TARGET_PROPERTIES *desc, ID2D1RenderTarget **render_target)
{
    struct d2d_factory *factory = impl_from_ID2D1Factory(iface);
    struct d2d_wic_render_target *object;
    ID3D10Device1 *device;
    HRESULT hr;

    TRACE("iface %p, target %p, desc %p, render_target %p.\n", iface, target, desc, render_target);

    if (!(object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = d2d_factory_get_device(factory, &device)))
    {
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    if (FAILED(hr = d2d_wic_render_target_init(object, iface, device, target, desc)))
    {
        WARN("Failed to initialize render target, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    TRACE("Created render target %p.\n", object);
    *render_target = &object->ID2D1RenderTarget_iface;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d2d_factory_CreateHwndRenderTarget(ID2D1Factory *iface,
        const D2D1_RENDER_TARGET_PROPERTIES *desc, const D2D1_HWND_RENDER_TARGET_PROPERTIES *hwnd_rt_desc,
        ID2D1HwndRenderTarget **render_target)
{
    struct d2d_factory *factory = impl_from_ID2D1Factory(iface);
    struct d2d_hwnd_render_target *object;
    ID3D10Device1 *device;
    HRESULT hr;

    TRACE("iface %p, desc %p, hwnd_rt_desc %p, render_target %p.\n", iface, desc, hwnd_rt_desc, render_target);

    if (FAILED(hr = d2d_factory_get_device(factory, &device)))
        return hr;

    if (!(object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = d2d_hwnd_render_target_init(object, iface, device, desc, hwnd_rt_desc)))
    {
        WARN("Failed to initialize render target, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    TRACE("Created render target %p.\n", object);
    *render_target = &object->ID2D1HwndRenderTarget_iface;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d2d_factory_CreateDxgiSurfaceRenderTarget(ID2D1Factory *iface,
        IDXGISurface *surface, const D2D1_RENDER_TARGET_PROPERTIES *desc, ID2D1RenderTarget **render_target)
{
    TRACE("iface %p, surface %p, desc %p, render_target %p.\n", iface, surface, desc, render_target);

    return d2d_d3d_create_render_target(iface, surface, NULL, desc, render_target);
}

static HRESULT STDMETHODCALLTYPE d2d_factory_CreateDCRenderTarget(ID2D1Factory *iface,
        const D2D1_RENDER_TARGET_PROPERTIES *desc, ID2D1DCRenderTarget **render_target)
{
    struct d2d_factory *factory = impl_from_ID2D1Factory(iface);
    struct d2d_dc_render_target *object;
    ID3D10Device1 *device;
    HRESULT hr;

    TRACE("iface %p, desc %p, render_target %p.\n", iface, desc, render_target);

    if (FAILED(hr = d2d_factory_get_device(factory, &device)))
        return hr;

    if (!(object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = d2d_dc_render_target_init(object, iface, device, desc)))
    {
        WARN("Failed to initialize render target, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    TRACE("Created render target %p.\n", object);
    *render_target = &object->ID2D1DCRenderTarget_iface;

    return S_OK;
}

static const struct ID2D1FactoryVtbl d2d_factory_vtbl =
{
    d2d_factory_QueryInterface,
    d2d_factory_AddRef,
    d2d_factory_Release,
    d2d_factory_ReloadSystemMetrics,
    d2d_factory_GetDesktopDpi,
    d2d_factory_CreateRectangleGeometry,
    d2d_factory_CreateRoundedRectangleGeometry,
    d2d_factory_CreateEllipseGeometry,
    d2d_factory_CreateGeometryGroup,
    d2d_factory_CreateTransformedGeometry,
    d2d_factory_CreatePathGeometry,
    d2d_factory_CreateStrokeStyle,
    d2d_factory_CreateDrawingStateBlock,
    d2d_factory_CreateWicBitmapRenderTarget,
    d2d_factory_CreateHwndRenderTarget,
    d2d_factory_CreateDxgiSurfaceRenderTarget,
    d2d_factory_CreateDCRenderTarget,
};

static void d2d_factory_init(struct d2d_factory *factory, D2D1_FACTORY_TYPE factory_type,
        const D2D1_FACTORY_OPTIONS *factory_options)
{
    if (factory_type != D2D1_FACTORY_TYPE_SINGLE_THREADED)
        FIXME("Ignoring factory type %#x.\n", factory_type);
    if (factory_options && factory_options->debugLevel != D2D1_DEBUG_LEVEL_NONE)
        WARN("Ignoring debug level %#x.\n", factory_options->debugLevel);

    factory->ID2D1Factory_iface.lpVtbl = &d2d_factory_vtbl;
    factory->refcount = 1;
    d2d_factory_reload_sysmetrics(factory);
}

HRESULT WINAPI D2D1CreateFactory(D2D1_FACTORY_TYPE factory_type, REFIID iid,
        const D2D1_FACTORY_OPTIONS *factory_options, void **factory)
{
    struct d2d_factory *object;
    HRESULT hr;

    TRACE("factory_type %#x, iid %s, factory_options %p, factory %p.\n",
            factory_type, debugstr_guid(iid), factory_options, factory);

    if (!(object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object))))
        return E_OUTOFMEMORY;

    d2d_factory_init(object, factory_type, factory_options);

    TRACE("Created factory %p.\n", object);

    hr = ID2D1Factory_QueryInterface(&object->ID2D1Factory_iface, iid, factory);
    ID2D1Factory_Release(&object->ID2D1Factory_iface);

    return hr;
}

void WINAPI D2D1MakeRotateMatrix(float angle, D2D1_POINT_2F center, D2D1_MATRIX_3X2_F *matrix)
{
    float theta, sin_theta, cos_theta;

    TRACE("angle %.8e, center {%.8e, %.8e}, matrix %p.\n", angle, center.x, center.y, matrix);

    theta = angle * (M_PI / 180.0f);
    sin_theta = sinf(theta);
    cos_theta = cosf(theta);

    /* translate(center) * rotate(theta) * translate(-center) */
    matrix->_11 = cos_theta;
    matrix->_12 = sin_theta;
    matrix->_21 = -sin_theta;
    matrix->_22 = cos_theta;
    matrix->_31 = center.x - center.x * cos_theta + center.y * sin_theta;
    matrix->_32 = center.y - center.x * sin_theta - center.y * cos_theta;
}
