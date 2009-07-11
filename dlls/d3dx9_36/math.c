/*
 * Mathematical operations specific to D3DX9.
 *
 * Copyright (C) 2008 David Adam
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

#define NONAMELESSUNION

#include "config.h"
#include "windef.h"
#include "wingdi.h"
#include "d3dx9.h"


/*************************************************************************
 * D3DXMatrixAffineTransformation2D
 */
D3DXMATRIX* WINAPI D3DXMatrixAffineTransformation2D(
    D3DXMATRIX *pout, FLOAT scaling,
    CONST D3DXVECTOR2 *protationcenter, FLOAT rotation,
    CONST D3DXVECTOR2 *ptranslation)
{
    D3DXMATRIX m1, m2, m3, m4, m5;
    D3DXQUATERNION rot;
    D3DXVECTOR3 rot_center, trans;

    rot.w=cos(rotation/2.0f);
    rot.x=0.0f;
    rot.y=0.0f;
    rot.z=sin(rotation/2.0f);

    if ( protationcenter )
    {
        rot_center.x=protationcenter->x;
        rot_center.y=protationcenter->y;
        rot_center.z=0.0f;
    }
    else
    {
        rot_center.x=0.0f;
        rot_center.y=0.0f;
        rot_center.z=0.0f;
    }

    if ( ptranslation )
    {
        trans.x=ptranslation->x;
        trans.y=ptranslation->y;
        trans.z=0.0f;
    }
    else
    {
        trans.x=0.0f;
        trans.y=0.0f;
        trans.z=0.0f;
    }

    D3DXMatrixScaling(&m1, scaling, scaling, 1.0f);
    D3DXMatrixTranslation(&m2, -rot_center.x, -rot_center.y, -rot_center.z);
    D3DXMatrixTranslation(&m4, rot_center.x, rot_center.y, rot_center.z);
    D3DXMatrixRotationQuaternion(&m3, &rot);
    D3DXMatrixTranslation(&m5, trans.x, trans.y, trans.z);

    D3DXMatrixMultiply(&m1, &m1, &m2);
    D3DXMatrixMultiply(&m1, &m1, &m3);
    D3DXMatrixMultiply(&m1, &m1, &m4);
    D3DXMatrixMultiply(pout, &m1, &m5);

    return pout;
}

/*************************************************************************
 * D3DXMatrixDecompose
 */
HRESULT WINAPI D3DXMatrixDecompose(D3DXVECTOR3 *poutscale, D3DXQUATERNION *poutrotation, D3DXVECTOR3 *pouttranslation, CONST D3DXMATRIX *pm)
{
    D3DXMATRIX normalized;
    D3DXVECTOR3 vec;

    /*Compute the scaling part.*/
    vec.x=pm->u.m[0][0];
    vec.y=pm->u.m[0][1];
    vec.z=pm->u.m[0][2];
    poutscale->x=D3DXVec3Length(&vec);

    vec.x=pm->u.m[1][0];
    vec.y=pm->u.m[1][1];
    vec.z=pm->u.m[1][2];
    poutscale->y=D3DXVec3Length(&vec);

    vec.x=pm->u.m[2][0];
    vec.y=pm->u.m[2][1];
    vec.z=pm->u.m[2][2];
    poutscale->z=D3DXVec3Length(&vec);

    /*Compute the translation part.*/
    pouttranslation->x=pm->u.m[3][0];
    pouttranslation->y=pm->u.m[3][1];
    pouttranslation->z=pm->u.m[3][2];

    /*Let's calculate the rotation now*/
    if ( (poutscale->x == 0.0f) || (poutscale->y == 0.0f) || (poutscale->z == 0.0f) )
    {
     return D3DERR_INVALIDCALL;
    }

    normalized.u.m[0][0]=pm->u.m[0][0]/poutscale->x;
    normalized.u.m[0][1]=pm->u.m[0][1]/poutscale->x;
    normalized.u.m[0][2]=pm->u.m[0][2]/poutscale->x;
    normalized.u.m[1][0]=pm->u.m[1][0]/poutscale->y;
    normalized.u.m[1][1]=pm->u.m[1][1]/poutscale->y;
    normalized.u.m[1][2]=pm->u.m[1][2]/poutscale->y;
    normalized.u.m[2][0]=pm->u.m[2][0]/poutscale->z;
    normalized.u.m[2][1]=pm->u.m[2][1]/poutscale->z;
    normalized.u.m[2][2]=pm->u.m[2][2]/poutscale->z;

    D3DXQuaternionRotationMatrix(poutrotation,&normalized);
    return S_OK;
}

/*************************************************************************
 * D3DXMatrixTransformation2D
 */
D3DXMATRIX* WINAPI D3DXMatrixTransformation2D(
    D3DXMATRIX *pout, CONST D3DXVECTOR2 *pscalingcenter,
    FLOAT scalingrotation, CONST D3DXVECTOR2 *pscaling,
    CONST D3DXVECTOR2 *protationcenter, FLOAT rotation,
    CONST D3DXVECTOR2 *ptranslation)
{
    D3DXQUATERNION rot, sca_rot;
    D3DXVECTOR3 rot_center, sca, sca_center, trans;

    if ( pscalingcenter )
    {
        sca_center.x=pscalingcenter->x;
        sca_center.y=pscalingcenter->y;
        sca_center.z=0.0f;
    }
    else
    {
        sca_center.x=0.0f;
        sca_center.y=0.0f;
        sca_center.z=0.0f;
    }

    if ( pscaling )
    {
        sca.x=pscaling->x;
        sca.y=pscaling->y;
        sca.z=1.0f;
    }
    else
    {
        sca.x=1.0f;
        sca.y=1.0f;
        sca.z=1.0f;
    }

    if ( protationcenter )
    {
        rot_center.x=protationcenter->x;
        rot_center.y=protationcenter->y;
        rot_center.z=0.0f;
    }
    else
    {
        rot_center.x=0.0f;
        rot_center.y=0.0f;
        rot_center.z=0.0f;
    }

    if ( ptranslation )
    {
        trans.x=ptranslation->x;
        trans.y=ptranslation->y;
        trans.z=0.0f;
    }
    else
    {
        trans.x=0.0f;
        trans.y=0.0f;
        trans.z=0.0f;
    }

    rot.w=cos(rotation/2.0f);
    rot.x=0.0f;
    rot.y=0.0f;
    rot.z=sin(rotation/2.0f);

    sca_rot.w=cos(scalingrotation/2.0f);
    sca_rot.x=0.0f;
    sca_rot.y=0.0f;
    sca_rot.z=sin(scalingrotation/2.0f);

    D3DXMatrixTransformation(pout, &sca_center, &sca_rot, &sca, &rot_center, &rot, &trans);

    return pout;
}

/*************************************************************************
 * D3DXPlaneTransformArray
 */
D3DXPLANE* WINAPI D3DXPlaneTransformArray(
    D3DXPLANE* out, UINT outstride, CONST D3DXPLANE* in, UINT instride,
    CONST D3DXMATRIX* matrix, UINT elements)
{
    UINT i;

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

    for (i = 0; i < elements; ++i) {
        D3DXVec4Transform(
            (D3DXVECTOR4*)((char*)out + outstride * i),
            (CONST D3DXVECTOR4*)((const char*)in + instride * i),
            matrix);
    }
    return out;
}
