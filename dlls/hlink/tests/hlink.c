/*
 * Implementation of hyperlinking (hlink.dll)
 *
 * Copyright 2006 Mike McCormack
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

#include <hlink.h>
#include <hlguids.h>

#include "wine/test.h"

static void test_HlinkIsShortcut(void)
{
    int i;
    HRESULT hres;

    static const WCHAR file0[] = {'f','i','l','e',0};
    static const WCHAR file1[] = {'f','i','l','e','.','u','r','l',0};
    static const WCHAR file2[] = {'f','i','l','e','.','l','n','k',0};
    static const WCHAR file3[] = {'f','i','l','e','.','u','R','l',0};
    static const WCHAR file4[] = {'f','i','l','e','u','r','l',0};
    static const WCHAR file5[] = {'c',':','\\','f','i','l','e','.','u','r','l',0};
    static const WCHAR file6[] = {'c',':','\\','f','i','l','e','.','l','n','k',0};
    static const WCHAR file7[] = {'.','u','r','l',0};

    static struct {
        LPCWSTR file;
        HRESULT hres;
    } shortcut_test[] = {
        {file0, S_FALSE},
        {file1, S_OK},
        {file2, S_FALSE},
        {file3, S_OK},
        {file4, S_FALSE},
        {file5, S_OK},
        {file6, S_FALSE},
        {file7, S_OK},
        {NULL,  E_INVALIDARG}
    };

    for(i=0; i<sizeof(shortcut_test)/sizeof(shortcut_test[0]); i++) {
        hres = HlinkIsShortcut(shortcut_test[i].file);
        ok(hres == shortcut_test[i].hres, "[%d] HlinkIsShortcut returned %08lx, expected %08lx\n",
           i, hres, shortcut_test[i].hres);
    }
}

START_TEST(hlink)
{
    HRESULT r;
    IHlink *lnk = NULL;
    IMoniker *mk = NULL;
    const WCHAR url[] = { 'h','t','t','p',':','/','/','w','i','n','e','h','q','.','o','r','g',0 };
    const WCHAR url2[] = { 'h','t','t','p',':','/','/','w','i','n','e','h','q','.','o','r','g','/',0 };
    LPWSTR str = NULL;

    CoInitialize(NULL);

    r = HlinkCreateFromString(url, NULL, NULL, NULL,
                                0, NULL, &IID_IHlink, (LPVOID*) &lnk);
    ok(r == S_OK, "failed to create link\n");
    if (FAILED(r))
        return;

    r = IHlink_GetMonikerReference(lnk, HLINKGETREF_DEFAULT, NULL, NULL);
    ok(r == S_OK, "failed\n");

    r = IHlink_GetMonikerReference(lnk, HLINKGETREF_DEFAULT, &mk, &str);
    ok(r == S_OK, "failed\n");
    ok(mk != NULL, "no moniker\n");
    ok(str == NULL, "string should be null\n");

    r = IMoniker_Release(mk);
    ok( r == 1, "moniker refcount wrong\n");

    r = IHlink_GetStringReference(lnk, -1, &str, NULL);
    ok(r == S_OK, "failed\n");

    r = IHlink_GetStringReference(lnk, HLINKGETREF_DEFAULT, &str, NULL);
    ok(r == S_OK, "failed\n");
    todo_wine {
    ok(!lstrcmpW(str, url2), "url wrong\n");
    }
    CoTaskMemFree(str);

    r = IHlink_GetStringReference(lnk, HLINKGETREF_DEFAULT, NULL, NULL);
    ok(r == S_OK, "failed\n");

    r = IHlink_GetStringReference(lnk, HLINKGETREF_DEFAULT, NULL, &str);
    ok(r == S_OK, "failed\n");
    ok(str == NULL, "string should be null\n");

    test_HlinkIsShortcut();
}
