/*
 * Copyright 2018 Dmitry Timoshkov
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

#include <stdio.h>
#include <windows.h>
#include <ole2.h>
#include <rpcdce.h>
#include <mstask.h>
#include "atsvc.h"

#include "wine/test.h"

/* lmat.h defines those, but other types in that file conflict
 * with generated atsvc.h typedefs.
 */
#define JOB_ADD_CURRENT_DATE 0x08
#define JOB_NONINTERACTIVE   0x10

extern handle_t atsvc_handle;

static int test_failures, test_skipped;

static LONG CALLBACK rpc_exception_filter(EXCEPTION_POINTERS *ptrs)
{
    if (test_skipped)
        skip("Can't connect to ATSvc service: %#x\n", ptrs->ExceptionRecord->ExceptionCode);

    if (winetest_debug)
    {
        fprintf(stdout, "%04x:atsvcapi: 1 tests executed (0 marked as todo, %d %s), %d skipped.\n",
                GetCurrentProcessId(), test_failures, test_failures != 1 ? "failures" : "failure", test_skipped);
        fflush(stdout);
    }
    ExitProcess(test_failures);
}

START_TEST(atsvcapi)
{
    static unsigned char ncalrpc[] = "ncalrpc";
    static WCHAR task1W[] = { 'T','a','s','k','1','.','e','x','e',0 };
    HRESULT hr;
    unsigned char *binding_str;
    WCHAR server_name[MAX_COMPUTERNAME_LENGTH + 1];
    PTOP_LEVEL_EXCEPTION_FILTER old_exception_filter;
    AT_ENUM_CONTAINER container;
    AT_INFO info, *info2;
    DWORD ret, i, total, start_index, jobid, try, try_count;
    BOOL found;

    total = MAX_COMPUTERNAME_LENGTH + 1;
    SetLastError(0xdeadbeef);
    ret = GetComputerNameW(server_name, &total);
    ok(ret, "GetComputerName error %u\n", GetLastError());

    hr = RpcStringBindingComposeA(NULL, ncalrpc, NULL, NULL, NULL, &binding_str);
    ok(hr == RPC_S_OK, "RpcStringBindingCompose error %#x\n", hr);
    hr = RpcBindingFromStringBindingA(binding_str, &atsvc_handle);
    ok(hr == RPC_S_OK, "RpcBindingFromStringBinding error %#x\n", hr);
    hr = RpcStringFreeA(&binding_str);
    ok(hr == RPC_S_OK, "RpcStringFree error %#x\n", hr);

    /* widl generated RpcTryExcept/RpcExcept can't catch raised exceptions */
    old_exception_filter = SetUnhandledExceptionFilter(rpc_exception_filter);

    /* If the first call fails that's probably because the service is not running */
    test_failures = 0;
    test_skipped = 1;

    memset(&info, 0, sizeof(info));
    info.Flags = JOB_ADD_CURRENT_DATE | JOB_NONINTERACTIVE;
    info.Command = task1W;
    jobid = 0;
    ret = NetrJobAdd(server_name, &info, &jobid);
    if (ret == ERROR_ACCESS_DENIED)
    {
        win_skip("NetrJobAdd: Access denied, skipping the tests\n");
        goto skip_tests;
    }
    ok(ret == ERROR_SUCCESS || broken(ret == ERROR_NOT_SUPPORTED) /* Win8+ */, "NetrJobAdd error %u\n", ret);
    if (ret == ERROR_NOT_SUPPORTED)
    {
        /* FIXME: use win_skip() when todo_wine above is removed */
        skip("NetrJobAdd is not supported on this platform\n");
        goto skip_tests;
    }
    ok(jobid != 0, "jobid should not be 0\n");

    /* From now on: if the call fails that's a failure */
    test_failures = 1;
    test_skipped = 0;

    info2 = NULL;
    ret = NetrJobGetInfo(server_name, 0xdeadbeef, &info2);
    ok(ret == APE_AT_ID_NOT_FOUND || broken(1) /* vista and w2008 return rubbish here */, "wrong error %u\n", ret);

    ret = NetrJobDel(server_name, 0xdeadbeef, 0xdeadbeef);
    ok(ret == APE_AT_ID_NOT_FOUND, "wrong error %u\n", ret);

    try_count = 5;

    for (try = 1; try <= try_count; try++)
    {
        container.EntriesRead = 0;
        container.Buffer = NULL;
        total = start_index = 0;
        ret = NetrJobEnum(server_name, &container, -1, &total, &start_index);
        if (ret == ERROR_ACCESS_DENIED)
        {
            win_skip("NetrJobEnum: Access denied, skipping the tests\n");
            goto skip_tests_delete;
        }
        ok(ret == ERROR_SUCCESS, "NetrJobEnum error %u (%#x)\n", ret, ret);
        ok(total != 0, "total %u\n", total);
        ok(start_index == 0, "start_index %u\n", start_index);
        ok(container.Buffer != NULL, "Buffer %p\n", container.Buffer);
        ok(container.EntriesRead != 0, "EntriesRead %u\n", container.EntriesRead);

        found = FALSE;

        for (i = 0; i < container.EntriesRead; i++)
        {
            trace("%u: jobid %u, command %s\n", i, container.Buffer[i].JobId, wine_dbgstr_w(container.Buffer[i].Command));

            if (container.Buffer[i].JobId == jobid ||
                !lstrcmpW(container.Buffer[i].Command, task1W))
            {
                found = TRUE;
                trace("found %u: jobid %u, command %s\n", i, container.Buffer[i].JobId, wine_dbgstr_w(container.Buffer[i].Command));
            }

            info2 = NULL;
            ret = NetrJobGetInfo(server_name, container.Buffer[i].JobId, &info2);
            ok(ret == ERROR_SUCCESS, "NetrJobGetInfo error %u\n", ret);

            ok(container.Buffer[i].JobTime == info2->JobTime, "%u != %u\n", (UINT)container.Buffer[i].JobTime, (UINT)info2->JobTime);
            ok(container.Buffer[i].DaysOfMonth == info2->DaysOfMonth, "%u != %u\n", container.Buffer[i].DaysOfMonth, info2->DaysOfMonth);
            ok(container.Buffer[i].DaysOfWeek == info2->DaysOfWeek, "%u != %u\n", container.Buffer[i].DaysOfWeek, info2->DaysOfWeek);
            ok(container.Buffer[i].Flags == info2->Flags, "%#x != %#x\n", container.Buffer[i].Flags, info2->Flags);
            ok(!lstrcmpW(container.Buffer[i].Command, info2->Command), "%s != %s\n", wine_dbgstr_w(container.Buffer[i].Command), wine_dbgstr_w(info2->Command));

            MIDL_user_free(container.Buffer[i].Command);
            MIDL_user_free(info2->Command);
            MIDL_user_free(info2);
        }

        if (found)
            break;
    }

    MIDL_user_free(container.Buffer);

    ok(found, "just added jobid %u should be found\n", jobid);

skip_tests_delete:
    ret = NetrJobDel(server_name, jobid, jobid);
    ok(ret == ERROR_SUCCESS, "NetrJobDel error %u\n", ret);

skip_tests:
    SetUnhandledExceptionFilter(old_exception_filter);

    hr = RpcBindingFree(&atsvc_handle);
    ok(hr == RPC_S_OK, "RpcBindingFree error %#x\n", hr);
}

DECLSPEC_HIDDEN handle_t __RPC_USER ATSVC_HANDLE_bind(ATSVC_HANDLE str)
{
    static unsigned char ncalrpc[] = "ncalrpc";
    unsigned char *binding_str;
    handle_t rpc_handle;
    HRESULT hr;

    hr = RpcStringBindingComposeA(NULL, ncalrpc, NULL, NULL, NULL, &binding_str);
    ok(hr == RPC_S_OK, "RpcStringBindingCompose error %#x\n", hr);

    hr = RpcBindingFromStringBindingA(binding_str, &rpc_handle);
    ok(hr == RPC_S_OK, "RpcBindingFromStringBinding error %#x\n", hr);

    RpcStringFreeA(&binding_str);
    return rpc_handle;
}

DECLSPEC_HIDDEN void __RPC_USER ATSVC_HANDLE_unbind(ATSVC_HANDLE ServerName, handle_t rpc_handle)
{
    RpcBindingFree(&rpc_handle);
}
