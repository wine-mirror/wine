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

static const WCHAR empty[] = {0};
static const WCHAR winetest_wer[] = {'w','i','n','e','t','e','s','t','_','w','e','r','.','e','x','e',0};

/* ###### */

static void test_WerAddExcludedApplication(void)
{
    HRESULT hr;

    /* clean state */
    hr = WerRemoveExcludedApplication(winetest_wer, FALSE);
    if (hr == E_NOTIMPL) {
        skip("Wer*ExcludedApplication not implemented\n");
        return;
    }
    ok((hr == S_OK) || (hr == E_FAIL) || __HRESULT_FROM_WIN32(ERROR_ENVVAR_NOT_FOUND),
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
}

/* #### */

static void test_WerRemoveExcludedApplication(void)
{
    HRESULT hr;

    /* clean state */
    hr = WerRemoveExcludedApplication(winetest_wer, FALSE);
    if (hr == E_NOTIMPL) {
        skip("Wer*ExcludedApplication not implemented\n");
        return;
    }

    ok((hr == S_OK) || (hr == E_FAIL) || __HRESULT_FROM_WIN32(ERROR_ENVVAR_NOT_FOUND),
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
    ok((hr == E_FAIL) || __HRESULT_FROM_WIN32(ERROR_ENVVAR_NOT_FOUND),
        "got 0x%x (expected E_FAIL or HRESULT_FROM_WIN32(ERROR_ENVVAR_NOT_FOUND))\n", hr);

}

/* ########################### */

START_TEST(main)
{
    test_WerAddExcludedApplication();
    test_WerRemoveExcludedApplication();
}
