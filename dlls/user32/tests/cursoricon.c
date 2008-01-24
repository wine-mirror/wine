/*
 * Unit test suite for cursors and icons.
 *
 * Copyright 2006 Michael Kaufmann
 * Copyright 2007 Dmitry Timoshkov
 * Copyright 2007 Andrew Riedi
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

#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "wingdi.h"
#include "winuser.h"

static char **test_argv;
static int test_argc;
static HWND child = 0;
static HWND parent = 0;
static HANDLE child_process;

#define PROC_INIT (WM_USER+1)

static LRESULT CALLBACK callback_child(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    BOOL ret;
    DWORD error;

    switch (msg)
    {
        /* Destroy the cursor. */
        case WM_USER+1:
            SetLastError(0xdeadbeef);
            ret = DestroyCursor((HCURSOR) lParam);
            error = GetLastError();
            todo_wine {
            ok(!ret, "DestroyCursor on the active cursor succeeded.\n");
            ok(error == ERROR_DESTROY_OBJECT_OF_OTHER_THREAD,
                "Last error: %u\n", error);
            }
            return TRUE;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

static LRESULT CALLBACK callback_parent(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == PROC_INIT)
    {
        child = (HWND) wParam;
        return TRUE;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

static void do_child(void)
{
    WNDCLASS class;
    MSG msg;
    BOOL ret;

    /* Register a new class. */
    class.style = CS_GLOBALCLASS;
    class.lpfnWndProc = callback_child;
    class.cbClsExtra = 0;
    class.cbWndExtra = 0;
    class.hInstance = GetModuleHandle(NULL);
    class.hIcon = NULL;
    class.hCursor = NULL;
    class.hbrBackground = NULL;
    class.lpszMenuName = NULL;
    class.lpszClassName = "cursor_child";

    SetLastError(0xdeadbeef);
    ret = RegisterClass(&class);
    ok(ret, "Failed to register window class.  Error: %u\n", GetLastError());

    /* Create a window. */
    child = CreateWindowA("cursor_child", "cursor_child", WS_POPUP | WS_VISIBLE,
        0, 0, 200, 200, 0, 0, 0, NULL);
    ok(child != 0, "CreateWindowA failed.  Error: %u\n", GetLastError());

    /* Let the parent know our HWND. */
    PostMessage(parent, PROC_INIT, (WPARAM) child, 0);

    /* Receive messages. */
    while ((ret = GetMessage(&msg, child, 0, 0)))
    {
        ok(ret != -1, "GetMessage failed.  Error: %u\n", GetLastError());
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

static void do_parent(void)
{
    char path_name[MAX_PATH];
    PROCESS_INFORMATION info;
    STARTUPINFOA startup;
    WNDCLASS class;
    MSG msg;
    BOOL ret;

    /* Register a new class. */
    class.style = CS_GLOBALCLASS;
    class.lpfnWndProc = callback_parent;
    class.cbClsExtra = 0;
    class.cbWndExtra = 0;
    class.hInstance = GetModuleHandle(NULL);
    class.hIcon = NULL;
    class.hCursor = NULL;
    class.hbrBackground = NULL;
    class.lpszMenuName = NULL;
    class.lpszClassName = "cursor_parent";

    SetLastError(0xdeadbeef);
    ret = RegisterClass(&class);
    ok(ret, "Failed to register window class.  Error: %u\n", GetLastError());

    /* Create a window. */
    parent = CreateWindowA("cursor_parent", "cursor_parent", WS_POPUP | WS_VISIBLE,
        0, 0, 200, 200, 0, 0, 0, NULL);
    ok(parent != 0, "CreateWindowA failed.  Error: %u\n", GetLastError());

    /* Start child process. */
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;

    sprintf(path_name, "%s cursoricon %x", test_argv[0], (unsigned int) parent);
    ok(CreateProcessA(NULL, path_name, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info), "CreateProcess failed.\n");
    child_process = info.hProcess;

    /* Wait for child window handle. */
    while ((child == 0) && (ret = GetMessage(&msg, parent, 0, 0)))
    {
        ok(ret != -1, "GetMessage failed.  Error: %u\n", GetLastError());
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

static void finish_child_process(void)
{
    DWORD exit_code;
    BOOL ret;

    SendMessage(child, WM_CLOSE, 0, 0);
    ok(WaitForSingleObject(child_process, 30000) == WAIT_OBJECT_0, "Child process termination failed.\n");

    ret = GetExitCodeProcess(child_process, &exit_code);
    ok(ret, "GetExitCodeProcess() failed.  Error: %u\n", GetLastError());
    ok(exit_code == 0, "Exit code == %u.\n", exit_code);

    CloseHandle(child_process);
}

static void test_child_process(void)
{
    static const BYTE bmp_bits[4096];
    HCURSOR cursor;
    ICONINFO cursorInfo;
    UINT display_bpp;
    HDC hdc;

    /* Create and set a dummy cursor. */
    hdc = GetDC(0);
    display_bpp = GetDeviceCaps(hdc, BITSPIXEL);
    ReleaseDC(0, hdc);

    cursorInfo.fIcon = FALSE;
    cursorInfo.xHotspot = 0;
    cursorInfo.yHotspot = 0;
    cursorInfo.hbmMask = CreateBitmap(32, 32, 1, 1, bmp_bits);
    cursorInfo.hbmColor = CreateBitmap(32, 32, 1, display_bpp, bmp_bits);

    cursor = CreateIconIndirect(&cursorInfo);
    ok(cursor != NULL, "CreateIconIndirect returned %p.\n", cursor);

    SetCursor(cursor);

    /* Destroy the cursor. */
    SendMessage(child, WM_USER+1, 0, (LPARAM) cursor);
}

static void test_CopyImage_Check(HBITMAP bitmap, UINT flags, INT copyWidth, INT copyHeight,
                                  INT expectedWidth, INT expectedHeight, WORD expectedDepth, BOOL dibExpected)
{
    HBITMAP copy;
    BITMAP origBitmap;
    BITMAP copyBitmap;
    BOOL orig_is_dib;
    BOOL copy_is_dib;

    copy = (HBITMAP) CopyImage(bitmap, IMAGE_BITMAP, copyWidth, copyHeight, flags);
    ok(copy != NULL, "CopyImage() failed\n");
    if (copy != NULL)
    {
        GetObject(bitmap, sizeof(origBitmap), &origBitmap);
        GetObject(copy, sizeof(copyBitmap), &copyBitmap);
        orig_is_dib = (origBitmap.bmBits != NULL);
        copy_is_dib = (copyBitmap.bmBits != NULL);

        if (copy_is_dib && dibExpected
            && copyBitmap.bmBitsPixel == 24
            && (expectedDepth == 16 || expectedDepth == 32))
        {
            /* Windows 95 doesn't create DIBs with a depth of 16 or 32 bit */
            if (GetVersion() & 0x80000000)
            {
                expectedDepth = 24;
            }
        }

        if (copy_is_dib && !dibExpected && !(flags & LR_CREATEDIBSECTION))
        {
            /* It's not forbidden to create a DIB section if the flag
               LR_CREATEDIBSECTION is absent.
               Windows 9x does this if the bitmap has a depth that doesn't
               match the screen depth, Windows NT doesn't */
            dibExpected = TRUE;
            expectedDepth = origBitmap.bmBitsPixel;
        }

        ok((!(dibExpected ^ copy_is_dib)
             && (copyBitmap.bmWidth == expectedWidth)
             && (copyBitmap.bmHeight == expectedHeight)
             && (copyBitmap.bmBitsPixel == expectedDepth)),
             "CopyImage ((%s, %dx%d, %u bpp), %d, %d, %#x): Expected (%s, %dx%d, %u bpp), got (%s, %dx%d, %u bpp)\n",
                  orig_is_dib ? "DIB" : "DDB", origBitmap.bmWidth, origBitmap.bmHeight, origBitmap.bmBitsPixel,
                  copyWidth, copyHeight, flags,
                  dibExpected ? "DIB" : "DDB", expectedWidth, expectedHeight, expectedDepth,
                  copy_is_dib ? "DIB" : "DDB", copyBitmap.bmWidth, copyBitmap.bmHeight, copyBitmap.bmBitsPixel);

        DeleteObject(copy);
    }
}

static void test_CopyImage_Bitmap(int depth)
{
    HBITMAP ddb, dib;
    HDC screenDC;
    BITMAPINFO * info;
    VOID * bits;
    int screen_depth;
    unsigned int i;

    /* Create a device-independent bitmap (DIB) */
    info = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD));
    info->bmiHeader.biSize = sizeof(info->bmiHeader);
    info->bmiHeader.biWidth = 2;
    info->bmiHeader.biHeight = 2;
    info->bmiHeader.biPlanes = 1;
    info->bmiHeader.biBitCount = depth;
    info->bmiHeader.biCompression = BI_RGB;

    for (i=0; i < 256; i++)
    {
        info->bmiColors[i].rgbRed = i;
        info->bmiColors[i].rgbGreen = i;
        info->bmiColors[i].rgbBlue = 255 - i;
        info->bmiColors[i].rgbReserved = 0;
    }

    dib = CreateDIBSection(NULL, info, DIB_RGB_COLORS, &bits, NULL, 0);

    /* Create a device-dependent bitmap (DDB) */
    screenDC = GetDC(NULL);
    screen_depth = GetDeviceCaps(screenDC, BITSPIXEL);
    if (depth == 1 || depth == screen_depth)
    {
        ddb = CreateBitmap(2, 2, 1, depth, NULL);
    }
    else
    {
        ddb = NULL;
    }
    ReleaseDC(NULL, screenDC);

    if (ddb != NULL)
    {
        test_CopyImage_Check(ddb, 0, 0, 0, 2, 2, depth == 1 ? 1 : screen_depth, FALSE);
        test_CopyImage_Check(ddb, 0, 0, 5, 2, 5, depth == 1 ? 1 : screen_depth, FALSE);
        test_CopyImage_Check(ddb, 0, 5, 0, 5, 2, depth == 1 ? 1 : screen_depth, FALSE);
        test_CopyImage_Check(ddb, 0, 5, 5, 5, 5, depth == 1 ? 1 : screen_depth, FALSE);

        test_CopyImage_Check(ddb, LR_MONOCHROME, 0, 0, 2, 2, 1, FALSE);
        test_CopyImage_Check(ddb, LR_MONOCHROME, 5, 0, 5, 2, 1, FALSE);
        test_CopyImage_Check(ddb, LR_MONOCHROME, 0, 5, 2, 5, 1, FALSE);
        test_CopyImage_Check(ddb, LR_MONOCHROME, 5, 5, 5, 5, 1, FALSE);

        test_CopyImage_Check(ddb, LR_CREATEDIBSECTION, 0, 0, 2, 2, depth, TRUE);
        test_CopyImage_Check(ddb, LR_CREATEDIBSECTION, 5, 0, 5, 2, depth, TRUE);
        test_CopyImage_Check(ddb, LR_CREATEDIBSECTION, 0, 5, 2, 5, depth, TRUE);
        test_CopyImage_Check(ddb, LR_CREATEDIBSECTION, 5, 5, 5, 5, depth, TRUE);

        /* LR_MONOCHROME is ignored if LR_CREATEDIBSECTION is present */
        test_CopyImage_Check(ddb, LR_MONOCHROME | LR_CREATEDIBSECTION, 0, 0, 2, 2, depth, TRUE);
        test_CopyImage_Check(ddb, LR_MONOCHROME | LR_CREATEDIBSECTION, 5, 0, 5, 2, depth, TRUE);
        test_CopyImage_Check(ddb, LR_MONOCHROME | LR_CREATEDIBSECTION, 0, 5, 2, 5, depth, TRUE);
        test_CopyImage_Check(ddb, LR_MONOCHROME | LR_CREATEDIBSECTION, 5, 5, 5, 5, depth, TRUE);

        DeleteObject(ddb);
    }

    if (depth != 1)
    {
        test_CopyImage_Check(dib, 0, 0, 0, 2, 2, screen_depth, FALSE);
        test_CopyImage_Check(dib, 0, 5, 0, 5, 2, screen_depth, FALSE);
        test_CopyImage_Check(dib, 0, 0, 5, 2, 5, screen_depth, FALSE);
        test_CopyImage_Check(dib, 0, 5, 5, 5, 5, screen_depth, FALSE);
    }

    test_CopyImage_Check(dib, LR_MONOCHROME, 0, 0, 2, 2, 1, FALSE);
    test_CopyImage_Check(dib, LR_MONOCHROME, 5, 0, 5, 2, 1, FALSE);
    test_CopyImage_Check(dib, LR_MONOCHROME, 0, 5, 2, 5, 1, FALSE);
    test_CopyImage_Check(dib, LR_MONOCHROME, 5, 5, 5, 5, 1, FALSE);

    test_CopyImage_Check(dib, LR_CREATEDIBSECTION, 0, 0, 2, 2, depth, TRUE);
    test_CopyImage_Check(dib, LR_CREATEDIBSECTION, 5, 0, 5, 2, depth, TRUE);
    test_CopyImage_Check(dib, LR_CREATEDIBSECTION, 0, 5, 2, 5, depth, TRUE);
    test_CopyImage_Check(dib, LR_CREATEDIBSECTION, 5, 5, 5, 5, depth, TRUE);

    /* LR_MONOCHROME is ignored if LR_CREATEDIBSECTION is present */
    test_CopyImage_Check(dib, LR_MONOCHROME | LR_CREATEDIBSECTION, 0, 0, 2, 2, depth, TRUE);
    test_CopyImage_Check(dib, LR_MONOCHROME | LR_CREATEDIBSECTION, 5, 0, 5, 2, depth, TRUE);
    test_CopyImage_Check(dib, LR_MONOCHROME | LR_CREATEDIBSECTION, 0, 5, 2, 5, depth, TRUE);
    test_CopyImage_Check(dib, LR_MONOCHROME | LR_CREATEDIBSECTION, 5, 5, 5, 5, depth, TRUE);

    DeleteObject(dib);

    if (depth == 1)
    {
        /* Special case: A monochrome DIB is converted to a monochrome DDB if
           the colors in the color table are black and white.

           Skip this test on Windows 95, it always creates a monochrome DDB
           in this case */

        if (!(GetVersion() & 0x80000000))
        {
            info->bmiHeader.biBitCount = 1;
            info->bmiColors[0].rgbRed = 0xFF;
            info->bmiColors[0].rgbGreen = 0;
            info->bmiColors[0].rgbBlue = 0;
            info->bmiColors[1].rgbRed = 0;
            info->bmiColors[1].rgbGreen = 0xFF;
            info->bmiColors[1].rgbBlue = 0;

            dib = CreateDIBSection(NULL, info, DIB_RGB_COLORS, &bits, NULL, 0);
            test_CopyImage_Check(dib, 0, 0, 0, 2, 2, screen_depth, FALSE);
            test_CopyImage_Check(dib, 0, 5, 0, 5, 2, screen_depth, FALSE);
            test_CopyImage_Check(dib, 0, 0, 5, 2, 5, screen_depth, FALSE);
            test_CopyImage_Check(dib, 0, 5, 5, 5, 5, screen_depth, FALSE);
            DeleteObject(dib);

            info->bmiHeader.biBitCount = 1;
            info->bmiColors[0].rgbRed = 0;
            info->bmiColors[0].rgbGreen = 0;
            info->bmiColors[0].rgbBlue = 0;
            info->bmiColors[1].rgbRed = 0xFF;
            info->bmiColors[1].rgbGreen = 0xFF;
            info->bmiColors[1].rgbBlue = 0xFF;

            dib = CreateDIBSection(NULL, info, DIB_RGB_COLORS, &bits, NULL, 0);
            test_CopyImage_Check(dib, 0, 0, 0, 2, 2, 1, FALSE);
            test_CopyImage_Check(dib, 0, 5, 0, 5, 2, 1, FALSE);
            test_CopyImage_Check(dib, 0, 0, 5, 2, 5, 1, FALSE);
            test_CopyImage_Check(dib, 0, 5, 5, 5, 5, 1, FALSE);
            DeleteObject(dib);

            info->bmiHeader.biBitCount = 1;
            info->bmiColors[0].rgbRed = 0xFF;
            info->bmiColors[0].rgbGreen = 0xFF;
            info->bmiColors[0].rgbBlue = 0xFF;
            info->bmiColors[1].rgbRed = 0;
            info->bmiColors[1].rgbGreen = 0;
            info->bmiColors[1].rgbBlue = 0;

            dib = CreateDIBSection(NULL, info, DIB_RGB_COLORS, &bits, NULL, 0);
            test_CopyImage_Check(dib, 0, 0, 0, 2, 2, 1, FALSE);
            test_CopyImage_Check(dib, 0, 5, 0, 5, 2, 1, FALSE);
            test_CopyImage_Check(dib, 0, 0, 5, 2, 5, 1, FALSE);
            test_CopyImage_Check(dib, 0, 5, 5, 5, 5, 1, FALSE);
            DeleteObject(dib);
        }
    }

    HeapFree(GetProcessHeap(), 0, info);
}

static void test_initial_cursor(void)
{
    HCURSOR cursor, cursor2;
    DWORD error;

    cursor = GetCursor();

    /* Check what handle GetCursor() returns if a cursor is not set yet. */
    SetLastError(0xdeadbeef);
    cursor2 = LoadCursor(NULL, IDC_WAIT);
    todo_wine {
        ok(cursor == cursor2, "cursor (%p) is not IDC_WAIT (%p).\n", cursor, cursor2);
    }
    error = GetLastError();
    ok(error == 0xdeadbeef, "Last error: 0x%08x\n", error);
}

static void test_icon_info_dbg(HICON hIcon, UINT exp_cx, UINT exp_cy, UINT exp_bpp, int line)
{
    ICONINFO info;
    DWORD ret;
    BITMAP bmMask, bmColor;

    ret = GetIconInfo(hIcon, &info);
    ok_(__FILE__, line)(ret, "GetIconInfo failed\n");

    /* CreateIcon under XP causes info.fIcon to be 0 */
    ok_(__FILE__, line)(info.xHotspot == exp_cx/2, "info.xHotspot = %u\n", info.xHotspot);
    ok_(__FILE__, line)(info.yHotspot == exp_cy/2, "info.yHotspot = %u\n", info.yHotspot);
    ok_(__FILE__, line)(info.hbmMask != 0, "info.hbmMask is NULL\n");

    ret = GetObject(info.hbmMask, sizeof(bmMask), &bmMask);
    ok_(__FILE__, line)(ret == sizeof(bmMask), "GetObject(info.hbmMask) failed, ret %u\n", ret);

    if (exp_bpp == 1)
        ok_(__FILE__, line)(info.hbmColor == 0, "info.hbmColor should be NULL\n");

    if (info.hbmColor)
    {
        HDC hdc;
        UINT display_bpp;

        hdc = GetDC(0);
        display_bpp = GetDeviceCaps(hdc, BITSPIXEL);
        ReleaseDC(0, hdc);

        ret = GetObject(info.hbmColor, sizeof(bmColor), &bmColor);
        ok_(__FILE__, line)(ret == sizeof(bmColor), "GetObject(info.hbmColor) failed, ret %u\n", ret);

        ok_(__FILE__, line)(bmColor.bmBitsPixel == display_bpp /* XP */ ||
           bmColor.bmBitsPixel == exp_bpp /* Win98 */,
           "bmColor.bmBitsPixel = %d\n", bmColor.bmBitsPixel);
        ok_(__FILE__, line)(bmColor.bmWidth == exp_cx, "bmColor.bmWidth = %d\n", bmColor.bmWidth);
        ok_(__FILE__, line)(bmColor.bmHeight == exp_cy, "bmColor.bmHeight = %d\n", bmColor.bmHeight);

        ok_(__FILE__, line)(bmMask.bmBitsPixel == 1, "bmMask.bmBitsPixel = %d\n", bmMask.bmBitsPixel);
        ok_(__FILE__, line)(bmMask.bmWidth == exp_cx, "bmMask.bmWidth = %d\n", bmMask.bmWidth);
        ok_(__FILE__, line)(bmMask.bmHeight == exp_cy, "bmMask.bmHeight = %d\n", bmMask.bmHeight);
    }
    else
    {
        ok_(__FILE__, line)(bmMask.bmBitsPixel == 1, "bmMask.bmBitsPixel = %d\n", bmMask.bmBitsPixel);
        ok_(__FILE__, line)(bmMask.bmWidth == exp_cx, "bmMask.bmWidth = %d\n", bmMask.bmWidth);
        ok_(__FILE__, line)(bmMask.bmHeight == exp_cy * 2, "bmMask.bmHeight = %d\n", bmMask.bmHeight);
    }
}

#define test_icon_info(a,b,c,d) test_icon_info_dbg((a),(b),(c),(d),__LINE__)

static void test_CreateIcon(void)
{
    static const BYTE bmp_bits[1024];
    HICON hIcon;
    HBITMAP hbmMask, hbmColor;
    ICONINFO info;
    HDC hdc;
    UINT display_bpp;

    hdc = GetDC(0);
    display_bpp = GetDeviceCaps(hdc, BITSPIXEL);
    ReleaseDC(0, hdc);

    /* these crash under XP
    hIcon = CreateIcon(0, 16, 16, 1, 1, bmp_bits, NULL);
    hIcon = CreateIcon(0, 16, 16, 1, 1, NULL, bmp_bits);
    */

    hIcon = CreateIcon(0, 16, 16, 1, 1, bmp_bits, bmp_bits);
    ok(hIcon != 0, "CreateIcon failed\n");
    test_icon_info(hIcon, 16, 16, 1);
    DestroyIcon(hIcon);

    hIcon = CreateIcon(0, 16, 16, 1, display_bpp, bmp_bits, bmp_bits);
    ok(hIcon != 0, "CreateIcon failed\n");
    test_icon_info(hIcon, 16, 16, display_bpp);
    DestroyIcon(hIcon);

    hbmMask = CreateBitmap(16, 16, 1, 1, bmp_bits);
    ok(hbmMask != 0, "CreateBitmap failed\n");
    hbmColor = CreateBitmap(16, 16, 1, display_bpp, bmp_bits);
    ok(hbmColor != 0, "CreateBitmap failed\n");

    info.fIcon = TRUE;
    info.xHotspot = 8;
    info.yHotspot = 8;
    info.hbmMask = 0;
    info.hbmColor = 0;
    SetLastError(0xdeadbeaf);
    hIcon = CreateIconIndirect(&info);
    ok(!hIcon, "CreateIconIndirect should fail\n");
    ok(GetLastError() == 0xdeadbeaf, "wrong error %u\n", GetLastError());

    info.fIcon = TRUE;
    info.xHotspot = 8;
    info.yHotspot = 8;
    info.hbmMask = 0;
    info.hbmColor = hbmColor;
    SetLastError(0xdeadbeaf);
    hIcon = CreateIconIndirect(&info);
    ok(!hIcon, "CreateIconIndirect should fail\n");
    ok(GetLastError() == 0xdeadbeaf, "wrong error %u\n", GetLastError());

    info.fIcon = TRUE;
    info.xHotspot = 8;
    info.yHotspot = 8;
    info.hbmMask = hbmMask;
    info.hbmColor = hbmColor;
    hIcon = CreateIconIndirect(&info);
    ok(hIcon != 0, "CreateIconIndirect failed\n");
    test_icon_info(hIcon, 16, 16, display_bpp);
    DestroyIcon(hIcon);

    DeleteObject(hbmMask);
    DeleteObject(hbmColor);

    hbmMask = CreateBitmap(16, 32, 1, 1, bmp_bits);
    ok(hbmMask != 0, "CreateBitmap failed\n");

    info.fIcon = TRUE;
    info.xHotspot = 8;
    info.yHotspot = 8;
    info.hbmMask = hbmMask;
    info.hbmColor = 0;
    SetLastError(0xdeadbeaf);
    hIcon = CreateIconIndirect(&info);
    ok(hIcon != 0, "CreateIconIndirect failed\n");
    test_icon_info(hIcon, 16, 16, 1);
    DestroyIcon(hIcon);

    DeleteObject(hbmMask);
    DeleteObject(hbmColor);
}

static void test_DestroyCursor(void)
{
    static const BYTE bmp_bits[4096];
    ICONINFO cursorInfo;
    HCURSOR cursor, cursor2;
    BOOL ret;
    DWORD error;
    UINT display_bpp;
    HDC hdc;

    hdc = GetDC(0);
    display_bpp = GetDeviceCaps(hdc, BITSPIXEL);
    ReleaseDC(0, hdc);

    cursorInfo.fIcon = FALSE;
    cursorInfo.xHotspot = 0;
    cursorInfo.yHotspot = 0;
    cursorInfo.hbmMask = CreateBitmap(32, 32, 1, 1, bmp_bits);
    cursorInfo.hbmColor = CreateBitmap(32, 32, 1, display_bpp, bmp_bits);

    cursor = CreateIconIndirect(&cursorInfo);
    ok(cursor != NULL, "CreateIconIndirect returned %p\n", cursor);
    if(!cursor) {
        return;
    }
    SetCursor(cursor);

    SetLastError(0xdeadbeef);
    ret = DestroyCursor(cursor);
    ok(!ret, "DestroyCursor on the active cursor succeeded\n");
    error = GetLastError();
    ok(error == 0xdeadbeef, "Last error: %u\n", error);

    cursor2 = GetCursor();
    ok(cursor2 == cursor, "Active was set to %p when trying to destroy it\n", cursor2);

    SetCursor(NULL);

    /* Trying to destroy the cursor properly fails now with
     * ERROR_INVALID_CURSOR_HANDLE.  This happens because we called
     * DestroyCursor() 2+ times after calling SetCursor().  The calls to
     * GetCursor() and SetCursor(NULL) in between make no difference. */
    ret = DestroyCursor(cursor);
    todo_wine {
        ok(!ret, "DestroyCursor succeeded.\n");
        error = GetLastError();
        ok(error == ERROR_INVALID_CURSOR_HANDLE, "Last error: 0x%08x\n", error);
    }

    DeleteObject(cursorInfo.hbmMask);
    DeleteObject(cursorInfo.hbmColor);

    /* Try testing DestroyCursor() now using LoadCursor() cursors. */
    cursor = LoadCursor(NULL, IDC_ARROW);

    SetLastError(0xdeadbeef);
    ret = DestroyCursor(cursor);
    ok(ret, "DestroyCursor on the active cursor failed.\n");
    error = GetLastError();
    ok(error == 0xdeadbeef, "Last error: 0x%08x\n", error);

    /* Try setting the cursor to a destroyed OEM cursor. */
    SetLastError(0xdeadbeef);
    SetCursor(cursor);
    error = GetLastError();
    todo_wine {
        ok(error == 0xdeadbeef, "Last error: 0x%08x\n", error);
    }

    /* Check if LoadCursor() returns the same handle with the same icon. */
    cursor2 = LoadCursor(NULL, IDC_ARROW);
    ok(cursor2 == cursor, "cursor == %p, cursor2 == %p\n", cursor, cursor2);

    /* Check if LoadCursor() returns the same handle with a different icon. */
    cursor2 = LoadCursor(NULL, IDC_WAIT);
    ok(cursor2 != cursor, "cursor == %p, cursor2 == %p\n", cursor, cursor2);
}

START_TEST(cursoricon)
{
    test_argc = winetest_get_mainargs(&test_argv);

    if (test_argc >= 3)
    {
        /* Child process. */
        sscanf (test_argv[2], "%x", (unsigned int *) &parent);

        ok(parent != NULL, "Parent not found.\n");
        if (parent == NULL)
            ExitProcess(1);

        do_child();
        return;
    }

    test_CopyImage_Bitmap(1);
    test_CopyImage_Bitmap(4);
    test_CopyImage_Bitmap(8);
    test_CopyImage_Bitmap(16);
    test_CopyImage_Bitmap(24);
    test_CopyImage_Bitmap(32);
    test_initial_cursor();
    test_CreateIcon();
    test_DestroyCursor();
    do_parent();
    test_child_process();
    finish_child_process();
}
