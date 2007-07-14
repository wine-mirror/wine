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

#ifndef _FLATAPI_H
#define _FLATAPI_H

#define WINGDIPAPI __stdcall

#define GDIPCONST const

#ifdef __cplusplus
extern "C" {
#endif

GpStatus WINGDIPAPI GdipCreatePen1(ARGB,REAL,GpUnit,GpPen**);
GpStatus WINGDIPAPI GdipDeletePen(GpPen*);
GpStatus WINGDIPAPI GdipSetPenEndCap(GpPen*,GpLineCap);
GpStatus WINGDIPAPI GdipSetPenLineJoin(GpPen*,GpLineJoin);

GpStatus WINGDIPAPI GdipCreateFromHDC(HDC,GpGraphics**);
GpStatus WINGDIPAPI GdipCreateFromHWND(HWND,GpGraphics**);
GpStatus WINGDIPAPI GdipDeleteGraphics(GpGraphics *);
GpStatus WINGDIPAPI GdipDrawArc(GpGraphics*,GpPen*,REAL,REAL,REAL,REAL,REAL,REAL);
GpStatus WINGDIPAPI GdipDrawBezier(GpGraphics*,GpPen*,REAL,REAL,REAL,REAL,REAL,
    REAL,REAL,REAL);
GpStatus WINGDIPAPI GdipDrawCurve2(GpGraphics*,GpPen*,GDIPCONST GpPointF*,INT,REAL);
GpStatus WINGDIPAPI GdipDrawLineI(GpGraphics*,GpPen*,INT,INT,INT,INT);
GpStatus WINGDIPAPI GdipDrawLines(GpGraphics*,GpPen*,GDIPCONST GpPointF*,INT);
GpStatus WINGDIPAPI GdipDrawPath(GpGraphics*,GpPen*,GpPath*);
GpStatus WINGDIPAPI GdipDrawPie(GpGraphics*,GpPen*,REAL,REAL,REAL,REAL,REAL,REAL);
GpStatus WINGDIPAPI GdipDrawRectangleI(GpGraphics*,GpPen*,INT,INT,INT,INT);
GpStatus WINGDIPAPI GdipFillPie(GpGraphics*,GpBrush*,REAL,REAL,REAL,REAL,REAL,REAL);
GpStatus WINGDIPAPI GdipGetSmoothingMode(GpGraphics*,SmoothingMode*);
GpStatus WINGDIPAPI GdipSetSmoothingMode(GpGraphics*,SmoothingMode);

GpStatus WINGDIPAPI GdipCreateSolidFill(ARGB,GpSolidFill**);
GpStatus WINGDIPAPI GdipGetBrushType(GpBrush*,GpBrushType*);
GpStatus WINGDIPAPI GdipDeleteBrush(GpBrush*);

GpStatus WINGDIPAPI GdipAddPathArc(GpPath*,REAL,REAL,REAL,REAL,REAL,REAL);
GpStatus WINGDIPAPI GdipAddPathLine2(GpPath*,GDIPCONST GpPointF*,INT);
GpStatus WINGDIPAPI GdipAddPathPath(GpPath*,GDIPCONST GpPath*,BOOL);
GpStatus WINGDIPAPI GdipClosePathFigure(GpPath*);
GpStatus WINGDIPAPI GdipClosePathFigures(GpPath*);
GpStatus WINGDIPAPI GdipCreatePath(GpFillMode,GpPath**);
GpStatus WINGDIPAPI GdipDeletePath(GpPath*);
GpStatus WINGDIPAPI GdipGetPathFillMode(GpPath*,GpFillMode*);
GpStatus WINGDIPAPI GdipGetPathPoints(GpPath*,GpPointF*,INT);
GpStatus WINGDIPAPI GdipGetPathTypes(GpPath*,BYTE*,INT);
GpStatus WINGDIPAPI GdipGetPathWorldBounds(GpPath*,GpRectF*,GDIPCONST GpMatrix*,
    GDIPCONST GpPen*);
GpStatus WINGDIPAPI GdipGetPointCount(GpPath*,INT*);
GpStatus WINGDIPAPI GdipStartPathFigure(GpPath*);
GpStatus WINGDIPAPI GdipResetPath(GpPath*);
GpStatus WINGDIPAPI GdipSetPathFillMode(GpPath*,GpFillMode);
GpStatus WINGDIPAPI GdipTransformPath(GpPath*,GpMatrix*);

GpStatus WINGDIPAPI GdipCreateMatrix2(REAL,REAL,REAL,REAL,REAL,REAL,GpMatrix**);
GpStatus WINGDIPAPI GdipDeleteMatrix(GpMatrix*);
GpStatus WINGDIPAPI GdipTransformMatrixPoints(GpMatrix*,GpPointF*,INT);

#ifdef __cplusplus
}
#endif

#endif
