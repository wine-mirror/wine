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

#include "gdiplus.h"
#include "gdiplus_private.h"

GpStatus WINGDIPAPI GdipCreatePathIter(GpPathIterator **iterator, GpPath* path)
{
    INT size;

    if(!iterator || !path)
        return InvalidParameter;

    size = path->pathdata.Count;

    *iterator = GdipAlloc(sizeof(GpPathIterator));
    if(!*iterator)  return OutOfMemory;

    (*iterator)->pathdata.Types = GdipAlloc(size);
    (*iterator)->pathdata.Points = GdipAlloc(size * sizeof(PointF));

    memcpy((*iterator)->pathdata.Types, path->pathdata.Types, size);
    memcpy((*iterator)->pathdata.Points, path->pathdata.Points,
           size * sizeof(PointF));
    (*iterator)->pathdata.Count = size;

    (*iterator)->subpath_pos = 0;
    (*iterator)->marker_pos = 0;
    (*iterator)->pathtype_pos = 0;

    return Ok;
}

GpStatus WINGDIPAPI GdipDeletePathIter(GpPathIterator *iter)
{
    if(!iter)
        return InvalidParameter;

    GdipFree(iter->pathdata.Types);
    GdipFree(iter->pathdata.Points);
    GdipFree(iter);

    return Ok;
}

GpStatus WINGDIPAPI GdipPathIterCopyData(GpPathIterator* iterator,
    INT* resultCount, GpPointF* points, BYTE* types, INT startIndex, INT endIndex)
{
    if(!iterator || !types || !points)
        return InvalidParameter;

    if(endIndex > iterator->pathdata.Count - 1 || startIndex < 0 ||
        endIndex < startIndex){
        *resultCount = 0;
        return Ok;
    }

    *resultCount = endIndex - startIndex + 1;

    memcpy(types, &(iterator->pathdata.Types[startIndex]), *resultCount);
    memcpy(points, &(iterator->pathdata.Points[startIndex]),
        *resultCount * sizeof(PointF));

    return Ok;
}
