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
static NTSTATUS (WINAPI *pTpAllocPool)(TP_POOL **,PVOID);
static VOID     (WINAPI *pTpReleasePool)(TP_POOL *);
static NTSTATUS (WINAPI *pTpSimpleTryPost)(PTP_SIMPLE_CALLBACK,PVOID,TP_CALLBACK_ENVIRON *);

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

    NTDLL_GET_PROC(TpAllocPool);
    NTDLL_GET_PROC(TpReleasePool);
    NTDLL_GET_PROC(TpSimpleTryPost);

    if (!pTpAllocPool)
    {
        skip("Threadpool functions not supported, skipping tests\n");
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

static void test_tp_simple(void)
{
    TP_CALLBACK_ENVIRON environment;
    HANDLE semaphore;
    NTSTATUS status;
    TP_POOL *pool;
    DWORD result;

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
    ok(status == STATUS_INVALID_PARAMETER || broken(!status) /* Vista/2008 */,
       "TpSimpleTryPost unexpectedly returned status %x\n", status);
    if (!status)
    {
        result = WaitForSingleObject(semaphore, 1000);
        ok(result == WAIT_OBJECT_0, "WaitForSingleObject returned %u\n", result);
    }

    /* cleanup */
    pTpReleasePool(pool);
    CloseHandle(semaphore);
}

START_TEST(threadpool)
{
    if (!init_threadpool())
        return;

    test_tp_simple();
}
