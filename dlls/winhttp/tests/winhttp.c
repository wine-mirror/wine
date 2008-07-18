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

START_TEST (winhttp)
{
    test_OpenRequest();
}
