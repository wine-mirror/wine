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
