/*
 * TTYDRV initialization code
 *
 * Copyright 2000 Alexandre Julliard
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

#include "config.h"

#include <stdio.h>

#include "winbase.h"
#include "wine/winbase16.h"
#include "wine/debug.h"
#include "ttydrv.h"

WINE_DEFAULT_DEBUG_CHANNEL(ttydrv);

int cell_width = 8;
int cell_height = 8;
int screen_rows = 50;  /* default value */
int screen_cols = 80;  /* default value */
WINDOW *root_window;


/***********************************************************************
 *           TTYDRV process initialisation routine
 */
static void process_attach(void)
{
#ifdef WINE_CURSES
    if ((root_window = initscr()))
    {
        werase(root_window);
        wrefresh(root_window);
    }
    getmaxyx(root_window, screen_rows, screen_cols);
#endif  /* WINE_CURSES */

    TTYDRV_GDI_Initialize();

    /* load display.dll */
    LoadLibrary16( "display" );
}


/***********************************************************************
 *           TTYDRV process termination routine
 */
static void process_detach(void)
{
#ifdef WINE_CURSES
    if (root_window) endwin();
#endif  /* WINE_CURSES */
}


/***********************************************************************
 *           TTYDRV initialisation routine
 */
BOOL WINAPI DllMain( HINSTANCE hinst, DWORD reason, LPVOID reserved )
{
    switch(reason)
    {
    case DLL_PROCESS_ATTACH:
        process_attach();
        break;

    case DLL_PROCESS_DETACH:
        process_detach();
        break;
    }
    return TRUE;
}
