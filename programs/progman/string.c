/*
 * Program Manager
 *
 * Copyright 1996 Ulrich Schmid
 * Copyright 2002 Sylvain Petreolle
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

#define WIN32_LEAN_AND_MEAN

#include "windows.h"
#include "progman.h"

/* Class names */

CHAR STRING_MAIN_WIN_CLASS_NAME[]    = "PMMain";
CHAR STRING_MDI_WIN_CLASS_NAME[]     = "MDICLIENT";
CHAR STRING_GROUP_WIN_CLASS_NAME[]   = "PMGroup";
CHAR STRING_PROGRAM_WIN_CLASS_NAME[] = "PMProgram";

/* Resource names */
CHAR STRING_ACCEL[]      = "ACCEL";
CHAR STRING_MENU[]    = "MENU";
CHAR STRING_NEW[]     = "DIALOG_NEW";
CHAR STRING_OPEN[]    = "DIALOG_OPEN";
CHAR STRING_MOVE[]    = "DIALOG_MOVE";
CHAR STRING_COPY[]    = "DIALOG_COPY";
CHAR STRING_DELETE[]  = "DIALOG_DELETE";
CHAR STRING_GROUP[]   = "DIALOG_GROUP";
CHAR STRING_PROGRAM[] = "DIALOG_PROGRAM";
CHAR STRING_SYMBOL[]  = "DIALOG_SYMBOL";
CHAR STRING_EXECUTE[] = "DIALOG_EXECUTE";


VOID STRING_LoadMenus(VOID)
{
  CHAR   caption[MAX_STRING_LEN];
  HMENU  hMainMenu;

  /* Set frame caption */
  LoadString(Globals.hInstance, IDS_PROGRAM_MANAGER, caption, sizeof(caption));
  SetWindowText(Globals.hMainWnd, caption);

  /* Create menu */
  hMainMenu = LoadMenu(Globals.hInstance, MAKEINTRESOURCE(MAIN_MENU));
  Globals.hFileMenu     = GetSubMenu(hMainMenu, 0);
  Globals.hOptionMenu   = GetSubMenu(hMainMenu, 1);
  Globals.hWindowsMenu  = GetSubMenu(hMainMenu, 2);
  Globals.hLanguageMenu = GetSubMenu(hMainMenu, 3);

  if (Globals.hMDIWnd)
    SendMessage(Globals.hMDIWnd, WM_MDISETMENU,
		(WPARAM) hMainMenu,
		(LPARAM) Globals.hWindowsMenu);
  else SetMenu(Globals.hMainWnd, hMainMenu);

  /* Destroy old menu */
  if (Globals.hMainMenu) DestroyMenu(Globals.hMainMenu);
  Globals.hMainMenu = hMainMenu;
}

/* Local Variables:    */
/* c-file-style: "GNU" */
/* End:                */
