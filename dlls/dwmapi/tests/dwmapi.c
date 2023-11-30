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
    LARGE_INTEGER performance_frequency;
    int result, display_frequency;
    DWM_TIMING_INFO timing_info;
    QPC_TIME refresh_period;
    DEVMODEA mode;
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

    memset(&mode, 0, sizeof(mode));
    mode.dmSize = sizeof(mode);
    result = EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &mode);
    ok(!!result, "Failed to get display mode %#lx.\n", GetLastError());
    display_frequency = mode.dmDisplayFrequency;
    ok(!!QueryPerformanceFrequency(&performance_frequency), "Failed to get performance counter frequency.\n");
    refresh_period = performance_frequency.QuadPart / display_frequency;

    timing_info.cbSize = sizeof(timing_info);
    hr = DwmGetCompositionTimingInfo(NULL, &timing_info);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(timing_info.cbSize == sizeof(timing_info), "Got wrong struct size %d.\n", timing_info.cbSize);
    ok(timing_info.rateRefresh.uiDenominator == 1 && timing_info.rateRefresh.uiNumerator == display_frequency,
            "Got wrong monitor refresh rate %d/%d.\n", timing_info.rateRefresh.uiDenominator,
            timing_info.rateRefresh.uiNumerator);
    ok(timing_info.rateCompose.uiDenominator == 1 && timing_info.rateCompose.uiNumerator == display_frequency,
            "Got wrong composition rate %d/%d.\n", timing_info.rateCompose.uiDenominator,
            timing_info.rateCompose.uiNumerator);
    ok(timing_info.qpcRefreshPeriod == refresh_period
            || broken(timing_info.qpcRefreshPeriod == display_frequency), /* win10 v1507 */
            "Got wrong monitor refresh period %s.\n", wine_dbgstr_longlong(timing_info.qpcRefreshPeriod));
}

static void test_DWMWA_EXTENDED_FRAME_BOUNDS(void)
{
    DPI_AWARENESS_CONTEXT (WINAPI *pSetThreadDpiAwarenessContext)(DPI_AWARENESS_CONTEXT);
    DPI_AWARENESS_CONTEXT old_context = NULL;
    BOOL enabled;
    HRESULT hr;
    RECT rect, window_rect, intersection;
    HWND hwnd, child;

    pSetThreadDpiAwarenessContext = (void *)GetProcAddress(GetModuleHandleA("user32.dll"),
            "SetThreadDpiAwarenessContext");
    if (pSetThreadDpiAwarenessContext)
        old_context = pSetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);

    hwnd = CreateWindowW(L"static", L"static", WS_OVERLAPPEDWINDOW | WS_POPUP | WS_VISIBLE, 10, 10, 200, 200, NULL, NULL, NULL, NULL);
    child = CreateWindowW(L"edit", L"edit", WS_CHILD | WS_VISIBLE, 0, 0, 50, 50, hwnd, NULL, NULL, NULL);

    DwmIsCompositionEnabled(&enabled);
    hr = DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &rect, sizeof(rect));
    if (!enabled) {
        ok(hr == E_HANDLE, "Got hr %#lx.\n", hr);
        skip("DWM is disabled.\n");
        goto cleanup;
    }

    hr = DwmGetWindowAttribute(NULL, DWMWA_EXTENDED_FRAME_BOUNDS, &rect, sizeof(rect));
    ok(hr == E_HANDLE, "Got hr %#lx.\n", hr);
    hr = DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &enabled, sizeof(enabled));
    ok(hr == E_NOT_SUFFICIENT_BUFFER, "Got hr %#lx.\n", hr);
    hr = DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, NULL, 0);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    hr = DwmGetWindowAttribute(child, DWMWA_EXTENDED_FRAME_BOUNDS, &rect, sizeof(rect));
    ok(hr == E_HANDLE, "Got hr %#lx.\n", hr);
    hr = DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &rect, sizeof(rect));
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    /* Window rectangle covers extended frame */
    GetWindowRect(hwnd, &window_rect);
    IntersectRect(&intersection, &window_rect, &rect);
    ok(EqualRect(&intersection, &rect), "Got wrong frame %s, window %s.\n", wine_dbgstr_rect(&rect), wine_dbgstr_rect(&window_rect));

    /* Extended frame bounds aren't adjusted for DPI */
    if (pSetThreadDpiAwarenessContext) {
        RECT unaware_rect;
        pSetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_UNAWARE);
        hr = DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &unaware_rect, sizeof(unaware_rect));
        ok(hr == S_OK, "Got hr %#lx.\n", hr);
        ok(EqualRect(&rect, &unaware_rect), "Expected %s, got %s.\n", wine_dbgstr_rect(&rect), wine_dbgstr_rect(&unaware_rect));
    }

cleanup:
    if (pSetThreadDpiAwarenessContext)
        pSetThreadDpiAwarenessContext(old_context);
    DestroyWindow(child);
    DestroyWindow(hwnd);
}

START_TEST(dwmapi)
{
    test_DwmIsCompositionEnabled();
    test_DwmGetCompositionTimingInfo();
    test_DWMWA_EXTENDED_FRAME_BOUNDS();
}
