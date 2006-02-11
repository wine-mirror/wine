/*
* Unit test suite for rich edit control
*
* Copyright 2006 Google (Thomas Kho)
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

#include <wine/test.h>
#include <windows.h>
#include <richedit.h>
#include <time.h>

static HMODULE hmoduleRichEdit;
static const char haystack[] = "Think of Wine as a compatibility layer for "
  "running Windows programs. Wine does not require Microsoft Windows, as it "
  "is a completely free alternative implementation of the Windows API "
  "consisting of 100% non-Microsoft code, however Wine can optionally use "
  "native Windows DLLs if they are available. Wine provides both a "
  "development toolkit for porting Windows source code to Unix as well as a "
  "program loader, allowing many unmodified Windows programs to run on "
  "x86-based Unixes, including Linux, FreeBSD, and Solaris.";

static HWND new_window(LPCTSTR lpClassName, DWORD dwStyle, HWND parent) {
  HWND hwnd;
  hwnd = CreateWindow(lpClassName, NULL, dwStyle|WS_POPUP|WS_HSCROLL|WS_VSCROLL
                      |WS_VISIBLE, 0, 0, 200, 50, parent, NULL,
                      hmoduleRichEdit, NULL);
  ok(hwnd != NULL, "class: %s, error: %d\n", lpClassName, (int) GetLastError());
  return hwnd;
}

static HWND new_richedit(HWND parent) {
  return new_window(RICHEDIT_CLASS, ES_MULTILINE, parent);
}

static void check_EM_FINDTEXT(HWND hwnd, int start, int end, char needle[],
                              int flags, int expected_start) {
  int findloc;
  FINDTEXT ft;
  memset(&ft, 0, sizeof(ft));
  ft.chrg.cpMin = start;
  ft.chrg.cpMax = end;
  ft.lpstrText = needle;
  findloc = SendMessage(hwnd, EM_FINDTEXT, flags, (LPARAM) &ft);
  ok(findloc == expected_start,
     "EM_FINDTEXT '%s' in range (%d,%d), flags %08x, got start at %d\n",
     needle, start, end, flags, findloc);
}

static void test_EM_FINDTEXT(void)
{
  CHARRANGE cr;
  GETTEXTLENGTHEX gtl;
  int size;

  HWND hwndRichEdit = new_richedit(NULL);

  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) haystack);
  SendMessage(hwndRichEdit, EM_EXGETSEL, 0, (LPARAM)&cr);
  ok(cr.cpMin == cr.cpMax, "(%ld,%ld)\n", cr.cpMin, cr.cpMax);

  gtl.flags = GTL_NUMCHARS;
  gtl.codepage = CP_ACP;
  size = SendMessage(hwndRichEdit, EM_GETTEXTLENGTHEX, (WPARAM) &gtl, 0);
  ok(size == sizeof(haystack) - 1, "size=%d, sizeof haystack=%d\n",
     size, sizeof(haystack)); /* sizeof counts '\0' */

  check_EM_FINDTEXT(hwndRichEdit, 0, size, "Wine", FR_DOWN | FR_MATCHCASE, 9);
  check_EM_FINDTEXT(hwndRichEdit, 10, size, "Wine", FR_DOWN | FR_MATCHCASE, 69);
  check_EM_FINDTEXT(hwndRichEdit, 298, size, "Wine", FR_DOWN | FR_MATCHCASE,
                    -1);
  check_EM_FINDTEXT(hwndRichEdit, 0, size, "wine", FR_DOWN | FR_MATCHCASE, -1);
  todo_wine {
    check_EM_FINDTEXT(hwndRichEdit, 0, size, "wine", FR_DOWN | FR_WHOLEWORD, 9);
    check_EM_FINDTEXT(hwndRichEdit, 0, size, "win", FR_DOWN | FR_WHOLEWORD, -1);
  }

  /* Check the case noted in bug 4479 */
  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) "blahblah");
  check_EM_FINDTEXT(hwndRichEdit, 0, 8, "blah", FR_DOWN | FR_MATCHCASE, 0);
  check_EM_FINDTEXT(hwndRichEdit, 4, 8, "blah", FR_DOWN | FR_MATCHCASE, 4);
  check_EM_FINDTEXT(hwndRichEdit, 4, 9, "blah", FR_DOWN | FR_MATCHCASE, 4);
  check_EM_FINDTEXT(hwndRichEdit, 0, 8, "blahblah", FR_DOWN | FR_MATCHCASE, 0);

  DestroyWindow(hwndRichEdit);
}

static void check_EM_FINDTEXTEX(HWND hwnd, int start, int end, char needle[],
                                int flags, int expected_start,
                                int expected_end) {
  int findloc;
  FINDTEXTEX ft;
  memset(&ft, 0, sizeof(ft));
  ft.chrg.cpMin = start;
  ft.chrg.cpMax = end;
  ft.lpstrText = needle;
  findloc = SendMessage(hwnd, EM_FINDTEXTEX, flags, (LPARAM) &ft);
  ok(findloc == expected_start,
     "EM_FINDTEXTEX '%s' in range (%d,%d), flags %08x, got start at %d\n",
     needle, start, end, flags, findloc);
  if(findloc != -1) {
    ok(ft.chrgText.cpMin == expected_start,
       "EM_FINDTEXTEX '%s' in range (%d,%d), flags %08x, got start at %ld\n",
       needle, start, end, flags, ft.chrgText.cpMin);
    ok(ft.chrgText.cpMax == expected_end,
       "EM_FINDTEXTEX '%s' in range (%d,%d), flags %08x, got end at %ld\n",
       needle, start, end, flags, ft.chrgText.cpMax);
  }
}

static void test_EM_FINDTEXTEX(void)
{
  CHARRANGE cr;
  GETTEXTLENGTHEX gtl;
  int size;

  HWND hwndRichEdit = new_richedit(NULL);

  SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) haystack);
  SendMessage(hwndRichEdit, EM_EXGETSEL, 0, (LPARAM) &cr);
  ok(cr.cpMin == cr.cpMax, "(%ld,%ld)\n", cr.cpMin, cr.cpMax);

  gtl.flags = GTL_NUMCHARS;
  gtl.codepage = CP_ACP;
  size = SendMessage(hwndRichEdit, EM_GETTEXTLENGTHEX, (WPARAM) &gtl, 0);
  ok(size == sizeof(haystack) - 1, "size=%d, sizeof haystack=%d\n", size,
     sizeof(haystack)); /* sizeof counts '\0' */

  check_EM_FINDTEXTEX(hwndRichEdit, 0, size, "Wine", FR_DOWN | FR_MATCHCASE,
                      9, 13);
  check_EM_FINDTEXTEX(hwndRichEdit, 10, size, "Wine", FR_DOWN | FR_MATCHCASE,
                      69, 73);
  check_EM_FINDTEXTEX(hwndRichEdit, 298, size, "Wine", FR_DOWN | FR_MATCHCASE,
                      -1, -1);
  check_EM_FINDTEXTEX(hwndRichEdit, 0, size, "wine", FR_DOWN | FR_MATCHCASE,
                      -1, -1);
  todo_wine {
    check_EM_FINDTEXTEX(hwndRichEdit, 0, size, "wine", FR_DOWN | FR_WHOLEWORD,
                        9, 13);
    check_EM_FINDTEXTEX(hwndRichEdit, 0, size, "win", FR_DOWN | FR_WHOLEWORD,
                        -1, -1);
  }

  DestroyWindow(hwndRichEdit);
}

START_TEST( editor )
{
  MSG msg;
  time_t end;

  /* Must explicitly LoadLibrary(). The test has no references to functions in
   * RICHED20.DLL, so the linker doesn't actually link to it. */
  hmoduleRichEdit = LoadLibrary("RICHED20.DLL");
  ok(hmoduleRichEdit != NULL, "error: %d\n", (int) GetLastError());

  test_EM_FINDTEXT();
  test_EM_FINDTEXTEX();

  /* Set the environment variable WINETEST_RICHED20 to keep windows
   * responsive and open for 30 seconds. This is useful for debugging.
   *
   * The message pump uses PeekMessage() to empty the queue and then sleeps for
   * 50ms before retrying the queue. */
  end = time(NULL) + 30;
  if (getenv( "WINETEST_RICHED20" )) {
    while (time(NULL) < end) {
      if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      } else {
        Sleep(50);
      }
    }
  }

  ok(FreeLibrary(hmoduleRichEdit) != 0, "error: %d\n", (int) GetLastError());
}
