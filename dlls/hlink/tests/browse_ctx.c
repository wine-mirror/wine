/*
 * Copyright 2009 Andrew Eikum for CodeWeavers
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

#include <stdio.h>

#include <hlink.h>

#include "wine/test.h"

static void test_SetInitialHlink(void)
{
    IHlinkBrowseContext *bc;
    IHlink *found_hlink;
    IMoniker *dummy, *found_moniker;
    IBindCtx *bindctx;
    WCHAR one[] = {'1',0};
    WCHAR friend[] = {'f','r','i','e','n','d',0};
    WCHAR *found_name;
    HRESULT hres;

    hres = CreateItemMoniker(one, one, &dummy);
    ok(hres == S_OK, "CreateItemMoniker failed: 0x%08x\n", hres);

    hres = HlinkCreateBrowseContext(NULL, &IID_IHlinkBrowseContext, (void**)&bc);
    ok(hres == S_OK, "HlinkCreateBrowseContext failed: 0x%08x\n", hres);

    hres = IHlinkBrowseContext_SetInitialHlink(bc, dummy, one, friend);
    ok(hres == S_OK, "SetInitialHlink failed: 0x%08x\n", hres);

    hres = IHlinkBrowseContext_GetHlink(bc, HLID_CURRENT, &found_hlink);
    ok(hres == S_OK, "GetHlink failed: 0x%08x\n", hres);

    hres = IHlink_GetMonikerReference(found_hlink, HLINKGETREF_DEFAULT, &found_moniker, NULL);
    ok(hres == S_OK, "GetMonikerReference failed: 0x%08x\n", hres);

    hres = CreateBindCtx(0, &bindctx);
    ok(hres == S_OK, "CreateBindCtx failed: 0x%08x\n", hres);

    hres = IMoniker_GetDisplayName(found_moniker, bindctx, NULL, &found_name);
    ok(hres == S_OK, "GetDisplayName failed: 0x%08x\n", hres);
    ok(lstrcmpW(found_name, friend), "Found display name should have been %s, was: %s\n", wine_dbgstr_w(friend), wine_dbgstr_w(found_name));

    IBindCtx_Release(bindctx);
    IMoniker_Release(found_moniker);
    IHlink_Release(found_hlink);
    IHlinkBrowseContext_Release(bc);
    IMoniker_Release(dummy);
}

START_TEST(browse_ctx)
{
    CoInitialize(NULL);

    test_SetInitialHlink();

    CoUninitialize();
}
