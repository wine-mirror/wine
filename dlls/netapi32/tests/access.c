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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <wine/test.h>
#include <winbase.h>
#include <winerror.h>
#include <lmaccess.h>
#include <lmerr.h>
#include <lmapibuf.h>

WCHAR user_name[UNLEN + 1];
WCHAR computer_name[MAX_COMPUTERNAME_LENGTH + 1];

const WCHAR sAdminUserName[] = {'A','d','m','i','n','i','s','t','r','a','t',
                                'o','r',0};
const WCHAR sGuestUserName[] = {'G','u','e','s','t',0};
const WCHAR sNonexistentUser[] = {'N','o','n','e','x','i','s','t','e','n','t',' ',
                                'U','s','e','r',0};
const WCHAR sBadNetPath[] = {'\\','\\','B','a',' ',' ','p','a','t','h',0};
const WCHAR sInvalidName[] = {'\\',0};
const WCHAR sInvalidName2[] = {'\\','\\',0};
const WCHAR sEmptyStr[] = { 0 };


void init_access_tests(void)
{
    DWORD dwSize;

    user_name[0] = 0;
    dwSize = sizeof(user_name);
    ok(GetUserNameW(user_name, &dwSize), "User Name Retrieved");

    computer_name[0] = 0;
    dwSize = sizeof(computer_name);
    ok(GetComputerNameW(computer_name, &dwSize), "Computer Name Retrieved");
}

void run_usergetinfo_tests(void)
{
    PUSER_INFO_0 ui0 = NULL;
    PUSER_INFO_10 ui10 = NULL;
    DWORD dwSize;

    /* Level 0 */
    ok(NetUserGetInfo(NULL, sAdminUserName, 0, (LPBYTE *)&ui0) == NERR_Success,
       "NetUserGetInfo is successful");
    ok(!lstrcmpW(sAdminUserName, ui0->usri0_name), "This is really user name");
    NetApiBufferSize(ui0, &dwSize);
    ok(dwSize >= (sizeof(USER_INFO_0) +
                  (lstrlenW(ui0->usri0_name) + 1) * sizeof(WCHAR)),
       "Is allocated with NetApiBufferAllocate");

    /* Level 10 */
    ok(NetUserGetInfo(NULL, sAdminUserName, 10, (LPBYTE *)&ui10) == NERR_Success,
       "NetUserGetInfo is successful");
    ok(!lstrcmpW(sAdminUserName, ui10->usri10_name), "This is really user name");
    NetApiBufferSize(ui10, &dwSize);
    ok(dwSize >= (sizeof(USER_INFO_10) +
                  (lstrlenW(ui10->usri10_name) + 1 +
                   lstrlenW(ui10->usri10_comment) + 1 +
                   lstrlenW(ui10->usri10_usr_comment) + 1 +
                   lstrlenW(ui10->usri10_full_name) + 1) * sizeof(WCHAR)),
       "Is allocated with NetApiBufferAllocate");

    NetApiBufferFree(ui0);
    NetApiBufferFree(ui10);

    /* errors handling */
    ok(NetUserGetInfo(NULL, sAdminUserName, 10000, (LPBYTE *)&ui0) == ERROR_INVALID_LEVEL,
       "Invalid Level");
    ok(NetUserGetInfo(NULL, sNonexistentUser, 0, (LPBYTE *)&ui0) == NERR_UserNotFound,
       "Invalid User Name");
    todo_wine {
        /* FIXME - Currently Wine can't verify whether the network path is good or bad */
        ok(NetUserGetInfo(sBadNetPath, sAdminUserName, 0, (LPBYTE *)&ui0) == ERROR_BAD_NETPATH,
           "Bad Network Path");
    }
    ok(NetUserGetInfo(sEmptyStr, sAdminUserName, 0, (LPBYTE *)&ui0) == ERROR_BAD_NETPATH,
       "Bad Network Path");
    ok(NetUserGetInfo(sInvalidName, sAdminUserName, 0, (LPBYTE *)&ui0) == ERROR_INVALID_NAME,
       "Invalid Server Name");
    ok(NetUserGetInfo(sInvalidName2, sAdminUserName, 0, (LPBYTE *)&ui0) == ERROR_INVALID_NAME,
       "Invalid Server Name");
}

/* checks Level 1 of NetQueryDisplayInformation */
void run_querydisplayinformation1_tests(void)
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
           "Information Retrieved");
        rec = Buffer;
        for(; EntryCount > 0; EntryCount--)
        {
            if (!lstrcmpW(rec->usri1_name, sAdminUserName))
            {
                ok(!hasAdmin, "One admin user");
                ok(rec->usri1_flags & UF_SCRIPT, "UF_SCRIPT flag is set");
                ok(rec->usri1_flags & UF_NORMAL_ACCOUNT, "UF_NORMAL_ACCOUNT flag is set");
                hasAdmin = TRUE;
            }
            else if (!lstrcmpW(rec->usri1_name, sGuestUserName))
            {
                ok(!hasGuest, "One guest record");
                ok(rec->usri1_flags & UF_SCRIPT, "UF_SCRIPT flag is set");
                ok(rec->usri1_flags & UF_NORMAL_ACCOUNT, "UF_NORMAL_ACCOUNT flag is set");
                hasGuest = TRUE;
            }

            i = rec->usri1_next_index;
            rec++;
        }

        NetApiBufferFree(Buffer);
    } while (Result == ERROR_MORE_DATA);

    ok(hasAdmin, "Has Administrator account");
    ok(hasGuest, "Has Guest account");
}

START_TEST(access)
{
    init_access_tests();
    run_usergetinfo_tests();
    run_querydisplayinformation1_tests();
}
