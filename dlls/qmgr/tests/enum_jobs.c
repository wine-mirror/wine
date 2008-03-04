/*
 * Unit test suite for Enum Background Copy Jobs Interface
 *
 * Copyright 2007 Google (Roy Shea)
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

#define COBJMACROS

#include "wine/test.h"
#include "bits.h"

/* Globals used by many tests */
static const WCHAR test_displayNameA[] = {'T','e','s','t','A', 0};
static const WCHAR test_displayNameB[] = {'T','e','s','t','B', 0};
static IBackgroundCopyManager *test_manager;
static IBackgroundCopyJob *test_jobA;
static IBackgroundCopyJob *test_jobB;
static IEnumBackgroundCopyJobs *test_enumJobsA;
static IEnumBackgroundCopyJobs *test_enumJobsB;
static GUID test_jobIdA;
static GUID test_jobIdB;

/* Generic test setup */
static BOOL setup(void)
{
    HRESULT hres;

    test_manager = NULL;
    test_jobA = NULL;
    test_jobB = NULL;
    memset(&test_jobIdA, 0, sizeof test_jobIdA);
    memset(&test_jobIdB, 0, sizeof test_jobIdB);

    hres = CoCreateInstance(&CLSID_BackgroundCopyManager, NULL,
                            CLSCTX_LOCAL_SERVER, &IID_IBackgroundCopyManager,
                            (void **) &test_manager);
    if(hres != S_OK)
        return FALSE;

    hres = IBackgroundCopyManager_CreateJob(test_manager, test_displayNameA,
                                            BG_JOB_TYPE_DOWNLOAD, &test_jobIdA,
                                            &test_jobA);
    if(hres != S_OK)
        return FALSE;

    hres = IBackgroundCopyManager_EnumJobs(test_manager, 0, &test_enumJobsA);
    if(hres != S_OK)
        return FALSE;

    hres = IBackgroundCopyManager_CreateJob(test_manager, test_displayNameB,
                                            BG_JOB_TYPE_DOWNLOAD, &test_jobIdB,
                                            &test_jobB);
    if(hres != S_OK)
        return FALSE;

    hres = IBackgroundCopyManager_EnumJobs(test_manager, 0, &test_enumJobsB);
    if(hres != S_OK)
        return FALSE;

    return TRUE;
}

/* Generic test cleanup */
static void teardown(void)
{
    if (test_enumJobsB)
        IEnumBackgroundCopyJobs_Release(test_enumJobsB);
    test_enumJobsB = NULL;
    if (test_jobB)
        IBackgroundCopyJob_Release(test_jobB);
    test_jobB = NULL;
    if (test_enumJobsA)
        IEnumBackgroundCopyJobs_Release(test_enumJobsA);
    test_enumJobsA = NULL;
    if (test_jobA)
        IBackgroundCopyJob_Release(test_jobA);
    test_jobA = NULL;
    if (test_manager)
        IBackgroundCopyManager_Release(test_manager);
    test_manager = NULL;
}

/* We can't assume the job count will start at any fixed number since esp
   when testing on Windows there may be other jobs created by other
   processes.  Even this approach of creating two jobs and checking the
   difference in counts could fail if a job was created in between, but
   it's probably not worth worrying about in sane test environments.  */
static void test_GetCount(void)
{
    HRESULT hres;
    ULONG jobCountA, jobCountB;

    hres = IEnumBackgroundCopyJobs_GetCount(test_enumJobsA, &jobCountA);
    ok(hres == S_OK, "GetCount failed: %08x\n", hres);
    if(hres != S_OK)
    {
        skip("Couldn't get job count\n");
        return;
    }

    hres = IEnumBackgroundCopyJobs_GetCount(test_enumJobsB, &jobCountB);
    ok(hres == S_OK, "GetCount failed: %08x\n", hres);
    if(hres != S_OK)
    {
        skip("Couldn't get job count\n");
        return;
    }

    ok(jobCountB == jobCountA + 1, "Got incorrect count\n");
}

typedef void (*test_t)(void);

START_TEST(enum_jobs)
{
    static const test_t tests[] = {
        test_GetCount,
        0
    };
    const test_t *test;

    CoInitialize(NULL);
    for (test = tests; *test; ++test)
    {
        /* Keep state seperate between tests.  */
        if (!setup())
        {
            teardown();
            skip("Unable to setup test\n");
            break;
        }
        (*test)();
        teardown();
    }
    CoUninitialize();
}
