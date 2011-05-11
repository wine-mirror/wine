 /*
 * Mesh operations specific to D3DX9.
 *
 * Copyright (C) 2005 Henri Verbeet
 * Copyright (C) 2006 Ivan Gyurdiev
 * Copyright (C) 2009 David Adam
 * Copyright (C) 2010 Tony Wasserka
 * Copyright (C) 2011 Dylan Smith
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

#define COBJMACROS
#define NONAMELESSUNION
#include "windef.h"
#include "wingdi.h"
#include "d3dx9.h"
#include "wine/debug.h"
#include "wine/unicode.h"
#include "d3dx9_36_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);

typedef struct ID3DXMeshImpl
{
    ID3DXMesh ID3DXMesh_iface;
    LONG ref;

    DWORD numfaces;
    DWORD numvertices;
    DWORD options;
    DWORD fvf;
    IDirect3DDevice9 *device;
    IDirect3DVertexDeclaration9 *vertex_declaration;
    IDirect3DVertexBuffer9 *vertex_buffer;
    IDirect3DIndexBuffer9 *index_buffer;
    DWORD *attrib_buffer;
    int attrib_buffer_lock_count;
    DWORD attrib_table_size;
    D3DXATTRIBUTERANGE *attrib_table;
} ID3DXMeshImpl;

static inline ID3DXMeshImpl *impl_from_ID3DXMesh(ID3DXMesh *iface)
{
    return CONTAINING_RECORD(iface, ID3DXMeshImpl, ID3DXMesh_iface);
}

static HRESULT WINAPI ID3DXMeshImpl_QueryInterface(ID3DXMesh *iface, REFIID riid, LPVOID *object)
{
    ID3DXMeshImpl *This = impl_from_ID3DXMesh(iface);

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
    ID3DXMeshImpl *This = impl_from_ID3DXMesh(iface);

    TRACE("(%p)->(): AddRef from %d\n", This, This->ref);

    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI ID3DXMeshImpl_Release(ID3DXMesh *iface)
{
    ID3DXMeshImpl *This = impl_from_ID3DXMesh(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(): Release from %d\n", This, ref + 1);

    if (!ref)
    {
        IDirect3DIndexBuffer9_Release(This->index_buffer);
        IDirect3DVertexBuffer9_Release(This->vertex_buffer);
        IDirect3DVertexDeclaration9_Release(This->vertex_declaration);
        IDirect3DDevice9_Release(This->device);
        HeapFree(GetProcessHeap(), 0, This->attrib_buffer);
        HeapFree(GetProcessHeap(), 0, This->attrib_table);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

/*** ID3DXBaseMesh ***/
static HRESULT WINAPI ID3DXMeshImpl_DrawSubset(ID3DXMesh *iface, DWORD attrib_id)
{
    ID3DXMeshImpl *This = impl_from_ID3DXMesh(iface);
    HRESULT hr;
    DWORD face_start;
    DWORD face_end = 0;
    DWORD vertex_size;

    TRACE("(%p)->(%u)\n", This, attrib_id);

    vertex_size = iface->lpVtbl->GetNumBytesPerVertex(iface);

    hr = IDirect3DDevice9_SetVertexDeclaration(This->device, This->vertex_declaration);
    if (FAILED(hr)) return hr;
    hr = IDirect3DDevice9_SetStreamSource(This->device, 0, This->vertex_buffer, 0, vertex_size);
    if (FAILED(hr)) return hr;
    hr = IDirect3DDevice9_SetIndices(This->device, This->index_buffer);
    if (FAILED(hr)) return hr;

    while (face_end < This->numfaces)
    {
        for (face_start = face_end; face_start < This->numfaces; face_start++)
        {
            if (This->attrib_buffer[face_start] == attrib_id)
                break;
        }
        if (face_start >= This->numfaces)
            break;
        for (face_end = face_start + 1; face_end < This->numfaces; face_end++)
        {
            if (This->attrib_buffer[face_end] != attrib_id)
                break;
        }

        hr = IDirect3DDevice9_DrawIndexedPrimitive(This->device, D3DPT_TRIANGLELIST,
                0, 0, This->numvertices, face_start * 3, face_end - face_start);
        if (FAILED(hr)) return hr;
    }

    return D3D_OK;
}

static DWORD WINAPI ID3DXMeshImpl_GetNumFaces(ID3DXMesh *iface)
{
    ID3DXMeshImpl *This = impl_from_ID3DXMesh(iface);

    TRACE("(%p)\n", This);

    return This->numfaces;
}

static DWORD WINAPI ID3DXMeshImpl_GetNumVertices(ID3DXMesh *iface)
{
    ID3DXMeshImpl *This = impl_from_ID3DXMesh(iface);

    TRACE("(%p)\n", This);

    return This->numvertices;
}

static DWORD WINAPI ID3DXMeshImpl_GetFVF(ID3DXMesh *iface)
{
    ID3DXMeshImpl *This = impl_from_ID3DXMesh(iface);

    TRACE("(%p)\n", This);

    return This->fvf;
}

static HRESULT WINAPI ID3DXMeshImpl_GetDeclaration(ID3DXMesh *iface, D3DVERTEXELEMENT9 declaration[MAX_FVF_DECL_SIZE])
{
    ID3DXMeshImpl *This = impl_from_ID3DXMesh(iface);
    UINT numelements;

    TRACE("(%p)\n", This);

    if (declaration == NULL) return D3DERR_INVALIDCALL;

    return IDirect3DVertexDeclaration9_GetDeclaration(This->vertex_declaration,
                                                      declaration,
                                                      &numelements);
}

static DWORD WINAPI ID3DXMeshImpl_GetNumBytesPerVertex(ID3DXMesh *iface)
{
    ID3DXMeshImpl *This = impl_from_ID3DXMesh(iface);
    UINT numelements;
    D3DVERTEXELEMENT9 declaration[MAX_FVF_DECL_SIZE] = { D3DDECL_END() };

    TRACE("iface (%p)\n", This);

    IDirect3DVertexDeclaration9_GetDeclaration(This->vertex_declaration,
                                               declaration,
                                               &numelements);
    return D3DXGetDeclVertexSize(declaration, 0);
}

static DWORD WINAPI ID3DXMeshImpl_GetOptions(ID3DXMesh *iface)
{
    ID3DXMeshImpl *This = impl_from_ID3DXMesh(iface);

    TRACE("(%p)\n", This);

    return This->options;
}

static HRESULT WINAPI ID3DXMeshImpl_GetDevice(ID3DXMesh *iface, LPDIRECT3DDEVICE9 *device)
{
    ID3DXMeshImpl *This = impl_from_ID3DXMesh(iface);

    TRACE("(%p)->(%p)\n", This, device);

    if (device == NULL) return D3DERR_INVALIDCALL;
    *device = This->device;
    IDirect3DDevice9_AddRef(This->device);

    return D3D_OK;
}

static HRESULT WINAPI ID3DXMeshImpl_CloneMeshFVF(ID3DXMesh *iface, DWORD options, DWORD fvf, LPDIRECT3DDEVICE9 device, LPD3DXMESH *clone_mesh)
{
    ID3DXMeshImpl *This = impl_from_ID3DXMesh(iface);
    HRESULT hr;
    D3DVERTEXELEMENT9 declaration[MAX_FVF_DECL_SIZE];

    TRACE("(%p)->(%x,%x,%p,%p)\n", This, options, fvf, device, clone_mesh);

    hr = D3DXDeclaratorFromFVF(fvf, declaration);
    if (FAILED(hr)) return hr;

    return iface->lpVtbl->CloneMesh(iface, options, declaration, device, clone_mesh);
}

static HRESULT WINAPI ID3DXMeshImpl_CloneMesh(ID3DXMesh *iface, DWORD options, CONST D3DVERTEXELEMENT9 *declaration, LPDIRECT3DDEVICE9 device,
                                              LPD3DXMESH *clone_mesh_out)
{
    ID3DXMeshImpl *This = impl_from_ID3DXMesh(iface);
    ID3DXMeshImpl *cloned_this;
    ID3DXMesh *clone_mesh;
    D3DVERTEXELEMENT9 orig_declaration[MAX_FVF_DECL_SIZE] = { D3DDECL_END() };
    void *data_in, *data_out;
    DWORD vertex_size;
    HRESULT hr;
    int i;

    TRACE("(%p)->(%x,%p,%p,%p)\n", This, options, declaration, device, clone_mesh_out);

    if (!clone_mesh_out)
        return D3DERR_INVALIDCALL;

    hr = iface->lpVtbl->GetDeclaration(iface, orig_declaration);
    if (FAILED(hr)) return hr;

    for (i = 0; orig_declaration[i].Stream != 0xff; i++) {
        if (memcmp(&orig_declaration[i], &declaration[i], sizeof(*declaration)))
        {
            FIXME("Vertex buffer conversion not implemented.\n");
            return E_NOTIMPL;
        }
    }

    hr = D3DXCreateMesh(This->numfaces, This->numvertices, options & ~D3DXMESH_VB_SHARE,
                        declaration, device, &clone_mesh);
    if (FAILED(hr)) return hr;

    cloned_this = impl_from_ID3DXMesh(clone_mesh);
    vertex_size = clone_mesh->lpVtbl->GetNumBytesPerVertex(clone_mesh);

    if (options & D3DXMESH_VB_SHARE) {
        IDirect3DVertexBuffer9_AddRef(This->vertex_buffer);
        /* FIXME: refactor to avoid creating a new vertex buffer */
        IDirect3DVertexBuffer9_Release(cloned_this->vertex_buffer);
        cloned_this->vertex_buffer = This->vertex_buffer;
    } else {
        hr = iface->lpVtbl->LockVertexBuffer(iface, D3DLOCK_READONLY, &data_in);
        if (FAILED(hr)) goto error;
        hr = clone_mesh->lpVtbl->LockVertexBuffer(clone_mesh, D3DLOCK_DISCARD, &data_out);
        if (FAILED(hr)) {
            iface->lpVtbl->UnlockVertexBuffer(iface);
            goto error;
        }
        memcpy(data_out, data_in, This->numvertices * vertex_size);
        clone_mesh->lpVtbl->UnlockVertexBuffer(clone_mesh);
        iface->lpVtbl->UnlockVertexBuffer(iface);
    }

    hr = iface->lpVtbl->LockIndexBuffer(iface, D3DLOCK_READONLY, &data_in);
    if (FAILED(hr)) goto error;
    hr = clone_mesh->lpVtbl->LockIndexBuffer(clone_mesh, D3DLOCK_DISCARD, &data_out);
    if (FAILED(hr)) {
        iface->lpVtbl->UnlockIndexBuffer(iface);
        goto error;
    }
    if ((options ^ This->options) & D3DXMESH_32BIT) {
        if (options & D3DXMESH_32BIT) {
            for (i = 0; i < This->numfaces * 3; i++)
                ((DWORD*)data_out)[i] = ((WORD*)data_in)[i];
        } else {
            for (i = 0; i < This->numfaces * 3; i++)
                ((WORD*)data_out)[i] = ((DWORD*)data_in)[i];
        }
    } else {
        memcpy(data_out, data_in, This->numfaces * 3 * (options & D3DXMESH_32BIT ? 4 : 2));
    }
    clone_mesh->lpVtbl->UnlockIndexBuffer(clone_mesh);
    iface->lpVtbl->UnlockIndexBuffer(iface);

    memcpy(cloned_this->attrib_buffer, This->attrib_buffer, This->numfaces * sizeof(*This->attrib_buffer));

    if (This->attrib_table_size)
    {
        cloned_this->attrib_table_size = This->attrib_table_size;
        cloned_this->attrib_table = HeapAlloc(GetProcessHeap(), 0, This->attrib_table_size * sizeof(*This->attrib_table));
        if (!cloned_this->attrib_table) {
            hr = E_OUTOFMEMORY;
            goto error;
        }
        memcpy(cloned_this->attrib_table, This->attrib_table, This->attrib_table_size * sizeof(*This->attrib_table));
    }

    *clone_mesh_out = clone_mesh;

    return D3D_OK;
error:
    IUnknown_Release(clone_mesh);
    return hr;
}

static HRESULT WINAPI ID3DXMeshImpl_GetVertexBuffer(ID3DXMesh *iface, LPDIRECT3DVERTEXBUFFER9 *vertex_buffer)
{
    ID3DXMeshImpl *This = impl_from_ID3DXMesh(iface);

    TRACE("(%p)->(%p)\n", This, vertex_buffer);

    if (vertex_buffer == NULL) return D3DERR_INVALIDCALL;
    *vertex_buffer = This->vertex_buffer;
    IDirect3DVertexBuffer9_AddRef(This->vertex_buffer);

    return D3D_OK;
}

static HRESULT WINAPI ID3DXMeshImpl_GetIndexBuffer(ID3DXMesh *iface, LPDIRECT3DINDEXBUFFER9 *index_buffer)
{
    ID3DXMeshImpl *This = impl_from_ID3DXMesh(iface);

    TRACE("(%p)->(%p)\n", This, index_buffer);

    if (index_buffer == NULL) return D3DERR_INVALIDCALL;
    *index_buffer = This->index_buffer;
    IDirect3DIndexBuffer9_AddRef(This->index_buffer);

    return D3D_OK;
}

static HRESULT WINAPI ID3DXMeshImpl_LockVertexBuffer(ID3DXMesh *iface, DWORD flags, LPVOID *data)
{
    ID3DXMeshImpl *This = impl_from_ID3DXMesh(iface);

    TRACE("(%p)->(%u,%p)\n", This, flags, data);

    return IDirect3DVertexBuffer9_Lock(This->vertex_buffer, 0, 0, data, flags);
}

static HRESULT WINAPI ID3DXMeshImpl_UnlockVertexBuffer(ID3DXMesh *iface)
{
    ID3DXMeshImpl *This = impl_from_ID3DXMesh(iface);

    TRACE("(%p)\n", This);

    return IDirect3DVertexBuffer9_Unlock(This->vertex_buffer);
}

static HRESULT WINAPI ID3DXMeshImpl_LockIndexBuffer(ID3DXMesh *iface, DWORD flags, LPVOID *data)
{
    ID3DXMeshImpl *This = impl_from_ID3DXMesh(iface);

    TRACE("(%p)->(%u,%p)\n", This, flags, data);

    return IDirect3DIndexBuffer9_Lock(This->index_buffer, 0, 0, data, flags);
}

static HRESULT WINAPI ID3DXMeshImpl_UnlockIndexBuffer(ID3DXMesh *iface)
{
    ID3DXMeshImpl *This = impl_from_ID3DXMesh(iface);

    TRACE("(%p)\n", This);

    return IDirect3DIndexBuffer9_Unlock(This->index_buffer);
}

static HRESULT WINAPI ID3DXMeshImpl_GetAttributeTable(ID3DXMesh *iface, D3DXATTRIBUTERANGE *attrib_table, DWORD *attrib_table_size)
{
    ID3DXMeshImpl *This = impl_from_ID3DXMesh(iface);

    TRACE("(%p)->(%p,%p)\n", This, attrib_table, attrib_table_size);

    if (attrib_table_size)
        *attrib_table_size = This->attrib_table_size;

    if (attrib_table)
        CopyMemory(attrib_table, This->attrib_table, This->attrib_table_size * sizeof(*attrib_table));

    return D3D_OK;
}

static HRESULT WINAPI ID3DXMeshImpl_ConvertPointRepsToAdjacency(ID3DXMesh *iface, CONST DWORD *point_reps, DWORD *adjacency)
{
    ID3DXMeshImpl *This = impl_from_ID3DXMesh(iface);

    FIXME("(%p)->(%p,%p): stub\n", This, point_reps, adjacency);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXMeshImpl_ConvertAdjacencyToPointReps(ID3DXMesh *iface, CONST DWORD *adjacency, DWORD *point_reps)
{
    ID3DXMeshImpl *This = impl_from_ID3DXMesh(iface);

    FIXME("(%p)->(%p,%p): stub\n", This, adjacency, point_reps);

    return E_NOTIMPL;
}

struct vertex_metadata {
  float key;
  DWORD vertex_index;
  DWORD first_shared_index;
};

static int compare_vertex_keys(const void *a, const void *b)
{
    const struct vertex_metadata *left = a;
    const struct vertex_metadata *right = b;
    if (left->key == right->key)
        return 0;
    return left->key < right->key ? -1 : 1;
}

static HRESULT WINAPI ID3DXMeshImpl_GenerateAdjacency(ID3DXMesh *iface, FLOAT epsilon, DWORD *adjacency)
{
    ID3DXMeshImpl *This = impl_from_ID3DXMesh(iface);
    HRESULT hr;
    BYTE *vertices = NULL;
    const DWORD *indices = NULL;
    DWORD vertex_size;
    DWORD buffer_size;
    /* sort the vertices by (x + y + z) to quickly find coincident vertices */
    struct vertex_metadata *sorted_vertices;
    /* shared_indices links together identical indices in the index buffer so
     * that adjacency checks can be limited to faces sharing a vertex */
    DWORD *shared_indices = NULL;
    const FLOAT epsilon_sq = epsilon * epsilon;
    int i;

    TRACE("(%p)->(%f,%p)\n", This, epsilon, adjacency);

    if (!adjacency)
        return D3DERR_INVALIDCALL;

    buffer_size = This->numfaces * 3 * sizeof(*shared_indices) + This->numvertices * sizeof(*sorted_vertices);
    if (!(This->options & D3DXMESH_32BIT))
        buffer_size += This->numfaces * 3 * sizeof(*indices);
    shared_indices = HeapAlloc(GetProcessHeap(), 0, buffer_size);
    if (!shared_indices)
        return E_OUTOFMEMORY;
    sorted_vertices = (struct vertex_metadata*)(shared_indices + This->numfaces * 3);

    hr = iface->lpVtbl->LockVertexBuffer(iface, D3DLOCK_READONLY, (void**)&vertices);
    if (FAILED(hr)) goto cleanup;
    hr = iface->lpVtbl->LockIndexBuffer(iface, D3DLOCK_READONLY, (void**)&indices);
    if (FAILED(hr)) goto cleanup;

    if (!(This->options & D3DXMESH_32BIT)) {
        const WORD *word_indices = (const WORD*)indices;
        DWORD *dword_indices = (DWORD*)(sorted_vertices + This->numvertices);
        indices = dword_indices;
        for (i = 0; i < This->numfaces * 3; i++)
            *dword_indices++ = *word_indices++;
    }

    vertex_size = iface->lpVtbl->GetNumBytesPerVertex(iface);
    for (i = 0; i < This->numvertices; i++) {
        D3DXVECTOR3 *vertex = (D3DXVECTOR3*)(vertices + vertex_size * i);
        sorted_vertices[i].first_shared_index = -1;
        sorted_vertices[i].key = vertex->x + vertex->y + vertex->z;
        sorted_vertices[i].vertex_index = i;
    }
    for (i = 0; i < This->numfaces * 3; i++) {
        DWORD *first_shared_index = &sorted_vertices[indices[i]].first_shared_index;
        shared_indices[i] = *first_shared_index;
        *first_shared_index = i;
        adjacency[i] = -1;
    }
    qsort(sorted_vertices, This->numvertices, sizeof(*sorted_vertices), compare_vertex_keys);

    for (i = 0; i < This->numvertices; i++) {
        struct vertex_metadata *sorted_vertex_a = &sorted_vertices[i];
        D3DXVECTOR3 *vertex_a = (D3DXVECTOR3*)(vertices + sorted_vertex_a->vertex_index * vertex_size);
        DWORD shared_index_a = sorted_vertex_a->first_shared_index;

        while (shared_index_a != -1) {
            int j = i;
            DWORD shared_index_b = shared_indices[shared_index_a];
            struct vertex_metadata *sorted_vertex_b = sorted_vertex_a;

            while (TRUE) {
                while (shared_index_b != -1) {
                    /* faces are adjacent if they have another coincident vertex */
                    DWORD base_a = (shared_index_a / 3) * 3;
                    DWORD base_b = (shared_index_b / 3) * 3;
                    BOOL adjacent = FALSE;
                    int k;

                    for (k = 0; k < 3; k++) {
                        if (adjacency[base_b + k] == shared_index_a / 3) {
                            adjacent = TRUE;
                            break;
                        }
                    }
                    if (!adjacent) {
                        for (k = 1; k <= 2; k++) {
                            DWORD vertex_index_a = base_a + (shared_index_a + k) % 3;
                            DWORD vertex_index_b = base_b + (shared_index_b + (3 - k)) % 3;
                            adjacent = indices[vertex_index_a] == indices[vertex_index_b];
                            if (!adjacent && epsilon >= 0.0f) {
                                D3DXVECTOR3 delta = {0.0f, 0.0f, 0.0f};
                                FLOAT length_sq;

                                D3DXVec3Subtract(&delta,
                                                 (D3DXVECTOR3*)(vertices + indices[vertex_index_a] * vertex_size),
                                                 (D3DXVECTOR3*)(vertices + indices[vertex_index_b] * vertex_size));
                                length_sq = D3DXVec3LengthSq(&delta);
                                adjacent = epsilon == 0.0f ? length_sq == 0.0f : length_sq < epsilon_sq;
                            }
                            if (adjacent) {
                                DWORD adj_a = base_a + 2 - (vertex_index_a + shared_index_a + 1) % 3;
                                DWORD adj_b = base_b + 2 - (vertex_index_b + shared_index_b + 1) % 3;
                                if (adjacency[adj_a] == -1 && adjacency[adj_b] == -1) {
                                    adjacency[adj_a] = base_b / 3;
                                    adjacency[adj_b] = base_a / 3;
                                    break;
                                }
                            }
                        }
                    }

                    shared_index_b = shared_indices[shared_index_b];
                }
                while (++j < This->numvertices) {
                    D3DXVECTOR3 *vertex_b;

                    sorted_vertex_b++;
                    if (sorted_vertex_b->key - sorted_vertex_a->key > epsilon * 3.0f) {
                        /* no more coincident vertices to try */
                        j = This->numvertices;
                        break;
                    }
                    /* check for coincidence */
                    vertex_b = (D3DXVECTOR3*)(vertices + sorted_vertex_b->vertex_index * vertex_size);
                    if (fabsf(vertex_a->x - vertex_b->x) <= epsilon &&
                        fabsf(vertex_a->y - vertex_b->y) <= epsilon &&
                        fabsf(vertex_a->z - vertex_b->z) <= epsilon)
                    {
                        break;
                    }
                }
                if (j >= This->numvertices)
                    break;
                shared_index_b = sorted_vertex_b->first_shared_index;
            }

            sorted_vertex_a->first_shared_index = shared_indices[sorted_vertex_a->first_shared_index];
            shared_index_a = sorted_vertex_a->first_shared_index;
        }
    }

    hr = D3D_OK;
cleanup:
    if (indices) iface->lpVtbl->UnlockIndexBuffer(iface);
    if (vertices) iface->lpVtbl->UnlockVertexBuffer(iface);
    HeapFree(GetProcessHeap(), 0, shared_indices);
    return hr;
}

static HRESULT WINAPI ID3DXMeshImpl_UpdateSemantics(ID3DXMesh *iface, D3DVERTEXELEMENT9 declaration[MAX_FVF_DECL_SIZE])
{
    ID3DXMeshImpl *This = impl_from_ID3DXMesh(iface);

    FIXME("(%p)->(%p): stub\n", This, declaration);

    return E_NOTIMPL;
}

/*** ID3DXMesh ***/
static HRESULT WINAPI ID3DXMeshImpl_LockAttributeBuffer(ID3DXMesh *iface, DWORD flags, DWORD **data)
{
    ID3DXMeshImpl *This = impl_from_ID3DXMesh(iface);

    TRACE("(%p)->(%u,%p)\n", This, flags, data);

    InterlockedIncrement(&This->attrib_buffer_lock_count);

    if (!(flags & D3DLOCK_READONLY)) {
        D3DXATTRIBUTERANGE *attrib_table = This->attrib_table;
        This->attrib_table_size = 0;
        This->attrib_table = NULL;
        HeapFree(GetProcessHeap(), 0, attrib_table);
    }

    *data = This->attrib_buffer;

    return D3D_OK;
}

static HRESULT WINAPI ID3DXMeshImpl_UnlockAttributeBuffer(ID3DXMesh *iface)
{
    ID3DXMeshImpl *This = impl_from_ID3DXMesh(iface);
    int lock_count;

    TRACE("(%p)\n", This);

    lock_count = InterlockedDecrement(&This->attrib_buffer_lock_count);

    if (lock_count < 0) {
        InterlockedIncrement(&This->attrib_buffer_lock_count);
        return D3DERR_INVALIDCALL;
    }

    return D3D_OK;
}

static HRESULT WINAPI ID3DXMeshImpl_Optimize(ID3DXMesh *iface, DWORD flags, CONST DWORD *adjacency_in, DWORD *adjacency_out,
                                             DWORD *face_remap, LPD3DXBUFFER *vertex_remap, LPD3DXMESH *opt_mesh)
{
    ID3DXMeshImpl *This = impl_from_ID3DXMesh(iface);

    FIXME("(%p)->(%u,%p,%p,%p,%p,%p): stub\n", This, flags, adjacency_in, adjacency_out, face_remap, vertex_remap, opt_mesh);

    return E_NOTIMPL;
}

/* Creates a vertex_remap that removes unused vertices.
 * Indices are updated according to the vertex_remap. */
static HRESULT compact_mesh(ID3DXMeshImpl *This, DWORD *indices, DWORD *new_num_vertices, ID3DXBuffer **vertex_remap)
{
    HRESULT hr;
    DWORD *vertex_remap_ptr;
    DWORD num_used_vertices;
    DWORD i;

    hr = D3DXCreateBuffer(This->numvertices * sizeof(DWORD), vertex_remap);
    if (FAILED(hr)) return hr;
    vertex_remap_ptr = ID3DXBuffer_GetBufferPointer(*vertex_remap);

    for (i = 0; i < This->numfaces * 3; i++)
        vertex_remap_ptr[indices[i]] = 1;

    /* create old->new vertex mapping */
    num_used_vertices = 0;
    for (i = 0; i < This->numvertices; i++) {
        if (vertex_remap_ptr[i])
            vertex_remap_ptr[i] = num_used_vertices++;
        else
            vertex_remap_ptr[i] = -1;
    }
    /* convert indices */
    for (i = 0; i < This->numfaces * 3; i++)
        indices[i] = vertex_remap_ptr[indices[i]];

    /* create new->old vertex mapping */
    num_used_vertices = 0;
    for (i = 0; i < This->numvertices; i++) {
        if (vertex_remap_ptr[i] != -1)
            vertex_remap_ptr[num_used_vertices++] = i;
    }
    for (i = num_used_vertices; i < This->numvertices; i++)
        vertex_remap_ptr[i] = -1;

    *new_num_vertices = num_used_vertices;

    return D3D_OK;
}

/* count the number of unique attribute values in a sorted attribute buffer */
static DWORD count_attributes(const DWORD *attrib_buffer, DWORD numfaces)
{
    DWORD last_attribute = attrib_buffer[0];
    DWORD attrib_table_size = 1;
    DWORD i;
    for (i = 1; i < numfaces; i++) {
        if (attrib_buffer[i] != last_attribute) {
            last_attribute = attrib_buffer[i];
            attrib_table_size++;
        }
    }
    return attrib_table_size;
}

static void fill_attribute_table(DWORD *attrib_buffer, DWORD numfaces, void *indices,
                                 BOOL is_32bit_indices, D3DXATTRIBUTERANGE *attrib_table)
{
    DWORD attrib_table_size = 0;
    DWORD last_attribute = attrib_buffer[0];
    DWORD min_vertex, max_vertex;
    DWORD i;

    attrib_table[0].AttribId = last_attribute;
    attrib_table[0].FaceStart = 0;
    min_vertex = (DWORD)-1;
    max_vertex = 0;
    for (i = 0; i < numfaces; i++) {
        DWORD j;

        if (attrib_buffer[i] != last_attribute) {
            last_attribute = attrib_buffer[i];
            attrib_table[attrib_table_size].FaceCount = i - attrib_table[attrib_table_size].FaceStart;
            attrib_table[attrib_table_size].VertexStart = min_vertex;
            attrib_table[attrib_table_size].VertexCount = max_vertex - min_vertex + 1;
            attrib_table_size++;
            attrib_table[attrib_table_size].AttribId = attrib_buffer[i];
            attrib_table[attrib_table_size].FaceStart = i;
            min_vertex = (DWORD)-1;
            max_vertex = 0;
        }
        for (j = 0; j < 3; j++) {
            DWORD vertex_index = is_32bit_indices ? ((DWORD*)indices)[i * 3 + j] : ((WORD*)indices)[i * 3 + j];
            if (vertex_index < min_vertex)
                min_vertex = vertex_index;
            if (vertex_index > max_vertex)
                max_vertex = vertex_index;
        }
    }
    attrib_table[attrib_table_size].FaceCount = i - attrib_table[attrib_table_size].FaceStart;
    attrib_table[attrib_table_size].VertexStart = min_vertex;
    attrib_table[attrib_table_size].VertexCount = max_vertex - min_vertex + 1;
    attrib_table_size++;
}

static int attrib_entry_compare(const DWORD **a, const DWORD **b)
{
    const DWORD *ptr_a = *a;
    const DWORD *ptr_b = *b;
    int delta = *ptr_a - *ptr_b;

    if (delta)
        return delta;

    delta = ptr_a - ptr_b; /* for stable sort */
    return delta;
}

/* Create face_remap, a new attribute buffer for attribute sort optimization. */
static HRESULT remap_faces_for_attrsort(ID3DXMeshImpl *This, const DWORD *indices,
        const DWORD *attrib_buffer, DWORD **sorted_attrib_buffer, DWORD **face_remap)
{
    const DWORD **sorted_attrib_ptr_buffer = NULL;
    DWORD i;

    *face_remap = HeapAlloc(GetProcessHeap(), 0, This->numfaces * sizeof(**face_remap));
    sorted_attrib_ptr_buffer = HeapAlloc(GetProcessHeap(), 0, This->numfaces * sizeof(*sorted_attrib_ptr_buffer));
    if (!*face_remap || !sorted_attrib_ptr_buffer) {
        HeapFree(GetProcessHeap(), 0, sorted_attrib_ptr_buffer);
        return E_OUTOFMEMORY;
    }
    for (i = 0; i < This->numfaces; i++)
        sorted_attrib_ptr_buffer[i] = &attrib_buffer[i];
    qsort(sorted_attrib_ptr_buffer, This->numfaces, sizeof(*sorted_attrib_ptr_buffer),
         (int(*)(const void *, const void *))attrib_entry_compare);

    for (i = 0; i < This->numfaces; i++)
    {
        DWORD old_face = sorted_attrib_ptr_buffer[i] - attrib_buffer;
        (*face_remap)[old_face] = i;
    }

    /* overwrite sorted_attrib_ptr_buffer with the values themselves */
    *sorted_attrib_buffer = (DWORD*)sorted_attrib_ptr_buffer;
    for (i = 0; i < This->numfaces; i++)
        (*sorted_attrib_buffer)[(*face_remap)[i]] = attrib_buffer[i];

    return D3D_OK;
}

static HRESULT WINAPI ID3DXMeshImpl_OptimizeInplace(ID3DXMesh *iface, DWORD flags, CONST DWORD *adjacency_in, DWORD *adjacency_out,
                                                    DWORD *face_remap_out, LPD3DXBUFFER *vertex_remap_out)
{
    ID3DXMeshImpl *This = impl_from_ID3DXMesh(iface);
    void *indices = NULL;
    DWORD *attrib_buffer = NULL;
    HRESULT hr;
    ID3DXBuffer *vertex_remap = NULL;
    DWORD *face_remap = NULL; /* old -> new mapping */
    DWORD *dword_indices = NULL;
    DWORD new_num_vertices = 0;
    DWORD new_num_alloc_vertices = 0;
    IDirect3DVertexBuffer9 *vertex_buffer = NULL;
    DWORD *sorted_attrib_buffer = NULL;
    DWORD i;

    TRACE("(%p)->(%x,%p,%p,%p,%p)\n", This, flags, adjacency_in, adjacency_out, face_remap_out, vertex_remap_out);

    if (!flags)
        return D3DERR_INVALIDCALL;
    if (!adjacency_in && (flags & (D3DXMESHOPT_VERTEXCACHE | D3DXMESHOPT_STRIPREORDER)))
        return D3DERR_INVALIDCALL;
    if ((flags & (D3DXMESHOPT_VERTEXCACHE | D3DXMESHOPT_STRIPREORDER)) == (D3DXMESHOPT_VERTEXCACHE | D3DXMESHOPT_STRIPREORDER))
        return D3DERR_INVALIDCALL;

    if (flags & (D3DXMESHOPT_VERTEXCACHE | D3DXMESHOPT_STRIPREORDER))
    {
        if (flags & D3DXMESHOPT_VERTEXCACHE)
            FIXME("D3DXMESHOPT_VERTEXCACHE not implemented.\n");
        if (flags & D3DXMESHOPT_STRIPREORDER)
            FIXME("D3DXMESHOPT_STRIPREORDER not implemented.\n");
        return E_NOTIMPL;
    }

    hr = iface->lpVtbl->LockIndexBuffer(iface, 0, (void**)&indices);
    if (FAILED(hr)) goto cleanup;

    dword_indices = HeapAlloc(GetProcessHeap(), 0, This->numfaces * 3 * sizeof(DWORD));
    if (!dword_indices) return E_OUTOFMEMORY;
    if (This->options & D3DXMESH_32BIT) {
        memcpy(dword_indices, indices, This->numfaces * 3 * sizeof(DWORD));
    } else {
        WORD *word_indices = indices;
        for (i = 0; i < This->numfaces * 3; i++)
            dword_indices[i] = *word_indices++;
    }

    if ((flags & (D3DXMESHOPT_COMPACT | D3DXMESHOPT_IGNOREVERTS | D3DXMESHOPT_ATTRSORT)) == D3DXMESHOPT_COMPACT)
    {
        new_num_alloc_vertices = This->numvertices;
        hr = compact_mesh(This, dword_indices, &new_num_vertices, &vertex_remap);
        if (FAILED(hr)) goto cleanup;
    } else if (flags & D3DXMESHOPT_ATTRSORT) {
        if (!(flags & D3DXMESHOPT_IGNOREVERTS))
        {
            FIXME("D3DXMESHOPT_ATTRSORT vertex reordering not implemented.\n");
            hr = E_NOTIMPL;
            goto cleanup;
        }

        hr = iface->lpVtbl->LockAttributeBuffer(iface, 0, &attrib_buffer);
        if (FAILED(hr)) goto cleanup;

        hr = remap_faces_for_attrsort(This, dword_indices, attrib_buffer, &sorted_attrib_buffer, &face_remap);
        if (FAILED(hr)) goto cleanup;
    }

    if (vertex_remap)
    {
        /* reorder the vertices using vertex_remap */
        D3DVERTEXBUFFER_DESC vertex_desc;
        DWORD *vertex_remap_ptr = ID3DXBuffer_GetBufferPointer(vertex_remap);
        DWORD vertex_size = iface->lpVtbl->GetNumBytesPerVertex(iface);
        BYTE *orig_vertices;
        BYTE *new_vertices;

        hr = IDirect3DVertexBuffer9_GetDesc(This->vertex_buffer, &vertex_desc);
        if (FAILED(hr)) goto cleanup;

        hr = IDirect3DDevice9_CreateVertexBuffer(This->device, new_num_alloc_vertices * vertex_size,
                vertex_desc.Usage, This->fvf, vertex_desc.Pool, &vertex_buffer, NULL);
        if (FAILED(hr)) goto cleanup;

        hr = IDirect3DVertexBuffer9_Lock(This->vertex_buffer, 0, 0, (void**)&orig_vertices, D3DLOCK_READONLY);
        if (FAILED(hr)) goto cleanup;

        hr = IDirect3DVertexBuffer9_Lock(vertex_buffer, 0, 0, (void**)&new_vertices, D3DLOCK_DISCARD);
        if (FAILED(hr)) {
            IDirect3DVertexBuffer9_Unlock(This->vertex_buffer);
            goto cleanup;
        }

        for (i = 0; i < new_num_vertices; i++)
            memcpy(new_vertices + i * vertex_size, orig_vertices + vertex_remap_ptr[i] * vertex_size, vertex_size);

        IDirect3DVertexBuffer9_Unlock(This->vertex_buffer);
        IDirect3DVertexBuffer9_Unlock(vertex_buffer);
    } else if (vertex_remap_out) {
        DWORD *vertex_remap_ptr;

        hr = D3DXCreateBuffer(This->numvertices * sizeof(DWORD), &vertex_remap);
        if (FAILED(hr)) goto cleanup;
        vertex_remap_ptr = ID3DXBuffer_GetBufferPointer(vertex_remap);
        for (i = 0; i < This->numvertices; i++)
            *vertex_remap_ptr++ = i;
    }

    if (flags & D3DXMESHOPT_ATTRSORT)
    {
        D3DXATTRIBUTERANGE *attrib_table;
        DWORD attrib_table_size;

        attrib_table_size = count_attributes(sorted_attrib_buffer, This->numfaces);
        attrib_table = HeapAlloc(GetProcessHeap(), 0, attrib_table_size * sizeof(*attrib_table));
        if (!attrib_table) {
            hr = E_OUTOFMEMORY;
            goto cleanup;
        }

        memcpy(attrib_buffer, sorted_attrib_buffer, This->numfaces * sizeof(*attrib_buffer));

        /* reorder the indices using face_remap */
        if (This->options & D3DXMESH_32BIT) {
            for (i = 0; i < This->numfaces; i++)
                memcpy((DWORD*)indices + face_remap[i] * 3, dword_indices + i * 3, 3 * sizeof(DWORD));
        } else {
            WORD *word_indices = indices;
            for (i = 0; i < This->numfaces; i++) {
                DWORD new_pos = face_remap[i] * 3;
                DWORD old_pos = i * 3;
                word_indices[new_pos++] = dword_indices[old_pos++];
                word_indices[new_pos++] = dword_indices[old_pos++];
                word_indices[new_pos] = dword_indices[old_pos];
            }
        }

        fill_attribute_table(attrib_buffer, This->numfaces, indices,
                             This->options & D3DXMESH_32BIT, attrib_table);

        HeapFree(GetProcessHeap(), 0, This->attrib_table);
        This->attrib_table = attrib_table;
        This->attrib_table_size = attrib_table_size;
    } else {
        if (This->options & D3DXMESH_32BIT) {
            memcpy(indices, dword_indices, This->numfaces * 3 * sizeof(DWORD));
        } else {
            WORD *word_indices = indices;
            for (i = 0; i < This->numfaces * 3; i++)
                *word_indices++ = dword_indices[i];
        }
    }

    if (adjacency_out) {
        if (face_remap) {
            for (i = 0; i < This->numfaces; i++) {
                DWORD old_pos = i * 3;
                DWORD new_pos = face_remap[i] * 3;
                adjacency_out[new_pos++] = face_remap[adjacency_in[old_pos++]];
                adjacency_out[new_pos++] = face_remap[adjacency_in[old_pos++]];
                adjacency_out[new_pos++] = face_remap[adjacency_in[old_pos++]];
            }
        } else {
            memcpy(adjacency_out, adjacency_in, This->numfaces * 3 * sizeof(*adjacency_out));
        }
    }
    if (face_remap_out) {
        if (face_remap) {
            for (i = 0; i < This->numfaces; i++)
                face_remap_out[face_remap[i]] = i;
        } else {
            for (i = 0; i < This->numfaces; i++)
                face_remap_out[i] = i;
        }
    }
    if (vertex_remap_out)
        *vertex_remap_out = vertex_remap;
    vertex_remap = NULL;

    if (vertex_buffer) {
        IDirect3DVertexBuffer9_Release(This->vertex_buffer);
        This->vertex_buffer = vertex_buffer;
        vertex_buffer = NULL;
        This->numvertices = new_num_vertices;
    }

    hr = D3D_OK;
cleanup:
    HeapFree(GetProcessHeap(), 0, sorted_attrib_buffer);
    HeapFree(GetProcessHeap(), 0, face_remap);
    HeapFree(GetProcessHeap(), 0, dword_indices);
    if (vertex_remap) ID3DXBuffer_Release(vertex_remap);
    if (vertex_buffer) IDirect3DVertexBuffer9_Release(vertex_buffer);
    if (attrib_buffer) iface->lpVtbl->UnlockAttributeBuffer(iface);
    if (indices) iface->lpVtbl->UnlockIndexBuffer(iface);
    return hr;
}

static HRESULT WINAPI ID3DXMeshImpl_SetAttributeTable(ID3DXMesh *iface, CONST D3DXATTRIBUTERANGE *attrib_table, DWORD attrib_table_size)
{
    ID3DXMeshImpl *This = impl_from_ID3DXMesh(iface);
    D3DXATTRIBUTERANGE *new_table = NULL;

    TRACE("(%p)->(%p,%u)\n", This, attrib_table, attrib_table_size);

    if (attrib_table_size) {
        size_t size = attrib_table_size * sizeof(*attrib_table);

        new_table = HeapAlloc(GetProcessHeap(), 0, size);
        if (!new_table)
            return E_OUTOFMEMORY;

        CopyMemory(new_table, attrib_table, size);
    } else if (attrib_table) {
        return D3DERR_INVALIDCALL;
    }
    HeapFree(GetProcessHeap(), 0, This->attrib_table);
    This->attrib_table = new_table;
    This->attrib_table_size = attrib_table_size;

    return D3D_OK;
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
    DWORD *attrib_buffer;
    ID3DXMeshImpl *object;
    DWORD index_usage = 0;
    D3DPOOL index_pool = D3DPOOL_DEFAULT;
    D3DFORMAT index_format = D3DFMT_INDEX16;
    DWORD vertex_usage = 0;
    D3DPOOL vertex_pool = D3DPOOL_DEFAULT;
    int i;

    TRACE("(%d, %d, %x, %p, %p, %p)\n", numfaces, numvertices, options, declaration, device, mesh);

    if (numfaces == 0 || numvertices == 0 || declaration == NULL || device == NULL || mesh == NULL ||
        /* D3DXMESH_VB_SHARE is for cloning, and D3DXMESH_USEHWONLY is for ConvertToBlendedMesh */
        (options & (D3DXMESH_VB_SHARE | D3DXMESH_USEHWONLY | 0xfffe0000)))
    {
        return D3DERR_INVALIDCALL;
    }
    for (i = 0; declaration[i].Stream != 0xff; i++)
        if (declaration[i].Stream != 0)
            return D3DERR_INVALIDCALL;

    if (options & D3DXMESH_32BIT)
        index_format = D3DFMT_INDEX32;

    if (options & D3DXMESH_DONOTCLIP) {
        index_usage |= D3DUSAGE_DONOTCLIP;
        vertex_usage |= D3DUSAGE_DONOTCLIP;
    }
    if (options & D3DXMESH_POINTS) {
        index_usage |= D3DUSAGE_POINTS;
        vertex_usage |= D3DUSAGE_POINTS;
    }
    if (options & D3DXMESH_RTPATCHES) {
        index_usage |= D3DUSAGE_RTPATCHES;
        vertex_usage |= D3DUSAGE_RTPATCHES;
    }
    if (options & D3DXMESH_NPATCHES) {
        index_usage |= D3DUSAGE_NPATCHES;
        vertex_usage |= D3DUSAGE_NPATCHES;
    }

    if (options & D3DXMESH_VB_SYSTEMMEM)
        vertex_pool = D3DPOOL_SYSTEMMEM;
    else if (options & D3DXMESH_VB_MANAGED)
        vertex_pool = D3DPOOL_MANAGED;

    if (options & D3DXMESH_VB_WRITEONLY)
        vertex_usage |= D3DUSAGE_WRITEONLY;
    if (options & D3DXMESH_VB_DYNAMIC)
        vertex_usage |= D3DUSAGE_DYNAMIC;
    if (options & D3DXMESH_VB_SOFTWAREPROCESSING)
        vertex_usage |= D3DUSAGE_SOFTWAREPROCESSING;

    if (options & D3DXMESH_IB_SYSTEMMEM)
        index_pool = D3DPOOL_SYSTEMMEM;
    else if (options & D3DXMESH_IB_MANAGED)
        index_pool = D3DPOOL_MANAGED;

    if (options & D3DXMESH_IB_WRITEONLY)
        index_usage |= D3DUSAGE_WRITEONLY;
    if (options & D3DXMESH_IB_DYNAMIC)
        index_usage |= D3DUSAGE_DYNAMIC;
    if (options & D3DXMESH_IB_SOFTWAREPROCESSING)
        index_usage |= D3DUSAGE_SOFTWAREPROCESSING;

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
                                             vertex_usage,
                                             fvf,
                                             vertex_pool,
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
                                            numfaces * 3 * ((index_format == D3DFMT_INDEX16) ? 2 : 4),
                                            index_usage,
                                            index_format,
                                            index_pool,
                                            &index_buffer,
                                            NULL);
    if (FAILED(hr))
    {
        WARN("Unexpected return value %x from IDirect3DDevice9_CreateVertexBuffer.\n",hr);
        IDirect3DVertexBuffer9_Release(vertex_buffer);
        IDirect3DVertexDeclaration9_Release(vertex_declaration);
        return hr;
    }

    attrib_buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, numfaces * sizeof(*attrib_buffer));
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(ID3DXMeshImpl));
    if (object == NULL || attrib_buffer == NULL)
    {
        HeapFree(GetProcessHeap(), 0, attrib_buffer);
        IDirect3DIndexBuffer9_Release(index_buffer);
        IDirect3DVertexBuffer9_Release(vertex_buffer);
        IDirect3DVertexDeclaration9_Release(vertex_declaration);
        *mesh = NULL;
        return E_OUTOFMEMORY;
    }
    object->ID3DXMesh_iface.lpVtbl = &D3DXMesh_Vtbl;
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
    object->attrib_buffer = attrib_buffer;

    *mesh = &object->ID3DXMesh_iface;

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

HRESULT WINAPI D3DXCreateTextA(LPDIRECT3DDEVICE9 device,
                               HDC hdc, LPCSTR text,
                               FLOAT deviation, FLOAT extrusion,
                               LPD3DXMESH *mesh, LPD3DXBUFFER *adjacency,
                               LPGLYPHMETRICSFLOAT glyphmetrics)
{
    HRESULT hr;
    int len;
    LPWSTR textW;

    TRACE("(%p, %p, %s, %f, %f, %p, %p, %p)\n", device, hdc,
          debugstr_a(text), deviation, extrusion, mesh, adjacency, glyphmetrics);

    if (!text)
        return D3DERR_INVALIDCALL;

    len = MultiByteToWideChar(CP_ACP, 0, text, -1, NULL, 0);
    textW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, text, -1, textW, len);

    hr = D3DXCreateTextW(device, hdc, textW, deviation, extrusion,
                         mesh, adjacency, glyphmetrics);
    HeapFree(GetProcessHeap(), 0, textW);

    return hr;
}

enum pointtype {
    POINTTYPE_CURVE = 0,
    POINTTYPE_CORNER,
    POINTTYPE_CURVE_START,
    POINTTYPE_CURVE_END,
    POINTTYPE_CURVE_MIDDLE,
};

struct point2d
{
    D3DXVECTOR2 pos;
    enum pointtype corner;
};

struct dynamic_array
{
    int count, capacity;
    void *items;
};

/* is a dynamic_array */
struct outline
{
    int count, capacity;
    struct point2d *items;
};

/* is a dynamic_array */
struct outline_array
{
    int count, capacity;
    struct outline *items;
};

struct face_array
{
    int count;
    face *items;
};

struct point2d_index
{
    struct outline *outline;
    int vertex;
};

struct point2d_index_array
{
    int count;
    struct point2d_index *items;
};

struct glyphinfo
{
    struct outline_array outlines;
    struct face_array faces;
    struct point2d_index_array ordered_vertices;
    float offset_x;
};

/* is an dynamic_array */
struct word_array
{
    int count, capacity;
    WORD *items;
};

/* complex polygons are split into monotone polygons, which have
 * at most 2 intersections with the vertical sweep line */
struct triangulation
{
    struct word_array vertex_stack;
    BOOL last_on_top, merging;
};

/* is an dynamic_array */
struct triangulation_array
{
    int count, capacity;
    struct triangulation *items;

    struct glyphinfo *glyph;
};

static BOOL reserve(struct dynamic_array *array, int count, int itemsize)
{
    if (count > array->capacity) {
        void *new_buffer;
        int new_capacity;
        if (array->items && array->capacity) {
            new_capacity = max(array->capacity * 2, count);
            new_buffer = HeapReAlloc(GetProcessHeap(), 0, array->items, new_capacity * itemsize);
        } else {
            new_capacity = max(16, count);
            new_buffer = HeapAlloc(GetProcessHeap(), 0, new_capacity * itemsize);
        }
        if (!new_buffer)
            return FALSE;
        array->items = new_buffer;
        array->capacity = new_capacity;
    }
    return TRUE;
}

static struct point2d *add_points(struct outline *array, int num)
{
    struct point2d *item;

    if (!reserve((struct dynamic_array *)array, array->count + num, sizeof(array->items[0])))
        return NULL;

    item = &array->items[array->count];
    array->count += num;
    return item;
}

static struct outline *add_outline(struct outline_array *array)
{
    struct outline *item;

    if (!reserve((struct dynamic_array *)array, array->count + 1, sizeof(array->items[0])))
        return NULL;

    item = &array->items[array->count++];
    ZeroMemory(item, sizeof(*item));
    return item;
}

static inline face *add_face(struct face_array *array)
{
    return &array->items[array->count++];
}

static struct triangulation *add_triangulation(struct triangulation_array *array)
{
    struct triangulation *item;

    if (!reserve((struct dynamic_array *)array, array->count + 1, sizeof(array->items[0])))
        return NULL;

    item = &array->items[array->count++];
    ZeroMemory(item, sizeof(*item));
    return item;
}

static HRESULT add_vertex_index(struct word_array *array, WORD vertex_index)
{
    if (!reserve((struct dynamic_array *)array, array->count + 1, sizeof(array->items[0])))
        return E_OUTOFMEMORY;

    array->items[array->count++] = vertex_index;
    return S_OK;
}

/* assume fixed point numbers can be converted to float point in place */
C_ASSERT(sizeof(FIXED) == sizeof(float));
C_ASSERT(sizeof(POINTFX) == sizeof(D3DXVECTOR2));

static inline D3DXVECTOR2 *convert_fixed_to_float(POINTFX *pt, int count, float emsquare)
{
    D3DXVECTOR2 *ret = (D3DXVECTOR2*)pt;
    while (count--) {
        D3DXVECTOR2 *pt_flt = (D3DXVECTOR2*)pt;
        pt_flt->x = (pt->x.value + pt->x.fract / (float)0x10000) / emsquare;
        pt_flt->y = (pt->y.value + pt->y.fract / (float)0x10000) / emsquare;
        pt++;
    }
    return ret;
}

static HRESULT add_bezier_points(struct outline *outline, const D3DXVECTOR2 *p1,
                                 const D3DXVECTOR2 *p2, const D3DXVECTOR2 *p3,
                                 float max_deviation_sq)
{
    D3DXVECTOR2 split1 = {0, 0}, split2 = {0, 0}, middle, vec;
    float deviation_sq;

    D3DXVec2Scale(&split1, D3DXVec2Add(&split1, p1, p2), 0.5f);
    D3DXVec2Scale(&split2, D3DXVec2Add(&split2, p2, p3), 0.5f);
    D3DXVec2Scale(&middle, D3DXVec2Add(&middle, &split1, &split2), 0.5f);

    deviation_sq = D3DXVec2LengthSq(D3DXVec2Subtract(&vec, &middle, p2));
    if (deviation_sq < max_deviation_sq) {
        struct point2d *pt = add_points(outline, 1);
        if (!pt) return E_OUTOFMEMORY;
        pt->pos = *p2;
        pt->corner = POINTTYPE_CURVE;
        /* the end point is omitted because the end line merges into the next segment of
         * the split bezier curve, and the end of the split bezier curve is added outside
         * this recursive function. */
    } else {
        HRESULT hr = add_bezier_points(outline, p1, &split1, &middle, max_deviation_sq);
        if (hr != S_OK) return hr;
        hr = add_bezier_points(outline, &middle, &split2, p3, max_deviation_sq);
        if (hr != S_OK) return hr;
    }

    return S_OK;
}

static inline BOOL is_direction_similar(D3DXVECTOR2 *dir1, D3DXVECTOR2 *dir2, float cos_theta)
{
    /* dot product = cos(theta) */
    return D3DXVec2Dot(dir1, dir2) > cos_theta;
}

static inline D3DXVECTOR2 *unit_vec2(D3DXVECTOR2 *dir, const D3DXVECTOR2 *pt1, const D3DXVECTOR2 *pt2)
{
    return D3DXVec2Normalize(D3DXVec2Subtract(dir, pt2, pt1), dir);
}

struct cos_table
{
    float cos_half;
    float cos_45;
    float cos_90;
};

static BOOL attempt_line_merge(struct outline *outline,
                               int pt_index,
                               const D3DXVECTOR2 *nextpt,
                               BOOL to_curve,
                               const struct cos_table *table)
{
    D3DXVECTOR2 curdir, lastdir;
    struct point2d *prevpt, *pt;
    BOOL ret = FALSE;

    pt = &outline->items[pt_index];
    pt_index = (pt_index - 1 + outline->count) % outline->count;
    prevpt = &outline->items[pt_index];

    if (to_curve)
        pt->corner = pt->corner != POINTTYPE_CORNER ? POINTTYPE_CURVE_MIDDLE : POINTTYPE_CURVE_START;

    if (outline->count < 2)
        return FALSE;

    /* remove last point if the next line continues the last line */
    unit_vec2(&lastdir, &prevpt->pos, &pt->pos);
    unit_vec2(&curdir, &pt->pos, nextpt);
    if (is_direction_similar(&lastdir, &curdir, table->cos_half))
    {
        outline->count--;
        if (pt->corner == POINTTYPE_CURVE_END)
            prevpt->corner = pt->corner;
        if (prevpt->corner == POINTTYPE_CURVE_END && to_curve)
            prevpt->corner = POINTTYPE_CURVE_MIDDLE;
        pt = prevpt;

        ret = TRUE;
        if (outline->count < 2)
            return ret;

        pt_index = (pt_index - 1 + outline->count) % outline->count;
        prevpt = &outline->items[pt_index];
        unit_vec2(&lastdir, &prevpt->pos, &pt->pos);
        unit_vec2(&curdir, &pt->pos, nextpt);
    }
    return ret;
}

static HRESULT create_outline(struct glyphinfo *glyph, void *raw_outline, int datasize,
                              float max_deviation_sq, float emsquare, const struct cos_table *cos_table)
{
    TTPOLYGONHEADER *header = (TTPOLYGONHEADER *)raw_outline;

    while ((char *)header < (char *)raw_outline + datasize)
    {
        TTPOLYCURVE *curve = (TTPOLYCURVE *)(header + 1);
        struct point2d *lastpt, *pt;
        D3DXVECTOR2 lastdir;
        D3DXVECTOR2 *pt_flt;
        int j;
        struct outline *outline = add_outline(&glyph->outlines);

        if (!outline)
            return E_OUTOFMEMORY;

        pt = add_points(outline, 1);
        if (!pt)
            return E_OUTOFMEMORY;
        pt_flt = convert_fixed_to_float(&header->pfxStart, 1, emsquare);
        pt->pos = *pt_flt;
        pt->corner = POINTTYPE_CORNER;

        if (header->dwType != TT_POLYGON_TYPE)
            FIXME("Unknown header type %d\n", header->dwType);

        while ((char *)curve < (char *)header + header->cb)
        {
            D3DXVECTOR2 bezier_start = outline->items[outline->count - 1].pos;
            BOOL to_curve = curve->wType != TT_PRIM_LINE && curve->cpfx > 1;

            if (!curve->cpfx) {
                curve = (TTPOLYCURVE *)&curve->apfx[curve->cpfx];
                continue;
            }

            pt_flt = convert_fixed_to_float(curve->apfx, curve->cpfx, emsquare);

            attempt_line_merge(outline, outline->count - 1, &pt_flt[0], to_curve, cos_table);

            if (to_curve)
            {
                HRESULT hr;
                int count = curve->cpfx;
                j = 0;

                while (count > 2)
                {
                    D3DXVECTOR2 bezier_end;

                    D3DXVec2Scale(&bezier_end, D3DXVec2Add(&bezier_end, &pt_flt[j], &pt_flt[j+1]), 0.5f);
                    hr = add_bezier_points(outline, &bezier_start, &pt_flt[j], &bezier_end, max_deviation_sq);
                    if (hr != S_OK)
                        return hr;
                    bezier_start = bezier_end;
                    count--;
                    j++;
                }
                hr = add_bezier_points(outline, &bezier_start, &pt_flt[j], &pt_flt[j+1], max_deviation_sq);
                if (hr != S_OK)
                    return hr;

                pt = add_points(outline, 1);
                if (!pt)
                    return E_OUTOFMEMORY;
                j++;
                pt->pos = pt_flt[j];
                pt->corner = POINTTYPE_CURVE_END;
            } else {
                pt = add_points(outline, curve->cpfx);
                if (!pt)
                    return E_OUTOFMEMORY;
                for (j = 0; j < curve->cpfx; j++)
                {
                    pt->pos = pt_flt[j];
                    pt->corner = POINTTYPE_CORNER;
                    pt++;
                }
            }

            curve = (TTPOLYCURVE *)&curve->apfx[curve->cpfx];
        }

        /* remove last point if the next line continues the last line */
        if (outline->count >= 3) {
            BOOL to_curve;

            lastpt = &outline->items[outline->count - 1];
            pt = &outline->items[0];
            if (pt->pos.x == lastpt->pos.x && pt->pos.y == lastpt->pos.y) {
                if (lastpt->corner == POINTTYPE_CURVE_END)
                {
                    if (pt->corner == POINTTYPE_CURVE_START)
                        pt->corner = POINTTYPE_CURVE_MIDDLE;
                    else
                        pt->corner = POINTTYPE_CURVE_END;
                }
                outline->count--;
                lastpt = &outline->items[outline->count - 1];
            } else {
                /* outline closed with a line from end to start point */
                attempt_line_merge(outline, outline->count - 1, &pt->pos, FALSE, cos_table);
            }
            lastpt = &outline->items[0];
            to_curve = lastpt->corner != POINTTYPE_CORNER && lastpt->corner != POINTTYPE_CURVE_END;
            if (lastpt->corner == POINTTYPE_CURVE_START)
                lastpt->corner = POINTTYPE_CORNER;
            pt = &outline->items[1];
            if (attempt_line_merge(outline, 0, &pt->pos, to_curve, cos_table))
                *lastpt = outline->items[outline->count];
        }

        lastpt = &outline->items[outline->count - 1];
        pt = &outline->items[0];
        unit_vec2(&lastdir, &lastpt->pos, &pt->pos);
        for (j = 0; j < outline->count; j++)
        {
            D3DXVECTOR2 curdir;

            lastpt = pt;
            pt = &outline->items[(j + 1) % outline->count];
            unit_vec2(&curdir, &lastpt->pos, &pt->pos);

            switch (lastpt->corner)
            {
                case POINTTYPE_CURVE_START:
                case POINTTYPE_CURVE_END:
                    if (!is_direction_similar(&lastdir, &curdir, cos_table->cos_45))
                        lastpt->corner = POINTTYPE_CORNER;
                    break;
                case POINTTYPE_CURVE_MIDDLE:
                    if (!is_direction_similar(&lastdir, &curdir, cos_table->cos_90))
                        lastpt->corner = POINTTYPE_CORNER;
                    else
                        lastpt->corner = POINTTYPE_CURVE;
                    break;
                default:
                    break;
            }
            lastdir = curdir;
        }

        header = (TTPOLYGONHEADER *)((char *)header + header->cb);
    }
    return S_OK;
}

/* Get the y-distance from a line to a point */
static float get_line_to_point_y_distance(D3DXVECTOR2 *line_pt1,
                                          D3DXVECTOR2 *line_pt2,
                                          D3DXVECTOR2 *point)
{
    D3DXVECTOR2 line_vec = {0, 0};
    float line_pt_dx;
    float line_y;

    D3DXVec2Subtract(&line_vec, line_pt2, line_pt1);
    line_pt_dx = point->x - line_pt1->x;
    line_y = line_pt1->y + (line_vec.y * line_pt_dx) / line_vec.x;
    return point->y - line_y;
}

static D3DXVECTOR2 *get_indexed_point(struct point2d_index *pt_idx)
{
    return &pt_idx->outline->items[pt_idx->vertex].pos;
}

static D3DXVECTOR2 *get_ordered_vertex(struct glyphinfo *glyph, WORD index)
{
    return get_indexed_point(&glyph->ordered_vertices.items[index]);
}

static void remove_triangulation(struct triangulation_array *array, struct triangulation *item)
{
    HeapFree(GetProcessHeap(), 0, item->vertex_stack.items);
    MoveMemory(item, item + 1, (char*)&array->items[array->count] - (char*)(item + 1));
    array->count--;
}

static HRESULT triangulation_add_point(struct triangulation **t_ptr,
                                       struct triangulation_array *triangulations,
                                       WORD vtx_idx,
                                       BOOL to_top)
{
    struct glyphinfo *glyph = triangulations->glyph;
    struct triangulation *t = *t_ptr;
    HRESULT hr;
    face *face;
    int f1, f2;

    if (t->last_on_top) {
        f1 = 1;
        f2 = 2;
    } else {
        f1 = 2;
        f2 = 1;
    }

    if (t->last_on_top != to_top && t->vertex_stack.count > 1) {
        /* consume all vertices on the stack */
        WORD last_pt = t->vertex_stack.items[0];
        int i;
        for (i = 1; i < t->vertex_stack.count; i++)
        {
            face = add_face(&glyph->faces);
            if (!face) return E_OUTOFMEMORY;
            (*face)[0] = vtx_idx;
            (*face)[f1] = last_pt;
            (*face)[f2] = last_pt = t->vertex_stack.items[i];
        }
        t->vertex_stack.items[0] = last_pt;
        t->vertex_stack.count = 1;
    } else if (t->vertex_stack.count > 1) {
        int i = t->vertex_stack.count - 1;
        D3DXVECTOR2 *point = get_ordered_vertex(glyph, vtx_idx);
        WORD top_idx = t->vertex_stack.items[i--];
        D3DXVECTOR2 *top_pt = get_ordered_vertex(glyph, top_idx);

        while (i >= 0)
        {
            WORD prev_idx = t->vertex_stack.items[i--];
            D3DXVECTOR2 *prev_pt = get_ordered_vertex(glyph, prev_idx);

            if (prev_pt->x != top_pt->x &&
                ((to_top && get_line_to_point_y_distance(prev_pt, top_pt, point) > 0) ||
                 (!to_top && get_line_to_point_y_distance(prev_pt, top_pt, point) < 0)))
                break;

            face = add_face(&glyph->faces);
            if (!face) return E_OUTOFMEMORY;
            (*face)[0] = vtx_idx;
            (*face)[f1] = prev_idx;
            (*face)[f2] = top_idx;

            top_pt = prev_pt;
            top_idx = prev_idx;
            t->vertex_stack.count--;
        }
    }
    t->last_on_top = to_top;

    hr = add_vertex_index(&t->vertex_stack, vtx_idx);

    if (hr == S_OK && t->merging) {
        struct triangulation *t2;

        t2 = to_top ? t - 1 : t + 1;
        t2->merging = FALSE;
        hr = triangulation_add_point(&t2, triangulations, vtx_idx, to_top);
        if (hr != S_OK) return hr;
        remove_triangulation(triangulations, t);
        if (t2 > t)
            t2--;
        *t_ptr = t2;
    }
    return hr;
}

/* check if the point is next on the outline for either the top or bottom */
static D3DXVECTOR2 *triangulation_get_next_point(struct triangulation *t, struct glyphinfo *glyph, BOOL on_top)
{
    int i = t->last_on_top == on_top ? t->vertex_stack.count - 1 : 0;
    WORD idx = t->vertex_stack.items[i];
    struct point2d_index *pt_idx = &glyph->ordered_vertices.items[idx];
    struct outline *outline = pt_idx->outline;

    if (on_top)
        i = (pt_idx->vertex + outline->count - 1) % outline->count;
    else
        i = (pt_idx->vertex + 1) % outline->count;

    return &outline->items[i].pos;
}

static int compare_vertex_indices(const void *a, const void *b)
{
    const struct point2d_index *idx1 = a, *idx2 = b;
    const D3DXVECTOR2 *p1 = &idx1->outline->items[idx1->vertex].pos;
    const D3DXVECTOR2 *p2 = &idx2->outline->items[idx2->vertex].pos;
    float diff = p1->x - p2->x;

    if (diff == 0.0f)
        diff = p1->y - p2->y;

    return diff == 0.0f ? 0 : (diff > 0.0f ? -1 : 1);
}

static HRESULT triangulate(struct triangulation_array *triangulations)
{
    int sweep_idx;
    HRESULT hr;
    struct glyphinfo *glyph = triangulations->glyph;
    int nb_vertices = 0;
    int i;
    struct point2d_index *idx_ptr;

    for (i = 0; i < glyph->outlines.count; i++)
        nb_vertices += glyph->outlines.items[i].count;

    glyph->ordered_vertices.items = HeapAlloc(GetProcessHeap(), 0,
            nb_vertices * sizeof(*glyph->ordered_vertices.items));
    if (!glyph->ordered_vertices.items)
        return E_OUTOFMEMORY;

    idx_ptr = glyph->ordered_vertices.items;
    for (i = 0; i < glyph->outlines.count; i++)
    {
        struct outline *outline = &glyph->outlines.items[i];
        int j;

        idx_ptr->outline = outline;
        idx_ptr->vertex = 0;
        idx_ptr++;
        for (j = outline->count - 1; j > 0; j--)
        {
            idx_ptr->outline = outline;
            idx_ptr->vertex = j;
            idx_ptr++;
        }
    }
    glyph->ordered_vertices.count = nb_vertices;

    /* Native implementation seems to try to create a triangle fan from
     * the first outline point if the glyph only has one outline. */
    if (glyph->outlines.count == 1)
    {
        struct outline *outline = glyph->outlines.items;
        D3DXVECTOR2 *base = &outline->items[0].pos;
        D3DXVECTOR2 *last = &outline->items[1].pos;
        float ccw = 0;

        for (i = 2; i < outline->count; i++)
        {
            D3DXVECTOR2 *next = &outline->items[i].pos;
            D3DXVECTOR2 v1 = {0.0f, 0.0f};
            D3DXVECTOR2 v2 = {0.0f, 0.0f};

            D3DXVec2Subtract(&v1, base, last);
            D3DXVec2Subtract(&v2, last, next);
            ccw = D3DXVec2CCW(&v1, &v2);
            if (ccw > 0.0f)
                break;

            last = next;
        }
        if (ccw <= 0)
        {
            glyph->faces.items = HeapAlloc(GetProcessHeap(), 0,
                    (outline->count - 2) * sizeof(glyph->faces.items[0]));
            if (!glyph->faces.items)
                return E_OUTOFMEMORY;

            glyph->faces.count = outline->count - 2;
            for (i = 0; i < glyph->faces.count; i++)
            {
                glyph->faces.items[i][0] = 0;
                glyph->faces.items[i][1] = i + 1;
                glyph->faces.items[i][2] = i + 2;
            }
            return S_OK;
        }
    }

    /* Perform 2D polygon triangulation for complex glyphs.
     * Triangulation is performed using a sweep line concept, from right to left,
     * by processing vertices in sorted order. Complex polygons are split into
     * monotone polygons which are triangulated separately. */
    /* FIXME: The order of the faces is not consistent with the native implementation. */

    /* Reserve space for maximum possible faces from triangulation.
     * # faces for outer outlines = outline->count - 2
     * # faces for inner outlines = outline->count + 2
     * There must be at least 1 outer outline. */
    glyph->faces.items = HeapAlloc(GetProcessHeap(), 0,
            (nb_vertices + glyph->outlines.count * 2 - 4) * sizeof(glyph->faces.items[0]));
    if (!glyph->faces.items)
        return E_OUTOFMEMORY;

    qsort(glyph->ordered_vertices.items, nb_vertices,
          sizeof(glyph->ordered_vertices.items[0]), compare_vertex_indices);
    for (sweep_idx = 0; sweep_idx < glyph->ordered_vertices.count; sweep_idx++)
    {
        int start = 0;
        int end = triangulations->count;

        while (start < end)
        {
            D3DXVECTOR2 *sweep_vtx = get_ordered_vertex(glyph, sweep_idx);
            int current = (start + end) / 2;
            struct triangulation *t = &triangulations->items[current];
            BOOL on_top_outline = FALSE;
            D3DXVECTOR2 *top_next, *bottom_next;
            WORD top_idx, bottom_idx;

            if (t->merging && t->last_on_top)
                top_next = triangulation_get_next_point(t + 1, glyph, TRUE);
            else
                top_next = triangulation_get_next_point(t, glyph, TRUE);
            if (sweep_vtx == top_next)
            {
                if (t->merging && t->last_on_top)
                    t++;
                hr = triangulation_add_point(&t, triangulations, sweep_idx, TRUE);
                if (hr != S_OK) return hr;

                if (t + 1 < &triangulations->items[triangulations->count] &&
                    triangulation_get_next_point(t + 1, glyph, FALSE) == sweep_vtx)
                {
                    /* point also on bottom outline of higher triangulation */
                    struct triangulation *t2 = t + 1;
                    hr = triangulation_add_point(&t2, triangulations, sweep_idx, FALSE);
                    if (hr != S_OK) return hr;

                    t->merging = TRUE;
                    t2->merging = TRUE;
                }
                on_top_outline = TRUE;
            }

            if (t->merging && !t->last_on_top)
                bottom_next = triangulation_get_next_point(t - 1, glyph, FALSE);
            else
                bottom_next = triangulation_get_next_point(t, glyph, FALSE);
            if (sweep_vtx == bottom_next)
            {
                if (t->merging && !t->last_on_top)
                    t--;
                if (on_top_outline) {
                    /* outline finished */
                    remove_triangulation(triangulations, t);
                    break;
                }

                hr = triangulation_add_point(&t, triangulations, sweep_idx, FALSE);
                if (hr != S_OK) return hr;

                if (t > triangulations->items &&
                    triangulation_get_next_point(t - 1, glyph, TRUE) == sweep_vtx)
                {
                    struct triangulation *t2 = t - 1;
                    /* point also on top outline of lower triangulation */
                    hr = triangulation_add_point(&t2, triangulations, sweep_idx, TRUE);
                    if (hr != S_OK) return hr;
                    t = t2 + 1; /* t may be invalidated by triangulation merging */

                    t->merging = TRUE;
                    t2->merging = TRUE;
                }
                break;
            }
            if (on_top_outline)
                break;

            if (t->last_on_top) {
                top_idx = t->vertex_stack.items[t->vertex_stack.count - 1];
                bottom_idx = t->vertex_stack.items[0];
            } else {
                top_idx = t->vertex_stack.items[0];
                bottom_idx = t->vertex_stack.items[t->vertex_stack.count - 1];
            }

            /* check if the point is inside or outside this polygon */
            if (get_line_to_point_y_distance(get_ordered_vertex(glyph, top_idx),
                                             top_next, sweep_vtx) > 0)
            { /* above */
                start = current + 1;
            } else if (get_line_to_point_y_distance(get_ordered_vertex(glyph, bottom_idx),
                                                    bottom_next, sweep_vtx) < 0)
            { /* below */
                end = current;
            } else if (t->merging) {
                /* inside, so cancel merging */
                struct triangulation *t2 = t->last_on_top ? t + 1 : t - 1;
                t->merging = FALSE;
                t2->merging = FALSE;
                hr = triangulation_add_point(&t, triangulations, sweep_idx, t->last_on_top);
                if (hr != S_OK) return hr;
                hr = triangulation_add_point(&t2, triangulations, sweep_idx, t2->last_on_top);
                if (hr != S_OK) return hr;
                break;
            } else {
                /* inside, so split polygon into two monotone parts */
                struct triangulation *t2 = add_triangulation(triangulations);
                if (!t2) return E_OUTOFMEMORY;
                MoveMemory(t + 1, t, (char*)(t2 + 1) - (char*)t);
                if (t->last_on_top) {
                    t2 = t + 1;
                } else {
                    t2 = t;
                    t++;
                }

                ZeroMemory(&t2->vertex_stack, sizeof(t2->vertex_stack));
                hr = add_vertex_index(&t2->vertex_stack, t->vertex_stack.items[t->vertex_stack.count - 1]);
                if (hr != S_OK) return hr;
                hr = add_vertex_index(&t2->vertex_stack, sweep_idx);
                if (hr != S_OK) return hr;
                t2->last_on_top = !t->last_on_top;

                hr = triangulation_add_point(&t, triangulations, sweep_idx, t->last_on_top);
                if (hr != S_OK) return hr;
                break;
            }
        }
        if (start >= end)
        {
            struct triangulation *t;
            struct triangulation *t2 = add_triangulation(triangulations);
            if (!t2) return E_OUTOFMEMORY;
            t = &triangulations->items[start];
            MoveMemory(t + 1, t, (char*)(t2 + 1) - (char*)t);
            ZeroMemory(t, sizeof(*t));
            hr = add_vertex_index(&t->vertex_stack, sweep_idx);
            if (hr != S_OK) return hr;
        }
    }
    return S_OK;
}

HRESULT WINAPI D3DXCreateTextW(LPDIRECT3DDEVICE9 device,
                               HDC hdc, LPCWSTR text,
                               FLOAT deviation, FLOAT extrusion,
                               LPD3DXMESH *mesh_ptr, LPD3DXBUFFER *adjacency,
                               LPGLYPHMETRICSFLOAT glyphmetrics)
{
    HRESULT hr;
    ID3DXMesh *mesh = NULL;
    DWORD nb_vertices, nb_faces;
    DWORD nb_front_faces, nb_corners, nb_outline_points;
    struct vertex *vertices = NULL;
    face *faces = NULL;
    int textlen = 0;
    float offset_x;
    LOGFONTW lf;
    OUTLINETEXTMETRICW otm;
    HFONT font = NULL, oldfont = NULL;
    const MAT2 identity = {{0, 1}, {0, 0}, {0, 0}, {0, 1}};
    void *raw_outline = NULL;
    int bufsize = 0;
    struct glyphinfo *glyphs = NULL;
    GLYPHMETRICS gm;
    struct triangulation_array triangulations = {0, 0, NULL};
    int i;
    struct vertex *vertex_ptr;
    face *face_ptr;
    float max_deviation_sq;
    const struct cos_table cos_table = {
        cos(D3DXToRadian(0.5f)),
        cos(D3DXToRadian(45.0f)),
        cos(D3DXToRadian(90.0f)),
    };
    int f1, f2;

    TRACE("(%p, %p, %s, %f, %f, %p, %p, %p)\n", device, hdc,
          debugstr_w(text), deviation, extrusion, mesh_ptr, adjacency, glyphmetrics);

    if (!device || !hdc || !text || !*text || deviation < 0.0f || extrusion < 0.0f || !mesh_ptr)
        return D3DERR_INVALIDCALL;

    if (adjacency)
    {
        FIXME("Case of adjacency != NULL not implemented.\n");
        return E_NOTIMPL;
    }

    if (!GetObjectW(GetCurrentObject(hdc, OBJ_FONT), sizeof(lf), &lf) ||
        !GetOutlineTextMetricsW(hdc, sizeof(otm), &otm))
    {
        return D3DERR_INVALIDCALL;
    }

    if (deviation == 0.0f)
        deviation = 1.0f / otm.otmEMSquare;
    max_deviation_sq = deviation * deviation;

    lf.lfHeight = otm.otmEMSquare;
    lf.lfWidth = 0;
    font = CreateFontIndirectW(&lf);
    if (!font) {
        hr = E_OUTOFMEMORY;
        goto error;
    }
    oldfont = SelectObject(hdc, font);

    textlen = strlenW(text);
    for (i = 0; i < textlen; i++)
    {
        int datasize = GetGlyphOutlineW(hdc, text[i], GGO_NATIVE, &gm, 0, NULL, &identity);
        if (datasize < 0)
            return D3DERR_INVALIDCALL;
        if (bufsize < datasize)
            bufsize = datasize;
    }
    if (!bufsize) { /* e.g. text == " " */
        hr = D3DERR_INVALIDCALL;
        goto error;
    }

    glyphs = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, textlen * sizeof(*glyphs));
    raw_outline = HeapAlloc(GetProcessHeap(), 0, bufsize);
    if (!glyphs || !raw_outline) {
        hr = E_OUTOFMEMORY;
        goto error;
    }

    offset_x = 0.0f;
    for (i = 0; i < textlen; i++)
    {
        /* get outline points from data returned from GetGlyphOutline */
        int datasize;

        glyphs[i].offset_x = offset_x;

        datasize = GetGlyphOutlineW(hdc, text[i], GGO_NATIVE, &gm, bufsize, raw_outline, &identity);
        hr = create_outline(&glyphs[i], raw_outline, datasize,
                            max_deviation_sq, otm.otmEMSquare, &cos_table);
        if (hr != S_OK) goto error;

        triangulations.glyph = &glyphs[i];
        hr = triangulate(&triangulations);
        if (hr != S_OK) goto error;
        if (triangulations.count) {
            ERR("%d incomplete triangulations of glyph (%u).\n", triangulations.count, text[i]);
            triangulations.count = 0;
        }

        if (glyphmetrics)
        {
            glyphmetrics[i].gmfBlackBoxX = gm.gmBlackBoxX / (float)otm.otmEMSquare;
            glyphmetrics[i].gmfBlackBoxY = gm.gmBlackBoxY / (float)otm.otmEMSquare;
            glyphmetrics[i].gmfptGlyphOrigin.x = gm.gmptGlyphOrigin.x / (float)otm.otmEMSquare;
            glyphmetrics[i].gmfptGlyphOrigin.y = gm.gmptGlyphOrigin.y / (float)otm.otmEMSquare;
            glyphmetrics[i].gmfCellIncX = gm.gmCellIncX / (float)otm.otmEMSquare;
            glyphmetrics[i].gmfCellIncY = gm.gmCellIncY / (float)otm.otmEMSquare;
        }
        offset_x += gm.gmCellIncX / (float)otm.otmEMSquare;
    }

    /* corner points need an extra vertex for the different side faces normals */
    nb_corners = 0;
    nb_outline_points = 0;
    nb_front_faces = 0;
    for (i = 0; i < textlen; i++)
    {
        int j;
        nb_outline_points += glyphs[i].ordered_vertices.count;
        nb_front_faces += glyphs[i].faces.count;
        for (j = 0; j < glyphs[i].outlines.count; j++)
        {
            int k;
            struct outline *outline = &glyphs[i].outlines.items[j];
            nb_corners++; /* first outline point always repeated as a corner */
            for (k = 1; k < outline->count; k++)
                if (outline->items[k].corner)
                    nb_corners++;
        }
    }

    nb_vertices = (nb_outline_points + nb_corners) * 2 + nb_outline_points * 2;
    nb_faces = nb_outline_points * 2 + nb_front_faces * 2;


    hr = D3DXCreateMeshFVF(nb_faces, nb_vertices, D3DXMESH_MANAGED,
                           D3DFVF_XYZ | D3DFVF_NORMAL, device, &mesh);
    if (FAILED(hr))
        goto error;

    hr = mesh->lpVtbl->LockVertexBuffer(mesh, D3DLOCK_DISCARD, (LPVOID *)&vertices);
    if (FAILED(hr))
        goto error;

    hr = mesh->lpVtbl->LockIndexBuffer(mesh, D3DLOCK_DISCARD, (LPVOID *)&faces);
    if (FAILED(hr))
        goto error;

    /* convert 2D vertices and faces into 3D mesh */
    vertex_ptr = vertices;
    face_ptr = faces;
    if (extrusion == 0.0f) {
        f1 = 1;
        f2 = 2;
    } else {
        f1 = 2;
        f2 = 1;
    }
    for (i = 0; i < textlen; i++)
    {
        int j;
        int count;
        struct vertex *back_vertices;
        face *back_faces;

        /* side vertices and faces */
        for (j = 0; j < glyphs[i].outlines.count; j++)
        {
            struct vertex *outline_vertices = vertex_ptr;
            struct outline *outline = &glyphs[i].outlines.items[j];
            int k;
            struct point2d *prevpt = &outline->items[outline->count - 1];
            struct point2d *pt = &outline->items[0];

            for (k = 1; k <= outline->count; k++)
            {
                struct vertex vtx;
                struct point2d *nextpt = &outline->items[k % outline->count];
                WORD vtx_idx = vertex_ptr - vertices;
                D3DXVECTOR2 vec;

                if (pt->corner == POINTTYPE_CURVE_START)
                    D3DXVec2Subtract(&vec, &pt->pos, &prevpt->pos);
                else if (pt->corner)
                    D3DXVec2Subtract(&vec, &nextpt->pos, &pt->pos);
                else
                    D3DXVec2Subtract(&vec, &nextpt->pos, &prevpt->pos);
                D3DXVec2Normalize(&vec, &vec);
                vtx.normal.x = -vec.y;
                vtx.normal.y = vec.x;
                vtx.normal.z = 0;

                vtx.position.x = pt->pos.x + glyphs[i].offset_x;
                vtx.position.y = pt->pos.y;
                vtx.position.z = 0;
                *vertex_ptr++ = vtx;

                vtx.position.z = -extrusion;
                *vertex_ptr++ = vtx;

                vtx.position.x = nextpt->pos.x + glyphs[i].offset_x;
                vtx.position.y = nextpt->pos.y;
                if (pt->corner && nextpt->corner && nextpt->corner != POINTTYPE_CURVE_END) {
                    vtx.position.z = -extrusion;
                    *vertex_ptr++ = vtx;
                    vtx.position.z = 0;
                    *vertex_ptr++ = vtx;

                    (*face_ptr)[0] = vtx_idx;
                    (*face_ptr)[1] = vtx_idx + 2;
                    (*face_ptr)[2] = vtx_idx + 1;
                    face_ptr++;

                    (*face_ptr)[0] = vtx_idx;
                    (*face_ptr)[1] = vtx_idx + 3;
                    (*face_ptr)[2] = vtx_idx + 2;
                    face_ptr++;
                } else {
                    if (nextpt->corner) {
                        if (nextpt->corner == POINTTYPE_CURVE_END) {
                            D3DXVECTOR2 *nextpt2 = &outline->items[(k + 1) % outline->count].pos;
                            D3DXVec2Subtract(&vec, nextpt2, &nextpt->pos);
                        } else {
                            D3DXVec2Subtract(&vec, &nextpt->pos, &pt->pos);
                        }
                        D3DXVec2Normalize(&vec, &vec);
                        vtx.normal.x = -vec.y;
                        vtx.normal.y = vec.x;

                        vtx.position.z = 0;
                        *vertex_ptr++ = vtx;
                        vtx.position.z = -extrusion;
                        *vertex_ptr++ = vtx;
                    }

                    (*face_ptr)[0] = vtx_idx;
                    (*face_ptr)[1] = vtx_idx + 3;
                    (*face_ptr)[2] = vtx_idx + 1;
                    face_ptr++;

                    (*face_ptr)[0] = vtx_idx;
                    (*face_ptr)[1] = vtx_idx + 2;
                    (*face_ptr)[2] = vtx_idx + 3;
                    face_ptr++;
                }

                prevpt = pt;
                pt = nextpt;
            }
            if (!pt->corner) {
                *vertex_ptr++ = *outline_vertices++;
                *vertex_ptr++ = *outline_vertices++;
            }
        }

        /* back vertices and faces */
        back_faces = face_ptr;
        back_vertices = vertex_ptr;
        for (j = 0; j < glyphs[i].ordered_vertices.count; j++)
        {
            D3DXVECTOR2 *pt = get_ordered_vertex(&glyphs[i], j);
            vertex_ptr->position.x = pt->x + glyphs[i].offset_x;
            vertex_ptr->position.y = pt->y;
            vertex_ptr->position.z = 0;
            vertex_ptr->normal.x = 0;
            vertex_ptr->normal.y = 0;
            vertex_ptr->normal.z = 1;
            vertex_ptr++;
        }
        count = back_vertices - vertices;
        for (j = 0; j < glyphs[i].faces.count; j++)
        {
            face *f = &glyphs[i].faces.items[j];
            (*face_ptr)[0] = (*f)[0] + count;
            (*face_ptr)[1] = (*f)[1] + count;
            (*face_ptr)[2] = (*f)[2] + count;
            face_ptr++;
        }

        /* front vertices and faces */
        j = count = vertex_ptr - back_vertices;
        while (j--)
        {
            vertex_ptr->position.x = back_vertices->position.x;
            vertex_ptr->position.y = back_vertices->position.y;
            vertex_ptr->position.z = -extrusion;
            vertex_ptr->normal.x = 0;
            vertex_ptr->normal.y = 0;
            vertex_ptr->normal.z = extrusion == 0.0f ? 1.0f : -1.0f;
            vertex_ptr++;
            back_vertices++;
        }
        j = face_ptr - back_faces;
        while (j--)
        {
            (*face_ptr)[0] = (*back_faces)[0] + count;
            (*face_ptr)[1] = (*back_faces)[f1] + count;
            (*face_ptr)[2] = (*back_faces)[f2] + count;
            face_ptr++;
            back_faces++;
        }
    }

    *mesh_ptr = mesh;
    hr = D3D_OK;
error:
    if (mesh) {
        if (faces) mesh->lpVtbl->UnlockIndexBuffer(mesh);
        if (vertices) mesh->lpVtbl->UnlockVertexBuffer(mesh);
        if (hr != D3D_OK) mesh->lpVtbl->Release(mesh);
    }
    if (glyphs) {
        for (i = 0; i < textlen; i++)
        {
            int j;
            for (j = 0; j < glyphs[i].outlines.count; j++)
                HeapFree(GetProcessHeap(), 0, glyphs[i].outlines.items[j].items);
            HeapFree(GetProcessHeap(), 0, glyphs[i].outlines.items);
            HeapFree(GetProcessHeap(), 0, glyphs[i].faces.items);
            HeapFree(GetProcessHeap(), 0, glyphs[i].ordered_vertices.items);
        }
        HeapFree(GetProcessHeap(), 0, glyphs);
    }
    if (triangulations.items) {
        int i;
        for (i = 0; i < triangulations.count; i++)
            HeapFree(GetProcessHeap(), 0, triangulations.items[i].vertex_stack.items);
        HeapFree(GetProcessHeap(), 0, triangulations.items);
    }
    HeapFree(GetProcessHeap(), 0, raw_outline);
    if (oldfont) SelectObject(hdc, oldfont);
    if (font) DeleteObject(font);

    return hr;
}
