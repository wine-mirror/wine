/* Control Panel management
 *
 * Copyright 2001 Eric Pouech
 * Copyright 2008 Owen Rudge
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

#ifndef __WINE_SHELL_CPANEL_H
#define __WINE_SHELL_CPANEL_H

#include "cpl.h"

typedef struct CPlApplet {
    struct CPlApplet*   next;		/* linked list */
    HWND		hWnd;
    LPWSTR		cmd;        /* path to applet */
    unsigned		count;		/* number of subprograms */
    HMODULE     	hModule;	/* module of loaded applet */
    APPLET_PROC		proc;		/* entry point address */
    NEWCPLINFOW		info[1];	/* array of count information.
					 * dwSize field is 0 if entry is invalid */
} CPlApplet;

typedef struct CPanel {
    CPlApplet*  first;
    HWND        hWnd;
    HINSTANCE   hInst;
    unsigned    total_subprogs;
    HWND        hWndListView;
    HIMAGELIST  hImageListLarge;
    HIMAGELIST  hImageListSmall;
    HWND        hWndStatusBar;
} CPanel;

/* structure to reference an individual control panel item */
typedef struct CPlItem {
    CPlApplet *applet;
    unsigned id;
} CPlItem;

CPlApplet* Control_LoadApplet(HWND hWnd, LPCWSTR cmd, CPanel* panel) DECLSPEC_HIDDEN;
CPlApplet* Control_UnloadApplet(CPlApplet* applet) DECLSPEC_HIDDEN;

#endif /* __WINE_SHELL_CPANEL_H */
