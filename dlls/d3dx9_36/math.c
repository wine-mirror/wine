/*
 * Mathematical operations specific to D3DX9.
 *
 * Copyright (C) 2008 David Adam
 * Copyright (C) 2008 Luis Busquets
 * Copyright (C) 2008 Jérôme Gardou
 * Copyright (C) 2008 Philip Nilsson
 * Copyright (C) 2008 Henri Verbeet
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
#include "wine/port.h"

#include "windef.h"
#include "wingdi.h"
#include "d3dx9_36_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);

static const ID3DXMatrixStackVtbl ID3DXMatrixStack_Vtbl;

typedef struct ID3DXMatrixStackImpl
{
  ID3DXMatrixStack ID3DXMatrixStack_iface;
  LONG ref;

  unsigned int current;
  unsigned int stack_size;
  D3DXMATRIX *stack;
} ID3DXMatrixStackImpl;


/*_________________D3DXColor____________________*/

D3DXCOLOR* WINAPI D3DXColorAdjustContrast(D3DXCOLOR *pout, CONST D3DXCOLOR *pc, FLOAT s)
{
    TRACE("(%p, %p, %f)\n", pout, pc, s);

    pout->r = 0.5f + s * (pc->r - 0.5f);
    pout->g = 0.5f + s * (pc->g - 0.5f);
    pout->b = 0.5f + s * (pc->b - 0.5f);
    pout->a = pc->a;
    return pout;
}

D3DXCOLOR* WINAPI D3DXColorAdjustSaturation(D3DXCOLOR *pout, CONST D3DXCOLOR *pc, FLOAT s)
{
    FLOAT grey;

    TRACE("(%p, %p, %f)\n", pout, pc, s);

    grey = pc->r * 0.2125f + pc->g * 0.7154f + pc->b * 0.0721f;
    pout->r = grey + s * (pc->r - grey);
    pout->g = grey + s * (pc->g - grey);
    pout->b = grey + s * (pc->b - grey);
    pout->a = pc->a;
    return pout;
}

/*_________________Misc__________________________*/

FLOAT WINAPI D3DXFresnelTerm(FLOAT costheta, FLOAT refractionindex)
{
    FLOAT a, d, g, result;

    TRACE("(%f, %f)\n", costheta, refractionindex);

    g = sqrt(refractionindex * refractionindex + costheta * costheta - 1.0f);
    a = g + costheta;
    d = g - costheta;
    result = ( costheta * a - 1.0f ) * ( costheta * a - 1.0f ) / ( ( costheta * d + 1.0f ) * ( costheta * d + 1.0f ) ) + 1.0f;
    result = result * 0.5f * d * d / ( a * a );
    return result;
}

/*_________________D3DXMatrix____________________*/

D3DXMATRIX* WINAPI D3DXMatrixAffineTransformation(D3DXMATRIX *pout, FLOAT scaling, CONST D3DXVECTOR3 *rotationcenter, CONST D3DXQUATERNION *rotation, CONST D3DXVECTOR3 *translation)
{
    D3DXMATRIX m1, m2, m3, m4, m5;

    TRACE("(%p, %f, %p, %p, %p)\n", pout, scaling, rotationcenter, rotation, translation);

    D3DXMatrixScaling(&m1, scaling, scaling, scaling);

    if ( !rotationcenter )
    {
        D3DXMatrixIdentity(&m2);
        D3DXMatrixIdentity(&m4);
    }
    else
    {
        D3DXMatrixTranslation(&m2, -rotationcenter->x, -rotationcenter->y, -rotationcenter->z);
        D3DXMatrixTranslation(&m4, rotationcenter->x, rotationcenter->y, rotationcenter->z);
    }

    if ( !rotation ) D3DXMatrixIdentity(&m3);
    else D3DXMatrixRotationQuaternion(&m3, rotation);

    if ( !translation ) D3DXMatrixIdentity(&m5);
    else D3DXMatrixTranslation(&m5, translation->x, translation->y, translation->z);

    D3DXMatrixMultiply(&m1, &m1, &m2);
    D3DXMatrixMultiply(&m1, &m1, &m3);
    D3DXMatrixMultiply(&m1, &m1, &m4);
    D3DXMatrixMultiply(pout, &m1, &m5);
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixAffineTransformation2D(D3DXMATRIX *pout, FLOAT scaling, CONST D3DXVECTOR2 *protationcenter, FLOAT rotation, CONST D3DXVECTOR2 *ptranslation)
{
    D3DXMATRIX m1, m2, m3, m4, m5;
    D3DXQUATERNION rot;
    D3DXVECTOR3 rot_center, trans;

    TRACE("(%p, %f, %p, %f, %p)\n", pout, scaling, protationcenter, rotation, ptranslation);

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

HRESULT WINAPI D3DXMatrixDecompose(D3DXVECTOR3 *poutscale, D3DXQUATERNION *poutrotation, D3DXVECTOR3 *pouttranslation, CONST D3DXMATRIX *pm)
{
    D3DXMATRIX normalized;
    D3DXVECTOR3 vec;

    TRACE("(%p, %p, %p, %p)\n", poutscale, poutrotation, pouttranslation, pm);

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
    if ( (poutscale->x == 0.0f) || (poutscale->y == 0.0f) || (poutscale->z == 0.0f) ) return D3DERR_INVALIDCALL;

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

FLOAT WINAPI D3DXMatrixDeterminant(CONST D3DXMATRIX *pm)
{
    D3DXVECTOR4 minor, v1, v2, v3;
    FLOAT det;

    TRACE("(%p)\n", pm);

    v1.x = pm->u.m[0][0]; v1.y = pm->u.m[1][0]; v1.z = pm->u.m[2][0]; v1.w = pm->u.m[3][0];
    v2.x = pm->u.m[0][1]; v2.y = pm->u.m[1][1]; v2.z = pm->u.m[2][1]; v2.w = pm->u.m[3][1];
    v3.x = pm->u.m[0][2]; v3.y = pm->u.m[1][2]; v3.z = pm->u.m[2][2]; v3.w = pm->u.m[3][2];
    D3DXVec4Cross(&minor, &v1, &v2, &v3);
    det =  - (pm->u.m[0][3] * minor.x + pm->u.m[1][3] * minor.y + pm->u.m[2][3] * minor.z + pm->u.m[3][3] * minor.w);
    return det;
}

D3DXMATRIX* WINAPI D3DXMatrixInverse(D3DXMATRIX *pout, FLOAT *pdeterminant, CONST D3DXMATRIX *pm)
{
    int a, i, j;
    D3DXMATRIX out;
    D3DXVECTOR4 v, vec[3];
    FLOAT det;

    TRACE("(%p, %p, %p)\n", pout, pdeterminant, pm);

    det = D3DXMatrixDeterminant(pm);
    if ( !det ) return NULL;
    if ( pdeterminant ) *pdeterminant = det;
    for (i=0; i<4; i++)
    {
        for (j=0; j<4; j++)
        {
            if (j != i )
            {
                a = j;
                if ( j > i ) a = a-1;
                vec[a].x = pm->u.m[j][0];
                vec[a].y = pm->u.m[j][1];
                vec[a].z = pm->u.m[j][2];
                vec[a].w = pm->u.m[j][3];
            }
        }
    D3DXVec4Cross(&v, &vec[0], &vec[1], &vec[2]);
    out.u.m[0][i] = pow(-1.0f, i) * v.x / det;
    out.u.m[1][i] = pow(-1.0f, i) * v.y / det;
    out.u.m[2][i] = pow(-1.0f, i) * v.z / det;
    out.u.m[3][i] = pow(-1.0f, i) * v.w / det;
   }

   *pout = out;
   return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixLookAtLH(D3DXMATRIX *pout, CONST D3DXVECTOR3 *peye, CONST D3DXVECTOR3 *pat, CONST D3DXVECTOR3 *pup)
{
    D3DXVECTOR3 right, rightn, up, upn, vec, vec2;

    TRACE("(%p, %p, %p, %p)\n", pout, peye, pat, pup);

    D3DXVec3Subtract(&vec2, pat, peye);
    D3DXVec3Normalize(&vec, &vec2);
    D3DXVec3Cross(&right, pup, &vec);
    D3DXVec3Cross(&up, &vec, &right);
    D3DXVec3Normalize(&rightn, &right);
    D3DXVec3Normalize(&upn, &up);
    pout->u.m[0][0] = rightn.x;
    pout->u.m[1][0] = rightn.y;
    pout->u.m[2][0] = rightn.z;
    pout->u.m[3][0] = -D3DXVec3Dot(&rightn,peye);
    pout->u.m[0][1] = upn.x;
    pout->u.m[1][1] = upn.y;
    pout->u.m[2][1] = upn.z;
    pout->u.m[3][1] = -D3DXVec3Dot(&upn, peye);
    pout->u.m[0][2] = vec.x;
    pout->u.m[1][2] = vec.y;
    pout->u.m[2][2] = vec.z;
    pout->u.m[3][2] = -D3DXVec3Dot(&vec, peye);
    pout->u.m[0][3] = 0.0f;
    pout->u.m[1][3] = 0.0f;
    pout->u.m[2][3] = 0.0f;
    pout->u.m[3][3] = 1.0f;
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixLookAtRH(D3DXMATRIX *pout, CONST D3DXVECTOR3 *peye, CONST D3DXVECTOR3 *pat, CONST D3DXVECTOR3 *pup)
{
    D3DXVECTOR3 right, rightn, up, upn, vec, vec2;

    TRACE("(%p, %p, %p, %p)\n", pout, peye, pat, pup);

    D3DXVec3Subtract(&vec2, pat, peye);
    D3DXVec3Normalize(&vec, &vec2);
    D3DXVec3Cross(&right, pup, &vec);
    D3DXVec3Cross(&up, &vec, &right);
    D3DXVec3Normalize(&rightn, &right);
    D3DXVec3Normalize(&upn, &up);
    pout->u.m[0][0] = -rightn.x;
    pout->u.m[1][0] = -rightn.y;
    pout->u.m[2][0] = -rightn.z;
    pout->u.m[3][0] = D3DXVec3Dot(&rightn,peye);
    pout->u.m[0][1] = upn.x;
    pout->u.m[1][1] = upn.y;
    pout->u.m[2][1] = upn.z;
    pout->u.m[3][1] = -D3DXVec3Dot(&upn, peye);
    pout->u.m[0][2] = -vec.x;
    pout->u.m[1][2] = -vec.y;
    pout->u.m[2][2] = -vec.z;
    pout->u.m[3][2] = D3DXVec3Dot(&vec, peye);
    pout->u.m[0][3] = 0.0f;
    pout->u.m[1][3] = 0.0f;
    pout->u.m[2][3] = 0.0f;
    pout->u.m[3][3] = 1.0f;
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixMultiply(D3DXMATRIX *pout, CONST D3DXMATRIX *pm1, CONST D3DXMATRIX *pm2)
{
    D3DXMATRIX out;
    int i,j;

    TRACE("(%p, %p, %p)\n", pout, pm1, pm2);

    for (i=0; i<4; i++)
    {
        for (j=0; j<4; j++)
        {
            out.u.m[i][j] = pm1->u.m[i][0] * pm2->u.m[0][j] + pm1->u.m[i][1] * pm2->u.m[1][j] + pm1->u.m[i][2] * pm2->u.m[2][j] + pm1->u.m[i][3] * pm2->u.m[3][j];
        }
    }

    *pout = out;
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixMultiplyTranspose(D3DXMATRIX *pout, CONST D3DXMATRIX *pm1, CONST D3DXMATRIX *pm2)
{
    TRACE("%p, %p, %p)\n", pout, pm1, pm2);

    D3DXMatrixMultiply(pout, pm1, pm2);
    D3DXMatrixTranspose(pout, pout);
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixOrthoLH(D3DXMATRIX *pout, FLOAT w, FLOAT h, FLOAT zn, FLOAT zf)
{
    TRACE("(%p, %f, %f, %f, %f)\n", pout, w, h, zn, zf);

    D3DXMatrixIdentity(pout);
    pout->u.m[0][0] = 2.0f / w;
    pout->u.m[1][1] = 2.0f / h;
    pout->u.m[2][2] = 1.0f / (zf - zn);
    pout->u.m[3][2] = zn / (zn - zf);
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixOrthoOffCenterLH(D3DXMATRIX *pout, FLOAT l, FLOAT r, FLOAT b, FLOAT t, FLOAT zn, FLOAT zf)
{
    TRACE("(%p, %f, %f, %f, %f, %f, %f)\n", pout, l, r, b, t, zn, zf);

    D3DXMatrixIdentity(pout);
    pout->u.m[0][0] = 2.0f / (r - l);
    pout->u.m[1][1] = 2.0f / (t - b);
    pout->u.m[2][2] = 1.0f / (zf -zn);
    pout->u.m[3][0] = -1.0f -2.0f *l / (r - l);
    pout->u.m[3][1] = 1.0f + 2.0f * t / (b - t);
    pout->u.m[3][2] = zn / (zn -zf);
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixOrthoOffCenterRH(D3DXMATRIX *pout, FLOAT l, FLOAT r, FLOAT b, FLOAT t, FLOAT zn, FLOAT zf)
{
    TRACE("(%p, %f, %f, %f, %f, %f, %f)\n", pout, l, r, b, t, zn, zf);

    D3DXMatrixIdentity(pout);
    pout->u.m[0][0] = 2.0f / (r - l);
    pout->u.m[1][1] = 2.0f / (t - b);
    pout->u.m[2][2] = 1.0f / (zn -zf);
    pout->u.m[3][0] = -1.0f -2.0f *l / (r - l);
    pout->u.m[3][1] = 1.0f + 2.0f * t / (b - t);
    pout->u.m[3][2] = zn / (zn -zf);
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixOrthoRH(D3DXMATRIX *pout, FLOAT w, FLOAT h, FLOAT zn, FLOAT zf)
{
    TRACE("(%p, %f, %f, %f, %f)\n", pout, w, h, zn, zf);

    D3DXMatrixIdentity(pout);
    pout->u.m[0][0] = 2.0f / w;
    pout->u.m[1][1] = 2.0f / h;
    pout->u.m[2][2] = 1.0f / (zn - zf);
    pout->u.m[3][2] = zn / (zn - zf);
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixPerspectiveFovLH(D3DXMATRIX *pout, FLOAT fovy, FLOAT aspect, FLOAT zn, FLOAT zf)
{
    TRACE("(%p, %f, %f, %f, %f)\n", pout, fovy, aspect, zn, zf);

    D3DXMatrixIdentity(pout);
    pout->u.m[0][0] = 1.0f / (aspect * tan(fovy/2.0f));
    pout->u.m[1][1] = 1.0f / tan(fovy/2.0f);
    pout->u.m[2][2] = zf / (zf - zn);
    pout->u.m[2][3] = 1.0f;
    pout->u.m[3][2] = (zf * zn) / (zn - zf);
    pout->u.m[3][3] = 0.0f;
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixPerspectiveFovRH(D3DXMATRIX *pout, FLOAT fovy, FLOAT aspect, FLOAT zn, FLOAT zf)
{
    TRACE("(%p, %f, %f, %f, %f)\n", pout, fovy, aspect, zn, zf);

    D3DXMatrixIdentity(pout);
    pout->u.m[0][0] = 1.0f / (aspect * tan(fovy/2.0f));
    pout->u.m[1][1] = 1.0f / tan(fovy/2.0f);
    pout->u.m[2][2] = zf / (zn - zf);
    pout->u.m[2][3] = -1.0f;
    pout->u.m[3][2] = (zf * zn) / (zn - zf);
    pout->u.m[3][3] = 0.0f;
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixPerspectiveLH(D3DXMATRIX *pout, FLOAT w, FLOAT h, FLOAT zn, FLOAT zf)
{
    TRACE("(%p, %f, %f, %f, %f)\n", pout, w, h, zn, zf);

    D3DXMatrixIdentity(pout);
    pout->u.m[0][0] = 2.0f * zn / w;
    pout->u.m[1][1] = 2.0f * zn / h;
    pout->u.m[2][2] = zf / (zf - zn);
    pout->u.m[3][2] = (zn * zf) / (zn - zf);
    pout->u.m[2][3] = 1.0f;
    pout->u.m[3][3] = 0.0f;
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixPerspectiveOffCenterLH(D3DXMATRIX *pout, FLOAT l, FLOAT r, FLOAT b, FLOAT t, FLOAT zn, FLOAT zf)
{
    TRACE("(%p, %f, %f, %f, %f, %f, %f)\n", pout, l, r, b, t, zn, zf);

    D3DXMatrixIdentity(pout);
    pout->u.m[0][0] = 2.0f * zn / (r - l);
    pout->u.m[1][1] = -2.0f * zn / (b - t);
    pout->u.m[2][0] = -1.0f - 2.0f * l / (r - l);
    pout->u.m[2][1] = 1.0f + 2.0f * t / (b - t);
    pout->u.m[2][2] = - zf / (zn - zf);
    pout->u.m[3][2] = (zn * zf) / (zn -zf);
    pout->u.m[2][3] = 1.0f;
    pout->u.m[3][3] = 0.0f;
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixPerspectiveOffCenterRH(D3DXMATRIX *pout, FLOAT l, FLOAT r, FLOAT b, FLOAT t, FLOAT zn, FLOAT zf)
{
    TRACE("(%p, %f, %f, %f, %f, %f, %f)\n", pout, l, r, b, t, zn, zf);

    D3DXMatrixIdentity(pout);
    pout->u.m[0][0] = 2.0f * zn / (r - l);
    pout->u.m[1][1] = -2.0f * zn / (b - t);
    pout->u.m[2][0] = 1.0f + 2.0f * l / (r - l);
    pout->u.m[2][1] = -1.0f -2.0f * t / (b - t);
    pout->u.m[2][2] = zf / (zn - zf);
    pout->u.m[3][2] = (zn * zf) / (zn -zf);
    pout->u.m[2][3] = -1.0f;
    pout->u.m[3][3] = 0.0f;
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixPerspectiveRH(D3DXMATRIX *pout, FLOAT w, FLOAT h, FLOAT zn, FLOAT zf)
{
    TRACE("(%p, %f, %f, %f, %f)\n", pout, w, h, zn, zf);

    D3DXMatrixIdentity(pout);
    pout->u.m[0][0] = 2.0f * zn / w;
    pout->u.m[1][1] = 2.0f * zn / h;
    pout->u.m[2][2] = zf / (zn - zf);
    pout->u.m[3][2] = (zn * zf) / (zn - zf);
    pout->u.m[2][3] = -1.0f;
    pout->u.m[3][3] = 0.0f;
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixReflect(D3DXMATRIX *pout, CONST D3DXPLANE *pplane)
{
    D3DXPLANE Nplane;

    TRACE("(%p, %p)\n", pout, pplane);

    D3DXPlaneNormalize(&Nplane, pplane);
    D3DXMatrixIdentity(pout);
    pout->u.m[0][0] = 1.0f - 2.0f * Nplane.a * Nplane.a;
    pout->u.m[0][1] = -2.0f * Nplane.a * Nplane.b;
    pout->u.m[0][2] = -2.0f * Nplane.a * Nplane.c;
    pout->u.m[1][0] = -2.0f * Nplane.a * Nplane.b;
    pout->u.m[1][1] = 1.0f - 2.0f * Nplane.b * Nplane.b;
    pout->u.m[1][2] = -2.0f * Nplane.b * Nplane.c;
    pout->u.m[2][0] = -2.0f * Nplane.c * Nplane.a;
    pout->u.m[2][1] = -2.0f * Nplane.c * Nplane.b;
    pout->u.m[2][2] = 1.0f - 2.0f * Nplane.c * Nplane.c;
    pout->u.m[3][0] = -2.0f * Nplane.d * Nplane.a;
    pout->u.m[3][1] = -2.0f * Nplane.d * Nplane.b;
    pout->u.m[3][2] = -2.0f * Nplane.d * Nplane.c;
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixRotationAxis(D3DXMATRIX *pout, CONST D3DXVECTOR3 *pv, FLOAT angle)
{
    D3DXVECTOR3 v;

    TRACE("(%p, %p, %f)\n", pout, pv, angle);

    D3DXVec3Normalize(&v,pv);
    D3DXMatrixIdentity(pout);
    pout->u.m[0][0] = (1.0f - cos(angle)) * v.x * v.x + cos(angle);
    pout->u.m[1][0] = (1.0f - cos(angle)) * v.x * v.y - sin(angle) * v.z;
    pout->u.m[2][0] = (1.0f - cos(angle)) * v.x * v.z + sin(angle) * v.y;
    pout->u.m[0][1] = (1.0f - cos(angle)) * v.y * v.x + sin(angle) * v.z;
    pout->u.m[1][1] = (1.0f - cos(angle)) * v.y * v.y + cos(angle);
    pout->u.m[2][1] = (1.0f - cos(angle)) * v.y * v.z - sin(angle) * v.x;
    pout->u.m[0][2] = (1.0f - cos(angle)) * v.z * v.x - sin(angle) * v.y;
    pout->u.m[1][2] = (1.0f - cos(angle)) * v.z * v.y + sin(angle) * v.x;
    pout->u.m[2][2] = (1.0f - cos(angle)) * v.z * v.z + cos(angle);
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixRotationQuaternion(D3DXMATRIX *pout, CONST D3DXQUATERNION *pq)
{
    TRACE("(%p, %p)\n", pout, pq);

    D3DXMatrixIdentity(pout);
    pout->u.m[0][0] = 1.0f - 2.0f * (pq->y * pq->y + pq->z * pq->z);
    pout->u.m[0][1] = 2.0f * (pq->x *pq->y + pq->z * pq->w);
    pout->u.m[0][2] = 2.0f * (pq->x * pq->z - pq->y * pq->w);
    pout->u.m[1][0] = 2.0f * (pq->x * pq->y - pq->z * pq->w);
    pout->u.m[1][1] = 1.0f - 2.0f * (pq->x * pq->x + pq->z * pq->z);
    pout->u.m[1][2] = 2.0f * (pq->y *pq->z + pq->x *pq->w);
    pout->u.m[2][0] = 2.0f * (pq->x * pq->z + pq->y * pq->w);
    pout->u.m[2][1] = 2.0f * (pq->y *pq->z - pq->x *pq->w);
    pout->u.m[2][2] = 1.0f - 2.0f * (pq->x * pq->x + pq->y * pq->y);
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixRotationX(D3DXMATRIX *pout, FLOAT angle)
{
    TRACE("(%p, %f)\n", pout, angle);

    D3DXMatrixIdentity(pout);
    pout->u.m[1][1] = cos(angle);
    pout->u.m[2][2] = cos(angle);
    pout->u.m[1][2] = sin(angle);
    pout->u.m[2][1] = -sin(angle);
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixRotationY(D3DXMATRIX *pout, FLOAT angle)
{
    TRACE("(%p, %f)\n", pout, angle);

    D3DXMatrixIdentity(pout);
    pout->u.m[0][0] = cos(angle);
    pout->u.m[2][2] = cos(angle);
    pout->u.m[0][2] = -sin(angle);
    pout->u.m[2][0] = sin(angle);
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixRotationYawPitchRoll(D3DXMATRIX *pout, FLOAT yaw, FLOAT pitch, FLOAT roll)
{
    D3DXMATRIX m;

    TRACE("(%p, %f, %f, %f)\n", pout, yaw, pitch, roll);

    D3DXMatrixIdentity(pout);
    D3DXMatrixRotationZ(&m, roll);
    D3DXMatrixMultiply(pout, pout, &m);
    D3DXMatrixRotationX(&m, pitch);
    D3DXMatrixMultiply(pout, pout, &m);
    D3DXMatrixRotationY(&m, yaw);
    D3DXMatrixMultiply(pout, pout, &m);
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixRotationZ(D3DXMATRIX *pout, FLOAT angle)
{
    TRACE("(%p, %f)\n", pout, angle);

    D3DXMatrixIdentity(pout);
    pout->u.m[0][0] = cos(angle);
    pout->u.m[1][1] = cos(angle);
    pout->u.m[0][1] = sin(angle);
    pout->u.m[1][0] = -sin(angle);
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixScaling(D3DXMATRIX *pout, FLOAT sx, FLOAT sy, FLOAT sz)
{
    TRACE("(%p, %f, %f, %f)\n", pout, sx, sy, sz);

    D3DXMatrixIdentity(pout);
    pout->u.m[0][0] = sx;
    pout->u.m[1][1] = sy;
    pout->u.m[2][2] = sz;
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixShadow(D3DXMATRIX *pout, CONST D3DXVECTOR4 *plight, CONST D3DXPLANE *pplane)
{
    D3DXPLANE Nplane;
    FLOAT dot;

    TRACE("(%p, %p, %p)\n", pout, plight, pplane);

    D3DXPlaneNormalize(&Nplane, pplane);
    dot = D3DXPlaneDot(&Nplane, plight);
    pout->u.m[0][0] = dot - Nplane.a * plight->x;
    pout->u.m[0][1] = -Nplane.a * plight->y;
    pout->u.m[0][2] = -Nplane.a * plight->z;
    pout->u.m[0][3] = -Nplane.a * plight->w;
    pout->u.m[1][0] = -Nplane.b * plight->x;
    pout->u.m[1][1] = dot - Nplane.b * plight->y;
    pout->u.m[1][2] = -Nplane.b * plight->z;
    pout->u.m[1][3] = -Nplane.b * plight->w;
    pout->u.m[2][0] = -Nplane.c * plight->x;
    pout->u.m[2][1] = -Nplane.c * plight->y;
    pout->u.m[2][2] = dot - Nplane.c * plight->z;
    pout->u.m[2][3] = -Nplane.c * plight->w;
    pout->u.m[3][0] = -Nplane.d * plight->x;
    pout->u.m[3][1] = -Nplane.d * plight->y;
    pout->u.m[3][2] = -Nplane.d * plight->z;
    pout->u.m[3][3] = dot - Nplane.d * plight->w;
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixTransformation(D3DXMATRIX *pout, CONST D3DXVECTOR3 *pscalingcenter, CONST D3DXQUATERNION *pscalingrotation, CONST D3DXVECTOR3 *pscaling, CONST D3DXVECTOR3 *protationcenter, CONST D3DXQUATERNION *protation, CONST D3DXVECTOR3 *ptranslation)
{
    D3DXMATRIX m1, m2, m3, m4, m5, m6, m7;
    D3DXQUATERNION prc;
    D3DXVECTOR3 psc, pt;

    TRACE("(%p, %p, %p, %p, %p, %p, %p)\n", pout, pscalingcenter, pscalingrotation, pscaling, protationcenter, protation, ptranslation);

    if ( !pscalingcenter )
    {
        psc.x = 0.0f;
        psc.y = 0.0f;
        psc.z = 0.0f;
    }
    else
    {
        psc.x = pscalingcenter->x;
        psc.y = pscalingcenter->y;
        psc.z = pscalingcenter->z;
    }

    if ( !protationcenter )
    {
        prc.x = 0.0f;
        prc.y = 0.0f;
        prc.z = 0.0f;
    }
    else
    {
        prc.x = protationcenter->x;
        prc.y = protationcenter->y;
        prc.z = protationcenter->z;
    }

    if ( !ptranslation )
    {
        pt.x = 0.0f;
        pt.y = 0.0f;
        pt.z = 0.0f;
    }
    else
    {
        pt.x = ptranslation->x;
        pt.y = ptranslation->y;
        pt.z = ptranslation->z;
    }

    D3DXMatrixTranslation(&m1, -psc.x, -psc.y, -psc.z);

    if ( !pscalingrotation )
    {
        D3DXMatrixIdentity(&m2);
        D3DXMatrixIdentity(&m4);
    }
    else
    {
        D3DXMatrixRotationQuaternion(&m4, pscalingrotation);
        D3DXMatrixInverse(&m2, NULL, &m4);
    }

    if ( !pscaling ) D3DXMatrixIdentity(&m3);
    else D3DXMatrixScaling(&m3, pscaling->x, pscaling->y, pscaling->z);

    if ( !protation ) D3DXMatrixIdentity(&m6);
    else D3DXMatrixRotationQuaternion(&m6, protation);

    D3DXMatrixTranslation(&m5, psc.x - prc.x,  psc.y - prc.y,  psc.z - prc.z);
    D3DXMatrixTranslation(&m7, prc.x + pt.x, prc.y + pt.y, prc.z + pt.z);
    D3DXMatrixMultiply(&m1, &m1, &m2);
    D3DXMatrixMultiply(&m1, &m1, &m3);
    D3DXMatrixMultiply(&m1, &m1, &m4);
    D3DXMatrixMultiply(&m1, &m1, &m5);
    D3DXMatrixMultiply(&m1, &m1, &m6);
    D3DXMatrixMultiply(pout, &m1, &m7);
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixTransformation2D(D3DXMATRIX *pout, CONST D3DXVECTOR2 *pscalingcenter, FLOAT scalingrotation, CONST D3DXVECTOR2 *pscaling, CONST D3DXVECTOR2 *protationcenter, FLOAT rotation, CONST D3DXVECTOR2 *ptranslation)
{
    D3DXQUATERNION rot, sca_rot;
    D3DXVECTOR3 rot_center, sca, sca_center, trans;

    TRACE("(%p, %p, %f, %p, %p, %f, %p)\n", pout, pscalingcenter, scalingrotation, pscaling, protationcenter, rotation, ptranslation);

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

D3DXMATRIX* WINAPI D3DXMatrixTranslation(D3DXMATRIX *pout, FLOAT x, FLOAT y, FLOAT z)
{
    TRACE("(%p, %f, %f, %f)\n", pout, x, y, z);

    D3DXMatrixIdentity(pout);
    pout->u.m[3][0] = x;
    pout->u.m[3][1] = y;
    pout->u.m[3][2] = z;
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixTranspose(D3DXMATRIX *pout, CONST D3DXMATRIX *pm)
{
    CONST D3DXMATRIX m = *pm;
    int i,j;

    TRACE("(%p, %p)\n", pout, pm);

    for (i=0; i<4; i++)
        for (j=0; j<4; j++) pout->u.m[i][j] = m.u.m[j][i];

    return pout;
}

/*_________________D3DXMatrixStack____________________*/

static const unsigned int INITIAL_STACK_SIZE = 32;

HRESULT WINAPI D3DXCreateMatrixStack(DWORD flags, LPD3DXMATRIXSTACK* ppstack)
{
    ID3DXMatrixStackImpl* object;

    TRACE("flags %#x, ppstack %p\n", flags, ppstack);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(ID3DXMatrixStackImpl));
    if ( object == NULL )
    {
     *ppstack = NULL;
     return E_OUTOFMEMORY;
    }
    object->ID3DXMatrixStack_iface.lpVtbl = &ID3DXMatrixStack_Vtbl;
    object->ref = 1;

    object->stack = HeapAlloc(GetProcessHeap(), 0, INITIAL_STACK_SIZE * sizeof(D3DXMATRIX));
    if (!object->stack)
    {
        HeapFree(GetProcessHeap(), 0, object);
        *ppstack = NULL;
        return E_OUTOFMEMORY;
    }

    object->current = 0;
    object->stack_size = INITIAL_STACK_SIZE;
    D3DXMatrixIdentity(&object->stack[0]);

    TRACE("Created matrix stack %p\n", object);

    *ppstack = &object->ID3DXMatrixStack_iface;
    return D3D_OK;
}

static inline ID3DXMatrixStackImpl *impl_from_ID3DXMatrixStack(ID3DXMatrixStack *iface)
{
  return CONTAINING_RECORD(iface, ID3DXMatrixStackImpl, ID3DXMatrixStack_iface);
}

static HRESULT WINAPI ID3DXMatrixStackImpl_QueryInterface(ID3DXMatrixStack *iface, REFIID riid, void **out)
{
    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_ID3DXMatrixStack)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        ID3DXMatrixStack_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI ID3DXMatrixStackImpl_AddRef(ID3DXMatrixStack *iface)
{
    ID3DXMatrixStackImpl *This = impl_from_ID3DXMatrixStack(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) : AddRef from %d\n", This, ref - 1);
    return ref;
}

static ULONG WINAPI ID3DXMatrixStackImpl_Release(ID3DXMatrixStack* iface)
{
    ID3DXMatrixStackImpl *This = impl_from_ID3DXMatrixStack(iface);
    ULONG ref = InterlockedDecrement(&This->ref);
    if (!ref)
    {
        HeapFree(GetProcessHeap(), 0, This->stack);
        HeapFree(GetProcessHeap(), 0, This);
    }
    TRACE("(%p) : ReleaseRef to %d\n", This, ref);
    return ref;
}

static D3DXMATRIX* WINAPI ID3DXMatrixStackImpl_GetTop(ID3DXMatrixStack *iface)
{
    ID3DXMatrixStackImpl *This = impl_from_ID3DXMatrixStack(iface);

    TRACE("iface %p\n", iface);

    return &This->stack[This->current];
}

static HRESULT WINAPI ID3DXMatrixStackImpl_LoadIdentity(ID3DXMatrixStack *iface)
{
    ID3DXMatrixStackImpl *This = impl_from_ID3DXMatrixStack(iface);

    TRACE("iface %p\n", iface);

    D3DXMatrixIdentity(&This->stack[This->current]);

    return D3D_OK;
}

static HRESULT WINAPI ID3DXMatrixStackImpl_LoadMatrix(ID3DXMatrixStack *iface, CONST D3DXMATRIX *pm)
{
    ID3DXMatrixStackImpl *This = impl_from_ID3DXMatrixStack(iface);

    TRACE("iface %p\n", iface);

    This->stack[This->current] = *pm;

    return D3D_OK;
}

static HRESULT WINAPI ID3DXMatrixStackImpl_MultMatrix(ID3DXMatrixStack *iface, CONST D3DXMATRIX *pm)
{
    ID3DXMatrixStackImpl *This = impl_from_ID3DXMatrixStack(iface);

    TRACE("iface %p\n", iface);

    D3DXMatrixMultiply(&This->stack[This->current], &This->stack[This->current], pm);

    return D3D_OK;
}

static HRESULT WINAPI ID3DXMatrixStackImpl_MultMatrixLocal(ID3DXMatrixStack *iface, CONST D3DXMATRIX *pm)
{
    ID3DXMatrixStackImpl *This = impl_from_ID3DXMatrixStack(iface);

    TRACE("iface %p\n", iface);

    D3DXMatrixMultiply(&This->stack[This->current], pm, &This->stack[This->current]);

    return D3D_OK;
}

static HRESULT WINAPI ID3DXMatrixStackImpl_Pop(ID3DXMatrixStack *iface)
{
    ID3DXMatrixStackImpl *This = impl_from_ID3DXMatrixStack(iface);

    TRACE("iface %p\n", iface);

    /* Popping the last element on the stack returns D3D_OK, but does nothing. */
    if (!This->current) return D3D_OK;

    if (This->current <= This->stack_size / 4 && This->stack_size >= INITIAL_STACK_SIZE * 2)
    {
        unsigned int new_size;
        D3DXMATRIX *new_stack;

        new_size = This->stack_size / 2;
        new_stack = HeapReAlloc(GetProcessHeap(), 0, This->stack, new_size * sizeof(D3DXMATRIX));
        if (new_stack)
        {
            This->stack_size = new_size;
            This->stack = new_stack;
        }
    }

    --This->current;

    return D3D_OK;
}

static HRESULT WINAPI ID3DXMatrixStackImpl_Push(ID3DXMatrixStack *iface)
{
    ID3DXMatrixStackImpl *This = impl_from_ID3DXMatrixStack(iface);

    TRACE("iface %p\n", iface);

    if (This->current == This->stack_size - 1)
    {
        unsigned int new_size;
        D3DXMATRIX *new_stack;

        if (This->stack_size > UINT_MAX / 2) return E_OUTOFMEMORY;

        new_size = This->stack_size * 2;
        new_stack = HeapReAlloc(GetProcessHeap(), 0, This->stack, new_size * sizeof(D3DXMATRIX));
        if (!new_stack) return E_OUTOFMEMORY;

        This->stack_size = new_size;
        This->stack = new_stack;
    }

    ++This->current;
    This->stack[This->current] = This->stack[This->current - 1];

    return D3D_OK;
}

static HRESULT WINAPI ID3DXMatrixStackImpl_RotateAxis(ID3DXMatrixStack *iface, CONST D3DXVECTOR3 *pv, FLOAT angle)
{
    D3DXMATRIX temp;
    ID3DXMatrixStackImpl *This = impl_from_ID3DXMatrixStack(iface);

    TRACE("iface %p\n", iface);

    D3DXMatrixRotationAxis(&temp, pv, angle);
    D3DXMatrixMultiply(&This->stack[This->current], &This->stack[This->current], &temp);

    return D3D_OK;
}

static HRESULT WINAPI ID3DXMatrixStackImpl_RotateAxisLocal(ID3DXMatrixStack *iface, CONST D3DXVECTOR3 *pv, FLOAT angle)
{
    D3DXMATRIX temp;
    ID3DXMatrixStackImpl *This = impl_from_ID3DXMatrixStack(iface);

    TRACE("iface %p\n", iface);

    D3DXMatrixRotationAxis(&temp, pv, angle);
    D3DXMatrixMultiply(&This->stack[This->current], &temp, &This->stack[This->current]);

    return D3D_OK;
}

static HRESULT WINAPI ID3DXMatrixStackImpl_RotateYawPitchRoll(ID3DXMatrixStack *iface, FLOAT x, FLOAT y, FLOAT z)
{
    D3DXMATRIX temp;
    ID3DXMatrixStackImpl *This = impl_from_ID3DXMatrixStack(iface);

    TRACE("iface %p\n", iface);

    D3DXMatrixRotationYawPitchRoll(&temp, x, y, z);
    D3DXMatrixMultiply(&This->stack[This->current], &This->stack[This->current], &temp);

    return D3D_OK;
}

static HRESULT WINAPI ID3DXMatrixStackImpl_RotateYawPitchRollLocal(ID3DXMatrixStack *iface, FLOAT x, FLOAT y, FLOAT z)
{
    D3DXMATRIX temp;
    ID3DXMatrixStackImpl *This = impl_from_ID3DXMatrixStack(iface);

    TRACE("iface %p\n", iface);

    D3DXMatrixRotationYawPitchRoll(&temp, x, y, z);
    D3DXMatrixMultiply(&This->stack[This->current], &temp, &This->stack[This->current]);

    return D3D_OK;
}

static HRESULT WINAPI ID3DXMatrixStackImpl_Scale(ID3DXMatrixStack *iface, FLOAT x, FLOAT y, FLOAT z)
{
    D3DXMATRIX temp;
    ID3DXMatrixStackImpl *This = impl_from_ID3DXMatrixStack(iface);

    TRACE("iface %p\n", iface);

    D3DXMatrixScaling(&temp, x, y, z);
    D3DXMatrixMultiply(&This->stack[This->current], &This->stack[This->current], &temp);

    return D3D_OK;
}

static HRESULT WINAPI ID3DXMatrixStackImpl_ScaleLocal(ID3DXMatrixStack *iface, FLOAT x, FLOAT y, FLOAT z)
{
    D3DXMATRIX temp;
    ID3DXMatrixStackImpl *This = impl_from_ID3DXMatrixStack(iface);

    TRACE("iface %p\n", iface);

    D3DXMatrixScaling(&temp, x, y, z);
    D3DXMatrixMultiply(&This->stack[This->current], &temp, &This->stack[This->current]);

    return D3D_OK;
}

static HRESULT WINAPI ID3DXMatrixStackImpl_Translate(ID3DXMatrixStack *iface, FLOAT x, FLOAT y, FLOAT z)
{
    D3DXMATRIX temp;
    ID3DXMatrixStackImpl *This = impl_from_ID3DXMatrixStack(iface);

    TRACE("iface %p\n", iface);

    D3DXMatrixTranslation(&temp, x, y, z);
    D3DXMatrixMultiply(&This->stack[This->current], &This->stack[This->current], &temp);

    return D3D_OK;
}

static HRESULT WINAPI ID3DXMatrixStackImpl_TranslateLocal(ID3DXMatrixStack *iface, FLOAT x, FLOAT y, FLOAT z)
{
    D3DXMATRIX temp;
    ID3DXMatrixStackImpl *This = impl_from_ID3DXMatrixStack(iface);

    TRACE("iface %p\n", iface);

    D3DXMatrixTranslation(&temp, x, y, z);
    D3DXMatrixMultiply(&This->stack[This->current], &temp,&This->stack[This->current]);

    return D3D_OK;
}

static const ID3DXMatrixStackVtbl ID3DXMatrixStack_Vtbl =
{
    ID3DXMatrixStackImpl_QueryInterface,
    ID3DXMatrixStackImpl_AddRef,
    ID3DXMatrixStackImpl_Release,
    ID3DXMatrixStackImpl_Pop,
    ID3DXMatrixStackImpl_Push,
    ID3DXMatrixStackImpl_LoadIdentity,
    ID3DXMatrixStackImpl_LoadMatrix,
    ID3DXMatrixStackImpl_MultMatrix,
    ID3DXMatrixStackImpl_MultMatrixLocal,
    ID3DXMatrixStackImpl_RotateAxis,
    ID3DXMatrixStackImpl_RotateAxisLocal,
    ID3DXMatrixStackImpl_RotateYawPitchRoll,
    ID3DXMatrixStackImpl_RotateYawPitchRollLocal,
    ID3DXMatrixStackImpl_Scale,
    ID3DXMatrixStackImpl_ScaleLocal,
    ID3DXMatrixStackImpl_Translate,
    ID3DXMatrixStackImpl_TranslateLocal,
    ID3DXMatrixStackImpl_GetTop
};

/*_________________D3DXPLANE________________*/

D3DXPLANE* WINAPI D3DXPlaneFromPointNormal(D3DXPLANE *pout, CONST D3DXVECTOR3 *pvpoint, CONST D3DXVECTOR3 *pvnormal)
{
    TRACE("(%p, %p, %p)\n", pout, pvpoint, pvnormal);

    pout->a = pvnormal->x;
    pout->b = pvnormal->y;
    pout->c = pvnormal->z;
    pout->d = -D3DXVec3Dot(pvpoint, pvnormal);
    return pout;
}

D3DXPLANE* WINAPI D3DXPlaneFromPoints(D3DXPLANE *pout, CONST D3DXVECTOR3 *pv1, CONST D3DXVECTOR3 *pv2, CONST D3DXVECTOR3 *pv3)
{
    D3DXVECTOR3 edge1, edge2, normal, Nnormal;

    TRACE("(%p, %p, %p, %p)\n", pout, pv1, pv2, pv3);

    edge1.x = 0.0f; edge1.y = 0.0f; edge1.z = 0.0f;
    edge2.x = 0.0f; edge2.y = 0.0f; edge2.z = 0.0f;
    D3DXVec3Subtract(&edge1, pv2, pv1);
    D3DXVec3Subtract(&edge2, pv3, pv1);
    D3DXVec3Cross(&normal, &edge1, &edge2);
    D3DXVec3Normalize(&Nnormal, &normal);
    D3DXPlaneFromPointNormal(pout, pv1, &Nnormal);
    return pout;
}

D3DXVECTOR3* WINAPI D3DXPlaneIntersectLine(D3DXVECTOR3 *pout, CONST D3DXPLANE *pp, CONST D3DXVECTOR3 *pv1, CONST D3DXVECTOR3 *pv2)
{
    D3DXVECTOR3 direction, normal;
    FLOAT dot, temp;

    TRACE("(%p, %p, %p, %p)\n", pout, pp, pv1, pv2);

    normal.x = pp->a;
    normal.y = pp->b;
    normal.z = pp->c;
    direction.x = pv2->x - pv1->x;
    direction.y = pv2->y - pv1->y;
    direction.z = pv2->z - pv1->z;
    dot = D3DXVec3Dot(&normal, &direction);
    if ( !dot ) return NULL;
    temp = ( pp->d + D3DXVec3Dot(&normal, pv1) ) / dot;
    pout->x = pv1->x - temp * direction.x;
    pout->y = pv1->y - temp * direction.y;
    pout->z = pv1->z - temp * direction.z;
    return pout;
}

D3DXPLANE* WINAPI D3DXPlaneNormalize(D3DXPLANE *pout, CONST D3DXPLANE *pp)
{
    D3DXPLANE out;
    FLOAT norm;

    TRACE("(%p, %p)\n", pout, pp);

    norm = sqrt(pp->a * pp->a + pp->b * pp->b + pp->c * pp->c);
    if ( norm )
    {
     out.a = pp->a / norm;
     out.b = pp->b / norm;
     out.c = pp->c / norm;
     out.d = pp->d / norm;
    }
    else
    {
     out.a = 0.0f;
     out.b = 0.0f;
     out.c = 0.0f;
     out.d = 0.0f;
    }
    *pout = out;
    return pout;
}

D3DXPLANE* WINAPI D3DXPlaneTransform(D3DXPLANE *pout, CONST D3DXPLANE *pplane, CONST D3DXMATRIX *pm)
{
    CONST D3DXPLANE plane = *pplane;

    TRACE("(%p, %p, %p)\n", pout, pplane, pm);

    pout->a = pm->u.m[0][0] * plane.a + pm->u.m[1][0] * plane.b + pm->u.m[2][0] * plane.c + pm->u.m[3][0] * plane.d;
    pout->b = pm->u.m[0][1] * plane.a + pm->u.m[1][1] * plane.b + pm->u.m[2][1] * plane.c + pm->u.m[3][1] * plane.d;
    pout->c = pm->u.m[0][2] * plane.a + pm->u.m[1][2] * plane.b + pm->u.m[2][2] * plane.c + pm->u.m[3][2] * plane.d;
    pout->d = pm->u.m[0][3] * plane.a + pm->u.m[1][3] * plane.b + pm->u.m[2][3] * plane.c + pm->u.m[3][3] * plane.d;
    return pout;
}

D3DXPLANE* WINAPI D3DXPlaneTransformArray(D3DXPLANE* out, UINT outstride, CONST D3DXPLANE* in, UINT instride, CONST D3DXMATRIX* matrix, UINT elements)
{
    UINT i;

    TRACE("(%p, %u, %p, %u, %p, %u)\n", out, outstride, in, instride, matrix, elements);

    for (i = 0; i < elements; ++i) {
        D3DXPlaneTransform(
            (D3DXPLANE*)((char*)out + outstride * i),
            (CONST D3DXPLANE*)((const char*)in + instride * i),
            matrix);
    }
    return out;
}

/*_________________D3DXQUATERNION________________*/

D3DXQUATERNION* WINAPI D3DXQuaternionBaryCentric(D3DXQUATERNION *pout, CONST D3DXQUATERNION *pq1, CONST D3DXQUATERNION *pq2, CONST D3DXQUATERNION *pq3, FLOAT f, FLOAT g)
{
    D3DXQUATERNION temp1, temp2;

    TRACE("(%p, %p, %p, %p, %f, %f)\n", pout, pq1, pq2, pq3, f, g);

    D3DXQuaternionSlerp(pout, D3DXQuaternionSlerp(&temp1, pq1, pq2, f + g), D3DXQuaternionSlerp(&temp2, pq1, pq3, f+g), g / (f + g));
    return pout;
}

D3DXQUATERNION* WINAPI D3DXQuaternionExp(D3DXQUATERNION *pout, CONST D3DXQUATERNION *pq)
{
    FLOAT norm;

    TRACE("(%p, %p)\n", pout, pq);

    norm = sqrt(pq->x * pq->x + pq->y * pq->y + pq->z * pq->z);
    if (norm )
    {
     pout->x = sin(norm) * pq->x / norm;
     pout->y = sin(norm) * pq->y / norm;
     pout->z = sin(norm) * pq->z / norm;
     pout->w = cos(norm);
    }
    else
    {
     pout->x = 0.0f;
     pout->y = 0.0f;
     pout->z = 0.0f;
     pout->w = 1.0f;
    }
    return pout;
}

D3DXQUATERNION* WINAPI D3DXQuaternionInverse(D3DXQUATERNION *pout, CONST D3DXQUATERNION *pq)
{
    D3DXQUATERNION out;
    FLOAT norm;

    TRACE("(%p, %p)\n", pout, pq);

    norm = D3DXQuaternionLengthSq(pq);

    out.x = -pq->x / norm;
    out.y = -pq->y / norm;
    out.z = -pq->z / norm;
    out.w = pq->w / norm;

    *pout =out;
    return pout;
}

D3DXQUATERNION* WINAPI D3DXQuaternionLn(D3DXQUATERNION *pout, CONST D3DXQUATERNION *pq)
{
    FLOAT t;

    TRACE("(%p, %p)\n", pout, pq);

    if ( (pq->w >= 1.0f) || (pq->w == -1.0f) )
        t = 1.0f;
    else
        t = acos( pq->w ) / sqrt( 1.0f - pq->w * pq->w );

    pout->x = t * pq->x;
    pout->y = t * pq->y;
    pout->z = t * pq->z;
    pout->w = 0.0f;

    return pout;
}

D3DXQUATERNION* WINAPI D3DXQuaternionMultiply(D3DXQUATERNION *pout, CONST D3DXQUATERNION *pq1, CONST D3DXQUATERNION *pq2)
{
    D3DXQUATERNION out;

    TRACE("(%p, %p, %p)\n", pout, pq1, pq2);

    out.x = pq2->w * pq1->x + pq2->x * pq1->w + pq2->y * pq1->z - pq2->z * pq1->y;
    out.y = pq2->w * pq1->y - pq2->x * pq1->z + pq2->y * pq1->w + pq2->z * pq1->x;
    out.z = pq2->w * pq1->z + pq2->x * pq1->y - pq2->y * pq1->x + pq2->z * pq1->w;
    out.w = pq2->w * pq1->w - pq2->x * pq1->x - pq2->y * pq1->y - pq2->z * pq1->z;
    *pout = out;
    return pout;
}

D3DXQUATERNION* WINAPI D3DXQuaternionNormalize(D3DXQUATERNION *pout, CONST D3DXQUATERNION *pq)
{
    D3DXQUATERNION out;
    FLOAT norm;

    TRACE("(%p, %p)\n", pout, pq);

    norm = D3DXQuaternionLength(pq);

    out.x = pq->x / norm;
    out.y = pq->y / norm;
    out.z = pq->z / norm;
    out.w = pq->w / norm;

    *pout=out;

    return pout;
}

D3DXQUATERNION* WINAPI D3DXQuaternionRotationAxis(D3DXQUATERNION *pout, CONST D3DXVECTOR3 *pv, FLOAT angle)
{
    D3DXVECTOR3 temp;

    TRACE("(%p, %p, %f)\n", pout, pv, angle);

    D3DXVec3Normalize(&temp, pv);
    pout->x = sin( angle / 2.0f ) * temp.x;
    pout->y = sin( angle / 2.0f ) * temp.y;
    pout->z = sin( angle / 2.0f ) * temp.z;
    pout->w = cos( angle / 2.0f );
    return pout;
}

D3DXQUATERNION* WINAPI D3DXQuaternionRotationMatrix(D3DXQUATERNION *pout, CONST D3DXMATRIX *pm)
{
    int i, maxi;
    FLOAT maxdiag, S, trace;

    TRACE("(%p, %p)\n", pout, pm);

    trace = pm->u.m[0][0] + pm->u.m[1][1] + pm->u.m[2][2] + 1.0f;
    if ( trace > 1.0f)
    {
     pout->x = ( pm->u.m[1][2] - pm->u.m[2][1] ) / ( 2.0f * sqrt(trace) );
     pout->y = ( pm->u.m[2][0] - pm->u.m[0][2] ) / ( 2.0f * sqrt(trace) );
     pout->z = ( pm->u.m[0][1] - pm->u.m[1][0] ) / ( 2.0f * sqrt(trace) );
     pout->w = sqrt(trace) / 2.0f;
     return pout;
     }
    maxi = 0;
    maxdiag = pm->u.m[0][0];
    for (i=1; i<3; i++)
    {
     if ( pm->u.m[i][i] > maxdiag )
     {
      maxi = i;
      maxdiag = pm->u.m[i][i];
     }
    }
    switch( maxi )
    {
     case 0:
       S = 2.0f * sqrt(1.0f + pm->u.m[0][0] - pm->u.m[1][1] - pm->u.m[2][2]);
       pout->x = 0.25f * S;
       pout->y = ( pm->u.m[0][1] + pm->u.m[1][0] ) / S;
       pout->z = ( pm->u.m[0][2] + pm->u.m[2][0] ) / S;
       pout->w = ( pm->u.m[1][2] - pm->u.m[2][1] ) / S;
     break;
     case 1:
       S = 2.0f * sqrt(1.0f + pm->u.m[1][1] - pm->u.m[0][0] - pm->u.m[2][2]);
       pout->x = ( pm->u.m[0][1] + pm->u.m[1][0] ) / S;
       pout->y = 0.25f * S;
       pout->z = ( pm->u.m[1][2] + pm->u.m[2][1] ) / S;
       pout->w = ( pm->u.m[2][0] - pm->u.m[0][2] ) / S;
     break;
     case 2:
       S = 2.0f * sqrt(1.0f + pm->u.m[2][2] - pm->u.m[0][0] - pm->u.m[1][1]);
       pout->x = ( pm->u.m[0][2] + pm->u.m[2][0] ) / S;
       pout->y = ( pm->u.m[1][2] + pm->u.m[2][1] ) / S;
       pout->z = 0.25f * S;
       pout->w = ( pm->u.m[0][1] - pm->u.m[1][0] ) / S;
     break;
    }
    return pout;
}

D3DXQUATERNION* WINAPI D3DXQuaternionRotationYawPitchRoll(D3DXQUATERNION *pout, FLOAT yaw, FLOAT pitch, FLOAT roll)
{
    TRACE("(%p, %f, %f, %f)\n", pout, yaw, pitch, roll);

    pout->x = sin( yaw / 2.0f) * cos(pitch / 2.0f) * sin(roll / 2.0f) + cos(yaw / 2.0f) * sin(pitch / 2.0f) * cos(roll / 2.0f);
    pout->y = sin( yaw / 2.0f) * cos(pitch / 2.0f) * cos(roll / 2.0f) - cos(yaw / 2.0f) * sin(pitch / 2.0f) * sin(roll / 2.0f);
    pout->z = cos(yaw / 2.0f) * cos(pitch / 2.0f) * sin(roll / 2.0f) - sin( yaw / 2.0f) * sin(pitch / 2.0f) * cos(roll / 2.0f);
    pout->w = cos( yaw / 2.0f) * cos(pitch / 2.0f) * cos(roll / 2.0f) + sin(yaw / 2.0f) * sin(pitch / 2.0f) * sin(roll / 2.0f);
    return pout;
}

D3DXQUATERNION* WINAPI D3DXQuaternionSlerp(D3DXQUATERNION *pout, CONST D3DXQUATERNION *pq1, CONST D3DXQUATERNION *pq2, FLOAT t)
{
    FLOAT dot, epsilon, temp, theta, u;

    TRACE("(%p, %p, %p, %f)\n", pout, pq1, pq2, t);

    epsilon = 1.0f;
    temp = 1.0f - t;
    u = t;
    dot = D3DXQuaternionDot(pq1, pq2);
    if ( dot < 0.0f )
    {
        epsilon = -1.0f;
        dot = -dot;
    }
    if( 1.0f - dot > 0.001f )
    {
        theta = acos(dot);
        temp  = sin(theta * temp) / sin(theta);
        u = sin(theta * u) / sin(theta);
    }
    pout->x = temp * pq1->x + epsilon * u * pq2->x;
    pout->y = temp * pq1->y + epsilon * u * pq2->y;
    pout->z = temp * pq1->z + epsilon * u * pq2->z;
    pout->w = temp * pq1->w + epsilon * u * pq2->w;
    return pout;
}

D3DXQUATERNION* WINAPI D3DXQuaternionSquad(D3DXQUATERNION *pout, CONST D3DXQUATERNION *pq1, CONST D3DXQUATERNION *pq2, CONST D3DXQUATERNION *pq3, CONST D3DXQUATERNION *pq4, FLOAT t)
{
    D3DXQUATERNION temp1, temp2;

    TRACE("(%p, %p, %p, %p, %p, %f)\n", pout, pq1, pq2, pq3, pq4, t);

    D3DXQuaternionSlerp(pout, D3DXQuaternionSlerp(&temp1, pq1, pq4, t), D3DXQuaternionSlerp(&temp2, pq2, pq3, t), 2.0f * t * (1.0f - t));
    return pout;
}

static D3DXQUATERNION add_diff(CONST D3DXQUATERNION *q1, CONST D3DXQUATERNION *q2, CONST FLOAT add)
{
    D3DXQUATERNION temp;

    temp.x = q1->x + add * q2->x;
    temp.y = q1->y + add * q2->y;
    temp.z = q1->z + add * q2->z;
    temp.w = q1->w + add * q2->w;

    return temp;
}

void WINAPI D3DXQuaternionSquadSetup(D3DXQUATERNION *paout, D3DXQUATERNION *pbout, D3DXQUATERNION *pcout, CONST D3DXQUATERNION *pq0, CONST D3DXQUATERNION *pq1, CONST D3DXQUATERNION *pq2, CONST D3DXQUATERNION *pq3)
{
    D3DXQUATERNION q, temp1, temp2, temp3, zero;

    TRACE("(%p, %p, %p, %p, %p, %p, %p)\n", paout, pbout, pcout, pq0, pq1, pq2, pq3);

    zero.x = 0.0f;
    zero.y = 0.0f;
    zero.z = 0.0f;
    zero.w = 0.0f;

    if ( D3DXQuaternionDot(pq0, pq1) <  0.0f )
        temp2 = add_diff(&zero, pq0, -1.0f);
    else
        temp2 = *pq0;

    if ( D3DXQuaternionDot(pq1, pq2) < 0.0f )
        *pcout = add_diff(&zero, pq2, -1.0f);
    else
        *pcout = *pq2;

    if ( D3DXQuaternionDot(pcout, pq3) < 0.0f )
        temp3 = add_diff(&zero, pq3, -1.0f);
    else
        temp3 = *pq3;

    D3DXQuaternionInverse(&temp1, pq1);
    D3DXQuaternionMultiply(&temp2, &temp1, &temp2);
    D3DXQuaternionLn(&temp2, &temp2);
    D3DXQuaternionMultiply(&q, &temp1, pcout);
    D3DXQuaternionLn(&q, &q);
    temp1 = add_diff(&temp2, &q, 1.0f);
    temp1.x *= -0.25f;
    temp1.y *= -0.25f;
    temp1.z *= -0.25f;
    temp1.w *= -0.25f;
    D3DXQuaternionExp(&temp1, &temp1);
    D3DXQuaternionMultiply(paout, pq1, &temp1);

    D3DXQuaternionInverse(&temp1, pcout);
    D3DXQuaternionMultiply(&temp2, &temp1, pq1);
    D3DXQuaternionLn(&temp2, &temp2);
    D3DXQuaternionMultiply(&q, &temp1, &temp3);
    D3DXQuaternionLn(&q, &q);
    temp1 = add_diff(&temp2, &q, 1.0f);
    temp1.x *= -0.25f;
    temp1.y *= -0.25f;
    temp1.z *= -0.25f;
    temp1.w *= -0.25f;
    D3DXQuaternionExp(&temp1, &temp1);
    D3DXQuaternionMultiply(pbout, pcout, &temp1);

    return;
}

void WINAPI D3DXQuaternionToAxisAngle(CONST D3DXQUATERNION *pq, D3DXVECTOR3 *paxis, FLOAT *pangle)
{
    TRACE("(%p, %p, %p)\n", pq, paxis, pangle);

    paxis->x = pq->x;
    paxis->y = pq->y;
    paxis->z = pq->z;
    *pangle = 2.0f * acos(pq->w);
}

/*_________________D3DXVec2_____________________*/

D3DXVECTOR2* WINAPI D3DXVec2BaryCentric(D3DXVECTOR2 *pout, CONST D3DXVECTOR2 *pv1, CONST D3DXVECTOR2 *pv2, CONST D3DXVECTOR2 *pv3, FLOAT f, FLOAT g)
{
    TRACE("(%p, %p, %p, %p, %f, %f)\n", pout, pv1, pv2, pv3, f, g);

    pout->x = (1.0f-f-g) * (pv1->x) + f * (pv2->x) + g * (pv3->x);
    pout->y = (1.0f-f-g) * (pv1->y) + f * (pv2->y) + g * (pv3->y);
    return pout;
}

D3DXVECTOR2* WINAPI D3DXVec2CatmullRom(D3DXVECTOR2 *pout, CONST D3DXVECTOR2 *pv0, CONST D3DXVECTOR2 *pv1, CONST D3DXVECTOR2 *pv2, CONST D3DXVECTOR2 *pv3, FLOAT s)
{
    TRACE("(%p, %p, %p, %p, %p, %f)\n", pout, pv0, pv1, pv2, pv3, s);

    pout->x = 0.5f * (2.0f * pv1->x + (pv2->x - pv0->x) *s + (2.0f *pv0->x - 5.0f * pv1->x + 4.0f * pv2->x - pv3->x) * s * s + (pv3->x -3.0f * pv2->x + 3.0f * pv1->x - pv0->x) * s * s * s);
    pout->y = 0.5f * (2.0f * pv1->y + (pv2->y - pv0->y) *s + (2.0f *pv0->y - 5.0f * pv1->y + 4.0f * pv2->y - pv3->y) * s * s + (pv3->y -3.0f * pv2->y + 3.0f * pv1->y - pv0->y) * s * s * s);
    return pout;
}

D3DXVECTOR2* WINAPI D3DXVec2Hermite(D3DXVECTOR2 *pout, CONST D3DXVECTOR2 *pv1, CONST D3DXVECTOR2 *pt1, CONST D3DXVECTOR2 *pv2, CONST D3DXVECTOR2 *pt2, FLOAT s)
{
    FLOAT h1, h2, h3, h4;

    TRACE("(%p, %p, %p, %p, %p, %f)\n", pout, pv1, pt1, pv2, pt2, s);

    h1 = 2.0f * s * s * s - 3.0f * s * s + 1.0f;
    h2 = s * s * s - 2.0f * s * s + s;
    h3 = -2.0f * s * s * s + 3.0f * s * s;
    h4 = s * s * s - s * s;

    pout->x = h1 * (pv1->x) + h2 * (pt1->x) + h3 * (pv2->x) + h4 * (pt2->x);
    pout->y = h1 * (pv1->y) + h2 * (pt1->y) + h3 * (pv2->y) + h4 * (pt2->y);
    return pout;
}

D3DXVECTOR2* WINAPI D3DXVec2Normalize(D3DXVECTOR2 *pout, CONST D3DXVECTOR2 *pv)
{
    D3DXVECTOR2 out;
    FLOAT norm;

    TRACE("(%p, %p)\n", pout, pv);

    norm = D3DXVec2Length(pv);
    if ( !norm )
    {
     out.x = 0.0f;
     out.y = 0.0f;
    }
    else
    {
     out.x = pv->x / norm;
     out.y = pv->y / norm;
    }
    *pout=out;
    return pout;
}

D3DXVECTOR4* WINAPI D3DXVec2Transform(D3DXVECTOR4 *pout, CONST D3DXVECTOR2 *pv, CONST D3DXMATRIX *pm)
{
    TRACE("(%p, %p, %p)\n", pout, pv, pm);

    pout->x = pm->u.m[0][0] * pv->x + pm->u.m[1][0] * pv->y  + pm->u.m[3][0];
    pout->y = pm->u.m[0][1] * pv->x + pm->u.m[1][1] * pv->y  + pm->u.m[3][1];
    pout->z = pm->u.m[0][2] * pv->x + pm->u.m[1][2] * pv->y  + pm->u.m[3][2];
    pout->w = pm->u.m[0][3] * pv->x + pm->u.m[1][3] * pv->y  + pm->u.m[3][3];
    return pout;
}

D3DXVECTOR4* WINAPI D3DXVec2TransformArray(D3DXVECTOR4* out, UINT outstride, CONST D3DXVECTOR2* in, UINT instride, CONST D3DXMATRIX* matrix, UINT elements)
{
    UINT i;

    TRACE("(%p, %u, %p, %u, %p, %u)\n", out, outstride, in, instride, matrix, elements);

    for (i = 0; i < elements; ++i) {
        D3DXVec2Transform(
            (D3DXVECTOR4*)((char*)out + outstride * i),
            (CONST D3DXVECTOR2*)((const char*)in + instride * i),
            matrix);
    }
    return out;
}

D3DXVECTOR2* WINAPI D3DXVec2TransformCoord(D3DXVECTOR2 *pout, CONST D3DXVECTOR2 *pv, CONST D3DXMATRIX *pm)
{
    D3DXVECTOR2 v;
    FLOAT norm;

    TRACE("(%p, %p, %p)\n", pout, pv, pm);

    v = *pv;
    norm = pm->u.m[0][3] * pv->x + pm->u.m[1][3] * pv->y + pm->u.m[3][3];

    pout->x = (pm->u.m[0][0] * v.x + pm->u.m[1][0] * v.y + pm->u.m[3][0]) / norm;
    pout->y = (pm->u.m[0][1] * v.x + pm->u.m[1][1] * v.y + pm->u.m[3][1]) / norm;

    return pout;
}

D3DXVECTOR2* WINAPI D3DXVec2TransformCoordArray(D3DXVECTOR2* out, UINT outstride, CONST D3DXVECTOR2* in, UINT instride, CONST D3DXMATRIX* matrix, UINT elements)
{
    UINT i;

    TRACE("(%p, %u, %p, %u, %p, %u)\n", out, outstride, in, instride, matrix, elements);

    for (i = 0; i < elements; ++i) {
        D3DXVec2TransformCoord(
            (D3DXVECTOR2*)((char*)out + outstride * i),
            (CONST D3DXVECTOR2*)((const char*)in + instride * i),
            matrix);
    }
    return out;
}

D3DXVECTOR2* WINAPI D3DXVec2TransformNormal(D3DXVECTOR2 *pout, CONST D3DXVECTOR2 *pv, CONST D3DXMATRIX *pm)
{
    CONST D3DXVECTOR2 v = *pv;
    pout->x = pm->u.m[0][0] * v.x + pm->u.m[1][0] * v.y;
    pout->y = pm->u.m[0][1] * v.x + pm->u.m[1][1] * v.y;
    return pout;
}

D3DXVECTOR2* WINAPI D3DXVec2TransformNormalArray(D3DXVECTOR2* out, UINT outstride, CONST D3DXVECTOR2 *in, UINT instride, CONST D3DXMATRIX *matrix, UINT elements)
{
    UINT i;

    TRACE("(%p, %u, %p, %u, %p, %u)\n", out, outstride, in, instride, matrix, elements);

    for (i = 0; i < elements; ++i) {
        D3DXVec2TransformNormal(
            (D3DXVECTOR2*)((char*)out + outstride * i),
            (CONST D3DXVECTOR2*)((const char*)in + instride * i),
            matrix);
    }
    return out;
}

/*_________________D3DXVec3_____________________*/

D3DXVECTOR3* WINAPI D3DXVec3BaryCentric(D3DXVECTOR3 *pout, CONST D3DXVECTOR3 *pv1, CONST D3DXVECTOR3 *pv2, CONST D3DXVECTOR3 *pv3, FLOAT f, FLOAT g)
{
    TRACE("(%p, %p, %p, %p, %f, %f)\n", pout, pv1, pv2, pv3, f, g);

    pout->x = (1.0f-f-g) * (pv1->x) + f * (pv2->x) + g * (pv3->x);
    pout->y = (1.0f-f-g) * (pv1->y) + f * (pv2->y) + g * (pv3->y);
    pout->z = (1.0f-f-g) * (pv1->z) + f * (pv2->z) + g * (pv3->z);
    return pout;
}

D3DXVECTOR3* WINAPI D3DXVec3CatmullRom( D3DXVECTOR3 *pout, CONST D3DXVECTOR3 *pv0, CONST D3DXVECTOR3 *pv1, CONST D3DXVECTOR3 *pv2, CONST D3DXVECTOR3 *pv3, FLOAT s)
{
    TRACE("(%p, %p, %p, %p, %p, %f)\n", pout, pv0, pv1, pv2, pv3, s);

    pout->x = 0.5f * (2.0f * pv1->x + (pv2->x - pv0->x) *s + (2.0f *pv0->x - 5.0f * pv1->x + 4.0f * pv2->x - pv3->x) * s * s + (pv3->x -3.0f * pv2->x + 3.0f * pv1->x - pv0->x) * s * s * s);
    pout->y = 0.5f * (2.0f * pv1->y + (pv2->y - pv0->y) *s + (2.0f *pv0->y - 5.0f * pv1->y + 4.0f * pv2->y - pv3->y) * s * s + (pv3->y -3.0f * pv2->y + 3.0f * pv1->y - pv0->y) * s * s * s);
    pout->z = 0.5f * (2.0f * pv1->z + (pv2->z - pv0->z) *s + (2.0f *pv0->z - 5.0f * pv1->z + 4.0f * pv2->z - pv3->z) * s * s + (pv3->z -3.0f * pv2->z + 3.0f * pv1->z - pv0->z) * s * s * s);
    return pout;
}

D3DXVECTOR3* WINAPI D3DXVec3Hermite(D3DXVECTOR3 *pout, CONST D3DXVECTOR3 *pv1, CONST D3DXVECTOR3 *pt1, CONST D3DXVECTOR3 *pv2, CONST D3DXVECTOR3 *pt2, FLOAT s)
{
    FLOAT h1, h2, h3, h4;

    TRACE("(%p, %p, %p, %p, %p, %f)\n", pout, pv1, pt1, pv2, pt2, s);

    h1 = 2.0f * s * s * s - 3.0f * s * s + 1.0f;
    h2 = s * s * s - 2.0f * s * s + s;
    h3 = -2.0f * s * s * s + 3.0f * s * s;
    h4 = s * s * s - s * s;

    pout->x = h1 * (pv1->x) + h2 * (pt1->x) + h3 * (pv2->x) + h4 * (pt2->x);
    pout->y = h1 * (pv1->y) + h2 * (pt1->y) + h3 * (pv2->y) + h4 * (pt2->y);
    pout->z = h1 * (pv1->z) + h2 * (pt1->z) + h3 * (pv2->z) + h4 * (pt2->z);
    return pout;
}

D3DXVECTOR3* WINAPI D3DXVec3Normalize(D3DXVECTOR3 *pout, CONST D3DXVECTOR3 *pv)
{
    D3DXVECTOR3 out;
    FLOAT norm;

    TRACE("(%p, %p)\n", pout, pv);

    norm = D3DXVec3Length(pv);
    if ( !norm )
    {
     out.x = 0.0f;
     out.y = 0.0f;
     out.z = 0.0f;
    }
    else
    {
     out.x = pv->x / norm;
     out.y = pv->y / norm;
     out.z = pv->z / norm;
    }
    *pout = out;
    return pout;
}

D3DXVECTOR3* WINAPI D3DXVec3Project(D3DXVECTOR3 *pout, CONST D3DXVECTOR3 *pv, CONST D3DVIEWPORT9 *pviewport, CONST D3DXMATRIX *pprojection, CONST D3DXMATRIX *pview, CONST D3DXMATRIX *pworld)
{
    D3DXMATRIX m;
    D3DXVECTOR3 out;

    TRACE("(%p, %p, %p, %p, %p, %p)\n", pout, pv, pviewport, pprojection, pview, pworld);

    D3DXMatrixMultiply(&m, pworld, pview);
    D3DXMatrixMultiply(&m, &m, pprojection);
    D3DXVec3TransformCoord(&out, pv, &m);
    out.x = pviewport->X +  ( 1.0f + out.x ) * pviewport->Width / 2.0f;
    out.y = pviewport->Y +  ( 1.0f - out.y ) * pviewport->Height / 2.0f;
    out.z = pviewport->MinZ + out.z * ( pviewport->MaxZ - pviewport->MinZ );
    *pout = out;
    return pout;
}

D3DXVECTOR3* WINAPI D3DXVec3ProjectArray(D3DXVECTOR3* out, UINT outstride, CONST D3DXVECTOR3* in, UINT instride, CONST D3DVIEWPORT9* viewport, CONST D3DXMATRIX* projection, CONST D3DXMATRIX* view, CONST D3DXMATRIX* world, UINT elements)
{
    UINT i;

    TRACE("(%p, %u, %p, %u, %p, %p, %p, %p, %u)\n", out, outstride, in, instride, viewport, projection, view, world, elements);

    for (i = 0; i < elements; ++i) {
        D3DXVec3Project(
            (D3DXVECTOR3*)((char*)out + outstride * i),
            (CONST D3DXVECTOR3*)((const char*)in + instride * i),
            viewport, projection, view, world);
    }
    return out;
}

D3DXVECTOR4* WINAPI D3DXVec3Transform(D3DXVECTOR4 *pout, CONST D3DXVECTOR3 *pv, CONST D3DXMATRIX *pm)
{
    TRACE("(%p, %p, %p)\n", pout, pv, pm);

    pout->x = pm->u.m[0][0] * pv->x + pm->u.m[1][0] * pv->y + pm->u.m[2][0] * pv->z + pm->u.m[3][0];
    pout->y = pm->u.m[0][1] * pv->x + pm->u.m[1][1] * pv->y + pm->u.m[2][1] * pv->z + pm->u.m[3][1];
    pout->z = pm->u.m[0][2] * pv->x + pm->u.m[1][2] * pv->y + pm->u.m[2][2] * pv->z + pm->u.m[3][2];
    pout->w = pm->u.m[0][3] * pv->x + pm->u.m[1][3] * pv->y + pm->u.m[2][3] * pv->z + pm->u.m[3][3];
    return pout;
}

D3DXVECTOR4* WINAPI D3DXVec3TransformArray(D3DXVECTOR4* out, UINT outstride, CONST D3DXVECTOR3* in, UINT instride, CONST D3DXMATRIX* matrix, UINT elements)
{
    UINT i;

    TRACE("(%p, %u, %p, %u, %p, %u)\n", out, outstride, in, instride, matrix, elements);

    for (i = 0; i < elements; ++i) {
        D3DXVec3Transform(
            (D3DXVECTOR4*)((char*)out + outstride * i),
            (CONST D3DXVECTOR3*)((const char*)in + instride * i),
            matrix);
    }
    return out;
}

D3DXVECTOR3* WINAPI D3DXVec3TransformCoord(D3DXVECTOR3 *pout, CONST D3DXVECTOR3 *pv, CONST D3DXMATRIX *pm)
{
    D3DXVECTOR3 out;
    FLOAT norm;

    TRACE("(%p, %p, %p)\n", pout, pv, pm);

    norm = pm->u.m[0][3] * pv->x + pm->u.m[1][3] * pv->y + pm->u.m[2][3] *pv->z + pm->u.m[3][3];

    out.x = (pm->u.m[0][0] * pv->x + pm->u.m[1][0] * pv->y + pm->u.m[2][0] * pv->z + pm->u.m[3][0]) / norm;
    out.y = (pm->u.m[0][1] * pv->x + pm->u.m[1][1] * pv->y + pm->u.m[2][1] * pv->z + pm->u.m[3][1]) / norm;
    out.z = (pm->u.m[0][2] * pv->x + pm->u.m[1][2] * pv->y + pm->u.m[2][2] * pv->z + pm->u.m[3][2]) / norm;

    *pout = out;

    return pout;
}

D3DXVECTOR3* WINAPI D3DXVec3TransformCoordArray(D3DXVECTOR3* out, UINT outstride, CONST D3DXVECTOR3* in, UINT instride, CONST D3DXMATRIX* matrix, UINT elements)
{
    UINT i;

    TRACE("(%p, %u, %p, %u, %p, %u)\n", out, outstride, in, instride, matrix, elements);

    for (i = 0; i < elements; ++i) {
        D3DXVec3TransformCoord(
            (D3DXVECTOR3*)((char*)out + outstride * i),
            (CONST D3DXVECTOR3*)((const char*)in + instride * i),
            matrix);
    }
    return out;
}

D3DXVECTOR3* WINAPI D3DXVec3TransformNormal(D3DXVECTOR3 *pout, CONST D3DXVECTOR3 *pv, CONST D3DXMATRIX *pm)
{
    CONST D3DXVECTOR3 v = *pv;

    TRACE("(%p, %p, %p)\n", pout, pv, pm);

    pout->x = pm->u.m[0][0] * v.x + pm->u.m[1][0] * v.y + pm->u.m[2][0] * v.z;
    pout->y = pm->u.m[0][1] * v.x + pm->u.m[1][1] * v.y + pm->u.m[2][1] * v.z;
    pout->z = pm->u.m[0][2] * v.x + pm->u.m[1][2] * v.y + pm->u.m[2][2] * v.z;
    return pout;

}

D3DXVECTOR3* WINAPI D3DXVec3TransformNormalArray(D3DXVECTOR3* out, UINT outstride, CONST D3DXVECTOR3* in, UINT instride, CONST D3DXMATRIX* matrix, UINT elements)
{
    UINT i;

    TRACE("(%p, %u, %p, %u, %p, %u)\n", out, outstride, in, instride, matrix, elements);

    for (i = 0; i < elements; ++i) {
        D3DXVec3TransformNormal(
            (D3DXVECTOR3*)((char*)out + outstride * i),
            (CONST D3DXVECTOR3*)((const char*)in + instride * i),
            matrix);
    }
    return out;
}

D3DXVECTOR3* WINAPI D3DXVec3Unproject(D3DXVECTOR3 *pout, CONST D3DXVECTOR3 *pv, CONST D3DVIEWPORT9 *pviewport, CONST D3DXMATRIX *pprojection, CONST D3DXMATRIX *pview, CONST D3DXMATRIX *pworld)
{
    D3DXMATRIX m;
    D3DXVECTOR3 out;

    TRACE("(%p, %p, %p, %p, %p, %p)\n", pout, pv, pviewport, pprojection, pview, pworld);

    if (pworld) {
        D3DXMatrixMultiply(&m, pworld, pview);
        D3DXMatrixMultiply(&m, &m, pprojection);
    } else {
        D3DXMatrixMultiply(&m, pview, pprojection);
    }
    D3DXMatrixInverse(&m, NULL, &m);
    out.x = 2.0f * ( pv->x - pviewport->X ) / pviewport->Width - 1.0f;
    out.y = 1.0f - 2.0f * ( pv->y - pviewport->Y ) / pviewport->Height;
    out.z = ( pv->z - pviewport->MinZ) / ( pviewport->MaxZ - pviewport->MinZ );
    D3DXVec3TransformCoord(&out, &out, &m);
    *pout = out;
    return pout;
}

D3DXVECTOR3* WINAPI D3DXVec3UnprojectArray(D3DXVECTOR3* out, UINT outstride, CONST D3DXVECTOR3* in, UINT instride, CONST D3DVIEWPORT9* viewport, CONST D3DXMATRIX* projection, CONST D3DXMATRIX* view, CONST D3DXMATRIX* world, UINT elements)
{
    UINT i;

    TRACE("(%p, %u, %p, %u, %p, %p, %p, %p, %u)\n", out, outstride, in, instride, viewport, projection, view, world, elements);

    for (i = 0; i < elements; ++i) {
        D3DXVec3Unproject(
            (D3DXVECTOR3*)((char*)out + outstride * i),
            (CONST D3DXVECTOR3*)((const char*)in + instride * i),
            viewport, projection, view, world);
    }
    return out;
}

/*_________________D3DXVec4_____________________*/

D3DXVECTOR4* WINAPI D3DXVec4BaryCentric(D3DXVECTOR4 *pout, CONST D3DXVECTOR4 *pv1, CONST D3DXVECTOR4 *pv2, CONST D3DXVECTOR4 *pv3, FLOAT f, FLOAT g)
{
    TRACE("(%p, %p, %p, %p, %f, %f)\n", pout, pv1, pv2, pv3, f, g);

    pout->x = (1.0f-f-g) * (pv1->x) + f * (pv2->x) + g * (pv3->x);
    pout->y = (1.0f-f-g) * (pv1->y) + f * (pv2->y) + g * (pv3->y);
    pout->z = (1.0f-f-g) * (pv1->z) + f * (pv2->z) + g * (pv3->z);
    pout->w = (1.0f-f-g) * (pv1->w) + f * (pv2->w) + g * (pv3->w);
    return pout;
}

D3DXVECTOR4* WINAPI D3DXVec4CatmullRom(D3DXVECTOR4 *pout, CONST D3DXVECTOR4 *pv0, CONST D3DXVECTOR4 *pv1, CONST D3DXVECTOR4 *pv2, CONST D3DXVECTOR4 *pv3, FLOAT s)
{
    TRACE("(%p, %p, %p, %p, %p, %f)\n", pout, pv0, pv1, pv2, pv3, s);

    pout->x = 0.5f * (2.0f * pv1->x + (pv2->x - pv0->x) *s + (2.0f *pv0->x - 5.0f * pv1->x + 4.0f * pv2->x - pv3->x) * s * s + (pv3->x -3.0f * pv2->x + 3.0f * pv1->x - pv0->x) * s * s * s);
    pout->y = 0.5f * (2.0f * pv1->y + (pv2->y - pv0->y) *s + (2.0f *pv0->y - 5.0f * pv1->y + 4.0f * pv2->y - pv3->y) * s * s + (pv3->y -3.0f * pv2->y + 3.0f * pv1->y - pv0->y) * s * s * s);
    pout->z = 0.5f * (2.0f * pv1->z + (pv2->z - pv0->z) *s + (2.0f *pv0->z - 5.0f * pv1->z + 4.0f * pv2->z - pv3->z) * s * s + (pv3->z -3.0f * pv2->z + 3.0f * pv1->z - pv0->z) * s * s * s);
    pout->w = 0.5f * (2.0f * pv1->w + (pv2->w - pv0->w) *s + (2.0f *pv0->w - 5.0f * pv1->w + 4.0f * pv2->w - pv3->w) * s * s + (pv3->w -3.0f * pv2->w + 3.0f * pv1->w - pv0->w) * s * s * s);
    return pout;
}

D3DXVECTOR4* WINAPI D3DXVec4Cross(D3DXVECTOR4 *pout, CONST D3DXVECTOR4 *pv1, CONST D3DXVECTOR4 *pv2, CONST D3DXVECTOR4 *pv3)
{
    D3DXVECTOR4 out;

    TRACE("(%p, %p, %p, %p)\n", pout, pv1, pv2, pv3);

    out.x = pv1->y * (pv2->z * pv3->w - pv3->z * pv2->w) - pv1->z * (pv2->y * pv3->w - pv3->y * pv2->w) + pv1->w * (pv2->y * pv3->z - pv2->z *pv3->y);
    out.y = -(pv1->x * (pv2->z * pv3->w - pv3->z * pv2->w) - pv1->z * (pv2->x * pv3->w - pv3->x * pv2->w) + pv1->w * (pv2->x * pv3->z - pv3->x * pv2->z));
    out.z = pv1->x * (pv2->y * pv3->w - pv3->y * pv2->w) - pv1->y * (pv2->x *pv3->w - pv3->x * pv2->w) + pv1->w * (pv2->x * pv3->y - pv3->x * pv2->y);
    out.w = -(pv1->x * (pv2->y * pv3->z - pv3->y * pv2->z) - pv1->y * (pv2->x * pv3->z - pv3->x *pv2->z) + pv1->z * (pv2->x * pv3->y - pv3->x * pv2->y));
    *pout = out;
    return pout;
}

D3DXVECTOR4* WINAPI D3DXVec4Hermite(D3DXVECTOR4 *pout, CONST D3DXVECTOR4 *pv1, CONST D3DXVECTOR4 *pt1, CONST D3DXVECTOR4 *pv2, CONST D3DXVECTOR4 *pt2, FLOAT s)
{
    FLOAT h1, h2, h3, h4;

    TRACE("(%p, %p, %p, %p, %p, %f)\n", pout, pv1, pt1, pv2, pt2, s);

    h1 = 2.0f * s * s * s - 3.0f * s * s + 1.0f;
    h2 = s * s * s - 2.0f * s * s + s;
    h3 = -2.0f * s * s * s + 3.0f * s * s;
    h4 = s * s * s - s * s;

    pout->x = h1 * (pv1->x) + h2 * (pt1->x) + h3 * (pv2->x) + h4 * (pt2->x);
    pout->y = h1 * (pv1->y) + h2 * (pt1->y) + h3 * (pv2->y) + h4 * (pt2->y);
    pout->z = h1 * (pv1->z) + h2 * (pt1->z) + h3 * (pv2->z) + h4 * (pt2->z);
    pout->w = h1 * (pv1->w) + h2 * (pt1->w) + h3 * (pv2->w) + h4 * (pt2->w);
    return pout;
}

D3DXVECTOR4* WINAPI D3DXVec4Normalize(D3DXVECTOR4 *pout, CONST D3DXVECTOR4 *pv)
{
    D3DXVECTOR4 out;
    FLOAT norm;

    TRACE("(%p, %p)\n", pout, pv);

    norm = D3DXVec4Length(pv);

    out.x = pv->x / norm;
    out.y = pv->y / norm;
    out.z = pv->z / norm;
    out.w = pv->w / norm;

    *pout = out;
    return pout;
}

D3DXVECTOR4* WINAPI D3DXVec4Transform(D3DXVECTOR4 *pout, CONST D3DXVECTOR4 *pv, CONST D3DXMATRIX *pm)
{
    D3DXVECTOR4 out;

    TRACE("(%p, %p, %p)\n", pout, pv, pm);

    out.x = pm->u.m[0][0] * pv->x + pm->u.m[1][0] * pv->y + pm->u.m[2][0] * pv->z + pm->u.m[3][0] * pv->w;
    out.y = pm->u.m[0][1] * pv->x + pm->u.m[1][1] * pv->y + pm->u.m[2][1] * pv->z + pm->u.m[3][1] * pv->w;
    out.z = pm->u.m[0][2] * pv->x + pm->u.m[1][2] * pv->y + pm->u.m[2][2] * pv->z + pm->u.m[3][2] * pv->w;
    out.w = pm->u.m[0][3] * pv->x + pm->u.m[1][3] * pv->y + pm->u.m[2][3] * pv->z + pm->u.m[3][3] * pv->w;
    *pout = out;
    return pout;
}

D3DXVECTOR4* WINAPI D3DXVec4TransformArray(D3DXVECTOR4* out, UINT outstride, CONST D3DXVECTOR4* in, UINT instride, CONST D3DXMATRIX* matrix, UINT elements)
{
    UINT i;

    TRACE("(%p, %u, %p, %u, %p, %u)\n", out, outstride, in, instride, matrix, elements);

    for (i = 0; i < elements; ++i) {
        D3DXVec4Transform(
            (D3DXVECTOR4*)((char*)out + outstride * i),
            (CONST D3DXVECTOR4*)((const char*)in + instride * i),
            matrix);
    }
    return out;
}

static inline unsigned short float_32_to_16(const float in)
{
    int exp = 0, origexp;
    float tmp = fabs(in);
    int sign = (copysignf(1, in) < 0);
    unsigned int mantissa;
    unsigned short ret;

    /* Deal with special numbers */
    if (isinf(in)) return (sign ? 0xffff : 0x7fff);
    if (isnan(in)) return (sign ? 0xffff : 0x7fff);
    if (in == 0.0f) return (sign ? 0x8000 : 0x0000);

    if (tmp < powf(2, 10))
    {
        do
        {
            tmp *= 2.0f;
            exp--;
        } while (tmp < powf(2, 10));
    }
    else if (tmp >= powf(2, 11))
    {
        do
        {
            tmp /= 2.0f;
            exp++;
        } while (tmp >= powf(2, 11));
    }

    exp += 10;  /* Normalize the mantissa */
    exp += 15;  /* Exponent is encoded with excess 15 */

    origexp = exp;

    mantissa = (unsigned int) tmp;
    if ((tmp - mantissa == 0.5f && mantissa % 2 == 1) || /* round half to even */
        (tmp - mantissa > 0.5f))
    {
        mantissa++; /* round to nearest, away from zero */
    }
    if (mantissa == 2048)
    {
        mantissa = 1024;
        exp++;
    }

    if (exp > 31)
    {
        /* too big */
        ret = 0x7fff; /* INF */
    }
    else if (exp <= 0)
    {
        unsigned int rounding = 0;

        /* Denormalized half float */

        /* return 0x0000 (=0.0) for numbers too small to represent in half floats */
        if (exp < -11)
            return (sign ? 0x8000 : 0x0000);

        exp = origexp;

        /* the 13 extra bits from single precision are used for rounding */
        mantissa = (unsigned int)(tmp * powf(2, 13));
        mantissa >>= 1 - exp; /* denormalize */

        mantissa -= ~(mantissa >> 13) & 1; /* round half to even */
        /* remove 13 least significant bits to get half float precision */
        mantissa >>= 12;
        rounding = mantissa & 1;
        mantissa >>= 1;

        ret = mantissa + rounding;
    }
    else
    {
        ret = (exp << 10) | (mantissa & 0x3ff);
    }

    ret |= ((sign ? 1 : 0) << 15); /* Add the sign */
    return ret;
}

D3DXFLOAT16 *WINAPI D3DXFloat32To16Array(D3DXFLOAT16 *pout, CONST FLOAT *pin, UINT n)
{
    unsigned int i;

    TRACE("(%p, %p, %u)\n", pout, pin, n);

    for (i = 0; i < n; ++i)
    {
        pout[i].value = float_32_to_16(pin[i]);
    }

    return pout;
}

/* Native d3dx9's D3DXFloat16to32Array lacks support for NaN and Inf. Specifically, e = 16 is treated as a
 * regular number - e.g., 0x7fff is converted to 131008.0 and 0xffff to -131008.0. */
static inline float float_16_to_32(const unsigned short in)
{
    const unsigned short s = (in & 0x8000);
    const unsigned short e = (in & 0x7C00) >> 10;
    const unsigned short m = in & 0x3FF;
    const float sgn = (s ? -1.0f : 1.0f);

    if (e == 0)
    {
        if (m == 0) return sgn * 0.0f; /* +0.0 or -0.0 */
        else return sgn * powf(2, -14.0f) * (m / 1024.0f);
    }
    else
    {
        return sgn * powf(2, e - 15.0f) * (1.0f + (m / 1024.0f));
    }
}

FLOAT *WINAPI D3DXFloat16To32Array(FLOAT *pout, CONST D3DXFLOAT16 *pin, UINT n)
{
    unsigned int i;

    TRACE("(%p, %p, %u)\n", pout, pin, n);

    for (i = 0; i < n; ++i)
    {
        pout[i] = float_16_to_32(pin[i].value);
    }

    return pout;
}

/*_________________D3DXSH________________*/

FLOAT* WINAPI D3DXSHAdd(FLOAT *out, UINT order, const FLOAT *a, const FLOAT *b)
{
    UINT i;

    TRACE("out %p, order %u, a %p, b %p\n", out, order, a, b);

    for (i = 0; i < order * order; i++)
        out[i] = a[i] + b[i];

    return out;
}

FLOAT WINAPI D3DXSHDot(UINT order, CONST FLOAT *a, CONST FLOAT *b)
{
    FLOAT s;
    UINT i;

    TRACE("order %u, a %p, b %p\n", order, a, b);

    s = a[0] * b[0];
    for (i = 1; i < order * order; i++)
        s += a[i] * b[i];

    return s;
}

FLOAT* WINAPI D3DXSHEvalDirection(FLOAT *out, UINT order, CONST D3DXVECTOR3 *dir)
{

    TRACE("(%p, %u, %p)\n", out, order, dir);

    if ( (order < D3DXSH_MINORDER) || (order > D3DXSH_MAXORDER) )
        return out;

    out[0] = 0.5f / sqrt(D3DX_PI);
    out[1] = -0.5f / sqrt(D3DX_PI / 3.0f) * dir->y;
    out[2] = 0.5f / sqrt(D3DX_PI / 3.0f) * dir->z;
    out[3] = -0.5f / sqrt(D3DX_PI / 3.0f) * dir->x;
    if ( order == 2 )
        return out;

    out[4] = 0.5f / sqrt(D3DX_PI / 15.0f) * dir->x * dir->y;
    out[5] = -0.5f / sqrt(D3DX_PI / 15.0f) * dir->y * dir->z;
    out[6] = 0.25f / sqrt(D3DX_PI / 5.0f) * ( 3.0f * dir->z * dir->z - 1.0f );
    out[7] = -0.5f / sqrt(D3DX_PI / 15.0f) * dir->x * dir->z;
    out[8] = 0.25f / sqrt(D3DX_PI / 15.0f) * ( dir->x * dir->x - dir->y * dir->y );
    if ( order == 3 )
        return out;

    out[9] = -sqrt(70.0f / D3DX_PI) / 8.0f * dir->y * (3.0f * dir->x * dir->x - dir->y * dir->y );
    out[10] = sqrt(105.0f / D3DX_PI) / 2.0f * dir->x * dir->y * dir->z;
    out[11] = -sqrt(42.0 / D3DX_PI) / 8.0f * dir->y * ( -1.0f + 5.0f * dir->z * dir->z );
    out[12] = sqrt(7.0f / D3DX_PI) / 4.0f * dir->z * ( 5.0f * dir->z * dir->z - 3.0f );
    out[13] = sqrt(42.0 / D3DX_PI) / 8.0f * dir->x * ( 1.0f - 5.0f * dir->z * dir->z );
    out[14] = sqrt(105.0f / D3DX_PI) / 4.0f * dir->z * ( dir->x * dir->x - dir->y * dir->y );
    out[15] = -sqrt(70.0f / D3DX_PI) / 8.0f * dir->x * ( dir->x * dir->x - 3.0f * dir->y * dir->y );
    if ( order == 4 )
        return out;

    out[16] = 0.75f * sqrt(35.0f / D3DX_PI) * dir->x * dir->y * (dir->x * dir->x - dir->y * dir->y );
    out[17] = 3.0f * dir->z * out[9];
    out[18] = 0.75f * sqrt(5.0f / D3DX_PI) * dir->x * dir->y * ( 7.0f * dir->z * dir->z - 1.0f );
    out[19] = 0.375f * sqrt(10.0f / D3DX_PI) * dir->y * dir->z * ( 3.0f - 7.0f * dir->z * dir->z );
    out[20] = 3.0f / ( 16.0f * sqrt(D3DX_PI) ) * ( 35.0f * dir->z * dir->z * dir->z * dir->z - 30.f * dir->z * dir->z + 3.0f );
    out[21] = 0.375f * sqrt(10.0f / D3DX_PI) * dir->x * dir->z * ( 3.0f - 7.0f * dir->z * dir->z );
    out[22] = 0.375f * sqrt(5.0f / D3DX_PI) * ( dir->x * dir->x - dir->y * dir->y ) * ( 7.0f * dir->z * dir->z - 1.0f);
    out[23] = 3.0 * dir->z * out[15];
    out[24] = 3.0f / 16.0f * sqrt(35.0f / D3DX_PI) * ( dir->x * dir->x * dir->x * dir->x- 6.0f * dir->x * dir->x * dir->y * dir->y + dir->y * dir->y * dir->y * dir->y );
    if ( order == 5 )
        return out;

    out[25] = -3.0f/ 32.0f * sqrt(154.0f / D3DX_PI) * dir->y * ( 5.0f * dir->x * dir->x * dir->x * dir->x - 10.0f * dir->x * dir->x * dir->y * dir->y + dir->y * dir->y * dir->y * dir->y );
    out[26] = 0.75f * sqrt(385.0f / D3DX_PI) * dir->x * dir->y * dir->z * ( dir->x * dir->x - dir->y * dir->y );
    out[27] = sqrt(770.0f / D3DX_PI) / 32.0f * dir->y * ( 3.0f * dir->x * dir->x - dir->y * dir->y ) * ( 1.0f - 9.0f * dir->z * dir->z );
    out[28] = sqrt(1155.0f / D3DX_PI) / 4.0f * dir->x * dir->y * dir->z * ( 3.0f * dir->z * dir->z - 1.0f);
    out[29] = sqrt(165.0f / D3DX_PI) / 16.0f * dir->y * ( 14.0f * dir->z * dir->z - 21.0f * dir->z * dir->z * dir->z * dir->z - 1.0f );
    out[30] = sqrt(11.0f / D3DX_PI) / 16.0f * dir->z * ( 63.0f * dir->z * dir->z * dir->z * dir->z - 70.0f * dir->z * dir->z + 15.0f );
    out[31] = sqrt(165.0f / D3DX_PI) / 16.0f * dir->x * ( 14.0f * dir->z * dir->z - 21.0f * dir->z * dir->z * dir->z * dir->z - 1.0f );
    out[32] = sqrt(1155.0f / D3DX_PI) / 8.0f * dir->z * ( dir->x * dir->x - dir->y * dir->y ) * ( 3.0f * dir->z * dir->z - 1.0f );
    out[33] = sqrt(770.0f / D3DX_PI) / 32.0f * dir->x * ( dir->x * dir->x - 3.0f * dir->y * dir->y ) * ( 1.0f - 9.0f * dir->z * dir->z );
    out[34] = 3.0f / 16.0f * sqrt(385.0f / D3DX_PI) * dir->z * ( dir->x * dir->x * dir->x * dir->x - 6.0 * dir->x * dir->x * dir->y * dir->y + dir->y * dir->y * dir->y * dir->y );
    out[35] = -3.0f/ 32.0f * sqrt(154.0f / D3DX_PI) * dir->x * ( dir->x * dir->x * dir->x * dir->x - 10.0f * dir->x * dir->x * dir->y * dir->y + 5.0f * dir->y * dir->y * dir->y * dir->y );

    return out;
}

FLOAT* WINAPI D3DXSHMultiply2(FLOAT *out, CONST FLOAT *a, CONST FLOAT *b)
{
    FLOAT ta, tb;

    TRACE("(%p, %p, %p)\n", out, a, b);

    ta = 0.28209479f * a[0];
    tb = 0.28209479f * b[0];

    out[0]= 0.28209479f * D3DXSHDot(2, a, b);
    out[1] = ta * b[1] + tb * a[1];
    out[2] = ta * b[2] + tb * a[2];
    out[3] = ta * b[3] + tb * a[3];

    return out;
}

FLOAT* WINAPI D3DXSHMultiply3(FLOAT *out, CONST FLOAT *a, CONST FLOAT *b)
{
    FLOAT t, ta, tb;

    TRACE("(%p, %p, %p)\n", out, a, b);

    out[0]= 0.28209479f * a[0] * b[0];

    ta = 0.28209479f * a[0] - 0.12615662f * a[6] - 0.21850968f * a[8];
    tb = 0.28209479f * b[0] - 0.12615662f * b[6] - 0.21850968f * b[8];
    out[1] = ta * b[1] + tb * a[1];
    t = a[1] * b[1];
    out[0] += 0.28209479f * t;
    out[6] = -0.12615662f * t;
    out[8] = -0.21850968f * t;

    ta = 0.21850968f * a[5];
    tb = 0.21850968f * b[5];
    out[1] += ta * b[2] + tb * a[2];
    out[2] = ta * b[1] + tb * a[1];
    t = a[1] * b[2] +a[2] * b[1];
    out[5] = 0.21850968f * t;

    ta = 0.21850968f * a[4];
    tb = 0.21850968f * b[4];
    out[1] += ta * b[3] + tb * a[3];
    out[3]  = ta * b[1] + tb * a[1];
    t = a[1] * b[3] + a[3] * b[1];
    out[4] = 0.21850968f * t;

    ta = 0.28209480f * a[0] + 0.25231326f * a[6];
    tb = 0.28209480f * b[0] + 0.25231326f * b[6];
    out[2] += ta * b[2] + tb * a[2];
    t = a[2] * b[2];
    out[0] += 0.28209480f * t;
    out[6] += 0.25231326f * t;

    ta = 0.21850969f * a[7];
    tb = 0.21850969f * b[7];
    out[2] += ta * b[3] + tb * a[3];
    out[3] += ta * b[2] + tb * a[2];
    t = a[2] * b[3] + a[3] * b[2];
    out[7] = 0.21850969f * t;

    ta = 0.28209479f * a[0] - 0.12615663f * a[6] + 0.21850969f * a[8];
    tb = 0.28209479f * b[0] - 0.12615663f * b[6] + 0.21850969f * b[8];
    out[3] += ta * b[3] + tb * a[3];
    t = a[3] * b[3];
    out[0] += 0.28209479f * t;
    out[6] -= 0.12615663f * t;
    out[8] += 0.21850969f * t;

    ta = 0.28209479f * a[0] - 0.18022375f * a[6];
    tb = 0.28209479f * b[0] - 0.18022375f * b[6];
    out[4] += ta * b[4] + tb * a[4];
    t = a[4] * b[4];
    out[0] += 0.28209479f * t;
    out[6] -= 0.18022375f * t;

    ta = 0.15607835f * a[7];
    tb = 0.15607835f * b[7];
    out[4] += ta * b[5] + tb * a[5];
    out[5] += ta * b[4] + tb * a[4];
    t = a[4] * b[5] + a[5] * b[4];
    out[7] += 0.15607834f * t;

    ta = 0.28209479f * a[0] + 0.09011186 * a[6] - 0.15607835f * a[8];
    tb = 0.28209479f * b[0] + 0.09011186 * b[6] - 0.15607835f * b[8];
    out[5] += ta * b[5] + tb * a[5];
    t = a[5] * b[5];
    out[0] += 0.28209479f * t;
    out[6] += 0.09011186f * t;
    out[8] -= 0.15607835f * t;

    ta = 0.28209480f * a[0];
    tb = 0.28209480f * b[0];
    out[6] += ta * b[6] + tb * a[6];
    t = a[6] * b[6];
    out[0] += 0.28209480f * t;
    out[6] += 0.18022376f * t;

    ta = 0.28209479f * a[0] + 0.09011186 * a[6] + 0.15607835f * a[8];
    tb = 0.28209479f * b[0] + 0.09011186 * b[6] + 0.15607835f * b[8];
    out[7] += ta * b[7] + tb * a[7];
    t = a[7] * b[7];
    out[0] += 0.28209479f * t;
    out[6] += 0.09011186f * t;
    out[8] += 0.15607835f * t;

    ta = 0.28209479f * a[0] - 0.18022375f * a[6];
    tb = 0.28209479f * b[0] - 0.18022375f * b[6];
    out[8] += ta * b[8] + tb * a[8];
    t = a[8] * b[8];
    out[0] += 0.28209479f * t;
    out[6] -= 0.18022375f * t;

    return out;
}

FLOAT* WINAPI D3DXSHRotateZ(FLOAT *out, UINT order, FLOAT angle, CONST FLOAT *in)
{
    FLOAT c1a, c2a, c3a, c4a, c5a, s1a, s2a, s3a, s4a, s5a;

    TRACE("%p, %u, %f, %p)\n", out, order, angle, in);

    c1a = cos( angle );
    c2a = cos( 2.0f * angle );
    c3a = cos( 3.0f * angle );
    c4a = cos( 4.0f * angle );
    c5a = cos( 5.0f * angle );
    s1a = sin( angle );
    s2a = sin( 2.0f * angle );
    s3a = sin( 3.0f * angle );
    s4a = sin( 4.0f * angle );
    s5a = sin( 5.0f * angle );

    out[0] = in[0];
    out[1] = c1a * in[1] + s1a * in[3];
    out[2] = in[2];
    out[3] =  c1a * in[3] - s1a * in[1];
    if ( order <= D3DXSH_MINORDER )
        return out;

    out[4] = c2a * in[4] + s2a * in[8];
    out[5] = c1a * in[5] + s1a * in[7];
    out[6] = in[6];
    out[7] = c1a * in[7] - s1a * in[5];
    out[8] = c2a * in[8] - s2a * in[4];
    if ( order == 3 )
        return out;

    out[9] = c3a * in[9] + s3a * in[15];
    out[10] = c2a * in[10] + s2a * in[14];
    out[11] = c1a * in[11] + s1a * in[13];
    out[12] = in[12];
    out[13] = c1a * in[13] - s1a * in[11];
    out[14] = c2a * in[14] - s2a * in[10];
    out[15] = c3a * in[15] - s3a * in[9];
    if ( order == 4 )
        return out;

    out[16] = c4a * in[16] + s4a * in[24];
    out[17] = c3a * in[17] + s3a * in[23];
    out[18] = c2a * in[18] + s2a * in[22];
    out[19] = c1a * in[19] + s1a * in[21];
    out[20] = in[20];
    out[21] = c1a * in[21] - s1a * in[19];
    out[22] = c2a * in[22] - s2a * in[18];
    out[23] = c3a * in[23] - s3a * in[17];
    out[24] = c4a * in[24] - s4a * in[16];
    if ( order == 5 )
        return out;

    out[25] = c5a * in[25] + s5a * in[35];
    out[26] = c4a * in[26] + s4a * in[34];
    out[27] = c3a * in[27] + s3a * in[33];
    out[28] = c2a * in[28] + s2a * in[32];
    out[29] = c1a * in[29] + s1a * in[31];
    out[30] = in[30];
    out[31] = c1a * in[31] - s1a * in[29];
    out[32] = c2a * in[32] - s2a * in[28];
    out[33] = c3a * in[33] - s3a * in[27];
    out[34] = c4a * in[34] - s4a * in[26];
    out[35] = c5a * in[35] - s5a * in[25];

    return out;
}

FLOAT* WINAPI D3DXSHScale(FLOAT *out, UINT order, CONST FLOAT *a, CONST FLOAT scale)
{
    UINT i;

    TRACE("out %p, order %u, a %p, scale %f\n", out, order, a, scale);

    for (i = 0; i < order * order; i++)
        out[i] = a[i] * scale;

    return out;
}
