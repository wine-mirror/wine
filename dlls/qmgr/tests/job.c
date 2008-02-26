/*
 * Unit test suite for Background Copy Job Interface
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
static const WCHAR test_displayName[] = {'T', 'e', 's', 't', 0};
static IBackgroundCopyManager *test_manager;
static IBackgroundCopyJob *test_job;
static GUID test_jobId;
static BG_JOB_TYPE test_type;

/* Generic test setup */
static BOOL setup(void)
{
    HRESULT hres;

    test_manager = NULL;
    test_job = NULL;
    memset(&test_jobId, 0, sizeof test_jobId);
    test_type = BG_JOB_TYPE_DOWNLOAD;

    hres = CoCreateInstance(&CLSID_BackgroundCopyManager, NULL,
                            CLSCTX_LOCAL_SERVER,
                            &IID_IBackgroundCopyManager,
                            (void **) &test_manager);
    if(hres != S_OK)
        return FALSE;

    hres = IBackgroundCopyManager_CreateJob(test_manager, test_displayName,
                                            test_type, &test_jobId, &test_job);
    if(hres != S_OK)
    {
        IBackgroundCopyManager_Release(test_manager);
        return FALSE;
    }

    return TRUE;
}

/* Generic test cleanup */
static void teardown(void)
{
    IBackgroundCopyJob_Release(test_job);
    IBackgroundCopyManager_Release(test_manager);
}

/* Test that the jobId is properly set */
static void test_GetId(void)
{
    HRESULT hres;
    GUID tmpId;

    hres = IBackgroundCopyJob_GetId(test_job, &tmpId);
    ok(hres == S_OK, "GetId failed: %08x\n", hres);
    if(hres != S_OK)
    {
        skip("Unable to get ID of test_job.\n");
        return;
    }
    ok(memcmp(&tmpId, &test_jobId, sizeof tmpId) == 0, "Got incorrect GUID\n");
}

/* Test that the type is properly set */
static void test_GetType(void)
{
    HRESULT hres;
    BG_JOB_TYPE type;

    hres = IBackgroundCopyJob_GetType(test_job, &type);
    ok(hres == S_OK, "GetType failed: %08x\n", hres);
    if(hres != S_OK)
    {
        skip("Unable to get type of test_job.\n");
        return;
    }
    ok(type == test_type, "Got incorrect type\n");
}

typedef void (*test_t)(void);

START_TEST(job)
{
    static const test_t tests[] = {
        test_GetId,
        test_GetType,
        0
    };
    const test_t *test;

    CoInitialize(NULL);
    for (test = tests; *test; ++test)
    {
        /* Keep state seperate between tests.  */
        if (!setup())
        {
            skip("Unable to setup test\n");
            break;
        }
        (*test)();
        teardown();
    }
    CoUninitialize();
}
