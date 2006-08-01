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
}
