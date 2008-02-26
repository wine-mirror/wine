/*
 * Unit test suite for bits functions
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
#include "initguid.h"
#include "bits.h"

static void
test_CreateInstance(void)
{
    HRESULT hres;
    ULONG res;
    IBackgroundCopyManager *manager = NULL;

    /* Creating BITS instance */
    hres = CoCreateInstance(&CLSID_BackgroundCopyManager, NULL, CLSCTX_LOCAL_SERVER,
                            &IID_IBackgroundCopyManager, (void **) &manager);
    ok(hres == S_OK, "CoCreateInstance failed: %08x\n", hres);
    if(hres != S_OK) {
        skip("Unable to create bits instance.\n");
        return;
    }

    /* Releasing bits manager */
    res = IBackgroundCopyManager_Release(manager);
    ok(res == 0, "Bad ref count on release: %u\n", res);

}

static void test_CreateJob(void)
{
    /* Job information */
    static const WCHAR copyNameW[] = {'T', 'e', 's', 't', 0};
    IBackgroundCopyJob* job = NULL;
    GUID tmpId;
    HRESULT hres;
    ULONG res;
    IBackgroundCopyManager* manager = NULL;

    /* Setup */
    hres = CoCreateInstance(&CLSID_BackgroundCopyManager, NULL,
                            CLSCTX_LOCAL_SERVER, &IID_IBackgroundCopyManager,
                            (void **) &manager);
    if(hres != S_OK)
    {
        skip("Unable to create bits instance required for test.\n");
        return;
    }

    /* Create bits job */
    hres = IBackgroundCopyManager_CreateJob(manager, copyNameW,
                                            BG_JOB_TYPE_DOWNLOAD, &tmpId,
                                            &job);
    ok(hres == S_OK, "CreateJob failed: %08x\n", hres);
    if(hres != S_OK)
        skip("Unable to create bits job.\n");
    else
    {
        res = IBackgroundCopyJob_Release(job);
        ok(res == 0, "Bad ref count on release: %u\n", res);
    }

    IBackgroundCopyManager_Release(manager);
}

static void test_EnumJobs(void)
{
    /* Job Enumerator */
    IEnumBackgroundCopyJobs* enumJobs;

    static const WCHAR copyNameW[] = {'T', 'e', 's', 't', 0};
    IBackgroundCopyManager *manager = NULL;
    IBackgroundCopyJob *job = NULL;
    HRESULT hres;
    GUID tmpId;
    ULONG res;

    /* Setup */
    hres = CoCreateInstance(&CLSID_BackgroundCopyManager, NULL,
                            CLSCTX_LOCAL_SERVER, &IID_IBackgroundCopyManager,
                            (void **) &manager);
    if(hres != S_OK)
    {
        skip("Unable to create bits instance required for test.\n");
        return;
    }
    hres = IBackgroundCopyManager_CreateJob(manager, copyNameW,
                                            BG_JOB_TYPE_DOWNLOAD, &tmpId,
                                            &job);
    if(hres != S_OK)
    {
        skip("Unable to create bits job.\n");
        IBackgroundCopyManager_Release(manager);
        return;
    }

    hres = IBackgroundCopyManager_EnumJobs(manager, 0, &enumJobs);
    ok(hres == S_OK, "EnumJobs failed: %08x\n", hres);
    if(hres != S_OK)
        skip("Unable to create job enumerator.\n");
    else
    {
        res = IEnumBackgroundCopyJobs_Release(enumJobs);
        ok(res == 0, "Bad ref count on release: %u\n", res);
    }

    /* Tear down */
    IBackgroundCopyJob_Release(job);
    IBackgroundCopyManager_Release(manager);
}

START_TEST(qmgr)
{
    CoInitialize(NULL);
    test_CreateInstance();
    test_CreateJob();
    test_EnumJobs();
    CoUninitialize();
}
