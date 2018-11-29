/*
 * Copyright 2018 Nikolay Sivov for CodeWeavers
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

#include <windows.h>
#include "initguid.h"
#include "objidl.h"

#include "wine/test.h"

static HRESULT (WINAPI *pGetProcessReference)(IUnknown **);
static void (WINAPI *pSetProcessReference)(IUnknown *);
static HRESULT (WINAPI *pSHGetInstanceExplorer)(IUnknown **);
static int (WINAPI *pSHUnicodeToAnsi)(const WCHAR *, char *, int);
static int (WINAPI *pSHAnsiToUnicode)(const char *, WCHAR *, int);
static int (WINAPI *pSHAnsiToAnsi)(const char *, char *, int);
static int (WINAPI *pSHUnicodeToUnicode)(const WCHAR *, WCHAR *, int);
static HKEY (WINAPI *pSHRegDuplicateHKey)(HKEY);

static void init(HMODULE hshcore)
{
#define X(f) p##f = (void*)GetProcAddress(hshcore, #f)
    X(GetProcessReference);
    X(SetProcessReference);
    X(SHUnicodeToAnsi);
    X(SHAnsiToUnicode);
    X(SHAnsiToAnsi);
    X(SHUnicodeToUnicode);
    X(SHRegDuplicateHKey);
#undef X
}

static HRESULT WINAPI unk_QI(IUnknown *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

struct test_unk
{
    IUnknown IUnknown_iface;
    LONG refcount;
};

static struct test_unk *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct test_unk, IUnknown_iface);
}

static ULONG WINAPI unk_AddRef(IUnknown *iface)
{
    struct test_unk *obj = impl_from_IUnknown(iface);
    return InterlockedIncrement(&obj->refcount);
}

static ULONG WINAPI unk_Release(IUnknown *iface)
{
    struct test_unk *obj = impl_from_IUnknown(iface);
    return InterlockedDecrement(&obj->refcount);
}

static const IUnknownVtbl testunkvtbl =
{
    unk_QI,
    unk_AddRef,
    unk_Release,
};

static void test_unk_init(struct test_unk *testunk)
{
    testunk->IUnknown_iface.lpVtbl = &testunkvtbl;
    testunk->refcount = 1;
}

static void test_process_reference(void)
{
    struct test_unk test_unk, test_unk2;
    IUnknown *obj;
    HMODULE hmod;
    HRESULT hr;

    obj = (void *)0xdeadbeef;
    hr = pGetProcessReference(&obj);
    ok(hr == E_FAIL, "Unexpected hr %#x.\n", hr);
    ok(obj == NULL, "Unexpected pointer.\n");

    test_unk_init(&test_unk);
    test_unk_init(&test_unk2);

    pSetProcessReference(&test_unk.IUnknown_iface);
    ok(test_unk.refcount == 1, "Unexpected refcount %u.\n", test_unk.refcount);
    pSetProcessReference(&test_unk2.IUnknown_iface);
    ok(test_unk.refcount == 1, "Unexpected refcount %u.\n", test_unk.refcount);
    ok(test_unk2.refcount == 1, "Unexpected refcount %u.\n", test_unk2.refcount);

    hr = pGetProcessReference(&obj);
    ok(hr == S_OK, "Failed to get reference, hr %#x.\n", hr);
    ok(obj == &test_unk2.IUnknown_iface, "Unexpected pointer.\n");
    ok(test_unk2.refcount == 2, "Unexpected refcount %u.\n", test_unk2.refcount);

    hmod = LoadLibraryA("shell32.dll");

    pSHGetInstanceExplorer = (void *)GetProcAddress(hmod, "SHGetInstanceExplorer");
    hr = pSHGetInstanceExplorer(&obj);
    ok(hr == S_OK, "Failed to get reference, hr %#x.\n", hr);
    ok(obj == &test_unk2.IUnknown_iface, "Unexpected pointer.\n");
    ok(test_unk2.refcount == 3, "Unexpected refcount %u.\n", test_unk2.refcount);
}

static void test_SHUnicodeToAnsi(void)
{
    static const WCHAR testW[] = {'t','e','s','t',0};
    static const WCHAR emptyW[] = { 0 };
    char buff[16];
    int ret;

    ret = pSHUnicodeToAnsi(NULL, NULL, 0);
    ok(ret == 0, "Unexpected return value %d.\n", ret);

    strcpy(buff, "abc");
    ret = pSHUnicodeToAnsi(NULL, buff, 2);
    ok(ret == 1, "Unexpected return value %d.\n", ret);
    ok(buff[0] == 0 && buff[1] == 'b', "Unexpected buffer contents.\n");

    buff[0] = 1;
    ret = pSHUnicodeToAnsi(NULL, buff, 0);
    ok(ret == 0, "Unexpected return value %d.\n", ret);
    ok(buff[0] == 1, "Unexpected buffer contents.\n");

    buff[0] = 1;
    strcpy(buff, "test");
    ret = pSHUnicodeToAnsi(emptyW, buff, 1);
    ok(ret == 1, "Unexpected return value %d.\n", ret);
    ok(*buff == 0, "Unexpected buffer contents.\n");

    buff[0] = 1;
    ret = pSHUnicodeToAnsi(testW, buff, 0);
    ok(ret == 0, "Unexpected return value %d.\n", ret);
    ok(buff[0] == 1, "Unexpected buffer contents.\n");

    buff[0] = 1;
    ret = pSHUnicodeToAnsi(testW, buff, 1);
    ok(ret == 1, "Unexpected return value %d.\n", ret);
    ok(*buff == 0, "Unexpected buffer contents.\n");

    ret = pSHUnicodeToAnsi(testW, buff, 16);
    ok(ret == 5, "Unexpected return value %d.\n", ret);
    ok(!strcmp(buff, "test"), "Unexpected buffer contents.\n");

    ret = pSHUnicodeToAnsi(testW, buff, 2);
    ok(ret == 2, "Unexpected return value %d.\n", ret);
    ok(!strcmp(buff, "t"), "Unexpected buffer contents.\n");
}

static void test_SHAnsiToUnicode(void)
{
    static const WCHAR testW[] = {'t','e','s','t',0};
    WCHAR buffW[16];
    int ret;

    ret = pSHAnsiToUnicode(NULL, NULL, 0);
    ok(ret == 0, "Unexpected return value %d.\n", ret);

    buffW[0] = 1;
    buffW[1] = 2;
    ret = pSHAnsiToUnicode(NULL, buffW, 2);
    ok(ret == 1, "Unexpected return value %d.\n", ret);
    ok(buffW[0] == 0 && buffW[1] == 2, "Unexpected buffer contents.\n");

    buffW[0] = 1;
    ret = pSHAnsiToUnicode(NULL, buffW, 0);
    ok(ret == 0, "Unexpected return value %d.\n", ret);
    ok(buffW[0] == 1, "Unexpected buffer contents.\n");

    buffW[0] = 1;
    ret = pSHAnsiToUnicode("", buffW, 1);
    ok(ret == 1, "Unexpected return value %d.\n", ret);
    ok(*buffW == 0, "Unexpected buffer contents.\n");

    buffW[0] = 1;
    ret = pSHAnsiToUnicode("test", buffW, 0);
    ok(ret == 0, "Unexpected return value %d.\n", ret);
    ok(buffW[0] == 1, "Unexpected buffer contents.\n");

    buffW[0] = 1;
    ret = pSHAnsiToUnicode("test", buffW, 1);
    ok(ret == 1, "Unexpected return value %d.\n", ret);
    ok(*buffW == 0, "Unexpected buffer contents.\n");

    ret = pSHAnsiToUnicode("test", buffW, 16);
    ok(ret == 5, "Unexpected return value %d.\n", ret);
    ok(!lstrcmpW(buffW, testW), "Unexpected buffer contents.\n");

    ret = pSHAnsiToUnicode("test", buffW, 2);
    ok(ret == 2, "Unexpected return value %d.\n", ret);
    ok(buffW[0] == 't' && buffW[1] == 0, "Unexpected buffer contents.\n");
}

static void test_SHAnsiToAnsi(void)
{
    char buff[16];
    int ret;

    ret = pSHAnsiToAnsi(NULL, NULL, 0);
    ok(ret == 0, "Unexpected return value %d.\n", ret);

    strcpy(buff, "abcdefghijklm");
    ret = pSHAnsiToAnsi("test", buff, 3);
    ok(ret == 0, "Unexpected return value %d.\n", ret);
    ok(!strcmp(buff, "te"), "Unexpected buffer contents.\n");
    ok(buff[3] == 'd', "Unexpected buffer contents.\n");

    strcpy(buff, "abcdefghijklm");
    ret = pSHAnsiToAnsi("", buff, 3);
    ok(ret == 1, "Unexpected return value %d.\n", ret);
    ok(!*buff, "Unexpected buffer contents.\n");
    ok(buff[3] == 'd', "Unexpected buffer contents.\n");

    strcpy(buff, "abcdefghijklm");
    ret = pSHAnsiToAnsi("test", buff, 4);
    ok(ret == 0, "Unexpected return value %d.\n", ret);
    ok(!strcmp(buff, "tes"), "Unexpected buffer contents.\n");
    ok(buff[4] == 'e', "Unexpected buffer contents.\n");

    strcpy(buff, "abcdefghijklm");
    ret = pSHAnsiToAnsi("test", buff, 5);
    ok(ret == 5, "Unexpected return value %d.\n", ret);
    ok(!strcmp(buff, "test"), "Unexpected buffer contents.\n");
    ok(buff[5] == 'f', "Unexpected buffer contents.\n");

    strcpy(buff, "abcdefghijklm");
    ret = pSHAnsiToAnsi("test", buff, 6);
    ok(ret == 5, "Unexpected return value %d.\n", ret);
    ok(!strcmp(buff, "test"), "Unexpected buffer contents.\n");
    ok(buff[5] == 'f', "Unexpected buffer contents.\n");
}

static void test_SHUnicodeToUnicode(void)
{
    static const WCHAR testW[] = {'t','e','s','t',0};
    static const WCHAR strW[] = {'a','b','c','d','e','f','g','h','i','k','l','m',0};
    static const WCHAR emptyW[] = { 0 };
    WCHAR buff[16];
    int ret;

    ret = pSHUnicodeToUnicode(NULL, NULL, 0);
    ok(ret == 0, "Unexpected return value %d.\n", ret);

    lstrcpyW(buff, strW);
    ret = pSHUnicodeToUnicode(testW, buff, 3);
    ok(ret == 0, "Unexpected return value %d.\n", ret);
    ok(!memcmp(buff, testW, 2 * sizeof(WCHAR)) && !buff[2], "Unexpected buffer contents.\n");
    ok(buff[3] == 'd', "Unexpected buffer contents.\n");

    lstrcpyW(buff, strW);
    ret = pSHUnicodeToUnicode(emptyW, buff, 3);
    ok(ret == 1, "Unexpected return value %d.\n", ret);
    ok(!*buff, "Unexpected buffer contents.\n");
    ok(buff[3] == 'd', "Unexpected buffer contents.\n");

    lstrcpyW(buff, strW);
    ret = pSHUnicodeToUnicode(testW, buff, 4);
    ok(ret == 0, "Unexpected return value %d.\n", ret);
    ok(!memcmp(buff, testW, 3 * sizeof(WCHAR)) && !buff[3], "Unexpected buffer contents.\n");
    ok(buff[4] == 'e', "Unexpected buffer contents.\n");

    lstrcpyW(buff, strW);
    ret = pSHUnicodeToUnicode(testW, buff, 5);
    ok(ret == 5, "Unexpected return value %d.\n", ret);
    ok(!lstrcmpW(buff, testW), "Unexpected buffer contents.\n");
    ok(buff[5] == 'f', "Unexpected buffer contents.\n");

    lstrcpyW(buff, strW);
    ret = pSHUnicodeToUnicode(testW, buff, 6);
    ok(ret == 5, "Unexpected return value %d.\n", ret);
    ok(!lstrcmpW(buff, testW), "Unexpected buffer contents.\n");
    ok(buff[5] == 'f', "Unexpected buffer contents.\n");
}

static void test_SHRegDuplicateHKey(void)
{
    HKEY hkey, hkey2;
    DWORD ret;

    ret = RegCreateKeyA(HKEY_CURRENT_USER, "Software\\Wine\\Test", &hkey);
    ok(!ret, "Failed to create test key, ret %d.\n", ret);

    hkey2 = pSHRegDuplicateHKey(hkey);
    ok(hkey2 != NULL && hkey2 != hkey, "Unexpected duplicate key.\n");

    RegCloseKey(hkey2);
    RegCloseKey(hkey);

    RegDeleteKeyA(HKEY_CURRENT_USER, "Software\\Wine\\Test");
}

START_TEST(shcore)
{
    HMODULE hshcore = LoadLibraryA("shcore.dll");

    if (!hshcore)
    {
        win_skip("Shcore.dll is not available.\n");
        return;
    }

    init(hshcore);

    test_process_reference();
    test_SHUnicodeToAnsi();
    test_SHAnsiToUnicode();
    test_SHAnsiToAnsi();
    test_SHUnicodeToUnicode();
    test_SHRegDuplicateHKey();
}
