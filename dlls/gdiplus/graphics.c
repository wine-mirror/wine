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
#include <math.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "gdiplus.h"
#include "gdiplus_private.h"
#include "wine/debug.h"

GpStatus WINGDIPAPI GdipCreateFromHDC(HDC hdc, GpGraphics **graphics)
{
    if(hdc == NULL)
        return OutOfMemory;

    if(graphics == NULL)
        return InvalidParameter;

    *graphics = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
        sizeof(GpGraphics));
    (*graphics)->hdc = hdc;
    (*graphics)->hwnd = NULL;

    return Ok;
}

GpStatus WINGDIPAPI GdipCreateFromHWND(HWND hwnd, GpGraphics **graphics)
{
    GpStatus ret;

    if((ret = GdipCreateFromHDC(GetDC(hwnd), graphics)) != Ok)
        return ret;

    (*graphics)->hwnd = hwnd;

    return Ok;
}

GpStatus WINGDIPAPI GdipDeleteGraphics(GpGraphics *graphics)
{
    if(!graphics) return InvalidParameter;
    if(graphics->hwnd)
        ReleaseDC(graphics->hwnd, graphics->hdc);

    HeapFree(GetProcessHeap(), 0, graphics);

    return Ok;
}

GpStatus WINGDIPAPI GdipDrawLineI(GpGraphics *graphics, GpPen *pen, INT x1,
    INT y1, INT x2, INT y2)
{
    HGDIOBJ old_obj;

    if(!pen || !graphics)
        return InvalidParameter;

    old_obj = SelectObject(graphics->hdc, pen->gdipen);
    MoveToEx(graphics->hdc, x1, y1, NULL);
    LineTo(graphics->hdc, x2, y2);
    SelectObject(graphics->hdc, old_obj);

    return Ok;
}

GpStatus WINGDIPAPI GdipDrawRectangleI(GpGraphics *graphics, GpPen *pen, INT x,
    INT y, INT width, INT height)
{
    LOGBRUSH lb;
    HPEN hpen;
    HGDIOBJ old_obj;

    if(!pen || !graphics)
        return InvalidParameter;

    lb.lbStyle = BS_SOLID;
    lb.lbColor = pen->color;
    lb.lbHatch = 0;

    hpen = ExtCreatePen(PS_GEOMETRIC | PS_ENDCAP_SQUARE, (INT) pen->width,
        &lb, 0, NULL);

    old_obj = SelectObject(graphics->hdc, hpen);

    /* assume pen aligment centered */
    MoveToEx(graphics->hdc, x, y, NULL);
    LineTo(graphics->hdc, x+width, y);
    LineTo(graphics->hdc, x+width, y+height);
    LineTo(graphics->hdc, x, y+height);
    LineTo(graphics->hdc, x, y);

    SelectObject(graphics->hdc, old_obj);
    DeleteObject(hpen);

    return Ok;
}
