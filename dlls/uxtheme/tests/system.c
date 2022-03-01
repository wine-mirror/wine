/* Unit test suite for uxtheme API functions
 *
 * Copyright 2006 Paul Vriens
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

static HTHEME  (WINAPI * pOpenThemeDataEx)(HWND, LPCWSTR, DWORD);
static HTHEME (WINAPI *pOpenThemeDataForDpi)(HWND, LPCWSTR, UINT);
static HPAINTBUFFER (WINAPI *pBeginBufferedPaint)(HDC, const RECT *, BP_BUFFERFORMAT, BP_PAINTPARAMS *, HDC *);
static HRESULT (WINAPI *pBufferedPaintClear)(HPAINTBUFFER, const RECT *);
static HRESULT (WINAPI *pEndBufferedPaint)(HPAINTBUFFER, BOOL);
static HRESULT (WINAPI *pGetBufferedPaintBits)(HPAINTBUFFER, RGBQUAD **, int *);
static HDC (WINAPI *pGetBufferedPaintDC)(HPAINTBUFFER);
static HDC (WINAPI *pGetBufferedPaintTargetDC)(HPAINTBUFFER);
static HRESULT (WINAPI *pGetBufferedPaintTargetRect)(HPAINTBUFFER, RECT *);
static HRESULT (WINAPI *pGetThemeIntList)(HTHEME, int, int, int, INTLIST *);
static HRESULT (WINAPI *pGetThemeTransitionDuration)(HTHEME, int, int, int, int, DWORD *);

static LONG (WINAPI *pDisplayConfigGetDeviceInfo)(DISPLAYCONFIG_DEVICE_INFO_HEADER *);
static LONG (WINAPI *pDisplayConfigSetDeviceInfo)(DISPLAYCONFIG_DEVICE_INFO_HEADER *);
static BOOL (WINAPI *pGetDpiForMonitorInternal)(HMONITOR, UINT, UINT *, UINT *);
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

#define GET_PROC(module, func)                       \
    p##func = (void *)GetProcAddress(module, #func); \
    if (!p##func)                                    \
        trace("GetProcAddress(%s, %s) failed.\n", #module, #func);

    GET_PROC(uxtheme, BeginBufferedPaint)
    GET_PROC(uxtheme, BufferedPaintClear)
    GET_PROC(uxtheme, EndBufferedPaint)
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
    GET_PROC(user32, SetThreadDpiAwarenessContext)

    GET_PROC(gdi32, D3DKMTCloseAdapter)
    GET_PROC(gdi32, D3DKMTOpenAdapterFromGdiDisplayName)

#undef GET_PROC
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

#define CHECK_FUNC(func)                       \
    if (!p##func)                              \
    {                                          \
        skip("%s() is unavailable.\n", #func); \
        return FALSE;                          \
    }

    CHECK_FUNC(D3DKMTCloseAdapter)
    CHECK_FUNC(D3DKMTOpenAdapterFromGdiDisplayName)
    CHECK_FUNC(DisplayConfigGetDeviceInfo)
    CHECK_FUNC(DisplayConfigSetDeviceInfo)

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
        skip("DisplayConfigGetDeviceInfo failed, returned %d.\n", error);
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
    ok(hwnd != NULL, "CreateWindowA failed, error %#x.\n", GetLastError());

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        if (!is_theme_active && tests[i].class_name)
            continue;

        winetest_push_context("class %s part %d state %d", wine_dbgstr_w(tests[i].class_name),
                              tests[i].part, tests[i].state);

        if (tests[i].class_name)
        {
            theme = OpenThemeData(hwnd, tests[i].class_name);
            ok(theme != NULL, "OpenThemeData failed, error %#x.\n", GetLastError());
        }

        SetLastError(0xdeadbeef);
        ret = IsThemePartDefined(theme, tests[i].part, tests[i].state);
        error = GetLastError();
        ok(ret == tests[i].defined, "Expected %d.\n", tests[i].defined);
        ok(error == tests[i].error, "Expected %#x, got %#x.\n", tests[i].error, error);

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
    ok( GetLastError() == E_HANDLE, "Expected E_HANDLE, got 0x%08x\n", GetLastError() );

    /* Only do the bare minimum to get a valid hwnd */
    hWnd = CreateWindowExA(0, "static", "", WS_POPUP, 0,0,100,100,0, 0, 0, NULL);
    ok(hWnd != NULL, "Failed to create a test window.\n");

    SetLastError(0xdeadbeef);
    hTheme = GetWindowTheme(hWnd);
    ok( hTheme == NULL, "Expected a NULL return, got %p\n", hTheme);
    ok( GetLastError() == 0xdeadbeef,
        "Expected 0xdeadbeef, got 0x%08x\n",
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
    ok( hRes == E_HANDLE, "Expected E_HANDLE, got 0x%08x\n", hRes);

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
    ok(hRes == S_OK, "Expected %#x, got %#x.\n", S_OK, hRes);
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
    ok( hRes == S_OK, "Expected S_OK, got 0x%08x\n", hRes);

    if (IsThemeActive())
    {
        hTheme = OpenThemeData(hWnd, L"Button");
        ok(!!hTheme, "OpenThemeData failed.\n");
        CloseThemeData(hTheme);

        hRes = SetWindowTheme(hWnd, L"deadbeef", NULL);
        ok(hRes == S_OK, "Expected S_OK, got 0x%08x.\n", hRes);

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
            "Expected GLE() to be E_POINTER, got 0x%08x\n",
            GetLastError());

    /* A NULL hWnd and an invalid classlist */
    SetLastError(0xdeadbeef);
    hTheme = OpenThemeData(NULL, szInvalidClassList);
    ok( hTheme == NULL, "Expected a NULL return, got %p\n", hTheme);
    ok( GetLastError() == E_PROP_ID_UNSUPPORTED, "Expected 0x%08x, got 0x%08x\n",
        E_PROP_ID_UNSUPPORTED, GetLastError() );

    SetLastError(0xdeadbeef);
    hTheme = OpenThemeData(NULL, szClassList);
    if (bThemeActive)
    {
        ok( hTheme != NULL, "got NULL, expected a HTHEME handle\n");
        ok( GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got 0x%08x\n", GetLastError() );
    }
    else
    {
        ok( hTheme == NULL, "Expected a NULL return, got %p\n", hTheme);
        ok( GetLastError() == E_PROP_ID_UNSUPPORTED, "Expected 0x%08x, got 0x%08x\n",
            E_PROP_ID_UNSUPPORTED, GetLastError() );
    }

    /* Only do the bare minimum to get a valid hdc */
    hWnd = CreateWindowExA(0, "static", "", WS_POPUP, 0,0,100,100,0, 0, 0, NULL);
    if (!hWnd) return;

    SetLastError(0xdeadbeef);
    hTheme = OpenThemeData(hWnd, NULL);
    ok( hTheme == NULL, "Expected a NULL return, got %p\n", hTheme);
    ok( GetLastError() == E_POINTER,
            "Expected GLE() to be E_POINTER, got 0x%08x\n",
            GetLastError());

    SetLastError(0xdeadbeef);
    hTheme = OpenThemeData(hWnd, szInvalidClassList);
    ok( hTheme == NULL, "Expected a NULL return, got %p\n", hTheme);
    ok( GetLastError() == E_PROP_ID_UNSUPPORTED, "Expected 0x%08x, got 0x%08x\n",
        E_PROP_ID_UNSUPPORTED, GetLastError() );

    if (!bThemeActive)
    {
        SetLastError(0xdeadbeef);
        hTheme = OpenThemeData(hWnd, szButtonClassList);
        ok( hTheme == NULL, "Expected a NULL return, got %p\n", hTheme);
        ok( GetLastError() == E_PROP_ID_UNSUPPORTED, "Expected 0x%08x, got 0x%08x\n",
            E_PROP_ID_UNSUPPORTED, GetLastError() );
        skip("No active theme, skipping rest of OpenThemeData tests\n");
        return;
    }

    /* Only do the next checks if we have an active theme */

    SetLastError(0xdeadbeef);
    hTheme = OpenThemeData(hWnd, szButtonClassList);
    ok( hTheme != NULL, "got NULL, expected a HTHEME handle\n");
    ok( GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got 0x%08x\n", GetLastError() );

    /* Test with bUtToN instead of Button */
    SetLastError(0xdeadbeef);
    hTheme = OpenThemeData(hWnd, szButtonClassList2);
    ok( hTheme != NULL, "got NULL, expected a HTHEME handle\n");
    ok( GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got 0x%08x\n", GetLastError() );

    SetLastError(0xdeadbeef);
    hTheme = OpenThemeData(hWnd, szClassList);
    ok( hTheme != NULL, "got NULL, expected a HTHEME handle\n");
    ok( GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got 0x%08x\n", GetLastError() );

    /* GetWindowTheme should return the last handle opened by OpenThemeData */
    SetLastError(0xdeadbeef);
    hTheme2 = GetWindowTheme(hWnd);
    ok( hTheme == hTheme2, "Expected the same HTHEME handle (%p<->%p)\n",
        hTheme, hTheme2);
    ok( GetLastError() == 0xdeadbeef,
        "Expected 0xdeadbeef, got 0x%08x\n",
        GetLastError());

    hRes = CloseThemeData(hTheme);
    ok( hRes == S_OK, "Expected S_OK, got 0x%08x\n", hRes);

    /* Close a second time */
    hRes = CloseThemeData(hTheme);
    ok( hRes == S_OK, "Expected S_OK, got 0x%08x\n", hRes);

    /* See if closing makes a difference for GetWindowTheme */
    SetLastError(0xdeadbeef);
    hTheme2 = NULL;
    hTheme2 = GetWindowTheme(hWnd);
    ok( hTheme == hTheme2, "Expected the same HTHEME handle (%p<->%p)\n",
        hTheme, hTheme2);
    ok( GetLastError() == 0xdeadbeef,
        "Expected 0xdeadbeef, got 0x%08x\n",
        GetLastError());

    SetLastError(0xdeadbeef);
    bTPDefined = IsThemePartDefined(hTheme, 0 , 0);
    todo_wine
    ok( bTPDefined == FALSE, "Expected FALSE\n" );
    ok( GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got 0x%08x\n", GetLastError() );

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
            "Expected GLE() to be E_POINTER, got 0x%08x\n",
            GetLastError());

    /* A NULL hWnd and an invalid classlist without flags */
    SetLastError(0xdeadbeef);
    hTheme = pOpenThemeDataEx(NULL, szInvalidClassList, 0);
    ok( hTheme == NULL, "Expected a NULL return, got %p\n", hTheme);
    ok( GetLastError() == E_PROP_ID_UNSUPPORTED, "Expected 0x%08x, got 0x%08x\n",
        E_PROP_ID_UNSUPPORTED, GetLastError() );

    SetLastError(0xdeadbeef);
    hTheme = pOpenThemeDataEx(NULL, szClassList, 0);
    if (bThemeActive)
    {
        ok( hTheme != NULL, "got NULL, expected a HTHEME handle\n");
        ok( GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got 0x%08x\n", GetLastError() );
    }
    else
    {
        ok( hTheme == NULL, "Expected a NULL return, got %p\n", hTheme);
        ok( GetLastError() == E_PROP_ID_UNSUPPORTED, "Expected 0x%08x, got 0x%08x\n",
            E_PROP_ID_UNSUPPORTED, GetLastError() );
    }

    /* Only do the bare minimum to get a valid hdc */
    hWnd = CreateWindowExA(0, "static", "", WS_POPUP, 0,0,100,100,0, 0, 0, NULL);
    if (!hWnd) return;

    SetLastError(0xdeadbeef);
    hTheme = pOpenThemeDataEx(hWnd, NULL, 0);
    ok( hTheme == NULL, "Expected a NULL return, got %p\n", hTheme);
    ok( GetLastError() == E_POINTER,
            "Expected GLE() to be E_POINTER, got 0x%08x\n",
            GetLastError());

    SetLastError(0xdeadbeef);
    hTheme = pOpenThemeDataEx(hWnd, szInvalidClassList, 0);
    ok( hTheme == NULL, "Expected a NULL return, got %p\n", hTheme);
    ok( GetLastError() == E_PROP_ID_UNSUPPORTED, "Expected 0x%08x, got 0x%08x\n",
        E_PROP_ID_UNSUPPORTED, GetLastError() );

    if (!bThemeActive)
    {
        SetLastError(0xdeadbeef);
        hTheme = pOpenThemeDataEx(hWnd, szButtonClassList, 0);
        ok( hTheme == NULL, "Expected a NULL return, got %p\n", hTheme);
        ok( GetLastError() == E_PROP_ID_UNSUPPORTED, "Expected 0x%08x, got 0x%08x\n",
            E_PROP_ID_UNSUPPORTED, GetLastError() );
        skip("No active theme, skipping rest of OpenThemeDataEx tests\n");
        return;
    }

    /* Only do the next checks if we have an active theme */

    SetLastError(0xdeadbeef);
    hTheme = pOpenThemeDataEx(hWnd, szButtonClassList, 0);
    ok( hTheme != NULL, "got NULL, expected a HTHEME handle\n");
    ok( GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got 0x%08x\n", GetLastError() );

    SetLastError(0xdeadbeef);
    hTheme = pOpenThemeDataEx(hWnd, szButtonClassList, OTD_FORCE_RECT_SIZING);
    ok( hTheme != NULL, "got NULL, expected a HTHEME handle\n");
    ok( GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got 0x%08x\n", GetLastError() );

    SetLastError(0xdeadbeef);
    hTheme = pOpenThemeDataEx(hWnd, szButtonClassList, OTD_NONCLIENT);
    ok( hTheme != NULL, "got NULL, expected a HTHEME handle\n");
    ok( GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got 0x%08x\n", GetLastError() );

    SetLastError(0xdeadbeef);
    hTheme = pOpenThemeDataEx(hWnd, szButtonClassList, 0x3);
    ok( hTheme != NULL, "got NULL, expected a HTHEME handle\n");
    ok( GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got 0x%08x\n", GetLastError() );

    /* Test with bUtToN instead of Button */
    SetLastError(0xdeadbeef);
    hTheme = pOpenThemeDataEx(hWnd, szButtonClassList2, 0);
    ok( hTheme != NULL, "got NULL, expected a HTHEME handle\n");
    ok( GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got 0x%08x\n", GetLastError() );

    SetLastError(0xdeadbeef);
    hTheme = pOpenThemeDataEx(hWnd, szClassList, 0);
    ok( hTheme != NULL, "got NULL, expected a HTHEME handle\n");
    ok( GetLastError() == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got 0x%08x\n", GetLastError() );

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
        ok(GetLastError() == NO_ERROR, "Expected error %u, got %u.\n", NO_ERROR, GetLastError());
        CloseThemeData(htheme);
    }
    else
    {
        ok(!htheme, "Got a non-NULL handle.\n");
        ok(GetLastError() == E_PROP_ID_UNSUPPORTED, "Expected error %u, got %u.\n",
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
        ok( hRes == S_OK, "Expected S_OK, got 0x%08x\n", hRes);
    else
        ok( hRes == E_PROP_ID_UNSUPPORTED, "Expected E_PROP_ID_UNSUPPORTED, got 0x%08x\n", hRes);

    /* Number of characters given is 0 */
    hRes = GetCurrentThemeName(currentTheme, 0, NULL, 0, NULL, 0);
    if (bThemeActive)
        ok( hRes == S_OK || broken(hRes == E_FAIL /* WinXP SP1 */), "Expected S_OK, got 0x%08x\n", hRes);
    else
        ok( hRes == E_PROP_ID_UNSUPPORTED, "Expected E_PROP_ID_UNSUPPORTED, got 0x%08x\n", hRes);

    hRes = GetCurrentThemeName(currentTheme, 2, NULL, 0, NULL, 0);
    if (bThemeActive)
        todo_wine
            ok(hRes == E_NOT_SUFFICIENT_BUFFER ||
               broken(hRes == E_FAIL /* WinXP SP1 */),
               "Expected E_NOT_SUFFICIENT_BUFFER, got 0x%08x\n", hRes);
    else
        ok( hRes == E_PROP_ID_UNSUPPORTED, "Expected E_PROP_ID_UNSUPPORTED, got 0x%08x\n", hRes);

    /* The same is true if the number of characters is too small for Color and/or Size */
    hRes = GetCurrentThemeName(currentTheme, ARRAY_SIZE(currentTheme), currentColor, 2,
                               currentSize,  ARRAY_SIZE(currentSize));
    if (bThemeActive)
        todo_wine
            ok(hRes == E_NOT_SUFFICIENT_BUFFER ||
               broken(hRes == E_FAIL /* WinXP SP1 */),
               "Expected E_NOT_SUFFICIENT_BUFFER, got 0x%08x\n", hRes);
    else
        ok( hRes == E_PROP_ID_UNSUPPORTED, "Expected E_PROP_ID_UNSUPPORTED, got 0x%08x\n", hRes);

    /* Given number of characters is correct */
    hRes = GetCurrentThemeName(currentTheme, ARRAY_SIZE(currentTheme), NULL, 0, NULL, 0);
    if (bThemeActive)
        ok( hRes == S_OK, "Expected S_OK, got 0x%08x\n", hRes);
    else
        ok( hRes == E_PROP_ID_UNSUPPORTED, "Expected E_PROP_ID_UNSUPPORTED, got 0x%08x\n", hRes);

    /* Given number of characters for the theme name is too large */
    hRes = GetCurrentThemeName(currentTheme, sizeof(currentTheme), NULL, 0, NULL, 0);
    if (bThemeActive)
        ok( hRes == E_POINTER || hRes == S_OK, "Expected E_POINTER or S_OK, got 0x%08x\n", hRes);
    else
        ok( hRes == E_PROP_ID_UNSUPPORTED ||
            hRes == E_POINTER, /* win2k3 */
            "Expected E_PROP_ID_UNSUPPORTED, got 0x%08x\n", hRes);
 
    /* The too large case is only for the theme name, not for color name or size name */
    hRes = GetCurrentThemeName(currentTheme, ARRAY_SIZE(currentTheme), currentColor,
                               sizeof(currentTheme), currentSize,  ARRAY_SIZE(currentSize));
    if (bThemeActive)
        ok( hRes == S_OK, "Expected S_OK, got 0x%08x\n", hRes);
    else
        ok( hRes == E_PROP_ID_UNSUPPORTED, "Expected E_PROP_ID_UNSUPPORTED, got 0x%08x\n", hRes);

    hRes = GetCurrentThemeName(currentTheme, ARRAY_SIZE(currentTheme), currentColor,
                               ARRAY_SIZE(currentTheme), currentSize,  sizeof(currentSize));
    if (bThemeActive)
        ok( hRes == S_OK, "Expected S_OK, got 0x%08x\n", hRes);
    else
        ok( hRes == E_PROP_ID_UNSUPPORTED, "Expected E_PROP_ID_UNSUPPORTED, got 0x%08x\n", hRes);

    /* Correct call */
    hRes = GetCurrentThemeName(currentTheme, ARRAY_SIZE(currentTheme), currentColor,
                               ARRAY_SIZE(currentColor), currentSize,  ARRAY_SIZE(currentSize));
    if (bThemeActive)
        ok( hRes == S_OK, "Expected S_OK, got 0x%08x\n", hRes);
    else
        ok( hRes == E_PROP_ID_UNSUPPORTED, "Expected E_PROP_ID_UNSUPPORTED, got 0x%08x\n", hRes);
}

static void test_CloseThemeData(void)
{
    HRESULT hRes;

    hRes = CloseThemeData(NULL);
    ok( hRes == E_HANDLE, "Expected E_HANDLE, got 0x%08x\n", hRes);
    hRes = CloseThemeData(INVALID_HANDLE_VALUE);
    ok( hRes == E_HANDLE, "Expected E_HANDLE, got 0x%08x\n", hRes);
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
    ok(hr == S_OK, "Unexpected return code %#x\n", hr);

    SetRect(&rect, 0, 0, 5, 5);
    buffer = pBeginBufferedPaint(target, &rect, BPBF_COMPATIBLEBITMAP,
            &params, &src);
    ok(buffer != NULL, "Unexpected buffer %p\n", buffer);

    /* clearing */
    hr = pBufferedPaintClear(NULL, NULL);
todo_wine
    ok(hr == E_FAIL, "Unexpected return code %#x\n", hr);

    hr = pBufferedPaintClear(buffer, NULL);
todo_wine
    ok(hr == S_OK, "Unexpected return code %#x\n", hr);

    /* access buffer attributes */
    hdc = pGetBufferedPaintDC(buffer);
    ok(hdc == src, "Unexpected hdc, %p, buffered dc %p\n", hdc, src);

    hdc = pGetBufferedPaintTargetDC(buffer);
    ok(hdc == target, "Unexpected target hdc %p, original %p\n", hdc, target);

    hr = pGetBufferedPaintTargetRect(NULL, NULL);
    ok(hr == E_POINTER, "Unexpected return code %#x\n", hr);

    hr = pGetBufferedPaintTargetRect(buffer, NULL);
    ok(hr == E_POINTER, "Unexpected return code %#x\n", hr);

    hr = pGetBufferedPaintTargetRect(NULL, &rect2);
    ok(hr == E_FAIL, "Unexpected return code %#x\n", hr);

    SetRectEmpty(&rect2);
    hr = pGetBufferedPaintTargetRect(buffer, &rect2);
    ok(hr == S_OK, "Unexpected return code %#x\n", hr);
    ok(EqualRect(&rect, &rect2), "Wrong target rect\n");

    hr = pEndBufferedPaint(buffer, FALSE);
    ok(hr == S_OK, "Unexpected return code %#x\n", hr);

    /* invalid buffer handle */
    hr = pEndBufferedPaint(NULL, FALSE);
    ok(hr == E_INVALIDARG, "Unexpected return code %#x\n", hr);

    hdc = pGetBufferedPaintDC(NULL);
    ok(hdc == NULL, "Unexpected hdc %p\n", hdc);

    hdc = pGetBufferedPaintTargetDC(NULL);
    ok(hdc == NULL, "Unexpected target hdc %p\n", hdc);

    hr = pGetBufferedPaintTargetRect(NULL, &rect2);
    ok(hr == E_FAIL, "Unexpected return code %#x\n", hr);

    hr = pGetBufferedPaintTargetRect(NULL, NULL);
    ok(hr == E_POINTER, "Unexpected return code %#x\n", hr);

    bits = (void *)0xdeadbeef;
    row = 10;
    hr = pGetBufferedPaintBits(NULL, &bits, &row);
    ok(hr == E_FAIL, "Unexpected return code %#x\n", hr);
    ok(row == 10, "Unexpected row count %d\n", row);
    ok(bits == (void *)0xdeadbeef, "Unexpected data pointer %p\n", bits);

    hr = pGetBufferedPaintBits(NULL, NULL, NULL);
    ok(hr == E_POINTER, "Unexpected return code %#x\n", hr);

    hr = pGetBufferedPaintBits(NULL, &bits, NULL);
    ok(hr == E_POINTER, "Unexpected return code %#x\n", hr);

    hr = pGetBufferedPaintBits(NULL, NULL, &row);
    ok(hr == E_POINTER, "Unexpected return code %#x\n", hr);

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
    ok(hr == S_OK, "Unexpected return code %#x\n", hr);

    DeleteObject(hbm);
    DeleteDC(hdc);

    buffer = pBeginBufferedPaint(target, &rect, BPBF_COMPATIBLEBITMAP, NULL, &src);
    test_buffer_dc_props(src, &rect);
    hr = pEndBufferedPaint(buffer, FALSE);
    ok(hr == S_OK, "Unexpected return code %#x\n", hr);

    /* access buffer bits */
    for (format = BPBF_COMPATIBLEBITMAP; format <= BPBF_TOPDOWNMONODIB; format++)
    {
        buffer = pBeginBufferedPaint(target, &rect, format, &params, &src);

        /* only works for DIB buffers */
        bits = NULL;
        row = 0;
        hr = pGetBufferedPaintBits(buffer, &bits, &row);
        if (format == BPBF_COMPATIBLEBITMAP)
            ok(hr == E_FAIL, "Unexpected return code %#x\n", hr);
        else
        {
            ok(hr == S_OK, "Unexpected return code %#x\n", hr);
            ok(bits != NULL, "Bitmap bits %p\n", bits);
            ok(row >= (rect.right - rect.left), "format %d: bitmap width %d\n", format, row);
        }

        hr = pEndBufferedPaint(buffer, FALSE);
        ok(hr == S_OK, "Unexpected return code %#x\n", hr);
    }

    DeleteDC(target);
}

static void test_GetThemePartSize(void)
{
    static const DWORD enabled = 1;
    DPI_AWARENESS_CONTEXT old_context;
    unsigned int old_dpi, current_dpi;
    HTHEME htheme = NULL;
    HWND hwnd = NULL;
    SIZE size, size2;
    HKEY key = NULL;
    HRESULT hr;
    HDC hdc;

    if (!pSetThreadDpiAwarenessContext)
    {
        win_skip("SetThreadDpiAwarenessContext is unavailable.\n");
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
    /* DPI needs to be 50% larger than 96 to avoid the effect of TrueSizeStretchMark */
    if (current_dpi < 192 && !set_primary_monitor_effective_dpi(192))
    {
        skip("Failed to set primary monitor dpi to 192.\n");
        goto done;
    }

    hwnd = CreateWindowA("Button", "Test", WS_POPUP, 100, 100, 100, 100, NULL, NULL, NULL, NULL);
    htheme = OpenThemeData(hwnd, WC_BUTTONW);
    if (!htheme)
    {
        skip("Theming is inactive.\n");
        goto done;
    }

    hdc = GetDC(hwnd);
    hr = GetThemePartSize(htheme, hdc, BP_CHECKBOX, CBS_CHECKEDNORMAL, NULL, TS_DRAW, &size);
    ok(hr == S_OK, "GetThemePartSize failed, hr %#x.\n", hr);
    hr = GetThemePartSize(htheme, NULL, BP_CHECKBOX, CBS_CHECKEDNORMAL, NULL, TS_DRAW, &size2);
    ok(hr == S_OK, "GetThemePartSize failed, hr %#x.\n", hr);
    ok(size2.cx == size.cx && size2.cy == size.cy, "Expected size %dx%d, got %dx%d.\n",
       size.cx, size.cy, size2.cx, size2.cy);
    ReleaseDC(hwnd, hdc);

    /* Test that theme part size doesn't change even if DPI is changed */
    if (!set_primary_monitor_effective_dpi(96))
    {
        skip("Failed to set primary monitor dpi to 96.\n");
        goto done;
    }

    hdc = GetDC(hwnd);
    hr = GetThemePartSize(htheme, hdc, BP_CHECKBOX, CBS_CHECKEDNORMAL, NULL, TS_DRAW, &size2);
    ok(hr == S_OK, "GetThemePartSize failed, hr %#x.\n", hr);
    ok(size2.cx == size.cx && size2.cy == size.cy, "Expected size %dx%d, got %dx%d.\n", size.cx,
       size.cy, size2.cx, size2.cy);
    hr = GetThemePartSize(htheme, NULL, BP_CHECKBOX, CBS_CHECKEDNORMAL, NULL, TS_DRAW, &size2);
    ok(hr == S_OK, "GetThemePartSize failed, hr %#x.\n", hr);
    ok(size2.cx == size.cx && size2.cy == size.cy, "Expected size %dx%d, got %dx%d.\n", size.cx,
       size.cy, size2.cx, size2.cy);

    /* Test that theme part size changes after DPI is changed and theme handle is reopened.
     * If DPI awareness context is not DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2, theme part size
     * still doesn't change, even after the theme handle is reopened. */
    CloseThemeData(htheme);
    htheme = OpenThemeData(hwnd, WC_BUTTONW);

    hr = GetThemePartSize(htheme, hdc, BP_CHECKBOX, CBS_CHECKEDNORMAL, NULL, TS_DRAW, &size2);
    ok(hr == S_OK, "GetThemePartSize failed, hr %#x.\n", hr);
    ok(size2.cx != size.cx || size2.cy != size.cy, "Expected size not equal to %dx%d.\n", size.cx,
       size.cy);
    hr = GetThemePartSize(htheme, NULL, BP_CHECKBOX, CBS_CHECKEDNORMAL, NULL, TS_DRAW, &size2);
    ok(hr == S_OK, "GetThemePartSize failed, hr %#x.\n", hr);
    ok(size2.cx != size.cx || size2.cy != size.cy, "Expected size not equal to %dx%d.\n", size.cx,
       size.cy);
    ReleaseDC(hwnd, hdc);

    /* Test that OpenThemeData() without a window will assume the DPI is 96 */
    if (!set_primary_monitor_effective_dpi(192))
    {
        skip("Failed to set primary monitor dpi to 192.\n");
        goto done;
    }

    CloseThemeData(htheme);
    htheme = OpenThemeData(NULL, WC_BUTTONW);
    size = size2;

    hdc = GetDC(hwnd);
    hr = GetThemePartSize(htheme, hdc, BP_CHECKBOX, CBS_CHECKEDNORMAL, NULL, TS_DRAW, &size2);
    ok(hr == S_OK, "GetThemePartSize failed, hr %#x.\n", hr);
    ok(size2.cx == size.cx && size2.cy == size.cy, "Expected size %dx%d, got %dx%d.\n", size.cx,
       size.cy, size2.cx, size2.cy);
    hr = GetThemePartSize(htheme, NULL, BP_CHECKBOX, CBS_CHECKEDNORMAL, NULL, TS_DRAW, &size2);
    ok(hr == S_OK, "GetThemePartSize failed, hr %#x.\n", hr);
    ok(size2.cx == size.cx && size2.cy == size.cy, "Expected size %dx%d, got %dx%d.\n", size.cx,
       size.cy, size2.cx, size2.cy);
    ReleaseDC(hwnd, hdc);

done:
    if (hwnd)
        DestroyWindow(hwnd);
    if (htheme)
        CloseThemeData(htheme);
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
        ok(hr == S_OK, "EnableTheming failed, hr %#x.\n", hr);
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
            ok(ret, "SystemParametersInfoW failed, error %u.\n", GetLastError());

            memset(&old_logfont, 0, sizeof(old_logfont));
            ret = SystemParametersInfoW(SPI_GETICONTITLELOGFONT, sizeof(old_logfont), &old_logfont, 0);
            ok(ret, "SystemParametersInfoW failed, error %u.\n", GetLastError());
            ret = SystemParametersInfoW(SPI_GETFLATMENU, 0, &old_flat_menu, 0);
            ok(ret, "SystemParametersInfoW failed, error %u.\n", GetLastError());
            ret = SystemParametersInfoW(SPI_GETGRADIENTCAPTIONS, 0, &old_gradient_caption, 0);
            ok(ret, "SystemParametersInfoW failed, error %u.\n", GetLastError());

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
            ok(!ls, "RegOpenKeyExW failed, ls %#x.\n", ls);

            length = swprintf(new_color_string, ARRAY_SIZE(new_color_string), L"%d %d %d",
                              GetRValue(new_color), GetGValue(new_color), GetBValue(new_color));
            ls = RegSetValueExW(hkey, L"Scrollbar", 0, REG_SZ, (BYTE *)new_color_string,
                                (length + 1) * sizeof(WCHAR));
            ok(!ls, "RegSetValueExW failed, ls %#x.\n", ls);

            ret = SystemParametersInfoW(SPI_SETNONCLIENTMETRICS, sizeof(new_ncm), &new_ncm,
                                        SPIF_UPDATEINIFILE);
            ok(ret, "SystemParametersInfoW failed, error %u.\n", GetLastError());
            ret = SystemParametersInfoW(SPI_SETICONTITLELOGFONT, sizeof(new_logfont), &new_logfont,
                                        SPIF_UPDATEINIFILE);
            ok(ret, "SystemParametersInfoW failed, error %u.\n", GetLastError());
            ret = SystemParametersInfoW(SPI_SETFLATMENU, 0, (void *)(INT_PTR)new_flat_menu,
                                        SPIF_UPDATEINIFILE);
            ok(ret, "SystemParametersInfoW failed, error %u.\n", GetLastError());
            ret = SystemParametersInfoW(SPI_SETGRADIENTCAPTIONS, 0,
                                        (void *)(INT_PTR)new_gradient_caption, SPIF_UPDATEINIFILE);
            ok(ret, "SystemParametersInfoW failed, error %u.\n", GetLastError());

            /* Change theming state */
            hr = EnableTheming(FALSE);
            ok(hr == S_OK, "EnableTheming failed, hr %#x.\n", hr);
            is_theme_active = IsThemeActive();
            ok(!is_theme_active || broken(is_theme_active), /* Win8+ can no longer disable theming */
               "Expected theming inactive.\n");

            /* Test that system metrics are unchanged */
            size = sizeof(color_string);
            ls = RegQueryValueExW(hkey, L"Scrollbar", NULL, NULL, (BYTE *)color_string, &size);
            ok(!ls, "RegQueryValueExW failed, ls %#x.\n", ls);
            ok(!lstrcmpW(color_string, new_color_string), "Expected %s, got %s.\n",
               wine_dbgstr_w(new_color_string), wine_dbgstr_w(color_string));

            ret = SystemParametersInfoW(SPI_GETFLATMENU, 0, &flat_menu, 0);
            ok(ret, "SystemParametersInfoW failed, error %u.\n", GetLastError());
            ok(flat_menu == new_flat_menu, "Expected %d, got %d.\n", new_flat_menu, flat_menu);
            ret = SystemParametersInfoW(SPI_GETGRADIENTCAPTIONS, 0, &gradient_caption, 0);
            ok(ret, "SystemParametersInfoW failed, error %u.\n", GetLastError());
            ok(gradient_caption == new_gradient_caption, "Expected %d, got %d.\n",
               new_gradient_caption, gradient_caption);

            memset(&ncm, 0, sizeof(ncm));
            ncm.cbSize = FIELD_OFFSET(NONCLIENTMETRICSW, iPaddedBorderWidth);
            ret = SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
            ok(ret, "SystemParametersInfoW failed, error %u.\n", GetLastError());
            ok(!memcmp(&ncm, &new_ncm, sizeof(ncm)), "Expected non-client metrics unchanged.\n");

            memset(&logfont, 0, sizeof(logfont));
            ret = SystemParametersInfoW(SPI_GETICONTITLELOGFONT, sizeof(logfont), &logfont, 0);
            ok(ret, "SystemParametersInfoW failed, error %u.\n", GetLastError());
            ok(!memcmp(&logfont, &new_logfont, sizeof(logfont)),
               "Expected icon title font unchanged.\n");

            /* Test that theming cannot be turned on via EnableTheming() */
            hr = EnableTheming(TRUE);
            ok(hr == S_OK, "EnableTheming failed, hr %#x.\n", hr);
            is_theme_active = IsThemeActive();
            ok(!is_theme_active || broken(is_theme_active), /* Win8+ can no longer disable theming */
               "Expected theming inactive.\n");

            /* Restore system metrics */
            ls = RegSetValueExW(hkey, L"Scrollbar", 0, REG_SZ, (BYTE *)old_color_string,
                                (lstrlenW(old_color_string) + 1) * sizeof(WCHAR));
            ok(!ls, "RegSetValueExW failed, ls %#x.\n", ls);
            ret = SystemParametersInfoW(SPI_SETFLATMENU, 0, (void *)(INT_PTR)old_flat_menu,
                                        SPIF_UPDATEINIFILE);
            ok(ret, "SystemParametersInfoW failed, error %u.\n", GetLastError());
            ret = SystemParametersInfoW(SPI_SETGRADIENTCAPTIONS, 0,
                                        (void *)(INT_PTR)old_gradient_caption, SPIF_UPDATEINIFILE);
            ok(ret, "SystemParametersInfoW failed, error %u.\n", GetLastError());
            ret = SystemParametersInfoW(SPI_SETNONCLIENTMETRICS, sizeof(old_ncm), &old_ncm,
                                        SPIF_UPDATEINIFILE);
            ok(ret, "SystemParametersInfoW failed, error %u.\n", GetLastError());
            ret = SystemParametersInfoW(SPI_SETICONTITLELOGFONT, sizeof(old_logfont), &old_logfont,
                                        SPIF_UPDATEINIFILE);
            ok(ret, "SystemParametersInfoW failed, error %u.\n", GetLastError());

            RegCloseKey(hkey);
            if (pSetThreadDpiAwarenessContext)
                pSetThreadDpiAwarenessContext(old_context);
        }
    }
    else
    {
        hr = EnableTheming(FALSE);
        ok(hr == S_OK, "EnableTheming failed, hr %#x.\n", hr);
        ok(!IsThemeActive(), "Expected theming inactive.\n");

        hr = EnableTheming(TRUE);
        ok(hr == S_OK, "EnableTheming failed, hr %#x.\n", hr);
        ok(!IsThemeActive(), "Expected theming inactive.\n");

        hr = EnableTheming(FALSE);
        ok(hr == S_OK, "EnableTheming failed, hr %#x.\n", hr);
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
        ok(hr == E_PROP_ID_UNSUPPORTED, "Expected %#x, got %#x.\n", E_PROP_ID_UNSUPPORTED, hr);
    else
        ok(hr == S_OK, "GetThemeIntList failed, hr %#x.\n", hr);

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
    ok(hr == E_HANDLE, "GetThemeTransitionDuration failed, hr %#x.\n", hr);
    ok(duration == 0xdeadbeef, "Expected duration %#x, got %#x.\n", 0xdeadbeef, duration);

    /* Crash on Wine. HTHEME is not a pointer that can be directly referenced. */
    if (strcmp(winetest_platform, "wine"))
    {
    duration = 0xdeadbeef;
    hr = pGetThemeTransitionDuration((HTHEME)0xdeadbeef, BP_PUSHBUTTON, PBS_NORMAL,
                                     PBS_DEFAULTED_ANIMATING, TMT_TRANSITIONDURATIONS, &duration);
    todo_wine
    ok(hr == E_HANDLE, "GetThemeTransitionDuration failed, hr %#x.\n", hr);
    ok(duration == 0xdeadbeef, "Expected duration %#x, got %#x.\n", 0xdeadbeef, duration);
    }

    duration = 0xdeadbeef;
    hr = pGetThemeTransitionDuration(theme, 0xdeadbeef, PBS_NORMAL, PBS_DEFAULTED_ANIMATING,
                                     TMT_TRANSITIONDURATIONS, &duration);
    ok(hr == E_PROP_ID_UNSUPPORTED, "GetThemeTransitionDuration failed, hr %#x.\n", hr);
    ok(duration == 0, "Expected duration %#x, got %#x.\n", 0, duration);

    duration = 0xdeadbeef;
    hr = pGetThemeTransitionDuration(theme, BP_PUSHBUTTON, PBS_NORMAL - 1, PBS_DEFAULTED_ANIMATING,
                                     TMT_TRANSITIONDURATIONS, &duration);
    ok(hr == E_INVALIDARG, "GetThemeTransitionDuration failed, hr %#x.\n", hr);
    ok(duration == 0xdeadbeef, "Expected duration %#x, got %#x.\n", 0xdeadbeef, duration);

    duration = 0xdeadbeef;
    hr = pGetThemeTransitionDuration(theme, BP_PUSHBUTTON, PBS_DEFAULTED_ANIMATING + 1,
                                     PBS_DEFAULTED_ANIMATING, TMT_TRANSITIONDURATIONS, &duration);
    ok(hr == E_INVALIDARG, "GetThemeTransitionDuration failed, hr %#x.\n", hr);
    ok(duration == 0, "Expected duration %#x, got %#x.\n", 0, duration);

    duration = 0xdeadbeef;
    hr = pGetThemeTransitionDuration(theme, BP_PUSHBUTTON, PBS_NORMAL, PBS_NORMAL - 1,
                                     TMT_TRANSITIONDURATIONS, &duration);
    ok(hr == E_INVALIDARG, "GetThemeTransitionDuration failed, hr %#x.\n", hr);
    ok(duration == 0xdeadbeef, "Expected duration %#x, got %#x.\n", 0xdeadbeef, duration);

    duration = 0xdeadbeef;
    hr = pGetThemeTransitionDuration(theme, BP_PUSHBUTTON, PBS_NORMAL, PBS_DEFAULTED_ANIMATING + 1,
                                     TMT_TRANSITIONDURATIONS, &duration);
    ok(hr == E_INVALIDARG, "GetThemeTransitionDuration failed, hr %#x.\n", hr);
    ok(duration == 0, "Expected duration %#x, got %#x.\n", 0, duration);

    duration = 0xdeadbeef;
    hr = pGetThemeTransitionDuration(theme, BP_PUSHBUTTON, PBS_NORMAL, PBS_DEFAULTED_ANIMATING,
                                     TMT_BACKGROUND, &duration);
    ok(hr == E_PROP_ID_UNSUPPORTED, "GetThemeTransitionDuration failed, hr %#x.\n", hr);
    ok(duration == 0, "Expected duration %#x, got %#x.\n", 0, duration);

    duration = 0xdeadbeef;
    hr = pGetThemeTransitionDuration(theme, BP_PUSHBUTTON, PBS_NORMAL, PBS_DEFAULTED_ANIMATING,
                                     0xdeadbeef, &duration);
    ok(hr == E_PROP_ID_UNSUPPORTED, "GetThemeTransitionDuration failed, hr %#x.\n", hr);
    ok(duration == 0, "Expected duration %#x, got %#x.\n", 0, duration);

    hr = pGetThemeTransitionDuration(theme, BP_PUSHBUTTON, PBS_NORMAL, PBS_DEFAULTED_ANIMATING,
                                     TMT_TRANSITIONDURATIONS, NULL);
    ok(hr == E_INVALIDARG, "GetThemeTransitionDuration failed, hr %#x.\n", hr);

    /* Parts that don't have TMT_TRANSITIONDURATIONS */
    hr = GetThemeIntList(theme, BP_GROUPBOX, GBS_NORMAL, TMT_TRANSITIONDURATIONS, &intlist);
    ok(hr == E_PROP_ID_UNSUPPORTED, "GetThemeIntList failed, hr %#x.\n", hr);

    duration = 0xdeadbeef;
    hr = pGetThemeTransitionDuration(theme, BP_GROUPBOX, GBS_NORMAL, GBS_DISABLED,
                                     TMT_TRANSITIONDURATIONS, &duration);
    ok(hr == E_PROP_ID_UNSUPPORTED, "GetThemeTransitionDuration failed, hr %#x.\n", hr);
    ok(duration == 0, "Expected duration %#x, got %#x.\n", 0, duration);

    /* Test parsing TMT_TRANSITIONDURATIONS property. TMT_TRANSITIONDURATIONS is a vista+ property */
    if (LOBYTE(LOWORD(GetVersion())) < 6)
        goto done;

    hr = pGetThemeIntList(theme, BP_PUSHBUTTON, PBS_NORMAL, TMT_TRANSITIONDURATIONS, &intlist);
    ok(hr == S_OK, "GetThemeIntList failed, hr %#x.\n", hr);
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
                ok(hr == S_OK, "GetThemeTransitionDuration failed, hr %#x.\n", hr);
                expected = intlist.iValues[1 + intlist.iValues[0] * (from_state - 1) + (to_state - 1)];
                ok(duration == expected, "Expected duration %d, got %d.\n", expected, duration);

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
        ok(ret, "GetBrushOrgEx failed, error %d.\n", GetLastError());
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
    ok(parent != NULL, "CreateWindowA failed, error %d.\n", GetLastError());
    child = CreateWindowA(WC_STATICA, "child", WS_CHILD | WS_VISIBLE, 1, 2, 50, 50, parent, 0, 0,
                          NULL);
    ok(child != NULL, "CreateWindowA failed, error %d.\n", GetLastError());
    flush_events();
    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    hdc = GetDC(child);
    ret = GetBrushOrgEx(hdc, &org);
    ok(ret, "GetBrushOrgEx failed, error %d.\n", GetLastError());
    ok(org.x == 0 && org.y == 0, "Expected (0,0), got %s.\n", wine_dbgstr_point(&org));

    hr = DrawThemeParentBackground(child, hdc, NULL);
    ok(SUCCEEDED(hr), "DrawThemeParentBackground failed, hr %#x.\n", hr);
    ok_sequence(sequences, PARENT_SEQ_INDEX, DrawThemeParentBackground_seq,
                "DrawThemeParentBackground parent", FALSE);
    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    GetClientRect(child, &rect);
    hr = DrawThemeParentBackground(child, hdc, &rect);
    ok(SUCCEEDED(hr), "DrawThemeParentBackground failed, hr %#x.\n", hr);
    ok_sequence(sequences, PARENT_SEQ_INDEX, DrawThemeParentBackground_seq,
                "DrawThemeParentBackground parent", FALSE);
    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    ReleaseDC(child, hdc);
    DestroyWindow(parent);
    UnregisterClassA("TestDrawThemeParentBackgroundClass", GetModuleHandleA(0));
}

START_TEST(system)
{
    init_funcs();
    init_msg_sequences(sequences, NUM_MSG_SEQUENCES);

    /* No real functional theme API tests will be done (yet). The current tests
     * only show input/return behaviour
     */

    test_IsThemed();
    test_IsThemePartDefined();
    test_GetWindowTheme();
    test_SetWindowTheme();
    test_OpenThemeData();
    test_OpenThemeDataEx();
    test_OpenThemeDataForDpi();
    test_GetCurrentThemeName();
    test_GetThemePartSize();
    test_CloseThemeData();
    test_buffered_paint();
    test_GetThemeIntList();
    test_GetThemeTransitionDuration();
    test_DrawThemeParentBackground();

    /* Test EnableTheming() in the end because it may disable theming */
    test_EnableTheming();
}
