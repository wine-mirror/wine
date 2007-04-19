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
#include "wine/test.h"
#include "d3drmdef.h"
#include <math.h>

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
  ok( (fabs(expectedquat.v.x-gotquat.v.x)<admit_error) && \
      (fabs(expectedquat.v.y-gotquat.v.y)<admit_error) && \
      (fabs(expectedquat.v.z-gotquat.v.z)<admit_error) && \
      (fabs(expectedquat.s-gotquat.s)<admit_error), \
  "Expected Quaternion %f %f %f %f , Got Quaternion %f %f %f %f\n", \
  expectedquat.s,expectedquat.v.x,expectedquat.v.y,expectedquat.v.z, \
  gotquat.s,gotquat.v.x,gotquat.v.y,gotquat.v.z);

#define expect_vec(expectedvec,gotvec) \
  ok( ((fabs(expectedvec.x-gotvec.x)<admit_error)&&(fabs(expectedvec.y-gotvec.y)<admit_error)&&(fabs(expectedvec.z-gotvec.z)<admit_error)), \
  "Expected Vector= (%f, %f, %f)\n , Got Vector= (%f, %f, %f)\n", \
  expectedvec.x,expectedvec.y,expectedvec.z, gotvec.x, gotvec.y, gotvec.z);

static void VectorTest(void)
{
    D3DVALUE mod,par,theta;
    D3DVECTOR e,r,u,v,w,axis,casnul,norm,ray;

    u.x=2.0;u.y=2.0;u.z=1.0;
    v.x=4.0;v.y=4.0;v.z=0.0;

/*______________________VectorAdd_________________________________*/
    D3DRMVectorAdd(&r,&u,&v);
    e.x=6.0;e.y=6.0;e.z=1.0;
    expect_vec(e,r);

/*_______________________VectorSubtract__________________________*/
    D3DRMVectorSubtract(&r,&u,&v);
    e.x=-2.0;e.y=-2.0;e.z=1.0;
    expect_vec(e,r);

/*_______________________VectorCrossProduct_______________________*/
    D3DRMVectorCrossProduct(&r,&u,&v);
    e.x=-4.0;e.y=4.0;e.z=0.0;
    expect_vec(e,r);

/*_______________________VectorDotProduct__________________________*/
    mod=D3DRMVectorDotProduct(&u,&v);
    ok((mod == 16.0), "Expected 16.0, Got %f",mod);

/*_______________________VectorModulus_____________________________*/
    mod=D3DRMVectorModulus(&u);
    ok((mod == 3.0), "Expected 3.0, Got %f",mod);

/*_______________________VectorNormalize___________________________*/
    D3DRMVectorNormalize(&u);
    e.x=2.0/3.0;e.y=2.0/3.0;e.z=1.0/3.0;
    expect_vec(e,u);

/* If u is the NULL vector, MSDN says that the return vector is NULL. In fact, the returned vector is (1,0,0). The following test case prove it. */

    casnul.x=0.0; casnul.y=0.0; casnul.z=0.0;
    D3DRMVectorNormalize(&casnul);
    e.x=1.0; e.y=0.0; e.z=0.0;
    expect_vec(e,casnul);

/*____________________VectorReflect_________________________________*/
    ray.x=3.0; ray.y=-4.0; ray.z=5.0;
    norm.x=1.0; norm.y=-2.0; norm.z=6.0;
    e.x=79.0; e.y=-160.0; e.z=487.0;
    D3DRMVectorReflect(&r,&ray,&norm);
    expect_vec(e,r);

/*_______________________VectorRotate_______________________________*/
    w.x=3.0;w.y=4.0;w.z=0.0;
    axis.x=0.0;axis.y=0.0;axis.z=1.0;
    theta=2.0*PI/3.0;
    D3DRMVectorRotate(&r,&w,&axis,theta);
    e.x=-0.3-0.4*sqrt(3.0); e.y=0.3*sqrt(3.0)-0.4; e.z=0.0;
    expect_vec(e,r);

/* The same formula gives D3DRMVectorRotate, for theta in [-PI/2;+PI/2] or not. The following test proves this fact.*/
    theta=-PI/4.0;
    D3DRMVectorRotate(&r,&w,&axis,-PI/4);
    e.x=1.4/sqrt(2.0); e.y=0.2/sqrt(2.0); e.z=0.0;
    expect_vec(e,r);

/*_______________________VectorScale__________________________*/
    par=2.5;
    D3DRMVectorScale(&r,&v,par);
    e.x=10.0; e.y=10.0; e.z=0.0;
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
    q.s=1.0; q.v.x=2.0; q.v.y=3.0; q.v.z=4.0;

   D3DRMMatrixFromQuaternion(mat,&q);
   expect_mat(exp,mat);
}

static void QuaternionTest(void)
{
    D3DVECTOR axis;
    D3DVALUE theta;
    D3DRMQUATERNION q,r;

/*_________________QuaternionFromRotation___________________*/
    axis.x=1.0;axis.y=1.0;axis.z=1.0;
    theta=2.0*PI/3.0;
    D3DRMQuaternionFromRotation(&r,&axis,theta);
    q.s=0.5;q.v.x=0.5;q.v.y=0.5;q.v.z=0.5;
    expect_quat(q,r);
}

START_TEST(vector)
{
    VectorTest();
    MatrixTest();
    QuaternionTest();
}
