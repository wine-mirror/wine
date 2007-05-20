/*
 * Copyright 2007 David Adam
 * Copyright 2007 Vijay Kiran Kamuju
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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <math.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "d3drmdef.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3drm);

/* Determine the blue part of a color */
D3DVALUE WINAPI D3DRMColorGetBlue(D3DCOLOR color)
{
    return (RGBA_GETBLUE(color)/255.0);
}

/* Determine the green part of a color */
D3DVALUE WINAPI D3DRMColorGetGreen(D3DCOLOR color)
{
    return (RGBA_GETGREEN(color)/255.0);
}

/* Determine the red part of a color */
D3DVALUE WINAPI D3DRMColorGetRed(D3DCOLOR color)
{
    return (RGBA_GETRED(color)/255.0);
}

/* Product of 2 quaternions */
LPD3DRMQUATERNION WINAPI D3DRMQuaternionMultiply(LPD3DRMQUATERNION q, LPD3DRMQUATERNION a, LPD3DRMQUATERNION b)
{
    D3DVECTOR cross_product;
    D3DRMVectorCrossProduct(&cross_product, &a->v, &b->v);
    q->s = a->s * b->s - D3DRMVectorDotProduct(&a->v, &b->v);
    q->v.u1.x = a->s * b->v.u1.x + b->s * a->v.u1.x + cross_product.u1.x;
    q->v.u2.y = a->s * b->v.u2.y + b->s * a->v.u2.y + cross_product.u2.y;
    q->v.u3.z = a->s * b->v.u3.z + b->s * a->v.u3.z + cross_product.u3.z;
    return q;
}

/* Matrix for the Rotation that a unit quaternion represents */
void WINAPI D3DRMMatrixFromQuaternion(D3DRMMATRIX4D m, LPD3DRMQUATERNION q)
{
    D3DVALUE w,x,y,z;
    w = q->s;
    x = q->v.u1.x;
    y = q->v.u2.y;
    z = q->v.u3.z;
    m[0][0] = 1.0-2.0*(y*y+z*z);
    m[1][1] = 1.0-2.0*(x*x+z*z);
    m[2][2] = 1.0-2.0*(x*x+y*y);
    m[1][0] = 2.0*(x*y+z*w);
    m[0][1] = 2.0*(x*y-z*w);
    m[2][0] = 2.0*(x*z-y*w);
    m[0][2] = 2.0*(x*z+y*w);
    m[2][1] = 2.0*(y*z+x*w);
    m[1][2] = 2.0*(y*z-x*w);
    m[3][0] = 0.0;
    m[3][1] = 0.0;
    m[3][2] = 0.0;
    m[0][3] = 0.0;
    m[1][3] = 0.0;
    m[2][3] = 0.0;
    m[3][3] = 1.0;
}

/* Return a unit quaternion that represents a rotation of an angle around an axis */
LPD3DRMQUATERNION WINAPI D3DRMQuaternionFromRotation(LPD3DRMQUATERNION q, LPD3DVECTOR v, D3DVALUE theta)
{
    q->s = cos(theta/2.0);
    D3DRMVectorScale(&q->v, D3DRMVectorNormalize(v), sin(theta/2.0));
    return q;
}

/* Interpolation between two quaternions */
LPD3DRMQUATERNION WINAPI D3DRMQuaternionSlerp(LPD3DRMQUATERNION q, LPD3DRMQUATERNION a, LPD3DRMQUATERNION b, D3DVALUE alpha)
{
    D3DVALUE epsilon=1.0;
    D3DVECTOR sca1,sca2;
    if (a->s * b->s + D3DRMVectorDotProduct(&a->v, &b->v) < 0.0) epsilon = -1.0;
    q->s = (1.0 - alpha) * a->s + epsilon * alpha * b->s;
    D3DRMVectorAdd(&q->v, D3DRMVectorScale(&sca1, &a->v, 1.0 - alpha),
                   D3DRMVectorScale(&sca2, &b->v, epsilon * alpha));
    return q;
}

/* Add Two Vectors */
LPD3DVECTOR WINAPI D3DRMVectorAdd(LPD3DVECTOR d, LPD3DVECTOR s1, LPD3DVECTOR s2)
{
    d->u1.x=s1->u1.x + s2->u1.x;
    d->u2.y=s1->u2.y + s2->u2.y;
    d->u3.z=s1->u3.z + s2->u3.z;
    return d;
}

/* Subtract Two Vectors */
LPD3DVECTOR WINAPI D3DRMVectorSubtract(LPD3DVECTOR d, LPD3DVECTOR s1, LPD3DVECTOR s2)
{
    d->u1.x=s1->u1.x - s2->u1.x;
    d->u2.y=s1->u2.y - s2->u2.y;
    d->u3.z=s1->u3.z - s2->u3.z;
    return d;
}

/* Cross Product of Two Vectors */
LPD3DVECTOR WINAPI D3DRMVectorCrossProduct(LPD3DVECTOR d, LPD3DVECTOR s1, LPD3DVECTOR s2)
{
    d->u1.x=s1->u2.y * s2->u3.z - s1->u3.z * s2->u2.y;
    d->u2.y=s1->u3.z * s2->u1.x - s1->u1.x * s2->u3.z;
    d->u3.z=s1->u1.x * s2->u2.y - s1->u2.y * s2->u1.x;
    return d;
}

/* Dot Product of Two vectors */
D3DVALUE WINAPI D3DRMVectorDotProduct(LPD3DVECTOR s1, LPD3DVECTOR s2)
{
    D3DVALUE dot_product;
    dot_product=s1->u1.x * s2->u1.x + s1->u2.y * s2->u2.y + s1->u3.z * s2->u3.z;
    return dot_product;
}

/* Norm of a vector */
D3DVALUE WINAPI D3DRMVectorModulus(LPD3DVECTOR v)
{
    D3DVALUE result;
    result=sqrt(v->u1.x * v->u1.x + v->u2.y * v->u2.y + v->u3.z * v->u3.z);
    return result;
}

/* Normalize a vector.  Returns (1,0,0) if INPUT is the NULL vector. */
LPD3DVECTOR WINAPI D3DRMVectorNormalize(LPD3DVECTOR u)
{
    D3DVALUE modulus = D3DRMVectorModulus(u);
    if(modulus)
    {
        D3DRMVectorScale(u,u,1.0/modulus);
    }
    else
    {
        u->u1.x=1.0;
        u->u2.y=0.0;
        u->u3.z=0.0;
    }
    return u;
}

/* Returns a random unit vector */
LPD3DVECTOR WINAPI D3DRMVectorRandom(LPD3DVECTOR d)
{
    d->u1.x = rand();
    d->u2.y = rand();
    d->u3.z = rand();
    D3DRMVectorNormalize(d);
    return d;
}

/* Reflection of a vector on a surface */
LPD3DVECTOR WINAPI D3DRMVectorReflect(LPD3DVECTOR r, LPD3DVECTOR ray, LPD3DVECTOR norm)
{
    D3DVECTOR sca;
    D3DRMVectorSubtract(r, D3DRMVectorScale(&sca, norm, 2.0*D3DRMVectorDotProduct(ray,norm)), ray);
    return r;
}

/* Rotation of a vector */
LPD3DVECTOR WINAPI D3DRMVectorRotate(LPD3DVECTOR r, LPD3DVECTOR v, LPD3DVECTOR axis, D3DVALUE theta)
{
    D3DRMQUATERNION quaternion,quaternion1, quaternion2, quaternion3, resultq;
    D3DVECTOR NORM;

    quaternion1.s = cos(theta*.5);
    quaternion2.s = cos(theta*.5);
    NORM = *D3DRMVectorNormalize(axis);
    D3DRMVectorScale(&quaternion1.v, &NORM, sin(theta * .5));
    D3DRMVectorScale(&quaternion2.v, &NORM, -sin(theta * .5));
    quaternion3.s = 0.0;
    quaternion3.v = *v;
    D3DRMQuaternionMultiply(&quaternion, &quaternion1, &quaternion3);
    D3DRMQuaternionMultiply(&resultq, &quaternion, &quaternion2);
    *r = *D3DRMVectorNormalize(&resultq.v);
    return r;
}

/* Scale a vector */
LPD3DVECTOR WINAPI D3DRMVectorScale(LPD3DVECTOR d, LPD3DVECTOR s, D3DVALUE factor)
{
    d->u1.x=factor * s->u1.x;
    d->u2.y=factor * s->u2.y;
    d->u3.z=factor * s->u3.z;
    return d;
}
