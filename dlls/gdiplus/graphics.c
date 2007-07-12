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

WINE_DEFAULT_DEBUG_CHANNEL(gdiplus);

/* looks-right constants */
#define TENSION_CONST (0.3)
#define ANCHOR_WIDTH (2.0)
#define MAX_ITERS (50)

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
    INT save_state;
    REAL x_0, y_0, x_1, y_1, x_2, y_2;

    if(!graphics)
        return InvalidParameter;

    save_state = SaveDC(graphics->hdc);
    EndPath(graphics->hdc);
    SelectObject(graphics->hdc, gdipen);
    SelectObject(graphics->hdc, gdibrush);

    x_0 = x + (width/2.0);
    y_0 = y + (height/2.0);

    deg2xy(startAngle+sweepAngle, x_0, y_0, &x_1, &y_1);
    deg2xy(startAngle, x_0, y_0, &x_2, &y_2);

    Pie(graphics->hdc, roundr(x), roundr(y), roundr(x+width), roundr(y+height),
        roundr(x_1), roundr(y_1), roundr(x_2), roundr(y_2));

    RestoreDC(graphics->hdc, save_state);

    return Ok;
}

/* GdipDrawCurve helper function.
 * Calculates Bezier points from cardinal spline points. */
static void calc_curve_bezier(CONST GpPointF *pts, REAL tension, REAL *x1,
    REAL *y1, REAL *x2, REAL *y2)
{
    REAL xdiff, ydiff;

    /* calculate tangent */
    xdiff = pts[2].X - pts[0].X;
    ydiff = pts[2].Y - pts[0].Y;

    /* apply tangent to get control points */
    *x1 = pts[1].X - tension * xdiff;
    *y1 = pts[1].Y - tension * ydiff;
    *x2 = pts[1].X + tension * xdiff;
    *y2 = pts[1].Y + tension * ydiff;
}

/* GdipDrawCurve helper function.
 * Calculates Bezier points from cardinal spline endpoints. */
static void calc_curve_bezier_endp(REAL xend, REAL yend, REAL xadj, REAL yadj,
    REAL tension, REAL *x, REAL *y)
{
    /* tangent at endpoints is the line from the endpoint to the adjacent point */
    *x = roundr(tension * (xadj - xend) + xend);
    *y = roundr(tension * (yadj - yend) + yend);
}

/* Draws the linecap the specified color and size on the hdc.  The linecap is in
 * direction of the line from x1, y1 to x2, y2 and is anchored on x2, y2. */
static void draw_cap(HDC hdc, COLORREF color, GpLineCap cap, REAL size,
    REAL x1, REAL y1, REAL x2, REAL y2)
{
    HGDIOBJ oldbrush, oldpen;
    HBRUSH brush;
    HPEN pen;
    POINT pt[4];
    REAL theta, dsmall, dbig, dx, dy, invert;

    if(x2 != x1)
        theta = atan((y2 - y1) / (x2 - x1));
    else if(y2 != y1){
        theta = M_PI_2 * (y2 > y1 ? 1.0 : -1.0);
    }
    else
        return;

    invert = ((x2 - x1) >= 0.0 ? 1.0 : -1.0);
    brush = CreateSolidBrush(color);
    pen = CreatePen(PS_SOLID, 1, color);
    oldbrush = SelectObject(hdc, brush);
    oldpen = SelectObject(hdc, pen);

    switch(cap){
        case LineCapFlat:
            break;
        case LineCapSquare:
        case LineCapSquareAnchor:
        case LineCapDiamondAnchor:
            size = size * (cap & LineCapNoAnchor ? ANCHOR_WIDTH : 1.0) / 2.0;
            if(cap == LineCapDiamondAnchor){
                dsmall = cos(theta + M_PI_2) * size;
                dbig = sin(theta + M_PI_2) * size;
            }
            else{
                dsmall = cos(theta + M_PI_4) * size;
                dbig = sin(theta + M_PI_4) * size;
            }

            /* calculating the latter points from the earlier points makes them
             * look a little better because of rounding issues */
            pt[0].x = roundr(x2 - dsmall);
            pt[1].x = roundr(((REAL)pt[0].x) + dbig + dsmall);

            pt[0].y = roundr(y2 - dbig);
            pt[3].y = roundr(((REAL)pt[0].y) + dsmall + dbig);

            pt[1].y = roundr(y2 - dsmall);
            pt[2].y = roundr(dbig + dsmall + ((REAL)pt[1].y));

            pt[3].x = roundr(x2 - dbig);
            pt[2].x = roundr(((REAL)pt[3].x) + dsmall + dbig);

            Polygon(hdc, pt, 4);

            break;
        case LineCapArrowAnchor:
            size = size * 4.0 / sqrt(3.0);

            dx = cos(M_PI / 6.0 + theta) * size * invert;
            dy = sin(M_PI / 6.0 + theta) * size * invert;

            pt[0].x = roundr(x2 - dx);
            pt[0].y = roundr(y2 - dy);

            dx = cos(- M_PI / 6.0 + theta) * size * invert;
            dy = sin(- M_PI / 6.0 + theta) * size * invert;

            pt[1].x = roundr(x2 - dx);
            pt[1].y = roundr(y2 - dy);

            pt[2].x = roundr(x2);
            pt[2].y = roundr(y2);

            Polygon(hdc, pt, 3);

            break;
        case LineCapRoundAnchor:
            dx = dy = ANCHOR_WIDTH * size / 2.0;

            x2 = (REAL) roundr(x2 - dx);
            y2 = (REAL) roundr(y2 - dy);

            Ellipse(hdc, (INT) x2, (INT) y2, roundr(x2 + 2.0 * dx),
                roundr(y2 + 2.0 * dy));
            break;
        case LineCapTriangle:
            size = size / 2.0;
            dx = cos(M_PI_2 + theta) * size;
            dy = sin(M_PI_2 + theta) * size;

            /* Using roundr here can make the triangle float off the end of the
             * line. */
            pt[0].x = ((x2 - x1) >= 0 ? floorf(x2 - dx) : ceilf(x2 - dx));
            pt[0].y = ((y2 - y1) >= 0 ? floorf(y2 - dy) : ceilf(y2 - dy));
            pt[1].x = roundr(pt[0].x + 2.0 * dx);
            pt[1].y = roundr(pt[0].y + 2.0 * dy);

            dx = cos(theta) * size * invert;
            dy = sin(theta) * size * invert;

            pt[2].x = roundr(x2 + dx);
            pt[2].y = roundr(y2 + dy);

            Polygon(hdc, pt, 3);

            break;
        case LineCapRound:
            dx = -cos(M_PI_2 + theta) * size * invert;
            dy = -sin(M_PI_2 + theta) * size * invert;

            pt[0].x = ((x2 - x1) >= 0 ? floorf(x2 - dx) : ceilf(x2 - dx));
            pt[0].y = ((y2 - y1) >= 0 ? floorf(y2 - dy) : ceilf(y2 - dy));
            pt[1].x = roundr(pt[0].x + 2.0 * dx);
            pt[1].y = roundr(pt[0].y + 2.0 * dy);

            dx = dy = size / 2.0;

            x2 = (REAL) roundr(x2 - dx);
            y2 = (REAL) roundr(y2 - dy);

            Pie(hdc, (INT) x2, (INT) y2, roundr(x2 + 2.0 * dx),
                roundr(y2 + 2.0 * dy), pt[0].x, pt[0].y, pt[1].x, pt[1].y);
            break;
        case LineCapCustom:
            FIXME("line cap not implemented\n");
        default:
            break;
    }

    SelectObject(hdc, oldbrush);
    SelectObject(hdc, oldpen);
    DeleteObject(brush);
    DeleteObject(pen);
}

/* Shortens the line by the given percent by changing x2, y2.
 * If percent is > 1.0 then the line will change direction. */
static void shorten_line_percent(REAL x1, REAL  y1, REAL *x2, REAL *y2, REAL percent)
{
    REAL dist, theta, dx, dy;

    if((y1 == *y2) && (x1 == *x2))
        return;

    dist = sqrt((*x2 - x1) * (*x2 - x1) + (*y2 - y1) * (*y2 - y1)) * percent;
    theta = (*x2 == x1 ? M_PI_2 : atan((*y2 - y1) / (*x2 - x1)));
    dx = cos(theta) * dist;
    dy = sin(theta) * dist;

    *x2 = *x2 + fabs(dx) * (*x2 > x1 ? -1.0 : 1.0);
    *y2 = *y2 + fabs(dy) * (*y2 > y1 ? -1.0 : 1.0);
}

/* Shortens the line by the given amount by changing x2, y2.
 * If the amount is greater than the distance, the line will become length 0. */
static void shorten_line_amt(REAL x1, REAL y1, REAL *x2, REAL *y2, REAL amt)
{
    REAL dx, dy, percent;

    dx = *x2 - x1;
    dy = *y2 - y1;
    if(dx == 0 && dy == 0)
        return;

    percent = amt / sqrt(dx * dx + dy * dy);
    if(percent >= 1.0){
        *x2 = x1;
        *y2 = y1;
        return;
    }

    shorten_line_percent(x1, y1, x2, y2, percent);
}

/* Draws lines between the given points, and if caps is true then draws an endcap
 * at the end of the last line.  FIXME: Startcaps not implemented. */
static GpStatus draw_polyline(HDC hdc, GpPen *pen, GDIPCONST GpPointF * pt,
    INT count, BOOL caps)
{
    POINT *pti;
    REAL x = pt[count - 1].X, y = pt[count - 1].Y;
    INT i;
    GpStatus status = GenericError;

    if(!count)
        return Ok;

    pti = GdipAlloc(count * sizeof(POINT));

    if(!pti){
        status = OutOfMemory;
        goto end;
    }

    if(caps){
        if(pen->endcap == LineCapArrowAnchor)
            shorten_line_amt(pt[count-2].X, pt[count-2].Y, &x, &y, pen->width);

        draw_cap(hdc, pen->color, pen->endcap, pen->width, pt[count-2].X,
            pt[count-2].Y, pt[count - 1].X, pt[count - 1].Y);
    }

    for(i = 0; i < count - 1; i ++){
        pti[i].x = roundr(pt[i].X);
        pti[i].y = roundr(pt[i].Y);
    }

    pti[i].x = roundr(x);
    pti[i].y = roundr(y);

    Polyline(hdc, pti, count);

end:
    GdipFree(pti);

    return status;
}

/* Conducts a linear search to find the bezier points that will back off
 * the endpoint of the curve by a distance of amt. Linear search works
 * better than binary in this case because there are multiple solutions,
 * and binary searches often find a bad one. I don't think this is what
 * Windows does but short of rendering the bezier without GDI's help it's
 * the best we can do. */
static void shorten_bezier_amt(GpPointF * pt, REAL amt)
{
    GpPointF origpt[4];
    REAL percent = 0.00, dx, dy, origx = pt[3].X, origy = pt[3].Y, diff = -1.0;
    INT i;

    memcpy(origpt, pt, sizeof(GpPointF) * 4);

    for(i = 0; (i < MAX_ITERS) && (diff < amt); i++){
        /* reset bezier points to original values */
        memcpy(pt, origpt, sizeof(GpPointF) * 4);
        /* Perform magic on bezier points. Order is important here.*/
        shorten_line_percent(pt[2].X, pt[2].Y, &pt[3].X, &pt[3].Y, percent);
        shorten_line_percent(pt[1].X, pt[1].Y, &pt[2].X, &pt[2].Y, percent);
        shorten_line_percent(pt[2].X, pt[2].Y, &pt[3].X, &pt[3].Y, percent);
        shorten_line_percent(pt[0].X, pt[0].Y, &pt[1].X, &pt[1].Y, percent);
        shorten_line_percent(pt[1].X, pt[1].Y, &pt[2].X, &pt[2].Y, percent);
        shorten_line_percent(pt[2].X, pt[2].Y, &pt[3].X, &pt[3].Y, percent);

        dx = pt[3].X - origx;
        dy = pt[3].Y - origy;

        diff = sqrt(dx * dx + dy * dy);
        percent += 0.0005 * amt;
    }
}

/* Draws bezier curves between given points, and if caps is true then draws an
 * endcap at the end of the last line.  FIXME: Startcaps not implemented. */
static GpStatus draw_polybezier(HDC hdc, GpPen *pen, GDIPCONST GpPointF * pt,
    INT count, BOOL caps)
{
    POINT *pti;
    GpPointF *ptf;
    INT i;
    GpStatus status = GenericError;

    if(!count)
        return Ok;

    pti = GdipAlloc(count * sizeof(POINT));
    ptf = GdipAlloc(4 * sizeof(GpPointF));

    if(!pti || !ptf){
        status = OutOfMemory;
        goto end;
    }

    memcpy(ptf, &pt[count-4], 4 * sizeof(GpPointF));

    if(caps){
        if(pen->endcap == LineCapArrowAnchor)
            shorten_bezier_amt(ptf, pen->width);

        draw_cap(hdc, pen->color, pen->endcap, pen->width, ptf[3].X,
            ptf[3].Y, pt[count - 1].X, pt[count - 1].Y);
    }

    for(i = 0; i < count - 4; i ++){
        pti[i].x = roundr(pt[i].X);
        pti[i].y = roundr(pt[i].Y);
    }
    for(i = 0; i < 4; i ++){
        pti[i + count - 4].x = roundr(ptf[i].X);
        pti[i + count - 4].y = roundr(ptf[i].Y);
    }

    PolyBezier(hdc, pti, count);

    status = Ok;

end:
    GdipFree(pti);
    GdipFree(ptf);

    return status;
}

/* Converts from gdiplus path point type to gdi path point type. */
static BYTE convert_path_point_type(BYTE type)
{
    BYTE ret;

    switch(type & PathPointTypePathTypeMask){
        case PathPointTypeBezier:
            ret = PT_BEZIERTO;
            break;
        case PathPointTypeLine:
            ret = PT_LINETO;
            break;
        case PathPointTypeStart:
            ret = PT_MOVETO;
            break;
        default:
            ERR("Bad point type\n");
            return 0;
    }

    if(type & PathPointTypeCloseSubpath)
        ret |= PT_CLOSEFIGURE;

    return ret;
}

/* Draws a combination of bezier curves and lines between points. */
static GpStatus draw_poly(HDC hdc, GpPen *pen, GDIPCONST GpPointF * pt,
    GDIPCONST BYTE * types, INT count, BOOL caps)
{
    POINT *pti = GdipAlloc(count * sizeof(POINT));
    BYTE *tp = GdipAlloc(count);
    GpPointF *ptf = NULL;
    REAL x = pt[count - 1].X, y = pt[count - 1].Y;
    INT i;
    GpStatus status = GenericError;

    if(!count){
        status = Ok;
        goto end;
    }
    if(!pti || !tp){
        status = OutOfMemory;
        goto end;
    }

    for(i = 0; i < count; i++){
        if((types[i] & PathPointTypePathTypeMask) == PathPointTypeBezier){
            if((i + 2 >= count) || !(types[i + 1] & PathPointTypeBezier)
                || !(types[i + 1] & PathPointTypeBezier)){
                ERR("Bad bezier points\n");
                goto end;
            }

            i += 2;
        }
    }

    if((types[count - 1] & PathPointTypePathTypeMask) == PathPointTypeBezier){
        ptf = GdipAlloc(4 * sizeof(GpPointF));
        memcpy(ptf, &pt[count-4], 4 * sizeof(GpPointF));

        if(caps){
            if(pen->endcap == LineCapArrowAnchor)
                shorten_bezier_amt(ptf, pen->width);

            draw_cap(hdc, pen->color, pen->endcap, pen->width, ptf[3].X,
                ptf[3].Y, pt[count - 1].X, pt[count - 1].Y);
        }
        for(i = 0; i < 4; i ++){
            pti[i + count - 4].x = roundr(ptf[i].X);
            pti[i + count - 4].y = roundr(ptf[i].Y);
        }
        for(i = 0; i < count - 4; i ++){
            pti[i].x = roundr(pt[i].X);
            pti[i].y = roundr(pt[i].Y);
        }
    }
    else if((types[count - 1] & PathPointTypePathTypeMask) == PathPointTypeLine){
        if(caps){
            if(pen->endcap == LineCapArrowAnchor)
                shorten_line_amt(pt[count-2].X, pt[count-2].Y, &x, &y, pen->width);

            draw_cap(hdc, pen->color, pen->endcap, pen->width, pt[count-2].X,
                pt[count-2].Y, pt[count - 1].X, pt[count - 1].Y);
        }
        pti[count - 1].x = roundr(x);
        pti[count - 1].y = roundr(y);
        for(i = 0; i < count - 1; i ++){
            pti[i].x = roundr(pt[i].X);
            pti[i].y = roundr(pt[i].Y);
        }
    }
    else{
        ERR("Bad path last point\n");
        goto end;
    }

    for(i = 0; i < count; i++){
        tp[i] = convert_path_point_type(types[i]);
    }

    PolyDraw(hdc, pti, tp, count);

    status = Ok;

end:
    GdipFree(pti);
    GdipFree(ptf);
    GdipFree(tp);

    return status;
}

GpStatus WINGDIPAPI GdipCreateFromHDC(HDC hdc, GpGraphics **graphics)
{
    if(hdc == NULL)
        return OutOfMemory;

    if(graphics == NULL)
        return InvalidParameter;

    *graphics = GdipAlloc(sizeof(GpGraphics));
    if(!*graphics)  return OutOfMemory;

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

GpStatus WINGDIPAPI GdipDrawArc(GpGraphics *graphics, GpPen *pen, REAL x,
    REAL y, REAL width, REAL height, REAL startAngle, REAL sweepAngle)
{
    INT save_state, num_pts;
    GpPointF points[MAX_ARC_PTS];
    GpStatus retval;

    if(!graphics || !pen)
        return InvalidParameter;

    num_pts = arc2polybezier(points, x, y, width, height, startAngle, sweepAngle);

    save_state = SaveDC(graphics->hdc);
    EndPath(graphics->hdc);
    SelectObject(graphics->hdc, pen->gdipen);

    retval = draw_polybezier(graphics->hdc, pen, points, num_pts, TRUE);

    RestoreDC(graphics->hdc, save_state);

    return retval;
}

GpStatus WINGDIPAPI GdipDrawBezier(GpGraphics *graphics, GpPen *pen, REAL x1,
    REAL y1, REAL x2, REAL y2, REAL x3, REAL y3, REAL x4, REAL y4)
{
    INT save_state;
    GpPointF pt[4];
    GpStatus retval;

    if(!graphics || !pen)
        return InvalidParameter;

    pt[0].X = x1;
    pt[0].Y = y1;
    pt[1].X = x2;
    pt[1].Y = y2;
    pt[2].X = x3;
    pt[2].Y = y3;
    pt[3].X = x4;
    pt[3].Y = y4;

    save_state = SaveDC(graphics->hdc);
    EndPath(graphics->hdc);
    SelectObject(graphics->hdc, pen->gdipen);

    retval = draw_polybezier(graphics->hdc, pen, pt, 4, TRUE);

    RestoreDC(graphics->hdc, save_state);

    return retval;
}

/* Approximates cardinal spline with Bezier curves. */
GpStatus WINGDIPAPI GdipDrawCurve2(GpGraphics *graphics, GpPen *pen,
    GDIPCONST GpPointF *points, INT count, REAL tension)
{
    /* PolyBezier expects count*3-2 points. */
    INT i, len_pt = count*3-2, save_state;
    GpPointF *pt;
    REAL x1, x2, y1, y2;
    GpStatus retval;

    if(!graphics || !pen)
        return InvalidParameter;

    pt = GdipAlloc(len_pt * sizeof(GpPointF));
    tension = tension * TENSION_CONST;

    calc_curve_bezier_endp(points[0].X, points[0].Y, points[1].X, points[1].Y,
        tension, &x1, &y1);

    pt[0].X = points[0].X;
    pt[0].Y = points[0].Y;
    pt[1].X = x1;
    pt[1].Y = y1;

    for(i = 0; i < count-2; i++){
        calc_curve_bezier(&(points[i]), tension, &x1, &y1, &x2, &y2);

        pt[3*i+2].X = x1;
        pt[3*i+2].Y = y1;
        pt[3*i+3].X = points[i+1].X;
        pt[3*i+3].Y = points[i+1].Y;
        pt[3*i+4].X = x2;
        pt[3*i+4].Y = y2;
    }

    calc_curve_bezier_endp(points[count-1].X, points[count-1].Y,
        points[count-2].X, points[count-2].Y, tension, &x1, &y1);

    pt[len_pt-2].X = x1;
    pt[len_pt-2].Y = y1;
    pt[len_pt-1].X = points[count-1].X;
    pt[len_pt-1].Y = points[count-1].Y;

    save_state = SaveDC(graphics->hdc);
    EndPath(graphics->hdc);
    SelectObject(graphics->hdc, pen->gdipen);

    retval = draw_polybezier(graphics->hdc, pen, pt, len_pt, TRUE);

    GdipFree(pt);
    RestoreDC(graphics->hdc, save_state);

    return retval;
}

GpStatus WINGDIPAPI GdipDrawLineI(GpGraphics *graphics, GpPen *pen, INT x1,
    INT y1, INT x2, INT y2)
{
    INT save_state;
    GpPointF pt[2];
    GpStatus retval;

    if(!pen || !graphics)
        return InvalidParameter;

    pt[0].X = (REAL)x1;
    pt[0].Y = (REAL)y1;
    pt[1].X = (REAL)x2;
    pt[1].Y = (REAL)y2;

    save_state = SaveDC(graphics->hdc);
    EndPath(graphics->hdc);
    SelectObject(graphics->hdc, pen->gdipen);

    retval = draw_polyline(graphics->hdc, pen, pt, 2, TRUE);

    RestoreDC(graphics->hdc, save_state);

    return retval;
}

GpStatus WINGDIPAPI GdipDrawLines(GpGraphics *graphics, GpPen *pen, GDIPCONST
    GpPointF *points, INT count)
{
    INT save_state;
    GpStatus retval;

    if(!pen || !graphics || (count < 2))
        return InvalidParameter;

    save_state = SaveDC(graphics->hdc);
    EndPath(graphics->hdc);
    SelectObject(graphics->hdc, pen->gdipen);

    retval = draw_polyline(graphics->hdc, pen, points, count, TRUE);

    RestoreDC(graphics->hdc, save_state);

    return retval;
}

GpStatus WINGDIPAPI GdipDrawPath(GpGraphics *graphics, GpPen *pen, GpPath *path)
{
    INT save_state, i, this_fig = 0;
    GpStatus retval;

    if(!pen || !graphics)
        return InvalidParameter;

    save_state = SaveDC(graphics->hdc);
    EndPath(graphics->hdc);
    SelectObject(graphics->hdc, pen->gdipen);

    for(i = 0; i < path->pathdata.Count; i++){
        if(path->pathdata.Types[i] == PathPointTypeStart){
            retval = draw_poly(graphics->hdc, pen,
                         &path->pathdata.Points[this_fig],
                         &path->pathdata.Types[this_fig], i - this_fig, TRUE);
            this_fig = i;

            if(retval != Ok)
                goto end;
        }
    }

    retval = draw_poly(graphics->hdc, pen, &path->pathdata.Points[this_fig],
                       &path->pathdata.Types[this_fig], path->pathdata.Count - this_fig,
                       TRUE);

end:
    RestoreDC(graphics->hdc, save_state);

    return retval;
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
    INT save_state;

    if(!pen || !graphics)
        return InvalidParameter;

    save_state = SaveDC(graphics->hdc);
    EndPath(graphics->hdc);
    SelectObject(graphics->hdc, pen->gdipen);
    SelectObject(graphics->hdc, GetStockObject(NULL_BRUSH));

    Rectangle(graphics->hdc, x, y, x + width, y + height);

    RestoreDC(graphics->hdc, save_state);

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
