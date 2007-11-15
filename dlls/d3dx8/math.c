/*
 * Copyright 2007 David Adam
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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "d3dx8.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dx8);

/*_________________D3DXMatrix____________________*/

D3DXMATRIX* WINAPI D3DXMatrixAffineTransformation(D3DXMATRIX *pout, float scaling, D3DXVECTOR3 *rotationcenter, D3DXQUATERNION *rotation, D3DXVECTOR3 *translation)
{
    D3DXMATRIX m1, m2, m3, m4, m5, p1, p2, p3;

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
    if ( !rotation )
    {
     D3DXMatrixIdentity(&m3);
    }
    else
    {
     D3DXMatrixRotationQuaternion(&m3, rotation);
    }
    if ( !translation )
    {
     D3DXMatrixIdentity(&m5);
    }
    else
    {
     D3DXMatrixTranslation(&m5, translation->x, translation->y, translation->z);
    }
    D3DXMatrixMultiply(&p1, &m1, &m2);
    D3DXMatrixMultiply(&p2, &p1, &m3);
    D3DXMatrixMultiply(&p3, &p2, &m4);
    D3DXMatrixMultiply(pout, &p3, &m5);
    return pout;
}

FLOAT WINAPI D3DXMatrixfDeterminant(CONST D3DXMATRIX *pm)
{
    D3DXVECTOR4 minor, v1, v2, v3;
    FLOAT det;

    v1.x = pm->u.m[0][0]; v1.y = pm->u.m[1][0]; v1.z = pm->u.m[2][0]; v1.w = pm->u.m[3][0];
    v2.x = pm->u.m[0][1]; v2.y = pm->u.m[1][1]; v2.z = pm->u.m[2][1]; v2.w = pm->u.m[3][1];
    v3.x = pm->u.m[0][2]; v3.y = pm->u.m[1][2]; v3.z = pm->u.m[2][2]; v3.w = pm->u.m[3][2];
    D3DXVec4Cross(&minor,&v1,&v2,&v3);
    det =  - (pm->u.m[0][3] * minor.x + pm->u.m[1][3] * minor.y + pm->u.m[2][3] * minor.z + pm->u.m[3][3] * minor.w);
    return det;
}

D3DXMATRIX* WINAPI D3DXMatrixInverse(D3DXMATRIX *pout, FLOAT *pdeterminant, CONST D3DXMATRIX *pm)
{
    int a, i, j;
    D3DXVECTOR4 v, vec[3];
    FLOAT cofactor, det;

    det = D3DXMatrixfDeterminant(pm);
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
    for (j=0; j<4; j++)
    {
     switch(j)
     {
      case 0: cofactor = v.x; break;
      case 1: cofactor = v.y; break;
      case 2: cofactor = v.z; break;
      case 3: cofactor = v.w; break;
     }
    pout->u.m[j][i] = pow(-1.0f, i) * cofactor / det;
    }
   }
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixLookAtLH(D3DXMATRIX *pout, CONST D3DXVECTOR3 *peye, CONST D3DXVECTOR3 *pat, CONST D3DXVECTOR3 *pup)
{
    D3DXVECTOR3 right, rightn, up, upn, vec, vec2;

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
    int i,j;

    for (i=0; i<4; i++)
    {
     for (j=0; j<4; j++)
     {
      pout->u.m[i][j] = pm1->u.m[i][0] * pm2->u.m[0][j] + pm1->u.m[i][1] * pm2->u.m[1][j] + pm1->u.m[i][2] * pm2->u.m[2][j] + pm1->u.m[i][3] * pm2->u.m[3][j];
     }
    }
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixMultiplyTranspose(D3DXMATRIX *pout, CONST D3DXMATRIX *pm1, CONST D3DXMATRIX *pm2)
{
    D3DXMATRIX temp;

    D3DXMatrixMultiply(&temp, pm1, pm2);
    D3DXMatrixTranspose(pout, &temp);
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixOrthoLH(D3DXMATRIX *pout, FLOAT w, FLOAT h, FLOAT zn, FLOAT zf)
{
    D3DXMatrixIdentity(pout);
    pout->u.m[0][0] = 2.0f / w;
    pout->u.m[1][1] = 2.0f / h;
    pout->u.m[2][2] = 1.0f / (zf - zn);
    pout->u.m[3][2] = zn / (zn - zf);
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixOrthoOffCenterLH(D3DXMATRIX *pout, FLOAT l, FLOAT r, FLOAT b, FLOAT t, FLOAT zn, FLOAT zf)
{
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
    D3DXMatrixIdentity(pout);
    pout->u.m[0][0] = 2.0f / w;
    pout->u.m[1][1] = 2.0f / h;
    pout->u.m[2][2] = 1.0f / (zn - zf);
    pout->u.m[3][2] = zn / (zn - zf);
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixPerspectiveFovLH(D3DXMATRIX *pout, FLOAT fovy, FLOAT aspect, FLOAT zn, FLOAT zf)
{
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
    D3DXMatrixIdentity(pout);
    pout->u.m[1][1] = cos(angle);
    pout->u.m[2][2] = cos(angle);
    pout->u.m[1][2] = sin(angle);
    pout->u.m[2][1] = -sin(angle);
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixRotationY(D3DXMATRIX *pout, FLOAT angle)
{
    D3DXMatrixIdentity(pout);
    pout->u.m[0][0] = cos(angle);
    pout->u.m[2][2] = cos(angle);
    pout->u.m[0][2] = -sin(angle);
    pout->u.m[2][0] = sin(angle);
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixRotationYawPitchRoll(D3DXMATRIX *pout, FLOAT yaw, FLOAT pitch, FLOAT roll)
{
    D3DXMATRIX m, pout1, pout2, pout3;

    D3DXMatrixIdentity(&pout3);
    D3DXMatrixRotationZ(&m,roll);
    D3DXMatrixMultiply(&pout2,&pout3,&m);
    D3DXMatrixRotationX(&m,pitch);
    D3DXMatrixMultiply(&pout1,&pout2,&m);
    D3DXMatrixRotationY(&m,yaw);
    D3DXMatrixMultiply(pout,&pout1,&m);
    return pout;
}
D3DXMATRIX* WINAPI D3DXMatrixRotationZ(D3DXMATRIX *pout, FLOAT angle)
{
    D3DXMatrixIdentity(pout);
    pout->u.m[0][0] = cos(angle);
    pout->u.m[1][1] = cos(angle);
    pout->u.m[0][1] = sin(angle);
    pout->u.m[1][0] = -sin(angle);
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixScaling(D3DXMATRIX *pout, FLOAT sx, FLOAT sy, FLOAT sz)
{
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

D3DXMATRIX* WINAPI D3DXMatrixTranslation(D3DXMATRIX *pout, FLOAT x, FLOAT y, FLOAT z)
{
    D3DXMatrixIdentity(pout);
    pout->u.m[3][0] = x;
    pout->u.m[3][1] = y;
    pout->u.m[3][2] = z;
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixTranspose(D3DXMATRIX *pout, CONST D3DXMATRIX *pm)
{
    int i,j;

    for (i=0; i<4; i++)
    {
     for (j=0; j<4; j++)
     {
      pout->u.m[i][j] = pm->u.m[j][i];
     }
    }
    return pout;
}

/*_________________D3DXPLANE________________*/

D3DXVECTOR3* WINAPI D3DXPlaneIntersectLine(D3DXVECTOR3 *pout, CONST D3DXPLANE *pp, CONST D3DXVECTOR3 *pv1, CONST D3DXVECTOR3 *pv2)
{
    D3DXVECTOR3 direction, normal;
    FLOAT dot, temp;

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
    FLOAT norm;

    norm = sqrt(pp->a * pp->a + pp->b * pp->b + pp->c * pp->c);
    if ( norm )
    {
     pout->a = pp->a / norm;
     pout->b = pp->b / norm;
     pout->c = pp->c / norm;
     pout->d = pp->d / norm;
    }
    else
    {
     pout->a = 0.0f;
     pout->b = 0.0f;
     pout->c = 0.0f;
     pout->d = 0.0f;
    }
    return pout;
}

/*_________________D3DXQUATERNION________________*/

D3DXQUATERNION* WINAPI D3DXQuaternionNormalize(D3DXQUATERNION *pout, CONST D3DXQUATERNION *pq)
{
    FLOAT norm;

    norm = D3DXQuaternionLength(pq);
    if ( !norm )
    {
     pout->x = 0.0f;
     pout->y = 0.0f;
     pout->z = 0.0f;
     pout->w = 0.0f;
    }
    else
    {
     pout->x = pq->x / norm;
     pout->y = pq->y / norm;
     pout->z = pq->z / norm;
     pout->w = pq->w / norm;
    }
    return pout;
}
/*_________________D3DXVec2_____________________*/

D3DXVECTOR2* WINAPI D3DXVec2BaryCentric(D3DXVECTOR2 *pout, CONST D3DXVECTOR2 *pv1, CONST D3DXVECTOR2 *pv2, CONST D3DXVECTOR2 *pv3, FLOAT f, FLOAT g)
{
    pout->x = (1.0f-f-g) * (pv1->x) + f * (pv2->x) + g * (pv3->x);
    pout->y = (1.0f-f-g) * (pv1->y) + f * (pv2->y) + g * (pv3->y);
    return pout;
}

D3DXVECTOR2* WINAPI D3DXVec2CatmullRom(D3DXVECTOR2 *pout, CONST D3DXVECTOR2 *pv0, CONST D3DXVECTOR2 *pv1, CONST D3DXVECTOR2 *pv2, CONST D3DXVECTOR2 *pv3, FLOAT s)
{
    pout->x = 0.5f * (2.0f * pv1->x + (pv2->x - pv0->x) *s + (2.0f *pv0->x - 5.0f * pv1->x + 4.0f * pv2->x - pv3->x) * s * s + (pv3->x -3.0f * pv2->x + 3.0f * pv1->x - pv0->x) * s * s * s);
    pout->y = 0.5f * (2.0f * pv1->y + (pv2->y - pv0->y) *s + (2.0f *pv0->y - 5.0f * pv1->y + 4.0f * pv2->y - pv3->y) * s * s + (pv3->y -3.0f * pv2->y + 3.0f * pv1->y - pv0->y) * s * s * s);
    return pout;
}

D3DXVECTOR2* WINAPI D3DXVec2Hermite(D3DXVECTOR2 *pout, CONST D3DXVECTOR2 *pv1, CONST D3DXVECTOR2 *pt1, CONST D3DXVECTOR2 *pv2, CONST D3DXVECTOR2 *pt2, FLOAT s)
{
    FLOAT h1, h2, h3, h4;

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
    FLOAT norm;

    norm = D3DXVec2Length(pv);
    if ( !norm )
    {
     pout->x = 0.0f;
     pout->y = 0.0f;
    }
    else
    {
     pout->x = pv->x / norm;
     pout->y = pv->y / norm;
    }
    return pout;
}

D3DXVECTOR4* WINAPI D3DXVec2Transform(D3DXVECTOR4 *pout, CONST D3DXVECTOR2 *pv, CONST D3DXMATRIX *pm)
{
    pout->x = pm->u.m[0][0] * pv->x + pm->u.m[1][0] * pv->y  + pm->u.m[3][0];
    pout->y = pm->u.m[0][1] * pv->x + pm->u.m[1][1] * pv->y  + pm->u.m[3][1];
    pout->z = pm->u.m[0][2] * pv->x + pm->u.m[1][2] * pv->y  + pm->u.m[3][2];
    pout->w = pm->u.m[0][3] * pv->x + pm->u.m[1][3] * pv->y  + pm->u.m[3][3];
    return pout;
}

D3DXVECTOR2* WINAPI D3DXVec2TransformCoord(D3DXVECTOR2 *pout, CONST D3DXVECTOR2 *pv, CONST D3DXMATRIX *pm)
{
    FLOAT norm;

    norm = pm->u.m[0][3] * pv->x + pm->u.m[1][3] * pv->y + pm->u.m[3][3];
    if ( norm )
    {
     pout->x = (pm->u.m[0][0] * pv->x + pm->u.m[1][0] * pv->y + pm->u.m[3][0]) / norm;
     pout->y = (pm->u.m[0][1] * pv->x + pm->u.m[1][1] * pv->y + pm->u.m[3][1]) / norm;
    }
    else
    {
     pout->x = 0.0f;
     pout->y = 0.0f;
    }
    return pout;
}

D3DXVECTOR2* WINAPI D3DXVec2TransformNormal(D3DXVECTOR2 *pout, CONST D3DXVECTOR2 *pv, CONST D3DXMATRIX *pm)
{
    pout->x = pm->u.m[0][0] * pv->x + pm->u.m[1][0] * pv->y;
    pout->y = pm->u.m[0][1] * pv->x + pm->u.m[1][1] * pv->y;
    return pout;
}

/*_________________D3DXVec3_____________________*/

D3DXVECTOR3* WINAPI D3DXVec3BaryCentric(D3DXVECTOR3 *pout, CONST D3DXVECTOR3 *pv1, CONST D3DXVECTOR3 *pv2, CONST D3DXVECTOR3 *pv3, FLOAT f, FLOAT g)
{
    pout->x = (1.0f-f-g) * (pv1->x) + f * (pv2->x) + g * (pv3->x);
    pout->y = (1.0f-f-g) * (pv1->y) + f * (pv2->y) + g * (pv3->y);
    pout->z = (1.0f-f-g) * (pv1->z) + f * (pv2->z) + g * (pv3->z);
    return pout;
}

D3DXVECTOR3* WINAPI D3DXVec3CatmullRom( D3DXVECTOR3 *pout, CONST D3DXVECTOR3 *pv0, CONST D3DXVECTOR3 *pv1, CONST D3DXVECTOR3 *pv2, CONST D3DXVECTOR3 *pv3, FLOAT s)
{
    pout->x = 0.5f * (2.0f * pv1->x + (pv2->x - pv0->x) *s + (2.0f *pv0->x - 5.0f * pv1->x + 4.0f * pv2->x - pv3->x) * s * s + (pv3->x -3.0f * pv2->x + 3.0f * pv1->x - pv0->x) * s * s * s);
    pout->y = 0.5f * (2.0f * pv1->y + (pv2->y - pv0->y) *s + (2.0f *pv0->y - 5.0f * pv1->y + 4.0f * pv2->y - pv3->y) * s * s + (pv3->y -3.0f * pv2->y + 3.0f * pv1->y - pv0->y) * s * s * s);
    pout->z = 0.5f * (2.0f * pv1->z + (pv2->z - pv0->z) *s + (2.0f *pv0->z - 5.0f * pv1->z + 4.0f * pv2->z - pv3->z) * s * s + (pv3->z -3.0f * pv2->z + 3.0f * pv1->z - pv0->z) * s * s * s);
    return pout;
}

D3DXVECTOR3* WINAPI D3DXVec3Hermite(D3DXVECTOR3 *pout, CONST D3DXVECTOR3 *pv1, CONST D3DXVECTOR3 *pt1, CONST D3DXVECTOR3 *pv2, CONST D3DXVECTOR3 *pt2, FLOAT s)
{
    FLOAT h1, h2, h3, h4;

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
    FLOAT norm;

    norm = D3DXVec3Length(pv);
    if ( !norm )
    {
     pout->x = 0.0f;
     pout->y = 0.0f;
     pout->z = 0.0f;
    }
    else
    {
     pout->x = pv->x / norm;
     pout->y = pv->y / norm;
     pout->z = pv->z / norm;
    }
    return pout;
}

D3DXVECTOR3* WINAPI D3DXVec3Project(D3DXVECTOR3 *pout, CONST D3DXVECTOR3 *pv, CONST D3DVIEWPORT8 *pviewport, CONST D3DXMATRIX *pprojection, CONST D3DXMATRIX *pview, CONST D3DXMATRIX *pworld)
{
    D3DXMATRIX m1, m2;
    D3DXVECTOR3 vec;

    D3DXMatrixMultiply(&m1, pworld, pview);
    D3DXMatrixMultiply(&m2, &m1, pprojection);
    D3DXVec3TransformCoord(&vec, pv, &m2);
    pout->x = pviewport->X +  ( 1.0f + vec.x ) * pviewport->Width / 2.0f;
    pout->y = pviewport->Y +  ( 1.0f - vec.y ) * pviewport->Height / 2.0f;
    pout->z = pviewport->MinZ + vec.z * ( pviewport->MaxZ - pviewport->MinZ );
    return pout;
}

D3DXVECTOR4* WINAPI D3DXVec3Transform(D3DXVECTOR4 *pout, CONST D3DXVECTOR3 *pv, CONST D3DXMATRIX *pm)
{
    pout->x = pm->u.m[0][0] * pv->x + pm->u.m[1][0] * pv->y + pm->u.m[2][0] * pv->z + pm->u.m[3][0];
    pout->y = pm->u.m[0][1] * pv->x + pm->u.m[1][1] * pv->y + pm->u.m[2][1] * pv->z + pm->u.m[3][1];
    pout->z = pm->u.m[0][2] * pv->x + pm->u.m[1][2] * pv->y + pm->u.m[2][2] * pv->z + pm->u.m[3][2];
    pout->w = pm->u.m[0][3] * pv->x + pm->u.m[1][3] * pv->y + pm->u.m[2][3] * pv->z + pm->u.m[3][3];
    return pout;
}

D3DXVECTOR3* WINAPI D3DXVec3TransformCoord(D3DXVECTOR3 *pout, CONST D3DXVECTOR3 *pv, CONST D3DXMATRIX *pm)
{
    FLOAT norm;

    norm = pm->u.m[0][3] * pv->x + pm->u.m[1][3] * pv->y + pm->u.m[2][3] *pv->z + pm->u.m[3][3];

    if ( norm )
    {
     pout->x = (pm->u.m[0][0] * pv->x + pm->u.m[1][0] * pv->y + pm->u.m[2][0] * pv->z + pm->u.m[3][0]) / norm;
     pout->y = (pm->u.m[0][1] * pv->x + pm->u.m[1][1] * pv->y + pm->u.m[2][1] * pv->z + pm->u.m[3][1]) / norm;
     pout->z = (pm->u.m[0][2] * pv->x + pm->u.m[1][2] * pv->y + pm->u.m[2][2] * pv->z + pm->u.m[3][2]) / norm;
    }
    else
    {
     pout->x = 0.0f;
     pout->y = 0.0f;
     pout->z = 0.0f;
    }
    return pout;
}

D3DXVECTOR3* WINAPI D3DXVec3TransformNormal(D3DXVECTOR3 *pout, CONST D3DXVECTOR3 *pv, CONST D3DXMATRIX *pm)
{
    pout->x = pm->u.m[0][0] * pv->x + pm->u.m[1][0] * pv->y + pm->u.m[2][0] * pv->z;
    pout->y = pm->u.m[0][1] * pv->x + pm->u.m[1][1] * pv->y + pm->u.m[2][1] * pv->z;
    pout->z = pm->u.m[0][2] * pv->x + pm->u.m[1][2] * pv->y + pm->u.m[2][2] * pv->z;
    return pout;

}

D3DXVECTOR3* WINAPI D3DXVec3Unproject(D3DXVECTOR3 *pout, CONST D3DXVECTOR3 *pv, CONST D3DVIEWPORT8 *pviewport, CONST D3DXMATRIX *pprojection, CONST D3DXMATRIX *pview, CONST D3DXMATRIX *pworld)
{
    D3DXMATRIX m1, m2, m3;
    D3DXVECTOR3 vec;

    D3DXMatrixMultiply(&m1, pworld, pview);
    D3DXMatrixMultiply(&m2, &m1, pprojection);
    D3DXMatrixInverse(&m3, NULL, &m2);
    vec.x = 2.0f * ( pv->x - pviewport->X ) / pviewport->Width - 1.0f;
    vec.y = 1.0f - 2.0f * ( pv->y - pviewport->Y ) / pviewport->Height;
    vec.z = ( pv->z - pviewport->MinZ) / ( pviewport->MaxZ - pviewport->MinZ );
    D3DXVec3TransformCoord(pout, &vec, &m3);
    return pout;
}

/*_________________D3DXVec4_____________________*/

D3DXVECTOR4* WINAPI D3DXVec4BaryCentric(D3DXVECTOR4 *pout, CONST D3DXVECTOR4 *pv1, CONST D3DXVECTOR4 *pv2, CONST D3DXVECTOR4 *pv3, FLOAT f, FLOAT g)
{
    pout->x = (1.0f-f-g) * (pv1->x) + f * (pv2->x) + g * (pv3->x);
    pout->y = (1.0f-f-g) * (pv1->y) + f * (pv2->y) + g * (pv3->y);
    pout->z = (1.0f-f-g) * (pv1->z) + f * (pv2->z) + g * (pv3->z);
    pout->w = (1.0f-f-g) * (pv1->w) + f * (pv2->w) + g * (pv3->w);
    return pout;
}

D3DXVECTOR4* WINAPI D3DXVec4CatmullRom(D3DXVECTOR4 *pout, CONST D3DXVECTOR4 *pv0, CONST D3DXVECTOR4 *pv1, CONST D3DXVECTOR4 *pv2, CONST D3DXVECTOR4 *pv3, FLOAT s)
{
    pout->x = 0.5f * (2.0f * pv1->x + (pv2->x - pv0->x) *s + (2.0f *pv0->x - 5.0f * pv1->x + 4.0f * pv2->x - pv3->x) * s * s + (pv3->x -3.0f * pv2->x + 3.0f * pv1->x - pv0->x) * s * s * s);
    pout->y = 0.5f * (2.0f * pv1->y + (pv2->y - pv0->y) *s + (2.0f *pv0->y - 5.0f * pv1->y + 4.0f * pv2->y - pv3->y) * s * s + (pv3->y -3.0f * pv2->y + 3.0f * pv1->y - pv0->y) * s * s * s);
    pout->z = 0.5f * (2.0f * pv1->z + (pv2->z - pv0->z) *s + (2.0f *pv0->z - 5.0f * pv1->z + 4.0f * pv2->z - pv3->z) * s * s + (pv3->z -3.0f * pv2->z + 3.0f * pv1->z - pv0->z) * s * s * s);
    pout->w = 0.5f * (2.0f * pv1->w + (pv2->w - pv0->w) *s + (2.0f *pv0->w - 5.0f * pv1->w + 4.0f * pv2->w - pv3->w) * s * s + (pv3->w -3.0f * pv2->w + 3.0f * pv1->w - pv0->w) * s * s * s);
    return pout;
}

D3DXVECTOR4* WINAPI D3DXVec4Cross(D3DXVECTOR4 *pout, CONST D3DXVECTOR4 *pv1, CONST D3DXVECTOR4 *pv2, CONST D3DXVECTOR4 *pv3)
{
    pout->x = pv1->y * (pv2->z * pv3->w - pv3->z * pv2->w) - pv1->z * (pv2->y * pv3->w - pv3->y * pv2->w) + pv1->w * (pv2->y * pv3->z - pv2->z *pv3->y);
    pout->y = -(pv1->x * (pv2->z * pv3->w - pv3->z * pv2->w) - pv1->z * (pv2->x * pv3->w - pv3->x * pv2->w) + pv1->w * (pv2->x * pv3->z - pv3->x * pv2->z));
    pout->z = pv1->x * (pv2->y * pv3->w - pv3->y * pv2->w) - pv1->y * (pv2->x *pv3->w - pv3->x * pv2->w) + pv1->w * (pv2->x * pv3->y - pv3->x * pv2->y);
    pout->w = -(pv1->x * (pv2->y * pv3->z - pv3->y * pv2->z) - pv1->y * (pv2->x * pv3->z - pv3->x *pv2->z) + pv1->z * (pv2->x * pv3->y - pv3->x * pv2->y));
    return pout;
}

D3DXVECTOR4* WINAPI D3DXVec4Hermite(D3DXVECTOR4 *pout, CONST D3DXVECTOR4 *pv1, CONST D3DXVECTOR4 *pt1, CONST D3DXVECTOR4 *pv2, CONST D3DXVECTOR4 *pt2, FLOAT s)
{
    FLOAT h1, h2, h3, h4;

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
    FLOAT norm;

    norm = D3DXVec4Length(pv);
    if ( !norm )
    {
     pout->x = 0.0f;
     pout->y = 0.0f;
     pout->z = 0.0f;
     pout->w = 0.0f;
    }
    else
    {
     pout->x = pv->x / norm;
     pout->y = pv->y / norm;
     pout->z = pv->z / norm;
     pout->w = pv->w / norm;
    }
    return pout;
}

D3DXVECTOR4* WINAPI D3DXVec4Transform(D3DXVECTOR4 *pout, CONST D3DXVECTOR4 *pv, CONST D3DXMATRIX *pm)
{
    pout->x = pm->u.m[0][0] * pv->x + pm->u.m[1][0] * pv->y + pm->u.m[2][0] * pv->z + pm->u.m[3][0] * pv->w;
    pout->y = pm->u.m[0][1] * pv->x + pm->u.m[1][1] * pv->y + pm->u.m[2][1] * pv->z + pm->u.m[3][1] * pv->w;
    pout->z = pm->u.m[0][2] * pv->x + pm->u.m[1][2] * pv->y + pm->u.m[2][2] * pv->z + pm->u.m[3][2] * pv->w;
    pout->w = pm->u.m[0][3] * pv->x + pm->u.m[1][3] * pv->y + pm->u.m[2][3] * pv->z + pm->u.m[3][3] * pv->w;
    return pout;
}
