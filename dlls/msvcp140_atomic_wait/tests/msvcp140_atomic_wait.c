/*
 * Copyright 2022 Daniel Lehman
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

#include "windef.h"
#include "winbase.h"
#include "wine/test.h"

static unsigned int (__stdcall *p___std_parallel_algorithms_hw_threads)(void);

static PTP_WORK (__stdcall *p___std_create_threadpool_work)(PTP_WORK_CALLBACK, void*, PTP_CALLBACK_ENVIRON);

#define SETNOFAIL(x,y) x = (void*)GetProcAddress(msvcp,y)
#define SET(x,y) do { SETNOFAIL(x,y); ok(x != NULL, "Export '%s' not found\n", y); } while(0)
static HMODULE init(void)
{
    HMODULE msvcp;

    if (!(msvcp = LoadLibraryA("msvcp140_atomic_wait.dll")))
        return NULL;

    SET(p___std_parallel_algorithms_hw_threads, "__std_parallel_algorithms_hw_threads");

    SET(p___std_create_threadpool_work, "__std_create_threadpool_work");
    return msvcp;
}

static void test___std_parallel_algorithms_hw_threads(void)
{
    SYSTEM_INFO si;
    unsigned int nthr;

    GetSystemInfo(&si);
    nthr = p___std_parallel_algorithms_hw_threads();
    ok(nthr == si.dwNumberOfProcessors, "expected %u, got %u\n", si.dwNumberOfProcessors, nthr);
}

static PTP_WORK cb_work;
static void *cb_context;
static void WINAPI threadpool_workcallback(PTP_CALLBACK_INSTANCE instance, void *context, PTP_WORK work) {
    LONG *workcalled = context;
    cb_work = work;
    cb_context = context;

    InterlockedIncrement(workcalled);
}

static HANDLE cb_event;
static void CALLBACK threadpool_workfinalization(TP_CALLBACK_INSTANCE *instance, void *context)
{
    LONG *workcalled = context;

    InterlockedIncrement(workcalled);
    SetEvent(cb_event);
}

static void test_threadpool_work(void)
{
    PTP_WORK work;
    DWORD gle, ret;
    LONG workcalled;
    TP_CALLBACK_ENVIRON environment;
    TP_CALLBACK_ENVIRON_V3 environment3;

    if (0) /* crash on windows */
    {
        p___std_create_threadpool_work(NULL, NULL, NULL);
    }

    /* simple test */
    workcalled = 0;
    work = p___std_create_threadpool_work(threadpool_workcallback, &workcalled, NULL);
    ok(!!work, "failed to create threadpool_work\n");
    SubmitThreadpoolWork(work);
    WaitForThreadpoolWorkCallbacks(work, FALSE);
    CloseThreadpoolWork(work);
    ok(workcalled == 1, "expected work to be called once, got %d\n", workcalled);
    ok(cb_work == work, "expected %p, got %p\n", work, cb_work);
    ok(cb_context == &workcalled, "expected %p, got %p\n", &workcalled, cb_context);

    /* test with environment */
    cb_event = CreateEventW(NULL, TRUE, FALSE, NULL);
    memset(&environment, 0, sizeof(environment));
    environment.Version = 1;
    environment.FinalizationCallback = threadpool_workfinalization;
    workcalled = 0;
    work = p___std_create_threadpool_work(threadpool_workcallback, &workcalled, &environment);
    ok(!!work, "failed to create threadpool_work\n");
    SubmitThreadpoolWork(work);
    WaitForThreadpoolWorkCallbacks(work, FALSE);
    CloseThreadpoolWork(work);
    ret = WaitForSingleObject(cb_event, 1000);
    ok(ret == WAIT_OBJECT_0, "expected finalization callback to be called\n");
    ok(workcalled == 2, "expected work to be called twice, got %d\n", workcalled);
    CloseHandle(cb_event);

    /* test with environment version 3 */
    memset(&environment3, 0, sizeof(environment3));
    environment3.Version = 3;
    environment3.CallbackPriority = TP_CALLBACK_PRIORITY_NORMAL;
    SetLastError(0xdeadbeef);
    work = p___std_create_threadpool_work(threadpool_workcallback, &workcalled,
                                          (TP_CALLBACK_ENVIRON *)&environment3);
    gle = GetLastError();
    ok(gle == 0xdeadbeef, "expected 0xdeadbeef, got %x\n", gle);
    ok(!!work, "failed to create threadpool_work\n");
    CloseThreadpoolWork(work);

    memset(&environment3, 0, sizeof(environment3));
    environment3.Version = 3;
    environment3.CallbackPriority = TP_CALLBACK_PRIORITY_INVALID;
    SetLastError(0xdeadbeef);
    work = p___std_create_threadpool_work(threadpool_workcallback, &workcalled,
                                          (TP_CALLBACK_ENVIRON *)&environment3);
    gle = GetLastError();
    ok(gle == ERROR_INVALID_PARAMETER, "expected %d, got %d\n", ERROR_INVALID_PARAMETER, gle);
    ok(!work, "expected failure\n");
}

START_TEST(msvcp140_atomic_wait)
{
    HMODULE msvcp;
    if (!(msvcp = init()))
    {
        win_skip("msvcp140_atomic_wait.dll not installed\n");
        return;
    }
    test___std_parallel_algorithms_hw_threads();
    test_threadpool_work();
    FreeLibrary(msvcp);
}
