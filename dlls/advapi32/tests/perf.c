/*
 * Unit tests for Perflib functions
 *
 * Copyright (c) 2021 Paul Gofman for CodeWeavers
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

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "perflib.h"
#include "winperf.h"

#include "wine/test.h"

static ULONG WINAPI test_provider_callback(ULONG code, void *buffer, ULONG size)
{
    ok(0, "Provider callback called.\n");
    return ERROR_SUCCESS;
}

void test_provider_init(void)
{
    static GUID test_set_guid = {0xdeadbeef, 0x0002, 0x0003, {0x0f, 0x04, 0x05, 0x06, 0x07, 0x08, 0x00 ,0x0a}};
    static GUID test_set_guid2 = {0xdeadbeef, 0x0003, 0x0003, {0x0f, 0x04, 0x05, 0x06, 0x07, 0x08, 0x00 ,0x0a}};
    static GUID test_guid = {0xdeadbeef, 0x0001, 0x0002, {0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x00 ,0x0a}};
    static struct
    {
        PERF_COUNTERSET_INFO counterset;
        PERF_COUNTER_INFO counter[2];
    }
    pc_template =
    {
        {{0}},
        {
            {1, PERF_COUNTER_COUNTER, PERF_ATTRIB_BY_REFERENCE, sizeof(PERF_COUNTER_INFO),
                    PERF_DETAIL_NOVICE, 0, 0xdeadbeef},
            {2, PERF_COUNTER_COUNTER, PERF_ATTRIB_BY_REFERENCE, sizeof(PERF_COUNTER_INFO),
                    PERF_DETAIL_NOVICE, 0, 0xdeadbeef},
        },
    };

    PERF_COUNTERSET_INSTANCE *instance;
    PERF_PROVIDER_CONTEXT prov_context;
    UINT64 counter1, counter2;
    HANDLE prov, prov2;
    ULONG ret, size;
    BOOL bret;

    prov = (HANDLE)0xdeadbeef;
    ret = PerfStartProvider(NULL, test_provider_callback, &prov);
    ok(ret == ERROR_INVALID_PARAMETER, "Got unexpected ret %lu.\n", ret);
    ok(prov == (HANDLE)0xdeadbeef, "Got unexpected prov %p.\n", prov);

    prov = (HANDLE)0xdeadbeef;
    ret = PerfStartProvider(&test_guid, test_provider_callback, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "Got unexpected ret %lu.\n", ret);
    ok(prov == (HANDLE)0xdeadbeef, "Got unexpected prov %p.\n", prov);

    prov = (HANDLE)0xdeadbeef;
    ret = PerfStartProvider(&test_guid, test_provider_callback, &prov);
    ok(!ret, "Got unexpected ret %lu.\n", ret);
    ok(prov != (HANDLE)0xdeadbeef, "Provider handle is not set.\n");

    prov2 = prov;
    ret = PerfStartProvider(&test_guid, test_provider_callback, &prov2);
    ok(!ret, "Got unexpected ret %lu.\n", ret);
    ok(prov2 != prov, "Got the same provider handle.\n");

    ret = PerfStopProvider(prov2);
    ok(!ret, "Got unexpected ret %lu.\n", ret);

    if (0)
    {
        /* Access violation on Windows. */
        PerfStopProvider(prov2);
    }

    /* Provider handle is a pointer and not a kernel object handle. */
    bret = DuplicateHandle(GetCurrentProcess(), prov, GetCurrentProcess(), &prov2, 0, FALSE, DUPLICATE_SAME_ACCESS);
    ok(!bret && GetLastError() == ERROR_INVALID_HANDLE, "Got unexpected bret %d, err %lu.\n", bret, GetLastError());
    bret = IsBadWritePtr(prov, 8);
    ok(!bret, "Handle does not point to the data.\n");

    pc_template.counterset.CounterSetGuid = test_set_guid;
    pc_template.counterset.ProviderGuid = test_guid;
    pc_template.counterset.NumCounters = 0;
    pc_template.counterset.InstanceType = PERF_COUNTERSET_SINGLE_INSTANCE;
    ret = PerfSetCounterSetInfo(prov, &pc_template.counterset, sizeof(pc_template.counterset));
    ok(ret == ERROR_INVALID_PARAMETER, "Got unexpected ret %lu.\n", ret);

    pc_template.counterset.CounterSetGuid = test_set_guid;
    pc_template.counterset.ProviderGuid = test_guid;
    pc_template.counterset.NumCounters = 2;
    pc_template.counterset.InstanceType = PERF_COUNTERSET_SINGLE_INSTANCE;
    ret = PerfSetCounterSetInfo(prov, &pc_template.counterset, sizeof(pc_template));
    ok(!ret, "Got unexpected ret %lu.\n", ret);

    pc_template.counterset.CounterSetGuid = test_set_guid2;
    /* Looks like ProviderGuid doesn't need to match provider. */
    pc_template.counterset.ProviderGuid = test_set_guid;
    pc_template.counterset.NumCounters = 1;
    pc_template.counterset.InstanceType = PERF_COUNTERSET_SINGLE_INSTANCE;
    ret = PerfSetCounterSetInfo(prov, &pc_template.counterset, sizeof(pc_template));
    ok(!ret, "Got unexpected ret %lu.\n", ret);

    ret = PerfSetCounterSetInfo(prov, &pc_template.counterset, sizeof(pc_template));
    ok(ret == ERROR_ALREADY_EXISTS, "Got unexpected ret %lu.\n", ret);

    SetLastError(0xdeadbeef);
    instance = PerfCreateInstance(prov, NULL, L"1", 1);
    ok(!instance, "Got unexpected instance %p.\n", instance);
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %lu.\n", GetLastError());

    SetLastError(0xdeadbeef);
    instance = PerfCreateInstance(prov, &test_guid, L"1", 1);
    ok(!instance, "Got unexpected instance %p.\n", instance);
    ok(GetLastError() == ERROR_NOT_FOUND, "Got unexpected error %lu.\n", GetLastError());

    SetLastError(0xdeadbeef);
    instance = PerfCreateInstance(prov, &test_guid, NULL, 1);
    ok(!instance, "Got unexpected instance %p.\n", instance);
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %lu.\n", GetLastError());

    SetLastError(0xdeadbeef);
    instance = PerfCreateInstance(prov, &test_set_guid, L"11", 1);
    ok(!!instance, "Got NULL instance.\n");
    ok(GetLastError() == 0xdeadbeef, "Got unexpected error %lu.\n", GetLastError());
    ok(instance->InstanceId == 1, "Got unexpected InstanceId %lu.\n", instance->InstanceId);
    ok(instance->InstanceNameSize == 6, "Got unexpected InstanceNameSize %lu.\n", instance->InstanceNameSize);
    ok(IsEqualGUID(&instance->CounterSetGuid, &test_set_guid), "Got unexpected guid %s.\n",
            debugstr_guid(&instance->CounterSetGuid));

    ok(instance->InstanceNameOffset == sizeof(*instance) + sizeof(UINT64) * 2,
            "Got unexpected InstanceNameOffset %lu.\n", instance->InstanceNameOffset);
    ok(!lstrcmpW((WCHAR *)((BYTE *)instance + instance->InstanceNameOffset), L"11"),
            "Got unexpected instance name %s.\n",
            debugstr_w((WCHAR *)((BYTE *)instance + instance->InstanceNameOffset)));
    size = ((sizeof(*instance) + sizeof(UINT64) * 2 + instance->InstanceNameSize) + 7) & ~7;
    ok(size == instance->dwSize, "Got unexpected size %lu, instance->dwSize %lu.\n", size, instance->dwSize);

    ret = PerfSetCounterRefValue(prov, instance, 1, &counter1);
    ok(!ret, "Got unexpected ret %lu.\n", ret);
    ret = PerfSetCounterRefValue(prov, instance, 2, &counter2);
    ok(!ret, "Got unexpected ret %lu.\n", ret);

    ret = PerfSetCounterRefValue(prov, instance, 0, &counter2);
    ok(ret == ERROR_NOT_FOUND, "Got unexpected ret %lu.\n", ret);

    ok(*(void **)(instance + 1) == &counter1, "Got unexpected counter value %p.\n",
            *(void **)(instance + 1));
    ok(*(void **)((BYTE *)instance + sizeof(*instance) + sizeof(UINT64)) == &counter2,
            "Got unexpected counter value %p.\n", *(void **)(instance + 1));

    ret = PerfDeleteInstance(prov, instance);
    ok(!ret, "Got unexpected ret %lu.\n", ret);

    ret = PerfStopProvider(prov);
    ok(!ret, "Got unexpected ret %lu.\n", ret);

    memset( &prov_context, 0, sizeof(prov_context) );
    prov = (HANDLE)0xdeadbeef;
    ret = PerfStartProviderEx( &test_guid, &prov_context, &prov );
    ok(ret == ERROR_INVALID_PARAMETER, "Got unexpected ret %lu.\n", ret);
    ok(prov == (HANDLE)0xdeadbeef, "Got unexpected prov %p.\n", prov);

    prov_context.ContextSize = sizeof(prov_context) + 1;
    ret = PerfStartProviderEx( &test_guid, &prov_context, &prov );
    ok(!ret, "Got unexpected ret %lu.\n", ret);
    ok(prov != (HANDLE)0xdeadbeef, "Provider handle is not set.\n");

    ret = PerfStopProvider(prov);
    ok(!ret, "Got unexpected ret %lu.\n", ret);
}

START_TEST(perf)
{
    test_provider_init();
}
