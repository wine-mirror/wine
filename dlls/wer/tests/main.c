/*
 * Unit test suite for windows error reporting in Vista and above
 *
 * Copyright 2010 Detlef Riekenberg
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
 *
 */

#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"

#include "werapi.h"
#include "wine/test.h"

static const WCHAR appcrash[] = {'A','P','P','C','R','A','S','H',0};
static const WCHAR backslash[] = {'\\',0};
static const WCHAR empty[] = {0};
static const WCHAR winetest_wer[] = {'w','i','n','e','t','e','s','t','_','w','e','r','.','e','x','e',0};

/* ###### */

static void test_WerAddExcludedApplication(void)
{
    HRESULT hr;
    WCHAR buffer[MAX_PATH];
    UINT res;

    /* clean state */
    hr = WerRemoveExcludedApplication(winetest_wer, FALSE);
    ok((hr == S_OK) || (hr == E_FAIL) || (hr == __HRESULT_FROM_WIN32(ERROR_ENVVAR_NOT_FOUND)),
        "got 0x%x (expected S_OK, E_FAIL or HRESULT_FROM_WIN32(ERROR_ENVVAR_NOT_FOUND))\n", hr);

    hr = WerAddExcludedApplication(NULL, FALSE);
    ok(hr == E_INVALIDARG, "got 0x%x (expected E_INVALIDARG)\n", hr);

    hr = WerAddExcludedApplication(empty, FALSE);
    ok(hr == E_INVALIDARG, "got 0x%x (expected E_INVALIDARG)\n", hr);

    hr = WerAddExcludedApplication(winetest_wer, FALSE);
    ok(hr == S_OK, "got 0x%x (expected S_OK)\n", hr);
    /* app already in the list */
    hr = WerAddExcludedApplication(winetest_wer, FALSE);
    ok(hr == S_OK, "got 0x%x (expected S_OK)\n", hr);


    /* cleanup */
    hr = WerRemoveExcludedApplication(winetest_wer, FALSE);
    ok(hr == S_OK, "got 0x%x (expected S_OK)\n", hr);

    /* appname has a path */
    res = GetWindowsDirectoryW(buffer, ARRAY_SIZE(buffer));
    if (res > 0) {
        /* the last part from the path is added to the inclusion list */
        hr = WerAddExcludedApplication(buffer, FALSE);
        ok(hr == S_OK, "got 0x%x (expected S_OK)\n", hr);
        hr = WerRemoveExcludedApplication(buffer, FALSE);
        ok(hr == S_OK, "got 0x%x (expected S_OK)\n", hr);

        lstrcatW(buffer, backslash);
        hr = WerAddExcludedApplication(buffer, FALSE);
        ok(hr == E_INVALIDARG, "got 0x%x (expected E_INVALIDARG)\n", hr);

        lstrcatW(buffer, winetest_wer);
        hr = WerAddExcludedApplication(buffer, FALSE);
        ok(hr == S_OK, "got 0x%x (expected S_OK)\n", hr);
        hr = WerRemoveExcludedApplication(buffer, FALSE);
        ok(hr == S_OK, "got 0x%x (expected S_OK)\n", hr);
    }

}

/* #### */

static void test_WerRemoveExcludedApplication(void)
{
    HRESULT hr;
    WCHAR buffer[MAX_PATH];
    UINT res;

    /* clean state */
    hr = WerRemoveExcludedApplication(winetest_wer, FALSE);
    ok((hr == S_OK) || (hr == E_FAIL) || (hr == __HRESULT_FROM_WIN32(ERROR_ENVVAR_NOT_FOUND)),
        "got 0x%x (expected S_OK, E_FAIL or HRESULT_FROM_WIN32(ERROR_ENVVAR_NOT_FOUND))\n", hr);

    hr = WerAddExcludedApplication(winetest_wer, FALSE);
    ok(hr == S_OK, "got 0x%x (expected S_OK)\n", hr);

    hr = WerRemoveExcludedApplication(NULL, FALSE);
    ok(hr == E_INVALIDARG, "got 0x%x (expected E_INVALIDARG)\n", hr);

    hr = WerRemoveExcludedApplication(empty, FALSE);
    ok(hr == E_INVALIDARG, "got 0x%x (expected E_INVALIDARG)\n", hr);

    hr = WerRemoveExcludedApplication(winetest_wer, FALSE);
    ok(hr == S_OK, "got 0x%x (expected S_OK)\n", hr);

    /* app not in the list */
    hr = WerRemoveExcludedApplication(winetest_wer, FALSE);
    ok((hr == E_FAIL) || (hr == __HRESULT_FROM_WIN32(ERROR_ENVVAR_NOT_FOUND)),
        "got 0x%x (expected E_FAIL or HRESULT_FROM_WIN32(ERROR_ENVVAR_NOT_FOUND))\n", hr);

    /* appname has a path */
    res = GetWindowsDirectoryW(buffer, ARRAY_SIZE(buffer));
    if (res > 0) {
        hr = WerRemoveExcludedApplication(buffer, FALSE);
        ok((hr == E_FAIL) || (hr == __HRESULT_FROM_WIN32(ERROR_ENVVAR_NOT_FOUND)),
            "got 0x%x (expected E_FAIL or HRESULT_FROM_WIN32(ERROR_ENVVAR_NOT_FOUND))\n", hr);

        /* the last part from the path is added to the inclusion list */
        hr = WerAddExcludedApplication(buffer, FALSE);
        ok(hr == S_OK, "got 0x%x (expected S_OK)\n", hr);
        hr = WerRemoveExcludedApplication(buffer, FALSE);
        ok(hr == S_OK, "got 0x%x (expected S_OK)\n", hr);

        lstrcatW(buffer, backslash);
        hr = WerAddExcludedApplication(buffer, FALSE);
        ok(hr == E_INVALIDARG, "got 0x%x (expected E_INVALIDARG)\n", hr);
        hr = WerRemoveExcludedApplication(buffer, FALSE);
        ok(hr == E_INVALIDARG, "got 0x%x (expected E_INVALIDARG)\n", hr);

        lstrcatW(buffer, winetest_wer);
        hr = WerAddExcludedApplication(buffer, FALSE);
        ok(hr == S_OK, "got 0x%x (expected S_OK)\n", hr);
        hr = WerRemoveExcludedApplication(buffer, FALSE);
        ok(hr == S_OK, "got 0x%x (expected S_OK)\n", hr);
    }
}

/* #### */

static void  test_WerReportCloseHandle(void)
{
    HRESULT hr;
    HREPORT report;

    report = (void *) 0xdeadbeef;
    hr = WerReportCreate(appcrash, WerReportCritical, NULL, &report);
    ok(hr == S_OK, "got 0x%x and %p (expected S_OK)\n", hr, report);

    if (!report) {
        skip("Nothing left to test\n");
        return;
    }

    /* close the handle */
    hr = WerReportCloseHandle(report);
    ok(hr == S_OK, "got 0x%x for %p (expected S_OK)\n", hr, report);

    /* close the handle again */
    hr = WerReportCloseHandle(report);
    ok(hr == E_INVALIDARG, "got 0x%x for %p again (expected E_INVALIDARG)\n", hr, report);

    hr = WerReportCloseHandle(NULL);
    ok(hr == E_INVALIDARG, "got 0x%x for NULL(expected E_INVALIDARG)\n", hr);
}

/* #### */

static void  test_WerReportCreate(void)
{
    HRESULT hr;
    HREPORT report;
    HREPORT table[8];
    int i;

    report = (void *) 0xdeadbeef;
    /* test a simple valid case */
    hr = WerReportCreate(appcrash, WerReportCritical, NULL, &report);
    ok(hr == S_OK, "got 0x%x and %p (expected S_OK)\n", hr, report);

    if (!report) {
        skip("Nothing left to test\n");
        return;
    }

    hr = WerReportCloseHandle(report);
    ok(hr == S_OK, "got 0x%x for %p (expected S_OK)\n", hr, report);

    /* the ptr to store the created handle is always needed */
    hr = WerReportCreate(appcrash, WerReportCritical, NULL, NULL);
    ok(hr == E_INVALIDARG, "got 0x%x (expected E_INVALIDARG)\n", hr);

    /* the event type must be a valid string */
    report = (void *) 0xdeadbeef;
    hr = WerReportCreate(NULL, WerReportCritical, NULL, &report);
    ok(hr == E_INVALIDARG, "got 0x%x and %p(expected E_INVALIDARG)\n", hr, report);

    report = (void *) 0xdeadbeef;
    hr = WerReportCreate(empty, WerReportCritical, NULL, &report);
    ok(hr == E_INVALIDARG, "got 0x%x and %p(expected E_INVALIDARG)\n", hr, report);

    /* WER_REPORT_TYPE is not checked during WerReportCreate */
    for (i = 0; i <= WerReportInvalid + 1; i++) {
        report = (void *) 0xdeadbeef;
        hr = WerReportCreate(appcrash, i, NULL, &report);
        ok(hr == S_OK, "%d: got 0x%x and %p (expected S_OK)\n", i, hr, report);

        hr = WerReportCloseHandle(report);
        ok(hr == S_OK, "%d: got 0x%x for %p (expected S_OK)\n", i, hr, report);

    }
    report = (void *) 0xdeadbeef;
    hr = WerReportCreate(appcrash, 42, NULL, &report);
    ok(hr == S_OK, "42: got 0x%x and %p (expected S_OK)\n", hr, report);

    /* multiple active reports are possible */
    memset(table, 0, sizeof(table));
    for (i = 0; i < (ARRAY_SIZE(table) - 1); i++) {
        report = (void *) 0xdeadbeef;
        hr = WerReportCreate(appcrash, WerReportCritical, NULL, &table[i]);
        ok(hr == S_OK, "%02d: got 0x%x and %p (expected S_OK)\n", i, hr, table[i]);
    }

    while (i > 0) {
        i--;
        hr = WerReportCloseHandle(table[i]);
        ok(hr == S_OK, "got 0x%x for %p (expected S_OK)\n", hr, table[i]);
    }

}

/* ########################### */

START_TEST(main)
{
    test_WerAddExcludedApplication();
    test_WerRemoveExcludedApplication();
    test_WerReportCloseHandle();
    test_WerReportCreate();
}
