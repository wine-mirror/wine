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

#ifndef _GDIPLUSENUMS_H
#define _GDIPLUSENUMS_H

enum Unit
{
    UnitWorld       = 0,
    UnitDisplay     = 1,
    UnitPixel       = 2,
    UnitPoint       = 3,
    UnitInch        = 4,
    UnitDocument    = 5,
    UnitMillimeter  = 6
};

enum BrushType
{
   BrushTypeSolidColor       = 0,
   BrushTypeHatchFill        = 1,
   BrushTypeTextureFill      = 2,
   BrushTypePathGradient     = 3,
   BrushTypeLinearGradient   = 4
};

enum FillMode
{
    FillModeAlternate   = 0,
    FillModeWinding     = 1
};

enum LineCap
{
    LineCapFlat             = 0x00,
    LineCapSquare           = 0x01,
    LineCapRound            = 0x02,
    LineCapTriangle         = 0x03,

    LineCapNoAnchor         = 0x10,
    LineCapSquareAnchor     = 0x11,
    LineCapRoundAnchor      = 0x12,
    LineCapDiamondAnchor    = 0x13,
    LineCapArrowAnchor      = 0x14,

    LineCapCustom           = 0xff,
    LineCapAnchorMask       = 0xf0
};

enum PathPointType{
    PathPointTypeStart          = 0,    /* start of a figure */
    PathPointTypeLine           = 1,
    PathPointTypeBezier         = 3,
    PathPointTypePathTypeMask   = 7,
    PathPointTypePathDashMode   = 16,   /* not used */
    PathPointTypePathMarker     = 32,
    PathPointTypeCloseSubpath   = 128,  /* end of a closed figure */
    PathPointTypeBezier3        = 3
};

enum LineJoin
{
    LineJoinMiter           = 0,
    LineJoinBevel           = 1,
    LineJoinRound           = 2,
    LineJoinMiterClipped    = 3
};

#ifndef __cplusplus

typedef enum Unit Unit;
typedef enum BrushType BrushType;
typedef enum FillMode FillMode;
typedef enum LineCap LineCap;
typedef enum PathPointType PathPointType;
typedef enum LineJoin LineJoin;

#endif /* end of c typedefs */

#endif
