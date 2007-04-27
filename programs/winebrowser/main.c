/*
 * winebrowser - winelib app to launch native OS browser or mail client.
 *
 * Copyright (C) 2004 Chris Morgan
 * Copyright (C) 2005 Hans Leidekker
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
 *
 * NOTES:
 *  Winebrowser is a winelib application that will start the appropriate
 *  native browser or mail client for a wine installation that lacks a 
 *  windows browser/mail client. For example, you will be able to open
 *  urls via native mozilla if no browser has yet been installed in wine.
 *
 *  The application to launch is chosen from a default set or, if set,
 *  taken from a registry key.
 *  
 *  The argument may be a regular Windows file name, a file URL, an
 *  URL or a mailto URL. In the first three cases the argument
 *  will be fed to a web browser. In the last case the argument is fed
 *  to a mail client. A mailto URL is composed as follows:
 *
 *   mailto:[E-MAIL]?subject=[TOPIC]&cc=[E-MAIL]&bcc=[E-MAIL]&body=[TEXT]
 */

#define WIN32_LEAN_AND_MEAN

#include "config.h"
#include "wine/port.h"

#include <windows.h>
#include <shlwapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

typedef LPSTR (*wine_get_unix_file_name_t)(LPCWSTR unixname);

/* try to launch an app from a comma separated string of app names */
static int launch_app( char *candidates, const char *argv1 )
{
    char *app;
    const char *argv_new[3];

    app = strtok( candidates, "," );
    while (app)
    {
        argv_new[0] = app;
        argv_new[1] = argv1;
        argv_new[2] = NULL;

        fprintf( stderr, "Considering: %s\n", app );
        fprintf( stderr, "argv[1]: %s\n", argv1 );

        spawnvp( _P_OVERLAY, app, argv_new );  /* only returns on error */
        app = strtok( NULL, "," );  /* grab the next app */
    }
    fprintf( stderr, "winebrowser: could not find a suitable app to run\n" );
    return 1;
}

static int open_http_url( const char *url )
{
    static const char *defaultbrowsers =
        "xdg-open,firefox,konqueror,mozilla,netscape,galeon,opera,dillo";
    char browsers[256];

    DWORD length, type;
    HKEY key;
    LONG r;

    length = sizeof(browsers);
    /* @@ Wine registry key: HKCU\Software\Wine\WineBrowser */
    if  (!(r = RegOpenKey( HKEY_CURRENT_USER, "Software\\Wine\\WineBrowser", &key )))
    {
        r = RegQueryValueExA( key, "Browsers", 0, &type, (LPBYTE)browsers, &length );
        RegCloseKey( key );
    }
    if (r != ERROR_SUCCESS)
        strcpy( browsers, defaultbrowsers );

    return launch_app( browsers, url );
}

static int open_mailto_url( const char *url )
{
    static const char *defaultmailers =
        "xdg-email,mozilla-thunderbird,thunderbird,evolution";
    char mailers[256];

    DWORD length, type;
    HKEY key;
    LONG r;

    length = sizeof(mailers);
    /* @@ Wine registry key: HKCU\Software\Wine\WineBrowser */
    if (!(r = RegOpenKey( HKEY_CURRENT_USER, "Software\\Wine\\WineBrowser", &key )))
    {
        r = RegQueryValueExA( key, "Mailers", 0, &type, (LPBYTE)mailers, &length );
        RegCloseKey( key );
    }
    if (r != ERROR_SUCCESS)
        strcpy( mailers, defaultmailers );

    return launch_app( mailers, url );
}

/*****************************************************************************
 * Main entry point. This is a console application so we have a main() not a
 * winmain().
 */
int main(int argc, char *argv[])
{
    char *url = argv[1];
    wine_get_unix_file_name_t wine_get_unix_file_name_ptr;

    if (argc == 1)
    {
        fprintf( stderr, "Usage: winebrowser URL\n" );
        return 1;
    }

    /* handle an RFC1738 file URL */
    if (!strncasecmp( url, "file:", 5 ))
    {
        char *p;
        DWORD len = lstrlenA( url ) + 1;

        if (UrlUnescapeA( url, NULL, &len, URL_UNESCAPE_INPLACE ) != S_OK)
        {
            fprintf( stderr, "winebrowser: unescaping URL failed: %s\n", url );
            return 1;
        }

        /* look for a Windows path after 'file:' */
        p = url + 5;
        while (*p)
        {
            if (isalpha( p[0] ) && (p[1] == ':' || p[1] == '|')) break;
            p++;
        }
        if (!*p)
        {
            fprintf( stderr, "winebrowser: no valid Windows path in: %s\n", url );
            return 1;
        }

        if (p[1] == '|') p[1] = ':';
        url = p;
 
        while (*p)
        {
            if (*p == '/') *p = '\\';
            p++;
        }
    }

    /* check if the argument is a local file */
    wine_get_unix_file_name_ptr = (wine_get_unix_file_name_t)
        GetProcAddress( GetModuleHandle( "KERNEL32" ), "wine_get_unix_file_name" );

    if (wine_get_unix_file_name_ptr == NULL)
    {
        fprintf( stderr,
            "winebrowser: cannot get the address of 'wine_get_unix_file_name'\n" );
    }
    else
    {
        char *unixpath;
        WCHAR unixpathW[MAX_PATH];

        MultiByteToWideChar( CP_UNIXCP, 0, url, -1, unixpathW, MAX_PATH );
        if ((unixpath = wine_get_unix_file_name_ptr( unixpathW )))
        {
            struct stat dummy;

            if (stat( unixpath, &dummy ) >= 0)
                return open_http_url( unixpath );
        }
    }

    if (!strncasecmp( url, "mailto:", 7 ))
        return open_mailto_url( url );

    /* let the browser decide how to handle the given url */
    return open_http_url( url );
}
