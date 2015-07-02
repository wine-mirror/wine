/*
 * Unit test suite for thread pool functions
 *
 * Copyright 2015 Sebastian Lackner
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

#include "ntdll_test.h"

static HMODULE hntdll = 0;
static NTSTATUS (WINAPI *pTpAllocCleanupGroup)(TP_CLEANUP_GROUP **);
static NTSTATUS (WINAPI *pTpAllocPool)(TP_POOL **,PVOID);
static NTSTATUS (WINAPI *pTpAllocWork)(TP_WORK **,PTP_WORK_CALLBACK,PVOID,TP_CALLBACK_ENVIRON *);
static NTSTATUS (WINAPI *pTpCallbackMayRunLong)(TP_CALLBACK_INSTANCE *);
static VOID     (WINAPI *pTpCallbackReleaseSemaphoreOnCompletion)(TP_CALLBACK_INSTANCE *,HANDLE,DWORD);
static VOID     (WINAPI *pTpDisassociateCallback)(TP_CALLBACK_INSTANCE *);
static VOID     (WINAPI *pTpPostWork)(TP_WORK *);
static VOID     (WINAPI *pTpReleaseCleanupGroup)(TP_CLEANUP_GROUP *);
static VOID     (WINAPI *pTpReleaseCleanupGroupMembers)(TP_CLEANUP_GROUP *,BOOL,PVOID);
static VOID     (WINAPI *pTpReleasePool)(TP_POOL *);
static VOID     (WINAPI *pTpReleaseWork)(TP_WORK *);
static VOID     (WINAPI *pTpSetPoolMaxThreads)(TP_POOL *,DWORD);
static NTSTATUS (WINAPI *pTpSimpleTryPost)(PTP_SIMPLE_CALLBACK,PVOID,TP_CALLBACK_ENVIRON *);
static VOID     (WINAPI *pTpWaitForWork)(TP_WORK *,BOOL);

#define NTDLL_GET_PROC(func) \
    do \
    { \
        p ## func = (void *)GetProcAddress(hntdll, #func); \
        if (!p ## func) trace("Failed to get address for %s\n", #func); \
    } \
    while (0)

static BOOL init_threadpool(void)
{
    hntdll = GetModuleHandleA("ntdll");
    if (!hntdll)
    {
        win_skip("Could not load ntdll\n");
        return FALSE;
    }

    NTDLL_GET_PROC(TpAllocCleanupGroup);
    NTDLL_GET_PROC(TpAllocPool);
    NTDLL_GET_PROC(TpAllocWork);
    NTDLL_GET_PROC(TpCallbackMayRunLong);
    NTDLL_GET_PROC(TpCallbackReleaseSemaphoreOnCompletion);
    NTDLL_GET_PROC(TpDisassociateCallback);
    NTDLL_GET_PROC(TpPostWork);
    NTDLL_GET_PROC(TpReleaseCleanupGroup);
    NTDLL_GET_PROC(TpReleaseCleanupGroupMembers);
    NTDLL_GET_PROC(TpReleasePool);
    NTDLL_GET_PROC(TpReleaseWork);
    NTDLL_GET_PROC(TpSetPoolMaxThreads);
    NTDLL_GET_PROC(TpSimpleTryPost);
    NTDLL_GET_PROC(TpWaitForWork);

    if (!pTpAllocPool)
    {
        win_skip("Threadpool functions not supported, skipping tests\n");
        return FALSE;
    }

    return TRUE;
}

#undef NTDLL_GET_PROC


static void CALLBACK simple_cb(TP_CALLBACK_INSTANCE *instance, void *userdata)
{
    HANDLE semaphore = userdata;
    trace("Running simple callback\n");
    ReleaseSemaphore(semaphore, 1, NULL);
}

static void CALLBACK simple2_cb(TP_CALLBACK_INSTANCE *instance, void *userdata)
{
    trace("Running simple2 callback\n");
    Sleep(50);
    InterlockedIncrement((LONG *)userdata);
}

static void test_tp_simple(void)
{
    TP_CALLBACK_ENVIRON environment;
    TP_CLEANUP_GROUP *group;
    HANDLE semaphore;
    NTSTATUS status;
    TP_POOL *pool;
    LONG userdata;
    DWORD result;
    int i;

    semaphore = CreateSemaphoreA(NULL, 0, 1, NULL);
    ok(semaphore != NULL, "CreateSemaphoreA failed %u\n", GetLastError());

    /* post the callback using the default threadpool */
    memset(&environment, 0, sizeof(environment));
    environment.Version = 1;
    environment.Pool = NULL;
    status = pTpSimpleTryPost(simple_cb, semaphore, &environment);
    ok(!status, "TpSimpleTryPost failed with status %x\n", status);
    result = WaitForSingleObject(semaphore, 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %u\n", result);

    /* allocate new threadpool */
    pool = NULL;
    status = pTpAllocPool(&pool, NULL);
    ok(!status, "TpAllocPool failed with status %x\n", status);
    ok(pool != NULL, "expected pool != NULL\n");

    /* post the callback using the new threadpool */
    memset(&environment, 0, sizeof(environment));
    environment.Version = 1;
    environment.Pool = pool;
    status = pTpSimpleTryPost(simple_cb, semaphore, &environment);
    ok(!status, "TpSimpleTryPost failed with status %x\n", status);
    result = WaitForSingleObject(semaphore, 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %u\n", result);

    /* test with invalid version number */
    memset(&environment, 0, sizeof(environment));
    environment.Version = 9999;
    environment.Pool = pool;
    status = pTpSimpleTryPost(simple_cb, semaphore, &environment);
    todo_wine
    ok(status == STATUS_INVALID_PARAMETER || broken(!status) /* Vista/2008 */,
       "TpSimpleTryPost unexpectedly returned status %x\n", status);
    if (!status)
    {
        result = WaitForSingleObject(semaphore, 1000);
        ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %u\n", result);
    }

    /* allocate a cleanup group for synchronization */
    group = NULL;
    status = pTpAllocCleanupGroup(&group);
    ok(!status, "TpAllocCleanupGroup failed with status %x\n", status);
    ok(group != NULL, "expected pool != NULL\n");

    /* use cleanup group to wait for a simple callback */
    userdata = 0;
    memset(&environment, 0, sizeof(environment));
    environment.Version = 1;
    environment.Pool = pool;
    environment.CleanupGroup = group;
    status = pTpSimpleTryPost(simple2_cb, &userdata, &environment);
    ok(!status, "TpSimpleTryPost failed with status %x\n", status);
    pTpReleaseCleanupGroupMembers(group, FALSE, NULL);
    ok(userdata == 1, "expected userdata = 1, got %u\n", userdata);

    /* test cancellation of pending simple callbacks */
    userdata = 0;
    memset(&environment, 0, sizeof(environment));
    environment.Version = 1;
    environment.Pool = pool;
    environment.CleanupGroup = group;
    for (i = 0; i < 100; i++)
    {
        status = pTpSimpleTryPost(simple2_cb, &userdata, &environment);
        ok(!status, "TpSimpleTryPost failed with status %x\n", status);
    }
    pTpReleaseCleanupGroupMembers(group, TRUE, NULL);
    ok(userdata < 100, "expected userdata < 100, got %u\n", userdata);

    /* cleanup */
    pTpReleaseCleanupGroup(group);
    pTpReleasePool(pool);
    CloseHandle(semaphore);
}

static void CALLBACK work_cb(TP_CALLBACK_INSTANCE *instance, void *userdata, TP_WORK *work)
{
    trace("Running work callback\n");
    Sleep(10);
    InterlockedIncrement((LONG *)userdata);
}

static void CALLBACK work2_cb(TP_CALLBACK_INSTANCE *instance, void *userdata, TP_WORK *work)
{
    trace("Running work2 callback\n");
    Sleep(10);
    InterlockedExchangeAdd((LONG *)userdata, 0x10000);
}

static void test_tp_work(void)
{
    TP_CALLBACK_ENVIRON environment;
    TP_WORK *work;
    TP_POOL *pool;
    NTSTATUS status;
    LONG userdata;
    int i;

    /* allocate new threadpool */
    pool = NULL;
    status = pTpAllocPool(&pool, NULL);
    ok(!status, "TpAllocPool failed with status %x\n", status);
    ok(pool != NULL, "expected pool != NULL\n");

    /* allocate new work item */
    work = NULL;
    memset(&environment, 0, sizeof(environment));
    environment.Version = 1;
    environment.Pool = pool;
    status = pTpAllocWork(&work, work_cb, &userdata, &environment);
    ok(!status, "TpAllocWork failed with status %x\n", status);
    ok(work != NULL, "expected work != NULL\n");

    /* post 10 identical work items at once */
    userdata = 0;
    for (i = 0; i < 10; i++)
        pTpPostWork(work);
    pTpWaitForWork(work, FALSE);
    ok(userdata == 10, "expected userdata = 10, got %u\n", userdata);

    /* add more tasks and cancel them immediately */
    userdata = 0;
    for (i = 0; i < 10; i++)
        pTpPostWork(work);
    pTpWaitForWork(work, TRUE);
    ok(userdata < 10, "expected userdata < 10, got %u\n", userdata);

    /* cleanup */
    pTpReleaseWork(work);
    pTpReleasePool(pool);
}

static void test_tp_work_scheduler(void)
{
    TP_CALLBACK_ENVIRON environment;
    TP_CLEANUP_GROUP *group;
    TP_WORK *work, *work2;
    TP_POOL *pool;
    NTSTATUS status;
    LONG userdata;
    int i;

    /* allocate new threadpool */
    pool = NULL;
    status = pTpAllocPool(&pool, NULL);
    ok(!status, "TpAllocPool failed with status %x\n", status);
    ok(pool != NULL, "expected pool != NULL\n");

    /* we limit the pool to a single thread */
    pTpSetPoolMaxThreads(pool, 1);

    /* create a cleanup group */
    group = NULL;
    status = pTpAllocCleanupGroup(&group);
    ok(!status, "TpAllocCleanupGroup failed with status %x\n", status);
    ok(group != NULL, "expected pool != NULL\n");

    /* the first work item has no cleanup group associated */
    work = NULL;
    memset(&environment, 0, sizeof(environment));
    environment.Version = 1;
    environment.Pool = pool;
    status = pTpAllocWork(&work, work_cb, &userdata, &environment);
    ok(!status, "TpAllocWork failed with status %x\n", status);
    ok(work != NULL, "expected work != NULL\n");

    /* allocate a second work item with a cleanup group */
    work2 = NULL;
    memset(&environment, 0, sizeof(environment));
    environment.Version = 1;
    environment.Pool = pool;
    environment.CleanupGroup = group;
    status = pTpAllocWork(&work2, work2_cb, &userdata, &environment);
    ok(!status, "TpAllocWork failed with status %x\n", status);
    ok(work2 != NULL, "expected work2 != NULL\n");

    /* the 'work' callbacks are not blocking execution of 'work2' callbacks */
    userdata = 0;
    for (i = 0; i < 10; i++)
        pTpPostWork(work);
    for (i = 0; i < 10; i++)
        pTpPostWork(work2);
    Sleep(30);
    pTpWaitForWork(work, TRUE);
    pTpWaitForWork(work2, TRUE);
    ok(userdata & 0xffff, "expected userdata & 0xffff != 0, got %u\n", userdata & 0xffff);
    ok(userdata >> 16, "expected userdata >> 16 != 0, got %u\n", userdata >> 16);

    /* test TpReleaseCleanupGroupMembers on a work item */
    userdata = 0;
    for (i = 0; i < 100; i++)
        pTpPostWork(work);
    for (i = 0; i < 10; i++)
        pTpPostWork(work2);
    pTpReleaseCleanupGroupMembers(group, FALSE, NULL);
    pTpWaitForWork(work, TRUE);
    ok((userdata & 0xffff) < 100, "expected userdata & 0xffff < 100, got %u\n", userdata & 0xffff);
    ok((userdata >> 16) == 10, "expected userdata >> 16 == 10, got %u\n", userdata >> 16);

    /* cleanup */
    pTpReleaseWork(work);
    pTpReleaseCleanupGroup(group);
    pTpReleasePool(pool);
}

static DWORD group_cancel_tid;

static void CALLBACK group_cancel_cb(TP_CALLBACK_INSTANCE *instance, void *userdata)
{
    HANDLE *semaphores = userdata;
    NTSTATUS status;
    DWORD result;

    trace("Running group cancel callback\n");

    status = pTpCallbackMayRunLong(instance);
    ok(status == STATUS_TOO_MANY_THREADS || broken(status == 1) /* Win Vista / 2008 */,
       "expected STATUS_TOO_MANY_THREADS, got %08x\n", status);

    result = WaitForSingleObject(semaphores[0], 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %u\n", result);
    ReleaseSemaphore(semaphores[1], 1, NULL);
}

static void CALLBACK group_cancel_cleanup_release_cb(void *object, void *userdata)
{
    HANDLE *semaphores = userdata;
    trace("Running group cancel cleanup release callback\n");
    group_cancel_tid = GetCurrentThreadId();
    ReleaseSemaphore(semaphores[0], 1, NULL);
}

static void CALLBACK group_cancel_cleanup_increment_cb(void *object, void *userdata)
{
    trace("Running group cancel cleanup increment callback\n");
    group_cancel_tid = GetCurrentThreadId();
    InterlockedIncrement((LONG *)userdata);
}

static void CALLBACK unexpected_cb(TP_CALLBACK_INSTANCE *instance, void *userdata)
{
    ok(0, "Unexpected callback\n");
}

static void test_tp_group_cancel(void)
{
    TP_CALLBACK_ENVIRON environment;
    TP_CLEANUP_GROUP *group;
    LONG userdata, userdata2;
    HANDLE semaphores[2];
    NTSTATUS status;
    TP_WORK *work;
    TP_POOL *pool;
    DWORD result;
    int i;

    semaphores[0] = CreateSemaphoreA(NULL, 0, 1, NULL);
    ok(semaphores[0] != NULL, "CreateSemaphoreA failed %u\n", GetLastError());
    semaphores[1] = CreateSemaphoreA(NULL, 0, 1, NULL);
    ok(semaphores[1] != NULL, "CreateSemaphoreA failed %u\n", GetLastError());

    /* allocate new threadpool with only one thread */
    pool = NULL;
    status = pTpAllocPool(&pool, NULL);
    ok(!status, "TpAllocPool failed with status %x\n", status);
    ok(pool != NULL, "expected pool != NULL\n");
    pTpSetPoolMaxThreads(pool, 1);

    /* allocate a cleanup group */
    group = NULL;
    status = pTpAllocCleanupGroup(&group);
    ok(!status, "TpAllocCleanupGroup failed with status %x\n", status);
    ok(group != NULL, "expected pool != NULL\n");

    /* test execution of cancellation callback */
    memset(&environment, 0, sizeof(environment));
    environment.Version = 1;
    environment.Pool = pool;
    status = pTpSimpleTryPost(group_cancel_cb, semaphores, &environment);
    ok(!status, "TpSimpleTryPost failed with status %x\n", status);

    memset(&environment, 0, sizeof(environment));
    environment.Version = 1;
    environment.Pool = pool;
    environment.CleanupGroup = group;
    environment.CleanupGroupCancelCallback = group_cancel_cleanup_release_cb;
    status = pTpSimpleTryPost(unexpected_cb, NULL, &environment);
    ok(!status, "TpSimpleTryPost failed with status %x\n", status);

    group_cancel_tid = 0xdeadbeef;
    pTpReleaseCleanupGroupMembers(group, TRUE, semaphores);
    result = WaitForSingleObject(semaphores[1], 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %u\n", result);
    ok(group_cancel_tid == GetCurrentThreadId(), "expected tid %x, got %x\n",
       GetCurrentThreadId(), group_cancel_tid);

    /* test cancellation callback for objects with multiple instances */
    work = NULL;
    memset(&environment, 0, sizeof(environment));
    environment.Version = 1;
    environment.Pool = pool;
    environment.CleanupGroup = group;
    environment.CleanupGroupCancelCallback = group_cancel_cleanup_increment_cb;
    status = pTpAllocWork(&work, work_cb, &userdata, &environment);
    ok(!status, "TpAllocWork failed with status %x\n", status);
    ok(work != NULL, "expected work != NULL\n");

    /* post 10 identical work items at once */
    userdata = userdata2 = 0;
    for (i = 0; i < 10; i++)
        pTpPostWork(work);

    /* check if we get multiple cancellation callbacks */
    group_cancel_tid = 0xdeadbeef;
    pTpReleaseCleanupGroupMembers(group, TRUE, &userdata2);
    ok(userdata <= 8, "expected userdata <= 8, got %u\n", userdata);
    ok(userdata2 == 1, "expected only one cancellation callback, got %u\n", userdata2);
    ok(group_cancel_tid == GetCurrentThreadId(), "expected tid %x, got %x\n",
       GetCurrentThreadId(), group_cancel_tid);

    /* cleanup */
    pTpReleaseCleanupGroup(group);
    pTpReleasePool(pool);
    CloseHandle(semaphores[0]);
    CloseHandle(semaphores[1]);
}

static void CALLBACK instance_semaphore_completion_cb(TP_CALLBACK_INSTANCE *instance, void *userdata)
{
    HANDLE *semaphores = userdata;
    trace("Running instance completion callback\n");
    pTpCallbackReleaseSemaphoreOnCompletion(instance, semaphores[0], 1);
}

static void CALLBACK instance_finalization_cb(TP_CALLBACK_INSTANCE *instance, void *userdata)
{
    HANDLE *semaphores = userdata;
    DWORD result;

    trace("Running instance finalization callback\n");

    result = WaitForSingleObject(semaphores[0], 100);
    ok(result == WAIT_TIMEOUT, "WaitForSingleObject returned %u\n", result);
    ReleaseSemaphore(semaphores[1], 1, NULL);
}

static void test_tp_instance(void)
{
    TP_CALLBACK_ENVIRON environment;
    HANDLE semaphores[2];
    NTSTATUS status;
    TP_POOL *pool;
    DWORD result;

    semaphores[0] = CreateSemaphoreW(NULL, 0, 1, NULL);
    ok(semaphores[0] != NULL, "failed to create semaphore\n");
    semaphores[1] = CreateSemaphoreW(NULL, 0, 1, NULL);
    ok(semaphores[1] != NULL, "failed to create semaphore\n");

    /* allocate new threadpool */
    pool = NULL;
    status = pTpAllocPool(&pool, NULL);
    ok(!status, "TpAllocPool failed with status %x\n", status);
    ok(pool != NULL, "expected pool != NULL\n");

    /* test for TpCallbackReleaseSemaphoreOnCompletion */
    memset(&environment, 0, sizeof(environment));
    environment.Version = 1;
    environment.Pool = pool;
    status = pTpSimpleTryPost(instance_semaphore_completion_cb, semaphores, &environment);
    ok(!status, "TpSimpleTryPost failed with status %x\n", status);
    result = WaitForSingleObject(semaphores[0], 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %u\n", result);

    /* test for finalization callback */
    memset(&environment, 0, sizeof(environment));
    environment.Version = 1;
    environment.Pool = pool;
    environment.FinalizationCallback = instance_finalization_cb;
    status = pTpSimpleTryPost(instance_semaphore_completion_cb, semaphores, &environment);
    ok(!status, "TpSimpleTryPost failed with status %x\n", status);
    result = WaitForSingleObject(semaphores[0], 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %u\n", result);
    result = WaitForSingleObject(semaphores[1], 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %u\n", result);

    /* cleanup */
    pTpReleasePool(pool);
    CloseHandle(semaphores[0]);
    CloseHandle(semaphores[1]);
}

static void CALLBACK disassociate_cb(TP_CALLBACK_INSTANCE *instance, void *userdata, TP_WORK *work)
{
    HANDLE *semaphores = userdata;
    DWORD result;

    trace("Running disassociate callback\n");

    pTpDisassociateCallback(instance);
    result = WaitForSingleObject(semaphores[0], 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %u\n", result);
    ReleaseSemaphore(semaphores[1], 1, NULL);
}

static void CALLBACK disassociate2_cb(TP_CALLBACK_INSTANCE *instance, void *userdata, TP_WORK *work)
{
    HANDLE *semaphores = userdata;
    DWORD result;

    trace("Running disassociate2 callback\n");

    pTpDisassociateCallback(instance);
    result = WaitForSingleObject(semaphores[0], 100);
    ok(result == WAIT_TIMEOUT, "WaitForSingleObject returned %u\n", result);
    ReleaseSemaphore(semaphores[1], 1, NULL);
}

static void CALLBACK disassociate3_cb(TP_CALLBACK_INSTANCE *instance, void *userdata)
{
    HANDLE *semaphores = userdata;
    DWORD result;

    trace("Running disassociate3 callback\n");

    pTpDisassociateCallback(instance);
    result = WaitForSingleObject(semaphores[0], 100);
    ok(result == WAIT_TIMEOUT, "WaitForSingleObject returned %u\n", result);
    ReleaseSemaphore(semaphores[1], 1, NULL);
}

static void test_tp_disassociate(void)
{
    TP_CALLBACK_ENVIRON environment;
    TP_CLEANUP_GROUP *group;
    HANDLE semaphores[2];
    NTSTATUS status;
    TP_POOL *pool;
    TP_WORK *work;
    DWORD result;

    semaphores[0] = CreateSemaphoreW(NULL, 0, 1, NULL);
    ok(semaphores[0] != NULL, "failed to create semaphore\n");
    semaphores[1] = CreateSemaphoreW(NULL, 0, 1, NULL);
    ok(semaphores[1] != NULL, "failed to create semaphore\n");

    /* allocate new threadpool and cleanup group */
    pool = NULL;
    status = pTpAllocPool(&pool, NULL);
    ok(!status, "TpAllocPool failed with status %x\n", status);
    ok(pool != NULL, "expected pool != NULL\n");

    group = NULL;
    status = pTpAllocCleanupGroup(&group);
    ok(!status, "TpAllocCleanupGroup failed with status %x\n", status);
    ok(group != NULL, "expected pool != NULL\n");

    /* test TpDisassociateCallback on work objects without group */
    work = NULL;
    memset(&environment, 0, sizeof(environment));
    environment.Version = 1;
    environment.Pool = pool;
    status = pTpAllocWork(&work, disassociate_cb, semaphores, &environment);
    ok(!status, "TpAllocWork failed with status %x\n", status);
    ok(work != NULL, "expected work != NULL\n");

    pTpPostWork(work);
    pTpWaitForWork(work, FALSE);

    result = WaitForSingleObject(semaphores[1], 100);
    ok(result == WAIT_TIMEOUT, "WaitForSingleObject returned %u\n", result);
    ReleaseSemaphore(semaphores[0], 1, NULL);
    result = WaitForSingleObject(semaphores[1], 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %u\n", result);
    pTpReleaseWork(work);

    /* test TpDisassociateCallback on work objects with group (1) */
    work = NULL;
    memset(&environment, 0, sizeof(environment));
    environment.Version = 1;
    environment.Pool = pool;
    environment.CleanupGroup = group;
    status = pTpAllocWork(&work, disassociate_cb, semaphores, &environment);
    ok(!status, "TpAllocWork failed with status %x\n", status);
    ok(work != NULL, "expected work != NULL\n");

    pTpPostWork(work);
    pTpWaitForWork(work, FALSE);

    result = WaitForSingleObject(semaphores[1], 100);
    ok(result == WAIT_TIMEOUT, "WaitForSingleObject returned %u\n", result);
    ReleaseSemaphore(semaphores[0], 1, NULL);
    result = WaitForSingleObject(semaphores[1], 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %u\n", result);
    pTpReleaseCleanupGroupMembers(group, FALSE, NULL);

    /* test TpDisassociateCallback on work objects with group (2) */
    work = NULL;
    memset(&environment, 0, sizeof(environment));
    environment.Version = 1;
    environment.Pool = pool;
    environment.CleanupGroup = group;
    status = pTpAllocWork(&work, disassociate2_cb, semaphores, &environment);
    ok(!status, "TpAllocWork failed with status %x\n", status);
    ok(work != NULL, "expected work != NULL\n");

    pTpPostWork(work);
    pTpReleaseCleanupGroupMembers(group, FALSE, NULL);

    ReleaseSemaphore(semaphores[0], 1, NULL);
    result = WaitForSingleObject(semaphores[1], 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %u\n", result);
    result = WaitForSingleObject(semaphores[0], 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %u\n", result);

    /* test TpDisassociateCallback on simple callbacks */
    memset(&environment, 0, sizeof(environment));
    environment.Version = 1;
    environment.Pool = pool;
    environment.CleanupGroup = group;
    status = pTpSimpleTryPost(disassociate3_cb, semaphores, &environment);
    ok(!status, "TpSimpleTryPost failed with status %x\n", status);

    pTpReleaseCleanupGroupMembers(group, FALSE, NULL);

    ReleaseSemaphore(semaphores[0], 1, NULL);
    result = WaitForSingleObject(semaphores[1], 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %u\n", result);
    result = WaitForSingleObject(semaphores[0], 1000);
    ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %u\n", result);

    /* cleanup */
    pTpReleaseCleanupGroup(group);
    pTpReleasePool(pool);
    CloseHandle(semaphores[0]);
    CloseHandle(semaphores[1]);
}

START_TEST(threadpool)
{
    if (!init_threadpool())
        return;

    test_tp_simple();
    test_tp_work();
    test_tp_work_scheduler();
    test_tp_group_cancel();
    test_tp_instance();
    test_tp_disassociate();
}
