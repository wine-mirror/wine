/*
 *  Notepad (notepad.h)
 *
 *  Copyright 1997,98 Marcel Baur <mbaur@g26.ethz.ch>
 *  Copyright 2002 Sylvain Petreolle <spetreolle@yahoo.fr>
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

#include "notepad_res.h"

#define MAX_STRING_LEN      255

/* Values are indexes of the items in the Encoding combobox. */
typedef enum
{
    ENCODING_AUTO    = -1,
    ENCODING_ANSI    =  0,
    ENCODING_UTF16LE =  1,
    ENCODING_UTF16BE =  2,
    ENCODING_UTF8    =  3
} ENCODING;

#define MIN_ENCODING   0
#define MAX_ENCODING   3

typedef struct
{
  HANDLE   hInstance;
  HWND     hMainWnd;
  HWND     hFindReplaceDlg;
  HWND     hEdit;
  HFONT    hFont; /* Font used by the edit control */
  HWND     hStatusBar;
  BOOL     bStatusBar;
  WCHAR*   szStatusString;
  LOGFONTW lfFont;
  BOOL     bWrapLongLines;
  WCHAR    szFindText[MAX_PATH];
  WCHAR    szReplaceText[MAX_PATH];
  WCHAR    szFileName[MAX_PATH];
  WCHAR    szFileTitle[MAX_PATH];
  ENCODING encFile;
  WCHAR    szFilter[2 * MAX_STRING_LEN + 100];
  ENCODING encOfnCombo;  /* Encoding selected in IDC_OFN_ENCCOMBO */
  BOOL     bOfnIsOpenDialog;
  INT      iMarginTop;
  INT      iMarginBottom;
  INT      iMarginLeft;
  INT      iMarginRight;
  WCHAR    szHeader[MAX_PATH];
  WCHAR    szFooter[MAX_PATH];
  INT      trackedSel;
  INT      lastLn;
  INT      lastCol;

  FINDREPLACEW find;
  FINDREPLACEW lastFind;
  HGLOBAL hDevMode; /* printer mode */
  HGLOBAL hDevNames; /* printer names */
} NOTEPAD_GLOBALS;

extern NOTEPAD_GLOBALS Globals;

VOID SetFileNameAndEncoding(LPCWSTR szFileName, ENCODING enc);
void NOTEPAD_DoFind(FINDREPLACEW *fr);
void UpdateStatusBar(void);
void updateWindowSize(int width, int height);
LRESULT CALLBACK EDIT_CallBackProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
