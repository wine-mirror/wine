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

CHAR STRING_MENU_Xx[]      = "MENU_Xx";

VOID LANGUAGE_UpdateMenuCheckmarks(VOID) {

    if(Globals.bAnalog == TRUE) {

        /* analog clock */

        CheckMenuItem(Globals.hPropertiesMenu, 0x100,
                       MF_BYCOMMAND | MF_CHECKED);
        CheckMenuItem(Globals.hPropertiesMenu, 0x101,
                       MF_BYCOMMAND | MF_UNCHECKED);
        EnableMenuItem(Globals.hPropertiesMenu, 0x103,
                       MF_BYCOMMAND | MF_GRAYED);
    }
        else
    {

        /* digital clock */

        CheckMenuItem(Globals.hPropertiesMenu, 0x100,
                       MF_BYCOMMAND | MF_UNCHECKED);
        CheckMenuItem(Globals.hPropertiesMenu, 0x101,
                       MF_BYCOMMAND | MF_CHECKED);
        EnableMenuItem(Globals.hPropertiesMenu, 0x103,
                       MF_BYCOMMAND);

    }

    CheckMenuItem(Globals.hPropertiesMenu, 0x105, MF_BYCOMMAND |
                 (Globals.bWithoutTitle ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(Globals.hSystemMenu, 0x10D, MF_BYCOMMAND |
                 (Globals.bAlwaysOnTop ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(Globals.hPropertiesMenu, 0x107, MF_BYCOMMAND |
                 (Globals.bSeconds ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(Globals.hPropertiesMenu, 0x108, MF_BYCOMMAND |
                 (Globals.bDate ? MF_CHECKED : MF_UNCHECKED));
}

VOID LANGUAGE_UpdateWindowCaption(VOID) {

  CHAR szCaption[MAX_STRING_LEN];
  CHAR szDate[MAX_STRING_LEN];

  LPSTR date = szDate;

  SYSTEMTIME st;
  LPSYSTEMTIME lpst = &st;

  GetLocalTime(&st);
  GetDateFormat(LOCALE_USER_DEFAULT, LOCALE_SLONGDATE, lpst, NULL, date,
                MAX_STRING_LEN);

  /* Set frame caption */
  LoadString(Globals.hInstance, 0x10C, szCaption, sizeof(szCaption));
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

  LoadString(Globals.hInstance, 0x10D, szItem, sizeof(szItem));
  AppendMenu(Globals.hSystemMenu, MF_STRING | MF_BYCOMMAND, 1000, szItem);
}

/*
VOID LANGUAGE_DefaultHandle(WPARAM wParam)
{
  if ((wParam >=CL_FIRST_LANGUAGE) && (wParam<=CL_LAST_LANGUAGE))
          LANGUAGE_SelectByNumber(wParam - CL_FIRST_LANGUAGE);
     else printf("Unimplemented menu command %i\n", wParam);
}
*/

/* Local Variables:    */
/* c-file-style: "GNU" */
/* End:                */
