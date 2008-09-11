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
 * D3DXMatrixDecompose
 */
HRESULT WINAPI D3DXMatrixDecompose(D3DXVECTOR3 *poutscale, D3DXQUATERNION *poutrotation, D3DXVECTOR3 *pouttranslation, D3DXMATRIX *pm)
{
    D3DXMATRIX normalized;
    D3DXVECTOR3 vec;

    if (!pm)
    {
     return D3DERR_INVALIDCALL;
    }

    /*Compute the scaling part.*/
    vec.x=pm->m[0][0];
    vec.y=pm->m[0][1];
    vec.z=pm->m[0][2];
    poutscale->x=D3DXVec3Length(&vec);

    vec.x=pm->m[1][0];
    vec.y=pm->m[1][1];
    vec.z=pm->m[1][2];
    poutscale->y=D3DXVec3Length(&vec);

    vec.x=pm->m[2][0];
    vec.y=pm->m[2][1];
    vec.z=pm->m[2][2];
    poutscale->z=D3DXVec3Length(&vec);

    /*Compute the translation part.*/
    pouttranslation->x=pm->m[3][0];
    pouttranslation->y=pm->m[3][1];
    pouttranslation->z=pm->m[3][2];

    /*Let's calculate the rotation now*/
    if ( (poutscale->x == 0.0f) || (poutscale->y == 0.0f) || (poutscale->z == 0.0f) )
    {
     return D3DERR_INVALIDCALL;
    }

    normalized.m[0][0]=pm->m[0][0]/poutscale->x;
    normalized.m[0][1]=pm->m[0][1]/poutscale->x;
    normalized.m[0][2]=pm->m[0][2]/poutscale->x;
    normalized.m[1][0]=pm->m[1][0]/poutscale->y;
    normalized.m[1][1]=pm->m[1][1]/poutscale->y;
    normalized.m[1][2]=pm->m[1][2]/poutscale->y;
    normalized.m[2][0]=pm->m[2][0]/poutscale->z;
    normalized.m[2][1]=pm->m[2][1]/poutscale->z;
    normalized.m[2][2]=pm->m[2][2]/poutscale->z;

    D3DXQuaternionRotationMatrix(poutrotation,&normalized);
    return S_OK;
}

/*************************************************************************
 * D3DXPlaneTransformArray
 */
D3DXPLANE* WINAPI D3DXPlaneTransformArray(
    D3DXPLANE* out, UINT outstride, CONST D3DXPLANE* in, UINT instride,
    CONST D3DXMATRIX* matrix, UINT elements)
{
    UINT i;
    TRACE("\n");
    for (i = 0; i < elements; ++i) {
        D3DXPlaneTransform(
            (D3DXPLANE*)((char*)out + outstride * i),
            (CONST D3DXPLANE*)((const char*)in + instride * i),
            matrix);
    }
    return out;
}

/*************************************************************************
 * D3DXVec2TransformArray
 *
 * Transform an array of vectors by a matrix.
 */
D3DXVECTOR4* WINAPI D3DXVec2TransformArray(
    D3DXVECTOR4* out, UINT outstride, CONST D3DXVECTOR2* in, UINT instride,
    CONST D3DXMATRIX* matrix, UINT elements)
{
    UINT i;
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
    UINT i;
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
    UINT i;
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
    UINT i;
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
    UINT i;
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
    UINT i;
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
    UINT i;
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
    UINT i;
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
    UINT i;
    TRACE("\n");
    for (i = 0; i < elements; ++i) {
        D3DXVec4Transform(
            (D3DXVECTOR4*)((char*)out + outstride * i),
            (CONST D3DXVECTOR4*)((const char*)in + instride * i),
            matrix);
    }
    return out;
}
