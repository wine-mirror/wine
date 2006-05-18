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

#include <windows.h>            /* required for all Windows applications */
#include "globals.h"            /* prototypes specific to this application */


int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    MSG msg;
    HANDLE hAccelTable;

    /* Other instances of app running? */
    if (!hPrevInstance)
    {
      /* stuff to be done once */
      if (!InitApplication(hInstance))
      {
	return FALSE;              /* exit */
      }
    }

    /* stuff to be done every time */
    if (!InitInstance(hInstance, nCmdShow))
    {
      return FALSE;
    }

    hAccelTable = LoadAccelerators(hInstance, szAppName);

    /* Main loop */
    /* Acquire and dispatch messages until a WM_QUIT message is received */
    while (GetMessage(&msg, NULL, 0, 0))
    {
      /* Add other Translation functions (for modeless dialogs
	 and/or MDI windows) here. */

      if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    /* Add module specific instance free/delete functions here */

    /* Returns the value from PostQuitMessage */
    return msg.wParam;
}
