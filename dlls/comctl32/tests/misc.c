/*
 * Misc tests
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
 */

#include <stdio.h>
#include <windows.h>
#include <commctrl.h>

#include "wine/test.h"
#include "v6util.h"
#include "msg.h"

static PVOID (WINAPI * pAlloc)(LONG);
static PVOID (WINAPI * pReAlloc)(PVOID, LONG);
static BOOL (WINAPI * pFree)(PVOID);
static LONG (WINAPI * pGetSize)(PVOID);

static INT (WINAPI * pStr_GetPtrA)(LPCSTR, LPSTR, INT);
static BOOL (WINAPI * pStr_SetPtrA)(LPSTR, LPCSTR);
static INT (WINAPI * pStr_GetPtrW)(LPCWSTR, LPWSTR, INT);
static BOOL (WINAPI * pStr_SetPtrW)(LPWSTR, LPCWSTR);

static BOOL (WINAPI *pSetWindowSubclass)(HWND, SUBCLASSPROC, UINT_PTR, DWORD_PTR);
static BOOL (WINAPI *pRemoveWindowSubclass)(HWND, SUBCLASSPROC, UINT_PTR);
static LRESULT (WINAPI *pDefSubclassProc)(HWND, UINT, WPARAM, LPARAM);

static HMODULE hComctl32;

/* For message tests */
enum seq_index
{
    CHILD_SEQ_INDEX,
    NUM_MSG_SEQUENCES
};

static struct msg_sequence *sequences[NUM_MSG_SEQUENCES];

static char testicon_data[] =
{
    0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x02, 0x02, 0x00, 0x00, 0x01, 0x00,
    0x20, 0x00, 0x40, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x28, 0x00,
    0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x01, 0x00,
    0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x12, 0x0b,
    0x00, 0x00, 0x12, 0x0b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0xde, 0xde, 0xde, 0xff, 0xde, 0xde, 0xde, 0xff, 0xde, 0xde,
    0xde, 0xff, 0xde, 0xde, 0xde, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00
};

#define COMCTL32_GET_PROC(ordinal, func) \
    p ## func = (void*)GetProcAddress(hComctl32, (LPSTR)ordinal); \
    if(!p ## func) { \
      trace("GetProcAddress(%d)(%s) failed\n", ordinal, #func); \
      FreeLibrary(hComctl32); \
    }

static BOOL InitFunctionPtrs(void)
{
    hComctl32 = LoadLibraryA("comctl32.dll");

    if(!hComctl32)
    {
        trace("Could not load comctl32.dll\n");
        return FALSE;
    }

    COMCTL32_GET_PROC(71, Alloc);
    COMCTL32_GET_PROC(72, ReAlloc);
    COMCTL32_GET_PROC(73, Free);
    COMCTL32_GET_PROC(74, GetSize);

    COMCTL32_GET_PROC(233, Str_GetPtrA)
    COMCTL32_GET_PROC(234, Str_SetPtrA)
    COMCTL32_GET_PROC(235, Str_GetPtrW)
    COMCTL32_GET_PROC(236, Str_SetPtrW)

    return TRUE;
}

static BOOL init_functions_v6(void)
{
    hComctl32 = LoadLibraryA("comctl32.dll");
    if (!hComctl32)
    {
        trace("Could not load comctl32.dll version 6\n");
        return FALSE;
    }

    COMCTL32_GET_PROC(410, SetWindowSubclass)
    COMCTL32_GET_PROC(412, RemoveWindowSubclass)
    COMCTL32_GET_PROC(413, DefSubclassProc)

    return TRUE;
}

/* try to make sure pending X events have been processed before continuing */
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

static void test_GetPtrAW(void)
{
    if (pStr_GetPtrA)
    {
        static const char source[] = "Just a source string";
        static const char desttest[] = "Just a destination string";
        static char dest[MAX_PATH];
        int sourcelen;
        int destsize = MAX_PATH;
        int count;

        sourcelen = strlen(source) + 1;

        count = pStr_GetPtrA(NULL, NULL, 0);
        ok (count == 0, "Expected count to be 0, it was %d\n", count);

        if (0)
        {
            /* Crashes on W98, NT4, W2K, XP, W2K3
             * Our implementation also crashes and we should probably leave
             * it like that.
             */
            count = pStr_GetPtrA(NULL, NULL, destsize);
            trace("count : %d\n", count);
        }

        count = pStr_GetPtrA(source, NULL, 0);
        ok (count == sourcelen ||
            broken(count == sourcelen - 1), /* win9x */
            "Expected count to be %d, it was %d\n", sourcelen, count);

        strcpy(dest, desttest);
        count = pStr_GetPtrA(source, dest, 0);
        ok (count == sourcelen ||
            broken(count == 0), /* win9x */
            "Expected count to be %d, it was %d\n", sourcelen, count);
        ok (!lstrcmpA(dest, desttest) ||
            broken(!lstrcmpA(dest, "")), /* Win7 */
            "Expected destination to not have changed\n");

        count = pStr_GetPtrA(source, NULL, destsize);
        ok (count == sourcelen ||
            broken(count == sourcelen - 1), /* win9x */
            "Expected count to be %d, it was %d\n", sourcelen, count);

        count = pStr_GetPtrA(source, dest, destsize);
        ok (count == sourcelen ||
            broken(count == sourcelen - 1), /* win9x */
            "Expected count to be %d, it was %d\n", sourcelen, count);
        ok (!lstrcmpA(source, dest), "Expected source and destination to be the same\n");

        strcpy(dest, desttest);
        count = pStr_GetPtrA(NULL, dest, destsize);
        ok (count == 0, "Expected count to be 0, it was %d\n", count);
        ok (dest[0] == '\0', "Expected destination to be cut-off and 0 terminated\n");

        destsize = 15;
        count = pStr_GetPtrA(source, dest, destsize);
        ok (count == 15 ||
            broken(count == 14), /* win9x */
            "Expected count to be 15, it was %d\n", count);
        ok (!memcmp(source, dest, 14), "Expected first part of source and destination to be the same\n");
        ok (dest[14] == '\0', "Expected destination to be cut-off and 0 terminated\n");
    }
}

static void test_Alloc(void)
{
    PCHAR p;
    BOOL res;
    DWORD size, min;

    /* allocate size 0 */
    p = pAlloc(0);
    ok(p != NULL, "Expected non-NULL ptr\n");

    /* get the minimum size */
    min = pGetSize(p);

    /* free the block */
    res = pFree(p);
    ok(res == TRUE, "Expected TRUE, got %d\n", res);

    /* allocate size 1 */
    p = pAlloc(1);
    ok(p != NULL, "Expected non-NULL ptr\n");

    /* get the allocated size */
    size = pGetSize(p);
    ok(size == 1 ||
       broken(size == min), /* win9x */
       "Expected 1, got %d\n", size);

    /* reallocate the block */
    p = pReAlloc(p, 2);
    ok(p != NULL, "Expected non-NULL ptr\n");

    /* get the new size */
    size = pGetSize(p);
    ok(size == 2 ||
       broken(size == min), /* win9x */
       "Expected 2, got %d\n", size);

    /* free the block */
    res = pFree(p);
    ok(res == TRUE, "Expected TRUE, got %d\n", res);

    /* free a NULL ptr */
    res = pFree(NULL);
    ok(res == TRUE ||
       broken(res == FALSE), /* win9x */
       "Expected TRUE, got %d\n", res);

    /* reallocate a NULL ptr */
    p = pReAlloc(NULL, 2);
    ok(p != NULL, "Expected non-NULL ptr\n");

    res = pFree(p);
    ok(res == TRUE, "Expected TRUE, got %d\n", res);
}

static void test_LoadIconWithScaleDown(void)
{
    HRESULT (WINAPI *pLoadIconMetric)(HINSTANCE, const WCHAR *, int, HICON *);
    HRESULT (WINAPI *pLoadIconWithScaleDown)(HINSTANCE, const WCHAR *, int, int, HICON *);
    WCHAR tmp_path[MAX_PATH], icon_path[MAX_PATH];
    ICONINFO info;
    HMODULE hinst;
    HANDLE handle;
    DWORD written;
    HRESULT hr;
    BITMAP bmp;
    HICON icon;
    void *ptr;
    int bytes;
    BOOL res;

    hinst = LoadLibraryA("comctl32.dll");
    pLoadIconMetric        = (void *)GetProcAddress(hinst, "LoadIconMetric");
    pLoadIconWithScaleDown = (void *)GetProcAddress(hinst, "LoadIconWithScaleDown");
    if (!pLoadIconMetric || !pLoadIconWithScaleDown)
    {
        win_skip("LoadIconMetric or pLoadIconWithScaleDown not exported by name\n");
        FreeLibrary(hinst);
        return;
    }

    GetTempPathW(MAX_PATH, tmp_path);
    GetTempFileNameW(tmp_path, L"ICO", 0, icon_path);
    handle = CreateFileW(icon_path, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                         FILE_ATTRIBUTE_NORMAL, NULL);
    ok(handle != INVALID_HANDLE_VALUE, "CreateFileW failed with error %u\n", GetLastError());
    res = WriteFile(handle, testicon_data, sizeof(testicon_data), &written, NULL);
    ok(res && written == sizeof(testicon_data), "Failed to write icon file\n");
    CloseHandle(handle);

    /* test ordinals */
    ptr = GetProcAddress(hinst, (const char *)380);
    ok(ptr == pLoadIconMetric,
       "got wrong pointer for ordinal 380, %p expected %p\n", ptr, pLoadIconMetric);

    ptr = GetProcAddress(hinst, (const char *)381);
    ok(ptr == pLoadIconWithScaleDown,
       "got wrong pointer for ordinal 381, %p expected %p\n", ptr, pLoadIconWithScaleDown);

    /* invalid arguments */
    icon = (HICON)0x1234;
    hr = pLoadIconMetric(NULL, (LPWSTR)IDI_APPLICATION, 0x100, &icon);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %x\n", hr);
    ok(icon == NULL, "Expected NULL, got %p\n", icon);

    icon = (HICON)0x1234;
    hr = pLoadIconMetric(NULL, NULL, LIM_LARGE, &icon);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %x\n", hr);
    ok(icon == NULL, "Expected NULL, got %p\n", icon);

    icon = (HICON)0x1234;
    hr = pLoadIconWithScaleDown(NULL, NULL, 32, 32, &icon);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %x\n", hr);
    ok(icon == NULL, "Expected NULL, got %p\n", icon);

    /* non-existing filename */
    hr = pLoadIconMetric(NULL, L"nonexisting.ico", LIM_LARGE, &icon);
    ok(hr == HRESULT_FROM_WIN32(ERROR_RESOURCE_TYPE_NOT_FOUND) || hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) /* Win7 */,
       "Expected HRESULT_FROM_WIN32(ERROR_RESOURCE_TYPE_NOT_FOUND), got %x\n", hr);

    hr = pLoadIconWithScaleDown(NULL, L"nonexisting.ico", 32, 32, &icon);
    todo_wine
    ok(hr == HRESULT_FROM_WIN32(ERROR_RESOURCE_TYPE_NOT_FOUND),
       "Expected HRESULT_FROM_WIN32(ERROR_RESOURCE_TYPE_NOT_FOUND), got %x\n", hr);

    /* non-existing resource name */
    hr = pLoadIconMetric(hinst, L"Nonexisting", LIM_LARGE, &icon);
    ok(hr == HRESULT_FROM_WIN32(ERROR_RESOURCE_TYPE_NOT_FOUND),
       "Expected HRESULT_FROM_WIN32(ERROR_RESOURCE_TYPE_NOT_FOUND), got %x\n", hr);

    hr = pLoadIconWithScaleDown(hinst, L"Noneexisting", 32, 32, &icon);
    ok(hr == HRESULT_FROM_WIN32(ERROR_RESOURCE_TYPE_NOT_FOUND),
       "Expected HRESULT_FROM_WIN32(ERROR_RESOURCE_TYPE_NOT_FOUND), got %x\n", hr);

    /* load icon using predefined identifier */
    hr = pLoadIconMetric(NULL, (LPWSTR)IDI_APPLICATION, LIM_SMALL, &icon);
    ok(hr == S_OK, "Expected S_OK, got %x\n", hr);
    res = GetIconInfo(icon, &info);
    ok(res, "Failed to get icon info, error %u\n", GetLastError());
    bytes = GetObjectA(info.hbmColor, sizeof(bmp), &bmp);
    ok(bytes > 0, "Failed to get bitmap info for icon\n");
    ok(bmp.bmWidth  == GetSystemMetrics(SM_CXSMICON), "Wrong icon width %d\n", bmp.bmWidth);
    ok(bmp.bmHeight == GetSystemMetrics(SM_CYSMICON), "Wrong icon height %d\n", bmp.bmHeight);
    DestroyIcon(icon);

    hr = pLoadIconMetric(NULL, (LPWSTR)IDI_APPLICATION, LIM_LARGE, &icon);
    ok(hr == S_OK, "Expected S_OK, got %x\n", hr);
    res = GetIconInfo(icon, &info);
    ok(res, "Failed to get icon info, error %u\n", GetLastError());
    bytes = GetObjectA(info.hbmColor, sizeof(bmp), &bmp);
    ok(bytes > 0, "Failed to get bitmap info for icon\n");
    ok(bmp.bmWidth  == GetSystemMetrics(SM_CXICON), "Wrong icon width %d\n", bmp.bmWidth);
    ok(bmp.bmHeight == GetSystemMetrics(SM_CYICON), "Wrong icon height %d\n", bmp.bmHeight);
    DestroyIcon(icon);

    hr = pLoadIconWithScaleDown(NULL, (LPWSTR)IDI_APPLICATION, 42, 42, &icon);
    ok(hr == S_OK, "Expected S_OK, got %x\n", hr);
    res = GetIconInfo(icon, &info);
    ok(res, "Failed to get icon info, error %u\n", GetLastError());
    bytes = GetObjectA(info.hbmColor, sizeof(bmp), &bmp);
    ok(bytes > 0, "Failed to get bitmap info for icon\n");
    ok(bmp.bmWidth  == 42, "Wrong icon width %d\n", bmp.bmWidth);
    ok(bmp.bmHeight == 42, "Wrong icon height %d\n", bmp.bmHeight);
    DestroyIcon(icon);

    /* load icon from file */
    hr = pLoadIconMetric(NULL, icon_path, LIM_SMALL, &icon);
    ok(hr == S_OK, "Expected S_OK, got %x\n", hr);
    res = GetIconInfo(icon, &info);
    ok(res, "Failed to get icon info, error %u\n", GetLastError());
    bytes = GetObjectA(info.hbmColor, sizeof(bmp), &bmp);
    ok(bytes > 0, "Failed to get bitmap info for icon\n");
    ok(bmp.bmWidth  == GetSystemMetrics(SM_CXSMICON), "Wrong icon width %d\n", bmp.bmWidth);
    ok(bmp.bmHeight == GetSystemMetrics(SM_CYSMICON), "Wrong icon height %d\n", bmp.bmHeight);
    DestroyIcon(icon);

    hr = pLoadIconWithScaleDown(NULL, icon_path, 42, 42, &icon);
    ok(hr == S_OK, "Expected S_OK, got %x\n", hr);
    res = GetIconInfo(icon, &info);
    ok(res, "Failed to get icon info, error %u\n", GetLastError());
    bytes = GetObjectA(info.hbmColor, sizeof(bmp), &bmp);
    ok(bytes > 0, "Failed to get bitmap info for icon\n");
    ok(bmp.bmWidth  == 42, "Wrong icon width %d\n", bmp.bmWidth);
    ok(bmp.bmHeight == 42, "Wrong icon height %d\n", bmp.bmHeight);
    DestroyIcon(icon);

    DeleteFileW(icon_path);
    FreeLibrary(hinst);
}

static void check_class( const char *name, int must_exist, UINT style, UINT ignore, BOOL v6 )
{
    WNDCLASSA wc;

    if (GetClassInfoA( 0, name, &wc ))
    {
        char buff[64];
        HWND hwnd;

        todo_wine_if(!strcmp(name, "SysLink") && !must_exist && !v6)
        ok( must_exist, "System class %s should %sexist\n", name, must_exist ? "" : "NOT " );
        if (!must_exist) return;

        todo_wine_if(!strcmp(name, "ScrollBar") || (!strcmp(name, "tooltips_class32") && v6))
        ok( !(~wc.style & style & ~ignore), "System class %s is missing bits %x (%08x/%08x)\n",
            name, ~wc.style & style, wc.style, style );
        todo_wine_if((!strcmp(name, "tooltips_class32") && v6) || !strcmp(name, "SysLink"))
        ok( !(wc.style & ~style), "System class %s has extra bits %x (%08x/%08x)\n",
            name, wc.style & ~style, wc.style, style );
        ok( !wc.hInstance, "System class %s has hInstance %p\n", name, wc.hInstance );

        hwnd = CreateWindowA(name, 0, 0, 0, 0, 0, 0, 0, NULL, GetModuleHandleA(NULL), 0);
        ok( hwnd != NULL, "Failed to create window for class %s.\n", name );
        GetClassNameA(hwnd, buff, ARRAY_SIZE(buff));
        ok( !strcmp(name, buff), "Unexpected class name %s, expected %s.\n", buff, name );
        DestroyWindow(hwnd);
    }
    else
        ok( !must_exist, "System class %s does not exist\n", name );
}

/* test styles of system classes */
static void test_builtin_classes(void)
{
    /* check style bits */
    check_class( "Button",     1, CS_PARENTDC | CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW | CS_GLOBALCLASS, 0, FALSE );
    check_class( "ComboBox",   1, CS_PARENTDC | CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW | CS_GLOBALCLASS, 0, FALSE );
    check_class( "Edit",       1, CS_PARENTDC | CS_DBLCLKS | CS_GLOBALCLASS, 0, FALSE );
    check_class( "ListBox",    1, CS_PARENTDC | CS_DBLCLKS | CS_GLOBALCLASS, 0, FALSE );
    check_class( "ScrollBar",  1, CS_PARENTDC | CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW | CS_GLOBALCLASS, 0, FALSE );
    check_class( "Static",     1, CS_PARENTDC | CS_DBLCLKS | CS_GLOBALCLASS, 0, FALSE );
    check_class( "ComboLBox",  1, CS_SAVEBITS | CS_DBLCLKS | CS_DROPSHADOW | CS_GLOBALCLASS, CS_DROPSHADOW, FALSE );
}

static void test_comctl32_classes(BOOL v6)
{
    check_class(ANIMATE_CLASSA,      1, CS_DBLCLKS | CS_GLOBALCLASS, 0, FALSE);
    check_class(WC_COMBOBOXEXA,      1, CS_GLOBALCLASS, 0, FALSE);
    check_class(DATETIMEPICK_CLASSA, 1, CS_GLOBALCLASS, 0, FALSE);
    check_class(WC_HEADERA,          1, CS_DBLCLKS | CS_GLOBALCLASS, 0, FALSE);
    check_class(HOTKEY_CLASSA,       1, CS_GLOBALCLASS, 0, FALSE);
    check_class(WC_IPADDRESSA,       1, CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW | CS_GLOBALCLASS, 0, FALSE);
    check_class(WC_LISTVIEWA,        1, CS_DBLCLKS | CS_GLOBALCLASS, 0, FALSE);
    check_class(MONTHCAL_CLASSA,     1, CS_GLOBALCLASS, 0, FALSE);
    check_class(WC_NATIVEFONTCTLA,   1, CS_GLOBALCLASS, 0, FALSE);
    check_class(WC_PAGESCROLLERA,    1, CS_GLOBALCLASS, 0, FALSE);
    check_class(PROGRESS_CLASSA,     1, CS_HREDRAW | CS_VREDRAW | CS_GLOBALCLASS, 0, FALSE);
    check_class(REBARCLASSNAMEA,     1, CS_DBLCLKS | CS_GLOBALCLASS, 0, FALSE);
    check_class(STATUSCLASSNAMEA,    1, CS_DBLCLKS | CS_VREDRAW | CS_GLOBALCLASS, 0, FALSE);
    check_class(WC_TABCONTROLA,      1, CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW | CS_GLOBALCLASS, 0, FALSE);
    check_class(TOOLBARCLASSNAMEA,   1, CS_DBLCLKS | CS_GLOBALCLASS, 0, FALSE);
    if (v6)
        check_class(TOOLTIPS_CLASSA, 1, CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW | CS_GLOBALCLASS | CS_DROPSHADOW, CS_SAVEBITS | CS_HREDRAW | CS_VREDRAW /* XP */, TRUE);
    else
        check_class(TOOLTIPS_CLASSA, 1, CS_DBLCLKS | CS_GLOBALCLASS | CS_SAVEBITS, CS_HREDRAW | CS_VREDRAW /* XP */, FALSE);
    check_class(TRACKBAR_CLASSA,     1, CS_GLOBALCLASS, 0, FALSE);
    check_class(WC_TREEVIEWA,        1, CS_DBLCLKS | CS_GLOBALCLASS, 0, FALSE);
    check_class(UPDOWN_CLASSA,       1, CS_HREDRAW | CS_VREDRAW | CS_GLOBALCLASS, 0, FALSE);
    check_class("SysLink", v6, CS_GLOBALCLASS, 0, FALSE);
}

struct wm_themechanged_test
{
    const char *class;
    const struct message *expected_msg;
    int ignored_msg_count;
    DWORD ignored_msgs[16];
    BOOL todo;
};

static BOOL ignore_message(UINT msg)
{
    /* these are always ignored */
    return (msg >= 0xc000 ||
            msg == WM_GETICON ||
            msg == WM_GETOBJECT ||
            msg == WM_TIMECHANGE ||
            msg == WM_DISPLAYCHANGE ||
            msg == WM_DEVICECHANGE ||
            msg == WM_DWMNCRENDERINGCHANGED ||
            msg == WM_WININICHANGE ||
            msg == WM_CHILDACTIVATE);
}

static LRESULT CALLBACK test_wm_themechanged_proc(HWND hwnd, UINT message, WPARAM wParam,
                                                  LPARAM lParam, UINT_PTR id, DWORD_PTR ref_data)
{
    const struct wm_themechanged_test *test = (const struct wm_themechanged_test *)ref_data;
    static int defwndproc_counter = 0;
    struct message msg = {0};
    LRESULT ret;
    int i;

    if (ignore_message(message))
        return pDefSubclassProc(hwnd, message, wParam, lParam);

    /* Extra messages to be ignored for a test case */
    for (i = 0; i < test->ignored_msg_count; ++i)
    {
        if (message == test->ignored_msgs[i])
            return pDefSubclassProc(hwnd, message, wParam, lParam);
    }

    msg.message = message;
    msg.flags = sent | wparam | lparam;
    if (defwndproc_counter)
        msg.flags |= defwinproc;
    msg.wParam = wParam;
    msg.lParam = lParam;
    add_message(sequences, CHILD_SEQ_INDEX, &msg);

    if (message == WM_NCDESTROY)
        pRemoveWindowSubclass(hwnd, test_wm_themechanged_proc, 0);

    ++defwndproc_counter;
    ret = pDefSubclassProc(hwnd, message, wParam, lParam);
    --defwndproc_counter;

    return ret;
}

static HWND create_control(const char *class, DWORD style, HWND parent, DWORD_PTR data)
{
    HWND hwnd;

    if (parent)
        style |= WS_CHILD;
    hwnd = CreateWindowExA(0, class, "test", style, 0, 0, 50, 20, parent, 0, 0, NULL);
    ok(!!hwnd, "Failed to create %s style %#x parent %p\n", class, style, parent);
    pSetWindowSubclass(hwnd, test_wm_themechanged_proc, 0, data);
    return hwnd;
}

static const struct message wm_themechanged_paint_erase_seq[] =
{
    {WM_THEMECHANGED, sent | wparam | lparam},
    {WM_PAINT, sent | wparam | lparam},
    {WM_ERASEBKGND, sent | defwinproc},
    {0},
};

static const struct message wm_themechanged_paint_seq[] =
{
    {WM_THEMECHANGED, sent | wparam | lparam},
    {WM_PAINT, sent | wparam | lparam},
    {0},
};

static const struct message wm_themechanged_no_paint_seq[] =
{
    {WM_THEMECHANGED, sent | wparam | lparam},
    {0},
};

static void test_WM_THEMECHANGED(void)
{
    HWND parent, child;
    char buffer[64];
    int i;

    static const struct wm_themechanged_test tests[] =
    {
        {ANIMATE_CLASSA, wm_themechanged_no_paint_seq},
        {WC_BUTTONA, wm_themechanged_paint_erase_seq, 2, {WM_GETTEXT, WM_GETTEXTLENGTH}},
        {WC_COMBOBOXA, wm_themechanged_paint_erase_seq, 1, {WM_CTLCOLOREDIT}},
        {WC_COMBOBOXEXA, wm_themechanged_no_paint_seq},
        {DATETIMEPICK_CLASSA, wm_themechanged_paint_erase_seq},
        {WC_EDITA, wm_themechanged_paint_erase_seq, 7, {WM_GETTEXTLENGTH, WM_GETFONT, EM_GETSEL, EM_GETRECT, EM_CHARFROMPOS, EM_LINEFROMCHAR, EM_POSFROMCHAR}},
        {WC_HEADERA, wm_themechanged_paint_erase_seq},
        {HOTKEY_CLASSA, wm_themechanged_no_paint_seq},
        {WC_IPADDRESSA, wm_themechanged_paint_erase_seq, 1, {WM_CTLCOLOREDIT}},
        {WC_LISTBOXA, wm_themechanged_paint_erase_seq},
        {WC_LISTVIEWA, wm_themechanged_paint_erase_seq},
        {MONTHCAL_CLASSA, wm_themechanged_paint_erase_seq},
        {WC_NATIVEFONTCTLA, wm_themechanged_no_paint_seq},
        {WC_PAGESCROLLERA, wm_themechanged_paint_erase_seq},
        {PROGRESS_CLASSA, wm_themechanged_paint_erase_seq, 3, {WM_STYLECHANGING, WM_STYLECHANGED, WM_NCPAINT}},
        {REBARCLASSNAMEA, wm_themechanged_no_paint_seq, 1, {WM_WINDOWPOSCHANGING}},
        {WC_STATICA, wm_themechanged_paint_erase_seq, 2, {WM_GETTEXT, WM_GETTEXTLENGTH}},
        {STATUSCLASSNAMEA, wm_themechanged_paint_erase_seq},
        {"SysLink", wm_themechanged_no_paint_seq},
        {WC_TABCONTROLA, wm_themechanged_paint_erase_seq},
        {TOOLBARCLASSNAMEA, wm_themechanged_paint_erase_seq, 1, {WM_WINDOWPOSCHANGING}},
        {TOOLTIPS_CLASSA, wm_themechanged_no_paint_seq},
        {TRACKBAR_CLASSA, wm_themechanged_paint_seq},
        {WC_TREEVIEWA, wm_themechanged_paint_erase_seq, 1, {0x1128}},
        {UPDOWN_CLASSA, wm_themechanged_paint_erase_seq},
        {WC_SCROLLBARA, wm_themechanged_paint_erase_seq, 1, {SBM_GETSCROLLINFO}},
    };

    parent = CreateWindowExA(0, WC_STATICA, "parent", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100,
                             200, 200, 0, 0, 0, NULL);
    ok(!!parent, "Failed to create parent window\n");

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        child = create_control(tests[i].class, WS_VISIBLE, parent, (DWORD_PTR)&tests[i]);
        flush_events();
        flush_sequences(sequences, NUM_MSG_SEQUENCES);

        SendMessageW(child, WM_THEMECHANGED, 0, 0);
        flush_events();

        sprintf(buffer, "Test %d class %s WM_THEMECHANGED", i, tests[i].class);
        ok_sequence(sequences, CHILD_SEQ_INDEX, tests[i].expected_msg, buffer, tests[i].todo);
        DestroyWindow(child);
    }

    DestroyWindow(parent);
}

static const struct message wm_syscolorchange_seq[] =
{
    {WM_SYSCOLORCHANGE, sent | wparam | lparam},
    {0},
};

static INT_PTR CALLBACK wm_syscolorchange_dlg_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    struct message msg = {0};

    msg.message = message;
    msg.flags = sent | wparam | lparam;
    msg.wParam = wParam;
    msg.lParam = lParam;
    add_message(sequences, CHILD_SEQ_INDEX, &msg);
    return FALSE;
}

static void test_WM_SYSCOLORCHANGE(void)
{
    HWND parent, dialog;
    struct
    {
        DLGTEMPLATE tmplate;
        WORD menu;
        WORD class;
        WORD title;
    } temp = {{0}};

    parent = CreateWindowExA(0, WC_STATICA, "parent", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100,
                             200, 200, 0, 0, 0, NULL);
    ok(!!parent, "CreateWindowExA failed, error %d\n", GetLastError());

    temp.tmplate.style = WS_CHILD | WS_VISIBLE;
    temp.tmplate.cx = 50;
    temp.tmplate.cy = 50;
    dialog = CreateDialogIndirectParamA(NULL, &temp.tmplate, parent, wm_syscolorchange_dlg_proc, 0);
    ok(!!dialog, "CreateDialogIndirectParamA failed, error %d\n", GetLastError());
    flush_events();
    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    SendMessageW(dialog, WM_SYSCOLORCHANGE, 0, 0);
    ok_sequence(sequences, CHILD_SEQ_INDEX, wm_syscolorchange_seq, "test dialog WM_SYSCOLORCHANGE", FALSE);

    EndDialog(dialog, 0);
    DestroyWindow(parent);
}

START_TEST(misc)
{
    ULONG_PTR ctx_cookie;
    HANDLE hCtx;

    if(!InitFunctionPtrs())
        return;

    test_GetPtrAW();
    test_Alloc();
    test_comctl32_classes(FALSE);

    FreeLibrary(hComctl32);

    if (!load_v6_module(&ctx_cookie, &hCtx))
        return;
    if(!init_functions_v6())
        return;
    init_msg_sequences(sequences, NUM_MSG_SEQUENCES);

    test_comctl32_classes(TRUE);
    test_builtin_classes();
    test_LoadIconWithScaleDown();
    test_WM_THEMECHANGED();
    test_WM_SYSCOLORCHANGE();

    unload_v6_module(ctx_cookie, hCtx);
    FreeLibrary(hComctl32);
}
