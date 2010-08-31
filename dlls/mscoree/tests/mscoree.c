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

#include "corerror.h"
#include "mscoree.h"
#include "wine/test.h"

static HMODULE hmscoree;

static HRESULT (WINAPI *pGetCORVersion)(LPWSTR, DWORD, DWORD*);
static HRESULT (WINAPI *pGetCORSystemDirectory)(LPWSTR, DWORD, DWORD*);
static HRESULT (WINAPI *pGetRequestedRuntimeInfo)(LPCWSTR, LPCWSTR, LPCWSTR, DWORD, DWORD, LPWSTR, DWORD, DWORD*, LPWSTR, DWORD, DWORD*);

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
    pGetRequestedRuntimeInfo = (void *)GetProcAddress(hmscoree, "GetRequestedRuntimeInfo");

    if (!pGetCORVersion || !pGetCORSystemDirectory || !pGetRequestedRuntimeInfo)
    {
        win_skip("functions not available\n");
        FreeLibrary(hmscoree);
        return FALSE;
    }

    return TRUE;
}

static void test_versioninfo(void)
{
    const WCHAR v2_0[] = {'v','2','.','0','.','5','0','7','2','7',0};
    const WCHAR v1_1[] = {'v','1','.','1','.','4','3','2','2',0};

    WCHAR version[MAX_PATH];
    WCHAR path[MAX_PATH];
    DWORD size, path_len;
    HRESULT hr;

    if (0)  /* crashes on <= w2k3 */
    {
        hr = pGetCORVersion(NULL, MAX_PATH, &size);
        ok(hr == E_POINTER,"GetCORVersion returned %08x\n", hr);
    }

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

    if (0)  /* crashes on <= w2k3 */
    {
        hr = pGetCORSystemDirectory(NULL, MAX_PATH , &size);
        ok(hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER), "GetCORSystemDirectory returned %08x\n", hr);
    }

    hr = pGetCORSystemDirectory(path, MAX_PATH , NULL);
    ok(hr == E_POINTER,"GetCORSystemDirectory returned %08x\n", hr);

    trace("latest installed .net installed in directory: %s\n", wine_dbgstr_w(path));

    /* test GetRequestedRuntimeInfo, first get info about different versions of runtime */
    hr = pGetRequestedRuntimeInfo( NULL, v2_0, NULL, 0, 0, path, MAX_PATH, &path_len, version, MAX_PATH, &size);

    if(hr == CLR_E_SHIM_RUNTIME) return; /* skipping rest of tests on win2k as .net 2.0 not installed */

    ok(hr == S_OK, "GetRequestedRuntimeInfo returned %08x\n", hr);
    trace(" installed in directory %s is .net version %s\n", wine_dbgstr_w(path), wine_dbgstr_w(version));

    hr = pGetRequestedRuntimeInfo( NULL, v1_1, NULL, 0, 0, path, MAX_PATH, &path_len, version, MAX_PATH, &size);
    ok(hr == S_OK || CLR_E_SHIM_RUNTIME /*v1_1 not installed*/, "GetRequestedRuntimeInfo returned %08x\n", hr);
    if(hr == S_OK)
        trace(" installed in directory %s is .net version %s\n", wine_dbgstr_w(path), wine_dbgstr_w(version));
    /* version number NULL not allowed without RUNTIME_INFO_UPGRADE_VERSION flag */
    hr = pGetRequestedRuntimeInfo( NULL, NULL, NULL, 0, 0, path, MAX_PATH, &path_len, version, MAX_PATH, &size);
    ok(hr == CLR_E_SHIM_RUNTIME, "GetRequestedRuntimeInfo returned %08x\n", hr);
    /* with RUNTIME_INFO_UPGRADE_VERSION flag and version number NULL, latest installed version is returned */
    hr = pGetRequestedRuntimeInfo( NULL, NULL, NULL, 0, RUNTIME_INFO_UPGRADE_VERSION, path, MAX_PATH, &path_len, version, MAX_PATH, &size);
    ok(hr == S_OK, "GetRequestedRuntimeInfo returned %08x\n", hr);

    hr = pGetRequestedRuntimeInfo( NULL, v2_0, NULL, 0, 0, path, 1, &path_len, version, MAX_PATH, &size);
    ok(hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER), "GetRequestedRuntimeInfo returned %08x\n", hr);

    /* if one of the buffers is NULL, the other one is still happily filled */
    memset(version, 0, sizeof(version));
    hr = pGetRequestedRuntimeInfo( NULL, v2_0, NULL, 0, 0, NULL, MAX_PATH, &path_len, version, MAX_PATH, &size);
    ok(hr == S_OK, "GetRequestedRuntimeInfo returned %08x\n", hr);
    ok(!lstrcmpW(version, v2_0), "version is %s , expected %s\n", wine_dbgstr_w(version), wine_dbgstr_w(v2_0));
    /* With NULL-pointer for bufferlength, the buffer itsself still gets filled with correct string */
    memset(version, 0, sizeof(version));
    hr = pGetRequestedRuntimeInfo( NULL, v2_0, NULL, 0, 0, path, MAX_PATH, &path_len, version, MAX_PATH, NULL);
    ok(hr == S_OK, "GetRequestedRuntimeInfo returned %08x\n", hr);
    ok(!lstrcmpW(version, v2_0), "version is %s , expected %s\n", wine_dbgstr_w(version), wine_dbgstr_w(v2_0));
}

START_TEST(mscoree)
{
    if (!init_functionpointers())
        return;

    test_versioninfo();

    FreeLibrary(hmscoree);
}
