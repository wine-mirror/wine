/*
 * Copyright 1998 Douglas Ridgway
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

#include <windows.h>
#include "globals.h"
#include "resource.h"

/* global variables */

HINSTANCE hInst;
char szAppName[9];
char szTitle[80];

BOOL InitApplication(HINSTANCE hInstance)
{
  WNDCLASSEX wc;

  /* Load the application name and description strings */

  LoadString(hInstance, IDS_APPNAME, szAppName, sizeof(szAppName));
  LoadString(hInstance, IDS_DESCRIPTION, szTitle, sizeof(szTitle));

  /* Fill in window class structure with parameters that describe the
     main window */

  wc.cbSize = sizeof(WNDCLASSEX);

  /* Load small icon image */
  wc.hIconSm = LoadImage(hInstance,
			 MAKEINTRESOURCEA(IDI_APPICON),
			 IMAGE_ICON,
			 16, 16,
			 0);

  wc.style         = CS_HREDRAW | CS_VREDRAW;             /* Class style(s) */
  wc.lpfnWndProc   = WndProc;                             /* Window Procedure */
  wc.cbClsExtra    = 0;                          /* No per-class extra data */
  wc.cbWndExtra    = 0;                         /* No per-window extra data */
  wc.hInstance     = hInstance;                      /* Owner of this class */
  wc.hIcon         = LoadIcon(hInstance, szAppName);  /* Icon name from .rc */
  wc.hCursor       = LoadCursor(NULL, IDC_ARROW);                 /* Cursor */
  wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);           /* Default color */
  wc.lpszMenuName  = szAppName;                       /* Menu name from .rc */
  wc.lpszClassName = szAppName;                      /* Name to register as */

  /* Register the window class and return FALSE if unsuccesful */

  if (!RegisterClassEx(&wc))
    {
      if (!RegisterClass((LPWNDCLASS)&wc.style))
	return FALSE;
    }

  /* Call module specific initialization functions here */

  return TRUE;
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    HWND hwnd;

    /* Save the instance handle in a global variable for later use */
    hInst = hInstance;

    /* Create main window */
    hwnd = CreateWindow(szAppName,           /* See RegisterClass() call */
                        szTitle,             /* window title */
                        WS_OVERLAPPEDWINDOW, /* Window style */
                        CW_USEDEFAULT, 0,    /* positioning */
                        CW_USEDEFAULT, 0,    /* size */
                        NULL,                /* Overlapped has no parent */
                        NULL,                /* Use the window class menu */
                        hInstance,
                        NULL);

    if (!hwnd)
        return FALSE;

    /* Call module specific instance initialization functions here */

    /* show the window, and paint it for the first time */
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    return TRUE;
}
