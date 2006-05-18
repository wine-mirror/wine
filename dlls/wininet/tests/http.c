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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "wininet.h"
#include "winsock.h"

#include "wine/test.h"

#define TEST_URL "http://www.winehq.org/site/about"

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
  DWORD size, readbytes, totalbytes=0;
  BOOL ret;
  
  myhinternet = InternetOpen("Winetest",0,NULL,NULL,INTERNET_FLAG_NO_CACHE_WRITE);
  ok((myhinternet != 0), "InternetOpen failed, error %lx\n",GetLastError());
  size = 0x400;
  ret = InternetCanonicalizeUrl(TEST_URL, buffer, &size,ICU_BROWSER_MODE);
  ok( ret, "InternetCanonicalizeUrl failed, error %lx\n",GetLastError());

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

static const char okmsg[] =
"HTTP/1.0 200 OK\r\n"
"Server: winetest\r\n"
"\r\n";

static const char page1[] =
"<HTML>\r\n"
"<HEAD><TITLE>wininet test page</TITLE></HEAD>\r\n"
"<BODY>The quick brown fox jumped over the lazy dog<P></BODY>\r\n"
"</HTML>\r\n\r\n";

struct server_info {
    HANDLE hEvent;
    int port;
};

static DWORD CALLBACK server_thread(LPVOID param)
{
    struct server_info *si = param;
    int r, s, c, i;
    struct sockaddr_in sa;
    char buffer[0x100];
    WSADATA wsaData;

    WSAStartup(MAKEWORD(1,1), &wsaData);

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s<0)
        return FALSE;

    memset(&sa, 0, sizeof sa);
    sa.sin_family = AF_INET;
    sa.sin_port = htons(si->port);
    sa.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");

    r = bind(s, (struct sockaddr*) &sa, sizeof sa);
    if (r<0)
        return 1;

    listen(s, 1);

    SetEvent(si->hEvent);

    c = accept(s, NULL, NULL);

    memset(buffer, 0, sizeof buffer);
    for(i=0; i<(sizeof buffer-1); i++)
    {
        r = recv(c, &buffer[i], 1, 0);
        if (r != 1)
            break;
        if (i<4) continue;
        if (buffer[i-2] == '\n' && buffer[i] == '\n' &&
            buffer[i-3] == '\r' && buffer[i-1] == '\r')
            break;
    }

    send(c, okmsg, sizeof okmsg-1, 0);
    send(c, page1, sizeof page1-1, 0);

    closesocket(c);
    closesocket(s);

    return 0;
}

static void test_http_connection(void)
{
    struct server_info si;
    HINTERNET hi, hc, hr;
    HANDLE hThread;
    DWORD id = 0, r, count;
    char buffer[0x100];

    si.hEvent = CreateEvent(NULL, 0, 0, NULL);
    si.port = 7531;

    hThread = CreateThread(NULL, 0, server_thread, (LPVOID) &si, 0, &id);
    ok( hThread != NULL, "create thread failed\n");

    r = WaitForSingleObject(si.hEvent, 3000);
    ok (r == WAIT_OBJECT_0, "failed to start wininet test server\n");
    if (r != WAIT_OBJECT_0)
        return;

    hi = InternetOpen(NULL, 0, NULL, NULL, 0);
    ok(hi != NULL, "open failed\n");

    hc = InternetConnect(hi, "localhost", si.port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    ok(hc != NULL, "connect failed\n");

    hr = HttpOpenRequest(hc, NULL, "/test", NULL, NULL, NULL, 0, 0);
    ok(hr != NULL, "HttpOpenRequest failed\n");

    r = HttpSendRequest(hr, NULL, 0, NULL, 0);
    ok(r, "HttpSendRequest failed\n");

    count = 0;
    memset(buffer, 0, sizeof buffer);
    r = InternetReadFile(hr, buffer, sizeof buffer, &count);
    ok(r, "InternetReadFile failed\n");
    ok(count == sizeof page1 - 1, "count was wrong\n");
    ok(!memcmp(buffer, page1, sizeof page1), "http data wrong\n");

    InternetCloseHandle(hr);
    InternetCloseHandle(hc);
    InternetCloseHandle(hi);

    r = WaitForSingleObject(hThread, 3000);
    ok( r == WAIT_OBJECT_0, "thread wait failed\n");
    CloseHandle(hThread);
}

START_TEST(http)
{
    InternetReadFile_test(INTERNET_FLAG_ASYNC);
    InternetReadFile_test(0);
    InternetReadFileExA_test(INTERNET_FLAG_ASYNC);
    InternetOpenUrlA_test();
    InternetTimeFromSystemTimeA_test();
    InternetTimeFromSystemTimeW_test();
    InternetTimeToSystemTimeA_test();
    InternetTimeToSystemTimeW_test();
    HttpSendRequestEx_test();
    HttpHeaders_test();
    test_http_connection();
}
