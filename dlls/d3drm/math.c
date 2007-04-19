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

/* Add Two Vectors */
LPD3DVECTOR WINAPI D3DRMVectorAdd(LPD3DVECTOR d, LPD3DVECTOR s1, LPD3DVECTOR s2)
{
    d->x=s1->x + s2->x;
    d->y=s1->y + s2->y;
    d->z=s1->z + s2->z;
    return d;
}

/* Subtract Two Vectors */
LPD3DVECTOR WINAPI D3DRMVectorSubtract(LPD3DVECTOR d, LPD3DVECTOR s1, LPD3DVECTOR s2)
{
    d->x=s1->x - s2->x;
    d->y=s1->y - s2->y;
    d->z=s1->z - s2->z;
    return d;
}

/* Cross Product of Two Vectors */
LPD3DVECTOR WINAPI D3DRMVectorCrossProduct(LPD3DVECTOR d, LPD3DVECTOR s1, LPD3DVECTOR s2)
{
    d->x=s1->y * s2->z - s1->z * s2->y;
    d->y=s1->z * s2->x - s1->x * s2->z;
    d->z=s1->x * s2->y - s1->y * s2->x;
    return d;
}

/* Dot Product of Two vectors */
D3DVALUE WINAPI D3DRMVectorDotProduct(LPD3DVECTOR s1, LPD3DVECTOR s2)
{
    D3DVALUE dot_product;
    dot_product=s1->x * s2->x + s1->y * s2->y + s1->z * s2->z;
    return dot_product;
}

/* Norm of a vector */
D3DVALUE WINAPI D3DRMVectorModulus(LPD3DVECTOR v)
{
    D3DVALUE result;
    result=sqrt(v->x * v->x + v->y * v->y + v->z * v->z);
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
        u->x=1.0;
        u->y=0.0;
        u->z=0.0;
    }
    return u;
}

/* Returns a random unit vector */
LPD3DVECTOR WINAPI D3DRMVectorRandom(LPD3DVECTOR d)
{
    d->x = rand();
    d->y = rand();
    d->z = rand();
    D3DRMVectorNormalize(d);
    return d;
}

/* Scale a vector */
LPD3DVECTOR WINAPI D3DRMVectorScale(LPD3DVECTOR d, LPD3DVECTOR s, D3DVALUE factor)
{
    d->x=factor * s->x;
    d->y=factor * s->y;
    d->z=factor * s->z;
    return d;
}
