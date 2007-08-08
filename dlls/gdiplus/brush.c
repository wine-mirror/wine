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

#include "objbase.h"

#include "gdiplus.h"
#include "gdiplus_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdiplus);

GpStatus WINGDIPAPI GdipCloneBrush(GpBrush *brush, GpBrush **clone)
{
    if(!brush || !clone)
        return InvalidParameter;

    switch(brush->bt){
        case BrushTypeSolidColor:
            *clone = GdipAlloc(sizeof(GpSolidFill));
            if (!*clone) return OutOfMemory;

            memcpy(*clone, brush, sizeof(GpSolidFill));

            (*clone)->gdibrush = CreateBrushIndirect(&(*clone)->lb);
            break;
        case BrushTypePathGradient:{
            GpPathGradient *src, *dest;
            INT count;

            *clone = GdipAlloc(sizeof(GpPathGradient));
            if (!*clone) return OutOfMemory;

            src = (GpPathGradient*) brush,
            dest = (GpPathGradient*) *clone;
            count = src->pathdata.Count;

            memcpy(dest, src, sizeof(GpPathGradient));

            dest->pathdata.Count = count;
            dest->pathdata.Points = GdipAlloc(count * sizeof(PointF));
            dest->pathdata.Types = GdipAlloc(count);

            if(!dest->pathdata.Points || !dest->pathdata.Types){
                GdipFree(dest->pathdata.Points);
                GdipFree(dest->pathdata.Types);
                GdipFree(dest);
                return OutOfMemory;
            }

            memcpy(dest->pathdata.Points, src->pathdata.Points, count * sizeof(PointF));
            memcpy(dest->pathdata.Types, src->pathdata.Types, count);

            break;
        }
        default:
            return NotImplemented;
    }

    return Ok;
}

GpStatus WINGDIPAPI GdipCreateLineBrush(GDIPCONST GpPointF* startpoint,
    GDIPCONST GpPointF* endpoint, ARGB startcolor, ARGB endcolor,
    GpWrapMode wrap, GpLineGradient **line)
{
    COLORREF col = ARGB2COLORREF(startcolor);

    if(!line || !startpoint || !endpoint || wrap == WrapModeClamp)
        return InvalidParameter;

    *line = GdipAlloc(sizeof(GpLineGradient));
    if(!*line)  return OutOfMemory;

    (*line)->brush.lb.lbStyle = BS_SOLID;
    (*line)->brush.lb.lbColor = col;
    (*line)->brush.lb.lbHatch = 0;
    (*line)->brush.gdibrush = CreateSolidBrush(col);
    (*line)->brush.bt = BrushTypeLinearGradient;

    (*line)->startpoint.X = startpoint->X;
    (*line)->startpoint.Y = startpoint->Y;
    (*line)->endpoint.X = endpoint->X;
    (*line)->endpoint.Y = endpoint->Y;
    (*line)->startcolor = startcolor;
    (*line)->endcolor = endcolor;
    (*line)->wrap = wrap;
    (*line)->gamma = FALSE;

    return Ok;
}

GpStatus WINGDIPAPI GdipCreatePathGradient(GDIPCONST GpPointF* points,
    INT count, GpWrapMode wrap, GpPathGradient **grad)
{
    COLORREF col = ARGB2COLORREF(0xffffffff);

    if(!points || !grad)
        return InvalidParameter;

    if(count <= 0)
        return OutOfMemory;

    *grad = GdipAlloc(sizeof(GpPathGradient));
    if (!*grad) return OutOfMemory;

    (*grad)->pathdata.Count = count;
    (*grad)->pathdata.Points = GdipAlloc(count * sizeof(PointF));
    (*grad)->pathdata.Types = GdipAlloc(count);

    if(!(*grad)->pathdata.Points || !(*grad)->pathdata.Types){
        GdipFree((*grad)->pathdata.Points);
        GdipFree((*grad)->pathdata.Types);
        GdipFree(*grad);
        return OutOfMemory;
    }

    memcpy((*grad)->pathdata.Points, points, count * sizeof(PointF));
    memset((*grad)->pathdata.Types, PathPointTypeLine, count);

    (*grad)->brush.lb.lbStyle = BS_SOLID;
    (*grad)->brush.lb.lbColor = col;
    (*grad)->brush.lb.lbHatch = 0;

    (*grad)->brush.gdibrush = CreateSolidBrush(col);
    (*grad)->brush.bt = BrushTypePathGradient;
    (*grad)->centercolor = 0xffffffff;
    (*grad)->wrap = wrap;
    (*grad)->gamma = FALSE;
    (*grad)->center.X = 0.0;
    (*grad)->center.Y = 0.0;
    (*grad)->focus.X = 0.0;
    (*grad)->focus.Y = 0.0;

    return Ok;
}

/* FIXME: path gradient brushes not truly supported (drawn as solid brushes) */
GpStatus WINGDIPAPI GdipCreatePathGradientFromPath(GDIPCONST GpPath* path,
    GpPathGradient **grad)
{
    COLORREF col = ARGB2COLORREF(0xffffffff);

    if(!path || !grad)
        return InvalidParameter;

    *grad = GdipAlloc(sizeof(GpPathGradient));
    if (!*grad) return OutOfMemory;

    (*grad)->pathdata.Count = path->pathdata.Count;
    (*grad)->pathdata.Points = GdipAlloc(path->pathdata.Count * sizeof(PointF));
    (*grad)->pathdata.Types = GdipAlloc(path->pathdata.Count);

    if(!(*grad)->pathdata.Points || !(*grad)->pathdata.Types){
        GdipFree((*grad)->pathdata.Points);
        GdipFree((*grad)->pathdata.Types);
        GdipFree(*grad);
        return OutOfMemory;
    }

    memcpy((*grad)->pathdata.Points, path->pathdata.Points,
           path->pathdata.Count * sizeof(PointF));
    memcpy((*grad)->pathdata.Types, path->pathdata.Types, path->pathdata.Count);

    (*grad)->brush.lb.lbStyle = BS_SOLID;
    (*grad)->brush.lb.lbColor = col;
    (*grad)->brush.lb.lbHatch = 0;

    (*grad)->brush.gdibrush = CreateSolidBrush(col);
    (*grad)->brush.bt = BrushTypePathGradient;
    (*grad)->centercolor = 0xffffffff;
    (*grad)->wrap = WrapModeClamp;
    (*grad)->gamma = FALSE;
    /* FIXME: this should be set to the "centroid" of the path by default */
    (*grad)->center.X = 0.0;
    (*grad)->center.Y = 0.0;
    (*grad)->focus.X = 0.0;
    (*grad)->focus.Y = 0.0;

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
    (*sf)->color = color;

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

    switch(brush->bt)
    {
        case BrushTypePathGradient:
            GdipFree(((GpPathGradient*) brush)->pathdata.Points);
            GdipFree(((GpPathGradient*) brush)->pathdata.Types);
            break;
        case BrushTypeSolidColor:
        default:
            break;
    }

    DeleteObject(brush->gdibrush);
    GdipFree(brush);

    return Ok;
}

GpStatus WINGDIPAPI GdipGetLineGammaCorrection(GpLineGradient *line,
    BOOL *usinggamma)
{
    if(!line)
        return InvalidParameter;

    *usinggamma = line->gamma;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPathGradientCenterPoint(GpPathGradient *grad,
    GpPointF *point)
{
    if(!grad || !point)
        return InvalidParameter;

    point->X = grad->center.X;
    point->Y = grad->center.Y;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPathGradientFocusScales(GpPathGradient *grad,
    REAL *x, REAL *y)
{
    if(!grad || !x || !y)
        return InvalidParameter;

    *x = grad->focus.X;
    *y = grad->focus.Y;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPathGradientGammaCorrection(GpPathGradient *grad,
    BOOL *gamma)
{
    if(!grad || !gamma)
        return InvalidParameter;

    *gamma = grad->gamma;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPathGradientPointCount(GpPathGradient *grad,
    INT *count)
{
    if(!grad || !count)
        return InvalidParameter;

    *count = grad->pathdata.Count;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPathGradientSurroundColorsWithCount(GpPathGradient
    *grad, ARGB *argb, INT *count)
{
    static int calls;

    if(!grad || !argb || !count || (*count < grad->pathdata.Count))
        return InvalidParameter;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipGetSolidFillColor(GpSolidFill *sf, ARGB *argb)
{
    if(!sf || !argb)
        return InvalidParameter;

    *argb = sf->color;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetLineGammaCorrection(GpLineGradient *line,
    BOOL usegamma)
{
    if(!line)
        return InvalidParameter;

    line->gamma = usegamma;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetLineSigmaBlend(GpLineGradient *line, REAL focus,
    REAL scale)
{
    static int calls;

    if(!line || focus < 0.0 || focus > 1.0 || scale < 0.0 || scale > 1.0)
        return InvalidParameter;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipSetLineWrapMode(GpLineGradient *line,
    GpWrapMode wrap)
{
    if(!line || wrap == WrapModeClamp)
        return InvalidParameter;

    line->wrap = wrap;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetPathGradientCenterColor(GpPathGradient *grad,
    ARGB argb)
{
    if(!grad)
        return InvalidParameter;

    grad->centercolor = argb;
    grad->brush.lb.lbColor = ARGB2COLORREF(argb);

    DeleteObject(grad->brush.gdibrush);
    grad->brush.gdibrush = CreateSolidBrush(grad->brush.lb.lbColor);

    return Ok;
}

GpStatus WINGDIPAPI GdipSetPathGradientCenterPoint(GpPathGradient *grad,
    GpPointF *point)
{
    if(!grad || !point)
        return InvalidParameter;

    grad->center.X = point->X;
    grad->center.Y = point->Y;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetPathGradientFocusScales(GpPathGradient *grad,
    REAL x, REAL y)
{
    if(!grad)
        return InvalidParameter;

    grad->focus.X = x;
    grad->focus.Y = y;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetPathGradientGammaCorrection(GpPathGradient *grad,
    BOOL gamma)
{
    if(!grad)
        return InvalidParameter;

    grad->gamma = gamma;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetPathGradientSigmaBlend(GpPathGradient *grad,
    REAL focus, REAL scale)
{
    static int calls;

    if(!grad || focus < 0.0 || focus > 1.0 || scale < 0.0 || scale > 1.0)
        return InvalidParameter;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipSetPathGradientSurroundColorsWithCount(GpPathGradient
    *grad, ARGB *argb, INT *count)
{
    static int calls;

    if(!grad || !argb || !count || (*count <= 0) ||
        (*count > grad->pathdata.Count))
        return InvalidParameter;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipSetPathGradientWrapMode(GpPathGradient *grad,
    GpWrapMode wrap)
{
    if(!grad)
        return InvalidParameter;

    grad->wrap = wrap;

    return Ok;
}

GpStatus WINGDIPAPI GdipSetSolidFillColor(GpSolidFill *sf, ARGB argb)
{
    if(!sf)
        return InvalidParameter;

    sf->color = argb;
    sf->brush.lb.lbColor = ARGB2COLORREF(argb);

    DeleteObject(sf->brush.gdibrush);
    sf->brush.gdibrush = CreateSolidBrush(sf->brush.lb.lbColor);

    return Ok;
}
