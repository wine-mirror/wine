/*
 * test status notifications
 *
 * Copyright 2008 Hans Leidekker for CodeWeavers
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

static const WCHAR user_agent[] = {'w','i','n','e','t','e','s','t',0};
static const WCHAR test_winehq[] = {'t','e','s','t','.','w','i','n','e','h','q','.','o','r','g',0};
static const WCHAR tests_hello_html[] = {'/','t','e','s','t','s','/','h','e','l','l','o','.','h','t','m','l',0};
static const WCHAR tests_redirect[] = {'/','t','e','s','t','s','/','r','e','d','i','r','e','c','t',0};

enum api
{
    winhttp_connect = 1,
    winhttp_open_request,
    winhttp_send_request,
    winhttp_receive_response,
    winhttp_query_data,
    winhttp_read_data,
    winhttp_write_data,
    winhttp_close_handle
};

struct notification
{
    enum api function;      /* api responsible for notification */
    unsigned int status;    /* status received */
    DWORD flags;            /* a combination of NF_* flags */
};

#define NF_ALLOW       0x0001  /* notification may or may not happen */
#define NF_WINE_ALLOW  0x0002  /* wine sends notification when it should not */
#define NF_SIGNAL      0x0004  /* signal wait handle when notified */

struct info
{
    enum api function;
    const struct notification *test;
    unsigned int count;
    unsigned int index;
    HANDLE wait;
    unsigned int line;
};

static void CALLBACK check_notification( HINTERNET handle, DWORD_PTR context, DWORD status, LPVOID buffer, DWORD buflen )
{
    BOOL status_ok, function_ok;
    struct info *info = (struct info *)context;

    if (status == WINHTTP_CALLBACK_STATUS_HANDLE_CREATED)
    {
        DWORD size = sizeof(struct info *);
        WinHttpQueryOption( handle, WINHTTP_OPTION_CONTEXT_VALUE, &info, &size );
    }
    while (info->index < info->count && info->test[info->index].status != status && (info->test[info->index].flags & NF_ALLOW))
        info->index++;
    while (info->index < info->count && (info->test[info->index].flags & NF_WINE_ALLOW))
    {
        todo_wine ok(info->test[info->index].status != status, "unexpected %x notification\n", status);
        if (info->test[info->index].status == status) break;
        info->index++;
    }
    ok(info->index < info->count, "%u: unexpected notification 0x%08x\n", info->line, status);
    if (info->index >= info->count) return;

    status_ok   = (info->test[info->index].status == status);
    function_ok = (info->test[info->index].function == info->function);
    ok(status_ok, "%u: expected status 0x%08x got 0x%08x\n", info->line, info->test[info->index].status, status);
    ok(function_ok, "%u: expected function %u got %u\n", info->line, info->test[info->index].function, info->function);

    if (status_ok && function_ok && info->test[info->index++].flags & NF_SIGNAL)
    {
        SetEvent( info->wait );
    }
}

static const struct notification cache_test[] =
{
    { winhttp_connect,          WINHTTP_CALLBACK_STATUS_HANDLE_CREATED },
    { winhttp_open_request,     WINHTTP_CALLBACK_STATUS_HANDLE_CREATED },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_RESOLVING_NAME },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_NAME_RESOLVED },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_SENDING_REQUEST },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_REQUEST_SENT },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_CLOSING_CONNECTION, NF_WINE_ALLOW },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_CONNECTION_CLOSED, NF_WINE_ALLOW },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING, NF_SIGNAL },
    { winhttp_open_request,     WINHTTP_CALLBACK_STATUS_HANDLE_CREATED },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER, NF_WINE_ALLOW },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER, NF_WINE_ALLOW },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_SENDING_REQUEST },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_REQUEST_SENT },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_CLOSING_CONNECTION, NF_WINE_ALLOW },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_CONNECTION_CLOSED, NF_WINE_ALLOW },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING, NF_SIGNAL },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING, NF_SIGNAL },
    { winhttp_connect,          WINHTTP_CALLBACK_STATUS_HANDLE_CREATED },
    { winhttp_open_request,     WINHTTP_CALLBACK_STATUS_HANDLE_CREATED },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_RESOLVING_NAME, NF_WINE_ALLOW },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_NAME_RESOLVED, NF_WINE_ALLOW },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER, NF_WINE_ALLOW },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER, NF_WINE_ALLOW },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_SENDING_REQUEST },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_REQUEST_SENT },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_CLOSING_CONNECTION, NF_WINE_ALLOW },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_CONNECTION_CLOSED, NF_WINE_ALLOW },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING, NF_SIGNAL },
    { winhttp_open_request,     WINHTTP_CALLBACK_STATUS_HANDLE_CREATED },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER, NF_WINE_ALLOW },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER, NF_WINE_ALLOW },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_SENDING_REQUEST },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_REQUEST_SENT },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_CLOSING_CONNECTION, NF_WINE_ALLOW },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_CONNECTION_CLOSED, NF_WINE_ALLOW },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING, NF_SIGNAL },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING, NF_SIGNAL }
};

static void setup_test( struct info *info, enum api function, unsigned int line )
{
    if (info->wait) ResetEvent( info->wait );
    info->function = function;
    info->line = line;
    while (info->index < info->count && info->test[info->index].function != function
           && (info->test[info->index].flags & (NF_ALLOW | NF_WINE_ALLOW)))
        info->index++;
    ok_(__FILE__,line)(info->test[info->index].function == function,
                       "unexpected function %u, expected %u. probably some notifications were missing\n",
                       info->test[info->index].function, function);
}

static void test_connection_cache( void )
{
    HANDLE ses, con, req, event;
    DWORD size, status, err;
    BOOL ret, unload = TRUE;
    struct info info, *context = &info;

    info.test  = cache_test;
    info.count = sizeof(cache_test) / sizeof(cache_test[0]);
    info.index = 0;
    info.wait = CreateEventW( NULL, FALSE, FALSE, NULL );

    ses = WinHttpOpen( user_agent, 0, NULL, NULL, 0 );
    ok(ses != NULL, "failed to open session %u\n", GetLastError());

    event = CreateEventW( NULL, FALSE, FALSE, NULL );
    ret = WinHttpSetOption( ses, WINHTTP_OPTION_UNLOAD_NOTIFY_EVENT, &event, sizeof(event) );
    if (!ret)
    {
        win_skip("Unload event not supported\n");
        unload = FALSE;
    }

    WinHttpSetStatusCallback( ses, check_notification, WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS, 0 );

    ret = WinHttpSetOption( ses, WINHTTP_OPTION_CONTEXT_VALUE, &context, sizeof(struct info *) );
    ok(ret, "failed to set context value %u\n", GetLastError());

    setup_test( &info, winhttp_connect, __LINE__ );
    con = WinHttpConnect( ses, test_winehq, 0, 0 );
    ok(con != NULL, "failed to open a connection %u\n", GetLastError());

    setup_test( &info, winhttp_open_request, __LINE__ );
    req = WinHttpOpenRequest( con, NULL, tests_hello_html, NULL, NULL, NULL, 0 );
    ok(req != NULL, "failed to open a request %u\n", GetLastError());

    setup_test( &info, winhttp_send_request, __LINE__ );
    ret = WinHttpSendRequest( req, NULL, 0, NULL, 0, 0, 0 );
    err = GetLastError();
    if (!ret && (err == ERROR_WINHTTP_CANNOT_CONNECT || err == ERROR_WINHTTP_TIMEOUT))
    {
        skip("connection failed, skipping\n");
        goto done;
    }
    ok(ret, "failed to send request %u\n", GetLastError());

    setup_test( &info, winhttp_receive_response, __LINE__ );
    ret = WinHttpReceiveResponse( req, NULL );
    ok(ret, "failed to receive response %u\n", GetLastError());

    size = sizeof(status);
    ret = WinHttpQueryHeaders( req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, NULL, &status, &size, NULL );
    ok(ret, "failed unexpectedly %u\n", GetLastError());
    ok(status == 200, "request failed unexpectedly %u\n", status);

    ResetEvent( info.wait );
    setup_test( &info, winhttp_close_handle, __LINE__ );
    WinHttpCloseHandle( req );
    WaitForSingleObject( info.wait, INFINITE );

    setup_test( &info, winhttp_open_request, __LINE__ );
    req = WinHttpOpenRequest( con, NULL, tests_hello_html, NULL, NULL, NULL, 0 );
    ok(req != NULL, "failed to open a request %u\n", GetLastError());

    ret = WinHttpSetOption( req, WINHTTP_OPTION_CONTEXT_VALUE, &context, sizeof(struct info *) );
    ok(ret, "failed to set context value %u\n", GetLastError());

    setup_test( &info, winhttp_send_request, __LINE__ );
    ret = WinHttpSendRequest( req, NULL, 0, NULL, 0, 0, 0 );
    err = GetLastError();
    if (!ret && (err == ERROR_WINHTTP_CANNOT_CONNECT || err == ERROR_WINHTTP_TIMEOUT))
    {
        skip("connection failed, skipping\n");
        goto done;
    }
    ok(ret, "failed to send request %u\n", GetLastError());

    setup_test( &info, winhttp_receive_response, __LINE__ );
    ret = WinHttpReceiveResponse( req, NULL );
    ok(ret, "failed to receive response %u\n", GetLastError());

    size = sizeof(status);
    ret = WinHttpQueryHeaders( req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, NULL, &status, &size, NULL );
    ok(ret, "failed unexpectedly %u\n", GetLastError());
    ok(status == 200, "request failed unexpectedly %u\n", status);

    ResetEvent( info.wait );
    setup_test( &info, winhttp_close_handle, __LINE__ );
    WinHttpCloseHandle( req );
    WinHttpCloseHandle( req );
    WinHttpCloseHandle( con );
    WaitForSingleObject( info.wait, INFINITE );

    if (unload)
    {
        status = WaitForSingleObject( event, 0 );
        ok(status == WAIT_TIMEOUT, "got %08x\n", status);
    }

    setup_test( &info, winhttp_close_handle, __LINE__ );
    WinHttpCloseHandle( ses );
    WaitForSingleObject( info.wait, INFINITE );

    if (unload)
    {
        status = WaitForSingleObject( event, 100 );
        ok(status == WAIT_OBJECT_0, "got %08x\n", status);
    }


    ses = WinHttpOpen( user_agent, 0, NULL, NULL, 0 );
    ok(ses != NULL, "failed to open session %u\n", GetLastError());

    if (unload)
    {
        ret = WinHttpSetOption( ses, WINHTTP_OPTION_UNLOAD_NOTIFY_EVENT, &event, sizeof(event) );
        ok(ret, "failed to set unload option\n");
    }

    WinHttpSetStatusCallback( ses, check_notification, WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS, 0 );

    ret = WinHttpSetOption( ses, WINHTTP_OPTION_CONTEXT_VALUE, &context, sizeof(struct info *) );
    ok(ret, "failed to set context value %u\n", GetLastError());

    setup_test( &info, winhttp_connect, __LINE__ );
    con = WinHttpConnect( ses, test_winehq, 0, 0 );
    ok(con != NULL, "failed to open a connection %u\n", GetLastError());

    setup_test( &info, winhttp_open_request, __LINE__ );
    req = WinHttpOpenRequest( con, NULL, tests_hello_html, NULL, NULL, NULL, 0 );
    ok(req != NULL, "failed to open a request %u\n", GetLastError());

    ret = WinHttpSetOption( req, WINHTTP_OPTION_CONTEXT_VALUE, &context, sizeof(struct info *) );
    ok(ret, "failed to set context value %u\n", GetLastError());

    setup_test( &info, winhttp_send_request, __LINE__ );
    ret = WinHttpSendRequest( req, NULL, 0, NULL, 0, 0, 0 );
    err = GetLastError();
    if (!ret && (err == ERROR_WINHTTP_CANNOT_CONNECT || err == ERROR_WINHTTP_TIMEOUT))
    {
        skip("connection failed, skipping\n");
        goto done;
    }
    ok(ret, "failed to send request %u\n", GetLastError());

    setup_test( &info, winhttp_receive_response, __LINE__ );
    ret = WinHttpReceiveResponse( req, NULL );
    ok(ret, "failed to receive response %u\n", GetLastError());

    size = sizeof(status);
    ret = WinHttpQueryHeaders( req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, NULL, &status, &size, NULL );
    ok(ret, "failed unexpectedly %u\n", GetLastError());
    ok(status == 200, "request failed unexpectedly %u\n", status);

    ResetEvent( info.wait );
    setup_test( &info, winhttp_close_handle, __LINE__ );
    WinHttpCloseHandle( req );
    WaitForSingleObject( info.wait, INFINITE );

    setup_test( &info, winhttp_open_request, __LINE__ );
    req = WinHttpOpenRequest( con, NULL, tests_hello_html, NULL, NULL, NULL, 0 );
    ok(req != NULL, "failed to open a request %u\n", GetLastError());

    ret = WinHttpSetOption( req, WINHTTP_OPTION_CONTEXT_VALUE, &context, sizeof(struct info *) );
    ok(ret, "failed to set context value %u\n", GetLastError());

    setup_test( &info, winhttp_send_request, __LINE__ );
    ret = WinHttpSendRequest( req, NULL, 0, NULL, 0, 0, 0 );
    err = GetLastError();
    if (!ret && (err == ERROR_WINHTTP_CANNOT_CONNECT || err == ERROR_WINHTTP_TIMEOUT))
    {
        skip("connection failed, skipping\n");
        goto done;
    }
    ok(ret, "failed to send request %u\n", GetLastError());

    setup_test( &info, winhttp_receive_response, __LINE__ );
    ret = WinHttpReceiveResponse( req, NULL );
    ok(ret, "failed to receive response %u\n", GetLastError());

    size = sizeof(status);
    ret = WinHttpQueryHeaders( req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, NULL, &status, &size, NULL );
    ok(ret, "failed unexpectedly %u\n", GetLastError());
    ok(status == 200, "request failed unexpectedly %u\n", status);

    setup_test( &info, winhttp_close_handle, __LINE__ );
done:
    WinHttpCloseHandle( req );
    WinHttpCloseHandle( con );
    WaitForSingleObject( info.wait, INFINITE );

    if (unload)
    {
        status = WaitForSingleObject( event, 0 );
        ok(status == WAIT_TIMEOUT, "got %08x\n", status);
    }

    setup_test( &info, winhttp_close_handle, __LINE__ );
    WinHttpCloseHandle( ses );
    WaitForSingleObject( info.wait, INFINITE );
    CloseHandle( info.wait );

    if (unload)
    {
        status = WaitForSingleObject( event, 100 );
        ok(status == WAIT_OBJECT_0, "got %08x\n", status);
    }

    CloseHandle( event );
}

static const struct notification redirect_test[] =
{
    { winhttp_connect,          WINHTTP_CALLBACK_STATUS_HANDLE_CREATED },
    { winhttp_open_request,     WINHTTP_CALLBACK_STATUS_HANDLE_CREATED },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_RESOLVING_NAME, NF_WINE_ALLOW },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_NAME_RESOLVED, NF_WINE_ALLOW },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER, NF_WINE_ALLOW },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER, NF_WINE_ALLOW },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_SENDING_REQUEST },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_REQUEST_SENT },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_REDIRECT },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RESOLVING_NAME, NF_ALLOW },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_NAME_RESOLVED, NF_ALLOW },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER, NF_ALLOW },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER, NF_ALLOW },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_SENDING_REQUEST },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_REQUEST_SENT },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_CLOSING_CONNECTION, NF_WINE_ALLOW },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_CONNECTION_CLOSED, NF_WINE_ALLOW },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING, NF_SIGNAL }
};

static void test_redirect( void )
{
    HANDLE ses, con, req;
    DWORD size, status, err;
    BOOL ret;
    struct info info, *context = &info;

    info.test  = redirect_test;
    info.count = sizeof(redirect_test) / sizeof(redirect_test[0]);
    info.index = 0;
    info.wait = CreateEventW( NULL, FALSE, FALSE, NULL );

    ses = WinHttpOpen( user_agent, 0, NULL, NULL, 0 );
    ok(ses != NULL, "failed to open session %u\n", GetLastError());

    WinHttpSetStatusCallback( ses, check_notification, WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS, 0 );

    ret = WinHttpSetOption( ses, WINHTTP_OPTION_CONTEXT_VALUE, &context, sizeof(struct info *) );
    ok(ret, "failed to set context value %u\n", GetLastError());

    setup_test( &info, winhttp_connect, __LINE__ );
    con = WinHttpConnect( ses, test_winehq, 0, 0 );
    ok(con != NULL, "failed to open a connection %u\n", GetLastError());

    setup_test( &info, winhttp_open_request, __LINE__ );
    req = WinHttpOpenRequest( con, NULL, tests_redirect, NULL, NULL, NULL, 0 );
    ok(req != NULL, "failed to open a request %u\n", GetLastError());

    setup_test( &info, winhttp_send_request, __LINE__ );
    ret = WinHttpSendRequest( req, NULL, 0, NULL, 0, 0, 0 );
    err = GetLastError();
    if (!ret && (err == ERROR_WINHTTP_CANNOT_CONNECT || err == ERROR_WINHTTP_TIMEOUT))
    {
        skip("connection failed, skipping\n");
        goto done;
    }
    ok(ret, "failed to send request %u\n", GetLastError());

    setup_test( &info, winhttp_receive_response, __LINE__ );
    ret = WinHttpReceiveResponse( req, NULL );
    ok(ret, "failed to receive response %u\n", GetLastError());

    size = sizeof(status);
    ret = WinHttpQueryHeaders( req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, NULL, &status, &size, NULL );
    ok(ret, "failed unexpectedly %u\n", GetLastError());
    ok(status == 200, "request failed unexpectedly %u\n", status);

    setup_test( &info, winhttp_close_handle, __LINE__ );
done:
    WinHttpCloseHandle( req );
    WinHttpCloseHandle( con );
    WinHttpCloseHandle( ses );
    WaitForSingleObject( info.wait, INFINITE );
    CloseHandle( info.wait );
}

static const struct notification async_test[] =
{
    { winhttp_connect,          WINHTTP_CALLBACK_STATUS_HANDLE_CREATED },
    { winhttp_open_request,     WINHTTP_CALLBACK_STATUS_HANDLE_CREATED },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_RESOLVING_NAME, NF_WINE_ALLOW },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_NAME_RESOLVED, NF_WINE_ALLOW },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER, NF_WINE_ALLOW },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER, NF_WINE_ALLOW },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_SENDING_REQUEST },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_REQUEST_SENT },
    { winhttp_send_request,     WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE, NF_SIGNAL },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED },
    { winhttp_receive_response, WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE, NF_SIGNAL },
    { winhttp_query_data,       WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE, NF_SIGNAL },
    { winhttp_read_data,        WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE, NF_ALLOW },
    { winhttp_read_data,        WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED, NF_ALLOW },
    { winhttp_read_data,        WINHTTP_CALLBACK_STATUS_READ_COMPLETE, NF_SIGNAL },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_CLOSING_CONNECTION, NF_WINE_ALLOW },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_CONNECTION_CLOSED, NF_WINE_ALLOW },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING },
    { winhttp_close_handle,     WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING, NF_SIGNAL }
};

static void test_async( void )
{
    HANDLE ses, con, req, event;
    DWORD size, status, err;
    BOOL ret, unload = TRUE;
    struct info info, *context = &info;
    char buffer[1024];

    info.test  = async_test;
    info.count = sizeof(async_test) / sizeof(async_test[0]);
    info.index = 0;
    info.wait = CreateEventW( NULL, FALSE, FALSE, NULL );

    ses = WinHttpOpen( user_agent, 0, NULL, NULL, WINHTTP_FLAG_ASYNC );
    ok(ses != NULL, "failed to open session %u\n", GetLastError());

    event = CreateEventW( NULL, FALSE, FALSE, NULL );
    ret = WinHttpSetOption( ses, WINHTTP_OPTION_UNLOAD_NOTIFY_EVENT, &event, sizeof(event) );
    if (!ret)
    {
        win_skip("Unload event not supported\n");
        unload = FALSE;
    }

    SetLastError( 0xdeadbeef );
    WinHttpSetStatusCallback( ses, check_notification, WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS, 0 );
    err = GetLastError();
    ok(err == ERROR_SUCCESS || broken(err == 0xdeadbeef) /* < win7 */, "got %u\n", err);

    SetLastError( 0xdeadbeef );
    ret = WinHttpSetOption( ses, WINHTTP_OPTION_CONTEXT_VALUE, &context, sizeof(struct info *) );
    err = GetLastError();
    ok(ret, "failed to set context value %u\n", err);
    ok(err == ERROR_SUCCESS || broken(err == 0xdeadbeef) /* < win7 */, "got %u\n", err);

    setup_test( &info, winhttp_connect, __LINE__ );
    SetLastError( 0xdeadbeef );
    con = WinHttpConnect( ses, test_winehq, 0, 0 );
    err = GetLastError();
    ok(con != NULL, "failed to open a connection %u\n", err);
    ok(err == ERROR_SUCCESS || broken(err == WSAEINVAL) /* < win7 */, "got %u\n", err);

    setup_test( &info, winhttp_open_request, __LINE__ );
    SetLastError( 0xdeadbeef );
    req = WinHttpOpenRequest( con, NULL, tests_hello_html, NULL, NULL, NULL, 0 );
    err = GetLastError();
    ok(req != NULL, "failed to open a request %u\n", err);
    ok(err == ERROR_SUCCESS, "got %u\n", err);

    setup_test( &info, winhttp_send_request, __LINE__ );
    SetLastError( 0xdeadbeef );
    ret = WinHttpSendRequest( req, NULL, 0, NULL, 0, 0, 0 );
    err = GetLastError();
    if (!ret && (err == ERROR_WINHTTP_CANNOT_CONNECT || err == ERROR_WINHTTP_TIMEOUT))
    {
        skip("connection failed, skipping\n");
        WinHttpCloseHandle( req );
        WinHttpCloseHandle( con );
        WinHttpCloseHandle( ses );
        CloseHandle( info.wait );
        return;
    }
    ok(ret, "failed to send request %u\n", err);
    ok(err == ERROR_SUCCESS, "got %u\n", err);

    WaitForSingleObject( info.wait, INFINITE );

    setup_test( &info, winhttp_receive_response, __LINE__ );
    SetLastError( 0xdeadbeef );
    ret = WinHttpReceiveResponse( req, NULL );
    err = GetLastError();
    ok(ret, "failed to receive response %u\n", err);
    ok(err == ERROR_SUCCESS, "got %u\n", err);

    WaitForSingleObject( info.wait, INFINITE );

    size = sizeof(status);
    SetLastError( 0xdeadbeef );
    ret = WinHttpQueryHeaders( req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, NULL, &status, &size, NULL );
    err = GetLastError();
    ok(ret, "failed unexpectedly %u\n", err);
    ok(status == 200, "request failed unexpectedly %u\n", status);
    ok(err == ERROR_SUCCESS || broken(err == 0xdeadbeef) /* < win7 */, "got %u\n", err);

    setup_test( &info, winhttp_query_data, __LINE__ );
    SetLastError( 0xdeadbeef );
    ret = WinHttpQueryDataAvailable( req, NULL );
    err = GetLastError();
    ok(ret, "failed to query data available %u\n", err);
    ok(err == ERROR_SUCCESS || err == ERROR_IO_PENDING || broken(err == 0xdeadbeef) /* < win7 */, "got %u\n", err);

    WaitForSingleObject( info.wait, INFINITE );

    setup_test( &info, winhttp_read_data, __LINE__ );
    ret = WinHttpReadData( req, buffer, sizeof(buffer), NULL );
    ok(ret, "failed to read data %u\n", err);

    WaitForSingleObject( info.wait, INFINITE );

    setup_test( &info, winhttp_close_handle, __LINE__ );
    WinHttpCloseHandle( req );
    WinHttpCloseHandle( con );

    if (unload)
    {
        status = WaitForSingleObject( event, 0 );
        ok(status == WAIT_TIMEOUT, "got %08x\n", status);
    }
    WinHttpCloseHandle( ses );
    WaitForSingleObject( info.wait, INFINITE );

    if (unload)
    {
        status = WaitForSingleObject( event, 2000 );
        ok(status == WAIT_OBJECT_0, "got %08x\n", status);
    }
    CloseHandle( event );
    CloseHandle( info.wait );
}

START_TEST (notification)
{
    test_connection_cache();
    test_redirect();
    test_async();
}
