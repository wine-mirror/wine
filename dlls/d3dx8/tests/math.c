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

static void D3X8Vector2Test(void)
{
    D3DXVECTOR2 u;
    FLOAT expected, got;

    u.x=3.0f; u.y=4.0f;

/*_______________D3DXVec2Length__________________________*/
   expected = 5.0f;
   got = D3DXVec2Length(&u);
   ok(fabs( got - expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);
   /* Tests the case NULL */
    expected=0.0f;
    got= D3DXVec2Length(NULL);
    ok(fabs( got - expected ) < admitted_error, "Expected: %f, Got: %f\n", expected, got);
}
START_TEST(math)
{
    D3X8Vector2Test();
}
