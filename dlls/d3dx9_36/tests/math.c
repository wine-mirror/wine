/*
 * Copyright 2008 Philip Nilsson
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
#include "d3dx9math.h"

#define ARRAY_SIZE 5

#define admitted_error 0.01f
#define relative_error(exp, out) ((exp == out) ? 0.0f : (fabs(out - exp) / fabs(exp)))
#define compare_vectors(exp, out) \
    for (i = 0; i < ARRAY_SIZE + 2; ++i) { \
        ok(relative_error(exp[i].x, out[i].x) < admitted_error && \
           relative_error(exp[i].y, out[i].y) < admitted_error && \
           relative_error(exp[i].z, out[i].z) < admitted_error && \
           relative_error(exp[i].w, out[i].w) < admitted_error, \
            "Got (%f, %f, %f, %f), expected (%f, %f, %f, %f) for index %d.\n", \
            out[i].x, out[i].y, out[i].z, out[i].w, \
            exp[i].x, exp[i].y, exp[i].z, exp[i].w, \
            i); \
    }

/* The mathematical properties are checked in the d3dx8 testsuite.
 *
 * These tests check:
 *   That the functions work.
 *   That the stride functionality works.
 *   That nothing is written where it should not be.
 *
 * These tests should check:
 *   That inp_vec is not modified.
 *   That the input and output arrays can be the same (MSDN
 *     says they can, and some testing with a native DLL
 *     says so too).
 */
static void test_D3DXVec_Array(void)
{
    unsigned int i;
    D3DVIEWPORT9 viewport;
    D3DXMATRIX mat, projection, view, world;
    D3DXVECTOR4 inp_vec[ARRAY_SIZE];
    D3DXVECTOR4 out_vec[ARRAY_SIZE + 2];
    D3DXVECTOR4 exp_vec[ARRAY_SIZE + 2];

    viewport.Width = 800; viewport.MinZ = 0.2f; viewport.X = 10;
    viewport.Height = 680; viewport.MaxZ = 0.9f; viewport.Y = 5;

    for (i = 0; i < ARRAY_SIZE + 2; ++i) {
        out_vec[i].x = out_vec[i].y = out_vec[i].z = out_vec[i].w = 0.0f;
        exp_vec[i].x = exp_vec[i].y = exp_vec[i].z = exp_vec[i].w = 0.0f;
    }

    for (i = 0; i < ARRAY_SIZE; ++i) {
        inp_vec[i].x = inp_vec[i].z = i;
        inp_vec[i].y = inp_vec[i].w = ARRAY_SIZE - i;
    }

    U(mat).m[0][0] = 1.0f; U(mat).m[0][1] = 2.0f; U(mat).m[0][2] = 3.0f; U(mat).m[0][3] = 4.0f;
    U(mat).m[1][0] = 5.0f; U(mat).m[1][1] = 6.0f; U(mat).m[1][2] = 7.0f; U(mat).m[1][3] = 8.0f;
    U(mat).m[2][0] = 9.0f; U(mat).m[2][1] = 10.0f; U(mat).m[2][2] = 11.0f; U(mat).m[2][3] = 12.0f;
    U(mat).m[3][0] = 13.0f; U(mat).m[3][1] = 14.0f; U(mat).m[3][2] = 15.0f; U(mat).m[3][3] = 16.0f;

    D3DXMatrixPerspectiveFovLH(&projection,D3DX_PI/4.0f,20.0f/17.0f,1.0f,1000.0f);

    U(view).m[0][1] = 5.0f; U(view).m[0][2] = 7.0f; U(view).m[0][3] = 8.0f;
    U(view).m[1][0] = 11.0f; U(view).m[1][2] = 16.0f; U(view).m[1][3] = 33.0f;
    U(view).m[2][0] = 19.0f; U(view).m[2][1] = -21.0f; U(view).m[2][3] = 43.0f;
    U(view).m[3][0] = 2.0f; U(view).m[3][1] = 3.0f; U(view).m[3][2] = -4.0f;
    U(view).m[0][0] = 10.0f; U(view).m[1][1] = 20.0f; U(view).m[2][2] = 30.0f;
    U(view).m[3][3] = -40.0f;

    U(world).m[0][0] = 21.0f; U(world).m[0][1] = 2.0f; U(world).m[0][2] = 3.0f; U(world).m[0][3] = 4.0;
    U(world).m[1][0] = 5.0f; U(world).m[1][1] = 23.0f; U(world).m[1][2] = 7.0f; U(world).m[1][3] = 8.0f;
    U(world).m[2][0] = -8.0f; U(world).m[2][1] = -7.0f; U(world).m[2][2] = 25.0f; U(world).m[2][3] = -5.0f;
    U(world).m[3][0] = -4.0f; U(world).m[3][1] = -3.0f; U(world).m[3][2] = -2.0f; U(world).m[3][3] = 27.0f;

    /* D3DXVec2TransformCoordArray */
    exp_vec[1].x = 0.678571f; exp_vec[1].y = 0.785714f;
    exp_vec[2].x = 0.653846f; exp_vec[2].y = 0.769231f;
    exp_vec[3].x = 0.625f;    exp_vec[3].y = 0.75f;
    exp_vec[4].x = 0.590909f; exp_vec[4].y = 8.0f/11.0f;
    exp_vec[5].x = 0.55f;     exp_vec[5].y = 0.7f;
    D3DXVec2TransformCoordArray((D3DXVECTOR2*)(out_vec + 1), sizeof(D3DXVECTOR4), (D3DXVECTOR2*)inp_vec, sizeof(D3DXVECTOR4), &mat, ARRAY_SIZE);
    compare_vectors(exp_vec, out_vec);

    /* D3DXVec2TransformNormalArray */
    exp_vec[1].x = 25.0f; exp_vec[1].y = 30.0f;
    exp_vec[2].x = 21.0f; exp_vec[2].y = 26.0f;
    exp_vec[3].x = 17.0f; exp_vec[3].y = 22.0f;
    exp_vec[4].x = 13.0f; exp_vec[4].y = 18.0f;
    exp_vec[5].x =  9.0f; exp_vec[5].y = 14.0f;
    D3DXVec2TransformNormalArray((D3DXVECTOR2*)(out_vec + 1), sizeof(D3DXVECTOR4), (D3DXVECTOR2*)inp_vec, sizeof(D3DXVECTOR4), &mat, ARRAY_SIZE);
    compare_vectors(exp_vec, out_vec);

    /* D3DXVec3TransformCoordArray */
    exp_vec[1].x = 0.678571f; exp_vec[1].y = 0.785714f;  exp_vec[1].z = 0.892857f;
    exp_vec[2].x = 0.671875f; exp_vec[2].y = 0.78125f;   exp_vec[2].z = 0.890625f;
    exp_vec[3].x = 6.0f/9.0f; exp_vec[3].y = 7.0f/9.0f;  exp_vec[3].z = 8.0f/9.0f;
    exp_vec[4].x = 0.6625f;   exp_vec[4].y = 0.775f;     exp_vec[4].z = 0.8875f;
    exp_vec[5].x = 0.659091f; exp_vec[5].y = 0.772727f;  exp_vec[5].z = 0.886364f;
    D3DXVec3TransformCoordArray((D3DXVECTOR3*)(out_vec + 1), sizeof(D3DXVECTOR4), (D3DXVECTOR3*)inp_vec, sizeof(D3DXVECTOR4), &mat, ARRAY_SIZE);
    compare_vectors(exp_vec, out_vec);

    /* D3DXVec3TransformNormalArray */
    exp_vec[1].x = 25.0f; exp_vec[1].y = 30.0f; exp_vec[1].z = 35.0f;
    exp_vec[2].x = 30.0f; exp_vec[2].y = 36.0f; exp_vec[2].z = 42.0f;
    exp_vec[3].x = 35.0f; exp_vec[3].y = 42.0f; exp_vec[3].z = 49.0f;
    exp_vec[4].x = 40.0f; exp_vec[4].y = 48.0f; exp_vec[4].z = 56.0f;
    exp_vec[5].x = 45.0f; exp_vec[5].y = 54.0f; exp_vec[5].z = 63.0f;
    D3DXVec3TransformNormalArray((D3DXVECTOR3*)(out_vec + 1), sizeof(D3DXVECTOR4), (D3DXVECTOR3*)inp_vec, sizeof(D3DXVECTOR4), &mat, ARRAY_SIZE);
    compare_vectors(exp_vec, out_vec);

    /* D3DXVec3ProjectArray */
    exp_vec[1].x = 1089.554199f; exp_vec[1].y = -226.590622f; exp_vec[1].z = 0.215273f;
    exp_vec[2].x = 1068.903320f; exp_vec[2].y =  103.085129f; exp_vec[2].z = 0.183050f;
    exp_vec[3].x = 1051.778931f; exp_vec[3].y =  376.462250f; exp_vec[3].z = 0.156329f;
    exp_vec[4].x = 1037.348877f; exp_vec[4].y =  606.827393f; exp_vec[4].z = 0.133813f;
    exp_vec[5].x = 1025.023560f; exp_vec[5].y =  803.591248f; exp_vec[5].z = 0.114581f;
    D3DXVec3ProjectArray((D3DXVECTOR3*)(out_vec + 1), sizeof(D3DXVECTOR4), (CONST D3DXVECTOR3*)inp_vec, sizeof(D3DXVECTOR4), &viewport, &projection, &view, &world, ARRAY_SIZE);
    compare_vectors(exp_vec, out_vec);

    /* D3DXVec3UnprojectArray */
    exp_vec[1].x = -6.124031f; exp_vec[1].y = 3.225360f; exp_vec[1].z = 0.620571f;
    exp_vec[2].x = -3.807109f; exp_vec[2].y = 2.046579f; exp_vec[2].z = 0.446894f;
    exp_vec[3].x = -2.922839f; exp_vec[3].y = 1.596689f; exp_vec[3].z = 0.380609f;
    exp_vec[4].x = -2.456225f; exp_vec[4].y = 1.359290f; exp_vec[4].z = 0.345632f;
    exp_vec[5].x = -2.167897f; exp_vec[5].y = 1.212597f; exp_vec[5].z = 0.324019f;
    D3DXVec3UnprojectArray((D3DXVECTOR3*)(out_vec + 1), sizeof(D3DXVECTOR4), (CONST D3DXVECTOR3*)inp_vec, sizeof(D3DXVECTOR4), &viewport, &projection, &view, &world, ARRAY_SIZE);
    compare_vectors(exp_vec, out_vec);

    /* D3DXVec2TransformArray */
    exp_vec[1].x = 38.0f; exp_vec[1].y = 44.0f; exp_vec[1].z = 50.0f; exp_vec[1].w = 56.0f;
    exp_vec[2].x = 34.0f; exp_vec[2].y = 40.0f; exp_vec[2].z = 46.0f; exp_vec[2].w = 52.0f;
    exp_vec[3].x = 30.0f; exp_vec[3].y = 36.0f; exp_vec[3].z = 42.0f; exp_vec[3].w = 48.0f;
    exp_vec[4].x = 26.0f; exp_vec[4].y = 32.0f; exp_vec[4].z = 38.0f; exp_vec[4].w = 44.0f;
    exp_vec[5].x = 22.0f; exp_vec[5].y = 28.0f; exp_vec[5].z = 34.0f; exp_vec[5].w = 40.0f;
    D3DXVec2TransformArray(out_vec + 1, sizeof(D3DXVECTOR4), (D3DXVECTOR2*)inp_vec, sizeof(D3DXVECTOR4), &mat, ARRAY_SIZE);
    compare_vectors(exp_vec, out_vec);

    /* D3DXVec3TransformArray */
    exp_vec[1].x = 38.0f; exp_vec[1].y = 44.0f; exp_vec[1].z = 50.0f; exp_vec[1].w = 56.0f;
    exp_vec[2].x = 43.0f; exp_vec[2].y = 50.0f; exp_vec[2].z = 57.0f; exp_vec[2].w = 64.0f;
    exp_vec[3].x = 48.0f; exp_vec[3].y = 56.0f; exp_vec[3].z = 64.0f; exp_vec[3].w = 72.0f;
    exp_vec[4].x = 53.0f; exp_vec[4].y = 62.0f; exp_vec[4].z = 71.0f; exp_vec[4].w = 80.0f;
    exp_vec[5].x = 58.0f; exp_vec[5].y = 68.0f; exp_vec[5].z = 78.0f; exp_vec[5].w = 88.0f;
    D3DXVec3TransformArray(out_vec + 1, sizeof(D3DXVECTOR4), (D3DXVECTOR3*)inp_vec, sizeof(D3DXVECTOR4), &mat, ARRAY_SIZE);
    compare_vectors(exp_vec, out_vec);

    /* D3DXVec4TransformArray */
    exp_vec[1].x = 90.0f; exp_vec[1].y = 100.0f; exp_vec[1].z = 110.0f; exp_vec[1].w = 120.0f;
    exp_vec[2].x = 82.0f; exp_vec[2].y = 92.0f;  exp_vec[2].z = 102.0f; exp_vec[2].w = 112.0f;
    exp_vec[3].x = 74.0f; exp_vec[3].y = 84.0f;  exp_vec[3].z = 94.0f;  exp_vec[3].w = 104.0f;
    exp_vec[4].x = 66.0f; exp_vec[4].y = 76.0f;  exp_vec[4].z = 86.0f;  exp_vec[4].w = 96.0f;
    exp_vec[5].x = 58.0f; exp_vec[5].y = 68.0f;  exp_vec[5].z = 78.0f;  exp_vec[5].w = 88.0f;
    D3DXVec4TransformArray(out_vec + 1, sizeof(D3DXVECTOR4), inp_vec, sizeof(D3DXVECTOR4), &mat, ARRAY_SIZE);
    compare_vectors(exp_vec, out_vec);
}

START_TEST(math)
{
    test_D3DXVec_Array();
}
