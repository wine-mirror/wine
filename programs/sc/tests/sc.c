/*
 * Copyright 2022 Torge Matthies for CodeWeavers
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
#include <winsvc.h>
#include <stdio.h>
#include "wine/test.h"

#define lok ok_(__FILE__,line)

#define TEST_SERVICE_NAME "wine_test_svc"
#define TEST_SERVICE_NAME2 "wine_test_svc_2"
#define TEST_SERVICE_BINARY "c:\\windows\\system32\\cmd.exe"
#define TEST_SERVICE_BINARY_START_BOOT "\\SystemRoot\\system32\\cmd.exe"
#define TEST_SERVICE_BINARY_START_SYSTEM "\\??\\" TEST_SERVICE_BINARY

#define SC_EXIT_SUCCESS ERROR_SUCCESS
#define SC_EXIT_INVALID_PARAMETER ERROR_INVALID_PARAMETER
#define SC_EXIT_CIRCULAR_DEPENDENCY ERROR_CIRCULAR_DEPENDENCY
#define SC_EXIT_SERVICE_DOES_NOT_EXIST ERROR_SERVICE_DOES_NOT_EXIST
#define SC_EXIT_SERVICE_EXISTS ERROR_SERVICE_EXISTS
#define SC_EXIT_INVALID_COMMAND_LINE ERROR_INVALID_COMMAND_LINE

static HANDLE nul_file;
static SC_HANDLE scmgr;

/* Copied and modified from the reg.exe tests */
#define run_sc_exe(c,r) run_sc_exe_(__FILE__,__LINE__,c,r)
static BOOL run_sc_exe_(const char *file, unsigned line, const char *cmd, DWORD *rc)
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
    lok(bret, "GetExitCodeProcess failed: %ld\n", GetLastError());

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return bret;
}

#define BROKEN_CREATE               0x000000001UL
#define BROKEN_BINPATH              0x000000002UL
#define BROKEN_TYPE                 0x000000004UL
#define BROKEN_START                0x000000008UL
#define BROKEN_ERROR                0x000000010UL
#define BROKEN_DEPEND               0x000000020UL
#define BROKEN_DISPLAY_NAME         0x000000040UL
#define BROKEN_DELAYED_AUTO_START   0x000000080UL
#define BROKEN_ALL                  ~0UL

#define SERVICE_DELAYED_AUTO_START (SERVICE_AUTO_START | 0x80000000)

#define check_service_definition(n,bi,t,s,e,de,di,br) check_service_definition_(__FILE__,__LINE__,n,bi,t,s,e,de,di,br)
static void check_service_definition_(const char *file, unsigned line, char const *name,
                                      const char *binpath, DWORD type, DWORD start, DWORD error,
                                      const char *depend, const char *display_name, DWORD broken)
{
    SERVICE_DELAYED_AUTO_START_INFO delayed_auto_info = {0};
    union {
        char buffer[8192];
        QUERY_SERVICE_CONFIGA config;
    } cfg;
    BOOL delayed_auto;
    SC_HANDLE svc;
    DWORD needed;
    BOOL ret;

    delayed_auto = !!(start & 0x80000000);
    start &= ~0x80000000;

    if (!scmgr)
        return;

    svc = OpenServiceA(scmgr, name, GENERIC_READ);
    todo_wine_if(broken & BROKEN_CREATE)
    lok(!!svc, "OpenServiceA failed: %ld\n", GetLastError());
    if (!svc)
        return;

    ret = QueryServiceConfigA(svc, &cfg.config, sizeof(cfg.buffer), &needed);
    lok(!!ret, "QueryServiceConfigA failed: %ld\n", GetLastError());
    if (!ret)
        goto done;

    ret = QueryServiceConfig2A(svc, SERVICE_CONFIG_DELAYED_AUTO_START_INFO, (LPBYTE)&delayed_auto_info,
                               sizeof(delayed_auto_info), &needed);
    lok(!!ret, "QueryServiceConfig2A(SERVICE_CONFIG_DELAYED_AUTO_START_INFO) failed: %ld\n", GetLastError());

#define check_str(a, b, msg) lok((a) && (b) && (a) != (b) && !strcmp((a), (b)), msg ": %s != %s\n", \
                                 debugstr_a((a)), debugstr_a((b)))
#define check_dw(a, b, msg) lok((a) == (b), msg ": 0x%lx != 0x%lx\n", a, b)

    todo_wine_if(broken & BROKEN_BINPATH)
    check_str(cfg.config.lpBinaryPathName, binpath, "Wrong binary path");
    todo_wine_if(broken & BROKEN_TYPE)
    check_dw(cfg.config.dwServiceType, type, "Wrong service type");
    todo_wine_if(broken & BROKEN_START)
    check_dw(cfg.config.dwStartType, start, "Wrong start type");
    todo_wine_if(broken & BROKEN_ERROR)
    check_dw(cfg.config.dwErrorControl, error, "Wrong error control");
    todo_wine_if(broken & BROKEN_DEPEND)
    check_str(cfg.config.lpDependencies, depend, "Wrong dependencies");
    todo_wine_if(broken & BROKEN_DISPLAY_NAME)
    check_str(cfg.config.lpDisplayName, display_name, "Wrong display name");
    todo_wine_if(broken & BROKEN_DELAYED_AUTO_START)
    check_dw((DWORD)delayed_auto_info.fDelayedAutostart, (DWORD)delayed_auto, "Wrong delayed autostart value");

#undef check_dw
#undef check_str

done:
    CloseServiceHandle(svc);
}

#define delete_service(n,e,b) delete_service_(__FILE__,__LINE__,n,e,b)
static void delete_service_(const char *file, unsigned line, const char *name, DWORD expected_status, BOOL broken)
{
    char command[256];
    BOOL bret;
    DWORD r;

    strcpy(command, "sc delete ");
    strcat(command, name);
    bret = run_sc_exe_(file, line, command, &r);
    lok(bret, "run_sc_exe failed\n");
    todo_wine_if(broken) lok(r == expected_status, "got exit code %ld, expected %ld\n", r, expected_status);
}

static void test_create_service(BOOL elevated)
{
    static struct {
        const char *param;
        DWORD expected_start_type;
        DWORD expected_service_type;
        const char * expected_binary_path;
        DWORD broken;
    } start_types[] = {
        { "boot type= kernel", SERVICE_BOOT_START, SERVICE_KERNEL_DRIVER, TEST_SERVICE_BINARY_START_BOOT,
          BROKEN_BINPATH | BROKEN_DISPLAY_NAME },
        { "system type= kernel", SERVICE_SYSTEM_START, SERVICE_KERNEL_DRIVER, TEST_SERVICE_BINARY_START_SYSTEM,
          BROKEN_BINPATH | BROKEN_DISPLAY_NAME },
        { "auto", SERVICE_AUTO_START, SERVICE_WIN32_OWN_PROCESS, TEST_SERVICE_BINARY,
          BROKEN_DISPLAY_NAME },
        { "demand", SERVICE_DEMAND_START, SERVICE_WIN32_OWN_PROCESS, TEST_SERVICE_BINARY,
          BROKEN_DISPLAY_NAME },
        { "disabled", SERVICE_DISABLED, SERVICE_WIN32_OWN_PROCESS, TEST_SERVICE_BINARY,
          BROKEN_DISPLAY_NAME },
        { "delayed-auto", SERVICE_DELAYED_AUTO_START, SERVICE_WIN32_OWN_PROCESS, TEST_SERVICE_BINARY,
          BROKEN_START | BROKEN_DISPLAY_NAME | BROKEN_DELAYED_AUTO_START }
    };
    static struct {
        const char *param;
        DWORD expected_error_control;
    } error_severities[] = {
        { "normal", SERVICE_ERROR_NORMAL },
        { "severe", SERVICE_ERROR_SEVERE },
        { "critical", SERVICE_ERROR_CRITICAL },
        { "ignore", SERVICE_ERROR_IGNORE }
    };
    unsigned int i;
    DWORD r;

    if (!elevated)
    {
        win_skip("\"sc create\" tests need elevated permissions\n");
        return;
    }

#define check_exit_code(x) ok(r == (x), "got exit code %ld, expected %d\n", r, (x))
#define check_test_service(t,s,e,de,di,br) \
    check_service_definition(TEST_SERVICE_NAME, TEST_SERVICE_BINARY, t, s, e, de, di ? di : TEST_SERVICE_NAME, br)
#define delete_test_service(x, y) \
    delete_service(TEST_SERVICE_NAME, (x) ? SC_EXIT_SUCCESS : SC_EXIT_SERVICE_DOES_NOT_EXIST, (y))
#define check_test_service2(t,s,e,de,di,br) \
    check_service_definition(TEST_SERVICE_NAME2, TEST_SERVICE_BINARY, t, s, e, de, di ? di : TEST_SERVICE_NAME2, br)
#define delete_test_service2(x, y) \
    delete_service(TEST_SERVICE_NAME2, (x) ? SC_EXIT_SUCCESS : SC_EXIT_SERVICE_DOES_NOT_EXIST, (y))

    /* too few parameters */

    run_sc_exe("sc create", &r);
    check_exit_code(SC_EXIT_INVALID_COMMAND_LINE);
    delete_test_service(FALSE, FALSE);

    run_sc_exe("sc create " TEST_SERVICE_NAME, &r);
    check_exit_code(SC_EXIT_INVALID_COMMAND_LINE);
    delete_test_service(FALSE, FALSE);

    /* binpath= */

    run_sc_exe("sc create " TEST_SERVICE_NAME " binpath= \"" TEST_SERVICE_BINARY "\"", &r);
    check_exit_code(SC_EXIT_SUCCESS);
    check_test_service(SERVICE_WIN32_OWN_PROCESS, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL, "", NULL,
                       BROKEN_DISPLAY_NAME);

    /* existing service */

    run_sc_exe("sc create " TEST_SERVICE_NAME " binpath= \"" TEST_SERVICE_BINARY "\" start= auto", &r);
    check_exit_code(SC_EXIT_SERVICE_EXISTS);
    check_test_service(SERVICE_WIN32_OWN_PROCESS, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL, "", NULL,
                       BROKEN_DISPLAY_NAME);
    delete_test_service(TRUE, FALSE);

    /* type= */

    run_sc_exe("sc create " TEST_SERVICE_NAME " binpath= \"" TEST_SERVICE_BINARY "\" type= invalid", &r);
    todo_wine check_exit_code(SC_EXIT_INVALID_COMMAND_LINE);
    delete_test_service(FALSE, TRUE);

    run_sc_exe("sc create " TEST_SERVICE_NAME " binpath= \"" TEST_SERVICE_BINARY "\" type= own", &r);
    check_exit_code(SC_EXIT_SUCCESS);
    check_test_service(SERVICE_WIN32_OWN_PROCESS, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL, "", NULL,
                       BROKEN_DISPLAY_NAME);
    delete_test_service(TRUE, FALSE);

    run_sc_exe("sc create " TEST_SERVICE_NAME " binpath= \"" TEST_SERVICE_BINARY "\" type= interact", &r);
    todo_wine check_exit_code(SC_EXIT_INVALID_PARAMETER);
    delete_test_service(FALSE, TRUE);

    run_sc_exe("sc create " TEST_SERVICE_NAME " binpath= \"" TEST_SERVICE_BINARY "\" type= interact type= own", &r);
    check_exit_code(SC_EXIT_SUCCESS);
    check_test_service(SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS, SERVICE_DEMAND_START,
                       SERVICE_ERROR_NORMAL, "", NULL, BROKEN_TYPE | BROKEN_DISPLAY_NAME);
    delete_test_service(TRUE, FALSE);

    /* start= */

    run_sc_exe("sc create " TEST_SERVICE_NAME " binpath= \"" TEST_SERVICE_BINARY "\" start= invalid", &r);
    todo_wine check_exit_code(SC_EXIT_INVALID_COMMAND_LINE);
    delete_test_service(FALSE, TRUE);

    for (i = 0; i < ARRAY_SIZE(start_types); i++)
    {
        char cmdline[256];

        strcpy(cmdline, "sc create " TEST_SERVICE_NAME " binpath= \"" TEST_SERVICE_BINARY "\" start= ");
        strcat(cmdline, start_types[i].param);
        run_sc_exe(cmdline, &r);
        check_exit_code(SC_EXIT_SUCCESS);
        check_service_definition(TEST_SERVICE_NAME, start_types[i].expected_binary_path,
                                 start_types[i].expected_service_type, start_types[i].expected_start_type,
                                 SERVICE_ERROR_NORMAL, "", TEST_SERVICE_NAME, start_types[i].broken);
        delete_test_service(TRUE, FALSE);
    }

    /* error= */

    for (i = 0; i < ARRAY_SIZE(error_severities); i++)
    {
        char cmdline[256];

        strcpy(cmdline, "sc create " TEST_SERVICE_NAME " binpath= \"" TEST_SERVICE_BINARY "\" error= ");
        strcat(cmdline, error_severities[i].param);
        run_sc_exe(cmdline, &r);
        check_exit_code(SC_EXIT_SUCCESS);
        check_test_service(SERVICE_WIN32_OWN_PROCESS, SERVICE_DEMAND_START,
                           error_severities[i].expected_error_control, "", NULL, BROKEN_DISPLAY_NAME);
        delete_test_service(TRUE, FALSE);
    }

    /* tag= */

    run_sc_exe("sc create " TEST_SERVICE_NAME " binpath= \"" TEST_SERVICE_BINARY "\" tag= yes", &r);
    todo_wine check_exit_code(SC_EXIT_INVALID_PARAMETER);
    delete_test_service(FALSE, TRUE);

    run_sc_exe("sc create " TEST_SERVICE_NAME " binpath= \"" TEST_SERVICE_BINARY "\" tag= no", &r);
    check_exit_code(SC_EXIT_SUCCESS);
    check_test_service(SERVICE_WIN32_OWN_PROCESS, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL, "", NULL,
                       BROKEN_DISPLAY_NAME);
    delete_test_service(TRUE, FALSE);

    /* depend= */

    run_sc_exe("sc create " TEST_SERVICE_NAME " binpath= \"" TEST_SERVICE_BINARY "\" depend= " TEST_SERVICE_NAME, &r);
    todo_wine check_exit_code(SC_EXIT_CIRCULAR_DEPENDENCY);
    delete_test_service(FALSE, TRUE);

    run_sc_exe("sc create " TEST_SERVICE_NAME2 " binpath= \"" TEST_SERVICE_BINARY "\" depend= " TEST_SERVICE_NAME, &r);
    check_exit_code(SC_EXIT_SUCCESS);
    check_test_service2(SERVICE_WIN32_OWN_PROCESS, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
                        TEST_SERVICE_NAME, NULL, BROKEN_DEPEND | BROKEN_DISPLAY_NAME);
    delete_test_service2(TRUE, FALSE);

    run_sc_exe("sc create " TEST_SERVICE_NAME " binpath= " TEST_SERVICE_BINARY, &r);
    check_exit_code(SC_EXIT_SUCCESS);
    run_sc_exe("sc create " TEST_SERVICE_NAME2 " binpath= \"" TEST_SERVICE_BINARY "\" depend= " TEST_SERVICE_NAME, &r);
    check_exit_code(SC_EXIT_SUCCESS);
    check_test_service2(SERVICE_WIN32_OWN_PROCESS, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
                        TEST_SERVICE_NAME, NULL, BROKEN_DEPEND | BROKEN_DISPLAY_NAME);
    delete_test_service2(TRUE, FALSE);
    delete_test_service(TRUE, FALSE);

    /* displayname= */

    run_sc_exe("sc create " TEST_SERVICE_NAME " binpath= " TEST_SERVICE_BINARY
               " displayname= \"Wine Test Service\"", &r);
    check_exit_code(SC_EXIT_SUCCESS);
    check_test_service(SERVICE_WIN32_OWN_PROCESS, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL, "",
                       "Wine Test Service", 0);
    delete_test_service(TRUE, FALSE);

    /* without spaces */

    run_sc_exe("sc create " TEST_SERVICE_NAME2 " binpath=\"" TEST_SERVICE_BINARY "\" type=own start=auto"
               " error=normal tag=no depend=" TEST_SERVICE_NAME " displayname=\"Wine Test Service\"", &r);
    ok(r == SC_EXIT_SUCCESS || broken(r == SC_EXIT_INVALID_COMMAND_LINE), "got exit code %ld, expected %d\n",
       r, SC_EXIT_SUCCESS);
    if (r == SC_EXIT_SUCCESS)
    {
        check_test_service2(SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START, SERVICE_ERROR_NORMAL,
                            TEST_SERVICE_NAME, "Wine Test Service", BROKEN_DEPEND);
        delete_test_service2(TRUE, FALSE);
    }
    else
    {
        delete_test_service2(FALSE, FALSE);
    }

    /* case-insensitive */

    run_sc_exe("SC CREATE " TEST_SERVICE_NAME2 " BINPATH= \"" TEST_SERVICE_BINARY "\" TYPE= OWN START= AUTO"
               " ERROR= NORMAL TAG= NO DEPEND= " TEST_SERVICE_NAME " DISPLAYNAME= \"Wine Test Service\"", &r);
    check_exit_code(SC_EXIT_SUCCESS);
    check_test_service2(SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START, SERVICE_ERROR_NORMAL,
                        TEST_SERVICE_NAME, "Wine Test Service", BROKEN_DEPEND);
    delete_test_service2(TRUE, FALSE);

#undef delete_test_service2
#undef check_test_service2
#undef delete_test_service
#undef check_test_service
#undef check_exit_code
}

/* taken from winetest, only whitespace changes */
static int running_elevated(void)
{
    HANDLE token;
    TOKEN_ELEVATION elevation_info;
    DWORD size;

    /* Get the process token */
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
        return -1;

    /* Get the elevation info from the token */
    if (!GetTokenInformation(token, TokenElevation, &elevation_info, sizeof(TOKEN_ELEVATION), &size))
    {
        CloseHandle(token);
        return -1;
    }
    CloseHandle(token);

    return elevation_info.TokenIsElevated;
}


START_TEST(sc)
{
    SECURITY_ATTRIBUTES secattr = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};
    BOOL elevated = running_elevated();

    nul_file = CreateFileA("NUL", GENERIC_READ | GENERIC_WRITE, 0, &secattr, OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL, NULL);

    scmgr = OpenSCManagerA(NULL, NULL, GENERIC_READ);
    ok(!!scmgr, "OpenSCManagerA failed: %ld\n", GetLastError());

    test_create_service(elevated);

    CloseServiceHandle(scmgr);
    CloseHandle(nul_file);
}
