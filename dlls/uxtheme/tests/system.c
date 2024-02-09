/* Unit test suite for uxtheme API functions
 *
 * Copyright 2006 Paul Vriens
 * Copyright 2021-2022 Zhiyi Zhang for CodeWeavers
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

#include "ntstatus.h"
#define WIN32_NO_STATUS

#include "windows.h"
#include "winternl.h"
#include "ddk/d3dkmthk.h"
#include "vfwmsgs.h"
#include "uxtheme.h"
#include "vssym32.h"

#include "msg.h"
#include "wine/test.h"

#include "v6util.h"

static HTHEME  (WINAPI * pOpenThemeDataEx)(HWND, LPCWSTR, DWORD);
static HTHEME (WINAPI *pOpenThemeDataForDpi)(HWND, LPCWSTR, UINT);
static HRESULT (WINAPI *pOpenThemeFile)(const WCHAR *, const WCHAR *, const WCHAR *, HANDLE, DWORD);
static HRESULT (WINAPI *pCloseThemeFile)(HANDLE);
static HPAINTBUFFER (WINAPI *pBeginBufferedPaint)(HDC, const RECT *, BP_BUFFERFORMAT, BP_PAINTPARAMS *, HDC *);
static HRESULT (WINAPI *pBufferedPaintClear)(HPAINTBUFFER, const RECT *);
static HRESULT (WINAPI *pDrawThemeBackgroundEx)(HTHEME, HDC, int, int, const RECT *, const DTBGOPTS *);
static HRESULT (WINAPI *pEndBufferedPaint)(HPAINTBUFFER, BOOL);
static HRESULT (WINAPI *pGetBufferedPaintBits)(HPAINTBUFFER, RGBQUAD **, int *);
static HDC (WINAPI *pGetBufferedPaintDC)(HPAINTBUFFER);
static HDC (WINAPI *pGetBufferedPaintTargetDC)(HPAINTBUFFER);
static HRESULT (WINAPI *pGetBufferedPaintTargetRect)(HPAINTBUFFER, RECT *);
static HRESULT (WINAPI *pGetThemeIntList)(HTHEME, int, int, int, INTLIST *);
static HRESULT (WINAPI *pGetThemeTransitionDuration)(HTHEME, int, int, int, int, DWORD *);
static BOOLEAN (WINAPI *pShouldSystemUseDarkMode)(void);
static BOOLEAN (WINAPI *pShouldAppsUseDarkMode)(void);

static LONG (WINAPI *pDisplayConfigGetDeviceInfo)(DISPLAYCONFIG_DEVICE_INFO_HEADER *);
static LONG (WINAPI *pDisplayConfigSetDeviceInfo)(DISPLAYCONFIG_DEVICE_INFO_HEADER *);
static BOOL (WINAPI *pGetDpiForMonitorInternal)(HMONITOR, UINT, UINT *, UINT *);
static UINT (WINAPI *pGetDpiForSystem)(void);
static DPI_AWARENESS_CONTEXT (WINAPI *pSetThreadDpiAwarenessContext)(DPI_AWARENESS_CONTEXT);

static NTSTATUS (WINAPI *pD3DKMTCloseAdapter)(const D3DKMT_CLOSEADAPTER *);
static NTSTATUS (WINAPI *pD3DKMTOpenAdapterFromGdiDisplayName)(D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME *);

/* For message tests */
enum seq_index
{
    PARENT_SEQ_INDEX,
    CHILD_SEQ_INDEX,
    NUM_MSG_SEQUENCES
};

static struct msg_sequence *sequences[NUM_MSG_SEQUENCES];

static void init_funcs(void)
{
    HMODULE user32 = GetModuleHandleA("user32.dll");
    HMODULE gdi32 = GetModuleHandleA("gdi32.dll");
    HMODULE uxtheme = GetModuleHandleA("uxtheme.dll");

    pOpenThemeFile = (void *)GetProcAddress(uxtheme, MAKEINTRESOURCEA(2));
    pCloseThemeFile = (void *)GetProcAddress(uxtheme, MAKEINTRESOURCEA(3));
    pShouldSystemUseDarkMode = (void *)GetProcAddress(uxtheme, MAKEINTRESOURCEA(138));
    pShouldAppsUseDarkMode = (void *)GetProcAddress(uxtheme, MAKEINTRESOURCEA(132));

#define GET_PROC(module, func)                       \
    p##func = (void *)GetProcAddress(module, #func); \
    if (!p##func)                                    \
        trace("GetProcAddress(%s, %s) failed.\n", #module, #func);

    GET_PROC(uxtheme, BeginBufferedPaint)
    GET_PROC(uxtheme, BufferedPaintClear)
    GET_PROC(uxtheme, EndBufferedPaint)
    GET_PROC(uxtheme, DrawThemeBackgroundEx)
    GET_PROC(uxtheme, GetBufferedPaintBits)
    GET_PROC(uxtheme, GetBufferedPaintDC)
    GET_PROC(uxtheme, GetBufferedPaintTargetDC)
    GET_PROC(uxtheme, GetBufferedPaintTargetRect)
    GET_PROC(uxtheme, GetThemeIntList)
    GET_PROC(uxtheme, GetThemeTransitionDuration)
    GET_PROC(uxtheme, OpenThemeDataEx)
    GET_PROC(uxtheme, OpenThemeDataForDpi)

    GET_PROC(user32, DisplayConfigGetDeviceInfo)
    GET_PROC(user32, DisplayConfigSetDeviceInfo)
    GET_PROC(user32, GetDpiForMonitorInternal)
    GET_PROC(user32, GetDpiForSystem)
    GET_PROC(user32, SetThreadDpiAwarenessContext)

    GET_PROC(gdi32, D3DKMTCloseAdapter)
    GET_PROC(gdi32, D3DKMTOpenAdapterFromGdiDisplayName)

#undef GET_PROC
}

static BOOL compare_uint(unsigned int x, unsigned int y, unsigned int max_diff)
{
    unsigned int diff = x > y ? x - y : y - x;

    return diff <= max_diff;
}

/* Try to make sure pending X events have been processed before continuing */
static void flush_events(void)
{
    MSG msg;
    int diff = 200;
    int min_timeout = 100;
    DWORD time = GetTickCount() + diff;

    while (diff > 0)
    {
        if (MsgWaitForMultipleObjects(0, NULL, FALSE, min_timeout, QS_ALLINPUT) == WAIT_TIMEOUT)
            break;
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
            DispatchMessageA(&msg);
        diff = time - GetTickCount();
    }
}

static unsigned int get_primary_monitor_effective_dpi(void)
{
    DPI_AWARENESS_CONTEXT old_context;
    UINT dpi_x = 0, dpi_y = 0;
    POINT point = {0, 0};
    HMONITOR monitor;

    if (pSetThreadDpiAwarenessContext && pGetDpiForMonitorInternal)
    {
        old_context = pSetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
        monitor = MonitorFromPoint(point, MONITOR_DEFAULTTOPRIMARY);
        pGetDpiForMonitorInternal(monitor, 0, &dpi_x, &dpi_y);
        pSetThreadDpiAwarenessContext(old_context);
        return dpi_y;
    }

    return USER_DEFAULT_SCREEN_DPI;
}

static int get_dpi_scale_index(int dpi)
{
    static const int scales[] = {100, 125, 150, 175, 200, 225, 250, 300, 350, 400, 450, 500};
    int scale, scale_idx;

    scale = dpi * 100 / 96;
    for (scale_idx = 0; scale_idx < ARRAY_SIZE(scales); ++scale_idx)
    {
        if (scales[scale_idx] == scale)
            return scale_idx;
    }

    return -1;
}

static BOOL set_primary_monitor_effective_dpi(unsigned int primary_dpi)
{
    D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME open_adapter_gdi_desc;
    DISPLAYCONFIG_GET_SOURCE_DPI_SCALE get_scale_req;
    DISPLAYCONFIG_SET_SOURCE_DPI_SCALE set_scale_req;
    int current_scale_idx, target_scale_idx;
    D3DKMT_CLOSEADAPTER close_adapter_desc;
    BOOL ret = FALSE;
    LONG error;

#define CHECK_FUNC(func)                           \
    if (!p##func)                                  \
    {                                              \
        win_skip("%s() is unavailable.\n", #func); \
        ret = TRUE;                                \
    }

    CHECK_FUNC(D3DKMTCloseAdapter)
    CHECK_FUNC(D3DKMTOpenAdapterFromGdiDisplayName)
    CHECK_FUNC(DisplayConfigGetDeviceInfo)
    todo_wine CHECK_FUNC(DisplayConfigSetDeviceInfo)
    if (ret) return FALSE;

#undef CHECK_FUNC

    lstrcpyW(open_adapter_gdi_desc.DeviceName, L"\\\\.\\DISPLAY1");
    if (pD3DKMTOpenAdapterFromGdiDisplayName(&open_adapter_gdi_desc) == STATUS_PROCEDURE_NOT_FOUND)
    {
        win_skip("D3DKMTOpenAdapterFromGdiDisplayName() is unavailable.\n");
        return FALSE;
    }

    get_scale_req.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_DPI_SCALE;
    get_scale_req.header.size = sizeof(get_scale_req);
    get_scale_req.header.adapterId = open_adapter_gdi_desc.AdapterLuid;
    get_scale_req.header.id = open_adapter_gdi_desc.VidPnSourceId;
    error = pDisplayConfigGetDeviceInfo(&get_scale_req.header);
    if (error != NO_ERROR)
    {
        skip("DisplayConfigGetDeviceInfo failed, returned %ld.\n", error);
        goto failed;
    }

    current_scale_idx = get_dpi_scale_index(get_primary_monitor_effective_dpi());
    if (current_scale_idx == -1)
    {
        skip("Failed to find current scale index.\n");
        goto failed;
    }

    target_scale_idx = get_dpi_scale_index(primary_dpi);
    if (target_scale_idx < get_scale_req.minRelativeScaleStep
        || target_scale_idx > get_scale_req.maxRelativeScaleStep)
    {
        skip("DPI %d is not available.\n", primary_dpi);
        goto failed;
    }

    set_scale_req.header.type = DISPLAYCONFIG_DEVICE_INFO_SET_SOURCE_DPI_SCALE;
    set_scale_req.header.size = sizeof(set_scale_req);
    set_scale_req.header.adapterId = open_adapter_gdi_desc.AdapterLuid;
    set_scale_req.header.id = open_adapter_gdi_desc.VidPnSourceId;
    set_scale_req.relativeScaleStep = get_scale_req.curRelativeScaleStep + (target_scale_idx - current_scale_idx);
    error = pDisplayConfigSetDeviceInfo(&set_scale_req.header);
    if (error == NO_ERROR)
        ret = get_primary_monitor_effective_dpi() == primary_dpi;
    flush_events();

failed:
    close_adapter_desc.hAdapter = open_adapter_gdi_desc.hAdapter;
    pD3DKMTCloseAdapter(&close_adapter_desc);
    return ret;
}

static LRESULT WINAPI TestSetWindowThemeParentProcA(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static LONG defwndproc_counter;
    struct message msg = {0};
    LRESULT ret;

    /* Only care about WM_THEMECHANGED */
    if (message != WM_THEMECHANGED)
        return DefWindowProcA(hwnd, message, wParam, lParam);

    msg.message = message;
    msg.flags = sent | wparam | lparam;
    if (defwndproc_counter)
        msg.flags |= defwinproc;
    msg.wParam = wParam;
    msg.lParam = lParam;
    add_message(sequences, PARENT_SEQ_INDEX, &msg);

    InterlockedIncrement(&defwndproc_counter);
    ret = DefWindowProcA(hwnd, message, wParam, lParam);
    InterlockedDecrement(&defwndproc_counter);
    return ret;
}

static LRESULT WINAPI TestSetWindowThemeChildProcA(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static LONG defwndproc_counter;
    struct message msg = {0};
    LRESULT ret;

    /* Only care about WM_THEMECHANGED */
    if (message != WM_THEMECHANGED)
        return DefWindowProcA(hwnd, message, wParam, lParam);

    msg.message = message;
    msg.flags = sent | wparam | lparam;
    if (defwndproc_counter)
        msg.flags |= defwinproc;
    msg.wParam = wParam;
    msg.lParam = lParam;
    add_message(sequences, CHILD_SEQ_INDEX, &msg);

    InterlockedIncrement(&defwndproc_counter);
    ret = DefWindowProcA(hwnd, message, wParam, lParam);
    InterlockedDecrement(&defwndproc_counter);
    return ret;
}

static void test_IsThemed(void)
{
    BOOL bThemeActive;
    BOOL bAppThemed;

    bThemeActive = IsThemeActive();
    trace("Theming is %s\n", (bThemeActive) ? "active" : "inactive");

    bAppThemed = IsAppThemed();
    trace("Test executable is %s\n", (bAppThemed) ? "themed" : "not themed");
}

static void test_IsThemePartDefined(void)
{
    BOOL is_theme_active, ret;
    HTHEME theme = NULL;
    DWORD error;
    HWND hwnd;
    int i;

    static const struct
    {
        const WCHAR *class_name;
        int part;
        int state;
        BOOL defined;
        DWORD error;
    }
    tests[] =
    {
        {NULL, 0, 0, FALSE, E_HANDLE},
        {NULL, 0xdeadbeef, 0, FALSE, E_HANDLE},
        {L"Button", 0xdeadbeef, 0, FALSE, NO_ERROR},
        {L"Button", BP_PUSHBUTTON, 0, TRUE, NO_ERROR},
        {L"Button", BP_PUSHBUTTON, PBS_NORMAL, FALSE, NO_ERROR},
        {L"Button", BP_PUSHBUTTON, PBS_DEFAULTED_ANIMATING, FALSE, NO_ERROR},
        {L"Button", BP_PUSHBUTTON, PBS_DEFAULTED_ANIMATING + 1, FALSE, NO_ERROR},
        {L"Edit", EP_EDITTEXT, 0, TRUE, NO_ERROR},
        {L"Edit", EP_EDITTEXT, ETS_NORMAL, FALSE, NO_ERROR},
        {L"Edit", EP_EDITTEXT, ETS_FOCUSED, FALSE, NO_ERROR},
    };

    is_theme_active = IsThemeActive();
    hwnd = CreateWindowA(WC_STATICA, "", WS_POPUP, 0, 0, 1, 1, 0, 0, 0, NULL);
    ok(hwnd != NULL, "CreateWindowA failed, error %#lx.\n", GetLastError());

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        if (!is_theme_active && tests[i].class_name)
            continue;

        winetest_push_context("class %s part %d state %d", wine_dbgstr_w(tests[i].class_name),
                              tests[i].part, tests[i].state);

        if (tests[i].class_name)
        {
            theme = OpenThemeData(hwnd, tests[i].class_name);
            ok(theme != NULL, "OpenThemeData failed, error %#lx.\n", GetLastError());
        }

        SetLastError(0xdeadbeef);
        ret = IsThemePartDefined(theme, tests[i].part, tests[i].state);
        error = GetLastError();
        ok(ret == tests[i].defined, "Expected %d.\n", tests[i].defined);
        ok(error == tests[i].error, "Expected %#lx, got %#lx.\n", tests[i].error, error);

        if (theme)
        {
            CloseThemeData(theme);
            theme = NULL;
        }
        winetest_pop_context();
    }

    DestroyWindow(hwnd);
}

static void test_GetWindowTheme(void)
{
    HTHEME    hTheme;
    HWND      hWnd;

    SetLastError(0xdeadbeef);
    hTheme = GetWindowTheme(NULL);
    ok( hTheme == NULL, "Expected a NULL return, got %p\n", hTheme);
    ok( GetLastError() == E_HANDLE, "Expected E_HANDLE, got 0x%08lx\n", GetLastError() );

    /* Only do the bare minimum to get a valid hwnd */
    hWnd = CreateWindowExA(0, "static", "", WS_POPUP, 0,0,100,100,0, 0, 0, NULL);
    ok(hWnd != NULL, "Failed to create a test window.\n");

    SetLastError(0xdeadbeef);
    hTheme = GetWindowTheme(hWnd);
    ok( hTheme == NULL, "Expected a NULL return, got %p\n", hTheme);
    ok( GetLastError() == 0xdeadbeef,
        "Expected 0xdeadbeef, got 0x%08lx\n",
        GetLastError());

    DestroyWindow(hWnd);
}

static const struct message SetWindowThemeSeq[] =
{
    {WM_THEMECHANGED, sent},
    {0}
};

static const struct message EmptySeq[] =
{
    {0}
};

static void test_SetWindowTheme(void)
{
    WNDCLASSA cls = {0};
    HWND hWnd, child;
    HTHEME hTheme;
    HRESULT hRes;
    MSG msg;

    hRes = SetWindowTheme(NULL, NULL, NULL);
    ok( hRes == E_HANDLE, "Expected E_HANDLE, got 0x%08lx\n", hRes);

    /* Test that WM_THEMECHANGED is sent and not posted to windows */
    cls.hInstance = GetModuleHandleA(0);
    cls.hCursor = LoadCursorA(0, (LPCSTR)IDC_ARROW);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpfnWndProc = TestSetWindowThemeParentProcA;
    cls.lpszClassName = "TestSetWindowThemeParentClass";
    RegisterClassA(&cls);

    cls.lpfnWndProc = TestSetWindowThemeChildProcA;
    cls.lpszClassName = "TestSetWindowThemeChildClass";
    RegisterClassA(&cls);

    hWnd = CreateWindowA("TestSetWindowThemeParentClass", "parent",
                         WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100, 200, 200, 0, 0, 0, NULL);
    ok(!!hWnd, "Failed to create a parent window.\n");
    child = CreateWindowA("TestSetWindowThemeChildClass", "child", WS_CHILD | WS_VISIBLE, 0, 0, 50,
                          50, hWnd, 0, 0, NULL);
    ok(!!child, "Failed to create a child window.\n");
    flush_events();
    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    hRes = SetWindowTheme(hWnd, NULL, NULL);
    ok(hRes == S_OK, "Expected %#lx, got %#lx.\n", S_OK, hRes);
    while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
    {
        struct message recv_msg = {0};

        if (msg.message == WM_THEMECHANGED)
        {
            recv_msg.message = msg.message;
            recv_msg.flags = posted | wparam | lparam;
            recv_msg.wParam = msg.wParam;
            recv_msg.lParam = msg.lParam;
            add_message(sequences, msg.hwnd == hWnd ? PARENT_SEQ_INDEX : CHILD_SEQ_INDEX, &recv_msg);
        }
        DispatchMessageA(&msg);
    }
    ok_sequence(sequences, PARENT_SEQ_INDEX, SetWindowThemeSeq, "SetWindowTheme parent", FALSE);
    ok_sequence(sequences, CHILD_SEQ_INDEX, EmptySeq, "SetWindowTheme child", FALSE);
    DestroyWindow(hWnd);
    UnregisterClassA("TestSetWindowThemeParentClass", GetModuleHandleA(0));
    UnregisterClassA("TestSetWindowThemeChildClass", GetModuleHandleA(0));

    /* Only do the bare minimum to get a valid hwnd */
    hWnd = CreateWindowExA(0, "button", "test", WS_POPUP, 0, 0, 100, 100, 0, 0, 0, NULL);
    ok(hWnd != NULL, "Failed to create a test window.\n");

    hRes = SetWindowTheme(hWnd, NULL, NULL);
    ok( hRes == S_OK, "Expected S_OK, got 0x%08lx\n", hRes);

    if (IsThemeActive())
    {
        hTheme = OpenThemeData(hWnd, L"Button");
        ok(!!hTheme, "OpenThemeData failed.\n");
        CloseThemeData(hTheme);

        hRes = SetWindowTheme(hWnd, L"deadbeef", NULL);
        ok(hRes == S_OK, "Expected S_OK, got 0x%08lx.\n", hRes);

        hTheme = OpenThemeData(hWnd, L"Button");
        ok(!!hTheme, "OpenThemeData failed.\n");
        CloseThemeData(hTheme);
    }
    else
    {
        skip("No active theme, skipping rest of SetWindowTheme tests.\n");
    }

    DestroyWindow(hWnd);
}

static void test_OpenThemeData(void)
{
    HTHEME    hTheme, hTheme2;
    HWND      hWnd;
    BOOL      bThemeActive;
    HRESULT   hRes;
    BOOL      bTPDefined;

    const WCHAR szInvalidClassList[] = L"DEADBEEF";
    const WCHAR szButtonClassList[]  = L"Button";
    const WCHAR szButtonClassList2[] = L"bUtToN";
    const WCHAR szClassList[]        = L"Button;ListBox";

    bThemeActive = IsThemeActive();

    /* All NULL */
    SetLastError(0xdeadbeef);
    hTheme = OpenThemeData(NULL, NULL);
    ok( hTheme == NULL, "Expected a NULL return, got %p\n", hTheme);
    ok( GetLastError() == E_POINTER,
            "Expected GLE() to be E_POINTER, got 0x%08lx\n",
            GetLastError());

    /* A NULL hWnd and an invalid classlist */
    SetLastError(0xdeadbeef);
    hTheme = OpenThemeData(NULL, szInvalidClassList);
    ok( hTheme == NULL, "Expected a NULL return, got %p\n", hTheme);
    ok( GetLastError() == E_PROP_ID_UNSUPPORTED, "Expected 0x%08lx, got 0x%08lx\n",
        E_PROP_ID_UNSUPPORTED, GetLastError() );

    SetLastError(0xdeadbeef);
    hTheme = OpenThemeData(NULL, szClassList);
    if (bThemeActive)
    {
        ok( hTheme != NULL, "got NULL, expected a HTHEME handle\n");
        ok( GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got 0x%08lx\n", GetLastError() );
    }
    else
    {
        ok( hTheme == NULL, "Expected a NULL return, got %p\n", hTheme);
        ok( GetLastError() == E_PROP_ID_UNSUPPORTED, "Expected 0x%08lx, got 0x%08lx\n",
            E_PROP_ID_UNSUPPORTED, GetLastError() );
    }

    /* Only do the bare minimum to get a valid hdc */
    hWnd = CreateWindowExA(0, "static", "", WS_POPUP, 0,0,100,100,0, 0, 0, NULL);
    if (!hWnd) return;

    SetLastError(0xdeadbeef);
    hTheme = OpenThemeData(hWnd, NULL);
    ok( hTheme == NULL, "Expected a NULL return, got %p\n", hTheme);
    ok( GetLastError() == E_POINTER,
            "Expected GLE() to be E_POINTER, got 0x%08lx\n",
            GetLastError());

    SetLastError(0xdeadbeef);
    hTheme = OpenThemeData(hWnd, szInvalidClassList);
    ok( hTheme == NULL, "Expected a NULL return, got %p\n", hTheme);
    ok( GetLastError() == E_PROP_ID_UNSUPPORTED, "Expected 0x%08lx, got 0x%08lx\n",
        E_PROP_ID_UNSUPPORTED, GetLastError() );

    if (!bThemeActive)
    {
        SetLastError(0xdeadbeef);
        hTheme = OpenThemeData(hWnd, szButtonClassList);
        ok( hTheme == NULL, "Expected a NULL return, got %p\n", hTheme);
        ok( GetLastError() == E_PROP_ID_UNSUPPORTED, "Expected 0x%08lx, got 0x%08lx\n",
            E_PROP_ID_UNSUPPORTED, GetLastError() );
        skip("No active theme, skipping rest of OpenThemeData tests\n");
        return;
    }

    /* Only do the next checks if we have an active theme */

    hRes = SetWindowTheme(hWnd, L"explorer", NULL);
    ok(hRes == S_OK, "Got unexpected hr %#lx.\n", hRes);
    SetLastError(0xdeadbeef);
    hTheme = OpenThemeData(hWnd, L"explorer::treeview");
    ok(!hTheme, "OpenThemeData() should fail\n");
    ok(GetLastError() == E_PROP_ID_UNSUPPORTED, "Got unexpected %#lx.\n", GetLastError());
    SetWindowTheme(hWnd, NULL, NULL);

    SetLastError(0xdeadbeef);
    hTheme = OpenThemeData(hWnd, L"dead::beef;explorer::treeview");
    todo_wine
    ok(!hTheme, "OpenThemeData() should fail\n");
    todo_wine
    ok(GetLastError() == E_PROP_ID_UNSUPPORTED, "Got unexpected %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    hTheme = OpenThemeData(hWnd, L"explorer::treeview");
    ok(hTheme != NULL, "OpenThemeData() failed\n");
    ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got 0x%08lx\n", GetLastError());
    CloseThemeData(hTheme);

    SetLastError(0xdeadbeef);
    hTheme = OpenThemeData(hWnd, L"deadbeef::treeview;dead::beef");
    ok(hTheme != NULL, "OpenThemeData() failed\n");
    ok(GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got 0x%08lx\n", GetLastError());
    CloseThemeData(hTheme);

    SetLastError(0xdeadbeef);
    hTheme = OpenThemeData(hWnd, szButtonClassList);
    ok( hTheme != NULL, "got NULL, expected a HTHEME handle\n");
    ok( GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got 0x%08lx\n", GetLastError() );
    CloseThemeData(hTheme);

    /* Test with bUtToN instead of Button */
    SetLastError(0xdeadbeef);
    hTheme = OpenThemeData(hWnd, szButtonClassList2);
    ok( hTheme != NULL, "got NULL, expected a HTHEME handle\n");
    ok( GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got 0x%08lx\n", GetLastError() );
    CloseThemeData(hTheme);

    SetLastError(0xdeadbeef);
    hTheme = OpenThemeData(hWnd, szClassList);
    ok( hTheme != NULL, "got NULL, expected a HTHEME handle\n");
    ok( GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got 0x%08lx\n", GetLastError() );

    /* GetWindowTheme should return the last handle opened by OpenThemeData */
    SetLastError(0xdeadbeef);
    hTheme2 = GetWindowTheme(hWnd);
    ok( hTheme == hTheme2, "Expected the same HTHEME handle (%p<->%p)\n",
        hTheme, hTheme2);
    ok( GetLastError() == 0xdeadbeef,
        "Expected 0xdeadbeef, got 0x%08lx\n",
        GetLastError());

    SetLastError(0xdeadbeef);
    bTPDefined = IsThemePartDefined(hTheme, 0, 0);
    todo_wine
    ok( bTPDefined == FALSE, "Expected FALSE\n" );
    ok( GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got 0x%08lx\n", GetLastError() );

    hRes = CloseThemeData(hTheme);
    ok( hRes == S_OK, "Expected S_OK, got 0x%08lx\n", hRes);

    /* Close a second time */
    hRes = CloseThemeData(hTheme);
    ok( hRes == S_OK, "Expected S_OK, got 0x%08lx\n", hRes);

    /* See if closing makes a difference for GetWindowTheme */
    SetLastError(0xdeadbeef);
    hTheme2 = NULL;
    hTheme2 = GetWindowTheme(hWnd);
    ok( hTheme == hTheme2, "Expected the same HTHEME handle (%p<->%p)\n",
        hTheme, hTheme2);
    ok( GetLastError() == 0xdeadbeef,
        "Expected 0xdeadbeef, got 0x%08lx\n",
        GetLastError());

    DestroyWindow(hWnd);
}

static void test_OpenThemeDataEx(void)
{
    HTHEME    hTheme;
    HWND      hWnd;
    BOOL      bThemeActive;

    const WCHAR szInvalidClassList[] = L"DEADBEEF";
    const WCHAR szButtonClassList[]  = L"Button";
    const WCHAR szButtonClassList2[] = L"bUtToN";
    const WCHAR szClassList[]        = L"Button;ListBox";

    if (!pOpenThemeDataEx)
    {
        win_skip("OpenThemeDataEx not available\n");
        return;
    }

    bThemeActive = IsThemeActive();

    /* All NULL */
    SetLastError(0xdeadbeef);
    hTheme = pOpenThemeDataEx(NULL, NULL, 0);
    ok( hTheme == NULL, "Expected a NULL return, got %p\n", hTheme);
    ok( GetLastError() == E_POINTER,
            "Expected GLE() to be E_POINTER, got 0x%08lx\n",
            GetLastError());

    /* A NULL hWnd and an invalid classlist without flags */
    SetLastError(0xdeadbeef);
    hTheme = pOpenThemeDataEx(NULL, szInvalidClassList, 0);
    ok( hTheme == NULL, "Expected a NULL return, got %p\n", hTheme);
    ok( GetLastError() == E_PROP_ID_UNSUPPORTED, "Expected 0x%08lx, got 0x%08lx\n",
        E_PROP_ID_UNSUPPORTED, GetLastError() );

    SetLastError(0xdeadbeef);
    hTheme = pOpenThemeDataEx(NULL, szClassList, 0);
    if (bThemeActive)
    {
        ok( hTheme != NULL, "got NULL, expected a HTHEME handle\n");
        ok( GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got 0x%08lx\n", GetLastError() );
    }
    else
    {
        ok( hTheme == NULL, "Expected a NULL return, got %p\n", hTheme);
        ok( GetLastError() == E_PROP_ID_UNSUPPORTED, "Expected 0x%08lx, got 0x%08lx\n",
            E_PROP_ID_UNSUPPORTED, GetLastError() );
    }

    /* Only do the bare minimum to get a valid hdc */
    hWnd = CreateWindowExA(0, "static", "", WS_POPUP, 0,0,100,100,0, 0, 0, NULL);
    if (!hWnd) return;

    SetLastError(0xdeadbeef);
    hTheme = pOpenThemeDataEx(hWnd, NULL, 0);
    ok( hTheme == NULL, "Expected a NULL return, got %p\n", hTheme);
    ok( GetLastError() == E_POINTER,
            "Expected GLE() to be E_POINTER, got 0x%08lx\n",
            GetLastError());

    SetLastError(0xdeadbeef);
    hTheme = pOpenThemeDataEx(hWnd, szInvalidClassList, 0);
    ok( hTheme == NULL, "Expected a NULL return, got %p\n", hTheme);
    ok( GetLastError() == E_PROP_ID_UNSUPPORTED, "Expected 0x%08lx, got 0x%08lx\n",
        E_PROP_ID_UNSUPPORTED, GetLastError() );

    if (!bThemeActive)
    {
        SetLastError(0xdeadbeef);
        hTheme = pOpenThemeDataEx(hWnd, szButtonClassList, 0);
        ok( hTheme == NULL, "Expected a NULL return, got %p\n", hTheme);
        ok( GetLastError() == E_PROP_ID_UNSUPPORTED, "Expected 0x%08lx, got 0x%08lx\n",
            E_PROP_ID_UNSUPPORTED, GetLastError() );
        skip("No active theme, skipping rest of OpenThemeDataEx tests\n");
        return;
    }

    /* Only do the next checks if we have an active theme */

    SetLastError(0xdeadbeef);
    hTheme = pOpenThemeDataEx(hWnd, szButtonClassList, 0);
    ok( hTheme != NULL, "got NULL, expected a HTHEME handle\n");
    ok( GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got 0x%08lx\n", GetLastError() );

    SetLastError(0xdeadbeef);
    hTheme = pOpenThemeDataEx(hWnd, szButtonClassList, OTD_FORCE_RECT_SIZING);
    ok( hTheme != NULL, "got NULL, expected a HTHEME handle\n");
    ok( GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got 0x%08lx\n", GetLastError() );

    SetLastError(0xdeadbeef);
    hTheme = pOpenThemeDataEx(hWnd, szButtonClassList, OTD_NONCLIENT);
    ok( hTheme != NULL, "got NULL, expected a HTHEME handle\n");
    ok( GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got 0x%08lx\n", GetLastError() );

    SetLastError(0xdeadbeef);
    hTheme = pOpenThemeDataEx(hWnd, szButtonClassList, 0x3);
    ok( hTheme != NULL, "got NULL, expected a HTHEME handle\n");
    ok( GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got 0x%08lx\n", GetLastError() );

    /* Test with bUtToN instead of Button */
    SetLastError(0xdeadbeef);
    hTheme = pOpenThemeDataEx(hWnd, szButtonClassList2, 0);
    ok( hTheme != NULL, "got NULL, expected a HTHEME handle\n");
    ok( GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got 0x%08lx\n", GetLastError() );

    SetLastError(0xdeadbeef);
    hTheme = pOpenThemeDataEx(hWnd, szClassList, 0);
    ok( hTheme != NULL, "got NULL, expected a HTHEME handle\n");
    ok( GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got 0x%08lx\n", GetLastError() );

    DestroyWindow(hWnd);
}

static void test_OpenThemeDataForDpi(void)
{
    BOOL is_theme_active;
    HTHEME htheme;

    if (!pOpenThemeDataForDpi)
    {
        win_skip("OpenThemeDataForDpi is unavailable.\n");
        return;
    }

    is_theme_active = IsThemeActive();
    SetLastError(0xdeadbeef);
    htheme = pOpenThemeDataForDpi(NULL, WC_BUTTONW, 96);
    if (is_theme_active)
    {
        ok(!!htheme, "Got a NULL handle.\n");
        ok(GetLastError() == NO_ERROR, "Expected error %u, got %lu.\n", NO_ERROR, GetLastError());
        CloseThemeData(htheme);
    }
    else
    {
        ok(!htheme, "Got a non-NULL handle.\n");
        ok(GetLastError() == E_PROP_ID_UNSUPPORTED, "Expected error %lu, got %lu.\n",
           E_PROP_ID_UNSUPPORTED, GetLastError());
    }
}

static void test_GetCurrentThemeName(void)
{
    BOOL    bThemeActive;
    HRESULT hRes;
    WCHAR currentTheme[MAX_PATH];
    WCHAR currentColor[MAX_PATH];
    WCHAR currentSize[MAX_PATH];

    bThemeActive = IsThemeActive();

    /* All NULLs */
    hRes = GetCurrentThemeName(NULL, 0, NULL, 0, NULL, 0);
    if (bThemeActive)
        ok( hRes == S_OK, "Expected S_OK, got 0x%08lx\n", hRes);
    else
        ok( hRes == E_PROP_ID_UNSUPPORTED, "Expected E_PROP_ID_UNSUPPORTED, got 0x%08lx\n", hRes);

    /* Number of characters given is 0 */
    hRes = GetCurrentThemeName(currentTheme, 0, NULL, 0, NULL, 0);
    if (bThemeActive)
        ok( hRes == S_OK || broken(hRes == E_FAIL /* WinXP SP1 */), "Expected S_OK, got 0x%08lx\n", hRes);
    else
        ok( hRes == E_PROP_ID_UNSUPPORTED, "Expected E_PROP_ID_UNSUPPORTED, got 0x%08lx\n", hRes);

    hRes = GetCurrentThemeName(currentTheme, 2, NULL, 0, NULL, 0);
    if (bThemeActive)
        todo_wine
            ok(hRes == E_NOT_SUFFICIENT_BUFFER ||
               broken(hRes == E_FAIL /* WinXP SP1 */),
               "Expected E_NOT_SUFFICIENT_BUFFER, got 0x%08lx\n", hRes);
    else
        ok( hRes == E_PROP_ID_UNSUPPORTED, "Expected E_PROP_ID_UNSUPPORTED, got 0x%08lx\n", hRes);

    /* The same is true if the number of characters is too small for Color and/or Size */
    hRes = GetCurrentThemeName(currentTheme, ARRAY_SIZE(currentTheme), currentColor, 2,
                               currentSize,  ARRAY_SIZE(currentSize));
    if (bThemeActive)
        todo_wine
            ok(hRes == E_NOT_SUFFICIENT_BUFFER ||
               broken(hRes == E_FAIL /* WinXP SP1 */),
               "Expected E_NOT_SUFFICIENT_BUFFER, got 0x%08lx\n", hRes);
    else
        ok( hRes == E_PROP_ID_UNSUPPORTED, "Expected E_PROP_ID_UNSUPPORTED, got 0x%08lx\n", hRes);

    /* Given number of characters is correct */
    hRes = GetCurrentThemeName(currentTheme, ARRAY_SIZE(currentTheme), NULL, 0, NULL, 0);
    if (bThemeActive)
        ok( hRes == S_OK, "Expected S_OK, got 0x%08lx\n", hRes);
    else
        ok( hRes == E_PROP_ID_UNSUPPORTED, "Expected E_PROP_ID_UNSUPPORTED, got 0x%08lx\n", hRes);

    /* Given number of characters for the theme name is too large */
    hRes = GetCurrentThemeName(currentTheme, sizeof(currentTheme), NULL, 0, NULL, 0);
    if (bThemeActive)
        ok( hRes == E_POINTER || hRes == S_OK, "Expected E_POINTER or S_OK, got 0x%08lx\n", hRes);
    else
        ok( hRes == E_PROP_ID_UNSUPPORTED ||
            hRes == E_POINTER, /* win2k3 */
            "Expected E_PROP_ID_UNSUPPORTED, got 0x%08lx\n", hRes);
 
    /* The too large case is only for the theme name, not for color name or size name */
    hRes = GetCurrentThemeName(currentTheme, ARRAY_SIZE(currentTheme), currentColor,
                               sizeof(currentTheme), currentSize,  ARRAY_SIZE(currentSize));
    if (bThemeActive)
        ok( hRes == S_OK, "Expected S_OK, got 0x%08lx\n", hRes);
    else
        ok( hRes == E_PROP_ID_UNSUPPORTED, "Expected E_PROP_ID_UNSUPPORTED, got 0x%08lx\n", hRes);

    hRes = GetCurrentThemeName(currentTheme, ARRAY_SIZE(currentTheme), currentColor,
                               ARRAY_SIZE(currentTheme), currentSize,  sizeof(currentSize));
    if (bThemeActive)
        ok( hRes == S_OK, "Expected S_OK, got 0x%08lx\n", hRes);
    else
        ok( hRes == E_PROP_ID_UNSUPPORTED, "Expected E_PROP_ID_UNSUPPORTED, got 0x%08lx\n", hRes);

    /* Correct call */
    hRes = GetCurrentThemeName(currentTheme, ARRAY_SIZE(currentTheme), currentColor,
                               ARRAY_SIZE(currentColor), currentSize,  ARRAY_SIZE(currentSize));
    if (bThemeActive)
        ok( hRes == S_OK, "Expected S_OK, got 0x%08lx\n", hRes);
    else
        ok( hRes == E_PROP_ID_UNSUPPORTED, "Expected E_PROP_ID_UNSUPPORTED, got 0x%08lx\n", hRes);
}

static void test_CloseThemeData(void)
{
    HRESULT hRes;

    hRes = CloseThemeData(NULL);
    ok( hRes == E_HANDLE, "Expected E_HANDLE, got 0x%08lx\n", hRes);
    hRes = CloseThemeData(INVALID_HANDLE_VALUE);
    ok( hRes == E_HANDLE, "Expected E_HANDLE, got 0x%08lx\n", hRes);
    hRes = CloseThemeData((HTHEME)0xdeadbeef);
    ok(hRes == E_HANDLE, "Expected E_HANDLE, got 0x%08lx\n", hRes);
}

static void test_OpenThemeFile(void)
{
    WCHAR currentThemePath[MAX_PATH];
    DWORD pathSize = sizeof(currentThemePath);
    HANDLE htf;
    LSTATUS ls;
    HRESULT hr;
    SIZE partSize;

    if (!pOpenThemeFile)
    {
        win_skip("OpenThemeFile is unavailable.\n");
        return;
    }

    ls = RegGetValueW(HKEY_CURRENT_USER,
                      L"Software\\Microsoft\\Windows\\CurrentVersion\\ThemeManager", L"DllName",
                      RRF_RT_REG_SZ, NULL, currentThemePath, &pathSize);
    if (ls == ERROR_FILE_NOT_FOUND)
    {
        win_skip("DllName registry value not found.\n");
        return;
    }
    ok(ls == ERROR_SUCCESS, "RegGetValueW failed: %ld\n", ls);

    htf = (void *)0xdeadbeef;
    hr = pOpenThemeFile(NULL, NULL, NULL, &htf, 0);
    todo_wine ok(hr == E_POINTER, "Expected E_POINTER, got 0x%08lx\n", hr);
    ok(!htf, "Expected NULL, got %p\n", htf);

    htf = (void *)0xdeadbeef;
    hr = pOpenThemeFile(currentThemePath, NULL, NULL, &htf, 0);
    ok(hr == S_OK, "Expected S_OK, got 0x%08lx\n", hr);
    ok(htf != (void *)0xdeadbeef && htf != NULL && htf != INVALID_HANDLE_VALUE, "got %p\n", htf);

    hr = GetThemePartSize(htf, NULL, BP_CHECKBOX, CBS_CHECKEDNORMAL, NULL, TS_DRAW, &partSize);
    todo_wine ok(hr == E_HANDLE, "Expected E_HANDLE, got 0x%08lx\n", hr);

    hr = pCloseThemeFile(htf);
    ok(hr == S_OK, "Expected S_OK, got 0x%08lx\n", hr);
}

static void test_buffer_dc_props(HDC hdc, const RECT *rect)
{
    static const XFORM ident = { 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f };
    XFORM xform;
    POINT org;
    RECT box;
    BOOL ret;

    ret = GetWorldTransform(hdc, &xform);
    ok(ret, "Failed to get world transform\n");
    ok(!memcmp(&xform, &ident, sizeof(xform)), "Unexpected world transform\n");

    ret = GetViewportOrgEx(hdc, &org);
    ok(ret, "Failed to get vport origin\n");
    ok(org.x == 0 && org.y == 0, "Unexpected vport origin\n");

    ret = GetWindowOrgEx(hdc, &org);
    ok(ret, "Failed to get vport origin\n");
    ok(org.x == rect->left && org.y == rect->top, "Unexpected window origin\n");

    ret = GetClipBox(hdc, &box);
    ok(ret, "Failed to get clip box\n");
    ok(box.left == rect->left && box.top == rect->top, "Unexpected clip box\n");

    ok(GetGraphicsMode(hdc) == GM_COMPATIBLE, "wrong graphics mode\n");
}

static void test_buffered_paint(void)
{
    HDC target, src, hdc, screen_dc;
    BP_PAINTPARAMS params = { 0 };
    BP_BUFFERFORMAT format;
    HPAINTBUFFER buffer;
    RECT rect, rect2;
    RGBQUAD *bits;
    HBITMAP hbm;
    HRESULT hr;
    int row;

    if (!pBeginBufferedPaint)
    {
        win_skip("Buffered painting API is not supported.\n");
        return;
    }

    buffer = pBeginBufferedPaint(NULL, NULL, BPBF_COMPATIBLEBITMAP,
            NULL, NULL);
    ok(buffer == NULL, "Unexpected buffer %p\n", buffer);

    target = CreateCompatibleDC(0);
    buffer = pBeginBufferedPaint(target, NULL, BPBF_COMPATIBLEBITMAP,
            NULL, NULL);
    ok(buffer == NULL, "Unexpected buffer %p\n", buffer);

    params.cbSize = sizeof(params);
    buffer = pBeginBufferedPaint(target, NULL, BPBF_COMPATIBLEBITMAP,
            &params, NULL);
    ok(buffer == NULL, "Unexpected buffer %p\n", buffer);

    src = (void *)0xdeadbeef;
    buffer = pBeginBufferedPaint(target, NULL, BPBF_COMPATIBLEBITMAP,
            &params, &src);
    ok(buffer == NULL, "Unexpected buffer %p\n", buffer);
    ok(src == NULL, "Unexpected buffered dc %p\n", src);

    /* target rect is mandatory */
    SetRectEmpty(&rect);
    src = (void *)0xdeadbeef;
    buffer = pBeginBufferedPaint(target, &rect, BPBF_COMPATIBLEBITMAP,
            &params, &src);
    ok(buffer == NULL, "Unexpected buffer %p\n", buffer);
    ok(src == NULL, "Unexpected buffered dc %p\n", src);

    /* inverted rectangle */
    SetRect(&rect, 10, 0, 5, 5);
    src = (void *)0xdeadbeef;
    buffer = pBeginBufferedPaint(target, &rect, BPBF_COMPATIBLEBITMAP,
            &params, &src);
    ok(buffer == NULL, "Unexpected buffer %p\n", buffer);
    ok(src == NULL, "Unexpected buffered dc %p\n", src);

    SetRect(&rect, 0, 10, 5, 0);
    src = (void *)0xdeadbeef;
    buffer = pBeginBufferedPaint(target, &rect, BPBF_COMPATIBLEBITMAP,
            &params, &src);
    ok(buffer == NULL, "Unexpected buffer %p\n", buffer);
    ok(src == NULL, "Unexpected buffered dc %p\n", src);

    /* valid rectangle, no target dc */
    SetRect(&rect, 0, 0, 5, 5);
    src = (void *)0xdeadbeef;
    buffer = pBeginBufferedPaint(NULL, &rect, BPBF_COMPATIBLEBITMAP,
            &params, &src);
    ok(buffer == NULL, "Unexpected buffer %p\n", buffer);
    ok(src == NULL, "Unexpected buffered dc %p\n", src);

    SetRect(&rect, 0, 0, 5, 5);
    src = NULL;
    buffer = pBeginBufferedPaint(target, &rect, BPBF_COMPATIBLEBITMAP,
            &params, &src);
    ok(buffer != NULL, "Unexpected buffer %p\n", buffer);
    ok(src != NULL, "Expected buffered dc\n");
    hr = pEndBufferedPaint(buffer, FALSE);
    ok(hr == S_OK, "Unexpected return code %#lx\n", hr);

    SetRect(&rect, 0, 0, 5, 5);
    buffer = pBeginBufferedPaint(target, &rect, BPBF_COMPATIBLEBITMAP,
            &params, &src);
    ok(buffer != NULL, "Unexpected buffer %p\n", buffer);

    /* clearing */
    hr = pBufferedPaintClear(NULL, NULL);
    todo_wine
    ok(hr == E_FAIL, "Unexpected return code %#lx\n", hr);

    hr = pBufferedPaintClear(buffer, NULL);
    todo_wine
    ok(hr == S_OK, "Unexpected return code %#lx\n", hr);

    /* access buffer attributes */
    hdc = pGetBufferedPaintDC(buffer);
    ok(hdc == src, "Unexpected hdc, %p, buffered dc %p\n", hdc, src);

    hdc = pGetBufferedPaintTargetDC(buffer);
    ok(hdc == target, "Unexpected target hdc %p, original %p\n", hdc, target);

    hr = pGetBufferedPaintTargetRect(NULL, NULL);
    ok(hr == E_POINTER, "Unexpected return code %#lx\n", hr);

    hr = pGetBufferedPaintTargetRect(buffer, NULL);
    ok(hr == E_POINTER, "Unexpected return code %#lx\n", hr);

    hr = pGetBufferedPaintTargetRect(NULL, &rect2);
    ok(hr == E_FAIL, "Unexpected return code %#lx\n", hr);

    SetRectEmpty(&rect2);
    hr = pGetBufferedPaintTargetRect(buffer, &rect2);
    ok(hr == S_OK, "Unexpected return code %#lx\n", hr);
    ok(EqualRect(&rect, &rect2), "Wrong target rect\n");

    hr = pEndBufferedPaint(buffer, FALSE);
    ok(hr == S_OK, "Unexpected return code %#lx\n", hr);

    /* invalid buffer handle */
    hr = pEndBufferedPaint(NULL, FALSE);
    ok(hr == E_INVALIDARG, "Unexpected return code %#lx\n", hr);

    hdc = pGetBufferedPaintDC(NULL);
    ok(hdc == NULL, "Unexpected hdc %p\n", hdc);

    hdc = pGetBufferedPaintTargetDC(NULL);
    ok(hdc == NULL, "Unexpected target hdc %p\n", hdc);

    hr = pGetBufferedPaintTargetRect(NULL, &rect2);
    ok(hr == E_FAIL, "Unexpected return code %#lx\n", hr);

    hr = pGetBufferedPaintTargetRect(NULL, NULL);
    ok(hr == E_POINTER, "Unexpected return code %#lx\n", hr);

    bits = (void *)0xdeadbeef;
    row = 10;
    hr = pGetBufferedPaintBits(NULL, &bits, &row);
    ok(hr == E_FAIL, "Unexpected return code %#lx\n", hr);
    ok(row == 10, "Unexpected row count %d\n", row);
    ok(bits == (void *)0xdeadbeef, "Unexpected data pointer %p\n", bits);

    hr = pGetBufferedPaintBits(NULL, NULL, NULL);
    ok(hr == E_POINTER, "Unexpected return code %#lx\n", hr);

    hr = pGetBufferedPaintBits(NULL, &bits, NULL);
    ok(hr == E_POINTER, "Unexpected return code %#lx\n", hr);

    hr = pGetBufferedPaintBits(NULL, NULL, &row);
    ok(hr == E_POINTER, "Unexpected return code %#lx\n", hr);

    screen_dc = GetDC(0);

    hdc = CreateCompatibleDC(screen_dc);
    ok(hdc != NULL, "Failed to create a DC\n");
    hbm = CreateCompatibleBitmap(screen_dc, 64, 64);
    ok(hbm != NULL, "Failed to create a bitmap\n");
    SelectObject(hdc, hbm);

    ReleaseDC(0, screen_dc);

    SetRect(&rect, 1, 2, 34, 56);

    buffer = pBeginBufferedPaint(hdc, &rect, BPBF_COMPATIBLEBITMAP, NULL, &src);
    test_buffer_dc_props(src, &rect);
    hr = pEndBufferedPaint(buffer, FALSE);
    ok(hr == S_OK, "Unexpected return code %#lx\n", hr);

    DeleteObject(hbm);
    DeleteDC(hdc);

    buffer = pBeginBufferedPaint(target, &rect, BPBF_COMPATIBLEBITMAP, NULL, &src);
    test_buffer_dc_props(src, &rect);
    hr = pEndBufferedPaint(buffer, FALSE);
    ok(hr == S_OK, "Unexpected return code %#lx\n", hr);

    /* access buffer bits */
    for (format = BPBF_COMPATIBLEBITMAP; format <= BPBF_TOPDOWNMONODIB; format++)
    {
        buffer = pBeginBufferedPaint(target, &rect, format, &params, &src);

        /* only works for DIB buffers */
        bits = NULL;
        row = 0;
        hr = pGetBufferedPaintBits(buffer, &bits, &row);
        if (format == BPBF_COMPATIBLEBITMAP)
            ok(hr == E_FAIL, "Unexpected return code %#lx\n", hr);
        else
        {
            ok(hr == S_OK, "Unexpected return code %#lx\n", hr);
            ok(bits != NULL, "Bitmap bits %p\n", bits);
            ok(row >= (rect.right - rect.left), "format %d: bitmap width %d\n", format, row);
        }

        hr = pEndBufferedPaint(buffer, FALSE);
        ok(hr == S_OK, "Unexpected return code %#lx\n", hr);
    }

    DeleteDC(target);
}

static void test_GetThemePartSize(void)
{
    static const DWORD enabled = 1;
    int old_dpi, current_dpi, system_dpi, target_dpi, min_dpi;
    DPI_AWARENESS_CONTEXT old_context;
    int i, expected, stretch_mark = 0;
    HTHEME htheme = NULL;
    HWND hwnd = NULL;
    HKEY key = NULL;
    HDC hdc = NULL;
    HRESULT hr;
    SIZE size;

    if (!pSetThreadDpiAwarenessContext || !pGetDpiForSystem)
    {
        win_skip("SetThreadDpiAwarenessContext() or GetDpiForSystem() is unavailable.\n");
        return;
    }

    /* Set IgnorePerProcessSystemDPIToast to 1 to disable "fix blurry apps popup" on Windows 10,
     * which may interfere other tests because it steals focus */
    RegOpenKeyA(HKEY_CURRENT_USER, "Control Panel\\Desktop", &key);
    RegSetValueExA(key, "IgnorePerProcessSystemDPIToast", 0, REG_DWORD, (const BYTE *)&enabled,
                   sizeof(enabled));

    old_context = pSetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    current_dpi = get_primary_monitor_effective_dpi();
    old_dpi = current_dpi;
    system_dpi = pGetDpiForSystem();
    target_dpi = system_dpi;

    hwnd = CreateWindowA("Button", "Test", WS_POPUP, 100, 100, 100, 100, NULL, NULL, NULL, NULL);
    hdc = GetDC(hwnd);

    /* Test in the current DPI */
    /* Test that OpenThemeData() with NULL window handle uses system DPI even if DPI changes while running */
    htheme = OpenThemeData(NULL, WC_BUTTONW);
    if (!htheme)
    {
        skip("Theming is inactive.\n");
        goto done;
    }

    /* TMT_TRUESIZESTRETCHMARK is present, choose the next minimum DPI */
    hr = GetThemeInt(htheme, BP_CHECKBOX, CBS_CHECKEDNORMAL, TMT_TRUESIZESTRETCHMARK, &stretch_mark);
    if (SUCCEEDED(hr) && stretch_mark > 0)
    {
        for (i = 7; i >= 1; --i)
        {
            hr = GetThemeInt(htheme, BP_CHECKBOX, CBS_CHECKEDNORMAL,
                             i <= 5 ? TMT_MINDPI1 + i - 1 : TMT_MINDPI6 + i - 6, &min_dpi);
            if (SUCCEEDED(hr) && min_dpi <= system_dpi)
            {
                target_dpi = min_dpi;
                break;
            }
        }
    }
    expected = MulDiv(13, target_dpi, 96);

    hr = GetThemePartSize(htheme, hdc, BP_CHECKBOX, CBS_CHECKEDNORMAL, NULL, TS_DRAW, &size);
    ok(hr == S_OK, "GetThemePartSize failed, hr %#lx.\n", hr);
    ok(compare_uint(size.cx, expected, 1) && compare_uint(size.cy, expected, 1),
       "Got unexpected size %ldx%ld.\n", size.cx, size.cy);
    hr = GetThemePartSize(htheme, NULL, BP_CHECKBOX, CBS_CHECKEDNORMAL, NULL, TS_DRAW, &size);
    ok(hr == S_OK, "GetThemePartSize failed, hr %#lx.\n", hr);
    ok(compare_uint(size.cx, expected, 1) && compare_uint(size.cy, expected, 1),
       "Got unexpected size %ldx%ld.\n", size.cx, size.cy);
    CloseThemeData(htheme);

    /* Test in 192 DPI */
    /* DPI needs to be 50% larger than 96 to avoid the effect of TrueSizeStretchMark */
    if (current_dpi != 192 && !set_primary_monitor_effective_dpi(192))
    {
        skip("Failed to set primary monitor dpi to 192.\n");
        goto done;
    }

    /* Test that OpenThemeData() with NULL window handle uses system DPI even if DPI changes while running */
    expected = MulDiv(13, target_dpi, 96);
    htheme = OpenThemeData(NULL, WC_BUTTONW);
    hr = GetThemePartSize(htheme, hdc, BP_CHECKBOX, CBS_CHECKEDNORMAL, NULL, TS_DRAW, &size);
    ok(hr == S_OK, "GetThemePartSize failed, hr %#lx.\n", hr);
    ok(compare_uint(size.cx, expected, 1) && compare_uint(size.cy, expected, 1),
       "Got unexpected size %ldx%ld.\n", size.cx, size.cy);
    hr = GetThemePartSize(htheme, NULL, BP_CHECKBOX, CBS_CHECKEDNORMAL, NULL, TS_DRAW, &size);
    ok(hr == S_OK, "GetThemePartSize failed, hr %#lx.\n", hr);
    ok(compare_uint(size.cx, expected, 1) && compare_uint(size.cy, expected, 1),
       "Got unexpected size %ldx%ld.\n", size.cx, size.cy);
    CloseThemeData(htheme);

    /* Test that OpenThemeData() with a window handle use window DPI */
    expected = 26;
    htheme = OpenThemeData(hwnd, WC_BUTTONW);
    hr = GetThemePartSize(htheme, hdc, BP_CHECKBOX, CBS_CHECKEDNORMAL, NULL, TS_DRAW, &size);
    ok(hr == S_OK, "GetThemePartSize failed, hr %#lx.\n", hr);
    ok(compare_uint(size.cx, expected, 1) && compare_uint(size.cy, expected, 1),
       "Got unexpected size %ldx%ld.\n", size.cx, size.cy);
    hr = GetThemePartSize(htheme, NULL, BP_CHECKBOX, CBS_CHECKEDNORMAL, NULL, TS_DRAW, &size);
    ok(hr == S_OK, "GetThemePartSize failed, hr %#lx.\n", hr);
    ok(compare_uint(size.cx, expected, 1) && compare_uint(size.cy, expected, 1),
       "Got unexpected size %ldx%ld.\n", size.cx, size.cy);

    /* Test in 96 DPI */
    if (!set_primary_monitor_effective_dpi(96))
    {
        skip("Failed to set primary monitor dpi to 96.\n");
        CloseThemeData(htheme);
        goto done;
    }

    /* Test that theme part size doesn't change even if DPI is changed */
    expected = 26;
    hr = GetThemePartSize(htheme, hdc, BP_CHECKBOX, CBS_CHECKEDNORMAL, NULL, TS_DRAW, &size);
    ok(hr == S_OK, "GetThemePartSize failed, hr %#lx.\n", hr);
    ok(compare_uint(size.cx, expected, 1) && compare_uint(size.cy, expected, 1),
       "Got unexpected size %ldx%ld.\n", size.cx, size.cy);
    hr = GetThemePartSize(htheme, NULL, BP_CHECKBOX, CBS_CHECKEDNORMAL, NULL, TS_DRAW, &size);
    ok(hr == S_OK, "GetThemePartSize failed, hr %#lx.\n", hr);
    ok(compare_uint(size.cx, expected, 1) && compare_uint(size.cy, expected, 1),
       "Got unexpected size %ldx%ld.\n", size.cx, size.cy);

    /* Test that theme part size changes after DPI is changed and theme handle is reopened.
     * If DPI awareness context is not DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2, theme part size
     * still doesn't change, even after the theme handle is reopened. */
    CloseThemeData(htheme);
    htheme = OpenThemeData(hwnd, WC_BUTTONW);
    expected = 13;
    hr = GetThemePartSize(htheme, hdc, BP_CHECKBOX, CBS_CHECKEDNORMAL, NULL, TS_DRAW, &size);
    ok(hr == S_OK, "GetThemePartSize failed, hr %#lx.\n", hr);
    ok(compare_uint(size.cx, expected, 1) && compare_uint(size.cy, expected, 1),
       "Got unexpected size %ldx%ld.\n", size.cx, size.cy);
    hr = GetThemePartSize(htheme, NULL, BP_CHECKBOX, CBS_CHECKEDNORMAL, NULL, TS_DRAW, &size);
    ok(hr == S_OK, "GetThemePartSize failed, hr %#lx.\n", hr);
    ok(compare_uint(size.cx, expected, 1) && compare_uint(size.cy, expected, 1),
       "Got unexpected size %ldx%ld.\n", size.cx, size.cy);
    CloseThemeData(htheme);

    /* Test that OpenThemeData() with NULL window handle use system DPI even if DPI changes while running */
    expected = MulDiv(13, target_dpi, 96);
    htheme = OpenThemeData(NULL, WC_BUTTONW);
    hr = GetThemePartSize(htheme, hdc, BP_CHECKBOX, CBS_CHECKEDNORMAL, NULL, TS_DRAW, &size);
    ok(hr == S_OK, "GetThemePartSize failed, hr %#lx.\n", hr);
    ok(compare_uint(size.cx, expected, 1) && compare_uint(size.cy, expected, 1),
       "Got unexpected size %ldx%ld.\n", size.cx, size.cy);
    hr = GetThemePartSize(htheme, NULL, BP_CHECKBOX, CBS_CHECKEDNORMAL, NULL, TS_DRAW, &size);
    ok(hr == S_OK, "GetThemePartSize failed, hr %#lx.\n", hr);
    ok(compare_uint(size.cx, expected, 1) && compare_uint(size.cy, expected, 1),
       "Got unexpected size %ldx%ld.\n", size.cx, size.cy);
    CloseThemeData(htheme);

done:
    if (hdc)
        ReleaseDC(hwnd, hdc);
    if (hwnd)
        DestroyWindow(hwnd);
    if (get_primary_monitor_effective_dpi() != old_dpi)
        set_primary_monitor_effective_dpi(old_dpi);
    if (key)
    {
        RegDeleteValueA(key, "IgnorePerProcessSystemDPIToast");
        RegCloseKey(key);
    }
    pSetThreadDpiAwarenessContext(old_context);
}

static void test_EnableTheming(void)
{
    WCHAR old_color_string[13], new_color_string[13], color_string[13];
    BOOL old_gradient_caption, new_gradient_caption, gradient_caption;
    BOOL old_flat_menu, new_flat_menu, flat_menu;
    LOGFONTW old_logfont, new_logfont, logfont;
    NONCLIENTMETRICSW old_ncm, new_ncm, ncm;
    DPI_AWARENESS_CONTEXT old_context = 0;
    COLORREF old_color, new_color;
    BOOL is_theme_active, ret;
    DWORD size, length;
    HRESULT hr;
    LSTATUS ls;
    HKEY hkey;

    if (IsThemeActive())
    {
        hr = EnableTheming(TRUE);
        ok(hr == S_OK, "EnableTheming failed, hr %#lx.\n", hr);
        ok(IsThemeActive(), "Expected theming active.\n");

        /* Only run in interactive mode because once theming is disabled, it can't be turned back on
         * via EnableTheming() */
        if (winetest_interactive)
        {
            if (pSetThreadDpiAwarenessContext)
            {
                old_context = pSetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
            }
            else if (get_primary_monitor_effective_dpi() != 96)
            {
                skip("DPI isn't 96, skipping.\n");
                return;
            }

            /* Read current system metrics */
            old_color = GetSysColor(COLOR_SCROLLBAR);
            swprintf(old_color_string, ARRAY_SIZE(old_color_string), L"%d %d %d",
                     GetRValue(old_color), GetGValue(old_color), GetBValue(old_color));

            memset(&old_ncm, 0, sizeof(old_ncm));
            old_ncm.cbSize = FIELD_OFFSET(NONCLIENTMETRICSW, iPaddedBorderWidth);
            ret = SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(old_ncm), &old_ncm, 0);
            ok(ret, "SystemParametersInfoW failed, error %lu.\n", GetLastError());

            memset(&old_logfont, 0, sizeof(old_logfont));
            ret = SystemParametersInfoW(SPI_GETICONTITLELOGFONT, sizeof(old_logfont), &old_logfont, 0);
            ok(ret, "SystemParametersInfoW failed, error %lu.\n", GetLastError());
            ret = SystemParametersInfoW(SPI_GETFLATMENU, 0, &old_flat_menu, 0);
            ok(ret, "SystemParametersInfoW failed, error %lu.\n", GetLastError());
            ret = SystemParametersInfoW(SPI_GETGRADIENTCAPTIONS, 0, &old_gradient_caption, 0);
            ok(ret, "SystemParametersInfoW failed, error %lu.\n", GetLastError());

            /* Write new system metrics to the registry */
            new_color = ~old_color;
            new_flat_menu = !old_flat_menu;
            new_gradient_caption = !old_gradient_caption;
            memcpy(&new_ncm, &old_ncm, sizeof(new_ncm));
            new_ncm.iScrollWidth += 5;
            memcpy(&new_logfont, &old_logfont, sizeof(new_logfont));
            new_logfont.lfWidth += 5;

            ls = RegOpenKeyExW(HKEY_CURRENT_USER, L"Control Panel\\Colors", 0, KEY_ALL_ACCESS,
                               &hkey);
            ok(!ls, "RegOpenKeyExW failed, ls %#lx.\n", ls);

            length = swprintf(new_color_string, ARRAY_SIZE(new_color_string), L"%d %d %d",
                              GetRValue(new_color), GetGValue(new_color), GetBValue(new_color));
            ls = RegSetValueExW(hkey, L"Scrollbar", 0, REG_SZ, (BYTE *)new_color_string,
                                (length + 1) * sizeof(WCHAR));
            ok(!ls, "RegSetValueExW failed, ls %#lx.\n", ls);

            ret = SystemParametersInfoW(SPI_SETNONCLIENTMETRICS, sizeof(new_ncm), &new_ncm,
                                        SPIF_UPDATEINIFILE);
            ok(ret, "SystemParametersInfoW failed, error %lu.\n", GetLastError());
            ret = SystemParametersInfoW(SPI_SETICONTITLELOGFONT, sizeof(new_logfont), &new_logfont,
                                        SPIF_UPDATEINIFILE);
            ok(ret, "SystemParametersInfoW failed, error %lu.\n", GetLastError());
            ret = SystemParametersInfoW(SPI_SETFLATMENU, 0, (void *)(INT_PTR)new_flat_menu,
                                        SPIF_UPDATEINIFILE);
            ok(ret, "SystemParametersInfoW failed, error %lu.\n", GetLastError());
            ret = SystemParametersInfoW(SPI_SETGRADIENTCAPTIONS, 0,
                                        (void *)(INT_PTR)new_gradient_caption, SPIF_UPDATEINIFILE);
            ok(ret, "SystemParametersInfoW failed, error %lu.\n", GetLastError());

            /* Change theming state */
            hr = EnableTheming(FALSE);
            ok(hr == S_OK, "EnableTheming failed, hr %#lx.\n", hr);
            is_theme_active = IsThemeActive();
            ok(!is_theme_active || broken(is_theme_active), /* Win8+ can no longer disable theming */
               "Expected theming inactive.\n");

            /* Test that system metrics are unchanged */
            size = sizeof(color_string);
            ls = RegQueryValueExW(hkey, L"Scrollbar", NULL, NULL, (BYTE *)color_string, &size);
            ok(!ls, "RegQueryValueExW failed, ls %#lx.\n", ls);
            ok(!lstrcmpW(color_string, new_color_string), "Expected %s, got %s.\n",
               wine_dbgstr_w(new_color_string), wine_dbgstr_w(color_string));

            ret = SystemParametersInfoW(SPI_GETFLATMENU, 0, &flat_menu, 0);
            ok(ret, "SystemParametersInfoW failed, error %lu.\n", GetLastError());
            ok(flat_menu == new_flat_menu, "Expected %d, got %d.\n", new_flat_menu, flat_menu);
            ret = SystemParametersInfoW(SPI_GETGRADIENTCAPTIONS, 0, &gradient_caption, 0);
            ok(ret, "SystemParametersInfoW failed, error %lu.\n", GetLastError());
            ok(gradient_caption == new_gradient_caption, "Expected %d, got %d.\n",
               new_gradient_caption, gradient_caption);

            memset(&ncm, 0, sizeof(ncm));
            ncm.cbSize = FIELD_OFFSET(NONCLIENTMETRICSW, iPaddedBorderWidth);
            ret = SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
            ok(ret, "SystemParametersInfoW failed, error %lu.\n", GetLastError());
            ok(!memcmp(&ncm, &new_ncm, sizeof(ncm)), "Expected non-client metrics unchanged.\n");

            memset(&logfont, 0, sizeof(logfont));
            ret = SystemParametersInfoW(SPI_GETICONTITLELOGFONT, sizeof(logfont), &logfont, 0);
            ok(ret, "SystemParametersInfoW failed, error %lu.\n", GetLastError());
            ok(!memcmp(&logfont, &new_logfont, sizeof(logfont)),
               "Expected icon title font unchanged.\n");

            /* Test that theming cannot be turned on via EnableTheming() */
            hr = EnableTheming(TRUE);
            ok(hr == S_OK, "EnableTheming failed, hr %#lx.\n", hr);
            is_theme_active = IsThemeActive();
            ok(!is_theme_active || broken(is_theme_active), /* Win8+ can no longer disable theming */
               "Expected theming inactive.\n");

            /* Restore system metrics */
            ls = RegSetValueExW(hkey, L"Scrollbar", 0, REG_SZ, (BYTE *)old_color_string,
                                (lstrlenW(old_color_string) + 1) * sizeof(WCHAR));
            ok(!ls, "RegSetValueExW failed, ls %#lx.\n", ls);
            ret = SystemParametersInfoW(SPI_SETFLATMENU, 0, (void *)(INT_PTR)old_flat_menu,
                                        SPIF_UPDATEINIFILE);
            ok(ret, "SystemParametersInfoW failed, error %lu.\n", GetLastError());
            ret = SystemParametersInfoW(SPI_SETGRADIENTCAPTIONS, 0,
                                        (void *)(INT_PTR)old_gradient_caption, SPIF_UPDATEINIFILE);
            ok(ret, "SystemParametersInfoW failed, error %lu.\n", GetLastError());
            ret = SystemParametersInfoW(SPI_SETNONCLIENTMETRICS, sizeof(old_ncm), &old_ncm,
                                        SPIF_UPDATEINIFILE);
            ok(ret, "SystemParametersInfoW failed, error %lu.\n", GetLastError());
            ret = SystemParametersInfoW(SPI_SETICONTITLELOGFONT, sizeof(old_logfont), &old_logfont,
                                        SPIF_UPDATEINIFILE);
            ok(ret, "SystemParametersInfoW failed, error %lu.\n", GetLastError());

            RegCloseKey(hkey);
            if (pSetThreadDpiAwarenessContext)
                pSetThreadDpiAwarenessContext(old_context);
        }
    }
    else
    {
        hr = EnableTheming(FALSE);
        ok(hr == S_OK, "EnableTheming failed, hr %#lx.\n", hr);
        ok(!IsThemeActive(), "Expected theming inactive.\n");

        hr = EnableTheming(TRUE);
        ok(hr == S_OK, "EnableTheming failed, hr %#lx.\n", hr);
        ok(!IsThemeActive(), "Expected theming inactive.\n");

        hr = EnableTheming(FALSE);
        ok(hr == S_OK, "EnableTheming failed, hr %#lx.\n", hr);
        ok(!IsThemeActive(), "Expected theming inactive.\n");
    }
}

static void test_GetThemeIntList(void)
{
    INTLIST intlist;
    HTHEME theme;
    HRESULT hr;
    HWND hwnd;

    if (!pGetThemeIntList)
    {
        win_skip("GetThemeIntList is unavailable.\n");
        return;
    }

    hwnd = CreateWindowA("static", "", WS_POPUP, 0, 0, 100, 100, 0, 0, 0, NULL);
    theme = OpenThemeData(hwnd, L"Button");
    if (!theme)
    {
        skip("Theming is not active.\n");
        DestroyWindow(hwnd);
        return;
    }

    /* Check properties */
    /* TMT_TRANSITIONDURATIONS is a vista+ property */
    hr = pGetThemeIntList(theme, BP_PUSHBUTTON, PBS_NORMAL, TMT_TRANSITIONDURATIONS, &intlist);
    if (LOBYTE(LOWORD(GetVersion())) < 6)
        ok(hr == E_PROP_ID_UNSUPPORTED, "Expected %#lx, got %#lx.\n", E_PROP_ID_UNSUPPORTED, hr);
    else
        ok(hr == S_OK, "GetThemeIntList failed, hr %#lx.\n", hr);

    CloseThemeData(theme);
    DestroyWindow(hwnd);
}

static void test_GetThemeTransitionDuration(void)
{
    int from_state, to_state, expected;
    INTLIST intlist;
    DWORD duration;
    HTHEME theme;
    HRESULT hr;
    HWND hwnd;

    if (!pGetThemeTransitionDuration || !pGetThemeIntList)
    {
        win_skip("GetThemeTransitionDuration or GetThemeIntList is unavailable.\n");
        return;
    }

    hwnd = CreateWindowA("static", "", WS_POPUP, 0, 0, 100, 100, 0, 0, 0, NULL);
    theme = OpenThemeData(hwnd, L"Button");
    if (!theme)
    {
        skip("Theming is not active.\n");
        DestroyWindow(hwnd);
        return;
    }

    /* Invalid parameter tests */
    duration = 0xdeadbeef;
    hr = pGetThemeTransitionDuration(NULL, BP_PUSHBUTTON, PBS_NORMAL, PBS_DEFAULTED_ANIMATING,
                                     TMT_TRANSITIONDURATIONS, &duration);
    ok(hr == E_HANDLE, "GetThemeTransitionDuration failed, hr %#lx.\n", hr);
    ok(duration == 0xdeadbeef, "Expected duration %#x, got %#lx.\n", 0xdeadbeef, duration);

    /* Crash on Wine. HTHEME is not a pointer that can be directly referenced. */
    if (strcmp(winetest_platform, "wine"))
    {
    duration = 0xdeadbeef;
    hr = pGetThemeTransitionDuration((HTHEME)0xdeadbeef, BP_PUSHBUTTON, PBS_NORMAL,
                                     PBS_DEFAULTED_ANIMATING, TMT_TRANSITIONDURATIONS, &duration);
    todo_wine
    ok(hr == E_HANDLE, "GetThemeTransitionDuration failed, hr %#lx.\n", hr);
    ok(duration == 0xdeadbeef, "Expected duration %#x, got %#lx.\n", 0xdeadbeef, duration);
    }

    duration = 0xdeadbeef;
    hr = pGetThemeTransitionDuration(theme, 0xdeadbeef, PBS_NORMAL, PBS_DEFAULTED_ANIMATING,
                                     TMT_TRANSITIONDURATIONS, &duration);
    ok(hr == E_PROP_ID_UNSUPPORTED, "GetThemeTransitionDuration failed, hr %#lx.\n", hr);
    ok(duration == 0, "Expected duration %#x, got %#lx.\n", 0, duration);

    duration = 0xdeadbeef;
    hr = pGetThemeTransitionDuration(theme, BP_PUSHBUTTON, PBS_NORMAL - 1, PBS_DEFAULTED_ANIMATING,
                                     TMT_TRANSITIONDURATIONS, &duration);
    ok(hr == E_INVALIDARG, "GetThemeTransitionDuration failed, hr %#lx.\n", hr);
    ok(duration == 0xdeadbeef, "Expected duration %#x, got %#lx.\n", 0xdeadbeef, duration);

    duration = 0xdeadbeef;
    hr = pGetThemeTransitionDuration(theme, BP_PUSHBUTTON, PBS_DEFAULTED_ANIMATING + 1,
                                     PBS_DEFAULTED_ANIMATING, TMT_TRANSITIONDURATIONS, &duration);
    ok(hr == E_INVALIDARG, "GetThemeTransitionDuration failed, hr %#lx.\n", hr);
    ok(duration == 0, "Expected duration %#x, got %#lx.\n", 0, duration);

    duration = 0xdeadbeef;
    hr = pGetThemeTransitionDuration(theme, BP_PUSHBUTTON, PBS_NORMAL, PBS_NORMAL - 1,
                                     TMT_TRANSITIONDURATIONS, &duration);
    ok(hr == E_INVALIDARG, "GetThemeTransitionDuration failed, hr %#lx.\n", hr);
    ok(duration == 0xdeadbeef, "Expected duration %#x, got %#lx.\n", 0xdeadbeef, duration);

    duration = 0xdeadbeef;
    hr = pGetThemeTransitionDuration(theme, BP_PUSHBUTTON, PBS_NORMAL, PBS_DEFAULTED_ANIMATING + 1,
                                     TMT_TRANSITIONDURATIONS, &duration);
    ok(hr == E_INVALIDARG, "GetThemeTransitionDuration failed, hr %#lx.\n", hr);
    ok(duration == 0, "Expected duration %#x, got %#lx.\n", 0, duration);

    duration = 0xdeadbeef;
    hr = pGetThemeTransitionDuration(theme, BP_PUSHBUTTON, PBS_NORMAL, PBS_DEFAULTED_ANIMATING,
                                     TMT_BACKGROUND, &duration);
    ok(hr == E_PROP_ID_UNSUPPORTED, "GetThemeTransitionDuration failed, hr %#lx.\n", hr);
    ok(duration == 0, "Expected duration %#x, got %#lx.\n", 0, duration);

    duration = 0xdeadbeef;
    hr = pGetThemeTransitionDuration(theme, BP_PUSHBUTTON, PBS_NORMAL, PBS_DEFAULTED_ANIMATING,
                                     0xdeadbeef, &duration);
    ok(hr == E_PROP_ID_UNSUPPORTED, "GetThemeTransitionDuration failed, hr %#lx.\n", hr);
    ok(duration == 0, "Expected duration %#x, got %#lx.\n", 0, duration);

    hr = pGetThemeTransitionDuration(theme, BP_PUSHBUTTON, PBS_NORMAL, PBS_DEFAULTED_ANIMATING,
                                     TMT_TRANSITIONDURATIONS, NULL);
    ok(hr == E_INVALIDARG, "GetThemeTransitionDuration failed, hr %#lx.\n", hr);

    /* Parts that don't have TMT_TRANSITIONDURATIONS */
    hr = GetThemeIntList(theme, BP_GROUPBOX, GBS_NORMAL, TMT_TRANSITIONDURATIONS, &intlist);
    ok(hr == E_PROP_ID_UNSUPPORTED, "GetThemeIntList failed, hr %#lx.\n", hr);

    duration = 0xdeadbeef;
    hr = pGetThemeTransitionDuration(theme, BP_GROUPBOX, GBS_NORMAL, GBS_DISABLED,
                                     TMT_TRANSITIONDURATIONS, &duration);
    ok(hr == E_PROP_ID_UNSUPPORTED, "GetThemeTransitionDuration failed, hr %#lx.\n", hr);
    ok(duration == 0, "Expected duration %#x, got %#lx.\n", 0, duration);

    /* Test parsing TMT_TRANSITIONDURATIONS property. TMT_TRANSITIONDURATIONS is a vista+ property */
    if (LOBYTE(LOWORD(GetVersion())) < 6)
        goto done;

    hr = pGetThemeIntList(theme, BP_PUSHBUTTON, PBS_NORMAL, TMT_TRANSITIONDURATIONS, &intlist);
    ok(hr == S_OK, "GetThemeIntList failed, hr %#lx.\n", hr);
    /* The first value is the theme part state count. The following are the values from every state
     * to every state. So the total value count should be 1 + state ^ 2 */
    expected = PBS_DEFAULTED_ANIMATING - PBS_NORMAL + 1;
    ok(intlist.iValues[0] == expected, "Expected the first value %d, got %d.\n", expected,
       intlist.iValues[0]);
    expected = 1 + intlist.iValues[0] * intlist.iValues[0];
    ok(intlist.iValueCount == expected, "Expected value count %d, got %d.\n", expected,
       intlist.iValueCount);
    if (hr == S_OK)
    {
        for (from_state = PBS_NORMAL; from_state <= PBS_DEFAULTED_ANIMATING; ++from_state)
        {
            for (to_state = PBS_NORMAL; to_state <= PBS_DEFAULTED_ANIMATING; ++to_state)
            {
                winetest_push_context("from state %d to %d", from_state, to_state);

                duration = 0xdeadbeef;
                hr = pGetThemeTransitionDuration(theme, BP_PUSHBUTTON, from_state, to_state,
                                                 TMT_TRANSITIONDURATIONS, &duration);
                ok(hr == S_OK, "GetThemeTransitionDuration failed, hr %#lx.\n", hr);
                expected = intlist.iValues[1 + intlist.iValues[0] * (from_state - 1) + (to_state - 1)];
                ok(duration == expected, "Expected duration %d, got %ld.\n", expected, duration);

                winetest_pop_context();
            }
        }
    }

done:
    CloseThemeData(theme);
    DestroyWindow(hwnd);
}

static const struct message DrawThemeParentBackground_seq[] =
{
    {WM_ERASEBKGND, sent},
    {WM_PRINTCLIENT, sent | lparam, 0, PRF_CLIENT},
    {0}
};

static LRESULT WINAPI test_DrawThemeParentBackground_proc(HWND hwnd, UINT message, WPARAM wp,
                                                          LPARAM lp)
{
    static LONG defwndproc_counter;
    struct message msg = {0};
    LRESULT lr;
    POINT org;
    BOOL ret;

    switch (message)
    {
    /* Test that DrawThemeParentBackground() doesn't change brush origins */
    case WM_ERASEBKGND:
    case WM_PRINTCLIENT:
        ret = GetBrushOrgEx((HDC)wp, &org);
        ok(ret, "GetBrushOrgEx failed, error %ld.\n", GetLastError());
        ok(org.x == 0 && org.y == 0, "Expected (0,0), got %s.\n", wine_dbgstr_point(&org));
        break;

    default:
        break;
    }

    msg.message = message;
    msg.flags = sent | wparam | lparam;
    if (defwndproc_counter)
        msg.flags |= defwinproc;
    msg.wParam = wp;
    msg.lParam = lp;
    add_message(sequences, PARENT_SEQ_INDEX, &msg);

    InterlockedIncrement(&defwndproc_counter);
    lr = DefWindowProcA(hwnd, message, wp, lp);
    InterlockedDecrement(&defwndproc_counter);
    return lr;
}

static void test_DrawThemeParentBackground(void)
{
    HWND child, parent;
    WNDCLASSA cls;
    HRESULT hr;
    POINT org;
    RECT rect;
    BOOL ret;
    HDC hdc;

    memset(&cls, 0, sizeof(cls));
    cls.hInstance = GetModuleHandleA(0);
    cls.hCursor = LoadCursorA(0, (LPCSTR)IDC_ARROW);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpfnWndProc = test_DrawThemeParentBackground_proc;
    cls.lpszClassName = "TestDrawThemeParentBackgroundClass";
    RegisterClassA(&cls);

    parent = CreateWindowA("TestDrawThemeParentBackgroundClass", "parent", WS_POPUP | WS_VISIBLE, 0,
                           0, 100, 100, 0, 0, 0, 0);
    ok(parent != NULL, "CreateWindowA failed, error %ld.\n", GetLastError());
    child = CreateWindowA(WC_STATICA, "child", WS_CHILD | WS_VISIBLE, 1, 2, 50, 50, parent, 0, 0,
                          NULL);
    ok(child != NULL, "CreateWindowA failed, error %ld.\n", GetLastError());
    flush_events();
    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    hdc = GetDC(child);
    ret = GetBrushOrgEx(hdc, &org);
    ok(ret, "GetBrushOrgEx failed, error %ld.\n", GetLastError());
    ok(org.x == 0 && org.y == 0, "Expected (0,0), got %s.\n", wine_dbgstr_point(&org));

    hr = DrawThemeParentBackground(child, hdc, NULL);
    ok(SUCCEEDED(hr), "DrawThemeParentBackground failed, hr %#lx.\n", hr);
    ok_sequence(sequences, PARENT_SEQ_INDEX, DrawThemeParentBackground_seq,
                "DrawThemeParentBackground parent", FALSE);
    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    GetClientRect(child, &rect);
    hr = DrawThemeParentBackground(child, hdc, &rect);
    ok(SUCCEEDED(hr), "DrawThemeParentBackground failed, hr %#lx.\n", hr);
    ok_sequence(sequences, PARENT_SEQ_INDEX, DrawThemeParentBackground_seq,
                "DrawThemeParentBackground parent", FALSE);
    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    ReleaseDC(child, hdc);
    DestroyWindow(parent);
    UnregisterClassA("TestDrawThemeParentBackgroundClass", GetModuleHandleA(0));
}

struct test_EnableThemeDialogTexture_param
{
    const CHAR *class_name;
    DWORD style;
};

static const struct message empty_seq[] =
{
    {0}
};

static const struct message wm_erasebkgnd_seq[] =
{
    {WM_ERASEBKGND, sent},
    {WM_CTLCOLORDLG, sent},
    {0}
};

static const struct message wm_ctlcolormsgbox_seq[] =
{
    {WM_CTLCOLORMSGBOX, sent},
    {0}
};

static const struct message wm_ctlcolorbtn_seq[] =
{
    {WM_CTLCOLORBTN, sent},
    {0}
};

static const struct message wm_ctlcolordlg_seq[] =
{
    {WM_CTLCOLORDLG, sent},
    {0}
};

static const struct message wm_ctlcolorstatic_seq[] =
{
    {WM_CTLCOLORSTATIC, sent},
    {0}
};

static HWND dialog_child;
static DWORD dialog_init_flag;
static BOOL handle_WM_ERASEBKGND;
static BOOL handle_WM_CTLCOLORSTATIC;

static INT_PTR CALLBACK test_EnableThemeDialogTexture_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    struct test_EnableThemeDialogTexture_param *param;
    struct message message = {0};

    message.message = msg;
    message.flags = sent | wparam | lparam;
    message.wParam = wp;
    message.lParam = lp;
    add_message(sequences, PARENT_SEQ_INDEX, &message);

    switch (msg)
    {
    case WM_INITDIALOG:
        param = (struct test_EnableThemeDialogTexture_param *)lp;
        dialog_child = CreateWindowA(param->class_name, "child",
                                     param->style | WS_CHILD | WS_VISIBLE, 1, 2, 50, 50, hwnd,
                                     (HMENU)100, 0, NULL);
        ok(dialog_child != NULL, "CreateWindowA failed, error %ld.\n", GetLastError());
        if (dialog_init_flag)
            EnableThemeDialogTexture(hwnd, dialog_init_flag);
        return TRUE;

    case WM_ERASEBKGND:
    {
        if (!handle_WM_ERASEBKGND)
            return FALSE;

        SetWindowLongPtrW(hwnd, DWLP_MSGRESULT, 0);
        return TRUE;
    }

    case WM_CTLCOLORSTATIC:
        return (INT_PTR)(handle_WM_CTLCOLORSTATIC ? GetSysColorBrush(COLOR_MENU) : 0);

    case WM_CLOSE:
        DestroyWindow(dialog_child);
        dialog_child = NULL;
        return TRUE;

    default:
        return FALSE;
    }
}

static void test_EnableThemeDialogTexture(void)
{
    struct test_EnableThemeDialogTexture_param param;
    HWND dialog, child, hwnd, hwnd2;
    int mode, old_mode, count, i, j;
    COLORREF color, old_color;
    HBRUSH brush, brush2;
    HDC child_hdc, hdc;
    LOGBRUSH log_brush;
    char buffer[32];
    ULONG_PTR proc;
    WNDCLASSA cls;
    HTHEME theme;
    DWORD error;
    BITMAP bmp;
    HRESULT hr;
    LRESULT lr;
    POINT org;
    SIZE size;
    UINT msg;
    BOOL ret;

    struct
    {
        DLGTEMPLATE template;
        WORD menu;
        WORD class;
        WORD title;
    } temp = {{0}};

    static const DWORD flags[] =
    {
        ETDT_DISABLE,
        ETDT_ENABLE,
        ETDT_USETABTEXTURE,
        ETDT_USEAEROWIZARDTABTEXTURE,
        ETDT_ENABLETAB,
        ETDT_ENABLEAEROWIZARDTAB,
        /* Bad flags */
        0,
        ETDT_DISABLE | ETDT_ENABLE,
        ETDT_ENABLETAB | ETDT_ENABLEAEROWIZARDTAB
    };

    static const struct invalid_flag_test
    {
        DWORD flag;
        BOOL enabled;
    }
    invalid_flag_tests[] =
    {
        {0, FALSE},
        {ETDT_DISABLE | ETDT_ENABLE, FALSE},
        {ETDT_ENABLETAB | ETDT_ENABLEAEROWIZARDTAB, TRUE},
        {ETDT_USETABTEXTURE | ETDT_USEAEROWIZARDTABTEXTURE, TRUE},
        {ETDT_VALIDBITS, FALSE},
        {~ETDT_VALIDBITS, FALSE},
        {~ETDT_VALIDBITS | ETDT_DISABLE, FALSE}
    };

    static const struct class_test
    {
        struct test_EnableThemeDialogTexture_param param;
        BOOL texture_enabled;
    }
    class_tests[] =
    {
        {{ANIMATE_CLASSA}},
        {{WC_BUTTONA, BS_PUSHBUTTON}, TRUE},
        {{WC_BUTTONA, BS_DEFPUSHBUTTON}, TRUE},
        {{WC_BUTTONA, BS_CHECKBOX}, TRUE},
        {{WC_BUTTONA, BS_AUTOCHECKBOX}, TRUE},
        {{WC_BUTTONA, BS_RADIOBUTTON}, TRUE},
        {{WC_BUTTONA, BS_3STATE}, TRUE},
        {{WC_BUTTONA, BS_AUTO3STATE}, TRUE},
        {{WC_BUTTONA, BS_GROUPBOX}, TRUE},
        {{WC_BUTTONA, BS_USERBUTTON}, TRUE},
        {{WC_BUTTONA, BS_AUTORADIOBUTTON}, TRUE},
        {{WC_BUTTONA, BS_PUSHBOX}, TRUE},
        {{WC_BUTTONA, BS_OWNERDRAW}, TRUE},
        {{WC_BUTTONA, BS_SPLITBUTTON}, TRUE},
        {{WC_BUTTONA, BS_DEFSPLITBUTTON}, TRUE},
        {{WC_BUTTONA, BS_COMMANDLINK}, TRUE},
        {{WC_BUTTONA, BS_DEFCOMMANDLINK}, TRUE},
        {{WC_COMBOBOXA}},
        {{WC_COMBOBOXEXA}},
        {{DATETIMEPICK_CLASSA}},
        {{WC_EDITA}},
        {{WC_HEADERA}},
        {{HOTKEY_CLASSA}},
        {{WC_IPADDRESSA}},
        {{WC_LISTBOXA}},
        {{WC_LISTVIEWA}},
        {{MONTHCAL_CLASSA}},
        {{WC_NATIVEFONTCTLA}},
        {{WC_PAGESCROLLERA}},
        {{PROGRESS_CLASSA}},
        {{REBARCLASSNAMEA}},
        {{WC_STATICA, SS_LEFT}, TRUE},
        {{WC_STATICA, SS_ICON}, TRUE},
        {{WC_STATICA, SS_BLACKRECT}, TRUE},
        {{WC_STATICA, SS_OWNERDRAW}, TRUE},
        {{WC_STATICA, SS_BITMAP}, TRUE},
        {{WC_STATICA, SS_ENHMETAFILE}, TRUE},
        {{WC_STATICA, SS_ETCHEDHORZ}, TRUE},
        {{STATUSCLASSNAMEA}},
        {{"SysLink"}},
        {{WC_TABCONTROLA}},
        {{TOOLBARCLASSNAMEA}},
        {{TOOLTIPS_CLASSA}},
        {{TRACKBAR_CLASSA}},
        {{WC_TREEVIEWA}},
        {{UPDOWN_CLASSA}},
        {{WC_SCROLLBARA}},
    };

    static const struct message_test
    {
        UINT msg;
        const struct message *msg_seq;
    }
    message_tests[] =
    {
        {WM_ERASEBKGND, wm_erasebkgnd_seq},
        {WM_CTLCOLORMSGBOX, wm_ctlcolormsgbox_seq},
        {WM_CTLCOLORBTN, wm_ctlcolorbtn_seq},
        {WM_CTLCOLORDLG, wm_ctlcolordlg_seq},
        {WM_CTLCOLORSTATIC, wm_ctlcolorstatic_seq},
    };

    if (!IsThemeActive())
    {
        skip("Theming is inactive.\n");
        return;
    }

    memset(&cls, 0, sizeof(cls));
    cls.lpfnWndProc = DefWindowProcA;
    cls.hInstance = GetModuleHandleA(NULL);
    cls.hCursor = LoadCursorA(0, (LPCSTR)IDC_ARROW);
    cls.hbrBackground = GetStockObject(GRAY_BRUSH);
    cls.lpszClassName = "TestEnableThemeDialogTextureClass";
    RegisterClassA(&cls);

    temp.template.style = WS_CHILD | WS_VISIBLE;
    temp.template.cx = 100;
    temp.template.cy = 100;
    param.class_name = cls.lpszClassName;
    param.style = 0;
    dialog = CreateDialogIndirectParamA(NULL, &temp.template, GetDesktopWindow(),
                                        test_EnableThemeDialogTexture_proc, (LPARAM)&param);
    ok(dialog != NULL, "CreateDialogIndirectParamA failed, error %ld.\n", GetLastError());
    child = GetDlgItem(dialog, 100);
    ok(child != NULL, "Failed to get child control, error %ld.\n", GetLastError());
    child_hdc = GetDC(child);

    /* Test that dialog procedure is unchanged */
    proc = GetWindowLongPtrA(dialog, DWLP_DLGPROC);
    ok(proc == (ULONG_PTR)test_EnableThemeDialogTexture_proc, "Unexpected proc %#Ix.\n", proc);

    /* Test dialog texture is disabled by default. EnableThemeDialogTexture() needs to be called */
    ret = IsThemeDialogTextureEnabled(dialog);
    ok(!ret, "Expected theme dialog texture disabled.\n");
    ok(GetWindowTheme(dialog) == NULL, "Expected NULL theme handle.\n");

    /* Test ETDT_ENABLE | ETDT_USETABTEXTURE doesn't take effect immediately */
    hr = EnableThemeDialogTexture(dialog, ETDT_ENABLE | ETDT_USETABTEXTURE);
    ok(hr == S_OK, "EnableThemeDialogTexture failed, hr %#lx.\n", hr);
    ret = IsThemeDialogTextureEnabled(dialog);
    ok(ret, "Expected theme dialog texture enabled.\n");

    brush = (HBRUSH)SendMessageW(dialog, WM_CTLCOLORSTATIC, (WPARAM)child_hdc, (LPARAM)child);
    ok(brush == GetSysColorBrush(COLOR_BTNFACE), "Expected brush %p, got %p.\n",
       GetSysColorBrush(COLOR_BTNFACE), brush);
    ret = GetBrushOrgEx(child_hdc, &org);
    ok(ret, "GetBrushOrgEx failed, error %lu.\n", GetLastError());
    ok(org.x == 0 && org.y == 0, "Expected (0,0), got %s.\n", wine_dbgstr_point(&org));

    /* Test WM_THEMECHANGED doesn't make ETDT_ENABLE | ETDT_USETABTEXTURE take effect */
    lr = SendMessageA(dialog, WM_THEMECHANGED, 0, 0);
    ok(lr == 0, "WM_THEMECHANGED failed.\n");
    brush = (HBRUSH)SendMessageW(dialog, WM_CTLCOLORSTATIC, (WPARAM)child_hdc, (LPARAM)child);
    ok(brush == GetSysColorBrush(COLOR_BTNFACE), "Expected brush %p, got %p.\n",
       GetSysColorBrush(COLOR_BTNFACE), brush);

    /* Test WM_ERASEBKGND make ETDT_ENABLE | ETDT_USETABTEXTURE take effect */
    lr = SendMessageA(dialog, WM_ERASEBKGND, (WPARAM)child_hdc, 0);
    ok(lr != 0, "WM_ERASEBKGND failed.\n");
    brush = (HBRUSH)SendMessageW(dialog, WM_CTLCOLORSTATIC, (WPARAM)child_hdc, (LPARAM)child);
    ok(brush != GetSysColorBrush(COLOR_BTNFACE), "Expected brush changed.\n");

    /* Test disabling theme dialog texture should change the brush immediately */
    brush = (HBRUSH)SendMessageW(dialog, WM_CTLCOLORSTATIC, (WPARAM)child_hdc, (LPARAM)child);
    hr = EnableThemeDialogTexture(dialog, ETDT_DISABLE);
    ok(hr == S_OK, "EnableThemeDialogTexture failed, hr %#lx.\n", hr);
    brush2 = (HBRUSH)SendMessageW(dialog, WM_CTLCOLORSTATIC, (WPARAM)child_hdc, (LPARAM)child);
    ok(brush2 != brush, "Expected a different brush.\n");
    ok(brush2 == GetSysColorBrush(COLOR_BTNFACE), "Expected brush %p, got %p.\n",
       GetSysColorBrush(COLOR_BTNFACE), brush2);

    /* Test re-enabling theme dialog texture with ETDT_ENABLE doesn't change the brush */
    brush = (HBRUSH)SendMessageW(dialog, WM_CTLCOLORSTATIC, (WPARAM)child_hdc, (LPARAM)child);
    hr = EnableThemeDialogTexture(dialog, ETDT_ENABLE);
    ok(hr == S_OK, "EnableThemeDialogTexture failed, hr %#lx.\n", hr);
    brush2 = (HBRUSH)SendMessageW(dialog, WM_CTLCOLORSTATIC, (WPARAM)child_hdc, (LPARAM)child);
    ok(brush2 == brush, "Expected the same brush.\n");
    ok(brush2 == GetSysColorBrush(COLOR_BTNFACE), "Expected brush %p, got %p.\n",
       GetSysColorBrush(COLOR_BTNFACE), brush2);

    /* Test adding ETDT_USETABTEXTURE should change the brush immediately */
    brush = (HBRUSH)SendMessageW(dialog, WM_CTLCOLORSTATIC, (WPARAM)child_hdc, (LPARAM)child);
    hr = EnableThemeDialogTexture(dialog, ETDT_USETABTEXTURE);
    ok(hr == S_OK, "EnableThemeDialogTexture failed, hr %#lx.\n", hr);
    brush2 = (HBRUSH)SendMessageW(dialog, WM_CTLCOLORSTATIC, (WPARAM)child_hdc, (LPARAM)child);
    ok(brush2 != brush, "Expected a different brush.\n");

    /* Test ETDT_ENABLE | ETDT_USEAEROWIZARDTABTEXTURE should change the brush immediately */
    brush = (HBRUSH)SendMessageW(dialog, WM_CTLCOLORSTATIC, (WPARAM)child_hdc, (LPARAM)child);
    hr = EnableThemeDialogTexture(dialog, ETDT_ENABLE | ETDT_USEAEROWIZARDTABTEXTURE);
    ok(hr == S_OK, "EnableThemeDialogTexture failed, hr %#lx.\n", hr);
    brush2 = (HBRUSH)SendMessageW(dialog, WM_CTLCOLORSTATIC, (WPARAM)child_hdc, (LPARAM)child);
    /* ETDT_USEAEROWIZARDTABTEXTURE is supported only on Vista+ */
    if (LOBYTE(LOWORD(GetVersion())) < 6)
        ok(brush2 == brush, "Expected the same brush.\n");
    else
        ok(brush2 != brush, "Expected a different brush.\n");

    hr = EnableThemeDialogTexture(dialog, ETDT_DISABLE);
    ok(hr == S_OK, "EnableThemeDialogTexture failed, hr %#lx.\n", hr);
    hr = EnableThemeDialogTexture(dialog, ETDT_ENABLE | ETDT_USETABTEXTURE);
    ok(hr == S_OK, "EnableThemeDialogTexture failed, hr %#lx.\n", hr);

    /* Test that the dialog procedure should take precedence over DefDlgProc() for WM_CTLCOLORSTATIC */
    handle_WM_CTLCOLORSTATIC = TRUE;
    brush = (HBRUSH)SendMessageW(dialog, WM_CTLCOLORSTATIC, (WPARAM)child_hdc, (LPARAM)child);
    ok(brush == GetSysColorBrush(COLOR_MENU), "Expected brush %p, got %p.\n",
       GetSysColorBrush(COLOR_MENU), brush);
    handle_WM_CTLCOLORSTATIC = FALSE;

    /* Test that the dialog procedure should take precedence over DefDlgProc() for WM_ERASEBKGND */
    handle_WM_ERASEBKGND = TRUE;
    lr = SendMessageW(dialog, WM_ERASEBKGND, (WPARAM)child_hdc, 0);
    ok(lr == 0, "Expected 0, got %#Ix.\n", lr);
    handle_WM_ERASEBKGND = FALSE;

    /* Test that dialog doesn't have theme handle opened for itself */
    ok(GetWindowTheme(dialog) == NULL, "Expected NULL theme handle.\n");

    theme = OpenThemeData(NULL, L"Tab");
    ok(theme != NULL, "OpenThemeData failed.\n");
    size.cx = 0;
    size.cy = 0;
    hr = GetThemePartSize(theme, NULL, TABP_BODY, 0, NULL, TS_TRUE, &size);
    ok(hr == S_OK, "GetThemePartSize failed, hr %#lx.\n", hr);
    CloseThemeData(theme);

    /* Test which WM_CTLCOLOR* message uses tab background as dialog texture */
    for (msg = WM_CTLCOLORMSGBOX; msg <= WM_CTLCOLORSTATIC; ++msg)
    {
        winetest_push_context("msg %#x", msg);

        /* Test that some WM_CTLCOLOR* messages change brush origin when dialog texture is on */
        ret = SetBrushOrgEx(child_hdc, 0, 0, NULL);
        ok(ret, "SetBrushOrgEx failed, error %lu.\n", GetLastError());
        SendMessageW(dialog, msg, (WPARAM)child_hdc, (LPARAM)child);
        ret = GetBrushOrgEx(child_hdc, &org);
        ok(ret, "GetBrushOrgEx failed, error %lu.\n", GetLastError());
        /* WM_CTLCOLOREDIT, WM_CTLCOLORLISTBOX and WM_CTLCOLORSCROLLBAR don't use tab background */
        if (msg == WM_CTLCOLOREDIT || msg == WM_CTLCOLORLISTBOX || msg == WM_CTLCOLORSCROLLBAR)
        {
            ok(org.x == 0 && org.y == 0, "Expected (0,0), got %s.\n", wine_dbgstr_point(&org));
            winetest_pop_context();
            continue;
        }
        else
        {
            ok(org.x == -1 && org.y == -2, "Expected (-1,-2), got %s.\n", wine_dbgstr_point(&org));
        }

        /* Test that some WM_CTLCOLOR* messages change background mode when dialog texture is on */
        old_mode = SetBkMode(child_hdc, OPAQUE);
        ok(old_mode != 0, "SetBkMode failed.\n");
        SendMessageW(dialog, msg, (WPARAM)child_hdc, (LPARAM)child);
        mode = SetBkMode(child_hdc, old_mode);
        ok(mode == TRANSPARENT, "Expected mode %#x, got %#x.\n", TRANSPARENT, mode);

        /* Test that some WM_CTLCOLOR* messages change background color when dialog texture is on */
        old_color = SetBkColor(child_hdc, 0xaa5511);
        ok(old_color != CLR_INVALID, "SetBkColor failed.\n");
        SendMessageW(dialog, msg, (WPARAM)child_hdc, (LPARAM)child);
        color = SetBkColor(child_hdc, old_color);
        ok(color == GetSysColor(COLOR_BTNFACE), "Expected background color %#lx, got %#lx.\n",
           GetSysColor(COLOR_BTNFACE), color);

        /* Test that the returned brush is a pattern brush created from the tab body */
        brush = (HBRUSH)SendMessageW(dialog, msg, (WPARAM)child_hdc, (LPARAM)child);
        memset(&log_brush, 0, sizeof(log_brush));
        count = GetObjectA(brush, sizeof(log_brush), &log_brush);
        ok(count == sizeof(log_brush), "GetObjectA failed, error %lu.\n", GetLastError());
        ok(log_brush.lbColor == 0, "Expected brush color %#x, got %#lx.\n", 0, log_brush.lbColor);
        ok(log_brush.lbStyle == BS_PATTERN, "Expected brush style %#x, got %#x.\n", BS_PATTERN,
           log_brush.lbStyle);

        memset(&bmp, 0, sizeof(bmp));
        count = GetObjectA((HBITMAP)log_brush.lbHatch, sizeof(bmp), &bmp);
        ok(count == sizeof(bmp), "GetObjectA failed, error %lu.\n", GetLastError());
        ok(bmp.bmWidth == size.cx, "Expected width %ld, got %d.\n", size.cx, bmp.bmWidth);
        ok(bmp.bmHeight == size.cy, "Expected height %ld, got %d.\n", size.cy, bmp.bmHeight);

        /* Test that DefDlgProcA/W() are hooked for some WM_CTLCOLOR* messages */
        brush = (HBRUSH)SendMessageW(dialog, msg, (WPARAM)child_hdc, (LPARAM)child);
        ok(brush != GetSysColorBrush(COLOR_BTNFACE), "Expected a different brush.\n");
        brush2 = (HBRUSH)DefDlgProcW(dialog, msg, (WPARAM)child_hdc, (LPARAM)child);
        ok(brush2 == brush, "Expected the same brush.\n");
        brush2 = (HBRUSH)DefDlgProcA(dialog, msg, (WPARAM)child_hdc, (LPARAM)child);
        ok(brush2 == brush, "Expected the same brush.\n");

        /* Test that DefWindowProcA/W() are also hooked for some WM_CTLCOLOR* messages */
        brush = (HBRUSH)SendMessageW(dialog, msg, (WPARAM)child_hdc, (LPARAM)child);
        ok(brush != GetSysColorBrush(COLOR_BTNFACE), "Expected a different brush.\n");
        if (msg != WM_CTLCOLORDLG)
        {
            brush2 = (HBRUSH)DefWindowProcW(dialog, msg, (WPARAM)child_hdc, (LPARAM)child);
            todo_wine
            ok(brush2 == brush, "Expected the same brush.\n");
            brush2 = (HBRUSH)DefWindowProcA(dialog, msg, (WPARAM)child_hdc, (LPARAM)child);
            todo_wine
            ok(brush2 == brush, "Expected the same brush.\n");
        }
        else
        {
            brush2 = (HBRUSH)DefWindowProcW(dialog, msg, (WPARAM)child_hdc, (LPARAM)child);
            ok(brush2 != brush, "Expected a different brush.\n");
            brush2 = (HBRUSH)DefWindowProcA(dialog, msg, (WPARAM)child_hdc, (LPARAM)child);
            ok(brush2 != brush, "Expected a different brush.\n");
        }

        winetest_pop_context();
    }

    /* Test that DefWindowProcA/W() are not hooked for WM_ERASEBKGND. So the background is still
     * drawn with hbrBackground, which in this case, is GRAY_BRUSH.
     *
     * This test means it could be that both DefWindowProc() and DefDlgProc() are hooked for
     * WM_CTLCOLORSTATIC and only DefDlgProc() is hooked for WM_ERASEBKGND. Or it could mean
     * DefWindowProc() is hooked for WM_CTLCOLORSTATIC and DefDlgProc() is hooked for WM_ERASEBKGND.
     * Considering the dialog theming needs a WM_ERASEBKGND to activate, it would be weird for let
     * only DefWindowProc() to hook WM_CTLCOLORSTATIC. For example, what's the point of hooking
     * WM_CTLCOLORSTATIC in DefWindowProc() for a feature that can only be activated in
     * DefDlgProc()? So I tend to believe both DefWindowProc() and DefDlgProc() are hooked for
     * WM_CTLCOLORSTATIC */
    hwnd = CreateWindowA(cls.lpszClassName, "parent", WS_POPUP | WS_VISIBLE, 0, 0, 100, 100, 0, 0,
                         0, NULL);
    ok(hwnd != NULL, "CreateWindowA failed, error %ld.\n", GetLastError());
    hwnd2 = CreateWindowA(WC_STATICA, "child", WS_CHILD | WS_VISIBLE, 10, 10, 20, 20, hwnd, NULL, 0,
                          NULL);
    hr = EnableThemeDialogTexture(hwnd, ETDT_ENABLETAB);
    ok(hr == S_OK, "EnableThemeDialogTexture failed, hr %#lx.\n", hr);
    ret = IsThemeDialogTextureEnabled(hwnd);
    ok(ret, "Wrong dialog texture status.\n");
    flush_events();

    hdc = GetDC(hwnd);
    color = GetPixel(hdc, 0, 0);
    ok(color == 0x808080 || broken(color == 0xffffffff), /* Win 7 may report 0xffffffff */
       "Expected color %#x, got %#lx.\n", 0x808080, color);
    color = GetPixel(hdc, 50, 50);
    ok(color == 0x808080 || broken(color == 0xffffffff), /* Win 7 may report 0xffffffff */
       "Expected color %#x, got %#lx.\n", 0x808080, color);
    color = GetPixel(hdc, 99, 99);
    ok(color == 0x808080 || broken(color == 0xffffffff), /* Win 7 may report 0xffffffff */
       "Expected color %#x, got %#lx.\n", 0x808080, color);
    ReleaseDC(hwnd, hdc);

    /* Test EnableThemeDialogTexture() doesn't work for non-dialog windows */
    hdc = GetDC(hwnd2);
    brush = (HBRUSH)SendMessageW(hwnd, WM_CTLCOLORSTATIC, (WPARAM)hdc, (LPARAM)hwnd2);
    ok(brush == GetSysColorBrush(COLOR_BTNFACE), "Expected a different brush.\n");
    ReleaseDC(hwnd2, hdc);

    DestroyWindow(hwnd);

    /* Test that the brush is not a system object and has only one reference and shouldn't be freed */
    brush = (HBRUSH)SendMessageW(dialog, WM_CTLCOLORSTATIC, (WPARAM)child_hdc, (LPARAM)child);
    ret = DeleteObject(brush);
    ok(ret, "DeleteObject failed, error %lu.\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = GetObjectA(brush, sizeof(log_brush), &log_brush);
    error = GetLastError();
    ok(!ret || broken(ret) /* XP */, "GetObjectA succeeded.\n");
    todo_wine
    ok(error == ERROR_INVALID_PARAMETER || broken(error == 0xdeadbeef) /* XP */,
       "Expected error %u, got %lu.\n", ERROR_INVALID_PARAMETER, error);
    ret = DeleteObject(brush);
    ok(!ret || broken(ret) /* XP */, "DeleteObject succeeded.\n");

    /* Should still report the same brush handle after the brush handle was freed */
    brush2 = (HBRUSH)SendMessageW(dialog, WM_CTLCOLORSTATIC, (WPARAM)child_hdc, (LPARAM)child);
    ok(brush2 == brush, "Expected the same brush.\n");

    /* Test WM_THEMECHANGED can update the brush now that ETDT_ENABLE | ETDT_USETABTEXTURE is in
     * effect. This test needs to be ran last as it affect other tests for the same dialog for
     * unknown reason, causing the brush not to update */
    brush = (HBRUSH)SendMessageW(dialog, WM_CTLCOLORSTATIC, (WPARAM)child_hdc, (LPARAM)child);
    lr = SendMessageA(dialog, WM_THEMECHANGED, 0, 0);
    ok(lr == 0, "WM_THEMECHANGED failed.\n");
    brush2 = (HBRUSH)SendMessageW(dialog, WM_CTLCOLORSTATIC, (WPARAM)child_hdc, (LPARAM)child);
    ok(brush2 != brush, "Expected a different brush.\n");

    ReleaseDC(child, child_hdc);
    EndDialog(dialog, 0);

    /* Test invalid flags */
    for (i = 0; i < ARRAY_SIZE(invalid_flag_tests); ++i)
    {
        winetest_push_context("%d flag %#lx", i, invalid_flag_tests[i].flag);

        dialog = CreateDialogIndirectParamA(NULL, &temp.template, GetDesktopWindow(),
                                            test_EnableThemeDialogTexture_proc, (LPARAM)&param);
        ok(dialog != NULL, "CreateDialogIndirectParamA failed, error %ld.\n", GetLastError());
        hr = EnableThemeDialogTexture(dialog, invalid_flag_tests[i].flag);
        ok(hr == S_OK, "EnableThemeDialogTexture failed, hr %#lx.\n", hr);
        ret = IsThemeDialogTextureEnabled(dialog);
        ok(ret == invalid_flag_tests[i].enabled, "Wrong dialog texture status.\n");
        EndDialog(dialog, 0);

        winetest_pop_context();
    }

    /* Test different flag combinations */
    for (i = 0; i < ARRAY_SIZE(flags); ++i)
    {
        for (j = 0; j < ARRAY_SIZE(flags); ++j)
        {
            /* ETDT_USEAEROWIZARDTABTEXTURE is supported only on Vista+ */
            if (LOBYTE(LOWORD(GetVersion())) < 6
                && ((flags[i] | flags[j]) & ETDT_USEAEROWIZARDTABTEXTURE))
                continue;

            winetest_push_context("%#lx to %#lx", flags[i], flags[j]);

            dialog = CreateDialogIndirectParamA(NULL, &temp.template, GetDesktopWindow(),
                                                test_EnableThemeDialogTexture_proc, (LPARAM)&param);
            ok(dialog != NULL, "CreateDialogIndirectParamA failed, error %ld.\n", GetLastError());
            flush_events();
            flush_sequences(sequences, NUM_MSG_SEQUENCES);

            hr = EnableThemeDialogTexture(dialog, flags[i]);
            ok(hr == S_OK, "EnableThemeDialogTexture failed, hr %#lx.\n", hr);
            ok_sequence(sequences, PARENT_SEQ_INDEX, empty_seq,
                        "EnableThemeDialogTexture first flag", FALSE);
            ret = IsThemeDialogTextureEnabled(dialog);
            /* Non-zero flags without ETDT_DISABLE enables dialog texture */
            ok(ret == (!(flags[i] & ETDT_DISABLE) && flags[i]), "Wrong dialog texture status.\n");

            child = GetDlgItem(dialog, 100);
            ok(child != NULL, "Failed to get child control, error %ld.\n", GetLastError());
            child_hdc = GetDC(child);
            lr = SendMessageA(dialog, WM_ERASEBKGND, (WPARAM)child_hdc, 0);
            ok(lr != 0, "WM_ERASEBKGND failed.\n");
            brush = (HBRUSH)SendMessageW(dialog, WM_CTLCOLORSTATIC, (WPARAM)child_hdc, (LPARAM)child);
            if (flags[i] == ETDT_ENABLETAB || flags[i] == ETDT_ENABLEAEROWIZARDTAB)
                ok(brush != GetSysColorBrush(COLOR_BTNFACE), "Expected tab texture enabled.\n");
            else
                ok(brush == GetSysColorBrush(COLOR_BTNFACE), "Expected tab texture disabled.\n");
            flush_sequences(sequences, NUM_MSG_SEQUENCES);

            hr = EnableThemeDialogTexture(dialog, flags[j]);
            ok(hr == S_OK, "EnableThemeDialogTexture failed, hr %#lx.\n", hr);
            ok_sequence(sequences, PARENT_SEQ_INDEX, empty_seq,
                        "EnableThemeDialogTexture second flag", FALSE);
            ret = IsThemeDialogTextureEnabled(dialog);
            /* If the flag is zero, it will have previous dialog texture status */
            if (flags[j])
                ok(ret == !(flags[j] & ETDT_DISABLE), "Wrong dialog texture status.\n");
            else
                ok(ret == (!(flags[i] & ETDT_DISABLE) && flags[i]), "Wrong dialog texture status.\n");
            lr = SendMessageA(dialog, WM_ERASEBKGND, (WPARAM)child_hdc, 0);
            ok(lr != 0, "WM_ERASEBKGND failed.\n");
            brush = (HBRUSH)SendMessageW(dialog, WM_CTLCOLORSTATIC, (WPARAM)child_hdc, (LPARAM)child);
            /* Dialog texture is turned on when the flag contains ETDT_ENABLETAB or
             * ETDT_ENABLEAEROWIZARDTAB. The flag can be turned on in multiple steps, but you can't
             * do things like set ETDT_ENABLETAB and then ETDT_USEAEROWIZARDTABTEXTURE */
            if (((flags[j] == ETDT_ENABLETAB || flags[j] == ETDT_ENABLEAEROWIZARDTAB)
                 || ((((flags[i] | flags[j]) & ETDT_ENABLETAB) == ETDT_ENABLETAB
                      || ((flags[i] | flags[j]) & ETDT_ENABLEAEROWIZARDTAB) == ETDT_ENABLEAEROWIZARDTAB)
                      && !((flags[i] | flags[j]) & ETDT_DISABLE)))
                 && (((flags[i] | flags[j]) & (ETDT_ENABLETAB | ETDT_ENABLEAEROWIZARDTAB)) != (ETDT_ENABLETAB | ETDT_ENABLEAEROWIZARDTAB)))
                ok(brush != GetSysColorBrush(COLOR_BTNFACE), "Expected tab texture enabled.\n");
            else
                ok(brush == GetSysColorBrush(COLOR_BTNFACE), "Expected tab texture disabled.\n");

            ReleaseDC(child, child_hdc);
            EndDialog(dialog, 0);

            winetest_pop_context();
        }
    }

    /* Test that the dialog procedure should set ETDT_USETABTEXTURE/ETDT_USEAEROWIZARDTABTEXTURE and
     * find out which comctl32 class should set ETDT_ENABLE to turn on dialog texture */
    for (i = 0; i < ARRAY_SIZE(class_tests); ++i)
    {
        winetest_push_context("%s %#lx", wine_dbgstr_a(class_tests[i].param.class_name),
                              class_tests[i].param.style);

        dialog = CreateDialogIndirectParamA(NULL, &temp.template, GetDesktopWindow(),
                                            test_EnableThemeDialogTexture_proc,
                                            (LPARAM)&class_tests[i].param);
        ok(dialog != NULL, "CreateDialogIndirectParamA failed, error %ld.\n", GetLastError());
        /* GetDlgItem() fails to get the child control if the child is a tooltip */
        child = dialog_child;
        ok(child != NULL, "Failed to get child control, error %ld.\n", GetLastError());
        child_hdc = GetDC(child);

        brush = (HBRUSH)SendMessageW(dialog, WM_CTLCOLORSTATIC, (WPARAM)child_hdc, (LPARAM)child);
        ok(brush == GetSysColorBrush(COLOR_BTNFACE), "Expected tab texture disabled.\n");

        ReleaseDC(child, child_hdc);
        EndDialog(dialog, 0);

        dialog_init_flag = ETDT_ENABLE;
        dialog = CreateDialogIndirectParamA(NULL, &temp.template, GetDesktopWindow(),
                                            test_EnableThemeDialogTexture_proc,
                                            (LPARAM)&class_tests[i].param);
        ok(dialog != NULL, "CreateDialogIndirectParamA failed, error %ld.\n", GetLastError());
        child = dialog_child;
        ok(child != NULL, "Failed to get child control, error %ld.\n", GetLastError());
        child_hdc = GetDC(child);

        brush = (HBRUSH)SendMessageW(dialog, WM_CTLCOLORSTATIC, (WPARAM)child_hdc, (LPARAM)child);
        ok(brush == GetSysColorBrush(COLOR_BTNFACE), "Expected tab texture disabled.\n");

        ReleaseDC(child, child_hdc);
        EndDialog(dialog, 0);

        dialog_init_flag = ETDT_USETABTEXTURE;
        dialog = CreateDialogIndirectParamA(NULL, &temp.template, GetDesktopWindow(),
                                            test_EnableThemeDialogTexture_proc,
                                            (LPARAM)&class_tests[i].param);
        ok(dialog != NULL, "CreateDialogIndirectParamA failed, error %ld.\n", GetLastError());
        child = dialog_child;
        ok(child != NULL, "Failed to get child control, error %ld.\n", GetLastError());
        child_hdc = GetDC(child);
        brush = (HBRUSH)SendMessageW(dialog, WM_CTLCOLORSTATIC, (WPARAM)child_hdc, (LPARAM)child);
        if (class_tests[i].texture_enabled)
            ok(brush != GetSysColorBrush(COLOR_BTNFACE), "Expected tab texture enabled.\n");
        else
            ok(brush == GetSysColorBrush(COLOR_BTNFACE), "Expected tab texture disabled.\n");
        ReleaseDC(child, child_hdc);
        EndDialog(dialog, 0);

        if (LOBYTE(LOWORD(GetVersion())) < 6)
        {
            dialog_init_flag = 0;
            winetest_pop_context();
            continue;
        }

        dialog_init_flag = ETDT_USEAEROWIZARDTABTEXTURE;
        dialog = CreateDialogIndirectParamA(NULL, &temp.template, GetDesktopWindow(),
                                            test_EnableThemeDialogTexture_proc,
                                            (LPARAM)&class_tests[i].param);
        ok(dialog != NULL, "CreateDialogIndirectParamA failed, error %ld.\n", GetLastError());
        child = dialog_child;
        ok(child != NULL, "Failed to get child control, error %ld.\n", GetLastError());
        child_hdc = GetDC(child);
        brush = (HBRUSH)SendMessageW(dialog, WM_CTLCOLORSTATIC, (WPARAM)child_hdc, (LPARAM)child);
        if (class_tests[i].texture_enabled)
            ok(brush != GetSysColorBrush(COLOR_BTNFACE), "Expected tab texture enabled.\n");
        else
            ok(brush == GetSysColorBrush(COLOR_BTNFACE), "Expected tab texture disabled.\n");
        ReleaseDC(child, child_hdc);
        EndDialog(dialog, 0);
        dialog_init_flag = 0;

        winetest_pop_context();
    }

    /* Test that EnableThemeDialogTexture() is called from child controls for its parent */
    hwnd = CreateWindowA(cls.lpszClassName, "parent", WS_POPUP | WS_VISIBLE, 100, 100, 200, 200, 0,
                         0, 0, NULL);
    ok(hwnd != NULL, "CreateWindowA failed, error %ld.\n", GetLastError());
    ret = IsThemeDialogTextureEnabled(hwnd);
    ok(!ret, "Wrong dialog texture status.\n");
    child = CreateWindowA(WC_STATICA, "child", WS_CHILD | WS_VISIBLE, 0, 0, 50, 50, hwnd, 0, 0,
                          NULL);
    ok(child != NULL, "CreateWindowA failed, error %ld.\n", GetLastError());
    ret = IsThemeDialogTextureEnabled(hwnd);
    ok(ret, "Wrong dialog texture status.\n");

    /* Test that if you move the child control to another window, it doesn't enables tab texture for
     * the new parent */
    hwnd2 = CreateWindowA(cls.lpszClassName, "parent", WS_POPUP | WS_VISIBLE, 100, 100, 200, 200, 0,
                          0, 0, NULL);
    ok(hwnd2 != NULL, "CreateWindowA failed, error %ld.\n", GetLastError());
    ret = IsThemeDialogTextureEnabled(hwnd2);
    ok(!ret, "Wrong dialog texture status.\n");

    SetParent(child, hwnd2);
    ok(GetParent(child) == hwnd2, "Wrong parent.\n");
    ret = IsThemeDialogTextureEnabled(hwnd2);
    ok(!ret, "Wrong dialog texture status.\n");
    InvalidateRect(child, NULL, TRUE);
    flush_events();
    ret = IsThemeDialogTextureEnabled(hwnd2);
    ok(!ret, "Wrong dialog texture status.\n");

    DestroyWindow(hwnd2);
    DestroyWindow(hwnd);

    /* Test that application dialog procedures should be called only once for each message */
    dialog = CreateDialogIndirectParamA(NULL, &temp.template, GetDesktopWindow(),
                                        test_EnableThemeDialogTexture_proc,
                                        (LPARAM)&param);
    ok(dialog != NULL, "CreateDialogIndirectParamA failed, error %ld.\n", GetLastError());
    child = dialog_child;
    ok(child != NULL, "Failed to get child control, error %ld.\n", GetLastError());
    child_hdc = GetDC(child);
    for (i = 0; i < ARRAY_SIZE(message_tests); ++i)
    {
        sprintf(buffer, "message %#x\n", message_tests[i].msg);
        flush_sequences(sequences, NUM_MSG_SEQUENCES);
        SendMessageW(dialog, message_tests[i].msg, (WPARAM)child_hdc, (LPARAM)child);
        ok_sequence(sequences, PARENT_SEQ_INDEX, message_tests[i].msg_seq, buffer, FALSE);
    }
    ReleaseDC(child, child_hdc);
    EndDialog(dialog, 0);

    UnregisterClassA(cls.lpszClassName, GetModuleHandleA(NULL));
}

static void test_DrawThemeBackgroundEx(void)
{
    static const int width = 10, height = 10;
    static const RECT rect = {0, 0, 10, 10};
    HBITMAP bitmap, old_bitmap;
    BOOL transparent, found;
    BITMAPINFO bitmap_info;
    int i, glyph_type;
    BYTE *bits, *ptr;
    HTHEME htheme;
    void *proc;
    HDC mem_dc;
    HRESULT hr;
    HWND hwnd;

    proc = GetProcAddress(GetModuleHandleA("uxtheme.dll"), MAKEINTRESOURCEA(47));
    ok(proc == (void *)pDrawThemeBackgroundEx, "Expected DrawThemeBackgroundEx() at ordinal 47.\n");

    hwnd = CreateWindowA(WC_STATICA, "", WS_POPUP, 0, 0, 1, 1, 0, 0, 0, NULL);
    ok(hwnd != NULL, "CreateWindowA failed, error %#lx.\n", GetLastError());
    htheme = OpenThemeData(hwnd, L"Spin");
    if (!htheme)
    {
        skip("Theming is inactive.\n");
        DestroyWindow(hwnd);
        return;
    }

    bitmap_info.bmiHeader.biSize = sizeof(bitmap_info.bmiHeader);
    bitmap_info.bmiHeader.biWidth = width;
    bitmap_info.bmiHeader.biHeight = height;
    bitmap_info.bmiHeader.biPlanes = 1;
    bitmap_info.bmiHeader.biBitCount = 32;
    bitmap_info.bmiHeader.biCompression = BI_RGB;
    bitmap = CreateDIBSection(0, &bitmap_info, DIB_RGB_COLORS, (void **)&bits, 0, 0);
    mem_dc = CreateCompatibleDC(NULL);
    old_bitmap = SelectObject(mem_dc, bitmap);

    /* Drawing opaque background with transparent glyphs should discard the alpha values from the glyphs */
    transparent = IsThemeBackgroundPartiallyTransparent(htheme, SPNP_UP, UPS_NORMAL);
    ok(!transparent, "Expected spin button background opaque.\n");
    hr = GetThemeBool(htheme, SPNP_UP, UPS_NORMAL, TMT_TRANSPARENT, &transparent);
    ok(hr == E_PROP_ID_UNSUPPORTED, "Got unexpected hr %#lx.\n", hr);
    transparent = FALSE;
    hr = GetThemeBool(htheme, SPNP_UP, UPS_NORMAL, TMT_GLYPHTRANSPARENT, &transparent);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(transparent, "Expected spin button glyph transparent.\n");

    memset(bits, 0xa5, width * height * sizeof(int));
    hr = DrawThemeBackgroundEx(htheme, mem_dc, SPNP_UP, UPS_NORMAL, &rect, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ptr = bits;
    for (i = 0; i < width * height; ++i)
    {
        if (ptr[3] != 0xff)
            break;

        ptr += 4;
    }
    ok(i == width * height || broken(ptr[3] == 0) /* Spin button glyphs on XP don't use alpha */,
       "Unexpected alpha value %#x at (%d,%d).\n", ptr[3], i % height, i / height);

    /* Drawing transparent background without glyphs should keep the alpha values */
    CloseThemeData(htheme);
    htheme = OpenThemeData(hwnd, L"Scrollbar");
    transparent = IsThemeBackgroundPartiallyTransparent(htheme, SBP_SIZEBOX, SZB_RIGHTALIGN);
    ok(transparent, "Expected scrollbar sizebox transparent.\n");
    transparent = FALSE;
    hr = GetThemeBool(htheme, SBP_SIZEBOX, SZB_RIGHTALIGN, TMT_TRANSPARENT, &transparent);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(transparent, "Expected scrollbar sizebox transparent.\n");
    hr = GetThemeEnumValue(htheme, SBP_SIZEBOX, SZB_RIGHTALIGN, TMT_GLYPHTYPE, &glyph_type);
    ok(hr == E_PROP_ID_UNSUPPORTED, "Got unexpected hr %#lx.\n", hr);

    memset(bits, 0xa5, width * height * sizeof(int));
    hr = DrawThemeBackgroundEx(htheme, mem_dc, SBP_SIZEBOX, SZB_RIGHTALIGN, &rect, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    found = FALSE;
    ptr = bits;
    for (i = 0; i < width * height; ++i)
    {
        if (ptr[3] == 0xa5)
        {
            found = TRUE;
            break;
        }

        ptr += 4;
    }
    ok(found, "Expected alpha values found.\n");

    /* Drawing transparent background with transparent glyphs should keep alpha values */
    CloseThemeData(htheme);
    htheme = OpenThemeData(hwnd, L"Header");
    if (IsThemePartDefined(htheme, HP_HEADERDROPDOWN, 0))
    {
        transparent = IsThemeBackgroundPartiallyTransparent(htheme, HP_HEADERDROPDOWN, HDDS_NORMAL);
        ok(transparent, "Expected header dropdown transparent.\n");
        transparent = FALSE;
        hr = GetThemeBool(htheme, HP_HEADERDROPDOWN, HDDS_NORMAL, TMT_TRANSPARENT, &transparent);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(transparent, "Expected header dropdown background transparent.\n");
        transparent = FALSE;
        hr = GetThemeBool(htheme, HP_HEADERDROPDOWN, HDDS_NORMAL, TMT_GLYPHTRANSPARENT, &transparent);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(transparent, "Expected header dropdown glyph transparent.\n");

        memset(bits, 0xa5, width * height * sizeof(int));
        hr = DrawThemeBackgroundEx(htheme, mem_dc, HP_HEADERDROPDOWN, HDDS_NORMAL, &rect, NULL);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        found = FALSE;
        ptr = bits;
        for (i = 0; i < width * height; ++i)
        {
            if (ptr[3] == 0xa5)
            {
                found = TRUE;
                break;
            }

            ptr += 4;
        }
        ok(found, "Expected alpha values found.\n");
    }
    else
    {
        skip("Failed to get header dropdown parts.\n");
    }

    SelectObject(mem_dc, old_bitmap);
    DeleteDC(mem_dc);
    DeleteObject(bitmap);
    CloseThemeData(htheme);
    DestroyWindow(hwnd);
}

static void test_GetThemeBackgroundRegion(void)
{
    HTHEME htheme;
    HRGN region;
    HRESULT hr;
    HWND hwnd;
    RECT rect;
    int ret;

    hwnd = CreateWindowA(WC_STATICA, "", WS_POPUP, 0, 0, 1, 1, 0, 0, 0, NULL);
    ok(hwnd != NULL, "CreateWindowA failed, error %#lx.\n", GetLastError());
    htheme = OpenThemeData(hwnd, L"Rebar");
    if (!htheme)
    {
        skip("Theming is inactive.\n");
        DestroyWindow(hwnd);
        return;
    }

    hr = GetThemeEnumValue(htheme, RP_BAND, 0, TMT_BGTYPE, &ret);
    ok(hr == S_OK, "Got unexpected hr %#lx,\n", hr);
    ok(ret == BT_NONE, "Got expected type %d.\n", ret);

    SetRect(&rect, 0, 0, 10, 10);
    region = (HRGN)0xdeadbeef;
    hr = GetThemeBackgroundRegion(htheme, NULL, RP_BAND, 0, &rect, &region);
    ok(hr == E_UNEXPECTED || broken(hr == S_OK) /* < Win10 */, "Got unexpected hr %#lx.\n", hr);
    ok(region == (HRGN)0xdeadbeef, "Got unexpected region.\n");

    CloseThemeData(htheme);
    DestroyWindow(hwnd);
}

static void test_theme(void)
{
    BOOL transparent;
    HTHEME htheme;
    HRESULT hr;
    HWND hwnd;

    if (!IsThemeActive())
    {
        skip("Theming is inactive.\n");
        return;
    }

    hwnd = CreateWindowA(WC_STATICA, "", WS_POPUP, 0, 0, 1, 1, 0, 0, 0, NULL);
    ok(!!hwnd, "CreateWindowA failed, error %#lx.\n", GetLastError());

    /* Test that scrollbar arrow parts are transparent */
    htheme = OpenThemeData(hwnd, L"ScrollBar");
    ok(!!htheme, "OpenThemeData failed.\n");

    transparent = FALSE;
    hr = GetThemeBool(htheme, SBP_ARROWBTN, 0, TMT_TRANSPARENT, &transparent);
    /* XP does use opaque scrollbar arrow parts and TMT_TRANSPARENT is FALSE */
    if (LOBYTE(LOWORD(GetVersion())) < 6)
    {
        ok(hr == E_PROP_ID_UNSUPPORTED, "Got unexpected hr %#lx.\n", hr);

        transparent = IsThemeBackgroundPartiallyTransparent(htheme, SBP_ARROWBTN, 0);
        ok(!transparent, "Expected opaque.\n");
    }
    /* > XP use opaque scrollbar arrow parts, but TMT_TRANSPARENT is TRUE */
    else
    {
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(transparent, "Expected transparent.\n");

        transparent = IsThemeBackgroundPartiallyTransparent(htheme, SBP_ARROWBTN, 0);
        ok(transparent, "Expected transparent.\n");
    }
    CloseThemeData(htheme);

    DestroyWindow(hwnd);
}

static void test_ShouldSystemUseDarkMode(void)
{
    DWORD light_theme, light_theme_size = sizeof(light_theme), last_error;
    BOOLEAN result;
    LSTATUS ls;

    if (!pShouldSystemUseDarkMode)
    {
        win_skip("ShouldSystemUseDarkMode() is unavailable.\n");
        return;
    }

    ls = RegGetValueW(HKEY_CURRENT_USER,
                      L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                      L"SystemUsesLightTheme", RRF_RT_REG_DWORD, NULL, &light_theme, &light_theme_size);
    if (ls == ERROR_FILE_NOT_FOUND)
    {
        skip("SystemUsesLightTheme registry value not found.\n");
        return;
    }
    ok(ls == 0, "RegGetValue failed: %ld.\n", ls);

    SetLastError(0xdeadbeef);
    result = pShouldSystemUseDarkMode();
    last_error = GetLastError();
    ok(last_error == 0xdeadbeef, "ShouldSystemUseDarkMode set last error: %ld.\n", last_error);
    ok(result == !light_theme, "Expected value %d, got %d.\n", !light_theme, result);
}

static void test_ShouldAppsUseDarkMode(void)
{
    DWORD light_theme, light_theme_size = sizeof(light_theme), last_error;
    BOOLEAN result;
    LSTATUS ls;

    if (!pShouldAppsUseDarkMode)
    {
        win_skip("ShouldAppsUseDarkMode() is unavailable\n");
        return;
    }

    ls = RegGetValueW(HKEY_CURRENT_USER,
                      L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                      L"AppsUseLightTheme", RRF_RT_REG_DWORD, NULL, &light_theme, &light_theme_size);
    if (ls == ERROR_FILE_NOT_FOUND)
    {
        skip("AppsUseLightTheme registry value not found.\n");
        return;
    }
    ok(ls == 0, "RegGetValue failed: %ld.\n", ls);

    SetLastError(0xdeadbeef);
    result = pShouldAppsUseDarkMode();
    last_error = GetLastError();
    ok(last_error == 0xdeadbeef, "ShouldAppsUseDarkMode set last error: %ld.\n", last_error);
    ok(result == !light_theme, "Expected value %d, got %d\n", !light_theme, result);
}

START_TEST(system)
{
    ULONG_PTR ctx_cookie;
    HANDLE ctx;

    init_funcs();
    init_msg_sequences(sequences, NUM_MSG_SEQUENCES);

    test_IsThemed();
    test_IsThemePartDefined();
    test_GetWindowTheme();
    test_SetWindowTheme();
    test_OpenThemeData();
    test_OpenThemeDataEx();
    test_OpenThemeDataForDpi();
    test_OpenThemeFile();
    test_GetCurrentThemeName();
    test_GetThemePartSize();
    test_CloseThemeData();
    test_buffered_paint();
    test_GetThemeIntList();
    test_GetThemeTransitionDuration();
    test_DrawThemeParentBackground();
    test_DrawThemeBackgroundEx();
    test_GetThemeBackgroundRegion();
    test_theme();
    test_ShouldSystemUseDarkMode();
    test_ShouldAppsUseDarkMode();

    if (load_v6_module(&ctx_cookie, &ctx))
    {
        test_EnableThemeDialogTexture();

        unload_v6_module(ctx_cookie, ctx);
    }

    /* Test EnableTheming() in the end because it may disable theming */
    test_EnableTheming();
}
