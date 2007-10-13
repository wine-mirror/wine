/*
 * Copyright (C) 2007 David Adam
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

#include <d3dx8.h>

#ifndef __D3DX8MATH_H__
#define __D3DX8MATH_H__

#include <math.h>

#define D3DX_PI    ((FLOAT)3.141592654)
#define D3DX_1BYPI ((FLOAT)0.318309886)

#define D3DXToRadian(degree) ((degree) * (D3DX_PI / 180.0f))
#define D3DXToDegree(radian) ((radian) * (180.0f / D3DX_PI))

typedef struct D3DXVECTOR2
{
    FLOAT x, y;
} D3DXVECTOR2, *LPD3DXVECTOR2;

typedef struct _D3DVECTOR D3DXVECTOR3, *LPD3DXVECTOR3;

typedef struct D3DXVECTOR4
{
    FLOAT x, y, z, w;
} D3DXVECTOR4, *LPD3DXVECTOR4;

typedef struct _D3DMATRIX D3DXMATRIX, *LPD3DXMATRIX;

typedef struct D3DXQUATERNION
{
    FLOAT x, y, z, w;
} D3DXQUATERNION, *LPD3DXQUATERNION;

typedef struct D3DXPLANE
{
    FLOAT a, b, c, d;
} D3DXPLANE, *LPD3DXPLANE;

typedef struct D3DXCOLOR
{
    FLOAT r, g, b, a;
} D3DXCOLOR, *LPD3DXCOLOR;

FLOAT D3DXVec2Length(CONST D3DXVECTOR2 *pv);
FLOAT D3DXVec2LengthSq(CONST D3DXVECTOR2 *pv);

#endif /* __D3DX8MATH_H__ */
