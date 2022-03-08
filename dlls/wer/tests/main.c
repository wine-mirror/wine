/*
 * Unit test suite for windows error reporting in Vista and above
 *
 * Copyright 2010,2019 Detlef Riekenberg
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
static const WCHAR winetest_wer[] = L"winetest_wer.exe";

/* ###### */

static void test_WerAddExcludedApplication(void)
{
    HRESULT hr;
    WCHAR buffer[MAX_PATH];
    UINT res;

    /* clean state */
    hr = WerRemoveExcludedApplication(winetest_wer, FALSE);
    ok((hr == S_OK) || (hr == E_FAIL) || (hr == __HRESULT_FROM_WIN32(ERROR_ENVVAR_NOT_FOUND)),
        "got 0x%lx (expected S_OK, E_FAIL or HRESULT_FROM_WIN32(ERROR_ENVVAR_NOT_FOUND))\n", hr);

    hr = WerAddExcludedApplication(NULL, FALSE);
    ok(hr == E_INVALIDARG, "got 0x%lx (expected E_INVALIDARG)\n", hr);

    hr = WerAddExcludedApplication(L"", FALSE);
    ok(hr == E_INVALIDARG, "got 0x%lx (expected E_INVALIDARG)\n", hr);

    hr = WerAddExcludedApplication(winetest_wer, FALSE);
    ok(hr == S_OK, "got 0x%lx (expected S_OK)\n", hr);
    /* app already in the list */
    hr = WerAddExcludedApplication(winetest_wer, FALSE);
    ok(hr == S_OK, "got 0x%lx (expected S_OK)\n", hr);


    /* cleanup */
    hr = WerRemoveExcludedApplication(winetest_wer, FALSE);
    ok(hr == S_OK, "got 0x%lx (expected S_OK)\n", hr);

    /* appname has a path */
    res = GetWindowsDirectoryW(buffer, ARRAY_SIZE(buffer));
    if (res > 0) {
        /* the last part from the path is added to the inclusion list */
        hr = WerAddExcludedApplication(buffer, FALSE);
        ok(hr == S_OK, "got 0x%lx (expected S_OK)\n", hr);
        hr = WerRemoveExcludedApplication(buffer, FALSE);
        ok(hr == S_OK, "got 0x%lx (expected S_OK)\n", hr);

        lstrcatW(buffer, L"\\");
        hr = WerAddExcludedApplication(buffer, FALSE);
        ok(hr == E_INVALIDARG, "got 0x%lx (expected E_INVALIDARG)\n", hr);

        lstrcatW(buffer, winetest_wer);
        hr = WerAddExcludedApplication(buffer, FALSE);
        ok(hr == S_OK, "got 0x%lx (expected S_OK)\n", hr);
        hr = WerRemoveExcludedApplication(buffer, FALSE);
        ok(hr == S_OK, "got 0x%lx (expected S_OK)\n", hr);
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
        "got 0x%lx (expected S_OK, E_FAIL or HRESULT_FROM_WIN32(ERROR_ENVVAR_NOT_FOUND))\n", hr);

    hr = WerAddExcludedApplication(winetest_wer, FALSE);
    ok(hr == S_OK, "got 0x%lx (expected S_OK)\n", hr);

    hr = WerRemoveExcludedApplication(NULL, FALSE);
    ok(hr == E_INVALIDARG, "got 0x%lx (expected E_INVALIDARG)\n", hr);

    hr = WerRemoveExcludedApplication(L"", FALSE);
    ok(hr == E_INVALIDARG, "got 0x%lx (expected E_INVALIDARG)\n", hr);

    hr = WerRemoveExcludedApplication(winetest_wer, FALSE);
    ok(hr == S_OK, "got 0x%lx (expected S_OK)\n", hr);

    /* app not in the list */
    hr = WerRemoveExcludedApplication(winetest_wer, FALSE);
    ok((hr == E_FAIL) || (hr == __HRESULT_FROM_WIN32(ERROR_ENVVAR_NOT_FOUND)),
        "got 0x%lx (expected E_FAIL or HRESULT_FROM_WIN32(ERROR_ENVVAR_NOT_FOUND))\n", hr);

    /* appname has a path */
    res = GetWindowsDirectoryW(buffer, ARRAY_SIZE(buffer));
    if (res > 0) {
        hr = WerRemoveExcludedApplication(buffer, FALSE);
        ok((hr == E_FAIL) || (hr == __HRESULT_FROM_WIN32(ERROR_ENVVAR_NOT_FOUND)),
            "got 0x%lx (expected E_FAIL or HRESULT_FROM_WIN32(ERROR_ENVVAR_NOT_FOUND))\n", hr);

        /* the last part from the path is added to the inclusion list */
        hr = WerAddExcludedApplication(buffer, FALSE);
        ok(hr == S_OK, "got 0x%lx (expected S_OK)\n", hr);
        hr = WerRemoveExcludedApplication(buffer, FALSE);
        ok(hr == S_OK, "got 0x%lx (expected S_OK)\n", hr);

        lstrcatW(buffer, L"\\");
        hr = WerAddExcludedApplication(buffer, FALSE);
        ok(hr == E_INVALIDARG, "got 0x%lx (expected E_INVALIDARG)\n", hr);
        hr = WerRemoveExcludedApplication(buffer, FALSE);
        ok(hr == E_INVALIDARG, "got 0x%lx (expected E_INVALIDARG)\n", hr);

        lstrcatW(buffer, winetest_wer);
        hr = WerAddExcludedApplication(buffer, FALSE);
        ok(hr == S_OK, "got 0x%lx (expected S_OK)\n", hr);
        hr = WerRemoveExcludedApplication(buffer, FALSE);
        ok(hr == S_OK, "got 0x%lx (expected S_OK)\n", hr);
    }
}

/* #### */

static void  test_WerReportCloseHandle(void)
{
    HRESULT hr;
    HREPORT report;

    report = (void *) 0xdeadbeef;
    hr = WerReportCreate(L"APPCRASH", WerReportCritical, NULL, &report);
    ok(hr == S_OK, "got 0x%lx and %p (expected S_OK)\n", hr, report);

    if (!report) {
        skip("Nothing left to test\n");
        return;
    }

    /* close the handle */
    hr = WerReportCloseHandle(report);
    ok(hr == S_OK, "got 0x%lx for %p (expected S_OK)\n", hr, report);

    /* close the handle again */
    hr = WerReportCloseHandle(report);
    ok(hr == E_INVALIDARG, "got 0x%lx for %p again (expected E_INVALIDARG)\n", hr, report);

    hr = WerReportCloseHandle(NULL);
    ok(hr == E_INVALIDARG, "got 0x%lx for NULL(expected E_INVALIDARG)\n", hr);
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
    hr = WerReportCreate(L"APPCRASH", WerReportCritical, NULL, &report);
    ok(hr == S_OK, "got 0x%lx and %p (expected S_OK)\n", hr, report);

    if (!report) {
        skip("Nothing left to test\n");
        return;
    }

    hr = WerReportCloseHandle(report);
    ok(hr == S_OK, "got 0x%lx for %p (expected S_OK)\n", hr, report);

    /* the ptr to store the created handle is always needed */
    hr = WerReportCreate(L"APPCRASH", WerReportCritical, NULL, NULL);
    ok(hr == E_INVALIDARG, "got 0x%lx (expected E_INVALIDARG)\n", hr);

    /* the event type must be a valid string */
    report = (void *) 0xdeadbeef;
    hr = WerReportCreate(NULL, WerReportCritical, NULL, &report);
    ok(hr == E_INVALIDARG, "got 0x%lx and %p(expected E_INVALIDARG)\n", hr, report);

    report = (void *) 0xdeadbeef;
    hr = WerReportCreate(L"", WerReportCritical, NULL, &report);
    ok(hr == E_INVALIDARG, "got 0x%lx and %p(expected E_INVALIDARG)\n", hr, report);

    /* a valid WER_REPORT_TYPE works */
    for (i = 0; i < WerReportInvalid; i++) {
        report = (void *) 0xdeadbeef;
        hr = WerReportCreate(L"APPCRASH", i, NULL, &report);
        ok(hr == S_OK, "%d: got 0x%lx and %p (expected S_OK)\n", i, hr, report);

        hr = WerReportCloseHandle(report);
        ok(hr == S_OK, "%d: got 0x%lx for %p (expected S_OK)\n", i, hr, report);

    }

    /* values for WER_REPORT_TYPE are checked and invalid values are rejected since recent win10,
       but older windows versions did not check the report type and WerReportCreate always succeeded */

    report = (void *) 0xdeadbeef;
    hr = WerReportCreate(L"APPCRASH", WerReportInvalid, NULL, &report);
    ok((hr == E_INVALIDARG) | broken(hr == S_OK),
        "%d: got 0x%lx and %p (expected E_INVALIDARG or a broken S_OK)\n", i, hr, report);
    if (hr == S_OK) {
        hr = WerReportCloseHandle(report);
        ok(hr == S_OK, "%d: got 0x%lx for %p (expected S_OK)\n", i, hr, report);
    }

    report = (void *) 0xdeadbeef;
    hr = WerReportCreate(L"APPCRASH", 42, NULL, &report);
    ok((hr == E_INVALIDARG) | broken(hr == S_OK),
        "%d: got 0x%lx and %p (expected E_INVALIDARG or a broken S_OK)\n", i, hr, report);
    if (hr == S_OK) {
        hr = WerReportCloseHandle(report);
        ok(hr == S_OK, "%d: got 0x%lx for %p (expected S_OK)\n", i, hr, report);
    }

    /* multiple active reports are possible */
    memset(table, 0, sizeof(table));
    for (i = 0; i < (ARRAY_SIZE(table) - 1); i++) {
        report = (void *) 0xdeadbeef;
        hr = WerReportCreate(L"APPCRASH", WerReportCritical, NULL, &table[i]);
        ok(hr == S_OK, "%02d: got 0x%lx and %p (expected S_OK)\n", i, hr, table[i]);
    }

    while (i > 0) {
        i--;
        hr = WerReportCloseHandle(table[i]);
        ok(hr == S_OK, "got 0x%lx for %p (expected S_OK)\n", hr, table[i]);
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
