/*
 * Unit test suite for wing32 functions
 *
 * Copyright 2024 Bernhard Übelacker
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

#include <windows.h>

#include "wine/test.h"

HDC WINAPI WinGCreateDC(void);
HBITMAP WINAPI WinGCreateBitmap(HDC, BITMAPINFO*, void **);
void* WINAPI WinGGetDIBPointer(HBITMAP, BITMAPINFO*);

static void test_WinGGetDIBPointer(void)
{
    HDC dc;
    BITMAPINFO bmi;
    void *bits;
    HBITMAP bmp;
    void* dib;

    dc = WinGCreateDC();
    ok(dc != NULL, "WinGCreateDC failed\n");

    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = 1;
    bmi.bmiHeader.biHeight = 1;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount= 24;
    bmi.bmiHeader.biCompression= BI_RGB;
    bmi.bmiHeader.biSizeImage = 0;
    bmp = WinGCreateBitmap(dc, &bmi, &bits);
    ok(bmp != NULL, "WinGCreateBitmap failed\n");

    dib = WinGGetDIBPointer(NULL, NULL);
    ok(dib == NULL, "WinGGetDIBPointer returned unexpected value %p\n", dib);

    dib = WinGGetDIBPointer(bmp, &bmi);
    ok(dib != NULL, "WinGGetDIBPointer failed\n");

    DeleteObject(bmp);
    DeleteDC(dc);
}

START_TEST(wing32)
{
    test_WinGGetDIBPointer();
}
