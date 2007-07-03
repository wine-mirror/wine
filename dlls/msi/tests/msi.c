/*
 * tests for Microsoft Installer functionality
 *
 * Copyright 2005 Mike McCormack for CodeWeavers
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

#include <stdio.h>
#include <windows.h>
#include <msi.h>
#include <msiquery.h>
#include <sddl.h>

#include "wine/test.h"

typedef struct test_MSIFILEHASHINFO {
    ULONG dwFileHashInfoSize;
    ULONG dwData[4];
} test_MSIFILEHASHINFO, *test_PMSIFILEHASHINFO;

typedef INSTALLSTATE (WINAPI *fnMsiUseFeatureExA)(LPCSTR, LPCSTR ,DWORD, DWORD );
fnMsiUseFeatureExA pMsiUseFeatureExA;
typedef UINT (WINAPI *fnMsiOpenPackageExA)(LPCSTR, DWORD, MSIHANDLE*);
fnMsiOpenPackageExA pMsiOpenPackageExA;
typedef UINT (WINAPI *fnMsiOpenPackageExW)(LPCWSTR, DWORD, MSIHANDLE*);
fnMsiOpenPackageExW pMsiOpenPackageExW;
typedef INSTALLSTATE (WINAPI *fnMsiGetComponentPathA)(LPCSTR, LPCSTR, LPSTR, DWORD*);
fnMsiGetComponentPathA pMsiGetComponentPathA;
typedef UINT (WINAPI *fnMsiGetFileHashA)(LPCSTR, DWORD, test_PMSIFILEHASHINFO);
fnMsiGetFileHashA pMsiGetFileHashA;

static void test_usefeature(void)
{
    UINT r;

    if (!pMsiUseFeatureExA)
        return;

    r = MsiQueryFeatureState(NULL,NULL);
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return val\n");

    r = MsiQueryFeatureState("{9085040-6000-11d3-8cfe-0150048383c9}" ,NULL);
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return val\n");

    r = pMsiUseFeatureExA(NULL,NULL,0,0);
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return val\n");

    r = pMsiUseFeatureExA(NULL, "WORDVIEWFiles", -2, 1 );
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return val\n");

    r = pMsiUseFeatureExA("{90850409-6000-11d3-8cfe-0150048383c9}", 
                         NULL, -2, 0 );
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return val\n");

    r = pMsiUseFeatureExA("{9085040-6000-11d3-8cfe-0150048383c9}", 
                         "WORDVIEWFiles", -2, 0 );
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return val\n");

    r = pMsiUseFeatureExA("{0085040-6000-11d3-8cfe-0150048383c9}", 
                         "WORDVIEWFiles", -2, 0 );
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return val\n");

    r = pMsiUseFeatureExA("{90850409-6000-11d3-8cfe-0150048383c9}", 
                         "WORDVIEWFiles", -2, 1 );
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return val\n");
}

static void test_null(void)
{
    MSIHANDLE hpkg;
    UINT r;
    HKEY hkey;
    DWORD dwType, cbData;
    LPBYTE lpData = NULL;

    r = pMsiOpenPackageExW(NULL, 0, &hpkg);
    ok( r == ERROR_INVALID_PARAMETER,"wrong error\n");

    r = MsiQueryProductStateW(NULL);
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return\n");

    r = MsiEnumFeaturesW(NULL,0,NULL,NULL);
    ok( r == ERROR_INVALID_PARAMETER,"wrong error\n");

    r = MsiConfigureFeatureW(NULL, NULL, 0);
    ok( r == ERROR_INVALID_PARAMETER, "wrong error\n");

    r = MsiConfigureFeatureA("{00000000-0000-0000-0000-000000000000}", NULL, 0);
    ok( r == ERROR_INVALID_PARAMETER, "wrong error\n");

    r = MsiConfigureFeatureA("{00000000-0000-0000-0000-000000000000}", "foo", 0);
    ok( r == ERROR_INVALID_PARAMETER, "wrong error %d\n", r);

    r = MsiConfigureFeatureA("{00000000-0000-0000-0000-000000000000}", "foo", INSTALLSTATE_DEFAULT);
    ok( r == ERROR_UNKNOWN_PRODUCT, "wrong error %d\n", r);

    /* make sure empty string to MsiGetProductInfo is not a handle to default registry value, saving and restoring the
     * necessary registry values */

    /* empty product string */
    r = RegOpenKeyA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall", &hkey);
    ok( r == ERROR_SUCCESS, "wrong error %d\n", r);

    r = RegQueryValueExA(hkey, NULL, 0, &dwType, lpData, &cbData);
    ok ( r == ERROR_SUCCESS || r == ERROR_FILE_NOT_FOUND, "wrong error %d\n", r);
    if ( r == ERROR_SUCCESS )
    {
        lpData = HeapAlloc(GetProcessHeap(), 0, cbData);
        if (!lpData)
            skip("Out of memory\n");
        else
        {
            r = RegQueryValueExA(hkey, NULL, 0, &dwType, lpData, &cbData);
            ok ( r == ERROR_SUCCESS, "wrong error %d\n", r);
        }
    }

    r = RegSetValueA(hkey, NULL, REG_SZ, "test", strlen("test"));
    ok( r == ERROR_SUCCESS, "wrong error %d\n", r);

    r = MsiGetProductInfoA("", "", NULL, NULL);
    ok ( r == ERROR_INVALID_PARAMETER, "wrong error %d\n", r);

    if (lpData)
    {
        r = RegSetValueExA(hkey, NULL, 0, dwType, lpData, cbData);
        ok ( r == ERROR_SUCCESS, "wrong error %d\n", r);

        HeapFree(GetProcessHeap(), 0, lpData);
    }
    else
    {
        r = RegDeleteValueA(hkey, NULL);
        ok ( r == ERROR_SUCCESS, "wrong error %d\n", r);
    }

    r = RegCloseKey(hkey);
    ok( r == ERROR_SUCCESS, "wrong error %d\n", r);

    /* empty attribute */
    r = RegCreateKeyA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{F1C3AF50-8B56-4A69-A00C-00773FE42F30}", &hkey);
    ok( r == ERROR_SUCCESS, "wrong error %d\n", r);

    r = RegSetValueA(hkey, NULL, REG_SZ, "test", strlen("test"));
    ok( r == ERROR_SUCCESS, "wrong error %d\n", r);

    r = MsiGetProductInfoA("{F1C3AF50-8B56-4A69-A00C-00773FE42F30}", "", NULL, NULL);
    ok ( r == ERROR_UNKNOWN_PROPERTY, "wrong error %d\n", r);

    r = RegCloseKey(hkey);
    ok( r == ERROR_SUCCESS, "wrong error %d\n", r);

    r = RegDeleteKeyA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{F1C3AF50-8B56-4A69-A00C-00773FE42F30}");
    ok( r == ERROR_SUCCESS, "wrong error %d\n", r);
}

static void test_getcomponentpath(void)
{
    INSTALLSTATE r;
    char buffer[0x100];
    DWORD sz;

    if(!pMsiGetComponentPathA)
        return;

    r = pMsiGetComponentPathA( NULL, NULL, NULL, NULL );
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return value\n");

    r = pMsiGetComponentPathA( "bogus", "bogus", NULL, NULL );
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return value\n");

    r = pMsiGetComponentPathA( "bogus", "{00000000-0000-0000-000000000000}", NULL, NULL );
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return value\n");

    sz = sizeof buffer;
    buffer[0]=0;
    r = pMsiGetComponentPathA( "bogus", "{00000000-0000-0000-000000000000}", buffer, &sz );
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return value\n");

    r = pMsiGetComponentPathA( "{00000000-78E1-11D2-B60F-006097C998E7}",
        "{00000000-0000-0000-0000-000000000000}", buffer, &sz );
    ok( r == INSTALLSTATE_UNKNOWN, "wrong return value\n");

    r = pMsiGetComponentPathA( "{00000409-78E1-11D2-B60F-006097C998E7}",
        "{00000000-0000-0000-0000-00000000}", buffer, &sz );
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return value\n");

    r = pMsiGetComponentPathA( "{00000409-78E1-11D2-B60F-006097C998E7}",
        "{029E403D-A86A-1D11-5B5B0006799C897E}", buffer, &sz );
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return value\n");

    r = pMsiGetComponentPathA( "{00000000-78E1-11D2-B60F-006097C9987e}",
                            "{00000000-A68A-11d1-5B5B-0006799C897E}", buffer, &sz );
    ok( r == INSTALLSTATE_UNKNOWN, "wrong return value\n");
}

static void test_filehash(void)
{
    const char name[] = "msitest.bin";
    const char data[] = {'a','b','c'};
    HANDLE handle;
    UINT r;
    test_MSIFILEHASHINFO hash;
    DWORD count = 0;

    if (!pMsiGetFileHashA)
        return;

    DeleteFile(name);

    memset(&hash, 0, sizeof hash);
    r = pMsiGetFileHashA(name, 0, &hash);
    ok( r == ERROR_INVALID_PARAMETER, "wrong error %d\n", r);

    r = pMsiGetFileHashA(name, 0, NULL);
    ok( r == ERROR_INVALID_PARAMETER, "wrong error %d\n", r);

    memset(&hash, 0, sizeof hash);
    hash.dwFileHashInfoSize = sizeof hash;
    r = pMsiGetFileHashA(name, 0, &hash);
    ok( r == ERROR_FILE_NOT_FOUND, "wrong error %d\n", r);

    handle = CreateFile(name, GENERIC_READ|GENERIC_WRITE, 0, NULL, 
                CREATE_ALWAYS, FILE_ATTRIBUTE_ARCHIVE, NULL);
    ok(handle != INVALID_HANDLE_VALUE, "failed to create file\n");

    WriteFile(handle, data, sizeof data, &count, NULL);
    CloseHandle(handle);

    memset(&hash, 0, sizeof hash);
    r = pMsiGetFileHashA(name, 0, &hash);
    ok( r == ERROR_INVALID_PARAMETER, "wrong error %d\n", r);

    memset(&hash, 0, sizeof hash);
    hash.dwFileHashInfoSize = sizeof hash;
    r = pMsiGetFileHashA(name, 1, &hash);
    ok( r == ERROR_INVALID_PARAMETER, "wrong error %d\n", r);

    r = pMsiGetFileHashA(name, 0, &hash);
    ok( r == ERROR_SUCCESS, "wrong error %d\n", r);

    ok(hash.dwFileHashInfoSize == sizeof hash, "hash size changed\n");
    ok(hash.dwData[0] == 0x98500190 &&
       hash.dwData[1] == 0xb04fd23c &&
       hash.dwData[2] == 0x7d3f96d6 &&
       hash.dwData[3] == 0x727fe128, "hash of abc incorrect\n");

    DeleteFile(name);
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
    ConvertSidToStringSid(user->User.Sid, usersid);
}

static void test_MsiQueryProductState(void)
{
    CHAR prodcode[MAX_PATH];
    CHAR prod_squashed[MAX_PATH];
    CHAR keypath[MAX_PATH*2];
    LPSTR usersid;
    INSTALLSTATE state;
    LONG res;
    HKEY userkey, localkey, props;
    DWORD data;

    create_test_guid(prodcode, prod_squashed);
    get_user_sid(&usersid);

    /* NULL prodcode */
    state = MsiQueryProductStateA(NULL);
    ok(state == INSTALLSTATE_INVALIDARG, "Expected INSTALLSTATE_INVALIDARG, got %d\n", state);

    /* empty prodcode */
    state = MsiQueryProductStateA("");
    ok(state == INSTALLSTATE_INVALIDARG, "Expected INSTALLSTATE_INVALIDARG, got %d\n", state);

    /* garbage prodcode */
    state = MsiQueryProductStateA("garbage");
    ok(state == INSTALLSTATE_INVALIDARG, "Expected INSTALLSTATE_INVALIDARG, got %d\n", state);

    /* guid without brackets */
    state = MsiQueryProductStateA("6700E8CF-95AB-4D9C-BC2C-15840DEA7A5D");
    ok(state == INSTALLSTATE_INVALIDARG, "Expected INSTALLSTATE_INVALIDARG, got %d\n", state);

    /* guid with brackets */
    state = MsiQueryProductStateA("{6700E8CF-95AB-4D9C-BC2C-15840DEA7A5D}");
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    /* same length as guid, but random */
    state = MsiQueryProductStateA("A938G02JF-2NF3N93-VN3-2NNF-3KGKALDNF93");
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    /* created guid cannot possibly be an installed product code */
    state = MsiQueryProductStateA(prodcode);
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    lstrcpyA(keypath, "Software\\Microsoft\\Installer\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_CURRENT_USER, keypath, &userkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* user product key exists */
    state = MsiQueryProductStateA(prodcode);
    ok(state == INSTALLSTATE_ADVERTISED, "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\");
    lstrcatA(keypath, prodcode);

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &localkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* local uninstall key exists */
    state = MsiQueryProductStateA(prodcode);
    ok(state == INSTALLSTATE_ADVERTISED, "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);

    data = 1;
    res = RegSetValueExA(localkey, "WindowsInstaller", 0, REG_DWORD, (const BYTE *)&data, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* WindowsInstaller value exists */
    state = MsiQueryProductStateA(prodcode);
    ok(state == INSTALLSTATE_ADVERTISED, "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);

    RegDeleteValueA(localkey, "WindowsInstaller");
    RegDeleteKeyA(localkey, "");

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Products\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &localkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* local product key exists */
    state = MsiQueryProductStateA(prodcode);
    ok(state == INSTALLSTATE_ADVERTISED, "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);

    res = RegCreateKeyA(localkey, "InstallProperties", &props);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* install properties key exists */
    state = MsiQueryProductStateA(prodcode);
    ok(state == INSTALLSTATE_ADVERTISED, "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);

    data = 1;
    res = RegSetValueExA(props, "WindowsInstaller", 0, REG_DWORD, (const BYTE *)&data, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* WindowsInstaller value exists */
    state = MsiQueryProductStateA(prodcode);
    ok(state == INSTALLSTATE_DEFAULT, "Expected INSTALLSTATE_DEFAULT, got %d\n", state);

    data = 2;
    res = RegSetValueExA(props, "WindowsInstaller", 0, REG_DWORD, (const BYTE *)&data, sizeof(DWORD));
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* WindowsInstaller value is not 1 */
    state = MsiQueryProductStateA(prodcode);
    ok(state == INSTALLSTATE_DEFAULT, "Expected INSTALLSTATE_DEFAULT, got %d\n", state);

    RegDeleteKeyA(userkey, "");

    /* user product key does not exist */
    state = MsiQueryProductStateA(prodcode);
    ok(state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);

    LocalFree(usersid);
    RegDeleteValueA(props, "WindowsInstaller");
    RegDeleteKeyA(props, "");
    RegDeleteKeyA(localkey, "");
    RegCloseKey(userkey);
    RegCloseKey(localkey);
    RegCloseKey(props);
}

static const char table_enc85[] =
"!$%&'()*+,-.0123456789=?@ABCDEFGHIJKLMNO"
"PQRSTUVWXYZ[]^_`abcdefghijklmnopqrstuvwx"
"yz{}~";

/*
 *  Encodes a base85 guid given a GUID pointer
 *  Caller should provide a 21 character buffer for the encoded string.
 *
 *  returns TRUE if successful, FALSE if not
 */
static BOOL encode_base85_guid( GUID *guid, LPWSTR str )
{
    unsigned int x, *p, i;

    p = (unsigned int*) guid;
    for( i=0; i<4; i++ )
    {
        x = p[i];
        *str++ = table_enc85[x%85];
        x = x/85;
        *str++ = table_enc85[x%85];
        x = x/85;
        *str++ = table_enc85[x%85];
        x = x/85;
        *str++ = table_enc85[x%85];
        x = x/85;
        *str++ = table_enc85[x%85];
    }
    *str = 0;

    return TRUE;
}

static void compose_base85_guid(LPSTR component, LPSTR comp_base85, LPSTR squashed)
{
    WCHAR guidW[MAX_PATH];
    WCHAR base85W[MAX_PATH];
    WCHAR squashedW[MAX_PATH];
    GUID guid;
    HRESULT hr;
    int size;

    hr = CoCreateGuid(&guid);
    ok(hr == S_OK, "Expected S_OK, got %d\n", hr);

    size = StringFromGUID2(&guid, (LPOLESTR)guidW, MAX_PATH);
    ok(size == 39, "Expected 39, got %d\n", hr);

    WideCharToMultiByte(CP_ACP, 0, guidW, size, component, MAX_PATH, NULL, NULL);
    encode_base85_guid(&guid, base85W);
    WideCharToMultiByte(CP_ACP, 0, base85W, -1, comp_base85, MAX_PATH, NULL, NULL);
    squash_guid(guidW, squashedW);
    WideCharToMultiByte(CP_ACP, 0, squashedW, -1, squashed, MAX_PATH, NULL, NULL);
}

static void test_MsiQueryFeatureState(void)
{
    HKEY userkey, localkey, compkey;
    CHAR prodcode[MAX_PATH];
    CHAR prod_squashed[MAX_PATH];
    CHAR component[MAX_PATH];
    CHAR comp_base85[MAX_PATH];
    CHAR comp_squashed[MAX_PATH];
    CHAR keypath[MAX_PATH*2];
    INSTALLSTATE state;
    LPSTR usersid;
    LONG res;

    create_test_guid(prodcode, prod_squashed);
    compose_base85_guid(component, comp_base85, comp_squashed);
    get_user_sid(&usersid);

    /* NULL prodcode */
    state = MsiQueryFeatureStateA(NULL, "feature");
    ok(state == INSTALLSTATE_INVALIDARG, "Expected INSTALLSTATE_INVALIDARG, got %d\n", state);

    /* empty prodcode */
    state = MsiQueryFeatureStateA("", "feature");
    ok(state == INSTALLSTATE_INVALIDARG, "Expected INSTALLSTATE_INVALIDARG, got %d\n", state);

    /* garbage prodcode */
    state = MsiQueryFeatureStateA("garbage", "feature");
    ok(state == INSTALLSTATE_INVALIDARG, "Expected INSTALLSTATE_INVALIDARG, got %d\n", state);

    /* guid without brackets */
    state = MsiQueryFeatureStateA("6700E8CF-95AB-4D9C-BC2C-15840DEA7A5D", "feature");
    ok(state == INSTALLSTATE_INVALIDARG, "Expected INSTALLSTATE_INVALIDARG, got %d\n", state);

    /* guid with brackets */
    state = MsiQueryFeatureStateA("{6700E8CF-95AB-4D9C-BC2C-15840DEA7A5D}", "feature");
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    /* same length as guid, but random */
    state = MsiQueryFeatureStateA("A938G02JF-2NF3N93-VN3-2NNF-3KGKALDNF93", "feature");
    ok(state == INSTALLSTATE_INVALIDARG, "Expected INSTALLSTATE_INVALIDARG, got %d\n", state);

    /* NULL szFeature */
    state = MsiQueryFeatureStateA(prodcode, NULL);
    ok(state == INSTALLSTATE_INVALIDARG, "Expected INSTALLSTATE_INVALIDARG, got %d\n", state);

    /* empty szFeature */
    state = MsiQueryFeatureStateA(prodcode, "");
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    /* feature key does not exist yet */
    state = MsiQueryFeatureStateA(prodcode, "feature");
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    lstrcpyA(keypath, "Software\\Microsoft\\Installer\\Features\\");
    lstrcatA(keypath, prod_squashed);

    res = RegCreateKeyA(HKEY_CURRENT_USER, keypath, &userkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    /* feature key exists */
    state = MsiQueryFeatureStateA(prodcode, "feature");
    ok(state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);

    res = RegSetValueExA(userkey, "feature", 0, REG_SZ, (const BYTE *)"", 8);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    state = MsiQueryFeatureStateA(prodcode, "feature");
    ok(state == INSTALLSTATE_ADVERTISED, "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Products\\");
    lstrcatA(keypath, prod_squashed);
    lstrcatA(keypath, "\\Features");

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &localkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    state = MsiQueryFeatureStateA(prodcode, "feature");
    ok(state == INSTALLSTATE_ADVERTISED, "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);

    res = RegSetValueExA(localkey, "feature", 0, REG_SZ, (const BYTE *)"aaaaaaaaaaaaaaaaaaa", 20);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    state = MsiQueryFeatureStateA(prodcode, "feature");
    ok(state == INSTALLSTATE_BADCONFIG, "Expected INSTALLSTATE_BADCONFIG, got %d\n", state);

    res = RegSetValueExA(localkey, "feature", 0, REG_SZ, (const BYTE *)"aaaaaaaaaaaaaaaaaaaa", 21);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    state = MsiQueryFeatureStateA(prodcode, "feature");
    ok(state == INSTALLSTATE_ADVERTISED, "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);

    res = RegSetValueExA(localkey, "feature", 0, REG_SZ, (const BYTE *)"aaaaaaaaaaaaaaaaaaaaa", 22);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    state = MsiQueryFeatureStateA(prodcode, "feature");
    ok(state == INSTALLSTATE_ADVERTISED, "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);

    res = RegSetValueExA(localkey, "feature", 0, REG_SZ, (const BYTE *)comp_base85, 21);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    state = MsiQueryFeatureStateA(prodcode, "feature");
    ok(state == INSTALLSTATE_ADVERTISED, "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);

    lstrcpyA(keypath, "Software\\Microsoft\\Windows\\CurrentVersion\\Installer\\UserData\\");
    lstrcatA(keypath, usersid);
    lstrcatA(keypath, "\\Components\\");
    lstrcatA(keypath, comp_squashed);

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, keypath, &compkey);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    state = MsiQueryFeatureStateA(prodcode, "feature");
    ok(state == INSTALLSTATE_ADVERTISED, "Expected INSTALLSTATE_ADVERTISED, got %d\n", state);

    res = RegSetValueExA(compkey, prod_squashed, 0, REG_SZ, (const BYTE *)"", 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    state = MsiQueryFeatureStateA(prodcode, "feature");
    todo_wine
    {
        ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    }

    res = RegSetValueExA(compkey, prod_squashed, 0, REG_SZ, (const BYTE *)"apple", 1);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    state = MsiQueryFeatureStateA(prodcode, "feature");
    todo_wine
    {
        ok(state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    }

    RegDeleteValueA(compkey, prod_squashed);
    RegDeleteValueA(compkey, "");
    RegDeleteValueA(localkey, "feature");
    RegDeleteValueA(userkey, "feature");
    RegDeleteKeyA(userkey, "");
    RegCloseKey(compkey);
    RegCloseKey(localkey);
    RegCloseKey(userkey);
}

START_TEST(msi)
{
    HMODULE hmod = GetModuleHandle("msi.dll");
    pMsiUseFeatureExA = (fnMsiUseFeatureExA) 
        GetProcAddress(hmod, "MsiUseFeatureExA");
    pMsiOpenPackageExA = (fnMsiOpenPackageExA) 
        GetProcAddress(hmod, "MsiOpenPackageExA");
    pMsiOpenPackageExW = (fnMsiOpenPackageExW) 
        GetProcAddress(hmod, "MsiOpenPackageExW");
    pMsiGetComponentPathA = (fnMsiGetComponentPathA)
        GetProcAddress(hmod, "MsiGetComponentPathA" );
    pMsiGetFileHashA = (fnMsiGetFileHashA)
        GetProcAddress(hmod, "MsiGetFileHashA" );

    test_usefeature();
    test_null();
    test_getcomponentpath();
    test_filehash();
    test_MsiQueryProductState();
    test_MsiQueryFeatureState();
}
