/* Unit test suite for tab control.
 *
 * Copyright 2003 Vitaliy Margolen
 * Copyright 2007 Hagop Hagopian
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

#define DEFAULT_MIN_TAB_WIDTH 54
#define TAB_DEFAULT_WIDTH 96
#define TAB_PADDING_X 6
#define EXTRA_ICON_PADDING 3
#define MAX_TABLEN 32

#define expect(expected, got) ok ( expected == got, "Expected %d, got %d\n", expected, got)
#define expect_str(expected, got)\
 ok ( strcmp(expected, got) == 0, "Expected '%s', got '%s'\n", expected, got)

#define TabWidthPadded(padd_x, num) (DEFAULT_MIN_TAB_WIDTH - (TAB_PADDING_X - (padd_x)) * num)

#define TabCheckSetSize(hwnd, SetWidth, SetHeight, ExpWidth, ExpHeight, Msg)\
    SendMessage (hwnd, TCM_SETITEMSIZE, 0,\
	(LPARAM) MAKELPARAM((SetWidth >= 0) ? SetWidth:0, (SetHeight >= 0) ? SetHeight:0));\
    if (winetest_interactive) RedrawWindow (hwnd, NULL, 0, RDW_UPDATENOW);\
    CheckSize(hwnd, ExpWidth, ExpHeight, Msg);

#define CheckSize(hwnd,width,height,msg)\
    SendMessage (hwnd, TCM_GETITEMRECT, 0, (LPARAM) &rTab);\
    if ((width  >= 0) && (height < 0))\
	ok (width  == rTab.right  - rTab.left, "%s: Expected width [%d] got [%d]\n",\
        msg, (int)width,  rTab.right  - rTab.left);\
    else if ((height >= 0) && (width  < 0))\
	ok (height == rTab.bottom - rTab.top,  "%s: Expected height [%d] got [%d]\n",\
        msg, (int)height, rTab.bottom - rTab.top);\
    else\
	ok ((width  == rTab.right  - rTab.left) &&\
	    (height == rTab.bottom - rTab.top ),\
	    "%s: Expected [%d,%d] got [%d,%d]\n", msg, (int)width, (int)height,\
            rTab.right - rTab.left, rTab.bottom - rTab.top);

static HFONT hFont = 0;

static HWND
create_tabcontrol (DWORD style, DWORD mask)
{
    HWND handle;
    TCITEM tcNewTab;
    static char text1[] = "Tab 1",
    text2[] = "Wide Tab 2",
    text3[] = "T 3";

    handle = CreateWindow (
	WC_TABCONTROLA,
	"TestTab",
	WS_CLIPSIBLINGS | WS_CLIPCHILDREN | TCS_FOCUSNEVER | style,
        10, 10, 300, 100,
        NULL, NULL, NULL, 0);

    assert (handle);

    SetWindowLong(handle, GWL_STYLE, WS_CLIPSIBLINGS | WS_CLIPCHILDREN | TCS_FOCUSNEVER | style);
    SendMessage (handle, WM_SETFONT, 0, (LPARAM) hFont);

    tcNewTab.mask = mask;
    tcNewTab.pszText = text1;
    tcNewTab.iImage = 0;
    SendMessage (handle, TCM_INSERTITEM, 0, (LPARAM) &tcNewTab);
    tcNewTab.pszText = text2;
    tcNewTab.iImage = 1;
    SendMessage (handle, TCM_INSERTITEM, 1, (LPARAM) &tcNewTab);
    tcNewTab.pszText = text3;
    tcNewTab.iImage = 2;
    SendMessage (handle, TCM_INSERTITEM, 2, (LPARAM) &tcNewTab);

    if (winetest_interactive)
    {
        ShowWindow (handle, SW_SHOW);
        RedrawWindow (handle, NULL, 0, RDW_UPDATENOW);
        Sleep (1000);
    }

    return handle;
}

static HWND createFilledTabControl(DWORD style, DWORD mask, INT nTabs)
{
    HWND tabHandle;
    TCITEM tcNewTab;
    INT i;

    tabHandle = CreateWindow (
        WC_TABCONTROLA,
        "TestTab",
        WS_CLIPSIBLINGS | WS_CLIPCHILDREN | TCS_FOCUSNEVER | style,
        0, 0, 300, 100,
        NULL, NULL, NULL, 0);

    assert(tabHandle);

    SetWindowLong(tabHandle, GWL_STYLE, WS_CLIPSIBLINGS | WS_CLIPCHILDREN | TCS_FOCUSNEVER | style);
    SendMessage (tabHandle, WM_SETFONT, 0, (LPARAM) hFont);

    tcNewTab.mask = mask;

    for (i = 0; i < nTabs; i++)
    {
        char tabName[MAX_TABLEN];

        sprintf(tabName, "Tab %d", i+1);
        tcNewTab.pszText = tabName;
        tcNewTab.iImage = i;
        SendMessage (tabHandle, TCM_INSERTITEM, i, (LPARAM) &tcNewTab);
    }

    if (winetest_interactive)
    {
        ShowWindow (tabHandle, SW_SHOW);
        RedrawWindow (tabHandle, NULL, 0, RDW_UPDATENOW);
        Sleep (1000);
    }

    return tabHandle;
}

static HWND create_tooltip (HWND hTab, char toolTipText[])
{
    HWND hwndTT;

    TOOLINFO ti;
    LPTSTR lptstr = toolTipText;
    RECT rect;

    /* Creating a tooltip window*/
    hwndTT = CreateWindowEx(
        WS_EX_TOPMOST,
        TOOLTIPS_CLASS,
        NULL,
        WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        hTab, NULL, 0, NULL);

    SetWindowPos(
        hwndTT,
        HWND_TOPMOST,
        0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

    GetClientRect (hTab, &rect);

    /* Initialize members of toolinfo*/
    ti.cbSize = sizeof(TOOLINFO);
    ti.uFlags = TTF_SUBCLASS;
    ti.hwnd = hTab;
    ti.hinst = 0;
    ti.uId = 0;
    ti.lpszText = lptstr;

    ti.rect = rect;

    /* Add toolinfo structure to the tooltip control */
    SendMessage(hwndTT, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &ti);

    return hwndTT;
}

static void test_tab(INT nMinTabWidth)
{
    HWND hwTab;
    RECT rTab;
    HIMAGELIST himl = ImageList_Create(21, 21, ILC_COLOR, 3, 4);
    SIZE size;
    HDC hdc;
    HFONT hOldFont;
    INT i;

    hwTab = create_tabcontrol(TCS_FIXEDWIDTH, TCIF_TEXT|TCIF_IMAGE);
    SendMessage(hwTab, TCM_SETMINTABWIDTH, 0, nMinTabWidth);

    hdc = GetDC(hwTab);
    hOldFont = SelectObject(hdc, (HFONT)SendMessage(hwTab, WM_GETFONT, 0, 0));
    GetTextExtentPoint32A(hdc, "Tab 1", strlen("Tab 1"), &size);
    trace("Tab1 text size: size.cx=%d size.cy=%d\n", size.cx, size.cy);
    SelectObject(hdc, hOldFont);
    ReleaseDC(hwTab, hdc);

    trace ("  TCS_FIXEDWIDTH tabs no icon...\n");
    CheckSize(hwTab, TAB_DEFAULT_WIDTH, -1, "default width");
    TabCheckSetSize(hwTab, 50, 20, 50, 20, "set size");
    TabCheckSetSize(hwTab, 0, 1, 0, 1, "min size");

    SendMessage(hwTab, TCM_SETIMAGELIST, 0, (LPARAM)himl);

    trace ("  TCS_FIXEDWIDTH tabs with icon...\n");
    TabCheckSetSize(hwTab, 50, 30, 50, 30, "set size > icon");
    TabCheckSetSize(hwTab, 20, 20, 25, 20, "set size < icon");
    TabCheckSetSize(hwTab, 0, 1, 25, 1, "min size");

    DestroyWindow (hwTab);

    hwTab = create_tabcontrol(TCS_FIXEDWIDTH | TCS_BUTTONS, TCIF_TEXT|TCIF_IMAGE);
    SendMessage(hwTab, TCM_SETMINTABWIDTH, 0, nMinTabWidth);

    trace ("  TCS_FIXEDWIDTH buttons no icon...\n");
    CheckSize(hwTab, TAB_DEFAULT_WIDTH, -1, "default width");
    TabCheckSetSize(hwTab, 20, 20, 20, 20, "set size 1");
    TabCheckSetSize(hwTab, 10, 50, 10, 50, "set size 2");
    TabCheckSetSize(hwTab, 0, 1, 0, 1, "min size");

    SendMessage(hwTab, TCM_SETIMAGELIST, 0, (LPARAM)himl);

    trace ("  TCS_FIXEDWIDTH buttons with icon...\n");
    TabCheckSetSize(hwTab, 50, 30, 50, 30, "set size > icon");
    TabCheckSetSize(hwTab, 20, 20, 25, 20, "set size < icon");
    TabCheckSetSize(hwTab, 0, 1, 25, 1, "min size");
    SendMessage(hwTab, TCM_SETPADDING, 0, MAKELPARAM(4,4));
    TabCheckSetSize(hwTab, 0, 1, 25, 1, "set padding, min size");

    DestroyWindow (hwTab);

    hwTab = create_tabcontrol(TCS_FIXEDWIDTH | TCS_BOTTOM, TCIF_TEXT|TCIF_IMAGE);
    SendMessage(hwTab, TCM_SETMINTABWIDTH, 0, nMinTabWidth);

    trace ("  TCS_FIXEDWIDTH | TCS_BOTTOM tabs...\n");
    CheckSize(hwTab, TAB_DEFAULT_WIDTH, -1, "no icon, default width");

    TabCheckSetSize(hwTab, 20, 20, 20, 20, "no icon, set size 1");
    TabCheckSetSize(hwTab, 10, 50, 10, 50, "no icon, set size 2");
    TabCheckSetSize(hwTab, 0, 1, 0, 1, "no icon, min size");

    SendMessage(hwTab, TCM_SETIMAGELIST, 0, (LPARAM)himl);

    TabCheckSetSize(hwTab, 50, 30, 50, 30, "with icon, set size > icon");
    TabCheckSetSize(hwTab, 20, 20, 25, 20, "with icon, set size < icon");
    TabCheckSetSize(hwTab, 0, 1, 25, 1, "with icon, min size");
    SendMessage(hwTab, TCM_SETPADDING, 0, MAKELPARAM(4,4));
    TabCheckSetSize(hwTab, 0, 1, 25, 1, "set padding, min size");

    DestroyWindow (hwTab);

    hwTab = create_tabcontrol(0, TCIF_TEXT|TCIF_IMAGE);
    SendMessage(hwTab, TCM_SETMINTABWIDTH, 0, nMinTabWidth);

    trace ("  non fixed width, with text...\n");
    CheckSize(hwTab, max(size.cx +TAB_PADDING_X*2, (nMinTabWidth < 0) ? DEFAULT_MIN_TAB_WIDTH : nMinTabWidth), -1,
              "no icon, default width");
    for (i=0; i<8; i++)
    {
        INT nTabWidth = (nMinTabWidth < 0) ? TabWidthPadded(i, 2) : nMinTabWidth;

        SendMessage(hwTab, TCM_SETIMAGELIST, 0, 0);
        SendMessage(hwTab, TCM_SETPADDING, 0, MAKELPARAM(i,i));

        TabCheckSetSize(hwTab, 50, 20, max(size.cx + i*2, nTabWidth), 20, "no icon, set size");
        TabCheckSetSize(hwTab, 0, 1, max(size.cx + i*2, nTabWidth), 1, "no icon, min size");

        SendMessage(hwTab, TCM_SETIMAGELIST, 0, (LPARAM)himl);
        nTabWidth = (nMinTabWidth < 0) ? TabWidthPadded(i, 3) : nMinTabWidth;

        TabCheckSetSize(hwTab, 50, 30, max(size.cx + 21 + i*3, nTabWidth), 30, "with icon, set size > icon");
        TabCheckSetSize(hwTab, 20, 20, max(size.cx + 21 + i*3, nTabWidth), 20, "with icon, set size < icon");
        TabCheckSetSize(hwTab, 0, 1, max(size.cx + 21 + i*3, nTabWidth), 1, "with icon, min size");
    }
    DestroyWindow (hwTab);

    hwTab = create_tabcontrol(0, TCIF_IMAGE);
    SendMessage(hwTab, TCM_SETMINTABWIDTH, 0, nMinTabWidth);

    trace ("  non fixed width, no text...\n");
    CheckSize(hwTab, (nMinTabWidth < 0) ? DEFAULT_MIN_TAB_WIDTH : nMinTabWidth, -1, "no icon, default width");
    for (i=0; i<8; i++)
    {
        INT nTabWidth = (nMinTabWidth < 0) ? TabWidthPadded(i, 2) : nMinTabWidth;

        SendMessage(hwTab, TCM_SETIMAGELIST, 0, 0);
        SendMessage(hwTab, TCM_SETPADDING, 0, MAKELPARAM(i,i));

        TabCheckSetSize(hwTab, 50, 20, nTabWidth, 20, "no icon, set size");
        TabCheckSetSize(hwTab, 0, 1, nTabWidth, 1, "no icon, min size");

        SendMessage(hwTab, TCM_SETIMAGELIST, 0, (LPARAM)himl);
        if (i > 1 && nMinTabWidth > 0 && nMinTabWidth < DEFAULT_MIN_TAB_WIDTH)
            nTabWidth += EXTRA_ICON_PADDING *(i-1);

        TabCheckSetSize(hwTab, 50, 30, nTabWidth, 30, "with icon, set size > icon");
        TabCheckSetSize(hwTab, 20, 20, nTabWidth, 20, "with icon, set size < icon");
        TabCheckSetSize(hwTab, 0, 1, nTabWidth, 1, "with icon, min size");
    }

    DestroyWindow (hwTab);

    ImageList_Destroy(himl);
    DeleteObject(hFont);
}

static void test_getters_setters(INT nTabs)
{
    HWND hTab;
    RECT rTab;
    INT nTabsRetrieved;
    INT rowCount;

    hTab = createFilledTabControl(TCS_FIXEDWIDTH, TCIF_TEXT|TCIF_IMAGE, nTabs);
    ok(hTab != NULL, "Failed to create tab control\n");

    todo_wine{
        expect(DEFAULT_MIN_TAB_WIDTH, (int)SendMessage(hTab, TCM_SETMINTABWIDTH, 0, -1));
    }
    /* Testing GetItemCount */
    nTabsRetrieved = SendMessage(hTab, TCM_GETITEMCOUNT, 0, 0);
    expect(nTabs, nTabsRetrieved);

    /* Testing GetRowCount */
    rowCount = SendMessage(hTab, TCM_GETROWCOUNT, 0, 0);
    expect(1, rowCount);

    /* Testing GetItemRect */
    ok(SendMessage(hTab, TCM_GETITEMRECT, 0, (LPARAM) &rTab), "GetItemRect failed.\n");
    CheckSize(hTab, TAB_DEFAULT_WIDTH, -1 , "Default Width");

    /* Testing CurFocus */
    {
        INT focusIndex;

        /* Testing CurFocus with largest appropriate value */
        SendMessage(hTab, TCM_SETCURFOCUS, nTabs-1, 0);
        focusIndex = SendMessage(hTab, TCM_GETCURFOCUS, 0, 0);
            expect(nTabs-1, focusIndex);

        /* Testing CurFocus with negative value */
        SendMessage(hTab, TCM_SETCURFOCUS, -10, 0);
        focusIndex = SendMessage(hTab, TCM_GETCURFOCUS, 0, 0);
        todo_wine{
            expect(-1, focusIndex);
        }

        /* Testing CurFocus with value larger than number of tabs */
        focusIndex = SendMessage(hTab, TCM_SETCURSEL, 1, 0);
        todo_wine{
            expect(-1, focusIndex);
        }
        SendMessage(hTab, TCM_SETCURFOCUS, nTabs+1, 0);
        focusIndex = SendMessage(hTab, TCM_GETCURFOCUS, 0, 0);
        todo_wine{
            expect(1, focusIndex);
        }
    }

    /* Testing CurSel */
    {
        INT selectionIndex;

        /* Testing CurSel with largest appropriate value */
        selectionIndex = SendMessage(hTab, TCM_SETCURSEL, nTabs-1, 0);
            expect(1, selectionIndex);
        selectionIndex = SendMessage(hTab, TCM_GETCURSEL, 0, 0);
            expect(nTabs-1, selectionIndex);

        /* Testing CurSel with negative value */
        SendMessage(hTab, TCM_SETCURSEL, -10, 0);
        selectionIndex = SendMessage(hTab, TCM_GETCURSEL, 0, 0);
        todo_wine{
            expect(-1, selectionIndex);
        }

        /* Testing CurSel with value larger than number of tabs */
        selectionIndex = SendMessage(hTab, TCM_SETCURSEL, 1, 0);
        todo_wine{
            expect(-1, selectionIndex);
        }
        selectionIndex = SendMessage(hTab, TCM_SETCURSEL, nTabs+1, 0);
            expect(-1, selectionIndex);
        selectionIndex = SendMessage(hTab, TCM_GETCURFOCUS, 0, 0);
        todo_wine{
            expect(1, selectionIndex);
        }
    }

    /* Testing ExtendedStyle */
    {
        DWORD prevExtendedStyle;
        DWORD extendedStyle;

        /* Testing Flat Seperators */
        extendedStyle = SendMessage(hTab, TCM_GETEXTENDEDSTYLE, 0, 0);
        prevExtendedStyle = SendMessage(hTab, TCM_SETEXTENDEDSTYLE, 0, TCS_EX_FLATSEPARATORS);
            expect(extendedStyle, prevExtendedStyle);

        extendedStyle = SendMessage(hTab, TCM_GETEXTENDEDSTYLE, 0, 0);
        todo_wine{
            expect(TCS_EX_FLATSEPARATORS, extendedStyle);
        }

        /* Testing Register Drop */
        prevExtendedStyle = SendMessage(hTab, TCM_SETEXTENDEDSTYLE, 0, TCS_EX_REGISTERDROP);
            expect(extendedStyle, prevExtendedStyle);

        extendedStyle = SendMessage(hTab, TCM_GETEXTENDEDSTYLE, 0, 0);
        todo_wine{
            expect(TCS_EX_REGISTERDROP, extendedStyle);
        }
    }

    /* Testing UnicodeFormat */
    {
        INT unicodeFormat;

        unicodeFormat = SendMessage(hTab, TCM_SETUNICODEFORMAT, TRUE, 0);
        todo_wine{
            expect(0, unicodeFormat);
        }
        unicodeFormat = SendMessage(hTab, TCM_GETUNICODEFORMAT, 0, 0);
            expect(1, unicodeFormat);

        unicodeFormat = SendMessage(hTab, TCM_SETUNICODEFORMAT, FALSE, 0);
            expect(1, unicodeFormat);
        unicodeFormat = SendMessage(hTab, TCM_GETUNICODEFORMAT, 0, 0);
            expect(0, unicodeFormat);

        unicodeFormat = SendMessage(hTab, TCM_SETUNICODEFORMAT, TRUE, 0);
            expect(0, unicodeFormat);
    }

    /* Testing GetSet Item */
    {
        TCITEM tcItem;
        char szText[32] = "New Label";

        tcItem.mask = TCIF_TEXT;
        tcItem.pszText = &szText[0];
        tcItem.cchTextMax = sizeof(szText);

        ok ( SendMessage(hTab, TCM_SETITEM, 0, (LPARAM) &tcItem), "Setting new item failed.\n");
        ok ( SendMessage(hTab, TCM_GETITEM, 0, (LPARAM) &tcItem), "Getting item failed.\n");
        expect_str("New Label", tcItem.pszText);

        ok ( SendMessage(hTab, TCM_GETITEM, 1, (LPARAM) &tcItem), "Getting item failed.\n");
        expect_str("Tab 2", tcItem.pszText);
    }

    /* Testing GetSet ToolTip */
    {
        HWND toolTip;
        char toolTipText[32] = "ToolTip Text Test";

        toolTip = create_tooltip(hTab, toolTipText);
        SendMessage(hTab, TCM_SETTOOLTIPS, (LPARAM) toolTip, 0);
        ok (toolTip == (HWND) SendMessage(hTab,TCM_GETTOOLTIPS,0,0), "ToolTip was set incorrectly.");

        SendMessage(hTab, TCM_SETTOOLTIPS, (LPARAM) NULL, 0);
        ok (NULL  == (HWND) SendMessage(hTab,TCM_GETTOOLTIPS,0,0), "ToolTip was set incorrectly.");
    }

    DestroyWindow(hTab);
}

START_TEST(tab)
{
    LOGFONTA logfont;

    lstrcpyA(logfont.lfFaceName, "Arial");
    memset(&logfont, 0, sizeof(logfont));
    logfont.lfHeight = -12;
    logfont.lfWeight = FW_NORMAL;
    logfont.lfCharSet = ANSI_CHARSET;
    hFont = CreateFontIndirectA(&logfont);

    InitCommonControls();

    trace ("Testing with default MinWidth\n");
    test_tab(-1);
    trace ("Testing with MinWidth set to -3\n");
    test_tab(-3);
    trace ("Testing with MinWidth set to 24\n");
    test_tab(24);
    trace ("Testing with MinWidth set to 54\n");
    test_tab(54);
    trace ("Testing with MinWidth set to 94\n");
    test_tab(94);

    /* Testing getters and setters with 5 tabs */
    test_getters_setters(5);
}
