/*
 * Unit test suite for bitmaps
 *
 * Copyright 2004 Huw Davies
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"

#include "wine/test.h"


static void test_createdibitmap(void)
{
    HDC hdc, hdcmem;
    BITMAPINFOHEADER bmih;
    BITMAP bm;
    HBITMAP hbm, hbm_colour, hbm_old;
    INT screen_depth;

    hdc = GetDC(0);
    screen_depth = GetDeviceCaps(hdc, BITSPIXEL);
    memset(&bmih, 0, sizeof(bmih));
    bmih.biSize = sizeof(bmih);
    bmih.biWidth = 10;
    bmih.biHeight = 10;
    bmih.biPlanes = 1;
    bmih.biBitCount = 32;
    bmih.biCompression = BI_RGB;
 
    /* First create an un-initialised bitmap.  The depth of the bitmap
       should match that of the hdc and not that supplied in bmih.
    */

    /* First try 32 bits */
    hbm = CreateDIBitmap(hdc, &bmih, 0, NULL, NULL, 0);
    ok(hbm != NULL, "CreateDIBitmap failed\n");
    ok(GetObject(hbm, sizeof(bm), &bm), "GetObject failed\n");

    ok(bm.bmBitsPixel == screen_depth, "CreateDIBitmap created bitmap of incorrect depth %d != %d\n", bm.bmBitsPixel, screen_depth);
    DeleteObject(hbm);
    
    /* Then 16 */
    bmih.biBitCount = 16;
    hbm = CreateDIBitmap(hdc, &bmih, 0, NULL, NULL, 0);
    ok(hbm != NULL, "CreateDIBitmap failed\n");
    ok(GetObject(hbm, sizeof(bm), &bm), "GetObject failed\n");

    ok(bm.bmBitsPixel == screen_depth, "CreateDIBitmap created bitmap of incorrect depth %d != %d\n", bm.bmBitsPixel, screen_depth);
    DeleteObject(hbm);

    /* Then 1 */
    bmih.biBitCount = 1;
    hbm = CreateDIBitmap(hdc, &bmih, 0, NULL, NULL, 0);
    ok(hbm != NULL, "CreateDIBitmap failed\n");
    ok(GetObject(hbm, sizeof(bm), &bm), "GetObject failed\n");

    ok(bm.bmBitsPixel == screen_depth, "CreateDIBitmap created bitmap of incorrect depth %d != %d\n", bm.bmBitsPixel, screen_depth);
    DeleteObject(hbm);

    /* Now with a monochrome dc we expect a monochrome bitmap */
    hdcmem = CreateCompatibleDC(hdc);

    /* First try 32 bits */
    bmih.biBitCount = 32;
    hbm = CreateDIBitmap(hdcmem, &bmih, 0, NULL, NULL, 0);
    ok(hbm != NULL, "CreateDIBitmap failed\n");
    ok(GetObject(hbm, sizeof(bm), &bm), "GetObject failed\n");

    ok(bm.bmBitsPixel == 1, "CreateDIBitmap created bitmap of incorrect depth %d != %d\n", bm.bmBitsPixel, 1);
    DeleteObject(hbm);
    
    /* Then 16 */
    bmih.biBitCount = 16;
    hbm = CreateDIBitmap(hdcmem, &bmih, 0, NULL, NULL, 0);
    ok(hbm != NULL, "CreateDIBitmap failed\n");
    ok(GetObject(hbm, sizeof(bm), &bm), "GetObject failed\n");

    ok(bm.bmBitsPixel == 1, "CreateDIBitmap created bitmap of incorrect depth %d != %d\n", bm.bmBitsPixel, 1);
    DeleteObject(hbm);
    
    /* Then 1 */
    bmih.biBitCount = 1;
    hbm = CreateDIBitmap(hdcmem, &bmih, 0, NULL, NULL, 0);
    ok(hbm != NULL, "CreateDIBitmap failed\n");
    ok(GetObject(hbm, sizeof(bm), &bm), "GetObject failed\n");

    ok(bm.bmBitsPixel == 1, "CreateDIBitmap created bitmap of incorrect depth %d != %d\n", bm.bmBitsPixel, 1);
    DeleteObject(hbm);

    /* Now select a polychrome bitmap into the dc and we expect
       screen_depth bitmaps again */
    hbm_colour = CreateCompatibleBitmap(hdc, 1, 1);
    hbm_old = SelectObject(hdcmem, hbm_colour);

    /* First try 32 bits */
    bmih.biBitCount = 32;
    hbm = CreateDIBitmap(hdcmem, &bmih, 0, NULL, NULL, 0);
    ok(hbm != NULL, "CreateDIBitmap failed\n");
    ok(GetObject(hbm, sizeof(bm), &bm), "GetObject failed\n");

    ok(bm.bmBitsPixel == screen_depth, "CreateDIBitmap created bitmap of incorrect depth %d != %d\n", bm.bmBitsPixel, screen_depth);
    DeleteObject(hbm);
    
    /* Then 16 */
    bmih.biBitCount = 16;
    hbm = CreateDIBitmap(hdcmem, &bmih, 0, NULL, NULL, 0);
    ok(hbm != NULL, "CreateDIBitmap failed\n");
    ok(GetObject(hbm, sizeof(bm), &bm), "GetObject failed\n");

    ok(bm.bmBitsPixel == screen_depth, "CreateDIBitmap created bitmap of incorrect depth %d != %d\n", bm.bmBitsPixel, screen_depth);
    DeleteObject(hbm);
    
    /* Then 1 */
    bmih.biBitCount = 1;
    hbm = CreateDIBitmap(hdcmem, &bmih, 0, NULL, NULL, 0);
    ok(hbm != NULL, "CreateDIBitmap failed\n");
    ok(GetObject(hbm, sizeof(bm), &bm), "GetObject failed\n");

    ok(bm.bmBitsPixel == screen_depth, "CreateDIBitmap created bitmap of incorrect depth %d != %d\n", bm.bmBitsPixel, screen_depth);
    DeleteObject(hbm);

    SelectObject(hdcmem, hbm_old);
    DeleteObject(hbm_colour);
    DeleteDC(hdcmem);

    /* If hdc == 0 then we get a 1 bpp bitmap */
    bmih.biBitCount = 32;
    hbm = CreateDIBitmap(0, &bmih, 0, NULL, NULL, 0);
    ok(hbm != NULL, "CreateDIBitmap failed\n");
    ok(GetObject(hbm, sizeof(bm), &bm), "GetObject failed\n");

    ok(bm.bmBitsPixel == 1, "CreateDIBitmap created bitmap of incorrect depth %d != %d\n", bm.bmBitsPixel, 1);
    DeleteObject(hbm);
    
    ReleaseDC(0, hdc);
}

START_TEST(bitmap)
{
    test_createdibitmap();
}
