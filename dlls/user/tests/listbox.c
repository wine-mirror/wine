/* Unit test suite for list boxes.
 *
 * Copyright 2003 Ferenc Wagner
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

#include "wine/test.h"

#ifdef VISIBLE
#define WAIT Sleep (1000)
#define REDRAW RedrawWindow (handle, NULL, 0, RDW_UPDATENOW)
#else
#define WAIT
#define REDRAW
#endif

static const char *strings[4] = {
  "First added",
  "Second added",
  "Third added",
  "Fourth added which is very long because at some time we only had a 256 byte character buffer and that was overflowing in one of those applications that had a common dialog file open box and tried to add a 300 characters long custom filter string which of course the code did not like and crashed. Just make sure this string is longer than 256 characters."
};

static HWND
create_listbox (DWORD add_style)
{
  HWND handle=CreateWindow ("LISTBOX", "TestList",
                            (LBS_STANDARD & ~LBS_SORT) | add_style,
                            0, 0, 100, 100,
                            NULL, NULL, NULL, 0);

  assert (handle);
  SendMessage (handle, LB_ADDSTRING, 0, (LPARAM) (LPCTSTR) strings[0]);
  SendMessage (handle, LB_ADDSTRING, 0, (LPARAM) (LPCTSTR) strings[1]);
  SendMessage (handle, LB_ADDSTRING, 0, (LPARAM) (LPCTSTR) strings[2]);
  SendMessage (handle, LB_ADDSTRING, 0, (LPARAM) (LPCTSTR) strings[3]);

#ifdef VISIBLE
  ShowWindow (handle, SW_SHOW);
#endif
  REDRAW;

  return handle;
}

struct listbox_prop {
  DWORD add_style;
};

struct listbox_stat {
  int selected, anchor, caret, selcount;
};

struct listbox_test {
  struct listbox_prop prop;
  struct listbox_stat  init,  init_todo;
  struct listbox_stat click, click_todo;
  struct listbox_stat  step,  step_todo;
  struct listbox_stat   sel,   sel_todo;
};

static void
listbox_query (HWND handle, struct listbox_stat *results)
{
  results->selected = SendMessage (handle, LB_GETCURSEL, 0, 0);
  results->anchor   = SendMessage (handle, LB_GETANCHORINDEX, 0, 0);
  results->caret    = SendMessage (handle, LB_GETCARETINDEX, 0, 0);
  results->selcount = SendMessage (handle, LB_GETSELCOUNT, 0, 0);
}

static void
buttonpress (HWND handle, WORD x, WORD y)
{
  LPARAM lp=x+(y<<16);

  WAIT;
  SendMessage (handle, WM_LBUTTONDOWN, (WPARAM) MK_LBUTTON, lp);
  SendMessage (handle, WM_LBUTTONUP  , (WPARAM) 0         , lp);
  REDRAW;
}

static void
keypress (HWND handle, WPARAM keycode, BYTE scancode, BOOL extended)
{
  LPARAM lp=1+(scancode<<16)+(extended?KEYEVENTF_EXTENDEDKEY:0);

  WAIT;
  SendMessage (handle, WM_KEYDOWN, keycode, lp);
  SendMessage (handle, WM_KEYUP  , keycode, lp | 0xc000000);
  REDRAW;
}

#define listbox_field_ok(t, s, f, got) \
  ok (t.s.f==got.f, "style %#x, step " #s ", field " #f \
      ": expected %d, got %d\n", (unsigned int)t.prop.add_style, \
      t.s.f, got.f)

#define listbox_todo_field_ok(t, s, f, got) \
  if (t.s##_todo.f) todo_wine { listbox_field_ok(t, s, f, got); } \
  else listbox_field_ok(t, s, f, got)

#define listbox_ok(t, s, got) \
  listbox_todo_field_ok(t, s, selected, got); \
  listbox_todo_field_ok(t, s, anchor, got); \
  listbox_todo_field_ok(t, s, caret, got); \
  listbox_todo_field_ok(t, s, selcount, got)

static void
check (const struct listbox_test test)
{
  struct listbox_stat answer;
  HWND hLB=create_listbox (test.prop.add_style);
  RECT second_item;
  int i;

  listbox_query (hLB, &answer);
  listbox_ok (test, init, answer);

  SendMessage (hLB, LB_GETITEMRECT, (WPARAM) 1, (LPARAM) &second_item);
  buttonpress(hLB, (WORD)second_item.left, (WORD)second_item.top);

  listbox_query (hLB, &answer);
  listbox_ok (test, click, answer);

  keypress (hLB, VK_DOWN, 0x50, TRUE);

  listbox_query (hLB, &answer);
  listbox_ok (test, step, answer);

  DestroyWindow (hLB);
  hLB=create_listbox (test.prop.add_style);

  SendMessage (hLB, LB_SELITEMRANGE, TRUE, MAKELPARAM(1, 2));
  listbox_query (hLB, &answer);
  listbox_ok (test, sel, answer);

  for (i=0;i<4;i++) {
	DWORD size = SendMessage (hLB, LB_GETTEXTLEN, i, 0);
	CHAR *txt;
	WCHAR *txtw;

	txt = HeapAlloc (GetProcessHeap(), 0, size+1);
	SendMessageA(hLB, LB_GETTEXT, i, (LPARAM)txt);
        ok(!strcmp (txt, strings[i]), "returned string for item %d does not match %s vs %s\n", i, txt, strings[i]);

	txtw = HeapAlloc (GetProcessHeap(), 0, 2*size+2);
	SendMessageW(hLB, LB_GETTEXT, i, (LPARAM)txtw);
	WideCharToMultiByte( CP_ACP, 0, txtw, -1, txt, size, NULL, NULL );
        ok(!strcmp (txt, strings[i]), "returned string for item %d does not match %s vs %s\n", i, txt, strings[i]);

	HeapFree (GetProcessHeap(), 0, txtw);
	HeapFree (GetProcessHeap(), 0, txt);
  }
  
  WAIT;
  DestroyWindow (hLB);
}

static void check_item_height(void)
{
    HWND hLB;
    HDC hdc;
    HFONT font;
    TEXTMETRICW tm;
    INT itemHeight;

    hLB = create_listbox (0);
    ok ((hdc = GetDCEx( hLB, 0, DCX_CACHE )) != 0, "Can't get hdc\n");
    ok ((font = GetCurrentObject(hdc, OBJ_FONT)) != 0, "Can't get the current font\n");
    ok (GetTextMetricsW( hdc, &tm ), "Can't read font metrics\n");
    ReleaseDC( hLB, hdc);

    ok (SendMessageW(hLB, WM_SETFONT, (WPARAM)font, 0) == 0, "Can't set font\n");

    itemHeight = SendMessageW(hLB, LB_GETITEMHEIGHT, 0, 0);
    ok (itemHeight == tm.tmHeight, "Item height wrong, got %d, expecting %ld\n", itemHeight, tm.tmHeight);

    DestroyWindow (hLB);
}

START_TEST(listbox)
{
  const struct listbox_test SS =
/*   {add_style} */
    {{0},
     {LB_ERR, LB_ERR,      0, LB_ERR}, {0,0,0,0},
     {     1,      1,      1, LB_ERR}, {0,0,0,0},
     {     2,      2,      2, LB_ERR}, {0,0,0,0},
     {LB_ERR, LB_ERR,      0, LB_ERR}, {0,0,0,0}};
/* {selected, anchor,  caret, selcount}{TODO fields} */
  const struct listbox_test SS_NS =
    {{LBS_NOSEL},
     {LB_ERR, LB_ERR,      0, LB_ERR}, {0,0,0,0},
     {     1,      1,      1, LB_ERR}, {0,0,0,0},
     {     2,      2,      2, LB_ERR}, {0,0,0,0},
     {LB_ERR, LB_ERR,      0, LB_ERR}, {0,0,0,0}};
  const struct listbox_test MS =
    {{LBS_MULTIPLESEL},
     {     0, LB_ERR,      0,      0}, {0,0,0,0},
     {     1,      1,      1,      1}, {0,0,0,0},
     {     2,      1,      2,      1}, {0,0,0,0},
     {     0, LB_ERR,      0,      2}, {0,0,0,0}};
  const struct listbox_test MS_NS =
    {{LBS_MULTIPLESEL | LBS_NOSEL},
     {LB_ERR, LB_ERR,      0, LB_ERR}, {0,0,0,0},
     {     1,      1,      1, LB_ERR}, {0,0,0,0},
     {     2,      2,      2, LB_ERR}, {0,0,0,0},
     {LB_ERR, LB_ERR,      0, LB_ERR}, {0,0,0,0}};
  const struct listbox_test ES =
    {{LBS_EXTENDEDSEL},
     {     0, LB_ERR,      0,      0}, {0,0,0,0},
     {     1,      1,      1,      1}, {0,0,0,0},
     {     2,      2,      2,      1}, {0,0,0,0},
     {     0, LB_ERR,      0,      2}, {0,0,0,0}};
  const struct listbox_test ES_NS =
    {{LBS_EXTENDEDSEL | LBS_NOSEL},
     {LB_ERR, LB_ERR,      0, LB_ERR}, {0,0,0,0},
     {     1,      1,      1, LB_ERR}, {0,0,0,0},
     {     2,      2,      2, LB_ERR}, {0,0,0,0},
     {LB_ERR, LB_ERR,      0, LB_ERR}, {0,0,0,0}};
  const struct listbox_test EMS =
    {{LBS_EXTENDEDSEL | LBS_MULTIPLESEL},
     {     0, LB_ERR,      0,      0}, {0,0,0,0},
     {     1,      1,      1,      1}, {0,0,0,0},
     {     2,      2,      2,      1}, {0,0,0,0},
     {     0, LB_ERR,      0,      2}, {0,0,0,0}};
  const struct listbox_test EMS_NS =
    {{LBS_EXTENDEDSEL | LBS_MULTIPLESEL | LBS_NOSEL},
     {LB_ERR, LB_ERR,      0, LB_ERR}, {0,0,0,0},
     {     1,      1,      1, LB_ERR}, {0,0,0,0},
     {     2,      2,      2, LB_ERR}, {0,0,0,0},
     {LB_ERR, LB_ERR,      0, LB_ERR}, {0,0,0,0}};

  trace (" Testing single selection...\n");
  check (SS);
  trace (" ... with NOSEL\n");
  check (SS_NS);
  trace (" Testing multiple selection...\n");
  check (MS);
  trace (" ... with NOSEL\n");
  check (MS_NS);
  trace (" Testing extended selection...\n");
  check (ES);
  trace (" ... with NOSEL\n");
  check (ES_NS);
  trace (" Testing extended and multiple selection...\n");
  check (EMS);
  trace (" ... with NOSEL\n");
  check (EMS_NS);

  check_item_height();
}
