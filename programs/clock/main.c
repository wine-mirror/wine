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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdio.h>

#include "windows.h"
#include "commctrl.h"
#include "commdlg.h"
#include "shellapi.h"

#include "main.h"
#include "winclock.h"

#define INITIAL_WINDOW_SIZE 200
#define TIMER_ID 1

CLOCK_GLOBALS Globals;

static VOID CLOCK_UpdateMenuCheckmarks(VOID)
{
    HMENU hPropertiesMenu;
    hPropertiesMenu = GetSubMenu(Globals.hMainMenu, 0);
    if (!hPropertiesMenu)
	return;

    if(Globals.bAnalog) {

        /* analog clock */
        CheckMenuRadioItem(hPropertiesMenu, IDM_ANALOG, IDM_DIGITAL, IDM_ANALOG, MF_CHECKED);
        EnableMenuItem(hPropertiesMenu, IDM_FONT, MF_GRAYED);
    }
    else
    {
        /* digital clock */
        CheckMenuRadioItem(hPropertiesMenu, IDM_ANALOG, IDM_DIGITAL, IDM_DIGITAL, MF_CHECKED);
        EnableMenuItem(hPropertiesMenu, IDM_FONT, 0);
    }

    CheckMenuItem(hPropertiesMenu, IDM_NOTITLE, (Globals.bWithoutTitle ? MF_CHECKED : MF_UNCHECKED));

    CheckMenuItem(hPropertiesMenu, IDM_ONTOP, (Globals.bAlwaysOnTop ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hPropertiesMenu, IDM_SECONDS, (Globals.bSeconds ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hPropertiesMenu, IDM_DATE, (Globals.bDate ? MF_CHECKED : MF_UNCHECKED));
}

static VOID CLOCK_UpdateWindowCaption(VOID)
{
    WCHAR szCaption[MAX_STRING_LEN];
    int chars = 0;

    /* Set frame caption */
    if (Globals.bDate) {
	chars = GetDateFormatW(LOCALE_USER_DEFAULT, DATE_LONGDATE, NULL, NULL,
                               szCaption, ARRAY_SIZE(szCaption));
        if (chars) {
	    --chars;
	    szCaption[chars++] = ' ';
	    szCaption[chars++] = '-';
	    szCaption[chars++] = ' ';
	    szCaption[chars] = '\0';
	}
    }
    LoadStringW(0, IDS_CLOCK, szCaption + chars, MAX_STRING_LEN - chars);
    SetWindowTextW(Globals.hMainWnd, szCaption);
}

/***********************************************************************
 *
 *           CLOCK_ResetTimer
 */
static BOOL CLOCK_ResetTimer(void)
{
    UINT period; /* milliseconds */

    KillTimer(Globals.hMainWnd, TIMER_ID);

    if (Globals.bSeconds)
	if (Globals.bAnalog)
	    period = 50;
	else
	    period = 500;
    else
	period = 1000;

    if (!SetTimer (Globals.hMainWnd, TIMER_ID, period, NULL)) {
        WCHAR szApp[MAX_STRING_LEN];
        LoadStringW(Globals.hInstance, IDS_CLOCK, szApp, MAX_STRING_LEN);
        MessageBoxW(0, L"No available timers", szApp, MB_ICONEXCLAMATION | MB_OK);
        return FALSE;
    }
    return TRUE;
}

/***********************************************************************
 *
 *           CLOCK_ResetFont
 */
static VOID CLOCK_ResetFont(VOID)
{
    HFONT newfont;
    HDC dc = GetDC(Globals.hMainWnd);
    newfont = SizeFont(dc, Globals.MaxX, Globals.MaxY, Globals.bSeconds, &Globals.logfont);
    if (newfont) {
	DeleteObject(Globals.hFont);
	Globals.hFont = newfont;
    }
	
    ReleaseDC(Globals.hMainWnd, dc);
}


/***********************************************************************
 *
 *           CLOCK_ChooseFont
 */
static VOID CLOCK_ChooseFont(VOID)
{
    LOGFONTW lf;
    CHOOSEFONTW cf;
    memset(&cf, 0, sizeof(cf));
    lf = Globals.logfont;
    cf.lStructSize = sizeof(cf);
    cf.hwndOwner = Globals.hMainWnd;
    cf.lpLogFont = &lf;
    cf.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT | CF_NOVERTFONTS;
    if (ChooseFontW(&cf)) {
	Globals.logfont = lf;
	CLOCK_ResetFont();
    }
}

/***********************************************************************
 *
 *           CLOCK_ToggleTitle
 */
static VOID CLOCK_ToggleTitle(VOID)
{
    /* Also shows/hides the menu */
    LONG style = GetWindowLongW(Globals.hMainWnd, GWL_STYLE);
    if ((Globals.bWithoutTitle = !Globals.bWithoutTitle)) {
	style = (style & ~WS_OVERLAPPEDWINDOW) | WS_POPUP|WS_THICKFRAME;
	SetMenu(Globals.hMainWnd, 0);
    }
    else {
	style = (style & ~(WS_POPUP|WS_THICKFRAME)) | WS_OVERLAPPEDWINDOW;
        SetMenu(Globals.hMainWnd, Globals.hMainMenu);
        SetWindowRgn(Globals.hMainWnd, 0, TRUE);
    }
    SetWindowLongW(Globals.hMainWnd, GWL_STYLE, style);
    SetWindowPos(Globals.hMainWnd, 0,0,0,0,0, 
		 SWP_DRAWFRAME|SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER);
    
    CLOCK_UpdateMenuCheckmarks();
    CLOCK_UpdateWindowCaption();
}

/***********************************************************************
 *
 *           CLOCK_ToggleOnTop
 */
static VOID CLOCK_ToggleOnTop(VOID)
{
    if ((Globals.bAlwaysOnTop = !Globals.bAlwaysOnTop)) {
	SetWindowPos(Globals.hMainWnd, HWND_TOPMOST, 0,0,0,0,
		     SWP_NOMOVE|SWP_NOSIZE);
    }
    else {
	SetWindowPos(Globals.hMainWnd, HWND_NOTOPMOST, 0,0,0,0,
		     SWP_NOMOVE|SWP_NOSIZE);
    }
    CLOCK_UpdateMenuCheckmarks();
}
/***********************************************************************
 *
 *           CLOCK_MenuCommand
 *
 *  All handling of main menu events
 */

static int CLOCK_MenuCommand (WPARAM wParam)
{
    WCHAR szApp[MAX_STRING_LEN];
    WCHAR szAppRelease[MAX_STRING_LEN];
    switch (wParam) {
        /* switch to analog */
        case IDM_ANALOG: {
            Globals.bAnalog = TRUE;
            CLOCK_UpdateMenuCheckmarks();
	    CLOCK_ResetTimer();
	    InvalidateRect(Globals.hMainWnd, NULL, FALSE);
            break;
        }
            /* switch to digital */
        case IDM_DIGITAL: {
            Globals.bAnalog = FALSE;
            CLOCK_UpdateMenuCheckmarks();
	    CLOCK_ResetTimer();
	    CLOCK_ResetFont();
	    InvalidateRect(Globals.hMainWnd, NULL, FALSE);
            break;
        }
            /* change font */
        case IDM_FONT: {
            CLOCK_ChooseFont();
            break;
        }
            /* hide title bar */
        case IDM_NOTITLE: {
	    CLOCK_ToggleTitle();
            break;
        }
            /* always on top */
        case IDM_ONTOP: {
	    CLOCK_ToggleOnTop();
            break;
        }
            /* show or hide seconds */
        case IDM_SECONDS: {
            Globals.bSeconds = !Globals.bSeconds;
            CLOCK_UpdateMenuCheckmarks();
	    CLOCK_ResetTimer();
	    if (!Globals.bAnalog)
		CLOCK_ResetFont();
	    InvalidateRect(Globals.hMainWnd, NULL, FALSE);
            break;
        }
            /* show or hide date */
        case IDM_DATE: {
            Globals.bDate = !Globals.bDate;
            CLOCK_UpdateMenuCheckmarks();
            CLOCK_UpdateWindowCaption();
            break;
        }
            /* show "about" box */
        case IDM_ABOUT: {
            LoadStringW(Globals.hInstance, IDS_CLOCK, szApp, ARRAY_SIZE(szApp));
            lstrcpyW(szAppRelease,szApp);
            ShellAboutW(Globals.hMainWnd, szApp, szAppRelease, 0);
            break;
        }
    }
    return 0;
}

/***********************************************************************
 *
 *           CLOCK_Paint
 */
static VOID CLOCK_Paint(HWND hWnd)
{
    PAINTSTRUCT ps;
    HDC dcMem, dc;
    HBITMAP bmMem, bmOld;

    dc = BeginPaint(hWnd, &ps);

    /* Use an offscreen dc to avoid flicker */
    dcMem = CreateCompatibleDC(dc);
    bmMem = CreateCompatibleBitmap(dc, ps.rcPaint.right - ps.rcPaint.left,
				    ps.rcPaint.bottom - ps.rcPaint.top);

    bmOld = SelectObject(dcMem, bmMem);

    SetViewportOrgEx(dcMem, -ps.rcPaint.left, -ps.rcPaint.top, NULL);
    /* Erase the background */
    FillRect(dcMem, &ps.rcPaint, GetSysColorBrush(COLOR_3DFACE));

    if(Globals.bAnalog)
	AnalogClock(dcMem, Globals.MaxX, Globals.MaxY, Globals.bSeconds, Globals.bWithoutTitle);
    else
	DigitalClock(dcMem, Globals.MaxX, Globals.MaxY, Globals.bSeconds, Globals.hFont);

    /* Blit the changes to the screen */
    BitBlt(dc, 
	   ps.rcPaint.left, ps.rcPaint.top,
	   ps.rcPaint.right - ps.rcPaint.left, ps.rcPaint.bottom - ps.rcPaint.top,
           dcMem,
	   ps.rcPaint.left, ps.rcPaint.top,
           SRCCOPY);

    SelectObject(dcMem, bmOld);
    DeleteObject(bmMem);
    DeleteDC(dcMem);
    
    EndPaint(hWnd, &ps);
}

/***********************************************************************
 *
 *           CLOCK_WndProc
 */

static LRESULT WINAPI CLOCK_WndProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
	/* L button drag moves the window */
        case WM_NCHITTEST: {
	    LRESULT ret = DefWindowProcW(hWnd, msg, wParam, lParam);
	    if (ret == HTCLIENT)
		ret = HTCAPTION;
            return ret;
	}

        case WM_NCLBUTTONDBLCLK:
        case WM_LBUTTONDBLCLK: {
	    CLOCK_ToggleTitle();
            break;
        }

        case WM_PAINT: {
	    CLOCK_Paint(hWnd);
            break;

        }

        case WM_SIZE: {
            Globals.MaxX = LOWORD(lParam);
            Globals.MaxY = HIWORD(lParam);
            if (Globals.bAnalog && Globals.bWithoutTitle)
            {
                RECT rect;
                INT diameter = min( Globals.MaxX, Globals.MaxY );
                HRGN hrgn = CreateEllipticRgn( (Globals.MaxX - diameter) / 2,
                                               (Globals.MaxY - diameter) / 2,
                                               (Globals.MaxX + diameter) / 2,
                                               (Globals.MaxY + diameter) / 2 );
                GetWindowRect( hWnd, &rect );
                MapWindowPoints( 0, hWnd, (LPPOINT)&rect, 2 );
                OffsetRgn( hrgn, -rect.left, -rect.top );
                SetWindowRgn( Globals.hMainWnd, hrgn, TRUE );
            }
	    CLOCK_ResetFont();
            break;
        }

        case WM_COMMAND: {
            CLOCK_MenuCommand(wParam);
            break;
        }
            
        case WM_TIMER: {
            /* Could just invalidate what has changed,
             * but it doesn't really seem worth the effort
             */
	    InvalidateRect(Globals.hMainWnd, NULL, FALSE);
	    break;
        }

        case WM_DESTROY: {
            PostQuitMessage (0);
            break;
        }

        default:
            return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
    return 0;
}


/***********************************************************************
 *
 *           WinMain
 */

int PASCAL WinMain (HINSTANCE hInstance, HINSTANCE prev, LPSTR cmdline, int show)
{
    MSG      msg;
    WNDCLASSW class;

    InitCommonControls();

    /* Setup Globals */
    memset(&Globals.hFont, 0, sizeof (Globals.hFont));
    Globals.bAnalog         = TRUE;
    Globals.bSeconds        = TRUE;
    
    if (!prev){
        class.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        class.lpfnWndProc   = CLOCK_WndProc;
        class.cbClsExtra    = 0;
        class.cbWndExtra    = 0;
        class.hInstance     = hInstance;
        class.hIcon         = LoadIconW(0, (LPCWSTR)IDI_APPLICATION);
        class.hCursor       = LoadCursorW(0, (LPCWSTR)IDC_ARROW);
        class.hbrBackground = 0;
        class.lpszMenuName  = 0;
        class.lpszClassName = L"CLClass";
    }

    if (!RegisterClassW(&class)) return FALSE;

    Globals.MaxX = Globals.MaxY = INITIAL_WINDOW_SIZE;
    Globals.hMainWnd = CreateWindowW(L"CLClass", L"Clock", WS_OVERLAPPEDWINDOW,
                                     CW_USEDEFAULT, CW_USEDEFAULT,
                                     Globals.MaxX, Globals.MaxY, 0,
                                     0, hInstance, 0);

    if (!CLOCK_ResetTimer())
        return FALSE;

    Globals.hMainMenu = LoadMenuW(0, MAKEINTRESOURCEW(MAIN_MENU));
    SetMenu(Globals.hMainWnd, Globals.hMainMenu);
    CLOCK_UpdateMenuCheckmarks();
    CLOCK_UpdateWindowCaption();
    
    ShowWindow (Globals.hMainWnd, show);
    UpdateWindow (Globals.hMainWnd);
    
    while (GetMessageW(&msg, 0, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    KillTimer(Globals.hMainWnd, TIMER_ID);
    DeleteObject(Globals.hFont);

    return 0;
}
