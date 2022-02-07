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
 *  URLs via native mozilla if no browser has yet been installed in wine.
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
#define COBJMACROS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windows.h>
#include <winternl.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <urlmon.h>
#include <ddeml.h>

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winebrowser);

static char *strdup_unixcp( const WCHAR *str )
{
    char *ret;
    int len = WideCharToMultiByte( CP_UNIXCP, 0, str, -1, NULL, 0, NULL, NULL );
    if ((ret = malloc( len )))
        WideCharToMultiByte( CP_UNIXCP, 0, str, -1, ret, len, NULL, NULL );
    return ret;
}

/* try to launch a unix app from a comma separated string of app names */
static int launch_app( const WCHAR *candidates, const WCHAR *argv1 )
{
    char *cmdline;
    int i, count;
    char **argv_new;

    if (!(cmdline = strdup_unixcp( argv1 ))) return 1;

    while (*candidates)
    {
        WCHAR **args = CommandLineToArgvW( candidates, &count );

        if (!(argv_new = malloc( (count + 2) * sizeof(*argv_new) ))) break;
        for (i = 0; i < count; i++) argv_new[i] = strdup_unixcp( args[i] );
        argv_new[count] = cmdline;
        argv_new[count + 1] = NULL;

        TRACE( "Trying" );
        for (i = 0; i <= count; i++) TRACE( " %s", wine_dbgstr_a( argv_new[i] ));
        TRACE( "\n" );

        if (!__wine_unix_spawnvp( argv_new, FALSE )) ExitProcess(0);
        for (i = 0; i < count; i++) free( argv_new[i] );
        free( argv_new );
        candidates += lstrlenW( candidates ) + 1;  /* grab the next app */
    }
    WINE_ERR( "could not find a suitable app to open %s\n", debugstr_w( argv1 ));

    free( cmdline );
    return 1;
}

static LSTATUS get_commands( HKEY key, const WCHAR *value, WCHAR *buffer, DWORD size )
{
    DWORD type;
    LSTATUS res;

    size -= sizeof(WCHAR);
    if (!(res = RegQueryValueExW( key, value, 0, &type, (LPBYTE)buffer, &size )) && (type == REG_SZ))
    {
        /* convert to REG_MULTI_SZ type */
        WCHAR *p = buffer;
        p[lstrlenW(p) + 1] = 0;
        while ((p = wcschr( p, ',' ))) *p++ = 0;
    }
    return res;
}

static int open_http_url( const WCHAR *url )
{
    static const WCHAR defaultbrowsers[] =
        L"xdg-open\0"
        "/usr/bin/open\0"
        "firefox\0"
        "konqueror\0"
        "mozilla\0"
        "opera\0"
        "dillo\0";
    WCHAR browsers[256];
    HKEY key;
    LONG r;

    /* @@ Wine registry key: HKCU\Software\Wine\WineBrowser */
    if  (!(r = RegOpenKeyW( HKEY_CURRENT_USER, L"Software\\Wine\\WineBrowser", &key )))
    {
        r = get_commands( key, L"Browsers", browsers, sizeof(browsers) );
        RegCloseKey( key );
    }
    if (r != ERROR_SUCCESS)
        memcpy( browsers, defaultbrowsers, sizeof(defaultbrowsers) );

    return launch_app( browsers, url );
}

static int open_mailto_url( const WCHAR *url )
{
    static const WCHAR defaultmailers[] =
        L"/usr/bin/open\0"
        "xdg-email\0"
        "mozilla-thunderbird\0"
        "thunderbird\0"
        "evolution\0";
    WCHAR mailers[256];
    HKEY key;
    LONG r;

    /* @@ Wine registry key: HKCU\Software\Wine\WineBrowser */
    if (!(r = RegOpenKeyW( HKEY_CURRENT_USER, L"Software\\Wine\\WineBrowser", &key )))
    {
        r = get_commands( key, L"Mailers", mailers, sizeof(mailers) );
        RegCloseKey( key );
    }
    if (r != ERROR_SUCCESS)
        memcpy( mailers, defaultmailers, sizeof(defaultmailers) );

    return launch_app( mailers, url );
}

static int open_invalid_url( const WCHAR *url )
{
    WCHAR *url_prefixed;
    int ret;

    url_prefixed = malloc( (ARRAY_SIZE(L"http://") + lstrlenW( url )) * sizeof(WCHAR) );
    if (!url_prefixed)
    {
        WINE_ERR("Out of memory\n");
        return 1;
    }

    lstrcpyW( url_prefixed, L"http://" );
    lstrcatW( url_prefixed, url );

    ret = open_http_url( url_prefixed );
    free( url_prefixed );
    return ret;
}

/*****************************************************************************
 * DDE helper functions.
 */

static WCHAR *ddeString = NULL;
static HSZ hszTopic = 0, hszReturn = 0;
static DWORD ddeInst = 0;

/* Dde callback, save the execute or request string for processing */
static HDDEDATA CALLBACK ddeCb(UINT uType, UINT uFmt, HCONV hConv,
                                HSZ hsz1, HSZ hsz2, HDDEDATA hData,
                                ULONG_PTR dwData1, ULONG_PTR dwData2)
{
    DWORD size = 0, ret = 0;

    WINE_TRACE("dde_cb: %04x, %04x, %p, %p, %p, %p, %08Ix, %08Ix\n",
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
            else if (!(ddeString = malloc(size)))
                WINE_ERR("Out of memory\n");
            else if (DdeGetData(hData, (LPBYTE)ddeString, size, 0) != size)
                WINE_WARN("DdeGetData did not return %ld bytes\n", size);
            DdeFreeDataHandle(hData);
            return (HDDEDATA)DDE_FACK;

        case XTYP_REQUEST:
            ret = -3; /* error */
            if (!(size = DdeQueryStringW(ddeInst, hsz2, NULL, 0, CP_WINUNICODE)))
                WINE_ERR("DdeQueryString returned zero size of request string\n");
            else if (!(ddeString = malloc( (size + 1) * sizeof(WCHAR))))
                WINE_ERR("Out of memory\n");
            else if (DdeQueryStringW(ddeInst, hsz2, ddeString, size + 1, CP_WINUNICODE) != size)
                WINE_WARN("DdeQueryString did not return %ld characters\n", size);
            else
                ret = -2; /* acknowledgment */
            return DdeCreateDataHandle(ddeInst, (LPBYTE)&ret, sizeof(ret), 0,
                                       hszReturn, CF_TEXT, 0);

        default:
            return NULL;
    }
}

static WCHAR *get_url_from_dde(void)
{
    HSZ hszApplication = 0;
    UINT_PTR timer = 0;
    int rc;
    WCHAR *ret = NULL;

    rc = DdeInitializeW(&ddeInst, ddeCb, CBF_SKIP_ALLNOTIFICATIONS | CBF_FAIL_ADVISES | CBF_FAIL_POKES, 0);
    if (rc != DMLERR_NO_ERROR)
    {
        WINE_ERR("Unable to initialize DDE, DdeInitialize returned %d\n", rc);
        goto done;
    }

    hszApplication = DdeCreateStringHandleW(ddeInst, L"IExplore", CP_WINUNICODE);
    if (!hszApplication)
    {
        WINE_ERR("Unable to initialize DDE, DdeCreateStringHandle failed\n");
        goto done;
    }

    hszTopic = DdeCreateStringHandleW(ddeInst, L"WWW_OpenURL", CP_WINUNICODE);
    if (!hszTopic)
    {
        WINE_ERR("Unable to initialize DDE, DdeCreateStringHandle failed\n");
        goto done;
    }

    hszReturn = DdeCreateStringHandleW(ddeInst, L"Return", CP_WINUNICODE);
    if (!hszReturn)
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

    while (!ddeString)
    {
        MSG msg;
        if (!GetMessageW(&msg, NULL, 0, 0)) break;
        if (msg.message == WM_TIMER) break;
        DispatchMessageW(&msg);
    }

    if (ddeString)
    {
        if (*ddeString == '"')
        {
            WCHAR *endquote = wcschr(ddeString + 1, '"');
            if (!endquote)
            {
                WINE_ERR("Unable to retrieve URL from string %s\n", wine_dbgstr_w(ddeString));
                goto done;
            }
            *endquote = 0;
            ret = ddeString+1;
        }
        else
            ret = ddeString;
    }

done:
    if (timer) KillTimer(NULL, timer);
    if (ddeInst)
    {
        if (hszTopic && hszApplication) DdeNameService(ddeInst, hszApplication, 0, DNS_UNREGISTER);
        if (hszReturn) DdeFreeStringHandle(ddeInst, hszReturn);
        if (hszTopic) DdeFreeStringHandle(ddeInst, hszTopic);
        if (hszApplication) DdeFreeStringHandle(ddeInst, hszApplication);
        DdeUninitialize(ddeInst);
    }
    return ret;
}

static WCHAR *encode_unix_path(const char *src)
{
    WCHAR *dst, *tmp_dst;
    static const char safe_chars[] = "/-_.~@&=+$,:";
    static const char hex_digits[] = "0123456789ABCDEF";

    dst = malloc( sizeof(L"file://") + 3 * strlen(src) * sizeof(WCHAR) );
    lstrcpyW(dst, L"file://");

    tmp_dst = dst + lstrlenW(dst);

    while (*src != 0)
    {
        if ((*src >= 'a' && *src <= 'z') ||
            (*src >= 'A' && *src <= 'Z') ||
            (*src >= '0' && *src <= '9') ||
            strchr(safe_chars, *src))
        {
            *tmp_dst++ = *src;
        }
        else
        {
            *tmp_dst++ = '%';
            *tmp_dst++ = hex_digits[*(unsigned char*)src / 16];
            *tmp_dst++ = hex_digits[*src & 0xf];
        }
        src++;
    }

    *tmp_dst = 0;

    return dst;
}

static WCHAR *convert_file_uri(IUri *uri)
{
    UNICODE_STRING nt_name;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status;
    ULONG size = 256;
    char *buffer;
    WCHAR *new_path = NULL;
    BSTR filename;
    HRESULT hres;

    hres = IUri_GetPath(uri, &filename);
    if(FAILED(hres))
        return NULL;

    WINE_TRACE("Windows path: %s\n", wine_dbgstr_w(filename));

    if (!RtlDosPathNameToNtPathName_U( filename, &nt_name, NULL, NULL )) return NULL;
    InitializeObjectAttributes( &attr, &nt_name, 0, 0, NULL );
    for (;;)
    {
        if (!(buffer = malloc( size )))
        {
            RtlFreeUnicodeString( &nt_name );
            return NULL;
        }
        status = wine_nt_to_unix_file_name( &attr, buffer, &size, FILE_OPEN );
        if (status != STATUS_BUFFER_TOO_SMALL) break;
        free( buffer );
    }

    if (!status) new_path = encode_unix_path( buffer );
    SysFreeString(filename);
    free( buffer );

    WINE_TRACE("New path: %s\n", wine_dbgstr_w(new_path));

    return new_path;
}

/*****************************************************************************
 * Main entry point. This is a console application so we have a wmain() not a
 * winmain().
 */
int wmain(int argc, WCHAR *argv[])
{
    WCHAR *url = argv[1];
    BSTR display_uri = NULL;
    DWORD scheme;
    IUri *uri;
    HRESULT hres;

    /* DDE used only if -nohome is specified; avoids delay in printing usage info
     * when no parameters are passed */
    if (url && !wcsicmp( url, L"-nohome" ))
        url = argc > 2 ? argv[2] : get_url_from_dde();

    if (!url) {
        WINE_ERR( "Usage: winebrowser URL\n" );
        return -1;
    }

    hres = CreateUri(url, Uri_CREATE_ALLOW_IMPLICIT_FILE_SCHEME|Uri_CREATE_FILE_USE_DOS_PATH, 0, &uri);
    if(FAILED(hres)) {
        WINE_ERR("Failed to parse URL %s, treating as HTTP\n", wine_dbgstr_w(url));
        return open_invalid_url(url);
    }

    free(ddeString);
    IUri_GetScheme(uri, &scheme);

    if(scheme == URL_SCHEME_FILE) {
        display_uri = convert_file_uri(uri);
        if(!display_uri) {
            WINE_ERR("Failed to convert file URL to unix path\n");
        }
    }

    if (!display_uri)
        hres = IUri_GetDisplayUri(uri, &display_uri);
    IUri_Release(uri);
    if(FAILED(hres))
        return -1;

    WINE_TRACE("opening %s\n", wine_dbgstr_w(display_uri));

    if(scheme == URL_SCHEME_MAILTO)
        return open_mailto_url(display_uri);
    else
        /* let the browser decide how to handle the given url */
        return open_http_url(display_uri);
}
