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
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "d3dx8.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dx8);

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

/*_________________D3DXVec3_____________________*/

D3DXVECTOR3* WINAPI D3DXVec3BaryCentric(D3DXVECTOR3 *pout, CONST D3DXVECTOR3 *pv1, CONST D3DXVECTOR3 *pv2, CONST D3DXVECTOR3 *pv3, FLOAT f, FLOAT g)
{
    pout->x = (1.0f-f-g) * (pv1->x) + f * (pv2->x) + g * (pv3->x);
    pout->y = (1.0f-f-g) * (pv1->y) + f * (pv2->y) + g * (pv3->y);
    pout->z = (1.0f-f-g) * (pv1->z) + f * (pv2->z) + g * (pv3->z);
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

/*_________________D3DXVec4_____________________*/

D3DXVECTOR4* WINAPI D3DXVec4BaryCentric(D3DXVECTOR4 *pout, CONST D3DXVECTOR4 *pv1, CONST D3DXVECTOR4 *pv2, CONST D3DXVECTOR4 *pv3, FLOAT f, FLOAT g)
{
    pout->x = (1.0f-f-g) * (pv1->x) + f * (pv2->x) + g * (pv3->x);
    pout->y = (1.0f-f-g) * (pv1->y) + f * (pv2->y) + g * (pv3->y);
    pout->z = (1.0f-f-g) * (pv1->z) + f * (pv2->z) + g * (pv3->z);
    pout->w = (1.0f-f-g) * (pv1->w) + f * (pv2->w) + g * (pv3->w);
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
