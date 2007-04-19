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

#define expect_vec(expectedvec,gotvec) \
  ok( ((fabs(expectedvec.x-gotvec.x)<admit_error)&&(fabs(expectedvec.y-gotvec.y)<admit_error)&&(fabs(expectedvec.z-gotvec.z)<admit_error)), \
  "Expected Vector= (%f, %f, %f)\n , Got Vector= (%f, %f, %f)\n", \
  expectedvec.x,expectedvec.y,expectedvec.z, gotvec.x, gotvec.y, gotvec.z);

void VectorTest(void)
{
    D3DVECTOR e,r,u,v;

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
}

START_TEST(vector)
{
    VectorTest();
}
