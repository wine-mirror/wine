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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "wininet.h"
#include "winerror.h"

#include "wine/test.h"

void InternetQueryOptionA_test()
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
  ok(len == strlen(useragent)+1,"Got wrong user agent length %ld instead of %d\n",len,strlen(useragent));
  ok(retval == 0,"Got wrong return value %d\n",retval);
  todo_wine ok(err == ERROR_INSUFFICIENT_BUFFER, "Got wrong error code%ld\n",err);

  SetLastError(0xdeadbeef);
  len=strlen(useragent)+1;
  buffer=HeapAlloc(GetProcessHeap(),0,len);
  retval=InternetQueryOptionA(hinet,INTERNET_OPTION_USER_AGENT,buffer,&len);
  err=GetLastError();
  todo_wine ok(!strcmp(useragent,buffer),"Got wrong user agent string %s instead of %s\n",buffer,useragent);
  todo_wine ok(len == strlen(useragent),"Got wrong user agent length %ld instead of %d\n",len,strlen(useragent));
  todo_wine ok(retval == 1,"Got wrong return value %d\n",retval);
  ok(err == 0xdeadbeef, "Got wrong error code %ld\n",err);
  HeapFree(GetProcessHeap(),0,buffer);

  SetLastError(0xdeadbeef);
  len=0;
  buffer=HeapAlloc(GetProcessHeap(),0,100);
  retval=InternetQueryOptionA(hinet,INTERNET_OPTION_USER_AGENT,buffer,&len);
  err=GetLastError();
  todo_wine ok(!strcmp(useragent,buffer),"Got wrong user agent string %s instead of %s\n",buffer,useragent);
  todo_wine ok(len == strlen(useragent),"Got wrong user agent length %ld instead of %d\n",len,strlen(useragent));
  todo_wine ok(retval == 1,"Got wrong return value %d\n",retval);
  ok(err == 0xdeadbeef, "Got wrong error code %ld\n",err);
  HeapFree(GetProcessHeap(),0,buffer);

  hurl = InternetConnectA(hinet,"www.winehq.com",INTERNET_DEFAULT_HTTP_PORT,NULL,NULL,INTERNET_SERVICE_HTTP,0,0);

  SetLastError(0xdeadbeef);
  len=0;
  retval = InternetQueryOptionA(hurl,INTERNET_OPTION_USER_AGENT,NULL,&len);
  err=GetLastError();
  ok(len == 0,"Got wrong user agent length %ld instead of 0\n",len);
  ok(retval == 0,"Got wrong return value %d\n",retval);
  todo_wine ok(err == ERROR_INTERNET_INCORRECT_HANDLE_TYPE, "Got wrong error code %ld\n",err);

  InternetCloseHandle(hurl);
  InternetCloseHandle(hinet);

  hinet = InternetOpenA("",INTERNET_OPEN_TYPE_DIRECT,NULL,NULL, 0);
  ok((hinet != 0x0),"InternetOpen Failed\n");

  SetLastError(0xdeadbeef);
  len=0;
  retval=InternetQueryOptionA(hinet,INTERNET_OPTION_USER_AGENT,NULL,&len);
  err=GetLastError();
  todo_wine ok(len == 1,"Got wrong user agent length %ld instead of %d\n",len,1);
  ok(retval == 0,"Got wrong return value %d\n",retval);
  todo_wine ok(err == ERROR_INSUFFICIENT_BUFFER, "Got wrong error code%ld\n",err);

  InternetCloseHandle(hinet);

  hinet = InternetOpenA(NULL,INTERNET_OPEN_TYPE_DIRECT,NULL,NULL, 0);
  ok((hinet != 0x0),"InternetOpen Failed\n");
  SetLastError(0xdeadbeef);
  len=0;
  retval=InternetQueryOptionA(hinet,INTERNET_OPTION_USER_AGENT,NULL,&len);
  err=GetLastError();
  todo_wine ok(len == 1,"Got wrong user agent length %ld instead of %d\n",len,1);
  ok(retval == 0,"Got wrong return value %d\n",retval);
  todo_wine ok(err == ERROR_INSUFFICIENT_BUFFER, "Got wrong error code%ld\n",err);

  InternetCloseHandle(hinet);
}

START_TEST(internet)
{
  InternetQueryOptionA_test();
}
