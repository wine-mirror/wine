/* Unit test suite for SHLWAPI string functions
 *
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
 */

#include <stdlib.h>
#include <stdio.h>

#include "wine/test.h"
#include "winbase.h"
#include "winerror.h"
#include "winnls.h"
#define NO_SHLWAPI_REG
#define NO_SHLWAPI_PATH
#define NO_SHLWAPI_GDI
#define NO_SHLWAPI_STREAM
#include "shlwapi.h"

/* StrToInt/StrToIntEx results */
typedef struct tagStrToIntResult
{
  const char* string;
  int str_to_int;
  int str_to_int_ex;
  int str_to_int_hex;
} StrToIntResult;

static const StrToIntResult StrToInt_results[] = {
     { "1099", 1099, 1099, 1099 },
     { "+88987", 0, 88987, 88987 },
     { "012", 12, 12, 12 },
     { "-55", -55, -55, -55 },
     { "-0", 0, 0, 0 },
     { "0x44ff", 0, 0, 0x44ff },
     { "+0x44f4", 0, 0, 0x44f4 },
     { "-0x44fd", 0, 0, 0x44fd },
     { "+ 88987", 0, 0, 0 },
     { "- 55", 0, 0, 0 },
     { "- 0", 0, 0, 0 },
     { "+ 0x44f4", 0, 0, 0 },
     { "--0x44fd", 0, 0, 0 },
     { " 1999", 0, 1999, 1999 },
     { " +88987", 0, 88987, 88987 },
     { " 012", 0, 12, 12 },
     { " -55", 0, -55, -55 },
     { " 0x44ff", 0, 0, 0x44ff },
     { " +0x44f4", 0, 0, 0x44f4 },
     { " -0x44fd", 0, 0, 0x44fd },
     { NULL, 0, 0, 0 }
};

/* pStrFormatByteSize64/StrFormatKBSize results */
typedef struct tagStrFormatSizeResult
{
  LONGLONG value;
  const char* byte_size_64;
  const char* kb_size;
} StrFormatSizeResult;


static const StrFormatSizeResult StrFormatSize_results[] = {
  { -1023, "-1023 bytes", "0 KB"},
  { -24, "-24 bytes", "0 KB"},
  { 309, "309 bytes", "1 KB"},
  { 10191, "9.95 KB", "10 KB"},
  { 100353, "98.0 KB", "99 KB"},
  { 1022286, "998 KB", "999 KB"},
  { 1046862, "0.99 MB", "1,023 KB"},
  { 1048574619, "999 MB", "1,023,999 KB"},
  { 1073741775, "0.99 GB", "1,048,576 KB"},
  { ((LONGLONG)0x000000f9 << 32) | 0xfffff94e, "999 GB", "1,048,575,999 KB"},
  { ((LONGLONG)0x000000ff << 32) | 0xfffffa9b, "0.99 TB", "1,073,741,823 KB"},
  { ((LONGLONG)0x0003e7ff << 32) | 0xfffffa9b, "999 TB", "1,073,741,823,999 KB"},
  { ((LONGLONG)0x0003ffff << 32) | 0xfffffbe8, "0.99 PB", "1,099,511,627,775 KB"},
  { ((LONGLONG)0x0f9fffff << 32) | 0xfffffd35, "999 PB", "1,099,511,627,776,000 KB"},
  { ((LONGLONG)0x0fffffff << 32) | 0xfffffa9b, "0.99 EB", "1,125,899,906,842,623 KB"},
  { 0, NULL, NULL }
};

/* StrFormatByteSize64/StrFormatKBSize results */
typedef struct tagStrFromTimeIntervalResult
{
  DWORD ms;
  int   digits;
  const char* time_interval;
} StrFromTimeIntervalResult;


static const StrFromTimeIntervalResult StrFromTimeInterval_results[] = {
  { 1, 1, " 0 sec" },
  { 1, 2, " 0 sec" },
  { 1, 3, " 0 sec" },
  { 1, 4, " 0 sec" },
  { 1, 5, " 0 sec" },
  { 1, 6, " 0 sec" },
  { 1, 7, " 0 sec" },

  { 1000000, 1, " 10 min" },
  { 1000000, 2, " 16 min" },
  { 1000000, 3, " 16 min 40 sec" },
  { 1000000, 4, " 16 min 40 sec" },
  { 1000000, 5, " 16 min 40 sec" },
  { 1000000, 6, " 16 min 40 sec" },
  { 1000000, 7, " 16 min 40 sec" },

  { 1999999, 1, " 30 min" },
  { 1999999, 2, " 33 min" },
  { 1999999, 3, " 33 min 20 sec" },
  { 1999999, 4, " 33 min 20 sec" },
  { 1999999, 5, " 33 min 20 sec" },
  { 1999999, 6, " 33 min 20 sec" },
  { 1999999, 7, " 33 min 20 sec" },

  { 3999997, 1, " 1 hr" },
  { 3999997, 2, " 1 hr 6 min" },
  { 3999997, 3, " 1 hr 6 min 40 sec" },
  { 3999997, 4, " 1 hr 6 min 40 sec" },
  { 3999997, 5, " 1 hr 6 min 40 sec" },
  { 3999997, 6, " 1 hr 6 min 40 sec" },
  { 3999997, 7, " 1 hr 6 min 40 sec" },

  { 149999851, 7, " 41 hr 40 min 0 sec" },
  { 150999850, 1, " 40 hr" },
  { 150999850, 2, " 41 hr" },
  { 150999850, 3, " 41 hr 50 min" },
  { 150999850, 4, " 41 hr 56 min" },
  { 150999850, 5, " 41 hr 56 min 40 sec" },
  { 150999850, 6, " 41 hr 56 min 40 sec" },
  { 150999850, 7, " 41 hr 56 min 40 sec" },

  { 493999507, 1, " 100 hr" },
  { 493999507, 2, " 130 hr" },
  { 493999507, 3, " 137 hr" },
  { 493999507, 4, " 137 hr 10 min" },
  { 493999507, 5, " 137 hr 13 min" },
  { 493999507, 6, " 137 hr 13 min 20 sec" },
  { 493999507, 7, " 137 hr 13 min 20 sec" },

  { 0, 0, NULL }
};

static void test_StrChrA(void)
{
  char string[129];
  int count;

  ok(!StrChrA(NULL,'\0'), "found a character in a NULL string!");

  for (count = 32; count < 128; count++)
    string[count] = count;
  string[128] = '\0';

  for (count = 32; count < 128; count++)
  {
    LPSTR result = StrChrA(string+32, count);
    ok(result - string == count, "found char %d in wrong place", count);
  }

  for (count = 32; count < 128; count++)
  {
    LPSTR result = StrChrA(string+count+1, count);
    ok(!result, "found char not in the string");
  }
}

static void test_StrChrW(void)
{
  WCHAR string[16385];
  int count;

  ok(!StrChrW(NULL,'\0'), "found a character in a NULL string!");

  for (count = 32; count < 16384; count++)
    string[count] = count;
  string[16384] = '\0';

  for (count = 32; count < 16384; count++)
  {
    LPWSTR result = StrChrW(string+32, count);
    ok((result - string) == count, "found char %d in wrong place", count);
  }

  for (count = 32; count < 16384; count++)
  {
    LPWSTR result = StrChrW(string+count+1, count);
    ok(!result, "found char not in the string");
  }
}

static void test_StrChrIA(void)
{
  char string[129];
  int count;

  ok(!StrChrIA(NULL,'\0'), "found a character in a NULL string!");

  for (count = 32; count < 128; count++)
    string[count] = count;
  string[128] = '\0';

  for (count = 'A'; count <= 'X'; count++)
  {
    LPSTR result = StrChrIA(string+32, count);

    ok(result - string == count, "found char '%c' in wrong place", count);
    ok(StrChrIA(result, count)!=NULL, "didn't find lowercase '%c'", count);
  }

  for (count = 'a'; count < 'z'; count++)
  {
    LPSTR result = StrChrIA(string+count+1, count);
    ok(!result, "found char not in the string");
  }
}

static void test_StrChrIW(void)
{
  WCHAR string[129];
  int count;

  ok(!StrChrIA(NULL,'\0'), "found a character in a NULL string!");

  for (count = 32; count < 128; count++)
    string[count] = count;
  string[128] = '\0';

  for (count = 'A'; count <= 'X'; count++)
  {
    LPWSTR result = StrChrIW(string+32, count);

    ok(result - string == count, "found char '%c' in wrong place", count);
    ok(StrChrIW(result, count)!=NULL, "didn't find lowercase '%c'", count);
  }

  for (count = 'a'; count < 'z'; count++)
  {
    LPWSTR result = StrChrIW(string+count+1, count);
    ok(!result, "found char not in the string");
  }
}

static void test_StrRChrA(void)
{
  char string[129];
  int count;

  ok(!StrRChrA(NULL, NULL,'\0'), "found a character in a NULL string!");

  for (count = 32; count < 128; count++)
    string[count] = count;
  string[128] = '\0';

  for (count = 32; count < 128; count++)
  {
    LPSTR result = StrRChrA(string+32, NULL, count);
    ok(result - string == count, "found char %d in wrong place", count);
  }

  for (count = 32; count < 128; count++)
  {
    LPSTR result = StrRChrA(string+count+1, NULL, count);
    ok(!result, "found char not in the string");
  }

  for (count = 32; count < 128; count++)
  {
    LPSTR result = StrRChrA(string+count+1, string + 127, count);
    ok(!result, "found char not in the string");
  }
}

static void test_StrRChrW(void)
{
  WCHAR string[16385];
  int count;

  ok(!StrRChrW(NULL, NULL,'\0'), "found a character in a NULL string!");

  for (count = 32; count < 16384; count++)
    string[count] = count;
  string[16384] = '\0';

  for (count = 32; count < 16384; count++)
  {
    LPWSTR result = StrRChrW(string+32, NULL, count);
    ok(result - string == count, "found char %d in wrong place", count);
  }

  for (count = 32; count < 16384; count++)
  {
    LPWSTR result = StrRChrW(string+count+1, NULL, count);
    ok(!result, "found char not in the string");
  }

  for (count = 32; count < 16384; count++)
  {
    LPWSTR result = StrRChrW(string+count+1, string + 127, count);
    ok(!result, "found char not in the string");
  }
}

static void test_StrCpyW(void)
{
  WCHAR szSrc[256];
  WCHAR szBuff[256];
  const StrFormatSizeResult* result = StrFormatSize_results;


  while(result->value)
  {
    MultiByteToWideChar(0,0,result->byte_size_64,-1,szSrc,sizeof(szSrc)/sizeof(WCHAR));

    StrCpyW(szBuff, szSrc);
    ok(!StrCmpW(szSrc, szBuff), "Copied string %s wrong", result->byte_size_64);
    result++;
  }
}


static void test_StrToIntA(void)
{
  const StrToIntResult *result = StrToInt_results;
  int return_val;

  while (result->string)
  {
    return_val = StrToIntA(result->string);
    ok(return_val == result->str_to_int, "converted '%s' wrong (%d)",
       result->string, return_val);
    result++;
  }
}

static void test_StrToIntW(void)
{
  WCHAR szBuff[256];
  const StrToIntResult *result = StrToInt_results;
  int return_val;

  while (result->string)
  {
    MultiByteToWideChar(0,0,result->string,-1,szBuff,sizeof(szBuff)/sizeof(WCHAR));
    return_val = StrToIntW(szBuff);
    ok(return_val == result->str_to_int, "converted '%s' wrong (%d)",
       result->string, return_val);
    result++;
  }
}

static void test_StrToIntExA(void)
{
  const StrToIntResult *result = StrToInt_results;
  int return_val;
  BOOL bRet;

  while (result->string)
  {
    return_val = -1;
    bRet = StrToIntExA(result->string,0,&return_val);
    ok(!bRet || return_val != -1, "No result returned from '%s'",
       result->string);
    if (bRet)
      ok(return_val == result->str_to_int_ex, "converted '%s' wrong (%d)",
         result->string, return_val);
    result++;
  }

  result = StrToInt_results;
  while (result->string)
  {
    return_val = -1;
    bRet = StrToIntExA(result->string,STIF_SUPPORT_HEX,&return_val);
    ok(!bRet || return_val != -1, "No result returned from '%s'",
       result->string);
    if (bRet)
      ok(return_val == result->str_to_int_hex, "converted '%s' wrong (%d)",
         result->string, return_val);
    result++;
  }
}

static void test_StrToIntExW(void)
{
  WCHAR szBuff[256];
  const StrToIntResult *result = StrToInt_results;
  int return_val;
  BOOL bRet;

  while (result->string)
  {
    return_val = -1;
    MultiByteToWideChar(0,0,result->string,-1,szBuff,sizeof(szBuff)/sizeof(WCHAR));
    bRet = StrToIntExW(szBuff, 0, &return_val);
    ok(!bRet || return_val != -1, "No result returned from '%s'",
       result->string);
    if (bRet)
      ok(return_val == result->str_to_int_ex, "converted '%s' wrong (%d)",
         result->string, return_val);
    result++;
  }

  result = StrToInt_results;
  while (result->string)
  {
    return_val = -1;
    MultiByteToWideChar(0,0,result->string,-1,szBuff,sizeof(szBuff)/sizeof(WCHAR));
    bRet = StrToIntExW(szBuff, STIF_SUPPORT_HEX, &return_val);
    ok(!bRet || return_val != -1, "No result returned from '%s'",
       result->string);
    if (bRet)
      ok(return_val == result->str_to_int_hex, "converted '%s' wrong (%d)",
         result->string, return_val);
    result++;
  }
}

static void test_StrDupA()
{
  LPSTR lpszStr;
  const StrFormatSizeResult* result = StrFormatSize_results;

  while(result->value)
  {
    lpszStr = StrDupA(result->byte_size_64);

    ok(lpszStr != NULL, "Dup failed");
    if (lpszStr)
    {
      ok(!strcmp(result->byte_size_64, lpszStr), "Copied string wrong");
      LocalFree((HLOCAL)lpszStr);
    }
    result++;
  }

  /* Later versions of shlwapi return NULL for this, but earlier versions
   * returned an empty string (as Wine does).
   */
  lpszStr = StrDupA(NULL);
  ok(lpszStr == NULL || *lpszStr == '\0', "NULL string returned %p", lpszStr);
}

static void test_StrFormatByteSize64A(void)
{
  char szBuff[256];
  const StrFormatSizeResult* result = StrFormatSize_results;

  while(result->value)
  {
    StrFormatByteSize64A(result->value, szBuff, 256);

    ok(!strcmp(result->byte_size_64, szBuff), "Formatted %lld wrong", result->value);

    result++;
  }
}

static void test_StrFormatKBSizeW(void)
{
/* FIXME: Awaiting NLS fixes in kernel before these succeed */
#if 0
  WCHAR szBuffW[256];
  char szBuff[256];
  const StrFormatSizeResult* result = StrFormatSize_results;

  while(result->value)
  {
    StrFormatKBSizeW(result->value, szBuffW, 256);
    WideCharToMultiByte(0,0,szBuffW,-1,szBuff,sizeof(szBuff)/sizeof(WCHAR),0,0);
    ok(!strcmp(result->kb_size, szBuff), "Formatted %lld wrong",
       result->value);
    result++;
  }
#endif
}

static void test_StrFormatKBSizeA(void)
{
#if 0
  char szBuff[256];
  const StrFormatSizeResult* result = StrFormatSize_results;

  while(result->value)
  {
    StrFormatKBSizeA(result->value, szBuff, 256);

    ok(!strcmp(result->kb_size, szBuff), "Formatted %lld wrong",
       result->value);
    result++;
  }
#endif
}

void test_StrFromTimeIntervalA(void)
{
  char szBuff[256];
  const StrFromTimeIntervalResult* result = StrFromTimeInterval_results;

  while(result->ms)
  {
    StrFromTimeIntervalA(szBuff, 256, result->ms, result->digits);

    ok(!strcmp(result->time_interval, szBuff), "Formatted %ld %d wrong",
       result->ms, result->digits);
    result++;
  }
}

START_TEST(string)
{
  test_StrChrA();
  test_StrChrW();
  test_StrChrIA();
  test_StrChrIW();
  test_StrRChrA();
  test_StrRChrW();
  test_StrCpyW();
  test_StrToIntA();
  test_StrToIntW();
  test_StrToIntExA();
  test_StrToIntExW();
  test_StrDupA();
  test_StrFormatByteSize64A();
  test_StrFormatKBSizeA();
  test_StrFormatKBSizeW();
  test_StrFromTimeIntervalA();
}
