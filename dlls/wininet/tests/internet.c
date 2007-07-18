/*
 * Wininet - internet tests
 *
 * Copyright 2005 Vijay Kiran Kamuju
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "wininet.h"
#include "winerror.h"

#include "wine/test.h"

static void InternetQueryOptionA_test(void)
{
  HINTERNET hinet,hurl;
  DWORD len;
  DWORD err;
  static const char useragent[] = {"Wininet Test"};
  char *buffer;
  int retval;

  hinet = InternetOpenA(useragent,INTERNET_OPEN_TYPE_DIRECT,NULL,NULL, 0);
  ok((hinet != 0x0),"InternetOpen Failed\n");

  SetLastError(0xdeadbeef);
  len=strlen(useragent)+1;
  retval=InternetQueryOptionA(hinet,INTERNET_OPTION_USER_AGENT,NULL,&len);
  err=GetLastError();
  ok(len == strlen(useragent)+1,"Got wrong user agent length %d instead of %d\n",len,lstrlenA(useragent));
  ok(retval == 0,"Got wrong return value %d\n",retval);
  todo_wine ok(err == ERROR_INSUFFICIENT_BUFFER, "Got wrong error code%d\n",err);

  SetLastError(0xdeadbeef);
  len=strlen(useragent)+1;
  buffer=HeapAlloc(GetProcessHeap(),0,len);
  retval=InternetQueryOptionA(hinet,INTERNET_OPTION_USER_AGENT,buffer,&len);
  err=GetLastError();
  todo_wine ok(!strcmp(useragent,buffer),"Got wrong user agent string %s instead of %s\n",buffer,useragent);
  todo_wine ok(len == strlen(useragent),"Got wrong user agent length %d instead of %d\n",len,lstrlenA(useragent));
  todo_wine ok(retval == 1,"Got wrong return value %d\n",retval);
  ok(err == 0xdeadbeef, "Got wrong error code %d\n",err);
  HeapFree(GetProcessHeap(),0,buffer);

  SetLastError(0xdeadbeef);
  len=0;
  buffer=HeapAlloc(GetProcessHeap(),0,100);
  retval=InternetQueryOptionA(hinet,INTERNET_OPTION_USER_AGENT,buffer,&len);
  err=GetLastError();
  todo_wine ok(len == strlen(useragent) + 1,"Got wrong user agent length %d instead of %d\n", len, lstrlenA(useragent) + 1);
  ok(!retval, "Got wrong return value %d\n", retval);
  todo_wine ok(err == ERROR_INSUFFICIENT_BUFFER, "Got wrong error code %d\n", err);
  HeapFree(GetProcessHeap(),0,buffer);

  hurl = InternetConnectA(hinet,"www.winehq.com",INTERNET_DEFAULT_HTTP_PORT,NULL,NULL,INTERNET_SERVICE_HTTP,0,0);

  SetLastError(0xdeadbeef);
  len=0;
  retval = InternetQueryOptionA(hurl,INTERNET_OPTION_USER_AGENT,NULL,&len);
  err=GetLastError();
  ok(len == 0,"Got wrong user agent length %d instead of 0\n",len);
  ok(retval == 0,"Got wrong return value %d\n",retval);
  todo_wine ok(err == ERROR_INTERNET_INCORRECT_HANDLE_TYPE, "Got wrong error code %d\n",err);

  InternetCloseHandle(hurl);
  InternetCloseHandle(hinet);

  hinet = InternetOpenA("",INTERNET_OPEN_TYPE_DIRECT,NULL,NULL, 0);
  ok((hinet != 0x0),"InternetOpen Failed\n");

  SetLastError(0xdeadbeef);
  len=0;
  retval=InternetQueryOptionA(hinet,INTERNET_OPTION_USER_AGENT,NULL,&len);
  err=GetLastError();
  todo_wine ok(len == 1,"Got wrong user agent length %d instead of %d\n",len,1);
  ok(retval == 0,"Got wrong return value %d\n",retval);
  todo_wine ok(err == ERROR_INSUFFICIENT_BUFFER, "Got wrong error code%d\n",err);

  InternetCloseHandle(hinet);

  hinet = InternetOpenA(NULL,INTERNET_OPEN_TYPE_DIRECT,NULL,NULL, 0);
  ok((hinet != 0x0),"InternetOpen Failed\n");
  SetLastError(0xdeadbeef);
  len=0;
  retval=InternetQueryOptionA(hinet,INTERNET_OPTION_USER_AGENT,NULL,&len);
  err=GetLastError();
  todo_wine ok(len == 1,"Got wrong user agent length %d instead of %d\n",len,1);
  ok(retval == 0,"Got wrong return value %d\n",retval);
  todo_wine ok(err == ERROR_INSUFFICIENT_BUFFER, "Got wrong error code%d\n",err);

  InternetCloseHandle(hinet);
}

static void test_get_cookie(void)
{
  DWORD len;
  BOOL ret;

  SetLastError(0xdeadbeef);
  ret = InternetGetCookie("http://www.example.com", NULL, NULL, &len);
  ok(!ret && GetLastError() == ERROR_NO_MORE_ITEMS,
    "InternetGetCookie should have failed with %s and error %d\n",
    ret ? "TRUE" : "FALSE", GetLastError());
}

static void test_null(void)
{
  HINTERNET hi, hc;
  static const WCHAR szServer[] = { 's','e','r','v','e','r',0 };
  static const WCHAR szEmpty[] = { 0 };
  static const WCHAR szUrl[] = { 'h','t','t','p',':','/','/','a','.','b','.','c',0 };
  static const WCHAR szExpect[] = { 's','e','r','v','e','r',';',' ','s','e','r','v','e','r',0 };
  WCHAR buffer[0x20];
  BOOL r;
  DWORD sz;

  hi = InternetOpenW(NULL, 0, NULL, NULL, 0);
  ok(hi != NULL, "open failed\n");

  hc = InternetConnectW(hi, NULL, 0, NULL, NULL, 0, 0, 0);
  ok(GetLastError() == ERROR_INVALID_PARAMETER, "wrong error\n");
  ok(hc == NULL, "connect failed\n");

  hc = InternetConnectW(hi, NULL, 0, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
  ok(GetLastError() == ERROR_INVALID_PARAMETER, "wrong error\n");
  ok(hc == NULL, "connect failed\n");

  hc = InternetConnectW(hi, NULL, 0, NULL, NULL, INTERNET_SERVICE_FTP, 0, 0);
  ok(GetLastError() == ERROR_INVALID_PARAMETER, "wrong error\n");
  ok(hc == NULL, "connect failed\n");

  hc = InternetConnectW(NULL, szServer, 0, NULL, NULL, INTERNET_SERVICE_FTP, 0, 0);
  ok(GetLastError() == ERROR_INVALID_HANDLE, "wrong error\n");
  ok(hc == NULL, "connect failed\n");

  hc = InternetOpenUrlW(hi, NULL, NULL, 0, 0, 0);
  ok(GetLastError() == ERROR_INVALID_PARAMETER, "wrong error\n");
  ok(hc == NULL, "connect failed\n");

  hc = InternetOpenUrlW(hi, szServer, NULL, 0, 0, 0);
  ok(GetLastError() == ERROR_INTERNET_UNRECOGNIZED_SCHEME, "wrong error\n");
  ok(hc == NULL, "connect failed\n");

  InternetCloseHandle(hi);

  r = InternetSetCookieW(NULL, NULL, NULL);
  ok(GetLastError() == ERROR_INVALID_PARAMETER, "wrong error\n");
  ok(r == FALSE, "return wrong\n");

  r = InternetSetCookieW(szServer, NULL, NULL);
  ok(GetLastError() == ERROR_INVALID_PARAMETER, "wrong error\n");
  ok(r == FALSE, "return wrong\n");

  r = InternetSetCookieW(szUrl, szServer, NULL);
  ok(GetLastError() == ERROR_INVALID_PARAMETER, "wrong error\n");
  ok(r == FALSE, "return wrong\n");

  r = InternetSetCookieW(szUrl, szServer, szServer);
  ok(r == TRUE, "return wrong\n");

  todo_wine {
  r = InternetSetCookieW(szUrl, NULL, szServer);
  ok(r == TRUE, "return wrong\n");
  }

  r = InternetSetCookieW(szUrl, szServer, szEmpty);
  ok(r == TRUE, "return wrong\n");

  r = InternetSetCookieW(szServer, NULL, szServer);
  todo_wine {
  ok(GetLastError() == ERROR_INTERNET_UNRECOGNIZED_SCHEME, "wrong error\n");
  }
  ok(r == FALSE, "return wrong\n");

  sz = 0;
  r = InternetGetCookieW(NULL, NULL, NULL, &sz);
  ok(GetLastError() == ERROR_INTERNET_UNRECOGNIZED_SCHEME, "wrong error\n");
  ok( r == FALSE, "return wrong\n");

  r = InternetGetCookieW(szServer, NULL, NULL, &sz);
  todo_wine {
  ok(GetLastError() == ERROR_INTERNET_UNRECOGNIZED_SCHEME, "wrong error\n");
  }
  ok( r == FALSE, "return wrong\n");

  sz = 0;
  r = InternetGetCookieW(szUrl, szServer, NULL, &sz);
  ok( r == TRUE, "return wrong\n");
  todo_wine {
  ok( sz == 30, "sz wrong\n");
  }

  sz = 0x20;
  memset(buffer, 0, sizeof buffer);
  r = InternetGetCookieW(szUrl, szServer, buffer, &sz);
  ok( r == TRUE, "return wrong\n");
  todo_wine {
  ok( sz == lstrlenW(buffer), "sz wrong\n");
  ok( !lstrcmpW(szExpect, buffer), "cookie data wrong\n");
  }

  sz = sizeof(buffer);
  r = InternetQueryOptionA(NULL, INTERNET_OPTION_CONNECTED_STATE, buffer, &sz);
  ok(r == TRUE, "ret %d\n", r);

}

START_TEST(internet)
{
  InternetQueryOptionA_test();
  test_get_cookie();
  test_null();
}
