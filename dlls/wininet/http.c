/*
 * Wininet - Http Implementation
 *
 * Copyright 1999 Corel Corporation
 * Copyright 2002 CodeWeavers Inc.
 * Copyright 2002 TransGaming Technologies Inc.
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

#define HTTPHEADER " HTTP/1.0"
#define HTTPHOSTHEADER "\r\nHost: "
#define MAXHOSTNAME 100
#define MAX_FIELD_VALUE_LEN 256
#define MAX_FIELD_LEN 256


#define HTTP_REFERER    "Referer"
#define HTTP_ACCEPT     "Accept"
#define HTTP_USERAGENT  "User-Agent"

#define HTTP_ADDHDR_FLAG_ADD				0x20000000
#define HTTP_ADDHDR_FLAG_ADD_IF_NEW			0x10000000
#define HTTP_ADDHDR_FLAG_COALESCE			0x40000000
#define HTTP_ADDHDR_FLAG_COALESCE_WITH_COMMA		0x40000000
#define HTTP_ADDHDR_FLAG_COALESCE_WITH_SEMICOLON	0x01000000
#define HTTP_ADDHDR_FLAG_REPLACE			0x80000000
#define HTTP_ADDHDR_FLAG_REQ				0x02000000


BOOL HTTP_OpenConnection(LPWININETHTTPREQA lpwhr);
int HTTP_WriteDataToStream(LPWININETHTTPREQA lpwhr,
	void *Buffer, int BytesToWrite);
int HTTP_ReadDataFromStream(LPWININETHTTPREQA lpwhr,
	void *Buffer, int BytesToRead);
BOOL HTTP_GetResponseHeaders(LPWININETHTTPREQA lpwhr);
BOOL HTTP_ProcessHeader(LPWININETHTTPREQA lpwhr, LPCSTR field, LPCSTR value, DWORD dwModifier);
void HTTP_CloseConnection(LPWININETHTTPREQA lpwhr);
BOOL HTTP_InterpretHttpHeader(LPSTR buffer, LPSTR field, INT fieldlen, LPSTR value, INT valuelen);
INT HTTP_GetStdHeaderIndex(LPCSTR lpszField);
BOOL HTTP_InsertCustomHeader(LPWININETHTTPREQA lpwhr, LPHTTPHEADERA lpHdr);
INT HTTP_GetCustomHeaderIndex(LPWININETHTTPREQA lpwhr, LPCSTR lpszField);
BOOL HTTP_DeleteCustomHeader(LPWININETHTTPREQA lpwhr, INT index);

inline static LPSTR HTTP_strdup( LPCSTR str )
{
    LPSTR ret = HeapAlloc( GetProcessHeap(), 0, strlen(str) + 1 );
    if (ret) strcpy( ret, str );
    return ret;
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
    LPSTR lpszStart;
    LPSTR lpszEnd;
    LPSTR buffer;
    CHAR value[MAX_FIELD_VALUE_LEN], field[MAX_FIELD_LEN];
    BOOL bSuccess = FALSE;
    LPWININETHTTPREQA lpwhr = (LPWININETHTTPREQA) hHttpRequest;

    TRACE("%p, %s, %li, %li\n", hHttpRequest, lpszHeader, dwHeaderLength,
          dwModifier);


    if (NULL == lpwhr ||  lpwhr->hdr.htype != WH_HHTTPREQ)
    {
        INTERNET_SetLastError(ERROR_INTERNET_INCORRECT_HANDLE_TYPE);
        return FALSE;
    }

    if (!lpszHeader) 
      return TRUE;

    TRACE("copying header: %s\n", lpszHeader);
    buffer = HTTP_strdup(lpszHeader);
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

        TRACE("interpreting header %s\n", debugstr_a(lpszStart));
        if (HTTP_InterpretHttpHeader(lpszStart, field, MAX_FIELD_LEN, value, MAX_FIELD_VALUE_LEN))
            bSuccess = HTTP_ProcessHeader(lpwhr, field, value, dwModifier | HTTP_ADDHDR_FLAG_REQ);

        lpszStart = lpszEnd + 2; /* Jump over \0\n */

    } while (bSuccess);

    HeapFree(GetProcessHeap(), 0, buffer);
    return bSuccess;
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
    LPWININETHTTPSESSIONA lpwhs = (LPWININETHTTPSESSIONA) hHttpSession;
    LPWININETAPPINFOA hIC = NULL;

    TRACE("(%p, %s, %s, %s, %s, %p, %08lx, %08lx)\n", hHttpSession,
          debugstr_a(lpszVerb), lpszObjectName,
          debugstr_a(lpszVersion), debugstr_a(lpszReferrer), lpszAcceptTypes,
          dwFlags, dwContext);
    if(lpszAcceptTypes!=NULL)
    {
        int i;
        for(i=0;lpszAcceptTypes[i]!=NULL;i++)
            TRACE("\taccept type: %s\n",lpszAcceptTypes[i]);
    }    

    if (NULL == lpwhs ||  lpwhs->hdr.htype != WH_HHTTPSESSION)
    {
        INTERNET_SetLastError(ERROR_INTERNET_INCORRECT_HANDLE_TYPE);
	return FALSE;
    }
    hIC = (LPWININETAPPINFOA) lpwhs->hdr.lpwhparent;

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

	workRequest.asyncall = HTTPOPENREQUESTA;
	workRequest.HFTPSESSION = (DWORD)hHttpSession;
	workRequest.LPSZVERB = (DWORD)HTTP_strdup(lpszVerb);
	workRequest.LPSZOBJECTNAME = (DWORD)HTTP_strdup(lpszObjectName);
        if (lpszVersion)
            workRequest.LPSZVERSION = (DWORD)HTTP_strdup(lpszVersion);
        else
            workRequest.LPSZVERSION = 0;
        if (lpszReferrer)
            workRequest.LPSZREFERRER = (DWORD)HTTP_strdup(lpszReferrer);
        else
            workRequest.LPSZREFERRER = 0;
	workRequest.LPSZACCEPTTYPES = (DWORD)lpszAcceptTypes;
	workRequest.DWFLAGS = dwFlags;
	workRequest.DWCONTEXT = dwContext;

        INTERNET_AsyncCall(&workRequest);
        TRACE ("returning NULL\n");
        return NULL;
    }
    else
    {
        HINTERNET rec = HTTP_HttpOpenRequestA(hHttpSession, lpszVerb, lpszObjectName,
                                              lpszVersion, lpszReferrer, lpszAcceptTypes,
                                              dwFlags, dwContext);
        TRACE("returning %p\n", rec);
        return rec;
    }
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
 * FIXME: This should be the other way around (A should call W)
 */
HINTERNET WINAPI HttpOpenRequestW(HINTERNET hHttpSession,
	LPCWSTR lpszVerb, LPCWSTR lpszObjectName, LPCWSTR lpszVersion,
	LPCWSTR lpszReferrer , LPCWSTR *lpszAcceptTypes,
	DWORD dwFlags, DWORD dwContext)
{
    CHAR *szVerb = NULL, *szObjectName = NULL;
    CHAR *szVersion = NULL, *szReferrer = NULL, **szAcceptTypes = NULL;
    INT len;
    INT acceptTypesCount;
    HINTERNET rc = FALSE;
    TRACE("(%p, %s, %s, %s, %s, %p, %08lx, %08lx)\n", hHttpSession,
          debugstr_w(lpszVerb), debugstr_w(lpszObjectName),
          debugstr_w(lpszVersion), debugstr_w(lpszReferrer), lpszAcceptTypes,
          dwFlags, dwContext);

    if (lpszVerb)
    {
        len = WideCharToMultiByte(CP_ACP, 0, lpszVerb, -1, NULL, 0, NULL, NULL);
        szVerb = HeapAlloc(GetProcessHeap(), 0, len * sizeof(CHAR) );
        if ( !szVerb )
            goto end;
        WideCharToMultiByte(CP_ACP, 0, lpszVerb, -1, szVerb, len, NULL, NULL);
    }

    if (lpszObjectName)
    {
        len = WideCharToMultiByte(CP_ACP, 0, lpszObjectName, -1, NULL, 0, NULL, NULL);
        szObjectName = HeapAlloc(GetProcessHeap(), 0, len * sizeof(CHAR) );
        if ( !szObjectName )
            goto end;
        WideCharToMultiByte(CP_ACP, 0, lpszObjectName, -1, szObjectName, len, NULL, NULL);
    }

    if (lpszVersion)
    {
        len = WideCharToMultiByte(CP_ACP, 0, lpszVersion, -1, NULL, 0, NULL, NULL);
        szVersion = HeapAlloc(GetProcessHeap(), 0, len * sizeof(CHAR));
        if ( !szVersion )
            goto end;
        WideCharToMultiByte(CP_ACP, 0, lpszVersion, -1, szVersion, len, NULL, NULL);
    }

    if (lpszReferrer)
    {
        len = WideCharToMultiByte(CP_ACP, 0, lpszReferrer, -1, NULL, 0, NULL, NULL);
        szReferrer = HeapAlloc(GetProcessHeap(), 0, len * sizeof(CHAR));
        if ( !szReferrer )
            goto end;
        WideCharToMultiByte(CP_ACP, 0, lpszReferrer, -1, szReferrer, len, NULL, NULL);
    }

    acceptTypesCount = 0;
    if (lpszAcceptTypes)
    {
        while (lpszAcceptTypes[acceptTypesCount]) { acceptTypesCount++; } /* find out how many there are */
        szAcceptTypes = HeapAlloc(GetProcessHeap(), 0, sizeof(CHAR *) * acceptTypesCount);
        acceptTypesCount = 0;
        while (lpszAcceptTypes[acceptTypesCount])
        {
            len = WideCharToMultiByte(CP_ACP, 0, lpszAcceptTypes[acceptTypesCount],
                                -1, NULL, 0, NULL, NULL);
            szAcceptTypes[acceptTypesCount] = HeapAlloc(GetProcessHeap(), 0, len * sizeof(CHAR));
            if (!szAcceptTypes[acceptTypesCount] )
                goto end;
            WideCharToMultiByte(CP_ACP, 0, lpszAcceptTypes[acceptTypesCount],
                                -1, szAcceptTypes[acceptTypesCount], len, NULL, NULL);
            acceptTypesCount++;
        }
    }
    else szAcceptTypes = 0;

    rc = HttpOpenRequestA(hHttpSession, (LPCSTR)szVerb, (LPCSTR)szObjectName,
                          (LPCSTR)szVersion, (LPCSTR)szReferrer,
                          (LPCSTR *)szAcceptTypes, dwFlags, dwContext);

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
static UINT HTTP_Base64( LPCSTR bin, LPSTR base64 )
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
static LPSTR HTTP_EncodeBasicAuth( LPCSTR username, LPCSTR password)
{
    UINT len;
    LPSTR in, out, szBasic = "Basic ";

    len = strlen( username ) + 1 + strlen ( password ) + 1;
    in = HeapAlloc( GetProcessHeap(), 0, len );
    if( !in )
        return NULL;

    len = strlen(szBasic) +
          (strlen( username ) + 1 + strlen ( password ))*2 + 1 + 1;
    out = HeapAlloc( GetProcessHeap(), 0, len );
    if( out )
    {
        strcpy( in, username );
        strcat( in, ":" );
        strcat( in, password );
        strcpy( out, szBasic );
        HTTP_Base64( in, &out[strlen(out)] );
    }
    HeapFree( GetProcessHeap(), 0, in );

    return out;
}

/***********************************************************************
 *  HTTP_InsertProxyAuthorization
 *
 *   Insert the basic authorization field in the request header
 */
BOOL HTTP_InsertProxyAuthorization( LPWININETHTTPREQA lpwhr,
                       LPCSTR username, LPCSTR password )
{
    HTTPHEADERA hdr;
    INT index;

    hdr.lpszField = "Proxy-Authorization";
    hdr.lpszValue = HTTP_EncodeBasicAuth( username, password );
    hdr.wFlags = HDR_ISREQUEST;
    hdr.wCount = 0;
    if( !hdr.lpszValue )
        return FALSE;

    TRACE("Inserting %s = %s\n",
          debugstr_a( hdr.lpszField ), debugstr_a( hdr.lpszValue ) );

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
static BOOL HTTP_DealWithProxy( LPWININETAPPINFOA hIC,
    LPWININETHTTPSESSIONA lpwhs, LPWININETHTTPREQA lpwhr)
{
    char buf[MAXHOSTNAME];
    char proxy[MAXHOSTNAME + 15]; /* 15 == "http://" + sizeof(port#) + ":/\0" */
    char* url, *szNul = "";
    URL_COMPONENTSA UrlComponents;

    memset( &UrlComponents, 0, sizeof UrlComponents );
    UrlComponents.dwStructSize = sizeof UrlComponents;
    UrlComponents.lpszHostName = buf;
    UrlComponents.dwHostNameLength = MAXHOSTNAME;

    sprintf(proxy, "http://%s/", hIC->lpszProxy);
    if( !InternetCrackUrlA(proxy, 0, 0, &UrlComponents) )
        return FALSE;
    if( UrlComponents.dwHostNameLength == 0 )
        return FALSE;

    if( !lpwhr->lpszPath )
        lpwhr->lpszPath = szNul;
    TRACE("server='%s' path='%s'\n",
          lpwhs->lpszServerName, lpwhr->lpszPath);
    /* for constant 15 see above */
    url = HeapAlloc(GetProcessHeap(), 0,
        strlen(lpwhs->lpszServerName) + strlen(lpwhr->lpszPath) + 15);

    if(UrlComponents.nPort == INTERNET_INVALID_PORT_NUMBER)
        UrlComponents.nPort = INTERNET_DEFAULT_HTTP_PORT;

    sprintf(url, "http://%s:%d", lpwhs->lpszServerName,
            lpwhs->nServerPort);
    if( lpwhr->lpszPath[0] != '/' )
        strcat( url, "/" );
    strcat(url, lpwhr->lpszPath);
    if(lpwhr->lpszPath != szNul)
        HeapFree(GetProcessHeap(), 0, lpwhr->lpszPath);
    lpwhr->lpszPath = url;
    /* FIXME: Do I have to free lpwhs->lpszServerName here ? */
    lpwhs->lpszServerName = HTTP_strdup(UrlComponents.lpszHostName);
    lpwhs->nServerPort = UrlComponents.nPort;

    return TRUE;
}

/***********************************************************************
 *           HTTP_HttpOpenRequestA (internal)
 *
 * Open a HTTP request handle
 *
 * RETURNS
 *    HINTERNET  a HTTP request handle on success
 *    NULL 	 on failure
 *
 */
HINTERNET WINAPI HTTP_HttpOpenRequestA(HINTERNET hHttpSession,
	LPCSTR lpszVerb, LPCSTR lpszObjectName, LPCSTR lpszVersion,
	LPCSTR lpszReferrer , LPCSTR *lpszAcceptTypes,
	DWORD dwFlags, DWORD dwContext)
{
    LPWININETHTTPSESSIONA lpwhs = (LPWININETHTTPSESSIONA) hHttpSession;
    LPWININETAPPINFOA hIC = NULL;
    LPWININETHTTPREQA lpwhr;
    LPSTR lpszCookies;
    LPSTR lpszUrl = NULL;
    DWORD nCookieSize;

    TRACE("--> \n");

    if (NULL == lpwhs ||  lpwhs->hdr.htype != WH_HHTTPSESSION)
    {
        INTERNET_SetLastError(ERROR_INTERNET_INCORRECT_HANDLE_TYPE);
	return FALSE;
    }

    hIC = (LPWININETAPPINFOA) lpwhs->hdr.lpwhparent;

    lpwhr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WININETHTTPREQA));
    if (NULL == lpwhr)
    {
        INTERNET_SetLastError(ERROR_OUTOFMEMORY);
        return (HINTERNET) NULL;
    }

    lpwhr->hdr.htype = WH_HHTTPREQ;
    lpwhr->hdr.lpwhparent = hHttpSession;
    lpwhr->hdr.dwFlags = dwFlags;
    lpwhr->hdr.dwContext = dwContext;
    NETCON_init(&lpwhr->netConnection, dwFlags & INTERNET_FLAG_SECURE);

    if (NULL != lpszObjectName && strlen(lpszObjectName)) {
        DWORD needed = 0;
        HRESULT rc;
        rc = UrlEscapeA(lpszObjectName, NULL, &needed, URL_ESCAPE_SPACES_ONLY);
        if (rc != E_POINTER)
            needed = strlen(lpszObjectName)+1;
        lpwhr->lpszPath = HeapAlloc(GetProcessHeap(), 0, needed);
        rc = UrlEscapeA(lpszObjectName, lpwhr->lpszPath, &needed,
                   URL_ESCAPE_SPACES_ONLY);
        if (rc)
        {
            ERR("Unable to escape string!(%s) (%ld)\n",lpszObjectName,rc);
            strcpy(lpwhr->lpszPath,lpszObjectName);
        }
    }

    if (NULL != lpszReferrer && strlen(lpszReferrer))
        HTTP_ProcessHeader(lpwhr, HTTP_REFERER, lpszReferrer, HTTP_ADDHDR_FLAG_COALESCE);

    if(lpszAcceptTypes!=NULL)
    {
        int i;
        for(i=0;lpszAcceptTypes[i]!=NULL;i++)
            HTTP_ProcessHeader(lpwhr, HTTP_ACCEPT, lpszAcceptTypes[i], HTTP_ADDHDR_FLAG_COALESCE_WITH_COMMA|HTTP_ADDHDR_FLAG_REQ|HTTP_ADDHDR_FLAG_ADD_IF_NEW);
    }

    if (NULL == lpszVerb)
        lpwhr->lpszVerb = HTTP_strdup("GET");
    else if (strlen(lpszVerb))
        lpwhr->lpszVerb = HTTP_strdup(lpszVerb);

    if (NULL != lpszReferrer && strlen(lpszReferrer))
    {
        char buf[MAXHOSTNAME];
        URL_COMPONENTSA UrlComponents;

        memset( &UrlComponents, 0, sizeof UrlComponents );
        UrlComponents.dwStructSize = sizeof UrlComponents;
        UrlComponents.lpszHostName = buf;
        UrlComponents.dwHostNameLength = MAXHOSTNAME;

        InternetCrackUrlA(lpszReferrer, 0, 0, &UrlComponents);
        if (strlen(UrlComponents.lpszHostName))
            lpwhr->lpszHostName = HTTP_strdup(UrlComponents.lpszHostName);
    } else {
        lpwhr->lpszHostName = HTTP_strdup(lpwhs->lpszServerName);
    }
    if (NULL != hIC->lpszProxy && hIC->lpszProxy[0] != 0)
        HTTP_DealWithProxy( hIC, lpwhs, lpwhr );

    if (hIC->lpszAgent)
    {
        char *agent_header = HeapAlloc(GetProcessHeap(), 0, strlen(hIC->lpszAgent) + 1 + 14);
        sprintf(agent_header, "User-Agent: %s\r\n", hIC->lpszAgent);
        HttpAddRequestHeadersA((HINTERNET)lpwhr, agent_header, strlen(agent_header),
                               HTTP_ADDREQ_FLAG_ADD);
        HeapFree(GetProcessHeap(), 0, agent_header);
    }

    lpszUrl = HeapAlloc(GetProcessHeap(), 0, strlen(lpwhr->lpszHostName) + 1 + 7);
    sprintf(lpszUrl, "http://%s", lpwhr->lpszHostName);
    if (InternetGetCookieA(lpszUrl, NULL, NULL, &nCookieSize))
    {
        int cnt = 0;

        lpszCookies = HeapAlloc(GetProcessHeap(), 0, nCookieSize + 1 + 8);

        cnt += sprintf(lpszCookies, "Cookie: ");
        InternetGetCookieA(lpszUrl, NULL, lpszCookies + cnt, &nCookieSize);
        cnt += nCookieSize - 1;
        sprintf(lpszCookies + cnt, "\r\n");

        HttpAddRequestHeadersA((HINTERNET)lpwhr, lpszCookies, strlen(lpszCookies),
                               HTTP_ADDREQ_FLAG_ADD);
        HeapFree(GetProcessHeap(), 0, lpszCookies);
    }
    HeapFree(GetProcessHeap(), 0, lpszUrl);



    if (hIC->lpfnStatusCB)
    {
        INTERNET_ASYNC_RESULT iar;

        iar.dwResult = (DWORD)lpwhr;
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
                      strlen(lpwhs->lpszServerName)+1);
    if (!GetAddress(lpwhs->lpszServerName, lpwhs->nServerPort,
                    &lpwhs->phostent, &lpwhs->socketAddress))
    {
        INTERNET_SetLastError(ERROR_INTERNET_NAME_NOT_RESOLVED);
        return FALSE;
    }

    SendAsyncCallback(hIC, hHttpSession, lpwhr->hdr.dwContext,
                      INTERNET_STATUS_NAME_RESOLVED,
                      &(lpwhs->socketAddress),
                      sizeof(struct sockaddr_in));

    TRACE("<--\n");
    return (HINTERNET) lpwhr;
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
    LPHTTPHEADERA lphttpHdr = NULL;
    BOOL bSuccess = FALSE;
    LPWININETHTTPREQA lpwhr = (LPWININETHTTPREQA) hHttpRequest;

    TRACE("(0x%08lx)--> %ld\n", dwInfoLevel, dwInfoLevel);

    if (NULL == lpwhr ||  lpwhr->hdr.htype != WH_HHTTPREQ)
    {
        INTERNET_SetLastError(ERROR_INTERNET_INCORRECT_HANDLE_TYPE);
        return FALSE;
    }

    /* Find requested header structure */
    if ((dwInfoLevel & ~HTTP_QUERY_MODIFIER_FLAGS_MASK) == HTTP_QUERY_CUSTOM)
    {
        INT index = HTTP_GetCustomHeaderIndex(lpwhr, (LPSTR)lpBuffer);

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
                  size += strlen(lpwhr->pCustHeaders[i].lpszField) +
                       strlen(lpwhr->pCustHeaders[i].lpszValue) + delim + 2;
	       }
           }

           /* Calculate the length of stadard request headers */
           for (i = 0; i <= HTTP_QUERY_MAX; i++)
           {
              if ((~lpwhr->StdHeaders[i].wFlags & HDR_ISREQUEST) && lpwhr->StdHeaders[i].lpszField &&
                   lpwhr->StdHeaders[i].lpszValue)
              {
                 size += strlen(lpwhr->StdHeaders[i].lpszField) +
                    strlen(lpwhr->StdHeaders[i].lpszValue) + delim + 2;
              }
           }
           size += delim;

           if (size + 1 > *lpdwBufferLength)
           {
              *lpdwBufferLength = size + 1;
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
                   cnt += sprintf((char*)lpBuffer + cnt, "%s: %s%s", lpwhr->StdHeaders[i].lpszField, lpwhr->StdHeaders[i].lpszValue,
                          index == HTTP_QUERY_RAW_HEADERS_CRLF ? "\r\n" : "\0");
               }
            }

            /* Append custom request heades */
            for (i = 0; i < lpwhr->nCustHeaders; i++)
            {
                if ((~lpwhr->pCustHeaders[i].wFlags & HDR_ISREQUEST) &&
						lpwhr->pCustHeaders[i].lpszField &&
						lpwhr->pCustHeaders[i].lpszValue)
                {
                   cnt += sprintf((char*)lpBuffer + cnt, "%s: %s%s",
                    lpwhr->pCustHeaders[i].lpszField, lpwhr->pCustHeaders[i].lpszValue,
					index == HTTP_QUERY_RAW_HEADERS_CRLF ? "\r\n" : "\0");
                }
            }

            strcpy((char*)lpBuffer + cnt, index == HTTP_QUERY_RAW_HEADERS_CRLF ? "\r\n" : "");

           *lpdwBufferLength = cnt + delim;
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
       *(int *)lpBuffer = atoi(lphttpHdr->lpszValue);
	   bSuccess = TRUE;
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
    }
    else if (dwInfoLevel & HTTP_QUERY_FLAG_COALESCE)
    {
	    if (*lpdwIndex >= lphttpHdr->wCount)
		{
	        INTERNET_SetLastError(ERROR_HTTP_HEADER_NOT_FOUND);
		}
	    else
	    {
	    /* Copy strncpy(lpBuffer, lphttpHdr[*lpdwIndex], len); */
            (*lpdwIndex)++;
	    }
    }
    else
    {
        INT len = strlen(lphttpHdr->lpszValue);

        if (len + 1 > *lpdwBufferLength)
        {
            *lpdwBufferLength = len + 1;
            INTERNET_SetLastError(ERROR_INSUFFICIENT_BUFFER);
            goto lend;
        }

        strncpy(lpBuffer, lphttpHdr->lpszValue, len);
        ((char*)lpBuffer)[len]=0;
        *lpdwBufferLength = len;
        bSuccess = TRUE;
    }

lend:
    TRACE("%d <--\n", bSuccess);
    return bSuccess;
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
    BOOL result;
    DWORD charLen=*lpdwBufferLength;
    char* tempBuffer=HeapAlloc(GetProcessHeap(), 0, charLen);
    result=HttpQueryInfoA(hHttpRequest, dwInfoLevel, tempBuffer, &charLen, lpdwIndex);
    if((dwInfoLevel & HTTP_QUERY_FLAG_NUMBER) ||
       (dwInfoLevel & HTTP_QUERY_FLAG_SYSTEMTIME))
    {
        memcpy(lpBuffer,tempBuffer,charLen);
    }
    else
    {
        int nChars=MultiByteToWideChar(CP_ACP,0, tempBuffer,charLen,lpBuffer,*lpdwBufferLength);
        *lpdwBufferLength=nChars;
    }
    HeapFree(GetProcessHeap(), 0, tempBuffer);
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
BOOL WINAPI HttpSendRequestA(HINTERNET hHttpRequest, LPCSTR lpszHeaders,
	DWORD dwHeaderLength, LPVOID lpOptional ,DWORD dwOptionalLength)
{
    LPWININETHTTPREQA lpwhr = (LPWININETHTTPREQA) hHttpRequest;
    LPWININETHTTPSESSIONA lpwhs = NULL;
    LPWININETAPPINFOA hIC = NULL;

    TRACE("(0x%08lx, %p (%s), %li, %p, %li)\n", (unsigned long)hHttpRequest,
            lpszHeaders, debugstr_a(lpszHeaders), dwHeaderLength, lpOptional, dwOptionalLength);

    if (NULL == lpwhr || lpwhr->hdr.htype != WH_HHTTPREQ)
    {
        INTERNET_SetLastError(ERROR_INTERNET_INCORRECT_HANDLE_TYPE);
	return FALSE;
    }

    lpwhs = (LPWININETHTTPSESSIONA) lpwhr->hdr.lpwhparent;
    if (NULL == lpwhs ||  lpwhs->hdr.htype != WH_HHTTPSESSION)
    {
        INTERNET_SetLastError(ERROR_INTERNET_INCORRECT_HANDLE_TYPE);
	return FALSE;
    }

    hIC = (LPWININETAPPINFOA) lpwhs->hdr.lpwhparent;
    if (NULL == hIC ||  hIC->hdr.htype != WH_HINIT)
    {
        INTERNET_SetLastError(ERROR_INTERNET_INCORRECT_HANDLE_TYPE);
	return FALSE;
    }

    if (hIC->hdr.dwFlags & INTERNET_FLAG_ASYNC)
    {
        WORKREQUEST workRequest;

        workRequest.asyncall = HTTPSENDREQUESTA;
        workRequest.HFTPSESSION = (DWORD)hHttpRequest;
        if (lpszHeaders)
            workRequest.LPSZHEADER = (DWORD)HTTP_strdup(lpszHeaders);
        else
            workRequest.LPSZHEADER = 0;
        workRequest.DWHEADERLENGTH = dwHeaderLength;
        workRequest.LPOPTIONAL = (DWORD)lpOptional;
        workRequest.DWOPTIONALLENGTH = dwOptionalLength;

        INTERNET_AsyncCall(&workRequest);
        /*
         * This is from windows.
         */
        SetLastError(ERROR_IO_PENDING);
        return 0;
    }
    else
    {
	return HTTP_HttpSendRequestA(hHttpRequest, lpszHeaders,
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
BOOL WINAPI HttpSendRequestW(HINTERNET hHttpRequest, LPCWSTR lpszHeaders,
	DWORD dwHeaderLength, LPVOID lpOptional ,DWORD dwOptionalLength)
{
    BOOL result;
    char* szHeaders=NULL;
    DWORD nLen=dwHeaderLength;
    if(lpszHeaders!=NULL)
    {
        nLen=WideCharToMultiByte(CP_ACP,0,lpszHeaders,dwHeaderLength,NULL,0,NULL,NULL);
        szHeaders=HeapAlloc(GetProcessHeap(),0,nLen);
        WideCharToMultiByte(CP_ACP,0,lpszHeaders,dwHeaderLength,szHeaders,nLen,NULL,NULL);
    }
    result=HttpSendRequestA(hHttpRequest, szHeaders, nLen, lpOptional, dwOptionalLength);
    if(szHeaders!=NULL)
        HeapFree(GetProcessHeap(),0,szHeaders);
    return result;
}

/***********************************************************************
 *           HTTP_HandleRedirect (internal)
 */
static BOOL HTTP_HandleRedirect(LPWININETHTTPREQA lpwhr, LPCSTR lpszUrl, LPCSTR lpszHeaders,
                                DWORD dwHeaderLength, LPVOID lpOptional, DWORD dwOptionalLength)
{
    LPWININETHTTPSESSIONA lpwhs = (LPWININETHTTPSESSIONA) lpwhr->hdr.lpwhparent;
    LPWININETAPPINFOA hIC = (LPWININETAPPINFOA) lpwhs->hdr.lpwhparent;
    char path[2048];
    if(lpszUrl[0]=='/')
    {
        /* if it's an absolute path, keep the same session info */
        strcpy(path,lpszUrl);
    }
    else if (NULL != hIC->lpszProxy && hIC->lpszProxy[0] != 0)
    {
        TRACE("Redirect through proxy\n");
        strcpy(path,lpszUrl);
    }
    else
    {
        URL_COMPONENTSA urlComponents;
        char protocol[32], hostName[MAXHOSTNAME], userName[1024];
        char password[1024], extra[1024];
        urlComponents.dwStructSize = sizeof(URL_COMPONENTSA);
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
        if(!InternetCrackUrlA(lpszUrl, strlen(lpszUrl), 0, &urlComponents))
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
        if (NULL != lpwhs->lpszServerName && strlen(lpwhs->lpszServerName))
            HTTP_ProcessHeader(lpwhr, HTTP_REFERER, lpwhs->lpszServerName,
                           HTTP_ADDHDR_FLAG_REQ|HTTP_ADDREQ_FLAG_REPLACE|
                           HTTP_ADDHDR_FLAG_ADD_IF_NEW);
#endif
        
        if (NULL != lpwhs->lpszServerName)
            HeapFree(GetProcessHeap(), 0, lpwhs->lpszServerName);
        lpwhs->lpszServerName = HTTP_strdup(hostName);
        if (NULL != lpwhs->lpszUserName)
            HeapFree(GetProcessHeap(), 0, lpwhs->lpszUserName);
        lpwhs->lpszUserName = HTTP_strdup(userName);
        lpwhs->nServerPort = urlComponents.nPort;

        if (NULL != lpwhr->lpszHostName)
            HeapFree(GetProcessHeap(), 0, lpwhr->lpszHostName);
        lpwhr->lpszHostName=HTTP_strdup(hostName);

        SendAsyncCallback(hIC, lpwhs, lpwhr->hdr.dwContext,
                      INTERNET_STATUS_RESOLVING_NAME,
                      lpwhs->lpszServerName,
                      strlen(lpwhs->lpszServerName)+1);

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
    if (strlen(path))
    {
        DWORD needed = 0;
        HRESULT rc;
        rc = UrlEscapeA(path, NULL, &needed, URL_ESCAPE_SPACES_ONLY);
        if (rc != E_POINTER)
            needed = strlen(path)+1;
        lpwhr->lpszPath = HeapAlloc(GetProcessHeap(), 0, needed);
        rc = UrlEscapeA(path, lpwhr->lpszPath, &needed,
                        URL_ESCAPE_SPACES_ONLY);
        if (rc)
        {
            ERR("Unable to escape string!(%s) (%ld)\n",path,rc);
            strcpy(lpwhr->lpszPath,path);
        }
    }

    return HttpSendRequestA((HINTERNET)lpwhr, lpszHeaders, dwHeaderLength, lpOptional, dwOptionalLength);
}

/***********************************************************************
 *           HTTP_HttpSendRequestA (internal)
 *
 * Sends the specified request to the HTTP server
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI HTTP_HttpSendRequestA(HINTERNET hHttpRequest, LPCSTR lpszHeaders,
	DWORD dwHeaderLength, LPVOID lpOptional ,DWORD dwOptionalLength)
{
    INT cnt;
    INT i;
    BOOL bSuccess = FALSE;
    LPSTR requestString = NULL;
    INT requestStringLen;
    INT responseLen;
    INT headerLength = 0;
    LPWININETHTTPREQA lpwhr = (LPWININETHTTPREQA) hHttpRequest;
    LPWININETHTTPSESSIONA lpwhs = NULL;
    LPWININETAPPINFOA hIC = NULL;
    BOOL loop_next = FALSE;
    int CustHeaderIndex;

    TRACE("--> 0x%08lx\n", (ULONG)hHttpRequest);

    /* Verify our tree of internet handles */
    if (NULL == lpwhr ||  lpwhr->hdr.htype != WH_HHTTPREQ)
    {
        INTERNET_SetLastError(ERROR_INTERNET_INCORRECT_HANDLE_TYPE);
	return FALSE;
    }

    lpwhs = (LPWININETHTTPSESSIONA) lpwhr->hdr.lpwhparent;
    if (NULL == lpwhs ||  lpwhs->hdr.htype != WH_HHTTPSESSION)
    {
        INTERNET_SetLastError(ERROR_INTERNET_INCORRECT_HANDLE_TYPE);
	return FALSE;
    }

    hIC = (LPWININETAPPINFOA) lpwhs->hdr.lpwhparent;
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
        TRACE("Going to url %s %s\n", debugstr_a(lpwhr->lpszHostName), debugstr_a(lpwhr->lpszPath));
        loop_next = FALSE;

        /* If we don't have a path we set it to root */
        if (NULL == lpwhr->lpszPath)
            lpwhr->lpszPath = HTTP_strdup("/");

        if(strncmp(lpwhr->lpszPath, "http://", sizeof("http://") -1) != 0
           && lpwhr->lpszPath[0] != '/') /* not an absolute path ?? --> fix it !! */
        {
            char *fixurl = HeapAlloc(GetProcessHeap(), 0, strlen(lpwhr->lpszPath) + 2);
            *fixurl = '/';
            strcpy(fixurl + 1, lpwhr->lpszPath);
            HeapFree( GetProcessHeap(), 0, lpwhr->lpszPath );
            lpwhr->lpszPath = fixurl;
        }

        /* Calculate length of request string */
        requestStringLen =
            strlen(lpwhr->lpszVerb) +
            strlen(lpwhr->lpszPath) +
            strlen(HTTPHEADER) +
            5; /* " \r\n\r\n" */

        /* Add length of passed headers */
        if (lpszHeaders)
        {
            headerLength = -1 == dwHeaderLength ?  strlen(lpszHeaders) : dwHeaderLength;
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
                requestStringLen += strlen(lpwhr->pCustHeaders[i].lpszField) +
                    strlen(lpwhr->pCustHeaders[i].lpszValue) + 4; /*: \r\n */
	    }
        }

        /* Calculate the length of standard request headers */
        for (i = 0; i <= HTTP_QUERY_MAX; i++)
        {
            if (lpwhr->StdHeaders[i].wFlags & HDR_ISREQUEST)
            {
                requestStringLen += strlen(lpwhr->StdHeaders[i].lpszField) +
                    strlen(lpwhr->StdHeaders[i].lpszValue) + 4; /*: \r\n */
            }
        }

        if (lpwhr->lpszHostName)
            requestStringLen += (strlen(HTTPHOSTHEADER) + strlen(lpwhr->lpszHostName));

        /* if there is optional data to send, add the length */
        if (lpOptional)
        {
            requestStringLen += dwOptionalLength;
        }

        /* Allocate string to hold entire request */
        requestString = HeapAlloc(GetProcessHeap(), 0, requestStringLen + 1);
        if (NULL == requestString)
        {
            INTERNET_SetLastError(ERROR_OUTOFMEMORY);
            goto lend;
        }

        /* Build request string */
        cnt = sprintf(requestString, "%s %s%s",
                      lpwhr->lpszVerb,
                      lpwhr->lpszPath,
                      HTTPHEADER);

        /* Append standard request headers */
        for (i = 0; i <= HTTP_QUERY_MAX; i++)
        {
            if (lpwhr->StdHeaders[i].wFlags & HDR_ISREQUEST)
            {
                cnt += sprintf(requestString + cnt, "\r\n%s: %s",
                               lpwhr->StdHeaders[i].lpszField, lpwhr->StdHeaders[i].lpszValue);
                TRACE("Adding header %s (%s)\n",lpwhr->StdHeaders[i].lpszField,lpwhr->StdHeaders[i].lpszValue);
            }
        }

        /* Append custom request heades */
        for (i = 0; i < lpwhr->nCustHeaders; i++)
        {
            if (lpwhr->pCustHeaders[i].wFlags & HDR_ISREQUEST)
            {
                cnt += sprintf(requestString + cnt, "\r\n%s: %s",
                               lpwhr->pCustHeaders[i].lpszField, lpwhr->pCustHeaders[i].lpszValue);
                TRACE("Adding custom header %s (%s)\n",lpwhr->pCustHeaders[i].lpszField,lpwhr->pCustHeaders[i].lpszValue);
            }
        }

        if (lpwhr->lpszHostName)
            cnt += sprintf(requestString + cnt, "%s%s", HTTPHOSTHEADER, lpwhr->lpszHostName);

        /* Append passed request headers */
        if (lpszHeaders)
        {
            strcpy(requestString + cnt, "\r\n");
            cnt += 2;
            strcpy(requestString + cnt, lpszHeaders);
            cnt += headerLength;
        }

        /* Set (header) termination string for request */
        if (memcmp((requestString + cnt) - 4, "\r\n\r\n", 4) != 0)
        { /* only add it if the request string doesn't already
             have the thing.. (could happen if the custom header
             added it */
                strcpy(requestString + cnt, "\r\n");
                cnt += 2;
        }
        else
            requestStringLen -= 2;
 
        /* if optional data, append it */
        if (lpOptional)
        {
            memcpy(requestString + cnt, lpOptional, dwOptionalLength);
            cnt += dwOptionalLength;
            /* we also have to decrease the expected string length by two,
             * since we won't be adding on those following \r\n's */
            requestStringLen -= 2;
        }
        else
        { /* if there is no optional data, add on another \r\n just to be safe */
                /* termination for request */
                strcpy(requestString + cnt, "\r\n");
                cnt += 2;
        }

        TRACE("(%s) len(%d)\n", requestString, requestStringLen);
        /* Send the request and store the results */
        if (!HTTP_OpenConnection(lpwhr))
            goto lend;

        SendAsyncCallback(hIC, hHttpRequest, lpwhr->hdr.dwContext,
                          INTERNET_STATUS_SENDING_REQUEST, NULL, 0);

        NETCON_send(&lpwhr->netConnection, requestString, requestStringLen,
                    0, &cnt);


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
        CustHeaderIndex = HTTP_GetCustomHeaderIndex(lpwhr, "Set-Cookie");
        if (CustHeaderIndex >= 0)
        {
            LPHTTPHEADERA setCookieHeader;
            int nPosStart = 0, nPosEnd = 0;

            setCookieHeader = &lpwhr->pCustHeaders[CustHeaderIndex];

            while (setCookieHeader->lpszValue[nPosEnd] != '\0')
            {
                LPSTR buf_cookie, cookie_name, cookie_data;
                LPSTR buf_url;
                LPSTR domain = NULL;
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
                    LPSTR lpszDomain = strstr(&setCookieHeader->lpszValue[nPosEnd], "domain=");
                    if (lpszDomain)
                    { /* they have specified their own domain, lets use it */
                        while (lpszDomain[nDomainPosEnd] != ';' && lpszDomain[nDomainPosEnd] != ',' &&
                               lpszDomain[nDomainPosEnd] != '\0')
                        {
                            nDomainPosEnd++;
                        }
                        nDomainPosStart = strlen("domain=");
                        nDomainLength = (nDomainPosEnd - nDomainPosStart) + 1;
                        domain = HeapAlloc(GetProcessHeap(), 0, nDomainLength + 1);
                        strncpy(domain, &lpszDomain[nDomainPosStart], nDomainLength);
                        domain[nDomainLength] = '\0';
                    }
                }
                if (setCookieHeader->lpszValue[nPosEnd] == '\0') break;
                buf_cookie = HeapAlloc(GetProcessHeap(), 0, (nPosEnd - nPosStart) + 1);
                strncpy(buf_cookie, &setCookieHeader->lpszValue[nPosStart], (nPosEnd - nPosStart));
                buf_cookie[(nPosEnd - nPosStart)] = '\0';
                TRACE("%s\n", buf_cookie);
                while (buf_cookie[nEqualPos] != '=' && buf_cookie[nEqualPos] != '\0')
                {
                    nEqualPos++;
                }
                if (buf_cookie[nEqualPos] == '\0' || buf_cookie[nEqualPos + 1] == '\0')
                {
                    HeapFree(GetProcessHeap(), 0, buf_cookie);
                    break;
                }

                cookie_name = HeapAlloc(GetProcessHeap(), 0, nEqualPos + 1);
                strncpy(cookie_name, buf_cookie, nEqualPos);
                cookie_name[nEqualPos] = '\0';
                cookie_data = &buf_cookie[nEqualPos + 1];


                buf_url = HeapAlloc(GetProcessHeap(), 0, strlen((domain ? domain : lpwhr->lpszHostName)) + strlen(lpwhr->lpszPath) + 9);
                sprintf(buf_url, "http://%s/", (domain ? domain : lpwhr->lpszHostName)); /* FIXME PATH!!! */
                InternetSetCookieA(buf_url, cookie_name, cookie_data);

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

    /* TODO: send notification for P3P header */
    
    if(!(hIC->hdr.dwFlags & INTERNET_FLAG_NO_AUTO_REDIRECT) && bSuccess)
    {
        DWORD dwCode,dwCodeLength=sizeof(DWORD),dwIndex=0;
        if(HttpQueryInfoA(hHttpRequest,HTTP_QUERY_FLAG_NUMBER|HTTP_QUERY_STATUS_CODE,&dwCode,&dwCodeLength,&dwIndex) &&
            (dwCode==302 || dwCode==301))
        {
            char szNewLocation[2048];
            DWORD dwBufferSize=2048;
            dwIndex=0;
            if(HttpQueryInfoA(hHttpRequest,HTTP_QUERY_LOCATION,szNewLocation,&dwBufferSize,&dwIndex))
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
HINTERNET HTTP_Connect(HINTERNET hInternet, LPCSTR lpszServerName,
	INTERNET_PORT nServerPort, LPCSTR lpszUserName,
	LPCSTR lpszPassword, DWORD dwFlags, DWORD dwContext)
{
    BOOL bSuccess = FALSE;
    LPWININETAPPINFOA hIC = NULL;
    LPWININETHTTPSESSIONA lpwhs = NULL;

    TRACE("-->\n");

    if (((LPWININETHANDLEHEADER)hInternet)->htype != WH_HINIT)
        goto lerror;

    hIC = (LPWININETAPPINFOA) hInternet;
    hIC->hdr.dwContext = dwContext;
    
    lpwhs = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WININETHTTPSESSIONA));
    if (NULL == lpwhs)
    {
        INTERNET_SetLastError(ERROR_OUTOFMEMORY);
	goto lerror;
    }

   /*
    * According to my tests. The name is not resolved until a request is sent
    */

    if (nServerPort == INTERNET_INVALID_PORT_NUMBER)
	nServerPort = INTERNET_DEFAULT_HTTP_PORT;

    lpwhs->hdr.htype = WH_HHTTPSESSION;
    lpwhs->hdr.lpwhparent = (LPWININETHANDLEHEADER)hInternet;
    lpwhs->hdr.dwFlags = dwFlags;
    lpwhs->hdr.dwContext = dwContext;
    if(hIC->lpszProxy && hIC->dwAccessType == INTERNET_OPEN_TYPE_PROXY) {
        if(strchr(hIC->lpszProxy, ' '))
            FIXME("Several proxies not implemented.\n");
        if(hIC->lpszProxyBypass)
            FIXME("Proxy bypass is ignored.\n");
    }
    if (NULL != lpszServerName)
        lpwhs->lpszServerName = HTTP_strdup(lpszServerName);
    if (NULL != lpszUserName)
        lpwhs->lpszUserName = HTTP_strdup(lpszUserName);
    lpwhs->nServerPort = nServerPort;

    if (hIC->lpfnStatusCB)
    {
        INTERNET_ASYNC_RESULT iar;

        iar.dwResult = (DWORD)lpwhs;
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
	lpwhs = NULL;
    }

/*
 * a INTERNET_STATUS_REQUEST_COMPLETE is NOT sent here as per my tests on
 * windows
 */

    TRACE("%p -->\n", hInternet);
    return (HINTERNET)lpwhs;
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
BOOL HTTP_OpenConnection(LPWININETHTTPREQA lpwhr)
{
    BOOL bSuccess = FALSE;
    LPWININETHTTPSESSIONA lpwhs;
    LPWININETAPPINFOA hIC = NULL;

    TRACE("-->\n");


    if (NULL == lpwhr ||  lpwhr->hdr.htype != WH_HHTTPREQ)
    {
        INTERNET_SetLastError(ERROR_INVALID_PARAMETER);
        goto lend;
    }

    lpwhs = (LPWININETHTTPSESSIONA)lpwhr->hdr.lpwhparent;

    hIC = (LPWININETAPPINFOA) lpwhs->hdr.lpwhparent;
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
BOOL HTTP_GetResponseHeaders(LPWININETHTTPREQA lpwhr)
{
    INT cbreaks = 0;
    CHAR buffer[MAX_REPLY_LEN];
    DWORD buflen = MAX_REPLY_LEN;
    BOOL bSuccess = FALSE;
    INT  rc = 0;
    CHAR value[MAX_FIELD_VALUE_LEN], field[MAX_FIELD_LEN];

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
    if (!NETCON_getNextLine(&lpwhr->netConnection, buffer, &buflen))
        goto lend;

    if (strncmp(buffer, "HTTP", 4) != 0)
        goto lend;

    buffer[12]='\0';
    HTTP_ProcessHeader(lpwhr, "Status", buffer+9, (HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE));

    /* Parse each response line */
    do
    {
	buflen = MAX_REPLY_LEN;
        if (NETCON_getNextLine(&lpwhr->netConnection, buffer, &buflen))
	{
            TRACE("got line %s, now interpretting\n", debugstr_a(buffer));
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
INT stripSpaces(LPCSTR lpszSrc, LPSTR lpszStart, INT *len)
{
    LPCSTR lpsztmp;
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
    strncpy(lpszStart, lpszSrc, *len);
    lpszStart[*len] = '\0';

    return *len;
}


BOOL HTTP_InterpretHttpHeader(LPSTR buffer, LPSTR field, INT fieldlen, LPSTR value, INT valuelen)
{
    CHAR *pd;
    BOOL bSuccess = FALSE;

    TRACE("\n");

    *field = '\0';
    *value = '\0';

    pd = strchr(buffer, ':');
    if (pd)
    {
	*pd = '\0';
	if (stripSpaces(buffer, field, &fieldlen) > 0)
	{
	    if (stripSpaces(pd+1, value, &valuelen) > 0)
		bSuccess = TRUE;
	}
    }

    TRACE("%d: field(%s) Value(%s)\n", bSuccess, field, value);
    return bSuccess;
}


/***********************************************************************
 *           HTTP_GetStdHeaderIndex (internal)
 *
 * Lookup field index in standard http header array
 *
 * FIXME: This should be stuffed into a hash table
 */
INT HTTP_GetStdHeaderIndex(LPCSTR lpszField)
{
    INT index = -1;

    if (!strcasecmp(lpszField, "Content-Length"))
        index = HTTP_QUERY_CONTENT_LENGTH;
    else if (!strcasecmp(lpszField,"Status"))
        index = HTTP_QUERY_STATUS_CODE;
    else if (!strcasecmp(lpszField,"Content-Type"))
        index = HTTP_QUERY_CONTENT_TYPE;
    else if (!strcasecmp(lpszField,"Last-Modified"))
        index = HTTP_QUERY_LAST_MODIFIED;
    else if (!strcasecmp(lpszField,"Location"))
        index = HTTP_QUERY_LOCATION;
    else if (!strcasecmp(lpszField,"Accept"))
        index = HTTP_QUERY_ACCEPT;
    else if (!strcasecmp(lpszField,"Referer"))
        index = HTTP_QUERY_REFERER;
    else if (!strcasecmp(lpszField,"Content-Transfer-Encoding"))
        index = HTTP_QUERY_CONTENT_TRANSFER_ENCODING;
    else if (!strcasecmp(lpszField,"Date"))
        index = HTTP_QUERY_DATE;
    else if (!strcasecmp(lpszField,"Server"))
        index = HTTP_QUERY_SERVER;
    else if (!strcasecmp(lpszField,"Connection"))
        index = HTTP_QUERY_CONNECTION;
    else if (!strcasecmp(lpszField,"ETag"))
        index = HTTP_QUERY_ETAG;
    else if (!strcasecmp(lpszField,"Accept-Ranges"))
        index = HTTP_QUERY_ACCEPT_RANGES;
    else if (!strcasecmp(lpszField,"Expires"))
        index = HTTP_QUERY_EXPIRES;
    else if (!strcasecmp(lpszField,"Mime-Version"))
        index = HTTP_QUERY_MIME_VERSION;
    else if (!strcasecmp(lpszField,"Pragma"))
        index = HTTP_QUERY_PRAGMA;
    else if (!strcasecmp(lpszField,"Cache-Control"))
        index = HTTP_QUERY_CACHE_CONTROL;
    else if (!strcasecmp(lpszField,"Content-Length"))
        index = HTTP_QUERY_CONTENT_LENGTH;
    else if (!strcasecmp(lpszField,"User-Agent"))
        index = HTTP_QUERY_USER_AGENT;
    else if (!strcasecmp(lpszField,"Proxy-Authenticate"))
        index = HTTP_QUERY_PROXY_AUTHENTICATE;
    else
    {
       TRACE("Couldn't find %s in standard header table\n", lpszField);
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

BOOL HTTP_ProcessHeader(LPWININETHTTPREQA lpwhr, LPCSTR field, LPCSTR value, DWORD dwModifier)
{
    LPHTTPHEADERA lphttpHdr = NULL;
    BOOL bSuccess = FALSE;
    INT index;

    TRACE("--> %s: %s - 0x%08x\n", field, value, (unsigned int)dwModifier);

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
            HTTPHEADERA hdr;

            hdr.lpszField = (LPSTR)field;
            hdr.lpszValue = (LPSTR)value;
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
            lphttpHdr->lpszField = HTTP_strdup(field);

            if (dwModifier & HTTP_ADDHDR_FLAG_REQ)
                lphttpHdr->wFlags |= HDR_ISREQUEST;
        }

        slen = strlen(value) + 1;
        lphttpHdr->lpszValue = HeapAlloc(GetProcessHeap(), 0, slen);
        if (lphttpHdr->lpszValue)
        {
            memcpy(lphttpHdr->lpszValue, value, slen);
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
            LPSTR lpsztmp;
            INT len;

            len = strlen(value);

            if (len <= 0)
            {
	        /* if custom header delete from array */
                HeapFree(GetProcessHeap(), 0, lphttpHdr->lpszValue);
                lphttpHdr->lpszValue = NULL;
                bSuccess = TRUE;
            }
            else
            {
                lpsztmp = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,  lphttpHdr->lpszValue, len+1);
                if (lpsztmp)
                {
                    lphttpHdr->lpszValue = lpsztmp;
                    strcpy(lpsztmp, value);
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
            LPSTR lpsztmp;
            CHAR ch = 0;
            INT len = 0;
            INT origlen = strlen(lphttpHdr->lpszValue);
            INT valuelen = strlen(value);

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

            lpsztmp = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,  lphttpHdr->lpszValue, len+1);
            if (lpsztmp)
            {
		/* FIXME: Increment lphttpHdr->wCount. Perhaps lpszValue should be an array */
                if (ch > 0)
                {
                    lphttpHdr->lpszValue[origlen] = ch;
                    origlen++;
                }

                memcpy(&lphttpHdr->lpszValue[origlen], value, valuelen);
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
VOID HTTP_CloseConnection(LPWININETHTTPREQA lpwhr)
{


    LPWININETHTTPSESSIONA lpwhs = NULL;
    LPWININETAPPINFOA hIC = NULL;

    TRACE("%p\n",lpwhr);

    lpwhs = (LPWININETHTTPSESSIONA) lpwhr->hdr.lpwhparent;
    hIC = (LPWININETAPPINFOA) lpwhs->hdr.lpwhparent;

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
void HTTP_CloseHTTPRequestHandle(LPWININETHTTPREQA lpwhr)
{
    int i;
    LPWININETHTTPSESSIONA lpwhs = NULL;
    LPWININETAPPINFOA hIC = NULL;

    TRACE("\n");

    if (NETCON_connected(&lpwhr->netConnection))
        HTTP_CloseConnection(lpwhr);

    lpwhs = (LPWININETHTTPSESSIONA) lpwhr->hdr.lpwhparent;
    hIC = (LPWININETAPPINFOA) lpwhs->hdr.lpwhparent;

    SendAsyncCallback(hIC, lpwhr, lpwhr->hdr.dwContext,
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
void HTTP_CloseHTTPSessionHandle(LPWININETHTTPSESSIONA lpwhs)
{
    LPWININETAPPINFOA hIC = NULL;
    TRACE("%p\n", lpwhs);

    hIC = (LPWININETAPPINFOA) lpwhs->hdr.lpwhparent;

    SendAsyncCallback(hIC, lpwhs, lpwhs->hdr.dwContext,
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
INT HTTP_GetCustomHeaderIndex(LPWININETHTTPREQA lpwhr, LPCSTR lpszField)
{
    INT index;

    TRACE("%s\n", lpszField);

    for (index = 0; index < lpwhr->nCustHeaders; index++)
    {
	if (!strcasecmp(lpwhr->pCustHeaders[index].lpszField, lpszField))
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
BOOL HTTP_InsertCustomHeader(LPWININETHTTPREQA lpwhr, LPHTTPHEADERA lpHdr)
{
    INT count;
    LPHTTPHEADERA lph = NULL;
    BOOL r = FALSE;

    TRACE("--> %s: %s\n", lpHdr->lpszField, lpHdr->lpszValue);
    count = lpwhr->nCustHeaders + 1;
    if (count > 1)
	lph = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lpwhr->pCustHeaders, sizeof(HTTPHEADERA) * count);
    else
	lph = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(HTTPHEADERA) * count);

    if (NULL != lph)
    {
	lpwhr->pCustHeaders = lph;
        lpwhr->pCustHeaders[count-1].lpszField = HTTP_strdup(lpHdr->lpszField);
        lpwhr->pCustHeaders[count-1].lpszValue = HTTP_strdup(lpHdr->lpszValue);
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
BOOL HTTP_DeleteCustomHeader(LPWININETHTTPREQA lpwhr, INT index)
{
    if( lpwhr->nCustHeaders <= 0 )
        return FALSE;
    if( lpwhr->nCustHeaders >= index )
        return FALSE;
    lpwhr->nCustHeaders--;

    memmove( &lpwhr->pCustHeaders[index], &lpwhr->pCustHeaders[index+1],
             (lpwhr->nCustHeaders - index)* sizeof(HTTPHEADERA) );
    memset( &lpwhr->pCustHeaders[lpwhr->nCustHeaders], 0, sizeof(HTTPHEADERA) );

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
