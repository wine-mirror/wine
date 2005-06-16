/*
 * ITypeLib and ITypeInfo test
 *
 * Copyright 2004 Jacek Caban
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define COBJMACROS

#include <wine/test.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "oleauto.h"

static void ref_count_test(LPCWSTR type_lib)
{
    ITypeLib *iface;
    ITypeInfo *iti1, *iti2;
    HRESULT hRes;
    int ref_count;

    trace("Loading type library\n");
    hRes = LoadTypeLib(type_lib, &iface);
    ok(hRes == S_OK, "Could not load type library\n");
    if(hRes != S_OK)
        return;

    hRes = ITypeLib_GetTypeInfo(iface, 1, &iti1);
    ok(hRes == S_OK, "ITypeLib_GetTypeInfo failed on index = 1\n");
    ok(ref_count=ITypeLib_Release(iface) > 0, "ITypeLib destroyed while ITypeInfo has back pointer\n");
    if(!ref_count)
        return;

    hRes = ITypeLib_GetTypeInfo(iface, 1, &iti2);
    ok(hRes == S_OK, "ITypeLib_GetTypeInfo failed on index = 1\n");
    ok(iti1 == iti2, "ITypeLib_GetTypeInfo returned different pointers for same indexes\n");

    ITypeLib_AddRef(iface);
    ITypeInfo_Release(iti2);
    ITypeInfo_Release(iti1);
    ok(ITypeLib_Release(iface) == 0, "ITypeLib should be destroyed here.\n");
}

START_TEST(typelib)
{
    static const WCHAR type_lib_stdole32[] = {'s','t','d','o','l','e','3','2','.','t','l','b',0};
    ref_count_test(type_lib_stdole32);
}
