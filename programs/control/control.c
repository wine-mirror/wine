/*
 *   Control
 *   Copyright (C) 1998 by Marcel Baur <mbaur@g26.ethz.ch>
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
#include <string.h>
#include <windows.h>
#include <shellapi.h>
#include "params.h"

extern void WINAPI Control_RunDLL(HWND hWnd, HINSTANCE hInst, LPCSTR cmd, DWORD nCmdShow);
void launch(const char *what)
{
  Control_RunDLL(GetDesktopWindow(), 0, what, SW_SHOW);
  exit(0);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpszCmdLine, INT nCmdShow)
{
    char szParams[255];
    lstrcpy(szParams, lpszCmdLine);
    CharUpper(szParams);

    /* no parameters - pop up whole "Control Panel" by default */
    if (!*szParams) {
	launch("");
	return 0;
    }

    /* check for optional parameter */
    if (!strcmp(szParams,szP_DESKTOP))
	launch(szC_DESKTOP);
    if (!strcmp(szParams,szP_COLOR))
	launch(szC_COLOR);
    if (!strcmp(szParams,szP_DATETIME))
	launch(szC_DATETIME);
    if (!strcmp(szParams,szP_DESKTOP))
	launch(szC_DESKTOP);
    if (!strcmp(szParams,szP_INTERNATIONAL))
	launch(szC_INTERNATIONAL);
    if (!strcmp(szParams,szP_KEYBOARD))
	launch(szC_KEYBOARD);
    if (!strcmp(szParams,szP_MOUSE))
	launch(szC_MOUSE);
    if (!strcmp(szParams,szP_PORTS))
	launch(szC_PORTS);
    if (!strcmp(szParams,szP_PRINTERS))
	launch(szC_PRINTERS);

    /* try to launch if a .cpl file is given directly */
    launch(szParams);
    return 0;
}
