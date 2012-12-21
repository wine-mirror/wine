/*
 * MACDRV windowing driver
 *
 * Copyright 1996 Alexandre Julliard
 * Copyright 1999 Patrik Stridvall
 * Copyright 2011, 2012 Ken Thomases for CodeWeavers Inc.
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

#ifndef __WINE_MACDRV_H
#define __WINE_MACDRV_H

#ifndef __WINE_CONFIG_H
# error You must include config.h to use this header
#endif

#define GetCurrentProcess MacGetCurrentProcess
#define GetCurrentThread MacGetCurrentThread
#define LoadResource MacLoadResource
#define AnimatePalette MacAnimatePalette
#define EqualRgn MacEqualRgn
#define FillRgn MacFillRgn
#define FrameRgn MacFrameRgn
#define GetPixel MacGetPixel
#define InvertRgn MacInvertRgn
#define LineTo MacLineTo
#define OffsetRgn MacOffsetRgn
#define PaintRgn MacPaintRgn
#define Polygon MacPolygon
#define ResizePalette MacResizePalette
#define SetRectRgn MacSetRectRgn
#define EqualRect MacEqualRect
#define FillRect MacFillRect
#define FrameRect MacFrameRect
#define GetCursor MacGetCursor
#define InvertRect MacInvertRect
#define OffsetRect MacOffsetRect
#define PtInRect MacPtInRect
#define SetCursor MacSetCursor
#define SetRect MacSetRect
#define ShowCursor MacShowCursor
#define UnionRect MacUnionRect

#include <ApplicationServices/ApplicationServices.h>

#undef GetCurrentProcess
#undef GetCurrentThread
#undef LoadResource
#undef AnimatePalette
#undef EqualRgn
#undef FillRgn
#undef FrameRgn
#undef GetPixel
#undef InvertRgn
#undef LineTo
#undef OffsetRgn
#undef PaintRgn
#undef Polygon
#undef ResizePalette
#undef SetRectRgn
#undef EqualRect
#undef FillRect
#undef FrameRect
#undef GetCursor
#undef InvertRect
#undef OffsetRect
#undef PtInRect
#undef SetCursor
#undef SetRect
#undef ShowCursor
#undef UnionRect
#undef DPRINTF

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "wine/debug.h"
#include "wine/gdi_driver.h"


static inline const char *wine_dbgstr_cgrect(CGRect cgrect)
{
    return wine_dbg_sprintf("(%g,%g)-(%g,%g)", CGRectGetMinX(cgrect), CGRectGetMinY(cgrect),
                            CGRectGetMaxX(cgrect), CGRectGetMaxY(cgrect));
}


#endif  /* __WINE_MACDRV_H */
