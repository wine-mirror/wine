/*
* Unit test suite for rich edit control 1.0
*
* Copyright 2006 Google (Thomas Kho)
* Copyright 2007 Matt Finnicum
* Copyright 2007 Dmitry Timoshkov
* Copyright 2007 Alex Villacís Lasso
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

#include <stdarg.h>
#include <assert.h>
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>
#include <winnls.h>
#include <ole2.h>
#include <richedit.h>
#include <time.h>
#include <wine/test.h>

static HMODULE hmoduleRichEdit;

static HWND new_window(LPCTSTR lpClassName, DWORD dwStyle, HWND parent) {
  HWND hwnd;
  hwnd = CreateWindow(lpClassName, NULL, dwStyle|WS_POPUP|WS_HSCROLL|WS_VSCROLL
                      |WS_VISIBLE, 0, 0, 200, 60, parent, NULL,
                      hmoduleRichEdit, NULL);
  ok(hwnd != NULL, "class: %s, error: %d\n", lpClassName, (int) GetLastError());
  return hwnd;
}

static HWND new_richedit(HWND parent) {
  return new_window(RICHEDIT_CLASS10A, ES_MULTILINE, parent);
}

static void test_WM_SETTEXT()
{
  HWND hwndRichEdit = new_richedit(NULL);
  const char * TestItem1 = "TestSomeText";
  const char * TestItem2 = "TestSomeText\r";
  const char * TestItem3 = "TestSomeText\rSomeMoreText\r";
  const char * TestItem4 = "TestSomeText\n\nTestSomeText";
  const char * TestItem5 = "TestSomeText\r\r\nTestSomeText";
  const char * TestItem6 = "TestSomeText\r\r\n\rTestSomeText";
  const char * TestItem7 = "TestSomeText\r\n\r\r\n\rTestSomeText";
  const char * TestItem8 = "TestSomeText\r\n";
  const char * TestItem9 = "TestSomeText\r\nSomeMoreText\r\n";
  const char * TestItem10 = "TestSomeText\r\n\r\nTestSomeText";
  const char * TestItem11 = "TestSomeText TestSomeText";
  const char * TestItem12 = "TestSomeText \r\nTestSomeText";
  const char * TestItem13 = "TestSomeText\r\n \r\nTestSomeText";
  const char * TestItem14 = "TestSomeText\n";
  const char * TestItem15 = "TestSomeText\r\r\r";
  const char * TestItem16 = "TestSomeText\r\r\rSomeMoreText";
  char buf[1024] = {0};
  LRESULT result;

  /* This test attempts to show that WM_SETTEXT on a riched32 control does not
     attempt to modify the text that is pasted into the control, and should
     return it as is. In particular, \r\r\n is NOT converted, unlike riched20.
     Currently, builtin riched32 mangles solitary \r or \n when not part of
     a \r\n pair.

     For riched32, the rules for breaking lines seem to be the following:
     - \r\n is one line break. This is the normal case.
     - \r{0,N}\n is one line break. In particular, \n by itself is a line break.
     - \n{1,N} are that many line breaks.
     - \r with text or other characters (except \n) past it, is a line break. That
       is, a run of \r{N} without a terminating \n is considered N line breaks
     - \r at the end of the text is NOT a line break. This differs from riched20,
       where \r at the end of the text is a proper line break. This causes
       TestItem2 to fail its test.
   */

#define TEST_SETTEXT(a, b, nlines, is_todo, is_todo2) \
  result = SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) a); \
  ok (result == 1, "WM_SETTEXT returned %ld instead of 1\n", result); \
  result = SendMessage(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM) buf); \
  ok (result == lstrlen(buf), \
	"WM_GETTEXT returned %ld instead of expected %u\n", \
	result, lstrlen(buf)); \
  result = strcmp(b, buf); \
  if (is_todo) todo_wine { \
  ok(result == 0, \
        "WM_SETTEXT round trip: strcmp = %ld\n", result); \
  } else { \
  ok(result == 0, \
        "WM_SETTEXT round trip: strcmp = %ld\n", result); \
  } \
  result = SendMessage(hwndRichEdit, EM_GETLINECOUNT, 0, 0); \
  if (is_todo2) todo_wine { \
  ok(result == nlines, "EM_GETLINECOUNT returned %ld, expected %d\n", result, nlines); \
  } else { \
  ok(result == nlines, "EM_GETLINECOUNT returned %ld, expected %d\n", result, nlines); \
  }

  TEST_SETTEXT(TestItem1, TestItem1, 1, 0, 0)
  TEST_SETTEXT(TestItem2, TestItem2, 1, 1, 1)
  TEST_SETTEXT(TestItem3, TestItem3, 2, 1, 1)
  TEST_SETTEXT(TestItem4, TestItem4, 3, 1, 0)
  TEST_SETTEXT(TestItem5, TestItem5, 2, 1, 0)
  TEST_SETTEXT(TestItem6, TestItem6, 3, 1, 0)
  TEST_SETTEXT(TestItem7, TestItem7, 4, 1, 0)
  TEST_SETTEXT(TestItem8, TestItem8, 2, 0, 0)
  TEST_SETTEXT(TestItem9, TestItem9, 3, 0, 0)
  TEST_SETTEXT(TestItem10, TestItem10, 3, 0, 0)
  TEST_SETTEXT(TestItem11, TestItem11, 1, 0, 0)
  TEST_SETTEXT(TestItem12, TestItem12, 2, 0, 0)
  TEST_SETTEXT(TestItem13, TestItem13, 3, 0, 0)
  TEST_SETTEXT(TestItem14, TestItem14, 2, 1, 0)
  TEST_SETTEXT(TestItem15, TestItem15, 3, 1, 1)
  TEST_SETTEXT(TestItem16, TestItem16, 4, 1, 0)

#undef TEST_SETTEXT
  DestroyWindow(hwndRichEdit);
}

static void test_WM_GETTEXTLENGTH(void)
{
    HWND hwndRichEdit = new_richedit(NULL);
    static const char text3[] = "aaa\r\nbbb\r\nccc\r\nddd\r\neee";
    static const char text4[] = "aaa\r\nbbb\r\nccc\r\nddd\r\neee\r\n";
    int result;

    /* Test for WM_GETTEXTLENGTH */
    SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) text3);
    result = SendMessage(hwndRichEdit, WM_GETTEXTLENGTH, 0, 0);
    ok(result == lstrlen(text3),
        "WM_GETTEXTLENGTH reports incorrect length %d, expected %d\n",
        result, lstrlen(text3));

    SendMessage(hwndRichEdit, WM_SETTEXT, 0, (LPARAM) text4);
    result = SendMessage(hwndRichEdit, WM_GETTEXTLENGTH, 0, 0);
    ok(result == lstrlen(text4),
        "WM_GETTEXTLENGTH reports incorrect length %d, expected %d\n",
        result, lstrlen(text4));

    DestroyWindow(hwndRichEdit);
}

static DWORD CALLBACK test_EM_STREAMIN_esCallback(DWORD_PTR dwCookie,
                                         LPBYTE pbBuff,
                                         LONG cb,
                                         LONG *pcb)
{
  const char** str = (const char**)dwCookie;
  int size = strlen(*str);
  *pcb = cb;
  if (*pcb > size) {
    *pcb = size;
  }
  if (*pcb > 0) {
    memcpy(pbBuff, *str, *pcb);
    *str += *pcb;
  }
  return 0;
}


static void test_EM_STREAMIN(void)
{
  HWND hwndRichEdit = new_richedit(NULL);
  LRESULT result;
  EDITSTREAM es;
  char buffer[1024] = {0};

  const char * streamText0 = "{\\rtf1 TestSomeText}";
  const char * streamText0a = "{\\rtf1 TestSomeText\\par}";
  const char * streamText0b = "{\\rtf1 TestSomeText\\par\\par}";

  const char * streamText1 =
  "{\\rtf1\\ansi\\ansicpg1252\\deff0\\deflang12298{\\fonttbl{\\f0\\fswiss\\fprq2\\fcharset0 System;}}\r\n" \
  "\\viewkind4\\uc1\\pard\\f0\\fs17 TestSomeText\\par\r\n" \
  "}\r\n";

  /* This should be accepted in richedit 1.0 emulation. See bug #8326 */
  const char * streamText2 =
    "{{\\colortbl;\\red0\\green255\\blue102;\\red255\\green255\\blue255;" \
    "\\red170\\green255\\blue255;\\red255\\green238\\blue0;\\red51\\green255" \
    "\\blue221;\\red238\\green238\\blue238;}\\tx0 \\tx424 \\tx848 \\tx1272 " \
    "\\tx1696 \\tx2120 \\tx2544 \\tx2968 \\tx3392 \\tx3816 \\tx4240 \\tx4664 " \
    "\\tx5088 \\tx5512 \\tx5936 \\tx6360 \\tx6784 \\tx7208 \\tx7632 \\tx8056 " \
    "\\tx8480 \\tx8904 \\tx9328 \\tx9752 \\tx10176 \\tx10600 \\tx11024 " \
    "\\tx11448 \\tx11872 \\tx12296 \\tx12720 \\tx13144 \\cf2 RichEdit1\\line }";

  const char * streamText3 = "RichEdit1";

  /* Minimal test without \par at the end */
  es.dwCookie = (DWORD_PTR)&streamText0;
  es.dwError = 0;
  es.pfnCallback = test_EM_STREAMIN_esCallback;
  SendMessage(hwndRichEdit, EM_STREAMIN,
              (WPARAM)(SF_RTF), (LPARAM)&es);

  result = SendMessage(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
  ok (result  == 12,
      "EM_STREAMIN: Test 0 returned %ld, expected 12\n", result);
  result = strcmp (buffer,"TestSomeText");
  ok (result  == 0,
      "EM_STREAMIN: Test 0 set wrong text: Result: %s\n",buffer);

  /* Native richedit 2.0 ignores last \par */
  es.dwCookie = (DWORD_PTR)&streamText0a;
  es.dwError = 0;
  es.pfnCallback = test_EM_STREAMIN_esCallback;
  SendMessage(hwndRichEdit, EM_STREAMIN,
              (WPARAM)(SF_RTF), (LPARAM)&es);

  result = SendMessage(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
  ok (result  == 12,
      "EM_STREAMIN: Test 0-a returned %ld, expected 12\n", result);
  result = strcmp (buffer,"TestSomeText");
  ok (result  == 0,
      "EM_STREAMIN: Test 0-a set wrong text: Result: %s\n",buffer);

  /* Native richedit 2.0 ignores last \par, next-to-last \par appears */
  es.dwCookie = (DWORD_PTR)&streamText0b;
  es.dwError = 0;
  es.pfnCallback = test_EM_STREAMIN_esCallback;
  SendMessage(hwndRichEdit, EM_STREAMIN,
              (WPARAM)(SF_RTF), (LPARAM)&es);

  result = SendMessage(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
  ok (result  == 14,
      "EM_STREAMIN: Test 0-b returned %ld, expected 14\n", result);
  result = strcmp (buffer,"TestSomeText\r\n");
  ok (result  == 0,
      "EM_STREAMIN: Test 0-b set wrong text: Result: %s\n",buffer);

  es.dwCookie = (DWORD_PTR)&streamText1;
  es.dwError = 0;
  es.pfnCallback = test_EM_STREAMIN_esCallback;
  SendMessage(hwndRichEdit, EM_STREAMIN,
              (WPARAM)(SF_RTF), (LPARAM)&es);

  result = SendMessage(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
  ok (result  == 12,
      "EM_STREAMIN: Test 1 returned %ld, expected 12\n", result);
  result = strcmp (buffer,"TestSomeText");
  ok (result  == 0,
      "EM_STREAMIN: Test 1 set wrong text: Result: %s\n",buffer);


  es.dwCookie = (DWORD_PTR)&streamText2;
  SendMessage(hwndRichEdit, EM_STREAMIN,
              (WPARAM)(SF_RTF), (LPARAM)&es);

  result = SendMessage(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
  todo_wine {
  ok (result  == 9,
      "EM_STREAMIN: Test 2 returned %ld, expected 9\n", result);
  }
  result = strcmp (buffer,"RichEdit1");
  todo_wine {
  ok (result  == 0,
      "EM_STREAMIN: Test 2 set wrong text: Result: %s\n",buffer);
  }

  es.dwCookie = (DWORD_PTR)&streamText3;
  SendMessage(hwndRichEdit, EM_STREAMIN,
              (WPARAM)(SF_RTF), (LPARAM)&es);

  result = SendMessage(hwndRichEdit, WM_GETTEXT, 1024, (LPARAM) buffer);
  ok (result  == 0,
      "EM_STREAMIN: Test 3 returned %ld, expected 0\n", result);
  ok (strlen(buffer)  == 0,
      "EM_STREAMIN: Test 3 set wrong text: Result: %s\n",buffer);

  DestroyWindow(hwndRichEdit);
}


START_TEST( editor )
{
  MSG msg;
  time_t end;

  /* Must explicitly LoadLibrary(). The test has no references to functions in
   * RICHED32.DLL, so the linker doesn't actually link to it. */
  hmoduleRichEdit = LoadLibrary("RICHED32.DLL");
  ok(hmoduleRichEdit != NULL, "error: %d\n", (int) GetLastError());

  test_WM_SETTEXT();
  test_WM_GETTEXTLENGTH();
  test_EM_STREAMIN();

  /* Set the environment variable WINETEST_RICHED32 to keep windows
   * responsive and open for 30 seconds. This is useful for debugging.
   *
   * The message pump uses PeekMessage() to empty the queue and then sleeps for
   * 50ms before retrying the queue. */
  end = time(NULL) + 30;
  if (getenv( "WINETEST_RICHED32" )) {
    while (time(NULL) < end) {
      if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      } else {
        Sleep(50);
      }
    }
  }

  OleFlushClipboard();
  ok(FreeLibrary(hmoduleRichEdit) != 0, "error: %d\n", (int) GetLastError());
}
