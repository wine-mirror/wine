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
#include "commdlg.h"

#ifdef WINELIB
   #include "options.h"
   #include "resource.h"
   #include "shell.h"
   void LIBWINE_Register_Da();
   void LIBWINE_Register_De();
   void LIBWINE_Register_En();
   void LIBWINE_Register_Es();
   void LIBWINE_Register_Fr();
   void LIBWINE_Register_Sw();
   void LIBWINE_Register_Fi();
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
         LANGUAGE_UpdateMenuCheckmarks();
	 SendMessage(Globals.hMainWnd, WM_PAINT, 0, 0);
	 break;	     
       } 
     case CL_DIGITAL: {
         Globals.bAnalog = FALSE;
         LANGUAGE_UpdateMenuCheckmarks();
	 SendMessage(Globals.hMainWnd, WM_PAINT, 0, 0);
	 break;	       
       }
     case CL_FONT: {
         MAIN_FileChooseFont();
         break;
       }
     case CL_WITHOUT_TITLE: {     
         Globals.bWithoutTitle = !Globals.bWithoutTitle;
         LANGUAGE_UpdateWindowCaption();
         LANGUAGE_UpdateMenuCheckmarks();
         break;
       } 
     case CL_ON_TOP: {
         Globals.bAlwaysOnTop = !Globals.bAlwaysOnTop;
         LANGUAGE_UpdateMenuCheckmarks();
         break;
       }  
     case CL_SECONDS: {
         Globals.bSeconds = !Globals.bSeconds;
         LANGUAGE_UpdateMenuCheckmarks();
         SendMessage(Globals.hMainWnd, WM_PAINT, 0, 0);
         break;
       }
     case CL_DATE: {
         Globals.bDate = !Globals.bDate;
         LANGUAGE_UpdateMenuCheckmarks();
         LANGUAGE_UpdateWindowCaption();
         break;
       }
     case CL_INFO_LICENSE: {
         WineLicense(Globals.hMainWnd, Globals.lpszLanguage);
         break;
       }
     case CL_INFO_NO_WARRANTY: {
         WineWarranty(Globals.hMainWnd, Globals.lpszLanguage);
         break;
       }
     case CL_INFO_ABOUT_WINE: {
         ShellAbout(Globals.hMainWnd, "Clock", "Clock\n" WINE_RELEASE_INFO, 0);
         break;
       }
     /* Handle languages */
     default:
         LANGUAGE_DefaultHandle(wParam); 
   }
   return 0;
}

VOID MAIN_FileChooseFont(VOID) {

  CHOOSEFONT font;
  
        font.lStructSize     = sizeof(font);
        font.hwndOwner       = Globals.hMainWnd;
        font.hDC             = NULL;
        font.lpLogFont       = 0;
        font.iPointSize      = 0;
        font.Flags           = 0;
        font.rgbColors       = 0;
        font.lCustData       = 0;
        font.lpfnHook        = 0;
        font.lpTemplateName  = 0;
        font.hInstance       = Globals.hInstance;
/*        font.lpszStyle       = LF_FACESIZE; */
        font.nFontType       = 0;
        font.nSizeMin        = 0;
        font.nSizeMax        = 144;

        if (ChooseFont(&font)) {
            /* do nothing yet */
        };

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

        case WM_CREATE: {
            printf("WM_CREATE\n");
   	    break;
        }

        case WM_RBUTTONUP: {
	    printf("WM_RBUTTONUP\n");
            Globals.bWithoutTitle = !Globals.bWithoutTitle;
            LANGUAGE_UpdateMenuCheckmarks();               
            LANGUAGE_UpdateWindowCaption();
            UpdateWindow (Globals.hMainWnd);
            break;
        }

	case WM_PAINT: {
            printf("WM_PAINT\n");
            context = BeginPaint(hWnd, &ps);
	    if(Globals.bAnalog) {
	        DrawFace(context);
	        Idle(context);
	    }
	       else 
            {
               /* do nothing */
            }
            EndPaint(hWnd, &ps);
            break;
        }

        case WM_SIZE: {
            printf("WM_SIZE\n");
	    Globals.MaxX = LOWORD(lParam);
	    Globals.MaxY = HIWORD(lParam);
            OldHour.DontRedraw   = TRUE;
            OldMinute.DontRedraw = TRUE;
            OldSecond.DontRedraw = TRUE;
	    break;
        }
	  
        case WM_COMMAND: {
            CLOCK_MenuCommand(wParam);
            break;
        } 

        case WM_DESTROY: {
            printf("WM_DESTROY\n");
            PostQuitMessage (0);
            break;
        }

        default:
          return DefWindowProc (hWnd, msg, wParam, lParam);
    }
    return 0l;
}



/***********************************************************************
 *
 *           WinMain
 */

int PASCAL WinMain (HANDLE hInstance, HANDLE prev, LPSTR cmdline, int show)
{
    MSG      msg;
    WNDCLASS class;
    
    char szClassName[] = "CLClass";  /* To make sure className >= 0x10000 */
    char szWinName[]   = "Clock";

    #if defined(WINELIB) && !defined(HAVE_WINE_CONSTRUCTOR)
      /* Register resources */
      LIBWINE_Register_Da();
      LIBWINE_Register_De();
      LIBWINE_Register_En();
      LIBWINE_Register_Es();
      LIBWINE_Register_Fr();
      LIBWINE_Register_Sw();
      LIBWINE_Register_Fi();
    #endif

    /* Setup Globals */
    Globals.bAnalog	    = TRUE;
    Globals.bSeconds        = TRUE;
    Globals.lpszIniFile     = "clock.ini";
    Globals.lpszIcoFile     = "clock.ico";

    /* Select Language */
    LANGUAGE_Init();

    Globals.hInstance       = hInstance;
    Globals.hMainIcon       = ExtractIcon(Globals.hInstance, 
                                          Globals.lpszIcoFile, 0);
                                          
    if (!Globals.hMainIcon) Globals.hMainIcon = 
                                  LoadIcon(0, MAKEINTRESOURCE(DEFAULTICON));

    if (!prev){
	class.style         = CS_HREDRAW | CS_VREDRAW;
	class.lpfnWndProc   = CLOCK_WndProc;
	class.cbClsExtra    = 0;
	class.cbWndExtra    = 0;
	class.hInstance     = Globals.hInstance;
	class.hIcon         = LoadIcon (0, IDI_APPLICATION);
	class.hCursor       = LoadCursor (0, IDC_ARROW);
	class.hbrBackground = GetStockObject (GRAY_BRUSH);
	class.lpszMenuName  = 0;
	class.lpszClassName = szClassName;
    }

    if (!RegisterClass (&class)) return FALSE;

    Globals.hMainWnd = CreateWindow (szClassName, szWinName, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, Globals.MaxX, Globals.MaxY, 
        LoadMenu(Globals.hInstance, STRING_MENU_Xx), Globals.hInstance, 0);
			
    LANGUAGE_SelectByName(Globals.lpszLanguage);
    SetMenu(Globals.hMainWnd, Globals.hMainMenu);

    LANGUAGE_UpdateMenuCheckmarks();

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

    /* We will never reach the following statement !   */
    return 0;    
}

