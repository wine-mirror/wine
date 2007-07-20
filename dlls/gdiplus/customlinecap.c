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

GpStatus WINGDIPAPI GdipCreateCustomLineCap(GpPath* fillPath, GpPath* strokePath,
    GpLineCap baseCap, REAL baseInset, GpCustomLineCap **customCap)
{
    GpPathData *pathdata;

    if(!customCap || !(fillPath || strokePath))
        return InvalidParameter;

    *customCap = GdipAlloc(sizeof(GpCustomLineCap));
    if(!*customCap)    return OutOfMemory;

    if(strokePath){
        (*customCap)->fill = FALSE;
        pathdata = &strokePath->pathdata;
    }
    else{
        (*customCap)->fill = TRUE;
        pathdata = &fillPath->pathdata;
    }

    (*customCap)->pathdata.Points = GdipAlloc(pathdata->Count * sizeof(PointF));
    (*customCap)->pathdata.Types = GdipAlloc(pathdata->Count);

    if((!(*customCap)->pathdata.Types || !(*customCap)->pathdata.Points) &&
        pathdata->Count){
        GdipFree((*customCap)->pathdata.Points);
        GdipFree((*customCap)->pathdata.Types);
        GdipFree(*customCap);
        return OutOfMemory;
    }

    memcpy((*customCap)->pathdata.Points, pathdata->Points, pathdata->Count
           * sizeof(PointF));
    memcpy((*customCap)->pathdata.Types, pathdata->Types, pathdata->Count);
    (*customCap)->pathdata.Count = pathdata->Count;

    (*customCap)->inset = baseInset;
    (*customCap)->cap = baseCap;

    return Ok;
}

GpStatus WINGDIPAPI GdipDeleteCustomLineCap(GpCustomLineCap *customCap)
{
    if(!customCap)
        return InvalidParameter;

    GdipFree(customCap->pathdata.Points);
    GdipFree(customCap->pathdata.Types);
    GdipFree(customCap);

    return Ok;
}
