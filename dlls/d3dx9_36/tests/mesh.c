/*
 * Copyright 2008 David Adam
 * Copyright 2008 Luis Busquets
 * Copyright 2009 Henri Verbeet for CodeWeavers
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
#include "wine/test.h"
#include "d3dx9.h"

#define admitted_error 0.0001f

#define compare_vertex_sizes(type, exp) \
    got=D3DXGetFVFVertexSize(type); \
    ok(got==exp, "Expected: %d, Got: %d\n", exp, got);

static BOOL compare(FLOAT u, FLOAT v)
{
    return (fabs(u-v) < admitted_error);
}

static BOOL compare_vec3(D3DXVECTOR3 u, D3DXVECTOR3 v)
{
    return ( compare(u.x, v.x) && compare(u.y, v.y) && compare(u.z, v.z) );
}

struct vertex
{
    D3DXVECTOR3 position;
    D3DXVECTOR3 normal;
};

typedef WORD face[3];

static BOOL compare_face(face a, face b)
{
    return (a[0]==b[0] && a[1] == b[1] && a[2] == b[2]);
}

struct mesh
{
    DWORD number_of_vertices;
    struct vertex *vertices;

    DWORD number_of_faces;
    face *faces;
};

static void free_mesh(struct mesh *mesh)
{
    HeapFree(GetProcessHeap(), 0, mesh->faces);
    HeapFree(GetProcessHeap(), 0, mesh->vertices);
}

static BOOL new_mesh(struct mesh *mesh, DWORD number_of_vertices, DWORD number_of_faces)
{
    mesh->vertices = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, number_of_vertices * sizeof(*mesh->vertices));
    if (!mesh->vertices)
    {
        return FALSE;
    }
    mesh->number_of_vertices = number_of_vertices;

    mesh->faces = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, number_of_faces * sizeof(*mesh->faces));
    if (!mesh->faces)
    {
        HeapFree(GetProcessHeap(), 0, mesh->vertices);
        return FALSE;
    }
    mesh->number_of_faces = number_of_faces;

    return TRUE;
}

static void compare_mesh(const char *name, ID3DXMesh *d3dxmesh, struct mesh *mesh)
{
    HRESULT hr;
    DWORD number_of_vertices, number_of_faces;
    IDirect3DVertexBuffer9 *vertex_buffer;
    IDirect3DIndexBuffer9 *index_buffer;
    D3DVERTEXBUFFER_DESC vertex_buffer_description;
    D3DINDEXBUFFER_DESC index_buffer_description;
    struct vertex *vertices;
    face *faces;
    int expected, i;

    number_of_vertices = d3dxmesh->lpVtbl->GetNumVertices(d3dxmesh);
    ok(number_of_vertices == mesh->number_of_vertices, "Test %s, result %u, expected %d\n",
       name, number_of_vertices, mesh->number_of_vertices);

    number_of_faces = d3dxmesh->lpVtbl->GetNumFaces(d3dxmesh);
    ok(number_of_faces == mesh->number_of_faces, "Test %s, result %u, expected %d\n",
       name, number_of_faces, mesh->number_of_faces);

    /* vertex buffer */
    hr = d3dxmesh->lpVtbl->GetVertexBuffer(d3dxmesh, &vertex_buffer);
    ok(hr == D3D_OK, "Test %s, result %x, expected 0 (D3D_OK)\n", name, hr);

    if (hr != D3D_OK)
    {
        skip("Couldn't get vertex buffer\n");
    }
    else
    {
        hr = IDirect3DVertexBuffer9_GetDesc(vertex_buffer, &vertex_buffer_description);
        ok(hr == D3D_OK, "Test %s, result %x, expected 0 (D3D_OK)\n", name, hr);

        if (hr != D3D_OK)
        {
            skip("Couldn't get vertex buffer description\n");
        }
        else
        {
            ok(vertex_buffer_description.Format == D3DFMT_VERTEXDATA, "Test %s, result %x, expected %x (D3DFMT_VERTEXDATA)\n",
               name, vertex_buffer_description.Format, D3DFMT_VERTEXDATA);
            ok(vertex_buffer_description.Type == D3DRTYPE_VERTEXBUFFER, "Test %s, result %x, expected %x (D3DRTYPE_VERTEXBUFFER)\n",
               name, vertex_buffer_description.Type, D3DRTYPE_VERTEXBUFFER);
            ok(vertex_buffer_description.Usage == 0, "Test %s, result %x, expected %x\n", name, vertex_buffer_description.Usage, 0);
            ok(vertex_buffer_description.Pool == D3DPOOL_MANAGED, "Test %s, result %x, expected %x (D3DPOOL_DEFAULT)\n",
               name, vertex_buffer_description.Pool, D3DPOOL_DEFAULT);
            expected = number_of_vertices * sizeof(D3DXVECTOR3) * 2;
            ok(vertex_buffer_description.Size == expected, "Test %s, result %x, expected %x\n",
               name, vertex_buffer_description.Size, expected);
            ok(vertex_buffer_description.FVF == (D3DFVF_XYZ | D3DFVF_NORMAL), "Test %s, result %x, expected %x (D3DFVF_XYZ | D3DFVF_NORMAL)\n",
               name, vertex_buffer_description.FVF, D3DFVF_XYZ | D3DFVF_NORMAL);
        }

        /* specify offset and size to avoid potential overruns */
        hr = IDirect3DVertexBuffer9_Lock(vertex_buffer, 0, number_of_vertices * sizeof(D3DXVECTOR3) * 2,
                                         (LPVOID *)&vertices, D3DLOCK_DISCARD);
        ok(hr == D3D_OK, "Test %s, result %x, expected 0 (D3D_OK)\n", name, hr);

        if (hr != D3D_OK)
        {
            skip("Couldn't lock vertex buffer\n");
        }
        else
        {
            for (i = 0; i < number_of_vertices; i++)
            {
                ok(compare_vec3(vertices[i].position, mesh->vertices[i].position),
                   "Test %s, vertex position %d, result (%g, %g, %g), expected (%g, %g, %g)\n", name, i,
                   vertices[i].position.x, vertices[i].position.y, vertices[i].position.z,
                   mesh->vertices[i].position.x, mesh->vertices[i].position.y, mesh->vertices[i].position.z);
                ok(compare_vec3(vertices[i].normal, mesh->vertices[i].normal),
                   "Test %s, vertex normal %d, result (%g, %g, %g), expected (%g, %g, %g)\n", name, i,
                   vertices[i].normal.x, vertices[i].normal.y, vertices[i].normal.z,
                   mesh->vertices[i].normal.x, mesh->vertices[i].normal.y, mesh->vertices[i].normal.z);
            }

            IDirect3DVertexBuffer9_Unlock(vertex_buffer);
        }

        IDirect3DVertexBuffer9_Release(vertex_buffer);
    }

    /* index buffer */
    hr = d3dxmesh->lpVtbl->GetIndexBuffer(d3dxmesh, &index_buffer);
    ok(hr == D3D_OK, "Test %s, result %x, expected 0 (D3D_OK)\n", name, hr);

    if (!index_buffer)
    {
        skip("Couldn't get index buffer\n");
    }
    else
    {
        hr = IDirect3DIndexBuffer9_GetDesc(index_buffer, &index_buffer_description);
        ok(hr == D3D_OK, "Test %s, result %x, expected 0 (D3D_OK)\n", name, hr);

        if (hr != D3D_OK)
        {
            skip("Couldn't get index buffer description\n");
        }
        else
        {
            ok(index_buffer_description.Format == D3DFMT_INDEX16, "Test %s, result %x, expected %x (D3DFMT_INDEX16)\n",
               name, index_buffer_description.Format, D3DFMT_INDEX16);
            ok(index_buffer_description.Type == D3DRTYPE_INDEXBUFFER, "Test %s, result %x, expected %x (D3DRTYPE_INDEXBUFFER)\n",
               name, index_buffer_description.Type, D3DRTYPE_INDEXBUFFER);
            ok(index_buffer_description.Usage == 0, "Test %s, result %x, expected %x\n", name, index_buffer_description.Usage, 0);
            ok(index_buffer_description.Pool == D3DPOOL_MANAGED, "Test %s, result %x, expected %x (D3DPOOL_DEFAULT)\n",
               name, index_buffer_description.Pool, D3DPOOL_DEFAULT);
            expected = number_of_faces * sizeof(WORD) * 3;
            ok(index_buffer_description.Size == expected, "Test %s, result %x, expected %x\n",
               name, index_buffer_description.Size, expected);
        }

        /* specify offset and size to avoid potential overruns */
        hr = IDirect3DIndexBuffer9_Lock(index_buffer, 0, number_of_faces * sizeof(WORD) * 3,
                                        (LPVOID *)&faces, D3DLOCK_DISCARD);
        ok(hr == D3D_OK, "Test %s, result %x, expected 0 (D3D_OK)\n", name, hr);

        if (hr != D3D_OK)
        {
            skip("Couldn't lock index buffer\n");
        }
        else
        {
            for (i = 0; i < number_of_faces; i++)
            {
                ok(compare_face(faces[i], mesh->faces[i]),
                   "Test %s, face %d, result (%u, %u, %u), expected (%u, %u, %u)\n", name, i,
                   faces[i][0], faces[i][1], faces[i][2],
                   mesh->faces[i][0], mesh->faces[i][1], mesh->faces[i][2]);
            }

            IDirect3DIndexBuffer9_Unlock(index_buffer);
        }

        IDirect3DIndexBuffer9_Release(index_buffer);
    }
}

static void D3DXBoundProbeTest(void)
{
    BOOL result;
    D3DXVECTOR3 bottom_point, center, top_point, raydirection, rayposition;
    FLOAT radius;

/*____________Test the Box case___________________________*/
    bottom_point.x = -3.0f; bottom_point.y = -2.0f; bottom_point.z = -1.0f;
    top_point.x = 7.0f; top_point.y = 8.0f; top_point.z = 9.0f;

    raydirection.x = -4.0f; raydirection.y = -5.0f; raydirection.z = -6.0f;
    rayposition.x = 5.0f; rayposition.y = 5.0f; rayposition.z = 11.0f;
    result = D3DXBoxBoundProbe(&bottom_point, &top_point, &rayposition, &raydirection);
    ok(result == TRUE, "expected TRUE, received FALSE\n");

    raydirection.x = 4.0f; raydirection.y = 5.0f; raydirection.z = 6.0f;
    rayposition.x = 5.0f; rayposition.y = 5.0f; rayposition.z = 11.0f;
    result = D3DXBoxBoundProbe(&bottom_point, &top_point, &rayposition, &raydirection);
    ok(result == FALSE, "expected FALSE, received TRUE\n");

    rayposition.x = -4.0f; rayposition.y = 1.0f; rayposition.z = -2.0f;
    result = D3DXBoxBoundProbe(&bottom_point, &top_point, &rayposition, &raydirection);
    ok(result == TRUE, "expected TRUE, received FALSE\n");

    bottom_point.x = 1.0f; bottom_point.y = 0.0f; bottom_point.z = 0.0f;
    top_point.x = 1.0f; top_point.y = 0.0f; top_point.z = 0.0f;
    rayposition.x = 0.0f; rayposition.y = 1.0f; rayposition.z = 0.0f;
    raydirection.x = 0.0f; raydirection.y = 3.0f; raydirection.z = 0.0f;
    result = D3DXBoxBoundProbe(&bottom_point, &top_point, &rayposition, &raydirection);
    ok(result == FALSE, "expected FALSE, received TRUE\n");

    bottom_point.x = 1.0f; bottom_point.y = 2.0f; bottom_point.z = 3.0f;
    top_point.x = 10.0f; top_point.y = 15.0f; top_point.z = 20.0f;

    raydirection.x = 7.0f; raydirection.y = 8.0f; raydirection.z = 9.0f;
    rayposition.x = 3.0f; rayposition.y = 7.0f; rayposition.z = -6.0f;
    result = D3DXBoxBoundProbe(&bottom_point, &top_point, &rayposition, &raydirection);
    ok(result == TRUE, "expected TRUE, received FALSE\n");

    bottom_point.x = 0.0f; bottom_point.y = 0.0f; bottom_point.z = 0.0f;
    top_point.x = 1.0f; top_point.y = 1.0f; top_point.z = 1.0f;

    raydirection.x = 0.0f; raydirection.y = 1.0f; raydirection.z = .0f;
    rayposition.x = -3.0f; rayposition.y = 0.0f; rayposition.z = 0.0f;
    result = D3DXBoxBoundProbe(&bottom_point, &top_point, &rayposition, &raydirection);
    ok(result == FALSE, "expected FALSE, received TRUE\n");

    raydirection.x = 1.0f; raydirection.y = 0.0f; raydirection.z = .0f;
    rayposition.x = -3.0f; rayposition.y = 0.0f; rayposition.z = 0.0f;
    result = D3DXBoxBoundProbe(&bottom_point, &top_point, &rayposition, &raydirection);
    ok(result == TRUE, "expected TRUE, received FALSE\n");

/*____________Test the Sphere case________________________*/
    radius = sqrt(77.0f);
    center.x = 1.0f; center.y = 2.0f; center.z = 3.0f;
    raydirection.x = 2.0f; raydirection.y = -4.0f; raydirection.z = 2.0f;

    rayposition.x = 5.0f; rayposition.y = 5.0f; rayposition.z = 9.0f;
    result = D3DXSphereBoundProbe(&center, radius, &rayposition, &raydirection);
    ok(result == TRUE, "expected TRUE, received FALSE\n");

    rayposition.x = 45.0f; rayposition.y = -75.0f; rayposition.z = 49.0f;
    result = D3DXSphereBoundProbe(&center, radius, &rayposition, &raydirection);
    ok(result == FALSE, "expected FALSE, received TRUE\n");

    rayposition.x = 5.0f; rayposition.y = 11.0f; rayposition.z = 9.0f;
    result = D3DXSphereBoundProbe(&center, radius, &rayposition, &raydirection);
    ok(result == FALSE, "expected FALSE, received TRUE\n");
}

static void D3DXComputeBoundingBoxTest(void)
{
    D3DXVECTOR3 exp_max, exp_min, got_max, got_min, vertex[5];
    HRESULT hr;

    vertex[0].x = 1.0f; vertex[0].y = 1.0f; vertex[0].z = 1.0f;
    vertex[1].x = 1.0f; vertex[1].y = 1.0f; vertex[1].z = 1.0f;
    vertex[2].x = 1.0f; vertex[2].y = 1.0f; vertex[2].z = 1.0f;
    vertex[3].x = 1.0f; vertex[3].y = 1.0f; vertex[3].z = 1.0f;
    vertex[4].x = 9.0f; vertex[4].y = 9.0f; vertex[4].z = 9.0f;

    exp_min.x = 1.0f; exp_min.y = 1.0f; exp_min.z = 1.0f;
    exp_max.x = 9.0f; exp_max.y = 9.0f; exp_max.z = 9.0f;

    hr = D3DXComputeBoundingBox(&vertex[3],2,D3DXGetFVFVertexSize(D3DFVF_XYZ),&got_min,&got_max);

    ok( hr == D3D_OK, "Expected D3D_OK, got %#x\n", hr);
    ok( compare_vec3(exp_min,got_min), "Expected min: (%f, %f, %f), got: (%f, %f, %f)\n", exp_min.x,exp_min.y,exp_min.z,got_min.x,got_min.y,got_min.z);
    ok( compare_vec3(exp_max,got_max), "Expected max: (%f, %f, %f), got: (%f, %f, %f)\n", exp_max.x,exp_max.y,exp_max.z,got_max.x,got_max.y,got_max.z);

/*________________________*/

    vertex[0].x = 2.0f; vertex[0].y = 5.9f; vertex[0].z = -1.2f;
    vertex[1].x = -1.87f; vertex[1].y = 7.9f; vertex[1].z = 7.4f;
    vertex[2].x = 7.43f; vertex[2].y = -0.9f; vertex[2].z = 11.9f;
    vertex[3].x = -6.92f; vertex[3].y = 6.3f; vertex[3].z = -3.8f;
    vertex[4].x = 11.4f; vertex[4].y = -8.1f; vertex[4].z = 4.5f;

    exp_min.x = -6.92f; exp_min.y = -8.1f; exp_min.z = -3.80f;
    exp_max.x = 11.4f; exp_max.y = 7.90f; exp_max.z = 11.9f;

    hr = D3DXComputeBoundingBox(&vertex[0],5,D3DXGetFVFVertexSize(D3DFVF_XYZ),&got_min,&got_max);

    ok( hr == D3D_OK, "Expected D3D_OK, got %#x\n", hr);
    ok( compare_vec3(exp_min,got_min), "Expected min: (%f, %f, %f), got: (%f, %f, %f)\n", exp_min.x,exp_min.y,exp_min.z,got_min.x,got_min.y,got_min.z);
    ok( compare_vec3(exp_max,got_max), "Expected max: (%f, %f, %f), got: (%f, %f, %f)\n", exp_max.x,exp_max.y,exp_max.z,got_max.x,got_max.y,got_max.z);

/*________________________*/

    vertex[0].x = 2.0f; vertex[0].y = 5.9f; vertex[0].z = -1.2f;
    vertex[1].x = -1.87f; vertex[1].y = 7.9f; vertex[1].z = 7.4f;
    vertex[2].x = 7.43f; vertex[2].y = -0.9f; vertex[2].z = 11.9f;
    vertex[3].x = -6.92f; vertex[3].y = 6.3f; vertex[3].z = -3.8f;
    vertex[4].x = 11.4f; vertex[4].y = -8.1f; vertex[4].z = 4.5f;

    exp_min.x = -6.92f; exp_min.y = -0.9f; exp_min.z = -3.8f;
    exp_max.x = 7.43f; exp_max.y = 7.90f; exp_max.z = 11.9f;

    hr = D3DXComputeBoundingBox(&vertex[0],4,D3DXGetFVFVertexSize(D3DFVF_XYZ),&got_min,&got_max);

    ok( hr == D3D_OK, "Expected D3D_OK, got %#x\n", hr);
    ok( compare_vec3(exp_min,got_min), "Expected min: (%f, %f, %f), got: (%f, %f, %f)\n", exp_min.x,exp_min.y,exp_min.z,got_min.x,got_min.y,got_min.z);
    ok( compare_vec3(exp_max,got_max), "Expected max: (%f, %f, %f), got: (%f, %f, %f)\n", exp_max.x,exp_max.y,exp_max.z,got_max.x,got_max.y,got_max.z);

/*________________________*/
    hr = D3DXComputeBoundingBox(NULL,5,D3DXGetFVFVertexSize(D3DFVF_XYZ),&got_min,&got_max);
    ok( hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#x\n", hr);

/*________________________*/
    hr = D3DXComputeBoundingBox(&vertex[3],5,D3DXGetFVFVertexSize(D3DFVF_XYZ),NULL,&got_max);
    ok( hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#x\n", hr);

/*________________________*/
    hr = D3DXComputeBoundingBox(&vertex[3],5,D3DXGetFVFVertexSize(D3DFVF_XYZ),&got_min,NULL);
    ok( hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#x\n", hr);
}

static void D3DXComputeBoundingSphereTest(void)
{
    D3DXVECTOR3 exp_cen, got_cen, vertex[5];
    FLOAT exp_rad, got_rad;
    HRESULT hr;

    vertex[0].x = 1.0f; vertex[0].y = 1.0f; vertex[0].z = 1.0f;
    vertex[1].x = 1.0f; vertex[1].y = 1.0f; vertex[1].z = 1.0f;
    vertex[2].x = 1.0f; vertex[2].y = 1.0f; vertex[2].z = 1.0f;
    vertex[3].x = 1.0f; vertex[3].y = 1.0f; vertex[3].z = 1.0f;
    vertex[4].x = 9.0f; vertex[4].y = 9.0f; vertex[4].z = 9.0f;

    exp_rad = 6.928203f;
    exp_cen.x = 5.0; exp_cen.y = 5.0; exp_cen.z = 5.0;

    hr = D3DXComputeBoundingSphere(&vertex[3],2,D3DXGetFVFVertexSize(D3DFVF_XYZ),&got_cen,&got_rad);

    ok( hr == D3D_OK, "Expected D3D_OK, got %#x\n", hr);
    ok( compare(exp_rad, got_rad), "Expected radius: %f, got radius: %f\n", exp_rad, got_rad);
    ok( compare_vec3(exp_cen,got_cen), "Expected center: (%f, %f, %f), got center: (%f, %f, %f)\n", exp_cen.x,exp_cen.y,exp_cen.z,got_cen.x,got_cen.y,got_cen.z);

/*________________________*/

    vertex[0].x = 2.0f; vertex[0].y = 5.9f; vertex[0].z = -1.2f;
    vertex[1].x = -1.87f; vertex[1].y = 7.9f; vertex[1].z = 7.4f;
    vertex[2].x = 7.43f; vertex[2].y = -0.9f; vertex[2].z = 11.9f;
    vertex[3].x = -6.92f; vertex[3].y = 6.3f; vertex[3].z = -3.8f;
    vertex[4].x = 11.4f; vertex[4].y = -8.1f; vertex[4].z = 4.5f;

    exp_rad = 13.707883f;
    exp_cen.x = 2.408f; exp_cen.y = 2.22f; exp_cen.z = 3.76f;

    hr = D3DXComputeBoundingSphere(&vertex[0],5,D3DXGetFVFVertexSize(D3DFVF_XYZ),&got_cen,&got_rad);

    ok( hr == D3D_OK, "Expected D3D_OK, got %#x\n", hr);
    ok( compare(exp_rad, got_rad), "Expected radius: %f, got radius: %f\n", exp_rad, got_rad);
    ok( compare_vec3(exp_cen,got_cen), "Expected center: (%f, %f, %f), got center: (%f, %f, %f)\n", exp_cen.x,exp_cen.y,exp_cen.z,got_cen.x,got_cen.y,got_cen.z);

/*________________________*/
    hr = D3DXComputeBoundingSphere(NULL,5,D3DXGetFVFVertexSize(D3DFVF_XYZ),&got_cen,&got_rad);
    ok( hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#x\n", hr);

/*________________________*/
    hr = D3DXComputeBoundingSphere(&vertex[3],5,D3DXGetFVFVertexSize(D3DFVF_XYZ),NULL,&got_rad);
    ok( hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#x\n", hr);

/*________________________*/
    hr = D3DXComputeBoundingSphere(&vertex[3],5,D3DXGetFVFVertexSize(D3DFVF_XYZ),&got_cen,NULL);
    ok( hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#x\n", hr);
}

static void print_elements(const D3DVERTEXELEMENT9 *elements)
{
    D3DVERTEXELEMENT9 last = D3DDECL_END();
    const D3DVERTEXELEMENT9 *ptr = elements;
    int count = 0;

    while (memcmp(ptr, &last, sizeof(D3DVERTEXELEMENT9)))
    {
        trace(
            "[Element %d] Stream = %d, Offset = %d, Type = %d, Method = %d, Usage = %d, UsageIndex = %d\n",
             count, ptr->Stream, ptr->Offset, ptr->Type, ptr->Method, ptr->Usage, ptr->UsageIndex);
        ptr++;
        count++;
    }
}

static void compare_elements(const D3DVERTEXELEMENT9 *elements, const D3DVERTEXELEMENT9 *expected_elements,
        unsigned int line, unsigned int test_id)
{
    D3DVERTEXELEMENT9 last = D3DDECL_END();
    unsigned int i;

    for (i = 0; i < MAX_FVF_DECL_SIZE; i++)
    {
        int end1 = memcmp(&elements[i], &last, sizeof(last));
        int end2 = memcmp(&expected_elements[i], &last, sizeof(last));
        int status;

        if (!end1 && !end2) break;

        status = !end1 ^ !end2;
        ok(!status, "Line %u, test %u: Mismatch in size, test declaration is %s than expected.\n",
                line, test_id, end1 ? "shorter" : "longer");
        if (status)
        {
            print_elements(elements);
            break;
        }

        status = memcmp(&elements[i], &expected_elements[i], sizeof(D3DVERTEXELEMENT9));
        ok(!status, "Line %u, test %u: Mismatch in element %u.\n", line, test_id, i);
        if (status)
        {
            print_elements(elements);
            break;
        }
    }
}

static void test_fvf_to_decl(DWORD test_fvf, const D3DVERTEXELEMENT9 expected_elements[],
        HRESULT expected_hr, unsigned int line, unsigned int test_id)
{
    HRESULT hr;
    D3DVERTEXELEMENT9 decl[MAX_FVF_DECL_SIZE];

    hr = D3DXDeclaratorFromFVF(test_fvf, decl);
    ok(hr == expected_hr,
            "Line %u, test %u: D3DXDeclaratorFromFVF returned %#x, expected %#x.\n",
            line, test_id, hr, expected_hr);
    if (SUCCEEDED(hr)) compare_elements(decl, expected_elements, line, test_id);
}

static void test_decl_to_fvf(const D3DVERTEXELEMENT9 *decl, DWORD expected_fvf,
        HRESULT expected_hr, unsigned int line, unsigned int test_id)
{
    HRESULT hr;
    DWORD result_fvf = 0xdeadbeef;

    hr = D3DXFVFFromDeclarator(decl, &result_fvf);
    ok(hr == expected_hr,
       "Line %u, test %u: D3DXFVFFromDeclarator returned %#x, expected %#x.\n",
       line, test_id, hr, expected_hr);
    if (SUCCEEDED(hr))
    {
        ok(expected_fvf == result_fvf, "Line %u, test %u: Got FVF %#x, expected %#x.\n",
                line, test_id, result_fvf, expected_fvf);
    }
}

static void test_fvf_decl_conversion(void)
{
    static const struct
    {
        D3DVERTEXELEMENT9 decl[MAXD3DDECLLENGTH + 1];
        DWORD fvf;
    }
    test_data[] =
    {
        {{
            D3DDECL_END(),
        }, 0},
        {{
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
            D3DDECL_END(),
        }, D3DFVF_XYZ},
        {{
            {0, 0, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_POSITIONT, 0},
            D3DDECL_END(),
        }, D3DFVF_XYZRHW},
        {{
            {0, 0, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_POSITIONT, 0},
            D3DDECL_END(),
        }, D3DFVF_XYZRHW},
        {{
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
            {0, 12, D3DDECLTYPE_FLOAT1, 0, D3DDECLUSAGE_BLENDWEIGHT, 0},
            D3DDECL_END(),
        }, D3DFVF_XYZB1},
        {{
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
            {0, 12, D3DDECLTYPE_UBYTE4, 0, D3DDECLUSAGE_BLENDINDICES, 0},
            D3DDECL_END(),
        }, D3DFVF_XYZB1 | D3DFVF_LASTBETA_UBYTE4},
        {{
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
            {0, 12, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_BLENDINDICES, 0},
            D3DDECL_END(),
        }, D3DFVF_XYZB1 | D3DFVF_LASTBETA_D3DCOLOR},
        {{
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
            {0, 12, D3DDECLTYPE_FLOAT2, 0, D3DDECLUSAGE_BLENDWEIGHT, 0},
            D3DDECL_END(),
        }, D3DFVF_XYZB2},
        {{
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
            {0, 12, D3DDECLTYPE_FLOAT1, 0, D3DDECLUSAGE_BLENDWEIGHT, 0},
            {0, 16, D3DDECLTYPE_UBYTE4, 0, D3DDECLUSAGE_BLENDINDICES, 0},
            D3DDECL_END(),
        }, D3DFVF_XYZB2 | D3DFVF_LASTBETA_UBYTE4},
        {{
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
            {0, 12, D3DDECLTYPE_FLOAT1, 0, D3DDECLUSAGE_BLENDWEIGHT, 0},
            {0, 16, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_BLENDINDICES, 0},
            D3DDECL_END(),
        }, D3DFVF_XYZB2 | D3DFVF_LASTBETA_D3DCOLOR},
        {{
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
            {0, 12, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_BLENDWEIGHT, 0},
            D3DDECL_END(),
        }, D3DFVF_XYZB3},
        {{
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
            {0, 12, D3DDECLTYPE_FLOAT2, 0, D3DDECLUSAGE_BLENDWEIGHT, 0},
            {0, 20, D3DDECLTYPE_UBYTE4, 0, D3DDECLUSAGE_BLENDINDICES, 0},
            D3DDECL_END(),
        }, D3DFVF_XYZB3 | D3DFVF_LASTBETA_UBYTE4},
        {{
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
            {0, 12, D3DDECLTYPE_FLOAT2, 0, D3DDECLUSAGE_BLENDWEIGHT, 0},
            {0, 20, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_BLENDINDICES, 0},
            D3DDECL_END(),
        }, D3DFVF_XYZB3 | D3DFVF_LASTBETA_D3DCOLOR},
        {{
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
            {0, 12, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_BLENDWEIGHT, 0},
            D3DDECL_END(),
        }, D3DFVF_XYZB4},
        {{
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
            {0, 12, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_BLENDWEIGHT, 0},
            {0, 24, D3DDECLTYPE_UBYTE4, 0, D3DDECLUSAGE_BLENDINDICES, 0},
            D3DDECL_END(),
        }, D3DFVF_XYZB4 | D3DFVF_LASTBETA_UBYTE4},
        {{
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
            {0, 12, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_BLENDWEIGHT, 0},
            {0, 24, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_BLENDINDICES, 0},
            D3DDECL_END(),
        }, D3DFVF_XYZB4 | D3DFVF_LASTBETA_D3DCOLOR},
        {{
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
            {0, 12, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_BLENDWEIGHT, 0},
            {0, 28, D3DDECLTYPE_UBYTE4, 0, D3DDECLUSAGE_BLENDINDICES, 0},
            D3DDECL_END(),
        }, D3DFVF_XYZB5 | D3DFVF_LASTBETA_UBYTE4},
        {{
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
            {0, 12, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_BLENDWEIGHT, 0},
            {0, 28, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_BLENDINDICES, 0},
            D3DDECL_END(),
        }, D3DFVF_XYZB5 | D3DFVF_LASTBETA_D3DCOLOR},
        {{
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_NORMAL, 0},
            D3DDECL_END(),
        }, D3DFVF_NORMAL},
        {{
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_NORMAL, 0},
            {0, 12, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_COLOR, 0},
            D3DDECL_END(),
        }, D3DFVF_NORMAL | D3DFVF_DIFFUSE},
        {{
            {0, 0, D3DDECLTYPE_FLOAT1, 0, D3DDECLUSAGE_PSIZE, 0},
            D3DDECL_END(),
        }, D3DFVF_PSIZE},
        {{
            {0, 0, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_COLOR, 0},
            D3DDECL_END(),
        }, D3DFVF_DIFFUSE},
        {{
            {0, 0, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_COLOR, 1},
            D3DDECL_END(),
        }, D3DFVF_SPECULAR},
        /* Make sure textures of different sizes work. */
        {{
            {0, 0, D3DDECLTYPE_FLOAT1, 0, D3DDECLUSAGE_TEXCOORD, 0},
            D3DDECL_END(),
        }, D3DFVF_TEXCOORDSIZE1(0) | D3DFVF_TEX1},
        {{
            {0, 0, D3DDECLTYPE_FLOAT2, 0, D3DDECLUSAGE_TEXCOORD, 0},
            D3DDECL_END(),
        }, D3DFVF_TEXCOORDSIZE2(0) | D3DFVF_TEX1},
        {{
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_TEXCOORD, 0},
            D3DDECL_END(),
        }, D3DFVF_TEXCOORDSIZE3(0) | D3DFVF_TEX1},
        {{
            {0, 0, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_TEXCOORD, 0},
            D3DDECL_END(),
        }, D3DFVF_TEXCOORDSIZE4(0) | D3DFVF_TEX1},
        /* Make sure the TEXCOORD index works correctly - try several textures. */
        {{
            {0, 0, D3DDECLTYPE_FLOAT1, 0, D3DDECLUSAGE_TEXCOORD, 0},
            {0, 4, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_TEXCOORD, 1},
            {0, 16, D3DDECLTYPE_FLOAT2, 0, D3DDECLUSAGE_TEXCOORD, 2},
            {0, 24, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_TEXCOORD, 3},
            D3DDECL_END(),
        }, D3DFVF_TEX4 | D3DFVF_TEXCOORDSIZE1(0) | D3DFVF_TEXCOORDSIZE3(1)
                | D3DFVF_TEXCOORDSIZE2(2) | D3DFVF_TEXCOORDSIZE4(3)},
        /* Now try some combination tests. */
        {{
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
            {0, 12, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_BLENDWEIGHT, 0},
            {0, 28, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_COLOR, 0},
            {0, 32, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_COLOR, 1},
            {0, 36, D3DDECLTYPE_FLOAT2, 0, D3DDECLUSAGE_TEXCOORD, 0},
            {0, 44, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_TEXCOORD, 1},
            D3DDECL_END(),
        }, D3DFVF_XYZB4 | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX2
                | D3DFVF_TEXCOORDSIZE2(0) | D3DFVF_TEXCOORDSIZE3(1)},
        {{
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
            {0, 12, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_NORMAL, 0},
            {0, 24, D3DDECLTYPE_FLOAT1, 0, D3DDECLUSAGE_PSIZE, 0},
            {0, 28, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_COLOR, 1},
            {0, 32, D3DDECLTYPE_FLOAT1, 0, D3DDECLUSAGE_TEXCOORD, 0},
            {0, 36, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_TEXCOORD, 1},
            D3DDECL_END(),
        }, D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_PSIZE | D3DFVF_SPECULAR | D3DFVF_TEX2
                | D3DFVF_TEXCOORDSIZE1(0) | D3DFVF_TEXCOORDSIZE4(1)},
    };
    unsigned int i;

    for (i = 0; i < sizeof(test_data) / sizeof(*test_data); ++i)
    {
        test_decl_to_fvf(test_data[i].decl, test_data[i].fvf, D3D_OK, __LINE__, i);
        test_fvf_to_decl(test_data[i].fvf, test_data[i].decl, D3D_OK, __LINE__, i);
    }

    /* Usage indices for position and normal are apparently ignored. */
    {
        const D3DVERTEXELEMENT9 decl[] =
        {
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 1},
            D3DDECL_END(),
        };
        test_decl_to_fvf(decl, D3DFVF_XYZ, D3D_OK, __LINE__, 0);
    }
    {
        const D3DVERTEXELEMENT9 decl[] =
        {
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_NORMAL, 1},
            D3DDECL_END(),
        };
        test_decl_to_fvf(decl, D3DFVF_NORMAL, D3D_OK, __LINE__, 0);
    }
    /* D3DFVF_LASTBETA_UBYTE4 and D3DFVF_LASTBETA_D3DCOLOR are ignored if
     * there are no blend matrices. */
    {
        const D3DVERTEXELEMENT9 decl[] =
        {
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
            D3DDECL_END(),
        };
        test_fvf_to_decl(D3DFVF_XYZ | D3DFVF_LASTBETA_UBYTE4, decl, D3D_OK, __LINE__, 0);
    }
    {
        const D3DVERTEXELEMENT9 decl[] =
        {
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
            D3DDECL_END(),
        };
        test_fvf_to_decl(D3DFVF_XYZ | D3DFVF_LASTBETA_D3DCOLOR, decl, D3D_OK, __LINE__, 0);
    }
    /* D3DFVF_LASTBETA_UBYTE4 takes precedence over D3DFVF_LASTBETA_D3DCOLOR. */
    {
        const D3DVERTEXELEMENT9 decl[] =
        {
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
            {0, 12, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_BLENDWEIGHT, 0},
            {0, 28, D3DDECLTYPE_UBYTE4, 0, D3DDECLUSAGE_BLENDINDICES, 0},
            D3DDECL_END(),
        };
        test_fvf_to_decl(D3DFVF_XYZB5 | D3DFVF_LASTBETA_D3DCOLOR | D3DFVF_LASTBETA_UBYTE4,
                decl, D3D_OK, __LINE__, 0);
    }
    /* These are supposed to fail, both ways. */
    {
        const D3DVERTEXELEMENT9 decl[] =
        {
            {0, 0, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_POSITION, 0},
            D3DDECL_END(),
        };
        test_decl_to_fvf(decl, D3DFVF_XYZW, D3DERR_INVALIDCALL, __LINE__, 0);
        test_fvf_to_decl(D3DFVF_XYZW, decl, D3DERR_INVALIDCALL, __LINE__, 0);
    }
    {
        const D3DVERTEXELEMENT9 decl[] =
        {
            {0, 0, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_POSITION, 0},
            {0, 16, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_NORMAL, 0},
            D3DDECL_END(),
        };
        test_decl_to_fvf(decl, D3DFVF_XYZW | D3DFVF_NORMAL, D3DERR_INVALIDCALL, __LINE__, 0);
        test_fvf_to_decl(D3DFVF_XYZW | D3DFVF_NORMAL, decl, D3DERR_INVALIDCALL, __LINE__, 0);
    }
    {
        const D3DVERTEXELEMENT9 decl[] =
        {
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
            {0, 12, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_BLENDWEIGHT, 0},
            {0, 28, D3DDECLTYPE_FLOAT1, 0, D3DDECLUSAGE_BLENDINDICES, 0},
            D3DDECL_END(),
        };
        test_decl_to_fvf(decl, D3DFVF_XYZB5, D3DERR_INVALIDCALL, __LINE__, 0);
        test_fvf_to_decl(D3DFVF_XYZB5, decl, D3DERR_INVALIDCALL, __LINE__, 0);
    }
    /* Test a declaration that can't be converted to an FVF. */
    {
        const D3DVERTEXELEMENT9 decl[] =
        {
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
            {0, 12, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_NORMAL, 0},
            {0, 24, D3DDECLTYPE_FLOAT1, 0, D3DDECLUSAGE_PSIZE, 0},
            {0, 28, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_COLOR, 1},
            {0, 32, D3DDECLTYPE_FLOAT1, 0, D3DDECLUSAGE_TEXCOORD, 0},
            /* 8 bytes padding */
            {0, 44, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_TEXCOORD, 1},
            D3DDECL_END(),
        };
        test_decl_to_fvf(decl, 0, D3DERR_INVALIDCALL, __LINE__, 0);
    }
    /* Elements must be ordered by offset. */
    {
        const D3DVERTEXELEMENT9 decl[] =
        {
            {0, 12, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_COLOR, 0},
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
            D3DDECL_END(),
        };
        test_decl_to_fvf(decl, 0, D3DERR_INVALIDCALL, __LINE__, 0);
    }
    /* Basic tests for element order. */
    {
        const D3DVERTEXELEMENT9 decl[] =
        {
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
            {0, 12, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_COLOR, 0},
            {0, 16, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_NORMAL, 0},
            D3DDECL_END(),
        };
        test_decl_to_fvf(decl, 0, D3DERR_INVALIDCALL, __LINE__, 0);
    }
    {
        const D3DVERTEXELEMENT9 decl[] =
        {
            {0, 0, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_COLOR, 0},
            {0, 4, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
            D3DDECL_END(),
        };
        test_decl_to_fvf(decl, 0, D3DERR_INVALIDCALL, __LINE__, 0);
    }
    {
        const D3DVERTEXELEMENT9 decl[] =
        {
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_NORMAL, 0},
            {0, 12, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
            D3DDECL_END(),
        };
        test_decl_to_fvf(decl, 0, D3DERR_INVALIDCALL, __LINE__, 0);
    }
    /* Textures must be ordered by texcoords. */
    {
        const D3DVERTEXELEMENT9 decl[] =
        {
            {0, 0, D3DDECLTYPE_FLOAT1, 0, D3DDECLUSAGE_TEXCOORD, 0},
            {0, 4, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_TEXCOORD, 2},
            {0, 16, D3DDECLTYPE_FLOAT2, 0, D3DDECLUSAGE_TEXCOORD, 1},
            {0, 24, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_TEXCOORD, 3},
            D3DDECL_END(),
        };
        test_decl_to_fvf(decl, 0, D3DERR_INVALIDCALL, __LINE__, 0);
    }
    /* Duplicate elements are not allowed. */
    {
        const D3DVERTEXELEMENT9 decl[] =
        {
            {0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
            {0, 12, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_COLOR, 0},
            {0, 16, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_COLOR, 0},
            D3DDECL_END(),
        };
        test_decl_to_fvf(decl, 0, D3DERR_INVALIDCALL, __LINE__, 0);
    }
}

static void D3DXGetFVFVertexSizeTest(void)
{
    UINT got;

    compare_vertex_sizes (D3DFVF_XYZ, 12);

    compare_vertex_sizes (D3DFVF_XYZB3, 24);

    compare_vertex_sizes (D3DFVF_XYZB5, 32);

    compare_vertex_sizes (D3DFVF_XYZ | D3DFVF_NORMAL, 24);

    compare_vertex_sizes (D3DFVF_XYZ | D3DFVF_DIFFUSE, 16);

    compare_vertex_sizes (
        D3DFVF_XYZ |
        D3DFVF_TEX1 |
        D3DFVF_TEXCOORDSIZE1(0), 16);
    compare_vertex_sizes (
        D3DFVF_XYZ |
        D3DFVF_TEX2 |
        D3DFVF_TEXCOORDSIZE1(0) |
        D3DFVF_TEXCOORDSIZE1(1), 20);

    compare_vertex_sizes (
        D3DFVF_XYZ |
        D3DFVF_TEX1 |
        D3DFVF_TEXCOORDSIZE2(0), 20);

    compare_vertex_sizes (
        D3DFVF_XYZ |
        D3DFVF_TEX2 |
        D3DFVF_TEXCOORDSIZE2(0) |
        D3DFVF_TEXCOORDSIZE2(1), 28);

    compare_vertex_sizes (
        D3DFVF_XYZ |
        D3DFVF_TEX6 |
        D3DFVF_TEXCOORDSIZE2(0) |
        D3DFVF_TEXCOORDSIZE2(1) |
        D3DFVF_TEXCOORDSIZE2(2) |
        D3DFVF_TEXCOORDSIZE2(3) |
        D3DFVF_TEXCOORDSIZE2(4) |
        D3DFVF_TEXCOORDSIZE2(5), 60);

    compare_vertex_sizes (
        D3DFVF_XYZ |
        D3DFVF_TEX8 |
        D3DFVF_TEXCOORDSIZE2(0) |
        D3DFVF_TEXCOORDSIZE2(1) |
        D3DFVF_TEXCOORDSIZE2(2) |
        D3DFVF_TEXCOORDSIZE2(3) |
        D3DFVF_TEXCOORDSIZE2(4) |
        D3DFVF_TEXCOORDSIZE2(5) |
        D3DFVF_TEXCOORDSIZE2(6) |
        D3DFVF_TEXCOORDSIZE2(7), 76);

    compare_vertex_sizes (
        D3DFVF_XYZ |
        D3DFVF_TEX1 |
        D3DFVF_TEXCOORDSIZE3(0), 24);

    compare_vertex_sizes (
        D3DFVF_XYZ |
        D3DFVF_TEX4 |
        D3DFVF_TEXCOORDSIZE3(0) |
        D3DFVF_TEXCOORDSIZE3(1) |
        D3DFVF_TEXCOORDSIZE3(2) |
        D3DFVF_TEXCOORDSIZE3(3), 60);

    compare_vertex_sizes (
        D3DFVF_XYZ |
        D3DFVF_TEX1 |
        D3DFVF_TEXCOORDSIZE4(0), 28);

    compare_vertex_sizes (
        D3DFVF_XYZ |
        D3DFVF_TEX2 |
        D3DFVF_TEXCOORDSIZE4(0) |
        D3DFVF_TEXCOORDSIZE4(1), 44);

    compare_vertex_sizes (
        D3DFVF_XYZ |
        D3DFVF_TEX3 |
        D3DFVF_TEXCOORDSIZE4(0) |
        D3DFVF_TEXCOORDSIZE4(1) |
        D3DFVF_TEXCOORDSIZE4(2), 60);

    compare_vertex_sizes (
        D3DFVF_XYZB5 |
        D3DFVF_NORMAL |
        D3DFVF_DIFFUSE |
        D3DFVF_SPECULAR |
        D3DFVF_TEX8 |
        D3DFVF_TEXCOORDSIZE4(0) |
        D3DFVF_TEXCOORDSIZE4(1) |
        D3DFVF_TEXCOORDSIZE4(2) |
        D3DFVF_TEXCOORDSIZE4(3) |
        D3DFVF_TEXCOORDSIZE4(4) |
        D3DFVF_TEXCOORDSIZE4(5) |
        D3DFVF_TEXCOORDSIZE4(6) |
        D3DFVF_TEXCOORDSIZE4(7), 180);
}

static void D3DXIntersectTriTest(void)
{
    BOOL exp_res, got_res;
    D3DXVECTOR3 position, ray, vertex[3];
    FLOAT exp_dist, got_dist, exp_u, got_u, exp_v, got_v;

    vertex[0].x = 1.0f; vertex[0].y = 0.0f; vertex[0].z = 0.0f;
    vertex[1].x = 2.0f; vertex[1].y = 0.0f; vertex[1].z = 0.0f;
    vertex[2].x = 1.0f; vertex[2].y = 1.0f; vertex[2].z = 0.0f;

    position.x = -14.5f; position.y = -23.75f; position.z = -32.0f;

    ray.x = 2.0f; ray.y = 3.0f; ray.z = 4.0f;

    exp_res = TRUE; exp_u = 0.5f; exp_v = 0.25f; exp_dist = 8.0f;

    got_res = D3DXIntersectTri(&vertex[0],&vertex[1],&vertex[2],&position,&ray,&got_u,&got_v,&got_dist);
    ok( got_res == exp_res, "Expected result = %d, got %d\n",exp_res,got_res);
    ok( compare(exp_u,got_u), "Expected u = %f, got %f\n",exp_u,got_u);
    ok( compare(exp_v,got_v), "Expected v = %f, got %f\n",exp_v,got_v);
    ok( compare(exp_dist,got_dist), "Expected distance = %f, got %f\n",exp_dist,got_dist);

/*Only positive ray is taken in account*/

    vertex[0].x = 1.0f; vertex[0].y = 0.0f; vertex[0].z = 0.0f;
    vertex[1].x = 2.0f; vertex[1].y = 0.0f; vertex[1].z = 0.0f;
    vertex[2].x = 1.0f; vertex[2].y = 1.0f; vertex[2].z = 0.0f;

    position.x = 17.5f; position.y = 24.25f; position.z = 32.0f;

    ray.x = 2.0f; ray.y = 3.0f; ray.z = 4.0f;

    exp_res = FALSE;

    got_res = D3DXIntersectTri(&vertex[0],&vertex[1],&vertex[2],&position,&ray,&got_u,&got_v,&got_dist);
    ok( got_res == exp_res, "Expected result = %d, got %d\n",exp_res,got_res);

/*Intersection between ray and triangle in a same plane is considered as empty*/

    vertex[0].x = 4.0f; vertex[0].y = 0.0f; vertex[0].z = 0.0f;
    vertex[1].x = 6.0f; vertex[1].y = 0.0f; vertex[1].z = 0.0f;
    vertex[2].x = 4.0f; vertex[2].y = 2.0f; vertex[2].z = 0.0f;

    position.x = 1.0f; position.y = 1.0f; position.z = 0.0f;

    ray.x = 1.0f; ray.y = 0.0f; ray.z = 0.0f;

    exp_res = FALSE;

    got_res = D3DXIntersectTri(&vertex[0],&vertex[1],&vertex[2],&position,&ray,&got_u,&got_v,&got_dist);
    ok( got_res == exp_res, "Expected result = %d, got %d\n",exp_res,got_res);
}

static void D3DXCreateMeshTest(void)
{
    HRESULT hr;
    HWND wnd;
    IDirect3D9 *d3d;
    IDirect3DDevice9 *device, *test_device;
    D3DPRESENT_PARAMETERS d3dpp;
    ID3DXMesh *d3dxmesh;
    int i, size;
    D3DVERTEXELEMENT9 test_decl[MAX_FVF_DECL_SIZE];
    DWORD fvf, options;
    struct mesh mesh;

    static const D3DVERTEXELEMENT9 decl[3] = {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 0xC, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
        D3DDECL_END(), };

    hr = D3DXCreateMesh(0, 0, 0, NULL, NULL, NULL);
    todo_wine ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateMesh(1, 3, D3DXMESH_MANAGED, (LPD3DVERTEXELEMENT9 *)&decl, NULL, &d3dxmesh);
    todo_wine ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    wnd = CreateWindow("static", "d3dx9_test", 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL);
    if (!wnd)
    {
        skip("Couldn't create application window\n");
        return;
    }
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!d3d)
    {
        skip("Couldn't create IDirect3D9 object\n");
        DestroyWindow(wnd);
        return;
    }

    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    hr = IDirect3D9_CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, wnd, D3DCREATE_MIXED_VERTEXPROCESSING, &d3dpp, &device);
    if (FAILED(hr))
    {
        skip("Failed to create IDirect3DDevice9 object %#x\n", hr);
        IDirect3D9_Release(d3d);
        DestroyWindow(wnd);
        return;
    }

    hr = D3DXCreateMesh(0, 3, D3DXMESH_MANAGED, (LPD3DVERTEXELEMENT9 *)&decl, device, &d3dxmesh);
    todo_wine ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateMesh(1, 0, D3DXMESH_MANAGED, (LPD3DVERTEXELEMENT9 *)&decl, device, &d3dxmesh);
    todo_wine ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateMesh(1, 3, 0, (LPD3DVERTEXELEMENT9 *)&decl, device, &d3dxmesh);
    todo_wine ok(hr == D3D_OK, "Got result %x, expected %x (D3D_OK)\n", hr, D3D_OK);

    if (hr == D3D_OK)
    {
        d3dxmesh->lpVtbl->Release(d3dxmesh);
    }

    hr = D3DXCreateMesh(1, 3, D3DXMESH_MANAGED, 0, device, &d3dxmesh);
    todo_wine ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateMesh(1, 3, D3DXMESH_MANAGED, (LPD3DVERTEXELEMENT9 *)&decl, device, NULL);
    todo_wine ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateMesh(1, 3, D3DXMESH_MANAGED, (LPD3DVERTEXELEMENT9 *)&decl, device, &d3dxmesh);
    todo_wine ok(hr == D3D_OK, "Got result %x, expected 0 (D3D_OK)\n", hr);

    if (hr == D3D_OK)
    {
        /* device */
        hr = d3dxmesh->lpVtbl->GetDevice(d3dxmesh, NULL);
        ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

        hr = d3dxmesh->lpVtbl->GetDevice(d3dxmesh, &test_device);
        ok(hr == D3D_OK, "Got result %x, expected %x (D3D_OK)\n", hr, D3D_OK);
        ok(test_device == device, "Got result %p, expected %p\n", test_device, device);

        if (hr == D3D_OK)
        {
            IDirect3DDevice9_Release(device);
        }

        /* declaration */
        hr = d3dxmesh->lpVtbl->GetDeclaration(d3dxmesh, test_decl);
        ok(hr == D3D_OK, "Got result %x, expected 0 (D3D_OK)\n", hr);

        if (hr == D3D_OK)
        {
            size = sizeof(decl) / sizeof(decl[0]);
            for (i = 0; i < size - 1; i++)
            {
                ok(test_decl[i].Stream == decl[i].Stream, "Returned stream %d, expected %d\n", test_decl[i].Stream, decl[i].Stream);
                ok(test_decl[i].Type == decl[i].Type, "Returned type %d, expected %d\n", test_decl[i].Type, decl[i].Type);
                ok(test_decl[i].Method == decl[i].Method, "Returned method %d, expected %d\n", test_decl[i].Method, decl[i].Method);
                ok(test_decl[i].Usage == decl[i].Usage, "Returned usage %d, expected %d\n", test_decl[i].Usage, decl[i].Usage);
                ok(test_decl[i].UsageIndex == decl[i].UsageIndex, "Returned usage index %d, expected %d\n", test_decl[i].UsageIndex, decl[i].UsageIndex);
                ok(test_decl[i].Offset == decl[i].Offset, "Returned offset %d, expected %d\n", decl[1].Offset, decl[i].Offset);
            }
            ok(decl[size-1].Stream == 0xFF, "Returned too long vertex declaration\n"); /* end element */
        }

        /* FVF */
        fvf = d3dxmesh->lpVtbl->GetFVF(d3dxmesh);
        ok(fvf == (D3DFVF_XYZ | D3DFVF_NORMAL), "Got result %x, expected %x (D3DFVF_XYZ | D3DFVF_NORMAL)\n", fvf, D3DFVF_XYZ | D3DFVF_NORMAL);

        /* options */
        options = d3dxmesh->lpVtbl->GetOptions(d3dxmesh);
        ok(options == D3DXMESH_MANAGED, "Got result %x, expected %x (D3DXMESH_MANAGED)\n", options, D3DXMESH_MANAGED);

        /* rest */
        if (!new_mesh(&mesh, 3, 1))
        {
            skip("Couldn't create mesh\n");
        }
        else
        {
            memset(mesh.vertices, 0, mesh.number_of_vertices * sizeof(*mesh.vertices));
            memset(mesh.faces, 0, mesh.number_of_faces * sizeof(*mesh.faces));

            compare_mesh("createmeshfvf", d3dxmesh, &mesh);

            free_mesh(&mesh);
        }

        d3dxmesh->lpVtbl->Release(d3dxmesh);
    }

    IDirect3DDevice9_Release(device);
    IDirect3D9_Release(d3d);
    DestroyWindow(wnd);
}

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

static WORD sphere_vertex(UINT slices, int slice, int stack)
{
    return stack*slices+slice+1;
}

/* slices = subdivisions along xy plane, stacks = subdivisions along z axis */
static BOOL compute_sphere(struct mesh *mesh, FLOAT radius, UINT slices, UINT stacks)
{
    float theta_step, theta_start;
    struct sincos_table theta;
    float phi_step, phi_start;
    struct sincos_table phi;
    DWORD number_of_vertices, number_of_faces;
    DWORD vertex, face;
    int slice, stack;

    /* theta = angle on xy plane wrt x axis */
    theta_step = M_PI / stacks;
    theta_start = theta_step;

    /* phi = angle on xz plane wrt z axis */
    phi_step = -2 * M_PI / slices;
    phi_start = M_PI / 2;

    if (!compute_sincos_table(&theta, theta_start, theta_step, stacks))
    {
        return FALSE;
    }
    if (!compute_sincos_table(&phi, phi_start, phi_step, slices))
    {
        free_sincos_table(&theta);
        return FALSE;
    }

    number_of_vertices = 2 + slices * (stacks-1);
    number_of_faces = 2 * slices + (stacks - 2) * (2 * slices);

    if (!new_mesh(mesh, number_of_vertices, number_of_faces))
    {
        free_sincos_table(&phi);
        free_sincos_table(&theta);
        return FALSE;
    }

    vertex = 0;
    face = 0;
    stack = 0;

    mesh->vertices[vertex].normal.x = 0.0f;
    mesh->vertices[vertex].normal.y = 0.0f;
    mesh->vertices[vertex].normal.z = 1.0f;
    mesh->vertices[vertex].position.x = 0.0f;
    mesh->vertices[vertex].position.y = 0.0f;
    mesh->vertices[vertex].position.z = radius;
    vertex++;

    for (stack = 0; stack < stacks - 1; stack++)
    {
        for (slice = 0; slice < slices; slice++)
        {
            mesh->vertices[vertex].normal.x = theta.sin[stack] * phi.cos[slice];
            mesh->vertices[vertex].normal.y = theta.sin[stack] * phi.sin[slice];
            mesh->vertices[vertex].normal.z = theta.cos[stack];
            mesh->vertices[vertex].position.x = radius * theta.sin[stack] * phi.cos[slice];
            mesh->vertices[vertex].position.y = radius * theta.sin[stack] * phi.sin[slice];
            mesh->vertices[vertex].position.z = radius * theta.cos[stack];
            vertex++;

            if (slice > 0)
            {
                if (stack == 0)
                {
                    /* top stack is triangle fan */
                    mesh->faces[face][0] = 0;
                    mesh->faces[face][1] = slice + 1;
                    mesh->faces[face][2] = slice;
                    face++;
                }
                else
                {
                    /* stacks in between top and bottom are quad strips */
                    mesh->faces[face][0] = sphere_vertex(slices, slice-1, stack-1);
                    mesh->faces[face][1] = sphere_vertex(slices, slice, stack-1);
                    mesh->faces[face][2] = sphere_vertex(slices, slice-1, stack);
                    face++;

                    mesh->faces[face][0] = sphere_vertex(slices, slice, stack-1);
                    mesh->faces[face][1] = sphere_vertex(slices, slice, stack);
                    mesh->faces[face][2] = sphere_vertex(slices, slice-1, stack);
                    face++;
                }
            }
        }

        if (stack == 0)
        {
            mesh->faces[face][0] = 0;
            mesh->faces[face][1] = 1;
            mesh->faces[face][2] = slice;
            face++;
        }
        else
        {
            mesh->faces[face][0] = sphere_vertex(slices, slice-1, stack-1);
            mesh->faces[face][1] = sphere_vertex(slices, 0, stack-1);
            mesh->faces[face][2] = sphere_vertex(slices, slice-1, stack);
            face++;

            mesh->faces[face][0] = sphere_vertex(slices, 0, stack-1);
            mesh->faces[face][1] = sphere_vertex(slices, 0, stack);
            mesh->faces[face][2] = sphere_vertex(slices, slice-1, stack);
            face++;
        }
    }

    mesh->vertices[vertex].position.x = 0.0f;
    mesh->vertices[vertex].position.y = 0.0f;
    mesh->vertices[vertex].position.z = -radius;
    mesh->vertices[vertex].normal.x = 0.0f;
    mesh->vertices[vertex].normal.y = 0.0f;
    mesh->vertices[vertex].normal.z = -1.0f;

    /* bottom stack is triangle fan */
    for (slice = 1; slice < slices; slice++)
    {
        mesh->faces[face][0] = sphere_vertex(slices, slice-1, stack-1);
        mesh->faces[face][1] = sphere_vertex(slices, slice, stack-1);
        mesh->faces[face][2] = vertex;
        face++;
    }

    mesh->faces[face][0] = sphere_vertex(slices, slice-1, stack-1);
    mesh->faces[face][1] = sphere_vertex(slices, 0, stack-1);
    mesh->faces[face][2] = vertex;

    free_sincos_table(&phi);
    free_sincos_table(&theta);

    return TRUE;
}

static void test_sphere(IDirect3DDevice9 *device, FLOAT radius, UINT slices, UINT stacks)
{
    HRESULT hr;
    ID3DXMesh *sphere;
    struct mesh mesh;
    char name[256];

    hr = D3DXCreateSphere(device, radius, slices, stacks, &sphere, NULL);
    todo_wine ok(hr == D3D_OK, "Got result %x, expected 0 (D3D_OK)\n", hr);
    if (hr != D3D_OK)
    {
        skip("Couldn't create sphere\n");
        return;
    }

    if (!compute_sphere(&mesh, radius, slices, stacks))
    {
        skip("Couldn't create mesh\n");
        sphere->lpVtbl->Release(sphere);
        return;
    }

    sprintf(name, "sphere (%g, %u, %u)", radius, slices, stacks);
    compare_mesh(name, sphere, &mesh);

    free_mesh(&mesh);

    sphere->lpVtbl->Release(sphere);
}

static void D3DXCreateSphereTest(void)
{
    HRESULT hr;
    HWND wnd;
    IDirect3D9* d3d;
    IDirect3DDevice9* device;
    D3DPRESENT_PARAMETERS d3dpp;
    ID3DXMesh* sphere = NULL;

    hr = D3DXCreateSphere(NULL, 0.0f, 0, 0, NULL, NULL);
    todo_wine ok( hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n",hr,D3DERR_INVALIDCALL);

    hr = D3DXCreateSphere(NULL, 0.1f, 0, 0, NULL, NULL);
    todo_wine ok( hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n",hr,D3DERR_INVALIDCALL);

    hr = D3DXCreateSphere(NULL, 0.0f, 1, 0, NULL, NULL);
    todo_wine ok( hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n",hr,D3DERR_INVALIDCALL);

    hr = D3DXCreateSphere(NULL, 0.0f, 0, 1, NULL, NULL);
    todo_wine ok( hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n",hr,D3DERR_INVALIDCALL);

    wnd = CreateWindow("static", "d3dx9_test", 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!wnd)
    {
        skip("Couldn't create application window\n");
        return;
    }
    if (!d3d)
    {
        skip("Couldn't create IDirect3D9 object\n");
        DestroyWindow(wnd);
        return;
    }

    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    hr = IDirect3D9_CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, wnd, D3DCREATE_MIXED_VERTEXPROCESSING, &d3dpp, &device);
    if (FAILED(hr))
    {
        skip("Failed to create IDirect3DDevice9 object %#x\n", hr);
        IDirect3D9_Release(d3d);
        DestroyWindow(wnd);
        return;
    }

    hr = D3DXCreateSphere(device, 1.0f, 1, 1, &sphere, NULL);
    todo_wine ok( hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n",hr,D3DERR_INVALIDCALL);

    hr = D3DXCreateSphere(device, 1.0f, 2, 1, &sphere, NULL);
    todo_wine ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateSphere(device, 1.0f, 1, 2, &sphere, NULL);
    todo_wine ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateSphere(device, -0.1f, 1, 2, &sphere, NULL);
    todo_wine ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    test_sphere(device, 0.0f, 2, 2);
    test_sphere(device, 1.0f, 2, 2);
    test_sphere(device, 1.0f, 3, 2);
    test_sphere(device, 1.0f, 4, 4);
    test_sphere(device, 1.0f, 3, 4);
    test_sphere(device, 5.0f, 6, 7);
    test_sphere(device, 10.0f, 11, 12);

    IDirect3DDevice9_Release(device);
    IDirect3D9_Release(d3d);
    DestroyWindow(wnd);
}

static void test_get_decl_vertex_size(void)
{
    static const D3DVERTEXELEMENT9 declaration1[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT1, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {1, 0, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {2, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {3, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {4, 0, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {5, 0, D3DDECLTYPE_UBYTE4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {6, 0, D3DDECLTYPE_SHORT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {7, 0, D3DDECLTYPE_SHORT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {8, 0, D3DDECLTYPE_UBYTE4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {9, 0, D3DDECLTYPE_SHORT2N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {10, 0, D3DDECLTYPE_SHORT4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {11, 0, D3DDECLTYPE_UDEC3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {12, 0, D3DDECLTYPE_DEC3N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {13, 0, D3DDECLTYPE_FLOAT16_2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {14, 0, D3DDECLTYPE_FLOAT16_4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        D3DDECL_END(),
    };
    static const D3DVERTEXELEMENT9 declaration2[] =
    {
        {0, 8, D3DDECLTYPE_FLOAT1, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {1, 8, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {2, 8, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {3, 8, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {4, 8, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {5, 8, D3DDECLTYPE_UBYTE4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {6, 8, D3DDECLTYPE_SHORT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {7, 8, D3DDECLTYPE_SHORT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 8, D3DDECLTYPE_UBYTE4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {1, 8, D3DDECLTYPE_SHORT2N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {2, 8, D3DDECLTYPE_SHORT4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {3, 8, D3DDECLTYPE_UDEC3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {4, 8, D3DDECLTYPE_DEC3N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {5, 8, D3DDECLTYPE_FLOAT16_2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {6, 8, D3DDECLTYPE_FLOAT16_4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {7, 8, D3DDECLTYPE_FLOAT1, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        D3DDECL_END(),
    };
    static const UINT sizes1[] =
    {
        4,  8,  12, 16,
        4,  4,  4,  8,
        4,  4,  8,  4,
        4,  4,  8,  0,
    };
    static const UINT sizes2[] =
    {
        12, 16, 20, 24,
        12, 12, 16, 16,
    };
    unsigned int i;
    UINT size;

    size = D3DXGetDeclVertexSize(NULL, 0);
    ok(size == 0, "Got size %#x, expected 0.\n", size);

    for (i = 0; i < 16; ++i)
    {
        size = D3DXGetDeclVertexSize(declaration1, i);
        ok(size == sizes1[i], "Got size %u for stream %u, expected %u.\n", size, i, sizes1[i]);
    }

    for (i = 0; i < 8; ++i)
    {
        size = D3DXGetDeclVertexSize(declaration2, i);
        ok(size == sizes2[i], "Got size %u for stream %u, expected %u.\n", size, i, sizes2[i]);
    }
}

START_TEST(mesh)
{
    D3DXBoundProbeTest();
    D3DXComputeBoundingBoxTest();
    D3DXComputeBoundingSphereTest();
    D3DXGetFVFVertexSizeTest();
    D3DXIntersectTriTest();
    D3DXCreateMeshTest();
    D3DXCreateSphereTest();
    test_get_decl_vertex_size();
    test_fvf_decl_conversion();
}
