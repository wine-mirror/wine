/*
 * comctl32 month calendar unit tests
 *
 * Copyright (C) 2006 Vitaliy Margolen
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"

#include "commctrl.h"

#include "wine/test.h"

void test_monthcal(void)
{
    HWND hwnd;
    SYSTEMTIME st[2];
    INITCOMMONCONTROLSEX ic = {sizeof(INITCOMMONCONTROLSEX), ICC_DATE_CLASSES};

    InitCommonControlsEx(&ic);
    hwnd = CreateWindowA(MONTHCAL_CLASSA, "MonthCal", WS_POPUP | WS_VISIBLE, CW_USEDEFAULT,
                         0, 300, 300, 0, 0, NULL, NULL);
    ok(hwnd != NULL, "Failed to create MonthCal\n");
    GetSystemTime(&st[0]);
    st[0].wMonth = 5;
    st[1] = st[0];

    st[1].wMonth--;
    ok(SendMessage(hwnd, MCM_SETRANGE, GDTR_MIN | GDTR_MAX, (LPARAM)st), "Failed to set both min and max limits\n");
    st[1].wMonth += 2;
    ok(SendMessage(hwnd, MCM_SETRANGE, GDTR_MIN | GDTR_MAX, (LPARAM)st), "Failed to set both min and max limits\n");
    st[1].wYear --;
    ok(SendMessage(hwnd, MCM_SETRANGE, GDTR_MIN | GDTR_MAX, (LPARAM)st), "Failed to set both min and max limits\n");
    st[1].wYear += 1;
    ok(SendMessage(hwnd, MCM_SETRANGE, GDTR_MIN | GDTR_MAX, (LPARAM)st), "Failed to set both min and max limits\n");

/* This crashing Wine so commented untill fixed.
    st[1].wMonth -= 3;
    ok(SendMessage(hwnd, MCM_SETRANGE, GDTR_MAX, (LPARAM)st), "Failed to set max limit\n");
*/
    st[1].wMonth += 4;
    ok(SendMessage(hwnd, MCM_SETRANGE, GDTR_MAX, (LPARAM)st), "Failed to set max limit\n");
    st[1].wYear -= 3;
    ok(SendMessage(hwnd, MCM_SETRANGE, GDTR_MAX, (LPARAM)st), "Failed to set max limit\n");
    st[1].wYear += 4;
    ok(SendMessage(hwnd, MCM_SETRANGE, GDTR_MAX, (LPARAM)st), "Failed to set max limit\n");
}

START_TEST(monthcal)
{
    test_monthcal();
}
