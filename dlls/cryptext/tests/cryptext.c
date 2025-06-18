/*
 * Copyright 2019 Dmitry Timoshkov
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
#include <assert.h>
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <winerror.h>

#include "wine/test.h"

static HRESULT (WINAPI *pCryptExtOpenCER)(HWND,HINSTANCE,LPCSTR,DWORD);

static void test_CryptExtOpenCER(void)
{
    HRESULT hr;

    if (!pCryptExtOpenCER)
    {
        win_skip("CryptExtOpenCER is not available on this platform\n");
        return;
    }

    if (!winetest_interactive)
    {
        skip("CryptExtOpenCER test needs user interaction\n");
        return;
    }

    SetLastError(0xdeadbeef);
    hr = pCryptExtOpenCER(0, 0, "dead.beef", SW_HIDE);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = pCryptExtOpenCER(0, 0, "VeriSign Class 3 Public Primary Certification Authority - G4.txt", SW_SHOW);
    ok(hr == S_OK, "got %#lx\n", hr);
}

START_TEST(cryptext)
{
    HMODULE hmod = LoadLibraryA("cryptext.dll");

    pCryptExtOpenCER = (void *)GetProcAddress(hmod, "CryptExtOpenCER");

    test_CryptExtOpenCER();
}
