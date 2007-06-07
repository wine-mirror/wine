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
#include <ddeml.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winebrowser);

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

        WINE_TRACE( "Considering: %s\n", app );
        WINE_TRACE( "argv[1]: %s\n", argv1 );

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
 * DDE helper functions.
 */

static char *ddeExec = NULL;
static HSZ hszTopic = 0;

/* Dde callback, save the execute string for processing */
static HDDEDATA CALLBACK ddeCb(UINT uType, UINT uFmt, HCONV hConv,
                                HSZ hsz1, HSZ hsz2, HDDEDATA hData,
                                ULONG_PTR dwData1, ULONG_PTR dwData2)
{
    DWORD size = 0;

    WINE_TRACE("dde_cb: %04x, %04x, %p, %p, %p, %p, %08lx, %08lx\n",
               uType, uFmt, hConv, hsz1, hsz2, hData, dwData1, dwData2);

    switch (uType)
    {
        case XTYP_CONNECT:
            if (!DdeCmpStringHandles(hsz1, hszTopic))
                return (HDDEDATA)TRUE;
            return (HDDEDATA)FALSE;

        case XTYP_EXECUTE:
            if (!(size = DdeGetData(hData, NULL, 0, 0)))
                WINE_ERR("DdeGetData returned zero size of execute string\n");
            else if (!(ddeExec = HeapAlloc(GetProcessHeap(), 0, size)))
                WINE_ERR("Out of memory\n");
            else if (DdeGetData(hData, (LPBYTE)ddeExec, size, 0) != size)
                WINE_WARN("DdeGetData did not return %d bytes\n", size);
            DdeFreeDataHandle(hData);
            return (HDDEDATA)DDE_FACK;

        default:
            return NULL;
    }
}

static char *get_url_from_dde(void)
{
    static const char szApplication[] = "IExplore";
    static const char szTopic[] = "WWW_OpenURL";

    HSZ hszApplication = 0;
    DWORD ddeInst = 0;
    UINT_PTR timer = 0;
    int rc;
    char *ret = NULL;

    rc = DdeInitializeA(&ddeInst, ddeCb, CBF_SKIP_ALLNOTIFICATIONS | CBF_FAIL_ADVISES |
                        CBF_FAIL_POKES | CBF_FAIL_REQUESTS, 0);
    if (rc != DMLERR_NO_ERROR)
    {
        WINE_ERR("Unable to initialize DDE, DdeInitialize returned %d\n", rc);
        goto done;
    }

    hszApplication = DdeCreateStringHandleA(ddeInst, szApplication, CP_WINANSI);
    if (!hszApplication)
    {
        WINE_ERR("Unable to initialize DDE, DdeCreateStringHandle failed\n");
        goto done;
    }

    hszTopic = DdeCreateStringHandleA(ddeInst, szTopic, CP_WINANSI);
    if (!hszTopic)
    {
        WINE_ERR("Unable to initialize DDE, DdeCreateStringHandle failed\n");
        goto done;
    }

    if (!DdeNameService(ddeInst, hszApplication, 0, DNS_REGISTER))
    {
        WINE_ERR("Unable to initialize DDE, DdeNameService failed\n");
        goto done;
    }

    timer = SetTimer(NULL, 0, 5000, NULL);
    if (!timer)
    {
        WINE_ERR("SetTimer failed to create timer\n");
        goto done;
    }

    while (!ddeExec)
    {
        MSG msg;
        if (!GetMessage(&msg, NULL, 0, 0)) break;
        if (msg.message == WM_TIMER) break;
        DispatchMessage(&msg);
    }

    if (ddeExec)
    {
        if (*ddeExec == '"')
        {
            char *endquote = strchr(ddeExec+1, '"');
            if (!endquote)
            {
                WINE_ERR("Unabled to retrieve URL from string '%s'\n", ddeExec);
                goto done;
            }
            *endquote = 0;
            ret = ddeExec+1;
        }
        else
            ret = ddeExec;
    }

done:
    if (timer) KillTimer(NULL, timer);
    if (ddeInst)
    {
        if (hszTopic && hszApplication) DdeNameService(ddeInst, hszApplication, 0, DNS_UNREGISTER);
        if (hszTopic) DdeFreeStringHandle(ddeInst, hszTopic);
        if (hszApplication) DdeFreeStringHandle(ddeInst, hszApplication);
        DdeUninitialize(ddeInst);
    }
    return ret;
}

/*****************************************************************************
 * Main entry point. This is a console application so we have a main() not a
 * winmain().
 */
int main(int argc, char *argv[])
{
    char *url = argv[1];
    wine_get_unix_file_name_t wine_get_unix_file_name_ptr;
    int ret = 1;

    /* DDE used only if -nohome is specified; avoids delay in printing usage info
     * when no parameters are passed */
    if (url && !strcasecmp( url, "-nohome" ))
        url = argc > 2 ? argv[2] : get_url_from_dde();

    if (!url)
    {
        fprintf( stderr, "Usage: winebrowser URL\n" );
        goto done;
    }

    /* handle an RFC1738 file URL */
    if (!strncasecmp( url, "file:", 5 ))
    {
        char *p;
        DWORD len = lstrlenA( url ) + 1;

        if (UrlUnescapeA( url, NULL, &len, URL_UNESCAPE_INPLACE ) != S_OK)
        {
            fprintf( stderr, "winebrowser: unescaping URL failed: %s\n", url );
            goto done;
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
            goto done;
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
        WINE_ERR( "cannot get the address of 'wine_get_unix_file_name'\n" );
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
            {
                ret = open_http_url( unixpath );
                goto done;
            }
        }
    }

    if (!strncasecmp( url, "mailto:", 7 ))
        ret = open_mailto_url( url );
    else
        /* let the browser decide how to handle the given url */
        ret = open_http_url( url );

done:
    HeapFree(GetProcessHeap(), 0, ddeExec);
    return ret;
}
