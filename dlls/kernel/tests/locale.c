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

#define BUFFER_SIZE		128
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

	/* HTMLKit and "Font xplorer lite" expect GetLocaleInfoA to
	* partially fill the buffer even if it is too short. See bug 637.
	*/
	strcpy(Expected, "xxxxx");
	memset( buffer, 'x', sizeof(buffer) );
	ret = GetLocaleInfoA(lcid, LOCALE_SDAYNAME1, buffer, 0);
	cmp = strncmp (buffer, Expected, strlen(Expected));
	ok (cmp == 0, "GetLocaleInfoA got %s instead of %s", buffer, Expected);
	eq (ret, lstrlenA("Monday") + 1, "GetLocaleInfoA with len=0", "%d");

	strcpy(Expected, "Monxx");
	memset( buffer, 'x', sizeof(buffer) );
	ret = GetLocaleInfoA(lcid, LOCALE_SDAYNAME1, buffer, 3);
	cmp = strncmp (buffer, Expected, strlen(Expected));
	ok (cmp == 0, "GetLocaleInfoA got %s instead of %s", buffer, Expected);
	eq (ret, 0, "GetLocaleInfoA with len = 3", "%d");

	strcpy(Expected, "Monday");
	memset( buffer, 'x', sizeof(buffer) );
	ret = GetLocaleInfoA(lcid, LOCALE_SDAYNAME1, buffer, 10);
	/* We check also presence of '\0' */
	cmp = strncmp (buffer, Expected, strlen(Expected) + 1);
	ok (cmp == 0, "GetLocaleInfoA got %s instead of %s", buffer, Expected);
	eq (ret, lstrlenA(Expected)+1, "GetLocaleInfoA with len = 10", "%d" );

	/* We check the whole buffer with strncmp */
	memset( Expected, 'x', sizeof(Expected) );
	strcpy(Expected, "Monday");
	memset( buffer, 'x', sizeof(buffer) );
	ret = GetLocaleInfoA(lcid, LOCALE_SDAYNAME1, buffer, BUFFER_SIZE);
	cmp = strncmp (buffer, Expected, BUFFER_SIZE);
	ok (cmp == 0, "GetLocaleInfoA got %s instead of %s", buffer, Expected);
	eq (ret, lstrlenA(Expected)+1, "GetLocaleInfoA with len = 10", "%d" );

}


void TestGetTimeFormatA()
{
  int ret, error, cmp;
  SYSTEMTIME  curtime;
  char buffer[BUFFER_SIZE], format[BUFFER_SIZE], Expected[BUFFER_SIZE];
  LCID lcid;

  lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT );
  strcpy(format, "tt HH':'mm'@'ss");

  /* fill curtime with dummy data */
  memset(&curtime, 2, sizeof(SYSTEMTIME));
  memset(buffer, '0', sizeof(buffer));
  ret = GetTimeFormatA(lcid, TIME_FORCE24HOURFORMAT, &curtime, format, buffer, COUNTOF(buffer));
  error = GetLastError ();
  ok (ret == 0, "GetTimeFormat should fail on dummy data");
  eq (error, ERROR_INVALID_PARAMETER, "GetTimeFormat GetLastError()", "%d");
  SetLastError(NO_ERROR);   /* clear out the last error */

  /* test that we can correctly produce the expected output, not a very */
  /* demanding test ;-) */
  strcpy(Expected, "AM 08:56@13");
  curtime.wHour = 8;  curtime.wMinute = 56;
  curtime.wSecond = 13; curtime.wMilliseconds = 22;
  memset(buffer, '0', sizeof(buffer));
  ret = GetTimeFormatA(lcid, TIME_FORCE24HOURFORMAT, &curtime, format, buffer, COUNTOF(buffer));
  cmp = strncmp (Expected, buffer, strlen(Expected)+1);
  ok (cmp == 0, "GetTimeFormat got %s instead of %s", buffer, Expected);
  eq (ret, lstrlenA(Expected)+1, "GetTimeFormat", "%d");

  /* check that the size reported by the above call is accuate */
  memset(buffer, 'x', sizeof(buffer));
  ret = GetTimeFormatA(lcid, TIME_FORCE24HOURFORMAT, &curtime, format, buffer, ret);
  ok(ret==lstrlenA(Expected)+1 && GetLastError()==0,
           "GetTimeFormat(right size): ret=%d error=%ld\n",ret,GetLastError());
  ok(buffer[0] != 'x',"GetTimeFormat(right size): buffer=[%s]\n",buffer);

  /* test failure due to insufficent buffer */
  ret = GetTimeFormatA(lcid, TIME_FORCE24HOURFORMAT, &curtime, format, buffer, 2);
  ok(ret==0 && GetLastError()==ERROR_INSUFFICIENT_BUFFER,
           "GetTimeFormat(len=2): ret=%d error=%ld", ret, GetLastError());

  /* test with too small buffers */
  SetLastError(0);
  memset(buffer, '0', sizeof(buffer));
  ret = GetTimeFormatA(lcid, TIME_FORCE24HOURFORMAT, &curtime, format, NULL, 0);
  ok(ret==lstrlenA(Expected)+1 && GetLastError()==0,
           "GetTimeFormat(len=0): ret=%d error=%ld\n",ret,GetLastError());

  /************************************/
  /* test out TIME_NOMINUTESORSECONDS */
  strcpy(Expected, "8 AM");
  memset(buffer, '0', sizeof(buffer));
  ret = GetTimeFormatA(lcid, TIME_NOMINUTESORSECONDS, &curtime, NULL, buffer, sizeof(buffer));
  cmp = strncmp(buffer, Expected, BUFFER_SIZE);
  ok (cmp == 0, "GetTimeFormat got '%s' instead of '%s'", buffer, Expected);
  eq (ret, lstrlenA(Expected)+1, "GetTimeFormat", "%d");

  /* test out TIME_NOMINUTESORSECONDS with complex format strings */
  strcpy(Expected, "4");
  memset(buffer, '0', sizeof(buffer));
  ret = GetTimeFormatA(lcid, TIME_NOMINUTESORSECONDS, &curtime, "m1s2m3s4", buffer, sizeof(buffer));
  cmp = strncmp(buffer, Expected, BUFFER_SIZE);
  ok (cmp == 0, "GetTimeFormat got '%s' instead of '%s'", buffer, Expected);
  eq (ret, lstrlenA(Expected)+1, "GetTimeFormat", "%d");


  /************************************/
  /* test out TIME_NOSECONDS */
  strcpy(Expected, "8:56 AM");
  ret = GetTimeFormatA(lcid, TIME_NOSECONDS, &curtime, NULL, buffer, sizeof(buffer));
  cmp = strncmp(buffer, Expected, BUFFER_SIZE);
  ok (cmp == 0, "GetTimeFormat got '%s' instead of '%s'", buffer, Expected);
  eq (ret, lstrlenA(Expected)+1, "GetTimeFormat", "%d");

  /* test out TIME_NOSECONDS with a format string of "h:m:s tt" */
  strcpy(Expected, "8:56 AM");
  memset(buffer, '0', sizeof(buffer));
  ret = GetTimeFormatA(lcid, TIME_NOSECONDS, &curtime, "h:m:s tt", buffer, sizeof(buffer));
  cmp = strncmp(buffer, Expected, BUFFER_SIZE);
  ok (cmp == 0, "GetTimeFormat got '%s' instead of '%s'", buffer, Expected);
  eq (ret, lstrlenA(Expected)+1, "GetTimeFormat", "%d");

  /* test out TIME_NOSECONDS a strange format string of multiple delimiters "h@:m@:s tt" */
  /* expected behavior is to turn "hD1D2...mD3D4...sD5D6...tt" and turn this into */
  /* "hD1D2...mD3D4...tt" */
  strcpy(Expected, "8.@:56.@:AM");
  memset(buffer, '0', sizeof(buffer));
  ret = GetTimeFormatA(lcid, TIME_NOSECONDS, &curtime, "h.@:m.@:s.@:tt", buffer, sizeof(buffer));
  cmp = strncmp(buffer, Expected, BUFFER_SIZE);
  ok (cmp == 0, "GetTimeFormat got '%s' instead of '%s'", buffer, Expected);
  eq (ret, lstrlenA(Expected)+1, "GetTimeFormat", "%d");

  /* test out TIME_NOSECONDS with an string of "1s2s3s4" */
  /* expect to see only "3" */
  strcpy(Expected, "3");
  memset(buffer, '0', sizeof(buffer));
  ret = GetTimeFormatA(lcid, TIME_NOSECONDS, &curtime, "s1s2s3", buffer, sizeof(buffer));
  cmp = strncmp(buffer, Expected, BUFFER_SIZE);
  ok (cmp == 0, "GetTimeFormat got '%s' instead of '%s'", buffer, Expected);
  eq (ret, lstrlenA(Expected)+1, "GetTimeFormat", "%d");

  /************************/
  /* Test out time marker */
  /* test out time marker(AM/PM) behavior */
  strcpy(Expected, "A/AM");
  memset(buffer, '0', sizeof(buffer));
  ret = GetTimeFormatA(lcid, 0, &curtime, "t/tt", buffer, sizeof(buffer));
  cmp = strncmp(buffer, Expected, BUFFER_SIZE);
  ok (cmp == 0, "GetTimeFormat got '%s' instead of %s", buffer, Expected);
  eq (ret, lstrlenA(Expected)+1, "GetTimeFormat", "%d");

  /* time marker testing part 2 */
  curtime.wHour = 13;
  strcpy(Expected, "P/PM");
  memset(buffer, '0', sizeof(buffer));
  ret = GetTimeFormatA(lcid, 0, &curtime, "t/tt", buffer, sizeof(buffer));
  cmp = strncmp(buffer, Expected, BUFFER_SIZE);
  ok (cmp == 0, "GetTimeFormat got '%s' instead of %s", buffer, Expected);
  eq (ret, lstrlenA(Expected)+1, "GetTimeFormat", "%d");

  /******************************/
  /* test out TIME_NOTIMEMARKER */
  /* NOTE: TIME_NOTIMEMARKER elminates all text around any time marker */
  /*      formatting character until the previous or next formatting character */
  strcpy(Expected, "156");
  memset(buffer, '0', sizeof(buffer));
  ret = GetTimeFormatA(lcid, TIME_NOTIMEMARKER, &curtime, "h1t2tt3m", buffer, sizeof(buffer));
  cmp = strncmp(buffer, Expected, BUFFER_SIZE);
  ok (cmp == 0, "GetTimeFormat got '%s' instead of %s", buffer, Expected);
  eq (ret, lstrlenA(Expected)+1, "GetTimeFormat", "%d");

  /***********************************/
  /* test out TIME_FORCE24HOURFORMAT */
  strcpy(Expected, "13:56:13 PM");
  memset(buffer, '0', sizeof(buffer));
  ret = GetTimeFormatA(lcid, TIME_FORCE24HOURFORMAT, &curtime, "h:m:s tt", buffer, sizeof(buffer));
  cmp = strncmp(buffer, Expected, BUFFER_SIZE);
  ok (cmp == 0, "GetTimeFormat got '%s' instead of %s", buffer, Expected);
  eq (ret, lstrlenA(Expected)+1, "GetTimeFormat", "%d");

  /* check to confirm that unlike what msdn documentation suggests, the time marker */
  /* is not added under TIME_FORCE24HOURFORMAT */
  strcpy(Expected, "13:56:13");
  memset(buffer, '0', sizeof(buffer));
  ret = GetTimeFormatA(lcid, TIME_FORCE24HOURFORMAT, &curtime, "h:m:s", buffer, sizeof(buffer));
  cmp = strncmp(buffer, Expected, BUFFER_SIZE);
  ok (cmp == 0, "GetTimeFormat got '%s' instead of %s", buffer, Expected);
  eq (ret, lstrlenA(Expected)+1, "GetTimeFormat", "%d");


  /*********************************************/
  /* test advanced formatting of GetTimeFormat */

  /* test for 24 hour conversion and for leading zero */
  /* NOTE: we do not test the "hh or HH" case since hours is two digits */
  /* "h hh H HH m mm s ss t tt" */
  curtime.wHour = 14; /* change this to 14 or 2pm */
  curtime.wMinute = 5;
  curtime.wSecond = 3;
  strcpy(Expected, "2 02 14 14 5 05 3 03 P PM");
  memset(buffer, '0', sizeof(buffer));
  ret = GetTimeFormatA(lcid, 0, &curtime, "h hh H HH m mm s ss t tt", 
buffer, sizeof(buffer));
  cmp = strncmp(buffer, Expected, BUFFER_SIZE);
  ok (cmp == 0, "GetTimeFormat got '%s' instead of '%s'", buffer, Expected);
  eq (ret, lstrlenA(Expected)+1, "GetTimeFormat", "%d");

  /* complete testing on the advanced formatting by testing "hh" and "HH" */
  /* 0 hour is 12 o'clock or 00 hundred hours */
  curtime.wHour = 0;
  strcpy(Expected, "12/0/12/00");
  memset(buffer, '0', sizeof(buffer));
  ret = GetTimeFormatA(lcid, 0, &curtime, "h/H/hh/HH", buffer, sizeof(buffer));
  cmp = strncmp(buffer, Expected, BUFFER_SIZE);
  ok (cmp == 0, "GetTimeFormat got '%s' instead of '%s'", buffer, Expected);
  eq (ret, lstrlenA(Expected)+1, "GetTimeFormat", "%d");

  /* test for LOCALE_NOUSEROVERRIDE set, lpFormat must be NULL */
  strcpy(Expected, "0:5:3 AM");
  memset(buffer, '0', sizeof(buffer));
  ret = GetTimeFormatA(lcid, LOCALE_NOUSEROVERRIDE, &curtime, "h:m:s tt", buffer, sizeof(buffer));
  /* NOTE: we expect this to FAIL */
  cmp = strncmp(buffer, Expected, BUFFER_SIZE);
  ok (ret == 0, "GetTimeFormat succeeded instead of failing for LOCALE_NOUSEROVERRIDE and a non-null lpFormat\n");

  /* try to convert formatting strings with more than two letters */
  /* "h:hh:hhh:H:HH:HHH:m:mm:mmm:M:MM:MMM:s:ss:sss:S:SS:SSS" */
  /* NOTE: we expect any letter for which there is an upper case value */
  /*    we should expect to see a replacement.  For letters that DO NOT */
  /*    have upper case values we expect to see NO REPLACEMENT */
  curtime.wHour = 8;  curtime.wMinute = 56;
  curtime.wSecond = 13; curtime.wMilliseconds = 22;
  strcpy(Expected, "8:08:08 8:08:08 56:56:56 M:MM:MMM 13:13:13 S:SS:SSS");
  memset(buffer, '0', sizeof(buffer));
  ret = GetTimeFormatA(lcid, 0, &curtime, "h:hh:hhh H:HH:HHH m:mm:mmm M:MM:MMM s:ss:sss S:SS:SSS", buffer, sizeof(buffer));
  cmp = strncmp(buffer, Expected, BUFFER_SIZE);
  ok (cmp == 0, "GetTimeFormat got '%s' instead of '%s'", buffer, Expected);
  eq (ret, lstrlenA(Expected)+1, "GetTimeFormat", "%d");

  /* test that if the size of the buffer is zero that the buffer is not modified */
  /* and that the number of necessary characters is returned */
  /* NOTE: The count includes the terminating null. */
  strcpy(buffer, "THIS SHOULD NOT BE MODIFIED");
  strcpy(Expected, "THIS SHOULD NOT BE MODIFIED");
  ret = GetTimeFormatA(lcid, 0, &curtime, "h", buffer, 0);
  cmp = strncmp(buffer, Expected, BUFFER_SIZE);
  ok (cmp == 0, "GetTimeFormat got '%s' instead of '%s'", buffer, Expected);
  eq (ret, 2, "GetTimeFormat", "%d"); /* we expect to require two characters of space from "h" */

  /* test that characters in single quotation marks are ignored and left in */
  /* the same location in the output string */
  strcpy(Expected, "8 h 8 H 08 HH 56 m 13 s A t AM tt");
  memset(buffer, '0', sizeof(buffer));
  ret = GetTimeFormatA(lcid, 0, &curtime, "h 'h' H 'H' HH 'HH' m 'm' s 's' t 't' tt 'tt'", buffer, sizeof(buffer));
  cmp = strncmp(buffer, Expected, BUFFER_SIZE);
  ok (cmp == 0, "GetTimeFormat got '%s' instead of '%s'", buffer, Expected);
  eq (ret, lstrlenA(Expected)+1, "GetTimeFormat", "%d");

  /* test the printing of the single quotation marks when */
  /* we use an invalid formatting string of "'''" instead of "''''" */
  strcpy(Expected, "'");
  memset(buffer, '0', sizeof(buffer));
  ret = GetTimeFormatA(lcid, 0, &curtime, "'''", buffer, sizeof(buffer));
  cmp = strncmp(buffer, Expected, BUFFER_SIZE);
  ok (cmp == 0, "GetTimeFormat got '%s' instead of '%s'", buffer, Expected);
  eq (ret, lstrlenA(Expected)+1, "GetTimeFormat", "%d");

  /* test that msdn suggested single quotation usage works as expected */
  strcpy(Expected, "'");
  memset(buffer, '0', sizeof(buffer));
  ret = GetTimeFormatA(lcid, 0, &curtime, "''''", buffer, sizeof(buffer));
  cmp = strncmp(buffer, Expected, BUFFER_SIZE);
  ok (cmp == 0, "GetTimeFormat got '%s' instead of '%s'", buffer, Expected);
  eq (ret, lstrlenA(Expected)+1, "GetTimeFormat", "%d");

  /* test for more normal use of single quotation mark */
  strcpy(Expected, "08");
  memset(buffer, '0', sizeof(buffer));
  ret = GetTimeFormatA(lcid, 0, &curtime, "''HHHHHH", buffer, sizeof(buffer));
  cmp = strncmp(buffer, Expected, BUFFER_SIZE);
  ok (cmp == 0, "GetTimeFormat got '%s' instead of '%s'", buffer, Expected);
  eq (ret, lstrlenA(Expected)+1, "GetTimeFormat", "%d");

  /* and test for normal use of the single quotation mark */
  strcpy(Expected, "'HHHHHH");
  memset(buffer, '0', sizeof(buffer));
  ret = GetTimeFormatA(lcid, 0, &curtime, "'''HHHHHH'", buffer, sizeof(buffer));
  cmp = strncmp(buffer, Expected, BUFFER_SIZE);
  ok (cmp == 0, "GetTimeFormat got '%s' instead of '%s'", buffer, Expected);
  eq (ret, lstrlenA(Expected)+1, "GetTimeFormat", "%d");

  /* test for more odd use of the single quotation mark */
  strcpy(Expected, "'HHHHHH");
  memset(buffer, '0', sizeof(buffer));
  ret = GetTimeFormatA(lcid, 0, &curtime, "'''HHHHHH", buffer, sizeof(buffer));
  cmp = strncmp(buffer, Expected, BUFFER_SIZE);
  ok (cmp == 0, "GetTimeFormat got '%s' instead of '%s'", buffer, Expected);
  eq (ret, lstrlenA(Expected)+1, "GetTimeFormat", "%d");

  /* test that with TIME_NOTIMEMARKER that even if something is defined */
  /* as a literal we drop it before and after the markers until the next */
  /* formatting character */
  strcpy(Expected, "");
  memset(buffer, '0', sizeof(buffer));
  ret = GetTimeFormatA(lcid, TIME_NOTIMEMARKER, &curtime, "'123'tt", buffer, sizeof(buffer));
  cmp = strncmp(buffer, Expected, BUFFER_SIZE);
  ok (cmp == 0, "GetTimeFormat got '%s' instead of '%s'", buffer, Expected);
  eq (ret, lstrlenA(Expected)+1, "GetTimeFormat", "%d");

  /* test for expected return and error value when we have a */
  /* non-null format and LOCALE_NOUSEROVERRIDE for flags */
  SetLastError(NO_ERROR); /* reset last error value */
  memset(buffer, '0', sizeof(buffer));
  ret = GetTimeFormatA(lcid, LOCALE_NOUSEROVERRIDE, &curtime, "'123'tt", buffer, sizeof(buffer));
  error = GetLastError();
  cmp = strncmp(buffer, Expected, BUFFER_SIZE);
  ok ((ret == 0) && (error == ERROR_INVALID_FLAGS), "GetTimeFormat got ret of '%d' and error of '%d'", ret, error);

  /* test that invalid time values result in ERROR_INVALID_PARAMETER */
  /* and a return value of 0 */
  curtime.wHour = 25;
  SetLastError(NO_ERROR); /* reset last error value */
  memset(buffer, '0', sizeof(buffer));
  ret = GetTimeFormatA(lcid, 0, &curtime, "'123'tt", buffer, sizeof(buffer));
  error = GetLastError();
  cmp = strncmp(buffer, Expected, BUFFER_SIZE);
  ok ((ret == 0) && (error == ERROR_INVALID_PARAMETER), "GetTimeFormat got ret of '%d' and error of '%d'", ret, error);

  /* test that invalid information in the date section of the current time */
  /* doesn't result in an error since GetTimeFormat() should ignore this information */
  curtime.wHour = 12; /* valid wHour */
  curtime.wMonth = 60; /* very invalid wMonth */
  strcpy(Expected, "12:56:13");
  SetLastError(NO_ERROR); /* reset last error value */
  ret = GetTimeFormatA(lcid, 0, &curtime, "h:m:s", buffer, sizeof(buffer));
  error = GetLastError();
  cmp = strncmp(buffer, Expected, BUFFER_SIZE);
  ok ((ret == lstrlenA(Expected)+1) && (error == NO_ERROR), "GetTimeFormat got ret of '%d' and error of '%d' and a buffer of '%s'", ret, error, buffer);
}

void TestGetDateFormatA()
{
  int ret, error, cmp;
  SYSTEMTIME  curtime;
  char buffer[BUFFER_SIZE], format[BUFFER_SIZE], Expected[BUFFER_SIZE];
  LCID lcid;

  lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT );
  strcpy(format, "ddd',' MMM dd yy");

  /* test for failure on dummy data */
  memset(&curtime, 2, sizeof(SYSTEMTIME));
  memset(buffer, '0', sizeof(buffer));
  SetLastError(NO_ERROR);
  ret = GetDateFormatA(lcid, 0, &curtime, format, buffer, COUNTOF(buffer));
  error = GetLastError ();
  ok(ret == 0, "GetDateFormat should fail on dummy data");
  eq(error, ERROR_INVALID_PARAMETER, "GetDateFormat", "%d");

  /* test for a simple case of date conversion */
  strcpy(Expected, "Sat, May 04 02");
  curtime.wYear = 2002;
  curtime.wMonth = 5;
  curtime.wDay = 4;
  curtime.wDayOfWeek = 3;
  memset(buffer, 0, sizeof(buffer));
  ret = GetDateFormatA(lcid, 0, &curtime, format, buffer, COUNTOF(buffer));
  cmp = strncmp (Expected, buffer, strlen(Expected)+1);
  ok (cmp == 0, "GetDateFormat got %s instead of %s", buffer, Expected);
  eq (ret, lstrlenA(Expected)+1, "GetDateFormat", "%d");

  /* test format with "'" */
  strcpy(format, "ddd',' MMM dd ''''yy");
  strcpy(Expected, "Sat, May 04 '02");
  memset(buffer, 0, sizeof(buffer));
  ret = GetDateFormatA(lcid, 0, &curtime, format, buffer, COUNTOF(buffer));
  cmp = strncmp (Expected, buffer, strlen(Expected)+1);
  ok (cmp == 0, "GetDateFormat got %s instead of %s", buffer, Expected);
  eq (ret, lstrlenA(Expected)+1, "GetDateFormat", "%d");

  /* test for success with dummy time data */
  curtime.wHour = 36;
  memset(buffer, 0, sizeof(buffer));
  ret = GetDateFormatA(lcid, 0, &curtime, format, buffer, COUNTOF(buffer));
  cmp = strncmp (Expected, buffer, strlen(Expected)+1);
  ok (cmp == 0, "GetDateFormat got %s instead of %s", buffer, Expected);
  eq (ret, lstrlenA(Expected)+1, "GetDateFormat", "%d");

  /* test that we retrieve the expected size for the necessary output of this string */
  SetLastError(NO_ERROR);
  memset(buffer, 0, sizeof(buffer));
  ret = GetDateFormatA(lcid, 0, &curtime, format, NULL, 0);
  ok(ret==lstrlenA(Expected)+1 && GetLastError() == 0,
          "GetDateFormat(len=0): ret=%d error=%ld buffer='%s', expected NO_ERROR(0)\n",ret,GetLastError(), buffer);

  /* test that the expected size matches the actual required size by passing */
  /* in the expected size */
  memset(buffer, '0', sizeof(buffer));
  ret = GetDateFormatA(lcid, 0, &curtime, format, buffer, ret);
  ok(ret==lstrlenA(Expected)+1 && GetLastError()==0,
           "GetDateFormat(right size): ret=%d error=%ld, buffer = '%s'\n",ret,GetLastError(), buffer);
  ok(buffer[0]!='x',"GetDateFormat(right size): buffer=[%s]\n",buffer);

  /* test that a buffer shorter than the necessary size results in ERROR_INSUFFICIENT_BUFFER */
  ret = GetDateFormatA(lcid, 0, &curtime, format, buffer, 2);
  ok(ret==0 && GetLastError()==ERROR_INSUFFICIENT_BUFFER,
           "GetDateFormat(len=2): ret=%d error=%ld", ret, GetLastError());

  /* test for default behavior being DATE_SHORTDATE */
  todo_wine {
    strcpy(Expected, "5/4/02");
    memset(buffer, '0', sizeof(buffer));
    ret = GetDateFormat(lcid, 0, &curtime, NULL, buffer, sizeof(buffer));
    cmp = strncmp (Expected, buffer, strlen(Expected)+1);
    ok (cmp == 0, "GetDateFormat got '%s' instead of '%s'", buffer, Expected);
    eq (ret, lstrlenA(Expected)+1, "GetDateFormat", "%d");
  }


  /* test for expected DATE_LONGDATE behavior with null format */
  strcpy(Expected, "Saturday, May 04, 2002");
  memset(buffer, '0', sizeof(buffer));
  ret = GetDateFormat(lcid, DATE_LONGDATE, &curtime, NULL, buffer, sizeof(buffer));
  cmp = strncmp (Expected, buffer, strlen(Expected)+1);
  ok (cmp == 0, "GetDateFormat got '%s' instead of '%s'", buffer, Expected);
  eq (ret, lstrlenA(Expected)+1, "GetDateFormat", "%d");

  /* test for expected DATE_YEARMONTH behavior with null format */
  /* NT4 returns ERROR_INVALID_FLAGS for DATE_YEARMONTH */
  todo_wine {
    strcpy(Expected, "");
    buffer[0] = 0;
    SetLastError(NO_ERROR);
    memset(buffer, '0', sizeof(buffer));
    ret = GetDateFormat(lcid, DATE_YEARMONTH, &curtime, NULL, buffer, sizeof(buffer));
    error = GetLastError();
    cmp = strncmp (Expected, buffer, strlen(Expected)+1);
    ok (ret == 0 && (error == ERROR_INVALID_FLAGS), "GetDateFormat check DATE_YEARMONTH with null format expected ERROR_INVALID_FLAGS got return of '%d' and error of '%d'", ret, error);
  }

  /* Test that using invalid DATE_* flags results in the correct error */
  /* and return values */
  strcpy(format, "m/d/y");
  strcpy(Expected, "Saturday May 2002");
  SetLastError(NO_ERROR);
  memset(buffer, '0', sizeof(buffer));
  ret = GetDateFormat(lcid, DATE_YEARMONTH | DATE_SHORTDATE | DATE_LONGDATE, &curtime, format, buffer, sizeof(buffer));
  error = GetLastError();
  cmp = strncmp (Expected, buffer, strlen(Expected)+1);
  ok ((ret == 0) && (error == ERROR_INVALID_FLAGS), "GetDateFormat checking for mutually exclusive flags got '%s' instead of '%s', got error of %d, expected ERROR_INVALID_FLAGS", buffer, Expected, error);
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
    ret = GetDateFormatW (LOCALE_SYSTEM_DEFAULT, DATE_LONGDATE, NULL, format, buffer, COUNTOF(buffer));
    if (ret==0 && GetLastError()==ERROR_CALL_NOT_IMPLEMENTED)
        return;
    error = ret ? 0 : GetLastError();
    ok (ret == 0 && error == ERROR_INVALID_FLAGS, "GetDateFormatW allowed flags and format");

    /* 1b The buffer can only be null if the count is zero */
    /* For the record other bad pointers result in a page fault (Win98) */
    ret = GetDateFormatW (lcid, 0, NULL, format, NULL, COUNTOF(buffer));
    error = ret ? 0 : GetLastError();
    ok (ret == 0 && error == ERROR_INVALID_PARAMETER, "GetDateFormatW did not detect null buffer pointer.");
    ret = GetDateFormatW (lcid, 0, NULL, format, NULL, 0);
    error = ret ? 0 : GetLastError();
    ok (ret != 0 && error == 0, "GetDateFormatW did not permit null buffer pointer when counting.");

    /* 1c An incorrect day of week is corrected. */
    /* 1d The incorrect day of week can even be out of range. */
    /* 1e The time doesn't matter */
    curtime.wYear = 2002;
    curtime.wMonth = 10;
    curtime.wDay = 23;
    curtime.wDayOfWeek = 45612; /* should be 3 - Wednesday */
    curtime.wHour = 65432;
    curtime.wMinute = 34512;
    curtime.wSecond = 65535;
    curtime.wMilliseconds = 12345;
    MultiByteToWideChar (CP_ACP, 0, "dddd d MMMM yyyy", -1, format, COUNTOF(format));
    ret = GetDateFormatW (lcid, 0, &curtime, format, buffer, COUNTOF(buffer));
    error = ret ? 0 : GetLastError();
    MultiByteToWideChar (CP_ACP, 0, "Wednesday 23 October 2002", -1, Expected, COUNTOF(Expected));
    cmp = ret ? lstrcmpW (buffer, Expected) : 2;
    ok (ret == lstrlenW(Expected)+1 && error == 0 && cmp == 0, "Day of week correction failed\n");
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

	memset( buffer, 'x', sizeof(buffer) );
	ret = GetCurrencyFormatA(lcid, 0, "23,65", NULL, buffer, COUNTOF(buffer));
	error = GetLastError ();
	cmp = strncmp ("xxxx", buffer, 4);

	ok (cmp == 0, "GetCurrencyFormat should fail with ','");
	eq (ret, 0, "GetCurrencyFormat with ','", "%d");
	eq (error, ERROR_INVALID_PARAMETER, "GetCurrencyFormat", "%d");

	/* We check the whole buffer with strncmp */
	strcpy (Expected, "$23.53");
	strcpy (format, "23.53");
	memset( buffer, 'x', sizeof(buffer) );
	ret = GetCurrencyFormatA(lcid, 0, format, NULL, buffer, COUNTOF(buffer));
	cmp = strncmp (Expected, buffer, BUFFER_SIZE);
	ok (cmp == 0, "GetCurrencyFormatA got %s instead of %s", buffer, Expected);
	eq (ret, lstrlenA(Expected)+1, "GetCurrencyFormatA","%d");

        /* Test too small buffers */
        SetLastError(0);
	ret = GetCurrencyFormatA(lcid, 0, format, NULL, NULL, 0);
	ok(ret==lstrlenA(Expected)+1 && GetLastError()==0,
       "GetCurrencyFormatA(size=0): ret=%d error=%ld", ret, GetLastError());

	memset( buffer, 'x', sizeof(buffer) );
	ret = GetCurrencyFormatA(lcid, 0, format, NULL, buffer, ret);
	ok(strcmp(buffer,Expected)==0,
           "GetCurrencyFormatA(right size): got [%s] instead of [%s]", buffer, Expected);
	eq (ret, lstrlenA(Expected)+1, "GetCurrencyFormatA(right size)", "%d");

	ret = GetCurrencyFormatA(lcid, 0, format, NULL, buffer, 2);
	ok(ret==0 && GetLastError()==ERROR_INSUFFICIENT_BUFFER,
           "GetCurrencyFormatA(size=2): ret=%d error=%ld", ret, GetLastError());
}


void TestGetNumberFormat()
{
int ret, error, cmp;
char buffer[BUFFER_SIZE], Expected[BUFFER_SIZE], input[BUFFER_SIZE];
LCID lcid;
NUMBERFMTA format;

	lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT );

	memset( buffer, 'x', sizeof(buffer) );
	ret = GetNumberFormatA(lcid, 0, "23,65", NULL, buffer, COUNTOF(buffer));
	error = GetLastError ();
	cmp = strncmp ("xxx", buffer, 3);
	ok (cmp == 0, "GetNumberFormat");
	ok (ret == 0, "GetNumberFormat should return 0");
	eq (error, ERROR_INVALID_PARAMETER, "GetNumberFormat", "%d");

	strcpy(input, "2353");
	strcpy(Expected, "2,353.00");
        SetLastError(0);
	ret = GetNumberFormatA(lcid, 0, input, NULL, NULL, 0);
	ok(ret==lstrlenA(Expected)+1 && GetLastError()==0,
       "GetNumberFormatA(size=0): ret=%d error=%ld", ret, GetLastError());

        memset( buffer, 'x', sizeof(buffer) );
	ret = GetNumberFormatA(lcid, 0, input, NULL, buffer, ret);
	ok(strcmp(buffer,Expected)==0,
           "GetNumberFormatA(right size): got [%s] instead of [%s]", buffer, Expected);
	eq(ret, lstrlenA(Expected)+1, "GetNumberFormat", "%d");

	ret = GetNumberFormatA(lcid, 0, input, NULL, buffer, 2);
	ok(ret==0 && GetLastError()==ERROR_INSUFFICIENT_BUFFER,
           "GetNumberFormatA(size=2): ret=%d error=%ld", ret, GetLastError());

	/* We check the whole buffer with strncmp */
	memset(Expected, 'x', sizeof(Expected) );
	strcpy(Expected, "2,353.00");
	memset( buffer, 'x', sizeof(buffer) );
	ret = GetNumberFormatA(lcid, 0, input, NULL, buffer, COUNTOF(buffer));
	cmp = strncmp (Expected, buffer, BUFFER_SIZE);
	ok (cmp == 0, "GetNumberFormat got %s instead of %s", buffer, Expected);
	eq (ret, lstrlenA(Expected)+1, "GetNumberFormat", "%d");

        /* If the number of decimals is zero there should be no decimal
         * separator.
         * If the grouping size is zero there should be no grouping symbol
         */
        format.NumDigits = 0;
        format.LeadingZero = 0;
        format.Grouping = 0;
        format.NegativeOrder = 0;
        format.lpDecimalSep = ".";
        format.lpThousandSep = ",";
        strcpy (Expected, "123456789");
	memset( buffer, 'x', sizeof(buffer) );
        ret = GetNumberFormatA (0, 0, "123456789.0", &format, buffer, COUNTOF(buffer));
        cmp = strncmp (Expected, buffer, sizeof(buffer));
	ok (cmp == 0, "GetNumberFormat got %s instead of %s", buffer, Expected);

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
	memset( GlobalBuffer, 'x', sizeof(GlobalBuffer) );
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
