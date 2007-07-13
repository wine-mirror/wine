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

GpStatus WINGDIPAPI GdipCreatePen1(ARGB color, FLOAT width, GpUnit unit,
    GpPen **pen)
{
    LOGBRUSH lb;
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

    /* FIXME: Currently only solid lines supported. */
    lb.lbStyle = BS_SOLID;
    lb.lbColor = gp_pen->color;
    lb.lbHatch = 0;

    if((gp_pen->unit == UnitWorld) || (gp_pen->unit == UnitPixel)) {
        gp_pen->gdipen = ExtCreatePen(gp_pen->style, (INT) gp_pen->width, &lb,
            0, NULL);
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
    GdipFree(pen);

    return Ok;
}

GpStatus WINGDIPAPI GdipSetPenEndCap(GpPen *pen, GpLineCap cap)
{
    if(!pen)    return InvalidParameter;

    pen->endcap = cap;

    return Ok;
}

/* FIXME: Miter line joins behave a bit differently than they do in windows.
 * Both kinds of miter joins clip if the angle is less than 11 degrees. */
GpStatus WINGDIPAPI GdipSetPenLineJoin(GpPen *pen, GpLineJoin join)
{
    LOGBRUSH lb;

    if(!pen)    return InvalidParameter;

    DeleteObject(pen->gdipen);
    pen->join = join;
    pen->style &= ~(PS_JOIN_ROUND | PS_JOIN_BEVEL | PS_JOIN_MITER);
    pen->style |= gdip_to_gdi_join(join);

    lb.lbStyle = BS_SOLID;
    lb.lbColor = pen->color;
    lb.lbHatch = 0;

    pen->gdipen = ExtCreatePen(pen->style, (INT) pen->width, &lb, 0, NULL);

    return Ok;
}
