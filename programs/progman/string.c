/*
 * Program Manager
 *
 * Copyright 1996 Ulrich Schmid
 */

#include <windows.h>
#ifdef WINELIB
#include <options.h>
#endif
#include "progman.h"

/* Class names */

CHAR STRING_MAIN_WIN_CLASS_NAME[]    = "PMMain";
CHAR STRING_MDI_WIN_CLASS_NAME[]     = "MDICLIENT";
CHAR STRING_GROUP_WIN_CLASS_NAME[]   = "PMGroup";
CHAR STRING_PROGRAM_WIN_CLASS_NAME[] = "PMProgram";

/* Resource names */
/* Xx will be overwritten with En, ... */
CHAR STRING_ACCEL[]      = "ACCEL";
CHAR STRING_MAIN_Xx[]    = "MENU_Xx";
CHAR STRING_NEW_Xx[]     = "DIALOG_NEW_Xx";
CHAR STRING_OPEN_Xx[]    = "DIALOG_OPEN_Xx";
CHAR STRING_MOVE_Xx[]    = "DIALOG_MOVE_Xx";
CHAR STRING_COPY_Xx[]    = "DIALOG_COPY_Xx";
CHAR STRING_DELETE_Xx[]  = "DIALOG_DELETE_Xx";
CHAR STRING_GROUP_Xx[]   = "DIALOG_GROUP_Xx";
CHAR STRING_PROGRAM_Xx[] = "DIALOG_PROGRAM_Xx";
CHAR STRING_SYMBOL_Xx[]  = "DIALOG_SYMBOL_Xx";
CHAR STRING_EXECUTE_Xx[] = "DIALOG_EXECUTE_Xx";

static LPCSTR StringTableEn[];
static LPCSTR StringTableDe[];

VOID STRING_SelectLanguage(LPCSTR lang)
{
  HMENU  hMainMenu;
  HLOCAL hGroup;

  /* Change string table */
  Globals.StringTable = StringTableEn;
  if (!lstrcmp(lang, "De")) Globals.StringTable = StringTableDe;

  SetWindowText(Globals.hMainWnd, STRING_PROGRAM_MANAGER);

  /* Change Resource names */
  lstrcpyn(STRING_MAIN_Xx    + sizeof(STRING_MAIN_Xx)    - 3, lang, 3);
  lstrcpyn(STRING_NEW_Xx     + sizeof(STRING_NEW_Xx)     - 3, lang, 3);
  lstrcpyn(STRING_OPEN_Xx    + sizeof(STRING_OPEN_Xx)    - 3, lang, 3);
  lstrcpyn(STRING_MOVE_Xx    + sizeof(STRING_MOVE_Xx)    - 3, lang, 3);
  lstrcpyn(STRING_COPY_Xx    + sizeof(STRING_COPY_Xx)    - 3, lang, 3);
  lstrcpyn(STRING_DELETE_Xx  + sizeof(STRING_DELETE_Xx)  - 3, lang, 3);
  lstrcpyn(STRING_GROUP_Xx   + sizeof(STRING_GROUP_Xx)   - 3, lang, 3);
  lstrcpyn(STRING_PROGRAM_Xx + sizeof(STRING_PROGRAM_Xx) - 3, lang, 3);
  lstrcpyn(STRING_SYMBOL_Xx  + sizeof(STRING_SYMBOL_Xx)  - 3, lang, 3);
  lstrcpyn(STRING_EXECUTE_Xx + sizeof(STRING_EXECUTE_Xx) - 3, lang, 3);

  /* Create menu */
  hMainMenu = LoadMenu(Globals.hInstance, STRING_MAIN_Xx);
  if (hMainMenu)
  {
    Globals.hFileMenu    = GetSubMenu(hMainMenu, 0);
    Globals.hOptionMenu  = GetSubMenu(hMainMenu, 1);
    Globals.hWindowsMenu = GetSubMenu(hMainMenu, 2);

    if (Globals.hMDIWnd)
      SendMessage(Globals.hMDIWnd, WM_MDISETMENU,
		  (WPARAM) hMainMenu,
		  (LPARAM) Globals.hWindowsMenu);
    else SetMenu(Globals.hMainWnd, hMainMenu);

    /* Destroy old menu */
    if (Globals.hMainMenu) DestroyMenu(Globals.hMainMenu);
    Globals.hMainMenu = hMainMenu;
  }
  /* Unsupported language */
  else if(lstrcmp(lang, "En")) STRING_SelectLanguage("En");
  else
  {
    MessageBox(Globals.hMainWnd, "No language found", "FATAL ERROR", MB_OK);
    PostQuitMessage(1);
  }

  /* have to be last because of
   * the possible recursion */
  Globals.lpszLanguage = lang;
#ifdef WINELIB
  if (!lstrcmp(lang, "De")) Options.language = LANG_De;
  if (!lstrcmp(lang, "En")) Options.language = LANG_En;
  GetSystemMenu(Globals.hMainWnd, TRUE);
  for (hGroup = GROUP_FirstGroup(); hGroup;
       hGroup = GROUP_NextGroup(hGroup))
    {
      GROUP *group = LocalLock(hGroup);
      GetSystemMenu(group->hWnd, TRUE);
    }
#endif
}

/* Local Variables:    */
/* c-file-style: "GNU" */
/* End:                */
