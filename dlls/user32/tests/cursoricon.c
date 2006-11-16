/* Unit test suite for cursors and icons.
 *
 * Copyright 2006 Michael Kaufmann
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
    info = (BITMAPINFO *) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD));
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

START_TEST(cursoricon)
{
    test_CopyImage_Bitmap(1);
    test_CopyImage_Bitmap(4);
    test_CopyImage_Bitmap(8);
    test_CopyImage_Bitmap(16);
    test_CopyImage_Bitmap(24);
    test_CopyImage_Bitmap(32);
}
