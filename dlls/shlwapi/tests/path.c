/* Unit test suite for Path functions
 *
 * Copyright 2002 Matthew Mastracci
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

#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "wine/unicode.h"
#include "winreg.h"
#include "shlwapi.h"
#include "wininet.h"

const char* TEST_URL_1 = "http://www.winehq.org/tests?date=10/10/1923";
const char* TEST_URL_2 = "http://localhost:8080/tests%2e.html?date=Mon%2010/10/1923";
const char* TEST_URL_3 = "http://foo:bar@localhost:21/internal.php?query=x&return=y";

static LPWSTR GetWideString(const char* szString)
{
  LPWSTR wszString = (LPWSTR) HeapAlloc(GetProcessHeap(), 0,
					 (2*INTERNET_MAX_URL_LENGTH) * sizeof(WCHAR));
  
  MultiByteToWideChar(0, 0, szString, -1, wszString, INTERNET_MAX_URL_LENGTH);

  return wszString;
}

static void FreeWideString(LPWSTR wszString)
{
   HeapFree(GetProcessHeap(), 0, wszString);
}

static void hash_url(const char* szUrl)
{
  LPCSTR szTestUrl = szUrl;
  LPWSTR wszTestUrl = GetWideString(szTestUrl);
  
  DWORD cbSize = sizeof(DWORD);
  DWORD dwHash1, dwHash2;
  ok(UrlHashA(szTestUrl, (LPBYTE)&dwHash1, cbSize) == S_OK, "UrlHashA didn't return S_OK");
  ok(UrlHashW(wszTestUrl, (LPBYTE)&dwHash2, cbSize) == S_OK, "UrlHashW didn't return S_OK");

  FreeWideString(wszTestUrl);

  ok(dwHash1 == dwHash2, "Hashes didn't compare");
}

static void test_UrlHash(void)
{
  hash_url(TEST_URL_1);
  hash_url(TEST_URL_2);
  hash_url(TEST_URL_3);
}

static void test_url_part(const char* szUrl, DWORD dwPart, DWORD dwFlags, const char* szExpected)
{
  CHAR szPart[INTERNET_MAX_URL_LENGTH];
  WCHAR wszPart[INTERNET_MAX_URL_LENGTH];
  LPWSTR wszUrl = GetWideString(szUrl);
  LPWSTR wszConvertedPart;

  DWORD dwSize;

  dwSize = INTERNET_MAX_URL_LENGTH;
  ok( UrlGetPartA(szUrl, szPart, &dwSize, dwPart, dwFlags) == S_OK, "UrlGetPartA didn't return S_OK" );
  dwSize = INTERNET_MAX_URL_LENGTH;
  ok( UrlGetPartW(wszUrl, wszPart, &dwSize, dwPart, dwFlags) == S_OK, "UrlGetPartW didn't return S_OK" );

  wszConvertedPart = GetWideString(szPart);

  ok(strcmpW(wszPart,wszConvertedPart)==0, "Strings didn't match between ascii and unicode UrlGetPart!");

  FreeWideString(wszUrl);
  FreeWideString(wszConvertedPart);

  /* Note that v6.0 and later don't return '?' with the query */
  ok(strcmp(szPart,szExpected)==0 ||
     (*szExpected=='?' && !strcmp(szPart,szExpected+1)),
	 "Expected %s, but got %s", szExpected, szPart);
}

static void test_UrlGetPart(void)
{
  test_url_part(TEST_URL_3, URL_PART_HOSTNAME, 0, "localhost");
  test_url_part(TEST_URL_3, URL_PART_PORT, 0, "21");
  test_url_part(TEST_URL_3, URL_PART_USERNAME, 0, "foo");
  test_url_part(TEST_URL_3, URL_PART_PASSWORD, 0, "bar");
  test_url_part(TEST_URL_3, URL_PART_SCHEME, 0, "http");
  test_url_part(TEST_URL_3, URL_PART_QUERY, 0, "?query=x&return=y");
}

START_TEST(path)
{
  test_UrlHash();
  test_UrlGetPart();
}
