/*
 * ITypeLib and ITypeInfo test
 *
 * Copyright 2004 Jacek Caban
 * Copyright 2006,2015 Dmitry Timoshkov
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
#define CONST_VTABLE

#include <wine/test.h>
#include <stdarg.h>
#include <stdio.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "oleauto.h"
#include "ocidl.h"
#include "shlwapi.h"
#include "tmarshal.h"
#include "olectl.h"

#include "test_reg.h"
#include "test_tlb.h"
#include "test_simple.h"

#define expect_eq(expr, value, type, format) { type _ret = (expr); ok((value) == _ret, #expr " expected " format " got " format "\n", value, _ret); }
#define expect_int(expr, value) expect_eq(expr, (int)(value), int, "%d")
#define expect_hex(expr, value) expect_eq(expr, (int)(value), int, "0x%x")
#define expect_null(expr) expect_eq(expr, NULL, const void *, "%p")
#define expect_guid(expected, guid) { ok(IsEqualGUID(expected, guid), "got wrong guid %s\n", wine_dbgstr_guid(guid)); }

#define expect_wstr_acpval(expr, value) \
    { \
        CHAR buf[260]; \
        expect_eq(!WideCharToMultiByte(CP_ACP, 0, (expr), -1, buf, 260, NULL, NULL), 0, int, "%d"); \
        ok(value && strcmp(value, buf) == 0, #expr " expected \"%s\" got \"%s\"\n", value, buf); \
    }

#define ole_expect(expr, expect) { \
    HRESULT r = expr; \
    ok(r == (expect), #expr " returned %lx, expected %s (%lx)\n", r, #expect, expect); \
}

#define ole_check(expr) ole_expect(expr, S_OK);

#define ok_ole_success(hr, func) ok(hr == S_OK, #func " failed with error %#08lx\n", hr)

#ifdef __i386__
#define ARCH "x86"
#elif defined __aarch64__ || defined__arm64ec__
#define ARCH "arm64"
#elif defined __x86_64__
#define ARCH "amd64"
#elif defined __arm__
#define ARCH "arm"
#else
#define ARCH "none"
#endif

#define EXPECT_REF(obj,ref) _expect_ref((IUnknown*)obj, ref, __LINE__)
static void _expect_ref(IUnknown* obj, ULONG ref, int line)
{
    ULONG rc;
    IUnknown_AddRef(obj);
    rc = IUnknown_Release(obj);
    ok_(__FILE__,line)(rc == ref, "expected refcount %ld, got %ld\n", ref, rc);
}

static HRESULT (WINAPI *pRegisterTypeLibForUser)(ITypeLib*,OLECHAR*,OLECHAR*);
static HRESULT (WINAPI *pUnRegisterTypeLibForUser)(REFGUID,WORD,WORD,LCID,SYSKIND);

static BOOL   (WINAPI *pIsWow64Process)(HANDLE,LPBOOL);
static LONG   (WINAPI *pRegDeleteKeyExW)(HKEY,LPCWSTR,REGSAM,DWORD);

static const WCHAR wszStdOle2[] = {'s','t','d','o','l','e','2','.','t','l','b',0};
static WCHAR wszGUID[] = {'G','U','I','D',0};
static WCHAR wszguid[] = {'g','u','i','d',0};

static const BOOL is_win64 = sizeof(void *) > sizeof(int);
static BOOL is_wow64;

#ifdef __i386__
static const BOOL abi_supports_stdcall = TRUE;
#else
static const BOOL abi_supports_stdcall = FALSE;
#endif

static BOOL compare_wstr(const WCHAR* wstr, const WCHAR* ref)
{
    char nameA[16];
    WCHAR nameW[16];

    if (!wcscmp(wstr, ref)) return TRUE; /* for UTF code pages */

    WideCharToMultiByte(CP_ACP, 0, ref, -1, nameA, sizeof(nameA), NULL, NULL);
    MultiByteToWideChar(CP_ACP, 0, nameA, -1, nameW, ARRAY_SIZE(nameW));

    return !wcscmp(wstr, nameW);
}

static HRESULT WINAPI collection_QueryInterface(ICollection *iface, REFIID riid, void **ret)
{
    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IDispatch) ||
        IsEqualIID(riid, &IID_ICollection))
    {
        *ret = iface;
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI collection_AddRef(ICollection *iface)
{
    return 2;
}

static ULONG WINAPI collection_Release(ICollection *iface)
{
    return 1;
}

static HRESULT WINAPI collection_GetTypeInfoCount(ICollection *iface, UINT *cnt)
{
    ok(0, "unexpected call\n");
    *cnt = 0;
    return E_NOTIMPL;
}

static HRESULT WINAPI collection_GetTypeInfo(ICollection *iface, UINT index, LCID lcid, ITypeInfo **ti)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI collection_GetIDsOfNames(ICollection *iface, REFIID riid, LPOLESTR *names,
    UINT cnt, LCID lcid, DISPID *dispid)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI collection_Invoke(ICollection *iface, DISPID dispid, REFIID riid,
    LCID lcid, WORD flags, DISPPARAMS *dispparams, VARIANT *res, EXCEPINFO *ei, UINT *argerr)
{
    if(dispid != DISPID_VALUE) {
        ok(0, "unexpected call\n");
        return E_NOTIMPL;
    }

    ok(flags == (DISPATCH_METHOD|DISPATCH_PROPERTYGET), "flags = %x\n", flags);
    ok(dispparams != NULL, "dispparams == NULL\n");
    ok(!dispparams->rgdispidNamedArgs, "dispparams->rgdispidNamedArgs != NULL\n");
    ok(dispparams->cArgs == 1, "dispparams->cArgs = %d\n", dispparams->cArgs);
    ok(!dispparams->cNamedArgs, "dispparams->cNamedArgs = %d\n", dispparams->cNamedArgs);
    ok(V_VT(dispparams->rgvarg) == VT_I4, "V_VT(dispparams->rgvarg) = %d\n", V_VT(dispparams->rgvarg));
    ok(V_I4(dispparams->rgvarg) == 7, "V_I4(dispparams->rgvarg) = %ld\n", V_I4(dispparams->rgvarg));
    ok(res != NULL, "res == NULL\n");
    ok(V_VT(res) == VT_EMPTY, "V_VT(res) = %d\n", V_VT(res));

    V_VT(res) = VT_I4;
    V_I4(res) = 15;
    return S_OK;
}

static HRESULT WINAPI collection_Item(ICollection *iface, int i, int *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const ICollectionVtbl collectionvtbl = {
    collection_QueryInterface,
    collection_AddRef,
    collection_Release,
    collection_GetTypeInfoCount,
    collection_GetTypeInfo,
    collection_GetIDsOfNames,
    collection_Invoke,
    collection_Item
};

static ICollection collection = { &collectionvtbl };

static HRESULT WINAPI invoketest_QueryInterface(IInvokeTest *iface, REFIID riid, void **ret)
{
    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IDispatch) ||
        IsEqualIID(riid, &IID_IInvokeTest))
    {
        *ret = iface;
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI invoketest_AddRef(IInvokeTest *iface)
{
    return 2;
}

static ULONG WINAPI invoketest_Release(IInvokeTest *iface)
{
    return 1;
}

static HRESULT WINAPI invoketest_GetTypeInfoCount(IInvokeTest *iface, UINT *cnt)
{
    ok(0, "unexpected call\n");
    *cnt = 0;
    return E_NOTIMPL;
}

static HRESULT WINAPI invoketest_GetTypeInfo(IInvokeTest *iface, UINT index, LCID lcid, ITypeInfo **ti)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI invoketest_GetIDsOfNames(IInvokeTest *iface, REFIID riid, LPOLESTR *names,
    UINT cnt, LCID lcid, DISPID *dispid)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI invoketest_Invoke(IInvokeTest *iface, DISPID dispid, REFIID riid,
    LCID lcid, WORD flags, DISPPARAMS *dispparams, VARIANT *res, EXCEPINFO *ei, UINT *argerr)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static LONG WINAPI invoketest_get_test(IInvokeTest *iface, LONG i)
{
    return i+1;
}

static LONG WINAPI invoketest_putref_testprop(IInvokeTest *iface, LONG *i)
{
    return *i+2;
}

static LONG WINAPI invoketest_putref_testprop2(IInvokeTest *iface, IUnknown *i)
{
    return 6;
}

static HRESULT WINAPI invoketest_testfunc(IInvokeTest *iface, int i, int *p)
{
    *p = i+1;
    return S_OK;
}

static HRESULT WINAPI invoketest_testget(IInvokeTest *iface, ICollection **p)
{
    *p = &collection;
    ICollection_AddRef(&collection);
    return S_OK;
}

static LONG WINAPI invoketest_get_testget2(IInvokeTest *iface, int *value)
{
    *value = 4;
    return S_OK;
}

static int WINAPI invoketest_get_testget3(IInvokeTest *iface)
{
    return 4;
}

static int WINAPI invoketest_get_testget4(IInvokeTest *iface, int value)
{
    return 4;
}

static int WINAPI invoketest_get_testget5(IInvokeTest *iface, int *value1, int value2)
{
    return 4;
}

static int WINAPI invoketest_get_testget6(IInvokeTest *iface, int value1, int value2)
{
    return 4;
}

static void WINAPI invoketest_get_testget7(IInvokeTest *iface, int value1, int value2)
{
}

static void WINAPI invoketest_get_testget8(IInvokeTest *iface, int* value)
{
}

static void WINAPI invoketest_get_testput8(IInvokeTest *iface, int value)
{
}

static const IInvokeTestVtbl invoketestvtbl = {
    invoketest_QueryInterface,
    invoketest_AddRef,
    invoketest_Release,
    invoketest_GetTypeInfoCount,
    invoketest_GetTypeInfo,
    invoketest_GetIDsOfNames,
    invoketest_Invoke,
    invoketest_get_test,
    invoketest_putref_testprop,
    invoketest_putref_testprop2,
    invoketest_testfunc,
    invoketest_testget,
    invoketest_get_testget2,
    invoketest_get_testget3,
    invoketest_get_testget4,
    invoketest_get_testget5,
    invoketest_get_testget6,
    invoketest_get_testget7,
    invoketest_get_testget8,
    invoketest_get_testput8,
};

static IInvokeTest invoketest = { &invoketestvtbl };

static void init_function_pointers(void)
{
    HMODULE hmod = GetModuleHandleA("oleaut32.dll");
    HMODULE hk32 = GetModuleHandleA("kernel32.dll");
    HMODULE hadv = GetModuleHandleA("advapi32.dll");

    pRegisterTypeLibForUser = (void *)GetProcAddress(hmod, "RegisterTypeLibForUser");
    pUnRegisterTypeLibForUser = (void *)GetProcAddress(hmod, "UnRegisterTypeLibForUser");
    pIsWow64Process = (void *)GetProcAddress(hk32, "IsWow64Process");
    pRegDeleteKeyExW = (void*)GetProcAddress(hadv, "RegDeleteKeyExW");
}

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
    ref_count = ITypeLib_Release(iface);
    ok(ref_count > 0, "ITypeLib destroyed while ITypeInfo has back pointer\n");
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
    ITypeComp *pTypeComp, *tcomp, *pTypeComp_tmp;
    ITypeInfo *pTypeInfo, *ti, *pFontTypeInfo;
    ITypeLib *pTypeLib;
    HRESULT hr;
    ULONG ulHash;
    DESCKIND desckind;
    BINDPTR bindptr;
    static WCHAR wszStdFunctions[] = {'S','t','d','F','u','n','c','t','i','o','n','s',0};
    static WCHAR wszSavePicture[] = {'S','a','v','e','P','i','c','t','u','r','e',0};
    static WCHAR wszOLE_TRISTATE[] = {'O','L','E','_','T','R','I','S','T','A','T','E',0};
    static WCHAR wszUnchecked[] = {'U','n','c','h','e','c','k','e','d',0};
    static WCHAR wszIUnknown[] = {'I','U','n','k','n','o','w','n',0};
    static WCHAR wszFont[] = {'F','o','n','t',0};
    static WCHAR wszStdPicture[] = {'S','t','d','P','i','c','t','u','r','e',0};
    static WCHAR wszOLE_COLOR[] = {'O','L','E','_','C','O','L','O','R',0};
    static WCHAR wszClone[] = {'C','l','o','n','e',0};
    static WCHAR wszclone[] = {'c','l','o','n','e',0};
    static WCHAR wszJunk[] = {'J','u','n','k',0};
    static WCHAR wszAddRef[] = {'A','d','d','R','e','f',0};

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
    ITypeComp_Release(bindptr.lptcomp);

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
    ok(hr == TYPE_E_TYPEMISMATCH,
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

    /* test basic BindType argument handling */
    ulHash = LHashValOfNameSys(SYS_WIN32, LOCALE_NEUTRAL, wszGUID);
    hr = ITypeComp_BindType(pTypeComp, wszGUID, ulHash, NULL, NULL);
    ok(hr == E_INVALIDARG, "Got %08lx\n", hr);

    ulHash = LHashValOfNameSys(SYS_WIN32, LOCALE_NEUTRAL, wszGUID);
    pTypeInfo = (void*)0xdeadbeef;
    hr = ITypeComp_BindType(pTypeComp, wszGUID, ulHash, &pTypeInfo, NULL);
    ok(hr == E_INVALIDARG, "Got %08lx\n", hr);
    ok(pTypeInfo == (void*)0xdeadbeef, "Got %p\n", pTypeInfo);

    ulHash = LHashValOfNameSys(SYS_WIN32, LOCALE_NEUTRAL, wszGUID);
    pTypeComp_tmp = (void*)0xdeadbeef;
    hr = ITypeComp_BindType(pTypeComp, wszGUID, ulHash, NULL, &pTypeComp_tmp);
    ok(hr == E_INVALIDARG, "Got %08lx\n", hr);
    ok(pTypeComp_tmp == (void*)0xdeadbeef, "Got %p\n", pTypeComp_tmp);

    ulHash = LHashValOfNameSys(SYS_WIN32, LOCALE_NEUTRAL, wszGUID);
    pTypeComp_tmp = (void*)0xdeadbeef;
    pTypeInfo = (void*)0xdeadbeef;
    hr = ITypeComp_BindType(pTypeComp, NULL, ulHash, &pTypeInfo, &pTypeComp_tmp);
    ok(hr == E_INVALIDARG, "Got %08lx\n", hr);
    ok(pTypeInfo == (void*)0xdeadbeef, "Got %p\n", pTypeInfo);
    ok(pTypeComp_tmp == (void*)0xdeadbeef, "Got %p\n", pTypeComp_tmp);

    ulHash = LHashValOfNameSys(SYS_WIN32, LOCALE_NEUTRAL, wszGUID);
    pTypeComp_tmp = (void*)0xdeadbeef;
    pTypeInfo = (void*)0xdeadbeef;
    hr = ITypeComp_BindType(pTypeComp, wszGUID, ulHash, &pTypeInfo, &pTypeComp_tmp);
    ok_ole_success(hr, ITypeComp_BindType);
    ok(pTypeInfo != NULL, "Got NULL pTypeInfo\n");
    todo_wine ok(pTypeComp_tmp == NULL, "Got pTypeComp_tmp %p\n", pTypeComp_tmp);
    ITypeInfo_Release(pTypeInfo);
    if(pTypeComp_tmp) ITypeComp_Release(pTypeComp_tmp); /* fixme */

    /* test BindType case-insensitivity */
    ulHash = LHashValOfNameSys(SYS_WIN32, LOCALE_NEUTRAL, wszguid);
    pTypeComp_tmp = (void*)0xdeadbeef;
    pTypeInfo = (void*)0xdeadbeef;
    hr = ITypeComp_BindType(pTypeComp, wszguid, ulHash, &pTypeInfo, &pTypeComp_tmp);
    ok_ole_success(hr, ITypeComp_BindType);
    ok(pTypeInfo != NULL, "Got NULL pTypeInfo\n");
    todo_wine ok(pTypeComp_tmp == NULL, "Got pTypeComp_tmp %p\n", pTypeComp_tmp);
    ITypeInfo_Release(pTypeInfo);
    if(pTypeComp_tmp) ITypeComp_Release(pTypeComp_tmp); /* fixme */

    ITypeComp_Release(pTypeComp);

    /* tests for ITypeComp on an interface */
    hr = ITypeLib_GetTypeInfoOfGuid(pTypeLib, &IID_IFont, &pFontTypeInfo);
    ok_ole_success(hr, ITypeLib_GetTypeInfoOfGuid);

    hr = ITypeInfo_GetTypeComp(pFontTypeInfo, &pTypeComp);
    ok_ole_success(hr, ITypeLib_GetTypeComp);

    hr = ITypeInfo_QueryInterface(pFontTypeInfo, &IID_ITypeComp, (void**)&tcomp);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(tcomp == pTypeComp, "got %p, was %p\n", tcomp, pTypeComp);

    hr = ITypeComp_QueryInterface(tcomp, &IID_ITypeInfo, (void**)&ti);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(ti == pFontTypeInfo, "got %p, was %p\n", ti, pFontTypeInfo);
    ITypeInfo_Release(ti);

    ITypeComp_Release(tcomp);

    ulHash = LHashValOfNameSys(SYS_WIN32, LOCALE_NEUTRAL, wszClone);
    hr = ITypeComp_Bind(pTypeComp, wszClone, ulHash, 0, &pTypeInfo, &desckind, &bindptr);
    ok_ole_success(hr, ITypeComp_Bind);

    ok(desckind == DESCKIND_FUNCDESC,
        "desckind should have been DESCKIND_FUNCDESC instead of %d\n",
        desckind);
    ok(bindptr.lpfuncdesc != NULL, "bindptr.lpfuncdesc should not have been set to NULL\n");
    ITypeInfo_ReleaseFuncDesc(pTypeInfo, bindptr.lpfuncdesc);
    ITypeInfo_Release(pTypeInfo);

    ulHash = LHashValOfNameSys(SYS_WIN32, LOCALE_NEUTRAL, wszClone);
    hr = ITypeComp_Bind(pTypeComp, wszClone, ulHash, INVOKE_PROPERTYGET, &pTypeInfo, &desckind, &bindptr);
    ok(hr == TYPE_E_TYPEMISMATCH, "ITypeComp_Bind should have failed with TYPE_E_TYPEMISMATCH instead of 0x%08lx\n", hr);

    ok(desckind == DESCKIND_NONE,
        "desckind should have been DESCKIND_NONE instead of %d\n",
        desckind);
    ok(!pTypeInfo, "pTypeInfo should have been set to NULL\n");
    ok(!bindptr.lptcomp, "bindptr should have been set to NULL\n");

    /* tests that the compare is case-insensitive */
    ulHash = LHashValOfNameSys(SYS_WIN32, LOCALE_NEUTRAL, wszclone);
    hr = ITypeComp_Bind(pTypeComp, wszclone, ulHash, 0, &pTypeInfo, &desckind, &bindptr);
    ok_ole_success(hr, ITypeComp_Bind);

    ok(desckind == DESCKIND_FUNCDESC,
        "desckind should have been DESCKIND_FUNCDESC instead of %d\n",
        desckind);
    ok(bindptr.lpfuncdesc != NULL, "bindptr.lpfuncdesc should not have been set to NULL\n");
    ITypeInfo_ReleaseFuncDesc(pTypeInfo, bindptr.lpfuncdesc);
    ITypeInfo_Release(pTypeInfo);

    /* tests nonexistent members */
    desckind = 0xdeadbeef;
    bindptr.lptcomp = (ITypeComp*)0xdeadbeef;
    pTypeInfo = (ITypeInfo*)0xdeadbeef;
    ulHash = LHashValOfNameSys(SYS_WIN32, LOCALE_NEUTRAL, wszJunk);
    hr = ITypeComp_Bind(pTypeComp, wszJunk, ulHash, 0, &pTypeInfo, &desckind, &bindptr);
    ok_ole_success(hr, ITypeComp_Bind);
    ok(desckind == DESCKIND_NONE, "desckind should have been DESCKIND_NONE, was: %d\n", desckind);
    ok(pTypeInfo == NULL, "pTypeInfo should have been NULL, was: %p\n", pTypeInfo);
    ok(bindptr.lptcomp == NULL, "bindptr should have been NULL, was: %p\n", bindptr.lptcomp);

    /* tests inherited members */
    desckind = 0xdeadbeef;
    bindptr.lpfuncdesc = NULL;
    pTypeInfo = NULL;
    ulHash = LHashValOfNameSys(SYS_WIN32, LOCALE_NEUTRAL, wszAddRef);
    hr = ITypeComp_Bind(pTypeComp, wszAddRef, ulHash, 0, &pTypeInfo, &desckind, &bindptr);
    ok_ole_success(hr, ITypeComp_Bind);
    ok(desckind == DESCKIND_FUNCDESC, "desckind should have been DESCKIND_FUNCDESC, was: %d\n", desckind);
    ok(pTypeInfo != NULL, "pTypeInfo should not have been NULL, was: %p\n", pTypeInfo);
    ok(bindptr.lpfuncdesc != NULL, "bindptr should not have been NULL, was: %p\n", bindptr.lpfuncdesc);
    ITypeInfo_ReleaseFuncDesc(pTypeInfo, bindptr.lpfuncdesc);
    ITypeInfo_Release(pTypeInfo);

    ITypeComp_Release(pTypeComp);
    ITypeInfo_Release(pFontTypeInfo);
    ITypeLib_Release(pTypeLib);
}

static void test_CreateDispTypeInfo(void)
{
    ITypeInfo *pTypeInfo, *pTI2;
    HRESULT hr;
    INTERFACEDATA ifdata;
    METHODDATA methdata[4];
    PARAMDATA parms1[2];
    PARAMDATA parms3[1];
    TYPEATTR *pTypeAttr;
    HREFTYPE href;
    FUNCDESC *pFuncDesc;
    MEMBERID memid;

    static WCHAR func1[] = {'f','u','n','c','1',0};
    static const WCHAR func2[] = {'f','u','n','c','2',0};
    static const WCHAR func3[] = {'f','u','n','c','3',0};
    static const WCHAR parm1[] = {'p','a','r','m','1',0};
    static const WCHAR parm2[] = {'p','a','r','m','2',0};
    OLECHAR *name = func1;

    ifdata.pmethdata = methdata;
    ifdata.cMembers = ARRAY_SIZE(methdata);

    methdata[0].szName = SysAllocString(func1);
    methdata[0].ppdata = parms1;
    methdata[0].dispid = 0x123;
    methdata[0].iMeth = 0;
    methdata[0].cc = CC_STDCALL;
    methdata[0].cArgs = 2;
    methdata[0].wFlags = DISPATCH_METHOD;
    methdata[0].vtReturn = VT_HRESULT;
    parms1[0].szName = SysAllocString(parm1);
    parms1[0].vt = VT_I4;
    parms1[1].szName = SysAllocString(parm2);
    parms1[1].vt = VT_BSTR;

    methdata[1].szName = SysAllocString(func2);
    methdata[1].ppdata = NULL;
    methdata[1].dispid = 0x124;
    methdata[1].iMeth = 1;
    methdata[1].cc = CC_STDCALL;
    methdata[1].cArgs = 0;
    methdata[1].wFlags = DISPATCH_PROPERTYGET;
    methdata[1].vtReturn = VT_I4;

    methdata[2].szName = SysAllocString(func3);
    methdata[2].ppdata = parms3;
    methdata[2].dispid = 0x125;
    methdata[2].iMeth = 3;
    methdata[2].cc = CC_STDCALL;
    methdata[2].cArgs = 1;
    methdata[2].wFlags = DISPATCH_PROPERTYPUT;
    methdata[2].vtReturn = VT_HRESULT;
    parms3[0].szName = SysAllocString(parm1);
    parms3[0].vt = VT_I4;

    methdata[3].szName = SysAllocString(func3);
    methdata[3].ppdata = NULL;
    methdata[3].dispid = 0x125;
    methdata[3].iMeth = 4;
    methdata[3].cc = CC_STDCALL;
    methdata[3].cArgs = 0;
    methdata[3].wFlags = DISPATCH_PROPERTYGET;
    methdata[3].vtReturn = VT_I4;

    hr = CreateDispTypeInfo(&ifdata, LOCALE_NEUTRAL, &pTypeInfo);
    ok(hr == S_OK, "hr %08lx\n", hr);

    hr = ITypeInfo_GetTypeAttr(pTypeInfo, &pTypeAttr);
    ok(hr == S_OK, "hr %08lx\n", hr);

    ok(pTypeAttr->typekind == TKIND_COCLASS, "typekind %0x\n", pTypeAttr->typekind);
    ok(pTypeAttr->cImplTypes == 1, "cImplTypes %d\n", pTypeAttr->cImplTypes);
    ok(pTypeAttr->cFuncs == 0, "cFuncs %d\n", pTypeAttr->cFuncs);
    ok(pTypeAttr->wTypeFlags == 0, "wTypeFlags %04x\n", pTypeAttr->cFuncs);
    ITypeInfo_ReleaseTypeAttr(pTypeInfo, pTypeAttr);

    hr = ITypeInfo_GetRefTypeOfImplType(pTypeInfo, 0, &href);
    ok(hr == S_OK, "hr %08lx\n", hr);
    ok(href == 0, "href = 0x%lx\n", href);
    hr = ITypeInfo_GetRefTypeInfo(pTypeInfo, href, &pTI2);
    ok(hr == S_OK, "hr %08lx\n", hr);
    hr = ITypeInfo_GetTypeAttr(pTI2, &pTypeAttr);
    ok(hr == S_OK, "hr %08lx\n", hr);
    ok(pTypeAttr->typekind == TKIND_INTERFACE, "typekind %0x\n", pTypeAttr->typekind);
    ok(pTypeAttr->cFuncs == 4, "cFuncs %d\n", pTypeAttr->cFuncs);
    ok(IsEqualGUID(&pTypeAttr->guid, &GUID_NULL), "guid %s\n", debugstr_guid(&pTypeAttr->guid));
    ok(pTypeAttr->wTypeFlags == 0, "typeflags %08x\n", pTypeAttr->wTypeFlags);

    ITypeInfo_ReleaseTypeAttr(pTI2, pTypeAttr);

    hr = ITypeInfo_GetFuncDesc(pTI2, 0, &pFuncDesc);
    ok(hr == S_OK, "hr %08lx\n", hr);
    ok(pFuncDesc->memid == 0x123, "memid %lx\n", pFuncDesc->memid);
    ok(pFuncDesc->funckind == FUNC_VIRTUAL, "funckind %d\n", pFuncDesc->funckind);
    ok(pFuncDesc->invkind == methdata[0].wFlags, "invkind %d\n", pFuncDesc->invkind);
    ok(pFuncDesc->callconv == methdata[0].cc, "callconv %d\n", pFuncDesc->callconv);
    ok(pFuncDesc->cParams == methdata[0].cArgs, "cParams %d\n", pFuncDesc->cParams);
    ok(pFuncDesc->oVft == 0, "oVft %d\n", pFuncDesc->oVft);
    ok(pFuncDesc->wFuncFlags == 0, "oVft %d\n", pFuncDesc->wFuncFlags);
    ok(pFuncDesc->elemdescFunc.tdesc.vt == VT_HRESULT, "ret vt %x\n", pFuncDesc->elemdescFunc.tdesc.vt);
    ok(pFuncDesc->lprgelemdescParam[0].tdesc.vt == VT_I4, "parm 0 vt %x\n", pFuncDesc->lprgelemdescParam[0].tdesc.vt);
    ok(pFuncDesc->lprgelemdescParam[0].paramdesc.wParamFlags == PARAMFLAG_NONE, "parm 0 flags %x\n", pFuncDesc->lprgelemdescParam[0].paramdesc.wParamFlags);

    ok(pFuncDesc->lprgelemdescParam[1].tdesc.vt == VT_BSTR, "parm 1 vt %x\n", pFuncDesc->lprgelemdescParam[1].tdesc.vt);
    ok(pFuncDesc->lprgelemdescParam[1].paramdesc.wParamFlags == PARAMFLAG_NONE, "parm 1 flags %x\n", pFuncDesc->lprgelemdescParam[1].paramdesc.wParamFlags);
    ITypeInfo_ReleaseFuncDesc(pTI2, pFuncDesc);

    hr = ITypeInfo_GetFuncDesc(pTI2, 1, &pFuncDesc);
    ok(hr == S_OK, "hr %08lx\n", hr);
    ok(pFuncDesc->funckind == FUNC_VIRTUAL, "funckind %d\n", pFuncDesc->funckind);
    ok(pFuncDesc->invkind == methdata[1].wFlags, "invkind %d\n", pFuncDesc->invkind);
    ok(pFuncDesc->callconv == methdata[1].cc, "callconv %d\n", pFuncDesc->callconv);
    ok(pFuncDesc->cParams == methdata[1].cArgs, "cParams %d\n", pFuncDesc->cParams);
    ok(pFuncDesc->oVft == sizeof(void *), "oVft %d\n", pFuncDesc->oVft);
    ok(pFuncDesc->wFuncFlags == 0, "oVft %d\n", pFuncDesc->wFuncFlags);
    ok(pFuncDesc->elemdescFunc.tdesc.vt == VT_I4, "ret vt %x\n", pFuncDesc->elemdescFunc.tdesc.vt);
    ITypeInfo_ReleaseFuncDesc(pTI2, pFuncDesc);

    hr = ITypeInfo_GetFuncDesc(pTI2, 2, &pFuncDesc);
    ok(hr == S_OK, "hr %08lx\n", hr);
    ok(pFuncDesc->funckind == FUNC_VIRTUAL, "funckind %d\n", pFuncDesc->funckind);
    ok(pFuncDesc->invkind == methdata[2].wFlags, "invkind %d\n", pFuncDesc->invkind);
    ok(pFuncDesc->callconv == methdata[2].cc, "callconv %d\n", pFuncDesc->callconv);
    ok(pFuncDesc->cParams == methdata[2].cArgs, "cParams %d\n", pFuncDesc->cParams);
    ok(pFuncDesc->oVft == 3 * sizeof(void *), "oVft %d\n", pFuncDesc->oVft);
    ok(pFuncDesc->wFuncFlags == 0, "oVft %d\n", pFuncDesc->wFuncFlags);
    ok(pFuncDesc->elemdescFunc.tdesc.vt == VT_HRESULT, "ret vt %x\n", pFuncDesc->elemdescFunc.tdesc.vt);
    ok(pFuncDesc->lprgelemdescParam[0].tdesc.vt == VT_I4, "parm 0 vt %x\n", pFuncDesc->lprgelemdescParam[0].tdesc.vt);
    ok(pFuncDesc->lprgelemdescParam[0].paramdesc.wParamFlags == PARAMFLAG_NONE, "parm 0 flags %x\n", pFuncDesc->lprgelemdescParam[0].paramdesc.wParamFlags);
    ITypeInfo_ReleaseFuncDesc(pTI2, pFuncDesc);

    hr = ITypeInfo_GetFuncDesc(pTI2, 3, &pFuncDesc);
    ok(hr == S_OK, "hr %08lx\n", hr);
    ok(pFuncDesc->funckind == FUNC_VIRTUAL, "funckind %d\n", pFuncDesc->funckind);
    ok(pFuncDesc->invkind == methdata[3].wFlags, "invkind %d\n", pFuncDesc->invkind);
    ok(pFuncDesc->callconv == methdata[3].cc, "callconv %d\n", pFuncDesc->callconv);
    ok(pFuncDesc->cParams == methdata[3].cArgs, "cParams %d\n", pFuncDesc->cParams);
    ok(pFuncDesc->oVft == 4 * sizeof(void *), "oVft %d\n", pFuncDesc->oVft);
    ok(pFuncDesc->wFuncFlags == 0, "oVft %d\n", pFuncDesc->wFuncFlags);
    ok(pFuncDesc->elemdescFunc.tdesc.vt == VT_I4, "ret vt %x\n", pFuncDesc->elemdescFunc.tdesc.vt);
    ITypeInfo_ReleaseFuncDesc(pTI2, pFuncDesc);

    /* test GetIDsOfNames on a coclass to see if it searches its interfaces */
    hr = ITypeInfo_GetIDsOfNames(pTypeInfo, &name, 1, &memid);
    ok(hr == S_OK, "hr 0x%08lx\n", hr);
    ok(memid == 0x123, "memid 0x%08lx\n", memid);

    ITypeInfo_Release(pTI2);
    ITypeInfo_Release(pTypeInfo);

    SysFreeString(parms1[0].szName);
    SysFreeString(parms1[1].szName);
    SysFreeString(parms3[0].szName);
    SysFreeString(methdata[0].szName);
    SysFreeString(methdata[1].szName);
    SysFreeString(methdata[2].szName);
    SysFreeString(methdata[3].szName);
}

static void write_typelib(int res_no, const WCHAR *filename)
{
    DWORD written;
    HANDLE file;
    HRSRC res;
    void *ptr;

    file = CreateFileW( filename, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    ok( file != INVALID_HANDLE_VALUE, "file creation failed\n" );
    if (file == INVALID_HANDLE_VALUE) return;
    res = FindResourceW( GetModuleHandleA(NULL), (const WCHAR *)MAKEINTRESOURCE(res_no), L"TYPELIB" );
    ok( res != 0, "couldn't find typelib resource %d\n", res_no );
    ptr = LockResource( LoadResource( GetModuleHandleA(NULL), res ));
    WriteFile( file, ptr, SizeofResource( GetModuleHandleA(NULL), res ), &written, NULL );
    ok( written == SizeofResource( GetModuleHandleA(NULL), res ), "couldn't write resource\n" );
    CloseHandle( file );
}

static void test_invoke_func(ITypeInfo *typeinfo)
{
    DISPID named_args[3] = { DISPID_THIS };
    VARIANT args[3], res;
    DISPPARAMS dp = {args, named_args, 1, 0};
    UINT i;
    HRESULT hres;

    V_VT(args) = VT_INT;
    V_INT(args) = 3;
    V_VT(&res) = VT_ERROR;
    hres = ITypeInfo_Invoke(typeinfo, &invoketest, 3, DISPATCH_METHOD, &dp, &res, NULL, &i);
    ok(hres == S_OK, "got 0x%08lx\n", hres);
    ok(V_VT(&res) == VT_I4, "got %d\n", V_VT(&res));
    ok(V_I4(&res) == 4, "got %ld\n", V_I4(&res));

    V_VT(args) = VT_DISPATCH;
    V_DISPATCH(args) = (IDispatch*)&invoketest;
    V_VT(args+1) = VT_INT;
    V_INT(args+1) = 3;
    V_VT(&res) = VT_ERROR;
    dp.cNamedArgs = 1;
    dp.cArgs = 2;
    hres = ITypeInfo_Invoke(typeinfo, &invoketest, 3, DISPATCH_METHOD, &dp, &res, NULL, &i);
    ok(hres == DISP_E_BADPARAMCOUNT, "got 0x%08lx\n", hres);
}

static WCHAR *create_test_typelib(int res_no)
{
    static WCHAR filename[MAX_PATH];

    GetTempFileNameW(L".", L"tlb", 0, filename);
    write_typelib(res_no, filename);
    return filename;
}

static void test_TypeInfo(void)
{
    ITypeLib *pTypeLib;
    ITypeInfo *pTypeInfo, *ti;
    ITypeInfo2 *pTypeInfo2;
    HRESULT hr;
    static WCHAR wszBogus[] = { 'b','o','g','u','s',0 };
    static WCHAR wszGetTypeInfo[] = { 'G','e','t','T','y','p','e','I','n','f','o',0 };
    static WCHAR wszClone[] = {'C','l','o','n','e',0};
    static WCHAR wszTestDll[] = {'t','e','s','t','.','d','l','l',0};
    OLECHAR* bogus = wszBogus;
    OLECHAR* pwszGetTypeInfo = wszGetTypeInfo;
    OLECHAR* pwszClone = wszClone;
    DISPID dispidMember;
    DISPPARAMS dispparams;
    GUID bogusguid = {0x806afb4f,0x13f7,0x42d2,{0x89,0x2c,0x6c,0x97,0xc3,0x6a,0x36,0xc1}};
    static const GUID moduleTestGetDllEntryGuid = {0xf073cd92,0xa199,0x11ea,{0xbb,0x37,0x02,0x42,0xac,0x13,0x00,0x02}};
    VARIANT var, res, args[3];
    UINT count, i;
    TYPEKIND kind;
    const WCHAR *filename;
    TYPEATTR *attr;
    LONG l;
    WORD ordinal;
    BSTR bstrDllName, bstrName;

    hr = LoadTypeLib(wszStdOle2, &pTypeLib);
    ok_ole_success(hr, LoadTypeLib);

    count = ITypeLib_GetTypeInfoCount(pTypeLib);
    ok(count > 0, "got %d\n", count);

    /* invalid index */
    hr = ITypeLib_GetTypeInfo(pTypeLib, count, &pTypeInfo);
    ok(hr == TYPE_E_ELEMENTNOTFOUND, "got 0x%08lx\n", hr);

    hr = ITypeLib_GetTypeInfo(pTypeLib, 0, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

    hr = ITypeLib_GetLibAttr(pTypeLib, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

    hr = ITypeLib_GetTypeInfoType(pTypeLib, count, &kind);
    ok(hr == TYPE_E_ELEMENTNOTFOUND, "got 0x%08lx\n", hr);

    hr = ITypeLib_GetTypeInfoType(pTypeLib, count, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

    hr = ITypeLib_GetTypeInfoType(pTypeLib, 0, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

    hr = ITypeLib_GetTypeInfoOfGuid(pTypeLib, &IID_IFont, &pTypeInfo);
    ok_ole_success(hr, ITypeLib_GetTypeInfoOfGuid); 

    /* test nonexistent method name */
    hr = ITypeInfo_GetIDsOfNames(pTypeInfo, &bogus, 1, &dispidMember);
    ok(hr == DISP_E_UNKNOWNNAME,
       "ITypeInfo_GetIDsOfNames should have returned DISP_E_UNKNOWNNAME instead of 0x%08lx\n",
       hr);

    dispparams.cArgs = 0;
    dispparams.rgdispidNamedArgs = NULL;
    dispparams.rgvarg = NULL;

            /* test dispparams not NULL */

    /* invalid member id -- wrong flags -- cNamedArgs not bigger than cArgs */
    dispparams.cNamedArgs = 0;
    hr = ITypeInfo_Invoke(pTypeInfo, (void *)0xdeadbeef, 0xdeadbeef, DISPATCH_PROPERTYGET, &dispparams, NULL, NULL, NULL);
    ok(hr == DISP_E_MEMBERNOTFOUND, "ITypeInfo_Invoke should have returned DISP_E_MEMBERNOTFOUND instead of 0x%08lx\n", hr);
    /* invalid member id -- correct flags -- cNamedArgs not bigger than cArgs */
    hr = ITypeInfo_Invoke(pTypeInfo, (void *)0xdeadbeef, 0xdeadbeef, DISPATCH_METHOD, &dispparams, NULL, NULL, NULL);
    ok(hr == DISP_E_MEMBERNOTFOUND, "ITypeInfo_Invoke should have returned DISP_E_MEMBERNOTFOUND instead of 0x%08lx\n", hr);

    /* invalid member id -- wrong flags -- cNamedArgs bigger than cArgs */
    dispparams.cNamedArgs = 1;
    hr = ITypeInfo_Invoke(pTypeInfo, (void *)0xdeadbeef, 0xdeadbeef, DISPATCH_PROPERTYGET, &dispparams, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "ITypeInfo_Invoke should have returned E_INVALIDARG instead of 0x%08lx\n", hr);
    /* invalid member id -- correct flags -- cNamedArgs bigger than cArgs */
    hr = ITypeInfo_Invoke(pTypeInfo, (void *)0xdeadbeef, 0xdeadbeef, DISPATCH_METHOD, &dispparams, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "ITypeInfo_Invoke should have returned E_INVALIDARG instead of 0x%08lx\n", hr);


    hr = ITypeInfo_GetIDsOfNames(pTypeInfo, &pwszClone, 1, &dispidMember);
    ok_ole_success(hr, ITypeInfo_GetIDsOfNames);

    /* correct member id -- wrong flags -- cNamedArgs not bigger than cArgs */
    dispparams.cNamedArgs = 0;
    hr = ITypeInfo_Invoke(pTypeInfo, (void *)0xdeadbeef, dispidMember, DISPATCH_PROPERTYGET, &dispparams, NULL, NULL, NULL);
    ok(hr == DISP_E_MEMBERNOTFOUND, "ITypeInfo_Invoke should have returned DISP_E_MEMBERNOTFOUND instead of 0x%08lx\n", hr);
    /* correct member id -- correct flags -- cNamedArgs not bigger than cArgs */
    hr = ITypeInfo_Invoke(pTypeInfo, (void *)0xdeadbeef, dispidMember, DISPATCH_METHOD, &dispparams, NULL, NULL, NULL);
    ok(hr == DISP_E_BADPARAMCOUNT, "ITypeInfo_Invoke should have returned DISP_E_BADPARAMCOUNT instead of 0x%08lx\n", hr);

    /* correct member id -- wrong flags -- cNamedArgs bigger than cArgs */
    dispparams.cNamedArgs = 1;
    hr = ITypeInfo_Invoke(pTypeInfo, (void *)0xdeadbeef, dispidMember, DISPATCH_PROPERTYGET, &dispparams, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "ITypeInfo_Invoke should have returned E_INVALIDARG instead of 0x%08lx\n", hr);
    /* correct member id -- correct flags -- cNamedArgs bigger than cArgs */
    hr = ITypeInfo_Invoke(pTypeInfo, (void *)0xdeadbeef, dispidMember, DISPATCH_METHOD, &dispparams, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "ITypeInfo_Invoke should have returned E_INVALIDARG instead of 0x%08lx\n", hr);

            /* test NULL dispparams */

    /* correct member id -- wrong flags -- cNamedArgs not bigger than cArgs */
    dispparams.cNamedArgs = 0;
    hr = ITypeInfo_Invoke(pTypeInfo, (void *)0xdeadbeef, dispidMember, DISPATCH_PROPERTYGET, NULL, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "ITypeInfo_Invoke should have returned E_INVALIDARG instead of 0x%08lx\n", hr);
    /* correct member id -- correct flags -- cNamedArgs not bigger than cArgs */
    hr = ITypeInfo_Invoke(pTypeInfo, (void *)0xdeadbeef, dispidMember, DISPATCH_METHOD, NULL, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "ITypeInfo_Invoke should have returned E_INVALIDARG instead of 0x%08lx\n", hr);

    /* correct member id -- wrong flags -- cNamedArgs bigger than cArgs */
    dispparams.cNamedArgs = 1;
    hr = ITypeInfo_Invoke(pTypeInfo, (void *)0xdeadbeef, dispidMember, DISPATCH_PROPERTYGET, NULL, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "ITypeInfo_Invoke should have returned E_INVALIDARG instead of 0x%08lx\n", hr);
    /* correct member id -- correct flags -- cNamedArgs bigger than cArgs */
    hr = ITypeInfo_Invoke(pTypeInfo, (void *)0xdeadbeef, dispidMember, DISPATCH_METHOD, NULL, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "ITypeInfo_Invoke should have returned E_INVALIDARG instead of 0x%08lx\n", hr);

    ITypeInfo_Release(pTypeInfo);

    hr = ITypeLib_GetTypeInfoOfGuid(pTypeLib, &IID_IDispatch, &pTypeInfo);
    ok_ole_success(hr, ITypeLib_GetTypeInfoOfGuid); 

    hr = ITypeInfo_GetIDsOfNames(pTypeInfo, &pwszGetTypeInfo, 1, &dispidMember);
    ok_ole_success(hr, ITypeInfo_GetIDsOfNames);

    hr = ITypeInfo_QueryInterface(pTypeInfo, &IID_ITypeInfo2, (void**)&pTypeInfo2);
    ok_ole_success(hr, ITypeInfo_QueryInterface);

    if (SUCCEEDED(hr))
    {
        VariantInit(&var);

        V_VT(&var) = VT_I4;

        /* test unknown guid passed to GetCustData */
        hr = ITypeInfo2_GetCustData(pTypeInfo2, &bogusguid, &var);
        ok_ole_success(hr, ITypeInfo_GetCustData);
        ok(V_VT(&var) == VT_EMPTY, "got %i, expected VT_EMPTY\n", V_VT(&var));

        ITypeInfo2_Release(pTypeInfo2);

        VariantClear(&var);
    }

    /* Check instance size for IDispatch, typelib is loaded using system SYS_WIN* kind so it always matches
       system bitness. */
    hr = ITypeInfo_GetTypeAttr(pTypeInfo, &attr);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(attr->cbSizeInstance == sizeof(void*), "got size %ld\n", attr->cbSizeInstance);
    ok(attr->typekind == TKIND_INTERFACE, "got typekind %d\n", attr->typekind);
    ITypeInfo_ReleaseTypeAttr(pTypeInfo, attr);

    /* same size check with some general interface */
    hr = ITypeLib_GetTypeInfoOfGuid(pTypeLib, &IID_IEnumVARIANT, &ti);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    hr = ITypeInfo_GetTypeAttr(ti, &attr);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(attr->cbSizeInstance == sizeof(void*), "got size %ld\n", attr->cbSizeInstance);
    ITypeInfo_ReleaseTypeAttr(ti, attr);
    ITypeInfo_Release(ti);

            /* test invoking a method with a [restricted] keyword  */

    /* correct member id -- wrong flags -- cNamedArgs not bigger than cArgs */
    dispparams.cNamedArgs = 0;
    hr = ITypeInfo_Invoke(pTypeInfo, (void *)0xdeadbeef, dispidMember, DISPATCH_PROPERTYGET, &dispparams, NULL, NULL, NULL);
    ok(hr == DISP_E_MEMBERNOTFOUND, "ITypeInfo_Invoke should have returned DISP_E_MEMBERNOTFOUND instead of 0x%08lx\n", hr);
    /* correct member id -- correct flags -- cNamedArgs not bigger than cArgs */
    hr = ITypeInfo_Invoke(pTypeInfo, (void *)0xdeadbeef, dispidMember, DISPATCH_METHOD, &dispparams, NULL, NULL, NULL);
    ok(hr == DISP_E_MEMBERNOTFOUND, "ITypeInfo_Invoke should have returned DISP_E_MEMBERNOTFOUND instead of 0x%08lx\n", hr);

    /* correct member id -- wrong flags -- cNamedArgs bigger than cArgs */
    dispparams.cNamedArgs = 1;
    hr = ITypeInfo_Invoke(pTypeInfo, (void *)0xdeadbeef, dispidMember, DISPATCH_PROPERTYGET, &dispparams, NULL, NULL, NULL);
    ok(hr == DISP_E_MEMBERNOTFOUND, "ITypeInfo_Invoke should have returned DISP_E_MEMBERNOTFOUND instead of 0x%08lx\n", hr);
    /* correct member id -- correct flags -- cNamedArgs bigger than cArgs */
    hr = ITypeInfo_Invoke(pTypeInfo, (void *)0xdeadbeef, dispidMember, DISPATCH_METHOD, &dispparams, NULL, NULL, NULL);
    ok(hr == DISP_E_MEMBERNOTFOUND, "ITypeInfo_Invoke should have returned DISP_E_MEMBERNOTFOUND instead of 0x%08lx\n", hr);

            /* test NULL dispparams */

    /* correct member id -- wrong flags -- cNamedArgs not bigger than cArgs */
    dispparams.cNamedArgs = 0;
    hr = ITypeInfo_Invoke(pTypeInfo, (void *)0xdeadbeef, dispidMember, DISPATCH_PROPERTYGET, NULL, NULL, NULL, NULL);
    ok(hr == DISP_E_MEMBERNOTFOUND, "ITypeInfo_Invoke should have returned DISP_E_MEMBERNOTFOUND instead of 0x%08lx\n", hr);
    /* correct member id -- correct flags -- cNamedArgs not bigger than cArgs */
    hr = ITypeInfo_Invoke(pTypeInfo, (void *)0xdeadbeef, dispidMember, DISPATCH_METHOD, NULL, NULL, NULL, NULL);
    ok(hr == DISP_E_MEMBERNOTFOUND, "ITypeInfo_Invoke should have returned DISP_E_MEMBERNOTFOUND instead of 0x%08lx\n", hr);

    /* correct member id -- wrong flags -- cNamedArgs bigger than cArgs */
    dispparams.cNamedArgs = 1;
    hr = ITypeInfo_Invoke(pTypeInfo, (void *)0xdeadbeef, dispidMember, DISPATCH_PROPERTYGET, NULL, NULL, NULL, NULL);
    ok(hr == DISP_E_MEMBERNOTFOUND, "ITypeInfo_Invoke should have returned DISP_E_MEMBERNOTFOUND instead of 0x%08lx\n", hr);
    /* correct member id -- correct flags -- cNamedArgs bigger than cArgs */
    hr = ITypeInfo_Invoke(pTypeInfo, (void *)0xdeadbeef, dispidMember, DISPATCH_METHOD, NULL, NULL, NULL, NULL);
    ok(hr == DISP_E_MEMBERNOTFOUND, "ITypeInfo_Invoke should have returned DISP_E_MEMBERNOTFOUND instead of 0x%08lx\n", hr);

    ITypeInfo_Release(pTypeInfo);
    ITypeLib_Release(pTypeLib);

    filename = create_test_typelib(3);
    hr = LoadTypeLib(filename, &pTypeLib);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    /* test GetDllEntry */
    hr = ITypeLib_GetTypeInfoOfGuid(pTypeLib, &moduleTestGetDllEntryGuid, &pTypeInfo);
    ok_ole_success(hr, ITypeLib_GetTypeInfoOfGuid);
    ok(pTypeInfo != NULL, "got NULL typeinfo\n");

    /* wrong memberid -- wrong invkind */
    hr = ITypeInfo_GetDllEntry(pTypeInfo, 0x6000000d, INVOKE_FUNC, &bstrDllName, &bstrName, &ordinal);
    ok(hr == TYPE_E_ELEMENTNOTFOUND, "ITypeInfo_GetDllEntry should have returned TYPE_E_ELEMENTNOTFOUND instead of 0x%08lx\n", hr);

    hr = ITypeInfo_GetDllEntry(pTypeInfo, 0x6000000d, INVOKE_PROPERTYPUTREF, &bstrDllName, &bstrName, &ordinal);
    ok(hr == TYPE_E_ELEMENTNOTFOUND, "ITypeInfo_GetDllEntry should have returned TYPE_E_ELEMENTNOTFOUND instead of 0x%08lx\n", hr);

    /* wrong memberid -- correct invkind */
    hr = ITypeInfo_GetDllEntry(pTypeInfo, 0x6000000d, INVOKE_PROPERTYGET, &bstrDllName, &bstrName, &ordinal);
    ok(hr == TYPE_E_ELEMENTNOTFOUND, "ITypeInfo_GetDllEntry should have returned TYPE_E_ELEMENTNOTFOUND instead of 0x%08lx\n", hr);

    hr = ITypeInfo_GetDllEntry(pTypeInfo, 0x6000000d, INVOKE_PROPERTYPUT, &bstrDllName, &bstrName, &ordinal);
    ok(hr == TYPE_E_ELEMENTNOTFOUND, "ITypeInfo_GetDllEntry should have returned TYPE_E_ELEMENTNOTFOUND instead of 0x%08lx\n", hr);

    /* correct memberid -- wrong invkind */
    hr = ITypeInfo_GetDllEntry(pTypeInfo, 0x60000000, INVOKE_FUNC, &bstrDllName, &bstrName, &ordinal);
    ok(hr == TYPE_E_ELEMENTNOTFOUND, "ITypeInfo_GetDllEntry should have returned TYPE_E_ELEMENTNOTFOUND instead of 0x%08lx\n", hr);

    hr = ITypeInfo_GetDllEntry(pTypeInfo, 0x60000000, INVOKE_PROPERTYPUTREF, &bstrDllName, &bstrName, &ordinal);
    ok(hr == TYPE_E_ELEMENTNOTFOUND, "ITypeInfo_GetDllEntry should have returned TYPE_E_ELEMENTNOTFOUND instead of 0x%08lx\n", hr);

    /* correct memberid -- correct invkind */
    hr = ITypeInfo_GetDllEntry(pTypeInfo, 0x60000000, INVOKE_PROPERTYGET, &bstrDllName, &bstrName, &ordinal);
    ok_ole_success(hr, ITypeInfo_GetDllEntry);
    ok(!lstrcmpW(bstrDllName, wszTestDll), "got %s\n", wine_dbgstr_w(bstrDllName));
    ok(bstrName == NULL, "got %s\n", wine_dbgstr_w(bstrName));
    ok(ordinal == 1, "got ordinal: %04x\n", ordinal);
    SysFreeString(bstrDllName);

    hr = ITypeInfo_GetDllEntry(pTypeInfo, 0x60000000, INVOKE_PROPERTYPUT, &bstrDllName, &bstrName, &ordinal);
    ok_ole_success(hr, ITypeInfo_GetDllEntry);
    ok(!lstrcmpW(bstrDllName, wszTestDll), "got %s\n", wine_dbgstr_w(bstrDllName));
    ok(bstrName == NULL, "got %s\n", wine_dbgstr_w(bstrName));
    ok(ordinal == 2, "got ordinal: %04x\n", ordinal);
    SysFreeString(bstrDllName);

    ITypeInfo_Release(pTypeInfo);

    hr = ITypeLib_GetTypeInfoOfGuid(pTypeLib, &IID_IInvokeTest, &pTypeInfo);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    dispparams.cArgs = 1;
    dispparams.cNamedArgs = 0;
    dispparams.rgdispidNamedArgs = NULL;
    dispparams.rgvarg = args;

    V_VT(&args[0]) = VT_I4;
    V_I4(&args[0]) = 0;

    i = 0;
    V_VT(&res) = VT_EMPTY;
    V_I4(&res) = 0;
    /* call propget with DISPATCH_METHOD|DISPATCH_PROPERTYGET flags */
    hr = ITypeInfo_Invoke(pTypeInfo, &invoketest, DISPID_VALUE, DISPATCH_METHOD|DISPATCH_PROPERTYGET,
        &dispparams, &res, NULL, &i);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(V_VT(&res) == VT_I4, "got %d\n", V_VT(&res));
    ok(V_I4(&res) == 1, "got %ld\n", V_I4(&res));

    i = 0;
    /* call propget with DISPATCH_METHOD flags */
    hr = ITypeInfo_Invoke(pTypeInfo, &invoketest, DISPID_VALUE, DISPATCH_METHOD,
        &dispparams, &res, NULL, &i);
    ok(hr == DISP_E_MEMBERNOTFOUND, "got 0x%08lx, %d\n", hr, i);

    i = 0;
    V_VT(&res) = VT_EMPTY;
    V_I4(&res) = 0;
    hr = ITypeInfo_Invoke(pTypeInfo, &invoketest, DISPID_VALUE, DISPATCH_PROPERTYGET,
        &dispparams, &res, NULL, &i);
    ok(hr == S_OK, "got 0x%08lx, %d\n", hr, i);
    ok(V_VT(&res) == VT_I4, "got %d\n", V_VT(&res));
    ok(V_I4(&res) == 1, "got %ld\n", V_I4(&res));

    /* call propget with DISPATCH_METHOD|DISPATCH_PROPERTYGET flags */
    V_VT(&args[0]) = VT_I4;
    V_I4(&args[0]) = 7;

    dispparams.cArgs = 1;
    dispparams.rgvarg = args;

    i = 0;
    V_VT(&res) = VT_EMPTY;
    V_I4(&res) = 0;
    hr = ITypeInfo_Invoke(pTypeInfo, &invoketest, 4, DISPATCH_METHOD|DISPATCH_PROPERTYGET, &dispparams, &res, NULL, &i);
    ok(hr == S_OK, "got 0x%08lx, %d\n", hr, i);
    ok(V_VT(&res) == VT_I4, "got %d\n", V_VT(&res));
    ok(V_I4(&res) == 15, "got %ld\n", V_I4(&res));


    /* DISPATCH_PROPERTYPUTREF */
    l = 1;
    V_VT(&args[0]) = VT_I4|VT_BYREF;
    V_I4REF(&args[0]) = &l;

    dispidMember = DISPID_PROPERTYPUT;
    dispparams.cArgs = 1;
    dispparams.cNamedArgs = 1;
    dispparams.rgdispidNamedArgs = &dispidMember;
    dispparams.rgvarg = args;

    i = 0;
    V_VT(&res) = VT_EMPTY;
    V_I4(&res) = 0;
    hr = ITypeInfo_Invoke(pTypeInfo, &invoketest, 1, DISPATCH_PROPERTYPUTREF, &dispparams, &res, NULL, &i);
    ok(hr == S_OK, "got 0x%08lx, %d\n", hr, i);
    ok(V_VT(&res) == VT_I4, "got %d\n", V_VT(&res));
    ok(V_I4(&res) == 3, "got %ld\n", V_I4(&res));

    i = 0;
    V_VT(&res) = VT_EMPTY;
    V_I4(&res) = 0;
    hr = ITypeInfo_Invoke(pTypeInfo, &invoketest, 1, DISPATCH_PROPERTYPUT, &dispparams, &res, NULL, &i);
    ok(hr == DISP_E_MEMBERNOTFOUND, "got 0x%08lx, %d\n", hr, i);

    i = 0;
    V_VT(&args[0]) = VT_UNKNOWN;
    V_UNKNOWN(&args[0]) = NULL;

    V_VT(&res) = VT_EMPTY;
    V_I4(&res) = 0;
    hr = ITypeInfo_Invoke(pTypeInfo, &invoketest, 2, DISPATCH_PROPERTYPUTREF, &dispparams, &res, NULL, &i);
    ok(hr == S_OK, "got 0x%08lx, %d\n", hr, i);
    ok(V_VT(&res) == VT_I4, "got %d\n", V_VT(&res));
    ok(V_I4(&res) == 6, "got %ld\n", V_I4(&res));

    i = 0;
    V_VT(&res) = VT_EMPTY;
    V_I4(&res) = 0;
    hr = ITypeInfo_Invoke(pTypeInfo, &invoketest, 2, DISPATCH_PROPERTYPUT, &dispparams, &res, NULL, &i);
    ok(hr == DISP_E_MEMBERNOTFOUND, "got 0x%08lx, %d\n", hr, i);

    V_VT(&res) = VT_I4;
    V_I4(&res) = 42;
    V_VT(&args[0]) = VT_I4;
    V_I4(&args[0]) = 1;
    V_VT(&args[1]) = VT_I4;
    V_I4(&args[1]) = 1;
    V_VT(&args[2]) = VT_I4;
    V_I4(&args[2]) = 1;
    dispparams.cNamedArgs = 0;

    /* call propget with DISPATCH_PROPERTYPUT (missing member id) */
    dispparams.cArgs = 1;
    hr = ITypeInfo_Invoke(pTypeInfo, &invoketest, 0xdeadbeef, DISPATCH_PROPERTYPUT, &dispparams, &res, NULL, &i);
    ok(hr == DISP_E_MEMBERNOTFOUND, "got 0x%08lx\n", hr);

    /* call propget with DISPATCH_PROPERTYPUT on HRESULT func(out int) - 0 params */
    dispparams.cArgs = 0;
    hr = ITypeInfo_Invoke(pTypeInfo, &invoketest, 5, DISPATCH_PROPERTYPUT, &dispparams, &res, NULL, &i);
    ok(hr == DISP_E_BADPARAMCOUNT, "got 0x%08lx\n", hr);

    /* call propget with DISPATCH_PROPERTYPUT on HRESULT func(out int) - 1 param */
    dispparams.cArgs = 1;
    hr = ITypeInfo_Invoke(pTypeInfo, &invoketest, 5, DISPATCH_PROPERTYPUT, &dispparams, &res, NULL, &i);
    ok(hr == DISP_E_BADPARAMCOUNT, "got 0x%08lx\n", hr);

    /* call propget with DISPATCH_PROPERTYPUT on HRESULT func(out int) - 0 params - NULL result passed */
    dispparams.cArgs = 0;
    hr = ITypeInfo_Invoke(pTypeInfo, &invoketest, 5, DISPATCH_PROPERTYPUT, &dispparams, NULL, NULL, &i);
    ok(hr == DISP_E_BADPARAMCOUNT, "got 0x%08lx\n", hr);

    /* call propget with DISPATCH_PROPERTYPUT on on HRESULT func(out int) - 1 params - NULL result passed */
    dispparams.cArgs = 1;
    hr = ITypeInfo_Invoke(pTypeInfo, &invoketest, 5, DISPATCH_PROPERTYPUT, &dispparams, NULL, NULL, &i);
    ok(hr == DISP_E_BADPARAMCOUNT, "got 0x%08lx\n", hr);

    /* call propget with DISPATCH_PROPERTYPUT on int func() - 0 params */
    dispparams.cArgs = 0;
    hr = ITypeInfo_Invoke(pTypeInfo, &invoketest, 6, DISPATCH_PROPERTYPUT, &dispparams, &res, NULL, &i);
    ok(hr == DISP_E_BADPARAMCOUNT, "got 0x%08lx\n", hr);

    /* call propget with DISPATCH_PROPERTYPUT on int func() - 1 params  */
    dispparams.cArgs = 1;
    hr = ITypeInfo_Invoke(pTypeInfo, &invoketest, 6, DISPATCH_PROPERTYPUT, &dispparams, &res, NULL, &i);
    ok(hr == DISP_E_BADPARAMCOUNT, "got 0x%08lx\n", hr);

    /* call propget with DISPATCH_PROPERTYPUT on int func() - 0 params - NULL result passed */
    dispparams.cArgs = 0;
    hr = ITypeInfo_Invoke(pTypeInfo, &invoketest, 6, DISPATCH_PROPERTYPUT, &dispparams, NULL, NULL, &i);
    ok(hr == DISP_E_BADPARAMCOUNT, "got 0x%08lx\n", hr);

    /* call propget with DISPATCH_PROPERTYPUT on int func() - 1 param - NULL result passed */
    dispparams.cArgs = 1;
    hr = ITypeInfo_Invoke(pTypeInfo, &invoketest, 6, DISPATCH_PROPERTYPUT, &dispparams, NULL, NULL, &i);
    ok(hr == DISP_E_BADPARAMCOUNT, "got 0x%08lx\n", hr);

    /* call propget with DISPATCH_PROPERTYPUT on int func(int) - 0 param - NULL result passed  */
    dispparams.cArgs = 0;
    hr = ITypeInfo_Invoke(pTypeInfo, &invoketest, 7, DISPATCH_PROPERTYPUT, &dispparams, NULL, NULL, &i);
    ok(hr == DISP_E_MEMBERNOTFOUND, "got 0x%08lx\n", hr);

    /* call propget with DISPATCH_PROPERTYPUT on int func(int) - 1 param - NULL result passed  */
    dispparams.cArgs = 1;
    hr = ITypeInfo_Invoke(pTypeInfo, &invoketest, 7, DISPATCH_PROPERTYPUT, &dispparams, NULL, NULL, &i);
    ok(hr == DISP_E_MEMBERNOTFOUND, "got 0x%08lx\n", hr);

    /* call propget with DISPATCH_PROPERTYPUT on int func(int) - 2 params - NULL result passed  */
    dispparams.cArgs = 2;
    hr = ITypeInfo_Invoke(pTypeInfo, &invoketest, 7, DISPATCH_PROPERTYPUT, &dispparams, NULL, NULL, &i);
    ok(hr == DISP_E_BADPARAMCOUNT, "got 0x%08lx\n", hr);

    /* call propget with DISPATCH_PROPERTYPUT on int func(int) - 3 params - NULL result passed  */
    dispparams.cArgs = 3;
    hr = ITypeInfo_Invoke(pTypeInfo, &invoketest, 7, DISPATCH_PROPERTYPUT, &dispparams, NULL, NULL, &i);
    ok(hr == DISP_E_MEMBERNOTFOUND, "got 0x%08lx\n", hr);

    /* call propget with DISPATCH_PROPERTYPUT on HRESULT func(out int, int) - 0 param - NULL result passed  */
    dispparams.cArgs = 0;
    hr = ITypeInfo_Invoke(pTypeInfo, &invoketest, 8, DISPATCH_PROPERTYPUT, &dispparams, NULL, NULL, &i);
    ok(hr == DISP_E_MEMBERNOTFOUND, "got 0x%08lx\n", hr);

    /* call propget with DISPATCH_PROPERTYPUT on HRESULT func(out int, int) - 1 param - NULL result passed  */
    dispparams.cArgs = 1;
    hr = ITypeInfo_Invoke(pTypeInfo, &invoketest, 8, DISPATCH_PROPERTYPUT, &dispparams, NULL, NULL, &i);
    ok(hr == DISP_E_MEMBERNOTFOUND, "got 0x%08lx\n", hr);

    /* call propget with DISPATCH_PROPERTYPUT on HRESULT func(out int, int) - 2 params - NULL result passed  */
    dispparams.cArgs = 2;
    hr = ITypeInfo_Invoke(pTypeInfo, &invoketest, 8, DISPATCH_PROPERTYPUT, &dispparams, NULL, NULL, &i);
    ok(hr == DISP_E_BADPARAMCOUNT, "got 0x%08lx\n", hr);

    /* call propget with DISPATCH_PROPERTYPUT on HRESULT func(out int, int) - 3 params - NULL result passed  */
    dispparams.cArgs = 3;
    hr = ITypeInfo_Invoke(pTypeInfo, &invoketest, 8, DISPATCH_PROPERTYPUT, &dispparams, NULL, NULL, &i);
    ok(hr == DISP_E_MEMBERNOTFOUND, "got 0x%08lx\n", hr);

    /* call propget with DISPATCH_PROPERTYPUT on int func(int, int) - 0 params - NULL result passed */
    dispparams.cArgs = 0;
    hr = ITypeInfo_Invoke(pTypeInfo, &invoketest, 9, DISPATCH_PROPERTYPUT, &dispparams, NULL, NULL, &i);
    ok(hr == DISP_E_MEMBERNOTFOUND, "got 0x%08lx\n", hr);

    /* call propget with DISPATCH_PROPERTYPUT on int func(int, int) - 1 params - NULL result passed */
    dispparams.cArgs = 1;
    hr = ITypeInfo_Invoke(pTypeInfo, &invoketest, 9, DISPATCH_PROPERTYPUT, &dispparams, NULL, NULL, &i);
    ok(hr == DISP_E_MEMBERNOTFOUND, "got 0x%08lx\n", hr);

    /* call propget with DISPATCH_PROPERTYPUT on int func(int, int) - 2 params - NULL result passed */
    dispparams.cArgs = 2;
    hr = ITypeInfo_Invoke(pTypeInfo, &invoketest, 9, DISPATCH_PROPERTYPUT, &dispparams, NULL, NULL, &i);
    ok(hr == DISP_E_MEMBERNOTFOUND, "got 0x%08lx\n", hr);

    /* call propget with DISPATCH_PROPERTYPUT on int func(int, int) - 3 params - NULL result passed */
    dispparams.cArgs = 3;
    hr = ITypeInfo_Invoke(pTypeInfo, &invoketest, 9, DISPATCH_PROPERTYPUT, &dispparams, NULL, NULL, &i);
    ok(hr == DISP_E_BADPARAMCOUNT, "got 0x%08lx\n", hr);

    /* call propget with DISPATCH_PROPERTYPUT on void func(int, int) - 0 params - NULL result passed */
    dispparams.cArgs = 0;
    hr = ITypeInfo_Invoke(pTypeInfo, &invoketest, 10, DISPATCH_PROPERTYPUT, &dispparams, NULL, NULL, &i);
    ok(hr == DISP_E_MEMBERNOTFOUND, "got 0x%08lx\n", hr);

    /* call propget with DISPATCH_PROPERTYPUT on void func(int, int) - 1 params - NULL result passed */
    dispparams.cArgs = 1;
    hr = ITypeInfo_Invoke(pTypeInfo, &invoketest, 10, DISPATCH_PROPERTYPUT, &dispparams, NULL, NULL, &i);
    ok(hr == DISP_E_MEMBERNOTFOUND, "got 0x%08lx\n", hr);

    /* call propget with DISPATCH_PROPERTYPUT on void func(int, int) - 2 params - NULL result passed */
    dispparams.cArgs = 2;
    hr = ITypeInfo_Invoke(pTypeInfo, &invoketest, 10, DISPATCH_PROPERTYPUT, &dispparams, NULL, NULL, &i);
    ok(hr == DISP_E_MEMBERNOTFOUND, "got 0x%08lx\n", hr);

    /* call propget with DISPATCH_PROPERTYPUT on void func(int, int) - 3 params - NULL result passed */
    dispparams.cArgs = 3;
    hr = ITypeInfo_Invoke(pTypeInfo, &invoketest, 10, DISPATCH_PROPERTYPUT, &dispparams, NULL, NULL, &i);
    ok(hr == DISP_E_BADPARAMCOUNT, "got 0x%08lx\n", hr);

    /* If there's get and put, the put must work */
    /* call propput with DISPATCH_PROPERTYPUT on int func(int) */
    dispidMember = DISPID_PROPERTYPUT;
    dispparams.cArgs = 1;
    dispparams.cNamedArgs = 1;
    dispparams.rgdispidNamedArgs = &dispidMember;
    hr = ITypeInfo_Invoke(pTypeInfo, &invoketest, 11, DISPATCH_PROPERTYPUT, &dispparams, NULL, NULL, &i);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    test_invoke_func(pTypeInfo);

    ITypeInfo_Release(pTypeInfo);
    ITypeLib_Release(pTypeLib);
    DeleteFileW(filename);
}

static int WINAPI int_func( int a0, int a1, int a2, int a3, int a4 )
{
    ok( a0 == 1, "wrong arg0 %x\n", a0 );
    ok( a1 == -1, "wrong arg1 %x\n", a1 );
#ifdef __arm__  /* parameters are extended on arm */
    ok( a2 == 1234, "wrong arg2 %x\n", a2 );
#else
    ok( a2 == (0x55550000 | 1234), "wrong arg2 %x\n", a2 );
#endif
    ok( a3 == 0xdeadbeef, "wrong arg3 %x\n", a3 );
    ok( a4 == 0x555555fd, "wrong arg4 %x\n", a4 );
    return 4321;
}

static double WINAPI double_func( double a0, float a1, double a2, int a3 )
{
    ok( a0 == 1.2, "wrong arg0 %f\n", a0 );
    ok( a1 == 3.25, "wrong arg1 %f\n", (double)a1 );
    ok( a2 == 1.2e12, "wrong arg2 %f\n", a2);
    ok( a3 == -4433.0, "wrong arg3 %f\n", (double)a3 );
    return 4321;
}

static LONGLONG WINAPI longlong_func( LONGLONG a0, CY a1 )
{
    ok( a0 == 0xdead00000000beefll, "wrong arg0 %#I64x\n", a0);
    ok( a1.int64 == ((ULONGLONG)10000 * 12345678), "wrong arg1 %#I64x\n", a1.int64 );
    return ((ULONGLONG)4321 << 32) | 8765;
}

static VARIANT WINAPI variant_func( int a0, BOOL a1, DECIMAL a2, VARIANT a3 )
{
    VARIANT var;
    ok( a0 == 2233, "wrong arg0 %x\n", a0 );
    ok( a1 == 1 || broken(a1 == 0x55550001), "wrong arg1 %x\n", a1 );
    V_VT(&var) = VT_LPWSTR;
    V_UI4(&var) = 0xbabe;
    ok( a2.Hi32 == 1122, "wrong arg2.Hi32 %lx\n", a2.Hi32 );
    ok( a2.Lo64 == 3344, "wrong arg2.Lo64 %#I64x\n", a2.Lo64 );
    ok( V_VT(&a3) == VT_EMPTY, "wrong arg3 type %x\n", V_VT(&a3) );
    ok( V_UI4(&a3) == 0xdeadbeef, "wrong arg3 value %lx\n", V_UI4(&a3) );
    return var;
}

static int CDECL void_func( int a0, int a1 )
{
#ifdef __i386__
    ok( a0 == 1111, "wrong arg0 %x\n", a0 );
    ok( a1 == 0, "wrong arg1 %x\n", a1 );
#else  /* VT_EMPTY is passed as real arg on other platforms */
    ok( a0 == 0x55555555, "wrong arg0 %x\n", a0 );
    ok( a1 == 1111, "wrong arg1 %x\n", a1 );
#endif
    return 12;
}

static int WINAPI stdcall_func( int a )
{
    return 0;
}

static int WINAPI inst_func( void *inst, int a )
{
    ok( (*(void ***)inst)[3] == inst_func, "wrong ptr %p\n", inst );
    ok( a == 3, "wrong arg %x\n", a );
    return a * 2;
}

static HRESULT WINAPI ret_false_func(void)
{
    return S_FALSE;
}

static const WCHAR testW[] = { 'T','e','s','t',0 };

static VARIANT WINAPI variant_func2(VARIANT v1, VARIANT v2)
{
    VARIANT ret;

    ok(V_VT(&v1) == VT_I4, "unexpected %d\n", V_VT(&v1));
    ok(V_I4(&v1) == 2, "unexpected %ld\n", V_I4(&v1));
    ok(V_VT(&v2) == VT_BSTR, "unexpected %d\n", V_VT(&v2));
    ok(lstrcmpW(V_BSTR(&v2), testW) == 0, "unexpected %s\n", wine_dbgstr_w(V_BSTR(&v2)));

    V_VT(&ret) = VT_UI4;
    V_I4(&ret) = 4321;
    return ret;
}

#ifdef __aarch64__
static VARIANT WINAPI inst_func2(void *inst, VARIANT v1, VARIANT v2)
{
    VARIANT ret;

    ok( (*(void ***)inst)[3] == inst_func2, "wrong ptr %p\n", inst );

    ok(V_VT(&v1) == VT_I4, "unexpected %d\n", V_VT(&v1));
    ok(V_I4(&v1) == 2, "unexpected %ld\n", V_I4(&v1));
    ok(V_VT(&v2) == VT_BSTR, "unexpected %d\n", V_VT(&v2));
    ok(lstrcmpW(V_BSTR(&v2), testW) == 0, "unexpected %s\n", wine_dbgstr_w(V_BSTR(&v2)));

    V_VT(&ret) = VT_UI4;
    V_I4(&ret) = 4321;
    return ret;
}
#else
static void WINAPI inst_func2(void *inst, VARIANT *ret, VARIANT v1, VARIANT v2)
{
    ok( (*(void ***)inst)[3] == inst_func2, "wrong ptr %p\n", inst );

    ok(V_VT(ret) == VT_I4 || broken(V_VT(ret) == VT_VARIANT) /* win64 */, "unexpected %d\n", V_VT(ret));
    ok(V_I4(ret) == 1234, "unexpected %ld\n", V_I4(ret));

    ok(V_VT(&v1) == VT_I4, "unexpected %d\n", V_VT(&v1));
    ok(V_I4(&v1) == 2, "unexpected %ld\n", V_I4(&v1));
    ok(V_VT(&v2) == VT_BSTR, "unexpected %d\n", V_VT(&v2));
    ok(lstrcmpW(V_BSTR(&v2), testW) == 0, "unexpected %s\n", wine_dbgstr_w(V_BSTR(&v2)));

    V_VT(ret) = VT_UI4;
    V_I4(ret) = 4321;
}
#endif

static void *vtable[] = { NULL, NULL, NULL, inst_func };
static void *vtable2[] = { NULL, NULL, NULL, inst_func2 };

static void test_DispCallFunc(void)
{
    void **inst;
    HRESULT res;
    VARIANT result, args[5];
    VARIANTARG *pargs[5];
    VARTYPE types[5];
    int i;

    for (i = 0; i < 5; i++) pargs[i] = &args[i];

    memset( args, 0x55, sizeof(args) );

    types[0] = VT_VARIANT;
    V_VT(&args[0]) = VT_I4;
    V_I4(&args[0]) = 2;
    types[1] = VT_VARIANT;
    V_VT(&args[1]) = VT_BSTR;
    V_BSTR(&args[1]) = SysAllocString(testW);
    memset( &result, 0xcc, sizeof(result) );
    res = DispCallFunc(NULL, (ULONG_PTR)variant_func2, CC_STDCALL, VT_VARIANT, 2, types, pargs, &result);
    ok(res == S_OK, "DispCallFunc error %#lx\n", res);
    ok(V_VT(&result) == VT_UI4, "wrong result type %d\n", V_VT(&result));
    ok(V_UI4(&result) == 4321, "wrong result %lu\n", V_UI4(&result));

    V_VT(&result) = VT_I4;
    V_UI4(&result) = 1234;
    inst = vtable2;
    res = DispCallFunc(&inst, 3 * sizeof(void *), CC_STDCALL, VT_VARIANT, 2, types, pargs, &result);
    ok(res == S_OK, "DispCallFunc error %#lx\n", res);
    ok(V_VT(&result) == VT_UI4, "wrong result type %d\n", V_VT(&result));
    ok(V_UI4(&result) == 4321, "wrong result %lu\n", V_UI4(&result));

    VariantClear(&args[1]);

    memset( args, 0x55, sizeof(args) );
    types[0] = VT_UI4;
    V_UI4(&args[0]) = 1;
    types[1] = VT_I4;
    V_I4(&args[1]) = -1;
    types[2] = VT_I2;
    V_I2(&args[2]) = 1234;
    types[3] = VT_UI4;
    V_UI4(&args[3]) = 0xdeadbeef;
    types[4] = VT_UI4;
    V_I1(&args[4]) = -3;
    memset( &result, 0xcc, sizeof(result) );
    res = DispCallFunc( NULL, (ULONG_PTR)int_func, CC_STDCALL, VT_UI4, 5, types, pargs, &result );
    ok( res == S_OK, "DispCallFunc failed %lx\n", res );
    ok( V_VT(&result) == VT_UI4, "wrong result type %d\n", V_VT(&result) );
    ok( V_UI4(&result) == 4321, "wrong result %lu\n", V_UI4(&result) );

    /* the function checks the argument sizes for stdcall */
    if (abi_supports_stdcall)
    {
        res = DispCallFunc( NULL, (ULONG_PTR)stdcall_func, CC_STDCALL, VT_UI4, 0, types, pargs, &result );
        ok( res == DISP_E_BADCALLEE, "DispCallFunc wrong error %lx\n", res );
        res = DispCallFunc( NULL, (ULONG_PTR)stdcall_func, CC_STDCALL, VT_UI4, 1, types, pargs, &result );
        ok( res == S_OK, "DispCallFunc failed %lx\n", res );
        res = DispCallFunc( NULL, (ULONG_PTR)stdcall_func, CC_STDCALL, VT_UI4, 2, types, pargs, &result );
        ok( res == DISP_E_BADCALLEE, "DispCallFunc wrong error %lx\n", res );
    }

    memset( args, 0x55, sizeof(args) );
    types[0] = VT_R8;
    V_R8(&args[0]) = 1.2;
    types[1] = VT_R4;
    V_R4(&args[1]) = 3.25;
    types[2] = VT_R8;
    V_R8(&args[2]) = 1.2e12;
    types[3] = VT_I4;
    V_I4(&args[3]) = -4433;
    memset( &result, 0xcc, sizeof(result) );
    res = DispCallFunc( NULL, (ULONG_PTR)double_func, CC_STDCALL, VT_R8, 4, types, pargs, &result );
    ok( res == S_OK, "DispCallFunc failed %lx\n", res );
    ok( V_VT(&result) == VT_R8, "wrong result type %d\n", V_VT(&result) );
    ok( V_R8(&result) == 4321, "wrong result %f\n", V_R8(&result) );

    memset( args, 0x55, sizeof(args) );
    types[0] = VT_I8;
    V_I8(&args[0]) = 0xdead00000000beefll;
    types[1] = VT_CY;
    V_CY(&args[1]).int64 = (ULONGLONG)10000 * 12345678;
    memset( &result, 0xcc, sizeof(result) );
    res = DispCallFunc( NULL, (ULONG_PTR)longlong_func, CC_STDCALL, VT_I8, 2, types, pargs, &result );
    ok( res == S_OK || broken(res == E_INVALIDARG),  /* longlong not supported on <= win2k */
        "DispCallFunc failed %lx\n", res );
    if (res == S_OK)
    {
        ok( V_VT(&result) == VT_I8, "wrong result type %d\n", V_VT(&result) );
        ok( V_I8(&result) == (((ULONGLONG)4321 << 32) | 8765), "wrong result %#I64x\n",
            V_I8(&result) );
    }

    memset( args, 0x55, sizeof(args) );
    types[0] = VT_I4;
    V_I4(&args[0]) = 2233;
    types[1] = VT_BOOL;
    V_BOOL(&args[1]) = 1;
    types[2] = VT_DECIMAL;
    V_DECIMAL(&args[2]).Hi32 = 1122;
    V_DECIMAL(&args[2]).Lo64 = 3344;
    types[3] = VT_VARIANT;
    V_VT(&args[3]) = VT_EMPTY;
    V_UI4(&args[3]) = 0xdeadbeef;
    types[4] = VT_EMPTY;
    memset( &result, 0xcc, sizeof(result) );
    res = DispCallFunc( NULL, (ULONG_PTR)variant_func, CC_STDCALL, VT_VARIANT, 5, types, pargs, &result );
    ok( res == S_OK, "DispCallFunc failed %lx\n", res );
    ok( V_VT(&result) == VT_LPWSTR, "wrong result type %d\n", V_VT(&result) );
    ok( V_UI4(&result) == 0xbabe, "wrong result %08lx\n", V_UI4(&result) );

    memset( args, 0x55, sizeof(args) );
    types[0] = VT_EMPTY;
    types[1] = VT_I4;
    V_I4(&args[1]) = 1111;
    types[2] = VT_EMPTY;
    types[3] = VT_I4;
    V_I4(&args[3]) = 0;
    types[4] = VT_EMPTY;
    memset( &result, 0xcc, sizeof(result) );
    res = DispCallFunc( NULL, (ULONG_PTR)void_func, CC_CDECL, VT_EMPTY, 5, types, pargs, &result );
    ok( res == S_OK, "DispCallFunc failed %lx\n", res );
    ok( V_VT(&result) == VT_EMPTY, "wrong result type %d\n", V_VT(&result) );
#ifdef __i386__
    ok( V_UI4(&result) == 0xcccccccc, "wrong result %08lx\n", V_UI4(&result) );
#else
    ok( V_UI4(&result) == 12, "wrong result %08lx\n", V_UI4(&result) );
#endif
    memset( args, 0x55, sizeof(args) );
    types[0] = VT_I4;
    V_I4(&args[0]) = 3;
    memset( &result, 0xcc, sizeof(result) );
    inst = vtable;
    res = DispCallFunc( &inst, 3 * sizeof(void*), CC_STDCALL, VT_I4, 1, types, pargs, &result );
    ok( res == S_OK, "DispCallFunc failed %lx\n", res );
    ok( V_VT(&result) == VT_I4, "wrong result type %d\n", V_VT(&result) );
    ok( V_I4(&result) == 6, "wrong result %08lx\n", V_I4(&result) );

    memset( &result, 0xcc, sizeof(result) );
    res = DispCallFunc(NULL, (ULONG_PTR)ret_false_func, CC_STDCALL, VT_ERROR, 0, NULL, NULL, &result);
    ok(res == S_OK, "DispCallFunc failed: %08lx\n", res);
    ok(V_VT(&result) == VT_ERROR, "V_VT(result) = %u\n", V_VT(&result));
    ok(V_ERROR(&result) == S_FALSE, "V_ERROR(result) = %08lx\n", V_ERROR(&result));

    memset( &result, 0xcc, sizeof(result) );
    res = DispCallFunc(NULL, (ULONG_PTR)ret_false_func, CC_STDCALL, VT_HRESULT, 0, NULL, NULL, &result);
    ok(res == E_INVALIDARG, "DispCallFunc failed: %08lx\n", res);
    ok(V_VT(&result) == 0xcccc, "V_VT(result) = %u\n", V_VT(&result));
}

/* RegDeleteTreeW from dlls/advapi32/registry.c, plus additional view flag */
static LSTATUS myRegDeleteTreeW(HKEY hKey, LPCWSTR lpszSubKey, REGSAM view)
{
    LONG ret;
    DWORD dwMaxSubkeyLen, dwMaxValueLen;
    DWORD dwMaxLen, dwSize;
    WCHAR szNameBuf[MAX_PATH], *lpszName = szNameBuf;
    HKEY hSubKey = hKey;
    view &= (KEY_WOW64_64KEY | KEY_WOW64_32KEY);

    if(lpszSubKey)
    {
        ret = RegOpenKeyExW(hKey, lpszSubKey, 0, KEY_READ | view, &hSubKey);
        if (ret) return ret;
    }

    ret = RegQueryInfoKeyW(hSubKey, NULL, NULL, NULL, NULL,
            &dwMaxSubkeyLen, NULL, NULL, &dwMaxValueLen, NULL, NULL, NULL);
    if (ret) goto cleanup;

    dwMaxSubkeyLen++;
    dwMaxValueLen++;
    dwMaxLen = max(dwMaxSubkeyLen, dwMaxValueLen);
    if (dwMaxLen > ARRAY_SIZE(szNameBuf))
    {
        /* Name too big: alloc a buffer for it */
        if (!(lpszName = HeapAlloc( GetProcessHeap(), 0, dwMaxLen*sizeof(WCHAR))))
        {
            ret = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
    }

    /* Recursively delete all the subkeys */
    while (TRUE)
    {
        dwSize = dwMaxLen;
        if (RegEnumKeyExW(hSubKey, 0, lpszName, &dwSize, NULL,
                          NULL, NULL, NULL)) break;

        ret = myRegDeleteTreeW(hSubKey, lpszName, view);
        if (ret) goto cleanup;
    }

    if (lpszSubKey)
        if (pRegDeleteKeyExW && view != 0)
            ret = pRegDeleteKeyExW(hKey, lpszSubKey, view, 0);
        else
            ret = RegDeleteKeyW(hKey, lpszSubKey);
    else
        while (TRUE)
        {
            dwSize = dwMaxLen;
            if (RegEnumValueW(hKey, 0, lpszName, &dwSize,
                  NULL, NULL, NULL, NULL)) break;

            ret = RegDeleteValueW(hKey, lpszName);
            if (ret) goto cleanup;
        }

cleanup:
    if (lpszName != szNameBuf)
        HeapFree(GetProcessHeap(), 0, lpszName);
    if(lpszSubKey)
        RegCloseKey(hSubKey);
    return ret;
}

static BOOL do_typelib_reg_key(GUID *uid, WORD maj, WORD min, DWORD arch, LPCWSTR base, BOOL remove)
{
    static const WCHAR typelibW[] = {'T','y','p','e','l','i','b','\\',0};
    static const WCHAR formatW[] = {'\\','%','x','.','%','x','\\','0','\\','w','i','n','%','u',0};
    static const WCHAR format2W[] = {'%','s','_','%','u','_','%','u','.','d','l','l',0};
    WCHAR buf[128];
    HKEY hkey;
    BOOL ret = TRUE;
    DWORD res;

    memcpy(buf, typelibW, sizeof(typelibW));
    StringFromGUID2(uid, buf + lstrlenW(buf), 40);

    if (remove)
    {
        ok(myRegDeleteTreeW(HKEY_CLASSES_ROOT, buf, 0) == ERROR_SUCCESS, "SHDeleteKey failed\n");
        return TRUE;
    }

    wsprintfW(buf + lstrlenW(buf), formatW, maj, min, arch);

    SetLastError(0xdeadbeef);
    res = RegCreateKeyExW(HKEY_CLASSES_ROOT, buf, 0, NULL, 0,
                          KEY_WRITE, NULL, &hkey, NULL);
    if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("W-calls are not implemented\n");
        return FALSE;
    }

    if (res != ERROR_SUCCESS)
    {
        trace("RegCreateKeyExW failed: %lu\n", res);
        return FALSE;
    }

    wsprintfW(buf, format2W, base, maj, min);
    if (RegSetValueExW(hkey, NULL, 0, REG_SZ,
                       (BYTE *)buf, (lstrlenW(buf) + 1) * sizeof(WCHAR)) != ERROR_SUCCESS)
    {
        trace("RegSetValueExW failed\n");
        ret = FALSE;
    }
    RegCloseKey(hkey);
    return ret;
}

static void test_QueryPathOfRegTypeLib(DWORD arch)
{
    static const struct test_data
    {
        WORD maj, min;
        HRESULT ret;
        const WCHAR path[16];
    } td[] = {
        { 1, 0, TYPE_E_LIBNOTREGISTERED, { 0 } },
        { 3, 0, S_OK, {'f','a','k','e','_','3','_','0','.','d','l','l',0 } },
        { 3, 1, S_OK, {'f','a','k','e','_','3','_','1','.','d','l','l',0 } },
        { 3, 22, S_OK, {'f','a','k','e','_','3','_','3','7','.','d','l','l',0 } },
        { 3, 37, S_OK, {'f','a','k','e','_','3','_','3','7','.','d','l','l',0 } },
        { 3, 40, TYPE_E_LIBNOTREGISTERED, { 0 } },
        { 0xffff, 0xffff, S_OK, {'f','a','k','e','_','5','_','3','7','.','d','l','l',0 } },
        { 0xffff, 0, TYPE_E_LIBNOTREGISTERED, { 0 } },
        { 3, 0xffff, TYPE_E_LIBNOTREGISTERED, { 0 } },
        { 5, 0xffff, TYPE_E_LIBNOTREGISTERED, { 0 } },
        { 4, 0, TYPE_E_LIBNOTREGISTERED, { 0 } }
    };
    static const WCHAR base[] = {'f','a','k','e',0};
    static const WCHAR wrongW[] = {'w','r','o','n','g',0};
    UINT i;
    RPC_STATUS status;
    GUID uid;
    WCHAR uid_str[40];
    HRESULT ret;
    BSTR path;

    status = UuidCreate(&uid);
    ok(!status || status == RPC_S_UUID_LOCAL_ONLY, "UuidCreate error %08lx\n", status);

    StringFromGUID2(&uid, uid_str, 40);
    /*trace("GUID: %s\n", wine_dbgstr_w(uid_str));*/

    if (!do_typelib_reg_key(&uid, 3, 0, arch, base, FALSE)) return;
    if (!do_typelib_reg_key(&uid, 3, 1, arch, base, FALSE)) return;
    if (!do_typelib_reg_key(&uid, 3, 37, arch, base, FALSE)) return;
    if (!do_typelib_reg_key(&uid, 5, 37, arch, base, FALSE)) return;
    if (!do_typelib_reg_key(&uid, 5, 37, (arch == 32) ? 64 : 32, wrongW, FALSE)) return;

    for (i = 0; i < ARRAY_SIZE(td); i++)
    {
        winetest_push_context( "win%lu: %u", arch, i );
        ret = QueryPathOfRegTypeLib(&uid, td[i].maj, td[i].min, LOCALE_NEUTRAL, &path);
        ok(ret == td[i].ret || (ret == TYPE_E_LIBNOTREGISTERED && arch == 64 && !is_win64 && !is_wow64),
           "QueryPathOfRegTypeLib(%u.%u) returned %08lx\n", td[i].maj, td[i].min, ret);
        if (ret == S_OK)
        {
            if (i == 6 && arch == (is_win64 ? 32 : 64))
                ok( !lstrcmpW( path, L"wrong_5_37.dll" ), "typelib %u.%u path doesn't match: %s\n",
                    td[i].maj, td[i].min, debugstr_w(path));
            else
                ok(!lstrcmpW(td[i].path, path), "typelib %u.%u path doesn't match: %s\n",
                   td[i].maj, td[i].min, debugstr_w(path));
            SysFreeString(path);
        }
        winetest_pop_context();
    }

    do_typelib_reg_key(&uid, 0, 0, arch, NULL, TRUE);
}

static void test_inheritance(void)
{
    HRESULT hr;
    ITypeLib *pTL;
    ITypeInfo *pTI, *pTI_p, *dual_ti;
    TYPEATTR *pTA;
    HREFTYPE href;
    FUNCDESC *pFD;
    WCHAR path[MAX_PATH];
    CHAR pathA[MAX_PATH];

    GetModuleFileNameA(NULL, pathA, MAX_PATH);
    MultiByteToWideChar(CP_ACP, 0, pathA, -1, path, MAX_PATH);

    hr = LoadTypeLib(path, &pTL);
    if(FAILED(hr)) return;


    /* ItestIF3 is a syntax 2 dispinterface */
    hr = ITypeLib_GetTypeInfoOfGuid(pTL, &DIID_ItestIF3, &pTI);
    ok(hr == S_OK, "hr %08lx\n", hr);

    hr = ITypeInfo_GetTypeAttr(pTI, &pTA);
    ok(hr == S_OK, "hr %08lx\n", hr);
    ok(pTA->typekind == TKIND_DISPATCH, "kind %04x\n", pTA->typekind);
    ok(pTA->cbSizeVft == 7 * sizeof(void *), "sizevft %d\n", pTA->cbSizeVft);
    ok(pTA->wTypeFlags == TYPEFLAG_FDISPATCHABLE, "typeflags %x\n", pTA->wTypeFlags);
    ok(pTA->cFuncs == 6, "cfuncs %d\n", pTA->cFuncs);
    ok(pTA->cImplTypes == 1, "cimpltypes %d\n", pTA->cImplTypes);
    ITypeInfo_ReleaseTypeAttr(pTI, pTA);

    hr = ITypeInfo_GetRefTypeOfImplType(pTI, 0, &href);
    ok(hr == S_OK, "hr %08lx\n", hr);
    hr = ITypeInfo_GetRefTypeInfo(pTI, href, &pTI_p);
    ok(hr == S_OK, "hr %08lx\n", hr);
    hr = ITypeInfo_GetTypeAttr(pTI_p, &pTA);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(IsEqualGUID(&pTA->guid, &IID_IDispatch), "guid %s\n", debugstr_guid(&pTA->guid));
    ITypeInfo_ReleaseTypeAttr(pTI_p, pTA);
    ITypeInfo_Release(pTI_p);

    /* Should have six methods */
    hr = ITypeInfo_GetFuncDesc(pTI, 6, &pFD);
    ok(hr == TYPE_E_ELEMENTNOTFOUND, "hr %08lx\n", hr);
    hr = ITypeInfo_GetFuncDesc(pTI, 5, &pFD);
    ok(hr == S_OK, "hr %08lx\n", hr);
    ok(pFD->memid == 0x60020000, "memid %08lx\n", pFD->memid);
    ok(pFD->oVft == 5 * sizeof(void *), "oVft %d\n", pFD->oVft);
    ITypeInfo_ReleaseFuncDesc(pTI, pFD);
    ITypeInfo_Release(pTI);


    /* ItestIF4 is a syntax 1 dispinterface */
    hr = ITypeLib_GetTypeInfoOfGuid(pTL, &DIID_ItestIF4, &pTI);
    ok(hr == S_OK, "hr %08lx\n", hr);

    hr = ITypeInfo_GetTypeAttr(pTI, &pTA);
    ok(hr == S_OK, "hr %08lx\n", hr);
    ok(pTA->typekind == TKIND_DISPATCH, "kind %04x\n", pTA->typekind);
    ok(pTA->cbSizeVft == 7 * sizeof(void *), "sizevft %d\n", pTA->cbSizeVft);
    ok(pTA->wTypeFlags == TYPEFLAG_FDISPATCHABLE, "typeflags %x\n", pTA->wTypeFlags);
    ok(pTA->cFuncs == 1, "cfuncs %d\n", pTA->cFuncs);
    ok(pTA->cImplTypes == 1, "cimpltypes %d\n", pTA->cImplTypes);
    ITypeInfo_ReleaseTypeAttr(pTI, pTA);

    hr = ITypeInfo_GetRefTypeOfImplType(pTI, 0, &href);
    ok(hr == S_OK, "hr %08lx\n", hr);
    hr = ITypeInfo_GetRefTypeInfo(pTI, href, &pTI_p);
    ok(hr == S_OK, "hr %08lx\n", hr);
    hr = ITypeInfo_GetTypeAttr(pTI_p, &pTA);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(IsEqualGUID(&pTA->guid, &IID_IDispatch), "guid %s\n", debugstr_guid(&pTA->guid));
    ITypeInfo_ReleaseTypeAttr(pTI_p, pTA);
    ITypeInfo_Release(pTI_p);
    hr = ITypeInfo_GetFuncDesc(pTI, 1, &pFD);
    ok(hr == TYPE_E_ELEMENTNOTFOUND, "hr %08lx\n", hr);
    hr = ITypeInfo_GetFuncDesc(pTI, 0, &pFD);
    ok(hr == S_OK, "hr %08lx\n", hr);
    ok(pFD->memid == 0x1c, "memid %08lx\n", pFD->memid);
    ITypeInfo_ReleaseFuncDesc(pTI, pFD);
    ITypeInfo_Release(pTI);


    /* ItestIF5 is dual with inherited ifaces which derive from IUnknown but not IDispatch */
    hr = ITypeLib_GetTypeInfoOfGuid(pTL, &IID_ItestIF5, &pTI);
    ok(hr == S_OK, "hr %08lx\n", hr);

    hr = ITypeInfo_GetTypeAttr(pTI, &pTA);
    ok(hr == S_OK, "hr %08lx\n", hr);
    ok(pTA->typekind == TKIND_DISPATCH, "kind %04x\n", pTA->typekind);
    ok(pTA->cbSizeVft == 7 * sizeof(void *), "sizevft %d\n", pTA->cbSizeVft);
    ok(pTA->wTypeFlags == TYPEFLAG_FDUAL, "typeflags %x\n", pTA->wTypeFlags);
    ok(pTA->cFuncs == 8, "cfuncs %d\n", pTA->cFuncs);
    ok(pTA->cImplTypes == 1, "cimpltypes %d\n", pTA->cImplTypes);
    ITypeInfo_ReleaseTypeAttr(pTI, pTA);

    hr = ITypeInfo_GetRefTypeOfImplType(pTI, 0, &href);
    ok(hr == S_OK, "hr %08lx\n", hr);
    hr = ITypeInfo_GetRefTypeInfo(pTI, href, &pTI_p);
    ok(hr == S_OK, "hr %08lx\n", hr);
    hr = ITypeInfo_GetTypeAttr(pTI_p, &pTA);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(IsEqualGUID(&pTA->guid, &IID_IDispatch), "guid %s\n", debugstr_guid(&pTA->guid));
    ITypeInfo_ReleaseTypeAttr(pTI_p, pTA);
    ITypeInfo_Release(pTI_p);
    hr = ITypeInfo_GetFuncDesc(pTI, 6, &pFD);
    ok(hr == S_OK, "hr %08lx\n", hr);
    ok(pFD->memid == 0x1234, "memid %08lx\n", pFD->memid);
    ITypeInfo_ReleaseFuncDesc(pTI, pFD);
    ITypeInfo_Release(pTI);

    /* ItestIF7 is dual with inherited ifaces which derive from Dispatch */
    hr = ITypeLib_GetTypeInfoOfGuid(pTL, &IID_ItestIF7, &pTI);
    ok(hr == S_OK, "hr %08lx\n", hr);

    hr = ITypeInfo_GetTypeAttr(pTI, &pTA);
    ok(hr == S_OK, "hr %08lx\n", hr);
    ok(pTA->typekind == TKIND_DISPATCH, "kind %04x\n", pTA->typekind);
    ok(pTA->cbSizeVft == 7 * sizeof(void *), "sizevft %d\n", pTA->cbSizeVft);
    ok(pTA->wTypeFlags == (TYPEFLAG_FDISPATCHABLE|TYPEFLAG_FDUAL), "typeflags %x\n", pTA->wTypeFlags);
    ok(pTA->cFuncs == 10, "cfuncs %d\n", pTA->cFuncs);
    ok(pTA->cImplTypes == 1, "cimpltypes %d\n", pTA->cImplTypes);
    ITypeInfo_ReleaseTypeAttr(pTI, pTA);

    hr = ITypeInfo_GetRefTypeOfImplType(pTI, 0, &href);
    ok(hr == S_OK, "hr %08lx\n", hr);
    hr = ITypeInfo_GetRefTypeInfo(pTI, href, &pTI_p);
    ok(hr == S_OK, "hr %08lx\n", hr);
    hr = ITypeInfo_GetTypeAttr(pTI_p, &pTA);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(IsEqualGUID(&pTA->guid, &IID_IDispatch), "guid %s\n", debugstr_guid(&pTA->guid));
    ITypeInfo_ReleaseTypeAttr(pTI_p, pTA);
    ITypeInfo_Release(pTI_p);

    hr = ITypeInfo_GetFuncDesc(pTI, 9, &pFD);
    ok(hr == S_OK, "hr %08lx\n", hr);
    ok(pFD->memid == 0x1236, "memid %08lx\n", pFD->memid);
    ITypeInfo_ReleaseFuncDesc(pTI, pFD);
    ITypeInfo_Release(pTI);

    /* ItestIF10 is a syntax 2 dispinterface which doesn't derive from IUnknown */
    hr = ITypeLib_GetTypeInfoOfGuid(pTL, &DIID_ItestIF10, &pTI);
    ok(hr == S_OK, "hr %08lx\n", hr);

    hr = ITypeInfo_GetTypeAttr(pTI, &pTA);
    ok(hr == S_OK, "hr %08lx\n", hr);
    ok(pTA->typekind == TKIND_DISPATCH, "kind %04x\n", pTA->typekind);
    ok(pTA->cbSizeVft == sizeof(IDispatchVtbl), "sizevft %d\n", pTA->cbSizeVft);
    ok(pTA->wTypeFlags == TYPEFLAG_FDISPATCHABLE, "typeflags %x\n", pTA->wTypeFlags);
    ok(pTA->cFuncs == 3, "cfuncs %d\n", pTA->cFuncs);
    ok(pTA->cImplTypes == 1, "cimpltypes %d\n", pTA->cImplTypes);
    ITypeInfo_ReleaseTypeAttr(pTI, pTA);

    hr = ITypeInfo_GetRefTypeOfImplType(pTI, -1, &href);
    ok(hr == TYPE_E_ELEMENTNOTFOUND, "hr %08lx\n", hr);
    hr = ITypeInfo_GetRefTypeOfImplType(pTI, 0, &href);
    ok(hr == S_OK, "hr %08lx\n", hr);
    hr = ITypeInfo_GetRefTypeInfo(pTI, href, &pTI_p);
    ok(hr == S_OK, "hr %08lx\n", hr);
    hr = ITypeInfo_GetTypeAttr(pTI_p, &pTA);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(IsEqualGUID(&pTA->guid, &IID_IDispatch), "guid %s\n", debugstr_guid(&pTA->guid));
    ITypeInfo_ReleaseTypeAttr(pTI_p, pTA);
    ITypeInfo_Release(pTI_p);

    /* Should have three methods */
    hr = ITypeInfo_GetFuncDesc(pTI, 3, &pFD);
    ok(hr == TYPE_E_ELEMENTNOTFOUND, "hr %08lx\n", hr);
    hr = ITypeInfo_GetFuncDesc(pTI, 2, &pFD);
    ok(hr == S_OK, "hr %08lx\n", hr);
    ok(pFD->memid == 0x60010000, "memid %08lx\n", pFD->memid);
    ok(pFD->oVft == 2 * sizeof(void *), "oVft %d\n", pFD->oVft);
    ITypeInfo_ReleaseFuncDesc(pTI, pFD);
    ITypeInfo_Release(pTI);

    /* ItestIF11 is a syntax 2 dispinterface which derives from IDispatch */
    hr = ITypeLib_GetTypeInfoOfGuid(pTL, &DIID_ItestIF11, &pTI);
    ok(hr == S_OK, "hr %08lx\n", hr);

    hr = ITypeInfo_GetTypeAttr(pTI, &pTA);
    ok(hr == S_OK, "hr %08lx\n", hr);
    ok(pTA->typekind == TKIND_DISPATCH, "kind %04x\n", pTA->typekind);
    ok(pTA->cbSizeVft == 7 * sizeof(void *), "sizevft %d\n", pTA->cbSizeVft);
    ok(pTA->wTypeFlags == TYPEFLAG_FDISPATCHABLE, "typeflags %x\n", pTA->wTypeFlags);
    ok(pTA->cFuncs == 10, "cfuncs %d\n", pTA->cFuncs);
    ok(pTA->cImplTypes == 1, "cimpltypes %d\n", pTA->cImplTypes);
    ITypeInfo_ReleaseTypeAttr(pTI, pTA);

    hr = ITypeInfo_GetRefTypeOfImplType(pTI, 0, &href);
    ok(hr == S_OK, "hr %08lx\n", hr);
    hr = ITypeInfo_GetRefTypeInfo(pTI, href, &pTI_p);
    ok(hr == S_OK, "hr %08lx\n", hr);
    hr = ITypeInfo_GetTypeAttr(pTI_p, &pTA);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(IsEqualGUID(&pTA->guid, &IID_IDispatch), "guid %s\n", debugstr_guid(&pTA->guid));
    ITypeInfo_ReleaseTypeAttr(pTI_p, pTA);
    ITypeInfo_Release(pTI_p);

    /* Should have ten methods */
    hr = ITypeInfo_GetFuncDesc(pTI, 10, &pFD);
    ok(hr == TYPE_E_ELEMENTNOTFOUND, "hr %08lx\n", hr);
    hr = ITypeInfo_GetFuncDesc(pTI, 9, &pFD);
    ok(hr == S_OK, "hr %08lx\n", hr);
    ok(pFD->memid == 0x1236, "memid %08lx\n", pFD->memid);
    ok(pFD->oVft == 9 * sizeof(void *), "oVft %d\n", pFD->oVft);

    /* first argument to 10th function is an HREFTYPE from the impl type */
    ok(pFD->cParams == 1, "cParams %i\n", pFD->cParams);
    ok(pFD->lprgelemdescParam[0].tdesc.vt == VT_USERDEFINED,
        "vt 0x%x\n", pFD->lprgelemdescParam[0].tdesc.vt);
    href = pFD->lprgelemdescParam[0].tdesc.hreftype;
    ok((href & 0xff000000) == 0x04000000, "href 0x%08lx\n", href);
    hr = ITypeInfo_GetRefTypeInfo(pTI, href, &pTI_p);
    ok(hr == S_OK, "hr %08lx\n", hr);
    if (SUCCEEDED(hr)) ITypeInfo_Release(pTI_p);
    ITypeInfo_ReleaseFuncDesc(pTI, pFD);
    ITypeInfo_Release(pTI);


    /* ItestIF2 is an interface which derives from IUnknown */
    hr = ITypeLib_GetTypeInfoOfGuid(pTL, &IID_ItestIF2, &pTI);
    ok(hr == S_OK, "hr %08lx\n", hr);

    hr = ITypeInfo_GetTypeAttr(pTI, &pTA);
    ok(hr == S_OK, "hr %08lx\n", hr);
    ok(pTA->typekind == TKIND_INTERFACE, "kind %04x\n", pTA->typekind);
    ok(pTA->cbSizeVft == 6 * sizeof(void *), "sizevft %d\n", pTA->cbSizeVft);
    ok(pTA->wTypeFlags == 0, "typeflags %x\n", pTA->wTypeFlags);
    ok(pTA->cFuncs == 1, "cfuncs %d\n", pTA->cFuncs);
    ok(pTA->cImplTypes == 1, "cimpltypes %d\n", pTA->cImplTypes);
    ITypeInfo_ReleaseTypeAttr(pTI, pTA);

    /* Should have one method */
    hr = ITypeInfo_GetFuncDesc(pTI, 1, &pFD);
    ok(hr == TYPE_E_ELEMENTNOTFOUND, "hr %08lx\n", hr);
    hr = ITypeInfo_GetFuncDesc(pTI, 0, &pFD);
    ok(hr == S_OK, "hr %08lx\n", hr);
    ok(pFD->memid == 0x60020000, "memid %08lx\n", pFD->memid);
    ok(pFD->oVft == 5 * sizeof(void *), "oVft %d\n", pFD->oVft);
    ITypeInfo_ReleaseFuncDesc(pTI, pFD);
    ITypeInfo_Release(pTI);

    /* ItestIF13 is dual with inherited dual ifaces */
    hr = ITypeLib_GetTypeInfoOfGuid(pTL, &IID_ItestIF13, &pTI);
    ok(hr == S_OK, "hr %08lx\n", hr);

    hr = ITypeInfo_GetTypeAttr(pTI, &pTA);
    ok(hr == S_OK, "hr %08lx\n", hr);
    ok(pTA->typekind == TKIND_DISPATCH, "kind %04x\n", pTA->typekind);
    ok(pTA->cbSizeVft == 7 * sizeof(void *), "sizevft %d\n", pTA->cbSizeVft);
    ok(pTA->wTypeFlags == (TYPEFLAG_FDISPATCHABLE|TYPEFLAG_FDUAL), "typeflags %x\n", pTA->wTypeFlags);
    ok(pTA->cFuncs == 10, "cfuncs %d\n", pTA->cFuncs);
    ok(pTA->cImplTypes == 1, "cimpltypes %d\n", pTA->cImplTypes);
    ITypeInfo_ReleaseTypeAttr(pTI, pTA);

    hr = ITypeInfo_GetRefTypeOfImplType(pTI, 0, &href);
    ok(hr == S_OK, "hr %08lx\n", hr);
    hr = ITypeInfo_GetRefTypeInfo(pTI, href, &pTI_p);
    ok(hr == S_OK, "hr %08lx\n", hr);
    hr = ITypeInfo_GetTypeAttr(pTI_p, &pTA);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(IsEqualGUID(&pTA->guid, &IID_IDispatch), "guid %s\n", wine_dbgstr_guid(&pTA->guid));
    ITypeInfo_ReleaseTypeAttr(pTI_p, pTA);

    hr = ITypeInfo_GetFuncDesc(pTI, 9, &pFD);
    ok(hr == S_OK, "hr %08lx\n", hr);
    ok(pFD->memid == 0x1236, "memid %08lx\n", pFD->memid);
    ITypeInfo_ReleaseFuncDesc(pTI, pFD);

    hr = ITypeInfo_GetRefTypeInfo(pTI, -2, &dual_ti);
    ok(hr == S_OK, "hr %08lx\n", hr);

    hr = ITypeInfo_GetTypeAttr(dual_ti, &pTA);
    ok(hr == S_OK, "hr %08lx\n", hr);
    ok(pTA->typekind == TKIND_INTERFACE, "kind %04x\n", pTA->typekind);
    ok(pTA->cbSizeVft == sizeof(ItestIF13Vtbl), "sizevft %d\n", pTA->cbSizeVft);
    ok(pTA->wTypeFlags == (TYPEFLAG_FDISPATCHABLE|TYPEFLAG_FOLEAUTOMATION|TYPEFLAG_FDUAL), "typeflags %x\n", pTA->wTypeFlags);
    ok(pTA->cFuncs == 1, "cfuncs %d\n", pTA->cFuncs);
    ok(pTA->cImplTypes == 1, "cimpltypes %d\n", pTA->cImplTypes);
    ITypeInfo_ReleaseTypeAttr(dual_ti, pTA);

    hr = ITypeInfo_GetRefTypeOfImplType(dual_ti, 0, &href);
    ok(hr == S_OK, "hr %08lx\n", hr);
    hr = ITypeInfo_GetRefTypeInfo(dual_ti, href, &pTI_p);
    ok(hr == S_OK, "hr %08lx\n", hr);
    hr = ITypeInfo_GetTypeAttr(pTI_p, &pTA);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(pTA->typekind == TKIND_INTERFACE, "kind %04x\n", pTA->typekind);
    ok(pTA->cbSizeVft == sizeof(ItestIF12Vtbl), "sizevft %d\n", pTA->cbSizeVft);
    ok(IsEqualGUID(&pTA->guid, &IID_ItestIF12), "guid %s\n", wine_dbgstr_guid(&pTA->guid));
    ITypeInfo_ReleaseTypeAttr(pTI_p, pTA);
    ITypeInfo_Release(pTI_p);

    ITypeInfo_Release(dual_ti);
    ITypeInfo_Release(pTI);

    ITypeLib_Release(pTL);

    return;
}

static void test_CreateTypeLib(SYSKIND sys) {
    static OLECHAR typelibW[] = {'t','y','p','e','l','i','b',0};
    static OLECHAR helpfileW[] = {'C',':','\\','b','o','g','u','s','.','h','l','p',0};
    static OLECHAR interface1W[] = {'i','n','t','e','r','f','a','c','e','1',0};
    static OLECHAR interface2W[] = {'i','n','t','e','r','f','a','c','e','2',0};
    static OLECHAR interface3W[] = {'i','n','t','e','r','f','a','c','e','3',0};
    static OLECHAR dualW[] = {'d','u','a','l',0};
    static OLECHAR coclassW[] = {'c','o','c','l','a','s','s',0};
    static const WCHAR defaultW[] = {'d','e','f','a','u','l','t',0x3213,0};
    static OLECHAR func1W[] = {'f','u','n','c','1',0};
    static OLECHAR func2W[] = {'f','u','n','c','2',0};
    static OLECHAR prop1W[] = {'P','r','o','p','1',0};
    static OLECHAR param1W[] = {'p','a','r','a','m','1',0};
    static OLECHAR param2W[] = {'p','a','r','a','m','2',0};
    static OLECHAR asdfW[] = {'A','s','d','f',0};
    static OLECHAR aliasW[] = {'a','l','i','a','s',0};
    static OLECHAR invokeW[] = {'I','n','v','o','k','e',0};
    static OLECHAR *names1[] = {func1W, param1W, param2W};
    static OLECHAR *names2[] = {func2W, param1W, param2W};
    static OLECHAR *propname[] = {prop1W, param1W};
    static const GUID tlcustguid = {0xbf611abe,0x5b38,0x11df,{0x91,0x5c,0x08,0x02,0x79,0x79,0x94,0x69}};
    static const GUID custguid = {0xbf611abe,0x5b38,0x11df,{0x91,0x5c,0x08,0x02,0x79,0x79,0x94,0x70}};
    static const GUID bogusguid = {0xbf611abe,0x5b38,0x11df,{0x91,0x5c,0x08,0x02,0x79,0x79,0x94,0x71}};
    static const GUID interfaceguid = {0x3b9ff02f,0x9675,0x4861,{0xb7,0x81,0xce,0xae,0xa4,0x78,0x2a,0xcc}};
    static const GUID interface2guid = {0x3b9ff02f,0x9675,0x4861,{0xb7,0x81,0xce,0xae,0xa4,0x78,0x2a,0xcd}};

    ITypeInfo *interface1, *interface2, *dual, *unknown, *dispatch, *ti, *ti_2;
    char filename[MAX_PATH];
    WCHAR filenameW[MAX_PATH];
    ICreateTypeLib2 *createtl;
    ICreateTypeInfo *createti;
    ICreateTypeInfo2 *createti2;
    ITypeLib *tl, *stdole;
    ITypeLib2 *tl2;
    ITypeInfo *tinfos[2];
    ITypeInfo2 *ti2;
    ITypeComp *tcomp, *tcomp2;
    MEMBERID memids[2];
    FUNCDESC funcdesc, *pfuncdesc;
    ELEMDESC elemdesc[5], *edesc;
    PARAMDESCEX paramdescex;
    TYPEDESC typedesc1, typedesc2;
    TYPEATTR *typeattr;
    TLIBATTR *libattr;
    HREFTYPE hreftype;
    BSTR name, docstring, helpfile, names[3];
    DWORD helpcontext, ptr_size, alignment;
    int impltypeflags;
    unsigned int cnames;
    USHORT found;
    VARIANT cust_data;
    HRESULT hres;
    TYPEKIND kind;
    DESCKIND desckind;
    BINDPTR bindptr;
    SYSKIND native_sys = is_win64 ? SYS_WIN64 : SYS_WIN32;

    switch(sys){
    case SYS_WIN32:
        winetest_push_context( "win32" );
        ptr_size = 4;
        alignment = sizeof(void*);
        break;
    case SYS_WIN64:
        winetest_push_context( "win64" );
        ptr_size = 8;
        alignment = 4;
        break;
    default:
        return;
    }

    hres = LoadTypeLib(wszStdOle2, &stdole);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ITypeLib_GetTypeInfoOfGuid(stdole, &IID_IUnknown, &unknown);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ITypeInfo_GetTypeAttr(unknown, &typeattr);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(typeattr->cbSizeVft == 3 * sizeof(void*), "Got wrong cbSizeVft: %u\n", typeattr->cbSizeVft);
    ITypeInfo_ReleaseTypeAttr(unknown, typeattr);

    hres = ITypeLib_GetTypeInfoOfGuid(stdole, &IID_IDispatch, &dispatch);
    ok(hres == S_OK, "got %08lx\n", hres);

    GetTempFileNameA(".", "tlb", 0, filename);
    MultiByteToWideChar(CP_ACP, 0, filename, -1, filenameW, MAX_PATH);

    hres = CreateTypeLib2(sys, filenameW, &createtl);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ICreateTypeLib2_QueryInterface(createtl, &IID_ITypeLib, (void**)&tl);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ITypeLib_GetTypeInfo(tl, 0, NULL);
    ok(hres == E_INVALIDARG, "got 0x%08lx\n", hres);

    hres = ITypeLib_GetTypeInfoType(tl, 0, &kind);
    ok(hres == TYPE_E_ELEMENTNOTFOUND, "got 0x%08lx\n", hres);

    hres = ITypeLib_GetTypeInfoType(tl, 0, NULL);
    ok(hres == E_INVALIDARG, "got 0x%08lx\n", hres);

    hres = ITypeLib_GetTypeInfoType(tl, 0, NULL);
    ok(hres == E_INVALIDARG, "got 0x%08lx\n", hres);

    hres = ITypeLib_GetLibAttr(tl, NULL);
    ok(hres == E_INVALIDARG, "got %08lx\n", hres);

    hres = ITypeLib_GetLibAttr(tl, &libattr);
    ok(hres == S_OK, "got %08lx\n", hres);

    ok(libattr->syskind == sys, "syskind = %d\n", libattr->syskind);
    ok(libattr->wMajorVerNum == 0, "wMajorVer = %d\n", libattr->wMajorVerNum);
    ok(libattr->wMinorVerNum == 0, "wMinorVerNum = %d\n", libattr->wMinorVerNum);
    ok(libattr->wLibFlags == 0, "wLibFlags = %d\n", libattr->wLibFlags);

    ITypeLib_ReleaseTLibAttr(tl, libattr);

    name = (BSTR)0xdeadbeef;
    hres = ITypeLib_GetDocumentation(tl, -1, &name, &docstring, &helpcontext, &helpfile);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(name == NULL, "name != NULL\n");
    ok(docstring == NULL, "docstring != NULL\n");
    ok(helpcontext == 0, "helpcontext != 0\n");
    ok(helpfile == NULL, "helpfile != NULL\n");

    hres = ITypeLib_GetDocumentation(tl, 0, &name, NULL, NULL, NULL);
    ok(hres == TYPE_E_ELEMENTNOTFOUND, "got %08lx\n", hres);

    hres = ICreateTypeLib2_SetName(createtl, typelibW);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ICreateTypeLib2_SetHelpFileName(createtl, helpfileW);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ITypeLib_GetDocumentation(tl, -1, NULL, NULL, NULL, NULL);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ITypeLib_GetDocumentation(tl, -1, &name, NULL, NULL, &helpfile);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(!memcmp(name, typelibW, sizeof(typelibW)), "name = %s\n", wine_dbgstr_w(name));
    ok(!memcmp(helpfile, helpfileW, sizeof(helpfileW)), "helpfile = %s\n", wine_dbgstr_w(helpfile));

    SysFreeString(name);
    SysFreeString(helpfile);

    V_VT(&cust_data) = VT_I4;
    V_I4(&cust_data) = 1;
    hres = ICreateTypeLib2_SetCustData(createtl, &tlcustguid, &cust_data);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ITypeLib_QueryInterface(tl, &IID_ITypeLib2, (void**)&tl2);
    ok(hres == S_OK, "no ITypeLib2 interface (%lx)\n", hres);

    V_VT(&cust_data) = VT_EMPTY;
    V_I4(&cust_data) = 0;
    hres = ITypeLib2_GetCustData(tl2, &tlcustguid, &cust_data);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(V_VT(&cust_data) == VT_I4, "V_VT(&cust_data) = %d\n", V_VT(&cust_data));
    ok(V_I4(&cust_data) == 1, "V_I4(&cust_data) = %ld\n", V_I4(&cust_data));

    ITypeLib2_Release(tl2);

    /* invalid parameters */
    hres = ICreateTypeLib2_CreateTypeInfo(createtl, NULL, TKIND_INTERFACE, &createti);
    ok(hres == E_INVALIDARG, "got %08lx\n", hres);

    hres = ICreateTypeLib2_CreateTypeInfo(createtl, interface1W, TKIND_INTERFACE, NULL);
    ok(hres == E_INVALIDARG, "got %08lx\n", hres);

    hres = ICreateTypeLib2_CreateTypeInfo(createtl, NULL, TKIND_INTERFACE, NULL);
    ok(hres == E_INVALIDARG, "got %08lx\n", hres);

    hres = ICreateTypeLib2_CreateTypeInfo(createtl, interface1W, TKIND_INTERFACE, &createti);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ICreateTypeInfo_QueryInterface(createti, &IID_ITypeInfo, (void**)&interface1);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ITypeLib_GetDocumentation(tl, 0, &name, NULL, NULL, NULL);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(!memcmp(name, interface1W, sizeof(interface1W)), "name = %s\n", wine_dbgstr_w(name));

    SysFreeString(name);

    ITypeLib_Release(tl);

    name = (BSTR)0xdeadbeef;
    helpfile = (BSTR)0xdeadbeef;
    hres = ITypeInfo_GetDocumentation(interface1, -1, &name, &docstring, &helpcontext, &helpfile);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(!memcmp(name, interface1W, sizeof(interface1W)), "name = %s\n", wine_dbgstr_w(name));
    ok(docstring == NULL, "docstring != NULL\n");
    ok(helpcontext == 0, "helpcontext != 0\n");
    ok(!memcmp(helpfile, helpfileW, sizeof(helpfileW)), "helpfile = %s\n", wine_dbgstr_w(helpfile));

    SysFreeString(name);
    SysFreeString(helpfile);

    hres = ITypeInfo_GetDocumentation(interface1, 0, &name, NULL, NULL, NULL);
    ok(hres == TYPE_E_ELEMENTNOTFOUND, "got %08lx\n", hres);

    hres = ITypeInfo_GetRefTypeInfo(interface1, 0, NULL);
    ok(hres == E_INVALIDARG, "got %08lx\n", hres);


    hres = ICreateTypeInfo_LayOut(createti);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ICreateTypeInfo_SetGuid(createti, &interfaceguid);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ICreateTypeInfo_AddRefTypeInfo(createti, NULL, &hreftype);
    ok(hres == E_INVALIDARG, "got %08lx\n", hres);

    hres = ICreateTypeInfo_AddRefTypeInfo(createti, unknown, NULL);
    ok(hres == E_INVALIDARG, "got %08lx\n", hres);

    hres = ICreateTypeInfo_AddRefTypeInfo(createti, unknown, &hreftype);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ICreateTypeInfo_AddImplType(createti, 1, hreftype);
    ok(hres == TYPE_E_ELEMENTNOTFOUND, "got %08lx\n", hres);

    hres = ICreateTypeInfo_AddImplType(createti, 0, hreftype);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ITypeInfo_GetRefTypeOfImplType(interface1, 0, &hreftype);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(hreftype == 3, "hreftype = %ld\n", hreftype);

    hres = ITypeInfo_GetRefTypeInfo(interface1, hreftype, &ti);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ITypeInfo_GetTypeAttr(ti, &typeattr);
    ok(hres == S_OK, "got %08lx\n", hres);
    todo_wine_if(sys != native_sys)
    ok(typeattr->cbSizeInstance == ptr_size, "retrieved IUnknown gave wrong cbSizeInstance: %lu\n", typeattr->cbSizeInstance);
    todo_wine_if(sys != native_sys)
    ok(typeattr->cbSizeVft == 3 * ptr_size, "retrieved IUnknown gave wrong cbSizeVft: %u\n", typeattr->cbSizeVft);
    ITypeInfo_ReleaseTypeAttr(ti, typeattr);

    ITypeInfo_Release(ti);

    hres = ITypeInfo_GetRefTypeOfImplType(interface1, -1, &hreftype);
    ok(hres == TYPE_E_ELEMENTNOTFOUND, "got %08lx\n", hres);

    ICreateTypeInfo_QueryInterface(createti, &IID_ITypeInfo2, (void**)&ti2);

    memset(&funcdesc, 0, sizeof(FUNCDESC));
    funcdesc.funckind = FUNC_PUREVIRTUAL;
    funcdesc.invkind = INVOKE_PROPERTYGET;
    funcdesc.callconv = CC_STDCALL;
    funcdesc.elemdescFunc.tdesc.vt = VT_BSTR;
    funcdesc.elemdescFunc.idldesc.wIDLFlags = IDLFLAG_NONE;

    hres = ICreateTypeInfo_AddFuncDesc(createti, 0, NULL);
    ok(hres == E_INVALIDARG, "got %08lx\n", hres);

    hres = ICreateTypeInfo_AddFuncDesc(createti, 1, &funcdesc);
    ok(hres == TYPE_E_ELEMENTNOTFOUND, "got %08lx\n", hres);

    hres = ICreateTypeInfo_AddFuncDesc(createti, 0, &funcdesc);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ITypeInfo2_GetFuncDesc(ti2, 0, NULL);
    ok(hres == E_INVALIDARG, "got %08lx\n", hres);

    hres = ITypeInfo2_GetFuncDesc(ti2, 1, &pfuncdesc);
    ok(hres == TYPE_E_ELEMENTNOTFOUND, "got %08lx\n", hres);

    hres = ITypeInfo2_GetFuncDesc(ti2, 0, &pfuncdesc);
    ok(hres == S_OK, "got %08lx\n", hres);

    ok(pfuncdesc->memid == 0, "got %lx\n", pfuncdesc->memid);
    ok(pfuncdesc->lprgscode == NULL, "got %p\n", pfuncdesc->lprgscode);
    ok(pfuncdesc->lprgelemdescParam == NULL, "got %p\n", pfuncdesc->lprgelemdescParam);
    ok(pfuncdesc->funckind == FUNC_PUREVIRTUAL, "got 0x%x\n", pfuncdesc->funckind);
    ok(pfuncdesc->invkind == INVOKE_PROPERTYGET, "got 0x%x\n", pfuncdesc->invkind);
    ok(pfuncdesc->callconv == CC_STDCALL, "got 0x%x\n", pfuncdesc->callconv);
    ok(pfuncdesc->cParams == 0, "got %d\n", pfuncdesc->cParams);
    ok(pfuncdesc->cParamsOpt == 0, "got %d\n", pfuncdesc->cParamsOpt);
    todo_wine_if(sys != native_sys)
    ok(pfuncdesc->oVft == 3 * ptr_size, "got %d\n", pfuncdesc->oVft);
    ok(pfuncdesc->cScodes == 0, "got %d\n", pfuncdesc->cScodes);
    ok(pfuncdesc->elemdescFunc.tdesc.vt == VT_BSTR, "got %d\n", pfuncdesc->elemdescFunc.tdesc.vt);
    ok(pfuncdesc->wFuncFlags == 0, "got 0x%x\n", pfuncdesc->wFuncFlags);

    ITypeInfo2_ReleaseFuncDesc(ti2, pfuncdesc);

    hres = ICreateTypeInfo_SetFuncHelpContext(createti, 0, 0xabcdefab);
    ok(hres == S_OK, "got %08lx\n", hres);

    funcdesc.invkind = INVOKE_PROPERTYPUT;
    hres = ICreateTypeInfo_AddFuncDesc(createti, 1, &funcdesc);
    ok(hres == TYPE_E_INCONSISTENTPROPFUNCS, "got %08lx\n", hres);

    funcdesc.invkind = INVOKE_PROPERTYPUTREF;
    hres = ICreateTypeInfo_AddFuncDesc(createti, 1, &funcdesc);
    ok(hres == TYPE_E_INCONSISTENTPROPFUNCS, "got %08lx\n", hres);

    elemdesc[0].tdesc.vt = VT_BSTR;
    elemdesc[0].idldesc.dwReserved = 0;
    elemdesc[0].idldesc.wIDLFlags = IDLFLAG_FIN;

    funcdesc.lprgelemdescParam = elemdesc;
    funcdesc.invkind = INVOKE_PROPERTYPUT;
    funcdesc.cParams = 1;
    funcdesc.elemdescFunc.tdesc.vt = VT_VOID;

    hres = ICreateTypeInfo_AddFuncDesc(createti, 1, &funcdesc);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ICreateTypeInfo_SetFuncHelpContext(createti, 1, 0xabcdefab);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ICreateTypeInfo_SetFuncAndParamNames(createti, 0, propname, 0);
    ok(hres == TYPE_E_ELEMENTNOTFOUND, "got %08lx\n", hres);

    hres = ICreateTypeInfo_SetFuncAndParamNames(createti, 0, NULL, 1);
    ok(hres == E_INVALIDARG, "got %08lx\n", hres);

    hres = ICreateTypeInfo_SetFuncAndParamNames(createti, 0, propname, 1);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ICreateTypeInfo_SetFuncAndParamNames(createti, 1, propname, 1);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ICreateTypeInfo_SetFuncAndParamNames(createti, 1, propname, 2);
    ok(hres == TYPE_E_ELEMENTNOTFOUND, "got %08lx\n", hres);

    hres = ITypeInfo2_GetFuncDesc(ti2, 1, &pfuncdesc);
    ok(hres == S_OK, "got %08lx\n", hres);

    ok(pfuncdesc->memid == 0, "got %lx\n", pfuncdesc->memid);
    ok(pfuncdesc->lprgscode == NULL, "got %p\n", pfuncdesc->lprgscode);
    ok(pfuncdesc->lprgelemdescParam != NULL, "got %p\n", pfuncdesc->lprgelemdescParam);
    ok(pfuncdesc->funckind == FUNC_PUREVIRTUAL, "got 0x%x\n", pfuncdesc->funckind);
    ok(pfuncdesc->invkind == INVOKE_PROPERTYPUT, "got 0x%x\n", pfuncdesc->invkind);
    ok(pfuncdesc->callconv == CC_STDCALL, "got 0x%x\n", pfuncdesc->callconv);
    ok(pfuncdesc->cParams == 1, "got %d\n", pfuncdesc->cParams);
    ok(pfuncdesc->cParamsOpt == 0, "got %d\n", pfuncdesc->cParamsOpt);
    todo_wine_if(sys != native_sys)
    ok(pfuncdesc->oVft == 4 * ptr_size, "got %d\n", pfuncdesc->oVft);
    ok(pfuncdesc->cScodes == 0, "got %d\n", pfuncdesc->cScodes);
    ok(pfuncdesc->elemdescFunc.tdesc.vt == VT_VOID, "got %d\n", pfuncdesc->elemdescFunc.tdesc.vt);
    ok(pfuncdesc->wFuncFlags == 0, "got 0x%x\n", pfuncdesc->wFuncFlags);

    edesc = pfuncdesc->lprgelemdescParam;
    ok(edesc->tdesc.vt == VT_BSTR, "got: %d\n", edesc->tdesc.vt);
    ok(edesc->idldesc.wIDLFlags == IDLFLAG_FIN, "got: %x\n", edesc->idldesc.wIDLFlags);

    ITypeInfo2_ReleaseFuncDesc(ti2, pfuncdesc);


    funcdesc.invkind = INVOKE_PROPERTYPUTREF;
    hres = ICreateTypeInfo_AddFuncDesc(createti, 0, &funcdesc);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ICreateTypeInfo_SetFuncHelpContext(createti, 0, 0xabcdefab);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ICreateTypeInfo_SetFuncHelpContext(createti, 0, 0x201);
    ok(hres == S_OK, "got %08lx\n", hres);

    funcdesc.memid = 1;
    funcdesc.lprgelemdescParam = NULL;
    funcdesc.invkind = INVOKE_FUNC;
    funcdesc.cParams = 0;
    funcdesc.cScodes = 1;
    funcdesc.lprgscode = NULL;
    hres = ICreateTypeInfo_AddFuncDesc(createti, 1, &funcdesc);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ITypeInfo2_GetFuncDesc(ti2, 1, &pfuncdesc);
    ok(hres == S_OK, "got %08lx\n", hres);

    ok(pfuncdesc->memid == 1, "got %ld\n", pfuncdesc->memid);
    ok(pfuncdesc->lprgscode == NULL, "got %p\n", pfuncdesc->lprgscode);
    ok(pfuncdesc->lprgelemdescParam == NULL, "got %p\n", pfuncdesc->lprgelemdescParam);
    ok(pfuncdesc->funckind == FUNC_PUREVIRTUAL, "got 0x%x\n", pfuncdesc->funckind);
    ok(pfuncdesc->invkind == INVOKE_FUNC, "got 0x%x\n", pfuncdesc->invkind);
    ok(pfuncdesc->callconv == CC_STDCALL, "got 0x%x\n", pfuncdesc->callconv);
    ok(pfuncdesc->cParams == 0, "got %d\n", pfuncdesc->cParams);
    ok(pfuncdesc->cParamsOpt == 0, "got %d\n", pfuncdesc->cParamsOpt);
    todo_wine_if(sys != native_sys)
    ok(pfuncdesc->oVft == 4 * ptr_size, "got %d\n", pfuncdesc->oVft);
    ok(pfuncdesc->cScodes == 0, "got %d\n", pfuncdesc->cScodes);
    ok(pfuncdesc->elemdescFunc.tdesc.vt == VT_VOID, "got %d\n", pfuncdesc->elemdescFunc.tdesc.vt);
    ok(pfuncdesc->wFuncFlags == 0, "got 0x%x\n", pfuncdesc->wFuncFlags);

    ITypeInfo2_ReleaseFuncDesc(ti2, pfuncdesc);

    funcdesc.memid = MEMBERID_NIL;
    hres = ICreateTypeInfo_AddFuncDesc(createti, 1, &funcdesc);
    ok(hres == S_OK, "got %08lx\n", hres);

    elemdesc[0].tdesc.vt = VT_PTR;
    elemdesc[0].tdesc.lptdesc = &typedesc1;
    typedesc1.vt = VT_BSTR;
    funcdesc.cParams = 1;
    funcdesc.lprgelemdescParam = elemdesc;
    hres = ICreateTypeInfo_AddFuncDesc(createti, 4, &funcdesc);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ITypeInfo2_GetFuncDesc(ti2, 4, &pfuncdesc);
    ok(hres == S_OK, "got %08lx\n", hres);

    ok(pfuncdesc->memid == 0x60010004, "got %lx\n", pfuncdesc->memid);
    ok(pfuncdesc->lprgscode == NULL, "got %p\n", pfuncdesc->lprgscode);
    ok(pfuncdesc->lprgelemdescParam != NULL, "got %p\n", pfuncdesc->lprgelemdescParam);
    ok(pfuncdesc->funckind == FUNC_PUREVIRTUAL, "got 0x%x\n", pfuncdesc->funckind);
    ok(pfuncdesc->invkind == INVOKE_FUNC, "got 0x%x\n", pfuncdesc->invkind);
    ok(pfuncdesc->callconv == CC_STDCALL, "got 0x%x\n", pfuncdesc->callconv);
    ok(pfuncdesc->cParams == 1, "got %d\n", pfuncdesc->cParams);
    ok(pfuncdesc->cParamsOpt == 0, "got %d\n", pfuncdesc->cParamsOpt);
    todo_wine_if(sys != native_sys)
    ok(pfuncdesc->oVft == 7 * ptr_size, "got %d\n", pfuncdesc->oVft);
    ok(pfuncdesc->cScodes == 0, "got %d\n", pfuncdesc->cScodes);
    ok(pfuncdesc->elemdescFunc.tdesc.vt == VT_VOID, "got %d\n", pfuncdesc->elemdescFunc.tdesc.vt);
    ok(pfuncdesc->wFuncFlags == 0, "got 0x%x\n", pfuncdesc->wFuncFlags);

    edesc = pfuncdesc->lprgelemdescParam;
    ok(edesc->tdesc.vt == VT_PTR, "got: %d\n", edesc->tdesc.vt);
    ok(edesc->paramdesc.wParamFlags == PARAMFLAG_FIN, "got: 0x%x\n", edesc->paramdesc.wParamFlags);
    ok(edesc->paramdesc.pparamdescex == NULL, "got: %p\n", edesc->paramdesc.pparamdescex);
    ok(edesc->tdesc.lptdesc != NULL, "got: %p\n", edesc->tdesc.lptdesc);
    ok(edesc->tdesc.lptdesc->vt == VT_BSTR, "got: %d\n", edesc->tdesc.lptdesc->vt);

    ITypeInfo2_ReleaseFuncDesc(ti2, pfuncdesc);

    elemdesc[0].tdesc.lptdesc = &typedesc2;
    typedesc2.vt = VT_PTR;
    typedesc2.lptdesc = &typedesc1;
    hres = ICreateTypeInfo_AddFuncDesc(createti, 4, &funcdesc);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ITypeInfo2_GetFuncDesc(ti2, 4, &pfuncdesc);
    ok(hres == S_OK, "got %08lx\n", hres);

    ok(pfuncdesc->memid == 0x60010007, "got %lx\n", pfuncdesc->memid);
    ok(pfuncdesc->lprgscode == NULL, "got %p\n", pfuncdesc->lprgscode);
    ok(pfuncdesc->lprgelemdescParam != NULL, "got %p\n", pfuncdesc->lprgelemdescParam);
    ok(pfuncdesc->funckind == FUNC_PUREVIRTUAL, "got 0x%x\n", pfuncdesc->funckind);
    ok(pfuncdesc->invkind == INVOKE_FUNC, "got 0x%x\n", pfuncdesc->invkind);
    ok(pfuncdesc->callconv == CC_STDCALL, "got 0x%x\n", pfuncdesc->callconv);
    ok(pfuncdesc->cParams == 1, "got %d\n", pfuncdesc->cParams);
    ok(pfuncdesc->cParamsOpt == 0, "got %d\n", pfuncdesc->cParamsOpt);
    todo_wine_if(sys != native_sys)
    ok(pfuncdesc->oVft == 7 * ptr_size, "got %d\n", pfuncdesc->oVft);
    ok(pfuncdesc->cScodes == 0, "got %d\n", pfuncdesc->cScodes);
    ok(pfuncdesc->elemdescFunc.tdesc.vt == VT_VOID, "got %d\n", pfuncdesc->elemdescFunc.tdesc.vt);
    ok(pfuncdesc->wFuncFlags == 0, "got 0x%x\n", pfuncdesc->wFuncFlags);

    edesc = pfuncdesc->lprgelemdescParam;
    ok(edesc->tdesc.vt == VT_PTR, "got: %d\n", edesc->tdesc.vt);
    ok(edesc->paramdesc.wParamFlags == PARAMFLAG_FIN, "got: 0x%x\n", edesc->paramdesc.wParamFlags);
    ok(edesc->paramdesc.pparamdescex == NULL, "got: %p\n", edesc->paramdesc.pparamdescex);
    ok(edesc->tdesc.lptdesc != NULL, "got: %p\n", edesc->tdesc.lptdesc);
    ok(edesc->tdesc.lptdesc->vt == VT_PTR, "got: %d\n", edesc->tdesc.lptdesc->vt);
    ok(edesc->tdesc.lptdesc->lptdesc != NULL, "got: %p\n", edesc->tdesc.lptdesc->lptdesc);
    ok(edesc->tdesc.lptdesc->lptdesc->vt == VT_BSTR, "got: %d\n", edesc->tdesc.lptdesc->lptdesc->vt);

    ITypeInfo2_ReleaseFuncDesc(ti2, pfuncdesc);

    elemdesc[0].tdesc.vt = VT_INT;
    elemdesc[0].paramdesc.wParamFlags = PARAMFLAG_FHASDEFAULT;
    elemdesc[0].paramdesc.pparamdescex = &paramdescex;
    V_VT(&paramdescex.varDefaultValue) = VT_INT;
    V_INT(&paramdescex.varDefaultValue) = 0x123;
    hres = ICreateTypeInfo_AddFuncDesc(createti, 3, &funcdesc);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ITypeInfo2_GetFuncDesc(ti2, 3, &pfuncdesc);
    ok(hres == S_OK, "got %08lx\n", hres);

    ok(pfuncdesc->memid == 0x60010003, "got %lx\n", pfuncdesc->memid);
    ok(pfuncdesc->lprgscode == NULL, "got %p\n", pfuncdesc->lprgscode);
    ok(pfuncdesc->lprgelemdescParam != NULL, "got %p\n", pfuncdesc->lprgelemdescParam);
    ok(pfuncdesc->funckind == FUNC_PUREVIRTUAL, "got 0x%x\n", pfuncdesc->funckind);
    ok(pfuncdesc->invkind == INVOKE_FUNC, "got 0x%x\n", pfuncdesc->invkind);
    ok(pfuncdesc->callconv == CC_STDCALL, "got 0x%x\n", pfuncdesc->callconv);
    ok(pfuncdesc->cParams == 1, "got %d\n", pfuncdesc->cParams);
    ok(pfuncdesc->cParamsOpt == 0, "got %d\n", pfuncdesc->cParamsOpt);
    todo_wine_if(sys != native_sys)
    ok(pfuncdesc->oVft == 6 * ptr_size, "got %d\n", pfuncdesc->oVft);
    ok(pfuncdesc->cScodes == 0, "got %d\n", pfuncdesc->cScodes);
    ok(pfuncdesc->elemdescFunc.tdesc.vt == VT_VOID, "got %d\n", pfuncdesc->elemdescFunc.tdesc.vt);
    ok(pfuncdesc->wFuncFlags == 0, "got 0x%x\n", pfuncdesc->wFuncFlags);

    edesc = pfuncdesc->lprgelemdescParam;
    ok(edesc->tdesc.vt == VT_INT, "got: %d\n", edesc->tdesc.vt);
    ok(edesc->paramdesc.wParamFlags == PARAMFLAG_FHASDEFAULT, "got: 0x%x\n", edesc->paramdesc.wParamFlags);
    ok(edesc->paramdesc.pparamdescex != NULL, "got: %p\n", edesc->paramdesc.pparamdescex);
    ok(edesc->paramdesc.pparamdescex->cBytes == sizeof(PARAMDESCEX), "got: %ld\n",
            edesc->paramdesc.pparamdescex->cBytes);
    ok(V_VT(&edesc->paramdesc.pparamdescex->varDefaultValue) == VT_I4, "got: %d\n",
            V_VT(&edesc->paramdesc.pparamdescex->varDefaultValue));
    ok(V_I4(&edesc->paramdesc.pparamdescex->varDefaultValue) == 0x123, "got: 0x%lx\n",
            V_I4(&edesc->paramdesc.pparamdescex->varDefaultValue));

    ITypeInfo2_ReleaseFuncDesc(ti2, pfuncdesc);

    elemdesc[0].idldesc.dwReserved = 0;
    elemdesc[0].idldesc.wIDLFlags = IDLFLAG_FIN;
    elemdesc[1].tdesc.vt = VT_UI2;
    elemdesc[1].paramdesc.wParamFlags = PARAMFLAG_FHASDEFAULT;
    elemdesc[1].paramdesc.pparamdescex = &paramdescex;
    V_VT(&paramdescex.varDefaultValue) = VT_UI2;
    V_UI2(&paramdescex.varDefaultValue) = 0xffff;
    funcdesc.cParams = 2;
    hres = ICreateTypeInfo_AddFuncDesc(createti, 3, &funcdesc);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ITypeInfo2_GetFuncDesc(ti2, 3, &pfuncdesc);
    ok(hres == S_OK, "got %08lx\n", hres);

    ok(pfuncdesc->memid == 0x60010009, "got %lx\n", pfuncdesc->memid);
    ok(pfuncdesc->lprgscode == NULL, "got %p\n", pfuncdesc->lprgscode);
    ok(pfuncdesc->lprgelemdescParam != NULL, "got %p\n", pfuncdesc->lprgelemdescParam);
    ok(pfuncdesc->funckind == FUNC_PUREVIRTUAL, "got 0x%x\n", pfuncdesc->funckind);
    ok(pfuncdesc->invkind == INVOKE_FUNC, "got 0x%x\n", pfuncdesc->invkind);
    ok(pfuncdesc->callconv == CC_STDCALL, "got 0x%x\n", pfuncdesc->callconv);
    ok(pfuncdesc->cParams == 2, "got %d\n", pfuncdesc->cParams);
    ok(pfuncdesc->cParamsOpt == 0, "got %d\n", pfuncdesc->cParamsOpt);
    todo_wine_if(sys != native_sys)
    ok(pfuncdesc->oVft == 6 * ptr_size, "got %d\n", pfuncdesc->oVft);
    ok(pfuncdesc->cScodes == 0, "got %d\n", pfuncdesc->cScodes);
    ok(pfuncdesc->elemdescFunc.tdesc.vt == VT_VOID, "got %d\n", pfuncdesc->elemdescFunc.tdesc.vt);
    ok(pfuncdesc->wFuncFlags == 0, "got 0x%x\n", pfuncdesc->wFuncFlags);

    edesc = pfuncdesc->lprgelemdescParam;
    ok(edesc->tdesc.vt == VT_INT, "got: %d\n", edesc->tdesc.vt);
    ok(edesc->paramdesc.wParamFlags == PARAMFLAG_FIN, "got: 0x%x\n", edesc->paramdesc.wParamFlags);
    ok(edesc->paramdesc.pparamdescex == NULL, "got: %p\n", edesc->paramdesc.pparamdescex);

    edesc = pfuncdesc->lprgelemdescParam + 1;
    ok(edesc->tdesc.vt == VT_UI2, "got: %d\n", edesc->tdesc.vt);
    ok(edesc->paramdesc.wParamFlags == PARAMFLAG_FHASDEFAULT, "got: 0x%x\n", edesc->paramdesc.wParamFlags);
    ok(edesc->paramdesc.pparamdescex != NULL, "got: %p\n", edesc->paramdesc.pparamdescex);
    ok(edesc->paramdesc.pparamdescex->cBytes == sizeof(PARAMDESCEX), "got: %ld\n",
            edesc->paramdesc.pparamdescex->cBytes);
    ok(V_VT(&edesc->paramdesc.pparamdescex->varDefaultValue) == VT_UI2, "got: %d\n",
            V_VT(&edesc->paramdesc.pparamdescex->varDefaultValue));
    ok(V_UI2(&edesc->paramdesc.pparamdescex->varDefaultValue) == 0xFFFF, "got: 0x%x\n",
            V_UI2(&edesc->paramdesc.pparamdescex->varDefaultValue));

    ITypeInfo2_ReleaseFuncDesc(ti2, pfuncdesc);

    elemdesc[0].paramdesc.wParamFlags = PARAMFLAG_FHASDEFAULT;
    elemdesc[0].paramdesc.pparamdescex = &paramdescex;
    elemdesc[1].tdesc.vt = VT_INT;
    V_VT(&paramdescex.varDefaultValue) = VT_INT;
    V_INT(&paramdescex.varDefaultValue) = 0xffffffff;
    hres = ICreateTypeInfo_AddFuncDesc(createti, 3, &funcdesc);
    ok(hres == S_OK, "got %08lx\n", hres);

    elemdesc[0].tdesc.vt = VT_BSTR;
    elemdesc[1].tdesc.vt = VT_BSTR;
    V_VT(&paramdescex.varDefaultValue) = VT_BSTR;
    V_BSTR(&paramdescex.varDefaultValue) = SysAllocString(defaultW);
    hres = ICreateTypeInfo_AddFuncDesc(createti, 3, &funcdesc);
    ok(hres == S_OK, "got %08lx\n", hres);
    SysFreeString(V_BSTR(&paramdescex.varDefaultValue));

    hres = ITypeInfo2_GetFuncDesc(ti2, 3, &pfuncdesc);
    ok(hres == S_OK, "got %08lx\n", hres);

    ok(pfuncdesc->memid == 0x6001000b, "got %lx\n", pfuncdesc->memid);
    ok(pfuncdesc->lprgscode == NULL, "got %p\n", pfuncdesc->lprgscode);
    ok(pfuncdesc->lprgelemdescParam != NULL, "got %p\n", pfuncdesc->lprgelemdescParam);
    ok(pfuncdesc->funckind == FUNC_PUREVIRTUAL, "got 0x%x\n", pfuncdesc->funckind);
    ok(pfuncdesc->invkind == INVOKE_FUNC, "got 0x%x\n", pfuncdesc->invkind);
    ok(pfuncdesc->callconv == CC_STDCALL, "got 0x%x\n", pfuncdesc->callconv);
    ok(pfuncdesc->cParams == 2, "got %d\n", pfuncdesc->cParams);
    ok(pfuncdesc->cParamsOpt == 0, "got %d\n", pfuncdesc->cParamsOpt);
    todo_wine_if(sys != native_sys)
    ok(pfuncdesc->oVft == 6 * ptr_size, "got %d\n", pfuncdesc->oVft);
    ok(pfuncdesc->cScodes == 0, "got %d\n", pfuncdesc->cScodes);
    ok(pfuncdesc->elemdescFunc.tdesc.vt == VT_VOID, "got %d\n", pfuncdesc->elemdescFunc.tdesc.vt);
    ok(pfuncdesc->wFuncFlags == 0, "got 0x%x\n", pfuncdesc->wFuncFlags);

    edesc = pfuncdesc->lprgelemdescParam;
    ok(edesc->tdesc.vt == VT_BSTR, "got: %d\n", edesc->tdesc.vt);
    ok(edesc->paramdesc.wParamFlags == PARAMFLAG_FHASDEFAULT, "got: 0x%x\n", edesc->paramdesc.wParamFlags);
    ok(edesc->paramdesc.pparamdescex != NULL, "got: %p\n", edesc->paramdesc.pparamdescex);
    ok(edesc->paramdesc.pparamdescex->cBytes == sizeof(PARAMDESCEX), "got: %ld\n",
            edesc->paramdesc.pparamdescex->cBytes);
    ok(V_VT(&edesc->paramdesc.pparamdescex->varDefaultValue) == VT_BSTR, "got: %d\n",
            V_VT(&edesc->paramdesc.pparamdescex->varDefaultValue));
    ok(compare_wstr(V_BSTR(&edesc->paramdesc.pparamdescex->varDefaultValue), defaultW),
            "got: %s\n",
            wine_dbgstr_w(V_BSTR(&edesc->paramdesc.pparamdescex->varDefaultValue)));

    edesc = pfuncdesc->lprgelemdescParam + 1;
    ok(edesc->tdesc.vt == VT_BSTR, "got: %d\n", edesc->tdesc.vt);
    ok(edesc->paramdesc.wParamFlags == PARAMFLAG_FHASDEFAULT, "got: 0x%x\n", edesc->paramdesc.wParamFlags);
    ok(edesc->paramdesc.pparamdescex != NULL, "got: %p\n", edesc->paramdesc.pparamdescex);
    ok(edesc->paramdesc.pparamdescex->cBytes == sizeof(PARAMDESCEX), "got: %ld\n",
            edesc->paramdesc.pparamdescex->cBytes);
    ok(V_VT(&edesc->paramdesc.pparamdescex->varDefaultValue) == VT_BSTR, "got: %d\n",
            V_VT(&edesc->paramdesc.pparamdescex->varDefaultValue));
    ok(compare_wstr(V_BSTR(&edesc->paramdesc.pparamdescex->varDefaultValue), defaultW),
            "got: %s\n",
            wine_dbgstr_w(V_BSTR(&edesc->paramdesc.pparamdescex->varDefaultValue)));

    ITypeInfo2_ReleaseFuncDesc(ti2, pfuncdesc);

    elemdesc[0].tdesc.vt = VT_USERDEFINED;
    elemdesc[0].tdesc.hreftype = hreftype;
    elemdesc[0].paramdesc.pparamdescex = &paramdescex;
    elemdesc[0].paramdesc.wParamFlags = PARAMFLAG_FHASDEFAULT;
    V_VT(&paramdescex.varDefaultValue) = VT_INT;
    V_INT(&paramdescex.varDefaultValue) = 0x789;

    funcdesc.lprgelemdescParam = elemdesc;
    funcdesc.invkind = INVOKE_FUNC;
    funcdesc.cParams = 1;
    funcdesc.elemdescFunc.tdesc.vt = VT_VOID;

    hres = ICreateTypeInfo_AddFuncDesc(createti, 5, &funcdesc);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ITypeInfo2_GetFuncDesc(ti2, 5, &pfuncdesc);
    ok(hres == S_OK, "got %08lx\n", hres);

    ok(pfuncdesc->memid == 0x60010005, "got %lx\n", pfuncdesc->memid);
    ok(pfuncdesc->lprgscode == NULL, "got %p\n", pfuncdesc->lprgscode);
    ok(pfuncdesc->lprgelemdescParam != NULL, "got %p\n", pfuncdesc->lprgelemdescParam);
    ok(pfuncdesc->funckind == FUNC_PUREVIRTUAL, "got 0x%x\n", pfuncdesc->funckind);
    ok(pfuncdesc->invkind == INVOKE_FUNC, "got 0x%x\n", pfuncdesc->invkind);
    ok(pfuncdesc->callconv == CC_STDCALL, "got 0x%x\n", pfuncdesc->callconv);
    ok(pfuncdesc->cParams == 1, "got %d\n", pfuncdesc->cParams);
    ok(pfuncdesc->cParamsOpt == 0, "got %d\n", pfuncdesc->cParamsOpt);
    todo_wine_if(sys != native_sys)
    ok(pfuncdesc->oVft == 8 * ptr_size, "got %d\n", pfuncdesc->oVft);
    ok(pfuncdesc->cScodes == 0, "got %d\n", pfuncdesc->cScodes);
    ok(pfuncdesc->elemdescFunc.tdesc.vt == VT_VOID, "got %d\n", pfuncdesc->elemdescFunc.tdesc.vt);
    ok(pfuncdesc->wFuncFlags == 0, "got 0x%x\n", pfuncdesc->wFuncFlags);

    edesc = pfuncdesc->lprgelemdescParam;
    ok(edesc->paramdesc.pparamdescex != NULL, "got: %p\n", edesc->paramdesc.pparamdescex);
    ok(edesc->paramdesc.wParamFlags == PARAMFLAG_FHASDEFAULT,
            "got: 0x%x\n", edesc->paramdesc.wParamFlags);
    ok(edesc->tdesc.vt == VT_USERDEFINED, "got: %d\n", edesc->tdesc.vt);
    ok(edesc->tdesc.hreftype == hreftype, "got: 0x%lx\n", edesc->tdesc.hreftype);
    ok(V_VT(&edesc->paramdesc.pparamdescex->varDefaultValue) == VT_INT, "got: %d\n",
            V_VT(&edesc->paramdesc.pparamdescex->varDefaultValue));
    ok(V_INT(&edesc->paramdesc.pparamdescex->varDefaultValue) == 0x789, "got: %d\n",
            V_INT(&edesc->paramdesc.pparamdescex->varDefaultValue));

    ITypeInfo2_ReleaseFuncDesc(ti2, pfuncdesc);

    elemdesc[0].tdesc.vt = VT_VARIANT;
    elemdesc[0].paramdesc.pparamdescex = &paramdescex;
    elemdesc[0].paramdesc.wParamFlags = PARAMFLAG_FHASDEFAULT;
    V_VT(&paramdescex.varDefaultValue) = VT_INT;
    V_INT(&paramdescex.varDefaultValue) = 3;

    funcdesc.lprgelemdescParam = elemdesc;
    funcdesc.invkind = INVOKE_FUNC;
    funcdesc.cParams = 1;
    funcdesc.elemdescFunc.tdesc.vt = VT_VARIANT;

    hres = ICreateTypeInfo_AddFuncDesc(createti, 6, &funcdesc);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ITypeInfo2_GetFuncDesc(ti2, 6, &pfuncdesc);
    ok(hres == S_OK, "got %08lx\n", hres);

    ok(pfuncdesc->memid == 0x60010006, "got %lx\n", pfuncdesc->memid);
    ok(pfuncdesc->lprgscode == NULL, "got %p\n", pfuncdesc->lprgscode);
    ok(pfuncdesc->lprgelemdescParam != NULL, "got %p\n", pfuncdesc->lprgelemdescParam);
    ok(pfuncdesc->funckind == FUNC_PUREVIRTUAL, "got 0x%x\n", pfuncdesc->funckind);
    ok(pfuncdesc->invkind == INVOKE_FUNC, "got 0x%x\n", pfuncdesc->invkind);
    ok(pfuncdesc->callconv == CC_STDCALL, "got 0x%x\n", pfuncdesc->callconv);
    ok(pfuncdesc->cParams == 1, "got %d\n", pfuncdesc->cParams);
    ok(pfuncdesc->cParamsOpt == 0, "got %d\n", pfuncdesc->cParamsOpt);
    todo_wine_if(sys != native_sys)
    ok(pfuncdesc->oVft == 9 * ptr_size, "got %d\n", pfuncdesc->oVft);
    ok(pfuncdesc->cScodes == 0, "got %d\n", pfuncdesc->cScodes);
    ok(pfuncdesc->elemdescFunc.tdesc.vt == VT_VARIANT, "got %d\n", pfuncdesc->elemdescFunc.tdesc.vt);
    ok(pfuncdesc->wFuncFlags == 0, "got 0x%x\n", pfuncdesc->wFuncFlags);

    edesc = pfuncdesc->lprgelemdescParam;
    ok(edesc->paramdesc.pparamdescex != NULL, "got: %p\n", edesc->paramdesc.pparamdescex);
    ok(edesc->paramdesc.wParamFlags == PARAMFLAG_FHASDEFAULT,
            "got: 0x%x\n", edesc->paramdesc.wParamFlags);
    ok(edesc->tdesc.vt == VT_VARIANT, "got: %d\n", edesc->tdesc.vt);
    ok(V_VT(&edesc->paramdesc.pparamdescex->varDefaultValue) == VT_INT, "got: %d\n",
            V_VT(&edesc->paramdesc.pparamdescex->varDefaultValue));
    ok(V_INT(&edesc->paramdesc.pparamdescex->varDefaultValue) == 3, "got: %d\n",
            V_INT(&edesc->paramdesc.pparamdescex->varDefaultValue));

    ITypeInfo2_ReleaseFuncDesc(ti2, pfuncdesc);

    hres = ITypeInfo_GetDocumentation(interface1, 0, &name, &docstring, &helpcontext, &helpfile);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(name == NULL, "name != NULL\n");
    ok(docstring == NULL, "docstring != NULL\n");
    ok(helpcontext == 0x201, "helpcontext != 0x201\n");
    ok(!memcmp(helpfile, helpfileW, sizeof(helpfileW)), "helpfile = %s\n", wine_dbgstr_w(helpfile));

    SysFreeString(helpfile);

    hres = ICreateTypeInfo_SetFuncAndParamNames(createti, 1000, NULL, 1);
    ok(hres == E_INVALIDARG, "got %08lx\n", hres);

    hres = ICreateTypeInfo_SetFuncAndParamNames(createti, 1000, names1, 1);
    ok(hres == TYPE_E_ELEMENTNOTFOUND, "got %08lx\n", hres);

    hres = ICreateTypeInfo_SetFuncAndParamNames(createti, 0, names1, 2);
    ok(hres == TYPE_E_ELEMENTNOTFOUND, "got %08lx\n", hres);

    hres = ICreateTypeInfo_SetFuncAndParamNames(createti, 0, names2, 1);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ICreateTypeInfo_SetFuncAndParamNames(createti, 0, names1, 1);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ITypeInfo_GetDocumentation(interface1, 0, &name, NULL, NULL, NULL);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(!memcmp(name, func1W, sizeof(func1W)), "name = %s\n", wine_dbgstr_w(name));

    SysFreeString(name);

    hres = ICreateTypeInfo_SetFuncAndParamNames(createti, 3, names2, 3);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ICreateTypeInfo_SetFuncAndParamNames(createti, 3, names1, 3);
    ok(hres == TYPE_E_AMBIGUOUSNAME, "got %08lx\n", hres);

    ITypeInfo2_Release(ti2);
    ICreateTypeInfo_Release(createti);

    hres = ICreateTypeLib2_CreateTypeInfo(createtl, interface1W, TKIND_INTERFACE, &createti);
    ok(hres == TYPE_E_NAMECONFLICT, "got %08lx\n", hres);

    hres = ICreateTypeLib2_CreateTypeInfo(createtl, interface2W, TKIND_INTERFACE, &createti);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ICreateTypeInfo_SetGuid(createti, &interface2guid);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ICreateTypeInfo_QueryInterface(createti, &IID_ITypeInfo, (void**)&interface2);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ITypeInfo_GetRefTypeOfImplType(interface2, 0, &hreftype);
    ok(hres == TYPE_E_ELEMENTNOTFOUND, "got %08lx\n", hres);

    hres = ICreateTypeInfo_AddRefTypeInfo(createti, interface1, &hreftype);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ITypeInfo_GetRefTypeInfo(interface2, 0, &ti);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(ti == interface1, "Received and added interfaces are different\n");

    ITypeInfo_Release(ti);

    hres = ICreateTypeInfo_AddImplType(createti, 0, hreftype);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ITypeInfo_GetRefTypeOfImplType(interface2, 0, &hreftype);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(hreftype == 2, "hreftype = %ld\n", hreftype);

    hres = ITypeInfo_GetRefTypeOfImplType(interface2, -1, &hreftype);
    ok(hres == TYPE_E_ELEMENTNOTFOUND, "got %08lx\n", hres);

    hres = ICreateTypeInfo_SetImplTypeFlags(createti, 0, IMPLTYPEFLAG_FDEFAULT);
    ok(hres == TYPE_E_BADMODULEKIND, "got %08lx\n", hres);

    hres = ITypeInfo_GetImplTypeFlags(interface2, 0, &impltypeflags);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(impltypeflags == 0, "impltypeflags = %x\n", impltypeflags);

    hres = ITypeInfo_GetImplTypeFlags(interface2, 1, &impltypeflags);
    ok(hres == TYPE_E_ELEMENTNOTFOUND, "got %08lx\n", hres);

    funcdesc.elemdescFunc.tdesc.vt = VT_VOID;
    funcdesc.oVft = 0xaaac;
    hres = ICreateTypeInfo_AddFuncDesc(createti, 0, &funcdesc);
    if(sys == SYS_WIN64){
        ok(hres == E_INVALIDARG, "got %08lx\n", hres);
        funcdesc.oVft = 0xaab0;
        hres = ICreateTypeInfo_AddFuncDesc(createti, 0, &funcdesc);
    }
    ok(hres == S_OK, "got %08lx\n", hres);
    funcdesc.oVft = 0xaaa8;
    hres = ICreateTypeInfo_AddFuncDesc(createti, 0, &funcdesc);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ICreateTypeInfo_QueryInterface(createti, &IID_ITypeInfo, (void**)&ti2);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ITypeInfo2_GetFuncDesc(ti2, 0, &pfuncdesc);
    ok(hres == S_OK, "got %08lx\n", hres);

    ok(pfuncdesc->memid == 0x60020000, "got %lx\n", pfuncdesc->memid);
    ok(pfuncdesc->lprgscode == NULL, "got %p\n", pfuncdesc->lprgscode);
    ok(pfuncdesc->lprgelemdescParam != NULL, "got %p\n", pfuncdesc->lprgelemdescParam);
    ok(pfuncdesc->funckind == FUNC_PUREVIRTUAL, "got 0x%x\n", pfuncdesc->funckind);
    ok(pfuncdesc->invkind == INVOKE_FUNC, "got 0x%x\n", pfuncdesc->invkind);
    ok(pfuncdesc->callconv == CC_STDCALL, "got 0x%x\n", pfuncdesc->callconv);
    ok(pfuncdesc->cParams == 1, "got %d\n", pfuncdesc->cParams);
    ok(pfuncdesc->cParamsOpt == 0, "got %d\n", pfuncdesc->cParamsOpt);
    ok(pfuncdesc->oVft == (short)0xaaa8, "got %d\n", pfuncdesc->oVft);
    ok(pfuncdesc->cScodes == 0, "got %d\n", pfuncdesc->cScodes);
    ok(pfuncdesc->elemdescFunc.tdesc.vt == VT_VOID, "got %d\n", pfuncdesc->elemdescFunc.tdesc.vt);
    ok(pfuncdesc->wFuncFlags == 0, "got 0x%x\n", pfuncdesc->wFuncFlags);

    ITypeInfo2_ReleaseFuncDesc(ti2, pfuncdesc);
    ITypeInfo2_Release(ti2);

    funcdesc.oVft = 0;

    ICreateTypeInfo_Release(createti);

    VariantInit(&cust_data);

    hres = ICreateTypeLib2_CreateTypeInfo(createtl, interface3W, TKIND_INTERFACE, &createti);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ICreateTypeInfo_QueryInterface(createti, &IID_ICreateTypeInfo2, (void**)&createti2);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ICreateTypeInfo2_QueryInterface(createti2, &IID_ITypeInfo2, (void**)&ti2);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ITypeInfo2_GetCustData(ti2, NULL, NULL);
    ok(hres == E_INVALIDARG, "got %08lx\n", hres);

    hres = ITypeInfo2_GetCustData(ti2, &custguid, NULL);
    ok(hres == E_INVALIDARG, "got %08lx\n", hres);

    hres = ITypeInfo2_GetCustData(ti2, &custguid, &cust_data);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ICreateTypeInfo2_SetCustData(createti2, NULL, NULL);
    ok(hres == E_INVALIDARG, "got %08lx\n", hres);

    hres = ICreateTypeInfo2_SetCustData(createti2, &custguid, NULL);
    ok(hres == E_INVALIDARG, "got %08lx\n", hres);

    hres = ICreateTypeInfo2_SetCustData(createti2, &custguid, &cust_data);
    ok(hres == DISP_E_BADVARTYPE, "got %08lx\n", hres);

    V_VT(&cust_data) = VT_UI4;
    V_I4(&cust_data) = 0xdeadbeef;

    hres = ICreateTypeInfo2_SetCustData(createti2, &custguid, &cust_data);
    ok(hres == S_OK, "got %08lx\n", hres);

    V_I4(&cust_data) = 0;
    V_VT(&cust_data) = VT_EMPTY;

    hres = ITypeInfo2_GetCustData(ti2, &custguid, &cust_data);
    ok(hres == S_OK, "got %08lx\n", hres);

    ok(V_VT(&cust_data) == VT_UI4, "got %d\n", V_VT(&cust_data));
    ok(V_I4(&cust_data) == 0xdeadbeef, "got 0x%08lx\n", V_I4(&cust_data));

    V_VT(&cust_data) = VT_UI4;
    V_I4(&cust_data) = 12345678;

    hres = ICreateTypeInfo2_SetCustData(createti2, &custguid, &cust_data);
    ok(hres == S_OK, "got %08lx\n", hres);

    V_I4(&cust_data) = 0;
    V_VT(&cust_data) = VT_EMPTY;

    hres = ITypeInfo2_GetCustData(ti2, &custguid, &cust_data);
    ok(hres == S_OK, "got %08lx\n", hres);

    ok(V_VT(&cust_data) == VT_UI4, "got %d\n", V_VT(&cust_data));
    ok(V_I4(&cust_data) == 12345678, "got 0x%08lx\n", V_I4(&cust_data));

    V_VT(&cust_data) = VT_BSTR;
    V_BSTR(&cust_data) = SysAllocString(asdfW);

    hres = ICreateTypeInfo2_SetCustData(createti2, &custguid, &cust_data);
    ok(hres == S_OK, "got %08lx\n", hres);

    SysFreeString(V_BSTR(&cust_data));
    V_I4(&cust_data) = 0;
    V_VT(&cust_data) = VT_EMPTY;

    hres = ITypeInfo2_GetCustData(ti2, &custguid, &cust_data);
    ok(hres == S_OK, "got %08lx\n", hres);

    ok(V_VT(&cust_data) == VT_BSTR, "got %d\n", V_VT(&cust_data));
    ok(!lstrcmpW(V_BSTR(&cust_data), asdfW), "got %s\n", wine_dbgstr_w(V_BSTR(&cust_data)));
    SysFreeString(V_BSTR(&cust_data));

    V_VT(&cust_data) = VT_UI4;
    V_UI4(&cust_data) = 17;

    hres = ITypeInfo2_GetCustData(ti2, &bogusguid, &cust_data);
    ok(hres == S_OK, "got %08lx\n", hres);

    ok(V_VT(&cust_data) == VT_EMPTY, "got: %d\n", V_VT(&cust_data));

    ITypeInfo2_Release(ti2);
    ICreateTypeInfo2_Release(createti2);
    ICreateTypeInfo_Release(createti);

    hres = ICreateTypeLib2_CreateTypeInfo(createtl, coclassW, TKIND_COCLASS, &createti);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ICreateTypeInfo_AddRefTypeInfo(createti, interface1, &hreftype);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ICreateTypeInfo_AddImplType(createti, 0, hreftype);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ICreateTypeInfo_AddImplType(createti, 0, hreftype);
    ok(hres == TYPE_E_ELEMENTNOTFOUND, "got %08lx\n", hres);

    hres = ICreateTypeInfo_AddRefTypeInfo(createti, unknown, &hreftype);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ICreateTypeInfo_AddImplType(createti, 1, hreftype);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ICreateTypeInfo_AddImplType(createti, 1, hreftype);
    ok(hres == TYPE_E_ELEMENTNOTFOUND, "got %08lx\n", hres);

    hres = ICreateTypeInfo_AddImplType(createti, 2, hreftype);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ICreateTypeInfo_SetImplTypeFlags(createti, 0, IMPLTYPEFLAG_FDEFAULT);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ICreateTypeInfo_SetImplTypeFlags(createti, 1, IMPLTYPEFLAG_FRESTRICTED);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ICreateTypeInfo_QueryInterface(createti, &IID_ITypeInfo, (void**)&ti);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ITypeInfo_GetImplTypeFlags(ti, 0, NULL);
    ok(hres == E_INVALIDARG, "got %08lx\n", hres);

    hres = ITypeInfo_GetImplTypeFlags(ti, 0, &impltypeflags);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(impltypeflags == IMPLTYPEFLAG_FDEFAULT, "impltypeflags = %x\n", impltypeflags);

    hres = ITypeInfo_GetImplTypeFlags(ti, 1, &impltypeflags);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(impltypeflags == IMPLTYPEFLAG_FRESTRICTED, "impltypeflags = %x\n", impltypeflags);

    hres = ITypeInfo_GetImplTypeFlags(ti, 2, &impltypeflags);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(impltypeflags == 0, "impltypeflags = %x\n", impltypeflags);

    hres = ITypeInfo_GetRefTypeOfImplType(ti, 0, &hreftype);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(hreftype == 0, "hreftype = %ld\n", hreftype);

    hres = ITypeInfo_GetRefTypeOfImplType(ti, 1, &hreftype);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(hreftype == 1, "hreftype = %ld\n", hreftype);

    hres = ITypeInfo_GetRefTypeOfImplType(ti, 2, &hreftype);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(hreftype == 1, "hreftype = %ld\n", hreftype);

    hres = ITypeInfo_GetRefTypeOfImplType(ti, -1, &hreftype);
    ok(hres == TYPE_E_ELEMENTNOTFOUND, "got %08lx\n", hres);

    ITypeInfo_Release(ti);

    ICreateTypeInfo_Release(createti);

    hres = ICreateTypeLib2_CreateTypeInfo(createtl, dualW, TKIND_INTERFACE, &createti);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ICreateTypeInfo_SetTypeFlags(createti, TYPEFLAG_FDUAL);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ICreateTypeInfo_AddFuncDesc(createti, 0, &funcdesc);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ICreateTypeInfo_AddRefTypeInfo(createti, dispatch, &hreftype);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ICreateTypeInfo_AddImplType(createti, 0, hreftype);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ICreateTypeInfo_QueryInterface(createti, &IID_ITypeInfo, (void**)&dual);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ITypeInfo_GetTypeAttr(dual, &typeattr);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(typeattr->cbSizeInstance == ptr_size, "cbSizeInstance = %ld\n", typeattr->cbSizeInstance);
    ok(typeattr->typekind == TKIND_INTERFACE, "typekind = %d\n", typeattr->typekind);
    ok(typeattr->cFuncs == 1, "cFuncs = %d\n", typeattr->cFuncs);
    ok(typeattr->cVars == 0, "cVars = %d\n", typeattr->cVars);
    ok(typeattr->cImplTypes == 1, "cImplTypes = %d\n", typeattr->cImplTypes);
    todo_wine_if(sys != native_sys)
    ok(typeattr->cbSizeVft == 8 * ptr_size, "cbSizeVft = %d\n", typeattr->cbSizeVft);
    ok(typeattr->cbAlignment == 4, "cbAlignment = %d\n", typeattr->cbAlignment);
    ok(typeattr->wTypeFlags == (TYPEFLAG_FDISPATCHABLE|TYPEFLAG_FDUAL), "wTypeFlags = %d\n", typeattr->wTypeFlags);
    ok(typeattr->wMajorVerNum == 0, "wMajorVerNum = %d\n", typeattr->wMajorVerNum);
    ok(typeattr->wMinorVerNum == 0, "wMinorVerNum = %d\n", typeattr->wMinorVerNum);

    ITypeInfo_ReleaseTypeAttr(dual, typeattr);

    hres = ITypeInfo_GetRefTypeOfImplType(dual, -1, &hreftype);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(hreftype == -2, "got %08lx\n", hreftype);

    EXPECT_REF(dual, 2);
    hres = ITypeInfo_GetRefTypeInfo(dual, -2, &ti);
    ok(hres == S_OK, "got %08lx\n", hres);
    todo_wine
    EXPECT_REF(dual, 3);

    hres = ITypeInfo_GetTypeAttr(ti, &typeattr);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(typeattr->cbSizeInstance == ptr_size, "cbSizeInstance = %ld\n", typeattr->cbSizeInstance);
    ok(typeattr->typekind == TKIND_DISPATCH, "typekind = %d\n", typeattr->typekind);
    todo_wine_if(sys != native_sys)
    ok(typeattr->cFuncs == 8, "cFuncs = %d\n", typeattr->cFuncs);
    ok(typeattr->cVars == 0, "cVars = %d\n", typeattr->cVars);
    ok(typeattr->cImplTypes == 1, "cImplTypes = %d\n", typeattr->cImplTypes);
    ok(typeattr->cbSizeVft == 7 * sizeof(void *), "cbSizeVft = %d\n", typeattr->cbSizeVft);
    ok(typeattr->cbAlignment == 4, "cbAlignment = %d\n", typeattr->cbAlignment);
    ok(typeattr->wTypeFlags == (TYPEFLAG_FDISPATCHABLE|TYPEFLAG_FDUAL), "wTypeFlags = %d\n", typeattr->wTypeFlags);
    ok(typeattr->wMajorVerNum == 0, "wMajorVerNum = %d\n", typeattr->wMajorVerNum);
    ok(typeattr->wMinorVerNum == 0, "wMinorVerNum = %d\n", typeattr->wMinorVerNum);

    ITypeInfo_ReleaseTypeAttr(ti, typeattr);

    hres = ITypeInfo_GetRefTypeInfo(dual, -2, &ti_2);
    ok(hres == S_OK, "Failed to get reference typeinfo, hr %#lx.\n", hres);
todo_wine {
    ok(ti == ti_2, "Unexpected typeinfo instance.\n");
    EXPECT_REF(dual, 4);
}
    ITypeInfo_AddRef(ti_2);
    todo_wine
    EXPECT_REF(dual, 5);
    ITypeInfo_Release(ti_2);

    ITypeInfo_Release(ti_2);
    ITypeInfo_Release(ti);

    hres = ICreateTypeInfo_SetTypeDescAlias(createti, &typedesc1);
    ok(hres == TYPE_E_BADMODULEKIND, "got %08lx\n", hres);

    ICreateTypeInfo_Release(createti);

    hres = ITypeInfo_GetTypeAttr(interface1, &typeattr);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(typeattr->cbSizeInstance == ptr_size, "cbSizeInstance = %ld\n", typeattr->cbSizeInstance);
    ok(typeattr->typekind == TKIND_INTERFACE, "typekind = %d\n", typeattr->typekind);
    ok(typeattr->cFuncs == 13, "cFuncs = %d\n", typeattr->cFuncs);
    ok(typeattr->cVars == 0, "cVars = %d\n", typeattr->cVars);
    ok(typeattr->cImplTypes == 1, "cImplTypes = %d\n", typeattr->cImplTypes);
    todo_wine_if(sys != native_sys)
    ok(typeattr->cbSizeVft == 16 * ptr_size, "cbSizeVft = %d\n", typeattr->cbSizeVft);
    ok(typeattr->cbAlignment == 4, "cbAlignment = %d\n", typeattr->cbAlignment);
    ok(typeattr->wTypeFlags == 0, "wTypeFlags = %d\n", typeattr->wTypeFlags);
    ok(typeattr->wMajorVerNum == 0, "wMajorVerNum = %d\n", typeattr->wMajorVerNum);
    ok(typeattr->wMinorVerNum == 0, "wMinorVerNum = %d\n", typeattr->wMinorVerNum);

    ITypeInfo_ReleaseTypeAttr(interface1, typeattr);

    hres = ITypeInfo_GetTypeAttr(interface2, &typeattr);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(typeattr->cbSizeInstance == ptr_size, "cbSizeInstance = %ld\n", typeattr->cbSizeInstance);
    ok(typeattr->typekind == TKIND_INTERFACE, "typekind = %d\n", typeattr->typekind);
    ok(typeattr->cFuncs == 2, "cFuncs = %d\n", typeattr->cFuncs);
    ok(typeattr->cVars == 0, "cVars = %d\n", typeattr->cVars);
    ok(typeattr->cImplTypes == 1, "cImplTypes = %d\n", typeattr->cImplTypes);
    ok((sys == SYS_WIN32 && typeattr->cbSizeVft == 0xaab0) ||
            (sys == SYS_WIN64 && typeattr->cbSizeVft == 0xaab8),
            "cbSizeVft = 0x%x\n", typeattr->cbSizeVft);
    ok(typeattr->cbAlignment == 4, "cbAlignment = %d\n", typeattr->cbAlignment);
    ok(typeattr->wTypeFlags == 0, "wTypeFlags = %d\n", typeattr->wTypeFlags);
    ok(typeattr->wMajorVerNum == 0, "wMajorVerNum = %d\n", typeattr->wMajorVerNum);
    ok(typeattr->wMinorVerNum == 0, "wMinorVerNum = %d\n", typeattr->wMinorVerNum);

    ITypeInfo_ReleaseTypeAttr(interface2, typeattr);

    ok(ITypeInfo_Release(interface2)==0, "Object should be freed\n");
    ok(ITypeInfo_Release(interface1)==0, "Object should be freed\n");
    ok(ITypeInfo_Release(dual)==0, "Object should be freed\n");

    hres = ICreateTypeLib2_CreateTypeInfo(createtl, aliasW, TKIND_ALIAS, &createti);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ICreateTypeInfo_QueryInterface(createti, &IID_ITypeInfo, (void**)&interface1);
    ok(hres == S_OK, "got %08lx\n", hres);

    if(0){
        /* windows gives invalid values here, and even breaks the typeinfo permanently
         * on winxp. only call GetTypeAttr() on a TKIND_ALIAS after SetTypeDescAlias. */
        hres = ITypeInfo_GetTypeAttr(interface1, &typeattr);
        ok(hres == S_OK, "got %08lx\n", hres);
        ok(typeattr->cbSizeInstance == 0xffffffb4, "cbSizeInstance = %ld\n", typeattr->cbSizeInstance);
        ok(typeattr->typekind == TKIND_ALIAS, "typekind = %d\n", typeattr->typekind);
        ok(typeattr->cFuncs == 0, "cFuncs = %d\n", typeattr->cFuncs);
        ok(typeattr->cVars == 0, "cVars = %d\n", typeattr->cVars);
        ok(typeattr->cImplTypes == 0, "cImplTypes = %d\n", typeattr->cImplTypes);
        ok(typeattr->cbSizeVft == 0, "cbSizeVft = %d\n", typeattr->cbSizeVft);
        ok(typeattr->cbAlignment == 0, "cbAlignment = %d\n", typeattr->cbAlignment);
        ok(typeattr->wTypeFlags == 0, "wTypeFlags = %d\n", typeattr->wTypeFlags);
        ok(typeattr->wMajorVerNum == 0, "wMajorVerNum = %d\n", typeattr->wMajorVerNum);
        ok(typeattr->wMinorVerNum == 0, "wMinorVerNum = %d\n", typeattr->wMinorVerNum);
        ok(typeattr->tdescAlias.vt == VT_EMPTY, "Got wrong tdescAlias.vt: %u\n", typeattr->tdescAlias.vt);
        ITypeInfo_ReleaseTypeAttr(interface1, typeattr);
    }

    hres = ICreateTypeInfo_SetTypeDescAlias(createti, NULL);
    ok(hres == E_INVALIDARG, "got %08lx\n", hres);

    typedesc1.vt = VT_I1;
    hres = ICreateTypeInfo_SetTypeDescAlias(createti, &typedesc1);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ITypeInfo_GetTypeAttr(interface1, &typeattr);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(typeattr->cbSizeInstance == 1, "cbSizeInstance = %ld\n", typeattr->cbSizeInstance);
    ok(typeattr->typekind == TKIND_ALIAS, "typekind = %d\n", typeattr->typekind);
    ok(typeattr->cFuncs == 0, "cFuncs = %d\n", typeattr->cFuncs);
    ok(typeattr->cVars == 0, "cVars = %d\n", typeattr->cVars);
    ok(typeattr->cImplTypes == 0, "cImplTypes = %d\n", typeattr->cImplTypes);
    ok(typeattr->cbSizeVft == 0, "cbSizeVft = %d\n", typeattr->cbSizeVft);
    ok(typeattr->cbAlignment == 1, "cbAlignment = %d\n", typeattr->cbAlignment);
    ok(typeattr->wTypeFlags == 0, "wTypeFlags = %d\n", typeattr->wTypeFlags);
    ok(typeattr->wMajorVerNum == 0, "wMajorVerNum = %d\n", typeattr->wMajorVerNum);
    ok(typeattr->wMinorVerNum == 0, "wMinorVerNum = %d\n", typeattr->wMinorVerNum);
    ok(typeattr->tdescAlias.vt == VT_I1, "Got wrong tdescAlias.vt: %u\n", typeattr->tdescAlias.vt);
    ITypeInfo_ReleaseTypeAttr(interface1, typeattr);

    typedesc1.vt = VT_R8;
    hres = ICreateTypeInfo_SetTypeDescAlias(createti, &typedesc1);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ITypeInfo_GetTypeAttr(interface1, &typeattr);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(typeattr->cbSizeInstance == 8, "cbSizeInstance = %ld\n", typeattr->cbSizeInstance);
    ok(typeattr->typekind == TKIND_ALIAS, "typekind = %d\n", typeattr->typekind);
    ok(typeattr->cFuncs == 0, "cFuncs = %d\n", typeattr->cFuncs);
    ok(typeattr->cVars == 0, "cVars = %d\n", typeattr->cVars);
    ok(typeattr->cImplTypes == 0, "cImplTypes = %d\n", typeattr->cImplTypes);
    ok(typeattr->cbSizeVft == 0, "cbSizeVft = %d\n", typeattr->cbSizeVft);
    ok(typeattr->cbAlignment == 4, "cbAlignment = %d\n", typeattr->cbAlignment);
    ok(typeattr->wTypeFlags == 0, "wTypeFlags = %d\n", typeattr->wTypeFlags);
    ok(typeattr->wMajorVerNum == 0, "wMajorVerNum = %d\n", typeattr->wMajorVerNum);
    ok(typeattr->wMinorVerNum == 0, "wMinorVerNum = %d\n", typeattr->wMinorVerNum);
    ok(typeattr->tdescAlias.vt == VT_R8, "Got wrong tdescAlias.vt: %u\n", typeattr->tdescAlias.vt);
    ITypeInfo_ReleaseTypeAttr(interface1, typeattr);

    ITypeInfo_Release(interface1);
    ICreateTypeInfo_Release(createti);

    hres = ICreateTypeLib2_SaveAllChanges(createtl);
    ok(hres == S_OK, "got %08lx\n", hres);

    ok(ICreateTypeLib2_Release(createtl)==0, "Object should be freed\n");

    ok(ITypeInfo_Release(dispatch)==0, "Object should be freed\n");
    ok(ITypeInfo_Release(unknown)==0, "Object should be freed\n");
    ok(ITypeLib_Release(stdole)==0, "Object should be freed\n");

    hres = LoadTypeLibEx(filenameW, REGKIND_NONE, &tl);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ITypeLib_GetLibAttr(tl, &libattr);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(libattr->syskind == sys, "syskind = %d\n", libattr->syskind);
    ok(libattr->wMajorVerNum == 0, "wMajorVer = %d\n", libattr->wMajorVerNum);
    ok(libattr->wMinorVerNum == 0, "wMinorVerNum = %d\n", libattr->wMinorVerNum);
    ok(libattr->wLibFlags == LIBFLAG_FHASDISKIMAGE, "wLibFlags = %d\n", libattr->wLibFlags);
    ITypeLib_ReleaseTLibAttr(tl, libattr);

    found = 2;
    memset(tinfos, 0, sizeof(tinfos));
    memids[0] = 0xdeadbeef;
    memids[1] = 0xdeadbeef;
    hres = ITypeLib_FindName(tl, param1W, 0, tinfos, memids, &found);
    ok(hres == S_OK, "got: %08lx\n", hres);
    ok(found == 0, "got wrong count: %u\n", found);
    ok(tinfos[0] == NULL, "got invalid typeinfo[0]\n");
    ok(tinfos[1] == NULL, "got invalid typeinfo[1]\n");
    ok(memids[0] == 0xdeadbeef, "got invalid memid[0]\n");
    ok(memids[1] == 0xdeadbeef, "got invalid memid[1]\n");

    found = 2;
    memset(tinfos, 0, sizeof(tinfos));
    memids[0] = 0xdeadbeef;
    memids[1] = 0xdeadbeef;
    hres = ITypeLib_FindName(tl, func1W, 0, tinfos, memids, &found);
    ok(hres == S_OK, "got: %08lx\n", hres);
    ok(found == 1, "got wrong count: %u\n", found);
    ok(tinfos[0] != NULL, "got invalid typeinfo[0]\n");
    ok(tinfos[1] == NULL, "got invalid typeinfo[1]\n");
    ok(memids[0] == 0, "got invalid memid[0]\n");
    ok(memids[1] == 0xdeadbeef, "got invalid memid[1]\n");
    if(tinfos[0])
        ITypeInfo_Release(tinfos[0]);

    found = 2;
    memset(tinfos, 0, sizeof(tinfos));
    memids[0] = 0xdeadbeef;
    memids[1] = 0xdeadbeef;
    hres = ITypeLib_FindName(tl, interface1W, 0, tinfos, memids, &found);
    ok(hres == S_OK, "got: %08lx\n", hres);
    ok(found == 1, "got wrong count: %u\n", found);
    ok(tinfos[0] != NULL, "got invalid typeinfo[0]\n");
    ok(tinfos[1] == NULL, "got invalid typeinfo[1]\n");
    ok(memids[0] == MEMBERID_NIL, "got invalid memid[0]: %lx\n", memids[0]);
    ok(memids[1] == 0xdeadbeef, "got invalid memid[1]\n");
    if(tinfos[0])
        ITypeInfo_Release(tinfos[0]);

    hres = ITypeLib_GetDocumentation(tl, -1, &name, &docstring, &helpcontext, &helpfile);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(memcmp(typelibW, name, sizeof(typelibW)) == 0, "got wrong typelib name: %s\n",
            wine_dbgstr_w(name));
    ok(docstring == NULL, "got wrong docstring: %s\n", wine_dbgstr_w(docstring));
    ok(helpcontext == 0, "got wrong helpcontext: 0x%lx\n", helpcontext);
    ok(memcmp(helpfileW, helpfile, sizeof(helpfileW)) == 0,
            "got wrong helpfile: %s\n", wine_dbgstr_w(helpfile));
    SysFreeString(name);
    SysFreeString(helpfile);

    hres = ITypeLib_GetDocumentation(tl, 0, &name, &docstring, &helpcontext, &helpfile);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(memcmp(interface1W, name, sizeof(interface1W)) == 0, "got wrong typeinfo name: %s\n",
            wine_dbgstr_w(name));
    ok(docstring == NULL, "got wrong docstring: %s\n", wine_dbgstr_w(docstring));
    ok(helpcontext == 0, "got wrong helpcontext: 0x%lx\n", helpcontext);
    ok(memcmp(helpfileW, helpfile, sizeof(helpfileW)) == 0,
            "got wrong helpfile: %s\n", wine_dbgstr_w(helpfile));
    SysFreeString(name);
    SysFreeString(helpfile);

    hres = ITypeLib_QueryInterface(tl, &IID_ITypeLib2, (void**)&tl2);
    ok(hres == S_OK, "no ITypeLib2 interface (%lx)\n", hres);
    V_VT(&cust_data) = VT_EMPTY;
    V_I4(&cust_data) = 0;
    hres = ITypeLib2_GetCustData(tl2, &tlcustguid, &cust_data);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(V_VT(&cust_data) == VT_I4, "V_VT(&cust_data) = %d\n", V_VT(&cust_data));
    ok(V_I4(&cust_data) == 1, "V_I4(&cust_data) = %ld\n", V_I4(&cust_data));
    ITypeLib2_Release(tl2);

    hres = ITypeLib_GetTypeInfo(tl, 0, &ti);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ITypeInfo_GetTypeAttr(ti, &typeattr);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(typeattr->cbSizeInstance == sizeof(void*), "cbSizeInstance = %ld\n", typeattr->cbSizeInstance);
    ok(typeattr->typekind == TKIND_INTERFACE, "typekind = %d\n", typeattr->typekind);
    ok(typeattr->cFuncs == 13, "cFuncs = %d\n", typeattr->cFuncs);
    ok(typeattr->cVars == 0, "cVars = %d\n", typeattr->cVars);
    ok(typeattr->cImplTypes == 1, "cImplTypes = %d\n", typeattr->cImplTypes);
    todo_wine_if(sys != native_sys)
        ok(typeattr->cbSizeVft == 16 * sizeof(void*), "cbSizeVft = %d\n", typeattr->cbSizeVft);
    ok(typeattr->cbAlignment == alignment, "cbAlignment = %d\n", typeattr->cbAlignment);
    ok(typeattr->wTypeFlags == 0, "wTypeFlags = %d\n", typeattr->wTypeFlags);
    ok(typeattr->wMajorVerNum == 0, "wMajorVerNum = %d\n", typeattr->wMajorVerNum);
    ok(typeattr->wMinorVerNum == 0, "wMinorVerNum = %d\n", typeattr->wMinorVerNum);
    ITypeInfo_ReleaseTypeAttr(ti, typeattr);

    hres = ITypeInfo_GetRefTypeOfImplType(ti, 0, &hreftype);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(hreftype == 3, "hreftype = %ld\n", hreftype);

    hres = ITypeInfo_GetRefTypeInfo(ti, hreftype, &unknown);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ITypeInfo_GetTypeAttr(unknown, &typeattr);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(IsEqualGUID(&typeattr->guid, &IID_IUnknown), "got wrong reftypeinfo\n");
    ITypeInfo_ReleaseTypeAttr(unknown, typeattr);

    ITypeInfo_Release(unknown);

    hres = ITypeInfo_GetFuncDesc(ti, 0, &pfuncdesc);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(pfuncdesc->memid == 0, "got %lx\n", pfuncdesc->memid);
    ok(pfuncdesc->lprgscode == NULL, "got %p\n", pfuncdesc->lprgscode);
    ok(pfuncdesc->lprgelemdescParam != NULL, "got %p\n", pfuncdesc->lprgelemdescParam);
    ok(pfuncdesc->funckind == FUNC_PUREVIRTUAL, "got 0x%x\n", pfuncdesc->funckind);
    ok(pfuncdesc->invkind == INVOKE_PROPERTYPUTREF, "got 0x%x\n", pfuncdesc->invkind);
    ok(pfuncdesc->callconv == CC_STDCALL, "got 0x%x\n", pfuncdesc->callconv);
    ok(pfuncdesc->cParams == 1, "got %d\n", pfuncdesc->cParams);
    ok(pfuncdesc->cParamsOpt == 0, "got %d\n", pfuncdesc->cParamsOpt);
    ok(pfuncdesc->cScodes == 0, "got %d\n", pfuncdesc->cScodes);
    ok(pfuncdesc->elemdescFunc.tdesc.vt == VT_VOID, "got %d\n", pfuncdesc->elemdescFunc.tdesc.vt);
    ok(pfuncdesc->wFuncFlags == 0, "got 0x%x\n", pfuncdesc->wFuncFlags);
    edesc = pfuncdesc->lprgelemdescParam;
    ok(edesc->tdesc.vt == VT_BSTR, "got: %d\n", edesc->tdesc.vt);
    ok(edesc->idldesc.wIDLFlags == IDLFLAG_FIN, "got: %x\n", edesc->idldesc.wIDLFlags);

    hres = ITypeInfo_GetDocumentation(ti, pfuncdesc->memid, &name, &docstring, &helpcontext, &helpfile);
    ok(hres == S_OK, "got: %08lx\n", hres);
    ok(!memcmp(name, func1W, sizeof(func1W)), "got name: %s\n", wine_dbgstr_w(name));
    ok(docstring == NULL, "got docstring: %s\n", wine_dbgstr_w(docstring));
    ok(helpcontext == 0x201, "got helpcontext: 0x%lx\n", helpcontext);
    ok(!memcmp(helpfile, helpfileW, sizeof(helpfileW)), "got helpfile: %s\n", wine_dbgstr_w(helpfile));
    SysFreeString(name);
    SysFreeString(helpfile);

    hres = ITypeInfo_GetNames(ti, pfuncdesc->memid, NULL, 0, &cnames);
    ok(hres == E_INVALIDARG, "got: %08lx\n", hres);

    cnames = 8;
    hres = ITypeInfo_GetNames(ti, pfuncdesc->memid, names, 0, &cnames);
    ok(hres == S_OK, "got: %08lx\n", hres);
    ok(cnames == 0, "got: %u\n", cnames);

    hres = ITypeInfo_GetNames(ti, pfuncdesc->memid, names, ARRAY_SIZE(names), &cnames);
    ok(hres == S_OK, "got: %08lx\n", hres);
    ok(cnames == 1, "got: %u\n", cnames);
    ok(!memcmp(names[0], func1W, sizeof(func1W)), "got names[0]: %s\n", wine_dbgstr_w(names[0]));
    SysFreeString(names[0]);

    ITypeInfo_ReleaseFuncDesc(ti, pfuncdesc);

    hres = ITypeInfo_GetFuncDesc(ti, 1, &pfuncdesc);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(pfuncdesc->memid == 0x60010001, "got %lx\n", pfuncdesc->memid);
    ok(pfuncdesc->lprgscode == NULL, "got %p\n", pfuncdesc->lprgscode);
    ok(pfuncdesc->lprgelemdescParam == NULL, "got %p\n", pfuncdesc->lprgelemdescParam);
    ok(pfuncdesc->funckind == FUNC_PUREVIRTUAL, "got 0x%x\n", pfuncdesc->funckind);
    ok(pfuncdesc->invkind == INVOKE_FUNC, "got 0x%x\n", pfuncdesc->invkind);
    ok(pfuncdesc->callconv == CC_STDCALL, "got 0x%x\n", pfuncdesc->callconv);
    ok(pfuncdesc->cParams == 0, "got %d\n", pfuncdesc->cParams);
    ok(pfuncdesc->cParamsOpt == 0, "got %d\n", pfuncdesc->cParamsOpt);
    todo_wine_if(sys != native_sys)
    ok(pfuncdesc->oVft == 4 * sizeof(void*), "got %d\n", pfuncdesc->oVft);
    ok(pfuncdesc->cScodes == 0, "got %d\n", pfuncdesc->cScodes);
    ok(pfuncdesc->elemdescFunc.tdesc.vt == VT_VOID, "got %d\n", pfuncdesc->elemdescFunc.tdesc.vt);
    ok(pfuncdesc->wFuncFlags == 0, "got 0x%x\n", pfuncdesc->wFuncFlags);

    hres = ITypeInfo_GetDocumentation(ti, pfuncdesc->memid, &name, &docstring, &helpcontext, &helpfile);
    ok(hres == S_OK, "got: %08lx\n", hres);
    ok(name == NULL, "got name: %s\n", wine_dbgstr_w(name));
    ok(docstring == NULL, "got docstring: %s\n", wine_dbgstr_w(docstring));
    ok(helpcontext == 0, "got helpcontext: 0x%lx\n", helpcontext);
    ok(!memcmp(helpfile, helpfileW, sizeof(helpfileW)), "got helpfile: %s\n", wine_dbgstr_w(helpfile));
    SysFreeString(helpfile);
    ITypeInfo_ReleaseFuncDesc(ti, pfuncdesc);

    hres = ITypeInfo_GetFuncDesc(ti, 2, &pfuncdesc);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(pfuncdesc->memid == 0x1, "got %lx\n", pfuncdesc->memid);
    ok(pfuncdesc->lprgscode == NULL, "got %p\n", pfuncdesc->lprgscode);
    ok(pfuncdesc->lprgelemdescParam == NULL, "got %p\n", pfuncdesc->lprgelemdescParam);
    ok(pfuncdesc->funckind == FUNC_PUREVIRTUAL, "got 0x%x\n", pfuncdesc->funckind);
    ok(pfuncdesc->invkind == INVOKE_FUNC, "got 0x%x\n", pfuncdesc->invkind);
    ok(pfuncdesc->callconv == CC_STDCALL, "got 0x%x\n", pfuncdesc->callconv);
    ok(pfuncdesc->cParams == 0, "got %d\n", pfuncdesc->cParams);
    ok(pfuncdesc->cParamsOpt == 0, "got %d\n", pfuncdesc->cParamsOpt);
    todo_wine_if(sys != native_sys)
    ok(pfuncdesc->oVft == 5 * sizeof(void*), "got %d\n", pfuncdesc->oVft);
    ok(pfuncdesc->cScodes == 0, "got %d\n", pfuncdesc->cScodes);
    ok(pfuncdesc->elemdescFunc.tdesc.vt == VT_VOID, "got %d\n", pfuncdesc->elemdescFunc.tdesc.vt);
    ok(pfuncdesc->wFuncFlags == 0, "got 0x%x\n", pfuncdesc->wFuncFlags);

    hres = ITypeInfo_GetDocumentation(ti, pfuncdesc->memid, &name, &docstring, &helpcontext, &helpfile);
    ok(hres == S_OK, "got: %08lx\n", hres);
    ok(name == NULL, "got name: %s\n", wine_dbgstr_w(name));
    ok(docstring == NULL, "got docstring: %s\n", wine_dbgstr_w(docstring));
    ok(helpcontext == 0, "got helpcontext: 0x%lx\n", helpcontext);
    ok(!memcmp(helpfile, helpfileW, sizeof(helpfileW)), "got helpfile: %s\n", wine_dbgstr_w(helpfile));
    SysFreeString(helpfile);
    ITypeInfo_ReleaseFuncDesc(ti, pfuncdesc);

    hres = ITypeInfo_GetFuncDesc(ti, 3, &pfuncdesc);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(pfuncdesc->memid == 0x6001000b, "got %lx\n", pfuncdesc->memid);
    ok(pfuncdesc->lprgscode == NULL, "got %p\n", pfuncdesc->lprgscode);
    ok(pfuncdesc->lprgelemdescParam != NULL, "got %p\n", pfuncdesc->lprgelemdescParam);
    ok(pfuncdesc->funckind == FUNC_PUREVIRTUAL, "got 0x%x\n", pfuncdesc->funckind);
    ok(pfuncdesc->invkind == INVOKE_FUNC, "got 0x%x\n", pfuncdesc->invkind);
    ok(pfuncdesc->callconv == CC_STDCALL, "got 0x%x\n", pfuncdesc->callconv);
    ok(pfuncdesc->cParams == 2, "got %d\n", pfuncdesc->cParams);
    ok(pfuncdesc->cParamsOpt == 0, "got %d\n", pfuncdesc->cParamsOpt);
    todo_wine_if(sys != native_sys)
    ok(pfuncdesc->oVft == 6 * sizeof(void*), "got %d\n", pfuncdesc->oVft);
    ok(pfuncdesc->cScodes == 0, "got %d\n", pfuncdesc->cScodes);
    ok(pfuncdesc->elemdescFunc.tdesc.vt == VT_VOID, "got %d\n", pfuncdesc->elemdescFunc.tdesc.vt);
    ok(pfuncdesc->wFuncFlags == 0, "got 0x%x\n", pfuncdesc->wFuncFlags);

    edesc = pfuncdesc->lprgelemdescParam;
    ok(edesc->tdesc.vt == VT_BSTR, "got: %d\n", edesc->tdesc.vt);
    ok(edesc->paramdesc.wParamFlags == PARAMFLAG_FHASDEFAULT, "got: 0x%x\n", edesc->paramdesc.wParamFlags);
    ok(edesc->paramdesc.pparamdescex != NULL, "got: %p\n", edesc->paramdesc.pparamdescex);
    ok(edesc->paramdesc.pparamdescex->cBytes == sizeof(PARAMDESCEX), "got: %ld\n",
            edesc->paramdesc.pparamdescex->cBytes);
    ok(V_VT(&edesc->paramdesc.pparamdescex->varDefaultValue) == VT_BSTR, "got: %d\n",
            V_VT(&edesc->paramdesc.pparamdescex->varDefaultValue));
    ok(compare_wstr(V_BSTR(&edesc->paramdesc.pparamdescex->varDefaultValue), defaultW),
            "got: %s\n",
            wine_dbgstr_w(V_BSTR(&edesc->paramdesc.pparamdescex->varDefaultValue)));

    edesc = pfuncdesc->lprgelemdescParam + 1;
    ok(edesc->tdesc.vt == VT_BSTR, "got: %d\n", edesc->tdesc.vt);
    ok(edesc->paramdesc.wParamFlags == PARAMFLAG_FHASDEFAULT, "got: 0x%x\n", edesc->paramdesc.wParamFlags);
    ok(edesc->paramdesc.pparamdescex != NULL, "got: %p\n", edesc->paramdesc.pparamdescex);
    ok(edesc->paramdesc.pparamdescex->cBytes == sizeof(PARAMDESCEX), "got: %ld\n",
            edesc->paramdesc.pparamdescex->cBytes);
    ok(V_VT(&edesc->paramdesc.pparamdescex->varDefaultValue) == VT_BSTR, "got: %d\n",
            V_VT(&edesc->paramdesc.pparamdescex->varDefaultValue));
    ok(compare_wstr(V_BSTR(&edesc->paramdesc.pparamdescex->varDefaultValue), defaultW),
            "got: %s\n",
            wine_dbgstr_w(V_BSTR(&edesc->paramdesc.pparamdescex->varDefaultValue)));

    hres = ITypeInfo_GetDocumentation(ti, pfuncdesc->memid, &name, &docstring, &helpcontext, &helpfile);
    ok(hres == S_OK, "got: %08lx\n", hres);
    ok(!memcmp(name, func2W, sizeof(func2W)), "got name: %s\n", wine_dbgstr_w(name));
    ok(docstring == NULL, "got docstring: %s\n", wine_dbgstr_w(docstring));
    ok(helpcontext == 0, "got helpcontext: 0x%lx\n", helpcontext);
    ok(!memcmp(helpfile, helpfileW, sizeof(helpfileW)), "got helpfile: %s\n", wine_dbgstr_w(helpfile));
    SysFreeString(name);
    SysFreeString(helpfile);

    hres = ITypeInfo_GetNames(ti, pfuncdesc->memid, names, ARRAY_SIZE(names), &cnames);
    ok(hres == S_OK, "got: %08lx\n", hres);
    ok(cnames == 3, "got: %u\n", cnames);
    ok(!memcmp(names[0], func2W, sizeof(func2W)), "got names[0]: %s\n", wine_dbgstr_w(names[0]));
    ok(!memcmp(names[1], param1W, sizeof(func2W)), "got names[1]: %s\n", wine_dbgstr_w(names[1]));
    ok(!memcmp(names[2], param2W, sizeof(func2W)), "got names[2]: %s\n", wine_dbgstr_w(names[2]));
    SysFreeString(names[0]);
    SysFreeString(names[1]);
    SysFreeString(names[2]);
    ITypeInfo_ReleaseFuncDesc(ti, pfuncdesc);

    hres = ITypeInfo_GetFuncDesc(ti, 4, &pfuncdesc);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(pfuncdesc->memid == 0x6001000c, "got %lx\n", pfuncdesc->memid);
    ok(pfuncdesc->lprgscode == NULL, "got %p\n", pfuncdesc->lprgscode);
    ok(pfuncdesc->lprgelemdescParam != NULL, "got %p\n", pfuncdesc->lprgelemdescParam);
    ok(pfuncdesc->funckind == FUNC_PUREVIRTUAL, "got 0x%x\n", pfuncdesc->funckind);
    ok(pfuncdesc->invkind == INVOKE_FUNC, "got 0x%x\n", pfuncdesc->invkind);
    ok(pfuncdesc->callconv == CC_STDCALL, "got 0x%x\n", pfuncdesc->callconv);
    ok(pfuncdesc->cParams == 2, "got %d\n", pfuncdesc->cParams);
    ok(pfuncdesc->cParamsOpt == 0, "got %d\n", pfuncdesc->cParamsOpt);
    todo_wine_if(sys != native_sys)
    ok(pfuncdesc->oVft == 7 * sizeof(void*), "got %d\n", pfuncdesc->oVft);
    ok(pfuncdesc->cScodes == 0, "got %d\n", pfuncdesc->cScodes);
    ok(pfuncdesc->elemdescFunc.tdesc.vt == VT_VOID, "got %d\n", pfuncdesc->elemdescFunc.tdesc.vt);
    ok(pfuncdesc->wFuncFlags == 0, "got 0x%x\n", pfuncdesc->wFuncFlags);

    edesc = pfuncdesc->lprgelemdescParam;
    ok(edesc->tdesc.vt == VT_INT, "got: %d\n", edesc->tdesc.vt);
    ok(edesc->paramdesc.wParamFlags == PARAMFLAG_FHASDEFAULT, "got: 0x%x\n", edesc->paramdesc.wParamFlags);
    ok(edesc->paramdesc.pparamdescex != NULL, "got: %p\n", edesc->paramdesc.pparamdescex);
    ok(edesc->paramdesc.pparamdescex->cBytes == sizeof(PARAMDESCEX), "got: %ld\n",
            edesc->paramdesc.pparamdescex->cBytes);
    ok(V_VT(&edesc->paramdesc.pparamdescex->varDefaultValue) == VT_I4, "got: %d\n",
            V_VT(&edesc->paramdesc.pparamdescex->varDefaultValue));
    ok(V_I4(&edesc->paramdesc.pparamdescex->varDefaultValue) == 0xFFFFFFFF,
            "got: 0x%lx\n", V_I4(&edesc->paramdesc.pparamdescex->varDefaultValue));

    edesc = pfuncdesc->lprgelemdescParam + 1;
    ok(edesc->tdesc.vt == VT_INT, "got: %d\n", edesc->tdesc.vt);
    ok(edesc->paramdesc.wParamFlags == PARAMFLAG_FHASDEFAULT, "got: 0x%x\n", edesc->paramdesc.wParamFlags);
    ok(edesc->paramdesc.pparamdescex != NULL, "got: %p\n", edesc->paramdesc.pparamdescex);
    ok(edesc->paramdesc.pparamdescex->cBytes == sizeof(PARAMDESCEX), "got: %ld\n",
            edesc->paramdesc.pparamdescex->cBytes);
    ok(V_VT(&edesc->paramdesc.pparamdescex->varDefaultValue) == VT_I4, "got: %d\n",
            V_VT(&edesc->paramdesc.pparamdescex->varDefaultValue));
    ok(V_I4(&edesc->paramdesc.pparamdescex->varDefaultValue) == 0xFFFFFFFF,
            "got: 0x%lx\n", V_I4(&edesc->paramdesc.pparamdescex->varDefaultValue));

    hres = ITypeInfo_GetDocumentation(ti, pfuncdesc->memid, &name, &docstring, &helpcontext, &helpfile);
    ok(hres == S_OK, "got: %08lx\n", hres);
    ok(name == NULL, "got name: %s\n", wine_dbgstr_w(name));
    ok(docstring == NULL, "got docstring: %s\n", wine_dbgstr_w(docstring));
    ok(helpcontext == 0, "got helpcontext: 0x%lx\n", helpcontext);
    ok(!memcmp(helpfile, helpfileW, sizeof(helpfileW)), "got helpfile: %s\n", wine_dbgstr_w(helpfile));
    SysFreeString(helpfile);
    ITypeInfo_ReleaseFuncDesc(ti, pfuncdesc);

    hres = ITypeInfo_GetFuncDesc(ti, 5, &pfuncdesc);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(pfuncdesc->memid == 0x60010005, "got %lx\n", pfuncdesc->memid);
    ok(pfuncdesc->lprgscode == NULL, "got %p\n", pfuncdesc->lprgscode);
    ok(pfuncdesc->lprgelemdescParam != NULL, "got %p\n", pfuncdesc->lprgelemdescParam);
    ok(pfuncdesc->funckind == FUNC_PUREVIRTUAL, "got 0x%x\n", pfuncdesc->funckind);
    ok(pfuncdesc->invkind == INVOKE_FUNC, "got 0x%x\n", pfuncdesc->invkind);
    ok(pfuncdesc->callconv == CC_STDCALL, "got 0x%x\n", pfuncdesc->callconv);
    ok(pfuncdesc->cParams == 1, "got %d\n", pfuncdesc->cParams);
    ok(pfuncdesc->cParamsOpt == 0, "got %d\n", pfuncdesc->cParamsOpt);
    todo_wine_if(sys != native_sys)
    ok(pfuncdesc->oVft == 8 * sizeof(void*), "got %d\n", pfuncdesc->oVft);
    ok(pfuncdesc->cScodes == 0, "got %d\n", pfuncdesc->cScodes);
    ok(pfuncdesc->elemdescFunc.tdesc.vt == VT_VOID, "got %d\n", pfuncdesc->elemdescFunc.tdesc.vt);
    ok(pfuncdesc->wFuncFlags == 0, "got 0x%x\n", pfuncdesc->wFuncFlags);

    edesc = pfuncdesc->lprgelemdescParam;
    ok(edesc->paramdesc.pparamdescex != NULL, "got: %p\n", edesc->paramdesc.pparamdescex);
    ok(edesc->paramdesc.pparamdescex->cBytes == sizeof(PARAMDESCEX), "got: %ld\n",
            edesc->paramdesc.pparamdescex->cBytes);
    ok(V_VT(&edesc->paramdesc.pparamdescex->varDefaultValue) == VT_INT, "got: %d\n",
            V_VT(&edesc->paramdesc.pparamdescex->varDefaultValue));
    ok(V_UI2(&edesc->paramdesc.pparamdescex->varDefaultValue) == 0x789, "got: 0x%x\n",
            V_UI2(&edesc->paramdesc.pparamdescex->varDefaultValue));
    ok(edesc->paramdesc.wParamFlags == PARAMFLAG_FHASDEFAULT,
            "got: 0x%x\n", edesc->paramdesc.wParamFlags);
    ok(edesc->tdesc.vt == VT_USERDEFINED, "got: %d\n", edesc->tdesc.vt);
    ok(edesc->tdesc.hreftype == hreftype, "got: 0x%lx\n", edesc->tdesc.hreftype);

    hres = ITypeInfo_GetDocumentation(ti, pfuncdesc->memid, &name, &docstring, &helpcontext, &helpfile);
    ok(hres == S_OK, "got: %08lx\n", hres);
    ok(name == NULL, "got name: %s\n", wine_dbgstr_w(name));
    ok(docstring == NULL, "got docstring: %s\n", wine_dbgstr_w(docstring));
    ok(helpcontext == 0, "got helpcontext: 0x%lx\n", helpcontext);
    ok(!memcmp(helpfile, helpfileW, sizeof(helpfileW)), "got helpfile: %s\n", wine_dbgstr_w(helpfile));
    SysFreeString(helpfile);
    ITypeInfo_ReleaseFuncDesc(ti, pfuncdesc);

    hres = ITypeInfo_GetFuncDesc(ti, 6, &pfuncdesc);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(pfuncdesc->memid == 0x60010006, "got %lx\n", pfuncdesc->memid);
    ok(pfuncdesc->lprgscode == NULL, "got %p\n", pfuncdesc->lprgscode);
    ok(pfuncdesc->lprgelemdescParam != NULL, "got %p\n", pfuncdesc->lprgelemdescParam);
    ok(pfuncdesc->funckind == FUNC_PUREVIRTUAL, "got 0x%x\n", pfuncdesc->funckind);
    ok(pfuncdesc->invkind == INVOKE_FUNC, "got 0x%x\n", pfuncdesc->invkind);
    ok(pfuncdesc->callconv == CC_STDCALL, "got 0x%x\n", pfuncdesc->callconv);
    ok(pfuncdesc->cParams == 1, "got %d\n", pfuncdesc->cParams);
    ok(pfuncdesc->cParamsOpt == 0, "got %d\n", pfuncdesc->cParamsOpt);
    todo_wine_if(sys != native_sys)
    ok(pfuncdesc->oVft == 9 * sizeof(void*), "got %d\n", pfuncdesc->oVft);
    ok(pfuncdesc->cScodes == 0, "got %d\n", pfuncdesc->cScodes);
    ok(pfuncdesc->elemdescFunc.tdesc.vt == VT_VARIANT, "got %d\n", pfuncdesc->elemdescFunc.tdesc.vt);
    ok(pfuncdesc->wFuncFlags == 0, "got 0x%x\n", pfuncdesc->wFuncFlags);

    edesc = pfuncdesc->lprgelemdescParam;
    ok(edesc->paramdesc.pparamdescex != NULL, "got: %p\n", edesc->paramdesc.pparamdescex);
    ok(edesc->paramdesc.pparamdescex->cBytes == sizeof(PARAMDESCEX), "got: %ld\n",
            edesc->paramdesc.pparamdescex->cBytes);
    ok(V_VT(&edesc->paramdesc.pparamdescex->varDefaultValue) == VT_INT, "got: %d\n",
            V_VT(&edesc->paramdesc.pparamdescex->varDefaultValue));
    ok(V_UI2(&edesc->paramdesc.pparamdescex->varDefaultValue) == 0x3, "got: 0x%x\n",
            V_UI2(&edesc->paramdesc.pparamdescex->varDefaultValue));
    ok(edesc->paramdesc.wParamFlags == PARAMFLAG_FHASDEFAULT,
            "got: 0x%x\n", edesc->paramdesc.wParamFlags);
    ok(edesc->tdesc.vt == VT_VARIANT, "got: %d\n", edesc->tdesc.vt);
    ok(edesc->tdesc.hreftype == 0, "got: 0x%lx\n", edesc->tdesc.hreftype);

    hres = ITypeInfo_GetDocumentation(ti, pfuncdesc->memid, &name, &docstring, &helpcontext, &helpfile);
    ok(hres == S_OK, "got: %08lx\n", hres);
    ok(name == NULL, "got name: %s\n", wine_dbgstr_w(name));
    ok(docstring == NULL, "got docstring: %s\n", wine_dbgstr_w(docstring));
    ok(helpcontext == 0, "got helpcontext: 0x%lx\n", helpcontext);
    ok(!memcmp(helpfile, helpfileW, sizeof(helpfileW)), "got helpfile: %s\n", wine_dbgstr_w(helpfile));
    SysFreeString(helpfile);
    ITypeInfo_ReleaseFuncDesc(ti, pfuncdesc);

    hres = ITypeInfo_GetFuncDesc(ti, 7, &pfuncdesc);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(pfuncdesc->memid == 0x60010009, "got %lx\n", pfuncdesc->memid);
    ok(pfuncdesc->lprgscode == NULL, "got %p\n", pfuncdesc->lprgscode);
    ok(pfuncdesc->lprgelemdescParam != NULL, "got %p\n", pfuncdesc->lprgelemdescParam);
    ok(pfuncdesc->funckind == FUNC_PUREVIRTUAL, "got 0x%x\n", pfuncdesc->funckind);
    ok(pfuncdesc->invkind == INVOKE_FUNC, "got 0x%x\n", pfuncdesc->invkind);
    ok(pfuncdesc->callconv == CC_STDCALL, "got 0x%x\n", pfuncdesc->callconv);
    ok(pfuncdesc->cParams == 2, "got %d\n", pfuncdesc->cParams);
    ok(pfuncdesc->cParamsOpt == 0, "got %d\n", pfuncdesc->cParamsOpt);
    todo_wine_if(sys != native_sys)
    ok(pfuncdesc->oVft == 10 * sizeof(void*), "got %d\n", pfuncdesc->oVft);
    ok(pfuncdesc->cScodes == 0, "got %d\n", pfuncdesc->cScodes);
    ok(pfuncdesc->elemdescFunc.tdesc.vt == VT_VOID, "got %d\n", pfuncdesc->elemdescFunc.tdesc.vt);
    ok(pfuncdesc->wFuncFlags == 0, "got 0x%x\n", pfuncdesc->wFuncFlags);

    edesc = pfuncdesc->lprgelemdescParam;
    ok(edesc->tdesc.vt == VT_INT, "got: %d\n", edesc->tdesc.vt);
    ok(edesc->paramdesc.wParamFlags == PARAMFLAG_FIN, "got: 0x%x\n", edesc->paramdesc.wParamFlags);
    ok(edesc->paramdesc.pparamdescex == NULL, "got: %p\n", edesc->paramdesc.pparamdescex);

    edesc = pfuncdesc->lprgelemdescParam + 1;
    ok(edesc->tdesc.vt == VT_UI2, "got: %d\n", edesc->tdesc.vt);
    ok(edesc->paramdesc.wParamFlags == PARAMFLAG_FHASDEFAULT, "got: 0x%x\n", edesc->paramdesc.wParamFlags);
    ok(edesc->paramdesc.pparamdescex != NULL, "got: %p\n", edesc->paramdesc.pparamdescex);
    ok(edesc->paramdesc.pparamdescex->cBytes == sizeof(PARAMDESCEX), "got: %ld\n",
            edesc->paramdesc.pparamdescex->cBytes);
    ok(V_VT(&edesc->paramdesc.pparamdescex->varDefaultValue) == VT_UI2, "got: %d\n",
            V_VT(&edesc->paramdesc.pparamdescex->varDefaultValue));
    ok(V_UI2(&edesc->paramdesc.pparamdescex->varDefaultValue) == 0xFFFF, "got: 0x%x\n",
            V_UI2(&edesc->paramdesc.pparamdescex->varDefaultValue));

    hres = ITypeInfo_GetDocumentation(ti, pfuncdesc->memid, &name, &docstring, &helpcontext, &helpfile);
    ok(hres == S_OK, "got: %08lx\n", hres);
    ok(name == NULL, "got name: %s\n", wine_dbgstr_w(name));
    ok(docstring == NULL, "got docstring: %s\n", wine_dbgstr_w(docstring));
    ok(helpcontext == 0, "got helpcontext: 0x%lx\n", helpcontext);
    ok(!memcmp(helpfile, helpfileW, sizeof(helpfileW)), "got helpfile: %s\n", wine_dbgstr_w(helpfile));
    SysFreeString(helpfile);
    ITypeInfo_ReleaseFuncDesc(ti, pfuncdesc);

    hres = ITypeInfo_GetFuncDesc(ti, 8, &pfuncdesc);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(pfuncdesc->memid == 0x60010003, "got %lx\n", pfuncdesc->memid);
    ok(pfuncdesc->lprgscode == NULL, "got %p\n", pfuncdesc->lprgscode);
    ok(pfuncdesc->lprgelemdescParam != NULL, "got %p\n", pfuncdesc->lprgelemdescParam);
    ok(pfuncdesc->funckind == FUNC_PUREVIRTUAL, "got 0x%x\n", pfuncdesc->funckind);
    ok(pfuncdesc->invkind == INVOKE_FUNC, "got 0x%x\n", pfuncdesc->invkind);
    ok(pfuncdesc->callconv == CC_STDCALL, "got 0x%x\n", pfuncdesc->callconv);
    ok(pfuncdesc->cParams == 1, "got %d\n", pfuncdesc->cParams);
    ok(pfuncdesc->cParamsOpt == 0, "got %d\n", pfuncdesc->cParamsOpt);
    todo_wine_if(sys != native_sys)
    ok(pfuncdesc->oVft == 11 * sizeof(void*), "got %d\n", pfuncdesc->oVft);
    ok(pfuncdesc->cScodes == 0, "got %d\n", pfuncdesc->cScodes);
    ok(pfuncdesc->elemdescFunc.tdesc.vt == VT_VOID, "got %d\n", pfuncdesc->elemdescFunc.tdesc.vt);
    ok(pfuncdesc->wFuncFlags == 0, "got 0x%x\n", pfuncdesc->wFuncFlags);

    edesc = pfuncdesc->lprgelemdescParam;
    ok(edesc->tdesc.vt == VT_INT, "got: %d\n", edesc->tdesc.vt);
    ok(edesc->paramdesc.wParamFlags == PARAMFLAG_FHASDEFAULT, "got: 0x%x\n", edesc->paramdesc.wParamFlags);
    ok(edesc->paramdesc.pparamdescex != NULL, "got: %p\n", edesc->paramdesc.pparamdescex);
    ok(edesc->paramdesc.pparamdescex->cBytes == sizeof(PARAMDESCEX), "got: %ld\n",
            edesc->paramdesc.pparamdescex->cBytes);
    ok(V_VT(&edesc->paramdesc.pparamdescex->varDefaultValue) == VT_I4, "got: %d\n",
            V_VT(&edesc->paramdesc.pparamdescex->varDefaultValue));
    ok(V_I4(&edesc->paramdesc.pparamdescex->varDefaultValue) == 0x123, "got: 0x%lx\n",
            V_I4(&edesc->paramdesc.pparamdescex->varDefaultValue));

    hres = ITypeInfo_GetDocumentation(ti, pfuncdesc->memid, &name, &docstring, &helpcontext, &helpfile);
    ok(hres == S_OK, "got: %08lx\n", hres);
    ok(name == NULL, "got name: %s\n", wine_dbgstr_w(name));
    ok(docstring == NULL, "got docstring: %s\n", wine_dbgstr_w(docstring));
    ok(helpcontext == 0, "got helpcontext: 0x%lx\n", helpcontext);
    ok(!memcmp(helpfile, helpfileW, sizeof(helpfileW)), "got helpfile: %s\n", wine_dbgstr_w(helpfile));
    SysFreeString(helpfile);
    ITypeInfo_ReleaseFuncDesc(ti, pfuncdesc);

    hres = ITypeInfo_GetFuncDesc(ti, 9, &pfuncdesc);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(pfuncdesc->memid == 0, "got %lx\n", pfuncdesc->memid);
    ok(pfuncdesc->lprgscode == NULL, "got %p\n", pfuncdesc->lprgscode);
    ok(pfuncdesc->lprgelemdescParam == NULL, "got %p\n", pfuncdesc->lprgelemdescParam);
    ok(pfuncdesc->funckind == FUNC_PUREVIRTUAL, "got 0x%x\n", pfuncdesc->funckind);
    ok(pfuncdesc->invkind == INVOKE_PROPERTYGET, "got 0x%x\n", pfuncdesc->invkind);
    ok(pfuncdesc->callconv == CC_STDCALL, "got 0x%x\n", pfuncdesc->callconv);
    ok(pfuncdesc->cParams == 0, "got %d\n", pfuncdesc->cParams);
    ok(pfuncdesc->cParamsOpt == 0, "got %d\n", pfuncdesc->cParamsOpt);
    todo_wine_if(sys != native_sys)
    ok(pfuncdesc->oVft == 12 * sizeof(void*), "got %d\n", pfuncdesc->oVft);
    ok(pfuncdesc->cScodes == 0, "got %d\n", pfuncdesc->cScodes);
    ok(pfuncdesc->elemdescFunc.tdesc.vt == VT_BSTR, "got %d\n", pfuncdesc->elemdescFunc.tdesc.vt);
    ok(pfuncdesc->wFuncFlags == 0, "got 0x%x\n", pfuncdesc->wFuncFlags);

    hres = ITypeInfo_GetDocumentation(ti, pfuncdesc->memid, &name, &docstring, &helpcontext, &helpfile);
    ok(hres == S_OK, "got: %08lx\n", hres);
    ok(!memcmp(name, func1W, sizeof(func1W)), "got name: %s\n", wine_dbgstr_w(name));
    ok(docstring == NULL, "got docstring: %s\n", wine_dbgstr_w(docstring));
    ok(helpcontext == 0x201, "got helpcontext: 0x%lx\n", helpcontext);
    ok(!memcmp(helpfile, helpfileW, sizeof(helpfileW)), "got helpfile: %s\n", wine_dbgstr_w(helpfile));
    SysFreeString(name);
    SysFreeString(helpfile);

    hres = ITypeInfo_GetNames(ti, pfuncdesc->memid, names, ARRAY_SIZE(names), &cnames);
    ok(hres == S_OK, "got: %08lx\n", hres);
    ok(cnames == 1, "got: %u\n", cnames);
    ok(!memcmp(names[0], func1W, sizeof(func1W)), "got names[0]: %s\n", wine_dbgstr_w(names[0]));
    SysFreeString(names[0]);
    ITypeInfo_ReleaseFuncDesc(ti, pfuncdesc);

    hres = ITypeInfo_GetFuncDesc(ti, 10, &pfuncdesc);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(pfuncdesc->memid == 0x60010007, "got %lx\n", pfuncdesc->memid);
    ok(pfuncdesc->lprgscode == NULL, "got %p\n", pfuncdesc->lprgscode);
    ok(pfuncdesc->lprgelemdescParam != NULL, "got %p\n", pfuncdesc->lprgelemdescParam);
    ok(pfuncdesc->funckind == FUNC_PUREVIRTUAL, "got 0x%x\n", pfuncdesc->funckind);
    ok(pfuncdesc->invkind == INVOKE_FUNC, "got 0x%x\n", pfuncdesc->invkind);
    ok(pfuncdesc->callconv == CC_STDCALL, "got 0x%x\n", pfuncdesc->callconv);
    ok(pfuncdesc->cParams == 1, "got %d\n", pfuncdesc->cParams);
    ok(pfuncdesc->cParamsOpt == 0, "got %d\n", pfuncdesc->cParamsOpt);
    todo_wine_if(sys != native_sys)
    ok(pfuncdesc->oVft == 13 * sizeof(void*), "got %d\n", pfuncdesc->oVft);
    ok(pfuncdesc->cScodes == 0, "got %d\n", pfuncdesc->cScodes);
    ok(pfuncdesc->elemdescFunc.tdesc.vt == VT_VOID, "got %d\n", pfuncdesc->elemdescFunc.tdesc.vt);
    ok(pfuncdesc->wFuncFlags == 0, "got 0x%x\n", pfuncdesc->wFuncFlags);

    edesc = pfuncdesc->lprgelemdescParam;
    ok(edesc->tdesc.vt == VT_PTR, "got: %d\n", edesc->tdesc.vt);
    ok(edesc->paramdesc.wParamFlags == PARAMFLAG_FIN, "got: 0x%x\n", edesc->paramdesc.wParamFlags);
    ok(edesc->paramdesc.pparamdescex == NULL, "got: %p\n", edesc->paramdesc.pparamdescex);
    ok(edesc->tdesc.lptdesc != NULL, "got: %p\n", edesc->tdesc.lptdesc);
    ok(edesc->tdesc.lptdesc->vt == VT_PTR, "got: %d\n", edesc->tdesc.lptdesc->vt);
    ok(edesc->tdesc.lptdesc->lptdesc != NULL, "got: %p\n", edesc->tdesc.lptdesc->lptdesc);
    ok(edesc->tdesc.lptdesc->lptdesc->vt == VT_BSTR, "got: %d\n", edesc->tdesc.lptdesc->lptdesc->vt);

    hres = ITypeInfo_GetDocumentation(ti, pfuncdesc->memid, &name, &docstring, &helpcontext, &helpfile);
    ok(hres == S_OK, "got: %08lx\n", hres);
    ok(name == NULL, "got name: %s\n", wine_dbgstr_w(name));
    ok(docstring == NULL, "got docstring: %s\n", wine_dbgstr_w(docstring));
    ok(helpcontext == 0, "got helpcontext: 0x%lx\n", helpcontext);
    ok(!memcmp(helpfile, helpfileW, sizeof(helpfileW)), "got helpfile: %s\n", wine_dbgstr_w(helpfile));
    SysFreeString(helpfile);
    ITypeInfo_ReleaseFuncDesc(ti, pfuncdesc);

    hres = ITypeInfo_GetFuncDesc(ti, 11, &pfuncdesc);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(pfuncdesc->memid == 0x60010004, "got %lx\n", pfuncdesc->memid);
    ok(pfuncdesc->lprgscode == NULL, "got %p\n", pfuncdesc->lprgscode);
    ok(pfuncdesc->lprgelemdescParam != NULL, "got %p\n", pfuncdesc->lprgelemdescParam);
    ok(pfuncdesc->funckind == FUNC_PUREVIRTUAL, "got 0x%x\n", pfuncdesc->funckind);
    ok(pfuncdesc->invkind == INVOKE_FUNC, "got 0x%x\n", pfuncdesc->invkind);
    ok(pfuncdesc->callconv == CC_STDCALL, "got 0x%x\n", pfuncdesc->callconv);
    ok(pfuncdesc->cParams == 1, "got %d\n", pfuncdesc->cParams);
    ok(pfuncdesc->cParamsOpt == 0, "got %d\n", pfuncdesc->cParamsOpt);
    todo_wine_if(sys != native_sys)
    ok(pfuncdesc->oVft == 14 * sizeof(void*), "got %d\n", pfuncdesc->oVft);
    ok(pfuncdesc->cScodes == 0, "got %d\n", pfuncdesc->cScodes);
    ok(pfuncdesc->elemdescFunc.tdesc.vt == VT_VOID, "got %d\n", pfuncdesc->elemdescFunc.tdesc.vt);
    ok(pfuncdesc->wFuncFlags == 0, "got 0x%x\n", pfuncdesc->wFuncFlags);

    edesc = pfuncdesc->lprgelemdescParam;
    ok(edesc->tdesc.vt == VT_PTR, "got: %d\n", edesc->tdesc.vt);
    ok(edesc->paramdesc.wParamFlags == PARAMFLAG_FIN, "got: 0x%x\n", edesc->paramdesc.wParamFlags);
    ok(edesc->paramdesc.pparamdescex == NULL, "got: %p\n", edesc->paramdesc.pparamdescex);
    ok(edesc->tdesc.lptdesc != NULL, "got: %p\n", edesc->tdesc.lptdesc);
    ok(edesc->tdesc.lptdesc->vt == VT_BSTR, "got: %d\n", edesc->tdesc.lptdesc->vt);

    hres = ITypeInfo_GetDocumentation(ti, pfuncdesc->memid, &name, &docstring, &helpcontext, &helpfile);
    ok(hres == S_OK, "got: %08lx\n", hres);
    ok(name == NULL, "got name: %s\n", wine_dbgstr_w(name));
    ok(docstring == NULL, "got docstring: %s\n", wine_dbgstr_w(docstring));
    ok(helpcontext == 0, "got helpcontext: 0x%lx\n", helpcontext);
    ok(!memcmp(helpfile, helpfileW, sizeof(helpfileW)), "got helpfile: %s\n", wine_dbgstr_w(helpfile));
    SysFreeString(helpfile);
    ITypeInfo_ReleaseFuncDesc(ti, pfuncdesc);

    hres = ITypeInfo_GetFuncDesc(ti, 12, &pfuncdesc);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(pfuncdesc->memid == 0, "got %lx\n", pfuncdesc->memid);
    ok(pfuncdesc->lprgscode == NULL, "got %p\n", pfuncdesc->lprgscode);
    ok(pfuncdesc->lprgelemdescParam != NULL, "got %p\n", pfuncdesc->lprgelemdescParam);
    ok(pfuncdesc->funckind == FUNC_PUREVIRTUAL, "got 0x%x\n", pfuncdesc->funckind);
    ok(pfuncdesc->invkind == INVOKE_PROPERTYPUT, "got 0x%x\n", pfuncdesc->invkind);
    ok(pfuncdesc->callconv == CC_STDCALL, "got 0x%x\n", pfuncdesc->callconv);
    ok(pfuncdesc->cParams == 1, "got %d\n", pfuncdesc->cParams);
    ok(pfuncdesc->cParamsOpt == 0, "got %d\n", pfuncdesc->cParamsOpt);
    todo_wine_if(sys != native_sys)
    ok(pfuncdesc->oVft == 15 * sizeof(void*), "got %d\n", pfuncdesc->oVft);
    ok(pfuncdesc->cScodes == 0, "got %d\n", pfuncdesc->cScodes);
    ok(pfuncdesc->elemdescFunc.tdesc.vt == VT_VOID, "got %d\n", pfuncdesc->elemdescFunc.tdesc.vt);
    ok(pfuncdesc->wFuncFlags == 0, "got 0x%x\n", pfuncdesc->wFuncFlags);

    edesc = pfuncdesc->lprgelemdescParam;
    ok(edesc->tdesc.vt == VT_BSTR, "got: %d\n", edesc->tdesc.vt);
    ok(edesc->idldesc.wIDLFlags == IDLFLAG_FIN, "got: %x\n", edesc->idldesc.wIDLFlags);

    hres = ITypeInfo_GetDocumentation(ti, pfuncdesc->memid, &name, &docstring, &helpcontext, &helpfile);
    ok(hres == S_OK, "got: %08lx\n", hres);
    ok(!memcmp(name, func1W, sizeof(func1W)), "got name: %s\n", wine_dbgstr_w(name));
    ok(docstring == NULL, "got docstring: %s\n", wine_dbgstr_w(docstring));
    ok(helpcontext == 0x201, "got helpcontext: 0x%lx\n", helpcontext);
    ok(!memcmp(helpfile, helpfileW, sizeof(helpfileW)), "got helpfile: %s\n", wine_dbgstr_w(helpfile));
    SysFreeString(name);
    SysFreeString(helpfile);

    hres = ITypeInfo_GetNames(ti, pfuncdesc->memid, names, ARRAY_SIZE(names), &cnames);
    ok(hres == S_OK, "got: %08lx\n", hres);
    ok(cnames == 1, "got: %u\n", cnames);
    ok(!memcmp(names[0], func1W, sizeof(func1W)), "got names[0]: %s\n", wine_dbgstr_w(names[0]));
    SysFreeString(names[0]);
    ITypeInfo_ReleaseFuncDesc(ti, pfuncdesc);

    hres = ITypeInfo_GetFuncDesc(ti, 13, &pfuncdesc);
    ok(hres == TYPE_E_ELEMENTNOTFOUND, "got %08lx\n", hres);

    ok(ITypeInfo_Release(ti) == 0, "Object should be freed\n");

    hres = ITypeLib_GetTypeInfo(tl, 1, &ti);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ITypeInfo_GetTypeAttr(ti, &typeattr);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(typeattr->cbSizeInstance == sizeof(void*), "cbSizeInstance = %ld\n", typeattr->cbSizeInstance);
    ok(typeattr->typekind == TKIND_INTERFACE, "typekind = %d\n", typeattr->typekind);
    ok(typeattr->cFuncs == 2, "cFuncs = %d\n", typeattr->cFuncs);
    ok(typeattr->cVars == 0, "cVars = %d\n", typeattr->cVars);
    ok(typeattr->cImplTypes == 1, "cImplTypes = %d\n", typeattr->cImplTypes);
    ok(typeattr->cbSizeVft == 0xaab8 || typeattr->cbSizeVft == 0xaab0 ||
       typeattr->cbSizeVft == 0x555c || typeattr->cbSizeVft == 0x5560,
       "cbSizeVft = 0x%x\n", typeattr->cbSizeVft);
    ok(typeattr->cbAlignment == alignment, "cbAlignment = %d\n", typeattr->cbAlignment);
    ok(typeattr->wTypeFlags == 0, "wTypeFlags = %d\n", typeattr->wTypeFlags);
    ok(typeattr->wMajorVerNum == 0, "wMajorVerNum = %d\n", typeattr->wMajorVerNum);
    ok(typeattr->wMinorVerNum == 0, "wMinorVerNum = %d\n", typeattr->wMinorVerNum);
    ITypeInfo_ReleaseTypeAttr(ti, typeattr);

    hres = ITypeInfo_GetRefTypeOfImplType(ti, 0, &hreftype);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ITypeInfo_GetFuncDesc(ti, 0, &pfuncdesc);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(pfuncdesc->memid == 0x60020000, "got %lx\n", pfuncdesc->memid);
    ok(pfuncdesc->lprgscode == NULL, "got %p\n", pfuncdesc->lprgscode);
    ok(pfuncdesc->lprgelemdescParam != NULL, "got %p\n", pfuncdesc->lprgelemdescParam);
    ok(pfuncdesc->funckind == FUNC_PUREVIRTUAL, "got 0x%x\n", pfuncdesc->funckind);
    ok(pfuncdesc->invkind == INVOKE_FUNC, "got 0x%x\n", pfuncdesc->invkind);
    ok(pfuncdesc->callconv == CC_STDCALL, "got 0x%x\n", pfuncdesc->callconv);
    ok(pfuncdesc->cParams == 1, "got %d\n", pfuncdesc->cParams);
    ok(pfuncdesc->cParamsOpt == 0, "got %d\n", pfuncdesc->cParamsOpt);
    ok(pfuncdesc->oVft == (short)(0xaaa8 * sizeof(void *) / ptr_size), "got %x\n", pfuncdesc->oVft);
    ok(pfuncdesc->cScodes == 0, "got %d\n", pfuncdesc->cScodes);
    ok(pfuncdesc->elemdescFunc.tdesc.vt == VT_VOID, "got %d\n", pfuncdesc->elemdescFunc.tdesc.vt);
    ok(pfuncdesc->wFuncFlags == 0, "got 0x%x\n", pfuncdesc->wFuncFlags);

    edesc = pfuncdesc->lprgelemdescParam;
    ok(edesc->tdesc.vt == VT_VARIANT, "got: %d\n", edesc->tdesc.vt);
    ok(edesc->paramdesc.wParamFlags == PARAMFLAG_FHASDEFAULT,
            "got: 0x%x\n", edesc->paramdesc.wParamFlags);
    ok(edesc->paramdesc.pparamdescex != NULL, "got: %p\n", edesc->paramdesc.pparamdescex);
    ok(edesc->paramdesc.pparamdescex->cBytes == sizeof(PARAMDESCEX), "got: %ld\n",
            edesc->paramdesc.pparamdescex->cBytes);
    ok(V_VT(&edesc->paramdesc.pparamdescex->varDefaultValue) == VT_INT, "got: %d\n",
            V_VT(&edesc->paramdesc.pparamdescex->varDefaultValue));
    ok(V_UI2(&edesc->paramdesc.pparamdescex->varDefaultValue) == 0x3, "got: 0x%x\n",
            V_UI2(&edesc->paramdesc.pparamdescex->varDefaultValue));
    ok(edesc->paramdesc.wParamFlags == PARAMFLAG_FHASDEFAULT,
            "got: 0x%x\n", edesc->paramdesc.wParamFlags);
    ok(edesc->tdesc.lptdesc == NULL, "got: %p\n", edesc->tdesc.lptdesc);
    ok(edesc->tdesc.hreftype == 0, "got: %ld\n", edesc->tdesc.hreftype);
    ITypeInfo_ReleaseFuncDesc(ti, pfuncdesc);

    hres = ITypeInfo_GetFuncDesc(ti, 1, &pfuncdesc);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(pfuncdesc->memid == 0x60020001, "got %lx\n", pfuncdesc->memid);
    ok(pfuncdesc->lprgscode == NULL, "got %p\n", pfuncdesc->lprgscode);
    ok(pfuncdesc->lprgelemdescParam != NULL, "got %p\n", pfuncdesc->lprgelemdescParam);
    ok(pfuncdesc->funckind == FUNC_PUREVIRTUAL, "got 0x%x\n", pfuncdesc->funckind);
    ok(pfuncdesc->invkind == INVOKE_FUNC, "got 0x%x\n", pfuncdesc->invkind);
    ok(pfuncdesc->callconv == CC_STDCALL, "got 0x%x\n", pfuncdesc->callconv);
    ok(pfuncdesc->cParams == 1, "got %d\n", pfuncdesc->cParams);
    ok(pfuncdesc->cParamsOpt == 0, "got %d\n", pfuncdesc->cParamsOpt);
    ok(pfuncdesc->oVft == (short)((sys == SYS_WIN64 ? 0xaab0 : 0xaaac) * sizeof(void *) / ptr_size), "got %x\n", pfuncdesc->oVft);
    ok(pfuncdesc->cScodes == 0, "got %d\n", pfuncdesc->cScodes);
    ok(pfuncdesc->elemdescFunc.tdesc.vt == VT_VOID, "got %d\n", pfuncdesc->elemdescFunc.tdesc.vt);
    ok(pfuncdesc->wFuncFlags == 0, "got 0x%x\n", pfuncdesc->wFuncFlags);

    edesc = pfuncdesc->lprgelemdescParam;
    ok(edesc->tdesc.vt == VT_VARIANT, "got: %d\n", edesc->tdesc.vt);
    ok(edesc->paramdesc.wParamFlags == PARAMFLAG_FHASDEFAULT,
            "got: 0x%x\n", edesc->paramdesc.wParamFlags);
    ok(edesc->paramdesc.pparamdescex != NULL, "got: %p\n", edesc->paramdesc.pparamdescex);
    ok(edesc->paramdesc.pparamdescex->cBytes == sizeof(PARAMDESCEX), "got: %ld\n",
            edesc->paramdesc.pparamdescex->cBytes);
    ok(V_VT(&edesc->paramdesc.pparamdescex->varDefaultValue) == VT_INT, "got: %d\n",
            V_VT(&edesc->paramdesc.pparamdescex->varDefaultValue));
    ok(V_UI2(&edesc->paramdesc.pparamdescex->varDefaultValue) == 0x3, "got: 0x%x\n",
            V_UI2(&edesc->paramdesc.pparamdescex->varDefaultValue));
    ok(edesc->paramdesc.wParamFlags == PARAMFLAG_FHASDEFAULT,
            "got: 0x%x\n", edesc->paramdesc.wParamFlags);
    ok(edesc->tdesc.lptdesc == NULL, "got: %p\n", edesc->tdesc.lptdesc);
    ok(edesc->tdesc.hreftype == 0, "got: %ld\n", edesc->tdesc.hreftype);
    ITypeInfo_ReleaseFuncDesc(ti, pfuncdesc);

    ok(ITypeInfo_Release(ti) == 0, "Object should be freed\n");

    hres = ITypeLib_GetTypeInfo(tl, 2, &ti);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ITypeInfo_QueryInterface(ti, &IID_ITypeInfo2, (void**)&ti2);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ITypeInfo_GetTypeAttr(ti, &typeattr);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(typeattr->cbSizeInstance == sizeof(void*), "cbSizeInstance = %ld\n", typeattr->cbSizeInstance);
    ok(typeattr->typekind == TKIND_INTERFACE, "typekind = %d\n", typeattr->typekind);
    ok(typeattr->cFuncs == 0, "cFuncs = %d\n", typeattr->cFuncs);
    ok(typeattr->cVars == 0, "cVars = %d\n", typeattr->cVars);
    ok(typeattr->cImplTypes == 0, "cImplTypes = %d\n", typeattr->cImplTypes);
    ok(typeattr->cbSizeVft == 0, "cbSizeVft = %d\n", typeattr->cbSizeVft);
    ok(typeattr->cbAlignment == alignment, "cbAlignment = %d\n", typeattr->cbAlignment);
    ok(typeattr->wTypeFlags == 0, "wTypeFlags = %d\n", typeattr->wTypeFlags);
    ok(typeattr->wMajorVerNum == 0, "wMajorVerNum = %d\n", typeattr->wMajorVerNum);
    ok(typeattr->wMinorVerNum == 0, "wMinorVerNum = %d\n", typeattr->wMinorVerNum);
    ITypeInfo_ReleaseTypeAttr(ti, typeattr);

    VariantClear(&cust_data);
    hres = ITypeInfo2_GetCustData(ti2, &custguid, &cust_data);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(V_VT(&cust_data) == VT_BSTR, "got wrong custdata type: %u\n", V_VT(&cust_data));
    ok(!lstrcmpW(V_BSTR(&cust_data), asdfW), "got wrong custdata value: %s\n", wine_dbgstr_w(V_BSTR(&cust_data)));
    SysFreeString(V_BSTR(&cust_data));

    ITypeInfo2_Release(ti2);
    ok(ITypeInfo_Release(ti) == 0, "Object should be freed\n");

    hres = ITypeLib_GetTypeInfo(tl, 3, &ti);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ITypeInfo_GetTypeAttr(ti, &typeattr);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(typeattr->cbSizeInstance == sizeof(void*), "cbSizeInstance = %ld\n", typeattr->cbSizeInstance);
    ok(typeattr->typekind == TKIND_COCLASS, "typekind = %d\n", typeattr->typekind);
    ok(typeattr->cFuncs == 0, "cFuncs = %d\n", typeattr->cFuncs);
    ok(typeattr->cVars == 0, "cVars = %d\n", typeattr->cVars);
    ok(typeattr->cImplTypes == 3, "cImplTypes = %d\n", typeattr->cImplTypes);
    ok(typeattr->cbSizeVft == 0, "cbSizeVft = %d\n", typeattr->cbSizeVft);
    ok(typeattr->cbAlignment == alignment, "cbAlignment = %d\n", typeattr->cbAlignment);
    ok(typeattr->wTypeFlags == 0, "wTypeFlags = %d\n", typeattr->wTypeFlags);
    ok(typeattr->wMajorVerNum == 0, "wMajorVerNum = %d\n", typeattr->wMajorVerNum);
    ok(typeattr->wMinorVerNum == 0, "wMinorVerNum = %d\n", typeattr->wMinorVerNum);
    ITypeInfo_ReleaseTypeAttr(ti, typeattr);

    hres = ITypeInfo_GetRefTypeOfImplType(ti, 0, &hreftype);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(hreftype == 0, "got wrong hreftype: %lx\n", hreftype);

    hres = ITypeInfo_GetImplTypeFlags(ti, 0, &impltypeflags);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(impltypeflags == IMPLTYPEFLAG_FDEFAULT, "got wrong flag: %x\n", impltypeflags);

    hres = ITypeInfo_GetRefTypeOfImplType(ti, 1, &hreftype);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(hreftype == 1, "got wrong hreftype: %lx\n", hreftype);

    hres = ITypeInfo_GetImplTypeFlags(ti, 1, &impltypeflags);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(impltypeflags == IMPLTYPEFLAG_FRESTRICTED, "got wrong flag: %x\n", impltypeflags);

    hres = ITypeInfo_GetRefTypeOfImplType(ti, 2, &hreftype);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(hreftype == 1, "got wrong hreftype: %lx\n", hreftype);

    hres = ITypeInfo_GetImplTypeFlags(ti, 2, &impltypeflags);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(impltypeflags == 0, "got wrong flag: %x\n", impltypeflags);

    hres = ITypeInfo_GetRefTypeOfImplType(ti, 3, &hreftype);
    ok(hres == TYPE_E_ELEMENTNOTFOUND, "got %08lx\n", hres);

    ok(ITypeInfo_Release(ti) == 0, "Object should be freed\n");

    hres = ITypeLib_GetTypeInfo(tl, 4, &ti);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ITypeInfo_GetTypeAttr(ti, &typeattr);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(typeattr->cbSizeInstance == sizeof(void*), "cbSizeInstance = %ld\n", typeattr->cbSizeInstance);
    ok(typeattr->typekind == TKIND_DISPATCH, "typekind = %d\n", typeattr->typekind);
    todo_wine_if(sys != native_sys)
    ok(typeattr->cFuncs == 8, "cFuncs = %d\n", typeattr->cFuncs);
    ok(typeattr->cVars == 0, "cVars = %d\n", typeattr->cVars);
    ok(typeattr->cImplTypes == 1, "cImplTypes = %d\n", typeattr->cImplTypes);
    ok(typeattr->cbSizeVft == 7 * sizeof(void*), "cbSizeVft = %d\n", typeattr->cbSizeVft);
    ok(typeattr->cbAlignment == alignment, "cbAlignment = %d\n", typeattr->cbAlignment);
    ok(typeattr->wTypeFlags == (TYPEFLAG_FDISPATCHABLE | TYPEFLAG_FDUAL), "wTypeFlags = 0x%x\n", typeattr->wTypeFlags);
    ok(typeattr->wMajorVerNum == 0, "wMajorVerNum = %d\n", typeattr->wMajorVerNum);
    ok(typeattr->wMinorVerNum == 0, "wMinorVerNum = %d\n", typeattr->wMinorVerNum);
    ITypeInfo_ReleaseTypeAttr(ti, typeattr);

    hres = ITypeInfo_GetTypeComp(ti, &tcomp);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ITypeInfo_QueryInterface(ti, &IID_ITypeComp, (void**)&tcomp2);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(tcomp == tcomp2, "got %p, was %p\n", tcomp2, tcomp);
    ITypeComp_Release(tcomp2);

    hres = ITypeComp_Bind(tcomp, invokeW, 0, INVOKE_FUNC, &interface1, &desckind, &bindptr);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(desckind == DESCKIND_FUNCDESC, "got wrong desckind: 0x%x\n", desckind);
    ok(bindptr.lpfuncdesc->memid == 0x60010003, "got %lx\n", bindptr.lpfuncdesc->memid);
    ok(bindptr.lpfuncdesc->lprgscode == NULL, "got %p\n", bindptr.lpfuncdesc->lprgscode);
    ok(bindptr.lpfuncdesc->lprgelemdescParam != NULL, "got %p\n", bindptr.lpfuncdesc->lprgelemdescParam);
    ok(bindptr.lpfuncdesc->funckind == FUNC_DISPATCH, "got 0x%x\n", bindptr.lpfuncdesc->funckind);
    ok(bindptr.lpfuncdesc->invkind == INVOKE_FUNC, "got 0x%x\n", bindptr.lpfuncdesc->invkind);
    ok(bindptr.lpfuncdesc->callconv == CC_STDCALL, "got 0x%x\n", bindptr.lpfuncdesc->callconv);
    ok(bindptr.lpfuncdesc->cParams == 8, "got %d\n", bindptr.lpfuncdesc->cParams);
    ok(bindptr.lpfuncdesc->cParamsOpt == 0, "got %d\n", bindptr.lpfuncdesc->cParamsOpt);
    ok(bindptr.lpfuncdesc->oVft == 6 * sizeof(void*), "got %x\n", bindptr.lpfuncdesc->oVft);
    ok(bindptr.lpfuncdesc->cScodes == 0, "got %d\n", bindptr.lpfuncdesc->cScodes);
    ok(bindptr.lpfuncdesc->elemdescFunc.tdesc.vt == VT_VOID, "got %d\n", bindptr.lpfuncdesc->elemdescFunc.tdesc.vt);
    ok(bindptr.lpfuncdesc->wFuncFlags == FUNCFLAG_FRESTRICTED, "got 0x%x\n", bindptr.lpfuncdesc->wFuncFlags);

    ITypeInfo_ReleaseFuncDesc(interface1, bindptr.lpfuncdesc);
    ITypeInfo_Release(interface1);
    ITypeComp_Release(tcomp);

    hres = ITypeInfo_GetRefTypeOfImplType(ti, -1, &hreftype);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(hreftype == -2, "got wrong hreftype: %lx\n", hreftype);

    hres = ITypeInfo_GetRefTypeInfo(ti, hreftype, &interface1);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ITypeInfo_GetTypeAttr(interface1, &typeattr);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(typeattr->cbSizeInstance == sizeof(void*), "cbSizeInstance = %ld\n", typeattr->cbSizeInstance);
    ok(typeattr->typekind == TKIND_INTERFACE, "typekind = %d\n", typeattr->typekind);
    ok(typeattr->cFuncs == 1, "cFuncs = %d\n", typeattr->cFuncs);
    ok(typeattr->cVars == 0, "cVars = %d\n", typeattr->cVars);
    ok(typeattr->cImplTypes == 1, "cImplTypes = %d\n", typeattr->cImplTypes);
    todo_wine_if(sys != native_sys)
        ok(typeattr->cbSizeVft == 8 * sizeof(void*), "cbSizeVft = %d\n", typeattr->cbSizeVft);
    ok(typeattr->cbAlignment == alignment, "cbAlignment = %d\n", typeattr->cbAlignment);
    ok(typeattr->wTypeFlags == (TYPEFLAG_FDISPATCHABLE | TYPEFLAG_FDUAL), "wTypeFlags = 0x%x\n", typeattr->wTypeFlags);
    ok(typeattr->wMajorVerNum == 0, "wMajorVerNum = %d\n", typeattr->wMajorVerNum);
    ok(typeattr->wMinorVerNum == 0, "wMinorVerNum = %d\n", typeattr->wMinorVerNum);
    ITypeInfo_ReleaseTypeAttr(interface1, typeattr);

    ITypeInfo_Release(interface1);

    ok(ITypeInfo_Release(ti) == 0, "Object should be freed\n");

    hres = ITypeLib_GetTypeInfo(tl, 5, &ti);
    ok(hres == S_OK, "got %08lx\n", hres);

    hres = ITypeInfo_GetTypeAttr(ti, &typeattr);
    ok(hres == S_OK, "got %08lx\n", hres);
    ok(typeattr->cbSizeInstance == 8, "cbSizeInstance = %ld\n", typeattr->cbSizeInstance);
    ok(typeattr->typekind == TKIND_ALIAS, "typekind = %d\n", typeattr->typekind);
    ok(typeattr->cFuncs == 0, "cFuncs = %d\n", typeattr->cFuncs);
    ok(typeattr->cVars == 0, "cVars = %d\n", typeattr->cVars);
    ok(typeattr->cImplTypes == 0, "cImplTypes = %d\n", typeattr->cImplTypes);
    ok(typeattr->cbSizeVft == 0, "cbSizeVft = %d\n", typeattr->cbSizeVft);
    ok(typeattr->cbAlignment == alignment, "cbAlignment = %d\n", typeattr->cbAlignment);
    ok(typeattr->wTypeFlags == 0, "wTypeFlags = 0x%x\n", typeattr->wTypeFlags);
    ok(typeattr->wMajorVerNum == 0, "wMajorVerNum = %d\n", typeattr->wMajorVerNum);
    ok(typeattr->wMinorVerNum == 0, "wMinorVerNum = %d\n", typeattr->wMinorVerNum);
    ok(typeattr->tdescAlias.vt == VT_R8, "Got wrong tdescAlias.vt: %u\n", typeattr->tdescAlias.vt);
    ITypeInfo_ReleaseTypeAttr(ti, typeattr);

    ok(ITypeInfo_Release(ti) == 0, "Object should be freed\n");

    ok(ITypeLib_Release(tl)==0, "Object should be freed\n");

    DeleteFileA(filename);
    winetest_pop_context();
}

#if 0       /* use this to generate more tests */

#define OLE_CHECK(x) { HRESULT hr = x; if (FAILED(hr)) { printf(#x "failed - %x\n", hr); return; } }

static char *dump_string(LPWSTR wstr)
{
    int size = lstrlenW(wstr)+1;
    char *out = CoTaskMemAlloc(size);
    WideCharToMultiByte(20127, 0, wstr, -1, out, size, NULL, NULL);
    return out;
}

struct map_entry
{
    DWORD value;
    const char *name;
};

#define MAP_ENTRY(x) { x, #x }
static const struct map_entry tkind_map[] = {
    MAP_ENTRY(TKIND_ENUM),
    MAP_ENTRY(TKIND_RECORD),
    MAP_ENTRY(TKIND_MODULE),
    MAP_ENTRY(TKIND_INTERFACE),
    MAP_ENTRY(TKIND_DISPATCH),
    MAP_ENTRY(TKIND_COCLASS),
    MAP_ENTRY(TKIND_ALIAS),
    MAP_ENTRY(TKIND_UNION),
    MAP_ENTRY(TKIND_MAX),
    {0, NULL}
};

static const struct map_entry funckind_map[] = {
    MAP_ENTRY(FUNC_VIRTUAL),
    MAP_ENTRY(FUNC_PUREVIRTUAL),
    MAP_ENTRY(FUNC_NONVIRTUAL),
    MAP_ENTRY(FUNC_STATIC),
    MAP_ENTRY(FUNC_DISPATCH),
    {0, NULL}
};

static const struct map_entry varkind_map[] = {
    MAP_ENTRY(VAR_PERINSTANCE),
    MAP_ENTRY(VAR_STATIC),
    MAP_ENTRY(VAR_CONST),
    MAP_ENTRY(VAR_DISPATCH),
    {0, NULL}
};

static const struct map_entry invkind_map[] = {
    MAP_ENTRY(INVOKE_FUNC),
    MAP_ENTRY(INVOKE_PROPERTYGET),
    MAP_ENTRY(INVOKE_PROPERTYPUT),
    MAP_ENTRY(INVOKE_PROPERTYPUTREF),
    {0, NULL}
};

static const struct map_entry callconv_map[] = {
    MAP_ENTRY(CC_FASTCALL),
    MAP_ENTRY(CC_CDECL),
    MAP_ENTRY(CC_PASCAL),
    MAP_ENTRY(CC_MACPASCAL),
    MAP_ENTRY(CC_STDCALL),
    MAP_ENTRY(CC_FPFASTCALL),
    MAP_ENTRY(CC_SYSCALL),
    MAP_ENTRY(CC_MPWCDECL),
    MAP_ENTRY(CC_MPWPASCAL),
    {0, NULL}
};

static const struct map_entry vt_map[] = {
    MAP_ENTRY(VT_EMPTY),
    MAP_ENTRY(VT_NULL),
    MAP_ENTRY(VT_I2),
    MAP_ENTRY(VT_I4),
    MAP_ENTRY(VT_R4),
    MAP_ENTRY(VT_R8),
    MAP_ENTRY(VT_CY),
    MAP_ENTRY(VT_DATE),
    MAP_ENTRY(VT_BSTR),
    MAP_ENTRY(VT_DISPATCH),
    MAP_ENTRY(VT_ERROR),
    MAP_ENTRY(VT_BOOL),
    MAP_ENTRY(VT_VARIANT),
    MAP_ENTRY(VT_UNKNOWN),
    MAP_ENTRY(VT_DECIMAL),
    MAP_ENTRY(15),
    MAP_ENTRY(VT_I1),
    MAP_ENTRY(VT_UI1),
    MAP_ENTRY(VT_UI2),
    MAP_ENTRY(VT_UI4),
    MAP_ENTRY(VT_I8),
    MAP_ENTRY(VT_UI8),
    MAP_ENTRY(VT_INT),
    MAP_ENTRY(VT_UINT),
    MAP_ENTRY(VT_VOID),
    MAP_ENTRY(VT_HRESULT),
    MAP_ENTRY(VT_PTR),
    MAP_ENTRY(VT_SAFEARRAY),
    MAP_ENTRY(VT_CARRAY),
    MAP_ENTRY(VT_USERDEFINED),
    MAP_ENTRY(VT_LPSTR),
    MAP_ENTRY(VT_LPWSTR),
    MAP_ENTRY(VT_RECORD),
    MAP_ENTRY(VT_INT_PTR),
    MAP_ENTRY(VT_UINT_PTR),
    MAP_ENTRY(39),
    MAP_ENTRY(40),
    MAP_ENTRY(41),
    MAP_ENTRY(42),
    MAP_ENTRY(43),
    MAP_ENTRY(44),
    MAP_ENTRY(45),
    MAP_ENTRY(46),
    MAP_ENTRY(47),
    MAP_ENTRY(48),
    MAP_ENTRY(49),
    MAP_ENTRY(50),
    MAP_ENTRY(51),
    MAP_ENTRY(52),
    MAP_ENTRY(53),
    MAP_ENTRY(54),
    MAP_ENTRY(55),
    MAP_ENTRY(56),
    MAP_ENTRY(57),
    MAP_ENTRY(58),
    MAP_ENTRY(59),
    MAP_ENTRY(60),
    MAP_ENTRY(61),
    MAP_ENTRY(62),
    MAP_ENTRY(63),
    MAP_ENTRY(VT_FILETIME),
    MAP_ENTRY(VT_BLOB),
    MAP_ENTRY(VT_STREAM),
    MAP_ENTRY(VT_STORAGE),
    MAP_ENTRY(VT_STREAMED_OBJECT),
    MAP_ENTRY(VT_STORED_OBJECT),
    MAP_ENTRY(VT_BLOB_OBJECT),
    MAP_ENTRY(VT_CF),
    MAP_ENTRY(VT_CLSID),
    {0, NULL}
};

#undef MAP_ENTRY

static const char *map_value(int val, const struct map_entry *map)
{
    static int map_id;
    static char bufs[16][256];
    char *buf;

    while (map->name)
    {
        if (map->value == val)
            return map->name;
        map++;
    }

    buf = bufs[(map_id++)%16];
    sprintf(buf, "%d", val);
    return buf;
}

static const char *dump_type_flags(DWORD flags)
{
    static char buf[256];

    if (!flags) return "0";

    buf[0] = 0;

#define ADD_FLAG(x) if (flags & x) { if (buf[0]) strcat(buf, "|"); strcat(buf, #x); flags &= ~x; }
    ADD_FLAG(TYPEFLAG_FPROXY)
    ADD_FLAG(TYPEFLAG_FREVERSEBIND)
    ADD_FLAG(TYPEFLAG_FDISPATCHABLE)
    ADD_FLAG(TYPEFLAG_FREPLACEABLE)
    ADD_FLAG(TYPEFLAG_FAGGREGATABLE)
    ADD_FLAG(TYPEFLAG_FRESTRICTED)
    ADD_FLAG(TYPEFLAG_FOLEAUTOMATION)
    ADD_FLAG(TYPEFLAG_FNONEXTENSIBLE)
    ADD_FLAG(TYPEFLAG_FDUAL)
    ADD_FLAG(TYPEFLAG_FCONTROL)
    ADD_FLAG(TYPEFLAG_FHIDDEN)
    ADD_FLAG(TYPEFLAG_FPREDECLID)
    ADD_FLAG(TYPEFLAG_FLICENSED)
    ADD_FLAG(TYPEFLAG_FCANCREATE)
    ADD_FLAG(TYPEFLAG_FAPPOBJECT)
#undef ADD_FLAG

    assert(!flags);
    assert(strlen(buf) < sizeof(buf));

    return buf;
}

static char *print_size(BSTR name, TYPEATTR *attr)
{
    static char buf[256];

    switch (attr->typekind)
    {
    case TKIND_DISPATCH:
    case TKIND_INTERFACE:
        sprintf(buf, "sizeof(%s*)", dump_string(name));
        break;

    case TKIND_RECORD:
        sprintf(buf, "sizeof(struct %s)", dump_string(name));
        break;

    case TKIND_UNION:
        sprintf(buf, "sizeof(union %s)", dump_string(name));
        break;

    case TKIND_ALIAS:
        sprintf(buf, "sizeof(%s)", dump_string(name));
        break;

    case TKIND_ENUM:
        sprintf(buf, "4");
        break;

    default:
        assert(0);
        return NULL;
    }

    return buf;
}

static char *print_align(BSTR name, TYPEATTR *attr)
{
    static char buf[256];

    switch (attr->typekind)
    {
    case TKIND_DISPATCH:
    case TKIND_INTERFACE:
        sprintf(buf, "TYPE_ALIGNMENT(%s*)", dump_string(name));
        break;

    case TKIND_RECORD:
        sprintf(buf, "TYPE_ALIGNMENT(struct %s)", dump_string(name));
        break;

    case TKIND_UNION:
        sprintf(buf, "TYPE_ALIGNMENT(union %s)", dump_string(name));
        break;

    case TKIND_ALIAS:
        sprintf(buf, "TYPE_ALIGNMENT(%s)", dump_string(name));
        break;

    case TKIND_ENUM:
        sprintf(buf, "4");
        break;

    default:
        assert(0);
        return NULL;
    }

    return buf;
}

static const char *dump_param_flags(DWORD flags)
{
    static char buf[256];

    if (!flags) return "PARAMFLAG_NONE";

    buf[0] = 0;

#define ADD_FLAG(x) if (flags & x) { if (buf[0]) strcat(buf, "|"); strcat(buf, #x); flags &= ~x; }
    ADD_FLAG(PARAMFLAG_FIN)
    ADD_FLAG(PARAMFLAG_FOUT)
    ADD_FLAG(PARAMFLAG_FLCID)
    ADD_FLAG(PARAMFLAG_FRETVAL)
    ADD_FLAG(PARAMFLAG_FOPT)
    ADD_FLAG(PARAMFLAG_FHASDEFAULT)
    ADD_FLAG(PARAMFLAG_FHASCUSTDATA)
#undef ADD_FLAG

    assert(!flags);
    assert(strlen(buf) < sizeof(buf));

    return buf;
}

static const char *dump_func_flags(DWORD flags)
{
    static char buf[256];

    if (!flags) return "0";

    buf[0] = 0;

#define ADD_FLAG(x) if (flags & x) { if (buf[0]) strcat(buf, "|"); strcat(buf, #x); flags &= ~x; }
    ADD_FLAG(FUNCFLAG_FRESTRICTED)
    ADD_FLAG(FUNCFLAG_FSOURCE)
    ADD_FLAG(FUNCFLAG_FBINDABLE)
    ADD_FLAG(FUNCFLAG_FREQUESTEDIT)
    ADD_FLAG(FUNCFLAG_FDISPLAYBIND)
    ADD_FLAG(FUNCFLAG_FDEFAULTBIND)
    ADD_FLAG(FUNCFLAG_FHIDDEN)
    ADD_FLAG(FUNCFLAG_FUSESGETLASTERROR)
    ADD_FLAG(FUNCFLAG_FDEFAULTCOLLELEM)
    ADD_FLAG(FUNCFLAG_FUIDEFAULT)
    ADD_FLAG(FUNCFLAG_FNONBROWSABLE)
    ADD_FLAG(FUNCFLAG_FREPLACEABLE)
    ADD_FLAG(FUNCFLAG_FIMMEDIATEBIND)
#undef ADD_FLAG

    assert(!flags);
    assert(strlen(buf) < sizeof(buf));

    return buf;
}

static const char *dump_var_flags(DWORD flags)
{
    static char buf[256];

    if (!flags) return "0";

    buf[0] = 0;

#define ADD_FLAG(x) if (flags & x) { if (buf[0]) strcat(buf, "|"); strcat(buf, #x); flags &= ~x; }
    ADD_FLAG(VARFLAG_FREADONLY)
    ADD_FLAG(VARFLAG_FSOURCE)
    ADD_FLAG(VARFLAG_FBINDABLE)
    ADD_FLAG(VARFLAG_FREQUESTEDIT)
    ADD_FLAG(VARFLAG_FDISPLAYBIND)
    ADD_FLAG(VARFLAG_FDEFAULTBIND)
    ADD_FLAG(VARFLAG_FHIDDEN)
    ADD_FLAG(VARFLAG_FRESTRICTED)
    ADD_FLAG(VARFLAG_FDEFAULTCOLLELEM)
    ADD_FLAG(VARFLAG_FUIDEFAULT)
    ADD_FLAG(VARFLAG_FNONBROWSABLE)
    ADD_FLAG(VARFLAG_FREPLACEABLE)
    ADD_FLAG(VARFLAG_FIMMEDIATEBIND)
#undef ADD_FLAG

    assert(!flags);
    assert(strlen(buf) < sizeof(buf));

    return buf;
}

static const char *dump_variant_info(const VARIANT *v)
{
    const char *vt_str = map_value(V_VT(v), vt_map);
    static char buf[256];
    switch(V_VT(v)) {
        case VT_I1: sprintf(buf, "{ %s, { .value_int = %d } }", vt_str, V_I1(v)); break;
        case VT_I2: sprintf(buf, "{ %s, { .value_int = %d } }", vt_str, V_I2(v)); break;
        case VT_I4: sprintf(buf, "{ %s, { .value_int = %d } }", vt_str, V_I4(v)); break;
        case VT_I8: sprintf(buf, "{ %s, { .value_int = %s } }", vt_str, wine_dbgstr_longlong(V_I8(v))); break;
        case VT_INT: sprintf(buf, "{ %s, { .value_int = %d } }", vt_str, V_UINT(v)); break;
        case VT_BOOL: sprintf(buf, "{ %s, { .value_int = %d } }", vt_str, V_BOOL(v)); break;

        case VT_UI1: sprintf(buf, "{ %s, { .value_uint = %u } }", vt_str, V_UI1(v)); break;
        case VT_UI2: sprintf(buf, "{ %s, { .value_uint = %u } }", vt_str, V_UI2(v)); break;
        case VT_UI4: sprintf(buf, "{ %s, { .value_uint = %u } }", vt_str, V_UI4(v)); break;
        case VT_UI8: sprintf(buf, "{ %s, { .value_uint = %u } }", vt_str, wine_dbgstr_longlong(V_UI8(v))); break;
        case VT_UINT: sprintf(buf, "{ %s, { .value_uint = %u } }", vt_str, V_UINT(v)); break;

        case VT_R4: sprintf(buf, "{ %s, { .value_float = %0.9g } }", vt_str, V_R4(v)); break;
        case VT_R8: sprintf(buf, "{ %s, { .value_float = %0.17g } }", vt_str, V_R8(v)); break;

        case VT_BSTR: sprintf(buf, "{ %s, { .value_str = \"%s\" } }", vt_str, dump_string(V_BSTR(v))); break;
        default:
            printf("failed - dump_variant_info: cannot serialize %s\n", vt_str);
            sprintf(buf, "{ %s, { /* cannot dump */ } }", vt_str);
    }
    return buf;
}

static const char *dump_custdata_info(LPCUSTDATAITEM item) {
    static char buf[256];
    sprintf(buf, "{ \"%s\", %s }", wine_dbgstr_guid(&item->guid), dump_variant_info(&item->varValue));
    return buf;
}

static int get_href_type(ITypeInfo *info, TYPEDESC *tdesc)
{
    int href_type = -1;

    if (tdesc->vt == VT_USERDEFINED)
    {
        HRESULT hr;
        ITypeInfo *param;
        TYPEATTR *attr;

        hr = ITypeInfo_GetRefTypeInfo(info, tdesc->hreftype, &param);
        ok(hr == S_OK, "GetRefTypeInfo error %#x\n", hr);
        hr = ITypeInfo_GetTypeAttr(param, &attr);
        ok(hr == S_OK, "GetTypeAttr error %#x\n", hr);

        href_type = attr->typekind;

        ITypeInfo_ReleaseTypeAttr(param, attr);
        ITypeInfo_Release(param);
    }

    return href_type;
}

static void test_dump_typelib(const WCHAR *name)
{
    ITypeInfo *info;
    ITypeInfo2 *info2;
    ITypeLib *lib;
    int count;
    int i;
    HREFTYPE hRefType = 0;
    CUSTDATA cust_data;

    OLE_CHECK(LoadTypeLib(name, &lib));

    printf("/*** Autogenerated data. Do not edit, change the generator above instead. ***/\n");

    count = ITypeLib_GetTypeInfoCount(lib);
    for (i = 0; i < count;)
    {
        TYPEATTR *attr;
        BSTR name;
        DWORD help_ctx;
        int f = 0, v = 0, c = 0;

        OLE_CHECK(ITypeLib_GetDocumentation(lib, i, &name, NULL, &help_ctx, NULL));
        printf("{\n"
               "  \"%s\",\n", dump_string(name));

        OLE_CHECK(ITypeLib_GetTypeInfo(lib, i, &info));
        if (hRefType)
        {
            ITypeInfo *refInfo;
            OLE_CHECK(ITypeInfo_GetRefTypeInfo(info, hRefType, &refInfo));
            ITypeInfo_Release(info);
            info = refInfo;
        }
        OLE_CHECK(ITypeInfo_QueryInterface(info, &IID_ITypeInfo2, (void**)&info2));

        OLE_CHECK(ITypeInfo_GetTypeAttr(info, &attr));
        OLE_CHECK(ITypeInfo2_GetAllCustData(info2,&cust_data));

        printf("  \"%s\",\n", wine_dbgstr_guid(&attr->guid));

        printf("  /*kind*/ %s, /*flags*/ %s, /*align*/ %s, /*size*/ %s,\n"
               "  /*helpctx*/ 0x%04x, /*version*/ 0x%08x, /*#vtbl*/ %d, /*#func*/ %d, /*#var*/ %d,\n",
            map_value(attr->typekind, tkind_map), dump_type_flags(attr->wTypeFlags),
            print_align(name, attr), print_size(name, attr),
            help_ctx, MAKELONG(attr->wMinorVerNum, attr->wMajorVerNum),
            attr->cbSizeVft/sizeof(void*), attr->cFuncs, attr->cVars);

        printf("  /*#custdata*/ %d, %s\n", cust_data.cCustData, cust_data.cCustData ? "{" : "{},");
        for (c = 0; c < cust_data.cCustData; ++c) {
            printf("    %s,\n", dump_custdata_info(&cust_data.prgCustData[c]));
        }
        if (cust_data.cCustData) printf("  },\n");
        ClearCustData(&cust_data);

        printf("  { /* funcs */%s", attr->cFuncs ? "\n" : " },\n");
        while (1)
        {
            FUNCDESC *desc;
            BSTR tab[256];
            UINT cNames;
            int p;

            if (FAILED(ITypeInfo_GetFuncDesc(info, f, &desc)))
                break;
            OLE_CHECK(ITypeInfo2_GetAllFuncCustData(info2,f,&cust_data));
            printf("    {\n"
                   "      /*id*/ 0x%x, /*func*/ %s, /*inv*/ %s, /*call*/ %s,\n",
                desc->memid, map_value(desc->funckind, funckind_map), map_value(desc->invkind, invkind_map),
                map_value(desc->callconv, callconv_map));
            printf("      /*#param*/ %d, /*#opt*/ %d, /*vtbl*/ %d, /*#scodes*/ %d, /*flags*/ %s,\n",
                desc->cParams, desc->cParamsOpt, desc->oVft/sizeof(void*), desc->cScodes, dump_func_flags(desc->wFuncFlags));
            printf("      {%s, %s, %s}, /* ret */\n", map_value(desc->elemdescFunc.tdesc.vt, vt_map),
                map_value(get_href_type(info, &desc->elemdescFunc.tdesc), tkind_map), dump_param_flags(U(desc->elemdescFunc).paramdesc.wParamFlags));
            printf("      /*#custdata*/ %d, %s\n", cust_data.cCustData, cust_data.cCustData ? "{" : "{},");
            for (c = 0; c < cust_data.cCustData; ++c) {
                printf("        %s,\n", dump_custdata_info(&cust_data.prgCustData[c]));
            }
            if (cust_data.cCustData) printf("      },\n");
            ClearCustData(&cust_data);

            printf("      { /* params */\n");
            for (p = 0; p < desc->cParams; p++)
            {
                ELEMDESC e = desc->lprgelemdescParam[p];
                OLE_CHECK(ITypeInfo2_GetAllParamCustData(info2,f,p,&cust_data));
                printf("        {%s, %s, %s", map_value(e.tdesc.vt, vt_map),
                    map_value(get_href_type(info, &e.tdesc), tkind_map), dump_param_flags(e.paramdesc.wParamFlags));
                if (cust_data.cCustData) {
                    printf(", /*#custdata*/ %d, {\n", cust_data.cCustData);
                    for (c = 0; c < cust_data.cCustData; ++c) {
                        printf("          %s,\n", dump_custdata_info(&cust_data.prgCustData[c]));
                    }
                    printf("        } },\n");
                } else {
                    printf("},\n");
                }
                ClearCustData(&cust_data);
            }
            printf("        {-1, 0, 0}\n");
            printf("      },\n");
            printf("      { /* names */\n");
            OLE_CHECK(ITypeInfo_GetNames(info, desc->memid, tab, 256, &cNames));
            for (p = 0; p < cNames; p++)
            {
                printf("        \"%s\",\n", dump_string(tab[p]));
                SysFreeString(tab[p]);
            }
            printf("        NULL,\n");
            printf("      },\n");
            printf("    },\n");
            ITypeInfo_ReleaseFuncDesc(info, desc);
            f++;
        }
        if (attr->cFuncs) printf("  },\n");

        printf("  { /* vars */%s", attr->cVars ? "\n" : " },\n");
        while (1)
        {
            VARDESC *desc;
            BSTR varname;
            UINT cNames;
            if (FAILED(ITypeInfo_GetVarDesc(info, v, &desc)))
                break;
            OLE_CHECK(ITypeInfo2_GetAllVarCustData(info2,v,&cust_data));
            OLE_CHECK(ITypeInfo_GetNames(info, desc->memid, &varname, 1, &cNames));
            if(cNames!=1) { printf("GetNames failed - VARDESC should have one name, got %d\n", cNames); return; }
            printf("    {\n"
                   "      /*id*/ 0x%x, /*name*/ \"%s\", /*flags*/ %s, /*kind*/ %s,\n",
                desc->memid, dump_string(varname), dump_var_flags(desc->wVarFlags), map_value(desc->varkind, varkind_map));
            SysFreeString(varname);
            if (desc->varkind == VAR_PERINSTANCE) {
                printf("      { .oInst = %d },\n", desc->DUMMYUNIONNAME.oInst);
            } else if (desc->varkind == VAR_CONST) {
                printf("      { .varValue = %s },\n", dump_variant_info(desc->DUMMYUNIONNAME.lpvarValue));
            } else {
                printf("      { /* DUMMYUNIONNAME unused*/ },\n");
            }

            printf("      /*#custdata*/ %d, %s\n", cust_data.cCustData, cust_data.cCustData ? "{" : "{},");
            for (c = 0; c < cust_data.cCustData; ++c) {
                printf("        %s,\n", dump_custdata_info(&cust_data.prgCustData[c]));
            }
            if (cust_data.cCustData) printf("      },\n");
            ClearCustData(&cust_data);

            printf("      {%s, %s, %s}, /* ret */\n", map_value(desc->elemdescVar.tdesc.vt, vt_map),
                map_value(get_href_type(info, &desc->elemdescVar.tdesc), tkind_map), dump_param_flags(U(desc->elemdescVar).paramdesc.wParamFlags));
            printf("    },\n");
            ITypeInfo_ReleaseVarDesc(info, desc);
            v++;
        }
        if (attr->cVars) printf("  },\n");

        printf("},\n");

        if ((attr->typekind == TKIND_DISPATCH) && (attr->wTypeFlags & TYPEFLAG_FDUAL) &&
            SUCCEEDED(ITypeInfo_GetRefTypeOfImplType(info, -1, &hRefType)))
        {
            /* next iteration dumps hRefType, the TKIND_INTERFACE reference underneath this [dual] TKIND_DISPATCH */
        }
        else
        {
            i++; /* move to the next item in lib */
            hRefType = 0;
        }

        ITypeInfo_ReleaseTypeAttr(info, attr);
        ITypeInfo2_Release(info2);
        ITypeInfo_Release(info);
        SysFreeString(name);
    }
    ITypeLib_Release(lib);
}

#else

typedef struct _variant_info {
    VARTYPE vt;
    union {
        INT64 value_int;
        UINT64 value_uint;
        double value_float;
        const char * value_str;
    };
} variant_info;

typedef struct _custdata_info {
    LPCSTR uuid;
    variant_info value;
} custdata_info;

typedef struct _element_info
{
    VARTYPE vt;
    TYPEKIND type;
    USHORT wParamFlags;
    DWORD cCustData;
    custdata_info custdata[5];
} element_info;

typedef struct _function_info
{
    MEMBERID memid;
    FUNCKIND funckind;
    INVOKEKIND invkind;
    CALLCONV callconv;
    short cParams;
    short cParamsOpt;
    short vtbl_index;
    short cScodes;
    WORD wFuncFlags;
    element_info ret_type;
    DWORD cCustData;
    custdata_info custdata[5];
    element_info params[15];
    LPCSTR names[15];
} function_info;

typedef struct _var_info
{
    MEMBERID memid;
    LPCSTR name;
    WORD wVarFlags;
    VARKIND varkind;
    union {
        ULONG oInst; /* VAR_PERINSTANCE */
        variant_info varValue; /* VAR_CONST */
    } DUMMYUNIONNAME;
    DWORD cCustData;
    custdata_info custdata[5];
    element_info elemdescVar;
} var_info;

typedef struct _type_info
{
    LPCSTR name;
    LPCSTR uuid;
    TYPEKIND type;
    WORD wTypeFlags;
    USHORT cbAlignment;
    USHORT cbSizeInstance;
    USHORT help_ctx;
    DWORD version;
    USHORT cbSizeVft;
    USHORT cFuncs;
    USHORT cVars;
    DWORD cCustData;
    custdata_info custdata[5];
    function_info funcs[20];
    var_info vars[20];
} type_info;

static const SYSKIND info_syskind = SYS_WIN32;
static const type_info info[] = {
/*** Autogenerated data. Do not edit, change the generator above instead. ***/
{
  "g",
  "{b14b6bb5-904e-4ff9-b247-bd361f7a0001}",
  /*kind*/ TKIND_RECORD, /*flags*/ 0, /*align*/ TYPE_ALIGNMENT(struct g), /*size*/ sizeof(struct g),
  /*helpctx*/ 0x0000, /*version*/ 0x00000000, /*#vtbl*/ 0, /*#func*/ 0, /*#var*/ 1,
  /*#custdata*/ 0, {},
  { /* funcs */ },
  { /* vars */
    {
      /*id*/ 0x40000000, /*name*/ "g1", /*flags*/ 0, /*kind*/ VAR_PERINSTANCE,
      { .oInst = 0 },
      /*#custdata*/ 0, {},
      {VT_INT, -1, PARAMFLAG_NONE}, /* ret */
    },
  },
},
{
  "test_iface",
  "{b14b6bb5-904e-4ff9-b247-bd361f7a0002}",
  /*kind*/ TKIND_INTERFACE, /*flags*/ 0, /*align*/ TYPE_ALIGNMENT(test_iface*), /*size*/ sizeof(test_iface*),
  /*helpctx*/ 0x0000, /*version*/ 0x00000000, /*#vtbl*/ 4, /*#func*/ 1, /*#var*/ 0,
  /*#custdata*/ 0, {},
  { /* funcs */
    {
      /*id*/ 0x60010000, /*func*/ FUNC_PUREVIRTUAL, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 1, /*#opt*/ 0, /*vtbl*/ 3, /*#scodes*/ 0, /*flags*/ 0,
      {VT_HRESULT, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {VT_PTR, -1, PARAMFLAG_FIN},
        {-1, 0, 0}
      },
      { /* names */
        "Test",
        "ptr",
        NULL,
      },
    },
  },
  { /* vars */ },
},
{
  "parent_iface",
  "{b14b6bb5-904e-4ff9-b247-bd361f7aa001}",
  /*kind*/ TKIND_INTERFACE, /*flags*/ 0, /*align*/ TYPE_ALIGNMENT(parent_iface*), /*size*/ sizeof(parent_iface*),
  /*helpctx*/ 0x0000, /*version*/ 0x00000000, /*#vtbl*/ 4, /*#func*/ 1, /*#var*/ 0,
  /*#custdata*/ 0, {},
  { /* funcs */
    {
      /*id*/ 0x60010000, /*func*/ FUNC_PUREVIRTUAL, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 1, /*#opt*/ 0, /*vtbl*/ 3, /*#scodes*/ 0, /*flags*/ 0,
      {VT_HRESULT, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {VT_PTR, -1, PARAMFLAG_FOUT|PARAMFLAG_FRETVAL},
        {-1, 0, 0}
      },
      { /* names */
        "test1",
        "iface",
        NULL,
      },
    },
  },
  { /* vars */ },
},
{
  "child_iface",
  "{b14b6bb5-904e-4ff9-b247-bd361f7aa002}",
  /*kind*/ TKIND_INTERFACE, /*flags*/ 0, /*align*/ TYPE_ALIGNMENT(child_iface*), /*size*/ sizeof(child_iface*),
  /*helpctx*/ 0x0000, /*version*/ 0x00000000, /*#vtbl*/ 5, /*#func*/ 1, /*#var*/ 0,
  /*#custdata*/ 0, {},
  { /* funcs */
    {
      /*id*/ 0x60020000, /*func*/ FUNC_PUREVIRTUAL, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 0, /*#opt*/ 0, /*vtbl*/ 4, /*#scodes*/ 0, /*flags*/ 0,
      {VT_HRESULT, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {-1, 0, 0}
      },
      { /* names */
        "test2",
        NULL,
      },
    },
  },
  { /* vars */ },
},
{
  "_n",
  "{016fe2ec-b2c8-45f8-b23b-39e53a753903}",
  /*kind*/ TKIND_RECORD, /*flags*/ 0, /*align*/ TYPE_ALIGNMENT(struct _n), /*size*/ sizeof(struct _n),
  /*helpctx*/ 0x0003, /*version*/ 0x00010002, /*#vtbl*/ 0, /*#func*/ 0, /*#var*/ 1,
  /*#custdata*/ 0, {},
  { /* funcs */ },
  { /* vars */
    {
      /*id*/ 0x40000000, /*name*/ "n1", /*flags*/ 0, /*kind*/ VAR_PERINSTANCE,
      { .oInst = 0 },
      /*#custdata*/ 0, {},
      {VT_INT, -1, PARAMFLAG_NONE}, /* ret */
    },
  },
},
{
  "n",
  "{016fe2ec-b2c8-45f8-b23b-39e53a753902}",
  /*kind*/ TKIND_ALIAS, /*flags*/ TYPEFLAG_FHIDDEN, /*align*/ TYPE_ALIGNMENT(n), /*size*/ sizeof(n),
  /*helpctx*/ 0x0000, /*version*/ 0x00000000, /*#vtbl*/ 0, /*#func*/ 0, /*#var*/ 0,
  /*#custdata*/ 0, {},
  { /* funcs */ },
  { /* vars */ },
},
{
  "nn",
  "{00000000-0000-0000-0000-000000000000}",
  /*kind*/ TKIND_ALIAS, /*flags*/ 0, /*align*/ TYPE_ALIGNMENT(nn), /*size*/ sizeof(nn),
  /*helpctx*/ 0x0003, /*version*/ 0x00010002, /*#vtbl*/ 0, /*#func*/ 0, /*#var*/ 0,
  /*#custdata*/ 0, {},
  { /* funcs */ },
  { /* vars */ },
},
{
  "_m",
  "{016fe2ec-b2c8-45f8-b23b-39e53a753906}",
  /*kind*/ TKIND_RECORD, /*flags*/ 0, /*align*/ TYPE_ALIGNMENT(struct _m), /*size*/ sizeof(struct _m),
  /*helpctx*/ 0x0003, /*version*/ 0x00000000, /*#vtbl*/ 0, /*#func*/ 0, /*#var*/ 1,
  /*#custdata*/ 0, {},
  { /* funcs */ },
  { /* vars */
    {
      /*id*/ 0x40000000, /*name*/ "m1", /*flags*/ 0, /*kind*/ VAR_PERINSTANCE,
      { .oInst = 0 },
      /*#custdata*/ 0, {},
      {VT_INT, -1, PARAMFLAG_NONE}, /* ret */
    },
  },
},
{
  "m",
  "{016fe2ec-b2c8-45f8-b23b-39e53a753905}",
  /*kind*/ TKIND_ALIAS, /*flags*/ TYPEFLAG_FHIDDEN, /*align*/ TYPE_ALIGNMENT(m), /*size*/ sizeof(m),
  /*helpctx*/ 0x0000, /*version*/ 0x00010002, /*#vtbl*/ 0, /*#func*/ 0, /*#var*/ 0,
  /*#custdata*/ 0, {},
  { /* funcs */ },
  { /* vars */ },
},
{
  "mm",
  "{00000000-0000-0000-0000-000000000000}",
  /*kind*/ TKIND_ALIAS, /*flags*/ 0, /*align*/ TYPE_ALIGNMENT(mm), /*size*/ sizeof(mm),
  /*helpctx*/ 0x0003, /*version*/ 0x00000000, /*#vtbl*/ 0, /*#func*/ 0, /*#var*/ 0,
  /*#custdata*/ 0, {},
  { /* funcs */ },
  { /* vars */ },
},
{
  "IDualIface",
  "{b14b6bb5-904e-4ff9-b247-bd361f7aaedd}",
  /*kind*/ TKIND_DISPATCH, /*flags*/ TYPEFLAG_FDISPATCHABLE|TYPEFLAG_FDUAL, /*align*/ TYPE_ALIGNMENT(IDualIface*), /*size*/ sizeof(IDualIface*),
  /*helpctx*/ 0x0000, /*version*/ 0x00000000, /*#vtbl*/ 7, /*#func*/ 8, /*#var*/ 0,
  /*#custdata*/ 0, {},
  { /* funcs */
    {
      /*id*/ 0x60000000, /*func*/ FUNC_DISPATCH, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 2, /*#opt*/ 0, /*vtbl*/ 0, /*#scodes*/ 0, /*flags*/ FUNCFLAG_FRESTRICTED,
      {VT_VOID, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {VT_PTR, -1, PARAMFLAG_FIN},
        {VT_PTR, -1, PARAMFLAG_FOUT},
        {-1, 0, 0}
      },
      { /* names */
        "QueryInterface",
        "riid",
        "ppvObj",
        NULL,
      },
    },
    {
      /*id*/ 0x60000001, /*func*/ FUNC_DISPATCH, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 0, /*#opt*/ 0, /*vtbl*/ 1, /*#scodes*/ 0, /*flags*/ FUNCFLAG_FRESTRICTED,
      {VT_UI4, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {-1, 0, 0}
      },
      { /* names */
        "AddRef",
        NULL,
      },
    },
    {
      /*id*/ 0x60000002, /*func*/ FUNC_DISPATCH, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 0, /*#opt*/ 0, /*vtbl*/ 2, /*#scodes*/ 0, /*flags*/ FUNCFLAG_FRESTRICTED,
      {VT_UI4, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {-1, 0, 0}
      },
      { /* names */
        "Release",
        NULL,
      },
    },
    {
      /*id*/ 0x60010000, /*func*/ FUNC_DISPATCH, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 1, /*#opt*/ 0, /*vtbl*/ 3, /*#scodes*/ 0, /*flags*/ FUNCFLAG_FRESTRICTED,
      {VT_VOID, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {VT_PTR, -1, PARAMFLAG_FOUT},
        {-1, 0, 0}
      },
      { /* names */
        "GetTypeInfoCount",
        "pctinfo",
        NULL,
      },
    },
    {
      /*id*/ 0x60010001, /*func*/ FUNC_DISPATCH, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 3, /*#opt*/ 0, /*vtbl*/ 4, /*#scodes*/ 0, /*flags*/ FUNCFLAG_FRESTRICTED,
      {VT_VOID, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {VT_UINT, -1, PARAMFLAG_FIN},
        {VT_UI4, -1, PARAMFLAG_FIN},
        {VT_PTR, -1, PARAMFLAG_FOUT},
        {-1, 0, 0}
      },
      { /* names */
        "GetTypeInfo",
        "itinfo",
        "lcid",
        "pptinfo",
        NULL,
      },
    },
    {
      /*id*/ 0x60010002, /*func*/ FUNC_DISPATCH, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 5, /*#opt*/ 0, /*vtbl*/ 5, /*#scodes*/ 0, /*flags*/ FUNCFLAG_FRESTRICTED,
      {VT_VOID, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {VT_PTR, -1, PARAMFLAG_FIN},
        {VT_PTR, -1, PARAMFLAG_FIN},
        {VT_UINT, -1, PARAMFLAG_FIN},
        {VT_UI4, -1, PARAMFLAG_FIN},
        {VT_PTR, -1, PARAMFLAG_FOUT},
        {-1, 0, 0}
      },
      { /* names */
        "GetIDsOfNames",
        "riid",
        "rgszNames",
        "cNames",
        "lcid",
        "rgdispid",
        NULL,
      },
    },
    {
      /*id*/ 0x60010003, /*func*/ FUNC_DISPATCH, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 8, /*#opt*/ 0, /*vtbl*/ 6, /*#scodes*/ 0, /*flags*/ FUNCFLAG_FRESTRICTED,
      {VT_VOID, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {VT_I4, -1, PARAMFLAG_FIN},
        {VT_PTR, -1, PARAMFLAG_FIN},
        {VT_UI4, -1, PARAMFLAG_FIN},
        {VT_UI2, -1, PARAMFLAG_FIN},
        {VT_PTR, -1, PARAMFLAG_FIN},
        {VT_PTR, -1, PARAMFLAG_FOUT},
        {VT_PTR, -1, PARAMFLAG_FOUT},
        {VT_PTR, -1, PARAMFLAG_FOUT},
        {-1, 0, 0}
      },
      { /* names */
        "Invoke",
        "dispidMember",
        "riid",
        "lcid",
        "wFlags",
        "pdispparams",
        "pvarResult",
        "pexcepinfo",
        "puArgErr",
        NULL,
      },
    },
    {
      /*id*/ 0x60020000, /*func*/ FUNC_DISPATCH, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 0, /*#opt*/ 0, /*vtbl*/ 7, /*#scodes*/ 0, /*flags*/ 0,
      {VT_VOID, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {-1, 0, 0}
      },
      { /* names */
        "Test",
        NULL,
      },
    },
  },
  { /* vars */ },
},
{
  "IDualIface",
  "{b14b6bb5-904e-4ff9-b247-bd361f7aaedd}",
  /*kind*/ TKIND_INTERFACE, /*flags*/ TYPEFLAG_FDISPATCHABLE|TYPEFLAG_FOLEAUTOMATION|TYPEFLAG_FDUAL, /*align*/ TYPE_ALIGNMENT(IDualIface*), /*size*/ sizeof(IDualIface*),
  /*helpctx*/ 0x0000, /*version*/ 0x00000000, /*#vtbl*/ 8, /*#func*/ 1, /*#var*/ 0,
  /*#custdata*/ 0, {},
  { /* funcs */
    {
      /*id*/ 0x60020000, /*func*/ FUNC_PUREVIRTUAL, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 0, /*#opt*/ 0, /*vtbl*/ 7, /*#scodes*/ 0, /*flags*/ 0,
      {VT_HRESULT, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {-1, 0, 0}
      },
      { /* names */
        "Test",
        NULL,
      },
    },
  },
  { /* vars */ },
},
{
  "ISimpleIface",
  "{ec5dfcd6-eeb0-4cd6-b51e-8030e1dac009}",
  /*kind*/ TKIND_INTERFACE, /*flags*/ TYPEFLAG_FDISPATCHABLE, /*align*/ TYPE_ALIGNMENT(ISimpleIface*), /*size*/ sizeof(ISimpleIface*),
  /*helpctx*/ 0x0000, /*version*/ 0x00000000, /*#vtbl*/ 8, /*#func*/ 1, /*#var*/ 0,
  /*#custdata*/ 0, {},
  { /* funcs */
    {
      /*id*/ 0x60020000, /*func*/ FUNC_PUREVIRTUAL, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 0, /*#opt*/ 0, /*vtbl*/ 7, /*#scodes*/ 0, /*flags*/ 0,
      {VT_HRESULT, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {-1, 0, 0}
      },
      { /* names */
        "Test",
        NULL,
      },
    },
  },
  { /* vars */ },
},
{
  "test_struct",
  "{4029f190-ca4a-4611-aeb9-673983cb96dd}",
  /*kind*/ TKIND_RECORD, /*flags*/ 0, /*align*/ TYPE_ALIGNMENT(struct test_struct), /*size*/ sizeof(struct test_struct),
  /*helpctx*/ 0x0000, /*version*/ 0x00000000, /*#vtbl*/ 0, /*#func*/ 0, /*#var*/ 4,
  /*#custdata*/ 0, {},
  { /* funcs */ },
  { /* vars */
    {
      /*id*/ 0x40000000, /*name*/ "hr", /*flags*/ 0, /*kind*/ VAR_PERINSTANCE,
      { .oInst = 0 },
      /*#custdata*/ 0, {},
      {VT_HRESULT, -1, PARAMFLAG_NONE}, /* ret */
    },
    {
      /*id*/ 0x40000001, /*name*/ "b", /*flags*/ 0, /*kind*/ VAR_PERINSTANCE,
      { .oInst = 4 },
      /*#custdata*/ 0, {},
      {VT_BOOL, -1, PARAMFLAG_NONE}, /* ret */
    },
    {
      /*id*/ 0x40000002, /*name*/ "disp", /*flags*/ 0, /*kind*/ VAR_PERINSTANCE,
      { .oInst = 8 },
      /*#custdata*/ 0, {},
      {VT_DISPATCH, -1, PARAMFLAG_NONE}, /* ret */
    },
    {
      /*id*/ 0x40000003, /*name*/ "bstr", /*flags*/ 0, /*kind*/ VAR_PERINSTANCE,
      { .oInst = 12 },
      /*#custdata*/ 0, {},
      {VT_BSTR, -1, PARAMFLAG_NONE}, /* ret */
    },
  },
},
{
  "test_struct2",
  "{4029f190-ca4a-4611-aeb9-673983cb96de}",
  /*kind*/ TKIND_RECORD, /*flags*/ 0, /*align*/ TYPE_ALIGNMENT(struct test_struct2), /*size*/ sizeof(struct test_struct2),
  /*helpctx*/ 0x0000, /*version*/ 0x00000000, /*#vtbl*/ 0, /*#func*/ 0, /*#var*/ 4,
  /*#custdata*/ 0, {},
  { /* funcs */ },
  { /* vars */
    {
      /*id*/ 0x40000000, /*name*/ "hr", /*flags*/ 0, /*kind*/ VAR_PERINSTANCE,
      { .oInst = 0 },
      /*#custdata*/ 0, {},
      {VT_HRESULT, -1, PARAMFLAG_NONE}, /* ret */
    },
    {
      /*id*/ 0x40000001, /*name*/ "b", /*flags*/ 0, /*kind*/ VAR_PERINSTANCE,
      { .oInst = 4 },
      /*#custdata*/ 0, {},
      {VT_BOOL, -1, PARAMFLAG_NONE}, /* ret */
    },
    {
      /*id*/ 0x40000002, /*name*/ "disp", /*flags*/ 0, /*kind*/ VAR_PERINSTANCE,
      { .oInst = 8 },
      /*#custdata*/ 0, {},
      {VT_DISPATCH, -1, PARAMFLAG_NONE}, /* ret */
    },
    {
      /*id*/ 0x40000003, /*name*/ "bstr", /*flags*/ 0, /*kind*/ VAR_PERINSTANCE,
      { .oInst = 12 },
      /*#custdata*/ 0, {},
      {VT_BSTR, -1, PARAMFLAG_NONE}, /* ret */
    },
  },
},
{
  "t_INT",
  "{016fe2ec-b2c8-45f8-b23b-39e53a75396a}",
  /*kind*/ TKIND_ALIAS, /*flags*/ TYPEFLAG_FRESTRICTED, /*align*/ TYPE_ALIGNMENT(t_INT), /*size*/ sizeof(t_INT),
  /*helpctx*/ 0x0000, /*version*/ 0x00000000, /*#vtbl*/ 0, /*#func*/ 0, /*#var*/ 0,
  /*#custdata*/ 0, {},
  { /* funcs */ },
  { /* vars */ },
},
{
  "a",
  "{00000000-0000-0000-0000-000000000000}",
  /*kind*/ TKIND_ALIAS, /*flags*/ 0, /*align*/ TYPE_ALIGNMENT(a), /*size*/ sizeof(a),
  /*helpctx*/ 0x0000, /*version*/ 0x00000000, /*#vtbl*/ 0, /*#func*/ 0, /*#var*/ 0,
  /*#custdata*/ 0, {},
  { /* funcs */ },
  { /* vars */ },
},
{
  "_a",
  "{00000000-0000-0000-0000-000000000000}",
  /*kind*/ TKIND_ENUM, /*flags*/ 0, /*align*/ 4, /*size*/ 4,
  /*helpctx*/ 0x0000, /*version*/ 0x00000000, /*#vtbl*/ 0, /*#func*/ 0, /*#var*/ 2,
  /*#custdata*/ 0, {},
  { /* funcs */ },
  { /* vars */
    {
      /*id*/ 0x40000000, /*name*/ "a1", /*flags*/ 0, /*kind*/ VAR_CONST,
      { .varValue = { VT_I4, { .value_int = 0 } } },
      /*#custdata*/ 0, {},
      {VT_INT, -1, PARAMFLAG_NONE}, /* ret */
    },
    {
      /*id*/ 0x40000001, /*name*/ "a2", /*flags*/ 0, /*kind*/ VAR_CONST,
      { .varValue = { VT_I4, { .value_int = 1 } } },
      /*#custdata*/ 0, {},
      {VT_INT, -1, PARAMFLAG_NONE}, /* ret */
    },
  },
},
{
  "aa",
  "{00000000-0000-0000-0000-000000000000}",
  /*kind*/ TKIND_ENUM, /*flags*/ 0, /*align*/ 4, /*size*/ 4,
  /*helpctx*/ 0x0000, /*version*/ 0x00000000, /*#vtbl*/ 0, /*#func*/ 0, /*#var*/ 2,
  /*#custdata*/ 0, {},
  { /* funcs */ },
  { /* vars */
    {
      /*id*/ 0x40000000, /*name*/ "aa1", /*flags*/ 0, /*kind*/ VAR_CONST,
      { .varValue = { VT_I4, { .value_int = 0 } } },
      /*#custdata*/ 0, {},
      {VT_INT, -1, PARAMFLAG_NONE}, /* ret */
    },
    {
      /*id*/ 0x40000001, /*name*/ "aa2", /*flags*/ 0, /*kind*/ VAR_CONST,
      { .varValue = { VT_I4, { .value_int = 1 } } },
      /*#custdata*/ 0, {},
      {VT_INT, -1, PARAMFLAG_NONE}, /* ret */
    },
  },
},
{
  "_b",
  "{00000000-0000-0000-0000-000000000000}",
  /*kind*/ TKIND_ENUM, /*flags*/ 0, /*align*/ 4, /*size*/ 4,
  /*helpctx*/ 0x0000, /*version*/ 0x00000000, /*#vtbl*/ 0, /*#func*/ 0, /*#var*/ 2,
  /*#custdata*/ 0, {},
  { /* funcs */ },
  { /* vars */
    {
      /*id*/ 0x40000000, /*name*/ "b1", /*flags*/ 0, /*kind*/ VAR_CONST,
      { .varValue = { VT_I4, { .value_int = 0 } } },
      /*#custdata*/ 0, {},
      {VT_INT, -1, PARAMFLAG_NONE}, /* ret */
    },
    {
      /*id*/ 0x40000001, /*name*/ "b2", /*flags*/ 0, /*kind*/ VAR_CONST,
      { .varValue = { VT_I4, { .value_int = 1 } } },
      /*#custdata*/ 0, {},
      {VT_INT, -1, PARAMFLAG_NONE}, /* ret */
    },
  },
},
{
  "bb",
  "{00000000-0000-0000-0000-000000000000}",
  /*kind*/ TKIND_ENUM, /*flags*/ 0, /*align*/ 4, /*size*/ 4,
  /*helpctx*/ 0x0000, /*version*/ 0x00000000, /*#vtbl*/ 0, /*#func*/ 0, /*#var*/ 2,
  /*#custdata*/ 0, {},
  { /* funcs */ },
  { /* vars */
    {
      /*id*/ 0x40000000, /*name*/ "bb1", /*flags*/ 0, /*kind*/ VAR_CONST,
      { .varValue = { VT_I4, { .value_int = 0 } } },
      /*#custdata*/ 0, {},
      {VT_INT, -1, PARAMFLAG_NONE}, /* ret */
    },
    {
      /*id*/ 0x40000001, /*name*/ "bb2", /*flags*/ 0, /*kind*/ VAR_CONST,
      { .varValue = { VT_I4, { .value_int = 1 } } },
      /*#custdata*/ 0, {},
      {VT_INT, -1, PARAMFLAG_NONE}, /* ret */
    },
  },
},
{
  "c",
  "{016fe2ec-b2c8-45f8-b23b-39e53a75396b}",
  /*kind*/ TKIND_ALIAS, /*flags*/ 0, /*align*/ TYPE_ALIGNMENT(c), /*size*/ sizeof(c),
  /*helpctx*/ 0x0000, /*version*/ 0x00000000, /*#vtbl*/ 0, /*#func*/ 0, /*#var*/ 0,
  /*#custdata*/ 0, {},
  { /* funcs */ },
  { /* vars */ },
},
{
  "_c",
  "{00000000-0000-0000-0000-000000000000}",
  /*kind*/ TKIND_ENUM, /*flags*/ 0, /*align*/ 4, /*size*/ 4,
  /*helpctx*/ 0x0000, /*version*/ 0x00000000, /*#vtbl*/ 0, /*#func*/ 0, /*#var*/ 2,
  /*#custdata*/ 0, {},
  { /* funcs */ },
  { /* vars */
    {
      /*id*/ 0x40000000, /*name*/ "c1", /*flags*/ 0, /*kind*/ VAR_CONST,
      { .varValue = { VT_I4, { .value_int = 0 } } },
      /*#custdata*/ 0, {},
      {VT_INT, -1, PARAMFLAG_NONE}, /* ret */
    },
    {
      /*id*/ 0x40000001, /*name*/ "c2", /*flags*/ 0, /*kind*/ VAR_CONST,
      { .varValue = { VT_I4, { .value_int = 1 } } },
      /*#custdata*/ 0, {},
      {VT_INT, -1, PARAMFLAG_NONE}, /* ret */
    },
  },
},
{
  "cc",
  "{016fe2ec-b2c8-45f8-b23b-39e53a75396c}",
  /*kind*/ TKIND_ENUM, /*flags*/ 0, /*align*/ 4, /*size*/ 4,
  /*helpctx*/ 0x0000, /*version*/ 0x00000000, /*#vtbl*/ 0, /*#func*/ 0, /*#var*/ 2,
  /*#custdata*/ 0, {},
  { /* funcs */ },
  { /* vars */
    {
      /*id*/ 0x40000000, /*name*/ "cc1", /*flags*/ 0, /*kind*/ VAR_CONST,
      { .varValue = { VT_I4, { .value_int = 0 } } },
      /*#custdata*/ 0, {},
      {VT_INT, -1, PARAMFLAG_NONE}, /* ret */
    },
    {
      /*id*/ 0x40000001, /*name*/ "cc2", /*flags*/ 0, /*kind*/ VAR_CONST,
      { .varValue = { VT_I4, { .value_int = 1 } } },
      /*#custdata*/ 0, {},
      {VT_INT, -1, PARAMFLAG_NONE}, /* ret */
    },
  },
},
{
  "d",
  "{016fe2ec-b2c8-45f8-b23b-39e53a75396d}",
  /*kind*/ TKIND_ALIAS, /*flags*/ TYPEFLAG_FRESTRICTED|TYPEFLAG_FHIDDEN, /*align*/ TYPE_ALIGNMENT(d), /*size*/ sizeof(d),
  /*helpctx*/ 0x0000, /*version*/ 0x00000000, /*#vtbl*/ 0, /*#func*/ 0, /*#var*/ 0,
  /*#custdata*/ 0, {},
  { /* funcs */ },
  { /* vars */ },
},
{
  "_d",
  "{00000000-0000-0000-0000-000000000000}",
  /*kind*/ TKIND_ENUM, /*flags*/ TYPEFLAG_FRESTRICTED|TYPEFLAG_FHIDDEN, /*align*/ 4, /*size*/ 4,
  /*helpctx*/ 0x0000, /*version*/ 0x00000000, /*#vtbl*/ 0, /*#func*/ 0, /*#var*/ 2,
  /*#custdata*/ 0, {},
  { /* funcs */ },
  { /* vars */
    {
      /*id*/ 0x40000000, /*name*/ "d1", /*flags*/ 0, /*kind*/ VAR_CONST,
      { .varValue = { VT_I4, { .value_int = 0 } } },
      /*#custdata*/ 0, {},
      {VT_INT, -1, PARAMFLAG_NONE}, /* ret */
    },
    {
      /*id*/ 0x40000001, /*name*/ "d2", /*flags*/ 0, /*kind*/ VAR_CONST,
      { .varValue = { VT_I4, { .value_int = 1 } } },
      /*#custdata*/ 0, {},
      {VT_INT, -1, PARAMFLAG_NONE}, /* ret */
    },
  },
},
{
  "dd",
  "{016fe2ec-b2c8-45f8-b23b-39e53a75396e}",
  /*kind*/ TKIND_ENUM, /*flags*/ TYPEFLAG_FRESTRICTED|TYPEFLAG_FHIDDEN, /*align*/ 4, /*size*/ 4,
  /*helpctx*/ 0x0000, /*version*/ 0x00000000, /*#vtbl*/ 0, /*#func*/ 0, /*#var*/ 2,
  /*#custdata*/ 0, {},
  { /* funcs */ },
  { /* vars */
    {
      /*id*/ 0x40000000, /*name*/ "dd1", /*flags*/ 0, /*kind*/ VAR_CONST,
      { .varValue = { VT_I4, { .value_int = 0 } } },
      /*#custdata*/ 0, {},
      {VT_INT, -1, PARAMFLAG_NONE}, /* ret */
    },
    {
      /*id*/ 0x40000001, /*name*/ "dd2", /*flags*/ 0, /*kind*/ VAR_CONST,
      { .varValue = { VT_I4, { .value_int = 1 } } },
      /*#custdata*/ 0, {},
      {VT_INT, -1, PARAMFLAG_NONE}, /* ret */
    },
  },
},
{
  "e",
  "{016fe2ec-b2c8-45f8-b23b-39e53a753970}",
  /*kind*/ TKIND_ALIAS, /*flags*/ TYPEFLAG_FRESTRICTED|TYPEFLAG_FHIDDEN, /*align*/ TYPE_ALIGNMENT(e), /*size*/ sizeof(e),
  /*helpctx*/ 0x0000, /*version*/ 0x00000000, /*#vtbl*/ 0, /*#func*/ 0, /*#var*/ 0,
  /*#custdata*/ 0, {},
  { /* funcs */ },
  { /* vars */ },
},
{
  "_e",
  "{00000000-0000-0000-0000-000000000000}",
  /*kind*/ TKIND_RECORD, /*flags*/ TYPEFLAG_FRESTRICTED|TYPEFLAG_FHIDDEN, /*align*/ TYPE_ALIGNMENT(struct _e), /*size*/ sizeof(struct _e),
  /*helpctx*/ 0x0000, /*version*/ 0x00000000, /*#vtbl*/ 0, /*#func*/ 0, /*#var*/ 1,
  /*#custdata*/ 0, {},
  { /* funcs */ },
  { /* vars */
    {
      /*id*/ 0x40000000, /*name*/ "e1", /*flags*/ 0, /*kind*/ VAR_PERINSTANCE,
      { .oInst = 0 },
      /*#custdata*/ 0, {},
      {VT_INT, -1, PARAMFLAG_NONE}, /* ret */
    },
  },
},
{
  "ee",
  "{016fe2ec-b2c8-45f8-b23b-39e53a753971}",
  /*kind*/ TKIND_RECORD, /*flags*/ TYPEFLAG_FRESTRICTED|TYPEFLAG_FHIDDEN, /*align*/ TYPE_ALIGNMENT(struct ee), /*size*/ sizeof(struct ee),
  /*helpctx*/ 0x0000, /*version*/ 0x00000000, /*#vtbl*/ 0, /*#func*/ 0, /*#var*/ 1,
  /*#custdata*/ 0, {},
  { /* funcs */ },
  { /* vars */
    {
      /*id*/ 0x40000000, /*name*/ "ee1", /*flags*/ 0, /*kind*/ VAR_PERINSTANCE,
      { .oInst = 0 },
      /*#custdata*/ 0, {},
      {VT_INT, -1, PARAMFLAG_NONE}, /* ret */
    },
  },
},
{
  "f",
  "{016fe2ec-b2c8-45f8-b23b-39e53a753972}",
  /*kind*/ TKIND_ALIAS, /*flags*/ TYPEFLAG_FRESTRICTED|TYPEFLAG_FHIDDEN, /*align*/ TYPE_ALIGNMENT(f), /*size*/ sizeof(f),
  /*helpctx*/ 0x0000, /*version*/ 0x00000000, /*#vtbl*/ 0, /*#func*/ 0, /*#var*/ 0,
  /*#custdata*/ 0, {},
  { /* funcs */ },
  { /* vars */ },
},
{
  "_f",
  "{00000000-0000-0000-0000-000000000000}",
  /*kind*/ TKIND_UNION, /*flags*/ TYPEFLAG_FRESTRICTED|TYPEFLAG_FHIDDEN, /*align*/ TYPE_ALIGNMENT(union _f), /*size*/ sizeof(union _f),
  /*helpctx*/ 0x0000, /*version*/ 0x00000000, /*#vtbl*/ 0, /*#func*/ 0, /*#var*/ 2,
  /*#custdata*/ 0, {},
  { /* funcs */ },
  { /* vars */
    {
      /*id*/ 0x40000000, /*name*/ "f1", /*flags*/ 0, /*kind*/ VAR_PERINSTANCE,
      { .oInst = 0 },
      /*#custdata*/ 0, {},
      {VT_INT, -1, PARAMFLAG_NONE}, /* ret */
    },
    {
      /*id*/ 0x40000001, /*name*/ "f2", /*flags*/ 0, /*kind*/ VAR_PERINSTANCE,
      { .oInst = 0 },
      /*#custdata*/ 0, {},
      {VT_PTR, -1, PARAMFLAG_NONE}, /* ret */
    },
  },
},
{
  "ff",
  "{016fe2ec-b2c8-45f8-b23b-39e53a753973}",
  /*kind*/ TKIND_UNION, /*flags*/ TYPEFLAG_FRESTRICTED|TYPEFLAG_FHIDDEN, /*align*/ TYPE_ALIGNMENT(union ff), /*size*/ sizeof(union ff),
  /*helpctx*/ 0x0000, /*version*/ 0x00000000, /*#vtbl*/ 0, /*#func*/ 0, /*#var*/ 2,
  /*#custdata*/ 0, {},
  { /* funcs */ },
  { /* vars */
    {
      /*id*/ 0x40000000, /*name*/ "ff1", /*flags*/ 0, /*kind*/ VAR_PERINSTANCE,
      { .oInst = 0 },
      /*#custdata*/ 0, {},
      {VT_INT, -1, PARAMFLAG_NONE}, /* ret */
    },
    {
      /*id*/ 0x40000001, /*name*/ "ff2", /*flags*/ 0, /*kind*/ VAR_PERINSTANCE,
      { .oInst = 0 },
      /*#custdata*/ 0, {},
      {VT_PTR, -1, PARAMFLAG_NONE}, /* ret */
    },
  },
},
{
  "ITestIface",
  "{ec5dfcd6-eeb0-4cd6-b51e-8030e1dac00a}",
  /*kind*/ TKIND_INTERFACE, /*flags*/ TYPEFLAG_FDISPATCHABLE, /*align*/ TYPE_ALIGNMENT(ITestIface*), /*size*/ sizeof(ITestIface*),
  /*helpctx*/ 0x0000, /*version*/ 0x00000000, /*#vtbl*/ 13, /*#func*/ 6, /*#var*/ 0,
  /*#custdata*/ 0, {},
  { /* funcs */
    {
      /*id*/ 0x60020000, /*func*/ FUNC_PUREVIRTUAL, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 1, /*#opt*/ 0, /*vtbl*/ 7, /*#scodes*/ 0, /*flags*/ 0,
      {VT_HRESULT, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {VT_USERDEFINED, TKIND_ALIAS, PARAMFLAG_NONE},
        {-1, 0, 0}
      },
      { /* names */
        "test1",
        "value",
        NULL,
      },
    },
    {
      /*id*/ 0x60020001, /*func*/ FUNC_PUREVIRTUAL, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 1, /*#opt*/ 0, /*vtbl*/ 8, /*#scodes*/ 0, /*flags*/ 0,
      {VT_HRESULT, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {VT_USERDEFINED, TKIND_ENUM, PARAMFLAG_NONE},
        {-1, 0, 0}
      },
      { /* names */
        "test2",
        "value",
        NULL,
      },
    },
    {
      /*id*/ 0x60020002, /*func*/ FUNC_PUREVIRTUAL, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 1, /*#opt*/ 0, /*vtbl*/ 9, /*#scodes*/ 0, /*flags*/ 0,
      {VT_HRESULT, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {VT_USERDEFINED, TKIND_ALIAS, PARAMFLAG_NONE},
        {-1, 0, 0}
      },
      { /* names */
        "test3",
        "value",
        NULL,
      },
    },
    {
      /*id*/ 0x60020003, /*func*/ FUNC_PUREVIRTUAL, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 1, /*#opt*/ 0, /*vtbl*/ 10, /*#scodes*/ 0, /*flags*/ 0,
      {VT_HRESULT, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {VT_USERDEFINED, TKIND_ALIAS, PARAMFLAG_NONE},
        {-1, 0, 0}
      },
      { /* names */
        "test4",
        "value",
        NULL,
      },
    },
    {
      /*id*/ 0x60020004, /*func*/ FUNC_PUREVIRTUAL, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 1, /*#opt*/ 0, /*vtbl*/ 11, /*#scodes*/ 0, /*flags*/ 0,
      {VT_HRESULT, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {VT_USERDEFINED, TKIND_ALIAS, PARAMFLAG_NONE},
        {-1, 0, 0}
      },
      { /* names */
        "test5",
        "value",
        NULL,
      },
    },
    {
      /*id*/ 0x60020005, /*func*/ FUNC_PUREVIRTUAL, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 1, /*#opt*/ 0, /*vtbl*/ 12, /*#scodes*/ 0, /*flags*/ 0,
      {VT_HRESULT, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {VT_USERDEFINED, TKIND_ALIAS, PARAMFLAG_NONE},
        {-1, 0, 0}
      },
      { /* names */
        "test6",
        "value",
        NULL,
      },
    },
  },
  { /* vars */ },
},
{
  "ITestDispatch",
  "{2d4430d5-99ea-4645-85f0-c5814b72804b}",
  /*kind*/ TKIND_DISPATCH, /*flags*/ TYPEFLAG_FDISPATCHABLE, /*align*/ TYPE_ALIGNMENT(ITestDispatch*), /*size*/ sizeof(ITestDispatch*),
  /*helpctx*/ 0x0000, /*version*/ 0x00000000, /*#vtbl*/ 7, /*#func*/ 7, /*#var*/ 2,
  /*#custdata*/ 0, {},
  { /* funcs */
    {
      /*id*/ 0x1, /*func*/ FUNC_DISPATCH, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 0, /*#opt*/ 0, /*vtbl*/ 0, /*#scodes*/ 0, /*flags*/ 0,
      {VT_VOID, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {-1, 0, 0}
      },
      { /* names */
        "test_void",
        NULL,
      },
    },
    {
      /*id*/ 0x2, /*func*/ FUNC_DISPATCH, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 1, /*#opt*/ 0, /*vtbl*/ 0, /*#scodes*/ 0, /*flags*/ 0,
      {VT_VOID, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {VT_PTR, -1, PARAMFLAG_FOUT|PARAMFLAG_FRETVAL},
        {-1, 0, 0}
      },
      { /* names */
        "test_void_retval",
        "ret",
        NULL,
      },
    },
    {
      /*id*/ 0x3, /*func*/ FUNC_DISPATCH, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 0, /*#opt*/ 0, /*vtbl*/ 0, /*#scodes*/ 0, /*flags*/ 0,
      {VT_HRESULT, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {-1, 0, 0}
      },
      { /* names */
        "test_HRESULT",
        NULL,
      },
    },
    {
      /*id*/ 0x4, /*func*/ FUNC_DISPATCH, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 1, /*#opt*/ 0, /*vtbl*/ 0, /*#scodes*/ 0, /*flags*/ 0,
      {VT_HRESULT, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {VT_PTR, -1, PARAMFLAG_FOUT|PARAMFLAG_FRETVAL},
        {-1, 0, 0}
      },
      { /* names */
        "test_HRESULT_retval",
        "ret",
        NULL,
      },
    },
    {
      /*id*/ 0x5, /*func*/ FUNC_DISPATCH, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 0, /*#opt*/ 0, /*vtbl*/ 0, /*#scodes*/ 0, /*flags*/ 0,
      {VT_INT, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {-1, 0, 0}
      },
      { /* names */
        "test_int",
        NULL,
      },
    },
    {
      /*id*/ 0x6, /*func*/ FUNC_DISPATCH, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 1, /*#opt*/ 0, /*vtbl*/ 0, /*#scodes*/ 0, /*flags*/ 0,
      {VT_INT, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {VT_PTR, -1, PARAMFLAG_FOUT|PARAMFLAG_FRETVAL},
        {-1, 0, 0}
      },
      { /* names */
        "test_int_retval",
        "ret",
        NULL,
      },
    },
    {
      /*id*/ 0x7, /*func*/ FUNC_DISPATCH, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 2, /*#opt*/ 0, /*vtbl*/ 0, /*#scodes*/ 0, /*flags*/ 0,
      {VT_R8, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {VT_BSTR, -1, PARAMFLAG_FIN},
        {VT_I4, -1, PARAMFLAG_FLCID},
        {-1, 0, 0}
      },
      { /* names */
        "parse_lcid",
        "x",
        "lcid",
        NULL,
      },
    },
  },
  { /* vars */
    {
      /*id*/ 0xa, /*name*/ "property_int", /*flags*/ 0, /*kind*/ VAR_DISPATCH,
      { /* DUMMYUNIONNAME unused*/ },
      /*#custdata*/ 0, {},
      {VT_INT, -1, PARAMFLAG_NONE}, /* ret */
    },
    {
      /*id*/ 0xb, /*name*/ "property_HRESULT", /*flags*/ 0, /*kind*/ VAR_DISPATCH,
      { /* DUMMYUNIONNAME unused*/ },
      /*#custdata*/ 0, {},
      {VT_HRESULT, -1, PARAMFLAG_NONE}, /* ret */
    },
  },
},
{
  "ITestDispDual",
  "{79ca07f9-ac22-44ac-9aaf-811f45412293}",
  /*kind*/ TKIND_DISPATCH, /*flags*/ TYPEFLAG_FDISPATCHABLE|TYPEFLAG_FDUAL, /*align*/ TYPE_ALIGNMENT(ITestDispDual*), /*size*/ sizeof(ITestDispDual*),
  /*helpctx*/ 0x0000, /*version*/ 0x00000000, /*#vtbl*/ 7, /*#func*/ 14, /*#var*/ 0,
  /*#custdata*/ 0, {},
  { /* funcs */
    {
      /*id*/ 0x60000000, /*func*/ FUNC_DISPATCH, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 2, /*#opt*/ 0, /*vtbl*/ 0, /*#scodes*/ 0, /*flags*/ FUNCFLAG_FRESTRICTED,
      {VT_VOID, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {VT_PTR, -1, PARAMFLAG_FIN},
        {VT_PTR, -1, PARAMFLAG_FOUT},
        {-1, 0, 0}
      },
      { /* names */
        "QueryInterface",
        "riid",
        "ppvObj",
        NULL,
      },
    },
    {
      /*id*/ 0x60000001, /*func*/ FUNC_DISPATCH, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 0, /*#opt*/ 0, /*vtbl*/ 1, /*#scodes*/ 0, /*flags*/ FUNCFLAG_FRESTRICTED,
      {VT_UI4, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {-1, 0, 0}
      },
      { /* names */
        "AddRef",
        NULL,
      },
    },
    {
      /*id*/ 0x60000002, /*func*/ FUNC_DISPATCH, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 0, /*#opt*/ 0, /*vtbl*/ 2, /*#scodes*/ 0, /*flags*/ FUNCFLAG_FRESTRICTED,
      {VT_UI4, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {-1, 0, 0}
      },
      { /* names */
        "Release",
        NULL,
      },
    },
    {
      /*id*/ 0x60010000, /*func*/ FUNC_DISPATCH, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 1, /*#opt*/ 0, /*vtbl*/ 3, /*#scodes*/ 0, /*flags*/ FUNCFLAG_FRESTRICTED,
      {VT_VOID, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {VT_PTR, -1, PARAMFLAG_FOUT},
        {-1, 0, 0}
      },
      { /* names */
        "GetTypeInfoCount",
        "pctinfo",
        NULL,
      },
    },
    {
      /*id*/ 0x60010001, /*func*/ FUNC_DISPATCH, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 3, /*#opt*/ 0, /*vtbl*/ 4, /*#scodes*/ 0, /*flags*/ FUNCFLAG_FRESTRICTED,
      {VT_VOID, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {VT_UINT, -1, PARAMFLAG_FIN},
        {VT_UI4, -1, PARAMFLAG_FIN},
        {VT_PTR, -1, PARAMFLAG_FOUT},
        {-1, 0, 0}
      },
      { /* names */
        "GetTypeInfo",
        "itinfo",
        "lcid",
        "pptinfo",
        NULL,
      },
    },
    {
      /*id*/ 0x60010002, /*func*/ FUNC_DISPATCH, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 5, /*#opt*/ 0, /*vtbl*/ 5, /*#scodes*/ 0, /*flags*/ FUNCFLAG_FRESTRICTED,
      {VT_VOID, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {VT_PTR, -1, PARAMFLAG_FIN},
        {VT_PTR, -1, PARAMFLAG_FIN},
        {VT_UINT, -1, PARAMFLAG_FIN},
        {VT_UI4, -1, PARAMFLAG_FIN},
        {VT_PTR, -1, PARAMFLAG_FOUT},
        {-1, 0, 0}
      },
      { /* names */
        "GetIDsOfNames",
        "riid",
        "rgszNames",
        "cNames",
        "lcid",
        "rgdispid",
        NULL,
      },
    },
    {
      /*id*/ 0x60010003, /*func*/ FUNC_DISPATCH, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 8, /*#opt*/ 0, /*vtbl*/ 6, /*#scodes*/ 0, /*flags*/ FUNCFLAG_FRESTRICTED,
      {VT_VOID, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {VT_I4, -1, PARAMFLAG_FIN},
        {VT_PTR, -1, PARAMFLAG_FIN},
        {VT_UI4, -1, PARAMFLAG_FIN},
        {VT_UI2, -1, PARAMFLAG_FIN},
        {VT_PTR, -1, PARAMFLAG_FIN},
        {VT_PTR, -1, PARAMFLAG_FOUT},
        {VT_PTR, -1, PARAMFLAG_FOUT},
        {VT_PTR, -1, PARAMFLAG_FOUT},
        {-1, 0, 0}
      },
      { /* names */
        "Invoke",
        "dispidMember",
        "riid",
        "lcid",
        "wFlags",
        "pdispparams",
        "pvarResult",
        "pexcepinfo",
        "puArgErr",
        NULL,
      },
    },
    {
      /*id*/ 0x1, /*func*/ FUNC_DISPATCH, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 0, /*#opt*/ 0, /*vtbl*/ 7, /*#scodes*/ 0, /*flags*/ 0,
      {VT_VOID, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {-1, 0, 0}
      },
      { /* names */
        "test_void",
        NULL,
      },
    },
    {
      /*id*/ 0x2, /*func*/ FUNC_DISPATCH, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 0, /*#opt*/ 0, /*vtbl*/ 8, /*#scodes*/ 0, /*flags*/ 0,
      {VT_R8, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {-1, 0, 0}
      },
      { /* names */
        "test_void_retval",
        NULL,
      },
    },
    {
      /*id*/ 0x3, /*func*/ FUNC_DISPATCH, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 0, /*#opt*/ 0, /*vtbl*/ 9, /*#scodes*/ 0, /*flags*/ 0,
      {VT_VOID, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {-1, 0, 0}
      },
      { /* names */
        "test_HRESULT",
        NULL,
      },
    },
    {
      /*id*/ 0x4, /*func*/ FUNC_DISPATCH, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 0, /*#opt*/ 0, /*vtbl*/ 10, /*#scodes*/ 0, /*flags*/ 0,
      {VT_R8, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {-1, 0, 0}
      },
      { /* names */
        "test_HRESULT_retval",
        NULL,
      },
    },
    {
      /*id*/ 0x5, /*func*/ FUNC_DISPATCH, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 0, /*#opt*/ 0, /*vtbl*/ 11, /*#scodes*/ 0, /*flags*/ 0,
      {VT_INT, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {-1, 0, 0}
      },
      { /* names */
        "test_int",
        NULL,
      },
    },
    {
      /*id*/ 0x6, /*func*/ FUNC_DISPATCH, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 0, /*#opt*/ 0, /*vtbl*/ 12, /*#scodes*/ 0, /*flags*/ 0,
      {VT_R8, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {-1, 0, 0}
      },
      { /* names */
        "test_int_retval",
        NULL,
      },
    },
    {
      /*id*/ 0x7, /*func*/ FUNC_DISPATCH, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 1, /*#opt*/ 0, /*vtbl*/ 13, /*#scodes*/ 0, /*flags*/ 0,
      {VT_R8, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {VT_BSTR, -1, PARAMFLAG_FIN},
        {-1, 0, 0}
      },
      { /* names */
        "parse_lcid",
        "x",
        NULL,
      },
    },
  },
  { /* vars */ },
},
{
  "ITestDispDual",
  "{79ca07f9-ac22-44ac-9aaf-811f45412293}",
  /*kind*/ TKIND_INTERFACE, /*flags*/ TYPEFLAG_FDISPATCHABLE|TYPEFLAG_FOLEAUTOMATION|TYPEFLAG_FDUAL, /*align*/ TYPE_ALIGNMENT(ITestDispDual*), /*size*/ sizeof(ITestDispDual*),
  /*helpctx*/ 0x0000, /*version*/ 0x00000000, /*#vtbl*/ 14, /*#func*/ 7, /*#var*/ 0,
  /*#custdata*/ 0, {},
  { /* funcs */
    {
      /*id*/ 0x1, /*func*/ FUNC_PUREVIRTUAL, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 0, /*#opt*/ 0, /*vtbl*/ 7, /*#scodes*/ 0, /*flags*/ 0,
      {VT_VOID, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {-1, 0, 0}
      },
      { /* names */
        "test_void",
        NULL,
      },
    },
    {
      /*id*/ 0x2, /*func*/ FUNC_PUREVIRTUAL, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 1, /*#opt*/ 0, /*vtbl*/ 8, /*#scodes*/ 0, /*flags*/ 0,
      {VT_VOID, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {VT_PTR, -1, PARAMFLAG_FOUT|PARAMFLAG_FRETVAL},
        {-1, 0, 0}
      },
      { /* names */
        "test_void_retval",
        "ret",
        NULL,
      },
    },
    {
      /*id*/ 0x3, /*func*/ FUNC_PUREVIRTUAL, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 0, /*#opt*/ 0, /*vtbl*/ 9, /*#scodes*/ 0, /*flags*/ 0,
      {VT_HRESULT, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {-1, 0, 0}
      },
      { /* names */
        "test_HRESULT",
        NULL,
      },
    },
    {
      /*id*/ 0x4, /*func*/ FUNC_PUREVIRTUAL, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 1, /*#opt*/ 0, /*vtbl*/ 10, /*#scodes*/ 0, /*flags*/ 0,
      {VT_HRESULT, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {VT_PTR, -1, PARAMFLAG_FOUT|PARAMFLAG_FRETVAL},
        {-1, 0, 0}
      },
      { /* names */
        "test_HRESULT_retval",
        "ret",
        NULL,
      },
    },
    {
      /*id*/ 0x5, /*func*/ FUNC_PUREVIRTUAL, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 0, /*#opt*/ 0, /*vtbl*/ 11, /*#scodes*/ 0, /*flags*/ 0,
      {VT_INT, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {-1, 0, 0}
      },
      { /* names */
        "test_int",
        NULL,
      },
    },
    {
      /*id*/ 0x6, /*func*/ FUNC_PUREVIRTUAL, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 1, /*#opt*/ 0, /*vtbl*/ 12, /*#scodes*/ 0, /*flags*/ 0,
      {VT_INT, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {VT_PTR, -1, PARAMFLAG_FOUT|PARAMFLAG_FRETVAL},
        {-1, 0, 0}
      },
      { /* names */
        "test_int_retval",
        "ret",
        NULL,
      },
    },
    {
      /*id*/ 0x7, /*func*/ FUNC_PUREVIRTUAL, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 3, /*#opt*/ 0, /*vtbl*/ 13, /*#scodes*/ 0, /*flags*/ 0,
      {VT_HRESULT, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {VT_BSTR, -1, PARAMFLAG_FIN},
        {VT_I4, -1, PARAMFLAG_FLCID},
        {VT_PTR, -1, PARAMFLAG_FOUT|PARAMFLAG_FRETVAL},
        {-1, 0, 0}
      },
      { /* names */
        "parse_lcid",
        "x",
        "lcid",
        "ret",
        NULL,
      },
    },
  },
  { /* vars */ },
},
{
  "ITestDispInherit",
  "{cdb105e3-24fb-4ae6-b826-801b7b2a0a07}",
  /*kind*/ TKIND_DISPATCH, /*flags*/ TYPEFLAG_FDISPATCHABLE, /*align*/ TYPE_ALIGNMENT(ITestDispInherit*), /*size*/ sizeof(ITestDispInherit*),
  /*helpctx*/ 0x0000, /*version*/ 0x00000000, /*#vtbl*/ 7, /*#func*/ 14, /*#var*/ 0,
  /*#custdata*/ 0, {},
  { /* funcs */
    {
      /*id*/ 0x60000000, /*func*/ FUNC_DISPATCH, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 2, /*#opt*/ 0, /*vtbl*/ 0, /*#scodes*/ 0, /*flags*/ FUNCFLAG_FRESTRICTED,
      {VT_VOID, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {VT_PTR, -1, PARAMFLAG_FIN},
        {VT_PTR, -1, PARAMFLAG_FOUT},
        {-1, 0, 0}
      },
      { /* names */
        "QueryInterface",
        "riid",
        "ppvObj",
        NULL,
      },
    },
    {
      /*id*/ 0x60000001, /*func*/ FUNC_DISPATCH, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 0, /*#opt*/ 0, /*vtbl*/ 1, /*#scodes*/ 0, /*flags*/ FUNCFLAG_FRESTRICTED,
      {VT_UI4, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {-1, 0, 0}
      },
      { /* names */
        "AddRef",
        NULL,
      },
    },
    {
      /*id*/ 0x60000002, /*func*/ FUNC_DISPATCH, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 0, /*#opt*/ 0, /*vtbl*/ 2, /*#scodes*/ 0, /*flags*/ FUNCFLAG_FRESTRICTED,
      {VT_UI4, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {-1, 0, 0}
      },
      { /* names */
        "Release",
        NULL,
      },
    },
    {
      /*id*/ 0x60010000, /*func*/ FUNC_DISPATCH, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 1, /*#opt*/ 0, /*vtbl*/ 3, /*#scodes*/ 0, /*flags*/ FUNCFLAG_FRESTRICTED,
      {VT_VOID, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {VT_PTR, -1, PARAMFLAG_FOUT},
        {-1, 0, 0}
      },
      { /* names */
        "GetTypeInfoCount",
        "pctinfo",
        NULL,
      },
    },
    {
      /*id*/ 0x60010001, /*func*/ FUNC_DISPATCH, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 3, /*#opt*/ 0, /*vtbl*/ 4, /*#scodes*/ 0, /*flags*/ FUNCFLAG_FRESTRICTED,
      {VT_VOID, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {VT_UINT, -1, PARAMFLAG_FIN},
        {VT_UI4, -1, PARAMFLAG_FIN},
        {VT_PTR, -1, PARAMFLAG_FOUT},
        {-1, 0, 0}
      },
      { /* names */
        "GetTypeInfo",
        "itinfo",
        "lcid",
        "pptinfo",
        NULL,
      },
    },
    {
      /*id*/ 0x60010002, /*func*/ FUNC_DISPATCH, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 5, /*#opt*/ 0, /*vtbl*/ 5, /*#scodes*/ 0, /*flags*/ FUNCFLAG_FRESTRICTED,
      {VT_VOID, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {VT_PTR, -1, PARAMFLAG_FIN},
        {VT_PTR, -1, PARAMFLAG_FIN},
        {VT_UINT, -1, PARAMFLAG_FIN},
        {VT_UI4, -1, PARAMFLAG_FIN},
        {VT_PTR, -1, PARAMFLAG_FOUT},
        {-1, 0, 0}
      },
      { /* names */
        "GetIDsOfNames",
        "riid",
        "rgszNames",
        "cNames",
        "lcid",
        "rgdispid",
        NULL,
      },
    },
    {
      /*id*/ 0x60010003, /*func*/ FUNC_DISPATCH, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 8, /*#opt*/ 0, /*vtbl*/ 6, /*#scodes*/ 0, /*flags*/ FUNCFLAG_FRESTRICTED,
      {VT_VOID, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {VT_I4, -1, PARAMFLAG_FIN},
        {VT_PTR, -1, PARAMFLAG_FIN},
        {VT_UI4, -1, PARAMFLAG_FIN},
        {VT_UI2, -1, PARAMFLAG_FIN},
        {VT_PTR, -1, PARAMFLAG_FIN},
        {VT_PTR, -1, PARAMFLAG_FOUT},
        {VT_PTR, -1, PARAMFLAG_FOUT},
        {VT_PTR, -1, PARAMFLAG_FOUT},
        {-1, 0, 0}
      },
      { /* names */
        "Invoke",
        "dispidMember",
        "riid",
        "lcid",
        "wFlags",
        "pdispparams",
        "pvarResult",
        "pexcepinfo",
        "puArgErr",
        NULL,
      },
    },
    {
      /*id*/ 0x1, /*func*/ FUNC_DISPATCH, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 0, /*#opt*/ 0, /*vtbl*/ 7, /*#scodes*/ 0, /*flags*/ 0,
      {VT_VOID, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {-1, 0, 0}
      },
      { /* names */
        "test_void",
        NULL,
      },
    },
    {
      /*id*/ 0x2, /*func*/ FUNC_DISPATCH, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 0, /*#opt*/ 0, /*vtbl*/ 8, /*#scodes*/ 0, /*flags*/ 0,
      {VT_R8, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {-1, 0, 0}
      },
      { /* names */
        "test_void_retval",
        NULL,
      },
    },
    {
      /*id*/ 0x3, /*func*/ FUNC_DISPATCH, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 0, /*#opt*/ 0, /*vtbl*/ 9, /*#scodes*/ 0, /*flags*/ 0,
      {VT_VOID, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {-1, 0, 0}
      },
      { /* names */
        "test_HRESULT",
        NULL,
      },
    },
    {
      /*id*/ 0x4, /*func*/ FUNC_DISPATCH, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 0, /*#opt*/ 0, /*vtbl*/ 10, /*#scodes*/ 0, /*flags*/ 0,
      {VT_R8, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {-1, 0, 0}
      },
      { /* names */
        "test_HRESULT_retval",
        NULL,
      },
    },
    {
      /*id*/ 0x5, /*func*/ FUNC_DISPATCH, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 0, /*#opt*/ 0, /*vtbl*/ 11, /*#scodes*/ 0, /*flags*/ 0,
      {VT_INT, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {-1, 0, 0}
      },
      { /* names */
        "test_int",
        NULL,
      },
    },
    {
      /*id*/ 0x6, /*func*/ FUNC_DISPATCH, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 0, /*#opt*/ 0, /*vtbl*/ 12, /*#scodes*/ 0, /*flags*/ 0,
      {VT_R8, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {-1, 0, 0}
      },
      { /* names */
        "test_int_retval",
        NULL,
      },
    },
    {
      /*id*/ 0x7, /*func*/ FUNC_DISPATCH, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 1, /*#opt*/ 0, /*vtbl*/ 13, /*#scodes*/ 0, /*flags*/ 0,
      {VT_R8, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 0, {},
      { /* params */
        {VT_BSTR, -1, PARAMFLAG_FIN},
        {-1, 0, 0}
      },
      { /* names */
        "parse_lcid",
        "x",
        NULL,
      },
    },
  },
  { /* vars */ },
},
{
  "custdata_interface",
  "{786ee4ff-c5dd-4bf4-9578-0d22fb5369cc}",
  /*kind*/ TKIND_INTERFACE, /*flags*/ TYPEFLAG_FDISPATCHABLE, /*align*/ TYPE_ALIGNMENT(custdata_interface*), /*size*/ sizeof(custdata_interface*),
  /*helpctx*/ 0x0000, /*version*/ 0x00000000, /*#vtbl*/ 8, /*#func*/ 1, /*#var*/ 0,
  /*#custdata*/ 1, {
    { "{c8768723-e6d2-4442-b039-92e9c82429c4}", { VT_BSTR, { .value_str = "ITypeInfo2::GetCustData interface" } } },
  },
  { /* funcs */
    {
      /*id*/ 0x60020000, /*func*/ FUNC_PUREVIRTUAL, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 1, /*#opt*/ 0, /*vtbl*/ 7, /*#scodes*/ 0, /*flags*/ 0,
      {VT_HRESULT, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 1, {
        { "{c8768723-e6d2-4442-b039-92e9c82429c4}", { VT_BSTR, { .value_str = "ITypeInfo2::GetFuncCustData custdata_interface::test_method" } } },
      },
      { /* params */
        {VT_INT, -1, PARAMFLAG_FIN, /*#custdata*/ 1, {
          { "{c8768723-e6d2-4442-b039-92e9c82429c4}", { VT_BSTR, { .value_str = "ITypeInfo2::GetParamCustData custdata_interface::test_method(x)" } } },
        } },
        {-1, 0, 0}
      },
      { /* names */
        "test_method",
        "x",
        NULL,
      },
    },
  },
  { /* vars */ },
},
{
  "custdata_enum",
  "{6ca99f5e-c86a-42ad-a5ee-5bd4c8e5553c}",
  /*kind*/ TKIND_ENUM, /*flags*/ 0, /*align*/ 4, /*size*/ 4,
  /*helpctx*/ 0x0000, /*version*/ 0x00000000, /*#vtbl*/ 0, /*#func*/ 0, /*#var*/ 2,
  /*#custdata*/ 1, {
    { "{c8768723-e6d2-4442-b039-92e9c82429c4}", { VT_BSTR, { .value_str = "ITypeInfo2::GetCustData enum" } } },
  },
  { /* funcs */ },
  { /* vars */
    {
      /*id*/ 0x40000000, /*name*/ "One", /*flags*/ 0, /*kind*/ VAR_CONST,
      { .varValue = { VT_I4, { .value_int = 0 } } },
      /*#custdata*/ 1, {
        { "{c8768723-e6d2-4442-b039-92e9c82429c4}", { VT_BSTR, { .value_str = "ITypeInfo2::GetVarCustData enum" } } },
      },
      {VT_INT, -1, PARAMFLAG_NONE}, /* ret */
    },
    {
      /*id*/ 0x40000001, /*name*/ "Two", /*flags*/ 0, /*kind*/ VAR_CONST,
      { .varValue = { VT_I4, { .value_int = 1 } } },
      /*#custdata*/ 0, {},
      {VT_INT, -1, PARAMFLAG_NONE}, /* ret */
    },
  },
},
{
  "custdata_struct",
  "{62fabe17-f733-4b09-b859-3f455dcda450}",
  /*kind*/ TKIND_RECORD, /*flags*/ 0, /*align*/ TYPE_ALIGNMENT(struct custdata_struct), /*size*/ sizeof(struct custdata_struct),
  /*helpctx*/ 0x0000, /*version*/ 0x00000000, /*#vtbl*/ 0, /*#func*/ 0, /*#var*/ 1,
  /*#custdata*/ 1, {
    { "{c8768723-e6d2-4442-b039-92e9c82429c4}", { VT_BSTR, { .value_str = "ITypeInfo2::GetCustData struct" } } },
  },
  { /* funcs */ },
  { /* vars */
    {
      /*id*/ 0x40000000, /*name*/ "test_field", /*flags*/ 0, /*kind*/ VAR_PERINSTANCE,
      { .oInst = 0 },
      /*#custdata*/ 1, {
        { "{c8768723-e6d2-4442-b039-92e9c82429c4}", { VT_BSTR, { .value_str = "ITypeInfo2::GetVarCustData struct" } } },
      },
      {VT_INT, -1, PARAMFLAG_NONE}, /* ret */
    },
  },
},
{
  "custdata_union",
  "{00000000-0000-0000-0000-000000000000}",
  /*kind*/ TKIND_UNION, /*flags*/ 0, /*align*/ TYPE_ALIGNMENT(union custdata_union), /*size*/ sizeof(union custdata_union),
  /*helpctx*/ 0x0000, /*version*/ 0x00000000, /*#vtbl*/ 0, /*#func*/ 0, /*#var*/ 1,
  /*#custdata*/ 1, {
    { "{c8768723-e6d2-4442-b039-92e9c82429c4}", { VT_BSTR, { .value_str = "ITypeInfo2::GetCustData union" } } },
  },
  { /* funcs */ },
  { /* vars */
    {
      /*id*/ 0x40000000, /*name*/ "test_field", /*flags*/ 0, /*kind*/ VAR_PERINSTANCE,
      { .oInst = 0 },
      /*#custdata*/ 1, {
        { "{c8768723-e6d2-4442-b039-92e9c82429c4}", { VT_BSTR, { .value_str = "ITypeInfo2::GetVarCustData union" } } },
      },
      {VT_INT, -1, PARAMFLAG_NONE}, /* ret */
    },
  },
},
{
  "custdata_typedef",
  "{d58744d6-63f9-467c-87e5-c95158098b18}",
  /*kind*/ TKIND_ALIAS, /*flags*/ 0, /*align*/ TYPE_ALIGNMENT(custdata_typedef), /*size*/ sizeof(custdata_typedef),
  /*helpctx*/ 0x0000, /*version*/ 0x00000000, /*#vtbl*/ 0, /*#func*/ 0, /*#var*/ 0,
  /*#custdata*/ 1, {
    { "{c8768723-e6d2-4442-b039-92e9c82429c4}", { VT_BSTR, { .value_str = "ITypeInfo2::GetCustData typedef" } } },
  },
  { /* funcs */ },
  { /* vars */ },
},
{
  "custdata_dispatch",
  "{bffc216e-2159-465a-80df-b85fd4f4f122}",
  /*kind*/ TKIND_DISPATCH, /*flags*/ TYPEFLAG_FDISPATCHABLE, /*align*/ TYPE_ALIGNMENT(custdata_dispatch*), /*size*/ sizeof(custdata_dispatch*),
  /*helpctx*/ 0x0000, /*version*/ 0x00000000, /*#vtbl*/ 7, /*#func*/ 1, /*#var*/ 1,
  /*#custdata*/ 1, {
    { "{c8768723-e6d2-4442-b039-92e9c82429c4}", { VT_BSTR, { .value_str = "ITypeInfo2::GetCustData dispinterface" } } },
  },
  { /* funcs */
    {
      /*id*/ 0x1, /*func*/ FUNC_DISPATCH, /*inv*/ INVOKE_FUNC, /*call*/ CC_STDCALL,
      /*#param*/ 1, /*#opt*/ 0, /*vtbl*/ 0, /*#scodes*/ 0, /*flags*/ 0,
      {VT_VOID, -1, PARAMFLAG_NONE}, /* ret */
      /*#custdata*/ 1, {
        { "{c8768723-e6d2-4442-b039-92e9c82429c4}", { VT_BSTR, { .value_str = "ITypeInfo2::GetFuncCustData dispinterface method" } } },
      },
      { /* params */
        {VT_INT, -1, PARAMFLAG_FIN, /*#custdata*/ 1, {
          { "{c8768723-e6d2-4442-b039-92e9c82429c4}", { VT_BSTR, { .value_str = "ITypeInfo2::GetParamCustData test_dispatch::test_method(x)" } } },
        } },
        {-1, 0, 0}
      },
      { /* names */
        "test_method",
        "x",
        NULL,
      },
    },
  },
  { /* vars */
    {
      /*id*/ 0x0, /*name*/ "test_property", /*flags*/ 0, /*kind*/ VAR_DISPATCH,
      { /* DUMMYUNIONNAME unused*/ },
      /*#custdata*/ 1, {
        { "{c8768723-e6d2-4442-b039-92e9c82429c4}", { VT_BSTR, { .value_str = "ITypeInfo2::GetVarCustData dispinterface property" } } },
      },
      {VT_INT, -1, PARAMFLAG_NONE}, /* ret */
    },
  },
}
};

#define check_variant_info(value, expected) { \
    expect_int(V_VT(value), (expected)->vt); \
    switch(V_VT(value)) { \
        case VT_I1: expect_int(V_I1(value), (expected)->value_int); break; \
        case VT_I2: expect_int(V_I2(value), (expected)->value_int); break; \
        case VT_I4: expect_int(V_I4(value), (expected)->value_int); break; \
        case VT_I8: expect_int(V_I8(value), (expected)->value_int); break; \
        case VT_BOOL: expect_int(V_BOOL(value), (expected)->value_int); break; \
        case VT_INT: expect_int(V_INT(value), (expected)->value_int); break; \
        case VT_UI1: expect_int(V_UI1(value), (expected)->value_uint); break; \
        case VT_UI2: expect_int(V_UI2(value), (expected)->value_uint); break; \
        case VT_UI4: expect_int(V_UI4(value), (expected)->value_uint); break; \
        case VT_UI8: expect_int(V_UI8(value), (expected)->value_uint); break; \
        case VT_UINT: expect_int(V_UINT(value), (expected)->value_uint); break; \
        case VT_BSTR: expect_wstr_acpval(V_BSTR(value), (expected)->value_str); break; \
        default: skip("check_variant_info: comparing value not implemented for VARTYPE %d\n",V_VT(value)); \
    } }

#define check_type(elem, info) { \
      expect_int((elem)->tdesc.vt, (info)->vt);                     \
      expect_hex((elem)->paramdesc.wParamFlags, (info)->wParamFlags); \
  }

static void parse_guid(LPCSTR strGuid, GUID *guid)
{
    WCHAR guidW[39];
    MultiByteToWideChar(CP_ACP, 0, strGuid, -1, guidW, ARRAY_SIZE(guidW));
    ole_check(IIDFromString(guidW, guid));
}

static void test_dump_typelib(const WCHAR *name)
{
    ITypeLib *typelib;
    CUSTDATA cust_data;
    int iface = 0, func, var, cust;
    HREFTYPE hRefType = 0;
    VARIANT v;
    HRESULT hr;
    TLIBATTR *libattr;
    const type_info *ti;

    ole_check(LoadTypeLibEx(name, REGKIND_NONE, &typelib));

    ole_check(ITypeLib_GetLibAttr(typelib, &libattr));
    if(libattr->syskind != info_syskind) {
        /* struct VARDESC::oInst may vary from changes in sizeof(void *) affecting the offset of later fields*/
        skip("ignoring VARDESC::oInst, (libattr->syskind expected %d got %d)\n", info_syskind, libattr->syskind);
    }

    for (ti = info; ti != info + ARRAY_SIZE(info); ti++)
    {
        ITypeInfo2 *typeinfo2;
        ITypeInfo *typeinfo;
        TYPEATTR *typeattr;
        BSTR bstrIfName;
        DWORD help_ctx;

        winetest_push_context("Interface %s", ti->name);
        ole_check(ITypeLib_GetTypeInfo(typelib, iface, &typeinfo));
        if (hRefType)
        {
            ITypeInfo *refInfo;
            ole_check(ITypeInfo_GetRefTypeInfo(typeinfo, hRefType, &refInfo));
            ITypeInfo_Release(typeinfo);
            typeinfo = refInfo;
        }
        ole_check(ITypeLib_GetDocumentation(typelib, iface, &bstrIfName, NULL, &help_ctx, NULL));
        expect_wstr_acpval(bstrIfName, ti->name);
        SysFreeString(bstrIfName);

        ole_check(ITypeInfo_GetTypeAttr(typeinfo, &typeattr));
        expect_int(typeattr->typekind, ti->type);
        expect_hex(typeattr->wTypeFlags, ti->wTypeFlags);
        expect_int(typeattr->cbAlignment, ti->cbAlignment);
        expect_int(typeattr->cbSizeInstance, ti->cbSizeInstance);
        expect_int(help_ctx, ti->help_ctx);
        expect_int(MAKELONG(typeattr->wMinorVerNum, typeattr->wMajorVerNum), ti->version);
        expect_int(typeattr->cbSizeVft, ti->cbSizeVft * sizeof(void*));
        expect_int(typeattr->cFuncs, ti->cFuncs);
        expect_int(typeattr->cVars, ti->cVars);

        /* compare type uuid */
        if (ti->uuid && *ti->uuid)
        {
            ITypeInfo *typeinfo2;
            HRESULT hr;
            GUID guid;

            parse_guid(ti->uuid,&guid);
            expect_guid(&guid, &typeattr->guid);

            /* check that it's possible to search using this uuid */
            typeinfo2 = NULL;
            hr = ITypeLib_GetTypeInfoOfGuid(typelib, &guid, &typeinfo2);
            ok(hr == S_OK || (IsEqualGUID(&guid, &IID_NULL) && hr == TYPE_E_ELEMENTNOTFOUND), "got 0x%08lx\n", hr);
            if (hr == S_OK) ITypeInfo_Release(typeinfo2);
        }

        ole_check(ITypeInfo_GetTypeAttr(typeinfo, &typeattr));

        hr = ITypeInfo_QueryInterface(typeinfo, &IID_ITypeInfo2, (void**)&typeinfo2);
        ok(hr == S_OK, "Could not get ITypeInfo2: %08lx\n", hr);

        memset(&cust_data, 0, sizeof(cust_data));
        ole_check(ITypeInfo2_GetAllCustData(typeinfo2,&cust_data));
        expect_int(cust_data.cCustData, ti->cCustData);
        ClearCustData(&cust_data);
        for (cust = 0; cust < ti->cCustData; cust++)
        {
            GUID guid;
            parse_guid(ti->custdata[cust].uuid,&guid);
            /* check that it's possible to search using this uuid */
            hr = ITypeInfo2_GetCustData(typeinfo2,&guid,&v);
            ok(hr == S_OK, "GetCustDatafailed: %08lx\n", hr);
            check_variant_info(&v,&ti->custdata[cust].value);
            VariantClear(&v);
        }

        for (func = 0; func < typeattr->cFuncs; func++)
        {
            const function_info *fn_info = &ti->funcs[func];
            FUNCDESC *desc;
            BSTR namesTab[256];
            UINT cNames;
            int i;

            winetest_push_context("Function %s", fn_info->names[0]);
            ole_check(ITypeInfo_GetFuncDesc(typeinfo, func, &desc));
            expect_int(desc->memid, fn_info->memid);
            expect_int(desc->funckind, fn_info->funckind);
            expect_int(desc->invkind, fn_info->invkind);
            expect_int(desc->callconv, fn_info->callconv);
            expect_int(desc->cParams, fn_info->cParams);
            expect_int(desc->cParamsOpt, fn_info->cParamsOpt);
            expect_int(desc->oVft, fn_info->vtbl_index * sizeof(void*));
            expect_int(desc->cScodes, fn_info->cScodes);
            expect_int(desc->wFuncFlags, fn_info->wFuncFlags);

            memset(&cust_data, 0, sizeof(cust_data));
            ole_check(ITypeInfo2_GetAllFuncCustData(typeinfo2,func,&cust_data));
            expect_int(cust_data.cCustData, fn_info->cCustData);
            ClearCustData(&cust_data);
            for (cust = 0; cust < fn_info->cCustData; cust++)
            {
                GUID guid;
                parse_guid(fn_info->custdata[cust].uuid,&guid);
                /* check that it's possible to search using this uuid */
                hr = ITypeInfo2_GetFuncCustData(typeinfo2,func,&guid,&v);
                ok(hr == S_OK, "GetCustDatafailed: %08lx\n", hr);
                check_variant_info(&v,&fn_info->custdata[cust].value);
                VariantClear(&v);
            }

            ole_check(ITypeInfo_GetNames(typeinfo, desc->memid, namesTab, 256, &cNames));
            for (i = 0; i < cNames; i++)
            {
                expect_wstr_acpval(namesTab[i], fn_info->names[i]);
                SysFreeString(namesTab[i]);
            }
            expect_null(fn_info->names[cNames]);

            check_type(&desc->elemdescFunc, &fn_info->ret_type);
            for (i = 0 ; i < desc->cParams; i++)
            {
                check_type(&desc->lprgelemdescParam[i], &fn_info->params[i]);

                memset(&cust_data, 0, sizeof(cust_data));
                ole_check(ITypeInfo2_GetAllParamCustData(typeinfo2,func,i,&cust_data));
                expect_int(cust_data.cCustData, fn_info->params[i].cCustData);
                ClearCustData(&cust_data);
                for (cust = 0; cust < fn_info->params[i].cCustData; cust++)
                {
                    GUID guid;
                    parse_guid(fn_info->params[i].custdata[cust].uuid,&guid);
                    /* check that it's possible to search using this uuid */
                    hr = ITypeInfo2_GetParamCustData(typeinfo2,func,i,&guid,&v);
                    ok(hr == S_OK, "GetParamCustDatafailed: %08lx\n", hr);
                    check_variant_info(&v,&fn_info->params[i].custdata[cust].value);
                    VariantClear(&v);
                }

                if (desc->lprgelemdescParam[i].tdesc.vt == VT_USERDEFINED)
                {
                    ITypeInfo *param;
                    TYPEATTR *var_attr;

                    ole_check(ITypeInfo_GetRefTypeInfo(typeinfo, desc->lprgelemdescParam[i].tdesc.hreftype, &param));
                    ole_check(ITypeInfo_GetTypeAttr(param, &var_attr));

                    ok(var_attr->typekind == fn_info->params[i].type, "expected %#x, got %#x\n", fn_info->params[i].type, var_attr->typekind);

                    ITypeInfo_ReleaseTypeAttr(param, var_attr);
                    ITypeInfo_Release(param);
                }
            }
            expect_int(fn_info->params[desc->cParams].vt, (VARTYPE)-1);

            V_VT(&v) = VT_ERROR;
            hr = ITypeInfo2_GetFuncCustData(typeinfo2, func, &IID_NULL, &v);
            ok(hr == S_OK, "GetFuncCustData failed: %08lx\n", hr);
            ok(V_VT(&v) == VT_EMPTY, "V_VT(&v) = %d\n", V_VT(&v));
            VariantClear(&v);

            V_VT(&v) = VT_ERROR;
            hr = ITypeInfo2_GetFuncCustData(typeinfo2, func, &IID_IBaseIface, &v);
            ok(hr == S_OK, "GetFuncCustData failed: %08lx\n", hr);
            ok(V_VT(&v) == VT_EMPTY, "V_VT(&v) = %d\n", V_VT(&v));
            VariantClear(&v);

            memset(&cust_data, 0, sizeof(cust_data));
            ITypeInfo_ReleaseFuncDesc(typeinfo, desc);
            winetest_pop_context();
        }

        for (var = 0; var < typeattr->cVars; var++)
        {
            const var_info *var_info = &ti->vars[var];
            VARDESC *desc;
            BSTR varname;
            UINT cNames;

            winetest_push_context("Variable %s", var_info->name);
            ole_check(ITypeInfo_GetVarDesc(typeinfo, var, &desc));

            expect_int(desc->memid, var_info->memid);

            ole_check(ITypeInfo_GetNames(typeinfo, desc->memid, &varname, 1, &cNames));
            expect_int(cNames, 1);
            expect_wstr_acpval(varname, var_info->name);
            SysFreeString(varname);

            expect_null(desc->lpstrSchema); /* Reserved */
            expect_int(desc->wVarFlags, var_info->wVarFlags);
            expect_int(desc->varkind, var_info->varkind);
            if (desc->varkind == VAR_PERINSTANCE) {
                /* oInst depends on preceding field data sizes (except for unions),
                 * so it may not be valid to expect it to match info[] on other platforms */
                if ((libattr->syskind == info_syskind) || (typeattr->typekind == TKIND_UNION)) {
                    expect_int(desc->oInst, var_info->oInst);
                }
            } else if(desc->varkind == VAR_CONST) {
                check_variant_info(desc->lpvarValue, &var_info->varValue);
            } else {
                expect_null(desc->lpvarValue);
            }
            memset(&cust_data, 0, sizeof(cust_data));
            ole_check(ITypeInfo2_GetAllVarCustData(typeinfo2,var,&cust_data));
            expect_int(cust_data.cCustData, var_info->cCustData);
            ClearCustData(&cust_data);
            for (cust = 0; cust < var_info->cCustData; cust++)
            {
                GUID guid;
                parse_guid(var_info->custdata[cust].uuid,&guid);
                /* check that it's possible to search using this uuid */
                hr = ITypeInfo2_GetVarCustData(typeinfo2,var,&guid,&v);
                ok(hr == S_OK, "GetVarCustData failed: %08lx\n", hr);
                check_variant_info(&v,&var_info->custdata[cust].value);
                VariantClear(&v);
            }

            check_type(&desc->elemdescVar, &var_info->elemdescVar);

            ITypeInfo_ReleaseVarDesc(typeinfo, desc);
            winetest_pop_context();
        }

        if ((typeattr->typekind == TKIND_DISPATCH) && (typeattr->wTypeFlags & TYPEFLAG_FDUAL) &&
            SUCCEEDED(ITypeInfo_GetRefTypeOfImplType(typeinfo, -1, &hRefType)))
        {
            /* next iteration dumps hRefType, the TKIND_INTERFACE reference underneath this [dual] TKIND_DISPATCH */
        }
        else
        {
            iface++; /* move to the next item in typelib */
            hRefType = 0;
        }

        ITypeInfo_ReleaseTypeAttr(typeinfo, typeattr);

        ITypeInfo2_Release(typeinfo2);
        ITypeInfo_Release(typeinfo);
        winetest_pop_context();
    }
    expect_eq(ITypeLib_GetTypeInfoCount(typelib), iface, UINT, "%d");
    ITypeLib_ReleaseTLibAttr(typelib, libattr);
    ITypeLib_Release(typelib);
}

#endif

static void test_create_typelib_lcid(LCID lcid)
{
    char filename[MAX_PATH];
    WCHAR name[MAX_PATH];
    HRESULT hr;
    ICreateTypeLib2 *tl;
    HANDLE file;
    DWORD msft_header[8];
    ITypeLib *typelib;
    TLIBATTR *attr;
    DWORD read;

    GetTempFileNameA( ".", "tlb", 0, filename );
    MultiByteToWideChar(CP_ACP, 0, filename, -1, name, MAX_PATH);

    hr = CreateTypeLib2(SYS_WIN32, name, &tl);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ICreateTypeLib2_QueryInterface(tl, &IID_ITypeLib, (void**)&typelib);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ITypeLib_GetLibAttr(typelib, &attr);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(attr->wLibFlags == 0, "flags 0x%x\n", attr->wLibFlags);
    ITypeLib_ReleaseTLibAttr(typelib, attr);

    hr = ICreateTypeLib2_SetLcid(tl, lcid);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ICreateTypeLib2_SetVersion(tl, 3, 4);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ICreateTypeLib2_SaveAllChanges(tl);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ITypeLib_GetLibAttr(typelib, &attr);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(attr->wLibFlags == 0, "flags 0x%x\n", attr->wLibFlags);
    ITypeLib_ReleaseTLibAttr(typelib, attr);

    ITypeLib_Release(typelib);
    ICreateTypeLib2_Release(tl);

    file = CreateFileA( filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0 );
    ok( file != INVALID_HANDLE_VALUE, "file creation failed\n" );

    ReadFile( file, msft_header, sizeof(msft_header), &read, NULL );
    ok(read == sizeof(msft_header), "read %ld\n", read);
    CloseHandle( file );

    ok(msft_header[0] == 0x5446534d, "got %08lx\n", msft_header[0]);
    ok(msft_header[1] == 0x00010002, "got %08lx\n", msft_header[1]);
    ok(msft_header[2] == 0xffffffff, "got %08lx\n", msft_header[2]);
    ok(msft_header[3] == (lcid ? lcid : 0x409), "got %08lx (lcid %08lx)\n", msft_header[3], lcid);
    ok(msft_header[4] == lcid, "got %08lx (lcid %08lx)\n", msft_header[4], lcid);
    ok(msft_header[6] == 0x00040003, "got %08lx\n", msft_header[6]);
    ok(msft_header[7] == 0, "got %08lx\n", msft_header[7]);

    /* check flags after loading */
    hr = LoadTypeLib(name, &typelib);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ITypeLib_GetLibAttr(typelib, &attr);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(attr->wLibFlags == LIBFLAG_FHASDISKIMAGE, "flags 0x%x\n", attr->wLibFlags);
    ITypeLib_ReleaseTLibAttr(typelib, attr);
    ITypeLib_Release(typelib);

    DeleteFileA(filename);
}

static void test_create_typelibs(void)
{
    test_create_typelib_lcid(LOCALE_SYSTEM_DEFAULT);
    test_create_typelib_lcid(LOCALE_USER_DEFAULT);
    test_create_typelib_lcid(LOCALE_NEUTRAL);

    test_create_typelib_lcid(0x009);
    test_create_typelib_lcid(0x409);
    test_create_typelib_lcid(0x809);

    test_create_typelib_lcid(0x007);
    test_create_typelib_lcid(0x407);
}


static void test_register_typelib(BOOL system_registration)
{
    HRESULT hr;
    WCHAR *filename;
    ITypeLib *typelib;
    WCHAR uuidW[40];
    char key_name[MAX_PATH], uuid[40];
    LONG ret, expect_ret;
    UINT count, i;
    HKEY hkey;
    REGSAM opposite = (sizeof(void*) == 8 ? KEY_WOW64_32KEY : KEY_WOW64_64KEY);
    BOOL is_wow64 = FALSE;
    LONG size;
    struct
    {
        TYPEKIND kind;
        WORD flags;
    } attrs[] =
    {
        { TKIND_INTERFACE, 0 },
        { TKIND_INTERFACE, TYPEFLAG_FDISPATCHABLE },
        { TKIND_INTERFACE, TYPEFLAG_FOLEAUTOMATION },
        { TKIND_INTERFACE, TYPEFLAG_FDISPATCHABLE | TYPEFLAG_FOLEAUTOMATION },
        { TKIND_DISPATCH,  TYPEFLAG_FDUAL },
        { TKIND_DISPATCH,  TYPEFLAG_FDUAL },
        { TKIND_DISPATCH,  TYPEFLAG_FDISPATCHABLE | TYPEFLAG_FDUAL },
        { TKIND_DISPATCH,  TYPEFLAG_FDISPATCHABLE | TYPEFLAG_FDUAL },
        { TKIND_DISPATCH,  TYPEFLAG_FDISPATCHABLE },
        { TKIND_DISPATCH,  TYPEFLAG_FDISPATCHABLE },
        { TKIND_DISPATCH,  TYPEFLAG_FDISPATCHABLE },
        { TKIND_INTERFACE, TYPEFLAG_FDISPATCHABLE },
        { TKIND_INTERFACE, TYPEFLAG_FDISPATCHABLE },
        { TKIND_RECORD, 0 },
        { TKIND_MODULE, 0 },
    };

    if (!system_registration && (!pRegisterTypeLibForUser || !pUnRegisterTypeLibForUser))
    {
        win_skip("User typelib registration functions are not available\n");
        return;
    }
    winetest_push_context( "%s", system_registration ? "system" : "user" );

    if (pIsWow64Process)
        pIsWow64Process(GetCurrentProcess(), &is_wow64);

    filename = create_test_typelib(3);

    hr = LoadTypeLibEx(filename, REGKIND_NONE, &typelib);
    ok(hr == S_OK, "got %08lx\n", hr);

    if (system_registration)
        hr = RegisterTypeLib(typelib, filename, NULL);
    else
        hr = pRegisterTypeLibForUser(typelib, filename, NULL);
    if (hr == TYPE_E_REGISTRYACCESS)
    {
        win_skip("Insufficient privileges to register typelib in the registry\n");
        ITypeLib_Release(typelib);
        DeleteFileW(filename);
        winetest_pop_context();
        return;
    }
    ok(hr == S_OK, "got %08lx\n", hr);

    count = ITypeLib_GetTypeInfoCount(typelib);
    ok(count == 15, "got %d\n", count);

    for(i = 0; i < count; i++)
    {
        ITypeInfo *typeinfo;
        TYPEATTR *attr;

        hr = ITypeLib_GetTypeInfo(typelib, i, &typeinfo);
        ok(hr == S_OK, "got %08lx\n", hr);

        hr = ITypeInfo_GetTypeAttr(typeinfo, &attr);
        ok(hr == S_OK, "got %08lx\n", hr);

        ok(attr->typekind == attrs[i].kind, "%d: got kind %d\n", i, attr->typekind);
        ok(attr->wTypeFlags == attrs[i].flags, "%d: got flags %04x\n", i, attr->wTypeFlags);

        if(attr->typekind == TKIND_DISPATCH && (attr->wTypeFlags & TYPEFLAG_FDUAL))
        {
            HREFTYPE reftype;
            ITypeInfo *dual_info;
            TYPEATTR *dual_attr;

            hr = ITypeInfo_GetRefTypeOfImplType(typeinfo, -1, &reftype);
            ok(hr == S_OK, "got %08lx\n", hr);

            hr = ITypeInfo_GetRefTypeInfo(typeinfo, reftype, &dual_info);
            ok(hr == S_OK, "got %08lx\n", hr);

            hr = ITypeInfo_GetTypeAttr(dual_info, &dual_attr);
            ok(hr == S_OK, "got %08lx\n", hr);

            ok(dual_attr->typekind == TKIND_INTERFACE, "%d: got kind %d\n", i, dual_attr->typekind);
            ok(dual_attr->wTypeFlags == (attrs[i].flags | TYPEFLAG_FOLEAUTOMATION),
                "%d: got flags %04x\n", i, dual_attr->wTypeFlags);

            ITypeInfo_ReleaseTypeAttr(dual_info, dual_attr);
            ITypeInfo_Release(dual_info);

        }

        StringFromGUID2(&attr->guid, uuidW, ARRAY_SIZE(uuidW));
        WideCharToMultiByte(CP_ACP, 0, uuidW, -1, uuid, sizeof(uuid), NULL, NULL);
        sprintf(key_name, "Interface\\%s", uuid);

        /* All dispinterfaces will be registered (this includes dual interfaces) as well
           as oleautomation interfaces */
        if((attr->typekind == TKIND_INTERFACE && (attr->wTypeFlags & TYPEFLAG_FOLEAUTOMATION)) ||
           attr->typekind == TKIND_DISPATCH)
            expect_ret = ERROR_SUCCESS;
        else
            expect_ret = ERROR_FILE_NOT_FOUND;

        ret = RegOpenKeyExA(HKEY_CLASSES_ROOT, key_name, 0, KEY_READ, &hkey);
        ok(ret == expect_ret, "%d: got %ld\n", i, ret);
        if (ret == ERROR_SUCCESS)
        {
            size = sizeof(uuid);
            ret = RegQueryValueA(hkey, "ProxyStubClsid32", uuid, &size);
            ok(!ret, "Failed to get proxy GUID, error %lu.\n", ret);

            if (attrs[i].kind == TKIND_INTERFACE || (attrs[i].flags & TYPEFLAG_FDUAL))
            {
                ok(!strcasecmp(uuid, "{00020424-0000-0000-c000-000000000046}"),
                        "Got unexpected proxy CLSID %s.\n", uuid);
            }
            else
            {
                ok(!strcasecmp(uuid, "{00020420-0000-0000-c000-000000000046}"),
                        "Got unexpected proxy CLSID %s.\n", uuid);
            }

            RegCloseKey(hkey);
        }


        /* 32-bit typelibs should be registered into both registry bit modes */
        if (is_win64 || is_wow64)
        {
            ret = RegOpenKeyExA(HKEY_CLASSES_ROOT, key_name, 0, KEY_READ | opposite, &hkey);
            ok(ret == expect_ret, "%d: got %ld\n", i, ret);
            if(ret == ERROR_SUCCESS) RegCloseKey(hkey);
        }

        ITypeInfo_ReleaseTypeAttr(typeinfo, attr);
        ITypeInfo_Release(typeinfo);
    }

    if (system_registration)
        hr = UnRegisterTypeLib(&LIBID_register_test, 1, 0, LOCALE_NEUTRAL, is_win64 ? SYS_WIN64 : SYS_WIN32);
    else
        hr = pUnRegisterTypeLibForUser(&LIBID_register_test, 1, 0, LOCALE_NEUTRAL, is_win64 ? SYS_WIN64 : SYS_WIN32);
    ok(hr == S_OK, "got %08lx\n", hr);

    for(i = 0; i < count; i++)
    {
        ITypeInfo *typeinfo;
        TYPEATTR *attr;

        hr = ITypeLib_GetTypeInfo(typelib, i, &typeinfo);
        ok(hr == S_OK, "got %08lx\n", hr);

        hr = ITypeInfo_GetTypeAttr(typeinfo, &attr);
        ok(hr == S_OK, "got %08lx\n", hr);

        if((attr->typekind == TKIND_INTERFACE && (attr->wTypeFlags & TYPEFLAG_FOLEAUTOMATION)) ||
           attr->typekind == TKIND_DISPATCH)
        {
            StringFromGUID2(&attr->guid, uuidW, ARRAY_SIZE(uuidW));
            WideCharToMultiByte(CP_ACP, 0, uuidW, -1, uuid, sizeof(uuid), NULL, NULL);
            sprintf(key_name, "Interface\\%s", uuid);

            ret = RegOpenKeyExA(HKEY_CLASSES_ROOT, key_name, 0, KEY_READ, &hkey);
            ok(ret == ERROR_FILE_NOT_FOUND, "Interface registry remains in %s (%d)\n", key_name, i);
            if (is_win64 || is_wow64)
            {
                ret = RegOpenKeyExA(HKEY_CLASSES_ROOT, key_name, 0, KEY_READ | opposite, &hkey);
                ok(ret == ERROR_FILE_NOT_FOUND, "Interface registry remains in %s (%d)\n", key_name, i);
            }
        }
        ITypeInfo_ReleaseTypeAttr(typeinfo, attr);
        ITypeInfo_Release(typeinfo);
    }

    ITypeLib_Release(typelib);
    DeleteFileW(filename);
    winetest_pop_context();
}

static void test_register_typelib_64(void)
{
    REGSAM opposite = (sizeof(void*) == 8 ? KEY_WOW64_32KEY : KEY_WOW64_64KEY);
    ICreateTypeInfo *createti, *createti_co;
    ELEMDESC elemdesc[5], elemdesc2[5];
    FUNCDESC funcdesc, funcdesc2;
    ITypeInfo *unknown, *tinfo;
    ITypeLib *stdole, *typelib;
    ICreateTypeLib2 *createtl;
    WCHAR filename[MAX_PATH];
    BOOL is_wow64 = FALSE;
    HREFTYPE hreftype;
    HRESULT hr;

    static const WCHAR wszStdOle2[] = { 's','t','d','o','l','e','2','.','t','l','b',0 };

    static OLECHAR typelibW[] = { 'W','i','n','e','T','e','s','t','T','y','p','e','L','i','b',0};
    static OLECHAR helpfileW[] = { 'C',':','\\','b','o','g','u','s','.','h','l','p',0 };
    static OLECHAR dllfileW[] = { 'C',':','\\','b','o','g','u','s','.','d','l','l',0 };
    static OLECHAR interface1W[] = { 'I','T','e','s','t','W','i','n','e','6', '4', 0};
    static OLECHAR coclassW[] = {'W','i','n','e','T','e','s','t','C','o','c','l','a','s','s',0 };
    static OLECHAR func1W[] = { 'f','u','n','c','1',0 };
    static OLECHAR func2W[] = { 'f','u','n','c','2',0 };
    static OLECHAR param1W[] = { 'p','a','r','a','m','1',0 };
    static OLECHAR param2W[] = { 'p','a','r','a','m','2',0 };
    static OLECHAR *names1[] = { func1W, param1W, param2W };
    static OLECHAR *names2[] = { func2W, param1W };
    static const GUID tlcustguid = { 0x512d2fec,0xcaf6,0x4c33,{0xbc,0x38,0x84,0x2f,0x2e,0x37,0x0d,0x7b} };
    static const GUID coclassguid = { 0x317cd4dd,0x0ce0,0x4525,{0x8d,0x33,0x68,0x14,0x4c,0x53,0x60,0xe9} };
    static const GUID interfaceguid = { 0x35cc5cea,0x11cc,0x4bca,{0x89,0x8c,0xf8,0x92,0x8e,0xb8,0xda,0x24} };

    static const SYSKIND sys = SYS_WIN64;

    if (pIsWow64Process)
        pIsWow64Process(GetCurrentProcess(), &is_wow64);

    hr = LoadTypeLib(wszStdOle2, &stdole);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ITypeLib_GetTypeInfoOfGuid(stdole, &IID_IDispatch, &unknown);
    ok(hr == S_OK, "got %08lx\n", hr);

    GetTempFileNameW(L".", L"tlb", 0, filename);

    hr = CreateTypeLib2(sys, filename, &createtl);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ICreateTypeLib2_SetName(createtl, typelibW);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ICreateTypeLib2_SetHelpFileName(createtl, helpfileW);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ICreateTypeLib2_SetHelpStringDll(createtl, dllfileW);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ICreateTypeLib2_SetGuid(createtl, &tlcustguid);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ICreateTypeLib2_SetVersion(createtl, 1, 0);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ICreateTypeLib2_CreateTypeInfo(createtl, coclassW, TKIND_COCLASS, &createti_co);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ICreateTypeLib2_CreateTypeInfo(createtl,  interface1W, TKIND_INTERFACE, &createti);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ICreateTypeInfo_LayOut(createti);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ICreateTypeInfo_SetGuid(createti, &interfaceguid);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ICreateTypeInfo_SetVersion(createti, 1, 0);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ICreateTypeInfo_AddRefTypeInfo(createti, unknown, &hreftype);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ICreateTypeInfo_AddImplType(createti, 0, hreftype);
    ok(hr == S_OK, "got %08lx\n", hr);

    elemdesc[0].tdesc.vt = VT_UINT;
    elemdesc[0].idldesc.dwReserved = 0;
    elemdesc[0].idldesc.wIDLFlags = IDLFLAG_FIN;

    elemdesc[1].tdesc.vt = VT_DECIMAL;
    elemdesc[1].idldesc.dwReserved = 0;
    elemdesc[1].idldesc.wIDLFlags = IDLFLAG_FIN;

    memset(&funcdesc, 0, sizeof(FUNCDESC));
    funcdesc.funckind = FUNC_PUREVIRTUAL;
    funcdesc.invkind = INVOKE_FUNC;
    funcdesc.callconv = CC_STDCALL;
    funcdesc.cParams = 2;
    funcdesc.memid = 0x1;
    funcdesc.elemdescFunc.tdesc.vt = VT_HRESULT;
    funcdesc.lprgelemdescParam = elemdesc;

    hr = ICreateTypeInfo_AddFuncDesc(createti, 0, &funcdesc);
    ok(hr == S_OK, "got %08lx\n", hr);

    elemdesc2[0].tdesc.vt = VT_UINT;
    elemdesc2[0].idldesc.dwReserved = 0;
    elemdesc2[0].idldesc.wIDLFlags = IDLFLAG_FIN | IDLFLAG_FOUT;

    memset(&funcdesc2, 0, sizeof(FUNCDESC));
    funcdesc2.funckind = FUNC_PUREVIRTUAL;
    funcdesc2.invkind = INVOKE_FUNC;
    funcdesc.callconv = CC_STDCALL;
    funcdesc2.cParams = 1;
    funcdesc2.memid = 0x2;
    funcdesc2.elemdescFunc.tdesc.vt = VT_HRESULT;
    funcdesc2.lprgelemdescParam = elemdesc2;

    hr = ICreateTypeInfo_AddFuncDesc(createti, 1, &funcdesc2);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ICreateTypeInfo_SetFuncHelpContext(createti, 0, 0xabcdefab);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ICreateTypeInfo_SetFuncHelpContext(createti, 1, 0xabcdefab);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ICreateTypeInfo_SetFuncAndParamNames(createti, 0, names1, 3);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ICreateTypeInfo_SetFuncAndParamNames(createti, 1, names2, 2);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ICreateTypeInfo_SetTypeFlags(createti, TYPEFLAG_FOLEAUTOMATION | TYPEFLAG_FNONEXTENSIBLE | TYPEFLAG_FDUAL);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ICreateTypeInfo_QueryInterface(createti, &IID_ITypeInfo, (void**)&tinfo);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ITypeInfo_GetRefTypeOfImplType(tinfo, 0, &hreftype);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ICreateTypeInfo_SetGuid(createti_co, &coclassguid);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ICreateTypeInfo_AddRefTypeInfo(createti_co, tinfo, &hreftype);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ICreateTypeInfo_AddImplType(createti_co, 0, hreftype);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ICreateTypeInfo_SetImplTypeFlags(createti_co, 0, IMPLTYPEFLAG_FDEFAULT);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ICreateTypeInfo_Release(createti_co);
    ok(hr == S_OK, "got %08lx\n", hr);

    ICreateTypeInfo_Release(createti);

    hr = ICreateTypeLib2_SaveAllChanges(createtl);
    ok(hr == S_OK, "got %08lx\n", hr);

    ICreateTypeLib2_Release(createtl);

    trace("TypeLib to load: %ls\n", filename);
    hr = LoadTypeLibEx(filename, REGKIND_NONE, &typelib);
    ok(hr == S_OK, "got: %08lx\n", hr);

    if(typelib)
    {
        WCHAR key_name[MAX_PATH], uuid[40];
        OLECHAR tlb_name[16];
        HKEY hkey;
        LONG size;

        hr = RegisterTypeLib(typelib, filename, NULL);
        if (hr == TYPE_E_REGISTRYACCESS)
        {
            win_skip("Insufficient privileges to register typelib in the registry\n");
            ITypeLib_Release(typelib);
            DeleteFileW(filename);
            return;
        }
        ok(hr == S_OK, "got: %08lx\n", hr);

        ITypeLib_Release(typelib);

        StringFromGUID2(&tlcustguid, uuid, ARRAY_SIZE(uuid));
        swprintf(key_name, sizeof(key_name), L"TypeLib\\%ls\\1.0", uuid);

        hr = RegOpenKeyExW(HKEY_CLASSES_ROOT, key_name, 0, KEY_READ, &hkey);
        ok(hr == S_OK, "got %08lx\n", hr);

        size = sizeof(tlb_name);
        hr = RegQueryValueW(hkey, L"", tlb_name, &size);
        ok(hr == S_OK, "got %08lx\n", hr);

        /* The typelib should be registered in WoW64_32 and WoW64_64 mode */
        if(is_win64 || is_wow64)
        {
            hr = RegOpenKeyExW(HKEY_CLASSES_ROOT, key_name, 0, KEY_READ | opposite, &hkey);
            ok(hr == S_OK, "got %08lx\n", hr);

            if(hkey)
            {
                size = sizeof(tlb_name);
                hr = RegQueryValueW(hkey, L"", tlb_name, &size);
                ok(hr == S_OK, "got %08lx\n", hr);

                RegCloseKey(hkey);
            }
        }

        ok(!wcscmp(tlb_name, typelibW),
            "Got unexpected TypLib description: %ls\n", tlb_name);
    }

    DeleteFileW(filename);
}

static void test_LoadTypeLib(void)
{
    ITypeLib *tl;
    HRESULT hres;

    static const WCHAR kernel32_dllW[] = {'k','e','r','n','e','l','3','2','.','d','l','l',0};

    hres = LoadTypeLib(kernel32_dllW, &tl);
    ok(hres == TYPE_E_CANTLOADLIBRARY, "LoadTypeLib returned: %08lx, expected TYPE_E_CANTLOADLIBRARY\n", hres);

    hres = LoadTypeLib(NULL, NULL);
    ok(hres == E_INVALIDARG, "Got %#lx.\n", hres);

    tl = (void *)0xdeadbeef;
    hres = LoadTypeLib(NULL, &tl);
    ok(hres == E_INVALIDARG, "Got %#lx.\n", hres);
    ok(tl == (void *)0xdeadbeef, "Got %p.\n", tl);

    hres = LoadTypeLibEx(NULL, REGKIND_NONE, NULL);
    ok(hres == E_INVALIDARG, "Got %#lx.\n", hres);

    tl = (void *)0xdeadbeef;
    hres = LoadTypeLibEx(NULL, REGKIND_NONE, &tl);
    ok(hres == E_INVALIDARG, "Got %#lx.\n", hres);
    ok(tl == (void *)0xdeadbeef, "Got %p.\n", tl);
}

static void test_SetVarHelpContext(void)
{
    static OLECHAR nameW[] = {'n','a','m','e',0};
    CHAR filenameA[MAX_PATH];
    WCHAR filenameW[MAX_PATH];
    ICreateTypeLib2 *ctl;
    ICreateTypeInfo *cti;
    ITypeLib *tl;
    ITypeInfo *ti;
    VARDESC desc, *pdesc;
    HRESULT hr;
    DWORD ctx;
    VARIANT v;

    GetTempFileNameA(".", "tlb", 0, filenameA);
    MultiByteToWideChar(CP_ACP, 0, filenameA, -1, filenameW, MAX_PATH);

    hr = CreateTypeLib2(SYS_WIN32, filenameW, &ctl);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ICreateTypeLib2_CreateTypeInfo(ctl, nameW, TKIND_ENUM, &cti);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ICreateTypeInfo_SetVarHelpContext(cti, 0, 0);
    ok(hr == TYPE_E_ELEMENTNOTFOUND, "got %08lx\n", hr);

    memset(&desc, 0, sizeof(desc));
    desc.memid = MEMBERID_NIL;
    desc.elemdescVar.tdesc.vt = VT_INT;
    desc.varkind = VAR_CONST;

    V_VT(&v) = VT_INT;
    V_INT(&v) = 1;
    desc.lpvarValue = &v;
    hr = ICreateTypeInfo_AddVarDesc(cti, 0, &desc);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ICreateTypeInfo_SetVarHelpContext(cti, 0, 0);
    ok(hr == S_OK, "got %08lx\n", hr);

    /* another time */
    hr = ICreateTypeInfo_SetVarHelpContext(cti, 0, 1);
    ok(hr == S_OK, "got %08lx\n", hr);

    /* wrong index now */
    hr = ICreateTypeInfo_SetVarHelpContext(cti, 1, 0);
    ok(hr == TYPE_E_ELEMENTNOTFOUND, "got %08lx\n", hr);

    ICreateTypeInfo_Release(cti);

    hr = ICreateTypeLib2_SaveAllChanges(ctl);
    ok(hr == S_OK, "got: %08lx\n", hr);

    ICreateTypeLib2_Release(ctl);

    hr = LoadTypeLib(filenameW, &tl);
    ok(hr == S_OK, "got: %08lx\n", hr);

    hr = ITypeLib_GetTypeInfo(tl, 0, &ti);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ITypeInfo_GetVarDesc(ti, 0, &pdesc);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(pdesc->memid == 0x40000000, "got wrong memid: %lx\n", pdesc->memid);
    ok(pdesc->elemdescVar.tdesc.vt == VT_INT, "got wrong vardesc type: %u\n", pdesc->elemdescVar.tdesc.vt);
    ok(pdesc->varkind == VAR_CONST, "got wrong varkind: %u\n", pdesc->varkind);
    ok(V_VT(pdesc->lpvarValue) == VT_INT, "got wrong value type: %u\n", V_VT(pdesc->lpvarValue));
    ok(V_INT(pdesc->lpvarValue) == 1, "got wrong value: 0x%x\n", V_INT(pdesc->lpvarValue));

    hr = ITypeInfo_GetDocumentation(ti, pdesc->memid, NULL, NULL, &ctx, NULL);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(ctx == 1, "got wrong help context: 0x%lx\n", ctx);

    ITypeInfo_ReleaseVarDesc(ti, pdesc);
    ITypeInfo_Release(ti);
    ITypeLib_Release(tl);

    DeleteFileA(filenameA);
}

static void test_SetFuncAndParamNames(void)
{
    static OLECHAR nameW[] = {'n','a','m','e',0};
    static OLECHAR name2W[] = {'n','a','m','e','2',0};
    static OLECHAR prop[] = {'p','r','o','p',0};
    static OLECHAR *propW[] = {prop};
    static OLECHAR func[] = {'f','u','n','c',0};
    static OLECHAR *funcW[] = {func, NULL};
    CHAR filenameA[MAX_PATH];
    WCHAR filenameW[MAX_PATH];
    ICreateTypeLib2 *ctl;
    ICreateTypeInfo *cti;
    ITypeLib *tl;
    ITypeInfo *infos[3];
    MEMBERID memids[3];
    FUNCDESC funcdesc;
    ELEMDESC edesc;
    HRESULT hr;
    USHORT found;

    GetTempFileNameA(".", "tlb", 0, filenameA);
    MultiByteToWideChar(CP_ACP, 0, filenameA, -1, filenameW, MAX_PATH);

    hr = CreateTypeLib2(SYS_WIN32, filenameW, &ctl);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ICreateTypeLib2_CreateTypeInfo(ctl, nameW, TKIND_DISPATCH, &cti);
    ok(hr == S_OK, "got %08lx\n", hr);

    /* get method */
    memset(&funcdesc, 0, sizeof(FUNCDESC));
    funcdesc.funckind = FUNC_DISPATCH;
    funcdesc.callconv = CC_STDCALL;
    funcdesc.elemdescFunc.tdesc.vt = VT_VOID;
    funcdesc.wFuncFlags = FUNCFLAG_FBINDABLE;

    /* put method */
    memset(&edesc, 0, sizeof(edesc));
    edesc.tdesc.vt = VT_BSTR;
    edesc.idldesc.dwReserved = 0;
    edesc.idldesc.wIDLFlags = IDLFLAG_FIN;

    funcdesc.lprgelemdescParam = &edesc;
    funcdesc.invkind = INVOKE_PROPERTYPUT;
    funcdesc.cParams = 1;

    hr = ICreateTypeInfo_AddFuncDesc(cti, 0, &funcdesc);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    /* setter name */
    hr = ICreateTypeInfo_SetFuncAndParamNames(cti, 0, propW, 1);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    /* putref method */
    funcdesc.invkind = INVOKE_PROPERTYPUTREF;
    hr = ICreateTypeInfo_AddFuncDesc(cti, 1, &funcdesc);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    /* putref name */
    hr = ICreateTypeInfo_SetFuncAndParamNames(cti, 1, propW, 1);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    funcdesc.invkind = INVOKE_PROPERTYGET;
    funcdesc.cParams = 0;
    hr = ICreateTypeInfo_AddFuncDesc(cti, 2, &funcdesc);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    /* getter name */
    hr = ICreateTypeInfo_SetFuncAndParamNames(cti, 2, propW, 1);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = ICreateTypeInfo_AddFuncDesc(cti, 3, &funcdesc);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    /* getter name again */
    hr = ICreateTypeInfo_SetFuncAndParamNames(cti, 3, propW, 1);
    ok(hr == TYPE_E_AMBIGUOUSNAME, "got 0x%08lx\n", hr);

    /* regular function */
    funcdesc.invkind = INVOKE_FUNC;
    funcdesc.cParams = 1;
    hr = ICreateTypeInfo_AddFuncDesc(cti, 4, &funcdesc);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = ICreateTypeInfo_SetFuncAndParamNames(cti, 4, funcW, 2);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    ICreateTypeInfo_Release(cti);

    hr = ICreateTypeLib2_CreateTypeInfo(ctl, name2W, TKIND_INTERFACE, &cti);
    ok(hr == S_OK, "got %08lx\n", hr);

    funcdesc.funckind = FUNC_PUREVIRTUAL;
    funcdesc.invkind = INVOKE_FUNC;
    funcdesc.cParams = 0;
    funcdesc.lprgelemdescParam = NULL;
    hr = ICreateTypeInfo_AddFuncDesc(cti, 0, &funcdesc);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = ICreateTypeInfo_SetFuncAndParamNames(cti, 0, funcW, 1);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    ICreateTypeInfo_Release(cti);

    hr = ICreateTypeLib2_QueryInterface(ctl, &IID_ITypeLib, (void**)&tl);
    ok(hr == S_OK, "got %08lx\n", hr);

    found = 1;
    memset(infos, 0, sizeof(infos));
    memids[0] = 0xdeadbeef;
    memids[1] = 0xdeadbeef;
    memids[2] = 0xdeadbeef;
    hr = ITypeLib_FindName(tl, func, 0, infos, memids, &found);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(found == 1, "got wrong count: %u\n", found);
    ok(infos[0] && !infos[1] && !infos[2], "got wrong typeinfo\n");
    ok(memids[0] == 0, "got wrong memid[0]\n");
    ok(memids[1] == 0xdeadbeef && memids[2] == 0xdeadbeef, "got wrong memids\n");
    ITypeInfo_Release(infos[0]);

    found = 3;
    memset(infos, 0, sizeof(infos));
    memids[0] = 0xdeadbeef;
    memids[1] = 0xdeadbeef;
    memids[2] = 0xdeadbeef;
    hr = ITypeLib_FindName(tl, func, 0, infos, memids, &found);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(found == 2, "got wrong count: %u\n", found);
    ok(infos[0] && infos[1] && infos[0] != infos[1], "got same typeinfo\n");
    ok(memids[0] == 0, "got wrong memid[0]\n");
    ok(memids[1] == 0, "got wrong memid[1]\n");
    ITypeInfo_Release(infos[0]);
    ITypeInfo_Release(infos[1]);

    ITypeLib_Release(tl);
    ICreateTypeLib2_Release(ctl);
    DeleteFileA(filenameA);
}

static void test_SetDocString(void)
{
    static OLECHAR nameW[] = {'n','a','m','e',0};
    static OLECHAR name2W[] = {'n','a','m','e','2',0};
    static OLECHAR doc1W[] = {'d','o','c','1',0};
    static OLECHAR doc2W[] = {'d','o','c','2',0};
    static OLECHAR var_nameW[] = {'v','a','r','n','a','m','e',0};
    CHAR filenameA[MAX_PATH];
    WCHAR filenameW[MAX_PATH];
    ICreateTypeLib2 *ctl;
    ICreateTypeInfo *cti;
    ITypeLib *tl;
    ITypeInfo *ti;
    BSTR namestr, docstr;
    VARDESC desc, *pdesc;
    FUNCDESC funcdesc, *pfuncdesc;
    HRESULT hr;
    VARIANT v;

    GetTempFileNameA(".", "tlb", 0, filenameA);
    MultiByteToWideChar(CP_ACP, 0, filenameA, -1, filenameW, MAX_PATH);

    hr = CreateTypeLib2(SYS_WIN32, filenameW, &ctl);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ICreateTypeLib2_CreateTypeInfo(ctl, nameW, TKIND_ENUM, &cti);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ICreateTypeInfo_SetVarDocString(cti, 0, doc1W);
    ok(hr == TYPE_E_ELEMENTNOTFOUND, "got %08lx\n", hr);

    hr = ICreateTypeInfo_SetVarDocString(cti, 0, NULL);
    ok(hr == E_INVALIDARG, "got %08lx\n", hr);

    memset(&desc, 0, sizeof(desc));
    desc.memid = MEMBERID_NIL;
    desc.elemdescVar.tdesc.vt = VT_INT;
    desc.varkind = VAR_CONST;

    V_VT(&v) = VT_INT;
    V_INT(&v) = 1;
    desc.lpvarValue = &v;
    hr = ICreateTypeInfo_AddVarDesc(cti, 0, &desc);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ICreateTypeInfo_SetVarName(cti, 0, NULL);
    ok(hr == E_INVALIDARG, "got %08lx\n", hr);

    hr = ICreateTypeInfo_SetVarName(cti, 1, var_nameW);
    ok(hr == TYPE_E_ELEMENTNOTFOUND, "got %08lx\n", hr);

    hr = ICreateTypeInfo_SetVarName(cti, 0, var_nameW);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ICreateTypeInfo_SetVarDocString(cti, 0, NULL);
    ok(hr == E_INVALIDARG, "got %08lx\n", hr);

    hr = ICreateTypeInfo_SetVarDocString(cti, 0, doc1W);
    ok(hr == S_OK, "got %08lx\n", hr);

    /* already set */
    hr = ICreateTypeInfo_SetVarDocString(cti, 0, doc2W);
    ok(hr == S_OK, "got %08lx\n", hr);

    /* wrong index now */
    hr = ICreateTypeInfo_SetVarDocString(cti, 1, doc1W);
    ok(hr == TYPE_E_ELEMENTNOTFOUND, "got %08lx\n", hr);

    ICreateTypeInfo_Release(cti);

    hr = ICreateTypeLib2_CreateTypeInfo(ctl, name2W, TKIND_INTERFACE, &cti);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ICreateTypeInfo_SetFuncDocString(cti, 0, doc1W);
    ok(hr == TYPE_E_ELEMENTNOTFOUND, "got %08lx\n", hr);

    hr = ICreateTypeInfo_SetFuncDocString(cti, 0, NULL);
    ok(hr == E_INVALIDARG, "got %08lx\n", hr);

    memset(&funcdesc, 0, sizeof(funcdesc));
    funcdesc.memid = MEMBERID_NIL;
    funcdesc.funckind = FUNC_PUREVIRTUAL;
    funcdesc.invkind = INVOKE_FUNC;
    funcdesc.callconv = CC_STDCALL;

    hr = ICreateTypeInfo_AddFuncDesc(cti, 0, &funcdesc);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ICreateTypeInfo_SetFuncDocString(cti, 0, doc1W);
    ok(hr == S_OK, "got %08lx\n", hr);

    ICreateTypeInfo_Release(cti);

    hr = ICreateTypeLib2_SaveAllChanges(ctl);
    ok(hr == S_OK, "got: %08lx\n", hr);

    ICreateTypeLib2_Release(ctl);

    hr = LoadTypeLib(filenameW, &tl);
    ok(hr == S_OK, "got: %08lx\n", hr);

    hr = ITypeLib_GetTypeInfo(tl, 0, &ti);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ITypeInfo_GetVarDesc(ti, 0, &pdesc);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(pdesc->memid == 0x40000000, "got wrong memid: %lx\n", pdesc->memid);
    ok(pdesc->elemdescVar.tdesc.vt == VT_INT, "got wrong vardesc type: %u\n", pdesc->elemdescVar.tdesc.vt);
    ok(pdesc->varkind == VAR_CONST, "got wrong varkind: %u\n", pdesc->varkind);
    ok(V_VT(pdesc->lpvarValue) == VT_INT, "got wrong value type: %u\n", V_VT(pdesc->lpvarValue));
    ok(V_INT(pdesc->lpvarValue) == 1, "got wrong value: 0x%x\n", V_INT(pdesc->lpvarValue));

    hr = ITypeInfo_GetDocumentation(ti, pdesc->memid, &namestr, &docstr, NULL, NULL);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(memcmp(namestr, var_nameW, sizeof(var_nameW)) == 0, "got wrong name: %s\n", wine_dbgstr_w(namestr));
    ok(memcmp(docstr, doc2W, sizeof(doc2W)) == 0, "got wrong docstring: %s\n", wine_dbgstr_w(docstr));

    SysFreeString(namestr);
    SysFreeString(docstr);

    ITypeInfo_ReleaseVarDesc(ti, pdesc);
    ITypeInfo_Release(ti);

    hr = ITypeLib_GetTypeInfo(tl, 1, &ti);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ITypeInfo_GetFuncDesc(ti, 0, &pfuncdesc);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(pfuncdesc->memid == 0x60000000, "got wrong memid: %lx\n", pfuncdesc->memid);
    ok(pfuncdesc->funckind == FUNC_PUREVIRTUAL, "got wrong funckind: %x\n", pfuncdesc->funckind);
    ok(pfuncdesc->invkind == INVOKE_FUNC, "got wrong invkind: %x\n", pfuncdesc->invkind);
    ok(pfuncdesc->callconv == CC_STDCALL, "got wrong callconv: %x\n", pfuncdesc->callconv);

    hr = ITypeInfo_GetDocumentation(ti, pfuncdesc->memid, &namestr, &docstr, NULL, NULL);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(namestr == NULL, "got wrong name: %s\n", wine_dbgstr_w(namestr));
    ok(memcmp(docstr, doc1W, sizeof(doc1W)) == 0, "got wrong docstring: %s\n", wine_dbgstr_w(docstr));

    SysFreeString(docstr);

    ITypeInfo_ReleaseFuncDesc(ti, pfuncdesc);
    ITypeInfo_Release(ti);

    ITypeLib_Release(tl);

    DeleteFileA(filenameA);
}

static void test_FindName(void)
{
    static const WCHAR invalidW[] = {'i','n','v','a','l','i','d',0};
    WCHAR buffW[100];
    MEMBERID memid;
    ITypeInfo *ti;
    ITypeLib *tl;
    HRESULT hr;
    UINT16 c;

    hr = LoadTypeLib(wszStdOle2, &tl);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = ITypeLib_FindName(tl, NULL, 0, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

    lstrcpyW(buffW, wszGUID);
    hr = ITypeLib_FindName(tl, buffW, 0, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

    c = 0;
    ti = (void*)0xdeadbeef;
    hr = ITypeLib_FindName(tl, buffW, 0, &ti, NULL, &c);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);
    ok(c == 0, "got %d\n", c);
    ok(ti == (void*)0xdeadbeef, "got %p\n", ti);

    c = 1;
    ti = (void*)0xdeadbeef;
    hr = ITypeLib_FindName(tl, buffW, 0, &ti, NULL, &c);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);
    ok(c == 1, "got %d\n", c);
    ok(ti == (void*)0xdeadbeef, "got %p\n", ti);

    c = 1;
    memid = 0;
    ti = (void*)0xdeadbeef;
    hr = ITypeLib_FindName(tl, buffW, 0, &ti, &memid, &c);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(memid == MEMBERID_NIL, "got %ld\n", memid);
    ok(!lstrcmpW(buffW, wszGUID), "got %s\n", wine_dbgstr_w(buffW));
    ok(c == 1, "got %d\n", c);
    ITypeInfo_Release(ti);

    c = 1;
    memid = 0;
    lstrcpyW(buffW, wszguid);
    ti = (void*)0xdeadbeef;
    hr = ITypeLib_FindName(tl, buffW, 0, &ti, &memid, &c);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
todo_wine {
    ok(memid == MEMBERID_NIL, "got %ld\n", memid);
    ok(!lstrcmpW(buffW, wszGUID), "got %s\n", wine_dbgstr_w(buffW));
    ok(c == 1, "got %d\n", c);
}
    if (c == 1)
        ITypeInfo_Release(ti);

    c = 1;
    memid = -1;
    lstrcpyW(buffW, invalidW);
    ti = (void*)0xdeadbeef;
    hr = ITypeLib_FindName(tl, buffW, 0, &ti, &memid, &c);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(memid == MEMBERID_NIL, "got %ld\n", memid);
    ok(!lstrcmpW(buffW, invalidW), "got %s\n", wine_dbgstr_w(buffW));
    ok(c == 0, "got %d\n", c);
    ok(ti == (void*)0xdeadbeef, "got %p\n", ti);

    ITypeLib_Release(tl);
}

static void test_TypeInfo2_GetContainingTypeLib(void)
{
    static const WCHAR test[] = {'t','e','s','t','.','t','l','b',0};
    static OLECHAR testTI[] = {'t','e','s','t','T','y','p','e','I','n','f','o',0};

    ICreateTypeLib2 *ctl2;
    ICreateTypeInfo *cti;
    ITypeInfo2 *ti2;
    ITypeLib *tl;
    UINT Index;
    HRESULT hr;

    hr = CreateTypeLib2(SYS_WIN32, test, &ctl2);
    ok_ole_success(hr, CreateTypeLib2);

    hr = ICreateTypeLib2_CreateTypeInfo(ctl2, testTI, TKIND_DISPATCH, &cti);
    ok_ole_success(hr, ICreateTypeLib2_CreateTypeInfo);

    hr = ICreateTypeInfo_QueryInterface(cti, &IID_ITypeInfo2, (void**)&ti2);
    ok_ole_success(hr, ICreateTypeInfo2_QueryInterface);

    tl = NULL;
    Index = 888;
    hr = ITypeInfo2_GetContainingTypeLib(ti2, &tl, &Index);
    ok_ole_success(hr, ITypeInfo2_GetContainingTypeLib);
    ok(tl != NULL, "ITypeInfo2_GetContainingTypeLib returned empty TypeLib\n");
    ok(Index == 0, "ITypeInfo2_GetContainingTypeLib returned Index = %u, expected 0\n", Index);
    if(tl) ITypeLib_Release(tl);

    tl = NULL;
    hr = ITypeInfo2_GetContainingTypeLib(ti2, &tl, NULL);
    ok_ole_success(hr, ITypeInfo2_GetContainingTypeLib);
    ok(tl != NULL, "ITypeInfo2_GetContainingTypeLib returned empty TypeLib\n");
    if(tl) ITypeLib_Release(tl);

    Index = 888;
    hr = ITypeInfo2_GetContainingTypeLib(ti2, NULL, &Index);
    ok_ole_success(hr, ITypeInfo2_GetContainingTypeLib);
    ok(Index == 0, "ITypeInfo2_GetContainingTypeLib returned Index = %u, expected 0\n", Index);

    hr = ITypeInfo2_GetContainingTypeLib(ti2, NULL, NULL);
    ok_ole_success(hr, ITypeInfo2_GetContainingTypeLib);

    ITypeInfo2_Release(ti2);
    ICreateTypeInfo_Release(cti);
    ICreateTypeLib2_Release(ctl2);
}

static void create_manifest_file(const char *filename, const char *manifest)
{
    HANDLE file;
    DWORD size;

    file = CreateFileA(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL, NULL);
    ok(file != INVALID_HANDLE_VALUE, "CreateFile failed: %lu\n", GetLastError());
    WriteFile(file, manifest, strlen(manifest), &size, NULL);
    CloseHandle(file);
}

static HANDLE create_actctx(const char *file)
{
    WCHAR path[MAX_PATH];
    ACTCTXW actctx;
    HANDLE handle;

    MultiByteToWideChar(CP_ACP, 0, file, -1, path, MAX_PATH);
    memset(&actctx, 0, sizeof(ACTCTXW));
    actctx.cbSize = sizeof(ACTCTXW);
    actctx.lpSource = path;

    handle = CreateActCtxW(&actctx);
    ok(handle != INVALID_HANDLE_VALUE, "handle == INVALID_HANDLE_VALUE, error %lu\n", GetLastError());

    ok(actctx.cbSize == sizeof(actctx), "actctx.cbSize=%ld\n", actctx.cbSize);
    ok(actctx.dwFlags == 0, "actctx.dwFlags=%ld\n", actctx.dwFlags);
    ok(actctx.lpSource == path, "actctx.lpSource=%p\n", actctx.lpSource);
    ok(actctx.wProcessorArchitecture == 0, "actctx.wProcessorArchitecture=%d\n", actctx.wProcessorArchitecture);
    ok(actctx.wLangId == 0, "actctx.wLangId=%d\n", actctx.wLangId);
    ok(actctx.lpAssemblyDirectory == NULL, "actctx.lpAssemblyDirectory=%p\n", actctx.lpAssemblyDirectory);
    ok(actctx.lpResourceName == NULL, "actctx.lpResourceName=%p\n", actctx.lpResourceName);
    ok(actctx.lpApplicationName == NULL, "actctx.lpApplicationName=%p\n",
       actctx.lpApplicationName);
    ok(actctx.hModule == NULL, "actctx.hModule=%p\n", actctx.hModule);

    return handle;
}

static const char manifest_dep[] =
"<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">"
"<assemblyIdentity version=\"1.2.3.4\" name=\"testdep\" type=\"win32\" processorArchitecture=\"" ARCH "\"/>"
"<file name=\"test_actctx_tlb.tlb\">"
" <typelib tlbid=\"{d96d8a3e-78b6-4c8d-8f27-059db959be8a}\" version=\"2.7\" helpdir=\"\" resourceid=\"409\""
"          flags=\"Restricted,cONTROL\""
" />"
"</file>"
"<file name=\"test_actctx_tlb2.tlb\">"
" <typelib tlbid=\"{a2cfdbd3-2bbf-4b1c-a414-5a5904e634c9}\" version=\"2.0\" helpdir=\"\" resourceid=\"409\""
"          flags=\"RESTRICTED,CONTROL\""
" />"
"</file>"
"</assembly>";

static const char manifest_main[] =
"<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">"
"<assemblyIdentity version=\"1.2.3.4\" name=\"Wine.Test\" type=\"win32\" />"
"<dependency>"
" <dependentAssembly>"
"  <assemblyIdentity type=\"win32\" name=\"testdep\" version=\"1.2.3.4\" processorArchitecture=\"" ARCH "\" />"
" </dependentAssembly>"
"</dependency>"
"</assembly>";

static void test_LoadRegTypeLib(void)
{
    LCID lcid_en = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);
    LCID lcid_ru = MAKELCID(MAKELANGID(LANG_RUSSIAN, SUBLANG_NEUTRAL), SORT_DEFAULT);
    ULONG_PTR cookie;
    TLIBATTR *attr;
    HANDLE handle;
    ITypeLib *tl;
    HRESULT hr;
    BSTR path;
    BOOL ret;

    create_manifest_file("testdep.manifest", manifest_dep);
    create_manifest_file("main.manifest", manifest_main);

    handle = create_actctx("main.manifest");
    DeleteFileA("testdep.manifest");
    DeleteFileA("main.manifest");

    /* create typelib file */
    write_typelib(1, L"test_actctx_tlb.tlb");
    write_typelib(3, L"test_actctx_tlb2.tlb");

    hr = LoadRegTypeLib(&LIBID_TestTypelib, 1, 0, LOCALE_NEUTRAL, &tl);
    ok(hr == TYPE_E_LIBNOTREGISTERED, "got 0x%08lx\n", hr);

    hr = LoadRegTypeLib(&LIBID_register_test, 1, 0, LOCALE_NEUTRAL, &tl);
    ok(hr == TYPE_E_LIBNOTREGISTERED, "got 0x%08lx\n", hr);

    hr = QueryPathOfRegTypeLib(&LIBID_TestTypelib, 2, 0, LOCALE_NEUTRAL, &path);
    ok(hr == TYPE_E_LIBNOTREGISTERED, "got 0x%08lx\n", hr);

    ret = ActivateActCtx(handle, &cookie);
    ok(ret, "ActivateActCtx failed: %lu\n", GetLastError());

    path = NULL;
    hr = QueryPathOfRegTypeLib(&LIBID_TestTypelib, 2, 0, LOCALE_NEUTRAL, &path);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    SysFreeString(path);

    path = NULL;
    hr = QueryPathOfRegTypeLib(&LIBID_TestTypelib, 2, 0, lcid_en, &path);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    SysFreeString(path);

    path = NULL;
    hr = QueryPathOfRegTypeLib(&LIBID_TestTypelib, 2, 0, lcid_ru, &path);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    SysFreeString(path);

    hr = QueryPathOfRegTypeLib(&LIBID_TestTypelib, 2, 8, LOCALE_NEUTRAL, &path);
    ok(hr == TYPE_E_LIBNOTREGISTERED || broken(hr == S_OK) /* winxp */, "got 0x%08lx\n", hr);

    path = NULL;
    hr = QueryPathOfRegTypeLib(&LIBID_TestTypelib, 2, 7, LOCALE_NEUTRAL, &path);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    SysFreeString(path);

    path = NULL;
    hr = QueryPathOfRegTypeLib(&LIBID_TestTypelib, 1, 0, LOCALE_NEUTRAL, &path);
    ok(hr == TYPE_E_LIBNOTREGISTERED || broken(hr == S_OK) /* winxp */, "got 0x%08lx\n", hr);
    SysFreeString(path);

    path = NULL;
    hr = QueryPathOfRegTypeLib(&LIBID_TestTypelib, 0xffff, 0xffff, LOCALE_NEUTRAL, &path);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    SysFreeString(path);

    /* manifest version is 2.0, actual is 1.0 */
    hr = LoadRegTypeLib(&LIBID_register_test, 1, 0, LOCALE_NEUTRAL, &tl);
    ok(hr == TYPE_E_LIBNOTREGISTERED || broken(hr == S_OK) /* winxp */, "got 0x%08lx\n", hr);
    if (hr == S_OK) ITypeLib_Release(tl);

    hr = LoadRegTypeLib(&LIBID_register_test, 2, 0, LOCALE_NEUTRAL, &tl);
    ok(hr == TYPE_E_LIBNOTREGISTERED, "got 0x%08lx\n", hr);

    /* manifest version is 2.7, actual is 2.5 */
    hr = LoadRegTypeLib(&LIBID_TestTypelib, 2, 0, LOCALE_NEUTRAL, &tl);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    if (hr == S_OK) ITypeLib_Release(tl);

    hr = LoadRegTypeLib(&LIBID_TestTypelib, 2, 1, LOCALE_NEUTRAL, &tl);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    if (hr == S_OK) ITypeLib_Release(tl);

    hr = LoadRegTypeLib(&LIBID_TestTypelib, 2, 0, lcid_en, &tl);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    if (hr == S_OK) ITypeLib_Release(tl);

    hr = LoadRegTypeLib(&LIBID_TestTypelib, 2, 0, lcid_ru, &tl);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    if (hr == S_OK) ITypeLib_Release(tl);

    hr = LoadRegTypeLib(&LIBID_TestTypelib, 2, 7, LOCALE_NEUTRAL, &tl);
    ok(hr == TYPE_E_LIBNOTREGISTERED, "got 0x%08lx\n", hr);

    hr = LoadRegTypeLib(&LIBID_TestTypelib, 2, 5, LOCALE_NEUTRAL, &tl);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = ITypeLib_GetLibAttr(tl, &attr);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    ok(attr->lcid == 0, "got %lx\n", attr->lcid);
    ok(attr->wMajorVerNum == 2, "got %d\n", attr->wMajorVerNum);
    ok(attr->wMinorVerNum == 5, "got %d\n", attr->wMinorVerNum);
    ok(attr->wLibFlags == LIBFLAG_FHASDISKIMAGE, "got %x\n", attr->wLibFlags);

    ITypeLib_ReleaseTLibAttr(tl, attr);
    ITypeLib_Release(tl);

    hr = LoadRegTypeLib(&LIBID_TestTypelib, 1, 7, LOCALE_NEUTRAL, &tl);
    ok(hr == TYPE_E_LIBNOTREGISTERED, "got 0x%08lx\n", hr);

    tl = NULL;
    hr = LoadRegTypeLib(&LIBID_TestTypelib, 0xffff, 0xffff, LOCALE_NEUTRAL, &tl);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    if (tl)
    {
        hr = ITypeLib_GetLibAttr(tl, &attr);
        ok(hr == S_OK, "got 0x%08lx\n", hr);

        ok(attr->lcid == 0, "got %lx\n", attr->lcid);
        ok(attr->wMajorVerNum == 2, "got %d\n", attr->wMajorVerNum);
        ok(attr->wMinorVerNum == 5, "got %d\n", attr->wMinorVerNum);
        ok(attr->wLibFlags == LIBFLAG_FHASDISKIMAGE, "got %x\n", attr->wLibFlags);

        ITypeLib_ReleaseTLibAttr(tl, attr);
        ITypeLib_Release(tl);
    }

    DeleteFileA("test_actctx_tlb.tlb");
    DeleteFileA("test_actctx_tlb2.tlb");

    ret = DeactivateActCtx(0, cookie);
    ok(ret, "DeactivateActCtx failed: %lu\n", GetLastError());

    ReleaseActCtx(handle);
}

#define AUX_HREF 1
#define AUX_TDESC 2
#define AUX_ADESC 3
static struct _TDATest {
    VARTYPE vt;
    ULONG size; /* -1 == typelib ptr size */
    WORD align;
    WORD align3264; /* for 32-bit typelibs loaded in 64-bit mode */
    DWORD aux;
    TYPEDESC tdesc;
    ARRAYDESC adesc;
} TDATests[] = {
    { VT_I2, 2, 2, 2 },
    { VT_I4, 4, 4, 4 },
    { VT_R4, 4, 4, 4 },
    { VT_R8, 8, 4, 8 },
    { VT_CY, 8, 4, 8 },
    { VT_DATE, 8, 4, 8 },
    { VT_BSTR, -1, 4, 8 },
    { VT_DISPATCH, -1, 4, 8 },
    { VT_ERROR, 4, 4, 4 },
    { VT_BOOL, 2, 2, 2 },
    { VT_VARIANT, 0 /* see code below */, 4, 8 },
    { VT_UNKNOWN, -1, 4, 8 },
    { VT_DECIMAL, 16, 4, 8 },
    { VT_I1, 1, 1, 1 },
    { VT_UI1, 1, 1, 1 },
    { VT_UI2, 2, 2, 2 },
    { VT_UI4, 4, 4, 4 },
    { VT_I8, 8, 4, 8 },
    { VT_UI8, 8, 4, 8 },
    { VT_INT, 4, 4, 4 },
    { VT_UINT, 4, 4, 4 },
    { VT_VOID, 0, 0, 0 },
    { VT_HRESULT, 4, 4, 4 },
    { VT_PTR, -1, 4, 8, AUX_TDESC, { { 0 }, VT_INT } },
    { VT_SAFEARRAY, -1, 4, 8, AUX_TDESC, { { 0 }, VT_INT } },
    { VT_CARRAY, 16 /* == 4 * sizeof(int) */, 4, 4, AUX_ADESC, { { 0 } }, { { { 0 }, VT_INT }, 1, { { 4, 0 } } } },
    { VT_USERDEFINED, 0, 0, 0, AUX_HREF },
    { VT_LPSTR, -1, 4, 8 },
    { VT_LPWSTR, -1, 4, 8 },
    { 0 }
};

static void testTDA(ITypeLib *tl, struct _TDATest *TDATest,
        ULONG ptr_size, HREFTYPE hreftype, ULONG href_cbSizeInstance,
        WORD href_cbAlignment, BOOL create)
{
    TYPEDESC tdesc;
    WCHAR nameW[32];
    ITypeInfo *ti;
    ICreateTypeInfo *cti;
    ICreateTypeLib2 *ctl;
    ULONG size;
    WORD alignment;
    TYPEATTR *typeattr;
    HRESULT hr;

    static const WCHAR name_fmtW[] = {'a','l','i','a','s','%','0','2','u',0};

    wsprintfW(nameW, name_fmtW, TDATest->vt);

    if(create){
        hr = ITypeLib_QueryInterface(tl, &IID_ICreateTypeLib2, (void**)&ctl);
        ok(hr == S_OK, "got %08lx\n", hr);

        hr = ICreateTypeLib2_CreateTypeInfo(ctl, nameW, TKIND_ALIAS, &cti);
        ok(hr == S_OK, "got %08lx\n", hr);

        tdesc.vt = TDATest->vt;
        if(TDATest->aux == AUX_TDESC)
            tdesc.lptdesc = &TDATest->tdesc;
        else if(TDATest->aux == AUX_ADESC)
            tdesc.lpadesc = &TDATest->adesc;
        else if(TDATest->aux == AUX_HREF)
            tdesc.hreftype = hreftype;

        hr = ICreateTypeInfo_SetTypeDescAlias(cti, &tdesc);
        ok(hr == S_OK, "for VT %u, got %08lx\n", TDATest->vt, hr);

        hr = ICreateTypeInfo_QueryInterface(cti, &IID_ITypeInfo, (void**)&ti);
        ok(hr == S_OK, "got %08lx\n", hr);

        ICreateTypeInfo_Release(cti);
        ICreateTypeLib2_Release(ctl);
    }else{
        USHORT found = 1;
        MEMBERID memid;

        hr = ITypeLib_FindName(tl, nameW, 0, &ti, &memid, &found);
        ok(hr == S_OK, "for VT %u, got %08lx\n", TDATest->vt, hr);
    }

    hr = ITypeInfo_GetTypeAttr(ti, &typeattr);
    ok(hr == S_OK, "got %08lx\n", hr);

    if(TDATest->aux == AUX_HREF){
        size = href_cbSizeInstance;
        alignment = href_cbAlignment;
    }else{
        size = TDATest->size;
        if(size == -1){
            if(create)
                size = ptr_size;
            else
                size = sizeof(void*);
        }else if(TDATest->vt == VT_VARIANT){
            if(create){
                size = sizeof(VARIANT);
                if(ptr_size != sizeof(void*))
                    size += (is_win64 ? -8 : 8);  /* 32-bit variant is 8 bytes smaller than 64-bit variant */
            }else
                size = sizeof(VARIANT);
        }
        alignment = TDATest->align;
#ifdef _WIN64
        if(!create && ptr_size != sizeof(void*))
            alignment = TDATest->align3264;
#endif
    }

    ok(typeattr->cbSizeInstance == size, "got wrong size for VT %u: 0x%lx\n", TDATest->vt, typeattr->cbSizeInstance);
    ok(typeattr->cbAlignment == alignment, "got wrong alignment for VT %u: 0x%x\n", TDATest->vt, typeattr->cbAlignment);
    ok(typeattr->tdescAlias.vt == TDATest->vt, "got wrong VT for VT %u: 0x%x\n", TDATest->vt, typeattr->tdescAlias.vt);

    switch(TDATest->aux){
    case AUX_HREF:
        ok(typeattr->tdescAlias.hreftype == hreftype, "got wrong hreftype for VT %u: 0x%lx\n", TDATest->vt, typeattr->tdescAlias.hreftype);
        break;
    case AUX_TDESC:
        ok(typeattr->tdescAlias.lptdesc->vt == TDATest->tdesc.vt, "got wrong typedesc VT for VT %u: 0x%x\n", TDATest->vt, typeattr->tdescAlias.lptdesc->vt);
        break;
    case AUX_ADESC:
        ok(typeattr->tdescAlias.lpadesc->tdescElem.vt == TDATest->adesc.tdescElem.vt, "got wrong arraydesc element VT for VT %u: 0x%x\n", TDATest->vt, typeattr->tdescAlias.lpadesc->tdescElem.vt);
        ok(typeattr->tdescAlias.lpadesc->cDims == TDATest->adesc.cDims, "got wrong arraydesc dimension count for VT %u: 0x%x\n", TDATest->vt, typeattr->tdescAlias.lpadesc->cDims);
        ok(typeattr->tdescAlias.lpadesc->rgbounds[0].cElements == TDATest->adesc.rgbounds[0].cElements, "got wrong arraydesc element count for VT %u: 0x%lx\n", TDATest->vt, typeattr->tdescAlias.lpadesc->rgbounds[0].cElements);
        ok(typeattr->tdescAlias.lpadesc->rgbounds[0].lLbound == TDATest->adesc.rgbounds[0].lLbound, "got wrong arraydesc lower bound for VT %u: 0x%lx\n", TDATest->vt, typeattr->tdescAlias.lpadesc->rgbounds[0].lLbound);
        break;
    }

    ITypeInfo_ReleaseTypeAttr(ti, typeattr);
    ITypeInfo_Release(ti);
}

static void test_SetTypeDescAlias(SYSKIND kind)
{
    CHAR filenameA[MAX_PATH];
    WCHAR filenameW[MAX_PATH];
    ITypeLib *tl;
    ICreateTypeLib2 *ctl;
    ITypeInfo *ti;
    ICreateTypeInfo *cti;
    HREFTYPE hreftype;
    TYPEATTR *typeattr;
    ULONG href_cbSizeInstance, i;
    WORD href_cbAlignment, ptr_size;
    HRESULT hr;

    static OLECHAR interfaceW[] = {'i','n','t','e','r','f','a','c','e',0};

    switch(kind){
    case SYS_WIN32:
        winetest_push_context("win32");
        ptr_size = 4;
        break;
    case SYS_WIN64:
        winetest_push_context("win64");
        ptr_size = 8;
        break;
    default:
        return;
    }

    GetTempFileNameA(".", "tlb", 0, filenameA);
    MultiByteToWideChar(CP_ACP, 0, filenameA, -1, filenameW, MAX_PATH);

    hr = CreateTypeLib2(kind, filenameW, &ctl);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ICreateTypeLib2_CreateTypeInfo(ctl, interfaceW, TKIND_INTERFACE, &cti);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ICreateTypeInfo_QueryInterface(cti, &IID_ITypeInfo, (void**)&ti);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ICreateTypeInfo_AddRefTypeInfo(cti, ti, &hreftype);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ITypeInfo_GetTypeAttr(ti, &typeattr);
    ok(hr == S_OK, "got %08lx\n", hr);

    href_cbSizeInstance = typeattr->cbSizeInstance;
    href_cbAlignment = typeattr->cbAlignment;

    ITypeInfo_ReleaseTypeAttr(ti, typeattr);

    ITypeInfo_Release(ti);
    ICreateTypeInfo_Release(cti);

    hr = ICreateTypeLib2_QueryInterface(ctl, &IID_ITypeLib, (void**)&tl);
    ok(hr == S_OK, "got %08lx\n", hr);

    for(i = 0; TDATests[i].vt; ++i)
        testTDA(tl, &TDATests[i], ptr_size, hreftype, href_cbSizeInstance, href_cbAlignment, TRUE);

    hr = ICreateTypeLib2_SaveAllChanges(ctl);
    ok(hr == S_OK, "got %08lx\n", hr);

    ITypeLib_Release(tl);
    ok(0 == ICreateTypeLib2_Release(ctl), "typelib should have been released\n");

    hr = LoadTypeLibEx(filenameW, REGKIND_NONE, &tl);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ITypeLib_GetTypeInfo(tl, 0, &ti);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ITypeInfo_GetTypeAttr(ti, &typeattr);
    ok(hr == S_OK, "got %08lx\n", hr);

    href_cbSizeInstance = typeattr->cbSizeInstance;
    href_cbAlignment = typeattr->cbAlignment;

    ITypeInfo_ReleaseTypeAttr(ti, typeattr);
    ITypeInfo_Release(ti);

    for(i = 0; TDATests[i].vt; ++i)
        testTDA(tl, &TDATests[i], ptr_size, hreftype, href_cbSizeInstance, href_cbAlignment, FALSE);

    ok(0 == ITypeLib_Release(tl), "typelib should have been released\n");

    DeleteFileA(filenameA);
    winetest_pop_context();
}

static void test_GetLibAttr(void)
{
    ULONG ref1, ref2;
    TLIBATTR *attr;
    ITypeLib *tl;
    HRESULT hr;

    hr = LoadTypeLib(wszStdOle2, &tl);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    ref1 = ITypeLib_AddRef(tl);
    ITypeLib_Release(tl);

    hr = ITypeLib_GetLibAttr(tl, &attr);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    ref2 = ITypeLib_AddRef(tl);
    ITypeLib_Release(tl);
    ok(ref2 == ref1, "got %ld, %ld\n", ref2, ref1);

    ITypeLib_ReleaseTLibAttr(tl, attr);
    ITypeLib_Release(tl);
}

static HRESULT WINAPI uk_QueryInterface(IUnknown *obj, REFIID iid, void **out)
{
    return E_NOINTERFACE;
}

static ULONG WINAPI uk_AddRef(IUnknown *obj)
{
    return 2;
}

static ULONG WINAPI uk_Release(IUnknown *obj)
{
    return 1;
}

IUnknownVtbl vt = {
    uk_QueryInterface,
    uk_AddRef,
    uk_Release,
};

IUnknown uk = {&vt};

static void test_stub(void)
{
    BOOL is_wow64 = FALSE;
    DWORD *sam_list;
    HRESULT hr;
    ITypeLib *stdole;
    ICreateTypeLib2 *ctl;
    ICreateTypeInfo *cti;
    ITypeLib *tl;
    ITypeInfo *unk, *ti;
    HREFTYPE href;
    char filenameA[MAX_PATH];
    WCHAR filenameW[MAX_PATH];
    int i;

    static const GUID libguid = {0x3b9ff02e,0x9675,0x4861,{0xb7,0x81,0xce,0xae,0xa4,0x78,0x2a,0xcc}};
    static const GUID interfaceguid = {0x3b9ff02f,0x9675,0x4861,{0xb7,0x81,0xce,0xae,0xa4,0x78,0x2a,0xcc}};
    static const GUID coclassguid = {0x3b9ff030,0x9675,0x4861,{0xb7,0x81,0xce,0xae,0xa4,0x78,0x2a,0xcc}};
    static OLECHAR interfaceW[] = {'i','n','t','e','r','f','a','c','e',0};
    static OLECHAR classW[] = {'c','l','a','s','s',0};
    static DWORD sam_list32[] = { 0, ~0 };
    static DWORD sam_list64[] = { 0, KEY_WOW64_32KEY, KEY_WOW64_64KEY, ~0 };

    if (pIsWow64Process)
        pIsWow64Process(GetCurrentProcess(), &is_wow64);
    if (is_wow64 || is_win64)
        sam_list = sam_list64;
    else
        sam_list = sam_list32;

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = LoadTypeLib(wszStdOle2, &stdole);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ITypeLib_GetTypeInfoOfGuid(stdole, &IID_IUnknown, &unk);
    ok(hr == S_OK, "got %08lx\n", hr);

    GetTempFileNameA(".", "tlb", 0, filenameA);
    MultiByteToWideChar(CP_ACP, 0, filenameA, -1, filenameW, MAX_PATH);

    hr = CreateTypeLib2(SYS_WIN32, filenameW, &ctl);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ICreateTypeLib2_SetGuid(ctl, &libguid);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ICreateTypeLib2_SetLcid(ctl, LOCALE_NEUTRAL);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ICreateTypeLib2_CreateTypeInfo(ctl, interfaceW, TKIND_INTERFACE, &cti);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ICreateTypeInfo_SetGuid(cti, &interfaceguid);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ICreateTypeInfo_SetTypeFlags(cti, TYPEFLAG_FOLEAUTOMATION);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ICreateTypeInfo_AddRefTypeInfo(cti, unk, &href);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ICreateTypeInfo_AddImplType(cti, 0, href);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ICreateTypeInfo_QueryInterface(cti, &IID_ITypeInfo, (void**)&ti);
    ok(hr == S_OK, "got %08lx\n", hr);

    ICreateTypeInfo_Release(cti);
    ITypeInfo_Release(unk);
    ITypeLib_Release(stdole);

    hr = ICreateTypeLib2_CreateTypeInfo(ctl, classW, TKIND_COCLASS, &cti);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ICreateTypeInfo_SetGuid(cti, &coclassguid);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ICreateTypeInfo_AddRefTypeInfo(cti, ti, &href);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ICreateTypeInfo_AddImplType(cti, 0, href);
    ok(hr == S_OK, "got %08lx\n", hr);

    ITypeInfo_Release(ti);
    ICreateTypeInfo_Release(cti);

    hr = ICreateTypeLib2_SaveAllChanges(ctl);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ICreateTypeLib2_QueryInterface(ctl, &IID_ITypeLib, (void**)&tl);
    ok(hr == S_OK, "got %08lx\n", hr);

    for (i = 0; sam_list[i] != ~0; i++)
    {
        IPSFactoryBuffer *factory;
        IRpcStubBuffer *base_stub;
        REGSAM side = sam_list[i];
        CLSID clsid;
        HKEY hkey;
        LONG lr;

        hr = RegisterTypeLib(tl, filenameW, NULL);
        if (hr == TYPE_E_REGISTRYACCESS)
        {
            win_skip("Insufficient privileges to register typelib in the registry\n");
            break;
        }
        ok(hr == S_OK, "got %08lx, side: %04lx\n", hr, side);

        /* SYS_WIN32 typelibs should be registered only as 32-bit */
        lr = RegOpenKeyExA(HKEY_CLASSES_ROOT, "TypeLib\\{3b9ff02e-9675-4861-b781-ceaea4782acc}\\0.0\\0\\win64", 0, KEY_READ | side, &hkey);
        ok(lr == ERROR_FILE_NOT_FOUND, "got wrong return code: %lu, side: %04lx\n", lr, side);

        lr = RegOpenKeyExA(HKEY_CLASSES_ROOT, "TypeLib\\{3b9ff02e-9675-4861-b781-ceaea4782acc}\\0.0\\0\\win32", 0, KEY_READ | side, &hkey);
        ok(lr == ERROR_SUCCESS, "got wrong return code: %lu, side: %04lx\n", lr, side);
        RegCloseKey(hkey);

        /* Simulate pre-win7 installers that create interface key on one side */
        if (side != 0)
        {
            WCHAR guidW[40];
            REGSAM opposite = side ^ (KEY_WOW64_64KEY | KEY_WOW64_32KEY);

            StringFromGUID2(&interfaceguid, guidW, ARRAY_SIZE(guidW));

            /* Delete the opposite interface key */
            lr = RegOpenKeyExA(HKEY_CLASSES_ROOT, "Interface", 0, KEY_READ | opposite, &hkey);
            ok(lr == ERROR_SUCCESS, "got wrong return code: %lu, side: %04lx\n", lr, side);
            lr = myRegDeleteTreeW(hkey, guidW, opposite);
            ok(lr == ERROR_SUCCESS, "got wrong return code: %lu, side: %04lx\n", lr, side);
            RegCloseKey(hkey);

            /* Is our side interface key affected by above operation? */
            lr = RegOpenKeyExA(HKEY_CLASSES_ROOT, "Interface\\{3b9ff02f-9675-4861-b781-ceaea4782acc}", 0, KEY_READ | side, &hkey);
            ok(lr == ERROR_SUCCESS || broken(lr == ERROR_FILE_NOT_FOUND), "got wrong return code: %lu, side: %04lx\n", lr, side);
            if (lr == ERROR_FILE_NOT_FOUND)
            {
                /* win2k3, vista, 2008 */
                win_skip("Registry reflection is enabled on this platform.\n");
                goto next;
            }
            RegCloseKey(hkey);

            /* Opposite side typelib key still exists */
            lr = RegOpenKeyExA(HKEY_CLASSES_ROOT, "TypeLib\\{3b9ff02e-9675-4861-b781-ceaea4782acc}\\0.0\\0\\win32", 0, KEY_READ | opposite, &hkey);
            ok(lr == ERROR_SUCCESS, "got wrong return code: %lu, side: %04lx\n", lr, side);
            RegCloseKey(hkey);
        }

        hr = CoGetPSClsid(&interfaceguid, &clsid);
        ok(hr == S_OK, "got: %lx, side: %04lx\n", hr, side);

        hr = CoGetClassObject(&clsid, CLSCTX_INPROC_SERVER, NULL,
                              &IID_IPSFactoryBuffer, (void **)&factory);
        ok(hr == S_OK, "got: %lx, side: %04lx\n", hr, side);

        hr = IPSFactoryBuffer_CreateStub(factory, &interfaceguid, &uk, &base_stub);
        ok(hr == S_OK, "got: %lx, side: %04lx\n", hr, side);
        IRpcStubBuffer_Release(base_stub);

        IPSFactoryBuffer_Release(factory);
    next:
        hr = UnRegisterTypeLib(&libguid, 0, 0, 0, SYS_WIN32);
        ok(hr == S_OK, "got: %lx, side: %04lx\n", hr, side);
    }

    ITypeLib_Release(tl);
    ok(0 == ICreateTypeLib2_Release(ctl), "Typelib still has references\n");

    DeleteFileW(filenameW);

    CoUninitialize();
}

static void test_dep( SYSKIND sys )
{
    HRESULT          hr;
    const WCHAR     *refFilename;
    ITypeLib        *preftLib;
    ITypeInfo       *preftInfo;
    char             filename[MAX_PATH];
    WCHAR            filenameW[MAX_PATH];
    ICreateTypeLib2 *pctLib;
    ICreateTypeInfo *pctInfo;
    ITypeLib        *ptLib;
    ITypeInfo       *ptInfo;
    ITypeInfo       *ptInfoExt = NULL;
    HREFTYPE         refType;

    static WCHAR ifacenameW[] = {'I','T','e','s','t','D','e','p',0};

    static const GUID libguid = {0xe0228f26,0x2946,0x478c,{0xb6,0x4a,0x93,0xfe,0xef,0xa5,0x05,0x32}};
    static const GUID ifaceguid = {0x394376dd,0x3bb8,0x4804,{0x8c,0xcc,0x95,0x59,0x43,0x40,0x04,0xf3}};

    winetest_push_context( "%s", sys == SYS_WIN32 ? "win32" : "win64" );

    refFilename = create_test_typelib(4);
    hr = LoadTypeLibEx(refFilename, REGKIND_NONE, &preftLib);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ITypeLib_GetTypeInfoOfGuid(preftLib, &IID_IBaseIface, &preftInfo);
    ok(hr == S_OK, "got %08lx\n", hr);

    GetTempFileNameA(".", "tlb", 0, filename);
    MultiByteToWideChar(CP_ACP, 0, filename, -1, filenameW, MAX_PATH);

    hr = CreateTypeLib2(sys, filenameW, &pctLib);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ICreateTypeLib2_SetGuid(pctLib, &libguid);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ICreateTypeLib2_SetLcid(pctLib, LOCALE_NEUTRAL);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ICreateTypeLib2_CreateTypeInfo(pctLib, ifacenameW, TKIND_INTERFACE, &pctInfo);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ICreateTypeInfo_SetGuid(pctInfo, &ifaceguid);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ICreateTypeInfo_SetTypeFlags(pctInfo, TYPEFLAG_FOLEAUTOMATION);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ICreateTypeInfo_AddRefTypeInfo(pctInfo, preftInfo, &refType);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ICreateTypeInfo_AddImplType(pctInfo, 0, refType);
    ok(hr == S_OK, "got %08lx\n", hr);

    ICreateTypeInfo_Release(pctInfo);

    hr = ICreateTypeLib2_SaveAllChanges(pctLib);
    ok(hr == S_OK, "got %08lx\n", hr);

    ICreateTypeLib2_Release(pctLib);

    ITypeInfo_Release(preftInfo);
    ITypeLib_Release(preftLib);
    DeleteFileW(refFilename);

    hr = LoadTypeLibEx(filenameW, REGKIND_NONE, &ptLib);
    ok(hr == S_OK, "got: %lx\n", hr);

    hr = ITypeLib_GetTypeInfoOfGuid(ptLib, &ifaceguid, &ptInfo);
    ok(hr == S_OK, "got: %lx\n", hr);

    hr = ITypeInfo_GetRefTypeOfImplType(ptInfo, 0, &refType);
    ok(hr == S_OK, "got: %lx\n", hr);

    hr = ITypeInfo_GetRefTypeInfo(ptInfo, refType, &ptInfoExt);
    ok(hr == TYPE_E_CANTLOADLIBRARY, "got: %lx\n", hr);

    ITypeInfo_Release(ptInfo);
    if(ptInfoExt)
        ITypeInfo_Release(ptInfoExt);
    ITypeLib_Release(ptLib);

    hr = LoadTypeLibEx(filenameW, REGKIND_NONE, &ptLib);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ITypeLib_GetTypeInfo(ptLib, 0, &ptInfo);
    ok(hr == S_OK, "GetTypeInfo failed: %08lx\n", hr);

    hr = ITypeInfo_GetRefTypeOfImplType(ptInfo, 0, &refType);
    ok(hr == S_OK, "GetRefTypeOfImplType failed: %08lx\n", hr);

    hr = ITypeInfo_GetRefTypeInfo(ptInfo, refType, &ptInfoExt);
    ok(hr == TYPE_E_CANTLOADLIBRARY, "got: %lx\n", hr);

    refFilename = create_test_typelib(4);
    hr = LoadTypeLibEx(refFilename, REGKIND_NONE, &preftLib);
    ok(hr == S_OK, "got %08lx\n", hr);

    hr = ITypeInfo_GetRefTypeInfo(ptInfo, refType, &ptInfoExt);
    ok(hr == S_OK, "got: %lx\n", hr);
    ITypeInfo_Release(ptInfoExt);

    ITypeLib_Release(preftLib);
    DeleteFileW(refFilename);

    ITypeInfo_Release(ptInfo);
    ITypeLib_Release(ptLib);

    DeleteFileW(filenameW);
    winetest_pop_context();
}

static void test_DeleteImplType(void)
{
    static OLECHAR interface1W[] = L"interface1";
    HREFTYPE hreftype, hreftype2;
    ICreateTypeInfo2 *createti2;
    ICreateTypeInfo *createti;
    ICreateTypeLib2 *createtl;
    WCHAR filenameW[MAX_PATH];
    ITypeInfo *dispti, *ti;
    ITypeLib *stdole, *tl;
    TYPEATTR *typeattr;
    HRESULT hr;
    int flags;

    hr = LoadTypeLib(L"stdole2.tlb", &stdole);
    ok(hr == S_OK, "Failed to load stdole2, hr %#lx.\n", hr);

    hr = ITypeLib_GetTypeInfoOfGuid(stdole, &IID_IDispatch, &dispti);
    ok(hr == S_OK, "Failed to get IDispatch typeinfo, hr %#lx.\n", hr);

    GetTempFileNameW(L".", L"tlb", 0, filenameW);

    hr = CreateTypeLib2(SYS_WIN32, filenameW, &createtl);
    ok(hr == S_OK, "Failed to create instance, hr %#lx.\n", hr);

    hr = ICreateTypeLib2_CreateTypeInfo(createtl, interface1W, TKIND_INTERFACE, &createti);
    ok(hr == S_OK, "Failed to create instance, hr %#lx.\n", hr);
    hr = ICreateTypeInfo_QueryInterface(createti, &IID_ICreateTypeInfo2, (void **)&createti2);
    ok(hr == S_OK, "Failed to get interface, hr %#lx.\n", hr);
    ICreateTypeInfo_Release(createti);

    hr = ICreateTypeInfo2_AddRefTypeInfo(createti2, dispti, &hreftype);
    ok(hr == S_OK, "Failed to add referenced typeinfo, hr %#lx.\n", hr);

    hr = ICreateTypeInfo2_QueryInterface(createti2, &IID_ITypeInfo, (void **)&ti);
    ok(hr == S_OK, "Failed to get interface, hr %#lx.\n", hr);

    hr = ITypeInfo_GetTypeAttr(ti, &typeattr);
    ok(hr == S_OK, "Failed to get type attr, hr %#lx.\n", hr);
    ok(!(typeattr->wTypeFlags & TYPEFLAG_FDISPATCHABLE), "Unexpected type flags %#x.\n", typeattr->wTypeFlags);
    ITypeInfo_ReleaseTypeAttr(ti, typeattr);

    hr = ICreateTypeInfo2_AddImplType(createti2, 0, hreftype);
    ok(hr == S_OK, "Failed to add impl type, hr %#lx.\n", hr);

    hr = ITypeInfo_GetTypeAttr(ti, &typeattr);
    ok(hr == S_OK, "Failed to get type attr, hr %#lx.\n", hr);
    ok(typeattr->wTypeFlags & TYPEFLAG_FDISPATCHABLE, "Unexpected type flags %#x.\n", typeattr->wTypeFlags);
    ok(typeattr->cImplTypes == 1, "Unexpected cImplTypes value.\n");
    ITypeInfo_ReleaseTypeAttr(ti, typeattr);

    /* Delete impltype, check flags. */
    hr = ICreateTypeInfo2_DeleteImplType(createti2, 0);
    ok(hr == S_OK, "Failed to delete impl type, hr %#lx.\n", hr);

    hr = ICreateTypeInfo2_DeleteImplType(createti2, 0);
    ok(hr == TYPE_E_ELEMENTNOTFOUND, "Unexpected hr %#lx.\n", hr);

    hr = ITypeInfo_GetTypeAttr(ti, &typeattr);
    ok(hr == S_OK, "Failed to get type attr, hr %#lx.\n", hr);
    ok(typeattr->wTypeFlags & TYPEFLAG_FDISPATCHABLE, "Unexpected type flags %#x.\n", typeattr->wTypeFlags);
    ok(!typeattr->cImplTypes, "Unexpected cImplTypes value.\n");
    ITypeInfo_ReleaseTypeAttr(ti, typeattr);

    hr = ITypeInfo_GetImplTypeFlags(ti, 0, &flags);
    ok(hr == TYPE_E_ELEMENTNOTFOUND, "Unexpected hr %#lx.\n", hr);

    hr = ITypeInfo_GetRefTypeOfImplType(ti, 0, &hreftype2);
    ok(hr == TYPE_E_ELEMENTNOTFOUND, "Unexpected hr %#lx.\n", hr);

    hr = ICreateTypeLib2_SaveAllChanges(createtl);
    ok(hr == S_OK, "Failed to save changes, hr %#lx.\n", hr);
    ICreateTypeLib2_Release(createtl);
    ITypeInfo_Release(ti);
    ICreateTypeInfo2_Release(createti2);

    /* Load and check typeinfo. */
    hr = LoadTypeLibEx(filenameW, REGKIND_NONE, &tl);
    ok(hr == S_OK, "Failed to load typelib, hr %#lx.\n", hr);

    hr = ITypeLib_GetTypeInfo(tl, 0, &ti);
    ok(hr == S_OK, "Failed to get typeinfo, hr %#lx.\n", hr);
    hr = ITypeInfo_GetTypeAttr(ti, &typeattr);
    ok(hr == S_OK, "Failed to get type attr, hr %#lx.\n", hr);
    ok(typeattr->wTypeFlags & TYPEFLAG_FDISPATCHABLE, "Unexpected type flags %#x.\n", typeattr->wTypeFlags);
    ok(!typeattr->cImplTypes, "Unexpected cImplTypes value.\n");
    ITypeInfo_ReleaseTypeAttr(ti, typeattr);
    ITypeInfo_Release(ti);

    ITypeLib_Release(tl);

    ITypeLib_Release(stdole);
    ITypeInfo_Release(dispti);

    DeleteFileW(filenameW);
}

static void test_DeleteFuncDesc(void)
{
    OLECHAR interface1W[] = L"interface1";
    WCHAR filenameW[MAX_PATH], temp_path[MAX_PATH];
    ICreateTypeInfo2 *createti2;
    ICreateTypeInfo *createti;
    ICreateTypeLib2 *createtl;
    FUNCDESC funcdesc;
    TYPEATTR *typeattr;
    ITypeInfo *ti;
    ITypeLib *tl;
    HRESULT hr;

    GetTempPathW(ARRAY_SIZE(temp_path), temp_path);
    GetTempFileNameW(temp_path, L"tlb", 0, filenameW);

    hr = CreateTypeLib2(SYS_WIN32, filenameW, &createtl);
    ok(hr == S_OK, "Failed to create instance, hr %#lx.\n", hr);

    hr = ICreateTypeLib2_CreateTypeInfo(createtl, interface1W, TKIND_INTERFACE, &createti);
    ok(hr == S_OK, "Failed to create instance, hr %#lx.\n", hr);
    hr = ICreateTypeInfo_QueryInterface(createti, &IID_ICreateTypeInfo2, (void **)&createti2);
    ok(hr == S_OK, "Failed to get interface, hr %#lx.\n", hr);
    ICreateTypeInfo_Release(createti);

    hr = ICreateTypeInfo2_QueryInterface(createti2, &IID_ITypeInfo, (void **)&ti);
    ok(hr == S_OK, "Failed to get typeinfo, hr %#lx.\n", hr);

    memset(&funcdesc, 0, sizeof(FUNCDESC));
    funcdesc.funckind = FUNC_PUREVIRTUAL;
    funcdesc.invkind = INVOKE_PROPERTYGET;
    funcdesc.callconv = CC_STDCALL;
    funcdesc.elemdescFunc.tdesc.vt = VT_BSTR;
    funcdesc.elemdescFunc.idldesc.wIDLFlags = IDLFLAG_NONE;

    hr = ICreateTypeInfo2_AddFuncDesc(createti2, 0, &funcdesc);
    ok(hr == S_OK, "Failed to add a funcdesc, hr %#lx.\n", hr);

    hr = ITypeInfo_GetTypeAttr(ti, &typeattr);
    ok(hr == S_OK, "Failed to get type attr, hr %#lx.\n", hr);
    ok(typeattr->cFuncs == 1, "Unexpected cFuncs %u.\n", typeattr->cFuncs);
    ITypeInfo_ReleaseTypeAttr(ti, typeattr);

    hr = ICreateTypeInfo2_DeleteFuncDesc(createti2, 1);
    ok(hr == TYPE_E_ELEMENTNOTFOUND, "Unexpected hr %#lx.\n", hr);

    hr = ICreateTypeInfo2_DeleteFuncDesc(createti2, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ITypeInfo_GetTypeAttr(ti, &typeattr);
    ok(hr == S_OK, "Failed to get type attr, hr %#lx.\n", hr);
    ok(!typeattr->cFuncs, "Unexpected cFuncs %u.\n", typeattr->cFuncs);
    ITypeInfo_ReleaseTypeAttr(ti, typeattr);

    hr = ICreateTypeLib2_SaveAllChanges(createtl);
    ok(hr == S_OK, "Failed to save changes, hr %#lx.\n", hr);
    ICreateTypeLib2_Release(createtl);
    ITypeInfo_Release(ti);
    ICreateTypeInfo2_Release(createti2);

    /* Load and check typeinfo. */
    hr = LoadTypeLibEx(filenameW, REGKIND_NONE, &tl);
    ok(hr == S_OK, "Failed to load typelib, hr %#lx.\n", hr);

    hr = ITypeLib_GetTypeInfo(tl, 0, &ti);
    ok(hr == S_OK, "Failed to get typeinfo, hr %#lx.\n", hr);
    hr = ITypeInfo_GetTypeAttr(ti, &typeattr);
    ok(hr == S_OK, "Failed to get type attr, hr %#lx.\n", hr);
    ok(!typeattr->cFuncs, "Unexpected cFuncs value.\n");
    ITypeInfo_ReleaseTypeAttr(ti, typeattr);
    ITypeInfo_Release(ti);

    ITypeLib_Release(tl);

    DeleteFileW(filenameW);
}

START_TEST(typelib)
{
    const WCHAR *filename;

    init_function_pointers();
    if (!is_win64) IsWow64Process( GetCurrentProcess(), &is_wow64 );

    ref_count_test(wszStdOle2);
    test_TypeComp();
    test_CreateDispTypeInfo();
    test_TypeInfo();
    test_DispCallFunc();
    test_QueryPathOfRegTypeLib(32);
    test_QueryPathOfRegTypeLib(64);
    test_CreateTypeLib(SYS_WIN64);
    test_SetTypeDescAlias(SYS_WIN64);
    test_dep(SYS_WIN64);
    test_CreateTypeLib(SYS_WIN32);
    test_SetTypeDescAlias(SYS_WIN32);
    test_dep(SYS_WIN32);
    test_inheritance();
    test_SetVarHelpContext();
    test_SetFuncAndParamNames();
    test_SetDocString();
    test_FindName();

    if ((filename = create_test_typelib(2)))
    {
        test_dump_typelib( filename );
        DeleteFileW( filename );
    }

    test_register_typelib(TRUE);
    test_register_typelib(FALSE);
    test_register_typelib_64();
    test_create_typelibs();
    test_LoadTypeLib();
    test_TypeInfo2_GetContainingTypeLib();
    test_LoadRegTypeLib();
    test_GetLibAttr();
    test_stub();
    test_DeleteImplType();
    test_DeleteFuncDesc();
}
