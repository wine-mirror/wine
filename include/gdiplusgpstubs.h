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

#ifndef _GDIPLUSGPSTUBS_H
#define _GDIPLUSGPSTUBS_H

#ifdef __cplusplus

class GpGraphics {};
class GpGraphics {};
class GpBrush {};
class GpSolidFill {};
class GpPath {};
class GpMatrix {};
class GpPathIterator {};

#else /* end of c++ declarations */

typedef struct GpGraphics GpGraphics;
typedef struct GpPen GpPen;
typedef struct GpBrush GpBrush;
typedef struct GpSolidFill GpSolidFill;
typedef struct GpPath GpPath;
typedef struct GpMatrix GpMatrix;
typedef struct GpPathIterator GpPathIterator;

#endif /* end of c declarations */

typedef Status GpStatus;
typedef Unit GpUnit;
typedef BrushType GpBrushType;
typedef PointF GpPointF;
typedef FillMode GpFillMode;
typedef PathData GpPathData;
typedef LineCap GpLineCap;
typedef RectF GpRectF;
typedef LineJoin GpLineJoin;
typedef DashCap GpDashCap;
typedef DashStyle GpDashStyle;
typedef MatrixOrder GpMatrixOrder;

#endif
