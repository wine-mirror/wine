/*
 * Unit tests for locale functions
 *
 * Copyright 2002 YASAR Mehmet
 * Copyright 2003 Dmitry Timoshkov
 * Copyright 2003 Jon Griffiths
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
 *
 * NOTES
 *  We must pass LOCALE_NOUSEROVERRIDE (NUO) to formatting functions so that
 *  even when the user has overridden their default i8n settings (e.g. in
 *  the control panel i8n page), we will still get the expected results.
 */
#include <stdarg.h>

#include "wine/test.h"
#include "windef.h"
#include "wine/unicode.h"
#include "winbase.h"
#include "winerror.h"
#include "winnls.h"

/* Some functions are only in later versions of kernel32.dll */
static HMODULE hKernel32;

typedef BOOL (WINAPI *EnumSystemLanguageGroupsAFn)(LANGUAGEGROUP_ENUMPROC,
                                                   DWORD, LONG_PTR);
static EnumSystemLanguageGroupsAFn pEnumSystemLanguageGroupsA;
typedef BOOL (WINAPI *EnumLanguageGroupLocalesAFn)(LANGGROUPLOCALE_ENUMPROC,
                                                   LGRPID, DWORD, LONG_PTR);
static EnumLanguageGroupLocalesAFn pEnumLanguageGroupLocalesA;


static void InitFunctionPointers(void)
{
  hKernel32 = GetModuleHandleA("kernel32");

  if (hKernel32)
  {
    pEnumSystemLanguageGroupsA = (void*)GetProcAddress(hKernel32, "EnumSystemLanguageGroupsA");
	pEnumLanguageGroupLocalesA = (void*)GetProcAddress(hKernel32, "EnumLanguageGroupLocalesA");
  }
}

#define eq(received, expected, label, type) \
        ok((received) == (expected), "%s: got " type " instead of " type "\n", \
           (label), (received), (expected))

#define BUFFER_SIZE    128
char GlobalBuffer[BUFFER_SIZE]; /* Buffer used by callback function */
#define COUNTOF(x) (sizeof(x)/sizeof(x)[0])

#define EXPECT_LEN(len) ok(ret == len, "Expected Len %d, got %d\n", len, ret)
#define EXPECT_INVALID  ok(GetLastError() == ERROR_INVALID_PARAMETER, \
 "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError())
#define EXPECT_BUFFER  ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, \
 "Expected ERROR_INSUFFICIENT_BUFFER, got %ld\n", GetLastError())
#define EXPECT_FLAGS  ok(GetLastError() == ERROR_INVALID_FLAGS, \
 "Expected ERROR_INVALID_FLAGS, got %ld\n", GetLastError())
#define EXPECT_INVALIDFLAGS ok(GetLastError() == ERROR_INVALID_FLAGS || \
  GetLastError() == ERROR_INVALID_PARAMETER, \
 "Expected ERROR_INVALID_FLAGS, got %ld\n", GetLastError())
#define EXPECT_VALID    ok(GetLastError() == 0, \
 "Expected GetLastError() == 0, got %ld\n", GetLastError())

#define STRINGSA(x,y) strcpy(input, x); strcpy(Expected, y); SetLastError(0); buffer[0] = '\0'
#define EXPECT_LENA EXPECT_LEN(strlen(Expected)+1)
#define EXPECT_EQA ok(strncmp(buffer, Expected, strlen(Expected)) == 0, \
  "Expected '%s', got '%s'", Expected, buffer)

#define STRINGSW(x,y) MultiByteToWideChar(CP_ACP,0,x,-1,input,COUNTOF(input)); \
   MultiByteToWideChar(CP_ACP,0,y,-1,Expected,COUNTOF(Expected)); \
   SetLastError(0); buffer[0] = '\0'
#define EXPECT_LENW EXPECT_LEN(strlenW(Expected)+1)
#define EXPECT_EQW  ok(strncmpW(buffer, Expected, strlenW(Expected)) == 0, "Bad conversion\n")

#define NUO LOCALE_NOUSEROVERRIDE

static void test_GetLocaleInfoA()
{
  int ret;
  LCID lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);
  char buffer[BUFFER_SIZE];

  ok(lcid == 0x409, "wrong LCID calculated - %ld\n", lcid);

  /* HTMLKit and "Font xplorer lite" expect GetLocaleInfoA to
   * partially fill the buffer even if it is too short. See bug 637.
   */
  SetLastError(0); memset(buffer, 0, COUNTOF(buffer));
  ret = GetLocaleInfoA(lcid, NUO|LOCALE_SDAYNAME1, buffer, 0);
  ok(ret == 7 && !buffer[0], "Expected len=7, got %d\n", ret);

  SetLastError(0); memset(buffer, 0, COUNTOF(buffer));
  ret = GetLocaleInfoA(lcid, NUO|LOCALE_SDAYNAME1, buffer, 3);
  EXPECT_BUFFER; EXPECT_LEN(0);
  ok(!strcmp(buffer, "Mon"), "Expected 'Mon', got '%s'\n", buffer);

  SetLastError(0); memset(buffer, 0, COUNTOF(buffer));
  ret = GetLocaleInfoA(lcid, NUO|LOCALE_SDAYNAME1, buffer, 10);
  EXPECT_VALID; EXPECT_LEN(7);
  ok(!strcmp(buffer, "Monday"), "Expected 'Monday', got '%s'\n", buffer);
}

static void test_GetTimeFormatA()
{
  int ret;
  SYSTEMTIME  curtime;
  LCID lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);
  char buffer[BUFFER_SIZE], input[BUFFER_SIZE], Expected[BUFFER_SIZE];

  memset(&curtime, 2, sizeof(SYSTEMTIME));
  STRINGSA("tt HH':'mm'@'ss", ""); /* Invalid time */
  ret = GetTimeFormatA(lcid, TIME_FORCE24HOURFORMAT, &curtime, input, buffer, COUNTOF(buffer));
  EXPECT_INVALID; EXPECT_LEN(0); EXPECT_EQA;

  curtime.wHour = 8;
  curtime.wMinute = 56;
  curtime.wSecond = 13;
  curtime.wMilliseconds = 22;
  STRINGSA("tt HH':'mm'@'ss", "AM 08:56@13"); /* Valid time */
  ret = GetTimeFormatA(lcid, TIME_FORCE24HOURFORMAT, &curtime, input, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  STRINGSA("tt HH':'mm'@'ss", "A"); /* Insufficent buffer */
  ret = GetTimeFormatA(lcid, TIME_FORCE24HOURFORMAT, &curtime, input, buffer, 2);
  EXPECT_BUFFER; EXPECT_LEN(0); EXPECT_EQA;

  STRINGSA("tt HH':'mm'@'ss", "AM 08:56@13"); /* Calculate length only */
  ret = GetTimeFormatA(lcid, TIME_FORCE24HOURFORMAT, &curtime, input, NULL, 0);
  EXPECT_VALID; EXPECT_LENA;

  STRINGSA("", "8 AM"); /* TIME_NOMINUTESORSECONDS, default format */
  ret = GetTimeFormatA(lcid, NUO|TIME_NOMINUTESORSECONDS, &curtime, NULL, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  STRINGSA("m1s2m3s4", ""); /* TIME_NOMINUTESORSECONDS/complex format */
  ret = GetTimeFormatA(lcid, TIME_NOMINUTESORSECONDS, &curtime, input, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  STRINGSA("", "8:56 AM"); /* TIME_NOSECONDS/Default format */
  ret = GetTimeFormatA(lcid, NUO|TIME_NOSECONDS, &curtime, NULL, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  STRINGSA("h:m:s tt", "8:56 AM"); /* TIME_NOSECONDS */
  strcpy(Expected, "8:56 AM");
  ret = GetTimeFormatA(lcid, TIME_NOSECONDS, &curtime, input, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  STRINGSA("h.@:m.@:s.@:tt", "8.@:56AM"); /* Multiple delimiters */
  ret = GetTimeFormatA(lcid, TIME_NOSECONDS, &curtime, input, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  STRINGSA("s1s2s3", ""); /* Duplicate tokens */
  ret = GetTimeFormatA(lcid, TIME_NOSECONDS, &curtime, input, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  STRINGSA("t/tt", "A/AM"); /* AM time marker */
  ret = GetTimeFormatA(lcid, 0, &curtime, input, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  curtime.wHour = 13;
  STRINGSA("t/tt", "P/PM"); /* PM time marker */
  ret = GetTimeFormatA(lcid, 0, &curtime, input, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  STRINGSA("h1t2tt3m", "156"); /* TIME_NOTIMEMARKER: removes text around time marker token */
  ret = GetTimeFormatA(lcid, TIME_NOTIMEMARKER, &curtime, input, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  STRINGSA("h:m:s tt", "13:56:13 PM"); /* TIME_FORCE24HOURFORMAT */
  ret = GetTimeFormatA(lcid, TIME_FORCE24HOURFORMAT, &curtime, input, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  STRINGSA("h:m:s", "13:56:13"); /* TIME_FORCE24HOURFORMAT doesn't add time marker */
  ret = GetTimeFormatA(lcid, TIME_FORCE24HOURFORMAT, &curtime, input, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  curtime.wHour = 14; /* change this to 14 or 2pm */
  curtime.wMinute = 5;
  curtime.wSecond = 3;
  STRINGSA("h hh H HH m mm s ss t tt", "2 02 14 14 5 05 3 03 P PM"); /* 24 hrs, leading 0 */
  ret = GetTimeFormatA(lcid, 0, &curtime, input, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  curtime.wHour = 0;
  STRINGSA("h/H/hh/HH", "12/0/12/00"); /* "hh" and "HH" */
  ret = GetTimeFormatA(lcid, 0, &curtime, input, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  STRINGSA("h:m:s tt", "12:5:3 AM"); /* non-zero flags should fail with format, doesn't */
  ret = GetTimeFormatA(lcid, 0, &curtime, input, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  /* try to convert formatting strings with more than two letters
   * "h:hh:hhh:H:HH:HHH:m:mm:mmm:M:MM:MMM:s:ss:sss:S:SS:SSS"
   * NOTE: We expect any letter for which there is an upper case value
   *       we should see a replacement.  For letters that DO NOT have
   *       upper case values we should see NO REPLACEMENT.
   */
  curtime.wHour = 8;
  curtime.wMinute = 56;
  curtime.wSecond = 13;
  curtime.wMilliseconds = 22;
  STRINGSA("h:hh:hhh H:HH:HHH m:mm:mmm M:MM:MMM s:ss:sss S:SS:SSS",
           "8:08:08 8:08:08 56:56:56 M:MM:MMM 13:13:13 S:SS:SSS");
  ret = GetTimeFormatA(lcid, 0, &curtime, input, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  STRINGSA("h", "text"); /* Dont write to buffer if len is 0*/
  strcpy(buffer, "text");
  ret = GetTimeFormatA(lcid, 0, &curtime, input, buffer, 0);
  EXPECT_VALID; EXPECT_LEN(2); EXPECT_EQA;

  STRINGSA("h 'h' H 'H' HH 'HH' m 'm' s 's' t 't' tt 'tt'",
           "8 h 8 H 08 HH 56 m 13 s A t AM tt"); /* "'" preserves tokens */
  ret = GetTimeFormatA(lcid, 0, &curtime, input, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  STRINGSA("'''", "'"); /* invalid quoted string */
  ret = GetTimeFormatA(lcid, 0, &curtime, input, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  /* test that msdn suggested single quotation usage works as expected */
  STRINGSA("''''", "'"); /* single quote mark */
  ret = GetTimeFormatA(lcid, 0, &curtime, input, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  STRINGSA("''HHHHHH", "08"); /* Normal use */
  ret = GetTimeFormatA(lcid, 0, &curtime, input, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  /* and test for normal use of the single quotation mark */
  STRINGSA("'''HHHHHH'", "'HHHHHH"); /* Normal use */
  ret = GetTimeFormatA(lcid, 0, &curtime, input, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  STRINGSA("'''HHHHHH", "'HHHHHH"); /* Odd use */
  ret = GetTimeFormatA(lcid, 0, &curtime, input, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  STRINGSA("'123'tt", ""); /* TIME_NOTIMEMARKER drops literals too */
  ret = GetTimeFormatA(lcid, TIME_NOTIMEMARKER, &curtime, input, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  curtime.wHour = 25;
  STRINGSA("'123'tt", ""); /* Invalid time */
  ret = GetTimeFormatA(lcid, 0, &curtime, input, buffer, COUNTOF(buffer));
  EXPECT_INVALID; EXPECT_LEN(0); EXPECT_EQA;

  curtime.wHour = 12;
  curtime.wMonth = 60; /* Invalid */
  STRINGSA("h:m:s", "12:56:13"); /* Invalid date */
  ret = GetTimeFormatA(lcid, 0, &curtime, input, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;
}

static void test_GetDateFormatA()
{
  int ret;
  SYSTEMTIME  curtime;
  LCID lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);
  char buffer[BUFFER_SIZE], input[BUFFER_SIZE], Expected[BUFFER_SIZE];

  memset(&curtime, 2, sizeof(SYSTEMTIME)); /* Invalid time */
  STRINGSA("ddd',' MMM dd yy","");
  ret = GetDateFormatA(lcid, 0, &curtime, input, buffer, COUNTOF(buffer));
  EXPECT_INVALID; EXPECT_LEN(0); EXPECT_EQA;

  curtime.wYear = 2002;
  curtime.wMonth = 5;
  curtime.wDay = 4;
  curtime.wDayOfWeek = 3;
  STRINGSA("ddd',' MMM dd yy","Sat, May 04 02"); /* Simple case */
  ret = GetDateFormatA(lcid, 0, &curtime, input, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  STRINGSA("ddd',' MMM dd yy","Sat, May 04 02"); /* Format containing "'" */
  ret = GetDateFormatA(lcid, 0, &curtime, input, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  curtime.wHour = 36; /* Invalid */
  STRINGSA("ddd',' MMM dd ''''yy","Sat, May 04 '02"); /* Invalid time */
  ret = GetDateFormatA(lcid, 0, &curtime, input, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  STRINGSA("ddd',' MMM dd ''''yy",""); /* Get size only */
  ret = GetDateFormatA(lcid, 0, &curtime, input, NULL, 0);
  EXPECT_VALID; EXPECT_LEN(16); EXPECT_EQA;

  STRINGSA("ddd',' MMM dd ''''yy",""); /* Buffer too small */
  ret = GetDateFormatA(lcid, 0, &curtime, input, buffer, 2);
  EXPECT_BUFFER; EXPECT_LEN(0); EXPECT_EQA;

  STRINGSA("ddd',' MMM dd ''''yy","5/4/2002"); /* Default to DATE_SHORTDATE */
  ret = GetDateFormat(lcid, NUO, &curtime, NULL, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  STRINGSA("ddd',' MMM dd ''''yy", "Saturday, May 04, 2002"); /* DATE_LONGDATE */
  ret = GetDateFormat(lcid, NUO|DATE_LONGDATE, &curtime, NULL, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  /* test for expected DATE_YEARMONTH behavior with null format */
  /* NT4 returns ERROR_INVALID_FLAGS for DATE_YEARMONTH */
  STRINGSA("ddd',' MMM dd ''''yy", ""); /* DATE_YEARMONTH */
  ret = GetDateFormat(lcid, NUO|DATE_YEARMONTH, &curtime, input, buffer, COUNTOF(buffer));
  EXPECT_FLAGS; EXPECT_LEN(0); EXPECT_EQA;

  /* Test that using invalid DATE_* flags results in the correct error */
  /* and return values */
  STRINGSA("m/d/y", ""); /* Invalid flags */
  ret = GetDateFormat(lcid, DATE_YEARMONTH|DATE_SHORTDATE|DATE_LONGDATE,
                      &curtime, input, buffer, COUNTOF(buffer));
  EXPECT_FLAGS; EXPECT_LEN(0); EXPECT_EQA;
}

static void test_GetDateFormatW()
{
  int ret;
  SYSTEMTIME  curtime;
  WCHAR buffer[BUFFER_SIZE], input[BUFFER_SIZE], Expected[BUFFER_SIZE];
  LCID lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);

  STRINGSW("",""); /* If flags is not zero then format must be NULL */
  ret = GetDateFormatW(LOCALE_SYSTEM_DEFAULT, DATE_LONGDATE, NULL,
                       input, buffer, COUNTOF(buffer));
  if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
      return;
  EXPECT_FLAGS; EXPECT_LEN(0); EXPECT_EQW;

  STRINGSW("",""); /* NULL buffer, len > 0 */
  ret = GetDateFormatW (lcid, 0, NULL, input, NULL, COUNTOF(buffer));
  EXPECT_INVALID; EXPECT_LEN(0); EXPECT_EQW;

  STRINGSW("",""); /* NULL buffer, len == 0 */
  ret = GetDateFormatW (lcid, 0, NULL, input, NULL, 0);
  EXPECT_VALID; EXPECT_LENW; EXPECT_EQW;

  curtime.wYear = 2002;
  curtime.wMonth = 10;
  curtime.wDay = 23;
  curtime.wDayOfWeek = 45612; /* Should be 3 - Wednesday */
  curtime.wHour = 65432; /* Invalid */
  curtime.wMinute = 34512; /* Invalid */
  curtime.wSecond = 65535; /* Invalid */
  curtime.wMilliseconds = 12345;
  STRINGSW("dddd d MMMM yyyy","Wednesday 23 October 2002"); /* Incorrect DOW and time */
  ret = GetDateFormatW (lcid, 0, &curtime, input, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENW; EXPECT_EQW;
}


#define CY_POS_LEFT  0
#define CY_POS_RIGHT 1
#define CY_POS_LEFT_SPACE  2
#define CY_POS_RIGHT_SPACE 3

static void test_GetCurrencyFormatA()
{
  static char szDot[] = { '.', '\0' };
  static char szComma[] = { ',', '\0' };
  static char szDollar[] = { '$', '\0' };
  int ret;
  LCID lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);
  char buffer[BUFFER_SIZE], Expected[BUFFER_SIZE], input[BUFFER_SIZE];
  CURRENCYFMTA format;

  memset(&format, 0, sizeof(format));

  STRINGSA("23",""); /* NULL output, length > 0 --> Error */
  ret = GetCurrencyFormatA(lcid, 0, input, NULL, NULL, COUNTOF(buffer));
  EXPECT_INVALID; EXPECT_LEN(0); EXPECT_EQA;

  STRINGSA("23,53",""); /* Invalid character --> Error */
  ret = GetCurrencyFormatA(lcid, 0, input, NULL, buffer, COUNTOF(buffer));
  EXPECT_INVALID; EXPECT_LEN(0); EXPECT_EQA;

  STRINGSA("--",""); /* Double '-' --> Error */
  ret = GetCurrencyFormatA(lcid, 0, input, NULL, buffer, COUNTOF(buffer));
  EXPECT_INVALID; EXPECT_LEN(0); EXPECT_EQA;

  STRINGSA("0-",""); /* Trailing '-' --> Error */
  ret = GetCurrencyFormatA(lcid, 0, input, NULL, buffer, COUNTOF(buffer));
  EXPECT_INVALID; EXPECT_LEN(0); EXPECT_EQA;

  STRINGSA("0..",""); /* Double '.' --> Error */
  ret = GetCurrencyFormatA(lcid, 0, input, NULL, buffer, COUNTOF(buffer));
  EXPECT_INVALID; EXPECT_LEN(0); EXPECT_EQA;

  STRINGSA(" 0.1",""); /* Leading space --> Error */
  ret = GetCurrencyFormatA(lcid, 0, input, NULL, buffer, COUNTOF(buffer));
  EXPECT_INVALID; EXPECT_LEN(0); EXPECT_EQA;

  STRINGSA("1234","$"); /* Length too small --> Write up to length chars */
  ret = GetCurrencyFormatA(lcid, NUO, input, NULL, buffer, 2);
  EXPECT_BUFFER; EXPECT_LEN(0); EXPECT_EQA;

  STRINGSA("2353",""); /* Format and flags given --> Error */
  ret = GetCurrencyFormatA(lcid, NUO, input, &format, buffer, COUNTOF(buffer));
  EXPECT_INVALIDFLAGS; EXPECT_LEN(0); EXPECT_EQA;

  STRINGSA("2353",""); /* Invalid format --> Error */
  ret = GetCurrencyFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  EXPECT_INVALID; EXPECT_LEN(0); EXPECT_EQA;

  STRINGSA("2353","$2,353.00"); /* Valid number */
  ret = GetCurrencyFormatA(lcid, NUO, input, NULL, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  STRINGSA("-2353","($2,353.00)"); /* Valid negative number */
  ret = GetCurrencyFormatA(lcid, NUO, input, NULL, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  STRINGSA("2353.1","$2,353.10"); /* Valid real number */
  ret = GetCurrencyFormatA(lcid, NUO, input, NULL, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  STRINGSA("2353.111","$2,353.11"); /* Too many DP --> Truncated */
  ret = GetCurrencyFormatA(lcid, NUO, input, NULL, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  STRINGSA("2353.119","$2,353.12");  /* Too many DP --> Rounded */
  ret = GetCurrencyFormatA(lcid, NUO, input, NULL, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  format.NumDigits = 0; /* No decimal separator */
  format.LeadingZero = 0;
  format.Grouping = 0;  /* No grouping char */
  format.NegativeOrder = 0;
  format.PositiveOrder = CY_POS_LEFT;
  format.lpDecimalSep = szDot;
  format.lpThousandSep = szComma;
  format.lpCurrencySymbol = szDollar;

  STRINGSA("2353","$2353"); /* No decimal or grouping chars expected */
  ret = GetCurrencyFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  format.NumDigits = 1; /* 1 DP --> Expect decimal separator */
  STRINGSA("2353","$2353.0");
  ret = GetCurrencyFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  format.Grouping = 2; /* Group by 100's */
  STRINGSA("2353","$23,53.0");
  ret = GetCurrencyFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  format.LeadingZero = 1; /* Always provide leading zero */
  STRINGSA(".5","$0.5");
  ret = GetCurrencyFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  format.PositiveOrder = CY_POS_RIGHT;
  STRINGSA("1","1.0$");
  ret = GetCurrencyFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  format.PositiveOrder = CY_POS_LEFT_SPACE;
  STRINGSA("1","$ 1.0");
  ret = GetCurrencyFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  format.PositiveOrder = CY_POS_RIGHT_SPACE;
  STRINGSA("1","1.0 $");
  ret = GetCurrencyFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  format.NegativeOrder = 0;
  STRINGSA("-1","($1.0)");
  ret = GetCurrencyFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  format.NegativeOrder = 1;
  STRINGSA("-1","-$1.0");
  ret = GetCurrencyFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  format.NegativeOrder = 2;
  STRINGSA("-1","$-1.0");
  ret = GetCurrencyFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  format.NegativeOrder = 3;
  STRINGSA("-1","$1.0-");
  ret = GetCurrencyFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  format.NegativeOrder = 4;
  STRINGSA("-1","(1.0$)");
  ret = GetCurrencyFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  format.NegativeOrder = 5;
  STRINGSA("-1","-1.0$");
  ret = GetCurrencyFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  format.NegativeOrder = 6;
  STRINGSA("-1","1.0-$");
  ret = GetCurrencyFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  format.NegativeOrder = 7;
  STRINGSA("-1","1.0$-");
  ret = GetCurrencyFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  format.NegativeOrder = 8;
  STRINGSA("-1","-1.0 $");
  ret = GetCurrencyFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  format.NegativeOrder = 9;
  STRINGSA("-1","-$ 1.0");
  ret = GetCurrencyFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  format.NegativeOrder = 10;
  STRINGSA("-1","1.0 $-");
  ret = GetCurrencyFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  format.NegativeOrder = 11;
  STRINGSA("-1","$ 1.0-");
  ret = GetCurrencyFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  format.NegativeOrder = 12;
  STRINGSA("-1","$ -1.0");
  ret = GetCurrencyFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  format.NegativeOrder = 13;
  STRINGSA("-1","1.0- $");
  ret = GetCurrencyFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  format.NegativeOrder = 14;
  STRINGSA("-1","($ 1.0)");
  ret = GetCurrencyFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  format.NegativeOrder = 15;
  STRINGSA("-1","(1.0 $)");
  ret = GetCurrencyFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;
}

#define NEG_PARENS      0 /* "(1.1)" */
#define NEG_LEFT        1 /* "-1.1"  */
#define NEG_LEFT_SPACE  2 /* "- 1.1" */
#define NEG_RIGHT       3 /* "1.1-"  */
#define NEG_RIGHT_SPACE 4 /* "1.1 -" */

static void test_GetNumberFormatA()
{
  static char szDot[] = { '.', '\0' };
  static char szComma[] = { ',', '\0' };
  int ret;
  LCID lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);
  char buffer[BUFFER_SIZE], Expected[BUFFER_SIZE], input[BUFFER_SIZE];
  NUMBERFMTA format;

  memset(&format, 0, sizeof(format));

  STRINGSA("23",""); /* NULL output, length > 0 --> Error */
  ret = GetNumberFormatA(lcid, 0, input, NULL, NULL, COUNTOF(buffer));
  EXPECT_INVALID; EXPECT_LEN(0); EXPECT_EQA;

  STRINGSA("23,53",""); /* Invalid character --> Error */
  ret = GetNumberFormatA(lcid, 0, input, NULL, buffer, COUNTOF(buffer));
  EXPECT_INVALID; EXPECT_LEN(0); EXPECT_EQA;

  STRINGSA("--",""); /* Double '-' --> Error */
  ret = GetNumberFormatA(lcid, 0, input, NULL, buffer, COUNTOF(buffer));
  EXPECT_INVALID; EXPECT_LEN(0); EXPECT_EQA;

  STRINGSA("0-",""); /* Trailing '-' --> Error */
  ret = GetNumberFormatA(lcid, 0, input, NULL, buffer, COUNTOF(buffer));
  EXPECT_INVALID; EXPECT_LEN(0); EXPECT_EQA;

  STRINGSA("0..",""); /* Double '.' --> Error */
  ret = GetNumberFormatA(lcid, 0, input, NULL, buffer, COUNTOF(buffer));
  EXPECT_INVALID; EXPECT_LEN(0); EXPECT_EQA;

  STRINGSA(" 0.1",""); /* Leading space --> Error */
  ret = GetNumberFormatA(lcid, 0, input, NULL, buffer, COUNTOF(buffer));
  EXPECT_INVALID; EXPECT_LEN(0); EXPECT_EQA;

  STRINGSA("1234","1"); /* Length too small --> Write up to length chars */
  ret = GetNumberFormatA(lcid, NUO, input, NULL, buffer, 2);
  EXPECT_BUFFER; EXPECT_LEN(0); EXPECT_EQA;

  STRINGSA("2353",""); /* Format and flags given --> Error */
  ret = GetNumberFormatA(lcid, NUO, input, &format, buffer, COUNTOF(buffer));
  EXPECT_INVALIDFLAGS; EXPECT_LEN(0); EXPECT_EQA;

  STRINGSA("2353",""); /* Invalid format --> Error */
  ret = GetNumberFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  EXPECT_INVALID; EXPECT_LEN(0); EXPECT_EQA;

  STRINGSA("2353","2,353.00"); /* Valid number */
  ret = GetNumberFormatA(lcid, NUO, input, NULL, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  STRINGSA("-2353","-2,353.00"); /* Valid negative number */
  ret = GetNumberFormatA(lcid, NUO, input, NULL, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  STRINGSA("2353.1","2,353.10"); /* Valid real number */
  ret = GetNumberFormatA(lcid, NUO, input, NULL, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  STRINGSA("2353.111","2,353.11"); /* Too many DP --> Truncated */
  ret = GetNumberFormatA(lcid, NUO, input, NULL, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  STRINGSA("2353.119","2,353.12");  /* Too many DP --> Rounded */
  ret = GetNumberFormatA(lcid, NUO, input, NULL, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  format.NumDigits = 0; /* No decimal separator */
  format.LeadingZero = 0;
  format.Grouping = 0;  /* No grouping char */
  format.NegativeOrder = 0;
  format.lpDecimalSep = szDot;
  format.lpThousandSep = szComma;

  STRINGSA("2353","2353"); /* No decimal or grouping chars expected */
  ret = GetNumberFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  format.NumDigits = 1; /* 1 DP --> Expect decimal separator */
  STRINGSA("2353","2353.0");
  ret = GetNumberFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  format.Grouping = 2; /* Group by 100's */
  STRINGSA("2353","23,53.0");
  ret = GetNumberFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  format.LeadingZero = 1; /* Always provide leading zero */
  STRINGSA(".5","0.5");
  ret = GetNumberFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  format.NegativeOrder = NEG_PARENS;
  STRINGSA("-1","(1.0)");
  ret = GetNumberFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  format.NegativeOrder = NEG_LEFT;
  STRINGSA("-1","-1.0");
  ret = GetNumberFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  format.NegativeOrder = NEG_LEFT_SPACE;
  STRINGSA("-1","- 1.0");
  ret = GetNumberFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  format.NegativeOrder = NEG_RIGHT;
  STRINGSA("-1","1.0-");
  ret = GetNumberFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  format.NegativeOrder = NEG_RIGHT_SPACE;
  STRINGSA("-1","1.0 -");
  ret = GetNumberFormatA(lcid, 0, input, &format, buffer, COUNTOF(buffer));
  EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;

  lcid = MAKELCID(MAKELANGID(LANG_FRENCH, SUBLANG_DEFAULT), SORT_DEFAULT);

  if (IsValidLocale(lcid, 0))
  {
    STRINGSA("-12345","-12 345,00"); /* Try French formatting */
    Expected[3] = 160; /* Non breaking space */
    ret = GetNumberFormatA(lcid, NUO, input, NULL, buffer, COUNTOF(buffer));
    EXPECT_VALID; EXPECT_LENA; EXPECT_EQA;
  }
}


/* Callback function used by TestEnumTimeFormats */
static BOOL CALLBACK EnumTimeFormatsProc(char * lpTimeFormatString)
{
  trace("%s\n", lpTimeFormatString);
  strcpy(GlobalBuffer, lpTimeFormatString);
#if 0
  return TRUE;
#endif
  return FALSE;
}

void test_EnumTimeFormats()
{
  int ret;
  LCID lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);

  GlobalBuffer[0] = '\0';
  ret = EnumTimeFormatsA(EnumTimeFormatsProc, lcid, 0);
  ok (ret == 1 && !strcmp(GlobalBuffer,"h:mm:ss tt"), "Expected %d '%s'\n", ret, GlobalBuffer);
}

static void test_CompareStringA()
{
  int ret;
  LCID lcid = MAKELCID(MAKELANGID(LANG_FRENCH, SUBLANG_DEFAULT), SORT_DEFAULT);

  ret = CompareStringA(lcid, NORM_IGNORECASE, "Salut", -1, "Salute", -1);
  ok (ret== 1, "(Salut/Salute) Expected 1, got %d\n", ret);

  ret = CompareStringA(lcid, NORM_IGNORECASE, "Salut", -1, "SaLuT", -1);
  ok (ret== 2, "(Salut/SaLuT) Expected 2, got %d\n", ret);

  ret = CompareStringA(lcid, NORM_IGNORECASE, "Salut", -1, "hola", -1);
  ok (ret== 3, "(Salut/hola) Expected 3, got %d\n", ret);

  ret = CompareStringA(lcid, NORM_IGNORECASE, "haha", -1, "hoho", -1);
  ok (ret== 1, "(haha/hoho) Expected 1, got %d\n", ret);

  lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);

  ret = CompareStringA(lcid, NORM_IGNORECASE, "haha", -1, "hoho", -1);
  ok (ret== 1, "(haha/hoho) Expected 1, got %d\n", ret);

  ret = CompareStringA(lcid, NORM_IGNORECASE, "haha", -1, "hoho", 0);
  ok (ret== 3, "(haha/hoho) Expected 3, got %d\n", ret);

  ret = CompareStringA(lcid, NORM_IGNORECASE, "Salut", 5, "SaLuT", -1);
  ok (ret== 2, "(Salut/SaLuT) Expected 2, got %d\n", ret);
}

void test_LCMapStringA(void)
{
    int ret, ret2;
    char buf[256], buf2[256];
    static const char upper_case[] = "\tJUST! A, TEST; STRING 1/*+-.\r\n";
    static const char lower_case[] = "\tjust! a, test; string 1/*+-.\r\n";
    static const char symbols_stripped[] = "justateststring1";

    ret = LCMapStringA(LOCALE_USER_DEFAULT, LCMAP_LOWERCASE | LCMAP_UPPERCASE,
                       upper_case, -1, buf, sizeof(buf));
    ok(!ret, "LCMAP_LOWERCASE and LCMAP_UPPERCASE are mutually exclusive\n");
    ok(GetLastError() == ERROR_INVALID_FLAGS,
       "unexpected error code %ld\n", GetLastError());

    ret = LCMapStringA(LOCALE_USER_DEFAULT, LCMAP_HIRAGANA | LCMAP_KATAKANA,
                       upper_case, -1, buf, sizeof(buf));
    ok(!ret, "LCMAP_HIRAGANA and LCMAP_KATAKANA are mutually exclusive\n");
    ok(GetLastError() == ERROR_INVALID_FLAGS,
       "unexpected error code %ld\n", GetLastError());

    ret = LCMapStringA(LOCALE_USER_DEFAULT, LCMAP_HALFWIDTH | LCMAP_FULLWIDTH,

                       upper_case, -1, buf, sizeof(buf));
    ok(!ret, "LCMAP_HALFWIDTH | LCMAP_FULLWIDTH are mutually exclusive\n");
    ok(GetLastError() == ERROR_INVALID_FLAGS,
       "unexpected error code %ld\n", GetLastError());

    ret = LCMapStringA(LOCALE_USER_DEFAULT, LCMAP_TRADITIONAL_CHINESE | LCMAP_SIMPLIFIED_CHINESE,
                       upper_case, -1, buf, sizeof(buf));
    ok(!ret, "LCMAP_TRADITIONAL_CHINESE and LCMAP_SIMPLIFIED_CHINESE are mutually exclusive\n");
    ok(GetLastError() == ERROR_INVALID_FLAGS,
       "unexpected error code %ld\n", GetLastError());

    /* SORT_STRINGSORT must be used exclusively with LCMAP_SORTKEY */
    SetLastError(0xdeadbeef);
    ret = LCMapStringA(LOCALE_USER_DEFAULT, LCMAP_LOWERCASE | SORT_STRINGSORT,
                       upper_case, -1, buf, sizeof(buf));
    ok(GetLastError() == ERROR_INVALID_FLAGS, "expected ERROR_INVALID_FLAGS, got %ld\n", GetLastError());
    ok(!ret, "SORT_STRINGSORT without LCMAP_SORTKEY must fail\n");

    /* test LCMAP_LOWERCASE */
    ret = LCMapStringA(LOCALE_USER_DEFAULT, LCMAP_LOWERCASE,
                       upper_case, -1, buf, sizeof(buf));
    ok(ret == lstrlenA(upper_case) + 1,
       "ret %d, error %ld, expected value %d\n",
       ret, GetLastError(), lstrlenA(upper_case) + 1);
    ok(!lstrcmpA(buf, lower_case), "LCMapStringA should return %s, but not %s\n", lower_case, buf);

    /* test LCMAP_UPPERCASE */
    ret = LCMapStringA(LOCALE_USER_DEFAULT, LCMAP_UPPERCASE,
                       lower_case, -1, buf, sizeof(buf));
    ok(ret == lstrlenA(lower_case) + 1,
       "ret %d, error %ld, expected value %d\n",
       ret, GetLastError(), lstrlenA(lower_case) + 1);
    ok(!lstrcmpA(buf, upper_case), "LCMapStringA should return %s, but not %s\n", upper_case, buf);

    /* LCMAP_UPPERCASE or LCMAP_LOWERCASE should accept src == dst */
    lstrcpyA(buf, lower_case);
    ret = LCMapStringA(LOCALE_USER_DEFAULT, LCMAP_UPPERCASE,
                       buf, -1, buf, sizeof(buf));
    if (!ret) /* Win9x */
        trace("Ignoring LCMapStringA(LCMAP_UPPERCASE, buf, buf) error on Win9x\n");
    else
    {
        ok(ret == lstrlenA(lower_case) + 1,
           "ret %d, error %ld, expected value %d\n",
           ret, GetLastError(), lstrlenA(lower_case) + 1);
        ok(!lstrcmpA(buf, upper_case), "LCMapStringA should return %s, but not %s\n", upper_case, buf);
    }
    lstrcpyA(buf, upper_case);
    ret = LCMapStringA(LOCALE_USER_DEFAULT, LCMAP_LOWERCASE,
                       buf, -1, buf, sizeof(buf));
    if (!ret) /* Win9x */
        trace("Ignoring LCMapStringA(LCMAP_LOWERCASE, buf, buf) error on Win9x\n");
    else
    {
        ok(ret == lstrlenA(upper_case) + 1,
           "ret %d, error %ld, expected value %d\n",
           ret, GetLastError(), lstrlenA(lower_case) + 1);
        ok(!lstrcmpA(buf, lower_case), "LCMapStringA should return %s, but not %s\n", lower_case, buf);
    }

    /* otherwise src == dst should fail */
    SetLastError(0xdeadbeef);
    ret = LCMapStringA(LOCALE_USER_DEFAULT, LCMAP_SORTKEY | LCMAP_UPPERCASE,
                       buf, 10, buf, sizeof(buf));
    ok(GetLastError() == ERROR_INVALID_FLAGS /* NT */ ||
       GetLastError() == ERROR_INVALID_PARAMETER /* Win9x */,
       "unexpected error code %ld\n", GetLastError());
    ok(!ret, "src == dst without LCMAP_UPPERCASE or LCMAP_LOWERCASE must fail\n");

    /* test whether '\0' is always appended */
    ret = LCMapStringA(LOCALE_USER_DEFAULT, LCMAP_SORTKEY,
                       upper_case, -1, buf, sizeof(buf));
    ok(ret, "LCMapStringA must succeed\n");
    ret2 = LCMapStringA(LOCALE_USER_DEFAULT, LCMAP_SORTKEY,
                       upper_case, lstrlenA(upper_case), buf2, sizeof(buf2));
    ok(ret, "LCMapStringA must succeed\n");
    ok(ret == ret2, "lengths of sort keys must be equal\n");
    ok(!lstrcmpA(buf, buf2), "sort keys must be equal\n");

    /* test LCMAP_SORTKEY | NORM_IGNORECASE */
    ret = LCMapStringA(LOCALE_USER_DEFAULT, LCMAP_SORTKEY | NORM_IGNORECASE,
                       upper_case, -1, buf, sizeof(buf));
    ok(ret, "LCMapStringA must succeed\n");
    ret2 = LCMapStringA(LOCALE_USER_DEFAULT, LCMAP_SORTKEY,
                       lower_case, -1, buf2, sizeof(buf2));
    ok(ret2, "LCMapStringA must succeed\n");
    ok(ret == ret2, "lengths of sort keys must be equal\n");
    ok(!lstrcmpA(buf, buf2), "sort keys must be equal\n");

    /* test LCMAP_SORTKEY | NORM_IGNORENONSPACE */
    ret = LCMapStringA(LOCALE_USER_DEFAULT, LCMAP_SORTKEY | NORM_IGNORENONSPACE,
                       lower_case, -1, buf, sizeof(buf));
    ok(ret, "LCMapStringA must succeed\n");
    ret2 = LCMapStringA(LOCALE_USER_DEFAULT, LCMAP_SORTKEY,
                       lower_case, -1, buf2, sizeof(buf2));
    ok(ret2, "LCMapStringA must succeed\n");
    ok(ret == ret2, "lengths of sort keys must be equal\n");
    ok(!lstrcmpA(buf, buf2), "sort keys must be equal\n");

    /* test LCMAP_SORTKEY | NORM_IGNORESYMBOLS */
    ret = LCMapStringA(LOCALE_USER_DEFAULT, LCMAP_SORTKEY | NORM_IGNORESYMBOLS,
                       lower_case, -1, buf, sizeof(buf));
    ok(ret, "LCMapStringA must succeed\n");
    ret2 = LCMapStringA(LOCALE_USER_DEFAULT, LCMAP_SORTKEY,
                       symbols_stripped, -1, buf2, sizeof(buf2));
    ok(ret2, "LCMapStringA must succeed\n");
    ok(ret == ret2, "lengths of sort keys must be equal\n");
    ok(!lstrcmpA(buf, buf2), "sort keys must be equal\n");

    /* test NORM_IGNORENONSPACE */
    lstrcpyA(buf, "foo");
    ret = LCMapStringA(LOCALE_USER_DEFAULT, NORM_IGNORENONSPACE,
                       lower_case, -1, buf, sizeof(buf));
    ok(ret == lstrlenA(lower_case) + 1, "LCMapStringA should return %d, ret = %d\n",
	lstrlenA(lower_case) + 1, ret);
    ok(!lstrcmpA(buf, lower_case), "LCMapStringA should return %s, but not %s\n", lower_case, buf);

    /* test NORM_IGNORESYMBOLS */
    lstrcpyA(buf, "foo");
    ret = LCMapStringA(LOCALE_USER_DEFAULT, NORM_IGNORESYMBOLS,
                       lower_case, -1, buf, sizeof(buf));
    ok(ret == lstrlenA(symbols_stripped) + 1, "LCMapStringA should return %d, ret = %d\n",
	lstrlenA(symbols_stripped) + 1, ret);
    ok(!lstrcmpA(buf, symbols_stripped), "LCMapStringA should return %s, but not %s\n", lower_case, buf);
}

void test_LCMapStringW(void)
{
    int ret, ret2;
    WCHAR buf[256], buf2[256];
    char *p_buf = (char *)buf, *p_buf2 = (char *)buf2;
    static const WCHAR upper_case[] = {'\t','J','U','S','T','!',' ','A',',',' ','T','E','S','T',';',' ','S','T','R','I','N','G',' ','1','/','*','+','-','.','\r','\n',0};
    static const WCHAR lower_case[] = {'\t','j','u','s','t','!',' ','a',',',' ','t','e','s','t',';',' ','s','t','r','i','n','g',' ','1','/','*','+','-','.','\r','\n',0};
    static const WCHAR symbols_stripped[] = {'j','u','s','t','a','t','e','s','t','s','t','r','i','n','g','1',0};
    static const WCHAR fooW[] = {'f','o','o',0};

    ret = LCMapStringW(LOCALE_USER_DEFAULT, LCMAP_LOWERCASE | LCMAP_UPPERCASE,
                       upper_case, -1, buf, sizeof(buf)/sizeof(WCHAR));
    if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        trace("Skipping LCMapStringW tests on Win9x\n");
        return;
    }
    ok(!ret, "LCMAP_LOWERCASE and LCMAP_UPPERCASE are mutually exclusive\n");
    ok(GetLastError() == ERROR_INVALID_FLAGS,
       "unexpected error code %ld\n", GetLastError());

    ret = LCMapStringW(LOCALE_USER_DEFAULT, LCMAP_HIRAGANA | LCMAP_KATAKANA,
                       upper_case, -1, buf, sizeof(buf)/sizeof(WCHAR));
    ok(!ret, "LCMAP_HIRAGANA and LCMAP_KATAKANA are mutually exclusive\n");
    ok(GetLastError() == ERROR_INVALID_FLAGS,
       "unexpected error code %ld\n", GetLastError());

    ret = LCMapStringW(LOCALE_USER_DEFAULT, LCMAP_HALFWIDTH | LCMAP_FULLWIDTH,
                       upper_case, -1, buf, sizeof(buf)/sizeof(WCHAR));
    ok(!ret, "LCMAP_HALFWIDTH | LCMAP_FULLWIDTH are mutually exclusive\n");
    ok(GetLastError() == ERROR_INVALID_FLAGS,
       "unexpected error code %ld\n", GetLastError());

    ret = LCMapStringW(LOCALE_USER_DEFAULT, LCMAP_TRADITIONAL_CHINESE | LCMAP_SIMPLIFIED_CHINESE,
                       upper_case, -1, buf, sizeof(buf)/sizeof(WCHAR));
    ok(!ret, "LCMAP_TRADITIONAL_CHINESE and LCMAP_SIMPLIFIED_CHINESE are mutually exclusive\n");
    ok(GetLastError() == ERROR_INVALID_FLAGS,
       "unexpected error code %ld\n", GetLastError());

    /* SORT_STRINGSORT must be used exclusively with LCMAP_SORTKEY */
    SetLastError(0xdeadbeef);
    ret = LCMapStringW(LOCALE_USER_DEFAULT, LCMAP_LOWERCASE | SORT_STRINGSORT,
                       upper_case, -1, buf, sizeof(buf)/sizeof(WCHAR));
    ok(GetLastError() == ERROR_INVALID_FLAGS, "expected ERROR_INVALID_FLAGS, got %ld\n", GetLastError());
    ok(!ret, "SORT_STRINGSORT without LCMAP_SORTKEY must fail\n");

    /* test LCMAP_LOWERCASE */
    ret = LCMapStringW(LOCALE_USER_DEFAULT, LCMAP_LOWERCASE,
                       upper_case, -1, buf, sizeof(buf)/sizeof(WCHAR));
    ok(ret == lstrlenW(upper_case) + 1,
       "ret %d, error %ld, expected value %d\n",
       ret, GetLastError(), lstrlenW(upper_case) + 1);
    ok(!lstrcmpW(buf, lower_case), "string compare mismatch\n");

    /* test LCMAP_UPPERCASE */
    ret = LCMapStringW(LOCALE_USER_DEFAULT, LCMAP_UPPERCASE,
                       lower_case, -1, buf, sizeof(buf)/sizeof(WCHAR));
    ok(ret == lstrlenW(lower_case) + 1,
       "ret %d, error %ld, expected value %d\n",
       ret, GetLastError(), lstrlenW(lower_case) + 1);
    ok(!lstrcmpW(buf, upper_case), "string compare mismatch\n");

    /* LCMAP_UPPERCASE or LCMAP_LOWERCASE should accept src == dst */
    lstrcpyW(buf, lower_case);
    ret = LCMapStringW(LOCALE_USER_DEFAULT, LCMAP_UPPERCASE,
                       buf, -1, buf, sizeof(buf)/sizeof(WCHAR));
    ok(ret == lstrlenW(lower_case) + 1,
       "ret %d, error %ld, expected value %d\n",
       ret, GetLastError(), lstrlenW(lower_case) + 1);
    ok(!lstrcmpW(buf, upper_case), "string compare mismatch\n");

    lstrcpyW(buf, upper_case);
    ret = LCMapStringW(LOCALE_USER_DEFAULT, LCMAP_LOWERCASE,
                       buf, -1, buf, sizeof(buf)/sizeof(WCHAR));
    ok(ret == lstrlenW(upper_case) + 1,
       "ret %d, error %ld, expected value %d\n",
       ret, GetLastError(), lstrlenW(lower_case) + 1);
    ok(!lstrcmpW(buf, lower_case), "string compare mismatch\n");

    /* otherwise src == dst should fail */
    SetLastError(0xdeadbeef);
    ret = LCMapStringW(LOCALE_USER_DEFAULT, LCMAP_SORTKEY | LCMAP_UPPERCASE,
                       buf, 10, buf, sizeof(buf));
    ok(GetLastError() == ERROR_INVALID_FLAGS /* NT */ ||
       GetLastError() == ERROR_INVALID_PARAMETER /* Win9x */,
       "unexpected error code %ld\n", GetLastError());
    ok(!ret, "src == dst without LCMAP_UPPERCASE or LCMAP_LOWERCASE must fail\n");

    /* test whether '\0' is always appended */
    ret = LCMapStringW(LOCALE_USER_DEFAULT, LCMAP_SORTKEY,
                       upper_case, -1, buf, sizeof(buf));
    ok(ret, "LCMapStringW must succeed\n");
    ret2 = LCMapStringW(LOCALE_USER_DEFAULT, LCMAP_SORTKEY,
                       upper_case, lstrlenW(upper_case), buf2, sizeof(buf2));
    ok(ret, "LCMapStringW must succeed\n");
    ok(ret == ret2, "lengths of sort keys must be equal\n");
    ok(!lstrcmpA(p_buf, p_buf2), "sort keys must be equal\n");

    /* test LCMAP_SORTKEY | NORM_IGNORECASE */
    ret = LCMapStringW(LOCALE_USER_DEFAULT, LCMAP_SORTKEY | NORM_IGNORECASE,
                       upper_case, -1, buf, sizeof(buf));
    ok(ret, "LCMapStringW must succeed\n");
    ret2 = LCMapStringW(LOCALE_USER_DEFAULT, LCMAP_SORTKEY,
                       lower_case, -1, buf2, sizeof(buf2));
    ok(ret2, "LCMapStringW must succeed\n");
    ok(ret == ret2, "lengths of sort keys must be equal\n");
    ok(!lstrcmpA(p_buf, p_buf2), "sort keys must be equal\n");

    /* test LCMAP_SORTKEY | NORM_IGNORENONSPACE */
    ret = LCMapStringW(LOCALE_USER_DEFAULT, LCMAP_SORTKEY | NORM_IGNORENONSPACE,
                       lower_case, -1, buf, sizeof(buf));
    ok(ret, "LCMapStringW must succeed\n");
    ret2 = LCMapStringW(LOCALE_USER_DEFAULT, LCMAP_SORTKEY,
                       lower_case, -1, buf2, sizeof(buf2));
    ok(ret2, "LCMapStringW must succeed\n");
    ok(ret == ret2, "lengths of sort keys must be equal\n");
    ok(!lstrcmpA(p_buf, p_buf2), "sort keys must be equal\n");

    /* test LCMAP_SORTKEY | NORM_IGNORESYMBOLS */
    ret = LCMapStringW(LOCALE_USER_DEFAULT, LCMAP_SORTKEY | NORM_IGNORESYMBOLS,
                       lower_case, -1, buf, sizeof(buf));
    ok(ret, "LCMapStringW must succeed\n");
    ret2 = LCMapStringW(LOCALE_USER_DEFAULT, LCMAP_SORTKEY,
                       symbols_stripped, -1, buf2, sizeof(buf2));
    ok(ret2, "LCMapStringW must succeed\n");
    ok(ret == ret2, "lengths of sort keys must be equal\n");
    ok(!lstrcmpA(p_buf, p_buf2), "sort keys must be equal\n");

    /* test NORM_IGNORENONSPACE */
    lstrcpyW(buf, fooW);
    ret = LCMapStringW(LOCALE_USER_DEFAULT, NORM_IGNORENONSPACE,
                       lower_case, -1, buf, sizeof(buf)/sizeof(WCHAR));
    ok(ret == lstrlenW(lower_case) + 1, "LCMapStringW should return %d, ret = %d\n",
	lstrlenW(lower_case) + 1, ret);
    ok(!lstrcmpW(buf, lower_case), "string comparison mismatch\n");

    /* test NORM_IGNORESYMBOLS */
    lstrcpyW(buf, fooW);
    ret = LCMapStringW(LOCALE_USER_DEFAULT, NORM_IGNORESYMBOLS,
                       lower_case, -1, buf, sizeof(buf)/sizeof(WCHAR));
    ok(ret == lstrlenW(symbols_stripped) + 1, "LCMapStringW should return %d, ret = %d\n",
	lstrlenW(symbols_stripped) + 1, ret);
    ok(!lstrcmpW(buf, symbols_stripped), "string comparison mismatch\n");
}

#define LCID_OK(l) \
  ok(lcid == l, "Expected lcid = %08lx, got %08lx\n", l, lcid)
#define MKLCID(x,y,z) MAKELCID(MAKELANGID(x, y), z)
#define LCID_RES(src, res) lcid = ConvertDefaultLocale(src); LCID_OK(res)
#define TEST_LCIDLANG(a,b) LCID_RES(MAKELCID(a,b), MAKELCID(a,b))
#define TEST_LCID(a,b,c) LCID_RES(MKLCID(a,b,c), MKLCID(a,b,c))

static void test_ConvertDefaultLocale(void)
{
  LCID lcid;

  /* Doesn't change lcid, even if non default sublang/sort used */
  TEST_LCID(LANG_ENGLISH,  SUBLANG_ENGLISH_US, SORT_DEFAULT);
  TEST_LCID(LANG_ENGLISH,  SUBLANG_ENGLISH_UK, SORT_DEFAULT);
  TEST_LCID(LANG_JAPANESE, SUBLANG_DEFAULT,    SORT_DEFAULT);
  TEST_LCID(LANG_JAPANESE, SUBLANG_DEFAULT,    SORT_JAPANESE_UNICODE);

  /* SUBLANG_NEUTRAL -> SUBLANG_DEFAULT */
  LCID_RES(MKLCID(LANG_ENGLISH,  SUBLANG_NEUTRAL, SORT_DEFAULT),
           MKLCID(LANG_ENGLISH,  SUBLANG_DEFAULT, SORT_DEFAULT));
  LCID_RES(MKLCID(LANG_JAPANESE, SUBLANG_NEUTRAL, SORT_DEFAULT),
           MKLCID(LANG_JAPANESE, SUBLANG_DEFAULT, SORT_DEFAULT));
  LCID_RES(MKLCID(LANG_JAPANESE, SUBLANG_NEUTRAL, SORT_JAPANESE_UNICODE),
           MKLCID(LANG_JAPANESE, SUBLANG_DEFAULT, SORT_JAPANESE_UNICODE));

  /* Invariant language is not treated specially */
  TEST_LCID(LANG_INVARIANT, SUBLANG_DEFAULT, SORT_DEFAULT);
  LCID_RES(MKLCID(LANG_INVARIANT, SUBLANG_NEUTRAL, SORT_DEFAULT),
           MKLCID(LANG_INVARIANT, SUBLANG_DEFAULT, SORT_DEFAULT));

  /* User/system default languages alone are not mapped */
  TEST_LCIDLANG(LANG_SYSTEM_DEFAULT, SORT_JAPANESE_UNICODE);
  TEST_LCIDLANG(LANG_USER_DEFAULT,   SORT_JAPANESE_UNICODE);

  /* Default lcids */
  LCID_RES(LOCALE_SYSTEM_DEFAULT, GetSystemDefaultLCID());
  LCID_RES(LOCALE_USER_DEFAULT,   GetUserDefaultLCID());
  LCID_RES(LOCALE_NEUTRAL,        GetUserDefaultLCID());
}

static BOOL CALLBACK langgrp_procA(LGRPID lgrpid, LPSTR lpszNum, LPSTR lpszName,
                                    DWORD dwFlags, LONG_PTR lParam)
{
  trace("%08lx, %s, %s, %08lx, %08lx\n",
        lgrpid, lpszNum, lpszName, dwFlags, lParam);

  ok(IsValidLanguageGroup(lgrpid, dwFlags) == TRUE,
     "Enumerated grp %ld not valid (flags %ld)\n", lgrpid, dwFlags);

  /* If lParam is one, we are calling with flags defaulted from 0 */
  ok(!lParam || dwFlags == LGRPID_INSTALLED,
	 "Expected dwFlags == LGRPID_INSTALLED, got %ld\n", dwFlags);

  return TRUE;
}

static void test_EnumSystemLanguageGroupsA(void)
{
  if (!pEnumSystemLanguageGroupsA)
    return;

  /* No enumeration proc */
  SetLastError(0);
  pEnumSystemLanguageGroupsA(0, LGRPID_INSTALLED, 0);
  EXPECT_INVALID;

  /* Invalid flags */
  SetLastError(0);
  pEnumSystemLanguageGroupsA(langgrp_procA, LGRPID_INSTALLED|LGRPID_SUPPORTED, 0);
  EXPECT_FLAGS;

  /* No flags - defaults to LGRPID_INSTALLED */
  SetLastError(0);
  pEnumSystemLanguageGroupsA(langgrp_procA, 0, 1);
  EXPECT_VALID;

  pEnumSystemLanguageGroupsA(langgrp_procA, LGRPID_INSTALLED, 0);
  pEnumSystemLanguageGroupsA(langgrp_procA, LGRPID_SUPPORTED, 0);
}


static BOOL CALLBACK lgrplocale_procA(LGRPID lgrpid, LCID lcid, LPSTR lpszNum,
                                      LONG_PTR lParam)
{
  trace("%08lx, %08lx, %s, %08lx\n", lgrpid, lcid, lpszNum, lParam);

  ok(IsValidLanguageGroup(lgrpid, LGRPID_SUPPORTED) == TRUE,
     "Enumerated grp %ld not valid\n", lgrpid);
  ok(IsValidLocale(lcid, LCID_SUPPORTED) == TRUE,
     "Enumerated grp locale %ld not valid\n", lcid);
  return TRUE;
}

static void test_EnumLanguageGroupLocalesA(void)
{
  if (!pEnumLanguageGroupLocalesA)
   return;

  /* No enumeration proc */
  SetLastError(0);
  pEnumLanguageGroupLocalesA(0, LGRPID_WESTERN_EUROPE, 0, 0);
  EXPECT_INVALID;

  /* lgrpid too small */
  SetLastError(0);
  pEnumLanguageGroupLocalesA(lgrplocale_procA, 0, 0, 0);
  EXPECT_INVALID;

  /* lgrpid too big */
  SetLastError(0);
  pEnumLanguageGroupLocalesA(lgrplocale_procA, LGRPID_ARMENIAN + 1, 0, 0);
  EXPECT_INVALID;

  /* dwFlags is reserved */
  SetLastError(0);
  pEnumLanguageGroupLocalesA(0, LGRPID_WESTERN_EUROPE, 0x1, 0);
  EXPECT_INVALID;

  pEnumLanguageGroupLocalesA(lgrplocale_procA, LGRPID_WESTERN_EUROPE, 0, 0);
}

static void test_SetLocaleInfoA(void)
{
  BOOL bRet;
  LCID lcid = GetUserDefaultLCID();

  /* Null data */
  SetLastError(0);
  bRet = SetLocaleInfoA(lcid, LOCALE_SDATE, 0);
  EXPECT_INVALID;

  /* IDATE */
  SetLastError(0);
  bRet = SetLocaleInfoA(lcid, LOCALE_IDATE, (LPSTR)test_SetLocaleInfoA);
  EXPECT_FLAGS;

  /* ILDATE */
  SetLastError(0);
  bRet = SetLocaleInfoA(lcid, LOCALE_ILDATE, (LPSTR)test_SetLocaleInfoA);
  EXPECT_FLAGS;
}

START_TEST(locale)
{
  InitFunctionPointers();

#if 0
  test_EnumTimeFormats();
#endif
  test_GetLocaleInfoA();
  test_GetTimeFormatA();
  test_GetDateFormatA();
  test_GetDateFormatW();
  test_GetCurrencyFormatA(); /* Also tests the W version */
  test_GetNumberFormatA();   /* Also tests the W version */
  test_CompareStringA();
  test_LCMapStringA();
  test_LCMapStringW();
  test_ConvertDefaultLocale();
  test_EnumSystemLanguageGroupsA();
  test_EnumLanguageGroupLocalesA();
  test_SetLocaleInfoA();
}
