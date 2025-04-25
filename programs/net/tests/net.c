/*
 * Copyright 2024 Fabian Maurer
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

static HANDLE nul_file;

#define check_exit_code(x) ok(r == (x), "got exit code %ld, expected %d\n", r, (x))

/* Copied and modified from the reg.exe tests */
#define run_net_exe(c,r) run_net_exe_(__FILE__,__LINE__,c,r)
static BOOL run_net_exe_(const char *file, unsigned line, const char *cmd, DWORD *rc)
{
    STARTUPINFOA si = {sizeof(STARTUPINFOA)};
    PROCESS_INFORMATION pi;
    BOOL bret;
    DWORD ret;
    char cmdline[256];

    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput  = nul_file;
    si.hStdOutput = nul_file;
    si.hStdError  = nul_file;

    strcpy(cmdline, cmd);
    if (!CreateProcessA(NULL, cmdline, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
        return FALSE;

    ret = WaitForSingleObject(pi.hProcess, 10000);
    if (ret == WAIT_TIMEOUT)
        TerminateProcess(pi.hProcess, 1);

    bret = GetExitCodeProcess(pi.hProcess, rc);
    ok_(__FILE__, line)(bret, "GetExitCodeProcess failed: %ld\n", GetLastError());

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return bret;
}

static void test_stop(void)
{
    DWORD r;

    /* Stop non existing service */

    run_net_exe("net stop non-existing-service", &r);
    check_exit_code(2);
}

START_TEST(net)
{
    SECURITY_ATTRIBUTES secattr = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};

    nul_file = CreateFileA("NUL", GENERIC_READ | GENERIC_WRITE, 0, &secattr, OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL, NULL);

    test_stop();

    CloseHandle(nul_file);
}
