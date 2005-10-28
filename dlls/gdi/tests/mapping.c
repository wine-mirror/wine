/*
 * Unit tests for mapping functions
 *
 * Copyright (c) 2005 Huw Davies
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <assert.h>
#include <stdio.h>
#include <math.h>

#include "wine/test.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"


void test_modify_world_transform(void)
{
    HDC hdc = GetDC(0);
    int ret;

    ret = SetGraphicsMode(hdc, GM_ADVANCED);
    if(!ret) /* running in win9x so quit */
    {
        ReleaseDC(0, hdc);
        return;
    }

    ret = ModifyWorldTransform(hdc, NULL, MWT_IDENTITY);
    ok(ret, "ret = %d\n", ret);

    ret = ModifyWorldTransform(hdc, NULL, MWT_LEFTMULTIPLY);
    ok(!ret, "ret = %d\n", ret);

    ret = ModifyWorldTransform(hdc, NULL, MWT_RIGHTMULTIPLY);
    ok(!ret, "ret = %d\n", ret);

    ReleaseDC(0, hdc);
}

START_TEST(mapping)
{
    test_modify_world_transform();
}
