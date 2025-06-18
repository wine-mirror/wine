/*
 * Copyright 2002 Andriy Palamarchuk
 *
 * Conformance test of the access functions.
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

#include <windef.h>
#include <winbase.h>
#include <winerror.h>
#include <lmaccess.h>
#include <lmerr.h>
#include <lmapibuf.h>

#include "wine/test.h"

static WCHAR user_name[UNLEN + 1];
static WCHAR computer_name[MAX_COMPUTERNAME_LENGTH + 1];

static WCHAR sTooLongName[] = L"This is a bad username";
static WCHAR sTooLongPassword[] = L"abcdefghabcdefghabcdefghabcdefghabcdefgh"
        "abcdefghabcdefghabcdefghabcdefghabcdefghabcdefghabcdefghabcdefghabcdefgh"
        "abcdefghabcdefghabcdefghabcdefghabcdefghabcdefghabcdefghabcdefghabcdefgh"
        "abcdefghabcdefghabcdefghabcdefghabcdefghabcdefghabcdefghabcdefghabcdefgha";

static WCHAR sTestUserOldPass[] = L"OldPassW0rdSet!~";

static DWORD (WINAPI *pDavGetHTTPFromUNCPath)(LPCWSTR,LPWSTR,LPDWORD);
static DWORD (WINAPI *pDavGetUNCFromHTTPPath)(LPCWSTR,LPWSTR,LPDWORD);

static NET_API_STATUS create_test_user(void)
{
    USER_INFO_1 usri;

    usri.usri1_name = (WCHAR *)L"testuser";
    usri.usri1_password = sTestUserOldPass;
    usri.usri1_priv = USER_PRIV_USER;
    usri.usri1_home_dir = NULL;
    usri.usri1_comment = NULL;
    usri.usri1_flags = UF_SCRIPT;
    usri.usri1_script_path = NULL;

    return NetUserAdd(NULL, 1, (BYTE *)&usri, NULL);
}

static NET_API_STATUS delete_test_user(void)
{
    return NetUserDel(NULL, L"testuser");
}

static void run_usergetinfo_tests(void)
{
    NET_API_STATUS rc;
    PUSER_INFO_0 ui0 = NULL;
    PUSER_INFO_10 ui10 = NULL;
    DWORD dwSize;

    if((rc = create_test_user()) != NERR_Success )
    {
        skip("Skipping usergetinfo_tests, create_test_user failed: 0x%08lx\n", rc);
        return;
    }

    /* Level 0 */
    rc = NetUserGetInfo(NULL, L"testuser", 0, (BYTE **)&ui0);
    ok(rc == NERR_Success, "NetUserGetInfo level 0 failed: 0x%08lx.\n", rc);
    ok(!wcscmp(L"testuser", ui0->usri0_name), "Got level 0 name %s.\n", debugstr_w(ui0->usri0_name));
    NetApiBufferSize(ui0, &dwSize);
    ok(dwSize >= (sizeof(USER_INFO_0) + (wcslen(ui0->usri0_name) + 1) * sizeof(WCHAR)),
       "Is allocated with NetApiBufferAllocate\n");

    /* Level 10 */
    rc = NetUserGetInfo(NULL, L"testuser", 10, (BYTE **)&ui10);
    ok(rc == NERR_Success, "NetUserGetInfo level 10 failed: 0x%08lx.\n", rc);
    ok(!wcscmp(L"testuser", ui10->usri10_name), "Got level 10 name %s.\n", debugstr_w(ui10->usri10_name));
    NetApiBufferSize(ui10, &dwSize);
    ok(dwSize >= (sizeof(USER_INFO_10) +
                  (wcslen(ui10->usri10_name) + 1 +
                   wcslen(ui10->usri10_comment) + 1 +
                   wcslen(ui10->usri10_usr_comment) + 1 +
                   wcslen(ui10->usri10_full_name) + 1) * sizeof(WCHAR)),
       "Is allocated with NetApiBufferAllocate\n");

    NetApiBufferFree(ui0);
    NetApiBufferFree(ui10);

    /* NetUserGetInfo should always work for the current user. */
    rc = NetUserGetInfo(NULL, user_name, 0, (BYTE **)&ui0);
    ok(rc == NERR_Success, "NetUsetGetInfo for current user failed: 0x%08lx.\n", rc);
    NetApiBufferFree(ui0);

    /* errors handling */
    rc = NetUserGetInfo(NULL, L"testuser", 10000, (BYTE **)&ui0);
    ok(rc == ERROR_INVALID_LEVEL,"Invalid Level: rc=%ld\n",rc);
    rc = NetUserGetInfo(NULL, L"Nonexistent User", 0, (BYTE **)&ui0);
    ok(rc == NERR_UserNotFound,"Invalid User Name: rc=%ld\n",rc);
    todo_wine {
        /* FIXME - Currently Wine can't verify whether the network path is good or bad */
        rc = NetUserGetInfo(L"\\\\Ba  path", L"testuser", 0, (BYTE **)&ui0);
        ok(rc == ERROR_BAD_NETPATH ||
           rc == ERROR_NETWORK_UNREACHABLE ||
           rc == RPC_S_SERVER_UNAVAILABLE ||
           rc == NERR_WkstaNotStarted || /* workstation service not running */
           rc == RPC_S_INVALID_NET_ADDR, /* Some Win7 */
           "Bad Network Path: rc=%ld\n",rc);
    }
    rc = NetUserGetInfo(L"", L"testuser", 0, (BYTE **)&ui0);
    ok(rc == ERROR_BAD_NETPATH || rc == NERR_Success,
       "Bad Network Path: rc=%ld\n",rc);
    rc = NetUserGetInfo(L"\\", L"testuser", 0, (BYTE **)&ui0);
    ok(rc == ERROR_INVALID_NAME || rc == ERROR_INVALID_HANDLE,"Invalid Server Name: rc=%ld\n",rc);
    rc = NetUserGetInfo(L"\\\\", L"testuser", 0, (BYTE **)&ui0);
    ok(rc == ERROR_INVALID_NAME || rc == ERROR_INVALID_HANDLE,"Invalid Server Name: rc=%ld\n",rc);

    if(delete_test_user() != NERR_Success)
        trace("Deleting the test user failed. You might have to manually delete it.\n");
}

/* Checks Level 1 of NetQueryDisplayInformation */
static void run_querydisplayinformation1_tests(void)
{
    PNET_DISPLAY_USER Buffer, rec;
    DWORD Result, EntryCount;
    DWORD i = 0;
    BOOL hasAdmin = FALSE;
    BOOL hasGuest = FALSE;

    do
    {
        Result = NetQueryDisplayInformation(
            NULL, 1, i, 1000, MAX_PREFERRED_LENGTH, &EntryCount,
            (PVOID *)&Buffer);

        ok((Result == ERROR_SUCCESS) || (Result == ERROR_MORE_DATA),
           "Information Retrieved\n");
        rec = Buffer;
        for(; EntryCount > 0; EntryCount--)
        {
            if (rec->usri1_user_id == DOMAIN_USER_RID_ADMIN)
            {
                ok(!hasAdmin, "One admin user\n");
                ok(rec->usri1_flags & UF_SCRIPT, "UF_SCRIPT flag is set\n");
                ok(rec->usri1_flags & UF_NORMAL_ACCOUNT, "UF_NORMAL_ACCOUNT flag is set\n");
                hasAdmin = TRUE;
            }
            else if (rec->usri1_user_id == DOMAIN_USER_RID_GUEST)
            {
                ok(!hasGuest, "One guest record\n");
                ok(rec->usri1_flags & UF_SCRIPT, "UF_SCRIPT flag is set\n");
                ok(rec->usri1_flags & UF_NORMAL_ACCOUNT, "UF_NORMAL_ACCOUNT flag is set\n");
                hasGuest = TRUE;
            }

            i = rec->usri1_next_index;
            rec++;
        }

        NetApiBufferFree(Buffer);
    } while (Result == ERROR_MORE_DATA);

    ok(hasAdmin, "Doesn't have 'Administrator' account\n");
}

static void run_usermodalsget_tests(void)
{
    NET_API_STATUS rc;
    USER_MODALS_INFO_2 * umi2 = NULL;

    rc = NetUserModalsGet(NULL, 2, (BYTE **)&umi2);
    ok(rc == ERROR_SUCCESS, "NetUserModalsGet failed, rc = %ld\n", rc);

    if (umi2)
        NetApiBufferFree(umi2);
}

static void run_userhandling_tests(void)
{
    NET_API_STATUS ret;
    USER_INFO_1 usri;

    usri.usri1_priv = USER_PRIV_USER;
    usri.usri1_home_dir = NULL;
    usri.usri1_comment = NULL;
    usri.usri1_flags = UF_SCRIPT;
    usri.usri1_script_path = NULL;

    usri.usri1_name = sTooLongName;
    usri.usri1_password = sTestUserOldPass;

    ret = NetUserAdd(NULL, 1, (BYTE *)&usri, NULL);
    if (ret == ERROR_ACCESS_DENIED)
    {
        skip("not enough permissions to add a user\n");
        return;
    }
    else
        ok(ret == NERR_BadUsername, "Got %lu.\n", ret);

    usri.usri1_name = (WCHAR *)L"testuser";
    usri.usri1_password = sTooLongPassword;

    ret = NetUserAdd(NULL, 1, (BYTE *)&usri, NULL);
    ok(ret == NERR_PasswordTooShort || ret == ERROR_ACCESS_DENIED /* Win2003 */,
       "Adding user with too long password returned 0x%08lx\n", ret);

    usri.usri1_name = sTooLongName;
    usri.usri1_password = sTooLongPassword;

    ret = NetUserAdd(NULL, 1, (BYTE *)&usri, NULL);
    /* NT4 doesn't have a problem with the username so it will report the too long password
     * as the error. NERR_PasswordTooShort is reported for all kind of password related errors.
     */
    ok(ret == NERR_BadUsername || ret == NERR_PasswordTooShort,
            "Adding user with too long username/password returned 0x%08lx\n", ret);

    usri.usri1_name = (WCHAR *)L"testuser";
    usri.usri1_password = sTestUserOldPass;

    ret = NetUserAdd(NULL, 5, (BYTE *)&usri, NULL);
    ok(ret == ERROR_INVALID_LEVEL, "Adding user with level 5 returned 0x%08lx\n", ret);

    ret = NetUserAdd(NULL, 1, (BYTE *)&usri, NULL);
    if(ret == ERROR_ACCESS_DENIED)
    {
        skip("Insufficient permissions to add users. Skipping test.\n");
        return;
    }
    if(ret == NERR_UserExists)
    {
        skip("User already exists, skipping test to not mess up the system\n");
        return;
    }

    ok(!ret, "Got %lu.\n", ret);

    /* On Windows XP (and newer), calling NetUserChangePassword with a NULL
     * domainname parameter creates a user home directory, iff the machine is
     * not member of a domain.
     * Using \\127.0.0.1 as domain name does not work on standalone machines
     * either, unless the ForceGuest option in the registry is turned off.
     * So let's not test NetUserChangePassword for now.
     */

    ret = NetUserDel(NULL, L"testuser");
    ok(ret == NERR_Success, "Deleting the user failed.\n");

    ret = NetUserDel(NULL, L"testuser");
    ok(ret == NERR_UserNotFound, "Deleting a nonexistent user returned 0x%08lx\n",ret);
}

static void run_localgroupgetinfo_tests(void)
{
    NET_API_STATUS status;
    PLOCALGROUP_INFO_1 lgi = NULL;
    PLOCALGROUP_MEMBERS_INFO_3 buffer = NULL;
    DWORD entries_read = 0, total_entries =0;
    int i;

    status = NetLocalGroupGetInfo(NULL, L"Administrators", 1, (BYTE **)&lgi);
    ok(status == NERR_Success || broken(status == NERR_GroupNotFound),
       "NetLocalGroupGetInfo unexpectedly returned %ld\n", status);
    if (status != NERR_Success) return;

    trace("Local groupname:%s\n", wine_dbgstr_w( lgi->lgrpi1_name));
    trace("Comment: %s\n", wine_dbgstr_w( lgi->lgrpi1_comment));

    NetApiBufferFree(lgi);

    status = NetLocalGroupGetMembers(NULL, L"Administrators", 3, (BYTE **)&buffer,
            MAX_PREFERRED_LENGTH, &entries_read, &total_entries, NULL);
    ok(status == NERR_Success, "NetLocalGroupGetMembers unexpectedly returned %ld\n", status);
    ok(entries_read > 0 && total_entries > 0, "Amount of entries is unexpectedly 0\n");

    for(i=0;i<entries_read;i++)
        trace("domain and name: %s\n", wine_dbgstr_w(buffer[i].lgrmi3_domainandname));

    NetApiBufferFree(buffer);
}

static void test_DavGetHTTPFromUNCPath(void)
{
    static const struct
    {
        const WCHAR *path;
        DWORD ret;
        const WCHAR *ret_path;
        DWORD broken_ret; /* < Win10 1709 */
        BOOL todo;
    }
    tests[] =
    {
        {L"",                       ERROR_BAD_NET_NAME, NULL, ERROR_INVALID_PARAMETER, TRUE},
        {L"c:\\",                   ERROR_BAD_NET_NAME, NULL, ERROR_INVALID_PARAMETER, TRUE},
        {L"\\\\",                   ERROR_BAD_NET_NAME, NULL, ERROR_INVALID_PARAMETER, TRUE},
        {L"\\a\\b",                 ERROR_BAD_NET_NAME, NULL, ERROR_INVALID_PARAMETER, TRUE},
        {L"\\\\a",                  ERROR_SUCCESS, L"http://a"},
        {L"\\\\a\\",                ERROR_SUCCESS, L"http://a"},
        {L"\\\\a\\b",               ERROR_SUCCESS, L"http://a/b"},
        {L"\\\\a\\b\\",             ERROR_SUCCESS, L"http://a/b"},
        {L"\\\\a\\b\\c",            ERROR_SUCCESS, L"http://a/b/c"},
        {L"\\\\a@SSL\\b",           ERROR_SUCCESS, L"https://a/b"},
        {L"\\\\a@ssl\\b",           ERROR_SUCCESS, L"https://a/b"},
        {L"\\\\a@tls\\b",           ERROR_INVALID_PARAMETER},
        {L"\\\\a@SSL@443\\b",       ERROR_SUCCESS, L"https://a/b"},
        {L"\\\\a@SSL@80\\b",        ERROR_SUCCESS, L"https://a:80/b"},
        {L"\\\\a@80@SSL\\b",        ERROR_INVALID_PARAMETER},
        {L"\\\\a@80\\b",            ERROR_SUCCESS, L"http://a/b"},
        {L"\\\\a@8080\\b",          ERROR_SUCCESS, L"http://a:8080/b"},
        {L"\\\\a\\b/",              ERROR_SUCCESS, L"http://a/b"},
        {L"\\\\a/b",                ERROR_SUCCESS, L"http://a/b"},
        {L"\\\\a.\\b",              ERROR_SUCCESS, L"http://a./b"},
        {L"\\\\.a\\b",              ERROR_SUCCESS, L"http://.a/b"},
        {L"//a/b",                  ERROR_SUCCESS, L"http://a/b", ERROR_INVALID_PARAMETER, TRUE},
        {L"\\\\a\\\\",              ERROR_BAD_NET_NAME, NULL, ERROR_SUCCESS, TRUE},
        {L"\\\\\\a\\",              ERROR_BAD_NET_NAME, NULL, ERROR_SUCCESS, TRUE},
        {L"\\\\a\\b\\\\",           ERROR_BAD_NET_NAME, NULL, ERROR_SUCCESS, TRUE},
        {L"\\\\.\\a",               ERROR_BAD_NET_NAME, NULL, ERROR_SUCCESS, TRUE},
        {L"\\\\a\\b:",              ERROR_BAD_NET_NAME, NULL, ERROR_SUCCESS, TRUE},
    };
    WCHAR buf[MAX_PATH];
    DWORD i, ret, size;

    if (!pDavGetHTTPFromUNCPath)
    {
        win_skip( "DavGetHTTPFromUNCPath is missing\n" );
        return;
    }

    if (0) /* crashes on Windows */
    {
        ret = pDavGetHTTPFromUNCPath(NULL, NULL, NULL);
        ok(ret == ERROR_INVALID_PARAMETER, "got %lu\n", ret);

        size = 0;
        ret = pDavGetHTTPFromUNCPath(L"", buf, &size);
        ok(ret == ERROR_INVALID_PARAMETER, "got %lu\n", ret);

        ret = pDavGetHTTPFromUNCPath(L"\\\\a\\b", buf, NULL);
        ok(ret == ERROR_INVALID_PARAMETER, "got %lu\n", ret);
    }

    ret = pDavGetHTTPFromUNCPath(L"", buf, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "got %lu\n", ret);

    size = 0;
    ret = pDavGetHTTPFromUNCPath(L"", NULL, &size);
    ok(ret == ERROR_INVALID_PARAMETER || ret == ERROR_BAD_NET_NAME /* Win10 1709+ */, "got %lu\n", ret);

    size = 0;
    ret = pDavGetHTTPFromUNCPath(L"\\\\a\\b", NULL, &size);
    ok(ret == ERROR_INSUFFICIENT_BUFFER, "got %lu\n", ret);

    buf[0] = 0;
    size = 0;
    ret = pDavGetHTTPFromUNCPath(L"\\\\a\\b", buf, &size);
    ok(ret == ERROR_INSUFFICIENT_BUFFER, "got %lu\n", ret);
    ok(size == 11, "got %lu\n", size);

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        buf[0] = 0;
        size = ARRAY_SIZE(buf);
        ret = pDavGetHTTPFromUNCPath( tests[i].path, buf, &size );
        todo_wine_if (tests[i].todo)
            ok(ret == tests[i].ret || broken(ret == tests[i].broken_ret),
                    "%lu: expected %lu got %lu\n", i, tests[i].ret, ret);
        if (!ret)
        {
            if (tests[i].ret_path)
                ok(!wcscmp(buf, tests[i].ret_path), "%lu: expected %s got %s\n",
                        i, wine_dbgstr_w(tests[i].ret_path), wine_dbgstr_w(buf));
            ok(size == wcslen(buf) + 1, "%lu: got %lu\n", i, size);
        }
        else
            ok(size == ARRAY_SIZE(buf), "%lu: wrong size %lu\n", i, size);
    }
}


static void test_DavGetUNCFromHTTPPath(void)
{
    static const struct
    {
        const WCHAR *path;
        DWORD ret;
        const WCHAR *ret_path;
        DWORD broken_ret; /* < Win10 1709 */
        BOOL todo;
    }
    tests[] =
    {
        {L"",                       ERROR_INVALID_PARAMETER},
        {L"http://server/path",     ERROR_SUCCESS, L"\\\\server\\DavWWWRoot\\path"},
        {L"https://host/path",      ERROR_SUCCESS, L"\\\\host@SSL\\DavWWWRoot\\path"},
        {L"\\\\server",             ERROR_INVALID_PARAMETER},
        {L"\\\\server\\path",       ERROR_INVALID_PARAMETER},
        {L"\\\\http://server/path", ERROR_INVALID_PARAMETER},
        {L"http://",                ERROR_BAD_NETPATH, NULL, ERROR_BAD_NET_NAME, TRUE},
        {L"http:",                  ERROR_BAD_NET_NAME, NULL, ERROR_INVALID_PARAMETER, TRUE},
        {L"http",                   ERROR_INVALID_PARAMETER},
        {L"http:server",            ERROR_BAD_NET_NAME, NULL, ERROR_INVALID_PARAMETER, TRUE},
        {L"http://server:80",       ERROR_SUCCESS, L"\\\\server\\DavWWWRoot"},
        {L"http://server:81",       ERROR_SUCCESS, L"\\\\server@81\\DavWWWRoot"},
        {L"https://server:80",      ERROR_SUCCESS, L"\\\\server@SSL@80\\DavWWWRoot"},
        {L"HTTP://server/path",     ERROR_SUCCESS, L"\\\\server\\DavWWWRoot\\path"},
        {L"http://server:65537",    ERROR_BAD_NETPATH, NULL, ERROR_SUCCESS, TRUE},
        {L"http://server/path/",    ERROR_SUCCESS, L"\\\\server\\DavWWWRoot\\path"},
        {L"http://server/path//",   ERROR_SUCCESS, L"\\\\server\\DavWWWRoot\\path", ERROR_BAD_NET_NAME, TRUE},
        {L"http://server:/path",    ERROR_BAD_NETPATH, NULL, ERROR_SUCCESS, TRUE},
        {L"http://server",          ERROR_SUCCESS, L"\\\\server\\DavWWWRoot"},
        {L"https://server:443",     ERROR_SUCCESS, L"\\\\server@SSL\\DavWWWRoot"},
    };
    WCHAR buf[MAX_PATH];
    DWORD i, ret, size;

    if (!pDavGetUNCFromHTTPPath)
    {
        win_skip( "DavGetUNCFromHTTPPath is missing\n" );
        return;
    }

    if (0) /* crashes on Windows */
    {
        ret = pDavGetUNCFromHTTPPath(NULL, NULL, NULL);
        ok(ret == ERROR_INVALID_PARAMETER, "got %lu\n", ret);

        ret = pDavGetUNCFromHTTPPath(L"http://server/path", buf, NULL);
        ok(ret == ERROR_INVALID_PARAMETER, "got %lu\n", ret);
    }

    ret = pDavGetUNCFromHTTPPath(L"", buf, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "got %lu\n", ret);

    size = 0;
    ret = pDavGetUNCFromHTTPPath(L"", NULL, &size);
    ok(ret == ERROR_INVALID_PARAMETER, "got %lu\n", ret);

    buf[0] = 0;
    size = 0;
    ret = pDavGetUNCFromHTTPPath(L"", buf, &size);
    ok(ret == ERROR_INVALID_PARAMETER, "got %lu\n", ret);

    size = 0;
    ret = pDavGetUNCFromHTTPPath(L"http://server/path", NULL, &size);
    ok( ret == ERROR_INSUFFICIENT_BUFFER, "got %lu\n", ret );

    buf[0] = 0;
    size = 0;
    ret = pDavGetUNCFromHTTPPath(L"http://server/path", buf, &size);
    ok(ret == ERROR_INSUFFICIENT_BUFFER, "got %lu\n", ret);
    ok(size == 25, "got %lu\n", size);

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        buf[0] = 0;
        size = ARRAY_SIZE(buf);
        ret = pDavGetUNCFromHTTPPath( tests[i].path, buf, &size );
        todo_wine_if (tests[i].todo)
            ok(ret == tests[i].ret || broken(ret == tests[i].broken_ret),
                    "%lu: expected %lu got %lu\n", i, tests[i].ret, ret);
        if (!ret)
        {
            if (tests[i].ret_path)
                ok(!wcscmp(buf, tests[i].ret_path), "%lu: expected %s got %s\n",
                        i, wine_dbgstr_w(tests[i].ret_path), wine_dbgstr_w(buf));
            ok(size == wcslen(buf) + 1, "%lu: got %lu\n", i, size);
        }
        else
            ok(size == ARRAY_SIZE(buf), "%lu: wrong size %lu\n", i, size);
    }
}

START_TEST(access)
{
    HMODULE hnetapi32 = GetModuleHandleA("netapi32.dll");
    DWORD size;
    BOOL ret;

    pDavGetHTTPFromUNCPath = (void*)GetProcAddress(hnetapi32, "DavGetHTTPFromUNCPath");
    pDavGetUNCFromHTTPPath = (void*)GetProcAddress(hnetapi32, "DavGetUNCFromHTTPPath");

    size = ARRAY_SIZE(user_name);
    ret = GetUserNameW(user_name, &size);
    ok(ret, "Failed to get user name, error %lu.\n", GetLastError());
    size = ARRAY_SIZE(computer_name);
    ret = GetComputerNameW(computer_name, &size);
    ok(ret, "Failed to get computer name, error %lu.\n", GetLastError());

    run_userhandling_tests();
    run_usergetinfo_tests();
    run_querydisplayinformation1_tests();
    run_usermodalsget_tests();
    run_localgroupgetinfo_tests();

    test_DavGetHTTPFromUNCPath();
    test_DavGetUNCFromHTTPPath();
}
