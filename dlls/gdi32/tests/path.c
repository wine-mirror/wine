/*
 * Unit test suite for paths
 *
 * Copyright 2007 Laurent Vromman
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

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"

#include "wine/test.h"

#include "winuser.h"
#include "winerror.h"

static void test_widenpath(void)
{
    HDC hdc = GetDC(0);
    HPEN greenPen;
    HPEN oldPen;
    POINT pnt[6];
    INT nSize;

    /* Create a pen to be used in WidenPath */
    greenPen = CreatePen(PS_SOLID, 10, RGB(0,0,0));
    oldPen = SelectObject(hdc, greenPen);

    /* Prepare a path */
    pnt[0].x = 100;
    pnt[0].y = 0;
    pnt[1].x = 200;
    pnt[1].y = 0;
    pnt[2].x = 300;
    pnt[2].y = 100;
    pnt[3].x = 300;
    pnt[3].y = 200;
    pnt[4].x = 200;
    pnt[4].y = 300;
    pnt[5].x = 100;
    pnt[5].y = 300;

    /* Set a polyline path */
    BeginPath(hdc);
    Polyline(hdc, pnt, 6);
    EndPath(hdc);

    /* Widen the polyline path */
    ok(WidenPath(hdc), "WidenPath fails while widening a poyline path.\n");

    /* Test if WidenPath seems to have done his job */
    nSize = GetPath(hdc, NULL, NULL, 0);
    ok(nSize != -1, "GetPath fails after calling WidenPath.\n");
    ok(nSize > 6, "Path number of points is to low. Should be more than 6 but is %d\n", nSize);

    todo_wine {
        /* Test WidenPath with an empty path */
        SetLastError(0xdeadbeef);
        BeginPath(hdc);
        ok(WidenPath(hdc) == FALSE, "WidenPath fails while widening an empty path. Error : %d\n", GetLastError());
    }

    ReleaseDC(0, hdc);
    return;
}

START_TEST(path)
{
    test_widenpath();
}
