/*
 * Wininet
 *
 * Copyright 1999 Corel Corporation
 * Copyright 2002 CodeWeavers Inc.
 * Copyright 2002 Jaco Greeff
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

#define MAXHOSTNAME 100 /* from http.c */

#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#include <stdlib.h>
#include <ctype.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "wininet.h"
#include "winnls.h"
#include "wine/debug.h"
#include "winerror.h"
#define NO_SHLWAPI_STREAM
#include "shlwapi.h"

#include "wine/exception.h"
#include "excpt.h"

#include "internet.h"

#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(wininet);

#define MAX_IDLE_WORKER 1000*60*1
#define MAX_WORKER_THREADS 10
#define RESPONSE_TIMEOUT        30

#define GET_HWININET_FROM_LPWININETFINDNEXT(lpwh) \
(LPWININETAPPINFOA)(((LPWININETFTPSESSIONA)(lpwh->hdr.lpwhparent))->hdr.lpwhparent)

/* filter for page-fault exceptions */
static WINE_EXCEPTION_FILTER(page_fault)
{
    if (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ||
        GetExceptionCode() == EXCEPTION_PRIV_INSTRUCTION)
        return EXCEPTION_EXECUTE_HANDLER;
    return EXCEPTION_CONTINUE_SEARCH;
}

typedef struct
{
    DWORD  dwError;
    CHAR   response[MAX_REPLY_LEN];
} WITHREADERROR, *LPWITHREADERROR;

BOOL WINAPI INTERNET_FindNextFileA(HINTERNET hFind, LPVOID lpvFindData);
VOID INTERNET_ExecuteWork();

DWORD g_dwTlsErrIndex = TLS_OUT_OF_INDEXES;
DWORD dwNumThreads;
DWORD dwNumIdleThreads;
DWORD dwNumJobs;
HANDLE hEventArray[2];
#define hQuitEvent hEventArray[0]
#define hWorkEvent hEventArray[1]
CRITICAL_SECTION csQueue;
LPWORKREQUEST lpHeadWorkQueue;
LPWORKREQUEST lpWorkQueueTail;

/***********************************************************************
 * DllMain [Internal] Initializes the internal 'WININET.DLL'.
 *
 * PARAMS
 *     hinstDLL    [I] handle to the DLL's instance
 *     fdwReason   [I]
 *     lpvReserved [I] reserved, must be NULL
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL WINAPI DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("%p,%lx,%p\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:

            g_dwTlsErrIndex = TlsAlloc();

	    if (g_dwTlsErrIndex == TLS_OUT_OF_INDEXES)
		return FALSE;

	    hQuitEvent = CreateEventA(0, TRUE, FALSE, NULL);
	    hWorkEvent = CreateEventA(0, FALSE, FALSE, NULL);
	    InitializeCriticalSection(&csQueue);

            dwNumThreads = 0;
            dwNumIdleThreads = 0;
	    dwNumJobs = 0;

        case DLL_THREAD_ATTACH:
	    {
                LPWITHREADERROR lpwite = HeapAlloc(GetProcessHeap(), 0, sizeof(WITHREADERROR));
		if (NULL == lpwite)
                    return FALSE;

                TlsSetValue(g_dwTlsErrIndex, (LPVOID)lpwite);
	    }
	    break;

        case DLL_THREAD_DETACH:
	    if (g_dwTlsErrIndex != TLS_OUT_OF_INDEXES)
			{
				LPVOID lpwite = TlsGetValue(g_dwTlsErrIndex);
				if (lpwite)
                   HeapFree(GetProcessHeap(), 0, lpwite);
			}
	    break;

        case DLL_PROCESS_DETACH:

	    if (g_dwTlsErrIndex != TLS_OUT_OF_INDEXES)
	    {
	        HeapFree(GetProcessHeap(), 0, TlsGetValue(g_dwTlsErrIndex));
	        TlsFree(g_dwTlsErrIndex);
	    }

	    SetEvent(hQuitEvent);

	    CloseHandle(hQuitEvent);
	    CloseHandle(hWorkEvent);
	    DeleteCriticalSection(&csQueue);
            break;
    }

    return TRUE;
}


/***********************************************************************
 *           InternetInitializeAutoProxyDll   (WININET.@)
 *
 * Setup the internal proxy
 *
 * PARAMETERS
 *     dwReserved
 *
 * RETURNS
 *     FALSE on failure
 *
 */
BOOL WINAPI InternetInitializeAutoProxyDll(DWORD dwReserved)
{
    FIXME("STUB\n");
    INTERNET_SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *           DetectAutoProxyUrl   (WININET.@)
 *
 * Auto detect the proxy url
 *
 * RETURNS
 *     FALSE on failure
 *
 */
BOOL WINAPI DetectAutoProxyUrl(LPSTR lpszAutoProxyUrl,
	DWORD dwAutoProxyUrlLength, DWORD dwDetectFlags)
{
    FIXME("STUB\n");
    INTERNET_SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/***********************************************************************
 *           InternetOpenA   (WININET.@)
 *
 * Per-application initialization of wininet
 *
 * RETURNS
 *    HINTERNET on success
 *    NULL on failure
 *
 */
HINTERNET WINAPI InternetOpenA(LPCSTR lpszAgent, DWORD dwAccessType,
    LPCSTR lpszProxy, LPCSTR lpszProxyBypass, DWORD dwFlags)
{
    LPWININETAPPINFOA lpwai = NULL;

    TRACE("(%s, %li, %s, %s, %li)\n", debugstr_a(lpszAgent), dwAccessType,
	 debugstr_a(lpszProxy), debugstr_a(lpszProxyBypass), dwFlags);

    /* Clear any error information */
    INTERNET_SetLastError(0);

    lpwai = HeapAlloc(GetProcessHeap(), 0, sizeof(WININETAPPINFOA));
    if (NULL == lpwai)
        INTERNET_SetLastError(ERROR_OUTOFMEMORY);
    else
    {
        memset(lpwai, 0, sizeof(WININETAPPINFOA));
        lpwai->hdr.htype = WH_HINIT;
        lpwai->hdr.lpwhparent = NULL;
        lpwai->hdr.dwFlags = dwFlags;
        if (NULL != lpszAgent)
        {
            if ((lpwai->lpszAgent = HeapAlloc( GetProcessHeap(),0,strlen(lpszAgent)+1)))
                strcpy( lpwai->lpszAgent, lpszAgent );
        }
        if(dwAccessType == INTERNET_OPEN_TYPE_PRECONFIG)
        {
            HKEY key;
            if (!RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings", &key))
            {
                DWORD keytype, len, enabled;
                RegQueryValueExA(key, "ProxyEnable", NULL, NULL, (BYTE*)&enabled, NULL);
                if(enabled)
                {
                    if(!RegQueryValueExA(key, "ProxyServer", NULL, &keytype, NULL, &len) && len && keytype == REG_SZ)
                    {
                        lpwai->lpszProxy=HeapAlloc( GetProcessHeap(), 0, len+1 );
                        RegQueryValueExA(key, "ProxyServer", NULL, &keytype, (BYTE*)lpwai->lpszProxy, &len);
						TRACE("Proxy = %s\n", lpwai->lpszProxy);
                        dwAccessType = INTERNET_OPEN_TYPE_PROXY;
                    }
                }
                else
                {
                    TRACE("Proxy is not enabled.\n");
                }
                RegCloseKey(key);
            }
        }
        else if (NULL != lpszProxy)
        {
            if ((lpwai->lpszProxy = HeapAlloc( GetProcessHeap(), 0, strlen(lpszProxy)+1 )))
                strcpy( lpwai->lpszProxy, lpszProxy );
        }
        if (NULL != lpszProxyBypass)
        {
            if ((lpwai->lpszProxyBypass = HeapAlloc( GetProcessHeap(), 0, strlen(lpszProxyBypass)+1)))
                strcpy( lpwai->lpszProxyBypass, lpszProxyBypass );
        }
        lpwai->dwAccessType = dwAccessType;
    }

    TRACE("returning %p\n", (HINTERNET)lpwai);
    return (HINTERNET)lpwai;
}


/***********************************************************************
 *           InternetOpenW   (WININET.@)
 *
 * Per-application initialization of wininet
 *
 * RETURNS
 *    HINTERNET on success
 *    NULL on failure
 *
 */
HINTERNET WINAPI InternetOpenW(LPCWSTR lpszAgent, DWORD dwAccessType,
    LPCWSTR lpszProxy, LPCWSTR lpszProxyBypass, DWORD dwFlags)
{
    HINTERNET rc = (HINTERNET)NULL;
    INT lenAgent = lstrlenW(lpszAgent)+1;
    INT lenProxy = lstrlenW(lpszProxy)+1;
    INT lenBypass = lstrlenW(lpszProxyBypass)+1;
    CHAR *szAgent = (CHAR *)HeapAlloc(GetProcessHeap(), 0, lenAgent*sizeof(CHAR));
    CHAR *szProxy = (CHAR *)HeapAlloc(GetProcessHeap(), 0, lenProxy*sizeof(CHAR));
    CHAR *szBypass = (CHAR *)HeapAlloc(GetProcessHeap(), 0, lenBypass*sizeof(CHAR));

    if (!szAgent || !szProxy || !szBypass)
    {
        if (szAgent)
            free(szAgent);
        if (szProxy)
            free(szProxy);
        if (szBypass)
            free(szBypass);
        return (HINTERNET)NULL;
    }

    WideCharToMultiByte(CP_ACP, -1, lpszAgent, -1, szAgent, lenAgent,
        NULL, NULL);
    WideCharToMultiByte(CP_ACP, -1, lpszProxy, -1, szProxy, lenProxy,
        NULL, NULL);
    WideCharToMultiByte(CP_ACP, -1, lpszProxyBypass, -1, szBypass, lenBypass,
        NULL, NULL);

    rc = InternetOpenA(szAgent, dwAccessType, szProxy, szBypass, dwFlags);

    HeapFree(GetProcessHeap(), 0, szAgent);
    HeapFree(GetProcessHeap(), 0, szProxy);
    HeapFree(GetProcessHeap(), 0, szBypass);

    return rc;
}

/***********************************************************************
 *           InternetGetLastResponseInfoA (WININET.@)
 *
 * Return last wininet error description on the calling thread
 *
 * RETURNS
 *    TRUE on success of writting to buffer
 *    FALSE on failure
 *
 */
BOOL WINAPI InternetGetLastResponseInfoA(LPDWORD lpdwError,
    LPSTR lpszBuffer, LPDWORD lpdwBufferLength)
{
    LPWITHREADERROR lpwite = (LPWITHREADERROR)TlsGetValue(g_dwTlsErrIndex);

    TRACE("\n");

    *lpdwError = lpwite->dwError;
    if (lpwite->dwError)
    {
        strncpy(lpszBuffer, lpwite->response, *lpdwBufferLength);
	*lpdwBufferLength = strlen(lpszBuffer);
    }
    else
        *lpdwBufferLength = 0;

    return TRUE;
}


/***********************************************************************
 *           InternetGetConnectedState (WININET.@)
 *
 * Return connected state
 *
 * RETURNS
 *    TRUE if connected
 *    if lpdwStatus is not null, return the status (off line,
 *    modem, lan...) in it.
 *    FALSE if not connected
 */
BOOL WINAPI InternetGetConnectedState(LPDWORD lpdwStatus, DWORD dwReserved)
{
    if (lpdwStatus) {
	FIXME("always returning LAN connection.\n");
	*lpdwStatus = INTERNET_CONNECTION_LAN;
    }
    return TRUE;
}

/***********************************************************************
 *           InternetGetConnectedStateEx (WININET.@)
 *
 * Return connected state
 *
 * RETURNS
 *    TRUE if connected
 *    if lpdwStatus is not null, return the status (off line,
 *    modem, lan...) in it.
 *    FALSE if not connected
 */
BOOL WINAPI InternetGetConnectedStateExW(LPDWORD lpdwStatus, LPWSTR lpszConnectionName,
                                         DWORD dwNameLen, DWORD dwReserved)
{
    /* Must be zero */
    if(dwReserved)
	return FALSE;

    if (lpdwStatus) {
        FIXME("always returning LAN connection.\n");
        *lpdwStatus = INTERNET_CONNECTION_LAN;
    }
    return TRUE;
}

/***********************************************************************
 *           InternetConnectA (WININET.@)
 *
 * Open a ftp, gopher or http session
 *
 * RETURNS
 *    HINTERNET a session handle on success
 *    NULL on failure
 *
 */
HINTERNET WINAPI InternetConnectA(HINTERNET hInternet,
    LPCSTR lpszServerName, INTERNET_PORT nServerPort,
    LPCSTR lpszUserName, LPCSTR lpszPassword,
    DWORD dwService, DWORD dwFlags, DWORD dwContext)
{
    HINTERNET rc = (HINTERNET) NULL;

    TRACE("(%p, %s, %i, %s, %s, %li, %li, %li)\n", hInternet, debugstr_a(lpszServerName),
	  nServerPort, debugstr_a(lpszUserName), debugstr_a(lpszPassword),
	  dwService, dwFlags, dwContext);

    /* Clear any error information */
    INTERNET_SetLastError(0);

    switch (dwService)
    {
        case INTERNET_SERVICE_FTP:
            rc = FTP_Connect(hInternet, lpszServerName, nServerPort,
            lpszUserName, lpszPassword, dwFlags, dwContext);
            break;

        case INTERNET_SERVICE_HTTP:
	    rc = HTTP_Connect(hInternet, lpszServerName, nServerPort,
            lpszUserName, lpszPassword, dwFlags, dwContext);
            break;

        case INTERNET_SERVICE_GOPHER:
        default:
            break;
    }

    TRACE("returning %p\n", rc);
    return rc;
}


/***********************************************************************
 *           InternetConnectW (WININET.@)
 *
 * Open a ftp, gopher or http session
 *
 * RETURNS
 *    HINTERNET a session handle on success
 *    NULL on failure
 *
 */
HINTERNET WINAPI InternetConnectW(HINTERNET hInternet,
    LPCWSTR lpszServerName, INTERNET_PORT nServerPort,
    LPCWSTR lpszUserName, LPCWSTR lpszPassword,
    DWORD dwService, DWORD dwFlags, DWORD dwContext)
{
    HINTERNET rc = (HINTERNET)NULL;
    INT lenServer = 0;
    INT lenUser = 0;
    INT lenPass = 0;
    CHAR *szServerName = NULL;
    CHAR *szUserName = NULL;
    CHAR *szPassword = NULL;

    if (lpszServerName)
    {
	lenServer = lstrlenW(lpszServerName)+1;
        szServerName = (CHAR *)HeapAlloc(GetProcessHeap(), 0, lenServer*sizeof(CHAR));
        WideCharToMultiByte(CP_ACP, -1, lpszServerName, -1, szServerName, lenServer,
                            NULL, NULL);
    }
    if (lpszUserName)
    {
	lenUser = lstrlenW(lpszUserName)+1;
        szUserName = (CHAR *)HeapAlloc(GetProcessHeap(), 0, lenUser*sizeof(CHAR));
        WideCharToMultiByte(CP_ACP, -1, lpszUserName, -1, szUserName, lenUser,
                            NULL, NULL);
    }
    if (lpszPassword)
    {
	lenPass = lstrlenW(lpszPassword)+1;
        szPassword = (CHAR *)HeapAlloc(GetProcessHeap(), 0, lenPass*sizeof(CHAR));
        WideCharToMultiByte(CP_ACP, -1, lpszPassword, -1, szPassword, lenPass,
                            NULL, NULL);
    }


    rc = InternetConnectA(hInternet, szServerName, nServerPort,
        szUserName, szPassword, dwService, dwFlags, dwContext);

    if (szServerName) HeapFree(GetProcessHeap(), 0, szServerName);
    if (szUserName) HeapFree(GetProcessHeap(), 0, szUserName);
    if (szPassword) HeapFree(GetProcessHeap(), 0, szPassword);
    return rc;
}


/***********************************************************************
 *           InternetFindNextFileA (WININET.@)
 *
 * Continues a file search from a previous call to FindFirstFile
 *
 * RETURNS
 *    TRUE on success
 *    FALSE on failure
 *
 */
BOOL WINAPI InternetFindNextFileA(HINTERNET hFind, LPVOID lpvFindData)
{
    LPWININETAPPINFOA hIC = NULL;
    LPWININETFINDNEXTA lpwh = (LPWININETFINDNEXTA) hFind;

    TRACE("\n");

    if (NULL == lpwh || lpwh->hdr.htype != WH_HFINDNEXT)
    {
        INTERNET_SetLastError(ERROR_INTERNET_INCORRECT_HANDLE_TYPE);
        return FALSE;
    }

    hIC = GET_HWININET_FROM_LPWININETFINDNEXT(lpwh);
    if (hIC->hdr.dwFlags & INTERNET_FLAG_ASYNC)
    {
        WORKREQUEST workRequest;

        workRequest.asyncall = INTERNETFINDNEXTA;
	workRequest.HFTPSESSION = (DWORD)hFind;
	workRequest.LPFINDFILEDATA = (DWORD)lpvFindData;

	return INTERNET_AsyncCall(&workRequest);
    }
    else
    {
        return INTERNET_FindNextFileA(hFind, lpvFindData);
    }
}

/***********************************************************************
 *           INTERNET_FindNextFileA (Internal)
 *
 * Continues a file search from a previous call to FindFirstFile
 *
 * RETURNS
 *    TRUE on success
 *    FALSE on failure
 *
 */
BOOL WINAPI INTERNET_FindNextFileA(HINTERNET hFind, LPVOID lpvFindData)
{
    BOOL bSuccess = TRUE;
    LPWININETAPPINFOA hIC = NULL;
    LPWIN32_FIND_DATAA lpFindFileData;
    LPWININETFINDNEXTA lpwh = (LPWININETFINDNEXTA) hFind;

    TRACE("\n");

    if (NULL == lpwh || lpwh->hdr.htype != WH_HFINDNEXT)
    {
        INTERNET_SetLastError(ERROR_INTERNET_INCORRECT_HANDLE_TYPE);
        return FALSE;
    }

    /* Clear any error information */
    INTERNET_SetLastError(0);

    if (lpwh->hdr.lpwhparent->htype != WH_HFTPSESSION)
    {
        FIXME("Only FTP find next supported\n");
        INTERNET_SetLastError(ERROR_INTERNET_INCORRECT_HANDLE_TYPE);
	return FALSE;
    }

    TRACE("index(%d) size(%ld)\n", lpwh->index, lpwh->size);

    lpFindFileData = (LPWIN32_FIND_DATAA) lpvFindData;
    ZeroMemory(lpFindFileData, sizeof(WIN32_FIND_DATAA));

    if (lpwh->index >= lpwh->size)
    {
        INTERNET_SetLastError(ERROR_NO_MORE_FILES);
        bSuccess = FALSE;
	goto lend;
    }

    FTP_ConvertFileProp(&lpwh->lpafp[lpwh->index], lpFindFileData);
    lpwh->index++;

    TRACE("\nName: %s\nSize: %ld\n", lpFindFileData->cFileName, lpFindFileData->nFileSizeLow);

lend:

    hIC = GET_HWININET_FROM_LPWININETFINDNEXT(lpwh);
    if (hIC->lpfnStatusCB)
    {
        INTERNET_ASYNC_RESULT iar;

        iar.dwResult = (DWORD)bSuccess;
        iar.dwError = iar.dwError = bSuccess ? ERROR_SUCCESS :
                                               INTERNET_GetLastError();

        SendAsyncCallback(hIC, hFind, lpwh->hdr.dwContext,
                      INTERNET_STATUS_REQUEST_COMPLETE, &iar,
                       sizeof(INTERNET_ASYNC_RESULT));
    }

    return bSuccess;
}


/***********************************************************************
 *           INTERNET_CloseHandle (internal)
 *
 * Close internet handle
 *
 * RETURNS
 *    Void
 *
 */
VOID INTERNET_CloseHandle(LPWININETAPPINFOA lpwai)
{
    TRACE("%p\n",lpwai);

    SendAsyncCallback(lpwai, lpwai, lpwai->hdr.dwContext,
                      INTERNET_STATUS_HANDLE_CLOSING, lpwai,
                      sizeof(HINTERNET));

    if (lpwai->lpszAgent)
        HeapFree(GetProcessHeap(), 0, lpwai->lpszAgent);

    if (lpwai->lpszProxy)
        HeapFree(GetProcessHeap(), 0, lpwai->lpszProxy);

    if (lpwai->lpszProxyBypass)
        HeapFree(GetProcessHeap(), 0, lpwai->lpszProxyBypass);

    HeapFree(GetProcessHeap(), 0, lpwai);
}


/***********************************************************************
 *           InternetCloseHandle (WININET.@)
 *
 * Generic close handle function
 *
 * RETURNS
 *    TRUE on success
 *    FALSE on failure
 *
 */
BOOL WINAPI InternetCloseHandle(HINTERNET hInternet)
{
    BOOL retval;
    LPWININETHANDLEHEADER lpwh = (LPWININETHANDLEHEADER) hInternet;

    TRACE("%p\n",hInternet);
    if (NULL == lpwh)
        return FALSE;

    __TRY {
	/* Clear any error information */
	INTERNET_SetLastError(0);
        retval = FALSE;

	switch (lpwh->htype)
	{
	    case WH_HINIT:
		INTERNET_CloseHandle((LPWININETAPPINFOA) lpwh);
		retval = TRUE;
		break;

	    case WH_HHTTPSESSION:
		HTTP_CloseHTTPSessionHandle((LPWININETHTTPSESSIONA) lpwh);
		retval = TRUE;
		break;

	    case WH_HHTTPREQ:
		HTTP_CloseHTTPRequestHandle((LPWININETHTTPREQA) lpwh);
		retval = TRUE;
		break;

	    case WH_HFTPSESSION:
		retval = FTP_CloseSessionHandle((LPWININETFTPSESSIONA) lpwh);
		break;

	    case WH_HFINDNEXT:
		retval = FTP_CloseFindNextHandle((LPWININETFINDNEXTA) lpwh);
		break;

	    default:
		break;
	}
    } __EXCEPT(page_fault) {
	INTERNET_SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
    }
    __ENDTRY

    return retval;
}


/***********************************************************************
 *           ConvertUrlComponentValue (Internal)
 *
 * Helper function for InternetCrackUrlW
 *
 */
void ConvertUrlComponentValue(LPSTR* lppszComponent, LPDWORD dwComponentLen,
                              LPWSTR lpwszComponent, DWORD dwwComponentLen,
                              LPCSTR lpszStart,
                              LPCWSTR lpwszStart)
{
    if (*dwComponentLen != 0)
    {
        int nASCIILength=WideCharToMultiByte(CP_ACP,0,lpwszComponent,dwwComponentLen,NULL,0,NULL,NULL);
        if (*lppszComponent == NULL)
        {
            int nASCIIOffset=WideCharToMultiByte(CP_ACP,0,lpwszStart,lpwszComponent-lpwszStart,NULL,0,NULL,NULL);
            *lppszComponent = (LPSTR)lpszStart+nASCIIOffset;
            *dwComponentLen = nASCIILength;
        }
        else
        {
            INT ncpylen = min((*dwComponentLen)-1, nASCIILength);
            WideCharToMultiByte(CP_ACP,0,lpwszComponent,dwwComponentLen,*lppszComponent,ncpylen+1,NULL,NULL);
            (*lppszComponent)[ncpylen]=0;
            *dwComponentLen = ncpylen;
        }
    }
}


/***********************************************************************
 *           InternetCrackUrlA (WININET.@)
 *
 * Break up URL into its components
 *
 * TODO: Handle dwFlags
 *
 * RETURNS
 *    TRUE on success
 *    FALSE on failure
 *
 */
BOOL WINAPI InternetCrackUrlA(LPCSTR lpszUrl, DWORD dwUrlLength, DWORD dwFlags,
    LPURL_COMPONENTSA lpUrlComponents)
{
  DWORD nLength;
  URL_COMPONENTSW UCW;
  WCHAR* lpwszUrl;
  if(dwUrlLength==0)
      dwUrlLength=strlen(lpszUrl);
  lpwszUrl=HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR)*(dwUrlLength+1));
  memset(lpwszUrl,0,sizeof(WCHAR)*(dwUrlLength+1));
  nLength=MultiByteToWideChar(CP_ACP,0,lpszUrl,dwUrlLength,lpwszUrl,dwUrlLength+1);
  memset(&UCW,0,sizeof(UCW));
  if(lpUrlComponents->dwHostNameLength!=0)
      UCW.dwHostNameLength=1;
  if(lpUrlComponents->dwUserNameLength!=0)
      UCW.dwUserNameLength=1;
  if(lpUrlComponents->dwPasswordLength!=0)
      UCW.dwPasswordLength=1;
  if(lpUrlComponents->dwUrlPathLength!=0)
      UCW.dwUrlPathLength=1;
  if(lpUrlComponents->dwSchemeLength!=0)
      UCW.dwSchemeLength=1;
  if(lpUrlComponents->dwExtraInfoLength!=0)
      UCW.dwExtraInfoLength=1;
  if(!InternetCrackUrlW(lpwszUrl,nLength,dwFlags,&UCW))
  {
      HeapFree(GetProcessHeap(), 0, lpwszUrl);
      return FALSE;
  }
  ConvertUrlComponentValue(&lpUrlComponents->lpszHostName, &lpUrlComponents->dwHostNameLength,
                           UCW.lpszHostName, UCW.dwHostNameLength,
                           lpszUrl, lpwszUrl);
  ConvertUrlComponentValue(&lpUrlComponents->lpszUserName, &lpUrlComponents->dwUserNameLength,
                           UCW.lpszUserName, UCW.dwUserNameLength,
                           lpszUrl, lpwszUrl);
  ConvertUrlComponentValue(&lpUrlComponents->lpszPassword, &lpUrlComponents->dwPasswordLength,
                           UCW.lpszPassword, UCW.dwPasswordLength,
                           lpszUrl, lpwszUrl);
  ConvertUrlComponentValue(&lpUrlComponents->lpszUrlPath, &lpUrlComponents->dwUrlPathLength,
                           UCW.lpszUrlPath, UCW.dwUrlPathLength,
                           lpszUrl, lpwszUrl);
  ConvertUrlComponentValue(&lpUrlComponents->lpszScheme, &lpUrlComponents->dwSchemeLength,
                           UCW.lpszScheme, UCW.dwSchemeLength,
                           lpszUrl, lpwszUrl);
  ConvertUrlComponentValue(&lpUrlComponents->lpszExtraInfo, &lpUrlComponents->dwExtraInfoLength,
                           UCW.lpszExtraInfo, UCW.dwExtraInfoLength,
                           lpszUrl, lpwszUrl);
  lpUrlComponents->nScheme=UCW.nScheme;
  lpUrlComponents->nPort=UCW.nPort;
  HeapFree(GetProcessHeap(), 0, lpwszUrl);
  
  TRACE("%s: scheme(%s) host(%s) path(%s) extra(%s)\n", lpszUrl,
          debugstr_an(lpUrlComponents->lpszScheme,lpUrlComponents->dwSchemeLength),
          debugstr_an(lpUrlComponents->lpszHostName,lpUrlComponents->dwHostNameLength),
          debugstr_an(lpUrlComponents->lpszUrlPath,lpUrlComponents->dwUrlPathLength),
          debugstr_an(lpUrlComponents->lpszExtraInfo,lpUrlComponents->dwExtraInfoLength));

  return TRUE;
}

/***********************************************************************
 *           GetInternetSchemeW (internal)
 *
 * Get scheme of url
 *
 * RETURNS
 *    scheme on success
 *    INTERNET_SCHEME_UNKNOWN on failure
 *
 */
INTERNET_SCHEME GetInternetSchemeW(LPCWSTR lpszScheme, INT nMaxCmp)
{
    INTERNET_SCHEME iScheme=INTERNET_SCHEME_UNKNOWN;
    WCHAR lpszFtp[]={'f','t','p',0};
    WCHAR lpszGopher[]={'g','o','p','h','e','r',0};
    WCHAR lpszHttp[]={'h','t','t','p',0};
    WCHAR lpszHttps[]={'h','t','t','p','s',0};
    WCHAR lpszFile[]={'f','i','l','e',0};
    WCHAR lpszNews[]={'n','e','w','s',0};
    WCHAR lpszMailto[]={'m','a','i','l','t','o',0};
    WCHAR lpszRes[]={'r','e','s',0};
    WCHAR* tempBuffer=NULL;
    TRACE("\n");
    if(lpszScheme==NULL)
        return INTERNET_SCHEME_UNKNOWN;

    tempBuffer=malloc(nMaxCmp+1);
    strncpyW(tempBuffer,lpszScheme,nMaxCmp);
    tempBuffer[nMaxCmp]=0;
    strlwrW(tempBuffer);
    if (nMaxCmp==strlenW(lpszFtp) && !strncmpW(lpszFtp, tempBuffer, nMaxCmp))
        iScheme=INTERNET_SCHEME_FTP;
    else if (nMaxCmp==strlenW(lpszGopher) && !strncmpW(lpszGopher, tempBuffer, nMaxCmp))
        iScheme=INTERNET_SCHEME_GOPHER;
    else if (nMaxCmp==strlenW(lpszHttp) && !strncmpW(lpszHttp, tempBuffer, nMaxCmp))
        iScheme=INTERNET_SCHEME_HTTP;
    else if (nMaxCmp==strlenW(lpszHttps) && !strncmpW(lpszHttps, tempBuffer, nMaxCmp))
        iScheme=INTERNET_SCHEME_HTTPS;
    else if (nMaxCmp==strlenW(lpszFile) && !strncmpW(lpszFile, tempBuffer, nMaxCmp))
        iScheme=INTERNET_SCHEME_FILE;
    else if (nMaxCmp==strlenW(lpszNews) && !strncmpW(lpszNews, tempBuffer, nMaxCmp))
        iScheme=INTERNET_SCHEME_NEWS;
    else if (nMaxCmp==strlenW(lpszMailto) && !strncmpW(lpszMailto, tempBuffer, nMaxCmp))
        iScheme=INTERNET_SCHEME_MAILTO;
    else if (nMaxCmp==strlenW(lpszRes) && !strncmpW(lpszRes, tempBuffer, nMaxCmp))
        iScheme=INTERNET_SCHEME_RES;
    free(tempBuffer);
    return iScheme;
}

/***********************************************************************
 *           SetUrlComponentValueW (Internal)
 *
 * Helper function for InternetCrackUrlW
 *
 * RETURNS
 *    TRUE on success
 *    FALSE on failure
 *
 */
BOOL SetUrlComponentValueW(LPWSTR* lppszComponent, LPDWORD dwComponentLen, LPCWSTR lpszStart, INT len)
{
    TRACE("%s (%d)\n", debugstr_wn(lpszStart,len), len);

    if (*dwComponentLen != 0 || *lppszComponent == NULL)
    {
        if (*lppszComponent == NULL)
        {
            *lppszComponent = (LPWSTR)lpszStart;
            *dwComponentLen = len;
        }
        else
        {
            INT ncpylen = min((*dwComponentLen)-1, len);
            strncpyW(*lppszComponent, lpszStart, ncpylen);
            (*lppszComponent)[ncpylen] = '\0';
            *dwComponentLen = ncpylen;
        }
    }

    return TRUE;
}

/***********************************************************************
 *           InternetCrackUrlW   (WININET.@)
 */
BOOL WINAPI InternetCrackUrlW(LPCWSTR lpszUrl, DWORD dwUrlLength, DWORD dwFlags,
                              LPURL_COMPONENTSW lpUC)
{
  /*
   * RFC 1808
   * <protocol>:[//<net_loc>][/path][;<params>][?<query>][#<fragment>]
   *
   */
    LPWSTR lpszParam    = NULL;
    BOOL  bIsAbsolute = FALSE;
    LPWSTR lpszap = (WCHAR*)lpszUrl;
    LPWSTR lpszcp = NULL;
    WCHAR lpszSeparators[3]={';','?',0};
    WCHAR lpszSlash[2]={'/',0};
    if(dwUrlLength==0)
        dwUrlLength=strlenW(lpszUrl);

    TRACE("\n");

    /* Determine if the URI is absolute. */
    while (*lpszap != '\0')
    {
        if (isalnumW(*lpszap))
        {
            lpszap++;
            continue;
        }
        if ((*lpszap == ':') && (lpszap - lpszUrl >= 2))
        {
            bIsAbsolute = TRUE;
            lpszcp = lpszap;
        }
        else
        {
            lpszcp = (LPWSTR)lpszUrl; /* Relative url */
        }

        break;
    }

    /* Parse <params> */
    lpszParam = strpbrkW(lpszap, lpszSeparators);
    if (lpszParam != NULL)
    {
        if (!SetUrlComponentValueW(&lpUC->lpszExtraInfo, &lpUC->dwExtraInfoLength,
                                   lpszParam, dwUrlLength-(lpszParam-lpszUrl)))
        {
            return FALSE;
        }
    }

    if (bIsAbsolute) /* Parse <protocol>:[//<net_loc>] */
    {
        LPWSTR lpszNetLoc;
        WCHAR wszAbout[]={'a','b','o','u','t',':',0};

        /* Get scheme first. */
        lpUC->nScheme = GetInternetSchemeW(lpszUrl, lpszcp - lpszUrl);
        if (!SetUrlComponentValueW(&lpUC->lpszScheme, &lpUC->dwSchemeLength,
                                   lpszUrl, lpszcp - lpszUrl))
            return FALSE;

        /* Eat ':' in protocol. */
        lpszcp++;

        /* if the scheme is "about", there is no host */
        if(strncmpW(wszAbout,lpszUrl, lpszcp - lpszUrl)==0)
        {
            SetUrlComponentValueW(&lpUC->lpszUserName, &lpUC->dwUserNameLength, NULL, 0);
            SetUrlComponentValueW(&lpUC->lpszPassword, &lpUC->dwPasswordLength, NULL, 0);
            SetUrlComponentValueW(&lpUC->lpszHostName, &lpUC->dwHostNameLength, NULL, 0);
            lpUC->nPort = 0;
        }
        else
        {
            /* Skip over slashes. */
            if (*lpszcp == '/')
            {
                lpszcp++;
                if (*lpszcp == '/')
                {
                    lpszcp++;
                    if (*lpszcp == '/')
                        lpszcp++;
                }
            }

            lpszNetLoc = strpbrkW(lpszcp, lpszSlash);
            if (lpszParam)
            {
                if (lpszNetLoc)
                    lpszNetLoc = min(lpszNetLoc, lpszParam);
                else
                    lpszNetLoc = lpszParam;
            }
            else if (!lpszNetLoc)
                lpszNetLoc = lpszcp + dwUrlLength-(lpszcp-lpszUrl);

            /* Parse net-loc */
            if (lpszNetLoc)
            {
                LPWSTR lpszHost;
                LPWSTR lpszPort;

                /* [<user>[<:password>]@]<host>[:<port>] */
                /* First find the user and password if they exist */

                lpszHost = strchrW(lpszcp, '@');
                if (lpszHost == NULL || lpszHost > lpszNetLoc)
                {
                    /* username and password not specified. */
                    SetUrlComponentValueW(&lpUC->lpszUserName, &lpUC->dwUserNameLength, NULL, 0);
                    SetUrlComponentValueW(&lpUC->lpszPassword, &lpUC->dwPasswordLength, NULL, 0);
                }
                else /* Parse out username and password */
                {
                    LPWSTR lpszUser = lpszcp;
                    LPWSTR lpszPasswd = lpszHost;

                    while (lpszcp < lpszHost)
                    {
                        if (*lpszcp == ':')
                            lpszPasswd = lpszcp;

                        lpszcp++;
                    }

                    SetUrlComponentValueW(&lpUC->lpszUserName, &lpUC->dwUserNameLength,
                                          lpszUser, lpszPasswd - lpszUser);

                    if (lpszPasswd != lpszHost)
                        lpszPasswd++;
                    SetUrlComponentValueW(&lpUC->lpszPassword, &lpUC->dwPasswordLength,
                                          lpszPasswd == lpszHost ? NULL : lpszPasswd,
                                          lpszHost - lpszPasswd);

                    lpszcp++; /* Advance to beginning of host */
                }

                /* Parse <host><:port> */

                lpszHost = lpszcp;
                lpszPort = lpszNetLoc;

                /* special case for res:// URLs: there is no port here, so the host is the
                   entire string up to the first '/' */
                if(lpUC->nScheme==INTERNET_SCHEME_RES)
                {
                    SetUrlComponentValueW(&lpUC->lpszHostName, &lpUC->dwHostNameLength,
                                          lpszHost, lpszPort - lpszHost);
                    lpUC->nPort = 0;
                    lpszcp=lpszNetLoc;
                }
                else
                {
                    while (lpszcp < lpszNetLoc)
                    {
                        if (*lpszcp == ':')
                            lpszPort = lpszcp;

                        lpszcp++;
                    }

                    /* If the scheme is "file" and the host is just one letter, it's not a host */
                    if(lpUC->nScheme==INTERNET_SCHEME_FILE && (lpszPort-lpszHost)==1)
                    {
                        lpszcp=lpszHost;
                        SetUrlComponentValueW(&lpUC->lpszHostName, &lpUC->dwHostNameLength,
                                              NULL, 0);
                        lpUC->nPort = 0;
                    }
                    else
                    {
                        SetUrlComponentValueW(&lpUC->lpszHostName, &lpUC->dwHostNameLength,
                                              lpszHost, lpszPort - lpszHost);
                        if (lpszPort != lpszNetLoc)
                            lpUC->nPort = atoiW(++lpszPort);
                        else
                            lpUC->nPort = 0;
                    }
                }
            }
        }
    }

    /* Here lpszcp points to:
     *
     * <protocol>:[//<net_loc>][/path][;<params>][?<query>][#<fragment>]
     *                          ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
     */
    if (lpszcp != 0 && *lpszcp != '\0' && (!lpszParam || lpszcp < lpszParam))
    {
        INT len;

        /* Only truncate the parameter list if it's already been saved
         * in lpUC->lpszExtraInfo.
         */
        if (lpszParam && lpUC->dwExtraInfoLength)
            len = lpszParam - lpszcp;
        else
        {
            /* Leave the parameter list in lpszUrlPath.  Strip off any trailing
             * newlines if necessary.
             */
            LPWSTR lpsznewline = strchrW(lpszcp, '\n');
            if (lpsznewline != NULL)
                len = lpsznewline - lpszcp;
            else
                len = dwUrlLength-(lpszcp-lpszUrl);
        }

        if (!SetUrlComponentValueW(&lpUC->lpszUrlPath, &lpUC->dwUrlPathLength,
                                   lpszcp, len))
            return FALSE;
    }
    else
    {
        lpUC->dwUrlPathLength = 0;
    }

    TRACE("%s: host(%s) path(%s) extra(%s)\n", debugstr_wn(lpszUrl,dwUrlLength),
             debugstr_wn(lpUC->lpszHostName,lpUC->dwHostNameLength),
             debugstr_wn(lpUC->lpszUrlPath,lpUC->dwUrlPathLength),
             debugstr_wn(lpUC->lpszExtraInfo,lpUC->dwExtraInfoLength));

    return TRUE;
}

/***********************************************************************
 *           InternetAttemptConnect (WININET.@)
 *
 * Attempt to make a connection to the internet
 *
 * RETURNS
 *    ERROR_SUCCESS on success
 *    Error value   on failure
 *
 */
DWORD WINAPI InternetAttemptConnect(DWORD dwReserved)
{
    FIXME("Stub\n");
    return ERROR_SUCCESS;
}


/***********************************************************************
 *           InternetCanonicalizeUrlA (WININET.@)
 *
 * Escape unsafe characters and spaces
 *
 * RETURNS
 *    TRUE on success
 *    FALSE on failure
 *
 */
BOOL WINAPI InternetCanonicalizeUrlA(LPCSTR lpszUrl, LPSTR lpszBuffer,
	LPDWORD lpdwBufferLength, DWORD dwFlags)
{
    HRESULT hr;
    TRACE("%s %p %p %08lx\n",debugstr_a(lpszUrl), lpszBuffer,
	  lpdwBufferLength, dwFlags);

    /* Flip this bit to correspond to URL_ESCAPE_UNSAFE */
    dwFlags ^= ICU_NO_ENCODE;

    dwFlags |= 0x80000000; /* Don't know what this means */

    hr = UrlCanonicalizeA(lpszUrl, lpszBuffer, lpdwBufferLength, dwFlags);

    return (hr == S_OK) ? TRUE : FALSE;
}

/***********************************************************************
 *           InternetCanonicalizeUrlW (WININET.@)
 *
 * Escape unsafe characters and spaces
 *
 * RETURNS
 *    TRUE on success
 *    FALSE on failure
 *
 */
BOOL WINAPI InternetCanonicalizeUrlW(LPCWSTR lpszUrl, LPWSTR lpszBuffer,
    LPDWORD lpdwBufferLength, DWORD dwFlags)
{
    HRESULT hr;
    TRACE("%s %p %p %08lx\n", debugstr_w(lpszUrl), lpszBuffer,
        lpdwBufferLength, dwFlags);

    /* Flip this bit to correspond to URL_ESCAPE_UNSAFE */
    dwFlags ^= ICU_NO_ENCODE;

    dwFlags |= 0x80000000; /* Don't know what this means */

    hr = UrlCanonicalizeW(lpszUrl, lpszBuffer, lpdwBufferLength, dwFlags);

    return (hr == S_OK) ? TRUE : FALSE;
}


/***********************************************************************
 *           InternetSetStatusCallbackA (WININET.@)
 *
 * Sets up a callback function which is called as progress is made
 * during an operation.
 *
 * RETURNS
 *    Previous callback or NULL 	on success
 *    INTERNET_INVALID_STATUS_CALLBACK  on failure
 *
 */
INTERNET_STATUS_CALLBACK WINAPI InternetSetStatusCallbackA(
	HINTERNET hInternet ,INTERNET_STATUS_CALLBACK lpfnIntCB)
{
    INTERNET_STATUS_CALLBACK retVal;
    LPWININETAPPINFOA lpwai = (LPWININETAPPINFOA)hInternet;

    TRACE("0x%08lx\n", (ULONG)hInternet);
    if (lpwai->hdr.htype != WH_HINIT)
        return INTERNET_INVALID_STATUS_CALLBACK;

    retVal = lpwai->lpfnStatusCB;
    lpwai->lpfnStatusCB = lpfnIntCB;

    return retVal;
}


/***********************************************************************
 *           InternetWriteFile (WININET.@)
 *
 * Write data to an open internet file
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI InternetWriteFile(HINTERNET hFile, LPCVOID lpBuffer ,
	DWORD dwNumOfBytesToWrite, LPDWORD lpdwNumOfBytesWritten)
{
    BOOL retval = FALSE;
    int nSocket = -1;
    LPWININETHANDLEHEADER lpwh = (LPWININETHANDLEHEADER) hFile;

    TRACE("\n");
    if (NULL == lpwh)
        return FALSE;

    switch (lpwh->htype)
    {
        case WH_HHTTPREQ:
            FIXME("This shouldn't be here! We don't support this kind"
                  " of connection anymore. Must use NETCON functions,"
                  " especially if using SSL\n");
            nSocket = ((LPWININETHTTPREQA)hFile)->netConnection.socketFD;
            break;

        case WH_HFILE:
            nSocket = ((LPWININETFILE)hFile)->nDataSocket;
            break;

        default:
            break;
    }

    if (nSocket != -1)
    {
        int res = send(nSocket, lpBuffer, dwNumOfBytesToWrite, 0);
        retval = (res >= 0);
        *lpdwNumOfBytesWritten = retval ? res : 0;
    }

    return retval;
}


/***********************************************************************
 *           InternetReadFile (WININET.@)
 *
 * Read data from an open internet file
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI InternetReadFile(HINTERNET hFile, LPVOID lpBuffer,
	DWORD dwNumOfBytesToRead, LPDWORD dwNumOfBytesRead)
{
    BOOL retval = FALSE;
    int nSocket = -1;
    LPWININETHANDLEHEADER lpwh = (LPWININETHANDLEHEADER) hFile;

    TRACE("\n");

    if (NULL == lpwh)
        return FALSE;

    /* FIXME: this should use NETCON functions! */
    switch (lpwh->htype)
    {
        case WH_HHTTPREQ:
            if (!NETCON_recv(&((LPWININETHTTPREQA)hFile)->netConnection, lpBuffer,
                             dwNumOfBytesToRead, 0, (int *)dwNumOfBytesRead))
            {
                *dwNumOfBytesRead = 0;
                retval = TRUE; /* Under windows, it seems to return 0 even if nothing was read... */
            }
            else
                retval = TRUE;
            break;

        case WH_HFILE:
            /* FIXME: FTP should use NETCON_ stuff */
            nSocket = ((LPWININETFILE)hFile)->nDataSocket;
            if (nSocket != -1)
            {
                int res = recv(nSocket, lpBuffer, dwNumOfBytesToRead, 0);
                retval = (res >= 0);
                *dwNumOfBytesRead = retval ? res : 0;
            }
            break;

        default:
            break;
    }

    if (nSocket != -1)
    {
        int res = recv(nSocket, lpBuffer, dwNumOfBytesToRead, 0);
        retval = (res >= 0);
        *dwNumOfBytesRead = retval ? res : 0;
    }
    return retval;
}

/***********************************************************************
 *           InternetReadFileExA (WININET.@)
 *
 * Read data from an open internet file
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI InternetReadFileExA(HINTERNET hFile, LPINTERNET_BUFFERSA lpBuffer,
	DWORD dwFlags, DWORD dwContext)
{
  FIXME("stub\n");
  return FALSE;
}

/***********************************************************************
 *           InternetReadFileExW (WININET.@)
 *
 * Read data from an open internet file
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI InternetReadFileExW(HINTERNET hFile, LPINTERNET_BUFFERSW lpBuffer,
	DWORD dwFlags, DWORD dwContext)
{
  FIXME("stub\n");

  INTERNET_SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           INET_QueryOptionHelper (internal)
 */
static BOOL INET_QueryOptionHelper(BOOL bIsUnicode, HINTERNET hInternet, DWORD dwOption,
                                   LPVOID lpBuffer, LPDWORD lpdwBufferLength)
{
    LPWININETHANDLEHEADER lpwhh;
    BOOL bSuccess = FALSE;

    TRACE("(%p, 0x%08lx, %p, %p)\n", hInternet, dwOption, lpBuffer, lpdwBufferLength);

    if (NULL == hInternet)
    {
        INTERNET_SetLastError(ERROR_INTERNET_INCORRECT_HANDLE_TYPE);
	return FALSE;
    }

    lpwhh = (LPWININETHANDLEHEADER) hInternet;

    switch (dwOption)
    {
        case INTERNET_OPTION_HANDLE_TYPE:
        {
            ULONG type = lpwhh->htype;
            TRACE("INTERNET_OPTION_HANDLE_TYPE: %ld\n", type);

            if (*lpdwBufferLength < sizeof(ULONG))
                INTERNET_SetLastError(ERROR_INSUFFICIENT_BUFFER);
            else
            {
                memcpy(lpBuffer, &type, sizeof(ULONG));
                    *lpdwBufferLength = sizeof(ULONG);
                bSuccess = TRUE;
            }
            break;
        }

        case INTERNET_OPTION_REQUEST_FLAGS:
        {
            ULONG flags = 4;
            TRACE("INTERNET_OPTION_REQUEST_FLAGS: %ld\n", flags);
            if (*lpdwBufferLength < sizeof(ULONG))
                INTERNET_SetLastError(ERROR_INSUFFICIENT_BUFFER);
            else
            {
                memcpy(lpBuffer, &flags, sizeof(ULONG));
                    *lpdwBufferLength = sizeof(ULONG);
                bSuccess = TRUE;
            }
            break;
        }

        case INTERNET_OPTION_URL:
        case INTERNET_OPTION_DATAFILE_NAME:
        {
            ULONG type = lpwhh->htype;
            if (type == WH_HHTTPREQ)
            {
                LPWININETHTTPREQA lpreq = hInternet;
                char url[1023];

                sprintf(url,"http://%s%s",lpreq->lpszHostName,lpreq->lpszPath);
                TRACE("INTERNET_OPTION_URL: %s\n",url);
                if (*lpdwBufferLength < strlen(url)+1)
                    INTERNET_SetLastError(ERROR_INSUFFICIENT_BUFFER);
                else
                {
                    if(bIsUnicode)
                    {
                        *lpdwBufferLength=MultiByteToWideChar(CP_ACP,0,url,-1,lpBuffer,*lpdwBufferLength);
                    }
                    else
                    {
                        memcpy(lpBuffer, url, strlen(url)+1);
                        *lpdwBufferLength = strlen(url)+1;
                    }
                    bSuccess = TRUE;
                }
            }
            break;
        }
       case INTERNET_OPTION_HTTP_VERSION:
       {
            /*
             * Presently hardcoded to 1.1
             */
            ((HTTP_VERSION_INFO*)lpBuffer)->dwMajorVersion = 1;
            ((HTTP_VERSION_INFO*)lpBuffer)->dwMinorVersion = 1;
            bSuccess = TRUE;
        break;
       }

       default:
         FIXME("Stub! %ld \n",dwOption);
         break;
    }

    return bSuccess;
}

/***********************************************************************
 *           InternetQueryOptionW (WININET.@)
 *
 * Queries an options on the specified handle
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI InternetQueryOptionW(HINTERNET hInternet, DWORD dwOption,
                                 LPVOID lpBuffer, LPDWORD lpdwBufferLength)
{
    return INET_QueryOptionHelper(TRUE, hInternet, dwOption, lpBuffer, lpdwBufferLength);
}

/***********************************************************************
 *           InternetQueryOptionA (WININET.@)
 *
 * Queries an options on the specified handle
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI InternetQueryOptionA(HINTERNET hInternet, DWORD dwOption,
                                 LPVOID lpBuffer, LPDWORD lpdwBufferLength)
{
    return INET_QueryOptionHelper(FALSE, hInternet, dwOption, lpBuffer, lpdwBufferLength);
}


/***********************************************************************
 *           InternetSetOptionW (WININET.@)
 *
 * Sets an options on the specified handle
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI InternetSetOptionW(HINTERNET hInternet, DWORD dwOption,
                           LPVOID lpBuffer, DWORD dwBufferLength)
{
    LPWININETHANDLEHEADER lpwhh;

    TRACE("0x%08lx\n", dwOption);

    if (NULL == hInternet)
    {
        INTERNET_SetLastError(ERROR_INTERNET_INCORRECT_HANDLE_TYPE);
        return FALSE;
    }

    lpwhh = (LPWININETHANDLEHEADER) hInternet;

    switch (dwOption)
    {
    case INTERNET_OPTION_HTTP_VERSION:
      {
        HTTP_VERSION_INFO* pVersion=(HTTP_VERSION_INFO*)lpBuffer;
        FIXME("Option INTERNET_OPTION_HTTP_VERSION(%ld,%ld): STUB\n",pVersion->dwMajorVersion,pVersion->dwMinorVersion);
      }
      break;
    case INTERNET_OPTION_ERROR_MASK:
      {
        unsigned long flags=*(unsigned long*)lpBuffer;
        FIXME("Option INTERNET_OPTION_ERROR_MASK(%ld): STUB\n",flags);
      }
      break;
    case INTERNET_OPTION_CODEPAGE:
      {
        unsigned long codepage=*(unsigned long*)lpBuffer;
        FIXME("Option INTERNET_OPTION_CODEPAGE (%ld): STUB\n",codepage);
      }
      break;
    case INTERNET_OPTION_REQUEST_PRIORITY:
      {
        unsigned long priority=*(unsigned long*)lpBuffer;
        FIXME("Option INTERNET_OPTION_REQUEST_PRIORITY (%ld): STUB\n",priority);
      }
      break;
    default:
        FIXME("Option %ld STUB\n",dwOption);
        INTERNET_SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return TRUE;
}


/***********************************************************************
 *           InternetSetOptionA (WININET.@)
 *
 * Sets an options on the specified handle.
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI InternetSetOptionA(HINTERNET hInternet, DWORD dwOption,
                           LPVOID lpBuffer, DWORD dwBufferLength)
{
    LPVOID wbuffer;
    DWORD wlen;
    BOOL r;

    switch( dwOption )
    {
    case INTERNET_OPTION_PROXY:
        {
        LPINTERNET_PROXY_INFOA pi = (LPINTERNET_PROXY_INFOA) lpBuffer;
        LPINTERNET_PROXY_INFOW piw;
        DWORD proxlen, prbylen;
        LPWSTR prox, prby;

        proxlen = MultiByteToWideChar( CP_ACP, 0, pi->lpszProxy, -1, NULL, 0);
        prbylen= MultiByteToWideChar( CP_ACP, 0, pi->lpszProxyBypass, -1, NULL, 0);
        wlen = sizeof(*piw) + proxlen + prbylen;
        wbuffer = HeapAlloc( GetProcessHeap(), 0, wlen );
        piw = (LPINTERNET_PROXY_INFOW) wbuffer;
        piw->dwAccessType = pi->dwAccessType;
        prox = (LPWSTR) &piw[1];
        prby = &prox[proxlen+1];
        MultiByteToWideChar( CP_ACP, 0, pi->lpszProxy, -1, prox, proxlen);
        MultiByteToWideChar( CP_ACP, 0, pi->lpszProxyBypass, -1, prby, prbylen);
        piw->lpszProxy = prox;
        piw->lpszProxyBypass = prby;
        }
        break;
    case INTERNET_OPTION_USER_AGENT:
    case INTERNET_OPTION_USERNAME:
    case INTERNET_OPTION_PASSWORD:
        wlen = MultiByteToWideChar( CP_ACP, 0, lpBuffer, dwBufferLength,
                                   NULL, 0 );
        wbuffer = HeapAlloc( GetProcessHeap(), 0, wlen );
        MultiByteToWideChar( CP_ACP, 0, lpBuffer, dwBufferLength,
                                   wbuffer, wlen );
        break;
    default:
        wbuffer = lpBuffer;
        wlen = dwBufferLength;
    }

    r = InternetSetOptionW(hInternet,dwOption, wbuffer, wlen);

    if( lpBuffer != wbuffer )
        HeapFree( GetProcessHeap(), 0, wbuffer );

    return r;
}


/***********************************************************************
 *           InternetSetOptionExA (WININET.@)
 */
BOOL WINAPI InternetSetOptionExA(HINTERNET hInternet, DWORD dwOption,
                           LPVOID lpBuffer, DWORD dwBufferLength, DWORD dwFlags)
{
    FIXME("Flags %08lx ignored\n", dwFlags);
    return InternetSetOptionA( hInternet, dwOption, lpBuffer, dwBufferLength );
}

/***********************************************************************
 *           InternetSetOptionExW (WININET.@)
 */
BOOL WINAPI InternetSetOptionExW(HINTERNET hInternet, DWORD dwOption,
                           LPVOID lpBuffer, DWORD dwBufferLength, DWORD dwFlags)
{
    FIXME("Flags %08lx ignored\n", dwFlags);
    if( dwFlags & ~ISO_VALID_FLAGS )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    return InternetSetOptionW( hInternet, dwOption, lpBuffer, dwBufferLength );
}


/***********************************************************************
 *	InternetCheckConnectionA (WININET.@)
 *
 * Pings a requested host to check internet connection
 *
 * RETURNS
 *   TRUE on success and FALSE on failure. If a failure then
 *   ERROR_NOT_CONNECTED is placesd into GetLastError
 *
 */
BOOL WINAPI InternetCheckConnectionA( LPCSTR lpszUrl, DWORD dwFlags, DWORD dwReserved )
{
/*
 * this is a kludge which runs the resident ping program and reads the output.
 *
 * Anyone have a better idea?
 */

  BOOL   rc = FALSE;
  char command[1024];
  char host[1024];
  int status = -1;

  FIXME("\n");

  /*
   * Crack or set the Address
   */
  if (lpszUrl == NULL)
  {
     /*
      * According to the doc we are supost to use the ip for the next
      * server in the WnInet internal server database. I have
      * no idea what that is or how to get it.
      *
      * So someone needs to implement this.
      */
     FIXME("Unimplemented with URL of NULL\n");
     return TRUE;
  }
  else
  {
     URL_COMPONENTSA componets;

     ZeroMemory(&componets,sizeof(URL_COMPONENTSA));
     componets.lpszHostName = (LPSTR)&host;
     componets.dwHostNameLength = 1024;

     if (!InternetCrackUrlA(lpszUrl,0,0,&componets))
       goto End;

     TRACE("host name : %s\n",componets.lpszHostName);
  }

  /*
   * Build our ping command
   */
  strcpy(command,"ping -w 1 ");
  strcat(command,host);
  strcat(command," >/dev/null 2>/dev/null");

  TRACE("Ping command is : %s\n",command);

  status = system(command);

  TRACE("Ping returned a code of %i \n",status);

  /* Ping return code of 0 indicates success */
  if (status == 0)
     rc = TRUE;

End:

  if (rc == FALSE)
    SetLastError(ERROR_NOT_CONNECTED);

  return rc;
}


/***********************************************************************
 *	InternetCheckConnectionW (WININET.@)
 *
 * Pings a requested host to check internet connection
 *
 * RETURNS
 *   TRUE on success and FALSE on failure. If a failure then
 *   ERROR_NOT_CONNECTED is placed into GetLastError
 *
 */
BOOL WINAPI InternetCheckConnectionW(LPCWSTR lpszUrl, DWORD dwFlags, DWORD dwReserved)
{
    CHAR *szUrl;
    INT len;
    BOOL rc;

    len = lstrlenW(lpszUrl)+1;
    if (!(szUrl = (CHAR *)HeapAlloc(GetProcessHeap(), 0, len*sizeof(CHAR))))
        return FALSE;
    WideCharToMultiByte(CP_ACP, -1, lpszUrl, -1, szUrl, len, NULL, NULL);
    rc = InternetCheckConnectionA((LPCSTR)szUrl, dwFlags, dwReserved);
    HeapFree(GetProcessHeap(), 0, szUrl);
    
    return rc;
}


/**********************************************************
 *	InternetOpenUrlA (WININET.@)
 *
 * Opens an URL
 *
 * RETURNS
 *   handle of connection or NULL on failure
 */
HINTERNET WINAPI InternetOpenUrlA(HINTERNET hInternet, LPCSTR lpszUrl,
    LPCSTR lpszHeaders, DWORD dwHeadersLength, DWORD dwFlags, DWORD dwContext)
{
  URL_COMPONENTSA urlComponents;
  char protocol[32], hostName[MAXHOSTNAME], userName[1024];
  char password[1024], path[2048], extra[1024];
  HINTERNET client = NULL, client1 = NULL;

  TRACE("(%p, %s, %s, %08lx, %08lx, %08lx\n", hInternet, debugstr_a(lpszUrl), debugstr_a(lpszHeaders),
       dwHeadersLength, dwFlags, dwContext);

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
    return NULL;
  switch(urlComponents.nScheme) {
  case INTERNET_SCHEME_FTP:
    if(urlComponents.nPort == 0)
      urlComponents.nPort = INTERNET_DEFAULT_FTP_PORT;
    client = InternetConnectA(hInternet, hostName, urlComponents.nPort,
        userName, password, INTERNET_SERVICE_FTP, dwFlags, dwContext);
    return FtpOpenFileA(client, path, GENERIC_READ, dwFlags, dwContext);
  case INTERNET_SCHEME_HTTP:
  case INTERNET_SCHEME_HTTPS:
  {
    LPCSTR accept[2] = { "*/*", NULL };
    char *hostreq=(char*)malloc(strlen(hostName)+9);
    sprintf(hostreq, "Host: %s\r\n", hostName);
    if(urlComponents.nPort == 0) {
      if(urlComponents.nScheme == INTERNET_SCHEME_HTTP)
        urlComponents.nPort = INTERNET_DEFAULT_HTTP_PORT;
      else
	urlComponents.nPort = INTERNET_DEFAULT_HTTPS_PORT;
    }
    client = InternetConnectA(hInternet, hostName, urlComponents.nPort, userName,
        password, INTERNET_SERVICE_HTTP, dwFlags, dwContext);
    if(client == NULL)
      return NULL;
    client1 = HttpOpenRequestA(client, NULL, path, NULL, NULL, accept, dwFlags, dwContext);
    if(client1 == NULL) {
      InternetCloseHandle(client);
      return NULL;
    }
    HttpAddRequestHeadersA(client1, lpszHeaders, dwHeadersLength, HTTP_ADDREQ_FLAG_ADD);
    if(!HttpSendRequestA(client1, NULL, 0, NULL, 0)) {
      InternetCloseHandle(client1);
      InternetCloseHandle(client);
      return NULL;
    }
    return client1;
  }
  case INTERNET_SCHEME_GOPHER:
    /* gopher doesn't seem to be implemented in wine, but it's supposed
     * to be supported by InternetOpenUrlA. */
  default:
    return NULL;
  }
}


/**********************************************************
 *	InternetOpenUrlW (WININET.@)
 *
 * Opens an URL
 *
 * RETURNS
 *   handle of connection or NULL on failure
 */
HINTERNET WINAPI InternetOpenUrlW(HINTERNET hInternet, LPCWSTR lpszUrl,
    LPCWSTR lpszHeaders, DWORD dwHeadersLength, DWORD dwFlags, DWORD dwContext)
{
    HINTERNET rc = (HINTERNET)NULL;

    INT lenUrl = lstrlenW(lpszUrl)+1;
    INT lenHeaders = lstrlenW(lpszHeaders)+1;
    CHAR *szUrl = (CHAR *)HeapAlloc(GetProcessHeap(), 0, lenUrl*sizeof(CHAR));
    CHAR *szHeaders = (CHAR *)HeapAlloc(GetProcessHeap(), 0, lenHeaders*sizeof(CHAR));

    if (!szUrl || !szHeaders)
    {
        if (szUrl)
            HeapFree(GetProcessHeap(), 0, szUrl);
        if (szHeaders)
            HeapFree(GetProcessHeap(), 0, szHeaders);
        return (HINTERNET)NULL;
    }

    WideCharToMultiByte(CP_ACP, -1, lpszUrl, -1, szUrl, lenUrl,
        NULL, NULL);
    WideCharToMultiByte(CP_ACP, -1, lpszHeaders, -1, szHeaders, lenHeaders,
        NULL, NULL);

    rc = InternetOpenUrlA(hInternet, szUrl, szHeaders,
        dwHeadersLength, dwFlags, dwContext);

    HeapFree(GetProcessHeap(), 0, szUrl);
    HeapFree(GetProcessHeap(), 0, szHeaders);

    return rc;
}


/***********************************************************************
 *           INTERNET_SetLastError (internal)
 *
 * Set last thread specific error
 *
 * RETURNS
 *
 */
void INTERNET_SetLastError(DWORD dwError)
{
    LPWITHREADERROR lpwite = (LPWITHREADERROR)TlsGetValue(g_dwTlsErrIndex);

    SetLastError(dwError);
    if(lpwite)
        lpwite->dwError = dwError;
}


/***********************************************************************
 *           INTERNET_GetLastError (internal)
 *
 * Get last thread specific error
 *
 * RETURNS
 *
 */
DWORD INTERNET_GetLastError()
{
    LPWITHREADERROR lpwite = (LPWITHREADERROR)TlsGetValue(g_dwTlsErrIndex);
    return lpwite->dwError;
}


/***********************************************************************
 *           INTERNET_WorkerThreadFunc (internal)
 *
 * Worker thread execution function
 *
 * RETURNS
 *
 */
DWORD INTERNET_WorkerThreadFunc(LPVOID *lpvParam)
{
    DWORD dwWaitRes;

    while (1)
    {
	if(dwNumJobs > 0) {
	    INTERNET_ExecuteWork();
	    continue;
	}
        dwWaitRes = WaitForMultipleObjects(2, hEventArray, FALSE, MAX_IDLE_WORKER);

        if (dwWaitRes == WAIT_OBJECT_0 + 1)
            INTERNET_ExecuteWork();
        else
            break;

        InterlockedIncrement(&dwNumIdleThreads);
    }

    InterlockedDecrement(&dwNumIdleThreads);
    InterlockedDecrement(&dwNumThreads);
    TRACE("Worker thread exiting\n");
    return TRUE;
}


/***********************************************************************
 *           INTERNET_InsertWorkRequest (internal)
 *
 * Insert work request into queue
 *
 * RETURNS
 *
 */
BOOL INTERNET_InsertWorkRequest(LPWORKREQUEST lpWorkRequest)
{
    BOOL bSuccess = FALSE;
    LPWORKREQUEST lpNewRequest;

    TRACE("\n");

    lpNewRequest = HeapAlloc(GetProcessHeap(), 0, sizeof(WORKREQUEST));
    if (lpNewRequest)
    {
        memcpy(lpNewRequest, lpWorkRequest, sizeof(WORKREQUEST));
	lpNewRequest->prev = NULL;

        EnterCriticalSection(&csQueue);

        lpNewRequest->next = lpWorkQueueTail;
	if (lpWorkQueueTail)
            lpWorkQueueTail->prev = lpNewRequest;
        lpWorkQueueTail = lpNewRequest;
	if (!lpHeadWorkQueue)
            lpHeadWorkQueue = lpWorkQueueTail;

        LeaveCriticalSection(&csQueue);

	bSuccess = TRUE;
	InterlockedIncrement(&dwNumJobs);
    }

    return bSuccess;
}


/***********************************************************************
 *           INTERNET_GetWorkRequest (internal)
 *
 * Retrieves work request from queue
 *
 * RETURNS
 *
 */
BOOL INTERNET_GetWorkRequest(LPWORKREQUEST lpWorkRequest)
{
    BOOL bSuccess = FALSE;
    LPWORKREQUEST lpRequest = NULL;

    TRACE("\n");

    EnterCriticalSection(&csQueue);

    if (lpHeadWorkQueue)
    {
        lpRequest = lpHeadWorkQueue;
        lpHeadWorkQueue = lpHeadWorkQueue->prev;
	if (lpRequest == lpWorkQueueTail)
            lpWorkQueueTail = lpHeadWorkQueue;
    }

    LeaveCriticalSection(&csQueue);

    if (lpRequest)
    {
        memcpy(lpWorkRequest, lpRequest, sizeof(WORKREQUEST));
        HeapFree(GetProcessHeap(), 0, lpRequest);
	bSuccess = TRUE;
	InterlockedDecrement(&dwNumJobs);
    }

    return bSuccess;
}


/***********************************************************************
 *           INTERNET_AsyncCall (internal)
 *
 * Retrieves work request from queue
 *
 * RETURNS
 *
 */
BOOL INTERNET_AsyncCall(LPWORKREQUEST lpWorkRequest)
{
    HANDLE hThread;
    DWORD dwTID;
    BOOL bSuccess = FALSE;

    TRACE("\n");

    if (InterlockedDecrement(&dwNumIdleThreads) < 0)
    {
        InterlockedIncrement(&dwNumIdleThreads);

	if (InterlockedIncrement(&dwNumThreads) > MAX_WORKER_THREADS ||
	    !(hThread = CreateThread(NULL, 0,
            (LPTHREAD_START_ROUTINE)INTERNET_WorkerThreadFunc, NULL, 0, &dwTID)))
	{
            InterlockedDecrement(&dwNumThreads);
            INTERNET_SetLastError(ERROR_INTERNET_ASYNC_THREAD_FAILED);
	    goto lerror;
	}

	TRACE("Created new thread\n");
    }

    bSuccess = TRUE;
    INTERNET_InsertWorkRequest(lpWorkRequest);
    SetEvent(hWorkEvent);

lerror:

    return bSuccess;
}


/***********************************************************************
 *           INTERNET_ExecuteWork (internal)
 *
 * RETURNS
 *
 */
VOID INTERNET_ExecuteWork()
{
    WORKREQUEST workRequest;

    TRACE("\n");

    if (INTERNET_GetWorkRequest(&workRequest))
    {
	TRACE("Got work %d\n", workRequest.asyncall);
	switch (workRequest.asyncall)
	{
            case FTPPUTFILEA:
		FTP_FtpPutFileA((HINTERNET)workRequest.HFTPSESSION, (LPCSTR)workRequest.LPSZLOCALFILE,
                    (LPCSTR)workRequest.LPSZNEWREMOTEFILE, workRequest.DWFLAGS, workRequest.DWCONTEXT);
		HeapFree(GetProcessHeap(), 0, (LPVOID)workRequest.LPSZLOCALFILE);
		HeapFree(GetProcessHeap(), 0, (LPVOID)workRequest.LPSZNEWREMOTEFILE);
		break;

            case FTPSETCURRENTDIRECTORYA:
		FTP_FtpSetCurrentDirectoryA((HINTERNET)workRequest.HFTPSESSION,
			(LPCSTR)workRequest.LPSZDIRECTORY);
		HeapFree(GetProcessHeap(), 0, (LPVOID)workRequest.LPSZDIRECTORY);
		break;

            case FTPCREATEDIRECTORYA:
		FTP_FtpCreateDirectoryA((HINTERNET)workRequest.HFTPSESSION,
			(LPCSTR)workRequest.LPSZDIRECTORY);
		HeapFree(GetProcessHeap(), 0, (LPVOID)workRequest.LPSZDIRECTORY);
		break;

            case FTPFINDFIRSTFILEA:
                FTP_FtpFindFirstFileA((HINTERNET)workRequest.HFTPSESSION,
			(LPCSTR)workRequest.LPSZSEARCHFILE,
	           (LPWIN32_FIND_DATAA)workRequest.LPFINDFILEDATA, workRequest.DWFLAGS,
		   workRequest.DWCONTEXT);
		HeapFree(GetProcessHeap(), 0, (LPVOID)workRequest.LPSZSEARCHFILE);
		break;

            case FTPGETCURRENTDIRECTORYA:
                FTP_FtpGetCurrentDirectoryA((HINTERNET)workRequest.HFTPSESSION,
			(LPSTR)workRequest.LPSZDIRECTORY, (LPDWORD)workRequest.LPDWDIRECTORY);
		break;

            case FTPOPENFILEA:
                 FTP_FtpOpenFileA((HINTERNET)workRequest.HFTPSESSION,
                    (LPCSTR)workRequest.LPSZFILENAME,
                    workRequest.FDWACCESS,
                    workRequest.DWFLAGS,
                    workRequest.DWCONTEXT);
                 HeapFree(GetProcessHeap(), 0, (LPVOID)workRequest.LPSZFILENAME);
                 break;

            case FTPGETFILEA:
                FTP_FtpGetFileA((HINTERNET)workRequest.HFTPSESSION,
                    (LPCSTR)workRequest.LPSZREMOTEFILE,
                    (LPCSTR)workRequest.LPSZNEWFILE,
                    (BOOL)workRequest.FFAILIFEXISTS,
                    workRequest.DWLOCALFLAGSATTRIBUTE,
                    workRequest.DWFLAGS,
                    workRequest.DWCONTEXT);
		HeapFree(GetProcessHeap(), 0, (LPVOID)workRequest.LPSZREMOTEFILE);
		HeapFree(GetProcessHeap(), 0, (LPVOID)workRequest.LPSZNEWFILE);
		break;

            case FTPDELETEFILEA:
                FTP_FtpDeleteFileA((HINTERNET)workRequest.HFTPSESSION,
			(LPCSTR)workRequest.LPSZFILENAME);
		HeapFree(GetProcessHeap(), 0, (LPVOID)workRequest.LPSZFILENAME);
		break;

            case FTPREMOVEDIRECTORYA:
                FTP_FtpRemoveDirectoryA((HINTERNET)workRequest.HFTPSESSION,
			(LPCSTR)workRequest.LPSZDIRECTORY);
		HeapFree(GetProcessHeap(), 0, (LPVOID)workRequest.LPSZDIRECTORY);
		break;

            case FTPRENAMEFILEA:
                FTP_FtpRenameFileA((HINTERNET)workRequest.HFTPSESSION,
			(LPCSTR)workRequest.LPSZSRCFILE,
			(LPCSTR)workRequest.LPSZDESTFILE);
		HeapFree(GetProcessHeap(), 0, (LPVOID)workRequest.LPSZSRCFILE);
		HeapFree(GetProcessHeap(), 0, (LPVOID)workRequest.LPSZDESTFILE);
		break;

            case INTERNETFINDNEXTA:
		INTERNET_FindNextFileA((HINTERNET)workRequest.HFTPSESSION,
                    (LPWIN32_FIND_DATAA)workRequest.LPFINDFILEDATA);
		break;

            case HTTPSENDREQUESTA:
               HTTP_HttpSendRequestA((HINTERNET)workRequest.HFTPSESSION,
                       (LPCSTR)workRequest.LPSZHEADER,
                       workRequest.DWHEADERLENGTH,
                       (LPVOID)workRequest.LPOPTIONAL,
                       workRequest.DWOPTIONALLENGTH);
               HeapFree(GetProcessHeap(), 0, (LPVOID)workRequest.LPSZHEADER);
               break;

            case HTTPOPENREQUESTA:
               HTTP_HttpOpenRequestA((HINTERNET)workRequest.HFTPSESSION,
                       (LPCSTR)workRequest.LPSZVERB,
                       (LPCSTR)workRequest.LPSZOBJECTNAME,
                       (LPCSTR)workRequest.LPSZVERSION,
                       (LPCSTR)workRequest.LPSZREFERRER,
                       (LPCSTR*)workRequest.LPSZACCEPTTYPES,
                       workRequest.DWFLAGS,
                       workRequest.DWCONTEXT);
               HeapFree(GetProcessHeap(), 0, (LPVOID)workRequest.LPSZVERB);
               HeapFree(GetProcessHeap(), 0, (LPVOID)workRequest.LPSZOBJECTNAME);
               HeapFree(GetProcessHeap(), 0, (LPVOID)workRequest.LPSZVERSION);
               HeapFree(GetProcessHeap(), 0, (LPVOID)workRequest.LPSZREFERRER);
                break;

            case SENDCALLBACK:
               SendAsyncCallbackInt((LPWININETAPPINFOA)workRequest.param1,
                       (HINTERNET)workRequest.param2, workRequest.param3,
                        workRequest.param4, (LPVOID)workRequest.param5,
                        workRequest.param6);
               break;
	}
    }
}


/***********************************************************************
 *          INTERNET_GetResponseBuffer
 *
 * RETURNS
 *
 */
LPSTR INTERNET_GetResponseBuffer()
{
    LPWITHREADERROR lpwite = (LPWITHREADERROR)TlsGetValue(g_dwTlsErrIndex);
    TRACE("\n");
    return lpwite->response;
}

/***********************************************************************
 *           INTERNET_GetNextLine  (internal)
 *
 * Parse next line in directory string listing
 *
 * RETURNS
 *   Pointer to beginning of next line
 *   NULL on failure
 *
 */

LPSTR INTERNET_GetNextLine(INT nSocket, LPSTR lpszBuffer, LPDWORD dwBuffer)
{
    struct timeval tv;
    fd_set infd;
    BOOL bSuccess = FALSE;
    INT nRecv = 0;

    TRACE("\n");

    FD_ZERO(&infd);
    FD_SET(nSocket, &infd);
    tv.tv_sec=RESPONSE_TIMEOUT;
    tv.tv_usec=0;

    while (nRecv < *dwBuffer)
    {
        if (select(nSocket+1,&infd,NULL,NULL,&tv) > 0)
        {
            if (recv(nSocket, &lpszBuffer[nRecv], 1, 0) <= 0)
            {
                INTERNET_SetLastError(ERROR_FTP_TRANSFER_IN_PROGRESS);
                goto lend;
            }

            if (lpszBuffer[nRecv] == '\n')
	    {
		bSuccess = TRUE;
                break;
	    }
            if (lpszBuffer[nRecv] != '\r')
                nRecv++;
        }
	else
	{
            INTERNET_SetLastError(ERROR_INTERNET_TIMEOUT);
            goto lend;
        }
    }

lend:
    if (bSuccess)
    {
        lpszBuffer[nRecv] = '\0';
	*dwBuffer = nRecv - 1;
        TRACE(":%d %s\n", nRecv, lpszBuffer);
        return lpszBuffer;
    }
    else
    {
        return NULL;
    }
}

/***********************************************************************
 *
 */
BOOL WINAPI InternetQueryDataAvailable( HINTERNET hFile,
                                LPDWORD lpdwNumberOfBytesAvailble,
                                DWORD dwFlags, DWORD dwConext)
{
    LPWININETHTTPREQA lpwhr = (LPWININETHTTPREQA) hFile;
    INT retval = -1;
    char buffer[4048];


    if (NULL == lpwhr)
    {
        SetLastError(ERROR_NO_MORE_FILES);
        return FALSE;
    }

    TRACE("-->  %p %i\n",lpwhr,lpwhr->hdr.htype);

    switch (lpwhr->hdr.htype)
    {
    case WH_HHTTPREQ:
        if (!NETCON_recv(&((LPWININETHTTPREQA)hFile)->netConnection, buffer,
                         4048, MSG_PEEK, (int *)lpdwNumberOfBytesAvailble))
        {
            SetLastError(ERROR_NO_MORE_FILES);
            retval = FALSE;
        }
        else
            retval = TRUE;
        break;

    default:
        FIXME("unsuported file type\n");
        break;
    }

    TRACE("<-- %i\n",retval);
    return (retval+1);
}


/***********************************************************************
 *
 */
BOOL WINAPI InternetLockRequestFile( HINTERNET hInternet, HANDLE
*lphLockReqHandle)
{
    FIXME("STUB\n");
    return FALSE;
}

BOOL WINAPI InternetUnlockRequestFile( HANDLE hLockHandle)
{
    FIXME("STUB\n");
    return FALSE;
}


/***********************************************************************
 *           InternetAutodial
 *
 * On windows this function is supposed to dial the default internet
 * connection. We don't want to have Wine dial out to the internet so
 * we return TRUE by default. It might be nice to check if we are connected.
 *
 * RETURNS
 *   TRUE on success
 *   FALSE on failure
 *
 */
BOOL WINAPI InternetAutodial(DWORD dwFlags, HWND hwndParent)
{
    FIXME("STUB\n");

    /* Tell that we are connected to the internet. */
    return TRUE;
}

/***********************************************************************
 *           InternetAutodialHangup
 *
 * Hangs up an connection made with InternetAutodial
 *
 * PARAM
 *    dwReserved
 * RETURNS
 *   TRUE on success
 *   FALSE on failure
 *
 */
BOOL WINAPI InternetAutodialHangup(DWORD dwReserved)
{
    FIXME("STUB\n");

    /* we didn't dial, we don't disconnect */
    return TRUE;
}

/***********************************************************************
 *
 *         InternetCombineUrlA
 *
 * Combine a base URL with a relative URL
 *
 * RETURNS
 *   TRUE on success
 *   FALSE on failure
 *
 */

BOOL WINAPI InternetCombineUrlA(LPCSTR lpszBaseUrl, LPCSTR lpszRelativeUrl,
                                LPSTR lpszBuffer, LPDWORD lpdwBufferLength,
                                DWORD dwFlags)
{
    HRESULT hr=S_OK;
    /* Flip this bit to correspond to URL_ESCAPE_UNSAFE */
    dwFlags ^= ICU_NO_ENCODE;
    hr=UrlCombineA(lpszBaseUrl,lpszRelativeUrl,lpszBuffer,lpdwBufferLength,dwFlags);

    return (hr==S_OK);
}

/***********************************************************************
 *
 *         InternetCombineUrlW
 *
 * Combine a base URL with a relative URL
 *
 * RETURNS
 *   TRUE on success
 *   FALSE on failure
 *
 */

BOOL WINAPI InternetCombineUrlW(LPCWSTR lpszBaseUrl, LPCWSTR lpszRelativeUrl,
                                LPWSTR lpszBuffer, LPDWORD lpdwBufferLength,
                                DWORD dwFlags)
{
    HRESULT hr=S_OK;
    /* Flip this bit to correspond to URL_ESCAPE_UNSAFE */
    dwFlags ^= ICU_NO_ENCODE;
    hr=UrlCombineW(lpszBaseUrl,lpszRelativeUrl,lpszBuffer,lpdwBufferLength,dwFlags);

    return (hr==S_OK);
}
