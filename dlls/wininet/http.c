/* 
 * Wininet - Http Implementation
 *
 * Copyright 1999 Corel Corporation
 *
 * Ulrich Czekalla
 *
 */

#include "windows.h"
#include "wininet.h"
#include "debugtools.h"
#include "winerror.h"
#include "heap.h"
#include "tchar.h"
#include "winsock.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "internet.h"

DEFAULT_DEBUG_CHANNEL(wininet)

#define HTTPHEADER " HTTP/1.0"
#define HTTPHOSTHEADER "\r\nHost: "
#define MAXHOSTNAME 100
#define MAX_FIELD_VALUE_LEN 256
#define MAX_FIELD_LEN 256


#define HTTP_REFERER	"Referer"
#define HTTP_ACCEPT		"Accept"

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
INT HTTP_InsertCustomHeader(LPWININETHTTPREQA lpwhr, LPHTTPHEADERA lpHdr);
INT HTTP_GetCustomHeaderIndex(LPWININETHTTPREQA lpwhr, LPCSTR lpszField);

/***********************************************************************
 *           HttpAddRequestHeadersA (WININET.68)
 *
 * Adds one or more HTTP header to the request handler
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
INTERNETAPI BOOL WINAPI HttpAddRequestHeadersA(HINTERNET hHttpRequest, 
	LPCSTR lpszHeader, DWORD dwHeaderLength, DWORD dwModifier)
{
    LPSTR lpszStart;
    LPSTR lpszEnd;
    LPSTR buffer;
    CHAR value[MAX_FIELD_VALUE_LEN], field[MAX_FIELD_LEN];
    BOOL bSuccess = FALSE;
    LPWININETHTTPREQA lpwhr = (LPWININETHTTPREQA) hHttpRequest;

    if (NULL == lpwhr ||  lpwhr->hdr.htype != WH_HHTTPREQ)
    {
        INTERNET_SetLastError(ERROR_INTERNET_INCORRECT_HANDLE_TYPE);
        return FALSE;
    }

    buffer = HEAP_strdupA(GetProcessHeap(), 0, lpszHeader);
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

        if (HTTP_InterpretHttpHeader(lpszStart, field, MAX_FIELD_LEN, value, MAX_FIELD_VALUE_LEN))
            bSuccess = HTTP_ProcessHeader(lpwhr, field, value, dwModifier | HTTP_ADDHDR_FLAG_REQ);

        lpszStart = lpszEnd + 2; /* Jump over \0\n */

    } while (bSuccess);

    HeapFree(GetProcessHeap(), 0, buffer);
    return bSuccess;
}


/***********************************************************************
 *           HttpOpenRequestA (WININET.72)
 *
 * Open a HTTP request handle
 *
 * RETURNS
 *    HINTERNET  a HTTP request handle on success
 *    NULL 	 on failure
 *
 */
INTERNETAPI HINTERNET WINAPI HttpOpenRequestA(HINTERNET hHttpSession,
	LPCSTR lpszVerb, LPCSTR lpszObjectName, LPCSTR lpszVersion,
	LPCSTR lpszReferrer , LPCSTR *lpszAcceptTypes, 
	DWORD dwFlags, DWORD dwContext)
{
    LPWININETHTTPSESSIONA lpwhs = (LPWININETHTTPSESSIONA) hHttpSession;
    LPWININETAPPINFOA hIC = NULL;

    TRACE("\n");

    if (NULL == lpwhs ||  lpwhs->hdr.htype != WH_HHTTPSESSION)
    {
        INTERNET_SetLastError(ERROR_INTERNET_INCORRECT_HANDLE_TYPE);
	return FALSE;
    }

    hIC = (LPWININETAPPINFOA) lpwhs->hdr.lpwhparent;

    if (hIC->hdr.dwFlags & INTERNET_FLAG_ASYNC)
    {
        WORKREQUEST workRequest;

	workRequest.asyncall = HTTPOPENREQUESTA;
	workRequest.HFTPSESSION = (DWORD)hHttpSession;
	workRequest.LPSZVERB = (DWORD)HEAP_strdupA(GetProcessHeap(), 0, lpszVerb);
	workRequest.LPSZOBJECTNAME = (DWORD)HEAP_strdupA(GetProcessHeap(), 0, lpszObjectName);
	workRequest.LPSZVERSION = (DWORD)HEAP_strdupA(GetProcessHeap(), 0, lpszVersion);
	workRequest.LPSZREFERRER = (DWORD)HEAP_strdupA(GetProcessHeap(), 0, lpszReferrer);
	workRequest.LPSZACCEPTTYPES = (DWORD)lpszAcceptTypes;
	workRequest.DWFLAGS = dwFlags;
	workRequest.DWCONTEXT = dwContext;

	return (HINTERNET)INTERNET_AsyncCall(&workRequest);
    }
    else
    {
	return HTTP_HttpOpenRequestA(hHttpSession, lpszVerb, lpszObjectName, 
		lpszVersion, lpszReferrer, lpszAcceptTypes, dwFlags, dwContext);
    }
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
INTERNETAPI HINTERNET WINAPI HTTP_HttpOpenRequestA(HINTERNET hHttpSession,
	LPCSTR lpszVerb, LPCSTR lpszObjectName, LPCSTR lpszVersion,
	LPCSTR lpszReferrer , LPCSTR *lpszAcceptTypes, 
	DWORD dwFlags, DWORD dwContext)
{
    LPWININETHTTPSESSIONA lpwhs = (LPWININETHTTPSESSIONA) hHttpSession;
    LPWININETAPPINFOA hIC = NULL;
    LPWININETHTTPREQA lpwhr;

    TRACE("\n");

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

    if (NULL != lpszObjectName && strlen(lpszObjectName))
    	lpwhr->lpszPath = HEAP_strdupA(GetProcessHeap(), 0, lpszObjectName);

    if (NULL != lpszReferrer && strlen(lpszReferrer))
        HTTP_ProcessHeader(lpwhr, HTTP_REFERER, lpszReferrer, HTTP_ADDHDR_FLAG_COALESCE);

    //! FIXME
    if (NULL != lpszAcceptTypes && strlen(*lpszAcceptTypes))
        HTTP_ProcessHeader(lpwhr, HTTP_ACCEPT, *lpszAcceptTypes, HTTP_ADDHDR_FLAG_COALESCE);

    if (NULL == lpszVerb)
        lpwhr->lpszVerb = HEAP_strdupA(GetProcessHeap(), 0, "GET");
    else if (strlen(lpszVerb))
        lpwhr->lpszVerb = HEAP_strdupA(GetProcessHeap(), 0, lpszVerb);

    if (NULL != lpszReferrer)
    {
        char buf[MAXHOSTNAME];
        URL_COMPONENTSA UrlComponents;

        UrlComponents.lpszExtraInfo = NULL;
        UrlComponents.lpszPassword = NULL;
        UrlComponents.lpszScheme = NULL;
        UrlComponents.lpszUrlPath = NULL;
        UrlComponents.lpszUserName = NULL;
        UrlComponents.lpszHostName = buf;
        UrlComponents.dwHostNameLength = MAXHOSTNAME;

        InternetCrackUrlA(lpszReferrer, 0, 0, &UrlComponents);
        if (strlen(UrlComponents.lpszHostName))
            lpwhr->lpszHostName = HEAP_strdupA(GetProcessHeap(), 0, UrlComponents.lpszHostName);
    }

    if (hIC->lpfnStatusCB)
    {
        INTERNET_ASYNC_RESULT iar;

        iar.dwResult = (DWORD)lpwhr;
        iar.dwError = ERROR_SUCCESS;

        hIC->lpfnStatusCB(hHttpSession, dwContext, INTERNET_STATUS_HANDLE_CREATED,
             &iar, sizeof(INTERNET_ASYNC_RESULT));
    }

    if (hIC->hdr.dwFlags & INTERNET_FLAG_ASYNC && hIC->lpfnStatusCB)
    {
        INTERNET_ASYNC_RESULT iar;
	       
        iar.dwResult = (DWORD)lpwhr;
        iar.dwError = lpwhr ? ERROR_SUCCESS : INTERNET_GetLastError();
        hIC->lpfnStatusCB(hHttpSession, dwContext, INTERNET_STATUS_REQUEST_COMPLETE,
            &iar, sizeof(INTERNET_ASYNC_RESULT));
    }

    return (HINTERNET) lpwhr;
}


/***********************************************************************
 *           HttpQueryInfoA (WININET.74)
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
                   cnt += sprintf(lpBuffer + cnt, "%s: %s%s", lpwhr->StdHeaders[i].lpszField, lpwhr->StdHeaders[i].lpszValue,
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
                   cnt += sprintf(lpBuffer + cnt, "%s: %s%s", 
                    lpwhr->pCustHeaders[i].lpszField, lpwhr->pCustHeaders[i].lpszValue,
					index == HTTP_QUERY_RAW_HEADERS_CRLF ? "\r\n" : "\0");
                }
            }

            strcpy(lpBuffer + cnt, index == HTTP_QUERY_RAW_HEADERS_CRLF ? "\r\n" : "");

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
	    //! Copy strncpy(lpBuffer, lphttpHdr[*lpdwIndex], len);
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
        *lpdwBufferLength = len;
        bSuccess = TRUE; 
    }

lend:
    TRACE("%d <--\n", bSuccess);
    return bSuccess;
}

/***********************************************************************
 *           HttpSendRequestA (WININET.76)
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

    TRACE("0x%08lx\n", (unsigned long)hHttpRequest);

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
	workRequest.LPSZHEADER = (DWORD)HEAP_strdupA(GetProcessHeap(), 0, lpszHeaders);
	workRequest.DWHEADERLENGTH = dwHeaderLength;
	workRequest.LPOPTIONAL = (DWORD)lpOptional;
	workRequest.DWOPTIONALLENGTH = dwOptionalLength;

	return INTERNET_AsyncCall(&workRequest);
    }
    else
    {
	return HTTP_HttpSendRequestA(hHttpRequest, lpszHeaders, 
		dwHeaderLength, lpOptional, dwOptionalLength);
    }
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
    INT headerLength = 0;
    LPWININETHTTPREQA lpwhr = (LPWININETHTTPREQA) hHttpRequest;
    LPWININETHTTPSESSIONA lpwhs = NULL;
    LPWININETAPPINFOA hIC = NULL;

    TRACE("0x%08lx\n", (ULONG)hHttpRequest);

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

    /* If we don't have a path we set it to root */
    if (NULL == lpwhr->lpszPath)
        lpwhr->lpszPath = HEAP_strdupA(GetProcessHeap(), 0, "/");

    /* Calculate length of request string */
    requestStringLen = 
        strlen(lpwhr->lpszVerb) +
        strlen(lpwhr->lpszPath) +
        (lpwhr->lpszHostName ? (strlen(HTTPHOSTHEADER) + strlen(lpwhr->lpszHostName)) : 0) +
        strlen(HTTPHEADER) +
        5; /* " \r\n\r\n" */

    /* Add length of passed headers */
    if (lpszHeaders)
    {
        headerLength = -1 == dwHeaderLength ?  strlen(lpszHeaders) : dwHeaderLength; 
        requestStringLen += headerLength +  2; /* \r\n */
    }

    /* Calculate length of custom reuqest headers */
    for (i = 0; i < lpwhr->nCustHeaders; i++)
    {
	    if (lpwhr->pCustHeaders[i].wFlags & HDR_ISREQUEST)
	    {
            requestStringLen += strlen(lpwhr->pCustHeaders[i].lpszField) + 
                strlen(lpwhr->pCustHeaders[i].lpszValue) + 4; /*: \r\n */
	    }
    }

    /* Calculate the length of stadard request headers */
    for (i = 0; i <= HTTP_QUERY_MAX; i++)
    {
       if (lpwhr->StdHeaders[i].wFlags & HDR_ISREQUEST)
       {
          requestStringLen += strlen(lpwhr->StdHeaders[i].lpszField) + 
             strlen(lpwhr->StdHeaders[i].lpszValue) + 4; /*: \r\n */
       }
    }

    /* Allocate string to hold entire request */
    requestString = HeapAlloc(GetProcessHeap(), 0, requestStringLen + 1);
    if (NULL == requestString)
    {
        INTERNET_SetLastError(ERROR_OUTOFMEMORY);
        goto lend;
    }

    /* Build request string */
    cnt = sprintf(requestString, "%s %s%s%s",
        lpwhr->lpszVerb,
        lpwhr->lpszPath,
        lpwhr->lpszHostName ? (HTTPHEADER HTTPHOSTHEADER) : HTTPHEADER,
        lpwhr->lpszHostName ? lpwhr->lpszHostName : "");

    /* Append standard request heades */
    for (i = 0; i <= HTTP_QUERY_MAX; i++)
    {
       if (lpwhr->StdHeaders[i].wFlags & HDR_ISREQUEST)
       {
           cnt += sprintf(requestString + cnt, "\r\n%s: %s", 
               lpwhr->StdHeaders[i].lpszField, lpwhr->StdHeaders[i].lpszValue);
       }
    }

    /* Append custom request heades */
    for (i = 0; i < lpwhr->nCustHeaders; i++)
    {
       if (lpwhr->pCustHeaders[i].wFlags & HDR_ISREQUEST)
       {
           cnt += sprintf(requestString + cnt, "\r\n%s: %s", 
               lpwhr->pCustHeaders[i].lpszField, lpwhr->pCustHeaders[i].lpszValue);
       }
    }

    /* Append passed request headers */
    if (lpszHeaders)
    {
        strcpy(requestString + cnt, "\r\n");
        cnt += 2;
        strcpy(requestString + cnt, lpszHeaders);
        cnt += headerLength;
    }

    /* Set termination string for request */
    strcpy(requestString + cnt, "\r\n\r\n");

    if (hIC->lpfnStatusCB)
        hIC->lpfnStatusCB(hHttpRequest, lpwhr->hdr.dwContext, INTERNET_STATUS_SENDING_REQUEST, NULL, 0);

    TRACE("(%s) len(%d)\n", requestString, requestStringLen);
    /* Send the request and store the results */
    if (!HTTP_OpenConnection(lpwhr))
        goto lend;

    cnt = INTERNET_WriteDataToStream(lpwhr->nSocketFD, requestString, requestStringLen);

    if (cnt < 0)
        goto lend;

    if (HTTP_GetResponseHeaders(lpwhr))
	    bSuccess = TRUE;

lend:

    if (requestString)
        HeapFree(GetProcessHeap(), 0, requestString);

    if (hIC->hdr.dwFlags & INTERNET_FLAG_ASYNC  && hIC->lpfnStatusCB)
    {
        INTERNET_ASYNC_RESULT iar;
	       
        iar.dwResult = (DWORD)bSuccess;
        iar.dwError = bSuccess ? ERROR_SUCCESS : INTERNET_GetLastError();
        hIC->lpfnStatusCB(hHttpRequest, lpwhr->hdr.dwContext, INTERNET_STATUS_REQUEST_COMPLETE,
            &iar, sizeof(INTERNET_ASYNC_RESULT));
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

    TRACE("\n");

    if (((LPWININETHANDLEHEADER)hInternet)->htype != WH_HINIT)
        goto lerror;

    hIC = (LPWININETAPPINFOA) hInternet;

    lpwhs = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WININETHTTPSESSIONA));
    if (NULL == lpwhs)
    {
        INTERNET_SetLastError(ERROR_OUTOFMEMORY);
	goto lerror;
    }

    if (hIC->lpfnStatusCB)
        hIC->lpfnStatusCB(hInternet, dwContext, INTERNET_STATUS_RESOLVING_NAME,
           lpszServerName, strlen(lpszServerName));

    if (nServerPort == INTERNET_INVALID_PORT_NUMBER)
	nServerPort = INTERNET_DEFAULT_HTTP_PORT;

    if (!GetAddress(lpszServerName, nServerPort, &lpwhs->phostent, &lpwhs->socketAddress))
    {
            INTERNET_SetLastError(ERROR_INTERNET_NAME_NOT_RESOLVED);
            goto lerror;
    }

    if (hIC->lpfnStatusCB)
        hIC->lpfnStatusCB(hInternet, dwContext, INTERNET_STATUS_NAME_RESOLVED,
           lpszServerName, strlen(lpszServerName));

    lpwhs->hdr.htype = WH_HHTTPSESSION;
    lpwhs->hdr.lpwhparent = (LPWININETHANDLEHEADER)hInternet;
    lpwhs->hdr.dwFlags = dwFlags;
    lpwhs->hdr.dwContext = dwContext;
    if (NULL != lpszServerName)
        lpwhs->lpszServerName = HEAP_strdupA(GetProcessHeap(), 0, lpszServerName);
    if (NULL != lpszUserName)
        lpwhs->lpszUserName = HEAP_strdupA(GetProcessHeap(), 0, lpszUserName);
    lpwhs->nServerPort = nServerPort;

    if (hIC->lpfnStatusCB)
    {
        INTERNET_ASYNC_RESULT iar;

        iar.dwResult = (DWORD)lpwhs;
        iar.dwError = ERROR_SUCCESS;

        hIC->lpfnStatusCB(hInternet, dwContext, INTERNET_STATUS_HANDLE_CREATED,
             &iar, sizeof(INTERNET_ASYNC_RESULT));
    }

    bSuccess = TRUE;

lerror:
    if (!bSuccess && lpwhs)
    {
        HeapFree(GetProcessHeap(), 0, lpwhs);
	lpwhs = NULL;
    }

    if (hIC->hdr.dwFlags & INTERNET_FLAG_ASYNC && hIC->lpfnStatusCB)
    {
        INTERNET_ASYNC_RESULT iar;
	       
        iar.dwResult = (DWORD)lpwhs;
        iar.dwError = bSuccess ? ERROR_SUCCESS : INTERNET_GetLastError();
        hIC->lpfnStatusCB(hInternet, dwContext, INTERNET_STATUS_REQUEST_COMPLETE,
            &iar, sizeof(INTERNET_ASYNC_RESULT));
    }
TRACE("<--\n");
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
    INT result;
    LPWININETHTTPSESSIONA lpwhs;

    TRACE("\n");

    if (NULL == lpwhr ||  lpwhr->hdr.htype != WH_HHTTPREQ)
    {
        INTERNET_SetLastError(ERROR_INVALID_PARAMETER);
        goto lend;
    }

    lpwhs = (LPWININETHTTPSESSIONA)lpwhr->hdr.lpwhparent;

    lpwhr->nSocketFD = socket(lpwhs->phostent->h_addrtype,SOCK_STREAM,0);
    if (INVALID_SOCKET == lpwhr->nSocketFD)
    {
	WARN("Socket creation failed\n");
        goto lend;
    }

    result = connect(lpwhr->nSocketFD, (struct sockaddr *)&lpwhs->socketAddress,
        sizeof(lpwhs->socketAddress));

    if (SOCKET_ERROR == result)
    {
       WARN("Unable to connect to host: %d\n", errno);
       goto lend;
    }

    bSuccess = TRUE;

lend:
    TRACE(": %d\n", bSuccess);
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
    CHAR value[MAX_FIELD_VALUE_LEN], field[MAX_FIELD_LEN];

    TRACE("\n");

    if (INVALID_SOCKET == lpwhr->nSocketFD)
        goto lend;

    /* 
     * We should first receive 'HTTP/1.x nnn' where nnn is the status code.
     */
    if (!INTERNET_GetNextLine(lpwhr->nSocketFD, buffer, &buflen))
        goto lend;

    if (strncmp(buffer, "HTTP", 4) != 0)
        goto lend;
	
    buffer[12]='\0';
    HTTP_ProcessHeader(lpwhr, "Status", buffer+9, (HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE));

    /* Parse each response line */
    do
    {
	buflen = MAX_REPLY_LEN;
	if (INTERNET_GetNextLine(lpwhr->nSocketFD, buffer, &buflen))
	{
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

    return bSuccess;
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
 * Lookup field index in stadard http header array
 *
 * FIXME: This should be stuffed into a hash table
 */
INT HTTP_GetStdHeaderIndex(LPCSTR lpszField)
{
    INT index = -1;

    if (!_stricmp(lpszField, "Content-Length"))
        index = HTTP_QUERY_CONTENT_LENGTH;
    else if (!_stricmp(lpszField,"Status"))
        index = HTTP_QUERY_STATUS_CODE;
    else if (!_stricmp(lpszField,"Content-Type"))
        index = HTTP_QUERY_CONTENT_TYPE;
    else if (!_stricmp(lpszField,"Last-Modified"))
        index = HTTP_QUERY_LAST_MODIFIED;
    else if (!_stricmp(lpszField,"Location"))
        index = HTTP_QUERY_LOCATION;
    else if (!_stricmp(lpszField,"Accept"))
        index = HTTP_QUERY_ACCEPT;
    else if (!_stricmp(lpszField,"Referer"))
        index = HTTP_QUERY_REFERER;
    else if (!_stricmp(lpszField,"Content-Transfer-Encoding"))
        index = HTTP_QUERY_CONTENT_TRANSFER_ENCODING;
    else
    {
       FIXME("Couldn't find %s in standard header table\n", lpszField);
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

    TRACE("%s:%s - 0x%08x\n", field, value, (unsigned int)dwModifier);

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

	    hdr.lpszField = field;
	    hdr.lpszValue = value;
	    hdr.wFlags = hdr.wCount = 0;

            if (dwModifier & HTTP_ADDHDR_FLAG_REQ)
                hdr.wFlags |= HDR_ISREQUEST;

	    index = HTTP_InsertCustomHeader(lpwhr, &hdr);
	    return index >= 0;
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
            lphttpHdr->lpszField = HEAP_strdupA(GetProcessHeap(), 0, field);

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
		//! if custom header delete from array
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

            len = origlen + valuelen + (ch > 0) ? 1 : 0;

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
                INTERNET_SetLastError(ERROR_OUTOFMEMORY);
            }
        }
    }

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
	if (lpwhr->nSocketFD != INVALID_SOCKET)
	{
		close(lpwhr->nSocketFD);
		lpwhr->nSocketFD = INVALID_SOCKET;
	}
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

    TRACE("\n");

    if (lpwhr->nSocketFD != INVALID_SOCKET)
        HTTP_CloseConnection(lpwhr);

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
    TRACE("\n");

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
	if (!_stricmp(lpwhr->pCustHeaders[index].lpszField, lpszField))
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
INT HTTP_InsertCustomHeader(LPWININETHTTPREQA lpwhr, LPHTTPHEADERA lpHdr)
{
    INT count;
    LPHTTPHEADERA lph = NULL;

    TRACE("%s: %s\n", lpHdr->lpszField, lpHdr->lpszValue);
    count = lpwhr->nCustHeaders + 1;
    if (count > 1)
	lph = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lpwhr->pCustHeaders, sizeof(HTTPHEADERA) * count);
    else
	lph = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(HTTPHEADERA) * count);

    if (NULL != lph)
    {
	lpwhr->pCustHeaders = lph;
        lpwhr->pCustHeaders[count-1].lpszField = HEAP_strdupA(GetProcessHeap(), 0, lpHdr->lpszField);
        lpwhr->pCustHeaders[count-1].lpszValue = HEAP_strdupA(GetProcessHeap(), 0, lpHdr->lpszValue);
        lpwhr->pCustHeaders[count-1].wFlags = lpHdr->wFlags;
        lpwhr->pCustHeaders[count-1].wCount= lpHdr->wCount;
	lpwhr->nCustHeaders++;
    }
    else
    {
        INTERNET_SetLastError(ERROR_OUTOFMEMORY);
	count = 0;
    }

    TRACE("%d <--\n", count-1);
    return count - 1;
}


/***********************************************************************
 *           HTTP_DeleteCustomHeader (internal)
 *
 * Delete header from array
 *
 */
BOOL HTTP_DeleteCustomHeader(INT index)
{
    TRACE("\n");
    return FALSE;
}
