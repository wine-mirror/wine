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
 *
 */

#include <stdarg.h>
#include <math.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "gdiplus.h"
#include "gdiplus_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdiplus);

/* make sure path has enough space for len more points */
static BOOL lengthen_path(GpPath *path, INT len)
{
    /* initial allocation */
    if(path->datalen == 0){
        path->datalen = len * 2;

        path->pathdata.Points = GdipAlloc(path->datalen * sizeof(PointF));
        if(!path->pathdata.Points)   return FALSE;

        path->pathdata.Types = GdipAlloc(path->datalen);
        if(!path->pathdata.Types){
            GdipFree(path->pathdata.Points);
            return FALSE;
        }
    }
    /* reallocation, double size of arrays */
    else if(path->datalen - path->pathdata.Count < len){
        while(path->datalen - path->pathdata.Count < len)
            path->datalen *= 2;

        path->pathdata.Points = HeapReAlloc(GetProcessHeap(), 0,
            path->pathdata.Points, path->datalen * sizeof(PointF));
        if(!path->pathdata.Points)  return FALSE;

        path->pathdata.Types = HeapReAlloc(GetProcessHeap(), 0,
            path->pathdata.Types, path->datalen);
        if(!path->pathdata.Types)   return FALSE;
    }

    return TRUE;
}

GpStatus WINGDIPAPI GdipAddPathArc(GpPath *path, REAL x1, REAL y1, REAL x2,
    REAL y2, REAL startAngle, REAL sweepAngle)
{
    INT count, old_count, i;

    if(!path)
        return InvalidParameter;

    count = arc2polybezier(NULL, x1, y1, x2, y2, startAngle, sweepAngle);

    if(count == 0)
        return Ok;
    if(!lengthen_path(path, count))
        return OutOfMemory;

    old_count = path->pathdata.Count;
    arc2polybezier(&path->pathdata.Points[old_count], x1, y1, x2, y2,
                   startAngle, sweepAngle);

    for(i = 0; i < count; i++){
        path->pathdata.Types[old_count + i] = PathPointTypeBezier;
    }

    path->pathdata.Types[old_count] =
        (path->newfigure ? PathPointTypeStart : PathPointTypeLine);
    path->newfigure = FALSE;
    path->pathdata.Count += count;

    return Ok;
}

GpStatus WINGDIPAPI GdipAddPathLine2(GpPath *path, GDIPCONST GpPointF *points,
    INT count)
{
    INT i, old_count;

    if(!path || !points)
        return InvalidParameter;

    if(!lengthen_path(path, count))
        return OutOfMemory;

    old_count = path->pathdata.Count;

    for(i = 0; i < count; i++){
        path->pathdata.Points[old_count + i].X = points[i].X;
        path->pathdata.Points[old_count + i].Y = points[i].Y;
        path->pathdata.Types[old_count + i] = PathPointTypeLine;
    }

    if(path->newfigure){
        path->pathdata.Types[old_count] = PathPointTypeStart;
        path->newfigure = FALSE;
    }

    path->pathdata.Count += count;

    return Ok;
}

GpStatus WINGDIPAPI GdipClosePathFigure(GpPath* path)
{
    if(!path)
        return InvalidParameter;

    if(path->pathdata.Count > 0){
        path->pathdata.Types[path->pathdata.Count - 1] |= PathPointTypeCloseSubpath;
        path->newfigure = TRUE;
    }

    return Ok;
}

GpStatus WINGDIPAPI GdipClosePathFigures(GpPath* path)
{
    INT i;

    if(!path)
        return InvalidParameter;

    for(i = 1; i < path->pathdata.Count; i++){
        if(path->pathdata.Types[i] == PathPointTypeStart)
            path->pathdata.Types[i-1] |= PathPointTypeCloseSubpath;
    }

    path->newfigure = TRUE;

    return Ok;
}

GpStatus WINGDIPAPI GdipCreatePath(GpFillMode fill, GpPath **path)
{
    if(!path)
        return InvalidParameter;

    *path = GdipAlloc(sizeof(GpPath));
    if(!*path)  return OutOfMemory;

    (*path)->fill = fill;
    (*path)->newfigure = TRUE;

    return Ok;
}

GpStatus WINGDIPAPI GdipDeletePath(GpPath *path)
{
    if(!path)
        return InvalidParameter;

    GdipFree(path->pathdata.Points);
    GdipFree(path->pathdata.Types);
    GdipFree(path);

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPathPoints(GpPath *path, GpPointF* points, INT count)
{
    if(!path)
        return InvalidParameter;

    if(count < path->pathdata.Count)
        return InsufficientBuffer;

    memcpy(points, path->pathdata.Points, path->pathdata.Count * sizeof(GpPointF));

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPathTypes(GpPath *path, BYTE* types, INT count)
{
    if(!path)
        return InvalidParameter;

    if(count < path->pathdata.Count)
        return InsufficientBuffer;

    memcpy(types, path->pathdata.Types, path->pathdata.Count);

    return Ok;
}

/* Windows expands the bounding box to the maximum possible bounding box
 * for a given pen.  For example, if a line join can extend past the point
 * it's joining by x units, the bounding box is extended by x units in every
 * direction (even though this is too conservative for most cases). */
GpStatus WINGDIPAPI GdipGetPathWorldBounds(GpPath* path, GpRectF* bounds,
    GDIPCONST GpMatrix *matrix, GDIPCONST GpPen *pen)
{
    /* extrema[0] is upper left corner of bounding box,
     * extrema[1] is lower right corner */
    GpPointF extrema[2];
    GpPointF * points;
    INT count, i;
    REAL path_width;

    /* Matrix and pen can be null. */
    if(!path || !bounds)
        return InvalidParameter;

    /* If path is empty just return. */
    count = path->pathdata.Count;
    if(count == 0){
        bounds->X = bounds->Y = bounds->Width = bounds->Height = 0.0;
        return Ok;
    }

    points = path->pathdata.Points;
    extrema[0].X = extrema[1].X  = points[0].X;
    extrema[0].Y = extrema[1].Y = points[0].Y;

    for(i = 1; i < count; i++){
        extrema[0].X = min(points[i].X, extrema[0].X);
        extrema[0].Y = min(points[i].Y, extrema[0].Y);
        extrema[1].X = max(points[i].X, extrema[1].X);
        extrema[1].Y = max(points[i].Y, extrema[1].Y);
    }

    /* If matrix is non-null transform the points. */
    if(matrix){
        GdipTransformMatrixPoints((GpMatrix*)matrix, extrema, 2);
    }

    if(pen){
        path_width = pen->width * pen->miterlimit / 2.0;
        extrema[0].X -= path_width;
        extrema[0].Y -= path_width;
        extrema[1].X += path_width;
        extrema[1].Y += path_width;
    }

    bounds->X = extrema[0].X;
    bounds->Y = extrema[0].Y;
    bounds->Width = extrema[1].X - extrema[0].X;
    bounds->Height = extrema[1].Y - extrema[0].Y;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetPointCount(GpPath *path, INT *count)
{
    if(!path)
        return InvalidParameter;

    *count = path->pathdata.Count;

    return Ok;
}

GpStatus WINGDIPAPI GdipStartPathFigure(GpPath *path)
{
    if(!path)
        return InvalidParameter;

    path->newfigure = TRUE;

    return Ok;
}

GpStatus WINGDIPAPI GdipTransformPath(GpPath *path, GpMatrix *matrix)
{
    if(!path)
        return InvalidParameter;

    if(path->pathdata.Count == 0)
        return Ok;

    return GdipTransformMatrixPoints(matrix, (GpPointF*) path->pathdata.Points,
                                     path->pathdata.Count);
}
