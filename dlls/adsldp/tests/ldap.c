/*
 * LDAPNamespace Tests
 *
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
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "iads.h"
#include "adserr.h"
#include "adshlp.h"

#include "wine/test.h"

#include "initguid.h"
DEFINE_GUID(CLSID_LDAP,0x228d9a81,0xc302,0x11cf,0x9a,0xa4,0x00,0xaa,0x00,0x4a,0x56,0x91);
DEFINE_GUID(CLSID_LDAPNamespace,0x228d9a82,0xc302,0x11cf,0x9a,0xa4,0x00,0xaa,0x00,0x4a,0x56,0x91);
DEFINE_OLEGUID(CLSID_PointerMoniker,0x306,0,0);

static BOOL server_down;

static const struct
{
    const WCHAR *path;
    HRESULT hr, hr_ads_open, hr_ads_get;
    const WCHAR *user, *password;
    LONG flags;
} test[] =
{
    { L"invalid", MK_E_SYNTAX, E_ADS_BAD_PATHNAME, E_FAIL },
    { L"LDAP", MK_E_SYNTAX, E_ADS_BAD_PATHNAME, E_FAIL },
    { L"LDAP:", S_OK },
    { L"LDAP:/", E_ADS_BAD_PATHNAME },
    { L"LDAP://", E_ADS_BAD_PATHNAME },
    { L"LDAP://ldap.forumsys.com", S_OK },
    { L"LDAP:///ldap.forumsys.com", E_ADS_BAD_PATHNAME },
    { L"LDAP://ldap.forumsys.com:389", S_OK },
    { L"LDAP://ldap.forumsys.com:389/DC=example,DC=com", S_OK },
    { L"LDAP://ldap.forumsys.com/", E_ADS_BAD_PATHNAME },
    { L"LDAP://ldap.forumsys.com/rootDSE", S_OK },
    { L"LDAP://ldap.forumsys.com/rootDSE/", E_ADS_BAD_PATHNAME },
    { L"LDAP://ldap.forumsys.com/rootDSE/invalid", E_ADS_BAD_PATHNAME },
    { L"LDAP://ldap.forumsys.com/rootDSE", S_OK, S_OK, S_OK, NULL, NULL, 0 },
    { L"LDAP://ldap.forumsys.com/rootDSE", S_OK, S_OK, S_OK, L"CN=read-only-admin,DC=example,DC=com", L"password", 0 },
};

static void test_LDAP(void)
{
    HRESULT hr;
    IUnknown *unk;
    IADs *ads;
    IADsOpenDSObject *ads_open;
    IDispatch *disp;
    BSTR path, user, password;
    int i;

    if (server_down) return;

    hr = CoCreateInstance(&CLSID_LDAPNamespace, 0, CLSCTX_INPROC_SERVER, &IID_IADs, (void **)&ads);
    ok(hr == S_OK, "got %#lx\n", hr);
    IADs_Release(ads);

    hr = CoCreateInstance(&CLSID_LDAPNamespace, 0, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void **)&unk);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IUnknown_QueryInterface(unk, &IID_IDispatch, (void **)&disp);
    ok(hr == S_OK, "got %#lx\n", hr);
    IDispatch_Release(disp);

    hr = IUnknown_QueryInterface(unk, &IID_IADsOpenDSObject, (void **)&ads_open);
    ok(hr == S_OK, "got %#lx\n", hr);

    for (i = 0; i < ARRAY_SIZE(test); i++)
    {
        path = SysAllocString(test[i].path);
        user = test[i].user ? SysAllocString(test[i].user) : NULL;
        password = test[i].password ? SysAllocString(test[i].password) : NULL;

        hr = IADsOpenDSObject_OpenDSObject(ads_open, path, user, password, test[i].flags, &disp);
        if (hr == HRESULT_FROM_WIN32(ERROR_DS_SERVER_DOWN))
        {
            SysFreeString(path);
            skip("server is down\n");
            server_down = TRUE;
            break;
        }
        ok(hr == test[i].hr || hr == test[i].hr_ads_open, "%d: got %#lx, expected %#lx\n", i, hr, test[i].hr);
        if (hr == S_OK)
            IDispatch_Release(disp);

        hr = ADsOpenObject(path, user, password, test[i].flags, &IID_IADs, (void **)&ads);
        if (hr == HRESULT_FROM_WIN32(ERROR_DS_SERVER_DOWN))
        {
            SysFreeString(path);
            skip("server is down\n");
            server_down = TRUE;
            break;
        }
        ok(hr == test[i].hr || hr == test[i].hr_ads_get, "%d: got %#lx, expected %#lx\n", i, hr, test[i].hr);
        if (hr == S_OK)
            IADs_Release(ads);

        hr = ADsGetObject(path, &IID_IDispatch, (void **)&disp);
        if (hr == HRESULT_FROM_WIN32(ERROR_DS_SERVER_DOWN))
        {
            SysFreeString(path);
            skip("server is down\n");
            server_down = TRUE;
            break;
        }
        ok(hr == test[i].hr || hr == test[i].hr_ads_get, "%d: got %#lx, expected %#lx\n", i, hr, test[i].hr);
        if (hr == S_OK)
            IDispatch_Release(disp);

        SysFreeString(path);
        SysFreeString(user);
        SysFreeString(password);
    }


    IADsOpenDSObject_Release(ads_open);
    IUnknown_Release(unk);
}

static void test_ParseDisplayName(void)
{
    HRESULT hr;
    IBindCtx *bc;
    IParseDisplayName *parse;
    IMoniker *mk;
    IUnknown *unk;
    CLSID clsid;
    BSTR path;
    ULONG count;
    int i;

    if (server_down) return;

    hr = CoCreateInstance(&CLSID_LDAP, 0, CLSCTX_INPROC_SERVER, &IID_IParseDisplayName, (void **)&parse);
    ok(hr == S_OK, "got %#lx\n", hr);
    IParseDisplayName_Release(parse);

    hr = CoCreateInstance(&CLSID_LDAP, 0, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void **)&unk);
    ok(hr == S_OK, "got %#lx\n", hr);
    hr = IUnknown_QueryInterface(unk, &IID_IParseDisplayName, (void **)&parse);
    ok(hr == S_OK, "got %#lx\n", hr);
    IUnknown_Release(unk);

    hr = CreateBindCtx(0, &bc);
    ok(hr == S_OK, "got %#lx\n", hr);

    for (i = 0; i < ARRAY_SIZE(test); i++)
    {
        path = SysAllocString(test[i].path);

        count = 0xdeadbeef;
        hr = IParseDisplayName_ParseDisplayName(parse, bc, path, &count, &mk);
        if (hr == HRESULT_FROM_WIN32(ERROR_DS_SERVER_DOWN))
        {
            SysFreeString(path);
            skip("server is down\n");
            server_down = TRUE;
            break;
        }
        ok(hr == test[i].hr || hr == test[i].hr_ads_open, "%d: got %#lx, expected %#lx\n", i, hr, test[i].hr);
        if (hr == S_OK)
        {
            ok(count == lstrlenW(test[i].path), "%d: got %ld\n", i, count);

            hr = IMoniker_GetClassID(mk, &clsid);
            ok(hr == S_OK, "got %#lx\n", hr);
            ok(IsEqualGUID(&clsid, &CLSID_PointerMoniker), "%d: got %s\n", i, wine_dbgstr_guid(&clsid));

            IMoniker_Release(mk);
        }

        SysFreeString(path);

        count = 0xdeadbeef;
        hr = MkParseDisplayName(bc, test[i].path, &count, &mk);
        if (hr == HRESULT_FROM_WIN32(ERROR_DS_SERVER_DOWN))
        {
            skip("server is down\n");
            server_down = TRUE;
            break;
        }
        todo_wine_if(i == 0 || i == 1 || i == 11 || i == 12)
        ok(hr == test[i].hr, "%d: got %#lx, expected %#lx\n", i, hr, test[i].hr);
        if (hr == S_OK)
        {
            ok(count == lstrlenW(test[i].path), "%d: got %ld\n", i, count);

            hr = IMoniker_GetClassID(mk, &clsid);
            ok(hr == S_OK, "got %#lx\n", hr);
            ok(IsEqualGUID(&clsid, &CLSID_PointerMoniker), "%d: got %s\n", i, wine_dbgstr_guid(&clsid));

            IMoniker_Release(mk);
        }
    }

    IBindCtx_Release(bc);
    IParseDisplayName_Release(parse);
}

struct result
{
    const WCHAR *name;
    ADSTYPEENUM type;
    const WCHAR *values[16];
};

struct search
{
    const WCHAR *dn;
    ADS_SCOPEENUM scope;
    struct result res[16];
};

static void do_search(const struct search *s)
{
    HRESULT hr;
    IDirectorySearch *ds;
    ADS_SEARCHPREF_INFO pref[2];
    ADS_SEARCH_HANDLE sh;
    ADS_SEARCH_COLUMN col;
    LPWSTR name;
    const struct result *res;

    if (server_down) return;

    trace("search DN %s\n", wine_dbgstr_w(s->dn));

    hr = ADsGetObject(s->dn, &IID_IDirectorySearch, (void **)&ds);
    if (hr == HRESULT_FROM_WIN32(ERROR_DS_SERVER_DOWN))
    {
        skip("server is down\n");
        server_down = TRUE;
        return;
    }
    ok(hr == S_OK, "got %#lx\n", hr);

    pref[0].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
    pref[0].vValue.dwType = ADSTYPE_INTEGER;
    pref[0].vValue.Integer = s->scope;
    pref[0].dwStatus = 0xdeadbeef;

    pref[1].dwSearchPref = ADS_SEARCHPREF_SIZE_LIMIT;
    pref[1].vValue.dwType = ADSTYPE_INTEGER;
    pref[1].vValue.Integer = 5;
    pref[1].dwStatus = 0xdeadbeef;

    hr = IDirectorySearch_SetSearchPreference(ds, pref, ARRAY_SIZE(pref));
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(pref[0].dwStatus == ADS_STATUS_S_OK, "got %u\n", pref[0].dwStatus);
    ok(pref[1].dwStatus == ADS_STATUS_S_OK, "got %u\n", pref[1].dwStatus);

    hr = IDirectorySearch_ExecuteSearch(ds, (WCHAR *)L"(objectClass=*)", NULL, ~0, &sh);
    ok(hr == S_OK, "got %#lx\n", hr);

    res = s->res;

    while ((hr = IDirectorySearch_GetNextRow(ds, sh)) != S_ADS_NOMORE_ROWS)
    {
        ok(hr == S_OK, "got %#lx\n", hr);

        while ((hr = IDirectorySearch_GetNextColumnName(ds, sh, &name)) != S_ADS_NOMORE_COLUMNS)
        {
            DWORD i;

            ok(hr == S_OK, "got %#lx\n", hr);
            ok(res->name != NULL, "got extra row %s\n", wine_dbgstr_w(name));
            ok(!wcscmp(res->name, name), "expected %s, got %s\n", wine_dbgstr_w(res->name), wine_dbgstr_w(name));

            memset(&col, 0xde, sizeof(col));
            hr = IDirectorySearch_GetColumn(ds, sh, name, &col);
            ok(hr == S_OK, "got %#lx\n", hr);

            ok(col.dwADsType == res->type, "got %d for %s\n", col.dwADsType, wine_dbgstr_w(name));

            for (i = 0; i < col.dwNumValues; i++)
            {
                ok(col.pADsValues[i].dwType == col.dwADsType, "%lu: got %d for %s\n", i, col.pADsValues[i].dwType, wine_dbgstr_w(name));

                ok(res->values[i] != NULL, "expected to have more values for %s\n", wine_dbgstr_w(name));
                if (!res->values[i]) break;

                ok(!wcscmp(res->values[i], col.pADsValues[i].CaseIgnoreString),
                   "expected %s, got %s\n", wine_dbgstr_w(res->values[i]), wine_dbgstr_w(col.pADsValues[i].CaseIgnoreString));
            }

            IDirectorySearch_FreeColumn(ds, &col);
            FreeADsMem(name);
            res++;
        }
    }

    ok(res->name == NULL, "there are more rows in test data: %s\n", wine_dbgstr_w(res->name));

    hr = IDirectorySearch_CloseSearchHandle(ds, sh);
    ok(hr == S_OK, "got %#lx\n", hr);

    IDirectorySearch_Release(ds);
}

static void test_DirectorySearch(void)
{
    static const struct search root_base =
    {
        L"LDAP://ldap.forumsys.com", ADS_SCOPE_BASE,
        {
            { L"objectClass", ADSTYPE_CASE_IGNORE_STRING, { L"top", L"OpenLDAProotDSE", NULL } },
            { L"ADsPath", ADSTYPE_CASE_IGNORE_STRING, { L"LDAP://ldap.forumsys.com/", NULL } },
            { NULL }
        }
    };
    static const struct search scientists_base =
    {
        L"LDAP://ldap.forumsys.com/OU=scientists,DC=example,DC=com", ADS_SCOPE_BASE,
        {
            { L"uniqueMember", ADSTYPE_CASE_IGNORE_STRING, { L"uid=einstein,dc=example,dc=com",
              L"uid=tesla,dc=example,dc=com", L"uid=newton,dc=example,dc=com", L"uid=galileo,dc=example,dc=com",
              L"uid=training,dc=example,dc=com", L"uid=jmacy,dc=example,dc=com", NULL } },
            { L"ou", ADSTYPE_CASE_IGNORE_STRING, { L"scientists", NULL } },
            { L"cn", ADSTYPE_CASE_IGNORE_STRING, { L"Scientists", NULL } },
            { L"objectClass", ADSTYPE_CASE_IGNORE_STRING, { L"groupOfUniqueNames", L"top", NULL } },
            { L"ADsPath", ADSTYPE_CASE_IGNORE_STRING, { L"LDAP://ldap.forumsys.com/ou=scientists,dc=example,dc=com", NULL } },
            { NULL }
        }
    };
    static const struct search scientists_subtree =
    {
        L"LDAP://ldap.forumsys.com/OU=scientists,DC=example,DC=com", ADS_SCOPE_SUBTREE,
        {
            { L"uniqueMember", ADSTYPE_CASE_IGNORE_STRING, { L"uid=einstein,dc=example,dc=com",
              L"uid=tesla,dc=example,dc=com", L"uid=newton,dc=example,dc=com", L"uid=galileo,dc=example,dc=com",
              L"uid=training,dc=example,dc=com", L"uid=jmacy,dc=example,dc=com", NULL } },
            { L"ou", ADSTYPE_CASE_IGNORE_STRING, { L"scientists", NULL } },
            { L"cn", ADSTYPE_CASE_IGNORE_STRING, { L"Scientists", NULL } },
            { L"objectClass", ADSTYPE_CASE_IGNORE_STRING, { L"groupOfUniqueNames", L"top", NULL } },
            { L"ADsPath", ADSTYPE_CASE_IGNORE_STRING, { L"LDAP://ldap.forumsys.com/ou=scientists,dc=example,dc=com", NULL } },
            { L"uniqueMember", ADSTYPE_CASE_IGNORE_STRING, { L"uid=tesla,dc=example,dc=com", NULL } },
            { L"ou", ADSTYPE_CASE_IGNORE_STRING, { L"italians", NULL } },
            { L"cn", ADSTYPE_CASE_IGNORE_STRING, { L"Italians", NULL } },
            { L"objectClass", ADSTYPE_CASE_IGNORE_STRING, { L"groupOfUniqueNames", L"top", NULL } },
            { L"ADsPath", ADSTYPE_CASE_IGNORE_STRING, { L"LDAP://ldap.forumsys.com/ou=italians,ou=scientists,dc=example,dc=com", NULL } },
            { NULL }
        }
    };
    HRESULT hr;
    IDirectorySearch *ds;
    ADS_SEARCHPREF_INFO pref[2];
    ADS_SEARCH_HANDLE sh;
    ADS_SEARCH_COLUMN col;
    LPWSTR name;

    if (server_down) return;

    hr = ADsGetObject(L"LDAP:", &IID_IDirectorySearch, (void **)&ds);
    ok(hr == E_NOINTERFACE, "got %#lx\n", hr);

    hr = ADsGetObject(L"LDAP://ldap.forumsys.com/rootDSE", &IID_IDirectorySearch, (void **)&ds);
    if (hr == HRESULT_FROM_WIN32(ERROR_DS_SERVER_DOWN))
    {
        skip("server is down\n");
        server_down = TRUE;
        return;
    }
    ok(hr == E_NOINTERFACE, "got %#lx\n", hr);

    hr = ADsGetObject(L"LDAP://ldap.forumsys.com", &IID_IDirectorySearch, (void **)&ds);
    if (hr == HRESULT_FROM_WIN32(ERROR_DS_SERVER_DOWN))
    {
        skip("server is down\n");
        server_down = TRUE;
        return;
    }
    ok(hr == S_OK, "got %#lx\n", hr);

    pref[0].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
    pref[0].vValue.dwType = ADSTYPE_INTEGER;
    pref[0].vValue.Integer = ADS_SCOPE_BASE;
    pref[0].dwStatus = 0xdeadbeef;

    pref[1].dwSearchPref = ADS_SEARCHPREF_SIZE_LIMIT;
    pref[1].vValue.dwType = ADSTYPE_INTEGER;
    pref[1].vValue.Integer = 5;
    pref[1].dwStatus = 0xdeadbeef;

    hr = IDirectorySearch_SetSearchPreference(ds, pref, ARRAY_SIZE(pref));
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(pref[0].dwStatus == ADS_STATUS_S_OK, "got %u\n", pref[0].dwStatus);
    ok(pref[1].dwStatus == ADS_STATUS_S_OK, "got %u\n", pref[1].dwStatus);

    hr = IDirectorySearch_ExecuteSearch(ds, (WCHAR *)L"(objectClass=*)", NULL, ~0, NULL);
    ok(hr == E_ADS_BAD_PARAMETER, "got %#lx\n", hr);

    hr = IDirectorySearch_ExecuteSearch(ds, (WCHAR *)L"(objectClass=*)", NULL, 1, &sh);
    ok(hr == E_ADS_BAD_PARAMETER, "got %#lx\n", hr);

    hr = IDirectorySearch_ExecuteSearch(ds, (WCHAR *)L"(objectClass=*)", NULL, ~0, &sh);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IDirectorySearch_GetNextColumnName(ds, sh, &name);
    ok(hr == E_ADS_BAD_PARAMETER, "got %#lx\n", hr);

    hr = IDirectorySearch_GetPreviousRow(ds, sh);
    todo_wine
    ok(hr == E_ADS_BAD_PARAMETER, "got %#lx\n", hr);

    while (IDirectorySearch_GetNextRow(ds, sh) != S_ADS_NOMORE_ROWS)
    {
        while (IDirectorySearch_GetNextColumnName(ds, sh, &name) != S_ADS_NOMORE_COLUMNS)
        {
            DWORD i;

            hr = IDirectorySearch_GetColumn(ds, sh, name, &col);
            ok(hr == S_OK, "got %#lx for column %s\n", hr, wine_dbgstr_w(name));

            if (winetest_debug > 1) /* useful to create test arrays */
            {
                printf("Column %s (values type %d):\n", wine_dbgstr_w(name), col.dwADsType);
                printf("{ ");
                for (i = 0; i < col.dwNumValues; i++)
                    printf("%s, ", wine_dbgstr_w(col.pADsValues[i].CaseIgnoreString));
                printf("NULL }\n");
            }

            hr = IDirectorySearch_FreeColumn(ds, &col);
            ok(hr == S_OK, "got %#lx\n", hr);
        }

        name = (void *)0xdeadbeef;
        hr = IDirectorySearch_GetNextColumnName(ds, sh, &name);
        ok(hr == S_ADS_NOMORE_COLUMNS, "got %#lx\n", hr);
        ok(name == NULL, "got %p/%s\n", name, wine_dbgstr_w(name));
    }

    hr = IDirectorySearch_GetNextRow(ds, sh);
    ok(hr == S_ADS_NOMORE_ROWS, "got %#lx\n", hr);

    name = NULL;
    hr = IDirectorySearch_GetNextColumnName(ds, sh, &name);
    todo_wine
    ok(hr == S_OK, "got %#lx\n", hr);
    todo_wine
    ok(name && !wcscmp(name, L"ADsPath"), "got %s\n", wine_dbgstr_w(name));
    FreeADsMem(name);

    name = (void *)0xdeadbeef;
    hr = IDirectorySearch_GetNextColumnName(ds, sh, &name);
    ok(hr == S_ADS_NOMORE_COLUMNS, "got %#lx\n", hr);
    ok(name == NULL, "got %p/%s\n", name, wine_dbgstr_w(name));

    hr = IDirectorySearch_GetColumn(ds, sh, NULL, &col);
    ok(hr == E_ADS_BAD_PARAMETER, "got %#lx\n", hr);

    hr = IDirectorySearch_GetFirstRow(ds, sh);
    ok(hr == S_OK, "got %#lx\n", hr);

    memset(&col, 0x55, sizeof(col));
    hr = IDirectorySearch_GetColumn(ds, sh, (WCHAR *)L"deadbeef", &col);
    ok(hr == E_ADS_COLUMN_NOT_SET, "got %#lx\n", hr);
    ok(!col.pszAttrName, "got %p\n", col.pszAttrName);
    ok(col.dwADsType == ADSTYPE_INVALID, "got %d\n", col.dwADsType);
    ok(!col.pADsValues, "got %p\n", col.pADsValues);
    ok(!col.dwNumValues, "got %lu\n", col.dwNumValues);
    ok(!col.hReserved, "got %p\n", col.hReserved);

    hr = IDirectorySearch_CloseSearchHandle(ds, sh);
    ok(hr == S_OK, "got %#lx\n", hr);

    IDirectorySearch_Release(ds);

    do_search(&root_base);
    do_search(&scientists_base);
    do_search(&scientists_subtree);
}

static void test_DirectoryObject(void)
{
    HRESULT hr;
    IDirectoryObject *dirobj;
    IUnknown *unk;
    IDirectorySearch *ds;
    ADS_SEARCHPREF_INFO pref[3];
    ADS_SEARCH_HANDLE sh;
    ADS_SEARCH_COLUMN col;

    if (server_down) return;

    hr = ADsGetObject(L"LDAP://ldap.forumsys.com/OU=scientists,DC=example,DC=com", &IID_IDirectoryObject, (void **)&dirobj);
    if (hr == HRESULT_FROM_WIN32(ERROR_DS_SERVER_DOWN))
    {
        skip("server is down\n");
        server_down = TRUE;
        return;
    }
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IDirectoryObject_QueryInterface(dirobj, &IID_IADsOpenDSObject, (void **)&unk);
    todo_wine
    ok(hr == E_NOINTERFACE, "got %#lx\n", hr);
    if (hr == S_OK) IUnknown_Release(unk);
    hr = IDirectoryObject_QueryInterface(dirobj, &IID_IDispatch, (void **)&unk);
    ok(hr == S_OK, "got %#lx\n", hr);
    IUnknown_Release(unk);
    hr = IDirectoryObject_QueryInterface(dirobj, &IID_IADs, (void **)&unk);
    ok(hr == S_OK, "got %#lx\n", hr);
    IUnknown_Release(unk);
    hr = IDirectoryObject_QueryInterface(dirobj, &IID_IDirectorySearch, (void **)&ds);
    ok(hr == S_OK, "got %#lx\n", hr);

    pref[0].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
    pref[0].vValue.dwType = ADSTYPE_INTEGER;
    pref[0].vValue.Integer = ADS_SCOPE_BASE;
    pref[0].dwStatus = 0xdeadbeef;

    pref[1].dwSearchPref = ADS_SEARCHPREF_SECURITY_MASK;
    pref[1].vValue.dwType = ADSTYPE_INTEGER;
    pref[1].vValue.Integer = ADS_SECURITY_INFO_OWNER | ADS_SECURITY_INFO_GROUP | ADS_SECURITY_INFO_DACL;
    pref[1].dwStatus = 0xdeadbeef;

    pref[2].dwSearchPref = ADS_SEARCHPREF_SIZE_LIMIT;
    pref[2].vValue.dwType = ADSTYPE_INTEGER;
    pref[2].vValue.Integer = 5;
    pref[2].dwStatus = 0xdeadbeef;

    hr = IDirectorySearch_SetSearchPreference(ds, pref, ARRAY_SIZE(pref));
    ok(hr == S_ADS_ERRORSOCCURRED, "got %#lx\n", hr);
    ok(pref[0].dwStatus == ADS_STATUS_S_OK, "got %d\n", pref[0].dwStatus);
    /* ldap.forumsys.com doesn't support NT security, real ADs DC - does  */
    ok(pref[1].dwStatus == ADS_STATUS_INVALID_SEARCHPREF, "got %d\n", pref[1].dwStatus);
    ok(pref[2].dwStatus == ADS_STATUS_S_OK, "got %d\n", pref[2].dwStatus);

    hr = IDirectorySearch_ExecuteSearch(ds, (WCHAR *)L"(objectClass=*)", NULL, ~0, &sh);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IDirectorySearch_GetNextRow(ds, sh);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IDirectorySearch_GetColumn(ds, sh, (WCHAR *)L"nTSecurityDescriptor", &col);
    ok(hr == E_ADS_COLUMN_NOT_SET, "got %#lx\n", hr);

    hr = IDirectorySearch_CloseSearchHandle(ds, sh);
    ok(hr == S_OK, "got %#lx\n", hr);

    pref[0].dwSearchPref = ADS_SEARCHPREF_TOMBSTONE;
    pref[0].vValue.dwType = ADSTYPE_BOOLEAN;
    pref[0].vValue.Integer = 1;
    pref[0].dwStatus = 0xdeadbeef;

    pref[1].dwSearchPref = ADS_SEARCHPREF_SIZE_LIMIT;
    pref[1].vValue.dwType = ADSTYPE_INTEGER;
    pref[1].vValue.Integer = 5;
    pref[1].dwStatus = 0xdeadbeef;

    hr = IDirectorySearch_SetSearchPreference(ds, pref, 2);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(pref[0].dwStatus == ADS_STATUS_S_OK, "got %d\n", pref[0].dwStatus);
    ok(pref[1].dwStatus == ADS_STATUS_S_OK, "got %d\n", pref[1].dwStatus);

    hr = IDirectorySearch_ExecuteSearch(ds, (WCHAR *)L"(objectClass=*)", NULL, ~0, &sh);
    todo_wine ok(hr == S_OK, "got %#lx\n", hr);
    if (hr == S_OK)
    {
        hr = IDirectorySearch_GetNextRow(ds, sh);
        ok(hr == HRESULT_FROM_WIN32(ERROR_DS_UNAVAILABLE_CRIT_EXTENSION), "got %#lx\n", hr);

        hr = IDirectorySearch_CloseSearchHandle(ds, sh);
        ok(hr == S_OK, "got %#lx\n", hr);
    }

    IDirectorySearch_Release(ds);
    IDirectoryObject_Release(dirobj);
}

START_TEST(ldap)
{
    HRESULT hr;

    hr = CoInitialize(NULL);
    ok(hr == S_OK, "got %#lx\n", hr);

    test_LDAP();
    test_ParseDisplayName();
    test_DirectorySearch();
    test_DirectoryObject();

    CoUninitialize();
}
