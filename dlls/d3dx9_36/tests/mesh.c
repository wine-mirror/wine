/*
 * Copyright 2008 David Adam
 * Copyright 2008 Luis Busquets
 * Copyright 2009 Henri Verbeet for CodeWeavers
 * Copyright 2011 Michael Mc Donnell
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

#define COBJMACROS
#include <stdio.h>
#include <float.h>
#include "wine/test.h"
#include "d3dx9.h"

/* Set the WINETEST_DEBUG environment variable to be greater than 1 for verbose
 * function call traces of ID3DXAllocateHierarchy callbacks. */
#define TRACECALLBACK if(winetest_debug > 1) trace

#define admitted_error 0.0001f

#define ARRAY_SIZE(array) (sizeof(array)/sizeof(*array))

#define compare_vertex_sizes(type, exp) \
    got=D3DXGetFVFVertexSize(type); \
    ok(got==exp, "Expected: %d, Got: %d\n", exp, got);

#define compare_float(got, exp) \
    do { \
        float _got = (got); \
        float _exp = (exp); \
        ok(_got == _exp, "Expected: %g, Got: %g\n", _exp, _got); \
    } while (0)

static BOOL compare(FLOAT u, FLOAT v)
{
    return (fabs(u-v) < admitted_error);
}

static BOOL compare_vec3(D3DXVECTOR3 u, D3DXVECTOR3 v)
{
    return ( compare(u.x, v.x) && compare(u.y, v.y) && compare(u.z, v.z) );
}

#define check_floats(got, exp, dim) check_floats_(__LINE__, "", got, exp, dim)
static void check_floats_(int line, const char *prefix, const float *got, const float *exp, int dim)
{
    int i;
    char exp_buffer[256] = "";
    char got_buffer[256] = "";
    char *exp_buffer_ptr = exp_buffer;
    char *got_buffer_ptr = got_buffer;
    BOOL equal = TRUE;

    for (i = 0; i < dim; i++) {
        if (i) {
            exp_buffer_ptr += sprintf(exp_buffer_ptr, ", ");
            got_buffer_ptr += sprintf(got_buffer_ptr, ", ");
        }
        equal = equal && compare(*exp, *got);
        exp_buffer_ptr += sprintf(exp_buffer_ptr, "%g", *exp);
        got_buffer_ptr += sprintf(got_buffer_ptr, "%g", *got);
        exp++, got++;
    }
    ok_(__FILE__,line)(equal, "%sExpected (%s), got (%s)", prefix, exp_buffer, got_buffer);
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

struct test_context
{
    HWND hwnd;
    IDirect3D9 *d3d;
    IDirect3DDevice9 *device;
};

/* Initializes a test context struct. Use it to initialize DirectX.
 *
 * Returns NULL if an error occurred.
 */
static struct test_context *new_test_context(void)
{
    HRESULT hr;
    HWND hwnd = NULL;
    IDirect3D9 *d3d = NULL;
    IDirect3DDevice9 *device = NULL;
    D3DPRESENT_PARAMETERS d3dpp = {0};
    struct test_context *test_context;

    hwnd = CreateWindow("static", "d3dx9_test", 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL);
    if (!hwnd)
    {
        skip("Couldn't create application window\n");
        goto error;
    }

    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!d3d)
    {
        skip("Couldn't create IDirect3D9 object\n");
        goto error;
    }

    memset(&d3dpp, 0, sizeof(d3dpp));
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    hr = IDirect3D9_CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd,
                                 D3DCREATE_MIXED_VERTEXPROCESSING, &d3dpp, &device);
    if (FAILED(hr))
    {
        skip("Couldn't create IDirect3DDevice9 object %#x\n", hr);
        goto error;
    }

    test_context = HeapAlloc(GetProcessHeap(), 0, sizeof(*test_context));
    if (!test_context)
    {
        skip("Couldn't allocate memory for test_context\n");
        goto error;
    }
    test_context->hwnd = hwnd;
    test_context->d3d = d3d;
    test_context->device = device;

    return test_context;

error:
    if (device)
        IDirect3DDevice9_Release(device);

    if (d3d)
        IDirect3D9_Release(d3d);

    if (hwnd)
        DestroyWindow(hwnd);

    return NULL;
}

static void free_test_context(struct test_context *test_context)
{
    if (!test_context)
        return;

    if (test_context->device)
        IDirect3DDevice9_Release(test_context->device);

    if (test_context->d3d)
        IDirect3D9_Release(test_context->d3d);

    if (test_context->hwnd)
        DestroyWindow(test_context->hwnd);

    HeapFree(GetProcessHeap(), 0, test_context);
}

struct mesh
{
    DWORD number_of_vertices;
    struct vertex *vertices;

    DWORD number_of_faces;
    face *faces;

    DWORD fvf;
    UINT vertex_size;
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
            ok(vertex_buffer_description.Pool == D3DPOOL_MANAGED, "Test %s, result %x, expected %x (D3DPOOL_MANAGED)\n",
               name, vertex_buffer_description.Pool, D3DPOOL_MANAGED);
            ok(vertex_buffer_description.FVF == mesh->fvf, "Test %s, result %x, expected %x\n",
               name, vertex_buffer_description.FVF, mesh->fvf);
            if (mesh->fvf == 0)
            {
                expected = number_of_vertices * mesh->vertex_size;
            }
            else
            {
                expected = number_of_vertices * D3DXGetFVFVertexSize(mesh->fvf);
            }
            ok(vertex_buffer_description.Size == expected, "Test %s, result %x, expected %x\n",
               name, vertex_buffer_description.Size, expected);
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
            todo_wine ok(index_buffer_description.Usage == 0, "Test %s, result %x, expected %x\n", name, index_buffer_description.Usage, 0);
            ok(index_buffer_description.Pool == D3DPOOL_MANAGED, "Test %s, result %x, expected %x (D3DPOOL_MANAGED)\n",
               name, index_buffer_description.Pool, D3DPOOL_MANAGED);
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
    /* Invalid FVFs cannot be converted to a declarator. */
    test_fvf_to_decl(0xdeadbeef, NULL, D3DERR_INVALIDCALL, __LINE__, 0);
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
    DWORD options;
    struct mesh mesh;

    static const D3DVERTEXELEMENT9 decl1[3] = {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
        D3DDECL_END(), };

    static const D3DVERTEXELEMENT9 decl2[] = {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
        {0, 24, D3DDECLTYPE_FLOAT1, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_PSIZE, 0},
        {0, 28, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 1},
        {0, 32, D3DDECLTYPE_FLOAT1, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
        /* 8 bytes padding */
        {0, 44, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1},
        D3DDECL_END(),
    };

    static const D3DVERTEXELEMENT9 decl3[] = {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {1, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
        D3DDECL_END(),
    };

    hr = D3DXCreateMesh(0, 0, 0, NULL, NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateMesh(1, 3, D3DXMESH_MANAGED, decl1, NULL, &d3dxmesh);
    ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

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

    hr = D3DXCreateMesh(0, 3, D3DXMESH_MANAGED, decl1, device, &d3dxmesh);
    ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateMesh(1, 0, D3DXMESH_MANAGED, decl1, device, &d3dxmesh);
    ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateMesh(1, 3, 0, decl1, device, &d3dxmesh);
    ok(hr == D3D_OK, "Got result %x, expected %x (D3D_OK)\n", hr, D3D_OK);

    if (hr == D3D_OK)
    {
        d3dxmesh->lpVtbl->Release(d3dxmesh);
    }

    hr = D3DXCreateMesh(1, 3, D3DXMESH_MANAGED, 0, device, &d3dxmesh);
    ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateMesh(1, 3, D3DXMESH_MANAGED, decl1, device, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateMesh(1, 3, D3DXMESH_MANAGED, decl1, device, &d3dxmesh);
    ok(hr == D3D_OK, "Got result %x, expected 0 (D3D_OK)\n", hr);

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
        hr = d3dxmesh->lpVtbl->GetDeclaration(d3dxmesh, NULL);
        ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

        hr = d3dxmesh->lpVtbl->GetDeclaration(d3dxmesh, test_decl);
        ok(hr == D3D_OK, "Got result %x, expected 0 (D3D_OK)\n", hr);

        if (hr == D3D_OK)
        {
            size = sizeof(decl1) / sizeof(decl1[0]);
            for (i = 0; i < size - 1; i++)
            {
                ok(test_decl[i].Stream == decl1[i].Stream, "Returned stream %d, expected %d\n", test_decl[i].Stream, decl1[i].Stream);
                ok(test_decl[i].Type == decl1[i].Type, "Returned type %d, expected %d\n", test_decl[i].Type, decl1[i].Type);
                ok(test_decl[i].Method == decl1[i].Method, "Returned method %d, expected %d\n", test_decl[i].Method, decl1[i].Method);
                ok(test_decl[i].Usage == decl1[i].Usage, "Returned usage %d, expected %d\n", test_decl[i].Usage, decl1[i].Usage);
                ok(test_decl[i].UsageIndex == decl1[i].UsageIndex, "Returned usage index %d, expected %d\n", test_decl[i].UsageIndex, decl1[i].UsageIndex);
                ok(test_decl[i].Offset == decl1[i].Offset, "Returned offset %d, expected %d\n", test_decl[i].Offset, decl1[i].Offset);
            }
            ok(decl1[size-1].Stream == 0xFF, "Returned too long vertex declaration\n"); /* end element */
        }

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
            mesh.fvf = D3DFVF_XYZ | D3DFVF_NORMAL;

            compare_mesh("createmesh1", d3dxmesh, &mesh);

            free_mesh(&mesh);
        }

        d3dxmesh->lpVtbl->Release(d3dxmesh);
    }

    /* Test a declaration that can't be converted to an FVF. */
    hr = D3DXCreateMesh(1, 3, D3DXMESH_MANAGED, decl2, device, &d3dxmesh);
    ok(hr == D3D_OK, "Got result %x, expected 0 (D3D_OK)\n", hr);

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
            size = sizeof(decl2) / sizeof(decl2[0]);
            for (i = 0; i < size - 1; i++)
            {
                ok(test_decl[i].Stream == decl2[i].Stream, "Returned stream %d, expected %d\n", test_decl[i].Stream, decl2[i].Stream);
                ok(test_decl[i].Type == decl2[i].Type, "Returned type %d, expected %d\n", test_decl[i].Type, decl2[i].Type);
                ok(test_decl[i].Method == decl2[i].Method, "Returned method %d, expected %d\n", test_decl[i].Method, decl2[i].Method);
                ok(test_decl[i].Usage == decl2[i].Usage, "Returned usage %d, expected %d\n", test_decl[i].Usage, decl2[i].Usage);
                ok(test_decl[i].UsageIndex == decl2[i].UsageIndex, "Returned usage index %d, expected %d\n", test_decl[i].UsageIndex, decl2[i].UsageIndex);
                ok(test_decl[i].Offset == decl2[i].Offset, "Returned offset %d, expected %d\n", test_decl[i].Offset, decl2[i].Offset);
            }
            ok(decl2[size-1].Stream == 0xFF, "Returned too long vertex declaration\n"); /* end element */
        }

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
            mesh.fvf = 0;
            mesh.vertex_size = 60;

            compare_mesh("createmesh2", d3dxmesh, &mesh);

            free_mesh(&mesh);
        }

        mesh.vertex_size = d3dxmesh->lpVtbl->GetNumBytesPerVertex(d3dxmesh);
        ok(mesh.vertex_size == 60, "Got vertex size %u, expected %u\n", mesh.vertex_size, 60);

        d3dxmesh->lpVtbl->Release(d3dxmesh);
    }

    /* Test a declaration with multiple streams. */
    hr = D3DXCreateMesh(1, 3, D3DXMESH_MANAGED, decl3, device, &d3dxmesh);
    ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    IDirect3DDevice9_Release(device);
    IDirect3D9_Release(d3d);
    DestroyWindow(wnd);
}

static void D3DXCreateMeshFVFTest(void)
{
    HRESULT hr;
    HWND wnd;
    IDirect3D9 *d3d;
    IDirect3DDevice9 *device, *test_device;
    D3DPRESENT_PARAMETERS d3dpp;
    ID3DXMesh *d3dxmesh;
    int i, size;
    D3DVERTEXELEMENT9 test_decl[MAX_FVF_DECL_SIZE];
    DWORD options;
    struct mesh mesh;

    static const D3DVERTEXELEMENT9 decl[3] = {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
        D3DDECL_END(), };

    hr = D3DXCreateMeshFVF(0, 0, 0, 0, NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateMeshFVF(1, 3, D3DXMESH_MANAGED, D3DFVF_XYZ | D3DFVF_NORMAL, NULL, &d3dxmesh);
    ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

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

    hr = D3DXCreateMeshFVF(0, 3, D3DXMESH_MANAGED, D3DFVF_XYZ | D3DFVF_NORMAL, device, &d3dxmesh);
    ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateMeshFVF(1, 0, D3DXMESH_MANAGED, D3DFVF_XYZ | D3DFVF_NORMAL, device, &d3dxmesh);
    ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateMeshFVF(1, 3, 0, D3DFVF_XYZ | D3DFVF_NORMAL, device, &d3dxmesh);
    ok(hr == D3D_OK, "Got result %x, expected %x (D3D_OK)\n", hr, D3D_OK);

    if (hr == D3D_OK)
    {
        d3dxmesh->lpVtbl->Release(d3dxmesh);
    }

    hr = D3DXCreateMeshFVF(1, 3, D3DXMESH_MANAGED, 0xdeadbeef, device, &d3dxmesh);
    ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateMeshFVF(1, 3, D3DXMESH_MANAGED, D3DFVF_XYZ | D3DFVF_NORMAL, device, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateMeshFVF(1, 3, D3DXMESH_MANAGED, D3DFVF_XYZ | D3DFVF_NORMAL, device, &d3dxmesh);
    ok(hr == D3D_OK, "Got result %x, expected 0 (D3D_OK)\n", hr);

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
        hr = d3dxmesh->lpVtbl->GetDeclaration(d3dxmesh, NULL);
        ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

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
                ok(test_decl[i].UsageIndex == decl[i].UsageIndex, "Returned usage index %d, expected %d\n",
                   test_decl[i].UsageIndex, decl[i].UsageIndex);
                ok(test_decl[i].Offset == decl[i].Offset, "Returned offset %d, expected %d\n", test_decl[i].Offset, decl[i].Offset);
            }
            ok(decl[size-1].Stream == 0xFF, "Returned too long vertex declaration\n"); /* end element */
        }

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
            mesh.fvf = D3DFVF_XYZ | D3DFVF_NORMAL;

            compare_mesh("createmeshfvf", d3dxmesh, &mesh);

            free_mesh(&mesh);
        }

        d3dxmesh->lpVtbl->Release(d3dxmesh);
    }

    IDirect3DDevice9_Release(device);
    IDirect3D9_Release(d3d);
    DestroyWindow(wnd);
}

#define check_vertex_buffer(mesh, vertices, num_vertices, fvf) \
    check_vertex_buffer_(__LINE__, mesh, vertices, num_vertices, fvf)
static void check_vertex_buffer_(int line, ID3DXMesh *mesh, const void *vertices, DWORD num_vertices, DWORD fvf)
{
    DWORD mesh_num_vertices = mesh->lpVtbl->GetNumVertices(mesh);
    DWORD mesh_fvf = mesh->lpVtbl->GetFVF(mesh);
    const void *mesh_vertices;
    HRESULT hr;

    ok_(__FILE__,line)(fvf == mesh_fvf, "expected FVF %x, got %x\n", fvf, mesh_fvf);
    ok_(__FILE__,line)(num_vertices == mesh_num_vertices,
       "Expected %u vertices, got %u\n", num_vertices, mesh_num_vertices);

    hr = mesh->lpVtbl->LockVertexBuffer(mesh, D3DLOCK_READONLY, (void**)&mesh_vertices);
    ok_(__FILE__,line)(hr == D3D_OK, "LockVertexBuffer returned %x, expected %x (D3D_OK)\n", hr, D3D_OK);
    if (FAILED(hr))
        return;

    if (mesh_fvf == fvf) {
        DWORD vertex_size = D3DXGetFVFVertexSize(fvf);
        int i;
        for (i = 0; i < min(num_vertices, mesh_num_vertices); i++)
        {
            const FLOAT *exp_float = vertices;
            const FLOAT *got_float = mesh_vertices;
            DWORD texcount;
            DWORD pos_dim = 0;
            int j;
            BOOL last_beta_dword = FALSE;
            char prefix[128];

            switch (fvf & D3DFVF_POSITION_MASK) {
                case D3DFVF_XYZ: pos_dim = 3; break;
                case D3DFVF_XYZRHW: pos_dim = 4; break;
                case D3DFVF_XYZB1:
                case D3DFVF_XYZB2:
                case D3DFVF_XYZB3:
                case D3DFVF_XYZB4:
                case D3DFVF_XYZB5:
                    pos_dim = (fvf & D3DFVF_POSITION_MASK) - D3DFVF_XYZB1 + 1;
                    if (fvf & (D3DFVF_LASTBETA_UBYTE4 | D3DFVF_LASTBETA_D3DCOLOR))
                    {
                        pos_dim--;
                        last_beta_dword = TRUE;
                    }
                    break;
                case D3DFVF_XYZW: pos_dim = 4; break;
            }
            sprintf(prefix, "vertex[%u] position, ", i);
            check_floats_(line, prefix, got_float, exp_float, pos_dim);
            exp_float += pos_dim;
            got_float += pos_dim;

            if (last_beta_dword) {
                ok_(__FILE__,line)(*(DWORD*)exp_float == *(DWORD*)got_float,
                    "Vertex[%u]: Expected last beta %08x, got %08x\n", i, *(DWORD*)exp_float, *(DWORD*)got_float);
                exp_float++;
                got_float++;
            }

            if (fvf & D3DFVF_NORMAL) {
                sprintf(prefix, "vertex[%u] normal, ", i);
                check_floats_(line, prefix, got_float, exp_float, 3);
                exp_float += 3;
                got_float += 3;
            }
            if (fvf & D3DFVF_PSIZE) {
                ok_(__FILE__,line)(compare(*exp_float, *got_float),
                        "Vertex[%u]: Expected psize %g, got %g\n", i, *exp_float, *got_float);
                exp_float++;
                got_float++;
            }
            if (fvf & D3DFVF_DIFFUSE) {
                ok_(__FILE__,line)(*(DWORD*)exp_float == *(DWORD*)got_float,
                    "Vertex[%u]: Expected diffuse %08x, got %08x\n", i, *(DWORD*)exp_float, *(DWORD*)got_float);
                exp_float++;
                got_float++;
            }
            if (fvf & D3DFVF_SPECULAR) {
                ok_(__FILE__,line)(*(DWORD*)exp_float == *(DWORD*)got_float,
                    "Vertex[%u]: Expected specular %08x, got %08x\n", i, *(DWORD*)exp_float, *(DWORD*)got_float);
                exp_float++;
                got_float++;
            }

            texcount = (fvf & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
            for (j = 0; j < texcount; j++) {
                DWORD dim = (((fvf >> (16 + 2 * j)) + 1) & 0x03) + 1;
                sprintf(prefix, "vertex[%u] texture, ", i);
                check_floats_(line, prefix, got_float, exp_float, dim);
                exp_float += dim;
                got_float += dim;
            }

            vertices = (BYTE*)vertices + vertex_size;
            mesh_vertices = (BYTE*)mesh_vertices + vertex_size;
        }
    }

    mesh->lpVtbl->UnlockVertexBuffer(mesh);
}

#define check_index_buffer(mesh, indices, num_indices, index_size) \
    check_index_buffer_(__LINE__, mesh, indices, num_indices, index_size)
static void check_index_buffer_(int line, ID3DXMesh *mesh, const void *indices, DWORD num_indices, DWORD index_size)
{
    DWORD mesh_index_size = (mesh->lpVtbl->GetOptions(mesh) & D3DXMESH_32BIT) ? 4 : 2;
    DWORD mesh_num_indices = mesh->lpVtbl->GetNumFaces(mesh) * 3;
    const void *mesh_indices;
    HRESULT hr;
    DWORD i;

    ok_(__FILE__,line)(index_size == mesh_index_size,
        "Expected index size %u, got %u\n", index_size, mesh_index_size);
    ok_(__FILE__,line)(num_indices == mesh_num_indices,
        "Expected %u indices, got %u\n", num_indices, mesh_num_indices);

    hr = mesh->lpVtbl->LockIndexBuffer(mesh, D3DLOCK_READONLY, (void**)&mesh_indices);
    ok_(__FILE__,line)(hr == D3D_OK, "LockIndexBuffer returned %x, expected %x (D3D_OK)\n", hr, D3D_OK);
    if (FAILED(hr))
        return;

    if (mesh_index_size == index_size) {
        for (i = 0; i < min(num_indices, mesh_num_indices); i++)
        {
            if (index_size == 4)
                ok_(__FILE__,line)(*(DWORD*)indices == *(DWORD*)mesh_indices,
                    "Index[%u]: expected %u, got %u\n", i, *(DWORD*)indices, *(DWORD*)mesh_indices);
            else
                ok_(__FILE__,line)(*(WORD*)indices == *(WORD*)mesh_indices,
                    "Index[%u]: expected %u, got %u\n", i, *(WORD*)indices, *(WORD*)mesh_indices);
            indices = (BYTE*)indices + index_size;
            mesh_indices = (BYTE*)mesh_indices + index_size;
        }
    }
    mesh->lpVtbl->UnlockIndexBuffer(mesh);
}

#define check_matrix(got, expected) check_matrix_(__LINE__, got, expected)
static void check_matrix_(int line, const D3DXMATRIX *got, const D3DXMATRIX *expected)
{
    int i, j;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            ok_(__FILE__,line)(compare(U(*expected).m[i][j], U(*got).m[i][j]),
                    "matrix[%u][%u]: expected %g, got %g\n",
                    i, j, U(*expected).m[i][j], U(*got).m[i][j]);
        }
    }
}

static void check_colorvalue_(int line, const char *prefix, const D3DCOLORVALUE got, const D3DCOLORVALUE expected)
{
    ok_(__FILE__,line)(expected.r == got.r && expected.g == got.g && expected.b == got.b && expected.a == got.a,
            "%sExpected (%g, %g, %g, %g), got (%g, %g, %g, %g)\n", prefix,
            expected.r, expected.g, expected.b, expected.a, got.r, got.g, got.b, got.a);
}

#define check_materials(got, got_count, expected, expected_count) \
    check_materials_(__LINE__, got, got_count, expected, expected_count)
static void check_materials_(int line, const D3DXMATERIAL *got, DWORD got_count, const D3DXMATERIAL *expected, DWORD expected_count)
{
    int i;
    ok_(__FILE__,line)(expected_count == got_count, "Expected %u materials, got %u\n", expected_count, got_count);
    if (!expected) {
        ok_(__FILE__,line)(got == NULL, "Expected NULL material ptr, got %p\n", got);
        return;
    }
    for (i = 0; i < min(expected_count, got_count); i++)
    {
        if (!expected[i].pTextureFilename)
            ok_(__FILE__,line)(got[i].pTextureFilename == NULL,
                    "Expected NULL pTextureFilename, got %p\n", got[i].pTextureFilename);
        else
            ok_(__FILE__,line)(!strcmp(expected[i].pTextureFilename, got[i].pTextureFilename),
                    "Expected '%s' for pTextureFilename, got '%s'\n", expected[i].pTextureFilename, got[i].pTextureFilename);
        check_colorvalue_(line, "Diffuse: ", got[i].MatD3D.Diffuse, expected[i].MatD3D.Diffuse);
        check_colorvalue_(line, "Ambient: ", got[i].MatD3D.Ambient, expected[i].MatD3D.Ambient);
        check_colorvalue_(line, "Specular: ", got[i].MatD3D.Specular, expected[i].MatD3D.Specular);
        check_colorvalue_(line, "Emissive: ", got[i].MatD3D.Emissive, expected[i].MatD3D.Emissive);
        ok_(__FILE__,line)(expected[i].MatD3D.Power == got[i].MatD3D.Power,
                "Power: Expected %g, got %g\n", expected[i].MatD3D.Power, got[i].MatD3D.Power);
    }
}

#define check_generated_adjacency(mesh, got, epsilon) check_generated_adjacency_(__LINE__, mesh, got, epsilon)
static void check_generated_adjacency_(int line, ID3DXMesh *mesh, const DWORD *got, FLOAT epsilon)
{
    DWORD *expected;
    DWORD num_faces = mesh->lpVtbl->GetNumFaces(mesh);
    HRESULT hr;

    expected = HeapAlloc(GetProcessHeap(), 0, num_faces * sizeof(DWORD) * 3);
    if (!expected) {
        skip_(__FILE__, line)("Out of memory\n");
        return;
    }
    hr = mesh->lpVtbl->GenerateAdjacency(mesh, epsilon, expected);
    ok_(__FILE__, line)(hr == D3D_OK, "Expected D3D_OK, got %#x\n", hr);
    if (SUCCEEDED(hr))
    {
        int i;
        for (i = 0; i < num_faces; i++)
        {
            ok_(__FILE__, line)(expected[i * 3] == got[i * 3] &&
                    expected[i * 3 + 1] == got[i * 3 + 1] &&
                    expected[i * 3 + 2] == got[i * 3 + 2],
                    "Face %u adjacencies: Expected (%u, %u, %u), got (%u, %u, %u)\n", i,
                    expected[i * 3], expected[i * 3 + 1], expected[i * 3 + 2],
                    got[i * 3], got[i * 3 + 1], got[i * 3 + 2]);
        }
    }
    HeapFree(GetProcessHeap(), 0, expected);
}

#define check_generated_effects(materials, num_materials, effects) \
    check_generated_effects_(__LINE__, materials, num_materials, effects)
static void check_generated_effects_(int line, const D3DXMATERIAL *materials, DWORD num_materials, const D3DXEFFECTINSTANCE *effects)
{
    int i;
    static const struct {
        const char *name;
        DWORD name_size;
        DWORD num_bytes;
        DWORD value_offset;
    } params[] = {
#define EFFECT_TABLE_ENTRY(str, field) \
    {str, sizeof(str), sizeof(materials->MatD3D.field), offsetof(D3DXMATERIAL, MatD3D.field)}
        EFFECT_TABLE_ENTRY("Diffuse", Diffuse),
        EFFECT_TABLE_ENTRY("Power", Power),
        EFFECT_TABLE_ENTRY("Specular", Specular),
        EFFECT_TABLE_ENTRY("Emissive", Emissive),
        EFFECT_TABLE_ENTRY("Ambient", Ambient),
#undef EFFECT_TABLE_ENTRY
    };

    if (!num_materials) {
        ok_(__FILE__, line)(effects == NULL, "Expected NULL effects, got %p\n", effects);
        return;
    }
    for (i = 0; i < num_materials; i++)
    {
        int j;
        DWORD expected_num_defaults = ARRAY_SIZE(params) + (materials[i].pTextureFilename ? 1 : 0);

        ok_(__FILE__,line)(expected_num_defaults == effects[i].NumDefaults,
                "effect[%u] NumDefaults: Expected %u, got %u\n", i,
                expected_num_defaults, effects[i].NumDefaults);
        for (j = 0; j < min(ARRAY_SIZE(params), effects[i].NumDefaults); j++)
        {
            int k;
            D3DXEFFECTDEFAULT *got_param = &effects[i].pDefaults[j];
            ok_(__FILE__,line)(!strcmp(params[j].name, got_param->pParamName),
               "effect[%u].pDefaults[%u].pParamName: Expected '%s', got '%s'\n", i, j,
               params[j].name, got_param->pParamName);
            ok_(__FILE__,line)(D3DXEDT_FLOATS == got_param->Type,
               "effect[%u].pDefaults[%u].Type: Expected %u, got %u\n", i, j,
               D3DXEDT_FLOATS, got_param->Type);
            ok_(__FILE__,line)(params[j].num_bytes == got_param->NumBytes,
               "effect[%u].pDefaults[%u].NumBytes: Expected %u, got %u\n", i, j,
               params[j].num_bytes, got_param->NumBytes);
            for (k = 0; k < min(params[j].num_bytes, got_param->NumBytes) / 4; k++)
            {
                FLOAT expected = ((FLOAT*)((BYTE*)&materials[i] + params[j].value_offset))[k];
                FLOAT got = ((FLOAT*)got_param->pValue)[k];
                ok_(__FILE__,line)(compare(expected, got),
                   "effect[%u].pDefaults[%u] float value %u: Expected %g, got %g\n", i, j, k, expected, got);
            }
        }
        if (effects[i].NumDefaults > ARRAY_SIZE(params)) {
            D3DXEFFECTDEFAULT *got_param = &effects[i].pDefaults[j];
            static const char *expected_name = "Texture0@Name";

            ok_(__FILE__,line)(!strcmp(expected_name, got_param->pParamName),
               "effect[%u].pDefaults[%u].pParamName: Expected '%s', got '%s'\n", i, j,
               expected_name, got_param->pParamName);
            ok_(__FILE__,line)(D3DXEDT_STRING == got_param->Type,
               "effect[%u].pDefaults[%u].Type: Expected %u, got %u\n", i, j,
               D3DXEDT_STRING, got_param->Type);
            if (materials[i].pTextureFilename) {
                ok_(__FILE__,line)(strlen(materials[i].pTextureFilename) + 1 == got_param->NumBytes,
                   "effect[%u] texture filename length: Expected %u, got %u\n", i,
                   (DWORD)strlen(materials[i].pTextureFilename) + 1, got_param->NumBytes);
                ok_(__FILE__,line)(!strcmp(materials[i].pTextureFilename, got_param->pValue),
                   "effect[%u] texture filename: Expected '%s', got '%s'\n", i,
                   materials[i].pTextureFilename, (char*)got_param->pValue);
            }
        }
    }
}

static LPSTR strdupA(LPCSTR p)
{
    LPSTR ret;
    if (!p) return NULL;
    ret = HeapAlloc(GetProcessHeap(), 0, strlen(p) + 1);
    if (ret) strcpy(ret, p);
    return ret;
}

static CALLBACK HRESULT ID3DXAllocateHierarchyImpl_DestroyFrame(ID3DXAllocateHierarchy *iface, LPD3DXFRAME frame)
{
    TRACECALLBACK("ID3DXAllocateHierarchyImpl_DestroyFrame(%p, %p)\n", iface, frame);
    if (frame) {
        HeapFree(GetProcessHeap(), 0, frame->Name);
        HeapFree(GetProcessHeap(), 0, frame);
    }
    return D3D_OK;
}

static CALLBACK HRESULT ID3DXAllocateHierarchyImpl_CreateFrame(ID3DXAllocateHierarchy *iface, LPCSTR name, LPD3DXFRAME *new_frame)
{
    LPD3DXFRAME frame;

    TRACECALLBACK("ID3DXAllocateHierarchyImpl_CreateFrame(%p, '%s', %p)\n", iface, name, new_frame);
    frame = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*frame));
    if (!frame)
        return E_OUTOFMEMORY;
    if (name) {
        frame->Name = strdupA(name);
        if (!frame->Name) {
            HeapFree(GetProcessHeap(), 0, frame);
            return E_OUTOFMEMORY;
        }
    }
    *new_frame = frame;
    return D3D_OK;
}

static HRESULT destroy_mesh_container(LPD3DXMESHCONTAINER mesh_container)
{
    int i;

    if (!mesh_container)
        return D3D_OK;
    HeapFree(GetProcessHeap(), 0, mesh_container->Name);
    if (U(mesh_container->MeshData).pMesh)
        IUnknown_Release(U(mesh_container->MeshData).pMesh);
    if (mesh_container->pMaterials) {
        for (i = 0; i < mesh_container->NumMaterials; i++)
            HeapFree(GetProcessHeap(), 0, mesh_container->pMaterials[i].pTextureFilename);
        HeapFree(GetProcessHeap(), 0, mesh_container->pMaterials);
    }
    if (mesh_container->pEffects) {
        for (i = 0; i < mesh_container->NumMaterials; i++) {
            HeapFree(GetProcessHeap(), 0, mesh_container->pEffects[i].pEffectFilename);
            if (mesh_container->pEffects[i].pDefaults) {
                int j;
                for (j = 0; j < mesh_container->pEffects[i].NumDefaults; j++) {
                    HeapFree(GetProcessHeap(), 0, mesh_container->pEffects[i].pDefaults[j].pParamName);
                    HeapFree(GetProcessHeap(), 0, mesh_container->pEffects[i].pDefaults[j].pValue);
                }
                HeapFree(GetProcessHeap(), 0, mesh_container->pEffects[i].pDefaults);
            }
        }
        HeapFree(GetProcessHeap(), 0, mesh_container->pEffects);
    }
    HeapFree(GetProcessHeap(), 0, mesh_container->pAdjacency);
    if (mesh_container->pSkinInfo)
        IUnknown_Release(mesh_container->pSkinInfo);
    HeapFree(GetProcessHeap(), 0, mesh_container);
    return D3D_OK;
}

static CALLBACK HRESULT ID3DXAllocateHierarchyImpl_DestroyMeshContainer(ID3DXAllocateHierarchy *iface, LPD3DXMESHCONTAINER mesh_container)
{
    TRACECALLBACK("ID3DXAllocateHierarchyImpl_DestroyMeshContainer(%p, %p)\n", iface, mesh_container);
    return destroy_mesh_container(mesh_container);
}

static CALLBACK HRESULT ID3DXAllocateHierarchyImpl_CreateMeshContainer(ID3DXAllocateHierarchy *iface,
        LPCSTR name, CONST D3DXMESHDATA *mesh_data, CONST D3DXMATERIAL *materials,
        CONST D3DXEFFECTINSTANCE *effects, DWORD num_materials, CONST DWORD *adjacency,
        LPD3DXSKININFO skin_info, LPD3DXMESHCONTAINER *new_mesh_container)
{
    LPD3DXMESHCONTAINER mesh_container = NULL;
    int i;

    TRACECALLBACK("ID3DXAllocateHierarchyImpl_CreateMeshContainer(%p, '%s', %u, %p, %p, %p, %d, %p, %p, %p)\n",
            iface, name, mesh_data->Type, U(*mesh_data).pMesh, materials, effects,
            num_materials, adjacency, skin_info, *new_mesh_container);

    mesh_container = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*mesh_container));
    if (!mesh_container)
        return E_OUTOFMEMORY;

    if (name) {
        mesh_container->Name = strdupA(name);
        if (!mesh_container->Name)
            goto error;
    }

    mesh_container->NumMaterials = num_materials;
    if (num_materials) {
        mesh_container->pMaterials = HeapAlloc(GetProcessHeap(), 0, num_materials * sizeof(*materials));
        if (!mesh_container->pMaterials)
            goto error;

        memcpy(mesh_container->pMaterials, materials, num_materials * sizeof(*materials));
        for (i = 0; i < num_materials; i++)
            mesh_container->pMaterials[i].pTextureFilename = NULL;
        for (i = 0; i < num_materials; i++) {
            if (materials[i].pTextureFilename) {
                mesh_container->pMaterials[i].pTextureFilename = strdupA(materials[i].pTextureFilename);
                if (!mesh_container->pMaterials[i].pTextureFilename)
                    goto error;
            }
        }

        mesh_container->pEffects = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, num_materials * sizeof(*effects));
        if (!mesh_container->pEffects)
            goto error;
        for (i = 0; i < num_materials; i++) {
            int j;
            const D3DXEFFECTINSTANCE *effect_src = &effects[i];
            D3DXEFFECTINSTANCE *effect_dest = &mesh_container->pEffects[i];

            if (effect_src->pEffectFilename) {
                effect_dest->pEffectFilename = strdupA(effect_src->pEffectFilename);
                if (!effect_dest->pEffectFilename)
                    goto error;
            }
            effect_dest->pDefaults = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                    effect_src->NumDefaults * sizeof(*effect_src->pDefaults));
            if (!effect_dest->pDefaults)
                goto error;
            effect_dest->NumDefaults = effect_src->NumDefaults;
            for (j = 0; j < effect_src->NumDefaults; j++) {
                const D3DXEFFECTDEFAULT *default_src = &effect_src->pDefaults[j];
                D3DXEFFECTDEFAULT *default_dest = &effect_dest->pDefaults[j];

                if (default_src->pParamName) {
                    default_dest->pParamName = strdupA(default_src->pParamName);
                    if (!default_dest->pParamName)
                        goto error;
                }
                default_dest->NumBytes = default_src->NumBytes;
                default_dest->Type = default_src->Type;
                default_dest->pValue = HeapAlloc(GetProcessHeap(), 0, default_src->NumBytes);
                memcpy(default_dest->pValue, default_src->pValue, default_src->NumBytes);
            }
        }
    }

    ok(adjacency != NULL, "Expected non-NULL adjacency, got NULL\n");
    if (adjacency) {
        if (mesh_data->Type == D3DXMESHTYPE_MESH || mesh_data->Type == D3DXMESHTYPE_PMESH) {
            ID3DXBaseMesh *basemesh = (ID3DXBaseMesh*)U(*mesh_data).pMesh;
            DWORD num_faces = basemesh->lpVtbl->GetNumFaces(basemesh);
            size_t size = num_faces * sizeof(DWORD) * 3;
            mesh_container->pAdjacency = HeapAlloc(GetProcessHeap(), 0, size);
            if (!mesh_container->pAdjacency)
                goto error;
            memcpy(mesh_container->pAdjacency, adjacency, size);
        } else {
            ok(mesh_data->Type == D3DXMESHTYPE_PATCHMESH, "Unknown mesh type %u\n", mesh_data->Type);
            if (mesh_data->Type == D3DXMESHTYPE_PATCHMESH)
                trace("FIXME: copying adjacency data for patch mesh not implemented\n");
        }
    }

    memcpy(&mesh_container->MeshData, mesh_data, sizeof(*mesh_data));
    if (U(*mesh_data).pMesh)
        IUnknown_AddRef(U(*mesh_data).pMesh);
    if (skin_info) {
        mesh_container->pSkinInfo = skin_info;
        skin_info->lpVtbl->AddRef(skin_info);
    }
    *new_mesh_container = mesh_container;

    return S_OK;
error:
    destroy_mesh_container(mesh_container);
    return E_OUTOFMEMORY;
}

static ID3DXAllocateHierarchyVtbl ID3DXAllocateHierarchyImpl_Vtbl = {
    ID3DXAllocateHierarchyImpl_CreateFrame,
    ID3DXAllocateHierarchyImpl_CreateMeshContainer,
    ID3DXAllocateHierarchyImpl_DestroyFrame,
    ID3DXAllocateHierarchyImpl_DestroyMeshContainer,
};
static ID3DXAllocateHierarchy alloc_hier = { &ID3DXAllocateHierarchyImpl_Vtbl };

#define test_LoadMeshFromX(device, xfile_str, vertex_array, fvf, index_array, materials_array, check_adjacency) \
    test_LoadMeshFromX_(__LINE__, device, xfile_str, sizeof(xfile_str) - 1, vertex_array, ARRAY_SIZE(vertex_array), fvf, \
            index_array, ARRAY_SIZE(index_array), sizeof(*index_array), materials_array, ARRAY_SIZE(materials_array), \
            check_adjacency);
static void test_LoadMeshFromX_(int line, IDirect3DDevice9 *device, const char *xfile_str, size_t xfile_strlen,
        const void *vertices, DWORD num_vertices, DWORD fvf, const void *indices, DWORD num_indices, size_t index_size,
        const D3DXMATERIAL *expected_materials, DWORD expected_num_materials, BOOL check_adjacency)
{
    HRESULT hr;
    ID3DXBuffer *materials = NULL;
    ID3DXBuffer *effects = NULL;
    ID3DXBuffer *adjacency = NULL;
    ID3DXMesh *mesh = NULL;
    DWORD num_materials = 0;

    /* Adjacency is not checked when the X file contains multiple meshes,
     * since calling GenerateAdjacency on the merged mesh is not equivalent
     * to calling GenerateAdjacency on the individual meshes and then merging
     * the adjacency data. */
    hr = D3DXLoadMeshFromXInMemory(xfile_str, xfile_strlen, D3DXMESH_MANAGED, device,
            check_adjacency ? &adjacency : NULL, &materials, &effects, &num_materials, &mesh);
    ok_(__FILE__,line)(hr == D3D_OK, "Expected D3D_OK, got %#x\n", hr);
    if (SUCCEEDED(hr)) {
        D3DXMATERIAL *materials_ptr = materials ? ID3DXBuffer_GetBufferPointer(materials) : NULL;
        D3DXEFFECTINSTANCE *effects_ptr = effects ? ID3DXBuffer_GetBufferPointer(effects) : NULL;
        DWORD *adjacency_ptr = check_adjacency ? ID3DXBuffer_GetBufferPointer(adjacency) : NULL;

        check_vertex_buffer_(line, mesh, vertices, num_vertices, fvf);
        check_index_buffer_(line, mesh, indices, num_indices, index_size);
        check_materials_(line, materials_ptr, num_materials, expected_materials, expected_num_materials);
        check_generated_effects_(line, materials_ptr, num_materials, effects_ptr);
        if (check_adjacency)
            check_generated_adjacency_(line, mesh, adjacency_ptr, 0.0f);

        if (materials) ID3DXBuffer_Release(materials);
        if (effects) ID3DXBuffer_Release(effects);
        if (adjacency) ID3DXBuffer_Release(adjacency);
        IUnknown_Release(mesh);
    }
}

static void D3DXLoadMeshTest(void)
{
    static const char empty_xfile[] = "xof 0303txt 0032";
    /*________________________*/
    static const char simple_xfile[] =
        "xof 0303txt 0032"
        "Mesh {"
            "3;"
            "0.0; 0.0; 0.0;,"
            "0.0; 1.0; 0.0;,"
            "1.0; 1.0; 0.0;;"
            "1;"
            "3; 0, 1, 2;;"
        "}";
    static const WORD simple_index_buffer[] = {0, 1, 2};
    static const D3DXVECTOR3 simple_vertex_buffer[] = {
        {0.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {1.0, 1.0, 0.0}
    };
    const DWORD simple_fvf = D3DFVF_XYZ;
    static const char framed_xfile[] =
        "xof 0303txt 0032"
        "Frame {"
            "Mesh { 3; 0.0; 0.0; 0.0;, 0.0; 1.0; 0.0;, 1.0; 1.0; 0.0;; 1; 3; 0, 1, 2;; }"
            "FrameTransformMatrix {" /* translation (0.0, 0.0, 2.0) */
              "1.0, 0.0, 0.0, 0.0,"
              "0.0, 1.0, 0.0, 0.0,"
              "0.0, 0.0, 1.0, 0.0,"
              "0.0, 0.0, 2.0, 1.0;;"
            "}"
            "Mesh { 3; 0.0; 0.0; 0.0;, 0.0; 1.0; 0.0;, 2.0; 1.0; 0.0;; 1; 3; 0, 1, 2;; }"
            "FrameTransformMatrix {" /* translation (0.0, 0.0, 3.0) */
              "1.0, 0.0, 0.0, 0.0,"
              "0.0, 1.0, 0.0, 0.0,"
              "0.0, 0.0, 1.0, 0.0,"
              "0.0, 0.0, 3.0, 1.0;;"
            "}"
            "Mesh { 3; 0.0; 0.0; 0.0;, 0.0; 1.0; 0.0;, 3.0; 1.0; 0.0;; 1; 3; 0, 1, 2;; }"
        "}";
    static const WORD framed_index_buffer[] = { 0, 1, 2 };
    static const D3DXVECTOR3 framed_vertex_buffers[3][3] = {
        {{0.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {1.0, 1.0, 0.0}},
        {{0.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {2.0, 1.0, 0.0}},
        {{0.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {3.0, 1.0, 0.0}},
    };
    static const WORD merged_index_buffer[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
    /* frame transforms accumulates for D3DXLoadMeshFromX */
    static const D3DXVECTOR3 merged_vertex_buffer[] = {
        {0.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {1.0, 1.0, 0.0},
        {0.0, 0.0, 2.0}, {0.0, 1.0, 2.0}, {2.0, 1.0, 2.0},
        {0.0, 0.0, 5.0}, {0.0, 1.0, 5.0}, {3.0, 1.0, 5.0},
    };
    const DWORD framed_fvf = D3DFVF_XYZ;
    /*________________________*/
    static const char box_xfile[] =
        "xof 0303txt 0032"
        "Mesh {"
            "8;" /* DWORD nVertices; */
            /* array Vector vertices[nVertices]; */
            "0.0; 0.0; 0.0;,"
            "0.0; 0.0; 1.0;,"
            "0.0; 1.0; 0.0;,"
            "0.0; 1.0; 1.0;,"
            "1.0; 0.0; 0.0;,"
            "1.0; 0.0; 1.0;,"
            "1.0; 1.0; 0.0;,"
            "1.0; 1.0; 1.0;;"
            "6;" /* DWORD nFaces; */
            /* array MeshFace faces[nFaces]; */
            "4; 0, 1, 3, 2;," /* (left side) */
            "4; 2, 3, 7, 6;," /* (top side) */
            "4; 6, 7, 5, 4;," /* (right side) */
            "4; 1, 0, 4, 5;," /* (bottom side) */
            "4; 1, 5, 7, 3;," /* (back side) */
            "4; 0, 2, 6, 4;;" /* (front side) */
            "MeshNormals {"
              "6;" /* DWORD nNormals; */
              /* array Vector normals[nNormals]; */
              "-1.0; 0.0; 0.0;,"
              "0.0; 1.0; 0.0;,"
              "1.0; 0.0; 0.0;,"
              "0.0; -1.0; 0.0;,"
              "0.0; 0.0; 1.0;,"
              "0.0; 0.0; -1.0;;"
              "6;" /* DWORD nFaceNormals; */
              /* array MeshFace faceNormals[nFaceNormals]; */
              "4; 0, 0, 0, 0;,"
              "4; 1, 1, 1, 1;,"
              "4; 2, 2, 2, 2;,"
              "4; 3, 3, 3, 3;,"
              "4; 4, 4, 4, 4;,"
              "4; 5, 5, 5, 5;;"
            "}"
            "MeshMaterialList materials {"
              "2;" /* DWORD nMaterials; */
              "6;" /* DWORD nFaceIndexes; */
              /* array DWORD faceIndexes[nFaceIndexes]; */
              "0, 0, 0, 1, 1, 1;;"
              "Material {"
                /* ColorRGBA faceColor; */
                "0.0; 0.0; 1.0; 1.0;;"
                /* FLOAT power; */
                "0.5;"
                /* ColorRGB specularColor; */
                "1.0; 1.0; 1.0;;"
                /* ColorRGB emissiveColor; */
                "0.0; 0.0; 0.0;;"
              "}"
              "Material {"
                /* ColorRGBA faceColor; */
                "1.0; 1.0; 1.0; 1.0;;"
                /* FLOAT power; */
                "1.0;"
                /* ColorRGB specularColor; */
                "1.0; 1.0; 1.0;;"
                /* ColorRGB emissiveColor; */
                "0.0; 0.0; 0.0;;"
                "TextureFilename { \"texture.jpg\"; }"
              "}"
            "}"
            "MeshVertexColors {"
              "8;" /* DWORD nVertexColors; */
              /* array IndexedColor vertexColors[nVertexColors]; */
              "0; 0.0; 0.0; 0.0; 0.0;;"
              "1; 0.0; 0.0; 1.0; 0.1;;"
              "2; 0.0; 1.0; 0.0; 0.2;;"
              "3; 0.0; 1.0; 1.0; 0.3;;"
              "4; 1.0; 0.0; 0.0; 0.4;;"
              "5; 1.0; 0.0; 1.0; 0.5;;"
              "6; 1.0; 1.0; 0.0; 0.6;;"
              "7; 1.0; 1.0; 1.0; 0.7;;"
            "}"
            "MeshTextureCoords {"
              "8;" /* DWORD nTextureCoords; */
              /* array Coords2d textureCoords[nTextureCoords]; */
              "0.0; 1.0;,"
              "1.0; 1.0;,"
              "0.0; 0.0;,"
              "1.0; 0.0;,"
              "1.0; 1.0;,"
              "0.0; 1.0;,"
              "1.0; 0.0;,"
              "0.0; 0.0;;"
            "}"
          "}";
    static const WORD box_index_buffer[] = {
        0, 1, 3,
        0, 3, 2,
        8, 9, 7,
        8, 7, 6,
        10, 11, 5,
        10, 5, 4,
        12, 13, 14,
        12, 14, 15,
        16, 17, 18,
        16, 18, 19,
        20, 21, 22,
        20, 22, 23,
    };
    static const struct {
        D3DXVECTOR3 position;
        D3DXVECTOR3 normal;
        D3DCOLOR diffuse;
        D3DXVECTOR2 tex_coords;
    } box_vertex_buffer[] = {
        {{0.0, 0.0, 0.0}, {-1.0, 0.0, 0.0}, 0x00000000, {0.0, 1.0}},
        {{0.0, 0.0, 1.0}, {-1.0, 0.0, 0.0}, 0x1a0000ff, {1.0, 1.0}},
        {{0.0, 1.0, 0.0}, {-1.0, 0.0, 0.0}, 0x3300ff00, {0.0, 0.0}},
        {{0.0, 1.0, 1.0}, {-1.0, 0.0, 0.0}, 0x4d00ffff, {1.0, 0.0}},
        {{1.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, 0x66ff0000, {1.0, 1.0}},
        {{1.0, 0.0, 1.0}, {1.0, 0.0, 0.0}, 0x80ff00ff, {0.0, 1.0}},
        {{1.0, 1.0, 0.0}, {0.0, 1.0, 0.0}, 0x99ffff00, {1.0, 0.0}},
        {{1.0, 1.0, 1.0}, {0.0, 1.0, 0.0}, 0xb3ffffff, {0.0, 0.0}},
        {{0.0, 1.0, 0.0}, {0.0, 1.0, 0.0}, 0x3300ff00, {0.0, 0.0}},
        {{0.0, 1.0, 1.0}, {0.0, 1.0, 0.0}, 0x4d00ffff, {1.0, 0.0}},
        {{1.0, 1.0, 0.0}, {1.0, 0.0, 0.0}, 0x99ffff00, {1.0, 0.0}},
        {{1.0, 1.0, 1.0}, {1.0, 0.0, 0.0}, 0xb3ffffff, {0.0, 0.0}},
        {{0.0, 0.0, 1.0}, {0.0, -1.0, 0.0}, 0x1a0000ff, {1.0, 1.0}},
        {{0.0, 0.0, 0.0}, {0.0, -1.0, 0.0}, 0x00000000, {0.0, 1.0}},
        {{1.0, 0.0, 0.0}, {0.0, -1.0, 0.0}, 0x66ff0000, {1.0, 1.0}},
        {{1.0, 0.0, 1.0}, {0.0, -1.0, 0.0}, 0x80ff00ff, {0.0, 1.0}},
        {{0.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, 0x1a0000ff, {1.0, 1.0}},
        {{1.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, 0x80ff00ff, {0.0, 1.0}},
        {{1.0, 1.0, 1.0}, {0.0, 0.0, 1.0}, 0xb3ffffff, {0.0, 0.0}},
        {{0.0, 1.0, 1.0}, {0.0, 0.0, 1.0}, 0x4d00ffff, {1.0, 0.0}},
        {{0.0, 0.0, 0.0}, {0.0, 0.0, -1.0}, 0x00000000, {0.0, 1.0}},
        {{0.0, 1.0, 0.0}, {0.0, 0.0, -1.0}, 0x3300ff00, {0.0, 0.0}},
        {{1.0, 1.0, 0.0}, {0.0, 0.0, -1.0}, 0x99ffff00, {1.0, 0.0}},
        {{1.0, 0.0, 0.0}, {0.0, 0.0, -1.0}, 0x66ff0000, {1.0, 1.0}},
    };
    static const D3DXMATERIAL box_materials[] = {
        {
            {
                {0.0, 0.0, 1.0, 1.0}, /* Diffuse */
                {0.0, 0.0, 0.0, 1.0}, /* Ambient */
                {1.0, 1.0, 1.0, 1.0}, /* Specular */
                {0.0, 0.0, 0.0, 1.0}, /* Emissive */
                0.5, /* Power */
            },
            NULL, /* pTextureFilename */
        },
        {
            {
                {1.0, 1.0, 1.0, 1.0}, /* Diffuse */
                {0.0, 0.0, 0.0, 1.0}, /* Ambient */
                {1.0, 1.0, 1.0, 1.0}, /* Specular */
                {0.0, 0.0, 0.0, 1.0}, /* Emissive */
                1.0, /* Power */
            },
            (char *)"texture.jpg", /* pTextureFilename */
        },
    };
    const DWORD box_fvf = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX1;
    /*________________________*/
    static const D3DXMATERIAL default_materials[] = {
        {
            {
                {0.5, 0.5, 0.5, 0.0}, /* Diffuse */
                {0.0, 0.0, 0.0, 0.0}, /* Ambient */
                {0.5, 0.5, 0.5, 0.0}, /* Specular */
                {0.0, 0.0, 0.0, 0.0}, /* Emissive */
                0.0, /* Power */
            },
            NULL, /* pTextureFilename */
        }
    };
    HRESULT hr;
    HWND wnd = NULL;
    IDirect3D9 *d3d = NULL;
    IDirect3DDevice9 *device = NULL;
    D3DPRESENT_PARAMETERS d3dpp;
    ID3DXMesh *mesh = NULL;
    D3DXFRAME *frame_hier = NULL;
    D3DXMATRIX transform;

    wnd = CreateWindow("static", "d3dx9_test", WS_POPUP, 0, 0, 1000, 1000, NULL, NULL, NULL, NULL);
    if (!wnd)
    {
        skip("Couldn't create application window\n");
        return;
    }
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!d3d)
    {
        skip("Couldn't create IDirect3D9 object\n");
        goto cleanup;
    }

    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    hr = IDirect3D9_CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, wnd, D3DCREATE_MIXED_VERTEXPROCESSING, &d3dpp, &device);
    if (FAILED(hr))
    {
        skip("Failed to create IDirect3DDevice9 object %#x\n", hr);
        goto cleanup;
    }

    hr = D3DXLoadMeshHierarchyFromXInMemory(NULL, sizeof(simple_xfile) - 1,
            D3DXMESH_MANAGED, device, &alloc_hier, NULL, &frame_hier, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#x\n", hr);

    hr = D3DXLoadMeshHierarchyFromXInMemory(simple_xfile, 0,
            D3DXMESH_MANAGED, device, &alloc_hier, NULL, &frame_hier, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#x\n", hr);

    hr = D3DXLoadMeshHierarchyFromXInMemory(simple_xfile, sizeof(simple_xfile) - 1,
            D3DXMESH_MANAGED, NULL, &alloc_hier, NULL, &frame_hier, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#x\n", hr);

    hr = D3DXLoadMeshHierarchyFromXInMemory(simple_xfile, sizeof(simple_xfile) - 1,
            D3DXMESH_MANAGED, device, NULL, NULL, &frame_hier, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#x\n", hr);

    hr = D3DXLoadMeshHierarchyFromXInMemory(empty_xfile, sizeof(empty_xfile) - 1,
            D3DXMESH_MANAGED, device, &alloc_hier, NULL, &frame_hier, NULL);
    ok(hr == E_FAIL, "Expected E_FAIL, got %#x\n", hr);

    hr = D3DXLoadMeshHierarchyFromXInMemory(simple_xfile, sizeof(simple_xfile) - 1,
            D3DXMESH_MANAGED, device, &alloc_hier, NULL, NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#x\n", hr);

    hr = D3DXLoadMeshHierarchyFromXInMemory(simple_xfile, sizeof(simple_xfile) - 1,
            D3DXMESH_MANAGED, device, &alloc_hier, NULL, &frame_hier, NULL);
    ok(hr == D3D_OK, "Expected D3D_OK, got %#x\n", hr);
    if (SUCCEEDED(hr)) {
        D3DXMESHCONTAINER *container = frame_hier->pMeshContainer;

        ok(frame_hier->Name == NULL, "Expected NULL, got '%s'\n", frame_hier->Name);
        D3DXMatrixIdentity(&transform);
        check_matrix(&frame_hier->TransformationMatrix, &transform);

        ok(!strcmp(container->Name, ""), "Expected '', got '%s'\n", container->Name);
        ok(container->MeshData.Type == D3DXMESHTYPE_MESH, "Expected %d, got %d\n",
           D3DXMESHTYPE_MESH, container->MeshData.Type);
        mesh = U(container->MeshData).pMesh;
        check_vertex_buffer(mesh, simple_vertex_buffer, ARRAY_SIZE(simple_vertex_buffer), simple_fvf);
        check_index_buffer(mesh, simple_index_buffer, ARRAY_SIZE(simple_index_buffer), sizeof(*simple_index_buffer));
        check_materials(container->pMaterials, container->NumMaterials, NULL, 0);
        check_generated_effects(container->pMaterials, container->NumMaterials, container->pEffects);
        check_generated_adjacency(mesh, container->pAdjacency, 0.0f);
        hr = D3DXFrameDestroy(frame_hier, &alloc_hier);
        ok(hr == D3D_OK, "Expected D3D_OK, got %#x\n", hr);
        frame_hier = NULL;
    }

    hr = D3DXLoadMeshHierarchyFromXInMemory(box_xfile, sizeof(box_xfile) - 1,
            D3DXMESH_MANAGED, device, &alloc_hier, NULL, &frame_hier, NULL);
    ok(hr == D3D_OK, "Expected D3D_OK, got %#x\n", hr);
    if (SUCCEEDED(hr)) {
        D3DXMESHCONTAINER *container = frame_hier->pMeshContainer;

        ok(frame_hier->Name == NULL, "Expected NULL, got '%s'\n", frame_hier->Name);
        D3DXMatrixIdentity(&transform);
        check_matrix(&frame_hier->TransformationMatrix, &transform);

        ok(!strcmp(container->Name, ""), "Expected '', got '%s'\n", container->Name);
        ok(container->MeshData.Type == D3DXMESHTYPE_MESH, "Expected %d, got %d\n",
           D3DXMESHTYPE_MESH, container->MeshData.Type);
        mesh = U(container->MeshData).pMesh;
        check_vertex_buffer(mesh, box_vertex_buffer, ARRAY_SIZE(box_vertex_buffer), box_fvf);
        check_index_buffer(mesh, box_index_buffer, ARRAY_SIZE(box_index_buffer), sizeof(*box_index_buffer));
        check_materials(container->pMaterials, container->NumMaterials, box_materials, ARRAY_SIZE(box_materials));
        check_generated_effects(container->pMaterials, container->NumMaterials, container->pEffects);
        check_generated_adjacency(mesh, container->pAdjacency, 0.0f);
        hr = D3DXFrameDestroy(frame_hier, &alloc_hier);
        ok(hr == D3D_OK, "Expected D3D_OK, got %#x\n", hr);
        frame_hier = NULL;
    }

    hr = D3DXLoadMeshHierarchyFromXInMemory(framed_xfile, sizeof(framed_xfile) - 1,
            D3DXMESH_MANAGED, device, &alloc_hier, NULL, &frame_hier, NULL);
    ok(hr == D3D_OK, "Expected D3D_OK, got %#x\n", hr);
    if (SUCCEEDED(hr)) {
        D3DXMESHCONTAINER *container = frame_hier->pMeshContainer;
        int i;

        ok(!strcmp(frame_hier->Name, ""), "Expected '', got '%s'\n", frame_hier->Name);
        /* last frame transform replaces the first */
        D3DXMatrixIdentity(&transform);
        U(transform).m[3][2] = 3.0;
        check_matrix(&frame_hier->TransformationMatrix, &transform);

        for (i = 0; i < 3; i++) {
            ok(!strcmp(container->Name, ""), "Expected '', got '%s'\n", container->Name);
            ok(container->MeshData.Type == D3DXMESHTYPE_MESH, "Expected %d, got %d\n",
               D3DXMESHTYPE_MESH, container->MeshData.Type);
            mesh = U(container->MeshData).pMesh;
            check_vertex_buffer(mesh, framed_vertex_buffers[i], ARRAY_SIZE(framed_vertex_buffers[0]), framed_fvf);
            check_index_buffer(mesh, framed_index_buffer, ARRAY_SIZE(framed_index_buffer), sizeof(*framed_index_buffer));
            check_materials(container->pMaterials, container->NumMaterials, NULL, 0);
            check_generated_effects(container->pMaterials, container->NumMaterials, container->pEffects);
            check_generated_adjacency(mesh, container->pAdjacency, 0.0f);
            container = container->pNextMeshContainer;
        }
        ok(container == NULL, "Expected NULL, got %p\n", container);
        hr = D3DXFrameDestroy(frame_hier, &alloc_hier);
        ok(hr == D3D_OK, "Expected D3D_OK, got %#x\n", hr);
        frame_hier = NULL;
    }


    hr = D3DXLoadMeshFromXInMemory(NULL, 0, D3DXMESH_MANAGED,
                                   device, NULL, NULL, NULL, NULL, &mesh);
    ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#x\n", hr);

    hr = D3DXLoadMeshFromXInMemory(NULL, sizeof(simple_xfile) - 1, D3DXMESH_MANAGED,
                                   device, NULL, NULL, NULL, NULL, &mesh);
    ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#x\n", hr);

    hr = D3DXLoadMeshFromXInMemory(simple_xfile, 0, D3DXMESH_MANAGED,
                                   device, NULL, NULL, NULL, NULL, &mesh);
    ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#x\n", hr);

    hr = D3DXLoadMeshFromXInMemory(simple_xfile, sizeof(simple_xfile) - 1, D3DXMESH_MANAGED,
                                   device, NULL, NULL, NULL, NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#x\n", hr);

    hr = D3DXLoadMeshFromXInMemory(simple_xfile, sizeof(simple_xfile) - 1, D3DXMESH_MANAGED,
                                   NULL, NULL, NULL, NULL, NULL, &mesh);
    ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#x\n", hr);

    hr = D3DXLoadMeshFromXInMemory(empty_xfile, sizeof(empty_xfile) - 1, D3DXMESH_MANAGED,
                                   device, NULL, NULL, NULL, NULL, &mesh);
    ok(hr == E_FAIL, "Expected E_FAIL, got %#x\n", hr);

    hr = D3DXLoadMeshFromXInMemory(simple_xfile, sizeof(simple_xfile) - 1, D3DXMESH_MANAGED,
                                   device, NULL, NULL, NULL, NULL, &mesh);
    ok(hr == D3D_OK, "Expected D3D_OK, got %#x\n", hr);
    if (SUCCEEDED(hr))
        IUnknown_Release(mesh);

    test_LoadMeshFromX(device, simple_xfile, simple_vertex_buffer, simple_fvf, simple_index_buffer, default_materials, TRUE);
    test_LoadMeshFromX(device, box_xfile, box_vertex_buffer, box_fvf, box_index_buffer, box_materials, TRUE);
    test_LoadMeshFromX(device, framed_xfile, merged_vertex_buffer, framed_fvf, merged_index_buffer, default_materials, FALSE);

cleanup:
    if (device) IDirect3DDevice9_Release(device);
    if (d3d) IDirect3D9_Release(d3d);
    if (wnd) DestroyWindow(wnd);
}

static void D3DXCreateBoxTest(void)
{
    HRESULT hr;
    HWND wnd;
    WNDCLASS wc={0};
    IDirect3D9* d3d;
    IDirect3DDevice9* device;
    D3DPRESENT_PARAMETERS d3dpp;
    ID3DXMesh* box;
    ID3DXBuffer* ppBuffer;
    DWORD *buffer;
    static const DWORD adjacency[36]=
        {6, 9, 1, 2, 10, 0,
         1, 9, 3, 4, 10, 2,
         3, 8, 5, 7, 11, 4,
         0, 11, 7, 5, 8, 6,
         7, 4, 9, 2, 0, 8,
         1, 3, 11, 5, 6, 10};
    unsigned int i;

    wc.lpfnWndProc = DefWindowProcA;
    wc.lpszClassName = "d3dx9_test_wc";
    if (!RegisterClass(&wc))
    {
        skip("RegisterClass failed\n");
        return;
    }

    wnd = CreateWindow("d3dx9_test_wc", "d3dx9_test",
                        WS_SYSMENU | WS_POPUP , 0, 0, 640, 480, 0, 0, 0, 0);
    ok(wnd != NULL, "Expected to have a window, received NULL\n");
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

    memset(&d3dpp, 0, sizeof(d3dpp));
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

    hr = D3DXCreateBuffer(36 * sizeof(DWORD), &ppBuffer);
    ok(hr==D3D_OK, "Expected D3D_OK, received %#x\n", hr);
    if (FAILED(hr)) goto end;

    hr = D3DXCreateBox(device,2.0f,20.0f,4.9f,NULL, &ppBuffer);
    todo_wine ok(hr==D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, received %#x\n", hr);

    hr = D3DXCreateBox(NULL,22.0f,20.0f,4.9f,&box, &ppBuffer);
    todo_wine ok(hr==D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, received %#x\n", hr);

    hr = D3DXCreateBox(device,-2.0f,20.0f,4.9f,&box, &ppBuffer);
    todo_wine ok(hr==D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, received %#x\n", hr);

    hr = D3DXCreateBox(device,22.0f,-20.0f,4.9f,&box, &ppBuffer);
    todo_wine ok(hr==D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, received %#x\n", hr);

    hr = D3DXCreateBox(device,22.0f,20.0f,-4.9f,&box, &ppBuffer);
    todo_wine ok(hr==D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, received %#x\n", hr);

    hr = D3DXCreateBox(device,10.9f,20.0f,4.9f,&box, &ppBuffer);
    todo_wine ok(hr==D3D_OK, "Expected D3D_OK, received %#x\n", hr);

    if (FAILED(hr))
    {
        skip("D3DXCreateBox failed\n");
        goto end;
    }

    buffer = ID3DXBuffer_GetBufferPointer(ppBuffer);
    for(i=0; i<36; i++)
        todo_wine ok(adjacency[i]==buffer[i], "expected adjacency %d: %#x, received %#x\n",i,adjacency[i], buffer[i]);

    box->lpVtbl->Release(box);

end:
    IDirect3DDevice9_Release(device);
    IDirect3D9_Release(d3d);
    ID3DXBuffer_Release(ppBuffer);
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

static WORD vertex_index(UINT slices, int slice, int stack)
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
                    mesh->faces[face][0] = vertex_index(slices, slice-1, stack-1);
                    mesh->faces[face][1] = vertex_index(slices, slice, stack-1);
                    mesh->faces[face][2] = vertex_index(slices, slice-1, stack);
                    face++;

                    mesh->faces[face][0] = vertex_index(slices, slice, stack-1);
                    mesh->faces[face][1] = vertex_index(slices, slice, stack);
                    mesh->faces[face][2] = vertex_index(slices, slice-1, stack);
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
            mesh->faces[face][0] = vertex_index(slices, slice-1, stack-1);
            mesh->faces[face][1] = vertex_index(slices, 0, stack-1);
            mesh->faces[face][2] = vertex_index(slices, slice-1, stack);
            face++;

            mesh->faces[face][0] = vertex_index(slices, 0, stack-1);
            mesh->faces[face][1] = vertex_index(slices, 0, stack);
            mesh->faces[face][2] = vertex_index(slices, slice-1, stack);
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
        mesh->faces[face][0] = vertex_index(slices, slice-1, stack-1);
        mesh->faces[face][1] = vertex_index(slices, slice, stack-1);
        mesh->faces[face][2] = vertex;
        face++;
    }

    mesh->faces[face][0] = vertex_index(slices, slice-1, stack-1);
    mesh->faces[face][1] = vertex_index(slices, 0, stack-1);
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
    ok(hr == D3D_OK, "Got result %x, expected 0 (D3D_OK)\n", hr);
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

    mesh.fvf = D3DFVF_XYZ | D3DFVF_NORMAL;

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
    ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n",hr,D3DERR_INVALIDCALL);

    hr = D3DXCreateSphere(NULL, 0.1f, 0, 0, NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n",hr,D3DERR_INVALIDCALL);

    hr = D3DXCreateSphere(NULL, 0.0f, 1, 0, NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n",hr,D3DERR_INVALIDCALL);

    hr = D3DXCreateSphere(NULL, 0.0f, 0, 1, NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n",hr,D3DERR_INVALIDCALL);

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
    ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n",hr,D3DERR_INVALIDCALL);

    hr = D3DXCreateSphere(device, 1.0f, 2, 1, &sphere, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateSphere(device, 1.0f, 1, 2, &sphere, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateSphere(device, -0.1f, 1, 2, &sphere, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

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

static BOOL compute_cylinder(struct mesh *mesh, FLOAT radius1, FLOAT radius2, FLOAT length, UINT slices, UINT stacks)
{
    float theta_step, theta_start;
    struct sincos_table theta;
    FLOAT delta_radius, radius, radius_step;
    FLOAT z, z_step, z_normal;
    DWORD number_of_vertices, number_of_faces;
    DWORD vertex, face;
    int slice, stack;

    /* theta = angle on xy plane wrt x axis */
    theta_step = -2 * M_PI / slices;
    theta_start = M_PI / 2;

    if (!compute_sincos_table(&theta, theta_start, theta_step, slices))
    {
        return FALSE;
    }

    number_of_vertices = 2 + (slices * (3 + stacks));
    number_of_faces = 2 * slices + stacks * (2 * slices);

    if (!new_mesh(mesh, number_of_vertices, number_of_faces))
    {
        free_sincos_table(&theta);
        return FALSE;
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

    mesh->vertices[vertex].normal.x = 0.0f;
    mesh->vertices[vertex].normal.y = 0.0f;
    mesh->vertices[vertex].normal.z = -1.0f;
    mesh->vertices[vertex].position.x = 0.0f;
    mesh->vertices[vertex].position.y = 0.0f;
    mesh->vertices[vertex++].position.z = z;

    for (slice = 0; slice < slices; slice++, vertex++)
    {
        mesh->vertices[vertex].normal.x = 0.0f;
        mesh->vertices[vertex].normal.y = 0.0f;
        mesh->vertices[vertex].normal.z = -1.0f;
        mesh->vertices[vertex].position.x = radius * theta.cos[slice];
        mesh->vertices[vertex].position.y = radius * theta.sin[slice];
        mesh->vertices[vertex].position.z = z;

        if (slice > 0)
        {
            mesh->faces[face][0] = 0;
            mesh->faces[face][1] = slice;
            mesh->faces[face++][2] = slice + 1;
        }
    }

    mesh->faces[face][0] = 0;
    mesh->faces[face][1] = slice;
    mesh->faces[face++][2] = 1;

    for (stack = 1; stack <= stacks+1; stack++)
    {
        for (slice = 0; slice < slices; slice++, vertex++)
        {
            mesh->vertices[vertex].normal.x = theta.cos[slice];
            mesh->vertices[vertex].normal.y = theta.sin[slice];
            mesh->vertices[vertex].normal.z = z_normal;
            D3DXVec3Normalize(&mesh->vertices[vertex].normal, &mesh->vertices[vertex].normal);
            mesh->vertices[vertex].position.x = radius * theta.cos[slice];
            mesh->vertices[vertex].position.y = radius * theta.sin[slice];
            mesh->vertices[vertex].position.z = z;

            if (stack > 1 && slice > 0)
            {
                mesh->faces[face][0] = vertex_index(slices, slice-1, stack-1);
                mesh->faces[face][1] = vertex_index(slices, slice-1, stack);
                mesh->faces[face++][2] = vertex_index(slices, slice, stack-1);

                mesh->faces[face][0] = vertex_index(slices, slice, stack-1);
                mesh->faces[face][1] = vertex_index(slices, slice-1, stack);
                mesh->faces[face++][2] = vertex_index(slices, slice, stack);
            }
        }

        if (stack > 1)
        {
            mesh->faces[face][0] = vertex_index(slices, slice-1, stack-1);
            mesh->faces[face][1] = vertex_index(slices, slice-1, stack);
            mesh->faces[face++][2] = vertex_index(slices, 0, stack-1);

            mesh->faces[face][0] = vertex_index(slices, 0, stack-1);
            mesh->faces[face][1] = vertex_index(slices, slice-1, stack);
            mesh->faces[face++][2] = vertex_index(slices, 0, stack);
        }

        if (stack < stacks + 1)
        {
            z += z_step;
            radius -= radius_step;
        }
    }

    for (slice = 0; slice < slices; slice++, vertex++)
    {
        mesh->vertices[vertex].normal.x = 0.0f;
        mesh->vertices[vertex].normal.y = 0.0f;
        mesh->vertices[vertex].normal.z = 1.0f;
        mesh->vertices[vertex].position.x = radius * theta.cos[slice];
        mesh->vertices[vertex].position.y = radius * theta.sin[slice];
        mesh->vertices[vertex].position.z = z;

        if (slice > 0)
        {
            mesh->faces[face][0] = vertex_index(slices, slice-1, stack);
            mesh->faces[face][1] = number_of_vertices - 1;
            mesh->faces[face++][2] = vertex_index(slices, slice, stack);
        }
    }

    mesh->vertices[vertex].position.x = 0.0f;
    mesh->vertices[vertex].position.y = 0.0f;
    mesh->vertices[vertex].position.z = z;
    mesh->vertices[vertex].normal.x = 0.0f;
    mesh->vertices[vertex].normal.y = 0.0f;
    mesh->vertices[vertex].normal.z = 1.0f;

    mesh->faces[face][0] = vertex_index(slices, slice-1, stack);
    mesh->faces[face][1] = number_of_vertices - 1;
    mesh->faces[face][2] = vertex_index(slices, 0, stack);

    free_sincos_table(&theta);

    return TRUE;
}

static void test_cylinder(IDirect3DDevice9 *device, FLOAT radius1, FLOAT radius2, FLOAT length, UINT slices, UINT stacks)
{
    HRESULT hr;
    ID3DXMesh *cylinder;
    struct mesh mesh;
    char name[256];

    hr = D3DXCreateCylinder(device, radius1, radius2, length, slices, stacks, &cylinder, NULL);
    ok(hr == D3D_OK, "Got result %x, expected 0 (D3D_OK)\n", hr);
    if (hr != D3D_OK)
    {
        skip("Couldn't create cylinder\n");
        return;
    }

    if (!compute_cylinder(&mesh, radius1, radius2, length, slices, stacks))
    {
        skip("Couldn't create mesh\n");
        cylinder->lpVtbl->Release(cylinder);
        return;
    }

    mesh.fvf = D3DFVF_XYZ | D3DFVF_NORMAL;

    sprintf(name, "cylinder (%g, %g, %g, %u, %u)", radius1, radius2, length, slices, stacks);
    compare_mesh(name, cylinder, &mesh);

    free_mesh(&mesh);

    cylinder->lpVtbl->Release(cylinder);
}

static void D3DXCreateCylinderTest(void)
{
    HRESULT hr;
    HWND wnd;
    IDirect3D9* d3d;
    IDirect3DDevice9* device;
    D3DPRESENT_PARAMETERS d3dpp;
    ID3DXMesh* cylinder = NULL;

    hr = D3DXCreateCylinder(NULL, 0.0f, 0.0f, 0.0f, 0, 0, NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n",hr,D3DERR_INVALIDCALL);

    hr = D3DXCreateCylinder(NULL, 1.0f, 1.0f, 1.0f, 2, 1, &cylinder, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n",hr,D3DERR_INVALIDCALL);

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

    hr = D3DXCreateCylinder(device, -0.1f, 1.0f, 1.0f, 2, 1, &cylinder, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n",hr,D3DERR_INVALIDCALL);

    hr = D3DXCreateCylinder(device, 0.0f, 1.0f, 1.0f, 2, 1, &cylinder, NULL);
    ok(hr == D3D_OK, "Got result %x, expected 0 (D3D_OK)\n",hr);

    if (SUCCEEDED(hr) && cylinder)
    {
        cylinder->lpVtbl->Release(cylinder);
    }

    hr = D3DXCreateCylinder(device, 1.0f, -0.1f, 1.0f, 2, 1, &cylinder, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n",hr,D3DERR_INVALIDCALL);

    hr = D3DXCreateCylinder(device, 1.0f, 0.0f, 1.0f, 2, 1, &cylinder, NULL);
    ok(hr == D3D_OK, "Got result %x, expected 0 (D3D_OK)\n",hr);

    if (SUCCEEDED(hr) && cylinder)
    {
        cylinder->lpVtbl->Release(cylinder);
    }

    hr = D3DXCreateCylinder(device, 1.0f, 1.0f, -0.1f, 2, 1, &cylinder, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n",hr,D3DERR_INVALIDCALL);

    /* Test with length == 0.0f succeeds */
    hr = D3DXCreateCylinder(device, 1.0f, 1.0f, 0.0f, 2, 1, &cylinder, NULL);
    ok(hr == D3D_OK, "Got result %x, expected 0 (D3D_OK)\n",hr);

    if (SUCCEEDED(hr) && cylinder)
    {
        cylinder->lpVtbl->Release(cylinder);
    }

    hr = D3DXCreateCylinder(device, 1.0f, 1.0f, 1.0f, 1, 1, &cylinder, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n",hr,D3DERR_INVALIDCALL);

    hr = D3DXCreateCylinder(device, 1.0f, 1.0f, 1.0f, 2, 0, &cylinder, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n",hr,D3DERR_INVALIDCALL);

    hr = D3DXCreateCylinder(device, 1.0f, 1.0f, 1.0f, 2, 1, NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n",hr,D3DERR_INVALIDCALL);

    test_cylinder(device, 0.0f, 0.0f, 0.0f, 2, 1);
    test_cylinder(device, 1.0f, 1.0f, 1.0f, 2, 1);
    test_cylinder(device, 1.0f, 1.0f, 2.0f, 3, 4);
    test_cylinder(device, 3.0f, 2.0f, 4.0f, 3, 4);
    test_cylinder(device, 2.0f, 3.0f, 4.0f, 3, 4);
    test_cylinder(device, 3.0f, 4.0f, 5.0f, 11, 20);

    IDirect3DDevice9_Release(device);
    IDirect3D9_Release(d3d);
    DestroyWindow(wnd);
}

struct dynamic_array
{
    int count, capacity;
    void *items;
};

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

struct glyphinfo
{
    struct outline_array outlines;
    float offset_x;
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

static struct point2d *add_point(struct outline *array)
{
    struct point2d *item;

    if (!reserve((struct dynamic_array *)array, array->count + 1, sizeof(array->items[0])))
        return NULL;

    item = &array->items[array->count++];
    ZeroMemory(item, sizeof(*item));
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
                                 float max_deviation)
{
    D3DXVECTOR2 split1 = {0, 0}, split2 = {0, 0}, middle, vec;
    float deviation;

    D3DXVec2Scale(&split1, D3DXVec2Add(&split1, p1, p2), 0.5f);
    D3DXVec2Scale(&split2, D3DXVec2Add(&split2, p2, p3), 0.5f);
    D3DXVec2Scale(&middle, D3DXVec2Add(&middle, &split1, &split2), 0.5f);

    deviation = D3DXVec2Length(D3DXVec2Subtract(&vec, &middle, p2));
    if (deviation < max_deviation) {
        struct point2d *pt = add_point(outline);
        if (!pt) return E_OUTOFMEMORY;
        pt->pos = *p2;
        pt->corner = POINTTYPE_CURVE;
        /* the end point is omitted because the end line merges into the next segment of
         * the split bezier curve, and the end of the split bezier curve is added outside
         * this recursive function. */
    } else {
        HRESULT hr = add_bezier_points(outline, p1, &split1, &middle, max_deviation);
        if (hr != S_OK) return hr;
        hr = add_bezier_points(outline, &middle, &split2, p3, max_deviation);
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

static BOOL attempt_line_merge(struct outline *outline,
                               int pt_index,
                               const D3DXVECTOR2 *nextpt,
                               BOOL to_curve)
{
    D3DXVECTOR2 curdir, lastdir;
    struct point2d *prevpt, *pt;
    BOOL ret = FALSE;
    const float cos_half = cos(D3DXToRadian(0.5f));

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
    if (is_direction_similar(&lastdir, &curdir, cos_half))
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
                              float max_deviation, float emsquare)
{
    const float cos_45 = cos(D3DXToRadian(45.0f));
    const float cos_90 = cos(D3DXToRadian(90.0f));
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

        pt = add_point(outline);
        if (!pt)
            return E_OUTOFMEMORY;
        pt_flt = convert_fixed_to_float(&header->pfxStart, 1, emsquare);
        pt->pos = *pt_flt;
        pt->corner = POINTTYPE_CORNER;

        if (header->dwType != TT_POLYGON_TYPE)
            trace("Unknown header type %d\n", header->dwType);

        while ((char *)curve < (char *)header + header->cb)
        {
            D3DXVECTOR2 bezier_start = outline->items[outline->count - 1].pos;
            BOOL to_curve = curve->wType != TT_PRIM_LINE && curve->cpfx > 1;

            if (!curve->cpfx) {
                curve = (TTPOLYCURVE *)&curve->apfx[curve->cpfx];
                continue;
            }

            pt_flt = convert_fixed_to_float(curve->apfx, curve->cpfx, emsquare);

            attempt_line_merge(outline, outline->count - 1, &pt_flt[0], to_curve);

            if (to_curve)
            {
                HRESULT hr;
                int count = curve->cpfx;
                j = 0;

                while (count > 2)
                {
                    D3DXVECTOR2 bezier_end;

                    D3DXVec2Scale(&bezier_end, D3DXVec2Add(&bezier_end, &pt_flt[j], &pt_flt[j+1]), 0.5f);
                    hr = add_bezier_points(outline, &bezier_start, &pt_flt[j], &bezier_end, max_deviation);
                    if (hr != S_OK)
                        return hr;
                    bezier_start = bezier_end;
                    count--;
                    j++;
                }
                hr = add_bezier_points(outline, &bezier_start, &pt_flt[j], &pt_flt[j+1], max_deviation);
                if (hr != S_OK)
                    return hr;

                pt = add_point(outline);
                if (!pt)
                    return E_OUTOFMEMORY;
                j++;
                pt->pos = pt_flt[j];
                pt->corner = POINTTYPE_CURVE_END;
            } else {
                for (j = 0; j < curve->cpfx; j++)
                {
                    pt = add_point(outline);
                    if (!pt)
                        return E_OUTOFMEMORY;
                    pt->pos = pt_flt[j];
                    pt->corner = POINTTYPE_CORNER;
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
                attempt_line_merge(outline, outline->count - 1, &pt->pos, FALSE);
            }
            lastpt = &outline->items[0];
            to_curve = lastpt->corner != POINTTYPE_CORNER && lastpt->corner != POINTTYPE_CURVE_END;
            if (lastpt->corner == POINTTYPE_CURVE_START)
                lastpt->corner = POINTTYPE_CORNER;
            pt = &outline->items[1];
            if (attempt_line_merge(outline, 0, &pt->pos, to_curve))
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
                    if (!is_direction_similar(&lastdir, &curdir, cos_45))
                        lastpt->corner = POINTTYPE_CORNER;
                    break;
                case POINTTYPE_CURVE_MIDDLE:
                    if (!is_direction_similar(&lastdir, &curdir, cos_90))
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

static BOOL compute_text_mesh(struct mesh *mesh, HDC hdc, LPCSTR text, FLOAT deviation, FLOAT extrusion, FLOAT otmEMSquare)
{
    HRESULT hr = E_FAIL;
    DWORD nb_vertices, nb_faces;
    DWORD nb_corners, nb_outline_points;
    int textlen = 0;
    float offset_x;
    char *raw_outline = NULL;
    struct glyphinfo *glyphs = NULL;
    GLYPHMETRICS gm;
    int i;
    struct vertex *vertex_ptr;
    face *face_ptr;

    if (deviation == 0.0f)
        deviation = 1.0f / otmEMSquare;

    textlen = strlen(text);
    glyphs = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, textlen * sizeof(*glyphs));
    if (!glyphs) {
        hr = E_OUTOFMEMORY;
        goto error;
    }

    offset_x = 0.0f;
    for (i = 0; i < textlen; i++)
    {
        /* get outline points from data returned from GetGlyphOutline */
        const MAT2 identity = {{0, 1}, {0, 0}, {0, 0}, {0, 1}};
        int datasize;

        glyphs[i].offset_x = offset_x;

        datasize = GetGlyphOutline(hdc, text[i], GGO_NATIVE, &gm, 0, NULL, &identity);
        if (datasize < 0) {
            hr = E_FAIL;
            goto error;
        }
        HeapFree(GetProcessHeap(), 0, raw_outline);
        raw_outline = HeapAlloc(GetProcessHeap(), 0, datasize);
        if (!glyphs) {
            hr = E_OUTOFMEMORY;
            goto error;
        }
        datasize = GetGlyphOutline(hdc, text[i], GGO_NATIVE, &gm, datasize, raw_outline, &identity);

        create_outline(&glyphs[i], raw_outline, datasize, deviation, otmEMSquare);

        offset_x += gm.gmCellIncX / (float)otmEMSquare;
    }

    /* corner points need an extra vertex for the different side faces normals */
    nb_corners = 0;
    nb_outline_points = 0;
    for (i = 0; i < textlen; i++)
    {
        int j;
        for (j = 0; j < glyphs[i].outlines.count; j++)
        {
            int k;
            struct outline *outline = &glyphs[i].outlines.items[j];
            nb_outline_points += outline->count;
            nb_corners++; /* first outline point always repeated as a corner */
            for (k = 1; k < outline->count; k++)
                if (outline->items[k].corner)
                    nb_corners++;
        }
    }

    nb_vertices = (nb_outline_points + nb_corners) * 2 + textlen;
    nb_faces = nb_outline_points * 2;

    if (!new_mesh(mesh, nb_vertices, nb_faces))
        goto error;

    /* convert 2D vertices and faces into 3D mesh */
    vertex_ptr = mesh->vertices;
    face_ptr = mesh->faces;
    for (i = 0; i < textlen; i++)
    {
        int j;

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
                WORD vtx_idx = vertex_ptr - mesh->vertices;
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
                            struct point2d *nextpt2 = &outline->items[(k + 1) % outline->count];
                            D3DXVec2Subtract(&vec, &nextpt2->pos, &nextpt->pos);
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

        /* FIXME: compute expected faces */
        /* Add placeholder to separate glyph outlines */
        vertex_ptr->position.x = 0;
        vertex_ptr->position.y = 0;
        vertex_ptr->position.z = 0;
        vertex_ptr->normal.x = 0;
        vertex_ptr->normal.y = 0;
        vertex_ptr->normal.z = 1;
        vertex_ptr++;
    }

    hr = D3D_OK;
error:
    if (glyphs) {
        for (i = 0; i < textlen; i++)
        {
            int j;
            for (j = 0; j < glyphs[i].outlines.count; j++)
                HeapFree(GetProcessHeap(), 0, glyphs[i].outlines.items[j].items);
            HeapFree(GetProcessHeap(), 0, glyphs[i].outlines.items);
        }
        HeapFree(GetProcessHeap(), 0, glyphs);
    }
    HeapFree(GetProcessHeap(), 0, raw_outline);

    return hr == D3D_OK;
}

static void compare_text_outline_mesh(const char *name, ID3DXMesh *d3dxmesh, struct mesh *mesh, int textlen, float extrusion)
{
    HRESULT hr;
    DWORD number_of_vertices, number_of_faces;
    IDirect3DVertexBuffer9 *vertex_buffer = NULL;
    IDirect3DIndexBuffer9 *index_buffer = NULL;
    D3DVERTEXBUFFER_DESC vertex_buffer_description;
    D3DINDEXBUFFER_DESC index_buffer_description;
    struct vertex *vertices = NULL;
    face *faces = NULL;
    int expected, i;
    int vtx_idx1, face_idx1, vtx_idx2, face_idx2;

    number_of_vertices = d3dxmesh->lpVtbl->GetNumVertices(d3dxmesh);
    number_of_faces = d3dxmesh->lpVtbl->GetNumFaces(d3dxmesh);

    /* vertex buffer */
    hr = d3dxmesh->lpVtbl->GetVertexBuffer(d3dxmesh, &vertex_buffer);
    ok(hr == D3D_OK, "Test %s, result %x, expected 0 (D3D_OK)\n", name, hr);
    if (hr != D3D_OK)
    {
        skip("Couldn't get vertex buffers\n");
        goto error;
    }

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
        ok(vertex_buffer_description.Pool == D3DPOOL_MANAGED, "Test %s, result %x, expected %x (D3DPOOL_MANAGED)\n",
           name, vertex_buffer_description.Pool, D3DPOOL_MANAGED);
        ok(vertex_buffer_description.FVF == mesh->fvf, "Test %s, result %x, expected %x\n",
           name, vertex_buffer_description.FVF, mesh->fvf);
        if (mesh->fvf == 0)
        {
            expected = number_of_vertices * mesh->vertex_size;
        }
        else
        {
            expected = number_of_vertices * D3DXGetFVFVertexSize(mesh->fvf);
        }
        ok(vertex_buffer_description.Size == expected, "Test %s, result %x, expected %x\n",
           name, vertex_buffer_description.Size, expected);
    }

    hr = d3dxmesh->lpVtbl->GetIndexBuffer(d3dxmesh, &index_buffer);
    ok(hr == D3D_OK, "Test %s, result %x, expected 0 (D3D_OK)\n", name, hr);
    if (hr != D3D_OK)
    {
        skip("Couldn't get index buffer\n");
        goto error;
    }

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
        todo_wine ok(index_buffer_description.Usage == 0, "Test %s, result %x, expected %x\n", name, index_buffer_description.Usage, 0);
        ok(index_buffer_description.Pool == D3DPOOL_MANAGED, "Test %s, result %x, expected %x (D3DPOOL_MANAGED)\n",
           name, index_buffer_description.Pool, D3DPOOL_MANAGED);
        expected = number_of_faces * sizeof(WORD) * 3;
        ok(index_buffer_description.Size == expected, "Test %s, result %x, expected %x\n",
           name, index_buffer_description.Size, expected);
    }

    /* specify offset and size to avoid potential overruns */
    hr = IDirect3DVertexBuffer9_Lock(vertex_buffer, 0, number_of_vertices * sizeof(D3DXVECTOR3) * 2,
                                     (LPVOID *)&vertices, D3DLOCK_DISCARD);
    ok(hr == D3D_OK, "Test %s, result %x, expected 0 (D3D_OK)\n", name, hr);
    if (hr != D3D_OK)
    {
        skip("Couldn't lock vertex buffer\n");
        goto error;
    }
    hr = IDirect3DIndexBuffer9_Lock(index_buffer, 0, number_of_faces * sizeof(WORD) * 3,
                                    (LPVOID *)&faces, D3DLOCK_DISCARD);
    ok(hr == D3D_OK, "Test %s, result %x, expected 0 (D3D_OK)\n", name, hr);
    if (hr != D3D_OK)
    {
        skip("Couldn't lock index buffer\n");
        goto error;
    }

    face_idx1 = 0;
    vtx_idx2 = 0;
    face_idx2 = 0;
    vtx_idx1 = 0;
    for (i = 0; i < textlen; i++)
    {
        int nb_outline_vertices1, nb_outline_faces1;
        int nb_outline_vertices2, nb_outline_faces2;
        int nb_back_vertices, nb_back_faces;
        int first_vtx1, first_vtx2;
        int first_face1, first_face2;
        int j;

        first_vtx1 = vtx_idx1;
        first_vtx2 = vtx_idx2;
        for (; vtx_idx1 < number_of_vertices; vtx_idx1++) {
            if (vertices[vtx_idx1].normal.z != 0)
                break;
        }
        for (; vtx_idx2 < mesh->number_of_vertices; vtx_idx2++) {
            if (mesh->vertices[vtx_idx2].normal.z != 0)
                break;
        }
        nb_outline_vertices1 = vtx_idx1 - first_vtx1;
        nb_outline_vertices2 = vtx_idx2 - first_vtx2;
        ok(nb_outline_vertices1 == nb_outline_vertices2,
           "Test %s, glyph %d, outline vertex count result %d, expected %d\n", name, i,
           nb_outline_vertices1, nb_outline_vertices2);

        for (j = 0; j < min(nb_outline_vertices1, nb_outline_vertices2); j++)
        {
            vtx_idx1 = first_vtx1 + j;
            vtx_idx2 = first_vtx2 + j;
            ok(compare_vec3(vertices[vtx_idx1].position, mesh->vertices[vtx_idx2].position),
               "Test %s, glyph %d, vertex position %d, result (%g, %g, %g), expected (%g, %g, %g)\n", name, i, vtx_idx1,
               vertices[vtx_idx1].position.x, vertices[vtx_idx1].position.y, vertices[vtx_idx1].position.z,
               mesh->vertices[vtx_idx2].position.x, mesh->vertices[vtx_idx2].position.y, mesh->vertices[vtx_idx2].position.z);
            ok(compare_vec3(vertices[vtx_idx1].normal, mesh->vertices[first_vtx2 + j].normal),
               "Test %s, glyph %d, vertex normal %d, result (%g, %g, %g), expected (%g, %g, %g)\n", name, i, vtx_idx1,
               vertices[vtx_idx1].normal.x, vertices[vtx_idx1].normal.y, vertices[vtx_idx1].normal.z,
               mesh->vertices[vtx_idx2].normal.x, mesh->vertices[vtx_idx2].normal.y, mesh->vertices[vtx_idx2].normal.z);
        }
        vtx_idx1 = first_vtx1 + nb_outline_vertices1;
        vtx_idx2 = first_vtx2 + nb_outline_vertices2;

        first_face1 = face_idx1;
        first_face2 = face_idx2;
        for (; face_idx1 < number_of_faces; face_idx1++)
        {
            if (faces[face_idx1][0] >= vtx_idx1 ||
                faces[face_idx1][1] >= vtx_idx1 ||
                faces[face_idx1][2] >= vtx_idx1)
                break;
        }
        for (; face_idx2 < mesh->number_of_faces; face_idx2++)
        {
            if (mesh->faces[face_idx2][0] >= vtx_idx2 ||
                mesh->faces[face_idx2][1] >= vtx_idx2 ||
                mesh->faces[face_idx2][2] >= vtx_idx2)
                break;
        }
        nb_outline_faces1 = face_idx1 - first_face1;
        nb_outline_faces2 = face_idx2 - first_face2;
        ok(nb_outline_faces1 == nb_outline_faces2,
           "Test %s, glyph %d, outline face count result %d, expected %d\n", name, i,
           nb_outline_faces1, nb_outline_faces2);

        for (j = 0; j < min(nb_outline_faces1, nb_outline_faces2); j++)
        {
            face_idx1 = first_face1 + j;
            face_idx2 = first_face2 + j;
            ok(faces[face_idx1][0] - first_vtx1 == mesh->faces[face_idx2][0] - first_vtx2 &&
               faces[face_idx1][1] - first_vtx1 == mesh->faces[face_idx2][1] - first_vtx2 &&
               faces[face_idx1][2] - first_vtx1 == mesh->faces[face_idx2][2] - first_vtx2,
               "Test %s, glyph %d, face %d, result (%d, %d, %d), expected (%d, %d, %d)\n", name, i, face_idx1,
               faces[face_idx1][0], faces[face_idx1][1], faces[face_idx1][2],
               mesh->faces[face_idx2][0] - first_vtx2 + first_vtx1,
               mesh->faces[face_idx2][1] - first_vtx2 + first_vtx1,
               mesh->faces[face_idx2][2] - first_vtx2 + first_vtx1);
        }
        face_idx1 = first_face1 + nb_outline_faces1;
        face_idx2 = first_face2 + nb_outline_faces2;

        /* partial test on back vertices and faces  */
        first_vtx1 = vtx_idx1;
        for (; vtx_idx1 < number_of_vertices; vtx_idx1++) {
            struct vertex vtx;

            if (vertices[vtx_idx1].normal.z != 1.0f)
                break;

            vtx.position.z = 0.0f;
            vtx.normal.x = 0.0f;
            vtx.normal.y = 0.0f;
            vtx.normal.z = 1.0f;
            ok(compare(vertices[vtx_idx1].position.z, vtx.position.z),
               "Test %s, glyph %d, vertex position.z %d, result %g, expected %g\n", name, i, vtx_idx1,
               vertices[vtx_idx1].position.z, vtx.position.z);
            ok(compare_vec3(vertices[vtx_idx1].normal, vtx.normal),
               "Test %s, glyph %d, vertex normal %d, result (%g, %g, %g), expected (%g, %g, %g)\n", name, i, vtx_idx1,
               vertices[vtx_idx1].normal.x, vertices[vtx_idx1].normal.y, vertices[vtx_idx1].normal.z,
               vtx.normal.x, vtx.normal.y, vtx.normal.z);
        }
        nb_back_vertices = vtx_idx1 - first_vtx1;
        first_face1 = face_idx1;
        for (; face_idx1 < number_of_faces; face_idx1++)
        {
            const D3DXVECTOR3 *vtx1, *vtx2, *vtx3;
            D3DXVECTOR3 normal;
            D3DXVECTOR3 v1 = {0, 0, 0};
            D3DXVECTOR3 v2 = {0, 0, 0};
            D3DXVECTOR3 forward = {0.0f, 0.0f, 1.0f};

            if (faces[face_idx1][0] >= vtx_idx1 ||
                faces[face_idx1][1] >= vtx_idx1 ||
                faces[face_idx1][2] >= vtx_idx1)
                break;

            vtx1 = &vertices[faces[face_idx1][0]].position;
            vtx2 = &vertices[faces[face_idx1][1]].position;
            vtx3 = &vertices[faces[face_idx1][2]].position;

            D3DXVec3Subtract(&v1, vtx2, vtx1);
            D3DXVec3Subtract(&v2, vtx3, vtx2);
            D3DXVec3Cross(&normal, &v1, &v2);
            D3DXVec3Normalize(&normal, &normal);
            ok(!D3DXVec3Length(&normal) || compare_vec3(normal, forward),
               "Test %s, glyph %d, face %d normal, result (%g, %g, %g), expected (%g, %g, %g)\n", name, i, face_idx1,
               normal.x, normal.y, normal.z, forward.x, forward.y, forward.z);
        }
        nb_back_faces = face_idx1 - first_face1;

        /* compare front and back faces & vertices */
        if (extrusion == 0.0f) {
            /* Oddly there are only back faces in this case */
            nb_back_vertices /= 2;
            nb_back_faces /= 2;
            face_idx1 -= nb_back_faces;
            vtx_idx1 -= nb_back_vertices;
        }
        for (j = 0; j < nb_back_vertices; j++)
        {
            struct vertex vtx = vertices[first_vtx1];
            vtx.position.z = -extrusion;
            vtx.normal.x = 0.0f;
            vtx.normal.y = 0.0f;
            vtx.normal.z = extrusion == 0.0f ? 1.0f : -1.0f;
            ok(compare_vec3(vertices[vtx_idx1].position, vtx.position),
               "Test %s, glyph %d, vertex position %d, result (%g, %g, %g), expected (%g, %g, %g)\n", name, i, vtx_idx1,
               vertices[vtx_idx1].position.x, vertices[vtx_idx1].position.y, vertices[vtx_idx1].position.z,
               vtx.position.x, vtx.position.y, vtx.position.z);
            ok(compare_vec3(vertices[vtx_idx1].normal, vtx.normal),
               "Test %s, glyph %d, vertex normal %d, result (%g, %g, %g), expected (%g, %g, %g)\n", name, i, vtx_idx1,
               vertices[vtx_idx1].normal.x, vertices[vtx_idx1].normal.y, vertices[vtx_idx1].normal.z,
               vtx.normal.x, vtx.normal.y, vtx.normal.z);
            vtx_idx1++;
            first_vtx1++;
        }
        for (j = 0; j < nb_back_faces; j++)
        {
            int f1, f2;
            if (extrusion == 0.0f) {
                f1 = 1;
                f2 = 2;
            } else {
                f1 = 2;
                f2 = 1;
            }
            ok(faces[face_idx1][0] == faces[first_face1][0] + nb_back_vertices &&
               faces[face_idx1][1] == faces[first_face1][f1] + nb_back_vertices &&
               faces[face_idx1][2] == faces[first_face1][f2] + nb_back_vertices,
               "Test %s, glyph %d, face %d, result (%d, %d, %d), expected (%d, %d, %d)\n", name, i, face_idx1,
               faces[face_idx1][0], faces[face_idx1][1], faces[face_idx1][2],
               faces[first_face1][0] - nb_back_faces,
               faces[first_face1][f1] - nb_back_faces,
               faces[first_face1][f2] - nb_back_faces);
            first_face1++;
            face_idx1++;
        }

        /* skip to the outline for the next glyph */
        for (; vtx_idx2 < mesh->number_of_vertices; vtx_idx2++) {
            if (mesh->vertices[vtx_idx2].normal.z == 0)
                break;
        }
        for (; face_idx2 < mesh->number_of_faces; face_idx2++)
        {
            if (mesh->faces[face_idx2][0] >= vtx_idx2 ||
                mesh->faces[face_idx2][1] >= vtx_idx2 ||
                mesh->faces[face_idx2][2] >= vtx_idx2) break;
        }
    }

error:
    if (vertices) IDirect3DVertexBuffer9_Unlock(vertex_buffer);
    if (faces) IDirect3DIndexBuffer9_Unlock(index_buffer);
    if (index_buffer) IDirect3DIndexBuffer9_Release(index_buffer);
    if (vertex_buffer) IDirect3DVertexBuffer9_Release(vertex_buffer);
}

static void test_createtext(IDirect3DDevice9 *device, HDC hdc, LPCSTR text, FLOAT deviation, FLOAT extrusion)
{
    HRESULT hr;
    ID3DXMesh *d3dxmesh;
    struct mesh mesh;
    char name[256];
    OUTLINETEXTMETRIC otm;
    GLYPHMETRICS gm;
    GLYPHMETRICSFLOAT *glyphmetrics_float = HeapAlloc(GetProcessHeap(), 0, sizeof(GLYPHMETRICSFLOAT) * strlen(text));
    int i;
    LOGFONT lf;
    HFONT font = NULL, oldfont = NULL;

    sprintf(name, "text ('%s', %f, %f)", text, deviation, extrusion);

    hr = D3DXCreateText(device, hdc, text, deviation, extrusion, &d3dxmesh, NULL, glyphmetrics_float);
    ok(hr == D3D_OK, "Got result %x, expected 0 (D3D_OK)\n", hr);
    if (hr != D3D_OK)
    {
        skip("Couldn't create text with D3DXCreateText\n");
        return;
    }

    /* must select a modified font having lfHeight = otm.otmEMSquare before
     * calling GetGlyphOutline to get the expected values */
    if (!GetObject(GetCurrentObject(hdc, OBJ_FONT), sizeof(lf), &lf) ||
        !GetOutlineTextMetrics(hdc, sizeof(otm), &otm))
    {
        d3dxmesh->lpVtbl->Release(d3dxmesh);
        skip("Couldn't get text outline\n");
        return;
    }
    lf.lfHeight = otm.otmEMSquare;
    lf.lfWidth = 0;
    font = CreateFontIndirect(&lf);
    if (!font) {
        d3dxmesh->lpVtbl->Release(d3dxmesh);
        skip("Couldn't create the modified font\n");
        return;
    }
    oldfont = SelectObject(hdc, font);

    for (i = 0; i < strlen(text); i++)
    {
        const MAT2 identity = {{0, 1}, {0, 0}, {0, 0}, {0, 1}};
        GetGlyphOutlineA(hdc, text[i], GGO_NATIVE, &gm, 0, NULL, &identity);
        compare_float(glyphmetrics_float[i].gmfBlackBoxX, gm.gmBlackBoxX / (float)otm.otmEMSquare);
        compare_float(glyphmetrics_float[i].gmfBlackBoxY, gm.gmBlackBoxY / (float)otm.otmEMSquare);
        compare_float(glyphmetrics_float[i].gmfptGlyphOrigin.x, gm.gmptGlyphOrigin.x / (float)otm.otmEMSquare);
        compare_float(glyphmetrics_float[i].gmfptGlyphOrigin.y, gm.gmptGlyphOrigin.y / (float)otm.otmEMSquare);
        compare_float(glyphmetrics_float[i].gmfCellIncX, gm.gmCellIncX / (float)otm.otmEMSquare);
        compare_float(glyphmetrics_float[i].gmfCellIncY, gm.gmCellIncY / (float)otm.otmEMSquare);
    }

    ZeroMemory(&mesh, sizeof(mesh));
    if (!compute_text_mesh(&mesh, hdc, text, deviation, extrusion, otm.otmEMSquare))
    {
        skip("Couldn't create mesh\n");
        d3dxmesh->lpVtbl->Release(d3dxmesh);
        return;
    }
    mesh.fvf = D3DFVF_XYZ | D3DFVF_NORMAL;

    compare_text_outline_mesh(name, d3dxmesh, &mesh, strlen(text), extrusion);

    free_mesh(&mesh);

    d3dxmesh->lpVtbl->Release(d3dxmesh);
    SelectObject(hdc, oldfont);
    HeapFree(GetProcessHeap(), 0, glyphmetrics_float);
}

static void D3DXCreateTextTest(void)
{
    HRESULT hr;
    HWND wnd;
    HDC hdc;
    IDirect3D9* d3d;
    IDirect3DDevice9* device;
    D3DPRESENT_PARAMETERS d3dpp;
    ID3DXMesh* d3dxmesh = NULL;
    HFONT hFont;
    OUTLINETEXTMETRIC otm;
    int number_of_vertices;
    int number_of_faces;

    wnd = CreateWindow("static", "d3dx9_test", WS_POPUP, 0, 0, 1000, 1000, NULL, NULL, NULL, NULL);
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

    hdc = CreateCompatibleDC(NULL);

    hFont = CreateFont(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                       OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                       "Arial");
    SelectObject(hdc, hFont);
    GetOutlineTextMetrics(hdc, sizeof(otm), &otm);

    hr = D3DXCreateText(device, hdc, "wine", 0.001f, 0.4f, NULL, NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    /* D3DXCreateTextA page faults from passing NULL text */

    hr = D3DXCreateTextW(device, hdc, NULL, 0.001f, 0.4f, &d3dxmesh, NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateText(device, hdc, "", 0.001f, 0.4f, &d3dxmesh, NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateText(device, hdc, " ", 0.001f, 0.4f, &d3dxmesh, NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateText(NULL, hdc, "wine", 0.001f, 0.4f, &d3dxmesh, NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateText(device, NULL, "wine", 0.001f, 0.4f, &d3dxmesh, NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateText(device, hdc, "wine", -FLT_MIN, 0.4f, &d3dxmesh, NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateText(device, hdc, "wine", 0.001f, -FLT_MIN, &d3dxmesh, NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    /* deviation = 0.0f treated as if deviation = 1.0f / otm.otmEMSquare */
    hr = D3DXCreateText(device, hdc, "wine", 1.0f / otm.otmEMSquare, 0.4f, &d3dxmesh, NULL, NULL);
    ok(hr == D3D_OK, "Got result %x, expected %x (D3D_OK)\n", hr, D3D_OK);
    number_of_vertices = d3dxmesh->lpVtbl->GetNumVertices(d3dxmesh);
    number_of_faces = d3dxmesh->lpVtbl->GetNumFaces(d3dxmesh);
    if (SUCCEEDED(hr) && d3dxmesh) d3dxmesh->lpVtbl->Release(d3dxmesh);

    hr = D3DXCreateText(device, hdc, "wine", 0.0f, 0.4f, &d3dxmesh, NULL, NULL);
    ok(hr == D3D_OK, "Got result %x, expected %x (D3D_OK)\n", hr, D3D_OK);
    ok(number_of_vertices == d3dxmesh->lpVtbl->GetNumVertices(d3dxmesh),
       "Got %d vertices, expected %d\n",
       d3dxmesh->lpVtbl->GetNumVertices(d3dxmesh), number_of_vertices);
    ok(number_of_faces == d3dxmesh->lpVtbl->GetNumFaces(d3dxmesh),
       "Got %d faces, expected %d\n",
       d3dxmesh->lpVtbl->GetNumVertices(d3dxmesh), number_of_faces);
    if (SUCCEEDED(hr) && d3dxmesh) d3dxmesh->lpVtbl->Release(d3dxmesh);

#if 0
    /* too much detail requested, so will appear to hang */
    trace("Waiting for D3DXCreateText to finish with deviation = FLT_MIN ...\n");
    hr = D3DXCreateText(device, hdc, "wine", FLT_MIN, 0.4f, &d3dxmesh, NULL, NULL);
    ok(hr == D3D_OK, "Got result %x, expected %x (D3D_OK)\n", hr, D3D_OK);
    if (SUCCEEDED(hr) && d3dxmesh) d3dxmesh->lpVtbl->Release(d3dxmesh);
    trace("D3DXCreateText finish with deviation = FLT_MIN\n");
#endif

    hr = D3DXCreateText(device, hdc, "wine", 0.001f, 0.4f, &d3dxmesh, NULL, NULL);
    ok(hr == D3D_OK, "Got result %x, expected %x (D3D_OK)\n", hr, D3D_OK);
    if (SUCCEEDED(hr) && d3dxmesh) d3dxmesh->lpVtbl->Release(d3dxmesh);

    test_createtext(device, hdc, "wine", FLT_MAX, 0.4f);
    test_createtext(device, hdc, "wine", 0.001f, FLT_MIN);
    test_createtext(device, hdc, "wine", 0.001f, 0.0f);
    test_createtext(device, hdc, "wine", 0.001f, FLT_MAX);
    test_createtext(device, hdc, "wine", 0.0f, 1.0f);

    DeleteDC(hdc);

    IDirect3DDevice9_Release(device);
    IDirect3D9_Release(d3d);
    DestroyWindow(wnd);
}

static void test_get_decl_length(void)
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
    UINT size;

    size = D3DXGetDeclLength(declaration1);
    ok(size == 15, "Got size %u, expected 15.\n", size);

    size = D3DXGetDeclLength(declaration2);
    ok(size == 16, "Got size %u, expected 16.\n", size);
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

static void D3DXGenerateAdjacencyTest(void)
{
    HRESULT hr;
    HWND wnd;
    IDirect3D9 *d3d;
    IDirect3DDevice9 *device;
    D3DPRESENT_PARAMETERS d3dpp;
    ID3DXMesh *d3dxmesh = NULL;
    D3DXVECTOR3 *vertices = NULL;
    WORD *indices = NULL;
    int i;
    struct {
        DWORD num_vertices;
        D3DXVECTOR3 vertices[6];
        DWORD num_faces;
        WORD indices[3 * 3];
        FLOAT epsilon;
        DWORD adjacency[3 * 3];
    } test_data[] = {
        { /* for epsilon < 0, indices must match for faces to be adjacent */
            4, {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {1.0, 1.0, 0.0}, {0.0, 1.0, 0.0}},
            2, {0, 1, 2,  0, 2, 3},
            -1.0,
            {-1, -1, 1,  0, -1, -1},
        },
        {
            6, {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {1.0, 1.0, 0.0}, {0.0, 0.0, 0.0}, {1.0, 1.0, 0.0}, {0.0, 1.0, 0.0}},
            2, {0, 1, 2,  3, 4, 5},
            -1.0,
            {-1, -1, -1,  -1, -1, -1},
        },
        { /* for epsilon == 0, indices or vertices must match for faces to be adjacent */
            6, {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {1.0, 1.0, 0.0}, {0.0, 0.0, 0.0}, {1.0, 1.0, 0.0}, {0.0, 1.0, 0.0}},
            2, {0, 1, 2,  3, 4, 5},
            0.0,
            {-1, -1, 1,  0, -1, -1},
        },
        { /* for epsilon > 0, vertices must be less than (but NOT equal to) epsilon distance away */
            6, {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {1.0, 1.0, 0.0}, {0.0, 0.0, 0.25}, {1.0, 1.0, 0.25}, {0.0, 1.0, 0.25}},
            2, {0, 1, 2,  3, 4, 5},
            0.25,
            {-1, -1, -1,  -1, -1, -1},
        },
        { /* for epsilon > 0, vertices must be less than (but NOT equal to) epsilon distance away */
            6, {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {1.0, 1.0, 0.0}, {0.0, 0.0, 0.25}, {1.0, 1.0, 0.25}, {0.0, 1.0, 0.25}},
            2, {0, 1, 2,  3, 4, 5},
            0.250001,
            {-1, -1, 1,  0, -1, -1},
        },
        { /* length between vertices are compared to epsilon, not the individual dimension deltas */
            6, {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {1.0, 1.0, 0.0}, {0.0, 0.25, 0.25}, {1.0, 1.25, 0.25}, {0.0, 1.25, 0.25}},
            2, {0, 1, 2,  3, 4, 5},
            0.353, /* < sqrt(0.25*0.25 + 0.25*0.25) */
            {-1, -1, -1,  -1, -1, -1},
        },
        {
            6, {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {1.0, 1.0, 0.0}, {0.0, 0.25, 0.25}, {1.0, 1.25, 0.25}, {0.0, 1.25, 0.25}},
            2, {0, 1, 2,  3, 4, 5},
            0.354, /* > sqrt(0.25*0.25 + 0.25*0.25) */
            {-1, -1, 1,  0, -1, -1},
        },
        { /* adjacent faces must have opposite winding orders at the shared edge */
            4, {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {1.0, 1.0, 0.0}, {0.0, 1.0, 0.0}},
            2, {0, 1, 2,  0, 3, 2},
            0.0,
            {-1, -1, -1,  -1, -1, -1},
        },
    };

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

    for (i = 0; i < ARRAY_SIZE(test_data); i++)
    {
        DWORD adjacency[ARRAY_SIZE(test_data[0].adjacency)];
        int j;

        if (d3dxmesh) d3dxmesh->lpVtbl->Release(d3dxmesh);
        d3dxmesh = NULL;

        hr = D3DXCreateMeshFVF(test_data[i].num_faces, test_data[i].num_vertices, 0, D3DFVF_XYZ, device, &d3dxmesh);
        ok(hr == D3D_OK, "Got result %x, expected %x (D3D_OK)\n", hr, D3D_OK);

        hr = d3dxmesh->lpVtbl->LockVertexBuffer(d3dxmesh, D3DLOCK_DISCARD, (void**)&vertices);
        ok(hr == D3D_OK, "test %d: Got result %x, expected %x (D3D_OK)\n", i, hr, D3D_OK);
        if (FAILED(hr)) continue;
        CopyMemory(vertices, test_data[i].vertices, test_data[i].num_vertices * sizeof(test_data[0].vertices[0]));
        d3dxmesh->lpVtbl->UnlockVertexBuffer(d3dxmesh);

        hr = d3dxmesh->lpVtbl->LockIndexBuffer(d3dxmesh, D3DLOCK_DISCARD, (void**)&indices);
        ok(hr == D3D_OK, "test %d: Got result %x, expected %x (D3D_OK)\n", i, hr, D3D_OK);
        if (FAILED(hr)) continue;
        CopyMemory(indices, test_data[i].indices, test_data[i].num_faces * 3 * sizeof(test_data[0].indices[0]));
        d3dxmesh->lpVtbl->UnlockIndexBuffer(d3dxmesh);

        if (i == 0) {
            hr = d3dxmesh->lpVtbl->GenerateAdjacency(d3dxmesh, 0.0f, NULL);
            ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);
        }

        hr = d3dxmesh->lpVtbl->GenerateAdjacency(d3dxmesh, test_data[i].epsilon, adjacency);
        ok(hr == D3D_OK, "Got result %x, expected %x (D3D_OK)\n", hr, D3D_OK);
        if (FAILED(hr)) continue;

        for (j = 0; j < test_data[i].num_faces * 3; j++)
            ok(adjacency[j] == test_data[i].adjacency[j],
               "Test %d adjacency %d: Got result %u, expected %u\n", i, j,
               adjacency[j], test_data[i].adjacency[j]);
    }
    if (d3dxmesh) d3dxmesh->lpVtbl->Release(d3dxmesh);
}

static void test_update_semantics(void)
{
    HRESULT hr;
    struct test_context *test_context = NULL;
    ID3DXMesh *mesh = NULL;
    D3DVERTEXELEMENT9 declaration0[] =
    {
         {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
         {0, 24, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
         {0, 36, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
         D3DDECL_END()
    };
    D3DVERTEXELEMENT9 declaration_pos_type_color[] =
    {
         {0, 0, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
         {0, 24, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
         {0, 36, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
         D3DDECL_END()
    };
    D3DVERTEXELEMENT9 declaration_smaller[] =
    {
         {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
         {0, 24, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
         D3DDECL_END()
    };
    D3DVERTEXELEMENT9 declaration_larger[] =
    {
         {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
         {0, 24, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
         {0, 36, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
         {0, 40, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TANGENT, 0},
         D3DDECL_END()
    };
    D3DVERTEXELEMENT9 declaration_multiple_streams[] =
    {
         {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
         {1, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TANGENT, 0},
         {0, 24, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
         {0, 36, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},

         D3DDECL_END()
    };
    D3DVERTEXELEMENT9 declaration_double_usage[] =
    {
         {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
         {0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
         {0, 24, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
         {0, 36, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
         D3DDECL_END()
    };
    D3DVERTEXELEMENT9 declaration_undefined_type[] =
    {
         {0, 0, D3DDECLTYPE_UNUSED+1, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
         {0, 24, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
         {0, 36, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
         D3DDECL_END()
    };
    D3DVERTEXELEMENT9 declaration_not_4_byte_aligned_offset[] =
    {
         {0, 3, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
         {0, 24, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
         {0, 36, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
         D3DDECL_END()
    };
    static const struct
    {
        D3DXVECTOR3 position0;
        D3DXVECTOR3 position1;
        D3DXVECTOR3 normal;
        DWORD color;
    }
    vertices[] =
    {
        { { 0.0f,  1.0f,  0.f}, { 1.0f,  0.0f,  0.f}, {0.0f, 0.0f, 1.0f}, 0xffff0000 },
        { { 1.0f, -1.0f,  0.f}, {-1.0f, -1.0f,  0.f}, {0.0f, 0.0f, 1.0f}, 0xff00ff00 },
        { {-1.0f, -1.0f,  0.f}, {-1.0f,  1.0f,  0.f}, {0.0f, 0.0f, 1.0f}, 0xff0000ff },
    };
    unsigned int faces[] = {0, 1, 2};
    unsigned int attributes[] = {0};
    unsigned int num_faces = ARRAY_SIZE(faces) / 3;
    unsigned int num_vertices = ARRAY_SIZE(vertices);
    int offset = sizeof(D3DXVECTOR3);
    DWORD options = D3DXMESH_32BIT | D3DXMESH_SYSTEMMEM;
    void *vertex_buffer;
    void *index_buffer;
    DWORD *attributes_buffer;
    D3DVERTEXELEMENT9 declaration[MAX_FVF_DECL_SIZE];
    D3DVERTEXELEMENT9 *decl_ptr;
    DWORD exp_vertex_size = sizeof(*vertices);
    DWORD vertex_size = 0;
    int equal;
    int i = 0;
    int *decl_mem;
    int filler_a = 0xaaaaaaaa;
    int filler_b = 0xbbbbbbbb;

    test_context = new_test_context();
    if (!test_context)
    {
        skip("Couldn't create a test_context\n");
        goto cleanup;
    }

    hr = D3DXCreateMesh(num_faces, num_vertices, options, declaration0,
                        test_context->device, &mesh);
    if (FAILED(hr))
    {
        skip("Couldn't create test mesh %#x\n", hr);
        goto cleanup;
    }

    mesh->lpVtbl->LockVertexBuffer(mesh, 0, &vertex_buffer);
    memcpy(vertex_buffer, vertices, sizeof(vertices));
    mesh->lpVtbl->UnlockVertexBuffer(mesh);

    mesh->lpVtbl->LockIndexBuffer(mesh, 0, &index_buffer);
    memcpy(index_buffer, faces, sizeof(faces));
    mesh->lpVtbl->UnlockIndexBuffer(mesh);

    mesh->lpVtbl->LockAttributeBuffer(mesh, 0, &attributes_buffer);
    memcpy(attributes_buffer, attributes, sizeof(attributes));
    mesh->lpVtbl->UnlockAttributeBuffer(mesh);

    /* Get the declaration and try to change it */
    hr = mesh->lpVtbl->GetDeclaration(mesh, declaration);
    if (FAILED(hr))
    {
        skip("Couldn't get vertex declaration %#x\n", hr);
        goto cleanup;
    }
    equal = memcmp(declaration, declaration0, sizeof(declaration0));
    ok(equal == 0, "Vertex declarations were not equal\n");

    for (decl_ptr = declaration; decl_ptr->Stream != 0xFF; decl_ptr++)
    {
        if (decl_ptr->Usage == D3DDECLUSAGE_POSITION)
        {
            /* Use second vertex position instead of first */
            decl_ptr->Offset = offset;
        }
    }

    hr = mesh->lpVtbl->UpdateSemantics(mesh, declaration);
    ok(hr == D3D_OK, "Test UpdateSematics, got %#x expected %#x\n", hr, D3D_OK);

    /* Check that declaration was written by getting it again */
    memset(declaration, 0, sizeof(declaration));
    hr = mesh->lpVtbl->GetDeclaration(mesh, declaration);
    if (FAILED(hr))
    {
        skip("Couldn't get vertex declaration %#x\n", hr);
        goto cleanup;
    }

    for (decl_ptr = declaration; decl_ptr->Stream != 0xFF; decl_ptr++)
    {
        if (decl_ptr->Usage == D3DDECLUSAGE_POSITION)
        {
            ok(decl_ptr->Offset == offset, "Test UpdateSematics, got offset %d expected %d\n",
               decl_ptr->Offset, offset);
        }
    }

    /* Check that GetDeclaration only writes up to the D3DDECL_END() marker and
     * not the full MAX_FVF_DECL_SIZE elements.
     */
    memset(declaration, filler_a, sizeof(declaration));
    memcpy(declaration, declaration0, sizeof(declaration0));
    hr = mesh->lpVtbl->UpdateSemantics(mesh, declaration);
    ok(hr == D3D_OK, "Test UpdateSematics, "
       "got %#x expected D3D_OK\n", hr);
    memset(declaration, filler_b, sizeof(declaration));
    hr = mesh->lpVtbl->GetDeclaration(mesh, declaration);
    ok(hr == D3D_OK, "Couldn't get vertex declaration. Got %#x, expected D3D_OK\n", hr);
    decl_mem = (int*)declaration;
    for (i = sizeof(declaration0)/sizeof(*decl_mem); i < sizeof(declaration)/sizeof(*decl_mem); i++)
    {
        equal = memcmp(&decl_mem[i], &filler_b, sizeof(filler_b));
        ok(equal == 0,
           "GetDeclaration wrote past the D3DDECL_END() marker. "
           "Got %#x, expected  %#x\n", decl_mem[i], filler_b);
        if (equal != 0) break;
    }

    /* UpdateSemantics does not check for overlapping fields */
    memset(declaration, 0, sizeof(declaration));
    hr = mesh->lpVtbl->GetDeclaration(mesh, declaration);
    if (FAILED(hr))
    {
        skip("Couldn't get vertex declaration %#x\n", hr);
        goto cleanup;
    }

    for (decl_ptr = declaration; decl_ptr->Stream != 0xFF; decl_ptr++)
    {
        if (decl_ptr->Type == D3DDECLTYPE_FLOAT3)
        {
            decl_ptr->Type = D3DDECLTYPE_FLOAT4;
        }
    }

    hr = mesh->lpVtbl->UpdateSemantics(mesh, declaration);
    ok(hr == D3D_OK, "Test UpdateSematics for overlapping fields, "
       "got %#x expected D3D_OK\n", hr);

    /* Set the position type to color instead of float3 */
    hr = mesh->lpVtbl->UpdateSemantics(mesh, declaration_pos_type_color);
    ok(hr == D3D_OK, "Test UpdateSematics position type color, "
       "got %#x expected D3D_OK\n", hr);

    /* The following test cases show that NULL, smaller or larger declarations,
     * and declarations with non-zero Stream values are not accepted.
     * UpdateSemantics returns D3DERR_INVALIDCALL and the previously set
     * declaration will be used by DrawSubset, GetNumBytesPerVertex, and
     * GetDeclaration.
     */

    /* Null declaration (invalid declaration) */
    mesh->lpVtbl->UpdateSemantics(mesh, declaration0); /* Set a valid declaration */
    hr = mesh->lpVtbl->UpdateSemantics(mesh, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Test UpdateSematics null pointer declaration, "
       "got %#x expected D3DERR_INVALIDCALL\n", hr);
    vertex_size = mesh->lpVtbl->GetNumBytesPerVertex(mesh);
    ok(vertex_size == exp_vertex_size, "Got vertex declaration size %u, expected %u\n",
       vertex_size, exp_vertex_size);
    memset(declaration, 0, sizeof(declaration));
    hr = mesh->lpVtbl->GetDeclaration(mesh, declaration);
    ok(hr == D3D_OK, "Couldn't get vertex declaration. Got %#x, expected D3D_OK\n", hr);
    equal = memcmp(declaration, declaration0, sizeof(declaration0));
    ok(equal == 0, "Vertex declarations were not equal\n");

    /* Smaller vertex declaration (invalid declaration) */
    mesh->lpVtbl->UpdateSemantics(mesh, declaration0); /* Set a valid declaration */
    hr = mesh->lpVtbl->UpdateSemantics(mesh, declaration_smaller);
    ok(hr == D3DERR_INVALIDCALL, "Test UpdateSematics for smaller vertex declaration, "
       "got %#x expected D3DERR_INVALIDCALL\n", hr);
    vertex_size = mesh->lpVtbl->GetNumBytesPerVertex(mesh);
    ok(vertex_size == exp_vertex_size, "Got vertex declaration size %u, expected %u\n",
       vertex_size, exp_vertex_size);
    memset(declaration, 0, sizeof(declaration));
    hr = mesh->lpVtbl->GetDeclaration(mesh, declaration);
    ok(hr == D3D_OK, "Couldn't get vertex declaration. Got %#x, expected D3D_OK\n", hr);
    equal = memcmp(declaration, declaration0, sizeof(declaration0));
    ok(equal == 0, "Vertex declarations were not equal\n");

    /* Larger vertex declaration (invalid declaration) */
    mesh->lpVtbl->UpdateSemantics(mesh, declaration0); /* Set a valid declaration */
    hr = mesh->lpVtbl->UpdateSemantics(mesh, declaration_larger);
    ok(hr == D3DERR_INVALIDCALL, "Test UpdateSematics for larger vertex declaration, "
       "got %#x expected D3DERR_INVALIDCALL\n", hr);
    vertex_size = mesh->lpVtbl->GetNumBytesPerVertex(mesh);
    ok(vertex_size == exp_vertex_size, "Got vertex declaration size %u, expected %u\n",
       vertex_size, exp_vertex_size);
    memset(declaration, 0, sizeof(declaration));
    hr = mesh->lpVtbl->GetDeclaration(mesh, declaration);
    ok(hr == D3D_OK, "Couldn't get vertex declaration. Got %#x, expected D3D_OK\n", hr);
    equal = memcmp(declaration, declaration0, sizeof(declaration0));
    ok(equal == 0, "Vertex declarations were not equal\n");

    /* Use multiple streams and keep the same vertex size (invalid declaration) */
    mesh->lpVtbl->UpdateSemantics(mesh, declaration0); /* Set a valid declaration */
    hr = mesh->lpVtbl->UpdateSemantics(mesh, declaration_multiple_streams);
    ok(hr == D3DERR_INVALIDCALL, "Test UpdateSematics using multiple streams, "
                 "got %#x expected D3DERR_INVALIDCALL\n", hr);
    vertex_size = mesh->lpVtbl->GetNumBytesPerVertex(mesh);
    ok(vertex_size == exp_vertex_size, "Got vertex declaration size %u, expected %u\n",
       vertex_size, exp_vertex_size);
    memset(declaration, 0, sizeof(declaration));
    hr = mesh->lpVtbl->GetDeclaration(mesh, declaration);
    ok(hr == D3D_OK, "Couldn't get vertex declaration. Got %#x, expected D3D_OK\n", hr);
    equal = memcmp(declaration, declaration0, sizeof(declaration0));
    ok(equal == 0, "Vertex declarations were not equal\n");

    /* The next following test cases show that some invalid declarations are
     * accepted with a D3D_OK. An access violation is thrown on Windows if
     * DrawSubset is called. The methods GetNumBytesPerVertex and GetDeclaration
     * are not affected, which indicates that the declaration is cached.
     */

    /* Double usage (invalid declaration) */
    mesh->lpVtbl->UpdateSemantics(mesh, declaration0); /* Set a valid declaration */
    hr = mesh->lpVtbl->UpdateSemantics(mesh, declaration_double_usage);
    ok(hr == D3D_OK, "Test UpdateSematics double usage, "
       "got %#x expected D3D_OK\n", hr);
    vertex_size = mesh->lpVtbl->GetNumBytesPerVertex(mesh);
    ok(vertex_size == exp_vertex_size, "Got vertex declaration size %u, expected %u\n",
       vertex_size, exp_vertex_size);
    memset(declaration, 0, sizeof(declaration));
    hr = mesh->lpVtbl->GetDeclaration(mesh, declaration);
    ok(hr == D3D_OK, "Couldn't get vertex declaration. Got %#x, expected D3D_OK\n", hr);
    equal = memcmp(declaration, declaration_double_usage, sizeof(declaration_double_usage));
    ok(equal == 0, "Vertex declarations were not equal\n");

    /* Set the position to an undefined type (invalid declaration) */
    mesh->lpVtbl->UpdateSemantics(mesh, declaration0); /* Set a valid declaration */
    hr = mesh->lpVtbl->UpdateSemantics(mesh, declaration_undefined_type);
    ok(hr == D3D_OK, "Test UpdateSematics undefined type, "
       "got %#x expected D3D_OK\n", hr);
    vertex_size = mesh->lpVtbl->GetNumBytesPerVertex(mesh);
    ok(vertex_size == exp_vertex_size, "Got vertex declaration size %u, expected %u\n",
       vertex_size, exp_vertex_size);
    memset(declaration, 0, sizeof(declaration));
    hr = mesh->lpVtbl->GetDeclaration(mesh, declaration);
    ok(hr == D3D_OK, "Couldn't get vertex declaration. Got %#x, expected D3D_OK\n", hr);
    equal = memcmp(declaration, declaration_undefined_type, sizeof(declaration_undefined_type));
    ok(equal == 0, "Vertex declarations were not equal\n");

    /* Use a not 4 byte aligned offset (invalid declaration) */
    mesh->lpVtbl->UpdateSemantics(mesh, declaration0); /* Set a valid declaration */
    hr = mesh->lpVtbl->UpdateSemantics(mesh, declaration_not_4_byte_aligned_offset);
    ok(hr == D3D_OK, "Test UpdateSematics not 4 byte aligned offset, "
       "got %#x expected D3D_OK\n", hr);
    vertex_size = mesh->lpVtbl->GetNumBytesPerVertex(mesh);
    ok(vertex_size == exp_vertex_size, "Got vertex declaration size %u, expected %u\n",
       vertex_size, exp_vertex_size);
    memset(declaration, 0, sizeof(declaration));
    hr = mesh->lpVtbl->GetDeclaration(mesh, declaration);
    ok(hr == D3D_OK, "Couldn't get vertex declaration. Got %#x, expected D3D_OK\n", hr);
    equal = memcmp(declaration, declaration_not_4_byte_aligned_offset,
                   sizeof(declaration_not_4_byte_aligned_offset));
    ok(equal == 0, "Vertex declarations were not equal\n");

cleanup:
    if (mesh)
        mesh->lpVtbl->Release(mesh);

    free_test_context(test_context);
}

static void test_create_skin_info(void)
{
    HRESULT hr;
    ID3DXSkinInfo *skininfo = NULL;
    D3DVERTEXELEMENT9 empty_declaration[] = { D3DDECL_END() };
    D3DVERTEXELEMENT9 declaration_out[MAX_FVF_DECL_SIZE];
    const D3DVERTEXELEMENT9 declaration_with_nonzero_stream[] = {
        {1, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0},
        D3DDECL_END()
    };

    hr = D3DXCreateSkinInfo(0, empty_declaration, 0, &skininfo);
    ok(hr == D3D_OK, "Expected D3D_OK, got %#x\n", hr);
    if (skininfo) IUnknown_Release(skininfo);
    skininfo = NULL;

    hr = D3DXCreateSkinInfo(1, NULL, 1, &skininfo);
    ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#x\n", hr);

    hr = D3DXCreateSkinInfo(1, declaration_with_nonzero_stream, 1, &skininfo);
    ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#x\n", hr);

    hr = D3DXCreateSkinInfoFVF(1, 0, 1, &skininfo);
    ok(hr == D3D_OK, "Expected D3D_OK, got %#x\n", hr);
    if (skininfo) {
        DWORD dword_result;
        FLOAT flt_result;
        LPCSTR string_result;
        D3DXMATRIX *transform;
        D3DXMATRIX identity_matrix;

        /* test initial values */
        hr = skininfo->lpVtbl->GetDeclaration(skininfo, declaration_out);
        ok(hr == D3D_OK, "Expected D3D_OK, got %#x\n", hr);
        if (SUCCEEDED(hr))
            compare_elements(declaration_out, empty_declaration, __LINE__, 0);

        dword_result = skininfo->lpVtbl->GetNumBones(skininfo);
        ok(dword_result == 1, "Expected 1, got %u\n", dword_result);

        flt_result = skininfo->lpVtbl->GetMinBoneInfluence(skininfo);
        ok(flt_result == 0.0f, "Expected 0.0, got %g\n", flt_result);

        string_result = skininfo->lpVtbl->GetBoneName(skininfo, 0);
        ok(string_result == NULL, "Expected NULL, got %p\n", string_result);

        dword_result = skininfo->lpVtbl->GetFVF(skininfo);
        ok(dword_result == 0, "Expected 0, got %u\n", dword_result);

        dword_result = skininfo->lpVtbl->GetNumBoneInfluences(skininfo, 0);
        ok(dword_result == 0, "Expected 0, got %u\n", dword_result);

        dword_result = skininfo->lpVtbl->GetNumBoneInfluences(skininfo, 1);
        ok(dword_result == 0, "Expected 0, got %u\n", dword_result);

        transform = skininfo->lpVtbl->GetBoneOffsetMatrix(skininfo, -1);
        ok(transform == NULL, "Expected NULL, got %p\n", transform);

        {
            /* test [GS]etBoneOffsetMatrix */
            hr = skininfo->lpVtbl->SetBoneOffsetMatrix(skininfo, 1, &identity_matrix);
            ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#x\n", hr);

            hr = skininfo->lpVtbl->SetBoneOffsetMatrix(skininfo, 0, NULL);
            ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#x\n", hr);

            D3DXMatrixIdentity(&identity_matrix);
            hr = skininfo->lpVtbl->SetBoneOffsetMatrix(skininfo, 0, &identity_matrix);
            ok(hr == D3D_OK, "Expected D3D_OK, got %#x\n", hr);

            transform = skininfo->lpVtbl->GetBoneOffsetMatrix(skininfo, 0);
            check_matrix(transform, &identity_matrix);
        }

        {
            /* test [GS]etBoneName */
            const char *name_in = "testBoneName";
            const char *string_result2;

            hr = skininfo->lpVtbl->SetBoneName(skininfo, 1, name_in);
            ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#x\n", hr);

            hr = skininfo->lpVtbl->SetBoneName(skininfo, 0, NULL);
            ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#x\n", hr);

            hr = skininfo->lpVtbl->SetBoneName(skininfo, 0, name_in);
            ok(hr == D3D_OK, "Expected D3D_OK, got %#x\n", hr);

            string_result = skininfo->lpVtbl->GetBoneName(skininfo, 0);
            ok(string_result != NULL, "Expected non-NULL string, got %p\n", string_result);
            ok(!strcmp(string_result, name_in), "Expected '%s', got '%s'\n", name_in, string_result);

            string_result2 = skininfo->lpVtbl->GetBoneName(skininfo, 0);
            ok(string_result == string_result2, "Expected %p, got %p\n", string_result, string_result2);

            string_result = skininfo->lpVtbl->GetBoneName(skininfo, 1);
            ok(string_result == NULL, "Expected NULL, got %p\n", string_result);
        }

        {
            /* test [GS]etBoneInfluence */
            DWORD vertices[2];
            FLOAT weights[2];
            int i;
            DWORD num_influences;
            DWORD exp_vertices[2];
            FLOAT exp_weights[2];

            /* vertex and weight arrays untouched when num_influences is 0 */
            vertices[0] = 0xdeadbeef;
            weights[0] = FLT_MAX;
            hr = skininfo->lpVtbl->GetBoneInfluence(skininfo, 0, vertices, weights);
            ok(hr == D3D_OK, "Expected D3D_OK, got %#x\n", hr);
            ok(vertices[0] == 0xdeadbeef, "expected 0xdeadbeef, got %#x\n", vertices[0]);
            ok(weights[0] == FLT_MAX, "expected %g, got %g\n", FLT_MAX, weights[0]);

            hr = skininfo->lpVtbl->GetBoneInfluence(skininfo, 1, vertices, weights);
            ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#x\n", hr);

            hr = skininfo->lpVtbl->GetBoneInfluence(skininfo, 0, NULL, NULL);
            ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#x\n", hr);

            hr = skininfo->lpVtbl->GetBoneInfluence(skininfo, 0, vertices, NULL);
            ok(hr == D3D_OK, "Expected D3D_OK, got %#x\n", hr);

            hr = skininfo->lpVtbl->GetBoneInfluence(skininfo, 0, NULL, weights);
            ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#x\n", hr);


            /* no vertex or weight value checking */
            exp_vertices[0] = 0;
            exp_vertices[1] = 0x87654321;
            exp_weights[0] = 0.5;
            exp_weights[1] = 0.0f / 0.0f; /* NAN */
            num_influences = 2;

            hr = skininfo->lpVtbl->SetBoneInfluence(skininfo, 1, num_influences, vertices, weights);
            ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#x\n", hr);

            hr = skininfo->lpVtbl->SetBoneInfluence(skininfo, 0, num_influences, NULL, weights);
            ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#x\n", hr);

            hr = skininfo->lpVtbl->SetBoneInfluence(skininfo, 0, num_influences, vertices, NULL);
            ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#x\n", hr);

            hr = skininfo->lpVtbl->SetBoneInfluence(skininfo, 0, num_influences, NULL, NULL);
            ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#x\n", hr);

            hr = skininfo->lpVtbl->SetBoneInfluence(skininfo, 0, num_influences, exp_vertices, exp_weights);
            ok(hr == D3D_OK, "Expected D3D_OK, got %#x\n", hr);

            memset(vertices, 0, sizeof(vertices));
            memset(weights, 0, sizeof(weights));
            hr = skininfo->lpVtbl->GetBoneInfluence(skininfo, 0, vertices, weights);
            ok(hr == D3D_OK, "Expected D3D_OK, got %#x\n", hr);
            for (i = 0; i < num_influences; i++) {
                ok(exp_vertices[i] == vertices[i],
                   "influence[%d]: expected vertex %u, got %u\n", i, exp_vertices[i], vertices[i]);
                ok((isnan(exp_weights[i]) && isnan(weights[i])) || exp_weights[i] == weights[i],
                   "influence[%d]: expected weights %g, got %g\n", i, exp_weights[i], weights[i]);
            }

            /* vertices and weights aren't returned after setting num_influences to 0 */
            memset(vertices, 0, sizeof(vertices));
            memset(weights, 0, sizeof(weights));
            hr = skininfo->lpVtbl->SetBoneInfluence(skininfo, 0, 0, vertices, weights);
            ok(hr == D3D_OK, "Expected D3D_OK, got %#x\n", hr);

            vertices[0] = 0xdeadbeef;
            weights[0] = FLT_MAX;
            hr = skininfo->lpVtbl->GetBoneInfluence(skininfo, 0, vertices, weights);
            ok(hr == D3D_OK, "Expected D3D_OK, got %#x\n", hr);
            ok(vertices[0] == 0xdeadbeef, "expected vertex 0xdeadbeef, got %u\n", vertices[0]);
            ok(weights[0] == FLT_MAX, "expected weight %g, got %g\n", FLT_MAX, weights[0]);
        }

        {
            /* test [GS]etFVF and [GS]etDeclaration */
            D3DVERTEXELEMENT9 declaration_in[MAX_FVF_DECL_SIZE];
            DWORD fvf = D3DFVF_XYZ;
            DWORD got_fvf;

            hr = skininfo->lpVtbl->SetDeclaration(skininfo, NULL);
            ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#x\n", hr);

            hr = skininfo->lpVtbl->SetDeclaration(skininfo, declaration_with_nonzero_stream);
            ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#x\n", hr);

            hr = skininfo->lpVtbl->SetFVF(skininfo, 0);
            ok(hr == D3D_OK, "Expected D3D_OK, got %#x\n", hr);

            hr = D3DXDeclaratorFromFVF(fvf, declaration_in);
            ok(hr == D3D_OK, "Expected D3D_OK, got %#x\n", hr);
            hr = skininfo->lpVtbl->SetDeclaration(skininfo, declaration_in);
            ok(hr == D3D_OK, "Expected D3D_OK, got %#x\n", hr);
            got_fvf = skininfo->lpVtbl->GetFVF(skininfo);
            ok(fvf == got_fvf, "Expected %#x, got %#x\n", fvf, got_fvf);
            hr = skininfo->lpVtbl->GetDeclaration(skininfo, declaration_out);
            ok(hr == D3D_OK, "Expected D3D_OK, got %#x\n", hr);
            compare_elements(declaration_out, declaration_in, __LINE__, 0);

            hr = skininfo->lpVtbl->SetDeclaration(skininfo, empty_declaration);
            ok(hr == D3D_OK, "Expected D3D_OK, got %#x\n", hr);
            got_fvf = skininfo->lpVtbl->GetFVF(skininfo);
            ok(got_fvf == 0, "Expected 0, got %#x\n", got_fvf);
            hr = skininfo->lpVtbl->GetDeclaration(skininfo, declaration_out);
            ok(hr == D3D_OK, "Expected D3D_OK, got %#x\n", hr);
            compare_elements(declaration_out, empty_declaration, __LINE__, 0);

            hr = skininfo->lpVtbl->SetFVF(skininfo, fvf);
            ok(hr == D3D_OK, "Expected D3D_OK, got %#x\n", hr);
            got_fvf = skininfo->lpVtbl->GetFVF(skininfo);
            ok(fvf == got_fvf, "Expected %#x, got %#x\n", fvf, got_fvf);
            hr = skininfo->lpVtbl->GetDeclaration(skininfo, declaration_out);
            ok(hr == D3D_OK, "Expected D3D_OK, got %#x\n", hr);
            compare_elements(declaration_out, declaration_in, __LINE__, 0);
        }
    }
    if (skininfo) IUnknown_Release(skininfo);
    skininfo = NULL;

    hr = D3DXCreateSkinInfoFVF(1, D3DFVF_XYZ, 1, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#x\n", hr);

    hr = D3DXCreateSkinInfo(1, NULL, 1, &skininfo);
    ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %#x\n", hr);
}

static void test_convert_adjacency_to_point_reps(void)
{
    HRESULT hr;
    struct test_context *test_context = NULL;
    const DWORD options = D3DXMESH_32BIT | D3DXMESH_SYSTEMMEM;
    const DWORD options_16bit = D3DXMESH_SYSTEMMEM;
    const D3DVERTEXELEMENT9 declaration[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
        {0, 24, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
        D3DDECL_END()
    };
    const unsigned int VERTS_PER_FACE = 3;
    void *vertex_buffer;
    void *index_buffer;
    DWORD *attributes_buffer;
    int i, j;
    enum color { RED = 0xffff0000, GREEN = 0xff00ff00, BLUE = 0xff0000ff};
    struct vertex_pnc
    {
        D3DXVECTOR3 position;
        D3DXVECTOR3 normal;
        enum color color; /* In case of manual visual inspection */
    };
    D3DXVECTOR3 up = {0.0f, 0.0f, 1.0f};
    /* mesh0 (one face)
     *
     * 0--1
     * | /
     * |/
     * 2
     */
    const struct vertex_pnc vertices0[] =
    {
        {{ 0.0f,  3.0f,  0.f}, up, RED},
        {{ 2.0f,  3.0f,  0.f}, up, GREEN},
        {{ 0.0f,  0.0f,  0.f}, up, BLUE},
    };
    const DWORD indices0[] = {0, 1, 2};
    const unsigned int num_vertices0 = ARRAY_SIZE(vertices0);
    const unsigned int num_faces0 = ARRAY_SIZE(indices0) / VERTS_PER_FACE;
    const DWORD adjacency0[] = {-1, -1, -1};
    const DWORD exp_point_rep0[] = {0, 1, 2};
    /* mesh1 (right)
     *
     * 0--1 3
     * | / /|
     * |/ / |
     * 2 5--4
     */
    const struct vertex_pnc vertices1[] =
    {
        {{ 0.0f,  3.0f,  0.f}, up, RED},
        {{ 2.0f,  3.0f,  0.f}, up, GREEN},
        {{ 0.0f,  0.0f,  0.f}, up, BLUE},

        {{ 3.0f,  3.0f,  0.f}, up, GREEN},
        {{ 3.0f,  0.0f,  0.f}, up, RED},
        {{ 1.0f,  0.0f,  0.f}, up, BLUE},
    };
    const DWORD indices1[] = {0, 1, 2, 3, 4, 5};
    const unsigned int num_vertices1 = ARRAY_SIZE(vertices1);
    const unsigned int num_faces1 = ARRAY_SIZE(indices1) / VERTS_PER_FACE;
    const DWORD adjacency1[] = {-1, 1, -1, -1, -1, 0};
    const DWORD exp_point_rep1[] = {0, 1, 2, 1, 4, 2};
    /* mesh2 (left)
     *
     *    3 0--1
     *   /| | /
     *  / | |/
     * 5--4 2
     */
    const struct vertex_pnc vertices2[] =
    {
        {{ 0.0f,  3.0f,  0.f}, up, RED},
        {{ 2.0f,  3.0f,  0.f}, up, GREEN},
        {{ 0.0f,  0.0f,  0.f}, up, BLUE},

        {{-1.0f,  3.0f,  0.f}, up, RED},
        {{-1.0f,  0.0f,  0.f}, up, GREEN},
        {{-3.0f,  0.0f,  0.f}, up, BLUE},
    };
    const DWORD indices2[] = {0, 1, 2, 3, 4, 5};
    const unsigned int num_vertices2 = ARRAY_SIZE(vertices2);
    const unsigned int num_faces2 = ARRAY_SIZE(indices2) / VERTS_PER_FACE;
    const DWORD adjacency2[] = {-1, -1, 1, 0, -1, -1};
    const DWORD exp_point_rep2[] = {0, 1, 2, 0, 2, 5};
    /* mesh3 (above)
     *
     *    3
     *   /|
     *  / |
     * 5--4
     * 0--1
     * | /
     * |/
     * 2
     */
    struct vertex_pnc vertices3[] =
    {
        {{ 0.0f,  3.0f,  0.f}, up, RED},
        {{ 2.0f,  3.0f,  0.f}, up, GREEN},
        {{ 0.0f,  0.0f,  0.f}, up, BLUE},

        {{ 2.0f,  7.0f,  0.f}, up, BLUE},
        {{ 2.0f,  4.0f,  0.f}, up, GREEN},
        {{ 0.0f,  4.0f,  0.f}, up, RED},
    };
    const DWORD indices3[] = {0, 1, 2, 3, 4, 5};
    const unsigned int num_vertices3 = ARRAY_SIZE(vertices3);
    const unsigned int num_faces3 = ARRAY_SIZE(indices3) / VERTS_PER_FACE;
    const DWORD adjacency3[] = {1, -1, -1, -1, 0, -1};
    const DWORD exp_point_rep3[] = {0, 1, 2, 3, 1, 0};
    /* mesh4 (below, tip against tip)
     *
     * 0--1
     * | /
     * |/
     * 2
     * 3
     * |\
     * | \
     * 5--4
     */
    struct vertex_pnc vertices4[] =
    {
        {{ 0.0f,  3.0f,  0.f}, up, RED},
        {{ 2.0f,  3.0f,  0.f}, up, GREEN},
        {{ 0.0f,  0.0f,  0.f}, up, BLUE},

        {{ 0.0f, -4.0f,  0.f}, up, BLUE},
        {{ 2.0f, -7.0f,  0.f}, up, GREEN},
        {{ 0.0f, -7.0f,  0.f}, up, RED},
    };
    const DWORD indices4[] = {0, 1, 2, 3, 4, 5};
    const unsigned int num_vertices4 = ARRAY_SIZE(vertices4);
    const unsigned int num_faces4 = ARRAY_SIZE(indices4) / VERTS_PER_FACE;
    const DWORD adjacency4[] = {-1, -1, -1, -1, -1, -1};
    const DWORD exp_point_rep4[] = {0, 1, 2, 3, 4, 5};
    /* mesh5 (gap in mesh)
     *
     *    0      3-----4  15
     *   / \      \   /  /  \
     *  /   \      \ /  /    \
     * 2-----1      5 17-----16
     * 6-----7      9 12-----13
     *  \   /      / \  \    /
     *   \ /      /   \  \  /
     *    8     10-----11 14
     *
     */
    const struct vertex_pnc vertices5[] =
    {
        {{ 0.0f,  1.0f,  0.f}, up, RED},
        {{ 1.0f, -1.0f,  0.f}, up, GREEN},
        {{-1.0f, -1.0f,  0.f}, up, BLUE},

        {{ 0.1f,  1.0f,  0.f}, up, RED},
        {{ 2.1f,  1.0f,  0.f}, up, BLUE},
        {{ 1.1f, -1.0f,  0.f}, up, GREEN},

        {{-1.0f, -1.1f,  0.f}, up, BLUE},
        {{ 1.0f, -1.1f,  0.f}, up, GREEN},
        {{ 0.0f, -3.1f,  0.f}, up, RED},

        {{ 1.1f, -1.1f,  0.f}, up, GREEN},
        {{ 2.1f, -3.1f,  0.f}, up, BLUE},
        {{ 0.1f, -3.1f,  0.f}, up, RED},

        {{ 1.2f, -1.1f,  0.f}, up, GREEN},
        {{ 3.2f, -1.1f,  0.f}, up, RED},
        {{ 2.2f, -3.1f,  0.f}, up, BLUE},

        {{ 2.2f,  1.0f,  0.f}, up, BLUE},
        {{ 3.2f, -1.0f,  0.f}, up, RED},
        {{ 1.2f, -1.0f,  0.f}, up, GREEN},
    };
    const DWORD indices5[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17};
    const unsigned int num_vertices5 = ARRAY_SIZE(vertices5);
    const unsigned int num_faces5 = ARRAY_SIZE(indices5) / VERTS_PER_FACE;
    const DWORD adjacency5[] = {-1, 2, -1, -1, 5, -1, 0, -1, -1, 4, -1, -1, 5, -1, 3, -1, 4, 1};
    const DWORD exp_point_rep5[] = {0, 1, 2, 3, 4, 5, 2, 1, 8, 5, 10, 11, 5, 13, 10, 4, 13, 5};
    const WORD indices5_16bit[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17};
    /* mesh6 (indices re-ordering)
     *
     * 0--1 6 3
     * | / /| |\
     * |/ / | | \
     * 2 8--7 5--4
     */
    const struct vertex_pnc vertices6[] =
    {
        {{ 0.0f,  3.0f,  0.f}, up, RED},
        {{ 2.0f,  3.0f,  0.f}, up, GREEN},
        {{ 0.0f,  0.0f,  0.f}, up, BLUE},

        {{ 3.0f,  3.0f,  0.f}, up, GREEN},
        {{ 3.0f,  0.0f,  0.f}, up, RED},
        {{ 1.0f,  0.0f,  0.f}, up, BLUE},

        {{ 4.0f,  3.0f,  0.f}, up, GREEN},
        {{ 6.0f,  0.0f,  0.f}, up, BLUE},
        {{ 4.0f,  0.0f,  0.f}, up, RED},
    };
    const DWORD indices6[] = {0, 1, 2, 6, 7, 8, 3, 4, 5};
    const unsigned int num_vertices6 = ARRAY_SIZE(vertices6);
    const unsigned int num_faces6 = ARRAY_SIZE(indices6) / VERTS_PER_FACE;
    const DWORD adjacency6[] = {-1, 1, -1, 2, -1, 0, -1, -1, 1};
    const DWORD exp_point_rep6[] = {0, 1, 2, 1, 4, 5, 1, 5, 2};
    /* mesh7 (expands collapsed triangle)
     *
     * 0--1 3
     * | / /|
     * |/ / |
     * 2 5--4
     */
    const struct vertex_pnc vertices7[] =
    {
        {{ 0.0f,  3.0f,  0.f}, up, RED},
        {{ 2.0f,  3.0f,  0.f}, up, GREEN},
        {{ 0.0f,  0.0f,  0.f}, up, BLUE},

        {{ 3.0f,  3.0f,  0.f}, up, GREEN},
        {{ 3.0f,  0.0f,  0.f}, up, RED},
        {{ 1.0f,  0.0f,  0.f}, up, BLUE},
    };
    const DWORD indices7[] = {0, 1, 2, 3, 3, 3}; /* Face 1 is collapsed*/
    const unsigned int num_vertices7 = ARRAY_SIZE(vertices7);
    const unsigned int num_faces7 = ARRAY_SIZE(indices7) / VERTS_PER_FACE;
    const DWORD adjacency7[] = {-1, -1, -1, -1, -1, -1};
    const DWORD exp_point_rep7[] = {0, 1, 2, 3, 4, 5};
    /* mesh8 (indices re-ordering and double replacement)
     *
     * 0--1 9  6
     * | / /|  |\
     * |/ / |  | \
     * 2 11-10 8--7
     *         3--4
     *         | /
     *         |/
     *         5
     */
    const struct vertex_pnc vertices8[] =
    {
        {{ 0.0f,  3.0f,  0.f}, up, RED},
        {{ 2.0f,  3.0f,  0.f}, up, GREEN},
        {{ 0.0f,  0.0f,  0.f}, up, BLUE},

        {{ 4.0,  -4.0,  0.f}, up, RED},
        {{ 6.0,  -4.0,  0.f}, up, BLUE},
        {{ 4.0,  -7.0,  0.f}, up, GREEN},

        {{ 4.0f,  3.0f,  0.f}, up, GREEN},
        {{ 6.0f,  0.0f,  0.f}, up, BLUE},
        {{ 4.0f,  0.0f,  0.f}, up, RED},

        {{ 3.0f,  3.0f,  0.f}, up, GREEN},
        {{ 3.0f,  0.0f,  0.f}, up, RED},
        {{ 1.0f,  0.0f,  0.f}, up, BLUE},
    };
    const DWORD indices8[] = {0, 1, 2, 9, 10, 11, 6, 7, 8, 3, 4, 5};
    const unsigned int num_vertices8 = ARRAY_SIZE(vertices8);
    const unsigned int num_faces8 = ARRAY_SIZE(indices8) / VERTS_PER_FACE;
    const DWORD adjacency8[] = {-1, 1, -1, 2, -1, 0, -1, 3, 1, 2, -1, -1};
    const DWORD exp_point_rep8[] = {0, 1, 2, 3, 4, 5, 1, 4, 3, 1, 3, 2};
    /* mesh9 (right, shared vertices)
     *
     * 0--1
     * | /|
     * |/ |
     * 2--3
     */
    const struct vertex_pnc vertices9[] =
    {
        {{ 0.0f,  3.0f,  0.f}, up, RED},
        {{ 2.0f,  3.0f,  0.f}, up, GREEN},
        {{ 0.0f,  0.0f,  0.f}, up, BLUE},

        {{ 2.0f,  0.0f,  0.f}, up, RED},
    };
    const DWORD indices9[] = {0, 1, 2, 1, 3, 2};
    const unsigned int num_vertices9 = ARRAY_SIZE(vertices9);
    const unsigned int num_faces9 = ARRAY_SIZE(indices9) / VERTS_PER_FACE;
    const DWORD adjacency9[] = {-1, 1, -1, -1, -1, 0};
    const DWORD exp_point_rep9[] = {0, 1, 2, 3};
    /* All mesh data */
    ID3DXMesh *mesh = NULL;
    ID3DXMesh *mesh_null_check = NULL;
    unsigned int attributes[] = {0};
    struct
    {
        const struct vertex_pnc *vertices;
        const DWORD *indices;
        const DWORD num_vertices;
        const DWORD num_faces;
        const DWORD *adjacency;
        const DWORD *exp_point_reps;
        const DWORD options;
    }
    tc[] =
    {
        {
            vertices0,
            indices0,
            num_vertices0,
            num_faces0,
            adjacency0,
            exp_point_rep0,
            options
        },
        {
            vertices1,
            indices1,
            num_vertices1,
            num_faces1,
            adjacency1,
            exp_point_rep1,
            options
        },
        {
            vertices2,
            indices2,
            num_vertices2,
            num_faces2,
            adjacency2,
            exp_point_rep2,
            options
        },
        {
            vertices3,
            indices3,
            num_vertices3,
            num_faces3,
            adjacency3,
            exp_point_rep3,
            options
        },
        {
            vertices4,
            indices4,
            num_vertices4,
            num_faces4,
            adjacency4,
            exp_point_rep4,
            options
        },
        {
            vertices5,
            indices5,
            num_vertices5,
            num_faces5,
            adjacency5,
            exp_point_rep5,
            options
        },
        {
            vertices6,
            indices6,
            num_vertices6,
            num_faces6,
            adjacency6,
            exp_point_rep6,
            options
        },
        {
            vertices7,
            indices7,
            num_vertices7,
            num_faces7,
            adjacency7,
            exp_point_rep7,
            options
        },
        {
            vertices8,
            indices8,
            num_vertices8,
            num_faces8,
            adjacency8,
            exp_point_rep8,
            options
        },
        {
            vertices9,
            indices9,
            num_vertices9,
            num_faces9,
            adjacency9,
            exp_point_rep9,
            options
        },
        {
            vertices5,
            (DWORD*)indices5_16bit,
            num_vertices5,
            num_faces5,
            adjacency5,
            exp_point_rep5,
            options_16bit
        },
    };
    DWORD *point_reps = NULL;

    test_context = new_test_context();
    if (!test_context)
    {
        skip("Couldn't create test context\n");
        goto cleanup;
    }

    for (i = 0; i < ARRAY_SIZE(tc); i++)
    {
        hr = D3DXCreateMesh(tc[i].num_faces, tc[i].num_vertices, tc[i].options, declaration,
                            test_context->device, &mesh);
        if (FAILED(hr))
        {
            skip("Couldn't create mesh %d. Got %x expected D3D_OK\n", i, hr);
            goto cleanup;
        }

        if (i == 0) /* Save first mesh for later NULL checks */
            mesh_null_check = mesh;

        point_reps = HeapAlloc(GetProcessHeap(), 0, tc[i].num_vertices * sizeof(*point_reps));
        if (!point_reps)
        {
            skip("Couldn't allocate point reps array.\n");
            goto cleanup;
        }

        hr = mesh->lpVtbl->LockVertexBuffer(mesh, 0, &vertex_buffer);
        if (FAILED(hr))
        {
            skip("Couldn't lock vertex buffer.\n");
            goto cleanup;
        }
        memcpy(vertex_buffer, tc[i].vertices, tc[i].num_vertices * sizeof(*tc[i].vertices));
        hr = mesh->lpVtbl->UnlockVertexBuffer(mesh);
        if (FAILED(hr))
        {
            skip("Couldn't unlock vertex buffer.\n");
            goto cleanup;
        }

        hr = mesh->lpVtbl->LockIndexBuffer(mesh, 0, &index_buffer);
        if (FAILED(hr))
        {
            skip("Couldn't lock index buffer.\n");
            goto cleanup;
        }
        if (tc[i].options & D3DXMESH_32BIT)
        {
            memcpy(index_buffer, tc[i].indices, VERTS_PER_FACE * tc[i].num_faces * sizeof(DWORD));
        }
        else
        {
            memcpy(index_buffer, tc[i].indices, VERTS_PER_FACE * tc[i].num_faces * sizeof(WORD));
        }
        hr = mesh->lpVtbl->UnlockIndexBuffer(mesh);
        if (FAILED(hr)) {
            skip("Couldn't unlock index buffer.\n");
            goto cleanup;
        }

        hr = mesh->lpVtbl->LockAttributeBuffer(mesh, 0, &attributes_buffer);
        if (FAILED(hr))
        {
            skip("Couldn't lock attributes buffer.\n");
            goto cleanup;
        }
        memcpy(attributes_buffer, attributes, sizeof(attributes));
        hr = mesh->lpVtbl->UnlockAttributeBuffer(mesh);
        if (FAILED(hr))
        {
            skip("Couldn't unlock attributes buffer.\n");
            goto cleanup;
        }

        /* Convert adjacency to point representation */
        memset(point_reps, -1, tc[i].num_vertices * sizeof(*point_reps));
        hr = mesh->lpVtbl->ConvertAdjacencyToPointReps(mesh, tc[i].adjacency, point_reps);
        ok(hr == D3D_OK, "ConvertAdjacencyToPointReps failed case %d. "
           "Got %x expected D3D_OK\n", i, hr);

        /* Check point representation */
        for (j = 0; j < tc[i].num_vertices; j++)
        {
            ok(point_reps[j] == tc[i].exp_point_reps[j],
               "Unexpected point representation at (%d, %d)."
               " Got %d expected %d\n",
               i, j, point_reps[j], tc[i].exp_point_reps[j]);
        }

        HeapFree(GetProcessHeap(), 0, point_reps);
        point_reps = NULL;

        if (i != 0) /* First mesh will be freed during cleanup */
            mesh->lpVtbl->Release(mesh);
    }

    /* NULL checks */
    hr = mesh_null_check->lpVtbl->ConvertAdjacencyToPointReps(mesh_null_check, tc[0].adjacency, NULL);
    ok(hr == D3DERR_INVALIDCALL, "ConvertAdjacencyToPointReps point_reps NULL. "
       "Got %x expected D3DERR_INVALIDCALL\n", hr);
    hr = mesh_null_check->lpVtbl->ConvertAdjacencyToPointReps(mesh_null_check, NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "ConvertAdjacencyToPointReps adjacency and point_reps NULL. "
       "Got %x expected D3DERR_INVALIDCALL\n", hr);

cleanup:
    if (mesh_null_check)
        mesh_null_check->lpVtbl->Release(mesh_null_check);
    HeapFree(GetProcessHeap(), 0, point_reps);
    free_test_context(test_context);
}

START_TEST(mesh)
{
    D3DXBoundProbeTest();
    D3DXComputeBoundingBoxTest();
    D3DXComputeBoundingSphereTest();
    D3DXGetFVFVertexSizeTest();
    D3DXIntersectTriTest();
    D3DXCreateMeshTest();
    D3DXCreateMeshFVFTest();
    D3DXLoadMeshTest();
    D3DXCreateBoxTest();
    D3DXCreateSphereTest();
    D3DXCreateCylinderTest();
    D3DXCreateTextTest();
    test_get_decl_length();
    test_get_decl_vertex_size();
    test_fvf_decl_conversion();
    D3DXGenerateAdjacencyTest();
    test_update_semantics();
    test_create_skin_info();
    test_convert_adjacency_to_point_reps();
}
