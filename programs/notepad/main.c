/*
 * Notepad
 *
 * Copyright 1997 Marcel Baur <mbaur@g26.ethz.ch>
 */

#include <stdio.h>
#include "windows.h"
#include "main.h"
#include "license.h"
#include "dialog.h"
#ifdef WINELIB
#include "options.h"
#include "resource.h"
#include "shell.h"
void LIBWINE_Register_De();
void LIBWINE_Register_En();
void LIBWINE_Register_Sw();
#endif

NOTEPAD_GLOBALS Globals;

CHAR STRING_MENU_Xx[]      = "MENU_Xx";
CHAR STRING_PAGESETUP_Xx[] = "DIALOG_PAGESETUP_Xx";

static BOOL MAIN_LoadStringOtherLanguage(UINT num, UINT ids, LPSTR str, UINT len)
{
  ids -= Globals.wStringTableOffset;
  ids += num * 0x100;
  return(LoadString(Globals.hInstance, ids, str, len));
};

VOID MAIN_SelectLanguageByName(LPCSTR lang)
{
  INT i;
  CHAR newlang[3];

  for (i = 0; i <= MAX_LANGUAGE_NUMBER; i++)
    if (MAIN_LoadStringOtherLanguage(i, IDS_LANGUAGE_ID, newlang, sizeof(newlang)) &&
	!lstrcmp(lang, newlang))
      {
        MAIN_SelectLanguageByNumber(i);
	return;
      }

  /* Fallback */
    for (i = 0; i <= MAX_LANGUAGE_NUMBER; i++)
    if (MAIN_LoadStringOtherLanguage(i, IDS_LANGUAGE_ID, newlang, sizeof(newlang)))
      {
	MAIN_SelectLanguageByNumber(i);
	return;
      }

  MessageBox(Globals.hMainWnd, "No language found", "FATAL ERROR", MB_OK);
  PostQuitMessage(1);
}

VOID MAIN_SelectLanguageByNumber(UINT num)
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
  LoadString(Globals.hInstance, IDS_NOTEPAD, caption, sizeof(caption));
  SetWindowText(Globals.hMainWnd, caption);

  /* Change Resource names */
  lstrcpyn(STRING_MENU_Xx    + sizeof(STRING_MENU_Xx)    - 3, lang, 3);
  lstrcpyn(STRING_PAGESETUP_Xx    + sizeof(STRING_PAGESETUP_Xx)    - 3, lang, 3);

  /* Create menu */
  hMainMenu = LoadMenu(Globals.hInstance, STRING_MENU_Xx);
    Globals.hFileMenu     = GetSubMenu(hMainMenu, 0);
    Globals.hEditMenu     = GetSubMenu(hMainMenu, 1);
    Globals.hSearchMenu   = GetSubMenu(hMainMenu, 2);
    Globals.hLanguageMenu = GetSubMenu(hMainMenu, 3);
    Globals.hHelpMenu     = GetSubMenu(hMainMenu, 4);

  /* Remove dummy item */
  RemoveMenu(Globals.hLanguageMenu, 0, MF_BYPOSITION);
  /* Add language items */
  for (i = 0; i <= MAX_LANGUAGE_NUMBER; i++)
    if (MAIN_LoadStringOtherLanguage(i, IDS_LANGUAGE_MENU_ITEM, item, sizeof(item)))
      AppendMenu(Globals.hLanguageMenu, MF_STRING | MF_BYCOMMAND,
		 NP_FIRST_LANGUAGE + i, item);

  SetMenu(Globals.hMainWnd, hMainMenu);

  /* Destroy old menu */
  if (Globals.hMainMenu) DestroyMenu(Globals.hMainMenu);
  Globals.hMainMenu = hMainMenu;

#ifdef WINELIB
  /* Update system menus */
  for (i = 0; Languages[i].name && lstrcmp(lang, Languages[i].name);) i++;
  if (Languages[i].name) Options.language = i;

#endif
}

/***********************************************************************
 *
 *           NOTEPAD_RegisterLanguages
 *
 *  Handle language stuff at startup
 */

void NOTEPAD_RegisterLanguages(void) {

  LPCSTR opt_lang = "En";
  CHAR lang[3];
  INT langnum;
  
 /* Find language specific string table */
  for (langnum = 0; langnum <= MAX_LANGUAGE_NUMBER; langnum++)
    {
      Globals.wStringTableOffset = langnum * 0x100;
      if (LoadString(Globals.hInstance, IDS_LANGUAGE_ID, lang, 
            sizeof(lang)) && !lstrcmp(opt_lang, lang))
      break;
    }
  if (langnum > MAX_LANGUAGE_NUMBER)
    {
      /* Find fallback language */
      for (langnum = 0; langnum <= MAX_LANGUAGE_NUMBER; langnum++)
	{
	  Globals.wStringTableOffset = langnum * 0x100;
	  if (LoadString(Globals.hInstance, IDS_LANGUAGE_ID, lang, sizeof(lang)))
          break;
	}
      if (langnum > MAX_LANGUAGE_NUMBER)
	{
	MessageBox(0, "No language found", "FATAL ERROR", MB_OK);
	PostQuitMessage(0);
	}
    }

  /* Change Resource names */
  lstrcpyn(STRING_MENU_Xx + lstrlen(STRING_MENU_Xx) - 2, lang, 3);
}


/***********************************************************************
 *
 *           NOTEPAD_MenuCommand
 *
 *  All handling of main menu events
 */

int NOTEPAD_MenuCommand (WPARAM wParam)
{  
//   printf("NOTEPAD_MenuCommand()\n");

   switch (wParam) {
     case NP_FILE_NEW:          DIALOG_FileNew(); break;
     case NP_FILE_OPEN:         DIALOG_FileOpen(); break;
     case NP_FILE_SAVE:         DIALOG_FileSave(); break;
     case NP_FILE_SAVEAS:       DIALOG_FileSaveAs(); break;
     case NP_FILE_PRINT:        DIALOG_FilePrint(); break;
     case NP_FILE_PAGESETUP:    DIALOG_FilePageSetup(); break;
     case NP_FILE_PRINTSETUP:   DIALOG_FilePrinterSetup();break;
     case NP_FILE_EXIT:         DIALOG_FileExit(); break;

     case NP_EDIT_UNDO:         DIALOG_EditUndo(); break;
     case NP_EDIT_CUT:          DIALOG_EditCut(); break;
     case NP_EDIT_COPY:         DIALOG_EditCopy(); break;
     case NP_EDIT_PASTE:        DIALOG_EditPaste(); break;
     case NP_EDIT_DELETE:       DIALOG_EditDelete(); break;
     case NP_EDIT_SELECTALL:    DIALOG_EditSelectAll(); break;
     case NP_EDIT_TIMEDATE:     DIALOG_EditTimeDate();break;
     case NP_EDIT_WRAP:         DIALOG_EditWrap(); break;

     case NP_SEARCH_SEARCH:     DIALOG_Search(); break;
     case NP_SEARCH_NEXT:       DIALOG_SearchNext(); break;

     case NP_HELP_CONTENTS:     DIALOG_HelpContents(); break;
     case NP_HELP_SEARCH:       DIALOG_HelpSearch(); break;
     case NP_HELP_ON_HELP:      DIALOG_HelpHelp(); break;
     case NP_HELP_LICENSE:      DIALOG_HelpLicense(); break;
     case NP_HELP_NO_WARRANTY:  DIALOG_HelpNoWarranty(); break;
     case NP_HELP_ABOUT_WINE:   DIALOG_HelpAboutWine(); break;
     
     // Handle languages
     default:
       if ((wParam >=NP_FIRST_LANGUAGE) && (wParam<=NP_LAST_LANGUAGE))
          MAIN_SelectLanguageByNumber(wParam - NP_FIRST_LANGUAGE);
     else printf("Unimplemented menu command %i\n", wParam);  
   }
   return 0;
}



/***********************************************************************
 *
 *           NOTEPAD_WndProc
 */

LRESULT NOTEPAD_WndProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;

    switch (msg) {

       case WM_CREATE:
        printf("WM_CREATE\n");
   	break;

        case WM_PAINT:
	  printf("WM_PAINT\n");
          BeginPaint(hWnd, &ps);
          EndPaint(hWnd, &ps);
   	break;

       case WM_COMMAND:
          printf("WM_COMMAND\n");
          NOTEPAD_MenuCommand(wParam);
          break;

       case WM_DESTROY:
          printf("WM_DESTROY\n");
          PostQuitMessage (0);
          break;

       default:
          return DefWindowProc (hWnd, msg, wParam, lParam);
    }
    return 0l;
}



/***********************************************************************
 *
 *           WinMain
 */

void DumpGlobals(void) {

    printf("DumpGlobals()\n");
    printf(" Globals.lpszIniFile: %s\n", Globals.lpszIniFile); 
    printf(" Globals.lpszIcoFile: %s\n", Globals.lpszIcoFile);
    printf("Globals.lpszLanguage: %s\n", Globals.lpszLanguage);
    printf("   Globals.hInstance: %i\n", Globals.hInstance);
    printf("   Globals.hMainMenu: %i\n", Globals.hMainMenu);
    
}
 
int PASCAL WinMain (HANDLE hInstance, HANDLE prev, LPSTR cmdline, int show)
{
    MSG      msg;
    WNDCLASS class;
    char className[] = "NPClass";  /* To make sure className >= 0x10000 */
    char winName[]   = "Notepad";

    #if defined(WINELIB) && !defined(HAVE_WINE_CONSTRUCTOR)
      /* Register resources */
      LIBWINE_Register_De();
      LIBWINE_Register_En();
      LIBWINE_Register_Sw();
    #endif

    printf("WinMain()\n");
    
    /* Setup Globals */

    Globals.lpszIniFile   = "notepad.ini";
    Globals.lpszIcoFile   = "notepad.ico";

  /* Select Language */
#ifdef WINELIB
  Globals.lpszLanguage = Languages[Options.language].name;
#else
  Globals.lpszLanguage = "En";
#endif

    Globals.hInstance     = hInstance;
    Globals.hMainIcon     = ExtractIcon(Globals.hInstance, 
                                        Globals.lpszIcoFile, 0);
    if (!Globals.hMainIcon) Globals.hMainIcon = 
                                  LoadIcon(0, MAKEINTRESOURCE(DEFAULTICON));

    DumpGlobals();				  
				  
    if (!prev){
	class.style = CS_HREDRAW | CS_VREDRAW;
	class.lpfnWndProc = NOTEPAD_WndProc;
	class.cbClsExtra = 0;
	class.cbWndExtra = 0;
	class.hInstance  = Globals.hInstance;
	class.hIcon      = LoadIcon (0, IDI_APPLICATION);
	class.hCursor    = LoadCursor (0, IDC_ARROW);
	class.hbrBackground = GetStockObject (WHITE_BRUSH);
	class.lpszMenuName = "bla\0";
	class.lpszClassName = (SEGPTR)className;
    }
    if (!RegisterClass (&class))
	return FALSE;

    Globals.hMainWnd = CreateWindow (className, winName, WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, 0, 
			LoadMenu(Globals.hInstance, STRING_MENU_Xx),
			Globals.hInstance, 0);
    MAIN_SelectLanguageByName(Globals.lpszLanguage);

    SetMenu(Globals.hMainWnd, Globals.hMainMenu);		
			
    ShowWindow (Globals.hMainWnd, show);
    UpdateWindow (Globals.hMainWnd);


    while (GetMessage (&msg, 0, 0, 0)) {
        TranslateMessage (&msg);
        DispatchMessage (&msg);
    }
    return 0;
}




/* Local Variables:    */
/* c-file-style: "GNU" */
/* End:                */
