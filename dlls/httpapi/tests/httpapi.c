/*
 * HTTP server API tests
 *
 * Copyright 2017 Nikolay Sivov for CodeWeavers
 * Copyright 2019 Zebediah Figura
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
#include <wchar.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winnt.h"
#include "winternl.h"
#include "http.h"

#include "wine/test.h"

static ULONG (WINAPI *pHttpAddUrlToUrlGroup)(HTTP_URL_GROUP_ID id, const WCHAR *url, HTTP_URL_CONTEXT context, ULONG reserved);
static ULONG (WINAPI *pHttpCreateServerSession)(HTTPAPI_VERSION version, HTTP_SERVER_SESSION_ID *session_id, ULONG reserved);
static ULONG (WINAPI *pHttpCreateRequestQueue)(HTTPAPI_VERSION version, const WCHAR *name, SECURITY_ATTRIBUTES *sa, ULONG flags, HANDLE *handle);
static ULONG (WINAPI *pHttpCreateUrlGroup)(HTTP_SERVER_SESSION_ID session_id, HTTP_URL_GROUP_ID *group_id, ULONG reserved);
static ULONG (WINAPI *pHttpCloseRequestQueue)(HANDLE queue);
static ULONG (WINAPI *pHttpCloseServerSession)(HTTP_SERVER_SESSION_ID session_id);
static ULONG (WINAPI *pHttpCloseUrlGroup)(HTTP_URL_GROUP_ID group_id);
static ULONG (WINAPI *pHttpRemoveUrlFromUrlGroup)(HTTP_URL_GROUP_ID id, const WCHAR *url, ULONG flags);
static ULONG (WINAPI *pHttpSetUrlGroupProperty)(HTTP_URL_GROUP_ID id, HTTP_SERVER_PROPERTY property, void *value, ULONG length);

static void init(void)
{
    HMODULE mod = GetModuleHandleA("httpapi.dll");

#define X(f) p##f = (void *)GetProcAddress(mod, #f)
    X(HttpAddUrlToUrlGroup);
    X(HttpCreateRequestQueue);
    X(HttpCreateServerSession);
    X(HttpCreateUrlGroup);
    X(HttpCloseRequestQueue);
    X(HttpCloseServerSession);
    X(HttpCloseUrlGroup);
    X(HttpRemoveUrlFromUrlGroup);
    X(HttpSetUrlGroupProperty);
#undef X
}

static const char simple_req[] =
    "GET /foobar HTTP/1.1\r\n"
    "Host: localhost:%u\r\n"
    "Connection: keep-alive\r\n"
    "User-Agent: WINE\r\n"
    "\r\n";

static SOCKET create_client_socket(unsigned short port)
{
    struct sockaddr_in sockaddr =
    {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr.S_un.S_addr = inet_addr("127.0.0.1"),
    };
    SOCKET s = socket(AF_INET, SOCK_STREAM, 0), ret;
    ret = connect(s, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
    ok(!ret, "Failed to connect socket, error %lu.\n", GetLastError());
    return s;
}

/* Helper function for when we don't care about the response received. */
static void send_response_v1(HANDLE queue, HTTP_REQUEST_ID id, int s)
{
    HTTP_RESPONSE_V1 response = {};
    char response_buffer[2048];
    int ret;

    response.StatusCode = 418;
    response.pReason = "I'm a teapot";
    response.ReasonLength = 12;
    ret = HttpSendHttpResponse(queue, id, 0, (HTTP_RESPONSE *)&response, NULL, NULL, NULL, 0, NULL, NULL);
    ok(!ret, "Got error %u.\n", ret);
    ret = recv(s, response_buffer, sizeof(response_buffer), 0);
    ok(ret > 0, "recv() failed.\n");
}

static unsigned short add_url_v1(HANDLE queue)
{
    unsigned short port;
    WCHAR url[50];
    int ret;

    for (port = 50000; port < 51000; ++port)
    {
        swprintf(url, ARRAY_SIZE(url), L"http://localhost:%u/", port);
        if (!(ret = HttpAddUrl(queue, url, NULL)))
            return port;
        ok(ret == ERROR_SHARING_VIOLATION || ret == ERROR_ALREADY_EXISTS, "Failed to add %s, error %u.\n", debugstr_w(url), ret);
    }
    ok(0, "Failed to add url %s, error %u.\n", debugstr_w(url), ret);
    return 0;
}

static unsigned short add_url_v2(HTTP_URL_GROUP_ID group)
{
    unsigned short port;
    WCHAR url[50];
    int ret;

    for (port = 50010; port < 51000; ++port)
    {
        swprintf(url, ARRAY_SIZE(url), L"http://localhost:%u/", port);
        if (!(ret = pHttpAddUrlToUrlGroup(group, url, 0xdeadbeef, 0)))
            return port;
        ok(ret == ERROR_SHARING_VIOLATION, "Failed to add %s, error %u.\n", debugstr_w(url), ret);
    }
    ok(0, "Failed to add url %s, error %u.\n", debugstr_w(url), ret);
    return 0;
}

static ULONG remove_url_v1(HANDLE queue, unsigned short port)
{
    WCHAR url[50];
    swprintf(url, ARRAY_SIZE(url), L"http://localhost:%u/", port);
    return HttpRemoveUrl(queue, url);
}

static ULONG remove_url_v2(HTTP_URL_GROUP_ID group, unsigned short port)
{
    WCHAR url[50];
    swprintf(url, ARRAY_SIZE(url), L"http://localhost:%u/", port);
    return pHttpRemoveUrlFromUrlGroup(group, url, 0);
}

static void test_v1_server(void)
{
    char DECLSPEC_ALIGN(8) req_buffer[2048], response_buffer[2048];
    HTTP_REQUEST_V1 *req = (HTTP_REQUEST_V1 *)req_buffer;
    struct sockaddr_in sockaddr, *sin;
    HTTP_RESPONSE_V1 response = {};
    HANDLE queue, queue2;
    unsigned short port;
    char req_text[200];
    unsigned int i;
    OVERLAPPED ovl;
    DWORD ret_size;
    WCHAR url[50];
    int ret, len;
    SOCKET s;

    ovl.hEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
    memset(req_buffer, 0xcc, sizeof(req_buffer));

    ret = HttpCreateHttpHandle(NULL, 0);
    ok(ret == ERROR_INVALID_PARAMETER, "Unexpected error %u.\n", ret);

    /* Non-zero reserved parameter is accepted on XP/2k3. */
    queue = NULL;
    ret = HttpCreateHttpHandle(&queue, 0);
    ok(!ret, "Unexpected ret value %u.\n", ret);
    ok(!!queue, "Unexpected handle value %p.\n", queue);

    queue2 = NULL;
    ret = HttpCreateHttpHandle(&queue2, 0);
    ok(!ret, "Unexpected ret value %u.\n", ret);
    ok(queue2 && queue2 != queue, "Unexpected handle %p.\n", queue2);
    ret = CloseHandle(queue2);
    ok(ret, "Failed to close queue handle, error %lu.\n", GetLastError());

    ret_size = 0xdeadbeef;
    ret = HttpReceiveHttpRequest(NULL, HTTP_NULL_ID, 0, (HTTP_REQUEST *)req, sizeof(req_buffer), &ret_size, NULL);
    ok(ret == ERROR_INVALID_HANDLE, "Got error %u.\n", ret);
    ret = HttpReceiveHttpRequest(NULL, HTTP_NULL_ID, 0, (HTTP_REQUEST *)req, sizeof(req_buffer), NULL, &ovl);
    ok(ret == ERROR_INVALID_HANDLE, "Got error %u.\n", ret);
    ret = HttpReceiveHttpRequest(queue, 0xdeadbeef, 0, (HTTP_REQUEST *)req, sizeof(req_buffer), NULL, &ovl);
    ok(ret == ERROR_CONNECTION_INVALID, "Got error %u.\n", ret);
    ret = HttpReceiveHttpRequest(queue, HTTP_NULL_ID, 0, (HTTP_REQUEST *)req, sizeof(req_buffer), NULL, &ovl);
    ok(ret == ERROR_IO_PENDING, "Got error %u.\n", ret);

    SetLastError(0xdeadbeef);
    ret = GetOverlappedResult(queue, &ovl, &ret_size, FALSE);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_IO_INCOMPLETE, "Got error %lu.\n", GetLastError());

    ret = HttpAddUrl(NULL, L"http://localhost:50000/", NULL);
    ok(ret == ERROR_INVALID_HANDLE || ret == ERROR_INVALID_PARAMETER /* < Vista */, "Got error %u.\n", ret);
    ret = HttpAddUrl(queue, L"http://localhost:50000", NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "Got error %u.\n", ret);
    ret = HttpAddUrl(queue, L"localhost:50000", NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "Got error %u.\n", ret);
    ret = HttpAddUrl(queue, L"localhost:50000/", NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "Got error %u.\n", ret);
    ret = HttpAddUrl(queue, L"http://localhost/", NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "Got error %u.\n", ret);
    ret = HttpAddUrl(queue, L"http://localhost:/", NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "Got error %u.\n", ret);
    ret = HttpAddUrl(queue, L"http://localhost:0/", NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "Got error %u.\n", ret);
    port = add_url_v1(queue);
    swprintf(url, ARRAY_SIZE(url), L"http://localhost:%u/", port);
    ret = HttpAddUrl(queue, url, NULL);
    ok(ret == ERROR_ALREADY_EXISTS, "Got error %u.\n", ret);

    s = create_client_socket(port);
    len = sizeof(sockaddr);
    ret = getsockname(s, (struct sockaddr *)&sockaddr, &len);
    ok(ret == 0, "getsockname() failed, error %u.\n", WSAGetLastError());

    SetLastError(0xdeadbeef);
    ret = GetOverlappedResult(queue, &ovl, &ret_size, FALSE);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_IO_INCOMPLETE, "Got error %lu.\n", GetLastError());

    sprintf(req_text, simple_req, port);
    ret = send(s, req_text, strlen(req_text), 0);
    ok(ret == strlen(req_text), "send() returned %d.\n", ret);

    /* Various versions of Windows (observed on 64-bit Windows 8 and Windows 10
     * version 1507, but probably affecting others) suffer from a bug where the
     * kernel will report success before completely filling the buffer or
     * reporting the correct buffer size. Wait for a short interval to work
     * around this. */
    Sleep(100);

    ret = GetOverlappedResult(queue, &ovl, &ret_size, TRUE);
    ok(ret, "Got error %lu.\n", GetLastError());
    ok(ret_size > sizeof(*req), "Got size %lu.\n", ret_size);

    ok(!req->Flags, "Got flags %#lx.\n", req->Flags);
    ok(req->ConnectionId, "Expected nonzero connection ID.\n");
    ok(req->RequestId, "Expected nonzero connection ID.\n");
    ok(!req->UrlContext, "Got URL context %s.\n", wine_dbgstr_longlong(req->UrlContext));
    ok(req->Version.MajorVersion == 1, "Got major version %u.\n", req->Version.MajorVersion);
    ok(req->Version.MinorVersion == 1, "Got major version %u.\n", req->Version.MinorVersion);
    ok(req->Verb == HttpVerbGET, "Got verb %u.\n", req->Verb);
    ok(!req->UnknownVerbLength, "Got unknown verb length %u.\n", req->UnknownVerbLength);
    ok(req->RawUrlLength == 7, "Got raw URL length %u.\n", req->RawUrlLength);
    ok(!req->pUnknownVerb, "Got unknown verb %s.\n", req->pUnknownVerb);
    ok(!strcmp(req->pRawUrl, "/foobar"), "Got raw URL %s.\n", req->pRawUrl);
    ok(req->CookedUrl.FullUrlLength == 58, "Got full URL length %u.\n", req->CookedUrl.FullUrlLength);
    ok(req->CookedUrl.HostLength == 30, "Got host length %u.\n", req->CookedUrl.HostLength);
    ok(req->CookedUrl.AbsPathLength == 14, "Got absolute path length %u.\n", req->CookedUrl.AbsPathLength);
    ok(!req->CookedUrl.QueryStringLength, "Got query string length %u.\n", req->CookedUrl.QueryStringLength);
    swprintf(url, ARRAY_SIZE(url), L"http://localhost:%u/foobar", port);
    ok(!wcscmp(req->CookedUrl.pFullUrl, url), "Got full URL %s.\n", wine_dbgstr_w(req->CookedUrl.pFullUrl));
    ok(req->CookedUrl.pHost == req->CookedUrl.pFullUrl + 7, "Got host %s.\n", wine_dbgstr_w(req->CookedUrl.pHost));
    ok(req->CookedUrl.pAbsPath == req->CookedUrl.pFullUrl + 22,
            "Got absolute path %s.\n", wine_dbgstr_w(req->CookedUrl.pAbsPath));
    ok(!req->CookedUrl.pQueryString, "Got query string %s.\n", wine_dbgstr_w(req->CookedUrl.pQueryString));
    ok(!memcmp(req->Address.pRemoteAddress, &sockaddr, len), "Client addresses didn't match.\n");
    sin = (SOCKADDR_IN *)req->Address.pLocalAddress;
    ok(sin->sin_family == AF_INET, "Got family %u.\n", sin->sin_family);
    ok(ntohs(sin->sin_port) == port, "Got wrong port %u.\n", ntohs(sin->sin_port));
    ok(sin->sin_addr.S_un.S_addr == inet_addr("127.0.0.1"), "Got address %08lx.\n", sin->sin_addr.S_un.S_addr);
    ok(!req->Headers.UnknownHeaderCount, "Got %u unknown headers.\n", req->Headers.UnknownHeaderCount);
    ok(!req->Headers.pUnknownHeaders, "Got unknown headers %p.\n", req->Headers.pUnknownHeaders);
    for (i = 0; i < ARRAY_SIZE(req->Headers.KnownHeaders); ++i)
    {
        if (i == HttpHeaderConnection)
        {
            ok(req->Headers.KnownHeaders[i].RawValueLength == 10, "Got length %u.\n",
                    req->Headers.KnownHeaders[i].RawValueLength);
            ok(!strcmp(req->Headers.KnownHeaders[i].pRawValue, "keep-alive"),
                    "Got connection '%s'.\n", req->Headers.KnownHeaders[i].pRawValue);
        }
        else if (i == HttpHeaderHost)
        {
            char expect[16];
            sprintf(expect, "localhost:%u", port);
            ok(req->Headers.KnownHeaders[i].RawValueLength == strlen(expect), "Got length %u.\n",
                    req->Headers.KnownHeaders[i].RawValueLength);
            ok(!strcmp(req->Headers.KnownHeaders[i].pRawValue, expect),
                    "Got connection '%s'.\n", req->Headers.KnownHeaders[i].pRawValue);
        }
        else if (i == HttpHeaderUserAgent)
        {
            ok(req->Headers.KnownHeaders[i].RawValueLength == 4, "Got length %u.\n",
                    req->Headers.KnownHeaders[i].RawValueLength);
            ok(!strcmp(req->Headers.KnownHeaders[i].pRawValue, "WINE"),
                    "Got connection '%s'.\n", req->Headers.KnownHeaders[i].pRawValue);
        }
        else
        {
            ok(!req->Headers.KnownHeaders[i].RawValueLength, "Header %#x: got length %u.\n",
                    i, req->Headers.KnownHeaders[i].RawValueLength);
            ok(!req->Headers.KnownHeaders[i].pRawValue, "Header %#x: got value '%s'.\n",
                    i, req->Headers.KnownHeaders[i].pRawValue);
        }
    }
    ok(req->BytesReceived == strlen(req_text), "Got %s bytes.\n", wine_dbgstr_longlong(req->BytesReceived));
    ok(!req->EntityChunkCount, "Got %u entity chunks.\n", req->EntityChunkCount);
    ok(!req->pEntityChunks, "Got entity chunks %p.\n", req->pEntityChunks);
    /* req->RawConnectionId is zero until Win10 2004. */
    ok(!req->pSslInfo, "Got SSL info %p.\n", req->pSslInfo);

    response.StatusCode = 418;
    response.pReason = "I'm a teapot";
    response.ReasonLength = 12;
    response.Headers.KnownHeaders[HttpHeaderRetryAfter].pRawValue = "120";
    response.Headers.KnownHeaders[HttpHeaderRetryAfter].RawValueLength = 3;
    ret = HttpSendHttpResponse(queue, 0xdeadbeef, 0, (HTTP_RESPONSE *)&response, NULL, NULL, NULL, 0, &ovl, NULL);
    ok(ret == ERROR_CONNECTION_INVALID, "Got error %u.\n", ret);
    ret = HttpSendHttpResponse(queue, req->RequestId, 0, (HTTP_RESPONSE *)&response, NULL, NULL, NULL, 0, &ovl, NULL);
    ok(!ret, "Got error %u.\n", ret);
    ret = GetOverlappedResult(queue, &ovl, &ret_size, FALSE);
    ok(ret, "Got error %lu.\n", GetLastError());

    ret = recv(s, response_buffer, sizeof(response_buffer), 0);
    ok(ret == ret_size, "Expected size %lu, got %u.\n", ret_size, ret);

    if (winetest_debug > 1)
        trace("%.*s\n", ret, response_buffer);

    ok(!strncmp(response_buffer, "HTTP/1.1 418 I'm a teapot\r\n", 27), "Got incorrect status line.\n");
    ok(!!strstr(response_buffer, "\r\nRetry-After: 120\r\n"), "Missing or malformed Retry-After header.\n");
    ok(!!strstr(response_buffer, "\r\nDate:"), "Missing Date header.\n");

    ret = HttpReceiveHttpRequest(queue, req->RequestId, 0, (HTTP_REQUEST *)req, sizeof(req_buffer), NULL, &ovl);
    ok(ret == ERROR_CONNECTION_INVALID, "Got error %u.\n", ret);

    /* HttpReceiveHttpRequest() may return synchronously, but this cannot be
     * reliably tested. Introducing a delay after send() and before
     * HttpReceiveHttpRequest() confirms this. */

    ret = remove_url_v1(NULL, port);
    ok(ret == ERROR_INVALID_PARAMETER, "Got error %u.\n", ret);
    ret = remove_url_v1(queue, port);
    ok(!ret, "Got error %u.\n", ret);
    ret = remove_url_v1(queue, port);
    ok(ret == ERROR_FILE_NOT_FOUND, "Got error %u.\n", ret);

    ret = HttpReceiveHttpRequest(queue, HTTP_NULL_ID, 0, (HTTP_REQUEST *)req, sizeof(req_buffer), NULL, &ovl);
    ok(ret == ERROR_IO_PENDING, "Got error %u.\n", ret);

    ret = CancelIo(queue);
    ok(ret, "Failed to close queue handle, error %lu.\n", GetLastError());

    ret = WaitForSingleObject(ovl.hEvent, 100);
    ok(!ret, "Got %u.\n", ret);
    ret_size = 0xdeadbeef;
    ret = GetOverlappedResult(queue, &ovl, &ret_size, FALSE);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_OPERATION_ABORTED, "Got error %lu.\n", GetLastError());
    ok(!ret_size, "Got size %lu.\n", ret_size);

    closesocket(s);
    CloseHandle(ovl.hEvent);
    ret = CloseHandle(queue);
    ok(ret, "Failed to close queue handle, error %lu.\n", GetLastError());

    ret = HttpAddUrl(queue, L"http://localhost:50000/", NULL);
    ok(ret == ERROR_INVALID_HANDLE, "Got error %u.\n", ret);
}

static void test_v1_completion_port(void)
{
    char DECLSPEC_ALIGN(8) req_buffer[2048], response_buffer[2048];
    HTTP_REQUEST_V1 *req = (HTTP_REQUEST_V1 *)req_buffer;
    HTTP_RESPONSE_V1 response = {};
    unsigned short tcp_port;
    OVERLAPPED ovl, *povl;
    HANDLE queue, port;
    char req_text[200];
    DWORD ret_size;
    ULONG_PTR key;
    SOCKET s;
    int ret;

    ovl.hEvent = CreateEventA(NULL, TRUE, FALSE, NULL);

    ret = HttpCreateHttpHandle(&queue, 0);
    ok(!ret, "Got error %u.\n", ret);

    port = CreateIoCompletionPort(queue, NULL, 123, 0);
    ok(!!port, "Failed to create completion port, error %lu.\n", GetLastError());

    ret = GetQueuedCompletionStatus(port, &ret_size, &key, &povl, 0);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == WAIT_TIMEOUT, "Got error %lu.\n", GetLastError());

    ret = HttpReceiveHttpRequest(queue, HTTP_NULL_ID, 0, (HTTP_REQUEST *)req, sizeof(req_buffer), NULL, &ovl);
    ok(ret == ERROR_IO_PENDING, "Got error %u.\n", ret);

    tcp_port = add_url_v1(queue);
    s = create_client_socket(tcp_port);

    ret = GetQueuedCompletionStatus(port, &ret_size, &key, &povl, 0);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == WAIT_TIMEOUT, "Got error %lu.\n", GetLastError());

    sprintf(req_text, simple_req, tcp_port);
    ret = send(s, req_text, strlen(req_text), 0);
    ok(ret == strlen(req_text), "send() returned %d.\n", ret);

    ret_size = key = 0xdeadbeef;
    ret = GetQueuedCompletionStatus(port, &ret_size, &key, &povl, 1000);
    ok(ret, "Got error %lu.\n", GetLastError());
    ok(povl == &ovl, "OVERLAPPED pointers didn't match.\n");
    ok(key == 123, "Got unexpected key %Iu.\n", key);
    ok(ret_size > sizeof(*req), "Got size %lu.\n", ret_size);

    ret = GetQueuedCompletionStatus(port, &ret_size, &key, &povl, 0);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == WAIT_TIMEOUT, "Got error %lu.\n", GetLastError());

    response.StatusCode = 418;
    response.pReason = "I'm a teapot";
    response.ReasonLength = 12;
    ret = HttpSendHttpResponse(queue, req->RequestId, 0, (HTTP_RESPONSE *)&response, NULL, NULL, NULL, 0, &ovl, NULL);
    ok(!ret, "Got error %u.\n", ret);

    ret_size = key = 0xdeadbeef;
    ret = GetQueuedCompletionStatus(port, &ret_size, &key, &povl, 1000);
    ok(ret, "Got error %lu.\n", GetLastError());
    ok(povl == &ovl, "OVERLAPPED pointers didn't match.\n");
    ok(key == 123, "Got unexpected key %Iu.\n", key);

    ret = recv(s, response_buffer, sizeof(response_buffer), 0);
    ok(ret == ret_size, "Expected size %lu, got %u.\n", ret_size, ret);

    ret = remove_url_v1(queue, tcp_port);
    ok(!ret, "Got error %u.\n", ret);
    closesocket(s);
    CloseHandle(port);
    CloseHandle(ovl.hEvent);
    ret = CloseHandle(queue);
    ok(ret, "Failed to close queue handle, error %lu.\n", GetLastError());
}

static void test_v1_multiple_requests(void)
{
    char DECLSPEC_ALIGN(8) req_buffer1[2048];
    char DECLSPEC_ALIGN(8) req_buffer2[2048];
    HTTP_REQUEST_V1 *req1 = (HTTP_REQUEST_V1 *)req_buffer1, *req2 = (HTTP_REQUEST_V1 *)req_buffer2;
    HTTP_RESPONSE_V1 response = {};
    struct sockaddr_in sockaddr;
    OVERLAPPED ovl1, ovl2;
    unsigned short port;
    char req_text[200];
    DWORD ret_size;
    SOCKET s1, s2;
    HANDLE queue;
    int ret, len;

    ovl1.hEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
    ovl2.hEvent = CreateEventA(NULL, TRUE, FALSE, NULL);

    ret = HttpCreateHttpHandle(&queue, 0);
    ok(!ret, "Got error %u.\n", ret);
    port = add_url_v1(queue);

    ret = HttpReceiveHttpRequest(queue, HTTP_NULL_ID, 0, (HTTP_REQUEST *)req1, sizeof(req_buffer1), NULL, &ovl1);
    ok(ret == ERROR_IO_PENDING, "Got error %u.\n", ret);
    ret = HttpReceiveHttpRequest(queue, HTTP_NULL_ID, 0, (HTTP_REQUEST *)req2, sizeof(req_buffer2), NULL, &ovl2);
    ok(ret == ERROR_IO_PENDING, "Got error %u.\n", ret);

    SetLastError(0xdeadbeef);
    ret = GetOverlappedResult(queue, &ovl1, &ret_size, FALSE);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_IO_INCOMPLETE, "Got error %lu.\n", GetLastError());

    s1 = create_client_socket(port);
    sprintf(req_text, simple_req, port);
    ret = send(s1, req_text, strlen(req_text), 0);
    ok(ret == strlen(req_text), "send() returned %d.\n", ret);

    ret = WaitForSingleObject(ovl1.hEvent, 100);
    ok(!ret, "Got %u.\n", ret);
    ret = WaitForSingleObject(ovl2.hEvent, 100);
    ok(ret == WAIT_TIMEOUT, "Got %u.\n", ret);

    s2 = create_client_socket(port);
    ret = send(s2, req_text, strlen(req_text), 0);
    ok(ret == strlen(req_text), "send() returned %d.\n", ret);

    ret = WaitForSingleObject(ovl1.hEvent, 0);
    ok(!ret, "Got %u.\n", ret);
    ret = WaitForSingleObject(ovl2.hEvent, 100);
    ok(!ret, "Got %u.\n", ret);

    len = sizeof(sockaddr);
    getsockname(s1, (struct sockaddr *)&sockaddr, &len);
    ok(!memcmp(req1->Address.pRemoteAddress, &sockaddr, len), "Client addresses didn't match.\n");
    len = sizeof(sockaddr);
    getsockname(s2, (struct sockaddr *)&sockaddr, &len);
    ok(!memcmp(req2->Address.pRemoteAddress, &sockaddr, len), "Client addresses didn't match.\n");
    ok(req1->ConnectionId != req2->ConnectionId,
            "Expected different connection IDs, but got %s.\n", wine_dbgstr_longlong(req1->ConnectionId));
    ok(req1->RequestId != req2->RequestId,
            "Expected different request IDs, but got %s.\n", wine_dbgstr_longlong(req1->RequestId));

    response.StatusCode = 418;
    response.pReason = "I'm a teapot";
    response.ReasonLength = 12;
    ret = HttpSendHttpResponse(queue, req1->RequestId, 0, (HTTP_RESPONSE *)&response, NULL, NULL, NULL, 0, &ovl1, NULL);
    ok(!ret, "Got error %u.\n", ret);
    ret = HttpSendHttpResponse(queue, req2->RequestId, 0, (HTTP_RESPONSE *)&response, NULL, NULL, NULL, 0, &ovl2, NULL);
    ok(!ret, "Got error %u.\n", ret);

    /* Test sending multiple requests from the same socket. */

    ret = HttpReceiveHttpRequest(queue, HTTP_NULL_ID, 0, (HTTP_REQUEST *)req1, sizeof(req_buffer1), NULL, &ovl1);
    ok(ret == ERROR_IO_PENDING, "Got error %u.\n", ret);
    ret = HttpReceiveHttpRequest(queue, HTTP_NULL_ID, 0, (HTTP_REQUEST *)req2, sizeof(req_buffer2), NULL, &ovl2);
    ok(ret == ERROR_IO_PENDING, "Got error %u.\n", ret);

    ret = send(s1, req_text, strlen(req_text), 0);
    ok(ret == strlen(req_text), "send() returned %d.\n", ret);
    ret = send(s1, req_text, strlen(req_text), 0);
    ok(ret == strlen(req_text), "send() returned %d.\n", ret);

    ret = WaitForSingleObject(ovl1.hEvent, 100);
    ok(!ret, "Got %u.\n", ret);
    ret = WaitForSingleObject(ovl2.hEvent, 100);
    ok(ret == WAIT_TIMEOUT, "Got %u.\n", ret);

    ret = HttpSendHttpResponse(queue, req1->RequestId, 0, (HTTP_RESPONSE *)&response, NULL, NULL, NULL, 0, &ovl1, NULL);
    ok(!ret, "Got error %u.\n", ret);

    ret = WaitForSingleObject(ovl2.hEvent, 100);
    ok(!ret, "Got %u.\n", ret);
    ok(req1->ConnectionId == req2->ConnectionId, "Expected same connection IDs.\n");
    ok(req1->RequestId != req2->RequestId,
            "Expected different request IDs, but got %s.\n", wine_dbgstr_longlong(req1->RequestId));

    ret = remove_url_v1(queue, port);
    ok(!ret, "Got error %u.\n", ret);
    closesocket(s1);
    closesocket(s2);
    CloseHandle(ovl1.hEvent);
    CloseHandle(ovl2.hEvent);
    ret = CloseHandle(queue);
    ok(ret, "Failed to close queue handle, error %lu.\n", GetLastError());
}

static void test_v1_short_buffer(void)
{
    char DECLSPEC_ALIGN(8) req_buffer[2048];
    char DECLSPEC_ALIGN(8) req_buffer2[2048];
    HTTP_REQUEST_V1 *req = (HTTP_REQUEST_V1 *)req_buffer, *req2 = (HTTP_REQUEST_V1 *)req_buffer2;
    HTTP_REQUEST_ID req_id;
    unsigned short port;
    char req_text[200];
    OVERLAPPED ovl;
    DWORD ret_size;
    HANDLE queue;
    SOCKET s;
    int ret;

    ovl.hEvent = CreateEventA(NULL, TRUE, FALSE, NULL);

    ret = HttpCreateHttpHandle(&queue, 0);
    ok(!ret, "Got error %u.\n", ret);
    port = add_url_v1(queue);

    s = create_client_socket(port);
    sprintf(req_text, simple_req, port);
    ret = send(s, req_text, strlen(req_text), 0);
    ok(ret == strlen(req_text), "send() returned %d.\n", ret);

    memset(req_buffer, 0xcc, sizeof(req_buffer));

    ret_size = 1;
    ret = HttpReceiveHttpRequest(queue, HTTP_NULL_ID, 0, (HTTP_REQUEST *)req, sizeof(HTTP_REQUEST_V1) - 1, &ret_size, NULL);
    ok(ret == ERROR_INSUFFICIENT_BUFFER, "Got error %u.\n", ret);
    ok(ret_size == 1 /* win < 10 */ || !ret_size, "Got size %lu.\n", ret_size);

    ret_size = 1;
    ret = HttpReceiveHttpRequest(queue, HTTP_NULL_ID, 0, (HTTP_REQUEST *)req, sizeof(HTTP_REQUEST_V1), &ret_size, NULL);
    ok(ret == ERROR_MORE_DATA, "Got error %u.\n", ret);
    ok(ret_size > sizeof(*req), "Got size %lu.\n", ret_size);

    ok(!!req->ConnectionId, "Got connection ID %s.\n", wine_dbgstr_longlong(req->ConnectionId));
    ok(!!req->RequestId, "Got request ID %s.\n", wine_dbgstr_longlong(req->RequestId));
    ok(!req->Version.MajorVersion || req->Version.MajorVersion == 0xcccc /* < Vista */,
            "Got major version %u.\n", req->Version.MajorVersion);
    ok(!req->BytesReceived || req->BytesReceived == ((ULONGLONG)0xcccccccc << 32 | 0xcccccccc) /* < Vista */,
            "Got %s bytes.\n", wine_dbgstr_longlong(req->BytesReceived));
    req_id = req->RequestId;

    /* At this point the request has been assigned a specific ID, and one cannot
     * receive it by calling with HTTP_NULL_ID. */
    ret = HttpReceiveHttpRequest(queue, HTTP_NULL_ID, 0, (HTTP_REQUEST *)req2, sizeof(req_buffer2), NULL, &ovl);
    ok(ret == ERROR_IO_PENDING, "Got error %u.\n", ret);

    memset(req_buffer, 0xcc, sizeof(req_buffer));
    ret = HttpReceiveHttpRequest(queue, req_id, 0, (HTTP_REQUEST *)req, ret_size - 1, &ret_size, NULL);
    ok(ret == ERROR_MORE_DATA, "Got error %u.\n", ret);
    ok(req->RequestId == req_id, "Got request ID %s.\n", wine_dbgstr_longlong(req->RequestId));

    memset(req_buffer, 0xcc, sizeof(req_buffer));
    ret = HttpReceiveHttpRequest(queue, req_id, 0, (HTTP_REQUEST *)req, ret_size - 1, NULL, NULL);
    ok(ret == ERROR_MORE_DATA, "Got error %u.\n", ret);
    ok(req->RequestId == req_id, "Got request ID %s.\n", wine_dbgstr_longlong(req->RequestId));

    memset(req_buffer, 0xcc, sizeof(req_buffer));
    ret = HttpReceiveHttpRequest(queue, req_id, 0, (HTTP_REQUEST *)req, ret_size, &ret_size, NULL);
    ok(!ret, "Got error %u.\n", ret);
    ok(req->RequestId == req_id, "Got request ID %s.\n", wine_dbgstr_longlong(req->RequestId));

    CancelIo(queue);

    ret = remove_url_v1(queue, port);
    ok(!ret, "Got error %u.\n", ret);
    closesocket(s);
    CloseHandle(ovl.hEvent);
    ret = CloseHandle(queue);
    ok(ret, "Failed to close queue handle, error %lu.\n", GetLastError());
}

static void test_v1_entity_body(void)
{
    char DECLSPEC_ALIGN(8) req_buffer[4096], response_buffer[2048], req_body[2048], recv_body[2000];
    HTTP_REQUEST_V1 *req = (HTTP_REQUEST_V1 *)req_buffer;
    HTTP_RESPONSE_V1 response = {};
    HTTP_DATA_CHUNK chunks[2] = {};
    unsigned short port;
    int ret, chunk_size, len, max_attempts;
    char req_text[200];
    unsigned int i;
    OVERLAPPED ovl;
    DWORD ret_size;
    HANDLE queue;
    SOCKET s;

    static const char post_req[] =
        "POST /xyzzy HTTP/1.1\r\n"
        "Host: localhost:%u\r\n"
        "Connection: keep-alive\r\n"
        "Content-Length: 5\r\n"
        "\r\n"
        "ping";

    static const char post_req2[] =
        "POST /xyzzy HTTP/1.1\r\n"
        "Host: localhost:%u\r\n"
        "Connection: keep-alive\r\n"
        "Content-Length: 2048\r\n"
        "\r\n";

    ovl.hEvent = CreateEventA(NULL, TRUE, FALSE, NULL);

    for (i = 0; i < sizeof(req_body); ++i)
        req_body[i] = i / 111;

    ret = HttpCreateHttpHandle(&queue, 0);
    ok(!ret, "Got error %u.\n", ret);
    port = add_url_v1(queue);

    s = create_client_socket(port);
    sprintf(req_text, post_req, port);
    ret = send(s, req_text, strlen(req_text) + 1, 0);
    ok(ret == strlen(req_text) + 1, "send() returned %d.\n", ret);
    /* Windows versions before 8 will return success, and report that an entity
     * body exists in the Flags member, but fail to account for it in the
     * BytesReceived member or actually copy it to the buffer, if
     * HttpReceiveHttpRequest() is called before the kernel has finished
     * receiving the entity body. Add a small delay to work around this. */
    Sleep(100);

    memset(req_buffer, 0xcc, sizeof(req_buffer));
    ret = HttpReceiveHttpRequest(queue, HTTP_NULL_ID, 0, (HTTP_REQUEST *)req, sizeof(req_buffer), &ret_size, NULL);
    ok(!ret, "Got error %u.\n", ret);
    ok(ret_size > sizeof(*req), "Got size %lu.\n", ret_size);
    ok(req->Flags == HTTP_REQUEST_FLAG_MORE_ENTITY_BODY_EXISTS, "Got flags %#lx.\n", req->Flags);
    ok(req->BytesReceived == strlen(req_text) + 1, "Got %s bytes.\n", wine_dbgstr_longlong(req->BytesReceived));
    ok(req->Headers.KnownHeaders[HttpHeaderContentLength].RawValueLength == 1,
            "Got header length %u.\n", req->Headers.KnownHeaders[HttpHeaderContentLength].RawValueLength);
    ok(!strcmp(req->Headers.KnownHeaders[HttpHeaderContentLength].pRawValue, "5"),
            "Got header value %s.\n", req->Headers.KnownHeaders[HttpHeaderContentLength].pRawValue);
    ok(!req->EntityChunkCount, "Got %u entity chunks.\n", req->EntityChunkCount);
    ok(!req->pEntityChunks, "Got entity chunks %p.\n", req->pEntityChunks);

    response.StatusCode = 418;
    response.pReason = "I'm a teapot";
    response.ReasonLength = 12;
    response.EntityChunkCount = ARRAY_SIZE(chunks);
    response.pEntityChunks = chunks;
    chunks[0].DataChunkType = HttpDataChunkFromMemory;
    chunks[0].FromMemory.pBuffer = (void *)"pong";
    chunks[0].FromMemory.BufferLength = 4;
    chunks[1].DataChunkType = HttpDataChunkFromMemory;
    chunks[1].FromMemory.pBuffer = (void *)"pang";
    chunks[1].FromMemory.BufferLength = 4;
    ret = HttpSendHttpResponse(queue, req->RequestId, 0, (HTTP_RESPONSE *)&response, NULL, NULL, NULL, 0, NULL, NULL);
    ok(!ret, "Got error %u.\n", ret);

    memset(response_buffer, 0, sizeof(response_buffer));

    ret = recv(s, response_buffer, sizeof(response_buffer), 0);
    ok(ret > 0, "recv() failed.\n");
    if (winetest_debug > 1)
        trace("%.*s\n", ret, response_buffer);
    ok(!strncmp(response_buffer, "HTTP/1.1 418 I'm a teapot\r\n", 27), "Got incorrect status line.\n");
    ok(!!strstr(response_buffer, "\r\nContent-Length: 8\r\n"), "Missing or malformed Content-Length header.\n");
    ok(!!strstr(response_buffer, "\r\nDate:"), "Missing Date header.\n");
    ok(!memcmp(response_buffer + ret - 12, "\r\n\r\npongpang", 12), "Response did not end with entity data.\n");

    ret = HttpReceiveHttpRequest(queue, req->RequestId, 0, (HTTP_REQUEST *)req, sizeof(req_buffer), &ret_size, NULL);
    ok(ret == ERROR_CONNECTION_INVALID, "Got error %u.\n", ret);

    /* http won't overwrite a Content-Length header if we manually supply one,
     * but it also won't truncate the entity body to match. It will however
     * always write its own Date header. */

    ret = send(s, req_text, strlen(req_text) + 1, 0);
    ok(ret == strlen(req_text) + 1, "send() returned %d.\n", ret);
    ret = HttpReceiveHttpRequest(queue, HTTP_NULL_ID, 0, (HTTP_REQUEST *)req, sizeof(req_buffer), &ret_size, NULL);
    ok(!ret, "Got error %u.\n", ret);

    response.Headers.KnownHeaders[HttpHeaderContentLength].RawValueLength = 1;
    response.Headers.KnownHeaders[HttpHeaderContentLength].pRawValue = "6";
    response.Headers.KnownHeaders[HttpHeaderDate].RawValueLength = 10;
    response.Headers.KnownHeaders[HttpHeaderDate].pRawValue = "yesteryear";
    ret = HttpSendHttpResponse(queue, req->RequestId, 0, (HTTP_RESPONSE *)&response, NULL, NULL, NULL, 0, NULL, NULL);
    ok(!ret, "Got error %u.\n", ret);

    ret = recv(s, response_buffer, sizeof(response_buffer), 0);
    ok(ret > 0, "recv() failed.\n");
    if (winetest_debug > 1)
        trace("%.*s\n", ret, response_buffer);
    ok(!strncmp(response_buffer, "HTTP/1.1 418 I'm a teapot\r\n", 27), "Got incorrect status line.\n");
    ok(!!strstr(response_buffer, "\r\nContent-Length: 6\r\n"), "Missing or malformed Content-Length header.\n");
    ok(!!strstr(response_buffer, "\r\nDate:"), "Missing Date header.\n");
    ok(!strstr(response_buffer, "yesteryear"), "Unexpected Date value.\n");
    ok(!memcmp(response_buffer + ret - 12, "\r\n\r\npongpang", 12), "Response did not end with entity data.\n");

    /* Test the COPY_BODY flag. */

    ret = send(s, req_text, strlen(req_text) + 1, 0);
    ok(ret == strlen(req_text) + 1, "send() returned %d.\n", ret);
    Sleep(100);

    ret = HttpReceiveHttpRequest(queue, HTTP_NULL_ID, HTTP_RECEIVE_REQUEST_FLAG_COPY_BODY,
            (HTTP_REQUEST *)req, sizeof(req_buffer), &ret_size, NULL);
    ok(!ret, "Got error %u.\n", ret);
    ok(ret_size > sizeof(*req), "Got size %lu.\n", ret_size);
    ok(!req->Flags, "Got flags %#lx.\n", req->Flags);
    ok(req->BytesReceived == strlen(req_text) + 1, "Got %s bytes.\n", wine_dbgstr_longlong(req->BytesReceived));
    ok(req->Headers.KnownHeaders[HttpHeaderContentLength].RawValueLength == 1,
            "Got header length %u.\n", req->Headers.KnownHeaders[HttpHeaderContentLength].RawValueLength);
    ok(!strcmp(req->Headers.KnownHeaders[HttpHeaderContentLength].pRawValue, "5"),
            "Got header value %s.\n", req->Headers.KnownHeaders[HttpHeaderContentLength].pRawValue);
    ok(req->EntityChunkCount == 1, "Got %u entity chunks.\n", req->EntityChunkCount);
    ok(req->pEntityChunks[0].DataChunkType == HttpDataChunkFromMemory,
            "Got chunk type %u.\n", req->pEntityChunks[0].DataChunkType);
    ok(req->pEntityChunks[0].FromMemory.BufferLength == 5,
            "Got chunk length %lu.\n", req->pEntityChunks[0].FromMemory.BufferLength);
    ok(!memcmp(req->pEntityChunks[0].FromMemory.pBuffer, "ping", 5),
            "Got chunk data '%s'.\n", (char *)req->pEntityChunks[0].FromMemory.pBuffer);

    send_response_v1(queue, req->RequestId, s);

    sprintf(req_text, post_req2, port);
    ret = send(s, req_text, strlen(req_text), 0);
    ok(ret == strlen(req_text), "send() returned %d.\n", ret);
    ret = send(s, req_body, sizeof(req_body), 0);
    ok(ret == sizeof(req_body), "send() returned %d.\n", ret);

    Sleep(100);

    ret = HttpReceiveHttpRequest(queue, HTTP_NULL_ID, HTTP_RECEIVE_REQUEST_FLAG_COPY_BODY,
            (HTTP_REQUEST *)req, 2000, &ret_size, NULL);
    ok(!ret, "Got error %u.\n", ret);
    ok(ret_size == 2000, "Got size %lu.\n", ret_size);
    ok(req->Flags == HTTP_REQUEST_FLAG_MORE_ENTITY_BODY_EXISTS, "Got flags %#lx.\n", req->Flags);
    ok(req->BytesReceived == strlen(req_text) + 2048, "Got %s bytes.\n", wine_dbgstr_longlong(req->BytesReceived));
    ok(req->Headers.KnownHeaders[HttpHeaderContentLength].RawValueLength == 4,
            "Got header length %u.\n", req->Headers.KnownHeaders[HttpHeaderContentLength].RawValueLength);
    ok(!strcmp(req->Headers.KnownHeaders[HttpHeaderContentLength].pRawValue, "2048"),
            "Got header value %s.\n", req->Headers.KnownHeaders[HttpHeaderContentLength].pRawValue);
    ok(req->EntityChunkCount == 1, "Got %u entity chunks.\n", req->EntityChunkCount);
    ok(req->pEntityChunks[0].DataChunkType == HttpDataChunkFromMemory,
            "Got chunk type %u.\n", req->pEntityChunks[0].DataChunkType);
    chunk_size = req->pEntityChunks[0].FromMemory.BufferLength;
    ok(chunk_size > 0 && chunk_size < 2000, "Got chunk size %u.\n", chunk_size);
    ok(!memcmp(req->pEntityChunks[0].FromMemory.pBuffer, req_body, chunk_size), "Chunk data didn't match.\n");

    send_response_v1(queue, req->RequestId, s);

    /* Test HttpReceiveRequestEntityBody(). */

    ret_size = 0xdeadbeef;
    ret = HttpReceiveRequestEntityBody(NULL, HTTP_NULL_ID, 0, recv_body, sizeof(recv_body), &ret_size, NULL);
    ok(ret == ERROR_INVALID_HANDLE, "Got error %u.\n", ret);
    ret = HttpReceiveRequestEntityBody(NULL, HTTP_NULL_ID, 0, recv_body, sizeof(recv_body), NULL, &ovl);
    ok(ret == ERROR_INVALID_HANDLE, "Got error %u.\n", ret);

    sprintf(req_text, post_req, port);
    ret = send(s, req_text, strlen(req_text) + 1, 0);
    ok(ret == strlen(req_text) + 1, "send() returned %d.\n", ret);
    Sleep(100);

    ret = HttpReceiveHttpRequest(queue, HTTP_NULL_ID, 0, (HTTP_REQUEST *)req, sizeof(req_buffer), &ret_size, NULL);
    ok(!ret, "Got error %u.\n", ret);

    ret = HttpReceiveRequestEntityBody(queue, HTTP_NULL_ID, 0, recv_body, sizeof(recv_body), &ret_size, NULL);
    ok(ret == ERROR_CONNECTION_INVALID, "Got error %u.\n", ret);

    ret_size = 0xdeadbeef;
    ret = HttpReceiveRequestEntityBody(queue, req->RequestId, 0, recv_body, sizeof(recv_body), &ret_size, NULL);
    ok(!ret, "Got error %u.\n", ret);
    ok(ret_size == 5, "Got size %lu.\n", ret_size);
    ok(!memcmp(recv_body, "ping", 5), "Entity body didn't match.\n");

    ret_size = 0xdeadbeef;
    ret = HttpReceiveRequestEntityBody(queue, req->RequestId, 0, recv_body, sizeof(recv_body), &ret_size, NULL);
    ok(ret == ERROR_HANDLE_EOF, "Got error %u.\n", ret);
    ok(ret_size == 0xdeadbeef || !ret_size /* Win10+ */, "Got size %lu.\n", ret_size);

    send_response_v1(queue, req->RequestId, s);

    ret = HttpReceiveRequestEntityBody(queue, req->RequestId, 0, recv_body, sizeof(recv_body), &ret_size, NULL);
    ok(ret == ERROR_CONNECTION_INVALID, "Got error %u.\n", ret);

    ret = send(s, req_text, strlen(req_text) + 1, 0);
    ok(ret == strlen(req_text) + 1, "send() returned %d.\n", ret);
    Sleep(100);

    ret = HttpReceiveHttpRequest(queue, HTTP_NULL_ID, 0, (HTTP_REQUEST *)req, sizeof(req_buffer), &ret_size, NULL);
    ok(!ret, "Got error %u.\n", ret);

    memset(recv_body, 0xcc, sizeof(recv_body));
    ret_size = 0xdeadbeef;
    ret = HttpReceiveRequestEntityBody(queue, req->RequestId, 0, recv_body, 2, &ret_size, NULL);
    ok(!ret, "Got error %u.\n", ret);
    ok(ret_size == 2, "Got size %lu.\n", ret_size);
    ok(!memcmp(recv_body, "pi", 2), "Entity body didn't match.\n");

    ret = HttpReceiveRequestEntityBody(queue, req->RequestId, 0, recv_body, 1, NULL, NULL);
    ok(!ret, "Got error %u.\n", ret);
    ok(!memcmp(recv_body, "n", 1), "Entity body didn't match.\n");

    ret_size = 0xdeadbeef;
    ret = HttpReceiveRequestEntityBody(queue, req->RequestId, 0, recv_body, 4, &ret_size, NULL);
    ok(!ret, "Got error %u.\n", ret);
    ok(ret_size == 2, "Got size %lu.\n", ret_size);
    ok(!memcmp(recv_body, "g", 2), "Entity body didn't match.\n");

    ret_size = 0xdeadbeef;
    ret = HttpReceiveRequestEntityBody(queue, req->RequestId, 0, recv_body, sizeof(recv_body), &ret_size, NULL);
    ok(ret == ERROR_HANDLE_EOF, "Got error %u.\n", ret);
    ok(ret_size == 0xdeadbeef || !ret_size /* Win10+ */, "Got size %lu.\n", ret_size);

    send_response_v1(queue, req->RequestId, s);

    ret = send(s, req_text, strlen(req_text) + 1, 0);
    ok(ret == strlen(req_text) + 1, "send() returned %d.\n", ret);
    Sleep(100);

    ret = HttpReceiveHttpRequest(queue, HTTP_NULL_ID, 0, (HTTP_REQUEST *)req, sizeof(req_buffer), &ret_size, NULL);
    ok(!ret, "Got error %u.\n", ret);

    memset(recv_body, 0xcc, sizeof(recv_body));
    ret_size = 0xdeadbeef;
    ret = HttpReceiveRequestEntityBody(queue, req->RequestId, 0, recv_body, sizeof(recv_body), NULL, &ovl);
    ok(!ret || ret == ERROR_IO_PENDING, "Got error %u.\n", ret);
    ret = GetOverlappedResult(queue, &ovl, &ret_size, TRUE);
    ok(ret, "Got error %lu.\n", GetLastError());
    ok(ret_size == 5, "Got size %lu.\n", ret_size);
    ok(!memcmp(recv_body, "ping", 5), "Entity body didn't match.\n");

    ret = HttpReceiveRequestEntityBody(queue, req->RequestId, 0, recv_body, sizeof(recv_body), NULL, &ovl);
    ok(ret == ERROR_HANDLE_EOF, "Got error %u.\n", ret);

    send_response_v1(queue, req->RequestId, s);

    ret = send(s, req_text, strlen(req_text) + 1, 0);
    ok(ret == strlen(req_text) + 1, "send() returned %d.\n", ret);
    Sleep(100);

    ret = HttpReceiveHttpRequest(queue, HTTP_NULL_ID, HTTP_RECEIVE_REQUEST_FLAG_COPY_BODY,
            (HTTP_REQUEST *)req, sizeof(req_buffer), &ret_size, NULL);
    ok(!ret, "Got error %u.\n", ret);

    ret = HttpReceiveRequestEntityBody(queue, req->RequestId, 0, recv_body, sizeof(recv_body), NULL, &ovl);
    ok(ret == ERROR_HANDLE_EOF, "Got error %u.\n", ret);

    send_response_v1(queue, req->RequestId, s);

    sprintf(req_text, post_req2, port);
    ret = send(s, req_text, strlen(req_text), 0);
    ok(ret == strlen(req_text), "send() returned %d.\n", ret);
    ret = send(s, req_body, sizeof(req_body), 0);
    ok(ret == sizeof(req_body), "send() returned %d.\n", ret);

    Sleep(100);

    ret = HttpReceiveHttpRequest(queue, HTTP_NULL_ID, HTTP_RECEIVE_REQUEST_FLAG_COPY_BODY,
            (HTTP_REQUEST *)req, 2000, &ret_size, NULL);
    ok(!ret, "Got error %u.\n", ret);
    ok(ret_size == 2000, "Got size %lu.\n", ret_size);
    ok(req->Flags == HTTP_REQUEST_FLAG_MORE_ENTITY_BODY_EXISTS, "Got flags %#lx.\n", req->Flags);
    chunk_size = req->pEntityChunks[0].FromMemory.BufferLength;

    memset(recv_body, 0xcc, sizeof(recv_body));
    ret_size = 0xdeadbeef;
    ret = HttpReceiveRequestEntityBody(queue, req->RequestId, 0, recv_body, sizeof(recv_body), &ret_size, NULL);
    ok(!ret, "Got error %u.\n", ret);
    ok(ret_size == 2048 - chunk_size, "Got size %lu.\n", ret_size);
    ok(!memcmp(recv_body, req_body + chunk_size, ret_size), "Entity body didn't match.\n");

    send_response_v1(queue, req->RequestId, s);

    /* Test HttpSendResponseEntityBody(). */

    ret = HttpSendResponseEntityBody(queue, HTTP_NULL_ID, 0, ARRAY_SIZE(chunks), chunks, NULL, NULL, 0, NULL, NULL);
    ok(ret == ERROR_CONNECTION_INVALID, "Got error %u.\n", ret);

    sprintf(req_text, post_req, port);
    ret = send(s, req_text, strlen(req_text) + 1, 0);
    ok(ret == strlen(req_text) + 1, "send() returned %d.\n", ret);

    Sleep(100);

    memset(req_buffer, 0xcc, sizeof(req_buffer));
    ret = HttpReceiveHttpRequest(queue, HTTP_NULL_ID, 0, (HTTP_REQUEST *)req, sizeof(req_buffer), &ret_size, NULL);
    ok(!ret, "Got error %u.\n", ret);
    ok(ret_size > sizeof(*req), "Got size %lu.\n", ret_size);

    memset(&response, 0, sizeof(response));

    response.StatusCode = 418;
    response.pReason = "I'm a teapot";
    response.ReasonLength = 12;
    response.EntityChunkCount = ARRAY_SIZE(chunks);
    response.pEntityChunks = chunks;
    chunks[0].DataChunkType = HttpDataChunkFromMemory;
    chunks[0].FromMemory.pBuffer = (void *)"pong";
    chunks[0].FromMemory.BufferLength = 4;
    chunks[1].DataChunkType = HttpDataChunkFromMemory;
    chunks[1].FromMemory.pBuffer = (void *)"pang";
    chunks[1].FromMemory.BufferLength = 4;
    ret = HttpSendHttpResponse(queue, req->RequestId, HTTP_SEND_RESPONSE_FLAG_MORE_DATA, (HTTP_RESPONSE *)&response, NULL, NULL, NULL, 0, NULL, NULL);
    ok(!ret, "Got error %u.\n", ret);

    ret = HttpSendResponseEntityBody(queue, req->RequestId, HTTP_SEND_RESPONSE_FLAG_MORE_DATA, 0, NULL, NULL, NULL, 0, NULL, NULL);
    ok(!ret, "Got error %u.\n", ret);

    ret_size = 0xdeadbeef;
    memset(chunks, 0, sizeof(chunks));
    chunks[0].DataChunkType = HttpDataChunkFromMemory;
    chunks[0].FromMemory.pBuffer = (void *)"foo";
    chunks[0].FromMemory.BufferLength = 3;
    chunks[1].DataChunkType = HttpDataChunkFromMemory;
    chunks[1].FromMemory.pBuffer = (void *)"bar";
    chunks[1].FromMemory.BufferLength = 3;
    ret = HttpSendResponseEntityBody(queue, req->RequestId, 0, ARRAY_SIZE(chunks), chunks, &ret_size, NULL, 0, NULL, NULL);
    ok(!ret, "Got error %u.\n", ret);
    ok(ret_size == 6, "Got size %lu.\n", ret_size);

    memset(response_buffer, 0, sizeof(response_buffer));

    /*
     * Loop on recv() for at most 1 second to ensure we get the full response
     * while ensuring we do not block forever if the expected response is never
     * sent.
     */
    len = 0;
    max_attempts = 100;
    for (i = 0; i < max_attempts && !strstr(response_buffer, "foobar"); ++i)
    {
        struct timeval tv;
        fd_set rfds;

        tv.tv_sec = 0;
        tv.tv_usec = 10000;
        FD_ZERO(&rfds);
        FD_SET(s, &rfds);

        ret = select(1, &rfds, NULL, NULL, &tv);
        ok(ret >= 0, "select() failed, ret = %d.\n", ret);

        if (ret > 0)
        {
            ret = recv(s, response_buffer + len, sizeof(response_buffer) - len, 0);
            ok(ret > 0, "recv() failed.\n");
            len += ret;
        }
    }
    ret = len;

    if (winetest_debug > 1)
        trace("%.*s\n", ret, response_buffer);
    ok(!strncmp(response_buffer, "HTTP/1.1 418 I'm a teapot\r\n", 27), "Got incorrect status line.\n");
    ok(!strstr(response_buffer, "\r\nContent-Length:"), "Unexpected Content-Length header.\n");
    ok(!!strstr(response_buffer, "\r\nDate:"), "Missing Date header.\n");
    ok(!memcmp(response_buffer + ret - 18, "\r\n\r\npongpangfoobar", 18), "Response did not end with entity data.\n");

    ret = HttpReceiveHttpRequest(queue, req->RequestId, 0, (HTTP_REQUEST *)req, sizeof(req_buffer), &ret_size, NULL);
    ok(ret == ERROR_CONNECTION_INVALID, "Got error %u.\n", ret);

    CloseHandle(ovl.hEvent);
    ret = remove_url_v1(queue, port);
    ok(!ret, "Got error %u.\n", ret);
    closesocket(s);
    ret = CloseHandle(queue);
    ok(ret, "Failed to close queue handle, error %lu.\n", GetLastError());
}

static void test_v1_bad_request(void)
{
    char response_buffer[2048];
    unsigned short port;
    HANDLE queue;
    SOCKET s;
    int ret;

    ret = HttpCreateHttpHandle(&queue, 0);
    ok(!ret, "Got error %u.\n", ret);
    port = add_url_v1(queue);

    s = create_client_socket(port);
    ret = send(s, "foo\r\n", strlen("foo\r\n"), 0);
    ok(ret == strlen("foo\r\n"), "send() returned %d.\n", ret);

    ret = recv(s, response_buffer, sizeof(response_buffer), 0);
    ok(ret > 0, "recv() failed.\n");

    if (winetest_debug > 1)
        trace("%.*s\n", ret, response_buffer);

    ok(!strncmp(response_buffer, "HTTP/1.1 400 Bad Request\r\n", 26), "Got incorrect status line.\n");
    ok(!!strstr(response_buffer, "\r\nConnection: close\r\n"), "Missing or malformed Connection header.\n");

    ret = send(s, "foo\r\n", strlen("foo\r\n"), 0);
    ok(ret == strlen("foo\r\n"), "send() returned %d.\n", ret);

    WSASetLastError(0xdeadbeef);
    ret = recv(s, response_buffer, sizeof(response_buffer), 0);
    ok(!ret, "Connection should be shut down.\n");
    ok(!WSAGetLastError(), "Got error %u.\n", WSAGetLastError());

    ret = remove_url_v1(queue, port);
    ok(!ret, "Got error %u.\n", ret);
    closesocket(s);
    ret = CloseHandle(queue);
    ok(ret, "Failed to close queue handle, error %lu.\n", GetLastError());
}

static void test_v1_cooked_url(void)
{
    char DECLSPEC_ALIGN(8) req_buffer[2048];
    HTTP_REQUEST_V1 *req = (HTTP_REQUEST_V1 *)req_buffer;
    char expect[24], req_text[200];
    unsigned short port;
    WCHAR expectW[50];
    DWORD ret_size;
    HANDLE queue;
    SOCKET s;
    int ret;

    static const char req1[] =
        "GET /foobar?a=b HTTP/1.1\r\n"
        "Host: localhost:%u\r\n"
        "Connection: keep-alive\r\n"
        "\r\n";

    static const char req2[] =
        "GET http://localhost:%u/ HTTP/1.1\r\n"
        "Host: ignored\r\n"
        "Connection: keep-alive\r\n"
        "\r\n";

    ret = HttpCreateHttpHandle(&queue, 0);
    ok(!ret, "Got error %u.\n", ret);
    port = add_url_v1(queue);

    s = create_client_socket(port);
    sprintf(req_text, req1, port);
    ret = send(s, req_text, strlen(req_text), 0);
    ok(ret == strlen(req_text), "send() returned %d.\n", ret);

    memset(req_buffer, 0xcc, sizeof(req_buffer));
    ret = HttpReceiveHttpRequest(queue, HTTP_NULL_ID, 0, (HTTP_REQUEST *)req, sizeof(req_buffer), &ret_size, NULL);
    ok(!ret, "Got error %u.\n", ret);
    ok(ret_size > sizeof(*req), "Got size %lu.\n", ret_size);
    ok(req->RawUrlLength == 11, "Got raw URL length %u.\n", req->RawUrlLength);
    ok(!strcmp(req->pRawUrl, "/foobar?a=b"), "Got raw URL %s.\n", req->pRawUrl);
    ok(req->CookedUrl.FullUrlLength == 66, "Got full URL length %u.\n", req->CookedUrl.FullUrlLength);
    ok(req->CookedUrl.HostLength == 30, "Got host length %u.\n", req->CookedUrl.HostLength);
    ok(req->CookedUrl.AbsPathLength == 14, "Got absolute path length %u.\n", req->CookedUrl.AbsPathLength);
    ok(req->CookedUrl.QueryStringLength == 8, "Got query string length %u.\n", req->CookedUrl.QueryStringLength);
    swprintf(expectW, ARRAY_SIZE(expectW), L"http://localhost:%u/foobar?a=b", port);
    ok(!wcscmp(req->CookedUrl.pFullUrl, expectW), "Expected full URL %s, got %s.\n",
            debugstr_w(expectW), debugstr_w(req->CookedUrl.pFullUrl));
    ok(req->CookedUrl.pHost == req->CookedUrl.pFullUrl + 7, "Got host %s.\n", wine_dbgstr_w(req->CookedUrl.pHost));
    ok(req->CookedUrl.pAbsPath == req->CookedUrl.pFullUrl + 22,
            "Got absolute path %s.\n", wine_dbgstr_w(req->CookedUrl.pAbsPath));
    ok(req->CookedUrl.pQueryString == req->CookedUrl.pFullUrl + 29,
            "Got query string %s.\n", wine_dbgstr_w(req->CookedUrl.pQueryString));

    send_response_v1(queue, req->RequestId, s);

    sprintf(req_text, req2, port);
    ret = send(s, req_text, strlen(req_text), 0);
    ok(ret == strlen(req_text), "send() returned %d.\n", ret);

    memset(req_buffer, 0xcc, sizeof(req_buffer));
    ret = HttpReceiveHttpRequest(queue, HTTP_NULL_ID, 0, (HTTP_REQUEST *)req, sizeof(req_buffer), &ret_size, NULL);
    ok(!ret, "Got error %u.\n", ret);
    ok(ret_size > sizeof(*req), "Got size %lu.\n", ret_size);
    ok(req->RawUrlLength == 23, "Got raw URL length %u.\n", req->RawUrlLength);
    sprintf(expect, "http://localhost:%u/", port);
    ok(!strcmp(req->pRawUrl, expect), "Expected raw URL \"%s\", got \"%s\".\n", expect, req->pRawUrl);
    ok(req->CookedUrl.FullUrlLength == 46, "Got full URL length %u.\n", req->CookedUrl.FullUrlLength);
    ok(req->CookedUrl.HostLength == 30, "Got host length %u.\n", req->CookedUrl.HostLength);
    ok(req->CookedUrl.AbsPathLength == 2, "Got absolute path length %u.\n", req->CookedUrl.AbsPathLength);
    ok(!req->CookedUrl.QueryStringLength, "Got query string length %u.\n", req->CookedUrl.QueryStringLength);
    swprintf(expectW, ARRAY_SIZE(expectW), L"http://localhost:%u/", port);
    ok(!wcscmp(req->CookedUrl.pFullUrl, expectW), "Expected full URL %s, got %s.\n",
            wine_dbgstr_w(expectW), wine_dbgstr_w(req->CookedUrl.pFullUrl));
    ok(req->CookedUrl.pHost == req->CookedUrl.pFullUrl + 7, "Got host %s.\n", wine_dbgstr_w(req->CookedUrl.pHost));
    ok(req->CookedUrl.pAbsPath == req->CookedUrl.pFullUrl + 22,
            "Got absolute path %s.\n", wine_dbgstr_w(req->CookedUrl.pAbsPath));
    ok(!req->CookedUrl.pQueryString, "Got query string %s.\n", wine_dbgstr_w(req->CookedUrl.pQueryString));

    send_response_v1(queue, req->RequestId, s);

    ret = remove_url_v1(queue, port);
    ok(!ret, "Got error %u.\n", ret);
    closesocket(s);
    ret = CloseHandle(queue);
    ok(ret, "Failed to close queue handle, error %lu.\n", GetLastError());
}

static void test_v1_unknown_tokens(void)
{
    char DECLSPEC_ALIGN(8) req_buffer[2048];
    HTTP_REQUEST_V1 *req = (HTTP_REQUEST_V1 *)req_buffer;
    unsigned short port;
    char req_text[200];
    DWORD ret_size;
    HANDLE queue;
    SOCKET s;
    int ret;

    static const char req1[] =
        "xyzzy / HTTP/1.1\r\n"
        "Host: localhost:%u\r\n"
        "Connection: keep-alive\r\n"
        "Qux: foo baz \r\n"
        "\r\n";

    ret = HttpCreateHttpHandle(&queue, 0);
    ok(!ret, "Got error %u.\n", ret);
    port = add_url_v1(queue);

    s = create_client_socket(port);
    sprintf(req_text, req1, port);
    ret = send(s, req_text, strlen(req_text), 0);
    ok(ret == strlen(req_text), "send() returned %d.\n", ret);

    memset(req_buffer, 0xcc, sizeof(req_buffer));
    ret = HttpReceiveHttpRequest(queue, HTTP_NULL_ID, 0, (HTTP_REQUEST *)req, sizeof(req_buffer), &ret_size, NULL);
    ok(!ret, "Got error %u.\n", ret);
    ok(req->Verb == HttpVerbUnknown, "Got verb %u.\n", req->Verb);
    ok(req->UnknownVerbLength == 5, "Got unknown verb length %u.\n", req->UnknownVerbLength);
    ok(!strcmp(req->pUnknownVerb, "xyzzy"), "Got unknown verb %s.\n", req->pUnknownVerb);
    ok(req->Headers.UnknownHeaderCount == 1, "Got %u unknown headers.\n", req->Headers.UnknownHeaderCount);
    ok(req->Headers.pUnknownHeaders[0].NameLength == 3, "Got name length %u.\n",
            req->Headers.pUnknownHeaders[0].NameLength);
    ok(!strcmp(req->Headers.pUnknownHeaders[0].pName, "Qux"), "Got name %s.\n",
            req->Headers.pUnknownHeaders[0].pName);
    ok(req->Headers.pUnknownHeaders[0].RawValueLength == 7, "Got value length %u.\n",
            req->Headers.pUnknownHeaders[0].RawValueLength);
    ok(!strcmp(req->Headers.pUnknownHeaders[0].pRawValue, "foo baz"), "Got value %s.\n",
            req->Headers.pUnknownHeaders[0].pRawValue);

    ret = remove_url_v1(queue, port);
    ok(!ret, "Got error %u.\n", ret);
    closesocket(s);
    ret = CloseHandle(queue);
    ok(ret, "Failed to close queue handle, error %lu.\n", GetLastError());
}

static void test_v1_multiple_urls(void)
{
    char DECLSPEC_ALIGN(8) req_buffer[2048];
    HTTP_REQUEST_V1 *req = (HTTP_REQUEST_V1 *)req_buffer;
    unsigned short ports[4];
    char req_text[200];
    DWORD ret_size;
    HANDLE queue;
    SOCKET s;
    unsigned int i;
    int ret;

    ret = HttpCreateHttpHandle(&queue, 0);
    ok(!ret, "Got error %u.\n", ret);

    for (i = 0; i < 4; ++i)
        ports[i] = add_url_v1(queue);

    for (i = 0; i < 4; ++i)
    {
        s = create_client_socket(ports[i]);
        sprintf(req_text, simple_req, ports[i]);
        ret = send(s, req_text, strlen(req_text), 0);
        ok(ret == strlen(req_text), "send() returned %d.\n", ret);

        memset(req_buffer, 0xcc, sizeof(req_buffer));
        ret = HttpReceiveHttpRequest(queue, HTTP_NULL_ID, 0, (HTTP_REQUEST *)req, sizeof(req_buffer), &ret_size, NULL);
        ok(!ret, "Got error %u.\n", ret);
        ok(ret_size > sizeof(*req), "Got size %lu.\n", ret_size);

        send_response_v1(queue, req->RequestId, s);
        closesocket(s);
    }

    ret = CloseHandle(queue);
    ok(ret, "Failed to close queue handle, error %lu.\n", GetLastError());
}

static void test_v1_relative_urls(void)
{
    char DECLSPEC_ALIGN(8) req_buffer[2048];
    char relative_req[] =
        "GET %s HTTP/1.1\r\n"
        "Host: localhost:%u\r\n"
        "Connection: keep-alive\r\n"
        "User-Agent: WINE\r\n"
        "\r\n";
    HTTP_REQUEST_V1 *req = (HTTP_REQUEST_V1 *)req_buffer;
    unsigned short port;
    WCHAR url[50], url2[50];
    char req_text[200];
    DWORD ret_size;
    HANDLE queue, queue2;
    SOCKET s;
    int ret;

    ret = HttpCreateHttpHandle(&queue, 0);
    ok(!ret, "Got error %u.\n", ret);
    ret = HttpCreateHttpHandle(&queue2, 0);
    ok(!ret, "Got error %u.\n", ret);

    port = add_url_v1(queue);
    swprintf(url2, ARRAY_SIZE(url2), L"http://localhost:%u/foobar/foo/", port);
    ret = HttpAddUrl(queue2, url2, NULL);
    ok(!ret, "Got error %u.\n", ret);

    s = create_client_socket(port);
    sprintf(req_text, simple_req, port);
    ret = send(s, req_text, strlen(req_text), 0);
    ok(ret == strlen(req_text), "send() returned %d.\n", ret);

    memset(req_buffer, 0xcc, sizeof(req_buffer));
    ret = HttpReceiveHttpRequest(queue, HTTP_NULL_ID, 0, (HTTP_REQUEST *)req, sizeof(req_buffer), &ret_size, NULL);
    ok(!ret, "Got error %u.\n", ret);
    ok(ret_size > sizeof(*req), "Got size %lu.\n", ret_size);

    send_response_v1(queue, req->RequestId, s);

    sprintf(req_text, relative_req, "/foobar/foo/bar", port);
    ret = send(s, req_text, strlen(req_text), 0);
    ok(ret == strlen(req_text), "send() returned %d.\n", ret);

    memset(req_buffer, 0xcc, sizeof(req_buffer));
    ret = HttpReceiveHttpRequest(queue2, HTTP_NULL_ID, 0, (HTTP_REQUEST *)req, sizeof(req_buffer), &ret_size, NULL);
    ok(!ret, "Got error %u.\n", ret);
    ok(ret_size > sizeof(*req), "Got size %lu.\n", ret_size);

    send_response_v1(queue2, req->RequestId, s);

    sprintf(req_text, relative_req, "/foobar/foo", port);
    ret = send(s, req_text, strlen(req_text), 0);
    ok(ret == strlen(req_text), "send() returned %d.\n", ret);

    memset(req_buffer, 0xcc, sizeof(req_buffer));
    ret = HttpReceiveHttpRequest(queue2, HTTP_NULL_ID, 0, (HTTP_REQUEST *)req, sizeof(req_buffer), &ret_size, NULL);
    ok(!ret, "Got error %u.\n", ret);
    ok(ret_size > sizeof(*req), "Got size %lu.\n", ret_size);

    send_response_v1(queue2, req->RequestId, s);

    sprintf(req_text, relative_req, "/foobar/foo?a=b", port);
    ret = send(s, req_text, strlen(req_text), 0);
    ok(ret == strlen(req_text), "send() returned %d.\n", ret);

    memset(req_buffer, 0xcc, sizeof(req_buffer));
    ret = HttpReceiveHttpRequest(queue2, HTTP_NULL_ID, 0, (HTTP_REQUEST *)req, sizeof(req_buffer), &ret_size, NULL);
    ok(!ret, "Got error %u.\n", ret);
    ok(ret_size > sizeof(*req), "Got size %lu.\n", ret_size);

    send_response_v1(queue2, req->RequestId, s);

    closesocket(s);
    remove_url_v1(queue, port);
    ret = HttpRemoveUrl(queue2, url2);
    ok(!ret, "Got error %u.\n", ret);

    swprintf(url, ARRAY_SIZE(url), L"http://localhost:%u/barfoo/", port);
    ret = HttpAddUrl(queue, url, NULL);
    ok(!ret, "Got error %u.\n", ret);
    swprintf(url2, ARRAY_SIZE(url2), L"http://localhost:%u/barfoo/", port);
    ret = HttpAddUrl(queue2, url2, NULL);
    ok(ret == ERROR_ALREADY_EXISTS, "Got error %u.\n", ret);

    swprintf(url2, ARRAY_SIZE(url2), L"http://localhost:%u/barfoo", port);
    ret = HttpAddUrl(queue2, url2, NULL);
    ok(ret == ERROR_ALREADY_EXISTS, "Got error %u.\n", ret);

    swprintf(url2, ARRAY_SIZE(url2), L"http://localhost:%u/barfoo?a=b", port);
    ret = HttpAddUrl(queue2, url2, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "Got error %u.\n", ret);

    ret = CloseHandle(queue);
    ok(ret, "Failed to close queue handle, error %lu.\n", GetLastError());
    ret = CloseHandle(queue2);
    ok(ret, "Failed to close queue handle, error %lu.\n", GetLastError());

    return;
}

static void test_v1_urls(void)
{
    char DECLSPEC_ALIGN(8) req_buffer[2048];
    HTTP_REQUEST_V1 *req = (HTTP_REQUEST_V1 *)req_buffer;
    unsigned short port;
    char req_text[200];
    DWORD ret_size;
    WCHAR url[50];
    HANDLE queue;
    SOCKET s;
    int ret;

    ret = HttpCreateHttpHandle(&queue, 0);
    ok(!ret, "Got error %u.\n", ret);

    for (port = 50000; port < 51000; ++port)
    {
        swprintf(url, ARRAY_SIZE(url), L"http://+:%u/", port);
        if (!(ret = HttpAddUrl(queue, url, NULL)))
            break;
        if (ret == ERROR_ACCESS_DENIED)
        {
            skip("Not enough permissions to bind to all URLs.\n");
            CloseHandle(queue);
            return;
        }
        ok(ret == ERROR_SHARING_VIOLATION, "Failed to add %s, error %u.\n", debugstr_w(url), ret);
    }

    ok(!ret, "Got error %u.\n", ret);

    s = create_client_socket(port);
    sprintf(req_text, simple_req, port);
    ret = send(s, req_text, strlen(req_text), 0);
    ok(ret == strlen(req_text), "send() returned %d.\n", ret);

    memset(req_buffer, 0xcc, sizeof(req_buffer));
    ret = HttpReceiveHttpRequest(queue, HTTP_NULL_ID, 0, (HTTP_REQUEST *)req, sizeof(req_buffer), &ret_size, NULL);
    ok(!ret, "Got error %u.\n", ret);
    ok(ret_size > sizeof(*req), "Got size %lu.\n", ret_size);

    send_response_v1(queue, req->RequestId, s);

    ret = HttpRemoveUrl(queue, url);
    ok(!ret, "Got error %u.\n", ret);
    closesocket(s);
    ret = CloseHandle(queue);
    ok(ret, "Failed to close queue handle, error %lu.\n", GetLastError());
}

static void test_HttpCreateServerSession(void)
{
    HTTP_SERVER_SESSION_ID session;
    HTTPAPI_VERSION version;
    int ret;

    version.HttpApiMajorVersion = 1;
    version.HttpApiMinorVersion = 0;
    ret = pHttpCreateServerSession(version, NULL, 0);
    ok(ret == ERROR_INVALID_PARAMETER, "Unexpected return value %u.\n", ret);

    version.HttpApiMajorVersion = 1;
    version.HttpApiMinorVersion = 1;
    ret = pHttpCreateServerSession(version, &session, 0);
    ok(ret == ERROR_REVISION_MISMATCH, "Unexpected return value %u.\n", ret);

    version.HttpApiMajorVersion = 3;
    version.HttpApiMinorVersion = 0;
    ret = pHttpCreateServerSession(version, &session, 0);
    ok(ret == ERROR_REVISION_MISMATCH, "Unexpected return value %u.\n", ret);

    version.HttpApiMajorVersion = 2;
    version.HttpApiMinorVersion = 0;
    ret = pHttpCreateServerSession(version, &session, 0);
    ok(!ret, "Unexpected return value %u.\n", ret);
    ret = pHttpCloseServerSession(session);
    ok(!ret, "Unexpected return value %u.\n", ret);

    version.HttpApiMajorVersion = 1;
    version.HttpApiMinorVersion = 0;
    ret = pHttpCreateServerSession(version, &session, 0);
    ok(!ret, "Unexpected return value %u.\n", ret);
    ret = pHttpCloseServerSession(session);
    ok(!ret, "Unexpected return value %u.\n", ret);

    ret = pHttpCloseServerSession(0xdead);
    ok(ret == ERROR_INVALID_PARAMETER, "Unexpected return value %u.\n", ret);
}

static void test_HttpCreateUrlGroup(void)
{
    HTTP_SERVER_SESSION_ID session;
    HTTP_URL_GROUP_ID group_id;
    HTTPAPI_VERSION version;
    int ret;

    group_id = 1;
    ret = pHttpCreateUrlGroup(0, &group_id, 0);
    ok(ret == ERROR_INVALID_PARAMETER, "Unexpected return value %u.\n", ret);
    ok(group_id == 1, "Unexpected group id %s.\n", wine_dbgstr_longlong(group_id));

    /* Create session, url group, close session. */
    version.HttpApiMajorVersion = 1;
    version.HttpApiMinorVersion = 0;
    ret = pHttpCreateServerSession(version, &session, 0);
    ok(!ret, "Unexpected return value %u.\n", ret);

    group_id = 0;
    ret = pHttpCreateUrlGroup(session, &group_id, 0);
    ok(!ret, "Unexpected return value %u.\n", ret);
    ok(group_id != 0, "Unexpected group id %s.\n", wine_dbgstr_longlong(group_id));

    ret = pHttpCloseServerSession(session);
    ok(!ret, "Unexpected return value %u.\n", ret);

    /* Groups are closed together with their session. */
    ret = pHttpCloseUrlGroup(group_id);
    ok(ret == ERROR_INVALID_PARAMETER, "Unexpected return value %u.\n", ret);

    /* Create session, url group, close group. */
    ret = pHttpCreateServerSession(version, &session, 0);
    ok(!ret, "Unexpected return value %u.\n", ret);

    group_id = 0;
    ret = pHttpCreateUrlGroup(session, &group_id, 0);
    ok(!ret, "Unexpected return value %u.\n", ret);
    ok(group_id != 0, "Unexpected group id %s.\n", wine_dbgstr_longlong(group_id));

    ret = pHttpCloseUrlGroup(group_id);
    ok(!ret, "Unexpected return value %u.\n", ret);

    ret = pHttpCloseServerSession(session);
    ok(!ret, "Unexpected return value %u.\n", ret);
}

static void test_v2_bound_port(void)
{
    static const HTTPAPI_VERSION version = {2, 0};
    struct sockaddr_in sockaddr;
    HTTP_SERVER_SESSION_ID session;
    HTTP_BINDING_INFO binding;
    HTTP_URL_GROUP_ID group;
    unsigned short port;
    WCHAR url[50];
    HANDLE queue, dummy_queue;
    int ret;
    SOCKET s2;

    ret = pHttpCreateServerSession(version, &session, 0);
    ok(!ret, "Failed to create session, error %u.\n", ret);
    ret = pHttpCreateUrlGroup(session, &group, 0);
    ok(!ret, "Failed to create URL group, error %u.\n", ret);

    ret = pHttpCreateRequestQueue(version, NULL, NULL, 0, &queue);
    ok(!ret, "Failed to create request queue, error %u.\n", ret);
    ret = pHttpCreateRequestQueue(version, NULL, NULL, 0, &dummy_queue);
    ok(!ret, "Failed to create request queue, error %u.\n", ret);
    binding.Flags.Present = 1;
    binding.RequestQueueHandle = queue;
    ret = pHttpSetUrlGroupProperty(group, HttpServerBindingProperty, &binding, sizeof(binding));
    ok(!ret, "Failed to bind request queue, error %u.\n", ret);

    s2 = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    for (port = 50000; port < 51000; ++port)
    {
        sockaddr.sin_port = htons(port);
        ret = bind(s2, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
        if (!ret)
            break;
    }
    ok(!ret, "Failed to bind to port\n");
    swprintf(url, ARRAY_SIZE(url), L"http://localhost:%u/", port);
    ret = pHttpAddUrlToUrlGroup(group, url, 0xdeadbeef, 0);
    ok(ret == ERROR_SHARING_VIOLATION, "Unexpected failure adding %s, error %u.\n", debugstr_w(url), ret);
    shutdown(s2, SD_BOTH);
    closesocket(s2);

    binding.RequestQueueHandle = dummy_queue;
    ret = pHttpSetUrlGroupProperty(group, HttpServerBindingProperty, &binding, sizeof(binding));
    ok(!ret, "Failed to rebind request queue, error %u.\n", ret);

    ret = pHttpCloseRequestQueue(dummy_queue);
    ok(!ret, "Failed to close queue handle, error %u.\n", ret);
    ret = pHttpCloseRequestQueue(queue);
    ok(!ret, "Failed to close queue handle, error %u.\n", ret);
    ret = pHttpCloseUrlGroup(group);
    ok(!ret, "Failed to close group, error %u.\n", ret);
    ret = pHttpCloseServerSession(session);
    ok(!ret, "Failed to close group, error %u.\n", ret);
}

static void test_v2_queue_after_url(void)
{
    char DECLSPEC_ALIGN(8) req_buffer[2048];
    HTTP_REQUEST_V2 *reqv2 = (HTTP_REQUEST_V2 *)req_buffer;
    static const HTTPAPI_VERSION version = {2, 0};
    HTTP_REQUEST_V1 *req = &reqv2->s;
    HTTP_SERVER_SESSION_ID session;
    HTTP_BINDING_INFO binding;
    HTTP_URL_GROUP_ID group;
    unsigned short port;
    char req_text[100];
    HANDLE queue;
    int ret;
    SOCKET s;

    ret = pHttpCreateServerSession(version, &session, 0);
    ok(!ret, "Failed to create session, error %u.\n", ret);
    ret = pHttpCreateUrlGroup(session, &group, 0);
    ok(!ret, "Failed to create URL group, error %u.\n", ret);

    port = add_url_v2(group);

    ret = pHttpCreateRequestQueue(version, NULL, NULL, 0, &queue);
    ok(!ret, "Failed to create request queue, error %u.\n", ret);
    binding.Flags.Present = 1;
    binding.RequestQueueHandle = queue;
    ret = pHttpSetUrlGroupProperty(group, HttpServerBindingProperty, &binding, sizeof(binding));
    ok(!ret, "Failed to bind request queue, error %u.\n", ret);

    s = create_client_socket(port);

    sprintf(req_text, simple_req, port);
    ret = send(s, req_text, strlen(req_text), 0);
    ok(ret == strlen(req_text), "send() returned %d.\n", ret);

    ret = HttpReceiveHttpRequest(queue, HTTP_NULL_ID, 0, (HTTP_REQUEST *)req, sizeof(req_buffer), NULL, NULL);
    ok(!ret, "Got error %u.\n", ret);

    ok(req->BytesReceived == strlen(req_text), "Got %s bytes.\n", wine_dbgstr_longlong(req->BytesReceived));

    ret = remove_url_v2(group, port);
    ok(!ret, "Got error %u.\n", ret);

    closesocket(s);
    ret = pHttpCloseRequestQueue(queue);
    ok(!ret, "Failed to close queue handle, error %u.\n", ret);
    ret = pHttpCloseUrlGroup(group);
    ok(!ret, "Failed to close group, error %u.\n", ret);
    ret = pHttpCloseServerSession(session);
    ok(!ret, "Failed to close group, error %u.\n", ret);
}

static void test_v2_server(void)
{
    char DECLSPEC_ALIGN(8) req_buffer[2048], response_buffer[2048];
    HTTP_REQUEST_V2 *reqv2 = (HTTP_REQUEST_V2 *)req_buffer;
    static const HTTPAPI_VERSION version = {2, 0};
    struct sockaddr_in sockaddr, *sin;
    HTTP_REQUEST_V1 *req = &reqv2->s;
    HTTP_SERVER_SESSION_ID session;
    HTTP_RESPONSE_V2 response = {};
    HTTP_BINDING_INFO binding;
    HTTP_URL_GROUP_ID group;
    unsigned short port;
    char req_text[100];
    unsigned int i;
    OVERLAPPED ovl;
    DWORD ret_size;
    WCHAR url[50];
    HANDLE queue;
    int ret, len;
    SOCKET s;

    ovl.hEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
    memset(req_buffer, 0xcc, sizeof(req_buffer));

    ret = pHttpCreateServerSession(version, &session, 0);
    ok(!ret, "Failed to create session, error %u.\n", ret);
    ret = pHttpCreateUrlGroup(session, &group, 0);
    ok(!ret, "Failed to create URL group, error %u.\n", ret);
    ret = pHttpCreateRequestQueue(version, NULL, NULL, 0, &queue);
    ok(!ret, "Failed to create request queue, error %u.\n", ret);
    binding.Flags.Present = 1;
    binding.RequestQueueHandle = queue;
    ret = pHttpSetUrlGroupProperty(group, HttpServerBindingProperty, &binding, sizeof(binding));
    ok(!ret, "Failed to bind request queue, error %u.\n", ret);

    ret = HttpReceiveHttpRequest(NULL, HTTP_NULL_ID, 0, (HTTP_REQUEST *)req, sizeof(req_buffer), NULL, &ovl);
    ok(ret == ERROR_INVALID_HANDLE, "Got error %u.\n", ret);
    ret = HttpReceiveHttpRequest(queue, 0xdeadbeef, 0, (HTTP_REQUEST *)req, sizeof(req_buffer), NULL, &ovl);
    ok(ret == ERROR_CONNECTION_INVALID, "Got error %u.\n", ret);
    ret = HttpReceiveHttpRequest(queue, HTTP_NULL_ID, 0, (HTTP_REQUEST *)req, sizeof(req_buffer), NULL, &ovl);
    ok(ret == ERROR_IO_PENDING, "Got error %u.\n", ret);

    SetLastError(0xdeadbeef);
    ret = GetOverlappedResult(queue, &ovl, &ret_size, FALSE);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_IO_INCOMPLETE, "Got error %lu.\n", GetLastError());

    port = add_url_v2(group);

    ret = pHttpAddUrlToUrlGroup(group, L"http://localhost:50000", 0xdeadbeef, 0);
    todo_wine ok(ret == ERROR_INVALID_PARAMETER, "Got error %u.\n", ret);
    ret = pHttpAddUrlToUrlGroup(group, L"localhost:50000", 0xdeadbeef, 0);
    todo_wine ok(ret == ERROR_INVALID_PARAMETER, "Got error %u.\n", ret);
    ret = pHttpAddUrlToUrlGroup(group, L"localhost:50000/", 0xdeadbeef, 0);
    todo_wine ok(ret == ERROR_INVALID_PARAMETER, "Got error %u.\n", ret);
    ret = pHttpAddUrlToUrlGroup(group, L"http://localhost/", 0xdeadbeef, 0);
    todo_wine ok(ret == ERROR_INVALID_PARAMETER, "Got error %u.\n", ret);
    ret = pHttpAddUrlToUrlGroup(group, L"http://localhost:/", 0xdeadbeef, 0);
    todo_wine ok(ret == ERROR_INVALID_PARAMETER, "Got error %u.\n", ret);
    ret = pHttpAddUrlToUrlGroup(group, L"http://localhost:0/", 0xdeadbeef, 0);
    todo_wine ok(ret == ERROR_INVALID_PARAMETER, "Got error %u.\n", ret);
    swprintf(url, ARRAY_SIZE(url), L"http://localhost:%u/", port);
    ret = pHttpAddUrlToUrlGroup(group, url, 0xdeadbeef, 0);
    todo_wine ok(ret == ERROR_ALREADY_EXISTS, "Got error %u.\n", ret);

    s = create_client_socket(port);
    len = sizeof(sockaddr);
    ret = getsockname(s, (struct sockaddr *)&sockaddr, &len);
    ok(ret == 0, "getsockname() failed, error %u.\n", WSAGetLastError());

    SetLastError(0xdeadbeef);
    ret = GetOverlappedResult(queue, &ovl, &ret_size, FALSE);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_IO_INCOMPLETE, "Got error %lu.\n", GetLastError());

    sprintf(req_text, simple_req, port);
    ret = send(s, req_text, strlen(req_text), 0);
    ok(ret == strlen(req_text), "send() returned %d.\n", ret);

    ret = WaitForSingleObject(ovl.hEvent, 100);
    ok(!ret, "Got %u.\n", ret);

    Sleep(100);

    ret = GetOverlappedResult(queue, &ovl, &ret_size, TRUE);
    ok(ret, "Got error %lu.\n", GetLastError());
    ok(ret_size > sizeof(*req), "Got size %lu.\n", ret_size);

    ok(!req->Flags, "Got flags %#lx.\n", req->Flags);
    ok(req->ConnectionId, "Expected nonzero connection ID.\n");
    ok(req->RequestId, "Expected nonzero connection ID.\n");
    ok(req->UrlContext == 0xdeadbeef, "Got URL context %s.\n", wine_dbgstr_longlong(req->UrlContext));
    ok(req->Version.MajorVersion == 1, "Got major version %u.\n", req->Version.MajorVersion);
    ok(req->Version.MinorVersion == 1, "Got major version %u.\n", req->Version.MinorVersion);
    ok(req->Verb == HttpVerbGET, "Got verb %u.\n", req->Verb);
    ok(!req->UnknownVerbLength, "Got unknown verb length %u.\n", req->UnknownVerbLength);
    ok(req->RawUrlLength == 7, "Got raw URL length %u.\n", req->RawUrlLength);
    ok(!req->pUnknownVerb, "Got unknown verb %s.\n", req->pUnknownVerb);
    ok(!strcmp(req->pRawUrl, "/foobar"), "Got raw URL %s.\n", req->pRawUrl);
    ok(req->CookedUrl.FullUrlLength == 58, "Got full URL length %u.\n", req->CookedUrl.FullUrlLength);
    ok(req->CookedUrl.HostLength == 30, "Got host length %u.\n", req->CookedUrl.HostLength);
    ok(req->CookedUrl.AbsPathLength == 14, "Got absolute path length %u.\n", req->CookedUrl.AbsPathLength);
    ok(!req->CookedUrl.QueryStringLength, "Got query string length %u.\n", req->CookedUrl.QueryStringLength);
    swprintf(url, ARRAY_SIZE(url), L"http://localhost:%u/foobar", port);
    ok(!wcscmp(req->CookedUrl.pFullUrl, url), "Expected full URL %s, got %s.\n",
            debugstr_w(url), debugstr_w(req->CookedUrl.pFullUrl));
    ok(req->CookedUrl.pHost == req->CookedUrl.pFullUrl + 7, "Got host %s.\n", wine_dbgstr_w(req->CookedUrl.pHost));
    ok(req->CookedUrl.pAbsPath == req->CookedUrl.pFullUrl + 22,
            "Got absolute path %s.\n", wine_dbgstr_w(req->CookedUrl.pAbsPath));
    ok(!req->CookedUrl.pQueryString, "Got query string %s.\n", wine_dbgstr_w(req->CookedUrl.pQueryString));
    ok(!memcmp(req->Address.pRemoteAddress, &sockaddr, len), "Client addresses didn't match.\n");
    sin = (SOCKADDR_IN *)req->Address.pLocalAddress;
    ok(sin->sin_family == AF_INET, "Got family %u.\n", sin->sin_family);
    ok(ntohs(sin->sin_port) == port, "Got wrong port %u.\n", ntohs(sin->sin_port));
    ok(sin->sin_addr.S_un.S_addr == inet_addr("127.0.0.1"), "Got address %08lx.\n", sin->sin_addr.S_un.S_addr);
    ok(!req->Headers.UnknownHeaderCount, "Got %u unknown headers.\n", req->Headers.UnknownHeaderCount);
    ok(!req->Headers.pUnknownHeaders, "Got unknown headers %p.\n", req->Headers.pUnknownHeaders);
    for (i = 0; i < ARRAY_SIZE(req->Headers.KnownHeaders); ++i)
    {
        if (i == HttpHeaderConnection)
        {
            ok(req->Headers.KnownHeaders[i].RawValueLength == 10, "Got length %u.\n",
                    req->Headers.KnownHeaders[i].RawValueLength);
            ok(!strcmp(req->Headers.KnownHeaders[i].pRawValue, "keep-alive"),
                    "Got connection '%s'.\n", req->Headers.KnownHeaders[i].pRawValue);
        }
        else if (i == HttpHeaderHost)
        {
            char expect[16];
            sprintf(expect, "localhost:%u", port);
            ok(req->Headers.KnownHeaders[i].RawValueLength == strlen(expect), "Got length %u.\n",
                    req->Headers.KnownHeaders[i].RawValueLength);
            ok(!strcmp(req->Headers.KnownHeaders[i].pRawValue, expect),
                    "Got connection '%s'.\n", req->Headers.KnownHeaders[i].pRawValue);
        }
        else if (i == HttpHeaderUserAgent)
        {
            ok(req->Headers.KnownHeaders[i].RawValueLength == 4, "Got length %u.\n",
                    req->Headers.KnownHeaders[i].RawValueLength);
            ok(!strcmp(req->Headers.KnownHeaders[i].pRawValue, "WINE"),
                    "Got connection '%s'.\n", req->Headers.KnownHeaders[i].pRawValue);
        }
        else
        {
            ok(!req->Headers.KnownHeaders[i].RawValueLength, "Header %#x: got length %u.\n",
                    i, req->Headers.KnownHeaders[i].RawValueLength);
            ok(!req->Headers.KnownHeaders[i].pRawValue, "Header %#x: got value '%s'.\n",
                    i, req->Headers.KnownHeaders[i].pRawValue);
        }
    }
    ok(req->BytesReceived == strlen(req_text), "Got %s bytes.\n", wine_dbgstr_longlong(req->BytesReceived));
    ok(!req->EntityChunkCount, "Got %u entity chunks.\n", req->EntityChunkCount);
    ok(!req->pEntityChunks, "Got entity chunks %p.\n", req->pEntityChunks);
    /* req->RawConnectionId is zero until Win10 2004. */
    ok(!req->pSslInfo, "Got SSL info %p.\n", req->pSslInfo);
    /* RequestInfoCount and pRequestInfo are zero until Win10 1909. */

    response.s.StatusCode = 418;
    response.s.pReason = "I'm a teapot";
    response.s.ReasonLength = 12;
    response.s.Headers.KnownHeaders[HttpHeaderRetryAfter].pRawValue = "120";
    response.s.Headers.KnownHeaders[HttpHeaderRetryAfter].RawValueLength = 3;
    ret = HttpSendHttpResponse(queue, 0xdeadbeef, 0, (HTTP_RESPONSE *)&response, NULL, NULL, NULL, 0, &ovl, NULL);
    ok(ret == ERROR_CONNECTION_INVALID, "Got error %u.\n", ret);
    ret = HttpSendHttpResponse(queue, req->RequestId, 0, (HTTP_RESPONSE *)&response, NULL, NULL, NULL, 0, &ovl, NULL);
    ok(!ret, "Got error %u.\n", ret);
    ret = GetOverlappedResult(queue, &ovl, &ret_size, FALSE);
    ok(ret, "Got error %lu.\n", GetLastError());

    ret = recv(s, response_buffer, sizeof(response_buffer), 0);
    ok(ret == ret_size, "Expected size %lu, got %u.\n", ret_size, ret);

    if (winetest_debug > 1)
        trace("%.*s\n", ret, response_buffer);

    ok(!strncmp(response_buffer, "HTTP/1.1 418 I'm a teapot\r\n", 27), "Got incorrect status line.\n");
    ok(!!strstr(response_buffer, "\r\nRetry-After: 120\r\n"), "Missing or malformed Retry-After header.\n");
    ok(!!strstr(response_buffer, "\r\nDate:"), "Missing Date header.\n");

    ret = HttpReceiveHttpRequest(queue, req->RequestId, 0, (HTTP_REQUEST *)req, sizeof(req_buffer), NULL, &ovl);
    ok(ret == ERROR_CONNECTION_INVALID, "Got error %u.\n", ret);

    ret = remove_url_v2(group, port);
    ok(!ret, "Got error %u.\n", ret);
    ret = remove_url_v2(group, port);
    ok(ret == ERROR_FILE_NOT_FOUND, "Got error %u.\n", ret);

    closesocket(s);
    CloseHandle(ovl.hEvent);
    ret = pHttpCloseRequestQueue(queue);
    ok(!ret, "Failed to close queue handle, error %u.\n", ret);
    ret = pHttpCloseUrlGroup(group);
    ok(!ret, "Failed to close group, error %u.\n", ret);
    ret = pHttpCloseServerSession(session);
    ok(!ret, "Failed to close group, error %u.\n", ret);
}

static void test_v2_completion_port(void)
{
    char DECLSPEC_ALIGN(8) req_buffer[2048], response_buffer[2048];
    HTTP_REQUEST_V2 *req = (HTTP_REQUEST_V2 *)req_buffer;
    static const HTTPAPI_VERSION version = {2, 0};
    HTTP_SERVER_SESSION_ID session;
    HTTP_RESPONSE_V2 response = {};
    HTTP_BINDING_INFO binding;
    HTTP_URL_GROUP_ID group;
    unsigned short tcp_port;
    OVERLAPPED ovl, *povl;
    HANDLE queue, port;
    char req_text[100];
    DWORD ret_size;
    ULONG_PTR key;
    SOCKET s;
    int ret;

    ovl.hEvent = CreateEventA(NULL, TRUE, FALSE, NULL);

    ret = pHttpCreateServerSession(version, &session, 0);
    ok(!ret, "Failed to create session, error %u.\n", ret);
    ret = pHttpCreateUrlGroup(session, &group, 0);
    ok(!ret, "Failed to create URL group, error %u.\n", ret);
    ret = pHttpCreateRequestQueue(version, NULL, NULL, 0, &queue);
    ok(!ret, "Failed to create request queue, error %u.\n", ret);
    binding.Flags.Present = 1;
    binding.RequestQueueHandle = queue;
    ret = pHttpSetUrlGroupProperty(group, HttpServerBindingProperty, &binding, sizeof(binding));
    ok(!ret, "Failed to bind request queue, error %u.\n", ret);

    port = CreateIoCompletionPort(queue, NULL, 123, 0);
    ok(!!port, "Failed to create completion port, error %lu.\n", GetLastError());

    ret = GetQueuedCompletionStatus(port, &ret_size, &key, &povl, 0);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == WAIT_TIMEOUT, "Got error %lu.\n", GetLastError());

    ret = HttpReceiveHttpRequest(queue, HTTP_NULL_ID, 0, (HTTP_REQUEST *)req, sizeof(req_buffer), NULL, &ovl);
    ok(ret == ERROR_IO_PENDING, "Got error %u.\n", ret);

    tcp_port = add_url_v2(group);
    s = create_client_socket(tcp_port);

    ret = GetQueuedCompletionStatus(port, &ret_size, &key, &povl, 0);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == WAIT_TIMEOUT, "Got error %lu.\n", GetLastError());

    sprintf(req_text, simple_req, tcp_port);
    ret = send(s, req_text, strlen(req_text), 0);
    ok(ret == strlen(req_text), "send() returned %d.\n", ret);

    ret_size = key = 0xdeadbeef;
    ret = GetQueuedCompletionStatus(port, &ret_size, &key, &povl, 1000);
    ok(ret, "Got error %lu.\n", GetLastError());
    ok(povl == &ovl, "OVERLAPPED pointers didn't match.\n");
    ok(key == 123, "Got unexpected key %Iu.\n", key);
    ok(ret_size > sizeof(*req), "Got size %lu.\n", ret_size);

    ret = GetQueuedCompletionStatus(port, &ret_size, &key, &povl, 0);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == WAIT_TIMEOUT, "Got error %lu.\n", GetLastError());

    response.s.StatusCode = 418;
    response.s.pReason = "I'm a teapot";
    response.s.ReasonLength = 12;
    ret = HttpSendHttpResponse(queue, req->s.RequestId, 0, (HTTP_RESPONSE *)&response, NULL, NULL, NULL, 0, &ovl, NULL);
    ok(!ret, "Got error %u.\n", ret);

    ret_size = key = 0xdeadbeef;
    ret = GetQueuedCompletionStatus(port, &ret_size, &key, &povl, 1000);
    ok(ret, "Got error %lu.\n", GetLastError());
    ok(povl == &ovl, "OVERLAPPED pointers didn't match.\n");
    ok(key == 123, "Got unexpected key %Iu.\n", key);

    ret = recv(s, response_buffer, sizeof(response_buffer), 0);
    ok(ret == ret_size, "Expected size %lu, got %u.\n", ret_size, ret);

    ret = remove_url_v2(group, tcp_port);
    ok(!ret, "Got error %u.\n", ret);
    closesocket(s);
    CloseHandle(port);
    CloseHandle(ovl.hEvent);
    ret = pHttpCloseRequestQueue(queue);
    ok(!ret, "Failed to close queue handle, error %u.\n", ret);
    ret = pHttpCloseUrlGroup(group);
    ok(!ret, "Failed to close group, error %u.\n", ret);
    ret = pHttpCloseServerSession(session);
    ok(!ret, "Failed to close group, error %u.\n", ret);
}

START_TEST(httpapi)
{
    HTTPAPI_VERSION version = { 1, 0 };
    WSADATA wsadata;
    int ret;

    init();

    WSAStartup(MAKEWORD(1,1), &wsadata);

    ret = HttpInitialize(version, HTTP_INITIALIZE_SERVER, NULL);
    ok(!ret, "Failed to initialize library, ret %u.\n", ret);

    test_v1_server();
    test_v1_completion_port();
    test_v1_multiple_requests();
    test_v1_short_buffer();
    test_v1_entity_body();
    test_v1_bad_request();
    test_v1_cooked_url();
    test_v1_unknown_tokens();
    test_v1_multiple_urls();
    test_v1_relative_urls();
    test_v1_urls();

    ret = HttpTerminate(HTTP_INITIALIZE_SERVER, NULL);
    ok(!ret, "Failed to terminate, ret %u.\n", ret);

    version.HttpApiMajorVersion = 2;
    if (!HttpInitialize(version, HTTP_INITIALIZE_SERVER, NULL))
    {
        test_HttpCreateServerSession();
        test_HttpCreateUrlGroup();
        test_v2_server();
        test_v2_queue_after_url();
        test_v2_bound_port();
        test_v2_completion_port();

        ret = HttpTerminate(HTTP_INITIALIZE_SERVER, NULL);
        ok(!ret, "Failed to terminate, ret %u.\n", ret);
    }
    else
        win_skip("Version 2 is not supported.\n");
}
