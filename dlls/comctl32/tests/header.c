/* Unit test suite for header control.
 *
 * Copyright 2005 Vijay Kiran Kamuju 
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


#include <windows.h>
#include <commctrl.h>
#include <assert.h>

#include "wine/test.h"

static HWND hHeaderParentWnd;
#define MAX_CHARS 100

static LONG addItem(HWND hdex, int idx, LPCSTR text)
{
    HDITEMA hdItem;
    hdItem.mask       = HDI_TEXT | HDI_WIDTH;
    hdItem.cxy        = 100;
    hdItem.pszText    = (LPSTR)text;
    hdItem.cchTextMax = 0;
    return (LONG)SendMessage(hdex, HDM_INSERTITEMA, (WPARAM)idx, (LPARAM)&hdItem);
}

static LONG setItem(HWND hdex, int idx, LPCSTR text)
{
    HDITEMA hdexItem;
    hdexItem.mask       = HDI_TEXT;
    hdexItem.pszText    = (LPSTR)text;
    hdexItem.cchTextMax = 0;
    return (LONG)SendMessage(hdex, HDM_SETITEMA, (WPARAM)idx, (LPARAM)&hdexItem);
}

static LONG delItem(HWND hdex, int idx)
{
    return (LONG)SendMessage(hdex, HDM_DELETEITEM, (WPARAM)idx, 0);
}

static LONG getItemCount(HWND hdex)
{
    return (LONG)SendMessage(hdex, HDM_GETITEMCOUNT, 0, 0);
}

static LONG getItem(HWND hdex, int idx, LPSTR textBuffer)
{
    HDITEMA hdItem;
    hdItem.mask         = HDI_TEXT;
    hdItem.pszText      = textBuffer;
    hdItem.cchTextMax   = MAX_CHARS;
    return (LONG)SendMessage(hdex, HDM_GETITEMA, (WPARAM)idx, (LPARAM)&hdItem);
}

static HWND create_header_control (void)
{
    HWND handle;
    HDLAYOUT hlayout;
    RECT rectwin;
    WINDOWPOS winpos;

    handle = CreateWindowEx(0, WC_HEADER, NULL,
			    WS_CHILD|WS_BORDER|WS_VISIBLE|HDS_BUTTONS|HDS_HORZ,
			    0, 0, 0, 0,
			    hHeaderParentWnd, NULL, NULL, NULL);
    assert(handle);

    if (winetest_interactive)
	ShowWindow (hHeaderParentWnd, SW_SHOW);

    GetClientRect(hHeaderParentWnd,&rectwin);
    hlayout.prc = &rectwin;
    hlayout.pwpos = &winpos;
    SendMessageA(handle,HDM_LAYOUT,0,(LPARAM) &hlayout);
    SetWindowPos(handle, winpos.hwndInsertAfter, winpos.x, winpos.y, 
                 winpos.cx, winpos.cy, 0);

    return handle;
}

static const char *str_items[] =
    {"First Item", "Second Item", "Third Item", "Fourth Item", "Replace Item", "Out Of Range Item"};

#define TEST_GET_ITEM(i,c)\
{   res = getItem(hWndHeader, i, buffer);\
    ok(res != 0, "Getting item[%d] using valid index failed unexpectedly (%ld)\n", i, res);\
    ok(strcmp(str_items[c], buffer) == 0, "Getting item[%d] returned \"%s\" expecting \"%s\"\n", i, buffer, str_items[c]);\
}

#define TEST_GET_ITEMCOUNT(i)\
{   res = getItemCount(hWndHeader);\
    ok(res == i, "Got Item Count as %ld\n", res);\
}

static void test_header_control (void)
{
    HWND hWndHeader;
    LONG res;
    static char buffer[MAX_CHARS];
    int i;

    hWndHeader = create_header_control ();

    for (i = 3; i >= 0; i--)
    {
        TEST_GET_ITEMCOUNT(3-i);
        res = addItem(hWndHeader, 0, str_items[i]);
        ok(res == 0, "Adding simple item failed (%ld)\n", res);
    }

    TEST_GET_ITEMCOUNT(4);
    res = addItem(hWndHeader, 99, str_items[i+1]);
    ok(res != -1, "Adding Out of Range item should fail with -1 got (%ld)\n", res);
    TEST_GET_ITEMCOUNT(5);
    res = addItem(hWndHeader, 5, str_items[i+1]);
    ok(res != -1, "Adding Out of Range item should fail with -1 got (%ld)\n", res);
    TEST_GET_ITEMCOUNT(6);

    for (i = 0; i < 4; i++) { TEST_GET_ITEM(i,i); TEST_GET_ITEMCOUNT(6); }

    res=getItem(hWndHeader, 99, buffer);
    ok(res == 0, "Getting Out of Range item should fail with 0 (%ld), got %s\n", res,buffer);
    res=getItem(hWndHeader, 5, buffer);
    ok(res == 1, "Getting Out of Range item should fail with 1 (%ld), got %s\n", res,buffer);
    res=getItem(hWndHeader, -2, buffer);
    ok(res == 0, "Getting Out of Range item should fail with 0 (%ld), got %s\n", res,buffer);

    if (winetest_interactive)
    {
        UpdateWindow(hHeaderParentWnd);
        UpdateWindow(hWndHeader);
    }

    TEST_GET_ITEMCOUNT(6);
    res=setItem(hWndHeader, 99, str_items[5]);
    ok(res == 0, "Setting Out of Range item should fail with 0 (%ld)\n", res);
    res=setItem(hWndHeader, 5, str_items[5]);
    ok(res == 1, "Setting Out of Range item should fail with 1 (%ld)\n", res);
    res=setItem(hWndHeader, -2, str_items[5]);
    ok(res == 0, "Setting Out of Range item should fail with 0 (%ld)\n", res);
    TEST_GET_ITEMCOUNT(6);

    for (i = 0; i < 4; i++)
    {
        res = setItem(hWndHeader, i, str_items[4]);
        ok(res != 0, "Setting %d item failed (%ld)\n", i+1, res);
        TEST_GET_ITEM(i, 4);
        TEST_GET_ITEMCOUNT(6);
    }

    res = delItem(hWndHeader, 5);
    ok(res == 1, "Deleting Out of Range item should fail with 1 (%ld)\n", res);
    res = delItem(hWndHeader, -2);
    ok(res == 0, "Deleting Out of Range item should fail with 0 (%ld)\n", res);
    TEST_GET_ITEMCOUNT(5);

    res = delItem(hWndHeader, 3);
    ok(res != 0, "Deleting using out of range index failed (%ld)\n", res);
    TEST_GET_ITEMCOUNT(4);
    res = delItem(hWndHeader, 0);
    ok(res != 0, "Deleting using out of range index failed (%ld)\n", res);
    TEST_GET_ITEMCOUNT(3);
    res = delItem(hWndHeader, 0);
    ok(res != 0, "Deleting using out of range index failed (%ld)\n", res);
    TEST_GET_ITEMCOUNT(2);
    res = delItem(hWndHeader, 0);
    ok(res != 0, "Deleting using out of range index failed (%ld)\n", res);
    TEST_GET_ITEMCOUNT(1);

    DestroyWindow(hWndHeader);
}

START_TEST(header)
{
    INITCOMMONCONTROLSEX icex;

    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC  = ICC_USEREX_CLASSES;
    InitCommonControlsEx(&icex);
    hHeaderParentWnd = CreateWindowExA(0, "static", "Header test", WS_OVERLAPPEDWINDOW,
                        CW_USEDEFAULT, CW_USEDEFAULT, 480, 100, NULL, NULL, NULL, 0);
    assert(hHeaderParentWnd != NULL);

    test_header_control();

    DestroyWindow(hHeaderParentWnd);
}
