/*
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

#include <d3dx9.h>

#ifndef __WINE_D3DX9MESH_H
#define __WINE_D3DX9MESH_H

DEFINE_GUID(IID_ID3DXBaseMesh, 0x7ed943dd, 0x52e8, 0x40b5, 0xa8, 0xd8, 0x76, 0x68, 0x5c, 0x40, 0x63, 0x30);
DEFINE_GUID(IID_ID3DXMesh,     0x4020e5c2, 0x1403, 0x4929, 0x88, 0x3f, 0xe2, 0xe8, 0x49, 0xfa, 0xc1, 0x95);

enum _MAX_FVF_DECL_SIZE
{
    MAX_FVF_DECL_SIZE = MAXD3DDECLLENGTH + 1
};

typedef struct ID3DXBaseMesh* LPD3DXBASEMESH;
typedef struct ID3DXMesh* LPD3DXMESH;

typedef struct _D3DXATTRIBUTERANGE {
    DWORD AttribId;
    DWORD FaceStart;
    DWORD FaceCount;
    DWORD VertexStart;
    DWORD VertexCount;
} D3DXATTRIBUTERANGE;

typedef D3DXATTRIBUTERANGE* LPD3DXATTRIBUTERANGE;

#undef INTERFACE
#define INTERFACE ID3DXBaseMesh

DECLARE_INTERFACE_(ID3DXBaseMesh, IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID* object) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    /*** ID3DXBaseMesh ***/
    STDMETHOD(DrawSubset)(THIS_ DWORD attrib_id) PURE;
    STDMETHOD_(DWORD, GetNumFaces)(THIS) PURE;
    STDMETHOD_(DWORD, GetNumVertices)(THIS) PURE;
    STDMETHOD_(DWORD, GetFVF)(THIS) PURE;
    STDMETHOD(GetDeclaration)(THIS_ D3DVERTEXELEMENT9 declaration[MAX_FVF_DECL_SIZE]) PURE;
    STDMETHOD_(DWORD, GetNumBytesPerVertex)(THIS) PURE;
    STDMETHOD_(DWORD, GetOptions)(THIS) PURE;
    STDMETHOD(GetDevice)(THIS_ LPDIRECT3DDEVICE9* device) PURE;
    STDMETHOD(CloneMeshFVF)(THIS_ DWORD options, DWORD fvf, LPDIRECT3DDEVICE9 device, LPD3DXMESH* clone_mesh) PURE;
    STDMETHOD(CloneMesh)(THIS_ DWORD options, CONST D3DVERTEXELEMENT9* declaration, LPDIRECT3DDEVICE9 device,
        LPD3DXMESH* clone_mesh) PURE;
    STDMETHOD(GetVertexBuffer)(THIS_ LPDIRECT3DVERTEXBUFFER9* vertex_buffer) PURE;
    STDMETHOD(GetIndexBuffer)(THIS_ LPDIRECT3DINDEXBUFFER9* index_buffer) PURE;
    STDMETHOD(LockVertexBuffer)(THIS_ DWORD flags, LPVOID* data) PURE;
    STDMETHOD(UnlockVertexBuffer)(THIS) PURE;
    STDMETHOD(LockIndexBuffer)(THIS_ DWORD flags, LPVOID* data) PURE;
    STDMETHOD(UnlockIndexBuffer)(THIS) PURE;
    STDMETHOD(GetAttributeTable)(THIS_ D3DXATTRIBUTERANGE* attrib_table, DWORD* attrib_table_size) PURE;
    STDMETHOD(ConvertPointRepsToAdjacency)(THIS_ CONST DWORD* point_reps, DWORD* adjacency) PURE;
    STDMETHOD(ConvertAdjacencyToPointReps)(THIS_ CONST DWORD* adjacency, DWORD* point_reps) PURE;
    STDMETHOD(GenerateAdjacency)(THIS_ FLOAT epsilon, DWORD* adjacency) PURE;
    STDMETHOD(UpdateSemantics)(THIS_ D3DVERTEXELEMENT9 declaration[MAX_FVF_DECL_SIZE]) PURE;
};


#undef INTERFACE
#define INTERFACE ID3DXMesh

DECLARE_INTERFACE_(ID3DXMesh, ID3DXBaseMesh)
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID* object) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    /*** ID3DXBaseMesh ***/
    STDMETHOD(DrawSubset)(THIS_ DWORD attrib_id) PURE;
    STDMETHOD_(DWORD, GetNumFaces)(THIS) PURE;
    STDMETHOD_(DWORD, GetNumVertices)(THIS) PURE;
    STDMETHOD_(DWORD, GetFVF)(THIS) PURE;
    STDMETHOD(GetDeclaration)(THIS_ D3DVERTEXELEMENT9 declaration[MAX_FVF_DECL_SIZE]) PURE;
    STDMETHOD_(DWORD, GetNumBytesPerVertex)(THIS) PURE;
    STDMETHOD_(DWORD, GetOptions)(THIS) PURE;
    STDMETHOD(GetDevice)(THIS_ LPDIRECT3DDEVICE9* device) PURE;
    STDMETHOD(CloneMeshFVF)(THIS_ DWORD options, DWORD fvf, LPDIRECT3DDEVICE9 device, LPD3DXMESH* clone_mesh) PURE;
    STDMETHOD(CloneMesh)(THIS_ DWORD options, CONST D3DVERTEXELEMENT9* declaration, LPDIRECT3DDEVICE9 device,
        LPD3DXMESH* clone_mesh) PURE;
    STDMETHOD(GetVertexBuffer)(THIS_ LPDIRECT3DVERTEXBUFFER9* vertex_buffer) PURE;
    STDMETHOD(GetIndexBuffer)(THIS_ LPDIRECT3DINDEXBUFFER9* index_buffer) PURE;
    STDMETHOD(LockVertexBuffer)(THIS_ DWORD flags, LPVOID* data) PURE;
    STDMETHOD(UnlockVertexBuffer)(THIS) PURE;
    STDMETHOD(LockIndexBuffer)(THIS_ DWORD flags, LPVOID* data) PURE;
    STDMETHOD(UnlockIndexBuffer)(THIS) PURE;
    STDMETHOD(GetAttributeTable)(THIS_ D3DXATTRIBUTERANGE* attrib_table, DWORD* attrib_table_size) PURE;
    STDMETHOD(ConvertPointRepsToAdjacency)(THIS_ CONST DWORD* point_reps, DWORD* adjacency) PURE;
    STDMETHOD(ConvertAdjacencyToPointReps)(THIS_ CONST DWORD* adjacency, DWORD* point_reps) PURE;
    STDMETHOD(GenerateAdjacency)(THIS_ FLOAT epsilon, DWORD* adjacency) PURE;
    STDMETHOD(UpdateSemantics)(THIS_ D3DVERTEXELEMENT9 declaration[MAX_FVF_DECL_SIZE]) PURE;
    /*** ID3DXMesh ***/
    STDMETHOD(LockAttributeBuffer)(THIS_ DWORD flags, DWORD** data) PURE;
    STDMETHOD(UnlockAttributeBuffer)(THIS) PURE;
    STDMETHOD(Optimize)(THIS_ DWORD flags, CONST DWORD* adjacency_in, DWORD* adjacency_out,
        DWORD* face_remap, LPD3DXBUFFER* vertex_remap, LPD3DXMESH* opt_mesh) PURE;
    STDMETHOD(OptimizeInplace)(THIS_ DWORD flags, CONST DWORD* adjacency_in, DWORD* adjacency_out,
                     DWORD* face_remap, LPD3DXBUFFER* vertex_remap) PURE;
    STDMETHOD(SetAttributeTable)(THIS_ CONST D3DXATTRIBUTERANGE* attrib_table, DWORD attrib_table_size) PURE;
};

#ifdef __cplusplus
extern "C" {
#endif

HRESULT WINAPI D3DXCreateBuffer(DWORD, LPD3DXBUFFER*);
UINT    WINAPI D3DXGetDeclVertexSize(const D3DVERTEXELEMENT9 *decl, DWORD stream_idx);
UINT    WINAPI D3DXGetFVFVertexSize(DWORD);
BOOL    WINAPI D3DXBoxBoundProbe(CONST D3DXVECTOR3 *, CONST D3DXVECTOR3 *, CONST D3DXVECTOR3 *, CONST D3DXVECTOR3 *);
BOOL    WINAPI D3DXSphereBoundProbe(CONST D3DXVECTOR3 *,FLOAT,CONST D3DXVECTOR3 *,CONST D3DXVECTOR3 *);
HRESULT WINAPI D3DXComputeBoundingBox(CONST D3DXVECTOR3 *, DWORD, DWORD, D3DXVECTOR3 *, D3DXVECTOR3 *);
HRESULT WINAPI D3DXComputeBoundingSphere(CONST D3DXVECTOR3 *, DWORD, DWORD, D3DXVECTOR3 *, FLOAT *);
HRESULT WINAPI D3DXDeclaratorFromFVF(DWORD, D3DVERTEXELEMENT9[MAX_FVF_DECL_SIZE]);
BOOL    WINAPI D3DXIntersectTri(CONST D3DXVECTOR3 *, CONST D3DXVECTOR3 *, CONST D3DXVECTOR3 *, CONST D3DXVECTOR3 *, CONST D3DXVECTOR3*, FLOAT *, FLOAT *, FLOAT *);

#ifdef __cplusplus
}
#endif

#endif /* __WINE_D3DX9MESH_H */
