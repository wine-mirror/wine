/*
 * Clock (main.h)
 *
 * Copyright 1998 Marcel Baur <mbaur@g26.ethz.ch>
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

#include "clock_res.h"

#define MAX_STRING_LEN      255
#define DEFAULTICON OIC_WINLOGO

/* hide the following from winerc */
#ifndef RC_INVOKED

typedef struct
{
  HANDLE  hInstance;
  HWND    hMainWnd;
  HICON   hMainIcon;
  HICON   hDefaultIcon;
  HMENU   hMainMenu;
  HMENU   hPropertiesMenu;
  HMENU   hLanguageMenu;
  HMENU   hInfoMenu;
  HMENU   hSystemMenu;
  HMENU   hPopupMenu1;
  LPCSTR  lpszIniFile;
  LPCSTR  lpszIcoFile;

  BOOL    bAnalog;
  BOOL    bAlwaysOnTop;
  BOOL    bWithoutTitle;
  BOOL    bSeconds;
  BOOL    bDate;

  int     MaxX;
  int     MaxY;
} CLOCK_GLOBALS;

extern CLOCK_GLOBALS Globals;

/* function prototypes */
VOID MAIN_FileChooseFont(VOID);

/* class names */

/* Resource names */
extern CHAR STRING_MENU_Xx[];

   #define STRINGID(id) (0x##id + Globals.wStringTableOffset)

#else  /* RC_INVOKED */

   #define STRINGID(id) id

#endif

/* string table index */
#define IDS_LANGUAGE_ID                STRINGID(00)
#define IDS_LANGUAGE_MENU_ITEM         STRINGID(01)
#define IDS_CLOCK                      STRINGID(02)
#define IDS_MENU_ON_TOP                STRINGID(03)

/* main menu */

#define CL_ON_TOP                99

#define CL_ANALOG                100
#define CL_DIGITAL               101
#define CL_FONT                  102
#define CL_WITHOUT_TITLE         103
#define CL_SECONDS               104
#define CL_DATE                  105

#define CL_LANGUAGE              200

#define CL_INFO                  301
#define CL_INFO_LICENSE          302
#define CL_INFO_NO_WARRANTY      303
#define CL_INFO_ABOUT_WINE       304

#define CL_FIRST_LANGUAGE        200
#define CL_LAST_LANGUAGE         220
