/*
 * Wininet
 *
 * Copyright 2003 Mike McCormack for CodeWeavers Inc.
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "wininet.h"
#include "winnetwk.h"
#include "winnls.h"
#include "wine/debug.h"
#include "winerror.h"
#define NO_SHLWAPI_STREAM
#include "shlwapi.h"

#include "internet.h"

#include "wine/unicode.h"

#include "resource.h"

WINE_DEFAULT_DEBUG_CHANNEL(wininet);

struct WININET_ErrorDlgParams
{
    HWND       hWnd;
    HINTERNET  hRequest;
    DWORD      dwError;
    DWORD      dwFlags;
    LPVOID*    lppvData;
};

/***********************************************************************
 *         WININET_GetProxyServer
 *
 *  Determine the name of the proxy server the request is using
 */
static BOOL WININET_GetProxyServer( HINTERNET hRequest, LPSTR szBuf, DWORD sz )
{
    LPWININETHTTPREQA lpwhr = (LPWININETHTTPREQA) hRequest;
    LPWININETHTTPSESSIONA lpwhs = NULL;
    LPWININETAPPINFOA hIC = NULL;
    LPSTR p;

    if (NULL == lpwhr)
	return FALSE;

    lpwhs = (LPWININETHTTPSESSIONA) lpwhr->hdr.lpwhparent;
    if (NULL == lpwhs)
	return FALSE;

    hIC = (LPWININETAPPINFOA) lpwhs->hdr.lpwhparent;
    if (NULL == hIC)
	return FALSE;

    strncpy(szBuf, hIC->lpszProxy, sz);

    /* FIXME: perhaps it would be better to use InternetCrackUrl here */
    p = strchr(szBuf, ':');
    if(*p)
        *p = 0;

    return TRUE;
}

/***********************************************************************
 *         WININET_GetAuthRealm
 *
 *  Determine the name of the (basic) Authentication realm
 */
static BOOL WININET_GetAuthRealm( HINTERNET hRequest, LPSTR szBuf, DWORD sz )
{
    LPSTR p, q;
    DWORD index;

    /* extract the Realm from the proxy response and show it */
    index = 0;
    if( !HttpQueryInfoA( hRequest, HTTP_QUERY_PROXY_AUTHENTICATE,
                         szBuf, &sz, &index) )
        return FALSE;

    /*
     * FIXME: maybe we should check that we're
     * dealing with 'Basic' Authentication
     */
    p = strchr( szBuf, ' ' );
    if( p && !strncmp( p+1, "realm=", 6 ) )
    {
        /* remove quotes */
        p += 7;
        if( *p == '"' )
        {
            p++;
            q = strrchr( p, '"' );
            if( q )
                *q = 0;
        }
    }

    strcpy( szBuf, p );

    return TRUE;
}

/***********************************************************************
 *         WININET_GetSetPassword
 */
static BOOL WININET_GetSetPassword( HWND hdlg, LPCSTR szServer, 
                                    LPCSTR szRealm, BOOL bSet )
{
    CHAR szResource[0x80], szUserPass[0x40];
    LPSTR p;
    HWND hUserItem, hPassItem;
    DWORD r, dwMagic = 19;
    UINT len;
    WORD sz;

    hUserItem = GetDlgItem( hdlg, IDC_USERNAME );
    hPassItem = GetDlgItem( hdlg, IDC_PASSWORD );

    /* now try fetch the username and password */
    strcpy( szResource, szServer);
    strcat( szResource, "/");
    strcat( szResource, szRealm);

    if( bSet )
    {
        szUserPass[0] = 0;
        GetWindowTextA( hUserItem, szUserPass, sizeof szUserPass-1 );
        strcat(szUserPass, ":");
        len = strlen( szUserPass );
        GetWindowTextA( hPassItem, szUserPass+len, sizeof szUserPass-len );

        r = WNetCachePassword( szResource, strlen( szResource ) + 1,
                            szUserPass, strlen( szUserPass ) + 1, dwMagic, 0 );

        return ( r == WN_SUCCESS );
    }

    sz = sizeof szUserPass;
    r = WNetGetCachedPassword( szResource, strlen( szResource ) + 1,
                               szUserPass, &sz, dwMagic );
    if( r != WN_SUCCESS )
        return FALSE;

    p = strchr( szUserPass, ':' );
    if( p )
    {
        *p = 0;
        SetWindowTextA( hUserItem, szUserPass );
        SetWindowTextA( hPassItem, p+1 );
    }

    return TRUE;
}

/***********************************************************************
 *         WININET_SetProxyAuthorization
 */
static BOOL WININET_SetProxyAuthorization( HINTERNET hRequest,
                                         LPSTR username, LPSTR password )
{
    LPWININETHTTPREQA lpwhr = (LPWININETHTTPREQA) hRequest;
    LPWININETHTTPSESSIONA lpwhs;
    LPWININETAPPINFOA hIC;
    LPSTR p;

    lpwhs = (LPWININETHTTPSESSIONA) lpwhr->hdr.lpwhparent;
    if (NULL == lpwhs ||  lpwhs->hdr.htype != WH_HHTTPSESSION)
    {
        INTERNET_SetLastError(ERROR_INTERNET_INCORRECT_HANDLE_TYPE);
	return FALSE;
    }

    hIC = (LPWININETAPPINFOA) lpwhs->hdr.lpwhparent;

    p = HeapAlloc( GetProcessHeap(), 0, strlen( username ) + 1 );
    if( !p )
        return FALSE;
    
    strcpy( p, username );
    hIC->lpszProxyUsername = p;

    p = HeapAlloc( GetProcessHeap(), 0, strlen( password ) + 1 );
    if( !p )
        return FALSE;
    
    strcpy( p, password );
    hIC->lpszProxyPassword = p;

    return TRUE;
}

/***********************************************************************
 *         WININET_ProxyPasswordDialog
 */
static INT_PTR WINAPI WININET_ProxyPasswordDialog(
    HWND hdlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    HWND hitem;
    struct WININET_ErrorDlgParams *params;
    CHAR szRealm[0x80], szServer[0x80];

    if( uMsg == WM_INITDIALOG )
    {
        TRACE("WM_INITDIALOG (%08lx)\n", lParam);

        /* save the parameter list */
        params = (struct WININET_ErrorDlgParams*) lParam;
        SetWindowLongW( hdlg, GWL_USERDATA, lParam );

        /* extract the Realm from the proxy response and show it */
        if( WININET_GetAuthRealm( params->hRequest,
                                  szRealm, sizeof szRealm) )
        {
            hitem = GetDlgItem( hdlg, IDC_REALM );
            SetWindowTextA( hitem, szRealm );
        }

        /* extract the name of the proxy server */
        if( WININET_GetProxyServer( params->hRequest, 
                                    szServer, sizeof szServer) )
        {
            hitem = GetDlgItem( hdlg, IDC_PROXY );
            SetWindowTextA( hitem, szServer );
        }

        WININET_GetSetPassword( hdlg, szServer, szRealm, FALSE );

        return TRUE;
    }

    params = (struct WININET_ErrorDlgParams*)
                 GetWindowLongW( hdlg, GWL_USERDATA );

    switch( uMsg )
    {
    case WM_COMMAND:
        if( wParam == IDOK )
        {
            LPWININETHTTPREQA lpwhr = (LPWININETHTTPREQA) params->hRequest;
            CHAR username[0x20], password[0x20];

            username[0] = 0;
            hitem = GetDlgItem( hdlg, IDC_USERNAME );
            if( hitem )
                GetWindowTextA( hitem, username, sizeof username );
            
            password[0] = 0;
            hitem = GetDlgItem( hdlg, IDC_PASSWORD );
            if( hitem )
                GetWindowTextA( hitem, password, sizeof password );

            hitem = GetDlgItem( hdlg, IDC_SAVEPASSWORD );
            if( hitem &&
                SendMessageA( hitem, BM_GETSTATE, 0, 0 ) &&
                WININET_GetAuthRealm( params->hRequest,
                                  szRealm, sizeof szRealm) &&
                WININET_GetProxyServer( params->hRequest, 
                                    szServer, sizeof szServer) )
            {
                WININET_GetSetPassword( hdlg, szServer, szRealm, TRUE );
            }
            WININET_SetProxyAuthorization( lpwhr, username, password );

            EndDialog( hdlg, ERROR_INTERNET_FORCE_RETRY );
            return TRUE;
        }
        if( wParam == IDCANCEL )
        {
            EndDialog( hdlg, 0 );
            return TRUE;
        }
        break;
    }
    return FALSE;
}

/***********************************************************************
 *         WININET_GetConnectionStatus
 */
static INT WININET_GetConnectionStatus( HINTERNET hRequest )
{
    CHAR szStatus[0x20];
    DWORD sz, index, dwStatus;

    TRACE("%p\n", hRequest );

    sz = sizeof szStatus;
    index = 0;
    if( !HttpQueryInfoA( hRequest, HTTP_QUERY_STATUS_CODE,
                    szStatus, &sz, &index))
        return -1;
    dwStatus = atoi( szStatus );

    TRACE("request %p status = %ld\n", hRequest, dwStatus );

    return dwStatus;
}


/***********************************************************************
 *         InternetErrorDlg
 */
DWORD WINAPI InternetErrorDlg(HWND hWnd, HINTERNET hRequest,
                 DWORD dwError, DWORD dwFlags, LPVOID* lppvData)
{
    struct WININET_ErrorDlgParams params;
    HMODULE hwininet = GetModuleHandleA( "wininet.dll" );
    INT dwStatus;

    TRACE("%p %p %ld %08lx %p\n", hWnd, hRequest, dwError, dwFlags, lppvData);

    params.hWnd = hWnd;
    params.hRequest = hRequest;
    params.dwError = dwError;
    params.dwFlags = dwFlags;
    params.lppvData = lppvData;

    switch( dwError )
    {
    case ERROR_SUCCESS:
        if( !(dwFlags & FLAGS_ERROR_UI_FILTER_FOR_ERRORS ) )
            return 0;
        dwStatus = WININET_GetConnectionStatus( hRequest );
        if( HTTP_STATUS_PROXY_AUTH_REQ != dwStatus )
            return ERROR_SUCCESS;
        return DialogBoxParamW( hwininet, MAKEINTRESOURCEW( IDD_PROXYDLG ),
                    hWnd, WININET_ProxyPasswordDialog, (LPARAM) &params );

    case ERROR_INTERNET_INCORRECT_PASSWORD:
        return DialogBoxParamW( hwininet, MAKEINTRESOURCEW( IDD_PROXYDLG ),
                    hWnd, WININET_ProxyPasswordDialog, (LPARAM) &params );

    case ERROR_INTERNET_HTTP_TO_HTTPS_ON_REDIR:
    case ERROR_INTERNET_INVALID_CA:
    case ERROR_INTERNET_POST_IS_NON_SECURE:
    case ERROR_INTERNET_SEC_CERT_CN_INVALID:
    case ERROR_INTERNET_SEC_CERT_DATE_INVALID:
        FIXME("Need to display dialog for error %ld\n", dwError);
        return ERROR_SUCCESS;
    }
    return ERROR_INVALID_PARAMETER;
}
