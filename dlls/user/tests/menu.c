/*
 * Unit tests for menus
 *
 * Copyright 2005 Robert Shearman
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

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include <stdlib.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"

#include "wine/test.h"

static ATOM atomMenuCheckClass;

static LRESULT WINAPI menu_check_wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_ENTERMENULOOP:
        /* mark window as having entered menu loop */
        SetWindowLongPtr(hwnd, GWLP_USERDATA, TRUE);
        /* exit menu modal loop */
        return SendMessage(hwnd, WM_CANCELMODE, 0, 0);
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

static void register_menu_check_class(void)
{
    WNDCLASS wc =
    {
        0,
        menu_check_wnd_proc,
        0,
        0,
        GetModuleHandle(NULL),
        NULL,
        LoadCursor(NULL, IDC_ARROW),
        (HBRUSH)(COLOR_BTNFACE+1),
        NULL,
        TEXT("WineMenuCheck"),
    };
    
    atomMenuCheckClass = RegisterClass(&wc);
}

/* demonstrates that windows lock the menu object so that it is still valid
 * even after a client calls DestroyMenu on it */
static void test_menu_locked_by_window()
{
    BOOL ret;
    HMENU hmenu;
    HWND hwnd = CreateWindowEx(0, MAKEINTATOM(atomMenuCheckClass), NULL,
        WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL, NULL, NULL, NULL);
    ok(hwnd != NULL, "CreateWindowEx failed with error %ld\n", GetLastError());
    hmenu = CreateMenu();
    ok(hmenu != NULL, "CreateMenu failed with error %ld\n", GetLastError());
    ret = InsertMenu(hmenu, 0, MF_STRING, 0, TEXT("&Test"));
    ok(ret, "InsertMenu failed with error %ld\n", GetLastError());
    ret = SetMenu(hwnd, hmenu);
    ok(ret, "SetMenu failed with error %ld\n", GetLastError());
    ret = DestroyMenu(hmenu);
    ok(ret, "DestroyMenu failed with error %ld\n", GetLastError());

    ret = DrawMenuBar(hwnd);
    todo_wine {
    ok(ret, "DrawMenuBar failed with error %ld\n", GetLastError());
    }
    ret = IsMenu(GetMenu(hwnd));
    ok(!ret, "Menu handle should have been destroyed\n");

    SendMessage(hwnd, WM_SYSCOMMAND, SC_KEYMENU, 0);
    /* did we process the WM_INITMENU message? */
    ret = GetWindowLongPtr(hwnd, GWLP_USERDATA);
    todo_wine {
    ok(ret, "WM_INITMENU should have been sent\n");
    }

    DestroyWindow(hwnd);
}

START_TEST(menu)
{
    register_menu_check_class();

    test_menu_locked_by_window();
}
