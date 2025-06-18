/*
 * Copyright 2000, 2003, 2004, 2005 Martin Fuchs
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

#include <windows.h>
#include <commdlg.h>
#include <locale.h>
#include <time.h>

#include <shlwapi.h>
#include <shellapi.h>   /* for ShellExecuteW() */
#include <shlobj.h>

#define BUFFER_LEN  1024

enum IMAGE {
    IMG_NONE=-1,    IMG_FILE=0,         IMG_DOCUMENT,   IMG_EXECUTABLE,
    IMG_FOLDER,     IMG_OPEN_FOLDER,    IMG_FOLDER_PLUS,IMG_OPEN_PLUS,  IMG_OPEN_MINUS,
    IMG_FOLDER_UP,  IMG_FOLDER_CUR
};

#define IMAGE_WIDTH         16
#define IMAGE_HEIGHT        13
#define SPLIT_WIDTH         5
#define TREE_LINE_DX        3

#define IDW_STATUSBAR       0x100
#define IDW_TOOLBAR         0x101
#define IDW_DRIVEBAR        0x102
#define IDW_FIRST_CHILD     0xC000  /*0x200*/

#define IDW_TREE_LEFT       3
#define IDW_TREE_RIGHT      6
#define IDW_HEADER_LEFT     2
#define IDW_HEADER_RIGHT    5

#define WM_DISPATCH_COMMAND 0xBF80

#define COLOR_COMPRESSED    RGB(0,0,255)
#define COLOR_SELECTION     RGB(0,0,128)
#define COLOR_SPLITBAR      LTGRAY_BRUSH

#define FRM_CALC_CLIENT     0xBF83
#define Frame_CalcFrameClient(hwnd, prt) (SendMessageW(hwnd, FRM_CALC_CLIENT, 0, (LPARAM)(PRECT)prt))

typedef struct
{
  int       start_x;
  int       start_y;
  int       width;
  int       height;
} windowOptions;

typedef struct
{
  HANDLE    hInstance;
  HACCEL    haccel;
  ATOM      hframeClass;

  HWND      hMainWnd;
  HMENU     hMenuFrame;
  HMENU     hWindowsMenu;
  HMENU     hLanguageMenu;
  HMENU     hMenuView;
  HMENU     hMenuOptions;
  HWND      hmdiclient;
  HWND      hstatusbar;
  HWND      htoolbar;
  HWND      hdrivebar;
  HFONT     hfont;

  WCHAR     num_sep;
  SIZE      spaceSize;
  HIMAGELIST himl;

  WCHAR     drives[BUFFER_LEN];
  BOOL      prescan_node;   /*TODO*/
  BOOL      saveSettings;

  IShellFolder* iDesktop;
  IMalloc*      iMalloc;
  UINT          cfStrFName;
} WINEFILE_GLOBALS;

extern WINEFILE_GLOBALS Globals;
