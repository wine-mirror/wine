/*
 * UI Automation tests
 *
 * Copyright 2019 Nikolay Sivov for CodeWeavers
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

#define COBJMACROS

#include "windows.h"
#include "initguid.h"
#include "uiautomation.h"

#include "wine/test.h"

static LRESULT WINAPI test_wnd_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProcA(hwnd, message, wParam, lParam);
}

static void test_UiaHostProviderFromHwnd(void)
{
    IRawElementProviderSimple *p, *p2;
    WNDCLASSA cls;
    HRESULT hr;
    HWND hwnd;

    cls.style = 0;
    cls.lpfnWndProc = test_wnd_proc;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(NULL);
    cls.hIcon = 0;
    cls.hCursor = NULL;
    cls.hbrBackground = NULL;
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "HostProviderFromHwnd class";

    RegisterClassA(&cls);

    hwnd = CreateWindowExA(0, "HostProviderFromHwnd class", "Test window 1",
            WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_VISIBLE,
            0, 0, 100, 100, GetDesktopWindow(), NULL, GetModuleHandleA(NULL), NULL);
    ok(hwnd != NULL, "Failed to create a test window.\n");

    p = (void *)0xdeadbeef;
    hr = UiaHostProviderFromHwnd(NULL, &p);
todo_wine {
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);
    ok(p == NULL, "Unexpected instance.\n");
}
    p = NULL;
    hr = UiaHostProviderFromHwnd(hwnd, &p);
todo_wine
    ok(hr == S_OK, "Failed to get host provider, hr %#x.\n", hr);

if (hr == S_OK)
{
    p2 = NULL;
    hr = UiaHostProviderFromHwnd(hwnd, &p2);
    ok(hr == S_OK, "Failed to get host provider, hr %#x.\n", hr);
    ok(p != p2, "Unexpected instance.\n");
    IRawElementProviderSimple_Release(p2);

    hr = IRawElementProviderSimple_get_HostRawElementProvider(p, &p2);
    ok(hr == S_OK, "Unexpected hr %#x.\n", hr);
    ok(p2 == NULL, "Unexpected instance.\n");

    IRawElementProviderSimple_Release(p);
}

    DestroyWindow(hwnd);
    UnregisterClassA("HostProviderFromHwnd class", NULL);
}

START_TEST(uiautomation)
{
    test_UiaHostProviderFromHwnd();
}
