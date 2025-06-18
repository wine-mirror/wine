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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "ws2tcpip.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "wininet.h"
#include "winnetwk.h"
#include "wine/debug.h"
#include "winerror.h"
#define NO_SHLWAPI_STREAM
#include "shlwapi.h"
#include "cryptuiapi.h"

#include "internet.h"
#include "resource.h"

#define MAX_STRING_LEN 1024

WINE_DEFAULT_DEBUG_CHANNEL(wininet);

struct WININET_ErrorDlgParams
{
    http_request_t *req;
    HWND       hWnd;
    DWORD      dwError;
    DWORD      dwFlags;
    LPVOID*    lppvData;
};

/***********************************************************************
 *         WININET_GetAuthRealm
 *
 *  Determine the name of the (basic) Authentication realm
 */
static BOOL WININET_GetAuthRealm( HINTERNET hRequest, LPWSTR szBuf, DWORD sz, BOOL proxy )
{
    LPWSTR p, q;
    DWORD index, query;

    if (proxy)
        query = HTTP_QUERY_PROXY_AUTHENTICATE;
    else
        query = HTTP_QUERY_WWW_AUTHENTICATE;

    /* extract the Realm from the response and show it */
    index = 0;
    if( !HttpQueryInfoW( hRequest, query, szBuf, &sz, &index) )
        return FALSE;

    /*
     * FIXME: maybe we should check that we're
     * dealing with 'Basic' Authentication
     */
    p = wcschr( szBuf, ' ' );
    if( !p || wcsncmp( p+1, L"realm=", lstrlenW(L"realm=") ) )
    {
        ERR("response wrong? (%s)\n", debugstr_w(szBuf));
        return FALSE;
    }

    /* remove quotes */
    p += 7;
    if( *p == '"' )
    {
        p++;
        q = wcsrchr( p, '"' );
        if( q )
            *q = 0;
    }
    lstrcpyW( szBuf, p );

    return TRUE;
}

/* These two are not defined in the public headers */
extern DWORD WINAPI WNetCachePassword(LPSTR,WORD,LPSTR,WORD,BYTE,WORD);
extern DWORD WINAPI WNetGetCachedPassword(LPSTR,WORD,LPSTR,LPWORD,BYTE);

/***********************************************************************
 *         WININET_GetSetPassword
 */
static BOOL WININET_GetSetPassword( HWND hdlg, LPCWSTR szServer, 
                                    LPCWSTR szRealm, BOOL bSet )
{
    WCHAR szResource[0x80], szUserPass[0x40];
    LPWSTR p;
    HWND hUserItem, hPassItem;
    DWORD r, dwMagic = 19;
    UINT r_len, u_len;
    WORD sz;

    hUserItem = GetDlgItem( hdlg, IDC_USERNAME );
    hPassItem = GetDlgItem( hdlg, IDC_PASSWORD );

    /* now try fetch the username and password */
    lstrcpyW( szResource, szServer);
    lstrcatW( szResource, L"/");
    lstrcatW( szResource, szRealm);

    /*
     * WNetCachePassword is only concerned with the length
     * of the data stored (which we tell it) and it does
     * not use strlen() internally so we can add WCHAR data
     * instead of ASCII data and get it back the same way.
     */
    if( bSet )
    {
        szUserPass[0] = 0;
        GetWindowTextW( hUserItem, szUserPass, ARRAY_SIZE( szUserPass ) - 1 );
        lstrcatW(szUserPass, L":");
        u_len = lstrlenW( szUserPass );
        GetWindowTextW( hPassItem, szUserPass+u_len, ARRAY_SIZE( szUserPass ) - u_len );

        r_len = (lstrlenW( szResource ) + 1)*sizeof(WCHAR);
        u_len = (lstrlenW( szUserPass ) + 1)*sizeof(WCHAR);
        r = WNetCachePassword( (CHAR*)szResource, r_len,
                               (CHAR*)szUserPass, u_len, dwMagic, 0 );

        return ( r == WN_SUCCESS );
    }

    sz = sizeof szUserPass;
    r_len = (lstrlenW( szResource ) + 1)*sizeof(WCHAR);
    r = WNetGetCachedPassword( (CHAR*)szResource, r_len,
                               (CHAR*)szUserPass, &sz, dwMagic );
    if( r != WN_SUCCESS )
        return FALSE;

    p = wcschr( szUserPass, ':' );
    if( p )
    {
        struct WININET_ErrorDlgParams *params;
        HWND hwnd;

        params = (struct WININET_ErrorDlgParams*)
                 GetWindowLongPtrW( hdlg, GWLP_USERDATA );

        *p = 0;
        SetWindowTextW( hUserItem, szUserPass );

        if (!params->req->clear_auth)
        {
            SetWindowTextW( hPassItem, p+1 );

            hwnd = GetDlgItem( hdlg, IDC_SAVEPASSWORD );
            if (hwnd)
                SendMessageW(hwnd, BM_SETCHECK, BST_CHECKED, 0);
        }
        else
            WININET_GetSetPassword( hdlg, szServer, szRealm, TRUE );
    }

    return TRUE;
}

/***********************************************************************
 *         WININET_SetAuthorization
 */
static BOOL WININET_SetAuthorization( http_request_t *request, LPWSTR username,
                                      LPWSTR password, BOOL proxy )
{
    http_session_t *session = request->session;
    LPWSTR p, q;

    p = wcsdup(username);
    if( !p )
        return FALSE;

    q = wcsdup(password);
    if( !q )
    {
        free(p);
        return FALSE;
    }

    if (proxy)
    {
        appinfo_t *hIC = session->appInfo;

        free(hIC->proxyUsername);
        hIC->proxyUsername = p;

        free(hIC->proxyPassword);
        hIC->proxyPassword = q;
    }
    else
    {
        free(session->userName);
        session->userName = p;

        free(session->password);
        session->password = q;
    }

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
    WCHAR szRealm[0x80];

    if( uMsg == WM_INITDIALOG )
    {
        TRACE("WM_INITDIALOG (%08Ix)\n", lParam);

        /* save the parameter list */
        params = (struct WININET_ErrorDlgParams*) lParam;
        SetWindowLongPtrW( hdlg, GWLP_USERDATA, lParam );

        /* extract the Realm from the proxy response and show it */
        if( WININET_GetAuthRealm( params->req->hdr.hInternet,
                                  szRealm, ARRAY_SIZE( szRealm ), TRUE ) )
        {
            hitem = GetDlgItem( hdlg, IDC_REALM );
            SetWindowTextW( hitem, szRealm );
        }

        hitem = GetDlgItem( hdlg, IDC_PROXY );
        SetWindowTextW( hitem, params->req->session->appInfo->proxy );

        WININET_GetSetPassword( hdlg, params->req->session->appInfo->proxy, szRealm, FALSE );

        return TRUE;
    }

    params = (struct WININET_ErrorDlgParams*)
                 GetWindowLongPtrW( hdlg, GWLP_USERDATA );

    switch( uMsg )
    {
    case WM_COMMAND:
        if( wParam == IDOK )
        {
            WCHAR username[0x20], password[0x20];

            username[0] = 0;
            hitem = GetDlgItem( hdlg, IDC_USERNAME );
            if( hitem )
                GetWindowTextW( hitem, username, ARRAY_SIZE( username ));

            password[0] = 0;
            hitem = GetDlgItem( hdlg, IDC_PASSWORD );
            if( hitem )
                GetWindowTextW( hitem, password, ARRAY_SIZE( password ));

            hitem = GetDlgItem( hdlg, IDC_SAVEPASSWORD );
            if( hitem &&
                SendMessageW( hitem, BM_GETSTATE, 0, 0 ) &&
                WININET_GetAuthRealm( params->req->hdr.hInternet,
                                      szRealm, ARRAY_SIZE( szRealm ), TRUE) )
                WININET_GetSetPassword( hdlg, params->req->session->appInfo->proxy, szRealm, TRUE );
            WININET_SetAuthorization( params->req, username, password, TRUE );

            params->req->clear_auth = TRUE;

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
 *         WININET_PasswordDialog
 */
static INT_PTR WINAPI WININET_PasswordDialog(
    HWND hdlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    HWND hitem;
    struct WININET_ErrorDlgParams *params;
    WCHAR szRealm[0x80];

    if( uMsg == WM_INITDIALOG )
    {
        TRACE("WM_INITDIALOG (%08Ix)\n", lParam);

        /* save the parameter list */
        params = (struct WININET_ErrorDlgParams*) lParam;
        SetWindowLongPtrW( hdlg, GWLP_USERDATA, lParam );

        /* extract the Realm from the response and show it */
        if( WININET_GetAuthRealm( params->req->hdr.hInternet,
                                  szRealm, ARRAY_SIZE( szRealm ), FALSE ) )
        {
            hitem = GetDlgItem( hdlg, IDC_REALM );
            SetWindowTextW( hitem, szRealm );
        }

        hitem = GetDlgItem( hdlg, IDC_SERVER );
        SetWindowTextW( hitem, params->req->session->hostName );

        WININET_GetSetPassword( hdlg, params->req->session->hostName, szRealm, FALSE );

        return TRUE;
    }

    params = (struct WININET_ErrorDlgParams*)
                 GetWindowLongPtrW( hdlg, GWLP_USERDATA );

    switch( uMsg )
    {
    case WM_COMMAND:
        if( wParam == IDOK )
        {
            WCHAR username[0x20], password[0x20];

            username[0] = 0;
            hitem = GetDlgItem( hdlg, IDC_USERNAME );
            if( hitem )
                GetWindowTextW( hitem, username, ARRAY_SIZE( username ));

            password[0] = 0;
            hitem = GetDlgItem( hdlg, IDC_PASSWORD );
            if( hitem )
                GetWindowTextW( hitem, password, ARRAY_SIZE( password ));

            hitem = GetDlgItem( hdlg, IDC_SAVEPASSWORD );
            if( hitem &&
                SendMessageW( hitem, BM_GETSTATE, 0, 0 ) &&
                WININET_GetAuthRealm( params->req->hdr.hInternet,
                                      szRealm, ARRAY_SIZE( szRealm ), FALSE ))
            {
                WININET_GetSetPassword( hdlg, params->req->session->hostName, szRealm, TRUE );
            }
            WININET_SetAuthorization( params->req, username, password, FALSE );
            params->req->clear_auth = TRUE;

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
 *         WININET_InvalidCertificateDialog
 */
static INT_PTR WINAPI WININET_InvalidCertificateDialog(
    HWND hdlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    struct WININET_ErrorDlgParams *params;
    HWND hitem;
    WCHAR buf[1024];

    if( uMsg == WM_INITDIALOG )
    {
        TRACE("WM_INITDIALOG (%08Ix)\n", lParam);

        /* save the parameter list */
        params = (struct WININET_ErrorDlgParams*) lParam;
        SetWindowLongPtrW( hdlg, GWLP_USERDATA, lParam );

        switch( params->dwError )
        {
        case ERROR_INTERNET_INVALID_CA:
            LoadStringW( WININET_hModule, IDS_CERT_CA_INVALID, buf, 1024 );
            break;
        case ERROR_INTERNET_SEC_CERT_DATE_INVALID:
            LoadStringW( WININET_hModule, IDS_CERT_DATE_INVALID, buf, 1024 );
            break;
        case ERROR_INTERNET_SEC_CERT_CN_INVALID:
            LoadStringW( WININET_hModule, IDS_CERT_CN_INVALID, buf, 1024 );
            break;
        case ERROR_INTERNET_SEC_CERT_ERRORS:
            /* FIXME: We should fetch information about the
             * certificate here and show all the relevant errors.
             */
            LoadStringW( WININET_hModule, IDS_CERT_ERRORS, buf, 1024 );
            break;
        default:
            FIXME( "No message for error %ld\n", params->dwError );
            buf[0] = '\0';
        }

        hitem = GetDlgItem( hdlg, IDC_CERT_ERROR );
        SetWindowTextW( hitem, buf );

        return TRUE;
    }

    params = (struct WININET_ErrorDlgParams*)
                 GetWindowLongPtrW( hdlg, GWLP_USERDATA );

    switch( uMsg )
    {
    case WM_COMMAND:
        if( wParam == IDOK )
        {
            if( params->dwFlags & FLAGS_ERROR_UI_FLAGS_CHANGE_OPTIONS )
            {
                http_request_t *req = params->req;
                DWORD flags, size = sizeof(flags);

                InternetQueryOptionW( req->hdr.hInternet, INTERNET_OPTION_SECURITY_FLAGS, &flags, &size );
                switch( params->dwError )
                {
                case ERROR_INTERNET_INVALID_CA:
                    flags |= SECURITY_FLAG_IGNORE_UNKNOWN_CA;
                    break;
                case ERROR_INTERNET_SEC_CERT_DATE_INVALID:
                    flags |= SECURITY_FLAG_IGNORE_CERT_DATE_INVALID;
                    break;
                case ERROR_INTERNET_SEC_CERT_CN_INVALID:
                    flags |= SECURITY_FLAG_IGNORE_CERT_CN_INVALID;
                    break;
                case ERROR_INTERNET_SEC_CERT_REV_FAILED:
                    flags |= SECURITY_FLAG_IGNORE_REVOCATION;
                    break;
                case ERROR_INTERNET_SEC_CERT_ERRORS:
                    if(flags & _SECURITY_FLAG_CERT_REV_FAILED)
                        flags |= SECURITY_FLAG_IGNORE_REVOCATION;
                    if(flags & _SECURITY_FLAG_CERT_INVALID_CA)
                        flags |= SECURITY_FLAG_IGNORE_UNKNOWN_CA;
                    if(flags & _SECURITY_FLAG_CERT_INVALID_CN)
                        flags |= SECURITY_FLAG_IGNORE_CERT_CN_INVALID;
                    if(flags & _SECURITY_FLAG_CERT_INVALID_DATE)
                        flags |= SECURITY_FLAG_IGNORE_CERT_DATE_INVALID;
                    break;
                }
                /* FIXME: Use helper function */
                flags |= SECURITY_FLAG_SECURE;
                req->security_flags |= flags;
                if(is_valid_netconn(req->netconn))
                    req->netconn->security_flags |= flags;
            }

            EndDialog( hdlg, ERROR_SUCCESS );
            return TRUE;
        }
        if( wParam == IDCANCEL )
        {
            TRACE("Pressed cancel.\n");

            EndDialog( hdlg, ERROR_CANCELLED );
            return TRUE;
        }
        break;
    }

    return FALSE;
}

/***********************************************************************
 *         InternetErrorDlg
 */
DWORD WINAPI InternetErrorDlg(HWND hWnd, HINTERNET hRequest,
                 DWORD dwError, DWORD dwFlags, LPVOID* lppvData)
{
    struct WININET_ErrorDlgParams params;
    http_request_t *req = NULL;
    DWORD res = ERROR_SUCCESS;

    TRACE("%p %p %ld %08lx %p\n", hWnd, hRequest, dwError, dwFlags, lppvData);

    if( !hWnd && !(dwFlags & FLAGS_ERROR_UI_FLAGS_NO_UI) )
        return ERROR_INVALID_HANDLE;

    if(hRequest) {
        req = (http_request_t*)get_handle_object(hRequest);
        if(!req)
            return ERROR_INVALID_HANDLE;
        if(req->hdr.htype != WH_HHTTPREQ)
            return ERROR_SUCCESS; /* Yes, that was tested */
    }

    params.req = req;
    params.hWnd = hWnd;
    params.dwError = dwError;
    params.dwFlags = dwFlags;
    params.lppvData = lppvData;

    switch( dwError )
    {
    case ERROR_SUCCESS:
    case ERROR_INTERNET_INCORRECT_PASSWORD: {
        if( !dwError && !(dwFlags & FLAGS_ERROR_UI_FILTER_FOR_ERRORS ) )
            break;
        if(!req)
            return ERROR_INVALID_PARAMETER;

        switch(req->status_code) {
        case HTTP_STATUS_PROXY_AUTH_REQ:
            res = DialogBoxParamW( WININET_hModule, MAKEINTRESOURCEW( IDD_PROXYDLG ),
                                   hWnd, WININET_ProxyPasswordDialog, (LPARAM) &params );
            break;
        case HTTP_STATUS_DENIED:
            res = DialogBoxParamW( WININET_hModule, MAKEINTRESOURCEW( IDD_AUTHDLG ),
                                    hWnd, WININET_PasswordDialog, (LPARAM) &params );
            break;
        case HTTP_STATUS_OK:
            req->clear_auth = FALSE;
            break;
        default:
            WARN("unhandled status %lu\n", req->status_code);
        }
        break;
    }

    case ERROR_INTERNET_CLIENT_AUTH_CERT_NEEDED:
        if(!req)
            return ERROR_INVALID_PARAMETER;
        /* fall through */
    case ERROR_INTERNET_SEC_CERT_ERRORS:
    case ERROR_INTERNET_SEC_CERT_CN_INVALID:
    case ERROR_INTERNET_SEC_CERT_DATE_INVALID:
    case ERROR_INTERNET_INVALID_CA:
    case ERROR_INTERNET_SEC_CERT_REV_FAILED:
    case ERROR_INTERNET_SEC_CERT_WEAK_SIGNATURE:
        if( dwFlags & FLAGS_ERROR_UI_FLAGS_NO_UI ) {
            res = ERROR_CANCELLED;
            break;
        }

        if( dwFlags & ~FLAGS_ERROR_UI_FLAGS_CHANGE_OPTIONS )
            FIXME("%08lx contains unsupported flags.\n", dwFlags);

        res = DialogBoxParamW( WININET_hModule, MAKEINTRESOURCEW( IDD_INVCERTDLG ),
                               hWnd, WININET_InvalidCertificateDialog, (LPARAM) &params );
        break;

    case ERROR_HTTP_COOKIE_NEEDS_CONFIRMATION:
        if(dwFlags & FLAGS_ERROR_UI_FLAGS_NO_UI) {
            res = ERROR_HTTP_COOKIE_DECLINED;
            break;
        }
        FIXME("Need to display dialog for error %ld\n", dwError);
        res = ERROR_CANCELLED;
        break;

    case ERROR_INTERNET_INSERT_CDROM:
        if(!req)
            return ERROR_INVALID_PARAMETER;
        /* fall through */
    case ERROR_HTTP_REDIRECT_NEEDS_CONFIRMATION:
    case ERROR_INTERNET_BAD_AUTO_PROXY_SCRIPT:
    case ERROR_INTERNET_UNABLE_TO_DOWNLOAD_SCRIPT:
    case ERROR_INTERNET_MIXED_SECURITY:
    case ERROR_INTERNET_HTTPS_HTTP_SUBMIT_REDIR:
        if(!(dwFlags & FLAGS_ERROR_UI_FLAGS_NO_UI))
            FIXME("Need to display dialog for error %ld\n", dwError);
        res = ERROR_CANCELLED;
        break;

    case ERROR_INTERNET_HTTPS_TO_HTTP_ON_REDIR:
    case ERROR_INTERNET_CHG_POST_IS_NON_SECURE:
        if(dwFlags & FLAGS_ERROR_UI_FLAGS_NO_UI) {
            res = ERROR_SUCCESS;
            break;
        }
        FIXME("Need to display dialog for error %ld\n", dwError);
        res = ERROR_CANCELLED;
        break;

    case ERROR_INTERNET_HTTP_TO_HTTPS_ON_REDIR:
    case ERROR_INTERNET_POST_IS_NON_SECURE:
        if (!(dwFlags & FLAGS_ERROR_UI_FLAGS_NO_UI))
            FIXME("Need to display dialog for error %ld\n", dwError);
        res = ERROR_SUCCESS;
        break;

    default:
        if(!(dwFlags & FLAGS_ERROR_UI_FILTER_FOR_ERRORS))
            res = ERROR_CANCELLED;
        break;
    }

    if(req)
        WININET_Release(&req->hdr);
    return res;
}

/***********************************************************************
 *           InternetShowSecurityInfoByURLA (@)
 */
BOOL WINAPI InternetShowSecurityInfoByURLA(LPCSTR url, HWND window)
{
   FIXME("stub: %s %p\n", url, window);
   return FALSE;
}

/***********************************************************************
 *           InternetShowSecurityInfoByURLW (@)
 */
BOOL WINAPI InternetShowSecurityInfoByURLW(LPCWSTR url, HWND window)
{
   FIXME("stub: %s %p\n", debugstr_w(url), window);
   return FALSE;
}

/***********************************************************************
 *           ParseX509EncodedCertificateForListBoxEntry (@)
 */
DWORD WINAPI ParseX509EncodedCertificateForListBoxEntry(LPBYTE cert, DWORD len, LPSTR szlistbox, LPDWORD listbox)
{
   FIXME("stub: %p %ld %s %p\n", cert, len, debugstr_a(szlistbox), listbox);
   return ERROR_CALL_NOT_IMPLEMENTED;
}

/***********************************************************************
 *           ShowX509EncodedCertificate (@)
 */
DWORD WINAPI ShowX509EncodedCertificate(HWND parent, LPBYTE cert, DWORD len)
{
    PCCERT_CONTEXT certContext = CertCreateCertificateContext(X509_ASN_ENCODING,
        cert, len);
    DWORD ret;

    if (certContext)
    {
        CRYPTUI_VIEWCERTIFICATE_STRUCTW view;

        memset(&view, 0, sizeof(view));
        view.hwndParent = parent;
        view.pCertContext = certContext;
        if (CryptUIDlgViewCertificateW(&view, NULL))
            ret = ERROR_SUCCESS;
        else
            ret = GetLastError();
        CertFreeCertificateContext(certContext);
    }
    else
        ret = GetLastError();
    return ret;
}
