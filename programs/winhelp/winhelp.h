/*
 * Help Viewer
 *
 * Copyright 1996 Ulrich Schmid
 * Copyright 2002 Sylvain Petreolle <spetreolle@yahoo.fr>

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

#define MAX_LANGUAGE_NUMBER 255
#define MAX_PATHNAME_LEN   1024
#define MAX_STRING_LEN      255

#define INTERNAL_BORDER_WIDTH  5
#define POPUP_YDISTANCE       20
#define SHADOW_DX     20
#define SHADOW_DY     20
#define BUTTON_CX      6
#define BUTTON_CY      6

#ifndef RC_INVOKED

#include "hlpfile.h"
#include "macro.h"
#include "winhelp_res.h"

typedef struct tagHelpLinePart
{
  RECT      rect;
  LPCSTR    lpsText;
  UINT      wTextLen;
  HFONT     hFont;
  COLORREF  color;

  struct
  {
  LPCSTR    lpszPath;
  LONG      lHash;
  BOOL      bPopup;
  }         link;

  HGLOBAL   hSelf;
  struct tagHelpLinePart *next;
} WINHELP_LINE_PART;

typedef struct tagHelpLine
{
  RECT              rect;
  WINHELP_LINE_PART first_part;
  struct tagHelpLine *next;
} WINHELP_LINE;

typedef struct tagHelpButton
{
  HWND hWnd;

  LPCSTR lpszID;
  LPCSTR lpszName;
  LPCSTR lpszMacro;

  WPARAM wParam;

  RECT rect;

  HGLOBAL hSelf;
  struct tagHelpButton *next;
} WINHELP_BUTTON;

typedef struct tagWinHelp
{
  LPCSTR lpszName;

  WINHELP_BUTTON *first_button;
  HLPFILE_PAGE   *page;
  WINHELP_LINE   *first_line;

  HWND hMainWnd;
  HWND hButtonBoxWnd;
  HWND hTextWnd;
  HWND hShadowWnd;

  HFONT (*fonts)[2];
  UINT  fonts_len;

  HCURSOR hArrowCur;
  HCURSOR hHandCur;

  HGLOBAL hSelf;
  struct tagWinHelp *next;
} WINHELP_WINDOW;

typedef struct
{
  UINT   wVersion;
  HANDLE hInstance;
  HWND   hPopupWnd;
  UINT   wStringTableOffset;
  WINHELP_WINDOW *active_win;
  WINHELP_WINDOW *win_list;
} WINHELP_GLOBALS;

extern WINHELP_GLOBALS Globals;

VOID WINHELP_CreateHelpWindow(LPCSTR, LONG, LPCSTR, BOOL, HWND, LPPOINT, INT);
INT  WINHELP_MessageBoxIDS(UINT, UINT, WORD);
INT  WINHELP_MessageBoxIDS_s(UINT, LPCSTR, UINT, WORD);

extern CHAR MAIN_WIN_CLASS_NAME[];
extern CHAR BUTTON_BOX_WIN_CLASS_NAME[];
extern CHAR TEXT_WIN_CLASS_NAME[];
extern CHAR SHADOW_WIN_CLASS_NAME[];
extern CHAR STRING_BUTTON[];
extern CHAR STRING_MENU_Xx[];
extern CHAR STRING_DIALOG_TEST[];
#endif

/* Buttons */
#define WH_FIRST_BUTTON     500

/* Local Variables:    */
/* c-file-style: "GNU" */
/* End:                */
