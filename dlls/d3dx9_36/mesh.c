 /*
 * Mesh operations specific to D3DX9.
 *
 * Copyright (C) 2005 Henri Verbeet
 * Copyright (C) 2006 Ivan Gyurdiev
 * Copyright (C) 2009 David Adam
 * Copyright (C) 2010 Tony Wasserka
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
#include "wine/port.h"

#define NONAMELESSUNION
#include "windef.h"
#include "wingdi.h"
#include "d3dx9.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);

/*************************************************************************
 * D3DXBoxBoundProbe
 */
BOOL WINAPI D3DXBoxBoundProbe(CONST D3DXVECTOR3 *pmin, CONST D3DXVECTOR3 *pmax, CONST D3DXVECTOR3 *prayposition, CONST D3DXVECTOR3 *praydirection)

/* Algorithm taken from the article: An Efficient and Robust Ray-Box Intersection Algoritm
Amy Williams             University of Utah
Steve Barrus             University of Utah
R. Keith Morley          University of Utah
Peter Shirley            University of Utah

International Conference on Computer Graphics and Interactive Techniques  archive
ACM SIGGRAPH 2005 Courses
Los Angeles, California

This algorithm is free of patents or of copyrights, as confirmed by Peter Shirley himself.

Algorithm: Consider the box as the intersection of three slabs. Clip the ray
against each slab, if there's anything left of the ray after we're
done we've got an intersection of the ray with the box.
*/

{
    FLOAT div, tmin, tmax, tymin, tymax, tzmin, tzmax;

    div = 1.0f / praydirection->x;
    if ( div >= 0.0f )
    {
        tmin = ( pmin->x - prayposition->x ) * div;
        tmax = ( pmax->x - prayposition->x ) * div;
    }
    else
    {
        tmin = ( pmax->x - prayposition->x ) * div;
        tmax = ( pmin->x - prayposition->x ) * div;
    }

    if ( tmax < 0.0f ) return FALSE;

    div = 1.0f / praydirection->y;
    if ( div >= 0.0f )
    {
        tymin = ( pmin->y - prayposition->y ) * div;
        tymax = ( pmax->y - prayposition->y ) * div;
    }
    else
    {
        tymin = ( pmax->y - prayposition->y ) * div;
        tymax = ( pmin->y - prayposition->y ) * div;
    }

    if ( ( tymax < 0.0f ) || ( tmin > tymax ) || ( tymin > tmax ) ) return FALSE;

    if ( tymin > tmin ) tmin = tymin;
    if ( tymax < tmax ) tmax = tymax;

    div = 1.0f / praydirection->z;
    if ( div >= 0.0f )
    {
        tzmin = ( pmin->z - prayposition->z ) * div;
        tzmax = ( pmax->z - prayposition->z ) * div;
    }
    else
    {
        tzmin = ( pmax->z - prayposition->z ) * div;
        tzmax = ( pmin->z - prayposition->z ) * div;
    }

    if ( (tzmax < 0.0f ) || ( tmin > tzmax ) || ( tzmin > tmax ) ) return FALSE;

    return TRUE;
}

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
        vec = *( (const D3DXVECTOR3*)((const char*)pfirstposition + dwstride * i) );

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
        D3DXVec3Add(&temp1, &temp, (const D3DXVECTOR3*)((const char*)pfirstposition + dwstride * i));
        temp = temp1;
    }

    D3DXVec3Scale(pcenter, &temp, 1.0f/((FLOAT)numvertices));

    for(i=0; i<numvertices; i++)
    {
        d = D3DXVec3Length(D3DXVec3Subtract(&temp, (const D3DXVECTOR3*)((const char*)pfirstposition + dwstride * i), pcenter));
        if ( d > *pradius ) *pradius = d;
    }
    return D3D_OK;
}

static const UINT d3dx_decltype_size[D3DDECLTYPE_UNUSED] =
{
   /* D3DDECLTYPE_FLOAT1    */ 1 * 4,
   /* D3DDECLTYPE_FLOAT2    */ 2 * 4,
   /* D3DDECLTYPE_FLOAT3    */ 3 * 4,
   /* D3DDECLTYPE_FLOAT4    */ 4 * 4,
   /* D3DDECLTYPE_D3DCOLOR  */ 4 * 1,
   /* D3DDECLTYPE_UBYTE4    */ 4 * 1,
   /* D3DDECLTYPE_SHORT2    */ 2 * 2,
   /* D3DDECLTYPE_SHORT4    */ 4 * 2,
   /* D3DDECLTYPE_UBYTE4N   */ 4 * 1,
   /* D3DDECLTYPE_SHORT2N   */ 2 * 2,
   /* D3DDECLTYPE_SHORT4N   */ 4 * 2,
   /* D3DDECLTYPE_USHORT2N  */ 2 * 2,
   /* D3DDECLTYPE_USHORT4N  */ 4 * 2,
   /* D3DDECLTYPE_UDEC3     */ 4, /* 3 * 10 bits + 2 padding */
   /* D3DDECLTYPE_DEC3N     */ 4,
   /* D3DDECLTYPE_FLOAT16_2 */ 2 * 2,
   /* D3DDECLTYPE_FLOAT16_4 */ 4 * 2,
};

static void append_decl_element(D3DVERTEXELEMENT9 *declaration, UINT *idx, UINT *offset,
        D3DDECLTYPE type, D3DDECLUSAGE usage, UINT usage_idx)
{
    declaration[*idx].Stream = 0;
    declaration[*idx].Offset = *offset;
    declaration[*idx].Type = type;
    declaration[*idx].Method = D3DDECLMETHOD_DEFAULT;
    declaration[*idx].Usage = usage;
    declaration[*idx].UsageIndex = usage_idx;

    *offset += d3dx_decltype_size[type];
    ++(*idx);
}

/*************************************************************************
 * D3DXDeclaratorFromFVF
 */
HRESULT WINAPI D3DXDeclaratorFromFVF(DWORD fvf, D3DVERTEXELEMENT9 declaration[MAX_FVF_DECL_SIZE])
{
    static const D3DVERTEXELEMENT9 end_element = D3DDECL_END();
    DWORD tex_count = (fvf & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
    unsigned int offset = 0;
    unsigned int idx = 0;
    unsigned int i;

    TRACE("fvf %#x, declaration %p.\n", fvf, declaration);

    if (fvf & D3DFVF_POSITION_MASK)
    {
        BOOL has_blend = (fvf & D3DFVF_XYZB5) >= D3DFVF_XYZB1;
        DWORD blend_count = 1 + (((fvf & D3DFVF_XYZB5) - D3DFVF_XYZB1) >> 1);
        BOOL has_blend_idx = (fvf & D3DFVF_LASTBETA_D3DCOLOR) || (fvf & D3DFVF_LASTBETA_UBYTE4);

        if (has_blend_idx) --blend_count;

        if ((fvf & D3DFVF_POSITION_MASK) == D3DFVF_XYZW
                || (has_blend && blend_count > 4))
            return D3DERR_INVALIDCALL;

        if ((fvf & D3DFVF_POSITION_MASK) == D3DFVF_XYZRHW)
            append_decl_element(declaration, &idx, &offset, D3DDECLTYPE_FLOAT4, D3DDECLUSAGE_POSITIONT, 0);
        else
            append_decl_element(declaration, &idx, &offset, D3DDECLTYPE_FLOAT3, D3DDECLUSAGE_POSITION, 0);

        if (has_blend)
        {
            switch (blend_count)
            {
                 case 0:
                    break;
                 case 1:
                    append_decl_element(declaration, &idx, &offset, D3DDECLTYPE_FLOAT1, D3DDECLUSAGE_BLENDWEIGHT, 0);
                    break;
                 case 2:
                    append_decl_element(declaration, &idx, &offset, D3DDECLTYPE_FLOAT2, D3DDECLUSAGE_BLENDWEIGHT, 0);
                    break;
                 case 3:
                    append_decl_element(declaration, &idx, &offset, D3DDECLTYPE_FLOAT3, D3DDECLUSAGE_BLENDWEIGHT, 0);
                    break;
                 case 4:
                    append_decl_element(declaration, &idx, &offset, D3DDECLTYPE_FLOAT4, D3DDECLUSAGE_BLENDWEIGHT, 0);
                    break;
                 default:
                     ERR("Invalid blend count %u.\n", blend_count);
                     break;
            }

            if (has_blend_idx)
            {
                if (fvf & D3DFVF_LASTBETA_UBYTE4)
                    append_decl_element(declaration, &idx, &offset, D3DDECLTYPE_UBYTE4, D3DDECLUSAGE_BLENDINDICES, 0);
                else if (fvf & D3DFVF_LASTBETA_D3DCOLOR)
                    append_decl_element(declaration, &idx, &offset, D3DDECLTYPE_D3DCOLOR, D3DDECLUSAGE_BLENDINDICES, 0);
            }
        }
    }

    if (fvf & D3DFVF_NORMAL)
        append_decl_element(declaration, &idx, &offset, D3DDECLTYPE_FLOAT3, D3DDECLUSAGE_NORMAL, 0);
    if (fvf & D3DFVF_PSIZE)
        append_decl_element(declaration, &idx, &offset, D3DDECLTYPE_FLOAT1, D3DDECLUSAGE_PSIZE, 0);
    if (fvf & D3DFVF_DIFFUSE)
        append_decl_element(declaration, &idx, &offset, D3DDECLTYPE_D3DCOLOR, D3DDECLUSAGE_COLOR, 0);
    if (fvf & D3DFVF_SPECULAR)
        append_decl_element(declaration, &idx, &offset, D3DDECLTYPE_D3DCOLOR, D3DDECLUSAGE_COLOR, 1);

    for (i = 0; i < tex_count; ++i)
    {
        switch ((fvf >> (16 + 2 * i)) & 0x03)
        {
            case D3DFVF_TEXTUREFORMAT1:
                append_decl_element(declaration, &idx, &offset, D3DDECLTYPE_FLOAT1, D3DDECLUSAGE_TEXCOORD, i);
                break;
            case D3DFVF_TEXTUREFORMAT2:
                append_decl_element(declaration, &idx, &offset, D3DDECLTYPE_FLOAT2, D3DDECLUSAGE_TEXCOORD, i);
                break;
            case D3DFVF_TEXTUREFORMAT3:
                append_decl_element(declaration, &idx, &offset, D3DDECLTYPE_FLOAT3, D3DDECLUSAGE_TEXCOORD, i);
                break;
            case D3DFVF_TEXTUREFORMAT4:
                append_decl_element(declaration, &idx, &offset, D3DDECLTYPE_FLOAT4, D3DDECLUSAGE_TEXCOORD, i);
                break;
        }
    }

    declaration[idx] = end_element;

    return D3D_OK;
}

/*************************************************************************
 * D3DXFVFFromDeclarator
 */
HRESULT WINAPI D3DXFVFFromDeclarator(const D3DVERTEXELEMENT9 *declaration, DWORD *fvf)
{
    unsigned int i = 0, texture, offset;

    TRACE("(%p, %p)\n", declaration, fvf);

    *fvf = 0;
    if (declaration[0].Type == D3DDECLTYPE_FLOAT3 && declaration[0].Usage == D3DDECLUSAGE_POSITION)
    {
        if ((declaration[1].Type == D3DDECLTYPE_FLOAT4 && declaration[1].Usage == D3DDECLUSAGE_BLENDWEIGHT &&
             declaration[1].UsageIndex == 0) &&
            (declaration[2].Type == D3DDECLTYPE_FLOAT1 && declaration[2].Usage == D3DDECLUSAGE_BLENDINDICES &&
             declaration[2].UsageIndex == 0))
        {
            return D3DERR_INVALIDCALL;
        }
        else if ((declaration[1].Type == D3DDECLTYPE_UBYTE4 || declaration[1].Type == D3DDECLTYPE_D3DCOLOR) &&
                 declaration[1].Usage == D3DDECLUSAGE_BLENDINDICES && declaration[1].UsageIndex == 0)
        {
            if (declaration[1].Type == D3DDECLTYPE_UBYTE4)
            {
                *fvf |= D3DFVF_XYZB1 | D3DFVF_LASTBETA_UBYTE4;
            }
            else
            {
                *fvf |= D3DFVF_XYZB1 | D3DFVF_LASTBETA_D3DCOLOR;
            }
            i = 2;
        }
        else if (declaration[1].Type <= D3DDECLTYPE_FLOAT4 && declaration[1].Usage == D3DDECLUSAGE_BLENDWEIGHT &&
                 declaration[1].UsageIndex == 0)
        {
            if ((declaration[2].Type == D3DDECLTYPE_UBYTE4 || declaration[2].Type == D3DDECLTYPE_D3DCOLOR) &&
                declaration[2].Usage == D3DDECLUSAGE_BLENDINDICES && declaration[2].UsageIndex == 0)
            {
                if (declaration[2].Type == D3DDECLTYPE_UBYTE4)
                {
                    *fvf |= D3DFVF_LASTBETA_UBYTE4;
                }
                else
                {
                    *fvf |= D3DFVF_LASTBETA_D3DCOLOR;
                }
                switch (declaration[1].Type)
                {
                    case D3DDECLTYPE_FLOAT1: *fvf |= D3DFVF_XYZB2; break;
                    case D3DDECLTYPE_FLOAT2: *fvf |= D3DFVF_XYZB3; break;
                    case D3DDECLTYPE_FLOAT3: *fvf |= D3DFVF_XYZB4; break;
                    case D3DDECLTYPE_FLOAT4: *fvf |= D3DFVF_XYZB5; break;
                }
                i = 3;
            }
            else
            {
                switch (declaration[1].Type)
                {
                    case D3DDECLTYPE_FLOAT1: *fvf |= D3DFVF_XYZB1; break;
                    case D3DDECLTYPE_FLOAT2: *fvf |= D3DFVF_XYZB2; break;
                    case D3DDECLTYPE_FLOAT3: *fvf |= D3DFVF_XYZB3; break;
                    case D3DDECLTYPE_FLOAT4: *fvf |= D3DFVF_XYZB4; break;
                }
                i = 2;
            }
        }
        else
        {
            *fvf |= D3DFVF_XYZ;
            i = 1;
        }
    }
    else if (declaration[0].Type == D3DDECLTYPE_FLOAT4 && declaration[0].Usage == D3DDECLUSAGE_POSITIONT &&
             declaration[0].UsageIndex == 0)
    {
        *fvf |= D3DFVF_XYZRHW;
        i = 1;
    }

    if (declaration[i].Type == D3DDECLTYPE_FLOAT3 && declaration[i].Usage == D3DDECLUSAGE_NORMAL)
    {
        *fvf |= D3DFVF_NORMAL;
        i++;
    }
    if (declaration[i].Type == D3DDECLTYPE_FLOAT1 && declaration[i].Usage == D3DDECLUSAGE_PSIZE &&
        declaration[i].UsageIndex == 0)
    {
        *fvf |= D3DFVF_PSIZE;
        i++;
    }
    if (declaration[i].Type == D3DDECLTYPE_D3DCOLOR && declaration[i].Usage == D3DDECLUSAGE_COLOR &&
        declaration[i].UsageIndex == 0)
    {
        *fvf |= D3DFVF_DIFFUSE;
        i++;
    }
    if (declaration[i].Type == D3DDECLTYPE_D3DCOLOR && declaration[i].Usage == D3DDECLUSAGE_COLOR &&
        declaration[i].UsageIndex == 1)
    {
        *fvf |= D3DFVF_SPECULAR;
        i++;
    }

    for (texture = 0; texture < D3DDP_MAXTEXCOORD; i++, texture++)
    {
        if (declaration[i].Stream == 0xFF)
        {
            break;
        }
        else if (declaration[i].Type == D3DDECLTYPE_FLOAT1 && declaration[i].Usage == D3DDECLUSAGE_TEXCOORD &&
                 declaration[i].UsageIndex == texture)
        {
            *fvf |= D3DFVF_TEXCOORDSIZE1(declaration[i].UsageIndex);
        }
        else if (declaration[i].Type == D3DDECLTYPE_FLOAT2 && declaration[i].Usage == D3DDECLUSAGE_TEXCOORD &&
                 declaration[i].UsageIndex == texture)
        {
            *fvf |= D3DFVF_TEXCOORDSIZE2(declaration[i].UsageIndex);
        }
        else if (declaration[i].Type == D3DDECLTYPE_FLOAT3 && declaration[i].Usage == D3DDECLUSAGE_TEXCOORD &&
                 declaration[i].UsageIndex == texture)
        {
            *fvf |= D3DFVF_TEXCOORDSIZE3(declaration[i].UsageIndex);
        }
        else if (declaration[i].Type == D3DDECLTYPE_FLOAT4 && declaration[i].Usage == D3DDECLUSAGE_TEXCOORD &&
                 declaration[i].UsageIndex == texture)
        {
            *fvf |= D3DFVF_TEXCOORDSIZE4(declaration[i].UsageIndex);
        }
        else
        {
            return D3DERR_INVALIDCALL;
        }
    }

    *fvf |= (texture << D3DFVF_TEXCOUNT_SHIFT);

    for (offset = 0, i = 0; declaration[i].Stream != 0xFF;
         offset += d3dx_decltype_size[declaration[i].Type], i++)
    {
        if (declaration[i].Offset != offset)
        {
            return D3DERR_INVALIDCALL;
        }
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

    if (FVF & D3DFVF_NORMAL) size += sizeof(D3DXVECTOR3);
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
 * D3DXGetDeclVertexSize
 */
UINT WINAPI D3DXGetDeclVertexSize(const D3DVERTEXELEMENT9 *decl, DWORD stream_idx)
{
    const D3DVERTEXELEMENT9 *element;
    UINT size = 0;

    TRACE("decl %p, stream_idx %u\n", decl, stream_idx);

    if (!decl) return 0;

    for (element = decl; element->Stream != 0xff; ++element)
    {
        UINT type_size;

        if (element->Stream != stream_idx) continue;

        if (element->Type >= sizeof(d3dx_decltype_size) / sizeof(*d3dx_decltype_size))
        {
            FIXME("Unhandled element type %#x, size will be incorrect.\n", element->Type);
            continue;
        }

        type_size = d3dx_decltype_size[element->Type];
        if (element->Offset + type_size > size) size = element->Offset + type_size;
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

    m.u.m[0][0] = p1->x - p0->x;
    m.u.m[1][0] = p2->x - p0->x;
    m.u.m[2][0] = -praydir->x;
    m.u.m[3][0] = 0.0f;
    m.u.m[0][1] = p1->y - p0->z;
    m.u.m[1][1] = p2->y - p0->z;
    m.u.m[2][1] = -praydir->y;
    m.u.m[3][1] = 0.0f;
    m.u.m[0][2] = p1->z - p0->z;
    m.u.m[1][2] = p2->z - p0->z;
    m.u.m[2][2] = -praydir->z;
    m.u.m[3][2] = 0.0f;
    m.u.m[0][3] = 0.0f;
    m.u.m[1][3] = 0.0f;
    m.u.m[2][3] = 0.0f;
    m.u.m[3][3] = 1.0f;

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

/*************************************************************************
 * D3DXSphereBoundProbe
 */
BOOL WINAPI D3DXSphereBoundProbe(CONST D3DXVECTOR3 *pcenter, FLOAT radius, CONST D3DXVECTOR3 *prayposition, CONST D3DXVECTOR3 *praydirection)
{
    D3DXVECTOR3 difference;
    FLOAT a, b, c, d;

    a = D3DXVec3LengthSq(praydirection);
    if (!D3DXVec3Subtract(&difference, prayposition, pcenter)) return FALSE;
    b = D3DXVec3Dot(&difference, praydirection);
    c = D3DXVec3LengthSq(&difference) - radius * radius;
    d = b * b - a * c;

    if ( ( d <= 0.0f ) || ( sqrt(d) <= b ) ) return FALSE;
    return TRUE;
}

HRESULT WINAPI D3DXCreateMesh(DWORD numfaces, DWORD numvertices, DWORD options, CONST LPD3DVERTEXELEMENT9 *declaration,
                              LPDIRECT3DDEVICE9 device, LPD3DXMESH *mesh)
{
    FIXME("(%d, %d, %d, %p, %p, %p): stub\n", numfaces, numvertices, options, declaration, device, mesh);

    return E_NOTIMPL;
}

HRESULT WINAPI D3DXCreateBox(LPDIRECT3DDEVICE9 device, FLOAT width, FLOAT height,
                             FLOAT depth, LPD3DXMESH* mesh, LPD3DXBUFFER* adjacency)
{
    FIXME("(%p, %f, %f, %f, %p, %p): stub\n", device, width, height, depth, mesh, adjacency);

    return E_NOTIMPL;
}

HRESULT WINAPI D3DXCreateSphere(LPDIRECT3DDEVICE9 device, FLOAT radius, UINT slices,
                                UINT stacks, LPD3DXMESH* mesh, LPD3DXBUFFER* adjacency)
{
    FIXME("(%p, %f, %d, %d, %p, %p): stub\n", device, radius, slices, stacks, mesh, adjacency);

    return E_NOTIMPL;
}
