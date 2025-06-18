/*
 * Copyright 2016 Nikolay Sivov for CodeWeavers
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

#include <initguid.h>
#include <ole2.h>
#include <olectl.h>
#include "dispex.h"
#include "activscp.h"
#include "activdbg.h"
#include "objsafe.h"

#include "msscript.h"
#include "wine/test.h"
DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);

#define TESTSCRIPT_CLSID "{178fc164-f585-4e24-9c13-4bb7faf80746}"
static const GUID CLSID_TestScript =
    {0x178fc164,0xf585,0x4e24,{0x9c,0x13,0x4b,0xb7,0xfa,0xf8,0x07,0x46}};

#ifdef _WIN64

#define CTXARG_T DWORDLONG
#define IActiveScriptParseVtbl IActiveScriptParse64Vtbl
#define IActiveScriptSiteDebug_Release IActiveScriptSiteDebug64_Release

#else

#define CTXARG_T DWORD
#define IActiveScriptParseVtbl IActiveScriptParse32Vtbl
#define IActiveScriptSiteDebug_Release IActiveScriptSiteDebug32_Release

#endif

#define DEFINE_EXPECT(func) \
    static BOOL expect_ ## func = FALSE, called_ ## func = FALSE

#define SET_EXPECT(func) \
    do { called_ ## func = FALSE; expect_ ## func = TRUE; } while(0)

#define CHECK_EXPECT2(func) \
    do { \
        ok(expect_ ##func, "unexpected call " #func "\n"); \
        called_ ## func = TRUE; \
    }while(0)

#define CHECK_EXPECT(func) \
    do { \
        CHECK_EXPECT2(func); \
        expect_ ## func = FALSE; \
    }while(0)

#define CHECK_CALLED(func) \
    do { \
        ok(called_ ## func, "expected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

#define CHECK_CALLED_BROKEN(func) \
    do { \
        ok(called_ ## func || broken(!called_ ## func), "expected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

#define CHECK_NOT_CALLED(func) \
    do { \
        ok(!called_ ## func, "unexpected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

#define CLEAR_CALLED(func) \
    expect_ ## func = called_ ## func = FALSE

DEFINE_EXPECT(CreateInstance);
DEFINE_EXPECT(SetInterfaceSafetyOptions);
DEFINE_EXPECT(SetInterfaceSafetyOptions_UseSafeSubset);
DEFINE_EXPECT(InitNew);
DEFINE_EXPECT(Close);
DEFINE_EXPECT(Bind);
DEFINE_EXPECT(QI_ITypeComp);
DEFINE_EXPECT(GetTypeAttr);
DEFINE_EXPECT(GetNames);
DEFINE_EXPECT(GetFuncDesc);
DEFINE_EXPECT(ReleaseTypeAttr);
DEFINE_EXPECT(ReleaseFuncDesc);
DEFINE_EXPECT(ReleaseVarDesc);
DEFINE_EXPECT(QI_IDispatchEx);
DEFINE_EXPECT(GetTypeInfo);
DEFINE_EXPECT(GetIDsOfNames);
DEFINE_EXPECT(Invoke);
DEFINE_EXPECT(InvokeEx);
DEFINE_EXPECT(DeferredFillIn);
DEFINE_EXPECT(GetExceptionInfo);
DEFINE_EXPECT(GetSourcePosition);
DEFINE_EXPECT(GetSourceLineText);
DEFINE_EXPECT(SetScriptSite);
DEFINE_EXPECT(QI_IActiveScriptParse);
DEFINE_EXPECT(GetScriptState);
DEFINE_EXPECT(SetScriptState_INITIALIZED);
DEFINE_EXPECT(SetScriptState_STARTED);
DEFINE_EXPECT(SetScriptState_CONNECTED);
DEFINE_EXPECT(ParseScriptText);
DEFINE_EXPECT(AddNamedItem);
DEFINE_EXPECT(GetScriptDispatch);

#define EXPECT_REF(obj,ref) _expect_ref((IUnknown*)obj, ref, __LINE__)
static void _expect_ref(IUnknown* obj, ULONG ref, int line)
{
    ULONG rc;
    IUnknown_AddRef(obj);
    rc = IUnknown_Release(obj);
    ok_(__FILE__,line)(rc == ref, "expected refcount %ld, got %ld\n", ref, rc);
}

static IActiveScriptSite *site;
static SCRIPTSTATE state;
static ITypeInfo TypeInfo;

static struct
{
    const WCHAR *name;
    SHORT num_args;
    SHORT num_opt_args;
    VARTYPE ret_type;
    FUNCKIND func_kind;
    INVOKEKIND invoke_kind;
    WORD flags;
} custom_engine_funcs[] =
{
    { L"foobar",    3,  0, VT_I4,     FUNC_DISPATCH,    INVOKE_FUNC,           0 },
    { L"barfoo",   11,  2, VT_VOID,   FUNC_VIRTUAL,     INVOKE_FUNC,           FUNCFLAG_FRESTRICTED },
    { L"empty",     0,  0, VT_EMPTY,  FUNC_PUREVIRTUAL, INVOKE_PROPERTYGET,    FUNCFLAG_FBINDABLE | FUNCFLAG_FDISPLAYBIND },
    { L"vararg",    4, -1, VT_BSTR,   FUNC_NONVIRTUAL,  INVOKE_PROPERTYPUT,    FUNCFLAG_FREQUESTEDIT },
    { L"static",    0,  1, VT_PTR,    FUNC_STATIC,      INVOKE_PROPERTYPUTREF, FUNCFLAG_FHIDDEN },
    { L"deadbeef", 21, -9, VT_ERROR,  0xdeadbeef,       0xdeadbeef,            0xffff }
};

static int memid_to_func_index(MEMBERID memid)
{
    UINT idx = memid - 0xdeadbeef;
    return idx < ARRAY_SIZE(custom_engine_funcs) ? idx : -1;
}

static MEMBERID func_index_to_memid(UINT idx)
{
    return idx + 0xdeadbeef;
}

static FUNCDESC *get_func_desc(UINT i)
{
    FUNCDESC *desc;

    if (!(desc = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*desc))))
        return NULL;

    desc->memid = func_index_to_memid(i);
    desc->cParams = custom_engine_funcs[i].num_args;
    desc->cParamsOpt = custom_engine_funcs[i].num_opt_args;
    desc->elemdescFunc.tdesc.vt = custom_engine_funcs[i].ret_type;
    desc->elemdescFunc.paramdesc.wParamFlags = PARAMFLAG_FRETVAL;
    desc->funckind = custom_engine_funcs[i].func_kind;
    desc->invkind = custom_engine_funcs[i].invoke_kind;
    desc->wFuncFlags = custom_engine_funcs[i].flags;
    return desc;
}

static HRESULT WINAPI ActiveScriptParse_QueryInterface(IActiveScriptParse *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;
    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI ActiveScriptParse_AddRef(IActiveScriptParse *iface)
{
    return 2;
}

static ULONG WINAPI ActiveScriptParse_Release(IActiveScriptParse *iface)
{
    return 1;
}

static HRESULT WINAPI ActiveScriptParse_InitNew(IActiveScriptParse *iface)
{
    CHECK_EXPECT(InitNew);
    return S_OK;
}

static HRESULT WINAPI ActiveScriptParse_AddScriptlet(IActiveScriptParse *iface,
        LPCOLESTR pstrDefaultName, LPCOLESTR pstrCode, LPCOLESTR pstrItemName,
        LPCOLESTR pstrSubItemName, LPCOLESTR pstrEventName, LPCOLESTR pstrDelimiter,
        CTXARG_T dwSourceContextCookie, ULONG ulStartingLineNumber, DWORD dwFlags,
        BSTR *pbstrName, EXCEPINFO *pexcepinfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const WCHAR *parse_item_name;
static DWORD parse_flags = 0;

static HRESULT WINAPI ActiveScriptParse_ParseScriptText(IActiveScriptParse *iface,
        LPCOLESTR pstrCode, LPCOLESTR pstrItemName, IUnknown *punkContext,
        LPCOLESTR pstrDelimiter, CTXARG_T dwSourceContextCookie, ULONG ulStartingLine,
        DWORD dwFlags, VARIANT *pvarResult, EXCEPINFO *pexcepinfo)
{
    ok(!!pstrCode, "got wrong pointer: %p.\n", pstrCode);
    ok(!lstrcmpW(pstrItemName, parse_item_name), "got wrong item name: %s (expected %s).\n",
        wine_dbgstr_w(pstrItemName), wine_dbgstr_w(parse_item_name));
    ok(!punkContext, "got wrong pointer: %p.\n", punkContext);
    ok(!pstrDelimiter, "got wrong pointer: %p.\n", pstrDelimiter);
    ok(!dwSourceContextCookie, "got wrong value: %s.\n", wine_dbgstr_longlong(dwSourceContextCookie));
    ok(ulStartingLine == 1, "got wrong value: %ld.\n", ulStartingLine);
    ok(!!pexcepinfo, "got wrong pointer: %p.\n", pexcepinfo);
    ok(dwFlags == parse_flags, "got wrong flags: %lx.\n", dwFlags);
    if (parse_flags == SCRIPTTEXT_ISEXPRESSION)
        ok(!!pvarResult, "got wrong pointer: %p.\n", pvarResult);
    else
        ok(!pvarResult, "got wrong pointer: %p.\n", pvarResult);

    CHECK_EXPECT(ParseScriptText);
    return S_OK;
}

static const IActiveScriptParseVtbl ActiveScriptParseVtbl = {
    ActiveScriptParse_QueryInterface,
    ActiveScriptParse_AddRef,
    ActiveScriptParse_Release,
    ActiveScriptParse_InitNew,
    ActiveScriptParse_AddScriptlet,
    ActiveScriptParse_ParseScriptText
};

static IActiveScriptParse ActiveScriptParse = { &ActiveScriptParseVtbl };

static HRESULT WINAPI ObjectSafety_QueryInterface(IObjectSafety *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;
    ok(0, "unexpected call %s\n", wine_dbgstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI ObjectSafety_AddRef(IObjectSafety *iface)
{
    return 2;
}

static ULONG WINAPI ObjectSafety_Release(IObjectSafety *iface)
{
    return 1;
}

static HRESULT WINAPI ObjectSafety_GetInterfaceSafetyOptions(IObjectSafety *iface, REFIID riid,
        DWORD *pdwSupportedOptions, DWORD *pdwEnabledOptions)
{
    ok(0, "unexpected riid %s\n", wine_dbgstr_guid(riid));
    return E_NOTIMPL;
}

static HRESULT WINAPI ObjectSafety_SetInterfaceSafetyOptions(IObjectSafety *iface, REFIID riid,
        DWORD mask, DWORD options)
{
    if (options == INTERFACESAFE_FOR_UNTRUSTED_DATA)
        CHECK_EXPECT(SetInterfaceSafetyOptions_UseSafeSubset);
    else
        CHECK_EXPECT(SetInterfaceSafetyOptions);

    ok(IsEqualGUID(&IID_IActiveScriptParse, riid), "unexpected riid %s\n", wine_dbgstr_guid(riid));
    ok(mask == INTERFACESAFE_FOR_UNTRUSTED_DATA, "option mask = %lx\n", mask);

    return S_OK;
}

static const IObjectSafetyVtbl ObjectSafetyVtbl = {
    ObjectSafety_QueryInterface,
    ObjectSafety_AddRef,
    ObjectSafety_Release,
    ObjectSafety_GetInterfaceSafetyOptions,
    ObjectSafety_SetInterfaceSafetyOptions
};

static IObjectSafety ObjectSafety = { &ObjectSafetyVtbl };

static HRESULT WINAPI TypeComp_QueryInterface(ITypeComp *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;
    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI TypeComp_AddRef(ITypeComp *iface)
{
    return 2;
}

static ULONG WINAPI TypeComp_Release(ITypeComp *iface)
{
    return 1;
}

static HRESULT WINAPI TypeComp_Bind(ITypeComp *iface, LPOLESTR szName, ULONG lHashVal, WORD wFlags,
        ITypeInfo **ppTInfo, DESCKIND *pDescKind, BINDPTR *pBindPtr)
{
    ULONG hash = LHashValOfNameSys(sizeof(void*) == 8 ? SYS_WIN64 : SYS_WIN32, LOCALE_USER_DEFAULT, szName);
    UINT i;

    CHECK_EXPECT(Bind);
    ok(lHashVal == hash, "wrong hash, expected 0x%08lx, got 0x%08lx.\n", hash, lHashVal);
    ok(wFlags == INVOKE_FUNC, "wrong flags, got 0x%x.\n", wFlags);

    *ppTInfo = NULL;
    *pDescKind = DESCKIND_NONE;
    pBindPtr->lptcomp = NULL;

    if (!lstrcmpW(szName, L"type_mismatch"))
        return TYPE_E_TYPEMISMATCH;

    if (!lstrcmpW(szName, L"variable"))
    {
        if (!(pBindPtr->lpvardesc = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(VARDESC))))
            return E_OUTOFMEMORY;
        *ppTInfo = &TypeInfo;
        *pDescKind = DESCKIND_VARDESC;
        return S_OK;
    }

    for (i = 0; i < ARRAY_SIZE(custom_engine_funcs); i++)
    {
        if (!lstrcmpW(szName, custom_engine_funcs[i].name))
        {
            *ppTInfo = &TypeInfo;
            *pDescKind = DESCKIND_FUNCDESC;
            pBindPtr->lpfuncdesc = get_func_desc(i);
            return S_OK;
        }
    }

    return S_OK;
}

static HRESULT WINAPI TypeComp_BindType(ITypeComp *iface, LPOLESTR szName, ULONG lHashVal,
        ITypeInfo **ppTInfo, ITypeComp **ppTComp)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const ITypeCompVtbl TypeCompVtbl = {
    TypeComp_QueryInterface,
    TypeComp_AddRef,
    TypeComp_Release,
    TypeComp_Bind,
    TypeComp_BindType
};

static ITypeComp TypeComp = { &TypeCompVtbl };

static BOOL TypeComp_available = FALSE;
static HRESULT WINAPI TypeInfo_QueryInterface(ITypeInfo *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if (IsEqualGUID(&IID_ITypeComp, riid))
    {
        CHECK_EXPECT(QI_ITypeComp);
        if (TypeComp_available)
        {
            *ppv = &TypeComp;
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    ok(0, "unexpected riid %s\n", wine_dbgstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI TypeInfo_AddRef(ITypeInfo *iface)
{
    return 2;
}

static ULONG WINAPI TypeInfo_Release(ITypeInfo *iface)
{
    return 1;
}

static UINT TypeInfo_GetTypeAttr_cFuncs;
static HRESULT WINAPI TypeInfo_GetTypeAttr(ITypeInfo *iface, TYPEATTR **ppTypeAttr)
{
    CHECK_EXPECT(GetTypeAttr);

    if (!(*ppTypeAttr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(TYPEATTR))))
        return E_OUTOFMEMORY;

    (*ppTypeAttr)->cFuncs = TypeInfo_GetTypeAttr_cFuncs;
    return S_OK;
}

static HRESULT WINAPI TypeInfo_GetTypeComp(ITypeInfo *iface, ITypeComp **ppTComp)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI TypeInfo_GetFuncDesc(ITypeInfo *iface, UINT index, FUNCDESC **ppFuncDesc)
{
    CHECK_EXPECT(GetFuncDesc);

    if (index >= ARRAY_SIZE(custom_engine_funcs))
        return E_INVALIDARG;

    *ppFuncDesc = get_func_desc(index);
    return S_OK;
}

static HRESULT WINAPI TypeInfo_GetVarDesc(ITypeInfo *iface, UINT index, VARDESC **ppVarDesc)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI TypeInfo_GetNames(ITypeInfo *iface, MEMBERID memid, BSTR *rgBstrNames,
        UINT cMaxNames, UINT *pcNames)
{
    int idx;

    CHECK_EXPECT(GetNames);
    ok(cMaxNames == 1, "unexpected cMaxNames, got %u.\n", cMaxNames);
    ok(rgBstrNames != NULL, "rgBstrNames is NULL.\n");
    ok(pcNames != NULL, "pcNames is NULL.\n");

    idx = memid_to_func_index(memid);
    if (idx != -1)
    {
        *rgBstrNames = SysAllocString(custom_engine_funcs[idx].name);
        *pcNames = 1;
        return S_OK;
    }

    *pcNames = 0;
    return TYPE_E_ELEMENTNOTFOUND;
}

static HRESULT WINAPI TypeInfo_GetRefTypeOfImplType(ITypeInfo *iface, UINT index, HREFTYPE *pRefType)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI TypeInfo_GetImplTypeFlags(ITypeInfo *iface, UINT index, INT *pImplTypeFlags)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI TypeInfo_GetIDsOfNames(ITypeInfo *iface, LPOLESTR *rgszNames, UINT cNames,
        MEMBERID *pMemId)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI TypeInfo_Invoke(ITypeInfo *iface, PVOID pvInstance, MEMBERID memid, WORD wFlags,
        DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI TypeInfo_GetDocumentation(ITypeInfo *iface, MEMBERID memid, BSTR *pBstrName,
        BSTR *pBstrDocString, DWORD *pdwHelpContext, BSTR *pBstrHelpFile)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI TypeInfo_GetDllEntry(ITypeInfo *iface, MEMBERID memid, INVOKEKIND invKind,
        BSTR *pBstrDllName, BSTR *pBstrName, WORD *pwOrdinal)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI TypeInfo_GetRefTypeInfo(ITypeInfo *iface, HREFTYPE hRefType, ITypeInfo **ppTInfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI TypeInfo_AddressOfMember(ITypeInfo *iface, MEMBERID memid, INVOKEKIND invKind, PVOID *ppv)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI TypeInfo_CreateInstance(ITypeInfo *iface, IUnknown *pUnkOuter, REFIID riid, PVOID *ppvObj)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI TypeInfo_GetMops(ITypeInfo *iface, MEMBERID memid, BSTR *pBstrMops)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI TypeInfo_GetContainingTypeLib(ITypeInfo *iface, ITypeLib **ppTLib, UINT *pIndex)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static void WINAPI TypeInfo_ReleaseTypeAttr(ITypeInfo *iface, TYPEATTR *pTypeAttr)
{
    CHECK_EXPECT(ReleaseTypeAttr);
    HeapFree(GetProcessHeap(), 0, pTypeAttr);
}

static void WINAPI TypeInfo_ReleaseFuncDesc(ITypeInfo *iface, FUNCDESC *pFuncDesc)
{
    CHECK_EXPECT(ReleaseFuncDesc);
    HeapFree(GetProcessHeap(), 0, pFuncDesc);
}

static void WINAPI TypeInfo_ReleaseVarDesc(ITypeInfo *iface, VARDESC *pVarDesc)
{
    CHECK_EXPECT(ReleaseVarDesc);
    HeapFree(GetProcessHeap(), 0, pVarDesc);
}

static const ITypeInfoVtbl TypeInfoVtbl = {
    TypeInfo_QueryInterface,
    TypeInfo_AddRef,
    TypeInfo_Release,
    TypeInfo_GetTypeAttr,
    TypeInfo_GetTypeComp,
    TypeInfo_GetFuncDesc,
    TypeInfo_GetVarDesc,
    TypeInfo_GetNames,
    TypeInfo_GetRefTypeOfImplType,
    TypeInfo_GetImplTypeFlags,
    TypeInfo_GetIDsOfNames,
    TypeInfo_Invoke,
    TypeInfo_GetDocumentation,
    TypeInfo_GetDllEntry,
    TypeInfo_GetRefTypeInfo,
    TypeInfo_AddressOfMember,
    TypeInfo_CreateInstance,
    TypeInfo_GetMops,
    TypeInfo_GetContainingTypeLib,
    TypeInfo_ReleaseTypeAttr,
    TypeInfo_ReleaseFuncDesc,
    TypeInfo_ReleaseVarDesc
};

static ITypeInfo TypeInfo = { &TypeInfoVtbl };

static BOOL DispatchEx_available = FALSE;
static HRESULT WINAPI DispatchEx_QueryInterface(IDispatchEx *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if (IsEqualGUID(&IID_IDispatchEx, riid))
    {
        CHECK_EXPECT(QI_IDispatchEx);
        if (DispatchEx_available)
        {
            *ppv = iface;
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    ok(0, "unexpected riid %s\n", wine_dbgstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI DispatchEx_AddRef(IDispatchEx *iface)
{
    return 2;
}

static ULONG WINAPI DispatchEx_Release(IDispatchEx *iface)
{
    return 1;
}

static HRESULT WINAPI DispatchEx_GetTypeInfoCount(IDispatchEx *iface, UINT *pctinfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetTypeInfo(IDispatchEx *iface, UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    CHECK_EXPECT(GetTypeInfo);
    ok(iTInfo == 0, "unexpected iTInfo %u.\n", iTInfo);
    ok(lcid == LOCALE_USER_DEFAULT, "unexpected lcid %lu\n", lcid);

    *ppTInfo = &TypeInfo;
    return S_OK;
}

static BSTR Dispatch_expected_name;
static HRESULT WINAPI DispatchEx_GetIDsOfNames(IDispatchEx *iface, REFIID riid, LPOLESTR *rgszNames,
        UINT cNames, LCID lcid, DISPID *rgDispId)
{
    CHECK_EXPECT(GetIDsOfNames);
    ok(IsEqualGUID(&IID_NULL, riid), "unexpected riid %s\n", wine_dbgstr_guid(riid));
    ok(lcid == LOCALE_USER_DEFAULT, "unexpected lcid %lu\n", lcid);
    ok(cNames == 1, "unexpected cNames %u\n", cNames);
    ok(Dispatch_expected_name && !lstrcmpW(rgszNames[0], Dispatch_expected_name),
        "unexpected name %s (expected %s).\n", wine_dbgstr_w(rgszNames[0]), wine_dbgstr_w(Dispatch_expected_name));

    *rgDispId = 0xdeadbeef;
    return S_OK;
}

static HRESULT WINAPI DispatchEx_Invoke(IDispatchEx *iface, DISPID dispIdMember, REFIID riid, LCID lcid,
        WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    CHECK_EXPECT(Invoke);
    ok(IsEqualGUID(&IID_NULL, riid), "unexpected riid %s.\n", wine_dbgstr_guid(riid));
    ok(lcid == LOCALE_USER_DEFAULT, "unexpected lcid %lu.\n", lcid);
    ok(wFlags == DISPATCH_METHOD, "unexpected wFlags %u.\n", wFlags);
    ok(dispIdMember == 0xdeadbeef, "unexpected dispIdMember %ld.\n", dispIdMember);
    ok(pDispParams->cNamedArgs == 0, "unexpected number of named args %u.\n", pDispParams->cNamedArgs);
    ok(!pDispParams->rgdispidNamedArgs, "unexpected named args array %p\n", pDispParams->rgdispidNamedArgs);
    ok(pDispParams->cArgs == 2, "unexpected number of args %u.\n", pDispParams->cArgs);
    ok(!!pDispParams->rgvarg, "unexpected NULL rgvarg.\n");
    if (pDispParams->rgvarg && pDispParams->cArgs >= 2)
    {
        ok(V_VT(pDispParams->rgvarg + 1) == VT_I4 && V_I4(pDispParams->rgvarg + 1) == 10,
            "unexpected first parameter V_VT = %d, V_I4 = %ld.\n",
            V_VT(pDispParams->rgvarg + 1), V_I4(pDispParams->rgvarg + 1));
        ok(V_VT(pDispParams->rgvarg) == VT_I4 && V_I4(pDispParams->rgvarg) == 3,
            "unexpected second parameter V_VT = %d, V_I4 = %ld.\n",
            V_VT(pDispParams->rgvarg), V_I4(pDispParams->rgvarg));
    }

    V_VT(pVarResult) = VT_R8;
    V_R8(pVarResult) = 4.2;
    return S_OK;
}

static HRESULT WINAPI DispatchEx_GetDispID(IDispatchEx *iface, BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_InvokeEx(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags,
        DISPPARAMS *pdp, VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    IServiceProvider *sp;
    IUnknown *unk;
    HRESULT hr;

    CHECK_EXPECT(InvokeEx);
    ok(lcid == LOCALE_USER_DEFAULT, "unexpected lcid %lu.\n", lcid);
    ok(wFlags == DISPATCH_METHOD, "unexpected wFlags %u.\n", wFlags);
    ok(id == 0xdeadbeef, "unexpected id %ld.\n", id);
    ok(pdp->cNamedArgs == 0, "unexpected number of named args %u.\n", pdp->cNamedArgs);
    ok(!pdp->rgdispidNamedArgs, "unexpected named args array %p.\n", pdp->rgdispidNamedArgs);
    ok(pdp->cArgs == 2, "unexpected number of args %u.\n", pdp->cArgs);
    ok(!!pdp->rgvarg, "unexpected NULL rgvarg.\n");
    if (pdp->rgvarg && pdp->cArgs >= 2)
    {
        ok(V_VT(pdp->rgvarg + 1) == VT_I4 && V_I4(pdp->rgvarg + 1) == 10,
            "unexpected first parameter V_VT = %d, V_I4 = %ld.\n",
            V_VT(pdp->rgvarg + 1), V_I4(pdp->rgvarg + 1));
        ok(V_VT(pdp->rgvarg) == VT_I4 && V_I4(pdp->rgvarg) == 3,
            "unexpected second parameter V_VT = %d, V_I4 = %ld.\n",
            V_VT(pdp->rgvarg), V_I4(pdp->rgvarg));
    }
    ok(!!pspCaller, "unexpected NULL pspCaller.\n");

    hr = IActiveScriptSite_QueryInterface(site, &IID_IServiceProvider, (void**)&sp);
    ok(hr == S_OK, "Failed to retrieve IID_IServiceProvider from script site: 0x%08lx.\n", hr);
    ok(sp != pspCaller, "Same IServiceProvider objects.\n");
    IServiceProvider_Release(sp);

    hr = IServiceProvider_QueryInterface(pspCaller, &IID_IActiveScriptSite, (void**)&unk);
    ok(hr == E_NOINTERFACE, "QueryInterface IActiveScriptSite returned: 0x%08lx.\n", hr);

    unk = (IUnknown*)0xdeadbeef;
    hr = IServiceProvider_QueryService(pspCaller, &SID_GetCaller, NULL, (void**)&unk);
    ok(hr == S_OK, "QueryService failed: 0x%08lx.\n", hr);
    ok(!unk, "unexpected object returned %p.\n", unk);
    unk = (IUnknown*)0xdeadbeef;
    hr = IServiceProvider_QueryService(pspCaller, &SID_GetCaller, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "QueryService failed: 0x%08lx.\n", hr);
    ok(!unk, "unexpected object returned %p.\n", unk);
    sp = (IServiceProvider*)0xdeadbeef;
    hr = IServiceProvider_QueryService(pspCaller, &SID_GetCaller, &IID_IServiceProvider, (void**)&sp);
    ok(hr == S_OK, "QueryService failed: 0x%08lx.\n", hr);
    ok(!sp, "unexpected object returned %p.\n", sp);
    unk = (IUnknown*)0xdeadbeef;
    hr = IServiceProvider_QueryService(pspCaller, &SID_VariantConversion, &IID_IVariantChangeType, (void**)&unk);
    ok(hr == E_NOINTERFACE, "QueryService returned: 0x%08lx.\n", hr);
    ok(!unk, "unexpected object returned %p.\n", unk);

    V_VT(pvarRes) = VT_I2;
    V_I2(pvarRes) = 42;
    return S_OK;
}

static HRESULT WINAPI DispatchEx_DeleteMemberByName(IDispatchEx *iface, BSTR bstrName, DWORD grfdex)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_DeleteMemberByDispID(IDispatchEx *iface, DISPID id)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetMemberProperties(IDispatchEx *iface, DISPID id, DWORD grfdexFetch,
        DWORD *pgrfdex)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetMemberName(IDispatchEx *iface, DISPID id, BSTR *pbstrName)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetNextDispID(IDispatchEx *iface, DWORD grfdex, DISPID id, DISPID *pid)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetNameSpaceParent(IDispatchEx *iface, IUnknown **ppunk)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IDispatchExVtbl DispatchExVtbl = {
    DispatchEx_QueryInterface,
    DispatchEx_AddRef,
    DispatchEx_Release,
    DispatchEx_GetTypeInfoCount,
    DispatchEx_GetTypeInfo,
    DispatchEx_GetIDsOfNames,
    DispatchEx_Invoke,
    DispatchEx_GetDispID,
    DispatchEx_InvokeEx,
    DispatchEx_DeleteMemberByName,
    DispatchEx_DeleteMemberByDispID,
    DispatchEx_GetMemberProperties,
    DispatchEx_GetMemberName,
    DispatchEx_GetNextDispID,
    DispatchEx_GetNameSpaceParent
};

static IDispatchEx DispatchEx = { &DispatchExVtbl };

static HRESULT WINAPI ActiveScript_QueryInterface(IActiveScript *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IActiveScript, riid)) {
        *ppv = iface;
        return S_OK;
    }

    if(IsEqualGUID(&IID_IObjectSafety, riid)) {
        *ppv = &ObjectSafety;
        return S_OK;
    }

    if(IsEqualGUID(&IID_IActiveScriptParse, riid)) {
        CHECK_EXPECT(QI_IActiveScriptParse);
        *ppv = &ActiveScriptParse;
        return S_OK;
    }

    if(IsEqualGUID(&IID_IActiveScriptGarbageCollector, riid))
        return E_NOINTERFACE;

    ok(0, "unexpected riid %s\n", wine_dbgstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI ActiveScript_AddRef(IActiveScript *iface)
{
    return 2;
}

static ULONG WINAPI ActiveScript_Release(IActiveScript *iface)
{
    return 1;
}

static HRESULT WINAPI ActiveScript_SetScriptSite(IActiveScript *iface, IActiveScriptSite *pass)
{
    IActiveScriptSiteInterruptPoll *poll;
    IActiveScriptSiteWindow *window;
    IActiveScriptSiteDebug *debug;
    IServiceProvider *service;
    ICanHandleException *canexception;
    LCID lcid;
    HRESULT hres;

    CHECK_EXPECT(SetScriptSite);

    ok(pass != NULL, "pass == NULL\n");

    hres = IActiveScriptSite_QueryInterface(pass, &IID_IActiveScriptSiteInterruptPoll, (void**)&poll);
    ok(hres == E_NOINTERFACE, "Got IActiveScriptSiteInterruptPoll interface: %08lx\n", hres);

    hres = IActiveScriptSite_GetLCID(pass, &lcid);
    ok(hres == S_OK, "GetLCID failed: %08lx\n", hres);

    hres = IActiveScriptSite_OnStateChange(pass, (state = SCRIPTSTATE_INITIALIZED));
    ok(hres == E_NOTIMPL, "OnStateChange failed: %08lx\n", hres);

    hres = IActiveScriptSite_QueryInterface(pass, &IID_IActiveScriptSiteDebug, (void**)&debug);
    ok(hres == E_NOINTERFACE, "Got IActiveScriptSiteDebug interface: %08lx\n", hres);

    hres = IActiveScriptSite_QueryInterface(pass, &IID_ICanHandleException, (void**)&canexception);
    ok(hres == E_NOINTERFACE, "Got IID_ICanHandleException interface: %08lx\n", hres);

    hres = IActiveScriptSite_QueryInterface(pass, &IID_IServiceProvider, (void**)&service);
    ok(hres == S_OK, "Could not get IServiceProvider interface: %08lx\n", hres);
    IServiceProvider_Release(service);

    hres = IActiveScriptSite_QueryInterface(pass, &IID_IActiveScriptSiteWindow, (void**)&window);
    ok(hres == S_OK, "Could not get IActiveScriptSiteWindow interface: %08lx\n", hres);
    IActiveScriptSiteWindow_Release(window);

    if (site)
        IActiveScriptSite_Release(site);
    site = pass;
    IActiveScriptSite_AddRef(site);

    return S_OK;
}

static HRESULT WINAPI ActiveScript_GetScriptSite(IActiveScript *iface, REFIID riid,
                                            void **ppvObject)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScript_SetScriptState(IActiveScript *iface, SCRIPTSTATE ss)
{
    switch(ss)
    {
    case SCRIPTSTATE_INITIALIZED:
        CHECK_EXPECT(SetScriptState_INITIALIZED);
        return S_OK;
    case SCRIPTSTATE_STARTED:
        CHECK_EXPECT(SetScriptState_STARTED);
        return S_OK;
    case SCRIPTSTATE_CONNECTED:
        CHECK_EXPECT(SetScriptState_CONNECTED);
        return S_OK;
    default:
        ok(0, "unexpected call, state %u\n", ss);
        return E_NOTIMPL;
    }
}

static SCRIPTSTATE emulated_script_state;

static HRESULT WINAPI ActiveScript_GetScriptState(IActiveScript *iface, SCRIPTSTATE *pssState)
{
    CHECK_EXPECT(GetScriptState);
    *pssState = emulated_script_state;
    return S_OK;
}

static HRESULT WINAPI ActiveScript_Close(IActiveScript *iface)
{
    CHECK_EXPECT(Close);
    return E_NOTIMPL;
}

static const WCHAR *AddNamedItem_expected_name;
static DWORD AddNamedItem_expected_flags;
static HRESULT WINAPI ActiveScript_AddNamedItem(IActiveScript *iface, LPCOLESTR name, DWORD flags)
{
    CHECK_EXPECT(AddNamedItem);
    ok(!lstrcmpW(name, AddNamedItem_expected_name), "got name %s\n", wine_dbgstr_w(name));
    ok(flags == AddNamedItem_expected_flags, "got flags %#lx\n", flags);
    return S_OK;
}

static HRESULT WINAPI ActiveScript_AddTypeLib(IActiveScript *iface, REFGUID rguidTypeLib,
                                         DWORD dwMajor, DWORD dwMinor, DWORD dwFlags)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const WCHAR *GetScriptDispatch_expected_name;
static HRESULT WINAPI ActiveScript_GetScriptDispatch(IActiveScript *iface, LPCOLESTR pstrItemName,
                                                IDispatch **ppdisp)
{
    CHECK_EXPECT(GetScriptDispatch);
    ok(GetScriptDispatch_expected_name ? (pstrItemName && !lstrcmpW(pstrItemName, GetScriptDispatch_expected_name)) : !pstrItemName,
        "pstrItemName not %s, got %s.\n", wine_dbgstr_w(GetScriptDispatch_expected_name), wine_dbgstr_w(pstrItemName));

    *ppdisp = (IDispatch*)&DispatchEx;

    return S_OK;
}

static HRESULT WINAPI ActiveScript_GetCurrentScriptThreadID(IActiveScript *iface,
                                                       SCRIPTTHREADID *pstridThread)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScript_GetScriptThreadID(IActiveScript *iface,
                                                DWORD dwWin32ThreadId, SCRIPTTHREADID *pstidThread)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScript_GetScriptThreadState(IActiveScript *iface,
        SCRIPTTHREADID stidThread, SCRIPTTHREADSTATE *pstsState)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScript_InterruptScriptThread(IActiveScript *iface,
        SCRIPTTHREADID stidThread, const EXCEPINFO *pexcepinfo, DWORD dwFlags)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScript_Clone(IActiveScript *iface, IActiveScript **ppscript)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IActiveScriptVtbl ActiveScriptVtbl = {
    ActiveScript_QueryInterface,
    ActiveScript_AddRef,
    ActiveScript_Release,
    ActiveScript_SetScriptSite,
    ActiveScript_GetScriptSite,
    ActiveScript_SetScriptState,
    ActiveScript_GetScriptState,
    ActiveScript_Close,
    ActiveScript_AddNamedItem,
    ActiveScript_AddTypeLib,
    ActiveScript_GetScriptDispatch,
    ActiveScript_GetCurrentScriptThreadID,
    ActiveScript_GetScriptThreadID,
    ActiveScript_GetScriptThreadState,
    ActiveScript_InterruptScriptThread,
    ActiveScript_Clone
};

static IActiveScript ActiveScript = { &ActiveScriptVtbl };

static HRESULT WINAPI ActiveScriptError_QueryInterface(IActiveScriptError *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IActiveScriptError, riid)) {
        *ppv = iface;
        return S_OK;
    }

    ok(0, "unexpected riid %s\n", wine_dbgstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI ActiveScriptError_AddRef(IActiveScriptError *iface)
{
    return 2;
}

static ULONG WINAPI ActiveScriptError_Release(IActiveScriptError *iface)
{
    return 1;
}

static unsigned ActiveScriptError_stage = 0;

static HRESULT WINAPI ActiveScriptError_DeferredFillIn(EXCEPINFO *pexcepinfo)
{
    CHECK_EXPECT(DeferredFillIn);

    pexcepinfo->bstrSource = SysAllocString(L"foobar");
    pexcepinfo->bstrDescription = SysAllocString(L"barfoo");

    /* Failure is ignored */
    return E_FAIL;
}

static HRESULT WINAPI ActiveScriptError_GetExceptionInfo(IActiveScriptError *iface, EXCEPINFO *pexcepinfo)
{
    CHECK_EXPECT(GetExceptionInfo);

    memset(pexcepinfo, 0, sizeof(*pexcepinfo));
    pexcepinfo->wCode = 0xdead;
    pexcepinfo->pfnDeferredFillIn = ActiveScriptError_DeferredFillIn;

    if (ActiveScriptError_stage == 0) return E_FAIL;
    if (ActiveScriptError_stage == 2)
    {
        pexcepinfo->wCode = 0;
        pexcepinfo->scode = 0xbeef;
        pexcepinfo->bstrSource = SysAllocString(L"source");
        pexcepinfo->pfnDeferredFillIn = NULL;
    }
    return S_OK;
}

static HRESULT WINAPI ActiveScriptError_GetSourcePosition(IActiveScriptError *iface, DWORD *pdwSourceContext,
                                                          ULONG *pulLineNumber, LONG *pichCharPosition)
{
    CHECK_EXPECT(GetSourcePosition);

    *pdwSourceContext = 0xdeadbeef;
    *pulLineNumber = 42;
    *pichCharPosition = 10;

    return (ActiveScriptError_stage == 0) ? E_FAIL : S_OK;
}

static HRESULT WINAPI ActiveScriptError_GetSourceLineText(IActiveScriptError *iface, BSTR *pbstrSourceLine)
{
    CHECK_EXPECT(GetSourceLineText);

    *pbstrSourceLine = SysAllocString(L"source line");

    /* Failure is ignored */
    return E_FAIL;
}

static const IActiveScriptErrorVtbl ActiveScriptErrorVtbl = {
    ActiveScriptError_QueryInterface,
    ActiveScriptError_AddRef,
    ActiveScriptError_Release,
    ActiveScriptError_GetExceptionInfo,
    ActiveScriptError_GetSourcePosition,
    ActiveScriptError_GetSourceLineText
};

static IActiveScriptError ActiveScriptError = { &ActiveScriptErrorVtbl };

static HRESULT WINAPI ClassFactory_QueryInterface(IClassFactory *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IClassFactory, riid)) {
        *ppv = iface;
        return S_OK;
    }

    if(IsEqualGUID(&IID_IMarshal, riid))
        return E_NOINTERFACE;

    ok(0, "unexpected riid %s\n", wine_dbgstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI ClassFactory_AddRef(IClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI ClassFactory_Release(IClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI ClassFactory_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **ppv)
{
    CHECK_EXPECT(CreateInstance);

    ok(!outer, "outer = %p\n", outer);
    ok(IsEqualGUID(&IID_IActiveScript, riid), "unexpected riid %s\n", wine_dbgstr_guid(riid));
    *ppv = &ActiveScript;
    site = NULL;
    return S_OK;
}

static HRESULT WINAPI ClassFactory_LockServer(IClassFactory *iface, BOOL dolock)
{
    ok(0, "unexpected call\n");
    return S_OK;
}

static const IClassFactoryVtbl ClassFactoryVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    ClassFactory_CreateInstance,
    ClassFactory_LockServer
};

static IClassFactory script_cf = { &ClassFactoryVtbl };

static BOOL init_key(const char *key_name, const char *def_value, BOOL init)
{
    HKEY hkey;
    DWORD res;

    if(!init) {
        RegDeleteKeyA(HKEY_CLASSES_ROOT, key_name);
        return TRUE;
    }

    res = RegCreateKeyA(HKEY_CLASSES_ROOT, key_name, &hkey);
    if(res != ERROR_SUCCESS)
        return FALSE;

    if(def_value)
        res = RegSetValueA(hkey, NULL, REG_SZ, def_value, strlen(def_value));

    RegCloseKey(hkey);

    return res == ERROR_SUCCESS;
}

static BOOL init_registry(BOOL init)
{
    return init_key("TestScript\\CLSID", TESTSCRIPT_CLSID, init)
        && init_key("CLSID\\"TESTSCRIPT_CLSID"\\Implemented Categories\\{F0B7A1A1-9847-11CF-8F20-00805F2CD064}",
                    NULL, init)
        && init_key("CLSID\\"TESTSCRIPT_CLSID"\\Implemented Categories\\{F0B7A1A2-9847-11CF-8F20-00805F2CD064}",
                    NULL, init);
}

static BOOL register_script_engine(void)
{
    DWORD regid;
    HRESULT hres;

    if(!init_registry(TRUE)) {
        init_registry(FALSE);
        return FALSE;
    }

    hres = CoRegisterClassObject(&CLSID_TestScript, (IUnknown *)&script_cf,
                                 CLSCTX_INPROC_SERVER, REGCLS_MULTIPLEUSE, &regid);
    ok(hres == S_OK, "Could not register script engine: %08lx\n", hres);

    return TRUE;
}

static BOOL have_custom_engine;

static HRESULT WINAPI OleClientSite_QueryInterface(IOleClientSite *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IOleClientSite) || IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IOleClientSite_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI OleClientSite_AddRef(IOleClientSite *iface)
{
    return 2;
}

static ULONG WINAPI OleClientSite_Release(IOleClientSite *iface)
{
    return 1;
}

static HRESULT WINAPI OleClientSite_SaveObject(IOleClientSite *iface)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI OleClientSite_GetMoniker(IOleClientSite *iface, DWORD assign,
    DWORD which, IMoniker **moniker)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI OleClientSite_GetContainer(IOleClientSite *iface, IOleContainer **container)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI OleClientSite_ShowObject(IOleClientSite *iface)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI OleClientSite_OnShowWindow(IOleClientSite *iface, BOOL show)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI OleClientSite_RequestNewObjectLayout(IOleClientSite *iface)
{
    return E_NOTIMPL;
}

static const IOleClientSiteVtbl OleClientSiteVtbl = {
    OleClientSite_QueryInterface,
    OleClientSite_AddRef,
    OleClientSite_Release,
    OleClientSite_SaveObject,
    OleClientSite_GetMoniker,
    OleClientSite_GetContainer,
    OleClientSite_ShowObject,
    OleClientSite_OnShowWindow,
    OleClientSite_RequestNewObjectLayout
};

static IOleClientSite testclientsite = { &OleClientSiteVtbl };

static void test_oleobject(void)
{
    DWORD status, dpi_x, dpi_y;
    IOleClientSite *site;
    IOleObject *obj;
    SIZEL extent;
    HRESULT hr;
    HDC hdc;

    hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IOleObject, (void**)&obj);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    if (0) /* crashes on w2k3 */
        hr = IOleObject_GetMiscStatus(obj, DVASPECT_CONTENT, NULL);

    status = 0;
    hr = IOleObject_GetMiscStatus(obj, DVASPECT_CONTENT, &status);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(status != 0, "got 0x%08lx\n", status);

    hr = IOleObject_SetClientSite(obj, &testclientsite);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    if (0) /* crashes on w2k3 */
        hr = IOleObject_GetClientSite(obj, NULL);

    hr = IOleObject_GetClientSite(obj, &site);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(site == &testclientsite, "got %p, %p\n", site, &testclientsite);
    IOleClientSite_Release(site);

    hr = IOleObject_SetClientSite(obj, NULL);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = IOleObject_GetClientSite(obj, &site);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(site == NULL, "got %p\n", site);

    /* extents */
    hdc = GetDC(0);
    dpi_x = GetDeviceCaps(hdc, LOGPIXELSX);
    dpi_y = GetDeviceCaps(hdc, LOGPIXELSY);
    ReleaseDC(0, hdc);

    memset(&extent, 0, sizeof(extent));
    hr = IOleObject_GetExtent(obj, DVASPECT_CONTENT, &extent);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(extent.cx == MulDiv(38, 2540, dpi_x), "got %ld\n", extent.cx);
    ok(extent.cy == MulDiv(38, 2540, dpi_y), "got %ld\n", extent.cy);

    extent.cx = extent.cy = 0xdeadbeef;
    hr = IOleObject_GetExtent(obj, DVASPECT_THUMBNAIL, &extent);
    ok(hr == DV_E_DVASPECT, "got 0x%08lx\n", hr);
    ok(extent.cx == 0xdeadbeef, "got %ld\n", extent.cx);
    ok(extent.cy == 0xdeadbeef, "got %ld\n", extent.cy);

    extent.cx = extent.cy = 0xdeadbeef;
    hr = IOleObject_GetExtent(obj, DVASPECT_ICON, &extent);
    ok(hr == DV_E_DVASPECT, "got 0x%08lx\n", hr);
    ok(extent.cx == 0xdeadbeef, "got %ld\n", extent.cx);
    ok(extent.cy == 0xdeadbeef, "got %ld\n", extent.cy);

    extent.cx = extent.cy = 0xdeadbeef;
    hr = IOleObject_GetExtent(obj, DVASPECT_DOCPRINT, &extent);
    ok(hr == DV_E_DVASPECT, "got 0x%08lx\n", hr);
    ok(extent.cx == 0xdeadbeef, "got %ld\n", extent.cx);
    ok(extent.cy == 0xdeadbeef, "got %ld\n", extent.cy);

    IOleObject_Release(obj);
}

static void test_persiststreaminit(void)
{
    IPersistStreamInit *init;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IPersistStreamInit, (void**)&init);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    IPersistStreamInit_Release(init);
}

static void test_olecontrol(void)
{
    IOleControl *olecontrol;
    CONTROLINFO info;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IOleControl, (void**)&olecontrol);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    memset(&info, 0xab, sizeof(info));
    info.cb = sizeof(info);
    hr = IOleControl_GetControlInfo(olecontrol, &info);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(info.hAccel == NULL, "got %p\n", info.hAccel);
    ok(info.cAccel == 0, "got %d\n", info.cAccel);
    ok(info.dwFlags == 0xabababab, "got %lx\n", info.dwFlags);

    memset(&info, 0xab, sizeof(info));
    info.cb = sizeof(info) - 1;
    hr = IOleControl_GetControlInfo(olecontrol, &info);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(info.hAccel == NULL, "got %p\n", info.hAccel);
    ok(info.cAccel == 0, "got %d\n", info.cAccel);
    ok(info.dwFlags == 0xabababab, "got %lx\n", info.dwFlags);

    if (0) /* crashes on win2k3 */
    {
        hr = IOleControl_GetControlInfo(olecontrol, NULL);
        ok(hr == E_POINTER, "got 0x%08lx\n", hr);
    }

    IOleControl_Release(olecontrol);
}

static void test_Language(void)
{
    IScriptControl *sc;
    HRESULT hr;
    BSTR str;

    hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IScriptControl, (void**)&sc);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = IScriptControl_get_Language(sc, NULL);
    ok(hr == E_POINTER, "got 0x%08lx\n", hr);

    str = (BSTR)0xdeadbeef;
    hr = IScriptControl_get_Language(sc, &str);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(str == NULL, "got %s\n", wine_dbgstr_w(str));

    str = SysAllocString(L"VBScript");
    hr = IScriptControl_put_Language(sc, str);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    SysFreeString(str);

    str = SysAllocString(L"vBscript");
    hr = IScriptControl_put_Language(sc, str);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    SysFreeString(str);

    hr = IScriptControl_get_Language(sc, &str);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(!lstrcmpW(str, L"VBScript"), "got %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    str = SysAllocString(L"dummy");
    hr = IScriptControl_put_Language(sc, str);
    ok(hr == CTL_E_INVALIDPROPERTYVALUE, "got 0x%08lx\n", hr);
    SysFreeString(str);

    hr = IScriptControl_get_Language(sc, &str);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(!lstrcmpW(str, L"VBScript"), "got %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    str = SysAllocString(L"JScript");
    hr = IScriptControl_put_Language(sc, str);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    SysFreeString(str);

    hr = IScriptControl_get_Language(sc, &str);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(!lstrcmpW(str, L"JScript"), "got %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    hr = IScriptControl_put_Language(sc, NULL);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = IScriptControl_get_Language(sc, &str);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(str == NULL, "got %s\n", wine_dbgstr_w(str));

    IScriptControl_Release(sc);

    /* custom script engine */
    if (have_custom_engine) {
        hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
                &IID_IScriptControl, (void**)&sc);
        ok(hr == S_OK, "got 0x%08lx\n", hr);

        SET_EXPECT(CreateInstance);
        SET_EXPECT(SetInterfaceSafetyOptions);
        SET_EXPECT(SetScriptSite);
        SET_EXPECT(QI_IActiveScriptParse);
        SET_EXPECT(InitNew);

        str = SysAllocString(L"testscript");
        hr = IScriptControl_put_Language(sc, str);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        SysFreeString(str);

        CHECK_CALLED(CreateInstance);
        CHECK_CALLED(SetInterfaceSafetyOptions);
        CHECK_CALLED(SetScriptSite);
        CHECK_CALLED(QI_IActiveScriptParse);
        CHECK_CALLED(InitNew);
        hr = IScriptControl_get_Language(sc, &str);
        todo_wine
        ok(hr == S_OK, "got 0x%08lx\n", hr);
         if (hr == S_OK)
            ok(!lstrcmpW(L"testscript", str), "%s\n", wine_dbgstr_w(str));
        SysFreeString(str);

        IActiveScriptSite_Release(site);

        SET_EXPECT(Close);

        IScriptControl_Release(sc);

        CHECK_CALLED(Close);
    }
}

static void test_connectionpoints(void)
{
    IConnectionPointContainer *container, *container2;
    IConnectionPoint *cp;
    IScriptControl *sc;
    HRESULT hr;
    IID iid;

    hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IScriptControl, (void**)&sc);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    EXPECT_REF(sc, 1);
    hr = IScriptControl_QueryInterface(sc, &IID_IConnectionPointContainer, (void**)&container);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    EXPECT_REF(sc, 2);
    EXPECT_REF(container, 2);

    hr = IConnectionPointContainer_FindConnectionPoint(container, &IID_IPropertyNotifySink, &cp);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    if (0) /* crashes on win2k3 */
    {
        hr = IConnectionPoint_GetConnectionPointContainer(cp, NULL);
        ok(hr == E_POINTER, "got 0x%08lx\n", hr);
    }

    hr = IConnectionPoint_GetConnectionInterface(cp, &iid);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(IsEqualIID(&iid, &IID_IPropertyNotifySink), "got %s\n", wine_dbgstr_guid(&iid));

    hr = IConnectionPoint_GetConnectionPointContainer(cp, &container2);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(container2 == container, "got %p, expected %p\n", container2, container);
    IConnectionPointContainer_Release(container2);

    IConnectionPoint_Release(cp);

    hr = IConnectionPointContainer_FindConnectionPoint(container, &DIID_DScriptControlSource, &cp);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    if (0) /* crashes on win2k3 */
    {
        hr = IConnectionPoint_GetConnectionPointContainer(cp, NULL);
        ok(hr == E_POINTER, "got 0x%08lx\n", hr);
    }

    hr = IConnectionPoint_GetConnectionInterface(cp, &iid);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(IsEqualIID(&iid, &DIID_DScriptControlSource), "got %s\n", wine_dbgstr_guid(&iid));

    hr = IConnectionPoint_GetConnectionPointContainer(cp, &container2);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(container2 == container, "got %p, expected %p\n", container2, container);
    IConnectionPointContainer_Release(container2);

    IConnectionPoint_Release(cp);

    IConnectionPointContainer_Release(container);
    IScriptControl_Release(sc);
}

static void test_quickactivate(void)
{
    IScriptControl *sc;
    IQuickActivate *qa;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IScriptControl, (void**)&sc);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = IScriptControl_QueryInterface(sc, &IID_IQuickActivate, (void**)&qa);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    IQuickActivate_Release(qa);
    IScriptControl_Release(sc);
}

static void test_viewobject(void)
{
    DWORD status, aspect, flags;
    IViewObjectEx *viewex;
    IScriptControl *sc;
    IViewObject *view;
    IAdviseSink *sink;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IScriptControl, (void**)&sc);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = IScriptControl_QueryInterface(sc, &IID_IViewObject, (void**)&view);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    IViewObject_Release(view);

    hr = IScriptControl_QueryInterface(sc, &IID_IViewObject2, (void**)&view);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    sink = (IAdviseSink*)0xdeadbeef;
    hr = IViewObject_GetAdvise(view, &aspect, &flags, &sink);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(aspect == DVASPECT_CONTENT, "got %lu\n", aspect);
    ok(flags == 0, "got %#lx\n", flags);
    ok(sink == NULL, "got %p\n", sink);

    hr = IViewObject_SetAdvise(view, DVASPECT_CONTENT, 0, NULL);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = IViewObject_SetAdvise(view, DVASPECT_THUMBNAIL, 0, NULL);
    ok(hr == DV_E_DVASPECT, "got 0x%08lx\n", hr);

    hr = IViewObject_SetAdvise(view, DVASPECT_ICON, 0, NULL);
    ok(hr == DV_E_DVASPECT, "got 0x%08lx\n", hr);

    hr = IViewObject_SetAdvise(view, DVASPECT_DOCPRINT, 0, NULL);
    ok(hr == DV_E_DVASPECT, "got 0x%08lx\n", hr);

    sink = (IAdviseSink*)0xdeadbeef;
    hr = IViewObject_GetAdvise(view, &aspect, &flags, &sink);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(aspect == DVASPECT_CONTENT, "got %lu\n", aspect);
    ok(flags == 0, "got %#lx\n", flags);
    ok(sink == NULL, "got %p\n", sink);

    IViewObject_Release(view);

    hr = IScriptControl_QueryInterface(sc, &IID_IViewObjectEx, (void**)&viewex);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    if (0) /* crashes */
        hr = IViewObjectEx_GetViewStatus(viewex, NULL);

    status = 0;
    hr = IViewObjectEx_GetViewStatus(viewex, &status);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(status == VIEWSTATUS_OPAQUE, "got %#lx\n", status);

    IViewObjectEx_Release(viewex);

    IScriptControl_Release(sc);
}

static void test_pointerinactive(void)
{
    IPointerInactive *pi;
    IScriptControl *sc;
    DWORD policy;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IScriptControl, (void**)&sc);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = IScriptControl_QueryInterface(sc, &IID_IPointerInactive, (void**)&pi);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    if (0) /* crashes w2k3 */
        hr = IPointerInactive_GetActivationPolicy(pi, NULL);

    policy = 123;
    hr = IPointerInactive_GetActivationPolicy(pi, &policy);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(policy == 0, "got %#lx\n", policy);

    IPointerInactive_Release(pi);
    IScriptControl_Release(sc);
}

static void test_timeout(void)
{
    IScriptControl *sc;
    HRESULT hr;
    LONG val;
    BSTR str;

    hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IScriptControl, (void**)&sc);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = IScriptControl_get_Timeout(sc, NULL);
    ok(hr == E_POINTER, "got 0x%08lx\n", hr);

    val = 0;
    hr = IScriptControl_get_Timeout(sc, &val);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(val == 10000, "got %ld\n", val);

    hr = IScriptControl_put_Timeout(sc, -1);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    val = 0;
    hr = IScriptControl_get_Timeout(sc, &val);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(val == -1, "got %ld\n", val);

    hr = IScriptControl_put_Timeout(sc, -2);
    ok(hr == CTL_E_INVALIDPROPERTYVALUE, "got 0x%08lx\n", hr);

    val = 0;
    hr = IScriptControl_get_Timeout(sc, &val);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(val == -1, "got %ld\n", val);

    hr = IScriptControl_put_Timeout(sc, 0);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    val = 1;
    hr = IScriptControl_get_Timeout(sc, &val);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(val == 0, "got %ld\n", val);

    str = SysAllocString(L"VBScript");
    hr = IScriptControl_put_Language(sc, str);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    SysFreeString(str);

    val = 1;
    hr = IScriptControl_get_Timeout(sc, &val);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(val == 0, "got %ld\n", val);

    hr = IScriptControl_put_Timeout(sc, 10000);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    IScriptControl_Release(sc);
}

static void test_Reset(void)
{
    IScriptControl *sc;
    HRESULT hr;
    BSTR str;

    hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IScriptControl, (void**)&sc);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = IScriptControl_Reset(sc);
    ok(hr == E_FAIL, "got 0x%08lx\n", hr);

    str = SysAllocString(L"VBScript");
    hr = IScriptControl_put_Language(sc, str);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    SysFreeString(str);

    hr = IScriptControl_Reset(sc);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = IScriptControl_get_Language(sc, &str);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(!lstrcmpW(str, L"VBScript"), "got %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    IScriptControl_Release(sc);

    /* custom script engine */
    if (have_custom_engine) {
        hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
                &IID_IScriptControl, (void**)&sc);
        ok(hr == S_OK, "got 0x%08lx\n", hr);

        SET_EXPECT(CreateInstance);
        SET_EXPECT(SetInterfaceSafetyOptions);
        SET_EXPECT(SetScriptSite);
        SET_EXPECT(QI_IActiveScriptParse);
        SET_EXPECT(InitNew);

        str = SysAllocString(L"testscript");
        hr = IScriptControl_put_Language(sc, str);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        SysFreeString(str);

        SET_EXPECT(SetScriptState_INITIALIZED);
        hr = IScriptControl_Reset(sc);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        CHECK_CALLED(SetScriptState_INITIALIZED);

        SET_EXPECT(SetScriptState_INITIALIZED);
        hr = IScriptControl_Reset(sc);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        CHECK_CALLED(SetScriptState_INITIALIZED);

        CHECK_CALLED(SetScriptSite);
        IActiveScriptSite_Release(site);

        SET_EXPECT(Close);

        IScriptControl_Release(sc);

        CHECK_CALLED(Close);
    }
}

static HRESULT WINAPI disp_QI(IDispatch *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IDispatch) || IsEqualIID(riid, &IID_IUnknown)) {
        *obj = iface;
        IDispatch_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI disp_AddRef(IDispatch *iface)
{
    return 2;
}

static ULONG WINAPI disp_Release(IDispatch *iface)
{
    return 1;
}

static HRESULT WINAPI disp_GetTypeInfoCount(IDispatch *iface, UINT *count)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI disp_GetTypeInfo(IDispatch *iface, UINT index, LCID lcid, ITypeInfo **ti)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI disp_GetIDsOfNames(IDispatch *iface, REFIID riid, LPOLESTR *names,
    UINT cnames, LCID lcid, DISPID *dispid)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI disp_Invoke(IDispatch *iface, DISPID dispid, REFIID riid, LCID lcid,
    WORD flags, DISPPARAMS *dispparams, VARIANT *result, EXCEPINFO *ei, UINT *arg_err)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IDispatchVtbl dispvtbl = {
    disp_QI,
    disp_AddRef,
    disp_Release,
    disp_GetTypeInfoCount,
    disp_GetTypeInfo,
    disp_GetIDsOfNames,
    disp_Invoke
};

static IDispatch testdisp = { &dispvtbl };

static void test_AddObject(void)
{
    IScriptControl *sc;
    BSTR str, objname;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IScriptControl, (void**)&sc);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    objname = SysAllocString(L"o");
    hr = IScriptControl_AddObject(sc, objname, NULL, VARIANT_FALSE);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

    hr = IScriptControl_AddObject(sc, objname, &testdisp, VARIANT_FALSE);
    ok(hr == E_FAIL, "got 0x%08lx\n", hr);

    str = SysAllocString(L"VBScript");
    hr = IScriptControl_put_Language(sc, str);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    SysFreeString(str);

    hr = IScriptControl_AddObject(sc, objname, &testdisp, VARIANT_TRUE);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = IScriptControl_Reset(sc);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = IScriptControl_AddObject(sc, objname, &testdisp, VARIANT_TRUE);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    IScriptControl_Release(sc);

    /* custom script engine */
    if (have_custom_engine) {
        hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
                &IID_IScriptControl, (void**)&sc);
        ok(hr == S_OK, "got 0x%08lx\n", hr);

        SET_EXPECT(CreateInstance);
        SET_EXPECT(SetInterfaceSafetyOptions);
        SET_EXPECT(SetScriptSite);
        SET_EXPECT(QI_IActiveScriptParse);
        SET_EXPECT(InitNew);

        str = SysAllocString(L"testscript");
        hr = IScriptControl_put_Language(sc, str);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        SysFreeString(str);

        hr = IScriptControl_AddObject(sc, objname, NULL, VARIANT_FALSE);
        ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

        SET_EXPECT(AddNamedItem);
        AddNamedItem_expected_name = objname;
        AddNamedItem_expected_flags = SCRIPTITEM_ISVISIBLE|SCRIPTITEM_ISSOURCE|SCRIPTITEM_GLOBALMEMBERS;
        hr = IScriptControl_AddObject(sc, objname, &testdisp, VARIANT_TRUE);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        CHECK_CALLED(AddNamedItem);

        hr = IScriptControl_AddObject(sc, objname, &testdisp, VARIANT_TRUE);
        ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

        CHECK_CALLED(SetScriptSite);
        IActiveScriptSite_Release(site);

        SET_EXPECT(Close);

        IScriptControl_Release(sc);

        CHECK_CALLED(Close);
    }

    SysFreeString(objname);
}

static void test_AllowUI(void)
{
    IScriptControl *sc;
    VARIANT_BOOL allow;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IScriptControl, (void**)&sc);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = IScriptControl_get_AllowUI(sc, NULL);
    ok(hr == E_POINTER, "got 0x%08lx\n", hr);

    hr = IScriptControl_get_AllowUI(sc, &allow);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(allow == VARIANT_TRUE, "got %d\n", allow);

    hr = IScriptControl_put_AllowUI(sc, VARIANT_FALSE);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = IScriptControl_get_AllowUI(sc, &allow);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(allow == VARIANT_FALSE, "got %d\n", allow);

    IScriptControl_Release(sc);
}

static void test_SitehWnd(void)
{
    IScriptControl *sc;
    LONG site_hwnd;
    HRESULT hr;
    HWND hwnd;
    BSTR str;

    hwnd = CreateWindowA("static", NULL, WS_OVERLAPPEDWINDOW, 50, 50, 100, 100, NULL, NULL, NULL, NULL);
    ok(hwnd != NULL, "failed to create window, error %08lx\n", GetLastError());

    hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IScriptControl, (void**)&sc);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = IScriptControl_get_SitehWnd(sc, NULL);
    ok(hr == E_POINTER, "got 0x%08lx\n", hr);

    site_hwnd = 0xdeadbeef;
    hr = IScriptControl_get_SitehWnd(sc, &site_hwnd);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(!site_hwnd, "got %p\n", (HWND)(LONG_PTR)site_hwnd);

    hr = IScriptControl_put_SitehWnd(sc, 1);
    ok(hr == CTL_E_INVALIDPROPERTYVALUE, "got 0x%08lx\n", hr);

    site_hwnd = 0xdeadbeef;
    hr = IScriptControl_get_SitehWnd(sc, &site_hwnd);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(!site_hwnd, "got %p\n", (HWND)(LONG_PTR)site_hwnd);

    hr = IScriptControl_put_SitehWnd(sc, 0);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = IScriptControl_put_SitehWnd(sc, (LONG)(LONG_PTR)hwnd);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    str = SysAllocString(L"vbscript");
    hr = IScriptControl_put_Language(sc, str);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    SysFreeString(str);

    site_hwnd = 0;
    hr = IScriptControl_get_SitehWnd(sc, &site_hwnd);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok((HWND)(LONG_PTR)site_hwnd == hwnd, "got %p, expected %p\n", (HWND)(LONG_PTR)site_hwnd, hwnd);

    IScriptControl_Release(sc);

    /* custom script engine */
    if (have_custom_engine)
    {
        IActiveScriptSiteWindow *site_window;
        HWND window;

        hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
                              &IID_IScriptControl, (void **)&sc);
        ok(hr == S_OK, "Failed to create IScriptControl interface: 0x%08lx.\n", hr);

        hr = IScriptControl_put_SitehWnd(sc, (LONG)(LONG_PTR)hwnd);
        ok(hr == S_OK, "got 0x%08lx\n", hr);

        SET_EXPECT(CreateInstance);
        SET_EXPECT(SetInterfaceSafetyOptions);
        SET_EXPECT(SetScriptSite);
        SET_EXPECT(QI_IActiveScriptParse);
        SET_EXPECT(InitNew);

        str = SysAllocString(L"testscript");
        hr = IScriptControl_put_Language(sc, str);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        SysFreeString(str);

        CHECK_CALLED(CreateInstance);
        CHECK_CALLED(SetInterfaceSafetyOptions);
        CHECK_CALLED(SetScriptSite);
        CHECK_CALLED(QI_IActiveScriptParse);
        CHECK_CALLED(InitNew);

        hr = IActiveScriptSite_QueryInterface(site, &IID_IActiveScriptSiteWindow, (void**)&site_window);
        ok(hr == S_OK, "got 0x%08lx\n", hr);

        site_hwnd = 0;
        hr = IScriptControl_get_SitehWnd(sc, &site_hwnd);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        ok((HWND)(LONG_PTR)site_hwnd == hwnd, "got %p, expected %p\n", (HWND)(LONG_PTR)site_hwnd, hwnd);

        window = NULL;
        hr = IActiveScriptSiteWindow_GetWindow(site_window, NULL);
        ok(hr == E_POINTER, "got 0x%08lx\n", hr);
        hr = IActiveScriptSiteWindow_GetWindow(site_window, &window);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        ok(window == hwnd, "got %p, expected %p\n", window, hwnd);

        hr = IActiveScriptSiteWindow_EnableModeless(site_window, FALSE);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        hr = IActiveScriptSiteWindow_EnableModeless(site_window, TRUE);
        ok(hr == S_OK, "got 0x%08lx\n", hr);

        site_hwnd = 0xdeadbeef;
        hr = IScriptControl_put_SitehWnd(sc, 0);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        hr = IScriptControl_get_SitehWnd(sc, &site_hwnd);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        ok(!site_hwnd, "got %p\n", (HWND)(LONG_PTR)site_hwnd);

        window = (HWND)0xdeadbeef;
        hr = IActiveScriptSiteWindow_GetWindow(site_window, &window);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        ok(!window, "got %p\n", window);

        site_hwnd = 0;
        hr = IScriptControl_put_SitehWnd(sc, (LONG)(LONG_PTR)hwnd);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        hr = IScriptControl_get_SitehWnd(sc, &site_hwnd);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        ok((HWND)(LONG_PTR)site_hwnd == hwnd, "got %p, expected %p\n", (HWND)(LONG_PTR)site_hwnd, hwnd);

        window = NULL;
        hr = IActiveScriptSiteWindow_GetWindow(site_window, &window);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        ok(window == hwnd, "got %p, expected %p\n", window, hwnd);

        hr = IScriptControl_put_AllowUI(sc, VARIANT_FALSE);
        ok(hr == S_OK, "got 0x%08lx\n", hr);

        hr = IScriptControl_get_SitehWnd(sc, &site_hwnd);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        ok((HWND)(LONG_PTR)site_hwnd == hwnd, "got %p, expected %p\n", (HWND)(LONG_PTR)site_hwnd, hwnd);

        window = NULL;
        hr = IActiveScriptSiteWindow_GetWindow(site_window, &window);
        ok(hr == E_FAIL, "got 0x%08lx\n", hr);
        ok(!window, "got %p\n", window);

        hr = IScriptControl_put_AllowUI(sc, VARIANT_TRUE);
        ok(hr == S_OK, "got 0x%08lx\n", hr);

        window = NULL;
        hr = IActiveScriptSiteWindow_GetWindow(site_window, &window);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        ok(window == hwnd, "got %p, expected %p\n", window, hwnd);

        IActiveScriptSite_Release(site);

        SET_EXPECT(Close);
        IScriptControl_Release(sc);
        CHECK_CALLED(Close);

        window = NULL;
        hr = IActiveScriptSiteWindow_GetWindow(site_window, &window);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        ok(window == hwnd, "got %p, expected %p\n", window, hwnd);

        IActiveScriptSiteWindow_Release(site_window);
    }

    DestroyWindow(hwnd);
}

static void test_UseSafeSubset(void)
{
    IScriptControl *sc;
    VARIANT_BOOL use_safe_subset;
    HRESULT hr;
    BSTR str;

    hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IScriptControl, (void**)&sc);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = IScriptControl_get_UseSafeSubset(sc, NULL);
    ok(hr == E_POINTER, "got 0x%08lx\n", hr);

    hr = IScriptControl_get_UseSafeSubset(sc, &use_safe_subset);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(use_safe_subset == VARIANT_FALSE, "got %d\n", use_safe_subset);

    hr = IScriptControl_put_UseSafeSubset(sc, VARIANT_TRUE);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = IScriptControl_get_UseSafeSubset(sc, &use_safe_subset);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(use_safe_subset == VARIANT_TRUE, "got %d\n", use_safe_subset);

    str = SysAllocString(L"VBScript");
    hr = IScriptControl_put_Language(sc, str);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    SysFreeString(str);

    hr = IScriptControl_get_UseSafeSubset(sc, &use_safe_subset);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(use_safe_subset == VARIANT_TRUE, "got %d\n", use_safe_subset);

    IScriptControl_Release(sc);

    /* custom script engine */
    if (have_custom_engine)
    {
        hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
                              &IID_IScriptControl, (void **)&sc);
        ok(hr == S_OK, "Failed to create IScriptControl interface: 0x%08lx.\n", hr);

        SET_EXPECT(CreateInstance);
        SET_EXPECT(SetInterfaceSafetyOptions);
        SET_EXPECT(SetScriptSite);
        SET_EXPECT(QI_IActiveScriptParse);
        SET_EXPECT(InitNew);

        str = SysAllocString(L"testscript");
        hr = IScriptControl_put_Language(sc, str);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        SysFreeString(str);

        CHECK_CALLED(CreateInstance);
        CHECK_CALLED(SetInterfaceSafetyOptions);
        CHECK_CALLED(SetScriptSite);
        CHECK_CALLED(QI_IActiveScriptParse);
        CHECK_CALLED(InitNew);

        hr = IScriptControl_get_UseSafeSubset(sc, &use_safe_subset);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        ok(use_safe_subset == VARIANT_FALSE, "got %d\n", use_safe_subset);

        SET_EXPECT(SetInterfaceSafetyOptions_UseSafeSubset);
        hr = IScriptControl_put_UseSafeSubset(sc, VARIANT_TRUE);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        CHECK_CALLED(SetInterfaceSafetyOptions_UseSafeSubset);

        hr = IScriptControl_get_UseSafeSubset(sc, &use_safe_subset);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        ok(use_safe_subset == VARIANT_TRUE, "got %d\n", use_safe_subset);

        hr = IScriptControl_put_UseSafeSubset(sc, VARIANT_TRUE);
        ok(hr == S_OK, "got 0x%08lx\n", hr);

        SET_EXPECT(SetInterfaceSafetyOptions);
        hr = IScriptControl_put_UseSafeSubset(sc, VARIANT_FALSE);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        CHECK_CALLED(SetInterfaceSafetyOptions);

        hr = IScriptControl_put_UseSafeSubset(sc, VARIANT_FALSE);
        ok(hr == S_OK, "got 0x%08lx\n", hr);

        IActiveScriptSite_Release(site);

        SET_EXPECT(Close);
        IScriptControl_Release(sc);
        CHECK_CALLED(Close);

        hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
                              &IID_IScriptControl, (void **)&sc);
        ok(hr == S_OK, "Failed to create IScriptControl interface: 0x%08lx.\n", hr);

        hr = IScriptControl_put_UseSafeSubset(sc, VARIANT_TRUE);
        ok(hr == S_OK, "got 0x%08lx\n", hr);

        SET_EXPECT(CreateInstance);
        SET_EXPECT(SetInterfaceSafetyOptions_UseSafeSubset);
        SET_EXPECT(SetScriptSite);
        SET_EXPECT(QI_IActiveScriptParse);
        SET_EXPECT(InitNew);

        str = SysAllocString(L"testscript");
        hr = IScriptControl_put_Language(sc, str);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        SysFreeString(str);

        CHECK_CALLED(CreateInstance);
        CHECK_CALLED(SetInterfaceSafetyOptions_UseSafeSubset);
        CHECK_CALLED(SetScriptSite);
        CHECK_CALLED(QI_IActiveScriptParse);
        CHECK_CALLED(InitNew);

        IActiveScriptSite_Release(site);

        SET_EXPECT(Close);
        IScriptControl_Release(sc);
        CHECK_CALLED(Close);
    }
}

static void test_State(void)
{
    IScriptControl *sc;
    ScriptControlStates state;
    HRESULT hr;
    BSTR str;

    hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IScriptControl, (void**)&sc);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = IScriptControl_get_State(sc, NULL);
    ok(hr == E_POINTER, "got 0x%08lx\n", hr);

    hr = IScriptControl_get_State(sc, &state);
    ok(hr == E_FAIL, "got 0x%08lx\n", hr);

    hr = IScriptControl_put_State(sc, Connected);
    ok(hr == E_FAIL, "got 0x%08lx\n", hr);

    str = SysAllocString(L"VBScript");
    hr = IScriptControl_put_Language(sc, str);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    SysFreeString(str);

    hr = IScriptControl_get_State(sc, &state);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(state == Initialized, "got %d\n", state);

    hr = IScriptControl_put_State(sc, Connected);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = IScriptControl_get_State(sc, &state);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(state == Connected, "got %d\n", state);

    hr = IScriptControl_put_State(sc, 2);
    ok(hr == CTL_E_INVALIDPROPERTYVALUE, "got 0x%08lx\n", hr);

    hr = IScriptControl_get_State(sc, &state);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(state == Connected, "got %d\n", state);

    hr = IScriptControl_put_State(sc, -1);
    ok(hr == CTL_E_INVALIDPROPERTYVALUE, "got 0x%08lx\n", hr);

    hr = IScriptControl_get_State(sc, &state);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(state == Connected, "got %d\n", state);

    IScriptControl_Release(sc);

    /* custom script engine */
    if (have_custom_engine)
    {
        BSTR language, code_str;

        hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
                              &IID_IScriptControl, (void **)&sc);
        ok(hr == S_OK, "Failed to create IScriptControl interface: 0x%08lx.\n", hr);

        SET_EXPECT(CreateInstance);
        SET_EXPECT(SetInterfaceSafetyOptions);
        SET_EXPECT(SetScriptSite);
        SET_EXPECT(QI_IActiveScriptParse);
        SET_EXPECT(InitNew);

        language = SysAllocString(L"testscript");
        hr = IScriptControl_put_Language(sc, language);
        ok(hr == S_OK, "IScriptControl_put_Language failed: 0x%08lx.\n", hr);
        SysFreeString(language);

        CHECK_CALLED(CreateInstance);
        CHECK_CALLED(SetInterfaceSafetyOptions);
        CHECK_CALLED(SetScriptSite);
        CHECK_CALLED(QI_IActiveScriptParse);
        CHECK_CALLED(InitNew);

        SET_EXPECT(SetScriptState_STARTED);
        hr = IScriptControl_put_State(sc, Initialized);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        CHECK_CALLED(SetScriptState_STARTED);

        SET_EXPECT(SetScriptState_STARTED);
        hr = IScriptControl_put_State(sc, Initialized);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        CHECK_CALLED(SetScriptState_STARTED);

        SET_EXPECT(SetScriptState_CONNECTED);
        hr = IScriptControl_put_State(sc, Connected);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        CHECK_CALLED(SetScriptState_CONNECTED);

        SET_EXPECT(SetScriptState_STARTED);
        hr = IScriptControl_put_State(sc, Initialized);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        CHECK_CALLED(SetScriptState_STARTED);

        SET_EXPECT(SetScriptState_STARTED);
        SET_EXPECT(ParseScriptText);
        parse_item_name = NULL;
        parse_flags = SCRIPTTEXT_ISVISIBLE;
        code_str = SysAllocString(L"1 + 1");
        hr = IScriptControl_AddCode(sc, code_str);
        ok(hr == S_OK, "IScriptControl_AddCode failed: 0x%08lx.\n", hr);
        SysFreeString(code_str);
        CHECK_CALLED(SetScriptState_STARTED);
        CHECK_CALLED(ParseScriptText);

        emulated_script_state = SCRIPTSTATE_INITIALIZED;
        SET_EXPECT(GetScriptState);
        hr = IScriptControl_get_State(sc, &state);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        ok(state == Initialized, "got %d\n", state);
        CHECK_CALLED(GetScriptState);

        emulated_script_state = SCRIPTSTATE_STARTED;
        SET_EXPECT(GetScriptState);
        hr = IScriptControl_get_State(sc, &state);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        ok(state == Initialized, "got %d\n", state);
        CHECK_CALLED(GetScriptState);

        emulated_script_state = SCRIPTSTATE_CONNECTED;
        SET_EXPECT(GetScriptState);
        hr = IScriptControl_get_State(sc, &state);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        ok(state == Connected, "got %d\n", state);
        CHECK_CALLED(GetScriptState);

        emulated_script_state = SCRIPTSTATE_UNINITIALIZED;
        SET_EXPECT(GetScriptState);
        hr = IScriptControl_get_State(sc, &state);
        ok(hr == E_FAIL, "got 0x%08lx\n", hr);
        CHECK_CALLED(GetScriptState);

        SET_EXPECT(ParseScriptText);
        code_str = SysAllocString(L"0x100");
        hr = IScriptControl_AddCode(sc, code_str);
        ok(hr == S_OK, "IScriptControl_AddCode failed: 0x%08lx.\n", hr);
        SysFreeString(code_str);
        CHECK_CALLED(ParseScriptText);

        IActiveScriptSite_Release(site);

        SET_EXPECT(Close);
        IScriptControl_Release(sc);
        CHECK_CALLED(Close);

        hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
                              &IID_IScriptControl, (void **)&sc);
        ok(hr == S_OK, "Failed to create IScriptControl interface: 0x%08lx.\n", hr);

        SET_EXPECT(CreateInstance);
        SET_EXPECT(SetInterfaceSafetyOptions);
        SET_EXPECT(SetScriptSite);
        SET_EXPECT(QI_IActiveScriptParse);
        SET_EXPECT(InitNew);

        language = SysAllocString(L"testscript");
        hr = IScriptControl_put_Language(sc, language);
        ok(hr == S_OK, "IScriptControl_put_Language failed: 0x%08lx.\n", hr);
        SysFreeString(language);

        CHECK_CALLED(CreateInstance);
        CHECK_CALLED(SetInterfaceSafetyOptions);
        CHECK_CALLED(SetScriptSite);
        CHECK_CALLED(QI_IActiveScriptParse);
        CHECK_CALLED(InitNew);

        SET_EXPECT(SetScriptState_STARTED);
        hr = IScriptControl_put_State(sc, Initialized);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        CHECK_CALLED(SetScriptState_STARTED);

        SET_EXPECT(SetScriptState_STARTED);
        hr = IScriptControl_put_State(sc, Initialized);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        CHECK_CALLED(SetScriptState_STARTED);

        SET_EXPECT(SetScriptState_CONNECTED);
        hr = IScriptControl_put_State(sc, Connected);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        CHECK_CALLED(SetScriptState_CONNECTED);

        SET_EXPECT(SetScriptState_STARTED);
        SET_EXPECT(ParseScriptText);
        parse_item_name = NULL;
        parse_flags = SCRIPTTEXT_ISVISIBLE;
        code_str = SysAllocString(L"1 + 1");
        hr = IScriptControl_AddCode(sc, code_str);
        ok(hr == S_OK, "IScriptControl_AddCode failed: 0x%08lx.\n", hr);
        SysFreeString(code_str);
        CHECK_CALLED(SetScriptState_STARTED);
        CHECK_CALLED(ParseScriptText);

        SET_EXPECT(SetScriptState_CONNECTED);
        hr = IScriptControl_put_State(sc, Connected);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        CHECK_CALLED(SetScriptState_CONNECTED);

        SET_EXPECT(ParseScriptText);
        code_str = SysAllocString(L"0x100");
        hr = IScriptControl_AddCode(sc, code_str);
        ok(hr == S_OK, "IScriptControl_AddCode failed: 0x%08lx.\n", hr);
        SysFreeString(code_str);
        CHECK_CALLED(ParseScriptText);

        IActiveScriptSite_Release(site);

        SET_EXPECT(Close);
        IScriptControl_Release(sc);
        CHECK_CALLED(Close);
    }
}

#define CHECK_ERROR(sc,exp_num) _check_error(sc, exp_num, FALSE, __LINE__)
#define CHECK_ERROR_TODO(sc,exp_num) _check_error(sc, exp_num, TRUE, __LINE__)
static void _check_error(IScriptControl *sc, LONG exp_num, BOOL todo, int line)
{
    IScriptError *script_err;
    LONG error_num;
    HRESULT hr;

    hr = IScriptControl_get_Error(sc, &script_err);
    ok_(__FILE__,line)(hr == S_OK, "IScriptControl_get_Error failed: 0x%08lx.\n", hr);
    error_num = 0xdeadbeef;
    hr = IScriptError_get_Number(script_err, &error_num);
    ok_(__FILE__,line)(hr == S_OK, "IScriptError_get_Number failed: 0x%08lx.\n", hr);
    todo_wine_if(todo == TRUE)
    ok_(__FILE__,line)(error_num == exp_num, "got wrong error number: %ld, expected %ld.\n",
                       error_num, exp_num);
    IScriptError_Release(script_err);
}

static void test_IScriptControl_Eval(void)
{
    IScriptControl *sc;
    HRESULT hr;
    BSTR script_str, language, expected_string;
    VARIANT var;
    ScriptControlStates state;

    hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
                          &IID_IScriptControl, (void **)&sc);
    ok(hr == S_OK, "Failed to create IScriptControl interface: 0x%08lx.\n", hr);

    hr = IScriptControl_Eval(sc, NULL, NULL);
    ok(hr == E_POINTER, "IScriptControl_Eval returned: 0x%08lx.\n", hr);

    script_str = SysAllocString(L"1 + 1");
    hr = IScriptControl_Eval(sc, script_str, NULL);
    ok(hr == E_POINTER, "IScriptControl_Eval returned: 0x%08lx.\n", hr);
    SysFreeString(script_str);

    V_VT(&var) = VT_NULL;
    V_I4(&var) = 0xdeadbeef;
    hr = IScriptControl_Eval(sc, NULL, &var);
    ok(hr == E_FAIL, "IScriptControl_Eval returned: 0x%08lx.\n", hr);
    ok((V_VT(&var) == VT_EMPTY) && (V_I4(&var) == 0xdeadbeef), "V_VT(var) = %d, V_I4(var) = %ld.\n",
       V_VT(&var), V_I4(&var));
    CHECK_ERROR(sc, 0);

    script_str = SysAllocString(L"1 + 1");
    V_VT(&var) = VT_NULL;
    V_I4(&var) = 0xdeadbeef;
    hr = IScriptControl_Eval(sc, script_str, &var);
    ok(hr == E_FAIL, "IScriptControl_Eval returned: 0x%08lx.\n", hr);
    ok((V_VT(&var) == VT_EMPTY) && (V_I4(&var) == 0xdeadbeef), "V_VT(var) = %d, V_I4(var) = %ld.\n",
       V_VT(&var), V_I4(&var));
    SysFreeString(script_str);

    language = SysAllocString(L"jscript");
    hr = IScriptControl_put_Language(sc, language);
    ok(hr == S_OK, "IScriptControl_put_Language failed: 0x%08lx.\n", hr);
    hr = IScriptControl_get_State(sc, &state);
    ok(hr == S_OK, "IScriptControl_get_State failed: 0x%08lx.\n", hr);
    ok(state == Initialized, "got wrong state: %d\n", state);
    SysFreeString(language);
    script_str = SysAllocString(L"var1 = 1 + 1");
    V_VT(&var) = VT_NULL;
    V_I4(&var) = 0xdeadbeef;
    hr = IScriptControl_Eval(sc, script_str, &var);
    ok(hr == S_OK, "IScriptControl_Eval failed: 0x%08lx.\n", hr);
    ok((V_VT(&var) == VT_I4) && (V_I4(&var) == 2), "V_VT(var) = %d, V_I4(var) = %ld.\n",
       V_VT(&var), V_I4(&var));
    hr = IScriptControl_get_State(sc, &state);
    ok(hr == S_OK, "IScriptControl_get_State failed: 0x%08lx.\n", hr);
    ok(state == Initialized, "got wrong state: %d\n", state);
    SysFreeString(script_str);

    script_str = SysAllocString(L"var2 = 10 + var1");
    V_VT(&var) = VT_NULL;
    V_I4(&var) = 0xdeadbeef;
    hr = IScriptControl_Eval(sc, script_str, &var);
    ok(hr == S_OK, "IScriptControl_Eval failed: 0x%08lx.\n", hr);
    ok((V_VT(&var) == VT_I4) && (V_I4(&var) == 12), "V_VT(var) = %d, V_I4(var) = %ld.\n",
       V_VT(&var), V_I4(&var));
    SysFreeString(script_str);

    script_str = SysAllocString(L"invalid syntax");
    V_VT(&var) = VT_NULL;
    V_I4(&var) = 0xdeadbeef;
    hr = IScriptControl_Eval(sc, script_str, &var);
    todo_wine ok(hr == 0x800a03ec, "IScriptControl_Eval failed: 0x%08lx.\n", hr);
    ok(V_VT(&var) == VT_EMPTY, "V_VT(var) = %d\n", V_VT(&var));
    ok(V_I4(&var) == 0xdeadbeef || broken(V_I4(&var) == 0) /* after Win8 */,
       "V_I4(var) = %ld.\n", V_I4(&var));
    SysFreeString(script_str);
    CHECK_ERROR_TODO(sc, 1004);

    script_str = SysAllocString(L"var2 = var1 + var2");
    V_VT(&var) = VT_NULL;
    V_I4(&var) = 0xdeadbeef;
    hr = IScriptControl_Eval(sc, script_str, &var);
    ok(hr == S_OK, "IScriptControl_Eval failed: 0x%08lx.\n", hr);
    ok((V_VT(&var) == VT_I4) && (V_I4(&var) == 14), "V_VT(var) = %d, V_I4(var) = %ld.\n",
       V_VT(&var), V_I4(&var));
    SysFreeString(script_str);

    script_str = SysAllocString(L"\"Hello\"");
    V_VT(&var) = VT_NULL;
    V_I4(&var) = 0xdeadbeef;
    hr = IScriptControl_Eval(sc, script_str, &var);
    ok(hr == S_OK, "IScriptControl_Eval failed: 0x%08lx.\n", hr);
    expected_string = SysAllocString(L"Hello");
    ok((V_VT(&var) == VT_BSTR) && (!lstrcmpW(V_BSTR(&var), expected_string)),
       "V_VT(var) = %d, V_BSTR(var) = %s.\n", V_VT(&var), wine_dbgstr_w(V_BSTR(&var)));
    SysFreeString(expected_string);
    SysFreeString(script_str);

    IScriptControl_Release(sc);

    if (have_custom_engine)
    {
        hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
                              &IID_IScriptControl, (void **)&sc);
        ok(hr == S_OK, "Failed to create IScriptControl interface: 0x%08lx.\n", hr);

        SET_EXPECT(CreateInstance);
        SET_EXPECT(SetInterfaceSafetyOptions);
        SET_EXPECT(SetScriptSite);
        SET_EXPECT(QI_IActiveScriptParse);
        SET_EXPECT(InitNew);

        language= SysAllocString(L"testscript");
        hr = IScriptControl_put_Language(sc, language);
        ok(hr == S_OK, "IScriptControl_put_Language failed: 0x%08lx.\n", hr);
        SysFreeString(language);

        CHECK_CALLED(CreateInstance);
        CHECK_CALLED(SetInterfaceSafetyOptions);
        CHECK_CALLED(SetScriptSite);
        CHECK_CALLED(QI_IActiveScriptParse);
        CHECK_CALLED(InitNew);

        SET_EXPECT(SetScriptState_STARTED);
        SET_EXPECT(ParseScriptText);
        parse_item_name = NULL;
        parse_flags = SCRIPTTEXT_ISEXPRESSION;
        script_str = SysAllocString(L"var1 = 1 + 1");
        V_VT(&var) = VT_NULL;
        hr = IScriptControl_Eval(sc, script_str, &var);
        ok(hr == S_OK, "IScriptControl_Eval failed: 0x%08lx.\n", hr);
        ok(V_VT(&var) == VT_EMPTY, "V_VT(var) = %d.\n", V_VT(&var));
        SysFreeString(script_str);
        CHECK_CALLED(SetScriptState_STARTED);
        CHECK_CALLED(ParseScriptText);

        SET_EXPECT(ParseScriptText);
        script_str = SysAllocString(L"var2 = 10 + var1");
        V_VT(&var) = VT_NULL;
        V_I4(&var) = 0xdeadbeef;
        hr = IScriptControl_Eval(sc, script_str, &var);
        ok(hr == S_OK, "IScriptControl_Eval failed: 0x%08lx.\n", hr);
        ok((V_VT(&var) == VT_EMPTY) && (V_I4(&var) == 0xdeadbeef), "V_VT(var) = %d, V_I4(var) = %ld.\n",
           V_VT(&var), V_I4(&var));
        SysFreeString(script_str);
        CHECK_CALLED(ParseScriptText);

        SET_EXPECT(SetScriptState_INITIALIZED);
        hr = IScriptControl_Reset(sc);
        ok(hr == S_OK, "IScriptControl_Reset failed: 0x%08lx.\n", hr);
        CHECK_CALLED(SetScriptState_INITIALIZED);

        SET_EXPECT(SetScriptState_STARTED);
        SET_EXPECT(ParseScriptText);
        script_str = SysAllocString(L"var2 = 10 + var1");
        V_VT(&var) = VT_NULL;
        V_I4(&var) = 0xdeadbeef;
        hr = IScriptControl_Eval(sc, script_str, &var);
        ok(hr == S_OK, "IScriptControl_Eval failed: 0x%08lx.\n", hr);
        ok((V_VT(&var) == VT_EMPTY) && (V_I4(&var) == 0xdeadbeef), "V_VT(var) = %d, V_I4(var) = %ld.\n",
           V_VT(&var), V_I4(&var));
        SysFreeString(script_str);
        CHECK_CALLED(SetScriptState_STARTED);
        CHECK_CALLED(ParseScriptText);

        IActiveScriptSite_Release(site);

        SET_EXPECT(Close);

        IScriptControl_Release(sc);

        CHECK_CALLED(Close);
    }
}

static void test_IScriptControl_AddCode(void)
{
    BSTR code_str, language;
    IScriptControl *sc;
    VARIANT var;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
                          &IID_IScriptControl, (void **)&sc);
    ok(hr == S_OK, "Failed to create IScriptControl interface: 0x%08lx.\n", hr);

    code_str = SysAllocString(L"1 + 1");
    hr = IScriptControl_AddCode(sc, code_str);
    ok(hr == E_FAIL, "IScriptControl_AddCode returned: 0x%08lx.\n", hr);
    SysFreeString(code_str);

    hr = IScriptControl_AddCode(sc, NULL);
    ok(hr == E_FAIL, "IScriptControl_AddCode returned: 0x%08lx.\n", hr);

    language = SysAllocString(L"jscript");
    hr = IScriptControl_put_Language(sc, language);
    ok(hr == S_OK, "IScriptControl_put_Language failed: 0x%08lx.\n", hr);
    SysFreeString(language);

    code_str = SysAllocString(L"1 + 1");
    hr = IScriptControl_AddCode(sc, code_str);
    ok(hr == S_OK, "IScriptControl_AddCode failed: 0x%08lx.\n", hr);
    SysFreeString(code_str);
    CHECK_ERROR(sc, 0);

    code_str = SysAllocString(L"invalid syntax");
    hr = IScriptControl_AddCode(sc, code_str);
    todo_wine ok(hr == 0x800a03ec, "IScriptControl_AddCode returned: 0x%08lx.\n", hr);
    SysFreeString(code_str);
    CHECK_ERROR_TODO(sc, 1004);

    IScriptControl_Release(sc);

    /* custom script engine */
    if (have_custom_engine)
    {
        hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
                              &IID_IScriptControl, (void **)&sc);
        ok(hr == S_OK, "Failed to create IScriptControl interface: 0x%08lx.\n", hr);

        SET_EXPECT(CreateInstance);
        SET_EXPECT(SetInterfaceSafetyOptions);
        SET_EXPECT(SetScriptSite);
        SET_EXPECT(QI_IActiveScriptParse);
        SET_EXPECT(InitNew);

        language = SysAllocString(L"testscript");
        hr = IScriptControl_put_Language(sc, language);
        ok(hr == S_OK, "IScriptControl_put_Language failed: 0x%08lx.\n", hr);
        SysFreeString(language);

        CHECK_CALLED(CreateInstance);
        CHECK_CALLED(SetInterfaceSafetyOptions);
        CHECK_CALLED(SetScriptSite);
        CHECK_CALLED(QI_IActiveScriptParse);
        CHECK_CALLED(InitNew);

        SET_EXPECT(SetScriptState_STARTED);
        SET_EXPECT(ParseScriptText);
        parse_item_name = NULL;
        parse_flags = SCRIPTTEXT_ISVISIBLE;
        code_str = SysAllocString(L"1 + 1");
        hr = IScriptControl_AddCode(sc, code_str);
        ok(hr == S_OK, "IScriptControl_AddCode failed: 0x%08lx.\n", hr);
        SysFreeString(code_str);
        CHECK_CALLED(SetScriptState_STARTED);
        CHECK_CALLED(ParseScriptText);

        SET_EXPECT(ParseScriptText);
        code_str = SysAllocString(L"0x100");
        hr = IScriptControl_AddCode(sc, code_str);
        ok(hr == S_OK, "IScriptControl_AddCode failed: 0x%08lx.\n", hr);
        SysFreeString(code_str);
        CHECK_CALLED(ParseScriptText);

        /* Call Eval() after AddCode() for checking if it will call SetScriptState() again. */
        SET_EXPECT(ParseScriptText);
        parse_item_name = NULL;
        parse_flags = SCRIPTTEXT_ISEXPRESSION;
        code_str = SysAllocString(L"var2 = 10 + var1");
        V_VT(&var) = VT_NULL;
        hr = IScriptControl_Eval(sc, code_str, &var);
        ok(hr == S_OK, "IScriptControl_Eval failed: 0x%08lx.\n", hr);
        ok(V_VT(&var) == VT_EMPTY, "V_VT(var) = %d.\n", V_VT(&var));
        SysFreeString(code_str);
        CHECK_CALLED(ParseScriptText);

        IActiveScriptSite_Release(site);

        SET_EXPECT(Close);

        IScriptControl_Release(sc);

        CHECK_CALLED(Close);
    }
}

static void test_IScriptControl_ExecuteStatement(void)
{
    IScriptControl *sc;
    HRESULT hr;
    BSTR str;

    hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
                          &IID_IScriptControl, (void**)&sc);
    ok(hr == S_OK, "Failed to create IScriptControl interface: 0x%08lx.\n", hr);

    str = SysAllocString(L"1 + 1");
    hr = IScriptControl_ExecuteStatement(sc, str);
    ok(hr == E_FAIL, "IScriptControl_ExecuteStatement returned: 0x%08lx.\n", hr);
    SysFreeString(str);

    hr = IScriptControl_ExecuteStatement(sc, NULL);
    ok(hr == E_FAIL, "IScriptControl_ExecuteStatement returned: 0x%08lx.\n", hr);

    str = SysAllocString(L"jscript");
    hr = IScriptControl_put_Language(sc, str);
    ok(hr == S_OK, "IScriptControl_put_Language failed: 0x%08lx.\n", hr);
    SysFreeString(str);

    str = SysAllocString(L"1 + 1");
    hr = IScriptControl_ExecuteStatement(sc, str);
    ok(hr == S_OK, "IScriptControl_ExecuteStatement failed: 0x%08lx.\n", hr);
    SysFreeString(str);
    CHECK_ERROR(sc, 0);

    str = SysAllocString(L"invalid syntax");
    hr = IScriptControl_ExecuteStatement(sc, str);
    todo_wine ok(hr == 0x800a03ec, "IScriptControl_ExecuteStatement returned: 0x%08lx.\n", hr);
    SysFreeString(str);
    CHECK_ERROR_TODO(sc, 1004);

    IScriptControl_Release(sc);

    /* custom script engine */
    if (have_custom_engine)
    {
        hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
                              &IID_IScriptControl, (void**)&sc);
        ok(hr == S_OK, "Failed to create IScriptControl interface: 0x%08lx.\n", hr);

        SET_EXPECT(CreateInstance);
        SET_EXPECT(SetInterfaceSafetyOptions);
        SET_EXPECT(SetScriptSite);
        SET_EXPECT(QI_IActiveScriptParse);
        SET_EXPECT(InitNew);

        str = SysAllocString(L"testscript");
        hr = IScriptControl_put_Language(sc, str);
        ok(hr == S_OK, "IScriptControl_put_Language failed: 0x%08lx.\n", hr);
        SysFreeString(str);

        CHECK_CALLED(CreateInstance);
        CHECK_CALLED(SetInterfaceSafetyOptions);
        CHECK_CALLED(SetScriptSite);
        CHECK_CALLED(QI_IActiveScriptParse);
        CHECK_CALLED(InitNew);

        SET_EXPECT(SetScriptState_STARTED);
        SET_EXPECT(ParseScriptText);
        parse_item_name = NULL;
        parse_flags = 0;
        str = SysAllocString(L"1 + 1");
        hr = IScriptControl_ExecuteStatement(sc, str);
        ok(hr == S_OK, "IScriptControl_ExecuteStatement failed: 0x%08lx.\n", hr);
        SysFreeString(str);
        CHECK_CALLED(SetScriptState_STARTED);
        CHECK_CALLED(ParseScriptText);

        SET_EXPECT(ParseScriptText);
        str = SysAllocString(L"0x100");
        hr = IScriptControl_ExecuteStatement(sc, str);
        ok(hr == S_OK, "IScriptControl_ExecuteStatement failed: 0x%08lx.\n", hr);
        SysFreeString(str);
        CHECK_CALLED(ParseScriptText);

        IActiveScriptSite_Release(site);

        SET_EXPECT(Close);
        IScriptControl_Release(sc);
        CHECK_CALLED(Close);
    }
}

static void test_IScriptControl_Run(void)
{
    SAFEARRAYBOUND bnd[] = { { 2, 0 }, { 2, 0 } };
    LONG idx0_0[] = { 0, 0 };
    LONG idx0_1[] = { 1, 0 };
    LONG idx1_0[] = { 0, 1 };
    LONG idx1_1[] = { 1, 1 };
    IScriptControl *sc;
    SAFEARRAY *params;
    VARIANT var;
    HRESULT hr;
    BSTR str;

    hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
                          &IID_IScriptControl, (void**)&sc);
    ok(hr == S_OK, "Failed to create IScriptControl interface: 0x%08lx.\n", hr);

    params = NULL;
    str = SysAllocString(L"identifier");
    hr = IScriptControl_Run(sc, str, &params, &var);
    ok(hr == E_POINTER, "IScriptControl_Run returned: 0x%08lx.\n", hr);

    params = SafeArrayCreate(VT_VARIANT, 1, bnd);
    ok(params != NULL, "Failed to create SafeArray.\n");

    V_VT(&var) = VT_I4;
    V_I4(&var) = 10;
    SafeArrayPutElement(params, idx0_0, &var);
    V_I4(&var) = 3;
    SafeArrayPutElement(params, idx0_1, &var);

    hr = IScriptControl_Run(sc, str, &params, &var);
    ok(hr == E_FAIL, "IScriptControl_Run returned: 0x%08lx.\n", hr);

    hr = IScriptControl_Run(sc, str, NULL, &var);
    ok(hr == E_POINTER, "IScriptControl_Run returned: 0x%08lx.\n", hr);

    hr = IScriptControl_Run(sc, str, &params, NULL);
    ok(hr == E_POINTER, "IScriptControl_Run returned: 0x%08lx.\n", hr);
    SysFreeString(str);

    hr = IScriptControl_Run(sc, NULL, &params, &var);
    ok(hr == E_FAIL, "IScriptControl_Run returned: 0x%08lx.\n", hr);

    str = SysAllocString(L"jscript");
    hr = IScriptControl_put_Language(sc, str);
    ok(hr == S_OK, "IScriptControl_put_Language failed: 0x%08lx.\n", hr);
    SysFreeString(str);

    str = SysAllocString(L"foobar");
    hr = IScriptControl_Run(sc, str, &params, &var);
    ok(hr == DISP_E_UNKNOWNNAME, "IScriptControl_Run failed: 0x%08lx.\n", hr);
    CHECK_ERROR(sc, 0);
    SysFreeString(str);

    str = SysAllocString(L"function subtract(a, b) { return a - b; }\n");
    hr = IScriptControl_AddCode(sc, str);
    ok(hr == S_OK, "IScriptControl_AddCode failed: 0x%08lx.\n", hr);
    CHECK_ERROR(sc, 0);
    SysFreeString(str);

    str = SysAllocString(L"Subtract");
    hr = IScriptControl_Run(sc, str, &params, &var);
    ok(hr == DISP_E_UNKNOWNNAME, "IScriptControl_Run failed: 0x%08lx.\n", hr);
    SysFreeString(str);

    str = SysAllocString(L"subtract");
    hr = IScriptControl_Run(sc, str, &params, NULL);
    ok(hr == E_POINTER, "IScriptControl_Run failed: 0x%08lx.\n", hr);
    CHECK_ERROR(sc, 0);

    hr = IScriptControl_Run(sc, str, &params, &var);
    ok(hr == S_OK, "IScriptControl_Run failed: 0x%08lx.\n", hr);
    ok((V_VT(&var) == VT_I4) && (V_I4(&var) == 7), "V_VT(var) = %d, V_I4(var) = %ld.\n", V_VT(&var), V_I4(&var));
    CHECK_ERROR(sc, 0);
    SafeArrayDestroy(params);

    /* The array's other dimensions are ignored */
    params = SafeArrayCreate(VT_VARIANT, 2, bnd);
    ok(params != NULL, "Failed to create SafeArray.\n");

    V_VT(&var) = VT_I4;
    V_I4(&var) = 10;
    SafeArrayPutElement(params, idx0_0, &var);
    V_I4(&var) = 3;
    SafeArrayPutElement(params, idx0_1, &var);
    V_I4(&var) = 90;
    SafeArrayPutElement(params, idx1_0, &var);
    V_I4(&var) = 80;
    SafeArrayPutElement(params, idx1_1, &var);

    hr = IScriptControl_Run(sc, str, &params, &var);
    ok(hr == S_OK, "IScriptControl_Run failed: 0x%08lx.\n", hr);
    ok((V_VT(&var) == VT_I4) && (V_I4(&var) == 7), "V_VT(var) = %d, V_I4(var) = %ld.\n", V_VT(&var), V_I4(&var));

    /* Hack the array's dimensions to 0 */
    params->cDims = 0;
    hr = IScriptControl_Run(sc, str, &params, &var);
    ok(hr == DISP_E_BADINDEX, "IScriptControl_Run returned: 0x%08lx.\n", hr);
    ok(V_VT(&var) == VT_EMPTY, "V_VT(var) = %d.\n", V_VT(&var));
    params->cDims = 2;
    SysFreeString(str);
    IScriptControl_Release(sc);

    /* custom script engine */
    if (have_custom_engine)
    {
        hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
                              &IID_IScriptControl, (void**)&sc);
        ok(hr == S_OK, "Failed to create IScriptControl interface: 0x%08lx.\n", hr);

        SET_EXPECT(CreateInstance);
        SET_EXPECT(SetInterfaceSafetyOptions);
        SET_EXPECT(SetScriptSite);
        SET_EXPECT(QI_IActiveScriptParse);
        SET_EXPECT(InitNew);
        str = SysAllocString(L"testscript");
        hr = IScriptControl_put_Language(sc, str);
        ok(hr == S_OK, "IScriptControl_put_Language failed: 0x%08lx.\n", hr);
        SysFreeString(str);
        CHECK_CALLED(CreateInstance);
        CHECK_CALLED(SetInterfaceSafetyOptions);
        CHECK_CALLED(SetScriptSite);
        CHECK_CALLED(QI_IActiveScriptParse);
        CHECK_CALLED(InitNew);

        GetScriptDispatch_expected_name = NULL;
        SET_EXPECT(SetScriptState_STARTED);
        SET_EXPECT(GetScriptDispatch);
        SET_EXPECT(QI_IDispatchEx);
        SET_EXPECT(GetIDsOfNames);
        SET_EXPECT(Invoke);
        Dispatch_expected_name = SysAllocString(L"function");
        hr = IScriptControl_Run(sc, Dispatch_expected_name, &params, &var);
        ok(hr == S_OK, "IScriptControl_Run failed: 0x%08lx.\n", hr);
        ok((V_VT(&var) == VT_R8) && (V_R8(&var) == 4.2), "V_VT(var) = %d, V_R8(var) = %lf.\n", V_VT(&var), V_R8(&var));
        SysFreeString(Dispatch_expected_name);
        CHECK_CALLED(SetScriptState_STARTED);
        CHECK_CALLED(GetScriptDispatch);
        CHECK_CALLED(QI_IDispatchEx);
        CHECK_CALLED(GetIDsOfNames);
        CHECK_CALLED(Invoke);

        /* GetScriptDispatch is cached and not called again */
        CLEAR_CALLED(GetScriptDispatch);
        SET_EXPECT(QI_IDispatchEx);
        SET_EXPECT(GetIDsOfNames);
        SET_EXPECT(Invoke);
        Dispatch_expected_name = SysAllocString(L"BarFoo");
        hr = IScriptControl_Run(sc, Dispatch_expected_name, &params, &var);
        ok(hr == S_OK, "IScriptControl_Run failed: 0x%08lx.\n", hr);
        SysFreeString(Dispatch_expected_name);
        CHECK_NOT_CALLED(GetScriptDispatch);
        CHECK_CALLED(QI_IDispatchEx);
        CHECK_CALLED(GetIDsOfNames);
        CHECK_CALLED(Invoke);

        /* Make DispatchEx available */
        DispatchEx_available = TRUE;
        CLEAR_CALLED(GetScriptDispatch);
        SET_EXPECT(QI_IDispatchEx);
        SET_EXPECT(GetIDsOfNames);
        SET_EXPECT(InvokeEx);
        Dispatch_expected_name = SysAllocString(L"FooBar");
        hr = IScriptControl_Run(sc, Dispatch_expected_name, &params, &var);
        ok(hr == S_OK, "IScriptControl_Run failed: 0x%08lx.\n", hr);
        ok((V_VT(&var) == VT_I2) && (V_I2(&var) == 42), "V_VT(var) = %d, V_I2(var) = %d.\n", V_VT(&var), V_I2(&var));
        SysFreeString(Dispatch_expected_name);
        CHECK_NOT_CALLED(GetScriptDispatch);
        CHECK_CALLED(QI_IDispatchEx);
        CHECK_CALLED(GetIDsOfNames);
        CHECK_CALLED(InvokeEx);

        /* QueryInterface for IDispatchEx is always called and not cached */
        CLEAR_CALLED(GetScriptDispatch);
        SET_EXPECT(QI_IDispatchEx);
        SET_EXPECT(GetIDsOfNames);
        SET_EXPECT(InvokeEx);
        Dispatch_expected_name = SysAllocString(L"1");
        hr = IScriptControl_Run(sc, Dispatch_expected_name, &params, &var);
        ok(hr == S_OK, "IScriptControl_Run failed: 0x%08lx.\n", hr);
        SysFreeString(Dispatch_expected_name);
        CHECK_NOT_CALLED(GetScriptDispatch);
        CHECK_CALLED(QI_IDispatchEx);
        CHECK_CALLED(GetIDsOfNames);
        CHECK_CALLED(InvokeEx);
        DispatchEx_available = FALSE;

        IActiveScriptSite_Release(site);

        SET_EXPECT(Close);
        IScriptControl_Release(sc);
        CHECK_CALLED(Close);
    }

    SafeArrayDestroy(params);
}

static void test_IScriptControl_get_Modules(void)
{
    SAFEARRAYBOUND bnd[] = { { 2, 0 }, { 2, 0 } };
    LONG idx0_0[] = { 0, 0 };
    LONG idx0_1[] = { 1, 0 };
    LONG idx1_0[] = { 0, 1 };
    LONG idx1_1[] = { 1, 1 };

    IScriptProcedureCollection *procs;
    IEnumVARIANT *enumvar, *enumvar2;
    IScriptModuleCollection *mods;
    VARIANT var, vars[3];
    IScriptModule *mod;
    IScriptControl *sc;
    SAFEARRAY *params;
    IUnknown *unknown;
    IDispatch *disp;
    ULONG fetched;
    LONG count;
    HRESULT hr;
    DISPID id;
    BSTR str;
    UINT i;

    hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
                          &IID_IScriptControl, (void**)&sc);
    ok(hr == S_OK, "Failed to create IScriptControl interface: 0x%08lx.\n", hr);

    hr = IScriptControl_get_Modules(sc, &mods);
    ok(hr == E_FAIL, "IScriptControl_get_Modules returned: 0x%08lx.\n", hr);

    str = SysAllocString(L"jscript");
    hr = IScriptControl_put_Language(sc, str);
    ok(hr == S_OK, "IScriptControl_put_Language failed: 0x%08lx.\n", hr);
    SysFreeString(str);

    hr = IScriptControl_get_Modules(sc, &mods);
    ok(hr == S_OK, "IScriptControl_get_Modules failed: 0x%08lx.\n", hr);

    hr = IScriptModuleCollection_get_Count(mods, NULL);
    ok(hr == E_POINTER, "IScriptModuleCollection_get_Count returned: 0x%08lx.\n", hr);
    count = 0;
    hr = IScriptModuleCollection_get_Count(mods, &count);
    ok(hr == S_OK, "IScriptModuleCollection_get_Count failed: 0x%08lx.\n", hr);
    ok(count == 1, "count is not 1, got %ld.\n", count);

    V_VT(&var) = VT_I4;
    V_I4(&var) = -1;
    hr = IScriptModuleCollection_get_Item(mods, var, NULL);
    ok(hr == E_POINTER, "IScriptModuleCollection_get_Item returned: 0x%08lx.\n", hr);
    hr = IScriptModuleCollection_get_Item(mods, var, &mod);
    ok(hr == 0x800a0009, "IScriptModuleCollection_get_Item returned: 0x%08lx.\n", hr);

    V_VT(&var) = VT_EMPTY;
    str = SysAllocString(L"foobar");
    hr = IScriptModuleCollection_Add(mods, str, &var, &mod);
    ok(hr == E_INVALIDARG, "IScriptModuleCollection_Add returned: 0x%08lx.\n", hr);
    hr = IScriptModuleCollection_Add(mods, str, &var, NULL);
    ok(hr == E_POINTER, "IScriptModuleCollection_Add returned: 0x%08lx.\n", hr);
    V_VT(&var) = VT_DISPATCH;
    V_DISPATCH(&var) = NULL;
    hr = IScriptModuleCollection_Add(mods, NULL, &var, &mod);
    ok(hr == E_INVALIDARG, "IScriptModuleCollection_Add returned: 0x%08lx.\n", hr);
    hr = IScriptModuleCollection_Add(mods, str, &var, &mod);
    ok(hr == S_OK, "IScriptModuleCollection_Add failed: 0x%08lx.\n", hr);
    IScriptModule_Release(mod);
    SysFreeString(str);

    /* Grab an enumerator before we add another module */
    hr = IScriptModuleCollection_get__NewEnum(mods, NULL);
    ok(hr == E_POINTER, "IScriptModuleCollection_get__NewEnum returned: 0x%08lx.\n", hr);
    hr = IScriptModuleCollection_get__NewEnum(mods, &unknown);
    ok(hr == S_OK, "IScriptModuleCollection_get__NewEnum failed: 0x%08lx.\n", hr);
    hr = IUnknown_QueryInterface(unknown, &IID_IEnumVARIANT, (void**)&enumvar);
    ok(hr == S_OK, "Failed to query for IEnumVARIANT: 0x%08lx.\n", hr);
    ok((char*)unknown == (char*)enumvar, "unknown and enumvar are not the same (%p vs %p).\n", unknown, enumvar);
    IUnknown_Release(unknown);

    str = SysAllocString(L"some other Module");
    hr = IScriptModuleCollection_Add(mods, str, &var, &mod);
    ok(hr == S_OK, "IScriptModuleCollection_Add failed: 0x%08lx.\n", hr);
    IScriptModule_Release(mod);
    SysFreeString(str);

    /* Adding a module with the same name is invalid (case insensitive) */
    str = SysAllocString(L"FooBar");
    hr = IScriptModuleCollection_Add(mods, str, &var, &mod);
    ok(hr == E_INVALIDARG, "IScriptModuleCollection_Add failed: 0x%08lx.\n", hr);
    SysFreeString(str);

    hr = IScriptModuleCollection_get_Count(mods, &count);
    ok(hr == S_OK, "IScriptModuleCollection_get_Count failed: 0x%08lx.\n", hr);
    ok(count == 3, "count is not 3, got %ld.\n", count);
    V_VT(&var) = VT_I4;
    V_I4(&var) = count + 1;
    hr = IScriptModuleCollection_get_Item(mods, var, &mod);
    ok(hr == 0x800a0009, "IScriptModuleCollection_get_Item returned: 0x%08lx.\n", hr);
    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(L"non-existent module");
    hr = IScriptModuleCollection_get_Item(mods, var, &mod);
    ok(hr == CTL_E_ILLEGALFUNCTIONCALL, "IScriptModuleCollection_get_Item returned: 0x%08lx.\n", hr);
    ok(V_VT(&var) == VT_BSTR, "var type not BSTR, got %d.\n", V_VT(&var));
    VariantClear(&var);

    V_VT(&var) = VT_I4;
    V_I4(&var) = 1;
    hr = IScriptModuleCollection_get_Item(mods, var, &mod);
    ok(hr == S_OK, "IScriptModuleCollection_get_Item failed: 0x%08lx.\n", hr);
    hr = IScriptModule_get_Name(mod, NULL);
    ok(hr == E_POINTER, "IScriptModule_get_Name returned: 0x%08lx.\n", hr);
    hr = IScriptModule_get_Name(mod, &str);
    ok(hr == S_OK, "IScriptModule_get_Name failed: 0x%08lx.\n", hr);
    ok(!lstrcmpW(str, L"Global"), "got %s.\n", wine_dbgstr_w(str));
    SysFreeString(str);
    str = SysAllocString(L"function add(a, b) { return a + b; }\n");
    hr = IScriptModule_AddCode(mod, str);
    ok(hr == S_OK, "IScriptModule_AddCode failed: 0x%08lx.\n", hr);
    IScriptModule_Release(mod);
    SysFreeString(str);

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(L"some other module");
    hr = IScriptModuleCollection_get_Item(mods, var, &mod);
    ok(hr == S_OK, "IScriptModuleCollection_get_Item failed: 0x%08lx.\n", hr);
    ok(V_VT(&var) == VT_BSTR, "var type not BSTR, got %d.\n", V_VT(&var));
    VariantClear(&var);
    hr = IScriptModule_get_Name(mod, &str);
    ok(hr == S_OK, "IScriptModule_get_Name failed: 0x%08lx.\n", hr);
    ok(!lstrcmpW(str, L"some other Module"), "got %s.\n", wine_dbgstr_w(str));
    IScriptModule_Release(mod);
    SysFreeString(str);

    V_VT(&var) = VT_R8;
    V_R8(&var) = 2.0;
    hr = IScriptModuleCollection_get_Item(mods, var, &mod);
    ok(hr == S_OK, "IScriptModuleCollection_get_Item failed: 0x%08lx.\n", hr);
    hr = IScriptModule_get_Name(mod, &str);
    ok(hr == S_OK, "IScriptModule_get_Name failed: 0x%08lx.\n", hr);
    ok(!lstrcmpW(str, L"foobar"), "got %s.\n", wine_dbgstr_w(str));
    SysFreeString(str);
    str = SysAllocString(L"function sub(a, b) { return a - b; }\n");
    hr = IScriptModule_AddCode(mod, str);
    ok(hr == S_OK, "IScriptModule_AddCode failed: 0x%08lx.\n", hr);
    IScriptModule_Release(mod);
    SysFreeString(str);

    /* Test the enumerator, should be updated */
    fetched = 0xdeadbeef;
    hr = IEnumVARIANT_Next(enumvar, 0, NULL, NULL);
    ok(hr == E_POINTER, "IEnumVARIANT_Next returned: 0x%08lx.\n", hr);
    hr = IEnumVARIANT_Next(enumvar, 0, NULL, &fetched);
    ok(hr == E_POINTER, "IEnumVARIANT_Next failed: 0x%08lx.\n", hr);
    ok(fetched == 0xdeadbeef, "got %lu.\n", fetched);
    hr = IEnumVARIANT_Next(enumvar, 0, &var, &fetched);
    ok(hr == S_OK, "IEnumVARIANT_Next returned: 0x%08lx.\n", hr);
    ok(fetched == 0, "got %lu.\n", fetched);
    hr = IEnumVARIANT_Next(enumvar, 0, &var, NULL);
    ok(hr == S_OK, "IEnumVARIANT_Next returned: 0x%08lx.\n", hr);
    hr = IEnumVARIANT_Clone(enumvar, NULL);
    ok(hr == E_POINTER, "IEnumVARIANT_Clone failed: 0x%08lx.\n", hr);

    hr = IEnumVARIANT_Next(enumvar, ARRAY_SIZE(vars), vars, &fetched);
    ok(hr == S_OK, "IEnumVARIANT_Next failed: 0x%08lx.\n", hr);
    ok(fetched == ARRAY_SIZE(vars), "got %lu.\n", fetched);
    hr = IEnumVARIANT_Next(enumvar, 1, &var, &fetched);
    ok(hr == S_FALSE, "IEnumVARIANT_Next failed: 0x%08lx.\n", hr);
    ok(fetched == 0, "got %lu.\n", fetched);
    hr = IEnumVARIANT_Skip(enumvar, 0);
    ok(hr == S_OK, "IEnumVARIANT_Skip failed: 0x%08lx.\n", hr);
    hr = IEnumVARIANT_Skip(enumvar, 1);
    ok(hr == S_FALSE, "IEnumVARIANT_Skip failed: 0x%08lx.\n", hr);
    hr = IEnumVARIANT_Clone(enumvar, &enumvar2);
    ok(hr == S_OK, "IEnumVARIANT_Clone failed: 0x%08lx.\n", hr);
    hr = IEnumVARIANT_Skip(enumvar2, 1);
    ok(hr == S_FALSE, "IEnumVARIANT_Skip failed: 0x%08lx.\n", hr);
    IEnumVARIANT_Release(enumvar2);

    for (i = 0; i < ARRAY_SIZE(vars); i++)
    {
        ok(V_VT(&vars[i]) == VT_DISPATCH, "V_VT(vars[%u]) = %d.\n", i, V_VT(&vars[i]));
        hr = IDispatch_QueryInterface(V_DISPATCH(&vars[i]), &IID_IScriptModule, (void**)&mod);
        ok(hr == S_OK, "Failed to query IScriptModule from vars[%u]: 0x%08lx.\n", i, hr);
        IScriptModule_Release(mod);
        VariantClear(&vars[i]);
    }

    /* The 'Global' module is the same as the script control */
    str = SysAllocString(L"add(10, 5)");
    hr = IScriptControl_Eval(sc, str, &var);
    ok(hr == S_OK, "IScriptControl_Eval failed: 0x%08lx.\n", hr);
    ok(V_VT(&var) == VT_I4 && V_I4(&var) == 15, "V_VT(var) = %d, V_I4(var) = %ld.\n", V_VT(&var), V_I4(&var));
    SysFreeString(str);
    str = SysAllocString(L"sub(10, 5)");
    hr = IScriptControl_Eval(sc, str, &var);
    ok(FAILED(hr), "IScriptControl_Eval succeeded: 0x%08lx.\n", hr);
    SysFreeString(str);

    V_VT(&var) = VT_R4;
    V_R4(&var) = 2.0f;
    hr = IScriptModuleCollection_get_Item(mods, var, &mod);
    ok(hr == S_OK, "IScriptModuleCollection_get_Item failed: 0x%08lx.\n", hr);

    hr = IScriptModule_get_CodeObject(mod, &disp);
    ok(hr == S_OK, "IScriptModule_get_CodeObject failed: 0x%08lx.\n", hr);

    str = SysAllocString(L"sub");
    hr = IDispatch_GetIDsOfNames(disp, &IID_NULL, &str, 1, LOCALE_USER_DEFAULT, &id);
    ok(hr == S_OK, "IDispatch_GetIDsOfNames failed: 0x%08lx.\n", hr);
    ok(id != -1, "Unexpected id %ld.\n", id);
    SysFreeString(str);

    str = SysAllocString(L"add");
    hr = IDispatch_GetIDsOfNames(disp, &IID_NULL, &str, 1, LOCALE_USER_DEFAULT, &id);
    ok(hr == DISP_E_UNKNOWNNAME, "IDispatch_GetIDsOfNames returned: 0x%08lx.\n", hr);
    ok(id == -1, "Unexpected id %ld.\n", id);
    SysFreeString(str);

    IDispatch_Release(disp);

    params = SafeArrayCreate(VT_VARIANT, 1, bnd);
    ok(params != NULL, "Failed to create SafeArray.\n");

    V_VT(&var) = VT_I4;
    V_I4(&var) = 10;
    SafeArrayPutElement(params, idx0_0, &var);
    V_I4(&var) = 3;
    SafeArrayPutElement(params, idx0_1, &var);

    str = SysAllocString(L"sub");
    hr = IScriptModule_Run(mod, str, NULL, &var);
    ok(hr == E_POINTER, "IScriptModule_Run returned: 0x%08lx.\n", hr);
    hr = IScriptModule_Run(mod, str, &params, NULL);
    ok(hr == E_POINTER, "IScriptModule_Run returned: 0x%08lx.\n", hr);

    hr = IScriptControl_Run(sc, str, &params, &var);
    ok(hr == DISP_E_UNKNOWNNAME, "IScriptControl_Run failed: 0x%08lx.\n", hr);
    hr = IScriptModule_Run(mod, str, &params, &var);
    ok(hr == S_OK, "IScriptModule_Run failed: 0x%08lx.\n", hr);
    ok((V_VT(&var) == VT_I4) && (V_I4(&var) == 7), "V_VT(var) = %d, V_I4(var) = %ld.\n", V_VT(&var), V_I4(&var));
    SysFreeString(str);

    str = SysAllocString(L"add");
    hr = IScriptControl_Run(sc, str, &params, &var);
    ok(hr == S_OK, "IScriptControl_Run failed: 0x%08lx.\n", hr);
    ok((V_VT(&var) == VT_I4) && (V_I4(&var) == 13), "V_VT(var) = %d, V_I4(var) = %ld.\n", V_VT(&var), V_I4(&var));
    hr = IScriptModule_Run(mod, str, &params, &var);
    ok(hr == DISP_E_UNKNOWNNAME, "IScriptModule_Run failed: 0x%08lx.\n", hr);
    SysFreeString(str);

    SafeArrayDestroy(params);
    params = SafeArrayCreate(VT_VARIANT, 2, bnd);
    ok(params != NULL, "Failed to create SafeArray.\n");

    V_VT(&var) = VT_I4;
    V_I4(&var) = 49;
    SafeArrayPutElement(params, idx0_0, &var);
    V_I4(&var) = 7;
    SafeArrayPutElement(params, idx0_1, &var);
    V_I4(&var) = 30;
    SafeArrayPutElement(params, idx1_0, &var);
    V_I4(&var) = 25;
    SafeArrayPutElement(params, idx1_1, &var);

    str = SysAllocString(L"sub");
    hr = IScriptModule_Run(mod, str, &params, &var);
    ok(hr == S_OK, "IScriptModule_Run failed: 0x%08lx.\n", hr);
    ok((V_VT(&var) == VT_I4) && (V_I4(&var) == 42), "V_VT(var) = %d, V_I4(var) = %ld.\n", V_VT(&var), V_I4(&var));

    params->cDims = 0;
    hr = IScriptModule_Run(mod, str, &params, &var);
    ok(hr == DISP_E_BADINDEX, "IScriptModule_Run returned: 0x%08lx.\n", hr);
    ok(V_VT(&var) == VT_EMPTY, "V_VT(var) = %d.\n", V_VT(&var));
    params->cDims = 2;
    SysFreeString(str);

    IScriptModule_Release(mod);

    /* Grab a module ref and change the language to something valid */
    V_VT(&var) = VT_I2;
    V_I2(&var) = 3;
    hr = IScriptModuleCollection_get_Item(mods, var, &mod);
    ok(hr == S_OK, "IScriptModuleCollection_get_Item failed: 0x%08lx.\n", hr);
    str = SysAllocString(L"vbscript");
    hr = IScriptControl_put_Language(sc, str);
    ok(hr == S_OK, "IScriptControl_put_Language failed: 0x%08lx.\n", hr);
    SysFreeString(str);

    /* The module collection changes and module ref is invalid */
    hr = IScriptModuleCollection_get_Count(mods, &count);
    ok(hr == S_OK, "IScriptModuleCollection_get_Count failed: 0x%08lx.\n", hr);
    ok(count == 1, "count is not 1, got %ld.\n", count);
    hr = IScriptModule_get_Name(mod, &str);
    ok(hr == E_FAIL, "IScriptModule_get_Name returned: 0x%08lx.\n", hr);
    hr = IScriptModule_get_CodeObject(mod, &disp);
    ok(hr == E_FAIL, "IScriptModule_get_CodeObject returned: 0x%08lx.\n", hr);
    hr = IScriptModule_get_Procedures(mod, &procs);
    ok(hr == E_FAIL, "IScriptModule_get_Procedures returned: 0x%08lx.\n", hr);
    str = SysAllocString(L"function closed() { }\n");
    hr = IScriptModule_AddCode(mod, str);
    ok(hr == E_FAIL, "IScriptModule_AddCode failed: 0x%08lx.\n", hr);
    SysFreeString(str);
    str = SysAllocString(L"sub closed\nend sub");
    hr = IScriptModule_AddCode(mod, str);
    ok(hr == E_FAIL, "IScriptModule_AddCode failed: 0x%08lx.\n", hr);
    SysFreeString(str);
    str = SysAllocString(L"identifier");
    hr = IScriptModule_Run(mod, str, &params, &var);
    ok(hr == E_FAIL, "IScriptModule_Run returned: 0x%08lx.\n", hr);
    IScriptModule_Release(mod);
    SafeArrayDestroy(params);
    SysFreeString(str);

    /* The enumerator is also invalid */
    hr = IEnumVARIANT_Reset(enumvar);
    ok(hr == E_FAIL, "IEnumVARIANT_Skip returned: 0x%08lx.\n", hr);
    hr = IEnumVARIANT_Next(enumvar, 1, &var, &fetched);
    ok(hr == E_FAIL, "IEnumVARIANT_Next returned: 0x%08lx.\n", hr);
    hr = IEnumVARIANT_Skip(enumvar, 0);
    ok(hr == E_FAIL, "IEnumVARIANT_Skip returned: 0x%08lx.\n", hr);
    hr = IEnumVARIANT_Clone(enumvar, &enumvar2);
    ok(hr == E_FAIL, "IEnumVARIANT_Clone returned: 0x%08lx.\n", hr);
    IEnumVARIANT_Release(enumvar);

    hr = IScriptControl_put_Language(sc, NULL);
    ok(hr == S_OK, "IScriptControl_put_Language failed: 0x%08lx.\n", hr);

    hr = IScriptModuleCollection_get_Count(mods, &count);
    ok(hr == E_FAIL, "IScriptModuleCollection_get_Count returned: 0x%08lx.\n", hr);
    V_VT(&var) = VT_I4;
    V_I4(&var) = 1;
    hr = IScriptModuleCollection_get_Item(mods, var, &mod);
    ok(hr == E_FAIL, "IScriptModuleCollection_get_Item returned: 0x%08lx.\n", hr);
    V_VT(&var) = VT_DISPATCH;
    V_DISPATCH(&var) = NULL;
    str = SysAllocString(L"module_name");
    hr = IScriptModuleCollection_Add(mods, str, &var, &mod);
    ok(hr == E_FAIL, "IScriptModuleCollection_Add returned: 0x%08lx.\n", hr);
    SysFreeString(str);
    hr = IScriptModuleCollection_get__NewEnum(mods, &unknown);
    ok(hr == E_FAIL, "IScriptModuleCollection_get__NewEnum returned: 0x%08lx.\n", hr);

    IScriptModuleCollection_Release(mods);
    hr = IScriptControl_get_Modules(sc, &mods);
    ok(hr == E_FAIL, "IScriptControl_get_Modules returned: 0x%08lx.\n", hr);

    IScriptControl_Release(sc);

    /* custom script engine */
    if (have_custom_engine)
    {
        BSTR code_str;

        /* A module collection ref keeps the control alive */
        hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
                              &IID_IScriptControl, (void**)&sc);
        ok(hr == S_OK, "Failed to create IScriptControl interface: 0x%08lx.\n", hr);

        SET_EXPECT(CreateInstance);
        SET_EXPECT(SetInterfaceSafetyOptions);
        SET_EXPECT(SetScriptSite);
        SET_EXPECT(QI_IActiveScriptParse);
        SET_EXPECT(InitNew);
        str = SysAllocString(L"testscript");
        hr = IScriptControl_put_Language(sc, str);
        ok(hr == S_OK, "IScriptControl_put_Language failed: 0x%08lx.\n", hr);
        SysFreeString(str);
        CHECK_CALLED(CreateInstance);
        CHECK_CALLED(SetInterfaceSafetyOptions);
        CHECK_CALLED(SetScriptSite);
        CHECK_CALLED(QI_IActiveScriptParse);
        CHECK_CALLED(InitNew);

        hr = IScriptControl_get_Modules(sc, &mods);
        ok(hr == S_OK, "IScriptControl_get_Modules failed: 0x%08lx.\n", hr);

        IActiveScriptSite_Release(site);
        IScriptControl_Release(sc);

        SET_EXPECT(Close);
        IScriptModuleCollection_Release(mods);
        CHECK_CALLED(Close);

        /* Add a module with a non-null object and add some code to it */
        hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
                              &IID_IScriptControl, (void**)&sc);
        ok(hr == S_OK, "Failed to create IScriptControl interface: 0x%08lx.\n", hr);

        SET_EXPECT(CreateInstance);
        SET_EXPECT(SetInterfaceSafetyOptions);
        SET_EXPECT(SetScriptSite);
        SET_EXPECT(QI_IActiveScriptParse);
        SET_EXPECT(InitNew);
        str = SysAllocString(L"testscript");
        hr = IScriptControl_put_Language(sc, str);
        ok(hr == S_OK, "IScriptControl_put_Language failed: 0x%08lx.\n", hr);
        SysFreeString(str);
        CHECK_CALLED(CreateInstance);
        CHECK_CALLED(SetInterfaceSafetyOptions);
        CHECK_CALLED(SetScriptSite);
        CHECK_CALLED(QI_IActiveScriptParse);
        CHECK_CALLED(InitNew);

        hr = IScriptControl_get_Modules(sc, &mods);
        ok(hr == S_OK, "IScriptControl_get_Modules failed: 0x%08lx.\n", hr);

        SET_EXPECT(AddNamedItem);
        str = SysAllocString(L"modname");
        AddNamedItem_expected_name = str;
        AddNamedItem_expected_flags = 0;
        V_VT(&var) = VT_DISPATCH;
        V_DISPATCH(&var) = &testdisp;
        hr = IScriptModuleCollection_Add(mods, str, &var, &mod);
        ok(hr == S_OK, "IScriptModuleCollection_Add failed: 0x%08lx.\n", hr);
        VariantClear(&var);
        CHECK_CALLED(AddNamedItem);

        unknown = (IUnknown*)0xdeadbeef;
        hr = IActiveScriptSite_GetItemInfo(site, str, SCRIPTINFO_IUNKNOWN, &unknown, NULL);
        ok(hr == S_OK, "IActiveScriptSite_GetItemInfo failed: 0x%08lx.\n", hr);
        ok(unknown == (IUnknown*)&testdisp, "Unexpected IUnknown for the item: %p.\n", unknown);
        IUnknown_Release(unknown);

        GetScriptDispatch_expected_name = str;
        SET_EXPECT(SetScriptState_STARTED);
        SET_EXPECT(GetScriptDispatch);
        hr = IScriptModule_get_CodeObject(mod, &disp);
        ok(hr == S_OK, "IScriptModule_get_CodeObject failed: 0x%08lx.\n", hr);
        ok(disp == (IDispatch*)&DispatchEx, "Unexpected code object %p.\n", disp);
        CHECK_CALLED(GetScriptDispatch);
        CHECK_CALLED(SetScriptState_STARTED);
        GetScriptDispatch_expected_name = NULL;
        IDispatch_Release(disp);

        SET_EXPECT(ParseScriptText);
        parse_item_name = str;
        parse_flags = SCRIPTTEXT_ISVISIBLE;
        code_str = SysAllocString(L"some code");
        hr = IScriptModule_AddCode(mod, code_str);
        ok(hr == S_OK, "IScriptControl_AddCode failed: 0x%08lx.\n", hr);
        CHECK_CALLED(ParseScriptText);
        SysFreeString(code_str);
        SysFreeString(str);

        /* Keep the module ref before changing the language */
        SET_EXPECT(Close);
        hr = IScriptControl_put_Language(sc, NULL);
        ok(hr == S_OK, "IScriptControl_put_Language failed: 0x%08lx.\n", hr);
        CHECK_CALLED(Close);
        IScriptModuleCollection_Release(mods);
        IActiveScriptSite_Release(site);
        IScriptControl_Release(sc);
        IScriptModule_Release(mod);

        /* Now try holding a module ref while closing the script */
        hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
                              &IID_IScriptControl, (void**)&sc);
        ok(hr == S_OK, "Failed to create IScriptControl interface: 0x%08lx.\n", hr);

        SET_EXPECT(CreateInstance);
        SET_EXPECT(SetInterfaceSafetyOptions);
        SET_EXPECT(SetScriptSite);
        SET_EXPECT(QI_IActiveScriptParse);
        SET_EXPECT(InitNew);
        str = SysAllocString(L"testscript");
        hr = IScriptControl_put_Language(sc, str);
        ok(hr == S_OK, "IScriptControl_put_Language failed: 0x%08lx.\n", hr);
        SysFreeString(str);
        CHECK_CALLED(CreateInstance);
        CHECK_CALLED(SetInterfaceSafetyOptions);
        CHECK_CALLED(SetScriptSite);
        CHECK_CALLED(QI_IActiveScriptParse);
        CHECK_CALLED(InitNew);

        hr = IScriptControl_get_Modules(sc, &mods);
        ok(hr == S_OK, "IScriptControl_get_Modules failed: 0x%08lx.\n", hr);

        SET_EXPECT(AddNamedItem);
        str = SysAllocString(L"foo");
        AddNamedItem_expected_name = str;
        AddNamedItem_expected_flags = SCRIPTITEM_CODEONLY;
        V_VT(&var) = VT_DISPATCH;
        V_DISPATCH(&var) = NULL;
        hr = IScriptModuleCollection_Add(mods, str, &var, &mod);
        ok(hr == S_OK, "IScriptModuleCollection_Add failed: 0x%08lx.\n", hr);
        VariantClear(&var);
        CHECK_CALLED(AddNamedItem);

        unknown = (IUnknown*)0xdeadbeef;
        hr = IActiveScriptSite_GetItemInfo(site, str, SCRIPTINFO_IUNKNOWN, &unknown, NULL);
        ok(hr == TYPE_E_ELEMENTNOTFOUND, "IActiveScriptSite_GetItemInfo returned: 0x%08lx.\n", hr);
        IScriptModuleCollection_Release(mods);
        IActiveScriptSite_Release(site);
        IScriptControl_Release(sc);

        SET_EXPECT(SetScriptState_STARTED);
        SET_EXPECT(ParseScriptText);
        parse_item_name = str;
        parse_flags = SCRIPTTEXT_ISVISIBLE;
        code_str = SysAllocString(L"code after close");
        hr = IScriptModule_AddCode(mod, code_str);
        ok(hr == S_OK, "IScriptControl_AddCode failed: 0x%08lx.\n", hr);
        CHECK_CALLED(SetScriptState_STARTED);
        CHECK_CALLED(ParseScriptText);
        SysFreeString(code_str);
        SysFreeString(str);

        SET_EXPECT(Close);
        IScriptModule_Release(mod);
        CHECK_CALLED(Close);

        /* Hold an enumerator while releasing the script control */
        hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
                              &IID_IScriptControl, (void**)&sc);
        ok(hr == S_OK, "Failed to create IScriptControl interface: 0x%08lx.\n", hr);

        SET_EXPECT(CreateInstance);
        SET_EXPECT(SetInterfaceSafetyOptions);
        SET_EXPECT(SetScriptSite);
        SET_EXPECT(QI_IActiveScriptParse);
        SET_EXPECT(InitNew);
        str = SysAllocString(L"testscript");
        hr = IScriptControl_put_Language(sc, str);
        ok(hr == S_OK, "IScriptControl_put_Language failed: 0x%08lx.\n", hr);
        SysFreeString(str);
        CHECK_CALLED(CreateInstance);
        CHECK_CALLED(SetInterfaceSafetyOptions);
        CHECK_CALLED(SetScriptSite);
        CHECK_CALLED(QI_IActiveScriptParse);
        CHECK_CALLED(InitNew);

        hr = IScriptControl_get_Modules(sc, &mods);
        ok(hr == S_OK, "IScriptControl_get_Modules failed: 0x%08lx.\n", hr);
        IScriptControl_Release(sc);

        SET_EXPECT(AddNamedItem);
        str = SysAllocString(L"bar");
        AddNamedItem_expected_name = str;
        AddNamedItem_expected_flags = SCRIPTITEM_CODEONLY;
        V_VT(&var) = VT_DISPATCH;
        V_DISPATCH(&var) = NULL;
        hr = IScriptModuleCollection_Add(mods, str, &var, &mod);
        ok(hr == S_OK, "IScriptModuleCollection_Add failed: 0x%08lx.\n", hr);
        IScriptModule_Release(mod);
        VariantClear(&var);
        SysFreeString(str);
        CHECK_CALLED(AddNamedItem);

        hr = IScriptModuleCollection_get__NewEnum(mods, &unknown);
        ok(hr == S_OK, "IScriptModuleCollection_get__NewEnum failed: 0x%08lx.\n", hr);
        hr = IUnknown_QueryInterface(unknown, &IID_IEnumVARIANT, (void**)&enumvar);
        ok(hr == S_OK, "Failed to query for IEnumVARIANT: 0x%08lx.\n", hr);
        IUnknown_Release(unknown);

        IScriptModuleCollection_Release(mods);
        IActiveScriptSite_Release(site);

        hr = IEnumVARIANT_Next(enumvar, ARRAY_SIZE(vars), vars, &fetched);
        ok(hr == S_FALSE, "IEnumVARIANT_Next failed: 0x%08lx.\n", hr);
        ok(fetched == 2, "got %lu.\n", fetched);
        for (i = 0; i < fetched; i++)
        {
            ok(V_VT(&vars[i]) == VT_DISPATCH, "V_VT(vars[%u]) = %d.\n", i, V_VT(&vars[i]));
            hr = IDispatch_QueryInterface(V_DISPATCH(&vars[i]), &IID_IScriptModule, (void**)&mod);
            ok(hr == S_OK, "Failed to query IScriptModule from vars[%u]: 0x%08lx.\n", i, hr);
            hr = IScriptModule_get_Name(mod, &str);
            ok(hr == S_OK, "IScriptModule_get_Name failed for vars[%u]: 0x%08lx.\n", i, hr);
            ok(!lstrcmpW(str, i ? L"bar" : L"Global"), "wrong name for vars[%u]: %s.\n", i, wine_dbgstr_w(str));
            IScriptModule_Release(mod);
            VariantClear(&vars[i]);
            SysFreeString(str);
        }

        SET_EXPECT(Close);
        IEnumVARIANT_Release(enumvar);
        CHECK_CALLED(Close);
    }
}

static void test_IScriptControl_get_Error(void)
{
    IScriptError *error, *error2;
    IScriptControl *sc;
    HRESULT hr;
    BSTR str;
    LONG x;

    hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
                          &IID_IScriptControl, (void**)&sc);
    ok(hr == S_OK, "Failed to create IScriptControl interface: 0x%08lx.\n", hr);

    hr = IScriptControl_get_Error(sc, NULL);
    ok(hr == E_POINTER, "IScriptControl_get_Error returned: 0x%08lx.\n", hr);
    hr = IScriptControl_get_Error(sc, &error);
    ok(hr == S_OK, "IScriptControl_get_Error failed: 0x%08lx.\n", hr);

    x = 0xdeadbeef;
    hr = IScriptError_get_Number(error, &x);
    ok(hr == S_OK, "IScriptError_get_Number failed: 0x%08lx.\n", hr);
    ok(x == 0, "Error Number is not 0, got %ld.\n", x);
    hr = IScriptError_get_Source(error, &str);
    ok(hr == S_OK, "IScriptError_get_Source failed: 0x%08lx.\n", hr);
    ok(str == NULL, "Error Source is not (null), got %s.\n", wine_dbgstr_w(str));
    hr = IScriptError_get_Description(error, &str);
    ok(hr == S_OK, "IScriptError_get_Description failed: 0x%08lx.\n", hr);
    ok(str == NULL, "Error Description is not (null), got %s.\n", wine_dbgstr_w(str));
    hr = IScriptError_get_HelpFile(error, &str);
    ok(hr == S_OK, "IScriptError_get_HelpFile failed: 0x%08lx.\n", hr);
    ok(str == NULL, "Error HelpFile is not (null), got %s.\n", wine_dbgstr_w(str));
    hr = IScriptError_get_HelpContext(error, &x);
    ok(hr == S_OK, "IScriptError_get_HelpContext failed: 0x%08lx.\n", hr);
    ok(x == 0, "Error HelpContext is not 0, got %ld.\n", x);
    hr = IScriptError_get_Text(error, &str);
    ok(hr == S_OK, "IScriptError_get_Text failed: 0x%08lx.\n", hr);
    ok(str == NULL, "Error Text is not (null), got %s.\n", wine_dbgstr_w(str));
    hr = IScriptError_get_Line(error, &x);
    ok(hr == S_OK, "IScriptError_get_Line failed: 0x%08lx.\n", hr);
    ok(x == 0, "Error Line is not 0, got %ld.\n", x);
    hr = IScriptError_get_Column(error, &x);
    ok(hr == S_OK, "IScriptError_get_Column failed: 0x%08lx.\n", hr);
    ok(x == 0, "Error Column is not 0, got %ld.\n", x);

    str = SysAllocString(L"jscript");
    hr = IScriptControl_put_Language(sc, str);
    ok(hr == S_OK, "IScriptControl_put_Language failed: 0x%08lx.\n", hr);
    SysFreeString(str);

    str = SysAllocString(L""
        "var valid_var = 1;\n"
        "// comment\n"
        "this is an invalid line\n");
    hr = IScriptControl_AddCode(sc, str);
    ok(FAILED(hr), "IScriptControl_AddCode succeeded: 0x%08lx.\n", hr);
    SysFreeString(str);

    x = 0xdeadbeef;
    hr = IScriptError_get_Number(error, &x);
    ok(hr == S_OK, "IScriptError_get_Number failed: 0x%08lx.\n", hr);
    todo_wine ok(x > 1000, "Error Number is wrong, got %ld.\n", x);
    hr = IScriptError_get_Source(error, &str);
    ok(hr == S_OK, "IScriptError_get_Source failed: 0x%08lx.\n", hr);
    ok(str != NULL, "Error Source is (null).\n");
    SysFreeString(str);
    hr = IScriptError_get_Description(error, &str);
    ok(hr == S_OK, "IScriptError_get_Description failed: 0x%08lx.\n", hr);
    ok(str != NULL, "Error Description is (null).\n");
    SysFreeString(str);
    hr = IScriptError_get_Text(error, &str);
    ok(hr == S_OK, "IScriptError_get_Text failed: 0x%08lx.\n", hr);
    ok(!lstrcmpW(str, L"this is an invalid line"), "Error Text is wrong, got %s.\n", wine_dbgstr_w(str));
    SysFreeString(str);
    hr = IScriptError_get_Line(error, &x);
    ok(hr == S_OK, "IScriptError_get_Line failed: 0x%08lx.\n", hr);
    ok(x == 3, "Error Line is not 3, got %ld.\n", x);
    hr = IScriptError_get_Column(error, &x);
    ok(hr == S_OK, "IScriptError_get_Column failed: 0x%08lx.\n", hr);
    ok(x == 5, "Error Column is not 5, got %ld.\n", x);

    hr = IScriptError_Clear(error);
    ok(hr == S_OK, "IScriptError_Clear failed: 0x%08lx.\n", hr);
    x = 0xdeadbeef;
    hr = IScriptError_get_Number(error, &x);
    ok(hr == S_OK, "IScriptError_get_Number failed: 0x%08lx.\n", hr);
    ok(x == 0, "Error Number is not 0, got %ld.\n", x);
    hr = IScriptError_get_Source(error, &str);
    ok(hr == S_OK, "IScriptError_get_Source failed: 0x%08lx.\n", hr);
    ok(str == NULL, "Error Source is not (null), got %s.\n", wine_dbgstr_w(str));
    hr = IScriptError_get_Description(error, &str);
    ok(hr == S_OK, "IScriptError_get_Description failed: 0x%08lx.\n", hr);
    ok(str == NULL, "Error Description is not (null), got %s.\n", wine_dbgstr_w(str));
    hr = IScriptError_get_HelpFile(error, &str);
    ok(hr == S_OK, "IScriptError_get_HelpFile failed: 0x%08lx.\n", hr);
    ok(str == NULL, "Error HelpFile is not (null), got %s.\n", wine_dbgstr_w(str));
    hr = IScriptError_get_HelpContext(error, &x);
    ok(hr == S_OK, "IScriptError_get_HelpContext failed: 0x%08lx.\n", hr);
    ok(x == 0, "Error HelpContext is not 0, got %ld.\n", x);
    hr = IScriptError_get_Text(error, &str);
    ok(hr == S_OK, "IScriptError_get_Text failed: 0x%08lx.\n", hr);
    ok(str == NULL, "Error Text is not (null), got %s.\n", wine_dbgstr_w(str));
    hr = IScriptError_get_Line(error, &x);
    ok(hr == S_OK, "IScriptError_get_Line failed: 0x%08lx.\n", hr);
    ok(x == 0, "Error Line is not 0, got %ld.\n", x);
    hr = IScriptError_get_Column(error, &x);
    ok(hr == S_OK, "IScriptError_get_Column failed: 0x%08lx.\n", hr);
    ok(x == 0, "Error Column is not 0, got %ld.\n", x);

    hr = IScriptControl_get_Error(sc, &error2);
    ok(hr == S_OK, "IScriptControl_get_Error failed: 0x%08lx.\n", hr);
    ok(error == error2, "Error objects are not the same (%p vs %p).\n", error, error2);
    IScriptError_Release(error2);

    IScriptError_Release(error);
    IScriptControl_Release(sc);

    /* custom script engine */
    if (have_custom_engine)
    {
        hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
                              &IID_IScriptControl, (void**)&sc);
        ok(hr == S_OK, "Failed to create IScriptControl interface: 0x%08lx.\n", hr);

        SET_EXPECT(CreateInstance);
        SET_EXPECT(SetInterfaceSafetyOptions);
        SET_EXPECT(SetScriptSite);
        SET_EXPECT(QI_IActiveScriptParse);
        SET_EXPECT(InitNew);
        str = SysAllocString(L"testscript");
        hr = IScriptControl_put_Language(sc, str);
        ok(hr == S_OK, "IScriptControl_put_Language failed: 0x%08lx.\n", hr);
        SysFreeString(str);
        CHECK_CALLED(CreateInstance);
        CHECK_CALLED(SetInterfaceSafetyOptions);
        CHECK_CALLED(SetScriptSite);
        CHECK_CALLED(QI_IActiveScriptParse);
        CHECK_CALLED(InitNew);

        hr = IScriptControl_get_Error(sc, &error);
        ok(hr == S_OK, "IScriptControl_get_Error failed: 0x%08lx.\n", hr);

        x = 0xdeadbeef;
        hr = IScriptError_get_Number(error, &x);
        ok(hr == S_OK, "IScriptError_get_Number failed: 0x%08lx.\n", hr);
        ok(x == 0, "Error Number is not 0, got %ld.\n", x);

        /* The calls are cached even if they fail */
        ActiveScriptError_stage = 0;
        hr = IActiveScriptSite_OnScriptError(site, &ActiveScriptError);
        ok(hr == S_FALSE, "IActiveScriptSite_OnScriptError returned: 0x%08lx.\n", hr);

        SET_EXPECT(GetExceptionInfo);
        x = 0xdeadbeef;
        hr = IScriptError_get_Number(error, &x);
        ok(hr == S_OK, "IScriptError_get_Number failed: 0x%08lx.\n", hr);
        ok(x == 0, "Error Number is not 0, got %ld.\n", x);
        CHECK_CALLED(GetExceptionInfo);
        hr = IScriptError_get_Source(error, &str);
        ok(hr == S_OK, "IScriptError_get_Source failed: 0x%08lx.\n", hr);
        ok(str == NULL, "Error Source is not (null), got %s.\n", wine_dbgstr_w(str));
        hr = IScriptError_get_Description(error, &str);
        ok(hr == S_OK, "IScriptError_get_Description failed: 0x%08lx.\n", hr);
        ok(str == NULL, "Error Description is not (null), got %s.\n", wine_dbgstr_w(str));
        hr = IScriptError_get_HelpFile(error, &str);
        ok(hr == S_OK, "IScriptError_get_HelpFile failed: 0x%08lx.\n", hr);
        ok(str == NULL, "Error HelpFile is not (null), got %s.\n", wine_dbgstr_w(str));
        hr = IScriptError_get_HelpContext(error, &x);
        ok(hr == S_OK, "IScriptError_get_HelpContext failed: 0x%08lx.\n", hr);
        ok(x == 0, "Error HelpContext is not 0, got %ld.\n", x);

        SET_EXPECT(GetSourceLineText);
        hr = IScriptError_get_Text(error, &str);
        ok(hr == S_OK, "IScriptError_get_Text failed: 0x%08lx.\n", hr);
        ok(!lstrcmpW(str, L"source line"), "Error Text is wrong, got %s.\n", wine_dbgstr_w(str));
        SysFreeString(str);
        CHECK_CALLED(GetSourceLineText);

        SET_EXPECT(GetSourcePosition);
        hr = IScriptError_get_Line(error, &x);
        ok(hr == S_OK, "IScriptError_get_Line failed: 0x%08lx.\n", hr);
        ok(x == 0, "Error Line is not 0, got %ld.\n", x);
        CHECK_CALLED(GetSourcePosition);
        hr = IScriptError_get_Column(error, &x);
        ok(hr == S_OK, "IScriptError_get_Column failed: 0x%08lx.\n", hr);
        ok(x == 0, "Error Column is not 0, got %ld.\n", x);

        /* Check with deferred fill-in */
        ActiveScriptError_stage = 1;
        hr = IActiveScriptSite_OnScriptError(site, &ActiveScriptError);
        ok(hr == S_FALSE, "IActiveScriptSite_OnScriptError returned: 0x%08lx.\n", hr);

        SET_EXPECT(GetExceptionInfo);
        SET_EXPECT(DeferredFillIn);
        x = 0xdeadbeef;
        hr = IScriptError_get_Number(error, &x);
        ok(hr == S_OK, "IScriptError_get_Number failed: 0x%08lx.\n", hr);
        ok(x == 0, "Error Number is not 0, got %ld.\n", x);
        CHECK_CALLED(GetExceptionInfo);
        CHECK_CALLED(DeferredFillIn);
        hr = IScriptError_get_Source(error, &str);
        ok(hr == S_OK, "IScriptError_get_Source failed: 0x%08lx.\n", hr);
        ok(!lstrcmpW(str, L"foobar"), "Error Source is wrong, got %s.\n", wine_dbgstr_w(str));
        SysFreeString(str);
        hr = IScriptError_get_Description(error, &str);
        ok(hr == S_OK, "IScriptError_get_Description failed: 0x%08lx.\n", hr);
        ok(!lstrcmpW(str, L"barfoo"), "Error Description is wrong, got %s.\n", wine_dbgstr_w(str));
        SysFreeString(str);

        SET_EXPECT(GetSourceLineText);
        hr = IScriptError_get_Text(error, &str);
        ok(hr == S_OK, "IScriptError_get_Text failed: 0x%08lx.\n", hr);
        ok(!lstrcmpW(str, L"source line"), "Error Text is wrong, got %s.\n", wine_dbgstr_w(str));
        SysFreeString(str);
        CHECK_CALLED(GetSourceLineText);

        SET_EXPECT(GetSourcePosition);
        hr = IScriptError_get_Line(error, &x);
        ok(hr == S_OK, "IScriptError_get_Line failed: 0x%08lx.\n", hr);
        ok(x == 42, "Error Line is not 42, got %ld.\n", x);
        CHECK_CALLED(GetSourcePosition);
        hr = IScriptError_get_Column(error, &x);
        ok(hr == S_OK, "IScriptError_get_Column failed: 0x%08lx.\n", hr);
        ok(x == 10, "Error Column is not 10, got %ld.\n", x);

        /* Check without deferred fill-in, but using scode */
        ActiveScriptError_stage = 2;
        hr = IActiveScriptSite_OnScriptError(site, &ActiveScriptError);
        ok(hr == S_FALSE, "IActiveScriptSite_OnScriptError returned: 0x%08lx.\n", hr);

        SET_EXPECT(GetExceptionInfo);
        x = 0xdeadbeef;
        hr = IScriptError_get_Number(error, &x);
        ok(hr == S_OK, "IScriptError_get_Number failed: 0x%08lx.\n", hr);
        ok(x == 0xbeef, "Error Number is not 0xbeef, got 0x%04lx.\n", x);
        CHECK_CALLED(GetExceptionInfo);
        hr = IScriptError_get_Source(error, &str);
        ok(hr == S_OK, "IScriptError_get_Source failed: 0x%08lx.\n", hr);
        ok(!lstrcmpW(str, L"source"), "Error Source is wrong, got %s.\n", wine_dbgstr_w(str));
        SysFreeString(str);
        hr = IScriptError_get_Description(error, &str);
        ok(hr == S_OK, "IScriptError_get_Description failed: 0x%08lx.\n", hr);
        ok(str == NULL, "Error Description is not (null), got %s.\n", wine_dbgstr_w(str));

        SET_EXPECT(GetSourceLineText);
        hr = IScriptError_get_Text(error, &str);
        ok(hr == S_OK, "IScriptError_get_Text failed: 0x%08lx.\n", hr);
        ok(!lstrcmpW(str, L"source line"), "Error Text is wrong, got %s.\n", wine_dbgstr_w(str));
        SysFreeString(str);
        CHECK_CALLED(GetSourceLineText);

        SET_EXPECT(GetSourcePosition);
        hr = IScriptError_get_Line(error, &x);
        ok(hr == S_OK, "IScriptError_get_Line failed: 0x%08lx.\n", hr);
        ok(x == 42, "Error Line is not 42, got %ld.\n", x);
        CHECK_CALLED(GetSourcePosition);
        hr = IScriptError_get_Column(error, &x);
        ok(hr == S_OK, "IScriptError_get_Column failed: 0x%08lx.\n", hr);
        ok(x == 10, "Error Column is not 10, got %ld.\n", x);

        IActiveScriptSite_Release(site);

        SET_EXPECT(Close);
        IScriptControl_Release(sc);
        CHECK_CALLED(Close);

        IScriptError_Release(error);
    }
}

static void test_IScriptControl_get_CodeObject(void)
{
    IScriptControl *sc;
    IDispatch *disp;
    HRESULT hr;
    BSTR str;

    hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
                          &IID_IScriptControl, (void**)&sc);
    ok(hr == S_OK, "Failed to create IScriptControl interface: 0x%08lx.\n", hr);

    hr = IScriptControl_get_CodeObject(sc, &disp);
    ok(hr == E_FAIL, "IScriptControl_get_CodeObject returned: 0x%08lx.\n", hr);

    str = SysAllocString(L"jscript");
    hr = IScriptControl_put_Language(sc, str);
    ok(hr == S_OK, "IScriptControl_put_Language failed: 0x%08lx.\n", hr);
    SysFreeString(str);

    hr = IScriptControl_get_CodeObject(sc, &disp);
    ok(hr == S_OK, "IScriptControl_get_CodeObject failed: 0x%08lx.\n", hr);

    IDispatch_Release(disp);
    IScriptControl_Release(sc);

    if (have_custom_engine)
    {
        hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
                              &IID_IScriptControl, (void**)&sc);
        ok(hr == S_OK, "Failed to create IScriptControl interface: 0x%08lx.\n", hr);

        SET_EXPECT(CreateInstance);
        SET_EXPECT(SetInterfaceSafetyOptions);
        SET_EXPECT(SetScriptSite);
        SET_EXPECT(QI_IActiveScriptParse);
        SET_EXPECT(InitNew);

        str = SysAllocString(L"testscript");
        hr = IScriptControl_put_Language(sc, str);
        ok(hr == S_OK, "IScriptControl_put_Language failed: 0x%08lx.\n", hr);
        SysFreeString(str);

        CHECK_CALLED(CreateInstance);
        CHECK_CALLED(SetInterfaceSafetyOptions);
        CHECK_CALLED(SetScriptSite);
        CHECK_CALLED(QI_IActiveScriptParse);
        CHECK_CALLED(InitNew);

        GetScriptDispatch_expected_name = NULL;
        SET_EXPECT(SetScriptState_STARTED);
        SET_EXPECT(GetScriptDispatch);
        hr = IScriptControl_get_CodeObject(sc, &disp);
        ok(hr == S_OK, "IScriptControl_get_CodeObject failed: 0x%08lx.\n", hr);
        ok(disp == (IDispatch*)&DispatchEx, "unexpected code object %p\n", disp);
        CHECK_CALLED(GetScriptDispatch);
        CHECK_CALLED(SetScriptState_STARTED);

        IDispatch_Release(disp);
        IActiveScriptSite_Release(site);

        SET_EXPECT(Close);
        IScriptControl_Release(sc);
        CHECK_CALLED(Close);
    }
}

static void test_IScriptControl_get_Procedures(void)
{
    IScriptProcedureCollection *procs, *procs2;
    IEnumVARIANT *enumvar, *enumvar2;
    IScriptProcedure *proc, *proc2;
    IScriptControl *sc;
    IUnknown *unknown;
    VARIANT_BOOL vbool;
    ULONG fetched;
    VARIANT var;
    LONG count;
    HRESULT hr;
    BSTR str;
    UINT i;

    hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
                          &IID_IScriptControl, (void**)&sc);
    ok(hr == S_OK, "Failed to create IScriptControl interface: 0x%08lx.\n", hr);

    hr = IScriptControl_get_Procedures(sc, &procs);
    ok(hr == E_FAIL, "IScriptControl_get_Procedures returned: 0x%08lx.\n", hr);

    str = SysAllocString(L"jscript");
    hr = IScriptControl_put_Language(sc, str);
    ok(hr == S_OK, "IScriptControl_put_Language failed: 0x%08lx.\n", hr);
    SysFreeString(str);

    hr = IScriptControl_get_Procedures(sc, &procs);
    ok(hr == S_OK, "IScriptControl_get_Procedures failed: 0x%08lx.\n", hr);

    hr = IScriptProcedureCollection_get_Count(procs, NULL);
    ok(hr == E_POINTER, "IScriptProcedureCollection_get_Count returned: 0x%08lx.\n", hr);
    hr = IScriptProcedureCollection_get_Count(procs, &count);
    ok(hr == S_OK, "IScriptProcedureCollection_get_Count failed: 0x%08lx.\n", hr);
    ok(count == 0, "count is not 0, got %ld.\n", count);

    V_VT(&var) = VT_I4;
    V_I4(&var) = -1;
    hr = IScriptProcedureCollection_get_Item(procs, var, NULL);
    ok(hr == E_POINTER, "IScriptProcedureCollection_get_Item returned: 0x%08lx.\n", hr);
    hr = IScriptProcedureCollection_get_Item(procs, var, &proc);
    ok(hr == 0x800a0009, "IScriptProcedureCollection_get_Item returned: 0x%08lx.\n", hr);

    str = SysAllocString(L""
        "function add(a, b) { return a + b; }\n"
        "function nop(a) { }\n"
        "function muladd(a, b, c) { return a * b + c; }\n"
    );
    hr = IScriptControl_AddCode(sc, str);
    ok(hr == S_OK, "IScriptControl_AddCode failed: 0x%08lx.\n", hr);
    CHECK_ERROR(sc, 0);
    SysFreeString(str);

    hr = IScriptProcedureCollection_get_Count(procs, &count);
    ok(hr == S_OK, "IScriptProcedureCollection_get_Count failed: 0x%08lx.\n", hr);
    ok(count == 3, "count is not 3, got %ld.\n", count);

    V_VT(&var) = VT_I4;
    V_I4(&var) = 1;
    IScriptProcedureCollection_AddRef(procs);
    i = IScriptProcedureCollection_Release(procs);
    hr = IScriptProcedureCollection_get_Item(procs, var, &proc);
    ok(hr == S_OK, "IScriptProcedureCollection_get_Item failed: 0x%08lx.\n", hr);
    IScriptProcedureCollection_AddRef(procs);
    ok(i == IScriptProcedureCollection_Release(procs),
        "IScriptProcedureCollection_get_Item should not have added a ref to the collection.\n");
    hr = IScriptProcedure_get_Name(proc, NULL);
    ok(hr == E_POINTER, "IScriptProcedure_get_Name returned: 0x%08lx.\n", hr);
    hr = IScriptProcedure_get_Name(proc, &str);
    ok(hr == S_OK, "IScriptProcedure_get_Name failed: 0x%08lx.\n", hr);
    ok(!lstrcmpW(str, L"add"), "Wrong name, got %s.\n", wine_dbgstr_w(str));
    SysFreeString(str);
    hr = IScriptProcedure_get_NumArgs(proc, NULL);
    ok(hr == E_POINTER, "IScriptProcedure_get_NumArgs returned: 0x%08lx.\n", hr);
    hr = IScriptProcedure_get_NumArgs(proc, &count);
    ok(hr == S_OK, "IScriptProcedure_get_NumArgs failed: 0x%08lx.\n", hr);
    ok(count == 2, "Wrong NumArgs, got %ld.\n", count);
    hr = IScriptProcedure_get_HasReturnValue(proc, NULL);
    ok(hr == E_POINTER, "IScriptProcedure_get_HasReturnValue returned: 0x%08lx.\n", hr);
    hr = IScriptProcedure_get_HasReturnValue(proc, &vbool);
    ok(hr == S_OK, "IScriptProcedure_get_HasReturnValue failed: 0x%08lx.\n", hr);
    ok(vbool == VARIANT_TRUE, "HasReturnValue did not return True, got %x.\n", vbool);
    IScriptProcedure_Release(proc);

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(L"Nop");
    hr = IScriptProcedureCollection_get_Item(procs, var, &proc);
    ok(hr == S_OK, "IScriptProcedureCollection_get_Item failed: 0x%08lx.\n", hr);
    ok(V_VT(&var) == VT_BSTR, "var type not BSTR, got %d.\n", V_VT(&var));
    VariantClear(&var);
    hr = IScriptProcedure_get_Name(proc, &str);
    ok(hr == S_OK, "IScriptProcedure_get_Name failed: 0x%08lx.\n", hr);
    ok(!lstrcmpW(str, L"nop"), "Wrong name, got %s.\n", wine_dbgstr_w(str));
    SysFreeString(str);
    hr = IScriptProcedure_get_NumArgs(proc, &count);
    ok(hr == S_OK, "IScriptProcedure_get_NumArgs failed: 0x%08lx.\n", hr);
    ok(count == 1, "Wrong NumArgs, got %ld.\n", count);
    hr = IScriptProcedure_get_HasReturnValue(proc, &vbool);
    ok(hr == S_OK, "IScriptProcedure_get_HasReturnValue failed: 0x%08lx.\n", hr);
    ok(vbool == VARIANT_TRUE, "HasReturnValue did not return True, got %x.\n", vbool);
    IScriptProcedure_Release(proc);

    V_VT(&var) = VT_R8;
    V_R8(&var) = 3.0;
    hr = IScriptProcedureCollection_get_Item(procs, var, &proc);
    ok(hr == S_OK, "IScriptProcedureCollection_get_Item failed: 0x%08lx.\n", hr);
    hr = IScriptProcedure_get_Name(proc, &str);
    ok(hr == S_OK, "IScriptProcedure_get_Name failed: 0x%08lx.\n", hr);
    ok(!lstrcmpW(str, L"muladd"), "Wrong name, got %s.\n", wine_dbgstr_w(str));
    SysFreeString(str);
    hr = IScriptProcedure_get_NumArgs(proc, &count);
    ok(hr == S_OK, "IScriptProcedure_get_NumArgs failed: 0x%08lx.\n", hr);
    ok(count == 3, "Wrong NumArgs, got %ld.\n", count);
    IScriptProcedure_Release(proc);

    IScriptProcedureCollection_Release(procs);
    IScriptControl_Release(sc);

    if (have_custom_engine)
    {
        hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
                              &IID_IScriptControl, (void**)&sc);
        ok(hr == S_OK, "Failed to create IScriptControl interface: 0x%08lx.\n", hr);

        SET_EXPECT(CreateInstance);
        SET_EXPECT(SetInterfaceSafetyOptions);
        SET_EXPECT(SetScriptSite);
        SET_EXPECT(QI_IActiveScriptParse);
        SET_EXPECT(InitNew);

        str = SysAllocString(L"testscript");
        hr = IScriptControl_put_Language(sc, str);
        ok(hr == S_OK, "IScriptControl_put_Language failed: 0x%08lx.\n", hr);
        SysFreeString(str);

        CHECK_CALLED(CreateInstance);
        CHECK_CALLED(SetInterfaceSafetyOptions);
        CHECK_CALLED(SetScriptSite);
        CHECK_CALLED(QI_IActiveScriptParse);
        CHECK_CALLED(InitNew);

        hr = IScriptControl_get_Procedures(sc, &procs);
        ok(hr == S_OK, "IScriptControl_get_Procedures failed: 0x%08lx.\n", hr);
        hr = IScriptControl_get_Procedures(sc, &procs2);
        ok(hr == S_OK, "IScriptControl_get_Procedures failed: 0x%08lx.\n", hr);
        ok(procs == procs2, "Procedure collections are not the same (%p vs %p).\n", procs, procs2);
        IScriptProcedureCollection_Release(procs2);

        GetScriptDispatch_expected_name = NULL;
        SET_EXPECT(SetScriptState_STARTED);
        SET_EXPECT(GetScriptDispatch);
        SET_EXPECT(GetTypeInfo);
        SET_EXPECT(GetTypeAttr);
        SET_EXPECT(ReleaseTypeAttr);
        TypeInfo_GetTypeAttr_cFuncs = 1337;
        hr = IScriptProcedureCollection_get_Count(procs, &count);
        ok(hr == S_OK, "IScriptProcedureCollection_get_Count failed: 0x%08lx.\n", hr);
        ok(count == 1337, "count is not 1337, got %ld.\n", count);
        CHECK_CALLED(SetScriptState_STARTED);
        CHECK_CALLED(GetScriptDispatch);
        CHECK_CALLED(GetTypeInfo);
        CHECK_CALLED(GetTypeAttr);
        CHECK_CALLED(ReleaseTypeAttr);
        TypeInfo_GetTypeAttr_cFuncs = ARRAY_SIZE(custom_engine_funcs);
        count = 0;
        hr = IScriptProcedureCollection_get_Count(procs, &count);
        ok(hr == S_OK, "IScriptProcedureCollection_get_Count failed: 0x%08lx.\n", hr);
        ok(count == 1337, "count is not 1337, got %ld.\n", count);

        /* Reload the collection to update the cached function count */
        IScriptProcedureCollection_Release(procs);
        hr = IScriptControl_get_Procedures(sc, &procs);
        ok(hr == S_OK, "IScriptControl_get_Procedures failed: 0x%08lx.\n", hr);
        count = 0;
        SET_EXPECT(GetTypeAttr);
        SET_EXPECT(ReleaseTypeAttr);
        hr = IScriptProcedureCollection_get_Count(procs, &count);
        ok(hr == S_OK, "IScriptProcedureCollection_get_Count failed: 0x%08lx.\n", hr);
        ok(count == ARRAY_SIZE(custom_engine_funcs), "count is not %u, got %ld.\n", TypeInfo_GetTypeAttr_cFuncs, count);
        CHECK_CALLED(GetTypeAttr);
        CHECK_CALLED(ReleaseTypeAttr);

        /* Adding code reloads the typeinfo the next time */
        SET_EXPECT(ParseScriptText);
        parse_item_name = NULL;
        parse_flags = SCRIPTTEXT_ISVISIBLE;
        str = SysAllocString(L" ");
        hr = IScriptControl_AddCode(sc, str);
        ok(hr == S_OK, "IScriptControl_AddCode failed: 0x%08lx.\n", hr);
        SysFreeString(str);
        CHECK_ERROR(sc, 0);
        CHECK_CALLED(ParseScriptText);

        GetScriptDispatch_expected_name = NULL;
        SET_EXPECT(GetScriptDispatch);
        SET_EXPECT(GetTypeInfo);
        SET_EXPECT(GetTypeAttr);
        SET_EXPECT(ReleaseTypeAttr);
        hr = IScriptProcedureCollection_get_Count(procs, &count);
        ok(hr == S_OK, "IScriptProcedureCollection_get_Count failed: 0x%08lx.\n", hr);
        ok(count == ARRAY_SIZE(custom_engine_funcs), "count is not %u, got %ld.\n", TypeInfo_GetTypeAttr_cFuncs, count);
        CHECK_CALLED(GetScriptDispatch);
        CHECK_CALLED(GetTypeInfo);
        CHECK_CALLED(GetTypeAttr);
        CHECK_CALLED(ReleaseTypeAttr);

        /* Reset uncaches the objects, but not the count */
        SET_EXPECT(SetScriptState_INITIALIZED);
        IScriptControl_Reset(sc);
        CHECK_CALLED(SetScriptState_INITIALIZED);
        count = 0;
        hr = IScriptProcedureCollection_get_Count(procs, &count);
        ok(hr == S_OK, "IScriptProcedureCollection_get_Count failed: 0x%08lx.\n", hr);
        ok(count == ARRAY_SIZE(custom_engine_funcs), "count is not %u, got %ld.\n", TypeInfo_GetTypeAttr_cFuncs, count);

        /* Try without ITypeComp interface */
        SET_EXPECT(SetScriptState_STARTED);
        SET_EXPECT(GetScriptDispatch);
        SET_EXPECT(GetTypeInfo);
        SET_EXPECT(QI_ITypeComp);
        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = SysAllocString(L"foobar");
        hr = IScriptProcedureCollection_get_Item(procs, var, &proc);
        ok(hr == E_NOINTERFACE, "IScriptProcedureCollection_get_Item returned: 0x%08lx.\n", hr);
        ok(V_VT(&var) == VT_BSTR, "var type not BSTR, got %d.\n", V_VT(&var));
        VariantClear(&var);
        CHECK_CALLED(SetScriptState_STARTED);
        CHECK_CALLED(GetScriptDispatch);
        CHECK_CALLED(GetTypeInfo);
        CHECK_CALLED(QI_ITypeComp);

        count = 0;
        hr = IScriptProcedureCollection_get_Count(procs, &count);
        ok(hr == S_OK, "IScriptProcedureCollection_get_Count failed: 0x%08lx.\n", hr);
        ok(count == ARRAY_SIZE(custom_engine_funcs), "count is not %u, got %ld.\n", TypeInfo_GetTypeAttr_cFuncs, count);

        /* Make ITypeComp available */
        TypeComp_available = TRUE;
        SET_EXPECT(QI_ITypeComp);
        SET_EXPECT(Bind);
        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = SysAllocString(L"type_mismatch");
        hr = IScriptProcedureCollection_get_Item(procs, var, &proc);
        ok(hr == TYPE_E_TYPEMISMATCH, "IScriptProcedureCollection_get_Item returned: 0x%08lx.\n", hr);
        VariantClear(&var);
        CHECK_CALLED(QI_ITypeComp);
        CHECK_CALLED(Bind);
        TypeComp_available = FALSE;

        SET_EXPECT(Bind);
        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = SysAllocString(L"not_found");
        hr = IScriptProcedureCollection_get_Item(procs, var, &proc);
        ok(hr == CTL_E_ILLEGALFUNCTIONCALL, "IScriptProcedureCollection_get_Item failed: 0x%08lx.\n", hr);
        VariantClear(&var);
        CHECK_CALLED(Bind);

        SET_EXPECT(Bind);
        SET_EXPECT(ReleaseVarDesc);
        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = SysAllocString(L"variable");
        hr = IScriptProcedureCollection_get_Item(procs, var, &proc);
        ok(hr == CTL_E_ILLEGALFUNCTIONCALL, "IScriptProcedureCollection_get_Item failed: 0x%08lx.\n", hr);
        VariantClear(&var);
        CHECK_CALLED(Bind);
        CHECK_CALLED(ReleaseVarDesc);

        /* Index 0 and below are invalid (doesn't even call GetFuncDesc) */
        V_VT(&var) = VT_I4;
        V_I4(&var) = 0;
        hr = IScriptProcedureCollection_get_Item(procs, var, &proc);
        ok(hr == 0x800a0009, "IScriptProcedureCollection_get_Item returned: 0x%08lx.\n", hr);
        V_I4(&var) = -1;
        hr = IScriptProcedureCollection_get_Item(procs, var, &proc);
        ok(hr == 0x800a0009, "IScriptProcedureCollection_get_Item returned: 0x%08lx.\n", hr);
        SET_EXPECT(GetFuncDesc);
        V_I4(&var) = 1337;
        hr = IScriptProcedureCollection_get_Item(procs, var, &proc);
        ok(hr == E_INVALIDARG, "IScriptProcedureCollection_get_Item returned: 0x%08lx.\n", hr);
        CHECK_CALLED(GetFuncDesc);

        /* _NewEnum never caches the function count */
        hr = IScriptProcedureCollection_get__NewEnum(procs, NULL);
        ok(hr == E_POINTER, "IScriptProcedureCollection_get__NewEnum returned: 0x%08lx.\n", hr);
        SET_EXPECT(GetTypeAttr);
        SET_EXPECT(ReleaseTypeAttr);
        hr = IScriptProcedureCollection_get__NewEnum(procs, &unknown);
        ok(hr == S_OK, "IScriptProcedureCollection_get__NewEnum failed: 0x%08lx.\n", hr);
        CHECK_CALLED(GetTypeAttr);
        CHECK_CALLED(ReleaseTypeAttr);
        hr = IUnknown_QueryInterface(unknown, &IID_IEnumVARIANT, (void**)&enumvar);
        ok(hr == S_OK, "Failed to query for IEnumVARIANT: 0x%08lx.\n", hr);
        ok((char*)unknown == (char*)enumvar, "unknown and enumvar are not the same (%p vs %p).\n", unknown, enumvar);
        IEnumVARIANT_Release(enumvar);
        IUnknown_Release(unknown);
        SET_EXPECT(GetTypeAttr);
        SET_EXPECT(ReleaseTypeAttr);
        hr = IScriptProcedureCollection_get__NewEnum(procs, &unknown);
        ok(hr == S_OK, "IScriptProcedureCollection_get__NewEnum failed: 0x%08lx.\n", hr);
        CHECK_CALLED(GetTypeAttr);
        CHECK_CALLED(ReleaseTypeAttr);
        hr = IUnknown_QueryInterface(unknown, &IID_IEnumVARIANT, (void**)&enumvar);
        ok(hr == S_OK, "Failed to query for IEnumVARIANT: 0x%08lx.\n", hr);
        IUnknown_Release(unknown);

        fetched = 0xdeadbeef;
        hr = IEnumVARIANT_Next(enumvar, 0, NULL, NULL);
        ok(hr == E_POINTER, "IEnumVARIANT_Next returned: 0x%08lx.\n", hr);
        hr = IEnumVARIANT_Next(enumvar, 0, NULL, &fetched);
        ok(hr == E_POINTER, "IEnumVARIANT_Next failed: 0x%08lx.\n", hr);
        ok(fetched == 0xdeadbeef, "got %lu.\n", fetched);
        hr = IEnumVARIANT_Next(enumvar, 0, &var, &fetched);
        ok(hr == S_OK, "IEnumVARIANT_Next returned: 0x%08lx.\n", hr);
        ok(fetched == 0, "got %lu.\n", fetched);
        hr = IEnumVARIANT_Next(enumvar, 0, &var, NULL);
        ok(hr == S_OK, "IEnumVARIANT_Next returned: 0x%08lx.\n", hr);
        hr = IEnumVARIANT_Clone(enumvar, NULL);
        ok(hr == E_POINTER, "IEnumVARIANT_Clone failed: 0x%08lx.\n", hr);

        for (i = 0; i < ARRAY_SIZE(custom_engine_funcs); i++)
        {
            /* Querying by index still goes through the Bind process */
            SET_EXPECT(GetFuncDesc);
            SET_EXPECT(GetNames);
            SET_EXPECT(ReleaseFuncDesc);
            V_VT(&var) = VT_R4;
            V_R4(&var) = i + 1;
            hr = IScriptProcedureCollection_get_Item(procs, var, &proc);
            ok(hr == S_OK, "get_Item for index %u failed: 0x%08lx.\n", i, hr);
            CHECK_CALLED(GetFuncDesc);
            CHECK_CALLED(GetNames);
            CHECK_CALLED(ReleaseFuncDesc);

            /* The name is cached and not looked up with Bind anymore */
            V_VT(&var) = VT_BSTR;
            V_BSTR(&var) = SysAllocString(custom_engine_funcs[i].name);
            hr = IScriptProcedureCollection_get_Item(procs, var, &proc2);
            ok(hr == S_OK, "get_Item for %s failed: 0x%08lx.\n", wine_dbgstr_w(custom_engine_funcs[i].name), hr);
            ok(proc == proc2, "proc and proc2 are not the same for %s and index %u.\n",
                wine_dbgstr_w(custom_engine_funcs[i].name), i + 1);
            IScriptProcedure_Release(proc);
            IScriptProcedure_Release(proc2);

            /* Since both were released, the cache entry is destroyed */
            SET_EXPECT(Bind);
            SET_EXPECT(GetNames);
            SET_EXPECT(ReleaseFuncDesc);
            hr = IScriptProcedureCollection_get_Item(procs, var, &proc);
            ok(hr == S_OK, "get_Item for %s failed: 0x%08lx.\n", wine_dbgstr_w(custom_engine_funcs[i].name), hr);
            VariantClear(&var);
            CHECK_CALLED(Bind);
            CHECK_CALLED(GetNames);
            CHECK_CALLED(ReleaseFuncDesc);

            /* Compare with the enumerator */
            SET_EXPECT(GetFuncDesc);
            SET_EXPECT(GetNames);
            SET_EXPECT(ReleaseFuncDesc);
            hr = IEnumVARIANT_Next(enumvar, 1, &var, &fetched);
            ok(hr == S_OK, "IEnumVARIANT_Next for index %u failed: 0x%08lx.\n", i, hr);
            ok(fetched == 1, "got %lu.\n", fetched);
            ok(V_VT(&var) == VT_DISPATCH, "V_VT(var) = %d.\n", V_VT(&var));
            CHECK_CALLED(GetFuncDesc);
            CHECK_CALLED(GetNames);
            CHECK_CALLED(ReleaseFuncDesc);
            hr = IDispatch_QueryInterface(V_DISPATCH(&var), &IID_IScriptProcedure, (void**)&proc2);
            ok(hr == S_OK, "Failed to query IScriptProcedure for index %u: 0x%08lx.\n", i, hr);
            VariantClear(&var);

            ok(proc == proc2, "proc and proc2 are not the same for %s and enum index %u.\n",
                wine_dbgstr_w(custom_engine_funcs[i].name), i);
            IScriptProcedure_Release(proc2);

            /* Verify the properties */
            hr = IScriptProcedure_get_Name(proc, &str);
            ok(hr == S_OK, "get_Name for %s failed: 0x%08lx.\n", wine_dbgstr_w(custom_engine_funcs[i].name), hr);
            ok(!lstrcmpW(str, custom_engine_funcs[i].name), "Name is not %s, got %s.\n",
                wine_dbgstr_w(custom_engine_funcs[i].name), wine_dbgstr_w(str));
            SysFreeString(str);
            hr = IScriptProcedure_get_NumArgs(proc, &count);
            ok(hr == S_OK, "get_NumArgs for %s failed: 0x%08lx.\n", wine_dbgstr_w(custom_engine_funcs[i].name), hr);
            ok(count == custom_engine_funcs[i].num_args + custom_engine_funcs[i].num_opt_args,
                "NumArgs is not %d, got %ld.\n", custom_engine_funcs[i].num_args + custom_engine_funcs[i].num_opt_args, count);
            hr = IScriptProcedure_get_HasReturnValue(proc, &vbool);
            ok(hr == S_OK, "get_HasReturnValue for %s failed: 0x%08lx.\n", wine_dbgstr_w(custom_engine_funcs[i].name), hr);
            ok(vbool == ((custom_engine_funcs[i].ret_type == VT_VOID) ? VARIANT_FALSE : VARIANT_TRUE),
                "get_HasReturnValue for %s returned %x.\n", wine_dbgstr_w(custom_engine_funcs[i].name), vbool);

            IScriptProcedure_Release(proc);
        }

        hr = IEnumVARIANT_Next(enumvar, 1, &var, &fetched);
        ok(hr == S_FALSE, "IEnumVARIANT_Next failed: 0x%08lx.\n", hr);
        ok(fetched == 0, "got %lu.\n", fetched);
        hr = IEnumVARIANT_Skip(enumvar, 0);
        ok(hr == S_OK, "IEnumVARIANT_Skip failed: 0x%08lx.\n", hr);
        hr = IEnumVARIANT_Skip(enumvar, 1);
        ok(hr == S_FALSE, "IEnumVARIANT_Skip failed: 0x%08lx.\n", hr);
        hr = IEnumVARIANT_Reset(enumvar);
        ok(hr == S_OK, "IEnumVARIANT_Reset failed: 0x%08lx.\n", hr);
        hr = IEnumVARIANT_Skip(enumvar, ARRAY_SIZE(custom_engine_funcs) - 1);
        ok(hr == S_OK, "IEnumVARIANT_Skip failed: 0x%08lx.\n", hr);
        hr = IEnumVARIANT_Clone(enumvar, &enumvar2);
        ok(hr == S_OK, "IEnumVARIANT_Clone failed: 0x%08lx.\n", hr);
        hr = IEnumVARIANT_Skip(enumvar2, 1);
        ok(hr == S_OK, "IEnumVARIANT_Skip failed: 0x%08lx.\n", hr);
        hr = IEnumVARIANT_Skip(enumvar2, 1);
        ok(hr == S_FALSE, "IEnumVARIANT_Skip failed: 0x%08lx.\n", hr);

        IEnumVARIANT_Release(enumvar2);
        IEnumVARIANT_Release(enumvar);
        IScriptProcedureCollection_Release(procs);
        IActiveScriptSite_Release(site);

        SET_EXPECT(Close);
        IScriptControl_Release(sc);
        CHECK_CALLED(Close);
    }
}

START_TEST(msscript)
{
    IUnknown *unk;
    HRESULT hr;

    CoInitialize(NULL);

    hr = CoCreateInstance(&CLSID_ScriptControl, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IUnknown, (void**)&unk);
    if (FAILED(hr)) {
        win_skip("Could not create ScriptControl object: %08lx\n", hr);
        return;
    }
    IUnknown_Release(unk);

    have_custom_engine = register_script_engine();
    if (!have_custom_engine)
        skip("Could not register TestScript engine.\n");

    test_oleobject();
    test_persiststreaminit();
    test_olecontrol();
    test_Language();
    test_connectionpoints();
    test_quickactivate();
    test_viewobject();
    test_pointerinactive();
    test_timeout();
    test_Reset();
    test_AddObject();
    test_AllowUI();
    test_SitehWnd();
    test_UseSafeSubset();
    test_State();
    test_IScriptControl_Eval();
    test_IScriptControl_AddCode();
    test_IScriptControl_ExecuteStatement();
    test_IScriptControl_Run();
    test_IScriptControl_get_Modules();
    test_IScriptControl_get_Error();
    test_IScriptControl_get_CodeObject();
    test_IScriptControl_get_Procedures();

    init_registry(FALSE);

    CoUninitialize();
}
