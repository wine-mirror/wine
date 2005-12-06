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

#define ok_ole_success(hr, func) ok(hr == S_OK, #func " failed with error 0x%08lx\n", hr)

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

static void test_TypeComp(void)
{
    ITypeLib *pTypeLib;
    ITypeComp *pTypeComp;
    HRESULT hr;
    ULONG ulHash;
    DESCKIND desckind;
    BINDPTR bindptr;
    ITypeInfo *pTypeInfo;
    static WCHAR wszStdOle2[] = {'s','t','d','o','l','e','2','.','t','l','b',0};
    static WCHAR wszStdFunctions[] = {'S','t','d','F','u','n','c','t','i','o','n','s',0};
    static WCHAR wszSavePicture[] = {'S','a','v','e','P','i','c','t','u','r','e',0};
    static WCHAR wszOLE_TRISTATE[] = {'O','L','E','_','T','R','I','S','T','A','T','E',0};
    static WCHAR wszUnchecked[] = {'U','n','c','h','e','c','k','e','d',0};
    static WCHAR wszIUnknown[] = {'I','U','n','k','n','o','w','n',0};
    static WCHAR wszFont[] = {'F','o','n','t',0};
    static WCHAR wszGUID[] = {'G','U','I','D',0};
    static WCHAR wszStdPicture[] = {'S','t','d','P','i','c','t','u','r','e',0};
    static WCHAR wszOLE_COLOR[] = {'O','L','E','_','C','O','L','O','R',0};

    hr = LoadTypeLib(wszStdOle2, &pTypeLib);
    ok_ole_success(hr, LoadTypeLib);

    hr = ITypeLib_GetTypeComp(pTypeLib, &pTypeComp);
    ok_ole_success(hr, ITypeLib_GetTypeComp);

    /* test getting a TKIND_MODULE */
    ulHash = LHashValOfNameSys(SYS_WIN32, LOCALE_NEUTRAL, wszStdFunctions);
    hr = ITypeComp_Bind(pTypeComp, wszStdFunctions, ulHash, 0, &pTypeInfo, &desckind, &bindptr);
    ok_ole_success(hr, ITypeComp_Bind);

    ok(desckind == DESCKIND_TYPECOMP,
        "desckind should have been DESCKIND_TYPECOMP instead of %d\n",
        desckind);
    ok(!pTypeInfo, "pTypeInfo should have been set to NULL\n");

    ITypeComp_Release(bindptr.lptcomp);

    /* test getting a TKIND_MODULE with INVOKE_PROPERTYGET */
    ulHash = LHashValOfNameSys(SYS_WIN32, LOCALE_NEUTRAL, wszStdFunctions);
    hr = ITypeComp_Bind(pTypeComp, wszStdFunctions, ulHash, INVOKE_PROPERTYGET, &pTypeInfo, &desckind, &bindptr);
    ok_ole_success(hr, ITypeComp_Bind);

    ok(desckind == DESCKIND_TYPECOMP,
        "desckind should have been DESCKIND_TYPECOMP instead of %d\n",
        desckind);
    ok(!pTypeInfo, "pTypeInfo should have been set to NULL\n");

    /* test getting a function within a TKIND_MODULE */
    ulHash = LHashValOfNameSys(SYS_WIN32, LOCALE_NEUTRAL, wszSavePicture);
    hr = ITypeComp_Bind(pTypeComp, wszSavePicture, ulHash, 0, &pTypeInfo, &desckind, &bindptr);
    ok_ole_success(hr, ITypeComp_Bind);

    ok(desckind == DESCKIND_FUNCDESC,
        "desckind should have been DESCKIND_FUNCDESC instead of %d\n",
        desckind);
    ok(bindptr.lpfuncdesc != NULL, "bindptr.lpfuncdesc should not have been set to NULL\n");
    ITypeInfo_ReleaseFuncDesc(pTypeInfo, bindptr.lpfuncdesc);
    ITypeInfo_Release(pTypeInfo);

    /* test getting a function within a TKIND_MODULE with INVOKE_PROPERTYGET */
    ulHash = LHashValOfNameSys(SYS_WIN32, LOCALE_NEUTRAL, wszSavePicture);
    hr = ITypeComp_Bind(pTypeComp, wszSavePicture, ulHash, INVOKE_PROPERTYGET, &pTypeInfo, &desckind, &bindptr);
    todo_wine ok(hr == TYPE_E_TYPEMISMATCH,
        "ITypeComp_Bind should have failed with TYPE_E_TYPEMISMATCH instead of 0x%08lx\n",
        hr);

    ok(desckind == DESCKIND_NONE,
        "desckind should have been DESCKIND_NONE instead of %d\n",
        desckind);
    ok(!pTypeInfo, "pTypeInfo should have been set to NULL\n");
    ok(!bindptr.lptcomp, "bindptr should have been set to NULL\n");

    /* test getting a TKIND_ENUM */
    ulHash = LHashValOfNameSys(SYS_WIN32, LOCALE_NEUTRAL, wszOLE_TRISTATE);
    hr = ITypeComp_Bind(pTypeComp, wszOLE_TRISTATE, ulHash, 0, &pTypeInfo, &desckind, &bindptr);
    ok_ole_success(hr, ITypeComp_Bind);

    ok(desckind == DESCKIND_TYPECOMP,
        "desckind should have been DESCKIND_TYPECOMP instead of %d\n",
        desckind);
    ok(!pTypeInfo, "pTypeInfo should have been set to NULL\n");

    ITypeComp_Release(bindptr.lptcomp);

    /* test getting a value within a TKIND_ENUM */
    ulHash = LHashValOfNameSys(SYS_WIN32, LOCALE_NEUTRAL, wszUnchecked);
    hr = ITypeComp_Bind(pTypeComp, wszUnchecked, ulHash, 0, &pTypeInfo, &desckind, &bindptr);
    ok_ole_success(hr, ITypeComp_Bind);

    ok(desckind == DESCKIND_VARDESC,
        "desckind should have been DESCKIND_VARDESC instead of %d\n",
        desckind);
    ITypeInfo_ReleaseVarDesc(pTypeInfo, bindptr.lpvardesc);
    ITypeInfo_Release(pTypeInfo);

    /* test getting a TKIND_INTERFACE */
    ulHash = LHashValOfNameSys(SYS_WIN32, LOCALE_NEUTRAL, wszIUnknown);
    hr = ITypeComp_Bind(pTypeComp, wszIUnknown, ulHash, 0, &pTypeInfo, &desckind, &bindptr);
    ok_ole_success(hr, ITypeComp_Bind);

    ok(desckind == DESCKIND_NONE,
        "desckind should have been DESCKIND_NONE instead of %d\n",
        desckind);
    ok(!pTypeInfo, "pTypeInfo should have been set to NULL\n");
    ok(!bindptr.lptcomp, "bindptr should have been set to NULL\n");

    /* test getting a TKIND_DISPATCH */
    ulHash = LHashValOfNameSys(SYS_WIN32, LOCALE_NEUTRAL, wszFont);
    hr = ITypeComp_Bind(pTypeComp, wszFont, ulHash, 0, &pTypeInfo, &desckind, &bindptr);
    ok_ole_success(hr, ITypeComp_Bind);

    ok(desckind == DESCKIND_NONE,
        "desckind should have been DESCKIND_NONE instead of %d\n",
        desckind);
    ok(!pTypeInfo, "pTypeInfo should have been set to NULL\n");
    ok(!bindptr.lptcomp, "bindptr should have been set to NULL\n");

    /* test getting a TKIND_RECORD/TKIND_ALIAS */
    ulHash = LHashValOfNameSys(SYS_WIN32, LOCALE_NEUTRAL, wszGUID);
    hr = ITypeComp_Bind(pTypeComp, wszGUID, ulHash, 0, &pTypeInfo, &desckind, &bindptr);
    ok_ole_success(hr, ITypeComp_Bind);

    ok(desckind == DESCKIND_NONE,
        "desckind should have been DESCKIND_NONE instead of %d\n",
        desckind);
    ok(!pTypeInfo, "pTypeInfo should have been set to NULL\n");
    ok(!bindptr.lptcomp, "bindptr should have been set to NULL\n");

    /* test getting a TKIND_ALIAS */
    ulHash = LHashValOfNameSys(SYS_WIN32, LOCALE_NEUTRAL, wszOLE_COLOR);
    hr = ITypeComp_Bind(pTypeComp, wszOLE_COLOR, ulHash, 0, &pTypeInfo, &desckind, &bindptr);
    ok_ole_success(hr, ITypeComp_Bind);

    ok(desckind == DESCKIND_NONE,
        "desckind should have been DESCKIND_NONE instead of %d\n",
        desckind);
    ok(!pTypeInfo, "pTypeInfo should have been set to NULL\n");
    ok(!bindptr.lptcomp, "bindptr should have been set to NULL\n");

    /* test getting a TKIND_COCLASS */
    ulHash = LHashValOfNameSys(SYS_WIN32, LOCALE_NEUTRAL, wszStdPicture);
    hr = ITypeComp_Bind(pTypeComp, wszStdPicture, ulHash, 0, &pTypeInfo, &desckind, &bindptr);
    ok_ole_success(hr, ITypeComp_Bind);

    ok(desckind == DESCKIND_NONE,
        "desckind should have been DESCKIND_NONE instead of %d\n",
        desckind);
    ok(!pTypeInfo, "pTypeInfo should have been set to NULL\n");
    ok(!bindptr.lptcomp, "bindptr should have been set to NULL\n");

    ITypeComp_Release(pTypeComp);
    ITypeLib_Release(pTypeLib);
}

START_TEST(typelib)
{
    static const WCHAR type_lib_stdole32[] = {'s','t','d','o','l','e','3','2','.','t','l','b',0};
    ref_count_test(type_lib_stdole32);
    test_TypeComp();
}
