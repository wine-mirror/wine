/*
 * Copyright 2007 David Adam
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

#include <assert.h>
#include "d3dx8.h"

#include "wine/test.h"

#define admitted_error 0.00001f

#define expect_color(expectedcolor,gotcolor) ok((fabs(expectedcolor.r-gotcolor.r)<admitted_error)&&(fabs(expectedcolor.g-gotcolor.g)<admitted_error)&&(fabs(expectedcolor.b-gotcolor.b)<admitted_error)&&(fabs(expectedcolor.a-gotcolor.a)<admitted_error),"Expected Color= (%f, %f, %f, %f)\n , Got Color= (%f, %f, %f, %f)\n", expectedcolor.r, expectedcolor.g, expectedcolor.b, expectedcolor.a, gotcolor.r, gotcolor.g, gotcolor.b, gotcolor.a);

#define expect_mat(expectedmat,gotmat)\
{ \
    int i,j,equal=1; \
    for (i=0; i<4; i++)\
        {\
         for (j=0; j<4; j++)\
             {\
              if (fabs(expectedmat.m[i][j]-gotmat.m[i][j])>admitted_error)\
                 {\
                  equal=0;\
                 }\
             }\
        }\
    ok(equal, "Expected matrix=\n(%f,%f,%f,%f\n %f,%f,%f,%f\n %f,%f,%f,%f\n %f,%f,%f,%f\n)\n\n" \
       "Got matrix=\n(%f,%f,%f,%f\n %f,%f,%f,%f\n %f,%f,%f,%f\n %f,%f,%f,%f)\n", \
       expectedmat.m[0][0],expectedmat.m[0][1],expectedmat.m[0][2],expectedmat.m[0][3], \
       expectedmat.m[1][0],expectedmat.m[1][1],expectedmat.m[1][2],expectedmat.m[1][3], \
       expectedmat.m[2][0],expectedmat.m[2][1],expectedmat.m[2][2],expectedmat.m[2][3], \
       expectedmat.m[3][0],expectedmat.m[3][1],expectedmat.m[3][2],expectedmat.m[3][3], \
       gotmat.m[0][0],gotmat.m[0][1],gotmat.m[0][2],gotmat.m[0][3], \
       gotmat.m[1][0],gotmat.m[1][1],gotmat.m[1][2],gotmat.m[1][3], \
       gotmat.m[2][0],gotmat.m[2][1],gotmat.m[2][2],gotmat.m[2][3], \
       gotmat.m[3][0],gotmat.m[3][1],gotmat.m[3][2],gotmat.m[3][3]); \
}

#define expect_vec(expectedvec,gotvec) ok((fabs(expectedvec.x-gotvec.x)<admitted_error)&&(fabs(expectedvec.y-gotvec.y)<admitted_error),"Expected Vector= (%f, %f)\n , Got Vector= (%f, %f)\n", expectedvec.x, expectedvec.y, gotvec.x, gotvec.y);

#define expect_vec3(expectedvec,gotvec) ok((fabs(expectedvec.x-gotvec.x)<admitted_error)&&(fabs(expectedvec.y-gotvec.y)<admitted_error)&&(fabs(expectedvec.z-gotvec.z)<admitted_error),"Expected Vector= (%f, %f, %f)\n , Got Vector= (%f, %f, %f)\n", expectedvec.x, expectedvec.y, expectedvec.z, gotvec.x, gotvec.y, gotvec.z);

#define expect_vec4(expectedvec,gotvec) ok((fabs(expectedvec.x-gotvec.x)<admitted_error)&&(fabs(expectedvec.y-gotvec.y)<admitted_error)&&(fabs(expectedvec.z-gotvec.z)<admitted_error)&&(fabs(expectedvec.w-gotvec.w)<admitted_error),"Expected Vector= (%f, %f, %f, %f)\n , Got Vector= (%f, %f, %f, %f)\n", expectedvec.x, expectedvec.y, expectedvec.z, expectedvec.w, gotvec.x, gotvec.y, gotvec.z, gotvec.w);

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

static void D3DXMatrixTest(void)
{
    D3DXMATRIX expectedmat, gotmat, mat, mat2, mat3;
    D3DXQUATERNION q;
    D3DXVECTOR3 at, axis, eye;
    BOOL expected, got;
    FLOAT angle, expectedfloat, gotfloat;

    U(mat).m[0][1] = 5.0f; U(mat).m[0][2] = 7.0f; U(mat).m[0][3] = 8.0f;
    U(mat).m[1][0] = 11.0f; U(mat).m[1][2] = 16.0f; U(mat).m[1][3] = 33.0f;
    U(mat).m[2][0] = 19.0f; U(mat).m[2][1] = -21.0f; U(mat).m[2][3] = 43.0f;
    U(mat).m[3][0] = 2.0f; U(mat).m[3][1] = 3.0f; U(mat).m[3][2] = -4.0f;
    U(mat).m[0][0] = 10.0f; U(mat).m[1][1] = 20.0f; U(mat).m[2][2] = 30.0f;
    U(mat).m[3][3] = -40.0f;

    mat2.m[0][0] = 1.0f; mat2.m[1][0] = 2.0f; mat2.m[2][0] = 3.0f;
    mat2.m[3][0] = 4.0f; mat2.m[0][1] = 5.0f; mat2.m[1][1] = 6.0f;
    mat2.m[2][1] = 7.0f; mat2.m[3][1] = 8.0f; mat2.m[0][2] = -8.0f;
    mat2.m[1][2] = -7.0f; mat2.m[2][2] = -6.0f; mat2.m[3][2] = -5.0f;
    mat2.m[0][3] = -4.0f; mat2.m[1][3] = -3.0f; mat2.m[2][3] = -2.0f;
    mat2.m[3][3] = -1.0f;

    q.x = 1.0f; q.y = -4.0f; q.z =7.0f; q.w = -11.0f;

    at.x = -2.0f; at.y = 13.0f; at.z = -9.0f;
    axis.x = 1.0f; axis.y = -3.0f; axis.z = 7.0f;
    eye.x = 8.0f; eye.y = -5.0f; eye.z = 5.75f;

    angle = D3DX_PI/3.0f;

/*____________D3DXMatrixfDeterminant_____________*/
    expectedfloat = -147888.0f;
    gotfloat = D3DXMatrixfDeterminant(&mat);
    ok(fabs( gotfloat - expectedfloat ) < admitted_error, "Expected: %f, Got: %f\n", expectedfloat, gotfloat);

/*____________D3DXMatrixIsIdentity______________*/
    expected = FALSE;
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
    expectedmat.m[0][0] = -0.822465f; expectedmat.m[0][1] = -0.409489f; expectedmat.m[0][2] = -0.394803f; expectedmat.m[0][3] = 0.0f;
    expectedmat.m[1][0] = -0.555856f; expectedmat.m[1][1] = 0.431286f; expectedmat.m[1][2] = 0.710645f; expectedmat.m[1][3] = 0.0f;
    expectedmat.m[2][0] = -0.120729f; expectedmat.m[2][1] = 0.803935f; expectedmat.m[2][2] = -0.582335f; expectedmat.m[2][3] = 0.0f;
    expectedmat.m[3][0] = 4.494634f; expectedmat.m[3][1] = 0.809719f; expectedmat.m[3][2] = 10.060076f; expectedmat.m[3][3] = 1.0f;
    D3DXMatrixLookAtLH(&gotmat,&eye,&at,&axis);
    expect_mat(expectedmat,gotmat);

/*____________D3DXMatrixLookatRH_______________*/
    expectedmat.m[0][0] = 0.822465f; expectedmat.m[0][1] = -0.409489f; expectedmat.m[0][2] = 0.394803f; expectedmat.m[0][3] = 0.0f;
    expectedmat.m[1][0] = 0.555856f; expectedmat.m[1][1] = 0.431286f; expectedmat.m[1][2] = -0.710645f; expectedmat.m[1][3] = 0.0f;
    expectedmat.m[2][0] = 0.120729f; expectedmat.m[2][1] = 0.803935f; expectedmat.m[2][2] = 0.582335f; expectedmat.m[2][3] = 0.0f;
    expectedmat.m[3][0] = -4.494634f; expectedmat.m[3][1] = 0.809719f; expectedmat.m[3][2] = -10.060076f; expectedmat.m[3][3] = 1.0f;
    D3DXMatrixLookAtRH(&gotmat,&eye,&at,&axis);
    expect_mat(expectedmat,gotmat);

/*____________D3DXMatrixMultiply______________*/
    expectedmat.m[0][0] = 73.0f; expectedmat.m[0][1] = 193.0f; expectedmat.m[0][2] = -197.0f; expectedmat.m[0][3] = -77.0f;
    expectedmat.m[1][0] = 231.0f; expectedmat.m[1][1] = 551.0f; expectedmat.m[1][2] = -489.0f; expectedmat.m[1][3] = -169.0;
    expectedmat.m[2][0] = 239.0f; expectedmat.m[2][1] = 523.0f; expectedmat.m[2][2] = -400.0f; expectedmat.m[2][3] = -116.0f;
    expectedmat.m[3][0] = -164.0f; expectedmat.m[3][1] = -320.0f; expectedmat.m[3][2] = 187.0f; expectedmat.m[3][3] = 31.0f;
    D3DXMatrixMultiply(&gotmat,&mat,&mat2);
    expect_mat(expectedmat,gotmat);

/*____________D3DXMatrixPerspectiveRH_______________*/
    expectedmat.m[0][0] = -24.0f; expectedmat.m[0][1] = -0.0f; expectedmat.m[0][2] = 0.0f; expectedmat.m[0][3] = 0.0f;
    expectedmat.m[1][0] = 0.0f; expectedmat.m[1][1] = -6.4f; expectedmat.m[1][2] = 0.0; expectedmat.m[1][3] = 0.0f;
    expectedmat.m[2][0] = 0.0f; expectedmat.m[2][1] = 0.0f; expectedmat.m[2][2] = -0.783784f; expectedmat.m[2][3] = -1.0f;
    expectedmat.m[3][0] = 0.0f; expectedmat.m[3][1] = 0.0f; expectedmat.m[3][2] = 1.881081f; expectedmat.m[3][3] = 0.0f;
    D3DXMatrixPerspectiveRH(&gotmat, 0.2f, 0.75f, -2.4f, 8.7f);
    expect_mat(expectedmat,gotmat);

/*____________D3DXMatrixRotationAxis_____*/
    expectedmat.m[0][0] = 0.508475f; expectedmat.m[0][1] = 0.763805f; expectedmat.m[0][2] = 0.397563f; expectedmat.m[0][3] = 0.0f;
    expectedmat.m[1][0] = -0.814652f; expectedmat.m[1][1] = 0.576271f; expectedmat.m[1][2] = -0.065219f; expectedmat.m[1][3] = 0.0f;
    expectedmat.m[2][0] = -0.278919f; expectedmat.m[2][1] = -0.290713f; expectedmat.m[2][2] = 0.915254f; expectedmat.m[2][3] = 0.0f;
    expectedmat.m[3][0] = 0.0f; expectedmat.m[3][1] = 0.0f; expectedmat.m[3][2] = 0.0f; expectedmat.m[3][3] = 1.0f;
    D3DXMatrixRotationAxis(&gotmat,&axis,angle);
    expect_mat(expectedmat,gotmat);

/*____________D3DXMatrixRotationQuaternion______________*/
    expectedmat.m[0][0] = -129.0f; expectedmat.m[0][1] = -162.0f; expectedmat.m[0][2] = -74.0f; expectedmat.m[0][3] = 0.0f;
    expectedmat.m[1][0] = 146.0f; expectedmat.m[1][1] = -99.0f; expectedmat.m[1][2] = -78.0f; expectedmat.m[1][3] = 0.0f;
    expectedmat.m[2][0] = 102.0f; expectedmat.m[2][1] = -34.0f; expectedmat.m[2][2] = -33.0f; expectedmat.m[2][3] = 0.0f;
    expectedmat.m[3][0] = 0.0f; expectedmat.m[3][1] = 0.0f; expectedmat.m[3][2] = 0.0f; expectedmat.m[3][3] = 1.0f;
    D3DXMatrixRotationQuaternion(&gotmat,&q);
    expect_mat(expectedmat,gotmat);

/*____________D3DXMatrixRotationX______________*/
    expectedmat.m[0][0] = 1.0f; expectedmat.m[0][1] = 0.0f; expectedmat.m[0][2] = 0.0f; expectedmat.m[0][3] = 0.0f;
    expectedmat.m[1][0] = 0.0; expectedmat.m[1][1] = 0.5f; expectedmat.m[1][2] = sqrt(3.0f)/2.0f; expectedmat.m[1][3] = 0.0f;
    expectedmat.m[2][0] = 0.0f; expectedmat.m[2][1] = -sqrt(3.0f)/2.0f; expectedmat.m[2][2] = 0.5f; expectedmat.m[2][3] = 0.0f;
    expectedmat.m[3][0] = 0.0f; expectedmat.m[3][1] = 0.0f; expectedmat.m[3][2] = 0.0f; expectedmat.m[3][3] = 1.0f;
    D3DXMatrixRotationX(&gotmat,angle);
    expect_mat(expectedmat,gotmat);

/*____________D3DXMatrixRotationY______________*/
    expectedmat.m[0][0] = 0.5f; expectedmat.m[0][1] = 0.0f; expectedmat.m[0][2] = -sqrt(3.0f)/2.0f; expectedmat.m[0][3] = 0.0f;
    expectedmat.m[1][0] = 0.0; expectedmat.m[1][1] = 1.0f; expectedmat.m[1][2] = 0.0f; expectedmat.m[1][3] = 0.0f;
    expectedmat.m[2][0] = sqrt(3.0f)/2.0f; expectedmat.m[2][1] = 0.0f; expectedmat.m[2][2] = 0.5f; expectedmat.m[2][3] = 0.0f;
    expectedmat.m[3][0] = 0.0f; expectedmat.m[3][1] = 0.0f; expectedmat.m[3][2] = 0.0f; expectedmat.m[3][3] = 1.0f;
    D3DXMatrixRotationY(&gotmat,angle);
    expect_mat(expectedmat,gotmat);

/*____________D3DXMatrixRotationYawPitchRoll____*/
    expectedmat.m[0][0] = 0.888777f; expectedmat.m[0][1] = 0.091875f; expectedmat.m[0][2] = -0.449037f; expectedmat.m[0][3] = 0.0f;
    expectedmat.m[1][0] = 0.351713f; expectedmat.m[1][1] = 0.491487f; expectedmat.m[1][2] = 0.796705f; expectedmat.m[1][3] = 0.0f;
    expectedmat.m[2][0] = 0.293893f; expectedmat.m[2][1] = -0.866025f; expectedmat.m[2][2] = 0.404509f; expectedmat.m[2][3] = 0.0f;
    expectedmat.m[3][0] = 0.0f; expectedmat.m[3][1] = 0.0f; expectedmat.m[3][2] = 0.0f; expectedmat.m[3][3] = 1.0f;
    D3DXMatrixRotationYawPitchRoll(&gotmat, 3.0f*angle/5.0f, angle, 3.0f*angle/17.0f);
    expect_mat(expectedmat,gotmat);

/*____________D3DXMatrixRotationZ______________*/
    expectedmat.m[0][0] = 0.5f; expectedmat.m[0][1] = sqrt(3.0f)/2.0f; expectedmat.m[0][2] = 0.0f; expectedmat.m[0][3] = 0.0f;
    expectedmat.m[1][0] = -sqrt(3.0f)/2.0f; expectedmat.m[1][1] = 0.5f; expectedmat.m[1][2] = 0.0f; expectedmat.m[1][3] = 0.0f;
    expectedmat.m[2][0] = 0.0f; expectedmat.m[2][1] = 0.0f; expectedmat.m[2][2] = 1.0f; expectedmat.m[2][3] = 0.0f;
    expectedmat.m[3][0] = 0.0f; expectedmat.m[3][1] = 0.0f; expectedmat.m[3][2] = 0.0f; expectedmat.m[3][3] = 1.0f;
    D3DXMatrixRotationZ(&gotmat,angle);
    expect_mat(expectedmat,gotmat);

/*____________D3DXMatrixScaling______________*/
    expectedmat.m[0][0] = 0.69f; expectedmat.m[0][1] = 0.0f; expectedmat.m[0][2] = 0.0f; expectedmat.m[0][3] = 0.0f;
    expectedmat.m[1][0] = 0.0; expectedmat.m[1][1] = 0.53f; expectedmat.m[1][2] = 0.0f; expectedmat.m[1][3] = 0.0f;
    expectedmat.m[2][0] = 0.0f; expectedmat.m[2][1] = 0.0f; expectedmat.m[2][2] = 4.11f; expectedmat.m[2][3] = 0.0f;
    expectedmat.m[3][0] = 0.0f; expectedmat.m[3][1] = 0.0f; expectedmat.m[3][2] = 0.0f; expectedmat.m[3][3] = 1.0f;
    D3DXMatrixScaling(&gotmat,0.69f,0.53f,4.11f);
    expect_mat(expectedmat,gotmat);

/*____________D3DXMatrixTranslation______________*/
    expectedmat.m[0][0] = 1.0f; expectedmat.m[0][1] = 0.0f; expectedmat.m[0][2] = 0.0f; expectedmat.m[0][3] = 0.0f;
    expectedmat.m[1][0] = 0.0; expectedmat.m[1][1] = 1.0f; expectedmat.m[1][2] = 0.0f; expectedmat.m[1][3] = 0.0f;
    expectedmat.m[2][0] = 0.0f; expectedmat.m[2][1] = 0.0f; expectedmat.m[2][2] = 1.0f; expectedmat.m[2][3] = 0.0f;
    expectedmat.m[3][0] = 0.69f; expectedmat.m[3][1] = 0.53f; expectedmat.m[3][2] = 4.11f; expectedmat.m[3][3] = 1.0f;
    D3DXMatrixTranslation(&gotmat,0.69f,0.53f,4.11f);
    expect_mat(expectedmat,gotmat);

/*____________D3DXMatrixTranspose______________*/
    expectedmat.m[0][0] = 10.0f; expectedmat.m[0][1] = 11.0f; expectedmat.m[0][2] = 19.0f; expectedmat.m[0][3] = 2.0f;
    expectedmat.m[1][0] = 5.0; expectedmat.m[1][1] = 20.0f; expectedmat.m[1][2] = -21.0f; expectedmat.m[1][3] = 3.0f;
    expectedmat.m[2][0] = 7.0f; expectedmat.m[2][1] = 16.0f; expectedmat.m[2][2] = 30.f; expectedmat.m[2][3] = -4.0f;
    expectedmat.m[3][0] = 8.0f; expectedmat.m[3][1] = 33.0f; expectedmat.m[3][2] = 43.0f; expectedmat.m[3][3] = -40.0f;
    D3DXMatrixTranspose(&gotmat,&mat);
    expect_mat(expectedmat,gotmat);
}

static void D3DXPlaneTest(void)
{
    D3DXPLANE plane;
    D3DXVECTOR4 vec;
    FLOAT expected, got;

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
}

static void D3X8QuaternionTest(void)
{
    D3DXQUATERNION expectedquat, gotquat, nul, q, r, s;
    LPD3DXQUATERNION funcpointer;
    FLOAT expected, got;
    BOOL expectedbool, gotbool;

    nul.x = 0.0f; nul.y = 0.0f; nul.z = 0.0f; nul.w = 0.0f;
    q.x = 1.0f, q.y = 2.0f; q.z = 4.0f; q.w = 10.0f;
    r.x = -3.0f; r.y = 4.0f; r.z = -5.0f; r.w = 7.0;

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
    ok(fabs( got - expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);
    /* Tests the case NULL */
    expected=0.0f;
    got = D3DXQuaternionDot(NULL,&r);
    ok(fabs( got - expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);
    expected=0.0f;
    got = D3DXQuaternionDot(NULL,NULL);
    ok(fabs( got - expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);

/*_______________D3DXQuaternionIdentity________________*/
    expectedquat.x = 0.0f; expectedquat.y = 0.0f; expectedquat.z = 0.0f; expectedquat.w = 1.0f;
    D3DXQuaternionIdentity(&gotquat);
    expect_vec4(expectedquat,gotquat);
    /* Test the NULL case */
    funcpointer = D3DXQuaternionIdentity(NULL);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);

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
   ok(fabs( got - expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);
   /* Tests the case NULL */
    expected=0.0f;
    got = D3DXQuaternionLength(NULL);
    ok(fabs( got - expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);

/*_______________D3DXQuaternionLengthSq________________________*/
    expected = 121.0f;
    got = D3DXQuaternionLengthSq(&q);
    ok(fabs( got - expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);
    /* Tests the case NULL */
    expected=0.0f;
    got = D3DXQuaternionLengthSq(NULL);
    ok(fabs( got - expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);

/*_______________D3DXQuaternionNormalize________________________*/
    expectedquat.x = 1.0f/11.0f; expectedquat.y = 2.0f/11.0f; expectedquat.z = 4.0f/11.0f; expectedquat.w = 10.0f/11.0f;
    D3DXQuaternionNormalize(&gotquat,&q);
    expect_vec4(expectedquat,gotquat);
    /* Test the nul quaternion */
    expectedquat.x = 0.0f; expectedquat.y = 0.0f; expectedquat.z = 0.0f; expectedquat.w = 0.0f;
    D3DXQuaternionNormalize(&gotquat,&nul);
    expect_vec4(expectedquat,gotquat);
}

static void D3X8Vector2Test(void)
{
    D3DXVECTOR2 expectedvec, gotvec, nul, nulproj, u, v, w, x;
    LPD3DXVECTOR2 funcpointer;
    D3DXVECTOR4 expectedtrans, gottrans;
    D3DXMATRIX mat;
    FLOAT coeff1, coeff2, expected, got, scale;

    nul.x = 0.0f; nul.y = 0.0f;
    u.x = 3.0f; u.y = 4.0f;
    v.x = -7.0f; v.y = 9.0f;
    w.x = 4.0f; w.y = -3.0f;
    x.x = 2.0f; x.y = -11.0f;

    mat.m[0][0] = 1.0f; mat.m[0][1] = 2.0f; mat.m[0][2] = 3.0f; mat.m[0][3] = 4.0f;
    mat.m[1][0] = 5.0f; mat.m[1][1] = 6.0f; mat.m[1][2] = 7.0f; mat.m[1][3] = 8.0f;
    mat.m[2][0] = 9.0f; mat.m[2][1] = 10.0f; mat.m[2][2] = 11.0f; mat.m[2][3] = 12.0f;
    mat.m[3][0] = 13.0f; mat.m[3][1] = 14.0f; mat.m[3][2] = 15.0f; mat.m[3][3] = 16.0f;

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
   ok(fabs( got - expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);
   /* Tests the case NULL */
    expected=0.0f;
    got = D3DXVec2CCW(NULL,&v);
    ok(fabs( got - expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);
    expected=0.0f;
    got = D3DXVec2CCW(NULL,NULL);
    ok(fabs( got - expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);

/*_______________D3DXVec2Dot__________________________*/
    expected = 15.0f;
    got = D3DXVec2Dot(&u,&v);
    ok(fabs( got - expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);
    /* Tests the case NULL */
    expected=0.0f;
    got = D3DXVec2Dot(NULL,&v);
    ok(fabs( got - expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);
    expected=0.0f;
    got = D3DXVec2Dot(NULL,NULL);
    ok(fabs( got - expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);

/*_______________D3DXVec2Hermite__________________________*/
    expectedvec.x = 2604.625f; expectedvec.y = -4533.0f;
    D3DXVec2Hermite(&gotvec,&u,&v,&w,&x,scale);
    expect_vec(expectedvec,gotvec);

/*_______________D3DXVec2Length__________________________*/
   expected = 5.0f;
   got = D3DXVec2Length(&u);
   ok(fabs( got - expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);
   /* Tests the case NULL */
    expected=0.0f;
    got = D3DXVec2Length(NULL);
    ok(fabs( got - expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);

/*_______________D3DXVec2LengthSq________________________*/
   expected = 25.0f;
   got = D3DXVec2LengthSq(&u);
   ok(fabs( got - expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);
   /* Tests the case NULL */
    expected=0.0f;
    got = D3DXVec2LengthSq(NULL);
    ok(fabs( got - expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);

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
    /* Test the nul projected vector */
    nulproj.x = -2.0f; nulproj.y = -1.0f;
    expectedvec.x = 0.0f; expectedvec.y = 0.0f;
    D3DXVec2TransformCoord(&gotvec,&nulproj,&mat);
    expect_vec(expectedvec,gotvec);

 /*_______________D3DXVec2TransformNormal______________________*/
    expectedvec.x = 23.0f; expectedvec.y = 30.0f;
    D3DXVec2TransformNormal(&gotvec,&u,&mat);
    expect_vec(expectedvec,gotvec);
}

static void D3X8Vector3Test(void)
{
    D3DXVECTOR3 expectedvec, gotvec, nul, nulproj, u, v, w, x;
    LPD3DXVECTOR3 funcpointer;
    D3DXVECTOR4 expectedtrans, gottrans;
    D3DXMATRIX mat;
    FLOAT coeff1, coeff2, expected, got, scale;

    nul.x = 0.0f; nul.y = 0.0f; nul.z = 0.0f;
    u.x = 9.0f; u.y = 6.0f; u.z = 2.0f;
    v.x = 2.0f; v.y = -3.0f; v.z = -4.0;
    w.x = 3.0f; w.y = -5.0f; w.z = 7.0f;
    x.x = 4.0f; x.y = 1.0f; x.z = 11.0f;

    mat.m[0][0] = 1.0f; mat.m[0][1] = 2.0f; mat.m[0][2] = 3.0f; mat.m[0][3] = 4.0f;
    mat.m[1][0] = 5.0f; mat.m[1][1] = 6.0f; mat.m[1][2] = 7.0f; mat.m[1][3] = 8.0f;
    mat.m[2][0] = 9.0f; mat.m[2][1] = 10.0f; mat.m[2][2] = 11.0f; mat.m[2][3] = 12.0f;
    mat.m[3][0] = 13.0f; mat.m[3][1] = 14.0f; mat.m[3][2] = 15.0f; mat.m[3][3] = 16.0f;

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
    ok(fabs( got - expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);
    /* Tests the case NULL */
    expected=0.0f;
    got = D3DXVec3Dot(NULL,&v);
    ok(fabs( got - expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);
    expected=0.0f;
    got = D3DXVec3Dot(NULL,NULL);
    ok(fabs( got - expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);

/*_______________D3DXVec3Hermite__________________________*/
    expectedvec.x = -6045.75f; expectedvec.y = -6650.0f; expectedvec.z = 1358.875f;
    D3DXVec3Hermite(&gotvec,&u,&v,&w,&x,scale);
    expect_vec3(expectedvec,gotvec);

/*_______________D3DXVec3Length__________________________*/
   expected = 11.0f;
   got = D3DXVec3Length(&u);
   ok(fabs( got - expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);
   /* Tests the case NULL */
    expected=0.0f;
    got = D3DXVec3Length(NULL);
    ok(fabs( got - expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);

/*_______________D3DXVec3LengthSq________________________*/
    expected = 121.0f;
    got = D3DXVec3LengthSq(&u);
    ok(fabs( got - expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);
   /* Tests the case NULL */
    expected=0.0f;
    got = D3DXVec3LengthSq(NULL);
    ok(fabs( got - expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);

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
    /* Test the nul projected vector */
    nulproj.x = 1.0f; nulproj.y = -1.0f, nulproj.z = -1.0f;
    expectedvec.x = 0.0f; expectedvec.y = 0.0f; expectedvec.z = 0.0f;
    D3DXVec3TransformCoord(&gotvec,&nulproj,&mat);
    expect_vec3(expectedvec,gotvec);

/*_______________D3DXVec3TransformNormal______________________*/
    expectedvec.x = 57.0f; expectedvec.y = 74.0f; expectedvec.z = 91.0f;
    D3DXVec3TransformNormal(&gotvec,&u,&mat);
    expect_vec3(expectedvec,gotvec);
}

static void D3X8Vector4Test(void)
{
    D3DXVECTOR4 expectedvec, gotvec, nul, u, v, w, x;
    LPD3DXVECTOR4 funcpointer;
    D3DXVECTOR4 expectedtrans, gottrans;
    D3DXMATRIX mat;
    FLOAT coeff1, coeff2, expected, got, scale;

    nul.x = 0.0f; nul.y = 0.0f; nul.z = 0.0f; nul.w = 0.0f;
    u.x = 1.0f; u.y = 2.0f; u.z = 4.0f; u.w = 10.0;
    v.x = -3.0f; v.y = 4.0f; v.z = -5.0f; v.w = 7.0;
    w.x = 4.0f; w.y =6.0f; w.z = -2.0f; w.w = 1.0f;
    x.x = 6.0f; x.y = -7.0f; x.z =8.0f; x.w = -9.0f;

    mat.m[0][0] = 1.0f; mat.m[0][1] = 2.0f; mat.m[0][2] = 3.0f; mat.m[0][3] = 4.0f;
    mat.m[1][0] = 5.0f; mat.m[1][1] = 6.0f; mat.m[1][2] = 7.0f; mat.m[1][3] = 8.0f;
    mat.m[2][0] = 9.0f; mat.m[2][1] = 10.0f; mat.m[2][2] = 11.0f; mat.m[2][3] = 12.0f;
    mat.m[3][0] = 13.0f; mat.m[3][1] = 14.0f; mat.m[3][2] = 15.0f; mat.m[3][3] = 16.0f;

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
    ok(fabs( got - expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);
    /* Tests the case NULL */
    expected=0.0f;
    got = D3DXVec4Dot(NULL,&v);
    ok(fabs( got - expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);
    expected=0.0f;
    got = D3DXVec4Dot(NULL,NULL);
    ok(fabs( got - expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);

/*_______________D3DXVec4Hermite_________________________*/
    expectedvec.x = 1224.625f; expectedvec.y = 3461.625f; expectedvec.z = -4758.875f; expectedvec.w = -5781.5f;
    D3DXVec4Hermite(&gotvec,&u,&v,&w,&x,scale);
    expect_vec4(expectedvec,gotvec);

/*_______________D3DXVec4Length__________________________*/
   expected = 11.0f;
   got = D3DXVec4Length(&u);
   ok(fabs( got - expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);
   /* Tests the case NULL */
    expected=0.0f;
    got = D3DXVec4Length(NULL);
    ok(fabs( got - expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);

/*_______________D3DXVec4LengthSq________________________*/
    expected = 121.0f;
    got = D3DXVec4LengthSq(&u);
    ok(fabs( got - expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);
    /* Tests the case NULL */
    expected=0.0f;
    got = D3DXVec4LengthSq(NULL);
    ok(fabs( got - expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);

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
    /* Test the nul vector */
    expectedvec.x = 0.0f; expectedvec.y = 0.0f; expectedvec.z = 0.0f; expectedvec.w = 0.0f;
    D3DXVec4Normalize(&gotvec,&nul);
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

START_TEST(math)
{
    D3DXColorTest();
    D3DXMatrixTest();
    D3DXPlaneTest();
    D3X8QuaternionTest();
    D3X8Vector2Test();
    D3X8Vector3Test();
    D3X8Vector4Test();
}
