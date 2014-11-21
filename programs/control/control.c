/*
 *   Start a control panel applet or open the control panel window
 *
 *   Copyright (C) 1998 by Marcel Baur <mbaur@g26.ethz.ch>
 *   Copyright 2010 Detlef Riekenberg
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

#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <shellapi.h>

/* alphabetical list of recognized optional command line parameters */
static const WCHAR szP_COLOR[] = {'C','O','L','O','R',0};
static const WCHAR szP_DATETIME[] = {'D','A','T','E','/','T','I','M','E',0};
static const WCHAR szP_DESKTOP[] = {'D','E','S','K','T','O','P',0};
static const WCHAR szP_INTERNATIONAL[] = {'I','N','T','E','R','N','A','T','I','O','N','A','L',0};
static const WCHAR szP_KEYBOARD[] = {'K','E','Y','B','O','A','R','D',0};
static const WCHAR szP_MOUSE[] = {'M','O','U','S','E',0};
static const WCHAR szP_PORTS[] = {'P','O','R','T','S',0};
static const WCHAR szP_PRINTERS [] = {'P','R','I','N','T','E','R','S',0};

/* alphabetical list of appropriate commands to execute */
static const WCHAR szC_COLOR[] = {'d','e','s','k','.','c','p','l',',',',','2',0};
static const WCHAR szC_DATETIME[] = {'t','i','m','e','d','a','t','e','.','c','p','l',0};
static const WCHAR szC_DESKTOP[] = {'d','e','s','k','.','c','p','l',0};
static const WCHAR szC_INTERNATIONAL[] = {'i','n','t','l','.','c','p','l',0};
static const WCHAR szC_KEYBOARD[] = {'m','a','i','n','.','c','p','l',' ','@','1',0};
static const WCHAR szC_MOUSE[] = {'m','a','i','n','.','c','p','l',0};
static const WCHAR szC_PORTS[] = {'s','y','s','d','m','.','c','p','l',',',',','1',0};
static const WCHAR szC_PRINTERS[] = {'m','a','i','n','.','c','p','l',' ','@','2',0};


extern void WINAPI Control_RunDLLW(HWND hWnd, HINSTANCE hInst, LPCWSTR cmd, DWORD nCmdShow);

static void launch(LPCWSTR what)
{
  Control_RunDLLW(GetDesktopWindow(), 0, what, SW_SHOW);
  ExitProcess(0);
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE hPrev, LPWSTR lpszCmdLine, INT nCmdShow)
{
    /* no parameters - pop up whole "Control Panel" by default */
    if (!*lpszCmdLine) {
        launch(lpszCmdLine);
    }

    /* check for optional parameter */
    if (!lstrcmpiW(lpszCmdLine,szP_COLOR))
        launch(szC_COLOR);
    if (!lstrcmpiW(lpszCmdLine,szP_DATETIME))
        launch(szC_DATETIME);
    if (!lstrcmpiW(lpszCmdLine,szP_DESKTOP))
        launch(szC_DESKTOP);
    if (!lstrcmpiW(lpszCmdLine,szP_INTERNATIONAL))
        launch(szC_INTERNATIONAL);
    if (!lstrcmpiW(lpszCmdLine,szP_KEYBOARD))
        launch(szC_KEYBOARD);
    if (!lstrcmpiW(lpszCmdLine,szP_MOUSE))
        launch(szC_MOUSE);
    if (!lstrcmpiW(lpszCmdLine,szP_PORTS))
        launch(szC_PORTS);
    if (!lstrcmpiW(lpszCmdLine,szP_PRINTERS))
        launch(szC_PRINTERS);

    /* try to launch if a .cpl file is given directly */
    launch(lpszCmdLine);
    return 0;
}
