/*
 * Unit test suite for Enum Background Copy Files Interface
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

#include <shlwapi.h>
#include <stdio.h>

#define COBJMACROS

#include "wine/test.h"
#include "bits.h"

/* Globals used by many tests */
#define NUM_FILES 2
static const WCHAR test_remoteNameA[] = {'r','e','m','o','t','e','A', 0};
static const WCHAR test_localNameA[] = {'l','o','c','a','l','A', 0};
static const WCHAR test_remoteNameB[] = {'r','e','m','o','t','e','B', 0};
static const WCHAR test_localNameB[] = {'l','o','c','a','l','B', 0};
static const WCHAR test_displayName[] = {'T', 'e', 's', 't', 0};
static const ULONG test_fileCount = NUM_FILES;
static IBackgroundCopyJob *test_job;
static IBackgroundCopyManager *test_manager;
static IEnumBackgroundCopyFiles *test_enumFiles;

/* Helper function to add a file to a job.  The helper function takes base
   file name and creates properly formed path and URL strings for creation of
   the file. */
static HRESULT addFileHelper(IBackgroundCopyJob* job,
                             const WCHAR *localName, const WCHAR *remoteName)
{
    DWORD urlSize;
    WCHAR localFile[MAX_PATH];
    WCHAR remoteUrl[MAX_PATH];
    WCHAR remoteFile[MAX_PATH];

    GetCurrentDirectoryW(MAX_PATH, localFile);
    PathAppendW(localFile, localName);
    GetCurrentDirectoryW(MAX_PATH, remoteFile);
    PathAppendW(remoteFile, remoteName);
    urlSize = MAX_PATH;
    UrlCreateFromPathW(remoteFile, remoteUrl, &urlSize, 0);
    UrlUnescapeW(remoteUrl, NULL, &urlSize, URL_UNESCAPE_INPLACE);
    return IBackgroundCopyJob_AddFile(test_job, remoteUrl, localFile);
}

/* Generic test setup */
static BOOL setup(void)
{
    HRESULT hres;
    GUID test_jobId;

    hres = CoCreateInstance(&CLSID_BackgroundCopyManager, NULL,
                            CLSCTX_LOCAL_SERVER, &IID_IBackgroundCopyManager,
                            (void **) &test_manager);
    if(hres != S_OK)
        return FALSE;

    hres = IBackgroundCopyManager_CreateJob(test_manager, test_displayName,
                                            BG_JOB_TYPE_DOWNLOAD, &test_jobId,
                                            &test_job);
    if(hres != S_OK)
    {
        IBackgroundCopyManager_Release(test_manager);
        return FALSE;
    }

    if (addFileHelper(test_job, test_localNameA, test_remoteNameA) != S_OK
        || addFileHelper(test_job, test_localNameB, test_remoteNameB) != S_OK
        || IBackgroundCopyJob_EnumFiles(test_job, &test_enumFiles) != S_OK)
    {
        IBackgroundCopyJob_Release(test_job);
        IBackgroundCopyManager_Release(test_manager);
        return FALSE;
    }

    return TRUE;
}

/* Generic test cleanup */
static void teardown(void)
{
    IEnumBackgroundCopyFiles_Release(test_enumFiles);
    IBackgroundCopyJob_Release(test_job);
    IBackgroundCopyManager_Release(test_manager);
}

/* Test GetCount */
static void test_GetCount(void)
{
    HRESULT hres;
    ULONG fileCount;

    hres = IEnumBackgroundCopyFiles_GetCount(test_enumFiles, &fileCount);
    ok(hres == S_OK, "GetCount failed: %08x\n", hres);
    if(hres != S_OK)
    {
        skip("Unable to get count from test_enumFiles.\n");
        return;
    }
    ok(fileCount == test_fileCount, "Got incorrect count\n");
}

typedef void (*test_t)(void);

START_TEST(enum_files)
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
            skip("Unable to setup test\n");
            break;
        }
        (*test)();
        teardown();
    }
    CoUninitialize();
}
