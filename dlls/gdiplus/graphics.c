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

static inline INT roundr(REAL x)
{
    return (INT) floor(x+0.5);
}

static inline REAL deg2rad(REAL degrees)
{
    return (M_PI*2.0) * degrees / 360.0;
}

/* Converts angle (in degrees) to x/y coordinates */
static void deg2xy(REAL angle, REAL x_0, REAL y_0, REAL *x, REAL *y)
{
    REAL radAngle, hypotenuse;

    radAngle = deg2rad(angle);
    hypotenuse = 50.0; /* arbitrary */

    *x = x_0 + cos(radAngle) * hypotenuse;
    *y = y_0 + sin(radAngle) * hypotenuse;
}

/* GdipDrawPie/GdipFillPie helper function */
static GpStatus draw_pie(GpGraphics *graphics, HBRUSH gdibrush, HPEN gdipen,
    REAL x, REAL y, REAL width, REAL height, REAL startAngle, REAL sweepAngle)
{
    HGDIOBJ old_pen, old_brush;
    REAL x_0, y_0, x_1, y_1, x_2, y_2;

    if(!graphics)
        return InvalidParameter;

    old_pen = SelectObject(graphics->hdc, gdipen);
    old_brush = SelectObject(graphics->hdc, gdibrush);

    x_0 = x + (width/2.0);
    y_0 = y + (height/2.0);

    deg2xy(startAngle+sweepAngle, x_0, y_0, &x_1, &y_1);
    deg2xy(startAngle, x_0, y_0, &x_2, &y_2);

    Pie(graphics->hdc, roundr(x), roundr(y), roundr(x+width), roundr(y+height),
        roundr(x_1), roundr(y_1), roundr(x_2), roundr(y_2));

    SelectObject(graphics->hdc, old_pen);
    SelectObject(graphics->hdc, old_brush);

    return Ok;
}

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

GpStatus WINGDIPAPI GdipDrawBezier(GpGraphics *graphics, GpPen *pen, REAL x1,
    REAL y1, REAL x2, REAL y2, REAL x3, REAL y3, REAL x4, REAL y4)
{
    HGDIOBJ old_pen;
    POINT pt[4] = {{roundr(x1), roundr(y1)}, {roundr(x2), roundr(y2)},
                   {roundr(x3), roundr(y3)}, {roundr(x4), roundr(y4)}};

    if(!graphics || !pen)
        return InvalidParameter;

    old_pen = SelectObject(graphics->hdc, pen->gdipen);

    PolyBezier(graphics->hdc, pt, 4);

    SelectObject(graphics->hdc, old_pen);

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

GpStatus WINGDIPAPI GdipDrawPie(GpGraphics *graphics, GpPen *pen, REAL x,
    REAL y, REAL width, REAL height, REAL startAngle, REAL sweepAngle)
{
    if(!pen)
        return InvalidParameter;

    return draw_pie(graphics, GetStockObject(NULL_BRUSH), pen->gdipen, x, y,
        width, height, startAngle, sweepAngle);
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

GpStatus WINGDIPAPI GdipFillPie(GpGraphics *graphics, GpBrush *brush, REAL x,
    REAL y, REAL width, REAL height, REAL startAngle, REAL sweepAngle)
{
    if(!brush)
        return InvalidParameter;

    return draw_pie(graphics, brush->gdibrush, GetStockObject(NULL_PEN), x, y,
        width, height, startAngle, sweepAngle);
}
