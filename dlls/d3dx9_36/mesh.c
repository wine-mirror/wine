 /*
 * Mesh operations specific to D3DX9.
 *
 * Copyright (C) 2009 David Adam
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
#include "d3dx9.h"


/*************************************************************************
 * D3DXComputeBoundingBox
 */
HRESULT WINAPI D3DXComputeBoundingBox(CONST D3DXVECTOR3 *pfirstposition, DWORD numvertices, DWORD dwstride, D3DXVECTOR3 *pmin, D3DXVECTOR3 *pmax)
{
    D3DXVECTOR3 vec;
    unsigned int i;

    if( !pfirstposition || !pmin || !pmax ) return D3DERR_INVALIDCALL;

    *pmin = *pfirstposition;
    *pmax = *pmin;

    for(i=0; i<numvertices; i++)
    {
        vec = *( (D3DXVECTOR3*)((char*)pfirstposition + dwstride * i) );

        if ( vec.x < pmin->x ) pmin->x = vec.x;
        if ( vec.x > pmax->x ) pmax->x = vec.x;

        if ( vec.y < pmin->y ) pmin->y = vec.y;
        if ( vec.y > pmax->y ) pmax->y = vec.y;

        if ( vec.z < pmin->z ) pmin->z = vec.z;
        if ( vec.z > pmax->z ) pmax->z = vec.z;
    }

    return D3D_OK;
}

/*************************************************************************
 * D3DXComputeBoundingSphere
 */
HRESULT WINAPI D3DXComputeBoundingSphere(CONST D3DXVECTOR3* pfirstposition, DWORD numvertices, DWORD dwstride, D3DXVECTOR3 *pcenter, FLOAT *pradius)
{
    D3DXVECTOR3 temp, temp1;
    FLOAT d;
    unsigned int i;

    if( !pfirstposition || !pcenter || !pradius ) return D3DERR_INVALIDCALL;

    temp.x = 0.0f;
    temp.y = 0.0f;
    temp.z = 0.0f;
    temp1 = temp;
    d = 0.0f;
    *pradius = 0.0f;

    for(i=0; i<numvertices; i++)
    {
        D3DXVec3Add(&temp1, &temp, (D3DXVECTOR3*)((char*)pfirstposition + dwstride * i));
        temp = temp1;
    }

    D3DXVec3Scale(pcenter, &temp, 1.0f/((FLOAT)numvertices));

    for(i=0; i<numvertices; i++)
    {
        d = D3DXVec3Length(D3DXVec3Subtract(&temp, (D3DXVECTOR3*)((char*)pfirstposition + dwstride * i), pcenter));
        if ( d > *pradius ) *pradius = d;
    }
    return D3D_OK;
}

/*************************************************************************
 * D3DXGetFVFVertexSize
 */
static UINT Get_TexCoord_Size_From_FVF(DWORD FVF, int tex_num)
{
    return (((((FVF) >> (16 + (2 * (tex_num)))) + 1) & 0x03) + 1);
}

UINT WINAPI D3DXGetFVFVertexSize(DWORD FVF)
{
    DWORD size = 0;
    UINT i;
    UINT numTextures = (FVF & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;

    if (FVF & D3DFVF_NORMAL) size +=  sizeof(D3DXVECTOR3);
    if (FVF & D3DFVF_DIFFUSE) size += sizeof(DWORD);
    if (FVF & D3DFVF_SPECULAR) size += sizeof(DWORD);
    if (FVF & D3DFVF_PSIZE) size += sizeof(DWORD);

    switch (FVF & D3DFVF_POSITION_MASK)
    {
        case D3DFVF_XYZ:    size += sizeof(D3DXVECTOR3); break;
        case D3DFVF_XYZRHW: size += 4 * sizeof(FLOAT); break;
        case D3DFVF_XYZB1:  size += 4 * sizeof(FLOAT); break;
        case D3DFVF_XYZB2:  size += 5 * sizeof(FLOAT); break;
        case D3DFVF_XYZB3:  size += 6 * sizeof(FLOAT); break;
        case D3DFVF_XYZB4:  size += 7 * sizeof(FLOAT); break;
        case D3DFVF_XYZB5:  size += 8 * sizeof(FLOAT); break;
        case D3DFVF_XYZW:   size += 4 * sizeof(FLOAT); break;
    }

    for (i = 0; i < numTextures; i++)
    {
        size += Get_TexCoord_Size_From_FVF(FVF, i) * sizeof(FLOAT);
    }

    return size;
}

/*************************************************************************
 * D3DXIntersectTri
 */
BOOL WINAPI D3DXIntersectTri(CONST D3DXVECTOR3 *p0, CONST D3DXVECTOR3 *p1, CONST D3DXVECTOR3 *p2, CONST D3DXVECTOR3 *praypos, CONST D3DXVECTOR3 *praydir, FLOAT *pu, FLOAT *pv, FLOAT *pdist)
{
    D3DXMATRIX m;
    D3DXVECTOR4 vec;

    m.m[0][0] = p1->x - p0->x;
    m.m[1][0] = p2->x - p0->x;
    m.m[2][0] = -praydir->x;
    m.m[3][0] = 0.0f;
    m.m[0][1] = p1->y - p0->z;
    m.m[1][1] = p2->y - p0->z;
    m.m[2][1] = -praydir->y;
    m.m[3][1] = 0.0f;
    m.m[0][2] = p1->z - p0->z;
    m.m[1][2] = p2->z - p0->z;
    m.m[2][2] = -praydir->z;
    m.m[3][2] = 0.0f;
    m.m[0][3] = 0.0f;
    m.m[1][3] = 0.0f;
    m.m[2][3] = 0.0f;
    m.m[3][3] = 1.0f;

    vec.x = praypos->x - p0->x;
    vec.y = praypos->y - p0->y;
    vec.z = praypos->z - p0->z;
    vec.w = 0.0f;

    if ( D3DXMatrixInverse(&m, NULL, &m) )
    {
        D3DXVec4Transform(&vec, &vec, &m);
        if ( (vec.x >= 0.0f) && (vec.y >= 0.0f) && (vec.x + vec.y <= 1.0f) && (vec.z >= 0.0f) )
        {
            *pu = vec.x;
            *pv = vec.y;
            *pdist = fabs( vec.z );
            return TRUE;
        }
    }

    return FALSE;
}
