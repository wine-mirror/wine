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

#include "windef.h"
#include "wingdi.h"
#include "gdiplus.h"
#include "gdiplus_private.h"

GpStatus WINGDIPAPI GdipCloneBrush(GpBrush *brush, GpBrush **clone)
{
    if(!brush || !clone)
        return InvalidParameter;

    *clone = GdipAlloc(sizeof(GpBrush));
    if (!*clone) return OutOfMemory;

    memcpy(*clone, brush, sizeof(GpBrush));

    (*clone)->gdibrush = CreateBrushIndirect(&(*clone)->lb);

    return Ok;
}

GpStatus WINGDIPAPI GdipCreateSolidFill(ARGB color, GpSolidFill **sf)
{
    COLORREF col = ARGB2COLORREF(color);

    if(!sf)  return InvalidParameter;

    *sf = GdipAlloc(sizeof(GpSolidFill));
    if (!*sf) return OutOfMemory;

    (*sf)->brush.lb.lbStyle = BS_SOLID;
    (*sf)->brush.lb.lbColor = col;
    (*sf)->brush.lb.lbHatch = 0;

    (*sf)->brush.gdibrush = CreateSolidBrush(col);
    (*sf)->brush.bt = BrushTypeSolidColor;
    (*sf)->brush.color = col;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetBrushType(GpBrush *brush, GpBrushType *type)
{
    if(!brush || !type)  return InvalidParameter;

    *type = brush->bt;

    return Ok;
}

GpStatus WINGDIPAPI GdipDeleteBrush(GpBrush *brush)
{
    if(!brush)  return InvalidParameter;

    DeleteObject(brush->gdibrush);
    GdipFree(brush);

    return Ok;
}
