/* Unit tests for appbars
 *
 * Copyright 2008 Vincent Povirk for CodeWeavers
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
#include <stdarg.h>

#include <windows.h>

#include "wine/test.h"

#define MSG_APPBAR WM_APP

static const WCHAR testwindow_class[] = {'t','e','s','t','w','i','n','d','o','w',0};

static HMONITOR (WINAPI *pMonitorFromWindow)(HWND, DWORD);

struct testwindow_info
{
    RECT desired_rect;
    UINT edge;
    RECT allocated_rect;
};

static void testwindow_setpos(HWND hwnd)
{
    struct testwindow_info* info = (struct testwindow_info*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    APPBARDATA abd;
    BOOL ret;

    ok(info != NULL, "got unexpected ABN_POSCHANGED notification\n");

    if (!info)
    {
        return;
    }

    abd.cbSize = sizeof(abd);
    abd.hWnd = hwnd;
    abd.uEdge = info->edge;
    abd.rc = info->desired_rect;
    ret = SHAppBarMessage(ABM_QUERYPOS, &abd);
    ok(ret == TRUE, "SHAppBarMessage returned %i\n", ret);
    switch (info->edge)
    {
        case ABE_BOTTOM:
            ok(info->desired_rect.top == abd.rc.top, "ABM_QUERYPOS changed top of rect from %i to %i\n", info->desired_rect.top, abd.rc.top);
            abd.rc.top = abd.rc.bottom - (info->desired_rect.bottom - info->desired_rect.top);
            break;
        case ABE_LEFT:
            ok(info->desired_rect.right == abd.rc.right, "ABM_QUERYPOS changed right of rect from %i to %i\n", info->desired_rect.top, abd.rc.top);
            abd.rc.right = abd.rc.left + (info->desired_rect.right - info->desired_rect.left);
            break;
        case ABE_RIGHT:
            ok(info->desired_rect.left == abd.rc.left, "ABM_QUERYPOS changed left of rect from %i to %i\n", info->desired_rect.top, abd.rc.top);
            abd.rc.left = abd.rc.right - (info->desired_rect.right - info->desired_rect.left);
            break;
        case ABE_TOP:
            ok(info->desired_rect.bottom == abd.rc.bottom, "ABM_QUERYPOS changed bottom of rect from %i to %i\n", info->desired_rect.top, abd.rc.top);
            abd.rc.bottom = abd.rc.top + (info->desired_rect.bottom - info->desired_rect.top);
            break;
    }

    ret = SHAppBarMessage(ABM_SETPOS, &abd);
    ok(ret == TRUE, "SHAppBarMessage returned %i\n", ret);

    info->allocated_rect = abd.rc;
    MoveWindow(hwnd, abd.rc.left, abd.rc.top, abd.rc.right-abd.rc.left, abd.rc.bottom-abd.rc.top, TRUE);
}

static LRESULT CALLBACK testwindow_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch(msg)
    {
        case MSG_APPBAR:
        {
            switch(wparam)
            {
                case ABN_POSCHANGED:
                    testwindow_setpos(hwnd);
                    break;
            }
            return 0;
        }
    }

    return DefWindowProc(hwnd, msg, wparam, lparam);
}

/* process any pending messages */
static void do_events(void)
{
    MSG msg;

    while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

static void register_testwindow_class(void)
{
    WNDCLASSEXW cls;

    ZeroMemory(&cls, sizeof(cls));
    cls.cbSize = sizeof(cls);
    cls.style = 0;
    cls.lpfnWndProc = testwindow_wndproc;
    cls.hInstance = NULL;
    cls.hCursor = LoadCursor(0, IDC_ARROW);
    cls.hbrBackground = (HBRUSH) COLOR_WINDOW;
    cls.lpszClassName = testwindow_class;

    RegisterClassExW(&cls);
}

static void test_setpos(void)
{
    APPBARDATA abd;
    RECT rc;
    int screen_width, screen_height, expected;
    HWND window1, window2, window3;
    struct testwindow_info window1_info, window2_info, window3_info;
    BOOL ret;

    screen_width = GetSystemMetrics(SM_CXSCREEN);
    screen_height = GetSystemMetrics(SM_CYSCREEN);

    /* create and register window1 */
    window1 = CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_TOPMOST,
        testwindow_class, testwindow_class, WS_POPUP|WS_VISIBLE, 0, 0, 0, 0,
        NULL, NULL, NULL, NULL);
    ok(window1 != NULL, "couldn't create window\n");
    do_events();
    abd.cbSize = sizeof(abd);
    abd.hWnd = window1;
    abd.uCallbackMessage = MSG_APPBAR;
    ret = SHAppBarMessage(ABM_NEW, &abd);
    ok(ret == TRUE, "SHAppBarMessage returned %i\n", ret);

    /* ABM_NEW should return FALSE if the window is already registered */
    ret = SHAppBarMessage(ABM_NEW, &abd);
    todo_wine ok(ret == FALSE, "SHAppBarMessage returned %i\n", ret);
    do_events();

    /* dock window1 to the bottom of the screen */
    window1_info.edge = ABE_BOTTOM;
    window1_info.desired_rect.left = 0;
    window1_info.desired_rect.right = screen_width;
    window1_info.desired_rect.top = screen_height - 15;
    window1_info.desired_rect.bottom = screen_height;
    SetWindowLongPtr(window1, GWLP_USERDATA, (LONG_PTR)&window1_info);
    testwindow_setpos(window1);
    do_events();

    /* create and register window2 */
    window2 = CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_TOPMOST,
        testwindow_class, testwindow_class, WS_POPUP|WS_VISIBLE, 0, 0, 0, 0,
        NULL, NULL, NULL, NULL);
    ok(window2 != NULL, "couldn't create window\n");
    abd.hWnd = window2;
    ret = SHAppBarMessage(ABM_NEW, &abd);
    ok(ret == TRUE, "SHAppBarMessage returned %i\n", ret);

    /* dock window2 to the bottom of the screen */
    window2_info.edge = ABE_BOTTOM;
    window2_info.desired_rect.left = 0;
    window2_info.desired_rect.right = screen_width;
    window2_info.desired_rect.top = screen_height - 10;
    window2_info.desired_rect.bottom = screen_height;
    SetWindowLongPtr(window2, GWLP_USERDATA, (LONG_PTR)&window2_info);
    testwindow_setpos(window2);
    do_events();

    /* the windows are adjusted to they don't overlap */
    todo_wine ok(!IntersectRect(&rc, &window1_info.allocated_rect, &window2_info.allocated_rect),
        "rectangles intersect (%i,%i,%i,%i)/(%i,%i,%i,%i)\n",
        window1_info.allocated_rect.left, window1_info.allocated_rect.top, window1_info.allocated_rect.right, window1_info.allocated_rect.bottom,
        window2_info.allocated_rect.left, window2_info.allocated_rect.top, window2_info.allocated_rect.right, window2_info.allocated_rect.bottom);

    /* make window1 larger, forcing window2 to move out of its way */
    window1_info.desired_rect.top = screen_height - 20;
    testwindow_setpos(window1);
    do_events();
    todo_wine ok(!IntersectRect(&rc, &window1_info.allocated_rect, &window2_info.allocated_rect),
        "rectangles intersect (%i,%i,%i,%i)/(%i,%i,%i,%i)\n",
        window1_info.allocated_rect.left, window1_info.allocated_rect.top, window1_info.allocated_rect.right, window1_info.allocated_rect.bottom,
        window2_info.allocated_rect.left, window2_info.allocated_rect.top, window2_info.allocated_rect.right, window2_info.allocated_rect.bottom);

    /* create and register window3 */
    window3 = CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_TOPMOST,
        testwindow_class, testwindow_class, WS_POPUP|WS_VISIBLE, 0, 0, 0, 0,
        NULL, NULL, NULL, NULL);
    ok(window3 != NULL, "couldn't create window\n");
    do_events();

    abd.hWnd = window3;
    ret = SHAppBarMessage(ABM_NEW, &abd);
    ok(ret == TRUE, "SHAppBarMessage returned %i\n", ret);

    /* dock window3 to the bottom of the screen */
    window3_info.edge = ABE_BOTTOM;
    window3_info.desired_rect.left = 0;
    window3_info.desired_rect.right = screen_width;
    window3_info.desired_rect.top = screen_height - 10;
    window3_info.desired_rect.bottom = screen_height;
    SetWindowLongPtr(window3, GWLP_USERDATA, (LONG_PTR)&window3_info);
    testwindow_setpos(window3);
    do_events();

    todo_wine ok(!IntersectRect(&rc, &window1_info.allocated_rect, &window2_info.allocated_rect),
        "rectangles intersect (%i,%i,%i,%i)/(%i,%i,%i,%i)\n",
        window1_info.allocated_rect.left, window1_info.allocated_rect.top, window1_info.allocated_rect.right, window1_info.allocated_rect.bottom,
        window2_info.allocated_rect.left, window2_info.allocated_rect.top, window2_info.allocated_rect.right, window2_info.allocated_rect.bottom);
    todo_wine ok(!IntersectRect(&rc, &window1_info.allocated_rect, &window3_info.allocated_rect),
        "rectangles intersect (%i,%i,%i,%i)/(%i,%i,%i,%i)\n",
        window1_info.allocated_rect.left, window1_info.allocated_rect.top, window1_info.allocated_rect.right, window1_info.allocated_rect.bottom,
        window3_info.allocated_rect.left, window3_info.allocated_rect.top, window3_info.allocated_rect.right, window3_info.allocated_rect.bottom);
    todo_wine ok(!IntersectRect(&rc, &window3_info.allocated_rect, &window2_info.allocated_rect),
        "rectangles intersect (%i,%i,%i,%i)/(%i,%i,%i,%i)\n",
        window3_info.allocated_rect.left, window3_info.allocated_rect.top, window3_info.allocated_rect.right, window3_info.allocated_rect.bottom,
        window2_info.allocated_rect.left, window2_info.allocated_rect.top, window2_info.allocated_rect.right, window2_info.allocated_rect.bottom);

    /* move window3 to the right side of the screen */
    window3_info.edge = ABE_RIGHT;
    window3_info.desired_rect.left = screen_width - 15;
    window3_info.desired_rect.right = screen_width;
    window3_info.desired_rect.top = 0;
    window3_info.desired_rect.bottom = screen_height;
    testwindow_setpos(window3);
    do_events();

    todo_wine ok(!IntersectRect(&rc, &window1_info.allocated_rect, &window2_info.allocated_rect),
        "rectangles intersect (%i,%i,%i,%i)/(%i,%i,%i,%i)\n",
        window1_info.allocated_rect.left, window1_info.allocated_rect.top, window1_info.allocated_rect.right, window1_info.allocated_rect.bottom,
        window2_info.allocated_rect.left, window2_info.allocated_rect.top, window2_info.allocated_rect.right, window2_info.allocated_rect.bottom);
    todo_wine ok(!IntersectRect(&rc, &window1_info.allocated_rect, &window3_info.allocated_rect),
        "rectangles intersect (%i,%i,%i,%i)/(%i,%i,%i,%i)\n",
        window1_info.allocated_rect.left, window1_info.allocated_rect.top, window1_info.allocated_rect.right, window1_info.allocated_rect.bottom,
        window3_info.allocated_rect.left, window3_info.allocated_rect.top, window3_info.allocated_rect.right, window3_info.allocated_rect.bottom);
    todo_wine ok(!IntersectRect(&rc, &window3_info.allocated_rect, &window2_info.allocated_rect),
        "rectangles intersect (%i,%i,%i,%i)/(%i,%i,%i,%i)\n",
        window3_info.allocated_rect.left, window3_info.allocated_rect.top, window3_info.allocated_rect.right, window3_info.allocated_rect.bottom,
        window2_info.allocated_rect.left, window2_info.allocated_rect.top, window2_info.allocated_rect.right, window2_info.allocated_rect.bottom);

    /* move window2 to the top of the screen */
    window2_info.edge = ABE_TOP;
    window2_info.desired_rect.left = 0;
    window2_info.desired_rect.right = screen_width;
    window2_info.desired_rect.top = 0;
    window2_info.desired_rect.bottom = 15;
    testwindow_setpos(window2);
    do_events();

    ok(!IntersectRect(&rc, &window1_info.allocated_rect, &window2_info.allocated_rect),
        "rectangles intersect (%i,%i,%i,%i)/(%i,%i,%i,%i)\n",
        window1_info.allocated_rect.left, window1_info.allocated_rect.top, window1_info.allocated_rect.right, window1_info.allocated_rect.bottom,
        window2_info.allocated_rect.left, window2_info.allocated_rect.top, window2_info.allocated_rect.right, window2_info.allocated_rect.bottom);
    todo_wine ok(!IntersectRect(&rc, &window1_info.allocated_rect, &window3_info.allocated_rect),
        "rectangles intersect (%i,%i,%i,%i)/(%i,%i,%i,%i)\n",
        window1_info.allocated_rect.left, window1_info.allocated_rect.top, window1_info.allocated_rect.right, window1_info.allocated_rect.bottom,
        window3_info.allocated_rect.left, window3_info.allocated_rect.top, window3_info.allocated_rect.right, window3_info.allocated_rect.bottom);
    todo_wine ok(!IntersectRect(&rc, &window3_info.allocated_rect, &window2_info.allocated_rect),
        "rectangles intersect (%i,%i,%i,%i)/(%i,%i,%i,%i)\n",
        window3_info.allocated_rect.left, window3_info.allocated_rect.top, window3_info.allocated_rect.right, window3_info.allocated_rect.bottom,
        window2_info.allocated_rect.left, window2_info.allocated_rect.top, window2_info.allocated_rect.right, window2_info.allocated_rect.bottom);

    /* move window2 back to the bottom of the screen */
    window2_info.edge = ABE_BOTTOM;
    window2_info.desired_rect.left = 0;
    window2_info.desired_rect.right = screen_width;
    window2_info.desired_rect.top = screen_height - 10;
    window2_info.desired_rect.bottom = screen_height;
    testwindow_setpos(window2);
    do_events();

    todo_wine ok(!IntersectRect(&rc, &window1_info.allocated_rect, &window2_info.allocated_rect),
        "rectangles intersect (%i,%i,%i,%i)/(%i,%i,%i,%i)\n",
        window1_info.allocated_rect.left, window1_info.allocated_rect.top, window1_info.allocated_rect.right, window1_info.allocated_rect.bottom,
        window2_info.allocated_rect.left, window2_info.allocated_rect.top, window2_info.allocated_rect.right, window2_info.allocated_rect.bottom);
    todo_wine ok(!IntersectRect(&rc, &window1_info.allocated_rect, &window3_info.allocated_rect),
        "rectangles intersect (%i,%i,%i,%i)/(%i,%i,%i,%i)\n",
        window1_info.allocated_rect.left, window1_info.allocated_rect.top, window1_info.allocated_rect.right, window1_info.allocated_rect.bottom,
        window3_info.allocated_rect.left, window3_info.allocated_rect.top, window3_info.allocated_rect.right, window3_info.allocated_rect.bottom);
    todo_wine ok(!IntersectRect(&rc, &window3_info.allocated_rect, &window2_info.allocated_rect),
        "rectangles intersect (%i,%i,%i,%i)/(%i,%i,%i,%i)\n",
        window3_info.allocated_rect.left, window3_info.allocated_rect.top, window3_info.allocated_rect.right, window3_info.allocated_rect.bottom,
        window2_info.allocated_rect.left, window2_info.allocated_rect.top, window2_info.allocated_rect.right, window2_info.allocated_rect.bottom);

    /* removing window1 will cause window2 to move down into its space */
    expected = max(window1_info.allocated_rect.bottom, window2_info.allocated_rect.bottom);

    SetWindowLongPtr(window1, GWLP_USERDATA, 0); /* don't expect further ABN_POSCHANGED notifications */
    abd.hWnd = window1;
    ret = SHAppBarMessage(ABM_REMOVE, &abd);
    ok(ret == TRUE, "SHAppBarMessage returned %i\n", ret);
    do_events();
    DestroyWindow(window1);

    ok(window2_info.allocated_rect.bottom = expected, "window2's bottom is %i, expected %i\n", window2_info.allocated_rect.bottom, expected);

    todo_wine ok(!IntersectRect(&rc, &window3_info.allocated_rect, &window2_info.allocated_rect),
        "rectangles intersect (%i,%i,%i,%i)/(%i,%i,%i,%i)\n",
        window3_info.allocated_rect.left, window3_info.allocated_rect.top, window3_info.allocated_rect.right, window3_info.allocated_rect.bottom,
        window2_info.allocated_rect.left, window2_info.allocated_rect.top, window2_info.allocated_rect.right, window2_info.allocated_rect.bottom);

    /* remove the other windows */
    SetWindowLongPtr(window2, GWLP_USERDATA, 0);
    abd.hWnd = window2;
    ret = SHAppBarMessage(ABM_REMOVE, &abd);
    ok(ret == TRUE, "SHAppBarMessage returned %i\n", ret);
    do_events();
    DestroyWindow(window2);

    SetWindowLongPtr(window3, GWLP_USERDATA, 0);
    abd.hWnd = window3;
    ret = SHAppBarMessage(ABM_REMOVE, &abd);
    ok(ret == TRUE, "SHAppBarMessage returned %i\n", ret);
    do_events();
    DestroyWindow(window3);
}

static void test_appbarget(void)
{
    APPBARDATA abd;
    HWND hwnd, foregnd;
    UINT_PTR ret;

    memset(&abd, 0xcc, sizeof(abd));
    abd.cbSize = sizeof(abd);
    abd.uEdge = ABE_BOTTOM;

    hwnd = (HWND)SHAppBarMessage(ABM_GETAUTOHIDEBAR, &abd);
    ok(hwnd == NULL || IsWindow(hwnd), "ret %p which is not a window\n", hwnd);
    ok(abd.hWnd == (HWND)0xcccccccc, "hWnd overwritten\n");

    if (!pMonitorFromWindow)
    {
        skip("MonitorFromWindow is not available\n");
    }
    else
    {
        /* Presumably one can pass a hwnd with ABM_GETAUTOHIDEBAR to specify a monitor.
           Pass the foreground window and check */
        foregnd = GetForegroundWindow();
        if(foregnd)
        {
            abd.hWnd = foregnd;
            hwnd = (HWND)SHAppBarMessage(ABM_GETAUTOHIDEBAR, &abd);
            ok(hwnd == NULL || IsWindow(hwnd), "ret %p which is not a window\n", hwnd);
            ok(abd.hWnd == foregnd, "hWnd overwritten\n");
            if(hwnd)
            {
                HMONITOR appbar_mon, foregnd_mon;
                appbar_mon = pMonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
                foregnd_mon = pMonitorFromWindow(foregnd, MONITOR_DEFAULTTONEAREST);
                ok(appbar_mon == foregnd_mon, "Windows on different monitors\n");
            }
        }
    }

    memset(&abd, 0xcc, sizeof(abd));
    abd.cbSize = sizeof(abd);
    ret = SHAppBarMessage(ABM_GETTASKBARPOS, &abd);
    if(ret)
    {
        ok(abd.hWnd == (HWND)0xcccccccc, "hWnd overwritten\n");
todo_wine
{
        ok(abd.uEdge <= ABE_BOTTOM, "uEdge not returned\n");
        ok(abd.rc.left != 0xcccccccc, "rc not updated\n");
}
    }

    return;
}

START_TEST(appbar)
{
    HMODULE huser32;

    huser32 = GetModuleHandleA("user32.dll");
    pMonitorFromWindow = (void*)GetProcAddress(huser32, "MonitorFromWindow");

    register_testwindow_class();

    test_setpos();
    test_appbarget();
}
