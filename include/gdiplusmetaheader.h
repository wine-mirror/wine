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

#ifndef _GDIPLUSMETAHEADER_H
#define _GDIPLUSMETAHEADER_H

#include <pshpack2.h>

typedef struct
{
    INT16  Left;
    INT16  Top;
    INT16  Right;
    INT16  Bottom;
} PWMFRect16;

typedef struct
{
    UINT32     Key;
    INT16      Hmf;
    PWMFRect16 BoundingBox;
    INT16      Inch;
    UINT32     Reserved;
    INT16      Checksum;
} WmfPlaceableFileHeader;

#include <poppack.h>

#endif /* _GDIPLUSMETAHEADER_H */
