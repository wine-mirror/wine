/*
 * Clock
 *
 * Copyright 1998 Marcel Baur <mbaur@g26.ethz.ch>
 * Copyright 1998 Karl Backström <karl_b@geocities.com>
 */

#include <stdio.h>
#include "windows.h"
#include "main.h"
#include "language.h"
#ifdef WINELIB
#include "options.h"
#endif

CHAR STRING_MENU_Xx[]      = "MENU_Xx";

static BOOL LANGUAGE_LoadStringOther(UINT num, UINT ids, LPSTR str, UINT len)
{
  ids -= Globals.wStringTableOffset;
  ids += num * 0x100;
  return(LoadString(Globals.hInstance, ids, str, len));
};

VOID LANGUAGE_SelectByName(LPCSTR lang)
{
  INT i;
  CHAR newlang[3];

  for (i = 0; i <= MAX_LANGUAGE_NUMBER; i++)
    if (LANGUAGE_LoadStringOther(i, IDS_LANGUAGE_ID, newlang, sizeof(newlang)) &&
	!lstrcmp(lang, newlang))
      {
        LANGUAGE_SelectByNumber(i);
	return;
      }

  /* Fallback */
    for (i = 0; i <= MAX_LANGUAGE_NUMBER; i++)
    if (LANGUAGE_LoadStringOther(i, IDS_LANGUAGE_ID, newlang, sizeof(newlang)))
      {
	LANGUAGE_SelectByNumber(i);
	return;
      }

  MessageBox(Globals.hMainWnd, "No language found", "FATAL ERROR", MB_OK);
  PostQuitMessage(1);
}

VOID LANGUAGE_SelectByNumber(UINT num)
{
  INT    i;
  CHAR   lang[3];
  CHAR   caption[MAX_STRING_LEN];
  CHAR   item[MAX_STRING_LEN];
  HMENU  hMainMenu;

  /* Select string table */
  Globals.wStringTableOffset = num * 0x100;

  /* Get Language id */
  LoadString(Globals.hInstance, IDS_LANGUAGE_ID, lang, sizeof(lang));
  Globals.lpszLanguage = lang;

  /* Set frame caption */
  LoadString(Globals.hInstance, IDS_CLOCK, caption, sizeof(caption));
  SetWindowText(Globals.hMainWnd, caption);

  /* Change Resource names */
  lstrcpyn(STRING_MENU_Xx    + sizeof(STRING_MENU_Xx)    - 3, lang, 3);

  /* Create menu */
  hMainMenu = LoadMenu(Globals.hInstance, STRING_MENU_Xx);
    Globals.hPropertiesMenu     = GetSubMenu(hMainMenu, 0);
    Globals.hLanguageMenu 	= GetSubMenu(hMainMenu, 1);
    Globals.hInfoMenu     	= GetSubMenu(hMainMenu, 2);

  /* Remove dummy item */
  RemoveMenu(Globals.hLanguageMenu, 0, MF_BYPOSITION);
  /* Add language items */
  for (i = 0; i <= MAX_LANGUAGE_NUMBER; i++)
    if (LANGUAGE_LoadStringOther(i, IDS_LANGUAGE_MENU_ITEM, item, sizeof(item)))
      AppendMenu(Globals.hLanguageMenu, MF_STRING | MF_BYCOMMAND,
		 CL_FIRST_LANGUAGE + i, item);

  SetMenu(Globals.hMainWnd, hMainMenu);

  /* Destroy old menu */
  if (Globals.hMainMenu) DestroyMenu(Globals.hMainMenu);
  Globals.hMainMenu = hMainMenu;

#ifdef WINELIB
  /* Update system menus */
  for (i = 0; Languages[i].name && lstrcmp(lang, Languages[i].name);) i++;
  if (Languages[i].name) Options.language = i;

#endif

/* Specific for clock */
if(Globals.bAnalog == TRUE) {
	CheckMenuItem(Globals.hPropertiesMenu, CL_ANALOG, \
                       MF_BYCOMMAND | MF_CHECKED);
        CheckMenuItem(Globals.hPropertiesMenu, CL_DIGITAL, \
                       MF_BYCOMMAND | MF_UNCHECKED);
	EnableMenuItem(Globals.hPropertiesMenu, CL_FONT, \
                       MF_BYCOMMAND | MF_GRAYED);
}
else {
	CheckMenuItem(Globals.hPropertiesMenu, CL_ANALOG, \
                       MF_BYCOMMAND | MF_UNCHECKED);
        CheckMenuItem(Globals.hPropertiesMenu, CL_DIGITAL, \
                       MF_BYCOMMAND | MF_CHECKED);
}
	CheckMenuItem(Globals.hPropertiesMenu, CL_WITHOUT_TITLE, MF_BYCOMMAND | \
	                     (Globals.bWithoutTitle ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(Globals.hPropertiesMenu, CL_ON_TOP, MF_BYCOMMAND | \
	                     (Globals.bAlwaysOnTop ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(Globals.hPropertiesMenu, CL_SECONDS, MF_BYCOMMAND | \
	                     (Globals.bSeconds ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(Globals.hPropertiesMenu, CL_DATE, MF_BYCOMMAND | \
	                     (Globals.bDate ? MF_CHECKED : MF_UNCHECKED));
}

VOID LANGUAGE_DefaultHandle(WPARAM wParam)
{
  if ((wParam >=CL_FIRST_LANGUAGE) && (wParam<=CL_LAST_LANGUAGE))
          LANGUAGE_SelectByNumber(wParam - CL_FIRST_LANGUAGE);
     else printf("Unimplemented menu command %i\n", wParam);
}

VOID LANGUAGE_Init(VOID)
{
  #ifdef WINELIB
   Globals.lpszLanguage = Languages[Options.language].name;
  #endif
//  #else
  printf("Globals.lpszLanguage == %s\n", Globals.lpszLanguage);
  if (Globals.lpszLanguage == "En") {
    CHAR buffer[MAX_PATHNAME_LEN], *p;
    printf("Muu\n");
    PROFILE_GetWineIniString("programs", "language", "En", 
                             buffer, sizeof(buffer));
  Globals.lpszLanguage = p = LocalLock(LocalAlloc(LMEM_FIXED, lstrlen(buffer)));
  hmemcpy(p, buffer, 1 + lstrlen(buffer)); }
//  #endif
//  if (!Globals.lpszLanguage) Globals.lpszLanguage = "En";
}

/* Local Variables:    */
/* c-file-style: "GNU" */
/* End:                */
