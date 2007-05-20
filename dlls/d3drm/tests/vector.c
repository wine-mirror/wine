/*
 * Copyright 2007 Vijay Kiran Kamuju
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
#include "d3drmdef.h"
#include <math.h>

#include "wine/test.h"

#define PI (4*atan(1.0))
#define admit_error 0.000001

#define expect_mat( expectedmat, gotmat)\
{ \
    int i,j,equal=1; \
    for (i=0; i<4; i++)\
        {\
         for (j=0; j<4; j++)\
             {\
              if (fabs(expectedmat[i][j]-gotmat[i][j])>admit_error)\
                 {\
                  equal=0;\
                 }\
             }\
        }\
    ok(equal, "Expected matrix=\n(%f,%f,%f,%f\n %f,%f,%f,%f\n %f,%f,%f,%f\n %f,%f,%f,%f\n)\n\n" \
       "Got matrix=\n(%f,%f,%f,%f\n %f,%f,%f,%f\n %f,%f,%f,%f\n %f,%f,%f,%f)\n", \
       expectedmat[0][0],expectedmat[0][1],expectedmat[0][2],expectedmat[0][3], \
       expectedmat[1][0],expectedmat[1][1],expectedmat[1][2],expectedmat[1][3], \
       expectedmat[2][0],expectedmat[2][1],expectedmat[2][2],expectedmat[2][3], \
       expectedmat[3][0],expectedmat[3][1],expectedmat[3][2],expectedmat[3][3], \
       gotmat[0][0],gotmat[0][1],gotmat[0][2],gotmat[0][3], \
       gotmat[1][0],gotmat[1][1],gotmat[1][2],gotmat[1][3], \
       gotmat[2][0],gotmat[2][1],gotmat[2][2],gotmat[2][3], \
       gotmat[3][0],gotmat[3][1],gotmat[3][2],gotmat[3][3] ); \
}

#define expect_quat(expectedquat,gotquat) \
  ok( (fabs(U1(expectedquat.v).x-U1(gotquat.v).x)<admit_error) &&     \
      (fabs(U2(expectedquat.v).y-U2(gotquat.v).y)<admit_error) &&     \
      (fabs(U3(expectedquat.v).z-U3(gotquat.v).z)<admit_error) &&     \
      (fabs(expectedquat.s-gotquat.s)<admit_error), \
  "Expected Quaternion %f %f %f %f , Got Quaternion %f %f %f %f\n", \
      expectedquat.s,U1(expectedquat.v).x,U2(expectedquat.v).y,U3(expectedquat.v).z, \
      gotquat.s,U1(gotquat.v).x,U2(gotquat.v).y,U3(gotquat.v).z);

#define expect_vec(expectedvec,gotvec) \
  ok( ((fabs(U1(expectedvec).x-U1(gotvec).x)<admit_error)&&(fabs(U2(expectedvec).y-U2(gotvec).y)<admit_error)&&(fabs(U3(expectedvec).z-U3(gotvec).z)<admit_error)), \
  "Expected Vector= (%f, %f, %f)\n , Got Vector= (%f, %f, %f)\n", \
  U1(expectedvec).x,U2(expectedvec).y,U3(expectedvec).z, U1(gotvec).x, U2(gotvec).y, U3(gotvec).z);

static HMODULE d3drm_handle = 0;

static void (WINAPI * pD3DRMMatrixFromQuaternion)(D3DRMMATRIX4D, LPD3DRMQUATERNION);
static LPD3DVECTOR (WINAPI* pD3DRMVectorAdd)(LPD3DVECTOR, LPD3DVECTOR, LPD3DVECTOR);
static LPD3DVECTOR (WINAPI* pD3DRMVectorCrossProduct)(LPD3DVECTOR, LPD3DVECTOR, LPD3DVECTOR);
static D3DVALUE (WINAPI* pD3DRMVectorDotProduct)(LPD3DVECTOR, LPD3DVECTOR);
static D3DVALUE (WINAPI* pD3DRMVectorModulus)(LPD3DVECTOR);
static LPD3DVECTOR (WINAPI * pD3DRMVectorNormalize)(LPD3DVECTOR);
static LPD3DVECTOR (WINAPI * pD3DRMVectorReflect)(LPD3DVECTOR, LPD3DVECTOR, LPD3DVECTOR);
static LPD3DVECTOR (WINAPI * pD3DRMVectorRotate)(LPD3DVECTOR, LPD3DVECTOR, LPD3DVECTOR, D3DVALUE);
static LPD3DVECTOR (WINAPI * pD3DRMVectorScale)(LPD3DVECTOR, LPD3DVECTOR, D3DVALUE);
static LPD3DVECTOR (WINAPI * pD3DRMVectorSubtract)(LPD3DVECTOR, LPD3DVECTOR, LPD3DVECTOR);
static LPD3DRMQUATERNION (WINAPI * pD3DRMQuaternionFromRotation)(LPD3DRMQUATERNION, LPD3DVECTOR, D3DVALUE);
static LPD3DRMQUATERNION (WINAPI * pD3DRMQuaternionSlerp)(LPD3DRMQUATERNION, LPD3DRMQUATERNION, LPD3DRMQUATERNION, D3DVALUE);
static D3DVALUE (WINAPI * pD3DRMColorGetAlpha)(D3DCOLOR);
static D3DVALUE (WINAPI * pD3DRMColorGetBlue)(D3DCOLOR);
static D3DVALUE (WINAPI * pD3DRMColorGetGreen)(D3DCOLOR);
static D3DVALUE (WINAPI * pD3DRMColorGetRed)(D3DCOLOR);

#define D3DRM_GET_PROC(func) \
    p ## func = (void*)GetProcAddress(d3drm_handle, #func); \
    if(!p ## func) { \
      trace("GetProcAddress(%s) failed\n", #func); \
      FreeLibrary(d3drm_handle); \
      return FALSE; \
    }

static BOOL InitFunctionPtrs(void)
{
    d3drm_handle = LoadLibraryA("d3drm.dll");

    if(!d3drm_handle)
    {
        skip("Could not load d3drm.dll\n");
        return FALSE;
    }

    D3DRM_GET_PROC(D3DRMMatrixFromQuaternion)
    D3DRM_GET_PROC(D3DRMVectorAdd)
    D3DRM_GET_PROC(D3DRMVectorCrossProduct)
    D3DRM_GET_PROC(D3DRMVectorDotProduct)
    D3DRM_GET_PROC(D3DRMVectorModulus)
    D3DRM_GET_PROC(D3DRMVectorNormalize)
    D3DRM_GET_PROC(D3DRMVectorReflect)
    D3DRM_GET_PROC(D3DRMVectorRotate)
    D3DRM_GET_PROC(D3DRMVectorScale)
    D3DRM_GET_PROC(D3DRMVectorSubtract)
    D3DRM_GET_PROC(D3DRMQuaternionFromRotation)
    D3DRM_GET_PROC(D3DRMQuaternionSlerp)
    D3DRM_GET_PROC(D3DRMColorGetAlpha)
    D3DRM_GET_PROC(D3DRMColorGetBlue)
    D3DRM_GET_PROC(D3DRMColorGetGreen)
    D3DRM_GET_PROC(D3DRMColorGetRed)

    return TRUE;
}


static void VectorTest(void)
{
    D3DVALUE mod,par,theta;
    D3DVECTOR e,r,u,v,w,axis,casnul,norm,ray;

    U1(u).x=2.0;U2(u).y=2.0;U3(u).z=1.0;
    U1(v).x=4.0;U2(v).y=4.0;U3(v).z=0.0;

/*______________________VectorAdd_________________________________*/
    pD3DRMVectorAdd(&r,&u,&v);
    U1(e).x=6.0;U2(e).y=6.0;U3(e).z=1.0;
    expect_vec(e,r);

/*_______________________VectorSubtract__________________________*/
    pD3DRMVectorSubtract(&r,&u,&v);
    U1(e).x=-2.0;U2(e).y=-2.0;U3(e).z=1.0;
    expect_vec(e,r);

/*_______________________VectorCrossProduct_______________________*/
    pD3DRMVectorCrossProduct(&r,&u,&v);
    U1(e).x=-4.0;U2(e).y=4.0;U3(e).z=0.0;
    expect_vec(e,r);

/*_______________________VectorDotProduct__________________________*/
    mod=pD3DRMVectorDotProduct(&u,&v);
    ok((mod == 16.0), "Expected 16.0, Got %f\n",mod);

/*_______________________VectorModulus_____________________________*/
    mod=pD3DRMVectorModulus(&u);
    ok((mod == 3.0), "Expected 3.0, Got %f\n",mod);

/*_______________________VectorNormalize___________________________*/
    pD3DRMVectorNormalize(&u);
    U1(e).x=2.0/3.0;U2(e).y=2.0/3.0;U3(e).z=1.0/3.0;
    expect_vec(e,u);

/* If u is the NULL vector, MSDN says that the return vector is NULL. In fact, the returned vector is (1,0,0). The following test case prove it. */

    U1(casnul).x=0.0; U2(casnul).y=0.0; U3(casnul).z=0.0;
    pD3DRMVectorNormalize(&casnul);
    U1(e).x=1.0; U2(e).y=0.0; U3(e).z=0.0;
    expect_vec(e,casnul);

/*____________________VectorReflect_________________________________*/
    U1(ray).x=3.0; U2(ray).y=-4.0; U3(ray).z=5.0;
    U1(norm).x=1.0; U2(norm).y=-2.0; U3(norm).z=6.0;
    U1(e).x=79.0; U2(e).y=-160.0; U3(e).z=487.0;
    pD3DRMVectorReflect(&r,&ray,&norm);
    expect_vec(e,r);

/*_______________________VectorRotate_______________________________*/
    U1(w).x=3.0; U2(w).y=4.0; U3(w).z=0.0;
    U1(axis).x=0.0; U2(axis).y=0.0; U3(axis).z=1.0;
    theta=2.0*PI/3.0;
    pD3DRMVectorRotate(&r,&w,&axis,theta);
    U1(e).x=-0.3-0.4*sqrt(3.0); U2(e).y=0.3*sqrt(3.0)-0.4; U3(e).z=0.0;
    expect_vec(e,r);

/* The same formula gives D3DRMVectorRotate, for theta in [-PI/2;+PI/2] or not. The following test proves this fact.*/
    theta=-PI/4.0;
    pD3DRMVectorRotate(&r,&w,&axis,-PI/4);
    U1(e).x=1.4/sqrt(2.0); U2(e).y=0.2/sqrt(2.0); U3(e).z=0.0;
    expect_vec(e,r);

/*_______________________VectorScale__________________________*/
    par=2.5;
    pD3DRMVectorScale(&r,&v,par);
    U1(e).x=10.0; U2(e).y=10.0; U3(e).z=0.0;
    expect_vec(e,r);
}

static void MatrixTest(void)
{
    D3DRMQUATERNION q;
    D3DRMMATRIX4D exp,mat;

    exp[0][0]=-49.0; exp[0][1]=4.0;   exp[0][2]=22.0;  exp[0][3]=0.0;
    exp[1][0]=20.0;  exp[1][1]=-39.0; exp[1][2]=20.0;  exp[1][3]=0.0;
    exp[2][0]=10.0;  exp[2][1]=28.0;  exp[2][2]=-25.0; exp[2][3]=0.0;
    exp[3][0]=0.0;   exp[3][1]=0.0;   exp[3][2]=0.0;   exp[3][3]=1.0;
    q.s=1.0; U1(q.v).x=2.0; U2(q.v).y=3.0; U3(q.v).z=4.0;

   pD3DRMMatrixFromQuaternion(mat,&q);
   expect_mat(exp,mat);
}

static void QuaternionTest(void)
{
    D3DVECTOR axis;
    D3DVALUE g,h,epsilon,par,theta;
    D3DRMQUATERNION q,q1,q2,r;

/*_________________QuaternionFromRotation___________________*/
    U1(axis).x=1.0; U2(axis).y=1.0; U3(axis).z=1.0;
    theta=2.0*PI/3.0;
    pD3DRMQuaternionFromRotation(&r,&axis,theta);
    q.s=0.5; U1(q.v).x=0.5; U2(q.v).y=0.5; U3(q.v).z=0.5;
    expect_quat(q,r);

/*_________________QuaternionSlerp_________________________*/
/* Interpolation slerp is in fact a linear interpolation, not a spherical linear
 * interpolation. Moreover, if the angle of the two quaternions is in ]PI/2;3PI/2[, QuaternionSlerp
 * interpolates between the first quaternion and the opposite of the second one. The test proves
 * these two facts. */
    par=0.31;
    q1.s=1.0; U1(q1.v).x=2.0; U2(q1.v).y=3.0; U3(q1.v).z=50.0;
    q2.s=-4.0; U1(q2.v).x=6.0; U2(q2.v).y=7.0; U3(q2.v).z=8.0;
/* The angle between q1 and q2 is in [-PI/2,PI/2]. So, one interpolates between q1 and q2. */
    epsilon=1.0;
    g=1.0-par; h=epsilon*par;
/* Part of the test proving that the interpolation is linear. */
    q.s=g*q1.s+h*q2.s;
    U1(q.v).x=g*U1(q1.v).x+h*U1(q2.v).x;
    U2(q.v).y=g*U2(q1.v).y+h*U2(q2.v).y;
    U3(q.v).z=g*U3(q1.v).z+h*U3(q2.v).z;
    pD3DRMQuaternionSlerp(&r,&q1,&q2,par);
    expect_quat(q,r);

    q1.s=1.0; U1(q1.v).x=2.0; U2(q1.v).y=3.0; U3(q1.v).z=50.0;
    q2.s=-94.0; U1(q2.v).x=6.0; U2(q2.v).y=7.0; U3(q2.v).z=-8.0;
/* The angle between q1 and q2 is not in [-PI/2,PI/2]. So, one interpolates between q1 and -q2. */
    epsilon=-1.0;
    g=1.0-par; h=epsilon*par;
    q.s=g*q1.s+h*q2.s;
    U1(q.v).x=g*U1(q1.v).x+h*U1(q2.v).x;
    U2(q.v).y=g*U2(q1.v).y+h*U2(q2.v).y;
    U3(q.v).z=g*U3(q1.v).z+h*U3(q2.v).z;
    pD3DRMQuaternionSlerp(&r,&q1,&q2,par);
    expect_quat(q,r);
}

static void ColorTest(void)
{
    D3DCOLOR color;
    D3DVALUE expected, got;

/*___________D3DRMColorGetAlpha_________________________*/
    color=0x0e4921bf;
    expected=14.0/255.0;
    got=pD3DRMColorGetAlpha(color);
    ok((fabs(expected-got)<admit_error),"Expected=%f, Got=%f\n",expected,got);

/*___________D3DRMColorGetBlue__________________________*/
    color=0xc82a1455;
    expected=1.0/3.0;
    got=pD3DRMColorGetBlue(color);
    ok((fabs(expected-got)<admit_error),"Expected=%f, Got=%f\n",expected,got);

/*___________D3DRMColorGetGreen_________________________*/
    color=0xad971203;
    expected=6.0/85.0;
    got=pD3DRMColorGetGreen(color);
    ok((fabs(expected-got)<admit_error),"Expected=%f, Got=%f\n",expected,got);

/*___________D3DRMColorGetRed__________________________*/
    color=0xb62d7a1c;
    expected=3.0/17.0;
    got=pD3DRMColorGetRed(color);
    ok((fabs(expected-got)<admit_error),"Expected=%f, Got=%f\n",expected,got);
}

START_TEST(vector)
{
    if(!InitFunctionPtrs())
        return;

    VectorTest();
    MatrixTest();
    QuaternionTest();
    ColorTest();

    FreeLibrary(d3drm_handle);
}
