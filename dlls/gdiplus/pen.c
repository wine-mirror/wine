/*
 * Copyright (C) 2007 Google (Evan Stade)
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

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "gdiplus.h"
#include "gdiplus_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdiplus);

static DWORD gdip_to_gdi_dash(GpDashStyle dash)
{
    switch(dash){
        case DashStyleSolid:
            return PS_SOLID;
        case DashStyleDash:
            return PS_DASH;
        case DashStyleDot:
            return PS_DOT;
        case DashStyleDashDot:
            return PS_DASHDOT;
        case DashStyleDashDotDot:
            return PS_DASHDOTDOT;
        case DashStyleCustom:
            FIXME("DashStyleCustom not implemented\n");
            return PS_SOLID;
        default:
            ERR("Not a member of GpDashStyle enumeration\n");
            return 0;
    }
}

static DWORD gdip_to_gdi_join(GpLineJoin join)
{
    switch(join){
        case LineJoinRound:
            return PS_JOIN_ROUND;
        case LineJoinBevel:
            return PS_JOIN_BEVEL;
        case LineJoinMiter:
        case LineJoinMiterClipped:
            return PS_JOIN_MITER;
        default:
            ERR("Not a member of GpLineJoin enumeration\n");
            return 0;
    }
}

GpStatus WINGDIPAPI GdipClonePen(GpPen *pen, GpPen **clonepen)
{
    if(!pen || !clonepen)
        return InvalidParameter;

    *clonepen = GdipAlloc(sizeof(GpPen));
    if(!*clonepen)  return OutOfMemory;

    memcpy(*clonepen, pen, sizeof(GpPen));

    GdipCloneCustomLineCap(pen->customstart, &(*clonepen)->customstart);
    GdipCloneCustomLineCap(pen->customend, &(*clonepen)->customend);
    GdipCloneBrush(pen->brush, &(*clonepen)->brush);

    (*clonepen)->gdipen = ExtCreatePen((*clonepen)->style,
                                       roundr((*clonepen)->width),
                                       &(*clonepen)->brush->lb, 0, NULL);

    return Ok;
}

GpStatus WINGDIPAPI GdipCreatePen1(ARGB color, FLOAT width, GpUnit unit,
    GpPen **pen)
{
    GpPen *gp_pen;

    if(!pen)
        return InvalidParameter;

    gp_pen = GdipAlloc(sizeof(GpPen));
    if(!gp_pen)    return OutOfMemory;

    gp_pen->style = GP_DEFAULT_PENSTYLE;
    gp_pen->color = ARGB2COLORREF(color);
    gp_pen->width = width;
    gp_pen->unit = unit;
    gp_pen->endcap = LineCapFlat;
    gp_pen->join = LineJoinMiter;
    gp_pen->miterlimit = 10.0;
    gp_pen->dash = DashStyleSolid;
    GdipCreateSolidFill(color, (GpSolidFill **)(&gp_pen->brush));

    if((gp_pen->unit == UnitWorld) || (gp_pen->unit == UnitPixel)) {
        gp_pen->gdipen = ExtCreatePen(gp_pen->style, (INT) gp_pen->width,
                                      &gp_pen->brush->lb, 0, NULL);
    } else {
        FIXME("UnitWorld, UnitPixel only supported units\n");
        GdipFree(gp_pen);
        return NotImplemented;
    }

    *pen = gp_pen;

    return Ok;
}

GpStatus WINGDIPAPI GdipDeletePen(GpPen *pen)
{
    if(!pen)    return InvalidParameter;
    DeleteObject(pen->gdipen);

    GdipDeleteBrush(pen->brush);
    GdipDeleteCustomLineCap(pen->customstart);
    GdipDeleteCustomLineCap(pen->customend);
    GdipFree(pen);

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPenDashStyle(GpPen *pen, GpDashStyle *dash)
{
    if(!pen || !dash)
        return InvalidParameter;

    *dash = pen->dash;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetPenCustomEndCap(GpPen *pen, GpCustomLineCap* customCap)
{
    GpCustomLineCap * cap;
    GpStatus ret;

    if(!pen || !customCap) return InvalidParameter;

    if((ret = GdipCloneCustomLineCap(customCap, &cap)) == Ok){
        GdipDeleteCustomLineCap(pen->customend);
        pen->endcap = LineCapCustom;
        pen->customend = cap;
    }

    return ret;
}

GpStatus WINGDIPAPI GdipSetPenCustomStartCap(GpPen *pen, GpCustomLineCap* customCap)
{
    GpCustomLineCap * cap;
    GpStatus ret;

    if(!pen || !customCap) return InvalidParameter;

    if((ret = GdipCloneCustomLineCap(customCap, &cap)) == Ok){
        GdipDeleteCustomLineCap(pen->customstart);
        pen->startcap = LineCapCustom;
        pen->customstart = cap;
    }

    return ret;
}

GpStatus WINGDIPAPI GdipSetPenDashStyle(GpPen *pen, GpDashStyle dash)
{
    if(!pen)
        return InvalidParameter;

    DeleteObject(pen->gdipen);
    pen->dash = dash;
    pen->style &= ~(PS_ALTERNATE | PS_SOLID | PS_DASH | PS_DOT | PS_DASHDOT |
                    PS_DASHDOTDOT | PS_NULL | PS_USERSTYLE | PS_INSIDEFRAME);
    pen->style |= gdip_to_gdi_dash(dash);

    pen->gdipen = ExtCreatePen(pen->style, (INT) pen->width, &pen->brush->lb, 0, NULL);

    return Ok;
}

GpStatus WINGDIPAPI GdipSetPenEndCap(GpPen *pen, GpLineCap cap)
{
    if(!pen)    return InvalidParameter;

    /* The old custom cap gets deleted even if the new style is LineCapCustom. */
    GdipDeleteCustomLineCap(pen->customend);
    pen->customend = NULL;
    pen->endcap = cap;

    return Ok;
}

/* FIXME: startcap, dashcap not used. */
GpStatus WINGDIPAPI GdipSetPenLineCap197819(GpPen *pen, GpLineCap start,
    GpLineCap end, GpDashCap dash)
{
    if(!pen)
        return InvalidParameter;

    GdipDeleteCustomLineCap(pen->customend);
    GdipDeleteCustomLineCap(pen->customstart);
    pen->customend = NULL;
    pen->customstart = NULL;

    pen->startcap = start;
    pen->endcap = end;
    pen->dashcap = dash;

    return Ok;
}

/* FIXME: Miter line joins behave a bit differently than they do in windows.
 * Both kinds of miter joins clip if the angle is less than 11 degrees. */
GpStatus WINGDIPAPI GdipSetPenLineJoin(GpPen *pen, GpLineJoin join)
{
    if(!pen)    return InvalidParameter;

    DeleteObject(pen->gdipen);
    pen->join = join;
    pen->style &= ~(PS_JOIN_ROUND | PS_JOIN_BEVEL | PS_JOIN_MITER);
    pen->style |= gdip_to_gdi_join(join);

    pen->gdipen = ExtCreatePen(pen->style, (INT) pen->width, &pen->brush->lb, 0, NULL);

    return Ok;
}

GpStatus WINGDIPAPI GdipSetPenMiterLimit(GpPen *pen, REAL limit)
{
    if(!pen)
        return InvalidParameter;

    pen->miterlimit = limit;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetPenStartCap(GpPen *pen, GpLineCap cap)
{
    if(!pen)    return InvalidParameter;

    GdipDeleteCustomLineCap(pen->customstart);
    pen->customstart = NULL;
    pen->startcap = cap;

    return Ok;
}
