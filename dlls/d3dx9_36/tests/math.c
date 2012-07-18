/*
 * Copyright 2008 David Adam
 * Copyright 2008 Luis Busquets
 * Copyright 2008 Philip Nilsson
 * Copyright 2008 Henri Verbeet
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
#include <math.h>

#define ARRAY_SIZE 5

#define admitted_error 0.0001f

#define relative_error(exp, out) ((exp == 0.0f) ? fabs(exp - out) : (fabs(1.0f - (out) / (exp))))

#define expect_color(expectedcolor,gotcolor) ok((relative_error(expectedcolor.r, gotcolor.r)<admitted_error)&&(relative_error(expectedcolor.g, gotcolor.g)<admitted_error)&&(relative_error(expectedcolor.b, gotcolor.b)<admitted_error)&&(relative_error(expectedcolor.a, gotcolor.a)<admitted_error),"Expected Color= (%f, %f, %f, %f)\n , Got Color= (%f, %f, %f, %f)\n", expectedcolor.r, expectedcolor.g, expectedcolor.b, expectedcolor.a, gotcolor.r, gotcolor.g, gotcolor.b, gotcolor.a);

static inline BOOL compare_matrix(const D3DXMATRIX *m1, const D3DXMATRIX *m2)
{
    int i, j;

    for (i = 0; i < 4; ++i)
    {
        for (j = 0; j < 4; ++j)
        {
            if (relative_error(U(*m1).m[i][j], U(*m2).m[i][j]) > admitted_error)
                return FALSE;
        }
    }

    return TRUE;
}

#define expect_mat(expectedmat, gotmat) \
do { \
    const D3DXMATRIX *__m1 = (expectedmat); \
    const D3DXMATRIX *__m2 = (gotmat); \
    ok(compare_matrix(__m1, __m2), "Expected matrix=\n(%f,%f,%f,%f\n %f,%f,%f,%f\n %f,%f,%f,%f\n %f,%f,%f,%f\n)\n\n" \
            "Got matrix=\n(%f,%f,%f,%f\n %f,%f,%f,%f\n %f,%f,%f,%f\n %f,%f,%f,%f)\n", \
            U(*__m1).m[0][0], U(*__m1).m[0][1], U(*__m1).m[0][2], U(*__m1).m[0][3], \
            U(*__m1).m[1][0], U(*__m1).m[1][1], U(*__m1).m[1][2], U(*__m1).m[1][3], \
            U(*__m1).m[2][0], U(*__m1).m[2][1], U(*__m1).m[2][2], U(*__m1).m[2][3], \
            U(*__m1).m[3][0], U(*__m1).m[3][1], U(*__m1).m[3][2], U(*__m1).m[3][3], \
            U(*__m2).m[0][0], U(*__m2).m[0][1], U(*__m2).m[0][2], U(*__m2).m[0][3], \
            U(*__m2).m[1][0], U(*__m2).m[1][1], U(*__m2).m[1][2], U(*__m2).m[1][3], \
            U(*__m2).m[2][0], U(*__m2).m[2][1], U(*__m2).m[2][2], U(*__m2).m[2][3], \
            U(*__m2).m[3][0], U(*__m2).m[3][1], U(*__m2).m[3][2], U(*__m2).m[3][3]); \
} while(0)

#define compare_rotation(exp, got) \
    ok(relative_error(exp.w, got.w) < admitted_error && \
       relative_error(exp.x, got.x) < admitted_error && \
       relative_error(exp.y, got.y) < admitted_error && \
       relative_error(exp.z, got.z) < admitted_error, \
       "Expected rotation = (%f, %f, %f, %f), \
        got rotation = (%f, %f, %f, %f)\n", \
        exp.w, exp.x, exp.y, exp.z, got.w, got.x, got.y, got.z)

#define compare_scale(exp, got) \
    ok(relative_error(exp.x, got.x) < admitted_error && \
       relative_error(exp.y, got.y) < admitted_error && \
       relative_error(exp.z, got.z) < admitted_error, \
       "Expected scale = (%f, %f, %f), \
        got scale = (%f, %f, %f)\n", \
        exp.x, exp.y, exp.z, got.x, got.y, got.z)

#define compare_translation(exp, got) \
    ok(relative_error(exp.x, got.x) < admitted_error && \
       relative_error(exp.y, got.y) < admitted_error && \
       relative_error(exp.z, got.z) < admitted_error, \
       "Expected translation = (%f, %f, %f), \
        got translation = (%f, %f, %f)\n", \
        exp.x, exp.y, exp.z, got.x, got.y, got.z)

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

#define compare_planes(exp, out) \
    for (i = 0; i < ARRAY_SIZE + 2; ++i) { \
        ok(relative_error(exp[i].a, out[i].a) < admitted_error && \
           relative_error(exp[i].b, out[i].b) < admitted_error && \
           relative_error(exp[i].c, out[i].c) < admitted_error && \
           relative_error(exp[i].d, out[i].d) < admitted_error, \
            "Got (%f, %f, %f, %f), expected (%f, %f, %f, %f) for index %d.\n", \
            out[i].a, out[i].b, out[i].c, out[i].d, \
            exp[i].a, exp[i].b, exp[i].c, exp[i].d, \
            i); \
    }

#define expect_plane(expectedplane,gotplane) ok((relative_error(expectedplane.a, gotplane.a)<admitted_error)&&(relative_error(expectedplane.b, gotplane.b)<admitted_error)&&(relative_error(expectedplane.c, gotplane.c)<admitted_error)&&(relative_error(expectedplane.d, gotplane.d)<admitted_error),"Expected Plane= (%f, %f, %f, %f)\n , Got Plane= (%f, %f, %f, %f)\n", expectedplane.a, expectedplane.b, expectedplane.c, expectedplane.d, gotplane.a, gotplane.b, gotplane.c, gotplane.d);

#define expect_vec(expectedvec,gotvec) ok((relative_error(expectedvec.x, gotvec.x)<admitted_error)&&(relative_error(expectedvec.y, gotvec.y)<admitted_error),"Expected Vector= (%f, %f)\n , Got Vector= (%f, %f)\n", expectedvec.x, expectedvec.y, gotvec.x, gotvec.y);

#define expect_vec3(expectedvec,gotvec) ok((relative_error(expectedvec.x, gotvec.x)<admitted_error)&&(relative_error(expectedvec.y, gotvec.y)<admitted_error)&&(relative_error(expectedvec.z, gotvec.z)<admitted_error),"Expected Vector= (%f, %f, %f)\n , Got Vector= (%f, %f, %f)\n", expectedvec.x, expectedvec.y, expectedvec.z, gotvec.x, gotvec.y, gotvec.z);

#define expect_vec4(expectedvec,gotvec) ok((relative_error(expectedvec.x, gotvec.x)<admitted_error)&&(relative_error(expectedvec.y, gotvec.y)<admitted_error)&&(relative_error(expectedvec.z, gotvec.z)<admitted_error)&&(relative_error(expectedvec.w, gotvec.w)<admitted_error),"Expected Vector= (%f, %f, %f, %f)\n , Got Vector= (%f, %f, %f, %f)\n", expectedvec.x, expectedvec.y, expectedvec.z, expectedvec.w, gotvec.x, gotvec.y, gotvec.z, gotvec.w);


static void D3DXColorTest(void)
{
    D3DXCOLOR color, color1, color2, expected, got;
    LPD3DXCOLOR funcpointer;
    FLOAT scale;

    color.r = 0.2f; color.g = 0.75f; color.b = 0.41f; color.a = 0.93f;
    color1.r = 0.6f; color1.g = 0.55f; color1.b = 0.23f; color1.a = 0.82f;
    color2.r = 0.3f; color2.g = 0.5f; color2.b = 0.76f; color2.a = 0.11f;

    scale = 0.3f;

/*_______________D3DXColorAdd________________*/
    expected.r = 0.9f; expected.g = 1.05f; expected.b = 0.99f, expected.a = 0.93f;
    D3DXColorAdd(&got,&color1,&color2);
    expect_color(expected,got);
    /* Test the NULL case */
    funcpointer = D3DXColorAdd(&got,NULL,&color2);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXColorAdd(NULL,NULL,&color2);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXColorAdd(NULL,NULL,NULL);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);

/*_______________D3DXColorAdjustContrast______*/
    expected.r = 0.41f; expected.g = 0.575f; expected.b = 0.473f, expected.a = 0.93f;
    D3DXColorAdjustContrast(&got,&color,scale);
    expect_color(expected,got);

/*_______________D3DXColorAdjustSaturation______*/
    expected.r = 0.486028f; expected.g = 0.651028f; expected.b = 0.549028f, expected.a = 0.93f;
    D3DXColorAdjustSaturation(&got,&color,scale);
    expect_color(expected,got);

/*_______________D3DXColorLerp________________*/
    expected.r = 0.32f; expected.g = 0.69f; expected.b = 0.356f; expected.a = 0.897f;
    D3DXColorLerp(&got,&color,&color1,scale);
    expect_color(expected,got);
    /* Test the NULL case */
    funcpointer = D3DXColorLerp(&got,NULL,&color1,scale);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXColorLerp(NULL,NULL,&color1,scale);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXColorLerp(NULL,NULL,NULL,scale);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);

/*_______________D3DXColorModulate________________*/
    expected.r = 0.18f; expected.g = 0.275f; expected.b = 0.1748f; expected.a = 0.0902f;
    D3DXColorModulate(&got,&color1,&color2);
    expect_color(expected,got);
    /* Test the NULL case */
    funcpointer = D3DXColorModulate(&got,NULL,&color2);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXColorModulate(NULL,NULL,&color2);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXColorModulate(NULL,NULL,NULL);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);

/*_______________D3DXColorNegative________________*/
    expected.r = 0.8f; expected.g = 0.25f; expected.b = 0.59f; expected.a = 0.93f;
    D3DXColorNegative(&got,&color);
    expect_color(got,expected);
    /* Test the greater than 1 case */
    color1.r = 0.2f; color1.g = 1.75f; color1.b = 0.41f; color1.a = 0.93f;
    expected.r = 0.8f; expected.g = -0.75f; expected.b = 0.59f; expected.a = 0.93f;
    D3DXColorNegative(&got,&color1);
    expect_color(got,expected);
    /* Test the negative case */
    color1.r = 0.2f; color1.g = -0.75f; color1.b = 0.41f; color1.a = 0.93f;
    expected.r = 0.8f; expected.g = 1.75f; expected.b = 0.59f; expected.a = 0.93f;
    D3DXColorNegative(&got,&color1);
    expect_color(got,expected);
    /* Test the NULL case */
    funcpointer = D3DXColorNegative(&got,NULL);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXColorNegative(NULL,NULL);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);

/*_______________D3DXColorScale________________*/
    expected.r = 0.06f; expected.g = 0.225f; expected.b = 0.123f; expected.a = 0.279f;
    D3DXColorScale(&got,&color,scale);
    expect_color(expected,got);
    /* Test the NULL case */
    funcpointer = D3DXColorScale(&got,NULL,scale);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXColorScale(NULL,NULL,scale);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);

/*_______________D3DXColorSubtract_______________*/
    expected.r = -0.1f; expected.g = 0.25f; expected.b = -0.35f, expected.a = 0.82f;
    D3DXColorSubtract(&got,&color,&color2);
    expect_color(expected,got);
    /* Test the NULL case */
    funcpointer = D3DXColorSubtract(&got,NULL,&color2);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXColorSubtract(NULL,NULL,&color2);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXColorSubtract(NULL,NULL,NULL);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
}

static void D3DXFresnelTest(void)
{
    FLOAT expected, got;

    expected = 0.089187;
    got = D3DXFresnelTerm(0.5f,1.5);
    ok(relative_error(got, expected) < admitted_error, "Expected: %f, Got: %f\n", expected, got);
}

static void D3DXMatrixTest(void)
{
    D3DXMATRIX expectedmat, gotmat, mat, mat2, mat3;
    LPD3DXMATRIX funcpointer;
    D3DXPLANE plane;
    D3DXQUATERNION q, r;
    D3DXVECTOR3 at, axis, eye, last;
    D3DXVECTOR4 light;
    BOOL expected, got;
    FLOAT angle, determinant, expectedfloat, gotfloat;

    U(mat).m[0][1] = 5.0f; U(mat).m[0][2] = 7.0f; U(mat).m[0][3] = 8.0f;
    U(mat).m[1][0] = 11.0f; U(mat).m[1][2] = 16.0f; U(mat).m[1][3] = 33.0f;
    U(mat).m[2][0] = 19.0f; U(mat).m[2][1] = -21.0f; U(mat).m[2][3] = 43.0f;
    U(mat).m[3][0] = 2.0f; U(mat).m[3][1] = 3.0f; U(mat).m[3][2] = -4.0f;
    U(mat).m[0][0] = 10.0f; U(mat).m[1][1] = 20.0f; U(mat).m[2][2] = 30.0f;
    U(mat).m[3][3] = -40.0f;

    U(mat2).m[0][0] = 1.0f; U(mat2).m[1][0] = 2.0f; U(mat2).m[2][0] = 3.0f;
    U(mat2).m[3][0] = 4.0f; U(mat2).m[0][1] = 5.0f; U(mat2).m[1][1] = 6.0f;
    U(mat2).m[2][1] = 7.0f; U(mat2).m[3][1] = 8.0f; U(mat2).m[0][2] = -8.0f;
    U(mat2).m[1][2] = -7.0f; U(mat2).m[2][2] = -6.0f; U(mat2).m[3][2] = -5.0f;
    U(mat2).m[0][3] = -4.0f; U(mat2).m[1][3] = -3.0f; U(mat2).m[2][3] = -2.0f;
    U(mat2).m[3][3] = -1.0f;

    plane.a = -3.0f; plane.b = -1.0f; plane.c = 4.0f; plane.d = 7.0f;

    q.x = 1.0f; q.y = -4.0f; q.z =7.0f; q.w = -11.0f;
    r.x = 0.87f; r.y = 0.65f; r.z =0.43f; r.w= 0.21f;

    at.x = -2.0f; at.y = 13.0f; at.z = -9.0f;
    axis.x = 1.0f; axis.y = -3.0f; axis.z = 7.0f;
    eye.x = 8.0f; eye.y = -5.0f; eye.z = 5.75f;
    last.x = 9.7f; last.y = -8.6; last.z = 1.3f;

    light.x = 9.6f; light.y = 8.5f; light.z = 7.4; light.w = 6.3;

    angle = D3DX_PI/3.0f;

/*____________D3DXMatrixAffineTransformation______*/
    U(expectedmat).m[0][0] = -459.239990f; U(expectedmat).m[0][1] = -576.719971f; U(expectedmat).m[0][2] = -263.440002f; U(expectedmat).m[0][3] = 0.0f;
    U(expectedmat).m[1][0] = 519.760010f; U(expectedmat).m[1][1] = -352.440002f; U(expectedmat).m[1][2] = -277.679993f; U(expectedmat).m[1][3] = 0.0f;
    U(expectedmat).m[2][0] = 363.119995f; U(expectedmat).m[2][1] = -121.040001f; U(expectedmat).m[2][2] = -117.479996f; U(expectedmat).m[2][3] = 0.0f;
    U(expectedmat).m[3][0] = -1239.0f; U(expectedmat).m[3][1] = 667.0f; U(expectedmat).m[3][2] = 567.0f; U(expectedmat).m[3][3] = 1.0f;
    D3DXMatrixAffineTransformation(&gotmat,3.56f,&at,&q,&axis);
    expect_mat(&expectedmat, &gotmat);
/* Test the NULL case */
    U(expectedmat).m[0][0] = -459.239990f; U(expectedmat).m[0][1] = -576.719971f; U(expectedmat).m[0][2] = -263.440002f; U(expectedmat).m[0][3] = 0.0f;
    U(expectedmat).m[1][0] = 519.760010f; U(expectedmat).m[1][1] = -352.440002f; U(expectedmat).m[1][2] = -277.679993f; U(expectedmat).m[1][3] = 0.0f;
    U(expectedmat).m[2][0] = 363.119995f; U(expectedmat).m[2][1] = -121.040001f; U(expectedmat).m[2][2] = -117.479996f; U(expectedmat).m[2][3] = 0.0f;
    U(expectedmat).m[3][0] = 1.0f; U(expectedmat).m[3][1] = -3.0f; U(expectedmat).m[3][2] = 7.0f; U(expectedmat).m[3][3] = 1.0f;
    D3DXMatrixAffineTransformation(&gotmat,3.56f,NULL,&q,&axis);
    expect_mat(&expectedmat, &gotmat);

/*____________D3DXMatrixfDeterminant_____________*/
    expectedfloat = -147888.0f;
    gotfloat = D3DXMatrixDeterminant(&mat);
    ok(relative_error(gotfloat, expectedfloat ) < admitted_error, "Expected: %f, Got: %f\n", expectedfloat, gotfloat);

/*____________D3DXMatrixInverse______________*/
    U(expectedmat).m[0][0] = 16067.0f/73944.0f; U(expectedmat).m[0][1] = -10165.0f/147888.0f; U(expectedmat).m[0][2] = -2729.0f/147888.0f; U(expectedmat).m[0][3] = -1631.0f/49296.0f;
    U(expectedmat).m[1][0] = -565.0f/36972.0f; U(expectedmat).m[1][1] = 2723.0f/73944.0f; U(expectedmat).m[1][2] = -1073.0f/73944.0f; U(expectedmat).m[1][3] = 289.0f/24648.0f;
    U(expectedmat).m[2][0] = -389.0f/2054.0f; U(expectedmat).m[2][1] = 337.0f/4108.0f; U(expectedmat).m[2][2] = 181.0f/4108.0f; U(expectedmat).m[2][3] = 317.0f/4108.0f;
    U(expectedmat).m[3][0] = 163.0f/5688.0f; U(expectedmat).m[3][1] = -101.0f/11376.0f; U(expectedmat).m[3][2] = -73.0f/11376.0f; U(expectedmat).m[3][3] = -127.0f/3792.0f;
    expectedfloat = -147888.0f;
    D3DXMatrixInverse(&gotmat,&determinant,&mat);
    expect_mat(&expectedmat, &gotmat);
    ok(relative_error( determinant, expectedfloat ) < admitted_error, "Expected: %f, Got: %f\n", expectedfloat, determinant);
    funcpointer = D3DXMatrixInverse(&gotmat,NULL,&mat2);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);

/*____________D3DXMatrixIsIdentity______________*/
    expected = FALSE;
    memset(&mat3, 0, sizeof(mat3));
    got = D3DXMatrixIsIdentity(&mat3);
    ok(expected == got, "Expected : %d, Got : %d\n", expected, got);
    D3DXMatrixIdentity(&mat3);
    expected = TRUE;
    got = D3DXMatrixIsIdentity(&mat3);
    ok(expected == got, "Expected : %d, Got : %d\n", expected, got);
    U(mat3).m[0][0] = 0.000009f;
    expected = FALSE;
    got = D3DXMatrixIsIdentity(&mat3);
    ok(expected == got, "Expected : %d, Got : %d\n", expected, got);
    /* Test the NULL case */
    expected = FALSE;
    got = D3DXMatrixIsIdentity(NULL);
    ok(expected == got, "Expected : %d, Got : %d\n", expected, got);

/*____________D3DXMatrixLookatLH_______________*/
    U(expectedmat).m[0][0] = -0.822465f; U(expectedmat).m[0][1] = -0.409489f; U(expectedmat).m[0][2] = -0.394803f; U(expectedmat).m[0][3] = 0.0f;
    U(expectedmat).m[1][0] = -0.555856f; U(expectedmat).m[1][1] = 0.431286f; U(expectedmat).m[1][2] = 0.710645f; U(expectedmat).m[1][3] = 0.0f;
    U(expectedmat).m[2][0] = -0.120729f; U(expectedmat).m[2][1] = 0.803935f; U(expectedmat).m[2][2] = -0.582335f; U(expectedmat).m[2][3] = 0.0f;
    U(expectedmat).m[3][0] = 4.494634f; U(expectedmat).m[3][1] = 0.809719f; U(expectedmat).m[3][2] = 10.060076f; U(expectedmat).m[3][3] = 1.0f;
    D3DXMatrixLookAtLH(&gotmat,&eye,&at,&axis);
    expect_mat(&expectedmat, &gotmat);

/*____________D3DXMatrixLookatRH_______________*/
    U(expectedmat).m[0][0] = 0.822465f; U(expectedmat).m[0][1] = -0.409489f; U(expectedmat).m[0][2] = 0.394803f; U(expectedmat).m[0][3] = 0.0f;
    U(expectedmat).m[1][0] = 0.555856f; U(expectedmat).m[1][1] = 0.431286f; U(expectedmat).m[1][2] = -0.710645f; U(expectedmat).m[1][3] = 0.0f;
    U(expectedmat).m[2][0] = 0.120729f; U(expectedmat).m[2][1] = 0.803935f; U(expectedmat).m[2][2] = 0.582335f; U(expectedmat).m[2][3] = 0.0f;
    U(expectedmat).m[3][0] = -4.494634f; U(expectedmat).m[3][1] = 0.809719f; U(expectedmat).m[3][2] = -10.060076f; U(expectedmat).m[3][3] = 1.0f;
    D3DXMatrixLookAtRH(&gotmat,&eye,&at,&axis);
    expect_mat(&expectedmat, &gotmat);

/*____________D3DXMatrixMultiply______________*/
    U(expectedmat).m[0][0] = 73.0f; U(expectedmat).m[0][1] = 193.0f; U(expectedmat).m[0][2] = -197.0f; U(expectedmat).m[0][3] = -77.0f;
    U(expectedmat).m[1][0] = 231.0f; U(expectedmat).m[1][1] = 551.0f; U(expectedmat).m[1][2] = -489.0f; U(expectedmat).m[1][3] = -169.0;
    U(expectedmat).m[2][0] = 239.0f; U(expectedmat).m[2][1] = 523.0f; U(expectedmat).m[2][2] = -400.0f; U(expectedmat).m[2][3] = -116.0f;
    U(expectedmat).m[3][0] = -164.0f; U(expectedmat).m[3][1] = -320.0f; U(expectedmat).m[3][2] = 187.0f; U(expectedmat).m[3][3] = 31.0f;
    D3DXMatrixMultiply(&gotmat,&mat,&mat2);
    expect_mat(&expectedmat, &gotmat);

/*____________D3DXMatrixMultiplyTranspose____*/
    U(expectedmat).m[0][0] = 73.0f; U(expectedmat).m[0][1] = 231.0f; U(expectedmat).m[0][2] = 239.0f; U(expectedmat).m[0][3] = -164.0f;
    U(expectedmat).m[1][0] = 193.0f; U(expectedmat).m[1][1] = 551.0f; U(expectedmat).m[1][2] = 523.0f; U(expectedmat).m[1][3] = -320.0;
    U(expectedmat).m[2][0] = -197.0f; U(expectedmat).m[2][1] = -489.0f; U(expectedmat).m[2][2] = -400.0f; U(expectedmat).m[2][3] = 187.0f;
    U(expectedmat).m[3][0] = -77.0f; U(expectedmat).m[3][1] = -169.0f; U(expectedmat).m[3][2] = -116.0f; U(expectedmat).m[3][3] = 31.0f;
    D3DXMatrixMultiplyTranspose(&gotmat,&mat,&mat2);
    expect_mat(&expectedmat, &gotmat);

/*____________D3DXMatrixOrthoLH_______________*/
    U(expectedmat).m[0][0] = 0.8f; U(expectedmat).m[0][1] = 0.0f; U(expectedmat).m[0][2] = 0.0f; U(expectedmat).m[0][3] = 0.0f;
    U(expectedmat).m[1][0] = 0.0f; U(expectedmat).m[1][1] = 0.270270f; U(expectedmat).m[1][2] = 0.0f; U(expectedmat).m[1][3] = 0.0f;
    U(expectedmat).m[2][0] = 0.0f; U(expectedmat).m[2][1] = 0.0f; U(expectedmat).m[2][2] = -0.151515f; U(expectedmat).m[2][3] = 0.0f;
    U(expectedmat).m[3][0] = 0.0f; U(expectedmat).m[3][1] = 0.0f; U(expectedmat).m[3][2] = -0.484848f; U(expectedmat).m[3][3] = 1.0f;
    D3DXMatrixOrthoLH(&gotmat, 2.5f, 7.4f, -3.2f, -9.8f);
    expect_mat(&expectedmat, &gotmat);

/*____________D3DXMatrixOrthoOffCenterLH_______________*/
    U(expectedmat).m[0][0] = 3.636364f; U(expectedmat).m[0][1] = 0.0f; U(expectedmat).m[0][2] = 0.0f; U(expectedmat).m[0][3] = 0.0f;
    U(expectedmat).m[1][0] = 0.0f; U(expectedmat).m[1][1] = 0.180180f; U(expectedmat).m[1][2] = 0.0; U(expectedmat).m[1][3] = 0.0f;
    U(expectedmat).m[2][0] = 0.0f; U(expectedmat).m[2][1] = 0.0f; U(expectedmat).m[2][2] = -0.045662f; U(expectedmat).m[2][3] = 0.0f;
    U(expectedmat).m[3][0] = -1.727272f; U(expectedmat).m[3][1] = -0.567568f; U(expectedmat).m[3][2] = 0.424658f; U(expectedmat).m[3][3] = 1.0f;
    D3DXMatrixOrthoOffCenterLH(&gotmat, 0.2f, 0.75f, -2.4f, 8.7f, 9.3, -12.6);
    expect_mat(&expectedmat, &gotmat);

/*____________D3DXMatrixOrthoOffCenterRH_______________*/
    U(expectedmat).m[0][0] = 3.636364f; U(expectedmat).m[0][1] = 0.0f; U(expectedmat).m[0][2] = 0.0f; U(expectedmat).m[0][3] = 0.0f;
    U(expectedmat).m[1][0] = 0.0f; U(expectedmat).m[1][1] = 0.180180f; U(expectedmat).m[1][2] = 0.0; U(expectedmat).m[1][3] = 0.0f;
    U(expectedmat).m[2][0] = 0.0f; U(expectedmat).m[2][1] = 0.0f; U(expectedmat).m[2][2] = 0.045662f; U(expectedmat).m[2][3] = 0.0f;
    U(expectedmat).m[3][0] = -1.727272f; U(expectedmat).m[3][1] = -0.567568f; U(expectedmat).m[3][2] = 0.424658f; U(expectedmat).m[3][3] = 1.0f;
    D3DXMatrixOrthoOffCenterRH(&gotmat, 0.2f, 0.75f, -2.4f, 8.7f, 9.3, -12.6);
    expect_mat(&expectedmat, &gotmat);

/*____________D3DXMatrixOrthoRH_______________*/
    U(expectedmat).m[0][0] = 0.8f; U(expectedmat).m[0][1] = 0.0f; U(expectedmat).m[0][2] = 0.0f; U(expectedmat).m[0][3] = 0.0f;
    U(expectedmat).m[1][0] = 0.0f; U(expectedmat).m[1][1] = 0.270270f; U(expectedmat).m[1][2] = 0.0f; U(expectedmat).m[1][3] = 0.0f;
    U(expectedmat).m[2][0] = 0.0f; U(expectedmat).m[2][1] = 0.0f; U(expectedmat).m[2][2] = 0.151515f; U(expectedmat).m[2][3] = 0.0f;
    U(expectedmat).m[3][0] = 0.0f; U(expectedmat).m[3][1] = 0.0f; U(expectedmat).m[3][2] = -0.484848f; U(expectedmat).m[3][3] = 1.0f;
    D3DXMatrixOrthoRH(&gotmat, 2.5f, 7.4f, -3.2f, -9.8f);
    expect_mat(&expectedmat, &gotmat);

/*____________D3DXMatrixPerspectiveFovLH_______________*/
    U(expectedmat).m[0][0] = 13.288858f; U(expectedmat).m[0][1] = 0.0f; U(expectedmat).m[0][2] = 0.0f; U(expectedmat).m[0][3] = 0.0f;
    U(expectedmat).m[1][0] = 0.0f; U(expectedmat).m[1][1] = 9.966644f; U(expectedmat).m[1][2] = 0.0; U(expectedmat).m[1][3] = 0.0f;
    U(expectedmat).m[2][0] = 0.0f; U(expectedmat).m[2][1] = 0.0f; U(expectedmat).m[2][2] = 0.783784f; U(expectedmat).m[2][3] = 1.0f;
    U(expectedmat).m[3][0] = 0.0f; U(expectedmat).m[3][1] = 0.0f; U(expectedmat).m[3][2] = 1.881081f; U(expectedmat).m[3][3] = 0.0f;
    D3DXMatrixPerspectiveFovLH(&gotmat, 0.2f, 0.75f, -2.4f, 8.7f);
    expect_mat(&expectedmat, &gotmat);

/*____________D3DXMatrixPerspectiveFovRH_______________*/
    U(expectedmat).m[0][0] = 13.288858f; U(expectedmat).m[0][1] = 0.0f; U(expectedmat).m[0][2] = 0.0f; U(expectedmat).m[0][3] = 0.0f;
    U(expectedmat).m[1][0] = 0.0f; U(expectedmat).m[1][1] = 9.966644f; U(expectedmat).m[1][2] = 0.0; U(expectedmat).m[1][3] = 0.0f;
    U(expectedmat).m[2][0] = 0.0f; U(expectedmat).m[2][1] = 0.0f; U(expectedmat).m[2][2] = -0.783784f; U(expectedmat).m[2][3] = -1.0f;
    U(expectedmat).m[3][0] = 0.0f; U(expectedmat).m[3][1] = 0.0f; U(expectedmat).m[3][2] = 1.881081f; U(expectedmat).m[3][3] = 0.0f;
    D3DXMatrixPerspectiveFovRH(&gotmat, 0.2f, 0.75f, -2.4f, 8.7f);
    expect_mat(&expectedmat, &gotmat);

/*____________D3DXMatrixPerspectiveLH_______________*/
    U(expectedmat).m[0][0] = -24.0f; U(expectedmat).m[0][1] = 0.0f; U(expectedmat).m[0][2] = 0.0f; U(expectedmat).m[0][3] = 0.0f;
    U(expectedmat).m[1][0] = 0.0f; U(expectedmat).m[1][1] = -6.4f; U(expectedmat).m[1][2] = 0.0; U(expectedmat).m[1][3] = 0.0f;
    U(expectedmat).m[2][0] = 0.0f; U(expectedmat).m[2][1] = 0.0f; U(expectedmat).m[2][2] = 0.783784f; U(expectedmat).m[2][3] = 1.0f;
    U(expectedmat).m[3][0] = 0.0f; U(expectedmat).m[3][1] = 0.0f; U(expectedmat).m[3][2] = 1.881081f; U(expectedmat).m[3][3] = 0.0f;
    D3DXMatrixPerspectiveLH(&gotmat, 0.2f, 0.75f, -2.4f, 8.7f);
    expect_mat(&expectedmat, &gotmat);

/*____________D3DXMatrixPerspectiveOffCenterLH_______________*/
    U(expectedmat).m[0][0] = 11.636364f; U(expectedmat).m[0][1] = 0.0f; U(expectedmat).m[0][2] = 0.0f; U(expectedmat).m[0][3] = 0.0f;
    U(expectedmat).m[1][0] = 0.0f; U(expectedmat).m[1][1] = 0.576577f; U(expectedmat).m[1][2] = 0.0; U(expectedmat).m[1][3] = 0.0f;
    U(expectedmat).m[2][0] = -1.727273f; U(expectedmat).m[2][1] = -0.567568f; U(expectedmat).m[2][2] = 0.840796f; U(expectedmat).m[2][3] = 1.0f;
    U(expectedmat).m[3][0] = 0.0f; U(expectedmat).m[3][1] = 0.0f; U(expectedmat).m[3][2] = -2.690547f; U(expectedmat).m[3][3] = 0.0f;
    D3DXMatrixPerspectiveOffCenterLH(&gotmat, 0.2f, 0.75f, -2.4f, 8.7f, 3.2f, -16.9f);
    expect_mat(&expectedmat, &gotmat);

/*____________D3DXMatrixPerspectiveOffCenterRH_______________*/
    U(expectedmat).m[0][0] = 11.636364f; U(expectedmat).m[0][1] = 0.0f; U(expectedmat).m[0][2] = 0.0f; U(expectedmat).m[0][3] = 0.0f;
    U(expectedmat).m[1][0] = 0.0f; U(expectedmat).m[1][1] = 0.576577f; U(expectedmat).m[1][2] = 0.0; U(expectedmat).m[1][3] = 0.0f;
    U(expectedmat).m[2][0] = 1.727273f; U(expectedmat).m[2][1] = 0.567568f; U(expectedmat).m[2][2] = -0.840796f; U(expectedmat).m[2][3] = -1.0f;
    U(expectedmat).m[3][0] = 0.0f; U(expectedmat).m[3][1] = 0.0f; U(expectedmat).m[3][2] = -2.690547f; U(expectedmat).m[3][3] = 0.0f;
    D3DXMatrixPerspectiveOffCenterRH(&gotmat, 0.2f, 0.75f, -2.4f, 8.7f, 3.2f, -16.9f);
    expect_mat(&expectedmat, &gotmat);

/*____________D3DXMatrixPerspectiveRH_______________*/
    U(expectedmat).m[0][0] = -24.0f; U(expectedmat).m[0][1] = -0.0f; U(expectedmat).m[0][2] = 0.0f; U(expectedmat).m[0][3] = 0.0f;
    U(expectedmat).m[1][0] = 0.0f; U(expectedmat).m[1][1] = -6.4f; U(expectedmat).m[1][2] = 0.0; U(expectedmat).m[1][3] = 0.0f;
    U(expectedmat).m[2][0] = 0.0f; U(expectedmat).m[2][1] = 0.0f; U(expectedmat).m[2][2] = -0.783784f; U(expectedmat).m[2][3] = -1.0f;
    U(expectedmat).m[3][0] = 0.0f; U(expectedmat).m[3][1] = 0.0f; U(expectedmat).m[3][2] = 1.881081f; U(expectedmat).m[3][3] = 0.0f;
    D3DXMatrixPerspectiveRH(&gotmat, 0.2f, 0.75f, -2.4f, 8.7f);
    expect_mat(&expectedmat, &gotmat);

/*____________D3DXMatrixReflect______________*/
    U(expectedmat).m[0][0] = 0.307692f; U(expectedmat).m[0][1] = -0.230769f; U(expectedmat).m[0][2] = 0.923077f; U(expectedmat).m[0][3] = 0.0f;
    U(expectedmat).m[1][0] = -0.230769; U(expectedmat).m[1][1] = 0.923077f; U(expectedmat).m[1][2] = 0.307693f; U(expectedmat).m[1][3] = 0.0f;
    U(expectedmat).m[2][0] = 0.923077f; U(expectedmat).m[2][1] = 0.307693f; U(expectedmat).m[2][2] = -0.230769f; U(expectedmat).m[2][3] = 0.0f;
    U(expectedmat).m[3][0] = 1.615385f; U(expectedmat).m[3][1] = 0.538462f; U(expectedmat).m[3][2] = -2.153846f; U(expectedmat).m[3][3] = 1.0f;
    D3DXMatrixReflect(&gotmat,&plane);
    expect_mat(&expectedmat, &gotmat);

/*____________D3DXMatrixRotationAxis_____*/
    U(expectedmat).m[0][0] = 0.508475f; U(expectedmat).m[0][1] = 0.763805f; U(expectedmat).m[0][2] = 0.397563f; U(expectedmat).m[0][3] = 0.0f;
    U(expectedmat).m[1][0] = -0.814652f; U(expectedmat).m[1][1] = 0.576271f; U(expectedmat).m[1][2] = -0.065219f; U(expectedmat).m[1][3] = 0.0f;
    U(expectedmat).m[2][0] = -0.278919f; U(expectedmat).m[2][1] = -0.290713f; U(expectedmat).m[2][2] = 0.915254f; U(expectedmat).m[2][3] = 0.0f;
    U(expectedmat).m[3][0] = 0.0f; U(expectedmat).m[3][1] = 0.0f; U(expectedmat).m[3][2] = 0.0f; U(expectedmat).m[3][3] = 1.0f;
    D3DXMatrixRotationAxis(&gotmat,&axis,angle);
    expect_mat(&expectedmat, &gotmat);

/*____________D3DXMatrixRotationQuaternion______________*/
    U(expectedmat).m[0][0] = -129.0f; U(expectedmat).m[0][1] = -162.0f; U(expectedmat).m[0][2] = -74.0f; U(expectedmat).m[0][3] = 0.0f;
    U(expectedmat).m[1][0] = 146.0f; U(expectedmat).m[1][1] = -99.0f; U(expectedmat).m[1][2] = -78.0f; U(expectedmat).m[1][3] = 0.0f;
    U(expectedmat).m[2][0] = 102.0f; U(expectedmat).m[2][1] = -34.0f; U(expectedmat).m[2][2] = -33.0f; U(expectedmat).m[2][3] = 0.0f;
    U(expectedmat).m[3][0] = 0.0f; U(expectedmat).m[3][1] = 0.0f; U(expectedmat).m[3][2] = 0.0f; U(expectedmat).m[3][3] = 1.0f;
    D3DXMatrixRotationQuaternion(&gotmat,&q);
    expect_mat(&expectedmat, &gotmat);

/*____________D3DXMatrixRotationX______________*/
    U(expectedmat).m[0][0] = 1.0f; U(expectedmat).m[0][1] = 0.0f; U(expectedmat).m[0][2] = 0.0f; U(expectedmat).m[0][3] = 0.0f;
    U(expectedmat).m[1][0] = 0.0; U(expectedmat).m[1][1] = 0.5f; U(expectedmat).m[1][2] = sqrt(3.0f)/2.0f; U(expectedmat).m[1][3] = 0.0f;
    U(expectedmat).m[2][0] = 0.0f; U(expectedmat).m[2][1] = -sqrt(3.0f)/2.0f; U(expectedmat).m[2][2] = 0.5f; U(expectedmat).m[2][3] = 0.0f;
    U(expectedmat).m[3][0] = 0.0f; U(expectedmat).m[3][1] = 0.0f; U(expectedmat).m[3][2] = 0.0f; U(expectedmat).m[3][3] = 1.0f;
    D3DXMatrixRotationX(&gotmat,angle);
    expect_mat(&expectedmat, &gotmat);

/*____________D3DXMatrixRotationY______________*/
    U(expectedmat).m[0][0] = 0.5f; U(expectedmat).m[0][1] = 0.0f; U(expectedmat).m[0][2] = -sqrt(3.0f)/2.0f; U(expectedmat).m[0][3] = 0.0f;
    U(expectedmat).m[1][0] = 0.0; U(expectedmat).m[1][1] = 1.0f; U(expectedmat).m[1][2] = 0.0f; U(expectedmat).m[1][3] = 0.0f;
    U(expectedmat).m[2][0] = sqrt(3.0f)/2.0f; U(expectedmat).m[2][1] = 0.0f; U(expectedmat).m[2][2] = 0.5f; U(expectedmat).m[2][3] = 0.0f;
    U(expectedmat).m[3][0] = 0.0f; U(expectedmat).m[3][1] = 0.0f; U(expectedmat).m[3][2] = 0.0f; U(expectedmat).m[3][3] = 1.0f;
    D3DXMatrixRotationY(&gotmat,angle);
    expect_mat(&expectedmat, &gotmat);

/*____________D3DXMatrixRotationYawPitchRoll____*/
    U(expectedmat).m[0][0] = 0.888777f; U(expectedmat).m[0][1] = 0.091875f; U(expectedmat).m[0][2] = -0.449037f; U(expectedmat).m[0][3] = 0.0f;
    U(expectedmat).m[1][0] = 0.351713f; U(expectedmat).m[1][1] = 0.491487f; U(expectedmat).m[1][2] = 0.796705f; U(expectedmat).m[1][3] = 0.0f;
    U(expectedmat).m[2][0] = 0.293893f; U(expectedmat).m[2][1] = -0.866025f; U(expectedmat).m[2][2] = 0.404509f; U(expectedmat).m[2][3] = 0.0f;
    U(expectedmat).m[3][0] = 0.0f; U(expectedmat).m[3][1] = 0.0f; U(expectedmat).m[3][2] = 0.0f; U(expectedmat).m[3][3] = 1.0f;
    D3DXMatrixRotationYawPitchRoll(&gotmat, 3.0f*angle/5.0f, angle, 3.0f*angle/17.0f);
    expect_mat(&expectedmat, &gotmat);

/*____________D3DXMatrixRotationZ______________*/
    U(expectedmat).m[0][0] = 0.5f; U(expectedmat).m[0][1] = sqrt(3.0f)/2.0f; U(expectedmat).m[0][2] = 0.0f; U(expectedmat).m[0][3] = 0.0f;
    U(expectedmat).m[1][0] = -sqrt(3.0f)/2.0f; U(expectedmat).m[1][1] = 0.5f; U(expectedmat).m[1][2] = 0.0f; U(expectedmat).m[1][3] = 0.0f;
    U(expectedmat).m[2][0] = 0.0f; U(expectedmat).m[2][1] = 0.0f; U(expectedmat).m[2][2] = 1.0f; U(expectedmat).m[2][3] = 0.0f;
    U(expectedmat).m[3][0] = 0.0f; U(expectedmat).m[3][1] = 0.0f; U(expectedmat).m[3][2] = 0.0f; U(expectedmat).m[3][3] = 1.0f;
    D3DXMatrixRotationZ(&gotmat,angle);
    expect_mat(&expectedmat, &gotmat);

/*____________D3DXMatrixScaling______________*/
    U(expectedmat).m[0][0] = 0.69f; U(expectedmat).m[0][1] = 0.0f; U(expectedmat).m[0][2] = 0.0f; U(expectedmat).m[0][3] = 0.0f;
    U(expectedmat).m[1][0] = 0.0; U(expectedmat).m[1][1] = 0.53f; U(expectedmat).m[1][2] = 0.0f; U(expectedmat).m[1][3] = 0.0f;
    U(expectedmat).m[2][0] = 0.0f; U(expectedmat).m[2][1] = 0.0f; U(expectedmat).m[2][2] = 4.11f; U(expectedmat).m[2][3] = 0.0f;
    U(expectedmat).m[3][0] = 0.0f; U(expectedmat).m[3][1] = 0.0f; U(expectedmat).m[3][2] = 0.0f; U(expectedmat).m[3][3] = 1.0f;
    D3DXMatrixScaling(&gotmat,0.69f,0.53f,4.11f);
    expect_mat(&expectedmat, &gotmat);

/*____________D3DXMatrixShadow______________*/
    U(expectedmat).m[0][0] = 12.786773f; U(expectedmat).m[0][1] = 5.000961f; U(expectedmat).m[0][2] = 4.353778f; U(expectedmat).m[0][3] = 3.706595f;
    U(expectedmat).m[1][0] = 1.882715; U(expectedmat).m[1][1] = 8.805615f; U(expectedmat).m[1][2] = 1.451259f; U(expectedmat).m[1][3] = 1.235532f;
    U(expectedmat).m[2][0] = -7.530860f; U(expectedmat).m[2][1] = -6.667949f; U(expectedmat).m[2][2] = 1.333590f; U(expectedmat).m[2][3] = -4.942127f;
    U(expectedmat).m[3][0] = -13.179006f; U(expectedmat).m[3][1] = -11.668910f; U(expectedmat).m[3][2] = -10.158816f; U(expectedmat).m[3][3] = -1.510094f;
    D3DXMatrixShadow(&gotmat,&light,&plane);
    expect_mat(&expectedmat, &gotmat);

/*____________D3DXMatrixTransformation______________*/
    U(expectedmat).m[0][0] = -0.2148f; U(expectedmat).m[0][1] = 1.3116f; U(expectedmat).m[0][2] = 0.4752f; U(expectedmat).m[0][3] = 0.0f;
    U(expectedmat).m[1][0] = 0.9504f; U(expectedmat).m[1][1] = -0.8836f; U(expectedmat).m[1][2] = 0.9244f; U(expectedmat).m[1][3] = 0.0f;
    U(expectedmat).m[2][0] = 1.0212f; U(expectedmat).m[2][1] = 0.1936f; U(expectedmat).m[2][2] = -1.3588f; U(expectedmat).m[2][3] = 0.0f;
    U(expectedmat).m[3][0] = 18.2985f; U(expectedmat).m[3][1] = -29.624001f; U(expectedmat).m[3][2] = 15.683499f; U(expectedmat).m[3][3] = 1.0f;
    D3DXMatrixTransformation(&gotmat,&at,&q,NULL,&eye,&r,&last);
    expect_mat(&expectedmat, &gotmat);

/*____________D3DXMatrixTranslation______________*/
    U(expectedmat).m[0][0] = 1.0f; U(expectedmat).m[0][1] = 0.0f; U(expectedmat).m[0][2] = 0.0f; U(expectedmat).m[0][3] = 0.0f;
    U(expectedmat).m[1][0] = 0.0; U(expectedmat).m[1][1] = 1.0f; U(expectedmat).m[1][2] = 0.0f; U(expectedmat).m[1][3] = 0.0f;
    U(expectedmat).m[2][0] = 0.0f; U(expectedmat).m[2][1] = 0.0f; U(expectedmat).m[2][2] = 1.0f; U(expectedmat).m[2][3] = 0.0f;
    U(expectedmat).m[3][0] = 0.69f; U(expectedmat).m[3][1] = 0.53f; U(expectedmat).m[3][2] = 4.11f; U(expectedmat).m[3][3] = 1.0f;
    D3DXMatrixTranslation(&gotmat,0.69f,0.53f,4.11f);
    expect_mat(&expectedmat, &gotmat);

/*____________D3DXMatrixTranspose______________*/
    U(expectedmat).m[0][0] = 10.0f; U(expectedmat).m[0][1] = 11.0f; U(expectedmat).m[0][2] = 19.0f; U(expectedmat).m[0][3] = 2.0f;
    U(expectedmat).m[1][0] = 5.0; U(expectedmat).m[1][1] = 20.0f; U(expectedmat).m[1][2] = -21.0f; U(expectedmat).m[1][3] = 3.0f;
    U(expectedmat).m[2][0] = 7.0f; U(expectedmat).m[2][1] = 16.0f; U(expectedmat).m[2][2] = 30.f; U(expectedmat).m[2][3] = -4.0f;
    U(expectedmat).m[3][0] = 8.0f; U(expectedmat).m[3][1] = 33.0f; U(expectedmat).m[3][2] = 43.0f; U(expectedmat).m[3][3] = -40.0f;
    D3DXMatrixTranspose(&gotmat,&mat);
    expect_mat(&expectedmat, &gotmat);
}

static void D3DXPlaneTest(void)
{
    D3DXMATRIX mat;
    D3DXPLANE expectedplane, gotplane, nulplane, plane;
    D3DXVECTOR3 expectedvec, gotvec, vec1, vec2, vec3;
    LPD3DXVECTOR3 funcpointer;
    D3DXVECTOR4 vec;
    FLOAT expected, got;

    U(mat).m[0][1] = 5.0f; U(mat).m[0][2] = 7.0f; U(mat).m[0][3] = 8.0f;
    U(mat).m[1][0] = 11.0f; U(mat).m[1][2] = 16.0f; U(mat).m[1][3] = 33.0f;
    U(mat).m[2][0] = 19.0f; U(mat).m[2][1] = -21.0f; U(mat).m[2][3] = 43.0f;
    U(mat).m[3][0] = 2.0f; U(mat).m[3][1] = 3.0f; U(mat).m[3][2] = -4.0f;
    U(mat).m[0][0] = 10.0f; U(mat).m[1][1] = 20.0f; U(mat).m[2][2] = 30.0f;
    U(mat).m[3][3] = -40.0f;

    plane.a = -3.0f; plane.b = -1.0f; plane.c = 4.0f; plane.d = 7.0f;

    vec.x = 2.0f; vec.y = 5.0f; vec.z = -6.0f; vec.w = 11.0f;

/*_______________D3DXPlaneDot________________*/
    expected = 42.0f;
    got = D3DXPlaneDot(&plane,&vec),
    ok( expected == got, "Expected : %f, Got : %f\n",expected, got);
    expected = 0.0f;
    got = D3DXPlaneDot(NULL,&vec),
    ok( expected == got, "Expected : %f, Got : %f\n",expected, got);
    expected = 0.0f;
    got = D3DXPlaneDot(NULL,NULL),
    ok( expected == got, "Expected : %f, Got : %f\n",expected, got);

/*_______________D3DXPlaneDotCoord________________*/
    expected = -28.0f;
    got = D3DXPlaneDotCoord(&plane,&vec),
    ok( expected == got, "Expected : %f, Got : %f\n",expected, got);
    expected = 0.0f;
    got = D3DXPlaneDotCoord(NULL,&vec),
    ok( expected == got, "Expected : %f, Got : %f\n",expected, got);
    expected = 0.0f;
    got = D3DXPlaneDotCoord(NULL,NULL),
    ok( expected == got, "Expected : %f, Got : %f\n",expected, got);

/*_______________D3DXPlaneDotNormal______________*/
    expected = -35.0f;
    got = D3DXPlaneDotNormal(&plane,&vec),
    ok( expected == got, "Expected : %f, Got : %f\n",expected, got);
    expected = 0.0f;
    got = D3DXPlaneDotNormal(NULL,&vec),
    ok( expected == got, "Expected : %f, Got : %f\n",expected, got);
    expected = 0.0f;
    got = D3DXPlaneDotNormal(NULL,NULL),
    ok( expected == got, "Expected : %f, Got : %f\n",expected, got);

/*_______________D3DXPlaneFromPointNormal_______*/
    vec1.x = 11.0f; vec1.y = 13.0f; vec1.z = 15.0f;
    vec2.x = 17.0f; vec2.y = 31.0f; vec2.z = 24.0f;
    expectedplane.a = 17.0f; expectedplane.b = 31.0f; expectedplane.c = 24.0f; expectedplane.d = -950.0f;
    D3DXPlaneFromPointNormal(&gotplane,&vec1,&vec2);
    expect_plane(expectedplane, gotplane);

/*_______________D3DXPlaneFromPoints_______*/
    vec1.x = 1.0f; vec1.y = 2.0f; vec1.z = 3.0f;
    vec2.x = 1.0f; vec2.y = -6.0f; vec2.z = -5.0f;
    vec3.x = 83.0f; vec3.y = 74.0f; vec3.z = 65.0f;
    expectedplane.a = 0.085914f; expectedplane.b = -0.704492f; expectedplane.c = 0.704492f; expectedplane.d = -0.790406f;
    D3DXPlaneFromPoints(&gotplane,&vec1,&vec2,&vec3);
    expect_plane(expectedplane, gotplane);

/*_______________D3DXPlaneIntersectLine___________*/
    vec1.x = 9.0f; vec1.y = 6.0f; vec1.z = 3.0f;
    vec2.x = 2.0f; vec2.y = 5.0f; vec2.z = 8.0f;
    expectedvec.x = 20.0f/3.0f; expectedvec.y = 17.0f/3.0f; expectedvec.z = 14.0f/3.0f;
    D3DXPlaneIntersectLine(&gotvec,&plane,&vec1,&vec2);
    expect_vec3(expectedvec, gotvec);
    /* Test a parallel line */
    vec1.x = 11.0f; vec1.y = 13.0f; vec1.z = 15.0f;
    vec2.x = 17.0f; vec2.y = 31.0f; vec2.z = 24.0f;
    expectedvec.x = 20.0f/3.0f; expectedvec.y = 17.0f/3.0f; expectedvec.z = 14.0f/3.0f;
    funcpointer = D3DXPlaneIntersectLine(&gotvec,&plane,&vec1,&vec2);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);

/*_______________D3DXPlaneNormalize______________*/
    expectedplane.a = -3.0f/sqrt(26.0f); expectedplane.b = -1.0f/sqrt(26.0f); expectedplane.c = 4.0f/sqrt(26.0f); expectedplane.d = 7.0/sqrt(26.0f);
    D3DXPlaneNormalize(&gotplane, &plane);
    expect_plane(expectedplane, gotplane);
    nulplane.a = 0.0; nulplane.b = 0.0f, nulplane.c = 0.0f; nulplane.d = 0.0f;
    expectedplane.a = 0.0f; expectedplane.b = 0.0f; expectedplane.c = 0.0f; expectedplane.d = 0.0f;
    D3DXPlaneNormalize(&gotplane, &nulplane);
    expect_plane(expectedplane, gotplane);

/*_______________D3DXPlaneTransform____________*/
    expectedplane.a = 49.0f; expectedplane.b = -98.0f; expectedplane.c = 55.0f; expectedplane.d = -165.0f;
    D3DXPlaneTransform(&gotplane,&plane,&mat);
    expect_plane(expectedplane, gotplane);
}

static void D3DXQuaternionTest(void)
{
    D3DXMATRIX mat;
    D3DXQUATERNION expectedquat, gotquat, Nq, Nq1, nul, smallq, smallr, q, r, s, t, u;
    LPD3DXQUATERNION funcpointer;
    D3DXVECTOR3 axis, expectedvec;
    FLOAT angle, expected, got, scale, scale2;
    BOOL expectedbool, gotbool;

    nul.x = 0.0f; nul.y = 0.0f; nul.z = 0.0f; nul.w = 0.0f;
    q.x = 1.0f, q.y = 2.0f; q.z = 4.0f; q.w = 10.0f;
    r.x = -3.0f; r.y = 4.0f; r.z = -5.0f; r.w = 7.0;
    t.x = -1111.0f, t.y = 111.0f; t.z = -11.0f; t.w = 1.0f;
    u.x = 91.0f; u.y = - 82.0f; u.z = 7.3f; u.w = -6.4f;
    smallq.x = 0.1f; smallq.y = 0.2f; smallq.z= 0.3f; smallq.w = 0.4f;
    smallr.x = 0.5f; smallr.y = 0.6f; smallr.z= 0.7f; smallr.w = 0.8f;

    scale = 0.3f;
    scale2 = 0.78f;

/*_______________D3DXQuaternionBaryCentric________________________*/
    expectedquat.x = -867.444458; expectedquat.y = 87.851111f; expectedquat.z = -9.937778f; expectedquat.w = 3.235555f;
    D3DXQuaternionBaryCentric(&gotquat,&q,&r,&t,scale,scale2);
    expect_vec4(expectedquat,gotquat);

/*_______________D3DXQuaternionConjugate________________*/
    expectedquat.x = -1.0f; expectedquat.y = -2.0f; expectedquat.z = -4.0f; expectedquat.w = 10.0f;
    D3DXQuaternionConjugate(&gotquat,&q);
    expect_vec4(expectedquat,gotquat);
    /* Test the NULL case */
    funcpointer = D3DXQuaternionConjugate(&gotquat,NULL);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXQuaternionConjugate(NULL,NULL);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);

/*_______________D3DXQuaternionDot______________________*/
    expected = 55.0f;
    got = D3DXQuaternionDot(&q,&r);
    ok(relative_error(got, expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);
    /* Tests the case NULL */
    expected=0.0f;
    got = D3DXQuaternionDot(NULL,&r);
    ok(relative_error(got, expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);
    expected=0.0f;
    got = D3DXQuaternionDot(NULL,NULL);
    ok(relative_error(got, expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);

/*_______________D3DXQuaternionExp______________________________*/
    expectedquat.x = -0.216382f; expectedquat.y = -0.432764f; expectedquat.z = -0.8655270f; expectedquat.w = -0.129449f;
    D3DXQuaternionExp(&gotquat,&q);
    expect_vec4(expectedquat,gotquat);
    /* Test the null quaternion */
    expectedquat.x = 0.0f; expectedquat.y = 0.0f; expectedquat.z = 0.0f; expectedquat.w = 1.0f;
    D3DXQuaternionExp(&gotquat,&nul);
    expect_vec4(expectedquat,gotquat);
    /* Test the case where the norm of the quaternion is <1 */
    Nq1.x = 0.2f; Nq1.y = 0.1f; Nq1.z = 0.3; Nq1.w= 0.9f;
    expectedquat.x = 0.195366; expectedquat.y = 0.097683f; expectedquat.z = 0.293049f; expectedquat.w = 0.930813f;
    D3DXQuaternionExp(&gotquat,&Nq1);
    expect_vec4(expectedquat,gotquat);

/*_______________D3DXQuaternionIdentity________________*/
    expectedquat.x = 0.0f; expectedquat.y = 0.0f; expectedquat.z = 0.0f; expectedquat.w = 1.0f;
    D3DXQuaternionIdentity(&gotquat);
    expect_vec4(expectedquat,gotquat);
    /* Test the NULL case */
    funcpointer = D3DXQuaternionIdentity(NULL);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);

/*_______________D3DXQuaternionInverse________________________*/
    expectedquat.x = -1.0f/121.0f; expectedquat.y = -2.0f/121.0f; expectedquat.z = -4.0f/121.0f; expectedquat.w = 10.0f/121.0f;
    D3DXQuaternionInverse(&gotquat,&q);
    expect_vec4(expectedquat,gotquat);

    expectedquat.x = 1.0f; expectedquat.y = 2.0f; expectedquat.z = 4.0f; expectedquat.w = 10.0f;
    D3DXQuaternionInverse(&gotquat,&gotquat);
    expect_vec4(expectedquat,gotquat);


/*_______________D3DXQuaternionIsIdentity________________*/
    s.x = 0.0f; s.y = 0.0f; s.z = 0.0f; s.w = 1.0f;
    expectedbool = TRUE;
    gotbool = D3DXQuaternionIsIdentity(&s);
    ok( expectedbool == gotbool, "Expected boolean : %d, Got bool : %d\n", expectedbool, gotbool);
    s.x = 2.3f; s.y = -4.2f; s.z = 1.2f; s.w=0.2f;
    expectedbool = FALSE;
    gotbool = D3DXQuaternionIsIdentity(&q);
    ok( expectedbool == gotbool, "Expected boolean : %d, Got bool : %d\n", expectedbool, gotbool);
    /* Test the NULL case */
    gotbool = D3DXQuaternionIsIdentity(NULL);
    ok(gotbool == FALSE, "Expected boolean: %d, Got boolean: %d\n", FALSE, gotbool);

/*_______________D3DXQuaternionLength__________________________*/
   expected = 11.0f;
   got = D3DXQuaternionLength(&q);
   ok(relative_error(got, expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);
   /* Tests the case NULL */
    expected=0.0f;
    got = D3DXQuaternionLength(NULL);
    ok(relative_error(got, expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);

/*_______________D3DXQuaternionLengthSq________________________*/
    expected = 121.0f;
    got = D3DXQuaternionLengthSq(&q);
    ok(relative_error(got, expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);
    /* Tests the case NULL */
    expected=0.0f;
    got = D3DXQuaternionLengthSq(NULL);
    ok(relative_error(got, expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);

/*_______________D3DXQuaternionLn______________________________*/
    expectedquat.x = 1.0f; expectedquat.y = 2.0f; expectedquat.z = 4.0f; expectedquat.w = 0.0f;
    D3DXQuaternionLn(&gotquat,&q);
    expect_vec4(expectedquat,gotquat);
    expectedquat.x = -3.0f; expectedquat.y = 4.0f; expectedquat.z = -5.0f; expectedquat.w = 0.0f;
    D3DXQuaternionLn(&gotquat,&r);
    expect_vec4(expectedquat,gotquat);
    Nq.x = 1.0f/11.0f; Nq.y = 2.0f/11.0f; Nq.z = 4.0f/11.0f; Nq.w=10.0f/11.0f;
    expectedquat.x = 0.093768f; expectedquat.y = 0.187536f; expectedquat.z = 0.375073f; expectedquat.w = 0.0f;
    D3DXQuaternionLn(&gotquat,&Nq);
    expect_vec4(expectedquat,gotquat);
    Nq.x = 0.0f; Nq.y = 0.0f; Nq.z = 0.0f; Nq.w = 1.0f;
    expectedquat.x = 0.0f; expectedquat.y = 0.0f; expectedquat.z = 0.0f; expectedquat.w = 0.0f;
    D3DXQuaternionLn(&gotquat,&Nq);
    expect_vec4(expectedquat,gotquat);
    Nq.x = 5.4f; Nq.y = 1.2f; Nq.z = -0.3f; Nq.w = -0.3f;
    expectedquat.x = 10.616652f; expectedquat.y = 2.359256f; expectedquat.z = -0.589814f; expectedquat.w = 0.0f;
    D3DXQuaternionLn(&gotquat,&Nq);
    expect_vec4(expectedquat,gotquat);
    /* Test the case where the norm of the quaternion is <1 */
    Nq1.x = 0.2f; Nq1.y = 0.1f; Nq1.z = 0.3; Nq1.w = 0.9f;
    expectedquat.x = 0.206945f; expectedquat.y = 0.103473f; expectedquat.z = 0.310418f; expectedquat.w = 0.0f;
    D3DXQuaternionLn(&gotquat,&Nq1);
    expect_vec4(expectedquat,gotquat);
    /* Test the case where the real part of the quaternion is -1.0f */
    Nq1.x = 0.2f; Nq1.y = 0.1f; Nq1.z = 0.3; Nq1.w = -1.0f;
    expectedquat.x = 0.2f; expectedquat.y = 0.1f; expectedquat.z = 0.3f; expectedquat.w = 0.0f;
    D3DXQuaternionLn(&gotquat,&Nq1);
    expect_vec4(expectedquat,gotquat);

/*_______________D3DXQuaternionMultiply________________________*/
    expectedquat.x = 3.0f; expectedquat.y = 61.0f; expectedquat.z = -32.0f; expectedquat.w = 85.0f;
    D3DXQuaternionMultiply(&gotquat,&q,&r);
    expect_vec4(expectedquat,gotquat);

/*_______________D3DXQuaternionNormalize________________________*/
    expectedquat.x = 1.0f/11.0f; expectedquat.y = 2.0f/11.0f; expectedquat.z = 4.0f/11.0f; expectedquat.w = 10.0f/11.0f;
    D3DXQuaternionNormalize(&gotquat,&q);
    expect_vec4(expectedquat,gotquat);

/*_______________D3DXQuaternionRotationAxis___________________*/
    axis.x = 2.0f; axis.y = 7.0; axis.z = 13.0f;
    angle = D3DX_PI/3.0f;
    expectedquat.x = 0.067116; expectedquat.y = 0.234905f; expectedquat.z = 0.436251f; expectedquat.w = 0.866025f;
    D3DXQuaternionRotationAxis(&gotquat,&axis,angle);
    expect_vec4(expectedquat,gotquat);
 /* Test the nul quaternion */
    axis.x = 0.0f; axis.y = 0.0; axis.z = 0.0f;
    expectedquat.x = 0.0f; expectedquat.y = 0.0f; expectedquat.z = 0.0f; expectedquat.w = 0.866025f;
    D3DXQuaternionRotationAxis(&gotquat,&axis,angle);
    expect_vec4(expectedquat,gotquat);

/*_______________D3DXQuaternionRotationMatrix___________________*/
    /* test when the trace is >0 */
    U(mat).m[0][1] = 5.0f; U(mat).m[0][2] = 7.0f; U(mat).m[0][3] = 8.0f;
    U(mat).m[1][0] = 11.0f; U(mat).m[1][2] = 16.0f; U(mat).m[1][3] = 33.0f;
    U(mat).m[2][0] = 19.0f; U(mat).m[2][1] = -21.0f; U(mat).m[2][3] = 43.0f;
    U(mat).m[3][0] = 2.0f; U(mat).m[3][1] = 3.0f; U(mat).m[3][2] = -4.0f;
    U(mat).m[0][0] = 10.0f; U(mat).m[1][1] = 20.0f; U(mat).m[2][2] = 30.0f;
    U(mat).m[3][3] = 48.0f;
    expectedquat.x = 2.368682f; expectedquat.y = 0.768221f; expectedquat.z = -0.384111f; expectedquat.w = 3.905125f;
    D3DXQuaternionRotationMatrix(&gotquat,&mat);
    expect_vec4(expectedquat,gotquat);
    /* test the case when the greater element is (2,2) */
    U(mat).m[0][1] = 5.0f; U(mat).m[0][2] = 7.0f; U(mat).m[0][3] = 8.0f;
    U(mat).m[1][0] = 11.0f; U(mat).m[1][2] = 16.0f; U(mat).m[1][3] = 33.0f;
    U(mat).m[2][0] = 19.0f; U(mat).m[2][1] = -21.0f; U(mat).m[2][3] = 43.0f;
    U(mat).m[3][0] = 2.0f; U(mat).m[3][1] = 3.0f; U(mat).m[3][2] = -4.0f;
    U(mat).m[0][0] = -10.0f; U(mat).m[1][1] = -60.0f; U(mat).m[2][2] = 40.0f;
    U(mat).m[3][3] = 48.0f;
    expectedquat.x = 1.233905f; expectedquat.y = -0.237290f; expectedquat.z = 5.267827f; expectedquat.w = -0.284747f;
    D3DXQuaternionRotationMatrix(&gotquat,&mat);
    expect_vec4(expectedquat,gotquat);
    /* test the case when the greater element is (1,1) */
    U(mat).m[0][1] = 5.0f; U(mat).m[0][2] = 7.0f; U(mat).m[0][3] = 8.0f;
    U(mat).m[1][0] = 11.0f; U(mat).m[1][2] = 16.0f; U(mat).m[1][3] = 33.0f;
    U(mat).m[2][0] = 19.0f; U(mat).m[2][1] = -21.0f; U(mat).m[2][3] = 43.0f;
    U(mat).m[3][0] = 2.0f; U(mat).m[3][1] = 3.0f; U(mat).m[3][2] = -4.0f;
    U(mat).m[0][0] = -10.0f; U(mat).m[1][1] = 60.0f; U(mat).m[2][2] = -80.0f;
    U(mat).m[3][3] = 48.0f;
    expectedquat.x = 0.651031f; expectedquat.y = 6.144103f; expectedquat.z = -0.203447f; expectedquat.w = 0.488273f;
    D3DXQuaternionRotationMatrix(&gotquat,&mat);
    expect_vec4(expectedquat,gotquat);
    /* test the case when the trace is near 0 in a matrix which is not a rotation */
    U(mat).m[0][1] = 5.0f; U(mat).m[0][2] = 7.0f; U(mat).m[0][3] = 8.0f;
    U(mat).m[1][0] = 11.0f; U(mat).m[1][2] = 16.0f; U(mat).m[1][3] = 33.0f;
    U(mat).m[2][0] = 19.0f; U(mat).m[2][1] = -21.0f; U(mat).m[2][3] = 43.0f;
    U(mat).m[3][0] = 2.0f; U(mat).m[3][1] = 3.0f; U(mat).m[3][2] = -4.0f;
    U(mat).m[0][0] = -10.0f; U(mat).m[1][1] = 10.0f; U(mat).m[2][2] = -0.9f;
    U(mat).m[3][3] = 48.0f;
    expectedquat.x = 1.709495f; expectedquat.y = 2.339872f; expectedquat.z = -0.534217f; expectedquat.w = 1.282122f;
    D3DXQuaternionRotationMatrix(&gotquat,&mat);
    expect_vec4(expectedquat,gotquat);
    /* test the case when the trace is 0.49 in a matrix which is not a rotation */
    U(mat).m[0][1] = 5.0f; U(mat).m[0][2] = 7.0f; U(mat).m[0][3] = 8.0f;
    U(mat).m[1][0] = 11.0f; U(mat).m[1][2] = 16.0f; U(mat).m[1][3] = 33.0f;
    U(mat).m[2][0] = 19.0f; U(mat).m[2][1] = -21.0f; U(mat).m[2][3] = 43.0f;
    U(mat).m[3][0] = 2.0f; U(mat).m[3][1] = 3.0f; U(mat).m[3][2] = -4.0f;
    U(mat).m[0][0] = -10.0f; U(mat).m[1][1] = 10.0f; U(mat).m[2][2] = -0.51f;
    U(mat).m[3][3] = 48.0f;
    expectedquat.x = 1.724923f; expectedquat.y = 2.318944f; expectedquat.z = -0.539039f; expectedquat.w = 1.293692f;
    D3DXQuaternionRotationMatrix(&gotquat,&mat);
    expect_vec4(expectedquat,gotquat);
    /* test the case when the trace is 0.51 in a matrix which is not a rotation */
    U(mat).m[0][1] = 5.0f; U(mat).m[0][2] = 7.0f; U(mat).m[0][3] = 8.0f;
    U(mat).m[1][0] = 11.0f; U(mat).m[1][2] = 16.0f; U(mat).m[1][3] = 33.0f;
    U(mat).m[2][0] = 19.0f; U(mat).m[2][1] = -21.0f; U(mat).m[2][3] = 43.0f;
    U(mat).m[3][0] = 2.0f; U(mat).m[3][1] = 3.0f; U(mat).m[3][2] = -4.0f;
    U(mat).m[0][0] = -10.0f; U(mat).m[1][1] = 10.0f; U(mat).m[2][2] = -0.49f;
    U(mat).m[3][3] = 48.0f;
    expectedquat.x = 1.725726f; expectedquat.y = 2.317865f; expectedquat.z = -0.539289f; expectedquat.w = 1.294294f;
    D3DXQuaternionRotationMatrix(&gotquat,&mat);
    expect_vec4(expectedquat,gotquat);
    /* test the case when the trace is 0.99 in a matrix which is not a rotation */
    U(mat).m[0][1] = 5.0f; U(mat).m[0][2] = 7.0f; U(mat).m[0][3] = 8.0f;
    U(mat).m[1][0] = 11.0f; U(mat).m[1][2] = 16.0f; U(mat).m[1][3] = 33.0f;
    U(mat).m[2][0] = 19.0f; U(mat).m[2][1] = -21.0f; U(mat).m[2][3] = 43.0f;
    U(mat).m[3][0] = 2.0f; U(mat).m[3][1] = 3.0f; U(mat).m[3][2] = -4.0f;
    U(mat).m[0][0] = -10.0f; U(mat).m[1][1] = 10.0f; U(mat).m[2][2] = -0.01f;
    U(mat).m[3][3] = 48.0f;
    expectedquat.x = 1.745328f; expectedquat.y = 2.291833f; expectedquat.z = -0.545415f; expectedquat.w = 1.308996f;
    D3DXQuaternionRotationMatrix(&gotquat,&mat);
    expect_vec4(expectedquat,gotquat);
    /* test the case when the trace is 1.0 in a matrix which is not a rotation */
    U(mat).m[0][1] = 5.0f; U(mat).m[0][2] = 7.0f; U(mat).m[0][3] = 8.0f;
    U(mat).m[1][0] = 11.0f; U(mat).m[1][2] = 16.0f; U(mat).m[1][3] = 33.0f;
    U(mat).m[2][0] = 19.0f; U(mat).m[2][1] = -21.0f; U(mat).m[2][3] = 43.0f;
    U(mat).m[3][0] = 2.0f; U(mat).m[3][1] = 3.0f; U(mat).m[3][2] = -4.0f;
    U(mat).m[0][0] = -10.0f; U(mat).m[1][1] = 10.0f; U(mat).m[2][2] = 0.0f;
    U(mat).m[3][3] = 48.0f;
    expectedquat.x = 1.745743f; expectedquat.y = 2.291288f; expectedquat.z = -0.545545f; expectedquat.w = 1.309307f;
    D3DXQuaternionRotationMatrix(&gotquat,&mat);
    expect_vec4(expectedquat,gotquat);
    /* test the case when the trace is 1.01 in a matrix which is not a rotation */
    U(mat).m[0][1] = 5.0f; U(mat).m[0][2] = 7.0f; U(mat).m[0][3] = 8.0f;
    U(mat).m[1][0] = 11.0f; U(mat).m[1][2] = 16.0f; U(mat).m[1][3] = 33.0f;
    U(mat).m[2][0] = 19.0f; U(mat).m[2][1] = -21.0f; U(mat).m[2][3] = 43.0f;
    U(mat).m[3][0] = 2.0f; U(mat).m[3][1] = 3.0f; U(mat).m[3][2] = -4.0f;
    U(mat).m[0][0] = -10.0f; U(mat).m[1][1] = 10.0f; U(mat).m[2][2] = 0.01f;
    U(mat).m[3][3] = 48.0f;
    expectedquat.x = 18.408188f; expectedquat.y = 5.970223f; expectedquat.z = -2.985111f; expectedquat.w = 0.502494f;
    D3DXQuaternionRotationMatrix(&gotquat,&mat);
    expect_vec4(expectedquat,gotquat);
    /* test the case when the trace is 1.5 in a matrix which is not a rotation */
    U(mat).m[0][1] = 5.0f; U(mat).m[0][2] = 7.0f; U(mat).m[0][3] = 8.0f;
    U(mat).m[1][0] = 11.0f; U(mat).m[1][2] = 16.0f; U(mat).m[1][3] = 33.0f;
    U(mat).m[2][0] = 19.0f; U(mat).m[2][1] = -21.0f; U(mat).m[2][3] = 43.0f;
    U(mat).m[3][0] = 2.0f; U(mat).m[3][1] = 3.0f; U(mat).m[3][2] = -4.0f;
    U(mat).m[0][0] = -10.0f; U(mat).m[1][1] = 10.0f; U(mat).m[2][2] = 0.5f;
    U(mat).m[3][3] = 48.0f;
    expectedquat.x = 15.105186f; expectedquat.y = 4.898980f; expectedquat.z = -2.449490f; expectedquat.w = 0.612372f;
    D3DXQuaternionRotationMatrix(&gotquat,&mat);
    expect_vec4(expectedquat,gotquat);
    /* test the case when the trace is 1.7 in a matrix which is not a rotation */
    U(mat).m[0][1] = 5.0f; U(mat).m[0][2] = 7.0f; U(mat).m[0][3] = 8.0f;
    U(mat).m[1][0] = 11.0f; U(mat).m[1][2] = 16.0f; U(mat).m[1][3] = 33.0f;
    U(mat).m[2][0] = 19.0f; U(mat).m[2][1] = -21.0f; U(mat).m[2][3] = 43.0f;
    U(mat).m[3][0] = 2.0f; U(mat).m[3][1] = 3.0f; U(mat).m[3][2] = -4.0f;
    U(mat).m[0][0] = -10.0f; U(mat).m[1][1] = 10.0f; U(mat).m[2][2] = 0.70f;
    U(mat).m[3][3] = 48.0f;
    expectedquat.x = 14.188852f; expectedquat.y = 4.601790f; expectedquat.z = -2.300895f; expectedquat.w = 0.651920f;
    D3DXQuaternionRotationMatrix(&gotquat,&mat);
    expect_vec4(expectedquat,gotquat);
    /* test the case when the trace is 1.99 in a matrix which is not a rotation */
    U(mat).m[0][1] = 5.0f; U(mat).m[0][2] = 7.0f; U(mat).m[0][3] = 8.0f;
    U(mat).m[1][0] = 11.0f; U(mat).m[1][2] = 16.0f; U(mat).m[1][3] = 33.0f;
    U(mat).m[2][0] = 19.0f; U(mat).m[2][1] = -21.0f; U(mat).m[2][3] = 43.0f;
    U(mat).m[3][0] = 2.0f; U(mat).m[3][1] = 3.0f; U(mat).m[3][2] = -4.0f;
    U(mat).m[0][0] = -10.0f; U(mat).m[1][1] = 10.0f; U(mat).m[2][2] = 0.99f;
    U(mat).m[3][3] = 48.0f;
    expectedquat.x = 13.114303f; expectedquat.y = 4.253287f; expectedquat.z = -2.126644f; expectedquat.w = 0.705337f;
    D3DXQuaternionRotationMatrix(&gotquat,&mat);
    expect_vec4(expectedquat,gotquat);
    /* test the case when the trace is 2.0 in a matrix which is not a rotation */
    U(mat).m[0][1] = 5.0f; U(mat).m[0][2] = 7.0f; U(mat).m[0][3] = 8.0f;
    U(mat).m[1][0] = 11.0f; U(mat).m[1][2] = 16.0f; U(mat).m[1][3] = 33.0f;
    U(mat).m[2][0] = 19.0f; U(mat).m[2][1] = -21.0f; U(mat).m[2][3] = 43.0f;
    U(mat).m[3][0] = 2.0f; U(mat).m[3][1] = 3.0f; U(mat).m[3][2] = -4.0f;
    U(mat).m[0][0] = -10.0f; U(mat).m[1][1] = 10.0f; U(mat).m[2][2] = 2.0f;
    U(mat).m[3][3] = 48.0f;
    expectedquat.x = 10.680980f; expectedquat.y = 3.464102f; expectedquat.z = -1.732051f; expectedquat.w = 0.866025f;
    D3DXQuaternionRotationMatrix(&gotquat,&mat);
    expect_vec4(expectedquat,gotquat);

/*_______________D3DXQuaternionRotationYawPitchRoll__________*/
    expectedquat.x = 0.303261f; expectedquat.y = 0.262299f; expectedquat.z = 0.410073f; expectedquat.w = 0.819190f;
    D3DXQuaternionRotationYawPitchRoll(&gotquat,D3DX_PI/4.0f,D3DX_PI/11.0f,D3DX_PI/3.0f);
    expect_vec4(expectedquat,gotquat);

/*_______________D3DXQuaternionSlerp________________________*/
    expectedquat.x = -0.2f; expectedquat.y = 2.6f; expectedquat.z = 1.3f; expectedquat.w = 9.1f;
    D3DXQuaternionSlerp(&gotquat,&q,&r,scale);
    expect_vec4(expectedquat,gotquat);
    expectedquat.x = 334.0f; expectedquat.y = -31.9f; expectedquat.z = 6.1f; expectedquat.w = 6.7f;
    D3DXQuaternionSlerp(&gotquat,&q,&t,scale);
    expect_vec4(expectedquat,gotquat);
    expectedquat.x = 0.239485f; expectedquat.y = 0.346580f; expectedquat.z = 0.453676f; expectedquat.w = 0.560772f;
    D3DXQuaternionSlerp(&gotquat,&smallq,&smallr,scale);
    expect_vec4(expectedquat,gotquat);

/*_______________D3DXQuaternionSquad________________________*/
    expectedquat.x = -156.296f; expectedquat.y = 30.242f; expectedquat.z = -2.5022f; expectedquat.w = 7.3576f;
    D3DXQuaternionSquad(&gotquat,&q,&r,&t,&u,scale);
    expect_vec4(expectedquat,gotquat);

/*_______________D3DXQuaternionSquadSetup___________________*/
    r.x = 1.0f, r.y = 2.0f; r.z = 4.0f; r.w = 10.0f;
    s.x = -3.0f; s.y = 4.0f; s.z = -5.0f; s.w = 7.0;
    t.x = -1111.0f, t.y = 111.0f; t.z = -11.0f; t.w = 1.0f;
    u.x = 91.0f; u.y = - 82.0f; u.z = 7.3f; u.w = -6.4f;
    D3DXQuaternionSquadSetup(&gotquat,&Nq,&Nq1,&r,&s,&t,&u);
    expectedquat.x = 7.121285f; expectedquat.y = 2.159964f; expectedquat.z = -3.855094f; expectedquat.w = 5.362844f;
    expect_vec4(expectedquat,gotquat);
    expectedquat.x = -1113.492920f; expectedquat.y = 82.679260f; expectedquat.z = -6.696645f; expectedquat.w = -4.090050f;
    expect_vec4(expectedquat,Nq);
    expectedquat.x = -1111.0f; expectedquat.y = 111.0f; expectedquat.z = -11.0f; expectedquat.w = 1.0f;
    expect_vec4(expectedquat,Nq1);
    r.x = 0.2f; r.y = 0.3f; r.z = 1.3f; r.w = -0.6f;
    s.x = -3.0f; s.y =-2.0f; s.z = 4.0f; s.w = 0.2f;
    t.x = 0.4f; t.y = 8.3f; t.z = -3.1f; t.w = -2.7f;
    u.x = 1.1f; u.y = -0.7f; u.z = 9.2f; u.w = 0.0f;
    D3DXQuaternionSquadSetup(&gotquat,&Nq,&Nq1,&r,&s,&u,&t);
    expectedquat.x = -4.139569f; expectedquat.y = -2.469115f; expectedquat.z = 2.364477f; expectedquat.w = 0.465494f;
    expect_vec4(expectedquat,gotquat);
    expectedquat.x = 2.342533f; expectedquat.y = 2.365127f; expectedquat.z = 8.628538f; expectedquat.w = -0.898356f;
    expect_vec4(expectedquat,Nq);
    expectedquat.x = 1.1f; expectedquat.y = -0.7f; expectedquat.z = 9.2f; expectedquat.w = 0.0f;
    expect_vec4(expectedquat,Nq1);
    D3DXQuaternionSquadSetup(&gotquat,&Nq,&Nq1,&r,&s,&t,&u);
    expectedquat.x = -3.754567f; expectedquat.y = -0.586085f; expectedquat.z = 3.815818f; expectedquat.w = -0.198150f;
    expect_vec4(expectedquat,gotquat);
    expectedquat.x = 0.140773f; expectedquat.y = -8.737090f; expectedquat.z = -0.516593f; expectedquat.w = 3.053942f;
    expect_vec4(expectedquat,Nq);
    expectedquat.x = -0.4f; expectedquat.y = -8.3f; expectedquat.z = 3.1f; expectedquat.w = 2.7f;
    expect_vec4(expectedquat,Nq1);
    r.x = -1.0f; r.y = 0.0f; r.z = 0.0f; r.w = 0.0f;
    s.x = 1.0f; s.y =0.0f; s.z = 0.0f; s.w = 0.0f;
    t.x = 1.0f; t.y = 0.0f; t.z = 0.0f; t.w = 0.0f;
    u.x = -1.0f; u.y = 0.0f; u.z = 0.0f; u.w = 0.0f;
    D3DXQuaternionSquadSetup(&gotquat,&Nq,&Nq1,&r,&s,&t,&u);
    expectedquat.x = 1.0f; expectedquat.y = 0.0f; expectedquat.z = 0.0f; expectedquat.w = 0.0f;
    expect_vec4(expectedquat,gotquat);
    expectedquat.x = 1.0f; expectedquat.y = 0.0f; expectedquat.z = 0.0f; expectedquat.w = 0.0f;
    expect_vec4(expectedquat,Nq);
    expectedquat.x = 1.0f; expectedquat.y = 0.0f; expectedquat.z = 0.0f; expectedquat.w = 0.0f;
    expect_vec4(expectedquat,Nq1);

/*_______________D3DXQuaternionToAxisAngle__________________*/
    Nq.x = 1.0f/22.0f; Nq.y = 2.0f/22.0f; Nq.z = 4.0f/22.0f; Nq.w = 10.0f/22.0f;
    expectedvec.x = 1.0f/22.0f; expectedvec.y = 2.0f/22.0f; expectedvec.z = 4.0f/22.0f;
    expected = 2.197869f;
    D3DXQuaternionToAxisAngle(&Nq,&axis,&angle);
    expect_vec3(expectedvec,axis);
    ok(relative_error(angle,  expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, angle);
    /* Test if |w|>1.0f */
    expectedvec.x = 1.0f; expectedvec.y = 2.0f; expectedvec.z = 4.0f;
    D3DXQuaternionToAxisAngle(&q,&axis,&angle);
    expect_vec3(expectedvec,axis);
    /* Test the null quaternion */
    expectedvec.x = 0.0f; expectedvec.y = 0.0f; expectedvec.z = 0.0f;
    expected = 3.141593f;
    D3DXQuaternionToAxisAngle(&nul,&axis,&angle);
    expect_vec3(expectedvec,axis);
    ok(relative_error(angle, expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, angle);
}

static void D3DXVector2Test(void)
{
    D3DXVECTOR2 expectedvec, gotvec, nul, u, v, w, x;
    LPD3DXVECTOR2 funcpointer;
    D3DXVECTOR4 expectedtrans, gottrans;
    D3DXMATRIX mat;
    FLOAT coeff1, coeff2, expected, got, scale;

    nul.x = 0.0f; nul.y = 0.0f;
    u.x = 3.0f; u.y = 4.0f;
    v.x = -7.0f; v.y = 9.0f;
    w.x = 4.0f; w.y = -3.0f;
    x.x = 2.0f; x.y = -11.0f;

    U(mat).m[0][0] = 1.0f; U(mat).m[0][1] = 2.0f; U(mat).m[0][2] = 3.0f; U(mat).m[0][3] = 4.0f;
    U(mat).m[1][0] = 5.0f; U(mat).m[1][1] = 6.0f; U(mat).m[1][2] = 7.0f; U(mat).m[1][3] = 8.0f;
    U(mat).m[2][0] = 9.0f; U(mat).m[2][1] = 10.0f; U(mat).m[2][2] = 11.0f; U(mat).m[2][3] = 12.0f;
    U(mat).m[3][0] = 13.0f; U(mat).m[3][1] = 14.0f; U(mat).m[3][2] = 15.0f; U(mat).m[3][3] = 16.0f;

    coeff1 = 2.0f; coeff2 = 5.0f;
    scale = -6.5f;

/*_______________D3DXVec2Add__________________________*/
    expectedvec.x = -4.0f; expectedvec.y = 13.0f;
    D3DXVec2Add(&gotvec,&u,&v);
    expect_vec(expectedvec,gotvec);
    /* Tests the case NULL */
    funcpointer = D3DXVec2Add(&gotvec,NULL,&v);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXVec2Add(NULL,NULL,NULL);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);

/*_______________D3DXVec2BaryCentric___________________*/
    expectedvec.x = -12.0f; expectedvec.y = -21.0f;
    D3DXVec2BaryCentric(&gotvec,&u,&v,&w,coeff1,coeff2);
    expect_vec(expectedvec,gotvec);

/*_______________D3DXVec2CatmullRom____________________*/
    expectedvec.x = 5820.25f; expectedvec.y = -3654.5625f;
    D3DXVec2CatmullRom(&gotvec,&u,&v,&w,&x,scale);
    expect_vec(expectedvec,gotvec);

/*_______________D3DXVec2CCW__________________________*/
   expected = 55.0f;
   got = D3DXVec2CCW(&u,&v);
   ok(relative_error(got, expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);
   /* Tests the case NULL */
    expected=0.0f;
    got = D3DXVec2CCW(NULL,&v);
    ok(relative_error(got, expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);
    expected=0.0f;
    got = D3DXVec2CCW(NULL,NULL);
    ok(relative_error(got, expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);

/*_______________D3DXVec2Dot__________________________*/
    expected = 15.0f;
    got = D3DXVec2Dot(&u,&v);
    ok(relative_error(got,  expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);
    /* Tests the case NULL */
    expected=0.0f;
    got = D3DXVec2Dot(NULL,&v);
    ok(relative_error(got, expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);
    expected=0.0f;
    got = D3DXVec2Dot(NULL,NULL);
    ok(relative_error(got, expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);

/*_______________D3DXVec2Hermite__________________________*/
    expectedvec.x = 2604.625f; expectedvec.y = -4533.0f;
    D3DXVec2Hermite(&gotvec,&u,&v,&w,&x,scale);
    expect_vec(expectedvec,gotvec);

/*_______________D3DXVec2Length__________________________*/
   expected = 5.0f;
   got = D3DXVec2Length(&u);
   ok(relative_error(got, expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);
   /* Tests the case NULL */
    expected=0.0f;
    got = D3DXVec2Length(NULL);
    ok(relative_error(got, expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);

/*_______________D3DXVec2LengthSq________________________*/
   expected = 25.0f;
   got = D3DXVec2LengthSq(&u);
   ok(relative_error(got, expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);
   /* Tests the case NULL */
    expected=0.0f;
    got = D3DXVec2LengthSq(NULL);
    ok(relative_error(got, expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);

/*_______________D3DXVec2Lerp__________________________*/
   expectedvec.x = 68.0f; expectedvec.y = -28.5f;
   D3DXVec2Lerp(&gotvec,&u,&v,scale);
   expect_vec(expectedvec,gotvec);
   /* Tests the case NULL */
    funcpointer = D3DXVec2Lerp(&gotvec,NULL,&v,scale);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXVec2Lerp(NULL,NULL,NULL,scale);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);

/*_______________D3DXVec2Maximize__________________________*/
   expectedvec.x = 3.0f; expectedvec.y = 9.0f;
   D3DXVec2Maximize(&gotvec,&u,&v);
   expect_vec(expectedvec,gotvec);
   /* Tests the case NULL */
    funcpointer = D3DXVec2Maximize(&gotvec,NULL,&v);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXVec2Maximize(NULL,NULL,NULL);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);

/*_______________D3DXVec2Minimize__________________________*/
    expectedvec.x = -7.0f; expectedvec.y = 4.0f;
    D3DXVec2Minimize(&gotvec,&u,&v);
    expect_vec(expectedvec,gotvec);
    /* Tests the case NULL */
    funcpointer = D3DXVec2Minimize(&gotvec,NULL,&v);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXVec2Minimize(NULL,NULL,NULL);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);

/*_______________D3DXVec2Normalize_________________________*/
    expectedvec.x = 0.6f; expectedvec.y = 0.8f;
    D3DXVec2Normalize(&gotvec,&u);
    expect_vec(expectedvec,gotvec);
    /* Test the nul vector */
    expectedvec.x = 0.0f; expectedvec.y = 0.0f;
    D3DXVec2Normalize(&gotvec,&nul);
    expect_vec(expectedvec,gotvec);

/*_______________D3DXVec2Scale____________________________*/
    expectedvec.x = -19.5f; expectedvec.y = -26.0f;
    D3DXVec2Scale(&gotvec,&u,scale);
    expect_vec(expectedvec,gotvec);
    /* Tests the case NULL */
    funcpointer = D3DXVec2Scale(&gotvec,NULL,scale);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXVec2Scale(NULL,NULL,scale);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);

/*_______________D3DXVec2Subtract__________________________*/
   expectedvec.x = 10.0f; expectedvec.y = -5.0f;
   D3DXVec2Subtract(&gotvec,&u,&v);
   expect_vec(expectedvec,gotvec);
   /* Tests the case NULL */
    funcpointer = D3DXVec2Subtract(&gotvec,NULL,&v);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXVec2Subtract(NULL,NULL,NULL);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);

/*_______________D3DXVec2Transform_______________________*/
    expectedtrans.x = 36.0f; expectedtrans.y = 44.0f; expectedtrans.z = 52.0f; expectedtrans.w = 60.0f;
    D3DXVec2Transform(&gottrans,&u,&mat);
    expect_vec4(expectedtrans,gottrans);

/*_______________D3DXVec2TransformCoord_______________________*/
    expectedvec.x = 0.6f; expectedvec.y = 11.0f/15.0f;
    D3DXVec2TransformCoord(&gotvec,&u,&mat);
    expect_vec(expectedvec,gotvec);

 /*_______________D3DXVec2TransformNormal______________________*/
    expectedvec.x = 23.0f; expectedvec.y = 30.0f;
    D3DXVec2TransformNormal(&gotvec,&u,&mat);
    expect_vec(expectedvec,gotvec);
}

static void D3DXVector3Test(void)
{
    D3DVIEWPORT9 viewport;
    D3DXVECTOR3 expectedvec, gotvec, nul, u, v, w, x;
    LPD3DXVECTOR3 funcpointer;
    D3DXVECTOR4 expectedtrans, gottrans;
    D3DXMATRIX mat, projection, view, world;
    FLOAT coeff1, coeff2, expected, got, scale;

    nul.x = 0.0f; nul.y = 0.0f; nul.z = 0.0f;
    u.x = 9.0f; u.y = 6.0f; u.z = 2.0f;
    v.x = 2.0f; v.y = -3.0f; v.z = -4.0;
    w.x = 3.0f; w.y = -5.0f; w.z = 7.0f;
    x.x = 4.0f; x.y = 1.0f; x.z = 11.0f;

    viewport.Width = 800; viewport.MinZ = 0.2f; viewport.X = 10;
    viewport.Height = 680; viewport.MaxZ = 0.9f; viewport.Y = 5;

    U(mat).m[0][0] = 1.0f; U(mat).m[0][1] = 2.0f; U(mat).m[0][2] = 3.0f; U(mat).m[0][3] = 4.0f;
    U(mat).m[1][0] = 5.0f; U(mat).m[1][1] = 6.0f; U(mat).m[1][2] = 7.0f; U(mat).m[1][3] = 8.0f;
    U(mat).m[2][0] = 9.0f; U(mat).m[2][1] = 10.0f; U(mat).m[2][2] = 11.0f; U(mat).m[2][3] = 12.0f;
    U(mat).m[3][0] = 13.0f; U(mat).m[3][1] = 14.0f; U(mat).m[3][2] = 15.0f; U(mat).m[3][3] = 16.0f;

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

    coeff1 = 2.0f; coeff2 = 5.0f;
    scale = -6.5f;

/*_______________D3DXVec3Add__________________________*/
    expectedvec.x = 11.0f; expectedvec.y = 3.0f; expectedvec.z = -2.0f;
    D3DXVec3Add(&gotvec,&u,&v);
    expect_vec3(expectedvec,gotvec);
    /* Tests the case NULL */
    funcpointer = D3DXVec3Add(&gotvec,NULL,&v);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXVec3Add(NULL,NULL,NULL);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);

/*_______________D3DXVec3BaryCentric___________________*/
    expectedvec.x = -35.0f; expectedvec.y = -67.0; expectedvec.z = 15.0f;
    D3DXVec3BaryCentric(&gotvec,&u,&v,&w,coeff1,coeff2);

    expect_vec3(expectedvec,gotvec);

/*_______________D3DXVec3CatmullRom____________________*/
    expectedvec.x = 1458.0f; expectedvec.y = 22.1875f; expectedvec.z = 4141.375f;
    D3DXVec3CatmullRom(&gotvec,&u,&v,&w,&x,scale);
    expect_vec3(expectedvec,gotvec);

/*_______________D3DXVec3Cross________________________*/
    expectedvec.x = -18.0f; expectedvec.y = 40.0f; expectedvec.z = -39.0f;
    D3DXVec3Cross(&gotvec,&u,&v);
    expect_vec3(expectedvec,gotvec);
    /* Tests the case NULL */
    funcpointer = D3DXVec3Cross(&gotvec,NULL,&v);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXVec3Cross(NULL,NULL,NULL);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);

/*_______________D3DXVec3Dot__________________________*/
    expected = -8.0f;
    got = D3DXVec3Dot(&u,&v);
    ok(relative_error(got, expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);
    /* Tests the case NULL */
    expected=0.0f;
    got = D3DXVec3Dot(NULL,&v);
    ok(relative_error(got, expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);
    expected=0.0f;
    got = D3DXVec3Dot(NULL,NULL);
    ok(relative_error(got, expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);

/*_______________D3DXVec3Hermite__________________________*/
    expectedvec.x = -6045.75f; expectedvec.y = -6650.0f; expectedvec.z = 1358.875f;
    D3DXVec3Hermite(&gotvec,&u,&v,&w,&x,scale);
    expect_vec3(expectedvec,gotvec);

/*_______________D3DXVec3Length__________________________*/
    expected = 11.0f;
    got = D3DXVec3Length(&u);
    ok(relative_error(got, expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);
   /* Tests the case NULL */
    expected=0.0f;
    got = D3DXVec3Length(NULL);
    ok(relative_error(got, expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);

/*_______________D3DXVec3LengthSq________________________*/
    expected = 121.0f;
    got = D3DXVec3LengthSq(&u);
    ok(relative_error(got, expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);
   /* Tests the case NULL */
    expected=0.0f;
    got = D3DXVec3LengthSq(NULL);
    ok(relative_error(got, expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);

/*_______________D3DXVec3Lerp__________________________*/
    expectedvec.x = 54.5f; expectedvec.y = 64.5f, expectedvec.z = 41.0f ;
    D3DXVec3Lerp(&gotvec,&u,&v,scale);
    expect_vec3(expectedvec,gotvec);
    /* Tests the case NULL */
    funcpointer = D3DXVec3Lerp(&gotvec,NULL,&v,scale);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXVec3Lerp(NULL,NULL,NULL,scale);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);

/*_______________D3DXVec3Maximize__________________________*/
    expectedvec.x = 9.0f; expectedvec.y = 6.0f; expectedvec.z = 2.0f;
    D3DXVec3Maximize(&gotvec,&u,&v);
    expect_vec3(expectedvec,gotvec);
    /* Tests the case NULL */
    funcpointer = D3DXVec3Maximize(&gotvec,NULL,&v);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXVec3Maximize(NULL,NULL,NULL);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);

/*_______________D3DXVec3Minimize__________________________*/
    expectedvec.x = 2.0f; expectedvec.y = -3.0f; expectedvec.z = -4.0f;
    D3DXVec3Minimize(&gotvec,&u,&v);
    expect_vec3(expectedvec,gotvec);
    /* Tests the case NULL */
    funcpointer = D3DXVec3Minimize(&gotvec,NULL,&v);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXVec3Minimize(NULL,NULL,NULL);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);

/*_______________D3DXVec3Normalize_________________________*/
    expectedvec.x = 9.0f/11.0f; expectedvec.y = 6.0f/11.0f; expectedvec.z = 2.0f/11.0f;
    D3DXVec3Normalize(&gotvec,&u);
    expect_vec3(expectedvec,gotvec);
    /* Test the nul vector */
    expectedvec.x = 0.0f; expectedvec.y = 0.0f; expectedvec.z = 0.0f;
    D3DXVec3Normalize(&gotvec,&nul);
    expect_vec3(expectedvec,gotvec);

/*_______________D3DXVec3Project_________________________*/
    expectedvec.x = 1135.721924f; expectedvec.y = 147.086914f; expectedvec.z = 0.153412f;
    D3DXMatrixPerspectiveFovLH(&projection,D3DX_PI/4.0f,20.0f/17.0f,1.0f,1000.0f);
    D3DXVec3Project(&gotvec,&u,&viewport,&projection,&view,&world);
    expect_vec3(expectedvec,gotvec);

/*_______________D3DXVec3Scale____________________________*/
    expectedvec.x = -58.5f; expectedvec.y = -39.0f; expectedvec.z = -13.0f;
    D3DXVec3Scale(&gotvec,&u,scale);
    expect_vec3(expectedvec,gotvec);
    /* Tests the case NULL */
    funcpointer = D3DXVec3Scale(&gotvec,NULL,scale);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXVec3Scale(NULL,NULL,scale);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);

/*_______________D3DXVec3Subtract_______________________*/
    expectedvec.x = 7.0f; expectedvec.y = 9.0f; expectedvec.z = 6.0f;
    D3DXVec3Subtract(&gotvec,&u,&v);
    expect_vec3(expectedvec,gotvec);
    /* Tests the case NULL */
    funcpointer = D3DXVec3Subtract(&gotvec,NULL,&v);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXVec3Subtract(NULL,NULL,NULL);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);

/*_______________D3DXVec3Transform_______________________*/
    expectedtrans.x = 70.0f; expectedtrans.y = 88.0f; expectedtrans.z = 106.0f; expectedtrans.w = 124.0f;
    D3DXVec3Transform(&gottrans,&u,&mat);
    expect_vec4(expectedtrans,gottrans);

/*_______________D3DXVec3TransformCoord_______________________*/
    expectedvec.x = 70.0f/124.0f; expectedvec.y = 88.0f/124.0f; expectedvec.z = 106.0f/124.0f;
    D3DXVec3TransformCoord(&gotvec,&u,&mat);
    expect_vec3(expectedvec,gotvec);

/*_______________D3DXVec3TransformNormal______________________*/
    expectedvec.x = 57.0f; expectedvec.y = 74.0f; expectedvec.z = 91.0f;
    D3DXVec3TransformNormal(&gotvec,&u,&mat);
    expect_vec3(expectedvec,gotvec);

/*_______________D3DXVec3Unproject_________________________*/
    expectedvec.x = -2.913411f; expectedvec.y = 1.593215f; expectedvec.z = 0.380724f;
    D3DXMatrixPerspectiveFovLH(&projection,D3DX_PI/4.0f,20.0f/17.0f,1.0f,1000.0f);
    D3DXVec3Unproject(&gotvec,&u,&viewport,&projection,&view,&world);
    expect_vec3(expectedvec,gotvec);
    /* World matrix can be omitted */
    D3DXMatrixMultiply(&mat,&world,&view);
    D3DXVec3Unproject(&gotvec,&u,&viewport,&projection,&mat,NULL);
    expect_vec3(expectedvec,gotvec);
}

static void D3DXVector4Test(void)
{
    D3DXVECTOR4 expectedvec, gotvec, u, v, w, x;
    LPD3DXVECTOR4 funcpointer;
    D3DXVECTOR4 expectedtrans, gottrans;
    D3DXMATRIX mat;
    FLOAT coeff1, coeff2, expected, got, scale;

    u.x = 1.0f; u.y = 2.0f; u.z = 4.0f; u.w = 10.0;
    v.x = -3.0f; v.y = 4.0f; v.z = -5.0f; v.w = 7.0;
    w.x = 4.0f; w.y =6.0f; w.z = -2.0f; w.w = 1.0f;
    x.x = 6.0f; x.y = -7.0f; x.z =8.0f; x.w = -9.0f;

    U(mat).m[0][0] = 1.0f; U(mat).m[0][1] = 2.0f; U(mat).m[0][2] = 3.0f; U(mat).m[0][3] = 4.0f;
    U(mat).m[1][0] = 5.0f; U(mat).m[1][1] = 6.0f; U(mat).m[1][2] = 7.0f; U(mat).m[1][3] = 8.0f;
    U(mat).m[2][0] = 9.0f; U(mat).m[2][1] = 10.0f; U(mat).m[2][2] = 11.0f; U(mat).m[2][3] = 12.0f;
    U(mat).m[3][0] = 13.0f; U(mat).m[3][1] = 14.0f; U(mat).m[3][2] = 15.0f; U(mat).m[3][3] = 16.0f;

    coeff1 = 2.0f; coeff2 = 5.0;
    scale = -6.5f;

/*_______________D3DXVec4Add__________________________*/
    expectedvec.x = -2.0f; expectedvec.y = 6.0f; expectedvec.z = -1.0f; expectedvec.w = 17.0f;
    D3DXVec4Add(&gotvec,&u,&v);
    expect_vec4(expectedvec,gotvec);
    /* Tests the case NULL */
    funcpointer = D3DXVec4Add(&gotvec,NULL,&v);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXVec4Add(NULL,NULL,NULL);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);

/*_______________D3DXVec4BaryCentric____________________*/
    expectedvec.x = 8.0f; expectedvec.y = 26.0; expectedvec.z =  -44.0f; expectedvec.w = -41.0f;
    D3DXVec4BaryCentric(&gotvec,&u,&v,&w,coeff1,coeff2);
    expect_vec4(expectedvec,gotvec);

/*_______________D3DXVec4CatmullRom____________________*/
    expectedvec.x = 2754.625f; expectedvec.y = 2367.5625f; expectedvec.z = 1060.1875f; expectedvec.w = 131.3125f;
    D3DXVec4CatmullRom(&gotvec,&u,&v,&w,&x,scale);
    expect_vec4(expectedvec,gotvec);

/*_______________D3DXVec4Cross_________________________*/
    expectedvec.x = 390.0f; expectedvec.y = -393.0f; expectedvec.z = -316.0f; expectedvec.w = 166.0f;
    D3DXVec4Cross(&gotvec,&u,&v,&w);
    expect_vec4(expectedvec,gotvec);

/*_______________D3DXVec4Dot__________________________*/
    expected = 55.0f;
    got = D3DXVec4Dot(&u,&v);
    ok(relative_error(got, expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);
    /* Tests the case NULL */
    expected=0.0f;
    got = D3DXVec4Dot(NULL,&v);
    ok(relative_error(got, expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);
    expected=0.0f;
    got = D3DXVec4Dot(NULL,NULL);
    ok(relative_error(got, expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);

/*_______________D3DXVec4Hermite_________________________*/
    expectedvec.x = 1224.625f; expectedvec.y = 3461.625f; expectedvec.z = -4758.875f; expectedvec.w = -5781.5f;
    D3DXVec4Hermite(&gotvec,&u,&v,&w,&x,scale);
    expect_vec4(expectedvec,gotvec);

/*_______________D3DXVec4Length__________________________*/
   expected = 11.0f;
   got = D3DXVec4Length(&u);
   ok(relative_error(got, expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);
   /* Tests the case NULL */
    expected=0.0f;
    got = D3DXVec4Length(NULL);
    ok(relative_error(got, expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);

/*_______________D3DXVec4LengthSq________________________*/
    expected = 121.0f;
    got = D3DXVec4LengthSq(&u);
    ok(relative_error(got, expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);
    /* Tests the case NULL */
    expected=0.0f;
    got = D3DXVec4LengthSq(NULL);
    ok(relative_error(got, expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);

/*_______________D3DXVec4Lerp__________________________*/
    expectedvec.x = 27.0f; expectedvec.y = -11.0f; expectedvec.z = 62.5;  expectedvec.w = 29.5;
    D3DXVec4Lerp(&gotvec,&u,&v,scale);
    expect_vec4(expectedvec,gotvec);
    /* Tests the case NULL */
    funcpointer = D3DXVec4Lerp(&gotvec,NULL,&v,scale);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXVec4Lerp(NULL,NULL,NULL,scale);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);

/*_______________D3DXVec4Maximize__________________________*/
    expectedvec.x = 1.0f; expectedvec.y = 4.0f; expectedvec.z = 4.0f; expectedvec.w = 10.0;
    D3DXVec4Maximize(&gotvec,&u,&v);
    expect_vec4(expectedvec,gotvec);
    /* Tests the case NULL */
    funcpointer = D3DXVec4Maximize(&gotvec,NULL,&v);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXVec4Maximize(NULL,NULL,NULL);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);

/*_______________D3DXVec4Minimize__________________________*/
    expectedvec.x = -3.0f; expectedvec.y = 2.0f; expectedvec.z = -5.0f; expectedvec.w = 7.0;
    D3DXVec4Minimize(&gotvec,&u,&v);
    expect_vec4(expectedvec,gotvec);
    /* Tests the case NULL */
    funcpointer = D3DXVec4Minimize(&gotvec,NULL,&v);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXVec4Minimize(NULL,NULL,NULL);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);

/*_______________D3DXVec4Normalize_________________________*/
    expectedvec.x = 1.0f/11.0f; expectedvec.y = 2.0f/11.0f; expectedvec.z = 4.0f/11.0f; expectedvec.w = 10.0f/11.0f;
    D3DXVec4Normalize(&gotvec,&u);
    expect_vec4(expectedvec,gotvec);

/*_______________D3DXVec4Scale____________________________*/
    expectedvec.x = -6.5f; expectedvec.y = -13.0f; expectedvec.z = -26.0f; expectedvec.w = -65.0f;
    D3DXVec4Scale(&gotvec,&u,scale);
    expect_vec4(expectedvec,gotvec);
    /* Tests the case NULL */
    funcpointer = D3DXVec4Scale(&gotvec,NULL,scale);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXVec4Scale(NULL,NULL,scale);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);

/*_______________D3DXVec4Subtract__________________________*/
    expectedvec.x = 4.0f; expectedvec.y = -2.0f; expectedvec.z = 9.0f; expectedvec.w = 3.0f;
    D3DXVec4Subtract(&gotvec,&u,&v);
    expect_vec4(expectedvec,gotvec);
    /* Tests the case NULL */
    funcpointer = D3DXVec4Subtract(&gotvec,NULL,&v);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXVec4Subtract(NULL,NULL,NULL);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);

/*_______________D3DXVec4Transform_______________________*/
    expectedtrans.x = 177.0f; expectedtrans.y = 194.0f; expectedtrans.z = 211.0f; expectedtrans.w = 228.0f;
    D3DXVec4Transform(&gottrans,&u,&mat);
    expect_vec4(expectedtrans,gottrans);
}

static void test_matrix_stack(void)
{
    ID3DXMatrixStack *stack;
    ULONG refcount;
    HRESULT hr;

    const D3DXMATRIX mat1 = {{{
         1.0f,  2.0f,  3.0f,  4.0f,
         5.0f,  6.0f,  7.0f,  8.0f,
         9.0f, 10.0f, 11.0f, 12.0f,
        13.0f, 14.0f, 15.0f, 16.0f
    }}};

    const D3DXMATRIX mat2 = {{{
        17.0f, 18.0f, 19.0f, 20.0f,
        21.0f, 22.0f, 23.0f, 24.0f,
        25.0f, 26.0f, 27.0f, 28.0f,
        29.0f, 30.0f, 31.0f, 32.0f
    }}};

    hr = D3DXCreateMatrixStack(0, &stack);
    ok(SUCCEEDED(hr), "Failed to create a matrix stack, hr %#x\n", hr);
    if (FAILED(hr)) return;

    ok(D3DXMatrixIsIdentity(ID3DXMatrixStack_GetTop(stack)),
            "The top of an empty matrix stack should be an identity matrix\n");

    hr = ID3DXMatrixStack_Pop(stack);
    ok(SUCCEEDED(hr), "Pop failed, hr %#x\n", hr);

    hr = ID3DXMatrixStack_Push(stack);
    ok(SUCCEEDED(hr), "Push failed, hr %#x\n", hr);
    ok(D3DXMatrixIsIdentity(ID3DXMatrixStack_GetTop(stack)), "The top should be an identity matrix\n");

    hr = ID3DXMatrixStack_Push(stack);
    ok(SUCCEEDED(hr), "Push failed, hr %#x\n", hr);

    hr = ID3DXMatrixStack_LoadMatrix(stack, &mat1);
    ok(SUCCEEDED(hr), "LoadMatrix failed, hr %#x\n", hr);
    expect_mat(&mat1, ID3DXMatrixStack_GetTop(stack));

    hr = ID3DXMatrixStack_Push(stack);
    ok(SUCCEEDED(hr), "Push failed, hr %#x\n", hr);
    expect_mat(&mat1, ID3DXMatrixStack_GetTop(stack));

    hr = ID3DXMatrixStack_LoadMatrix(stack, &mat2);
    ok(SUCCEEDED(hr), "LoadMatrix failed, hr %#x\n", hr);
    expect_mat(&mat2, ID3DXMatrixStack_GetTop(stack));

    hr = ID3DXMatrixStack_Push(stack);
    ok(SUCCEEDED(hr), "Push failed, hr %#x\n", hr);
    expect_mat(&mat2, ID3DXMatrixStack_GetTop(stack));

    hr = ID3DXMatrixStack_LoadIdentity(stack);
    ok(SUCCEEDED(hr), "LoadIdentity failed, hr %#x\n", hr);
    ok(D3DXMatrixIsIdentity(ID3DXMatrixStack_GetTop(stack)), "The top should be an identity matrix\n");

    hr = ID3DXMatrixStack_Pop(stack);
    ok(SUCCEEDED(hr), "Pop failed, hr %#x\n", hr);
    expect_mat(&mat2, ID3DXMatrixStack_GetTop(stack));

    hr = ID3DXMatrixStack_Pop(stack);
    ok(SUCCEEDED(hr), "Pop failed, hr %#x\n", hr);
    expect_mat(&mat1, ID3DXMatrixStack_GetTop(stack));

    hr = ID3DXMatrixStack_Pop(stack);
    ok(SUCCEEDED(hr), "Pop failed, hr %#x\n", hr);
    ok(D3DXMatrixIsIdentity(ID3DXMatrixStack_GetTop(stack)), "The top should be an identity matrix\n");

    hr = ID3DXMatrixStack_Pop(stack);
    ok(SUCCEEDED(hr), "Pop failed, hr %#x\n", hr);
    ok(D3DXMatrixIsIdentity(ID3DXMatrixStack_GetTop(stack)), "The top should be an identity matrix\n");

    refcount = ID3DXMatrixStack_Release(stack);
    ok(!refcount, "Matrix stack has %u references left.\n", refcount);
}

static void test_Matrix_AffineTransformation2D(void)
{
    D3DXMATRIX exp_mat, got_mat;
    D3DXVECTOR2 center, position;
    FLOAT angle, scale;

    center.x = 3.0f;
    center.y = 4.0f;

    position.x = -6.0f;
    position.y = 7.0f;

    angle = D3DX_PI/3.0f;

    scale = 20.0f;

    U(exp_mat).m[0][0] = 10.0f;
    U(exp_mat).m[1][0] = -17.320507f;
    U(exp_mat).m[2][0] = 0.0f;
    U(exp_mat).m[3][0] = -1.035898f;
    U(exp_mat).m[0][1] = 17.320507f;
    U(exp_mat).m[1][1] = 10.0f;
    U(exp_mat).m[2][1] = 0.0f;
    U(exp_mat).m[3][1] = 6.401924f;
    U(exp_mat).m[0][2] = 0.0f;
    U(exp_mat).m[1][2] = 0.0f;
    U(exp_mat).m[2][2] = 1.0f;
    U(exp_mat).m[3][2] = 0.0f;
    U(exp_mat).m[0][3] = 0.0f;
    U(exp_mat).m[1][3] = 0.0f;
    U(exp_mat).m[2][3] = 0.0f;
    U(exp_mat).m[3][3] = 1.0f;

    D3DXMatrixAffineTransformation2D(&got_mat, scale, &center, angle, &position);

    expect_mat(&exp_mat, &got_mat);

/*______________*/

    center.x = 3.0f;
    center.y = 4.0f;

    angle = D3DX_PI/3.0f;

    scale = 20.0f;

    U(exp_mat).m[0][0] = 10.0f;
    U(exp_mat).m[1][0] = -17.320507f;
    U(exp_mat).m[2][0] = 0.0f;
    U(exp_mat).m[3][0] = 4.964102f;
    U(exp_mat).m[0][1] = 17.320507f;
    U(exp_mat).m[1][1] = 10.0f;
    U(exp_mat).m[2][1] = 0.0f;
    U(exp_mat).m[3][1] = -0.598076f;
    U(exp_mat).m[0][2] = 0.0f;
    U(exp_mat).m[1][2] = 0.0f;
    U(exp_mat).m[2][2] = 1.0f;
    U(exp_mat).m[3][2] = 0.0f;
    U(exp_mat).m[0][3] = 0.0f;
    U(exp_mat).m[1][3] = 0.0f;
    U(exp_mat).m[2][3] = 0.0f;
    U(exp_mat).m[3][3] = 1.0f;

    D3DXMatrixAffineTransformation2D(&got_mat, scale, &center, angle, NULL);

    expect_mat(&exp_mat, &got_mat);

/*______________*/

    position.x = -6.0f;
    position.y = 7.0f;

    angle = D3DX_PI/3.0f;

    scale = 20.0f;

    U(exp_mat).m[0][0] = 10.0f;
    U(exp_mat).m[1][0] = -17.320507f;
    U(exp_mat).m[2][0] = 0.0f;
    U(exp_mat).m[3][0] = -6.0f;
    U(exp_mat).m[0][1] = 17.320507f;
    U(exp_mat).m[1][1] = 10.0f;
    U(exp_mat).m[2][1] = 0.0f;
    U(exp_mat).m[3][1] = 7.0f;
    U(exp_mat).m[0][2] = 0.0f;
    U(exp_mat).m[1][2] = 0.0f;
    U(exp_mat).m[2][2] = 1.0f;
    U(exp_mat).m[3][2] = 0.0f;
    U(exp_mat).m[0][3] = 0.0f;
    U(exp_mat).m[1][3] = 0.0f;
    U(exp_mat).m[2][3] = 0.0f;
    U(exp_mat).m[3][3] = 1.0f;

    D3DXMatrixAffineTransformation2D(&got_mat, scale, NULL, angle, &position);

    expect_mat(&exp_mat, &got_mat);

/*______________*/

    angle = 5.0f * D3DX_PI/4.0f;

    scale = -20.0f;

    U(exp_mat).m[0][0] = 14.142133f;
    U(exp_mat).m[1][0] = -14.142133f;
    U(exp_mat).m[2][0] = 0.0f;
    U(exp_mat).m[3][0] = 0.0f;
    U(exp_mat).m[0][1] = 14.142133;
    U(exp_mat).m[1][1] = 14.142133f;
    U(exp_mat).m[2][1] = 0.0f;
    U(exp_mat).m[3][1] = 0.0f;
    U(exp_mat).m[0][2] = 0.0f;
    U(exp_mat).m[1][2] = 0.0f;
    U(exp_mat).m[2][2] = 1.0f;
    U(exp_mat).m[3][2] = 0.0f;
    U(exp_mat).m[0][3] = 0.0f;
    U(exp_mat).m[1][3] = 0.0f;
    U(exp_mat).m[2][3] = 0.0f;
    U(exp_mat).m[3][3] = 1.0f;

    D3DXMatrixAffineTransformation2D(&got_mat, scale, NULL, angle, NULL);

    expect_mat(&exp_mat, &got_mat);
}

static void test_Matrix_Decompose(void)
{
    D3DXMATRIX pm;
    D3DXQUATERNION exp_rotation, got_rotation;
    D3DXVECTOR3 exp_scale, got_scale, exp_translation, got_translation;
    HRESULT hr;

/*___________*/

    U(pm).m[0][0] = -0.9238790f;
    U(pm).m[1][0] = -0.2705984f;
    U(pm).m[2][0] = 0.2705984f;
    U(pm).m[3][0] = -5.0f;
    U(pm).m[0][1] = 0.2705984f;
    U(pm).m[1][1] = 0.03806049f;
    U(pm).m[2][1] = 0.9619395f;
    U(pm).m[3][1] = 0.0f;
    U(pm).m[0][2] = -0.2705984f;
    U(pm).m[1][2] = 0.9619395f;
    U(pm).m[2][2] = 0.03806049f;
    U(pm).m[3][2] = 10.0f;
    U(pm).m[0][3] = 0.0f;
    U(pm).m[1][3] = 0.0f;
    U(pm).m[2][3] = 0.0f;
    U(pm).m[3][3] = 1.0f;

    exp_scale.x = 1.0f;
    exp_scale.y = 1.0f;
    exp_scale.z = 1.0f;

    exp_rotation.w = 0.195091f;
    exp_rotation.x = 0.0f;
    exp_rotation.y = 0.693520f;
    exp_rotation.z = 0.693520f;

    exp_translation.x = -5.0f;
    exp_translation.y = 0.0f;
    exp_translation.z = 10.0f;

    D3DXMatrixDecompose(&got_scale, &got_rotation, &got_translation, &pm);

    compare_scale(exp_scale, got_scale);
    compare_rotation(exp_rotation, got_rotation);
    compare_translation(exp_translation, got_translation);

/*_________*/

    U(pm).m[0][0] = -2.255813f;
    U(pm).m[1][0] = 1.302324f;
    U(pm).m[2][0] = 1.488373f;
    U(pm).m[3][0] = 1.0f;
    U(pm).m[0][1] = 1.302327f;
    U(pm).m[1][1] = -0.7209296f;
    U(pm).m[2][1] = 2.60465f;
    U(pm).m[3][1] = 2.0f;
    U(pm).m[0][2] = 1.488371f;
    U(pm).m[1][2] = 2.604651f;
    U(pm).m[2][2] = -0.02325551f;
    U(pm).m[3][2] = 3.0f;
    U(pm).m[0][3] = 0.0f;
    U(pm).m[1][3] = 0.0f;
    U(pm).m[2][3] = 0.0f;
    U(pm).m[3][3] = 1.0f;

    exp_scale.x = 3.0f;
    exp_scale.y = 3.0f;
    exp_scale.z = 3.0f;

    exp_rotation.w = 0.0;
    exp_rotation.x = 0.352180f;
    exp_rotation.y = 0.616316f;
    exp_rotation.z = 0.704361f;

    exp_translation.x = 1.0f;
    exp_translation.y = 2.0f;
    exp_translation.z = 3.0f;

    D3DXMatrixDecompose(&got_scale, &got_rotation, &got_translation, &pm);

    compare_scale(exp_scale, got_scale);
    compare_rotation(exp_rotation, got_rotation);
    compare_translation(exp_translation, got_translation);

/*_____________*/

    U(pm).m[0][0] = 2.427051f;
    U(pm).m[1][0] = 0.0f;
    U(pm).m[2][0] = 1.763355f;
    U(pm).m[3][0] = 5.0f;
    U(pm).m[0][1] = 0.0f;
    U(pm).m[1][1] = 3.0f;
    U(pm).m[2][1] = 0.0f;
    U(pm).m[3][1] = 5.0f;
    U(pm).m[0][2] = -1.763355f;
    U(pm).m[1][2] = 0.0f;
    U(pm).m[2][2] = 2.427051f;
    U(pm).m[3][2] = 5.0f;
    U(pm).m[0][3] = 0.0f;
    U(pm).m[1][3] = 0.0f;
    U(pm).m[2][3] = 0.0f;
    U(pm).m[3][3] = 1.0f;

    exp_scale.x = 3.0f;
    exp_scale.y = 3.0f;
    exp_scale.z = 3.0f;

    exp_rotation.w = 0.951057f;
    exp_rotation.x = 0.0f;
    exp_rotation.y = 0.309017f;
    exp_rotation.z = 0.0f;

    exp_translation.x = 5.0f;
    exp_translation.y = 5.0f;
    exp_translation.z = 5.0f;

    D3DXMatrixDecompose(&got_scale, &got_rotation, &got_translation, &pm);

    compare_scale(exp_scale, got_scale);
    compare_rotation(exp_rotation, got_rotation);
    compare_translation(exp_translation, got_translation);

/*_____________*/

    U(pm).m[0][0] = -0.9238790f;
    U(pm).m[1][0] = -0.2705984f;
    U(pm).m[2][0] = 0.2705984f;
    U(pm).m[3][0] = -5.0f;
    U(pm).m[0][1] = 0.2705984f;
    U(pm).m[1][1] = 0.03806049f;
    U(pm).m[2][1] = 0.9619395f;
    U(pm).m[3][1] = 0.0f;
    U(pm).m[0][2] = -0.2705984f;
    U(pm).m[1][2] = 0.9619395f;
    U(pm).m[2][2] = 0.03806049f;
    U(pm).m[3][2] = 10.0f;
    U(pm).m[0][3] = 0.0f;
    U(pm).m[1][3] = 0.0f;
    U(pm).m[2][3] = 0.0f;
    U(pm).m[3][3] = 1.0f;

    exp_scale.x = 1.0f;
    exp_scale.y = 1.0f;
    exp_scale.z = 1.0f;

    exp_rotation.w = 0.195091f;
    exp_rotation.x = 0.0f;
    exp_rotation.y = 0.693520f;
    exp_rotation.z = 0.693520f;

    exp_translation.x = -5.0f;
    exp_translation.y = 0.0f;
    exp_translation.z = 10.0f;

    D3DXMatrixDecompose(&got_scale, &got_rotation, &got_translation, &pm);

    compare_scale(exp_scale, got_scale);
    compare_rotation(exp_rotation, got_rotation);
    compare_translation(exp_translation, got_translation);

/*__________*/

    U(pm).m[0][0] = -0.9238790f;
    U(pm).m[1][0] = -0.5411968f;
    U(pm).m[2][0] = 0.8117952f;
    U(pm).m[3][0] = -5.0f;
    U(pm).m[0][1] = 0.2705984f;
    U(pm).m[1][1] = 0.07612098f;
    U(pm).m[2][1] = 2.8858185f;
    U(pm).m[3][1] = 0.0f;
    U(pm).m[0][2] = -0.2705984f;
    U(pm).m[1][2] = 1.9238790f;
    U(pm).m[2][2] = 0.11418147f;
    U(pm).m[3][2] = 10.0f;
    U(pm).m[0][3] = 0.0f;
    U(pm).m[1][3] = 0.0f;
    U(pm).m[2][3] = 0.0f;
    U(pm).m[3][3] = 1.0f;

    exp_scale.x = 1.0f;
    exp_scale.y = 2.0f;
    exp_scale.z = 3.0f;

    exp_rotation.w = 0.195091f;
    exp_rotation.x = 0.0f;
    exp_rotation.y = 0.693520f;
    exp_rotation.z = 0.693520f;

    exp_translation.x = -5.0f;
    exp_translation.y = 0.0f;
    exp_translation.z = 10.0f;

    D3DXMatrixDecompose(&got_scale, &got_rotation, &got_translation, &pm);

    compare_scale(exp_scale, got_scale);
    compare_rotation(exp_rotation, got_rotation);
    compare_translation(exp_translation, got_translation);

/*__________*/

    U(pm).m[0][0] = 0.7156004f;
    U(pm).m[1][0] = -0.5098283f;
    U(pm).m[2][0] = -0.4774843f;
    U(pm).m[3][0] = -5.0f;
    U(pm).m[0][1] = -0.6612288f;
    U(pm).m[1][1] = -0.7147621f;
    U(pm).m[2][1] = -0.2277977f;
    U(pm).m[3][1] = 0.0f;
    U(pm).m[0][2] = -0.2251499f;
    U(pm).m[1][2] = 0.4787385f;
    U(pm).m[2][2] = -0.8485972f;
    U(pm).m[3][2] = 10.0f;
    U(pm).m[0][3] = 0.0f;
    U(pm).m[1][3] = 0.0f;
    U(pm).m[2][3] = 0.0f;
    U(pm).m[3][3] = 1.0f;

    exp_scale.x = 1.0f;
    exp_scale.y = 1.0f;
    exp_scale.z = 1.0f;

    exp_rotation.w = 0.195091f;
    exp_rotation.x = 0.905395f;
    exp_rotation.y = -0.323355f;
    exp_rotation.z = -0.194013f;

    exp_translation.x = -5.0f;
    exp_translation.y = 0.0f;
    exp_translation.z = 10.0f;

    D3DXMatrixDecompose(&got_scale, &got_rotation, &got_translation, &pm);

    compare_scale(exp_scale, got_scale);
    compare_rotation(exp_rotation, got_rotation);
    compare_translation(exp_translation, got_translation);

/*_____________*/

    U(pm).m[0][0] = 0.06554436f;
    U(pm).m[1][0] = -0.6873012f;
    U(pm).m[2][0] = 0.7234092f;
    U(pm).m[3][0] = -5.0f;
    U(pm).m[0][1] = -0.9617381f;
    U(pm).m[1][1] = -0.2367795f;
    U(pm).m[2][1] = -0.1378230f;
    U(pm).m[3][1] = 0.0f;
    U(pm).m[0][2] = 0.2660144f;
    U(pm).m[1][2] = -0.6866967f;
    U(pm).m[2][2] = -0.6765233f;
    U(pm).m[3][2] = 10.0f;
    U(pm).m[0][3] = 0.0f;
    U(pm).m[1][3] = 0.0f;
    U(pm).m[2][3] = 0.0f;
    U(pm).m[3][3] = 1.0f;

    exp_scale.x = 1.0f;
    exp_scale.y = 1.0f;
    exp_scale.z = 1.0f;

    exp_rotation.w = -0.195091f;
    exp_rotation.x = 0.703358f;
    exp_rotation.y = -0.586131f;
    exp_rotation.z = 0.351679f;

    exp_translation.x = -5.0f;
    exp_translation.y = 0.0f;
    exp_translation.z = 10.0f;

    D3DXMatrixDecompose(&got_scale, &got_rotation, &got_translation, &pm);

    compare_scale(exp_scale, got_scale);
    compare_rotation(exp_rotation, got_rotation);
    compare_translation(exp_translation, got_translation);

/*_________*/

    U(pm).m[0][0] = 7.121047f;
    U(pm).m[1][0] = -5.883487f;
    U(pm).m[2][0] = 11.81843f;
    U(pm).m[3][0] = -5.0f;
    U(pm).m[0][1] = 5.883487f;
    U(pm).m[1][1] = -10.60660f;
    U(pm).m[2][1] = -8.825232f;
    U(pm).m[3][1] = 0.0f;
    U(pm).m[0][2] = 11.81843f;
    U(pm).m[1][2] = 8.8252320f;
    U(pm).m[2][2] = -2.727645f;
    U(pm).m[3][2] = 2.0f;
    U(pm).m[0][3] = 0.0f;
    U(pm).m[1][3] = 0.0f;
    U(pm).m[2][3] = 0.0f;
    U(pm).m[3][3] = 1.0f;

    exp_scale.x = 15.0f;
    exp_scale.y = 15.0f;
    exp_scale.z = 15.0f;

    exp_rotation.w = 0.382684f;
    exp_rotation.x = 0.768714f;
    exp_rotation.y = 0.0f;
    exp_rotation.z = 0.512476f;

    exp_translation.x = -5.0f;
    exp_translation.y = 0.0f;
    exp_translation.z = 2.0f;

    D3DXMatrixDecompose(&got_scale, &got_rotation, &got_translation, &pm);

    compare_scale(exp_scale, got_scale);
    compare_rotation(exp_rotation, got_rotation);
    compare_translation(exp_translation, got_translation);

/*__________*/

    U(pm).m[0][0] = 0.0f;
    U(pm).m[1][0] = 4.0f;
    U(pm).m[2][0] = 5.0f;
    U(pm).m[3][0] = -5.0f;
    U(pm).m[0][1] = 0.0f;
    U(pm).m[1][1] = -10.60660f;
    U(pm).m[2][1] = -8.825232f;
    U(pm).m[3][1] = 6.0f;
    U(pm).m[0][2] = 0.0f;
    U(pm).m[1][2] = 8.8252320f;
    U(pm).m[2][2] = 2.727645;
    U(pm).m[3][2] = 3.0f;
    U(pm).m[0][3] = 0.0f;
    U(pm).m[1][3] = 0.0f;
    U(pm).m[2][3] = 0.0f;
    U(pm).m[3][3] = 1.0f;

    hr = D3DXMatrixDecompose(&got_scale, &got_rotation, &got_translation, &pm);
    ok(hr == D3DERR_INVALIDCALL, "Expected D3DERR_INVALIDCALL, got %x\n", hr);
}

static void test_Matrix_Transformation2D(void)
{
    D3DXMATRIX exp_mat, got_mat;
    D3DXVECTOR2 rot_center, sca, sca_center, trans;
    FLOAT rot, sca_rot;

    rot_center.x = 3.0f;
    rot_center.y = 4.0f;

    sca.x = 12.0f;
    sca.y = -3.0f;

    sca_center.x = 9.0f;
    sca_center.y = -5.0f;

    trans.x = -6.0f;
    trans.y = 7.0f;

    rot = D3DX_PI/3.0f;

    sca_rot = 5.0f*D3DX_PI/4.0f;

    U(exp_mat).m[0][0] = -4.245192f;
    U(exp_mat).m[1][0] = -0.147116f;
    U(exp_mat).m[2][0] = 0.0f;
    U(exp_mat).m[3][0] = 45.265373f;
    U(exp_mat).m[0][1] = 7.647113f;
    U(exp_mat).m[1][1] = 8.745192f;
    U(exp_mat).m[2][1] = 0.0f;
    U(exp_mat).m[3][1] = -13.401899f;
    U(exp_mat).m[0][2] = 0.0f;
    U(exp_mat).m[1][2] = 0.0f;
    U(exp_mat).m[2][2] = 1.0f;
    U(exp_mat).m[3][2] = 0.0f;
    U(exp_mat).m[0][3] = 0.0f;
    U(exp_mat).m[1][3] = 0.0f;
    U(exp_mat).m[2][3] = 0.0f;
    U(exp_mat).m[3][3] = 1.0f;

    D3DXMatrixTransformation2D(&got_mat, &sca_center, sca_rot, &sca, &rot_center, rot, &trans);

    expect_mat(&exp_mat, &got_mat);

/*_________*/

    sca_center.x = 9.0f;
    sca_center.y = -5.0f;

    trans.x = -6.0f;
    trans.y = 7.0f;

    rot = D3DX_PI/3.0f;

    sca_rot = 5.0f*D3DX_PI/4.0f;

    U(exp_mat).m[0][0] = 0.50f;
    U(exp_mat).m[1][0] = -0.866025f;
    U(exp_mat).m[2][0] = 0.0f;
    U(exp_mat).m[3][0] = -6.0f;
    U(exp_mat).m[0][1] = 0.866025f;
    U(exp_mat).m[1][1] = 0.50f;
    U(exp_mat).m[2][1] = 0.0f;
    U(exp_mat).m[3][1] = 7.0f;
    U(exp_mat).m[0][2] = 0.0f;
    U(exp_mat).m[1][2] = 0.0f;
    U(exp_mat).m[2][2] = 1.0f;
    U(exp_mat).m[3][2] = 0.0f;
    U(exp_mat).m[0][3] = 0.0f;
    U(exp_mat).m[1][3] = 0.0f;
    U(exp_mat).m[2][3] = 0.0f;
    U(exp_mat).m[3][3] = 1.0f;

    D3DXMatrixTransformation2D(&got_mat, &sca_center, sca_rot, NULL, NULL, rot, &trans);

    expect_mat(&exp_mat, &got_mat);

/*_________*/

    U(exp_mat).m[0][0] = 0.50f;
    U(exp_mat).m[1][0] = -0.866025f;
    U(exp_mat).m[2][0] = 0.0f;
    U(exp_mat).m[3][0] = 0.0f;
    U(exp_mat).m[0][1] = 0.866025f;
    U(exp_mat).m[1][1] = 0.50f;
    U(exp_mat).m[2][1] = 0.0f;
    U(exp_mat).m[3][1] = 0.0f;
    U(exp_mat).m[0][2] = 0.0f;
    U(exp_mat).m[1][2] = 0.0f;
    U(exp_mat).m[2][2] = 1.0f;
    U(exp_mat).m[3][2] = 0.0f;
    U(exp_mat).m[0][3] = 0.0f;
    U(exp_mat).m[1][3] = 0.0f;
    U(exp_mat).m[2][3] = 0.0f;
    U(exp_mat).m[3][3] = 1.0f;

    D3DXMatrixTransformation2D(&got_mat, NULL, sca_rot, NULL, NULL, rot, NULL);

    expect_mat(&exp_mat, &got_mat);
}

static void test_D3DXVec_Array(void)
{
    unsigned int i;
    D3DVIEWPORT9 viewport;
    D3DXMATRIX mat, projection, view, world;
    D3DXVECTOR4 inp_vec[ARRAY_SIZE];
    D3DXVECTOR4 out_vec[ARRAY_SIZE + 2];
    D3DXVECTOR4 exp_vec[ARRAY_SIZE + 2];
    D3DXPLANE inp_plane[ARRAY_SIZE];
    D3DXPLANE out_plane[ARRAY_SIZE + 2];
    D3DXPLANE exp_plane[ARRAY_SIZE + 2];

    viewport.Width = 800; viewport.MinZ = 0.2f; viewport.X = 10;
    viewport.Height = 680; viewport.MaxZ = 0.9f; viewport.Y = 5;

    for (i = 0; i < ARRAY_SIZE + 2; ++i) {
        out_vec[i].x = out_vec[i].y = out_vec[i].z = out_vec[i].w = 0.0f;
        exp_vec[i].x = exp_vec[i].y = exp_vec[i].z = exp_vec[i].w = 0.0f;
        out_plane[i].a = out_plane[i].b = out_plane[i].c = out_plane[i].d = 0.0f;
        exp_plane[i].a = exp_plane[i].b = exp_plane[i].c = exp_plane[i].d = 0.0f;
    }

    for (i = 0; i < ARRAY_SIZE; ++i) {
        inp_plane[i].a = inp_plane[i].c = inp_vec[i].x = inp_vec[i].z = i;
        inp_plane[i].b = inp_plane[i].d = inp_vec[i].y = inp_vec[i].w = ARRAY_SIZE - i;
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

    /* D3DXPlaneTransformArray */
    exp_plane[1].a = 90.0f; exp_plane[1].b = 100.0f; exp_plane[1].c = 110.0f; exp_plane[1].d = 120.0f;
    exp_plane[2].a = 82.0f; exp_plane[2].b = 92.0f;  exp_plane[2].c = 102.0f; exp_plane[2].d = 112.0f;
    exp_plane[3].a = 74.0f; exp_plane[3].b = 84.0f;  exp_plane[3].c = 94.0f;  exp_plane[3].d = 104.0f;
    exp_plane[4].a = 66.0f; exp_plane[4].b = 76.0f;  exp_plane[4].c = 86.0f;  exp_plane[4].d = 96.0f;
    exp_plane[5].a = 58.0f; exp_plane[5].b = 68.0f;  exp_plane[5].c = 78.0f;  exp_plane[5].d = 88.0f;
    D3DXPlaneTransformArray(out_plane + 1, sizeof(D3DXPLANE), inp_plane, sizeof(D3DXPLANE), &mat, ARRAY_SIZE);
    compare_planes(exp_plane, out_plane);
}

static void test_D3DXFloat_Array(void)
{
    static const float z = 0.0f;
    /* Compilers set different sign bits on 0.0 / 0.0, pick the right ones for NaN and -NaN */
    float tmpnan = 0.0f/z;
    float nnan = copysignf(1, tmpnan) < 0.0f ? tmpnan : -tmpnan;
    float nan = -nnan;
    unsigned int i;
    void *out = NULL;
    D3DXFLOAT16 half;
    FLOAT single;
    struct
    {
        FLOAT single_in;

        /* half_ver2 occurs on WXPPROSP3 (32 bit math), WVISTAADM (32 bit math), W7PRO (32 bit math) */
        WORD half_ver1, half_ver2;

        /* single_out_ver2 confirms that half -> single conversion is consistent across platforms */
        FLOAT single_out_ver1, single_out_ver2;
    } testdata[] = {
        { 80000.0f, 0x7c00, 0x7ce2, 65536.0f, 80000.0f },
        { 65503.0f, 0x7bff, 0x7bff, 65504.0f, 65504.0f },
        { 65504.0f, 0x7bff, 0x7bff, 65504.0f, 65504.0f },
        { 65520.0f, 0x7bff, 0x7c00, 65504.0f, 65536.0f },
        { 65521.0f, 0x7c00, 0x7c00, 65536.0f, 65536.0f },
        { 65534.0f, 0x7c00, 0x7c00, 65536.0f, 65536.0f },
        { 65535.0f, 0x7c00, 0x7c00, 65535.0f, 65536.0f },
        { 65536.0f, 0x7c00, 0x7c00, 65536.0f, 65536.0f },
        { -80000.0f, 0xfc00, 0xfce2, -65536.0f, -80000.0f },
        { -65503.0f, 0xfbff, 0xfbff, -65504.0f, -65504.0f },
        { -65504.0f, 0xfbff, 0xfbff, -65504.0f, -65504.0f },
        { -65520.0f, 0xfbff, 0xfc00, -65504.0f, -65536.0f },
        { -65521.0f, 0xfc00, 0xfc00, -65536.0f, -65536.0f },
        { -65534.0f, 0xfc00, 0xfc00, -65536.0f, -65536.0f },
        { -65535.0f, 0xfc00, 0xfc00, -65535.0f, -65536.0f },
        { -65536.0f, 0xfc00, 0xfc00, -65536.0f, -65536.0f },
        { 1.0f/z, 0x7c00, 0x7fff, 65536.0f, 131008.0f },
        { -1.0f/z, 0xffff, 0xffff, -131008.0f, -131008.0f },
        { nan, 0x7fff, 0x7fff, 131008.0f, 131008.0f },
        { nnan, 0xffff, 0xffff, -131008.0f, -131008.0f },
        { 0.0f, 0x0, 0x0, 0.0f, 0.0f },
        { -0.0f, 0x8000, 0x8000, 0.0f, 0.0f },
        { 2.9809595e-08f, 0x0, 0x0, 0.0f, 0.0f },
        { -2.9809595e-08f, 0x8000, 0x8000, -0.0f, -0.0f },
        { 2.9809598e-08f, 0x1, 0x1, 5.96046e-08f, 5.96046e-08f },
        { -2.9809598e-08f, 0x8001, 0x8001, -5.96046e-08f, -5.96046e-08f },
        { 8.9406967e-08f, 0x2, 0x2, 1.19209e-07f, 1.19209e-07f }
    };

    /* exception on NULL out or in parameter */
    out = D3DXFloat32To16Array(&half, &single, 0);
    ok(out == &half, "Got %p, expected %p.\n", out, &half);

    out = D3DXFloat16To32Array(&single, &half, 0);
    ok(out == &single, "Got %p, expected %p.\n", out, &single);

    for (i = 0; i < sizeof(testdata)/sizeof(testdata[0]); i++)
    {
        out = D3DXFloat32To16Array(&half, &testdata[i].single_in, 1);
        ok(out == &half, "Got %p, expected %p.\n", out, &half);
        ok(half.value == testdata[i].half_ver1 || half.value == testdata[i].half_ver2,
           "Got %x, expected %x or %x for index %d.\n", half.value, testdata[i].half_ver1,
           testdata[i].half_ver2, i);

        out = D3DXFloat16To32Array(&single, (D3DXFLOAT16 *)&testdata[i].half_ver1, 1);
        ok(out == &single, "Got %p, expected %p.\n", out, &single);
        ok(relative_error(single, testdata[i].single_out_ver1) < admitted_error,
           "Got %g, expected %g for index %d.\n", single, testdata[i].single_out_ver1, i);

        out = D3DXFloat16To32Array(&single, (D3DXFLOAT16 *)&testdata[i].half_ver2, 1);
        ok(out == &single, "Got %p, expected %p.\n", out, &single);
        ok(relative_error(single, testdata[i].single_out_ver2) < admitted_error,
           "Got %g, expected %g for index %d.\n", single, testdata[i].single_out_ver2, i);
    }
}

static void test_D3DXSHAdd(void)
{
    UINT i, k;
    FLOAT *ret = (FLOAT *)0xdeadbeef;
    const FLOAT in1[50] =
    {
        1.11f, 1.12f, 1.13f, 1.14f, 1.15f, 1.16f, 1.17f, 1.18f,
        1.19f, 1.20f, 1.21f, 1.22f, 1.23f, 1.24f, 1.25f, 1.26f,
        1.27f, 1.28f, 1.29f, 1.30f, 1.31f, 1.32f, 1.33f, 1.34f,
        1.35f, 1.36f, 1.37f, 1.38f, 1.39f, 1.40f, 1.41f, 1.42f,
        1.43f, 1.44f, 1.45f, 1.46f, 1.47f, 1.48f, 1.49f, 1.50f,
        1.51f, 1.52f, 1.53f, 1.54f, 1.55f, 1.56f, 1.57f, 1.58f,
        1.59f, 1.60f,
    };
    const FLOAT in2[50] =
    {
        2.11f, 2.12f, 2.13f, 2.14f, 2.15f, 2.16f, 2.17f, 2.18f,
        2.19f, 2.20f, 2.21f, 2.22f, 2.23f, 2.24f, 2.25f, 2.26f,
        2.27f, 2.28f, 2.29f, 2.30f, 2.31f, 2.32f, 2.33f, 2.34f,
        2.35f, 2.36f, 2.37f, 2.38f, 2.39f, 2.40f, 2.41f, 2.42f,
        2.43f, 2.44f, 2.45f, 2.46f, 2.47f, 2.48f, 2.49f, 2.50f,
        2.51f, 2.52f, 2.53f, 2.54f, 2.55f, 2.56f, 2.57f, 2.58f,
        2.59f, 2.60f,
    };
    FLOAT out[50] = {0.0f};

    /*
     * Order is not limited by D3DXSH_MINORDER and D3DXSH_MAXORDER!
     * All values will work, test from 0-7 [D3DXSH_MINORDER = 2, D3DXSH_MAXORDER = 6]
     * Exceptions will show up when out, in1 or in2 are NULL
     */
    for (k = 0; k < 8; ++k)
    {
        UINT count = k * k;

        ret = D3DXSHAdd(&out[0], k, &in1[0], &in2[0]);
        ok(ret == out, "%u: D3DXSHAdd() failed, got %p, expected %p\n", k, out, ret);

        for (i = 0; i < count; ++i)
        {
            ok(relative_error(in1[i] + in2[i], out[i]) < admitted_error,
                    "%u-%u: D3DXSHAdd() failed, got %f, expected %f\n", k, i, out[i], in1[i] + in2[i]);
        }
        ok(out[count] == 0.0f, "%u-%u: D3DXSHAdd() failed, got %f, expected 0.0\n", k, k * k, out[count]);
    }
}

static void test_D3DXSHDot(void)
{
    unsigned int i;
    FLOAT a[64], b[64], got;
    CONST FLOAT expected[] =
    { 0.5f, 0.5f, 25.0f, 262.5f, 1428.0f, 5362.0f, 15873.0f, 39812.0f, 88400.0f, };

    for (i = 0; i < 64; i++)
    {
        a[i] = (FLOAT)i + 1.0f;
        b[i] = (FLOAT)i + 0.5f;
    }

    /* D3DXSHDot computes by using order * order elements */
    for (i = 0; i < 9; i++)
    {
        got = D3DXSHDot(i, a, b);
        ok(relative_error(got, expected[i]) < admitted_error, "order %d: expected %f, received %f\n", i, expected[i], got);
    }

    return;
}

static void test_D3DXSHEvalDirection(void)
{
    unsigned int i, order;
    D3DXVECTOR3 d;
    FLOAT a[100], expected[100], *received_ptr;
    CONST FLOAT table[36] =
    { 0.282095f, -0.977205f, 1.465808f, -0.488603f, 2.185097f, -6.555291f,
      8.200181f, -3.277646f, -1.638823f, 1.180087f, 17.343668f, -40.220032f,
      47.020218f, -20.110016f, -13.007751f, 6.490479f, -15.020058f, 10.620785f,
      117.325661f, -240.856750f, 271.657288f, -120.428375f, -87.994247f, 58.414314f,
      -4.380850f, 24.942520f, -149.447693f, 78.278130f, 747.791748f, -1427.687866f,
      1574.619141, -713.843933f, -560.843811f, 430.529724, -43.588909, -26.911665, };

    d.x = 1.0; d.y = 2.0f; d.z = 3.0f;

    for(order = 0; order < 10; order++)
    {
        for(i = 0; i < 100; i++)
            a[i] = 1.5f + i;

        received_ptr = D3DXSHEvalDirection(a, order, &d);
        ok(received_ptr == a, "Expected %p, received %p\n", a, received_ptr);

        for(i = 0; i < 100; i++)
        {
            /* if the order is < D3DXSH_MINORDER or order > D3DXSH_MAXORDER or the index of the element is greater than order * order - 1, D3DXSHEvalDirection does not modify the output */
            if ( (order < D3DXSH_MINORDER) || (order > D3DXSH_MAXORDER) || (i >= order * order) )
                expected[i] = 1.5f + i;
            else
                expected[i] = table[i];

            ok(relative_error(a[i], expected[i]) < admitted_error, "order %u, index %u: expected %f, received %f\n", order, i, expected[i], a[i]);
        }
    }
}

static void test_D3DXSHMultiply2(void)
{
    unsigned int i;
    FLOAT a[20], b[20], c[20];
    /* D3DXSHMultiply2 only modifies the first 4 elements of the array */
    const FLOAT expected[20] =
    { 3.418594f, 1.698211f, 1.703853f, 1.709494f, 4.0f, 5.0f,
      6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f,
      14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f };

    for (i = 0; i < 20; i++)
    {
        a[i] = 1.0f + i / 100.0f;
        b[i] = 3.0f - i / 100.0f;
        c[i] = i;
    }

    D3DXSHMultiply2(c, a, b);
    for (i = 0; i < 20; i++)
        ok(relative_error(c[i], expected[i]) < admitted_error, "Expected[%d] = %f, received = %f\n", i, expected[i], c[i]);
}

static void test_D3DXSHMultiply3(void)
{
    unsigned int i;
    FLOAT a[20], b[20], c[20];
    /* D3DXSHMultiply only modifies the first 9 elements of the array */
    const FLOAT expected[20] =
    { 7.813913f, 2.256058f, 5.9484005f, 4.970894f, 2.899858f, 3.598946f,
      1.726572f, 5.573538f, 0.622063f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f,
      14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f };

    for (i = 0; i < 20; i++)
    {
        a[i] = 1.0f + (FLOAT)i/100.0f;
        b[i] = 3.0f - (FLOAT)i/100.0f;
        c[i] = (FLOAT)i;
    }

    D3DXSHMultiply3(c, a, b);
    for (i = 0; i < 20; i++)
        ok(relative_error(c[i], expected[i]) < admitted_error, "Expected[%d] = %f, received = %f\n", i, expected[i], c[i]);
}

static void test_D3DXSHRotateZ(void)
{
    unsigned int i, j, order, square;
    FLOAT angle[] = { D3DX_PI / 3.0f, -D3DX_PI / 3.0f, 4.0f * D3DX_PI / 3.0f, }, expected, in[100], out[100], *received_ptr, table[] =
    { /* Angle =  D3DX_PI / 3.0f */
      1.01f, 4.477762f, 3.010000f, 0.264289f, 5.297888f, 9.941864f, 7.010000f, -1.199813f,
      -8.843789f, -10.010002f, 7.494040f, 18.138016f, 13.010000, -3.395966f, -17.039942f,
      -16.009998f, -30.164297f, -18.010004f, 10.422242f, 29.066219f, 21.010000f, -6.324171f,
      -27.968145f, -24.009998f, 2.226099f, -18.180565, -43.824551f, -28.010004f, 14.082493f,
      42.726471f,  31.010000f, -9.984426f, -41.628399f, -34.009995f, 5.886358f, 40.530331f,
      /* Angle =  D3DX_PI / 3.0f */
      1.01f, -2.467762f, 3.010000f, 3.745711f, -10.307890f, -3.931864f, 7.010000f, 9.209813f,
      -0.166214f, -10.010002f, -18.504044f, -6.128017f, 13.010000f, 17.405966f, 2.029938f,
      -16.009998f, 13.154303f, -18.010004f, -29.432247f, -9.056221f, 21.010000f, 28.334169f,
      4.958139f, -24.010002f, -27.236092f, 44.190582f, 16.814558f, -28.009996f, -43.092499f,
      -12.716474f, 31.010000f, 41.994423f, 8.618393f, -34.010002f, -40.896347f, -4.520310f,
      /* Angle =  4.0f * D3DX_PI / 3.0f */
      1.01f, -4.477762f, 3.010000f, -0.264289f, 5.297887f, -9.941864f, 7.010000f, 1.199814f,
      -8.843788f, 10.010004f, 7.494038f, -18.138016f, 13.010000f, 3.395967f, -17.039940f,
      16.009996f, -30.164293f, 18.010006f, 10.422239f, -29.066219f, 21.010000f, 6.324172f,
      -27.968143f, 24.009993f, 2.226105f, 18.180552f, -43.824543f, 28.010008f, 14.082489f,
      -42.726471f, 31.010000f, 9.984427f, -41.628399f, 34.009987f, 5.886366f, -40.530327, };

    for (i = 0; i < 100; i++)
        in[i] = i + 1.01f;

    for (j = 0; j < 3; j++)
    {
        for (order = 0; order < 10; order++)
        {
            for (i = 0; i < 100; i++)
                out[i] = ( i + 1.0f ) * ( i + 1.0f );

            received_ptr = D3DXSHRotateZ(out, order, angle[j], in);
            ok(received_ptr == out, "angle %f, order %u, Expected %p, received %p\n", angle[j], order, out, received_ptr);

            for (i = 0; i < 100; i++)
            {
                /* order = 0 or order = 1 behaves like order = D3DXSH_MINORDER */
                square = ( order <= D3DXSH_MINORDER ) ? D3DXSH_MINORDER * D3DXSH_MINORDER : order * order;
                expected = table[36 * j + i];
                if ( i >= square || ( (order >= D3DXSH_MAXORDER) && ( i >= D3DXSH_MAXORDER * D3DXSH_MAXORDER ) ) )
                    expected = ( i + 1.0f ) * ( i + 1.0f );

                ok(relative_error(out[i], expected) < admitted_error, "angle %f, order %u index %u, Expected %f, received %f\n", angle[j], order, i, expected, out[i]);
            }
        }
    }
}

static void test_D3DXSHScale(void)
{
    unsigned int i, order;
    FLOAT a[100], b[100], expected, *received_array;

    for (i = 0; i < 100; i++)
    {
        a[i] = i;
        b[i] = i;
    }

    for (order = 0; order < 10; order++)
    {
        received_array = D3DXSHScale(b, order, a, 5.0f);
        ok(received_array == b, "Expected %p, received %p\n", b, received_array);

        for (i = 0; i < 100; i++)
        {
            if (i < order * order)
                expected = 5.0f * a[i];
            /* D3DXSHScale does not modify the elements of the array after the order * order-th element */
            else
                expected = a[i];
            ok(relative_error(b[i], expected) < admitted_error, "order %d, element %d, expected %f, received %f\n", order, i, expected, b[i]);
        }
    }
}

START_TEST(math)
{
    D3DXColorTest();
    D3DXFresnelTest();
    D3DXMatrixTest();
    D3DXPlaneTest();
    D3DXQuaternionTest();
    D3DXVector2Test();
    D3DXVector3Test();
    D3DXVector4Test();
    test_matrix_stack();
    test_Matrix_AffineTransformation2D();
    test_Matrix_Decompose();
    test_Matrix_Transformation2D();
    test_D3DXVec_Array();
    test_D3DXFloat_Array();
    test_D3DXSHAdd();
    test_D3DXSHDot();
    test_D3DXSHEvalDirection();
    test_D3DXSHMultiply2();
    test_D3DXSHMultiply3();
    test_D3DXSHRotateZ();
    test_D3DXSHScale();
}
