/*
 * Unit tests for advpack.dll
 *
 * Copyright (C) 2005 Robert Reif
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

#include <windows.h>
#include <advpub.h>

#include "wine/test.h"


static HRESULT (WINAPI *pGetVersionFromFile)(LPSTR,LPDWORD,LPDWORD,BOOL);


static void version_test()
{
    HRESULT hr;
    DWORD major, minor;

    major = minor = 0;
    hr = pGetVersionFromFile("kernel32.dll", &major, &minor, FALSE);
    ok (hr == S_OK, "GetVersionFromFileEx(kernel32.dll) failed, returned "
        "0x%08lx\n", hr);

    trace("kernel32.dll Language ID: 0x%08lx, Codepage ID: 0x%08lx\n",
           major, minor);

    major = minor = 0;
    hr = pGetVersionFromFile("kernel32.dll", &major, &minor, TRUE);
    ok (hr == S_OK, "GetVersionFromFileEx(kernel32.dll) failed, returned "
        "0x%08lx\n", hr);

    trace("kernel32.dll version: %d.%d.%d.%d\n", HIWORD(major), LOWORD(major),
          HIWORD(minor), LOWORD(minor));
}

START_TEST(advpack)
{
    HMODULE hdll;

    hdll = LoadLibraryA("advpack.dll");
    if (!hdll)
        return;
    pGetVersionFromFile = (void*)GetProcAddress(hdll, "GetVersionFromFile");
    if (!pGetVersionFromFile)
        return;

    version_test();
}
