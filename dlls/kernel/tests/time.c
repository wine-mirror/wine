/*
 * Unit test suite for time functions
 *
 * Copyright 2004 Uwe Bonnes
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

#include "wine/test.h"
#include "winbase.h"

#define SECSPERMIN         60
#define SECSPERDAY        86400
/* 1601 to 1970 is 369 years plus 89 leap days */
#define SECS_1601_TO_1970  ((369 * 365 + 89) * (ULONGLONG)SECSPERDAY)
#define TICKSPERSEC       10000000
#define TICKSPERMSEC      10000
#define TICKS_1601_TO_1970 (SECS_1601_TO_1970 * TICKSPERSEC)


void test_GetTimeZoneInformation()
{
    TIME_ZONE_INFORMATION tzinfo, tzinfo1;
    DWORD res =  GetTimeZoneInformation(&tzinfo);
    ok(res != TIME_ZONE_ID_INVALID, "GetTimeZoneInformation failed\n");
    ok(SetEnvironmentVariableA("TZ","GMT0") != 0,
       "SetEnvironmentVariableA failed\n");
    res =  GetTimeZoneInformation(&tzinfo1);
    ok(res != TIME_ZONE_ID_INVALID, "GetTimeZoneInformation failed\n");

    ok(((tzinfo.Bias == tzinfo1.Bias) && 
	(tzinfo.StandardBias == tzinfo1.StandardBias) &&
	(tzinfo.DaylightBias == tzinfo1.DaylightBias)),
       "Bias influenced by TZ variable\n"); 
    ok(SetEnvironmentVariableA("TZ",NULL) != 0,
       "SetEnvironmentVariableA failed\n");
        
}

void test_FileTimeToSystemTime()
{
    FILETIME ft;
    SYSTEMTIME st;
    ULONGLONG time = (ULONGLONG)TICKSPERSEC + TICKS_1601_TO_1970;

    ft.dwHighDateTime = 0;
    ft.dwLowDateTime  = 0;
    ok(FileTimeToSystemTime(&ft, &st),
       "FileTimeToSystemTime() failed with Error 0x%08lx\n",GetLastError());
    ok(((st.wYear == 1601) && (st.wMonth  == 1) && (st.wDay    == 1) &&
	(st.wHour ==    0) && (st.wMinute == 0) && (st.wSecond == 0) &&
	(st.wMilliseconds == 0)),
	"Got Year %4d Month %2d Day %2d\n",  st.wYear, st.wMonth, st.wDay);

    ft.dwHighDateTime = (UINT)(time >> 32);
    ft.dwLowDateTime  = (UINT)time;
    ok(FileTimeToSystemTime(&ft, &st),
       "FileTimeToSystemTime() failed with Error 0x%08lx\n",GetLastError());
    ok(((st.wYear == 1970) && (st.wMonth == 1) && (st.wDay == 1) &&
	(st.wHour ==    0) && (st.wMinute == 0) && (st.wSecond == 1) &&
	(st.wMilliseconds == 0)),
       "Got Year %4d Month %2d Day %2d Hour %2d Min %2d Sec %2d mSec %3d\n",
       st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond,
       st.wMilliseconds);
}

void test_FileTimeToLocalFileTime()
{
    FILETIME ft, lft;
    SYSTEMTIME st;
    TIME_ZONE_INFORMATION tzinfo;
    DWORD res =  GetTimeZoneInformation(&tzinfo);
    ULONGLONG time = (ULONGLONG)TICKSPERSEC + TICKS_1601_TO_1970 +
        (LONGLONG)(tzinfo.Bias + 
            ( res == TIME_ZONE_ID_STANDARD ? tzinfo.StandardBias :
            ( res == TIME_ZONE_ID_DAYLIGHT ? tzinfo.DaylightBias : 0 ))) *
             SECSPERMIN *TICKSPERSEC;
    ok( res != TIME_ZONE_ID_INVALID , "GetTimeZoneInformation failed\n");
    ft.dwHighDateTime = (UINT)(time >> 32);
    ft.dwLowDateTime  = (UINT)time;
    ok(FileTimeToLocalFileTime(&ft, &lft) !=0 ,
       "FileTimeToLocalFileTime() failed with Error 0x%08lx\n",GetLastError());
    FileTimeToSystemTime(&lft, &st);
    ok(((st.wYear == 1970) && (st.wMonth == 1) && (st.wDay == 1) &&
	(st.wHour ==    0) && (st.wMinute == 0) && (st.wSecond == 1) &&
	(st.wMilliseconds == 0)),
       "Got Year %4d Month %2d Day %2d Hour %2d Min %2d Sec %2d mSec %3d\n",
       st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond,
       st.wMilliseconds);

    ok(SetEnvironmentVariableA("TZ","GMT") != 0,
       "SetEnvironmentVariableA failed\n");
    ok(res != TIME_ZONE_ID_INVALID, "GetTimeZoneInformation failed\n");
    ok(FileTimeToLocalFileTime(&ft, &lft) !=0 ,
       "FileTimeToLocalFileTime() failed with Error 0x%08lx\n",GetLastError());
    FileTimeToSystemTime(&lft, &st);
    ok(((st.wYear == 1970) && (st.wMonth == 1) && (st.wDay == 1) &&
	(st.wHour ==    0) && (st.wMinute == 0) && (st.wSecond == 1) &&
	(st.wMilliseconds == 0)),
       "Got Year %4d Month %2d Day %2d Hour %2d Min %2d Sec %2d mSec %3d\n",
       st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond,
       st.wMilliseconds);
    ok(SetEnvironmentVariableA("TZ",NULL) != 0,
       "SetEnvironmentVariableA failed\n");
}


/* test TzSpecificLocalTimeToSystemTime and SystemTimeToTzSpecificLocalTime
 * these are in winXP and later */
typedef HANDLE (WINAPI *fnTzSpecificLocalTimeToSystemTime)( LPTIME_ZONE_INFORMATION, LPSYSTEMTIME, LPSYSTEMTIME);
typedef HANDLE (WINAPI *fnSystemTimeToTzSpecificLocalTime)( LPTIME_ZONE_INFORMATION, LPSYSTEMTIME, LPSYSTEMTIME);

typedef struct {
    int nr;             /* test case number for easier lookup */
    TIME_ZONE_INFORMATION *ptz; /* ptr to timezone */
    SYSTEMTIME slt;     /* system/local time to convert */
    WORD ehour;        /* expected hour */
} TZLT2ST_case;

void test_TzSpecificLocalTimeToSystemTime()
{    
    HMODULE hKernel = GetModuleHandle("kernel32");
    fnTzSpecificLocalTimeToSystemTime pTzSpecificLocalTimeToSystemTime;
    fnSystemTimeToTzSpecificLocalTime pSystemTimeToTzSpecificLocalTime = NULL;
    TIME_ZONE_INFORMATION tzE, tzW, tzS;
    SYSTEMTIME result;
    int i;
    pTzSpecificLocalTimeToSystemTime = (fnTzSpecificLocalTimeToSystemTime) GetProcAddress( hKernel, "TzSpecificLocalTimeToSystemTime");
    if(pTzSpecificLocalTimeToSystemTime)
        pSystemTimeToTzSpecificLocalTime = (fnTzSpecificLocalTimeToSystemTime) GetProcAddress( hKernel, "SystemTimeToTzSpecificLocalTime");
    if( !pSystemTimeToTzSpecificLocalTime)
        return;
    ZeroMemory( &tzE, sizeof(tzE));
    ZeroMemory( &tzW, sizeof(tzW));
    ZeroMemory( &tzS, sizeof(tzS));
    /* timezone Eastern hemisphere */
    tzE.Bias=-600;
    tzE.StandardBias=0;
    tzE.DaylightBias=-60;
    tzE.StandardDate.wMonth=10;
    tzE.StandardDate.wDayOfWeek=0; /*sunday */
    tzE.StandardDate.wDay=5;       /* last (sunday) of the month */
    tzE.StandardDate.wHour=3;
    tzE.DaylightDate.wMonth=3;
    tzE.DaylightDate.wDay=5;
    tzE.DaylightDate.wHour=2;
    /* timezone Western hemisphere */
    tzW.Bias=240;
    tzW.StandardBias=0;
    tzW.DaylightBias=-60;
    tzW.StandardDate.wMonth=10;
    tzW.StandardDate.wDayOfWeek=0; /*sunday */
    tzW.StandardDate.wDay=4;       /* 4th (sunday) of the month */
    tzW.StandardDate.wHour=2;
    tzW.DaylightDate.wMonth=4;
    tzW.DaylightDate.wDay=1;
    tzW.DaylightDate.wHour=2;
    /* timezone Eastern hemisphere */
    tzS.Bias=240;
    tzS.StandardBias=0;
    tzS.DaylightBias=-60;
    tzS.StandardDate.wMonth=4;
    tzS.StandardDate.wDayOfWeek=0; /*sunday */
    tzS.StandardDate.wDay=1;       /* 1st  (sunday) of the month */
    tzS.StandardDate.wHour=2;
    tzS.DaylightDate.wMonth=10;
    tzS.DaylightDate.wDay=4;
    tzS.DaylightDate.wHour=2;
    /*tests*/
        /* TzSpecificLocalTimeToSystemTime */
    {   TZLT2ST_case cases[] = {
            /* standard->daylight transition */
            { 1, &tzE, {2004,3,-1,28,1,0,0,0}, 15 },
            { 2, &tzE, {2004,3,-1,28,1,59,59,999}, 15},
            { 3, &tzE, {2004,3,-1,28,2,0,0,0}, 15},
            /* daylight->standard transition */
            { 4, &tzE, {2004,10,-1,31,2,0,0,0} , 15 },
            { 5, &tzE, {2004,10,-1,31,2,59,59,999}, 15 },
            { 6, &tzE, {2004,10,-1,31,3,0,0,0}, 17 },
            /* West and with fixed weekday of the month */
            { 7, &tzW, {2004,4,-1,4,1,0,0,0}, 5},
            { 8, &tzW, {2004,4,-1,4,1,59,59,999}, 5},
            { 9, &tzW, {2004,4,-1,4,2,0,0,0}, 5},
            { 10, &tzW, {2004,10,-1,24,1,0,0,0}, 4},
            { 11, &tzW, {2004,10,-1,24,1,59,59,999}, 4},
            { 12, &tzW, {2004,10,-1,24,2,0,0,0 }, 6},
            /* and now south */
            { 13, &tzS, {2004,4,-1,4,1,0,0,0}, 4},
            { 14, &tzS, {2004,4,-1,4,1,59,59,999}, 4},
            { 15, &tzS, {2004,4,-1,4,2,0,0,0}, 6},
            { 16, &tzS, {2004,10,-1,24,1,0,0,0}, 5},
            { 17, &tzS, {2004,10,-1,24,1,59,59,999}, 5},
            { 18, &tzS, {2004,10,-1,24,2,0,0,0}, 5},
            {0}
       };
        for (i=0; cases[i].nr; i++) {
            pTzSpecificLocalTimeToSystemTime( cases[i].ptz, &(cases[i].slt), &result);
            ok( result.wHour == cases[i].ehour,
                    "Test TzSpecificLocalTimeToSystemTime #%d. wrong system time. Hour is %d expected %d\n", 
                    cases[i].nr, result.wHour, cases[i].ehour);
        }

    }
        /* SystemTimeToTzSpecificLocalTime */
    {   TZLT2ST_case cases[] = {
            /* standard->daylight transition */
            { 1, &tzE, {2004,3,-1,27,15,0,0,0}, 1 },
            { 2, &tzE, {2004,3,-1,27,15,59,59,999}, 1},
            { 3, &tzE, {2004,3,-1,27,16,0,0,0}, 3},
            /* daylight->standard transition */
            { 4,  &tzE, {2004,10,-1,30,15,0,0,0}, 2 },
            { 5, &tzE, {2004,10,-1,30,15,59,59,999}, 2 },
            { 6, &tzE, {2004,10,-1,30,16,0,0,0}, 2 },
            /* West and with fixed weekday of the month */
            { 7, &tzW, {2004,4,-1,4,5,0,0,0}, 1},
            { 8, &tzW, {2004,4,-1,4,5,59,59,999}, 1},
            { 9, &tzW, {2004,4,-1,4,6,0,0,0}, 3},
            { 10, &tzW, {2004,10,-1,24,4,0,0,0}, 1},
            { 11, &tzW, {2004,10,-1,24,4,59,59,999}, 1},
            { 12, &tzW, {2004,10,-1,24,5,0,0,0 }, 1},
            /* and now south */
            { 13, &tzS, {2004,4,-1,4,4,0,0,0}, 1},
            { 14, &tzS, {2004,4,-1,4,4,59,59,999}, 1},
            { 15, &tzS, {2004,4,-1,4,5,0,0,0}, 1},
            { 16, &tzS, {2004,10,-1,24,5,0,0,0}, 1},
            { 17, &tzS, {2004,10,-1,24,5,59,59,999}, 1},
            { 18, &tzS, {2004,10,-1,24,6,0,0,0}, 3},

            {0}
       };
        for (i=0; cases[i].nr; i++) {
            pSystemTimeToTzSpecificLocalTime( cases[i].ptz, &(cases[i].slt), &result);
            ok( result.wHour == cases[i].ehour,
                    "Test SystemTimeToTzSpecificLocalTime #%d. wrong system time. Hour is %d expected %d\n", 
                    cases[i].nr, result.wHour, cases[i].ehour);
        }

    }        
}

START_TEST(time)
{
    test_GetTimeZoneInformation();
    test_FileTimeToSystemTime();
    test_FileTimeToLocalFileTime();
    test_TzSpecificLocalTimeToSystemTime();
}
