/*
 * Wininet - Utility functions
 *
 * Copyright 1999 Corel Corporation
 * Copyright 2002 CodeWeavers Inc.
 *
 * Ulrich Czekalla
 * Aric Stewart
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
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "windef.h"
#include "winbase.h"
#include "wininet.h"
#include "winerror.h"

#include "wine/debug.h"
#include "internet.h"

WINE_DEFAULT_DEBUG_CHANNEL(wininet);

#define TIME_STRING_LEN  30

time_t ConvertTimeString(LPCSTR asctime)
{
    char tmpChar[TIME_STRING_LEN];
    char *tmpChar2;
    struct tm t;
    int timelen = strlen(asctime);

    if(!asctime || !timelen)
        return 0;

    strncpy(tmpChar, asctime, TIME_STRING_LEN);

    /* Assert that the string is the expected length */
    if (tmpChar[TIME_STRING_LEN] != '\0')
    {
        tmpChar[TIME_STRING_LEN] = '\0';
        FIXME("\n");
    }

    /* Convert a time such as 'Mon, 15 Nov 1999 16:09:35 GMT' into a SYSTEMTIME structure
     * We assume the time is in this format
     * and divide it into easy to swallow chunks
     */
    tmpChar[3]='\0';
    tmpChar[7]='\0';
    tmpChar[11]='\0';
    tmpChar[16]='\0';
    tmpChar[19]='\0';
    tmpChar[22]='\0';
    tmpChar[25]='\0';

    t.tm_year = atoi(tmpChar+12) - 1900;
    t.tm_mday = atoi(tmpChar+5);
    t.tm_hour = atoi(tmpChar+17);
    t.tm_min = atoi(tmpChar+20);
    t.tm_sec = atoi(tmpChar+23);

    /* and month */
    tmpChar2 = tmpChar + 8;
    switch(tmpChar2[2])
    {
        case 'n':
            if(tmpChar2[1]=='a')
                t.tm_mon = 0;
            else
                t.tm_mon = 5;
            break;
        case 'b':
            t.tm_mon = 1;
            break;
        case 'r':
            if(tmpChar2[1]=='a')
                t.tm_mon = 2;
            else
                t.tm_mon = 3;
            break;
        case 'y':
            t.tm_mon = 4;
            break;
        case 'l':
            t.tm_mon = 6;
            break;
        case 'g':
            t.tm_mon = 7;
            break;
        case 'p':
            t.tm_mon = 8;
            break;
        case 't':
            t.tm_mon = 9;
            break;
        case 'v':
            t.tm_mon = 10;
            break;
        case 'c':
            t.tm_mon = 11;
            break;
        default:
            FIXME("\n");
    }

    return mktime(&t);
}


BOOL GetAddress(LPCSTR lpszServerName, INTERNET_PORT nServerPort,
	struct hostent **phe, struct sockaddr_in *psa)
{
    char *found;

    TRACE("%s\n", lpszServerName);

    /* Validate server name first
     * Check if there is sth. like
     * pinger.macromedia.com:80
     * if yes, eliminate the :80....
     */
    found = strchr(lpszServerName, ':');
    if (found)
    {
        int len = found - lpszServerName;
        char *new = HeapAlloc(GetProcessHeap(), 0, len + 1);
        memcpy( new, lpszServerName, len );
        new[len] = '\0';
        TRACE("Found a ':' inside the server name, reparsed name: %s\n", new);
        *phe = gethostbyname(new);
        HeapFree( GetProcessHeap(), 0, new );
    }
    else *phe = gethostbyname(lpszServerName);

    if (NULL == *phe)
    {
        TRACE("Failed to get hostname: (%s)\n", lpszServerName);
        return FALSE;
    }

    memset(psa,0,sizeof(struct sockaddr_in));
    memcpy((char *)&psa->sin_addr, (*phe)->h_addr, (*phe)->h_length);
    psa->sin_family = (*phe)->h_addrtype;
    psa->sin_port = htons((u_short)nServerPort);

    return TRUE;
}

/*
 * Helper function for sending async Callbacks
 */

static const char *get_callback_name(DWORD dwInternetStatus) {
    static const wininet_flag_info internet_status[] = {
#define FE(x) { x, #x }
	FE(INTERNET_STATUS_RESOLVING_NAME),
	FE(INTERNET_STATUS_NAME_RESOLVED),
	FE(INTERNET_STATUS_CONNECTING_TO_SERVER),
	FE(INTERNET_STATUS_CONNECTED_TO_SERVER),
	FE(INTERNET_STATUS_SENDING_REQUEST),
	FE(INTERNET_STATUS_REQUEST_SENT),
	FE(INTERNET_STATUS_RECEIVING_RESPONSE),
	FE(INTERNET_STATUS_RESPONSE_RECEIVED),
	FE(INTERNET_STATUS_CTL_RESPONSE_RECEIVED),
	FE(INTERNET_STATUS_PREFETCH),
	FE(INTERNET_STATUS_CLOSING_CONNECTION),
	FE(INTERNET_STATUS_CONNECTION_CLOSED),
	FE(INTERNET_STATUS_HANDLE_CREATED),
	FE(INTERNET_STATUS_HANDLE_CLOSING),
	FE(INTERNET_STATUS_REQUEST_COMPLETE),
	FE(INTERNET_STATUS_REDIRECT),
	FE(INTERNET_STATUS_INTERMEDIATE_RESPONSE),
	FE(INTERNET_STATUS_USER_INPUT_REQUIRED),
	FE(INTERNET_STATUS_STATE_CHANGE),
	FE(INTERNET_STATUS_COOKIE_SENT),
	FE(INTERNET_STATUS_COOKIE_RECEIVED),
	FE(INTERNET_STATUS_PRIVACY_IMPACTED),
	FE(INTERNET_STATUS_P3P_HEADER),
	FE(INTERNET_STATUS_P3P_POLICYREF),
	FE(INTERNET_STATUS_COOKIE_HISTORY),
	FE(INTERNET_STATE_CONNECTED),
	FE(INTERNET_STATE_DISCONNECTED),
	FE(INTERNET_STATE_DISCONNECTED_BY_USER),
	FE(INTERNET_STATE_IDLE),
	FE(INTERNET_STATE_BUSY)
#undef FE
    };
    int i;

    for (i = 0; i < (sizeof(internet_status) / sizeof(internet_status[0])); i++) {
	if (internet_status[i].val == dwInternetStatus) return internet_status[i].name;
    }
    return "Unknown";
}

VOID SendAsyncCallbackInt(LPWININETAPPINFOA hIC, HINTERNET hHttpSession,
                             DWORD dwContext, DWORD dwInternetStatus, LPVOID
                             lpvStatusInfo, DWORD dwStatusInfoLength)
{
        if (! (hIC->lpfnStatusCB))
            return;

        /* the IE5 version of wininet does not
           send callbacks if dwContext is zero */
        if( !dwContext )
            return;

        TRACE("--> Callback %ld (%s)\n",dwInternetStatus, get_callback_name(dwInternetStatus));

        hIC->lpfnStatusCB(hHttpSession, dwContext, dwInternetStatus,
                          lpvStatusInfo, dwStatusInfoLength);

        TRACE("<-- Callback %ld (%s)\n",dwInternetStatus, get_callback_name(dwInternetStatus));
}



VOID SendAsyncCallback(LPWININETAPPINFOA hIC, HINTERNET hHttpSession,
                             DWORD dwContext, DWORD dwInternetStatus, LPVOID
                             lpvStatusInfo,  DWORD dwStatusInfoLength)
{
        TRACE("Send Callback %ld (%s)\n",dwInternetStatus, get_callback_name(dwInternetStatus));

        if (! (hIC->lpfnStatusCB))
            return;
        if (hIC->hdr.dwFlags & INTERNET_FLAG_ASYNC)
        {
            WORKREQUEST workRequest;
            struct WORKREQ_SENDCALLBACK *req;

            workRequest.asyncall = SENDCALLBACK;
            workRequest.handle = hIC;
            req = &workRequest.u.SendCallback;
            req->hHttpSession = hHttpSession;
            req->dwContext = dwContext;
            req->dwInternetStatus = dwInternetStatus;
            req->lpvStatusInfo = lpvStatusInfo;
            req->dwStatusInfoLength = dwStatusInfoLength;

            INTERNET_AsyncCall(&workRequest);
        }
        else
            SendAsyncCallbackInt(hIC, hHttpSession, dwContext, dwInternetStatus,
                                  lpvStatusInfo, dwStatusInfoLength);
}
