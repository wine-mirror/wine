/*
 * Clock
 *
 * Copyright 1998 Marcel Baur <mbaur@g26.ethz.ch>
 * Copyright 1998 Karl Backström <karl_b@geocities.com>
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
#include "language.h"
#include "winnls.h"

VOID LANGUAGE_UpdateMenuCheckmarks(VOID)
{
    if(Globals.bAnalog == TRUE) {

        /* analog clock */

        CheckMenuItem(Globals.hPropertiesMenu, IDM_ANALOG, MF_BYCOMMAND | MF_CHECKED);
        CheckMenuItem(Globals.hPropertiesMenu, IDM_DIGITAL, MF_BYCOMMAND | MF_UNCHECKED);
        EnableMenuItem(Globals.hPropertiesMenu, IDM_FONT, MF_BYCOMMAND | MF_GRAYED);
    }
    else
    {
        /* digital clock */

        CheckMenuItem(Globals.hPropertiesMenu, IDM_ANALOG, MF_BYCOMMAND | MF_UNCHECKED);
        CheckMenuItem(Globals.hPropertiesMenu, IDM_DIGITAL, MF_BYCOMMAND | MF_CHECKED);
        EnableMenuItem(Globals.hPropertiesMenu, IDM_FONT, MF_BYCOMMAND);
    }

    CheckMenuItem(Globals.hPropertiesMenu, IDM_NOTITLE, MF_BYCOMMAND |
                 (Globals.bWithoutTitle ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(Globals.hSystemMenu, IDM_ONTOP, MF_BYCOMMAND |
                 (Globals.bAlwaysOnTop ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(Globals.hPropertiesMenu, IDM_SECONDS, MF_BYCOMMAND |
                 (Globals.bSeconds ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(Globals.hPropertiesMenu, IDM_DATE, MF_BYCOMMAND |
                 (Globals.bDate ? MF_CHECKED : MF_UNCHECKED));
}

VOID LANGUAGE_UpdateWindowCaption(VOID)
{
    CHAR szCaption[MAX_STRING_LEN];
    CHAR szDate[MAX_STRING_LEN];

    SYSTEMTIME st;

    GetLocalTime(&st);
    GetDateFormat(LOCALE_USER_DEFAULT, LOCALE_SLONGDATE, &st, NULL, szDate, sizeof(szDate));

    /* Set frame caption */
    LoadString(Globals.hInstance, IDS_CLOCK, szCaption, sizeof(szCaption));
    if (Globals.bDate) {
        lstrcat(szCaption, " - ");
        lstrcat(szCaption, szDate);
    }
    SetWindowText(Globals.hMainWnd, szCaption);
}

VOID LANGUAGE_LoadMenus(VOID)
{
    CHAR   szItem[MAX_STRING_LEN];
    HMENU  hMainMenu;

    /* Create menu */
    hMainMenu = LoadMenu(Globals.hInstance, MAKEINTRESOURCE(MAIN_MENU));
    Globals.hPropertiesMenu     = GetSubMenu(hMainMenu, 0);
    Globals.hLanguageMenu       = GetSubMenu(hMainMenu, 1);
    Globals.hInfoMenu           = GetSubMenu(hMainMenu, 2);
    
    SetMenu(Globals.hMainWnd, hMainMenu);

    /* Destroy old menu */
    if (Globals.hMainMenu) DestroyMenu(Globals.hMainMenu);
    Globals.hMainMenu = hMainMenu;

    /* specific for Clock: */

    LANGUAGE_UpdateMenuCheckmarks();
    LANGUAGE_UpdateWindowCaption();

    Globals.hSystemMenu = GetSystemMenu(Globals.hMainWnd, TRUE);
    
    /* FIXME: Append a SEPARATOR to Globals.hSystemMenu here */

    LoadString(Globals.hInstance, IDS_ONTOP, szItem, sizeof(szItem));
    AppendMenu(Globals.hSystemMenu, MF_STRING | MF_BYCOMMAND, IDM_ONTOP, szItem);
}

/* Local Variables:    */
/* c-file-style: "GNU" */
/* End:                */
