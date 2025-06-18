/*
 * Explorer.exe tests
 *
 * Copyright 2022 Zhiyi Zhang for CodeWeavers
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

#include <windows.h>
#include "wine/test.h"

static void test_taskbar(void)
{
    RECT taskbar_rect, expected_rect, primary_rect, work_rect;
    HWND hwnd;
    BOOL ret;

    hwnd = FindWindowA("Shell_TrayWnd", NULL);
    if (!hwnd)
    {
        skip("Failed to find the taskbar window.\n");
        return;
    }

    ret = GetWindowRect(hwnd, &taskbar_rect);
    ok(ret, "GetWindowRect failed, error %ld.\n", GetLastError());

    SystemParametersInfoW(SPI_GETWORKAREA, 0, &work_rect, 0);
    SetRect(&primary_rect, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
    SubtractRect(&expected_rect, &primary_rect, &work_rect);

    /* In standalone mode, the systray window is floating */
    todo_wine_if(!(GetWindowLongW(hwnd, GWL_STYLE) & WS_POPUP))
    ok(EqualRect(&taskbar_rect, &expected_rect), "Expected %s, got %s.\n",
       wine_dbgstr_rect(&expected_rect), wine_dbgstr_rect(&taskbar_rect));
}

START_TEST(explorer)
{
    test_taskbar();
}
