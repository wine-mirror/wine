/*
 * winebrowser - winelib app to launch native OS browser
 *
 * Copyright (C) 2004 Chris Morgan
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
 *
 * NOTES:
 *  Winebrowser is a winelib application that will start the appropriate
 *  native browser up for a wine installation that lacks a windows browser.
 *  Thus you will be able to open urls via native mozilla if no browser
 *  has yet been installed in wine.
 */

#include "config.h"
#include "wine/port.h"

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

/*****************************************************************************
 * Main entry point. This is a console application so we have a main() not a
 * winmain().
 */
int main (int argc, char *argv[])
{
  const char *argv_new[3];
  DWORD maxLength;
  CHAR szBrowsers[256];
  DWORD type;
  CHAR *defaultBrowsers = "mozilla,netscape,konqueror,galeon,opera";
  char *browser;
  HKEY hkey;
  LONG r;

  /* ensure that mozilla or other browsers don't think the */
  /* temp directory is "E:\\" */
  unsetenv("TEMP");
  unsetenv("TMP");

  maxLength = sizeof(szBrowsers);

  if(RegCreateKeyEx( HKEY_CURRENT_USER,
		      "Software\\Wine\\WineBrowser", 0, NULL,
		      REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
		      &hkey, NULL))
  {
    fprintf( stderr, "winebrowser: cannot create config key\n" );
    return 1;
  }

  r = RegQueryValueExA( hkey, "Browsers", 0, &type, szBrowsers, &maxLength);
  if(r != ERROR_SUCCESS)
  {
    /* set value to the default */
    RegSetValueExA(hkey, "Browsers", 0, REG_SZ,
		   (LPBYTE)defaultBrowsers, lstrlen(defaultBrowsers) + 1);
    strcpy( szBrowsers, defaultBrowsers );
  }

  RegCloseKey(hkey);


  /* now go through the list of browsers until we run out or we find one that */
  /* works */
  browser = strtok(szBrowsers, ",");

  while(browser)
  {
    argv_new[0] = browser;
    argv_new[1] = argv[1];
    argv_new[2] = NULL;

    spawnvp(_P_OVERLAY, browser, argv_new);  /* only returns on error */

    browser = strtok(NULL, ","); /* grab the next browser */
  }
  fprintf( stderr, "winebrowser: could not find a browser to run\n" );
  return 1;
}
