/*
 * Copyright 2011 Jacek Caban for CodeWeavers
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

#define COBJMACROS
#define CONST_VTABLE

#include <ole2.h>
#include <dispex.h>
#include <activscp.h>

#include "vbsregexp55.h"

#include "wine/test.h"

#ifdef _WIN64

#define IActiveScriptParse_QueryInterface IActiveScriptParse64_QueryInterface
#define IActiveScriptParse_Release IActiveScriptParse64_Release
#define IActiveScriptParse_InitNew IActiveScriptParse64_InitNew
#define IActiveScriptParse_ParseScriptText IActiveScriptParse64_ParseScriptText
#define IActiveScriptParseProcedure2_Release IActiveScriptParseProcedure2_64_Release
#define IActiveScriptParseProcedure2_ParseProcedureText IActiveScriptParseProcedure2_64_ParseProcedureText

#else

#define IActiveScriptParse_QueryInterface IActiveScriptParse32_QueryInterface
#define IActiveScriptParse_Release IActiveScriptParse32_Release
#define IActiveScriptParse_InitNew IActiveScriptParse32_InitNew
#define IActiveScriptParse_ParseScriptText IActiveScriptParse32_ParseScriptText
#define IActiveScriptParseProcedure2_Release IActiveScriptParseProcedure2_32_Release
#define IActiveScriptParseProcedure2_ParseProcedureText IActiveScriptParseProcedure2_32_ParseProcedureText

#endif

extern const CLSID CLSID_VBScript;
extern const CLSID CLSID_VBScriptRegExp;

#define DEFINE_EXPECT(func) \
    static BOOL expect_ ## func = FALSE, called_ ## func = FALSE

#define SET_EXPECT(func) \
    expect_ ## func = TRUE

#define REF_EXPECT(func) \
    (&expect_ ## func), (&called_ ## func)

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

#define CHECK_NOT_CALLED(func) \
    do { \
        ok(!called_ ## func, "unexpected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

#define CLEAR_CALLED(func) \
    expect_ ## func = called_ ## func = FALSE

DEFINE_EXPECT(global_success_d);
DEFINE_EXPECT(global_success_i);
DEFINE_EXPECT(global_vbvar_d);
DEFINE_EXPECT(global_vbvar_i);
DEFINE_EXPECT(global_letobj_i);
DEFINE_EXPECT(global_setobj_i);
DEFINE_EXPECT(global_globalcallback_i);
DEFINE_EXPECT(testobj_propget_d);
DEFINE_EXPECT(testobj_propget_i);
DEFINE_EXPECT(testobj_propput_d);
DEFINE_EXPECT(testobj_propput_i);
DEFINE_EXPECT(testobj_value_i);
DEFINE_EXPECT(testobj_valueput_i);
DEFINE_EXPECT(global_propargput_d);
DEFINE_EXPECT(global_propargput_i);
DEFINE_EXPECT(global_propargput1_d);
DEFINE_EXPECT(global_propargput1_i);
DEFINE_EXPECT(global_propargset_d);
DEFINE_EXPECT(global_propargset_i);
DEFINE_EXPECT(global_testoptionalarg_i);
DEFINE_EXPECT(global_testerrorobject_i);
DEFINE_EXPECT(collectionobj_newenum_i);
DEFINE_EXPECT(Next);
DEFINE_EXPECT(GetWindow);
DEFINE_EXPECT(GetUIBehavior);
DEFINE_EXPECT(EnableModeless);
DEFINE_EXPECT(OnScriptError);
DEFINE_EXPECT(OnEnterScript);
DEFINE_EXPECT(OnLeaveScript);

#define DISPID_GLOBAL_REPORTSUCCESS 1000
#define DISPID_GLOBAL_TRACE         1001
#define DISPID_GLOBAL_OK            1002
#define DISPID_GLOBAL_GETVT         1003
#define DISPID_GLOBAL_ISENGLANG     1004
#define DISPID_GLOBAL_VBVAR         1005
#define DISPID_GLOBAL_TESTOBJ       1006
#define DISPID_GLOBAL_ISNULLDISP    1007
#define DISPID_GLOBAL_TESTDISP      1008
#define DISPID_GLOBAL_REFOBJ        1009
#define DISPID_GLOBAL_COUNTER       1010
#define DISPID_GLOBAL_PROPARGPUT    1011
#define DISPID_GLOBAL_PROPARGPUT1   1012
#define DISPID_GLOBAL_COLLOBJ       1013
#define DISPID_GLOBAL_DOUBLEASSTRING 1014
#define DISPID_GLOBAL_TESTARRAY     1015
#define DISPID_GLOBAL_THROWINT      1016
#define DISPID_GLOBAL_TESTOPTIONALARG 1017
#define DISPID_GLOBAL_LETOBJ        1018
#define DISPID_GLOBAL_SETOBJ        1019
#define DISPID_GLOBAL_TODO_WINE_OK  1020
#define DISPID_GLOBAL_WEEKSTARTDAY  1021
#define DISPID_GLOBAL_GLOBALCALLBACK  1022
#define DISPID_GLOBAL_TESTERROROBJECT 1023
#define DISPID_GLOBAL_THROWWITHDESC   1024
#define DISPID_GLOBAL_PROPARGSET      1025
#define DISPID_GLOBAL_UNKOBJ          1026
#define DISPID_GLOBAL_THROWEXCEPTION  1027
#define DISPID_GLOBAL_ISARRAYFIXED    1028
#define DISPID_GLOBAL_MAXCHARSIZE     1029

#define DISPID_TESTOBJ_PROPGET      2000
#define DISPID_TESTOBJ_PROPPUT      2001
#define DISPID_TESTOBJ_KEYWORD      2002

#define DISPID_COLLOBJ_RESET        3000

#define FACILITY_VBS 0xa
#define MAKE_VBSERROR(code) MAKE_HRESULT(SEVERITY_ERROR, FACILITY_VBS, code)

static BOOL strict_dispid_check, is_english, allow_ui;
static UINT MaxCharSize;
static int first_day_of_week;
static const char *test_name = "(null)";
static int test_counter;
static SCRIPTUICHANDLING uic_handling = SCRIPTUICHANDLING_NOUIERROR;
static IDispatchEx testObj;
static HRESULT onerror_hres = E_NOTIMPL;
static BOOL strict_enter_script;

static BSTR a2bstr(const char *str)
{
    BSTR ret;
    int len;

    len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    ret = SysAllocStringLen(NULL, len-1);
    MultiByteToWideChar(CP_ACP, 0, str, -1, ret, len);

    return ret;
}

static const char *vt2a(VARIANT *v)
{
    if(V_VT(v) == (VT_BYREF|VT_VARIANT)) {
        static char buf[64];
        sprintf(buf, "%s*", vt2a(V_BYREF(v)));
        return buf;
    }

    switch(V_VT(v)) {
    case VT_EMPTY:
        return "VT_EMPTY";
    case VT_NULL:
        return "VT_NULL";
    case VT_I2:
        return "VT_I2";
    case VT_I4:
        return "VT_I4";
    case VT_R4:
        return "VT_R4";
    case VT_R8:
        return "VT_R8";
    case VT_CY:
        return "VT_CY";
    case VT_DATE:
        return "VT_DATE";
    case VT_BSTR:
        return "VT_BSTR";
    case VT_DISPATCH:
        return "VT_DISPATCH";
    case VT_UNKNOWN:
        return "VT_UNKNOWN";
    case VT_BOOL:
        return "VT_BOOL";
    case VT_ARRAY|VT_VARIANT:
        return "VT_ARRAY|VT_VARIANT";
    case VT_ARRAY|VT_BYREF|VT_VARIANT:
        return "VT_ARRAY|VT_BYREF|VT_VARIANT";
    case VT_UI1:
        return "VT_UI1";
    default:
        ok(0, "unknown vt %d\n", V_VT(v));
        return NULL;
    }
}

/* Sets is_english to true if the user interface is in English. Note that this
 * does not presume the formatting of dates, numbers, etc.
 * Sets first_day_of_week to 1 if Sunday, 2 if Monday, and so on.
 */
static void detect_locale(void)
{
    UINT cp;
    CPINFOEXA cpinfo;
    HMODULE kernel32 = GetModuleHandleA("kernel32.dll");
    LANGID (WINAPI *pGetThreadUILanguage)(void) = (void*)GetProcAddress(kernel32, "GetThreadUILanguage");

    is_english = ((!pGetThreadUILanguage || PRIMARYLANGID(pGetThreadUILanguage()) == LANG_ENGLISH) &&
                  PRIMARYLANGID(GetUserDefaultUILanguage()) == LANG_ENGLISH &&
                  PRIMARYLANGID(GetUserDefaultLangID()) == LANG_ENGLISH);

    GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_IDEFAULTANSICODEPAGE | LOCALE_RETURN_NUMBER, (WCHAR*)&cp, sizeof(cp));
    if (cp)
    {
        GetCPInfoExA( cp, 0, &cpinfo );
        MaxCharSize = cpinfo.MaxCharSize;
    }
    else
    {
        /* No ANSI code page for that locale -> the fallback is UTF-8 */
        MaxCharSize = 4;
    }

    GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_IFIRSTDAYOFWEEK | LOCALE_RETURN_NUMBER,
                   (void*)&first_day_of_week, sizeof(first_day_of_week));
    first_day_of_week = 1 + (first_day_of_week + 1) % 7;
}

static HRESULT WINAPI ServiceProvider_QueryInterface(IServiceProvider *iface, REFIID riid, void **ppv)
{
    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI ServiceProvider_AddRef(IServiceProvider *iface)
{
    return 2;
}

static ULONG WINAPI ServiceProvider_Release(IServiceProvider *iface)
{
    return 1;
}

static HRESULT WINAPI ServiceProvider_QueryService(IServiceProvider *iface, REFGUID guidService,
        REFIID riid, void **ppv)
{
    ok(0, "unexpected service %s\n", wine_dbgstr_guid(guidService));
    return E_NOINTERFACE;
}

static const IServiceProviderVtbl ServiceProviderVtbl = {
    ServiceProvider_QueryInterface,
    ServiceProvider_AddRef,
    ServiceProvider_Release,
    ServiceProvider_QueryService
};

static IServiceProvider caller_sp = { &ServiceProviderVtbl };

static void test_disp(IDispatch *disp)
{
    DISPID id, public_prop_id, public_prop2_id, public_func_id, public_sub_id, defvalget_id, gs_getter_id;
    DISPID named_args[5] = {DISPID_PROPERTYPUT};
    VARIANT v, args[5];
    DISPPARAMS dp = {args, named_args};
    IDispatchEx *dispex;
    EXCEPINFO ei = {0};
    BSTR str;
    HRESULT hres;

    hres = IDispatch_QueryInterface(disp, &IID_IDispatchEx, (void**)&dispex);
    ok(hres == S_OK, "Could not get IDispatchEx iface: %08lx\n", hres);

    str = SysAllocString(L"publicProp");
    hres = IDispatchEx_GetDispID(dispex, str, fdexNameCaseInsensitive, &public_prop_id);
    SysFreeString(str);
    ok(hres == S_OK, "GetDispID(publicProp) failed: %08lx\n", hres);

    str = SysAllocString(L"PUBLICPROP");
    hres = IDispatchEx_GetDispID(dispex, str, 0, &id);
    SysFreeString(str);
    ok(hres == S_OK, "GetDispID(PUBLICPROP) failed: %08lx\n", hres);
    ok(public_prop_id == id, "id = %ld\n", public_prop_id);

    str = SysAllocString(L"publicPROP2");
    hres = IDispatchEx_GetDispID(dispex, str, fdexNameCaseInsensitive, &public_prop2_id);
    SysFreeString(str);
    ok(hres == S_OK, "GetDispID(publicProp2) failed: %08lx\n", hres);

    str = SysAllocString(L"defValGet");
    hres = IDispatchEx_GetDispID(dispex, str, fdexNameCaseInsensitive, &defvalget_id);
    SysFreeString(str);
    ok(hres == S_OK, "GetDispID(defValGet) failed: %08lx\n", hres);
    ok(defvalget_id == DISPID_VALUE, "id = %ld\n", defvalget_id);

    str = SysAllocString(L"privateProp");
    hres = IDispatchEx_GetDispID(dispex, str, fdexNameCaseInsensitive, &id);
    SysFreeString(str);
    ok(hres == DISP_E_UNKNOWNNAME, "GetDispID(privateProp) failed: %08lx, expected DISP_E_UNKNOWNNAME\n", hres);
    ok(id == -1, "id = %ld\n", id);

    str = SysAllocString(L"class_initialize");
    hres = IDispatchEx_GetDispID(dispex, str, fdexNameCaseInsensitive, &id);
    SysFreeString(str);
    ok(hres == S_OK, "GetDispID(publicProp2) failed: %08lx\n", hres);

    hres = IDispatchEx_InvokeEx(dispex, public_prop_id, 0, DISPATCH_PROPERTYGET|DISPATCH_METHOD, &dp, &v, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_EMPTY, "V_VT(v) = %d\n", V_VT(&v));

    V_VT(args) = VT_BOOL;
    V_BOOL(args) = VARIANT_TRUE;
    dp.cArgs = dp.cNamedArgs = 1;
    V_VT(&v) = VT_BOOL;
    hres = IDispatchEx_InvokeEx(dispex, public_prop_id, 0, DISPATCH_PROPERTYPUT, &dp, &v, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_EMPTY, "V_VT(v) = %d\n", V_VT(&v));

    dp.cArgs = dp.cNamedArgs = 0;
    hres = IDispatchEx_InvokeEx(dispex, public_prop_id, 0, DISPATCH_PROPERTYGET|DISPATCH_METHOD, &dp, &v, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_BOOL, "V_VT(v) = %d\n", V_VT(&v));
    ok(V_BOOL(&v), "V_BOOL(v) = %x\n", V_BOOL(&v));

    dp.cArgs = dp.cNamedArgs = 0;
    hres = IDispatchEx_Invoke(dispex, public_prop_id, &IID_NULL, 0, DISPATCH_PROPERTYGET|DISPATCH_METHOD, &dp, &v, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_BOOL, "V_VT(v) = %d\n", V_VT(&v));
    ok(V_BOOL(&v), "V_BOOL(v) = %x\n", V_BOOL(&v));

    dp.cArgs = dp.cNamedArgs = 0;
    hres = IDispatchEx_InvokeEx(dispex, public_prop_id, 0, DISPATCH_PROPERTYGET, &dp, &v, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_BOOL, "V_VT(v) = %d\n", V_VT(&v));
    ok(V_BOOL(&v), "V_BOOL(v) = %x\n", V_BOOL(&v));

    dp.cArgs = 1;
    hres = IDispatchEx_InvokeEx(dispex, public_prop_id, 0, DISPATCH_PROPERTYGET|DISPATCH_METHOD, &dp, &v, &ei, NULL);
    ok(hres == DISP_E_MEMBERNOTFOUND, "InvokeEx failed: %08lx, expected DISP_E_MEMBERNOTFOUND\n", hres);
    ok(V_VT(&v) == VT_EMPTY, "V_VT(v) = %d\n", V_VT(&v));

    SET_EXPECT(testobj_value_i);
    V_VT(args) = VT_DISPATCH;
    V_DISPATCH(args) = (IDispatch*)&testObj;
    dp.cArgs = dp.cNamedArgs = 1;
    hres = IDispatchEx_InvokeEx(dispex, public_prop_id, 0, DISPATCH_PROPERTYPUT, &dp, NULL, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    CHECK_CALLED(testobj_value_i);

    dp.cArgs = dp.cNamedArgs = 0;
    hres = IDispatchEx_InvokeEx(dispex, public_prop_id, 0, DISPATCH_PROPERTYGET, &dp, &v, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_I2, "V_VT(v) = %d\n", V_VT(&v));
    ok(V_I2(&v) == 0, "V_I2(v) = %d\n", V_I2(&v));

    V_VT(args) = VT_DISPATCH;
    V_DISPATCH(args) = (IDispatch*)&testObj;
    dp.cArgs = dp.cNamedArgs = 1;
    hres = IDispatchEx_InvokeEx(dispex, public_prop_id, 0, DISPATCH_PROPERTYPUT|DISPATCH_PROPERTYPUTREF, &dp, NULL, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);

    V_VT(args) = VT_DISPATCH;
    V_DISPATCH(args) = (IDispatch*)&testObj;
    dp.cArgs = dp.cNamedArgs = 1;
    hres = IDispatchEx_InvokeEx(dispex, public_prop_id, 0, DISPATCH_PROPERTYPUTREF, &dp, NULL, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);

    dp.cArgs = dp.cNamedArgs = 0;
    hres = IDispatchEx_InvokeEx(dispex, public_prop_id, 0, DISPATCH_PROPERTYGET, &dp, &v, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_DISPATCH, "V_VT(v) = %d\n", V_VT(&v));
    ok(V_DISPATCH(&v) == (IDispatch*)&testObj, "V_DISPATCH(v) != testObj\n");

    V_VT(args) = VT_BOOL;
    V_BOOL(args) = VARIANT_TRUE;
    dp.cArgs = dp.cNamedArgs = 1;
    hres = IDispatchEx_InvokeEx(dispex, public_prop_id, 0, DISPATCH_PROPERTYPUT|DISPATCH_PROPERTYPUTREF, &dp, NULL, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);

    dp.cArgs = dp.cNamedArgs = 0;
    hres = IDispatchEx_InvokeEx(dispex, public_prop_id, 0, DISPATCH_PROPERTYGET, &dp, &v, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_BOOL, "V_VT(v) = %d\n", V_VT(&v));
    ok(V_BOOL(&v) == VARIANT_TRUE, "V_BOOL(v) = %x\n", V_BOOL(&v));

    V_VT(args) = VT_BOOL;
    V_BOOL(args) = VARIANT_FALSE;
    dp.cArgs = dp.cNamedArgs = 1;
    hres = IDispatchEx_InvokeEx(dispex, public_prop_id, 0, DISPATCH_PROPERTYPUTREF, &dp, NULL, &ei, NULL);
    ok(hres == DISP_E_EXCEPTION, "InvokeEx failed: %08lx, expected DISP_E_EXCEPTION\n", hres);

    V_VT(args) = VT_BOOL;
    V_BOOL(args) = VARIANT_FALSE;
    dp.cArgs = 1;
    dp.cNamedArgs = 0;
    V_VT(&v) = VT_BOOL;
    hres = IDispatchEx_InvokeEx(dispex, public_prop_id, 0, DISPATCH_PROPERTYPUT, &dp, NULL, &ei, NULL);
    ok(hres == DISP_E_PARAMNOTOPTIONAL, "InvokeEx failed: %08lx, expected DISP_E_PARAMNOTOPTIONAL\n", hres);

    str = SysAllocString(L"publicFunction");
    hres = IDispatchEx_GetDispID(dispex, str, fdexNameCaseInsensitive, &public_func_id);
    SysFreeString(str);
    ok(hres == S_OK, "GetDispID(publicFunction) failed: %08lx\n", hres);
    ok(public_func_id != -1, "public_func_id = -1\n");

    str = SysAllocString(L"publicSub");
    hres = IDispatchEx_GetDispID(dispex, str, fdexNameCaseInsensitive, &public_sub_id);
    SysFreeString(str);
    ok(hres == S_OK, "GetDispID(publicSub) failed: %08lx\n", hres);
    ok(public_sub_id != -1, "public_func_id = -1\n");

    dp.cArgs = dp.cNamedArgs = 0;
    hres = IDispatchEx_InvokeEx(dispex, public_func_id, 0, DISPATCH_PROPERTYGET|DISPATCH_METHOD, &dp, &v, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_I2, "V_VT(v) = %d\n", V_VT(&v));
    ok(V_I2(&v) == 4, "V_I2(v) = %d\n", V_I2(&v));

    dp.cArgs = dp.cNamedArgs = 0;
    hres = IDispatchEx_InvokeEx(dispex, public_func_id, 0, DISPATCH_PROPERTYGET, &dp, &v, &ei, NULL);
    ok(hres == DISP_E_MEMBERNOTFOUND, "InvokeEx failed: %08lx, expected DISP_E_MEMBERNOTFOUND\n", hres);
    ok(V_VT(&v) == VT_EMPTY, "V_VT(v) = %d\n", V_VT(&v));

    dp.cArgs = dp.cNamedArgs = 0;
    hres = IDispatchEx_InvokeEx(dispex, public_func_id, 0, DISPATCH_METHOD, &dp, &v, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_I2, "V_VT(v) = %d\n", V_VT(&v));
    ok(V_I2(&v) == 4, "V_I2(v) = %d\n", V_I2(&v));

    dp.cArgs = dp.cNamedArgs = 0;
    hres = IDispatchEx_InvokeEx(dispex, public_sub_id, 0, DISPATCH_PROPERTYGET|DISPATCH_METHOD, &dp, &v, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_EMPTY, "V_VT(v) = %d\n", V_VT(&v));

    dp.cArgs = dp.cNamedArgs = 0;
    hres = IDispatchEx_InvokeEx(dispex, public_sub_id, 0, DISPATCH_PROPERTYGET, &dp, &v, &ei, NULL);
    ok(hres == DISP_E_MEMBERNOTFOUND, "InvokeEx failed: %08lx, expected DISP_E_MEMBERNOTFOUND\n", hres);
    ok(V_VT(&v) == VT_EMPTY, "V_VT(v) = %d\n", V_VT(&v));

    dp.cArgs = dp.cNamedArgs = 0;
    hres = IDispatchEx_InvokeEx(dispex, public_sub_id, 0, DISPATCH_METHOD, &dp, &v, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_EMPTY, "V_VT(v) = %d\n", V_VT(&v));

    V_VT(args) = VT_BOOL;
    V_BOOL(args) = VARIANT_TRUE;
    dp.cArgs = dp.cNamedArgs = 1;
    hres = IDispatchEx_InvokeEx(dispex, public_sub_id, 0, DISPATCH_PROPERTYPUT, &dp, NULL, &ei, NULL);
    ok(FAILED(hres), "InvokeEx succeeded: %08lx\n", hres);

    dp.cArgs = dp.cNamedArgs = 0;
    hres = IDispatchEx_InvokeEx(dispex, public_func_id, 0, DISPATCH_PROPERTYGET|DISPATCH_METHOD, &dp, &v, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_I2, "V_VT(v) = %d\n", V_VT(&v));
    ok(V_I2(&v) == 4, "V_I2(v) = %d\n", V_I2(&v));

    dp.cArgs = dp.cNamedArgs = 0;
    hres = IDispatchEx_InvokeEx(dispex, public_func_id, 0, DISPATCH_METHOD, &dp, &v, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_I2, "V_VT(v) = %d\n", V_VT(&v));
    ok(V_I2(&v) == 4, "V_I2(v) = %d\n", V_I2(&v));

    dp.cArgs = dp.cNamedArgs = 0;
    hres = IDispatchEx_Invoke(dispex, public_func_id, &IID_NULL, 0, DISPATCH_METHOD, &dp, &v, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_I2, "V_VT(v) = %d\n", V_VT(&v));
    ok(V_I2(&v) == 4, "V_I2(v) = %d\n", V_I2(&v));

    dp.cArgs = dp.cNamedArgs = 0;
    hres = IDispatchEx_InvokeEx(dispex, public_sub_id, 0, DISPATCH_PROPERTYGET|DISPATCH_METHOD, &dp, &v, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_EMPTY, "V_VT(v) = %d\n", V_VT(&v));

    dp.cArgs = dp.cNamedArgs = 0;
    hres = IDispatchEx_InvokeEx(dispex, public_sub_id, 0, DISPATCH_METHOD, &dp, &v, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_EMPTY, "V_VT(v) = %d\n", V_VT(&v));

    str = SysAllocString(L"privateSub");
    hres = IDispatchEx_GetDispID(dispex, str, fdexNameCaseInsensitive, &id);
    SysFreeString(str);
    ok(hres == DISP_E_UNKNOWNNAME, "GetDispID(privateSub) failed: %08lx, expected DISP_E_UNKNOWNNAME\n", hres);
    ok(id == -1, "id = %ld\n", id);

    str = SysAllocString(L"dynprop");
    hres = IDispatchEx_GetDispID(dispex, str, fdexNameCaseInsensitive|fdexNameEnsure, &id);
    ok(hres == DISP_E_UNKNOWNNAME, "GetDispID(privateProp) failed: %08lx, expected DISP_E_UNKNOWNNAME\n", hres);
    ok(id == -1, "id = %ld\n", id);
    hres = IDispatchEx_GetDispID(dispex, str, fdexNameEnsure, &id);
    ok(hres == DISP_E_UNKNOWNNAME, "GetDispID(privateProp) failed: %08lx, expected DISP_E_UNKNOWNNAME\n", hres);
    ok(id == -1, "id = %ld\n", id);
    SysFreeString(str);

    str = SysAllocString(L"publicProp");
    hres = IDispatchEx_GetDispID(dispex, str, 0x80000000|fdexNameCaseInsensitive, &public_prop_id);
    SysFreeString(str);
    ok(hres == S_OK, "GetDispID(publicProp) failed: %08lx\n", hres);

    id = 0xdeadbeef;
    str = SysAllocString(L"publicProp");
    hres = IDispatchEx_GetDispID(dispex, str, fdexNameCaseSensitive, &id);
    SysFreeString(str);
    ok(hres == S_OK, "GetDispID(publicProp) failed: %08lx\n", hres);
    ok(id == public_prop_id, "id = %ld, expected %ld\n", id, public_prop_id);

    id = 0xdeadbeef;
    str = SysAllocString(L"publicprop");
    hres = IDispatchEx_GetDispID(dispex, str, fdexNameCaseSensitive, &id);
    SysFreeString(str);
    ok(hres == S_OK, "GetDispID(publicProp) failed: %08lx\n", hres);
    ok(id == public_prop_id, "id = %ld, expected %ld\n", id, public_prop_id);

    str = SysAllocString(L"gsGetProp");
    hres = IDispatchEx_GetDispID(dispex, str, fdexNameCaseInsensitive, &gs_getter_id);
    SysFreeString(str);
    ok(hres == S_OK, "GetDispID(publicFunction) failed: %08lx\n", hres);
    ok(gs_getter_id != -1, "gs_getter_id = -1\n");

    V_VT(args) = VT_BOOL;
    V_BOOL(args) = VARIANT_TRUE;
    dp.cNamedArgs = 0;
    dp.cArgs = 1;
    V_VT(&v) = VT_I8;
    hres = IDispatchEx_InvokeEx(dispex, gs_getter_id, 0, DISPATCH_PROPERTYGET, &dp, &v, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_BOOL && V_BOOL(&v), "V_VT(v) = %d\n", V_VT(&v));

    hres = IDispatchEx_InvokeEx(dispex, gs_getter_id, 0, DISPATCH_PROPERTYGET, &dp, NULL, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);

    V_VT(args) = VT_BOOL;
    V_BOOL(args) = VARIANT_FALSE;
    dp.cArgs = 1;
    V_VT(&v) = VT_I8;
    hres = IDispatchEx_InvokeEx(dispex, gs_getter_id, 0, DISPATCH_PROPERTYGET|DISPATCH_METHOD, &dp, &v, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_BOOL && !V_BOOL(&v), "V_VT(v) = %d\n", V_VT(&v));

    V_VT(args) = VT_BOOL;
    V_BOOL(args) = VARIANT_TRUE;
    V_VT(&v) = VT_I8;
    dp.cArgs = 1;
    hres = IDispatchEx_InvokeEx(dispex, gs_getter_id, 0, DISPATCH_METHOD, &dp, &v, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_BOOL && V_BOOL(&v), "V_VT(v) = %d\n", V_VT(&v));

    IDispatchEx_Release(dispex);
}

static void test_safearray(SAFEARRAY *safearray, unsigned indims)
{
    int i, exdims = indims;

    if(!exdims)
        exdims = 1;
    ok(safearray->cDims == exdims, "safearray->cDims = %d, expected %d\n", safearray->cDims, exdims);
    ok(safearray->fFeatures == (FADF_VARIANT|FADF_HAVEVARTYPE|FADF_FIXEDSIZE|FADF_STATIC),
       "safearray->fFeatures = %x\n", safearray->fFeatures);
    ok(safearray->cbElements == sizeof(VARIANT), "safearray->cbElements = %lx\n", safearray->cbElements);
    ok(!safearray->cLocks, "safearray->cLocks = %lx\n", safearray->cLocks);

    for(i=0; i < safearray->cDims; i++) {
        ok(safearray->rgsabound[i].cElements == (indims ? i+4 : 1), "safearray->rgsabound[%d].cElements = %ld\n", i,
           safearray->rgsabound[i].cElements);
        ok(!safearray->rgsabound[i].lLbound, "safearray->rgsabound[%d].lLbound = %ld\n", i, safearray->rgsabound[i].lLbound);
    }
}

#define test_grfdex(a,b) _test_grfdex(__LINE__,a,b)
static void _test_grfdex(unsigned line, DWORD grfdex, DWORD expect)
{
    ok_(__FILE__,line)(grfdex == expect, "grfdex = %lx, expected %lx\n", grfdex, expect);
}

static IDispatchEx enumDisp;

static HRESULT WINAPI unkObj_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(riid, &IID_IUnknown)) {
        *ppv = iface;
        return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI unkObj_AddRef(IUnknown *iface)
{
    return 2;
}

static ULONG WINAPI unkObj_Release(IUnknown *iface)
{
    return 1;
}

static const IUnknownVtbl unkObjVtbl = {
    unkObj_QueryInterface,
    unkObj_AddRef,
    unkObj_Release
};

static IUnknown unkObj = { &unkObjVtbl };

static HRESULT WINAPI EnumVARIANT_QueryInterface(IEnumVARIANT *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IEnumVARIANT)) {
        *ppv = iface;
        return S_OK;
    }

    if(IsEqualGUID(riid, &IID_IDispatch)) {
        *ppv = &enumDisp;
        return S_OK;
    }

    ok(0, "unexpected call %s\n", wine_dbgstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI EnumVARIANT_AddRef(IEnumVARIANT *iface)
{
    return 2;
}

static ULONG WINAPI EnumVARIANT_Release(IEnumVARIANT *iface)
{
    return 1;
}

static unsigned next_cnt;

static HRESULT WINAPI EnumVARIANT_Next(IEnumVARIANT *iface, ULONG celt, VARIANT *rgVar, ULONG *pCeltFetched)
{
    if(strict_dispid_check)
        CHECK_EXPECT2(Next);

    ok(celt == 1, "celt = %ld\n", celt);
    ok(V_VT(rgVar) == VT_EMPTY, "V_VT(rgVar) = %d\n", V_VT(rgVar));
    ok(!pCeltFetched, "pCeltFetched = %p\n", pCeltFetched);

    if(next_cnt++ < 3) {
        V_VT(rgVar) = VT_I2;
        V_I2(rgVar) = next_cnt;
        return S_OK;
    }

    return S_FALSE;
}

static HRESULT WINAPI EnumVARIANT_Skip(IEnumVARIANT *iface, ULONG celt)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI EnumVARIANT_Reset(IEnumVARIANT *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI EnumVARIANT_Clone(IEnumVARIANT *iface, IEnumVARIANT **ppEnum)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IEnumVARIANTVtbl EnumVARIANTVtbl = {
    EnumVARIANT_QueryInterface,
    EnumVARIANT_AddRef,
    EnumVARIANT_Release,
    EnumVARIANT_Next,
    EnumVARIANT_Skip,
    EnumVARIANT_Reset,
    EnumVARIANT_Clone
};

static IEnumVARIANT enumObj = { &EnumVARIANTVtbl };

typedef struct {
    const WCHAR *name;
    DISPID pid;
    BOOL *expect;
    BOOL *called;
} dispid_t;

static BOOL get_dispid(BSTR name, const dispid_t *dispids, SIZE_T dispids_cnt, DISPID *id)
{
    int i;

    for(i = 0; i < dispids_cnt; i++) {
        if(!wcsicmp(name, dispids[i].name)) {
            const dispid_t *d = &dispids[i];
            if(d->expect) {
                ok(*d->expect, "unexpected call %s\n", wine_dbgstr_w(d->name));
                *d->called = TRUE;
                *d->expect = FALSE;
            }
            *id = d->pid;
            return TRUE;
        }
    }

    return FALSE;
}

static HRESULT WINAPI DispatchEx_QueryInterface(IDispatchEx *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(riid, &IID_IUnknown)
       || IsEqualGUID(riid, &IID_IDispatch)
       || IsEqualGUID(riid, &IID_IDispatchEx))
        *ppv = iface;
    else {
        trace("QI %s\n", wine_dbgstr_guid(riid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
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

static HRESULT WINAPI DispatchEx_GetTypeInfo(IDispatchEx *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetIDsOfNames(IDispatchEx *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_Invoke(IDispatchEx *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_DeleteMemberByName(IDispatchEx *iface, BSTR bstrName, DWORD grfdex)
{
    ok(0, "unexpected call %s %lx\n", wine_dbgstr_w(bstrName), grfdex);
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_DeleteMemberByDispID(IDispatchEx *iface, DISPID id)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetMemberProperties(IDispatchEx *iface, DISPID id, DWORD grfdexFetch, DWORD *pgrfdex)
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

static HRESULT WINAPI DispatchEx_GetDispID(IDispatchEx *iface, BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_InvokeEx(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    ok(0, "unexpected call %ld\n", id);
    return E_NOTIMPL;
}

static HRESULT WINAPI testObj_GetDispID(IDispatchEx *iface, BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    static const dispid_t dispids[] = {
       { L"propget",  DISPID_TESTOBJ_PROPGET, REF_EXPECT(testobj_propget_d) },
       { L"propput",  DISPID_TESTOBJ_PROPPUT, REF_EXPECT(testobj_propput_d) },
       { L"rem",      DISPID_TESTOBJ_KEYWORD },
       { L"true",     DISPID_TESTOBJ_KEYWORD },
       { L"false",    DISPID_TESTOBJ_KEYWORD },
       { L"not",      DISPID_TESTOBJ_KEYWORD },
       { L"and",      DISPID_TESTOBJ_KEYWORD },
       { L"or",       DISPID_TESTOBJ_KEYWORD },
       { L"xor",      DISPID_TESTOBJ_KEYWORD },
       { L"eqv",      DISPID_TESTOBJ_KEYWORD },
       { L"imp",      DISPID_TESTOBJ_KEYWORD },
       { L"is",       DISPID_TESTOBJ_KEYWORD },
       { L"mod",      DISPID_TESTOBJ_KEYWORD },
       { L"call",     DISPID_TESTOBJ_KEYWORD },
       { L"dim",      DISPID_TESTOBJ_KEYWORD },
       { L"sub",      DISPID_TESTOBJ_KEYWORD },
       { L"function", DISPID_TESTOBJ_KEYWORD },
       { L"get",      DISPID_TESTOBJ_KEYWORD },
       { L"let",      DISPID_TESTOBJ_KEYWORD },
       { L"const",    DISPID_TESTOBJ_KEYWORD },
       { L"if",       DISPID_TESTOBJ_KEYWORD },
       { L"else",     DISPID_TESTOBJ_KEYWORD },
       { L"elseif",   DISPID_TESTOBJ_KEYWORD },
       { L"end",      DISPID_TESTOBJ_KEYWORD },
       { L"then",     DISPID_TESTOBJ_KEYWORD },
       { L"exit",     DISPID_TESTOBJ_KEYWORD },
       { L"while",    DISPID_TESTOBJ_KEYWORD },
       { L"wend",     DISPID_TESTOBJ_KEYWORD },
       { L"do",       DISPID_TESTOBJ_KEYWORD },
       { L"loop",     DISPID_TESTOBJ_KEYWORD },
       { L"until",    DISPID_TESTOBJ_KEYWORD },
       { L"for",      DISPID_TESTOBJ_KEYWORD },
       { L"to",       DISPID_TESTOBJ_KEYWORD },
       { L"each",     DISPID_TESTOBJ_KEYWORD },
       { L"in",       DISPID_TESTOBJ_KEYWORD },
       { L"select",   DISPID_TESTOBJ_KEYWORD },
       { L"case",     DISPID_TESTOBJ_KEYWORD },
       { L"byref",    DISPID_TESTOBJ_KEYWORD },
       { L"byval",    DISPID_TESTOBJ_KEYWORD },
       { L"option",   DISPID_TESTOBJ_KEYWORD },
       { L"nothing",  DISPID_TESTOBJ_KEYWORD },
       { L"empty",    DISPID_TESTOBJ_KEYWORD },
       { L"null",     DISPID_TESTOBJ_KEYWORD },
       { L"class",    DISPID_TESTOBJ_KEYWORD },
       { L"set",      DISPID_TESTOBJ_KEYWORD },
       { L"new",      DISPID_TESTOBJ_KEYWORD },
       { L"public",   DISPID_TESTOBJ_KEYWORD },
       { L"private",  DISPID_TESTOBJ_KEYWORD },
       { L"next",     DISPID_TESTOBJ_KEYWORD },
       { L"on",       DISPID_TESTOBJ_KEYWORD },
       { L"resume",   DISPID_TESTOBJ_KEYWORD },
       { L"goto",     DISPID_TESTOBJ_KEYWORD },
       { L"redim",    DISPID_TESTOBJ_KEYWORD },
       { L"preserve", DISPID_TESTOBJ_KEYWORD },
       { L"with",     DISPID_TESTOBJ_KEYWORD },
       { L"property", DISPID_TESTOBJ_KEYWORD },
       { L"me",       DISPID_TESTOBJ_KEYWORD },
       { L"stop",     DISPID_TESTOBJ_KEYWORD }
    };

    test_grfdex(grfdex, fdexNameCaseInsensitive);
    if(get_dispid(bstrName, dispids, ARRAY_SIZE(dispids), pid))
        return S_OK;
    ok(0, "unexpected call %s\n", wine_dbgstr_w(bstrName));
    return DISP_E_UNKNOWNNAME;
}

static HRESULT WINAPI testObj_InvokeEx(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    switch(id) {
    case DISPID_VALUE:
        if(wFlags == (DISPATCH_PROPERTYGET|DISPATCH_METHOD)) {
            VARIANT *arg;
            int i;

            CHECK_EXPECT(testobj_value_i);

            ok(pdp != NULL, "pdp == NULL\n");
            ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
            ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
            ok(pvarRes != NULL, "pvarRes == NULL\n");
            ok(pei != NULL, "pei == NULL\n");

            for(i=0; i<pdp->cArgs; i++) {
                arg = pdp->rgvarg+pdp->cArgs-i-1;
                ok(V_VT(arg) == VT_I2, "V_VT(arg) = %d\n", V_VT(arg));
                ok(V_I2(arg) == i+1, "V_I2(arg) = %d\n", V_I2(arg));
            }

            V_VT(pvarRes) = VT_I2;
            V_I2(pvarRes) = pdp->cArgs;
            return S_OK;
        }
        if(wFlags == DISPATCH_PROPERTYPUT) {
            CHECK_EXPECT(testobj_valueput_i);

            ok(pdp->cArgs == 3, "cArgs = %d\n", pdp->cArgs);
            ok(pdp->cNamedArgs == 1, "cNamedArgs = %d\n", pdp->cNamedArgs);
            ok(pdp->rgdispidNamedArgs[0] == DISPID_PROPERTYPUT, "pdp->rgdispidNamedArgs[0] = %ld\n", pdp->rgdispidNamedArgs[0]);
            ok(!pvarRes, "pvarRes != NULL\n");
            ok(pei != NULL, "pei == NULL\n");

            ok(V_VT(pdp->rgvarg) == VT_I2, "V_VT(args[0]) = %d\n", V_VT(pdp->rgvarg));
            ok(V_I2(pdp->rgvarg) == 0, "V_I2(args[0]) = %d\n", V_I2(pdp->rgvarg));
            ok(V_VT(pdp->rgvarg+1) == VT_I2, "V_VT(args[1]) = %d\n", V_VT(pdp->rgvarg+1));
            ok(V_I2(pdp->rgvarg+1) == 2, "V_I2(args[1]) = %d\n", V_I2(pdp->rgvarg+1));
            ok(V_VT(pdp->rgvarg+2) == VT_I2, "V_VT(args[2]) = %d\n", V_VT(pdp->rgvarg+2));
            ok(V_I2(pdp->rgvarg+2) == 1, "V_I2(args[2]) = %d\n", V_I2(pdp->rgvarg+2));

            return S_OK;
        }
        ok(0, "wFlags = %x\n", wFlags);
        break;
    case DISPID_TESTOBJ_PROPGET:
        CHECK_EXPECT(testobj_propget_i);

        ok(wFlags == (DISPATCH_PROPERTYGET|DISPATCH_METHOD), "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(!pdp->rgvarg, "rgvarg == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(!pdp->cArgs, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pvarRes != NULL, "pvarRes == NULL\n");
        ok(pei != NULL, "pei == NULL\n");

        V_VT(pvarRes) = VT_I2;
        V_I2(pvarRes) = 10;
        return S_OK;
    case DISPID_TESTOBJ_PROPPUT:
        CHECK_EXPECT(testobj_propput_i);

        ok(wFlags == DISPATCH_PROPERTYPUT, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(pdp->rgdispidNamedArgs != NULL, "rgdispidNamedArgs == NULL\n");
        ok(pdp->cArgs == 1, "cArgs = %d\n", pdp->cArgs);
        ok(pdp->cNamedArgs == 1, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pdp->rgdispidNamedArgs[0] == DISPID_PROPERTYPUT, "pdp->rgdispidNamedArgs[0] = %ld\n", pdp->rgdispidNamedArgs[0]);
        ok(!pvarRes, "pvarRes != NULL\n");
        ok(pei != NULL, "pei == NULL\n");

        ok(V_VT(pdp->rgvarg) == VT_I2, "V_VT(psp->rgvargs) = %d\n", V_VT(pdp->rgvarg));
        ok(V_I2(pdp->rgvarg) == 1, "V_I2(psp->rgvargs) = %d\n", V_I2(pdp->rgvarg));
        return S_OK;

    case DISPID_TESTOBJ_KEYWORD:
        V_VT(pvarRes) = VT_I2;
        V_I2(pvarRes) = 10;
        return S_OK;
    }

    ok(0, "unexpected call %ld\n", id);
    return E_FAIL;
}

static IDispatchExVtbl testObjVtbl = {
    DispatchEx_QueryInterface,
    DispatchEx_AddRef,
    DispatchEx_Release,
    DispatchEx_GetTypeInfoCount,
    DispatchEx_GetTypeInfo,
    DispatchEx_GetIDsOfNames,
    DispatchEx_Invoke,
    testObj_GetDispID,
    testObj_InvokeEx,
    DispatchEx_DeleteMemberByName,
    DispatchEx_DeleteMemberByDispID,
    DispatchEx_GetMemberProperties,
    DispatchEx_GetMemberName,
    DispatchEx_GetNextDispID,
    DispatchEx_GetNameSpaceParent
};

static IDispatchEx testObj = { &testObjVtbl };

static HRESULT WINAPI enumDisp_QueryInterface(IDispatchEx *iface, REFIID riid, void **ppv)
{
    return IEnumVARIANT_QueryInterface(&enumObj, riid, ppv);
}

static IDispatchExVtbl enumDispVtbl = {
    enumDisp_QueryInterface,
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

static IDispatchEx enumDisp = { &enumDispVtbl };

static HRESULT WINAPI collectionObj_GetDispID(IDispatchEx *iface, BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    if(!lstrcmpW(bstrName, L"reset")) {
        *pid = DISPID_COLLOBJ_RESET;
        return S_OK;
    }

    ok(0, "unexpected call %s\n", wine_dbgstr_w(bstrName));
    return DISP_E_UNKNOWNNAME;
}

static HRESULT WINAPI collectionObj_InvokeEx(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    switch(id) {
    case DISPID_NEWENUM:
        if(strict_dispid_check)
            CHECK_EXPECT(collectionobj_newenum_i);

        ok(wFlags == (DISPATCH_PROPERTYGET|DISPATCH_METHOD), "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(!pdp->rgvarg, "rgvarg == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(!pdp->cArgs, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pvarRes != NULL, "pvarRes == NULL\n");
        ok(pei != NULL, "pei == NULL\n");

        V_VT(pvarRes) = VT_UNKNOWN;
        V_UNKNOWN(pvarRes) = (IUnknown*)&enumObj;
        return S_OK;
    case DISPID_COLLOBJ_RESET:
        next_cnt = 0;
        return S_OK;
    }

    ok(0, "unexpected call %ld\n", id);
    return E_NOTIMPL;
}

static IDispatchExVtbl collectionObjVtbl = {
    DispatchEx_QueryInterface,
    DispatchEx_AddRef,
    DispatchEx_Release,
    DispatchEx_GetTypeInfoCount,
    DispatchEx_GetTypeInfo,
    DispatchEx_GetIDsOfNames,
    DispatchEx_Invoke,
    collectionObj_GetDispID,
    collectionObj_InvokeEx,
    DispatchEx_DeleteMemberByName,
    DispatchEx_DeleteMemberByDispID,
    DispatchEx_GetMemberProperties,
    DispatchEx_GetMemberName,
    DispatchEx_GetNextDispID,
    DispatchEx_GetNameSpaceParent
};

static IDispatchEx collectionObj = { &collectionObjVtbl };

static ULONG refobj_ref;

static ULONG WINAPI RefObj_AddRef(IDispatchEx *iface)
{
    return ++refobj_ref;
}

static ULONG WINAPI RefObj_Release(IDispatchEx *iface)
{
    return --refobj_ref;
}

static IDispatchExVtbl RefObjVtbl = {
    DispatchEx_QueryInterface,
    RefObj_AddRef,
    RefObj_Release,
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

static IDispatchEx RefObj = { &RefObjVtbl };

static ULONG global_ref;

static ULONG WINAPI Global_AddRef(IDispatchEx *iface)
{
    return ++global_ref;
}

static ULONG WINAPI Global_Release(IDispatchEx *iface)
{
    return --global_ref;
}

static HRESULT WINAPI Global_GetDispID(IDispatchEx *iface, BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    static const dispid_t dispids[] = {
        { L"ok",              DISPID_GLOBAL_OK },
        { L"todo_wine_ok",    DISPID_GLOBAL_TODO_WINE_OK },
        { L"trace",           DISPID_GLOBAL_TRACE },
        { L"reportSuccess",   DISPID_GLOBAL_REPORTSUCCESS, REF_EXPECT(global_success_d) },
        { L"getVT",           DISPID_GLOBAL_GETVT },
        { L"isEnglishLang",   DISPID_GLOBAL_ISENGLANG },
        { L"MaxCharSize",     DISPID_GLOBAL_MAXCHARSIZE },
        { L"firstDayOfWeek",  DISPID_GLOBAL_WEEKSTARTDAY },
        { L"globalCallback",  DISPID_GLOBAL_GLOBALCALLBACK },
        { L"testObj",         DISPID_GLOBAL_TESTOBJ },
        { L"collectionObj" ,  DISPID_GLOBAL_COLLOBJ },
        { L"vbvar",           DISPID_GLOBAL_VBVAR, REF_EXPECT(global_vbvar_d) },
        { L"letobj",          DISPID_GLOBAL_LETOBJ },
        { L"setobj",          DISPID_GLOBAL_SETOBJ },
        { L"isNullDisp",      DISPID_GLOBAL_ISNULLDISP },
        { L"testDisp",        DISPID_GLOBAL_TESTDISP },
        { L"RefObj",          DISPID_GLOBAL_REFOBJ },
        { L"propargput",      DISPID_GLOBAL_PROPARGPUT, REF_EXPECT(global_propargput_d) },
        { L"propargput1",     DISPID_GLOBAL_PROPARGPUT1, REF_EXPECT(global_propargput1_d) },
        { L"propargset",      DISPID_GLOBAL_PROPARGSET, REF_EXPECT(global_propargset_d) },
        { L"counter",         DISPID_GLOBAL_COUNTER },
        { L"doubleAsString",  DISPID_GLOBAL_DOUBLEASSTRING },
        { L"testArray",       DISPID_GLOBAL_TESTARRAY },
        { L"throwException",  DISPID_GLOBAL_THROWEXCEPTION },
        { L"throwInt",        DISPID_GLOBAL_THROWINT },
        { L"testOptionalArg", DISPID_GLOBAL_TESTOPTIONALARG },
        { L"testErrorObject", DISPID_GLOBAL_TESTERROROBJECT },
        { L"throwWithDesc",   DISPID_GLOBAL_THROWWITHDESC },
        { L"unkObj",          DISPID_GLOBAL_UNKOBJ },
        { L"isArrayFixed",    DISPID_GLOBAL_ISARRAYFIXED },
    };

    test_grfdex(grfdex, fdexNameCaseInsensitive);
    if(get_dispid(bstrName, dispids, ARRAY_SIZE(dispids), pid))
        return S_OK;
    if(strict_dispid_check && wcscmp(bstrName, L"x"))
        ok(0, "unexpected call %s %lx\n", wine_dbgstr_w(bstrName), grfdex);
    return DISP_E_UNKNOWNNAME;
}

static HRESULT WINAPI Global_InvokeEx(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    switch(id) {
    case DISPID_GLOBAL_TODO_WINE_OK:
    case DISPID_GLOBAL_OK: {
        VARIANT *b;

        ok(wFlags == INVOKE_FUNC || wFlags == (INVOKE_FUNC|INVOKE_PROPERTYGET), "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(pdp->cArgs == 2, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        if(wFlags & INVOKE_PROPERTYGET)
            ok(pvarRes != NULL, "pvarRes == NULL\n");
        else
            ok(!pvarRes, "pvarRes != NULL\n");
        ok(pei != NULL, "pei == NULL\n");

        ok(V_VT(pdp->rgvarg) == VT_BSTR, "V_VT(psp->rgvargs) = %d\n", V_VT(pdp->rgvarg));

        b = pdp->rgvarg+1;
        if(V_VT(b) == (VT_BYREF|VT_VARIANT))
            b = V_BYREF(b);

        ok(V_VT(b) == VT_BOOL, "V_VT(b) = %d\n", V_VT(b));

        todo_wine_if(id == DISPID_GLOBAL_TODO_WINE_OK)
            ok(V_BOOL(b), "%s: %s\n", test_name, wine_dbgstr_w(V_BSTR(pdp->rgvarg)));
        return S_OK;
    }

    case DISPID_GLOBAL_TRACE:
        ok(wFlags == INVOKE_FUNC, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(pdp->cArgs == 1, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(!pvarRes, "pvarRes != NULL\n");
        ok(pei != NULL, "pei == NULL\n");

        ok(V_VT(pdp->rgvarg) == VT_BSTR, "V_VT(psp->rgvargs) = %d\n", V_VT(pdp->rgvarg));
        if(V_VT(pdp->rgvarg) == VT_BSTR)
            trace("%s: %s\n", test_name, wine_dbgstr_w(V_BSTR(pdp->rgvarg)));

        return S_OK;

    case DISPID_GLOBAL_REPORTSUCCESS:
        CHECK_EXPECT(global_success_i);

        ok(wFlags == INVOKE_FUNC, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(pdp->cArgs == 0, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(!pvarRes, "pvarRes != NULL\n");
        ok(pei != NULL, "pei == NULL\n");

        return S_OK;

    case DISPID_GLOBAL_GETVT:
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(pdp->cArgs == 1, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pvarRes != NULL, "pvarRes == NULL\n");
        ok(V_VT(pvarRes) ==  VT_EMPTY, "V_VT(pvarRes) = %d\n", V_VT(pvarRes));
        ok(pei != NULL, "pei == NULL\n");

        V_VT(pvarRes) = VT_BSTR;
        V_BSTR(pvarRes) = a2bstr(vt2a(pdp->rgvarg));
        return S_OK;

    case DISPID_GLOBAL_ISENGLANG:
        ok(wFlags == (INVOKE_FUNC|INVOKE_PROPERTYGET), "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(pdp->cArgs == 0, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pvarRes != NULL, "pvarRes == NULL\n");
        ok(pei != NULL, "pei == NULL\n");

        V_VT(pvarRes) = VT_BOOL;
        V_BOOL(pvarRes) = is_english ? VARIANT_TRUE : VARIANT_FALSE;
        return S_OK;

    case DISPID_GLOBAL_MAXCHARSIZE:
        V_VT(pvarRes) = VT_I4;
        V_I4(pvarRes) = MaxCharSize;
        return S_OK;

    case DISPID_GLOBAL_WEEKSTARTDAY:
        V_VT(pvarRes) = VT_I4;
        V_I4(pvarRes) = first_day_of_week;
        return S_OK;

    case DISPID_GLOBAL_VBVAR:
        CHECK_EXPECT(global_vbvar_i);

        ok(wFlags == DISPATCH_PROPERTYPUT, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(pdp->rgdispidNamedArgs != NULL, "rgdispidNamedArgs == NULL\n");
        ok(pdp->cArgs == 1, "cArgs = %d\n", pdp->cArgs);
        ok(pdp->cNamedArgs == 1, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pdp->rgdispidNamedArgs[0] == DISPID_PROPERTYPUT, "pdp->rgdispidNamedArgs[0] = %ld\n", pdp->rgdispidNamedArgs[0]);
        ok(!pvarRes, "pvarRes != NULL\n");
        ok(pei != NULL, "pei == NULL\n");

        ok(V_VT(pdp->rgvarg) == VT_I2, "V_VT(psp->rgvargs) = %d\n", V_VT(pdp->rgvarg));
        ok(V_I2(pdp->rgvarg) == 3, "V_I2(psp->rgvargs) = %d\n", V_I2(pdp->rgvarg));
        return S_OK;

    case DISPID_GLOBAL_LETOBJ:
        CHECK_EXPECT(global_letobj_i);

        ok(wFlags == DISPATCH_PROPERTYPUT, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(pdp->rgdispidNamedArgs != NULL, "rgdispidNamedArgs == NULL\n");
        ok(pdp->cArgs == 1, "cArgs = %d\n", pdp->cArgs);
        ok(pdp->cNamedArgs == 1, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pdp->rgdispidNamedArgs[0] == DISPID_PROPERTYPUT, "pdp->rgdispidNamedArgs[0] = %ld\n", pdp->rgdispidNamedArgs[0]);
        ok(!pvarRes, "pvarRes != NULL\n");
        ok(pei != NULL, "pei == NULL\n");

        ok(V_VT(pdp->rgvarg) == VT_DISPATCH, "V_VT(psp->rgvargs) = %d\n", V_VT(pdp->rgvarg));
        ok(V_DISPATCH(pdp->rgvarg) == (IDispatch*)&testObj, "V_DISPATCH(psp->rgvargs) != testObj\n");
        return S_OK;

    case DISPID_GLOBAL_SETOBJ:
        CHECK_EXPECT(global_setobj_i);

        ok(wFlags == DISPATCH_PROPERTYPUTREF, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(pdp->rgdispidNamedArgs != NULL, "rgdispidNamedArgs == NULL\n");
        ok(pdp->cArgs == 1, "cArgs = %d\n", pdp->cArgs);
        ok(pdp->cNamedArgs == 1, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pdp->rgdispidNamedArgs[0] == DISPID_PROPERTYPUT, "pdp->rgdispidNamedArgs[0] = %ld\n", pdp->rgdispidNamedArgs[0]);
        ok(!pvarRes, "pvarRes != NULL\n");
        ok(pei != NULL, "pei == NULL\n");

        ok(V_VT(pdp->rgvarg) == VT_DISPATCH, "V_VT(psp->rgvargs) = %d\n", V_VT(pdp->rgvarg));
        ok(V_DISPATCH(pdp->rgvarg) == (IDispatch*)&testObj, "V_DISPATCH(psp->rgvargs) != testObj\n");
        return S_OK;

    case DISPID_GLOBAL_TESTOBJ:
        ok(wFlags == (DISPATCH_PROPERTYGET|DISPATCH_METHOD), "wFlags = %x\n", wFlags);

        ok(pdp != NULL, "pdp == NULL\n");
        ok(!pdp->rgvarg, "rgvarg != NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(!pdp->cArgs, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pvarRes != NULL, "pvarRes == NULL\n");
        ok(pei != NULL, "pei == NULL\n");

        V_VT(pvarRes) = VT_DISPATCH;
        V_DISPATCH(pvarRes) = (IDispatch*)&testObj;
        return S_OK;

    case DISPID_GLOBAL_COLLOBJ:
        ok(wFlags == (DISPATCH_PROPERTYGET|DISPATCH_METHOD), "wFlags = %x\n", wFlags);

        ok(pdp != NULL, "pdp == NULL\n");
        ok(!pdp->rgvarg, "rgvarg != NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(!pdp->cArgs, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pvarRes != NULL, "pvarRes == NULL\n");
        ok(pei != NULL, "pei == NULL\n");

        V_VT(pvarRes) = VT_DISPATCH;
        V_DISPATCH(pvarRes) = (IDispatch*)&collectionObj;
        return S_OK;

    case DISPID_GLOBAL_REFOBJ:
        ok(wFlags == (DISPATCH_PROPERTYGET|DISPATCH_METHOD), "wFlags = %x\n", wFlags);

        ok(pdp != NULL, "pdp == NULL\n");
        ok(!pdp->rgvarg, "rgvarg == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(!pdp->cArgs, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pvarRes != NULL, "pvarRes == NULL\n");
        ok(pei != NULL, "pei == NULL\n");

        IDispatchEx_AddRef(&RefObj);
        V_VT(pvarRes) = VT_DISPATCH;
        V_DISPATCH(pvarRes) = (IDispatch*)&RefObj;
        return S_OK;

    case DISPID_GLOBAL_ISNULLDISP: {
        VARIANT *v;

        ok(wFlags == (INVOKE_FUNC|INVOKE_PROPERTYGET), "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(pdp->cArgs == 1, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pvarRes != NULL, "pvarRes == NULL\n");
        ok(pei != NULL, "pei == NULL\n");

        v = pdp->rgvarg;
        if(V_VT(v) == (VT_VARIANT|VT_BYREF))
            v = V_VARIANTREF(v);

        ok(V_VT(v) == VT_DISPATCH, "V_VT(psp->rgvargs) = %d\n", V_VT(pdp->rgvarg));
        V_VT(pvarRes) = VT_BOOL;
        V_BOOL(pvarRes) = V_DISPATCH(v) ? VARIANT_FALSE : VARIANT_TRUE;
        return S_OK;
    }

    case DISPID_GLOBAL_TESTDISP:
        ok(wFlags == INVOKE_FUNC, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(pdp->cArgs == 1, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(!pvarRes, "pvarRes != NULL\n");
        ok(pei != NULL, "pei == NULL\n");

        ok(V_VT(pdp->rgvarg) == VT_DISPATCH, "V_VT(psp->rgvargs) = %d\n", V_VT(pdp->rgvarg));
        test_disp(V_DISPATCH(pdp->rgvarg));
        return S_OK;

    case DISPID_GLOBAL_PROPARGPUT:
        CHECK_EXPECT(global_propargput_i);

        ok(wFlags == DISPATCH_PROPERTYPUT, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(pdp->rgdispidNamedArgs != NULL, "rgdispidNamedArgs == NULL\n");
        ok(pdp->cArgs == 3, "cArgs = %d\n", pdp->cArgs);
        ok(pdp->cNamedArgs == 1, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pdp->rgdispidNamedArgs[0] == DISPID_PROPERTYPUT, "pdp->rgdispidNamedArgs[0] = %ld\n", pdp->rgdispidNamedArgs[0]);
        ok(!pvarRes, "pvarRes != NULL\n");
        ok(pei != NULL, "pei == NULL\n");

        ok(V_VT(pdp->rgvarg) == VT_I2, "V_VT(psp->rgvargs) = %d\n", V_VT(pdp->rgvarg));
        ok(V_I2(pdp->rgvarg) == 0, "V_I2(psp->rgvargs) = %d\n", V_I2(pdp->rgvarg));

        ok(V_VT(pdp->rgvarg+1) == VT_I2, "V_VT(psp->rgvargs+1) = %d\n", V_VT(pdp->rgvarg+1));
        ok(V_I2(pdp->rgvarg+1) == 2, "V_I2(psp->rgvargs+1) = %d\n", V_I2(pdp->rgvarg+1));

        ok(V_VT(pdp->rgvarg+2) == VT_I2, "V_VT(psp->rgvargs+2) = %d\n", V_VT(pdp->rgvarg+2));
        ok(V_I2(pdp->rgvarg+2) == 1, "V_I2(psp->rgvargs+2) = %d\n", V_I2(pdp->rgvarg+2));
        return S_OK;

    case DISPID_GLOBAL_PROPARGPUT1:
        CHECK_EXPECT(global_propargput1_i);

        ok(wFlags == DISPATCH_PROPERTYPUT, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(pdp->rgdispidNamedArgs != NULL, "rgdispidNamedArgs == NULL\n");
        ok(pdp->cArgs == 2, "cArgs = %d\n", pdp->cArgs);
        ok(pdp->cNamedArgs == 1, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pdp->rgdispidNamedArgs[0] == DISPID_PROPERTYPUT, "pdp->rgdispidNamedArgs[0] = %ld\n", pdp->rgdispidNamedArgs[0]);
        ok(!pvarRes, "pvarRes != NULL\n");
        ok(pei != NULL, "pei == NULL\n");

        ok(V_VT(pdp->rgvarg) == VT_I2, "V_VT(psp->rgvargs) = %d\n", V_VT(pdp->rgvarg));
        ok(V_I2(pdp->rgvarg) == 0, "V_I2(psp->rgvargs) = %d\n", V_I2(pdp->rgvarg));

        ok(V_VT(pdp->rgvarg+1) == VT_I2, "V_VT(psp->rgvargs+1) = %d\n", V_VT(pdp->rgvarg+1));
        ok(V_I2(pdp->rgvarg+1) == 1, "V_I2(psp->rgvargs+1) = %d\n", V_I2(pdp->rgvarg+1));

        return S_OK;

    case DISPID_GLOBAL_PROPARGSET:
        CHECK_EXPECT(global_propargset_i);

        ok(wFlags == DISPATCH_PROPERTYPUTREF, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(pdp->rgdispidNamedArgs != NULL, "rgdispidNamedArgs == NULL\n");
        ok(pdp->cArgs == 2, "cArgs = %d\n", pdp->cArgs);
        ok(pdp->cNamedArgs == 1, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pdp->rgdispidNamedArgs[0] == DISPID_PROPERTYPUT, "pdp->rgdispidNamedArgs[0] = %ld\n", pdp->rgdispidNamedArgs[0]);
        ok(!pvarRes, "pvarRes != NULL\n");
        ok(pei != NULL, "pei == NULL\n");

        ok(V_VT(pdp->rgvarg) == VT_DISPATCH, "V_VT(psp->rgvargs) = %d\n", V_VT(pdp->rgvarg));
        ok(V_DISPATCH(pdp->rgvarg) == (IDispatch*)&testObj, "V_DISPATCH(psp->rgvargs) = %p\n", V_DISPATCH(pdp->rgvarg));

        ok(V_VT(pdp->rgvarg+1) == VT_I2, "V_VT(psp->rgvargs+1) = %d\n", V_VT(pdp->rgvarg+1));
        ok(V_I2(pdp->rgvarg+1) == 0, "V_I2(psp->rgvargs+1) = %d\n", V_I2(pdp->rgvarg+1));

        return S_OK;

    case DISPID_GLOBAL_COUNTER:
        ok(pdp != NULL, "pdp == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(!pdp->cArgs, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pvarRes != NULL, "pvarRes == NULL\n");
        ok(V_VT(pvarRes) ==  VT_EMPTY, "V_VT(pvarRes) = %d\n", V_VT(pvarRes));
        ok(pei != NULL, "pei == NULL\n");

        V_VT(pvarRes) = VT_I2;
        V_I2(pvarRes) = test_counter++;
        return S_OK;

    case DISPID_GLOBAL_DOUBLEASSTRING:
        ok(wFlags == (INVOKE_FUNC|INVOKE_PROPERTYGET), "wFlags = %x\n", wFlags);
        ok(pdp->cArgs == 1, "cArgs = %d\n", pdp->cArgs);
        ok(V_VT(pdp->rgvarg) == VT_R8, "V_VT(pdp->rgvarg) = %d\n", V_VT(pdp->rgvarg));
        ok(pvarRes != NULL, "pvarRes == NULL\n");

        V_VT(pvarRes) = VT_BSTR;
        return VarBstrFromR8(V_R8(pdp->rgvarg), 0, 0, &V_BSTR(pvarRes));

    case DISPID_GLOBAL_TESTARRAY:
        ok(wFlags == INVOKE_FUNC, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(pdp->cArgs == 2, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(!pvarRes, "pvarRes != NULL\n");
        ok(pei != NULL, "pei == NULL\n");

        ok(V_VT(pdp->rgvarg+1) == VT_I2, "V_VT(psp->rgvargs+1) = %d\n", V_VT(pdp->rgvarg+1));
        ok(V_VT(pdp->rgvarg) == (VT_BYREF|VT_VARIANT), "V_VT(psp->rgvargs) = %d\n", V_VT(pdp->rgvarg));
        ok(V_VT(V_VARIANTREF(pdp->rgvarg)) == (VT_ARRAY|VT_BYREF|VT_VARIANT),
           "V_VT(V_VARIANTREF(psp->rgvargs)) = %d\n", V_VT(V_VARIANTREF(pdp->rgvarg)));
        if(V_I2(pdp->rgvarg+1) == -1)
            ok(!*V_ARRAYREF(V_VARIANTREF(pdp->rgvarg)), "*V_ARRAYREF(V_VARIANTREF(pdp->rgvarg)) != NULL\n");
        else
            test_safearray(*V_ARRAYREF(V_VARIANTREF(pdp->rgvarg)), V_I2(pdp->rgvarg+1));
        return S_OK;

    case DISPID_GLOBAL_THROWINT: {
        VARIANT *v = pdp->rgvarg;
        HRESULT hres;

        ok((wFlags & ~INVOKE_PROPERTYGET) == INVOKE_FUNC, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(pdp->cArgs == 1, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pei != NULL, "pei == NULL\n");
        if(pvarRes) {
            ok(V_VT(pvarRes) == VT_EMPTY, "V_VT(pvarRes) = %d\n", V_VT(pvarRes));
            V_VT(pvarRes) = VT_BOOL;
            V_BOOL(pvarRes) = VARIANT_FALSE;
        }

        if(V_VT(v) == (VT_VARIANT|VT_BYREF))
            v = V_VARIANTREF(v);

        switch(V_VT(v)) {
        case VT_I2:
            hres = V_I2(v);
            break;
        case VT_I4:
            hres = V_I4(v);
            break;
        default:
            ok(0, "unexpected vt %d\n", V_VT(v));
            return E_INVALIDARG;
        }

        return hres;

    case DISPID_GLOBAL_THROWEXCEPTION: {
        VARIANT *v = pdp->rgvarg + pdp->cArgs - 1;

        ok((wFlags & ~INVOKE_PROPERTYGET) == INVOKE_FUNC, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(pdp->cArgs >= 1, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pei != NULL, "pei == NULL\n");

        if(pvarRes) {
            ok(V_VT(pvarRes) == VT_EMPTY, "V_VT(pvarRes) = %d\n", V_VT(pvarRes));
            V_VT(pvarRes) = VT_BOOL;
            V_BOOL(pvarRes) = VARIANT_FALSE;
        }

        if(V_VT(v) == (VT_VARIANT|VT_BYREF))
            v = V_VARIANTREF(v);

        memset(pei, 0, sizeof(*pei));
        switch(V_VT(v)) {
        case VT_I2:
            pei->scode = V_I2(v);
            break;
        case VT_I4:
            pei->scode = V_I4(v);
            break;
        default:
            ok(0, "unexpected vt %d\n", V_VT(v));
            return E_INVALIDARG;
        }

        if(pdp->cArgs >= 2) {
            v = pdp->rgvarg + pdp->cArgs - 2;
            if(!(V_VT(v) == VT_ERROR && V_ERROR(v) == DISP_E_PARAMNOTFOUND)) /* != vtMissing */
            {
                ok(V_VT(v) == VT_BSTR, "v = %s\n", debugstr_variant(v));
                pei->bstrSource = SysAllocString(V_BSTR(v));
            }
        }

        if(pdp->cArgs >= 3) {
            v = pdp->rgvarg + pdp->cArgs - 3;
            ok(V_VT(v) == VT_BSTR, "v = %s\n", debugstr_variant(v));
            pei->bstrDescription = SysAllocString(V_BSTR(v));
        }

        return DISP_E_EXCEPTION;
    }

    case DISPID_GLOBAL_THROWWITHDESC:
        pei->scode = 0xdeadbeef;
        pei->bstrDescription = SysAllocString(L"test");
        pei->bstrHelpFile = SysAllocString(L"test.chm");
        pei->dwHelpContext = 10;
        return DISP_E_EXCEPTION;
    }

    case DISPID_GLOBAL_TESTOPTIONALARG: {
        VARIANT *v;
        int opt;

        CHECK_EXPECT(global_testoptionalarg_i);

        ok(wFlags == INVOKE_FUNC, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(pdp->cArgs == 3, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(!pvarRes, "pvarRes != NULL\n");
        ok(pei != NULL, "pei == NULL\n");

        ok(V_VT(pdp->rgvarg) == VT_I2, "V_VT(pdp->rgvarg) = %d\n", V_VT(pdp->rgvarg));
        opt = V_I2(pdp->rgvarg);
        ok(opt == 1 || opt == 2, "opt = %d\n", opt);
        v = pdp->rgvarg+pdp->cArgs-opt;
        ok(V_VT(v) == VT_ERROR, "V_VT(v) = %d\n", V_VT(v));
        ok(V_ERROR(v) == DISP_E_PARAMNOTFOUND, "V_ERROR(v) = %08lx\n", V_ERROR(v));
        return S_OK;
    }
    case DISPID_GLOBAL_GLOBALCALLBACK: {
        DISPPARAMS dp = {0};
        IDispatchEx *dispex;
        EXCEPINFO ei;
        VARIANT v;
        DISPID id;
        BSTR str;
        HRESULT hres;

        CHECK_EXPECT(global_globalcallback_i);
        CHECK_CALLED(OnEnterScript);

        ok(wFlags == INVOKE_FUNC, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(V_VT(pdp->rgvarg) == VT_DISPATCH, "V_VT(pdp->rgvarg) = %d\n", V_VT(pdp->rgvarg));
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(pdp->cArgs == 1, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(!pvarRes, "pvarRes != NULL\n");
        ok(pei != NULL, "pei == NULL\n");

        hres = IDispatch_QueryInterface(V_DISPATCH(pdp->rgvarg), &IID_IDispatchEx, (void**)&dispex);
        ok(hres == S_OK, "Could not get IDispatchEx iface: %08lx\n", hres);

        str = SysAllocString(L"callback");
        hres = IDispatchEx_GetDispID(dispex, str, fdexNameCaseInsensitive, &id);
        ok(hres == S_OK, "GetDispID failed: %08lx\n", hres);
        SysFreeString(str);

        SET_EXPECT(OnEnterScript);
        SET_EXPECT(OnLeaveScript);
        memset(&ei, 0, sizeof(ei));
        hres = IDispatchEx_InvokeEx(dispex, id, 0, DISPATCH_METHOD, &dp, &v, &ei, pspCaller);
        ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
        CHECK_CALLED(OnEnterScript);
        CHECK_CALLED(OnLeaveScript);
        ok(V_VT(&v) == VT_BOOL, "V_VT(v) = %d\n", V_VT(&v));
        ok(V_BOOL(&v) == VARIANT_TRUE, "V_BOOL(v) = %x\n", V_BOOL(&v));

        IDispatchEx_Release(dispex);

        SET_EXPECT(OnLeaveScript);
        return S_OK;
    }
    case DISPID_GLOBAL_TESTERROROBJECT: {
        ITypeInfo *typeinfo;
        IDispatchEx *dispex;
        DISPPARAMS dp;
        VARIANT v, r;
        EXCEPINFO ei;
        IDispatch *disp;
        WCHAR *names[1];
        UINT count, i;
        DISPID id;
        HRESULT hres;

        static WCHAR props[][32] = { L"clear", L"description", L"helpcontext", L"helpFILE", L"number", L"raise", L"source" };

        CHECK_EXPECT(global_testerrorobject_i);

        ok(wFlags == INVOKE_FUNC, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(pdp->cArgs == 1, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(!pvarRes, "pvarRes != NULL\n");
        ok(pei != NULL, "pei == NULL\n");

        ok(V_VT(pdp->rgvarg) == VT_DISPATCH, "V_VT(pdp->rgvarg) = %d\n", V_VT(pdp->rgvarg));
        disp = V_DISPATCH(pdp->rgvarg);
        hres = IDispatch_QueryInterface(disp, &IID_IDispatchEx, (void**)&dispex);
        ok(hres == E_NOINTERFACE, "Could not get IDispatchEx iface: %08lx\n", hres);

        hres = IDispatch_GetTypeInfoCount(disp, &count);
        ok(hres == S_OK, "GetTypeInfoCount returned: %08lx\n", hres);
        ok(count == 0, "count = %u\n", count);

        hres = IDispatch_GetTypeInfo(disp, 0, 0, &typeinfo);
        ok(hres == DISP_E_BADINDEX, "GetTypeInfo returned: %08lx\n", hres);

        for(i = 0; i < ARRAY_SIZE(props); i++) {
            names[0] = props[i];
            hres = IDispatch_GetIDsOfNames(disp, &IID_NULL, names, 1, 0, &id);
            ok(hres == S_OK, "GetIDsOfNames failed: %08lx\n", hres);
            ok(id == i + 1, "%s id = %lu\n", wine_dbgstr_w(props[i]), id);
        }

        memset(&dp, 0, sizeof(dp));
        memset(&ei, 0, sizeof(ei));
        V_VT(&v) = VT_ERROR;
        hres = IDispatch_Invoke(disp, 5, &IID_NULL, 0, DISPATCH_PROPERTYGET, &dp, &v, &ei, NULL);
        ok(hres == S_OK, "Invoke failed: %08lx\n", hres);
        ok(V_VT(&v) == VT_I4, "V_VT(v) = %d\n", V_VT(&v));
        ok(V_I4(&v) == 1, "V_I4(v) = %ld\n", V_I4(&v));
        hres = IDispatch_Invoke(disp, DISPID_VALUE, &IID_NULL, 0, DISPATCH_PROPERTYGET, &dp, &v, &ei, NULL);
        ok(hres == S_OK, "Invoke failed: %08lx\n", hres);
        ok(V_VT(&v) == VT_I4, "V_VT(v) = %d\n", V_VT(&v));
        ok(V_I4(&v) == 1, "V_I4(v) = %ld\n", V_I4(&v));

        dp.rgvarg = &v;
        V_VT(&v) = VT_I4;
        V_I4(&v) = 6;
        V_VT(&r) = VT_EMPTY;
        hres = IDispatch_Invoke(disp, DISPID_VALUE, &IID_NULL, 6, DISPATCH_METHOD|DISPATCH_PROPERTYGET, &dp, &r, &ei, NULL);
        ok(hres == S_OK, "Invoke failed: %08lx\n", hres);
        return S_OK;
    }
    case DISPID_GLOBAL_UNKOBJ:
        V_VT(pvarRes) = VT_UNKNOWN;
        V_UNKNOWN(pvarRes) = &unkObj;
        return S_OK;

    case DISPID_GLOBAL_ISARRAYFIXED:
    {
        BOOL is_fixed = FALSE;
        VARIANT *v;

        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(pdp->cArgs == 1, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pvarRes != NULL, "pvarRes == NULL\n");
        ok(V_VT(pvarRes) == VT_EMPTY, "V_VT(pvarRes) = %d\n", V_VT(pvarRes));
        ok(pei != NULL, "pei == NULL\n");

        ok(V_VT(pdp->rgvarg) == (VT_VARIANT|VT_BYREF), "Unexpected argument type %d.\n", V_VT(pdp->rgvarg));
        v = V_VARIANTREF(pdp->rgvarg);
        ok(V_VT(v) == (VT_VARIANT|VT_ARRAY|VT_BYREF), "Unexpected argument type %d.\n", V_VT(v));
        if (V_ISARRAY(v))
        {
            SAFEARRAY *sa = V_ISBYREF(v) ? *V_ARRAYREF(v) : V_ARRAY(v);
            is_fixed = (sa->fFeatures & (FADF_FIXEDSIZE | FADF_STATIC)) == (FADF_FIXEDSIZE | FADF_STATIC);
        }

        V_VT(pvarRes) = VT_BOOL;
        V_BOOL(pvarRes) = is_fixed ? VARIANT_TRUE : VARIANT_FALSE;
        return S_OK;
    }
    }

    ok(0, "unexpected call %ld\n", id);
    return DISP_E_MEMBERNOTFOUND;
}

static IDispatchExVtbl GlobalVtbl = {
    DispatchEx_QueryInterface,
    Global_AddRef,
    Global_Release,
    DispatchEx_GetTypeInfoCount,
    DispatchEx_GetTypeInfo,
    DispatchEx_GetIDsOfNames,
    DispatchEx_Invoke,
    Global_GetDispID,
    Global_InvokeEx,
    DispatchEx_DeleteMemberByName,
    DispatchEx_DeleteMemberByDispID,
    DispatchEx_GetMemberProperties,
    DispatchEx_GetMemberName,
    DispatchEx_GetNextDispID,
    DispatchEx_GetNameSpaceParent
};

static IDispatchEx Global = { &GlobalVtbl };

static HRESULT WINAPI ActiveScriptSiteWindow_QueryInterface(IActiveScriptSiteWindow *iface, REFIID riid, void **ppv)
{
    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI ActiveScriptSiteWindow_AddRef(IActiveScriptSiteWindow *iface)
{
    return 2;
}

static ULONG WINAPI ActiveScriptSiteWindow_Release(IActiveScriptSiteWindow *iface)
{
    return 1;
}

static HRESULT WINAPI ActiveScriptSiteWindow_GetWindow(IActiveScriptSiteWindow *iface, HWND *phwnd)
{
    if(!allow_ui)
        CHECK_EXPECT(GetWindow);
    *phwnd = NULL;
    return S_OK;
}

static HRESULT WINAPI ActiveScriptSiteWindow_EnableModeless(IActiveScriptSiteWindow *iface, BOOL fEnable)
{
    if(allow_ui)
        return S_OK;

    CHECK_EXPECT(EnableModeless);
    ok(!fEnable, "fEnable = %x\n", fEnable);
    return E_FAIL;
}

static const IActiveScriptSiteWindowVtbl ActiveScriptSiteWindowVtbl = {
    ActiveScriptSiteWindow_QueryInterface,
    ActiveScriptSiteWindow_AddRef,
    ActiveScriptSiteWindow_Release,
    ActiveScriptSiteWindow_GetWindow,
    ActiveScriptSiteWindow_EnableModeless
};

static IActiveScriptSiteWindow ActiveScriptSiteWindow = { &ActiveScriptSiteWindowVtbl };

static HRESULT WINAPI ActiveScriptSiteUIControl_QueryInterface(IActiveScriptSiteUIControl *iface, REFIID riid, void **ppv)
{
    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI ActiveScriptSiteUIControl_AddRef(IActiveScriptSiteUIControl *iface)
{
    return 2;
}

static ULONG WINAPI ActiveScriptSiteUIControl_Release(IActiveScriptSiteUIControl *iface)
{
    return 1;
}

static HRESULT WINAPI ActiveScriptSiteUIControl_GetUIBehavior(IActiveScriptSiteUIControl *iface, SCRIPTUICITEM UicItem,
        SCRIPTUICHANDLING *pUicHandling)
{
    if(!allow_ui) {
        CHECK_EXPECT(GetUIBehavior);
        ok(UicItem == SCRIPTUICITEM_MSGBOX, "UidItem = %d\n", UicItem);
    }
    *pUicHandling = uic_handling;
    return S_OK;
}

static const IActiveScriptSiteUIControlVtbl ActiveScriptSiteUIControlVtbl = {
    ActiveScriptSiteUIControl_QueryInterface,
    ActiveScriptSiteUIControl_AddRef,
    ActiveScriptSiteUIControl_Release,
    ActiveScriptSiteUIControl_GetUIBehavior
};

static IActiveScriptSiteUIControl ActiveScriptSiteUIControl = { &ActiveScriptSiteUIControlVtbl };

static HRESULT WINAPI ActiveScriptSite_QueryInterface(IActiveScriptSite *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid))
        *ppv = iface;
    else if(IsEqualGUID(&IID_IActiveScriptSite, riid))
        *ppv = iface;
    else if(IsEqualGUID(&IID_IActiveScriptSiteWindow, riid))
        *ppv = &ActiveScriptSiteWindow;
    else if(IsEqualGUID(&IID_IActiveScriptSiteUIControl, riid))
        *ppv = &ActiveScriptSiteUIControl;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI ActiveScriptSite_AddRef(IActiveScriptSite *iface)
{
    return 2;
}

static ULONG WINAPI ActiveScriptSite_Release(IActiveScriptSite *iface)
{
    return 1;
}

static HRESULT WINAPI ActiveScriptSite_GetLCID(IActiveScriptSite *iface, LCID *plcid)
{
    *plcid = GetUserDefaultLCID();
    return S_OK;
}

static HRESULT WINAPI ActiveScriptSite_GetItemInfo(IActiveScriptSite *iface, LPCOLESTR pstrName,
        DWORD dwReturnMask, IUnknown **ppiunkItem, ITypeInfo **ppti)
{
    ok(dwReturnMask == SCRIPTINFO_IUNKNOWN, "unexpected dwReturnMask %lx\n", dwReturnMask);
    ok(!ppti, "ppti != NULL\n");

    if(lstrcmpW(pstrName, L"test"))
        ok(0, "unexpected pstrName %s\n", wine_dbgstr_w(pstrName));

    *ppiunkItem = (IUnknown*)&Global;
    IUnknown_AddRef(*ppiunkItem);
    return S_OK;
}

static HRESULT WINAPI ActiveScriptSite_GetDocVersionString(IActiveScriptSite *iface, BSTR *pbstrVersion)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSite_OnScriptTerminate(IActiveScriptSite *iface,
        const VARIANT *pvarResult, const EXCEPINFO *pexcepinfo)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSite_OnStateChange(IActiveScriptSite *iface, SCRIPTSTATE ssScriptState)
{
    return E_NOTIMPL;
}

static IActiveScriptError **store_script_error;
static ULONG error_line;
static LONG error_char;

static HRESULT WINAPI ActiveScriptSite_OnScriptError(IActiveScriptSite *iface, IActiveScriptError *pscripterror)
{
    HRESULT hr = onerror_hres, hres;

    hres = IActiveScriptError_GetSourcePosition(pscripterror, NULL, &error_line, &error_char);
    ok(hres == S_OK, "GetSourcePosition failed: %08lx\n", hres);

    if(!expect_OnScriptError) {
        EXCEPINFO info;

        hres = IActiveScriptError_GetExceptionInfo(pscripterror, &info);
        if(SUCCEEDED(hres))
            trace("Error in line %lu: %x %s\n", error_line + 1, info.wCode, wine_dbgstr_w(info.bstrDescription));
    }else {
        IDispatchEx *dispex;

        hres = IActiveScriptError_QueryInterface(pscripterror, &IID_IDispatchEx, (void**)&dispex);
        ok(hres == E_NOINTERFACE, "QI(IDispatchEx) returned: %08lx\n", hres);
    }

    if(store_script_error) {
        IActiveScriptError_AddRef(pscripterror);
        *store_script_error = pscripterror;
        store_script_error = NULL;
    }

    CHECK_EXPECT(OnScriptError);
    onerror_hres = E_NOTIMPL;

    return hr;
}

static HRESULT WINAPI ActiveScriptSite_OnEnterScript(IActiveScriptSite *iface)
{
    if(strict_enter_script)
        CHECK_EXPECT(OnEnterScript);
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSite_OnLeaveScript(IActiveScriptSite *iface)
{
    if(strict_enter_script)
        CHECK_EXPECT(OnLeaveScript);
    return E_NOTIMPL;
}

static const IActiveScriptSiteVtbl ActiveScriptSiteVtbl = {
    ActiveScriptSite_QueryInterface,
    ActiveScriptSite_AddRef,
    ActiveScriptSite_Release,
    ActiveScriptSite_GetLCID,
    ActiveScriptSite_GetItemInfo,
    ActiveScriptSite_GetDocVersionString,
    ActiveScriptSite_OnScriptTerminate,
    ActiveScriptSite_OnStateChange,
    ActiveScriptSite_OnScriptError,
    ActiveScriptSite_OnEnterScript,
    ActiveScriptSite_OnLeaveScript
};

static IActiveScriptSite ActiveScriptSite = { &ActiveScriptSiteVtbl };

static IActiveScript *create_script(void)
{
    IActiveScript *script;
    HRESULT hres;

    hres = CoCreateInstance(&CLSID_VBScript, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IActiveScript, (void**)&script);
    ok(hres == S_OK, "CoCreateInstance failed: %08lx\n", hres);

    return script;
}

static IActiveScript *create_and_init_script(DWORD flags, BOOL start)
{
    IActiveScriptParse *parser;
    IActiveScript *engine;
    HRESULT hres;

    engine = create_script();
    if(!engine)
        return NULL;

    hres = IActiveScript_QueryInterface(engine, &IID_IActiveScriptParse, (void**)&parser);
    ok(hres == S_OK, "Could not get IActiveScriptParse: %08lx\n", hres);

    hres = IActiveScriptParse_InitNew(parser);
    ok(hres == S_OK, "InitNew failed: %08lx\n", hres);

    IActiveScriptParse_Release(parser);

    hres = IActiveScript_SetScriptSite(engine, &ActiveScriptSite);
    ok(hres == S_OK, "SetScriptSite failed: %08lx\n", hres);

    hres = IActiveScript_AddNamedItem(engine, L"test",
            SCRIPTITEM_ISVISIBLE|SCRIPTITEM_ISSOURCE|flags);
    ok(hres == S_OK, "AddNamedItem failed: %08lx\n", hres);

    if (start)
    {
        hres = IActiveScript_SetScriptState(engine, SCRIPTSTATE_STARTED);
        ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_STARTED) failed: %08lx\n", hres);
    }

    return engine;
}

static void close_script(IActiveScript *script)
{
    ULONG ref;
    HRESULT hres;

    hres = IActiveScript_Close(script);
    ok(hres == S_OK, "Close failed: %08lx\n", hres);

    ref = IActiveScript_Release(script);
    ok(!ref, "ref=%lu\n", ref);
}

static HRESULT parse_script(DWORD flags, BSTR script_str, const WCHAR *delim)
{
    IActiveScriptParse *parser;
    IActiveScript *engine;
    IDispatch *script_disp;
    HRESULT hres;

    engine = create_and_init_script(flags, TRUE);
    if(!engine)
        return S_OK;

    hres = IActiveScript_QueryInterface(engine, &IID_IActiveScriptParse, (void**)&parser);
    ok(hres == S_OK, "Could not get IActiveScriptParse: %08lx\n", hres);
    if (FAILED(hres))
    {
        IActiveScript_Release(engine);
        return hres;
    }

    hres = IActiveScript_GetScriptDispatch(engine, NULL, &script_disp);
    ok(hres == S_OK, "GetScriptDispatch failed: %08lx\n", hres);
    ok(script_disp != NULL, "script_disp == NULL\n");
    ok(script_disp != (IDispatch*)&Global, "script_disp == Global\n");

    test_counter = 0;

    hres = IActiveScriptParse_ParseScriptText(parser, script_str, NULL, NULL, delim, 0, 0, 0, NULL, NULL);

    IDispatch_Release(script_disp);
    IActiveScriptParse_Release(parser);
    close_script(engine);

    return hres;
}

static void parse_script_wf(DWORD flags, const WCHAR *src)
{
    BSTR tmp;
    HRESULT hres;

    tmp = SysAllocString(src);
    hres = parse_script(flags, tmp, NULL);
    SysFreeString(tmp);
    ok(hres == S_OK, "parse_script failed: %08lx\n", hres);
}

static HRESULT parse_script_wr(const WCHAR *src)
{
    BSTR tmp;
    HRESULT hres;

    tmp = SysAllocString(src);
    hres = parse_script(SCRIPTITEM_GLOBALMEMBERS, tmp, NULL);
    SysFreeString(tmp);
    return hres;
}

static void test_parse_context(void)
{
    IActiveScriptParse *parser;
    IActiveScript *engine;
    BSTR str;
    HRESULT hres;

    global_ref = 1;
    engine = create_and_init_script(0, TRUE);
    if(!engine)
        return;

    hres = IActiveScript_QueryInterface(engine, &IID_IActiveScriptParse, (void**)&parser);
    ok(hres == S_OK, "Could not get IActiveScriptParse: %08lx\n", hres);

    /* NULL code text succeeds but does nothing */
    hres = IActiveScriptParse_ParseScriptText(parser, NULL, NULL, NULL, NULL, 0, 0, 0, NULL, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);

    /* unknown identifier context is not a valid argument */
    str = SysAllocString(L"Call reportSuccess()\n");
    hres = IActiveScriptParse_ParseScriptText(parser, str, L"y", NULL, NULL, 0, 0, 0, NULL, NULL);
    ok(hres == E_INVALIDARG, "ParseScriptText failed: %08lx\n", hres);
    SysFreeString(str);

    str = SysAllocString(L"class Cl\n"
                         "    Public Sub ClMethod\n"
                         "        Call reportSuccess()\n"
                         "    End Sub\n"
                         "End Class\n"
                         "Dim x\n"
                         "set x = new Cl\n");
    hres = IActiveScriptParse_ParseScriptText(parser, str, NULL, NULL, NULL, 0, 0, 0, NULL, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);
    SysFreeString(str);

    /* known global variable is not a valid context */
    str = SysAllocString(L"Call reportSuccess()\n");
    hres = IActiveScriptParse_ParseScriptText(parser, str, L"x", NULL, NULL, 0, 0, 0, NULL, NULL);
    ok(hres == E_INVALIDARG, "ParseScriptText failed: %08lx\n", hres);
    SysFreeString(str);

    SET_EXPECT(global_success_d);
    SET_EXPECT(global_success_i);
    str = SysAllocString(L"Call reportSuccess()\n");
    hres = IActiveScriptParse_ParseScriptText(parser, str, L"test", NULL, NULL, 0, 0, 0, NULL, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);
    SysFreeString(str);
    CHECK_CALLED(global_success_d);
    CHECK_CALLED(global_success_i);

    IActiveScriptParse_Release(parser);
    close_script(engine);
    ok(global_ref == 1, "global_ref = %lu\n", global_ref);
}

static void parse_script_w(const WCHAR *src)
{
    parse_script_wf(SCRIPTITEM_GLOBALMEMBERS, src);
}

#define parse_htmlscript(a) _parse_htmlscript(__LINE__,a)
static void _parse_htmlscript(unsigned line, const WCHAR *src)
{
    BSTR tmp;
    HRESULT hres;

    tmp = SysAllocString(src);
    hres = parse_script(SCRIPTITEM_GLOBALMEMBERS, tmp, L"</SCRIPT>");
    SysFreeString(tmp);
    ok_(__FILE__,line)(hres == S_OK, "parse_script failed: %08lx\n", hres);
}

static IDispatchEx *parse_procedure(IActiveScriptParseProcedure2 *parse_proc, const WCHAR *src, DWORD flags)
{
    IDispatchEx *dispex;
    IDispatch *disp;
    BSTR str;
    HRESULT hres;

    str = SysAllocString(src);
    hres = IActiveScriptParseProcedure2_ParseProcedureText(parse_proc, str, NULL, L"", NULL, NULL, L"\"", 0, 0,
            SCRIPTPROC_HOSTMANAGESSOURCE|SCRIPTPROC_IMPLICIT_THIS|SCRIPTPROC_IMPLICIT_PARENTS|flags, &disp);
    SysFreeString(str);
    ok(hres == S_OK, "ParseProcedureText failed: %08lx\n", hres);
    ok(disp != NULL, "disp = NULL\n");

    hres = IDispatch_QueryInterface(disp, &IID_IDispatchEx, (void**)&dispex);
    IDispatch_Release(disp);
    ok(hres == S_OK, "Could not get IDispatchEx iface: %08lx\n", hres);

    return dispex;
}


static void test_procedures(void)
{
    IActiveScriptParseProcedure2 *parse_proc;
    DISPPARAMS dp = {NULL};
    IActiveScript *script;
    IDispatchEx *proc;
    IDispatch *disp;
    EXCEPINFO ei = {0};
    VARIANT v;
    HRESULT hres;

    strict_enter_script = TRUE;
    script = create_and_init_script(0, TRUE);

    hres = IActiveScript_QueryInterface(script, &IID_IActiveScriptParseProcedure2, (void**)&parse_proc);
    ok(hres == S_OK, "Could not get IActiveScriptParseProcedure2 iface: %08lx\n", hres);

    hres = IActiveScriptParseProcedure2_ParseProcedureText(parse_proc, NULL, NULL, L"", NULL, NULL, NULL, 0, 0, 0, &disp);
    ok(hres == S_OK, "ParseProcedureText failed: %08lx\n", hres);
    IDispatch_Release(disp);

    proc = parse_procedure(parse_proc, L"dim x\nif true then x=false", 0);

    SET_EXPECT(OnEnterScript);
    SET_EXPECT(OnLeaveScript);
    V_VT(&v) = VT_EMPTY;
    hres = IDispatchEx_InvokeEx(proc, DISPID_VALUE, 0, DISPATCH_METHOD, &dp, &v, &ei, &caller_sp);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    CHECK_CALLED(OnEnterScript);
    CHECK_CALLED(OnLeaveScript);
    VariantClear(&v);
    IDispatchEx_Release(proc);

    proc = parse_procedure(parse_proc, L"\"foobar\"", SCRIPTPROC_ISEXPRESSION);

    SET_EXPECT(OnEnterScript);
    SET_EXPECT(OnLeaveScript);
    hres = IDispatchEx_InvokeEx(proc, DISPID_VALUE, 0, DISPATCH_METHOD, &dp, &v, &ei, &caller_sp);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    CHECK_CALLED(OnEnterScript);
    CHECK_CALLED(OnLeaveScript);
    ok(V_VT(&v) == VT_BSTR, "Expected VT_BSTR, got %s\n", vt2a(&v));
    ok(!lstrcmpW(V_BSTR(&v), L"foobar"), "Wrong string, got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    IDispatchEx_Release(proc);

    IActiveScriptParseProcedure2_Release(parse_proc);

    close_script(script);
    strict_enter_script = FALSE;
}

static void free_ei(EXCEPINFO *ei)
{
    SysFreeString(ei->bstrSource);
    SysFreeString(ei->bstrDescription);
    SysFreeString(ei->bstrHelpFile);
}

static void test_callbacks(void)
{
    IActiveScriptError *error1, *error2;
    IActiveScriptParse *parser;
    DISPPARAMS dp = {NULL};
    IActiveScript *script;
    IDispatchEx *dispex;
    IDispatch *disp;
    DISPID id;
    EXCEPINFO ei = {0};
    BSTR str;
    VARIANT v;
    HRESULT hres;

    strict_enter_script = TRUE;
    script = create_and_init_script(SCRIPTITEM_GLOBALMEMBERS, TRUE);

    hres = IActiveScript_QueryInterface(script, &IID_IActiveScriptParse, (void**)&parser);
    ok(hres == S_OK, "Could not get IActiveScriptParseProcedure2 iface: %08lx\n", hres);

    SET_EXPECT(OnEnterScript);
    SET_EXPECT(OnLeaveScript);
    hres = IActiveScriptParse_ParseScriptText(parser,
                                              L"class CallbackClass\n"
                                              L"    public function callback()\n"
                                              L"        call ok(err.number = 2, \"err.number = \" & err.number)\n"
                                              L"        callback = true\n"
                                              L"    end function\n"
                                              L"end class\n"
                                              L"function callGlobalCallback()\n"
                                              L"    on error resume next\n"
                                              L"    err.raise 2\n"
                                              L"    call test.globalCallback(new CallbackClass)\n"
                                              L"    call ok(err.number = 2, \"err.number = \" & err.number)\n"
                                              L"end function\n",
                                              NULL, NULL, NULL, 0, 0, 0, NULL, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);
    CHECK_CALLED(OnEnterScript);
    CHECK_CALLED(OnLeaveScript);

    hres = IActiveScript_GetScriptDispatch(script, NULL, &disp);
    ok(hres == S_OK, "GetScriptDispatch failed: %08lx\n", hres);

    hres = IDispatch_QueryInterface(disp, &IID_IDispatchEx, (void**)&dispex);
    ok(hres == S_OK, "Could not get IDispatchEx iface: %08lx\n", hres);
    IDispatch_Release(disp);

    str = SysAllocString(L"callGlobalCallback");
    hres = IDispatchEx_GetDispID(dispex, str, fdexNameCaseInsensitive, &id);
    ok(hres == S_OK, "GetDispID failed: %08lx\n", hres);
    SysFreeString(str);

    SET_EXPECT(OnEnterScript);
    /* OnLeaveScript will be set in global callback */
    SET_EXPECT(global_globalcallback_i);
    hres = IDispatchEx_InvokeEx(dispex, id, 0, DISPATCH_METHOD, &dp, &v, &ei, &caller_sp);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    /* OnEnterScript was checked in global callback */
    CHECK_CALLED(OnLeaveScript);
    CHECK_CALLED(global_globalcallback_i);

    store_script_error = &error1;

    SET_EXPECT(OnEnterScript);
    SET_EXPECT(OnLeaveScript);
    SET_EXPECT(OnScriptError);
    hres = IActiveScriptParse_ParseScriptText(parser, L"err.raise 2\n",
                                              NULL, NULL, NULL, 0, 0, 0, NULL, NULL);
    ok(hres == MAKE_VBSERROR(2), "ParseScriptText failed: %08lx\n", hres);
    CHECK_CALLED(OnEnterScript);
    CHECK_CALLED(OnLeaveScript);
    CHECK_CALLED(OnScriptError);

    store_script_error = &error2;

    SET_EXPECT(OnEnterScript);
    SET_EXPECT(OnLeaveScript);
    SET_EXPECT(OnScriptError);
    hres = IActiveScriptParse_ParseScriptText(parser,
                                              L"call ok(err.number = 0, \"err.number = \" & err.number)\n"
                                              L"err.raise &h86664004&, \"src\", \"desc\", \"help\", 1\n",
                                              NULL, NULL, NULL, 0, 0, 0, NULL, NULL);
    ok(hres == SCRIPT_E_RECORDED, "ParseScriptText failed: %08lx\n", hres);
    CHECK_CALLED(OnEnterScript);
    CHECK_CALLED(OnLeaveScript);
    CHECK_CALLED(OnScriptError);

    memset(&ei, 0xcc, sizeof(ei));
    hres = IActiveScriptError_GetExceptionInfo(error1, &ei);
    ok(hres == S_OK, "GetExceptionInfo returned %08lx\n", hres);
    ok(!ei.wCode, "wCode = %x\n", ei.wCode);
    ok(!ei.wReserved, "wReserved = %x\n", ei.wReserved);
    if(is_english) {
        ok(!wcscmp(ei.bstrSource, L"Microsoft VBScript runtime error"),
           "bstrSource = %s\n", wine_dbgstr_w(ei.bstrSource));
        ok(!wcscmp(ei.bstrDescription, L"Unknown runtime error"),
           "bstrDescription = %s\n", wine_dbgstr_w(ei.bstrDescription));
    }
    ok(!ei.bstrHelpFile, "bstrHelpFile = %s\n", wine_dbgstr_w(ei.bstrHelpFile));
    ok(!ei.dwHelpContext, "dwHelpContext = %lx\n", ei.dwHelpContext);
    ok(!ei.pvReserved, "pvReserved = %p\n", ei.pvReserved);
    ok(!ei.pfnDeferredFillIn, "pfnDeferredFillIn = %p\n", ei.pfnDeferredFillIn);
    ok(ei.scode == MAKE_VBSERROR(2), "scode = %lx\n", ei.scode);
    free_ei(&ei);

    IActiveScriptError_Release(error1);

    memset(&ei, 0xcc, sizeof(ei));
    hres = IActiveScriptError_GetExceptionInfo(error2, &ei);
    ok(hres == S_OK, "GetExceptionInfo returned %08lx\n", hres);
    ok(!ei.wCode, "wCode = %x\n", ei.wCode);
    ok(!ei.wReserved, "wReserved = %x\n", ei.wReserved);
    ok(!wcscmp(ei.bstrSource, L"src"), "bstrSource = %s\n", wine_dbgstr_w(ei.bstrSource));
    ok(!wcscmp(ei.bstrDescription, L"desc"), "bstrDescription = %s\n", wine_dbgstr_w(ei.bstrDescription));
    ok(!wcscmp(ei.bstrHelpFile, L"help"), "bstrHelpFile = %s\n", wine_dbgstr_w(ei.bstrHelpFile));
    ok(ei.dwHelpContext == 1, "dwHelpContext = %lx\n", ei.dwHelpContext);
    ok(!ei.pvReserved, "pvReserved = %p\n", ei.pvReserved);
    ok(!ei.pfnDeferredFillIn, "pfnDeferredFillIn = %p\n", ei.pfnDeferredFillIn);
    ok(ei.scode == SCRIPT_E_RECORDED, "scode = %lx\n", ei.scode);
    free_ei(&ei);

    IActiveScriptError_Release(error2);

    SET_EXPECT(OnEnterScript);
    SET_EXPECT(OnLeaveScript);
    SET_EXPECT(OnScriptError);
    hres = IActiveScriptParse_ParseScriptText(parser, L"err.raise &hffff&\n",
                                              NULL, NULL, NULL, 0, 0, 0, NULL, NULL);
    ok(hres == MAKE_VBSERROR(0xffff), "ParseScriptText failed: %08lx\n", hres);
    CHECK_CALLED(OnEnterScript);
    CHECK_CALLED(OnLeaveScript);
    CHECK_CALLED(OnScriptError);

    SET_EXPECT(OnEnterScript);
    SET_EXPECT(OnLeaveScript);
    SET_EXPECT(OnScriptError);
    hres = IActiveScriptParse_ParseScriptText(parser, L"err.raise &h80102030&\n",
                                              NULL, NULL, NULL, 0, 0, 0, NULL, NULL);
    ok(hres == 0x80102030, "ParseScriptText failed: %08lx\n", hres);
    CHECK_CALLED(OnEnterScript);
    CHECK_CALLED(OnLeaveScript);
    CHECK_CALLED(OnScriptError);

    SET_EXPECT(OnEnterScript);
    SET_EXPECT(global_testerrorobject_i);
    SET_EXPECT(OnLeaveScript);
    hres = IActiveScriptParse_ParseScriptText(parser,
                                              L"on error resume next\n"
                                              L"err.raise 1\n"
                                              L"testErrorObject err\n",
                                              NULL, NULL, NULL, 0, 0, 0, NULL, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);
    CHECK_CALLED(OnEnterScript);
    CHECK_CALLED(global_testerrorobject_i);
    CHECK_CALLED(OnLeaveScript);

    IDispatchEx_Release(dispex);

    IActiveScriptParse_Release(parser);
    close_script(script);
    strict_enter_script = FALSE;

    store_script_error = &error1;
    SET_EXPECT(OnScriptError);
    hres = parse_script_wr(L"throwException &h80004002&");
    ok(hres == MAKE_VBSERROR(430), "got error: %08lx\n", hres);
    CHECK_CALLED(OnScriptError);

    memset(&ei, 0xcc, sizeof(ei));
    hres = IActiveScriptError_GetExceptionInfo(error1, &ei);
    ok(hres == S_OK, "GetExceptionInfo returned %08lx\n", hres);
    ok(!ei.wCode, "wCode = %x\n", ei.wCode);
    ok(!ei.wReserved, "wReserved = %x\n", ei.wReserved);
    if(is_english) {
        ok(!wcscmp(ei.bstrSource, L"Microsoft VBScript runtime error"),
           "bstrSource = %s\n", wine_dbgstr_w(ei.bstrSource));
        ok(!wcscmp(ei.bstrDescription, L"Class doesn't support Automation"),
           "bstrDescription = %s\n", wine_dbgstr_w(ei.bstrDescription));
    }
    ok(!ei.bstrHelpFile, "bstrHelpFile = %s\n", wine_dbgstr_w(ei.bstrHelpFile));
    ok(!ei.dwHelpContext, "dwHelpContext = %lx\n", ei.dwHelpContext);
    ok(!ei.pvReserved, "pvReserved = %p\n", ei.pvReserved);
    ok(!ei.pfnDeferredFillIn, "pfnDeferredFillIn = %p\n", ei.pfnDeferredFillIn);
    ok(ei.scode == MAKE_VBSERROR(430), "scode = %lx\n", ei.scode);
    free_ei(&ei);

    IActiveScriptError_Release(error1);

    store_script_error = &error1;
    SET_EXPECT(OnScriptError);
    hres = parse_script_wr(L"throwException &h80004002&, \"test src\"");
    ok(hres == MAKE_VBSERROR(430), "got error: %08lx\n", hres);
    CHECK_CALLED(OnScriptError);

    memset(&ei, 0xcc, sizeof(ei));
    hres = IActiveScriptError_GetExceptionInfo(error1, &ei);
    ok(hres == S_OK, "GetExceptionInfo returned %08lx\n", hres);
    ok(!ei.wCode, "wCode = %x\n", ei.wCode);
    ok(!ei.wReserved, "wReserved = %x\n", ei.wReserved);
    if(is_english) {
        ok(!wcscmp(ei.bstrSource, L"test src"), "bstrSource = %s\n", wine_dbgstr_w(ei.bstrSource));
        ok(!wcscmp(ei.bstrDescription, L"Class doesn't support Automation"),
           "bstrDescription = %s\n", wine_dbgstr_w(ei.bstrDescription));
    }
    ok(!ei.bstrHelpFile, "bstrHelpFile = %s\n", wine_dbgstr_w(ei.bstrHelpFile));
    ok(!ei.dwHelpContext, "dwHelpContext = %lx\n", ei.dwHelpContext);
    ok(!ei.pvReserved, "pvReserved = %p\n", ei.pvReserved);
    ok(!ei.pfnDeferredFillIn, "pfnDeferredFillIn = %p\n", ei.pfnDeferredFillIn);
    ok(ei.scode == MAKE_VBSERROR(430), "scode = %lx\n", ei.scode);
    free_ei(&ei);

    IActiveScriptError_Release(error1);

    store_script_error = &error1;
    SET_EXPECT(OnScriptError);
    hres = parse_script_wr(L"throwException &h80004002&, , \"test desc\"");
    ok(hres == E_NOINTERFACE, "got error: %08lx\n", hres);
    CHECK_CALLED(OnScriptError);

    memset(&ei, 0xcc, sizeof(ei));
    hres = IActiveScriptError_GetExceptionInfo(error1, &ei);
    ok(hres == S_OK, "GetExceptionInfo returned %08lx\n", hres);
    ok(!ei.wCode, "wCode = %x\n", ei.wCode);
    ok(!ei.wReserved, "wReserved = %x\n", ei.wReserved);
    if(is_english) {
        ok(!ei.bstrSource, "bstrSource = %s\n", wine_dbgstr_w(ei.bstrSource));
        ok(!wcscmp(ei.bstrDescription, L"test desc"),
           "bstrDescription = %s\n", wine_dbgstr_w(ei.bstrDescription));
    }
    ok(!ei.bstrHelpFile, "bstrHelpFile = %s\n", wine_dbgstr_w(ei.bstrHelpFile));
    ok(!ei.dwHelpContext, "dwHelpContext = %lx\n", ei.dwHelpContext);
    ok(!ei.pvReserved, "pvReserved = %p\n", ei.pvReserved);
    ok(!ei.pfnDeferredFillIn, "pfnDeferredFillIn = %p\n", ei.pfnDeferredFillIn);
    ok(ei.scode == E_NOINTERFACE, "scode = %lx\n", ei.scode);
    free_ei(&ei);

    IActiveScriptError_Release(error1);

    store_script_error = &error1;
    SET_EXPECT(OnScriptError);
    hres = parse_script_wr(L"throwException &h80004002&, \"test src\", \"test desc\"");
    ok(hres == E_NOINTERFACE, "got error: %08lx\n", hres);
    CHECK_CALLED(OnScriptError);

    memset(&ei, 0xcc, sizeof(ei));
    hres = IActiveScriptError_GetExceptionInfo(error1, &ei);
    ok(hres == S_OK, "GetExceptionInfo returned %08lx\n", hres);
    ok(!ei.wCode, "wCode = %x\n", ei.wCode);
    ok(!ei.wReserved, "wReserved = %x\n", ei.wReserved);
    if(is_english) {
        ok(!wcscmp(ei.bstrSource, L"test src"), "bstrSource = %s\n", wine_dbgstr_w(ei.bstrSource));
        ok(!wcscmp(ei.bstrDescription, L"test desc"),
           "bstrDescription = %s\n", wine_dbgstr_w(ei.bstrDescription));
    }
    ok(!ei.bstrHelpFile, "bstrHelpFile = %s\n", wine_dbgstr_w(ei.bstrHelpFile));
    ok(!ei.dwHelpContext, "dwHelpContext = %lx\n", ei.dwHelpContext);
    ok(!ei.pvReserved, "pvReserved = %p\n", ei.pvReserved);
    ok(!ei.pfnDeferredFillIn, "pfnDeferredFillIn = %p\n", ei.pfnDeferredFillIn);
    ok(ei.scode == E_NOINTERFACE, "scode = %lx\n", ei.scode);
    free_ei(&ei);

    IActiveScriptError_Release(error1);

    store_script_error = &error1;
    SET_EXPECT(OnScriptError);
    hres = parse_script_wr(L"throwException &h8000FFFF&");
    ok(hres == E_UNEXPECTED, "got error: %08lx\n", hres);
    CHECK_CALLED(OnScriptError);

    memset(&ei, 0xcc, sizeof(ei));
    hres = IActiveScriptError_GetExceptionInfo(error1, &ei);
    ok(hres == S_OK, "GetExceptionInfo returned %08lx\n", hres);
    ok(!ei.wCode, "wCode = %x\n", ei.wCode);
    ok(!ei.wReserved, "wReserved = %x\n", ei.wReserved);
    if(is_english) {
        ok(!ei.bstrSource,
           "bstrSource = %s\n", wine_dbgstr_w(ei.bstrSource));
        ok(!ei.bstrDescription,
           "bstrDescription = %s\n", wine_dbgstr_w(ei.bstrDescription));
    }
    ok(!ei.bstrHelpFile, "bstrHelpFile = %s\n", wine_dbgstr_w(ei.bstrHelpFile));
    ok(!ei.dwHelpContext, "dwHelpContext = %lx\n", ei.dwHelpContext);
    ok(!ei.pvReserved, "pvReserved = %p\n", ei.pvReserved);
    ok(!ei.pfnDeferredFillIn, "pfnDeferredFillIn = %p\n", ei.pfnDeferredFillIn);
    ok(ei.scode == E_UNEXPECTED, "scode = %lx\n", ei.scode);
    free_ei(&ei);

    IActiveScriptError_Release(error1);
}

static void test_gc(void)
{
    IActiveScriptParse *parser;
    IActiveScript *engine;
    BSTR src;
    HRESULT hres;

    strict_dispid_check = FALSE;

    engine = create_script();
    if(!engine)
        return;

    hres = IActiveScript_QueryInterface(engine, &IID_IActiveScriptParse, (void**)&parser);
    ok(hres == S_OK, "Could not get IActiveScriptParse: %08lx\n", hres);

    hres = IActiveScriptParse_InitNew(parser);
    ok(hres == S_OK, "InitNew failed: %08lx\n", hres);

    hres = IActiveScript_SetScriptSite(engine, &ActiveScriptSite);
    ok(hres == S_OK, "SetScriptSite failed: %08lx\n", hres);

    hres = IActiveScript_AddNamedItem(engine, L"test",
            SCRIPTITEM_ISVISIBLE|SCRIPTITEM_ISSOURCE|SCRIPTITEM_GLOBALMEMBERS);
    ok(hres == S_OK, "AddNamedItem failed: %08lx\n", hres);

    hres = IActiveScript_SetScriptState(engine, SCRIPTSTATE_STARTED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_STARTED) failed: %08lx\n", hres);

    src = SysAllocString(
            L"class C\n"
            "    Public ref\n"
            "    Public Sub Class_Terminate\n"
            "        Call reportSuccess()\n"
            "    End Sub\n"
            "End Class\n"
            "Dim x\n"
            "set x = new C\n"
            "set x.ref = x\n"
            "set x = nothing\n");

    hres = IActiveScriptParse_ParseScriptText(parser, src, NULL, NULL, NULL, 0, 0, 0, NULL, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);
    SysFreeString(src);

    SET_EXPECT(global_success_d);
    SET_EXPECT(global_success_i);
    hres = IActiveScript_Close(engine);
    ok(hres == S_OK, "Close failed: %08lx\n", hres);
    CHECK_CALLED(global_success_d);
    CHECK_CALLED(global_success_i);

    IActiveScript_Release(engine);
    IActiveScriptParse_Release(parser);
}

static void test_parse_errors(void)
{
    static const struct
    {
        const WCHAR *src;
        unsigned error_line;
        int error_char;
    }
    invalid_scripts[] =
    {
        {
            /* If...End If */
            L"If 0 > 1 Then\n"
            "    x = 0 End If\n",
            1, 10
        },
        {
            /* While...End While */
            L"While False\n"
            "    x = 0 End While\n",
            1, 10
        },
        {
            /* While...Wend */
            L"While False\n"
            "    x = 0 Wend\n",
            1, 10
        },
        {
            /* Do While...Loop */
            L"Do While False\n"
            "    x = 0 Loop\n",
            1, 10
        },
        {
            /* Do Until...Loop */
            L"Do Until True\n"
            "    x = 0 Loop\n",
            1, 10
        },
        {
            /* Do...Loop While */
            L"Do\n"
            "    x = 0 Loop While False\n",
            1, 10
        },
        {
            /* Do...Loop Until */
            L"Do\n"
            "    x = 0 Loop Until True\n",
            1, 10
        },
        {
            /* Select...End Select */
            L"x = False\n"
            "Select Case 42\n"
            "    Case 0\n"
            "        Call ok(False, \"unexpected case\")\n"
            "    Case 42\n"
            "        x = True End Select\n"
            "Call ok(x, \"wrong case\")\n",
            5, 17
        },
        {
            /* Class...End Class  (empty) */
            L"Class C End Class",
            0, 8
        },
        {
            /* Class...End Class  (empty) */
            L"Class C _\nEnd Class",
            1, 0
        },
        {
            /* invalid use of parentheses for call statement */
            L"strcomp(\"x\", \"y\")",
            0, -17
        },
        {
            L"\n\n\n  cint _\n   throwInt(&h80001234&)",
            3, 2
        },
        {
            L"dim x\n"
            "if true then throwInt(&h80001234&)",
            1, 13
        },
        {
            L"dim x\n"
            "if x = throwInt(&h80001234&) then x = 1",
            1, 0
        },
        {
            L"sub test\n"
            "    dim x\n"
            "    if x = throwInt(&h80001234&) then x = 1\n"
            "end sub\n"
            "test\n",
            2, 4
        },
        {
            L"dim x\n"
            "do\n"
            "    x = 1\n"
            "loop until throwInt(&h80001234&)\n",
            3, 0
        },
        {
            L"\n  select case 3\n"
            "    case 2\n"
            "        ok false, \"unexpected case\"\n"
            "    case throwInt(&h80001234&)\n"
            "        throwInt &h87001234&\n"
            "end select\n",
            1, 2
        },
        {
            L"if false then\n"
            "    ok false, \"unexpected case\"\n"
            " elseif throwInt(&h80001234&) then\n"
            "    throwInt &h87001234&\n"
            "else\n"
            "    throwInt &h87001234&\n"
            "end if\n",
            2, 1
        }
    };
    HRESULT hres;
    UINT i;

    for (i = 0; i < ARRAY_SIZE(invalid_scripts); i++)
    {
        error_line = ~0;
        error_char = -1;
        onerror_hres = S_OK;

        SET_EXPECT(OnScriptError);
        hres = parse_script_wr(invalid_scripts[i].src);
        ok(hres == SCRIPT_E_REPORTED, "[%u] script returned: %08lx\n", i, hres);
        CHECK_CALLED(OnScriptError);

        ok(error_line == invalid_scripts[i].error_line, "[%u] error line %lu expected %u\n",
           i, error_line, invalid_scripts[i].error_line);
        todo_wine_if(invalid_scripts[i].error_char < 0)
        ok(error_char == abs(invalid_scripts[i].error_char), "[%u] error char %ld expected %d\n",
           i, error_char, invalid_scripts[i].error_char);
    }
}

static void test_msgbox(void)
{
    HRESULT hres;

    uic_handling = SCRIPTUICHANDLING_NOUIDEFAULT;

    SET_EXPECT(GetUIBehavior);
    SET_EXPECT(GetWindow);
    SET_EXPECT(EnableModeless);
    hres = parse_script_wr(L"MsgBox \"testing...\"");
    CLEAR_CALLED(GetUIBehavior);
    CLEAR_CALLED(GetWindow);
    CLEAR_CALLED(EnableModeless);
    if(FAILED(hres)) {
        win_skip("Skipping MsgBox tests, broken (probably too old) vbscript\n");
        return;
    }

    SET_EXPECT(GetUIBehavior);
    parse_script_w(L"dim r\n r=MsgBox(\"testing...\")\n Call ok(r=0, \"r=\"&r)");
    CHECK_CALLED(GetUIBehavior);

    SET_EXPECT(GetUIBehavior);
    parse_script_w(L"MsgBox 10");
    CHECK_CALLED(GetUIBehavior);

    uic_handling = SCRIPTUICHANDLING_ALLOW;

    SET_EXPECT(GetUIBehavior);
    SET_EXPECT(GetWindow);
    SET_EXPECT(EnableModeless);
    SET_EXPECT(OnScriptError);
    hres = parse_script_wr(L"MsgBox \"testing...\"");
    ok(FAILED(hres), "script not failed\n");
    CHECK_CALLED(GetUIBehavior);
    CHECK_CALLED(GetWindow);
    CHECK_CALLED(EnableModeless);
    CHECK_CALLED(OnScriptError);

    uic_handling = SCRIPTUICHANDLING_NOUIERROR;

    SET_EXPECT(GetUIBehavior);
    SET_EXPECT(OnScriptError);
    hres = parse_script_wr(L"MsgBox \"testing...\"");
    ok(FAILED(hres), "script not failed\n");
    CHECK_CALLED(GetUIBehavior);
    CHECK_CALLED(OnScriptError);
}

static HRESULT test_global_vars_ref(BOOL use_close)
{
    IActiveScriptParse *parser;
    IActiveScript *engine;
    BSTR script_str;
    LONG ref;
    HRESULT hres;

    engine = create_script();
    if(!engine)
        return S_OK;

    hres = IActiveScript_QueryInterface(engine, &IID_IActiveScriptParse, (void**)&parser);
    ok(hres == S_OK, "Could not get IActiveScriptParse: %08lx\n", hres);
    if (FAILED(hres))
    {
        IActiveScript_Release(engine);
        return hres;
    }

    hres = IActiveScriptParse_InitNew(parser);
    ok(hres == S_OK, "InitNew failed: %08lx\n", hres);

    hres = IActiveScript_SetScriptSite(engine, &ActiveScriptSite);
    ok(hres == S_OK, "SetScriptSite failed: %08lx\n", hres);

    hres = IActiveScript_AddNamedItem(engine, L"test", SCRIPTITEM_ISVISIBLE|SCRIPTITEM_ISSOURCE|SCRIPTITEM_GLOBALMEMBERS);
    ok(hres == S_OK, "AddNamedItem failed: %08lx\n", hres);

    hres = IActiveScript_SetScriptState(engine, SCRIPTSTATE_STARTED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_STARTED) failed: %08lx\n", hres);

    refobj_ref = 0;

    script_str = SysAllocString(L"Dim x\nset x = RefObj\n");
    hres = IActiveScriptParse_ParseScriptText(parser, script_str, NULL, NULL, NULL, 0, 0, 0, NULL, NULL);
    SysFreeString(script_str);

    ok(refobj_ref, "refobj_ref = 0\n");

    if(use_close) {
        hres = IActiveScript_Close(engine);
        ok(hres == S_OK, "Close failed: %08lx\n", hres);
    }else {
        hres = IActiveScript_SetScriptState(engine, SCRIPTSTATE_UNINITIALIZED);
        ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_STARTED) failed: %08lx\n", hres);
    }

    ok(!refobj_ref, "refobj_ref = %ld\n", refobj_ref);

    IActiveScript_Release(engine);

    ref = IActiveScriptParse_Release(parser);
    ok(!ref, "ref=%ld\n", ref);
    return hres;
}

static void test_isexpression(void)
{
    IActiveScriptParse *parser;
    IActiveScript *engine;
    IDispatch *disp;
    SCRIPTSTATE ss;
    HRESULT hres;
    VARIANT var;
    BSTR str;

    if (!(engine = create_and_init_script(0, FALSE)))
        return;

    hres = IActiveScript_QueryInterface(engine, &IID_IActiveScriptParse, (void**)&parser);
    ok(hres == S_OK, "Could not get IActiveScriptParse: %08lx\n", hres);
    if (FAILED(hres))
    {
        close_script(engine);
        return;
    }

    /* Expression when script is not started is still executed */
    hres = IActiveScript_GetScriptState(engine, &ss);
    ok(hres == S_OK, "GetScriptState failed: %08lx\n", hres);
    ok(ss == SCRIPTSTATE_INITIALIZED, "Wrong script state %u\n", ss);

    str = SysAllocString(L"13");
    hres = IActiveScriptParse_ParseScriptText(parser, str, NULL, NULL, NULL, 0, 0, SCRIPTTEXT_ISEXPRESSION, &var, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);
    ok(V_VT(&var) == VT_I2, "Expected VT_I2, got %s\n", vt2a(&var));
    ok(V_I2(&var) == 13, "Expected 13, got %d\n", V_I2(&var));
    VariantClear(&var);
    SysFreeString(str);

    hres = IActiveScript_SetScriptState(engine, SCRIPTSTATE_STARTED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_STARTED) failed: %08lx\n", hres);

    /* Empty expressions */
    V_VT(&var) = VT_I2;
    str = SysAllocString(L"");
    hres = IActiveScriptParse_ParseScriptText(parser, str, NULL, NULL, NULL, 0, 0, SCRIPTTEXT_ISEXPRESSION, &var, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);
    ok(V_VT(&var) == VT_EMPTY, "Expected VT_EMPTY, got %s\n", vt2a(&var));
    VariantClear(&var);
    SysFreeString(str);

    /* Two expressions fail */
    str = SysAllocString(L"1\n3");
    SET_EXPECT(OnScriptError);
    hres = IActiveScriptParse_ParseScriptText(parser, str, NULL, NULL, NULL, 0, 0, SCRIPTTEXT_ISEXPRESSION, &var, NULL);
    ok(FAILED(hres), "ParseScriptText did not fail: %08lx\n", hres);
    CHECK_CALLED(OnScriptError);
    VariantClear(&var);
    SysFreeString(str);

    /* Simple numerical expression */
    str = SysAllocString(L"(1 + 7) * 2 - 3");
    hres = IActiveScriptParse_ParseScriptText(parser, str, NULL, NULL, NULL, 0, 0, SCRIPTTEXT_ISEXPRESSION, NULL, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);

    hres = IActiveScriptParse_ParseScriptText(parser, str, NULL, NULL, NULL, 0, 0, SCRIPTTEXT_ISEXPRESSION, &var, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);
    ok(V_VT(&var) == VT_I2, "Expected VT_I2, got %s\n", vt2a(&var));
    ok(V_I2(&var) == 13, "Expected 13, got %d\n", V_I2(&var));
    VariantClear(&var);
    SysFreeString(str);

    /* Without a global host or named item context, "me" returns the script dispatch */
    hres = IActiveScript_GetScriptDispatch(engine, NULL, &disp);
    ok(hres == S_OK, "GetScriptDispatch failed: %08lx\n", hres);
    str = SysAllocString(L"me");
    hres = IActiveScriptParse_ParseScriptText(parser, str, NULL, NULL, NULL, 0, 0, SCRIPTTEXT_ISEXPRESSION, &var, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);
    ok(V_VT(&var) == VT_DISPATCH, "Expected VT_DISPATCH, got %s\n", vt2a(&var));
    ok(V_DISPATCH(&var) == disp, "Wrong dispatch returned for 'me'\n");
    IDispatch_Release(disp);
    VariantClear(&var);
    SysFreeString(str);

    /* An expression can also refer to a variable, function, class, etc previously set */
    V_VT(&var) = VT_I2;
    str = SysAllocString(L"If True Then foo = 42 Else foo = 0\n");
    hres = IActiveScriptParse_ParseScriptText(parser, str, NULL, NULL, NULL, 0, 0, 0, &var, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);
    ok(V_VT(&var) == VT_EMPTY, "Expected VT_EMPTY, got %s\n", vt2a(&var));
    VariantClear(&var);
    SysFreeString(str);

    str = SysAllocString(L"foo\n\n");
    hres = IActiveScriptParse_ParseScriptText(parser, str, NULL, NULL, NULL, 0, 0, SCRIPTTEXT_ISEXPRESSION, &var, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);
    ok(V_VT(&var) == VT_I2, "Expected VT_I2, got %s\n", vt2a(&var));
    ok(V_I2(&var) == 42, "Expected 42, got %d\n", V_I2(&var));
    VariantClear(&var);
    SysFreeString(str);

    str = SysAllocString(L"foo : ");
    SET_EXPECT(OnScriptError);
    hres = IActiveScriptParse_ParseScriptText(parser, str, NULL, NULL, NULL, 0, 0, SCRIPTTEXT_ISEXPRESSION, &var, NULL);
    ok(FAILED(hres), "ParseScriptText did not fail: %08lx\n", hres);
    CHECK_CALLED(OnScriptError);
    VariantClear(&var);
    SysFreeString(str);

    str = SysAllocString(L"\"foo is \" & CStr(foo)  \n  \n\n ");
    hres = IActiveScriptParse_ParseScriptText(parser, str, NULL, NULL, NULL, 0, 0, SCRIPTTEXT_ISEXPRESSION, &var, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);
    ok(V_VT(&var) == VT_BSTR, "Expected VT_BSTR, got %s\n", vt2a(&var));
    ok(!lstrcmpW(V_BSTR(&var), L"foo is 42"), "Wrong string, got %s\n", wine_dbgstr_w(V_BSTR(&var)));
    VariantClear(&var);
    SysFreeString(str);

    str = SysAllocString(L"Function test(x)\n"
                         "    test = x + 0.5\n"
                         "End Function\n");
    hres = IActiveScriptParse_ParseScriptText(parser, str, NULL, NULL, NULL, 0, 0, 0, NULL, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);
    SysFreeString(str);

    str = SysAllocString(L"test(4) * 3\n");
    hres = IActiveScriptParse_ParseScriptText(parser, str, NULL, NULL, NULL, 0, 0, SCRIPTTEXT_ISEXPRESSION, &var, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);
    ok(V_VT(&var) == VT_R8, "Expected VT_R8, got %s\n", vt2a(&var));
    ok(V_R8(&var) == 13.5, "Expected %lf, got %lf\n", 13.5, V_R8(&var));
    VariantClear(&var);
    SysFreeString(str);

    str = SysAllocString(L"Class C\n"
                         "    Public x\n"
                         "End Class\n"
                         "Set obj = New C\n"
                         "obj.x = True\n");
    hres = IActiveScriptParse_ParseScriptText(parser, str, NULL, NULL, NULL, 0, 0, 0, NULL, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);
    SysFreeString(str);

    str = SysAllocString(L"obj.x");
    hres = IActiveScriptParse_ParseScriptText(parser, str, NULL, NULL, NULL, 0, 0, SCRIPTTEXT_ISEXPRESSION, &var, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);
    ok(V_VT(&var) == VT_BOOL, "Expected VT_BOOL, got %s\n", vt2a(&var));
    ok(V_BOOL(&var) == VARIANT_TRUE, "Expected %x, got %x\n", VARIANT_TRUE, V_BOOL(&var));
    VariantClear(&var);
    SysFreeString(str);

    IActiveScriptParse_Release(parser);
    close_script(engine);
}

static void test_multiple_parse(void)
{
    IActiveScriptParse *parser;
    IActiveScript *script;
    HRESULT hres;

    script = create_and_init_script(SCRIPTITEM_GLOBALMEMBERS, TRUE);

    hres = IActiveScript_QueryInterface(script, &IID_IActiveScriptParse, (void**)&parser);
    ok(hres == S_OK, "Could not get IActiveScriptParseProcedure2 iface: %08lx\n", hres);

    hres = IActiveScriptParse_ParseScriptText(parser,
                                              L"function duplicatedfunc\n"
                                              L"  ok false, \"duplicatedfunc called\"\n"
                                              L"end function\n",
                                              NULL, NULL, NULL, 0, 0, 0, NULL, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);

    hres = IActiveScriptParse_ParseScriptText(parser,
                                              L"sub duplicatedfunc\n"
                                              L"  ok false, \"duplicatedfunc called\"\n"
                                              L"end sub\n",
                                              NULL, NULL, NULL, 0, 0, 0, NULL, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);

    hres = IActiveScriptParse_ParseScriptText(parser,
                                              L"function duplicatedfunc\n"
                                              L"  duplicatedfunc = 2\n"
                                              L"end function\n",
                                              NULL, NULL, NULL, 0, 0, 0, NULL, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);

    hres = IActiveScriptParse_ParseScriptText(parser,
                                              L"ok duplicatedfunc() = 2, \"duplicatedfunc = \" & duplicatedfunc()\n",
                                              NULL, NULL, NULL, 0, 0, 0, NULL, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);

    IActiveScriptParse_Release(parser);
    close_script(script);
}

static BSTR get_script_from_file(const char *filename)
{
    DWORD size, len;
    HANDLE file, map;
    const char *file_map;
    BSTR ret;

    file = CreateFileA(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
    if(file == INVALID_HANDLE_VALUE) {
        trace("Could not open file: %lu\n", GetLastError());
        return NULL;
    }

    size = GetFileSize(file, NULL);

    map = CreateFileMappingW(file, NULL, PAGE_READONLY, 0, 0, NULL);
    CloseHandle(file);
    if(map == INVALID_HANDLE_VALUE) {
        trace("Could not create file mapping: %lu\n", GetLastError());
        return NULL;
    }

    file_map = MapViewOfFile(map, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(map);
    if(!file_map) {
        trace("MapViewOfFile failed: %lu\n", GetLastError());
        return NULL;
    }

    len = MultiByteToWideChar(CP_ACP, 0, file_map, size, NULL, 0);
    ret = SysAllocStringLen(NULL, len);
    MultiByteToWideChar(CP_ACP, 0, file_map, size, ret, len);

    UnmapViewOfFile(file_map);

    return ret;
}

static void run_from_file(const char *filename)
{
    BSTR script_str;
    HRESULT hres;

    script_str = get_script_from_file(filename);
    if(!script_str)
        return;

    strict_dispid_check = FALSE;
    hres = parse_script(SCRIPTITEM_GLOBALMEMBERS, script_str, NULL);
    SysFreeString(script_str);
    ok(hres == S_OK, "parse_script failed: %08lx\n", hres);
}

static void run_from_res(const char *name)
{
    const char *data;
    DWORD size, len;
    BSTR str;
    HRSRC src;
    HRESULT hres;

    strict_dispid_check = FALSE;
    test_name = name;

    src = FindResourceA(NULL, name, (LPCSTR)40);
    ok(src != NULL, "Could not find resource %s\n", name);

    size = SizeofResource(NULL, src);
    data = LoadResource(NULL, src);

    len = MultiByteToWideChar(CP_ACP, 0, data, size, NULL, 0);
    str = SysAllocStringLen(NULL, len);
    MultiByteToWideChar(CP_ACP, 0, data, size, str, len);

    SET_EXPECT(global_success_d);
    SET_EXPECT(global_success_i);
    hres = parse_script(SCRIPTITEM_GLOBALMEMBERS, str, NULL);
    CHECK_CALLED(global_success_d);
    CHECK_CALLED(global_success_i);

    ok(hres == S_OK, "parse_script failed: %08lx\n", hres);
    SysFreeString(str);
    test_name = "";
}

static void run_tests(void)
{
    HRESULT hres;

    strict_dispid_check = TRUE;

    parse_script_w(L"");
    parse_script_w(L"' empty ;");

    SET_EXPECT(global_success_d);
    SET_EXPECT(global_success_i);
    parse_script_w(L"reportSuccess");
    CHECK_CALLED(global_success_d);
    CHECK_CALLED(global_success_i);

    SET_EXPECT(global_success_d);
    SET_EXPECT(global_success_i);
    parse_script_w(L"reportSuccess()");
    CHECK_CALLED(global_success_d);
    CHECK_CALLED(global_success_i);

    SET_EXPECT(global_success_d);
    SET_EXPECT(global_success_i);
    parse_script_w(L"Call reportSuccess");
    CHECK_CALLED(global_success_d);
    CHECK_CALLED(global_success_i);

    SET_EXPECT(global_success_d);
    SET_EXPECT(global_success_i);
    parse_script_w(L"test.reportSuccess()");
    CHECK_CALLED(global_success_d);
    CHECK_CALLED(global_success_i);

    SET_EXPECT(global_success_d);
    SET_EXPECT(global_success_i);
    parse_script_wf(0, L"TEST.reportSuccess()");
    CHECK_CALLED(global_success_d);
    CHECK_CALLED(global_success_i);

    SET_EXPECT(global_vbvar_d);
    SET_EXPECT(global_vbvar_i);
    parse_script_w(L"Option Explicit\nvbvar = 3");
    CHECK_CALLED(global_vbvar_d);
    CHECK_CALLED(global_vbvar_i);

    SET_EXPECT(global_vbvar_d);
    SET_EXPECT(global_vbvar_i);
    parse_script_w(L"Option Explicit\nvbvar() = 3");
    CHECK_CALLED(global_vbvar_d);
    CHECK_CALLED(global_vbvar_i);

    SET_EXPECT(global_letobj_i);
    parse_script_w(L"Option Explicit\nletobj = testObj");
    CHECK_CALLED(global_letobj_i);

    SET_EXPECT(global_letobj_i);
    parse_script_w(L"Option Explicit\ntest.letobj = testObj");
    CHECK_CALLED(global_letobj_i);

    SET_EXPECT(global_setobj_i);
    parse_script_w(L"Option Explicit\nset setobj = testObj");
    CHECK_CALLED(global_setobj_i);

    SET_EXPECT(global_setobj_i);
    parse_script_w(L"Option Explicit\nset test.setobj = testObj");
    CHECK_CALLED(global_setobj_i);

    hres = parse_script_wr(L"dim x\nx = testObj.rem");
    ok(hres == S_OK, "use of 'rem' as dot identifier failed: %lx08\n", hres);

    SET_EXPECT(testobj_propget_d);
    SET_EXPECT(testobj_propget_i);
    parse_script_w(L"dim x\nx = testObj.propget");
    CHECK_CALLED(testobj_propget_d);
    CHECK_CALLED(testobj_propget_i);

    SET_EXPECT(testobj_propput_d);
    SET_EXPECT(testobj_propput_i);
    parse_script_w(L"testObj.propput = 1");
    CHECK_CALLED(testobj_propput_d);
    CHECK_CALLED(testobj_propput_i);

    SET_EXPECT(global_propargput_d);
    SET_EXPECT(global_propargput_i);
    parse_script_w(L"propargput(counter(), counter()) = counter()");
    CHECK_CALLED(global_propargput_d);
    CHECK_CALLED(global_propargput_i);

    SET_EXPECT(global_propargput_d);
    SET_EXPECT(global_propargput_i);
    parse_script_w(L"test.propargput(counter(), counter()) = counter()");
    CHECK_CALLED(global_propargput_d);
    CHECK_CALLED(global_propargput_i);

    SET_EXPECT(global_propargput1_d);
    SET_EXPECT(global_propargput1_i);
    parse_script_w(L"propargput1 (counter()) = counter()");
    CHECK_CALLED(global_propargput1_d);
    CHECK_CALLED(global_propargput1_i);

    SET_EXPECT(global_propargput1_d);
    SET_EXPECT(global_propargput1_i);
    parse_script_w(L"test.propargput1(counter()) = counter()");
    CHECK_CALLED(global_propargput1_d);
    CHECK_CALLED(global_propargput1_i);

    SET_EXPECT(global_propargset_d);
    SET_EXPECT(global_propargset_i);
    parse_script_w(L"set propargset (counter()) = testObj");
    CHECK_CALLED(global_propargset_d);
    CHECK_CALLED(global_propargset_i);

    SET_EXPECT(testobj_propget_d);
    SET_EXPECT(testobj_propget_i);
    parse_script_w(L"dim x\nwith testObj\nx=1+.propget\nend with");
    CHECK_CALLED(testobj_propget_d);
    CHECK_CALLED(testobj_propget_i);

    SET_EXPECT(testobj_propput_d);
    SET_EXPECT(testobj_propput_i);
    parse_script_w(L"with testObj\n.propput = 1\nend with");
    CHECK_CALLED(testobj_propput_d);
    CHECK_CALLED(testobj_propput_i);

    SET_EXPECT(testobj_valueput_i);
    parse_script_w(L"dim x\n"
                   "set x = testObj\n"
                   "x(counter(), counter()) = counter\n");
    CHECK_CALLED(testobj_valueput_i);

    parse_script_w(L"dim x\nx = testObj.property(1)");

    parse_htmlscript(L"<!--");
    parse_htmlscript(L" -->");
    parse_htmlscript(L"<!--\ndim x\nx=1\n-->\n");
    parse_htmlscript(L"<!--\ndim x\n-->\n<!--\nx=1\n-->\n");

    SET_EXPECT(OnScriptError);
    hres = parse_script_wr(L"<!--");
    ok(FAILED(hres), "script didn't fail\n");
    CHECK_CALLED(OnScriptError);

    SET_EXPECT(global_success_d);
    SET_EXPECT(global_success_i);
    parse_htmlscript(L"<!--\n<!-- ignore this <> <>\n--> <>\nCall reportSuccess()\n-->\n");
    CHECK_CALLED(global_success_d);
    CHECK_CALLED(global_success_i);

    next_cnt = 0;
    SET_EXPECT(collectionobj_newenum_i);
    SET_EXPECT(Next);
    parse_script_w(L"for each x in collectionObj\nnext");
    CHECK_CALLED(collectionobj_newenum_i);
    CHECK_CALLED(Next);
    ok(next_cnt == 4, "next_cnt = %d\n", next_cnt);

    parse_script_w(L"x = 1\n Call ok(x = 1, \"x = \" & x)");

    parse_script_w(L"x = _    \n3");

    test_global_vars_ref(TRUE);
    test_global_vars_ref(FALSE);

    SET_EXPECT(OnScriptError);
    hres = parse_script_wr(L"throwInt(&h80080008&)");
    ok(hres == 0x80080008, "hres = %08lx\n", hres);
    CHECK_CALLED(OnScriptError);

    /* DISP_E_BADINDEX */
    SET_EXPECT(OnScriptError);
    hres = parse_script_wr(L"throwInt(&h8002000b&)");
    ok(hres == MAKE_VBSERROR(9), "hres = %08lx\n", hres);
    CHECK_CALLED(OnScriptError);

    SET_EXPECT(OnScriptError);
    hres = parse_script_wr(L"throwInt(&h800a0009&)");
    ok(hres == MAKE_VBSERROR(9), "hres = %08lx\n", hres);
    CHECK_CALLED(OnScriptError);

    onerror_hres = S_OK;
    SET_EXPECT(OnScriptError);
    hres = parse_script_wr(L"throwInt(&h800a0009&)");
    ok(hres == SCRIPT_E_REPORTED, "hres = %08lx\n", hres);
    CHECK_CALLED(OnScriptError);

    /* E_NOTIMPL */
    SET_EXPECT(OnScriptError);
    hres = parse_script_wr(L"throwInt(&h80004001&)");
    ok(hres == MAKE_VBSERROR(445), "hres = %08lx\n", hres);
    CHECK_CALLED(OnScriptError);

    onerror_hres = S_OK;
    SET_EXPECT(OnScriptError);
    hres = parse_script_wr(L"throwInt(&h80004001&)");
    ok(hres == SCRIPT_E_REPORTED, "hres = %08lx\n", hres);
    CHECK_CALLED(OnScriptError);

    SET_EXPECT(global_testoptionalarg_i);
    parse_script_w(L"call testOptionalArg(1,,2)");
    CHECK_CALLED(global_testoptionalarg_i);

    SET_EXPECT(global_testoptionalarg_i);
    parse_script_w(L"call testOptionalArg(,1,1)");
    CHECK_CALLED(global_testoptionalarg_i);

    SET_EXPECT(global_testoptionalarg_i);
    parse_script_w(L"testOptionalArg 1,,2");
    CHECK_CALLED(global_testoptionalarg_i);

    parse_script_w(L"sub x()\n"
                   "    dim y\n"
                   "    y = cint(3)\n"
                   "end sub\n"
                   "x\n");

    strict_dispid_check = FALSE;

    SET_EXPECT(testobj_value_i);
    parse_script_w(L"dim n,o\n set o = testObj\n n = o(1,2)\n call ok(n=2, \"n = \" & n)\n");
    CHECK_CALLED(testobj_value_i);

    SET_EXPECT(testobj_value_i);
    parse_script_w(L"dim n,o\n set o = testObj\n n = o\n call ok(n=0, \"n = \" & n)\n");
    CHECK_CALLED(testobj_value_i);

    parse_script_w(L"Sub testsub\n"
                   "x = 1\n"
                   "Call ok(x = 1, \"x = \" & x)\n"
                   "End Sub\n"
                   "Call testsub()");

    parse_script_w(L"Call ok(getVT(x) = \"VT_EMPTY*\", \"getVT(x) = \" & getVT(x))\n");
    parse_script_w(L"Call ok(x = \"\", \"x = \" & x)\n");
    parse_script_w(L"x = y\n"
                   "Call ok(getVT(x) = \"VT_EMPTY*\", \"getVT(x) = \" & getVT(x))\n"
                   "Call ok(getVT(y) = \"VT_EMPTY*\", \"getVT(y) = \" & getVT(y))");
    SET_EXPECT(OnScriptError);
    hres = parse_script_wr(L"x = y(\"a\")");
    ok(FAILED(hres), "script didn't fail\n");
    CHECK_CALLED(OnScriptError);

    SET_EXPECT(global_success_d);
    SET_EXPECT(global_success_i);
    parse_script_w(L"' comment\r"
                   "x = _\r3\r"
                   "x = _\n3\r"
                   "x = _\r\n3\r"
                   "Sub testsub(arg)\r"
                   "If arg = 1 Then\r\r"
                   "Call reportSuccess()\n\n"
                   "End If\r\n"
                   "End Sub\n\r"
                   "Call testsub(1)");
    CHECK_CALLED(global_success_d);
    CHECK_CALLED(global_success_i);

    run_from_res("lang.vbs");
    run_from_res("api.vbs");
    run_from_res("regexp.vbs");
    run_from_res("error.vbs");

    test_procedures();
    test_gc();
    test_msgbox();
    test_isexpression();
    test_parse_errors();
    test_parse_context();
    test_callbacks();
    test_multiple_parse();
}

static BOOL check_vbscript(void)
{
    IRegExp2 *regexp;
    IUnknown *unk;
    HRESULT hres;

    hres = CoCreateInstance(&CLSID_VBScriptRegExp, NULL,
            CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IUnknown, (void**)&unk);
    if(hres == REGDB_E_CLASSNOTREG)
        return FALSE;
    ok(hres == S_OK, "CoCreateInstance(CLSID_VBScriptRegExp) failed: %lx\n", hres);

    hres = IUnknown_QueryInterface(unk, &IID_IRegExp2, (void**)&regexp);
    if(SUCCEEDED(hres))
        IRegExp2_Release(regexp);
    IUnknown_Release(unk);

    return hres == S_OK;
}

START_TEST(run)
{
    int argc;
    char **argv;

    detect_locale();
    if(!is_english)
        skip("Skipping some tests in non-English locale\n");

    argc = winetest_get_mainargs(&argv);

    CoInitialize(NULL);

    if(!check_vbscript()) {
        win_skip("Broken engine, probably too old\n");
    }else if(argc > 2) {
        allow_ui = TRUE;
        uic_handling = SCRIPTUICHANDLING_ALLOW;
        run_from_file(argv[2]);
    }else {
        run_tests();
    }

    CoUninitialize();
}
