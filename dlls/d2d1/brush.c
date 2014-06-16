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

static inline struct d2d_gradient *impl_from_ID2D1GradientStopCollection(ID2D1GradientStopCollection *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_gradient, ID2D1GradientStopCollection_iface);
}

static HRESULT STDMETHODCALLTYPE d2d_gradient_QueryInterface(ID2D1GradientStopCollection *iface,
        REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID2D1GradientStopCollection)
            || IsEqualGUID(iid, &IID_ID2D1Resource)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        ID2D1GradientStopCollection_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d2d_gradient_AddRef(ID2D1GradientStopCollection *iface)
{
    struct d2d_gradient *gradient = impl_from_ID2D1GradientStopCollection(iface);
    ULONG refcount = InterlockedIncrement(&gradient->refcount);

    TRACE("%p increasing refcount to %u.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d2d_gradient_Release(ID2D1GradientStopCollection *iface)
{
    struct d2d_gradient *gradient = impl_from_ID2D1GradientStopCollection(iface);
    ULONG refcount = InterlockedDecrement(&gradient->refcount);

    TRACE("%p decreasing refcount to %u.\n", iface, refcount);

    if (!refcount)
        HeapFree(GetProcessHeap(), 0, gradient);

    return refcount;
}

static void STDMETHODCALLTYPE d2d_gradient_GetFactory(ID2D1GradientStopCollection *iface, ID2D1Factory **factory)
{
    FIXME("iface %p, factory %p stub!\n", iface, factory);

    *factory = NULL;
}

static UINT32 STDMETHODCALLTYPE d2d_gradient_GetGradientStopCount(ID2D1GradientStopCollection *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static void STDMETHODCALLTYPE d2d_gradient_GetGradientStops(ID2D1GradientStopCollection *iface,
        D2D1_GRADIENT_STOP *stops, UINT32 stop_count)
{
    FIXME("iface %p, stops %p, stop_count %u stub!\n", iface, stops, stop_count);
}

static D2D1_GAMMA STDMETHODCALLTYPE d2d_gradient_GetColorInterpolationGamma(ID2D1GradientStopCollection *iface)
{
    FIXME("iface %p stub!\n", iface);

    return D2D1_GAMMA_1_0;
}

static D2D1_EXTEND_MODE STDMETHODCALLTYPE d2d_gradient_GetExtendMode(ID2D1GradientStopCollection *iface)
{
    FIXME("iface %p stub!\n", iface);

    return D2D1_EXTEND_MODE_CLAMP;
}

static const struct ID2D1GradientStopCollectionVtbl d2d_gradient_vtbl =
{
    d2d_gradient_QueryInterface,
    d2d_gradient_AddRef,
    d2d_gradient_Release,
    d2d_gradient_GetFactory,
    d2d_gradient_GetGradientStopCount,
    d2d_gradient_GetGradientStops,
    d2d_gradient_GetColorInterpolationGamma,
    d2d_gradient_GetExtendMode,
};

void d2d_gradient_init(struct d2d_gradient *gradient, ID2D1RenderTarget *render_target,
        const D2D1_GRADIENT_STOP *stops, UINT32 stop_count, D2D1_GAMMA gamma, D2D1_EXTEND_MODE extend_mode)
{
    FIXME("Ignoring gradient properties.\n");

    gradient->ID2D1GradientStopCollection_iface.lpVtbl = &d2d_gradient_vtbl;
    gradient->refcount = 1;
}

static void d2d_brush_init(struct d2d_brush *brush, ID2D1RenderTarget *render_target,
        const D2D1_BRUSH_PROPERTIES *desc, const struct ID2D1BrushVtbl *vtbl)
{
    brush->ID2D1Brush_iface.lpVtbl = vtbl;
    brush->refcount = 1;
}

static inline struct d2d_brush *impl_from_ID2D1SolidColorBrush(ID2D1SolidColorBrush *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_brush, ID2D1Brush_iface);
}

static HRESULT STDMETHODCALLTYPE d2d_solid_color_brush_QueryInterface(ID2D1SolidColorBrush *iface,
        REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID2D1SolidColorBrush)
            || IsEqualGUID(iid, &IID_ID2D1Brush)
            || IsEqualGUID(iid, &IID_ID2D1Resource)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        ID2D1SolidColorBrush_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d2d_solid_color_brush_AddRef(ID2D1SolidColorBrush *iface)
{
    struct d2d_brush *brush = impl_from_ID2D1SolidColorBrush(iface);
    ULONG refcount = InterlockedIncrement(&brush->refcount);

    TRACE("%p increasing refcount to %u.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d2d_solid_color_brush_Release(ID2D1SolidColorBrush *iface)
{
    struct d2d_brush *brush = impl_from_ID2D1SolidColorBrush(iface);
    ULONG refcount = InterlockedDecrement(&brush->refcount);

    TRACE("%p decreasing refcount to %u.\n", iface, refcount);

    if (!refcount)
        HeapFree(GetProcessHeap(), 0, brush);

    return refcount;
}

static void STDMETHODCALLTYPE d2d_solid_color_brush_GetFactory(ID2D1SolidColorBrush *iface, ID2D1Factory **factory)
{
    FIXME("iface %p, factory %p stub!\n", iface, factory);

    *factory = NULL;
}

static void STDMETHODCALLTYPE d2d_solid_color_brush_SetOpacity(ID2D1SolidColorBrush *iface, float opacity)
{
    FIXME("iface %p, opacity %.8e stub!\n", iface, opacity);
}

static void STDMETHODCALLTYPE d2d_solid_color_brush_SetTransform(ID2D1SolidColorBrush *iface,
        const D2D1_MATRIX_3X2_F *transform)
{
    FIXME("iface %p, transform %p stub!\n", iface, transform);
}

static float STDMETHODCALLTYPE d2d_solid_color_brush_GetOpacity(ID2D1SolidColorBrush *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0.0f;
}

static void STDMETHODCALLTYPE d2d_solid_color_brush_GetTransform(ID2D1SolidColorBrush *iface,
        D2D1_MATRIX_3X2_F *transform)
{
    static const D2D1_MATRIX_3X2_F identity =
    {
        1.0f, 0.0f,
        0.0f, 1.0f,
        0.0f, 0.0f,
    };

    FIXME("iface %p, transform %p stub!\n", iface, transform);

    *transform = identity;
}

static void STDMETHODCALLTYPE d2d_solid_color_brush_SetColor(ID2D1SolidColorBrush *iface, const D2D1_COLOR_F *color)
{
    FIXME("iface %p, color %p stub!\n", iface, color);
}

static D2D1_COLOR_F STDMETHODCALLTYPE d2d_solid_color_brush_GetColor(ID2D1SolidColorBrush *iface)
{
    static const D2D1_COLOR_F black = {0.0f, 0.0f, 0.0f, 1.0f};

    FIXME("iface %p stub!\n", iface);

    return black;
}

static const struct ID2D1SolidColorBrushVtbl d2d_solid_color_brush_vtbl =
{
    d2d_solid_color_brush_QueryInterface,
    d2d_solid_color_brush_AddRef,
    d2d_solid_color_brush_Release,
    d2d_solid_color_brush_GetFactory,
    d2d_solid_color_brush_SetOpacity,
    d2d_solid_color_brush_SetTransform,
    d2d_solid_color_brush_GetOpacity,
    d2d_solid_color_brush_GetTransform,
    d2d_solid_color_brush_SetColor,
    d2d_solid_color_brush_GetColor,
};

void d2d_solid_color_brush_init(struct d2d_brush *brush, ID2D1RenderTarget *render_target,
        const D2D1_COLOR_F *color, const D2D1_BRUSH_PROPERTIES *desc)
{
    FIXME("Ignoring brush properties.\n");

    d2d_brush_init(brush, render_target, desc, (ID2D1BrushVtbl *)&d2d_solid_color_brush_vtbl);
}

static inline struct d2d_brush *impl_from_ID2D1LinearGradientBrush(ID2D1LinearGradientBrush *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_brush, ID2D1Brush_iface);
}

static HRESULT STDMETHODCALLTYPE d2d_linear_gradient_brush_QueryInterface(ID2D1LinearGradientBrush *iface,
        REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID2D1LinearGradientBrush)
            || IsEqualGUID(iid, &IID_ID2D1Brush)
            || IsEqualGUID(iid, &IID_ID2D1Resource)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        ID2D1LinearGradientBrush_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d2d_linear_gradient_brush_AddRef(ID2D1LinearGradientBrush *iface)
{
    struct d2d_brush *brush = impl_from_ID2D1LinearGradientBrush(iface);
    ULONG refcount = InterlockedIncrement(&brush->refcount);

    TRACE("%p increasing refcount to %u.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d2d_linear_gradient_brush_Release(ID2D1LinearGradientBrush *iface)
{
    struct d2d_brush *brush = impl_from_ID2D1LinearGradientBrush(iface);
    ULONG refcount = InterlockedDecrement(&brush->refcount);

    TRACE("%p decreasing refcount to %u.\n", iface, refcount);

    if (!refcount)
        HeapFree(GetProcessHeap(), 0, brush);

    return refcount;
}

static void STDMETHODCALLTYPE d2d_linear_gradient_brush_GetFactory(ID2D1LinearGradientBrush *iface,
        ID2D1Factory **factory)
{
    FIXME("iface %p, factory %p stub!\n", iface, factory);

    *factory = NULL;
}

static void STDMETHODCALLTYPE d2d_linear_gradient_brush_SetOpacity(ID2D1LinearGradientBrush *iface, float opacity)
{
    FIXME("iface %p, opacity %.8e stub!\n", iface, opacity);
}

static void STDMETHODCALLTYPE d2d_linear_gradient_brush_SetTransform(ID2D1LinearGradientBrush *iface,
        const D2D1_MATRIX_3X2_F *transform)
{
    FIXME("iface %p, transform %p stub!\n", iface, transform);
}

static float STDMETHODCALLTYPE d2d_linear_gradient_brush_GetOpacity(ID2D1LinearGradientBrush *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0.0f;
}

static void STDMETHODCALLTYPE d2d_linear_gradient_brush_GetTransform(ID2D1LinearGradientBrush *iface,
        D2D1_MATRIX_3X2_F *transform)
{
    static const D2D1_MATRIX_3X2_F identity =
    {
        1.0f, 0.0f,
        0.0f, 1.0f,
        0.0f, 0.0f,
    };

    FIXME("iface %p, transform %p stub!\n", iface, transform);

    *transform = identity;
}

static void STDMETHODCALLTYPE d2d_linear_gradient_brush_SetStartPoint(ID2D1LinearGradientBrush *iface,
        D2D1_POINT_2F start_point)
{
    FIXME("iface %p, start_point {%.8e, %.8e} stub!\n", iface, start_point.x, start_point.y);
}

static void STDMETHODCALLTYPE d2d_linear_gradient_brush_SetEndPoint(ID2D1LinearGradientBrush *iface,
        D2D1_POINT_2F end_point)
{
    FIXME("iface %p, end_point {%.8e, %.8e} stub!\n", iface, end_point.x, end_point.y);
}

static D2D1_POINT_2F STDMETHODCALLTYPE d2d_linear_gradient_brush_GetStartPoint(ID2D1LinearGradientBrush *iface)
{
    static const D2D1_POINT_2F point = {0.0f, 0.0f};

    FIXME("iface %p stub!\n", iface);

    return point;
}

static D2D1_POINT_2F STDMETHODCALLTYPE d2d_linear_gradient_brush_GetEndPoint(ID2D1LinearGradientBrush *iface)
{
    static const D2D1_POINT_2F point = {0.0f, 0.0f};

    FIXME("iface %p stub!\n", iface);

    return point;
}

static void STDMETHODCALLTYPE d2d_linear_gradient_brush_GetGradientStopCollection(ID2D1LinearGradientBrush *iface,
        ID2D1GradientStopCollection **gradient)
{
    FIXME("iface %p, gradient %p stub!\n", iface, gradient);

    *gradient = NULL;
}

static const struct ID2D1LinearGradientBrushVtbl d2d_linear_gradient_brush_vtbl =
{
    d2d_linear_gradient_brush_QueryInterface,
    d2d_linear_gradient_brush_AddRef,
    d2d_linear_gradient_brush_Release,
    d2d_linear_gradient_brush_GetFactory,
    d2d_linear_gradient_brush_SetOpacity,
    d2d_linear_gradient_brush_SetTransform,
    d2d_linear_gradient_brush_GetOpacity,
    d2d_linear_gradient_brush_GetTransform,
    d2d_linear_gradient_brush_SetStartPoint,
    d2d_linear_gradient_brush_SetEndPoint,
    d2d_linear_gradient_brush_GetStartPoint,
    d2d_linear_gradient_brush_GetEndPoint,
    d2d_linear_gradient_brush_GetGradientStopCollection,
};

void d2d_linear_gradient_brush_init(struct d2d_brush *brush, ID2D1RenderTarget *render_target,
        const D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES *gradient_brush_desc, const D2D1_BRUSH_PROPERTIES *brush_desc,
        ID2D1GradientStopCollection *gradient)
{
    FIXME("Ignoring brush properties.\n");

    d2d_brush_init(brush, render_target, brush_desc, (ID2D1BrushVtbl *)&d2d_solid_color_brush_vtbl);
}
