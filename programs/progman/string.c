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
CHAR STRING_MENU_Xx[]    = "MENU_Xx";
CHAR STRING_NEW_Xx[]     = "DIALOG_NEW_Xx";
CHAR STRING_OPEN_Xx[]    = "DIALOG_OPEN_Xx";
CHAR STRING_MOVE_Xx[]    = "DIALOG_MOVE_Xx";
CHAR STRING_COPY_Xx[]    = "DIALOG_COPY_Xx";
CHAR STRING_DELETE_Xx[]  = "DIALOG_DELETE_Xx";
CHAR STRING_GROUP_Xx[]   = "DIALOG_GROUP_Xx";
CHAR STRING_PROGRAM_Xx[] = "DIALOG_PROGRAM_Xx";
CHAR STRING_SYMBOL_Xx[]  = "DIALOG_SYMBOL_Xx";
CHAR STRING_EXECUTE_Xx[] = "DIALOG_EXECUTE_Xx";

static BOOL STRING_LoadStringOtherLanguage(UINT num, UINT ids, LPSTR str, UINT len)
{
  ids -= Globals.wStringTableOffset;
  ids += num * 0x100;
  return(LoadString(Globals.hInstance, ids, str, len));
};

VOID STRING_SelectLanguageByName(LPCSTR lang)
{
  INT i;
  CHAR newlang[3];

  for (i = 0; i <= MAX_LANGUAGE_NUMBER; i++)
    if (STRING_LoadStringOtherLanguage(i, IDS_LANGUAGE_ID, newlang, sizeof(newlang)) &&
	!lstrcmp(lang, newlang))
      {
	STRING_SelectLanguageByNumber(i);
	return;
      }

  /* Fallback */
  for (i = 0; i <= MAX_LANGUAGE_NUMBER; i++)
    if (STRING_LoadStringOtherLanguage(i, IDS_LANGUAGE_ID, newlang, sizeof(newlang)))
      {
	STRING_SelectLanguageByNumber(i);
	return;
      }

  MessageBox(Globals.hMainWnd, "No language found", "FATAL ERROR", MB_OK);
  PostQuitMessage(1);
}

VOID STRING_SelectLanguageByNumber(UINT num)
{
  INT    i;
  CHAR   lang[3];
  CHAR   caption[MAX_STRING_LEN];
  CHAR   item[MAX_STRING_LEN];
  HMENU  hMainMenu;
  HLOCAL hGroup;

  /* Select string table */
  Globals.wStringTableOffset = num * 0x100;

  /* Get Language id */
  LoadString(Globals.hInstance, IDS_LANGUAGE_ID, lang, sizeof(lang));
  Globals.lpszLanguage = lang;

  /* Set frame caption */
  LoadString(Globals.hInstance, IDS_PROGRAM_MANAGER, caption, sizeof(caption));
  SetWindowText(Globals.hMainWnd, caption);

  /* Change Resource names */
  lstrcpyn(STRING_MENU_Xx    + sizeof(STRING_MENU_Xx)    - 3, lang, 3);
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
  hMainMenu = LoadMenu(Globals.hInstance, STRING_MENU_Xx);
  Globals.hFileMenu     = GetSubMenu(hMainMenu, 0);
  Globals.hOptionMenu   = GetSubMenu(hMainMenu, 1);
  Globals.hWindowsMenu  = GetSubMenu(hMainMenu, 2);
  Globals.hLanguageMenu = GetSubMenu(hMainMenu, 3);

  /* Remove dummy item */
  RemoveMenu(Globals.hLanguageMenu, 0, MF_BYPOSITION);
  /* Add language items */
  for (i = 0; i <= MAX_LANGUAGE_NUMBER; i++)
    if (STRING_LoadStringOtherLanguage(i, IDS_LANGUAGE_MENU_ITEM, item, sizeof(item)))
      AppendMenu(Globals.hLanguageMenu, MF_STRING | MF_BYCOMMAND,
		 PM_FIRST_LANGUAGE + i, item);

  if (Globals.hMDIWnd)
    SendMessage(Globals.hMDIWnd, WM_MDISETMENU,
		(WPARAM) hMainMenu,
		(LPARAM) Globals.hWindowsMenu);
  else SetMenu(Globals.hMainWnd, hMainMenu);

  /* Destroy old menu */
  if (Globals.hMainMenu) DestroyMenu(Globals.hMainMenu);
  Globals.hMainMenu = hMainMenu;

#ifndef WINELIB
  /* Update system menus (FIXME) */
  for (i = 0; langNames[i] && lstrcmp(lang, langNames[i]);) i++;
  if (langNames[i]) Options.language = i;

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
