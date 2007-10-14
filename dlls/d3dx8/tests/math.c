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

#define expect_vec(expectedvec,gotvec) ok((fabs(expectedvec.x-gotvec.x)<admitted_error)&&(fabs(expectedvec.y-gotvec.y)<admitted_error),"Expected Vector= (%f, %f)\n , Got Vector= (%f, %f)\n", expectedvec.x, expectedvec.y, gotvec.x, gotvec.y);

static void D3X8Vector2Test(void)
{
    D3DXVECTOR2 expectedvec, gotvec, u, v;
    LPD3DXVECTOR2 funcpointer;
    FLOAT expected, got;

    u.x=3.0f; u.y=4.0f;
    v.x=-7.0f; v.y=9.0f;

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

START_TEST(math)
{
    D3X8Vector2Test();
}
