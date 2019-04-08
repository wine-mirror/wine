/*
 * Copyright 2019 Nikolay Sivov for CodeWeavers
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

#include <stdarg.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"

#include "initguid.h"
#include "dbgeng.h"

#include "wine/test.h"

static void test_engine_options(void)
{
    IDebugControl *control;
    ULONG options;
    HRESULT hr;

    hr = DebugCreate(&IID_IDebugControl, (void **)&control);
    ok(hr == S_OK, "Failed to create engine object, hr %#x.\n", hr);

    options = 0xf;
    hr = control->lpVtbl->GetEngineOptions(control, &options);
    ok(hr == S_OK, "Failed to get engine options, hr %#x.\n", hr);
    ok(options == 0, "Unexpected options %#x.\n", options);

    hr = control->lpVtbl->AddEngineOptions(control, DEBUG_ENGOPT_INITIAL_BREAK);
    ok(hr == S_OK, "Failed to add engine options, hr %#x.\n", hr);

    options = 0;
    hr = control->lpVtbl->GetEngineOptions(control, &options);
    ok(hr == S_OK, "Failed to get engine options, hr %#x.\n", hr);
    ok(options == DEBUG_ENGOPT_INITIAL_BREAK, "Unexpected options %#x.\n", options);

    hr = control->lpVtbl->AddEngineOptions(control, 0x00800000);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    options = 0;
    hr = control->lpVtbl->GetEngineOptions(control, &options);
    ok(hr == S_OK, "Failed to get engine options, hr %#x.\n", hr);
    ok(options == DEBUG_ENGOPT_INITIAL_BREAK, "Unexpected options %#x.\n", options);

    hr = control->lpVtbl->RemoveEngineOptions(control, 0x00800000);
    ok(hr == S_OK, "Failed to remove options, hr %#x.\n", hr);

    hr = control->lpVtbl->AddEngineOptions(control, DEBUG_ENGOPT_IGNORE_DBGHELP_VERSION);
    ok(hr == S_OK, "Failed to add engine options, hr %#x.\n", hr);

    options = 0;
    hr = control->lpVtbl->GetEngineOptions(control, &options);
    ok(hr == S_OK, "Failed to get engine options, hr %#x.\n", hr);
    ok(options == (DEBUG_ENGOPT_INITIAL_BREAK | DEBUG_ENGOPT_IGNORE_DBGHELP_VERSION),
            "Unexpected options %#x.\n", options);

    hr = control->lpVtbl->RemoveEngineOptions(control, DEBUG_ENGOPT_INITIAL_BREAK);
    ok(hr == S_OK, "Failed to remove options, hr %#x.\n", hr);

    options = 0;
    hr = control->lpVtbl->GetEngineOptions(control, &options);
    ok(hr == S_OK, "Failed to get engine options, hr %#x.\n", hr);
    ok(options == DEBUG_ENGOPT_IGNORE_DBGHELP_VERSION, "Unexpected options %#x.\n", options);

    hr = control->lpVtbl->SetEngineOptions(control, DEBUG_ENGOPT_INITIAL_BREAK);
    ok(hr == S_OK, "Failed to set options, hr %#x.\n", hr);

    options = 0;
    hr = control->lpVtbl->GetEngineOptions(control, &options);
    ok(hr == S_OK, "Failed to get engine options, hr %#x.\n", hr);
    ok(options == DEBUG_ENGOPT_INITIAL_BREAK, "Unexpected options %#x.\n", options);

    hr = control->lpVtbl->SetEngineOptions(control, 0x00800000);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = control->lpVtbl->SetEngineOptions(control, 0x00800000 | DEBUG_ENGOPT_IGNORE_DBGHELP_VERSION);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    options = 0;
    hr = control->lpVtbl->GetEngineOptions(control, &options);
    ok(hr == S_OK, "Failed to get engine options, hr %#x.\n", hr);
    ok(options == DEBUG_ENGOPT_INITIAL_BREAK, "Unexpected options %#x.\n", options);

    control->lpVtbl->Release(control);
}

START_TEST(dbgeng)
{
    test_engine_options();
}
