/*
 * Clock
 *
 * Copyright 1998 Marcel Baur <mbaur@g26.ethz.ch>
 *
 * Clock is partially based on
 * - Program Manager by Ulrich Schmied
 * - rolex.c by Jim Peterson
 *
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

#include <stdio.h>
#include "windows.h"
#include "main.h"
#include "license.h"
#include "wine/version.h"
#include "language.h"
#include "winclock.h"
#include "commdlg.h"

CLOCK_GLOBALS Globals;

/***********************************************************************
 *
 *           CLOCK_MenuCommand
 *
 *  All handling of main menu events
 */

int CLOCK_MenuCommand (WPARAM wParam)
{  
CHAR szApp[MAX_STRING_LEN];
CHAR szAppRelease[MAX_STRING_LEN];
   switch (wParam) {
	/* switch to analog */
     case 0x100: { 
         Globals.bAnalog = TRUE;
         LANGUAGE_UpdateMenuCheckmarks();
	 SendMessage(Globals.hMainWnd, WM_PAINT, 0, 0);
	 break;	     
       } 
	/* switch to digital */
     case 0x101: {
         Globals.bAnalog = FALSE;
         LANGUAGE_UpdateMenuCheckmarks();
	 SendMessage(Globals.hMainWnd, WM_PAINT, 0, 0);
	 break;	       
       }
	/* change font */
     case 0x103: {
         MAIN_FileChooseFont();
         break;
       }
	/* hide title bar */
     case 0x105: {     
         Globals.bWithoutTitle = !Globals.bWithoutTitle;
         LANGUAGE_UpdateWindowCaption();
         LANGUAGE_UpdateMenuCheckmarks();
         break;
       } 
	/* always on top */
     case 0x10D: {
         Globals.bAlwaysOnTop = !Globals.bAlwaysOnTop;
         LANGUAGE_UpdateMenuCheckmarks();
         break;
       }  
	/* show or hide seconds */
     case 0x107: {
         Globals.bSeconds = !Globals.bSeconds;
         LANGUAGE_UpdateMenuCheckmarks();
         SendMessage(Globals.hMainWnd, WM_PAINT, 0, 0);
         break;
       }
	/* show or hide date */
     case 0x108: {
         Globals.bDate = !Globals.bDate;
         LANGUAGE_UpdateMenuCheckmarks();
         LANGUAGE_UpdateWindowCaption();
         break;
       }
	/* show license */
     case 0x109: {
         WineLicense(Globals.hMainWnd);
         break;
       }
	/* show warranties */
     case 0x10A: {
         WineWarranty(Globals.hMainWnd);
         break;
       }
	/* show "about" box */
     case 0x10B: {
         LoadString(Globals.hInstance, 0x10C, szApp, sizeof(szApp));
         lstrcpy(szAppRelease,szApp);
         lstrcat(szAppRelease,"\n" WINE_RELEASE_INFO);
         ShellAbout(Globals.hMainWnd, szApp, szAppRelease, 0);
         break;
       }
     /* Handle languages */
/*     default:
         LANGUAGE_DefaultHandle(wParam); 
*/
   }
   return 0;
}

VOID MAIN_FileChooseFont(VOID) {

  CHOOSEFONT font;
  LOGFONT	 lf;

        font.lStructSize     = sizeof(font);
        font.hwndOwner       = Globals.hMainWnd;
        font.hDC             = NULL;
        font.lpLogFont       = &lf;
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
        }

}

/***********************************************************************
 *
 *           CLOCK_WndProc
 */

LRESULT WINAPI CLOCK_WndProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
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

int PASCAL WinMain (HINSTANCE hInstance, HINSTANCE prev, LPSTR cmdline, int show)
{
    MSG      msg;
    WNDCLASS class;
    
    char szClassName[] = "CLClass";  /* To make sure className >= 0x10000 */
    char szWinName[]   = "Clock";

    /* Setup Globals */
    Globals.bAnalog	    = TRUE;
    Globals.bSeconds        = TRUE;
    Globals.lpszIniFile     = "clock.ini";
    Globals.lpszIcoFile     = "clock.ico";

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
        CW_USEDEFAULT, CW_USEDEFAULT, Globals.MaxX, Globals.MaxY, 0,
        LoadMenu(Globals.hInstance, STRING_MENU_Xx), Globals.hInstance, 0);
			
    LANGUAGE_LoadMenus();
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

