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

#ifndef __WINE_GP_PRIVATE_H_
#define __WINE_GP_PRIVATE_H_

#include "windef.h"
#include "gdiplus.h"

#define GP_DEFAULT_PENSTYLE (PS_GEOMETRIC | PS_ENDCAP_FLAT)

COLORREF ARGB2COLORREF(ARGB color);

struct GpPen{
    UINT style;
    COLORREF color;
    GpUnit unit;
    REAL width;
    HPEN gdipen;
};

struct GpGraphics{
    HDC hdc;
    HWND hwnd;
};

struct GpBrush{
    HBRUSH gdibrush;
    GpBrushType bt;
    COLORREF color;
};

struct GpSolidFill{
    GpBrush brush;
};

struct GpPath{
    GpFillMode fill;
    GpGraphics* graphics;
    GpPathData pathdata;
    BOOL newfigure; /* whether the next drawing action starts a new figure */
};

#endif
