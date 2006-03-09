/*
 * Wininet - Http tests
 *
 * Copyright 2002 Aric Stewart
 * Copyright 2004 Mike McCormack
 * Copyright 2005 Hans Leidekker
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
#include <stdio.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "wininet.h"

#include "wine/test.h"

#define TEST_URL "http://www.winehq.org/site/about"
#define TEST_URL_PATH "/site/about"
#define TEST_URL2 "http://www.myserver.com/myscript.php?arg=1"
#define TEST_URL2_SERVER "www.myserver.com"
#define TEST_URL2_PATH "/myscript.php"
#define TEST_URL2_PATHEXTRA "/myscript.php?arg=1"
#define TEST_URL2_EXTRA "?arg=1"
#define TEST_URL3 "file:///C:/Program%20Files/Atmel/AVR%20Tools/STK500/STK500.xml"

#define CREATE_URL1 "http://username:password@www.winehq.org/site/about"
#define CREATE_URL2 "http://username@www.winehq.org/site/about"
#define CREATE_URL3 "http://username:"
#define CREATE_URL4 "http://www.winehq.org/site/about"
#define CREATE_URL5 "http://"
#define CREATE_URL6 "nhttp://username:password@www.winehq.org:80/site/about"
#define CREATE_URL7 "http://username:password@www.winehq.org:42/site/about"
#define CREATE_URL8 "https://username:password@www.winehq.org/site/about"
#define CREATE_URL9 "about:blank"

static HANDLE hCompleteEvent;

static VOID WINAPI callback(
     HINTERNET hInternet,
     DWORD dwContext,
     DWORD dwInternetStatus,
     LPVOID lpvStatusInformation,
     DWORD dwStatusInformationLength
)
{
    switch (dwInternetStatus)
    {
        case INTERNET_STATUS_RESOLVING_NAME:
            trace("%04lx:Callback %p 0x%lx INTERNET_STATUS_RESOLVING_NAME \"%s\" %ld\n",
                GetCurrentThreadId(), hInternet, dwContext,
                (LPCSTR)lpvStatusInformation,dwStatusInformationLength);
            break;
        case INTERNET_STATUS_NAME_RESOLVED:
            trace("%04lx:Callback %p 0x%lx INTERNET_STATUS_NAME_RESOLVED \"%s\" %ld\n",
                GetCurrentThreadId(), hInternet, dwContext,
                (LPCSTR)lpvStatusInformation,dwStatusInformationLength);
            break;
        case INTERNET_STATUS_CONNECTING_TO_SERVER:
            trace("%04lx:Callback %p 0x%lx INTERNET_STATUS_CONNECTING_TO_SERVER \"%s\" %ld\n",
                GetCurrentThreadId(), hInternet, dwContext,
                (LPCSTR)lpvStatusInformation,dwStatusInformationLength);
            break;
        case INTERNET_STATUS_CONNECTED_TO_SERVER:
            trace("%04lx:Callback %p 0x%lx INTERNET_STATUS_CONNECTED_TO_SERVER \"%s\" %ld\n",
                GetCurrentThreadId(), hInternet, dwContext,
                (LPCSTR)lpvStatusInformation,dwStatusInformationLength);
            break;
        case INTERNET_STATUS_SENDING_REQUEST:
            trace("%04lx:Callback %p 0x%lx INTERNET_STATUS_SENDING_REQUEST %p %ld\n",
                GetCurrentThreadId(), hInternet, dwContext,
                lpvStatusInformation,dwStatusInformationLength);
            break;
        case INTERNET_STATUS_REQUEST_SENT:
            ok(dwStatusInformationLength == sizeof(DWORD),
                "info length should be sizeof(DWORD) instead of %ld\n",
                dwStatusInformationLength);
            trace("%04lx:Callback %p 0x%lx INTERNET_STATUS_REQUEST_SENT 0x%lx %ld\n",
                GetCurrentThreadId(), hInternet, dwContext,
                *(DWORD *)lpvStatusInformation,dwStatusInformationLength);
            break;
        case INTERNET_STATUS_RECEIVING_RESPONSE:
            trace("%04lx:Callback %p 0x%lx INTERNET_STATUS_RECEIVING_RESPONSE %p %ld\n",
                GetCurrentThreadId(), hInternet, dwContext,
                lpvStatusInformation,dwStatusInformationLength);
            break;
        case INTERNET_STATUS_RESPONSE_RECEIVED:
            ok(dwStatusInformationLength == sizeof(DWORD),
                "info length should be sizeof(DWORD) instead of %ld\n",
                dwStatusInformationLength);
            trace("%04lx:Callback %p 0x%lx INTERNET_STATUS_RESPONSE_RECEIVED 0x%lx %ld\n",
                GetCurrentThreadId(), hInternet, dwContext,
                *(DWORD *)lpvStatusInformation,dwStatusInformationLength);
            break;
        case INTERNET_STATUS_CTL_RESPONSE_RECEIVED:
            trace("%04lx:Callback %p 0x%lx INTERNET_STATUS_CTL_RESPONSE_RECEIVED %p %ld\n",
                GetCurrentThreadId(), hInternet,dwContext,
                lpvStatusInformation,dwStatusInformationLength);
            break;
        case INTERNET_STATUS_PREFETCH:
            trace("%04lx:Callback %p 0x%lx INTERNET_STATUS_PREFETCH %p %ld\n",
                GetCurrentThreadId(), hInternet, dwContext,
                lpvStatusInformation,dwStatusInformationLength);
            break;
        case INTERNET_STATUS_CLOSING_CONNECTION:
            trace("%04lx:Callback %p 0x%lx INTERNET_STATUS_CLOSING_CONNECTION %p %ld\n",
                GetCurrentThreadId(), hInternet, dwContext,
                lpvStatusInformation,dwStatusInformationLength);
            break;
        case INTERNET_STATUS_CONNECTION_CLOSED:
            trace("%04lx:Callback %p 0x%lx INTERNET_STATUS_CONNECTION_CLOSED %p %ld\n",
                GetCurrentThreadId(), hInternet, dwContext,
                lpvStatusInformation,dwStatusInformationLength);
            break;
        case INTERNET_STATUS_HANDLE_CREATED:
            ok(dwStatusInformationLength == sizeof(HINTERNET),
                "info length should be sizeof(HINTERNET) instead of %ld\n",
                dwStatusInformationLength);
            trace("%04lx:Callback %p 0x%lx INTERNET_STATUS_HANDLE_CREATED %p %ld\n",
                GetCurrentThreadId(), hInternet, dwContext,
                *(HINTERNET *)lpvStatusInformation,dwStatusInformationLength);
            break;
        case INTERNET_STATUS_HANDLE_CLOSING:
            ok(dwStatusInformationLength == sizeof(HINTERNET),
                "info length should be sizeof(HINTERNET) instead of %ld\n",
                dwStatusInformationLength);
            trace("%04lx:Callback %p 0x%lx INTERNET_STATUS_HANDLE_CLOSING %p %ld\n",
                GetCurrentThreadId(), hInternet, dwContext,
                *(HINTERNET *)lpvStatusInformation, dwStatusInformationLength);
            break;
        case INTERNET_STATUS_REQUEST_COMPLETE:
        {
            INTERNET_ASYNC_RESULT *iar = (INTERNET_ASYNC_RESULT *)lpvStatusInformation;
            ok(dwStatusInformationLength == sizeof(INTERNET_ASYNC_RESULT),
                "info length should be sizeof(INTERNET_ASYNC_RESULT) instead of %ld\n",
                dwStatusInformationLength);
            trace("%04lx:Callback %p 0x%lx INTERNET_STATUS_REQUEST_COMPLETE {%ld,%ld} %ld\n",
                GetCurrentThreadId(), hInternet, dwContext,
                iar->dwResult,iar->dwError,dwStatusInformationLength);
            SetEvent(hCompleteEvent);
            break;
        }
        case INTERNET_STATUS_REDIRECT:
            trace("%04lx:Callback %p 0x%lx INTERNET_STATUS_REDIRECT \"%s\" %ld\n",
                GetCurrentThreadId(), hInternet, dwContext,
                (LPCSTR)lpvStatusInformation, dwStatusInformationLength);
            break;
        case INTERNET_STATUS_INTERMEDIATE_RESPONSE:
            trace("%04lx:Callback %p 0x%lx INTERNET_STATUS_INTERMEDIATE_RESPONSE %p %ld\n",
                GetCurrentThreadId(), hInternet, dwContext,
                lpvStatusInformation, dwStatusInformationLength);
            break;
        default:
            trace("%04lx:Callback %p 0x%lx %ld %p %ld\n",
                GetCurrentThreadId(), hInternet, dwContext, dwInternetStatus,
                lpvStatusInformation, dwStatusInformationLength);
    }
}

static void InternetReadFile_test(int flags)
{
    DWORD rc;
    CHAR buffer[4000];
    DWORD length;
    DWORD out;
    const char *types[2] = { "*", NULL };
    HINTERNET hi, hic = 0, hor = 0;

    hCompleteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    trace("Starting InternetReadFile test with flags 0x%x\n",flags);

    trace("InternetOpenA <--\n");
    hi = InternetOpenA("", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, flags);
    ok((hi != 0x0),"InternetOpen failed with error %ld\n", GetLastError());
    trace("InternetOpenA -->\n");

    if (hi == 0x0) goto abort;

    InternetSetStatusCallback(hi,&callback);

    trace("InternetConnectA <--\n");
    hic=InternetConnectA(hi, "www.winehq.org", INTERNET_INVALID_PORT_NUMBER,
                         NULL, NULL, INTERNET_SERVICE_HTTP, 0x0, 0xdeadbeef);
    ok((hic != 0x0),"InternetConnect failed with error %ld\n", GetLastError());
    trace("InternetConnectA -->\n");

    if (hic == 0x0) goto abort;

    trace("HttpOpenRequestA <--\n");
    hor = HttpOpenRequestA(hic, "GET", "/about/", NULL, NULL, types,
                           INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_RESYNCHRONIZE,
                           0xdeadbead);
    if (hor == 0x0 && GetLastError() == ERROR_INTERNET_NAME_NOT_RESOLVED) {
        /*
         * If the internet name can't be resolved we are probably behind
         * a firewall or in some other way not directly connected to the
         * Internet. Not enough reason to fail the test. Just ignore and
         * abort.
         */
    } else  {
        ok((hor != 0x0),"HttpOpenRequest failed with error %ld\n", GetLastError());
    }
    trace("HttpOpenRequestA -->\n");

    if (hor == 0x0) goto abort;

    trace("HttpSendRequestA -->\n");
    SetLastError(0xdeadbeef);
    rc = HttpSendRequestA(hor, "", -1, NULL, 0);
    if (flags & INTERNET_FLAG_ASYNC)
        ok(((rc == 0)&&(GetLastError() == ERROR_IO_PENDING)),
            "Asynchronous HttpSendRequest NOT returning 0 with error ERROR_IO_PENDING\n");
    else
        ok((rc != 0) || GetLastError() == ERROR_INTERNET_NAME_NOT_RESOLVED,
           "Synchronous HttpSendRequest returning 0, error %ld\n", GetLastError());
    trace("HttpSendRequestA <--\n");

    if (flags & INTERNET_FLAG_ASYNC)
        WaitForSingleObject(hCompleteEvent, INFINITE);

    length = 4;
    rc = InternetQueryOptionA(hor,INTERNET_OPTION_REQUEST_FLAGS,&out,&length);
    trace("Option 0x17 -> %li  %li\n",rc,out);

    length = 100;
    rc = InternetQueryOptionA(hor,INTERNET_OPTION_URL,buffer,&length);
    trace("Option 0x22 -> %li  %s\n",rc,buffer);

    length = 4000;
    rc = HttpQueryInfoA(hor,HTTP_QUERY_RAW_HEADERS,buffer,&length,0x0);
    buffer[length]=0;
    trace("Option 0x16 -> %li  %s\n",rc,buffer);

    length = 4000;
    rc = InternetQueryOptionA(hor,INTERNET_OPTION_URL,buffer,&length);
    buffer[length]=0;
    trace("Option 0x22 -> %li  %s\n",rc,buffer);

    length = 16;
    rc = HttpQueryInfoA(hor,HTTP_QUERY_CONTENT_LENGTH,&buffer,&length,0x0);
    trace("Option 0x5 -> %li  %s  (%li)\n",rc,buffer,GetLastError());

    length = 100;
    rc = HttpQueryInfoA(hor,HTTP_QUERY_CONTENT_TYPE,buffer,&length,0x0);
    buffer[length]=0;
    trace("Option 0x1 -> %li  %s\n",rc,buffer);

    SetLastError(0xdeadbeef);
    rc = InternetReadFile(NULL, buffer, 100, &length);
    ok(!rc, "InternetReadFile should have failed\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE,
        "InternetReadFile should have set last error to ERROR_INVALID_HANDLE instead of %ld\n",
        GetLastError());

    length = 100;
    trace("Entering Query loop\n");

    while (length)
    {
        rc = InternetQueryDataAvailable(hor,&length,0x0,0x0);
        ok(!(rc == 0 && length != 0),"InternetQueryDataAvailable failed\n");

        if (length)
        {
            char *buffer;
            buffer = HeapAlloc(GetProcessHeap(),0,length+1);

            rc = InternetReadFile(hor,buffer,length,&length);

            buffer[length]=0;

            trace("ReadFile -> %li %li\n",rc,length);

            HeapFree(GetProcessHeap(),0,buffer);
        }
    }
abort:
    if (hor != 0x0) {
        SetLastError(0xdeadbeef);
        rc = InternetCloseHandle(hor);
        ok ((rc != 0), "InternetCloseHandle of handle opened by HttpOpenRequestA failed\n");
        SetLastError(0xdeadbeef);
        rc = InternetCloseHandle(hor);
        ok ((rc == 0), "Double close of handle opened by HttpOpenRequestA succeeded\n");
        ok (GetLastError() == ERROR_INVALID_HANDLE,
            "Double close of handle should have set ERROR_INVALID_HANDLE instead of %ld\n",
            GetLastError());
    }
    if (hic != 0x0) {
        rc = InternetCloseHandle(hic);
        ok ((rc != 0), "InternetCloseHandle of handle opened by InternetConnectA failed\n");
    }
    if (hi != 0x0) {
      rc = InternetCloseHandle(hi);
      ok ((rc != 0), "InternetCloseHandle of handle opened by InternetOpenA failed\n");
      if (flags & INTERNET_FLAG_ASYNC)
          Sleep(100);
    }
    CloseHandle(hCompleteEvent);
}

static void InternetReadFileExA_test(int flags)
{
    DWORD rc;
    DWORD length;
    const char *types[2] = { "*", NULL };
    HINTERNET hi, hic = 0, hor = 0;
    INTERNET_BUFFERS inetbuffers;

    hCompleteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    trace("Starting InternetReadFileExA test with flags 0x%x\n",flags);

    trace("InternetOpenA <--\n");
    hi = InternetOpenA("", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, flags);
    ok((hi != 0x0),"InternetOpen failed with error %ld\n", GetLastError());
    trace("InternetOpenA -->\n");

    if (hi == 0x0) goto abort;

    InternetSetStatusCallback(hi,&callback);

    trace("InternetConnectA <--\n");
    hic=InternetConnectA(hi, "www.winehq.org", INTERNET_INVALID_PORT_NUMBER,
                         NULL, NULL, INTERNET_SERVICE_HTTP, 0x0, 0xdeadbeef);
    ok((hic != 0x0),"InternetConnect failed with error %ld\n", GetLastError());
    trace("InternetConnectA -->\n");

    if (hic == 0x0) goto abort;

    trace("HttpOpenRequestA <--\n");
    hor = HttpOpenRequestA(hic, "GET", "/about/", NULL, NULL, types,
                           INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_RESYNCHRONIZE,
                           0xdeadbead);
    if (hor == 0x0 && GetLastError() == ERROR_INTERNET_NAME_NOT_RESOLVED) {
        /*
         * If the internet name can't be resolved we are probably behind
         * a firewall or in some other way not directly connected to the
         * Internet. Not enough reason to fail the test. Just ignore and
         * abort.
         */
    } else  {
        ok((hor != 0x0),"HttpOpenRequest failed with error %ld\n", GetLastError());
    }
    trace("HttpOpenRequestA -->\n");

    if (hor == 0x0) goto abort;

    trace("HttpSendRequestA -->\n");
    SetLastError(0xdeadbeef);
    rc = HttpSendRequestA(hor, "", -1, NULL, 0);
    if (flags & INTERNET_FLAG_ASYNC)
        ok(((rc == 0)&&(GetLastError() == ERROR_IO_PENDING)),
            "Asynchronous HttpSendRequest NOT returning 0 with error ERROR_IO_PENDING\n");
    else
        ok((rc != 0) || GetLastError() == ERROR_INTERNET_NAME_NOT_RESOLVED,
           "Synchronous HttpSendRequest returning 0, error %ld\n", GetLastError());
    trace("HttpSendRequestA <--\n");

    if (!rc && (GetLastError() == ERROR_IO_PENDING))
        WaitForSingleObject(hCompleteEvent, INFINITE);

    /* tests invalid dwStructSize */
    inetbuffers.dwStructSize = sizeof(INTERNET_BUFFERS)+1;
    inetbuffers.lpcszHeader = NULL;
    inetbuffers.dwHeadersLength = 0;
    inetbuffers.dwBufferLength = 10;
    inetbuffers.lpvBuffer = HeapAlloc(GetProcessHeap(), 0, 10);
    inetbuffers.dwOffsetHigh = 1234;
    inetbuffers.dwOffsetLow = 5678;
    rc = InternetReadFileEx(hor, &inetbuffers, 0, 0xdeadcafe);
    ok(!rc && (GetLastError() == ERROR_INVALID_PARAMETER),
        "InternetReadFileEx should have failed with ERROR_INVALID_PARAMETER instead of %s, %ld\n",
        rc ? "TRUE" : "FALSE", GetLastError());
    HeapFree(GetProcessHeap(), 0, inetbuffers.lpvBuffer);

    /* tests to see whether lpcszHeader is used - it isn't */
    inetbuffers.dwStructSize = sizeof(INTERNET_BUFFERS);
    inetbuffers.lpcszHeader = (LPCTSTR)0xdeadbeef;
    inetbuffers.dwHeadersLength = 255;
    inetbuffers.dwBufferLength = 0;
    inetbuffers.lpvBuffer = NULL;
    inetbuffers.dwOffsetHigh = 1234;
    inetbuffers.dwOffsetLow = 5678;
    rc = InternetReadFileEx(hor, &inetbuffers, 0, 0xdeadcafe);
    ok(rc, "InternetReadFileEx failed with error %ld\n", GetLastError());

    rc = InternetReadFileEx(NULL, &inetbuffers, 0, 0xdeadcafe);
    ok(!rc && (GetLastError() == ERROR_INVALID_HANDLE),
        "InternetReadFileEx should have failed with ERROR_INVALID_HANDLE instead of %s, %ld\n",
        rc ? "TRUE" : "FALSE", GetLastError());

    length = 0;
    trace("Entering Query loop\n");

    while (TRUE)
    {
        inetbuffers.dwStructSize = sizeof(INTERNET_BUFFERS);
        inetbuffers.dwBufferLength = 1024;
        inetbuffers.lpvBuffer = HeapAlloc(GetProcessHeap(), 0, inetbuffers.dwBufferLength+1);
        inetbuffers.dwOffsetHigh = 1234;
        inetbuffers.dwOffsetLow = 5678;

        rc = InternetReadFileExA(hor, &inetbuffers, IRF_ASYNC | IRF_USE_CONTEXT, 0xcafebabe);
        if (!rc)
        {
            if (GetLastError() == ERROR_IO_PENDING)
            {
                trace("InternetReadFileEx -> PENDING\n");
                WaitForSingleObject(hCompleteEvent, INFINITE);
            }
            else
            {
                trace("InternetReadFileEx -> FAILED %ld\n", GetLastError());
                break;
            }
        }
        else
            trace("InternetReadFileEx -> SUCCEEDED\n");

        trace("read %li bytes\n", inetbuffers.dwBufferLength);
        ((char *)inetbuffers.lpvBuffer)[inetbuffers.dwBufferLength] = '\0';

        ok(inetbuffers.dwOffsetHigh == 1234 && inetbuffers.dwOffsetLow == 5678,
            "InternetReadFileEx sets offsets to 0x%lx%08lx\n",
            inetbuffers.dwOffsetHigh, inetbuffers.dwOffsetLow);

        HeapFree(GetProcessHeap(), 0, inetbuffers.lpvBuffer);

        if (!inetbuffers.dwBufferLength)
            break;

        length += inetbuffers.dwBufferLength;
    }
    trace("Finished. Read %ld bytes\n", length);

abort:
    if (hor) {
        rc = InternetCloseHandle(hor);
        ok ((rc != 0), "InternetCloseHandle of handle opened by HttpOpenRequestA failed\n");
        rc = InternetCloseHandle(hor);
        ok ((rc == 0), "Double close of handle opened by HttpOpenRequestA succeeded\n");
    }
    if (hic) {
        rc = InternetCloseHandle(hic);
        ok ((rc != 0), "InternetCloseHandle of handle opened by InternetConnectA failed\n");
    }
    if (hi) {
      rc = InternetCloseHandle(hi);
      ok ((rc != 0), "InternetCloseHandle of handle opened by InternetOpenA failed\n");
      if (flags & INTERNET_FLAG_ASYNC)
          Sleep(100);
    }
    CloseHandle(hCompleteEvent);
}

static void InternetOpenUrlA_test(void)
{
  HINTERNET myhinternet, myhttp;
  char buffer[0x400];
  URL_COMPONENTSA urlComponents;
  char protocol[32], hostName[1024], userName[1024];
  char password[1024], extra[1024], path[1024];
  DWORD size, readbytes, totalbytes=0;
  BOOL ret;
  
  myhinternet = InternetOpen("Winetest",0,NULL,NULL,INTERNET_FLAG_NO_CACHE_WRITE);
  ok((myhinternet != 0), "InternetOpen failed, error %lx\n",GetLastError());
  size = 0x400;
  ret = InternetCanonicalizeUrl(TEST_URL, buffer, &size,ICU_BROWSER_MODE);
  ok( ret, "InternetCanonicalizeUrl failed, error %lx\n",GetLastError());
  
  urlComponents.dwStructSize = sizeof(URL_COMPONENTSA);
  urlComponents.lpszScheme = protocol;
  urlComponents.dwSchemeLength = 32;
  urlComponents.lpszHostName = hostName;
  urlComponents.dwHostNameLength = 1024;
  urlComponents.lpszUserName = userName;
  urlComponents.dwUserNameLength = 1024;
  urlComponents.lpszPassword = password;
  urlComponents.dwPasswordLength = 1024;
  urlComponents.lpszUrlPath = path;
  urlComponents.dwUrlPathLength = 2048;
  urlComponents.lpszExtraInfo = extra;
  urlComponents.dwExtraInfoLength = 1024;
  ret = InternetCrackUrl(TEST_URL, 0,0,&urlComponents);
  ok( ret, "InternetCrackUrl failed, error %lx\n",GetLastError());
  SetLastError(0);
  myhttp = InternetOpenUrl(myhinternet, TEST_URL, 0, 0,
			   INTERNET_FLAG_RELOAD|INTERNET_FLAG_NO_CACHE_WRITE|INTERNET_FLAG_TRANSFER_BINARY,0);
  if (GetLastError() == 12007)
    return; /* WinXP returns this when not connected to the net */
  ok((myhttp != 0),"InternetOpenUrl failed, error %lx\n",GetLastError());
  ret = InternetReadFile(myhttp, buffer,0x400,&readbytes);
  ok( ret, "InternetReadFile failed, error %lx\n",GetLastError());
  totalbytes += readbytes;
  while (readbytes && InternetReadFile(myhttp, buffer,0x400,&readbytes))
    totalbytes += readbytes;
  trace("read 0x%08lx bytes\n",totalbytes);
}
  
static inline void copy_compsA(
    URL_COMPONENTSA *src, 
    URL_COMPONENTSA *dst, 
    DWORD scheLen,
    DWORD hostLen,
    DWORD userLen,
    DWORD passLen,
    DWORD pathLen,
    DWORD extrLen )
{
    *dst = *src;
    dst->dwSchemeLength    = scheLen;
    dst->dwHostNameLength  = hostLen;
    dst->dwUserNameLength  = userLen;
    dst->dwPasswordLength  = passLen;
    dst->dwUrlPathLength   = pathLen;
    dst->dwExtraInfoLength = extrLen;
    SetLastError(0xfaceabad);
}
  
static inline void zero_compsA(
    URL_COMPONENTSA *dst, 
    DWORD scheLen,
    DWORD hostLen,
    DWORD userLen,
    DWORD passLen,
    DWORD pathLen,
    DWORD extrLen )
{
    ZeroMemory(dst, sizeof(URL_COMPONENTSA));
    dst->dwStructSize = sizeof(URL_COMPONENTSA);
    dst->dwSchemeLength    = scheLen;
    dst->dwHostNameLength  = hostLen;
    dst->dwUserNameLength  = userLen;
    dst->dwPasswordLength  = passLen;
    dst->dwUrlPathLength   = pathLen;
    dst->dwExtraInfoLength = extrLen;
    SetLastError(0xfaceabad);
}
  
static void InternetCrackUrl_test(void)
{
  URL_COMPONENTSA urlSrc, urlComponents;
  char protocol[32], hostName[1024], userName[1024];
  char password[1024], extra[1024], path[1024];
  BOOL ret;
  DWORD GLE;

  ZeroMemory(&urlSrc, sizeof(urlSrc));
  urlSrc.dwStructSize = sizeof(urlSrc);
  urlSrc.lpszScheme = protocol;
  urlSrc.lpszHostName = hostName;
  urlSrc.lpszUserName = userName;
  urlSrc.lpszPassword = password;
  urlSrc.lpszUrlPath = path;
  urlSrc.lpszExtraInfo = extra;

  copy_compsA(&urlSrc, &urlComponents, 32, 1024, 1024, 1024, 2048, 1024);
  ret = InternetCrackUrl(TEST_URL, 0,0,&urlComponents);
  ok( ret, "InternetCrackUrl failed, error %lx\n",GetLastError());
  ok((strcmp(TEST_URL_PATH,path) == 0),"path cracked wrong\n");

  /* Bug 1805: Confirm the returned lengths are correct:                     */
  /* 1. When extra info split out explicitly */
  zero_compsA(&urlComponents, 0, 1, 0, 0, 1, 1);
  ok(InternetCrackUrlA(TEST_URL2, 0, 0, &urlComponents),"InternetCrackUrl failed, error 0x%lx\n", GetLastError());
  ok(urlComponents.dwUrlPathLength == strlen(TEST_URL2_PATH),".dwUrlPathLength should be %ld, but is %ld\n", (DWORD)strlen(TEST_URL2_PATH), urlComponents.dwUrlPathLength);
  ok(!strncmp(urlComponents.lpszUrlPath,TEST_URL2_PATH,strlen(TEST_URL2_PATH)),"lpszUrlPath should be %s but is %s\n", TEST_URL2_PATH, urlComponents.lpszUrlPath);
  ok(urlComponents.dwHostNameLength == strlen(TEST_URL2_SERVER),".dwHostNameLength should be %ld, but is %ld\n", (DWORD)strlen(TEST_URL2_SERVER), urlComponents.dwHostNameLength);
  ok(!strncmp(urlComponents.lpszHostName,TEST_URL2_SERVER,strlen(TEST_URL2_SERVER)),"lpszHostName should be %s but is %s\n", TEST_URL2_SERVER, urlComponents.lpszHostName);
  ok(urlComponents.dwExtraInfoLength == strlen(TEST_URL2_EXTRA),".dwExtraInfoLength should be %ld, but is %ld\n", (DWORD)strlen(TEST_URL2_EXTRA), urlComponents.dwExtraInfoLength);
  ok(!strncmp(urlComponents.lpszExtraInfo,TEST_URL2_EXTRA,strlen(TEST_URL2_EXTRA)),"lpszExtraInfo should be %s but is %s\n", TEST_URL2_EXTRA, urlComponents.lpszHostName);

  /* 2. When extra info is not split out explicitly and is in url path */
  zero_compsA(&urlComponents, 0, 1, 0, 0, 1, 0);
  ok(InternetCrackUrlA(TEST_URL2, 0, 0, &urlComponents),"InternetCrackUrl failed with GLE 0x%lx\n",GetLastError());
  ok(urlComponents.dwUrlPathLength == strlen(TEST_URL2_PATHEXTRA),".dwUrlPathLength should be %ld, but is %ld\n", (DWORD)strlen(TEST_URL2_PATHEXTRA), urlComponents.dwUrlPathLength);
  ok(!strncmp(urlComponents.lpszUrlPath,TEST_URL2_PATHEXTRA,strlen(TEST_URL2_PATHEXTRA)),"lpszUrlPath should be %s but is %s\n", TEST_URL2_PATHEXTRA, urlComponents.lpszUrlPath);
  ok(urlComponents.dwHostNameLength == strlen(TEST_URL2_SERVER),".dwHostNameLength should be %ld, but is %ld\n", (DWORD)strlen(TEST_URL2_SERVER), urlComponents.dwHostNameLength);
  ok(!strncmp(urlComponents.lpszHostName,TEST_URL2_SERVER,strlen(TEST_URL2_SERVER)),"lpszHostName should be %s but is %s\n", TEST_URL2_SERVER, urlComponents.lpszHostName);

  /*3. Check for %20 */
  copy_compsA(&urlSrc, &urlComponents, 32, 1024, 1024, 1024, 2048, 1024);
  ok(InternetCrackUrlA(TEST_URL3, 0, ICU_DECODE, &urlComponents),"InternetCrackUrl failed with GLE 0x%lx\n",GetLastError());


  /* Tests for lpsz* members pointing to real strings while 
   * some corresponding length members are set to zero */
  copy_compsA(&urlSrc, &urlComponents, 0, 1024, 1024, 1024, 2048, 1024);
  ret = InternetCrackUrlA(TEST_URL3, 0, ICU_DECODE, &urlComponents);
  ok(ret==1, "InternetCrackUrl returned %d with GLE=%ld (expected to return 1)\n",
    ret, GetLastError());

  copy_compsA(&urlSrc, &urlComponents, 32, 0, 1024, 1024, 2048, 1024);
  ret = InternetCrackUrlA(TEST_URL3, 0, ICU_DECODE, &urlComponents);
  ok(ret==1, "InternetCrackUrl returned %d with GLE=%ld (expected to return 1)\n",
    ret, GetLastError());

  copy_compsA(&urlSrc, &urlComponents, 32, 1024, 0, 1024, 2048, 1024);
  ret = InternetCrackUrlA(TEST_URL3, 0, ICU_DECODE, &urlComponents);
  ok(ret==1, "InternetCrackUrl returned %d with GLE=%ld (expected to return 1)\n",
    ret, GetLastError());

  copy_compsA(&urlSrc, &urlComponents, 32, 1024, 1024, 0, 2048, 1024);
  ret = InternetCrackUrlA(TEST_URL3, 0, ICU_DECODE, &urlComponents);
  ok(ret==1, "InternetCrackUrl returned %d with GLE=%ld (expected to return 1)\n",
    ret, GetLastError());

  copy_compsA(&urlSrc, &urlComponents, 32, 1024, 1024, 1024, 0, 1024);
  ret = InternetCrackUrlA(TEST_URL3, 0, ICU_DECODE, &urlComponents);
  GLE = GetLastError();
  todo_wine
  ok(ret==0 && (GLE==ERROR_INVALID_HANDLE || GLE==ERROR_INSUFFICIENT_BUFFER),
     "InternetCrackUrl returned %d with GLE=%ld (expected to return 0 and ERROR_INVALID_HANDLE or ERROR_INSUFFICIENT_BUFFER)\n",
    ret, GLE);

  copy_compsA(&urlSrc, &urlComponents, 32, 1024, 1024, 1024, 2048, 0);
  ret = InternetCrackUrlA(TEST_URL3, 0, ICU_DECODE, &urlComponents);
  GLE = GetLastError();
  todo_wine
  ok(ret==0 && (GLE==ERROR_INVALID_HANDLE || GLE==ERROR_INSUFFICIENT_BUFFER),
     "InternetCrackUrl returned %d with GLE=%ld (expected to return 0 and ERROR_INVALID_HANDLE or ERROR_INSUFFICIENT_BUFFER)\n",
    ret, GLE);

  copy_compsA(&urlSrc, &urlComponents, 0, 0, 0, 0, 0, 0);
  ret = InternetCrackUrlA(TEST_URL3, 0, ICU_DECODE, &urlComponents);
  GLE = GetLastError();
  todo_wine
  ok(ret==0 && GLE==ERROR_INVALID_PARAMETER,
     "InternetCrackUrl returned %d with GLE=%ld (expected to return 0 and ERROR_INVALID_PARAMETER)\n",
    ret, GLE);
}

static void InternetCrackUrlW_test(void)
{
    WCHAR url[] = {
        'h','t','t','p',':','/','/','1','9','2','.','1','6','8','.','0','.','2','2','/',
        'C','F','I','D','E','/','m','a','i','n','.','c','f','m','?','C','F','S','V','R',
        '=','I','D','E','&','A','C','T','I','O','N','=','I','D','E','_','D','E','F','A',
        'U','L','T', 0 };
    URL_COMPONENTSW comp;
    WCHAR scheme[20], host[20], user[20], pwd[20], urlpart[50], extra[50];
    BOOL r;

    urlpart[0]=0;
    scheme[0]=0;
    extra[0]=0;
    host[0]=0;
    user[0]=0;
    pwd[0]=0;
    memset(&comp, 0, sizeof comp);
    comp.dwStructSize = sizeof comp;
    comp.lpszScheme = scheme;
    comp.dwSchemeLength = sizeof scheme;
    comp.lpszHostName = host;
    comp.dwHostNameLength = sizeof host;
    comp.lpszUserName = user;
    comp.dwUserNameLength = sizeof user;
    comp.lpszPassword = pwd;
    comp.dwPasswordLength = sizeof pwd;
    comp.lpszUrlPath = urlpart;
    comp.dwUrlPathLength = sizeof urlpart;
    comp.lpszExtraInfo = extra;
    comp.dwExtraInfoLength = sizeof extra;

    r = InternetCrackUrlW(url, 0, 0, &comp );
    ok( r, "failed to crack url\n");
    ok( comp.dwSchemeLength == 4, "scheme length wrong\n");
    ok( comp.dwHostNameLength == 12, "host length wrong\n");
    ok( comp.dwUserNameLength == 0, "user length wrong\n");
    ok( comp.dwPasswordLength == 0, "password length wrong\n");
    ok( comp.dwUrlPathLength == 15, "url length wrong\n");
    ok( comp.dwExtraInfoLength == 29, "extra length wrong\n");
 
    urlpart[0]=0;
    scheme[0]=0;
    extra[0]=0;
    host[0]=0;
    user[0]=0;
    pwd[0]=0;
    memset(&comp, 0, sizeof comp);
    comp.dwStructSize = sizeof comp;
    comp.lpszHostName = host;
    comp.dwHostNameLength = sizeof host;
    comp.lpszUrlPath = urlpart;
    comp.dwUrlPathLength = sizeof urlpart;

    r = InternetCrackUrlW(url, 0, 0, &comp );
    ok( r, "failed to crack url\n");
    ok( comp.dwSchemeLength == 0, "scheme length wrong\n");
    ok( comp.dwHostNameLength == 12, "host length wrong\n");
    ok( comp.dwUserNameLength == 0, "user length wrong\n");
    ok( comp.dwPasswordLength == 0, "password length wrong\n");
    ok( comp.dwUrlPathLength == 44, "url length wrong\n");
    ok( comp.dwExtraInfoLength == 0, "extra length wrong\n");

    urlpart[0]=0;
    scheme[0]=0;
    extra[0]=0;
    host[0]=0;
    user[0]=0;
    pwd[0]=0;
    memset(&comp, 0, sizeof comp);
    comp.dwStructSize = sizeof comp;
    comp.lpszHostName = host;
    comp.dwHostNameLength = sizeof host;
    comp.lpszUrlPath = urlpart;
    comp.dwUrlPathLength = sizeof urlpart;
    comp.lpszExtraInfo = NULL;
    comp.dwExtraInfoLength = sizeof extra;

    r = InternetCrackUrlW(url, 0, 0, &comp );
    ok( r, "failed to crack url\n");
    ok( comp.dwSchemeLength == 0, "scheme length wrong\n");
    ok( comp.dwHostNameLength == 12, "host length wrong\n");
    ok( comp.dwUserNameLength == 0, "user length wrong\n");
    ok( comp.dwPasswordLength == 0, "password length wrong\n");
    ok( comp.dwUrlPathLength == 15, "url length wrong\n");
    ok( comp.dwExtraInfoLength == 29, "extra length wrong\n");
}

static void InternetTimeFromSystemTimeA_test(void)
{
    BOOL ret;
    static const SYSTEMTIME time = { 2005, 1, 5, 7, 12, 6, 35, 0 };
    char string[INTERNET_RFC1123_BUFSIZE];
    static const char expect[] = "Fri, 07 Jan 2005 12:06:35 GMT";

    ret = InternetTimeFromSystemTimeA( &time, INTERNET_RFC1123_FORMAT, string, sizeof(string) );
    ok( ret, "InternetTimeFromSystemTimeA failed (%ld)\n", GetLastError() );

    ok( !memcmp( string, expect, sizeof(expect) ),
        "InternetTimeFromSystemTimeA failed (%ld)\n", GetLastError() );
}

static void InternetTimeFromSystemTimeW_test(void)
{
    BOOL ret;
    static const SYSTEMTIME time = { 2005, 1, 5, 7, 12, 6, 35, 0 };
    WCHAR string[INTERNET_RFC1123_BUFSIZE + 1];
    static const WCHAR expect[] = { 'F','r','i',',',' ','0','7',' ','J','a','n',' ','2','0','0','5',' ',
                                    '1','2',':','0','6',':','3','5',' ','G','M','T',0 };

    ret = InternetTimeFromSystemTimeW( &time, INTERNET_RFC1123_FORMAT, string, sizeof(string) );
    ok( ret, "InternetTimeFromSystemTimeW failed (%ld)\n", GetLastError() );

    ok( !memcmp( string, expect, sizeof(expect) ),
        "InternetTimeFromSystemTimeW failed (%ld)\n", GetLastError() );
}

static void InternetTimeToSystemTimeA_test(void)
{
    BOOL ret;
    SYSTEMTIME time;
    static const SYSTEMTIME expect = { 2005, 1, 5, 7, 12, 6, 35, 0 };
    static const char string[] = "Fri, 07 Jan 2005 12:06:35 GMT";
    static const char string2[] = " fri 7 jan 2005 12 06 35";

    ret = InternetTimeToSystemTimeA( string, &time, 0 );
    ok( ret, "InternetTimeToSystemTimeA failed (%ld)\n", GetLastError() );
    ok( !memcmp( &time, &expect, sizeof(expect) ),
        "InternetTimeToSystemTimeA failed (%ld)\n", GetLastError() );

    ret = InternetTimeToSystemTimeA( string2, &time, 0 );
    ok( ret, "InternetTimeToSystemTimeA failed (%ld)\n", GetLastError() );
    ok( !memcmp( &time, &expect, sizeof(expect) ),
        "InternetTimeToSystemTimeA failed (%ld)\n", GetLastError() );
}

static void InternetTimeToSystemTimeW_test(void)
{
    BOOL ret;
    SYSTEMTIME time;
    static const SYSTEMTIME expect = { 2005, 1, 5, 7, 12, 6, 35, 0 };
    static const WCHAR string[] = { 'F','r','i',',',' ','0','7',' ','J','a','n',' ','2','0','0','5',' ',
                                    '1','2',':','0','6',':','3','5',' ','G','M','T',0 };
    static const WCHAR string2[] = { ' ','f','r','i',' ','7',' ','j','a','n',' ','2','0','0','5',' ',
                                     '1','2',' ','0','6',' ','3','5',0 };
    static const WCHAR string3[] = { 'F','r',0 };

    ret = InternetTimeToSystemTimeW( NULL, NULL, 0 );
    ok( !ret, "InternetTimeToSystemTimeW succeeded (%ld)\n", GetLastError() );

    ret = InternetTimeToSystemTimeW( NULL, &time, 0 );
    ok( !ret, "InternetTimeToSystemTimeW succeeded (%ld)\n", GetLastError() );

    ret = InternetTimeToSystemTimeW( string, NULL, 0 );
    ok( !ret, "InternetTimeToSystemTimeW succeeded (%ld)\n", GetLastError() );

    ret = InternetTimeToSystemTimeW( string, &time, 1 );
    ok( ret, "InternetTimeToSystemTimeW failed (%ld)\n", GetLastError() );

    ret = InternetTimeToSystemTimeW( string, &time, 0 );
    ok( ret, "InternetTimeToSystemTimeW failed (%ld)\n", GetLastError() );
    ok( !memcmp( &time, &expect, sizeof(expect) ),
        "InternetTimeToSystemTimeW failed (%ld)\n", GetLastError() );

    ret = InternetTimeToSystemTimeW( string2, &time, 0 );
    ok( ret, "InternetTimeToSystemTimeW failed (%ld)\n", GetLastError() );
    ok( !memcmp( &time, &expect, sizeof(expect) ),
        "InternetTimeToSystemTimeW failed (%ld)\n", GetLastError() );

    ret = InternetTimeToSystemTimeW( string3, &time, 0 );
    ok( ret, "InternetTimeToSystemTimeW failed (%ld)\n", GetLastError() );
}

static void fill_url_components(LPURL_COMPONENTS lpUrlComponents)
{
	lpUrlComponents->dwStructSize = sizeof(URL_COMPONENTS);
	lpUrlComponents->lpszScheme = "http";
	lpUrlComponents->dwSchemeLength = strlen(lpUrlComponents->lpszScheme);
	lpUrlComponents->nScheme = INTERNET_SCHEME_HTTP;
	lpUrlComponents->lpszHostName = "www.winehq.org";
	lpUrlComponents->dwHostNameLength = strlen(lpUrlComponents->lpszHostName);
	lpUrlComponents->nPort = 80;
	lpUrlComponents->lpszUserName = "username";
	lpUrlComponents->dwUserNameLength = strlen(lpUrlComponents->lpszUserName);
	lpUrlComponents->lpszPassword = "password";
	lpUrlComponents->dwPasswordLength = strlen(lpUrlComponents->lpszPassword);
	lpUrlComponents->lpszUrlPath = "/site/about";
	lpUrlComponents->dwUrlPathLength = strlen(lpUrlComponents->lpszUrlPath);
	lpUrlComponents->lpszExtraInfo = "";
	lpUrlComponents->dwExtraInfoLength = strlen(lpUrlComponents->lpszExtraInfo);
}

static void InternetCreateUrlA_test(void)
{
	URL_COMPONENTS urlComp;
	LPSTR szUrl;
	DWORD len = -1;
	BOOL ret;

	/* test NULL lpUrlComponents */
	ret = InternetCreateUrlA(NULL, 0, NULL, &len);
	SetLastError(0xdeadbeef);
	ok(!ret, "Expected failure\n");
	ok(GetLastError() == 0xdeadbeef,
		"Expected 0xdeadbeef, got %ld\n", GetLastError());
	ok(len == -1, "Expected len -1, got %ld\n", len);

	/* test garbage lpUrlComponets */
	ret = InternetCreateUrlA(&urlComp, 0, NULL, &len);
	SetLastError(0xdeadbeef);
	ok(!ret, "Expected failure\n");
	ok(GetLastError() == 0xdeadbeef,
		"Expected 0xdeadbeef, got %ld\n", GetLastError());
	ok(len == -1, "Expected len -1, got %ld\n", len);

	/* test zero'ed lpUrlComponents */
	ZeroMemory(&urlComp, sizeof(URL_COMPONENTS));
	SetLastError(0xdeadbeef);
	ret = InternetCreateUrlA(&urlComp, 0, NULL, &len);
	ok(!ret, "Expected failure\n");
	ok(GetLastError() == ERROR_INVALID_PARAMETER,
		"Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());
	ok(len == -1, "Expected len -1, got %ld\n", len);

	/* test valid lpUrlComponets, NULL lpdwUrlLength */
	fill_url_components(&urlComp);
	SetLastError(0xdeadbeef);
	ret = InternetCreateUrlA(&urlComp, 0, NULL, NULL);
	ok(!ret, "Expected failure\n");
	ok(GetLastError() == ERROR_INVALID_PARAMETER,
		"Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());
	ok(len == -1, "Expected len -1, got %ld\n", len);

	/* test valid lpUrlComponets, emptry szUrl
	 * lpdwUrlLength is size of buffer required on exit, including
	 * the terminating null when GLE == ERROR_INSUFFICIENT_BUFFER
	 */
	SetLastError(0xdeadbeef);
	ret = InternetCreateUrlA(&urlComp, 0, NULL, &len);
	ok(!ret, "Expected failure\n");
	ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
		"Expected ERROR_INSUFFICIENT_BUFFER, got %ld\n", GetLastError());
	ok(len == 51, "Expected len 51, got %ld\n", len);

	/* test correct size, NULL szUrl */
	fill_url_components(&urlComp);
	SetLastError(0xdeadbeef);
	ret = InternetCreateUrlA(&urlComp, 0, NULL, &len);
	ok(!ret, "Expected failure\n");
	ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
		"Expected ERROR_INSUFFICIENT_BUFFER, got %ld\n", GetLastError());
	ok(len == 51, "Expected len 51, got %ld\n", len);

	/* test valid lpUrlComponets, alloced szUrl, small size */
	SetLastError(0xdeadbeef);
	szUrl = HeapAlloc(GetProcessHeap(), 0, len);
	len -= 2;
	ret = InternetCreateUrlA(&urlComp, 0, szUrl, &len);
	ok(!ret, "Expected failure\n");
	ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
		"Expected ERROR_INSUFFICIENT_BUFFER, got %ld\n", GetLastError());
	ok(len == 51, "Expected len 51, got %ld\n", len);

	/* alloced szUrl, NULL lpszScheme
	 * shows that it uses nScheme instead
	 */
	SetLastError(0xdeadbeef);
	urlComp.lpszScheme = NULL;
	ret = InternetCreateUrlA(&urlComp, 0, szUrl, &len);
	todo_wine {
	ok(ret, "Expected success\n");
	ok(GetLastError() == 0xdeadbeef,
		"Expected 0xdeadbeef, got %ld\n", GetLastError());
	ok(len == 50, "Expected len 50, got %ld\n", len);
	ok(!strcmp(szUrl, CREATE_URL1), "Expected %s, got %s\n", CREATE_URL1, szUrl);
	}

	/* alloced szUrl, invalid nScheme
	 * any nScheme out of range seems ignored
	 */
	fill_url_components(&urlComp);
	SetLastError(0xdeadbeef);
	urlComp.nScheme = -3;
	len++;
	ret = InternetCreateUrlA(&urlComp, 0, szUrl, &len);
	ok(ret, "Expected success\n");
	ok(GetLastError() == 0xdeadbeef,
		"Expected 0xdeadbeef, got %ld\n", GetLastError());
	ok(len == 50, "Expected len 50, got %ld\n", len);

	/* test valid lpUrlComponets, alloced szUrl */
	fill_url_components(&urlComp);
	SetLastError(0xdeadbeef);
	len = 51;
	ret = InternetCreateUrlA(&urlComp, 0, szUrl, &len);
	ok(ret, "Expected success\n");
	ok(GetLastError() == 0xdeadbeef,
		"Expected 0xdeadbeef, got %ld\n", GetLastError());
	ok(len == 50, "Expected len 50, got %ld\n", len);
	ok(strstr(szUrl, "80") == NULL, "Didn't expect to find 80 in szUrl\n");
	ok(!strcmp(szUrl, CREATE_URL1), "Expected %s, got %s\n", CREATE_URL1, szUrl);

	/* valid username, NULL password */
	fill_url_components(&urlComp);
	SetLastError(0xdeadbeef);
	urlComp.lpszPassword = NULL;
	len = 42;
	ret = InternetCreateUrlA(&urlComp, 0, szUrl, &len);
	ok(ret, "Expected success\n");
	ok(GetLastError() == 0xdeadbeef,
		"Expected 0xdeadbeef, got %ld\n", GetLastError());
	ok(len == 41, "Expected len 41, got %ld\n", len);
	ok(!strcmp(szUrl, CREATE_URL2), "Expected %s, got %s\n", CREATE_URL2, szUrl);

	/* valid username, empty password */
	fill_url_components(&urlComp);
	SetLastError(0xdeadbeef);
	urlComp.lpszPassword = "";
	len = 51;
	ret = InternetCreateUrlA(&urlComp, 0, szUrl, &len);
	ok(ret, "Expected success\n");
	ok(GetLastError() == 0xdeadbeef,
		"Expected 0xdeadbeef, got %ld\n", GetLastError());
	ok(len == 50, "Expected len 50, got %ld\n", len);
	ok(!strcmp(szUrl, CREATE_URL3), "Expected %s, got %s\n", CREATE_URL3, szUrl);

	/* valid password, NULL username
	 * if password is provided, username has to exist
	 */
	fill_url_components(&urlComp);
	SetLastError(0xdeadbeef);
	urlComp.lpszUserName = NULL;
	len = 42;
	ret = InternetCreateUrlA(&urlComp, 0, szUrl, &len);
	ok(!ret, "Expected failure\n");
	ok(GetLastError() == ERROR_INVALID_PARAMETER,
		"Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());
	ok(len == 42, "Expected len 42, got %ld\n", len);
	ok(!strcmp(szUrl, CREATE_URL3), "Expected %s, got %s\n", CREATE_URL3, szUrl);

	/* valid password, empty username
	 * if password is provided, username has to exist
	 */
	fill_url_components(&urlComp);
	SetLastError(0xdeadbeef);
	urlComp.lpszUserName = "";
	len = 51;
	ret = InternetCreateUrlA(&urlComp, 0, szUrl, &len);
	ok(ret, "Expected success\n");
	ok(GetLastError() == 0xdeadbeef,
		"Expected 0xdeadbeef, got %ld\n", GetLastError());
	ok(len == 50, "Expected len 50, got %ld\n", len);
	ok(!strcmp(szUrl, CREATE_URL5), "Expected %s, got %s\n", CREATE_URL5, szUrl);

	/* NULL username, NULL password */
	fill_url_components(&urlComp);
	SetLastError(0xdeadbeef);
	urlComp.lpszUserName = NULL;
	urlComp.lpszPassword = NULL;
	len = 42;
	ret = InternetCreateUrlA(&urlComp, 0, szUrl, &len);
	ok(ret, "Expected success\n");
	ok(GetLastError() == 0xdeadbeef,
		"Expected 0xdeadbeef, got %ld\n", GetLastError());
	ok(len == 32, "Expected len 32, got %ld\n", len);
	ok(!strcmp(szUrl, CREATE_URL4), "Expected %s, got %s\n", CREATE_URL4, szUrl);

	/* empty username, empty password */
	fill_url_components(&urlComp);
	SetLastError(0xdeadbeef);
	urlComp.lpszUserName = "";
	urlComp.lpszPassword = "";
	len = 51;
	ret = InternetCreateUrlA(&urlComp, 0, szUrl, &len);
	ok(ret, "Expected success\n");
	ok(GetLastError() == 0xdeadbeef,
		"Expected 0xdeadbeef, got %ld\n", GetLastError());
	ok(len == 50, "Expected len 50, got %ld\n", len);
	ok(!strcmp(szUrl, CREATE_URL5), "Expected %s, got %s\n", CREATE_URL5, szUrl);

	/* shows that nScheme is ignored, as the appearance of the port number
	 * depends on lpszScheme and the string copied depends on lpszScheme.
	 */
	fill_url_components(&urlComp);
	HeapFree(GetProcessHeap(), 0, szUrl);
	urlComp.lpszScheme = "nhttp";
	urlComp.dwSchemeLength = strlen(urlComp.lpszScheme);
	len = strlen(CREATE_URL6) + 1;
	szUrl = (char *)HeapAlloc(GetProcessHeap(), 0, len);
	ret = InternetCreateUrlA(&urlComp, ICU_ESCAPE, szUrl, &len);
	ok(ret, "Expected success\n");
	ok(len == strlen(CREATE_URL6), "Expected len %d, got %ld\n", strlen(CREATE_URL6) + 1, len);
	ok(!strcmp(szUrl, CREATE_URL6), "Expected %s, got %s\n", CREATE_URL6, szUrl);

	/* if lpszScheme != "http" or nPort != 80, display nPort */
	HeapFree(GetProcessHeap(), 0, szUrl);
	urlComp.lpszScheme = "http";
	urlComp.dwSchemeLength = strlen(urlComp.lpszScheme);
	urlComp.nPort = 42;
	szUrl = HeapAlloc(GetProcessHeap(), 0, ++len);
	ret = InternetCreateUrlA(&urlComp, ICU_ESCAPE, szUrl, &len);
	ok(ret, "Expected success\n");
	ok(len == 53, "Expected len 53, got %ld\n", len);
	ok(strstr(szUrl, "42") != NULL, "Expected to find 42 in szUrl\n");
	ok(!strcmp(szUrl, CREATE_URL7), "Expected %s, got %s\n", CREATE_URL7, szUrl);

	HeapFree(GetProcessHeap(), 0, szUrl);

	memset(&urlComp, 0, sizeof(urlComp));
	urlComp.dwStructSize = sizeof(URL_COMPONENTS);
	urlComp.lpszScheme = "http";
	urlComp.dwSchemeLength = 0;
	urlComp.nScheme = INTERNET_SCHEME_HTTP;
	urlComp.lpszHostName = "www.winehq.org";
	urlComp.dwHostNameLength = 0;
	urlComp.nPort = 80;
	urlComp.lpszUserName = "username";
	urlComp.dwUserNameLength = 0;
	urlComp.lpszPassword = "password";
	urlComp.dwPasswordLength = 0;
	urlComp.lpszUrlPath = "/site/about";
	urlComp.dwUrlPathLength = 0;
	urlComp.lpszExtraInfo = "";
	urlComp.dwExtraInfoLength = 0;
	len = strlen(CREATE_URL1);
	szUrl = (char *)HeapAlloc(GetProcessHeap(), 0, ++len);
	ret = InternetCreateUrlA(&urlComp, ICU_ESCAPE, szUrl, &len);
	ok(ret, "Expected success\n");
	ok(len == strlen(CREATE_URL1), "Expected len %d, got %ld\n", strlen(CREATE_URL1), len);
	ok(!strcmp(szUrl, CREATE_URL1), "Expected %s, got %s\n", CREATE_URL1, szUrl);

	memset(&urlComp, 0, sizeof(urlComp));
	urlComp.dwStructSize = sizeof(URL_COMPONENTS);
	urlComp.lpszScheme = "https";
	urlComp.dwSchemeLength = 0;
	urlComp.nScheme = INTERNET_SCHEME_HTTP;
	urlComp.lpszHostName = "www.winehq.org";
	urlComp.dwHostNameLength = 0;
	urlComp.nPort = 443;
	urlComp.lpszUserName = "username";
	urlComp.dwUserNameLength = 0;
	urlComp.lpszPassword = "password";
	urlComp.dwPasswordLength = 0;
	urlComp.lpszUrlPath = "/site/about";
	urlComp.dwUrlPathLength = 0;
	urlComp.lpszExtraInfo = "";
	urlComp.dwExtraInfoLength = 0;
	len = strlen(CREATE_URL8);
	szUrl = (char *)HeapAlloc(GetProcessHeap(), 0, ++len);
	ret = InternetCreateUrlA(&urlComp, ICU_ESCAPE, szUrl, &len);
	ok(ret, "Expected success\n");
	ok(len == strlen(CREATE_URL8), "Expected len %d, got %ld\n", strlen(CREATE_URL8), len);
	ok(!strcmp(szUrl, CREATE_URL8), "Expected %s, got %s\n", CREATE_URL8, szUrl);

	HeapFree(GetProcessHeap(), 0, szUrl);

	memset(&urlComp, 0, sizeof(urlComp));
	urlComp.dwStructSize = sizeof(URL_COMPONENTS);
	urlComp.lpszScheme = "about";
	urlComp.dwSchemeLength = 5;
	urlComp.lpszUrlPath = "blank";
	urlComp.dwUrlPathLength = 5;
	len = strlen(CREATE_URL9);
	szUrl = (char *)HeapAlloc(GetProcessHeap(), 0, ++len);
	ret = InternetCreateUrlA(&urlComp, ICU_ESCAPE, szUrl, &len);
	todo_wine {
	ok(ret, "Expected success\n");
	ok(len == strlen(CREATE_URL9), "Expected len %d, got %ld\n", strlen(CREATE_URL9), len);
	ok(!strcmp(szUrl, CREATE_URL9), "Expected %s, got %s\n", CREATE_URL9, szUrl);
	}

	HeapFree(GetProcessHeap(), 0, szUrl);
}

static void HttpSendRequestEx_test(void)
{
    HINTERNET hSession;
    HINTERNET hConnect;
    HINTERNET hRequest;

    INTERNET_BUFFERS BufferIn;
    DWORD dwBytesWritten;
    DWORD dwBytesRead;
    CHAR szBuffer[256];
    int i;
    BOOL ret;

    static const char szPostData[] = "mode=Test";
    static const char szContentType[] = "Content-Type: application/x-www-form-urlencoded";
    
    hSession = InternetOpen("Wine Regression Test",
            INTERNET_OPEN_TYPE_PRECONFIG,NULL,NULL,0);
    ok( hSession != NULL ,"Unable to open Internet session\n");
    hConnect = InternetConnect(hSession, "crossover.codeweavers.com",
            INTERNET_DEFAULT_HTTP_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0,
            0);
    ok( hConnect != NULL, "Unable to connect to http://crossover.codeweavers.com\n");
    hRequest = HttpOpenRequest(hConnect, "POST", "/posttest.php",
            NULL, NULL, NULL, INTERNET_FLAG_NO_CACHE_WRITE, 0);
    ok( hRequest != NULL, "Failed to open request handle\n");


    BufferIn.dwStructSize = sizeof( INTERNET_BUFFERS);
    BufferIn.Next = (LPINTERNET_BUFFERS)0xdeadcab;
    BufferIn.lpcszHeader = szContentType;
    BufferIn.dwHeadersLength = sizeof(szContentType);
    BufferIn.dwHeadersTotal = sizeof(szContentType);
    BufferIn.lpvBuffer = (LPVOID)szPostData;
    BufferIn.dwBufferLength = 3;
    BufferIn.dwBufferTotal = sizeof(szPostData)-1;
    BufferIn.dwOffsetLow = 0;
    BufferIn.dwOffsetHigh = 0;

    ret = HttpSendRequestEx(hRequest, &BufferIn, NULL, 0 ,0);
    ok(ret, "HttpSendRequestEx Failed with error %ld\n", GetLastError());

    for (i = 3; szPostData[i]; i++)
        ok(InternetWriteFile(hRequest, &szPostData[i], 1, &dwBytesWritten),
                "InternetWriteFile failed\n");

    ok(HttpEndRequest(hRequest, NULL, 0, 0), "HttpEndRequest Failed\n");

    ok(InternetReadFile(hRequest, szBuffer, 255, &dwBytesRead),
            "Unable to read response\n");
    szBuffer[dwBytesRead] = 0;

    ok(dwBytesRead == 13,"Read %lu bytes instead of 13\n",dwBytesRead);
    ok(strncmp(szBuffer,"mode => Test\n",dwBytesRead)==0,"Got string %s\n",szBuffer);

    ok(InternetCloseHandle(hRequest), "Close request handle failed\n");
    ok(InternetCloseHandle(hConnect), "Close connect handle failed\n");
    ok(InternetCloseHandle(hSession), "Close session handle failed\n");
}

static void HttpHeaders_test(void)
{
    HINTERNET hSession;
    HINTERNET hConnect;
    HINTERNET hRequest;
    CHAR      buffer[256];
    DWORD     len = 256;
    DWORD     index = 0;

    hSession = InternetOpen("Wine Regression Test",
            INTERNET_OPEN_TYPE_PRECONFIG,NULL,NULL,0);
    ok( hSession != NULL ,"Unable to open Internet session\n");
    hConnect = InternetConnect(hSession, "crossover.codeweavers.com",
            INTERNET_DEFAULT_HTTP_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0,
            0);
    ok( hConnect != NULL, "Unable to connect to http://crossover.codeweavers.com\n");
    hRequest = HttpOpenRequest(hConnect, "POST", "/posttest.php",
            NULL, NULL, NULL, INTERNET_FLAG_NO_CACHE_WRITE, 0);
    ok( hRequest != NULL, "Failed to open request handle\n");

    index = 0;
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfo(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS,
               buffer,&len,&index)==0,"Warning hearder reported as Existing\n");
    
    ok(HttpAddRequestHeaders(hRequest,"Warning:test1",-1,HTTP_ADDREQ_FLAG_ADD),
            "Failed to add new header\n");

    index = 0;
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfo(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS,
                buffer,&len,&index),"Unable to query header\n");
    ok(index == 1, "Index was not incremented\n");
    ok(strcmp(buffer,"test1")==0, "incorrect string was returned(%s)\n",buffer);
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfo(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS,
                buffer,&len,&index)==0,"Second Index Should Not Exist\n");

    ok(HttpAddRequestHeaders(hRequest,"Warning:test2",-1,HTTP_ADDREQ_FLAG_ADD),
            "Failed to add duplicate header using HTTP_ADDREQ_FLAG_ADD\n");

    index = 0;
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfo(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS,
                buffer,&len,&index),"Unable to query header\n");
    ok(index == 1, "Index was not incremented\n");
    ok(strcmp(buffer,"test1")==0, "incorrect string was returned(%s)\n",buffer);
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfo(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS,
                buffer,&len,&index),"Failed to get second header\n");
    ok(index == 2, "Index was not incremented\n");
    ok(strcmp(buffer,"test2")==0, "incorrect string was returned(%s)\n",buffer);
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfo(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS,
                buffer,&len,&index)==0,"Third Header Should Not Exist\n");

    ok(HttpAddRequestHeaders(hRequest,"Warning:test3",-1,HTTP_ADDREQ_FLAG_REPLACE), "Failed to replace header using HTTP_ADDREQ_FLAG_REPLACE\n");

    index = 0;
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfo(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS,
                buffer,&len,&index),"Unable to query header\n");
    ok(index == 1, "Index was not incremented\n");
    ok(strcmp(buffer,"test2")==0, "incorrect string was returned(%s)\n",buffer);
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfo(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS,
                buffer,&len,&index),"Failed to get second header\n");
    ok(index == 2, "Index was not incremented\n");
    ok(strcmp(buffer,"test3")==0, "incorrect string was returned(%s)\n",buffer);
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfo(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS,
                buffer,&len,&index)==0,"Third Header Should Not Exist\n");
    
    ok(HttpAddRequestHeaders(hRequest,"Warning:test4",-1,HTTP_ADDREQ_FLAG_ADD_IF_NEW)==0, "HTTP_ADDREQ_FLAG_ADD_IF_NEW replaced existing header\n");

    index = 0;
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfo(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS,
                buffer,&len,&index),"Unable to query header\n");
    ok(index == 1, "Index was not incremented\n");
    ok(strcmp(buffer,"test2")==0, "incorrect string was returned(%s)\n",buffer);
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfo(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS,
                buffer,&len,&index),"Failed to get second header\n");
    ok(index == 2, "Index was not incremented\n");
    ok(strcmp(buffer,"test3")==0, "incorrect string was returned(%s)\n",buffer);
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfo(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS,
                buffer,&len,&index)==0,"Third Header Should Not Exist\n");

    ok(HttpAddRequestHeaders(hRequest,"Warning:test4",-1, HTTP_ADDREQ_FLAG_COALESCE), "HTTP_ADDREQ_FLAG_COALESCE Did not work\n");

    index = 0;
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfo(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS,
                buffer,&len,&index),"Unable to query header\n");
    ok(index == 1, "Index was not incremented\n");
    ok(strcmp(buffer,"test2, test4")==0, "incorrect string was returned(%s)\n", buffer);
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfo(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer,&len,&index),"Failed to get second header\n");
    ok(index == 2, "Index was not incremented\n");
    ok(strcmp(buffer,"test3")==0, "incorrect string was returned(%s)\n",buffer);
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfo(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer,&len,&index)==0,"Third Header Should Not Exist\n");

    ok(HttpAddRequestHeaders(hRequest,"Warning:test5",-1, HTTP_ADDREQ_FLAG_COALESCE_WITH_COMMA), "HTTP_ADDREQ_FLAG_COALESCE Did not work\n");

    index = 0;
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfo(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer,&len,&index),"Unable to query header\n");
    ok(index == 1, "Index was not incremented\n");
    ok(strcmp(buffer,"test2, test4, test5")==0, "incorrect string was returned(%s)\n",buffer);
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfo(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer,&len,&index),"Failed to get second header\n");
    ok(index == 2, "Index was not incremented\n");
    ok(strcmp(buffer,"test3")==0, "incorrect string was returned(%s)\n",buffer);
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfo(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer,&len,&index)==0,"Third Header Should Not Exist\n");

    ok(HttpAddRequestHeaders(hRequest,"Warning:test6",-1, HTTP_ADDREQ_FLAG_COALESCE_WITH_SEMICOLON), "HTTP_ADDREQ_FLAG_COALESCE Did not work\n");

    index = 0;
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfo(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer,&len,&index),"Unable to query header\n");
    ok(index == 1, "Index was not incremented\n");
    ok(strcmp(buffer,"test2, test4, test5; test6")==0, "incorrect string was returned(%s)\n",buffer);
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfo(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer,&len,&index),"Failed to get second header\n");
    ok(index == 2, "Index was not incremented\n");
    ok(strcmp(buffer,"test3")==0, "incorrect string was returned(%s)\n",buffer);
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfo(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer,&len,&index)==0,"Third Header Should Not Exist\n");

    ok(HttpAddRequestHeaders(hRequest,"Warning:test7",-1, HTTP_ADDREQ_FLAG_ADD|HTTP_ADDREQ_FLAG_REPLACE), "HTTP_ADDREQ_FLAG_ADD with HTTP_ADDREQ_FLAG_REPALCE Did not work\n");

    index = 0;
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfo(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer,&len,&index),"Unable to query header\n");
    ok(index == 1, "Index was not incremented\n");
    ok(strcmp(buffer,"test3")==0, "incorrect string was returned(%s)\n",buffer);
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfo(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer,&len,&index),"Failed to get second header\n");
    ok(index == 2, "Index was not incremented\n");
    ok(strcmp(buffer,"test7")==0, "incorrect string was returned(%s)\n",buffer);
    len = sizeof(buffer);
    strcpy(buffer,"Warning");
    ok(HttpQueryInfo(hRequest,HTTP_QUERY_CUSTOM|HTTP_QUERY_FLAG_REQUEST_HEADERS, buffer,&len,&index)==0,"Third Header Should Not Exist\n");

    
    ok(InternetCloseHandle(hRequest), "Close request handle failed\n");
    ok(InternetCloseHandle(hConnect), "Close connect handle failed\n");
    ok(InternetCloseHandle(hSession), "Close session handle failed\n");
}



START_TEST(http)
{
    InternetReadFile_test(INTERNET_FLAG_ASYNC);
    InternetReadFile_test(0);
    InternetReadFileExA_test(INTERNET_FLAG_ASYNC);
    InternetCrackUrl_test();
    InternetOpenUrlA_test();
    InternetCrackUrlW_test();
    InternetTimeFromSystemTimeA_test();
    InternetTimeFromSystemTimeW_test();
    InternetTimeToSystemTimeA_test();
    InternetTimeToSystemTimeW_test();
    InternetCreateUrlA_test();
    HttpSendRequestEx_test();
    HttpHeaders_test();
}
