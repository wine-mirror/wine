/*
 * Wininet - Http Implementation
 *
 * Copyright 1999 Corel Corporation
 * Copyright 2002 CodeWeavers Inc.
 * Copyright 2002 TransGaming Technologies Inc.
 * Copyright 2004 Mike McCormack for CodeWeavers
 *
 * Ulrich Czekalla
 * Aric Stewart
 * David Hammerton
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

#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <errno.h>
#include <string.h>
#include <time.h>

#include "windef.h"
#include "winbase.h"
#include "wininet.h"
#include "winreg.h"
#include "winerror.h"
#define NO_SHLWAPI_STREAM
#include "shlwapi.h"

#include "internet.h"
#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(wininet);

const WCHAR g_szHttp[] = {' ','H','T','T','P','/','1','.','0',0 };
const WCHAR g_szHost[] = {'\r','\n','H','o','s','t',':',' ',0 };
const WCHAR g_szReferer[] = {'R','e','f','e','r','e','r',0};
const WCHAR g_szAccept[] = {'A','c','c','e','p','t',0};
const WCHAR g_szUserAgent[] = {'U','s','e','r','-','A','g','e','n','t',0};


#define HTTPHEADER g_szHttp
#define HTTPHOSTHEADER g_szHost
#define MAXHOSTNAME 100
#define MAX_FIELD_VALUE_LEN 256
#define MAX_FIELD_LEN 256

#define HTTP_REFERER    g_szReferer
#define HTTP_ACCEPT     g_szAccept
#define HTTP_USERAGENT  g_szUserAgent

#define HTTP_ADDHDR_FLAG_ADD				0x20000000
#define HTTP_ADDHDR_FLAG_ADD_IF_NEW			0x10000000
#define HTTP_ADDHDR_FLAG_COALESCE			0x40000000
#define HTTP_ADDHDR_FLAG_COALESCE_WITH_COMMA		0x40000000
#define HTTP_ADDHDR_FLAG_COALESCE_WITH_SEMICOLON	0x01000000
#define HTTP_ADDHDR_FLAG_REPLACE			0x80000000
#define HTTP_ADDHDR_FLAG_REQ				0x02000000


BOOL HTTP_OpenConnection(LPWININETHTTPREQW lpwhr);
int HTTP_WriteDataToStream(LPWININETHTTPREQW lpwhr,
	void *Buffer, int BytesToWrite);
int HTTP_ReadDataFromStream(LPWININETHTTPREQW lpwhr,
	void *Buffer, int BytesToRead);
BOOL HTTP_GetResponseHeaders(LPWININETHTTPREQW lpwhr);
BOOL HTTP_ProcessHeader(LPWININETHTTPREQW lpwhr, LPCWSTR field, LPCWSTR value, DWORD dwModifier);
void HTTP_CloseConnection(LPWININETHTTPREQW lpwhr);
BOOL HTTP_InterpretHttpHeader(LPWSTR buffer, LPWSTR field, INT fieldlen, LPWSTR value, INT valuelen);
INT HTTP_GetStdHeaderIndex(LPCWSTR lpszField);
BOOL HTTP_InsertCustomHeader(LPWININETHTTPREQW lpwhr, LPHTTPHEADERW lpHdr);
INT HTTP_GetCustomHeaderIndex(LPWININETHTTPREQW lpwhr, LPCWSTR lpszField);
BOOL HTTP_DeleteCustomHeader(LPWININETHTTPREQW lpwhr, INT index);

/***********************************************************************
 *           HttpAddRequestHeadersW (WININET.@)
 *
 * Adds one or more HTTP header to the request handler
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI HttpAddRequestHeadersW(HINTERNET hHttpRequest,
	LPCWSTR lpszHeader, DWORD dwHeaderLength, DWORD dwModifier)
{
    LPWSTR lpszStart;
    LPWSTR lpszEnd;
    LPWSTR buffer;
    WCHAR value[MAX_FIELD_VALUE_LEN], field[MAX_FIELD_LEN];
    BOOL bSuccess = FALSE;
    LPWININETHTTPREQW lpwhr;

    TRACE("%p, %s, %li, %li\n", hHttpRequest, debugstr_w(lpszHeader), dwHeaderLength,
          dwModifier);

    lpwhr = (LPWININETHTTPREQW) WININET_GetObject( hHttpRequest );

    if (NULL == lpwhr ||  lpwhr->hdr.htype != WH_HHTTPREQ)
    {
        INTERNET_SetLastError(ERROR_INTERNET_INCORRECT_HANDLE_TYPE);
        return FALSE;
    }

    if (!lpszHeader) 
      return TRUE;

    TRACE("copying header: %s\n", debugstr_w(lpszHeader));
    buffer = WININET_strdupW(lpszHeader);
    lpszStart = buffer;

    do
    {
        lpszEnd = lpszStart;

        while (*lpszEnd != '\0')
        {
            if (*lpszEnd == '\r' && *(lpszEnd + 1) == '\n')
                 break;
            lpszEnd++;
        }

        if (*lpszEnd == '\0')
	    break;

        *lpszEnd = '\0';

        TRACE("interpreting header %s\n", debugstr_w(lpszStart));
        if (HTTP_InterpretHttpHeader(lpszStart, field, MAX_FIELD_LEN, value, MAX_FIELD_VALUE_LEN))
            bSuccess = HTTP_ProcessHeader(lpwhr, field, value, dwModifier | HTTP_ADDHDR_FLAG_REQ);

        lpszStart = lpszEnd + 2; /* Jump over \0\n */

    } while (bSuccess);

    HeapFree(GetProcessHeap(), 0, buffer);
    return bSuccess;
}

/***********************************************************************
 *           HttpAddRequestHeadersA (WININET.@)
 *
 * Adds one or more HTTP header to the request handler
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI HttpAddRequestHeadersA(HINTERNET hHttpRequest,
	LPCSTR lpszHeader, DWORD dwHeaderLength, DWORD dwModifier)
{
    DWORD len;
    LPWSTR hdr;
    BOOL r;

    TRACE("%p, %s, %li, %li\n", hHttpRequest, debugstr_a(lpszHeader), dwHeaderLength,
          dwModifier);

    len = MultiByteToWideChar( CP_ACP, 0, lpszHeader, dwHeaderLength, NULL, 0 );
    hdr = HeapAlloc( GetProcessHeap(), 0, len*sizeof(WCHAR) );
    MultiByteToWideChar( CP_ACP, 0, lpszHeader, dwHeaderLength, hdr, len );
    if( dwHeaderLength != -1 )
        dwHeaderLength = len;

    r = HttpAddRequestHeadersW( hHttpRequest, hdr, dwHeaderLength, dwModifier );

    HeapFree( GetProcessHeap(), 0, hdr );

    return r;
}

/***********************************************************************
 *           HttpEndRequestA (WININET.@)
 *
 * Ends an HTTP request that was started by HttpSendRequestEx
 *
 * RETURNS
 *    TRUE	if successful
 *    FALSE	on failure
 *
 */
BOOL WINAPI HttpEndRequestA(HINTERNET hRequest, LPINTERNET_BUFFERSA lpBuffersOut, 
          DWORD dwFlags, DWORD dwContext)
{
  FIXME("stub\n");
  return FALSE;
}

/***********************************************************************
 *           HttpEndRequestW (WININET.@)
 *
 * Ends an HTTP request that was started by HttpSendRequestEx
 *
 * RETURNS
 *    TRUE	if successful
 *    FALSE	on failure
 *
 */
BOOL WINAPI HttpEndRequestW(HINTERNET hRequest, LPINTERNET_BUFFERSW lpBuffersOut, 
          DWORD dwFlags, DWORD dwContext)
{
  FIXME("stub\n");
  return FALSE;
}

/***********************************************************************
 *           HttpOpenRequestW (WININET.@)
 *
 * Open a HTTP request handle
 *
 * RETURNS
 *    HINTERNET  a HTTP request handle on success
 *    NULL 	 on failure
 *
 */
HINTERNET WINAPI HttpOpenRequestW(HINTERNET hHttpSession,
	LPCWSTR lpszVerb, LPCWSTR lpszObjectName, LPCWSTR lpszVersion,
	LPCWSTR lpszReferrer , LPCWSTR *lpszAcceptTypes,
	DWORD dwFlags, DWORD dwContext)
{
    LPWININETHTTPSESSIONW lpwhs;
    LPWININETAPPINFOW hIC = NULL;
    HINTERNET handle = NULL;

    TRACE("(%p, %s, %s, %s, %s, %p, %08lx, %08lx)\n", hHttpSession,
          debugstr_w(lpszVerb), debugstr_w(lpszObjectName),
          debugstr_w(lpszVersion), debugstr_w(lpszReferrer), lpszAcceptTypes,
          dwFlags, dwContext);
    if(lpszAcceptTypes!=NULL)
    {
        int i;
        for(i=0;lpszAcceptTypes[i]!=NULL;i++)
            TRACE("\taccept type: %s\n",debugstr_w(lpszAcceptTypes[i]));
    }    

    lpwhs = (LPWININETHTTPSESSIONW) WININET_GetObject( hHttpSession );
    if (NULL == lpwhs ||  lpwhs->hdr.htype != WH_HHTTPSESSION)
    {
        INTERNET_SetLastError(ERROR_INTERNET_INCORRECT_HANDLE_TYPE);
	return NULL;
    }
    hIC = (LPWININETAPPINFOW) lpwhs->hdr.lpwhparent;

    /*
     * My tests seem to show that the windows version does not
     * become asynchronous until after this point. And anyhow
     * if this call was asynchronous then how would you get the
     * necessary HINTERNET pointer returned by this function.
     *
     * I am leaving this here just in case I am wrong
     *
     * if (hIC->hdr.dwFlags & INTERNET_FLAG_ASYNC)
     */
    if (0)
    {
        WORKREQUEST workRequest;
        struct WORKREQ_HTTPOPENREQUESTW *req;

	workRequest.asyncall = HTTPOPENREQUESTW;
	workRequest.handle = hHttpSession;
        req = &workRequest.u.HttpOpenRequestW;
	req->lpszVerb = WININET_strdupW(lpszVerb);
	req->lpszObjectName = WININET_strdupW(lpszObjectName);
        if (lpszVersion)
            req->lpszVersion = WININET_strdupW(lpszVersion);
        else
            req->lpszVersion = 0;
        if (lpszReferrer)
            req->lpszReferrer = WININET_strdupW(lpszReferrer);
        else
            req->lpszReferrer = 0;
	req->lpszAcceptTypes = lpszAcceptTypes;
	req->dwFlags = dwFlags;
	req->dwContext = dwContext;

        INTERNET_AsyncCall(&workRequest);
    }
    else
    {
        handle = HTTP_HttpOpenRequestW(hHttpSession, lpszVerb, lpszObjectName,
                                              lpszVersion, lpszReferrer, lpszAcceptTypes,
                                              dwFlags, dwContext);
    }
    TRACE("returning %p\n", handle);
    return handle;
}


/***********************************************************************
 *           HttpOpenRequestA (WININET.@)
 *
 * Open a HTTP request handle
 *
 * RETURNS
 *    HINTERNET  a HTTP request handle on success
 *    NULL 	 on failure
 *
 */
HINTERNET WINAPI HttpOpenRequestA(HINTERNET hHttpSession,
	LPCSTR lpszVerb, LPCSTR lpszObjectName, LPCSTR lpszVersion,
	LPCSTR lpszReferrer , LPCSTR *lpszAcceptTypes,
	DWORD dwFlags, DWORD dwContext)
{
    LPWSTR szVerb = NULL, szObjectName = NULL;
    LPWSTR szVersion = NULL, szReferrer = NULL, *szAcceptTypes = NULL;
    INT len;
    INT acceptTypesCount;
    HINTERNET rc = FALSE;
    TRACE("(%p, %s, %s, %s, %s, %p, %08lx, %08lx)\n", hHttpSession,
          debugstr_a(lpszVerb), debugstr_a(lpszObjectName),
          debugstr_a(lpszVersion), debugstr_a(lpszReferrer), lpszAcceptTypes,
          dwFlags, dwContext);

    if (lpszVerb)
    {
        len = MultiByteToWideChar(CP_ACP, 0, lpszVerb, -1, NULL, 0 );
        szVerb = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR) );
        if ( !szVerb )
            goto end;
        MultiByteToWideChar(CP_ACP, 0, lpszVerb, -1, szVerb, len);
    }

    if (lpszObjectName)
    {
        len = MultiByteToWideChar(CP_ACP, 0, lpszObjectName, -1, NULL, 0 );
        szObjectName = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR) );
        if ( !szObjectName )
            goto end;
        MultiByteToWideChar(CP_ACP, 0, lpszObjectName, -1, szObjectName, len );
    }

    if (lpszVersion)
    {
        len = MultiByteToWideChar(CP_ACP, 0, lpszVersion, -1, NULL, 0 );
        szVersion = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if ( !szVersion )
            goto end;
        MultiByteToWideChar(CP_ACP, 0, lpszVersion, -1, szVersion, len );
    }

    if (lpszReferrer)
    {
        len = MultiByteToWideChar(CP_ACP, 0, lpszReferrer, -1, NULL, 0 );
        szReferrer = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if ( !szReferrer )
            goto end;
        MultiByteToWideChar(CP_ACP, 0, lpszReferrer, -1, szReferrer, len );
    }

    acceptTypesCount = 0;
    if (lpszAcceptTypes)
    {
        /* find out how many there are */
        while (lpszAcceptTypes[acceptTypesCount]) 
            acceptTypesCount++;
        szAcceptTypes = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR *) * (acceptTypesCount+1));
        acceptTypesCount = 0;
        while (lpszAcceptTypes[acceptTypesCount])
        {
            len = MultiByteToWideChar(CP_ACP, 0, lpszAcceptTypes[acceptTypesCount],
                                -1, NULL, 0 );
            szAcceptTypes[acceptTypesCount] = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
            if (!szAcceptTypes[acceptTypesCount] )
                goto end;
            MultiByteToWideChar(CP_ACP, 0, lpszAcceptTypes[acceptTypesCount],
                                -1, szAcceptTypes[acceptTypesCount], len );
            acceptTypesCount++;
        }
        szAcceptTypes[acceptTypesCount] = NULL;
    }
    else szAcceptTypes = 0;

    rc = HttpOpenRequestW(hHttpSession, szVerb, szObjectName,
                          szVersion, szReferrer,
                          (LPCWSTR*)szAcceptTypes, dwFlags, dwContext);

end:
    if (szAcceptTypes)
    {
        acceptTypesCount = 0;
        while (szAcceptTypes[acceptTypesCount])
        {
            HeapFree(GetProcessHeap(), 0, szAcceptTypes[acceptTypesCount]);
            acceptTypesCount++;
        }
        HeapFree(GetProcessHeap(), 0, szAcceptTypes);
    }
    if (szReferrer) HeapFree(GetProcessHeap(), 0, szReferrer);
    if (szVersion) HeapFree(GetProcessHeap(), 0, szVersion);
    if (szObjectName) HeapFree(GetProcessHeap(), 0, szObjectName);
    if (szVerb) HeapFree(GetProcessHeap(), 0, szVerb);

    return rc;
}

/***********************************************************************
 *  HTTP_Base64
 */
static UINT HTTP_Base64( LPCWSTR bin, LPWSTR base64 )
{
    UINT n = 0, x;
    static LPSTR HTTP_Base64Enc = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    while( bin[0] )
    {
        /* first 6 bits, all from bin[0] */
        base64[n++] = HTTP_Base64Enc[(bin[0] & 0xfc) >> 2];
        x = (bin[0] & 3) << 4;

        /* next 6 bits, 2 from bin[0] and 4 from bin[1] */
        if( !bin[1] )
        {
            base64[n++] = HTTP_Base64Enc[x];
            base64[n++] = '=';
            base64[n++] = '=';
            break;
        }
        base64[n++] = HTTP_Base64Enc[ x | ( (bin[1]&0xf0) >> 4 ) ];
        x = ( bin[1] & 0x0f ) << 2;

        /* next 6 bits 4 from bin[1] and 2 from bin[2] */
        if( !bin[2] )
        {
            base64[n++] = HTTP_Base64Enc[x];
            base64[n++] = '=';
            break;
        }
        base64[n++] = HTTP_Base64Enc[ x | ( (bin[2]&0xc0 ) >> 6 ) ];

        /* last 6 bits, all from bin [2] */
        base64[n++] = HTTP_Base64Enc[ bin[2] & 0x3f ];
        bin += 3;
    }
    base64[n] = 0;
    return n;
}

/***********************************************************************
 *  HTTP_EncodeBasicAuth
 *
 *  Encode the basic authentication string for HTTP 1.1
 */
static LPWSTR HTTP_EncodeBasicAuth( LPCWSTR username, LPCWSTR password)
{
    UINT len;
    LPWSTR in, out;
    WCHAR szBasic[] = {'B','a','s','i','c',' ',0};
    WCHAR szColon[] = {':',0};

    len = lstrlenW( username ) + 1 + lstrlenW ( password ) + 1;
    in = HeapAlloc( GetProcessHeap(), 0, len*sizeof(WCHAR) );
    if( !in )
        return NULL;

    len = lstrlenW(szBasic) +
          (lstrlenW( username ) + 1 + lstrlenW ( password ))*2 + 1 + 1;
    out = HeapAlloc( GetProcessHeap(), 0, len*sizeof(WCHAR) );
    if( out )
    {
        lstrcpyW( in, username );
        lstrcatW( in, szColon );
        lstrcatW( in, password );
        lstrcpyW( out, szBasic );
        HTTP_Base64( in, &out[strlenW(out)] );
    }
    HeapFree( GetProcessHeap(), 0, in );

    return out;
}

/***********************************************************************
 *  HTTP_InsertProxyAuthorization
 *
 *   Insert the basic authorization field in the request header
 */
BOOL HTTP_InsertProxyAuthorization( LPWININETHTTPREQW lpwhr,
                       LPCWSTR username, LPCWSTR password )
{
    HTTPHEADERW hdr;
    INT index;
    WCHAR szProxyAuthorization[] = {
        'P','r','o','x','y','-','A','u','t','h','o','r','i','z','a','t','i','o','n',0 };

    hdr.lpszValue = HTTP_EncodeBasicAuth( username, password );
    hdr.lpszField = szProxyAuthorization;
    hdr.wFlags = HDR_ISREQUEST;
    hdr.wCount = 0;
    if( !hdr.lpszValue )
        return FALSE;

    TRACE("Inserting %s = %s\n",
          debugstr_w( hdr.lpszField ), debugstr_w( hdr.lpszValue ) );

    /* remove the old proxy authorization header */
    index = HTTP_GetCustomHeaderIndex( lpwhr, hdr.lpszField );
    if( index >=0 )
        HTTP_DeleteCustomHeader( lpwhr, index );
    
    HTTP_InsertCustomHeader(lpwhr, &hdr);
    HeapFree( GetProcessHeap(), 0, hdr.lpszValue );
    
    return TRUE;
}

/***********************************************************************
 *           HTTP_DealWithProxy
 */
static BOOL HTTP_DealWithProxy( LPWININETAPPINFOW hIC,
    LPWININETHTTPSESSIONW lpwhs, LPWININETHTTPREQW lpwhr)
{
    WCHAR buf[MAXHOSTNAME];
    WCHAR proxy[MAXHOSTNAME + 15]; /* 15 == "http://" + sizeof(port#) + ":/\0" */
    WCHAR* url, szNul[] = { 0 };
    URL_COMPONENTSW UrlComponents;
    WCHAR szHttp[] = { 'h','t','t','p',':','/','/',0 }, szSlash[] = { '/',0 } ;
    /*WCHAR szColon[] = { ':',0 }; */
    WCHAR szFormat1[] = { 'h','t','t','p',':','/','/','%','s',':','%','d',0 };
    WCHAR szFormat2[] = { 'h','t','t','p',':','/','/','%','s',':','%','d',0 };
    int len;

    memset( &UrlComponents, 0, sizeof UrlComponents );
    UrlComponents.dwStructSize = sizeof UrlComponents;
    UrlComponents.lpszHostName = buf;
    UrlComponents.dwHostNameLength = MAXHOSTNAME;

    if( CSTR_EQUAL != CompareStringW(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
                                 buf,strlenW(szHttp),szHttp,strlenW(szHttp)) )
        sprintfW(proxy, szFormat1, hIC->lpszProxy);
    else
	strcpyW(proxy,buf);
    if( !InternetCrackUrlW(proxy, 0, 0, &UrlComponents) )
        return FALSE;
    if( UrlComponents.dwHostNameLength == 0 )
        return FALSE;

    if( !lpwhr->lpszPath )
        lpwhr->lpszPath = szNul;
    TRACE("server='%s' path='%s'\n",
          debugstr_w(lpwhs->lpszServerName), debugstr_w(lpwhr->lpszPath));
    /* for constant 15 see above */
    len = strlenW(lpwhs->lpszServerName) + strlenW(lpwhr->lpszPath) + 15;
    url = HeapAlloc(GetProcessHeap(), 0, len*sizeof(WCHAR));

    if(UrlComponents.nPort == INTERNET_INVALID_PORT_NUMBER)
        UrlComponents.nPort = INTERNET_DEFAULT_HTTP_PORT;

    sprintfW(url, szFormat2, lpwhs->lpszServerName, lpwhs->nServerPort);

    if( lpwhr->lpszPath[0] != '/' )
        strcatW( url, szSlash );
    strcatW(url, lpwhr->lpszPath);
    if(lpwhr->lpszPath != szNul)
        HeapFree(GetProcessHeap(), 0, lpwhr->lpszPath);
    lpwhr->lpszPath = url;
    /* FIXME: Do I have to free lpwhs->lpszServerName here ? */
    lpwhs->lpszServerName = WININET_strdupW(UrlComponents.lpszHostName);
    lpwhs->nServerPort = UrlComponents.nPort;

    return TRUE;
}

/***********************************************************************
 *           HTTP_HttpOpenRequestW (internal)
 *
 * Open a HTTP request handle
 *
 * RETURNS
 *    HINTERNET  a HTTP request handle on success
 *    NULL 	 on failure
 *
 */
HINTERNET WINAPI HTTP_HttpOpenRequestW(HINTERNET hHttpSession,
	LPCWSTR lpszVerb, LPCWSTR lpszObjectName, LPCWSTR lpszVersion,
	LPCWSTR lpszReferrer , LPCWSTR *lpszAcceptTypes,
	DWORD dwFlags, DWORD dwContext)
{
    LPWININETHTTPSESSIONW lpwhs;
    LPWININETAPPINFOW hIC = NULL;
    LPWININETHTTPREQW lpwhr;
    LPWSTR lpszCookies;
    LPWSTR lpszUrl = NULL;
    DWORD nCookieSize;
    HINTERNET handle;
    WCHAR szUrlForm[] = {'h','t','t','p',':','/','/','%','s',0};
    DWORD len;

    TRACE("--> \n");

    lpwhs = (LPWININETHTTPSESSIONW) WININET_GetObject( hHttpSession );
    if (NULL == lpwhs ||  lpwhs->hdr.htype != WH_HHTTPSESSION)
    {
        INTERNET_SetLastError(ERROR_INTERNET_INCORRECT_HANDLE_TYPE);
	return NULL;
    }

    hIC = (LPWININETAPPINFOW) lpwhs->hdr.lpwhparent;

    lpwhr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WININETHTTPREQW));
    if (NULL == lpwhr)
    {
        INTERNET_SetLastError(ERROR_OUTOFMEMORY);
        return NULL;
    }
    handle = WININET_AllocHandle( &lpwhr->hdr );
    if (NULL == handle)
    {
        INTERNET_SetLastError(ERROR_OUTOFMEMORY);
        HeapFree( GetProcessHeap(), 0, lpwhr );
        return NULL;
    }

    lpwhr->hdr.htype = WH_HHTTPREQ;
    lpwhr->hdr.lpwhparent = &lpwhs->hdr;
    lpwhr->hdr.dwFlags = dwFlags;
    lpwhr->hdr.dwContext = dwContext;
    NETCON_init(&lpwhr->netConnection, dwFlags & INTERNET_FLAG_SECURE);

    if (NULL != lpszObjectName && strlenW(lpszObjectName)) {
        HRESULT rc;

        len = 0;
        rc = UrlEscapeW(lpszObjectName, NULL, &len, URL_ESCAPE_SPACES_ONLY);
        if (rc != E_POINTER)
            len = strlenW(lpszObjectName)+1;
        lpwhr->lpszPath = HeapAlloc(GetProcessHeap(), 0, len*sizeof(WCHAR));
        rc = UrlEscapeW(lpszObjectName, lpwhr->lpszPath, &len,
                   URL_ESCAPE_SPACES_ONLY);
        if (rc)
        {
            ERR("Unable to escape string!(%s) (%ld)\n",debugstr_w(lpszObjectName),rc);
            strcpyW(lpwhr->lpszPath,lpszObjectName);
        }
    }

    if (NULL != lpszReferrer && strlenW(lpszReferrer))
        HTTP_ProcessHeader(lpwhr, HTTP_REFERER, lpszReferrer, HTTP_ADDHDR_FLAG_COALESCE);

    if(lpszAcceptTypes!=NULL)
    {
        int i;
        for(i=0;lpszAcceptTypes[i]!=NULL;i++)
            HTTP_ProcessHeader(lpwhr, HTTP_ACCEPT, lpszAcceptTypes[i], HTTP_ADDHDR_FLAG_COALESCE_WITH_COMMA|HTTP_ADDHDR_FLAG_REQ|HTTP_ADDHDR_FLAG_ADD_IF_NEW);
    }

    if (NULL == lpszVerb)
    {
        WCHAR szGet[] = {'G','E','T',0};
        lpwhr->lpszVerb = WININET_strdupW(szGet);
    }
    else if (strlenW(lpszVerb))
        lpwhr->lpszVerb = WININET_strdupW(lpszVerb);

    if (NULL != lpszReferrer && strlenW(lpszReferrer))
    {
        WCHAR buf[MAXHOSTNAME];
        URL_COMPONENTSW UrlComponents;

        memset( &UrlComponents, 0, sizeof UrlComponents );
        UrlComponents.dwStructSize = sizeof UrlComponents;
        UrlComponents.lpszHostName = buf;
        UrlComponents.dwHostNameLength = MAXHOSTNAME;

        InternetCrackUrlW(lpszReferrer, 0, 0, &UrlComponents);
        if (strlenW(UrlComponents.lpszHostName))
            lpwhr->lpszHostName = WININET_strdupW(UrlComponents.lpszHostName);
    } else {
        lpwhr->lpszHostName = WININET_strdupW(lpwhs->lpszServerName);
    }
    if (NULL != hIC->lpszProxy && hIC->lpszProxy[0] != 0)
        HTTP_DealWithProxy( hIC, lpwhs, lpwhr );

    if (hIC->lpszAgent)
    {
        WCHAR *agent_header;
        WCHAR user_agent[] = {'U','s','e','r','-','A','g','e','n','t',':',' ','%','s','\r','\n',0 };

        len = strlenW(hIC->lpszAgent) + strlenW(user_agent);
        agent_header = HeapAlloc( GetProcessHeap(), 0, len*sizeof(WCHAR) );
        sprintfW(agent_header, user_agent, hIC->lpszAgent );

        HttpAddRequestHeadersW(handle, agent_header, strlenW(agent_header),
                               HTTP_ADDREQ_FLAG_ADD);
        HeapFree(GetProcessHeap(), 0, agent_header);
    }

    len = strlenW(lpwhr->lpszHostName) + strlenW(szUrlForm);
    lpszUrl = HeapAlloc(GetProcessHeap(), 0, len*sizeof(WCHAR));
    sprintfW( lpszUrl, szUrlForm, lpwhr->lpszHostName );

    if (InternetGetCookieW(lpszUrl, NULL, NULL, &nCookieSize))
    {
        int cnt = 0;
        WCHAR szCookie[] = {'C','o','o','k','i','e',':',' ',0};
        WCHAR szcrlf[] = {'\r','\n',0};

        lpszCookies = HeapAlloc(GetProcessHeap(), 0, (nCookieSize + 1 + 8)*sizeof(WCHAR));

        cnt += sprintfW(lpszCookies, szCookie);
        InternetGetCookieW(lpszUrl, NULL, lpszCookies + cnt, &nCookieSize);
        strcatW(lpszCookies, szcrlf);

        HttpAddRequestHeadersW(handle, lpszCookies, strlenW(lpszCookies),
                               HTTP_ADDREQ_FLAG_ADD);
        HeapFree(GetProcessHeap(), 0, lpszCookies);
    }
    HeapFree(GetProcessHeap(), 0, lpszUrl);



    if (hIC->lpfnStatusCB)
    {
        INTERNET_ASYNC_RESULT iar;

        iar.dwResult = (DWORD)handle;
        iar.dwError = ERROR_SUCCESS;

        SendAsyncCallback(hIC, hHttpSession, dwContext,
                      INTERNET_STATUS_HANDLE_CREATED, &iar,
                      sizeof(INTERNET_ASYNC_RESULT));
    }

    /*
     * A STATUS_REQUEST_COMPLETE is NOT sent here as per my tests on windows
     */

    /*
     * According to my tests. The name is not resolved until a request is Opened
     */
    SendAsyncCallback(hIC, hHttpSession, dwContext,
                      INTERNET_STATUS_RESOLVING_NAME,
                      lpwhs->lpszServerName,
                      strlenW(lpwhs->lpszServerName)+1);
    if (!GetAddress(lpwhs->lpszServerName, lpwhs->nServerPort,
                    &lpwhs->phostent, &lpwhs->socketAddress))
    {
        INTERNET_SetLastError(ERROR_INTERNET_NAME_NOT_RESOLVED);
        return NULL;
    }

    SendAsyncCallback(hIC, hHttpSession, lpwhr->hdr.dwContext,
                      INTERNET_STATUS_NAME_RESOLVED,
                      &(lpwhs->socketAddress),
                      sizeof(struct sockaddr_in));

    TRACE("<-- %p\n", handle);
    return handle;
}


/***********************************************************************
 *           HttpQueryInfoW (WININET.@)
 *
 * Queries for information about an HTTP request
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI HttpQueryInfoW(HINTERNET hHttpRequest, DWORD dwInfoLevel,
	LPVOID lpBuffer, LPDWORD lpdwBufferLength, LPDWORD lpdwIndex)
{
    LPHTTPHEADERW lphttpHdr = NULL;
    BOOL bSuccess = FALSE;
    LPWININETHTTPREQW lpwhr;
    WCHAR szFmt[] = { '%','s',':',' ','%','s','%','s',0 };
    WCHAR szcrlf[] = { '\r','\n',0 };
    WCHAR sznul[] = { 0 };

    if (TRACE_ON(wininet)) {
#define FE(x) { x, #x }
	static const wininet_flag_info query_flags[] = {
	    FE(HTTP_QUERY_MIME_VERSION),
	    FE(HTTP_QUERY_CONTENT_TYPE),
	    FE(HTTP_QUERY_CONTENT_TRANSFER_ENCODING),
	    FE(HTTP_QUERY_CONTENT_ID),
	    FE(HTTP_QUERY_CONTENT_DESCRIPTION),
	    FE(HTTP_QUERY_CONTENT_LENGTH),
	    FE(HTTP_QUERY_CONTENT_LANGUAGE),
	    FE(HTTP_QUERY_ALLOW),
	    FE(HTTP_QUERY_PUBLIC),
	    FE(HTTP_QUERY_DATE),
	    FE(HTTP_QUERY_EXPIRES),
	    FE(HTTP_QUERY_LAST_MODIFIED),
	    FE(HTTP_QUERY_MESSAGE_ID),
	    FE(HTTP_QUERY_URI),
	    FE(HTTP_QUERY_DERIVED_FROM),
	    FE(HTTP_QUERY_COST),
	    FE(HTTP_QUERY_LINK),
	    FE(HTTP_QUERY_PRAGMA),
	    FE(HTTP_QUERY_VERSION),
	    FE(HTTP_QUERY_STATUS_CODE),
	    FE(HTTP_QUERY_STATUS_TEXT),
	    FE(HTTP_QUERY_RAW_HEADERS),
	    FE(HTTP_QUERY_RAW_HEADERS_CRLF),
	    FE(HTTP_QUERY_CONNECTION),
	    FE(HTTP_QUERY_ACCEPT),
	    FE(HTTP_QUERY_ACCEPT_CHARSET),
	    FE(HTTP_QUERY_ACCEPT_ENCODING),
	    FE(HTTP_QUERY_ACCEPT_LANGUAGE),
	    FE(HTTP_QUERY_AUTHORIZATION),
	    FE(HTTP_QUERY_CONTENT_ENCODING),
	    FE(HTTP_QUERY_FORWARDED),
	    FE(HTTP_QUERY_FROM),
	    FE(HTTP_QUERY_IF_MODIFIED_SINCE),
	    FE(HTTP_QUERY_LOCATION),
	    FE(HTTP_QUERY_ORIG_URI),
	    FE(HTTP_QUERY_REFERER),
	    FE(HTTP_QUERY_RETRY_AFTER),
	    FE(HTTP_QUERY_SERVER),
	    FE(HTTP_QUERY_TITLE),
	    FE(HTTP_QUERY_USER_AGENT),
	    FE(HTTP_QUERY_WWW_AUTHENTICATE),
	    FE(HTTP_QUERY_PROXY_AUTHENTICATE),
	    FE(HTTP_QUERY_ACCEPT_RANGES),
	    FE(HTTP_QUERY_SET_COOKIE),
	    FE(HTTP_QUERY_COOKIE),
	    FE(HTTP_QUERY_REQUEST_METHOD),
	    FE(HTTP_QUERY_REFRESH),
	    FE(HTTP_QUERY_CONTENT_DISPOSITION),
	    FE(HTTP_QUERY_AGE),
	    FE(HTTP_QUERY_CACHE_CONTROL),
	    FE(HTTP_QUERY_CONTENT_BASE),
	    FE(HTTP_QUERY_CONTENT_LOCATION),
	    FE(HTTP_QUERY_CONTENT_MD5),
	    FE(HTTP_QUERY_CONTENT_RANGE),
	    FE(HTTP_QUERY_ETAG),
	    FE(HTTP_QUERY_HOST),
	    FE(HTTP_QUERY_IF_MATCH),
	    FE(HTTP_QUERY_IF_NONE_MATCH),
	    FE(HTTP_QUERY_IF_RANGE),
	    FE(HTTP_QUERY_IF_UNMODIFIED_SINCE),
	    FE(HTTP_QUERY_MAX_FORWARDS),
	    FE(HTTP_QUERY_PROXY_AUTHORIZATION),
	    FE(HTTP_QUERY_RANGE),
	    FE(HTTP_QUERY_TRANSFER_ENCODING),
	    FE(HTTP_QUERY_UPGRADE),
	    FE(HTTP_QUERY_VARY),
	    FE(HTTP_QUERY_VIA),
	    FE(HTTP_QUERY_WARNING),
	    FE(HTTP_QUERY_CUSTOM)
	};
	static const wininet_flag_info modifier_flags[] = {
	    FE(HTTP_QUERY_FLAG_REQUEST_HEADERS),
	    FE(HTTP_QUERY_FLAG_SYSTEMTIME),
	    FE(HTTP_QUERY_FLAG_NUMBER),
	    FE(HTTP_QUERY_FLAG_COALESCE)
	};
#undef FE
	DWORD info_mod = dwInfoLevel & HTTP_QUERY_MODIFIER_FLAGS_MASK;
	DWORD info = dwInfoLevel & HTTP_QUERY_HEADER_MASK;
	int i;

	TRACE("(%p, 0x%08lx)--> %ld\n", hHttpRequest, dwInfoLevel, dwInfoLevel);
	TRACE("  Attribute:");
	for (i = 0; i < (sizeof(query_flags) / sizeof(query_flags[0])); i++) {
	    if (query_flags[i].val == info) {
		DPRINTF(" %s", query_flags[i].name);
		break;
	    }
	}
	if (i == (sizeof(query_flags) / sizeof(query_flags[0]))) {
	    DPRINTF(" Unknown (%08lx)", info);
	}

	DPRINTF(" Modifier:");
	for (i = 0; i < (sizeof(modifier_flags) / sizeof(modifier_flags[0])); i++) {
	    if (modifier_flags[i].val & info_mod) {
		DPRINTF(" %s", modifier_flags[i].name);
		info_mod &= ~ modifier_flags[i].val;
	    }
	}
	
	if (info_mod) {
	    DPRINTF(" Unknown (%08lx)", info_mod);
	}
	DPRINTF("\n");
    }
    
    lpwhr = (LPWININETHTTPREQW) WININET_GetObject( hHttpRequest );
    if (NULL == lpwhr ||  lpwhr->hdr.htype != WH_HHTTPREQ)
    {
        INTERNET_SetLastError(ERROR_INTERNET_INCORRECT_HANDLE_TYPE);
	goto lend;
    }

    /* Find requested header structure */
    if ((dwInfoLevel & ~HTTP_QUERY_MODIFIER_FLAGS_MASK) == HTTP_QUERY_CUSTOM)
    {
        INT index = HTTP_GetCustomHeaderIndex(lpwhr, (LPWSTR)lpBuffer);

        if (index < 0)
            goto lend;

        lphttpHdr = &lpwhr->pCustHeaders[index];
    }
    else
    {
        INT index = dwInfoLevel & ~HTTP_QUERY_MODIFIER_FLAGS_MASK;

        if (index == HTTP_QUERY_RAW_HEADERS_CRLF || index == HTTP_QUERY_RAW_HEADERS)
        {
            INT i, delim, size = 0, cnt = 0;

            delim = index == HTTP_QUERY_RAW_HEADERS_CRLF ? 2 : 1;

           /* Calculate length of custom reuqest headers */
           for (i = 0; i < lpwhr->nCustHeaders; i++)
           {
               if ((~lpwhr->pCustHeaders[i].wFlags & HDR_ISREQUEST) && lpwhr->pCustHeaders[i].lpszField &&
		   lpwhr->pCustHeaders[i].lpszValue)
	       {
                  size += strlenW(lpwhr->pCustHeaders[i].lpszField) +
                       strlenW(lpwhr->pCustHeaders[i].lpszValue) + delim + 2;
	       }
           }

           /* Calculate the length of stadard request headers */
           for (i = 0; i <= HTTP_QUERY_MAX; i++)
           {
              if ((~lpwhr->StdHeaders[i].wFlags & HDR_ISREQUEST) && lpwhr->StdHeaders[i].lpszField &&
                   lpwhr->StdHeaders[i].lpszValue)
              {
                 size += strlenW(lpwhr->StdHeaders[i].lpszField) +
                    strlenW(lpwhr->StdHeaders[i].lpszValue) + delim + 2;
              }
           }
           size += delim;

           if (size + 1 > *lpdwBufferLength/sizeof(WCHAR))
           {
              *lpdwBufferLength = (size + 1) * sizeof(WCHAR);
              INTERNET_SetLastError(ERROR_INSUFFICIENT_BUFFER);
              goto lend;
           }

           /* Append standard request heades */
           for (i = 0; i <= HTTP_QUERY_MAX; i++)
           {
               if ((~lpwhr->StdHeaders[i].wFlags & HDR_ISREQUEST) &&
					   lpwhr->StdHeaders[i].lpszField &&
					   lpwhr->StdHeaders[i].lpszValue)
               {
                   cnt += sprintfW((WCHAR*)lpBuffer + cnt, szFmt, 
                            lpwhr->StdHeaders[i].lpszField, lpwhr->StdHeaders[i].lpszValue,
                          index == HTTP_QUERY_RAW_HEADERS_CRLF ? szcrlf : sznul );
               }
            }

            /* Append custom request heades */
            for (i = 0; i < lpwhr->nCustHeaders; i++)
            {
                if ((~lpwhr->pCustHeaders[i].wFlags & HDR_ISREQUEST) &&
						lpwhr->pCustHeaders[i].lpszField &&
						lpwhr->pCustHeaders[i].lpszValue)
                {
                   cnt += sprintfW((WCHAR*)lpBuffer + cnt, szFmt,
                    lpwhr->pCustHeaders[i].lpszField, lpwhr->pCustHeaders[i].lpszValue,
					index == HTTP_QUERY_RAW_HEADERS_CRLF ? szcrlf : sznul);
                }
            }

            strcpyW((WCHAR*)lpBuffer + cnt, index == HTTP_QUERY_RAW_HEADERS_CRLF ? szcrlf : sznul);

            *lpdwBufferLength = (cnt + delim) * sizeof(WCHAR);
            bSuccess = TRUE;
            goto lend;
        }
	else if (index >= 0 && index <= HTTP_QUERY_MAX && lpwhr->StdHeaders[index].lpszValue)
	{
	    lphttpHdr = &lpwhr->StdHeaders[index];
	}
	else
            goto lend;
    }

    /* Ensure header satisifies requested attributes */
    if ((dwInfoLevel & HTTP_QUERY_FLAG_REQUEST_HEADERS) &&
	    (~lphttpHdr->wFlags & HDR_ISREQUEST))
	goto lend;

    /* coalesce value to reuqested type */
    if (dwInfoLevel & HTTP_QUERY_FLAG_NUMBER)
    {
	*(int *)lpBuffer = atoiW(lphttpHdr->lpszValue);
	bSuccess = TRUE;

	TRACE(" returning number : %d\n", *(int *)lpBuffer);
    }
    else if (dwInfoLevel & HTTP_QUERY_FLAG_SYSTEMTIME)
    {
        time_t tmpTime;
        struct tm tmpTM;
        SYSTEMTIME *STHook;

        tmpTime = ConvertTimeString(lphttpHdr->lpszValue);

        tmpTM = *gmtime(&tmpTime);
        STHook = (SYSTEMTIME *) lpBuffer;
        if(STHook==NULL)
            goto lend;

	STHook->wDay = tmpTM.tm_mday;
	STHook->wHour = tmpTM.tm_hour;
	STHook->wMilliseconds = 0;
	STHook->wMinute = tmpTM.tm_min;
	STHook->wDayOfWeek = tmpTM.tm_wday;
	STHook->wMonth = tmpTM.tm_mon + 1;
	STHook->wSecond = tmpTM.tm_sec;
	STHook->wYear = tmpTM.tm_year;
	
	bSuccess = TRUE;
	
	TRACE(" returning time : %04d/%02d/%02d - %d - %02d:%02d:%02d.%02d\n", 
	      STHook->wYear, STHook->wMonth, STHook->wDay, STHook->wDayOfWeek,
	      STHook->wHour, STHook->wMinute, STHook->wSecond, STHook->wMilliseconds);
    }
    else if (dwInfoLevel & HTTP_QUERY_FLAG_COALESCE)
    {
	    if (*lpdwIndex >= lphttpHdr->wCount)
		{
	        INTERNET_SetLastError(ERROR_HTTP_HEADER_NOT_FOUND);
		}
	    else
	    {
	    /* Copy strncpyW(lpBuffer, lphttpHdr[*lpdwIndex], len); */
            (*lpdwIndex)++;
	    }
    }
    else
    {
        INT len = (strlenW(lphttpHdr->lpszValue) + 1) * sizeof(WCHAR);

        if (len > *lpdwBufferLength)
        {
            *lpdwBufferLength = len;
            INTERNET_SetLastError(ERROR_INSUFFICIENT_BUFFER);
            goto lend;
        }

        memcpy(lpBuffer, lphttpHdr->lpszValue, len);
        *lpdwBufferLength = len - sizeof(WCHAR);
        bSuccess = TRUE;

	TRACE(" returning string : '%s'\n", debugstr_w(lpBuffer));
    }

lend:
    TRACE("%d <--\n", bSuccess);
    return bSuccess;
}

/***********************************************************************
 *           HttpQueryInfoA (WININET.@)
 *
 * Queries for information about an HTTP request
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI HttpQueryInfoA(HINTERNET hHttpRequest, DWORD dwInfoLevel,
	LPVOID lpBuffer, LPDWORD lpdwBufferLength, LPDWORD lpdwIndex)
{
    BOOL result;
    DWORD len;
    WCHAR* bufferW;

    if((dwInfoLevel & HTTP_QUERY_FLAG_NUMBER) ||
       (dwInfoLevel & HTTP_QUERY_FLAG_SYSTEMTIME))
    {
        return HttpQueryInfoW( hHttpRequest, dwInfoLevel, lpBuffer,
                               lpdwBufferLength, lpdwIndex );
    }

    len = (*lpdwBufferLength)*sizeof(WCHAR);
    bufferW = HeapAlloc( GetProcessHeap(), 0, len );
    result = HttpQueryInfoW( hHttpRequest, dwInfoLevel, bufferW,
                           &len, lpdwIndex );
    if( result )
    {
        len = WideCharToMultiByte( CP_ACP,0, bufferW, len / sizeof(WCHAR),
                                     lpBuffer, *lpdwBufferLength, NULL, NULL );
        *lpdwBufferLength = len * sizeof(WCHAR);
    }
    HeapFree(GetProcessHeap(), 0, bufferW );

    return result;
}

/***********************************************************************
 *           HttpSendRequestExA (WININET.@)
 *
 * Sends the specified request to the HTTP server and allows chunked
 * transfers
 */
BOOL WINAPI HttpSendRequestExA(HINTERNET hRequest,
			       LPINTERNET_BUFFERSA lpBuffersIn,
			       LPINTERNET_BUFFERSA lpBuffersOut,
			       DWORD dwFlags, DWORD dwContext)
{
  FIXME("(%p, %p, %p, %08lx, %08lx): stub\n", hRequest, lpBuffersIn,
	lpBuffersOut, dwFlags, dwContext);
  return FALSE;
}

/***********************************************************************
 *           HttpSendRequestA (WININET.@)
 *
 * Sends the specified request to the HTTP server
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI HttpSendRequestW(HINTERNET hHttpRequest, LPCWSTR lpszHeaders,
	DWORD dwHeaderLength, LPVOID lpOptional ,DWORD dwOptionalLength)
{
    LPWININETHTTPREQW lpwhr;
    LPWININETHTTPSESSIONW lpwhs = NULL;
    LPWININETAPPINFOW hIC = NULL;

    TRACE("%p, %p (%s), %li, %p, %li)\n", hHttpRequest,
            lpszHeaders, debugstr_w(lpszHeaders), dwHeaderLength, lpOptional, dwOptionalLength);

    lpwhr = (LPWININETHTTPREQW) WININET_GetObject( hHttpRequest );
    if (NULL == lpwhr || lpwhr->hdr.htype != WH_HHTTPREQ)
    {
        INTERNET_SetLastError(ERROR_INTERNET_INCORRECT_HANDLE_TYPE);
	return FALSE;
    }

    lpwhs = (LPWININETHTTPSESSIONW) lpwhr->hdr.lpwhparent;
    if (NULL == lpwhs ||  lpwhs->hdr.htype != WH_HHTTPSESSION)
    {
        INTERNET_SetLastError(ERROR_INTERNET_INCORRECT_HANDLE_TYPE);
	return FALSE;
    }

    hIC = (LPWININETAPPINFOW) lpwhs->hdr.lpwhparent;
    if (NULL == hIC ||  hIC->hdr.htype != WH_HINIT)
    {
        INTERNET_SetLastError(ERROR_INTERNET_INCORRECT_HANDLE_TYPE);
	return FALSE;
    }

    if (hIC->hdr.dwFlags & INTERNET_FLAG_ASYNC)
    {
        WORKREQUEST workRequest;
        struct WORKREQ_HTTPSENDREQUESTW *req;

        workRequest.asyncall = HTTPSENDREQUESTW;
        workRequest.handle = hHttpRequest;
        req = &workRequest.u.HttpSendRequestW;
        if (lpszHeaders)
            req->lpszHeader = WININET_strdupW(lpszHeaders);
        else
            req->lpszHeader = 0;
        req->dwHeaderLength = dwHeaderLength;
        req->lpOptional = lpOptional;
        req->dwOptionalLength = dwOptionalLength;

        INTERNET_AsyncCall(&workRequest);
        /*
         * This is from windows.
         */
        SetLastError(ERROR_IO_PENDING);
        return 0;
    }
    else
    {
	return HTTP_HttpSendRequestW(hHttpRequest, lpszHeaders,
		dwHeaderLength, lpOptional, dwOptionalLength);
    }
}

/***********************************************************************
 *           HttpSendRequestW (WININET.@)
 *
 * Sends the specified request to the HTTP server
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI HttpSendRequestA(HINTERNET hHttpRequest, LPCSTR lpszHeaders,
	DWORD dwHeaderLength, LPVOID lpOptional ,DWORD dwOptionalLength)
{
    BOOL result;
    LPWSTR szHeaders=NULL;
    DWORD nLen=dwHeaderLength;
    if(lpszHeaders!=NULL)
    {
        nLen=MultiByteToWideChar(CP_ACP,0,lpszHeaders,dwHeaderLength,NULL,0);
        szHeaders=HeapAlloc(GetProcessHeap(),0,nLen*sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP,0,lpszHeaders,dwHeaderLength,szHeaders,nLen);
    }
    result=HttpSendRequestW(hHttpRequest, szHeaders, nLen, lpOptional, dwOptionalLength);
    if(szHeaders!=NULL)
        HeapFree(GetProcessHeap(),0,szHeaders);
    return result;
}

/***********************************************************************
 *           HTTP_HandleRedirect (internal)
 */
static BOOL HTTP_HandleRedirect(LPWININETHTTPREQW lpwhr, LPCWSTR lpszUrl, LPCWSTR lpszHeaders,
                                DWORD dwHeaderLength, LPVOID lpOptional, DWORD dwOptionalLength)
{
    LPWININETHTTPSESSIONW lpwhs = (LPWININETHTTPSESSIONW) lpwhr->hdr.lpwhparent;
    LPWININETAPPINFOW hIC = (LPWININETAPPINFOW) lpwhs->hdr.lpwhparent;
    WCHAR path[2048];
    HINTERNET handle;

    if(lpszUrl[0]=='/')
    {
        /* if it's an absolute path, keep the same session info */
        strcpyW(path,lpszUrl);
    }
    else if (NULL != hIC->lpszProxy && hIC->lpszProxy[0] != 0)
    {
        TRACE("Redirect through proxy\n");
        strcpyW(path,lpszUrl);
    }
    else
    {
        URL_COMPONENTSW urlComponents;
        WCHAR protocol[32], hostName[MAXHOSTNAME], userName[1024];
        WCHAR password[1024], extra[1024];
        urlComponents.dwStructSize = sizeof(URL_COMPONENTSW);
        urlComponents.lpszScheme = protocol;
        urlComponents.dwSchemeLength = 32;
        urlComponents.lpszHostName = hostName;
        urlComponents.dwHostNameLength = MAXHOSTNAME;
        urlComponents.lpszUserName = userName;
        urlComponents.dwUserNameLength = 1024;
        urlComponents.lpszPassword = password;
        urlComponents.dwPasswordLength = 1024;
        urlComponents.lpszUrlPath = path;
        urlComponents.dwUrlPathLength = 2048;
        urlComponents.lpszExtraInfo = extra;
        urlComponents.dwExtraInfoLength = 1024;
        if(!InternetCrackUrlW(lpszUrl, strlenW(lpszUrl), 0, &urlComponents))
            return FALSE;
        
        if (urlComponents.nPort == INTERNET_INVALID_PORT_NUMBER)
            urlComponents.nPort = INTERNET_DEFAULT_HTTP_PORT;

#if 0
        /*
         * This upsets redirects to binary files on sourceforge.net 
         * and gives an html page instead of the target file
         * Examination of the HTTP request sent by native wininet.dll
         * reveals that it doesn't send a referrer in that case.
         * Maybe there's a flag that enables this, or maybe a referrer
         * shouldn't be added in case of a redirect.
         */

        /* consider the current host as the referrer */
        if (NULL != lpwhs->lpszServerName && strlenW(lpwhs->lpszServerName))
            HTTP_ProcessHeader(lpwhr, HTTP_REFERER, lpwhs->lpszServerName,
                           HTTP_ADDHDR_FLAG_REQ|HTTP_ADDREQ_FLAG_REPLACE|
                           HTTP_ADDHDR_FLAG_ADD_IF_NEW);
#endif
        
        if (NULL != lpwhs->lpszServerName)
            HeapFree(GetProcessHeap(), 0, lpwhs->lpszServerName);
        lpwhs->lpszServerName = WININET_strdupW(hostName);
        if (NULL != lpwhs->lpszUserName)
            HeapFree(GetProcessHeap(), 0, lpwhs->lpszUserName);
        lpwhs->lpszUserName = WININET_strdupW(userName);
        lpwhs->nServerPort = urlComponents.nPort;

        if (NULL != lpwhr->lpszHostName)
            HeapFree(GetProcessHeap(), 0, lpwhr->lpszHostName);
        lpwhr->lpszHostName=WININET_strdupW(hostName);

        SendAsyncCallback(hIC, lpwhs, lpwhr->hdr.dwContext,
                      INTERNET_STATUS_RESOLVING_NAME,
                      lpwhs->lpszServerName,
                      strlenW(lpwhs->lpszServerName)+1);

        if (!GetAddress(lpwhs->lpszServerName, lpwhs->nServerPort,
                    &lpwhs->phostent, &lpwhs->socketAddress))
        {
            INTERNET_SetLastError(ERROR_INTERNET_NAME_NOT_RESOLVED);
            return FALSE;
        }

        SendAsyncCallback(hIC, lpwhs, lpwhr->hdr.dwContext,
                      INTERNET_STATUS_NAME_RESOLVED,
                      &(lpwhs->socketAddress),
                      sizeof(struct sockaddr_in));

    }

    if(lpwhr->lpszPath)
        HeapFree(GetProcessHeap(), 0, lpwhr->lpszPath);
    lpwhr->lpszPath=NULL;
    if (strlenW(path))
    {
        DWORD needed = 0;
        HRESULT rc;

        rc = UrlEscapeW(path, NULL, &needed, URL_ESCAPE_SPACES_ONLY);
        if (rc != E_POINTER)
            needed = strlenW(path)+1;
        lpwhr->lpszPath = HeapAlloc(GetProcessHeap(), 0, needed*sizeof(WCHAR));
        rc = UrlEscapeW(path, lpwhr->lpszPath, &needed,
                        URL_ESCAPE_SPACES_ONLY);
        if (rc)
        {
            ERR("Unable to escape string!(%s) (%ld)\n",debugstr_w(path),rc);
            strcpyW(lpwhr->lpszPath,path);
        }
    }

    handle = WININET_FindHandle( &lpwhr->hdr );
    return HttpSendRequestW(handle, lpszHeaders, dwHeaderLength, lpOptional, dwOptionalLength);
}

/***********************************************************************
 *           HTTP_HttpSendRequestW (internal)
 *
 * Sends the specified request to the HTTP server
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI HTTP_HttpSendRequestW(HINTERNET hHttpRequest, LPCWSTR lpszHeaders,
	DWORD dwHeaderLength, LPVOID lpOptional ,DWORD dwOptionalLength)
{
    INT cnt;
    INT i;
    BOOL bSuccess = FALSE;
    LPWSTR requestString = NULL;
    LPWSTR lpszHeaders_r_n = NULL; /* lpszHeaders with atleast one pair of \r\n at the end */
    INT requestStringLen;
    INT responseLen;
    INT headerLength = 0;
    LPWININETHTTPREQW lpwhr;
    LPWININETHTTPSESSIONW lpwhs = NULL;
    LPWININETAPPINFOW hIC = NULL;
    BOOL loop_next = FALSE;
    int CustHeaderIndex;

    TRACE("--> 0x%08lx\n", (ULONG)hHttpRequest);

    /* Verify our tree of internet handles */
    lpwhr = (LPWININETHTTPREQW) WININET_GetObject( hHttpRequest );
    if (NULL == lpwhr ||  lpwhr->hdr.htype != WH_HHTTPREQ)
    {
        INTERNET_SetLastError(ERROR_INTERNET_INCORRECT_HANDLE_TYPE);
	return FALSE;
    }

    lpwhs = (LPWININETHTTPSESSIONW) lpwhr->hdr.lpwhparent;
    if (NULL == lpwhs ||  lpwhs->hdr.htype != WH_HHTTPSESSION)
    {
        INTERNET_SetLastError(ERROR_INTERNET_INCORRECT_HANDLE_TYPE);
	return FALSE;
    }

    hIC = (LPWININETAPPINFOW) lpwhs->hdr.lpwhparent;
    if (NULL == hIC ||  hIC->hdr.htype != WH_HINIT)
    {
        INTERNET_SetLastError(ERROR_INTERNET_INCORRECT_HANDLE_TYPE);
	return FALSE;
    }

    /* Clear any error information */
    INTERNET_SetLastError(0);


    /* We must have a verb */
    if (NULL == lpwhr->lpszVerb)
    {
	    goto lend;
    }

    /* if we are using optional stuff, we must add the fixed header of that option length */
    if (lpOptional && dwOptionalLength)
    {
        char contentLengthStr[sizeof("Content-Length: ") + 20 /* int */ + 2 /* \n\r */];
        sprintf(contentLengthStr, "Content-Length: %li\r\n", dwOptionalLength);
        HttpAddRequestHeadersA(hHttpRequest, contentLengthStr, -1L, HTTP_ADDREQ_FLAG_ADD);
    }

    do
    {
        WCHAR szSlash[] = { '/',0 };
        WCHAR szSpace[] = { ' ',0 };
        WCHAR szHttp[] = { 'h','t','t','p',':','/','/', 0 };
	WCHAR szcrlf[] = {'\r','\n', 0};
	WCHAR sztwocrlf[] = {'\r','\n','\r','\n', 0};
        WCHAR szSetCookie[] = {'S','e','t','-','C','o','o','k','i','e',0 };

        TRACE("Going to url %s %s\n", debugstr_w(lpwhr->lpszHostName), debugstr_w(lpwhr->lpszPath));
        loop_next = FALSE;

        /* If we don't have a path we set it to root */
        if (NULL == lpwhr->lpszPath)
            lpwhr->lpszPath = WININET_strdupW(szSlash);

        if(CSTR_EQUAL != CompareStringW( LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
                           lpwhr->lpszPath, strlenW(szHttp), szHttp, strlenW(szHttp) )
           && lpwhr->lpszPath[0] != '/') /* not an absolute path ?? --> fix it !! */
        {
            WCHAR *fixurl = HeapAlloc(GetProcessHeap(), 0, 
                                 (strlenW(lpwhr->lpszPath) + 2)*sizeof(WCHAR));
            *fixurl = '/';
            strcpyW(fixurl + 1, lpwhr->lpszPath);
            HeapFree( GetProcessHeap(), 0, lpwhr->lpszPath );
            lpwhr->lpszPath = fixurl;
        }

        /* Calculate length of request string */
        requestStringLen =
            strlenW(lpwhr->lpszVerb) +
            strlenW(lpwhr->lpszPath) +
            strlenW(HTTPHEADER) +
            5; /* " \r\n\r\n" */

	/* add "\r\n" to end of lpszHeaders if needed */
	if (lpszHeaders)
	{
	    int len = strlenW(lpszHeaders);

	    /* Check if the string is terminated with \r\n, but not if
	     * the string is less that 2 characters long, because then
	     * we would be looking at memory before the beginning of
	     * the string. Besides, if it is less than 2 characters
	     * long, then clearly, its not terminated with \r\n.
	     */
	    if ((len > 2) && (memcmp(lpszHeaders + (len - 2), szcrlf, sizeof szcrlf) == 0))
	    {
		lpszHeaders_r_n = WININET_strdupW(lpszHeaders);
	    }
	    else
	    {
		TRACE("Adding \r\n to lpszHeaders.\n");
		lpszHeaders_r_n =  HeapAlloc( GetProcessHeap(), 0, 
                                             (len + 3)*sizeof(WCHAR) );
		strcpyW( lpszHeaders_r_n, lpszHeaders );
		strcatW( lpszHeaders_r_n, szcrlf );
	    }
	}

        /* Add length of passed headers */
        if (lpszHeaders)
        {
            headerLength = -1 == dwHeaderLength ? strlenW(lpszHeaders_r_n) : dwHeaderLength;
            requestStringLen += headerLength +  2; /* \r\n */
        }


        /* if there isa proxy username and password, add it to the headers */
        if( hIC && (hIC->lpszProxyUsername || hIC->lpszProxyPassword ) )
        {
            HTTP_InsertProxyAuthorization( lpwhr, hIC->lpszProxyUsername, hIC->lpszProxyPassword );
        }

        /* Calculate length of custom request headers */
        for (i = 0; i < lpwhr->nCustHeaders; i++)
        {
	    if (lpwhr->pCustHeaders[i].wFlags & HDR_ISREQUEST)
	    {
                requestStringLen += strlenW(lpwhr->pCustHeaders[i].lpszField) +
                    strlenW(lpwhr->pCustHeaders[i].lpszValue) + 4; /*: \r\n */
	    }
        }

        /* Calculate the length of standard request headers */
        for (i = 0; i <= HTTP_QUERY_MAX; i++)
        {
            if (lpwhr->StdHeaders[i].wFlags & HDR_ISREQUEST)
            {
                requestStringLen += strlenW(lpwhr->StdHeaders[i].lpszField) +
                    strlenW(lpwhr->StdHeaders[i].lpszValue) + 4; /*: \r\n */
            }
        }

        if (lpwhr->lpszHostName)
            requestStringLen += (strlenW(HTTPHOSTHEADER) + strlenW(lpwhr->lpszHostName));

        /* if there is optional data to send, add the length */
        if (lpOptional)
        {
            requestStringLen += dwOptionalLength;
        }

        /* Allocate string to hold entire request */
        requestString = HeapAlloc(GetProcessHeap(), 0, (requestStringLen + 1)*sizeof(WCHAR));
        if (NULL == requestString)
        {
            INTERNET_SetLastError(ERROR_OUTOFMEMORY);
            goto lend;
        }

        /* Build request string */
        strcpyW(requestString, lpwhr->lpszVerb);
        strcatW(requestString, szSpace);
        strcatW(requestString, lpwhr->lpszPath);
        strcatW(requestString, HTTPHEADER );
        cnt = strlenW(requestString);

        /* Append standard request headers */
        for (i = 0; i <= HTTP_QUERY_MAX; i++)
        {
            if (lpwhr->StdHeaders[i].wFlags & HDR_ISREQUEST)
            {
                WCHAR szFmt[] = { '\r','\n','%','s',':',' ','%','s', 0};
                cnt += sprintfW(requestString + cnt, szFmt,
                               lpwhr->StdHeaders[i].lpszField, lpwhr->StdHeaders[i].lpszValue);
                TRACE("Adding header %s (%s)\n",
                       debugstr_w(lpwhr->StdHeaders[i].lpszField),
                       debugstr_w(lpwhr->StdHeaders[i].lpszValue));
            }
        }

        /* Append custom request heades */
        for (i = 0; i < lpwhr->nCustHeaders; i++)
        {
            if (lpwhr->pCustHeaders[i].wFlags & HDR_ISREQUEST)
            {
                WCHAR szFmt[] = { '\r','\n','%','s',':',' ','%','s', 0};
                cnt += sprintfW(requestString + cnt, szFmt,
                               lpwhr->pCustHeaders[i].lpszField, lpwhr->pCustHeaders[i].lpszValue);
                TRACE("Adding custom header %s (%s)\n",
                       debugstr_w(lpwhr->pCustHeaders[i].lpszField),
                       debugstr_w(lpwhr->pCustHeaders[i].lpszValue));
            }
        }

        if (lpwhr->lpszHostName)
        {
            WCHAR szFmt[] = { '%','s','%','s',0 };
            cnt += sprintfW(requestString + cnt, szFmt, HTTPHOSTHEADER, lpwhr->lpszHostName);
        }

        /* Append passed request headers */
        if (lpszHeaders_r_n)
        {
            strcpyW(requestString + cnt, szcrlf);
            cnt += 2;
            strcpyW(requestString + cnt, lpszHeaders_r_n);
            cnt += headerLength;
	    /* only add \r\n if not already present */
	    if (memcmp((requestString + cnt) - 2, szcrlf, sizeof szcrlf) != 0)
	    {
                strcpyW(requestString + cnt, szcrlf);
                cnt += 2;
	    }
        }

        /* Set (header) termination string for request */
        if (memcmp((requestString + cnt) - 4, sztwocrlf, sizeof sztwocrlf) != 0)
        { /* only add it if the request string doesn't already
             have the thing.. (could happen if the custom header
             added it */
                strcpyW(requestString + cnt, szcrlf);
                cnt += 2;
        }
        else
            requestStringLen -= 2;
 
        /* if optional data, append it */
        if (lpOptional)
        {
            memcpy(requestString + cnt, lpOptional, dwOptionalLength*sizeof(WCHAR));
            cnt += dwOptionalLength;
            /* we also have to decrease the expected string length by two,
             * since we won't be adding on those following \r\n's */
	    requestStringLen -= 2;
        }
        else
        { /* if there is no optional data, add on another \r\n just to be safe */
                /* termination for request */
                strcpyW(requestString + cnt, szcrlf);
                cnt += 2;
        }

        TRACE("(%s) len(%d)\n", debugstr_w(requestString), requestStringLen);
        /* Send the request and store the results */
        if (!HTTP_OpenConnection(lpwhr))
            goto lend;

        SendAsyncCallback(hIC, hHttpRequest, lpwhr->hdr.dwContext,
                          INTERNET_STATUS_SENDING_REQUEST, NULL, 0);

        /* send the request as ASCII */
        {
            int ascii_len;
            char *ascii_req;

            ascii_len = WideCharToMultiByte( CP_ACP, 0, requestString,
                            requestStringLen, NULL, 0, NULL, NULL );
            ascii_req = HeapAlloc( GetProcessHeap(), 0, ascii_len );
            WideCharToMultiByte( CP_ACP, 0, requestString, requestStringLen,
                            ascii_req, ascii_len, NULL, NULL );
            NETCON_send(&lpwhr->netConnection, ascii_req, ascii_len, 0, &cnt);
            HeapFree( GetProcessHeap(), 0, ascii_req );
        }


        SendAsyncCallback(hIC, hHttpRequest, lpwhr->hdr.dwContext,
                          INTERNET_STATUS_REQUEST_SENT,
                          &requestStringLen,sizeof(DWORD));

        SendAsyncCallback(hIC, hHttpRequest, lpwhr->hdr.dwContext,
                          INTERNET_STATUS_RECEIVING_RESPONSE, NULL, 0);

        if (cnt < 0)
            goto lend;

        responseLen = HTTP_GetResponseHeaders(lpwhr);
        if (responseLen)
	    bSuccess = TRUE;

        SendAsyncCallback(hIC, hHttpRequest, lpwhr->hdr.dwContext,
                          INTERNET_STATUS_RESPONSE_RECEIVED, &responseLen,
                          sizeof(DWORD));

        /* process headers here. Is this right? */
        CustHeaderIndex = HTTP_GetCustomHeaderIndex(lpwhr, szSetCookie);
        if (CustHeaderIndex >= 0)
        {
            LPHTTPHEADERW setCookieHeader;
            int nPosStart = 0, nPosEnd = 0, len;
            WCHAR szFmt[] = { 'h','t','t','p',':','/','/','%','s','/',0};

            setCookieHeader = &lpwhr->pCustHeaders[CustHeaderIndex];

            while (setCookieHeader->lpszValue[nPosEnd] != '\0')
            {
                LPWSTR buf_cookie, cookie_name, cookie_data;
                LPWSTR buf_url;
                LPWSTR domain = NULL;
                int nEqualPos = 0;
                while (setCookieHeader->lpszValue[nPosEnd] != ';' && setCookieHeader->lpszValue[nPosEnd] != ',' &&
                       setCookieHeader->lpszValue[nPosEnd] != '\0')
                {
                    nPosEnd++;
                }
                if (setCookieHeader->lpszValue[nPosEnd] == ';')
                {
                    /* fixme: not case sensitive, strcasestr is gnu only */
                    int nDomainPosEnd = 0;
                    int nDomainPosStart = 0, nDomainLength = 0;
                    WCHAR szDomain[] = {'d','o','m','a','i','n','=',0};
                    LPWSTR lpszDomain = strstrW(&setCookieHeader->lpszValue[nPosEnd], szDomain);
                    if (lpszDomain)
                    { /* they have specified their own domain, lets use it */
                        while (lpszDomain[nDomainPosEnd] != ';' && lpszDomain[nDomainPosEnd] != ',' &&
                               lpszDomain[nDomainPosEnd] != '\0')
                        {
                            nDomainPosEnd++;
                        }
                        nDomainPosStart = strlenW(szDomain);
                        nDomainLength = (nDomainPosEnd - nDomainPosStart) + 1;
                        domain = HeapAlloc(GetProcessHeap(), 0, (nDomainLength + 1)*sizeof(WCHAR));
                        strncpyW(domain, &lpszDomain[nDomainPosStart], nDomainLength);
                        domain[nDomainLength] = '\0';
                    }
                }
                if (setCookieHeader->lpszValue[nPosEnd] == '\0') break;
                buf_cookie = HeapAlloc(GetProcessHeap(), 0, ((nPosEnd - nPosStart) + 1)*sizeof(WCHAR));
                strncpyW(buf_cookie, &setCookieHeader->lpszValue[nPosStart], (nPosEnd - nPosStart));
                buf_cookie[(nPosEnd - nPosStart)] = '\0';
                TRACE("%s\n", debugstr_w(buf_cookie));
                while (buf_cookie[nEqualPos] != '=' && buf_cookie[nEqualPos] != '\0')
                {
                    nEqualPos++;
                }
                if (buf_cookie[nEqualPos] == '\0' || buf_cookie[nEqualPos + 1] == '\0')
                {
                    HeapFree(GetProcessHeap(), 0, buf_cookie);
                    break;
                }

                cookie_name = HeapAlloc(GetProcessHeap(), 0, (nEqualPos + 1)*sizeof(WCHAR));
                strncpyW(cookie_name, buf_cookie, nEqualPos);
                cookie_name[nEqualPos] = '\0';
                cookie_data = &buf_cookie[nEqualPos + 1];


                len = strlenW((domain ? domain : lpwhr->lpszHostName)) + strlenW(lpwhr->lpszPath) + 9;
                buf_url = HeapAlloc(GetProcessHeap(), 0, len*sizeof(WCHAR));
                sprintfW(buf_url, szFmt, (domain ? domain : lpwhr->lpszHostName)); /* FIXME PATH!!! */
                InternetSetCookieW(buf_url, cookie_name, cookie_data);

                HeapFree(GetProcessHeap(), 0, buf_url);
                HeapFree(GetProcessHeap(), 0, buf_cookie);
                HeapFree(GetProcessHeap(), 0, cookie_name);
                if (domain) HeapFree(GetProcessHeap(), 0, domain);
                nPosStart = nPosEnd;
            }
        }
    }
    while (loop_next);

lend:

    if (requestString)
        HeapFree(GetProcessHeap(), 0, requestString);

    if (lpszHeaders)
	HeapFree(GetProcessHeap(), 0, lpszHeaders_r_n);

    /* TODO: send notification for P3P header */
    
    if(!(hIC->hdr.dwFlags & INTERNET_FLAG_NO_AUTO_REDIRECT) && bSuccess)
    {
        DWORD dwCode,dwCodeLength=sizeof(DWORD),dwIndex=0;
        if(HttpQueryInfoW(hHttpRequest,HTTP_QUERY_FLAG_NUMBER|HTTP_QUERY_STATUS_CODE,&dwCode,&dwCodeLength,&dwIndex) &&
            (dwCode==302 || dwCode==301))
        {
            WCHAR szNewLocation[2048];
            DWORD dwBufferSize=2048;
            dwIndex=0;
            if(HttpQueryInfoW(hHttpRequest,HTTP_QUERY_LOCATION,szNewLocation,&dwBufferSize,&dwIndex))
            {
                SendAsyncCallback(hIC, hHttpRequest, lpwhr->hdr.dwContext,
                      INTERNET_STATUS_REDIRECT, szNewLocation,
                      dwBufferSize);
                return HTTP_HandleRedirect(lpwhr, szNewLocation, lpszHeaders,
                                           dwHeaderLength, lpOptional, dwOptionalLength);
            }
        }
    }

    if (hIC->lpfnStatusCB)
    {
        INTERNET_ASYNC_RESULT iar;

        iar.dwResult = (DWORD)bSuccess;
        iar.dwError = bSuccess ? ERROR_SUCCESS : INTERNET_GetLastError();

        SendAsyncCallback(hIC, hHttpRequest, lpwhr->hdr.dwContext,
                      INTERNET_STATUS_REQUEST_COMPLETE, &iar,
                      sizeof(INTERNET_ASYNC_RESULT));
    }

    TRACE("<--\n");
    return bSuccess;
}


/***********************************************************************
 *           HTTP_Connect  (internal)
 *
 * Create http session handle
 *
 * RETURNS
 *   HINTERNET a session handle on success
 *   NULL on failure
 *
 */
HINTERNET HTTP_Connect(HINTERNET hInternet, LPCWSTR lpszServerName,
	INTERNET_PORT nServerPort, LPCWSTR lpszUserName,
	LPCWSTR lpszPassword, DWORD dwFlags, DWORD dwContext)
{
    BOOL bSuccess = FALSE;
    LPWININETAPPINFOW hIC = NULL;
    LPWININETHTTPSESSIONW lpwhs = NULL;
    HINTERNET handle = NULL;

    TRACE("-->\n");

    hIC = (LPWININETAPPINFOW) WININET_GetObject( hInternet );
    if( (hIC == NULL) || (hIC->hdr.htype != WH_HINIT) )
        goto lerror;

    hIC->hdr.dwContext = dwContext;
    
    lpwhs = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WININETHTTPSESSIONW));
    if (NULL == lpwhs)
    {
        INTERNET_SetLastError(ERROR_OUTOFMEMORY);
	goto lerror;
    }

    handle = WININET_AllocHandle( &lpwhs->hdr );
    if (NULL == handle)
    {
        ERR("Failed to alloc handle\n");
        INTERNET_SetLastError(ERROR_OUTOFMEMORY);
	goto lerror;
    }

   /*
    * According to my tests. The name is not resolved until a request is sent
    */

    if (nServerPort == INTERNET_INVALID_PORT_NUMBER)
	nServerPort = INTERNET_DEFAULT_HTTP_PORT;

    lpwhs->hdr.htype = WH_HHTTPSESSION;
    lpwhs->hdr.lpwhparent = &hIC->hdr;
    lpwhs->hdr.dwFlags = dwFlags;
    lpwhs->hdr.dwContext = dwContext;
    if(hIC->lpszProxy && hIC->dwAccessType == INTERNET_OPEN_TYPE_PROXY) {
        if(strchrW(hIC->lpszProxy, ' '))
            FIXME("Several proxies not implemented.\n");
        if(hIC->lpszProxyBypass)
            FIXME("Proxy bypass is ignored.\n");
    }
    if (NULL != lpszServerName)
        lpwhs->lpszServerName = WININET_strdupW(lpszServerName);
    if (NULL != lpszUserName)
        lpwhs->lpszUserName = WININET_strdupW(lpszUserName);
    lpwhs->nServerPort = nServerPort;

    if (hIC->lpfnStatusCB)
    {
        INTERNET_ASYNC_RESULT iar;

        iar.dwResult = (DWORD)handle;
        iar.dwError = ERROR_SUCCESS;

        SendAsyncCallback(hIC, hInternet, dwContext,
                      INTERNET_STATUS_HANDLE_CREATED, &iar,
                      sizeof(INTERNET_ASYNC_RESULT));
    }

    bSuccess = TRUE;

lerror:
    if (!bSuccess && lpwhs)
    {
        HeapFree(GetProcessHeap(), 0, lpwhs);
        WININET_FreeHandle( handle );
	lpwhs = NULL;
    }

/*
 * a INTERNET_STATUS_REQUEST_COMPLETE is NOT sent here as per my tests on
 * windows
 */

    TRACE("%p --> %p\n", hInternet, handle);
    return handle;
}


/***********************************************************************
 *           HTTP_OpenConnection (internal)
 *
 * Connect to a web server
 *
 * RETURNS
 *
 *   TRUE  on success
 *   FALSE on failure
 */
BOOL HTTP_OpenConnection(LPWININETHTTPREQW lpwhr)
{
    BOOL bSuccess = FALSE;
    LPWININETHTTPSESSIONW lpwhs;
    LPWININETAPPINFOW hIC = NULL;

    TRACE("-->\n");


    if (NULL == lpwhr ||  lpwhr->hdr.htype != WH_HHTTPREQ)
    {
        INTERNET_SetLastError(ERROR_INVALID_PARAMETER);
        goto lend;
    }

    lpwhs = (LPWININETHTTPSESSIONW)lpwhr->hdr.lpwhparent;

    hIC = (LPWININETAPPINFOW) lpwhs->hdr.lpwhparent;
    SendAsyncCallback(hIC, lpwhr, lpwhr->hdr.dwContext,
                      INTERNET_STATUS_CONNECTING_TO_SERVER,
                      &(lpwhs->socketAddress),
                       sizeof(struct sockaddr_in));

    if (!NETCON_create(&lpwhr->netConnection, lpwhs->phostent->h_addrtype,
                         SOCK_STREAM, 0))
    {
	WARN("Socket creation failed\n");
        goto lend;
    }

    if (!NETCON_connect(&lpwhr->netConnection, (struct sockaddr *)&lpwhs->socketAddress,
                      sizeof(lpwhs->socketAddress)))
    {
       WARN("Unable to connect to host (%s)\n", strerror(errno));
       goto lend;
    }

    SendAsyncCallback(hIC, lpwhr, lpwhr->hdr.dwContext,
                      INTERNET_STATUS_CONNECTED_TO_SERVER,
                      &(lpwhs->socketAddress),
                       sizeof(struct sockaddr_in));

    bSuccess = TRUE;

lend:
    TRACE("%d <--\n", bSuccess);
    return bSuccess;
}


/***********************************************************************
 *           HTTP_GetResponseHeaders (internal)
 *
 * Read server response
 *
 * RETURNS
 *
 *   TRUE  on success
 *   FALSE on error
 */
BOOL HTTP_GetResponseHeaders(LPWININETHTTPREQW lpwhr)
{
    INT cbreaks = 0;
    WCHAR buffer[MAX_REPLY_LEN];
    DWORD buflen = MAX_REPLY_LEN;
    BOOL bSuccess = FALSE;
    INT  rc = 0;
    WCHAR value[MAX_FIELD_VALUE_LEN], field[MAX_FIELD_LEN];
    WCHAR szStatus[] = {'S','t','a','t','u','s',0};
    WCHAR szHttp[] = { 'H','T','T','P',0 };
    char bufferA[MAX_REPLY_LEN];

    TRACE("-->\n");

    if (!NETCON_connected(&lpwhr->netConnection))
        goto lend;

    /*
     * HACK peek at the buffer
     */
    NETCON_recv(&lpwhr->netConnection, buffer, buflen, MSG_PEEK, &rc);

    /*
     * We should first receive 'HTTP/1.x nnn' where nnn is the status code.
     */
    buflen = MAX_REPLY_LEN;
    memset(buffer, 0, MAX_REPLY_LEN);
    if (!NETCON_getNextLine(&lpwhr->netConnection, bufferA, &buflen))
        goto lend;
    MultiByteToWideChar( CP_ACP, 0, bufferA, buflen, buffer, MAX_REPLY_LEN );

    if (strncmpW(buffer, szHttp, 4) != 0)
        goto lend;

    buffer[12]='\0';
    HTTP_ProcessHeader(lpwhr, szStatus, buffer+9, (HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE));

    /* Parse each response line */
    do
    {
	buflen = MAX_REPLY_LEN;
        if (NETCON_getNextLine(&lpwhr->netConnection, bufferA, &buflen))
	{
            TRACE("got line %s, now interpretting\n", debugstr_a(bufferA));
            MultiByteToWideChar( CP_ACP, 0, bufferA, buflen, buffer, MAX_REPLY_LEN );
            if (!HTTP_InterpretHttpHeader(buffer, field, MAX_FIELD_LEN, value, MAX_FIELD_VALUE_LEN))
                break;

            HTTP_ProcessHeader(lpwhr, field, value, (HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE));
	}
	else
	{
	    cbreaks++;
	    if (cbreaks >= 2)
	       break;
	}
    }while(1);

    bSuccess = TRUE;

lend:

    TRACE("<--\n");
    if (bSuccess)
        return rc;
    else
        return FALSE;
}


/***********************************************************************
 *           HTTP_InterpretHttpHeader (internal)
 *
 * Parse server response
 *
 * RETURNS
 *
 *   TRUE  on success
 *   FALSE on error
 */
INT stripSpaces(LPCWSTR lpszSrc, LPWSTR lpszStart, INT *len)
{
    LPCWSTR lpsztmp;
    INT srclen;

    srclen = 0;

    while (*lpszSrc == ' ' && *lpszSrc != '\0')
	lpszSrc++;

    lpsztmp = lpszSrc;
    while(*lpsztmp != '\0')
    {
        if (*lpsztmp != ' ')
	    srclen = lpsztmp - lpszSrc + 1;

	lpsztmp++;
    }

    *len = min(*len, srclen);
    strncpyW(lpszStart, lpszSrc, *len);
    lpszStart[*len] = '\0';

    return *len;
}


BOOL HTTP_InterpretHttpHeader(LPWSTR buffer, LPWSTR field, INT fieldlen, LPWSTR value, INT valuelen)
{
    WCHAR *pd;
    BOOL bSuccess = FALSE;

    TRACE("\n");

    *field = '\0';
    *value = '\0';

    pd = strchrW(buffer, ':');
    if (pd)
    {
	*pd = '\0';
	if (stripSpaces(buffer, field, &fieldlen) > 0)
	{
	    if (stripSpaces(pd+1, value, &valuelen) > 0)
		bSuccess = TRUE;
	}
    }

    TRACE("%d: field(%s) Value(%s)\n", bSuccess, debugstr_w(field), debugstr_w(value));
    return bSuccess;
}


/***********************************************************************
 *           HTTP_GetStdHeaderIndex (internal)
 *
 * Lookup field index in standard http header array
 *
 * FIXME: This should be stuffed into a hash table
 */
INT HTTP_GetStdHeaderIndex(LPCWSTR lpszField)
{
    INT index = -1;
    WCHAR szContentLength[] = {
       'C','o','n','t','e','n','t','-','L','e','n','g','t','h',0};
    WCHAR szStatus[] = {'S','t','a','t','u','s',0};
    WCHAR szContentType[] = {
       'C','o','n','t','e','n','t','-','T','y','p','e',0};
    WCHAR szLastModified[] = {
       'L','a','s','t','-','M','o','d','i','f','i','e','d',0};
    WCHAR szLocation[] = {'L','o','c','a','t','i','o','n',0};
    WCHAR szAccept[] = {'A','c','c','e','p','t',0};
    WCHAR szReferer[] = { 'R','e','f','e','r','e','r',0};
    WCHAR szContentTrans[] = { 'C','o','n','t','e','n','t','-',
       'T','r','a','n','s','f','e','r','-','E','n','c','o','d','i','n','g',0};
    WCHAR szDate[] = { 'D','a','t','e',0};
    WCHAR szServer[] = { 'S','e','r','v','e','r',0};
    WCHAR szConnection[] = { 'C','o','n','n','e','c','t','i','o','n',0};
    WCHAR szETag[] = { 'E','T','a','g',0};
    WCHAR szAcceptRanges[] = {
       'A','c','c','e','p','t','-','R','a','n','g','e','s',0 };
    WCHAR szExpires[] = { 'E','x','p','i','r','e','s',0 };
    WCHAR szMimeVersion[] = {
       'M','i','m','e','-','V','e','r','s','i','o','n', 0};
    WCHAR szPragma[] = { 'P','r','a','g','m','a', 0};
    WCHAR szCacheControl[] = {
       'C','a','c','h','e','-','C','o','n','t','r','o','l',0};
    WCHAR szUserAgent[] = { 'U','s','e','r','-','A','g','e','n','t',0};
    WCHAR szProxyAuth[] = {
       'P','r','o','x','y','-',
       'A','u','t','h','e','n','t','i','c','a','t','e', 0};

    if (!strcmpiW(lpszField, szContentLength))
        index = HTTP_QUERY_CONTENT_LENGTH;
    else if (!strcmpiW(lpszField,szStatus))
        index = HTTP_QUERY_STATUS_CODE;
    else if (!strcmpiW(lpszField,szContentType))
        index = HTTP_QUERY_CONTENT_TYPE;
    else if (!strcmpiW(lpszField,szLastModified))
        index = HTTP_QUERY_LAST_MODIFIED;
    else if (!strcmpiW(lpszField,szLocation))
        index = HTTP_QUERY_LOCATION;
    else if (!strcmpiW(lpszField,szAccept))
        index = HTTP_QUERY_ACCEPT;
    else if (!strcmpiW(lpszField,szReferer))
        index = HTTP_QUERY_REFERER;
    else if (!strcmpiW(lpszField,szContentTrans))
        index = HTTP_QUERY_CONTENT_TRANSFER_ENCODING;
    else if (!strcmpiW(lpszField,szDate))
        index = HTTP_QUERY_DATE;
    else if (!strcmpiW(lpszField,szServer))
        index = HTTP_QUERY_SERVER;
    else if (!strcmpiW(lpszField,szConnection))
        index = HTTP_QUERY_CONNECTION;
    else if (!strcmpiW(lpszField,szETag))
        index = HTTP_QUERY_ETAG;
    else if (!strcmpiW(lpszField,szAcceptRanges))
        index = HTTP_QUERY_ACCEPT_RANGES;
    else if (!strcmpiW(lpszField,szExpires))
        index = HTTP_QUERY_EXPIRES;
    else if (!strcmpiW(lpszField,szMimeVersion))
        index = HTTP_QUERY_MIME_VERSION;
    else if (!strcmpiW(lpszField,szPragma))
        index = HTTP_QUERY_PRAGMA;
    else if (!strcmpiW(lpszField,szCacheControl))
        index = HTTP_QUERY_CACHE_CONTROL;
    else if (!strcmpiW(lpszField,szUserAgent))
        index = HTTP_QUERY_USER_AGENT;
    else if (!strcmpiW(lpszField,szProxyAuth))
        index = HTTP_QUERY_PROXY_AUTHENTICATE;
    else
    {
        TRACE("Couldn't find %s in standard header table\n", debugstr_w(lpszField));
    }

    return index;
}


/***********************************************************************
 *           HTTP_ProcessHeader (internal)
 *
 * Stuff header into header tables according to <dwModifier>
 *
 */

#define COALESCEFLASG (HTTP_ADDHDR_FLAG_COALESCE|HTTP_ADDHDR_FLAG_COALESCE_WITH_COMMA|HTTP_ADDHDR_FLAG_COALESCE_WITH_SEMICOLON)

BOOL HTTP_ProcessHeader(LPWININETHTTPREQW lpwhr, LPCWSTR field, LPCWSTR value, DWORD dwModifier)
{
    LPHTTPHEADERW lphttpHdr = NULL;
    BOOL bSuccess = FALSE;
    INT index;

    TRACE("--> %s: %s - 0x%08x\n", debugstr_w(field), debugstr_w(value), (unsigned int)dwModifier);

    /* Adjust modifier flags */
    if (dwModifier & COALESCEFLASG)
	dwModifier |= HTTP_ADDHDR_FLAG_ADD;

    /* Try to get index into standard header array */
    index = HTTP_GetStdHeaderIndex(field);
    if (index >= 0)
    {
        lphttpHdr = &lpwhr->StdHeaders[index];
    }
    else /* Find or create new custom header */
    {
        index = HTTP_GetCustomHeaderIndex(lpwhr, field);
        if (index >= 0)
        {
            if (dwModifier & HTTP_ADDHDR_FLAG_ADD_IF_NEW)
            {
                return FALSE;
            }
            lphttpHdr = &lpwhr->pCustHeaders[index];
        }
        else
        {
            HTTPHEADERW hdr;

            hdr.lpszField = (LPWSTR)field;
            hdr.lpszValue = (LPWSTR)value;
            hdr.wFlags = hdr.wCount = 0;

            if (dwModifier & HTTP_ADDHDR_FLAG_REQ)
                hdr.wFlags |= HDR_ISREQUEST;

            return HTTP_InsertCustomHeader(lpwhr, &hdr);
        }
    }

    if (dwModifier & HTTP_ADDHDR_FLAG_REQ)
	lphttpHdr->wFlags |= HDR_ISREQUEST;
    else
        lphttpHdr->wFlags &= ~HDR_ISREQUEST;

    if (!lphttpHdr->lpszValue && (dwModifier & (HTTP_ADDHDR_FLAG_ADD|HTTP_ADDHDR_FLAG_ADD_IF_NEW)))
    {
        INT slen;

        if (!lpwhr->StdHeaders[index].lpszField)
        {
            lphttpHdr->lpszField = WININET_strdupW(field);

            if (dwModifier & HTTP_ADDHDR_FLAG_REQ)
                lphttpHdr->wFlags |= HDR_ISREQUEST;
        }

        slen = strlenW(value) + 1;
        lphttpHdr->lpszValue = HeapAlloc(GetProcessHeap(), 0, slen*sizeof(WCHAR));
        if (lphttpHdr->lpszValue)
        {
            strcpyW(lphttpHdr->lpszValue, value);
            bSuccess = TRUE;
        }
        else
        {
            INTERNET_SetLastError(ERROR_OUTOFMEMORY);
        }
    }
    else if (lphttpHdr->lpszValue)
    {
        if (dwModifier & HTTP_ADDHDR_FLAG_REPLACE)
        {
            LPWSTR lpsztmp;
            INT len;

            len = strlenW(value);

            if (len <= 0)
            {
	        /* if custom header delete from array */
                HeapFree(GetProcessHeap(), 0, lphttpHdr->lpszValue);
                lphttpHdr->lpszValue = NULL;
                bSuccess = TRUE;
            }
            else
            {
                lpsztmp = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,  lphttpHdr->lpszValue, (len+1)*sizeof(WCHAR));
                if (lpsztmp)
                {
                    lphttpHdr->lpszValue = lpsztmp;
                    strcpyW(lpsztmp, value);
                    bSuccess = TRUE;
                }
                else
                {
                    WARN("HeapReAlloc (%d bytes) failed\n",len+1);
                    INTERNET_SetLastError(ERROR_OUTOFMEMORY);
                }
            }
        }
        else if (dwModifier & COALESCEFLASG)
        {
            LPWSTR lpsztmp;
            WCHAR ch = 0;
            INT len = 0;
            INT origlen = strlenW(lphttpHdr->lpszValue);
            INT valuelen = strlenW(value);

            if (dwModifier & HTTP_ADDHDR_FLAG_COALESCE_WITH_COMMA)
            {
                ch = ',';
                lphttpHdr->wFlags |= HDR_COMMADELIMITED;
            }
            else if (dwModifier & HTTP_ADDHDR_FLAG_COALESCE_WITH_SEMICOLON)
            {
                ch = ';';
                lphttpHdr->wFlags |= HDR_COMMADELIMITED;
            }

            len = origlen + valuelen + ((ch > 0) ? 1 : 0);

            lpsztmp = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,  lphttpHdr->lpszValue, (len+1)*sizeof(WCHAR));
            if (lpsztmp)
            {
		/* FIXME: Increment lphttpHdr->wCount. Perhaps lpszValue should be an array */
                if (ch > 0)
                {
                    lphttpHdr->lpszValue[origlen] = ch;
                    origlen++;
                }

                memcpy(&lphttpHdr->lpszValue[origlen], value, valuelen*sizeof(WCHAR));
                lphttpHdr->lpszValue[len] = '\0';
                bSuccess = TRUE;
            }
            else
            {
                WARN("HeapReAlloc (%d bytes) failed\n",len+1);
                INTERNET_SetLastError(ERROR_OUTOFMEMORY);
            }
        }
    }
    TRACE("<-- %d\n",bSuccess);
    return bSuccess;
}


/***********************************************************************
 *           HTTP_CloseConnection (internal)
 *
 * Close socket connection
 *
 */
VOID HTTP_CloseConnection(LPWININETHTTPREQW lpwhr)
{


    LPWININETHTTPSESSIONW lpwhs = NULL;
    LPWININETAPPINFOW hIC = NULL;
    HINTERNET handle;

    TRACE("%p\n",lpwhr);

    handle = WININET_FindHandle( &lpwhr->hdr );
    lpwhs = (LPWININETHTTPSESSIONW) lpwhr->hdr.lpwhparent;
    hIC = (LPWININETAPPINFOW) lpwhs->hdr.lpwhparent;

    SendAsyncCallback(hIC, lpwhr, lpwhr->hdr.dwContext,
                      INTERNET_STATUS_CLOSING_CONNECTION, 0, 0);

    if (NETCON_connected(&lpwhr->netConnection))
    {
        NETCON_close(&lpwhr->netConnection);
    }

    SendAsyncCallback(hIC, lpwhr, lpwhr->hdr.dwContext,
                      INTERNET_STATUS_CONNECTION_CLOSED, 0, 0);
}


/***********************************************************************
 *           HTTP_CloseHTTPRequestHandle (internal)
 *
 * Deallocate request handle
 *
 */
void HTTP_CloseHTTPRequestHandle(LPWININETHTTPREQW lpwhr)
{
    int i;
    LPWININETHTTPSESSIONW lpwhs = NULL;
    LPWININETAPPINFOW hIC = NULL;
    HINTERNET handle;

    TRACE("\n");

    if (NETCON_connected(&lpwhr->netConnection))
        HTTP_CloseConnection(lpwhr);

    handle = WININET_FindHandle( &lpwhr->hdr );
    lpwhs = (LPWININETHTTPSESSIONW) lpwhr->hdr.lpwhparent;
    hIC = (LPWININETAPPINFOW) lpwhs->hdr.lpwhparent;

    SendAsyncCallback(hIC, handle, lpwhr->hdr.dwContext,
                      INTERNET_STATUS_HANDLE_CLOSING, lpwhr,
                      sizeof(HINTERNET));

    if (lpwhr->lpszPath)
        HeapFree(GetProcessHeap(), 0, lpwhr->lpszPath);
    if (lpwhr->lpszVerb)
        HeapFree(GetProcessHeap(), 0, lpwhr->lpszVerb);
    if (lpwhr->lpszHostName)
        HeapFree(GetProcessHeap(), 0, lpwhr->lpszHostName);

    for (i = 0; i <= HTTP_QUERY_MAX; i++)
    {
	   if (lpwhr->StdHeaders[i].lpszField)
            HeapFree(GetProcessHeap(), 0, lpwhr->StdHeaders[i].lpszField);
	   if (lpwhr->StdHeaders[i].lpszValue)
            HeapFree(GetProcessHeap(), 0, lpwhr->StdHeaders[i].lpszValue);
    }

    for (i = 0; i < lpwhr->nCustHeaders; i++)
    {
	   if (lpwhr->pCustHeaders[i].lpszField)
            HeapFree(GetProcessHeap(), 0, lpwhr->pCustHeaders[i].lpszField);
	   if (lpwhr->pCustHeaders[i].lpszValue)
            HeapFree(GetProcessHeap(), 0, lpwhr->pCustHeaders[i].lpszValue);
    }

    HeapFree(GetProcessHeap(), 0, lpwhr->pCustHeaders);
    HeapFree(GetProcessHeap(), 0, lpwhr);
}


/***********************************************************************
 *           HTTP_CloseHTTPSessionHandle (internal)
 *
 * Deallocate session handle
 *
 */
void HTTP_CloseHTTPSessionHandle(LPWININETHTTPSESSIONW lpwhs)
{
    LPWININETAPPINFOW hIC = NULL;
    HINTERNET handle;

    TRACE("%p\n", lpwhs);

    hIC = (LPWININETAPPINFOW) lpwhs->hdr.lpwhparent;

    handle = WININET_FindHandle( &lpwhs->hdr );
    SendAsyncCallback(hIC, handle, lpwhs->hdr.dwContext,
                      INTERNET_STATUS_HANDLE_CLOSING, lpwhs,
                      sizeof(HINTERNET));

    if (lpwhs->lpszServerName)
        HeapFree(GetProcessHeap(), 0, lpwhs->lpszServerName);
    if (lpwhs->lpszUserName)
        HeapFree(GetProcessHeap(), 0, lpwhs->lpszUserName);
    HeapFree(GetProcessHeap(), 0, lpwhs);
}


/***********************************************************************
 *           HTTP_GetCustomHeaderIndex (internal)
 *
 * Return index of custom header from header array
 *
 */
INT HTTP_GetCustomHeaderIndex(LPWININETHTTPREQW lpwhr, LPCWSTR lpszField)
{
    INT index;

    TRACE("%s\n", debugstr_w(lpszField));

    for (index = 0; index < lpwhr->nCustHeaders; index++)
    {
	if (!strcmpiW(lpwhr->pCustHeaders[index].lpszField, lpszField))
	    break;

    }

    if (index >= lpwhr->nCustHeaders)
	index = -1;

    TRACE("Return: %d\n", index);
    return index;
}


/***********************************************************************
 *           HTTP_InsertCustomHeader (internal)
 *
 * Insert header into array
 *
 */
BOOL HTTP_InsertCustomHeader(LPWININETHTTPREQW lpwhr, LPHTTPHEADERW lpHdr)
{
    INT count;
    LPHTTPHEADERW lph = NULL;
    BOOL r = FALSE;

    TRACE("--> %s: %s\n", debugstr_w(lpHdr->lpszField), debugstr_w(lpHdr->lpszValue));
    count = lpwhr->nCustHeaders + 1;
    if (count > 1)
	lph = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lpwhr->pCustHeaders, sizeof(HTTPHEADERW) * count);
    else
	lph = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(HTTPHEADERW) * count);

    if (NULL != lph)
    {
	lpwhr->pCustHeaders = lph;
        lpwhr->pCustHeaders[count-1].lpszField = WININET_strdupW(lpHdr->lpszField);
        lpwhr->pCustHeaders[count-1].lpszValue = WININET_strdupW(lpHdr->lpszValue);
        lpwhr->pCustHeaders[count-1].wFlags = lpHdr->wFlags;
        lpwhr->pCustHeaders[count-1].wCount= lpHdr->wCount;
	lpwhr->nCustHeaders++;
        r = TRUE;
    }
    else
    {
        INTERNET_SetLastError(ERROR_OUTOFMEMORY);
    }

    return r;
}


/***********************************************************************
 *           HTTP_DeleteCustomHeader (internal)
 *
 * Delete header from array
 *  If this function is called, the indexs may change.
 */
BOOL HTTP_DeleteCustomHeader(LPWININETHTTPREQW lpwhr, INT index)
{
    if( lpwhr->nCustHeaders <= 0 )
        return FALSE;
    if( lpwhr->nCustHeaders >= index )
        return FALSE;
    lpwhr->nCustHeaders--;

    memmove( &lpwhr->pCustHeaders[index], &lpwhr->pCustHeaders[index+1],
             (lpwhr->nCustHeaders - index)* sizeof(HTTPHEADERW) );
    memset( &lpwhr->pCustHeaders[lpwhr->nCustHeaders], 0, sizeof(HTTPHEADERW) );

    return TRUE;
}

/***********************************************************************
 *          IsHostInProxyBypassList (@)
 *
 * Undocumented
 *
 */
BOOL WINAPI IsHostInProxyBypassList(DWORD flags, LPCSTR szHost, DWORD length)
{
   FIXME("STUB: flags=%ld host=%s length=%ld\n",flags,szHost,length);
   return FALSE;
}
