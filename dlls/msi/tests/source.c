/*
 * Tests for MSI Source functions
 *
 * Copyright (C) 2006 James Hawkins
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

#define _WIN32_MSI 300

#include <stdio.h>

#include <windows.h>
#include <msiquery.h>
#include <msidefs.h>
#include <msi.h>
#include <sddl.h>

#include "wine/test.h"

static BOOL (WINAPI *pConvertSidToStringSidA)(PSID, LPSTR*);
static UINT (WINAPI *pMsiSourceListGetInfoA)
    (LPCSTR, LPCSTR, MSIINSTALLCONTEXT, DWORD, LPCSTR, LPSTR, LPDWORD);
static UINT (WINAPI *pMsiSourceListAddSourceExA)
    (LPCSTR, LPCSTR, MSIINSTALLCONTEXT, DWORD, LPCSTR, DWORD);

static void init_functionpointers(void)
{
    HMODULE hmsi = GetModuleHandleA("msi.dll");
    HMODULE hadvapi32 = GetModuleHandleA("advapi32.dll");

#define GET_PROC(dll, func) \
    p ## func = (void *)GetProcAddress(dll, #func); \
    if(!p ## func) \
      trace("GetProcAddress(%s) failed\n", #func);

    GET_PROC(hmsi, MsiSourceListAddSourceExA)
    GET_PROC(hmsi, MsiSourceListGetInfoA)

    GET_PROC(hadvapi32, ConvertSidToStringSidA)

#undef GET_PROC
}

/* copied from dlls/msi/registry.c */
static BOOL squash_guid(LPCWSTR in, LPWSTR out)
{
    DWORD i,n=1;
    GUID guid;

    if (FAILED(CLSIDFromString((LPOLESTR)in, &guid)))
        return FALSE;

    for(i=0; i<8; i++)
        out[7-i] = in[n++];
    n++;
    for(i=0; i<4; i++)
        out[11-i] = in[n++];
    n++;
    for(i=0; i<4; i++)
        out[15-i] = in[n++];
    n++;
    for(i=0; i<2; i++)
    {
        out[17+i*2] = in[n++];
        out[16+i*2] = in[n++];
    }
    n++;
    for( ; i<8; i++)
    {
        out[17+i*2] = in[n++];
        out[16+i*2] = in[n++];
    }
    out[32]=0;
    return TRUE;
}

static void create_test_guid(LPSTR prodcode, LPSTR squashed)
{
    WCHAR guidW[MAX_PATH];
    WCHAR squashedW[MAX_PATH];
    GUID guid;
    HRESULT hr;
    int size;

    hr = CoCreateGuid(&guid);
    ok(hr == S_OK, "Expected S_OK, got %d\n", hr);

    size = StringFromGUID2(&guid, (LPOLESTR)guidW, MAX_PATH);
    ok(size == 39, "Expected 39, got %d\n", hr);

    WideCharToMultiByte(CP_ACP, 0, guidW, size, prodcode, MAX_PATH, NULL, NULL);
    squash_guid(guidW, squashedW);
    WideCharToMultiByte(CP_ACP, 0, squashedW, -1, squashed, MAX_PATH, NULL, NULL);
}

static void get_user_sid(LPSTR *usersid)
{
    HANDLE token;
    BYTE buf[1024];
    DWORD size;
    PTOKEN_USER user;

    OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token);
    size = sizeof(buf);
    GetTokenInformation(token, TokenUser, (void *)buf, size, &size);
    user = (PTOKEN_USER)buf;
    pConvertSidToStringSidA(user->User.Sid, usersid);
}

static void check_reg_str(HKEY prodkey, LPCSTR name, LPCSTR expected, BOOL bcase, DWORD line)
{
    char val[MAX_PATH];
    DWORD size, type;
    LONG res;

    size = MAX_PATH;
    val[0] = '\0';
    res = RegQueryValueExA(prodkey, name, NULL, &type, (LPBYTE)val, &size);

    if (res != ERROR_SUCCESS || (type != REG_SZ && type != REG_EXPAND_SZ))
    {
        ok_(__FILE__, line)(FALSE, "Key doesn't exist or wrong type\n");
        return;
    }

    if (!expected)
        ok_(__FILE__, line)(lstrlenA(val) == 0, "Expected empty string, got %s\n", val);
    else
    {
        if (bcase)
            ok_(__FILE__, line)(!lstrcmpA(val, expected), "Expected %s, got %s\n", expected, val);
        else
            ok_(__FILE__, line)(!lstrcmpiA(val, expected), "Expected %s, got %s\n", expected, val);
    }
}

#define CHECK_REG_STR(prodkey, name, expected) \
    check_reg_str(prodkey, name, expected, TRUE, __LINE__);

static void test_MsiSourceListGetInfo(void)
{
    CHAR prodcode[MAX_PATH];
    CHAR prod_squashed[MAX_PATH];
    CHAR keypath[MAX_PATH*2];
    CHAR value[MAX_PATH];
    LPSTR usersid;
    LPCSTR data;
    LONG res;
    UINT r;
    HKEY userkey, hkey, media;
    DWORD size;

    if (!pMsiSourceListGetInfoA)
    {
        skip("Skipping MsiSourceListGetInfoA tests\n");
        return;
    }

    create_test_guid(prodcode, prod_squashed);
    get_user_sid(&usersid);

    /* NULL szProductCodeOrPatchCode */
    r = pMsiSourceListGetInfoA(NULL, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAME, NULL, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* empty szProductCodeOrPatchCode */
    r = pMsiSourceListGetInfoA("", usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAME, NULL, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* garbage szProductCodeOrPatchCode */
    r = pMsiSourceListGetInfoA("garbage", usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAME, NULL, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /*  szProductCodeOrPatchCode */
    r = pMsiSourceListGetInfoA("garbage", usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAME, NULL, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* guid without brackets */
    r = pMsiSourceListGetInfoA("51CD2AD5-0482-4C46-8DDD-0ED1022AA1AA", usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAME, NULL, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* guid with brackets */
    r = pMsiSourceListGetInfoA("{51CD2AD5-0482-4C46-8DDD-0ED1022AA1AA}", usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAME, NULL, NULL);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);

    /* same length as guid, but random */
    r = pMsiSourceListGetInfoA("ADKD-2KSDFF2-DKK1KNFJASD9GLKWME-1I3KAD", usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAME, NULL, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* invalid context */
    r = pMsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_NONE,
                              MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAME, NULL, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* another invalid context */
    r = pMsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_ALLUSERMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAME, NULL, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* yet another invalid context */
    r = pMsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_ALL,
                              MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAME, NULL, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* mix two valid contexts */
    r = pMsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERMANAGED | MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAME, NULL, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* invalid option */
    r = pMsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              4, INSTALLPROPERTY_PACKAGENAME, NULL, NULL);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);

    /* NULL property */
    r = pMsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, NULL, NULL, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* empty property */
    r = pMsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, "", NULL, NULL);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);

    /* value is non-NULL while size is NULL */
    r = pMsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAME, value, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* size is non-NULL while value is NULL */
    r = pMsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAME, NULL, &size);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);

    lstrcpyA(keypath, "Software\\Microsoft\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_CURRENT_USER, keypath, &userkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* user product key exists */
    r = pMsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAME, NULL, &size);
    ok(r == ERROR_BAD_CONFIGURATION, "Expected ERROR_BAD_CONFIGURATION, got %d\n", r);

    res = RegCreateKeyA(userkey, "SourceList", &hkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* SourceList key exists */
    size = 0xdeadbeef;
    r = pMsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAME, NULL, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(size == 0, "Expected 0, got %d\n", size);

    data = "msitest.msi";
    res = RegSetValueExA(hkey, "PackageName", 0, REG_SZ, (const BYTE *)data, lstrlenA(data) + 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* PackageName value exists */
    size = 0xdeadbeef;
    r = pMsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAME, NULL, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(size == 11, "Expected 11, got %d\n", size);

    /* read the value, don't change size */
    lstrcpyA(value, "aaa");
    r = pMsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAME, value, &size);
    ok(r == ERROR_MORE_DATA, "Expected ERROR_MORE_DATA, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected 'aaa', got %s\n", value);
    ok(size == 11, "Expected 11, got %d\n", size);

    /* read the value, fix size */
    size++;
    r = pMsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAME, value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, "msitest.msi"), "Expected 'msitest.msi', got %s\n", value);
    ok(size == 11, "Expected 11, got %d\n", size);

    /* empty property now that product key exists */
    size = 0xdeadbeef;
    r = pMsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, "", NULL, &size);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(size == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", size);

    /* nonexistent property now that product key exists */
    size = 0xdeadbeef;
    r = pMsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, "nonexistent", NULL, &size);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(size == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", size);

    data = "tester";
    res = RegSetValueExA(hkey, "nonexistent", 0, REG_SZ, (const BYTE *)data, lstrlenA(data) + 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* nonexistent property now that nonexistent value exists */
    size = 0xdeadbeef;
    r = pMsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT, "nonexistent", NULL, &size);
    ok(r == ERROR_UNKNOWN_PROPERTY, "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);
    ok(size == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", size);

    /* invalid option now that product key exists */
    size = 0xdeadbeef;
    r = pMsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              4, INSTALLPROPERTY_PACKAGENAME, NULL, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(size == 11, "Expected 11, got %d\n", size);

    /* INSTALLPROPERTY_MEDIAPACKAGEPATH, media key does not exist */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = pMsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                               MSICODE_PRODUCT, INSTALLPROPERTY_MEDIAPACKAGEPATH,
                               value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, ""), "Expected \"\", got \"%s\"\n", value);
    ok(size == 0, "Expected 0, got %d\n", size);

    res = RegCreateKeyA(hkey, "Media", &media);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    data = "path";
    res = RegSetValueExA(media, "MediaPackage", 0, REG_SZ,
                         (const BYTE *)data, lstrlenA(data) + 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* INSTALLPROPERTY_MEDIAPACKAGEPATH */
    size = MAX_PATH;
    r = pMsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                               MSICODE_PRODUCT, INSTALLPROPERTY_MEDIAPACKAGEPATH,
                               value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, "path"), "Expected \"path\", got \"%s\"\n", value);
    ok(size == 4, "Expected 4, got %d\n", size);

    /* INSTALLPROPERTY_DISKPROMPT */
    data = "prompt";
    res = RegSetValueExA(media, "DiskPrompt", 0, REG_SZ,
                         (const BYTE *)data, lstrlenA(data) + 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    size = MAX_PATH;
    r = pMsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                               MSICODE_PRODUCT, INSTALLPROPERTY_DISKPROMPT,
                               value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, "prompt"), "Expected \"prompt\", got \"%s\"\n", value);
    ok(size == 6, "Expected 6, got %d\n", size);

    data = "";
    res = RegSetValueExA(hkey, "LastUsedSource", 0, REG_SZ,
                         (const BYTE *)data, lstrlenA(data) + 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* INSTALLPROPERTY_LASTUSEDSOURCE, source is empty */
    size = MAX_PATH;
    r = pMsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                               MSICODE_PRODUCT, INSTALLPROPERTY_LASTUSEDSOURCE,
                               value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, ""), "Expected \"\", got \"%s\"\n", value);
    ok(size == 0, "Expected 0, got %d\n", size);

    data = "source";
    res = RegSetValueExA(hkey, "LastUsedSource", 0, REG_SZ,
                         (const BYTE *)data, lstrlenA(data) + 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* INSTALLPROPERTY_LASTUSEDSOURCE */
    size = MAX_PATH;
    r = pMsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                               MSICODE_PRODUCT, INSTALLPROPERTY_LASTUSEDSOURCE,
                               value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, "source"), "Expected \"source\", got \"%s\"\n", value);
    ok(size == 6, "Expected 6, got %d\n", size);

    /* INSTALLPROPERTY_LASTUSEDSOURCE, size is too short */
    size = 4;
    lstrcpyA(value, "aaa");
    r = pMsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                               MSICODE_PRODUCT, INSTALLPROPERTY_LASTUSEDSOURCE,
                               value, &size);
    ok(r == ERROR_MORE_DATA, "Expected ERROR_MORE_DATA, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got \"%s\"\n", value);
    ok(size == 6, "Expected 6, got %d\n", size);

    /* INSTALLPROPERTY_LASTUSEDSOURCE, size is exactly 6 */
    size = 6;
    lstrcpyA(value, "aaa");
    r = pMsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                               MSICODE_PRODUCT, INSTALLPROPERTY_LASTUSEDSOURCE,
                               value, &size);
    ok(r == ERROR_MORE_DATA, "Expected ERROR_MORE_DATA, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got \"%s\"\n", value);
    ok(size == 6, "Expected 6, got %d\n", size);

    data = "a;source";
    res = RegSetValueExA(hkey, "LastUsedSource", 0, REG_SZ,
                         (const BYTE *)data, lstrlenA(data) + 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* INSTALLPROPERTY_LASTUSEDSOURCE, one semi-colon */
    size = MAX_PATH;
    r = pMsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                               MSICODE_PRODUCT, INSTALLPROPERTY_LASTUSEDSOURCE,
                               value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, "source"), "Expected \"source\", got \"%s\"\n", value);
    ok(size == 6, "Expected 6, got %d\n", size);

    data = "a:source";
    res = RegSetValueExA(hkey, "LastUsedSource", 0, REG_SZ,
                         (const BYTE *)data, lstrlenA(data) + 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* INSTALLPROPERTY_LASTUSEDSOURCE, one colon */
    size = MAX_PATH;
    r = pMsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                               MSICODE_PRODUCT, INSTALLPROPERTY_LASTUSEDSOURCE,
                               value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, "a:source"), "Expected \"a:source\", got \"%s\"\n", value);
    ok(size == 8, "Expected 8, got %d\n", size);

    /* INSTALLPROPERTY_LASTUSEDTYPE, invalid source format */
    size = MAX_PATH;
    r = pMsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                               MSICODE_PRODUCT, INSTALLPROPERTY_LASTUSEDTYPE,
                               value, &size);
    todo_wine
    {
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
        ok(!lstrcmpA(value, ""), "Expected \"\", got \"%s\"\n", value);
        ok(size == 0, "Expected 0, got %d\n", size);
    }

    data = "x;y;z";
    res = RegSetValueExA(hkey, "LastUsedSource", 0, REG_SZ,
                         (const BYTE *)data, lstrlenA(data) + 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* INSTALLPROPERTY_LASTUSEDTYPE, invalid source format */
    size = MAX_PATH;
    r = pMsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                               MSICODE_PRODUCT, INSTALLPROPERTY_LASTUSEDTYPE,
                               value, &size);
    todo_wine
    {
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
        ok(!lstrcmpA(value, ""), "Expected \"\", got \"%s\"\n", value);
        ok(size == 0, "Expected 0, got %d\n", size);
    }

    data = "n;y;z";
    res = RegSetValueExA(hkey, "LastUsedSource", 0, REG_SZ,
                         (const BYTE *)data, lstrlenA(data) + 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* INSTALLPROPERTY_LASTUSEDTYPE */
    size = MAX_PATH;
    r = pMsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                               MSICODE_PRODUCT, INSTALLPROPERTY_LASTUSEDTYPE,
                               value, &size);
    todo_wine
    {
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
        ok(!lstrcmpA(value, "n"), "Expected \"n\", got \"%s\"\n", value);
        ok(size == 1, "Expected 1, got %d\n", size);
    }

    data = "negatory";
    res = RegSetValueExA(hkey, "LastUsedSource", 0, REG_SZ,
                         (const BYTE *)data, lstrlenA(data) + 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* INSTALLPROPERTY_LASTUSEDTYPE */
    size = MAX_PATH;
    r = pMsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                               MSICODE_PRODUCT, INSTALLPROPERTY_LASTUSEDTYPE,
                               value, &size);
    todo_wine
    {
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
        ok(!lstrcmpA(value, "n"), "Expected \"n\", got \"%s\"\n", value);
        ok(size == 1, "Expected 1, got %d\n", size);
    }

    data = "megatron";
    res = RegSetValueExA(hkey, "LastUsedSource", 0, REG_SZ,
                         (const BYTE *)data, lstrlenA(data) + 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* INSTALLPROPERTY_LASTUSEDTYPE */
    size = MAX_PATH;
    r = pMsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                               MSICODE_PRODUCT, INSTALLPROPERTY_LASTUSEDTYPE,
                               value, &size);
    todo_wine
    {
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
        ok(!lstrcmpA(value, "m"), "Expected \"m\", got \"%s\"\n", value);
        ok(size == 1, "Expected 1, got %d\n", size);
    }

    data = "useless";
    res = RegSetValueExA(hkey, "LastUsedSource", 0, REG_SZ,
                         (const BYTE *)data, lstrlenA(data) + 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* INSTALLPROPERTY_LASTUSEDTYPE */
    size = MAX_PATH;
    r = pMsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                               MSICODE_PRODUCT, INSTALLPROPERTY_LASTUSEDTYPE,
                               value, &size);
    todo_wine
    {
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
        ok(!lstrcmpA(value, "u"), "Expected \"u\", got \"%s\"\n", value);
        ok(size == 1, "Expected 1, got %d\n", size);
    }

    RegDeleteValueA(media, "MediaPackage");
    RegDeleteValueA(media, "DiskPrompt");
    RegDeleteKeyA(media, "");
    RegDeleteValueA(hkey, "LastUsedSource");
    RegDeleteValueA(hkey, "nonexistent");
    RegDeleteValueA(hkey, "PackageName");
    RegDeleteKeyA(hkey, "");
    RegDeleteKeyA(userkey, "");
    RegCloseKey(hkey);
    RegCloseKey(userkey);

    /* try a patch */
    size = 0xdeadbeef;
    r = pMsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PATCH, INSTALLPROPERTY_PACKAGENAME, NULL, &size);
    ok(r == ERROR_UNKNOWN_PATCH, "Expected ERROR_UNKNOWN_PATCH, got %d\n", r);
    ok(size == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", size);

    lstrcpyA(keypath, "Software\\Microsoft\\Installer\\Patches\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_CURRENT_USER, keypath, &userkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* patch key exists
     * NOTE: using prodcode guid, but it really doesn't matter
     */
    size = 0xdeadbeef;
    r = pMsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PATCH, INSTALLPROPERTY_PACKAGENAME, NULL, &size);
    ok(r == ERROR_BAD_CONFIGURATION, "Expected ERROR_BAD_CONFIGURATION, got %d\n", r);
    ok(size == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", size);

    res = RegCreateKeyA(userkey, "SourceList", &hkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* SourceList key exists */
    size = 0xdeadbeef;
    r = pMsiSourceListGetInfoA(prodcode, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PATCH, INSTALLPROPERTY_PACKAGENAME, NULL, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(size == 0, "Expected 0, got %d\n", size);

    RegDeleteKeyA(hkey, "");
    RegDeleteKeyA(userkey, "");
    RegCloseKey(hkey);
    RegCloseKey(userkey);
}

static void test_MsiSourceListAddSourceEx(void)
{
    CHAR prodcode[MAX_PATH];
    CHAR prod_squashed[MAX_PATH];
    CHAR keypath[MAX_PATH*2];
    CHAR value[MAX_PATH];
    LPSTR usersid;
    LONG res;
    UINT r;
    HKEY prodkey, userkey, hkey;
    HKEY url, net;
    DWORD size;

    if (!pMsiSourceListAddSourceExA)
    {
        skip("Skipping MsiSourceListAddSourceExA tests\n");
        return;
    }

    create_test_guid(prodcode, prod_squashed);
    get_user_sid(&usersid);

    /* GetLastError is not set by the function */

    /* NULL szProductCodeOrPatchCode */
    r = pMsiSourceListAddSourceExA(NULL, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, "C:\\source", 0);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* empty szProductCodeOrPatchCode */
    r = pMsiSourceListAddSourceExA("", usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, "C:\\source", 0);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* garbage szProductCodeOrPatchCode */
    r = pMsiSourceListAddSourceExA("garbage", usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, "C:\\source", 0);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* guid without brackets */
    r = pMsiSourceListAddSourceExA("51CD2AD5-0482-4C46-8DDD-0ED1022AA1AA", usersid,
                                  MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, "C:\\source", 0);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* guid with brackets */
    r = pMsiSourceListAddSourceExA("{51CD2AD5-0482-4C46-8DDD-0ED1022AA1AA}", usersid,
                                  MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, "C:\\source", 0);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);

    /* MSIINSTALLCONTEXT_USERUNMANAGED */

    r = pMsiSourceListAddSourceExA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, "C:\\source", 0);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);

    lstrcpyA(keypath, "Software\\Microsoft\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_CURRENT_USER, keypath, &userkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* user product key exists */
    r = pMsiSourceListAddSourceExA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, "C:\\source", 0);
    ok(r == ERROR_BAD_CONFIGURATION, "Expected ERROR_BAD_CONFIGURATION, got %d\n", r);

    res = RegCreateKeyA(userkey, "SourceList", &url);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    RegCloseKey(url);

    /* SourceList key exists */
    r = pMsiSourceListAddSourceExA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, "C:\\source", 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    res = RegOpenKeyA(userkey, "SourceList\\URL", &url);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    size = MAX_PATH;
    res = RegQueryValueExA(url, "1", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    ok(!lstrcmpA(value, "C:\\source/"), "Expected 'C:\\source/', got %s\n", value);
    ok(size == 11, "Expected 11, got %d\n", size);

    /* add another source, index 0 */
    r = pMsiSourceListAddSourceExA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, "another", 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    res = RegQueryValueExA(url, "1", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    ok(!lstrcmpA(value, "C:\\source/"), "Expected 'C:\\source/', got %s\n", value);
    ok(size == 11, "Expected 11, got %d\n", size);

    size = MAX_PATH;
    res = RegQueryValueExA(url, "2", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    ok(!lstrcmpA(value, "another/"), "Expected 'another/', got %s\n", value);
    ok(size == 9, "Expected 9, got %d\n", size);

    /* add another source, index 1 */
    r = pMsiSourceListAddSourceExA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, "third/", 1);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    res = RegQueryValueExA(url, "1", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    ok(!lstrcmpA(value, "third/"), "Expected 'third/', got %s\n", value);
    ok(size == 7, "Expected 7, got %d\n", size);

    size = MAX_PATH;
    res = RegQueryValueExA(url, "2", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    ok(!lstrcmpA(value, "C:\\source/"), "Expected 'C:\\source/', got %s\n", value);
    ok(size == 11, "Expected 11, got %d\n", size);

    size = MAX_PATH;
    res = RegQueryValueExA(url, "3", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    ok(!lstrcmpA(value, "another/"), "Expected 'another/', got %s\n", value);
    ok(size == 9, "Expected 9, got %d\n", size);

    /* add another source, index > N */
    r = pMsiSourceListAddSourceExA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, "last/", 5);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    res = RegQueryValueExA(url, "1", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    ok(!lstrcmpA(value, "third/"), "Expected 'third/', got %s\n", value);
    ok(size == 7, "Expected 7, got %d\n", size);

    size = MAX_PATH;
    res = RegQueryValueExA(url, "2", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    ok(!lstrcmpA(value, "C:\\source/"), "Expected 'C:\\source/', got %s\n", value);
    ok(size == 11, "Expected 11, got %d\n", size);

    size = MAX_PATH;
    res = RegQueryValueExA(url, "3", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    ok(!lstrcmpA(value, "another/"), "Expected 'another/', got %s\n", value);
    ok(size == 9, "Expected 9, got %d\n", size);

    size = MAX_PATH;
    res = RegQueryValueExA(url, "4", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    ok(!lstrcmpA(value, "last/"), "Expected 'last/', got %s\n", value);
    ok(size == 6, "Expected 6, got %d\n", size);

    /* just MSISOURCETYPE_NETWORK */
    r = pMsiSourceListAddSourceExA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSISOURCETYPE_NETWORK, "source", 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    res = RegOpenKeyA(userkey, "SourceList\\Net", &net);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    size = MAX_PATH;
    res = RegQueryValueExA(net, "1", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    ok(!lstrcmpA(value, "source\\"), "Expected 'source\\', got %s\n", value);
    ok(size == 8, "Expected 8, got %d\n", size);

    /* just MSISOURCETYPE_URL */
    r = pMsiSourceListAddSourceExA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSISOURCETYPE_URL, "source", 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    res = RegQueryValueExA(url, "1", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    ok(!lstrcmpA(value, "third/"), "Expected 'third/', got %s\n", value);
    ok(size == 7, "Expected 7, got %d\n", size);

    size = MAX_PATH;
    res = RegQueryValueExA(url, "2", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    ok(!lstrcmpA(value, "C:\\source/"), "Expected 'C:\\source/', got %s\n", value);
    ok(size == 11, "Expected 11, got %d\n", size);

    size = MAX_PATH;
    res = RegQueryValueExA(url, "3", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    ok(!lstrcmpA(value, "another/"), "Expected 'another/', got %s\n", value);
    ok(size == 9, "Expected 9, got %d\n", size);

    size = MAX_PATH;
    res = RegQueryValueExA(url, "4", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    ok(!lstrcmpA(value, "last/"), "Expected 'last/', got %s\n", value);
    ok(size == 6, "Expected 6, got %d\n", size);

    size = MAX_PATH;
    res = RegQueryValueExA(url, "5", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    ok(!lstrcmpA(value, "source/"), "Expected 'source/', got %s\n", value);
    ok(size == 8, "Expected 8, got %d\n", size);

    /* NULL szUserSid */
    r = pMsiSourceListAddSourceExA(prodcode, NULL,
                                  MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSISOURCETYPE_NETWORK, "nousersid", 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    res = RegQueryValueExA(net, "1", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    ok(!lstrcmpA(value, "source\\"), "Expected 'source\\', got %s\n", value);
    ok(size == 8, "Expected 8, got %d\n", size);

    size = MAX_PATH;
    res = RegQueryValueExA(net, "2", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    ok(!lstrcmpA(value, "nousersid\\"), "Expected 'nousersid\\', got %s\n", value);
    ok(size == 11, "Expected 11, got %d\n", size);

    /* invalid options, must have source type */
    r = pMsiSourceListAddSourceExA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT, "source", 0);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    r = pMsiSourceListAddSourceExA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PATCH, "source", 0);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* NULL szSource */
    r = pMsiSourceListAddSourceExA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSISOURCETYPE_URL, NULL, 1);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* empty szSource */
    r = pMsiSourceListAddSourceExA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSISOURCETYPE_URL, "", 1);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* MSIINSTALLCONTEXT_USERMANAGED, non-NULL szUserSid */

    r = pMsiSourceListAddSourceExA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_USERMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, "C:\\source", 0);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Managed\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &prodkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* product key exists */
    r = pMsiSourceListAddSourceExA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_USERMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, "C:\\source", 0);
    ok(r == ERROR_BAD_CONFIGURATION, "Expected ERROR_BAD_CONFIGURATION, got %d\n", r);

    res = RegCreateKeyA(prodkey, "SourceList", &hkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    RegCloseKey(hkey);

    /* SourceList exists */
    r = pMsiSourceListAddSourceExA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_USERMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, "C:\\source", 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    res = RegOpenKeyA(prodkey, "SourceList\\URL", &url);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    size = MAX_PATH;
    res = RegQueryValueExA(url, "1", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    ok(!lstrcmpA(value, "C:\\source/"), "Expected 'C:\\source/', got %s\n", value);
    ok(size == 11, "Expected 11, got %d\n", size);

    RegCloseKey(url);

    /* MSIINSTALLCONTEXT_USERMANAGED, NULL szUserSid */

    r = pMsiSourceListAddSourceExA(prodcode, NULL,
                                  MSIINSTALLCONTEXT_USERMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, "another", 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    res = RegOpenKeyA(prodkey, "SourceList\\URL", &url);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    size = MAX_PATH;
    res = RegQueryValueExA(url, "1", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    ok(!lstrcmpA(value, "C:\\source/"), "Expected 'C:\\source/', got %s\n", value);
    ok(size == 11, "Expected 11, got %d\n", size);

    size = MAX_PATH;
    res = RegQueryValueExA(url, "2", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    ok(!lstrcmpA(value, "another/"), "Expected 'another/', got %s\n", value);
    ok(size == 9, "Expected 9, got %d\n", size);

    RegCloseKey(url);
    RegCloseKey(prodkey);

    /* MSIINSTALLCONTEXT_MACHINE */

    /* szUserSid must be NULL for MSIINSTALLCONTEXT_MACHINE */
    r = pMsiSourceListAddSourceExA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_MACHINE,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, "C:\\source", 0);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    r = pMsiSourceListAddSourceExA(prodcode, NULL,
                                  MSIINSTALLCONTEXT_MACHINE,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, "C:\\source", 0);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    lstrcpyA(keypath, "Software\\Classes\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &prodkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* product key exists */
    r = pMsiSourceListAddSourceExA(prodcode, NULL,
                                  MSIINSTALLCONTEXT_MACHINE,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, "C:\\source", 0);
    ok(r == ERROR_BAD_CONFIGURATION, "Expected ERROR_BAD_CONFIGURATION, got %d\n", r);

    res = RegCreateKeyA(prodkey, "SourceList", &hkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    RegCloseKey(hkey);

    /* SourceList exists */
    r = pMsiSourceListAddSourceExA(prodcode, NULL,
                                  MSIINSTALLCONTEXT_MACHINE,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, "C:\\source", 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    res = RegOpenKeyA(prodkey, "SourceList\\URL", &url);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    size = MAX_PATH;
    res = RegQueryValueExA(url, "1", NULL, NULL, (LPBYTE)value, &size);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    ok(!lstrcmpA(value, "C:\\source/"), "Expected 'C:\\source/', got %s\n", value);
    ok(size == 11, "Expected 11, got %d\n", size);

    RegCloseKey(url);
    RegCloseKey(prodkey);
    HeapFree(GetProcessHeap(), 0, usersid);
}

static void test_MsiSourceListEnumSources(void)
{
    CHAR prodcode[MAX_PATH];
    CHAR prod_squashed[MAX_PATH];
    CHAR keypath[MAX_PATH*2];
    CHAR value[MAX_PATH];
    LPSTR usersid;
    LONG res;
    UINT r;
    HKEY prodkey, userkey;
    HKEY url, net, source;
    DWORD size;

    create_test_guid(prodcode, prod_squashed);
    get_user_sid(&usersid);

    /* GetLastError is not set by the function */

    /* NULL szProductCodeOrPatchCode */
    size = 0xdeadbeef;
    r = MsiSourceListEnumSourcesA(NULL, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_NETWORK, 0, value, &size);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(size == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", size);

    /* empty szProductCodeOrPatchCode */
    size = 0xdeadbeef;
    r = MsiSourceListEnumSourcesA("", usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_NETWORK, 0, value, &size);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(size == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", size);

    /* garbage szProductCodeOrPatchCode */
    size = 0xdeadbeef;
    r = MsiSourceListEnumSourcesA("garbage", usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_NETWORK, 0, value, &size);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(size == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", size);

    /* guid without brackets */
    size = 0xdeadbeef;
    r = MsiSourceListEnumSourcesA("51CD2AD5-0482-4C46-8DDD-0ED1022AA1AA",
                                  usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_NETWORK, 0, value, &size);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(size == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", size);

    /* guid with brackets */
    size = 0xdeadbeef;
    r = MsiSourceListEnumSourcesA("{51CD2AD5-0482-4C46-8DDD-0ED1022AA1AA}",
                                  usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_NETWORK, 0, value, &size);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(size == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", size);

    /* MSIINSTALLCONTEXT_USERUNMANAGED */

    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 0, value, &size);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got %s\n", value);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %d\n", size);

    lstrcpyA(keypath, "Software\\Microsoft\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_CURRENT_USER, keypath, &userkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* user product key exists */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 0, value, &size);
    ok(r == ERROR_BAD_CONFIGURATION, "Expected ERROR_BAD_CONFIGURATION, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got %s\n", value);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %d\n", size);

    res = RegCreateKeyA(userkey, "SourceList", &source);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* SourceList key exists */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 0, value, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got %s\n", value);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %d\n", size);

    res = RegCreateKeyA(source, "URL", &url);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* URL key exists */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 0, value, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got %s\n", value);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %d\n", size);

    res = RegSetValueExA(url, "1", 0, REG_SZ, (LPBYTE)"first", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    res = RegSetValueExA(url, "2", 0, REG_SZ, (LPBYTE)"second", 7);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    res = RegSetValueExA(url, "4", 0, REG_SZ, (LPBYTE)"fourth", 7);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* sources exist */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 0, value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, "first"), "Expected \"first\", got %s\n", value);
    ok(size == 5, "Expected 5, got %d\n", size);

    /* try index 0 again */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 0, value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, "first"), "Expected \"first\", got %s\n", value);
    ok(size == 5, "Expected 5, got %d\n", size);

    /* both szSource and pcchSource are NULL, index 0 */
    r = MsiSourceListEnumSourcesA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 0, NULL, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* both szSource and pcchSource are NULL, index 1 */
    r = MsiSourceListEnumSourcesA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 1, NULL, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* size is exactly 5 */
    size = 5;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 0, value, &size);
    ok(r == ERROR_MORE_DATA, "Expected ERROR_MORE_DATA, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected \"aaa\", got %s\n", value);
    ok(size == 5, "Expected 5, got %d\n", size);

    /* szSource is non-NULL while pcchSource is NULL */
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 0, value, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected \"aaa\", got %s\n", value);

    /* try index 1 after failure */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 1, value, &size);
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected \"aaa\", got %s\n", value);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %d\n", size);

    /* reset the enumeration */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 0, value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, "first"), "Expected \"first\", got %s\n", value);
    ok(size == 5, "Expected 5, got %d\n", size);

    /* try index 1 */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 1, value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, "second"), "Expected \"second\", got %s\n", value);
    ok(size == 6, "Expected 6, got %d\n", size);

    /* try index 1 again */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 1, value, &size);
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got %s\n", value);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %d\n", size);

    /* try index 2 */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 2, value, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got %s\n", value);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %d\n", size);

    /* try index < 0 */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, -1, value, &size);
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got %s\n", value);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %d\n", size);

    /* NULL szUserSid */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, NULL,
                                  MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 0, value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, "first"), "Expected \"first\", got %s\n", value);
    ok(size == 5, "Expected 5, got %d\n", size);

    /* invalid dwOptions, must be one of MSICODE_ and MSISOURCETYPE_ */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, NULL,
                                  MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT, 0, value, &size);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got %s\n", value);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %d\n", size);

    /* invalid dwOptions, must be one of MSICODE_ and MSISOURCETYPE_ */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, NULL,
                                  MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PATCH, 0, value, &size);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got %s\n", value);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %d\n", size);

    /* invalid dwOptions, must be one of MSICODE_ and MSISOURCETYPE_ */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, NULL,
                                  MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSICODE_PATCH | MSISOURCETYPE_URL,
                                  0, value, &size);
    ok(r == ERROR_UNKNOWN_PATCH, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got %s\n", value);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %d\n", size);

    /* invalid dwOptions, must be one of MSICODE_ and MSISOURCETYPE_ */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, NULL,
                                  MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_NETWORK | MSISOURCETYPE_URL,
                                  0, value, &size);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got %s\n", value);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %d\n", size);

    RegDeleteValueA(url, "1");
    RegDeleteValueA(url, "2");
    RegDeleteValueA(url, "4");
    RegDeleteKeyA(url, "");
    RegCloseKey(url);

    /* SourceList key exists */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_NETWORK, 0, value, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got %s\n", value);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %d\n", size);

    res = RegCreateKeyA(source, "Net", &net);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* Net key exists */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_NETWORK, 0, value, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got %s\n", value);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %d\n", size);

    res = RegSetValueExA(net, "1", 0, REG_SZ, (LPBYTE)"first", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* sources exist */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_USERUNMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_NETWORK, 0, value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, "first"), "Expected \"first\", got %s\n", value);
    ok(size == 5, "Expected 5, got %d\n", size);

    RegDeleteValueA(net, "1");
    RegDeleteKeyA(net, "");
    RegCloseKey(net);
    RegDeleteKeyA(source, "");
    RegCloseKey(source);
    RegDeleteKeyA(userkey, "");
    RegCloseKey(userkey);

    /* MSIINSTALLCONTEXT_USERMANAGED */

    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_USERMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 0, value, &size);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got %s\n", value);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %d\n", size);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Managed\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &userkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* user product key exists */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_USERMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 0, value, &size);
    ok(r == ERROR_BAD_CONFIGURATION, "Expected ERROR_BAD_CONFIGURATION, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got %s\n", value);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %d\n", size);

    res = RegCreateKeyA(userkey, "SourceList", &source);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* SourceList key exists */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_USERMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 0, value, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got %s\n", value);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %d\n", size);

    res = RegCreateKeyA(source, "URL", &url);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* URL key exists */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_USERMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 0, value, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got %s\n", value);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %d\n", size);

    res = RegSetValueExA(url, "1", 0, REG_SZ, (LPBYTE)"first", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* sources exist */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_USERMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 0, value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, "first"), "Expected \"first\", got %s\n", value);
    ok(size == 5, "Expected 5, got %d\n", size);

    /* NULL szUserSid */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, NULL,
                                  MSIINSTALLCONTEXT_USERMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 0, value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, "first"), "Expected \"first\", got %s\n", value);
    ok(size == 5, "Expected 5, got %d\n", size);

    RegDeleteValueA(url, "1");
    RegDeleteKeyA(url, "");
    RegCloseKey(url);

    /* SourceList key exists */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_USERMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_NETWORK, 0, value, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got %s\n", value);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %d\n", size);

    res = RegCreateKeyA(source, "Net", &net);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* Net key exists */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_USERMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_NETWORK, 0, value, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got %s\n", value);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %d\n", size);

    res = RegSetValueExA(net, "1", 0, REG_SZ, (LPBYTE)"first", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* sources exist */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_USERMANAGED,
                                  MSICODE_PRODUCT | MSISOURCETYPE_NETWORK, 0, value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, "first"), "Expected \"first\", got %s\n", value);
    ok(size == 5, "Expected 5, got %d\n", size);

    RegDeleteValueA(net, "1");
    RegDeleteKeyA(net, "");
    RegCloseKey(net);
    RegDeleteKeyA(source, "");
    RegCloseKey(source);
    RegDeleteKeyA(userkey, "");
    RegCloseKey(userkey);

    /* MSIINSTALLCONTEXT_MACHINE */

    /* szUserSid is non-NULL */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, usersid,
                                  MSIINSTALLCONTEXT_MACHINE,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 0, value, &size);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got %s\n", value);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %d\n", size);

    /* szUserSid is non-NULL */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, NULL,
                                  MSIINSTALLCONTEXT_MACHINE,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 0, value, &size);
    ok(r == ERROR_UNKNOWN_PRODUCT, "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got %s\n", value);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %d\n", size);

    lstrcpyA(keypath, "Software\\Classes\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &prodkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* user product key exists */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, NULL,
                                  MSIINSTALLCONTEXT_MACHINE,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 0, value, &size);
    ok(r == ERROR_BAD_CONFIGURATION, "Expected ERROR_BAD_CONFIGURATION, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got %s\n", value);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %d\n", size);

    res = RegCreateKeyA(prodkey, "SourceList", &source);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* SourceList key exists */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, NULL,
                                  MSIINSTALLCONTEXT_MACHINE,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 0, value, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got %s\n", value);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %d\n", size);

    res = RegCreateKeyA(source, "URL", &url);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* URL key exists */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, NULL,
                                  MSIINSTALLCONTEXT_MACHINE,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 0, value, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got %s\n", value);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %d\n", size);

    res = RegSetValueExA(url, "1", 0, REG_SZ, (LPBYTE)"first", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* sources exist */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, NULL,
                                  MSIINSTALLCONTEXT_MACHINE,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 0, value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, "first"), "Expected \"first\", got %s\n", value);
    ok(size == 5, "Expected 5, got %d\n", size);

    /* NULL szUserSid */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, NULL,
                                  MSIINSTALLCONTEXT_MACHINE,
                                  MSICODE_PRODUCT | MSISOURCETYPE_URL, 0, value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, "first"), "Expected \"first\", got %s\n", value);
    ok(size == 5, "Expected 5, got %d\n", size);

    RegDeleteValueA(url, "1");
    RegDeleteKeyA(url, "");
    RegCloseKey(url);

    /* SourceList key exists */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, NULL,
                                  MSIINSTALLCONTEXT_MACHINE,
                                  MSICODE_PRODUCT | MSISOURCETYPE_NETWORK, 0, value, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got %s\n", value);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %d\n", size);

    res = RegCreateKeyA(source, "Net", &net);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* Net key exists */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, NULL,
                                  MSIINSTALLCONTEXT_MACHINE,
                                  MSICODE_PRODUCT | MSISOURCETYPE_NETWORK, 0, value, &size);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    ok(!lstrcmpA(value, "aaa"), "Expected value to be unchanged, got %s\n", value);
    ok(size == MAX_PATH, "Expected MAX_PATH, got %d\n", size);

    res = RegSetValueExA(net, "1", 0, REG_SZ, (LPBYTE)"first", 6);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* sources exist */
    size = MAX_PATH;
    lstrcpyA(value, "aaa");
    r = MsiSourceListEnumSourcesA(prodcode, NULL,
                                  MSIINSTALLCONTEXT_MACHINE,
                                  MSICODE_PRODUCT | MSISOURCETYPE_NETWORK, 0, value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(value, "first"), "Expected \"first\", got %s\n", value);
    ok(size == 5, "Expected 5, got %d\n", size);

    RegDeleteValueA(net, "1");
    RegDeleteKeyA(net, "");
    RegCloseKey(net);
    RegDeleteKeyA(source, "");
    RegCloseKey(source);
    RegDeleteKeyA(prodkey, "");
    RegCloseKey(prodkey);
}

static void test_MsiSourceListSetInfo(void)
{
    CHAR prodcode[MAX_PATH];
    CHAR prod_squashed[MAX_PATH];
    CHAR keypath[MAX_PATH*2];
    HKEY prodkey, userkey;
    HKEY net, url, media, source;
    LPSTR usersid;
    LONG res;
    UINT r;

    create_test_guid(prodcode, prod_squashed);
    get_user_sid(&usersid);

    /* GetLastError is not set by the function */

    /* NULL szProductCodeOrPatchCode */
    r = MsiSourceListSetInfoA(NULL, usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT | MSISOURCETYPE_NETWORK,
                              INSTALLPROPERTY_MEDIAPACKAGEPATH, "path");
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* empty szProductCodeOrPatchCode */
    r = MsiSourceListSetInfoA("", usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT | MSISOURCETYPE_NETWORK,
                              INSTALLPROPERTY_MEDIAPACKAGEPATH, "path");
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* garbage szProductCodeOrPatchCode */
    r = MsiSourceListSetInfoA("garbage", usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT | MSISOURCETYPE_NETWORK,
                              INSTALLPROPERTY_MEDIAPACKAGEPATH, "path");
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* guid without brackets */
    r = MsiSourceListSetInfoA("51CD2AD5-0482-4C46-8DDD-0ED1022AA1AA",
                              usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT | MSISOURCETYPE_NETWORK,
                              INSTALLPROPERTY_MEDIAPACKAGEPATH, "path");
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* guid with brackets */
    r = MsiSourceListSetInfoA("{51CD2AD5-0482-4C46-8DDD-0ED1022AA1AA}",
                              usersid, MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT | MSISOURCETYPE_NETWORK,
                              INSTALLPROPERTY_MEDIAPACKAGEPATH, "path");
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);

    /* dwOptions is MSICODE_PRODUCT */
    r = MsiSourceListSetInfoA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED, MSICODE_PRODUCT,
                              INSTALLPROPERTY_MEDIAPACKAGEPATH, "path");
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);

    /* dwOptions is MSICODE_PATCH */
    r = MsiSourceListSetInfoA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED, MSICODE_PATCH,
                              INSTALLPROPERTY_MEDIAPACKAGEPATH, "path");
    ok(r == ERROR_UNKNOWN_PATCH, "Expected ERROR_UNKNOWN_PATCH, got %d\n", r);

    /* dwOptions is both MSICODE_PRODUCT and MSICODE_PATCH */
    r = MsiSourceListSetInfoA(prodcode, usersid,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT | MSICODE_PATCH | MSISOURCETYPE_URL,
                              INSTALLPROPERTY_MEDIAPACKAGEPATH, "path");
    ok(r == ERROR_UNKNOWN_PATCH, "Expected ERROR_UNKNOWN_PATCH, got %d\n", r);

    /* dwOptions has both MSISOURCETYPE_NETWORK and MSISOURCETYPE_URL */
    r = MsiSourceListSetInfoA(prodcode, NULL,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT | MSISOURCETYPE_NETWORK | MSISOURCETYPE_URL,
                              INSTALLPROPERTY_MEDIAPACKAGEPATH, "path");
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);

    /* LastUsedSource and dwOptions has both
     * MSISOURCETYPE_NETWORK and MSISOURCETYPE_URL
     */
    r = MsiSourceListSetInfoA(prodcode, NULL,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT | MSISOURCETYPE_NETWORK | MSISOURCETYPE_URL,
                              INSTALLPROPERTY_LASTUSEDSOURCE, "path");
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);

    /* LastUsedSource and dwOptions has no source type */
    r = MsiSourceListSetInfoA(prodcode, NULL,
                              MSIINSTALLCONTEXT_USERUNMANAGED, MSICODE_PRODUCT,
                              INSTALLPROPERTY_LASTUSEDSOURCE, "path");
    ok(r == ERROR_UNKNOWN_PRODUCT,
       "Expected ERROR_UNKNOWN_PRODUCT, got %d\n", r);

    /* MSIINSTALLCONTEXT_USERUNMANAGED */

    lstrcpyA(keypath, "Software\\Microsoft\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_CURRENT_USER, keypath, &userkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* user product key exists */
    r = MsiSourceListSetInfoA(prodcode, NULL,
                              MSIINSTALLCONTEXT_USERUNMANAGED, MSICODE_PRODUCT,
                              INSTALLPROPERTY_MEDIAPACKAGEPATH, "path");
    ok(r == ERROR_BAD_CONFIGURATION,
       "Expected ERROR_BAD_CONFIGURATION, got %d\n", r);

    res = RegCreateKeyA(userkey, "SourceList", &source);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* SourceList key exists, no source type */
    r = MsiSourceListSetInfoA(prodcode, NULL,
                              MSIINSTALLCONTEXT_USERUNMANAGED, MSICODE_PRODUCT,
                              INSTALLPROPERTY_MEDIAPACKAGEPATH, "path");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* Media key is created by MsiSourceListSetInfo */
    res = RegOpenKeyA(source, "Media", &media);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    CHECK_REG_STR(media, "MediaPackage", "path");

    /* set the info again */
    r = MsiSourceListSetInfoA(prodcode, NULL,
                              MSIINSTALLCONTEXT_USERUNMANAGED, MSICODE_PRODUCT,
                              INSTALLPROPERTY_MEDIAPACKAGEPATH, "path2");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    CHECK_REG_STR(media, "MediaPackage", "path2");

    /* NULL szProperty */
    r = MsiSourceListSetInfoA(prodcode, NULL,
                              MSIINSTALLCONTEXT_USERUNMANAGED, MSICODE_PRODUCT,
                              NULL, "path");
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* empty szProperty */
    r = MsiSourceListSetInfoA(prodcode, NULL,
                              MSIINSTALLCONTEXT_USERUNMANAGED, MSICODE_PRODUCT,
                              "", "path");
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);

    /* NULL szValue */
    r = MsiSourceListSetInfoA(prodcode, NULL,
                              MSIINSTALLCONTEXT_USERUNMANAGED, MSICODE_PRODUCT,
                              INSTALLPROPERTY_MEDIAPACKAGEPATH, NULL);
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);

    /* empty szValue */
    r = MsiSourceListSetInfoA(prodcode, NULL,
                              MSIINSTALLCONTEXT_USERUNMANAGED, MSICODE_PRODUCT,
                              INSTALLPROPERTY_MEDIAPACKAGEPATH, "");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    CHECK_REG_STR(media, "MediaPackage", "");

    /* INSTALLPROPERTY_MEDIAPACKAGEPATH, MSISOURCETYPE_NETWORK */
    r = MsiSourceListSetInfoA(prodcode, NULL,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT | MSISOURCETYPE_NETWORK,
                              INSTALLPROPERTY_MEDIAPACKAGEPATH, "path");
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* INSTALLPROPERTY_MEDIAPACKAGEPATH, MSISOURCETYPE_URL */
    r = MsiSourceListSetInfoA(prodcode, NULL,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT | MSISOURCETYPE_URL,
                              INSTALLPROPERTY_MEDIAPACKAGEPATH, "path");
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* INSTALLPROPERTY_DISKPROMPT */
    r = MsiSourceListSetInfoA(prodcode, NULL,
                              MSIINSTALLCONTEXT_USERUNMANAGED, MSICODE_PRODUCT,
                              INSTALLPROPERTY_DISKPROMPT, "prompt");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    CHECK_REG_STR(media, "DiskPrompt", "prompt");

    /* INSTALLPROPERTY_DISKPROMPT, MSISOURCETYPE_NETWORK */
    r = MsiSourceListSetInfoA(prodcode, NULL,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT | MSISOURCETYPE_NETWORK,
                              INSTALLPROPERTY_DISKPROMPT, "prompt");
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* INSTALLPROPERTY_DISKPROMPT, MSISOURCETYPE_URL */
    r = MsiSourceListSetInfoA(prodcode, NULL,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT | MSISOURCETYPE_URL,
                              INSTALLPROPERTY_DISKPROMPT, "prompt");
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* INSTALLPROPERTY_LASTUSEDSOURCE */
    r = MsiSourceListSetInfoA(prodcode, NULL,
                              MSIINSTALLCONTEXT_USERUNMANAGED, MSICODE_PRODUCT,
                              INSTALLPROPERTY_LASTUSEDSOURCE, "source");
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* INSTALLPROPERTY_LASTUSEDSOURCE, MSISOURCETYPE_NETWORK */
    r = MsiSourceListSetInfoA(prodcode, NULL,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT | MSISOURCETYPE_NETWORK,
                              INSTALLPROPERTY_LASTUSEDSOURCE, "source");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* Net key is created by MsiSourceListSetInfo */
    res = RegOpenKeyA(source, "Net", &net);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    CHECK_REG_STR(net, "1", "source\\")
    CHECK_REG_STR(source, "LastUsedSource", "n;1;source");

    /* source has forward slash */
    r = MsiSourceListSetInfoA(prodcode, NULL,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT | MSISOURCETYPE_NETWORK,
                              INSTALLPROPERTY_LASTUSEDSOURCE, "source/");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    CHECK_REG_STR(net, "1", "source\\");
    CHECK_REG_STR(net, "2", "source/\\");
    CHECK_REG_STR(source, "LastUsedSource", "n;2;source/");

    /* INSTALLPROPERTY_LASTUSEDSOURCE, MSISOURCETYPE_URL */
    r = MsiSourceListSetInfoA(prodcode, NULL,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT | MSISOURCETYPE_URL,
                              INSTALLPROPERTY_LASTUSEDSOURCE, "source");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* URL key is created by MsiSourceListSetInfo */
    res = RegOpenKeyA(source, "URL", &url);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    CHECK_REG_STR(url, "1", "source/");
    CHECK_REG_STR(source, "LastUsedSource", "u;1;source");

    /* source has backslash */
    r = MsiSourceListSetInfoA(prodcode, NULL,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT | MSISOURCETYPE_URL,
                              INSTALLPROPERTY_LASTUSEDSOURCE, "source\\");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    CHECK_REG_STR(url, "1", "source/");
    CHECK_REG_STR(url, "2", "source\\/");
    CHECK_REG_STR(source, "LastUsedSource", "u;2;source\\");

    /* INSTALLPROPERTY_LASTUSEDSOURCE, MSISOURCETYPE_MEDIA */
    r = MsiSourceListSetInfoA(prodcode, NULL,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT | MSISOURCETYPE_MEDIA,
                              INSTALLPROPERTY_LASTUSEDSOURCE, "source");
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* INSTALLPROPERTY_PACKAGENAME */
    r = MsiSourceListSetInfoA(prodcode, NULL,
                              MSIINSTALLCONTEXT_USERUNMANAGED, MSICODE_PRODUCT,
                              INSTALLPROPERTY_PACKAGENAME, "name");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    CHECK_REG_STR(source, "PackageName", "name");

    /* INSTALLPROPERTY_PACKAGENAME, MSISOURCETYPE_NETWORK */
    r = MsiSourceListSetInfoA(prodcode, NULL,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT | MSISOURCETYPE_NETWORK,
                              INSTALLPROPERTY_PACKAGENAME, "name");
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* INSTALLPROPERTY_PACKAGENAME, MSISOURCETYPE_URL */
    r = MsiSourceListSetInfoA(prodcode, NULL,
                              MSIINSTALLCONTEXT_USERUNMANAGED,
                              MSICODE_PRODUCT | MSISOURCETYPE_URL,
                              INSTALLPROPERTY_PACKAGENAME, "name");
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* INSTALLPROPERTY_LASTUSEDTYPE */
    r = MsiSourceListSetInfoA(prodcode, NULL,
                              MSIINSTALLCONTEXT_USERUNMANAGED, MSICODE_PRODUCT,
                              INSTALLPROPERTY_LASTUSEDTYPE, "type");
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);

    /* definitely unknown property */
    r = MsiSourceListSetInfoA(prodcode, NULL,
                              MSIINSTALLCONTEXT_USERUNMANAGED, MSICODE_PRODUCT,
                              "unknown", "val");
    ok(r == ERROR_UNKNOWN_PROPERTY,
       "Expected ERROR_UNKNOWN_PROPERTY, got %d\n", r);

    RegDeleteValueA(net, "1");
    RegDeleteKeyA(net, "");
    RegCloseKey(net);
    RegDeleteValueA(url, "1");
    RegDeleteKeyA(url, "");
    RegCloseKey(url);
    RegDeleteValueA(media, "MediaPackage");
    RegDeleteValueA(media, "DiskPrompt");
    RegDeleteKeyA(media, "");
    RegCloseKey(media);
    RegDeleteValueA(source, "PackageName");
    RegDeleteKeyA(source, "");
    RegCloseKey(source);
    RegDeleteKeyA(userkey, "");
    RegCloseKey(userkey);

    /* MSIINSTALLCONTEXT_USERMANAGED */

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\Managed\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &userkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* user product key exists */
    r = MsiSourceListSetInfoA(prodcode, NULL,
                              MSIINSTALLCONTEXT_USERMANAGED, MSICODE_PRODUCT,
                              INSTALLPROPERTY_MEDIAPACKAGEPATH, "path");
    ok(r == ERROR_BAD_CONFIGURATION,
       "Expected ERROR_BAD_CONFIGURATION, got %d\n", r);

    res = RegCreateKeyA(userkey, "SourceList", &source);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* SourceList key exists, no source type */
    r = MsiSourceListSetInfoA(prodcode, NULL,
                              MSIINSTALLCONTEXT_USERMANAGED, MSICODE_PRODUCT,
                              INSTALLPROPERTY_MEDIAPACKAGEPATH, "path");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* Media key is created by MsiSourceListSetInfo */
    res = RegOpenKeyA(source, "Media", &media);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    CHECK_REG_STR(media, "MediaPackage", "path");

    RegDeleteValueA(media, "MediaPackage");
    RegDeleteKeyA(media, "");
    RegCloseKey(media);
    RegDeleteKeyA(source, "");
    RegCloseKey(source);
    RegDeleteKeyA(userkey, "");
    RegCloseKey(userkey);

    /* MSIINSTALLCONTEXT_MACHINE */

    lstrcpyA(keypath, "Software\\Classes\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &prodkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* user product key exists */
    r = MsiSourceListSetInfoA(prodcode, NULL,
                              MSIINSTALLCONTEXT_MACHINE, MSICODE_PRODUCT,
                              INSTALLPROPERTY_MEDIAPACKAGEPATH, "path");
    ok(r == ERROR_BAD_CONFIGURATION,
       "Expected ERROR_BAD_CONFIGURATION, got %d\n", r);

    res = RegCreateKeyA(prodkey, "SourceList", &source);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* SourceList key exists, no source type */
    r = MsiSourceListSetInfoA(prodcode, NULL,
                              MSIINSTALLCONTEXT_MACHINE, MSICODE_PRODUCT,
                              INSTALLPROPERTY_MEDIAPACKAGEPATH, "path");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* Media key is created by MsiSourceListSetInfo */
    res = RegOpenKeyA(source, "Media", &media);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    CHECK_REG_STR(media, "MediaPackage", "path");

    /* szUserSid is non-NULL */
    r = MsiSourceListSetInfoA(prodcode, usersid,
                              MSIINSTALLCONTEXT_MACHINE, MSICODE_PRODUCT,
                              INSTALLPROPERTY_MEDIAPACKAGEPATH, "path");
    ok(r == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    RegDeleteValueA(media, "MediaPackage");
    RegDeleteKeyA(media, "");
    RegCloseKey(media);
    RegDeleteKeyA(source, "");
    RegCloseKey(source);
    RegDeleteKeyA(prodkey, "");
    RegCloseKey(prodkey);
}

START_TEST(source)
{
    init_functionpointers();

    test_MsiSourceListGetInfo();
    test_MsiSourceListAddSourceEx();
    test_MsiSourceListEnumSources();
    test_MsiSourceListSetInfo();
}
