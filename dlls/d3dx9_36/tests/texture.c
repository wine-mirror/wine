/*
 * Tests for the D3DX9 texture functions
 *
 * Copyright 2009 Tony Wasserka
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

#include "wine/test.h"
#include "d3dx9tex.h"
#include "resources.h"

/* 1x1 bmp (1 bpp) */
static const unsigned char bmp01[66] = {
0x42,0x4d,0x42,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x3e,0x00,0x00,0x00,0x28,0x00,
0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x00,0x00,
0x00,0x00,0x04,0x00,0x00,0x00,0x12,0x0b,0x00,0x00,0x12,0x0b,0x00,0x00,0x02,0x00,
0x00,0x00,0x02,0x00,0x00,0x00,0xff,0xff,0xff,0x00,0xff,0xff,0xff,0x00,0x00,0x00,
0x00,0x00
};

/* invalid image file */
static const unsigned char noimage[4] = {
0x11,0x22,0x33,0x44
};

static HRESULT create_file(const char *filename, const unsigned char *data, const unsigned int size)
{
    HANDLE hfile;

    hfile = CreateFileA(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    if(hfile == INVALID_HANDLE_VALUE) return HRESULT_FROM_WIN32(GetLastError());

    WriteFile(hfile, data, size, NULL, NULL);
    CloseHandle(hfile);

    return D3D_OK;
}

static void test_D3DXGetImageInfo()
{
    HRESULT hr;
    D3DXIMAGE_INFO info;
    BOOL testdummy_ok, testbitmap_ok;

    hr = create_file("testdummy.bmp", noimage, sizeof(noimage));  /* invalid image */
    testdummy_ok = SUCCEEDED(hr);

    hr = create_file("testbitmap.bmp", bmp01, sizeof(bmp01));  /* valid image */
    testbitmap_ok = SUCCEEDED(hr);

    /* D3DXGetImageInfoFromFile */
    if(testbitmap_ok) {
        todo_wine {
            hr = D3DXGetImageInfoFromFileA("testbitmap.bmp", &info);
            ok(hr == D3D_OK, "D3DXGetImageInfoFromFile returned %#x, expected %#x\n", hr, D3D_OK);
        }

        hr = D3DXGetImageInfoFromFileA("testbitmap.bmp", NULL); /* valid image, second parameter is NULL */
        ok(hr == D3D_OK, "D3DXGetImageInfoFromFile returned %#x, expected %#x\n", hr, D3D_OK);
    } else skip("Couldn't create \"testbitmap.bmp\"\n");

    if(testdummy_ok) {
        hr = D3DXGetImageInfoFromFileA("testdummy.bmp", NULL); /* invalid image, second parameter is NULL */
        ok(hr == D3D_OK, "D3DXGetImageInfoFromFile returned %#x, expected %#x\n", hr, D3D_OK);

        todo_wine {
            hr = D3DXGetImageInfoFromFileA("testdummy.bmp", &info);
            ok(hr == D3DXERR_INVALIDDATA, "D3DXGetImageInfoFromFile returned %#x, expected %#x\n", hr, D3DXERR_INVALIDDATA);
        }
    } else skip("Couldn't create \"testdummy.bmp\"\n");

    hr = D3DXGetImageInfoFromFileA("filedoesnotexist.bmp", &info);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXGetImageInfoFromFile returned %#x, expected %#x\n", hr, D3DXERR_INVALIDDATA);

    hr = D3DXGetImageInfoFromFileA("filedoesnotexist.bmp", NULL);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXGetImageInfoFromFile returned %#x, expected %#x\n", hr, D3DXERR_INVALIDDATA);

    hr = D3DXGetImageInfoFromFileA("", &info);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXGetImageInfoFromFile returned %#x, expected %#x\n", hr, D3DXERR_INVALIDDATA);

    hr = D3DXGetImageInfoFromFileA(NULL, &info);
    ok(hr == D3DERR_INVALIDCALL, "D3DXGetImageInfoFromFile returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXGetImageInfoFromFileA(NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXGetImageInfoFromFile returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);


    /* D3DXGetImageInfoFromResource */
    todo_wine {
        hr = D3DXGetImageInfoFromResourceA(NULL, MAKEINTRESOURCEA(IDB_BITMAP_1x1), &info); /* RT_BITMAP */
        ok(hr == D3D_OK, "D3DXGetImageInfoFromResource returned %#x, expected %#x\n", hr, D3D_OK);

        hr = D3DXGetImageInfoFromResourceA(NULL, MAKEINTRESOURCEA(IDB_BITMAP_1x1), NULL);
        ok(hr == D3D_OK, "D3DXGetImageInfoFromResource returned %#x, expected %#x\n", hr, D3D_OK);

        hr = D3DXGetImageInfoFromResourceA(NULL, MAKEINTRESOURCEA(IDD_BITMAPDATA_1x1), &info); /* RT_RCDATA */
        ok(hr == D3D_OK, "D3DXGetImageInfoFromResource returned %#x, expected %#x\n", hr, D3D_OK);
    }

    hr = D3DXGetImageInfoFromResourceA(NULL, MAKEINTRESOURCEA(IDS_STRING), &info);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXGetImageInfoFromResource returned %#x, expected %#x\n", hr, D3DXERR_INVALIDDATA);

    hr = D3DXGetImageInfoFromResourceA(NULL, MAKEINTRESOURCEA(IDS_STRING), NULL);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXGetImageInfoFromResource returned %#x, expected %#x\n", hr, D3DXERR_INVALIDDATA);

    hr = D3DXGetImageInfoFromResourceA(NULL, "resourcedoesnotexist", &info);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXGetImageInfoFromResource returned %#x, expected %#x\n", hr, D3DXERR_INVALIDDATA);

    hr = D3DXGetImageInfoFromResourceA(NULL, "resourcedoesnotexist", NULL);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXGetImageInfoFromResource returned %#x, expected %#x\n", hr, D3DXERR_INVALIDDATA);

    hr = D3DXGetImageInfoFromResourceA(NULL, NULL, NULL);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXGetImageInfoFromResource returned %#x, expected %#x\n", hr, D3DXERR_INVALIDDATA);


    /* D3DXGetImageInfoFromFileInMemory */
    todo_wine {
        hr = D3DXGetImageInfoFromFileInMemory(bmp01, sizeof(bmp01), &info);
        ok(hr == D3D_OK, "D3DXGetImageInfoFromFileInMemory returned %#x, expected %#x\n", hr, D3D_OK);

        hr = D3DXGetImageInfoFromFileInMemory(bmp01, sizeof(bmp01)+5, &info); /* too large size */
        ok(hr == D3D_OK, "D3DXGetImageInfoFromFileInMemory returned %#x, expected %#x\n", hr, D3D_OK);
    }

    hr = D3DXGetImageInfoFromFileInMemory(bmp01, sizeof(bmp01), NULL);
    ok(hr == D3D_OK, "D3DXGetImageInfoFromFileInMemory returned %#x, expected %#x\n", hr, D3D_OK);

    hr = D3DXGetImageInfoFromFileInMemory(noimage, sizeof(noimage), NULL);
    ok(hr == D3D_OK, "D3DXGetImageInfoFromResource returned %#x, expected %#x\n", hr, D3D_OK);

    todo_wine {
        hr = D3DXGetImageInfoFromFileInMemory(noimage, sizeof(noimage), &info);
        ok(hr == D3DXERR_INVALIDDATA, "D3DXGetImageInfoFromFileInMemory returned %#x, expected %#x\n", hr, D3DXERR_INVALIDDATA);

        hr = D3DXGetImageInfoFromFileInMemory(bmp01, sizeof(bmp01)-1, &info);
        ok(hr == D3DXERR_INVALIDDATA, "D3DXGetImageInfoFromFileInMemory returned %#x, expected %#x\n", hr, D3DXERR_INVALIDDATA);

        hr = D3DXGetImageInfoFromFileInMemory(bmp01+1, sizeof(bmp01)-1, &info);
        ok(hr == D3DXERR_INVALIDDATA, "D3DXGetImageInfoFromFileInMemory returned %#x, expected %#x\n", hr, D3DXERR_INVALIDDATA);
    }

    hr = D3DXGetImageInfoFromFileInMemory(bmp01, 0, &info);
    ok(hr == D3DERR_INVALIDCALL, "D3DXGetImageInfoFromFileInMemory returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXGetImageInfoFromFileInMemory(bmp01, 0, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXGetImageInfoFromFileInMemory returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXGetImageInfoFromFileInMemory(noimage, 0, &info);
    ok(hr == D3DERR_INVALIDCALL, "D3DXGetImageInfoFromFileInMemory returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXGetImageInfoFromFileInMemory(noimage, 0, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXGetImageInfoFromFileInMemory returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXGetImageInfoFromFileInMemory(NULL, 0, &info);
    ok(hr == D3DERR_INVALIDCALL, "D3DXGetImageInfoFromFileInMemory returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXGetImageInfoFromFileInMemory(NULL, 4, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXGetImageInfoFromFileInMemory returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXGetImageInfoFromFileInMemory(NULL, 4, &info);
    ok(hr == D3DERR_INVALIDCALL, "D3DXGetImageInfoFromFileInMemory returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXGetImageInfoFromFileInMemory(NULL, 0, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXGetImageInfoFromFileInMemory returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    /* cleanup */
    if(testdummy_ok) DeleteFileA("testdummy.bmp");
    if(testbitmap_ok) DeleteFileA("testbitmap.bmp");
}

START_TEST(texture)
{
    test_D3DXGetImageInfo();
}
