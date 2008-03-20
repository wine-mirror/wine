/*
 * Mathematical operations specific to D3DX9.
 *
 * Copyright (C) 2008 Philip Nilsson
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

#include "config.h"
#include "windef.h"
#include "wingdi.h"
#include "wine/debug.h"
#include "d3dx9.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);

/*************************************************************************
 * D3DXVec2TransformArray
 *
 * Transform an array of vectors by a matrix.
 */
D3DXVECTOR4* WINAPI D3DXVec2TransformArray(
    D3DXVECTOR4* out, UINT outstride, CONST D3DXVECTOR2* in, UINT instride,
    CONST D3DXMATRIX* matrix, UINT elements)
{
    unsigned int i;
    TRACE("\n");
    for (i = 0; i < elements; ++i) {
        D3DXVec2Transform(
            (D3DXVECTOR4*)((char*)out + outstride * i),
            (CONST D3DXVECTOR2*)((const char*)in + instride * i),
            matrix);
    }
    return out;
}

/*************************************************************************
 * D3DXVec2TransformCoordArray
 */
D3DXVECTOR2* WINAPI D3DXVec2TransformCoordArray(
    D3DXVECTOR2* out, UINT outstride, CONST D3DXVECTOR2* in, UINT instride,
    CONST D3DXMATRIX* matrix, UINT elements)
{
    unsigned int i;
    TRACE("\n");
    for (i = 0; i < elements; ++i) {
        D3DXVec2TransformCoord(
            (D3DXVECTOR2*)((char*)out + outstride * i),
            (CONST D3DXVECTOR2*)((const char*)in + instride * i),
            matrix);
    }
    return out;
}

/*************************************************************************
 * D3DXVec2TransformNormalArray
 */
D3DXVECTOR2* WINAPI D3DXVec2TransformNormalArray(
    D3DXVECTOR2* out, UINT outstride, CONST D3DXVECTOR2 *in, UINT instride,
    CONST D3DXMATRIX *matrix, UINT elements)
{
    unsigned int i;
    TRACE("\n");
    for (i = 0; i < elements; ++i) {
        D3DXVec2TransformNormal(
            (D3DXVECTOR2*)((char*)out + outstride * i),
            (CONST D3DXVECTOR2*)((const char*)in + instride * i),
            matrix);
    }
    return out;
}

/*************************************************************************
 * D3DXVec3ProjectArray
 *
 * Projects an array of vectors to the screen.
 */
D3DXVECTOR3* WINAPI D3DXVec3ProjectArray(
    D3DXVECTOR3* out, UINT outstride, CONST D3DXVECTOR3* in, UINT instride,
    CONST D3DVIEWPORT9* viewport, CONST D3DXMATRIX* projection,
    CONST D3DXMATRIX* view, CONST D3DXMATRIX* world, UINT elements)
{
    unsigned int i;
    TRACE("\n");
    for (i = 0; i < elements; ++i) {
        D3DXVec3Project(
            (D3DXVECTOR3*)((char*)out + outstride * i),
            (CONST D3DXVECTOR3*)((const char*)in + instride * i),
            viewport, projection, view, world);
    }
    return out;
}

/*************************************************************************
 * D3DXVec3TransformArray
 */
D3DXVECTOR4* WINAPI D3DXVec3TransformArray(
    D3DXVECTOR4* out, UINT outstride, CONST D3DXVECTOR3* in, UINT instride,
    CONST D3DXMATRIX* matrix, UINT elements)
{
    unsigned int i;
    TRACE("\n");
    for (i = 0; i < elements; ++i) {
        D3DXVec3Transform(
            (D3DXVECTOR4*)((char*)out + outstride * i),
            (CONST D3DXVECTOR3*)((const char*)in + instride * i),
            matrix);
    }
    return out;
}

/*************************************************************************
 * D3DXVec3TransformCoordArray
 */
D3DXVECTOR3* WINAPI D3DXVec3TransformCoordArray(
    D3DXVECTOR3* out, UINT outstride, CONST D3DXVECTOR3* in, UINT instride,
    CONST D3DXMATRIX* matrix, UINT elements)
{
    unsigned int i;
    TRACE("\n");
    for (i = 0; i < elements; ++i) {
        D3DXVec3TransformCoord(
            (D3DXVECTOR3*)((char*)out + outstride * i),
            (CONST D3DXVECTOR3*)((const char*)in + instride * i),
            matrix);
    }
    return out;
}

/*************************************************************************
 * D3DXVec3TransformNormalArray
 */
D3DXVECTOR3* WINAPI D3DXVec3TransformNormalArray(
    D3DXVECTOR3* out, UINT outstride, CONST D3DXVECTOR3* in, UINT instride,
    CONST D3DXMATRIX* matrix, UINT elements)
{
    unsigned int i;
    TRACE("\n");
    for (i = 0; i < elements; ++i) {
        D3DXVec3TransformNormal(
            (D3DXVECTOR3*)((char*)out + outstride * i),
            (CONST D3DXVECTOR3*)((const char*)in + instride * i),
            matrix);
    }
    return out;
}

/*************************************************************************
 * D3DXVec3UnprojectArray
 */
D3DXVECTOR3* WINAPI D3DXVec3UnprojectArray(
    D3DXVECTOR3* out, UINT outstride, CONST D3DXVECTOR3* in, UINT instride,
    CONST D3DVIEWPORT9* viewport, CONST D3DXMATRIX* projection,
    CONST D3DXMATRIX* view, CONST D3DXMATRIX* world, UINT elements)
{
    unsigned int i;
    TRACE("\n");
    for (i = 0; i < elements; ++i) {
        D3DXVec3Unproject(
            (D3DXVECTOR3*)((char*)out + outstride * i),
            (CONST D3DXVECTOR3*)((const char*)in + instride * i),
            viewport, projection, view, world);
    }
    return out;
}

/*************************************************************************
 * D3DXVec4TransformArray
 */
D3DXVECTOR4* WINAPI D3DXVec4TransformArray(
    D3DXVECTOR4* out, UINT outstride, CONST D3DXVECTOR4* in, UINT instride,
    CONST D3DXMATRIX* matrix, UINT elements)
{
    unsigned int i;
    TRACE("\n");
    for (i = 0; i < elements; ++i) {
        D3DXVec4Transform(
            (D3DXVECTOR4*)((char*)out + outstride * i),
            (CONST D3DXVECTOR4*)((const char*)in + instride * i),
            matrix);
    }
    return out;
}
