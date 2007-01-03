/* Unit test suite for status control.
 *
 * Copyright 2007 Google (Lei Zhang)
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
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>

#include "wine/test.h"

static HINSTANCE hinst;

static HWND create_status_control(DWORD style, DWORD exstyle)
{
    HWND hWndStatus;

    /* make the control */
    hWndStatus = CreateWindowEx(0, STATUSCLASSNAME, NULL, style,
        /* placement */
        0, 0, 300, 20,
        /* parent, etc */
        NULL, NULL, hinst, NULL);
    assert (hWndStatus);
    return hWndStatus;
}

static void test_status_control(void)
{
    HWND hWndStatus;
    int r;
    int nParts[] = {50, 150, -1};
    RECT rc;

    hWndStatus = create_status_control(0, 0);
    r = SendMessage(hWndStatus, SB_SETPARTS, 3, (long)nParts);
    ok(r == TRUE, "Expected TRUE, got %d\n", r);
    r = SendMessage(hWndStatus, SB_SETTEXT, 0, (LPARAM)"First");
    ok(r == TRUE, "Expected TRUE, got %d\n", r);
    r = SendMessage(hWndStatus, SB_SETTEXT, 1, (LPARAM)"Second");
    ok(r == TRUE, "Expected TRUE, got %d\n", r);
    r = SendMessage(hWndStatus, SB_SETTEXT, 2, (LPARAM)"Third");
    ok(r == TRUE, "Expected TRUE, got %d\n", r);

    r = SendMessage(hWndStatus, SB_GETRECT, 0, (LPARAM)&rc);
    ok(r == TRUE, "Expected TRUE, got %d\n", r);
    ok(rc.top == 2, "Expected 2, got %d\n", rc.top);
    ok(rc.bottom == 21, "Expected 21, got %d\n", rc.bottom);
    ok(rc.left == 0, "Expected 0, got %d\n", rc.left);
    ok(rc.right == 50, "Expected 50, got %d\n", rc.right);

    r = SendMessage(hWndStatus, SB_GETRECT, -1, (LPARAM)&rc);
    ok(r == FALSE, "Expected FALSE, got %d\n", r);
    r = SendMessage(hWndStatus, SB_GETRECT, 5, (LPARAM)&rc);
    ok(r == FALSE, "Expected FALSE, got %d\n", r);

    DestroyWindow(hWndStatus);
}

START_TEST(status)
{
    hinst = GetModuleHandleA(NULL);

    InitCommonControls();

    test_status_control();
}
