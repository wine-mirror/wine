/*
 * Copyright (C) 2808 Google (Lei Zhang)
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

#ifndef _GDIPLUSCOLOR_H
#define _GDIPLUSCOLOR_H

enum ColorChannelFlags
{
    ColorChannelFlagsC,
    ColorChannelFlagsM,
    ColorChannelFlagsY,
    ColorChannelFlagsK,
    ColorChannelFlagsLast
};

#ifdef __cplusplus

/* FIXME: missing the methods. */
class Color
{
protected:
    ARGB Argb;

public:
    ARGB GetValue() const { return Argb; }
    void SetValue(ARGB argb) { Argb = argb; }
};

#else /* end of c++ typedefs */

typedef struct Color
{
    ARGB Argb;
} Color;

typedef enum ColorChannelFlags ColorChannelFlags;

#endif  /* end of c typedefs */

#endif  /* _GDIPLUSCOLOR_H */
