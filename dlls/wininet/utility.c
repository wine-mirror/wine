/* 
 * Wininet - Utility functions
 *
 * Copyright 1999 Corel Corporation
 *
 * Ulrich Czekalla
 *
 */

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "windows.h"
#include "wininet.h"
#include "winerror.h"
#include "winsock.h"

#include "debugtools.h"
#include "internet.h"

DEFAULT_DEBUG_CHANNEL(wininet);

#define TIME_STRING_LEN  30

time_t ConvertTimeString(LPCSTR asctime)
{
    char tmpChar[TIME_STRING_LEN];
    char *tmpChar2;
    struct tm SystemTime;
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

    SystemTime.tm_year = atoi(tmpChar+12) - 1900;
    SystemTime.tm_mday = atoi(tmpChar+5);
    SystemTime.tm_hour = atoi(tmpChar+17);
    SystemTime.tm_min = atoi(tmpChar+20);
    SystemTime.tm_sec = atoi(tmpChar+23);
	
    /* and month */
    tmpChar2 = tmpChar + 8;
    switch(tmpChar2[2])
    {
        case 'n':
            if(tmpChar2[1]=='a')
                SystemTime.tm_mon = 0;
            else
                SystemTime.tm_mon = 5;
            break;
        case 'b':
            SystemTime.tm_mon = 1;
            break;
        case 'r':
            if(tmpChar2[1]=='a')
                SystemTime.tm_mon = 2;
            else
                SystemTime.tm_mon = 3;
            break;
        case 'y':
            SystemTime.tm_mon = 4;
            break;
        case 'l':
            SystemTime.tm_mon = 6;
            break;
        case 'g':
            SystemTime.tm_mon = 7;
            break;
        case 'p':
            SystemTime.tm_mon = 8;
            break;
        case 't':
            SystemTime.tm_mon = 9;
            break;
        case 'v':
            SystemTime.tm_mon = 10;
            break;
        case 'c':
            SystemTime.tm_mon = 11;
            break;
        default:
            FIXME("\n");
    }

    return mktime(&SystemTime);
}


BOOL GetAddress(LPCSTR lpszServerName, INTERNET_PORT nServerPort,
	struct hostent **phe, struct sockaddr_in *psa)
{
    TRACE("%s\n", lpszServerName);

    *phe = gethostbyname(lpszServerName);
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
