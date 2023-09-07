/*
 * ntoskrnl.exe testing framework
 *
 * Copyright 2015 Sebastian Lackner
 * Copyright 2015 Michael Müller
 * Copyright 2015 Christian Costa
 * Copyright 2020 Paul Gofman for CodeWeavers
 * Copyright 2020-2021 Zebediah Figura for CodeWeavers
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
#include <winternl.h>

#include <ddk/wdm.h>

#include "wine/test.h"

static HANDLE okfile;

LONG winetest_successes = 0;
LONG winetest_failures = 0;
LONG winetest_flaky_failures = 0;
LONG winetest_skipped = 0;
LONG winetest_todo_successes = 0;
LONG winetest_todo_failures = 0;
LONG winetest_muted_traces = 0;
LONG winetest_muted_skipped = 0;
LONG winetest_muted_todo_successes = 0;

const char *winetest_platform;
int winetest_platform_is_wine;
int winetest_debug;
int winetest_report_success;
int winetest_color = 0;
int winetest_time = 0;
int winetest_start_time, winetest_last_time;

/* silence todos and skips above this threshold */
int winetest_mute_threshold = 42;

static KSPIN_LOCK tls_data_lock;
static struct winetest_thread_data tls_data_pool[128];
static HANDLE tls_data_thread[ARRAY_SIZE(tls_data_pool)];
static DWORD tls_data_count;

struct winetest_thread_data *winetest_get_thread_data(void)
{
    HANDLE thread = PsGetCurrentThreadId();
    struct winetest_thread_data *data;
    KIRQL irql;
    UINT i;

    KeAcquireSpinLock(&tls_data_lock, &irql);

    for (i = 0; i < tls_data_count; i++) if (tls_data_thread[i] == thread) break;
    data = tls_data_pool + i;

    if (tls_data_thread[i] != thread)
    {
        tls_data_count = min(tls_data_count + 1, ARRAY_SIZE(tls_data_pool));
        tls_data_thread[i] = thread;
        data->str_pos = data->strings;
    }

    KeReleaseSpinLock(&tls_data_lock, irql);

    return data;
}

void winetest_print_lock(void)
{
}

void winetest_print_unlock(void)
{
}

int winetest_vprintf(const char *format, va_list ap)
{
    struct winetest_thread_data *data = winetest_get_thread_data();
    IO_STATUS_BLOCK io;
    int len = vsnprintf(data->strings, sizeof(data->strings), format, ap);
    ZwWriteFile(okfile, NULL, NULL, NULL, &io, data->strings, len, NULL, NULL);
    return len;
}

int winetest_get_time(void)
{
    return 0;
}

static inline NTSTATUS winetest_init(void)
{
    const struct test_data *data;
    SIZE_T size = sizeof(*data);
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING string;
    IO_STATUS_BLOCK io;
    void *addr = NULL;
    HANDLE section;
    NTSTATUS ret;

    KeInitializeSpinLock(&tls_data_lock);

    RtlInitUnicodeString(&string, L"\\BaseNamedObjects\\winetest_ntoskrnl_section");
    /* OBJ_KERNEL_HANDLE is necessary for the file to be accessible from system threads */
    InitializeObjectAttributes(&attr, &string, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, 0, NULL);
    if ((ret = ZwOpenSection(&section, SECTION_MAP_READ, &attr)))
        return ret;

    if ((ret = ZwMapViewOfSection(section, NtCurrentProcess(), &addr,
            0, 0, NULL, &size, ViewUnmap, 0, PAGE_READONLY)))
    {
        ZwClose(section);
        return ret;
    }
    data = addr;
    winetest_platform_is_wine = data->running_under_wine;
    winetest_platform = winetest_platform_is_wine ? "wine" : "windows";
    winetest_debug = data->winetest_debug;
    winetest_report_success = data->winetest_report_success;

    ZwUnmapViewOfSection(NtCurrentProcess(), addr);
    ZwClose(section);

    RtlInitUnicodeString(&string, L"\\??\\C:\\windows\\winetest_ntoskrnl_okfile");
    return ZwOpenFile(&okfile, FILE_APPEND_DATA | SYNCHRONIZE, &attr, &io,
            FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_SYNCHRONOUS_IO_NONALERT);
}

static inline void winetest_cleanup(void)
{
    struct test_data *data;
    SIZE_T size = sizeof(*data);
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING string;
    void *addr = NULL;
    HANDLE section;

    if (winetest_debug)
    {
        winetest_printf("%04lx:ntoskrnl: %ld tests executed (%ld marked as todo, 0 as flaky, %ld %s), %ld skipped.\n",
                         (DWORD)(DWORD_PTR)PsGetCurrentProcessId(), winetest_successes + winetest_failures + winetest_todo_successes + winetest_todo_failures,
                         winetest_todo_successes, winetest_failures + winetest_todo_failures,
                         (winetest_failures + winetest_todo_failures != 1) ? "failures" : "failure", winetest_skipped);
    }

    RtlInitUnicodeString(&string, L"\\BaseNamedObjects\\winetest_ntoskrnl_section");
    /* OBJ_KERNEL_HANDLE is necessary for the file to be accessible from system threads */
    InitializeObjectAttributes(&attr, &string, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, 0, NULL);

    if (!ZwOpenSection(&section, SECTION_MAP_READ | SECTION_MAP_WRITE, &attr))
    {
        if (!ZwMapViewOfSection(section, NtCurrentProcess(), &addr,
                0, 0, NULL, &size, ViewUnmap, 0, PAGE_READWRITE))
        {
            data = addr;

            InterlockedExchangeAdd(&data->successes, winetest_successes);
            InterlockedExchangeAdd(&data->failures, winetest_failures);
            InterlockedExchangeAdd(&data->todo_successes, winetest_todo_successes);
            InterlockedExchangeAdd(&data->todo_failures, winetest_todo_failures);
            InterlockedExchangeAdd(&data->skipped, winetest_skipped);

            ZwUnmapViewOfSection(NtCurrentProcess(), addr);
        }
        ZwClose(section);
    }

    ZwClose(okfile);
}
