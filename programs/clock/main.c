/*
 * Clock
 *
 * Copyright 1998 Marcel Baur <mbaur@g26.ethz.ch>
 *
 * Clock is partially based on
 * - Program Manager by Ulrich Schmied
 * - rolex.c by Jim Peterson
 *
 */

#include <stdio.h>
#include "windows.h"
#include "main.h"
#include "license.h"
#include "version.h"
#include "language.h"
#include "winclock.h"
// #include "commdlg.h"

#ifdef WINELIB
   #include "options.h"
   #include "resource.h"
   #include "shell.h"
   void LIBWINE_Register_De();
   void LIBWINE_Register_En();
   void LIBWINE_Register_Sw();
#endif

CLOCK_GLOBALS Globals;

/***********************************************************************
 *
 *           CLOCK_MenuCommand
 *
 *  All handling of main menu events
 */

int CLOCK_MenuCommand (WPARAM wParam)
{  
   switch (wParam) {
     case CL_ANALOG: {
         Globals.bAnalog = TRUE;
	 CheckMenuItem(Globals.hPropertiesMenu, CL_ANALOG, \
	               MF_BYCOMMAND | MF_CHECKED);
	 CheckMenuItem(Globals.hPropertiesMenu, CL_DIGITAL, \
	               MF_BYCOMMAND | MF_UNCHECKED);
	 break;	     
       } 
     case CL_DIGITAL: {
         Globals.bAnalog = FALSE;
	 CheckMenuItem(Globals.hPropertiesMenu, CL_ANALOG, \
	               MF_BYCOMMAND | MF_UNCHECKED);
	 CheckMenuItem(Globals.hPropertiesMenu, CL_DIGITAL, \
	               MF_BYCOMMAND | MF_CHECKED);
	 break;	       
       }
     case CL_FONT:
       printf("FONT:");
/*	CHOOSEFONT fontl;
	fontl.lStructSize	= 0;
        fontl.hwndOwner		= Globals.hMainWnd;
        fontl.hDC		= NULL;
        fontl.lpLogFont  	= 0;
        fontl.iPointSize	   	= 0;
        fontl.Flags		= 0;
        fontl.rgbColors		= 0;
        fontl.lCustData		= 0;
        fontl.lpfnHook		= 0;
        fontl.lpTemplateName	= 0;
        fontl.hInstance		= Globals.hInstance;
        fontl.lpszStyle		= 0;
        fontl.nFontType		= 0;
        fontl.nSizeMin		= 0;
        fontl.nSizeMax		= 100;

	if (ChooseFont(&font)); 
*/
       break;
     case CL_WITHOUT_TITLE:
       Globals.bWithoutTitle = !Globals.bWithoutTitle;
       CheckMenuItem(Globals.hPropertiesMenu, CL_WITHOUT_TITLE, MF_BYCOMMAND | \
                     (Globals.bWithoutTitle ? MF_CHECKED : MF_UNCHECKED));
       printf("NO TITLE:");
       break;
     case CL_ON_TOP:
       Globals.bAlwaysOnTop = !Globals.bAlwaysOnTop;
       CheckMenuItem(Globals.hPropertiesMenu, CL_ON_TOP, MF_BYCOMMAND | \
                     (Globals.bAlwaysOnTop ? MF_CHECKED : MF_UNCHECKED));
       break;
     case CL_SECONDS:
       Globals.bSeconds = !Globals.bSeconds;
       CheckMenuItem(Globals.hPropertiesMenu, CL_SECONDS, MF_BYCOMMAND | \
                     (Globals.bSeconds ? MF_CHECKED : MF_UNCHECKED));
       break;
     case CL_DATE:
       Globals.bDate = !Globals.bDate;
       CheckMenuItem(Globals.hPropertiesMenu, CL_DATE, MF_BYCOMMAND | \
                     (Globals.bDate ? MF_CHECKED : MF_UNCHECKED));
       break;
     case CL_INFO_LICENSE:
       WineLicense(Globals.hMainWnd, Globals.lpszLanguage);
       break;
     case CL_INFO_NO_WARRANTY:
       WineWarranty(Globals.hMainWnd, Globals.lpszLanguage);
       break;
     case CL_INFO_ABOUT_WINE:
       ShellAbout(Globals.hMainWnd, "Clock", "Clock\n" WINE_RELEASE_INFO, 0);
     break;
     // Handle languages
     default:
       LANGUAGE_DefaultLanguageHandle(wParam); 
   }
   return 0;
}



/***********************************************************************
 *
 *           CLOCK_WndProc
 */

LRESULT CLOCK_WndProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC context;

    switch (msg) {

       case WM_CREATE:
        printf("WM_CREATE\n");
   	break;

        case WM_RBUTTONUP:
	  printf("WM_RBUTTONUP\n");
	break;

	case WM_PAINT:
	  printf("WM_PAINT\n");
	  context = BeginPaint(hWnd, &ps);
	  if(Globals.bAnalog) {
	    DrawFace(context);
	    Idle(context);
	  }
	  else {

	  }
          EndPaint(hWnd, &ps);
   	break;

       case WM_SIZE:
          printf("WM_SIZE\n");
	  Globals.MaxX = LOWORD(lParam);
	  Globals.MaxY = HIWORD(lParam);
	  break;
	  
       case WM_COMMAND:
          CLOCK_MenuCommand(wParam);
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
    char className[] = "CLClass";  /* To make sure className >= 0x10000 */
    char winName[]   = "Clock";

    #if defined(WINELIB) && !defined(HAVE_WINE_CONSTRUCTOR)
      /* Register resources */
      LIBWINE_Register_De();
      LIBWINE_Register_En();
      LIBWINE_Register_Sw();
    #endif

    printf("WinMain()\n");
    
    /* Setup Globals */

    Globals.bAnalog	  = TRUE;
    Globals.bSeconds      = TRUE;
    Globals.lpszIniFile   = "clock.ini";
    Globals.lpszIcoFile   = "clock.ico";

  /* Select Language */
    LANGUAGE_InitLanguage();

    Globals.hInstance     = hInstance;
    Globals.hMainIcon     = ExtractIcon(Globals.hInstance, 
                                        Globals.lpszIcoFile, 0);
    if (!Globals.hMainIcon) Globals.hMainIcon = 
                                  LoadIcon(0, MAKEINTRESOURCE(DEFAULTICON));

    DumpGlobals();				  
				  
    if (!prev){
	class.style         = CS_HREDRAW | CS_VREDRAW;
	class.lpfnWndProc   = CLOCK_WndProc;
	class.cbClsExtra    = 0;
	class.cbWndExtra    = 0;
	class.hInstance     = Globals.hInstance;
	class.hIcon         = LoadIcon (0, IDI_APPLICATION);
	class.hCursor       = LoadCursor (0, IDC_ARROW);
	class.hbrBackground = GetStockObject (GRAY_BRUSH);
	class.lpszMenuName  = "\0";
	class.lpszClassName = (SEGPTR)className;
    }
    if (!RegisterClass (&class))
	return FALSE;

    Globals.hMainWnd = CreateWindow (className, winName, WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, 0, CW_USEDEFAULT, Globals.MaxX, Globals.MaxY, 
			LoadMenu(Globals.hInstance, STRING_MENU_Xx),
			Globals.hInstance, 0);
			
    LANGUAGE_SelectLanguageByName(Globals.lpszLanguage);
    SetMenu(Globals.hMainWnd, Globals.hMainMenu);
    ShowWindow (Globals.hMainWnd, show);
    UpdateWindow (Globals.hMainWnd);

    while (TRUE) {
      Sleep(1);
      if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) return msg.wParam;
	TranslateMessage(&msg);
	DispatchMessage(&msg);
	Idle(NULL);
      }
      else Idle(NULL);
    }

    // We will never reach the following statement !  
    return 0;    
}


