/* Unit test suite for edit control.
 *
 * Copyright 2004 Vitaliy Margolen
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

#include <assert.h>
#include <windows.h>
#include <commctrl.h>

#include "wine/test.h"

#ifndef ES_COMBO
#define ES_COMBO 0x200
#endif


HWND create_editcontrol (DWORD style, DWORD exstyle)
{
    HWND handle;

    handle = CreateWindowEx(exstyle,
			  "EDIT",
			  NULL,
			  ES_AUTOHSCROLL | ES_AUTOVSCROLL | style,
			  10, 10, 300, 300,
			  NULL, NULL, NULL, NULL);
    assert (handle);
    if (winetest_interactive)
	ShowWindow (handle, SW_SHOW);
    return handle;
}

static LONG get_edit_style (HWND hwnd)
{
    return GetWindowLongA( hwnd, GWL_STYLE ) & (
	ES_LEFT |
/* FIXME: not implemented
	ES_CENTER |
	ES_RIGHT |
	ES_OEMCONVERT |
*/
	ES_MULTILINE |
	ES_UPPERCASE |
	ES_LOWERCASE |
	ES_PASSWORD |
	ES_AUTOVSCROLL |
	ES_AUTOHSCROLL |
	ES_NOHIDESEL |
	ES_COMBO |
	ES_READONLY |
	ES_WANTRETURN |
	ES_NUMBER
	);
}

static void set_client_height(HWND Wnd, unsigned Height)
{
    RECT ClientRect, WindowRect;

    GetWindowRect(Wnd, &WindowRect);
    GetClientRect(Wnd, &ClientRect);
    SetWindowPos(Wnd, NULL, WindowRect.left, WindowRect.top,
                 WindowRect.right - WindowRect.left,
                 Height + (WindowRect.bottom - WindowRect.top) - (ClientRect.bottom - ClientRect.top),
                 SWP_NOMOVE | SWP_NOZORDER);
}

static void test_edit_control(void)
{
    HWND hwEdit;
    MSG msMessage;
    int i;
    LONG r;
    HFONT Font, OldFont;
    HDC Dc;
    TEXTMETRIC Metrics;
    RECT FormatRect;

    msMessage.message = WM_KEYDOWN;

    trace("EDIT: Single line\n");
    hwEdit = create_editcontrol(0, 0);
    r = get_edit_style(hwEdit);
    ok(r == (ES_AUTOVSCROLL | ES_AUTOHSCROLL), "Wrong style expected 0xc0 got: 0x%lx\n", r); 
    for (i=0;i<65535;i++)
    {
	msMessage.wParam = i;
	r = SendMessage(hwEdit, WM_GETDLGCODE, 0, (LPARAM) &msMessage);
	ok(r == (DLGC_WANTCHARS | DLGC_HASSETSEL | DLGC_WANTARROWS),
	    "Expected DLGC_WANTCHARS | DLGC_HASSETSEL | DLGC_WANTARROWS got %lx\n", r);
    }
    DestroyWindow (hwEdit);

    trace("EDIT: Single line want returns\n");
    hwEdit = create_editcontrol(ES_WANTRETURN, 0);
    r = get_edit_style(hwEdit);
    ok(r == (ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_WANTRETURN), "Wrong style expected 0x10c0 got: 0x%lx\n", r); 
    for (i=0;i<65535;i++)
    {
	msMessage.wParam = i;
	r = SendMessage(hwEdit, WM_GETDLGCODE, 0, (LPARAM) &msMessage);
	ok(r == (DLGC_WANTCHARS | DLGC_HASSETSEL | DLGC_WANTARROWS),
	    "Expected DLGC_WANTCHARS | DLGC_HASSETSEL | DLGC_WANTARROWS got %lx\n", r);
    }
    DestroyWindow (hwEdit);

    trace("EDIT: Multiline line\n");
    hwEdit = create_editcontrol(ES_MULTILINE | WS_VSCROLL | ES_AUTOVSCROLL, 0);
    r = get_edit_style(hwEdit);
    ok(r == (ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_MULTILINE), "Wrong style expected 0xc4 got: 0x%lx\n", r); 
    for (i=0;i<65535;i++)
    {
	msMessage.wParam = i;
	r = SendMessage(hwEdit, WM_GETDLGCODE, 0, (LPARAM) &msMessage);
	ok(r == (DLGC_WANTCHARS | DLGC_HASSETSEL | DLGC_WANTALLKEYS | DLGC_WANTARROWS),
	    "Expected DLGC_WANTCHARS | DLGC_HASSETSEL | DLGC_WANTALLKEYS | DLGC_WANTARROWS got %lx\n", r);
    }
    DestroyWindow (hwEdit);

    trace("EDIT: Multi line want returns\n");
    hwEdit = create_editcontrol(ES_MULTILINE | WS_VSCROLL | ES_AUTOVSCROLL | ES_WANTRETURN, 0);
    r = get_edit_style(hwEdit);
    ok(r == (ES_WANTRETURN | ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_MULTILINE), "Wrong style expected 0x10c4 got: 0x%lx\n", r); 
    for (i=0;i<65535;i++)
    {
	msMessage.wParam = i;
	r = SendMessage(hwEdit, WM_GETDLGCODE, 0, (LPARAM) &msMessage);
	ok(r == (DLGC_WANTCHARS | DLGC_HASSETSEL | DLGC_WANTALLKEYS | DLGC_WANTARROWS),
	    "Expected DLGC_WANTCHARS | DLGC_HASSETSEL | DLGC_WANTALLKEYS | DLGC_WANTARROWS got %lx\n", r);
    }
    DestroyWindow (hwEdit);

    /* Get a stock font for which we can determine the metrics */
    Font = GetStockObject(SYSTEM_FONT);
    assert(NULL != Font);
    Dc = GetDC(NULL);
    assert(NULL != Dc);
    OldFont = SelectObject(Dc, Font);
    assert(NULL != OldFont);
    if (! GetTextMetrics(Dc, &Metrics))
    {
	assert(FALSE);
    }
    SelectObject(Dc, OldFont);
    ReleaseDC(NULL, Dc);

    trace("EDIT: vertical text position\n");
    hwEdit = create_editcontrol(WS_POPUP, 0);
    SendMessage(hwEdit, WM_SETFONT, (WPARAM) Font, (LPARAM) FALSE);
    set_client_height(hwEdit, Metrics.tmHeight - 1);
    SendMessage(hwEdit, EM_GETRECT, 0, (LPARAM) &FormatRect);
    ok(0 == FormatRect.top, "wrong vertical position expected 0 got %ld\n", FormatRect.top);
    ok(Metrics.tmHeight - 1 == FormatRect.bottom - FormatRect.top, "wrong height expected %ld got %ld\n", Metrics.tmHeight - 1, FormatRect.bottom - FormatRect.top);
    set_client_height(hwEdit, Metrics.tmHeight);
    SendMessage(hwEdit, EM_GETRECT, 0, (LPARAM) &FormatRect);
    ok(0 == FormatRect.top, "wrong vertical position expected 0 got %ld\n", FormatRect.top);
    ok(Metrics.tmHeight == FormatRect.bottom - FormatRect.top, "wrong height expected %ld got %ld\n", Metrics.tmHeight, FormatRect.bottom - FormatRect.top);
    set_client_height(hwEdit, Metrics.tmHeight + 1);
    SendMessage(hwEdit, EM_GETRECT, 0, (LPARAM) &FormatRect);
    ok(0 == FormatRect.top, "wrong vertical position expected 0 got %ld\n", FormatRect.top);
    ok(Metrics.tmHeight == FormatRect.bottom - FormatRect.top, "wrong height expected %ld got %ld\n", Metrics.tmHeight, FormatRect.bottom - FormatRect.top);
    set_client_height(hwEdit, Metrics.tmHeight + 2);
    SendMessage(hwEdit, EM_GETRECT, 0, (LPARAM) &FormatRect);
    ok(0 == FormatRect.top, "wrong vertical position expected 0 got %ld\n", FormatRect.top);
    ok(Metrics.tmHeight == FormatRect.bottom - FormatRect.top, "wrong height expected %ld got %ld\n", Metrics.tmHeight, FormatRect.bottom - FormatRect.top);
    set_client_height(hwEdit, Metrics.tmHeight + 10);
    SendMessage(hwEdit, EM_GETRECT, 0, (LPARAM) &FormatRect);
    ok(0 == FormatRect.top, "wrong vertical position expected 0 got %ld\n", FormatRect.top);
    ok(Metrics.tmHeight == FormatRect.bottom - FormatRect.top, "wrong height expected %ld got %ld\n", Metrics.tmHeight, FormatRect.bottom - FormatRect.top);
    DestroyWindow(hwEdit);

    hwEdit = create_editcontrol(WS_POPUP | WS_BORDER, 0);
    SendMessage(hwEdit, WM_SETFONT, (WPARAM) Font, (LPARAM) FALSE);
    set_client_height(hwEdit, Metrics.tmHeight - 1);
    SendMessage(hwEdit, EM_GETRECT, 0, (LPARAM) &FormatRect);
    ok(0 == FormatRect.top, "wrong vertical position expected 0 got %ld\n", FormatRect.top);
    ok(Metrics.tmHeight - 1 == FormatRect.bottom - FormatRect.top, "wrong height expected %ld got %ld\n", Metrics.tmHeight - 1, FormatRect.bottom - FormatRect.top);
    set_client_height(hwEdit, Metrics.tmHeight);
    SendMessage(hwEdit, EM_GETRECT, 0, (LPARAM) &FormatRect);
    ok(0 == FormatRect.top, "wrong vertical position expected 0 got %ld\n", FormatRect.top);
    ok(Metrics.tmHeight == FormatRect.bottom - FormatRect.top, "wrong height expected %ld got %ld\n", Metrics.tmHeight, FormatRect.bottom - FormatRect.top);
    set_client_height(hwEdit, Metrics.tmHeight + 2);
    SendMessage(hwEdit, EM_GETRECT, 0, (LPARAM) &FormatRect);
    ok(0 == FormatRect.top, "wrong vertical position expected 0 got %ld\n", FormatRect.top);
    ok(Metrics.tmHeight == FormatRect.bottom - FormatRect.top, "wrong height expected %ld got %ld\n", Metrics.tmHeight, FormatRect.bottom - FormatRect.top);
    set_client_height(hwEdit, Metrics.tmHeight + 3);
    SendMessage(hwEdit, EM_GETRECT, 0, (LPARAM) &FormatRect);
    ok(0 == FormatRect.top, "wrong vertical position expected 0 got %ld\n", FormatRect.top);
    ok(Metrics.tmHeight == FormatRect.bottom - FormatRect.top, "wrong height expected %ld got %ld\n", Metrics.tmHeight, FormatRect.bottom - FormatRect.top);
    set_client_height(hwEdit, Metrics.tmHeight + 4);
    SendMessage(hwEdit, EM_GETRECT, 0, (LPARAM) &FormatRect);
    ok(2 == FormatRect.top, "wrong vertical position expected 2 got %ld\n", FormatRect.top);
    ok(Metrics.tmHeight == FormatRect.bottom - FormatRect.top, "wrong height expected %ld got %ld\n", Metrics.tmHeight, FormatRect.bottom - FormatRect.top);
    set_client_height(hwEdit, Metrics.tmHeight + 10);
    SendMessage(hwEdit, EM_GETRECT, 0, (LPARAM) &FormatRect);
    ok(2 == FormatRect.top, "wrong vertical position expected 0 got %ld\n", FormatRect.top);
    ok(Metrics.tmHeight == FormatRect.bottom - FormatRect.top, "wrong height expected %ld got %ld\n", Metrics.tmHeight, FormatRect.bottom - FormatRect.top);
    DestroyWindow(hwEdit);

    hwEdit = create_editcontrol(WS_POPUP, WS_EX_CLIENTEDGE);
    SendMessage(hwEdit, WM_SETFONT, (WPARAM) Font, (LPARAM) FALSE);
    set_client_height(hwEdit, Metrics.tmHeight - 1);
    SendMessage(hwEdit, EM_GETRECT, 0, (LPARAM) &FormatRect);
    ok(0 == FormatRect.top, "wrong vertical position expected 0 got %ld\n", FormatRect.top);
    ok(Metrics.tmHeight - 1 == FormatRect.bottom - FormatRect.top, "wrong height expected %ld got %ld\n", Metrics.tmHeight - 1, FormatRect.bottom - FormatRect.top);
    set_client_height(hwEdit, Metrics.tmHeight);
    SendMessage(hwEdit, EM_GETRECT, 0, (LPARAM) &FormatRect);
    ok(0 == FormatRect.top, "wrong vertical position expected 0 got %ld\n", FormatRect.top);
    ok(Metrics.tmHeight == FormatRect.bottom - FormatRect.top, "wrong height expected %ld got %ld\n", Metrics.tmHeight, FormatRect.bottom - FormatRect.top);
    set_client_height(hwEdit, Metrics.tmHeight + 1);
    SendMessage(hwEdit, EM_GETRECT, 0, (LPARAM) &FormatRect);
    ok(0 == FormatRect.top, "wrong vertical position expected 0 got %ld\n", FormatRect.top);
    ok(Metrics.tmHeight == FormatRect.bottom - FormatRect.top, "wrong height expected %ld got %ld\n", Metrics.tmHeight, FormatRect.bottom - FormatRect.top);
    set_client_height(hwEdit, Metrics.tmHeight + 2);
    SendMessage(hwEdit, EM_GETRECT, 0, (LPARAM) &FormatRect);
    ok(1 == FormatRect.top, "wrong vertical position expected 1 got %ld\n", FormatRect.top);
    ok(Metrics.tmHeight == FormatRect.bottom - FormatRect.top, "wrong height expected %ld got %ld\n", Metrics.tmHeight, FormatRect.bottom - FormatRect.top);
    set_client_height(hwEdit, Metrics.tmHeight + 4);
    SendMessage(hwEdit, EM_GETRECT, 0, (LPARAM) &FormatRect);
    ok(1 == FormatRect.top, "wrong vertical position expected 1 got %ld\n", FormatRect.top);
    ok(Metrics.tmHeight == FormatRect.bottom - FormatRect.top, "wrong height expected %ld got %ld\n", Metrics.tmHeight, FormatRect.bottom - FormatRect.top);
    set_client_height(hwEdit, Metrics.tmHeight + 10);
    SendMessage(hwEdit, EM_GETRECT, 0, (LPARAM) &FormatRect);
    ok(1 == FormatRect.top, "wrong vertical position expected 1 got %ld\n", FormatRect.top);
    ok(Metrics.tmHeight == FormatRect.bottom - FormatRect.top, "wrong height expected %ld got %ld\n", Metrics.tmHeight, FormatRect.bottom - FormatRect.top);
    DestroyWindow(hwEdit);

    hwEdit = create_editcontrol(WS_POPUP | WS_BORDER, WS_EX_CLIENTEDGE);
    SendMessage(hwEdit, WM_SETFONT, (WPARAM) Font, (LPARAM) FALSE);
    set_client_height(hwEdit, Metrics.tmHeight - 1);
    SendMessage(hwEdit, EM_GETRECT, 0, (LPARAM) &FormatRect);
    ok(0 == FormatRect.top, "wrong vertical position expected 0 got %ld\n", FormatRect.top);
    ok(Metrics.tmHeight - 1 == FormatRect.bottom - FormatRect.top, "wrong height expected %ld got %ld\n", Metrics.tmHeight - 1, FormatRect.bottom - FormatRect.top);
    set_client_height(hwEdit, Metrics.tmHeight);
    SendMessage(hwEdit, EM_GETRECT, 0, (LPARAM) &FormatRect);
    ok(0 == FormatRect.top, "wrong vertical position expected 0 got %ld\n", FormatRect.top);
    ok(Metrics.tmHeight == FormatRect.bottom - FormatRect.top, "wrong height expected %ld got %ld\n", Metrics.tmHeight, FormatRect.bottom - FormatRect.top);
    set_client_height(hwEdit, Metrics.tmHeight + 1);
    SendMessage(hwEdit, EM_GETRECT, 0, (LPARAM) &FormatRect);
    ok(0 == FormatRect.top, "wrong vertical position expected 0 got %ld\n", FormatRect.top);
    ok(Metrics.tmHeight == FormatRect.bottom - FormatRect.top, "wrong height expected %ld got %ld\n", Metrics.tmHeight, FormatRect.bottom - FormatRect.top);
    set_client_height(hwEdit, Metrics.tmHeight + 2);
    SendMessage(hwEdit, EM_GETRECT, 0, (LPARAM) &FormatRect);
    ok(1 == FormatRect.top, "wrong vertical position expected 1 got %ld\n", FormatRect.top);
    ok(Metrics.tmHeight == FormatRect.bottom - FormatRect.top, "wrong height expected %ld got %ld\n", Metrics.tmHeight, FormatRect.bottom - FormatRect.top);
    set_client_height(hwEdit, Metrics.tmHeight + 4);
    SendMessage(hwEdit, EM_GETRECT, 0, (LPARAM) &FormatRect);
    ok(1 == FormatRect.top, "wrong vertical position expected 1 got %ld\n", FormatRect.top);
    ok(Metrics.tmHeight == FormatRect.bottom - FormatRect.top, "wrong height expected %ld got %ld\n", Metrics.tmHeight, FormatRect.bottom - FormatRect.top);
    set_client_height(hwEdit, Metrics.tmHeight + 10);
    SendMessage(hwEdit, EM_GETRECT, 0, (LPARAM) &FormatRect);
    ok(1 == FormatRect.top, "wrong vertical position expected 1 got %ld\n", FormatRect.top);
    ok(Metrics.tmHeight == FormatRect.bottom - FormatRect.top, "wrong height expected %ld got %ld\n", Metrics.tmHeight, FormatRect.bottom - FormatRect.top);
    DestroyWindow(hwEdit);
}

START_TEST(edit)
{
    test_edit_control();
}
