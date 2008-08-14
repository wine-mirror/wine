/*
 * WinHTTP - tests
 *
 * Copyright 2008 Google (Zac Brown)
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
#include <stdlib.h>

#include <windef.h>
#include <winbase.h>
#include <winhttp.h>

#include "wine/test.h"

static const WCHAR test_useragent[] =
    {'W','i','n','e',' ','R','e','g','r','e','s','s','i','o','n',' ','T','e','s','t',0};
static const WCHAR test_server[] = {'w','i','n','e','h','q','.','o','r','g',0};

static void test_OpenRequest (void)
{
    BOOL ret;
    HINTERNET session, request, connection;

    session = WinHttpOpen(test_useragent, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    todo_wine ok(session != NULL, "WinHttpOpen failed to open session.\n");

    /* Test with a bad server name */
    SetLastError(0xdeadbeef);
    connection = WinHttpConnect(session, NULL, INTERNET_DEFAULT_HTTP_PORT, 0);
    ok (connection == NULL, "WinHttpConnect succeeded in opening connection to NULL server argument.\n");
    todo_wine ok(GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected ERROR_INVALID_PARAMETER, got %u.\n", GetLastError());

    /* Test with a valid server name */
    connection = WinHttpConnect (session, test_server, INTERNET_DEFAULT_HTTP_PORT, 0);
    todo_wine ok(connection != NULL,
        "WinHttpConnect failed to open a connection, error: %u.\n", GetLastError());

    request = WinHttpOpenRequest(connection, NULL, NULL, NULL, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    if (request == NULL && GetLastError() == ERROR_WINHTTP_NAME_NOT_RESOLVED)
    {
        skip("Network unreachable, skipping.\n");
        goto done;
    }
    todo_wine ok(request != NULL,
        "WinHttpOpenrequest failed to open a request, error: %u.\n", GetLastError());

    ret = WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, NULL, 0, 0, 0);
    todo_wine ok(ret == TRUE, "WinHttpSendRequest failed: %u\n", GetLastError());
    ret = WinHttpCloseHandle(request);
    todo_wine ok(ret == TRUE, "WinHttpCloseHandle failed on closing request, got %d.\n", ret);

 done:
    ret = WinHttpCloseHandle(connection);
    todo_wine ok(ret == TRUE, "WinHttpCloseHandle failed on closing connection, got %d.\n", ret);
    ret = WinHttpCloseHandle(session);
    todo_wine ok(ret == TRUE, "WinHttpCloseHandle failed on closing session, got %d.\n", ret);

}

static void test_SendRequest (void)
{
    HINTERNET session, request, connection;
    DWORD header_len, optional_len, total_len;
    DWORD bytes_rw;
    BOOL ret;
    CHAR buffer[256];
    int i;

    static const WCHAR test_site[] = {'c','r','o','s','s','o','v','e','r','.',
                                'c','o','d','e','w','e','a','v','e','r','s','.','c','o','m',0};
    static const WCHAR content_type[] =
        {'C','o','n','t','e','n','t','-','T','y','p','e',':',' ','a','p','p','l','i','c','a','t','i','o','n',
         '/','x','-','w','w','w','-','f','o','r','m','-','u','r','l','e','n','c','o','d','e','d',0};
    static const WCHAR test_file[] = {'/','p','o','s','t','t','e','s','t','.','p','h','p',0};
    static const WCHAR test_verb[] = {'P','O','S','T',0};
    static CHAR post_data[] = "mode=Test";
    static CHAR test_post[] = "mode => Test\\0\n";

    header_len = -1L;
    total_len = optional_len = sizeof(post_data);
    memset(buffer, 0xff, sizeof(buffer));

    session = WinHttpOpen(test_useragent, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    todo_wine ok(session != NULL, "WinHttpOpen failed to open session.\n");

    connection = WinHttpConnect (session, test_site, INTERNET_DEFAULT_HTTP_PORT, 0);
    todo_wine ok(connection != NULL,
        "WinHttpConnect failed to open a connection, error: %u.\n", GetLastError());

    request = WinHttpOpenRequest(connection, test_verb, test_file, NULL, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_BYPASS_PROXY_CACHE);
    if (request == NULL && GetLastError() == ERROR_WINHTTP_NAME_NOT_RESOLVED)
    {
        skip("Network unreachable, skipping.\n");
        goto done;
    }
    todo_wine ok(request != NULL, "WinHttpOpenrequest failed to open a request, error: %u.\n", GetLastError());

    ret = WinHttpSendRequest(request, content_type, header_len, post_data, optional_len, total_len, 0);
    todo_wine ok(ret == TRUE, "WinHttpSendRequest failed: %u\n", GetLastError());

    for (i = 3; post_data[i]; i++)
    {
        bytes_rw = -1;
        ret = WinHttpWriteData(request, &post_data[i], 1, &bytes_rw);
        todo_wine ok(ret == TRUE, "WinHttpWriteData failed: %u.\n", GetLastError());
        todo_wine ok(bytes_rw == 1, "WinHttpWriteData failed, wrote %u bytes instead of 1 byte.\n", bytes_rw);
    }

    ret = WinHttpReceiveResponse(request, NULL);
    todo_wine ok(ret == TRUE, "WinHttpReceiveResponse failed: %u.\n", GetLastError());

    bytes_rw = -1;
    ret = WinHttpReadData(request, buffer, sizeof(buffer) - 1, &bytes_rw);
    todo_wine ok(ret == TRUE, "WinHttpReadData failed: %u.\n", GetLastError());

    todo_wine ok(bytes_rw == strlen(test_post), "Read %u bytes instead of %d.\n",
        bytes_rw, strlen(test_post));
    todo_wine ok(strncmp(buffer, test_post, bytes_rw) == 0,
        "Data read did not match, got '%s'.\n", buffer);

    ret = WinHttpCloseHandle(request);
    todo_wine ok(ret == TRUE, "WinHttpCloseHandle failed on closing request, got %d.\n", ret);
 done:
    ret = WinHttpCloseHandle(connection);
    todo_wine ok(ret == TRUE, "WinHttpCloseHandle failed on closing connection, got %d.\n", ret);
    ret = WinHttpCloseHandle(session);
    todo_wine ok(ret == TRUE, "WinHttpCloseHandle failed on closing session, got %d.\n", ret);
}

static void test_WinHttpTimeFromSystemTime(void)
{
    BOOL ret;
    static const SYSTEMTIME time = {2008, 7, 1, 28, 10, 5, 52, 0};
    static const WCHAR expected_string[] =
        {'M','o','n',',',' ','2','8',' ','J','u','l',' ','2','0','0','8',' ',
         '1','0',':','0','5',':','5','2',' ','G','M','T',0};
    WCHAR time_string[WINHTTP_TIME_FORMAT_BUFSIZE+1];

    ret = WinHttpTimeFromSystemTime(&time, time_string);
    todo_wine ok(ret == TRUE, "WinHttpTimeFromSystemTime failed: %u\n", GetLastError());
    todo_wine ok(memcmp(time_string, expected_string, sizeof(expected_string)) == 0,
        "Time string returned did not match expected time string.\n");
}

START_TEST (winhttp)
{
    test_OpenRequest();
    test_SendRequest();
    test_WinHttpTimeFromSystemTime();
}
