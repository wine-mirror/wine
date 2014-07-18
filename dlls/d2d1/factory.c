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

#include "d2d1_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d2d);

struct d2d_factory
{
    ID2D1Factory ID2D1Factory_iface;
    LONG refcount;
};

static inline struct d2d_factory *impl_from_ID2D1Factory(ID2D1Factory *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_factory, ID2D1Factory_iface);
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
        HeapFree(GetProcessHeap(), 0, factory);

    return refcount;
}

static HRESULT STDMETHODCALLTYPE d2d_factory_ReloadSystemMetrics(ID2D1Factory *iface)
{
    FIXME("iface %p stub!\n", iface);

    return E_NOTIMPL;
}

static void STDMETHODCALLTYPE d2d_factory_GetDesktopDpi(ID2D1Factory *iface, float *dpi_x, float *dpi_y)
{
    FIXME("iface %p, dpi_x %p, dpi_y %p stub!\n", iface, dpi_x, dpi_y);

    *dpi_x = 96.0f;
    *dpi_y = 96.0f;
}

static HRESULT STDMETHODCALLTYPE d2d_factory_CreateRectangleGeometry(ID2D1Factory *iface,
        const D2D1_RECT_F *rect, ID2D1RectangleGeometry **geometry)
{
    FIXME("iface %p, rect %p, geometry %p stub!\n", iface, rect, geometry);

    return E_NOTIMPL;
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
        D2D1_FILL_MODE fill_mode, ID2D1Geometry *geometry, UINT32 geometry_count, ID2D1GeometryGroup **group)
{
    FIXME("iface %p, fill_mode %#x, geometry %p, geometry_count %u, group %p stub!\n",
            iface, fill_mode, geometry, geometry_count, group);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_factory_CreateTransformedGeometry(ID2D1Factory *iface,
        ID2D1Geometry *src_geometry, const D2D1_MATRIX_3X2_F *transform,
        ID2D1TransformedGeometry **transformed_geometry)
{
    FIXME("iface %p, src_geometry %p, transform %p, transformed_geometry %p stub!\n",
            iface, src_geometry, transform, transformed_geometry);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_factory_CreatePathGeometry(ID2D1Factory *iface, ID2D1PathGeometry *geometry)
{
    FIXME("iface %p, geometry %p stub!\n", iface, geometry);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_factory_CreateStrokeStyle(ID2D1Factory *iface,
        const D2D1_STROKE_STYLE_PROPERTIES *desc, const float *dashes, UINT32 dash_count,
        ID2D1StrokeStyle **stroke_style)
{
    struct d2d_stroke_style *object;

    TRACE("iface %p, desc %p, dashes %p, dash_count %u, stroke_style %p.\n",
            iface, desc, dashes, dash_count, stroke_style);

    if (!(object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object))))
        return E_OUTOFMEMORY;

    d2d_stroke_style_init(object, iface, desc, dashes, dash_count);

    TRACE("Created stroke style %p.\n", object);
    *stroke_style = &object->ID2D1StrokeStyle_iface;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d2d_factory_CreateDrawingStateBlock(ID2D1Factory *iface,
        const D2D1_DRAWING_STATE_DESCRIPTION *desc, IDWriteRenderingParams *text_rendering_params,
        ID2D1DrawingStateBlock **state_block)
{
    FIXME("iface %p, desc %p, text_rendering_params %p, state_block %p stub!\n",
            iface, desc, text_rendering_params, state_block);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_factory_CreateWicBitmapRenderTarget(ID2D1Factory *iface,
        IWICBitmap *target, const D2D1_RENDER_TARGET_PROPERTIES *desc, ID2D1RenderTarget **render_target)
{
    FIXME("iface %p, target %p, desc %p, render_target %p stub!\n", iface, target, desc, render_target);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_factory_CreateHwndRenderTarget(ID2D1Factory *iface,
        const D2D1_RENDER_TARGET_PROPERTIES *desc, const D2D1_HWND_RENDER_TARGET_PROPERTIES *hwnd_rt_desc,
        ID2D1HwndRenderTarget **render_target)
{
    FIXME("iface %p, desc %p, hwnd_rt_desc %p, render_target %p stub!\n", iface, desc, hwnd_rt_desc, render_target);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_factory_CreateDxgiSurfaceRenderTarget(ID2D1Factory *iface,
        IDXGISurface *surface, const D2D1_RENDER_TARGET_PROPERTIES *desc, ID2D1RenderTarget **render_target)
{
    struct d2d_d3d_render_target *object;

    TRACE("iface %p, surface %p, desc %p, render_target %p.\n", iface, surface, desc, render_target);

    if (!(object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object))))
        return E_OUTOFMEMORY;

    d2d_d3d_render_target_init(object, iface, surface, desc);

    TRACE("Created render target %p.\n", object);
    *render_target = &object->ID2D1RenderTarget_iface;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d2d_factory_CreateDCRenderTarget(ID2D1Factory *iface,
        const D2D1_RENDER_TARGET_PROPERTIES *desc, ID2D1DCRenderTarget **render_target)
{
    FIXME("iface %p, desc %p, render_target %p stub!\n", iface, desc, render_target);

    return E_NOTIMPL;
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
    FIXME("Ignoring factory type and options.\n");

    factory->ID2D1Factory_iface.lpVtbl = &d2d_factory_vtbl;
    factory->refcount = 1;
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
