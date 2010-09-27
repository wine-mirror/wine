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
#include "d3dx9_36_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);

/*** IUnknown methods ***/
static HRESULT WINAPI ID3DXMeshImpl_QueryInterface(ID3DXMesh *iface, REFIID riid, LPVOID *object)
{
    ID3DXMeshImpl *This = (ID3DXMeshImpl *)iface;

    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_ID3DXBaseMesh) ||
        IsEqualGUID(riid, &IID_ID3DXMesh))
    {
        iface->lpVtbl->AddRef(iface);
        *object = This;
        return S_OK;
    }

    WARN("Interface %s not found.\n", debugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI ID3DXMeshImpl_AddRef(ID3DXMesh *iface)
{
    ID3DXMeshImpl *This = (ID3DXMeshImpl *)iface;

    TRACE("(%p)->(): AddRef from %d\n", This, This->ref);

    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI ID3DXMeshImpl_Release(ID3DXMesh *iface)
{
    ID3DXMeshImpl *This = (ID3DXMeshImpl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(): Release from %d\n", This, ref + 1);

    if (!ref)
    {
        IDirect3DIndexBuffer9_Release(This->index_buffer);
        IDirect3DVertexBuffer9_Release(This->vertex_buffer);
        IDirect3DVertexDeclaration9_Release(This->vertex_declaration);
        IDirect3DDevice9_Release(This->device);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

/*** ID3DXBaseMesh ***/
static HRESULT WINAPI ID3DXMeshImpl_DrawSubset(ID3DXMesh *iface, DWORD attrib_id)
{
    ID3DXMeshImpl *This = (ID3DXMeshImpl *)iface;

    FIXME("(%p)->(%u): stub\n", This, attrib_id);

    return E_NOTIMPL;
}

static DWORD WINAPI ID3DXMeshImpl_GetNumFaces(ID3DXMesh *iface)
{
    ID3DXMeshImpl *This = (ID3DXMeshImpl *)iface;

    TRACE("(%p)\n", This);

    return This->numfaces;
}

static DWORD WINAPI ID3DXMeshImpl_GetNumVertices(ID3DXMesh *iface)
{
    ID3DXMeshImpl *This = (ID3DXMeshImpl *)iface;

    TRACE("(%p)\n", This);

    return This->numvertices;
}

static DWORD WINAPI ID3DXMeshImpl_GetFVF(ID3DXMesh *iface)
{
    ID3DXMeshImpl *This = (ID3DXMeshImpl *)iface;

    TRACE("(%p)\n", This);

    return This->fvf;
}

static HRESULT WINAPI ID3DXMeshImpl_GetDeclaration(ID3DXMesh *iface, D3DVERTEXELEMENT9 declaration[MAX_FVF_DECL_SIZE])
{
    ID3DXMeshImpl *This = (ID3DXMeshImpl *)iface;
    UINT numelements;

    TRACE("(%p)\n", This);

    if (declaration == NULL) return D3DERR_INVALIDCALL;

    return IDirect3DVertexDeclaration9_GetDeclaration(This->vertex_declaration,
                                                      declaration,
                                                      &numelements);
}

static DWORD WINAPI ID3DXMeshImpl_GetNumBytesPerVertex(ID3DXMesh *iface)
{
    ID3DXMeshImpl *This = (ID3DXMeshImpl *)iface;

    FIXME("(%p): stub\n", This);

    return 0; /* arbitrary since we cannot return E_NOTIMPL */
}

static DWORD WINAPI ID3DXMeshImpl_GetOptions(ID3DXMesh *iface)
{
    ID3DXMeshImpl *This = (ID3DXMeshImpl *)iface;

    TRACE("(%p)\n", This);

    return This->options;
}

static HRESULT WINAPI ID3DXMeshImpl_GetDevice(ID3DXMesh *iface, LPDIRECT3DDEVICE9 *device)
{
    ID3DXMeshImpl *This = (ID3DXMeshImpl *)iface;

    TRACE("(%p)->(%p)\n", This, device);

    if (device == NULL) return D3DERR_INVALIDCALL;
    *device = This->device;
    IDirect3DDevice9_AddRef(This->device);

    return D3D_OK;
}

static HRESULT WINAPI ID3DXMeshImpl_CloneMeshFVF(ID3DXMesh *iface, DWORD options, DWORD fvf, LPDIRECT3DDEVICE9 device, LPD3DXMESH *clone_mesh)
{
    ID3DXMeshImpl *This = (ID3DXMeshImpl *)iface;

    FIXME("(%p)->(%u,%u,%p,%p): stub\n", This, options, fvf, device, clone_mesh);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXMeshImpl_CloneMesh(ID3DXMesh *iface, DWORD options, CONST D3DVERTEXELEMENT9 *declaration, LPDIRECT3DDEVICE9 device,
                                              LPD3DXMESH *clone_mesh)
{
    ID3DXMeshImpl *This = (ID3DXMeshImpl *)iface;

    FIXME("(%p)->(%u,%p,%p,%p): stub\n", This, options, declaration, device, clone_mesh);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXMeshImpl_GetVertexBuffer(ID3DXMesh *iface, LPDIRECT3DVERTEXBUFFER9 *vertex_buffer)
{
    ID3DXMeshImpl *This = (ID3DXMeshImpl *)iface;

    TRACE("(%p)->(%p)\n", This, vertex_buffer);

    if (vertex_buffer == NULL) return D3DERR_INVALIDCALL;
    *vertex_buffer = This->vertex_buffer;
    IDirect3DVertexBuffer9_AddRef(This->vertex_buffer);

    return D3D_OK;
}

static HRESULT WINAPI ID3DXMeshImpl_GetIndexBuffer(ID3DXMesh *iface, LPDIRECT3DINDEXBUFFER9 *index_buffer)
{
    ID3DXMeshImpl *This = (ID3DXMeshImpl *)iface;

    TRACE("(%p)->(%p)\n", This, index_buffer);

    if (index_buffer == NULL) return D3DERR_INVALIDCALL;
    *index_buffer = This->index_buffer;
    IDirect3DIndexBuffer9_AddRef(This->index_buffer);

    return D3D_OK;
}

static HRESULT WINAPI ID3DXMeshImpl_LockVertexBuffer(ID3DXMesh *iface, DWORD flags, LPVOID *data)
{
    ID3DXMeshImpl *This = (ID3DXMeshImpl *)iface;

    TRACE("(%p)->(%u,%p)\n", This, flags, data);

    return IDirect3DVertexBuffer9_Lock(This->vertex_buffer, 0, 0, data, flags);
}

static HRESULT WINAPI ID3DXMeshImpl_UnlockVertexBuffer(ID3DXMesh *iface)
{
    ID3DXMeshImpl *This = (ID3DXMeshImpl *)iface;

    TRACE("(%p)\n", This);

    return IDirect3DVertexBuffer9_Unlock(This->vertex_buffer);
}

static HRESULT WINAPI ID3DXMeshImpl_LockIndexBuffer(ID3DXMesh *iface, DWORD flags, LPVOID *data)
{
    ID3DXMeshImpl *This = (ID3DXMeshImpl *)iface;

    TRACE("(%p)->(%u,%p)\n", This, flags, data);

    return IDirect3DIndexBuffer9_Lock(This->index_buffer, 0, 0, data, flags);
}

static HRESULT WINAPI ID3DXMeshImpl_UnlockIndexBuffer(ID3DXMesh *iface)
{
    ID3DXMeshImpl *This = (ID3DXMeshImpl *)iface;

    TRACE("(%p)\n", This);

    return IDirect3DIndexBuffer9_Unlock(This->index_buffer);
}

static HRESULT WINAPI ID3DXMeshImpl_GetAttributeTable(ID3DXMesh *iface, D3DXATTRIBUTERANGE *attrib_table, DWORD *attrib_table_size)
{
    ID3DXMeshImpl *This = (ID3DXMeshImpl *)iface;

    FIXME("(%p)->(%p,%p): stub\n", This, attrib_table, attrib_table_size);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXMeshImpl_ConvertPointRepsToAdjacency(ID3DXMesh *iface, CONST DWORD *point_reps, DWORD *adjacency)
{
    ID3DXMeshImpl *This = (ID3DXMeshImpl *)iface;

    FIXME("(%p)->(%p,%p): stub\n", This, point_reps, adjacency);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXMeshImpl_ConvertAdjacencyToPointReps(ID3DXMesh *iface, CONST DWORD *adjacency, DWORD *point_reps)
{
    ID3DXMeshImpl *This = (ID3DXMeshImpl *)iface;

    FIXME("(%p)->(%p,%p): stub\n", This, adjacency, point_reps);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXMeshImpl_GenerateAdjacency(ID3DXMesh *iface, FLOAT epsilon, DWORD *adjacency)
{
    ID3DXMeshImpl *This = (ID3DXMeshImpl *)iface;

    FIXME("(%p)->(%f,%p): stub\n", This, epsilon, adjacency);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXMeshImpl_UpdateSemantics(ID3DXMesh *iface, D3DVERTEXELEMENT9 declaration[MAX_FVF_DECL_SIZE])
{
    ID3DXMeshImpl *This = (ID3DXMeshImpl *)iface;

    FIXME("(%p)->(%p): stub\n", This, declaration);

    return E_NOTIMPL;
}

/*** ID3DXMesh ***/
static HRESULT WINAPI ID3DXMeshImpl_LockAttributeBuffer(ID3DXMesh *iface, DWORD flags, DWORD **data)
{
    ID3DXMeshImpl *This = (ID3DXMeshImpl *)iface;

    FIXME("(%p)->(%u,%p): stub\n", This, flags, data);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXMeshImpl_UnlockAttributeBuffer(ID3DXMesh *iface)
{
    ID3DXMeshImpl *This = (ID3DXMeshImpl *)iface;

    FIXME("(%p): stub\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXMeshImpl_Optimize(ID3DXMesh *iface, DWORD flags, CONST DWORD *adjacency_in, DWORD *adjacency_out,
                                             DWORD *face_remap, LPD3DXBUFFER *vertex_remap, LPD3DXMESH *opt_mesh)
{
    ID3DXMeshImpl *This = (ID3DXMeshImpl *)iface;

    FIXME("(%p)->(%u,%p,%p,%p,%p,%p): stub\n", This, flags, adjacency_in, adjacency_out, face_remap, vertex_remap, opt_mesh);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXMeshImpl_OptimizeInplace(ID3DXMesh *iface, DWORD flags, CONST DWORD *adjacency_in, DWORD *adjacency_out,
                                                    DWORD *face_remap, LPD3DXBUFFER *vertex_remap)
{
    ID3DXMeshImpl *This = (ID3DXMeshImpl *)iface;

    FIXME("(%p)->(%u,%p,%p,%p,%p): stub\n", This, flags, adjacency_in, adjacency_out, face_remap, vertex_remap);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXMeshImpl_SetAttributeTable(ID3DXMesh *iface, CONST D3DXATTRIBUTERANGE *attrib_table, DWORD attrib_table_size)
{
    ID3DXMeshImpl *This = (ID3DXMeshImpl *)iface;

    FIXME("(%p)->(%p,%u): stub\n", This, attrib_table, attrib_table_size);

    return E_NOTIMPL;
}

static const struct ID3DXMeshVtbl D3DXMesh_Vtbl =
{
    /*** IUnknown methods ***/
    ID3DXMeshImpl_QueryInterface,
    ID3DXMeshImpl_AddRef,
    ID3DXMeshImpl_Release,
    /*** ID3DXBaseMesh ***/
    ID3DXMeshImpl_DrawSubset,
    ID3DXMeshImpl_GetNumFaces,
    ID3DXMeshImpl_GetNumVertices,
    ID3DXMeshImpl_GetFVF,
    ID3DXMeshImpl_GetDeclaration,
    ID3DXMeshImpl_GetNumBytesPerVertex,
    ID3DXMeshImpl_GetOptions,
    ID3DXMeshImpl_GetDevice,
    ID3DXMeshImpl_CloneMeshFVF,
    ID3DXMeshImpl_CloneMesh,
    ID3DXMeshImpl_GetVertexBuffer,
    ID3DXMeshImpl_GetIndexBuffer,
    ID3DXMeshImpl_LockVertexBuffer,
    ID3DXMeshImpl_UnlockVertexBuffer,
    ID3DXMeshImpl_LockIndexBuffer,
    ID3DXMeshImpl_UnlockIndexBuffer,
    ID3DXMeshImpl_GetAttributeTable,
    ID3DXMeshImpl_ConvertPointRepsToAdjacency,
    ID3DXMeshImpl_ConvertAdjacencyToPointReps,
    ID3DXMeshImpl_GenerateAdjacency,
    ID3DXMeshImpl_UpdateSemantics,
    /*** ID3DXMesh ***/
    ID3DXMeshImpl_LockAttributeBuffer,
    ID3DXMeshImpl_UnlockAttributeBuffer,
    ID3DXMeshImpl_Optimize,
    ID3DXMeshImpl_OptimizeInplace,
    ID3DXMeshImpl_SetAttributeTable
};

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

    if (fvf & (D3DFVF_RESERVED0 | D3DFVF_RESERVED2)) return D3DERR_INVALIDCALL;

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
 * D3DXGetDeclLength
 */
UINT WINAPI D3DXGetDeclLength(const D3DVERTEXELEMENT9 *decl)
{
    const D3DVERTEXELEMENT9 *element;

    TRACE("decl %p\n", decl);

    /* null decl results in exception on Windows XP */

    for (element = decl; element->Stream != 0xff; ++element);

    return element - decl;
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

/*************************************************************************
 * D3DXCreateMesh
 */
HRESULT WINAPI D3DXCreateMesh(DWORD numfaces, DWORD numvertices, DWORD options, CONST D3DVERTEXELEMENT9 *declaration,
                              LPDIRECT3DDEVICE9 device, LPD3DXMESH *mesh)
{
    HRESULT hr;
    DWORD fvf;
    IDirect3DVertexDeclaration9 *vertex_declaration;
    IDirect3DVertexBuffer9 *vertex_buffer;
    IDirect3DIndexBuffer9 *index_buffer;
    ID3DXMeshImpl *object;

    TRACE("(%d, %d, %d, %p, %p, %p)\n", numfaces, numvertices, options, declaration, device, mesh);

    if (numfaces == 0 || numvertices == 0 || declaration == NULL || device == NULL || mesh == NULL)
    {
        return D3DERR_INVALIDCALL;
    }

    hr = D3DXFVFFromDeclarator(declaration, &fvf);
    if (hr != D3D_OK)
    {
        fvf = 0;
    }

    /* Create vertex declaration */
    hr = IDirect3DDevice9_CreateVertexDeclaration(device,
                                                  declaration,
                                                  &vertex_declaration);
    if (FAILED(hr))
    {
        WARN("Unexpected return value %x from IDirect3DDevice9_CreateVertexDeclaration.\n",hr);
        return hr;
    }

    /* Create vertex buffer */
    hr = IDirect3DDevice9_CreateVertexBuffer(device,
                                             numvertices * D3DXGetDeclVertexSize(declaration, declaration[0].Stream),
                                             0,
                                             fvf,
                                             D3DPOOL_MANAGED,
                                             &vertex_buffer,
                                             NULL);
    if (FAILED(hr))
    {
        WARN("Unexpected return value %x from IDirect3DDevice9_CreateVertexBuffer.\n",hr);
        IDirect3DVertexDeclaration9_Release(vertex_declaration);
        return hr;
    }

    /* Create index buffer */
    hr = IDirect3DDevice9_CreateIndexBuffer(device,
                                            numfaces * 6, /* 3 vertices per triangle, 2 triangles per face */
                                            0,
                                            D3DFMT_INDEX16,
                                            D3DPOOL_MANAGED,
                                            &index_buffer,
                                            NULL);
    if (FAILED(hr))
    {
        WARN("Unexpected return value %x from IDirect3DDevice9_CreateVertexBuffer.\n",hr);
        IDirect3DVertexBuffer9_Release(vertex_buffer);
        IDirect3DVertexDeclaration9_Release(vertex_declaration);
        return hr;
    }

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(ID3DXMeshImpl));
    if (object == NULL)
    {
        IDirect3DIndexBuffer9_Release(index_buffer);
        IDirect3DVertexBuffer9_Release(vertex_buffer);
        IDirect3DVertexDeclaration9_Release(vertex_declaration);
        *mesh = NULL;
        return E_OUTOFMEMORY;
    }
    object->lpVtbl = &D3DXMesh_Vtbl;
    object->ref = 1;

    object->numfaces = numfaces;
    object->numvertices = numvertices;
    object->options = options;
    object->fvf = fvf;
    object->device = device;
    IDirect3DDevice9_AddRef(device);

    object->vertex_declaration = vertex_declaration;
    object->vertex_buffer = vertex_buffer;
    object->index_buffer = index_buffer;

    *mesh = (ID3DXMesh*)object;

    return D3D_OK;
}

/*************************************************************************
 * D3DXCreateMeshFVF
 */
HRESULT WINAPI D3DXCreateMeshFVF(DWORD numfaces, DWORD numvertices, DWORD options, DWORD fvf,
                                 LPDIRECT3DDEVICE9 device, LPD3DXMESH *mesh)
{
    HRESULT hr;
    D3DVERTEXELEMENT9 declaration[MAX_FVF_DECL_SIZE];

    TRACE("(%u, %u, %u, %u, %p, %p)\n", numfaces, numvertices, options, fvf, device, mesh);

    hr = D3DXDeclaratorFromFVF(fvf, declaration);
    if (FAILED(hr)) return hr;

    return D3DXCreateMesh(numfaces, numvertices, options, declaration, device, mesh);
}

HRESULT WINAPI D3DXCreateBox(LPDIRECT3DDEVICE9 device, FLOAT width, FLOAT height,
                             FLOAT depth, LPD3DXMESH* mesh, LPD3DXBUFFER* adjacency)
{
    FIXME("(%p, %f, %f, %f, %p, %p): stub\n", device, width, height, depth, mesh, adjacency);

    return E_NOTIMPL;
}

struct vertex
{
    D3DXVECTOR3 position;
    D3DXVECTOR3 normal;
};

typedef WORD face[3];

struct sincos_table
{
    float *sin;
    float *cos;
};

static void free_sincos_table(struct sincos_table *sincos_table)
{
    HeapFree(GetProcessHeap(), 0, sincos_table->cos);
    HeapFree(GetProcessHeap(), 0, sincos_table->sin);
}

/* pre compute sine and cosine tables; caller must free */
static BOOL compute_sincos_table(struct sincos_table *sincos_table, float angle_start, float angle_step, int n)
{
    float angle;
    int i;

    sincos_table->sin = HeapAlloc(GetProcessHeap(), 0, n * sizeof(*sincos_table->sin));
    if (!sincos_table->sin)
    {
        return FALSE;
    }
    sincos_table->cos = HeapAlloc(GetProcessHeap(), 0, n * sizeof(*sincos_table->cos));
    if (!sincos_table->cos)
    {
        HeapFree(GetProcessHeap(), 0, sincos_table->sin);
        return FALSE;
    }

    angle = angle_start;
    for (i = 0; i < n; i++)
    {
        sincos_table->sin[i] = sin(angle);
        sincos_table->cos[i] = cos(angle);
        angle += angle_step;
    }

    return TRUE;
}

static WORD vertex_index(UINT slices, int slice, int stack)
{
    return stack*slices+slice+1;
}

HRESULT WINAPI D3DXCreateSphere(LPDIRECT3DDEVICE9 device, FLOAT radius, UINT slices,
                                UINT stacks, LPD3DXMESH* mesh, LPD3DXBUFFER* adjacency)
{
    DWORD number_of_vertices, number_of_faces;
    HRESULT hr;
    ID3DXMesh *sphere;
    struct vertex *vertices;
    face *faces;
    float phi_step, phi_start;
    struct sincos_table phi;
    float theta_step, theta, sin_theta, cos_theta;
    DWORD vertex, face;
    int slice, stack;

    TRACE("(%p, %f, %u, %u, %p, %p)\n", device, radius, slices, stacks, mesh, adjacency);

    if (!device || radius < 0.0f || slices < 2 || stacks < 2 || !mesh)
    {
        return D3DERR_INVALIDCALL;
    }

    if (adjacency)
    {
        FIXME("Case of adjacency != NULL not implemented.\n");
        return E_NOTIMPL;
    }

    number_of_vertices = 2 + slices * (stacks-1);
    number_of_faces = 2 * slices + (stacks - 2) * (2 * slices);

    hr = D3DXCreateMeshFVF(number_of_faces, number_of_vertices, D3DXMESH_MANAGED,
                           D3DFVF_XYZ | D3DFVF_NORMAL, device, &sphere);
    if (FAILED(hr))
    {
        return hr;
    }

    hr = sphere->lpVtbl->LockVertexBuffer(sphere, D3DLOCK_DISCARD, (LPVOID *)&vertices);
    if (FAILED(hr))
    {
        sphere->lpVtbl->Release(sphere);
        return hr;
    }

    hr = sphere->lpVtbl->LockIndexBuffer(sphere, D3DLOCK_DISCARD, (LPVOID *)&faces);
    if (FAILED(hr))
    {
        sphere->lpVtbl->UnlockVertexBuffer(sphere);
        sphere->lpVtbl->Release(sphere);
        return hr;
    }

    /* phi = angle on xz plane wrt z axis */
    phi_step = -2 * M_PI / slices;
    phi_start = M_PI / 2;

    if (!compute_sincos_table(&phi, phi_start, phi_step, slices))
    {
        sphere->lpVtbl->UnlockIndexBuffer(sphere);
        sphere->lpVtbl->UnlockVertexBuffer(sphere);
        sphere->lpVtbl->Release(sphere);
        return E_OUTOFMEMORY;
    }

    /* theta = angle on xy plane wrt x axis */
    theta_step = M_PI / stacks;
    theta = theta_step;

    vertex = 0;
    face = 0;
    stack = 0;

    vertices[vertex].normal.x = 0.0f;
    vertices[vertex].normal.y = 0.0f;
    vertices[vertex].normal.z = 1.0f;
    vertices[vertex].position.x = 0.0f;
    vertices[vertex].position.y = 0.0f;
    vertices[vertex].position.z = radius;
    vertex++;

    for (stack = 0; stack < stacks - 1; stack++)
    {
        sin_theta = sin(theta);
        cos_theta = cos(theta);

        for (slice = 0; slice < slices; slice++)
        {
            vertices[vertex].normal.x = sin_theta * phi.cos[slice];
            vertices[vertex].normal.y = sin_theta * phi.sin[slice];
            vertices[vertex].normal.z = cos_theta;
            vertices[vertex].position.x = radius * sin_theta * phi.cos[slice];
            vertices[vertex].position.y = radius * sin_theta * phi.sin[slice];
            vertices[vertex].position.z = radius * cos_theta;
            vertex++;

            if (slice > 0)
            {
                if (stack == 0)
                {
                    /* top stack is triangle fan */
                    faces[face][0] = 0;
                    faces[face][1] = slice + 1;
                    faces[face][2] = slice;
                    face++;
                }
                else
                {
                    /* stacks in between top and bottom are quad strips */
                    faces[face][0] = vertex_index(slices, slice-1, stack-1);
                    faces[face][1] = vertex_index(slices, slice, stack-1);
                    faces[face][2] = vertex_index(slices, slice-1, stack);
                    face++;

                    faces[face][0] = vertex_index(slices, slice, stack-1);
                    faces[face][1] = vertex_index(slices, slice, stack);
                    faces[face][2] = vertex_index(slices, slice-1, stack);
                    face++;
                }
            }
        }

        theta += theta_step;

        if (stack == 0)
        {
            faces[face][0] = 0;
            faces[face][1] = 1;
            faces[face][2] = slice;
            face++;
        }
        else
        {
            faces[face][0] = vertex_index(slices, slice-1, stack-1);
            faces[face][1] = vertex_index(slices, 0, stack-1);
            faces[face][2] = vertex_index(slices, slice-1, stack);
            face++;

            faces[face][0] = vertex_index(slices, 0, stack-1);
            faces[face][1] = vertex_index(slices, 0, stack);
            faces[face][2] = vertex_index(slices, slice-1, stack);
            face++;
        }
    }

    vertices[vertex].position.x = 0.0f;
    vertices[vertex].position.y = 0.0f;
    vertices[vertex].position.z = -radius;
    vertices[vertex].normal.x = 0.0f;
    vertices[vertex].normal.y = 0.0f;
    vertices[vertex].normal.z = -1.0f;

    /* bottom stack is triangle fan */
    for (slice = 1; slice < slices; slice++)
    {
        faces[face][0] = vertex_index(slices, slice-1, stack-1);
        faces[face][1] = vertex_index(slices, slice, stack-1);
        faces[face][2] = vertex;
        face++;
    }

    faces[face][0] = vertex_index(slices, slice-1, stack-1);
    faces[face][1] = vertex_index(slices, 0, stack-1);
    faces[face][2] = vertex;

    free_sincos_table(&phi);
    sphere->lpVtbl->UnlockIndexBuffer(sphere);
    sphere->lpVtbl->UnlockVertexBuffer(sphere);
    *mesh = sphere;

    return D3D_OK;
}

HRESULT WINAPI D3DXCreateCylinder(LPDIRECT3DDEVICE9 device, FLOAT radius1, FLOAT radius2, FLOAT length, UINT slices,
                                  UINT stacks, LPD3DXMESH* mesh, LPD3DXBUFFER* adjacency)
{
    DWORD number_of_vertices, number_of_faces;
    HRESULT hr;
    ID3DXMesh *cylinder;
    struct vertex *vertices;
    face *faces;
    float theta_step, theta_start;
    struct sincos_table theta;
    float delta_radius, radius, radius_step;
    float z, z_step, z_normal;
    DWORD vertex, face;
    int slice, stack;

    TRACE("(%p, %f, %f, %f, %u, %u, %p, %p)\n", device, radius1, radius2, length, slices, stacks, mesh, adjacency);

    if (device == NULL || radius1 < 0.0f || radius2 < 0.0f || length < 0.0f || slices < 2 || stacks < 1 || mesh == NULL)
    {
        return D3DERR_INVALIDCALL;
    }

    if (adjacency)
    {
        FIXME("Case of adjacency != NULL not implemented.\n");
        return E_NOTIMPL;
    }

    number_of_vertices = 2 + (slices * (3 + stacks));
    number_of_faces = 2 * slices + stacks * (2 * slices);

    hr = D3DXCreateMeshFVF(number_of_faces, number_of_vertices, D3DXMESH_MANAGED,
                           D3DFVF_XYZ | D3DFVF_NORMAL, device, &cylinder);
    if (FAILED(hr))
    {
        return hr;
    }

    hr = cylinder->lpVtbl->LockVertexBuffer(cylinder, D3DLOCK_DISCARD, (LPVOID *)&vertices);
    if (FAILED(hr))
    {
        cylinder->lpVtbl->Release(cylinder);
        return hr;
    }

    hr = cylinder->lpVtbl->LockIndexBuffer(cylinder, D3DLOCK_DISCARD, (LPVOID *)&faces);
    if (FAILED(hr))
    {
        cylinder->lpVtbl->UnlockVertexBuffer(cylinder);
        cylinder->lpVtbl->Release(cylinder);
        return hr;
    }

    /* theta = angle on xy plane wrt x axis */
    theta_step = -2 * M_PI / slices;
    theta_start = M_PI / 2;

    if (!compute_sincos_table(&theta, theta_start, theta_step, slices))
    {
        cylinder->lpVtbl->UnlockIndexBuffer(cylinder);
        cylinder->lpVtbl->UnlockVertexBuffer(cylinder);
        cylinder->lpVtbl->Release(cylinder);
        return E_OUTOFMEMORY;
    }

    vertex = 0;
    face = 0;
    stack = 0;

    delta_radius = radius1 - radius2;
    radius = radius1;
    radius_step = delta_radius / stacks;

    z = -length / 2;
    z_step = length / stacks;
    z_normal = delta_radius / length;
    if (isnan(z_normal))
    {
        z_normal = 0.0f;
    }

    vertices[vertex].normal.x = 0.0f;
    vertices[vertex].normal.y = 0.0f;
    vertices[vertex].normal.z = -1.0f;
    vertices[vertex].position.x = 0.0f;
    vertices[vertex].position.y = 0.0f;
    vertices[vertex++].position.z = z;

    for (slice = 0; slice < slices; slice++, vertex++)
    {
        vertices[vertex].normal.x = 0.0f;
        vertices[vertex].normal.y = 0.0f;
        vertices[vertex].normal.z = -1.0f;
        vertices[vertex].position.x = radius * theta.cos[slice];
        vertices[vertex].position.y = radius * theta.sin[slice];
        vertices[vertex].position.z = z;

        if (slice > 0)
        {
            faces[face][0] = 0;
            faces[face][1] = slice;
            faces[face++][2] = slice + 1;
        }
    }

    faces[face][0] = 0;
    faces[face][1] = slice;
    faces[face++][2] = 1;

    for (stack = 1; stack <= stacks+1; stack++)
    {
        for (slice = 0; slice < slices; slice++, vertex++)
        {
            vertices[vertex].normal.x = theta.cos[slice];
            vertices[vertex].normal.y = theta.sin[slice];
            vertices[vertex].normal.z = z_normal;
            D3DXVec3Normalize(&vertices[vertex].normal, &vertices[vertex].normal);
            vertices[vertex].position.x = radius * theta.cos[slice];
            vertices[vertex].position.y = radius * theta.sin[slice];
            vertices[vertex].position.z = z;

            if (stack > 1 && slice > 0)
            {
                faces[face][0] = vertex_index(slices, slice-1, stack-1);
                faces[face][1] = vertex_index(slices, slice-1, stack);
                faces[face++][2] = vertex_index(slices, slice, stack-1);

                faces[face][0] = vertex_index(slices, slice, stack-1);
                faces[face][1] = vertex_index(slices, slice-1, stack);
                faces[face++][2] = vertex_index(slices, slice, stack);
            }
        }

        if (stack > 1)
        {
            faces[face][0] = vertex_index(slices, slice-1, stack-1);
            faces[face][1] = vertex_index(slices, slice-1, stack);
            faces[face++][2] = vertex_index(slices, 0, stack-1);

            faces[face][0] = vertex_index(slices, 0, stack-1);
            faces[face][1] = vertex_index(slices, slice-1, stack);
            faces[face++][2] = vertex_index(slices, 0, stack);
        }

        if (stack < stacks + 1)
        {
            z += z_step;
            radius -= radius_step;
        }
    }

    for (slice = 0; slice < slices; slice++, vertex++)
    {
        vertices[vertex].normal.x = 0.0f;
        vertices[vertex].normal.y = 0.0f;
        vertices[vertex].normal.z = 1.0f;
        vertices[vertex].position.x = radius * theta.cos[slice];
        vertices[vertex].position.y = radius * theta.sin[slice];
        vertices[vertex].position.z = z;

        if (slice > 0)
        {
            faces[face][0] = vertex_index(slices, slice-1, stack);
            faces[face][1] = number_of_vertices - 1;
            faces[face++][2] = vertex_index(slices, slice, stack);
        }
    }

    vertices[vertex].position.x = 0.0f;
    vertices[vertex].position.y = 0.0f;
    vertices[vertex].position.z = z;
    vertices[vertex].normal.x = 0.0f;
    vertices[vertex].normal.y = 0.0f;
    vertices[vertex].normal.z = 1.0f;

    faces[face][0] = vertex_index(slices, slice-1, stack);
    faces[face][1] = number_of_vertices - 1;
    faces[face][2] = vertex_index(slices, 0, stack);

    free_sincos_table(&theta);
    cylinder->lpVtbl->UnlockIndexBuffer(cylinder);
    cylinder->lpVtbl->UnlockVertexBuffer(cylinder);
    *mesh = cylinder;

    return D3D_OK;
}

HRESULT WINAPI D3DXCreateTeapot(LPDIRECT3DDEVICE9 device, LPD3DXMESH *mesh, LPD3DXBUFFER* adjacency)
{
    FIXME("(%p, %p, %p): stub\n", device, mesh, adjacency);

    return E_NOTIMPL;
}
