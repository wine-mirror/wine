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

GpStatus WINGDIPAPI GdipCreatePath(GpFillMode fill, GpPath **path)
{
    HDC hdc;
    GpStatus ret;

    if(!path)
        return InvalidParameter;

    *path = GdipAlloc(sizeof(GpPath));
    if(!*path)  return OutOfMemory;

    (*path)->fill = fill;
    (*path)->newfigure = TRUE;

    hdc = GetDC(0);
    ret = GdipCreateFromHDC(hdc, &((*path)->graphics));

    if(ret != Ok){
        ReleaseDC(0, hdc);
        GdipFree(*path);
    }

    return ret;
}

GpStatus WINGDIPAPI GdipDeletePath(GpPath *path)
{
    if(!path || !(path->graphics))
        return InvalidParameter;

    ReleaseDC(0, path->graphics->hdc);
    GdipDeleteGraphics(path->graphics);
    GdipFree(path);

    return Ok;
}
