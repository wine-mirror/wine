/*
 * Very basic unit test for locale functions
 * Test run on win2K (French)
 *
 * Copyright (c) 2002 YASAR Mehmet
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
#include "winerror.h"
#include "winnls.h"

#define eq(received, expected, label, type) \
        ok((received) == (expected), "%s: got " type " instead of " type, (label),(received),(expected))

#define BUFFER_SIZE		50
/* Buffer used by callback function */
char GlobalBuffer[BUFFER_SIZE];
#define COUNTOF(x) (sizeof(x)/sizeof(x)[0])

/* TODO :
 * Unicode versions
 * EnumTimeFormatsA
 * EnumDateFormatsA
 * LCMapStringA
 * GetUserDefaultLangID
 * ...
 */

void TestGetLocaleInfoA()
{
int ret, cmp;
LCID lcid;
char buffer[BUFFER_SIZE], Expected[BUFFER_SIZE];

	strcpy(Expected, "Monday");
	lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT );
	ok (lcid == 0x409, "wrong LCID calculated");

	strcpy(Expected, "xxxxx");
	memset( buffer, 'x', sizeof (buffer)/sizeof(buffer[0]) );
	ret = GetLocaleInfoA(lcid, LOCALE_SDAYNAME1, buffer, 0);
	cmp = strncmp (buffer, Expected, strlen(Expected));
	ok (cmp == 0, "GetLocaleInfoA got %s instead of %s", buffer, Expected);
	eq (ret, strlen("Monday") + 1, "GetLocaleInfoA with len=0", "%d");

	strcpy(Expected, "Monxx");
	memset( buffer, 'x', sizeof (buffer)/sizeof(buffer[0]) );
	ret = GetLocaleInfoA(lcid, LOCALE_SDAYNAME1, buffer, 3);
	cmp = strncmp (buffer, Expected, strlen(Expected));
	ok (cmp == 0, "GetLocaleInfoA got %s instead of %s", buffer, Expected);
	eq (ret, 0, "GetLocaleInfoA with len = 3", "%d");

	strcpy(Expected, "Monday");
	memset( buffer, 'x', sizeof (buffer)/sizeof(buffer[0]) );
	ret = GetLocaleInfoA(lcid, LOCALE_SDAYNAME1, buffer, 10);
	/* We check also presence of '\0' */
	cmp = strncmp (buffer, Expected, strlen(Expected) + 1);
	ok (cmp == 0, "GetLocaleInfoA got %s instead of %s", buffer, Expected);
	eq (ret, strlen(Expected)+1, "GetLocaleInfoA with len = 10", "%d" );

	/* We check the whole buffer with strncmp */
	memset( Expected, 'x', sizeof (Expected)/sizeof(Expected[0]) );
	strcpy(Expected, "Monday");
	memset( buffer, 'x', sizeof (buffer)/sizeof(buffer[0]) );
	ret = GetLocaleInfoA(lcid, LOCALE_SDAYNAME1, buffer, BUFFER_SIZE);
	cmp = strncmp (buffer, Expected, BUFFER_SIZE);
	ok (cmp == 0, "GetLocaleInfoA got %s instead of %s", buffer, Expected);
	eq (ret, strlen(Expected)+1, "GetLocaleInfoA with len = 10", "%d" );

}


void TestGetTimeFormatA()
{
int ret, error, cmp;
SYSTEMTIME  curtime;
char buffer[BUFFER_SIZE], format[BUFFER_SIZE], Expected[BUFFER_SIZE];
LCID lcid;

	lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT );
	strcpy(format, "tt HH':'mm'@'ss");

        todo_wine {
            /* fill curtime with dummy data */
            memset(&curtime, 2, sizeof(SYSTEMTIME));
            ret = GetTimeFormatA(lcid, TIME_FORCE24HOURFORMAT, &curtime, format, buffer, sizeof(buffer));
            error = GetLastError ();
            ok (ret == 0, "GetTimeFormat should fail on dummy data");
            eq (error, ERROR_INVALID_PARAMETER, "GetTimeFormat GetLastError()", "%d");
        }

	strcpy(Expected, "AM 08:56@13");
	curtime.wHour = 8;  curtime.wMinute = 56;
	curtime.wSecond = 13; curtime.wMilliseconds = 22;
	ret = GetTimeFormatA(lcid, TIME_FORCE24HOURFORMAT, &curtime, format, buffer, sizeof(buffer));
	cmp = strncmp (Expected, buffer, strlen(Expected)+1);
	ok (cmp == 0, "GetTimeFormat got %s instead of %s", buffer, Expected);
	eq (ret, strlen(Expected)+1, "GetTimeFormat", "%d");

	/* test with too small buffers */
	memset(buffer, 'x', sizeof (buffer)/sizeof(buffer[0]) );
	strcpy(Expected, "xxxx");
	ret = GetTimeFormatA(lcid, TIME_FORCE24HOURFORMAT, &curtime, format, buffer, 0);
	cmp = strncmp (Expected, buffer, 4);
	ok (cmp == 0, "GetTimeFormat with len=0 got %s instead of %s", buffer, Expected);
	todo_wine { eq (ret, 12, "GetTimeFormat with len=0", "%d"); }

	memset(buffer, 'x', sizeof (buffer)/sizeof(buffer[0]) );
	strcpy(Expected, "AMxx");
	ret = GetTimeFormatA(lcid, TIME_FORCE24HOURFORMAT, &curtime, format, buffer, 2);
	cmp = strncmp (Expected, buffer, 4);
	todo_wine { ok (cmp == 0, "GetTimeFormat with len=2 got %s instead of %s", buffer, Expected); }
	eq (ret, 0, "GetTimeFormat with len=2", "%d");
}

void TestGetDateFormatA()
{
int ret, error, cmp;
SYSTEMTIME  curtime;
char buffer[BUFFER_SIZE], format[BUFFER_SIZE], Expected[BUFFER_SIZE];
LCID lcid;

	lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT );
	strcpy(format, "ddd',' MMM dd yy");

        todo_wine {
            /* fill curtime with dummy data */
            memset(&curtime, 2, sizeof(SYSTEMTIME));
            memset(buffer, 'x', sizeof (buffer)/sizeof(buffer[0]) );
            ret = GetDateFormatA(lcid, 0, &curtime, format, buffer, sizeof(buffer));
            error = GetLastError ();
            ok (ret== 0, "GetDateFormat should fail on dummy data");
            eq (error, ERROR_INVALID_PARAMETER, "GetDateFormat", "%d");
        }

	strcpy(Expected, "Sat, May 04 02");
	memset(buffer, 'x', sizeof (buffer)/sizeof(buffer[0]) );
	curtime.wYear = 2002;
	curtime.wMonth = 5;
	curtime.wDay = 4;
	curtime.wDayOfWeek = 3;
	ret = GetDateFormatA(lcid, 0, &curtime, format, buffer, sizeof(buffer));
	cmp = strncmp (Expected, buffer, strlen(Expected)+1);
	todo_wine { ok (cmp == 0, "GetDateFormat got %s instead of %s", buffer, Expected); }
	eq (ret, strlen(Expected)+1, "GetDateFormat", "%d");

	/* test format with "'" */
	memset(buffer, 'x', sizeof (buffer)/sizeof(buffer[0]) );
	strcpy(format, "ddd',' MMM dd ''''yy");
	strcpy(Expected, "Sat, May 04 '02");
	ret = GetDateFormatA(lcid, 0, &curtime, format, buffer, sizeof(buffer));
	cmp = strncmp (Expected, buffer, strlen(Expected)+1);
	todo_wine { ok (cmp == 0, "GetDateFormat got %s instead of %s", buffer, Expected); }
	eq (ret, (strlen(Expected)+1), "GetDateFormat", "%d");

	/* test with too small buffers */
	memset(buffer, 'x', sizeof (buffer)/sizeof(buffer[0]) );
	strcpy(Expected, "xxxx");
	ret = GetDateFormatA(lcid, 0, &curtime, format, buffer, 0);
	cmp = strncmp (Expected, buffer, 4);
	ok (cmp == 0, "GetDateFormat got %s instead of %s", buffer, Expected);
	todo_wine { eq (ret, 16, "GetDateFormat with len=0", "%d"); }

	memset(buffer, 'x', sizeof (buffer)/sizeof(buffer[0]) );
	strcpy(Expected, "Saxx");
	ret = GetDateFormatA(lcid, 0, &curtime, format, buffer, 2);
	cmp = strncmp (Expected, buffer, 4);
	todo_wine { ok (cmp == 0, "GetDateFormat got %s instead of %s", buffer, Expected); }
	eq (ret, 0, "GetDateFormat with len=2", "%d");
}

void TestGetDateFormatW()
{
    int ret, error, cmp;
    SYSTEMTIME  curtime;
    WCHAR buffer[BUFFER_SIZE], format[BUFFER_SIZE], Expected[BUFFER_SIZE];
    LCID lcid;

    lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT );

    /* 1. Error cases */

    /* 1a If flags is not zero then format must be null. */
    ret = GetDateFormatW (LOCALE_SYSTEM_DEFAULT, DATE_LONGDATE, NULL, format, buffer, sizeof(buffer)/sizeof(buffer[0]));
    error = ret ? 0 : GetLastError();
    ok (ret == 0 && error == ERROR_INVALID_FLAGS, "GetDateFormatW allowed flags and format");

    /* 1b The buffer can only be null if the count is zero */
    /* For the record other bad pointers result in a page fault (Win98) */
    ret = GetDateFormatW (LOCALE_SYSTEM_DEFAULT, 0, NULL, format, NULL, sizeof(buffer)/sizeof(buffer[0]));
    error = ret ? 0 : GetLastError();
    ok (ret == 0 && error == ERROR_INVALID_PARAMETER, "GetDateFormatW did not detect null buffer pointer.");
    ret = GetDateFormatW (LOCALE_SYSTEM_DEFAULT, 0, NULL, format, NULL, 0);
    error = ret ? 0 : GetLastError();
    ok (ret != 0 && error == 0, "GetDateFormatW did not permit null buffer pointer when counting.");

    /* 1c An incorrect day of week is corrected. */
    curtime.wYear = 2002;
    curtime.wMonth = 10;
    curtime.wDay = 23;
    curtime.wDayOfWeek = 5; /* should be 3 - Wednesday */
    curtime.wHour = 0;
    curtime.wMinute = 0;
    curtime.wSecond = 0;
    curtime.wMilliseconds = 234;
    MultiByteToWideChar (CP_ACP, 0, "dddd d MMMM yyyy", -1, format, COUNTOF(format));
    ret = GetDateFormatW (lcid, 0, &curtime, format, buffer, COUNTOF(buffer));
    error = ret ? 0 : GetLastError();
    MultiByteToWideChar (CP_ACP, 0, "Wednesday 23 October 2002", -1, Expected, COUNTOF(Expected));
    cmp = ret ? lstrcmpW (buffer, Expected) : 2;
    ok (ret == lstrlenW(Expected)+1 && error == 0 && cmp == 0, "Day of week correction failed\n");

    /* 1d Invalid year, month or day results in error */

    /* 1e Insufficient space results in error */

    /* 2. Standard behaviour */
    /* 1c is a reasonable test */

    /* 3. Replicated undocumented behaviour */
    /* e.g. unexepected characters are retained. */
}


void TestGetCurrencyFormat()
{
int ret, error, cmp;
char buffer[BUFFER_SIZE], Expected[BUFFER_SIZE], format[BUFFER_SIZE];
LCID lcid;

	lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT );
#if 0
	lcid = MAKELCID(MAKELANGID(LANG_FRENCH, SUBLANG_DEFAULT), SORT_DEFAULT );
#endif

	memset( buffer, 'x', sizeof (buffer)/sizeof(buffer[0]) );
	ret = GetCurrencyFormatA(lcid, 0, "23,65", NULL, buffer, sizeof(buffer));
	error = GetLastError ();
	cmp = strncmp ("xxxx", buffer, 4);

	ok (cmp == 0, "GetCurrencyFormat should fail with ','");
	eq (ret, 0, "GetCurrencyFormat with ','", "%d");
	eq (error, ERROR_INVALID_PARAMETER, "GetCurrencyFormat", "%d");

	memset(Expected, 'x', sizeof (Expected)/sizeof(Expected[0]) );
	strcpy (Expected, "$23.53");
	strcpy (format, "23.53");
	memset( buffer, 'x', sizeof (buffer)/sizeof(buffer[0]) );
	ret = GetCurrencyFormatA(lcid, 0, format, NULL, buffer, 0);
	cmp = strncmp ("xxx", buffer, 3);
	ok (cmp == 0, "GetCurrencyFormat with 0 buffer size");
	eq (ret, strlen(Expected)+1, "GetCurrencyFormat with len=0", "%d");

	memset( buffer, 'x', sizeof (buffer)/sizeof(buffer[0]) );
	ret = GetCurrencyFormatA(lcid, 0, format, NULL, buffer, 2);
	cmp = strncmp ("$2xx", buffer, 4);
	ok (cmp == 0, "GetCurrencyFormat got %s instead of %s", buffer, Expected);
	eq (ret, 0, "GetCurrencyFormat with len=2", "%d");

	/* We check the whole buffer with strncmp */
	memset( buffer, 'x', sizeof (buffer)/sizeof(buffer[0]) );
	ret = GetCurrencyFormatA(lcid, 0, format, NULL, buffer, sizeof(buffer));
	cmp = strncmp (Expected, buffer, BUFFER_SIZE);
	ok (cmp == 0, "GetCurrencyFormat got %s instead of %s", buffer, Expected);
	eq (ret, strlen(Expected)+1, "GetCurrencyFormat","%d");

}


void TestGetNumberFormat()
{
int ret, error, cmp;
char buffer[BUFFER_SIZE], Expected[BUFFER_SIZE], input[BUFFER_SIZE];
LCID lcid;

	lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT );

	memset( buffer, 'x', sizeof (buffer)/sizeof(buffer[0]) );
	ret = GetNumberFormatA(lcid, 0, "23,65", NULL, buffer, sizeof(buffer));
	error = GetLastError ();
	cmp = strncmp ("xxx", buffer, 3);
	ok (cmp == 0, "GetCurrencyFormat");
	ok (ret == 0, "GetNumberFormat should return 0");
	eq (error, ERROR_INVALID_PARAMETER, "GetNumberFormat", "%d");

	strcpy(input, "2353");
	strcpy(Expected, "2,353.00");
	memset( buffer, 'x', sizeof (buffer)/sizeof(buffer[0]) );
	ret = GetNumberFormatA(lcid, 0, input, NULL, buffer, 0);
	cmp = strncmp ("xxx", buffer, 3);
	ok (cmp == 0, "GetNumberFormat with len=0 got %s instead of %s", buffer, "xxx");
	eq (ret, strlen(Expected)+1, "GetNumberFormat", "%d");

	strcpy(Expected, "2,xx");
	memset( buffer, 'x', sizeof (buffer)/sizeof(buffer[0]) );
	ret = GetNumberFormatA(lcid, 0, input, NULL, buffer, 2);
	cmp = strncmp (Expected, buffer, 4);
	ok (cmp == 0, "GetNumberFormat with len=2 got %s instead of %s", buffer, Expected);
	eq (ret, 0, "GetNumberFormat", "%d");

	/* We check the whole buffer with strncmp */
	memset(Expected, 'x', sizeof (Expected)/sizeof(Expected[0]) );
	strcpy(Expected, "2,353.00");
	memset( buffer, 'x', sizeof (buffer)/sizeof(buffer[0]) );
	ret = GetNumberFormatA(lcid, 0, input, NULL, buffer, sizeof(buffer));
	cmp = strncmp (Expected, buffer, BUFFER_SIZE);
	ok (cmp == 0, "GetNumberFormat got %s instead of %s", buffer, Expected);
	eq (ret, strlen(Expected)+1, "GetNumberFormat", "%d");
}


/* Callback function used by TestEnumTimeFormats */
BOOL CALLBACK EnumTimeFormatsProc(char * lpTimeFormatString)
{
	trace("%s\n", lpTimeFormatString);
	strcpy(GlobalBuffer, lpTimeFormatString);
#if 0
	return TRUE;
#endif
	return FALSE;
}

void TestEnumTimeFormats()
{
int ret;
LCID lcid;
char Expected[BUFFER_SIZE];

	lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT );
	memset( GlobalBuffer, 'x', sizeof (GlobalBuffer)/sizeof(GlobalBuffer[0]) );
	strcpy(Expected, "h:mm:ss tt");
	ret = EnumTimeFormatsA(EnumTimeFormatsProc, lcid, 0);

	eq (ret, 1, "EnumTimeFormats should return 1", "%d");
	ok (strncmp (GlobalBuffer, Expected, strlen(Expected)) == 0,
				"EnumTimeFormats failed");
	ok (ret == 1, "EnumTimeFormats should return 1");
}


void TestCompareStringA()
{
int ret;
LCID lcid;
char buffer1[BUFFER_SIZE], buffer2[BUFFER_SIZE];

	lcid = MAKELCID(MAKELANGID(LANG_FRENCH, SUBLANG_DEFAULT), SORT_DEFAULT );

	strcpy(buffer1, "Salut"); strcpy(buffer2, "Salute");
	ret = CompareStringA(lcid, NORM_IGNORECASE, buffer1, -1, buffer2, -1);
	ok (ret== 1, "CompareStringA (st1=%s str2=%s) expected result=1", buffer1, buffer2);

	strcpy(buffer1, "Salut"); strcpy(buffer2, "saLuT");
	ret = CompareStringA(lcid, NORM_IGNORECASE, buffer1, -1, buffer2, -1);
	ok (ret== 2, "CompareStringA (st1=%s str2=%s) expected result=2", buffer1, buffer2);

	strcpy(buffer1, "Salut"); strcpy(buffer2, "hola");
	ret = CompareStringA(lcid, NORM_IGNORECASE, buffer1, -1, buffer2, -1);
	ok (ret== 3, "CompareStringA (st1=%s str2=%s) expected result=3", buffer1, buffer2);

	strcpy(buffer1, "héhé"); strcpy(buffer2, "hèhè");
	ret = CompareStringA(lcid, NORM_IGNORECASE, buffer1, -1, buffer2, -1);
	ok (ret== 1, "CompareStringA (st1=%s str2=%s) expected result=1", buffer1, buffer2);

	lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT );

	strcpy(buffer1, "héhé"); strcpy(buffer2, "hèhè");
	ret = CompareStringA(lcid, NORM_IGNORECASE, buffer1, -1, buffer2, -1);
	ok (ret== 1, "CompareStringA (st1=%s str2=%s) expected result=1", buffer1, buffer2);

	ret = CompareStringA(lcid, NORM_IGNORECASE, buffer1, -1, buffer2, 0);
	ok (ret== 3, "CompareStringA (st1=%s str2=%s) expected result=3", buffer1, buffer2);
}


START_TEST(locale)
{
#if 0
	TestEnumTimeFormats();
#endif
	TestGetLocaleInfoA();
	TestGetTimeFormatA();
	TestGetDateFormatA();
	TestGetDateFormatW();
	TestGetNumberFormat();
	TestGetCurrencyFormat();
	TestCompareStringA();
}
