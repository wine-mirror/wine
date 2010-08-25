/*
 * Copyright 2010 Louis Lenders
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

static HMODULE hmscoree;

static HRESULT (WINAPI *pGetCORVersion)(LPWSTR, DWORD, DWORD*);
static HRESULT (WINAPI *pGetCORSystemDirectory)(LPWSTR, DWORD, DWORD*);

static BOOL init_functionpointers(void)
{
    hmscoree = LoadLibraryA("mscoree.dll");

    if (!hmscoree)
    {
        win_skip("mscoree.dll not available\n");
        return FALSE;
    }

    pGetCORVersion = (void *)GetProcAddress(hmscoree, "GetCORVersion");
    pGetCORSystemDirectory = (void *)GetProcAddress(hmscoree, "GetCORSystemDirectory");
    if (!pGetCORVersion || !pGetCORSystemDirectory)
    {
        win_skip("functions not available\n");
        FreeLibrary(hmscoree);
        return FALSE;
    }

    return TRUE;
}

static void test_versioninfo(void)
{
    WCHAR version[MAX_PATH];
    WCHAR path[MAX_PATH];
    DWORD size, path_len;
    HRESULT hr;

    hr =  pGetCORVersion(NULL, MAX_PATH, &size);
    ok(hr == E_POINTER,"GetCORVersion returned %08x\n", hr);

    hr =  pGetCORVersion(version, 1, &size);
    ok(hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER),"GetCORVersion returned %08x\n", hr);

    hr =  pGetCORVersion(version, MAX_PATH, &size);
    ok(hr == S_OK,"GetCORVersion returned %08x\n", hr);

    trace("latest installed .net runtime: %s\n", wine_dbgstr_w(version));

    hr = pGetCORSystemDirectory(path, MAX_PATH , &size);
    ok(hr == S_OK, "GetCORSystemDirectory returned %08x\n", hr);
    /* size includes terminating null-character */
    ok(size == (lstrlenW(path) + 1),"size is %d instead of %d\n", size, (lstrlenW(path) + 1));

    path_len = size;

    hr = pGetCORSystemDirectory(path, path_len-1 , &size);
    ok(hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER), "GetCORSystemDirectory returned %08x\n", hr);

    hr = pGetCORSystemDirectory(NULL, MAX_PATH , &size);
    ok(hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER), "GetCORSystemDirectory returned %08x\n", hr);

    hr = pGetCORSystemDirectory(path, MAX_PATH , NULL);
    ok(hr == E_POINTER,"GetCORSystemDirectory returned %08x\n", hr);

    trace("latest installed .net installed in directory: %s\n", wine_dbgstr_w(path));
}

START_TEST(mscoree)
{
    if (!init_functionpointers())
        return;

    test_versioninfo();

    FreeLibrary(hmscoree);
}
