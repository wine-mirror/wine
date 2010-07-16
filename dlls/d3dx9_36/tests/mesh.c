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

static void D3DXDeclaratorFromFVFTest(void)
{
    D3DVERTEXELEMENT9 decl[MAX_FVF_DECL_SIZE];
    HRESULT hr;
    int i, size;

    static const D3DVERTEXELEMENT9 exp1[6] = {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 0xC, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
        {0, 0x18, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
        {0, 0x1C,  D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 1},
        {0, 0x20, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_TEXCOORD, 0},
        D3DDECL_END(), };

    static const D3DVERTEXELEMENT9 exp2[3] = {
        {0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITIONT, 0},
        {0, 0x10, D3DDECLTYPE_FLOAT1, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_PSIZE, 0},
        D3DDECL_END(), };

    static const D3DVERTEXELEMENT9 exp3[4] = {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 0xC, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDWEIGHT, 0},
        {0, 0x1C, D3DDECLTYPE_UBYTE4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDINDICES, 0},
        D3DDECL_END(), };

    /* Note: passing NULL as declaration segfaults */
    todo_wine
    {
        hr = D3DXDeclaratorFromFVF(0, decl);
        ok(hr == D3D_OK, "D3DXDeclaratorFromFVF returned %#x, expected %#x\n", hr, D3D_OK);
        ok(decl[0].Stream == 0xFF, "D3DXDeclaratorFromFVF returned an incorrect vertex declaration\n"); /* end element */

        hr = D3DXDeclaratorFromFVF(D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX1, decl);
        ok(hr == D3D_OK, "D3DXDeclaratorFromFVF returned %#x, expected %#x\n", hr, D3D_OK);

        if (hr == D3D_OK)
        {
            size = sizeof(exp1)/sizeof(exp1[0]);
            for (i=0; i<size-1; i++)
            {
                ok(decl[i].Stream == exp1[i].Stream, "Returned stream %d, expected %d\n", decl[i].Stream, exp1[i].Stream);
                ok(decl[i].Type == exp1[i].Type, "Returned type %d, expected %d\n", decl[i].Type, exp1[i].Type);
                ok(decl[i].Method == exp1[i].Method, "Returned method %d, expected %d\n", decl[i].Method, exp1[i].Method);
                ok(decl[i].Usage == exp1[i].Usage, "Returned usage %d, expected %d\n", decl[i].Usage, exp1[i].Usage);
                ok(decl[i].UsageIndex == exp1[i].UsageIndex, "Returned usage index %d, expected %d\n", decl[i].UsageIndex, exp1[i].UsageIndex);
	        ok(decl[i].Offset == exp1[i].Offset, "Returned offset %d, expected %d\n", decl[i].Offset, exp1[i].Offset);
            }
            ok(decl[size-1].Stream == 0xFF, "Returned too long vertex declaration\n"); /* end element */
        }
    }

    todo_wine
    {
        hr = D3DXDeclaratorFromFVF(D3DFVF_XYZRHW | D3DFVF_PSIZE, decl);
        ok(hr == D3D_OK, "D3DXDeclaratorFromFVF returned %#x, expected %#x\n", hr, D3D_OK);

        if (hr == D3D_OK)
        {
            size = sizeof(exp2)/sizeof(exp2[0]);
            for (i=0; i<size-1; i++)
            {
                ok(decl[i].Stream == exp2[i].Stream, "Returned stream %d, expected %d\n", decl[i].Stream, exp2[i].Stream);
                ok(decl[i].Type == exp2[i].Type, "Returned type %d, expected %d\n", decl[i].Type, exp1[i].Type);
                ok(decl[i].Method == exp2[i].Method, "Returned method %d, expected %d\n", decl[i].Method, exp2[i].Method);
                ok(decl[i].Usage == exp2[i].Usage, "Returned usage %d, expected %d\n", decl[i].Usage, exp2[i].Usage);
                ok(decl[i].UsageIndex == exp2[i].UsageIndex, "Returned usage index %d, expected %d\n", decl[i].UsageIndex, exp2[i].UsageIndex);
                ok(decl[i].Offset == exp2[i].Offset, "Returned offset %d, expected %d\n", decl[i].Offset, exp2[i].Offset);
            }
            ok(decl[size-1].Stream == 0xFF, "Returned too long vertex declaration\n"); /* end element */
        }
    }

    todo_wine
    {
        hr = D3DXDeclaratorFromFVF(D3DFVF_XYZB5, decl);
        ok(hr == D3DERR_INVALIDCALL, "D3DXDeclaratorFromFVF returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

        hr = D3DXDeclaratorFromFVF(D3DFVF_XYZB5 | D3DFVF_LASTBETA_UBYTE4, decl);
        ok(hr == D3D_OK, "D3DXDeclaratorFromFVF returned %#x, expected %#x\n", hr, D3D_OK);

        if (hr == D3D_OK)
        {
            size = sizeof(exp3)/sizeof(exp3[0]);
            for (i=0; i<size-1; i++)
            {
                ok(decl[i].Stream == exp3[i].Stream, "Returned stream %d, expected %d\n", decl[i].Stream, exp3[i].Stream);
                ok(decl[i].Type == exp3[i].Type, "Returned type %d, expected %d\n", decl[i].Type, exp3[i].Type);
                ok(decl[i].Method == exp3[i].Method, "Returned method %d, expected %d\n", decl[i].Method, exp3[i].Method);
                ok(decl[i].Usage == exp3[i].Usage, "Returned usage %d, expected %d\n", decl[i].Usage, exp3[i].Usage);
                ok(decl[i].UsageIndex == exp3[i].UsageIndex, "Returned usage index %d, expected %d\n", decl[i].UsageIndex, exp3[i].UsageIndex);
                ok(decl[i].Offset == exp3[i].Offset, "Returned offset %d, expected %d\n", decl[i].Offset, exp3[i].Offset);
            }
            ok(decl[size-1].Stream == 0xFF, "Returned too long vertex declaration\n"); /* end element */
        }
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

    hr = D3DXCreateSphere(device, 1.0f, 2, 2, &sphere, NULL);
    todo_wine ok( hr == D3D_OK, "Got result %x, expected 0 (D3D_OK)\n",hr);

    if (sphere)
        sphere->lpVtbl->Release(sphere);

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
    D3DXDeclaratorFromFVFTest();
    D3DXGetFVFVertexSizeTest();
    D3DXIntersectTriTest();
    D3DXCreateSphereTest();
    test_get_decl_vertex_size();
}
