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
static HRESULT (WINAPI *pLoadLibraryShim)(LPCWSTR, LPCWSTR, LPVOID, HMODULE*);

static WCHAR tolowerW( WCHAR ch )
{
    if (ch >= 'A' && ch <= 'Z') return ch|32;
    else return ch;
}

static WCHAR *strstriW( const WCHAR *str, const WCHAR *sub )
{
    while (*str)
    {
        const WCHAR *p1 = str, *p2 = sub;
        while (*p1 && *p2 && tolowerW(*p1) == tolowerW(*p2)) { p1++; p2++; }
        if (!*p2) return (WCHAR *)str;
        str++;
    }
    return NULL;
}

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
    pLoadLibraryShim = (void *)GetProcAddress(hmscoree, "LoadLibraryShim");

    if (!pGetCORVersion || !pGetCORSystemDirectory || !pGetRequestedRuntimeInfo || !pLoadLibraryShim)
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
    if (hr == CLR_E_SHIM_RUNTIME)
    {
        /* FIXME: Get Mono packaged properly so we can fail here. */
        todo_wine ok(hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER),"GetCORVersion returned %08x\n", hr);
        skip("No .NET runtimes are installed\n");
        return;
    }

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

static void test_loadlibraryshim(void)
{
    const WCHAR v4_0[] = {'v','4','.','0','.','3','0','3','1','9',0};
    const WCHAR v2_0[] = {'v','2','.','0','.','5','0','7','2','7',0};
    const WCHAR v1_1[] = {'v','1','.','1','.','4','3','2','2',0};
    const WCHAR vbogus[] = {'v','b','o','g','u','s',0};
    const WCHAR fusion[] = {'f','u','s','i','o','n',0};
    const WCHAR fusiondll[] = {'f','u','s','i','o','n','.','d','l','l',0};
    const WCHAR nosuchdll[] = {'j','n','v','n','l','.','d','l','l',0};
    const WCHAR gdipdll[] = {'g','d','i','p','l','u','s','.','d','l','l',0};
    const WCHAR gdidll[] = {'g','d','i','3','2','.','d','l','l',0};
    HRESULT hr;
    const WCHAR *latest = NULL;
    HMODULE hdll;
    WCHAR dllpath[MAX_PATH];

    hr = pLoadLibraryShim(fusion, v1_1, NULL, &hdll);
    ok(hr == S_OK || hr == E_HANDLE, "LoadLibraryShim failed, hr=%x\n", hr);
    if (SUCCEEDED(hr))
    {
        latest = v1_1;

        GetModuleFileNameW(hdll, dllpath, MAX_PATH);

        todo_wine ok(strstriW(dllpath, v1_1) != 0, "incorrect fusion.dll path %s\n", wine_dbgstr_w(dllpath));
        ok(strstriW(dllpath, fusiondll) != 0, "incorrect fusion.dll path %s\n", wine_dbgstr_w(dllpath));

        FreeLibrary(hdll);
    }

    hr = pLoadLibraryShim(fusion, v2_0, NULL, &hdll);
    ok(hr == S_OK || hr == E_HANDLE, "LoadLibraryShim failed, hr=%x\n", hr);
    if (SUCCEEDED(hr))
    {
        latest = v2_0;

        GetModuleFileNameW(hdll, dllpath, MAX_PATH);

        todo_wine ok(strstriW(dllpath, v2_0) != 0, "incorrect fusion.dll path %s\n", wine_dbgstr_w(dllpath));
        ok(strstriW(dllpath, fusiondll) != 0, "incorrect fusion.dll path %s\n", wine_dbgstr_w(dllpath));

        FreeLibrary(hdll);
    }

    hr = pLoadLibraryShim(fusion, v4_0, NULL, &hdll);
    ok(hr == S_OK || hr == E_HANDLE, "LoadLibraryShim failed, hr=%x\n", hr);
    if (SUCCEEDED(hr))
    {
        /* LoadLibraryShim with a NULL version prefers 2.0 and earlier */
        if (!latest)
            latest = v4_0;

        GetModuleFileNameW(hdll, dllpath, MAX_PATH);

        todo_wine ok(strstriW(dllpath, v4_0) != 0, "incorrect fusion.dll path %s\n", wine_dbgstr_w(dllpath));
        ok(strstriW(dllpath, fusiondll) != 0, "incorrect fusion.dll path %s\n", wine_dbgstr_w(dllpath));

        FreeLibrary(hdll);
    }

    hr = pLoadLibraryShim(fusion, vbogus, NULL, &hdll);
    todo_wine ok(hr == E_HANDLE, "LoadLibraryShim failed, hr=%x\n", hr);
    if (SUCCEEDED(hr))
        FreeLibrary(hdll);

    hr = pLoadLibraryShim(fusion, NULL, NULL, &hdll);
    ok(hr == S_OK, "LoadLibraryShim failed, hr=%x\n", hr);
    if (SUCCEEDED(hr))
    {
        GetModuleFileNameW(hdll, dllpath, MAX_PATH);

        if (latest)
            todo_wine ok(strstriW(dllpath, latest) != 0, "incorrect fusion.dll path %s\n", wine_dbgstr_w(dllpath));
        ok(strstriW(dllpath, fusiondll) != 0, "incorrect fusion.dll path %s\n", wine_dbgstr_w(dllpath));

        FreeLibrary(hdll);
    }

    hr = pLoadLibraryShim(fusiondll, NULL, NULL, &hdll);
    ok(hr == S_OK, "LoadLibraryShim failed, hr=%x\n", hr);
    if (SUCCEEDED(hr))
    {
        GetModuleFileNameW(hdll, dllpath, MAX_PATH);

        if (latest)
            todo_wine ok(strstriW(dllpath, latest) != 0, "incorrect fusion.dll path %s\n", wine_dbgstr_w(dllpath));
        ok(strstriW(dllpath, fusiondll) != 0, "incorrect fusion.dll path %s\n", wine_dbgstr_w(dllpath));

        FreeLibrary(hdll);
    }

    hr = pLoadLibraryShim(nosuchdll, latest, NULL, &hdll);
    ok(hr == E_HANDLE, "LoadLibraryShim failed, hr=%x\n", hr);
    if (SUCCEEDED(hr))
        FreeLibrary(hdll);

    hr = pLoadLibraryShim(gdipdll, latest, NULL, &hdll);
    todo_wine ok(hr == E_HANDLE, "LoadLibraryShim failed, hr=%x\n", hr);
    if (SUCCEEDED(hr))
        FreeLibrary(hdll);

    hr = pLoadLibraryShim(gdidll, latest, NULL, &hdll);
    todo_wine ok(hr == E_HANDLE, "LoadLibraryShim failed, hr=%x\n", hr);
    if (SUCCEEDED(hr))
        FreeLibrary(hdll);
}

START_TEST(mscoree)
{
    if (!init_functionpointers())
        return;

    test_versioninfo();
    test_loadlibraryshim();

    FreeLibrary(hmscoree);
}
