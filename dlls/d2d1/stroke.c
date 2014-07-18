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

static inline struct d2d_stroke_style *impl_from_ID2D1StrokeStyle(ID2D1StrokeStyle *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_stroke_style, ID2D1StrokeStyle_iface);
}

static HRESULT STDMETHODCALLTYPE d2d_stroke_style_QueryInterface(ID2D1StrokeStyle *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID2D1StrokeStyle)
            || IsEqualGUID(iid, &IID_ID2D1Resource)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        ID2D1StrokeStyle_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d2d_stroke_style_AddRef(ID2D1StrokeStyle *iface)
{
    struct d2d_stroke_style *style = impl_from_ID2D1StrokeStyle(iface);
    ULONG refcount = InterlockedIncrement(&style->refcount);

    TRACE("%p increasing refcount to %u.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d2d_stroke_style_Release(ID2D1StrokeStyle *iface)
{
    struct d2d_stroke_style *style = impl_from_ID2D1StrokeStyle(iface);
    ULONG refcount = InterlockedDecrement(&style->refcount);

    TRACE("%p decreasing refcount to %u.\n", iface, refcount);

    if (!refcount)
        HeapFree(GetProcessHeap(), 0, style);

    return refcount;
}

static void STDMETHODCALLTYPE d2d_stroke_style_GetFactory(ID2D1StrokeStyle *iface, ID2D1Factory **factory)
{
    FIXME("iface %p, factory %p stub!\n", iface, factory);

    *factory = NULL;
}

static D2D1_CAP_STYLE STDMETHODCALLTYPE d2d_stroke_style_GetStartCap(ID2D1StrokeStyle *iface)
{
    FIXME("iface %p stub!\n", iface);

    return D2D1_CAP_STYLE_FLAT;
}

static D2D1_CAP_STYLE STDMETHODCALLTYPE d2d_stroke_style_GetEndCap(ID2D1StrokeStyle *iface)
{
    FIXME("iface %p stub!\n", iface);

    return D2D1_CAP_STYLE_FLAT;
}

static D2D1_CAP_STYLE STDMETHODCALLTYPE d2d_stroke_style_GetDashCap(ID2D1StrokeStyle *iface)
{
    FIXME("iface %p stub!\n", iface);

    return D2D1_CAP_STYLE_FLAT;
}

static float STDMETHODCALLTYPE d2d_stroke_style_GetMiterLimit(ID2D1StrokeStyle *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0.0f;
}

static D2D1_LINE_JOIN STDMETHODCALLTYPE d2d_stroke_style_GetLineJoin(ID2D1StrokeStyle *iface)
{
    FIXME("iface %p stub!\n", iface);

    return D2D1_LINE_JOIN_MITER;
}

static float STDMETHODCALLTYPE d2d_stroke_style_GetDashOffset(ID2D1StrokeStyle *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0.0f;
}

static D2D1_DASH_STYLE STDMETHODCALLTYPE d2d_stroke_style_GetDashStyle(ID2D1StrokeStyle *iface)
{
    FIXME("iface %p stub!\n", iface);

    return D2D1_DASH_STYLE_SOLID;
}

static UINT32 STDMETHODCALLTYPE d2d_stroke_style_GetDashesCount(ID2D1StrokeStyle *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static void STDMETHODCALLTYPE d2d_stroke_style_GetDashes(ID2D1StrokeStyle *iface, float *dashes, UINT32 count)
{
    FIXME("iface %p, dashes %p, count %u stub!\n", iface, dashes, count);
}

static const struct ID2D1StrokeStyleVtbl d2d_stroke_style_vtbl =
{
    d2d_stroke_style_QueryInterface,
    d2d_stroke_style_AddRef,
    d2d_stroke_style_Release,
    d2d_stroke_style_GetFactory,
    d2d_stroke_style_GetStartCap,
    d2d_stroke_style_GetEndCap,
    d2d_stroke_style_GetDashCap,
    d2d_stroke_style_GetMiterLimit,
    d2d_stroke_style_GetLineJoin,
    d2d_stroke_style_GetDashOffset,
    d2d_stroke_style_GetDashStyle,
    d2d_stroke_style_GetDashesCount,
    d2d_stroke_style_GetDashes,
};

void d2d_stroke_style_init(struct d2d_stroke_style *style, ID2D1Factory *factory,
        const D2D1_STROKE_STYLE_PROPERTIES *desc, const float *dashes, UINT32 dash_count)
{
    FIXME("Ignoring stroke style properties.\n");

    style->ID2D1StrokeStyle_iface.lpVtbl = &d2d_stroke_style_vtbl;
    style->refcount = 1;
}
