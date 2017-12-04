/*
 *    HttpApi tests
 *
 * Copyright 2017 Nikolay Sivov for CodeWeavers
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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winnt.h"
#include "winternl.h"
#include "http.h"

#include "wine/test.h"

static ULONG (WINAPI *pHttpCreateServerSession)(HTTPAPI_VERSION version, HTTP_SERVER_SESSION_ID *session_id,
        ULONG reserved);
static ULONG (WINAPI *pHttpCloseServerSession)(HTTP_SERVER_SESSION_ID session_id);
static ULONG (WINAPI *pHttpCreateUrlGroup)(HTTP_SERVER_SESSION_ID session_id, HTTP_URL_GROUP_ID *group_id,
        ULONG reserved);
static ULONG (WINAPI *pHttpCloseUrlGroup)(HTTP_URL_GROUP_ID group_id);

static void init(void)
{
    HMODULE mod = GetModuleHandleA("httpapi.dll");

#define X(f) p##f = (void *)GetProcAddress(mod, #f)
    X(HttpCreateServerSession);
    X(HttpCloseServerSession);
    X(HttpCreateUrlGroup);
    X(HttpCloseUrlGroup);
#undef X
}

static void test_HttpCreateHttpHandle(void)
{
    HANDLE handle, handle2;
    ULONG ret;
    BOOL b;

    ret = HttpCreateHttpHandle(NULL, 0);
todo_wine
    ok(ret == ERROR_INVALID_PARAMETER, "Unexpected ret value %u.\n", ret);

    /* Non-zero reserved parameter is accepted on XP/2k3. */
    handle = NULL;
    ret = HttpCreateHttpHandle(&handle, 0);
todo_wine {
    ok(!ret, "Unexpected ret value %u.\n", ret);
    ok(handle != NULL, "Unexpected handle value %p.\n", handle);
}
    handle2 = NULL;
    ret = HttpCreateHttpHandle(&handle2, 0);
todo_wine {
    ok(!ret, "Unexpected ret value %u.\n", ret);
    ok(handle2 != NULL && handle != handle2, "Unexpected handle %p.\n", handle2);
}
    b = CloseHandle(handle);
todo_wine
    ok(b, "Failed to close queue handle.\n");
}

static void test_HttpCreateServerSession(void)
{
    HTTP_SERVER_SESSION_ID session;
    HTTPAPI_VERSION version;
    ULONG ret;

    if (!pHttpCreateServerSession || !pHttpCloseServerSession)
    {
        skip("HttpCreateServerSession() is not supported.\n");
        return;
    }

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
    ULONG ret;

    if (!pHttpCreateUrlGroup)
    {
        skip("HttpCreateUrlGroup is not supported.\n");
        return;
    }

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

START_TEST(httpapi)
{
    HTTPAPI_VERSION version = { 1, 0 };
    ULONG ret;

    init();

    ret = HttpInitialize(version, HTTP_INITIALIZE_SERVER, NULL);
    ok(!ret, "Failed to initialize library, ret %u.\n", ret);

    test_HttpCreateHttpHandle();
    test_HttpCreateServerSession();
    test_HttpCreateUrlGroup();

    ret = HttpTerminate(HTTP_INITIALIZE_SERVER, NULL);
    ok(!ret, "Failed to terminate, ret %u.\n", ret);
}
