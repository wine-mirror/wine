/*
 * Copyright 2020 Arkadiusz Hiler for CodeWeavers
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

#include <windows.h>
#include <stdio.h>

#include "wine/test.h"

static BOOL is_process_elevated(void)
{
    HANDLE token;
    if (OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &token ))
    {
        TOKEN_ELEVATION_TYPE type;
        DWORD size;
        BOOL ret;

        ret = GetTokenInformation( token, TokenElevationType, &type, sizeof(type), &size );
        CloseHandle( token );
        return (ret && type == TokenElevationTypeFull);
    }
    return FALSE;
}

static DWORD runcmd(const char* cmd)
{
    STARTUPINFOA si = { sizeof(STARTUPINFOA) };
    PROCESS_INFORMATION pi;
    char* wcmd;
    DWORD rc;

    /* Create a writable copy for CreateProcessA() */
    wcmd = strdup(cmd);

    rc = CreateProcessA(NULL, wcmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    free(wcmd);

    if (!rc)
        return 260;

    rc = WaitForSingleObject(pi.hProcess, 5000);

    if (rc == WAIT_OBJECT_0)
        GetExitCodeProcess(pi.hProcess, &rc);
    else
        TerminateProcess(pi.hProcess, 1);

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    return rc;
}

static void test_hardlink(void)
{
    DWORD rc;
    BOOL boolrc;
    HANDLE hfile;
    BY_HANDLE_FILE_INFORMATION info;

    hfile = CreateFileA("file", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL, NULL);
    ok(hfile != INVALID_HANDLE_VALUE, "failed to create a file\n");
    CloseHandle(hfile);

    rc = runcmd("fsutil");
    if (rc == 1 && !is_process_elevated())
    {
        win_skip("Cannot run fsutil without elevated privileges on Windows <= 7\n");
        return;
    }
    ok(rc == 0, "failed to run fsutil\n");

    rc = runcmd("fsutil hardlink");
    ok(rc == 0, "failed to run fsutil hardlink\n");

    rc = runcmd("fsutil hardlink create link file");
    ok(rc == 0, "failed to create a hardlink\n");

    hfile = CreateFileA("link", GENERIC_READ, 0, NULL, OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL, NULL);
    ok(hfile != INVALID_HANDLE_VALUE, "failed to open the hardlink\n");
    boolrc = GetFileInformationByHandle(hfile, &info);
    ok(boolrc, "failed to get info about hardlink, error %#lx\n", GetLastError());
    CloseHandle(hfile);

    ok(info.nNumberOfLinks == 2, "our link is not a hardlink\n");

    rc = runcmd("fsutil hardlink create link file");
    ok(rc == 1, "fsutil didn't complain about already existing destination\n");

    rc = runcmd("fsutil hardlink create newlink nonexistingfile");
    ok(rc == 1, "fsutil didn't complain about nonexisting source file\n");

    boolrc = DeleteFileA("link");
    ok(boolrc, "failed to delete the hardlink, error %#lx\n", GetLastError());
    boolrc = DeleteFileA("file");
    ok(boolrc, "failed to delete the file, error %#lx\n", GetLastError());
}

START_TEST(fsutil)
{
    char tmpdir[MAX_PATH];

    GetTempPathA(MAX_PATH, tmpdir);
    SetCurrentDirectoryA(tmpdir);

    test_hardlink();
}
