/* Unit tests for treeview.
 *
 * Copyright 2005 Krzysztof Foltman
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

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "winreg.h"
#include "commctrl.h" 

#include "resources.h"

#include "wine/test.h"

HWND hMainWnd;
 
static void MakeButton(TBBUTTON *p, int idCommand, int fsStyle, int nString) {
  p->iBitmap = -2;
  p->idCommand = idCommand;
  p->fsState = TBSTATE_ENABLED;
  p->fsStyle = fsStyle;
  p->iString = nString;
}

static LRESULT CALLBACK MyWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProcA(hWnd, msg, wParam, lParam);
}

static void basic_test(void)
{
    TBBUTTON buttons[9];
    HWND hToolbar;
    int i;
    for (i=0; i<9; i++)
        MakeButton(buttons+i, 1000+i, TBSTYLE_CHECKGROUP, 0);
    MakeButton(buttons+3, 1003, TBSTYLE_SEP|TBSTYLE_GROUP, 0);
    MakeButton(buttons+6, 1006, TBSTYLE_SEP, 0);

    hToolbar = CreateToolbarEx(hMainWnd,
        WS_VISIBLE | WS_CLIPCHILDREN | CCS_TOP |
        WS_CHILD | TBSTYLE_LIST,
        100,
        0, NULL, (UINT)0,
        buttons, sizeof(buttons)/sizeof(buttons[0]),
        0, 0, 20, 16, sizeof(TBBUTTON));
    ok(hToolbar != NULL, "Toolbar creation\n");
    SendMessage(hToolbar, TB_ADDSTRINGA, 0, (LPARAM)"test\000");

    /* test for exclusion working inside a separator-separated :-) group */
    SendMessage(hToolbar, TB_CHECKBUTTON, 1000, 1); /* press A1 */
    ok(SendMessage(hToolbar, TB_ISBUTTONCHECKED, 1000, 0), "A1 pressed\n");
    ok(!SendMessage(hToolbar, TB_ISBUTTONCHECKED, 1001, 0), "A2 not pressed\n");

    SendMessage(hToolbar, TB_CHECKBUTTON, 1004, 1); /* press A5, release A1 */
    ok(SendMessage(hToolbar, TB_ISBUTTONCHECKED, 1004, 0), "A5 pressed\n");
    ok(!SendMessage(hToolbar, TB_ISBUTTONCHECKED, 1000, 0), "A1 not pressed anymore\n");

    SendMessage(hToolbar, TB_CHECKBUTTON, 1005, 1); /* press A6, release A5 */
    ok(SendMessage(hToolbar, TB_ISBUTTONCHECKED, 1005, 0), "A6 pressed\n");
    ok(!SendMessage(hToolbar, TB_ISBUTTONCHECKED, 1004, 0), "A5 not pressed anymore\n");

    /* test for inter-group crosstalk, ie. two radio groups interfering with each other */
    SendMessage(hToolbar, TB_CHECKBUTTON, 1007, 1); /* press B2 */
    ok(SendMessage(hToolbar, TB_ISBUTTONCHECKED, 1005, 0), "A6 still pressed, no inter-group crosstalk\n");
    ok(!SendMessage(hToolbar, TB_ISBUTTONCHECKED, 1000, 0), "A1 still not pressed\n");
    ok(SendMessage(hToolbar, TB_ISBUTTONCHECKED, 1007, 0), "B2 pressed\n");

    SendMessage(hToolbar, TB_CHECKBUTTON, 1000, 1); /* press A1 and ensure B group didn't suffer */
    ok(!SendMessage(hToolbar, TB_ISBUTTONCHECKED, 1005, 0), "A6 not pressed anymore\n");
    ok(SendMessage(hToolbar, TB_ISBUTTONCHECKED, 1000, 0), "A1 pressed\n");
    ok(SendMessage(hToolbar, TB_ISBUTTONCHECKED, 1007, 0), "B2 still pressed\n");

    SendMessage(hToolbar, TB_CHECKBUTTON, 1008, 1); /* press B3, and ensure A group didn't suffer */
    ok(!SendMessage(hToolbar, TB_ISBUTTONCHECKED, 1005, 0), "A6 pressed\n");
    ok(SendMessage(hToolbar, TB_ISBUTTONCHECKED, 1000, 0), "A1 pressed\n");
    ok(!SendMessage(hToolbar, TB_ISBUTTONCHECKED, 1007, 0), "B2 not pressed\n");
    ok(SendMessage(hToolbar, TB_ISBUTTONCHECKED, 1008, 0), "B3 pressed\n");
    DestroyWindow(hToolbar);
}

static void rebuild_toolbar(HWND *hToolbar)
{
    if (*hToolbar != NULL)
        DestroyWindow(*hToolbar);
    *hToolbar = CreateWindowEx(0, TOOLBARCLASSNAME, NULL, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
        hMainWnd, (HMENU)5, GetModuleHandle(NULL), NULL);
    ok(*hToolbar != NULL, "Toolbar creation problem\n");
    ok(SendMessage(*hToolbar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0) == 0, "TB_BUTTONSTRUCTSIZE failed\n");
    ok(SendMessage(*hToolbar, TB_AUTOSIZE, 0, 0) == 0, "TB_AUTOSIZE failed\n");
}

#define CHECK_IMAGELIST(count, dx, dy) { \
    int cx, cy; \
    HIMAGELIST himl = (HIMAGELIST)SendMessageA(hToolbar, TB_GETIMAGELIST, 0, 0); \
    ok(himl != NULL, "No image list\n"); \
    if (himl != NULL) {\
        ok(ImageList_GetImageCount(himl) == count, "Images count mismatch - %d vs %d\n", count, ImageList_GetImageCount(himl)); \
        ImageList_GetIconSize(himl, &cx, &cy); \
        ok(cx == dx && cy == cy, "Icon size mismatch - %dx%d vs %dx%d\n", dx, dy, cx, cy); \
    } \
}

#define CHECK_IMAGELIST_TODO_COUNT(count, dx, dy) { \
    int cx, cy; \
    HIMAGELIST himl = (HIMAGELIST)SendMessageA(hToolbar, TB_GETIMAGELIST, 0, 0); \
    ok(himl != NULL, "No image list\n"); \
    if (himl != NULL) {\
        todo_wine ok(ImageList_GetImageCount(himl) == count, "Images count mismatch - %d vs %d\n", count, ImageList_GetImageCount(himl)); \
        ImageList_GetIconSize(himl, &cx, &cy); \
        ok(cx == dx && cy == cy, "Icon size mismatch - %dx%d vs %dx%d\n", dx, dy, cx, cy); \
    } \
}

#define CHECK_IMAGELIST_TODO_COUNT_SIZE(count, dx, dy) { \
    int cx, cy; \
    HIMAGELIST himl = (HIMAGELIST)SendMessageA(hToolbar, TB_GETIMAGELIST, 0, 0); \
    ok(himl != NULL, "No image list\n"); \
    if (himl != NULL) {\
        todo_wine ok(ImageList_GetImageCount(himl) == count, "Images count mismatch - %d vs %d\n", count, ImageList_GetImageCount(himl)); \
        ImageList_GetIconSize(himl, &cx, &cy); \
        todo_wine ok(cx == dx && cy == cy, "Icon size mismatch - %dx%d vs %dx%d\n", dx, dy, cx, cy); \
    } \
}

static void test_add_bitmap(void)
{
    HWND hToolbar = NULL;
    TBADDBITMAP bmp128;
    TBADDBITMAP bmp80;
    TBADDBITMAP stdsmall;
    TBADDBITMAP addbmp;
    INT ret;

    /* empty 128x15 bitmap */
    bmp128.hInst = GetModuleHandle(NULL);
    bmp128.nID = IDB_BITMAP_128x15;

    /* empty 80x15 bitmap */
    bmp80.hInst = GetModuleHandle(NULL);
    bmp80.nID = IDB_BITMAP_80x15;

    /* standard bitmap - 240x15 pixels */
    stdsmall.hInst = HINST_COMMCTRL;
    stdsmall.nID = IDB_STD_SMALL_COLOR;

    rebuild_toolbar(&hToolbar);
    ok(SendMessageA(hToolbar, TB_ADDBITMAP, 8, (LPARAM)&bmp128) == 0, "TB_ADDBITMAP - unexpected return\n");
    CHECK_IMAGELIST(8, 16, 15);
    
    /* adding more bitmaps */
    ok(SendMessageA(hToolbar, TB_ADDBITMAP, 5, (LPARAM)&bmp80) == 8, "TB_ADDBITMAP - unexpected return\n");
    CHECK_IMAGELIST(13, 16, 15);
    /* adding the same bitmap will simply return the index of the already loaded block */
    ret = SendMessageA(hToolbar, TB_ADDBITMAP, 8, (LPARAM)&bmp128);
    todo_wine ok(ret == 0, "TB_ADDBITMAP - unexpected return %d\n", ret);
    CHECK_IMAGELIST_TODO_COUNT(13, 16, 15);
    ret = SendMessageA(hToolbar, TB_ADDBITMAP, 5, (LPARAM)&bmp80);
    todo_wine ok(ret == 8, "TB_ADDBITMAP - unexpected return %d\n", ret);
    CHECK_IMAGELIST_TODO_COUNT(13, 16, 15);
    /* even if we increase the wParam */
    ret = SendMessageA(hToolbar, TB_ADDBITMAP, 55, (LPARAM)&bmp80);
    todo_wine ok(ret == 8, "TB_ADDBITMAP - unexpected return %d\n", ret);
    CHECK_IMAGELIST_TODO_COUNT(13, 16, 15);

    /* when the wParam is smaller than the bitmaps count but non-zero, all the bitmaps will be added*/
    rebuild_toolbar(&hToolbar);
    ok(SendMessageA(hToolbar, TB_ADDBITMAP, 3, (LPARAM)&bmp128) == 0, "TB_ADDBITMAP - unexpected return\n");
    CHECK_IMAGELIST(8, 16, 15);
    ret = SendMessageA(hToolbar, TB_ADDBITMAP, 5, (LPARAM)&bmp80);
    todo_wine ok(ret == 3, "TB_ADDBITMAP - unexpected return %d\n", ret);
    /* the returned value is misleading - id 8 is the id of the first icon from bmp80 */
    CHECK_IMAGELIST(13, 16, 15);

    /* the same for negative wParam */
    rebuild_toolbar(&hToolbar);
    ret = SendMessageA(hToolbar, TB_ADDBITMAP, -143, (LPARAM)&bmp128);
    ok(ret == 0, "TB_ADDBITMAP - unexpected return %d\n", ret);
    CHECK_IMAGELIST(8, 16, 15);
    ret = SendMessageA(hToolbar, TB_ADDBITMAP, 1, (LPARAM)&bmp80);
    todo_wine ok(ret == -143, "TB_ADDBITMAP - unexpected return %d\n", ret);
    CHECK_IMAGELIST(13, 16, 15);

    /* for zero only one bitmap will be added */
    rebuild_toolbar(&hToolbar);
    ret = SendMessageA(hToolbar, TB_ADDBITMAP, 0, (LPARAM)&bmp80);
    ok(ret == 0, "TB_ADDBITMAP - unexpected return %d\n", ret);
    CHECK_IMAGELIST(1, 16, 15);

    /* if wParam is larger than the amount of icons, the list is grown */
    rebuild_toolbar(&hToolbar);
    ok(SendMessageA(hToolbar, TB_ADDBITMAP, 100, (LPARAM)&bmp80) == 0, "TB_ADDBITMAP - unexpected return\n");
    CHECK_IMAGELIST(100, 16, 15);
    ret = SendMessageA(hToolbar, TB_ADDBITMAP, 100, (LPARAM)&bmp128);
    ok(ret == 100, "TB_ADDBITMAP - unexpected return %d\n", ret);
    CHECK_IMAGELIST(200, 16, 15);

    /* adding built-in items - the wParam is ignored */
    rebuild_toolbar(&hToolbar);
    ok(SendMessageA(hToolbar, TB_ADDBITMAP, 5, (LPARAM)&bmp80) == 0, "TB_ADDBITMAP - unexpected return\n");
    CHECK_IMAGELIST(5, 16, 15);
    ok(SendMessageA(hToolbar, TB_ADDBITMAP, 0, (LPARAM)&stdsmall) == 5, "TB_ADDBITMAP - unexpected return\n");
    CHECK_IMAGELIST(20, 16, 15);
    ok(SendMessageA(hToolbar, TB_ADDBITMAP, 5, (LPARAM)&bmp128) == 20, "TB_ADDBITMAP - unexpected return\n");
    CHECK_IMAGELIST(28, 16, 15);

    /* when we increase the bitmap size, less icons will be created */
    rebuild_toolbar(&hToolbar);
    ok(SendMessageA(hToolbar, TB_SETBITMAPSIZE, 0, MAKELONG(20, 20)) == TRUE, "TB_SETBITMAPSIZE failed\n");
    ok(SendMessageA(hToolbar, TB_ADDBITMAP, 1, (LPARAM)&bmp128) == 0, "TB_ADDBITMAP - unexpected return\n");
    CHECK_IMAGELIST(6, 20, 20);
    ret = SendMessageA(hToolbar, TB_ADDBITMAP, 1, (LPARAM)&bmp80);
    todo_wine ok(ret == 1, "TB_ADDBITMAP - unexpected return %d\n", ret);
    CHECK_IMAGELIST(10, 20, 20);
    /* the icons can be resized - an UpdateWindow is needed as this probably happens during WM_PAINT */
    ok(SendMessageA(hToolbar, TB_SETBITMAPSIZE, 0, MAKELONG(8, 8)) == TRUE, "TB_SETBITMAPSIZE failed\n");
    UpdateWindow(hToolbar);
    CHECK_IMAGELIST_TODO_COUNT_SIZE(26, 8, 8);
    /* loading a standard bitmaps automatically resizes the icons */
    todo_wine ok(SendMessageA(hToolbar, TB_ADDBITMAP, 1, (LPARAM)&stdsmall) == 2, "TB_ADDBITMAP - unexpected return\n");
    UpdateWindow(hToolbar);
    CHECK_IMAGELIST_TODO_COUNT_SIZE(28, 16, 15);

    /* check standard bitmaps */
    addbmp.hInst = HINST_COMMCTRL;
    addbmp.nID = IDB_STD_SMALL_COLOR;
    rebuild_toolbar(&hToolbar);
    ok(SendMessageA(hToolbar, TB_ADDBITMAP, 1, (LPARAM)&addbmp) == 0, "TB_ADDBITMAP - unexpected return\n");
    CHECK_IMAGELIST(15, 16, 15);
    addbmp.nID = IDB_STD_LARGE_COLOR;
    rebuild_toolbar(&hToolbar);
    ok(SendMessageA(hToolbar, TB_ADDBITMAP, 1, (LPARAM)&addbmp) == 0, "TB_ADDBITMAP - unexpected return\n");
    CHECK_IMAGELIST(15, 24, 24);

    addbmp.nID = IDB_VIEW_SMALL_COLOR;
    rebuild_toolbar(&hToolbar);
    ok(SendMessageA(hToolbar, TB_ADDBITMAP, 1, (LPARAM)&addbmp) == 0, "TB_ADDBITMAP - unexpected return\n");
    CHECK_IMAGELIST(12, 16, 15);
    addbmp.nID = IDB_VIEW_LARGE_COLOR;
    rebuild_toolbar(&hToolbar);
    ok(SendMessageA(hToolbar, TB_ADDBITMAP, 1, (LPARAM)&addbmp) == 0, "TB_ADDBITMAP - unexpected return\n");
    CHECK_IMAGELIST(12, 24, 24);

    addbmp.nID = IDB_HIST_SMALL_COLOR;
    rebuild_toolbar(&hToolbar);
    ok(SendMessageA(hToolbar, TB_ADDBITMAP, 1, (LPARAM)&addbmp) == 0, "TB_ADDBITMAP - unexpected return\n");
    CHECK_IMAGELIST(5, 16, 15);
    addbmp.nID = IDB_HIST_LARGE_COLOR;
    rebuild_toolbar(&hToolbar);
    ok(SendMessageA(hToolbar, TB_ADDBITMAP, 1, (LPARAM)&addbmp) == 0, "TB_ADDBITMAP - unexpected return\n");
    CHECK_IMAGELIST(5, 24, 24);


    DestroyWindow(hToolbar);
}

START_TEST(toolbar)
{
    WNDCLASSA wc;
    MSG msg;
    RECT rc;
  
    InitCommonControls();
  
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.hIcon = NULL;
    wc.hCursor = LoadCursorA(NULL, MAKEINTRESOURCEA(IDC_IBEAM));
    wc.hbrBackground = GetSysColorBrush(COLOR_WINDOW);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = "MyTestWnd";
    wc.lpfnWndProc = MyWndProc;
    RegisterClassA(&wc);
    
    hMainWnd = CreateWindowExA(0, "MyTestWnd", "Blah", WS_OVERLAPPEDWINDOW, 
      CW_USEDEFAULT, CW_USEDEFAULT, 680, 260, NULL, NULL, GetModuleHandleA(NULL), 0);
    GetClientRect(hMainWnd, &rc);
    ShowWindow(hMainWnd, SW_SHOW);

    basic_test();
    test_add_bitmap();

    PostQuitMessage(0);
    while(GetMessageA(&msg,0,0,0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    DestroyWindow(hMainWnd);
}
