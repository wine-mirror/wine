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
#include "d3dx8math.h"
#include "d3dx8math.inl"

#include "wine/test.h"

#define admitted_error 0.00001f

#define expect_color(expectedcolor,gotcolor) ok((fabs(expectedcolor.r-gotcolor.r)<admitted_error)&&(fabs(expectedcolor.g-gotcolor.g)<admitted_error)&&(fabs(expectedcolor.b-gotcolor.b)<admitted_error)&&(fabs(expectedcolor.a-gotcolor.a)<admitted_error),"Expected Color= (%f, %f, %f, %f)\n , Got Color= (%f, %f, %f, %f)\n", expectedcolor.r, expectedcolor.g, expectedcolor.b, expectedcolor.a, gotcolor.r, gotcolor.g, gotcolor.b, gotcolor.a);

#define expect_vec(expectedvec,gotvec) ok((fabs(expectedvec.x-gotvec.x)<admitted_error)&&(fabs(expectedvec.y-gotvec.y)<admitted_error),"Expected Vector= (%f, %f)\n , Got Vector= (%f, %f)\n", expectedvec.x, expectedvec.y, gotvec.x, gotvec.y);

#define expect_vec3(expectedvec,gotvec) ok((fabs(expectedvec.x-gotvec.x)<admitted_error)&&(fabs(expectedvec.y-gotvec.y)<admitted_error)&&(fabs(expectedvec.z-gotvec.z)<admitted_error),"Expected Vector= (%f, %f,%f)\n , Got Vector= (%f, %f, %f)\n", expectedvec.x, expectedvec.y, expectedvec.z, gotvec.x, gotvec.y, gotvec.z);

#define expect_vec4(expectedvec,gotvec) ok((fabs(expectedvec.x-gotvec.x)<admitted_error)&&(fabs(expectedvec.y-gotvec.y)<admitted_error)&&(fabs(expectedvec.z-gotvec.z)<admitted_error)&&(fabs(expectedvec.w-gotvec.w)<admitted_error),"Expected Vector= (%f, %f, %f, %f)\n , Got Vector= (%f, %f, %f, %f)\n", expectedvec.x, expectedvec.y, expectedvec.z, expectedvec.w, gotvec.x, gotvec.y, gotvec.z, gotvec.w);

static void D3DXColorTest(void)
{
    D3DXCOLOR color, color1, expected, got;
    LPD3DXCOLOR funcpointer;

    color.r = 0.2f; color.g = 0.75f; color.b = 0.41f; color.a = 0.93f;

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
    D3DXQUATERNION expectedquat, gotquat, q, r, s;
    LPD3DXQUATERNION funcpointer;
    FLOAT expected, got;
    BOOL expectedbool, gotbool;

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
}

static void D3X8Vector2Test(void)
{
    D3DXVECTOR2 expectedvec, gotvec, u, v;
    LPD3DXVECTOR2 funcpointer;
    FLOAT expected, got, scale;

    u.x=3.0f; u.y=4.0f;
    v.x=-7.0f; v.y=9.0f;
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

}

static void D3X8Vector3Test(void)
{
    D3DXVECTOR3 expectedvec, gotvec, u, v;
    LPD3DXVECTOR3 funcpointer;
    FLOAT expected, got, scale;

    u.x = 9.0f; u.y = 6.0f; u.z = 2.0f;
    v.x = 2.0f; v.y = -3.0f; v.z = -4.0;
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

/*_______________D3DXVec3Cross________________________*/
    expectedvec.x = -18.0f; expectedvec.y = 40.0f; expectedvec.z = -30.0f;
    D3DXVec3Cross(&gotvec,&u,&v);
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

}

static void D3X8Vector4Test(void)
{
    D3DXVECTOR4 expectedvec, gotvec, u, v;
    LPD3DXVECTOR4 funcpointer;
    FLOAT expected, got, scale;
    scale = -6.5f;

    u.x = 1.0f; u.y = 2.0f; u.z = 4.0f; u.w = 10.0;
    v.x = -3.0f; v.y = 4.0f; v.z = -5.0f; v.w = 7.0;

/*_______________D3DXVec4Add__________________________*/
    expectedvec.x = -2.0f; expectedvec.y = 6.0f; expectedvec.z = -1.0f; expectedvec.w = 17.0f;
    D3DXVec4Add(&gotvec,&u,&v);
    expect_vec4(expectedvec,gotvec);
    /* Tests the case NULL */
    funcpointer = D3DXVec4Add(&gotvec,NULL,&v);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);
    funcpointer = D3DXVec4Add(NULL,NULL,NULL);
    ok(funcpointer == NULL, "Expected: %p, Got: %p\n", NULL, funcpointer);

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
}

START_TEST(math)
{
    D3DXColorTest();
    D3DXPlaneTest();
    D3X8QuaternionTest();
    D3X8Vector2Test();
    D3X8Vector3Test();
    D3X8Vector4Test();
}
