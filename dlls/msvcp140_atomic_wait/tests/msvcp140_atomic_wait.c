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
#include <process.h>

#include "windef.h"
#include "winbase.h"
#include "wine/test.h"

typedef struct
{
    SRWLOCK srwlock;
} shared_mutex;

enum tzdb_error
{
    TZDB_ERROR_SUCCESS,
    TZDB_ERROR_WIN,
    TZDB_ERROR_ICU,
};

struct tzdb_time_zones
{
    enum tzdb_error error;
    char *ver;
    unsigned int count;
    char **names;
    char **links;
};

static unsigned int (__stdcall *p___std_parallel_algorithms_hw_threads)(void);

static void (__stdcall *p___std_bulk_submit_threadpool_work)(PTP_WORK, size_t);
static void (__stdcall *p___std_close_threadpool_work)(PTP_WORK);
static PTP_WORK (__stdcall *p___std_create_threadpool_work)(PTP_WORK_CALLBACK, void*, PTP_CALLBACK_ENVIRON);
static void (__stdcall *p___std_submit_threadpool_work)(PTP_WORK);
static void (__stdcall *p___std_wait_for_threadpool_work_callbacks)(PTP_WORK, BOOL);
static BOOL (__stdcall *p___std_atomic_wait_direct)(volatile void*, void*, size_t, DWORD);
static void (__stdcall *p___std_atomic_notify_one_direct)(void*);
static void (__stdcall *p___std_execution_wait_on_uchar)(void *, unsigned char);
static void (__stdcall *p___std_execution_wake_by_address_all)(void *);
static shared_mutex* (__stdcall *p___std_acquire_shared_mutex_for_instance)(void*);
static void (__stdcall *p___std_release_shared_mutex_for_instance)(void*);
static struct tzdb_time_zones * (__stdcall *p___std_tzdb_get_time_zones)(void);
static void (__stdcall *p___std_tzdb_delete_time_zones)(struct tzdb_time_zones *);

#define SETNOFAIL(x,y) x = (void*)GetProcAddress(msvcp,y)
#define SET(x,y) do { SETNOFAIL(x,y); ok(x != NULL, "Export '%s' not found\n", y); } while(0)
static HMODULE init(void)
{
    HMODULE msvcp;

    if (!(msvcp = LoadLibraryA("msvcp140_atomic_wait.dll")))
        return NULL;

    SET(p___std_parallel_algorithms_hw_threads, "__std_parallel_algorithms_hw_threads");

    SET(p___std_bulk_submit_threadpool_work, "__std_bulk_submit_threadpool_work");
    SET(p___std_close_threadpool_work, "__std_close_threadpool_work");
    SET(p___std_create_threadpool_work, "__std_create_threadpool_work");
    SET(p___std_submit_threadpool_work, "__std_submit_threadpool_work");
    SET(p___std_wait_for_threadpool_work_callbacks, "__std_wait_for_threadpool_work_callbacks");
    SET(p___std_atomic_wait_direct, "__std_atomic_wait_direct");
    SET(p___std_atomic_notify_one_direct, "__std_atomic_notify_one_direct");
    SET(p___std_execution_wait_on_uchar, "__std_execution_wait_on_uchar");
    SET(p___std_execution_wake_by_address_all, "__std_execution_wake_by_address_all");
    SET(p___std_acquire_shared_mutex_for_instance, "__std_acquire_shared_mutex_for_instance");
    SET(p___std_release_shared_mutex_for_instance, "__std_release_shared_mutex_for_instance");
    SET(p___std_tzdb_get_time_zones, "__std_tzdb_get_time_zones");
    SET(p___std_tzdb_delete_time_zones, "__std_tzdb_delete_time_zones");
    return msvcp;
}

static void test___std_parallel_algorithms_hw_threads(void)
{
    SYSTEM_INFO si;
    unsigned int nthr;

    GetSystemInfo(&si);
    nthr = p___std_parallel_algorithms_hw_threads();
    ok(nthr == si.dwNumberOfProcessors, "expected %lu, got %u\n", si.dwNumberOfProcessors, nthr);
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
        p___std_bulk_submit_threadpool_work(NULL, 5);
        p___std_create_threadpool_work(NULL, NULL, NULL);
        p___std_submit_threadpool_work(NULL);
        p___std_wait_for_threadpool_work_callbacks(NULL, FALSE);
        p___std_close_threadpool_work(NULL);
    }

    /* simple test */
    workcalled = 0;
    work = p___std_create_threadpool_work(threadpool_workcallback, &workcalled, NULL);
    ok(!!work, "failed to create threadpool_work\n");
    p___std_submit_threadpool_work(work);
    p___std_wait_for_threadpool_work_callbacks(work, FALSE);
    p___std_close_threadpool_work(work);
    ok(workcalled == 1, "expected work to be called once, got %ld\n", workcalled);
    ok(cb_work == work, "expected %p, got %p\n", work, cb_work);
    ok(cb_context == &workcalled, "expected %p, got %p\n", &workcalled, cb_context);

    /* bulk submit */
    workcalled = 0;
    work = p___std_create_threadpool_work(threadpool_workcallback, &workcalled, NULL);
    ok(!!work, "failed to create threadpool_work\n");
    p___std_bulk_submit_threadpool_work(work, 13);
    p___std_wait_for_threadpool_work_callbacks(work, FALSE);
    p___std_close_threadpool_work(work);
    ok(workcalled == 13, "expected work to be called 13 times, got %ld\n", workcalled);

    workcalled = 0;
    work = p___std_create_threadpool_work(threadpool_workcallback, &workcalled, NULL);
    ok(!!work, "failed to create threadpool_work\n");
    p___std_bulk_submit_threadpool_work(work, 0);
    p___std_wait_for_threadpool_work_callbacks(work, FALSE);
    p___std_close_threadpool_work(work);
    ok(workcalled == 0, "expected no work, got %ld\n", workcalled);

    /* test with environment */
    cb_event = CreateEventW(NULL, TRUE, FALSE, NULL);
    memset(&environment, 0, sizeof(environment));
    environment.Version = 1;
    environment.FinalizationCallback = threadpool_workfinalization;
    workcalled = 0;
    work = p___std_create_threadpool_work(threadpool_workcallback, &workcalled, &environment);
    ok(!!work, "failed to create threadpool_work\n");
    p___std_submit_threadpool_work(work);
    p___std_wait_for_threadpool_work_callbacks(work, FALSE);
    p___std_close_threadpool_work(work);
    ret = WaitForSingleObject(cb_event, 1000);
    ok(ret == WAIT_OBJECT_0, "expected finalization callback to be called\n");
    ok(workcalled == 2, "expected work to be called twice, got %ld\n", workcalled);
    CloseHandle(cb_event);

    /* test with environment version 3 */
    memset(&environment3, 0, sizeof(environment3));
    environment3.Version = 3;
    environment3.CallbackPriority = TP_CALLBACK_PRIORITY_NORMAL;
    SetLastError(0xdeadbeef);
    work = p___std_create_threadpool_work(threadpool_workcallback, &workcalled,
                                          (TP_CALLBACK_ENVIRON *)&environment3);
    gle = GetLastError();
    ok(gle == 0xdeadbeef, "expected 0xdeadbeef, got %lx\n", gle);
    ok(!!work, "failed to create threadpool_work\n");
    p___std_close_threadpool_work(work);

    memset(&environment3, 0, sizeof(environment3));
    environment3.Version = 3;
    environment3.CallbackPriority = TP_CALLBACK_PRIORITY_INVALID;
    SetLastError(0xdeadbeef);
    work = p___std_create_threadpool_work(threadpool_workcallback, &workcalled,
                                          (TP_CALLBACK_ENVIRON *)&environment3);
    gle = GetLastError();
    ok(gle == ERROR_INVALID_PARAMETER, "expected %d, got %ld\n", ERROR_INVALID_PARAMETER, gle);
    ok(!work, "expected failure\n");
}

LONG64 address;
static void __cdecl atomic_wait_thread(void *arg)
{
    LONG64 compare = 0;
    int r;

    r  = p___std_atomic_wait_direct(&address, &compare, sizeof(address), 2000);
    ok(r == 1, "r = %d\n", r);
}

static void test___std_atomic_wait_direct(void)
{
    LONG64 compare;
    HANDLE thread;
    DWORD gle;
    int r;

    if (!GetProcAddress(GetModuleHandleA("kernelbase"), "WaitOnAddress"))
    {
        win_skip("WaitOnAddress not available\n");
        return;
    }

    address = compare = 0;
    SetLastError(0);
    r = p___std_atomic_wait_direct(&address, &compare, 5, 0);
    ok(!r, "r = %d\n", r);
    gle = GetLastError();
    ok(gle == ERROR_INVALID_PARAMETER, "expected %d, got %ld\n", ERROR_INVALID_PARAMETER, gle);

    SetLastError(0);
    r = p___std_atomic_wait_direct(&address, &compare, 1, 0);
    ok(!r, "r = %d\n", r);
    gle = GetLastError();
    ok(gle == ERROR_TIMEOUT, "expected %d, got %ld\n", ERROR_TIMEOUT, gle);
    r = p___std_atomic_wait_direct(&address, &compare, 2, 0);
    ok(!r, "r = %d\n", r);
    gle = GetLastError();
    ok(gle == ERROR_TIMEOUT, "expected %d, got %ld\n", ERROR_TIMEOUT, gle);
    r = p___std_atomic_wait_direct(&address, &compare, 4, 0);
    ok(!r, "r = %d\n", r);
    gle = GetLastError();
    ok(gle == ERROR_TIMEOUT, "expected %d, got %ld\n", ERROR_TIMEOUT, gle);
    r = p___std_atomic_wait_direct(&address, &compare, 8, 0);
    ok(!r, "r = %d\n", r);
    gle = GetLastError();
    ok(gle == ERROR_TIMEOUT, "expected %d, got %ld\n", ERROR_TIMEOUT, gle);

    SetLastError(0);
    r = p___std_atomic_wait_direct(&address, &compare, 8, 1);
    ok(!r, "r = %d\n", r);
    gle = GetLastError();
    ok(gle == ERROR_TIMEOUT, "expected %d, got %ld\n", ERROR_TIMEOUT, gle);

    compare = 1;
    SetLastError(0);
    r = p___std_atomic_wait_direct(&address, &compare, 8, 0);
    ok(r == 1, "r = %d\n", r);
    gle = GetLastError();
    ok(!gle, "expected 0, got %ld\n", gle);

    thread = (HANDLE)_beginthread(atomic_wait_thread, 0, NULL);
    ok(thread != INVALID_HANDLE_VALUE, "_beginthread failed\n");
    Sleep(100);
    address = 1;
    p___std_atomic_notify_one_direct(&address);
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
}

static LONG count;
static unsigned WINAPI execution_wait_thread(void *arg)
{
    unsigned char *phase = arg;

    if(!InterlockedDecrement(&count)) {
        *phase = 1;
        p___std_execution_wake_by_address_all(phase);
    }else {
        while(!*phase)
            p___std_execution_wait_on_uchar(phase, 0);
    }

    while(*phase == 1)
        p___std_execution_wait_on_uchar(phase, 1);

    InterlockedIncrement(&count);
    return 0;
}

static void test___std_execution(void)
{
    int i;
    DWORD ret;
    HANDLE threads[4];
    unsigned char phase;

    p___std_execution_wake_by_address_all(NULL);
    if(0) p___std_execution_wait_on_uchar(NULL, 0); /* crash on windows */

    count = ARRAY_SIZE(threads);
    phase = 0;
    for(i=0; i<ARRAY_SIZE(threads); i++) {
        threads[i] = (HANDLE)_beginthreadex(NULL, 0, execution_wait_thread, &phase, 0, NULL);
        ok(threads[i] != INVALID_HANDLE_VALUE, "_beginthreadex failed\n");
    }

    Sleep(0);
    while(!phase)
        p___std_execution_wait_on_uchar(&phase, 0);
    ok(!count, "expected 0, got %ld\n", count);

    phase = 2;
    p___std_execution_wake_by_address_all(&phase);

    ret = WaitForMultipleObjects(ARRAY_SIZE(threads), threads, TRUE, 1000);
    ok(ret == WAIT_OBJECT_0, "expected WAIT_OBJECT_0, got %ld\n", ret);
    ok(count == ARRAY_SIZE(threads), "expected %Iu, got %lu\n", ARRAY_SIZE(threads), count);
    for(i=0; i<ARRAY_SIZE(threads); i++)
        CloseHandle(threads[i]);
}

static void test___std_acquire_shared_mutex_for_instance(void)
{
    shared_mutex *ret1, *ret2;

    ret1 = p___std_acquire_shared_mutex_for_instance(NULL);
    ok(ret1 != NULL, "got %p\n", ret1);
    ret2 = p___std_acquire_shared_mutex_for_instance(NULL);
    ok(ret2 != NULL, "got %p\n", ret2);
    ok(ret1 == ret2, "got different instances of shared mutex\n");

    ret2 = p___std_acquire_shared_mutex_for_instance((void *)1);
    ok(ret2 != NULL, "got %p\n", ret2);
    ok(ret1 != ret2, "got the same instance of shared mutex\n");

    p___std_release_shared_mutex_for_instance(NULL);
    p___std_release_shared_mutex_for_instance((void *)1);

    AcquireSRWLockExclusive(&ret1->srwlock);
    ReleaseSRWLockExclusive(&ret1->srwlock);
    p___std_release_shared_mutex_for_instance(NULL);
    p___std_release_shared_mutex_for_instance(NULL);
}

static void test___std_tzdb(void)
{
    struct tzdb_time_zones *z;
    unsigned int i;

    if (!p___std_tzdb_get_time_zones)
    {
        win_skip("__std_tzdb_get_time_zones is not available, skipping tests.\n");
        return;
    }

    z = p___std_tzdb_get_time_zones();
    ok(!!z, "got NULL.\n");
    ok(!z->error || broken(z->error == TZDB_ERROR_WIN && !z->ver && !z->count), "got %u.\n", z->error);
    if (z->error)
    {
        win_skip("__std_tzdb_get_time_zones empty result, skipping remaining tests.\n");
        p___std_tzdb_delete_time_zones(z);
        return;
    }
    ok(!!z->ver, "got NULL.\n");
    ok(z->count, "got 0.\n");

    trace("ver %s.\n", debugstr_a(z->ver));
    for (i = 0; i < z->count; ++i)
    {
        ok(!!z->names[i], "got NULL.\n");
    }

    p___std_tzdb_delete_time_zones(z);
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
    test___std_atomic_wait_direct();
    test___std_execution();
    test___std_acquire_shared_mutex_for_instance();
    test___std_tzdb();
    FreeLibrary(msvcp);
}
