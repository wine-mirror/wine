/*
 * Notepad
 *
 * Copyright 1997 Marcel Baur <mbaur@g26.ethz.ch>
 */

#include <windows.h>

#include "main.h"
#include "license.h"

NOTEPAD_GLOBALS Globals;

CHAR STRING_MENU_Xx[]      = "MENU_En";
CHAR STRING_PAGESETUP_Xx[] = "DIALOG_PAGESETUP_En";

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
	return(1);
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
   printf("NOTEPAD_MenuCommand()\n");

   switch (wParam) {
     case NP_FILE_NEW:          break;
     case NP_FILE_SAVE:         break;
     case NP_FILE_SAVEAS:       break;
     case NP_FILE_PRINT:        break;
     case NP_FILE_PAGESETUP:    break;
     case NP_FILE_PRINTSETUP:   break;

     case NP_FILE_EXIT:         
        PostQuitMessage(0);
        break;

     case NP_EDIT_UNDO:         break;
     case NP_EDIT_CUT:          break;
     case NP_EDIT_COPY:         break;
     case NP_EDIT_PASTE:        break;
     case NP_EDIT_DELETE:       break;
     case NP_EDIT_TIMEDATE:     break;
     case NP_EDIT_WRAP:         
        Globals.bWrapLongLines = !Globals.bWrapLongLines;
	CheckMenuItem(Globals.hEditMenu, NP_EDIT_WRAP, MF_BYCOMMAND | 
                (Globals.bWrapLongLines ? MF_CHECKED : MF_UNCHECKED));
        break;

     case NP_SEARCH_SEARCH:     break;
     case NP_SEARCH_NEXT:       break;

     case NP_HELP_CONTENTS:     
        printf("NP_HELP_CONTENTS\n");
        WinHelp(Globals.hMainWnd, HELPFILE, HELP_INDEX, 0);
        break;

     case NP_HELP_SEARCH:       break;

     case NP_HELP_ON_HELP:      
        printf("NP_HELP_ON_HELP\n");
        WinHelp(Globals.hMainWnd, HELPFILE, HELP_HELPONHELP, 0);
        break;
	
     case NP_HELP_LICENSE:
        WineLicense(Globals.hMainWnd, Globals.lpszLanguage);
        break;
	
     case NP_HELP_NO_WARRANTY: 
        printf("NP_ABOUT_NO_WARRANTY\n");
        WineWarranty(Globals.hMainWnd, Globals.lpszLanguage);
        break;

     case NP_HELP_ABOUT_WINE:
        printf("NP_ABOUT_WINE\n");
        ShellAbout(Globals.hMainWnd, "WINE", "Notepad", 0);
        break;

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

    printf("DumpGlobals()");
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

    printf("WinMain()\n");
    
    /* Setup Globals */

    Globals.lpszIniFile   = "notepad.ini";
    Globals.lpszIcoFile   = "notepad.ico";
    Globals.lpszLanguage  = "En";
    Globals.hInstance     = hInstance;
    Globals.hMainMenu     = LoadMenu(Globals.hInstance, STRING_MENU_Xx);
    Globals.hFileMenu     = GetSubMenu(Globals.hMainMenu, 0);
    Globals.hEditMenu     = GetSubMenu(Globals.hMainMenu, 1);
    Globals.hSearchMenu   = GetSubMenu(Globals.hMainMenu, 2);
    Globals.hLanguageMenu = GetSubMenu(Globals.hMainMenu, 3);
    Globals.hHelpMenu     = GetSubMenu(Globals.hMainMenu, 4);
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
