/*
 * Copyright 2008 David Adam
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

#include "d3dx8.h"

#include "wine/test.h"

#define admitted_error 0.0001f

static BOOL compare(FLOAT u, FLOAT v)
{
    return (fabs(u-v) < admitted_error);
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

    rayposition.x = 5.0f; rayposition.y = 7.0f; rayposition.z = 9.0f;
    result = D3DXSphereBoundProbe(&center, radius, &rayposition, &raydirection);
    ok(result == FALSE, "expected FALSE, received TRUE\n");

    rayposition.x = 5.0f; rayposition.y = 11.0f; rayposition.z = 9.0f;
    result = D3DXSphereBoundProbe(&center, radius, &rayposition, &raydirection);
    ok(result == FALSE, "expected FALSE, received TRUE\n");
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

START_TEST(mesh)
{
    D3DXBoundProbeTest();
    D3DXIntersectTriTest();
}
