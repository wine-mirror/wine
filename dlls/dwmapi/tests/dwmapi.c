/*
 * Copyright 2020 Zhiyi Zhang for CodeWeavers
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

#include "dwmapi.h"
#include "wine/test.h"

static void test_DwmIsCompositionEnabled(void)
{
    BOOL enabled;
    HRESULT hr;

    hr = DwmIsCompositionEnabled(NULL);
    ok(hr == E_INVALIDARG, "Expected %#lx, got %#lx.\n", E_INVALIDARG, hr);

    enabled = -1;
    hr = DwmIsCompositionEnabled(&enabled);
    ok(hr == S_OK, "Expected %#lx, got %#lx.\n", S_OK, hr);
    ok(enabled == TRUE || enabled == FALSE, "Got unexpected %#x.\n", enabled);
}

static void test_DwmGetCompositionTimingInfo(void)
{
    DWM_TIMING_INFO timing_info;
    BOOL enabled;
    HRESULT hr;

    enabled = FALSE;
    hr = DwmIsCompositionEnabled(&enabled);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    if (!enabled)
    {
        skip("DWM is disabled.\n");
        return;
    }

    hr = DwmGetCompositionTimingInfo(NULL, NULL);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    memset(&timing_info, 0, sizeof(timing_info));
    hr = DwmGetCompositionTimingInfo(NULL, &timing_info);
    ok(hr == MILERR_MISMATCHED_SIZE, "Got hr %#lx.\n", hr);

    timing_info.cbSize = sizeof(timing_info);
    hr = DwmGetCompositionTimingInfo(NULL, &timing_info);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
}

START_TEST(dwmapi)
{
    test_DwmIsCompositionEnabled();
    test_DwmGetCompositionTimingInfo();
}
