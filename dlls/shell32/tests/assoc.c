/* Unit test suite for various shell Association objects
 *
 * Copyright 2012 Detlef Riekenberg
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

#define COBJMACROS

#include <stdarg.h>

#include "shlwapi.h"
#include "shlguid.h"
#include "shobjidl.h"

#include "wine/test.h"


static void test_IQueryAssociations_QueryInterface(void)
{
    IQueryAssociations *qa;
    IQueryAssociations *qa2;
    IUnknown *unk;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_QueryAssociations, NULL, CLSCTX_INPROC_SERVER, &IID_IQueryAssociations, (void*)&qa);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = IQueryAssociations_QueryInterface(qa, &IID_IQueryAssociations, (void**)&qa2);
    ok(hr == S_OK, "QueryInterface (IQueryAssociations) returned 0x%lx\n", hr);
    if (SUCCEEDED(hr)) {
        IQueryAssociations_Release(qa2);
    }

    hr = IQueryAssociations_QueryInterface(qa, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "QueryInterface (IUnknown) returned 0x%lx\n", hr);
    if (SUCCEEDED(hr)) {
        IUnknown_Release(unk);
    }

    hr = IQueryAssociations_QueryInterface(qa, &IID_IUnknown, NULL);
    ok(hr == E_POINTER, "got 0x%lx (expected E_POINTER)\n", hr);

    IQueryAssociations_Release(qa);
}


static void test_IApplicationAssociationRegistration_QueryInterface(IApplicationAssociationRegistration *appreg)
{
    IApplicationAssociationRegistration *appreg2;
    IUnknown *unk;
    HRESULT hr;

    hr = IApplicationAssociationRegistration_QueryInterface(appreg, &IID_IApplicationAssociationRegistration,
       (void**)&appreg2);
    ok(hr == S_OK, "QueryInterface (IApplicationAssociationRegistration) returned 0x%lx\n", hr);
    if (SUCCEEDED(hr)) {
        IApplicationAssociationRegistration_Release(appreg2);
    }

    hr = IApplicationAssociationRegistration_QueryInterface(appreg, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "QueryInterface (IUnknown) returned 0x%lx\n", hr);
    if (SUCCEEDED(hr)) {
        IUnknown_Release(unk);
    }

    hr = IApplicationAssociationRegistration_QueryInterface(appreg, &IID_IUnknown, NULL);
    ok(hr == E_POINTER, "got 0x%lx (expected E_POINTER)\n", hr);
}

struct assoc_getstring_test
{
    const WCHAR *key;
    ASSOCF       flags;
    ASSOCSTR     str;
    DWORD        len;
    HRESULT      hr;
    HRESULT      brokenhr;
};

static struct assoc_getstring_test getstring_tests[] =
{
    { L"http", 0, ASSOCSTR_EXECUTABLE, 2, 0x8007007a /* E_NOT_SUFFICIENT_BUFFER */, S_OK },
    { L"http", ASSOCF_NOTRUNCATE, ASSOCSTR_EXECUTABLE, 2, E_POINTER },
    { NULL }
};

static void getstring_test(LPCWSTR assocName, HKEY progIdKey, ASSOCSTR str, LPCWSTR expected_string, int line)
{
    IQueryAssociations *assoc;
    WCHAR *buffer;
    HRESULT hr;
    DWORD len;

    hr = CoCreateInstance(&CLSID_QueryAssociations, NULL, CLSCTX_INPROC_SERVER, &IID_IQueryAssociations, (void*)&assoc);
    ok_(__FILE__, line)(hr == S_OK, "failed to create IQueryAssociations, 0x%lx\n", hr);
    hr = IQueryAssociations_Init(assoc, ASSOCF_NONE, assocName, progIdKey, NULL);
    ok_(__FILE__, line)(hr == S_OK, "IQueryAssociations::Init failed, 0x%lx\n", hr);

    hr = IQueryAssociations_GetString(assoc, ASSOCF_NONE, str, NULL, NULL, &len);
    if (expected_string) {
        ok_(__FILE__, line)(hr == S_FALSE, "GetString returned 0x%lx, expected S_FALSE\n", hr);
        if (hr != S_FALSE) {
            /* don't try to allocate memory using uninitialized len */
            IQueryAssociations_Release(assoc);
            return;
        }

        buffer = malloc(len * sizeof(WCHAR));
        ok_(__FILE__, line)(buffer != NULL, "out of memory\n");
        hr = IQueryAssociations_GetString(assoc, 0, str, NULL, buffer, &len);
        ok_(__FILE__, line)(hr == S_OK, "GetString returned 0x%lx, expected S_OK\n", hr);

        ok_(__FILE__, line)(lstrcmpW(buffer, expected_string) == 0, "GetString returned %s, expected %s\n",
                wine_dbgstr_w(buffer), wine_dbgstr_w(expected_string));
        free(buffer);
    } else {
        ok_(__FILE__, line)(FAILED(hr), "GetString returned 0x%lx, expected failure\n", hr);
    }

    IQueryAssociations_Release(assoc);
}

static void test_IQueryAssociations_GetString(void)
{
    static WCHAR test_progidW[] = L"testfile";
    static WCHAR test_iconW[] = L"shell32.dll,1"; /* folder.ico, why not */
    HKEY test_extension_key;
    HKEY test_progid_key;
    HKEY test_defaulticon_key;
    LRESULT r;

    struct assoc_getstring_test *ptr = getstring_tests;
    IQueryAssociations *assoc;
    HRESULT hr;
    DWORD len;
    int i = 0;

    r = RegCreateKeyExW(HKEY_CLASSES_ROOT, L".test", 0, NULL, 0, KEY_ALL_ACCESS, NULL, &test_extension_key, NULL);
    if (r == ERROR_ACCESS_DENIED)
    {
        win_skip("Not enough permissions to create a test key.\n");
        return;
    }

    ok(r == ERROR_SUCCESS, "RegCreateKeyExW(HKCR, \".test\") failed: 0x%Ix\n", r);
    r = RegSetValueExW(test_extension_key, NULL, 0, REG_SZ, (PBYTE)test_progidW, sizeof(test_progidW));
    ok(r == ERROR_SUCCESS, "RegSetValueExW(HKCR\\.test, NULL, \"testfile\") failed: 0x%Ix\n", r);

    /* adding progid key with no information should fail to return information */
    r = RegCreateKeyExW(HKEY_CLASSES_ROOT, test_progidW, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &test_progid_key, NULL);
    ok(r == ERROR_SUCCESS, "RegCreateKeyExW(HKCR, \"testfile\") failed: 0x%Ix\n", r);
    getstring_test(L".test", NULL, ASSOCSTR_DEFAULTICON, NULL, __LINE__);
    getstring_test(test_progidW, NULL, ASSOCSTR_DEFAULTICON, NULL, __LINE__);
    getstring_test(NULL, test_progid_key, ASSOCSTR_DEFAULTICON, NULL, __LINE__);

    /* adding information to the progid should return that information */
    r = RegCreateKeyExW(test_progid_key, L"DefaultIcon", 0, NULL, 0, KEY_ALL_ACCESS, NULL, &test_defaulticon_key, NULL);
    ok(r == ERROR_SUCCESS, "RegCreateKeyExW(HKCR\\testfile\\DefaultIcon) failed: 0x%Ix\n", r);
    r = RegSetValueExW(test_defaulticon_key, NULL, 0, REG_SZ, (PBYTE)test_iconW, sizeof(test_iconW));
    ok(r == ERROR_SUCCESS, "RegSetValueExW(HKCR\\testfile\\DefaultIcon, NULL, \"folder.ico\") failed: 0x%Ix\n", r);
    getstring_test(L".test", NULL, ASSOCSTR_DEFAULTICON, test_iconW, __LINE__);
    getstring_test(test_progidW, NULL, ASSOCSTR_DEFAULTICON, test_iconW, __LINE__);
    getstring_test(NULL, test_progid_key, ASSOCSTR_DEFAULTICON, test_iconW, __LINE__);

    RegDeleteKeyW(test_progid_key, L"DefaultIcon");
    RegDeleteKeyW(HKEY_CLASSES_ROOT, test_progidW);
    RegDeleteKeyW(HKEY_CLASSES_ROOT, L".test");

    hr = CoCreateInstance(&CLSID_QueryAssociations, NULL, CLSCTX_INPROC_SERVER, &IID_IQueryAssociations, (void*)&assoc);
    ok(hr == S_OK, "failed to create object, 0x%lx\n", hr);

    hr = IQueryAssociations_Init(assoc, ASSOCF_NONE, L"http", NULL, NULL);
    ok(hr == S_OK, "Init failed, 0x%lx\n", hr);

    len = 0;
    hr = IQueryAssociations_GetString(assoc, ASSOCF_NONE, ASSOCSTR_EXECUTABLE, NULL, NULL, &len);
    ok(hr == S_FALSE, "got 0x%08lx\n", hr);
    ok(len > 0, "got wrong needed length, %ld\n", len);

    while (ptr->key)
    {
        WCHAR buffW[MAX_PATH];
        DWORD len;

        hr = IQueryAssociations_Init(assoc, ASSOCF_NONE, ptr->key, NULL, NULL);
        ok(hr == S_OK, "%d: Init failed, 0x%lx\n", i, hr);

        len = ptr->len;
        buffW[0] = ptr->flags & ASSOCF_NOTRUNCATE ? 0x1 : 0;
        hr = IQueryAssociations_GetString(assoc, ptr->flags, ptr->str, NULL, buffW, &len);
        if (hr != ptr->hr)
            ok(broken(hr == ptr->brokenhr), "%d: GetString failed, 0x%08lx\n", i, hr);
        else
        {
            ok(hr == ptr->hr, "%d: GetString failed, 0x%08lx\n", i, hr);
            ok(len > ptr->len, "%d: got needed length %ld\n", i, len);
        }

        /* even with ASSOCF_NOTRUNCATE it's null terminated */
        if ((ptr->flags & ASSOCF_NOTRUNCATE) && (ptr->len > 0))
            ok(buffW[0] == 0 || broken(buffW[0] == 0x1) /* pre win7 */, "%d: got %x\n", i, buffW[0]);

        if (!(ptr->flags & ASSOCF_NOTRUNCATE) && ptr->len && FAILED(ptr->hr))
            ok(buffW[0] != 0, "%d: got %x\n", i, buffW[0]);

        i++;
        ptr++;
    }

    IQueryAssociations_Release(assoc);
}

static void test_IQueryAssociations_Init(void)
{
    IQueryAssociations *assoc;
    HRESULT hr;
    DWORD len;

    hr = CoCreateInstance(&CLSID_QueryAssociations, NULL, CLSCTX_INPROC_SERVER, &IID_IQueryAssociations, (void*)&assoc);
    ok(hr == S_OK, "failed to create object, 0x%lx\n", hr);

    hr = IQueryAssociations_Init(assoc, ASSOCF_NONE, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "Init failed, 0x%08lx\n", hr);

    hr = IQueryAssociations_Init(assoc, ASSOCF_NONE, L"http", NULL, NULL);
    ok(hr == S_OK, "Init failed, 0x%08lx\n", hr);

    hr = IQueryAssociations_Init(assoc, ASSOCF_NONE, L"badbad", NULL, NULL);
    ok(hr == S_OK || broken(hr == S_FALSE) /* pre-vista */, "Init failed, 0x%08lx\n", hr);

    len = 0;
    hr = IQueryAssociations_GetString(assoc, ASSOCF_NONE, ASSOCSTR_EXECUTABLE, NULL, NULL, &len);
    ok(hr == HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION) || broken(hr == E_FAIL) /* pre-vista */, "got 0x%08lx\n", hr);

    IQueryAssociations_Release(assoc);
}

static void test_IApplicationAssociationRegistration_QueryCurrentDefault(IApplicationAssociationRegistration *appreg)
{
    HRESULT hr;
    LPWSTR assocprog = NULL;

    hr = IApplicationAssociationRegistration_QueryCurrentDefault(appreg, L"", AT_URLPROTOCOL, AL_EFFECTIVE, &assocprog);
    ok(hr == E_INVALIDARG, "got 0x%lx\n", hr);

    hr = IApplicationAssociationRegistration_QueryCurrentDefault(appreg, L"", AT_FILEEXTENSION, AL_EFFECTIVE, &assocprog);
    ok(hr == E_INVALIDARG, "got 0x%lx\n", hr);

    hr = IApplicationAssociationRegistration_QueryCurrentDefault(appreg, L" .txt", AT_FILEEXTENSION, AL_EFFECTIVE, &assocprog);
    ok(hr == E_INVALIDARG || hr == HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION) /*Win8*/, "got 0x%lx\n", hr);

    hr = IApplicationAssociationRegistration_QueryCurrentDefault(appreg, L"http", AT_URLPROTOCOL, AL_EFFECTIVE, NULL);
    ok(hr == E_INVALIDARG, "got 0x%lx\n", hr);

    /* AT_FILEEXTENSION must start with a period */
    hr = IApplicationAssociationRegistration_QueryCurrentDefault(appreg, L".txt", AT_FILEEXTENSION, AL_EFFECTIVE, &assocprog);
    ok(hr == S_OK, "got 0x%lx\n", hr);
    trace("%s\n", wine_dbgstr_w(assocprog));
    CoTaskMemFree(assocprog);

    hr = IApplicationAssociationRegistration_QueryCurrentDefault(appreg, L"", AT_STARTMENUCLIENT, AL_EFFECTIVE, &assocprog);
    ok(hr == HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION), "got 0x%lx\n", hr);

    hr = IApplicationAssociationRegistration_QueryCurrentDefault(appreg, L"", AT_MIMETYPE, AL_EFFECTIVE, &assocprog);
    ok(hr == HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION), "got 0x%lx\n", hr);

    hr = IApplicationAssociationRegistration_QueryCurrentDefault(appreg, L"http", AT_URLPROTOCOL, AL_EFFECTIVE, &assocprog);
    ok(hr == S_OK, "got 0x%lx\n", hr);
    trace("%s\n", wine_dbgstr_w(assocprog));

    CoTaskMemFree(assocprog);
}

START_TEST(assoc)
{
    IQueryAssociations *qa;
    IApplicationAssociationRegistration *appreg = NULL;
    HRESULT hr;

    CoInitialize(NULL);

    /* this works since XP */
    hr = CoCreateInstance(&CLSID_QueryAssociations, NULL, CLSCTX_INPROC_SERVER, &IID_IQueryAssociations, (void*)&qa);
    if (hr == S_OK)
    {
        test_IQueryAssociations_QueryInterface();
        test_IQueryAssociations_GetString();
        test_IQueryAssociations_Init();

        IQueryAssociations_Release(qa);
    }
    else
        win_skip("IQueryAssociations not supported, 0x%lx\n", hr);

    /* this works since Vista */
    hr = CoCreateInstance(&CLSID_ApplicationAssociationRegistration, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IApplicationAssociationRegistration, (LPVOID*)&appreg);
    if (hr == S_OK)
    {
        test_IApplicationAssociationRegistration_QueryInterface(appreg);
        test_IApplicationAssociationRegistration_QueryCurrentDefault(appreg);

        IApplicationAssociationRegistration_Release(appreg);
    }
    else
        win_skip("IApplicationAssociationRegistration not supported: 0x%lx\n", hr);

    CoUninitialize();
}
