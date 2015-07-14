/*
 * Copyright 2015 Henri Verbeet for CodeWeavers
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

static void d2d_geometry_destroy(struct d2d_geometry *geometry)
{
    HeapFree(GetProcessHeap(), 0, geometry->faces);
    HeapFree(GetProcessHeap(), 0, geometry->vertices);
    HeapFree(GetProcessHeap(), 0, geometry);
}

static void d2d_geometry_init(struct d2d_geometry *geometry, const struct ID2D1GeometryVtbl *vtbl)
{
    geometry->ID2D1Geometry_iface.lpVtbl = vtbl;
    geometry->refcount = 1;
}

static inline struct d2d_geometry *impl_from_ID2D1GeometrySink(ID2D1GeometrySink *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_geometry, u.path.ID2D1GeometrySink_iface);
}

static HRESULT STDMETHODCALLTYPE d2d_geometry_sink_QueryInterface(ID2D1GeometrySink *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID2D1GeometrySink)
            || IsEqualGUID(iid, &IID_ID2D1SimplifiedGeometrySink)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        ID2D1GeometrySink_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d2d_geometry_sink_AddRef(ID2D1GeometrySink *iface)
{
    struct d2d_geometry *geometry = impl_from_ID2D1GeometrySink(iface);

    TRACE("iface %p.\n", iface);

    return ID2D1Geometry_AddRef(&geometry->ID2D1Geometry_iface);
}

static ULONG STDMETHODCALLTYPE d2d_geometry_sink_Release(ID2D1GeometrySink *iface)
{
    struct d2d_geometry *geometry = impl_from_ID2D1GeometrySink(iface);

    TRACE("iface %p.\n", iface);

    return ID2D1Geometry_Release(&geometry->ID2D1Geometry_iface);
}

static void STDMETHODCALLTYPE d2d_geometry_sink_SetFillMode(ID2D1GeometrySink *iface, D2D1_FILL_MODE mode)
{
    FIXME("iface %p, mode %#x stub!\n", iface, mode);
}

static void STDMETHODCALLTYPE d2d_geometry_sink_SetSegmentFlags(ID2D1GeometrySink *iface, D2D1_PATH_SEGMENT flags)
{
    FIXME("iface %p, flags %#x stub!\n", iface, flags);
}

static void STDMETHODCALLTYPE d2d_geometry_sink_BeginFigure(ID2D1GeometrySink *iface,
        D2D1_POINT_2F start_point, D2D1_FIGURE_BEGIN figure_begin)
{
    struct d2d_geometry *geometry = impl_from_ID2D1GeometrySink(iface);

    FIXME("iface %p, start_point {%.8e, %.8e}, figure_begin %#x stub!\n",
            iface, start_point.x, start_point.y, figure_begin);

    if (geometry->u.path.state != D2D_GEOMETRY_STATE_OPEN)
    {
        geometry->u.path.state = D2D_GEOMETRY_STATE_ERROR;
        return;
    }
    geometry->u.path.state = D2D_GEOMETRY_STATE_FIGURE;
    ++geometry->u.path.figure_count;
    ++geometry->u.path.segment_count;
}

static void STDMETHODCALLTYPE d2d_geometry_sink_AddLines(ID2D1GeometrySink *iface,
        const D2D1_POINT_2F *points, UINT32 count)
{
    struct d2d_geometry *geometry = impl_from_ID2D1GeometrySink(iface);

    FIXME("iface %p, points %p, count %u stub!\n", iface, points, count);

    if (geometry->u.path.state != D2D_GEOMETRY_STATE_FIGURE)
    {
        geometry->u.path.state = D2D_GEOMETRY_STATE_ERROR;
        return;
    }

    geometry->u.path.segment_count += count;
}

static void STDMETHODCALLTYPE d2d_geometry_sink_AddBeziers(ID2D1GeometrySink *iface,
        const D2D1_BEZIER_SEGMENT *beziers, UINT32 count)
{
    struct d2d_geometry *geometry = impl_from_ID2D1GeometrySink(iface);

    FIXME("iface %p, beziers %p, count %u stub!\n", iface, beziers, count);

    if (geometry->u.path.state != D2D_GEOMETRY_STATE_FIGURE)
    {
        geometry->u.path.state = D2D_GEOMETRY_STATE_ERROR;
        return;
    }

    geometry->u.path.segment_count += count;
}

static void STDMETHODCALLTYPE d2d_geometry_sink_EndFigure(ID2D1GeometrySink *iface, D2D1_FIGURE_END figure_end)
{
    struct d2d_geometry *geometry = impl_from_ID2D1GeometrySink(iface);

    FIXME("iface %p, figure_end %#x stub!\n", iface, figure_end);

    if (geometry->u.path.state != D2D_GEOMETRY_STATE_FIGURE)
    {
        geometry->u.path.state = D2D_GEOMETRY_STATE_ERROR;
        return;
    }
    geometry->u.path.state = D2D_GEOMETRY_STATE_OPEN;
}

static HRESULT STDMETHODCALLTYPE d2d_geometry_sink_Close(ID2D1GeometrySink *iface)
{
    struct d2d_geometry *geometry = impl_from_ID2D1GeometrySink(iface);

    TRACE("iface %p.\n", iface);

    if (geometry->u.path.state != D2D_GEOMETRY_STATE_OPEN)
    {
        if (geometry->u.path.state != D2D_GEOMETRY_STATE_CLOSED)
            geometry->u.path.state = D2D_GEOMETRY_STATE_ERROR;
        return D2DERR_WRONG_STATE;
    }
    geometry->u.path.state = D2D_GEOMETRY_STATE_CLOSED;

    return S_OK;
}

static void STDMETHODCALLTYPE d2d_geometry_sink_AddLine(ID2D1GeometrySink *iface, D2D1_POINT_2F point)
{
    TRACE("iface %p, point {%.8e, %.8e}.\n", iface, point.x, point.y);

    d2d_geometry_sink_AddLines(iface, &point, 1);
}

static void STDMETHODCALLTYPE d2d_geometry_sink_AddBezier(ID2D1GeometrySink *iface, const D2D1_BEZIER_SEGMENT *bezier)
{
    TRACE("iface %p, bezier %p.\n", iface, bezier);

    d2d_geometry_sink_AddBeziers(iface, bezier, 1);
}

static void STDMETHODCALLTYPE d2d_geometry_sink_AddQuadraticBezier(ID2D1GeometrySink *iface,
        const D2D1_QUADRATIC_BEZIER_SEGMENT *bezier)
{
    TRACE("iface %p, bezier %p.\n", iface, bezier);

    ID2D1GeometrySink_AddQuadraticBeziers(iface, bezier, 1);
}

static void STDMETHODCALLTYPE d2d_geometry_sink_AddQuadraticBeziers(ID2D1GeometrySink *iface,
        const D2D1_QUADRATIC_BEZIER_SEGMENT *beziers, UINT32 bezier_count)
{
    struct d2d_geometry *geometry = impl_from_ID2D1GeometrySink(iface);

    FIXME("iface %p, beziers %p, bezier_count %u stub!\n", iface, beziers, bezier_count);

    if (geometry->u.path.state != D2D_GEOMETRY_STATE_FIGURE)
    {
        geometry->u.path.state = D2D_GEOMETRY_STATE_ERROR;
        return;
    }

    geometry->u.path.segment_count += bezier_count;
}

static void STDMETHODCALLTYPE d2d_geometry_sink_AddArc(ID2D1GeometrySink *iface, const D2D1_ARC_SEGMENT *arc)
{
    struct d2d_geometry *geometry = impl_from_ID2D1GeometrySink(iface);

    FIXME("iface %p, arc %p stub!\n", iface, arc);

    if (geometry->u.path.state != D2D_GEOMETRY_STATE_FIGURE)
    {
        geometry->u.path.state = D2D_GEOMETRY_STATE_ERROR;
        return;
    }

    ++geometry->u.path.segment_count;
}

struct ID2D1GeometrySinkVtbl d2d_geometry_sink_vtbl =
{
    d2d_geometry_sink_QueryInterface,
    d2d_geometry_sink_AddRef,
    d2d_geometry_sink_Release,
    d2d_geometry_sink_SetFillMode,
    d2d_geometry_sink_SetSegmentFlags,
    d2d_geometry_sink_BeginFigure,
    d2d_geometry_sink_AddLines,
    d2d_geometry_sink_AddBeziers,
    d2d_geometry_sink_EndFigure,
    d2d_geometry_sink_Close,
    d2d_geometry_sink_AddLine,
    d2d_geometry_sink_AddBezier,
    d2d_geometry_sink_AddQuadraticBezier,
    d2d_geometry_sink_AddQuadraticBeziers,
    d2d_geometry_sink_AddArc,
};

static inline struct d2d_geometry *impl_from_ID2D1PathGeometry(ID2D1PathGeometry *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_geometry, ID2D1Geometry_iface);
}

static HRESULT STDMETHODCALLTYPE d2d_path_geometry_QueryInterface(ID2D1PathGeometry *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID2D1PathGeometry)
            || IsEqualGUID(iid, &IID_ID2D1Geometry)
            || IsEqualGUID(iid, &IID_ID2D1Resource)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        ID2D1PathGeometry_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d2d_path_geometry_AddRef(ID2D1PathGeometry *iface)
{
    struct d2d_geometry *geometry = impl_from_ID2D1PathGeometry(iface);
    ULONG refcount = InterlockedIncrement(&geometry->refcount);

    TRACE("%p increasing refcount to %u.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d2d_path_geometry_Release(ID2D1PathGeometry *iface)
{
    struct d2d_geometry *geometry = impl_from_ID2D1PathGeometry(iface);
    ULONG refcount = InterlockedDecrement(&geometry->refcount);

    TRACE("%p decreasing refcount to %u.\n", iface, refcount);

    if (!refcount)
        d2d_geometry_destroy(geometry);

    return refcount;
}

static void STDMETHODCALLTYPE d2d_path_geometry_GetFactory(ID2D1PathGeometry *iface, ID2D1Factory **factory)
{
    FIXME("iface %p, factory %p stub!\n", iface, factory);

    *factory = NULL;
}

static HRESULT STDMETHODCALLTYPE d2d_path_geometry_GetBounds(ID2D1PathGeometry *iface,
        const D2D1_MATRIX_3X2_F *transform, D2D1_RECT_F *bounds)
{
    FIXME("iface %p, transform %p, bounds %p stub!\n", iface, transform, bounds);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_path_geometry_GetWidenedBounds(ID2D1PathGeometry *iface, float stroke_width,
        ID2D1StrokeStyle *stroke_style, const D2D1_MATRIX_3X2_F *transform, float tolerance, D2D1_RECT_F *bounds)
{
    FIXME("iface %p, stroke_width %.8e, stroke_style %p, transform %p, tolerance %.8e, bounds %p stub!\n",
            iface, stroke_width, stroke_style, transform, tolerance, bounds);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_path_geometry_StrokeContainsPoint(ID2D1PathGeometry *iface,
        D2D1_POINT_2F point, float stroke_width, ID2D1StrokeStyle *stroke_style, const D2D1_MATRIX_3X2_F *transform,
        float tolerance, BOOL *contains)
{
    FIXME("iface %p, point {%.8e, %.8e}, stroke_width %.8e, stroke_style %p, "
            "transform %p, tolerance %.8e, contains %p stub!\n",
            iface, point.x, point.y, stroke_width, stroke_style, transform, tolerance, contains);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_path_geometry_FillContainsPoint(ID2D1PathGeometry *iface,
        D2D1_POINT_2F point, const D2D1_MATRIX_3X2_F *transform, float tolerance, BOOL *contains)
{
    FIXME("iface %p, point {%.8e, %.8e}, transform %p, tolerance %.8e, contains %p stub!\n",
            iface, point.x, point.y, transform, tolerance, contains);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_path_geometry_CompareWithGeometry(ID2D1PathGeometry *iface,
        ID2D1Geometry *geometry, const D2D1_MATRIX_3X2_F *transform, float tolerance, D2D1_GEOMETRY_RELATION *relation)
{
    FIXME("iface %p, geometry %p, transform %p, tolerance %.8e, relation %p stub!\n",
            iface, geometry, transform, tolerance, relation);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_path_geometry_Simplify(ID2D1PathGeometry *iface,
        D2D1_GEOMETRY_SIMPLIFICATION_OPTION option, const D2D1_MATRIX_3X2_F *transform, float tolerance,
        ID2D1SimplifiedGeometrySink *sink)
{
    FIXME("iface %p, option %#x, transform %p, tolerance %.8e, sink %p stub!\n",
            iface, option, transform, tolerance, sink);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_path_geometry_Tessellate(ID2D1PathGeometry *iface,
        const D2D1_MATRIX_3X2_F *transform, float tolerance, ID2D1TessellationSink *sink)
{
    FIXME("iface %p, transform %p, tolerance %.8e, sink %p stub!\n", iface, transform, tolerance, sink);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_path_geometry_CombineWithGeometry(ID2D1PathGeometry *iface,
        ID2D1Geometry *geometry, D2D1_COMBINE_MODE combine_mode, const D2D1_MATRIX_3X2_F *transform,
        float tolerance, ID2D1SimplifiedGeometrySink *sink)
{
    FIXME("iface %p, geometry %p, combine_mode %#x, transform %p, tolerance %.8e, sink %p stub!\n",
            iface, geometry, combine_mode, transform, tolerance, sink);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_path_geometry_Outline(ID2D1PathGeometry *iface,
        const D2D1_MATRIX_3X2_F *transform, float tolerance, ID2D1SimplifiedGeometrySink *sink)
{
    FIXME("iface %p, transform %p, tolerance %.8e, sink %p stub!\n", iface, transform, tolerance, sink);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_path_geometry_ComputeArea(ID2D1PathGeometry *iface,
        const D2D1_MATRIX_3X2_F *transform, float tolerance, float *area)
{
    FIXME("iface %p, transform %p, tolerance %.8e, area %p stub!\n", iface, transform, tolerance, area);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_path_geometry_ComputeLength(ID2D1PathGeometry *iface,
        const D2D1_MATRIX_3X2_F *transform, float tolerance, float *length)
{
    FIXME("iface %p, transform %p, tolerance %.8e, length %p stub!\n", iface, transform, tolerance, length);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_path_geometry_ComputePointAtLength(ID2D1PathGeometry *iface, float length,
        const D2D1_MATRIX_3X2_F *transform, float tolerance, D2D1_POINT_2F *point, D2D1_POINT_2F *tangent)
{
    FIXME("iface %p, length %.8e, transform %p, tolerance %.8e, point %p, tangent %p stub!\n",
            iface, length, transform, tolerance, point, tangent);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_path_geometry_Widen(ID2D1PathGeometry *iface, float stroke_width,
        ID2D1StrokeStyle *stroke_style, const D2D1_MATRIX_3X2_F *transform, float tolerance,
        ID2D1SimplifiedGeometrySink *sink)
{
    FIXME("iface %p, stroke_width %.8e, stroke_style %p, transform %p, tolerance %.8e, sink %p stub!\n",
            iface, stroke_width, stroke_style, transform, tolerance, sink);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_path_geometry_Open(ID2D1PathGeometry *iface, ID2D1GeometrySink **sink)
{
    struct d2d_geometry *geometry = impl_from_ID2D1PathGeometry(iface);

    TRACE("iface %p, sink %p.\n", iface, sink);

    if (geometry->u.path.state != D2D_GEOMETRY_STATE_INITIAL)
        return D2DERR_WRONG_STATE;

    *sink = &geometry->u.path.ID2D1GeometrySink_iface;
    ID2D1GeometrySink_AddRef(*sink);

    geometry->u.path.state = D2D_GEOMETRY_STATE_OPEN;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d2d_path_geometry_Stream(ID2D1PathGeometry *iface, ID2D1GeometrySink *sink)
{
    FIXME("iface %p, sink %p stub!\n", iface, sink);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_path_geometry_GetSegmentCount(ID2D1PathGeometry *iface, UINT32 *count)
{
    struct d2d_geometry *geometry = impl_from_ID2D1PathGeometry(iface);

    TRACE("iface %p, count %p.\n", iface, count);

    if (geometry->u.path.state != D2D_GEOMETRY_STATE_CLOSED)
        return D2DERR_WRONG_STATE;

    *count = geometry->u.path.segment_count;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d2d_path_geometry_GetFigureCount(ID2D1PathGeometry *iface, UINT32 *count)
{
    struct d2d_geometry *geometry = impl_from_ID2D1PathGeometry(iface);

    TRACE("iface %p, count %p.\n", iface, count);

    if (geometry->u.path.state != D2D_GEOMETRY_STATE_CLOSED)
        return D2DERR_WRONG_STATE;

    *count = geometry->u.path.figure_count;

    return S_OK;
}

static const struct ID2D1PathGeometryVtbl d2d_path_geometry_vtbl =
{
    d2d_path_geometry_QueryInterface,
    d2d_path_geometry_AddRef,
    d2d_path_geometry_Release,
    d2d_path_geometry_GetFactory,
    d2d_path_geometry_GetBounds,
    d2d_path_geometry_GetWidenedBounds,
    d2d_path_geometry_StrokeContainsPoint,
    d2d_path_geometry_FillContainsPoint,
    d2d_path_geometry_CompareWithGeometry,
    d2d_path_geometry_Simplify,
    d2d_path_geometry_Tessellate,
    d2d_path_geometry_CombineWithGeometry,
    d2d_path_geometry_Outline,
    d2d_path_geometry_ComputeArea,
    d2d_path_geometry_ComputeLength,
    d2d_path_geometry_ComputePointAtLength,
    d2d_path_geometry_Widen,
    d2d_path_geometry_Open,
    d2d_path_geometry_Stream,
    d2d_path_geometry_GetSegmentCount,
    d2d_path_geometry_GetFigureCount,
};

void d2d_path_geometry_init(struct d2d_geometry *geometry)
{
    d2d_geometry_init(geometry, (ID2D1GeometryVtbl *)&d2d_path_geometry_vtbl);
    geometry->u.path.ID2D1GeometrySink_iface.lpVtbl = &d2d_geometry_sink_vtbl;
}

static inline struct d2d_geometry *impl_from_ID2D1RectangleGeometry(ID2D1RectangleGeometry *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_geometry, ID2D1Geometry_iface);
}

static HRESULT STDMETHODCALLTYPE d2d_rectangle_geometry_QueryInterface(ID2D1RectangleGeometry *iface,
        REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID2D1RectangleGeometry)
            || IsEqualGUID(iid, &IID_ID2D1Geometry)
            || IsEqualGUID(iid, &IID_ID2D1Resource)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        ID2D1RectangleGeometry_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d2d_rectangle_geometry_AddRef(ID2D1RectangleGeometry *iface)
{
    struct d2d_geometry *geometry = impl_from_ID2D1RectangleGeometry(iface);
    ULONG refcount = InterlockedIncrement(&geometry->refcount);

    TRACE("%p increasing refcount to %u.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d2d_rectangle_geometry_Release(ID2D1RectangleGeometry *iface)
{
    struct d2d_geometry *geometry = impl_from_ID2D1RectangleGeometry(iface);
    ULONG refcount = InterlockedDecrement(&geometry->refcount);

    TRACE("%p decreasing refcount to %u.\n", iface, refcount);

    if (!refcount)
        d2d_geometry_destroy(geometry);

    return refcount;
}

static void STDMETHODCALLTYPE d2d_rectangle_geometry_GetFactory(ID2D1RectangleGeometry *iface, ID2D1Factory **factory)
{
    FIXME("iface %p, factory %p stub!\n", iface, factory);

    *factory = NULL;
}

static HRESULT STDMETHODCALLTYPE d2d_rectangle_geometry_GetBounds(ID2D1RectangleGeometry *iface,
        const D2D1_MATRIX_3X2_F *transform, D2D1_RECT_F *bounds)
{
    FIXME("iface %p, transform %p, bounds %p stub!\n", iface, transform, bounds);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_rectangle_geometry_GetWidenedBounds(ID2D1RectangleGeometry *iface,
        float stroke_width, ID2D1StrokeStyle *stroke_style, const D2D1_MATRIX_3X2_F *transform,
        float tolerance, D2D1_RECT_F *bounds)
{
    FIXME("iface %p, stroke_width %.8e, stroke_style %p, transform %p, tolerance %.8e, bounds %p stub!\n",
            iface, stroke_width, stroke_style, transform, tolerance, bounds);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_rectangle_geometry_StrokeContainsPoint(ID2D1RectangleGeometry *iface,
        D2D1_POINT_2F point, float stroke_width, ID2D1StrokeStyle *stroke_style, const D2D1_MATRIX_3X2_F *transform,
        float tolerance, BOOL *contains)
{
    FIXME("iface %p, point {%.8e, %.8e}, stroke_width %.8e, stroke_style %p, "
            "transform %p, tolerance %.8e, contains %p stub!\n",
            iface, point.x, point.y, stroke_width, stroke_style, transform, tolerance, contains);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_rectangle_geometry_FillContainsPoint(ID2D1RectangleGeometry *iface,
        D2D1_POINT_2F point, const D2D1_MATRIX_3X2_F *transform, float tolerance, BOOL *contains)
{
    FIXME("iface %p, point {%.8e, %.8e}, transform %p, tolerance %.8e, contains %p stub!\n",
            iface, point.x, point.y, transform, tolerance, contains);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_rectangle_geometry_CompareWithGeometry(ID2D1RectangleGeometry *iface,
        ID2D1Geometry *geometry, const D2D1_MATRIX_3X2_F *transform, float tolerance, D2D1_GEOMETRY_RELATION *relation)
{
    FIXME("iface %p, geometry %p, transform %p, tolerance %.8e, relation %p stub!\n",
            iface, geometry, transform, tolerance, relation);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_rectangle_geometry_Simplify(ID2D1RectangleGeometry *iface,
        D2D1_GEOMETRY_SIMPLIFICATION_OPTION option, const D2D1_MATRIX_3X2_F *transform, float tolerance,
        ID2D1SimplifiedGeometrySink *sink)
{
    FIXME("iface %p, option %#x, transform %p, tolerance %.8e, sink %p stub!\n",
            iface, option, transform, tolerance, sink);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_rectangle_geometry_Tessellate(ID2D1RectangleGeometry *iface,
        const D2D1_MATRIX_3X2_F *transform, float tolerance, ID2D1TessellationSink *sink)
{
    FIXME("iface %p, transform %p, tolerance %.8e, sink %p stub!\n", iface, transform, tolerance, sink);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_rectangle_geometry_CombineWithGeometry(ID2D1RectangleGeometry *iface,
        ID2D1Geometry *geometry, D2D1_COMBINE_MODE combine_mode, const D2D1_MATRIX_3X2_F *transform,
        float tolerance, ID2D1SimplifiedGeometrySink *sink)
{
    FIXME("iface %p, geometry %p, combine_mode %#x, transform %p, tolerance %.8e, sink %p stub!\n",
            iface, geometry, combine_mode, transform, tolerance, sink);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_rectangle_geometry_Outline(ID2D1RectangleGeometry *iface,
        const D2D1_MATRIX_3X2_F *transform, float tolerance, ID2D1SimplifiedGeometrySink *sink)
{
    FIXME("iface %p, transform %p, tolerance %.8e, sink %p stub!\n", iface, transform, tolerance, sink);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_rectangle_geometry_ComputeArea(ID2D1RectangleGeometry *iface,
        const D2D1_MATRIX_3X2_F *transform, float tolerance, float *area)
{
    FIXME("iface %p, transform %p, tolerance %.8e, area %p stub!\n", iface, transform, tolerance, area);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_rectangle_geometry_ComputeLength(ID2D1RectangleGeometry *iface,
        const D2D1_MATRIX_3X2_F *transform, float tolerance, float *length)
{
    FIXME("iface %p, transform %p, tolerance %.8e, length %p stub!\n", iface, transform, tolerance, length);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_rectangle_geometry_ComputePointAtLength(ID2D1RectangleGeometry *iface,
        float length, const D2D1_MATRIX_3X2_F *transform, float tolerance, D2D1_POINT_2F *point,
        D2D1_POINT_2F *tangent)
{
    FIXME("iface %p, length %.8e, transform %p, tolerance %.8e, point %p, tangent %p stub!\n",
            iface, length, transform, tolerance, point, tangent);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_rectangle_geometry_Widen(ID2D1RectangleGeometry *iface, float stroke_width,
        ID2D1StrokeStyle *stroke_style, const D2D1_MATRIX_3X2_F *transform, float tolerance,
        ID2D1SimplifiedGeometrySink *sink)
{
    FIXME("iface %p, stroke_width %.8e, stroke_style %p, transform %p, tolerance %.8e, sink %p stub!\n",
            iface, stroke_width, stroke_style, transform, tolerance, sink);

    return E_NOTIMPL;
}

static void STDMETHODCALLTYPE d2d_rectangle_geometry_GetRect(ID2D1RectangleGeometry *iface, D2D1_RECT_F *rect)
{
    struct d2d_geometry *geometry = impl_from_ID2D1RectangleGeometry(iface);

    TRACE("iface %p, rect %p.\n", iface, rect);

    *rect = geometry->u.rectangle.rect;
}

static const struct ID2D1RectangleGeometryVtbl d2d_rectangle_geometry_vtbl =
{
    d2d_rectangle_geometry_QueryInterface,
    d2d_rectangle_geometry_AddRef,
    d2d_rectangle_geometry_Release,
    d2d_rectangle_geometry_GetFactory,
    d2d_rectangle_geometry_GetBounds,
    d2d_rectangle_geometry_GetWidenedBounds,
    d2d_rectangle_geometry_StrokeContainsPoint,
    d2d_rectangle_geometry_FillContainsPoint,
    d2d_rectangle_geometry_CompareWithGeometry,
    d2d_rectangle_geometry_Simplify,
    d2d_rectangle_geometry_Tessellate,
    d2d_rectangle_geometry_CombineWithGeometry,
    d2d_rectangle_geometry_Outline,
    d2d_rectangle_geometry_ComputeArea,
    d2d_rectangle_geometry_ComputeLength,
    d2d_rectangle_geometry_ComputePointAtLength,
    d2d_rectangle_geometry_Widen,
    d2d_rectangle_geometry_GetRect,
};

HRESULT d2d_rectangle_geometry_init(struct d2d_geometry *geometry, const D2D1_RECT_F *rect)
{
    d2d_geometry_init(geometry, (ID2D1GeometryVtbl *)&d2d_rectangle_geometry_vtbl);
    geometry->u.rectangle.rect = *rect;

    if (!(geometry->vertices = HeapAlloc(GetProcessHeap(), 0, 4 * sizeof(*geometry->vertices))))
        return E_OUTOFMEMORY;
    geometry->vertex_count = 4;
    if (!(geometry->faces = HeapAlloc(GetProcessHeap(), 0, 2 * sizeof(*geometry->faces))))
    {
        HeapFree(GetProcessHeap(), 0, geometry->vertices);
        return E_OUTOFMEMORY;
    }
    geometry->face_count = 2;

    geometry->vertices[0].x = min(rect->left, rect->right);
    geometry->vertices[0].y = min(rect->top, rect->bottom);
    geometry->vertices[1].x = min(rect->left, rect->right);
    geometry->vertices[1].y = max(rect->top, rect->bottom);
    geometry->vertices[2].x = max(rect->left, rect->right);
    geometry->vertices[2].y = min(rect->top, rect->bottom);
    geometry->vertices[3].x = max(rect->left, rect->right);
    geometry->vertices[3].y = max(rect->top, rect->bottom);

    geometry->faces[0].v[0] = 0;
    geometry->faces[0].v[1] = 2;
    geometry->faces[0].v[2] = 1;
    geometry->faces[1].v[0] = 1;
    geometry->faces[1].v[1] = 2;
    geometry->faces[1].v[2] = 3;

    return S_OK;
}
