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

#ifndef __D3DX8MATH_INL__
#define __D3DX8MATH_INL__

extern inline D3DXVECTOR2* D3DXVec2Add(D3DXVECTOR2 *pout, CONST D3DXVECTOR2 *pv1, CONST D3DXVECTOR2 *pv2)
{
    if ( !pout || !pv1 || !pv2) return NULL;
    pout->x = pv1->x + pv2->x;
    pout->y = pv1->y + pv2->y;
    return pout;
}

extern inline FLOAT D3DXVec2CCW(CONST D3DXVECTOR2 *pv1, CONST D3DXVECTOR2 *pv2)
{
    if ( !pv1 || !pv2) return 0.0f;
    return ( (pv1->x) * (pv2->y) - (pv1->y) * (pv2->x) );
}

extern inline FLOAT D3DXVec2Dot(CONST D3DXVECTOR2 *pv1, CONST D3DXVECTOR2 *pv2)
{
    if ( !pv1 || !pv2) return 0.0f;
    return ( (pv1->x * pv2->x + pv1->y * pv2->y) );
}

extern inline FLOAT D3DXVec2Length(CONST D3DXVECTOR2 *pv)
{
    if (!pv) return 0.0f;
    return sqrt( (pv->x) * (pv->x) + (pv->y) * (pv->y) );
}

extern inline FLOAT D3DXVec2LengthSq(CONST D3DXVECTOR2 *pv)
{
    if (!pv) return 0.0f;
    return( (pv->x) * (pv->x) + (pv->y) * (pv->y) );
}

#endif
