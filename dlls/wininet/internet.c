/*
 * Wininet
 *
 * Copyright 1999 Corel Corporation
 *
 * Ulrich Czekalla
 *
 */

#include "config.h"

#include <string.h>
#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#include <stdlib.h>

#include "windows.h"
#include "wininet.h"
#include "debugtools.h"
#include "winerror.h"
#include "winsock.h"

#include "internet.h"

DEFAULT_DEBUG_CHANNEL(wininet);

#define MAX_IDLE_WORKER 1000*60*1
#define MAX_WORKER_THREADS 10

#define GET_HWININET_FROM_LPWININETFINDNEXT(lpwh) \
(LPWININETAPPINFOA)(((LPWININETFTPSESSIONA)(lpwh->hdr.lpwhparent))->hdr.lpwhparent)

typedef struct
{
    DWORD  dwError;
    CHAR   response[MAX_REPLY_LEN];
} WITHREADERROR, *LPWITHREADERROR;

INTERNET_SCHEME GetInternetScheme(LPSTR lpszScheme);
BOOL WINAPI INTERNET_FindNextFileA(HINTERNET hFind, LPVOID lpvFindData);
VOID INTERNET_ExecuteWork();

DWORD g_dwTlsErrIndex = TLS_OUT_OF_INDEXES;
DWORD dwNumThreads;
DWORD dwNumIdleThreads;
HANDLE hEventArray[2];
#define hQuitEvent hEventArray[0]
#define hWorkEvent hEventArray[1]
CRITICAL_SECTION csQueue;
LPWORKREQUEST lpHeadWorkQueue;
LPWORKREQUEST lpWorkQueueTail;

/***********************************************************************
 * WININET_LibMain [Internal] Initializes the internal 'WININET.DLL'.
 *
 * PARAMS
 *     hinstDLL    [I] handle to the 'dlls' instance
 *     fdwReason   [I]
 *     lpvReserved [I] reserverd, must be NULL
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL WINAPI
WININET_LibMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("%x,%lx,%p\n", hinstDLL, fdwReason, lpvReserved);

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
	        HeapFree(GetProcessHeap(), 0, TlsGetValue(g_dwTlsErrIndex));
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
 *           InternetOpenA   (WININET.113)
 *
 * Per-application initialization of wininet
 *
 * RETURNS
 *    HINTERNET on success
 *    NULL on failure
 *
 */
INTERNETAPI HINTERNET WINAPI InternetOpenA(LPCSTR lpszAgent, 
	DWORD dwAccessType, LPCSTR lpszProxy,
    	LPCSTR lpszProxyBypass, DWORD dwFlags)
{
    LPWININETAPPINFOA lpwai = NULL;

    TRACE("\n");

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
            lpwai->lpszAgent = strdup(lpszAgent);
        if (NULL != lpszProxy)
            lpwai->lpszProxy = strdup(lpszProxy);
        if (NULL != lpszProxyBypass)
            lpwai->lpszProxyBypass = strdup(lpszProxyBypass);
        lpwai->dwAccessType = dwAccessType;
    }

    return (HINTERNET)lpwai;
}


/***********************************************************************
 *           InternetGetLastResponseInfoA (WININET.108)
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
 *           InternetConnectA (WININET.93)
 *
 * Open a ftp, gopher or http session
 *
 * RETURNS
 *    HINTERNET a session handle on success
 *    NULL on failure
 *
 */
INTERNETAPI HINTERNET WINAPI InternetConnectA(HINTERNET hInternet,
    LPCSTR lpszServerName, INTERNET_PORT nServerPort,
    LPCSTR lpszUserName, LPCSTR lpszPassword,
    DWORD dwService, DWORD dwFlags, DWORD dwContext)
{
    HINTERNET rc = (HINTERNET) NULL;

    TRACE("\n");

    /* Clear any error information */
    INTERNET_SetLastError(0);

    switch (dwService)
    {
        case INTERNET_SERVICE_FTP:
            rc = FTP_Connect(hInternet, lpszServerName, nServerPort,
            lpszUserName, lpszPassword, dwFlags, dwContext);
            break;

        case INTERNET_SERVICE_HTTP:
            break;

        case INTERNET_SERVICE_GOPHER:
        default:
            break;
    }

    return rc;
}

/***********************************************************************
 *           InternetFindNextFileA (WININET.102)
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
    if (hIC->hdr.dwFlags & INTERNET_FLAG_ASYNC && hIC->lpfnStatusCB)
    {	
        INTERNET_ASYNC_RESULT iar;

        iar.dwResult = (DWORD)bSuccess;
        iar.dwError = bSuccess ? ERROR_SUCCESS : INTERNET_GetLastError();

        hIC->lpfnStatusCB(hFind, lpwh->hdr.dwContext, INTERNET_STATUS_REQUEST_COMPLETE,
        &iar, sizeof(INTERNET_ASYNC_RESULT));
    }

    return bSuccess;
}


/***********************************************************************
 *           InternetCloseHandle (WININET.89)
 *
 * Continues a file search from a previous call to FindFirstFile
 *
 * RETURNS
 *    TRUE on success
 *    FALSE on failure
 *
 */
BOOL WINAPI InternetCloseHandle(HINTERNET hInternet)
{
    BOOL retval = FALSE;
    LPWININETHANDLEHEADER lpwh = (LPWININETHANDLEHEADER) hInternet;

    TRACE("\n");
    if (NULL == lpwh)
        return FALSE;

    /* Clear any error information */
    INTERNET_SetLastError(0);

    switch (lpwh->htype)
    {
        case WH_HINIT:
        case WH_HHTTPSESSION:
        case WH_HHTTPREQ:
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

    return retval;
}


/***********************************************************************
 *           InternetCrackUrlA (WININET.95)
 *
 * Break up URL into its components
 *
 * RETURNS
 *    TRUE on success
 *    FALSE on failure
 *
 */
BOOL WINAPI InternetCrackUrlA(LPCSTR lpszUrl, DWORD dwUrlLength, DWORD dwFlags, 
		LPURL_COMPONENTSA lpUrlComponents)
{
  /*
   * RFC 1808
   * <protocol>:[//<net_loc>][/path][;<params>][?<query>][#<fragment>]
   *
   */
    char* szScheme   = NULL;
    char* szUser     = NULL;
    char* szPass     = NULL;
    char* szHost     = NULL;
    char* szUrlPath  = NULL;
    char* szParam    = NULL;
    char* szNetLoc   = NULL;
    int   nPort      = 80;
    int   nSchemeLen = 0;
    int   nUserLen   = 0;
    int   nPassLen   = 0;
    int   nHostLen   = 0;
    int   nUrlLen    = 0;

    /* Find out if the URI is absolute... */
    BOOL  bIsAbsolute = FALSE;
    char  cAlphanum;
    char* ap = (char*)lpszUrl;
    char* cp = NULL;

    TRACE("\n");
    while( (cAlphanum = *ap) != '\0' )
    {
        if( ((cAlphanum >= 'a') && (cAlphanum <= 'z')) ||
            ((cAlphanum >= 'A') && (cAlphanum <= 'Z')) ||
            ((cAlphanum >= '0') && (cAlphanum <= '9')) )
        {
            ap++;
            continue;
        }
        if( (cAlphanum == ':') && (ap - lpszUrl >= 2) )
        {
            bIsAbsolute = TRUE;
            cp = ap;
            break;
        }
        break;
    }

    /* Absolute URI...
       FIXME!!!! This should work on relative urls too!*/
    if( bIsAbsolute )
    {
        /* Get scheme first... */
        nSchemeLen = cp - lpszUrl;
        szScheme   = strdup( lpszUrl );
        szScheme[ nSchemeLen ] = '\0';

        /* Eat ':' in protocol... */
        cp++;

        /* Parse <params>... */
        szParam = strpbrk( lpszUrl, ";" );
        if( szParam != NULL )
        {
            char* sParam;
            /* Eat ';' in Params... */
            szParam++;
            sParam    = strdup( szParam );
            *szParam = '\0';
        }

        /* Skip over slashes...*/
        if( *cp == '/' )
        {
            cp++;
            if( *cp == '/' )
            {
                cp++;
                if( *cp == '/' )
                    cp++;
            }
        }

        /* Parse the <net-loc>...*/
        if( GetInternetScheme( szScheme ) == INTERNET_SCHEME_FILE )
        {
            szUrlPath = strdup( cp );
            nUrlLen   = strlen( szUrlPath );
            if( nUrlLen >= 2 && szUrlPath[ 1 ] == '|' )
                szUrlPath[ 1 ] = ':';
        }
        else
        {
            size_t nNetLocLen;
            szUrlPath = strpbrk(cp, "/");
            if( szUrlPath != NULL )
                nUrlLen = strlen( szUrlPath );

            /* Find the end of our net-loc... */
            nNetLocLen = strcspn( cp, "/" );
            szNetLoc   = strdup( cp );
            szNetLoc[ nNetLocLen ] = '\0';
            if( szNetLoc != NULL )
            {
                char* lpszPort;
                int   nPortLen;
                /* [<user>[<:password>]@]<host>[:<port>] */
                /* First find the user and password if they exist...*/
			
                szHost = strchr( szNetLoc, '@' );
                if( szHost == NULL )
                {
                    /* username and password not specified... */
                    szHost   = szNetLoc;
                    nHostLen = nNetLocLen;
                }
                else
                {
                    int   nUserPassLen = nNetLocLen - nHostLen - 1;
		    char* szUserPass         = strdup( szNetLoc );
		    /* Get username and/or password... */
		    /* Eat '@' in domain... */
                    ++szHost;
                    nHostLen = strlen( szHost );

                    szUserPass[ nUserPassLen ] = '\0';
                    if( szUserPass != NULL )
                    {
		        szPass = strpbrk( szUserPass, ":" );
                        if( szPass != NULL )
                        {
                            /* Eat ':' in UserPass... */
                            ++szPass;
                            nPassLen = strlen( szPass );
                            nUserLen = nUserPassLen - nPassLen - 1;
                            szUser   = strdup( szUserPass );
                            szUser[ nUserLen ] = '\0';
                        }
                        else
                        {
                            /* password not specified... */
                            szUser = strdup( szUserPass );
                            nUserLen = strlen( szUser );
                        }
                    }        
                }

                /* <host><:port>...*/
                /* Then get the port if it exists... */
                lpszPort = strpbrk( szHost, ":" );
                nPortLen = 0;
                if( lpszPort != NULL )
                {
                    char* szPort = lpszPort + 1;
		    if( szPort != NULL )
		    {
                        nPortLen = strlen( szPort );
                        nPort    = atoi( szPort );
                    }
                    *lpszPort = '\0';
                    nHostLen = strlen(szHost);
                }
            }
        }
    }
    /* Relative URI... */
    else
        return FALSE;

    return TRUE;
}


/***********************************************************************
 *           InternetAttemptConnect (WININET.81)
 *
 * Attempt to make a connection to the internet
 *
 * RETURNS
 *    ERROR_SUCCESS on success
 *    Error value   on failure
 *
 */
INTERNETAPI DWORD WINAPI InternetAttemptConnect(DWORD dwReserved)
{
    FIXME("Stub\n");
    return ERROR_SUCCESS;
}


/***********************************************************************
 *           InternetCanonicalizeUrlA (WININET.85)
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
    BOOL bSuccess = FALSE;

    FIXME("Stub!\n");

    if (lpszUrl)
    {
        strncpy(lpszBuffer, lpszUrl, *lpdwBufferLength);
        *lpdwBufferLength = strlen(lpszBuffer);
	bSuccess = TRUE;
    }

    return bSuccess;
}


/***********************************************************************
 *           InternetSetStatusCallback (WININET.133)
 *
 * Sets up a callback function which is called as progress is made
 * during an operation. 
 *
 * RETURNS
 *    Previous callback or NULL 	on success
 *    INTERNET_INVALID_STATUS_CALLBACK  on failure
 *
 */
INTERNETAPI INTERNET_STATUS_CALLBACK WINAPI InternetSetStatusCallback(
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
 *           InternetWriteFile (WININET.138)
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
    int nSocket = INVALID_SOCKET;
    LPWININETHANDLEHEADER lpwh = (LPWININETHANDLEHEADER) hFile;

    TRACE("\n");
    if (NULL == lpwh)
        return FALSE;

    switch (lpwh->htype)
    {
        case WH_HHTTPREQ:
            nSocket = ((LPWININETHTTPREQA)hFile)->nSocketFD; 
            break;

        case WH_HFILE:
            nSocket = ((LPWININETFILE)hFile)->nDataSocket;
            break;

        default:
            break;
    }

    if (INVALID_SOCKET != nSocket)
    {
        *lpdwNumOfBytesWritten = INTERNET_WriteDataToStream(nSocket, lpBuffer, dwNumOfBytesToWrite);
        if (*lpdwNumOfBytesWritten < 0)
            *lpdwNumOfBytesWritten = 0;
        else
            retval = TRUE;
    }

    return retval;
}


/***********************************************************************
 *           InternetReadFile (WININET.121)
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
    int nSocket = INVALID_SOCKET;
    LPWININETHANDLEHEADER lpwh = (LPWININETHANDLEHEADER) hFile;

    TRACE("\n");
    if (NULL == lpwh)
        return FALSE;

    switch (lpwh->htype)
    {
        case WH_HHTTPREQ:
            nSocket = ((LPWININETHTTPREQA)hFile)->nSocketFD; 
            break;

        case WH_HFILE:
            nSocket = ((LPWININETFILE)hFile)->nDataSocket;
            break;

        default:
            break;
    }

    if (INVALID_SOCKET != nSocket)
    {
        *dwNumOfBytesRead = INTERNET_ReadDataFromStream(nSocket, lpBuffer, dwNumOfBytesToRead);
        if (*dwNumOfBytesRead < 0)
            *dwNumOfBytesRead = 0;
        else
            retval = TRUE;
    }

    return retval;
}


/***********************************************************************
 *           GetInternetScheme (internal)
 *
 * Get scheme of url
 *
 * RETURNS
 *    scheme on success
 *    INTERNET_SCHEME_UNKNOWN on failure
 *
 */
INTERNET_SCHEME GetInternetScheme(LPSTR lpszScheme)
{
    if(lpszScheme==NULL)
        return INTERNET_SCHEME_UNKNOWN;

    if( (strcmp("ftp", lpszScheme) == 0) ||
        (strcmp("FTP", lpszScheme) == 0) )
        return INTERNET_SCHEME_FTP;
    else if( (strcmp("gopher", lpszScheme) == 0) ||
        (strcmp("GOPHER", lpszScheme) == 0) )
        return INTERNET_SCHEME_GOPHER;
    else if( (strcmp("http", lpszScheme) == 0) ||
        (strcmp("HTTP", lpszScheme) == 0) )
        return INTERNET_SCHEME_HTTP;
    else if( (strcmp("https", lpszScheme) == 0) ||
        (strcmp("HTTPS", lpszScheme) == 0) )
        return INTERNET_SCHEME_HTTPS;
    else if( (strcmp("file", lpszScheme) == 0) ||
        (strcmp("FILE", lpszScheme) == 0) )
        return INTERNET_SCHEME_FILE;
    else if( (strcmp("news", lpszScheme) == 0) ||
        (strcmp("NEWS", lpszScheme) == 0) )
        return INTERNET_SCHEME_NEWS;
    else if( (strcmp("mailto", lpszScheme) == 0) ||
        (strcmp("MAILTO", lpszScheme) == 0) )
        return INTERNET_SCHEME_MAILTO;
    else
        return INTERNET_SCHEME_UNKNOWN;
}


/***********************************************************************
 *           INTERNET_WriteDataToStream (internal)
 *
 * Send data to server
 *
 * RETURNS
 *
 *   number of characters sent on success
 *   -1 on error
 */
int INTERNET_WriteDataToStream(int nDataSocket, LPCVOID Buffer, DWORD BytesToWrite)
{
    if (INVALID_SOCKET == nDataSocket)
        return SOCKET_ERROR;

    return send(nDataSocket, Buffer, BytesToWrite, 0);
}


/***********************************************************************
 *           INTERNET_ReadDataFromStream (internal)
 *
 * Read data from http server
 *
 * RETURNS
 *
 *   number of characters sent on success
 *   -1 on error
 */
int INTERNET_ReadDataFromStream(int nDataSocket, LPVOID Buffer, DWORD BytesToRead)
{
    if (INVALID_SOCKET == nDataSocket)
        return SOCKET_ERROR;

    return recv(nDataSocket, Buffer, BytesToRead, 0);
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
    }

    return bSuccess;
}


/***********************************************************************
 *           INTERNET_GetWokkRequest (internal)
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
    return lpwite->response;
}
