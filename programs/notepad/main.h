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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "notepad_res.h"

#define MAX_STRING_LEN      255
#define MAX_PATHNAME_LEN    1024
#define MAX_LANGUAGE_NUMBER (NP_LAST_LANGUAGE - NP_FIRST_LANGUAGE)

#define HELPFILE  "notepad.hlp"
#define LOGPREFIX ".LOG"

/* hide the following from winerc */
#ifndef RC_INVOKED

#define WINE_RELEASE_INFO "Wine (www.winehq.com)"

#include "commdlg.h"

/***** Compatibility *****/

#ifndef OIC_WINLOGO
#define OIC_WINLOGO 32517
#endif
#define DEFAULTICON OIC_WINLOGO

typedef struct
{
  HANDLE  hInstance;
  HWND    hMainWnd;
  HWND    hFindReplaceDlg;
  HICON   hMainIcon;
  HICON   hDefaultIcon;
  HMENU   hMainMenu;
  HMENU   hFileMenu;
  HMENU   hEditMenu;
  HMENU   hSearchMenu;
  HMENU   hLanguageMenu;
  HMENU   hHelpMenu;
  LPCSTR  lpszIniFile;
  LPCSTR  lpszIcoFile;
  UINT    wStringTableOffset;
  BOOL    bWrapLongLines;
  CHAR    szFindText[MAX_PATHNAME_LEN];
  CHAR    szReplaceText[MAX_PATHNAME_LEN];
  CHAR    szFileName[MAX_PATHNAME_LEN];
  CHAR    szMarginTop[MAX_PATHNAME_LEN];
  CHAR    szMarginBottom[MAX_PATHNAME_LEN];
  CHAR    szMarginLeft[MAX_PATHNAME_LEN];
  CHAR    szMarginRight[MAX_PATHNAME_LEN];
  CHAR    szHeader[MAX_PATHNAME_LEN];
  CHAR    szFooter[MAX_PATHNAME_LEN];

  FINDREPLACE find;
  WORD    nCommdlgFindReplaceMsg;
  CHAR    Buffer[12000];
} NOTEPAD_GLOBALS;

extern NOTEPAD_GLOBALS Globals;

/* function prototypes */

void TrashBuffer(void);
void LoadBufferFromFile(LPCSTR lpFileName);

/* class names */

/* Resource names */
extern CHAR STRING_MENU_Xx[];
extern CHAR STRING_PAGESETUP_Xx[];
   
#else  /* RC_INVOKED */
   
#endif
