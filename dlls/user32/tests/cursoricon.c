/*
 * Unit test suite for cursors and icons.
 *
 * Copyright 2006 Michael Kaufmann
 * Copyright 2007 Dmitry Timoshkov
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

START_TEST(cursoricon)
{
    test_CopyImage_Bitmap(1);
    test_CopyImage_Bitmap(4);
    test_CopyImage_Bitmap(8);
    test_CopyImage_Bitmap(16);
    test_CopyImage_Bitmap(24);
    test_CopyImage_Bitmap(32);
    test_CreateIcon();
}
