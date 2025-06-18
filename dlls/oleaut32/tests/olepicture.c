/*
 * OLEPICTURE test program
 *
 * Copyright 2005 Marcus Meissner
 * Copyright 2012 Dmitry Timoshkov
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

#include <stdarg.h>
#include <stdio.h>
#include <math.h>
#include <float.h>

#define COBJMACROS
#define CONST_VTABLE

#include "wine/test.h"
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <wingdi.h>
#include <winnls.h>
#include <winerror.h>
#include <winnt.h>

#include <urlmon.h>
#include <wtypes.h>
#include <olectl.h>
#include <objidl.h>

#define expect_eq(expr, value, type, format) { type ret = (expr); ok((value) == ret, #expr " expected " format " got " format "\n", value, ret); }

#define ole_expect(expr, expect) { \
    HRESULT r = expr; \
    ok(r == (expect), #expr " returned %lx, expected %s (%lx)\n", r, #expect, expect); \
}

#define ole_check(expr) ole_expect(expr, S_OK);

static HMODULE hOleaut32;

static HRESULT (WINAPI *pOleLoadPicture)(LPSTREAM,LONG,BOOL,REFIID,LPVOID*);
static HRESULT (WINAPI *pOleLoadPictureEx)(LPSTREAM,LONG,BOOL,REFIID,DWORD,DWORD,DWORD,LPVOID*);

#define ok_ole_success(hr, func) ok(hr == S_OK, func " failed with error %#08lx\n", hr)

/* 1x1 pixel gif */
static const unsigned char gifimage[35] = {
0x47,0x49,0x46,0x38,0x37,0x61,0x01,0x00,0x01,0x00,0x80,0x00,0x00,0xff,0xff,0xff,
0xff,0xff,0xff,0x2c,0x00,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x00,0x02,0x02,0x44,
0x01,0x00,0x3b
};

/* 1x1 pixel jpg */
static const unsigned char jpgimage[285] = {
0xff,0xd8,0xff,0xe0,0x00,0x10,0x4a,0x46,0x49,0x46,0x00,0x01,0x01,0x01,0x01,0x2c,
0x01,0x2c,0x00,0x00,0xff,0xdb,0x00,0x43,0x00,0x05,0x03,0x04,0x04,0x04,0x03,0x05,
0x04,0x04,0x04,0x05,0x05,0x05,0x06,0x07,0x0c,0x08,0x07,0x07,0x07,0x07,0x0f,0x0b,
0x0b,0x09,0x0c,0x11,0x0f,0x12,0x12,0x11,0x0f,0x11,0x11,0x13,0x16,0x1c,0x17,0x13,
0x14,0x1a,0x15,0x11,0x11,0x18,0x21,0x18,0x1a,0x1d,0x1d,0x1f,0x1f,0x1f,0x13,0x17,
0x22,0x24,0x22,0x1e,0x24,0x1c,0x1e,0x1f,0x1e,0xff,0xdb,0x00,0x43,0x01,0x05,0x05,
0x05,0x07,0x06,0x07,0x0e,0x08,0x08,0x0e,0x1e,0x14,0x11,0x14,0x1e,0x1e,0x1e,0x1e,
0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,
0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,
0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0xff,0xc0,
0x00,0x11,0x08,0x00,0x01,0x00,0x01,0x03,0x01,0x22,0x00,0x02,0x11,0x01,0x03,0x11,
0x01,0xff,0xc4,0x00,0x15,0x00,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0xff,0xc4,0x00,0x14,0x10,0x01,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xc4,
0x00,0x14,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0xff,0xc4,0x00,0x14,0x11,0x01,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xda,0x00,0x0c,0x03,0x01,
0x00,0x02,0x11,0x03,0x11,0x00,0x3f,0x00,0xb2,0xc0,0x07,0xff,0xd9
};

/* 1x1 pixel png */
static const unsigned char pngimage[285] = {
0x89,0x50,0x4e,0x47,0x0d,0x0a,0x1a,0x0a,0x00,0x00,0x00,0x0d,0x49,0x48,0x44,0x52,
0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x08,0x02,0x00,0x00,0x00,0x90,0x77,0x53,
0xde,0x00,0x00,0x00,0x09,0x70,0x48,0x59,0x73,0x00,0x00,0x0b,0x13,0x00,0x00,0x0b,
0x13,0x01,0x00,0x9a,0x9c,0x18,0x00,0x00,0x00,0x07,0x74,0x49,0x4d,0x45,0x07,0xd5,
0x06,0x03,0x0f,0x07,0x2d,0x12,0x10,0xf0,0xfd,0x00,0x00,0x00,0x0c,0x49,0x44,0x41,
0x54,0x08,0xd7,0x63,0xf8,0xff,0xff,0x3f,0x00,0x05,0xfe,0x02,0xfe,0xdc,0xcc,0x59,
0xe7,0x00,0x00,0x00,0x00,0x49,0x45,0x4e,0x44,0xae,0x42,0x60,0x82
};

/* 1bpp BI_RGB 1x1 pixel bmp */
static const unsigned char bmpimage[66] = {
0x42,0x4d,0x42,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x3e,0x00,0x00,0x00,0x28,0x00,
0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x00,0x00,
0x00,0x00,0x04,0x00,0x00,0x00,0x12,0x0b,0x00,0x00,0x12,0x0b,0x00,0x00,0x02,0x00,
0x00,0x00,0x02,0x00,0x00,0x00,0xff,0xff,0xff,0x00,0xff,0xff,0xff,0x00,0x00,0x00,
0x00,0x00
};

/* 8bpp BI_RLE8 1x1 pixel bmp */
static const unsigned char bmpimage_rle8[] = {
0x42,0x4d,0x42,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x3e,0x00,0x00,0x00,0x28,0x00,
0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x08,0x00,0x01,0x00,
0x00,0x00,0x04,0x00,0x00,0x00,0x12,0x0b,0x00,0x00,0x12,0x0b,0x00,0x00,0x02,0x00,
0x00,0x00,0x02,0x00,0x00,0x00,0xff,0xff,0xff,0x00,0xff,0xff,0xff,0x00,0x00,0x01,
0x00,0x00
};

/* 1x1 pixel 4-bit bmp */
static const unsigned char bmpimage4[122] = {
0x42,0x4d,0x7a,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x76,0x00,0x00,0x00,0x28,0x00,
0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x04,0x00,0x00,0x00,
0x00,0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x00,0x00,0x80,
0x00,0x00,0x00,0x80,0x80,0x00,0x80,0x00,0x00,0x00,0x80,0x00,0x80,0x00,0x80,0x80,
0x00,0x00,0x80,0x80,0x80,0x00,0xc0,0xc0,0xc0,0x00,0x00,0x00,0xff,0x00,0x00,0xff,
0x00,0x00,0x00,0xff,0xff,0x00,0xff,0x00,0x00,0x00,0xff,0x00,0xff,0x00,0xff,0xff,
0x00,0x00,0xff,0xff,0xff,0x00,0xf0,0x00,0x00,0x00,
};

/* 1x1 pixel 8-bit bmp */
static const unsigned char bmpimage8[1082] = {
0x42,0x4d,0x3a,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x36,0x04,0x00,0x00,0x28,0x00,
0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x08,0x00,0x00,0x00,
0x00,0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x00,0x00,0x80,
0x00,0x00,0x00,0x80,0x80,0x00,0x80,0x00,0x00,0x00,0x80,0x00,0x80,0x00,0x80,0x80,
0x00,0x00,0xc0,0xc0,0xc0,0x00,0xc0,0xdc,0xc0,0x00,0xf0,0xca,0xa6,0x00,0x00,0x20,
0x40,0x00,0x00,0x20,0x60,0x00,0x00,0x20,0x80,0x00,0x00,0x20,0xa0,0x00,0x00,0x20,
0xc0,0x00,0x00,0x20,0xe0,0x00,0x00,0x40,0x00,0x00,0x00,0x40,0x20,0x00,0x00,0x40,
0x40,0x00,0x00,0x40,0x60,0x00,0x00,0x40,0x80,0x00,0x00,0x40,0xa0,0x00,0x00,0x40,
0xc0,0x00,0x00,0x40,0xe0,0x00,0x00,0x60,0x00,0x00,0x00,0x60,0x20,0x00,0x00,0x60,
0x40,0x00,0x00,0x60,0x60,0x00,0x00,0x60,0x80,0x00,0x00,0x60,0xa0,0x00,0x00,0x60,
0xc0,0x00,0x00,0x60,0xe0,0x00,0x00,0x80,0x00,0x00,0x00,0x80,0x20,0x00,0x00,0x80,
0x40,0x00,0x00,0x80,0x60,0x00,0x00,0x80,0x80,0x00,0x00,0x80,0xa0,0x00,0x00,0x80,
0xc0,0x00,0x00,0x80,0xe0,0x00,0x00,0xa0,0x00,0x00,0x00,0xa0,0x20,0x00,0x00,0xa0,
0x40,0x00,0x00,0xa0,0x60,0x00,0x00,0xa0,0x80,0x00,0x00,0xa0,0xa0,0x00,0x00,0xa0,
0xc0,0x00,0x00,0xa0,0xe0,0x00,0x00,0xc0,0x00,0x00,0x00,0xc0,0x20,0x00,0x00,0xc0,
0x40,0x00,0x00,0xc0,0x60,0x00,0x00,0xc0,0x80,0x00,0x00,0xc0,0xa0,0x00,0x00,0xc0,
0xc0,0x00,0x00,0xc0,0xe0,0x00,0x00,0xe0,0x00,0x00,0x00,0xe0,0x20,0x00,0x00,0xe0,
0x40,0x00,0x00,0xe0,0x60,0x00,0x00,0xe0,0x80,0x00,0x00,0xe0,0xa0,0x00,0x00,0xe0,
0xc0,0x00,0x00,0xe0,0xe0,0x00,0x40,0x00,0x00,0x00,0x40,0x00,0x20,0x00,0x40,0x00,
0x40,0x00,0x40,0x00,0x60,0x00,0x40,0x00,0x80,0x00,0x40,0x00,0xa0,0x00,0x40,0x00,
0xc0,0x00,0x40,0x00,0xe0,0x00,0x40,0x20,0x00,0x00,0x40,0x20,0x20,0x00,0x40,0x20,
0x40,0x00,0x40,0x20,0x60,0x00,0x40,0x20,0x80,0x00,0x40,0x20,0xa0,0x00,0x40,0x20,
0xc0,0x00,0x40,0x20,0xe0,0x00,0x40,0x40,0x00,0x00,0x40,0x40,0x20,0x00,0x40,0x40,
0x40,0x00,0x40,0x40,0x60,0x00,0x40,0x40,0x80,0x00,0x40,0x40,0xa0,0x00,0x40,0x40,
0xc0,0x00,0x40,0x40,0xe0,0x00,0x40,0x60,0x00,0x00,0x40,0x60,0x20,0x00,0x40,0x60,
0x40,0x00,0x40,0x60,0x60,0x00,0x40,0x60,0x80,0x00,0x40,0x60,0xa0,0x00,0x40,0x60,
0xc0,0x00,0x40,0x60,0xe0,0x00,0x40,0x80,0x00,0x00,0x40,0x80,0x20,0x00,0x40,0x80,
0x40,0x00,0x40,0x80,0x60,0x00,0x40,0x80,0x80,0x00,0x40,0x80,0xa0,0x00,0x40,0x80,
0xc0,0x00,0x40,0x80,0xe0,0x00,0x40,0xa0,0x00,0x00,0x40,0xa0,0x20,0x00,0x40,0xa0,
0x40,0x00,0x40,0xa0,0x60,0x00,0x40,0xa0,0x80,0x00,0x40,0xa0,0xa0,0x00,0x40,0xa0,
0xc0,0x00,0x40,0xa0,0xe0,0x00,0x40,0xc0,0x00,0x00,0x40,0xc0,0x20,0x00,0x40,0xc0,
0x40,0x00,0x40,0xc0,0x60,0x00,0x40,0xc0,0x80,0x00,0x40,0xc0,0xa0,0x00,0x40,0xc0,
0xc0,0x00,0x40,0xc0,0xe0,0x00,0x40,0xe0,0x00,0x00,0x40,0xe0,0x20,0x00,0x40,0xe0,
0x40,0x00,0x40,0xe0,0x60,0x00,0x40,0xe0,0x80,0x00,0x40,0xe0,0xa0,0x00,0x40,0xe0,
0xc0,0x00,0x40,0xe0,0xe0,0x00,0x80,0x00,0x00,0x00,0x80,0x00,0x20,0x00,0x80,0x00,
0x40,0x00,0x80,0x00,0x60,0x00,0x80,0x00,0x80,0x00,0x80,0x00,0xa0,0x00,0x80,0x00,
0xc0,0x00,0x80,0x00,0xe0,0x00,0x80,0x20,0x00,0x00,0x80,0x20,0x20,0x00,0x80,0x20,
0x40,0x00,0x80,0x20,0x60,0x00,0x80,0x20,0x80,0x00,0x80,0x20,0xa0,0x00,0x80,0x20,
0xc0,0x00,0x80,0x20,0xe0,0x00,0x80,0x40,0x00,0x00,0x80,0x40,0x20,0x00,0x80,0x40,
0x40,0x00,0x80,0x40,0x60,0x00,0x80,0x40,0x80,0x00,0x80,0x40,0xa0,0x00,0x80,0x40,
0xc0,0x00,0x80,0x40,0xe0,0x00,0x80,0x60,0x00,0x00,0x80,0x60,0x20,0x00,0x80,0x60,
0x40,0x00,0x80,0x60,0x60,0x00,0x80,0x60,0x80,0x00,0x80,0x60,0xa0,0x00,0x80,0x60,
0xc0,0x00,0x80,0x60,0xe0,0x00,0x80,0x80,0x00,0x00,0x80,0x80,0x20,0x00,0x80,0x80,
0x40,0x00,0x80,0x80,0x60,0x00,0x80,0x80,0x80,0x00,0x80,0x80,0xa0,0x00,0x80,0x80,
0xc0,0x00,0x80,0x80,0xe0,0x00,0x80,0xa0,0x00,0x00,0x80,0xa0,0x20,0x00,0x80,0xa0,
0x40,0x00,0x80,0xa0,0x60,0x00,0x80,0xa0,0x80,0x00,0x80,0xa0,0xa0,0x00,0x80,0xa0,
0xc0,0x00,0x80,0xa0,0xe0,0x00,0x80,0xc0,0x00,0x00,0x80,0xc0,0x20,0x00,0x80,0xc0,
0x40,0x00,0x80,0xc0,0x60,0x00,0x80,0xc0,0x80,0x00,0x80,0xc0,0xa0,0x00,0x80,0xc0,
0xc0,0x00,0x80,0xc0,0xe0,0x00,0x80,0xe0,0x00,0x00,0x80,0xe0,0x20,0x00,0x80,0xe0,
0x40,0x00,0x80,0xe0,0x60,0x00,0x80,0xe0,0x80,0x00,0x80,0xe0,0xa0,0x00,0x80,0xe0,
0xc0,0x00,0x80,0xe0,0xe0,0x00,0xc0,0x00,0x00,0x00,0xc0,0x00,0x20,0x00,0xc0,0x00,
0x40,0x00,0xc0,0x00,0x60,0x00,0xc0,0x00,0x80,0x00,0xc0,0x00,0xa0,0x00,0xc0,0x00,
0xc0,0x00,0xc0,0x00,0xe0,0x00,0xc0,0x20,0x00,0x00,0xc0,0x20,0x20,0x00,0xc0,0x20,
0x40,0x00,0xc0,0x20,0x60,0x00,0xc0,0x20,0x80,0x00,0xc0,0x20,0xa0,0x00,0xc0,0x20,
0xc0,0x00,0xc0,0x20,0xe0,0x00,0xc0,0x40,0x00,0x00,0xc0,0x40,0x20,0x00,0xc0,0x40,
0x40,0x00,0xc0,0x40,0x60,0x00,0xc0,0x40,0x80,0x00,0xc0,0x40,0xa0,0x00,0xc0,0x40,
0xc0,0x00,0xc0,0x40,0xe0,0x00,0xc0,0x60,0x00,0x00,0xc0,0x60,0x20,0x00,0xc0,0x60,
0x40,0x00,0xc0,0x60,0x60,0x00,0xc0,0x60,0x80,0x00,0xc0,0x60,0xa0,0x00,0xc0,0x60,
0xc0,0x00,0xc0,0x60,0xe0,0x00,0xc0,0x80,0x00,0x00,0xc0,0x80,0x20,0x00,0xc0,0x80,
0x40,0x00,0xc0,0x80,0x60,0x00,0xc0,0x80,0x80,0x00,0xc0,0x80,0xa0,0x00,0xc0,0x80,
0xc0,0x00,0xc0,0x80,0xe0,0x00,0xc0,0xa0,0x00,0x00,0xc0,0xa0,0x20,0x00,0xc0,0xa0,
0x40,0x00,0xc0,0xa0,0x60,0x00,0xc0,0xa0,0x80,0x00,0xc0,0xa0,0xa0,0x00,0xc0,0xa0,
0xc0,0x00,0xc0,0xa0,0xe0,0x00,0xc0,0xc0,0x00,0x00,0xc0,0xc0,0x20,0x00,0xc0,0xc0,
0x40,0x00,0xc0,0xc0,0x60,0x00,0xc0,0xc0,0x80,0x00,0xc0,0xc0,0xa0,0x00,0xf0,0xfb,
0xff,0x00,0xa4,0xa0,0xa0,0x00,0x80,0x80,0x80,0x00,0x00,0x00,0xff,0x00,0x00,0xff,
0x00,0x00,0x00,0xff,0xff,0x00,0xff,0x00,0x00,0x00,0xff,0x00,0xff,0x00,0xff,0xff,
0x00,0x00,0xff,0xff,0xff,0x00,0xff,0x00,0x00,0x00,
};

/* 1x1 pixel 24-bit bmp */
static const unsigned char bmpimage24[58] = {
0x42,0x4d,0x3a,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x36,0x00,0x00,0x00,0x28,0x00,
0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x18,0x00,0x00,0x00,
0x00,0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,0xff,0x00,
};

/* 2x2 pixel gif */
static const unsigned char gif4pixel[42] = {
0x47,0x49,0x46,0x38,0x37,0x61,0x02,0x00,0x02,0x00,0xa1,0x00,0x00,0x00,0x00,0x00,
0x39,0x62,0xfc,0xff,0x1a,0xe5,0xff,0xff,0xff,0x2c,0x00,0x00,0x00,0x00,0x02,0x00,
0x02,0x00,0x00,0x02,0x03,0x14,0x16,0x05,0x00,0x3b
};

/* APM with an empty metafile with some padding zeros - looks like under Window the
 * metafile data should be at least 20 bytes */
static const unsigned char apmdata[] = {
0xd7,0xcd,0xc6,0x9a, 0x00,0x00,0x00,0x00, 0x00,0x00,0xee,0x02, 0xb1,0x03,0xa0,0x05,
0x00,0x00,0x00,0x00, 0xee,0x53,0x01,0x00, 0x09,0x00,0x00,0x03, 0x13,0x00,0x00,0x00,
0x01,0x00,0x05,0x00, 0x00,0x00,0x00,0x00, 0x03,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00
};

/* MF_TEXTOUT_ON_PATH_BITS from gdi32/tests/metafile.c */
static const unsigned char metafile[] = {
    0x01, 0x00, 0x09, 0x00, 0x00, 0x03, 0x19, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x32, 0x0a,
    0x16, 0x00, 0x0b, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x54, 0x65, 0x73, 0x74, 0x03, 0x00, 0x05, 0x00,
    0x08, 0x00, 0x0c, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x00, 0x00
};

/* EMF_TEXTOUT_ON_PATH_BITS from gdi32/tests/metafile.c */
static const unsigned char enhmetafile[] = {
    0x01, 0x00, 0x00, 0x00, 0x6c, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xe7, 0xff, 0xff, 0xff, 0xe9, 0xff, 0xff, 0xff,
    0x20, 0x45, 0x4d, 0x46, 0x00, 0x00, 0x01, 0x00,
    0xf4, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x05, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00,
    0x40, 0x01, 0x00, 0x00, 0xf0, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0xe2, 0x04, 0x00,
    0x80, 0xa9, 0x03, 0x00, 0x3b, 0x00, 0x00, 0x00,
    0x08, 0x00, 0x00, 0x00, 0x54, 0x00, 0x00, 0x00,
    0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0xc8, 0x41, 0x00, 0x80, 0xbb, 0x41,
    0x0b, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x4c, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0x54, 0x00, 0x00, 0x00,
    0x54, 0x00, 0x65, 0x00, 0x73, 0x00, 0x74, 0x00,
    0x03, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00,
    0x08, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00,
    0x3c, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
    0x0e, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
    0x14, 0x00, 0x00, 0x00
};

static HBITMAP stock_bm;

static HDC create_render_dc( void )
{
    HDC dc = CreateCompatibleDC( NULL );
    BITMAPINFO info = {{sizeof(info.bmiHeader), 100, 100, 1, 32, BI_RGB }};
    void *bits;
    HBITMAP dib = CreateDIBSection( NULL, &info, DIB_RGB_COLORS, &bits, NULL, 0 );

    stock_bm = SelectObject( dc, dib );
    return dc;
}

static void delete_render_dc( HDC dc )
{
    HBITMAP dib = SelectObject( dc, stock_bm );
    DeleteObject( dib );
    DeleteDC( dc );
}

typedef struct NoStatStreamImpl
{
	IStream			IStream_iface;
	LONG			ref;

	HGLOBAL			supportHandle;
	ULARGE_INTEGER		streamSize;
	ULARGE_INTEGER		currentPosition;
} NoStatStreamImpl;

static IStream* NoStatStream_Construct(HGLOBAL hGlobal);

static void
test_pic_with_stream(LPSTREAM stream, unsigned int imgsize, int bpp, BOOL todo)
{
	IPicture*	pic = NULL;
	HRESULT		hres;
	LPVOID		pvObj = NULL;
	OLE_HANDLE	handle, hPal;
	OLE_XSIZE_HIMETRIC	width;
	OLE_YSIZE_HIMETRIC	height;
	short		type;
	DWORD		attr;
	ULONG		res;

	pvObj = NULL;
	hres = pOleLoadPicture(stream, imgsize, TRUE, &IID_IPicture, &pvObj);
	pic = pvObj;

	ok(hres == S_OK,"OLP (NULL,..) does not return 0, but 0x%08lx\n",hres);
	ok(pic != NULL,"OLP (NULL,..) returns NULL, instead of !NULL\n");
	if (pic == NULL)
		return;

	pvObj = NULL;
	hres = IPicture_QueryInterface (pic, &IID_IPicture, &pvObj);

	ok(hres == S_OK,"IPicture_QI does not return S_OK, but 0x%08lx\n", hres);
	ok(pvObj != NULL,"IPicture_QI does return NULL, instead of a ptr\n");

	IPicture_Release ((IPicture*)pvObj);

	handle = 0;
	hres = IPicture_get_Handle (pic, &handle);
	ok(hres == S_OK,"IPicture_get_Handle does not return S_OK, but 0x%08lx\n", hres);
	ok(handle != 0, "IPicture_get_Handle returns a NULL handle, but it should be non NULL\n");

        if (handle)
        {
            BITMAP bmp;
            DIBSECTION dib;

            GetObjectA(UlongToHandle(handle), sizeof(BITMAP), &bmp);
            ok(bmp.bmBits != 0, "not a dib\n");
            todo_wine_if(todo)
            ok(bmp.bmBitsPixel == bpp, "expected %d, got %d\n", bpp, bmp.bmBitsPixel);

            GetObjectA(UlongToHandle(handle), sizeof(DIBSECTION), &dib);
            ok(dib.dsBm.bmBits != 0, "not a dib\n");
            todo_wine_if(todo) {
            ok(dib.dsBm.bmBitsPixel == bpp, "expected %d, got %d\n", bpp, dib.dsBm.bmBitsPixel);
            ok(dib.dsBmih.biBitCount == bpp, "expected %d, got %d\n", bpp, dib.dsBmih.biBitCount);
            }
            ok(dib.dsBmih.biCompression == BI_RGB, "expected %d, got %ld\n", BI_RGB, dib.dsBmih.biCompression);
            todo_wine_if(dib.dsBmih.biClrUsed != dib.dsBmih.biClrImportant)
            ok(dib.dsBmih.biClrUsed == dib.dsBmih.biClrImportant, "expected %ld, got %ld\n", dib.dsBmih.biClrUsed, dib.dsBmih.biClrImportant);
        }

	width = 0;
	hres = IPicture_get_Width (pic, &width);
	ok(hres == S_OK,"IPicture_get_Width does not return S_OK, but 0x%08lx\n", hres);
	ok(width != 0, "IPicture_get_Width returns 0, but it should not be 0.\n");

	height = 0;
	hres = IPicture_get_Height (pic, &height);
	ok(hres == S_OK,"IPicture_get_Height does not return S_OK, but 0x%08lx\n", hres);
	ok(height != 0, "IPicture_get_Height returns 0, but it should not be 0.\n");

	type = 0;
	hres = IPicture_get_Type (pic, &type);
	ok(hres == S_OK,"IPicture_get_Type does not return S_OK, but 0x%08lx\n", hres);
	ok(type == PICTYPE_BITMAP, "IPicture_get_Type returns %d, but it should be PICTYPE_BITMAP(%d).\n", type, PICTYPE_BITMAP);

	attr = 0;
	hres = IPicture_get_Attributes (pic, &attr);
	ok(hres == S_OK,"IPicture_get_Attributes does not return S_OK, but 0x%08lx\n", hres);
	ok(attr == 0, "IPicture_get_Attributes returns %ld, but it should be 0.\n", attr);

	hPal = 0;
	hres = IPicture_get_hPal (pic, &hPal);
	ok(hres == S_OK,"IPicture_get_hPal does not return S_OK, but 0x%08lx\n", hres);
	/* a single pixel b/w image has no palette */
	ok(hPal == 0, "IPicture_get_hPal returns %d, but it should be 0.\n", hPal);

	res = IPicture_Release (pic);
	ok (res == 0, "refcount after release is %ld, but should be 0?\n", res);
}

static void
test_pic(const unsigned char *imgdata, unsigned int imgsize, int bpp, BOOL todo)
{
	LPSTREAM 	stream;
	HGLOBAL		hglob;
	LPBYTE		data;
	HRESULT		hres;
	LARGE_INTEGER	seekto;
	ULARGE_INTEGER	newpos1;
	DWORD * 	header;
	unsigned int 	i,j;

	/* Let the fun begin */
	hglob = GlobalAlloc (0, imgsize);
	data = GlobalLock (hglob);
	memcpy(data, imgdata, imgsize);
	GlobalUnlock(hglob); data = NULL;

	hres = CreateStreamOnHGlobal (hglob, FALSE, &stream);
	ok (hres == S_OK, "createstreamonhglobal failed? doubt it... hres 0x%08lx\n", hres);

	memset(&seekto,0,sizeof(seekto));
	hres = IStream_Seek(stream, seekto, STREAM_SEEK_CUR, &newpos1);
	ok (hres == S_OK, "istream seek failed? doubt it... hres 0x%08lx\n", hres);
	test_pic_with_stream(stream, imgsize, bpp, todo);

	IStream_Release(stream);

	/* again with Non Statable and Non Seekable stream */
	stream = NoStatStream_Construct(hglob);
	hglob = 0;  /* Non-statable impl always deletes on release */
	test_pic_with_stream(stream, 0, bpp, todo);

	IStream_Release(stream);
	for (i = 1; i <= 8; i++) {
		/* more fun!!! */
		hglob = GlobalAlloc (0, imgsize + i * (2 * sizeof(DWORD)));
		data = GlobalLock (hglob);
		header = (DWORD *)data;

		/* multiple copies of header */
		memcpy(data,"lt\0\0",4);
		header[1] = imgsize;
		for (j = 2; j <= i; j++) {
			memcpy(&(header[2 * (j - 1)]), header, 2 * sizeof(DWORD));
		}
		memcpy(data + i * (2 * sizeof(DWORD)), imgdata, imgsize);
		GlobalUnlock(hglob); data = NULL;

		hres = CreateStreamOnHGlobal (hglob, FALSE, &stream);
		ok (hres == S_OK, "createstreamonhglobal failed? doubt it... hres 0x%08lx\n", hres);

		memset(&seekto,0,sizeof(seekto));
		hres = IStream_Seek(stream, seekto, STREAM_SEEK_CUR, &newpos1);
		ok (hres == S_OK, "istream seek failed? doubt it... hres 0x%08lx\n", hres);
		test_pic_with_stream(stream, imgsize, bpp, todo);

		IStream_Release(stream);

		/* again with Non Statable and Non Seekable stream */
		stream = NoStatStream_Construct(hglob);
		hglob = 0;  /* Non-statable impl always deletes on release */
		test_pic_with_stream(stream, 0, bpp, todo);

		IStream_Release(stream);
	}
}

static void test_empty_image(void) {
	LPBYTE		data;
	LPSTREAM	stream;
	IPicture*	pic = NULL;
	HRESULT		hres;
	LPVOID		pvObj = NULL;
	HGLOBAL		hglob;
	OLE_HANDLE	handle;
	ULARGE_INTEGER	newpos1;
	LARGE_INTEGER	seekto;
	short		type;
	DWORD		attr;

	/* Empty image. Happens occasionally in VB programs. */
	hglob = GlobalAlloc (0, 8);
	data = GlobalLock (hglob);
	memcpy(data,"lt\0\0",4);
	((DWORD*)data)[1] = 0;
	hres = CreateStreamOnHGlobal (hglob, TRUE, &stream);
	ok (hres == S_OK, "CreatestreamOnHGlobal failed? doubt it... hres 0x%08lx\n", hres);

	memset(&seekto,0,sizeof(seekto));
	hres = IStream_Seek(stream, seekto, STREAM_SEEK_CUR, &newpos1);
	ok (hres == S_OK, "istream seek failed? doubt it... hres 0x%08lx\n", hres);

	pvObj = NULL;
	hres = pOleLoadPicture(stream, 8, TRUE, &IID_IPicture, &pvObj);
	pic = pvObj;
	ok(hres == S_OK,"empty picture not loaded, hres 0x%08lx\n", hres);
	ok(pic != NULL,"empty picture not loaded, pic is NULL\n");

	hres = IPicture_get_Type (pic, &type);
	ok (hres == S_OK,"empty picture get type failed with hres 0x%08lx\n", hres);
	ok (type == PICTYPE_NONE,"type is %d, but should be PICTYPE_NONE(0)\n", type);

	attr = 0xdeadbeef;
	hres = IPicture_get_Attributes (pic, &attr);
	ok (hres == S_OK,"empty picture get attributes failed with hres 0x%08lx\n", hres);
	ok (attr == 0,"attr is %ld, but should be 0\n", attr);

	hres = IPicture_get_Handle (pic, &handle);
	ok (hres == S_OK,"empty picture get handle failed with hres 0x%08lx\n", hres);
	ok (handle == 0, "empty picture get handle did not return 0, but 0x%08x\n", handle);
	IPicture_Release (pic);
	IStream_Release (stream);
}

static void test_empty_image_2(void) {
	LPBYTE		data;
	LPSTREAM	stream;
	IPicture*	pic = NULL;
	HRESULT		hres;
	LPVOID		pvObj = NULL;
	HGLOBAL		hglob;
	ULARGE_INTEGER	newpos1;
	LARGE_INTEGER	seekto;
	short		type;

	/* Empty image at random stream position. */
	hglob = GlobalAlloc (0, 200);
	data = GlobalLock (hglob);
	data += 42;
	memcpy(data,"lt\0\0",4);
	((DWORD*)data)[1] = 0;
	hres = CreateStreamOnHGlobal (hglob, TRUE, &stream);
	ok (hres == S_OK, "CreatestreamOnHGlobal failed? doubt it... hres 0x%08lx\n", hres);

	memset(&seekto,0,sizeof(seekto));
	seekto.LowPart = 42;
	hres = IStream_Seek(stream, seekto, STREAM_SEEK_CUR, &newpos1);
	ok (hres == S_OK, "istream seek failed? doubt it... hres 0x%08lx\n", hres);

	pvObj = NULL;
	hres = pOleLoadPicture(stream, 8, TRUE, &IID_IPicture, &pvObj);
	pic = pvObj;
	ok(hres == S_OK,"empty picture not loaded, hres 0x%08lx\n", hres);
	ok(pic != NULL,"empty picture not loaded, pic is NULL\n");

	hres = IPicture_get_Type (pic, &type);
	ok (hres == S_OK,"empty picture get type failed with hres 0x%08lx\n", hres);
	ok (type == PICTYPE_NONE,"type is %d, but should be PICTYPE_NONE(0)\n", type);

	IPicture_Release (pic);
	IStream_Release (stream);
}

static void test_Invoke(void)
{
    IPictureDisp *picdisp;
    HRESULT hr;
    VARIANTARG vararg, args[10];
    DISPPARAMS dispparams;
    VARIANT varresult;
    IStream *stream;
    HGLOBAL hglob;
    void *data;
    HDC hdc;
    int i;

    hglob = GlobalAlloc (0, sizeof(gifimage));
    data = GlobalLock(hglob);
    memcpy(data, gifimage, sizeof(gifimage));
    GlobalUnlock(hglob);

    hr = CreateStreamOnHGlobal (hglob, FALSE, &stream);
    ok_ole_success(hr, "CreateStreamOnHGlobal");

    hr = pOleLoadPicture(stream, sizeof(gifimage), TRUE, &IID_IPictureDisp, (void **)&picdisp);
    IStream_Release(stream);
    GlobalFree(hglob);
    ok_ole_success(hr, "OleLoadPicture");

    V_VT(&vararg) = VT_BOOL;
    V_BOOL(&vararg) = VARIANT_FALSE;
    dispparams.cNamedArgs = 0;
    dispparams.rgdispidNamedArgs = NULL;
    dispparams.cArgs = 1;
    dispparams.rgvarg = &vararg;
    hr = IPictureDisp_Invoke(picdisp, DISPID_PICT_HPAL, &IID_IPictureDisp, 0, DISPATCH_PROPERTYPUT, &dispparams, NULL, NULL, NULL);
    ok(hr == DISP_E_UNKNOWNNAME, "IPictureDisp_Invoke should have returned DISP_E_UNKNOWNNAME instead of 0x%08lx\n", hr);
    hr = IPictureDisp_Invoke(picdisp, DISPID_PICT_HPAL, &IID_IUnknown, 0, DISPATCH_PROPERTYPUT, &dispparams, NULL, NULL, NULL);
    ok(hr == DISP_E_UNKNOWNNAME, "IPictureDisp_Invoke should have returned DISP_E_UNKNOWNNAME instead of 0x%08lx\n", hr);

    dispparams.cArgs = 0;
    dispparams.rgvarg = NULL;
    hr = IPictureDisp_Invoke(picdisp, DISPID_PICT_HPAL, &IID_NULL, 0, DISPATCH_PROPERTYPUT, &dispparams, NULL, NULL, NULL);
    ok(hr == DISP_E_BADPARAMCOUNT, "IPictureDisp_Invoke should have returned DISP_E_BADPARAMCOUNT instead of 0x%08lx\n", hr);

    hr = IPictureDisp_Invoke(picdisp, DISPID_PICT_HPAL, &IID_NULL, 0, DISPATCH_PROPERTYPUT, NULL, NULL, NULL, NULL);
    ok(hr == DISP_E_PARAMNOTOPTIONAL, "IPictureDisp_Invoke should have returned DISP_E_PARAMNOTOPTIONAL instead of 0x%08lx\n", hr);

    hr = IPictureDisp_Invoke(picdisp, DISPID_PICT_HPAL, &IID_NULL, 0, DISPATCH_PROPERTYGET, NULL, NULL, NULL, NULL);
    ok(hr == DISP_E_PARAMNOTOPTIONAL, "IPictureDisp_Invoke should have returned DISP_E_PARAMNOTOPTIONAL instead of 0x%08lx\n", hr);

    hr = IPictureDisp_Invoke(picdisp, DISPID_PICT_HPAL, &IID_NULL, 0, DISPATCH_PROPERTYGET, NULL, &varresult, NULL, NULL);
    ok(hr == DISP_E_PARAMNOTOPTIONAL, "IPictureDisp_Invoke should have returned DISP_E_PARAMNOTOPTIONAL instead of 0x%08lx\n", hr);

    hr = IPictureDisp_Invoke(picdisp, DISPID_PICT_HPAL, &IID_NULL, 0, DISPATCH_PROPERTYGET, &dispparams, &varresult, NULL, NULL);
    ok_ole_success(hr, "IPictureDisp_Invoke");
    ok(V_VT(&varresult) == VT_I4, "V_VT(&varresult) should have been VT_UINT instead of %d\n", V_VT(&varresult));

    hr = IPictureDisp_Invoke(picdisp, DISPID_PICT_HPAL, &IID_NULL, 0, DISPATCH_METHOD, &dispparams, &varresult, NULL, NULL);
    ok(hr == DISP_E_MEMBERNOTFOUND, "IPictureDisp_Invoke should have returned DISP_E_MEMBERNOTFOUND instead of 0x%08lx\n", hr);

    hr = IPictureDisp_Invoke(picdisp, 0xdeadbeef, &IID_NULL, 0, DISPATCH_PROPERTYGET, &dispparams, &varresult, NULL, NULL);
    ok(hr == DISP_E_MEMBERNOTFOUND, "IPictureDisp_Invoke should have returned DISP_E_MEMBERNOTFOUND instead of 0x%08lx\n", hr);

    dispparams.cArgs = 1;
    dispparams.rgvarg = &vararg;
    hr = IPictureDisp_Invoke(picdisp, DISPID_PICT_HPAL, &IID_NULL, 0, DISPATCH_PROPERTYGET, &dispparams, &varresult, NULL, NULL);
    ok(hr == DISP_E_BADPARAMCOUNT, "IPictureDisp_Invoke should have returned DISP_E_BADPARAMCOUNT instead of 0x%08lx\n", hr);

    dispparams.cArgs = 1;
    dispparams.rgvarg = &vararg;
    hr = IPictureDisp_Invoke(picdisp, DISPID_PICT_HPAL, &IID_NULL, 0, DISPATCH_PROPERTYGET, &dispparams, &varresult, NULL, NULL);
    ok(hr == DISP_E_BADPARAMCOUNT, "IPictureDisp_Invoke should have returned DISP_E_BADPARAMCOUNT instead of 0x%08lx\n", hr);

    /* DISPID_PICT_RENDER */
    hdc = create_render_dc();

    for (i = 0; i < ARRAY_SIZE(args); i++)
        V_VT(&args[i]) = VT_I4;

    V_I4(&args[0]) = 0;
    V_I4(&args[1]) = 10;
    V_I4(&args[2]) = 10;
    V_I4(&args[3]) = 0;
    V_I4(&args[4]) = 0;
    V_I4(&args[5]) = 10;
    V_I4(&args[6]) = 10;
    V_I4(&args[7]) = 0;
    V_I4(&args[8]) = 0;
    V_I4(&args[9]) = HandleToLong(hdc);

    dispparams.rgvarg = args;
    dispparams.rgdispidNamedArgs = NULL;
    dispparams.cArgs = 10;
    dispparams.cNamedArgs = 0;

    V_VT(&varresult) = VT_EMPTY;
    hr = IPictureDisp_Invoke(picdisp, DISPID_PICT_RENDER, &GUID_NULL, 0, DISPATCH_METHOD, &dispparams, &varresult, NULL, NULL);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    /* Try with one argument set to VT_I2, it'd still work if coerced. */
    V_VT(&args[3]) = VT_I2;
    hr = IPictureDisp_Invoke(picdisp, DISPID_PICT_RENDER, &GUID_NULL, 0, DISPATCH_METHOD, &dispparams, &varresult, NULL, NULL);
    ok(hr == DISP_E_TYPEMISMATCH, "got 0x%08lx\n", hr);
    V_VT(&args[3]) = VT_I4;

    /* Wrong argument count */
    dispparams.cArgs = 9;
    hr = IPictureDisp_Invoke(picdisp, DISPID_PICT_RENDER, &GUID_NULL, 0, DISPATCH_METHOD, &dispparams, &varresult, NULL, NULL);
    ok(hr == DISP_E_BADPARAMCOUNT, "got 0x%08lx\n", hr);

    delete_render_dc(hdc);
    IPictureDisp_Release(picdisp);
}

static HRESULT create_picture(short type, IPicture **pict)
{
    PICTDESC desc;

    desc.cbSizeofstruct = sizeof(desc);
    desc.picType = type;

    switch (type)
    {
    case PICTYPE_UNINITIALIZED:
        return OleCreatePictureIndirect(NULL, &IID_IPicture, TRUE, (void **)pict);

    case PICTYPE_NONE:
        break;

    case PICTYPE_BITMAP:
        desc.bmp.hbitmap = CreateBitmap(1, 1, 1, 1, NULL);
        desc.bmp.hpal = (HPALETTE)0xbeefdead;
        break;

    case PICTYPE_ICON:
        desc.icon.hicon = LoadIconA(NULL, (LPCSTR)IDI_APPLICATION);
        break;

    case PICTYPE_METAFILE:
    {
        HDC hdc = CreateMetaFileA(NULL);
        desc.wmf.hmeta = CloseMetaFile(hdc);
        desc.wmf.xExt = 1;
        desc.wmf.yExt = 1;
        break;
    }

    case PICTYPE_ENHMETAFILE:
    {
        HDC hdc = CreateEnhMetaFileA(0, NULL, NULL, NULL);
        desc.emf.hemf = CloseEnhMetaFile(hdc);
        break;
    }

    default:
        ok(0, "picture type %d is not supported\n", type);
        return E_NOTIMPL;
    }

    return OleCreatePictureIndirect(&desc, &IID_IPicture, TRUE, (void **)pict);
}

static void test_OleCreatePictureIndirect(void)
{
    PICTDESC desc;
    OLE_HANDLE handle;
    IPicture *pict;
    HRESULT hr;
    short type, i;

if (0)
{
    /* crashes on native */
    OleCreatePictureIndirect(NULL, &IID_IPicture, TRUE, NULL);
}

    desc.cbSizeofstruct = sizeof(desc);
    desc.picType = PICTYPE_UNINITIALIZED;
    pict = (void *)0xdeadbeef;
    hr = OleCreatePictureIndirect(&desc, &IID_IPicture, TRUE, (void **)&pict);
    ok(hr == E_UNEXPECTED, "got %#lx\n", hr);
    ok(pict == NULL, "got %p\n", pict);

    for (i = PICTYPE_UNINITIALIZED; i <= PICTYPE_ENHMETAFILE; i++)
    {
        hr = create_picture(i, &pict);
        ok(hr == S_OK, "%d: got %#lx\n", i, hr);

        type = 0xdead;
        hr = IPicture_get_Type(pict, &type);
        ok(hr == S_OK, "%d: got %#lx\n", i, hr);
        ok(type == i, "%d: got %d\n", i, type);

        handle = 0xdeadbeef;
        hr = IPicture_get_Handle(pict, &handle);
        ok(hr == S_OK, "%d: got %#lx\n", i, hr);
        if (type == PICTYPE_UNINITIALIZED || type == PICTYPE_NONE)
            ok(handle == 0, "%d: got %#x\n", i, handle);
        else
            ok(handle != 0 && handle != 0xdeadbeef, "%d: got %#x\n", i, handle);

        handle = 0xdeadbeef;
        hr = IPicture_get_hPal(pict, &handle);
        if (type == PICTYPE_BITMAP)
        {
            ok(hr == S_OK, "%d: got %#lx\n", i, hr);
            ok(handle == 0xbeefdead, "%d: got %#x\n", i, handle);
        }
        else
        {
            ok(hr == E_FAIL, "%d: got %#lx\n", i, hr);
            ok(handle == 0xdeadbeef || handle == 0 /* win64 */, "%d: got %#x\n", i, handle);
        }

        hr = IPicture_set_hPal(pict, HandleToUlong(GetStockObject(DEFAULT_PALETTE)));
        if (type == PICTYPE_BITMAP)
            ok(hr == S_OK, "%d: got %#lx\n", i, hr);
        else
            ok(hr == E_FAIL, "%d: got %#lx\n", i, hr);

        IPicture_Release(pict);
    }
}

static void test_apm(void)
{
    OLE_HANDLE handle;
    LPSTREAM stream;
    IPicture *pict;
    HGLOBAL hglob;
    LPBYTE *data;
    LONG cxy;
    BOOL keep;
    short type;

    hglob = GlobalAlloc (0, sizeof(apmdata));
    data = GlobalLock(hglob);
    memcpy(data, apmdata, sizeof(apmdata));

    ole_check(CreateStreamOnHGlobal(hglob, TRUE, &stream));
    ole_check(pOleLoadPictureEx(stream, sizeof(apmdata), TRUE, &IID_IPicture, 100, 100, 0, (LPVOID *)&pict));

    ole_check(IPicture_get_Handle(pict, &handle));
    ok(handle != 0, "handle is null\n");

    ole_check(IPicture_get_Type(pict, &type));
    expect_eq(type, PICTYPE_METAFILE, short, "%d");

    ole_check(IPicture_get_Height(pict, &cxy));
    expect_eq(cxy,  1667l, LONG, "%ld");

    ole_check(IPicture_get_Width(pict, &cxy));
    expect_eq(cxy,  1323l, LONG, "%ld");

    ole_check(IPicture_get_KeepOriginalFormat(pict, &keep));
    todo_wine expect_eq(keep, (LONG)FALSE, LONG, "%ld");

    ole_expect(IPicture_get_hPal(pict, &handle), E_FAIL);
    IPicture_Release(pict);
    IStream_Release(stream);
}

static void test_metafile(void)
{
    LPSTREAM stream;
    IPicture *pict;
    HGLOBAL hglob;
    LPBYTE *data;

    hglob = GlobalAlloc (0, sizeof(metafile));
    data = GlobalLock(hglob);
    memcpy(data, metafile, sizeof(metafile));

    ole_check(CreateStreamOnHGlobal(hglob, TRUE, &stream));
    /* Windows does not load simple metafiles */
    ole_expect(pOleLoadPictureEx(stream, sizeof(metafile), TRUE, &IID_IPicture, 100, 100, 0, (LPVOID *)&pict), E_FAIL);

    IStream_Release(stream);
}

static void test_enhmetafile(void)
{
    OLE_HANDLE handle;
    LPSTREAM stream;
    IPicture *pict;
    HGLOBAL hglob;
    LPBYTE *data;
    LONG cxy;
    BOOL keep;
    short type;

    hglob = GlobalAlloc (0, sizeof(enhmetafile));
    data = GlobalLock(hglob);
    memcpy(data, enhmetafile, sizeof(enhmetafile));

    ole_check(CreateStreamOnHGlobal(hglob, TRUE, &stream));
    ole_check(pOleLoadPictureEx(stream, sizeof(enhmetafile), TRUE, &IID_IPicture, 10, 10, 0, (LPVOID *)&pict));

    ole_check(IPicture_get_Handle(pict, &handle));
    ok(handle != 0, "handle is null\n");

    ole_check(IPicture_get_Type(pict, &type));
    expect_eq(type, PICTYPE_ENHMETAFILE, short, "%d");

    ole_check(IPicture_get_Height(pict, &cxy));
    expect_eq(cxy, -23l, LONG, "%ld");

    ole_check(IPicture_get_Width(pict, &cxy));
    expect_eq(cxy, -25l, LONG, "%ld");

    ole_check(IPicture_get_KeepOriginalFormat(pict, &keep));
    todo_wine expect_eq(keep, (LONG)FALSE, LONG, "%ld");

    IPicture_Release(pict);
    IStream_Release(stream);
}

static HRESULT picture_render(IPicture *iface, HDC hdc, LONG x, LONG y, LONG cx, LONG cy,
                              OLE_XPOS_HIMETRIC xSrc,
                              OLE_YPOS_HIMETRIC ySrc,
                              OLE_XSIZE_HIMETRIC cxSrc,
                              OLE_YSIZE_HIMETRIC cySrc,
                              const RECT *bounds)
{
    VARIANT ret, args[10];
    HRESULT hr, hr_disp;
    DISPPARAMS params;
    IDispatch *disp;
    int i;

    hr = IPicture_Render(iface, hdc, x, y, cx, cy, xSrc, ySrc, cxSrc, cySrc, bounds);

    IPicture_QueryInterface(iface, &IID_IDispatch, (void**)&disp);

    /* This is broken on 64 bits - accepted pointer argument type is still VT_I4 */
    for (i = 0; i < ARRAY_SIZE(args); i++)
        V_VT(&args[i]) = VT_I4;

    /* pack arguments and call */
    V_INT_PTR(&args[0]) = (INT_PTR)bounds;
    V_I4(&args[1]) = cySrc;
    V_I4(&args[2]) = cxSrc;
    V_I4(&args[3]) = ySrc;
    V_I4(&args[4]) = xSrc;
    V_I4(&args[5]) = cy;
    V_I4(&args[6]) = cx;
    V_I4(&args[7]) = y;
    V_I4(&args[8]) = x;
    V_I4(&args[9]) = HandleToLong(hdc);

    params.rgvarg = args;
    params.rgdispidNamedArgs = NULL;
    params.cArgs = 10;
    params.cNamedArgs = 0;

    V_VT(&ret) = VT_EMPTY;
    hr_disp = IDispatch_Invoke(disp, DISPID_PICT_RENDER, &GUID_NULL, 0, DISPATCH_METHOD,
        &params, &ret, NULL, NULL);
    ok(hr == hr_disp, "DISPID_PICT_RENDER returned wrong code, 0x%08lx, expected 0x%08lx\n",
       hr_disp, hr);

    IDispatch_Release(disp);

    return hr;
}

static void test_Render(void)
{
    IPicture *pic;
    HRESULT hres;
    short type;
    PICTDESC desc;
    OLE_XSIZE_HIMETRIC pWidth;
    OLE_YSIZE_HIMETRIC pHeight;
    COLORREF result, expected;
    HDC hdc = create_render_dc();

    /* test IPicture::Render return code on uninitialized picture */
    hres = OleCreatePictureIndirect(NULL, &IID_IPicture, TRUE, (void **)&pic);
    ok(hres == S_OK, "Failed to create a picture, hr %#lx.\n", hres);
    hres = IPicture_get_Type(pic, &type);
    ok(hres == S_OK, "IPicture_get_Type does not return S_OK, but 0x%08lx\n", hres);
    ok(type == PICTYPE_UNINITIALIZED, "Expected type = PICTYPE_UNINITIALIZED, got = %d\n", type);
    /* zero dimensions */
    hres = picture_render(pic, hdc, 0, 0, 0, 0, 0, 0, 0, 0, NULL);
    ole_expect(hres, CTL_E_INVALIDPROPERTYVALUE);
    hres = picture_render(pic, hdc, 0, 0, 10, 10, 0, 0, 10, 0, NULL);
    ole_expect(hres, CTL_E_INVALIDPROPERTYVALUE);
    hres = picture_render(pic, hdc, 0, 0, 10, 10, 0, 0, 0, 10, NULL);
    ole_expect(hres, CTL_E_INVALIDPROPERTYVALUE);
    hres = picture_render(pic, hdc, 0, 0, 10, 10, 0, 0, 0, 0, NULL);
    ole_expect(hres, CTL_E_INVALIDPROPERTYVALUE);
    hres = picture_render(pic, hdc, 0, 0, 0, 10, 0, 0, 10, 10, NULL);
    ole_expect(hres, CTL_E_INVALIDPROPERTYVALUE);
    hres = picture_render(pic, hdc, 0, 0, 10, 0, 0, 0, 10, 10, NULL);
    ole_expect(hres, CTL_E_INVALIDPROPERTYVALUE);
    hres = picture_render(pic, hdc, 0, 0, 0, 0, 0, 0, 10, 10, NULL);
    ole_expect(hres, CTL_E_INVALIDPROPERTYVALUE);
    /* nonzero dimensions, PICTYPE_UNINITIALIZED */
    hres = picture_render(pic, hdc, 0, 0, 10, 10, 0, 0, 10, 10, NULL);
    ole_expect(hres, S_OK);
    IPicture_Release(pic);

    desc.cbSizeofstruct = sizeof(PICTDESC);
    desc.picType = PICTYPE_ICON;
    desc.icon.hicon = LoadIconA(NULL, (LPCSTR)IDI_APPLICATION);
    if(!desc.icon.hicon){
        win_skip("LoadIcon failed. Skipping...\n");
        delete_render_dc(hdc);
        return;
    }

    hres = OleCreatePictureIndirect(&desc, &IID_IPicture, TRUE, (void **)&pic);
    ok(hres == S_OK, "Failed to create a picture, hr %#lx.\n", hres);
    /* zero dimensions, PICTYPE_ICON */
    hres = picture_render(pic, hdc, 0, 0, 0, 0, 0, 0, 0, 0, NULL);
    ole_expect(hres, CTL_E_INVALIDPROPERTYVALUE);
    hres = picture_render(pic, hdc, 0, 0, 10, 10, 0, 0, 10, 0, NULL);
    ole_expect(hres, CTL_E_INVALIDPROPERTYVALUE);
    hres = picture_render(pic, hdc, 0, 0, 10, 10, 0, 0, 0, 10, NULL);
    ole_expect(hres, CTL_E_INVALIDPROPERTYVALUE);
    hres = picture_render(pic, hdc, 0, 0, 10, 10, 0, 0, 0, 0, NULL);
    ole_expect(hres, CTL_E_INVALIDPROPERTYVALUE);
    hres = picture_render(pic, hdc, 0, 0, 0, 10, 0, 0, 10, 10, NULL);
    ole_expect(hres, CTL_E_INVALIDPROPERTYVALUE);
    hres = picture_render(pic, hdc, 0, 0, 10, 0, 0, 0, 10, 10, NULL);
    ole_expect(hres, CTL_E_INVALIDPROPERTYVALUE);
    hres = picture_render(pic, hdc, 0, 0, 0, 0, 0, 0, 10, 10, NULL);
    ole_expect(hres, CTL_E_INVALIDPROPERTYVALUE);

    /* Check if target size and position is respected */
    IPicture_get_Width(pic, &pWidth);
    IPicture_get_Height(pic, &pHeight);

    SetPixelV(hdc, 0, 0, 0x00223344);
    SetPixelV(hdc, 5, 5, 0x00223344);
    SetPixelV(hdc, 10, 10, 0x00223344);
    expected = GetPixel(hdc, 0, 0);

    hres = picture_render(pic, hdc, 1, 1, 9, 9, 0, pHeight, pWidth, -pHeight, NULL);
    ole_expect(hres, S_OK);

    if(hres != S_OK) goto done;

    /* Evaluate the rendered Icon */
    result = GetPixel(hdc, 0, 0);
    ok(result == expected,
       "Color at 0,0 should be unchanged 0x%06lX, but was 0x%06lX\n", expected, result);
    result = GetPixel(hdc, 5, 5);
    ok(result != expected,
       "Color at 5,5 should have changed, but still was 0x%06lX\n", expected);
    result = GetPixel(hdc, 10, 10);
    ok(result == expected,
       "Color at 10,10 should be unchanged 0x%06lX, but was 0x%06lX\n", expected, result);

done:
    IPicture_Release(pic);
    delete_render_dc(hdc);
}

static void test_get_Attributes(void)
{
    IPicture *pic;
    HRESULT hres;
    short type;
    DWORD attr;

    hres = OleCreatePictureIndirect(NULL, &IID_IPicture, TRUE, (void **)&pic);
    ok(hres == S_OK, "Failed to create a picture, hr %#lx.\n", hres);
    hres = IPicture_get_Type(pic, &type);
    ok(hres == S_OK, "IPicture_get_Type does not return S_OK, but 0x%08lx\n", hres);
    ok(type == PICTYPE_UNINITIALIZED, "Expected type = PICTYPE_UNINITIALIZED, got = %d\n", type);

    hres = IPicture_get_Attributes(pic, NULL);
    ole_expect(hres, E_POINTER);

    attr = 0xdeadbeef;
    hres = IPicture_get_Attributes(pic, &attr);
    ole_expect(hres, S_OK);
    ok(attr == 0, "IPicture_get_Attributes does not reset attr to zero, got %ld\n", attr);

    IPicture_Release(pic);
}

static void test_get_Handle(void)
{
    IPicture *pic;
    HRESULT hres;

    hres = OleCreatePictureIndirect(NULL, &IID_IPicture, TRUE, (void **)&pic);
    ok(hres == S_OK, "Failed to create a picture, hr %#lx.\n", hres);
    hres = IPicture_get_Handle(pic, NULL);
    ole_expect(hres, E_POINTER);

    IPicture_Release(pic);
}

static void test_get_Type(void)
{
    IPicture *pic;
    HRESULT hres;

    hres = OleCreatePictureIndirect(NULL, &IID_IPicture, TRUE, (void **)&pic);
    ok(hres == S_OK, "Failed to create a picture, hr %#lx.\n", hres);

    hres = IPicture_get_Type(pic, NULL);
    ole_expect(hres, E_POINTER);

    IPicture_Release(pic);
}

static void test_OleLoadPicturePath(void)
{
    static WCHAR emptyW[] = {0};

    IPicture *pic;
    HRESULT hres;
    int i;
    char temp_path[MAX_PATH];
    char temp_file[MAX_PATH];
    WCHAR temp_fileW[MAX_PATH + 5] = {'f','i','l','e',':','/','/','/'};
    HANDLE file;
    DWORD size;
    WCHAR *ptr;
    VARIANT var;

    const struct
    {
        LPOLESTR szURLorPath;
        REFIID riid;
        IPicture **pic;
    } invalid_parameters[] =
    {
        {NULL,  NULL,          NULL},
        {NULL,  NULL,          &pic},
        {NULL,  &IID_IPicture, NULL},
        {NULL,  &IID_IPicture, &pic},
        {emptyW, NULL,          NULL},
        {emptyW, &IID_IPicture, NULL},
    };

    for (i = 0; i < ARRAY_SIZE(invalid_parameters); i++)
    {
        pic = (IPicture *)0xdeadbeef;
        hres = OleLoadPicturePath(invalid_parameters[i].szURLorPath, NULL, 0, 0,
                                  invalid_parameters[i].riid,
                                  (void **)invalid_parameters[i].pic);
        ok(hres == E_INVALIDARG,
           "[%d] Expected OleLoadPicturePath to return E_INVALIDARG, got 0x%08lx\n", i, hres);
        ok(pic == (IPicture *)0xdeadbeef,
           "[%d] Expected output pointer to be 0xdeadbeef, got %p\n", i, pic);
    }

    pic = (IPicture *)0xdeadbeef;
    hres = OleLoadPicturePath(emptyW, NULL, 0, 0, NULL, (void **)&pic);
    todo_wine
    ok(hres == INET_E_UNKNOWN_PROTOCOL || /* XP/Vista+ */
       broken(hres == E_UNEXPECTED) || /* NT4 */
       broken(hres == E_OUTOFMEMORY), /* Win2k/Win2k3 */
       "Expected OleLoadPicturePath to return INET_E_UNKNOWN_PROTOCOL, got 0x%08lx\n", hres);
    ok(pic == NULL,
       "Expected the output interface pointer to be NULL, got %p\n", pic);

    pic = (IPicture *)0xdeadbeef;
    hres = OleLoadPicturePath(emptyW, NULL, 0, 0, &IID_IPicture, (void **)&pic);
    todo_wine
    ok(hres == INET_E_UNKNOWN_PROTOCOL || /* XP/Vista+ */
       broken(hres == E_UNEXPECTED) || /* NT4 */
       broken(hres == E_OUTOFMEMORY), /* Win2k/Win2k3 */
       "Expected OleLoadPicturePath to return INET_E_UNKNOWN_PROTOCOL, got 0x%08lx\n", hres);
    ok(pic == NULL,
       "Expected the output interface pointer to be NULL, got %p\n", pic);

    /* Create a local temporary image file for testing. */
    GetTempPathA(sizeof(temp_path), temp_path);
    GetTempFileNameA(temp_path, "bmp", 0, temp_file);
    file = CreateFileA(temp_file, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL, NULL);
    WriteFile(file, bmpimage, sizeof(bmpimage), &size, NULL);
    CloseHandle(file);

    MultiByteToWideChar(CP_ACP, 0, temp_file, -1, temp_fileW + 8, ARRAY_SIZE(temp_fileW) - 8);

    /* Try a normal DOS path. */
    hres = OleLoadPicturePath(temp_fileW + 8, NULL, 0, 0, &IID_IPicture, (void **)&pic);
    ok(hres == S_OK ||
       broken(hres == E_UNEXPECTED), /* NT4 */
       "Expected OleLoadPicturePath to return S_OK, got 0x%08lx\n", hres);
    if (pic)
        IPicture_Release(pic);

    VariantInit(&var);
    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(temp_fileW + 8);
    hres = OleLoadPictureFile(var, (IDispatch **)&pic);
    ok(hres == S_OK, "OleLoadPictureFile error %#lx\n", hres);
    IPicture_Release(pic);
    VariantClear(&var);

    /* Try a DOS path with tacked on "file:". */
    hres = OleLoadPicturePath(temp_fileW, NULL, 0, 0, &IID_IPicture, (void **)&pic);
    ok(hres == S_OK ||
       broken(hres == E_UNEXPECTED), /* NT4 */
       "Expected OleLoadPicturePath to return S_OK, got 0x%08lx\n", hres);
    if (pic)
        IPicture_Release(pic);

    VariantInit(&var);
    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(temp_fileW);
    hres = OleLoadPictureFile(var, (IDispatch **)&pic);
    ok(hres == CTL_E_PATHFILEACCESSERROR, "wrong error %#lx\n", hres);
    VariantClear(&var);

    DeleteFileA(temp_file);

    /* Try with a nonexistent file. */
    hres = OleLoadPicturePath(temp_fileW + 8, NULL, 0, 0, &IID_IPicture, (void **)&pic);
    ok(hres == INET_E_RESOURCE_NOT_FOUND || /* XP+ */
       broken(hres == E_UNEXPECTED) || /* NT4 */
       broken(hres == E_FAIL), /*Win2k */
       "Expected OleLoadPicturePath to return INET_E_RESOURCE_NOT_FOUND, got 0x%08lx\n", hres);

    VariantInit(&var);
    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(temp_fileW + 8);
    hres = OleLoadPictureFile(var, (IDispatch **)&pic);
    ok(hres == CTL_E_FILENOTFOUND, "wrong error %#lx\n", hres);
    VariantClear(&var);

    hres = OleLoadPicturePath(temp_fileW, NULL, 0, 0, &IID_IPicture, (void **)&pic);
    ok(hres == INET_E_RESOURCE_NOT_FOUND || /* XP+ */
       broken(hres == E_UNEXPECTED) || /* NT4 */
       broken(hres == E_FAIL), /* Win2k */
       "Expected OleLoadPicturePath to return INET_E_RESOURCE_NOT_FOUND, got 0x%08lx\n", hres);

    VariantInit(&var);
    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(temp_fileW);
    hres = OleLoadPictureFile(var, (IDispatch **)&pic);
    ok(hres == CTL_E_PATHFILEACCESSERROR, "wrong error %#lx\n", hres);
    VariantClear(&var);

    file = CreateFileA(temp_file, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL, NULL);
    WriteFile(file, bmpimage, sizeof(bmpimage), &size, NULL);
    CloseHandle(file);

    /* Try a "file:" URL with slash separators. */
    ptr = temp_fileW + 8;
    while (*ptr)
    {
        if (*ptr == '\\')
            *ptr = '/';
        ptr++;
    }

    hres = OleLoadPicturePath(temp_fileW, NULL, 0, 0, &IID_IPicture, (void **)&pic);
    ok(hres == S_OK ||
       broken(hres == E_UNEXPECTED), /* NT4 */
       "Expected OleLoadPicturePath to return S_OK, got 0x%08lx\n", hres);
    if (pic)
        IPicture_Release(pic);

    VariantInit(&var);
    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(temp_fileW);
    hres = OleLoadPictureFile(var, (IDispatch **)&pic);
    ok(hres == CTL_E_PATHFILEACCESSERROR, "wrong error %#lx\n", hres);
    VariantClear(&var);

    DeleteFileA(temp_file);

    /* Try with a nonexistent file. */
    hres = OleLoadPicturePath(temp_fileW, NULL, 0, 0, &IID_IPicture, (void **)&pic);
    ok(hres == INET_E_RESOURCE_NOT_FOUND || /* XP+ */
       broken(hres == E_UNEXPECTED) || /* NT4 */
       broken(hres == E_FAIL), /* Win2k */
       "Expected OleLoadPicturePath to return INET_E_RESOURCE_NOT_FOUND, got 0x%08lx\n", hres);

    VariantInit(&var);
    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(temp_fileW);
    hres = OleLoadPictureFile(var, (IDispatch **)&pic);
    ok(hres == CTL_E_PATHFILEACCESSERROR, "wrong error %#lx\n", hres);
    VariantClear(&var);

    VariantInit(&var);
    V_VT(&var) = VT_INT;
    V_INT(&var) = 762;
    hres = OleLoadPictureFile(var, (IDispatch **)&pic);
    ok(hres == CTL_E_FILENOTFOUND, "wrong error %#lx\n", hres);

    if (0) /* crashes under Windows */
    hres = OleLoadPictureFile(var, NULL);
}

static void test_himetric(void)
{
    static const BYTE bmp_bits[1024];
    OLE_XSIZE_HIMETRIC cx;
    OLE_YSIZE_HIMETRIC cy;
    IPicture *pic;
    PICTDESC desc;
    HBITMAP bmp;
    HRESULT hr;
    HICON icon;
    HDC hdc;
    INT d;

    desc.cbSizeofstruct = sizeof(desc);
    desc.picType = PICTYPE_BITMAP;
    desc.bmp.hpal = NULL;

    hdc = CreateCompatibleDC(0);

    bmp = CreateBitmap(1.9 * GetDeviceCaps(hdc, LOGPIXELSX),
                       1.9 * GetDeviceCaps(hdc, LOGPIXELSY), 1, 1, NULL);

    desc.bmp.hbitmap = bmp;

    /* size in himetric units reported rounded up to next integer value */
    hr = OleCreatePictureIndirect(&desc, &IID_IPicture, FALSE, (void**)&pic);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    cx = 0;
    d = MulDiv((INT)(1.9 * GetDeviceCaps(hdc, LOGPIXELSX)), 2540, GetDeviceCaps(hdc, LOGPIXELSX));
    hr = IPicture_get_Width(pic, &cx);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(cx == d, "got %ld, expected %d\n", cx, d);

    cy = 0;
    d = MulDiv((INT)(1.9 * GetDeviceCaps(hdc, LOGPIXELSY)), 2540, GetDeviceCaps(hdc, LOGPIXELSY));
    hr = IPicture_get_Height(pic, &cy);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(cy == d, "got %ld, expected %d\n", cy, d);

    DeleteObject(bmp);
    IPicture_Release(pic);

    /* same thing with icon */
    icon = CreateIcon(NULL, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON),
                      1, 1, bmp_bits, bmp_bits);
    ok(icon != NULL, "failed to create icon\n");

    desc.picType = PICTYPE_ICON;
    desc.icon.hicon = icon;

    hr = OleCreatePictureIndirect(&desc, &IID_IPicture, FALSE, (void**)&pic);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    cx = 0;
    d = MulDiv(GetSystemMetrics(SM_CXICON), 2540, GetDeviceCaps(hdc, LOGPIXELSX));
    hr = IPicture_get_Width(pic, &cx);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(cx == d, "got %ld, expected %d\n", cx, d);

    cy = 0;
    d = MulDiv(GetSystemMetrics(SM_CYICON), 2540, GetDeviceCaps(hdc, LOGPIXELSY));
    hr = IPicture_get_Height(pic, &cy);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(cy == d, "got %ld, expected %d\n", cy, d);

    IPicture_Release(pic);
    DestroyIcon(icon);

    DeleteDC(hdc);
}

static void test_load_save_bmp(void)
{
    IPicture *pic;
    PICTDESC desc;
    short type;
    OLE_HANDLE handle;
    HGLOBAL hmem;
    DWORD *mem;
    IPersistStream *src_stream;
    IStream *dst_stream;
    LARGE_INTEGER offset;
    HRESULT hr;
    LONG size;
    ULARGE_INTEGER maxsize;

    desc.cbSizeofstruct = sizeof(desc);
    desc.picType = PICTYPE_BITMAP;
    desc.bmp.hpal = 0;
    desc.bmp.hbitmap = CreateBitmap(1, 1, 1, 1, NULL);
    hr = OleCreatePictureIndirect(&desc, &IID_IPicture, FALSE, (void**)&pic);
    ok(hr == S_OK, "OleCreatePictureIndirect error %#lx\n", hr);

    type = -1;
    hr = IPicture_get_Type(pic, &type);
    ok(hr == S_OK,"get_Type error %#8lx\n", hr);
    ok(type == PICTYPE_BITMAP,"expected picture type PICTYPE_BITMAP, got %d\n", type);

    hr = IPicture_get_Handle(pic, &handle);
    ok(hr == S_OK,"get_Handle error %#8lx\n", hr);
    ok(IntToPtr(handle) == desc.bmp.hbitmap, "get_Handle returned wrong handle %#x\n", handle);

    hmem = GlobalAlloc(GMEM_ZEROINIT, 4096);
    hr = CreateStreamOnHGlobal(hmem, FALSE, &dst_stream);
    ok(hr == S_OK, "createstreamonhglobal error %#lx\n", hr);

    size = -1;
    hr = IPicture_SaveAsFile(pic, dst_stream, TRUE, &size);
    ok(hr == S_OK, "IPicture_SaveasFile error %#lx\n", hr);
    ok(size == 66, "expected 66, got %ld\n", size);
    mem = GlobalLock(hmem);
    ok(!memcmp(&mem[0], "BM", 2), "got wrong bmp header %04lx\n", mem[0]);
    GlobalUnlock(hmem);

    size = -1;
    hr = IPicture_SaveAsFile(pic, dst_stream, FALSE, &size);
    ok(hr == E_FAIL, "expected E_FAIL, got %#lx\n", hr);
    ok(size == -1, "expected -1, got %ld\n", size);

    offset.QuadPart = 0;
    hr = IStream_Seek(dst_stream, offset, STREAM_SEEK_SET, NULL);
    ok(hr == S_OK, "IStream_Seek %#lx\n", hr);

    hr = IPicture_QueryInterface(pic, &IID_IPersistStream, (void **)&src_stream);
    ok(hr == S_OK, "QueryInterface error %#lx\n", hr);

    maxsize.QuadPart = 0;
    hr = IPersistStream_GetSizeMax(src_stream, &maxsize);
    ok(hr == S_OK, "GetSizeMax error %#lx\n", hr);
    ok(maxsize.QuadPart == 74, "expected 74, got %s\n", wine_dbgstr_longlong(maxsize.QuadPart));

    hr = IPersistStream_Save(src_stream, dst_stream, TRUE);
    ok(hr == S_OK, "Save error %#lx\n", hr);

    maxsize.QuadPart = 0;
    hr = IPersistStream_GetSizeMax(src_stream, &maxsize);
    ok(hr == S_OK, "GetSizeMax error %#lx\n", hr);
    ok(maxsize.QuadPart == 74, "expected 74, got %s\n", wine_dbgstr_longlong(maxsize.QuadPart));

    IPersistStream_Release(src_stream);
    IStream_Release(dst_stream);

    mem = GlobalLock(hmem);
    ok(!memcmp(mem, "lt\0\0", 4), "got wrong stream header %04lx\n", mem[0]);
    ok(mem[1] == 66, "expected stream size 66, got %lu\n", mem[1]);
    ok(!memcmp(&mem[2], "BM", 2), "got wrong bmp header %04lx\n", mem[2]);

    GlobalUnlock(hmem);
    GlobalFree(hmem);

    DeleteObject(desc.bmp.hbitmap);
    IPicture_Release(pic);
}

static void test_load_save_icon(void)
{
    IPicture *pic;
    PICTDESC desc;
    short type;
    OLE_HANDLE handle;
    HGLOBAL hmem;
    DWORD *mem;
    IPersistStream *src_stream;
    IStream *dst_stream;
    LARGE_INTEGER offset;
    HRESULT hr;
    LONG size;
    ULARGE_INTEGER maxsize;

    desc.cbSizeofstruct = sizeof(desc);
    desc.picType = PICTYPE_ICON;
    desc.icon.hicon = LoadIconA(NULL, (LPCSTR)IDI_APPLICATION);
    hr = OleCreatePictureIndirect(&desc, &IID_IPicture, FALSE, (void**)&pic);
    ok(hr == S_OK, "OleCreatePictureIndirect error %#lx\n", hr);

    type = -1;
    hr = IPicture_get_Type(pic, &type);
    ok(hr == S_OK,"get_Type error %#8lx\n", hr);
    ok(type == PICTYPE_ICON,"expected picture type PICTYPE_ICON, got %d\n", type);

    hr = IPicture_get_Handle(pic, &handle);
    ok(hr == S_OK,"get_Handle error %#8lx\n", hr);
    ok(IntToPtr(handle) == desc.icon.hicon, "get_Handle returned wrong handle %#x\n", handle);

    hmem = GlobalAlloc(GMEM_ZEROINIT, 8192);
    hr = CreateStreamOnHGlobal(hmem, FALSE, &dst_stream);
    ok(hr == S_OK, "CreateStreamOnHGlobal error %#lx\n", hr);

    size = -1;
    hr = IPicture_SaveAsFile(pic, dst_stream, TRUE, &size);
    ok(hr == S_OK, "IPicture_SaveasFile error %#lx\n", hr);
    todo_wine
    ok(size == 766, "expected 766, got %ld\n", size);
    mem = GlobalLock(hmem);
    ok(mem[0] == 0x00010000, "got wrong icon header %04lx\n", mem[0]);
    GlobalUnlock(hmem);

    size = -1;
    hr = IPicture_SaveAsFile(pic, dst_stream, FALSE, &size);
    ok(hr == E_FAIL, "expected E_FAIL, got %#lx\n", hr);
    ok(size == -1, "expected -1, got %ld\n", size);

    offset.QuadPart = 0;
    hr = IStream_Seek(dst_stream, offset, STREAM_SEEK_SET, NULL);
    ok(hr == S_OK, "IStream_Seek %#lx\n", hr);

    hr = IPicture_QueryInterface(pic, &IID_IPersistStream, (void **)&src_stream);
    ok(hr == S_OK, "QueryInterface error %#lx\n", hr);

    maxsize.QuadPart = 0;
    hr = IPersistStream_GetSizeMax(src_stream, &maxsize);
    todo_wine
    ok(hr == S_OK, "GetSizeMax error %#lx\n", hr);
    todo_wine
    ok(maxsize.QuadPart == 774, "expected 774, got %s\n", wine_dbgstr_longlong(maxsize.QuadPart));

    hr = IPersistStream_Save(src_stream, dst_stream, TRUE);
    ok(hr == S_OK, "Saveerror %#lx\n", hr);

    maxsize.QuadPart = 0;
    hr = IPersistStream_GetSizeMax(src_stream, &maxsize);
    todo_wine
    ok(hr == S_OK, "GetSizeMax error %#lx\n", hr);
    todo_wine
    ok(maxsize.QuadPart == 774, "expected 774, got %s\n", wine_dbgstr_longlong(maxsize.QuadPart));

    IPersistStream_Release(src_stream);
    IStream_Release(dst_stream);

    mem = GlobalLock(hmem);
    ok(!memcmp(mem, "lt\0\0", 4), "got wrong stream header %04lx\n", mem[0]);
    todo_wine
    ok(mem[1] == 766, "expected stream size 766, got %lu\n", mem[1]);
    ok(mem[2] == 0x00010000, "got wrong icon header %04lx\n", mem[2]);

    GlobalUnlock(hmem);
    GlobalFree(hmem);

    DestroyIcon(desc.icon.hicon);
    IPicture_Release(pic);
}

static void test_load_save_empty_picture(void)
{
    IPicture *pic;
    PICTDESC desc;
    short type;
    OLE_HANDLE handle;
    HGLOBAL hmem;
    DWORD *mem;
    IPersistStream *src_stream;
    IStream *dst_stream, *stream;
    LARGE_INTEGER offset;
    HRESULT hr;
    LONG size;
    ULARGE_INTEGER maxsize;

    memset(&pic, 0, sizeof(pic));
    desc.cbSizeofstruct = sizeof(desc);
    desc.picType = PICTYPE_NONE;
    hr = OleCreatePictureIndirect(&desc, &IID_IPicture, FALSE, (void **)&pic);
    ok(hr == S_OK, "OleCreatePictureIndirect error %#lx\n", hr);

    type = -1;
    hr = IPicture_get_Type(pic, &type);
    ok(hr == S_OK, "get_Type error %#lx\n", hr);
    ok(type == PICTYPE_NONE,"expected picture type PICTYPE_NONE, got %d\n", type);

    handle = (OLE_HANDLE)0xdeadbeef;
    hr = IPicture_get_Handle(pic, &handle);
    ok(hr == S_OK,"get_Handle error %#8lx\n", hr);
    ok(!handle, "get_Handle returned wrong handle %#x\n", handle);

    hmem = GlobalAlloc(GMEM_ZEROINIT, 4096);
    hr = CreateStreamOnHGlobal(hmem, FALSE, &dst_stream);
    ok(hr == S_OK, "createstreamonhglobal error %#lx\n", hr);

    size = -1;
    hr = IPicture_SaveAsFile(pic, dst_stream, TRUE, &size);
    ok(hr == S_OK, "IPicture_SaveasFile error %#lx\n", hr);
    ok(size == -1, "expected -1, got %ld\n", size);

    size = -1;
    hr = IPicture_SaveAsFile(pic, dst_stream, FALSE, &size);
    ok(hr == S_OK, "IPicture_SaveasFile error %#lx\n", hr);
    ok(size == -1, "expected -1, got %ld\n", size);

    hr = IPicture_QueryInterface(pic, &IID_IPersistStream, (void **)&src_stream);
    ok(hr == S_OK, "QueryInterface error %#lx\n", hr);

    maxsize.QuadPart = 0;
    hr = IPersistStream_GetSizeMax(src_stream, &maxsize);
    ok(hr == S_OK, "GetSizeMax error %#lx\n", hr);
    ok(maxsize.QuadPart == 8, "expected 8, got %s\n", wine_dbgstr_longlong(maxsize.QuadPart));

    hr = IPersistStream_GetSizeMax(src_stream, NULL);
    ole_expect(hr, E_INVALIDARG);

    hr = IPersistStream_Save(src_stream, dst_stream, TRUE);
    ok(hr == S_OK, "Save error %#lx\n", hr);

    maxsize.QuadPart = 0;
    hr = IPersistStream_GetSizeMax(src_stream, &maxsize);
    ok(hr == S_OK, "GetSizeMax error %#lx\n", hr);
    ok(maxsize.QuadPart == 8, "expected 8, got %s\n", wine_dbgstr_longlong(maxsize.QuadPart));

    mem = GlobalLock(hmem);
    ok(!memcmp(mem, "lt\0\0", 4), "got wrong stream header %04lx\n", mem[0]);
    ok(mem[1] == 0, "expected stream size 0, got %lu\n", mem[1]);
    GlobalUnlock(hmem);

    IPersistStream_Release(src_stream);
    IPicture_Release(pic);

    /* first with statable and seekable stream */
    offset.QuadPart = 0;
    hr = IStream_Seek(dst_stream, offset, STREAM_SEEK_SET, NULL);
    ok(hr == S_OK, "IStream_Seek %#lx\n", hr);

    pic = NULL;
    hr = pOleLoadPicture(dst_stream, 0, FALSE, &IID_IPicture, (void **)&pic);
    ok(hr == S_OK, "OleLoadPicture error %#lx\n", hr);
    ok(pic != NULL,"picture should not be not NULL\n");
    if (pic != NULL)
    {
        type = -1;
        hr = IPicture_get_Type(pic, &type);
        ok(hr == S_OK,"get_Type error %#8lx\n", hr);
        ok(type == PICTYPE_NONE,"expected picture type PICTYPE_NONE, got %d\n", type);

        handle = (OLE_HANDLE)0xdeadbeef;
        hr = IPicture_get_Handle(pic, &handle);
        ok(hr == S_OK,"get_Handle error %#8lx\n", hr);
        ok(!handle, "get_Handle returned wrong handle %#x\n", handle);

        IPicture_Release(pic);
    }
    IStream_Release(dst_stream);

    /* again with non-statable and non-seekable stream */
    stream = NoStatStream_Construct(hmem);
    ok(stream != NULL, "failed to create empty image stream\n");

    pic = NULL;
    hr = pOleLoadPicture(stream, 0, FALSE, &IID_IPicture, (void **)&pic);
    ok(hr == S_OK, "OleLoadPicture error %#lx\n", hr);
    ok(pic != NULL,"picture should not be not NULL\n");
    if (pic != NULL)
    {
        type = -1;
        hr = IPicture_get_Type(pic, &type);
        ok(hr == S_OK,"get_Type error %#8lx\n", hr);
        ok(type == PICTYPE_NONE,"expected picture type PICTYPE_NONE, got %d\n", type);

        handle = (OLE_HANDLE)0xdeadbeef;
        hr = IPicture_get_Handle(pic, &handle);
        ok(hr == S_OK,"get_Handle error %#8lx\n", hr);
        ok(!handle, "get_Handle returned wrong handle %#x\n", handle);

        IPicture_Release(pic);
    }
    /* Non-statable impl always deletes on release */
    IStream_Release(stream);
}

static void test_load_save_dib(void)
{
    IPicture *pic;
    PICTDESC desc;
    short type;
    OLE_HANDLE handle;
    HGLOBAL hmem;
    DWORD *mem;
    IPersistStream *src_stream;
    IStream *dst_stream;
    LARGE_INTEGER offset;
    HRESULT hr;
    LONG size;
    ULARGE_INTEGER maxsize;
    unsigned int bpp;

    for (bpp = 4; bpp <= 32; bpp <<= 1) {
        char buffer[sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * 256];
        BITMAPINFO *info = (BITMAPINFO *)buffer;
        RGBQUAD *colors = info->bmiColors;
        DWORD expected_size, expected_bpp;
        void *bits;

        winetest_push_context("bpp %u", bpp);
        expected_size = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)
            + (bpp <= 8 ? sizeof(RGBQUAD) * (1u << bpp) : 0)
            + sizeof(DWORD); /* pixels */;
        expected_bpp = bpp <= 8 ? bpp : 24;

        memset(info, 0, sizeof(*info));
        info->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        info->bmiHeader.biWidth = 1;
        info->bmiHeader.biHeight = 1;
        info->bmiHeader.biPlanes = 1;
        info->bmiHeader.biBitCount = bpp;
        info->bmiHeader.biCompression = BI_RGB;
        memset(colors, 0xaa, sizeof(RGBQUAD) * 256);

        desc.cbSizeofstruct = sizeof(desc);
        desc.picType = PICTYPE_BITMAP;
        desc.bmp.hpal = 0;
        desc.bmp.hbitmap = CreateDIBSection(NULL, info, DIB_RGB_COLORS, &bits, NULL, 0);
        hr = OleCreatePictureIndirect(&desc, &IID_IPicture, TRUE, (void**)&pic);

        hr = IPicture_get_Type(pic, &type);
        ok(hr == S_OK,"get_Type error %#8lx\n", hr);
        ok(type == PICTYPE_BITMAP,"expected picture type PICTYPE_BITMAP, got %d\n", type);

        hr = IPicture_get_Handle(pic, &handle);
        ok(hr == S_OK,"get_Handle error %#8lx\n", hr);
        ok(IntToPtr(handle) == desc.bmp.hbitmap, "get_Handle returned wrong handle %#x\n", handle);

        hmem = GlobalAlloc(GMEM_ZEROINIT, 4096);
        hr = CreateStreamOnHGlobal(hmem, FALSE, &dst_stream);
        ok(hr == S_OK, "createstreamonhglobal error %#lx\n", hr);

        size = -1;
        hr = IPicture_SaveAsFile(pic, dst_stream, TRUE, &size);
        ok(hr == S_OK, "IPicture_SaveasFile error %#lx\n", hr);
        ok(size == expected_size, "expected %ld, got %ld\n", expected_size, size);
        if (size == expected_size) {
            mem = GlobalLock(hmem);
            ok(!memcmp(&mem[0], "BM", 2), "got wrong bmp header %04lx\n", mem[0]);
            info = (BITMAPINFO *)(((BITMAPFILEHEADER *)&mem[0]) + 1);
            ok(info->bmiHeader.biBitCount == expected_bpp, "expected bpp %lu, got %hu\n", expected_bpp, info->bmiHeader.biBitCount);
            ok(info->bmiHeader.biCompression == BI_RGB, "expected BI_RGB, got %lu\n", info->bmiHeader.biCompression);
            GlobalUnlock(hmem);
        }

        size = -1;
        hr = IPicture_SaveAsFile(pic, dst_stream, FALSE, &size);
        ok(hr == E_FAIL, "expected E_FAIL, got %#lx\n", hr);
        ok(size == -1, "expected -1, got %ld\n", size);

        offset.QuadPart = 0;
        hr = IStream_Seek(dst_stream, offset, STREAM_SEEK_SET, NULL);
        ok(hr == S_OK, "IStream_Seek %#lx\n", hr);

        hr = IPicture_QueryInterface(pic, &IID_IPersistStream, (void **)&src_stream);
        ok(hr == S_OK, "QueryInterface error %#lx\n", hr);

        maxsize.QuadPart = 0;
        hr = IPersistStream_GetSizeMax(src_stream, &maxsize);
        ok(hr == S_OK, "GetSizeMax error %#lx\n", hr);
        ok(maxsize.QuadPart == expected_size + 8, "expected %lx, got %s\n", expected_size + 8, wine_dbgstr_longlong(maxsize.QuadPart));

        hr = IPersistStream_Save(src_stream, dst_stream, TRUE);
        ok(hr == S_OK, "Save error %#lx\n", hr);

        maxsize.QuadPart = 0;
        hr = IPersistStream_GetSizeMax(src_stream, &maxsize);
        ok(hr == S_OK, "GetSizeMax error %#lx\n", hr);
        ok(maxsize.QuadPart == expected_size + 8, "expected %lx, got %s\n", expected_size + 8, wine_dbgstr_longlong(maxsize.QuadPart));

        IPersistStream_Release(src_stream);
        IStream_Release(dst_stream);

        mem = GlobalLock(hmem);
        ok(!memcmp(mem, "lt\0\0", 4), "got wrong stream header %04lx\n", mem[0]);
        ok(mem[1] == expected_size, "expected stream size %lu, got %lu\n", expected_size, mem[1]);
        ok(!memcmp(&mem[2], "BM", 2), "got wrong bmp header %04lx\n", mem[2]);
        info = (BITMAPINFO *)(((BITMAPFILEHEADER *)&mem[2]) + 1);
        ok(info->bmiHeader.biBitCount == expected_bpp, "expected bpp %lu, got %hu\n", expected_bpp, info->bmiHeader.biBitCount);
        ok(info->bmiHeader.biCompression == BI_RGB, "expected BI_RGB, got %lu\n", info->bmiHeader.biCompression);

        GlobalUnlock(hmem);
        GlobalFree(hmem);

        DeleteObject(desc.bmp.hbitmap);
        IPicture_Release(pic);
        winetest_pop_context();
    }
}

static void test_load_save_emf(void)
{
    HDC hdc;
    IPicture *pic;
    PICTDESC desc;
    short type;
    OLE_HANDLE handle;
    HGLOBAL hmem;
    DWORD *mem;
    ENHMETAHEADER *emh;
    IPersistStream *src_stream;
    IStream *dst_stream;
    LARGE_INTEGER offset;
    HRESULT hr;
    LONG size;

    hdc = CreateEnhMetaFileA(0, NULL, NULL, NULL);
    ok(hdc != 0, "CreateEnhMetaFileA failed\n");

    desc.cbSizeofstruct = sizeof(desc);
    desc.picType = PICTYPE_ENHMETAFILE;
    desc.emf.hemf = CloseEnhMetaFile(hdc);
    ok(desc.emf.hemf != 0, "CloseEnhMetaFile failed\n");
    hr = OleCreatePictureIndirect(&desc, &IID_IPicture, FALSE, (void**)&pic);
    ok(hr == S_OK, "OleCreatePictureIndirect error %#lx\n", hr);

    type = -1;
    hr = IPicture_get_Type(pic, &type);
    ok(hr == S_OK, "get_Type error %#lx\n", hr);
    ok(type == PICTYPE_ENHMETAFILE,"expected PICTYPE_ENHMETAFILE, got %d\n", type);

    hr = IPicture_get_Handle(pic, &handle);
    ok(hr == S_OK,"get_Handle error %#lx\n", hr);
    ok(IntToPtr(handle) == desc.emf.hemf, "get_Handle returned wrong handle %#x\n", handle);

    hmem = GlobalAlloc(GMEM_MOVEABLE, 0);
    hr = CreateStreamOnHGlobal(hmem, FALSE, &dst_stream);
    ok(hr == S_OK, "createstreamonhglobal error %#lx\n", hr);

    size = -1;
    hr = IPicture_SaveAsFile(pic, dst_stream, TRUE, &size);
    ok(hr == S_OK, "IPicture_SaveasFile error %#lx\n", hr);
    ok(size == 128, "expected 128, got %ld\n", size);
    emh = GlobalLock(hmem);
    ok(emh->iType == EMR_HEADER, "wrong iType %04lx\n", emh->iType);
    ok(emh->dSignature == ENHMETA_SIGNATURE, "wrong dSignature %08lx\n", emh->dSignature);
    GlobalUnlock(hmem);

    size = -1;
    hr = IPicture_SaveAsFile(pic, dst_stream, FALSE, &size);
    ok(hr == E_FAIL, "expected E_FAIL, got %#lx\n", hr);
    ok(size == -1, "expected -1, got %ld\n", size);

    offset.QuadPart = 0;
    hr = IStream_Seek(dst_stream, offset, SEEK_SET, NULL);
    ok(hr == S_OK, "IStream_Seek %#lx\n", hr);

    hr = IPicture_QueryInterface(pic, &IID_IPersistStream, (void **)&src_stream);
    ok(hr == S_OK, "QueryInterface error %#lx\n", hr);

    hr = IPersistStream_Save(src_stream, dst_stream, TRUE);
    ok(hr == S_OK, "Save error %#lx\n", hr);

    IPersistStream_Release(src_stream);
    IStream_Release(dst_stream);

    mem = GlobalLock(hmem);
    ok(!memcmp(mem, "lt\0\0", 4), "got wrong stream header %04lx\n", mem[0]);
    ok(mem[1] == 128, "expected 128, got %lu\n", mem[1]);
    emh = (ENHMETAHEADER *)(mem + 2);
    ok(emh->iType == EMR_HEADER, "wrong iType %04lx\n", emh->iType);
    ok(emh->dSignature == ENHMETA_SIGNATURE, "wrong dSignature %08lx\n", emh->dSignature);

    GlobalUnlock(hmem);
    GlobalFree(hmem);

    DeleteEnhMetaFile(desc.emf.hemf);
    IPicture_Release(pic);
}

START_TEST(olepicture)
{
    hOleaut32 = GetModuleHandleA("oleaut32.dll");
    pOleLoadPicture = (void*)GetProcAddress(hOleaut32, "OleLoadPicture");
    pOleLoadPictureEx = (void*)GetProcAddress(hOleaut32, "OleLoadPictureEx");
    if (!pOleLoadPicture)
    {
        win_skip("OleLoadPicture is not available\n");
        return;
    }

    /* Test regular 1x1 pixel images of gif, jpg, bmp type */
    test_pic(gifimage, sizeof(gifimage), 1, TRUE);
    test_pic(jpgimage, sizeof(jpgimage), 24, FALSE);
    test_pic(bmpimage, sizeof(bmpimage), 1, FALSE);
    test_pic(bmpimage_rle8, sizeof(bmpimage_rle8), 4, TRUE);
    test_pic(bmpimage4, sizeof(bmpimage4), 4, FALSE);
    test_pic(bmpimage8, sizeof(bmpimage8), 8, FALSE);
    test_pic(bmpimage24, sizeof(bmpimage24), 24, FALSE);
    test_pic(gif4pixel, sizeof(gif4pixel), 4, TRUE);
    /* FIXME: No PNG support in Windows... */
    if (0) test_pic(pngimage, sizeof(pngimage), 32, TRUE);
    test_empty_image();
    test_empty_image_2();
    if (pOleLoadPictureEx)
    {
        test_apm();
        test_metafile();
        test_enhmetafile();
    }
    else
        win_skip("OleLoadPictureEx is not available\n");
    test_Invoke();
    test_OleCreatePictureIndirect();
    test_Render();
    test_get_Attributes();
    test_get_Handle();
    test_get_Type();
    test_OleLoadPicturePath();
    test_himetric();
    test_load_save_bmp();
    test_load_save_dib();
    test_load_save_icon();
    test_load_save_empty_picture();
    test_load_save_emf();
}


/* Helper functions only ... */


static inline NoStatStreamImpl *impl_from_IStream(IStream *iface)
{
  return CONTAINING_RECORD(iface, NoStatStreamImpl, IStream_iface);
}

static void NoStatStreamImpl_Destroy(NoStatStreamImpl* This)
{
  GlobalFree(This->supportHandle);
  This->supportHandle=0;
  HeapFree(GetProcessHeap(), 0, This);
}

static ULONG WINAPI NoStatStreamImpl_AddRef(
		IStream* iface)
{
  NoStatStreamImpl* const This = impl_from_IStream(iface);
  return InterlockedIncrement(&This->ref);
}

static HRESULT WINAPI NoStatStreamImpl_QueryInterface(
		  IStream*     iface,
		  REFIID         riid,	      /* [in] */
		  void**         ppvObject)   /* [iid_is][out] */
{
  NoStatStreamImpl* const This = impl_from_IStream(iface);
  if (ppvObject==0) return E_INVALIDARG;
  *ppvObject = 0;

  if (IsEqualIID(&IID_IUnknown, riid) || IsEqualIID(&IID_IStream, riid))
    *ppvObject = &This->IStream_iface;

  if ((*ppvObject)==0)
    return E_NOINTERFACE;
  NoStatStreamImpl_AddRef(iface);
  return S_OK;
}

static ULONG WINAPI NoStatStreamImpl_Release(
		IStream* iface)
{
  NoStatStreamImpl* const This = impl_from_IStream(iface);
  ULONG newRef = InterlockedDecrement(&This->ref);
  if (newRef==0)
    NoStatStreamImpl_Destroy(This);
  return newRef;
}

static HRESULT WINAPI NoStatStreamImpl_Read(
		  IStream*     iface,
		  void*          pv,        /* [length_is][size_is][out] */
		  ULONG          cb,        /* [in] */
		  ULONG*         pcbRead)   /* [out] */
{
  NoStatStreamImpl* const This = impl_from_IStream(iface);
  void* supportBuffer;
  ULONG bytesReadBuffer;
  ULONG bytesToReadFromBuffer;

  if (pcbRead==0)
    pcbRead = &bytesReadBuffer;
  bytesToReadFromBuffer = min( This->streamSize.LowPart - This->currentPosition.LowPart, cb);
  supportBuffer = GlobalLock(This->supportHandle);
  memcpy(pv, (char *) supportBuffer+This->currentPosition.LowPart, bytesToReadFromBuffer);
  This->currentPosition.LowPart+=bytesToReadFromBuffer;
  *pcbRead = bytesToReadFromBuffer;
  GlobalUnlock(This->supportHandle);
  if(*pcbRead == cb)
    return S_OK;
  return S_FALSE;
}

static HRESULT WINAPI NoStatStreamImpl_Write(
	          IStream*     iface,
		  const void*    pv,          /* [size_is][in] */
		  ULONG          cb,          /* [in] */
		  ULONG*         pcbWritten)  /* [out] */
{
  NoStatStreamImpl* const This = impl_from_IStream(iface);
  void*          supportBuffer;
  ULARGE_INTEGER newSize;
  ULONG          bytesWritten = 0;

  if (pcbWritten == 0)
    pcbWritten = &bytesWritten;
  if (cb == 0)
    return S_OK;
  newSize.HighPart = 0;
  newSize.LowPart = This->currentPosition.LowPart + cb;
  if (newSize.LowPart > This->streamSize.LowPart)
   IStream_SetSize(iface, newSize);

  supportBuffer = GlobalLock(This->supportHandle);
  memcpy((char *) supportBuffer+This->currentPosition.LowPart, pv, cb);
  This->currentPosition.LowPart+=cb;
  *pcbWritten = cb;
  GlobalUnlock(This->supportHandle);
  return S_OK;
}

static HRESULT WINAPI NoStatStreamImpl_Seek(
		  IStream*      iface,
		  LARGE_INTEGER   dlibMove,         /* [in] */
		  DWORD           dwOrigin,         /* [in] */
		  ULARGE_INTEGER* plibNewPosition) /* [out] */
{
  NoStatStreamImpl* const This = impl_from_IStream(iface);
  ULARGE_INTEGER newPosition;
  switch (dwOrigin)
  {
    case STREAM_SEEK_SET:
      newPosition.HighPart = 0;
      newPosition.LowPart = 0;
      break;
    case STREAM_SEEK_CUR:
      newPosition = This->currentPosition;
      break;
    case STREAM_SEEK_END:
      newPosition = This->streamSize;
      break;
    default:
      return STG_E_INVALIDFUNCTION;
  }
  if (dlibMove.QuadPart < 0 && newPosition.QuadPart < -dlibMove.QuadPart)
      return STG_E_INVALIDFUNCTION;
  newPosition.QuadPart += dlibMove.QuadPart;
  if (plibNewPosition) *plibNewPosition = newPosition;
  This->currentPosition = newPosition;
  return S_OK;
}

static HRESULT WINAPI NoStatStreamImpl_SetSize(
				     IStream*      iface,
				     ULARGE_INTEGER  libNewSize)   /* [in] */
{
  NoStatStreamImpl* const This = impl_from_IStream(iface);
  HGLOBAL supportHandle;
  if (libNewSize.HighPart != 0)
    return STG_E_INVALIDFUNCTION;
  if (This->streamSize.LowPart == libNewSize.LowPart)
    return S_OK;
  supportHandle = GlobalReAlloc(This->supportHandle, libNewSize.LowPart, 0);
  if (supportHandle == 0)
    return STG_E_MEDIUMFULL;
  This->supportHandle = supportHandle;
  This->streamSize.LowPart = libNewSize.LowPart;
  return S_OK;
}

static HRESULT WINAPI NoStatStreamImpl_CopyTo(
				    IStream*      iface,
				    IStream*      pstm,         /* [unique][in] */
				    ULARGE_INTEGER  cb,           /* [in] */
				    ULARGE_INTEGER* pcbRead,      /* [out] */
				    ULARGE_INTEGER* pcbWritten)   /* [out] */
{
  HRESULT        hr = S_OK;
  BYTE           tmpBuffer[128];
  ULONG          bytesRead, bytesWritten, copySize;
  ULARGE_INTEGER totalBytesRead;
  ULARGE_INTEGER totalBytesWritten;

  if ( pstm == 0 )
    return STG_E_INVALIDPOINTER;
  totalBytesRead.LowPart = totalBytesRead.HighPart = 0;
  totalBytesWritten.LowPart = totalBytesWritten.HighPart = 0;

  while ( cb.LowPart > 0 )
  {
    if ( cb.LowPart >= 128 )
      copySize = 128;
    else
      copySize = cb.LowPart;
    IStream_Read(iface, tmpBuffer, copySize, &bytesRead);
    totalBytesRead.LowPart += bytesRead;
    IStream_Write(pstm, tmpBuffer, bytesRead, &bytesWritten);
    totalBytesWritten.LowPart += bytesWritten;
    if (bytesRead != bytesWritten)
    {
      hr = STG_E_MEDIUMFULL;
      break;
    }
    if (bytesRead!=copySize)
      cb.LowPart = 0;
    else
      cb.LowPart -= bytesRead;
  }
  if (pcbRead)
  {
    pcbRead->u.LowPart = totalBytesRead.LowPart;
    pcbRead->u.HighPart = totalBytesRead.HighPart;
  }

  if (pcbWritten)
  {
    pcbWritten->u.LowPart = totalBytesWritten.LowPart;
    pcbWritten->u.HighPart = totalBytesWritten.HighPart;
  }
  return hr;
}

static HRESULT WINAPI NoStatStreamImpl_Commit(IStream* iface,DWORD grfCommitFlags)
{
  return S_OK;
}
static HRESULT WINAPI NoStatStreamImpl_Revert(IStream* iface) { return S_OK; }

static HRESULT WINAPI NoStatStreamImpl_LockRegion(
		  IStream*       iface,
		  ULARGE_INTEGER libOffset,   /* [in] */
		  ULARGE_INTEGER cb,          /* [in] */
		  DWORD          dwLockType)  /* [in] */
{
  return S_OK;
}

static HRESULT WINAPI NoStatStreamImpl_UnlockRegion(
		  IStream*       iface,
		  ULARGE_INTEGER libOffset,   /* [in] */
		  ULARGE_INTEGER cb,          /* [in] */
		  DWORD          dwLockType)  /* [in] */
{
  return S_OK;
}

static HRESULT WINAPI NoStatStreamImpl_Stat(
		  IStream*     iface,
		  STATSTG*     pstatstg,     /* [out] */
		  DWORD        grfStatFlag)  /* [in] */
{
  return E_NOTIMPL;
}

static HRESULT WINAPI NoStatStreamImpl_Clone(
		  IStream*     iface,
		  IStream**    ppstm) /* [out] */
{
  return E_NOTIMPL;
}
static const IStreamVtbl NoStatStreamImpl_Vtbl;

/*
    Build an object that implements IStream, without IStream_Stat capabilities.
    Receives a memory handle with data buffer. If memory handle is non-null,
    it is assumed to be unlocked, otherwise an internal memory handle is allocated.
    In any case the object takes ownership of memory handle and will free it on
    object release.
 */
static IStream* NoStatStream_Construct(HGLOBAL hGlobal)
{
  NoStatStreamImpl* newStream;

  newStream = HeapAlloc(GetProcessHeap(), 0, sizeof(NoStatStreamImpl));
  if (newStream!=0)
  {
    newStream->IStream_iface.lpVtbl = &NoStatStreamImpl_Vtbl;
    newStream->ref    = 1;
    newStream->supportHandle = hGlobal;

    if (!newStream->supportHandle)
      newStream->supportHandle = GlobalAlloc(GMEM_MOVEABLE | GMEM_NODISCARD |
					     GMEM_SHARE, 0);
    newStream->currentPosition.HighPart = 0;
    newStream->currentPosition.LowPart = 0;
    newStream->streamSize.HighPart = 0;
    newStream->streamSize.LowPart  = GlobalSize(newStream->supportHandle);
  }
  return &newStream->IStream_iface;
}


static const IStreamVtbl NoStatStreamImpl_Vtbl =
{
    NoStatStreamImpl_QueryInterface,
    NoStatStreamImpl_AddRef,
    NoStatStreamImpl_Release,
    NoStatStreamImpl_Read,
    NoStatStreamImpl_Write,
    NoStatStreamImpl_Seek,
    NoStatStreamImpl_SetSize,
    NoStatStreamImpl_CopyTo,
    NoStatStreamImpl_Commit,
    NoStatStreamImpl_Revert,
    NoStatStreamImpl_LockRegion,
    NoStatStreamImpl_UnlockRegion,
    NoStatStreamImpl_Stat,
    NoStatStreamImpl_Clone
};
