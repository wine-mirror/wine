/*
 * VARIANT test program
 *
 * Copyright 1998 Jean-Claude Cote
 * Copyright 2006 Google (Benjamin Arai)
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
#include <math.h>
#include <float.h>

#define COBJMACROS
#define CONST_VTABLE

#include "windef.h"
#include "winbase.h"
#include "winsock2.h"
#include "winuser.h"
#include "wingdi.h"
#include "winnls.h"
#include "winerror.h"
#include "winnt.h"

#include "wtypes.h"
#include "oleauto.h"

#include "wine/test.h"

static HMODULE hOleaut32;

static const WCHAR sz12[] = {'1','2','\0'};
/* the strings are localized */
static WCHAR sz12_false[32];
static WCHAR sz12_true[32];

/* Get a conversion function ptr, return if function not available */
#define CHECKPTR(func) p##func = (void*)GetProcAddress(hOleaut32, #func); \
  if (!p##func) { win_skip("function " # func " not available, not testing it\n"); return; }

/* Has I8/UI8 data type? */
static BOOL has_i8;

/* When comparing floating point values we cannot expect an exact match
 * because the rounding errors depend on the exact algorithm.
 */
#define EQ_DOUBLE(a,b)     (fabs((a)-(b)) / (1.0+fabs(a)+fabs(b)) < 1e-14)
#define EQ_FLOAT(a,b)      (fabs((a)-(b)) / (1.0+fabs(a)+fabs(b)) < 1e-7)

#define SKIPTESTS(a)  if((a > VT_CLSID+10) && (a < VT_BSTR_BLOB-10)) continue

/* Allow our test macros to work for VT_NULL and VT_EMPTY too */
#define V_EMPTY(v) V_I4(v)
#define V_NULL(v) V_I4(v)

/* Size constraints for overflow tests */
#define I1_MAX   0x7f
#define I1_MIN   ((-I1_MAX)-1)
#define UI1_MAX  0xff
#define UI1_MIN  0
#define I2_MAX   0x7fff
#define I2_MIN   ((-I2_MAX)-1)
#define UI2_MAX  0xffff
#define UI2_MIN  0
#define I4_MAX   0x7fffffff
#define I4_MIN   ((-I4_MAX)-1)
#define UI4_MAX  0xffffffff
#define UI4_MIN  0
#define I8_MAX   (((LONGLONG)I4_MAX << 32) | UI4_MAX)
#define I8_MIN   ((-I8_MAX)-1)
#define UI8_MAX  (((ULONGLONG)UI4_MAX << 32) | UI4_MAX)
#define UI8_MIN  0
#define DATE_MAX 2958465
#define DATE_MIN -657434
#define R4_MAX FLT_MAX
#define R4_MIN FLT_MIN
#define R8_MAX DBL_MAX
#define R8_MIN DBL_MIN

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

DEFINE_EXPECT(dispatch_invoke);

typedef struct
{
    IDispatch IDispatch_iface;
    VARTYPE vt;
    HRESULT result;
} DummyDispatch;

static inline DummyDispatch *impl_from_IDispatch(IDispatch *iface)
{
    return CONTAINING_RECORD(iface, DummyDispatch, IDispatch_iface);
}

static ULONG WINAPI DummyDispatch_AddRef(IDispatch *iface)
{
    return 2;
}

static ULONG WINAPI DummyDispatch_Release(IDispatch *iface)
{
    return 1;
}

static HRESULT WINAPI DummyDispatch_QueryInterface(IDispatch *iface,
                                                   REFIID riid,
                                                   void** ppvObject)
{
    *ppvObject = NULL;

    if (IsEqualIID(riid, &IID_IDispatch) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *ppvObject = iface;
        IDispatch_AddRef(iface);
    }

    return *ppvObject ? S_OK : E_NOINTERFACE;
}

static HRESULT WINAPI DummyDispatch_GetTypeInfoCount(IDispatch *iface, UINT *pctinfo)
{
    ok(0, "Unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DummyDispatch_GetTypeInfo(IDispatch *iface, UINT tinfo, LCID lcid, ITypeInfo **ti)
{
    ok(0, "Unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DummyDispatch_GetIDsOfNames(IDispatch *iface, REFIID riid, LPOLESTR *names,
    UINT cnames, LCID lcid, DISPID *dispid)
{
    ok(0, "Unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DummyDispatch_Invoke(IDispatch *iface,
                                           DISPID dispid, REFIID riid,
                                           LCID lcid, WORD wFlags,
                                           DISPPARAMS *params,
                                           VARIANT *res,
                                           EXCEPINFO *ei,
                                           UINT *arg_err)
{
    DummyDispatch *This = impl_from_IDispatch(iface);

    CHECK_EXPECT(dispatch_invoke);

    ok(dispid == DISPID_VALUE, "got dispid %ld\n", dispid);
    ok(IsEqualIID(riid, &IID_NULL), "go riid %s\n", wine_dbgstr_guid(riid));
    ok(wFlags == DISPATCH_PROPERTYGET, "Flags wrong\n");

    ok(params->rgvarg == NULL, "got %p\n", params->rgvarg);
    ok(params->rgdispidNamedArgs == NULL, "got %p\n", params->rgdispidNamedArgs);
    ok(params->cArgs == 0, "got %d\n", params->cArgs);
    ok(params->cNamedArgs == 0, "got %d\n", params->cNamedArgs);

    ok(res != NULL, "got %p\n", res);
    ok(V_VT(res) == VT_EMPTY, "got %d\n", V_VT(res));
    ok(ei == NULL, "got %p\n", ei);
    ok(arg_err == NULL, "got %p\n", arg_err);

    if (FAILED(This->result))
        return This->result;

    V_VT(res) = This->vt;
    if (This->vt == VT_UI1)
        V_UI1(res) = 34;
    else if (This->vt == VT_NULL)
    {
        V_VT(res) = VT_NULL;
        V_BSTR(res) = NULL;
    }
    else
        memset(res, 0, sizeof(*res));

    return S_OK;
}

static const IDispatchVtbl DummyDispatch_VTable =
{
    DummyDispatch_QueryInterface,
    DummyDispatch_AddRef,
    DummyDispatch_Release,
    DummyDispatch_GetTypeInfoCount,
    DummyDispatch_GetTypeInfo,
    DummyDispatch_GetIDsOfNames,
    DummyDispatch_Invoke
};

static void init_test_dispatch(VARTYPE vt, DummyDispatch *dispatch)
{
    dispatch->IDispatch_iface.lpVtbl = &DummyDispatch_VTable;
    dispatch->vt = vt;
    dispatch->result = S_OK;
}

typedef struct IRecordInfoImpl
{
    IRecordInfo IRecordInfo_iface;
    LONG ref;
    unsigned int recordclear;
    unsigned int getsize;
    unsigned int recordcopy;
    VARIANT *v;
} IRecordInfoImpl;

static inline IRecordInfoImpl *impl_from_IRecordInfo(IRecordInfo *iface)
{
    return CONTAINING_RECORD(iface, IRecordInfoImpl, IRecordInfo_iface);
}

static HRESULT WINAPI RecordInfo_QueryInterface(IRecordInfo *iface, REFIID riid, void **obj)
{
    *obj = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IRecordInfo))
    {
        *obj = iface;
        IRecordInfo_AddRef(iface);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI RecordInfo_AddRef(IRecordInfo *iface)
{
    IRecordInfoImpl* This = impl_from_IRecordInfo(iface);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI RecordInfo_Release(IRecordInfo *iface)
{
    IRecordInfoImpl* This = impl_from_IRecordInfo(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    if (!ref)
        HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

static HRESULT WINAPI RecordInfo_RecordInit(IRecordInfo *iface, PVOID pvNew)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI RecordInfo_RecordClear(IRecordInfo *iface, void *data)
{
    IRecordInfoImpl* This = impl_from_IRecordInfo(iface);
    This->recordclear++;
    V_RECORD(This->v) = NULL;
    return S_OK;
}

static HRESULT WINAPI RecordInfo_RecordCopy(IRecordInfo *iface, void *src, void *dest)
{
    IRecordInfoImpl* This = impl_from_IRecordInfo(iface);
    This->recordcopy++;
    ok(src == (void*)0xdeadbeef, "wrong src pointer %p\n", src);
    return S_OK;
}

static HRESULT WINAPI RecordInfo_GetGuid(IRecordInfo *iface, GUID *pguid)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI RecordInfo_GetName(IRecordInfo *iface, BSTR *pbstrName)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI RecordInfo_GetSize(IRecordInfo *iface, ULONG* size)
{
    IRecordInfoImpl* This = impl_from_IRecordInfo(iface);
    This->getsize++;
    *size = 0;
    return S_OK;
}

static HRESULT WINAPI RecordInfo_GetTypeInfo(IRecordInfo *iface, ITypeInfo **ppTypeInfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI RecordInfo_GetField(IRecordInfo *iface, PVOID pvData,
                                                LPCOLESTR szFieldName, VARIANT *pvarField)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI RecordInfo_GetFieldNoCopy(IRecordInfo *iface, PVOID pvData,
                            LPCOLESTR szFieldName, VARIANT *pvarField, PVOID *ppvDataCArray)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI RecordInfo_PutField(IRecordInfo *iface, ULONG wFlags, PVOID pvData,
                                            LPCOLESTR szFieldName, VARIANT *pvarField)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI RecordInfo_PutFieldNoCopy(IRecordInfo *iface, ULONG wFlags,
                PVOID pvData, LPCOLESTR szFieldName, VARIANT *pvarField)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI RecordInfo_GetFieldNames(IRecordInfo *iface, ULONG *pcNames,
                                                BSTR *rgBstrNames)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static BOOL WINAPI RecordInfo_IsMatchingType(IRecordInfo *iface, IRecordInfo *info2)
{
    ok(0, "unexpected call\n");
    return FALSE;
}

static PVOID WINAPI RecordInfo_RecordCreate(IRecordInfo *iface)
{
    ok(0, "unexpected call\n");
    return NULL;
}

static HRESULT WINAPI RecordInfo_RecordCreateCopy(IRecordInfo *iface, PVOID pvSource,
                                                    PVOID *ppvDest)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI RecordInfo_RecordDestroy(IRecordInfo *iface, PVOID pvRecord)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IRecordInfoVtbl RecordInfoVtbl =
{
    RecordInfo_QueryInterface,
    RecordInfo_AddRef,
    RecordInfo_Release,
    RecordInfo_RecordInit,
    RecordInfo_RecordClear,
    RecordInfo_RecordCopy,
    RecordInfo_GetGuid,
    RecordInfo_GetName,
    RecordInfo_GetSize,
    RecordInfo_GetTypeInfo,
    RecordInfo_GetField,
    RecordInfo_GetFieldNoCopy,
    RecordInfo_PutField,
    RecordInfo_PutFieldNoCopy,
    RecordInfo_GetFieldNames,
    RecordInfo_IsMatchingType,
    RecordInfo_RecordCreate,
    RecordInfo_RecordCreateCopy,
    RecordInfo_RecordDestroy
};

static IRecordInfoImpl *get_test_recordinfo(void)
{
    IRecordInfoImpl *rec;

    rec = HeapAlloc(GetProcessHeap(), 0, sizeof(IRecordInfoImpl));
    rec->IRecordInfo_iface.lpVtbl = &RecordInfoVtbl;
    rec->ref = 1;
    rec->recordclear = 0;
    rec->getsize = 0;
    rec->recordcopy = 0;

    return rec;
}

static void init(void)
{
    BSTR bstr;
    HRESULT res;

    res = VarBstrFromBool(VARIANT_TRUE, LANG_USER_DEFAULT, VAR_LOCALBOOL, &bstr);
    ok(res == S_OK && bstr[0], "Expected localized string for 'True'\n");
    /* lstrcpyW / lstrcatW do not work on win95 */
    memcpy(sz12_true, sz12, sizeof(sz12));
    if (bstr) memcpy(&sz12_true[2], bstr, SysStringByteLen(bstr) + sizeof(WCHAR));
    SysFreeString(bstr);

    res = VarBstrFromBool(VARIANT_FALSE, LANG_USER_DEFAULT, VAR_LOCALBOOL, &bstr);
    ok(res == S_OK && bstr[0], "Expected localized string for 'False'\n");
    memcpy(sz12_false, sz12, sizeof(sz12));
    if (bstr) memcpy(&sz12_false[2], bstr, SysStringByteLen(bstr) + sizeof(WCHAR));
    SysFreeString(bstr);

    hOleaut32 = GetModuleHandleA("oleaut32.dll");
    has_i8 = GetProcAddress(hOleaut32, "VarI8FromI1") != NULL;
    if (!has_i8)
        skip("No support for I8 and UI8 data types\n");
}

/* Functions to set a DECIMAL */
static void setdec(DECIMAL* dec, BYTE scl, BYTE sgn, ULONG hi32, ULONG64 lo64)
{
    dec->scale = scl;
    dec->sign = sgn;
    dec->Hi32 = hi32;
    dec->Lo64 = lo64;
}

static void setdec64(DECIMAL* dec, BYTE scl, BYTE sgn, ULONG hi32, ULONG mid32, ULONG lo32)
{
    dec->scale = scl;
    dec->sign = sgn;
    dec->Hi32 = hi32;
    dec->Mid32 = mid32;
    dec->Lo32 = lo32;
}

static BOOL is_expected_variant( const VARIANT *result, const VARIANT *expected )
{
    if (V_VT(result) != V_VT(expected)) return FALSE;
    switch(V_VT(expected))
    {
    case VT_EMPTY:
    case VT_NULL:
        return TRUE;

#define CASE(vt) case VT_##vt: return (V_##vt(result) == V_##vt(expected))
    CASE(BOOL);
    CASE(I1);
    CASE(UI1);
    CASE(I2);
    CASE(UI2);
    CASE(I4);
    CASE(UI4);
    CASE(I8);
    CASE(UI8);
    CASE(INT);
    CASE(UINT);
#undef CASE

    case VT_DATE:
        return EQ_FLOAT(V_DATE(result), V_DATE(expected));
    case VT_R4:
        return EQ_FLOAT(V_R4(result), V_R4(expected));
    case VT_R8:
        return EQ_FLOAT(V_R8(result), V_R8(expected));
    case VT_CY:
        return (V_CY(result).int64 == V_CY(expected).int64);
    case VT_BSTR:
        return !lstrcmpW( V_BSTR(result), V_BSTR(expected) );
    case VT_DECIMAL:
        return !memcmp( &V_DECIMAL(result), &V_DECIMAL(expected), sizeof(DECIMAL) );
    default:
        ok(0, "unhandled variant type %s\n",wine_dbgstr_vt(V_VT(expected)));
        return FALSE;
    }
}

static void test_var_call1( int line, HRESULT (WINAPI *func)(LPVARIANT,LPVARIANT),
                           VARIANT *arg, VARIANT *expected )
{
    VARIANT old_arg = *arg;
    VARIANT result;
    HRESULT hres;

    memset( &result, 0, sizeof(result) );
    hres = func( arg, &result );
    ok_(__FILE__,line)( hres == S_OK, "wrong result %lx\n", hres );
    if (hres == S_OK)
        ok_(__FILE__,line)( is_expected_variant( &result, expected ),
                            "got %s expected %s\n", wine_dbgstr_variant(&result), wine_dbgstr_variant(expected) );
    ok_(__FILE__,line)( is_expected_variant( arg, &old_arg ), "Modified argument %s / %s\n",
                        wine_dbgstr_variant(&old_arg), wine_dbgstr_variant(arg));
    VariantClear( &result );
}

static void test_var_call2( int line, HRESULT (WINAPI *func)(LPVARIANT,LPVARIANT,LPVARIANT),
                            VARIANT *left, VARIANT *right, VARIANT *expected )
{
    VARIANT old_left = *left, old_right = *right;
    VARIANT result;
    HRESULT hres;

    memset( &result, 0, sizeof(result) );
    hres = func( left, right, &result );
    ok_(__FILE__,line)( hres == S_OK, "wrong result %lx\n", hres );
    if (hres == S_OK)
        ok_(__FILE__,line)( is_expected_variant( &result, expected ),
                            "got %s expected %s\n", wine_dbgstr_variant(&result), wine_dbgstr_variant(expected) );
    ok_(__FILE__,line)( is_expected_variant( left, &old_left ), "Modified left argument %s / %s\n",
                        wine_dbgstr_variant(&old_left), wine_dbgstr_variant(left));
    ok_(__FILE__,line)( is_expected_variant( right, &old_right ), "Modified right argument %s / %s\n",
                        wine_dbgstr_variant(&old_right), wine_dbgstr_variant(right));
    VariantClear( &result );
}

#define test_bstr_var(a,b) _test_bstr_var(__LINE__,a,b)
static void _test_bstr_var(unsigned line, const VARIANT *v, const WCHAR *str)
{
    ok_(__FILE__,line)(V_VT(v) == VT_BSTR, "unexpected vt=%d\n", V_VT(v));
    if(V_VT(v) == VT_BSTR)
        ok(!lstrcmpW(V_BSTR(v), str), "v=%s, expected %s\n", wine_dbgstr_w(V_BSTR(v)),
           wine_dbgstr_w(str));
}

static void test_VariantInit(void)
{
    VARIANT v, v2, v3;

    memset(&v, -1, sizeof(v));
    memset(&v2, 0, sizeof(v2));
    memset(&v3, -1, sizeof(v3));
    V_VT(&v3) = VT_EMPTY;

    VariantInit(&v);
    ok(!memcmp(&v, &v2, sizeof(v)) ||
            broken(!memcmp(&v, &v3, sizeof(v3)) /* pre Win8 */), "Unexpected contents.\n");
}

/* All possible combinations of extra V_VT() flags */
static const VARTYPE ExtraFlags[16] =
{
  0,
  VT_VECTOR,
  VT_ARRAY,
  VT_BYREF,
  VT_RESERVED,
  VT_VECTOR|VT_ARRAY,
  VT_VECTOR|VT_BYREF,
  VT_VECTOR|VT_RESERVED,
  VT_VECTOR|VT_ARRAY|VT_BYREF,
  VT_VECTOR|VT_ARRAY|VT_RESERVED,
  VT_VECTOR|VT_BYREF|VT_RESERVED,
  VT_VECTOR|VT_ARRAY|VT_BYREF|VT_RESERVED,
  VT_ARRAY|VT_BYREF,
  VT_ARRAY|VT_RESERVED,
  VT_ARRAY|VT_BYREF|VT_RESERVED,
  VT_BYREF|VT_RESERVED,
};

/* Determine if a vt is valid for VariantClear() */
static BOOL IsValidVariantClearVT(VARTYPE vt, VARTYPE extraFlags)
{
  BOOL ret = FALSE;

  /* Only the following flags/types are valid */
  if ((vt <= VT_LPWSTR || vt == VT_RECORD || vt == VT_CLSID) &&
      vt != (VARTYPE)15 &&
      (vt < (VARTYPE)24 || vt > (VARTYPE)31) &&
      (!(extraFlags & (VT_BYREF|VT_ARRAY)) || vt > VT_NULL) &&
      (extraFlags == 0 || extraFlags == VT_BYREF || extraFlags == VT_ARRAY ||
       extraFlags == (VT_ARRAY|VT_BYREF)))
    ret = TRUE; /* ok */

  if (!has_i8 && (vt == VT_I8 || vt == VT_UI8))
    ret = FALSE; /* Old versions of oleaut32 */
  return ret;
}

typedef struct
{
    IUnknown IUnknown_iface;
    LONG     ref;
    LONG     events;
} test_VariantClearImpl;

static inline test_VariantClearImpl *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, test_VariantClearImpl, IUnknown_iface);
}

static HRESULT WINAPI VC_QueryInterface(LPUNKNOWN iface,REFIID riid,LPVOID *ppobj)
{
    test_VariantClearImpl *This = impl_from_IUnknown(iface);
    This->events |= 0x1;
    return E_NOINTERFACE;
}

static ULONG WINAPI VC_AddRef(LPUNKNOWN iface) {
    test_VariantClearImpl *This = impl_from_IUnknown(iface);
    This->events |= 0x2;
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI VC_Release(LPUNKNOWN iface) {
    test_VariantClearImpl *This = impl_from_IUnknown(iface);
    /* static class, won't be  freed */
    This->events |= 0x4;
    return InterlockedDecrement(&This->ref);
}

static const IUnknownVtbl test_VariantClear_vtbl = {
    VC_QueryInterface,
    VC_AddRef,
    VC_Release,
};

static test_VariantClearImpl test_myVariantClearImpl = {{&test_VariantClear_vtbl}, 1, 0};

static void test_VariantClear(void)
{
  IRecordInfoImpl *recinfo;
  HRESULT hres;
  VARIANTARG v;
  VARIANT v2;
  size_t i;
  LONG i4;
  IUnknown *punk;

  /* Crashes: Native does not test input for NULL, so neither does Wine */
  if (0)
      VariantClear(NULL);

  /* Only the type field is set, to VT_EMPTY */
  V_VT(&v) = VT_UI4;
  V_UI4(&v) = ~0u;
  hres = VariantClear(&v);
  ok((hres == S_OK && V_VT(&v) == VT_EMPTY),
     "VariantClear: Type set to %d, res %08lx\n", V_VT(&v), hres);
  ok(V_UI4(&v) == ~0u, "VariantClear: Overwrote value\n");

  /* Test all possible V_VT values.
   * Also demonstrates that null pointers in 'v' are not dereferenced.
   * Individual variant tests should test VariantClear() with non-NULL values.
   */
  for (i = 0; i < ARRAY_SIZE(ExtraFlags); i++)
  {
    VARTYPE vt;

    for (vt = 0; vt <= VT_BSTR_BLOB; vt++)
    {
      HRESULT hExpected = DISP_E_BADVARTYPE;

      SKIPTESTS(vt);

      memset(&v, 0, sizeof(v));
      V_VT(&v) = vt | ExtraFlags[i];

      hres = VariantClear(&v);

      if (IsValidVariantClearVT(vt, ExtraFlags[i]))
        hExpected = S_OK;

      ok(hres == hExpected, "VariantClear: expected 0x%lX, got 0x%lX for vt %d | 0x%X\n",
         hExpected, hres, vt, ExtraFlags[i]);
    }
  }

  /* Some BYREF tests with non-NULL ptrs */

  /* VARIANT BYREF */
  V_VT(&v2) = VT_I4;
  V_I4(&v2) = 0x1234;
  V_VT(&v) = VT_VARIANT | VT_BYREF;
  V_VARIANTREF(&v) = &v2;

  hres = VariantClear(&v);
  ok(hres == S_OK, "ret %08lx\n", hres);
  ok(V_VT(&v) == 0, "vt %04x\n", V_VT(&v));
  ok(V_VARIANTREF(&v) == &v2, "variant ref %p\n", V_VARIANTREF(&v2));
  ok(V_VT(&v2) == VT_I4, "vt %04x\n", V_VT(&v2));
  ok(V_I4(&v2) == 0x1234, "i4 %04lx\n", V_I4(&v2));

  /* I4 BYREF */
  i4 = 0x4321;
  V_VT(&v) = VT_I4 | VT_BYREF;
  V_I4REF(&v) = &i4;

  hres = VariantClear(&v);
  ok(hres == S_OK, "ret %08lx\n", hres);
  ok(V_VT(&v) == 0, "vt %04x\n", V_VT(&v));
  ok(V_I4REF(&v) == &i4, "i4 ref %p\n", V_I4REF(&v2));
  ok(i4 == 0x4321, "i4 changed %08lx\n", i4);


  /* UNKNOWN */
  V_VT(&v) = VT_UNKNOWN;
  V_UNKNOWN(&v) = &test_myVariantClearImpl.IUnknown_iface;
  test_myVariantClearImpl.events = 0;
  hres = VariantClear(&v);
  ok(hres == S_OK, "ret %08lx\n", hres);
  ok(V_VT(&v) == 0, "vt %04x\n", V_VT(&v));
  ok(V_UNKNOWN(&v) == &test_myVariantClearImpl.IUnknown_iface, "unknown %p\n", V_UNKNOWN(&v));
  /* Check that Release got called, but nothing else */
  ok(test_myVariantClearImpl.events ==  0x4, "Unexpected call. events %08lx\n", test_myVariantClearImpl.events);

  /* UNKNOWN BYREF */
  punk = &test_myVariantClearImpl.IUnknown_iface;
  V_VT(&v) = VT_UNKNOWN | VT_BYREF;
  V_UNKNOWNREF(&v) = &punk;
  test_myVariantClearImpl.events = 0;
  hres = VariantClear(&v);
  ok(hres == S_OK, "ret %08lx\n", hres);
  ok(V_VT(&v) == 0, "vt %04x\n", V_VT(&v));
  ok(V_UNKNOWNREF(&v) == &punk, "unknown ref %p\n", V_UNKNOWNREF(&v));
  /* Check that nothing got called */
  ok(test_myVariantClearImpl.events ==  0, "Unexpected call. events %08lx\n", test_myVariantClearImpl.events);

  /* DISPATCH */
  V_VT(&v) = VT_DISPATCH;
  V_DISPATCH(&v) = (IDispatch*)&test_myVariantClearImpl.IUnknown_iface;
  test_myVariantClearImpl.events = 0;
  hres = VariantClear(&v);
  ok(hres == S_OK, "ret %08lx\n", hres);
  ok(V_VT(&v) == 0, "vt %04x\n", V_VT(&v));
  ok(V_DISPATCH(&v) == (IDispatch*)&test_myVariantClearImpl.IUnknown_iface,
     "dispatch %p\n", V_DISPATCH(&v));
  /* Check that Release got called, but nothing else */
  ok(test_myVariantClearImpl.events ==  0x4, "Unexpected call. events %08lx\n", test_myVariantClearImpl.events);

  /* DISPATCH BYREF */
  punk = &test_myVariantClearImpl.IUnknown_iface;
  V_VT(&v) = VT_DISPATCH | VT_BYREF;
  V_DISPATCHREF(&v) = (IDispatch**)&punk;
  test_myVariantClearImpl.events = 0;
  hres = VariantClear(&v);
  ok(hres == S_OK, "ret %08lx\n", hres);
  ok(V_VT(&v) == 0, "vt %04x\n", V_VT(&v));
  ok(V_DISPATCHREF(&v) == (IDispatch**)&punk, "dispatch ref %p\n", V_DISPATCHREF(&v));
  /* Check that nothing got called */
  ok(test_myVariantClearImpl.events ==  0, "Unexpected call. events %08lx\n", test_myVariantClearImpl.events);

  /* RECORD */
  recinfo = get_test_recordinfo();
  V_VT(&v) = VT_RECORD;
  V_RECORDINFO(&v) = &recinfo->IRecordInfo_iface;
  V_RECORD(&v) = (void*)0xdeadbeef;
  recinfo->recordclear = 0;
  recinfo->ref = 2;
  recinfo->v = &v;
  hres = VariantClear(&v);
  ok(hres == S_OK, "ret %08lx\n", hres);
  ok(V_RECORD(&v) == NULL, "got %p\n", V_RECORD(&v));
  ok(recinfo->recordclear == 1, "got %d\n", recinfo->recordclear);
  ok(recinfo->ref == 1, "got %ld\n", recinfo->ref);
  IRecordInfo_Release(&recinfo->IRecordInfo_iface);
}

static void test_VariantCopy(void)
{
  IRecordInfoImpl *recinfo;
  VARIANTARG vSrc, vDst;
  VARTYPE vt;
  size_t i;
  HRESULT hres, hExpected;

  /* Establish that the failure/other cases are dealt with. Individual tests
   * for each type should verify that data is copied correctly, references
   * are updated, etc.
   */

  /* vSrc == vDst */
  for (i = 0; i < ARRAY_SIZE(ExtraFlags); i++)
  {
    for (vt = 0; vt <= VT_BSTR_BLOB; vt++)
    {
      SKIPTESTS(vt);

      memset(&vSrc, 0, sizeof(vSrc));
      V_VT(&vSrc) = vt | ExtraFlags[i];

      hExpected = DISP_E_BADVARTYPE;
      /* src is allowed to be a VT_CLSID */
      if (vt != VT_CLSID && IsValidVariantClearVT(vt, ExtraFlags[i]))
        hExpected = S_OK;

      hres = VariantCopy(&vSrc, &vSrc);

      ok(hres == hExpected,
         "Copy(src==dst): expected 0x%lX, got 0x%lX for src==dest vt %d|0x%X\n",
         hExpected, hres, vt, ExtraFlags[i]);
    }
  }

  /* Test that if VariantClear() fails on dest, the function fails. This also
   * shows that dest is in fact cleared and not just overwritten
   */
  memset(&vSrc, 0, sizeof(vSrc));
  V_VT(&vSrc) = VT_UI1;

  for (i = 0; i < ARRAY_SIZE(ExtraFlags); i++)
  {
    for (vt = 0; vt <= VT_BSTR_BLOB; vt++)
    {
      SKIPTESTS(vt);

      hExpected = DISP_E_BADVARTYPE;

      memset(&vDst, 0, sizeof(vDst));
      V_VT(&vDst) = vt | ExtraFlags[i];

      if (IsValidVariantClearVT(vt, ExtraFlags[i]))
        hExpected = S_OK;

      hres = VariantCopy(&vDst, &vSrc);

      ok(hres == hExpected,
         "Copy(bad dst): expected 0x%lX, got 0x%lX for dest vt %d|0x%X\n",
         hExpected, hres, vt, ExtraFlags[i]);
      if (hres == S_OK)
        ok(V_VT(&vDst) == VT_UI1,
           "Copy(bad dst): expected vt = VT_UI1, got %d\n", V_VT(&vDst));
    }
  }

  /* Test that VariantClear() checks vSrc for validity before copying */
  for (i = 0; i < ARRAY_SIZE(ExtraFlags); i++)
  {
    for (vt = 0; vt <= VT_BSTR_BLOB; vt++)
    {
      SKIPTESTS(vt);

      hExpected = DISP_E_BADVARTYPE;

      memset(&vDst, 0, sizeof(vDst));
      V_VT(&vDst) = VT_EMPTY;

      memset(&vSrc, 0, sizeof(vSrc));
      V_VT(&vSrc) = vt | ExtraFlags[i];

      /* src is allowed to be a VT_CLSID */
      if (vt != VT_CLSID && IsValidVariantClearVT(vt, ExtraFlags[i]))
        hExpected = S_OK;

      hres = VariantCopy(&vDst, &vSrc);

      ok(hres == hExpected,
         "Copy(bad src): expected 0x%lX, got 0x%lX for src vt %d|0x%X\n",
         hExpected, hres, vt, ExtraFlags[i]);
      if (hres == S_OK)
      {
        ok(V_VT(&vDst) == (vt|ExtraFlags[i]),
           "Copy(bad src): expected vt = %d, got %d\n",
           vt | ExtraFlags[i], V_VT(&vDst));
        VariantClear(&vDst);
      }
    }
  }
  
  /* Test that copying a NULL BSTR results in an empty BSTR */
  memset(&vDst, 0, sizeof(vDst));
  V_VT(&vDst) = VT_EMPTY;
  memset(&vSrc, 0, sizeof(vSrc));
  V_VT(&vSrc) = VT_BSTR;
  hres = VariantCopy(&vDst, &vSrc);
  ok(hres == S_OK, "Copy(NULL BSTR): Failed to copy a NULL BSTR\n");
  if (hres == S_OK)
  {
    ok((V_VT(&vDst) == VT_BSTR) && V_BSTR(&vDst),
       "Copy(NULL BSTR): should have non-NULL result\n");
    if ((V_VT(&vDst) == VT_BSTR) && V_BSTR(&vDst))
    {
      ok(*V_BSTR(&vDst) == 0, "Copy(NULL BSTR): result not empty\n");
    }
    VariantClear(&vDst);
  }

  /* copy RECORD */
  recinfo = get_test_recordinfo();

  memset(&vDst, 0, sizeof(vDst));
  V_VT(&vDst) = VT_EMPTY;

  V_VT(&vSrc) = VT_RECORD;
  V_RECORDINFO(&vSrc) = &recinfo->IRecordInfo_iface;
  V_RECORD(&vSrc) = (void*)0xdeadbeef;

  recinfo->recordclear = 0;
  recinfo->recordcopy = 0;
  recinfo->getsize = 0;
  recinfo->v = &vSrc;
  hres = VariantCopy(&vDst, &vSrc);
  ok(hres == S_OK, "ret %08lx\n", hres);

  ok(V_RECORD(&vDst) != (void*)0xdeadbeef && V_RECORD(&vDst) != NULL, "got %p\n", V_RECORD(&vDst));
  ok(V_RECORDINFO(&vDst) == &recinfo->IRecordInfo_iface, "got %p\n", V_RECORDINFO(&vDst));
  ok(recinfo->getsize == 1, "got %d\n", recinfo->recordclear);
  ok(recinfo->recordcopy == 1, "got %d\n", recinfo->recordclear);

  VariantClear(&vDst);
  VariantClear(&vSrc);
}

/* Determine if a vt is valid for VariantCopyInd() */
static BOOL IsValidVariantCopyIndVT(VARTYPE vt, VARTYPE extraFlags)
{
  BOOL ret = FALSE;

  if ((extraFlags & VT_ARRAY) ||
     (vt > VT_NULL && vt != (VARTYPE)15 && vt < VT_VOID &&
     !(extraFlags & (VT_VECTOR|VT_RESERVED))))
  {
    ret = TRUE; /* ok */
  }
  return ret;
}

static void test_VariantCopyInd(void)
{
  VARIANTARG vSrc, vDst, vRef, vRef2;
  VARTYPE vt;
  size_t i;
  BYTE buffer[64];
  HRESULT hres, hExpected;

  memset(buffer, 0, sizeof(buffer));

  /* vSrc == vDst */
  for (i = 0; i < ARRAY_SIZE(ExtraFlags); i++)
  {
    if (ExtraFlags[i] & VT_ARRAY)
      continue; /* Native crashes on NULL safearray */

    for (vt = 0; vt <= VT_BSTR_BLOB; vt++)
    {
      SKIPTESTS(vt);

      memset(&vSrc, 0, sizeof(vSrc));
      V_VT(&vSrc) = vt | ExtraFlags[i];

      hExpected = DISP_E_BADVARTYPE;
      if (!(ExtraFlags[i] & VT_BYREF))
      {
        /* if src is not by-reference, acts as VariantCopy() */
        if (vt != VT_CLSID && IsValidVariantClearVT(vt, ExtraFlags[i]))
          hExpected = S_OK;
      }
      else
      {
        if (vt == VT_SAFEARRAY || vt == VT_BSTR || vt == VT_UNKNOWN ||
            vt == VT_DISPATCH || vt == VT_RECORD)
          continue; /* Need valid ptrs for deep copies */

        V_BYREF(&vSrc) = &buffer;
        hExpected = E_INVALIDARG;

        if ((vt == VT_I8 || vt == VT_UI8) &&
            ExtraFlags[i] == VT_BYREF)
        {
          if (has_i8)
            hExpected = S_OK; /* Only valid if I8 is a known type */
        }
        else if (IsValidVariantCopyIndVT(vt, ExtraFlags[i]))
          hExpected = S_OK;
      }

      hres = VariantCopyInd(&vSrc, &vSrc);

      ok(hres == hExpected,
         "CopyInd(src==dst): expected 0x%lX, got 0x%lX for src==dst vt %d|0x%X\n",
         hExpected, hres, vt, ExtraFlags[i]);
    }
  }

  /* Bad dest */
  memset(&vSrc, 0, sizeof(vSrc));
  V_VT(&vSrc) = VT_UI1|VT_BYREF;
  V_BYREF(&vSrc) = &buffer;

  for (i = 0; i < ARRAY_SIZE(ExtraFlags); i++)
  {
    for (vt = 0; vt <= VT_BSTR_BLOB; vt++)
    {
      SKIPTESTS(vt);

      memset(&vDst, 0, sizeof(vDst));
      V_VT(&vDst) = vt | ExtraFlags[i];

      hExpected = DISP_E_BADVARTYPE;

      if (IsValidVariantClearVT(vt, ExtraFlags[i]))
        hExpected = S_OK;

      hres = VariantCopyInd(&vDst, &vSrc);

      ok(hres == hExpected,
         "CopyInd(bad dst): expected 0x%lX, got 0x%lX for dst vt %d|0x%X\n",
         hExpected, hres, vt, ExtraFlags[i]);
      if (hres == S_OK)
        ok(V_VT(&vDst) == VT_UI1,
           "CopyInd(bad dst): expected vt = VT_UI1, got %d\n", V_VT(&vDst));
    }
  }

  /* bad src */
  for (i = 0; i < ARRAY_SIZE(ExtraFlags); i++)
  {
    if (ExtraFlags[i] & VT_ARRAY)
      continue; /* Native crashes on NULL safearray */

    for (vt = 0; vt <= VT_BSTR_BLOB; vt++)
    {
      SKIPTESTS(vt);

      memset(&vDst, 0, sizeof(vDst));
      V_VT(&vDst) = VT_EMPTY;

      memset(&vSrc, 0, sizeof(vSrc));
      V_VT(&vSrc) = vt | ExtraFlags[i];

      hExpected = DISP_E_BADVARTYPE;
      if (!(ExtraFlags[i] & VT_BYREF))
      {
        /* if src is not by-reference, acts as VariantCopy() */
        if (vt != VT_CLSID && IsValidVariantClearVT(vt, ExtraFlags[i]))
          hExpected = S_OK;
      }
      else
      {
        if (vt == VT_SAFEARRAY || vt == VT_BSTR || vt == VT_UNKNOWN ||
            vt == VT_DISPATCH || vt == VT_RECORD)
          continue; /* Need valid ptrs for deep copies, see vartype.c */

        V_BYREF(&vSrc) = &buffer;

        hExpected = E_INVALIDARG;

        if ((vt == VT_I8 || vt == VT_UI8) &&
            ExtraFlags[i] == VT_BYREF)
        {
          if (has_i8)
            hExpected = S_OK; /* Only valid if I8 is a known type */
        }
        else if (IsValidVariantCopyIndVT(vt, ExtraFlags[i]))
          hExpected = S_OK;
      }

      hres = VariantCopyInd(&vDst, &vSrc);

      ok(hres == hExpected,
         "CopyInd(bad src): expected 0x%lX, got 0x%lX for src vt %d|0x%X\n",
         hExpected, hres, vt, ExtraFlags[i]);
      if (hres == S_OK)
      {
        if (vt == VT_VARIANT && ExtraFlags[i] == VT_BYREF)
        {
          /* Type of vDst should be the type of the referenced variant.
           * Since we set the buffer to all zeros, its type should be
           * VT_EMPTY.
           */
          ok(V_VT(&vDst) == VT_EMPTY,
             "CopyInd(bad src): expected dst vt = VT_EMPTY, got %d|0x%X\n",
             V_VT(&vDst) & VT_TYPEMASK, V_VT(&vDst) & ~VT_TYPEMASK);
        }
        else
        {
          ok(V_VT(&vDst) == (vt|(ExtraFlags[i] & ~VT_BYREF)),
             "CopyInd(bad src): expected dst vt = %d|0x%X, got %d|0x%X\n",
             vt, ExtraFlags[i] & ~VT_BYREF,
             V_VT(&vDst) & VT_TYPEMASK, V_VT(&vDst) & ~VT_TYPEMASK);
        }
        VariantClear(&vDst);
      }
    }
  }

  /* By-reference variants are dereferenced */
  V_VT(&vRef) = VT_UI1;
  V_UI1(&vRef) = 0x77;
  V_VT(&vSrc) = VT_VARIANT|VT_BYREF;
  V_VARIANTREF(&vSrc) = &vRef;
  VariantInit(&vDst);

  hres = VariantCopyInd(&vDst, &vSrc);
  ok(hres == S_OK, "VariantCopyInd failed: 0x%08lx\n", hres);
  ok(V_VT(&vDst) == VT_UI1 && V_UI1(&vDst) == 0x77,
     "CopyInd(deref): expected dst vt = VT_UI1, val 0x77, got %d|0x%X, 0x%2X\n",
      V_VT(&vDst) & VT_TYPEMASK, V_VT(&vDst) & ~VT_TYPEMASK, V_UI1(&vDst));

  /* By-reference variant to a by-reference type succeeds */
  V_VT(&vRef) = VT_UI1|VT_BYREF;
  V_UI1REF(&vRef) = buffer; buffer[0] = 0x88;
  V_VT(&vSrc) = VT_VARIANT|VT_BYREF;
  V_VARIANTREF(&vSrc) = &vRef;
  VariantInit(&vDst);

  hres = VariantCopyInd(&vDst, &vSrc);
  ok(hres == S_OK, "VariantCopyInd failed: 0x%08lx\n", hres);
  ok(V_VT(&vDst) == VT_UI1 && V_UI1(&vDst) == 0x88,
     "CopyInd(deref): expected dst vt = VT_UI1, val 0x77, got %d|0x%X, 0x%2X\n",
      V_VT(&vDst) & VT_TYPEMASK, V_VT(&vDst) & ~VT_TYPEMASK, V_UI1(&vDst));

  /* But a by-reference variant to a by-reference variant fails */
  V_VT(&vRef2) = VT_UI1;
  V_UI1(&vRef2) = 0x77;
  V_VT(&vRef) = VT_VARIANT|VT_BYREF;
  V_VARIANTREF(&vRef) = &vRef2;
  V_VT(&vSrc) = VT_VARIANT|VT_BYREF;
  V_VARIANTREF(&vSrc) = &vRef;
  VariantInit(&vDst);

  hres = VariantCopyInd(&vDst, &vSrc);
  ok(hres == E_INVALIDARG,
     "CopyInd(ref->ref): expected E_INVALIDARG, got 0x%08lx\n", hres);
}

/* Macros for converting and testing the result of VarParseNumFromStr */
#define FAILDIG 255

static HRESULT wconvert_str( const OLECHAR *str, INT dig, ULONG npflags,
                             NUMPARSE *np, BYTE rgb[128], LCID lcid, ULONG flags)
{
    memset( rgb, FAILDIG, 128 );
    memset( np, 255, sizeof(*np) );
    np->cDig = dig;
    np->dwInFlags = npflags;
    return VarParseNumFromStr( str, lcid, flags, np, rgb);
}

static HRESULT convert_str( const char *str, INT dig, ULONG flags,
                            NUMPARSE *np, BYTE rgb[128], LCID lcid )
{
    OLECHAR buff[128];
    MultiByteToWideChar( CP_ACP,0, str, -1, buff, ARRAY_SIZE( buff ));
    return wconvert_str(buff, dig, flags, np, rgb, lcid, LOCALE_NOUSEROVERRIDE);
}

static void expect_NumFromStr( int line, HRESULT hres, NUMPARSE *np, INT a, ULONG b, ULONG c,
                               INT d, INT e, INT f )
{
    ok_(__FILE__,line)(hres == S_OK, "returned %08lx\n", hres);
    if (hres == S_OK)
    {
        ok_(__FILE__,line)(np->cDig == a, "Expected cDig = %d, got %d\n", a, np->cDig);
        ok_(__FILE__,line)(np->dwInFlags == b, "Expected dwInFlags = 0x%lx, got 0x%lx\n", b, np->dwInFlags);
        ok_(__FILE__,line)(np->dwOutFlags == c, "Expected dwOutFlags = 0x%lx, got 0x%lx\n", c, np->dwOutFlags);
        ok_(__FILE__,line)(np->cchUsed == d, "Expected cchUsed = %d, got %d\n", d, np->cchUsed);
        ok_(__FILE__,line)(np->nBaseShift == e, "Expected nBaseShift = %d, got %d\n", e, np->nBaseShift);
        ok_(__FILE__,line)(np->nPwr10 == f, "Expected nPwr10 = %d, got %d\n", f, np->nPwr10);
    }
}

#define WCONVERTN(str,dig,flags) hres = wconvert_str( str, dig, flags, &np, rgb, lcid, LOCALE_NOUSEROVERRIDE )
#define WCONVERT(str,flags) WCONVERTN(str,sizeof(rgb),flags)
#define CONVERTN(str,dig,flags) hres = convert_str( str, dig, flags, &np, rgb, lcid )
#define CONVERT(str,flags) CONVERTN(str,sizeof(rgb),flags)
#define EXPECT(a,b,c,d,e,f) expect_NumFromStr( __LINE__, hres, &np, a, b, c, d, e, f )
#define EXPECTRGB(a,b) ok(rgb[a] == b, "Digit[%d], expected %d, got %d\n", a, b, rgb[a])
#define EXPECTFAIL ok(hres == DISP_E_TYPEMISMATCH, "Call succeeded, hres = %08lx\n", hres)
#define EXPECT2(a,b) EXPECTRGB(0,a); EXPECTRGB(1,b)

static void test_VarParseNumFromStrEn(void)
{
  HRESULT hres;
  /* Ensure all tests are using the same locale characters for '$', ',' etc */
  LCID lcid = MAKELCID(MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),SORT_DEFAULT);
  NUMPARSE np;
  BYTE rgb[128];
  OLECHAR wstr[128];
  OLECHAR spaces[] = L" \xa0\f\n\r\t\v"; /* man isspace() */
  int i;

  /** No flags **/

  /* Consume an empty string */
  CONVERT("", 0);
  EXPECTFAIL;

  /* Consume a single digit */
  CONVERT("7", 0);
  EXPECT(1,0,0,1,0,0);
  EXPECT2(7,FAILDIG);

  /* cDig is not literal digits - zeros are suppressed and nPwr10 is increased */
  CONVERT("10", 0);
  EXPECT(1,0,0,2,0,1);
  /* Note: Win32 writes the trailing zeros if they are within cDig's limits,
   * but then excludes them from the returned cDig count.
   * In our implementation we don't bother writing them at all.
   */
  EXPECTRGB(0, 1);

  /* if cDig is too small and numbers follow, sets INEXACT */
  CONVERTN("11",1, 0);
  EXPECT(1,0,NUMPRS_INEXACT,2,0,1);
  EXPECT2(1,FAILDIG);

  /* Strips leading zeros */
  CONVERT("01", 0);
  EXPECT(1,0,0,2,0,0);
  EXPECT2(1,FAILDIG);

  /* Strips leading zeros */
  CONVERTN("01",1, 0);
  EXPECT(1,0,0,2,0,0);
  EXPECT2(1,FAILDIG);


  /* Fails on non digits */
  CONVERT("a", 0);
  EXPECTFAIL;
  EXPECTRGB(0,FAILDIG);

  /** NUMPRS_LEADING_WHITE/NUMPRS_TRAILING_WHITE **/

  /* Without flag, fails on whitespace */
  CONVERT(" 0", 0);
  EXPECTFAIL;
  EXPECTRGB(0,FAILDIG);


  /* With flag, consumes any type of space */
  for (i = 0; i < ARRAY_SIZE(spaces)-1; i++)
  {
      winetest_push_context("%d", i);

      wsprintfW(wstr, L"%c0", spaces[i]);
      WCONVERT(wstr, NUMPRS_LEADING_WHITE);
      EXPECT(1,NUMPRS_LEADING_WHITE,NUMPRS_LEADING_WHITE,2,0,0);
      EXPECT2(0,FAILDIG);

      winetest_pop_context();
  }

  /* Doesn't pick up trailing whitespace without flag */
  CONVERT("0 ", 0);
  EXPECT(1,0,0,1,0,0);
  EXPECT2(0,FAILDIG);

  /* With flag, consumes trailing whitespace */
  CONVERT("0 ", NUMPRS_TRAILING_WHITE);
  EXPECT(1,NUMPRS_TRAILING_WHITE,NUMPRS_TRAILING_WHITE,2,0,0);
  EXPECT2(0,FAILDIG);

  /* Leading flag only consumes leading */
  CONVERT(" 0 ", NUMPRS_LEADING_WHITE);
  EXPECT(1,NUMPRS_LEADING_WHITE,NUMPRS_LEADING_WHITE,2,0,0);
  EXPECT2(0,FAILDIG);

  /* Both flags consumes both */
  CONVERT(" 0 ", NUMPRS_LEADING_WHITE|NUMPRS_TRAILING_WHITE);
  EXPECT(1,NUMPRS_LEADING_WHITE|NUMPRS_TRAILING_WHITE,NUMPRS_LEADING_WHITE|NUMPRS_TRAILING_WHITE,3,0,0);
  EXPECT2(0,FAILDIG);

  /** NUMPRS_LEADING_PLUS/NUMPRS_TRAILING_PLUS **/

  /* Without flag, fails on + */
  CONVERT("+0", 0);
  EXPECTFAIL;
  EXPECTRGB(0,FAILDIG);

  /* With flag, consumes + */
  CONVERT("+0", NUMPRS_LEADING_PLUS);
  EXPECT(1,NUMPRS_LEADING_PLUS,NUMPRS_LEADING_PLUS,2,0,0);
  EXPECT2(0,FAILDIG);

  /* Without flag, doesn't consume trailing + */
  CONVERT("0+", 0);
  EXPECT(1,0,0,1,0,0);
  EXPECT2(0,FAILDIG);

  /* With flag, consumes trailing + */
  CONVERT("0+", NUMPRS_TRAILING_PLUS);
  EXPECT(1,NUMPRS_TRAILING_PLUS,NUMPRS_TRAILING_PLUS,2,0,0);
  EXPECT2(0,FAILDIG);

  /* With leading flag, doesn't consume trailing + */
  CONVERT("+0+", NUMPRS_LEADING_PLUS);
  EXPECT(1,NUMPRS_LEADING_PLUS,NUMPRS_LEADING_PLUS,2,0,0);
  EXPECT2(0,FAILDIG);

  /* Trailing + doesn't get consumed if we specify both (unlike whitespace) */
  CONVERT("+0+", NUMPRS_LEADING_PLUS|NUMPRS_TRAILING_PLUS);
  EXPECT(1,NUMPRS_LEADING_PLUS|NUMPRS_TRAILING_PLUS,NUMPRS_LEADING_PLUS,2,0,0);
  EXPECT2(0,FAILDIG);

  /** NUMPRS_LEADING_MINUS/NUMPRS_TRAILING_MINUS **/

  /* Without flag, fails on - */
  CONVERT("-0", 0);
  EXPECTFAIL;
  EXPECTRGB(0,FAILDIG);

  /* With flag, consumes - */
  CONVERT("-0", NUMPRS_LEADING_MINUS);
  EXPECT(1,NUMPRS_LEADING_MINUS,NUMPRS_NEG|NUMPRS_LEADING_MINUS,2,0,0);
  EXPECT2(0,FAILDIG);

  /* Without flag, doesn't consume trailing - */
  CONVERT("0-", 0);
  EXPECT(1,0,0,1,0,0);
  EXPECT2(0,FAILDIG);

  /* With flag, consumes trailing - */
  CONVERT("0-", NUMPRS_TRAILING_MINUS);
  EXPECT(1,NUMPRS_TRAILING_MINUS,NUMPRS_NEG|NUMPRS_TRAILING_MINUS,2,0,0);
  EXPECT2(0,FAILDIG);

  /* With leading flag, doesn't consume trailing - */
  CONVERT("-0-", NUMPRS_LEADING_MINUS);
  EXPECT(1,NUMPRS_LEADING_MINUS,NUMPRS_NEG|NUMPRS_LEADING_MINUS,2,0,0);
  EXPECT2(0,FAILDIG);

  /* Trailing - doesn't get consumed if we specify both (unlike whitespace) */
  CONVERT("-0-", NUMPRS_LEADING_MINUS|NUMPRS_TRAILING_MINUS);
  EXPECT(1,NUMPRS_LEADING_MINUS|NUMPRS_TRAILING_MINUS,NUMPRS_NEG|NUMPRS_LEADING_MINUS,2,0,0);
  EXPECT2(0,FAILDIG);

  /** NUMPRS_HEX_OCT **/

  /* Could be hex, octal or decimal - With flag reads as decimal */
  CONVERT("0", NUMPRS_HEX_OCT);
  EXPECT(1,NUMPRS_HEX_OCT,0,1,0,0);
  EXPECT2(0,FAILDIG);

  /* Doesn't recognise hex in .asm syntax */
  CONVERT("0h", NUMPRS_HEX_OCT);
  EXPECT(1,NUMPRS_HEX_OCT,0,1,0,0);
  EXPECT2(0,FAILDIG);

  /* Doesn't fail with valid leading string but no digits */
  CONVERT("0x", NUMPRS_HEX_OCT);
  EXPECT(1,NUMPRS_HEX_OCT,0,1,0,0);
  EXPECT2(0,FAILDIG);

  /* Doesn't recognise hex format numbers at all! */
  CONVERT("0x0", NUMPRS_HEX_OCT);
  EXPECT(1,NUMPRS_HEX_OCT,0,1,0,0);
  EXPECT2(0,FAILDIG);

  /* Doesn't recognise plain hex digits either */
  CONVERT("FE", NUMPRS_HEX_OCT);
  EXPECTFAIL;
  EXPECTRGB(0,FAILDIG);

  /* A leading 0 does not an octal number make */
  CONVERT("0100", NUMPRS_HEX_OCT);
  EXPECT(1,NUMPRS_HEX_OCT,0,4,0,2);
  EXPECTRGB(0,1);
  EXPECTRGB(1,0);
  EXPECTRGB(2,0);
  EXPECTRGB(3,FAILDIG);

  /* VB hex */
  CONVERT("&HF800", NUMPRS_HEX_OCT);
  EXPECT(4,NUMPRS_HEX_OCT,NUMPRS_HEX_OCT,6,4,0);
  EXPECTRGB(0,15);
  EXPECTRGB(1,8);
  EXPECTRGB(2,0);
  EXPECTRGB(3,0);
  EXPECTRGB(4,FAILDIG);

  /* VB hex lower case and leading zero */
  CONVERT("&h0abcdef", NUMPRS_HEX_OCT);
  EXPECT(6,NUMPRS_HEX_OCT,NUMPRS_HEX_OCT,9,4,0);
  EXPECTRGB(0,10);
  EXPECTRGB(1,11);
  EXPECTRGB(2,12);
  EXPECTRGB(3,13);
  EXPECTRGB(4,14);
  EXPECTRGB(5,15);
  EXPECTRGB(6,FAILDIG);

  /* VB oct */
  CONVERT("&O300", NUMPRS_HEX_OCT);
  EXPECT(3,NUMPRS_HEX_OCT,NUMPRS_HEX_OCT,5,3,0);
  EXPECTRGB(0,3);
  EXPECTRGB(1,0);
  EXPECTRGB(2,0);
  EXPECTRGB(3,FAILDIG);

  /* VB oct lower case and leading zero */
  CONVERT("&o0777", NUMPRS_HEX_OCT);
  EXPECT(3,NUMPRS_HEX_OCT,NUMPRS_HEX_OCT,6,3,0);
  EXPECTRGB(0,7);
  EXPECTRGB(1,7);
  EXPECTRGB(2,7);
  EXPECTRGB(3,FAILDIG);

  /* VB oct char bigger than 7 */
  CONVERT("&o128", NUMPRS_HEX_OCT);
  EXPECT(2,NUMPRS_HEX_OCT,NUMPRS_HEX_OCT,4,3,0);
  EXPECTRGB(0,1);
  EXPECTRGB(1,2);
  EXPECTRGB(3,FAILDIG);

  /* Only integers are allowed when using an alternative radix */
  CONVERT("&ha.2", NUMPRS_HEX_OCT|NUMPRS_DECIMAL);
  EXPECT(1,NUMPRS_HEX_OCT|NUMPRS_DECIMAL,NUMPRS_HEX_OCT,3,4,0);
  EXPECT2(10,FAILDIG);

  /* Except if it looks like a plain decimal number */
  CONVERT("01.2", NUMPRS_HEX_OCT|NUMPRS_DECIMAL);
  EXPECT(2,NUMPRS_HEX_OCT|NUMPRS_DECIMAL,NUMPRS_DECIMAL,4,0,-1);
  EXPECT2(1,2);
  EXPECTRGB(3,FAILDIG);

  /** NUMPRS_PARENS **/

  /* Empty parens = error */
  CONVERT("()", NUMPRS_PARENS);
  EXPECTFAIL;
  EXPECTRGB(0,FAILDIG);

  /* With flag, trailing parens not consumed */
  CONVERT("0()", NUMPRS_PARENS);
  EXPECT(1,NUMPRS_PARENS,0,1,0,0);
  EXPECT2(0,FAILDIG);

  /* With flag, Number in parens made negative and parens consumed */
  CONVERT("(0)", NUMPRS_PARENS);
  EXPECT(1,NUMPRS_PARENS,NUMPRS_NEG|NUMPRS_PARENS,3,0,0);
  EXPECT2(0,FAILDIG);

  /** NUMPRS_THOUSANDS **/

  /* With flag, thousands sep. not needed */
  CONVERT("0", NUMPRS_THOUSANDS);
  EXPECT(1,NUMPRS_THOUSANDS,0,1,0,0);
  EXPECT2(0,FAILDIG);

  /* Without flag stop at thousands separator */
  CONVERT("1,000", 0);
  EXPECT(1,0,0,1,0,0);
  EXPECT2(1,FAILDIG);

  /* With flag, thousands sep. and following digits consumed */
  CONVERT("1,000", NUMPRS_THOUSANDS);
  EXPECT(1,NUMPRS_THOUSANDS,NUMPRS_THOUSANDS,5,0,3);
  EXPECTRGB(0,1);
  /* VarParseNumFromStr() may have added more digits to rgb but they are not
   * part of the returned value. So consider that an implementation detail.
   */
  EXPECTRGB(4,FAILDIG);

  /* With flag, thousands sep. and following digits consumed */
  CONVERT("&h1,000", NUMPRS_HEX_OCT|NUMPRS_THOUSANDS);
  EXPECT(1,NUMPRS_HEX_OCT|NUMPRS_THOUSANDS,NUMPRS_HEX_OCT,3,4,0);
  EXPECTRGB(1,FAILDIG);

  /* With flag and decimal point, thousands sep. but not decimals consumed */
  CONVERT("1,001.0", NUMPRS_THOUSANDS);
  EXPECT(4,NUMPRS_THOUSANDS,NUMPRS_THOUSANDS,5,0,0);
  EXPECT2(1,0);
  EXPECTRGB(2,0);
  EXPECTRGB(3,1);
  EXPECTRGB(4,FAILDIG);

  /* With flag, consecutive thousands separators are allowed */
  CONVERT("1,,000", NUMPRS_THOUSANDS);
  EXPECT(1,NUMPRS_THOUSANDS,NUMPRS_THOUSANDS,6,0,3);
  EXPECTRGB(0,1); /* Don't test extra digits, see "1,000" test */
  EXPECTRGB(4,FAILDIG);

  /* With flag, thousands separators can be sprinkled at random */
  CONVERT("1,00,0,,", NUMPRS_THOUSANDS);
  EXPECT(1,NUMPRS_THOUSANDS,NUMPRS_THOUSANDS,8,0,3);
  EXPECTRGB(0,1); /* Don't test extra digits, see "1,000" test */
  EXPECTRGB(4,FAILDIG);

  /* With flag, but leading thousands separators are not allowed */
  CONVERT(",1,000", NUMPRS_THOUSANDS);
  EXPECTFAIL;

  /* With flag, thousands separator not needed but still reported */
  CONVERT("1,", NUMPRS_THOUSANDS);
  EXPECT(1,NUMPRS_THOUSANDS,NUMPRS_THOUSANDS,2,0,0);
  EXPECT2(1,FAILDIG);

  /** NUMPRS_CURRENCY **/

  /* Without flag, chokes on currency sign */
  CONVERT("$11", 0);
  EXPECTFAIL;
  EXPECTRGB(0,FAILDIG);

  /* With flag, allows having no currency sign */
  CONVERT("11", NUMPRS_CURRENCY);
  EXPECT(2,NUMPRS_CURRENCY,0,2,0,0);
  EXPECT2(1,1);
  EXPECTRGB(2,FAILDIG);

  /* With flag, does not allow a lone currency sign */
  CONVERT("$", NUMPRS_CURRENCY);
  EXPECTFAIL;

  /* With flag, consumes currency sign */
  CONVERT("$11", NUMPRS_CURRENCY);
  EXPECT(2,NUMPRS_CURRENCY,NUMPRS_CURRENCY,3,0,0);
  EXPECT2(1,1);
  EXPECTRGB(2,FAILDIG);

  /* With flag, currency amounts cannot be in hexadecimal */
  CONVERT("$&ha", NUMPRS_HEX_OCT|NUMPRS_CURRENCY);
  EXPECTFAIL;

  CONVERT("&ha$", NUMPRS_HEX_OCT|NUMPRS_CURRENCY);
  EXPECT(1,NUMPRS_HEX_OCT|NUMPRS_CURRENCY,NUMPRS_HEX_OCT,3,4,0);
  EXPECTRGB(0,10);
  EXPECTRGB(1,FAILDIG);

  /* With flag, the sign cannot be repeated before the amount */
  CONVERT("$$11", NUMPRS_CURRENCY);
  EXPECTFAIL;

  /* With flag, but is allowed after the amount and can even be repeated! */
  CONVERT("$11$$", NUMPRS_CURRENCY|NUMPRS_USE_ALL);
  EXPECT(2,NUMPRS_CURRENCY|NUMPRS_USE_ALL,NUMPRS_CURRENCY,5,0,0);
  EXPECT2(1,1);
  EXPECTRGB(2,FAILDIG);

  /* With flag, the British Pound is not allowed "1L" */
  WCONVERT(L"\x31\xa3", NUMPRS_CURRENCY|NUMPRS_USE_ALL);
  EXPECTFAIL;

  /* With flag, minus can go after the currency sign */
  CONVERT("$-11", NUMPRS_CURRENCY|NUMPRS_LEADING_MINUS);
  EXPECT(2,NUMPRS_CURRENCY|NUMPRS_LEADING_MINUS,NUMPRS_CURRENCY|NUMPRS_LEADING_MINUS|NUMPRS_NEG,4,0,0);
  EXPECT2(1,1);
  EXPECTRGB(2,FAILDIG);

  /* With flag, or before */
  CONVERT("-$11", NUMPRS_CURRENCY|NUMPRS_LEADING_MINUS);
  EXPECT(2,NUMPRS_CURRENCY|NUMPRS_LEADING_MINUS,NUMPRS_CURRENCY|NUMPRS_LEADING_MINUS|NUMPRS_NEG,4,0,0);
  EXPECT2(1,1);
  EXPECTRGB(2,FAILDIG);

  for (i = 0; i < ARRAY_SIZE(spaces)-1; i++)
  {
      winetest_push_context("%d", i);

      /* With flag, no space is allowed after the currency sign */
      wsprintfW(wstr, L"$%c11", spaces[i]);
      WCONVERT(wstr, NUMPRS_CURRENCY|NUMPRS_USE_ALL);
      EXPECTFAIL;

      /* With flag, unless explicitly allowed before the digits */
      WCONVERT(wstr, NUMPRS_CURRENCY|NUMPRS_LEADING_WHITE);
      EXPECT(2,NUMPRS_CURRENCY|NUMPRS_LEADING_WHITE,NUMPRS_CURRENCY|NUMPRS_LEADING_WHITE,4,0,0);
      EXPECT2(1,1);
      EXPECTRGB(2,FAILDIG);

      /* With flag, no space is allowed before the trailing currency sign */
      wsprintfW(wstr, L"11%c$", spaces[i]);
      WCONVERT(wstr, NUMPRS_CURRENCY|NUMPRS_USE_ALL);
      EXPECTFAIL;

      /* With flag, even with thousands flag (see the French situation) */
      WCONVERT(wstr, NUMPRS_CURRENCY|NUMPRS_THOUSANDS|NUMPRS_USE_ALL);
      EXPECTFAIL;

      /* With flag, unless explicitly allowed */
      WCONVERT(wstr, NUMPRS_CURRENCY|NUMPRS_TRAILING_WHITE|NUMPRS_USE_ALL);
      EXPECT(2,NUMPRS_CURRENCY|NUMPRS_TRAILING_WHITE|NUMPRS_USE_ALL,NUMPRS_CURRENCY|NUMPRS_TRAILING_WHITE,4,0,0);
      EXPECT2(1,1);
      EXPECTRGB(2,FAILDIG);

      winetest_pop_context();
  }

  /* With flag only, doesn't consume decimal point */
  CONVERT("$11.1", NUMPRS_CURRENCY);
  EXPECT(2,NUMPRS_CURRENCY,NUMPRS_CURRENCY,3,0,0);
  EXPECT2(1,1);
  EXPECTRGB(2,FAILDIG);

  /* With flag and decimal flag, consumes decimal point and following digits */
  CONVERT("$11.1", NUMPRS_CURRENCY|NUMPRS_DECIMAL);
  EXPECT(3,NUMPRS_CURRENCY|NUMPRS_DECIMAL,NUMPRS_CURRENCY|NUMPRS_DECIMAL,5,0,-1);
  EXPECT2(1,1);
  EXPECTRGB(2,1);
  EXPECTRGB(3,FAILDIG);

  /* With flag, the currency cannot replace the decimal sign (see comment about
   * the Cape Verdean escudo).
   */
  CONVERT("1$99", NUMPRS_CURRENCY|NUMPRS_DECIMAL);
  EXPECT(1,NUMPRS_CURRENCY|NUMPRS_DECIMAL,NUMPRS_CURRENCY,2,0,0);
  EXPECT2(1,FAILDIG);

  /* Thousands flag can also be used with currency */
  CONVERT("$1,234", NUMPRS_CURRENCY|NUMPRS_THOUSANDS);
  EXPECT(4,NUMPRS_CURRENCY|NUMPRS_THOUSANDS,NUMPRS_CURRENCY|NUMPRS_THOUSANDS,6,0,0);
  EXPECT2(1,2);
  EXPECTRGB(2,3);
  EXPECTRGB(3,4);
  EXPECTRGB(4,FAILDIG);

  /* Thousands flag can also be used with currency and decimal numbers */
  CONVERT("$1,234.5", NUMPRS_CURRENCY|NUMPRS_THOUSANDS|NUMPRS_DECIMAL);
  EXPECT(5,NUMPRS_CURRENCY|NUMPRS_THOUSANDS|NUMPRS_DECIMAL,NUMPRS_CURRENCY|NUMPRS_THOUSANDS|NUMPRS_DECIMAL,8,0,-1);
  EXPECT2(1,2);
  EXPECTRGB(2,3);
  EXPECTRGB(3,4);
  EXPECTRGB(4,5);
  EXPECTRGB(5,FAILDIG);

  /** NUMPRS_DECIMAL **/

  /* With flag, consumes decimal point */
  CONVERT("1.1", NUMPRS_DECIMAL);
  EXPECT(2,NUMPRS_DECIMAL,NUMPRS_DECIMAL,3,0,-1);
  EXPECT2(1,1);
  EXPECTRGB(2,FAILDIG);

  /* With flag, consumes decimal point. Skipping the decimal part is not an error */
  CONVERT("1.", NUMPRS_DECIMAL);
  EXPECT(1,NUMPRS_DECIMAL,NUMPRS_DECIMAL,2,0,0);
  EXPECT2(1,FAILDIG);

  /* Skipping the integer part is not an error */
  CONVERT(".2", NUMPRS_DECIMAL);
  EXPECT(1,NUMPRS_DECIMAL,NUMPRS_DECIMAL,2,0,-1);
  EXPECT2(2,FAILDIG);

  /* Even zero gets an exponent (it sort of indicates 'precision') */
  CONVERT(".0", NUMPRS_DECIMAL);
  EXPECT(1,NUMPRS_DECIMAL,NUMPRS_DECIMAL,2,0,-1);
  EXPECT2(0,FAILDIG);

  CONVERT(".000", NUMPRS_DECIMAL);
  EXPECT(1,NUMPRS_DECIMAL,NUMPRS_DECIMAL,4,0,-3);
  EXPECTRGB(0,0);
  EXPECTRGB(3,FAILDIG);

  CONVERT("$.02", NUMPRS_CURRENCY|NUMPRS_DECIMAL);
  EXPECT(1,NUMPRS_CURRENCY|NUMPRS_DECIMAL,NUMPRS_CURRENCY|NUMPRS_DECIMAL,4,0,-2);
  EXPECT2(2,FAILDIG);

  CONVERT(".001", NUMPRS_DECIMAL);
  EXPECT(1,NUMPRS_DECIMAL,NUMPRS_DECIMAL,4,0,-3);
  EXPECT2(1,FAILDIG);

  CONVERT(".101", NUMPRS_DECIMAL);
  EXPECT(3,NUMPRS_DECIMAL,NUMPRS_DECIMAL,4,0,-3);
  EXPECT2(1,0);
  EXPECTRGB(2,1);
  EXPECTRGB(3,FAILDIG);

  CONVERT(".30", NUMPRS_DECIMAL);
  EXPECT(1,NUMPRS_DECIMAL,NUMPRS_DECIMAL,3,0,-1);
  /* See the NUMPRS_THOUSANDS comment about trailing zeroes */
  EXPECTRGB(0,3);
  EXPECTRGB(2,FAILDIG);

  /* But skipping both the integer and decimal part is not allowed */
  CONVERT(".", NUMPRS_DECIMAL);
  EXPECTFAIL;

  /* Consumes only one decimal point */
  CONVERT("1.1.", NUMPRS_DECIMAL);
  EXPECT(2,NUMPRS_DECIMAL,NUMPRS_DECIMAL,3,0,-1);
  EXPECT2(1,1);
  EXPECTRGB(2,FAILDIG);

  /* With flag, including if they are consecutive */
  CONVERT("1..1", NUMPRS_DECIMAL);
  EXPECT(1,NUMPRS_DECIMAL,NUMPRS_DECIMAL,2,0,0);
  EXPECT2(1,FAILDIG);

  /** NUMPRS_EXPONENT **/

  /* Without flag, doesn't consume exponent */
  CONVERT("1e1", 0);
  EXPECT(1,0,0,1,0,0);
  EXPECT2(1,FAILDIG);

  /* With flag, consumes exponent */
  CONVERT("1e1", NUMPRS_EXPONENT);
  EXPECT(1,NUMPRS_EXPONENT,NUMPRS_EXPONENT,3,0,1);
  EXPECT2(1,FAILDIG);

  /* Spaces are not allowed before the exponent */
  CONVERT("1 e1", NUMPRS_EXPONENT|NUMPRS_TRAILING_WHITE);
  EXPECT(1,NUMPRS_EXPONENT|NUMPRS_TRAILING_WHITE,NUMPRS_TRAILING_WHITE,2,0,0);
  EXPECT2(1,FAILDIG);

  /* With flag, incompatible with NUMPRS_HEX_OCT */
  CONVERT("&o1e1", NUMPRS_HEX_OCT|NUMPRS_EXPONENT);
  EXPECT(1,NUMPRS_HEX_OCT|NUMPRS_EXPONENT,NUMPRS_HEX_OCT,3,3,0);
  EXPECT2(1,FAILDIG);

  /* With flag, even if it sort of looks like an exponent */
  CONVERT("&h1e2", NUMPRS_HEX_OCT|NUMPRS_EXPONENT);
  EXPECT(3,NUMPRS_HEX_OCT|NUMPRS_EXPONENT,NUMPRS_HEX_OCT,5,4,0);
  EXPECT2(1,0xe);
  EXPECTRGB(2,2);
  EXPECTRGB(3,FAILDIG);

  /* Negative exponents are accepted without flags */
  CONVERT("1e-1", NUMPRS_EXPONENT);
  EXPECT(1,NUMPRS_EXPONENT,NUMPRS_EXPONENT,4,0,-1);
  EXPECT2(1,FAILDIG);

  /* As are positive exponents and leading exponent 0s */
  CONVERT("1e+01", NUMPRS_EXPONENT);
  EXPECT(1,NUMPRS_EXPONENT,NUMPRS_EXPONENT,5,0,1);
  EXPECT2(1,FAILDIG);

  /* The same for zero exponents */
  CONVERT("1e0", NUMPRS_EXPONENT);
  EXPECT(1,NUMPRS_EXPONENT,NUMPRS_EXPONENT,3,0,0);
  EXPECT2(1,FAILDIG);

  /* Sign on a zero exponent doesn't matter */
  CONVERT("1e+0", NUMPRS_EXPONENT);
  EXPECT(1,NUMPRS_EXPONENT,NUMPRS_EXPONENT,4,0,0);
  EXPECT2(1,FAILDIG);

  CONVERT("1e-0", NUMPRS_EXPONENT);
  EXPECT(1,NUMPRS_EXPONENT,NUMPRS_EXPONENT,4,0,0);
  EXPECT2(1,FAILDIG);

  /* Doesn't consume a real number exponent */
  CONVERT("1e1.", NUMPRS_EXPONENT);
  EXPECT(1,NUMPRS_EXPONENT,NUMPRS_EXPONENT,3,0,1);
  EXPECT2(1,FAILDIG);

  /* Powers of 10 are calculated from the position of any decimal point */
  CONVERT("1.5e20", NUMPRS_EXPONENT|NUMPRS_DECIMAL);
  EXPECT(2,NUMPRS_EXPONENT|NUMPRS_DECIMAL,NUMPRS_EXPONENT|NUMPRS_DECIMAL,6,0,19);
  EXPECT2(1,5);

  CONVERT("1.5e-20", NUMPRS_EXPONENT|NUMPRS_DECIMAL);
  EXPECT(2,NUMPRS_EXPONENT|NUMPRS_DECIMAL,NUMPRS_EXPONENT|NUMPRS_DECIMAL,7,0,-21);
  EXPECT2(1,5);

  /** NUMPRS_USE_ALL **/

  /* Flag expects all digits */
  CONVERT("0", NUMPRS_USE_ALL);
  EXPECT(1,NUMPRS_USE_ALL,0,1,0,0);
  EXPECT2(0,FAILDIG);

  /* Rejects anything trailing */
  CONVERT("0 ", NUMPRS_USE_ALL);
  EXPECTFAIL;
  EXPECT2(0,FAILDIG);

  /* Unless consumed by trailing flag */
  CONVERT("0 ", NUMPRS_USE_ALL|NUMPRS_TRAILING_WHITE);
  EXPECT(1,NUMPRS_USE_ALL|NUMPRS_TRAILING_WHITE,NUMPRS_TRAILING_WHITE,2,0,0);
  EXPECT2(0,FAILDIG);

  /** Combinations **/

  /* Leading whitespace and plus, doesn't consume trailing whitespace */
  CONVERT("+ 0 ", NUMPRS_LEADING_PLUS|NUMPRS_LEADING_WHITE);
  EXPECT(1,NUMPRS_LEADING_PLUS|NUMPRS_LEADING_WHITE,NUMPRS_LEADING_PLUS|NUMPRS_LEADING_WHITE,3,0,0);
  EXPECT2(0,FAILDIG);

  /* Order of whitespace and plus is unimportant */
  CONVERT(" +0", NUMPRS_LEADING_PLUS|NUMPRS_LEADING_WHITE);
  EXPECT(1,NUMPRS_LEADING_PLUS|NUMPRS_LEADING_WHITE,NUMPRS_LEADING_PLUS|NUMPRS_LEADING_WHITE,3,0,0);
  EXPECT2(0,FAILDIG);

  /* Leading whitespace can be repeated */
  CONVERT(" + 0", NUMPRS_LEADING_PLUS|NUMPRS_LEADING_WHITE);
  EXPECT(1,NUMPRS_LEADING_PLUS|NUMPRS_LEADING_WHITE,NUMPRS_LEADING_PLUS|NUMPRS_LEADING_WHITE,4,0,0);
  EXPECT2(0,FAILDIG);

  /* But plus/minus etc. cannot */
  CONVERT("+ +0", NUMPRS_LEADING_PLUS|NUMPRS_LEADING_WHITE);
  EXPECTFAIL;
  EXPECTRGB(0,FAILDIG);

  /* Inexact is not set if trailing zeros are removed */
  CONVERTN("10", 1, 0);
  EXPECT(1,0,0,2,0,1);
  EXPECT2(1,FAILDIG);

  /* Make sure a leading 0 is stripped but decimals after it get read */
  CONVERT("-0.51", NUMPRS_STD);
  EXPECT(2,NUMPRS_STD,NUMPRS_NEG|NUMPRS_DECIMAL|NUMPRS_LEADING_MINUS,5,0,-2);
  EXPECT2(5,1);

  /* Keep trailing zeros on whole number part of a decimal */
  CONVERT("10.1", NUMPRS_STD);
  EXPECT(3,NUMPRS_STD,NUMPRS_DECIMAL,4,0,-1);
  EXPECT2(1,0);
  EXPECTRGB(2,1);

  /* Zeros after decimal sign */
  CONVERT("0.01", NUMPRS_STD);
  EXPECT(1,NUMPRS_STD,NUMPRS_DECIMAL,4,0,-2);
  EXPECT2(1,FAILDIG);

  /* Trailing zeros after decimal part */
  CONVERT("0.10", NUMPRS_STD);
  EXPECT(1,NUMPRS_STD,NUMPRS_DECIMAL,4,0,-1);
  EXPECT2(1,0);

  /* Arabic numerals are not allowed "0" */
  WCONVERT(L"\x660", NUMPRS_STD);
  EXPECTFAIL;
}

static void test_VarParseNumFromStrFr(void)
{
  HRESULT hres;
  /* Test some aspects that are different in a non-English locale */
  LCID lcid = MAKELCID(MAKELANGID(LANG_FRENCH,SUBLANG_FRENCH),SORT_DEFAULT);
  NUMPARSE np;
  BYTE rgb[128];
  OLECHAR wstr[128];
  OLECHAR spaces[] = L" \xa0\f\n\r\t\v"; /* man isspace() */
  int i;

  /** White spaces **/

  for (i = 0; i < ARRAY_SIZE(spaces)-1; i++)
  {
    winetest_push_context("%d", i);

    /* Leading spaces must be explicitly allowed */
    wsprintfW(wstr, L"%c2", spaces[i]);
    WCONVERT(wstr, NUMPRS_USE_ALL);
    EXPECTFAIL;

    WCONVERT(wstr, NUMPRS_LEADING_WHITE|NUMPRS_USE_ALL);
    EXPECT(1,NUMPRS_LEADING_WHITE|NUMPRS_USE_ALL,NUMPRS_LEADING_WHITE,2,0,0);
    EXPECT2(2,FAILDIG);

    /* But trailing spaces... */
    wsprintfW(wstr, L"3%c", spaces[i]);
    WCONVERT(wstr, NUMPRS_TRAILING_WHITE|NUMPRS_USE_ALL);
    if (spaces[i] == ' ' || spaces[i] == 0xa0 /* non-breaking space */)
    {
      /* Spaces aliased to the thousands separator are never allowed! */
      EXPECTFAIL;
    }
    else
    {
      /* The others behave normally */
      EXPECT(1,NUMPRS_TRAILING_WHITE|NUMPRS_USE_ALL,NUMPRS_TRAILING_WHITE,2,0,0);
      EXPECT2(3,FAILDIG);
    }

    WCONVERT(wstr, NUMPRS_THOUSANDS|NUMPRS_USE_ALL);
    if (spaces[i] == ' ' || spaces[i] == 0xa0 /* non-breaking space */)
    {
      /* Trailing thousands separators are allowed as usual */
      EXPECT(1,NUMPRS_THOUSANDS|NUMPRS_USE_ALL,NUMPRS_THOUSANDS,2,0,0);
      EXPECT2(3,FAILDIG);
    }
    else
    {
      /* But not other spaces */
      EXPECTFAIL;
    }

    /* No space of any type is allowed before the exponent... */
    wsprintfW(wstr, L"1%ce1", spaces[i]);
    WCONVERT(wstr, NUMPRS_EXPONENT|NUMPRS_TRAILING_WHITE|NUMPRS_USE_ALL);
    EXPECTFAIL;

    winetest_pop_context();
  }


  /** NUMPRS_PARENS **/

  /* With flag, Number in parens made negative and parens consumed */
  CONVERT("(0)", NUMPRS_PARENS);
  EXPECT(1,NUMPRS_PARENS,NUMPRS_NEG|NUMPRS_PARENS,3,0,0);
  EXPECT2(0,FAILDIG);

  /** NUMPRS_THOUSANDS **/

  for (i = 0; i < ARRAY_SIZE(spaces)-1; i++)
  {
    winetest_push_context("%d", i);

    /* With flag, thousands separator and following digits consumed */
    wsprintfW(wstr, L"1%c000", spaces[i]);
    WCONVERT(wstr, NUMPRS_THOUSANDS|NUMPRS_USE_ALL);
    if (spaces[i] == ' ' || spaces[i] == 0xa0 /* non-breaking space */)
    {
      /* Non-breaking space and regular spaces work */
      EXPECT(1,NUMPRS_THOUSANDS|NUMPRS_USE_ALL,NUMPRS_THOUSANDS,5,0,3);
      EXPECTRGB(0,1); /* Don't test extra digits, see "1,000" test */
      EXPECTRGB(4,FAILDIG);
    }
    else
    {
      /* But not other spaces */
      EXPECTFAIL;
    }

    winetest_pop_context();
  }

  /* With flag and decimal point, thousands sep. but not decimals consumed */
  CONVERT("1 001,0", NUMPRS_THOUSANDS);
  EXPECT(4,NUMPRS_THOUSANDS,NUMPRS_THOUSANDS,5,0,0);
  EXPECT2(1,0);
  EXPECTRGB(2,0);
  EXPECTRGB(3,1);
  EXPECTRGB(4,FAILDIG);

  /* With flag, consecutive thousands separators are allowed */
  CONVERT("1  000", NUMPRS_THOUSANDS|NUMPRS_USE_ALL);
  EXPECT(1,NUMPRS_THOUSANDS|NUMPRS_USE_ALL,NUMPRS_THOUSANDS,6,0,3);
  EXPECTRGB(0,1); /* Don't test extra digits, see "1,000" test */
  EXPECTRGB(4,FAILDIG);

  /* With flag, thousands separators can be sprinkled at random */
  CONVERT("1 00 0  ", NUMPRS_THOUSANDS|NUMPRS_USE_ALL);
  EXPECT(1,NUMPRS_THOUSANDS|NUMPRS_USE_ALL,NUMPRS_THOUSANDS,8,0,3);
  EXPECTRGB(0,1); /* Don't test extra digits, see "1,000" test */
  EXPECTRGB(4,FAILDIG);

  /* With flag, but leading thousands separators are not allowed */
  CONVERT(" 1 000", NUMPRS_THOUSANDS);
  EXPECTFAIL;

  /* With flag, thousands separator not needed but still reported */
  CONVERT("1 ", NUMPRS_THOUSANDS|NUMPRS_USE_ALL);
  EXPECT(1,NUMPRS_THOUSANDS|NUMPRS_USE_ALL,NUMPRS_THOUSANDS,2,0,0);
  EXPECT2(1,FAILDIG);


  /** NUMPRS_CURRENCY **/

  /* With flag, consumes currency sign "E12" */
  WCONVERT(L"\x20ac\x31\x32", NUMPRS_CURRENCY);
  EXPECT(2,NUMPRS_CURRENCY,NUMPRS_CURRENCY,3,0,0);
  EXPECT2(1,2);
  EXPECTRGB(2,FAILDIG);

  /* With flag, consumes all currency signs! "E12EE" */
  WCONVERT(L"\x20ac\x31\x32\x20ac\x20ac", NUMPRS_CURRENCY|NUMPRS_USE_ALL);
  EXPECT(2,NUMPRS_CURRENCY|NUMPRS_USE_ALL,NUMPRS_CURRENCY,5,0,0);
  EXPECT2(1,2);
  EXPECTRGB(2,FAILDIG);

  /* The presence of a trailing currency sign changes nothing for spaces */
  for (i = 0; i < ARRAY_SIZE(spaces)-1; i++)
  {
    winetest_push_context("%d", i);

    /* With flag, no space is allowed before the currency sign "12 E" */
    wsprintfW(wstr, L"12%c\x20ac", spaces[i]);
    WCONVERT(wstr, NUMPRS_CURRENCY|NUMPRS_USE_ALL);
    EXPECTFAIL;

    /* With flag, even if explicitly allowed "12 E" */
    WCONVERT(wstr, NUMPRS_CURRENCY|NUMPRS_TRAILING_WHITE|NUMPRS_USE_ALL);
    if (spaces[i] == ' ' || spaces[i] == 0xa0 /* non-breaking space */)
    {
      /* Spaces aliased to thousands separator are never allowed! */
      EXPECTFAIL;
    }
    else
    {
      /* The others behave normally */
      EXPECT(2,NUMPRS_CURRENCY|NUMPRS_TRAILING_WHITE|NUMPRS_USE_ALL,NUMPRS_CURRENCY|NUMPRS_TRAILING_WHITE,4,0,0);
      EXPECT2(1,2);
      EXPECTRGB(2,FAILDIG);
    }

    WCONVERT(wstr, NUMPRS_CURRENCY|NUMPRS_THOUSANDS|NUMPRS_USE_ALL);
    if (spaces[i] == ' ' || spaces[i] == 0xa0 /* non-breaking space */)
    {
      /* Spaces aliased to thousands separator are never allowed! */
      EXPECT(2,NUMPRS_CURRENCY|NUMPRS_THOUSANDS|NUMPRS_USE_ALL,NUMPRS_CURRENCY|NUMPRS_THOUSANDS,4,0,0);
      EXPECT2(1,2);
      EXPECTRGB(2,FAILDIG);
    }
    else
    {
      /* The others behave normally */
      EXPECTFAIL;
    }

    winetest_pop_context();
  }

  /* With flag only, doesn't consume decimal point */
  WCONVERT(L"12,1\x20ac", NUMPRS_CURRENCY);
  EXPECT(2,NUMPRS_CURRENCY,0,2,0,0);
  EXPECT2(1,2);
  EXPECTRGB(2,FAILDIG);

  /* The three-letter Euro abbreviation is not allowed */
  WCONVERT(L"12EUR", NUMPRS_CURRENCY);
  EXPECT(2,NUMPRS_CURRENCY,0,2,0,0);
  EXPECT2(1,2);
  EXPECTRGB(2,FAILDIG);

  /* With flag and decimal flag, consumes decimal point and following digits */
  WCONVERT(L"12,1\x20ac", NUMPRS_CURRENCY|NUMPRS_DECIMAL|NUMPRS_USE_ALL);
  EXPECT(3,NUMPRS_CURRENCY|NUMPRS_DECIMAL|NUMPRS_USE_ALL,NUMPRS_CURRENCY|NUMPRS_DECIMAL,5,0,-1);
  EXPECT2(1,2);
  EXPECTRGB(2,1);
  EXPECTRGB(3,FAILDIG);

  /* Thousands flag can also be used with currency */
  WCONVERT(L"1 234,5 \x20ac", NUMPRS_CURRENCY|NUMPRS_THOUSANDS|NUMPRS_DECIMAL|NUMPRS_USE_ALL);
  EXPECT(5,NUMPRS_CURRENCY|NUMPRS_THOUSANDS|NUMPRS_DECIMAL|NUMPRS_USE_ALL,NUMPRS_CURRENCY|NUMPRS_THOUSANDS|NUMPRS_DECIMAL,9,0,-1);
  EXPECT2(1,2);
  EXPECTRGB(2,3);
  EXPECTRGB(3,4);
  EXPECTRGB(4,5);
  EXPECTRGB(5,FAILDIG);


  /** NUMPRS_DECIMAL **/

  /* With flag, consumes decimal point */
  CONVERT("1,2", NUMPRS_DECIMAL);
  EXPECT(2,NUMPRS_DECIMAL,NUMPRS_DECIMAL,3,0,-1);
  EXPECT2(1,2);
  EXPECTRGB(2,FAILDIG);

  /* With flag, but not regular point */
  CONVERT("1.2", NUMPRS_DECIMAL);
  EXPECT(1,NUMPRS_DECIMAL,0,1,0,0);
  EXPECT2(1,FAILDIG);

  /* The integer part can still be omitted */
  CONVERT(",2", NUMPRS_DECIMAL);
  EXPECT(1,NUMPRS_DECIMAL,NUMPRS_DECIMAL,2,0,-1);
  EXPECT2(2,FAILDIG);

  CONVERT(".2", NUMPRS_DECIMAL);
  EXPECTFAIL;
}

static void test_VarParseNumFromStrMisc(void)
{
  HRESULT hres;
  LCID lcid;
  NUMPARSE np;
  BYTE rgb[128];
  OLECHAR currency[8], t1000[8], mont1000[8], dec[8], mondec[8];

  /* Customize the regional settings to perform extra tests */

  if (GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SCURRENCY, currency, ARRAY_SIZE(currency)) &&
      GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, t1000, ARRAY_SIZE(t1000)) &&
      GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SMONTHOUSANDSEP, mont1000, ARRAY_SIZE(mont1000)) &&
      GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, dec, ARRAY_SIZE(dec)) &&
      GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SMONDECIMALSEP, mondec, ARRAY_SIZE(mondec)))
  {
      /* Start from a known configuration */
      SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SCURRENCY, L"$");
      SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, L",");
      SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SMONTHOUSANDSEP, L",");
      SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, L".");
      SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SMONDECIMALSEP, L".");

      /* SCURRENCY defaults to '$' */
      SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SCURRENCY, L"");
      hres = wconvert_str(L"$1", ARRAY_SIZE(rgb), NUMPRS_CURRENCY, &np, rgb, LOCALE_USER_DEFAULT, 0);
      EXPECT(1,NUMPRS_CURRENCY,NUMPRS_CURRENCY,2,0,0);
      EXPECT2(1,FAILDIG);
      SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SCURRENCY, L"$");

      /* STHOUSAND has no default */
      SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SMONTHOUSANDSEP, L"~");
      SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, L"");
      hres = wconvert_str(L"1,000", ARRAY_SIZE(rgb), NUMPRS_THOUSANDS|NUMPRS_USE_ALL, &np, rgb, LOCALE_USER_DEFAULT, 0);
      EXPECTFAIL;
      SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, L"~");

      /* But SMONTHOUSANDSEP defaults to ','! */
      SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SMONTHOUSANDSEP, L"");
      hres = wconvert_str(L"$1,000", ARRAY_SIZE(rgb), NUMPRS_THOUSANDS|NUMPRS_CURRENCY, &np, rgb, LOCALE_USER_DEFAULT, 0);
      EXPECT(1,NUMPRS_THOUSANDS|NUMPRS_CURRENCY,NUMPRS_THOUSANDS|NUMPRS_CURRENCY,6,0,3);
      EXPECTRGB(0,1); /* Don't test extra digits, see "1,000" test */
      EXPECTRGB(4,FAILDIG);
      SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, L",");
      SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SMONTHOUSANDSEP, L",");

      /* SDECIMAL defaults to '.' */
      SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SMONDECIMALSEP, L"~");
      SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, L"");
      hres = wconvert_str(L"4.2", ARRAY_SIZE(rgb), NUMPRS_DECIMAL, &np, rgb, LOCALE_USER_DEFAULT, 0);
      EXPECT(2,NUMPRS_DECIMAL,NUMPRS_DECIMAL,3,0,-1);
      EXPECT2(4,2);
      EXPECTRGB(2,FAILDIG);
      SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, L"~");

      /* But SMONDECIMALSEP has no default! */
      SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SMONDECIMALSEP, L"");
      hres = wconvert_str(L"3.9", ARRAY_SIZE(rgb), NUMPRS_DECIMAL|NUMPRS_CURRENCY|NUMPRS_USE_ALL, &np, rgb, LOCALE_USER_DEFAULT, 0);
      EXPECTFAIL;
      SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, L".");
      SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SMONDECIMALSEP, L".");

      /* Non-breaking spaces are not allowed if sThousand is a regular space */
      SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, L" ");

      hres = wconvert_str(L"1 000", ARRAY_SIZE(rgb), NUMPRS_THOUSANDS, &np, rgb, LOCALE_USER_DEFAULT, 0);
      EXPECT(1,NUMPRS_THOUSANDS,NUMPRS_THOUSANDS,5,0,3);
      EXPECTRGB(0,1); /* Don't test extra digits, see "1,000" test */
      EXPECTRGB(4,FAILDIG);

      hres = wconvert_str(L"1\xa0\x30\x30\x30", ARRAY_SIZE(rgb), NUMPRS_THOUSANDS|NUMPRS_USE_ALL, &np, rgb, LOCALE_USER_DEFAULT, 0);
      EXPECTFAIL;


      /* Show that NUMPRS_THOUSANDS activates sThousand and that
       * NUMPRS_THOUSANDS+NUMPRS_CURRENCY activates sMonThousandSep
       * whether a currency sign is present or not. Also the presence of
       * sMonThousandSep flags the value as being a currency.
       */
      SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, L"|");
      SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SMONTHOUSANDSEP, L" ");

      hres = wconvert_str(L"1|000", ARRAY_SIZE(rgb), NUMPRS_THOUSANDS, &np, rgb, LOCALE_USER_DEFAULT, 0);
      EXPECT(1,NUMPRS_THOUSANDS,NUMPRS_THOUSANDS,5,0,3);
      EXPECTRGB(0,1); /* Don't test extra digits, see "1,000" test */
      EXPECTRGB(4,FAILDIG);

      hres = wconvert_str(L"1 000", ARRAY_SIZE(rgb), NUMPRS_THOUSANDS|NUMPRS_USE_ALL, &np, rgb, LOCALE_USER_DEFAULT, 0);
      EXPECTFAIL;

      hres = wconvert_str(L"1|000", ARRAY_SIZE(rgb), NUMPRS_THOUSANDS|NUMPRS_CURRENCY, &np, rgb, LOCALE_USER_DEFAULT, 0);
      EXPECT(1,NUMPRS_THOUSANDS|NUMPRS_CURRENCY,NUMPRS_THOUSANDS,5,0,3);
      EXPECTRGB(0,1); /* Don't test extra digits, see "1,000" test */
      EXPECTRGB(4,FAILDIG);

      hres = wconvert_str(L"1 000", ARRAY_SIZE(rgb), NUMPRS_THOUSANDS|NUMPRS_CURRENCY|NUMPRS_USE_ALL, &np, rgb, LOCALE_USER_DEFAULT, 0);
      EXPECT(1,NUMPRS_THOUSANDS|NUMPRS_CURRENCY|NUMPRS_USE_ALL,NUMPRS_THOUSANDS|NUMPRS_CURRENCY,5,0,3);
      EXPECTRGB(0,1); /* Don't test extra digits, see "1,000" test */
      EXPECTRGB(4,FAILDIG);


      /* Leading sMonThousandSep are not allowed (same as sThousand) */
      hres = wconvert_str(L" 1 000", ARRAY_SIZE(rgb), NUMPRS_THOUSANDS|NUMPRS_CURRENCY|NUMPRS_USE_ALL, &np, rgb, LOCALE_USER_DEFAULT, 0);
      EXPECTFAIL;

      /* But trailing ones are allowed (same as sThousand) */
      hres = wconvert_str(L"1 000 ", ARRAY_SIZE(rgb), NUMPRS_THOUSANDS|NUMPRS_CURRENCY|NUMPRS_USE_ALL, &np, rgb, LOCALE_USER_DEFAULT, 0);
      EXPECT(1,NUMPRS_THOUSANDS|NUMPRS_CURRENCY|NUMPRS_USE_ALL,NUMPRS_THOUSANDS|NUMPRS_CURRENCY,6,0,3);
      EXPECTRGB(0,1); /* Don't test extra digits, see "1,000" test */
      EXPECTRGB(4,FAILDIG);

      /* And they break NUMPRS_TRAILING_WHITE (same as sThousand) */
      hres = wconvert_str(L"1000 ", ARRAY_SIZE(rgb), NUMPRS_TRAILING_WHITE|NUMPRS_CURRENCY|NUMPRS_USE_ALL, &np, rgb, LOCALE_USER_DEFAULT, 0);
      EXPECTFAIL;


      /* NUMPRS_CURRENCY is not enough for sMonThousandSep */
      hres = wconvert_str(L"1 000", ARRAY_SIZE(rgb), NUMPRS_CURRENCY|NUMPRS_USE_ALL, &np, rgb, LOCALE_USER_DEFAULT, 0);
      EXPECTFAIL;


      /* Even with a currency sign, the regular thousands separator works */
      SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SCURRENCY, L"$");
      /* Make sure SMONDECIMALSEP is not the currency sign (see the
       * Cape Verdean escudo comment).
       */
      SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SMONDECIMALSEP, L"/");
      hres = wconvert_str(L"$1|000", ARRAY_SIZE(rgb), NUMPRS_THOUSANDS|NUMPRS_CURRENCY|NUMPRS_USE_ALL, &np, rgb, LOCALE_USER_DEFAULT, 0);
      EXPECT(1,NUMPRS_THOUSANDS|NUMPRS_CURRENCY|NUMPRS_USE_ALL,NUMPRS_THOUSANDS|NUMPRS_CURRENCY,6,0,3);
      EXPECTRGB(0,1); /* Don't test extra digits, see "1,000" test */
      EXPECTRGB(4,FAILDIG);

      /* Mixing both thousands separators is allowed */
      hres = wconvert_str(L"1 000|000", ARRAY_SIZE(rgb), NUMPRS_THOUSANDS|NUMPRS_CURRENCY|NUMPRS_USE_ALL, &np, rgb, LOCALE_USER_DEFAULT, 0);
      EXPECT(1,NUMPRS_THOUSANDS|NUMPRS_CURRENCY|NUMPRS_USE_ALL,NUMPRS_THOUSANDS|NUMPRS_CURRENCY,9,0,6);
      EXPECTRGB(0,1); /* Don't test extra digits, see "1,000" test */
      EXPECTRGB(7,FAILDIG);


      /* SMONTHOUSANDSEP does not consider regular spaces to be equivalent to
       * non-breaking spaces!
       */
      SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SMONTHOUSANDSEP, L"\xa0");
      hres = wconvert_str(L"1 000", ARRAY_SIZE(rgb), NUMPRS_THOUSANDS|NUMPRS_CURRENCY|NUMPRS_USE_ALL, &np, rgb, LOCALE_USER_DEFAULT, 0);
      EXPECTFAIL;

      SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, L"\xa0");
      hres = wconvert_str(L"1 000", ARRAY_SIZE(rgb), NUMPRS_THOUSANDS|NUMPRS_CURRENCY|NUMPRS_USE_ALL, &np, rgb, LOCALE_USER_DEFAULT, 0);
      EXPECT(1,NUMPRS_THOUSANDS|NUMPRS_CURRENCY|NUMPRS_USE_ALL,NUMPRS_THOUSANDS,5,0,3);
      EXPECTRGB(0,1); /* Don't test extra digits, see "1,000" test */
      EXPECTRGB(4,FAILDIG);


      /* Regular thousands separators also have precedence over the currency ones */
      hres = wconvert_str(L"1\xa0\x30\x30\x30", ARRAY_SIZE(rgb), NUMPRS_THOUSANDS|NUMPRS_CURRENCY|NUMPRS_USE_ALL, &np, rgb, LOCALE_USER_DEFAULT, 0);
      EXPECT(1,NUMPRS_THOUSANDS|NUMPRS_CURRENCY|NUMPRS_USE_ALL,NUMPRS_THOUSANDS,5,0,3);
      EXPECTRGB(0,1); /* Don't test extra digits, see "1,000" test */
      EXPECTRGB(4,FAILDIG);


      /* Show that the decimal separator masks the thousands one in all
       * positions, sometimes even without NUMPRS_DECIMAL.
       */
      SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, L",");
      SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SMONTHOUSANDSEP, L"~");
      SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, L",");
      SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SMONDECIMALSEP, L"~");

      hres = wconvert_str(L",1", ARRAY_SIZE(rgb), NUMPRS_THOUSANDS|NUMPRS_DECIMAL, &np, rgb, LOCALE_USER_DEFAULT, 0);
      EXPECT(1,NUMPRS_THOUSANDS|NUMPRS_DECIMAL,NUMPRS_DECIMAL,2,0,-1);
      EXPECT2(1,FAILDIG);

      hres = wconvert_str(L"1,000", ARRAY_SIZE(rgb), NUMPRS_THOUSANDS|NUMPRS_USE_ALL, &np, rgb, LOCALE_USER_DEFAULT, 0);
      EXPECTFAIL;

      hres = wconvert_str(L"1,", ARRAY_SIZE(rgb), NUMPRS_THOUSANDS|NUMPRS_DECIMAL|NUMPRS_USE_ALL, &np, rgb, LOCALE_USER_DEFAULT, 0);
      EXPECT(1,NUMPRS_THOUSANDS|NUMPRS_DECIMAL|NUMPRS_USE_ALL,NUMPRS_DECIMAL,2,0,0);
      EXPECT2(1,FAILDIG);

      /* But not for their monetary equivalents */
      hres = wconvert_str(L"~1", ARRAY_SIZE(rgb), NUMPRS_THOUSANDS|NUMPRS_DECIMAL|NUMPRS_CURRENCY, &np, rgb, LOCALE_USER_DEFAULT, 0);
      EXPECT(1,NUMPRS_THOUSANDS|NUMPRS_DECIMAL|NUMPRS_CURRENCY,NUMPRS_DECIMAL|NUMPRS_CURRENCY,2,0,-1);
      EXPECT2(1,FAILDIG);

      hres = wconvert_str(L"1~", ARRAY_SIZE(rgb), NUMPRS_THOUSANDS|NUMPRS_DECIMAL|NUMPRS_CURRENCY|NUMPRS_USE_ALL, &np, rgb, LOCALE_USER_DEFAULT, 0);
      EXPECT(1,NUMPRS_THOUSANDS|NUMPRS_DECIMAL|NUMPRS_CURRENCY|NUMPRS_USE_ALL,NUMPRS_THOUSANDS|NUMPRS_CURRENCY,2,0,0);
      EXPECT2(1,FAILDIG);


      /* Only the first sThousand character is used (sigh of relief) */
      SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, L" \xa0");
      SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SMONTHOUSANDSEP, L" \xa0");

      hres = wconvert_str(L"1 000", ARRAY_SIZE(rgb), NUMPRS_THOUSANDS|NUMPRS_USE_ALL, &np, rgb, LOCALE_USER_DEFAULT, 0);
      EXPECT(1,NUMPRS_THOUSANDS|NUMPRS_USE_ALL,NUMPRS_THOUSANDS,5,0,3);
      EXPECTRGB(0,1); /* Don't test extra digits, see "1,000" test */
      EXPECTRGB(4,FAILDIG);

      hres = wconvert_str(L"1\xa0\x30\x30\x30", ARRAY_SIZE(rgb), NUMPRS_THOUSANDS|NUMPRS_CURRENCY|NUMPRS_USE_ALL, &np, rgb, LOCALE_USER_DEFAULT, 0);
      EXPECTFAIL;

      hres = wconvert_str(L"1 \xa0\x30\x30\x30", ARRAY_SIZE(rgb), NUMPRS_THOUSANDS|NUMPRS_CURRENCY|NUMPRS_USE_ALL, &np, rgb, LOCALE_USER_DEFAULT, 0);
      EXPECTFAIL;


      /* Show that the currency decimal separator is active even without
       * NUMPRS_CURRENCY.
       */
      SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, L".");
      SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SMONDECIMALSEP, L",");

      hres = wconvert_str(L"1.2", ARRAY_SIZE(rgb), NUMPRS_DECIMAL|NUMPRS_USE_ALL, &np, rgb, LOCALE_USER_DEFAULT, 0);
      EXPECT(2,NUMPRS_DECIMAL|NUMPRS_USE_ALL,NUMPRS_DECIMAL,3,0,-1);
      EXPECT2(1,2);
      EXPECTRGB(2,FAILDIG);

      hres = wconvert_str(L"1,2", ARRAY_SIZE(rgb), NUMPRS_DECIMAL, &np, rgb, LOCALE_USER_DEFAULT, 0);
      EXPECT(2,NUMPRS_DECIMAL,NUMPRS_DECIMAL|NUMPRS_CURRENCY,3,0,-1);
      EXPECT2(1,2);
      EXPECTRGB(2,FAILDIG);

      hres = wconvert_str(L"1.2", ARRAY_SIZE(rgb), NUMPRS_DECIMAL|NUMPRS_CURRENCY|NUMPRS_USE_ALL, &np, rgb, LOCALE_USER_DEFAULT, 0);
      EXPECT(2,NUMPRS_DECIMAL|NUMPRS_CURRENCY|NUMPRS_USE_ALL,NUMPRS_DECIMAL,3,0,-1);
      EXPECT2(1,2);
      EXPECTRGB(2,FAILDIG);

      hres = wconvert_str(L"1,2", ARRAY_SIZE(rgb), NUMPRS_DECIMAL|NUMPRS_CURRENCY|NUMPRS_USE_ALL, &np, rgb, LOCALE_USER_DEFAULT, 0);
      EXPECT(2,NUMPRS_DECIMAL|NUMPRS_CURRENCY|NUMPRS_USE_ALL,NUMPRS_DECIMAL|NUMPRS_CURRENCY,3,0,-1);
      EXPECT2(1,2);
      EXPECTRGB(2,FAILDIG);

      hres = wconvert_str(L"1.2,3", ARRAY_SIZE(rgb), NUMPRS_DECIMAL|NUMPRS_CURRENCY, &np, rgb, LOCALE_USER_DEFAULT, 0);
      EXPECT(2,NUMPRS_DECIMAL|NUMPRS_CURRENCY,NUMPRS_DECIMAL,3,0,-1);
      EXPECT2(1,2);
      EXPECTRGB(2,FAILDIG);

      hres = wconvert_str(L"1,2.3", ARRAY_SIZE(rgb), NUMPRS_DECIMAL, &np, rgb, LOCALE_USER_DEFAULT, 0);
      EXPECT(2,NUMPRS_DECIMAL,NUMPRS_DECIMAL|NUMPRS_CURRENCY,3,0,-1);
      EXPECT2(1,2);
      EXPECTRGB(2,FAILDIG);


      /* In some locales the decimal separator is the currency sign.
       * For instance the Cape Verdean escudo.
       */
      SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SMONDECIMALSEP, L"$");
      hres = wconvert_str(L"1$99", ARRAY_SIZE(rgb), NUMPRS_DECIMAL|NUMPRS_USE_ALL, &np, rgb, LOCALE_USER_DEFAULT, 0);
      EXPECT(3,NUMPRS_DECIMAL|NUMPRS_USE_ALL,NUMPRS_DECIMAL|NUMPRS_CURRENCY,4,0,-2);
      EXPECT2(1,9);
      EXPECTRGB(2,9);
      EXPECTRGB(3,FAILDIG);


      /* Restore all the settings */
      ok(SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SCURRENCY, currency), "Restoring SCURRENCY failed\n");
      ok(SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, t1000), "Restoring STHOUSAND failed\n");
      ok(SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SMONTHOUSANDSEP, mont1000), "Restoring SMONTHOUSANDSEP failed\n");
      ok(SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, dec), "Restoring SDECIMAL failed\n");
      ok(SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SMONDECIMALSEP, mondec), "Restoring SMONDECIMALSEP failed\n");
  }


  /* Test currencies of various lengths */

  /* 2 Polish zloty */
  lcid = MAKELCID(MAKELANGID(LANG_POLISH,SUBLANG_POLISH_POLAND),SORT_DEFAULT);
  WCONVERT(L"z\x142\x32", NUMPRS_CURRENCY|NUMPRS_USE_ALL);
  EXPECT(1,NUMPRS_CURRENCY|NUMPRS_USE_ALL,NUMPRS_CURRENCY,3,0,0);
  EXPECT2(2,FAILDIG);

  /* Multi-character currencies can be repeated too "zl2zlzl" */
  WCONVERT(L"z\x142\x32z\x142z\x142", NUMPRS_CURRENCY|NUMPRS_USE_ALL);
  EXPECT(1,NUMPRS_CURRENCY|NUMPRS_USE_ALL,NUMPRS_CURRENCY,7,0,0);
  EXPECT2(2,FAILDIG);

  lcid = MAKELCID(MAKELANGID(LANG_FRENCH,SUBLANG_FRENCH_SWISS),SORT_DEFAULT);
  WCONVERT(L"3CHF", NUMPRS_CURRENCY|NUMPRS_USE_ALL);
  /* Windows <= 8.1 uses an old currency symbol: "fr. 5" */
  ok(hres == S_OK || broken(hres == DISP_E_TYPEMISMATCH), "returned %08lx\n", hres);
  if (hres == S_OK)
  {
    EXPECT(1,NUMPRS_CURRENCY|NUMPRS_USE_ALL,NUMPRS_CURRENCY,4,0,0);
    EXPECT2(3,FAILDIG);
  }

  /* 5 Moroccan dirham */
  lcid = MAKELCID(MAKELANGID(LANG_ARABIC,SUBLANG_ARABIC_MOROCCO),SORT_DEFAULT);
  WCONVERT(L"5\x62f.\x645.\x200f", NUMPRS_CURRENCY|NUMPRS_USE_ALL);
  /* Windows 8.1 incorrectly doubles the right-to-left mark:
   * "\x62f.\x645.\x200f\x200f 5"
   */
  ok(hres == S_OK || broken(hres == DISP_E_TYPEMISMATCH), "returned %08lx\n", hres);
  if (hres == S_OK)
  {
    EXPECT(1,NUMPRS_CURRENCY|NUMPRS_USE_ALL,NUMPRS_CURRENCY,6,0,0);
    EXPECT2(5,FAILDIG);
  }


  /* Test Arabic numerals in an Arabic locale */

  lcid = MAKELCID(MAKELANGID(LANG_ARABIC,SUBLANG_ARABIC_MOROCCO),SORT_DEFAULT);
  WCONVERT(L"\x660", NUMPRS_STD);
  EXPECTFAIL;
}

/* Macros for converting and testing the result of VarNumFromParseNum */
#define SETRGB(indx,val) if (!indx) memset(rgb, FAILDIG, sizeof(rgb)); rgb[indx] = val
#undef CONVERT
#define CONVERT(a,b,c,d,e,f,bits) \
    np.cDig = (a); np.dwInFlags = (b); np.dwOutFlags = (c); np.cchUsed = (d); \
    np.nBaseShift = (e); np.nPwr10 = (f); hres = VarNumFromParseNum(&np, rgb, bits, &vOut)
static const char *szFailOverflow = "Expected overflow, hres = %08x\n";
#define EXPECT_OVERFLOW ok(hres == DISP_E_OVERFLOW, szFailOverflow, hres)
static const char *szFailOk = "Call failed, hres = %08x\n";
#define EXPECT_OK ok(hres == S_OK, szFailOk, hres); \
  if (hres == S_OK)
#define EXPECT_TYPE(typ) ok(V_VT(&vOut) == typ,"Expected Type = " #typ ", got %d\n", V_VT(&vOut))
#define EXPECT_I1(val) EXPECT_OK { EXPECT_TYPE(VT_I1); \
  ok(V_I1(&vOut) == val, "Expected i1 = %d, got %d\n", (signed char)val, V_I1(&vOut)); }
#define EXPECT_UI1(val) EXPECT_OK { EXPECT_TYPE(VT_UI1); \
  ok(V_UI1(&vOut) == val, "Expected ui1 = %d, got %d\n", (BYTE)val, V_UI1(&vOut)); }
#define EXPECT_I2(val) EXPECT_OK { EXPECT_TYPE(VT_I2); \
  ok(V_I2(&vOut) == val, "Expected i2 = %d, got %d\n", (SHORT)val, V_I2(&vOut)); }
#define EXPECT_UI2(val) EXPECT_OK { EXPECT_TYPE(VT_UI2); \
  ok(V_UI2(&vOut) == val, "Expected ui2 = %d, got %d\n", (USHORT)val, V_UI2(&vOut)); }
#define EXPECT_I4(val) EXPECT_OK { EXPECT_TYPE(VT_I4); \
  ok(V_I4(&vOut) == val, "Expected i4 = %ld, got %ld\n", (LONG)val, V_I4(&vOut)); }
#define EXPECT_UI4(val) EXPECT_OK { EXPECT_TYPE(VT_UI4); \
  ok(V_UI4(&vOut) == val, "Expected ui4 = %d, got %d\n", (ULONG)val, V_UI4(&vOut)); }
#define EXPECT_I8(val) EXPECT_OK { EXPECT_TYPE(VT_I8); \
  ok(V_I8(&vOut) == val, "Expected i8 = %#I64x, got %#I64x\n", (LONG64)val, V_I8(&vOut)); }
#define EXPECT_UI8(val) EXPECT_OK { EXPECT_TYPE(VT_UI8); \
  ok(V_UI8(&vOut) == val, "Expected ui8 = %#I64x, got %#I64x\n", (ULONG64)val, V_UI8(&vOut); }
#define EXPECT_R4(val) EXPECT_OK { EXPECT_TYPE(VT_R4); \
  ok(V_R4(&vOut) == val, "Expected r4 = %f, got %f\n", val, V_R4(&vOut)); }
#define EXPECT_R8(val) EXPECT_OK { EXPECT_TYPE(VT_R8); \
  ok(V_R8(&vOut) == val, "Expected r8 = %g, got %g\n", val, V_R8(&vOut)); }
#define CY_MULTIPLIER 10000
#define EXPECT_CY(val) EXPECT_OK { EXPECT_TYPE(VT_CY); \
  ok(V_CY(&vOut).int64 == (LONG64)(val * CY_MULTIPLIER), "Expected r8 = %#I64x, got %#I64x\n", \
     (LONG64)val, V_CY(&vOut).int64); }
#define EXPECT_DECIMAL(valHi, valMid, valLo) EXPECT_OK { EXPECT_TYPE(VT_DECIMAL); \
      ok(V_DECIMAL(&vOut).Hi32 == valHi && V_DECIMAL(&vOut).Mid32 == valMid && \
      V_DECIMAL(&vOut).Lo32 == valLo, \
  "Expected decimal = %x/0x%x%08x, got %lx/0x%lx%08lx\n", valHi, valMid, valLo, \
      V_DECIMAL(&vOut).Hi32, V_DECIMAL(&vOut).Mid32, V_DECIMAL(&vOut).Lo32); }

static void test_VarNumFromParseNum(void)
{
  HRESULT hres;
  NUMPARSE np;
  BYTE rgb[128];
  VARIANT vOut;

  /* Convert the number 1 to different types */
  SETRGB(0, 1); CONVERT(1,0,0,1,0,0, VTBIT_I1); EXPECT_I1(1);
  SETRGB(0, 1); CONVERT(1,0,0,1,0,0, VTBIT_UI1); EXPECT_UI1(1);
  /* Prefers a signed type to unsigned of the same size */
  SETRGB(0, 1); CONVERT(1,0,0,1,0,0, VTBIT_I1|VTBIT_UI1); EXPECT_I1(1);
  /* But takes the smaller size if possible */
  SETRGB(0, 1); CONVERT(1,0,0,1,0,0, VTBIT_I2|VTBIT_UI1); EXPECT_UI1(1);

  /* Try different integer sizes */
#define INTEGER_VTBITS (VTBIT_I1|VTBIT_UI1|VTBIT_I2|VTBIT_UI2|VTBIT_I4|VTBIT_UI4|VTBIT_I8|VTBIT_UI8)

  SETRGB(0, 1); CONVERT(1,0,0,1,0,0, INTEGER_VTBITS); EXPECT_I1(1);
  /* 127 */
  SETRGB(0, 1); SETRGB(1, 2); SETRGB(2, 7);
  CONVERT(3,0,0,3,0,0, INTEGER_VTBITS); EXPECT_I1(127);
  /* 128 */
  SETRGB(0, 1); SETRGB(1, 2); SETRGB(2, 8);
  CONVERT(3,0,0,3,0,0, INTEGER_VTBITS); EXPECT_UI1(128);
  /* 255 */
  SETRGB(0, 2); SETRGB(1, 5); SETRGB(2, 5);
  CONVERT(3,0,0,3,0,0, INTEGER_VTBITS); EXPECT_UI1(255);
  /* 256 */
  SETRGB(0, 2); SETRGB(1, 5); SETRGB(2, 6);
  CONVERT(3,0,0,3,0,0, INTEGER_VTBITS); EXPECT_I2(256);
  /* 32767 */
  SETRGB(0, 3); SETRGB(1, 2); SETRGB(2, 7); SETRGB(3, 6); SETRGB(4, 7);
  CONVERT(5,0,0,5,0,0, INTEGER_VTBITS); EXPECT_I2(32767);
  /* 32768 */
  SETRGB(0, 3); SETRGB(1, 2); SETRGB(2, 7); SETRGB(3, 6); SETRGB(4, 8);
  CONVERT(5,0,0,5,0,0, INTEGER_VTBITS); EXPECT_UI2(32768);

  /* Assume the above pattern holds for remaining positive integers; test negative */

  /* -128 */
  SETRGB(0, 1); SETRGB(1, 2); SETRGB(2, 8);
  CONVERT(3,0,NUMPRS_NEG,3,0,0, INTEGER_VTBITS); EXPECT_I1(-128);
  /* -129 */
  SETRGB(0, 1); SETRGB(1, 2); SETRGB(2, 9);
  CONVERT(3,0,NUMPRS_NEG,3,0,0, INTEGER_VTBITS); EXPECT_I2(-129);
  /* -32768 */
  SETRGB(0, 3); SETRGB(1, 2); SETRGB(2, 7); SETRGB(3, 6); SETRGB(4, 8);
  CONVERT(5,0,NUMPRS_NEG,5,0,0, INTEGER_VTBITS); EXPECT_I2(-32768);
  /* -32768 */
  SETRGB(0, 3); SETRGB(1, 2); SETRGB(2, 7); SETRGB(3, 6); SETRGB(4, 9);
  CONVERT(5,0,NUMPRS_NEG,5,0,0, INTEGER_VTBITS); EXPECT_I4(-32769);

  /* Assume the above pattern holds for remaining negative integers */

  /* Test hexadecimal conversions */
  SETRGB(0, 1); CONVERT(1,0,0,1,4,0, INTEGER_VTBITS); EXPECT_I1(0x01);
  /* 0x7f */
  SETRGB(0, 7); SETRGB(1, 0xf);
  CONVERT(2,0,0,2,4,0, INTEGER_VTBITS); EXPECT_I1(0x7f);
  SETRGB(0, 7); SETRGB(1, 0xf);
  CONVERT(2,0,0,2,4,0, VTBIT_DECIMAL); EXPECT_DECIMAL(0,0,0x7f);
  /* 0x7fff */
  SETRGB(0, 7); SETRGB(1, 0xf); SETRGB(2, 0xf); SETRGB(3, 0xf);
  CONVERT(4,0,0,4,4,0, INTEGER_VTBITS); EXPECT_I2(0x7fff);
  /* 0x7fffffff */
  SETRGB(0, 7); SETRGB(1, 0xf); SETRGB(2, 0xf); SETRGB(3, 0xf);
  SETRGB(4, 0xf); SETRGB(5, 0xf); SETRGB(6, 0xf); SETRGB(7, 0xf);
  CONVERT(8,0,0,8,4,0, INTEGER_VTBITS); EXPECT_I4(0x7fffffffL);
  /* 0x7fffffffffffffff (64 bits) */
  SETRGB(0, 7); SETRGB(1, 0xf); SETRGB(2, 0xf); SETRGB(3, 0xf);
  SETRGB(4, 0xf); SETRGB(5, 0xf); SETRGB(6, 0xf); SETRGB(7, 0xf);
  SETRGB(8, 0xf); SETRGB(9, 0xf); SETRGB(10, 0xf); SETRGB(11, 0xf);
  SETRGB(12, 0xf); SETRGB(13, 0xf); SETRGB(14, 0xf); SETRGB(15, 0xf);
  if (has_i8)
  {
    /* We cannot use INTEGER_VTBITS as WinXP and Win2003 are broken(?). They
       truncate the number to the smallest integer size requested:
       CONVERT(16,0,0,16,4,0, INTEGER_VTBITS); EXPECT_I1((signed char)0xff); */
    CONVERT(16,0,0,16,4,0, VTBIT_I8); EXPECT_I8(0x7fffffffffffffffull);
  }

  /* Assume the above pattern holds for numbers without hi-bit set, test (preservation of) hi-bit */
  /* 0x82 */
  SETRGB(0, 8); SETRGB(1, 2);
  CONVERT(2,0,0,2,4,0, INTEGER_VTBITS);
  EXPECT_I1((signed char)0x82);
  /* 0x8002 */
  SETRGB(0, 8); SETRGB(1, 0); SETRGB(2, 0); SETRGB(3, 2);
  CONVERT(4,0,0,4,4,0, INTEGER_VTBITS);
  EXPECT_I2((signed short)0x8002);
  /* 0x80000002 */
  SETRGB(0, 8); SETRGB(1, 0); SETRGB(2, 0); SETRGB(3, 0);
  SETRGB(4, 0); SETRGB(5, 0); SETRGB(6, 0); SETRGB(7, 2);
  CONVERT(8,0,0,8,4,0, INTEGER_VTBITS); EXPECT_I4(0x80000002);
  /* 0x8000000000000002 (64 bits) */
  SETRGB(0, 8); SETRGB(1, 0); SETRGB(2, 0); SETRGB(3, 0);
  SETRGB(4, 0); SETRGB(5, 0); SETRGB(6, 0); SETRGB(7, 0);
  SETRGB(8, 0); SETRGB(9, 0); SETRGB(10, 0); SETRGB(11, 0);
  SETRGB(12, 0); SETRGB(13, 0); SETRGB(14, 0); SETRGB(15, 2);
  if (has_i8)
  {
    /* We cannot use INTEGER_VTBITS as WinXP and Win2003 are broken(?). They
       truncate the number to the smallest integer size requested:
       CONVERT(16,0,0,16,4,0, INTEGER_VTBITS & ~VTBIT_I1);
       EXPECT_I2((signed short)0x0002); */
    CONVERT(16,0,0,16,4,0, VTBIT_I8); EXPECT_I8(0x8000000000000002ull);
  }

  /* Test (preservation of) hi-bit with STRICT type requesting */
  /* 0x82 */
  SETRGB(0, 8); SETRGB(1, 2);
  CONVERT(2,0,0,2,4,0, VTBIT_I1);
  EXPECT_I1((signed char)0x82);
  /* 0x8002 */
  SETRGB(0, 8); SETRGB(1, 0); SETRGB(2, 0); SETRGB(3, 2);
  CONVERT(4,0,0,4,4,0, VTBIT_I2);
  EXPECT_I2((signed short)0x8002);
  /* 0x80000002 */
  SETRGB(0, 8); SETRGB(1, 0); SETRGB(2, 0); SETRGB(3, 0);
  SETRGB(4, 0); SETRGB(5, 0); SETRGB(6, 0); SETRGB(7, 2);
  CONVERT(8,0,0,8,4,0, VTBIT_I4); EXPECT_I4(0x80000002);
  /* 0x8000000000000002 (64 bits) */
  SETRGB(0, 8); SETRGB(1, 0); SETRGB(2, 0); SETRGB(3, 0);
  SETRGB(4, 0); SETRGB(5, 0); SETRGB(6, 0); SETRGB(7, 0);
  SETRGB(8, 0); SETRGB(9, 0); SETRGB(10, 0); SETRGB(11, 0);
  SETRGB(12, 0); SETRGB(13, 0); SETRGB(14, 0); SETRGB(15, 2);
  if (has_i8)
  {
    CONVERT(16,0,0,16,4,0, VTBIT_I8); EXPECT_I8(0x8000000000000002ull);
  }
  /* Assume the above pattern holds for numbers with hi-bit set */

  /* Negative numbers overflow if we have only unsigned outputs */
  /* -1 */
  SETRGB(0, 1); CONVERT(1,0,NUMPRS_NEG,1,0,0, VTBIT_UI1); EXPECT_OVERFLOW;
  /* -0.6 */
  SETRGB(0, 6); CONVERT(1,0,NUMPRS_NEG,1,0,~0u, VTBIT_UI1); EXPECT_OVERFLOW;

  /* Except that rounding is done first, so -0.5 to 0 are accepted as 0 */
  /* -0.5 */
  SETRGB(0, 5); CONVERT(1,0,NUMPRS_NEG,1,0,~0u, VTBIT_UI1); EXPECT_UI1(0);

  /* Floating point zero is OK */
  /* 0.00000000E0 */
  SETRGB(0, 0); CONVERT(1,0,NUMPRS_DECIMAL|NUMPRS_EXPONENT,12,0,-8, VTBIT_R8);
  EXPECT_R8(0.0);

  /* Float is acceptable for an integer input value */
  SETRGB(0, 1); CONVERT(1,0,0,1,0,0, VTBIT_R4); EXPECT_R4(1.0f);
  /* As is double */
  SETRGB(0, 1); CONVERT(1,0,0,1,0,0, VTBIT_R8); EXPECT_R8(1.0);
  /* As is currency */
  SETRGB(0, 1); CONVERT(1,0,0,1,0,0, VTBIT_CY); EXPECT_CY(1);

  /* Float is preferred over double */
  SETRGB(0, 1); CONVERT(1,0,0,1,0,0, VTBIT_R4|VTBIT_R8); EXPECT_R4(1.0f);

  /* Double is preferred over currency */
  SETRGB(0, 1); CONVERT(1,0,0,1,0,0, VTBIT_R8|VTBIT_CY); EXPECT_R8(1.0);

  /* Currency is preferred over decimal */
  SETRGB(0, 1); CONVERT(1,0,0,1,0,0, VTBIT_CY|VTBIT_DECIMAL); EXPECT_CY(1);

  /* Underflow test */
  SETRGB(0, 1); CONVERT(1,0,NUMPRS_EXPONENT,1,0,-94938484, VTBIT_R4); EXPECT_R4(0.0);
  SETRGB(0, 1); CONVERT(1,0,NUMPRS_EXPONENT,1,0,-94938484, VTBIT_R8); EXPECT_R8(0.0);
  SETRGB(0, 1); CONVERT(1,0,NUMPRS_EXPONENT,1,0,-94938484, VTBIT_CY); EXPECT_CY(0);
}


static void test_UdateFromDate( int line, DATE dt, ULONG flags, HRESULT r, WORD d, WORD m, WORD y,
                                WORD h, WORD mn, WORD s, WORD ms, WORD dw, WORD dy)
{
    UDATE ud;
    HRESULT res;

    memset(&ud, 0, sizeof(ud));
    res = VarUdateFromDate(dt, flags, &ud);
    ok_(__FILE__,line)(r == res && (res != S_OK || (ud.st.wYear == y && ud.st.wMonth == m && ud.st.wDay == d &&
                       ud.st.wHour == h && ud.st.wMinute == mn && ud.st.wSecond == s &&
                       ud.st.wMilliseconds == ms && ud.st.wDayOfWeek == dw && ud.wDayOfYear == dy)),
                       "%.16g expected res(%lx) %d,%d,%d,%d,%d,%d,%d  %d %d, got res(%lx) %d,%d,%d,%d,%d,%d,%d  %d %d\n",
                       dt, r, d, m, y, h, mn, s, ms, dw, dy,
                       res, ud.st.wDay, ud.st.wMonth, ud.st.wYear, ud.st.wHour, ud.st.wMinute,
                       ud.st.wSecond, ud.st.wMilliseconds, ud.st.wDayOfWeek, ud.wDayOfYear );
}
#define DT2UD(dt,flags,r,d,m,y,h,mn,s,ms,dw,dy) test_UdateFromDate(__LINE__,dt,flags,r,d,m,y,h,mn,s,ms,dw,dy)

static void test_VarUdateFromDate(void)
{
  DT2UD(29221.0,0,S_OK,1,1,1980,0,0,0,0,2,1);        /* 1 Jan 1980 */
  DT2UD(29222.0,0,S_OK,2,1,1980,0,0,0,0,3,2);        /* 2 Jan 1980 */
  DT2UD(33238.0,0,S_OK,31,12,1990,0,0,0,0,1,365);    /* 31 Dec 1990 */
  DT2UD(0.0,0,S_OK,30,12,1899,0,0,0,0,6,364);        /* 30 Dec 1899 - VT_DATE 0.0 */
  DT2UD(-657434.0,0,S_OK,1,1,100,0,0,0,0,5,1);       /* 1 Jan 100 - Min */
  DT2UD(-657435.0,0,E_INVALIDARG,0,0,0,0,0,0,0,0,0); /* < 1 Jan 100 => err */
  DT2UD(2958465.0,0,S_OK,31,12,9999,0,0,0,0,5,365);  /* 31 Dec 9999 - Max */
  DT2UD(2958466.0,0,E_INVALIDARG,0,0,0,0,0,0,0,0,0); /* > 31 Dec 9999 => err  */

  /* VAR_VALIDDATE doesn't prevent upper and lower bounds being checked */
  DT2UD(-657435.0,VAR_VALIDDATE,E_INVALIDARG,0,0,0,0,0,0,0,0,0);
  DT2UD(2958466.0,VAR_VALIDDATE,E_INVALIDARG,0,0,0,0,0,0,0,0,0);

  /* Times */
  DT2UD(29221.25,0,S_OK,1,1,1980,6,0,0,0,2,1);           /* 6 AM */
  DT2UD(29221.33333333,0,S_OK,1,1,1980,8,0,0,0,2,1);     /* 8 AM */
  DT2UD(29221.5,0,S_OK,1,1,1980,12,0,0,0,2,1);           /* 12 AM */
  DT2UD(29221.9888884444,0,S_OK,1,1,1980,23,44,0,0,2,1); /* 11:44 PM */
  DT2UD(29221.7508765432,0,S_OK,1,1,1980,18,1,16,0,2,1); /* 6:18:02 PM */

  /* Test handling of times on dates prior to the epoch */
  DT2UD(-5.25,0,S_OK,25,12,1899,6,0,0,0,1,359);
  DT2UD(-5.9999884259259,0,S_OK,25,12,1899,23,59,59,0,1,359);
  /* This just demonstrates the non-linear nature of values prior to the epoch */
  DT2UD(-4.0,0,S_OK,26,12,1899,0,0,0,0,2,360);
  /* Numerical oddity: for 0.0 < x < 1.0, x and -x represent the same datetime */
  DT2UD(-0.25,0,S_OK,30,12,1899,6,0,0,0,6,364);
  DT2UD(0.25,0,S_OK,30,12,1899,6,0,0,0,6,364);
}


static void test_DateFromUDate( int line, WORD d, WORD m, WORD y, WORD h, WORD mn, WORD s, WORD ms,
                                WORD dw, WORD dy, ULONG flags, HRESULT r, DATE dt )
{
    UDATE ud;
    double out;
    HRESULT res;

    ud.st.wYear = y;
    ud.st.wMonth = m;
    ud.st.wDay = d;
    ud.st.wHour = h;
    ud.st.wMinute = mn;
    ud.st.wSecond = s;
    ud.st.wMilliseconds = ms;
    ud.st.wDayOfWeek = dw;
    ud.wDayOfYear = dy;
    res = VarDateFromUdate(&ud, flags, &out);
    ok_(__FILE__,line)(r == res && (r != S_OK || EQ_DOUBLE(out, dt)),
                       "expected %lx, %.16g, got %lx, %.16g\n", r, dt, res, out);
}
#define UD2T(d,m,y,h,mn,s,ms,dw,dy,flags,r,dt) test_DateFromUDate(__LINE__,d,m,y,h,mn,s,ms,dw,dy,flags,r,dt)

static void test_VarDateFromUdate(void)
{
  UD2T(1,1,1980,0,0,0,0,2,1,0,S_OK,29221.0);      /* 1 Jan 1980 */
  UD2T(2,1,1980,0,0,0,0,3,2,0,S_OK,29222.0);      /* 2 Jan 1980 */
  UD2T(2,1,1980,0,0,0,0,4,5,0,S_OK,29222.0);      /* 2 Jan 1980 */
  UD2T(31,12,1990,0,0,0,0,0,0,0,S_OK,33238.0);    /* 31 Dec 1990 */
  UD2T(31,12,90,0,0,0,0,0,0,0,S_OK,33238.0);      /* year < 100 is 1900+year! */
  UD2T(30,12,1899,0,0,0,0,6,364,0,S_OK,0.0);      /* 30 Dec 1899 - VT_DATE 0.0 */
  UD2T(1,1,100,0,0,0,0,0,0,0,S_OK,-657434.0);     /* 1 Jan 100 - Min */
  UD2T(31,12,9999,0,0,0,0,0,0,0,S_OK,2958465.0);  /* 31 Dec 9999 - Max */
  UD2T(1,1,10000,0,0,0,0,0,0,0,E_INVALIDARG,0.0); /* > 31 Dec 9999 => err  */
  UD2T(1,1,-10000,0,0,0,0,0,0,0,E_INVALIDARG,0.0);/* < -9999 => err  */

  UD2T(30,12,1899,0,0,0,0,0,0,0,S_OK,0.0); /* 30 Dec 1899 0:00:00  */
  UD2T(30,12,1899,0,0,0,999,0,0,0,S_OK,0.0); /* Ignore milliseconds  */

  UD2T(1,1,1980,18,1,16,0,2,1,0,S_OK,29221.75087962963);             /* 6:18:02 PM */
  UD2T(1,300,1980,18,1,16,0,2,1,0,S_OK,38322.75087962963);           /* Test fwdrolled month */
  UD2T(300,1,1980,18,1,16,0,2,1,0,S_OK,29520.75087962963);           /* Test fwdrolled days */
  UD2T(0,1,1980,42,1,16,0,2,1,0,S_OK,29221.75087962963);             /* Test fwdrolled hours */
  UD2T(1,1,1980,17,61,16,0,2,1,0,S_OK,29221.75087962963);            /* Test fwdrolled minutes */
  UD2T(1,1,1980,18,0,76,0,2,1,0,S_OK,29221.75087962963);             /* Test fwdrolled seconds */
  UD2T(1,-300,1980,18,1,16,0,2,1,0,S_OK,20059.75087962963);          /* Test backrolled month */
  UD2T(-300,1,1980,18,1,16,0,2,1,0,S_OK,28920.75087962963);          /* Test backrolled days */
  UD2T(3,1,1980,-30,1,16,0,2,1,0,S_OK,29221.75087962963);            /* Test backrolled hours */
  UD2T(1,1,1980,20,-119,16,0,2,1,0,S_OK,29221.75087962963);          /* Test backrolled minutes */
  UD2T(1,1,1980,18,3,-104,0,2,1,0,S_OK,29221.75087962963);           /* Test backrolled seconds */
  UD2T(1,12001,-1020,18,1,16,0,0,0,0,S_OK,29221.75087962963);        /* Test rolled year and month */
  UD2T(1,-23,1982,18,1,16,0,0,0,0,S_OK,29221.75087962963);           /* Test backrolled month */
  UD2T(-59,3,1980,18,1,16,0,0,0,0,S_OK,29221.75087962963);           /* Test backrolled days */
  UD2T(1,1,0,0,0,0,0,0,0,0,S_OK,36526);                              /* Test zero year */
  UD2T(0,0,1980,0,0,0,0,0,0,0,S_OK,29189);                           /* Test zero day and month */
  UD2T(0,1,1980,0,0,0,0,2,1,0,S_OK,29220.0);                         /* Test zero day = LastDayOfMonth */
  UD2T(-1,1,1980,18,1,16,0,0,0,0,S_OK,29219.75087962963);            /* Test day -1 = LastDayOfMonth - 1 */
  UD2T(1,1,-1,18,1,16,0,0,0,0,S_OK,36161.75087962963);               /* Test year -1 = 1999 */
  UD2T(1,-1,1980,18,1,16,0,0,0,0,S_OK,29160.7508796296);             /* Test month -1 = 11 */
  UD2T(1,13,1980,0,0,0,0,2,1,0,S_OK,29587.0);                        /* Rolls fwd to 1/1/1981 */

  /* Test handling of times on dates prior to the epoch */
  UD2T(25,12,1899,6,0,0,0,1,359,0,S_OK,-5.25);
  UD2T(25,12,1899,23,59,59,0,1,359,0,S_OK,-5.9999884259259);
  /* This just demonstrates the non-linear nature of values prior to the epoch */
  UD2T(26,12,1899,0,0,0,0,2,360,0,S_OK,-4.0);
  /* for DATE values 0.0 < x < 1.0, x and -x represent the same datetime */
  /* but when converting to DATE, prefer the positive versions */
  UD2T(30,12,1899,6,0,0,0,6,364,0,S_OK,0.25);

  UD2T(1,1,1980,18,1,16,0,2,1,VAR_TIMEVALUEONLY,S_OK,0.7508796296296296);
  UD2T(1,1,1980,18,1,16,0,2,1,VAR_DATEVALUEONLY,S_OK,29221.0);
  UD2T(25,12,1899,6,0,0,0,1,359,VAR_TIMEVALUEONLY,S_OK,0.25);
  UD2T(25,12,1899,6,0,0,0,1,359,VAR_DATEVALUEONLY,S_OK,-5.0);
  UD2T(1,-1,1980,18,1,16,0,0,0,VAR_TIMEVALUEONLY|VAR_DATEVALUEONLY,S_OK,0.7508796296296296);
}

static void test_st2dt(int line, WORD d, WORD m, WORD y, WORD h, WORD mn,
                       WORD s, WORD ms, INT r, double dt, double dt2)
{
    SYSTEMTIME st;
    double out;
    INT res;

    st.wYear = y;
    st.wMonth = m;
    st.wDay = d;
    st.wHour = h;
    st.wMinute = mn;
    st.wSecond = s;
    st.wMilliseconds = ms;
    st.wDayOfWeek = 0;
    res = SystemTimeToVariantTime(&st, &out);
    ok_(__FILE__,line)(r == res, "expected %d, got %d\n", r, res);
    if (r && res)
        ok_(__FILE__,line)(EQ_DOUBLE(out, dt) || (dt2 && broken(EQ_DOUBLE(out, dt2))),
                           "expected %.16g or %.16g, got %.16g\n", dt, dt2, out);
}
#define ST2DT(d,m,y,h,mn,s,ms,r,dt,dt2) test_st2dt(__LINE__,d,m,y,h,mn,s,ms,r,dt,dt2)

static void test_SystemTimeToVariantTime(void)
{
  ST2DT(1,1,1980,0,0,0,0,TRUE,29221.0,0.0);
  ST2DT(2,1,1980,0,0,0,0,TRUE,29222.0,0.0);
  ST2DT(0,1,1980,0,0,0,0,TRUE,29220.0,0.0);   /* Rolls back to 31 Dec 1899 */
  ST2DT(1,13,1980,0,0,0,0,FALSE,0.0,0.0);     /* Fails on invalid month */
  ST2DT(32,1,1980,0,0,0,0,FALSE,0.0,0.0);     /* Fails on invalid day */
  ST2DT(1,1,-1,0,0,0,0,FALSE,0.0,0.0);        /* Fails on invalid year */
  ST2DT(1,1,10000,0,0,0,0,FALSE,0.0,0.0);     /* Fails on invalid year */
  ST2DT(1,1,9999,0,0,0,0,TRUE,2958101.0,0.0); /* 9999 is last valid year */

  /* Old Windows versions use 29 as the Y2K cutoff:
   * years 00-29 map to 2000-2029 while years 30-99 map to 1930-1999
   */
  ST2DT(1,1,0,0,0,0,0,TRUE,36526.0,0.0);
  ST2DT(1,1,29,0,0,0,0,TRUE,47119.0,0.0);
  ST2DT(1,1,30,0,0,0,0,TRUE,47484.0,10959.0);
  /* But Windows 1903+ uses 49 as the Y2K cutoff */
  ST2DT(1,1,49,0,0,0,0,TRUE,54424.0,17899.0);
  ST2DT(1,1,50,0,0,0,0,TRUE,18264.0,0.0);
  ST2DT(31,12,99,0,0,0,0,TRUE,36525.0,0.0);
}

static void test_dt2st(int line, double dt, INT r, WORD d, WORD m, WORD y,
                       WORD h, WORD mn, WORD s, WORD ms)
{
  SYSTEMTIME st;
  INT res;

  memset(&st, 0, sizeof(st));
  res = VariantTimeToSystemTime(dt, &st);
  ok_(__FILE__,line)(r == res &&
                     (!r || (st.wYear == y && st.wMonth == m && st.wDay == d &&
                             st.wHour == h && st.wMinute == mn &&
                             st.wSecond == s && st.wMilliseconds == ms)),
                     "%.16g expected %d, %d,%d,%d,%d,%d,%d,%d, got %d, %d,%d,%d,%d,%d,%d,%d\n",
                     dt, r, d, m, y, h, mn, s, ms, res, st.wDay, st.wMonth,
                     st.wYear, st.wHour, st.wMinute, st.wSecond,
                     st.wMilliseconds);
}
#define DT2ST(dt,r,d,m,y,h,mn,s,ms) test_dt2st(__LINE__,dt,r,d,m,y,h,mn,s,ms)

static void test_VariantTimeToSystemTime(void)
{
    DT2ST(29221.0,1,1,1,1980,0,0,0,0);
    DT2ST(29222.0,1,2,1,1980,0,0,0,0);
}

#define MKDOSDATE(d,m,y) ((d & 0x1f) | ((m & 0xf) << 5) | (((y-1980) & 0x7f) << 9))
#define MKDOSTIME(h,m,s) (((s>>1) & 0x1f) | ((m & 0x3f) << 5) | ((h & 0x1f) << 11))

static void test_dos2dt(int line, WORD d, WORD m, WORD y, WORD h, WORD mn,
                        WORD s, INT r, double dt)
{
    unsigned short dosDate, dosTime;
    double out;
    INT res;

    out = 0.0;
    dosDate = MKDOSDATE(d, m, y);
    dosTime = MKDOSTIME(h, mn, s);
    res = DosDateTimeToVariantTime(dosDate, dosTime, &out);
    ok_(__FILE__,line)(r == res && (!r || EQ_DOUBLE(out, dt)),
                       "expected %d, %.16g, got %d, %.16g\n", r, dt, res, out);
}
#define DOS2DT(d,m,y,h,mn,s,r,dt) test_dos2dt(__LINE__,d,m,y,h,mn,s,r,dt)

static void test_DosDateTimeToVariantTime(void)
{
  /* Date */
  DOS2DT(1,1,1980,0,0,0,1,29221.0); /* 1/1/1980 */
  DOS2DT(31,12,2099,0,0,0,1,73050.0); /* 31/12/2099 */
  /* Dates are limited to the dos date max of 31/12/2099 */
  DOS2DT(31,12,2100,0,0,0,0,0.0); /* 31/12/2100 */
  /* Days and months of 0 cause date to roll back 1 day or month */
  DOS2DT(0,1,1980,0,0,0,1,29220.0); /* 0 Day => 31/12/1979 */
  DOS2DT(1,0,1980,0,0,0,1,29190.0); /* 0 Mth =>  1/12/1979 */
  DOS2DT(0,0,1980,0,0,0,1,29189.0); /* 0 D/M => 30/11/1979 */
  /* Days > days in the month cause date to roll forward 1 month */
  DOS2DT(29,2,1981,0,0,0,1,29646.0); /* 29/2/1981 -> 3/1/1980 */
  DOS2DT(30,2,1981,0,0,0,1,29647.0); /* 30/2/1981 -> 4/1/1980 */
  /* Takes leap years into account when rolling forward */
  DOS2DT(29,2,1980,0,0,0,1,29280.0); /* 2/29/1980 */
  /* Months > 12 cause an error */
  DOS2DT(2,13,1980,0,0,0,0,0.0);

  /* Time */
  DOS2DT(1,1,1980,0,0,29,1,29221.00032407407); /* 1/1/1980 12:00:28 AM */
  DOS2DT(1,1,1980,0,0,31,1,29221.00034722222); /* 1/1/1980 12:00:30 AM */
  DOS2DT(1,1,1980,0,59,0,1,29221.04097222222); /* 1/1/1980 12:59:00 AM */
  DOS2DT(1,1,1980,0,60,0,0,0.0);               /* Invalid minutes */
  DOS2DT(1,1,1980,0,0,60,0,0.0);               /* Invalid seconds */
  DOS2DT(1,1,1980,23,0,0,1,29221.95833333333); /* 1/1/1980 11:00:00 PM */
  DOS2DT(1,1,1980,24,0,0,0,0.0);               /* Invalid hours */

  DOS2DT(1,1,1980,0,0,1,1,29221.0);
  DOS2DT(2,1,1980,0,0,0,1,29222.0);
  DOS2DT(2,1,1980,0,0,0,1,29222.0);
  DOS2DT(31,12,1990,0,0,0,1,33238.0);
  DOS2DT(31,12,90,0,0,0,1,40543.0);
  DOS2DT(30,12,1899,0,0,0,1,46751.0);
  DOS2DT(1,1,100,0,0,0,1,43831.0);
  DOS2DT(31,12,9999,0,0,0,1,59901.0);
  DOS2DT(1,1,10000,0,0,0,1,59902.0);
  DOS2DT(1,1,-10000,0,0,0,1,48214.0);

  DOS2DT(30,12,1899,0,0,0,1,46751.0);
  DOS2DT(30,12,1899,0,0,1,1,46751.0);

  DOS2DT(1,1,1980,18,1,16,1,29221.75087962963);
  DOS2DT(1,300,1980,18,1,16,1,29556.75087962963);
  DOS2DT(300,1,1980,18,1,16,1,29232.75087962963);
  DOS2DT(0,1,1980,42,1,16,1,29220.4175462963);
  DOS2DT(1,1,1980,17,61,16,0,0.0);
  DOS2DT(1,1,1980,18,0,76,1,29221.75013888889);
  DOS2DT(1,-300,1980,18,1,16,1,29312.75087962963);
  DOS2DT(-300,1,1980,18,1,16,1,29240.75087962963);
  DOS2DT(3,1,1980,-30,1,16,1,29223.08421296296);
  DOS2DT(1,1,1980,20,-119,16,1,29221.83976851852);
  DOS2DT(1,1,1980,18,3,-104,1,29221.75236111111);
  DOS2DT(1,12001,-1020,18,1,16,1,55519.75087962963);
  DOS2DT(1,-23,1982,18,1,16,1,30195.75087962963);
  DOS2DT(-59,3,1980,18,1,16,1,29285.75087962963);
  DOS2DT(1,1,0,0,0,0,1,54058.0);
  DOS2DT(0,0,1980,0,0,0,1,29189.0);
  DOS2DT(0,1,1980,0,0,0,1,29220.0);
  DOS2DT(-1,1,1980,18,1,16,1,29251.75087962963);
  DOS2DT(1,1,-1,18,1,16,1,53693.75087962963);
  DOS2DT(1,-1,1980,18,1,16,0,0);
}

static void test_dt2dos(int line, double dt, INT r, WORD d, WORD m, WORD y,
                        WORD h, WORD mn, WORD s)
{
    unsigned short dosDate, dosTime, expDosDate, expDosTime;
    INT res;

    dosTime = dosDate = 0;
    expDosDate = MKDOSDATE(d,m,y);
    expDosTime = MKDOSTIME(h,mn,s);
    res = VariantTimeToDosDateTime(dt, &dosDate, &dosTime);
    ok_(__FILE__,line)(r == res && (!r || (dosTime == expDosTime && dosDate == expDosDate)),
                       "%g: expected %d,%d(%d/%d/%d),%d(%d:%d:%d) got %d,%d(%d/%d/%d),%d(%d:%d:%d)\n",
                       dt, r, expDosDate, expDosDate & 0x1f,
                       (expDosDate >> 5) & 0xf, 1980 + (expDosDate >> 9),
                       expDosTime, expDosTime >> 11, (expDosTime >> 5) & 0x3f,
                       (expDosTime & 0x1f),
                       res, dosDate, dosDate & 0x1f, (dosDate >> 5) & 0xf,
                       1980 + (dosDate >> 9), dosTime, dosTime >> 11,
                       (dosTime >> 5) & 0x3f, (dosTime & 0x1f));
}
#define DT2DOS(dt,r,d,m,y,h,mn,s) test_dt2dos(__LINE__,dt,r,d,m,y,h,mn,s)

static void test_VariantTimeToDosDateTime(void)
{
  /* Date */
  DT2DOS(29221.0,1,1,1,1980,0,0,0);   /* 1/1/1980 */
  DT2DOS(73050.0,1,31,12,2099,0,0,0); /* 31/12/2099 */
  DT2DOS(29220.0,0,0,0,0,0,0,0);      /* 31/12/1979 - out of range */
  DT2DOS(73415.0,0,0,0,0,0,0,0);      /* 31/12/2100 - out of range */

  /* Time */
  DT2DOS(29221.00032407407,1,1,1,1980,0,0,29); /* 1/1/1980 12:00:28 AM */
  DT2DOS(29221.00034722222,1,1,1,1980,0,0,31); /* 1/1/1980 12:00:30 AM */
  DT2DOS(29221.04097222222,1,1,1,1980,0,59,0); /* 1/1/1980 12:59:00 AM */
  DT2DOS(29221.95833333333,1,1,1,1980,23,0,0); /* 1/1/1980 11:00:00 PM */
}

static HRESULT (WINAPI *pVarAbs)(LPVARIANT,LPVARIANT);

#define VARABS(vt,val,rvt,rval)                  \
    V_VT(&v) = VT_##vt; V_##vt(&v) = val;        \
    V_VT(&exp) = VT_##rvt; V_##rvt(&exp) = rval; \
    test_var_call1( __LINE__, pVarAbs, &v, &exp )

static void test_VarAbs(void)
{
    static WCHAR szNum[] = {'-','1','.','1','\0' };
    char buff[8];
    HRESULT hres;
    VARIANT v, vDst, exp;
    size_t i;

    CHECKPTR(VarAbs);

    /* Test all possible V_VT values.
     */
    for (i = 0; i < ARRAY_SIZE(ExtraFlags); i++)
    {
        VARTYPE vt;

        for (vt = 0; vt <= VT_BSTR_BLOB; vt++)
        {
            HRESULT hExpected = DISP_E_BADVARTYPE;

            SKIPTESTS(vt);

            memset(&v, 0, sizeof(v));
            V_VT(&v) = vt | ExtraFlags[i];
            V_VT(&vDst) = VT_EMPTY;

            hres = pVarAbs(&v,&vDst);
            if (ExtraFlags[i] & VT_ARRAY ||
                (!ExtraFlags[i] && (vt == VT_UNKNOWN || vt == VT_BSTR ||
                 vt == VT_DISPATCH || vt == VT_ERROR || vt == VT_RECORD)))
            {
                hExpected = DISP_E_TYPEMISMATCH;
            }
            else if (ExtraFlags[i] || vt >= VT_CLSID || vt == VT_VARIANT)
            {
                hExpected = DISP_E_BADVARTYPE;
            }
            else if (IsValidVariantClearVT(vt, ExtraFlags[i]))
                hExpected = S_OK;

            /* Native always fails on some vartypes that should be valid. don't
             * check that Wine does the same; these are bugs in native.
             */
            if (vt == VT_I8 || vt == VT_UI8 || vt == VT_INT || vt == VT_UINT ||
                vt == VT_I1 || vt == VT_UI2 || vt == VT_UI4)
                continue;
            ok(hres == hExpected, "VarAbs: expected 0x%lX, got 0x%lX for vt %d | 0x%X\n",
               hExpected, hres, vt, ExtraFlags[i]);
        }
    }

    /* BOOL->I2, BSTR->R8, all others remain the same */
    VARABS(BOOL,VARIANT_TRUE,I2,-VARIANT_TRUE);
    VARABS(BOOL,VARIANT_FALSE,I2,VARIANT_FALSE);
    VARABS(EMPTY,0,I2,0);
    VARABS(EMPTY,1,I2,0);
    VARABS(NULL,0,NULL,0);
    VARABS(NULL,1,NULL,0);
    VARABS(I2,1,I2,1);
    VARABS(I2,-1,I2,1);
    VARABS(I4,1,I4,1);
    VARABS(I4,-1,I4,1);
    VARABS(UI1,1,UI1,1);
    VARABS(R4,1,R4,1);
    VARABS(R4,-1,R4,1);
    VARABS(R8,1,R8,1);
    VARABS(R8,-1,R8,1);
    VARABS(R4,1.40129846432481707e-45,R4,1.40129846432481707e-45);
    VARABS(R8,4.94065645841246544e-324,R8,4.94065645841246544e-324);
    VARABS(DATE,1,DATE,1);
    VARABS(DATE,-1,DATE,1);
    V_VT(&v) = VT_CY;
    V_CY(&v).int64 = -10000;
    memset(&vDst,0,sizeof(vDst));
    hres = pVarAbs(&v,&vDst);
    ok(hres == S_OK && V_VT(&vDst) == VT_CY && V_CY(&vDst).int64 == 10000,
       "VarAbs(CY): expected 0x0 got 0x%lX\n", hres);
    GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, buff, ARRAY_SIZE(buff));
    if (buff[1])
    {
        trace("Skipping VarAbs(BSTR) as decimal separator is '%s'\n", buff);
        return;
    } else {
	szNum[2] = buff[0];
    }
    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = (BSTR)szNum;
    memset(&vDst,0,sizeof(vDst));
    hres = pVarAbs(&v,&vDst);
    ok(hres == S_OK && V_VT(&vDst) == VT_R8 && V_R8(&vDst) == 1.1,
       "VarAbs: expected 0x0,%d,%g, got 0x%lX,%d,%g\n", VT_R8, 1.1, hres, V_VT(&vDst), V_R8(&vDst));

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = SysAllocString(L"30000");
    memset(&vDst,0,sizeof(vDst));
    hres = pVarAbs(&v,&vDst);
    ok(hres == S_OK && V_VT(&vDst) == VT_R8 && V_R8(&vDst) == 30000.0,
       "VarAbs: expected 0x0,%d,%g, got 0x%lX,%d,%g\n", VT_R8, 30000.0, hres, V_VT(&vDst), V_R8(&vDst));
    SysFreeString(V_BSTR(&v));
}

static HRESULT (WINAPI *pVarNot)(LPVARIANT,LPVARIANT);

#define VARNOT(vt,val,rvt,rval)                  \
    V_VT(&v) = VT_##vt; V_##vt(&v) = val;        \
    V_VT(&exp) = VT_##rvt; V_##rvt(&exp) = rval; \
    test_var_call1( __LINE__, pVarNot, &v, &exp )

static void test_VarNot(void)
{
    static const WCHAR szNum0[] = {'0','\0' };
    static const WCHAR szNum1[] = {'1','\0' };
    static const WCHAR szFalse[] = { '#','F','A','L','S','E','#','\0' };
    static const WCHAR szTrue[] = { '#','T','R','U','E','#','\0' };
    HRESULT hres;
    VARIANT v, exp, vDst;
    DECIMAL *pdec = &V_DECIMAL(&v);
    CY *pcy = &V_CY(&v);
    size_t i;

    CHECKPTR(VarNot);

    /* Test all possible V_VT values */
    for (i = 0; i < ARRAY_SIZE(ExtraFlags); i++)
    {
        VARTYPE vt;

        for (vt = 0; vt <= VT_BSTR_BLOB; vt++)
        {
            HRESULT hExpected = DISP_E_BADVARTYPE;

            SKIPTESTS(vt);

            memset(&v, 0, sizeof(v));
            V_VT(&v) = vt | ExtraFlags[i];
            V_VT(&vDst) = VT_EMPTY;

            switch (V_VT(&v))
            {
            case VT_I1:  case VT_UI1: case VT_I2:  case VT_UI2:
            case VT_INT: case VT_UINT: case VT_I4:  case VT_UI4:
            case VT_R4:  case VT_R8:
            case VT_DECIMAL: case VT_BOOL: case VT_NULL: case VT_EMPTY:
            case VT_DATE: case VT_CY:
                hExpected = S_OK;
                break;
            case VT_I8: case VT_UI8:
                if (has_i8)
                    hExpected = S_OK;
                break;
            case VT_RECORD:
                hExpected = DISP_E_TYPEMISMATCH;
                break;
            case VT_UNKNOWN: case VT_BSTR: case VT_DISPATCH: case VT_ERROR:
                hExpected = DISP_E_TYPEMISMATCH;
                break;
            default:
                if (IsValidVariantClearVT(vt, ExtraFlags[i]) && vt != VT_CLSID)
                   hExpected = DISP_E_TYPEMISMATCH;
                break;
            }

            hres = pVarNot(&v,&vDst);
            ok(hres == hExpected, "VarNot: expected 0x%lX, got 0x%lX vt %d|0x%X\n",
               hExpected, hres, vt, ExtraFlags[i]);
        }
    }
    /* Test the values returned by all cases that can succeed */
    VARNOT(EMPTY,0,I2,-1);
    VARNOT(EMPTY,1,I2,-1);
    VARNOT(NULL,0,NULL,0);
    VARNOT(NULL,1,NULL,0);
    VARNOT(BOOL,VARIANT_TRUE,BOOL,VARIANT_FALSE);
    VARNOT(BOOL,VARIANT_FALSE,BOOL,VARIANT_TRUE);
    VARNOT(I1,-1,I4,0);
    VARNOT(I1,0,I4,-1);
    VARNOT(I2,-1,I2,0);
    VARNOT(I2,0,I2,-1);
    VARNOT(I2,1,I2,-2);
    VARNOT(I4,1,I4,-2);
    VARNOT(I4,0,I4,-1);
    VARNOT(UI1,1,UI1,254);
    VARNOT(UI1,0,UI1,255);
    VARNOT(UI2,0,I4,-1);
    VARNOT(UI2,1,I4,-2);
    VARNOT(UI4,0,I4,-1);
    VARNOT(UI4,1,I4,-2);
    VARNOT(INT,0,I4,-1);
    VARNOT(INT,1,I4,-2);
    VARNOT(UINT,0,I4,-1);
    VARNOT(UINT,1,I4,-2);
    if (has_i8)
    {
        VARNOT(I8,1,I8,-2);
        VARNOT(I8,0,I8,-1);
        VARNOT(UI8,0,I4,-1);
        VARNOT(UI8,1,I4,-2);
    }
    VARNOT(R4,1,I4,-2);
    VARNOT(R4,0,I4,-1);
    VARNOT(R8,1,I4,-2);
    VARNOT(R8,0,I4,-1);
    VARNOT(DATE,1,I4,-2);
    VARNOT(DATE,0,I4,-1);
    VARNOT(BSTR,(BSTR)szNum0,I4,-1);
    ok(V_VT(&v) == VT_BSTR && V_BSTR(&v) == szNum0, "VarNot(0): changed input\n");
    VARNOT(BSTR,(BSTR)szNum1,I4,-2);
    ok(V_VT(&v) == VT_BSTR && V_BSTR(&v) == szNum1, "VarNot(1): changed input\n");
    VARNOT(BSTR, (BSTR)szTrue, BOOL, VARIANT_FALSE);
    VARNOT(BSTR, (BSTR)szFalse, BOOL, VARIANT_TRUE);

    pdec->sign = DECIMAL_NEG;
    pdec->scale = 0;
    pdec->Hi32 = 0;
    pdec->Mid32 = 0;
    pdec->Lo32 = 1;
    VARNOT(DECIMAL,*pdec,I4,0);

    pcy->int64 = 10000;
    VARNOT(CY,*pcy,I4,-2);

    pcy->int64 = 0;
    VARNOT(CY,*pcy,I4,-1);

    pcy->int64 = -1;
    VARNOT(CY,*pcy,I4,-1);
}

static HRESULT (WINAPI *pVarSub)(LPVARIANT,LPVARIANT,LPVARIANT);

#define VARSUB(vt1,val1,vt2,val2,rvt,rval)               \
        V_VT(&left) = VT_##vt1; V_##vt1(&left) = val1;   \
        V_VT(&right) = VT_##vt2; V_##vt2(&right) = val2; \
        V_VT(&exp) = VT_##rvt; V_##rvt(&exp) = rval;     \
        test_var_call2( __LINE__, pVarSub, &left, &right, &exp )

static void test_VarSub(void)
{
    VARIANT left, right, exp, result, cy, dec;
    VARTYPE i;
    BSTR lbstr, rbstr;
    HRESULT hres, expectedhres;
    double r;

    CHECKPTR(VarSub);

    lbstr = SysAllocString(L"12");
    rbstr = SysAllocString(L"12");

    VariantInit(&left);
    VariantInit(&right);
    VariantInit(&result);

    /* Test all possible flag/vt combinations & the resulting vt type */
    for (i = 0; i < ARRAY_SIZE(ExtraFlags); i++)
    {

        VARTYPE leftvt, rightvt, resvt;

        for (leftvt = 0; leftvt <= VT_BSTR_BLOB; leftvt++)
        {

            SKIPTESTS(leftvt);

            for (rightvt = 0; rightvt <= VT_BSTR_BLOB; rightvt++)
            {

                SKIPTESTS(rightvt);
                expectedhres = S_OK;

                memset(&left, 0, sizeof(left));
                memset(&right, 0, sizeof(right));
                V_VT(&left) = leftvt | ExtraFlags[i];
                if (leftvt == VT_BSTR)
                    V_BSTR(&left) = lbstr;
                V_VT(&right) = rightvt | ExtraFlags[i];
                if (rightvt == VT_BSTR)
                    V_BSTR(&right) = rbstr;
                V_VT(&result) = VT_EMPTY;

                /* All extra flags produce errors */
                if (ExtraFlags[i] == (VT_VECTOR|VT_BYREF|VT_RESERVED) ||
                    ExtraFlags[i] == (VT_VECTOR|VT_RESERVED) ||
                    ExtraFlags[i] == (VT_VECTOR|VT_BYREF) ||
                    ExtraFlags[i] == (VT_BYREF|VT_RESERVED) ||
                    ExtraFlags[i] == VT_VECTOR ||
                    ExtraFlags[i] == VT_BYREF ||
                    ExtraFlags[i] == VT_RESERVED)
                {
                    expectedhres = DISP_E_BADVARTYPE;
                    resvt = VT_EMPTY;
                }
                else if (ExtraFlags[i] >= VT_ARRAY)
                {
                    expectedhres = DISP_E_TYPEMISMATCH;
                    resvt = VT_EMPTY;
                }
                /* Native VarSub cannot handle: VT_I1, VT_UI2, VT_UI4,
                   VT_INT, VT_UINT and VT_UI8. Tested with WinXP */
                else if (!IsValidVariantClearVT(leftvt, ExtraFlags[i]) ||
                    !IsValidVariantClearVT(rightvt, ExtraFlags[i]) ||
                    leftvt == VT_CLSID || rightvt == VT_CLSID ||
                    leftvt == VT_VARIANT || rightvt == VT_VARIANT ||
                    leftvt == VT_I1 || rightvt == VT_I1 ||
                    leftvt == VT_UI2 || rightvt == VT_UI2 ||
                    leftvt == VT_UI4 || rightvt == VT_UI4 ||
                    leftvt == VT_UI8 || rightvt == VT_UI8 ||
                    leftvt == VT_INT || rightvt == VT_INT ||
                    leftvt == VT_UINT || rightvt == VT_UINT ||
                    leftvt == VT_UNKNOWN || rightvt == VT_UNKNOWN ||
                    leftvt == VT_RECORD || rightvt == VT_RECORD)
                {
                    if (leftvt == VT_RECORD && rightvt == VT_I8)
                    {
                        if (has_i8)
                            expectedhres = DISP_E_TYPEMISMATCH;
                        else
                            expectedhres = DISP_E_BADVARTYPE;
                    }
                    else if (leftvt < VT_UI1 && rightvt == VT_RECORD)
                        expectedhres = DISP_E_TYPEMISMATCH;
                    else if (leftvt >= VT_UI1 && rightvt == VT_RECORD)
                        expectedhres = DISP_E_TYPEMISMATCH;
                    else if (leftvt == VT_RECORD && rightvt <= VT_UI1)
                        expectedhres = DISP_E_TYPEMISMATCH;
                    else if (leftvt == VT_RECORD && rightvt > VT_UI1)
                        expectedhres = DISP_E_BADVARTYPE;
                    else
                        expectedhres = DISP_E_BADVARTYPE;
                    resvt = VT_EMPTY;
                }
                else if ((leftvt == VT_NULL && rightvt == VT_DISPATCH) ||
                    (leftvt == VT_DISPATCH && rightvt == VT_NULL))
                    resvt = VT_NULL;
                else if (leftvt == VT_DISPATCH || rightvt == VT_DISPATCH ||
                    leftvt == VT_ERROR || rightvt == VT_ERROR)
                {
                    resvt = VT_EMPTY;
                    expectedhres = DISP_E_TYPEMISMATCH;
                }
                else if (leftvt == VT_NULL || rightvt == VT_NULL)
                    resvt = VT_NULL;
                else if ((leftvt == VT_EMPTY && rightvt == VT_BSTR) ||
                    (leftvt == VT_DATE && rightvt == VT_DATE) ||
                    (leftvt == VT_BSTR && rightvt == VT_EMPTY) ||
                    (leftvt == VT_BSTR && rightvt == VT_BSTR))
                    resvt = VT_R8;
                else if (leftvt == VT_DECIMAL || rightvt == VT_DECIMAL)
                    resvt = VT_DECIMAL;
                else if (leftvt == VT_DATE || rightvt == VT_DATE)
                    resvt = VT_DATE;
                else if (leftvt == VT_CY || rightvt == VT_CY)
                    resvt = VT_CY;
                else if (leftvt == VT_R8 || rightvt == VT_R8)
                    resvt = VT_R8;
                else if (leftvt == VT_BSTR || rightvt == VT_BSTR) {
                    resvt = VT_R8;
                } else if (leftvt == VT_R4 || rightvt == VT_R4) {
                    if (leftvt == VT_I4 || rightvt == VT_I4 ||
                        leftvt == VT_I8 || rightvt == VT_I8)
                        resvt = VT_R8;
                    else
                        resvt = VT_R4;
                }
                else if (leftvt == VT_I8 || rightvt == VT_I8)
                    resvt = VT_I8;
                else if (leftvt == VT_I4 || rightvt == VT_I4)
                    resvt = VT_I4;
                else if (leftvt == VT_I2 || rightvt == VT_I2 ||
                    leftvt == VT_BOOL || rightvt == VT_BOOL ||
                    (leftvt == VT_EMPTY && rightvt == VT_EMPTY))
                    resvt = VT_I2;
                else if (leftvt == VT_UI1 || rightvt == VT_UI1)
                    resvt = VT_UI1;
                else
                {
                    resvt = VT_EMPTY;
                    expectedhres = DISP_E_TYPEMISMATCH;
                }

                hres = pVarSub(&left, &right, &result);

                ok(hres == expectedhres && V_VT(&result) == resvt,
                    "VarSub: %d|0x%X, %d|0x%X: Expected failure 0x%lX, "
                    "got 0x%lX, expected vt %d got vt %d\n",
                    leftvt, ExtraFlags[i], rightvt, ExtraFlags[i],
                    expectedhres, hres, resvt, V_VT(&result));
            }
        }
    }

    /* Test returned values */
    VARSUB(I4,4,I4,2,I4,2);
    VARSUB(I2,4,I2,2,I2,2);
    VARSUB(I2,-13,I4,5,I4,-18);
    VARSUB(I4,-13,I4,5,I4,-18);
    VARSUB(I2,7,R4,0.5f,R4,6.5f);
    VARSUB(R4,0.5f,I4,5,R8,-4.5);
    VARSUB(R8,7.1,BOOL,0,R8,7.1);
    VARSUB(BSTR,lbstr,I2,4,R8,8);
    VARSUB(BSTR,lbstr,BOOL,1,R8,11);
    VARSUB(BSTR,lbstr,R4,0.1f,R8,11.9);
    VARSUB(R4,0.2f,BSTR,rbstr,R8,-11.8);
    VARSUB(DATE,2.25,I4,7,DATE,-4.75);
    VARSUB(DATE,1.25,R4,-1.7f,DATE,2.95);

    VARSUB(UI1, UI1_MAX, UI1, UI1_MAX, UI1, 0);
    VARSUB(I2, I2_MAX, I2, I2_MAX, I2, 0);
    VARSUB(I2, I2_MIN, I2, I2_MIN, I2, 0);
    VARSUB(I4, I4_MAX, I4, I4_MAX, I4, 0);
    VARSUB(I4, I4_MIN, I4, I4_MIN, I4, 0);
    VARSUB(R4, R4_MAX, R4, R4_MAX, R4, 0.0f);
    VARSUB(R4, R4_MAX, R4, R4_MIN, R4, R4_MAX - R4_MIN);
    VARSUB(R4, R4_MIN, R4, R4_MIN, R4, 0.0f);
    VARSUB(R8, R8_MAX, R8, R8_MIN, R8, R8_MAX - R8_MIN);
    VARSUB(R8, R8_MIN, R8, R8_MIN, R8, 0.0);

    /* Manually test BSTR + BSTR */
    V_VT(&left) = VT_BSTR;
    V_BSTR(&left) = lbstr;
    V_VT(&right) = VT_BSTR;
    V_BSTR(&right) = rbstr;
    hres = pVarSub(&left, &right, &result);
    ok(hres == S_OK && V_VT(&result) == VT_R8,
        "VarSub: expected coerced type VT_R8, got %s!\n", wine_dbgstr_vt(V_VT(&result)));
    ok(hres == S_OK && EQ_DOUBLE(V_R8(&result), 0.0),
        "VarSub: BSTR + BSTR, expected %f got %f\n", 0.0, V_R8(&result));

    /* Manually test some VT_CY and VT_DECIMAL variants */
    V_VT(&cy) = VT_CY;
    hres = VarCyFromI4(4711, &V_CY(&cy));
    ok(hres == S_OK, "VarCyFromI4 failed!\n");
    V_VT(&dec) = VT_DECIMAL;
    hres = VarDecFromR8(-4.2, &V_DECIMAL(&dec));
    ok(hres == S_OK, "VarDecFromR4 failed!\n");
    memset(&left, 0, sizeof(left));
    memset(&right, 0, sizeof(right));
    V_VT(&left) = VT_I4;
    V_I4(&left) = -11;
    V_VT(&right) = VT_UI1;
    V_UI1(&right) = 9;

    hres = pVarSub(&cy, &right, &result);
    ok(hres == S_OK && V_VT(&result) == VT_CY,
        "VarSub: expected coerced type VT_CY, got %s!\n", wine_dbgstr_vt(V_VT(&result)));
    hres = VarR8FromCy(V_CY(&result), &r);
    ok(hres == S_OK && EQ_DOUBLE(r, 4702.0), "VarSub: CY value %f, expected %f\n", r, 4720.0);

    hres = pVarSub(&left, &dec, &result);
    ok(hres == S_OK && V_VT(&result) == VT_DECIMAL,
        "VarSub: expected coerced type VT_DECIMAL, got %s!\n", wine_dbgstr_vt(V_VT(&result)));
    hres = VarR8FromDec(&V_DECIMAL(&result), &r);
    ok(hres == S_OK && EQ_DOUBLE(r, -6.8), "VarSub: DECIMAL value %f, expected %f\n", r, -6.8);

    SysFreeString(lbstr);
    SysFreeString(rbstr);
}

static HRESULT (WINAPI *pVarMod)(LPVARIANT,LPVARIANT,LPVARIANT);

static void test_Mod( int line, VARIANT *left, VARIANT *right, VARIANT *expected, HRESULT expres )
{
    VARIANT result;
    HRESULT hres;

    V_VT(&result) = 15;
    V_I4(&result) = 0x12345;
    hres = pVarMod( left, right, &result );
    ok_(__FILE__,line)( hres == expres, "wrong result %lx/%lx\n", hres, expres );
    if (hres == S_OK)
    {
        ok_(__FILE__,line)( is_expected_variant( &result, expected ),
                            "got %s expected %s\n", wine_dbgstr_variant(&result), wine_dbgstr_variant(expected) );
    }
    else
    {
        ok_(__FILE__,line)( V_VT(&result) == VT_EMPTY, "Unexpected type %d.\n", V_VT(&result) );
        ok_(__FILE__,line)( V_I4(&result) == 0x12345, "Unexpected value %ld.\n", V_I4(&result) );
    }
}

#define VARMOD(vt1,vt2,val1,val2,rvt,rval)               \
        V_VT(&left) = VT_##vt1; V_##vt1(&left) = val1;   \
        V_VT(&right) = VT_##vt2; V_##vt2(&right) = val2; \
        V_VT(&exp) = VT_##rvt; V_##rvt(&exp) = rval;     \
        test_var_call2( __LINE__, pVarMod, &left, &right, &exp )

#define VARMOD2(vt1,vt2,val1,val2,rvt,rval,hexpected)         \
        V_VT(&left) = VT_##vt1; V_I4(&left) = val1;           \
        V_VT(&right) = VT_##vt2; V_I4(&right) = val2;         \
        V_VT(&exp) = VT_##rvt; V_I4(&exp) = rval;             \
        test_Mod( __LINE__, &left, &right, &exp, hexpected )

static void test_VarMod(void)
{
  VARIANT v1, v2, vDst, left, right, exp;
  HRESULT hres;
  HRESULT hexpected = 0;
  int l, r;
  BOOL lFound, rFound;
  BOOL lValid;
  BSTR strNum0, strNum1;

  CHECKPTR(VarMod);

  VARMOD(I1,BOOL,100,10,I4,0);
  VARMOD(I1,I1,100,10,I4,0);
  VARMOD(I1,UI1,100,10,I4,0);
  VARMOD(I1,I2,100,10,I4,0);
  VARMOD(I1,UI2,100,10,I4,0);
  VARMOD(I1,I4,100,10,I4,0);
  VARMOD(I1,UI4,100,10,I4,0);
  VARMOD(I1,R4,100,10,I4,0);
  VARMOD(I1,R8,100,10,I4,0);

  VARMOD(UI1,BOOL,100,10,I2,0);
  VARMOD(UI1,I1,100,10,I4,0);
  VARMOD(UI1,UI1,100,10,UI1,0);
  VARMOD(UI1,I2,100,10,I2,0);
  VARMOD(UI1,UI2,100,10,I4,0);
  VARMOD(UI1,I4,100,10,I4,0);
  VARMOD(UI1,UI4,100,10,I4,0);
  VARMOD(UI1,R4,100,10,I4,0);
  VARMOD(UI1,R8,100,10,I4,0);

  VARMOD(I2,BOOL,100,10,I2,0);
  VARMOD(I2,I1,100,10,I4,0);
  VARMOD(I2,UI1,100,10,I2,0);
  VARMOD(I2,I2,100,10,I2,0);
  VARMOD(I2,UI2,100,10,I4,0);
  VARMOD(I2,I4,100,10,I4,0);
  VARMOD(I2,UI4,100,10,I4,0);
  VARMOD(I2,R4,100,10,I4,0);
  VARMOD(I2,R8,100,10,I4,0);

  VARMOD(I4,BOOL,100,10,I4,0);
  VARMOD(I4,I1,100,10,I4,0);
  VARMOD(I4,UI1,100,10,I4,0);
  VARMOD(I4,I2,100,10,I4,0);
  VARMOD(I4,UI2,100,10,I4,0);
  VARMOD(I4,I4,100,10,I4,0);
  VARMOD(I4,UI4,100,10,I4,0);
  VARMOD(I4,R4,100,10,I4,0);
  VARMOD(I4,R8,100,10,I4,0);
  VARMOD(UI4,BOOL,100,10,I4,0);
  VARMOD(UI4,I1,100,10,I4,0);
  VARMOD(UI4,UI1,100,10,I4,0);
  VARMOD(UI4,I2,100,10,I4,0);
  VARMOD(UI4,UI2,100,10,I4,0);
  VARMOD(UI4,I4,100,10,I4,0);
  VARMOD(UI4,UI4,100,10,I4,0);
  VARMOD(UI4,R4,100,10,I4,0);
  VARMOD(UI4,R8,100,10,I4,0);
  VARMOD(R4,BOOL,100,10,I4,0);
  VARMOD(R4,I1,100,10,I4,0);
  VARMOD(R4,UI1,100,10,I4,0);
  VARMOD(R4,I2,100,10,I4,0);
  VARMOD(R4,UI2,100,10,I4,0);
  VARMOD(R4,I4,100,10,I4,0);
  VARMOD(R4,UI4,100,10,I4,0);
  VARMOD(R4,R4,100,10,I4,0);
  VARMOD(R4,R8,100,10,I4,0);
  VARMOD(R8,BOOL,100,10,I4,0);
  VARMOD(R8,I1,100,10,I4,0);
  VARMOD(R8,UI1,100,10,I4,0);
  VARMOD(R8,I2,100,10,I4,0);
  VARMOD(R8,UI2,100,10,I4,0);
  VARMOD(R8,I4,100,10,I4,0);
  VARMOD(R8,UI4,100,10,I4,0);
  VARMOD(R8,R4,100,10,I4,0);
  VARMOD(R8,R8,100,10,I4,0);

  VARMOD(INT,INT,100,10,I4,0);
  VARMOD(INT,UINT,100,10,I4,0);

  VARMOD(BOOL,BOOL,100,10,I2,0);
  VARMOD(BOOL,I1,100,10,I4,0);
  VARMOD(BOOL,UI1,100,10,I2,0);
  VARMOD(BOOL,I2,100,10,I2,0);
  VARMOD(BOOL,UI2,100,10,I4,0);
  VARMOD(BOOL,I4,100,10,I4,0);
  VARMOD(BOOL,UI4,100,10,I4,0);
  VARMOD(BOOL,R4,100,10,I4,0);
  VARMOD(BOOL,R8,100,10,I4,0);
  VARMOD(BOOL,DATE,100,10,I4,0);

  VARMOD(DATE,BOOL,100,10,I4,0);
  VARMOD(DATE,I1,100,10,I4,0);
  VARMOD(DATE,UI1,100,10,I4,0);
  VARMOD(DATE,I2,100,10,I4,0);
  VARMOD(DATE,UI2,100,10,I4,0);
  VARMOD(DATE,I4,100,10,I4,0);
  VARMOD(DATE,UI4,100,10,I4,0);
  VARMOD(DATE,R4,100,10,I4,0);
  VARMOD(DATE,R8,100,10,I4,0);
  VARMOD(DATE,DATE,100,10,I4,0);

  strNum0 = SysAllocString(L"125");
  strNum1 = SysAllocString(L"10");
  VARMOD(BSTR,BSTR,strNum0,strNum1,I4,5);
  VARMOD(BSTR,I1,strNum0,10,I4,5);
  VARMOD(BSTR,I2,strNum0,10,I4,5);
  VARMOD(BSTR,I4,strNum0,10,I4,5);
  VARMOD(BSTR,R4,strNum0,10,I4,5);
  VARMOD(BSTR,R8,strNum0,10,I4,5);
  VARMOD(I4,BSTR,125,strNum1,I4,5);

  if (has_i8)
  {
    VARMOD(BOOL,I8,100,10,I8,0);
    VARMOD(I1,I8,100,10,I8,0);
    VARMOD(UI1,I8,100,10,I8,0);
    VARMOD(I2,I8,100,10,I8,0);
    VARMOD(I4,I8,100,10,I8,0);
    VARMOD(UI4,I8,100,10,I8,0);
    VARMOD(R4,I8,100,10,I8,0);
    VARMOD(R8,I8,100,10,I8,0);
    VARMOD(DATE,I8,100,10,I8,0);

    VARMOD(I8,BOOL,100,10,I8,0);
    VARMOD(I8,I1,100,10,I8,0);
    VARMOD(I8,UI1,100,10,I8,0);
    VARMOD(I8,I2,100,10,I8,0);
    VARMOD(I8,UI2,100,10,I8,0);
    VARMOD(I8,I4,100,10,I8,0);
    VARMOD(I8,UI4,100,10,I8,0);
    VARMOD(I8,R4,100,10,I8,0);
    VARMOD(I8,R8,100,10,I8,0);
    VARMOD(I8,I8,100,10,I8,0);

    VARMOD(BSTR,I8,strNum0,10,I8,5);
  }

  /* test all combinations of types */
  for(l = 0; l < VT_BSTR_BLOB; l++)
  {
    SKIPTESTS(l);

    for(r = 0; r < VT_BSTR_BLOB; r++)
    {
      SKIPTESTS(r);
        
      if(l == VT_BSTR) continue;
      if(l == VT_DISPATCH) continue;
      if(r == VT_BSTR) continue;
      if(r == VT_DISPATCH) continue;

      lFound = TRUE;
      lValid = TRUE;
      switch(l)
	{
	case VT_EMPTY:
	case VT_NULL:
	case VT_I1:
	case VT_UI1:
	case VT_I2:
	case VT_UI2:
	case VT_I4:
	case VT_I8:
	case VT_UI4:
	case VT_UI8:
	case VT_INT:
	case VT_UINT:
	case VT_R4:
	case VT_R8:
	case VT_BOOL:
	case VT_DATE:
	case VT_CY:
	case VT_DECIMAL:
	  hexpected = S_OK;
	  break;
	case VT_ERROR:
	case VT_VARIANT:
	case VT_UNKNOWN:
	case VT_RECORD:
	  lValid = FALSE;
	  break;
	default:
	  lFound = FALSE;
	  hexpected = DISP_E_BADVARTYPE;
	  break;
	}

      rFound = TRUE;
      switch(r)
	{
	case VT_EMPTY:
	case VT_NULL:
	case VT_I1:
	case VT_UI1:
	case VT_I2:
	case VT_UI2:
	case VT_I4:
	case VT_I8:
	case VT_UI4:
	case VT_UI8:
	case VT_INT:
	case VT_UINT:
	case VT_R4:
	case VT_R8:
	case VT_BOOL:
	case VT_DATE:
	case VT_DECIMAL:
	case VT_CY:
	  hexpected = S_OK;
	  break;
	case VT_ERROR:
	case VT_VARIANT:
	case VT_UNKNOWN:
	case VT_RECORD:
	  break;
	default:
	  rFound = FALSE;
	  break;
	}

      if(((l == VT_I8) && (r == VT_INT)) || ((l == VT_INT) && (r == VT_I8)))
      {
	hexpected = DISP_E_TYPEMISMATCH;
      } else if((l == VT_EMPTY) && (r == VT_NULL))
      {
	hexpected = S_OK;
      } else if((l == VT_NULL) && (r == VT_EMPTY))
      {
	hexpected = S_OK;
      } else if((l == VT_EMPTY) && (r == VT_CY))
      {
	hexpected = S_OK;
      } else if((l == VT_EMPTY) && (r == VT_RECORD))
      {
	hexpected = DISP_E_TYPEMISMATCH;
      } else if((r == VT_EMPTY) && lFound && lValid)
      {
	hexpected = DISP_E_DIVBYZERO;
      } else if((l == VT_ERROR) || ((r == VT_ERROR) && lFound && lValid))
      {
	hexpected = DISP_E_TYPEMISMATCH;
      } else if((l == VT_NULL) && (r == VT_NULL))
      {
	hexpected = S_OK;
      } else if((l == VT_VARIANT) || ((r == VT_VARIANT) && lFound && lValid))
      {
	hexpected = DISP_E_TYPEMISMATCH;
      } else if((l == VT_NULL) && (r == VT_RECORD))
      {
	hexpected = DISP_E_TYPEMISMATCH;
      } else if((l == VT_I8) && (r == VT_DECIMAL))
      {
	hexpected = S_OK;
      } else if((l == VT_DECIMAL) && (r == VT_I8))
      {
	hexpected = S_OK;
      } else if((l == VT_UNKNOWN) || ((r == VT_UNKNOWN) && lFound && lValid))
      {
	hexpected = DISP_E_TYPEMISMATCH;
      } else if((l == VT_NULL) && rFound)
      {
	hexpected = S_OK;
      } else if(l == VT_RECORD)
      {
	hexpected = DISP_E_TYPEMISMATCH;
      } else if((r == VT_RECORD) && lValid && lFound)
      {
	hexpected = DISP_E_TYPEMISMATCH;
      } else if((l == VT_EMPTY) && (r == VT_EMPTY))
      {
	hexpected = DISP_E_DIVBYZERO;
      } else if((l == VT_CY) && !rFound)
      {
	hexpected = DISP_E_BADVARTYPE;
      } else if(lFound && !rFound)
      {
	hexpected = DISP_E_BADVARTYPE;
      } else if(!lFound && rFound)
      {
	hexpected = DISP_E_BADVARTYPE;
      } else if((r == VT_NULL) && lFound && lValid)
      {
	hexpected = S_OK;
      } else if((l == VT_NULL) || (r == VT_NULL))
      {
	hexpected = DISP_E_BADVARTYPE;
      } else if((l == VT_VARIANT) || (r == VT_VARIANT))
      {
	hexpected = DISP_E_BADVARTYPE;
      } else if(!lFound && !rFound)
      {
	hexpected = DISP_E_BADVARTYPE;
      }

      V_VT(&v1) = l;
      V_VT(&v2) = r;

      if(l == VT_CY)
	V_CY(&v1).int64 = 1000000;
      else if(l == VT_R4)
	V_R4(&v1) = 100;
      else if(l == VT_R8)
	V_R8(&v1) = 100;
      else if(l == VT_UI8)
	V_UI8(&v1) = 100;
      else if(l == VT_I8)
	V_I8(&v1) = 100;
      else if(l == VT_DATE)
	V_DATE(&v1) = 1000;
      else if (l == VT_DECIMAL)
      {
	V_DECIMAL(&v1).Hi32 = 0;
	V_DECIMAL(&v1).Lo64 = 100;
	V_DECIMAL(&v1).sign = 0;
	V_DECIMAL(&v1).scale = 0;
      }
      else
	V_I4(&v1) = 10000;

      if(r == VT_CY)
	V_CY(&v2).int64 = 10000;
      else if(r == VT_R4)
	V_R4(&v2) = 100;
      else if(r == VT_R8)
	V_R8(&v2) = 100;
      else if(r == VT_UI8)
	V_UI8(&v2) = 100;
      else if(r == VT_I8)
	V_I8(&v2) = 100;
      else if(r == VT_DATE)
	V_DATE(&v2) = 1000;
      else if (r == VT_DECIMAL)
      {
	V_DECIMAL(&v2).Hi32 = 0;
	V_DECIMAL(&v2).Lo64 = 100;
	V_DECIMAL(&v2).sign = 0;
	V_DECIMAL(&v2).scale = 0;
      }
      else
	V_I4(&v2) = 10000;

      if ((l != VT_I8 && l != VT_UI8 && r != VT_I8 && r != VT_UI8) || has_i8)
      {
        hres = pVarMod(&v1,&v2,&vDst);
        ok(hres == hexpected,
           "VarMod: expected 0x%lx, got 0x%lX for l type of %d, r type of %d,\n", hexpected, hres, l, r);
      }
    }
  }


  /****************************/
  /* test some bad parameters */
  VARMOD(I4,I4,-1,-1,I4,0);

  /* test modulus with zero */
  VARMOD2(I4,I4,100,0,EMPTY,0,DISP_E_DIVBYZERO);

  VARMOD(I4,I4,0,10,I4,0); /* test 0 mod 10 */

  /* right parameter is type empty */
  VARMOD2(I4,EMPTY,100,10,EMPTY,0,DISP_E_DIVBYZERO);

  /* left parameter is type empty */
  VARMOD2(EMPTY,I4,100,10,I4,0,S_OK);

  /* mod with a null left value */
  VARMOD2(NULL,I4,125,10,NULL,0,S_OK);

  /* mod with a null right value */
  VARMOD2(I4,NULL,100,10,NULL,0,S_OK);

  /* void left value */
  VARMOD2(VOID,I4,100,10,EMPTY,0,DISP_E_BADVARTYPE);

  /* void right value */
  VARMOD2(I4,VOID,100,10,EMPTY,0,DISP_E_BADVARTYPE);

  /* null left value, void right value */
  VARMOD2(NULL,VOID,100,10,EMPTY, 0, DISP_E_BADVARTYPE);

  /* void left value, null right value */
  VARMOD2(VOID,NULL,100,10,EMPTY,0,DISP_E_BADVARTYPE);

  /* some currencies */
  V_VT(&v1) = VT_CY;
  V_VT(&v2) = VT_CY;
  V_CY(&v1).int64 = 100000;
  V_CY(&v2).int64 = 100000;
  hres = pVarMod(&v1,&v2,&vDst);
  ok(hres == S_OK && V_VT(&vDst) == VT_I4 && V_I4(&vDst) == 0,
     "VarMod: expected 0x%lx,%d,%d, got 0x%lX,%d,%ld\n", S_OK, VT_I4, 0, hres, V_VT(&vDst), V_I4(&vDst));

  V_VT(&v1) = VT_I4;
  V_VT(&v2) = VT_CY;
  V_I4(&v1) = 100;
  V_CY(&v2).int64 = 100000;
  hres = pVarMod(&v1,&v2,&vDst);
  ok(hres == S_OK && V_VT(&vDst) == VT_I4 && V_I4(&vDst) == 0,
     "VarMod: expected 0x%lx,%d,%d, got 0x%lX,%d,%ld\n", S_OK, VT_I4, 0, hres, V_VT(&vDst), V_I4(&vDst));

  /* some decimals */
  V_VT(&v1) = VT_DECIMAL;
  V_VT(&v2) = VT_DECIMAL;
  VarDecFromI4(100, &V_DECIMAL(&v1));
  VarDecFromI4(10, &V_DECIMAL(&v2));
  hres = pVarMod(&v1,&v2,&vDst);
  ok(hres == S_OK && V_VT(&vDst) == VT_I4 && V_I4(&vDst) == 0,
     "VarMod: expected 0x%lx,%d,%d, got 0x%lX,%d,%ld\n", S_OK, VT_I4, 0, hres, V_VT(&vDst), V_I4(&vDst));

  V_VT(&v1) = VT_I4;
  V_VT(&v2) = VT_DECIMAL;
  V_I4(&v1) = 100;
  VarDecFromI4(10, &V_DECIMAL(&v2));
  hres = pVarMod(&v1,&v2,&vDst);
  ok(hres == S_OK && V_VT(&vDst) == VT_I4 && V_I4(&vDst) == 0,
     "VarMod: expected 0x%lx,%d,%d, got 0x%lX,%d,%ld\n", S_OK, VT_I4, 0, hres, V_VT(&vDst), V_I4(&vDst));

  VARMOD2(UINT,I4,100,10,I4,0,S_OK);

  /* test that an error results in the type of the result changing but not its value */
  V_VT(&v1) = VT_UNKNOWN;
  V_VT(&v2) = VT_EMPTY;
  V_I4(&v1) = 100;
  V_CY(&v2).int64 = 100000;
  V_VT(&vDst) = VT_I4;
  V_I4(&vDst) = 1231;
  hres = pVarMod(&v1,&v2,&vDst);
  ok(hres == DISP_E_TYPEMISMATCH && V_VT(&vDst) == VT_EMPTY && V_I4(&vDst) == 1231,
     "VarMod: expected 0x%lx,%d,%d, got 0x%lX,%d,%ld\n", DISP_E_TYPEMISMATCH, VT_EMPTY, 1231, hres, V_VT(&vDst), V_I4(&vDst));


  /* test some invalid types */
  /*TODO: not testing VT_DISPATCH */
  if (has_i8)
  {
    VARMOD2(I8,INT,100,10,EMPTY,0,DISP_E_TYPEMISMATCH);
  }
  VARMOD2(ERROR,I4,100,10,EMPTY,0,DISP_E_TYPEMISMATCH);
  VARMOD2(VARIANT,I4,100,10,EMPTY,0,DISP_E_TYPEMISMATCH);
  VARMOD2(UNKNOWN,I4,100,10,EMPTY,0,DISP_E_TYPEMISMATCH);
  VARMOD2(VOID,I4,100,10,EMPTY,0,DISP_E_BADVARTYPE);
  VARMOD2(HRESULT,I4,100,10,EMPTY,0,DISP_E_BADVARTYPE);
  VARMOD2(PTR,I4,100,10,EMPTY,0,DISP_E_BADVARTYPE);
  VARMOD2(SAFEARRAY,I4,100,10,EMPTY,0,DISP_E_BADVARTYPE);
  VARMOD2(CARRAY,I4,100,10,EMPTY,0,DISP_E_BADVARTYPE);
  VARMOD2(USERDEFINED,I4,100,10,EMPTY,0,DISP_E_BADVARTYPE);
  VARMOD2(LPSTR,I4,100,10,EMPTY,0,DISP_E_BADVARTYPE);
  VARMOD2(LPWSTR,I4,100,10,EMPTY,0,DISP_E_BADVARTYPE);
  VARMOD2(RECORD,I4,100,10,EMPTY,0,DISP_E_TYPEMISMATCH);
  VARMOD2(FILETIME,I4,100,10,EMPTY,0,DISP_E_BADVARTYPE);
  VARMOD2(BLOB,I4,100,10,EMPTY,0,DISP_E_BADVARTYPE);
  VARMOD2(STREAM,I4,100,10,EMPTY,0,DISP_E_BADVARTYPE);
  VARMOD2(STORAGE,I4,100,10,EMPTY,0,DISP_E_BADVARTYPE);
  VARMOD2(STREAMED_OBJECT,I4,100,10,EMPTY,0,DISP_E_BADVARTYPE);
  VARMOD2(STORED_OBJECT,I4,100,10,EMPTY,0,DISP_E_BADVARTYPE);
  VARMOD2(BLOB_OBJECT,I4,100,10,EMPTY,0,DISP_E_BADVARTYPE);
  VARMOD2(CF,I4,100,10,EMPTY,0,DISP_E_BADVARTYPE);
  VARMOD2(CLSID,CLSID,100,10,EMPTY,0,DISP_E_BADVARTYPE);
  VARMOD2(VECTOR,I4,100,10,EMPTY,0,DISP_E_BADVARTYPE);
  VARMOD2(ARRAY,I4,100,10,EMPTY,0,DISP_E_BADVARTYPE);
  VARMOD2(BYREF,I4,100,10,EMPTY,0,DISP_E_BADVARTYPE);

  /* test some more invalid types */
  V_VT(&v1) = 456;
  V_VT(&v2) = 234;
  V_I4(&v1) = 100;
  V_I4(&v2)=  10;
  hres = pVarMod(&v1,&v2,&vDst);
  ok(hres == DISP_E_BADVARTYPE && V_VT(&vDst) == VT_EMPTY,
     "VarMod: expected 0x%lx,%d, got 0x%lX,%d\n", DISP_E_BADVARTYPE, VT_EMPTY, hres, V_VT(&vDst));

  SysFreeString(strNum0);
  SysFreeString(strNum1);
}

static HRESULT (WINAPI *pVarFix)(LPVARIANT,LPVARIANT);

#define VARFIX(vt,val,rvt,rval)                  \
    V_VT(&v) = VT_##vt; V_##vt(&v) = val;        \
    V_VT(&exp) = VT_##rvt; V_##rvt(&exp) = rval; \
    test_var_call1( __LINE__, pVarFix, &v, &exp )

static void test_VarFix(void)
{
    HRESULT hres;
    VARIANT v, exp, vDst;
    DECIMAL *pdec = &V_DECIMAL(&v);
    CY *pcy = &V_CY(&v);
    size_t i;

    CHECKPTR(VarFix);

    /* Test all possible V_VT values */
    for (i = 0; i < ARRAY_SIZE(ExtraFlags); i++)
    {
        VARTYPE vt;

        for (vt = 0; vt <= VT_BSTR_BLOB; vt++)
        {
            BOOL bFail = TRUE;

            SKIPTESTS(vt);

            memset(&v, 0, sizeof(v));
            V_VT(&v) = vt | ExtraFlags[i];
            V_VT(&vDst) = VT_EMPTY;

            switch (V_VT(&v))
            {
              case VT_UI1: case VT_I2: case VT_I4: case VT_R4:  case VT_R8:
              case VT_DECIMAL: case VT_BOOL: case VT_NULL: case VT_EMPTY:
              case VT_DATE: case VT_CY:
                bFail = FALSE;
                break;
              case VT_I8:
                if (has_i8)
                  bFail = FALSE;
                break;
            }

            hres = pVarFix(&v,&vDst);
            if (bFail)
              ok(hres == DISP_E_TYPEMISMATCH || hres == DISP_E_BADVARTYPE,
                 "VarFix: expected failure, got 0x%lX vt %d|0x%X\n",
                 hres, vt, ExtraFlags[i]);
            else
                 ok(hres == S_OK, "VarFix: expected S_OK, got 0x%lX vt %d|0x%X\n",
                    hres, vt, ExtraFlags[i]);
        }
    }

    VARFIX(BOOL,VARIANT_TRUE,I2,VARIANT_TRUE);
    VARFIX(BOOL,VARIANT_FALSE,I2,0);
    VARFIX(BOOL,1,I2,1);
    VARFIX(UI1,1,UI1,1);
    VARFIX(I2,-1,I2,-1);
    VARFIX(I4,-1,I4,-1);
    if (has_i8)
    {
        VARFIX(I8,-1,I8,-1);
    }
    VARFIX(R4,1.4f,R4,1);
    VARFIX(R4,1.5f,R4,1);
    VARFIX(R4,1.6f,R4,1);
    VARFIX(R4,-1.4f,R4,-1);
    VARFIX(R4,-1.5f,R4,-1);
    VARFIX(R4,-1.6f,R4,-1);
    /* DATE & R8 round as for R4 */
    VARFIX(DATE,-1,DATE,-1);
    VARFIX(R8,-1,R8,-1);
    VARFIX(BSTR,(BSTR)L"-1",R8,-1);

    V_VT(&v) = VT_EMPTY;
    hres = pVarFix(&v,&vDst);
    ok(hres == S_OK && V_VT(&vDst) == VT_I2 && V_I2(&vDst) == 0,
       "VarFix: expected 0x0,%d,0 got 0x%lX,%d,%d\n", VT_EMPTY,
       hres, V_VT(&vDst), V_I2(&vDst));

    V_VT(&v) = VT_NULL;
    hres = pVarFix(&v,&vDst);
    ok(hres == S_OK && V_VT(&vDst) == VT_NULL,
       "VarFix: expected 0x0,%d got 0x%lX,%d\n", VT_NULL, hres, V_VT(&vDst));

    V_VT(&v) = VT_DECIMAL;
    pdec->sign = DECIMAL_NEG;
    pdec->scale = 0;
    pdec->Hi32 = 0;
    pdec->Mid32 = 0;
    pdec->Lo32 = 1;
    hres = pVarFix(&v,&vDst);
    ok(hres == S_OK && V_VT(&vDst) == VT_DECIMAL && !memcmp(&V_DECIMAL(&v), &V_DECIMAL(&vDst), sizeof(DECIMAL)),
       "VarFix: expected 0x0,%d,identical, got 0x%lX,%d\n", VT_DECIMAL,
       hres, V_VT(&vDst));

    /* FIXME: Test some fractional decimals when VarDecFix is implemented */

    V_VT(&v) = VT_CY;
    pcy->int64 = -10000;
    hres = pVarFix(&v,&vDst);
    ok(hres == S_OK && V_VT(&vDst) == VT_CY && V_CY(&vDst).int64 == -10000,
       "VarFix: VT_CY wrong, hres=0x%lX\n", hres);

    V_VT(&v) = VT_CY;
    pcy->int64 = -16000;
    hres = pVarFix(&v,&vDst);
    ok(hres == S_OK && V_VT(&vDst) == VT_CY && V_CY(&vDst).int64 == -10000,
       "VarFix: VT_CY wrong, hres=0x%lX\n", hres);
}

static HRESULT (WINAPI *pVarInt)(LPVARIANT,LPVARIANT);

#define VARINT(vt,val,rvt,rval)                  \
    V_VT(&v) = VT_##vt; V_##vt(&v) = val;        \
    V_VT(&exp) = VT_##rvt; V_##rvt(&exp) = rval; \
    test_var_call1( __LINE__, pVarInt, &v, &exp )

static void test_VarInt(void)
{
    HRESULT hres;
    VARIANT v, exp, vDst;
    DECIMAL *pdec = &V_DECIMAL(&v);
    CY *pcy = &V_CY(&v);
    size_t i;

    CHECKPTR(VarInt);

    /* Test all possible V_VT values */
    for (i = 0; i < ARRAY_SIZE(ExtraFlags); i++)
    {
        VARTYPE vt;

        for (vt = 0; vt <= VT_BSTR_BLOB; vt++)
        {
            BOOL bFail = TRUE;

            SKIPTESTS(vt);

            memset(&v, 0, sizeof(v));
            V_VT(&v) = vt | ExtraFlags[i];
            V_VT(&vDst) = VT_EMPTY;

            switch (V_VT(&v))
            {
              case VT_UI1: case VT_I2: case VT_I4: case VT_R4:  case VT_R8:
              case VT_DECIMAL: case VT_BOOL: case VT_NULL: case VT_EMPTY:
              case VT_DATE: case VT_CY:
                bFail = FALSE;
                break;
              case VT_I8:
                if (has_i8)
                  bFail = FALSE;
                break;
            }

            hres = pVarInt(&v,&vDst);
            if (bFail)
              ok(hres == DISP_E_TYPEMISMATCH || hres == DISP_E_BADVARTYPE,
                 "VarInt: expected failure, got 0x%lX vt %d|0x%X\n",
                 hres, vt, ExtraFlags[i]);
            else
                 ok(hres == S_OK, "VarInt: expected S_OK, got 0x%lX vt %d|0x%X\n",
                    hres, vt, ExtraFlags[i]);
        }
    }

    VARINT(BOOL,VARIANT_TRUE,I2,VARIANT_TRUE);
    VARINT(BOOL,VARIANT_FALSE,I2,0);
    VARINT(BOOL,1,I2,1);
    VARINT(UI1,1,UI1,1);
    VARINT(I2,-1,I2,-1);
    VARINT(I4,-1,I4,-1);
    if (has_i8)
    {
        VARINT(I8,-1,I8,-1);
    }
    VARINT(R4,1.4f,R4,1);
    VARINT(R4,1.5f,R4,1);
    VARINT(R4,1.6f,R4,1);
    VARINT(R4,-1.4f,R4,-2); /* Note these 3 are different from VarFix */
    VARINT(R4,-1.5f,R4,-2);
    VARINT(R4,-1.6f,R4,-2);
    /* DATE & R8 round as for R4 */
    VARINT(DATE,-1,DATE,-1);
    VARINT(R8,-1,R8,-1);
    VARINT(BSTR,(BSTR)L"-1",R8,-1);

    V_VT(&v) = VT_EMPTY;
    hres = pVarInt(&v,&vDst);
    ok(hres == S_OK && V_VT(&vDst) == VT_I2 && V_I2(&vDst) == 0,
       "VarInt: expected 0x0,%d,0 got 0x%lX,%d,%d\n", VT_EMPTY,
       hres, V_VT(&vDst), V_I2(&vDst));

    V_VT(&v) = VT_NULL;
    hres = pVarInt(&v,&vDst);
    ok(hres == S_OK && V_VT(&vDst) == VT_NULL,
       "VarInt: expected 0x0,%d got 0x%lX,%d\n", VT_NULL, hres, V_VT(&vDst));

    V_VT(&v) = VT_DECIMAL;
    pdec->sign = DECIMAL_NEG;
    pdec->scale = 0;
    pdec->Hi32 = 0;
    pdec->Mid32 = 0;
    pdec->Lo32 = 1;
    hres = pVarInt(&v,&vDst);
    ok(hres == S_OK && V_VT(&vDst) == VT_DECIMAL && !memcmp(&V_DECIMAL(&v), &V_DECIMAL(&vDst), sizeof(DECIMAL)),
       "VarInt: expected 0x0,%d,identical, got 0x%lX,%d\n", VT_DECIMAL,
       hres, V_VT(&vDst));

    /* FIXME: Test some fractional decimals when VarDecInt is implemented */

    V_VT(&v) = VT_CY;
    pcy->int64 = -10000;
    hres = pVarInt(&v,&vDst);
    ok(hres == S_OK && V_VT(&vDst) == VT_CY && V_CY(&vDst).int64 == -10000,
       "VarInt: VT_CY wrong, hres=0x%lX\n", hres);

    V_VT(&v) = VT_CY;
    pcy->int64 = -11000;
    hres = pVarInt(&v,&vDst);
    ok(hres == S_OK && V_VT(&vDst) == VT_CY && V_CY(&vDst).int64 == -20000,
       "VarInt: VT_CY wrong, hres=%#lX %#I64x\n",
       hres, V_CY(&vDst).int64);
}

static HRESULT (WINAPI *pVarNeg)(LPVARIANT,LPVARIANT);

#define VARNEG(vt,val,rvt,rval)                  \
    V_VT(&v) = VT_##vt; V_##vt(&v) = val;        \
    V_VT(&exp) = VT_##rvt; V_##rvt(&exp) = rval; \
    test_var_call1( __LINE__, pVarNeg, &v, &exp )

static void test_VarNeg(void)
{
    HRESULT hres;
    VARIANT v, exp, vDst;
    DECIMAL *pdec = &V_DECIMAL(&v);
    CY *pcy = &V_CY(&v);
    size_t i;

    CHECKPTR(VarNeg);

    /* Test all possible V_VT values. But don't test the exact return values
     * except for success/failure, since M$ made a hash of them in the
     * native version. This at least ensures (as with all tests here) that
     * we will notice if/when new vtypes/flags are added in native.
     */
    for (i = 0; i < ARRAY_SIZE(ExtraFlags); i++)
    {
        VARTYPE vt;

        for (vt = 0; vt <= VT_BSTR_BLOB; vt++)
        {
            BOOL bFail = TRUE;

            SKIPTESTS(vt);

            memset(&v, 0, sizeof(v));
            V_VT(&v) = vt | ExtraFlags[i];
            V_VT(&vDst) = VT_EMPTY;

            switch (V_VT(&v))
            {
            case VT_UI1: case VT_I2: case VT_I4:
            case VT_R4:  case VT_R8:
            case VT_DECIMAL: case VT_BOOL: case VT_NULL: case VT_EMPTY:
            case VT_DATE: case VT_CY:
                bFail = FALSE;
                break;
            case VT_I8:
                if (has_i8)
                    bFail = FALSE;
            }

            hres = pVarNeg(&v,&vDst);
            if (bFail)
                ok(hres == DISP_E_TYPEMISMATCH || hres == DISP_E_BADVARTYPE,
                   "VarNeg: expected failure, got 0x%lX vt %d|0x%X\n",
                   hres, vt, ExtraFlags[i]);
            else
                ok(hres == S_OK, "VarNeg: expected S_OK, got 0x%lX vt %d|0x%X\n",
                    hres, vt, ExtraFlags[i]);
        }
    }

    VARNEG(BOOL,VARIANT_TRUE,I2,1);
    VARNEG(BOOL,VARIANT_FALSE,I2,0);
    VARNEG(BOOL,1,I2,-1);
    VARNEG(UI1,1,I2,-1);
    VARNEG(UI1,254,I2,-254);
    VARNEG(I2,-32768,I4,32768);
    VARNEG(I2,-1,I2,1);
    VARNEG(I2,1,I2,-1);
    VARNEG(I4,-((int)(~0u >> 1)) - 1,R8,-2147483648u);
    VARNEG(I4,-1,I4,1);
    VARNEG(I4,1,I4,-1);
    if (has_i8)
    {
        VARNEG(I8,1,I8,-1);
        VARNEG(I8,-1,I8,1);
    }
    VARNEG(R4,1,R4,-1);
    VARNEG(R4,-1,R4,1);
    VARNEG(DATE,1,DATE,-1);
    VARNEG(DATE,-1,DATE,1);
    VARNEG(R8,1,R8,-1);
    VARNEG(R8,-1,R8,1);
    VARNEG(BSTR,(BSTR)L"-1",R8,1);
    VARNEG(BSTR,(BSTR)L"1",R8,-1);

    V_VT(&v) = VT_EMPTY;
    hres = pVarNeg(&v,&vDst);
    ok(hres == S_OK && V_VT(&vDst) == VT_I2 && V_I2(&vDst) == 0,
       "VarNeg: expected 0x0,%d,0 got 0x%lX,%d,%d\n", VT_EMPTY,
       hres, V_VT(&vDst), V_I2(&vDst));

    V_VT(&v) = VT_NULL;
    hres = pVarNeg(&v,&vDst);
    ok(hres == S_OK && V_VT(&vDst) == VT_NULL,
       "VarNeg: expected 0x0,%d got 0x%lX,%d\n", VT_NULL, hres, V_VT(&vDst));

    V_VT(&v) = VT_DECIMAL;
    pdec->sign = DECIMAL_NEG;
    pdec->scale = 0;
    pdec->Hi32 = 0;
    pdec->Mid32 = 0;
    pdec->Lo32 = 1;
    hres = pVarNeg(&v,&vDst);
    ok(hres == S_OK && V_VT(&vDst) == VT_DECIMAL &&
       V_DECIMAL(&vDst).sign == 0,
       "VarNeg: expected 0x0,%d,0x00, got 0x%lX,%d,%02x\n", VT_DECIMAL,
       hres, V_VT(&vDst), V_DECIMAL(&vDst).sign);

    pdec->sign = 0;
    hres = pVarNeg(&v,&vDst);
    ok(hres == S_OK && V_VT(&vDst) == VT_DECIMAL &&
       V_DECIMAL(&vDst).sign == DECIMAL_NEG,
       "VarNeg: expected 0x0,%d,0x7f, got 0x%lX,%d,%02x\n", VT_DECIMAL,
       hres, V_VT(&vDst), V_DECIMAL(&vDst).sign);

    V_VT(&v) = VT_CY;
    pcy->int64 = -10000;
    hres = pVarNeg(&v,&vDst);
    ok(hres == S_OK && V_VT(&vDst) == VT_CY && V_CY(&vDst).int64 == 10000,
       "VarNeg: VT_CY wrong, hres=0x%lX\n", hres);
}

static void test_Round( int line, VARIANT *arg, int deci, VARIANT *expected )
{
    VARIANT result;
    HRESULT hres;

    memset( &result, 0, sizeof(result) );
    hres = VarRound( arg, deci, &result );
    ok_(__FILE__,line)( hres == S_OK, "wrong result %lx\n", hres );
    if (hres == S_OK)
        ok_(__FILE__,line)( is_expected_variant( &result, expected ),
                            "got %s expected %s\n", wine_dbgstr_variant(&result), wine_dbgstr_variant(expected) );
}
#define VARROUND(vt,val,deci,rvt,rval)           \
    V_VT(&v) = VT_##vt; V_##vt(&v) = val;        \
    V_VT(&exp) = VT_##rvt; V_##rvt(&exp) = rval; \
    test_Round( __LINE__, &v, deci, &exp )

struct decimal_t {
    BYTE scale;
    BYTE sign;
    ULONG Hi32;
    ULONG Mid32;
    ULONG Lo32;
};

struct decimal_round_t {
    struct decimal_t source;
    struct decimal_t ret;
    int dec;
};

static const struct decimal_round_t decimal_round_data[] = {
    {{ 0, DECIMAL_NEG, 0, 0, 1 }, { 0, DECIMAL_NEG, 0, 0, 1 }, 0},
    {{ 0, 0, 0, 0, 1 }, { 0, 0, 0, 0, 1 }, 0},
    {{ 2, 0, 0, 0, 155 }, { 0, 0, 0, 0, 16 }, 1},
    {{ 2, 0, 0, 0, 155 }, { 1, 0, 0, 0, 2 }, 0},
    {{ 2, 0, 0, 0, 199 }, { 1, 0, 0, 0, 20 }, 1},
    {{ 2, 0, 0, 0, 199 }, { 2, 0, 0, 0, 199 }, 2},
    {{ 2, DECIMAL_NEG, 0, 0, 199 }, { 2, DECIMAL_NEG, 0, 0, 199 }, 2},
    {{ 2, DECIMAL_NEG, 0, 0, 55 },  { 2, DECIMAL_NEG, 0, 0, 6 }, 1},
    {{ 2, 0, 0, 0, 55 },  { 2, 0, 0, 0, 6 }, 1},
    {{ 2, 0, 0, 0, 1999 }, { 1, 0, 0, 0, 200 }, 1},
};

static void test_VarRound(void)
{
    static WCHAR szNumMin[] = {'-','1','.','4','4','9','\0' };
    static WCHAR szNum[] = {'1','.','4','5','1','\0' };
    HRESULT hres;
    VARIANT v, exp, vDst;
    CY *pcy = &V_CY(&v);
    char buff[8];
    int i;

    /* first check valid integer types */
    VARROUND(BOOL,VARIANT_TRUE,0,I2,-1);
    VARROUND(BOOL,VARIANT_FALSE,0,I2,0);
    VARROUND(BOOL,1,0,I2,1);
    VARROUND(UI1,1,0,UI1,1);
    VARROUND(UI1,254,0,UI1,254);
    VARROUND(I2,-32768,0,I2,-32768);
    VARROUND(I2,-1,0,I2,-1);
    VARROUND(I2,1,0,I2,1);
    VARROUND(I4,-((int)(~0u >> 1)) - 1,0,I4,-((int)(~0u >> 1)) - 1);
    VARROUND(I4,-1,0,I4,-1);
    VARROUND(I4,1,0,I4,1);


    /* MSDN states that rounding of R4/R8 is dependent on the underlying
     * bit pattern of the number and so is architecture dependent. In this
     * case Wine returns .2 (which is more correct) and Native returns .3
     */

    VARROUND(R4,1.0f,0,R4,1.0f);
    VARROUND(R4,-1.0f,0,R4,-1.0f);
    VARROUND(R8,1.0,0,R8,1.0);
    VARROUND(R8,-1.0,0,R8,-1.0);

    /* floating point numbers aren't exactly equal and we can't just
     * compare the first few digits. */
    VARROUND(DATE,1.451,1,DATE,1.5);
    VARROUND(DATE,-1.449,1,DATE,-1.4);

    /* replace the decimal separator */
    GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, buff, ARRAY_SIZE(buff));
    if (!buff[1]) {
        szNumMin[2] = buff[0];
        szNum[1] = buff[0];
        VARROUND(BSTR,(BSTR)szNumMin,1,R8,-1.40);
        VARROUND(BSTR,(BSTR)szNum,1,R8,1.50);
    } else {
        skip("Skipping VarRound(BSTR) as decimal separator is '%s'\n", buff);
    }

    VARROUND(R4,1.23456f,0,R4,1.0f);
    VARROUND(R4,1.23456f,1,R4,1.2f);
    VARROUND(R4,1.23456f,2,R4,1.23f);
    VARROUND(R4,1.23456f,3,R4,1.235f);
    VARROUND(R4,1.23456f,4,R4,1.2346f);
    VARROUND(R4,-1.23456f,0,R4,-1.0f);
    VARROUND(R4,-1.23456f,1,R4,-1.2f);
    VARROUND(R4,-1.23456f,2,R4,-1.23f);
    VARROUND(R4,-1.23456f,3,R4,-1.235f);
    VARROUND(R4,-1.23456f,4,R4,-1.2346f);

    VARROUND(R8,1.23456,0,R8,1.0);
    VARROUND(R8,1.23456,1,R8,1.2);
    VARROUND(R8,1.23456,2,R8,1.23);
    VARROUND(R8,1.23456,3,R8,1.235);
    VARROUND(R8,1.23456,4,R8,1.2346);
    VARROUND(R8,-1.23456,0,R8,-1.0);
    VARROUND(R8,-1.23456,1,R8,-1.2);
    VARROUND(R8,-1.23456,2,R8,-1.23);
    VARROUND(R8,-1.23456,3,R8,-1.235);
    VARROUND(R8,-1.23456,4,R8,-1.2346);

    V_VT(&v) = VT_EMPTY;
    hres = VarRound(&v,0,&vDst);
    ok(hres == S_OK && V_VT(&vDst) == VT_I2 && V_I2(&vDst) == 0,
        "VarRound: expected 0x0,%d,0 got 0x%lX,%d,%d\n", VT_EMPTY,
        hres, V_VT(&vDst), V_I2(&vDst));

    V_VT(&v) = VT_NULL;
    hres = VarRound(&v,0,&vDst);
    ok(hres == S_OK && V_VT(&vDst) == VT_NULL,
        "VarRound: expected 0x0,%d got 0x%lX,%d\n", VT_NULL, hres, V_VT(&vDst));

    /* VT_DECIMAL */
    for (i = 0; i < ARRAY_SIZE(decimal_round_data); i++)
    {
        const struct decimal_round_t *ptr = &decimal_round_data[i];
        DECIMAL *pdec;

        pdec = &V_DECIMAL(&v);
        V_VT(&v) = VT_DECIMAL;
        pdec->sign = ptr->source.sign;
        pdec->scale = ptr->source.scale;
        pdec->Hi32 = ptr->source.Hi32;
        pdec->Mid32 = ptr->source.Mid32;
        pdec->Lo32 = ptr->source.Lo32;
        VariantInit(&vDst);
        hres = VarRound(&v, ptr->dec, &vDst);
        ok(hres == S_OK, "%d: got 0x%08lx\n", i, hres);
        if (hres == S_OK)
        {
            ok(V_VT(&vDst) == VT_DECIMAL, "%d: got VT %d, expected VT_DECIMAL\n", i, V_VT(&vDst));
            ok(V_DECIMAL(&vDst).sign == ptr->ret.sign, "%d: got sign 0x%02x, expected 0x%02x\n",
                i, V_DECIMAL(&vDst).sign, ptr->ret.sign);
            ok(V_DECIMAL(&vDst).Hi32 == ptr->ret.Hi32, "%d: got Hi32 %ld, expected %ld\n",
                i, V_DECIMAL(&vDst).Hi32, ptr->ret.Hi32);
            ok(V_DECIMAL(&vDst).Mid32 == ptr->ret.Mid32, "%d: got Mid32 %ld, expected %ld\n",
               i, V_DECIMAL(&vDst).Mid32,  ptr->ret.Mid32);
            ok(V_DECIMAL(&vDst).Lo32 == ptr->ret.Lo32, "%d: got Lo32 %ld, expected %ld\n",
                i, V_DECIMAL(&vDst).Lo32, ptr->ret.Lo32);
        }
    }

    /* VT_CY */
    V_VT(&v) = VT_CY;
    pcy->int64 = 10000;
    hres = VarRound(&v,0,&vDst);
    ok(hres == S_OK && V_VT(&vDst) == VT_CY && V_CY(&vDst).int64 == 10000,
        "VarRound: VT_CY wrong, hres=0x%lX\n", hres);
}

static HRESULT (WINAPI *pVarXor)(LPVARIANT,LPVARIANT,LPVARIANT);

#define VARXOR(vt1,val1,vt2,val2,rvt,rval)               \
        V_VT(&left) = VT_##vt1; V_##vt1(&left) = val1;   \
        V_VT(&right) = VT_##vt2; V_##vt2(&right) = val2; \
        V_VT(&exp) = VT_##rvt; V_##rvt(&exp) = rval;     \
        test_var_call2( __LINE__, pVarXor, &left, &right, &exp )

#define VARXORCY(vt1,val1,val2,rvt,rval)                 \
        V_VT(&left) = VT_##vt1; V_##vt1(&left) = val1;   \
        V_VT(&right) = VT_CY; V_CY(&right).int64 = val2; \
        V_VT(&exp) = VT_##rvt; V_##rvt(&exp) = rval;     \
        test_var_call2( __LINE__, pVarXor, &left, &right, &exp )

static void test_VarXor(void)
{
    static const WCHAR szFalse[] = { '#','F','A','L','S','E','#','\0' };
    static const WCHAR szTrue[] = { '#','T','R','U','E','#','\0' };
    VARIANT left, right, exp, result;
    BSTR lbstr, rbstr;
    VARTYPE i;
    HRESULT hres;

    CHECKPTR(VarXor);

    /* Test all possible flag/vt combinations & the resulting vt type */
    for (i = 0; i < ARRAY_SIZE(ExtraFlags); i++)
    {
        VARTYPE leftvt, rightvt, resvt;

        for (leftvt = 0; leftvt <= VT_BSTR_BLOB; leftvt++)
        {

            SKIPTESTS(leftvt);
                    
            for (rightvt = 0; rightvt <= VT_BSTR_BLOB; rightvt++)
            {
                BOOL bFail = FALSE;

                SKIPTESTS(rightvt);
                
                if (leftvt == VT_BSTR || rightvt == VT_BSTR ||
                    leftvt == VT_DISPATCH || rightvt == VT_DISPATCH ||
                    leftvt == VT_UNKNOWN || rightvt == VT_UNKNOWN)
                    continue;

                memset(&left, 0, sizeof(left));
                memset(&right, 0, sizeof(right));
                V_VT(&left) = leftvt | ExtraFlags[i];
                V_VT(&right) = rightvt | ExtraFlags[i];
                V_VT(&result) = VT_EMPTY;
                resvt = VT_I4;

                if (ExtraFlags[i] & VT_ARRAY || ExtraFlags[i] & VT_BYREF ||
                    !IsValidVariantClearVT(leftvt, ExtraFlags[i]) ||
                    !IsValidVariantClearVT(rightvt, ExtraFlags[i]) ||
                    leftvt == VT_CLSID || rightvt == VT_CLSID ||
                    leftvt == VT_RECORD || rightvt == VT_RECORD ||
                    leftvt == VT_VARIANT || rightvt == VT_VARIANT ||
                    leftvt == VT_ERROR || rightvt == VT_ERROR)
                {
                    bFail = TRUE;
                }
                if (leftvt == VT_EMPTY || rightvt == VT_EMPTY)
                {
                    if (leftvt == rightvt ||
                        leftvt == VT_I2 || rightvt == VT_I2 ||
                        leftvt == VT_UI1 || rightvt == VT_UI1 ||
                        leftvt == VT_BOOL || rightvt == VT_BOOL)
                        resvt = VT_I2;
                    else if (leftvt == VT_NULL || rightvt == VT_NULL)
                        resvt = VT_NULL;
                    else if (leftvt == VT_I8 || rightvt == VT_I8)
                        resvt = VT_I8;
                }
                else if (leftvt == VT_NULL || rightvt == VT_NULL)
                {
                    resvt = VT_NULL;
                }
                else if (leftvt == VT_UI1 || rightvt == VT_UI1)
                {
                    if (leftvt == rightvt)
                        resvt = VT_UI1;
                    else if (leftvt == rightvt ||
                        leftvt == VT_I2 || rightvt == VT_I2 ||
                        leftvt == VT_BOOL || rightvt == VT_BOOL)
                    {
                        resvt = VT_I2;
                    }
                    else if (leftvt == VT_I8 || rightvt == VT_I8)
                        resvt = VT_I8;
                }
                else if (leftvt == VT_I2 || rightvt == VT_I2)
                {
                    if (leftvt == rightvt ||
                        leftvt == VT_BOOL || rightvt == VT_BOOL)
                        resvt = VT_I2;
                    else if (leftvt == VT_I8 || rightvt == VT_I8)
                        resvt = VT_I8;
                }
                else if (leftvt == VT_BOOL && rightvt == VT_BOOL)
                {
                    resvt = VT_BOOL;
                }
                else if (leftvt == VT_I8 || rightvt == VT_I8)
                {
                    if (leftvt == VT_INT || rightvt == VT_INT)
                        bFail = TRUE;
                    else
                        resvt = VT_I8;
                }
                hres = pVarXor(&left, &right, &result);
                if (bFail)
                    ok(hres == DISP_E_TYPEMISMATCH || hres == DISP_E_BADVARTYPE,
                       "VarXor: %d|0x%X, %d|0x%X: Expected failure, got 0x%lX vt %d\n",
                       leftvt, ExtraFlags[i], rightvt, ExtraFlags[i], hres,
                       V_VT(&result));
                else
                    ok(hres == S_OK && V_VT(&result) == resvt,
                       "VarXor: %d|0x%X, %d|0x%X: expected S_OK, vt %d, got 0x%lX vt %d\n",
                       leftvt, ExtraFlags[i], rightvt, ExtraFlags[i], resvt, hres,
                       V_VT(&result));
            }
        }
    }

    /* Test returned values
     * FIXME: Test VT_DECIMAL/VT_DISPATCH
     */
    VARXOR(EMPTY,0,EMPTY,0,I2,0);
    VARXOR(EMPTY,1,EMPTY,0,I2,0);
    VARXOR(EMPTY,0,NULL,0,NULL,0);
    VARXOR(EMPTY,0,I1,0,I4,0);
    VARXOR(EMPTY,0,I1,1,I4,1);
    VARXOR(EMPTY,0,UI1,0,I2,0);
    VARXOR(EMPTY,0,UI1,1,I2,1);
    VARXOR(EMPTY,0,I2,0,I2,0);
    VARXOR(EMPTY,0,I2,1,I2,1);
    VARXOR(EMPTY,0,UI2,0,I4,0);
    VARXOR(EMPTY,0,UI2,1,I4,1);
    VARXOR(EMPTY,0,I4,0,I4,0);
    VARXOR(EMPTY,0,I4,1,I4,1);
    VARXOR(EMPTY,0,UI4,0,I4,0);
    VARXOR(EMPTY,0,UI4,1,I4,1);
    if (has_i8)
    {
        VARXOR(EMPTY,0,I8,0,I8,0);
        VARXOR(EMPTY,0,I8,1,I8,1);
        VARXOR(EMPTY,0,UI8,0,I4,0);
        VARXOR(EMPTY,0,UI8,1,I4,1);
    }
    VARXOR(EMPTY,0,INT,0,I4,0);
    VARXOR(EMPTY,0,INT,1,I4,1);
    VARXOR(EMPTY,0,UINT,0,I4,0);
    VARXOR(EMPTY,0,UINT,1,I4,1);
    VARXOR(EMPTY,0,BOOL,0,I2,0);
    VARXOR(EMPTY,0,BOOL,1,I2,1);
    VARXOR(EMPTY,0,R4,0,I4,0);
    VARXOR(EMPTY,0,R4,1,I4,1);
    VARXOR(EMPTY,0,R8,0,I4,0);
    VARXOR(EMPTY,0,R8,1,I4,1);
    rbstr = SysAllocString(szFalse);
    VARXOR(EMPTY,0,BSTR,rbstr,I2,0);
    SysFreeString(rbstr);
    rbstr = SysAllocString(szTrue);
    VARXOR(EMPTY,0,BSTR,rbstr,I2,-1);
    VARXORCY(EMPTY,0,10000,I4,1);
    SysFreeString(rbstr);

    /* NULL OR 0 = NULL. NULL OR n = n */
    VARXOR(NULL,0,NULL,0,NULL,0);
    VARXOR(NULL,1,NULL,0,NULL,0);
    VARXOR(NULL,0,I1,0,NULL,0);
    VARXOR(NULL,0,I1,1,NULL,0);
    VARXOR(NULL,0,UI1,0,NULL,0);
    VARXOR(NULL,0,UI1,1,NULL,0);
    VARXOR(NULL,0,I2,0,NULL,0);
    VARXOR(NULL,0,I2,1,NULL,0);
    VARXOR(NULL,0,UI2,0,NULL,0);
    VARXOR(NULL,0,UI2,1,NULL,0);
    VARXOR(NULL,0,I4,0,NULL,0);
    VARXOR(NULL,0,I4,1,NULL,0);
    VARXOR(NULL,0,UI4,0,NULL,0);
    VARXOR(NULL,0,UI4,1,NULL,0);
    if (has_i8)
    {
        VARXOR(NULL,0,I8,0,NULL,0);
        VARXOR(NULL,0,I8,1,NULL,0);
        VARXOR(NULL,0,UI8,0,NULL,0);
        VARXOR(NULL,0,UI8,1,NULL,0);
    }
    VARXOR(NULL,0,INT,0,NULL,0);
    VARXOR(NULL,0,INT,1,NULL,0);
    VARXOR(NULL,0,UINT,0,NULL,0);
    VARXOR(NULL,0,UINT,1,NULL,0);
    VARXOR(NULL,0,BOOL,0,NULL,0);
    VARXOR(NULL,0,BOOL,1,NULL,0);
    VARXOR(NULL,0,R4,0,NULL,0);
    VARXOR(NULL,0,R4,1,NULL,0);
    VARXOR(NULL,0,R8,0,NULL,0);
    VARXOR(NULL,0,R8,1,NULL,0);
    rbstr = SysAllocString(szFalse);
    VARXOR(NULL,0,BSTR,rbstr,NULL,0);
    SysFreeString(rbstr);
    rbstr = SysAllocString(szTrue);
    VARXOR(NULL,0,BSTR,rbstr,NULL,0);
    SysFreeString(rbstr);
    VARXORCY(NULL,0,10000,NULL,0);
    VARXORCY(NULL,0,0,NULL,0);

    VARXOR(BOOL,VARIANT_TRUE,BOOL,VARIANT_TRUE,BOOL,VARIANT_FALSE);
    VARXOR(BOOL,VARIANT_TRUE,BOOL,VARIANT_FALSE,BOOL,VARIANT_TRUE);
    VARXOR(BOOL,VARIANT_FALSE,BOOL,VARIANT_TRUE,BOOL,VARIANT_TRUE);
    VARXOR(BOOL,VARIANT_FALSE,BOOL,VARIANT_FALSE,BOOL,VARIANT_FALSE);
    /* Assume x,y & y,x are the same from now on to reduce the number of tests */
    VARXOR(BOOL,VARIANT_TRUE,I1,-1,I4,0);
    VARXOR(BOOL,VARIANT_TRUE,I1,0,I4,-1);
    VARXOR(BOOL,VARIANT_FALSE,I1,0,I4,0);
    VARXOR(BOOL,VARIANT_TRUE,UI1,255,I2,-256);
    VARXOR(BOOL,VARIANT_TRUE,UI1,0,I2,-1);
    VARXOR(BOOL,VARIANT_FALSE,UI1,0,I2,0);
    VARXOR(BOOL,VARIANT_TRUE,I2,-1,I2,0);
    VARXOR(BOOL,VARIANT_TRUE,I2,0,I2,-1);
    VARXOR(BOOL,VARIANT_FALSE,I2,0,I2,0);
    VARXOR(BOOL,VARIANT_TRUE,UI2,65535,I4,-65536);
    VARXOR(BOOL,VARIANT_TRUE,UI2,0,I4,-1);
    VARXOR(BOOL,VARIANT_FALSE,UI2,0,I4,0);
    VARXOR(BOOL,VARIANT_TRUE,I4,-1,I4,0);
    VARXOR(BOOL,VARIANT_TRUE,I4,0,I4,-1);
    VARXOR(BOOL,VARIANT_FALSE,I4,0,I4,0);
    VARXOR(BOOL,VARIANT_TRUE,UI4,0xffffffff,I4,0);
    VARXOR(BOOL,VARIANT_TRUE,UI4,0,I4,-1);
    VARXOR(BOOL,VARIANT_FALSE,UI4,0,I4,0);
    VARXOR(BOOL,VARIANT_TRUE,R4,-1,I4,0);
    VARXOR(BOOL,VARIANT_TRUE,R4,0,I4,-1);
    VARXOR(BOOL,VARIANT_FALSE,R4,0,I4,0);
    VARXOR(BOOL,VARIANT_TRUE,R8,-1,I4,0);
    VARXOR(BOOL,VARIANT_TRUE,R8,0,I4,-1);
    VARXOR(BOOL,VARIANT_FALSE,R8,0,I4,0);
    VARXOR(BOOL,VARIANT_TRUE,DATE,-1,I4,0);
    VARXOR(BOOL,VARIANT_TRUE,DATE,0,I4,-1);
    VARXOR(BOOL,VARIANT_FALSE,DATE,0,I4,0);
    if (has_i8)
    {
        VARXOR(BOOL,VARIANT_TRUE,I8,-1,I8,0);
        VARXOR(BOOL,VARIANT_TRUE,I8,0,I8,-1);
        VARXOR(BOOL,VARIANT_FALSE,I8,0,I8,0);
        /* This returns DISP_E_OVERFLOW which indicates that a conversion
         * to I4 is performed.
         */
        /* VARXOR(BOOL,VARIANT_TRUE,UI8,-1,I4,-1); */
        VARXOR(BOOL,VARIANT_TRUE,UI8,0,I4,-1);
        VARXOR(BOOL,VARIANT_FALSE,UI8,0,I4,0);
    }
    VARXOR(BOOL,VARIANT_TRUE,INT,-1,I4,0);
    VARXOR(BOOL,VARIANT_TRUE,INT,0,I4,-1);
    VARXOR(BOOL,VARIANT_FALSE,INT,0,I4,0);
    VARXOR(BOOL,VARIANT_TRUE,UINT,0xffffffff,I4,0);
    VARXOR(BOOL,VARIANT_TRUE,UINT,0,I4,-1);
    VARXOR(BOOL,VARIANT_FALSE,UINT,0,I4,0);
    rbstr = SysAllocString(szFalse);
    VARXOR(BOOL,VARIANT_FALSE,BSTR,rbstr,BOOL,VARIANT_FALSE);
    VARXOR(BOOL,VARIANT_TRUE,BSTR,rbstr,BOOL,VARIANT_TRUE);
    SysFreeString(rbstr);
    rbstr = SysAllocString(szTrue);
    VARXOR(BOOL,VARIANT_FALSE,BSTR,rbstr,BOOL,VARIANT_TRUE);
    VARXOR(BOOL,VARIANT_TRUE,BSTR,rbstr,BOOL,VARIANT_FALSE);
    SysFreeString(rbstr);
    VARXORCY(BOOL,VARIANT_TRUE,10000,I4,-2);
    VARXORCY(BOOL,VARIANT_TRUE,0,I4,-1);
    VARXORCY(BOOL,VARIANT_FALSE,0,I4,0);

    VARXOR(I1,-1,I1,-1,I4,0);
    VARXOR(I1,-1,I1,0,I4,-1);
    VARXOR(I1,0,I1,0,I4,0);
    VARXOR(I1,-1,UI1,255,I4,-256);
    VARXOR(I1,-1,UI1,0,I4,-1);
    VARXOR(I1,0,UI1,0,I4,0);
    VARXOR(I1,-1,I2,-1,I4,0);
    VARXOR(I1,-1,I2,0,I4,-1);
    VARXOR(I1,0,I2,0,I4,0);
    VARXOR(I1,-1,UI2,65535,I4,-65536);
    VARXOR(I1,-1,UI2,0,I4,-1);
    VARXOR(I1,0,UI2,0,I4,0);
    VARXOR(I1,-1,I4,-1,I4,0);
    VARXOR(I1,-1,I4,0,I4,-1);
    VARXOR(I1,0,I4,0,I4,0);
    VARXOR(I1,-1,UI4,0xffffffff,I4,0);
    VARXOR(I1,-1,UI4,0,I4,-1);
    VARXOR(I1,0,UI4,0,I4,0);
    VARXOR(I1,-1,R4,-1,I4,0);
    VARXOR(I1,-1,R4,0,I4,-1);
    VARXOR(I1,0,R4,0,I4,0);
    VARXOR(I1,-1,R8,-1,I4,0);
    VARXOR(I1,-1,R8,0,I4,-1);
    VARXOR(I1,0,R8,0,I4,0);
    VARXOR(I1,-1,DATE,-1,I4,0);
    VARXOR(I1,-1,DATE,0,I4,-1);
    VARXOR(I1,0,DATE,0,I4,0);
    if (has_i8)
    {
        VARXOR(I1,-1,I8,-1,I8,0);
        VARXOR(I1,-1,I8,0,I8,-1);
        VARXOR(I1,0,I8,0,I8,0);
        VARXOR(I1,-1,UI8,0,I4,-1);
        VARXOR(I1,0,UI8,0,I4,0);
    }
    VARXOR(I1,-1,INT,-1,I4,0);
    VARXOR(I1,-1,INT,0,I4,-1);
    VARXOR(I1,0,INT,0,I4,0);
    VARXOR(I1,-1,UINT,0xffffffff,I4,0);
    VARXOR(I1,-1,UINT,0,I4,-1);
    VARXOR(I1,0,UINT,0,I4,0);
    rbstr = SysAllocString(szFalse);
    VARXOR(I1,0,BSTR,rbstr,I4,0);
    VARXOR(I1,-1,BSTR,rbstr,I4,-1);
    SysFreeString(rbstr);
    rbstr = SysAllocString(szTrue);
    VARXOR(I1,0,BSTR,rbstr,I4,-1);
    VARXOR(I1,-1,BSTR,rbstr,I4,0);
    SysFreeString(rbstr);
    VARXORCY(I1,-1,10000,I4,-2);
    VARXORCY(I1,-1,0,I4,-1);
    VARXORCY(I1,0,0,I4,0);

    VARXOR(UI1,255,UI1,255,UI1,0);
    VARXOR(UI1,255,UI1,0,UI1,255);
    VARXOR(UI1,0,UI1,0,UI1,0);
    VARXOR(UI1,255,I2,-1,I2,-256);
    VARXOR(UI1,255,I2,0,I2,255);
    VARXOR(UI1,0,I2,0,I2,0);
    VARXOR(UI1,255,UI2,65535,I4,65280);
    VARXOR(UI1,255,UI2,0,I4,255);
    VARXOR(UI1,0,UI2,0,I4,0);
    VARXOR(UI1,255,I4,-1,I4,-256);
    VARXOR(UI1,255,I4,0,I4,255);
    VARXOR(UI1,0,I4,0,I4,0);
    VARXOR(UI1,255,UI4,0xffffffff,I4,-256);
    VARXOR(UI1,255,UI4,0,I4,255);
    VARXOR(UI1,0,UI4,0,I4,0);
    VARXOR(UI1,255,R4,-1,I4,-256);
    VARXOR(UI1,255,R4,0,I4,255);
    VARXOR(UI1,0,R4,0,I4,0);
    VARXOR(UI1,255,R8,-1,I4,-256);
    VARXOR(UI1,255,R8,0,I4,255);
    VARXOR(UI1,0,R8,0,I4,0);
    VARXOR(UI1,255,DATE,-1,I4,-256);
    VARXOR(UI1,255,DATE,0,I4,255);
    VARXOR(UI1,0,DATE,0,I4,0);
    if (has_i8)
    {
        VARXOR(UI1,255,I8,-1,I8,-256);
        VARXOR(UI1,255,I8,0,I8,255);
        VARXOR(UI1,0,I8,0,I8,0);
        VARXOR(UI1,255,UI8,0,I4,255);
        VARXOR(UI1,0,UI8,0,I4,0);
    }
    VARXOR(UI1,255,INT,-1,I4,-256);
    VARXOR(UI1,255,INT,0,I4,255);
    VARXOR(UI1,0,INT,0,I4,0);
    VARXOR(UI1,255,UINT,0xffffffff,I4,-256);
    VARXOR(UI1,255,UINT,0,I4,255);
    VARXOR(UI1,0,UINT,0,I4,0);
    rbstr = SysAllocString(szFalse);
    VARXOR(UI1,0,BSTR,rbstr,I2,0);
    VARXOR(UI1,255,BSTR,rbstr,I2,255);
    SysFreeString(rbstr);
    rbstr = SysAllocString(szTrue);
    VARXOR(UI1,0,BSTR,rbstr,I2,-1);
    VARXOR(UI1,255,BSTR,rbstr,I2,-256);
    SysFreeString(rbstr);
    VARXORCY(UI1,255,10000,I4,254);
    VARXORCY(UI1,255,0,I4,255);
    VARXORCY(UI1,0,0,I4,0);

    VARXOR(I2,-1,I2,-1,I2,0);
    VARXOR(I2,-1,I2,0,I2,-1);
    VARXOR(I2,0,I2,0,I2,0);
    VARXOR(I2,-1,UI2,65535,I4,-65536);
    VARXOR(I2,-1,UI2,0,I4,-1);
    VARXOR(I2,0,UI2,0,I4,0);
    VARXOR(I2,-1,I4,-1,I4,0);
    VARXOR(I2,-1,I4,0,I4,-1);
    VARXOR(I2,0,I4,0,I4,0);
    VARXOR(I2,-1,UI4,0xffffffff,I4,0);
    VARXOR(I2,-1,UI4,0,I4,-1);
    VARXOR(I2,0,UI4,0,I4,0);
    VARXOR(I2,-1,R4,-1,I4,0);
    VARXOR(I2,-1,R4,0,I4,-1);
    VARXOR(I2,0,R4,0,I4,0);
    VARXOR(I2,-1,R8,-1,I4,0);
    VARXOR(I2,-1,R8,0,I4,-1);
    VARXOR(I2,0,R8,0,I4,0);
    VARXOR(I2,-1,DATE,-1,I4,0);
    VARXOR(I2,-1,DATE,0,I4,-1);
    VARXOR(I2,0,DATE,0,I4,0);
    if (has_i8)
    {
        VARXOR(I2,-1,I8,-1,I8,0);
        VARXOR(I2,-1,I8,0,I8,-1);
        VARXOR(I2,0,I8,0,I8,0);
        VARXOR(I2,-1,UI8,0,I4,-1);
        VARXOR(I2,0,UI8,0,I4,0);
    }
    VARXOR(I2,-1,INT,-1,I4,0);
    VARXOR(I2,-1,INT,0,I4,-1);
    VARXOR(I2,0,INT,0,I4,0);
    VARXOR(I2,-1,UINT,0xffffffff,I4,0);
    VARXOR(I2,-1,UINT,0,I4,-1);
    VARXOR(I2,0,UINT,0,I4,0);
    rbstr = SysAllocString(szFalse);
    VARXOR(I2,0,BSTR,rbstr,I2,0);
    VARXOR(I2,-1,BSTR,rbstr,I2,-1);
    SysFreeString(rbstr);
    rbstr = SysAllocString(szTrue);
    VARXOR(I2,0,BSTR,rbstr,I2,-1);
    VARXOR(I2,-1,BSTR,rbstr,I2,0);
    SysFreeString(rbstr);
    VARXORCY(I2,-1,10000,I4,-2);
    VARXORCY(I2,-1,0,I4,-1);
    VARXORCY(I2,0,0,I4,0);

    VARXOR(UI2,65535,UI2,65535,I4,0);
    VARXOR(UI2,65535,UI2,0,I4,65535);
    VARXOR(UI2,0,UI2,0,I4,0);
    VARXOR(UI2,65535,I4,-1,I4,-65536);
    VARXOR(UI2,65535,I4,0,I4,65535);
    VARXOR(UI2,0,I4,0,I4,0);
    VARXOR(UI2,65535,UI4,0xffffffff,I4,-65536);
    VARXOR(UI2,65535,UI4,0,I4,65535);
    VARXOR(UI2,0,UI4,0,I4,0);
    VARXOR(UI2,65535,R4,-1,I4,-65536);
    VARXOR(UI2,65535,R4,0,I4,65535);
    VARXOR(UI2,0,R4,0,I4,0);
    VARXOR(UI2,65535,R8,-1,I4,-65536);
    VARXOR(UI2,65535,R8,0,I4,65535);
    VARXOR(UI2,0,R8,0,I4,0);
    VARXOR(UI2,65535,DATE,-1,I4,-65536);
    VARXOR(UI2,65535,DATE,0,I4,65535);
    VARXOR(UI2,0,DATE,0,I4,0);
    if (has_i8)
    {
        VARXOR(UI2,65535,I8,-1,I8,-65536);
        VARXOR(UI2,65535,I8,0,I8,65535);
        VARXOR(UI2,0,I8,0,I8,0);
        VARXOR(UI2,65535,UI8,0,I4,65535);
        VARXOR(UI2,0,UI8,0,I4,0);
    }
    VARXOR(UI2,65535,INT,-1,I4,-65536);
    VARXOR(UI2,65535,INT,0,I4,65535);
    VARXOR(UI2,0,INT,0,I4,0);
    VARXOR(UI2,65535,UINT,0xffffffff,I4,-65536);
    VARXOR(UI2,65535,UINT,0,I4,65535);
    VARXOR(UI2,0,UINT,0,I4,0);
    rbstr = SysAllocString(szFalse);
    VARXOR(UI2,0,BSTR,rbstr,I4,0);
    VARXOR(UI2,65535,BSTR,rbstr,I4,65535);
    SysFreeString(rbstr);
    rbstr = SysAllocString(szTrue);
    VARXOR(UI2,0,BSTR,rbstr,I4,-1);
    VARXOR(UI2,65535,BSTR,rbstr,I4,-65536);
    SysFreeString(rbstr);
    VARXORCY(UI2,65535,10000,I4,65534);
    VARXORCY(UI2,65535,0,I4,65535);
    VARXORCY(UI2,0,0,I4,0);

    VARXOR(I4,-1,I4,-1,I4,0);
    VARXOR(I4,-1,I4,0,I4,-1);
    VARXOR(I4,0,I4,0,I4,0);
    VARXOR(I4,-1,UI4,0xffffffff,I4,0);
    VARXOR(I4,-1,UI4,0,I4,-1);
    VARXOR(I4,0,UI4,0,I4,0);
    VARXOR(I4,-1,R4,-1,I4,0);
    VARXOR(I4,-1,R4,0,I4,-1);
    VARXOR(I4,0,R4,0,I4,0);
    VARXOR(I4,-1,R8,-1,I4,0);
    VARXOR(I4,-1,R8,0,I4,-1);
    VARXOR(I4,0,R8,0,I4,0);
    VARXOR(I4,-1,DATE,-1,I4,0);
    VARXOR(I4,-1,DATE,0,I4,-1);
    VARXOR(I4,0,DATE,0,I4,0);
    if (has_i8)
    {
        VARXOR(I4,-1,I8,-1,I8,0);
        VARXOR(I4,-1,I8,0,I8,-1);
        VARXOR(I4,0,I8,0,I8,0);
        VARXOR(I4,-1,UI8,0,I4,-1);
        VARXOR(I4,0,UI8,0,I4,0);
    }
    VARXOR(I4,-1,INT,-1,I4,0);
    VARXOR(I4,-1,INT,0,I4,-1);
    VARXOR(I4,0,INT,0,I4,0);
    VARXOR(I4,-1,UINT,0xffffffff,I4,0);
    VARXOR(I4,-1,UINT,0,I4,-1);
    VARXOR(I4,0,UINT,0,I4,0);
    rbstr = SysAllocString(szFalse);
    VARXOR(I4,0,BSTR,rbstr,I4,0);
    VARXOR(I4,-1,BSTR,rbstr,I4,-1);
    SysFreeString(rbstr);
    rbstr = SysAllocString(szTrue);
    VARXOR(I4,0,BSTR,rbstr,I4,-1);
    VARXOR(I4,-1,BSTR,rbstr,I4,0);
    SysFreeString(rbstr);
    VARXORCY(I4,-1,10000,I4,-2);
    VARXORCY(I4,-1,0,I4,-1);
    VARXORCY(I4,0,0,I4,0);

    VARXOR(UI4,0xffffffff,UI4,0xffffffff,I4,0);
    VARXOR(UI4,0xffffffff,UI4,0,I4,-1);
    VARXOR(UI4,0,UI4,0,I4,0);
    VARXOR(UI4,0xffffffff,R4,-1,I4,0);
    VARXOR(UI4,0xffffffff,R4,0,I4,-1);
    VARXOR(UI4,0,R4,0,I4,0);
    VARXOR(UI4,0xffffffff,R8,-1,I4,0);
    VARXOR(UI4,0xffffffff,R8,0,I4,-1);
    VARXOR(UI4,0,R8,0,I4,0);
    VARXOR(UI4,0xffffffff,DATE,-1,I4,0);
    VARXOR(UI4,0xffffffff,DATE,0,I4,-1);
    VARXOR(UI4,0,DATE,0,I4,0);
    if (has_i8)
    {
        VARXOR(UI4,0xffffffff,I8,0,I8,0xffffffff);
        VARXOR(UI4,VARIANT_FALSE,I8,VARIANT_FALSE,I8,0);
        VARXOR(UI4,0,I8,0,I8,0);
        VARXOR(UI4,0xffffffff,UI8,0,I4,-1);
        VARXOR(UI4,0,UI8,0,I4,0);
    }
    VARXOR(UI4,0xffffffff,INT,-1,I4,0);
    VARXOR(UI4,0xffffffff,INT,0,I4,-1);
    VARXOR(UI4,0,INT,0,I4,0);
    VARXOR(UI4,0xffffffff,UINT,0xffffffff,I4,0);
    VARXOR(UI4,0xffffffff,UINT,0,I4,-1);
    VARXOR(UI4,0,UINT,0,I4,0);
    rbstr = SysAllocString(szFalse);
    VARXOR(UI4,0,BSTR,rbstr,I4,0);
    VARXOR(UI4,0xffffffff,BSTR,rbstr,I4,-1);
    SysFreeString(rbstr);
    rbstr = SysAllocString(szTrue);
    VARXOR(UI4,0,BSTR,rbstr,I4,-1);
    VARXOR(UI4,0xffffffff,BSTR,rbstr,I4,0);
    SysFreeString(rbstr);
    VARXORCY(UI4,0xffffffff,10000,I4,-2);
    VARXORCY(UI4,0xffffffff,0,I4,-1);
    VARXORCY(UI4,0,0,I4,0);

    VARXOR(R4,-1,R4,-1,I4,0);
    VARXOR(R4,-1,R4,0,I4,-1);
    VARXOR(R4,0,R4,0,I4,0);
    VARXOR(R4,-1,R8,-1,I4,0);
    VARXOR(R4,-1,R8,0,I4,-1);
    VARXOR(R4,0,R8,0,I4,0);
    VARXOR(R4,-1,DATE,-1,I4,0);
    VARXOR(R4,-1,DATE,0,I4,-1);
    VARXOR(R4,0,DATE,0,I4,0);
    if (has_i8)
    {
        VARXOR(R4,-1,I8,-1,I8,0);
        VARXOR(R4,-1,I8,0,I8,-1);
        VARXOR(R4,0,I8,0,I8,0);
        VARXOR(R4,-1,UI8,0,I4,-1);
        VARXOR(R4,0,UI8,0,I4,0);
    }
    VARXOR(R4,-1,INT,-1,I4,0);
    VARXOR(R4,-1,INT,0,I4,-1);
    VARXOR(R4,0,INT,0,I4,0);
    VARXOR(R4,-1,UINT,0xffffffff,I4,0);
    VARXOR(R4,-1,UINT,0,I4,-1);
    VARXOR(R4,0,UINT,0,I4,0);
    rbstr = SysAllocString(szFalse);
    VARXOR(R4,0,BSTR,rbstr,I4,0);
    VARXOR(R4,-1,BSTR,rbstr,I4,-1);
    SysFreeString(rbstr);
    rbstr = SysAllocString(szTrue);
    VARXOR(R4,0,BSTR,rbstr,I4,-1);
    VARXOR(R4,-1,BSTR,rbstr,I4,0);
    SysFreeString(rbstr);
    VARXORCY(R4,-1,10000,I4,-2);
    VARXORCY(R4,-1,0,I4,-1);
    VARXORCY(R4,0,0,I4,0);

    VARXOR(R8,-1,R8,-1,I4,0);
    VARXOR(R8,-1,R8,0,I4,-1);
    VARXOR(R8,0,R8,0,I4,0);
    VARXOR(R8,-1,DATE,-1,I4,0);
    VARXOR(R8,-1,DATE,0,I4,-1);
    VARXOR(R8,0,DATE,0,I4,0);
    if (has_i8)
    {
        VARXOR(R8,-1,I8,-1,I8,0);
        VARXOR(R8,-1,I8,0,I8,-1);
        VARXOR(R8,0,I8,0,I8,0);
        VARXOR(R8,-1,UI8,0,I4,-1);
        VARXOR(R8,0,UI8,0,I4,0);
    }
    VARXOR(R8,-1,INT,-1,I4,0);
    VARXOR(R8,-1,INT,0,I4,-1);
    VARXOR(R8,0,INT,0,I4,0);
    VARXOR(R8,-1,UINT,0xffffffff,I4,0);
    VARXOR(R8,-1,UINT,0,I4,-1);
    VARXOR(R8,0,UINT,0,I4,0);
    rbstr = SysAllocString(szFalse);
    VARXOR(R8,0,BSTR,rbstr,I4,0);
    VARXOR(R8,-1,BSTR,rbstr,I4,-1);
    SysFreeString(rbstr);
    rbstr = SysAllocString(szTrue);
    VARXOR(R8,0,BSTR,rbstr,I4,-1);
    VARXOR(R8,-1,BSTR,rbstr,I4,0);
    SysFreeString(rbstr);
    VARXORCY(R8,-1,10000,I4,-2);
    VARXORCY(R8,-1,0,I4,-1);
    VARXORCY(R8,0,0,I4,0);

    VARXOR(DATE,-1,DATE,-1,I4,0);
    VARXOR(DATE,-1,DATE,0,I4,-1);
    VARXOR(DATE,0,DATE,0,I4,0);
    if (has_i8)
    {
        VARXOR(DATE,-1,I8,-1,I8,0);
        VARXOR(DATE,-1,I8,0,I8,-1);
        VARXOR(DATE,0,I8,0,I8,0);
        VARXOR(DATE,-1,UI8,0,I4,-1);
        VARXOR(DATE,0,UI8,0,I4,0);
    }
    VARXOR(DATE,-1,INT,-1,I4,0);
    VARXOR(DATE,-1,INT,0,I4,-1);
    VARXOR(DATE,0,INT,0,I4,0);
    VARXOR(DATE,-1,UINT,0xffffffff,I4,0);
    VARXOR(DATE,-1,UINT,0,I4,-1);
    VARXOR(DATE,0,UINT,0,I4,0);
    rbstr = SysAllocString(szFalse);
    VARXOR(DATE,0,BSTR,rbstr,I4,0);
    VARXOR(DATE,-1,BSTR,rbstr,I4,-1);
    SysFreeString(rbstr);
    rbstr = SysAllocString(szTrue);
    VARXOR(DATE,0,BSTR,rbstr,I4,-1);
    VARXOR(DATE,-1,BSTR,rbstr,I4,0);
    SysFreeString(rbstr);
    VARXORCY(DATE,-1,10000,I4,-2);
    VARXORCY(DATE,-1,0,I4,-1);
    VARXORCY(DATE,0,0,I4,0);

    if (has_i8)
    {
        VARXOR(I8,-1,I8,-1,I8,0);
        VARXOR(I8,-1,I8,0,I8,-1);
        VARXOR(I8,0,I8,0,I8,0);
        VARXOR(I8,-1,UI8,0,I8,-1);
        VARXOR(I8,0,UI8,0,I8,0);
        VARXOR(I8,-1,UINT,0,I8,-1);
        VARXOR(I8,0,UINT,0,I8,0);
        rbstr = SysAllocString(szFalse);
        VARXOR(I8,0,BSTR,rbstr,I8,0);
        VARXOR(I8,-1,BSTR,rbstr,I8,-1);
        SysFreeString(rbstr);
        rbstr = SysAllocString(szTrue);
        VARXOR(I8,0,BSTR,rbstr,I8,-1);
        VARXOR(I8,-1,BSTR,rbstr,I8,0);
        SysFreeString(rbstr);
        VARXORCY(I8,-1,10000,I8,-2);
        VARXORCY(I8,-1,0,I8,-1);
        VARXORCY(I8,0,0,I8,0);

        VARXOR(UI8,0xffff,UI8,0xffff,I4,0);
        VARXOR(UI8,0xffff,UI8,0,I4,0xffff);
        VARXOR(UI8,0,UI8,0,I4,0);
        VARXOR(UI8,0xffff,INT,-1,I4,-65536);
        VARXOR(UI8,0xffff,INT,0,I4,0xffff);
        VARXOR(UI8,0,INT,0,I4,0);
        VARXOR(UI8,0xffff,UINT,0xffff,I4,0);
        VARXOR(UI8,0xffff,UINT,0,I4,0xffff);
        VARXOR(UI8,0,UINT,0,I4,0);
        rbstr = SysAllocString(szFalse);
        VARXOR(UI8,0,BSTR,rbstr,I4,0);
        VARXOR(UI8,0xffff,BSTR,rbstr,I4,0xffff);
        SysFreeString(rbstr);
        rbstr = SysAllocString(szTrue);
        VARXOR(UI8,0,BSTR,rbstr,I4,-1);
        VARXOR(UI8,0xffff,BSTR,rbstr,I4,-65536);
        SysFreeString(rbstr);
        VARXORCY(UI8,0xffff,10000,I4,65534);
        VARXORCY(UI8,0xffff,0,I4,0xffff);
        VARXORCY(UI8,0,0,I4,0);
    }

    VARXOR(INT,-1,INT,-1,I4,0);
    VARXOR(INT,-1,INT,0,I4,-1);
    VARXOR(INT,0,INT,0,I4,0);
    VARXOR(INT,-1,UINT,0xffff,I4,-65536);
    VARXOR(INT,-1,UINT,0,I4,-1);
    VARXOR(INT,0,UINT,0,I4,0);
    rbstr = SysAllocString(szFalse);
    VARXOR(INT,0,BSTR,rbstr,I4,0);
    VARXOR(INT,-1,BSTR,rbstr,I4,-1);
    SysFreeString(rbstr);
    rbstr = SysAllocString(szTrue);
    VARXOR(INT,0,BSTR,rbstr,I4,-1);
    VARXOR(INT,-1,BSTR,rbstr,I4,0);
    SysFreeString(rbstr);
    VARXORCY(INT,-1,10000,I4,-2);
    VARXORCY(INT,-1,0,I4,-1);
    VARXORCY(INT,0,0,I4,0);

    VARXOR(UINT,0xffff,UINT,0xffff,I4,0);
    VARXOR(UINT,0xffff,UINT,0,I4,0xffff);
    VARXOR(UINT,0,UINT,0,I4,0);
    rbstr = SysAllocString(szFalse);
    VARXOR(UINT,0,BSTR,rbstr,I4,0);
    VARXOR(UINT,0xffff,BSTR,rbstr,I4,0xffff);
    SysFreeString(rbstr);
    rbstr = SysAllocString(szTrue);
    VARXOR(UINT,0,BSTR,rbstr,I4,-1);
    VARXOR(UINT,0xffff,BSTR,rbstr,I4,-65536);
    SysFreeString(rbstr);
    VARXORCY(UINT,0xffff,10000,I4,65534);
    VARXORCY(UINT,0xffff,0,I4,0xffff);
    VARXORCY(UINT,0,0,I4,0);

    lbstr = SysAllocString(szFalse);
    rbstr = SysAllocString(szFalse);
    VARXOR(BSTR,lbstr,BSTR,rbstr,BOOL,0);
    SysFreeString(rbstr);
    rbstr = SysAllocString(szTrue);
    VARXOR(BSTR,lbstr,BSTR,rbstr,BOOL,VARIANT_TRUE);
    SysFreeString(lbstr);
    lbstr = SysAllocString(szTrue);
    VARXOR(BSTR,lbstr,BSTR,rbstr,BOOL,VARIANT_FALSE);
    VARXORCY(BSTR,lbstr,10000,I4,-2);
    SysFreeString(lbstr);
    lbstr = SysAllocString(szFalse);
    VARXORCY(BSTR,lbstr,10000,I4,1);
    SysFreeString(lbstr);
    SysFreeString(rbstr);
}

static HRESULT (WINAPI *pVarOr)(LPVARIANT,LPVARIANT,LPVARIANT);

#define VAROR(vt1,val1,vt2,val2,rvt,rval)                \
        V_VT(&left) = VT_##vt1; V_##vt1(&left) = val1;   \
        V_VT(&right) = VT_##vt2; V_##vt2(&right) = val2; \
        V_VT(&exp) = VT_##rvt; V_##rvt(&exp) = rval;     \
        test_var_call2( __LINE__, pVarOr, &left, &right, &exp )

#define VARORCY(vt1,val1,val2,rvt,rval)                  \
        V_VT(&left) = VT_##vt1; V_##vt1(&left) = val1;   \
        V_VT(&right) = VT_CY; V_CY(&right).int64 = val2; \
        V_VT(&exp) = VT_##rvt; V_##rvt(&exp) = rval;     \
        test_var_call2( __LINE__, pVarOr, &left, &right, &exp )

static void test_VarOr(void)
{
    static const WCHAR szFalse[] = { '#','F','A','L','S','E','#','\0' };
    static const WCHAR szTrue[] = { '#','T','R','U','E','#','\0' };
    VARIANT left, right, exp, result;
    BSTR lbstr, rbstr;
    VARTYPE i;
    HRESULT hres;

    CHECKPTR(VarOr);

    /* Test all possible flag/vt combinations & the resulting vt type */
    for (i = 0; i < ARRAY_SIZE(ExtraFlags); i++)
    {
        VARTYPE leftvt, rightvt, resvt;

        for (leftvt = 0; leftvt <= VT_BSTR_BLOB; leftvt++)
        {
        
            SKIPTESTS(leftvt);
        
            for (rightvt = 0; rightvt <= VT_BSTR_BLOB; rightvt++)
            {
                BOOL bFail = FALSE;

                SKIPTESTS(rightvt);
                
                if (leftvt == VT_BSTR || rightvt == VT_BSTR ||
                    leftvt == VT_DISPATCH || rightvt == VT_DISPATCH ||
                    leftvt == VT_UNKNOWN || rightvt == VT_UNKNOWN)
                    continue;

                memset(&left, 0, sizeof(left));
                memset(&right, 0, sizeof(right));
                V_VT(&left) = leftvt | ExtraFlags[i];
                V_VT(&right) = rightvt | ExtraFlags[i];
                V_VT(&result) = VT_EMPTY;
                resvt = VT_I4;

                if (ExtraFlags[i] & VT_ARRAY || ExtraFlags[i] & VT_BYREF ||
                    !IsValidVariantClearVT(leftvt, ExtraFlags[i]) ||
                    !IsValidVariantClearVT(rightvt, ExtraFlags[i]) ||
                    leftvt == VT_CLSID || rightvt == VT_CLSID ||
                    leftvt == VT_RECORD || rightvt == VT_RECORD ||
                    leftvt == VT_VARIANT || rightvt == VT_VARIANT ||
                    leftvt == VT_ERROR || rightvt == VT_ERROR)
                {
                    bFail = TRUE;
                }
                if (leftvt == VT_EMPTY || rightvt == VT_EMPTY)
                {
                    if (leftvt == rightvt ||
                        leftvt == VT_I2 || rightvt == VT_I2 ||
                        leftvt == VT_UI1 || rightvt == VT_UI1 ||
                        leftvt == VT_BOOL || rightvt == VT_BOOL)
                        resvt = VT_I2;
                    else if (leftvt == VT_NULL || rightvt == VT_NULL)
                        resvt = VT_NULL;
                    else if (leftvt == VT_I8 || rightvt == VT_I8)
                        resvt = VT_I8;
                }
                else if (leftvt == VT_NULL || rightvt == VT_NULL)
                {
                    resvt = VT_NULL;
                }
                else if (leftvt == VT_UI1 || rightvt == VT_UI1)
                {
                    if (leftvt == rightvt)
                        resvt = VT_UI1;
                    else if (leftvt == rightvt ||
                        leftvt == VT_I2 || rightvt == VT_I2 ||
                        leftvt == VT_BOOL || rightvt == VT_BOOL)
                    {
                        resvt = VT_I2;
                    }
                    else if (leftvt == VT_I8 || rightvt == VT_I8)
                        resvt = VT_I8;
                }
                else if (leftvt == VT_I2 || rightvt == VT_I2)
                {
                    if (leftvt == rightvt ||
                        leftvt == VT_BOOL || rightvt == VT_BOOL)
                        resvt = VT_I2;
                    else if (leftvt == VT_I8 || rightvt == VT_I8)
                        resvt = VT_I8;
                }
                else if (leftvt == VT_BOOL && rightvt == VT_BOOL)
                {
                    resvt = VT_BOOL;
                }
                else if (leftvt == VT_I8 || rightvt == VT_I8)
                {
                    if (leftvt == VT_INT || rightvt == VT_INT)
                        bFail = TRUE;
                    else
                        resvt = VT_I8;
                }
                hres = pVarOr(&left, &right, &result);
                if (bFail)
                    ok(hres == DISP_E_TYPEMISMATCH || hres == DISP_E_BADVARTYPE,
                       "VarOr: %d|0x%X, %d|0x%X: Expected failure, got 0x%lX vt %d\n",
                       leftvt, ExtraFlags[i], rightvt, ExtraFlags[i], hres,
                       V_VT(&result));
                else
                    ok(hres == S_OK && V_VT(&result) == resvt,
                       "VarOr: %d|0x%X, %d|0x%X: expected S_OK, vt %d, got 0x%lX vt %d\n",
                       leftvt, ExtraFlags[i], rightvt, ExtraFlags[i], resvt, hres,
                       V_VT(&result));
            }
        }
    }

    /* Test returned values. Since we know the returned type is correct
     * and that we handle all combinations of invalid types, just check
     * that good type combinations produce the desired value.
     * FIXME: Test VT_DECIMAL/VT_DISPATCH
     */
    VAROR(EMPTY,0,EMPTY,0,I2,0);
    VAROR(EMPTY,1,EMPTY,0,I2,0);
    VAROR(EMPTY,0,NULL,0,NULL,0);
    VAROR(EMPTY,0,I1,0,I4,0);
    VAROR(EMPTY,0,I1,1,I4,1);
    VAROR(EMPTY,0,UI1,0,I2,0);
    VAROR(EMPTY,0,UI1,1,I2,1);
    VAROR(EMPTY,0,I2,0,I2,0);
    VAROR(EMPTY,0,I2,1,I2,1);
    VAROR(EMPTY,0,UI2,0,I4,0);
    VAROR(EMPTY,0,UI2,1,I4,1);
    VAROR(EMPTY,0,I4,0,I4,0);
    VAROR(EMPTY,0,I4,1,I4,1);
    VAROR(EMPTY,0,UI4,0,I4,0);
    VAROR(EMPTY,0,UI4,1,I4,1);
    if (has_i8)
    {
        VAROR(EMPTY,0,I8,0,I8,0);
        VAROR(EMPTY,0,I8,1,I8,1);
        VAROR(EMPTY,0,UI8,0,I4,0);
        VAROR(EMPTY,0,UI8,1,I4,1);
    }
    VAROR(EMPTY,0,INT,0,I4,0);
    VAROR(EMPTY,0,INT,1,I4,1);
    VAROR(EMPTY,0,UINT,0,I4,0);
    VAROR(EMPTY,0,UINT,1,I4,1);
    VAROR(EMPTY,0,BOOL,0,I2,0);
    VAROR(EMPTY,0,BOOL,1,I2,1);
    VAROR(EMPTY,0,R4,0,I4,0);
    VAROR(EMPTY,0,R4,1,I4,1);
    VAROR(EMPTY,0,R8,0,I4,0);
    VAROR(EMPTY,0,R8,1,I4,1);
    rbstr = SysAllocString(szFalse);
    VAROR(EMPTY,0,BSTR,rbstr,I2,0);
    SysFreeString(rbstr);
    rbstr = SysAllocString(szTrue);
    VAROR(EMPTY,0,BSTR,rbstr,I2,-1);
    SysFreeString(rbstr);
    VARORCY(EMPTY,0,10000,I4,1);

    /* NULL OR 0 = NULL. NULL OR n = n */
    VAROR(NULL,0,NULL,0,NULL,0);
    VAROR(NULL,1,NULL,0,NULL,0);
    VAROR(NULL,0,I1,0,NULL,0);
    VAROR(NULL,0,I1,1,I4,1);
    VAROR(NULL,0,UI1,0,NULL,0);
    VAROR(NULL,0,UI1,1,UI1,1);
    VAROR(NULL,0,I2,0,NULL,0);
    VAROR(NULL,0,I2,1,I2,1);
    VAROR(NULL,0,UI2,0,NULL,0);
    VAROR(NULL,0,UI2,1,I4,1);
    VAROR(NULL,0,I4,0,NULL,0);
    VAROR(NULL,0,I4,1,I4,1);
    VAROR(NULL,0,UI4,0,NULL,0);
    VAROR(NULL,0,UI4,1,I4,1);
    if (has_i8)
    {
        VAROR(NULL,0,I8,0,NULL,0);
        VAROR(NULL,0,I8,1,I8,1);
        VAROR(NULL,0,UI8,0,NULL,0);
        VAROR(NULL,0,UI8,1,I4,1);
    }
    VAROR(NULL,0,INT,0,NULL,0);
    VAROR(NULL,0,INT,1,I4,1);
    VAROR(NULL,0,UINT,0,NULL,0);
    VAROR(NULL,0,UINT,1,I4,1);
    VAROR(NULL,0,BOOL,0,NULL,0);
    VAROR(NULL,0,BOOL,1,BOOL,1);
    VAROR(NULL,0,R4,0,NULL,0);
    VAROR(NULL,0,R4,1,I4,1);
    VAROR(NULL,0,R8,0,NULL,0);
    VAROR(NULL,0,R8,1,I4,1);
    rbstr = SysAllocString(szFalse);
    VAROR(NULL,0,BSTR,rbstr,NULL,0);
    SysFreeString(rbstr);
    rbstr = SysAllocString(szTrue);
    VAROR(NULL,0,BSTR,rbstr,BOOL,VARIANT_TRUE);
    SysFreeString(rbstr);
    VARORCY(NULL,0,10000,I4,1);
    VARORCY(NULL,0,0,NULL,0);

    VAROR(BOOL,VARIANT_TRUE,BOOL,VARIANT_TRUE,BOOL,VARIANT_TRUE);
    VAROR(BOOL,VARIANT_TRUE,BOOL,VARIANT_FALSE,BOOL,VARIANT_TRUE);
    VAROR(BOOL,VARIANT_FALSE,BOOL,VARIANT_TRUE,BOOL,VARIANT_TRUE);
    VAROR(BOOL,VARIANT_FALSE,BOOL,VARIANT_FALSE,BOOL,VARIANT_FALSE);
    /* Assume x,y & y,x are the same from now on to reduce the number of tests */
    VAROR(BOOL,VARIANT_TRUE,I1,-1,I4,-1);
    VAROR(BOOL,VARIANT_TRUE,I1,0,I4,-1);
    VAROR(BOOL,VARIANT_FALSE,I1,0,I4,0);
    VAROR(BOOL,VARIANT_TRUE,UI1,255,I2,-1);
    VAROR(BOOL,VARIANT_TRUE,UI1,0,I2,-1);
    VAROR(BOOL,VARIANT_FALSE,UI1,0,I2,0);
    VAROR(BOOL,VARIANT_TRUE,I2,-1,I2,-1);
    VAROR(BOOL,VARIANT_TRUE,I2,0,I2,-1);
    VAROR(BOOL,VARIANT_FALSE,I2,0,I2,0);
    VAROR(BOOL,VARIANT_TRUE,UI2,65535,I4,-1);
    VAROR(BOOL,VARIANT_TRUE,UI2,0,I4,-1);
    VAROR(BOOL,VARIANT_FALSE,UI2,0,I4,0);
    VAROR(BOOL,VARIANT_TRUE,I4,-1,I4,-1);
    VAROR(BOOL,VARIANT_TRUE,I4,0,I4,-1);
    VAROR(BOOL,VARIANT_FALSE,I4,0,I4,0);
    VAROR(BOOL,VARIANT_TRUE,UI4,0xffffffff,I4,-1);
    VAROR(BOOL,VARIANT_TRUE,UI4,0,I4,-1);
    VAROR(BOOL,VARIANT_FALSE,UI4,0,I4,0);
    VAROR(BOOL,VARIANT_TRUE,R4,-1,I4,-1);
    VAROR(BOOL,VARIANT_TRUE,R4,0,I4,-1);
    VAROR(BOOL,VARIANT_FALSE,R4,0,I4,0);
    VAROR(BOOL,VARIANT_TRUE,R8,-1,I4,-1);
    VAROR(BOOL,VARIANT_TRUE,R8,0,I4,-1);
    VAROR(BOOL,VARIANT_FALSE,R8,0,I4,0);
    VAROR(BOOL,VARIANT_TRUE,DATE,-1,I4,-1);
    VAROR(BOOL,VARIANT_TRUE,DATE,0,I4,-1);
    VAROR(BOOL,VARIANT_FALSE,DATE,0,I4,0);
    if (has_i8)
    {
        VAROR(BOOL,VARIANT_TRUE,I8,-1,I8,-1);
        VAROR(BOOL,VARIANT_TRUE,I8,0,I8,-1);
        VAROR(BOOL,VARIANT_FALSE,I8,0,I8,0);
        /* This returns DISP_E_OVERFLOW which indicates that a conversion
         * to I4 is performed.
         */
        /* VAROR(BOOL,VARIANT_TRUE,UI8,-1,I4,-1); */
        VAROR(BOOL,VARIANT_TRUE,UI8,0,I4,-1);
        VAROR(BOOL,VARIANT_FALSE,UI8,0,I4,0);
    }
    VAROR(BOOL,VARIANT_TRUE,INT,-1,I4,-1);
    VAROR(BOOL,VARIANT_TRUE,INT,0,I4,-1);
    VAROR(BOOL,VARIANT_FALSE,INT,0,I4,0);
    VAROR(BOOL,VARIANT_TRUE,UINT,0xffffffff,I4,-1);
    VAROR(BOOL,VARIANT_TRUE,UINT,0,I4,-1);
    VAROR(BOOL,VARIANT_FALSE,UINT,0,I4,0);
    rbstr = SysAllocString(szFalse);
    VAROR(BOOL,VARIANT_FALSE,BSTR,rbstr,BOOL,VARIANT_FALSE);
    VAROR(BOOL,VARIANT_TRUE,BSTR,rbstr,BOOL,VARIANT_TRUE);
    SysFreeString(rbstr);
    rbstr = SysAllocString(szTrue);
    VAROR(BOOL,VARIANT_FALSE,BSTR,rbstr,BOOL,VARIANT_TRUE);
    VAROR(BOOL,VARIANT_TRUE,BSTR,rbstr,BOOL,VARIANT_TRUE);
    SysFreeString(rbstr);
    VARORCY(BOOL,VARIANT_TRUE,10000,I4,-1);
    VARORCY(BOOL,VARIANT_TRUE,0,I4,-1);
    VARORCY(BOOL,VARIANT_FALSE,0,I4,0);

    VAROR(I1,-1,I1,-1,I4,-1);
    VAROR(I1,-1,I1,0,I4,-1);
    VAROR(I1,0,I1,0,I4,0);
    VAROR(I1,-1,UI1,255,I4,-1);
    VAROR(I1,-1,UI1,0,I4,-1);
    VAROR(I1,0,UI1,0,I4,0);
    VAROR(I1,-1,I2,-1,I4,-1);
    VAROR(I1,-1,I2,0,I4,-1);
    VAROR(I1,0,I2,0,I4,0);
    VAROR(I1,-1,UI2,65535,I4,-1);
    VAROR(I1,-1,UI2,0,I4,-1);
    VAROR(I1,0,UI2,0,I4,0);
    VAROR(I1,-1,I4,-1,I4,-1);
    VAROR(I1,-1,I4,0,I4,-1);
    VAROR(I1,0,I4,0,I4,0);
    VAROR(I1,-1,UI4,0xffffffff,I4,-1);
    VAROR(I1,-1,UI4,0,I4,-1);
    VAROR(I1,0,UI4,0,I4,0);
    VAROR(I1,-1,R4,-1,I4,-1);
    VAROR(I1,-1,R4,0,I4,-1);
    VAROR(I1,0,R4,0,I4,0);
    VAROR(I1,-1,R8,-1,I4,-1);
    VAROR(I1,-1,R8,0,I4,-1);
    VAROR(I1,0,R8,0,I4,0);
    VAROR(I1,-1,DATE,-1,I4,-1);
    VAROR(I1,-1,DATE,0,I4,-1);
    VAROR(I1,0,DATE,0,I4,0);
    if (has_i8)
    {
        VAROR(I1,-1,I8,-1,I8,-1);
        VAROR(I1,-1,I8,0,I8,-1);
        VAROR(I1,0,I8,0,I8,0);
        VAROR(I1,-1,UI8,0,I4,-1);
        VAROR(I1,0,UI8,0,I4,0);
    }
    VAROR(I1,-1,INT,-1,I4,-1);
    VAROR(I1,-1,INT,0,I4,-1);
    VAROR(I1,0,INT,0,I4,0);
    VAROR(I1,-1,UINT,0xffffffff,I4,-1);
    VAROR(I1,-1,UINT,0,I4,-1);
    VAROR(I1,0,UINT,0,I4,0);
    rbstr = SysAllocString(szFalse);
    VAROR(I1,0,BSTR,rbstr,I4,0);
    VAROR(I1,-1,BSTR,rbstr,I4,-1);
    SysFreeString(rbstr);
    rbstr = SysAllocString(szTrue);
    VAROR(I1,0,BSTR,rbstr,I4,-1);
    VAROR(I1,-1,BSTR,rbstr,I4,-1);
    SysFreeString(rbstr);
    VARORCY(I1,-1,10000,I4,-1);
    VARORCY(I1,-1,0,I4,-1);
    VARORCY(I1,0,0,I4,0);

    VAROR(UI1,255,UI1,255,UI1,255);
    VAROR(UI1,255,UI1,0,UI1,255);
    VAROR(UI1,0,UI1,0,UI1,0);
    VAROR(UI1,255,I2,-1,I2,-1);
    VAROR(UI1,255,I2,0,I2,255);
    VAROR(UI1,0,I2,0,I2,0);
    VAROR(UI1,255,UI2,65535,I4,65535);
    VAROR(UI1,255,UI2,0,I4,255);
    VAROR(UI1,0,UI2,0,I4,0);
    VAROR(UI1,255,I4,-1,I4,-1);
    VAROR(UI1,255,I4,0,I4,255);
    VAROR(UI1,0,I4,0,I4,0);
    VAROR(UI1,255,UI4,0xffffffff,I4,-1);
    VAROR(UI1,255,UI4,0,I4,255);
    VAROR(UI1,0,UI4,0,I4,0);
    VAROR(UI1,255,R4,-1,I4,-1);
    VAROR(UI1,255,R4,0,I4,255);
    VAROR(UI1,0,R4,0,I4,0);
    VAROR(UI1,255,R8,-1,I4,-1);
    VAROR(UI1,255,R8,0,I4,255);
    VAROR(UI1,0,R8,0,I4,0);
    VAROR(UI1,255,DATE,-1,I4,-1);
    VAROR(UI1,255,DATE,0,I4,255);
    VAROR(UI1,0,DATE,0,I4,0);
    if (has_i8)
    {
        VAROR(UI1,255,I8,-1,I8,-1);
        VAROR(UI1,255,I8,0,I8,255);
        VAROR(UI1,0,I8,0,I8,0);
        VAROR(UI1,255,UI8,0,I4,255);
        VAROR(UI1,0,UI8,0,I4,0);
    }
    VAROR(UI1,255,INT,-1,I4,-1);
    VAROR(UI1,255,INT,0,I4,255);
    VAROR(UI1,0,INT,0,I4,0);
    VAROR(UI1,255,UINT,0xffffffff,I4,-1);
    VAROR(UI1,255,UINT,0,I4,255);
    VAROR(UI1,0,UINT,0,I4,0);
    rbstr = SysAllocString(szFalse);
    VAROR(UI1,0,BSTR,rbstr,I2,0);
    VAROR(UI1,255,BSTR,rbstr,I2,255);
    SysFreeString(rbstr);
    rbstr = SysAllocString(szTrue);
    VAROR(UI1,0,BSTR,rbstr,I2,-1);
    VAROR(UI1,255,BSTR,rbstr,I2,-1);
    SysFreeString(rbstr);
    VARORCY(UI1,255,10000,I4,255);
    VARORCY(UI1,255,0,I4,255);
    VARORCY(UI1,0,0,I4,0);

    VAROR(I2,-1,I2,-1,I2,-1);
    VAROR(I2,-1,I2,0,I2,-1);
    VAROR(I2,0,I2,0,I2,0);
    VAROR(I2,-1,UI2,65535,I4,-1);
    VAROR(I2,-1,UI2,0,I4,-1);
    VAROR(I2,0,UI2,0,I4,0);
    VAROR(I2,-1,I4,-1,I4,-1);
    VAROR(I2,-1,I4,0,I4,-1);
    VAROR(I2,0,I4,0,I4,0);
    VAROR(I2,-1,UI4,0xffffffff,I4,-1);
    VAROR(I2,-1,UI4,0,I4,-1);
    VAROR(I2,0,UI4,0,I4,0);
    VAROR(I2,-1,R4,-1,I4,-1);
    VAROR(I2,-1,R4,0,I4,-1);
    VAROR(I2,0,R4,0,I4,0);
    VAROR(I2,-1,R8,-1,I4,-1);
    VAROR(I2,-1,R8,0,I4,-1);
    VAROR(I2,0,R8,0,I4,0);
    VAROR(I2,-1,DATE,-1,I4,-1);
    VAROR(I2,-1,DATE,0,I4,-1);
    VAROR(I2,0,DATE,0,I4,0);
    if (has_i8)
    {
        VAROR(I2,-1,I8,-1,I8,-1);
        VAROR(I2,-1,I8,0,I8,-1);
        VAROR(I2,0,I8,0,I8,0);
        VAROR(I2,-1,UI8,0,I4,-1);
        VAROR(I2,0,UI8,0,I4,0);
    }
    VAROR(I2,-1,INT,-1,I4,-1);
    VAROR(I2,-1,INT,0,I4,-1);
    VAROR(I2,0,INT,0,I4,0);
    VAROR(I2,-1,UINT,0xffffffff,I4,-1);
    VAROR(I2,-1,UINT,0,I4,-1);
    VAROR(I2,0,UINT,0,I4,0);
    rbstr = SysAllocString(szFalse);
    VAROR(I2,0,BSTR,rbstr,I2,0);
    VAROR(I2,-1,BSTR,rbstr,I2,-1);
    SysFreeString(rbstr);
    rbstr = SysAllocString(szTrue);
    VAROR(I2,0,BSTR,rbstr,I2,-1);
    VAROR(I2,-1,BSTR,rbstr,I2,-1);
    SysFreeString(rbstr);
    VARORCY(I2,-1,10000,I4,-1);
    VARORCY(I2,-1,0,I4,-1);
    VARORCY(I2,0,0,I4,0);

    VAROR(UI2,65535,UI2,65535,I4,65535);
    VAROR(UI2,65535,UI2,0,I4,65535);
    VAROR(UI2,0,UI2,0,I4,0);
    VAROR(UI2,65535,I4,-1,I4,-1);
    VAROR(UI2,65535,I4,0,I4,65535);
    VAROR(UI2,0,I4,0,I4,0);
    VAROR(UI2,65535,UI4,0xffffffff,I4,-1);
    VAROR(UI2,65535,UI4,0,I4,65535);
    VAROR(UI2,0,UI4,0,I4,0);
    VAROR(UI2,65535,R4,-1,I4,-1);
    VAROR(UI2,65535,R4,0,I4,65535);
    VAROR(UI2,0,R4,0,I4,0);
    VAROR(UI2,65535,R8,-1,I4,-1);
    VAROR(UI2,65535,R8,0,I4,65535);
    VAROR(UI2,0,R8,0,I4,0);
    VAROR(UI2,65535,DATE,-1,I4,-1);
    VAROR(UI2,65535,DATE,0,I4,65535);
    VAROR(UI2,0,DATE,0,I4,0);
    if (has_i8)
    {
        VAROR(UI2,65535,I8,-1,I8,-1);
        VAROR(UI2,65535,I8,0,I8,65535);
        VAROR(UI2,0,I8,0,I8,0);
        VAROR(UI2,65535,UI8,0,I4,65535);
        VAROR(UI2,0,UI8,0,I4,0);
    }
    VAROR(UI2,65535,INT,-1,I4,-1);
    VAROR(UI2,65535,INT,0,I4,65535);
    VAROR(UI2,0,INT,0,I4,0);
    VAROR(UI2,65535,UINT,0xffffffff,I4,-1);
    VAROR(UI2,65535,UINT,0,I4,65535);
    VAROR(UI2,0,UINT,0,I4,0);
    rbstr = SysAllocString(szFalse);
    VAROR(UI2,0,BSTR,rbstr,I4,0);
    VAROR(UI2,65535,BSTR,rbstr,I4,65535);
    SysFreeString(rbstr);
    rbstr = SysAllocString(szTrue);
    VAROR(UI2,0,BSTR,rbstr,I4,-1);
    VAROR(UI2,65535,BSTR,rbstr,I4,-1);
    SysFreeString(rbstr);
    VARORCY(UI2,65535,10000,I4,65535);
    VARORCY(UI2,65535,0,I4,65535);
    VARORCY(UI2,0,0,I4,0);

    VAROR(I4,-1,I4,-1,I4,-1);
    VAROR(I4,-1,I4,0,I4,-1);
    VAROR(I4,0,I4,0,I4,0);
    VAROR(I4,-1,UI4,0xffffffff,I4,-1);
    VAROR(I4,-1,UI4,0,I4,-1);
    VAROR(I4,0,UI4,0,I4,0);
    VAROR(I4,-1,R4,-1,I4,-1);
    VAROR(I4,-1,R4,0,I4,-1);
    VAROR(I4,0,R4,0,I4,0);
    VAROR(I4,-1,R8,-1,I4,-1);
    VAROR(I4,-1,R8,0,I4,-1);
    VAROR(I4,0,R8,0,I4,0);
    VAROR(I4,-1,DATE,-1,I4,-1);
    VAROR(I4,-1,DATE,0,I4,-1);
    VAROR(I4,0,DATE,0,I4,0);
    if (has_i8)
    {
        VAROR(I4,-1,I8,-1,I8,-1);
        VAROR(I4,-1,I8,0,I8,-1);
        VAROR(I4,0,I8,0,I8,0);
        VAROR(I4,-1,UI8,0,I4,-1);
        VAROR(I4,0,UI8,0,I4,0);
    }
    VAROR(I4,-1,INT,-1,I4,-1);
    VAROR(I4,-1,INT,0,I4,-1);
    VAROR(I4,0,INT,0,I4,0);
    VAROR(I4,-1,UINT,0xffffffff,I4,-1);
    VAROR(I4,-1,UINT,0,I4,-1);
    VAROR(I4,0,UINT,0,I4,0);
    rbstr = SysAllocString(szFalse);
    VAROR(I4,0,BSTR,rbstr,I4,0);
    VAROR(I4,-1,BSTR,rbstr,I4,-1);
    SysFreeString(rbstr);
    rbstr = SysAllocString(szTrue);
    VAROR(I4,0,BSTR,rbstr,I4,-1);
    VAROR(I4,-1,BSTR,rbstr,I4,-1);
    SysFreeString(rbstr);
    VARORCY(I4,-1,10000,I4,-1);
    VARORCY(I4,-1,0,I4,-1);
    VARORCY(I4,0,0,I4,0);

    VAROR(UI4,0xffffffff,UI4,0xffffffff,I4,-1);
    VAROR(UI4,0xffffffff,UI4,0,I4,-1);
    VAROR(UI4,0,UI4,0,I4,0);
    VAROR(UI4,0xffffffff,R4,-1,I4,-1);
    VAROR(UI4,0xffffffff,R4,0,I4,-1);
    VAROR(UI4,0,R4,0,I4,0);
    VAROR(UI4,0xffffffff,R8,-1,I4,-1);
    VAROR(UI4,0xffffffff,R8,0,I4,-1);
    VAROR(UI4,0,R8,0,I4,0);
    VAROR(UI4,0xffffffff,DATE,-1,I4,-1);
    VAROR(UI4,0xffffffff,DATE,0,I4,-1);
    VAROR(UI4,0,DATE,0,I4,0);
    if (has_i8)
    {
        VAROR(UI4,0xffffffff,I8,-1,I8,-1);
        VAROR(UI4,0xffffffff,I8,0,I8,0xffffffff);
        VAROR(UI4,0,I8,0,I8,0);
        VAROR(UI4,0xffffffff,UI8,0,I4,-1);
        VAROR(UI4,0,UI8,0,I4,0);
    }
    VAROR(UI4,0xffffffff,INT,-1,I4,-1);
    VAROR(UI4,0xffffffff,INT,0,I4,-1);
    VAROR(UI4,0,INT,0,I4,0);
    VAROR(UI4,0xffffffff,UINT,0xffffffff,I4,-1);
    VAROR(UI4,0xffffffff,UINT,0,I4,-1);
    VAROR(UI4,0,UINT,0,I4,0);
    rbstr = SysAllocString(szFalse);
    VAROR(UI4,0,BSTR,rbstr,I4,0);
    VAROR(UI4,0xffffffff,BSTR,rbstr,I4,-1);
    SysFreeString(rbstr);
    rbstr = SysAllocString(szTrue);
    VAROR(UI4,0,BSTR,rbstr,I4,-1);
    VAROR(UI4,0xffffffff,BSTR,rbstr,I4,-1);
    SysFreeString(rbstr);
    VARORCY(UI4,0xffffffff,10000,I4,-1);
    VARORCY(UI4,0xffffffff,0,I4,-1);
    VARORCY(UI4,0,0,I4,0);

    VAROR(R4,-1,R4,-1,I4,-1);
    VAROR(R4,-1,R4,0,I4,-1);
    VAROR(R4,0,R4,0,I4,0);
    VAROR(R4,-1,R8,-1,I4,-1);
    VAROR(R4,-1,R8,0,I4,-1);
    VAROR(R4,0,R8,0,I4,0);
    VAROR(R4,-1,DATE,-1,I4,-1);
    VAROR(R4,-1,DATE,0,I4,-1);
    VAROR(R4,0,DATE,0,I4,0);
    if (has_i8)
    {
        VAROR(R4,-1,I8,-1,I8,-1);
        VAROR(R4,-1,I8,0,I8,-1);
        VAROR(R4,0,I8,0,I8,0);
        VAROR(R4,-1,UI8,0,I4,-1);
        VAROR(R4,0,UI8,0,I4,0);
    }
    VAROR(R4,-1,INT,-1,I4,-1);
    VAROR(R4,-1,INT,0,I4,-1);
    VAROR(R4,0,INT,0,I4,0);
    VAROR(R4,-1,UINT,0xffffffff,I4,-1);
    VAROR(R4,-1,UINT,0,I4,-1);
    VAROR(R4,0,UINT,0,I4,0);
    rbstr = SysAllocString(szFalse);
    VAROR(R4,0,BSTR,rbstr,I4,0);
    VAROR(R4,-1,BSTR,rbstr,I4,-1);
    SysFreeString(rbstr);
    rbstr = SysAllocString(szTrue);
    VAROR(R4,0,BSTR,rbstr,I4,-1);
    VAROR(R4,-1,BSTR,rbstr,I4,-1);
    SysFreeString(rbstr);
    VARORCY(R4,-1,10000,I4,-1);
    VARORCY(R4,-1,0,I4,-1);
    VARORCY(R4,0,0,I4,0);

    VAROR(R8,-1,R8,-1,I4,-1);
    VAROR(R8,-1,R8,0,I4,-1);
    VAROR(R8,0,R8,0,I4,0);
    VAROR(R8,-1,DATE,-1,I4,-1);
    VAROR(R8,-1,DATE,0,I4,-1);
    VAROR(R8,0,DATE,0,I4,0);
    if (has_i8)
    {
        VAROR(R8,-1,I8,-1,I8,-1);
        VAROR(R8,-1,I8,0,I8,-1);
        VAROR(R8,0,I8,0,I8,0);
        VAROR(R8,-1,UI8,0,I4,-1);
        VAROR(R8,0,UI8,0,I4,0);
    }
    VAROR(R8,-1,INT,-1,I4,-1);
    VAROR(R8,-1,INT,0,I4,-1);
    VAROR(R8,0,INT,0,I4,0);
    VAROR(R8,-1,UINT,0xffffffff,I4,-1);
    VAROR(R8,-1,UINT,0,I4,-1);
    VAROR(R8,0,UINT,0,I4,0);
    rbstr = SysAllocString(szFalse);
    VAROR(R8,0,BSTR,rbstr,I4,0);
    VAROR(R8,-1,BSTR,rbstr,I4,-1);
    SysFreeString(rbstr);
    rbstr = SysAllocString(szTrue);
    VAROR(R8,0,BSTR,rbstr,I4,-1);
    VAROR(R8,-1,BSTR,rbstr,I4,-1);
    SysFreeString(rbstr);
    VARORCY(R8,-1,10000,I4,-1);
    VARORCY(R8,-1,0,I4,-1);
    VARORCY(R8,0,0,I4,0);

    VAROR(DATE,-1,DATE,-1,I4,-1);
    VAROR(DATE,-1,DATE,0,I4,-1);
    VAROR(DATE,0,DATE,0,I4,0);
    if (has_i8)
    {
        VAROR(DATE,-1,I8,-1,I8,-1);
        VAROR(DATE,-1,I8,0,I8,-1);
        VAROR(DATE,0,I8,0,I8,0);
        VAROR(DATE,-1,UI8,0,I4,-1);
        VAROR(DATE,0,UI8,0,I4,0);
    }
    VAROR(DATE,-1,INT,-1,I4,-1);
    VAROR(DATE,-1,INT,0,I4,-1);
    VAROR(DATE,0,INT,0,I4,0);
    VAROR(DATE,-1,UINT,0xffffffff,I4,-1);
    VAROR(DATE,-1,UINT,0,I4,-1);
    VAROR(DATE,0,UINT,0,I4,0);
    rbstr = SysAllocString(szFalse);
    VAROR(DATE,0,BSTR,rbstr,I4,0);
    VAROR(DATE,-1,BSTR,rbstr,I4,-1);
    SysFreeString(rbstr);
    rbstr = SysAllocString(szTrue);
    VAROR(DATE,0,BSTR,rbstr,I4,-1);
    VAROR(DATE,-1,BSTR,rbstr,I4,-1);
    SysFreeString(rbstr);
    VARORCY(DATE,-1,10000,I4,-1);
    VARORCY(DATE,-1,0,I4,-1);
    VARORCY(DATE,0,0,I4,0);

    if (has_i8)
    {
        VAROR(I8,-1,I8,-1,I8,-1);
        VAROR(I8,-1,I8,0,I8,-1);
        VAROR(I8,0,I8,0,I8,0);
        VAROR(I8,-1,UI8,0,I8,-1);
        VAROR(I8,0,UI8,0,I8,0);
        /* These overflow under native and Wine
        VAROR(I8,-1,INT,-1,I4,-1);
        VAROR(I8,-1,INT,0,I4,-1);
        VAROR(I8,0,INT,0,I4,0); */
        VAROR(I8,-1,UINT,0xffffffff,I8,-1);
        VAROR(I8,-1,UINT,0,I8,-1);
        VAROR(I8,0,UINT,0,I8,0);
        rbstr = SysAllocString(szFalse);
        VAROR(I8,0,BSTR,rbstr,I8,0);
        VAROR(I8,-1,BSTR,rbstr,I8,-1);
        SysFreeString(rbstr);
        rbstr = SysAllocString(szTrue);
        VAROR(I8,0,BSTR,rbstr,I8,-1);
        VAROR(I8,-1,BSTR,rbstr,I8,-1);
        SysFreeString(rbstr);
        VARORCY(I8,-1,10000,I8,-1);
        VARORCY(I8,-1,0,I8,-1);
        VARORCY(I8,0,0,I8,0);

        VAROR(UI8,0xffff,UI8,0xffff,I4,0xffff);
        VAROR(UI8,0xffff,UI8,0,I4,0xffff);
        VAROR(UI8,0,UI8,0,I4,0);
        VAROR(UI8,0xffff,INT,-1,I4,-1);
        VAROR(UI8,0xffff,INT,0,I4,0xffff);
        VAROR(UI8,0,INT,0,I4,0);
        VAROR(UI8,0xffff,UINT,0xffff,I4,0xffff);
        VAROR(UI8,0xffff,UINT,0,I4,0xffff);
        VAROR(UI8,0,UINT,0,I4,0);
        rbstr = SysAllocString(szFalse);
        VAROR(UI8,0,BSTR,rbstr,I4,0);
        VAROR(UI8,0xffff,BSTR,rbstr,I4,0xffff);
        SysFreeString(rbstr);
        rbstr = SysAllocString(szTrue);
        VAROR(UI8,0,BSTR,rbstr,I4,-1);
        VAROR(UI8,0xffff,BSTR,rbstr,I4,-1);
        SysFreeString(rbstr);
        VARORCY(UI8,0xffff,10000,I4,0xffff);
        VARORCY(UI8,0xffff,0,I4,0xffff);
        VARORCY(UI8,0,0,I4,0);
    }

    VAROR(INT,-1,INT,-1,I4,-1);
    VAROR(INT,-1,INT,0,I4,-1);
    VAROR(INT,0,INT,0,I4,0);
    VAROR(INT,-1,UINT,0xffff,I4,-1);
    VAROR(INT,-1,UINT,0,I4,-1);
    VAROR(INT,0,UINT,0,I4,0);
    rbstr = SysAllocString(szFalse);
    VAROR(INT,0,BSTR,rbstr,I4,0);
    VAROR(INT,-1,BSTR,rbstr,I4,-1);
    SysFreeString(rbstr);
    rbstr = SysAllocString(szTrue);
    VAROR(INT,0,BSTR,rbstr,I4,-1);
    VAROR(INT,-1,BSTR,rbstr,I4,-1);
    SysFreeString(rbstr);
    VARORCY(INT,-1,10000,I4,-1);
    VARORCY(INT,-1,0,I4,-1);
    VARORCY(INT,0,0,I4,0);

    VAROR(UINT,0xffff,UINT,0xffff,I4,0xffff);
    VAROR(UINT,0xffff,UINT,0,I4,0xffff);
    VAROR(UINT,0,UINT,0,I4,0);
    rbstr = SysAllocString(szFalse);
    VAROR(UINT,0,BSTR,rbstr,I4,0);
    VAROR(UINT,0xffff,BSTR,rbstr,I4,0xffff);
    SysFreeString(rbstr);
    rbstr = SysAllocString(szTrue);
    VAROR(UINT,0,BSTR,rbstr,I4,-1);
    VAROR(UINT,0xffff,BSTR,rbstr,I4,-1);
    SysFreeString(rbstr);
    VARORCY(UINT,0xffff,10000,I4,0xffff);
    VARORCY(UINT,0xffff,0,I4,0xffff);
    VARORCY(UINT,0,0,I4,0);

    lbstr = SysAllocString(szFalse);
    rbstr = SysAllocString(szFalse);
    VAROR(BSTR,lbstr,BSTR,rbstr,BOOL,0);
    SysFreeString(rbstr);
    rbstr = SysAllocString(szTrue);
    VAROR(BSTR,lbstr,BSTR,rbstr,BOOL,VARIANT_TRUE);
    SysFreeString(lbstr);
    lbstr = SysAllocString(szTrue);
    VAROR(BSTR,lbstr,BSTR,rbstr,BOOL,VARIANT_TRUE);
    VARORCY(BSTR,lbstr,10000,I4,-1);
    SysFreeString(lbstr);
    lbstr = SysAllocString(szFalse);
    VARORCY(BSTR,lbstr,10000,I4,1);
    SysFreeString(lbstr);
    SysFreeString(rbstr);
}

static HRESULT (WINAPI *pVarEqv)(LPVARIANT,LPVARIANT,LPVARIANT);

#define VAREQV(vt1,val1,vt2,val2,rvt,rval)           \
    V_VT(&left) = VT_##vt1; V_##vt1(&left) = val1;   \
    V_VT(&right) = VT_##vt2; V_##vt2(&right) = val2; \
    V_VT(&exp) = VT_##rvt; V_##rvt(&exp) = rval;     \
    test_var_call2( __LINE__, pVarEqv, &left, &right, &exp )

static void test_VarEqv(void)
{
    VARIANT left, right, exp, result;
    VARTYPE i;
    HRESULT hres;

    CHECKPTR(VarEqv);

    /* Test all possible flag/vt combinations & the resulting vt type */
    for (i = 0; i < ARRAY_SIZE(ExtraFlags); i++)
    {
        VARTYPE leftvt, rightvt, resvt;

        for (leftvt = 0; leftvt <= VT_BSTR_BLOB; leftvt++)
        {
            SKIPTESTS(leftvt);

            for (rightvt = 0; rightvt <= VT_BSTR_BLOB; rightvt++)
            {
                BOOL bFail = FALSE;

                SKIPTESTS(rightvt);

                if (leftvt == VT_BSTR || rightvt == VT_BSTR ||
                    leftvt == VT_DISPATCH || rightvt == VT_DISPATCH ||
                    leftvt == VT_UNKNOWN || rightvt == VT_UNKNOWN)
                    continue;

                memset(&left, 0, sizeof(left));
                memset(&right, 0, sizeof(right));
                V_VT(&left) = leftvt | ExtraFlags[i];
                V_VT(&right) = rightvt | ExtraFlags[i];
                V_VT(&result) = VT_EMPTY;
                resvt = VT_I4;

                if (ExtraFlags[i] & VT_ARRAY || ExtraFlags[i] & VT_BYREF ||
                    !IsValidVariantClearVT(leftvt, ExtraFlags[i]) ||
                    !IsValidVariantClearVT(rightvt, ExtraFlags[i]) ||
                    leftvt == VT_CLSID || rightvt == VT_CLSID ||
                    leftvt == VT_RECORD || rightvt == VT_RECORD ||
                    leftvt == VT_VARIANT || rightvt == VT_VARIANT ||
                    leftvt == VT_ERROR || rightvt == VT_ERROR)
                {
                    bFail = TRUE;
                }
                if (leftvt == VT_EMPTY || rightvt == VT_EMPTY)
                {
                    if (leftvt == rightvt ||
                        leftvt == VT_I2 || rightvt == VT_I2 ||
                        leftvt == VT_UI1 || rightvt == VT_UI1 ||
                        leftvt == VT_BOOL || rightvt == VT_BOOL)
                        resvt = VT_I2;
                    else if (leftvt == VT_NULL || rightvt == VT_NULL)
                        resvt = VT_NULL;
                    else if (leftvt == VT_I8 || rightvt == VT_I8)
                        resvt = VT_I8;
                }
                else if (leftvt == VT_NULL || rightvt == VT_NULL)
                {
                    resvt = VT_NULL;
                }
                else if (leftvt == VT_UI1 || rightvt == VT_UI1)
                {
                    if (leftvt == rightvt)
                        resvt = VT_UI1;
                    else if (leftvt == rightvt ||
                        leftvt == VT_I2 || rightvt == VT_I2 ||
                        leftvt == VT_BOOL || rightvt == VT_BOOL)
                    {
                        resvt = VT_I2;
                    }
                    else if (leftvt == VT_I8 || rightvt == VT_I8)
                        resvt = VT_I8;
                }
                else if (leftvt == VT_I2 || rightvt == VT_I2)
                {
                    if (leftvt == rightvt ||
                        leftvt == VT_BOOL || rightvt == VT_BOOL)
                        resvt = VT_I2;
                    else if (leftvt == VT_I8 || rightvt == VT_I8)
                        resvt = VT_I8;
                }
                else if (leftvt == VT_BOOL && rightvt == VT_BOOL)
                {
                    resvt = VT_BOOL;
                }
                else if (leftvt == VT_I8 || rightvt == VT_I8)
                {
                    if (leftvt == VT_INT || rightvt == VT_INT)
                        bFail = TRUE;
                    else
                        resvt = VT_I8;
                }
                hres = pVarEqv(&left, &right, &result);
                if (bFail)
                    ok(hres == DISP_E_TYPEMISMATCH || hres == DISP_E_BADVARTYPE,
                       "VarEqv: %d|0x%X, %d|0x%X: Expected failure, got 0x%lX vt %d\n",
                       leftvt, ExtraFlags[i], rightvt, ExtraFlags[i], hres,
                       V_VT(&result));
                else
                    ok(hres == S_OK && V_VT(&result) == resvt,
                       "VarEqv: %d|0x%X, %d|0x%X: expected S_OK, vt %d, got 0x%lX vt %d\n",
                       leftvt, ExtraFlags[i], rightvt, ExtraFlags[i], resvt, hres,
                       V_VT(&result));
            }
        }
    }

    /* Test returned values */
    VAREQV(BOOL,VARIANT_TRUE,BOOL,VARIANT_TRUE,BOOL,VARIANT_TRUE);
    VAREQV(BOOL,VARIANT_FALSE,BOOL,VARIANT_TRUE,BOOL,VARIANT_FALSE);
    VAREQV(BOOL,TRUE,BOOL,TRUE,BOOL,VARIANT_TRUE);
    VAREQV(BOOL,FALSE,BOOL,FALSE,BOOL,VARIANT_TRUE);
    VAREQV(BOOL,TRUE,BOOL,FALSE,BOOL,-2);
    VAREQV(BOOL,FALSE,BOOL,TRUE,BOOL,-2);
    VAREQV(BOOL,6,BOOL,7,BOOL,-2);
    VAREQV(BOOL,6,BOOL,6,BOOL,VARIANT_TRUE);
    VAREQV(BOOL,VARIANT_TRUE,I2,VARIANT_TRUE,I2,VARIANT_TRUE);
    VAREQV(BOOL,VARIANT_TRUE,I2,VARIANT_FALSE,I2,VARIANT_FALSE);
    VAREQV(BOOL,6,I2,7,I2,-2);
    VAREQV(UI1,1,UI1,1,UI1,255);
    VAREQV(UI1,1,UI1,0,UI1,254);
    VAREQV(UI1,0,UI1,1,UI1,254);
    if (has_i8)
    {
        VAREQV(UI4,VARIANT_FALSE,I8,VARIANT_FALSE,I8,-1);
        VAREQV(UI4,5,I8,19,I8,-23);
        VAREQV(UI4,VARIANT_FALSE,UI8,VARIANT_FALSE,I4,-1);
    }
}

static HRESULT (WINAPI *pVarMul)(LPVARIANT,LPVARIANT,LPVARIANT);

#define VARMUL(vt1,val1,vt2,val2,rvt,rval)               \
        V_VT(&left) = VT_##vt1; V_##vt1(&left) = val1;   \
        V_VT(&right) = VT_##vt2; V_##vt2(&right) = val2; \
        V_VT(&exp) = VT_##rvt; V_##rvt(&exp) = rval;     \
        test_var_call2( __LINE__, pVarMul, &left, &right, &exp )

static void test_VarMul(void)
{
    VARIANT left, right, exp, result, cy, dec;
    VARTYPE i;
    BSTR lbstr, rbstr;
    HRESULT hres;
    double r;

    CHECKPTR(VarMul);

    lbstr = SysAllocString(L"12");
    rbstr = SysAllocString(L"12");

    /* Test all possible flag/vt combinations & the resulting vt type */
    for (i = 0; i < ARRAY_SIZE(ExtraFlags); i++)
    {
        VARTYPE leftvt, rightvt, resvt;

        for (leftvt = 0; leftvt <= VT_BSTR_BLOB; leftvt++)
        {

            SKIPTESTS(leftvt);

            for (rightvt = 0; rightvt <= VT_BSTR_BLOB; rightvt++)
            {
                BOOL bFail = FALSE;

                SKIPTESTS(rightvt);

                if (leftvt == VT_DISPATCH || rightvt == VT_DISPATCH ||
                    leftvt == VT_UNKNOWN || rightvt == VT_UNKNOWN)
                    continue;

                memset(&left, 0, sizeof(left));
                memset(&right, 0, sizeof(right));
                V_VT(&left) = leftvt | ExtraFlags[i];
                if (leftvt == VT_BSTR)
                    V_BSTR(&left) = lbstr;
                V_VT(&right) = rightvt | ExtraFlags[i];
                if (rightvt == VT_BSTR)
                    V_BSTR(&right) = rbstr;
                V_VT(&result) = VT_EMPTY;
                resvt = VT_UNKNOWN;

                /* Don't ask me why but native VarMul cannot handle:
                   VT_I1, VT_UI2, VT_UI4, VT_INT, VT_UINT and VT_UI8.
                   Tested with DCOM98, Win2k, WinXP */
                if (ExtraFlags[i] & VT_ARRAY || ExtraFlags[i] & VT_BYREF ||
                    !IsValidVariantClearVT(leftvt, ExtraFlags[i]) ||
                    !IsValidVariantClearVT(rightvt, ExtraFlags[i]) ||
                    leftvt == VT_CLSID || rightvt == VT_CLSID ||
                    leftvt == VT_RECORD || rightvt == VT_RECORD ||
                    leftvt == VT_VARIANT || rightvt == VT_VARIANT ||
                    leftvt == VT_ERROR || rightvt == VT_ERROR ||
                    leftvt == VT_I1 || rightvt == VT_I1 ||
                    leftvt == VT_UI2 || rightvt == VT_UI2 ||
                    leftvt == VT_UI4 || rightvt == VT_UI4 ||
                    leftvt == VT_UI8 || rightvt == VT_UI8 ||
                    leftvt == VT_INT || rightvt == VT_INT ||
                    leftvt == VT_UINT || rightvt == VT_UINT) {
                    bFail = TRUE;
                }

                if (leftvt == VT_NULL || rightvt == VT_NULL)
                    resvt = VT_NULL;
                else if (leftvt == VT_DECIMAL || rightvt == VT_DECIMAL)
                    resvt = VT_DECIMAL;
                else if (leftvt == VT_R8 || rightvt == VT_R8 ||
                         leftvt == VT_BSTR || rightvt == VT_BSTR ||
                         leftvt == VT_DATE || rightvt == VT_DATE)
                    resvt = VT_R8;
                else if (leftvt == VT_R4 || rightvt == VT_R4) {
                    if (leftvt == VT_I4 || rightvt == VT_I4 ||
                        leftvt == VT_I8 || rightvt == VT_I8 ||
                        leftvt == VT_CY || rightvt == VT_CY)
                        resvt = VT_R8;
                    else
                        resvt = VT_R4;
                } else if (leftvt == VT_CY || rightvt == VT_CY)
                    resvt = VT_CY;
                else if (leftvt == VT_I8 || rightvt == VT_I8)
                    resvt = VT_I8;
                else if (leftvt == VT_I4 || rightvt == VT_I4)
                    resvt = VT_I4;
                else if (leftvt == VT_I2 || rightvt == VT_I2 ||
                         leftvt == VT_BOOL || rightvt == VT_BOOL ||
                         (leftvt == VT_EMPTY && rightvt == VT_EMPTY))
                    resvt = VT_I2;
                else if (leftvt == VT_UI1 || rightvt == VT_UI1)
                    resvt = VT_UI1;

                hres = pVarMul(&left, &right, &result);
                if (bFail) {
                    ok(hres == DISP_E_TYPEMISMATCH || hres == DISP_E_BADVARTYPE,
                       "VarMul: %d|0x%X, %d|0x%X: Expected failure, got 0x%lX vt %d\n",
                       leftvt, ExtraFlags[i], rightvt, ExtraFlags[i], hres,
                       V_VT(&result));
                } else {
                    ok(hres == S_OK && V_VT(&result) == resvt,
                       "VarMul: %d|0x%X, %d|0x%X: expected S_OK, vt %d, got 0x%lX vt %d\n",
                       leftvt, ExtraFlags[i], rightvt, ExtraFlags[i], resvt, hres,
                       V_VT(&result));
                }
            }
        }
    }

    /* Test returned values */
    VARMUL(I4,4,I4,2,I4,8);
    VARMUL(I2,4,I2,2,I2,8);
    VARMUL(I2,-13,I4,5,I4,-65);
    VARMUL(I4,-13,I4,5,I4,-65);
    VARMUL(I2,7,R4,0.5f,R4,3.5f);
    VARMUL(R4,0.5f,I4,5,R8,2.5);
    VARMUL(R8,7.1,BOOL,0,R8,0);
    VARMUL(BSTR,lbstr,I2,4,R8,48);
    VARMUL(BSTR,lbstr,BOOL,1,R8,12);
    VARMUL(BSTR,lbstr,R4,0.1f,R8,1.2);
    VARMUL(BSTR,lbstr,BSTR,rbstr,R8,144);
    VARMUL(R4,0.2f,BSTR,rbstr,R8,2.4);
    VARMUL(DATE,2.25,I4,7,R8,15.75);

    VARMUL(UI1, UI1_MAX, UI1, UI1_MAX, I4, UI1_MAX * UI1_MAX);
    VARMUL(I2, I2_MAX, I2, I2_MAX, I4, I2_MAX * I2_MAX);
    VARMUL(I2, I2_MAX, I2, I2_MIN, I4, I2_MAX * I2_MIN);
    VARMUL(I2, I2_MIN, I2, I2_MIN, I4, I2_MIN * I2_MIN);
    VARMUL(I4, I4_MAX, I4, I4_MAX, R8, (double)I4_MAX * I4_MAX);
    VARMUL(I4, I4_MAX, I4, I4_MIN, R8, (double)I4_MAX * I4_MIN);
    VARMUL(I4, I4_MIN, I4, I4_MIN, R8, (double)I4_MIN * I4_MIN);
    VARMUL(R4, R4_MAX, R4, R4_MAX, R8, (double)R4_MAX * R4_MAX);
    VARMUL(R4, R4_MAX, R4, R4_MIN, R4, R4_MAX * R4_MIN);
    VARMUL(R4, R4_MIN, R4, R4_MIN, R4, R4_MIN * R4_MIN);
    VARMUL(R8, R8_MAX, R8, R8_MIN, R8, R8_MAX * R8_MIN);
    VARMUL(R8, R8_MIN, R8, R8_MIN, R8, R8_MIN * R8_MIN);

    /* Manuly test some VT_CY and VT_DECIMAL variants */
    V_VT(&cy) = VT_CY;
    hres = VarCyFromI4(4711, &V_CY(&cy));
    ok(hres == S_OK, "VarCyFromI4 failed!\n");
    V_VT(&dec) = VT_DECIMAL;
    hres = VarDecFromR8(-4.2, &V_DECIMAL(&dec));
    ok(hres == S_OK, "VarDecFromR4 failed!\n");
    memset(&left, 0, sizeof(left));
    memset(&right, 0, sizeof(right));
    V_VT(&left) = VT_I4;
    V_I4(&left) = -11;
    V_VT(&right) = VT_UI1;
    V_UI1(&right) = 9;

    hres = pVarMul(&cy, &right, &result);
    ok(hres == S_OK && V_VT(&result) == VT_CY, "VarMul: expected coerced type VT_CY, got %s!\n", wine_dbgstr_vt(V_VT(&result)));
    hres = VarR8FromCy(V_CY(&result), &r);
    ok(hres == S_OK && EQ_DOUBLE(r, 42399.0), "VarMul: CY value %f, expected %f\n", r, 42399.0);

    hres = pVarMul(&left, &dec, &result);
    ok(hres == S_OK && V_VT(&result) == VT_DECIMAL, "VarMul: expected coerced type VT_DECIMAL, got %s!\n", wine_dbgstr_vt(V_VT(&result)));
    hres = VarR8FromDec(&V_DECIMAL(&result), &r);
    ok(hres == S_OK && EQ_DOUBLE(r, 46.2), "VarMul: DECIMAL value %f, expected %f\n", r, 46.2);

    SysFreeString(lbstr);
    SysFreeString(rbstr);
}

static HRESULT (WINAPI *pVarAdd)(LPVARIANT,LPVARIANT,LPVARIANT);

#define VARADD(vt1,val1,vt2,val2,rvt,rval)               \
        V_VT(&left) = VT_##vt1; V_##vt1(&left) = val1;   \
        V_VT(&right) = VT_##vt2; V_##vt2(&right) = val2; \
        V_VT(&exp) = VT_##rvt; V_##rvt(&exp) = rval;     \
        test_var_call2( __LINE__, pVarAdd, &left, &right, &exp )

static void test_VarAdd(void)
{
    VARIANT left, right, exp, result, cy, dec;
    VARTYPE i;
    BSTR lbstr, rbstr;
    HRESULT hres;
    double r;

    CHECKPTR(VarAdd);

    lbstr = SysAllocString(L"12");
    rbstr = SysAllocString(L"12");

    /* Test all possible flag/vt combinations & the resulting vt type */
    for (i = 0; i < ARRAY_SIZE(ExtraFlags); i++)
    {
        VARTYPE leftvt, rightvt, resvt;

        for (leftvt = 0; leftvt <= VT_BSTR_BLOB; leftvt++)
        {

            SKIPTESTS(leftvt);

            for (rightvt = 0; rightvt <= VT_BSTR_BLOB; rightvt++)
            {
                BOOL bFail = FALSE;

                SKIPTESTS(rightvt);

                if (leftvt == VT_UNKNOWN || rightvt == VT_UNKNOWN)
                    continue;

                memset(&left, 0, sizeof(left));
                memset(&right, 0, sizeof(right));
                V_VT(&left) = leftvt | ExtraFlags[i];
                if (leftvt == VT_BSTR)
                    V_BSTR(&left) = lbstr;
                V_VT(&right) = rightvt | ExtraFlags[i];
                if (rightvt == VT_BSTR)
                    V_BSTR(&right) = rbstr;
                V_VT(&result) = VT_EMPTY;
                resvt = VT_ERROR;

                /* Don't ask me why but native VarAdd cannot handle:
                   VT_I1, VT_UI2, VT_UI4, VT_INT, VT_UINT and VT_UI8.
                   Tested with DCOM98, Win2k, WinXP */
                if (ExtraFlags[i] & VT_ARRAY || ExtraFlags[i] & VT_BYREF ||
                    !IsValidVariantClearVT(leftvt, ExtraFlags[i]) ||
                    !IsValidVariantClearVT(rightvt, ExtraFlags[i]) ||
                    leftvt == VT_CLSID || rightvt == VT_CLSID ||
                    leftvt == VT_RECORD || rightvt == VT_RECORD ||
                    leftvt == VT_VARIANT || rightvt == VT_VARIANT ||
                    leftvt == VT_ERROR || rightvt == VT_ERROR ||
                    leftvt == VT_I1 || rightvt == VT_I1 ||
                    leftvt == VT_UI2 || rightvt == VT_UI2 ||
                    leftvt == VT_UI4 || rightvt == VT_UI4 ||
                    leftvt == VT_UI8 || rightvt == VT_UI8 ||
                    leftvt == VT_INT || rightvt == VT_INT ||
                    leftvt == VT_UINT || rightvt == VT_UINT) {
                    bFail = TRUE;
                }

                if (leftvt == VT_NULL || rightvt == VT_NULL)
                    resvt = VT_NULL;
                else if (leftvt == VT_DISPATCH || rightvt == VT_DISPATCH)
                    bFail = TRUE;
                else if (leftvt == VT_DECIMAL || rightvt == VT_DECIMAL)
                    resvt = VT_DECIMAL;
                else if (leftvt == VT_DATE || rightvt == VT_DATE)
                    resvt = VT_DATE;
                else if (leftvt == VT_CY || rightvt == VT_CY)
                    resvt = VT_CY;
                else if (leftvt == VT_R8 || rightvt == VT_R8)
                    resvt = VT_R8;
                else if (leftvt == VT_BSTR || rightvt == VT_BSTR) {
                    if ((leftvt == VT_BSTR && rightvt == VT_BSTR) ||
                         leftvt == VT_EMPTY || rightvt == VT_EMPTY)
                        resvt = VT_BSTR;
                    else
                        resvt = VT_R8;
                } else if (leftvt == VT_R4 || rightvt == VT_R4) {
                    if (leftvt == VT_I4 || rightvt == VT_I4 ||
                        leftvt == VT_I8 || rightvt == VT_I8)
                        resvt = VT_R8;
                    else
                        resvt = VT_R4;
                }
                else if (leftvt == VT_I8 || rightvt == VT_I8)
                    resvt = VT_I8;
                else if (leftvt == VT_I4 || rightvt == VT_I4)
                    resvt = VT_I4;
                else if (leftvt == VT_I2 || rightvt == VT_I2 ||
                         leftvt == VT_BOOL || rightvt == VT_BOOL ||
                         (leftvt == VT_EMPTY && rightvt == VT_EMPTY))
                    resvt = VT_I2;
                else if (leftvt == VT_UI1 || rightvt == VT_UI1)
                    resvt = VT_UI1;

                hres = pVarAdd(&left, &right, &result);
                if (bFail) {
                    ok(hres == DISP_E_TYPEMISMATCH || hres == DISP_E_BADVARTYPE,
                       "VarAdd: %d|0x%X, %d|0x%X: Expected failure, got 0x%lX vt %d\n",
                       leftvt, ExtraFlags[i], rightvt, ExtraFlags[i], hres,
                       V_VT(&result));
                } else {
                    ok(hres == S_OK && V_VT(&result) == resvt,
                       "VarAdd: %d|0x%X, %d|0x%X: expected S_OK, vt %d, got 0x%lX vt %d\n",
                       leftvt, ExtraFlags[i], rightvt, ExtraFlags[i], resvt, hres,
                       V_VT(&result));
                }
                /* Note, we don't clear left/right deliberately here */
                VariantClear(&result);
            }
        }
    }

    /* Test returned values */
    VARADD(I4,4,I4,2,I4,6);
    VARADD(I2,4,I2,2,I2,6);
    VARADD(I2,-13,I4,5,I4,-8);
    VARADD(I4,-13,I4,5,I4,-8);
    VARADD(I2,7,R4,0.5f,R4,7.5f);
    VARADD(R4,0.5f,I4,5,R8,5.5);
    VARADD(R8,7.1,BOOL,0,R8,7.1);
    VARADD(BSTR,lbstr,I2,4,R8,16);
    VARADD(BSTR,lbstr,BOOL,1,R8,13);
    VARADD(BSTR,lbstr,R4,0.1f,R8,12.1);
    VARADD(R4,0.2f,BSTR,rbstr,R8,12.2);
    VARADD(DATE,2.25,I4,7,DATE,9.25);
    VARADD(DATE,1.25,R4,-1.7f,DATE,-0.45);

    VARADD(UI1, UI1_MAX, UI1, UI1_MAX, I2, UI1_MAX + UI1_MAX);
    VARADD(I2, I2_MAX, I2, I2_MAX, I4, I2_MAX + I2_MAX);
    VARADD(I2, I2_MAX, I2, I2_MIN, I2, I2_MAX + I2_MIN);
    VARADD(I2, I2_MIN, I2, I2_MIN, I4, I2_MIN + I2_MIN);
    VARADD(I4, I4_MAX, I4, I4_MIN, I4, I4_MAX + I4_MIN);
    VARADD(I4, I4_MAX, I4, I4_MAX, R8, (double)I4_MAX + I4_MAX);
    VARADD(I4, I4_MIN, I4, I4_MIN, R8, (double)I4_MIN + I4_MIN);
    VARADD(R4, R4_MAX, R4, R4_MAX, R8, (double)R4_MAX + R4_MAX);
    VARADD(R4, R4_MAX, R4, R4_MIN, R4, R4_MAX + R4_MIN);
    VARADD(R4, R4_MIN, R4, R4_MIN, R4, R4_MIN + R4_MIN);
    VARADD(R8, R8_MAX, R8, R8_MIN, R8, R8_MAX + R8_MIN);
    VARADD(R8, R8_MIN, R8, R8_MIN, R8, R8_MIN + R8_MIN);

    /* Manually test BSTR + BSTR */
    V_VT(&left) = VT_BSTR;
    V_BSTR(&left) = lbstr;
    V_VT(&right) = VT_BSTR;
    V_BSTR(&right) = rbstr;
    hres = pVarAdd(&left, &right, &result);
    ok(hres == S_OK && V_VT(&result) == VT_BSTR, "VarAdd: expected coerced type VT_BSTR, got %s!\n", wine_dbgstr_vt(V_VT(&result)));
    hres = VarR8FromStr(V_BSTR(&result), 0, 0, &r);
    ok(hres == S_OK && EQ_DOUBLE(r, 1212.0), "VarAdd: BSTR value %f, expected %f\n", r, 1212.0);
    VariantClear(&result);

    /* Manuly test some VT_CY and VT_DECIMAL variants */
    V_VT(&cy) = VT_CY;
    hres = VarCyFromI4(4711, &V_CY(&cy));
    ok(hres == S_OK, "VarCyFromI4 failed!\n");
    V_VT(&dec) = VT_DECIMAL;
    hres = VarDecFromR8(-4.2, &V_DECIMAL(&dec));
    ok(hres == S_OK, "VarDecFromR4 failed!\n");
    memset(&left, 0, sizeof(left));
    memset(&right, 0, sizeof(right));
    V_VT(&left) = VT_I4;
    V_I4(&left) = -11;
    V_VT(&right) = VT_UI1;
    V_UI1(&right) = 9;

    hres = pVarAdd(&cy, &right, &result);
    ok(hres == S_OK && V_VT(&result) == VT_CY, "VarAdd: expected coerced type VT_CY, got %s!\n", wine_dbgstr_vt(V_VT(&result)));
    hres = VarR8FromCy(V_CY(&result), &r);
    ok(hres == S_OK && EQ_DOUBLE(r, 4720.0), "VarAdd: CY value %f, expected %f\n", r, 4720.0);

    hres = pVarAdd(&left, &dec, &result);
    ok(hres == S_OK && V_VT(&result) == VT_DECIMAL, "VarAdd: expected coerced type VT_DECIMAL, got %s!\n", wine_dbgstr_vt(V_VT(&result)));
    hres = VarR8FromDec(&V_DECIMAL(&result), &r);
    ok(hres == S_OK && EQ_DOUBLE(r, -15.2), "VarAdd: DECIMAL value %f, expected %f\n", r, -15.2);
    VariantClear(&result);

    SysFreeString(lbstr);
    SysFreeString(rbstr);
}

static void test_VarCat(void)
{
    LCID lcid;
    VARIANT left, right, result;
    CHAR orig_date_format[128];
    VARTYPE leftvt, rightvt, resultvt;
    HRESULT hres;
    HRESULT expected_error_num;
    int cmp;
    DummyDispatch dispatch;

    /* Set date format for testing */
    lcid = LOCALE_USER_DEFAULT;
    GetLocaleInfoA(lcid,LOCALE_SSHORTDATE,orig_date_format,128);
    SetLocaleInfoA(lcid,LOCALE_SSHORTDATE,"M/d/yyyy");

    VariantInit(&left);
    VariantInit(&right);
    VariantInit(&result);

    /* Check expected types for all combinations */
    for (leftvt = 0; leftvt <= VT_BSTR_BLOB; leftvt++)
    {

        SKIPTESTS(leftvt);

        /* Check if we need/have support for I8 and/or UI8 */
        if ((leftvt == VT_I8 || leftvt == VT_UI8) && !has_i8)
            continue;

        for (rightvt = 0; rightvt <= VT_BSTR_BLOB; rightvt++)
        {

            SKIPTESTS(rightvt);
            expected_error_num = S_OK;
            resultvt = VT_EMPTY;

            if (leftvt == VT_DISPATCH || rightvt == VT_DISPATCH ||
                leftvt == VT_UNKNOWN || rightvt == VT_UNKNOWN  ||
                leftvt == VT_RECORD || rightvt == VT_RECORD  ||
                leftvt == 15 || rightvt == 15 /* Undefined type */)
                continue;

            /* Check if we need/have support for I8 and/or UI8 */
            if ((rightvt == VT_I8 || rightvt == VT_UI8) && !has_i8)
                continue;

            if (leftvt == VT_NULL && rightvt == VT_NULL)
                resultvt = VT_NULL;
            else if (leftvt == VT_VARIANT && (rightvt == VT_ERROR ||
                rightvt == VT_DATE || rightvt == VT_DECIMAL))
                expected_error_num = DISP_E_TYPEMISMATCH;
            else if ((leftvt == VT_I2 || leftvt == VT_I4 ||
                leftvt == VT_R4 || leftvt == VT_R8 ||
                leftvt == VT_CY || leftvt == VT_BOOL ||
                leftvt == VT_BSTR || leftvt == VT_I1 ||
                leftvt == VT_UI1 || leftvt == VT_UI2 ||
                leftvt == VT_UI4 || leftvt == VT_I8 ||
                leftvt == VT_UI8 || leftvt == VT_INT ||
                leftvt == VT_UINT || leftvt == VT_EMPTY ||
                leftvt == VT_NULL || leftvt == VT_DECIMAL ||
                leftvt == VT_DATE)
                &&
                (rightvt == VT_I2 || rightvt == VT_I4 ||
                rightvt == VT_R4 || rightvt == VT_R8 ||
                rightvt == VT_CY || rightvt == VT_BOOL ||
                rightvt == VT_BSTR || rightvt == VT_I1 ||
                rightvt == VT_UI1 || rightvt == VT_UI2 ||
                rightvt == VT_UI4 || rightvt == VT_I8 ||
                rightvt == VT_UI8 || rightvt == VT_INT ||
                rightvt == VT_UINT || rightvt == VT_EMPTY ||
                rightvt == VT_NULL || rightvt == VT_DECIMAL ||
                rightvt == VT_DATE))
                resultvt = VT_BSTR;
            else if (rightvt == VT_ERROR && leftvt < VT_VOID)
                expected_error_num = DISP_E_TYPEMISMATCH;
            else if (leftvt == VT_ERROR && (rightvt == VT_DATE ||
                rightvt == VT_ERROR || rightvt == VT_DECIMAL))
                expected_error_num = DISP_E_TYPEMISMATCH;
            else if (rightvt == VT_DATE || rightvt == VT_ERROR ||
                rightvt == VT_DECIMAL)
                expected_error_num = DISP_E_BADVARTYPE;
            else if (leftvt == VT_ERROR || rightvt == VT_ERROR)
                expected_error_num = DISP_E_TYPEMISMATCH;
            else if (leftvt == VT_VARIANT)
                expected_error_num = DISP_E_TYPEMISMATCH;
            else if (rightvt == VT_VARIANT && (leftvt == VT_EMPTY ||
                leftvt == VT_NULL || leftvt ==  VT_I2 ||
                leftvt == VT_I4 || leftvt == VT_R4 ||
                leftvt == VT_R8 || leftvt == VT_CY ||
                leftvt == VT_DATE || leftvt == VT_BSTR ||
                leftvt == VT_BOOL ||  leftvt == VT_DECIMAL ||
                leftvt == VT_I1 || leftvt == VT_UI1 ||
                leftvt == VT_UI2 || leftvt == VT_UI4 ||
                leftvt == VT_I8 || leftvt == VT_UI8 ||
                leftvt == VT_INT || leftvt == VT_UINT
                ))
                expected_error_num = DISP_E_TYPEMISMATCH;
            else
                expected_error_num = DISP_E_BADVARTYPE;

            V_VT(&left) = leftvt;
            V_VT(&right) = rightvt;

            switch (leftvt) {
            case VT_BSTR:
                V_BSTR(&left) = SysAllocString(L""); break;
            case VT_DATE:
                V_DATE(&left) = 0.0; break;
            case VT_DECIMAL:
                VarDecFromR8(0.0, &V_DECIMAL(&left)); break;
            default:
                V_I8(&left) = 0;
            }

            switch (rightvt) {
            case VT_BSTR:
                V_BSTR(&right) = SysAllocString(L""); break;
            case VT_DATE:
                V_DATE(&right) = 0.0; break;
            case VT_DECIMAL:
                VarDecFromR8(0.0, &V_DECIMAL(&right)); break;
            default:
                V_I8(&right) = 0;
            }

            hres = VarCat(&left, &right, &result);

            /* Determine the error code for the vt combination */
            ok(hres == expected_error_num,
                "VarCat: %d, %d returned error, 0x%lX expected 0x%lX.\n",
                leftvt, rightvt, hres, expected_error_num);

            /* Check types are correct */
            ok(V_VT(&result) == resultvt,
                "VarCat: %d, %d: expected vt %d, got vt %d\n",
                leftvt, rightvt, resultvt, V_VT(&result));

            VariantClear(&left);
            VariantClear(&right);
            VariantClear(&result);
        }
    }

    /* Running single comparison tests to compare outputs */

    /* Test concat strings */
    V_VT(&left) = VT_BSTR;
    V_VT(&right) = VT_BSTR;
    V_BSTR(&left) = SysAllocString(L"12");
    V_BSTR(&right) = SysAllocString(L"34");
    hres = VarCat(&left,&right,&result);
    ok(hres == S_OK, "VarCat failed with error 0x%08lx\n", hres);
    ok(V_VT(&result) == VT_BSTR, "Unexpected return type %d.\n", V_VT(&result));
    ok(!wcscmp(V_BSTR(&result), L"1234"), "Unexpected return value %s.\n", wine_dbgstr_w(V_BSTR(&result)));

    VariantClear(&left);
    VariantClear(&right);
    VariantClear(&result);

    /* Test if expression is VT_ERROR */
    V_VT(&left) = VT_ERROR;
    V_VT(&right) = VT_BSTR;
    V_BSTR(&right) = SysAllocString(L"1234");
    hres = VarCat(&left,&right,&result);
    ok(hres == DISP_E_TYPEMISMATCH, "VarCat should have returned DISP_E_TYPEMISMATCH instead of 0x%08lx\n", hres);
    ok(V_VT(&result) == VT_EMPTY,
        "VarCat: VT_ERROR concat with VT_BSTR should have returned VT_EMPTY\n");

    VariantClear(&left);
    VariantClear(&right);
    VariantClear(&result);

    V_VT(&left) = VT_BSTR;
    V_VT(&right) = VT_ERROR;
    V_BSTR(&left) = SysAllocString(L"1234");
    hres = VarCat(&left,&right,&result);
    ok(hres == DISP_E_TYPEMISMATCH, "VarCat should have returned DISP_E_TYPEMISMATCH instead of 0x%08lx\n", hres);
    ok(V_VT(&result) == VT_EMPTY,
        "VarCat: VT_BSTR concat with VT_ERROR should have returned VT_EMPTY\n");

    VariantClear(&left);
    VariantClear(&right);
    VariantClear(&result);

    /* Test combining boolean with number */
    V_VT(&left) = VT_INT;
    V_VT(&right) = VT_BOOL;
    V_INT(&left) = 12;
    V_BOOL(&right) = TRUE;
    hres = VarCat(&left,&right,&result);
    ok(hres == S_OK, "VarCat failed with error 0x%08lx\n", hres);
    ok(V_VT(&result) == VT_BSTR, "Unexpected return type %d.\n", V_VT(&result));
    ok(!wcscmp(V_BSTR(&result), sz12_true), "Unexpected return value %s.\n", wine_dbgstr_w(V_BSTR(&result)));

    VariantClear(&left);
    VariantClear(&right);
    VariantClear(&result);

    V_VT(&left) = VT_INT;
    V_VT(&right) = VT_BOOL;
    V_INT(&left) = 12;
    V_BOOL(&right) = FALSE;
    hres = VarCat(&left,&right,&result);
    ok(hres == S_OK, "VarCat failed with error 0x%08lx\n", hres);
    ok(V_VT(&result) == VT_BSTR, "Unexpected return type %d.\n", V_VT(&result));
    ok(!wcscmp(V_BSTR(&result), sz12_false), "Unexpected return value %s.\n", wine_dbgstr_w(V_BSTR(&result)));

    VariantClear(&left);
    VariantClear(&right);
    VariantClear(&result);

    /* Test when both expressions are numeric */
    V_VT(&left) = VT_INT;
    V_VT(&right) = VT_INT;
    V_INT(&left)  = 12;
    V_INT(&right) = 34;
    hres = VarCat(&left,&right,&result);
    ok(hres == S_OK, "VarCat failed with error 0x%08lx\n", hres);
    ok(V_VT(&result) == VT_BSTR, "Unexpected return type %d.\n", V_VT(&result));
    ok(!wcscmp(V_BSTR(&result), L"1234"), "Unexpected return value %s.\n", wine_dbgstr_w(V_BSTR(&result)));

    VariantClear(&left);
    VariantClear(&right);
    VariantClear(&result);

    /* Test if one expression is numeric and the other is a string */
    V_VT(&left) = VT_INT;
    V_VT(&right) = VT_BSTR;
    V_INT(&left) = 12;
    V_BSTR(&right) = SysAllocString(L"34");
    hres = VarCat(&left,&right,&result);
    ok(hres == S_OK, "VarCat failed with error 0x%08lx\n", hres);
    ok(V_VT(&result) == VT_BSTR, "Unexpected return type %d.\n", V_VT(&result));
    ok(!wcscmp(V_BSTR(&result), L"1234"), "Unexpected return value %s.\n", wine_dbgstr_w(V_BSTR(&result)));

    VariantClear(&left);
    VariantClear(&right);
    VariantClear(&result);

    V_VT(&left) = VT_BSTR;
    V_VT(&right) = VT_INT;
    V_BSTR(&left) = SysAllocString(L"12");
    V_INT(&right) = 34;
    hres = VarCat(&left,&right,&result);
    ok(hres == S_OK, "VarCat failed with error 0x%08lx\n", hres);
    ok(V_VT(&result) == VT_BSTR, "Unexpected return type %d.\n", V_VT(&result));
    ok(!wcscmp(V_BSTR(&result), L"1234"), "Unexpected return value %s.\n", wine_dbgstr_w(V_BSTR(&result)));

    VariantClear(&left);
    VariantClear(&right);
    VariantClear(&result);

    /* Test concat dates with strings */
    V_VT(&left) = VT_BSTR;
    V_VT(&right) = VT_DATE;
    V_BSTR(&left) = SysAllocString(L"12");
    V_DATE(&right) = 29494.0;
    hres = VarCat(&left,&right,&result);
    ok(hres == S_OK, "VarCat failed with error 0x%08lx\n", hres);
    ok(V_VT(&result) == VT_BSTR, "Unexpected return type %d.\n", V_VT(&result));
    ok(!wcscmp(V_BSTR(&result), L"129/30/1980"), "Unexpected return value %s.\n", wine_dbgstr_w(V_BSTR(&result)));

    VariantClear(&left);
    VariantClear(&right);
    VariantClear(&result);

    V_VT(&left) = VT_DATE;
    V_VT(&right) = VT_BSTR;
    V_DATE(&left) = 29494.0;
    V_BSTR(&right) = SysAllocString(L"12");
    hres = VarCat(&left,&right,&result);
    ok(hres == S_OK, "VarCat failed with error 0x%08lx\n", hres);
    ok(V_VT(&result) == VT_BSTR, "Unexpected return type %d.\n", V_VT(&result));
    ok(!wcscmp(V_BSTR(&result), L"9/30/198012"), "Unexpected return value %s.\n", wine_dbgstr_w(V_BSTR(&result)));

    VariantClear(&left);
    VariantClear(&right);
    VariantClear(&result);

    /* Test of both expressions are empty */
    V_VT(&left) = VT_BSTR;
    V_VT(&right) = VT_BSTR;
    V_BSTR(&left) = SysAllocString(L"");
    V_BSTR(&right) = SysAllocString(L"");
    hres = VarCat(&left,&right,&result);
    ok(hres == S_OK, "VarCat failed with error 0x%08lx\n", hres);
    ok(V_VT(&result) == VT_BSTR, "Unexpected return type %d.\n", V_VT(&result));
    ok(!wcscmp(V_BSTR(&result), L""), "Unexpected return value %s.\n", wine_dbgstr_w(V_BSTR(&result)));

    /* Restore original date format settings */
    SetLocaleInfoA(lcid,LOCALE_SSHORTDATE,orig_date_format);

    VariantClear(&left);
    VariantClear(&right);
    VariantClear(&result);

    /* Dispatch conversion */
    init_test_dispatch(VT_NULL, &dispatch);
    V_VT(&left) = VT_DISPATCH;
    V_DISPATCH(&left) = &dispatch.IDispatch_iface;

    SET_EXPECT(dispatch_invoke);
    hres = VarCat(&left, &right, &result);
    ok(hres == S_OK, "got 0x%08lx\n", hres);
    ok(V_VT(&result) == VT_BSTR, "got %d\n", V_VT(&result));
    ok(SysStringLen(V_BSTR(&result)) == 0, "got %d\n", SysStringLen(V_BSTR(&result)));
    CHECK_CALLED(dispatch_invoke);

    VariantClear(&left);
    VariantClear(&right);
    VariantClear(&result);

    init_test_dispatch(VT_NULL, &dispatch);
    V_VT(&right) = VT_DISPATCH;
    V_DISPATCH(&right) = &dispatch.IDispatch_iface;

    SET_EXPECT(dispatch_invoke);
    hres = VarCat(&left, &right, &result);
    ok(hres == S_OK, "got 0x%08lx\n", hres);
    ok(V_VT(&result) == VT_BSTR, "got %d\n", V_VT(&result));
    ok(SysStringLen(V_BSTR(&result)) == 0, "got %d\n", SysStringLen(V_BSTR(&result)));
    CHECK_CALLED(dispatch_invoke);

    VariantClear(&left);
    VariantClear(&right);
    VariantClear(&result);

    init_test_dispatch(VT_UI1, &dispatch);
    V_VT(&right) = VT_DISPATCH;
    V_DISPATCH(&right) = &dispatch.IDispatch_iface;

    V_VT(&left) = VT_BSTR;
    V_BSTR(&left) = SysAllocString(L"12");
    SET_EXPECT(dispatch_invoke);
    hres = VarCat(&left,&right,&result);
    ok(hres == S_OK, "VarCat failed with error 0x%08lx\n", hres);
    CHECK_CALLED(dispatch_invoke);
    ok(!lstrcmpW(V_BSTR(&result), L"1234"), "got %s\n", wine_dbgstr_w(V_BSTR(&result)));

    VariantClear(&left);
    VariantClear(&right);
    VariantClear(&result);

    init_test_dispatch(VT_NULL, &dispatch);
    dispatch.result = E_OUTOFMEMORY;
    V_VT(&right) = VT_DISPATCH;
    V_DISPATCH(&right) = &dispatch.IDispatch_iface;

    SET_EXPECT(dispatch_invoke);
    hres = VarCat(&left, &right, &result);
    ok(hres == E_OUTOFMEMORY, "got 0x%08lx\n", hres);
    CHECK_CALLED(dispatch_invoke);

    VariantClear(&left);
    VariantClear(&right);
    VariantClear(&result);

    init_test_dispatch(VT_NULL, &dispatch);
    dispatch.result = DISP_E_TYPEMISMATCH;
    V_VT(&right) = VT_DISPATCH;
    V_DISPATCH(&right) = &dispatch.IDispatch_iface;

    SET_EXPECT(dispatch_invoke);
    hres = VarCat(&left, &right, &result);
    ok(hres == DISP_E_TYPEMISMATCH, "got 0x%08lx\n", hres);
    CHECK_CALLED(dispatch_invoke);

    VariantClear(&left);
    VariantClear(&right);
    VariantClear(&result);

    /* Test boolean conversion */
    V_VT(&left) = VT_BOOL;
    V_BOOL(&left) = VARIANT_TRUE;
    V_VT(&right) = VT_BSTR;
    V_BSTR(&right) = SysAllocStringLen(NULL,0);
    hres = VarCat(&left, &right, &result);
    ok(hres == S_OK, "VarCat failed: %08lx\n", hres);
    VariantClear(&right);

    cmp = lstrcmpW(V_BSTR(&result), L"True");
    VariantClear(&result);
    if(!cmp) {
        V_VT(&right) = VT_BOOL;
        V_BOOL(&right) = 100;
        hres = VarCat(&left, &right, &result);
        ok(hres == S_OK, "VarCat failed: %08lx\n", hres);
        test_bstr_var(&result, L"TrueTrue");
        VariantClear(&result);

        V_BOOL(&right) = VARIANT_FALSE;
        hres = VarCat(&left, &right, &result);
        ok(hres == S_OK, "VarCat failed: %08lx\n", hres);
        test_bstr_var(&result, L"TrueFalse");
        VariantClear(&result);
    }else {
        skip("Got %s as True, assuming non-English locale\n", wine_dbgstr_w(V_BSTR(&result)));
    }
}

static HRESULT (WINAPI *pVarAnd)(LPVARIANT,LPVARIANT,LPVARIANT);

#define VARAND(vt1,val1,vt2,val2,rvt,rval)               \
        V_VT(&left) = VT_##vt1; V_##vt1(&left) = val1;   \
        V_VT(&right) = VT_##vt2; V_##vt2(&right) = val2; \
        V_VT(&exp) = VT_##rvt; V_##rvt(&exp) = rval;     \
        test_var_call2( __LINE__, pVarAnd, &left, &right, &exp )

#define VARANDCY(vt1,val1,val2,rvt,rval)                 \
        V_VT(&left) = VT_##vt1; V_##vt1(&left) = val1;   \
        V_VT(&right) = VT_CY; V_CY(&right).int64 = val2; \
        V_VT(&exp) = VT_##rvt; V_##rvt(&exp) = rval;     \
        test_var_call2( __LINE__, pVarAnd, &left, &right, &exp )

/* Skip any type that is not defined or produces an error for every case */
#define SKIPTESTAND(a)                                \
        if (a == VT_ERROR || a == VT_VARIANT ||       \
            a == VT_DISPATCH || a == VT_UNKNOWN ||    \
            a > VT_UINT || a == 15 /*not defined*/)   \
            continue

static void test_VarAnd(void)
{
    static const WCHAR szFalse[] = { '#','F','A','L','S','E','#','\0' };
    static const WCHAR szTrue[] = { '#','T','R','U','E','#','\0' };
    VARIANT left, right, exp, result;
    BSTR false_str, true_str;
    VARTYPE i;
    HRESULT hres;

    CHECKPTR(VarAnd);

    true_str = SysAllocString(szTrue);
    false_str = SysAllocString(szFalse);

    /* Test all possible flag/vt combinations & the resulting vt type */
    for (i = 0; i < ARRAY_SIZE(ExtraFlags); i++)
    {
        VARTYPE leftvt, rightvt, resvt;

        for (leftvt = 0; leftvt <= VT_BSTR_BLOB; leftvt++)
        {
            SKIPTESTAND(leftvt);

            /* Check if we need/have support for I8 and/or UI8 */
            if ((leftvt == VT_I8 || leftvt == VT_UI8) && !has_i8)
                continue;

            for (rightvt = 0; rightvt <= VT_BSTR_BLOB; rightvt++)
            {
                BOOL bFail = FALSE;
                SKIPTESTAND(rightvt);

                /* Check if we need/have support for I8 and/or UI8 */
                if ((rightvt == VT_I8 || rightvt == VT_UI8) && !has_i8)
                    continue;

                memset(&left, 0, sizeof(left));
                memset(&right, 0, sizeof(right));
                V_VT(&left) = leftvt | ExtraFlags[i];
                V_VT(&right) = rightvt | ExtraFlags[i];
                V_VT(&result) = VT_EMPTY;
                resvt = VT_EMPTY;
                if ((leftvt | ExtraFlags[i]) == VT_BSTR)
                    V_BSTR(&left) = true_str;
                if ((rightvt | ExtraFlags[i]) == VT_BSTR)
                    V_BSTR(&right) = true_str;

                /* Native VarAnd always returns an error when using extra
                 * flags or if the variant combination is I8 and INT.
                 */
                if ((leftvt == VT_I8 && rightvt == VT_INT) ||
                    (leftvt == VT_INT && rightvt == VT_I8) ||
                    ExtraFlags[i] != 0)
                    bFail = TRUE;

                /* Determine return type */
                else if (leftvt == VT_I8 || rightvt == VT_I8)
                    resvt = VT_I8;
                else if (leftvt == VT_I4 || rightvt == VT_I4 ||
                    leftvt == VT_UINT || rightvt == VT_UINT ||
                    leftvt == VT_INT || rightvt == VT_INT ||
                    leftvt == VT_R4 || rightvt == VT_R4 ||
                    leftvt == VT_R8 || rightvt == VT_R8 ||
                    leftvt == VT_CY || rightvt == VT_CY ||
                    leftvt == VT_DATE || rightvt == VT_DATE ||
                    leftvt == VT_I1 || rightvt == VT_I1 ||
                    leftvt == VT_UI2 || rightvt == VT_UI2 ||
                    leftvt == VT_UI4 || rightvt == VT_UI4 ||
                    leftvt == VT_UI8 || rightvt == VT_UI8 ||
                    leftvt == VT_DECIMAL || rightvt == VT_DECIMAL)
                    resvt = VT_I4;
                else if (leftvt == VT_UI1 || rightvt == VT_UI1 ||
                    leftvt == VT_I2 || rightvt == VT_I2 ||
                    leftvt == VT_EMPTY || rightvt == VT_EMPTY)
                    if ((leftvt == VT_NULL && rightvt == VT_UI1) ||
                        (leftvt == VT_UI1 && rightvt == VT_NULL) ||
                        (leftvt == VT_UI1 && rightvt == VT_UI1))
                        resvt = VT_UI1;
                    else
                        resvt = VT_I2;
                else if (leftvt == VT_BOOL || rightvt == VT_BOOL ||
                    (leftvt == VT_BSTR && rightvt == VT_BSTR))
                    resvt = VT_BOOL;
                else if (leftvt == VT_NULL || rightvt == VT_NULL ||
                    leftvt == VT_BSTR || rightvt == VT_BSTR)
                    resvt = VT_NULL;
                else
                    bFail = TRUE;

                hres = pVarAnd(&left, &right, &result);

                /* Check expected HRESULT and if result variant type is correct */
                if (bFail)
                    ok (hres == DISP_E_BADVARTYPE || hres == DISP_E_TYPEMISMATCH,
                        "VarAnd: %s|0x%X, %s|0x%X: got vt %s hr 0x%lX\n",
                        wine_dbgstr_vt(leftvt), ExtraFlags[i], wine_dbgstr_vt(rightvt), ExtraFlags[i],
                        wine_dbgstr_vt(V_VT(&result)), hres);
                else
                    ok (hres == S_OK && resvt == V_VT(&result),
                        "VarAnd: %s|0x%X, %s|0x%X: expected vt %s hr 0x%lX, got vt %s hr 0x%lX\n",
                        wine_dbgstr_vt(leftvt), ExtraFlags[i], wine_dbgstr_vt(rightvt), ExtraFlags[i], wine_dbgstr_vt(resvt),
                        S_OK, wine_dbgstr_vt(V_VT(&result)), hres);
            }
        }
    }

    /*
     * Test returned values. Since we know the returned type is correct
     * and that we handle all combinations of invalid types, just check
     * that good type combinations produce the desired value.
     * FIXME: Test VT_DECIMAL
     */
    VARAND(EMPTY,0,EMPTY,0,I2,0);
    VARAND(EMPTY,1,EMPTY,0,I2,0);
    VARAND(EMPTY,1,EMPTY,1,I2,0);
    VARAND(EMPTY,0,NULL,0,I2,0);
    VARAND(EMPTY,1,NULL,0,I2,0);
    VARAND(EMPTY,1,NULL,1,I2,0);
    VARAND(EMPTY,0,I1,0,I4,0);
    VARAND(EMPTY,0,I1,1,I4,0);
    VARAND(EMPTY,1,I1,1,I4,0);
    VARAND(EMPTY,0,UI1,0,I2,0);
    VARAND(EMPTY,0,UI1,1,I2,0);
    VARAND(EMPTY,1,UI1,1,I2,0);
    VARAND(EMPTY,0,I2,0,I2,0);
    VARAND(EMPTY,0,I2,1,I2,0);
    VARAND(EMPTY,1,I2,1,I2,0);
    VARAND(EMPTY,0,UI2,0,I4,0);
    VARAND(EMPTY,0,UI2,1,I4,0);
    VARAND(EMPTY,1,UI2,1,I4,0);
    VARAND(EMPTY,0,I4,0,I4,0);
    VARAND(EMPTY,0,I4,1,I4,0);
    VARAND(EMPTY,1,I4,1,I4,0);
    VARAND(EMPTY,0,UI4,0,I4,0);
    VARAND(EMPTY,0,UI4,1,I4,0);
    VARAND(EMPTY,1,UI4,1,I4,0);
    if (has_i8)
    {
        VARAND(EMPTY,0,I8,0,I8,0);
        VARAND(EMPTY,0,I8,1,I8,0);
        VARAND(EMPTY,1,I8,1,I8,0);
        VARAND(EMPTY,0,UI8,0,I4,0);
        VARAND(EMPTY,0,UI8,1,I4,0);
        VARAND(EMPTY,1,UI8,1,I4,0);
    }
    VARAND(EMPTY,0,INT,0,I4,0);
    VARAND(EMPTY,0,INT,1,I4,0);
    VARAND(EMPTY,1,INT,1,I4,0);
    VARAND(EMPTY,0,UINT,0,I4,0);
    VARAND(EMPTY,0,UINT,1,I4,0);
    VARAND(EMPTY,1,UINT,1,I4,0);
    VARAND(EMPTY,0,BOOL,0,I2,0);
    VARAND(EMPTY,0,BOOL,1,I2,0);
    VARAND(EMPTY,1,BOOL,1,I2,0);
    VARAND(EMPTY,0,R4,0,I4,0);
    VARAND(EMPTY,0,R4,1,I4,0);
    VARAND(EMPTY,1,R4,1,I4,0);
    VARAND(EMPTY,0,R8,0,I4,0);
    VARAND(EMPTY,0,R8,1,I4,0);
    VARAND(EMPTY,1,R8,1,I4,0);
    VARAND(EMPTY,0,BSTR,false_str,I2,0);
    VARAND(EMPTY,0,BSTR,true_str,I2,0);
    VARANDCY(EMPTY,0,10000,I4,0);

    /* NULL OR 0 = NULL. NULL OR n = n */
    VARAND(NULL,0,NULL,0,NULL,0);
    VARAND(NULL,1,NULL,0,NULL,0);
    VARAND(NULL,0,I1,0,I4,0);
    VARAND(NULL,0,I1,1,NULL,0);
    VARAND(NULL,0,UI1,0,UI1,0);
    VARAND(NULL,0,UI1,1,NULL,0);
    VARAND(NULL,0,I2,0,I2,0);
    VARAND(NULL,0,I2,1,NULL,0);
    VARAND(NULL,0,UI2,0,I4,0);
    VARAND(NULL,0,UI2,1,NULL,0);
    VARAND(NULL,0,I4,0,I4,0);
    VARAND(NULL,0,I4,1,NULL,0);
    VARAND(NULL,0,UI4,0,I4,0);
    VARAND(NULL,0,UI4,1,NULL,0);
    if (has_i8)
    {
        VARAND(NULL,0,I8,0,I8,0);
        VARAND(NULL,0,I8,1,NULL,0);
        VARAND(NULL,0,UI8,0,I4,0);
        VARAND(NULL,0,UI8,1,NULL,0);
    }
    VARAND(NULL,0,INT,0,I4,0);
    VARAND(NULL,0,INT,1,NULL,0);
    VARAND(NULL,0,UINT,0,I4,0);
    VARAND(NULL,0,UINT,1,NULL,0);
    VARAND(NULL,0,BOOL,0,BOOL,0);
    VARAND(NULL,0,BOOL,1,NULL,0);
    VARAND(NULL,0,R4,0,I4,0);
    VARAND(NULL,0,R4,1,NULL,0);
    VARAND(NULL,0,R8,0,I4,0);
    VARAND(NULL,0,R8,1,NULL,0);
    VARAND(NULL,0,BSTR,false_str,BOOL,0);
    VARAND(NULL,0,BSTR,true_str,NULL,VARIANT_FALSE);
    VARANDCY(NULL,0,10000,NULL,0);
    VARANDCY(NULL,0,0,I4,0);
    VARAND(BOOL,VARIANT_TRUE,BOOL,VARIANT_TRUE,BOOL,VARIANT_TRUE);
    VARAND(BOOL,VARIANT_TRUE,BOOL,VARIANT_FALSE,BOOL,VARIANT_FALSE);
    VARAND(BOOL,VARIANT_FALSE,BOOL,VARIANT_TRUE,BOOL,VARIANT_FALSE);
    VARAND(BOOL,VARIANT_FALSE,BOOL,VARIANT_FALSE,BOOL,VARIANT_FALSE);

    /* Assume x,y & y,x are the same from now on to reduce the number of tests */
    VARAND(BOOL,VARIANT_TRUE,I1,-1,I4,-1);
    VARAND(BOOL,VARIANT_TRUE,I1,0,I4,0);
    VARAND(BOOL,VARIANT_FALSE,I1,0,I4,0);
    VARAND(BOOL,VARIANT_TRUE,UI1,255,I2,255);
    VARAND(BOOL,VARIANT_TRUE,UI1,0,I2,0);
    VARAND(BOOL,VARIANT_FALSE,UI1,0,I2,0);
    VARAND(BOOL,VARIANT_TRUE,I2,-1,I2,-1);
    VARAND(BOOL,VARIANT_TRUE,I2,0,I2,0);
    VARAND(BOOL,VARIANT_FALSE,I2,0,I2,0);
    VARAND(BOOL,VARIANT_TRUE,UI2,65535,I4,65535);
    VARAND(BOOL,VARIANT_TRUE,UI2,0,I4,0);
    VARAND(BOOL,VARIANT_FALSE,UI2,0,I4,0);
    VARAND(BOOL,VARIANT_TRUE,I4,-1,I4,-1);
    VARAND(BOOL,VARIANT_TRUE,I4,0,I4,0);
    VARAND(BOOL,VARIANT_FALSE,I4,0,I4,0);
    VARAND(BOOL,VARIANT_TRUE,UI4,0xffffffff,I4,-1);
    VARAND(BOOL,VARIANT_TRUE,UI4,0,I4,0);
    VARAND(BOOL,VARIANT_FALSE,UI4,0,I4,0);
    VARAND(BOOL,VARIANT_TRUE,R4,-1,I4,-1);
    VARAND(BOOL,VARIANT_TRUE,R4,0,I4,0);
    VARAND(BOOL,VARIANT_FALSE,R4,0,I4,0);
    VARAND(BOOL,VARIANT_TRUE,R8,-1,I4,-1);
    VARAND(BOOL,VARIANT_TRUE,R8,0,I4,0);
    VARAND(BOOL,VARIANT_FALSE,R8,0,I4,0);
    VARAND(BOOL,VARIANT_TRUE,DATE,-1,I4,-1);
    VARAND(BOOL,VARIANT_TRUE,DATE,0,I4,0);
    VARAND(BOOL,VARIANT_FALSE,DATE,0,I4,0);
    if (has_i8)
    {
        VARAND(BOOL,VARIANT_TRUE,I8,-1,I8,-1);
        VARAND(BOOL,VARIANT_TRUE,I8,0,I8,0);
        VARAND(BOOL,VARIANT_FALSE,I8,0,I8,0);
        VARAND(BOOL,VARIANT_TRUE,UI8,0,I4,0);
        VARAND(BOOL,VARIANT_FALSE,UI8,0,I4,0);
    }
    VARAND(BOOL,VARIANT_TRUE,INT,-1,I4,-1);
    VARAND(BOOL,VARIANT_TRUE,INT,0,I4,0);
    VARAND(BOOL,VARIANT_FALSE,INT,0,I4,0);
    VARAND(BOOL,VARIANT_TRUE,UINT,0xffffffff,I4,-1);
    VARAND(BOOL,VARIANT_TRUE,UINT,0,I4,0);
    VARAND(BOOL,VARIANT_FALSE,UINT,0,I4,0);
    VARAND(BOOL,VARIANT_FALSE,BSTR,false_str,BOOL,VARIANT_FALSE);
    VARAND(BOOL,VARIANT_TRUE,BSTR,false_str,BOOL,VARIANT_FALSE);
    VARAND(BOOL,VARIANT_FALSE,BSTR,true_str,BOOL,VARIANT_FALSE);
    VARAND(BOOL,VARIANT_TRUE,BSTR,true_str,BOOL,VARIANT_TRUE);
    VARANDCY(BOOL,VARIANT_TRUE,10000,I4,1);
    VARANDCY(BOOL,VARIANT_TRUE,0,I4,0);
    VARANDCY(BOOL,VARIANT_FALSE,0,I4,0);
    VARAND(I1,-1,I1,-1,I4,-1);
    VARAND(I1,-1,I1,0,I4,0);
    VARAND(I1,0,I1,0,I4,0);
    VARAND(I1,-1,UI1,255,I4,255);
    VARAND(I1,-1,UI1,0,I4,0);
    VARAND(I1,0,UI1,0,I4,0);
    VARAND(I1,-1,I2,-1,I4,-1);
    VARAND(I1,-1,I2,0,I4,0);
    VARAND(I1,0,I2,0,I4,0);
    VARAND(I1,-1,UI2,65535,I4,65535);
    VARAND(I1,-1,UI2,0,I4,0);
    VARAND(I1,0,UI2,0,I4,0);
    VARAND(I1,-1,I4,-1,I4,-1);
    VARAND(I1,-1,I4,0,I4,0);
    VARAND(I1,0,I4,0,I4,0);
    VARAND(I1,-1,UI4,0xffffffff,I4,-1);
    VARAND(I1,-1,UI4,0,I4,0);
    VARAND(I1,0,UI4,0,I4,0);
    VARAND(I1,-1,R4,-1,I4,-1);
    VARAND(I1,-1,R4,0,I4,0);
    VARAND(I1,0,R4,0,I4,0);
    VARAND(I1,-1,R8,-1,I4,-1);
    VARAND(I1,-1,R8,0,I4,0);
    VARAND(I1,0,R8,0,I4,0);
    VARAND(I1,-1,DATE,-1,I4,-1);
    VARAND(I1,-1,DATE,0,I4,0);
    VARAND(I1,0,DATE,0,I4,0);
    if (has_i8)
    {
        VARAND(I1,-1,I8,-1,I8,-1);
        VARAND(I1,-1,I8,0,I8,0);
        VARAND(I1,0,I8,0,I8,0);
        VARAND(I1,-1,UI8,0,I4,0);
        VARAND(I1,0,UI8,0,I4,0);
    }
    VARAND(I1,-1,INT,-1,I4,-1);
    VARAND(I1,-1,INT,0,I4,0);
    VARAND(I1,0,INT,0,I4,0);
    VARAND(I1,-1,UINT,0xffffffff,I4,-1);
    VARAND(I1,-1,UINT,0,I4,0);
    VARAND(I1,0,UINT,0,I4,0);
    VARAND(I1,0,BSTR,false_str,I4,0);
    VARAND(I1,-1,BSTR,false_str,I4,0);
    VARAND(I1,0,BSTR,true_str,I4,0);
    VARAND(I1,-1,BSTR,true_str,I4,-1);
    VARANDCY(I1,-1,10000,I4,1);
    VARANDCY(I1,-1,0,I4,0);
    VARANDCY(I1,0,0,I4,0);

    VARAND(UI1,255,UI1,255,UI1,255);
    VARAND(UI1,255,UI1,0,UI1,0);
    VARAND(UI1,0,UI1,0,UI1,0);
    VARAND(UI1,255,I2,-1,I2,255);
    VARAND(UI1,255,I2,0,I2,0);
    VARAND(UI1,0,I2,0,I2,0);
    VARAND(UI1,255,UI2,65535,I4,255);
    VARAND(UI1,255,UI2,0,I4,0);
    VARAND(UI1,0,UI2,0,I4,0);
    VARAND(UI1,255,I4,-1,I4,255);
    VARAND(UI1,255,I4,0,I4,0);
    VARAND(UI1,0,I4,0,I4,0);
    VARAND(UI1,255,UI4,0xffffffff,I4,255);
    VARAND(UI1,255,UI4,0,I4,0);
    VARAND(UI1,0,UI4,0,I4,0);
    VARAND(UI1,255,R4,-1,I4,255);
    VARAND(UI1,255,R4,0,I4,0);
    VARAND(UI1,0,R4,0,I4,0);
    VARAND(UI1,255,R8,-1,I4,255);
    VARAND(UI1,255,R8,0,I4,0);
    VARAND(UI1,0,R8,0,I4,0);
    VARAND(UI1,255,DATE,-1,I4,255);
    VARAND(UI1,255,DATE,0,I4,0);
    VARAND(UI1,0,DATE,0,I4,0);
    if (has_i8)
    {
        VARAND(UI1,255,I8,-1,I8,255);
        VARAND(UI1,255,I8,0,I8,0);
        VARAND(UI1,0,I8,0,I8,0);
        VARAND(UI1,255,UI8,0,I4,0);
        VARAND(UI1,0,UI8,0,I4,0);
    }
    VARAND(UI1,255,INT,-1,I4,255);
    VARAND(UI1,255,INT,0,I4,0);
    VARAND(UI1,0,INT,0,I4,0);
    VARAND(UI1,255,UINT,0xffffffff,I4,255);
    VARAND(UI1,255,UINT,0,I4,0);
    VARAND(UI1,0,UINT,0,I4,0);
    VARAND(UI1,0,BSTR,false_str,I2,0);
    VARAND(UI1,255,BSTR,false_str,I2,0);
    VARAND(UI1,0,BSTR,true_str,I2,0);
    VARAND(UI1,255,BSTR,true_str,I2,255);
    VARANDCY(UI1,255,10000,I4,1);
    VARANDCY(UI1,255,0,I4,0);
    VARANDCY(UI1,0,0,I4,0);

    VARAND(I2,-1,I2,-1,I2,-1);
    VARAND(I2,-1,I2,0,I2,0);
    VARAND(I2,0,I2,0,I2,0);
    VARAND(I2,-1,UI2,65535,I4,65535);
    VARAND(I2,-1,UI2,0,I4,0);
    VARAND(I2,0,UI2,0,I4,0);
    VARAND(I2,-1,I4,-1,I4,-1);
    VARAND(I2,-1,I4,0,I4,0);
    VARAND(I2,0,I4,0,I4,0);
    VARAND(I2,-1,UI4,0xffffffff,I4,-1);
    VARAND(I2,-1,UI4,0,I4,0);
    VARAND(I2,0,UI4,0,I4,0);
    VARAND(I2,-1,R4,-1,I4,-1);
    VARAND(I2,-1,R4,0,I4,0);
    VARAND(I2,0,R4,0,I4,0);
    VARAND(I2,-1,R8,-1,I4,-1);
    VARAND(I2,-1,R8,0,I4,0);
    VARAND(I2,0,R8,0,I4,0);
    VARAND(I2,-1,DATE,-1,I4,-1);
    VARAND(I2,-1,DATE,0,I4,0);
    VARAND(I2,0,DATE,0,I4,0);
    if (has_i8)
    {
        VARAND(I2,-1,I8,-1,I8,-1);
        VARAND(I2,-1,I8,0,I8,0);
        VARAND(I2,0,I8,0,I8,0);
        VARAND(I2,-1,UI8,0,I4,0);
        VARAND(I2,0,UI8,0,I4,0);
    }
    VARAND(I2,-1,INT,-1,I4,-1);
    VARAND(I2,-1,INT,0,I4,0);
    VARAND(I2,0,INT,0,I4,0);
    VARAND(I2,-1,UINT,0xffffffff,I4,-1);
    VARAND(I2,-1,UINT,0,I4,0);
    VARAND(I2,0,UINT,0,I4,0);
    VARAND(I2,0,BSTR,false_str,I2,0);
    VARAND(I2,-1,BSTR,false_str,I2,0);
    VARAND(I2,0,BSTR,true_str,I2,0);
    VARAND(I2,-1,BSTR,true_str,I2,-1);
    VARANDCY(I2,-1,10000,I4,1);
    VARANDCY(I2,-1,0,I4,0);
    VARANDCY(I2,0,0,I4,0);

    VARAND(UI2,65535,UI2,65535,I4,65535);
    VARAND(UI2,65535,UI2,0,I4,0);
    VARAND(UI2,0,UI2,0,I4,0);
    VARAND(UI2,65535,I4,-1,I4,65535);
    VARAND(UI2,65535,I4,0,I4,0);
    VARAND(UI2,0,I4,0,I4,0);
    VARAND(UI2,65535,UI4,0xffffffff,I4,65535);
    VARAND(UI2,65535,UI4,0,I4,0);
    VARAND(UI2,0,UI4,0,I4,0);
    VARAND(UI2,65535,R4,-1,I4,65535);
    VARAND(UI2,65535,R4,0,I4,0);
    VARAND(UI2,0,R4,0,I4,0);
    VARAND(UI2,65535,R8,-1,I4,65535);
    VARAND(UI2,65535,R8,0,I4,0);
    VARAND(UI2,0,R8,0,I4,0);
    VARAND(UI2,65535,DATE,-1,I4,65535);
    VARAND(UI2,65535,DATE,0,I4,0);
    VARAND(UI2,0,DATE,0,I4,0);
    if (has_i8)
    {
        VARAND(UI2,65535,I8,-1,I8,65535);
        VARAND(UI2,65535,I8,0,I8,0);
        VARAND(UI2,0,I8,0,I8,0);
        VARAND(UI2,65535,UI8,0,I4,0);
        VARAND(UI2,0,UI8,0,I4,0);
    }
    VARAND(UI2,65535,INT,-1,I4,65535);
    VARAND(UI2,65535,INT,0,I4,0);
    VARAND(UI2,0,INT,0,I4,0);
    VARAND(UI2,65535,UINT,0xffffffff,I4,65535);
    VARAND(UI2,65535,UINT,0,I4,0);
    VARAND(UI2,0,UINT,0,I4,0);
    VARAND(UI2,0,BSTR,false_str,I4,0);
    VARAND(UI2,65535,BSTR,false_str,I4,0);
    VARAND(UI2,0,BSTR,true_str,I4,0);
    VARAND(UI2,65535,BSTR,true_str,I4,65535);
    VARANDCY(UI2,65535,10000,I4,1);
    VARANDCY(UI2,65535,0,I4,0);
    VARANDCY(UI2,0,0,I4,0);

    VARAND(I4,-1,I4,-1,I4,-1);
    VARAND(I4,-1,I4,0,I4,0);
    VARAND(I4,0,I4,0,I4,0);
    VARAND(I4,-1,UI4,0xffffffff,I4,-1);
    VARAND(I4,-1,UI4,0,I4,0);
    VARAND(I4,0,UI4,0,I4,0);
    VARAND(I4,-1,R4,-1,I4,-1);
    VARAND(I4,-1,R4,0,I4,0);
    VARAND(I4,0,R4,0,I4,0);
    VARAND(I4,-1,R8,-1,I4,-1);
    VARAND(I4,-1,R8,0,I4,0);
    VARAND(I4,0,R8,0,I4,0);
    VARAND(I4,-1,DATE,-1,I4,-1);
    VARAND(I4,-1,DATE,0,I4,0);
    VARAND(I4,0,DATE,0,I4,0);
    if (has_i8)
    {
        VARAND(I4,-1,I8,-1,I8,-1);
        VARAND(I4,-1,I8,0,I8,0);
        VARAND(I4,0,I8,0,I8,0);
        VARAND(I4,-1,UI8,0,I4,0);
        VARAND(I4,0,UI8,0,I4,0);
    }
    VARAND(I4,-1,INT,-1,I4,-1);
    VARAND(I4,-1,INT,0,I4,0);
    VARAND(I4,0,INT,0,I4,0);
    VARAND(I4,-1,UINT,0xffffffff,I4,-1);
    VARAND(I4,-1,UINT,0,I4,0);
    VARAND(I4,0,UINT,0,I4,0);
    VARAND(I4,0,BSTR,false_str,I4,0);
    VARAND(I4,-1,BSTR,false_str,I4,0);
    VARAND(I4,0,BSTR,true_str,I4,0);
    VARAND(I4,-1,BSTR,true_str,I4,-1);
    VARANDCY(I4,-1,10000,I4,1);
    VARANDCY(I4,-1,0,I4,0);
    VARANDCY(I4,0,0,I4,0);

    VARAND(UI4,0xffffffff,UI4,0xffffffff,I4,-1);
    VARAND(UI4,0xffffffff,UI4,0,I4,0);
    VARAND(UI4,0,UI4,0,I4,0);
    VARAND(UI4,0xffffffff,R4,-1,I4,-1);
    VARAND(UI4,0xffffffff,R4,0,I4,0);
    VARAND(UI4,0,R4,0,I4,0);
    VARAND(UI4,0xffffffff,R8,-1,I4,-1);
    VARAND(UI4,0xffffffff,R8,0,I4,0);
    VARAND(UI4,0,R8,0,I4,0);
    VARAND(UI4,0xffffffff,DATE,-1,I4,-1);
    VARAND(UI4,0xffffffff,DATE,0,I4,0);
    VARAND(UI4,0,DATE,0,I4,0);
    if (has_i8)
    {
        VARAND(UI4,0xffffffff,I8,0,I8,0);
        VARAND(UI4,0,I8,0,I8,0);
        VARAND(UI4,0xffffffff,UI8,0,I4,0);
        VARAND(UI4,0,UI8,0,I4,0);
    }
    VARAND(UI4,0xffffffff,INT,-1,I4,-1);
    VARAND(UI4,0xffffffff,INT,0,I4,0);
    VARAND(UI4,0,INT,0,I4,0);
    VARAND(UI4,0xffffffff,UINT,0xffffffff,I4,-1);
    VARAND(UI4,0xffffffff,UINT,0,I4,0);
    VARAND(UI4,0,UINT,0,I4,0);
    VARAND(UI4,0,BSTR,false_str,I4,0);
    VARAND(UI4,0xffffffff,BSTR,false_str,I4,0);
    VARAND(UI4,0,BSTR,true_str,I4,0);
    VARAND(UI4,0xffffffff,BSTR,true_str,I4,-1);
    VARANDCY(UI4,0xffffffff,10000,I4,1);
    VARANDCY(UI4,0xffffffff,0,I4,0);
    VARANDCY(UI4,0,0,I4,0);

    VARAND(R4,-1,R4,-1,I4,-1);
    VARAND(R4,-1,R4,0,I4,0);
    VARAND(R4,0,R4,0,I4,0);
    VARAND(R4,-1,R8,-1,I4,-1);
    VARAND(R4,-1,R8,0,I4,0);
    VARAND(R4,0,R8,0,I4,0);
    VARAND(R4,-1,DATE,-1,I4,-1);
    VARAND(R4,-1,DATE,0,I4,0);
    VARAND(R4,0,DATE,0,I4,0);
    if (has_i8)
    {
        VARAND(R4,-1,I8,-1,I8,-1);
        VARAND(R4,-1,I8,0,I8,0);
        VARAND(R4,0,I8,0,I8,0);
        VARAND(R4,-1,UI8,0,I4,0);
        VARAND(R4,0,UI8,0,I4,0);
    }
    VARAND(R4,-1,INT,-1,I4,-1);
    VARAND(R4,-1,INT,0,I4,0);
    VARAND(R4,0,INT,0,I4,0);
    VARAND(R4,-1,UINT,0xffffffff,I4,-1);
    VARAND(R4,-1,UINT,0,I4,0);
    VARAND(R4,0,UINT,0,I4,0);
    VARAND(R4,0,BSTR,false_str,I4,0);
    VARAND(R4,-1,BSTR,false_str,I4,0);
    VARAND(R4,0,BSTR,true_str,I4,0);
    VARAND(R4,-1,BSTR,true_str,I4,-1);
    VARANDCY(R4,-1,10000,I4,1);
    VARANDCY(R4,-1,0,I4,0);
    VARANDCY(R4,0,0,I4,0);

    VARAND(R8,-1,R8,-1,I4,-1);
    VARAND(R8,-1,R8,0,I4,0);
    VARAND(R8,0,R8,0,I4,0);
    VARAND(R8,-1,DATE,-1,I4,-1);
    VARAND(R8,-1,DATE,0,I4,0);
    VARAND(R8,0,DATE,0,I4,0);
    if (has_i8)
    {
        VARAND(R8,-1,I8,-1,I8,-1);
        VARAND(R8,-1,I8,0,I8,0);
        VARAND(R8,0,I8,0,I8,0);
        VARAND(R8,-1,UI8,0,I4,0);
        VARAND(R8,0,UI8,0,I4,0);
    }
    VARAND(R8,-1,INT,-1,I4,-1);
    VARAND(R8,-1,INT,0,I4,0);
    VARAND(R8,0,INT,0,I4,0);
    VARAND(R8,-1,UINT,0xffffffff,I4,-1);
    VARAND(R8,-1,UINT,0,I4,0);
    VARAND(R8,0,UINT,0,I4,0);
    VARAND(R8,0,BSTR,false_str,I4,0);
    VARAND(R8,-1,BSTR,false_str,I4,0);
    VARAND(R8,0,BSTR,true_str,I4,0);
    VARAND(R8,-1,BSTR,true_str,I4,-1);
    VARANDCY(R8,-1,10000,I4,1);
    VARANDCY(R8,-1,0,I4,0);
    VARANDCY(R8,0,0,I4,0);

    VARAND(DATE,-1,DATE,-1,I4,-1);
    VARAND(DATE,-1,DATE,0,I4,0);
    VARAND(DATE,0,DATE,0,I4,0);
    if (has_i8)
    {
        VARAND(DATE,-1,I8,-1,I8,-1);
        VARAND(DATE,-1,I8,0,I8,0);
        VARAND(DATE,0,I8,0,I8,0);
        VARAND(DATE,-1,UI8,0,I4,0);
        VARAND(DATE,0,UI8,0,I4,0);
    }
    VARAND(DATE,-1,INT,-1,I4,-1);
    VARAND(DATE,-1,INT,0,I4,0);
    VARAND(DATE,0,INT,0,I4,0);
    VARAND(DATE,-1,UINT,0xffffffff,I4,-1);
    VARAND(DATE,-1,UINT,0,I4,0);
    VARAND(DATE,0,UINT,0,I4,0);
    VARAND(DATE,0,BSTR,false_str,I4,0);
    VARAND(DATE,-1,BSTR,false_str,I4,0);
    VARAND(DATE,0,BSTR,true_str,I4,0);
    VARAND(DATE,-1,BSTR,true_str,I4,-1);
    VARANDCY(DATE,-1,10000,I4,1);
    VARANDCY(DATE,-1,0,I4,0);
    VARANDCY(DATE,0,0,I4,0);

    if (has_i8)
    {
        VARAND(I8,-1,I8,-1,I8,-1);
        VARAND(I8,-1,I8,0,I8,0);
        VARAND(I8,0,I8,0,I8,0);
        VARAND(I8,-1,UI8,0,I8,0);
        VARAND(I8,0,UI8,0,I8,0);
        VARAND(I8,-1,UINT,0,I8,0);
        VARAND(I8,0,UINT,0,I8,0);
        VARAND(I8,0,BSTR,false_str,I8,0);
        VARAND(I8,-1,BSTR,false_str,I8,0);
        VARAND(I8,0,BSTR,true_str,I8,0);
        VARAND(I8,-1,BSTR,true_str,I8,-1);
        VARANDCY(I8,-1,10000,I8,1);
        VARANDCY(I8,-1,0,I8,0);
        VARANDCY(I8,0,0,I8,0);

        VARAND(UI8,0xffff,UI8,0xffff,I4,0xffff);
        VARAND(UI8,0xffff,UI8,0,I4,0);
        VARAND(UI8,0,UI8,0,I4,0);
        VARAND(UI8,0xffff,INT,-1,I4,65535);
        VARAND(UI8,0xffff,INT,0,I4,0);
        VARAND(UI8,0,INT,0,I4,0);
        VARAND(UI8,0xffff,UINT,0xffff,I4,0xffff);
        VARAND(UI8,0xffff,UINT,0,I4,0);
        VARAND(UI8,0,UINT,0,I4,0);
        VARAND(UI8,0,BSTR,false_str,I4,0);
        VARAND(UI8,0xffff,BSTR,false_str,I4,0);
        VARAND(UI8,0,BSTR,true_str,I4,0);
        VARAND(UI8,0xffff,BSTR,true_str,I4,65535);
        VARANDCY(UI8,0xffff,10000,I4,1);
        VARANDCY(UI8,0xffff,0,I4,0);
        VARANDCY(UI8,0,0,I4,0);
    }

    VARAND(INT,-1,INT,-1,I4,-1);
    VARAND(INT,-1,INT,0,I4,0);
    VARAND(INT,0,INT,0,I4,0);
    VARAND(INT,-1,UINT,0xffff,I4,65535);
    VARAND(INT,-1,UINT,0,I4,0);
    VARAND(INT,0,UINT,0,I4,0);
    VARAND(INT,0,BSTR,false_str,I4,0);
    VARAND(INT,-1,BSTR,false_str,I4,0);
    VARAND(INT,0,BSTR,true_str,I4,0);
    VARAND(INT,-1,BSTR,true_str,I4,-1);
    VARANDCY(INT,-1,10000,I4,1);
    VARANDCY(INT,-1,0,I4,0);
    VARANDCY(INT,0,0,I4,0);

    VARAND(UINT,0xffff,UINT,0xffff,I4,0xffff);
    VARAND(UINT,0xffff,UINT,0,I4,0);
    VARAND(UINT,0,UINT,0,I4,0);
    VARAND(UINT,0,BSTR,false_str,I4,0);
    VARAND(UINT,0xffff,BSTR, false_str,I4,0);
    VARAND(UINT,0,BSTR,true_str,I4,0);
    VARAND(UINT,0xffff,BSTR,true_str,I4,65535);
    VARANDCY(UINT,0xffff,10000,I4,1);
    VARANDCY(UINT,0xffff,0,I4,0);
    VARANDCY(UINT,0,0,I4,0);

    VARAND(BSTR,false_str,BSTR,false_str,BOOL,0);
    VARAND(BSTR,true_str,BSTR,false_str,BOOL,VARIANT_FALSE);
    VARAND(BSTR,true_str,BSTR,true_str,BOOL,VARIANT_TRUE);
    VARANDCY(BSTR,true_str,10000,I4,1);
    VARANDCY(BSTR,false_str,10000,I4,0);

    SysFreeString(true_str);
    SysFreeString(false_str);
}

static void test_cmp( int line, LCID lcid, UINT flags, VARIANT *left, VARIANT *right, HRESULT result )
{
    HRESULT hres;

    hres = VarCmp(left,right,lcid,flags);
    ok_(__FILE__,line)(hres == result, "VarCmp(%s,%s): expected 0x%lx, got hres=0x%lx\n",
                       wine_dbgstr_variant(left), wine_dbgstr_variant(right), result, hres );
}
static void test_cmpex( int line, LCID lcid, VARIANT *left, VARIANT *right,
                        HRESULT res1, HRESULT res2, HRESULT res3, HRESULT res4 )
{
    test_cmp( line, lcid, 0, left, right, res1 );
    V_VT(left) |= VT_RESERVED;
    test_cmp( line, lcid, 0, left, right, res2 );
    V_VT(left) &= ~VT_RESERVED;
    V_VT(right) |= VT_RESERVED;
    test_cmp( line, lcid, 0, left, right, res3 );
    V_VT(left) |= VT_RESERVED;
    test_cmp( line, lcid, 0, left, right, res4 );
    ok_(__FILE__,line)(V_VT(left) & V_VT(right) & VT_RESERVED, "VT_RESERVED filtered out\n");
}

/* ERROR from wingdi.h is interfering here */
#undef ERROR
#define _VARCMP(vt1,val1,vtfl1,vt2,val2,vtfl2,lcid,flags,result) \
        V_##vt1(&left) = val1; V_VT(&left) = VT_##vt1 | vtfl1; \
        V_##vt2(&right) = val2; V_VT(&right) = VT_##vt2 | vtfl2; \
        test_cmp( __LINE__, lcid, flags, &left, &right, result )
#define VARCMPEX(vt1,val1,vt2,val2,res1,res2,res3,res4) \
        V_##vt1(&left) = val1; V_VT(&left) = VT_##vt1; \
        V_##vt2(&right) = val2; V_VT(&right) = VT_##vt2; \
        test_cmpex( __LINE__, lcid, &left, &right, res1, res2, res3, res4 )
#define VARCMP(vt1,val1,vt2,val2,result) \
        VARCMPEX(vt1,val1,vt2,val2,result,result,result,result)
/* The above macros do not work for VT_NULL as NULL gets expanded first */
#define V_NULL_  V_NULL
#define VT_NULL_ VT_NULL

static void test_VarCmp(void)
{
    VARIANT left, right;
    VARTYPE i;
    LCID lcid;
    HRESULT hres;
    DECIMAL dec;
    BSTR bstrhuh, bstrempty, bstr0, bstr1, bstr7, bstr42, bstr1neg, bstr666neg;
    BSTR bstr2cents, bstr1few;

    lcid = MAKELCID(MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),SORT_DEFAULT);
    bstrempty = SysAllocString(L"");
    bstrhuh = SysAllocString(L"huh?");
    bstr2cents = SysAllocString(L"2cents");
    bstr0 = SysAllocString(L"0");
    bstr1 = SysAllocString(L"1");
    bstr7 = SysAllocString(L"7");
    bstr42 = SysAllocString(L"42");
    bstr1neg = SysAllocString(L"-1");
    bstr666neg = SysAllocString(L"-666");
    bstr1few = SysAllocString(L"1.00000001");

    /* Test all possible flag/vt combinations & the resulting vt type */
    for (i = 0; i < ARRAY_SIZE(ExtraFlags); i++)
    {
        VARTYPE leftvt, rightvt;

        for (leftvt = 0; leftvt <= VT_BSTR_BLOB; leftvt++)
        {

            SKIPTESTS(leftvt);

            for (rightvt = 0; rightvt <= VT_BSTR_BLOB; rightvt++)
            {
                BOOL bFail = FALSE;
                HRESULT expect = VARCMP_EQ;

                SKIPTESTS(rightvt);

                memset(&left, 0, sizeof(left));
                memset(&right, 0, sizeof(right));
                V_VT(&left) = leftvt | ExtraFlags[i];
                if (leftvt == VT_BSTR) {
                    V_BSTR(&left) = bstr1neg;
                    if (ExtraFlags[i] & VT_RESERVED)
                        expect = VARCMP_LT;
                    else
                        expect = VARCMP_GT;
                }
                V_VT(&right) = rightvt | ExtraFlags[i];
                if (rightvt == VT_BSTR) {
                    V_BSTR(&right) = bstr1neg;
                    if (ExtraFlags[i] & VT_RESERVED)
                        expect = VARCMP_GT;
                    else
                        expect = VARCMP_LT;
                }

                /* Don't ask me why but native VarCmp cannot handle:
                   VT_I1, VT_UI2, VT_UI4, VT_UINT and VT_UI8.
                   VT_INT is only supported as left variant. Go figure.
                   Tested with DCOM98, Win2k, WinXP */
                if (ExtraFlags[i] & VT_ARRAY || ExtraFlags[i] & VT_BYREF ||
                    !IsValidVariantClearVT(leftvt, ExtraFlags[i] & ~VT_RESERVED) ||
                    !IsValidVariantClearVT(rightvt, ExtraFlags[i] & ~VT_RESERVED) ||
                    leftvt == VT_CLSID || rightvt == VT_CLSID ||
                    leftvt == VT_DISPATCH || rightvt == VT_DISPATCH ||
                    leftvt == VT_ERROR || rightvt == VT_ERROR ||
                    leftvt == VT_RECORD || rightvt == VT_RECORD ||
                    leftvt == VT_UNKNOWN || rightvt == VT_UNKNOWN ||
                    leftvt == VT_VARIANT || rightvt == VT_VARIANT ||
                    leftvt == VT_I1 || rightvt == VT_I1 ||
                    leftvt == VT_UI2 || rightvt == VT_UI2 ||
                    leftvt == VT_UI4 || rightvt == VT_UI4 ||
                    leftvt == VT_UI8 || rightvt == VT_UI8 ||
                    rightvt == VT_INT ||
                    leftvt == VT_UINT || rightvt == VT_UINT) {
                    bFail = TRUE;
                }

                if (leftvt == VT_ERROR && rightvt == VT_ERROR &&
                    !(ExtraFlags[i] & ~VT_RESERVED)) {
                    expect = VARCMP_EQ;
                    bFail = FALSE;
                } else if (leftvt == VT_NULL || rightvt == VT_NULL)
                    expect = VARCMP_NULL;
                else if (leftvt == VT_BSTR && rightvt == VT_BSTR)
                    expect = VARCMP_EQ;
                else if (leftvt == VT_BSTR && rightvt == VT_EMPTY)
                    expect = VARCMP_GT;
                else if (leftvt == VT_EMPTY && rightvt == VT_BSTR)
                    expect = VARCMP_LT;

                hres = VarCmp(&left, &right, LOCALE_USER_DEFAULT, 0);
                if (bFail) {
                    ok(hres == DISP_E_TYPEMISMATCH || hres == DISP_E_BADVARTYPE,
                       "VarCmp: %d|0x%X, %d|0x%X: Expected failure, got 0x%lX\n",
                       leftvt, ExtraFlags[i], rightvt, ExtraFlags[i], hres);
                } else {
                    ok(hres == expect,
                       "VarCmp: %d|0x%X, %d|0x%X: Expected 0x%lX, got 0x%lX\n",
                       leftvt, ExtraFlags[i], rightvt, ExtraFlags[i], expect,
                       hres);
                }
            }
        }
    }

    /* VARCMP{,EX} run each 4 tests with a permutation of all possible
       input variants with (1) and without (0) VT_RESERVED set. The order
       of the permutations is (0,0); (1,0); (0,1); (1,1) */
    VARCMP(INT,4711,I2,4711,VARCMP_EQ);
    VARCMP(INT,4711,I2,-4711,VARCMP_GT);
    VARCMP(ERROR,0,ERROR,0,VARCMP_EQ);
    VARCMP(ERROR,0,UI1,0,DISP_E_TYPEMISMATCH);
    VARCMP(EMPTY,0,EMPTY,0,VARCMP_EQ);
    VARCMP(I4,1,R8,1.0,VARCMP_EQ);
    VARCMP(EMPTY,19,I2,0,VARCMP_EQ);
    ok(V_EMPTY(&left) == 19, "VT_EMPTY modified!\n");
    VARCMP(I4,1,UI1,1,VARCMP_EQ);
    VARCMP(I2,2,I2,2,VARCMP_EQ);
    VARCMP(I2,1,I2,2,VARCMP_LT);
    VARCMP(I2,2,I2,1,VARCMP_GT);
    VARCMP(I2,2,EMPTY,1,VARCMP_GT);
    VARCMP(I2,2,NULL_,1,VARCMP_NULL);

    /* BSTR handling, especially in conjunction with VT_RESERVED */
    VARCMP(BSTR,bstr0,NULL_,0,VARCMP_NULL);
    VARCMP(BSTR,bstr0,BSTR,bstr0,VARCMP_EQ);
    VARCMP(BSTR,bstrempty,BSTR,bstr0,VARCMP_LT);
    VARCMP(BSTR,bstr7,BSTR,bstr0,VARCMP_GT);
    VARCMP(BSTR,bstr7,BSTR,bstr1neg,VARCMP_GT);
    VARCMP(BSTR,bstr0,BSTR,NULL,VARCMP_GT);
    VARCMP(BSTR,NULL,BSTR,NULL,VARCMP_EQ);
    VARCMP(BSTR,bstrempty,BSTR,NULL,VARCMP_EQ);
    VARCMP(BSTR,NULL,EMPTY,0,VARCMP_EQ);
    VARCMP(EMPTY,0,BSTR,NULL,VARCMP_EQ);
    VARCMP(EMPTY,0,BSTR,bstrempty,VARCMP_EQ);
    VARCMP(EMPTY,1,BSTR,bstrempty,VARCMP_EQ);
    VARCMP(BSTR,bstr0,EMPTY,0,VARCMP_GT);
    VARCMP(BSTR,bstr42,EMPTY,0,VARCMP_GT);
    VARCMPEX(BSTR,bstrempty,UI1,0,VARCMP_GT,VARCMP_LT,VARCMP_GT,VARCMP_GT);
    VARCMPEX(BSTR,bstrempty,I2,-1,VARCMP_GT,VARCMP_LT,VARCMP_GT,VARCMP_GT);
    VARCMPEX(I4,0,BSTR,bstrempty,VARCMP_LT,VARCMP_LT,VARCMP_GT,VARCMP_LT);
    VARCMPEX(BSTR,NULL,UI1,0,VARCMP_GT,VARCMP_LT,VARCMP_GT,VARCMP_GT);
    VARCMPEX(I4,7,BSTR,NULL,VARCMP_LT,VARCMP_LT,VARCMP_GT,VARCMP_LT);
    _VARCMP(BSTR,(BSTR)100,0,I2,100,0,lcid,0,VARCMP_GT);
    VARCMPEX(BSTR,bstr0,UI1,0,VARCMP_GT,VARCMP_EQ,VARCMP_EQ,VARCMP_GT);
    VARCMPEX(I2,0,BSTR,bstr0,VARCMP_LT,VARCMP_EQ,VARCMP_EQ,VARCMP_LT);
    VARCMP(BSTR,bstrhuh,I4,I4_MAX,VARCMP_GT);
    VARCMP(BSTR,bstr2cents,I4,2,VARCMP_GT);
    VARCMPEX(BSTR,bstr2cents,I4,42,VARCMP_GT,VARCMP_LT,VARCMP_GT,VARCMP_GT);
    VARCMP(BSTR,bstr2cents,I4,-1,VARCMP_GT);
    VARCMPEX(BSTR,bstr2cents,I4,-666,VARCMP_GT,VARCMP_LT,VARCMP_GT,VARCMP_GT);
    VARCMPEX(BSTR,bstr0,I2,0,VARCMP_GT,VARCMP_EQ,VARCMP_EQ,VARCMP_GT);
    VARCMPEX(BSTR,bstr0,I4,0,VARCMP_GT,VARCMP_EQ,VARCMP_EQ,VARCMP_GT);
    VARCMPEX(BSTR,bstr0,I4,-666,VARCMP_GT,VARCMP_LT,VARCMP_GT,VARCMP_GT);
    VARCMP(BSTR,bstr1,I4,0,VARCMP_GT);
    VARCMPEX(BSTR,bstr1,I4,1,VARCMP_GT,VARCMP_EQ,VARCMP_EQ,VARCMP_GT);
    VARCMPEX(BSTR,bstr1,I4,I4_MAX,VARCMP_GT,VARCMP_LT,VARCMP_LT,VARCMP_GT);
    VARCMPEX(BSTR,bstr1,I4,-1,VARCMP_GT,VARCMP_LT,VARCMP_GT,VARCMP_GT);
    VARCMP(BSTR,bstr7,I4,1,VARCMP_GT);
    VARCMPEX(BSTR,bstr7,I4,7,VARCMP_GT,VARCMP_EQ,VARCMP_EQ,VARCMP_GT);
    VARCMPEX(BSTR,bstr7,I4,42,VARCMP_GT,VARCMP_GT,VARCMP_LT,VARCMP_GT);
    VARCMPEX(BSTR,bstr42,I4,7,VARCMP_GT,VARCMP_LT,VARCMP_GT,VARCMP_GT);
    VARCMPEX(BSTR,bstr42,I4,42,VARCMP_GT,VARCMP_EQ,VARCMP_EQ,VARCMP_GT);
    VARCMPEX(BSTR,bstr42,I4,I4_MAX,VARCMP_GT,VARCMP_GT,VARCMP_LT,VARCMP_GT);
    VARCMPEX(BSTR,bstr1neg,I4,1,VARCMP_GT,VARCMP_GT,VARCMP_LT,VARCMP_LT);
    VARCMPEX(BSTR,bstr1neg,I4,42,VARCMP_GT,VARCMP_LT,VARCMP_LT,VARCMP_LT);
    VARCMPEX(BSTR,bstr1neg,I4,-1,VARCMP_GT,VARCMP_EQ,VARCMP_EQ,VARCMP_LT);
    VARCMPEX(BSTR,bstr1neg,I4,-666,VARCMP_GT,VARCMP_LT,VARCMP_GT,VARCMP_LT);
    VARCMPEX(BSTR,bstr666neg,I4,1,VARCMP_GT,VARCMP_GT,VARCMP_LT,VARCMP_LT);
    VARCMPEX(BSTR,bstr666neg,I4,7,VARCMP_GT,VARCMP_LT,VARCMP_LT,VARCMP_LT);
    VARCMPEX(BSTR,bstr666neg,I4,42,VARCMP_GT,VARCMP_GT,VARCMP_LT,VARCMP_LT);
    VARCMPEX(BSTR,bstr666neg,I4,I4_MAX,VARCMP_GT,VARCMP_GT,VARCMP_LT,VARCMP_LT);
    VARCMPEX(BSTR,bstr666neg,I4,-1,VARCMP_GT,VARCMP_GT,VARCMP_LT,VARCMP_LT);
    VARCMPEX(BSTR,bstr666neg,I4,-666,VARCMP_GT,VARCMP_EQ,VARCMP_EQ,VARCMP_LT);
    VARCMPEX(BSTR,bstr7,R8,7.0,VARCMP_GT,VARCMP_EQ,VARCMP_EQ,VARCMP_GT);
    VARCMPEX(R8,3.141592,BSTR,NULL,VARCMP_LT,VARCMP_LT,VARCMP_GT,VARCMP_LT);
    VARCMP(BSTR,bstr7,BSTR,bstr7,VARCMP_EQ);
    VARCMP(BSTR,bstr7,BSTR,bstr42,VARCMP_GT);
    VARCMP(BSTR,bstr42,BSTR,bstr7,VARCMP_LT);

    /* DECIMAL handling */
    setdec(&dec,0,0,0,0);
    VARCMPEX(DECIMAL,dec,BSTR,bstr0,VARCMP_LT,VARCMP_EQ,VARCMP_EQ,VARCMP_LT);
    setdec64(&dec,0,0,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF); /* max DECIMAL */
    VARCMP(DECIMAL,dec,R8,R8_MAX,VARCMP_LT);    /* R8 has bigger range */
    VARCMP(DECIMAL,dec,DATE,R8_MAX,VARCMP_LT);  /* DATE has bigger range */
    setdec64(&dec,0,0x80,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF);
    VARCMP(DECIMAL,dec,R8,-R8_MAX,VARCMP_GT);
    setdec64(&dec,20,0,0x5,0x6BC75E2D,0x63100001);    /* 1+1e-20 */
    VARCMP(DECIMAL,dec,R8,1,VARCMP_GT); /* DECIMAL has higher precision */

    /* Show that DATE is handled just as a R8 */
    VARCMP(DATE,DATE_MAX,DATE,DATE_MAX+1,VARCMP_LT);
    VARCMP(DATE,DATE_MIN,DATE,DATE_MIN-1,VARCMP_GT);
    VARCMP(DATE,R8_MIN,R8,R8_MIN,VARCMP_EQ);
    VARCMP(DATE,1,DATE,1+1e-15,VARCMP_LT);      /* 1e-15 == 8.64e-11 seconds */
    VARCMP(DATE,25570.0,DATE,25570.0,VARCMP_EQ);
    VARCMP(DATE,25570.0,DATE,25571.0,VARCMP_LT);
    VARCMP(DATE,25571.0,DATE,25570.0,VARCMP_GT);
    VARCMP(DATE,25570.0,EMPTY,0,VARCMP_GT);
    VARCMP(DATE,25570.0,NULL_,0,VARCMP_NULL);

    /* R4 precision handling */
    VARCMP(R4,1,R8,1+1e-8,VARCMP_EQ);
    VARCMP(R8,1+1e-8,R4,1,VARCMP_EQ);
    VARCMP(R8,1+1e-8,R8,1,VARCMP_GT);
    VARCMP(R8,R4_MAX*1.1,R4,R4_MAX,VARCMP_GT);
    VARCMP(R4,R4_MAX,R8,R8_MAX,VARCMP_LT);
    VARCMP(R4,1,DATE,1+1e-8,VARCMP_EQ);
    VARCMP(R4,1,BSTR,bstr1few,VARCMP_LT); /* bstr1few == 1+1e-8 */
    setdec(&dec,8,0,0,0x5F5E101);         /* 1+1e-8 */
    VARCMP(R4,1,DECIMAL,dec,VARCMP_LT);

    SysFreeString(bstrhuh);
    SysFreeString(bstrempty);
    SysFreeString(bstr0);
    SysFreeString(bstr1);
    SysFreeString(bstr7);
    SysFreeString(bstr42);
    SysFreeString(bstr1neg);
    SysFreeString(bstr666neg);
    SysFreeString(bstr2cents);
    SysFreeString(bstr1few);
}

static HRESULT (WINAPI *pVarPow)(LPVARIANT,LPVARIANT,LPVARIANT);

#define VARPOW(vt1,val1,vt2,val2,rvt,rval)               \
        V_VT(&left) = VT_##vt1; V_##vt1(&left) = val1;   \
        V_VT(&right) = VT_##vt2; V_##vt2(&right) = val2; \
        V_VT(&exp) = VT_##rvt; V_##rvt(&exp) = rval;     \
        test_var_call2( __LINE__, pVarPow, &left, &right, &exp )

/* Skip any type that is not defined or produces an error for every case */
#define SKIPTESTPOW(a)                            \
    if (a == VT_ERROR || a == VT_VARIANT ||       \
        a == VT_DISPATCH || a == VT_UNKNOWN ||    \
        a == VT_RECORD || a > VT_UINT ||          \
        a == 15 /*not defined*/)                  \
        continue

static void test_VarPow(void)
{
    VARIANT left, right, exp, result, cy, dec;
    BSTR num2_str, num3_str;
    VARTYPE i;
    HRESULT hres;

    CHECKPTR(VarPow);

    num2_str = SysAllocString(L"2");
    num3_str = SysAllocString(L"3");

    /* Test all possible flag/vt combinations & the resulting vt type */
    for (i = 0; i < ARRAY_SIZE(ExtraFlags); i++)
    {
        VARTYPE leftvt, rightvt, resvt;

        for (leftvt = 0; leftvt <= VT_BSTR_BLOB; leftvt++)
        {
            SKIPTESTPOW(leftvt);

            for (rightvt = 0; rightvt <= VT_BSTR_BLOB; rightvt++)
            {
                BOOL bFail = FALSE;
                SKIPTESTPOW(rightvt);

                /* Native crashes with VT_BYREF */
                if (ExtraFlags[i] == VT_BYREF)
                    continue;

                memset(&left, 0, sizeof(left));
                memset(&right, 0, sizeof(right));
                V_VT(&left) = leftvt | ExtraFlags[i];
                V_VT(&right) = rightvt | ExtraFlags[i];
                V_VT(&result) = VT_EMPTY;
                resvt = VT_EMPTY;

                if (leftvt == VT_BSTR)
                    V_BSTR(&left) = num2_str;
                if (rightvt == VT_BSTR)
                    V_BSTR(&right) = num2_str;

                /* Native VarPow always returns an error when using extra flags */
                if (ExtraFlags[i] != 0)
                    bFail = TRUE;

                /* Determine return type */
                else if ((leftvt == VT_NULL || rightvt == VT_NULL) &&
                         ((leftvt != VT_I8 && leftvt != VT_UI8 &&
                           rightvt != VT_I8 && rightvt != VT_UI8) || has_i8))
                    resvt = VT_NULL;
                else if ((leftvt == VT_EMPTY || leftvt == VT_I2 ||
                    leftvt == VT_I4 || leftvt == VT_R4 ||
                    leftvt == VT_R8 || leftvt == VT_CY ||
                    leftvt == VT_DATE || leftvt == VT_BSTR ||
                    leftvt == VT_BOOL || leftvt == VT_DECIMAL ||
                    (leftvt >= VT_I1 && leftvt <= VT_UI4) ||
                    (has_i8 && (leftvt == VT_I8 || leftvt == VT_UI8)) ||
                    leftvt == VT_INT || leftvt == VT_UINT) &&
                    (rightvt == VT_EMPTY || rightvt == VT_I2 ||
                    rightvt == VT_I4 || rightvt == VT_R4 ||
                    rightvt == VT_R8 || rightvt == VT_CY ||
                    rightvt == VT_DATE || rightvt == VT_BSTR ||
                    rightvt == VT_BOOL || rightvt == VT_DECIMAL ||
                    (rightvt >= VT_I1 && rightvt <= VT_UI4) ||
                    (has_i8 && (rightvt == VT_I8 || rightvt == VT_UI8)) ||
                    rightvt == VT_INT || rightvt == VT_UINT))
                    resvt = VT_R8;
                else
                    bFail = TRUE;

                hres = pVarPow(&left, &right, &result);

                /* Check expected HRESULT and if result variant type is correct */
                if (bFail)
                    ok (hres == DISP_E_BADVARTYPE || hres == DISP_E_TYPEMISMATCH,
                        "VarPow: %s|0x%X, %s|0x%X: got vt %s hr 0x%lX\n",
                        wine_dbgstr_vt(leftvt), ExtraFlags[i], wine_dbgstr_vt(rightvt), ExtraFlags[i],
                        wine_dbgstr_vt(V_VT(&result)), hres);
                else
                    ok (hres == S_OK && resvt == V_VT(&result),
                        "VarPow: %s|0x%X, %s|0x%X: expected vt %s hr 0x%lX, got vt %s hr 0x%lX\n",
                        wine_dbgstr_vt(leftvt), ExtraFlags[i], wine_dbgstr_vt(rightvt), ExtraFlags[i], wine_dbgstr_vt(resvt),
                        S_OK, wine_dbgstr_vt(V_VT(&result)), hres);
            }
        }
    }

    /* Check return values for valid variant type combinations */
    VARPOW(EMPTY,0,EMPTY,0,R8,1.0);
    VARPOW(EMPTY,0,NULL,0,NULL,0);
    VARPOW(EMPTY,0,I2,3,R8,0.0);
    VARPOW(EMPTY,0,I4,3,R8,0.0);
    VARPOW(EMPTY,0,R4,3.0f,R8,0.0);
    VARPOW(EMPTY,0,R8,3.0,R8,0.0);
    VARPOW(EMPTY,0,DATE,3,R8,0.0);
    VARPOW(EMPTY,0,BSTR,num3_str,R8,0.0);
    VARPOW(EMPTY,0,BOOL,VARIANT_FALSE,R8,1.0);
    VARPOW(EMPTY,0,I1,3,R8,0.0);
    VARPOW(EMPTY,0,UI1,3,R8,0.0);
    VARPOW(EMPTY,0,UI2,3,R8,0.0);
    VARPOW(EMPTY,0,UI4,3,R8,0.0);
    if (has_i8)
    {
        VARPOW(EMPTY,0,I8,3,R8,0.0);
        VARPOW(EMPTY,0,UI8,3,R8,0.0);
    }
    VARPOW(EMPTY,0,INT,3,R8,0.0);
    VARPOW(EMPTY,0,UINT,3,R8,0.0);
    VARPOW(NULL,0,EMPTY,0,NULL,0);
    VARPOW(NULL,0,NULL,0,NULL,0);
    VARPOW(NULL,0,I2,3,NULL,0);
    VARPOW(NULL,0,I4,3,NULL,0);
    VARPOW(NULL,0,R4,3.0f,NULL,0);
    VARPOW(NULL,0,R8,3.0,NULL,0);
    VARPOW(NULL,0,DATE,3,NULL,0);
    VARPOW(NULL,0,BSTR,num3_str,NULL,0);
    VARPOW(NULL,0,BOOL,VARIANT_TRUE,NULL,0);
    VARPOW(NULL,0,I1,3,NULL,0);
    VARPOW(NULL,0,UI1,3,NULL,0);
    VARPOW(NULL,0,UI2,3,NULL,0);
    VARPOW(NULL,0,UI4,3,NULL,0);
    if (has_i8)
    {
        VARPOW(NULL,0,I8,3,NULL,0);
        VARPOW(NULL,0,UI8,3,NULL,0);
    }
    VARPOW(NULL,0,INT,3,NULL,0);
    VARPOW(NULL,0,UINT,3,NULL,0);
    VARPOW(I2,2,EMPTY,0,R8,1.0);
    VARPOW(I2,2,NULL,0,NULL,0);
    VARPOW(I2,2,I2,3,R8,8.0);
    VARPOW(I2,2,I4,3,R8,8.0);
    VARPOW(I2,2,R4,3.0f,R8,8.0);
    VARPOW(I2,2,R8,3.0,R8,8.0);
    VARPOW(I2,2,DATE,3,R8,8.0);
    VARPOW(I2,2,BSTR,num3_str,R8,8.0);
    VARPOW(I2,2,BOOL,VARIANT_FALSE,R8,1.0);
    VARPOW(I2,2,I1,3,R8,8.0);
    VARPOW(I2,2,UI1,3,R8,8.0);
    VARPOW(I2,2,UI2,3,R8,8.0);
    VARPOW(I2,2,UI4,3,R8,8.0);
    if (has_i8)
    {
        VARPOW(I2,2,I8,3,R8,8.0);
        VARPOW(I2,2,UI8,3,R8,8.0);
    }
    VARPOW(I2,2,INT,3,R8,8.0);
    VARPOW(I2,2,UINT,3,R8,8.0);
    VARPOW(I4,2,EMPTY,0,R8,1.0);
    VARPOW(I4,2,NULL,0,NULL,0);
    VARPOW(I4,2,I2,3,R8,8.0);
    VARPOW(I4,2,I4,3,R8,8.0);
    VARPOW(I4,2,R4,3.0f,R8,8.0);
    VARPOW(I4,2,R8,3.0,R8,8.0);
    VARPOW(I4,2,DATE,3,R8,8.0);
    VARPOW(I4,2,BSTR,num3_str,R8,8.0);
    VARPOW(I4,2,BOOL,VARIANT_FALSE,R8,1.0);
    VARPOW(I4,2,I1,3,R8,8.0);
    VARPOW(I4,2,UI1,3,R8,8.0);
    VARPOW(I4,2,UI2,3,R8,8.0);
    VARPOW(I4,2,UI4,3,R8,8.0);
    if (has_i8)
    {
        VARPOW(I4,2,I8,3,R8,8.0);
        VARPOW(I4,2,UI8,3,R8,8.0);
    }
    VARPOW(I4,2,INT,3,R8,8.0);
    VARPOW(I4,2,UINT,3,R8,8.0);
    VARPOW(R4,2,EMPTY,0,R8,1.0);
    VARPOW(R4,2,NULL,0,NULL,0);
    VARPOW(R4,2,I2,3,R8,8.0);
    VARPOW(R4,2,I4,3,R8,8.0);
    VARPOW(R4,2,R4,3.0f,R8,8.0);
    VARPOW(R4,2,R8,3.0,R8,8.0);
    VARPOW(R4,2,DATE,3,R8,8.0);
    VARPOW(R4,2,BSTR,num3_str,R8,8.0);
    VARPOW(R4,2,BOOL,VARIANT_FALSE,R8,1.0);
    VARPOW(R4,2,I1,3,R8,8.0);
    VARPOW(R4,2,UI1,3,R8,8.0);
    VARPOW(R4,2,UI2,3,R8,8.0);
    VARPOW(R4,2,UI4,3,R8,8.0);
    if (has_i8)
    {
        VARPOW(R4,2,I8,3,R8,8.0);
        VARPOW(R4,2,UI8,3,R8,8.0);
    }
    VARPOW(R4,2,INT,3,R8,8.0);
    VARPOW(R4,2,UINT,3,R8,8.0);
    VARPOW(R8,2,EMPTY,0,R8,1.0);
    VARPOW(R8,2,NULL,0,NULL,0);
    VARPOW(R8,2,I2,3,R8,8.0);
    VARPOW(R8,2,I4,3,R8,8.0);
    VARPOW(R8,2,R4,3.0f,R8,8.0);
    VARPOW(R8,2,R8,3.0,R8,8.0);
    VARPOW(R8,2,DATE,3,R8,8.0);
    VARPOW(R8,2,BSTR,num3_str,R8,8.0);
    VARPOW(R8,2,BOOL,VARIANT_FALSE,R8,1.0);
    VARPOW(R8,2,I1,3,R8,8.0);
    VARPOW(R8,2,UI1,3,R8,8.0);
    VARPOW(R8,2,UI2,3,R8,8.0);
    VARPOW(R8,2,UI4,3,R8,8.0);
    if (has_i8)
    {
        VARPOW(R8,2,I8,3,R8,8.0);
        VARPOW(R8,2,UI8,3,R8,8.0);
    }
    VARPOW(R8,2,INT,3,R8,8.0);
    VARPOW(R8,2,UINT,3,R8,8.0);
    VARPOW(DATE,2,EMPTY,0,R8,1.0);
    VARPOW(DATE,2,NULL,0,NULL,0);
    VARPOW(DATE,2,I2,3,R8,8.0);
    VARPOW(DATE,2,I4,3,R8,8.0);
    VARPOW(DATE,2,R4,3.0f,R8,8.0);
    VARPOW(DATE,2,R8,3.0,R8,8.0);
    VARPOW(DATE,2,DATE,3,R8,8.0);
    VARPOW(DATE,2,BSTR,num3_str,R8,8.0);
    VARPOW(DATE,2,BOOL,VARIANT_FALSE,R8,1.0);
    VARPOW(DATE,2,I1,3,R8,8.0);
    VARPOW(DATE,2,UI1,3,R8,8.0);
    VARPOW(DATE,2,UI2,3,R8,8.0);
    VARPOW(DATE,2,UI4,3,R8,8.0);
    if (has_i8)
    {
        VARPOW(DATE,2,I8,3,R8,8.0);
        VARPOW(DATE,2,UI8,3,R8,8.0);
    }
    VARPOW(DATE,2,INT,3,R8,8.0);
    VARPOW(DATE,2,UINT,3,R8,8.0);
    VARPOW(BSTR,num2_str,EMPTY,0,R8,1.0);
    VARPOW(BSTR,num2_str,NULL,0,NULL,0);
    VARPOW(BSTR,num2_str,I2,3,R8,8.0);
    VARPOW(BSTR,num2_str,I4,3,R8,8.0);
    VARPOW(BSTR,num2_str,R4,3.0f,R8,8.0);
    VARPOW(BSTR,num2_str,R8,3.0,R8,8.0);
    VARPOW(BSTR,num2_str,DATE,3,R8,8.0);
    VARPOW(BSTR,num2_str,BSTR,num3_str,R8,8.0);
    VARPOW(BSTR,num2_str,BOOL,VARIANT_FALSE,R8,1.0);
    VARPOW(BSTR,num2_str,I1,3,R8,8.0);
    VARPOW(BSTR,num2_str,UI1,3,R8,8.0);
    VARPOW(BSTR,num2_str,UI2,3,R8,8.0);
    VARPOW(BSTR,num2_str,UI4,3,R8,8.0);
    if (has_i8)
    {
        VARPOW(BSTR,num2_str,I8,3,R8,8.0);
        VARPOW(BSTR,num2_str,UI8,3,R8,8.0);
    }
    VARPOW(BSTR,num2_str,INT,3,R8,8.0);
    VARPOW(BSTR,num2_str,UINT,3,R8,8.0);
    VARPOW(BOOL,VARIANT_TRUE,EMPTY,0,R8,1.0);
    VARPOW(BOOL,VARIANT_TRUE,NULL,0,NULL,0);
    VARPOW(BOOL,VARIANT_TRUE,I2,3,R8,-1.0);
    VARPOW(BOOL,VARIANT_TRUE,I4,3,R8,-1.0);
    VARPOW(BOOL,VARIANT_TRUE,R4,3.0f,R8,-1.0);
    VARPOW(BOOL,VARIANT_TRUE,R8,3.0,R8,-1.0);
    VARPOW(BOOL,VARIANT_TRUE,DATE,3,R8,-1.0);
    VARPOW(BOOL,VARIANT_TRUE,BSTR,num3_str,R8,-1.0);
    VARPOW(BOOL,VARIANT_TRUE,BOOL,VARIANT_TRUE,R8,-1.0);
    VARPOW(BOOL,VARIANT_TRUE,I1,3,R8,-1.0);
    VARPOW(BOOL,VARIANT_TRUE,UI1,3,R8,-1.0);
    VARPOW(BOOL,VARIANT_TRUE,UI2,3,R8,-1.0);
    VARPOW(BOOL,VARIANT_TRUE,UI4,3,R8,-1.0);
    if (has_i8)
    {
        VARPOW(BOOL,VARIANT_TRUE,I8,3,R8,-1.0);
        VARPOW(BOOL,VARIANT_TRUE,UI8,3,R8,-1.0);
    }
    VARPOW(BOOL,VARIANT_TRUE,INT,3,R8,-1.0);
    VARPOW(BOOL,VARIANT_TRUE,UINT,3,R8,-1.0);
    VARPOW(I1,2,EMPTY,0,R8,1.0);
    VARPOW(I1,2,NULL,0,NULL,0);
    VARPOW(I1,2,I2,3,R8,8.0);
    VARPOW(I1,2,I4,3,R8,8.0);
    VARPOW(I1,2,R4,3.0f,R8,8.0);
    VARPOW(I1,2,R8,3.0,R8,8.0);
    VARPOW(I1,2,DATE,3,R8,8.0);
    VARPOW(I1,2,BSTR,num3_str,R8,8.0);
    VARPOW(I1,2,BOOL,VARIANT_FALSE,R8,1.0);
    VARPOW(I1,2,I1,3,R8,8.0);
    VARPOW(I1,2,UI1,3,R8,8.0);
    VARPOW(I1,2,UI2,3,R8,8.0);
    VARPOW(I1,2,UI4,3,R8,8.0);
    if (has_i8)
    {
        VARPOW(I1,2,I8,3,R8,8.0);
        VARPOW(I1,2,UI8,3,R8,8.0);
    }
    VARPOW(I1,2,INT,3,R8,8.0);
    VARPOW(I1,2,UINT,3,R8,8.0);
    VARPOW(UI1,2,EMPTY,0,R8,1.0);
    VARPOW(UI1,2,NULL,0,NULL,0);
    VARPOW(UI1,2,I2,3,R8,8.0);
    VARPOW(UI1,2,I4,3,R8,8.0);
    VARPOW(UI1,2,R4,3.0f,R8,8.0);
    VARPOW(UI1,2,R8,3.0,R8,8.0);
    VARPOW(UI1,2,DATE,3,R8,8.0);
    VARPOW(UI1,2,BSTR,num3_str,R8,8.0);
    VARPOW(UI1,2,BOOL,VARIANT_FALSE,R8,1.0);
    VARPOW(UI1,2,I1,3,R8,8.0);
    VARPOW(UI1,2,UI1,3,R8,8.0);
    VARPOW(UI1,2,UI2,3,R8,8.0);
    VARPOW(UI1,2,UI4,3,R8,8.0);
    if (has_i8)
    {
        VARPOW(UI1,2,I8,3,R8,8.0);
        VARPOW(UI1,2,UI8,3,R8,8.0);
    }
    VARPOW(UI1,2,INT,3,R8,8.0);
    VARPOW(UI1,2,UINT,3,R8,8.0);
    VARPOW(UI2,2,EMPTY,0,R8,1.0);
    VARPOW(UI2,2,NULL,0,NULL,0);
    VARPOW(UI2,2,I2,3,R8,8.0);
    VARPOW(UI2,2,I4,3,R8,8.0);
    VARPOW(UI2,2,R4,3.0f,R8,8.0);
    VARPOW(UI2,2,R8,3.0,R8,8.0);
    VARPOW(UI2,2,DATE,3,R8,8.0);
    VARPOW(UI2,2,BSTR,num3_str,R8,8.0);
    VARPOW(UI2,2,BOOL,VARIANT_FALSE,R8,1.0);
    VARPOW(UI2,2,I1,3,R8,8.0);
    VARPOW(UI2,2,UI1,3,R8,8.0);
    VARPOW(UI2,2,UI2,3,R8,8.0);
    VARPOW(UI2,2,UI4,3,R8,8.0);
    if (has_i8)
    {
        VARPOW(UI2,2,I8,3,R8,8.0);
        VARPOW(UI2,2,UI8,3,R8,8.0);
    }
    VARPOW(UI2,2,INT,3,R8,8.0);
    VARPOW(UI2,2,UINT,3,R8,8.0);
    VARPOW(UI4,2,EMPTY,0,R8,1.0);
    VARPOW(UI4,2,NULL,0,NULL,0);
    VARPOW(UI4,2,I2,3,R8,8.0);
    VARPOW(UI4,2,I4,3,R8,8.0);
    VARPOW(UI4,2,R4,3.0f,R8,8.0);
    VARPOW(UI4,2,R8,3.0,R8,8.0);
    VARPOW(UI4,2,DATE,3,R8,8.0);
    VARPOW(UI4,2,BSTR,num3_str,R8,8.0);
    VARPOW(UI4,2,BOOL,VARIANT_FALSE,R8,1.0);
    VARPOW(UI4,2,I1,3,R8,8.0);
    VARPOW(UI4,2,UI1,3,R8,8.0);
    VARPOW(UI4,2,UI2,3,R8,8.0);
    VARPOW(UI4,2,UI4,3,R8,8.0);
    if (has_i8)
    {
        VARPOW(UI4,2,I8,3,R8,8.0);
        VARPOW(UI4,2,UI8,3,R8,8.0);
    }
    VARPOW(UI4,2,INT,3,R8,8.0);
    VARPOW(UI4,2,UINT,3,R8,8.0);
    if (has_i8)
    {
        VARPOW(I8,2,EMPTY,0,R8,1.0);
        VARPOW(I8,2,NULL,0,NULL,0);
        VARPOW(I8,2,I2,3,R8,8.0);
        VARPOW(I8,2,I4,3,R8,8.0);
        VARPOW(I8,2,R4,3.0f,R8,8.0);
        VARPOW(I8,2,R8,3.0,R8,8.0);
        VARPOW(I8,2,DATE,3,R8,8.0);
        VARPOW(I8,2,BSTR,num3_str,R8,8.0);
        VARPOW(I8,2,BOOL,VARIANT_FALSE,R8,1.0);
        VARPOW(I8,2,I1,3,R8,8.0);
        VARPOW(I8,2,UI1,3,R8,8.0);
        VARPOW(I8,2,UI2,3,R8,8.0);
        VARPOW(I8,2,UI4,3,R8,8.0);
        VARPOW(I8,2,I8,3,R8,8.0);
        VARPOW(I8,2,UI8,3,R8,8.0);
        VARPOW(I8,2,INT,3,R8,8.0);
        VARPOW(I8,2,UINT,3,R8,8.0);
        VARPOW(UI8,2,EMPTY,0,R8,1.0);
        VARPOW(UI8,2,NULL,0,NULL,0);
        VARPOW(UI8,2,I2,3,R8,8.0);
        VARPOW(UI8,2,I4,3,R8,8.0);
        VARPOW(UI8,2,R4,3.0f,R8,8.0);
        VARPOW(UI8,2,R8,3.0,R8,8.0);
        VARPOW(UI8,2,DATE,3,R8,8.0);
        VARPOW(UI8,2,BSTR,num3_str,R8,8.0);
        VARPOW(UI8,2,I1,3,R8,8.0);
        VARPOW(UI8,2,UI1,3,R8,8.0);
        VARPOW(UI8,2,UI2,3,R8,8.0);
        VARPOW(UI8,2,UI4,3,R8,8.0);
        VARPOW(UI8,2,I8,3,R8,8.0);
        VARPOW(UI8,2,UI8,3,R8,8.0);
        VARPOW(UI8,2,INT,3,R8,8.0);
        VARPOW(UI8,2,UINT,3,R8,8.0);
    }
    VARPOW(INT,2,EMPTY,0,R8,1.0);
    VARPOW(INT,2,NULL,0,NULL,0);
    VARPOW(INT,2,I2,3,R8,8.0);
    VARPOW(INT,2,I4,3,R8,8.0);
    VARPOW(INT,2,R4,3.0f,R8,8.0);
    VARPOW(INT,2,R8,3.0,R8,8.0);
    VARPOW(INT,2,DATE,3,R8,8.0);
    VARPOW(INT,2,BSTR,num3_str,R8,8.0);
    VARPOW(INT,2,BOOL,VARIANT_FALSE,R8,1.0);
    VARPOW(INT,2,I1,3,R8,8.0);
    VARPOW(INT,2,UI1,3,R8,8.0);
    VARPOW(INT,2,UI2,3,R8,8.0);
    VARPOW(INT,2,UI4,3,R8,8.0);
    if (has_i8)
    {
        VARPOW(INT,2,I8,3,R8,8.0);
        VARPOW(INT,2,UI8,3,R8,8.0);
    }
    VARPOW(INT,2,INT,3,R8,8.0);
    VARPOW(INT,2,UINT,3,R8,8.0);
    VARPOW(UINT,2,EMPTY,0,R8,1.0);
    VARPOW(UINT,2,NULL,0,NULL,0);
    VARPOW(UINT,2,I2,3,R8,8.0);
    VARPOW(UINT,2,I4,3,R8,8.0);
    VARPOW(UINT,2,R4,3.0f,R8,8.0);
    VARPOW(UINT,2,R8,3.0,R8,8.0);
    VARPOW(UINT,2,DATE,3,R8,8.0);
    VARPOW(UINT,2,BSTR,num3_str,R8,8.0);
    VARPOW(UINT,2,BOOL,VARIANT_FALSE,R8,1.0);
    VARPOW(UINT,2,I1,3,R8,8.0);
    VARPOW(UINT,2,UI1,3,R8,8.0);
    VARPOW(UINT,2,UI2,3,R8,8.0);
    VARPOW(UINT,2,UI4,3,R8,8.0);
    if (has_i8)
    {
        VARPOW(UINT,2,I8,3,R8,8.0);
        VARPOW(UINT,2,UI8,3,R8,8.0);
    }
    VARPOW(UINT,2,INT,3,R8,8.0);
    VARPOW(UINT,2,UINT,3,R8,8.0);

    /* Manually test some VT_CY, VT_DECIMAL variants */
    V_VT(&cy) = VT_CY;
    hres = VarCyFromI4(2, &V_CY(&cy));
    ok(hres == S_OK, "VarCyFromI4 failed!\n");
    V_VT(&dec) = VT_DECIMAL;
    hres = VarDecFromR8(2.0, &V_DECIMAL(&dec));
    ok(hres == S_OK, "VarDecFromR4 failed!\n");
    memset(&left, 0, sizeof(left));
    memset(&right, 0, sizeof(right));
    V_VT(&left) = VT_I4;
    V_I4(&left) = 100;
    V_VT(&right) = VT_I8;
    V_UI1(&right) = 2;

    hres = pVarPow(&cy, &cy, &result);
    ok(hres == S_OK && V_VT(&result) == VT_R8,
        "VARPOW: expected coerced hres 0x%lX type VT_R8, got hres 0x%lX type %s!\n",
        S_OK, hres, wine_dbgstr_vt(V_VT(&result)));
    ok(hres == S_OK && EQ_DOUBLE(V_R8(&result), 4.0),
        "VARPOW: CY value %f, expected %f\n", V_R8(&result), 4.0);

    hres = pVarPow(&cy, &right, &result);
    if (hres == S_OK)
    {
        ok(V_VT(&result) == VT_R8,
           "VARPOW: expected coerced hres 0x%lX type VT_R8, got hres 0x%lX type %s!\n",
           S_OK, hres, wine_dbgstr_vt(V_VT(&result)));
        ok(EQ_DOUBLE(V_R8(&result), 4.0),
           "VARPOW: CY value %f, expected %f\n", V_R8(&result), 4.0);
    }
    else
    {
        ok(hres == DISP_E_BADVARTYPE && V_VT(&result) == VT_EMPTY,
           "VARPOW: expected coerced hres 0x%lX type VT_EMPTY, got hres 0x%lX type %s!\n",
           DISP_E_BADVARTYPE, hres, wine_dbgstr_vt(V_VT(&result)));
    }

    hres = pVarPow(&left, &cy, &result);
    ok(hres == S_OK && V_VT(&result) == VT_R8,
        "VARPOW: expected coerced hres 0x%lX type VT_R8, got hres 0x%lX type %s!\n",
        S_OK, hres, wine_dbgstr_vt(V_VT(&result)));
    ok(hres == S_OK && EQ_DOUBLE(V_R8(&result), 10000.0),
        "VARPOW: CY value %f, expected %f\n", V_R8(&result), 10000.0);

    hres = pVarPow(&left, &dec, &result);
    ok(hres == S_OK && V_VT(&result) == VT_R8,
        "VARPOW: expected coerced hres 0x%lX type VT_R8, got hres 0x%lX type %s!\n",
        S_OK, hres, wine_dbgstr_vt(V_VT(&result)));
    ok(hres == S_OK && EQ_DOUBLE(V_R8(&result),10000.0),
        "VARPOW: DECIMAL value %f, expected %f\n", V_R8(&result), 10000.0);

    hres = pVarPow(&dec, &dec, &result);
    ok(hres == S_OK && V_VT(&result) == VT_R8,
        "VARPOW: expected coerced hres 0x%lX type VT_R8, got hres 0x%lX type %s!\n",
        S_OK, hres, wine_dbgstr_vt(V_VT(&result)));
    ok(hres == S_OK && EQ_DOUBLE(V_R8(&result), 4.0),
        "VARPOW: DECIMAL value %f, expected %f\n", V_R8(&result), 4.0);

    hres = pVarPow(&dec, &right, &result);
    if (hres == S_OK)
    {
        ok(V_VT(&result) == VT_R8,
           "VARPOW: expected coerced hres 0x%lX type VT_R8, got hres 0x%lX type %s!\n",
           S_OK, hres, wine_dbgstr_vt(V_VT(&result)));
        ok(EQ_DOUBLE(V_R8(&result), 4.0),
           "VARPOW: DECIMAL value %f, expected %f\n", V_R8(&result), 4.0);
    }
    else
    {
        ok(hres == DISP_E_BADVARTYPE && V_VT(&result) == VT_EMPTY,
           "VARPOW: expected coerced hres 0x%lX type VT_EMPTY, got hres 0x%lX type %s!\n",
           DISP_E_BADVARTYPE, hres, wine_dbgstr_vt(V_VT(&result)));
    }

    SysFreeString(num2_str);
    SysFreeString(num3_str);
}

static HRESULT (WINAPI *pVarDiv)(LPVARIANT,LPVARIANT,LPVARIANT);

#define VARDIV(vt1,val1,vt2,val2,rvt,rval)               \
        V_VT(&left) = VT_##vt1; V_##vt1(&left) = val1;   \
        V_VT(&right) = VT_##vt2; V_##vt2(&right) = val2; \
        V_VT(&exp) = VT_##rvt; V_##rvt(&exp) = rval;     \
        test_var_call2( __LINE__, pVarDiv, &left, &right, &exp )

/* Skip any type that is not defined or produces an error for every case */
#define SKIPTESTDIV(a)                            \
    if (a == VT_ERROR || a == VT_VARIANT ||       \
        a == VT_DISPATCH || a == VT_UNKNOWN ||    \
        a == VT_RECORD || a > VT_UINT ||          \
        a == VT_I1 || a == VT_UI8 ||              \
        a == VT_INT || a == VT_UINT ||            \
        a == VT_UI2 || a == VT_UI4 ||             \
        a == 15 /*not defined*/)                  \
        continue

static void test_VarDiv(void)
{
    static const WCHAR str1[] = { '1','\0' };
    static const WCHAR str2[] = { '2','\0' };
    VARIANT left, right, exp, result, cy, dec;
    BSTR num1_str, num2_str;
    VARTYPE i;
    HRESULT hres;
    double r;

    CHECKPTR(VarDiv);

    num1_str = SysAllocString(str1);
    num2_str = SysAllocString(str2);

    /* Test all possible flag/vt combinations & the resulting vt type */
    for (i = 0; i < ARRAY_SIZE(ExtraFlags); i++)
    {
        VARTYPE leftvt, rightvt, resvt;

        for (leftvt = 0; leftvt <= VT_BSTR_BLOB; leftvt++)
        {
            SKIPTESTDIV(leftvt);

            /* Check if we need/have support for I8 */
            if (leftvt == VT_I8 && !has_i8)
                continue;

            for (rightvt = 0; rightvt <= VT_BSTR_BLOB; rightvt++)
            {
                BOOL bFail = FALSE;
                SKIPTESTDIV(rightvt);

                /* Check if we need/have support for I8 */
                if (rightvt == VT_I8 && !has_i8)
                    continue;

                /* Native crashes with VT_BYREF */
                if (ExtraFlags[i] == VT_BYREF)
                    continue;

                memset(&left, 0, sizeof(left));
                memset(&right, 0, sizeof(right));
                V_VT(&left) = leftvt | ExtraFlags[i];
                V_VT(&right) = rightvt | ExtraFlags[i];
                V_VT(&result) = VT_EMPTY;
                resvt = VT_EMPTY;

                if (leftvt == VT_BSTR)
                    V_BSTR(&left) = num2_str;
                else if (leftvt == VT_DECIMAL)
                {
                    VarDecFromR8(2.0, &V_DECIMAL(&left));
                    V_VT(&left) = leftvt | ExtraFlags[i];
                }

                /* Division by 0 is undefined */
                switch(rightvt)
                {
                case VT_BSTR:
                    V_BSTR(&right) = num2_str;
                    break;
                case VT_DECIMAL:
                    VarDecFromR8(2.0, &V_DECIMAL(&right));
                    V_VT(&right) = rightvt | ExtraFlags[i];
                    break;
                case VT_BOOL:
                    V_BOOL(&right) = VARIANT_TRUE;
                    break;
                case VT_I2: V_I2(&right) = 2; break;
                case VT_I4: V_I4(&right) = 2; break;
                case VT_R4: V_R4(&right) = 2.0f; break;
                case VT_R8: V_R8(&right) = 2.0; break;
                case VT_CY: V_CY(&right).int64 = 2; break;
                case VT_DATE: V_DATE(&right) = 2; break;
                case VT_UI1: V_UI1(&right) = 2; break;
                case VT_I8: V_I8(&right) = 2; break;
                default: break;
                }

                /* Determine return type */
                if (rightvt != VT_EMPTY)
                {
                    if (leftvt == VT_NULL || rightvt == VT_NULL)
                        resvt = VT_NULL;
                    else if (leftvt == VT_DECIMAL || rightvt == VT_DECIMAL)
                        resvt = VT_DECIMAL;
                    else if (leftvt == VT_I8 || rightvt == VT_I8 ||
                        leftvt == VT_CY || rightvt == VT_CY ||
                        leftvt == VT_DATE || rightvt == VT_DATE ||
                        leftvt == VT_I4 || rightvt == VT_I4 ||
                        leftvt == VT_BSTR || rightvt == VT_BSTR ||
                        leftvt == VT_I2 || rightvt == VT_I2 ||
                        leftvt == VT_BOOL || rightvt == VT_BOOL ||
                        leftvt == VT_R8 || rightvt == VT_R8 ||
                        leftvt == VT_UI1 || rightvt == VT_UI1)
                    {
                        if ((leftvt == VT_UI1 && rightvt == VT_R4) ||
                            (leftvt == VT_R4 && rightvt == VT_UI1))
                            resvt = VT_R4;
                        else if ((leftvt == VT_R4 && (rightvt == VT_BOOL ||
                            rightvt == VT_I2)) || (rightvt == VT_R4 &&
                            (leftvt == VT_BOOL || leftvt == VT_I2)))
                            resvt = VT_R4;
                        else
                            resvt = VT_R8;
                    }
                    else if (leftvt == VT_R4 || rightvt == VT_R4)
                        resvt = VT_R4;
                }
                else if (leftvt == VT_NULL)
                    resvt = VT_NULL;
                else
                    bFail = TRUE;

                /* Native VarDiv always returns an error when using extra flags */
                if (ExtraFlags[i] != 0)
                    bFail = TRUE;

                hres = pVarDiv(&left, &right, &result);

                /* Check expected HRESULT and if result variant type is correct */
                if (bFail)
                    ok (hres == DISP_E_BADVARTYPE || hres == DISP_E_TYPEMISMATCH ||
                        hres == DISP_E_OVERFLOW || hres == DISP_E_DIVBYZERO,
                        "VarDiv: %s|0x%X, %s|0x%X: got vt %s hr 0x%lX\n",
                        wine_dbgstr_vt(leftvt), ExtraFlags[i], wine_dbgstr_vt(rightvt), ExtraFlags[i],
                        wine_dbgstr_vt(V_VT(&result)), hres);
                else
                    ok (hres == S_OK && resvt == V_VT(&result),
                        "VarDiv: %s|0x%X, %s|0x%X: expected vt %s hr 0x%lX, got vt %s hr 0x%lX\n",
                        wine_dbgstr_vt(leftvt), ExtraFlags[i], wine_dbgstr_vt(rightvt), ExtraFlags[i], wine_dbgstr_vt(resvt),
                        S_OK, wine_dbgstr_vt(V_VT(&result)), hres);
            }
        }
    }

    /* Test return values for all the good cases */
    VARDIV(EMPTY,0,NULL,0,NULL,0);
    VARDIV(EMPTY,0,I2,2,R8,0.0);
    VARDIV(EMPTY,0,I4,2,R8,0.0);
    VARDIV(EMPTY,0,R4,2.0f,R4,0.0f);
    VARDIV(EMPTY,0,R8,2.0,R8,0.0);
    VARDIV(EMPTY,0,DATE,2.0,R8,0.0);
    VARDIV(EMPTY,0,BSTR,num2_str,R8,0.0);
    VARDIV(EMPTY,0,BOOL,VARIANT_TRUE,R8,0.0);
    VARDIV(EMPTY,0,UI1,2,R8,0.0);
    if (has_i8)
    {
        VARDIV(EMPTY,0,I8,2,R8,0.0);
    }
    VARDIV(NULL,0,EMPTY,0,NULL,0);
    VARDIV(NULL,0,NULL,0,NULL,0);
    VARDIV(NULL,0,I2,2,NULL,0);
    VARDIV(NULL,0,I4,2,NULL,0);
    VARDIV(NULL,0,R4,2.0f,NULL,0);
    VARDIV(NULL,0,R8,2.0,NULL,0);
    VARDIV(NULL,0,DATE,2,NULL,0);
    VARDIV(NULL,0,BSTR,num2_str,NULL,0);
    VARDIV(NULL,0,BOOL,VARIANT_TRUE,NULL,0);
    VARDIV(NULL,0,UI1,2,NULL,0);
    if (has_i8)
    {
        VARDIV(NULL,0,I8,2,NULL,0);
    }
    VARDIV(I2,2,NULL,0,NULL,0);
    VARDIV(I2,1,I2,2,R8,0.5);
    VARDIV(I2,1,I4,2,R8,0.5);
    VARDIV(I2,1,R4,2,R4,0.5f);
    VARDIV(I2,1,R8,2.0,R8,0.5);
    VARDIV(I2,1,DATE,2,R8,0.5);
    VARDIV(I2,1,BOOL,VARIANT_TRUE,R8,-1.0);
    VARDIV(I2,1,UI1,2,R8,0.5);
    if (has_i8)
    {
        VARDIV(I2,1,I8,2,R8,0.5);
    }
    VARDIV(I4,1,NULL,0,NULL,0);
    VARDIV(I4,1,I2,2,R8,0.5);
    VARDIV(I4,1,I4,2,R8,0.5);
    VARDIV(I4,1,R4,2.0f,R8,0.5);
    VARDIV(I4,1,R8,2.0,R8,0.5);
    VARDIV(I4,1,DATE,2,R8,0.5);
    VARDIV(I4,1,BSTR,num2_str,R8,0.5);
    VARDIV(I4,1,BOOL,VARIANT_TRUE,R8,-1.0);
    VARDIV(I4,1,UI1,2,R8,0.5);
    if (has_i8)
    {
        VARDIV(I4,1,I8,2,R8,0.5);
    }
    VARDIV(R4,1.0f,NULL,0,NULL,0);
    VARDIV(R4,1.0f,I2,2,R4,0.5f);
    VARDIV(R4,1.0f,I4,2,R8,0.5);
    VARDIV(R4,1.0f,R4,2.0f,R4,0.5f);
    VARDIV(R4,1.0f,R8,2.0,R8,0.5);
    VARDIV(R4,1.0f,DATE,2,R8,0.5);
    VARDIV(R4,1.0f,BSTR,num2_str,R8,0.5);
    VARDIV(R4,1.0f,BOOL,VARIANT_TRUE,R4,-1);
    VARDIV(R4,1.0f,UI1,2,R4,0.5f);
    if (has_i8)
    {
        VARDIV(R4,1.0f,I8,2,R8,0.5);
    }
    VARDIV(R8,1.0,NULL,0,NULL,0);
    VARDIV(R8,1.0,I2,2,R8,0.5);
    VARDIV(R8,1.0,I4,2,R8,0.5);
    VARDIV(R8,1.0,R4,2.0f,R8,0.5);
    VARDIV(R8,1.0,R8,2.0,R8,0.5);
    VARDIV(R8,1.0,DATE,2,R8,0.5);
    VARDIV(R8,1.0,BSTR,num2_str,R8,0.5);
    VARDIV(R8,1.0,BOOL,VARIANT_TRUE,R8,-1.0);
    VARDIV(R8,1.0,UI1,2,R8,0.5);
    if (has_i8)
    {
        VARDIV(R8,1.0,I8,2,R8,0.5);
    }
    VARDIV(DATE,1,NULL,0,NULL,0);
    VARDIV(DATE,1,I2,2,R8,0.5);
    VARDIV(DATE,1,I4,2,R8,0.5);
    VARDIV(DATE,1,R4,2.0f,R8,0.5);
    VARDIV(DATE,1,R8,2.0,R8,0.5);
    VARDIV(DATE,1,DATE,2,R8,0.5);
    VARDIV(DATE,1,BSTR,num2_str,R8,0.5);
    VARDIV(DATE,1,BOOL,VARIANT_TRUE,R8,-1.0);
    VARDIV(DATE,1,UI1,2,R8,0.5);
    if (has_i8)
    {
        VARDIV(DATE,1,I8,2,R8,0.5);
    }
    VARDIV(BSTR,num1_str,NULL,0,NULL,0);
    VARDIV(BSTR,num1_str,I2,2,R8,0.5);
    VARDIV(BSTR,num1_str,I4,2,R8,0.5);
    VARDIV(BSTR,num1_str,R4,2.0f,R8,0.5);
    VARDIV(BSTR,num1_str,R8,2.0,R8,0.5);
    VARDIV(BSTR,num1_str,DATE,2,R8,0.5);
    VARDIV(BSTR,num1_str,BSTR,num2_str,R8,0.5);
    VARDIV(BSTR,num1_str,BOOL,VARIANT_TRUE,R8,-1);
    VARDIV(BSTR,num1_str,UI1,2,R8,0.5);
    if (has_i8)
    {
        VARDIV(BSTR,num1_str,I8,2,R8,0.5);
    }
    VARDIV(BOOL,VARIANT_TRUE,NULL,0,NULL,0);
    VARDIV(BOOL,VARIANT_TRUE,I2,1,R8,-1.0);
    VARDIV(BOOL,VARIANT_FALSE,I2,1,R8,0.0);
    VARDIV(BOOL,VARIANT_TRUE,I4,1,R8,-1.0);
    VARDIV(BOOL,VARIANT_FALSE,I4,1,R8,0.0);
    VARDIV(BOOL,VARIANT_TRUE,R4,1,R4,-1.0f);
    VARDIV(BOOL,VARIANT_FALSE,R4,1,R4,0.0f);
    VARDIV(BOOL,VARIANT_TRUE,R8,1.0,R8,-1.0);
    VARDIV(BOOL,VARIANT_FALSE,R8,1.0,R8,0.0);
    VARDIV(BOOL,VARIANT_FALSE,DATE,2,R8,0.0);
    VARDIV(BOOL,VARIANT_FALSE,BSTR,num2_str,R8,0.0);
    VARDIV(BOOL,VARIANT_TRUE,BOOL,VARIANT_TRUE,R8,1.0);
    VARDIV(BOOL,VARIANT_FALSE,BOOL,VARIANT_TRUE,R8,0.0);
    VARDIV(BOOL,VARIANT_TRUE,UI1,1,R8,-1.0);
    if (has_i8)
    {
        VARDIV(BOOL,VARIANT_TRUE,I8,1,R8,-1.0);
    }
    VARDIV(UI1,1,NULL,0,NULL,0);
    VARDIV(UI1,1,I2,2,R8,0.5);
    VARDIV(UI1,1,I4,2,R8,0.5);
    VARDIV(UI1,1,R4,2.0f,R4,0.5f);
    VARDIV(UI1,1,R8,2.0,R8,0.5);
    VARDIV(UI1,1,DATE,2,R8,0.5);
    VARDIV(UI1,1,BSTR,num2_str,R8,0.5);
    VARDIV(UI1,1,BOOL,VARIANT_TRUE,R8,-1);
    VARDIV(UI1,1,UI1,2,R8,0.5);
    if (has_i8)
    {
        VARDIV(UI1,1,I8,2,R8,0.5);
        VARDIV(I8,1,NULL,0,NULL,0);
        VARDIV(I8,1,I2,2,R8,0.5);
        VARDIV(I8,1,I4,2,R8,0.5);
        VARDIV(I8,1,R4,2.0f,R8,0.5);
        VARDIV(I8,1,R8,2.0,R8,0.5);
        VARDIV(I8,1,DATE,2,R8,0.5);
        VARDIV(I8,1,BSTR,num2_str,R8,0.5);
        VARDIV(I8,1,BOOL,VARIANT_TRUE,R8,-1);
        VARDIV(I8,1,UI1,2,R8,0.5);
        VARDIV(I8,1,I8,2,R8,0.5);
    }

    /* Manually test some VT_CY, VT_DECIMAL variants */
    V_VT(&cy) = VT_CY;
    hres = VarCyFromI4(10000, &V_CY(&cy));
    ok(hres == S_OK, "VarCyFromI4 failed!\n");
    V_VT(&dec) = VT_DECIMAL;
    hres = VarDecFromR8(2.0, &V_DECIMAL(&dec));
    ok(hres == S_OK, "VarDecFromR4 failed!\n");
    memset(&left, 0, sizeof(left));
    memset(&right, 0, sizeof(right));
    V_VT(&left) = VT_I4;
    V_I4(&left) = 100;
    V_VT(&right) = VT_UI1;
    V_UI1(&right) = 2;

    hres = pVarDiv(&cy, &cy, &result);
    ok(hres == S_OK && V_VT(&result) == VT_R8,
        "VARDIV: expected coerced type VT_R8, got %s!\n", wine_dbgstr_vt(V_VT(&result)));
    ok(hres == S_OK && EQ_DOUBLE(V_R8(&result), 1.0),
        "VARDIV: CY value %f, expected %f\n", V_R8(&result), 1.0);

    hres = pVarDiv(&cy, &right, &result);
    ok(hres == S_OK && V_VT(&result) == VT_R8,
        "VARDIV: expected coerced type VT_R8, got %s!\n", wine_dbgstr_vt(V_VT(&result)));
    ok(hres == S_OK && EQ_DOUBLE(V_R8(&result), 5000.0),
        "VARDIV: CY value %f, expected %f\n", V_R8(&result), 5000.0);

    hres = pVarDiv(&left, &cy, &result);
    ok(hres == S_OK && V_VT(&result) == VT_R8,
        "VARDIV: expected coerced type VT_R8, got %s!\n", wine_dbgstr_vt(V_VT(&result)));
    ok(hres == S_OK && EQ_DOUBLE(V_R8(&result), 0.01),
        "VARDIV: CY value %f, expected %f\n", V_R8(&result), 0.01);

    hres = pVarDiv(&left, &dec, &result);
    ok(hres == S_OK && V_VT(&result) == VT_DECIMAL,
        "VARDIV: expected coerced type VT_DECIMAL, got %s!\n", wine_dbgstr_vt(V_VT(&result)));
    hres = VarR8FromDec(&V_DECIMAL(&result), &r);
    ok(hres == S_OK && EQ_DOUBLE(r, 50.0), "VARDIV: DECIMAL value %f, expected %f\n", r, 50.0);

    hres = pVarDiv(&dec, &dec, &result);
    ok(hres == S_OK && V_VT(&result) == VT_DECIMAL,
        "VARDIV: expected coerced type VT_DECIMAL, got %s!\n", wine_dbgstr_vt(V_VT(&result)));
    hres = VarR8FromDec(&V_DECIMAL(&result), &r);
    ok(hres == S_OK && EQ_DOUBLE(r, 1.0), "VARDIV: DECIMAL value %f, expected %f\n", r, 1.0);

    hres = pVarDiv(&dec, &right, &result);
    ok(hres == S_OK && V_VT(&result) == VT_DECIMAL,
        "VARDIV: expected coerced type VT_DECIMAL, got %s!\n", wine_dbgstr_vt(V_VT(&result)));
    hres = VarR8FromDec(&V_DECIMAL(&result), &r);
    ok(hres == S_OK && EQ_DOUBLE(r, 1.0), "VARDIV: DECIMAL value %f, expected %f\n", r, 1.0);

    /* Check for division by zero and overflow */
    V_VT(&left) = VT_R8;
    V_I4(&left) = 1;
    V_VT(&right) = VT_R8;
    V_I4(&right) = 0;
    hres = pVarDiv(&left, &right, &result);
    ok(hres == DISP_E_DIVBYZERO && V_VT(&result) == VT_EMPTY,
        "VARDIV: Division by (1.0/0.0) should result in DISP_E_DIVBYZERO but got 0x%lX\n", hres);

    V_VT(&left) = VT_R8;
    V_I4(&left) = 0;
    V_VT(&right) = VT_R8;
    V_I4(&right) = 0;
    hres = pVarDiv(&left, &right, &result);
    ok(hres == DISP_E_OVERFLOW && V_VT(&result) == VT_EMPTY,
        "VARDIV: Division by (0.0/0.0) should result in DISP_E_OVERFLOW but got 0x%lX\n", hres);

    SysFreeString(num1_str);
    SysFreeString(num2_str);
}

static HRESULT (WINAPI *pVarIdiv)(LPVARIANT,LPVARIANT,LPVARIANT);

#define VARIDIV(vt1,val1,vt2,val2,rvt,rval)              \
        V_VT(&left) = VT_##vt1; V_##vt1(&left) = val1;   \
        V_VT(&right) = VT_##vt2; V_##vt2(&right) = val2; \
        V_VT(&exp) = VT_##rvt; V_##rvt(&exp) = rval;     \
        test_var_call2( __LINE__, pVarIdiv, &left, &right, &exp )

/* Skip any type that is not defined or produces an error for every case */
#define SKIPTESTIDIV(a)                           \
    if (a == VT_ERROR || a == VT_VARIANT ||       \
        a == VT_DISPATCH || a == VT_UNKNOWN ||    \
        a == VT_RECORD || a > VT_UINT ||          \
        a == 15 /*not defined*/)                  \
        continue

static void test_VarIdiv(void)
{
    VARIANT left, right, exp, result, cy, dec;
    BSTR num1_str, num2_str;
    VARTYPE i;
    HRESULT hres;

    CHECKPTR(VarIdiv);

    num1_str = SysAllocString(L"1");
    num2_str = SysAllocString(L"2");

    /* Test all possible flag/vt combinations & the resulting vt type */
    for (i = 0; i < ARRAY_SIZE(ExtraFlags); i++)
    {
        VARTYPE leftvt, rightvt, resvt;

        for (leftvt = 0; leftvt <= VT_BSTR_BLOB; leftvt++)
        {
            SKIPTESTIDIV(leftvt);

            /* Check if we need/have support for I8 and/or UI8 */
            if ((leftvt == VT_I8 || leftvt == VT_UI8) && !has_i8)
                continue;

            for (rightvt = 0; rightvt <= VT_BSTR_BLOB; rightvt++)
            {
                BOOL bFail = FALSE;
                SKIPTESTIDIV(rightvt);

                /* Native crashes with extra flag VT_BYREF */
                if (ExtraFlags[i] == VT_BYREF)
                    continue;

                /* Check if we need/have support for I8 and/or UI8 */
                if ((rightvt == VT_I8 || rightvt == VT_UI8) && !has_i8)
                    continue;

                memset(&left, 0, sizeof(left));
                memset(&right, 0, sizeof(right));
                V_VT(&left) = leftvt | ExtraFlags[i];
                V_VT(&right) = rightvt | ExtraFlags[i];
                V_VT(&result) = VT_EMPTY;
                resvt = VT_EMPTY;

                if (leftvt == VT_BSTR)
                    V_BSTR(&left) = num2_str;
                else if (leftvt == VT_DECIMAL)
                {
                    VarDecFromR8(2.0, &V_DECIMAL(&left));
                    V_VT(&left) = leftvt | ExtraFlags[i];
                }

                /* Division by 0 is undefined */
                switch(rightvt)
                {
                case VT_BSTR:
                    V_BSTR(&right) = num2_str;
                    break;
                case VT_DECIMAL:
                    VarDecFromR8(2.0, &V_DECIMAL(&right));
                    V_VT(&right) = rightvt | ExtraFlags[i];
                    break;
                case VT_BOOL:
                    V_BOOL(&right) = VARIANT_TRUE;
                    break;
                case VT_CY:
                    VarCyFromI4(10000, &V_CY(&right));
                    V_VT(&right) = rightvt | ExtraFlags[i];
                    break;
                case VT_I2: V_I2(&right) = 2; break;
                case VT_I4: V_I4(&right) = 2; break;
                case VT_R4: V_R4(&right) = 2.0f; break;
                case VT_R8: V_R8(&right) = 2.0; break;
                case VT_DATE: V_DATE(&right) = 2; break;
                case VT_I1: V_I1(&right) = 2; break;
                case VT_UI1: V_UI1(&right) = 2; break;
                case VT_UI2: V_UI2(&right) = 2; break;
                case VT_UI4: V_UI4(&right) = 2; break;
                case VT_I8: V_I8(&right) = 2; break;
                case VT_UI8: V_UI8(&right) = 2; break;
                case VT_INT: V_INT(&right) = 2; break;
                case VT_UINT: V_UINT(&right) = 2; break;
                default: break;
                }

                /* Native VarIdiv always returns an error when using extra
                 * flags or if the variant combination is I8 and INT.
                 */
                if ((leftvt == VT_I8 && rightvt == VT_INT) ||
                    (leftvt == VT_INT && rightvt == VT_I8) ||
                    (rightvt == VT_EMPTY && leftvt != VT_NULL) ||
                    ExtraFlags[i] != 0)
                    bFail = TRUE;

                /* Determine variant type */
                else if (leftvt == VT_NULL || rightvt == VT_NULL)
                    resvt = VT_NULL;
                else if (leftvt == VT_I8 || rightvt == VT_I8)
                    resvt = VT_I8;
                else if (leftvt == VT_I4 || rightvt == VT_I4 ||
                    leftvt == VT_INT || rightvt == VT_INT ||
                    leftvt == VT_UINT || rightvt == VT_UINT ||
                    leftvt == VT_UI8 || rightvt == VT_UI8 ||
                    leftvt == VT_UI4 || rightvt == VT_UI4 ||
                    leftvt == VT_UI2 || rightvt == VT_UI2 ||
                    leftvt == VT_I1 || rightvt == VT_I1 ||
                    leftvt == VT_BSTR || rightvt == VT_BSTR ||
                    leftvt == VT_DATE || rightvt == VT_DATE ||
                    leftvt == VT_CY || rightvt == VT_CY ||
                    leftvt == VT_DECIMAL || rightvt == VT_DECIMAL ||
                    leftvt == VT_R8 || rightvt == VT_R8 ||
                    leftvt == VT_R4 || rightvt == VT_R4)
                    resvt = VT_I4;
                else if (leftvt == VT_I2 || rightvt == VT_I2 ||
                    leftvt == VT_BOOL || rightvt == VT_BOOL ||
                    leftvt == VT_EMPTY)
                    resvt = VT_I2;
                else if (leftvt == VT_UI1 || rightvt == VT_UI1)
                    resvt = VT_UI1;
                else
                    bFail = TRUE;

                hres = pVarIdiv(&left, &right, &result);

                /* Check expected HRESULT and if result variant type is correct */
                if (bFail)
                    ok (hres == DISP_E_BADVARTYPE || hres == DISP_E_TYPEMISMATCH ||
                        hres == DISP_E_DIVBYZERO,
                        "VarIdiv: %s|0x%X, %s|0x%X: got vt %s hr 0x%lX\n",
                        wine_dbgstr_vt(leftvt), ExtraFlags[i], wine_dbgstr_vt(rightvt), ExtraFlags[i],
                        wine_dbgstr_vt(V_VT(&result)), hres);
                else
                    ok (hres == S_OK && resvt == V_VT(&result),
                        "VarIdiv: %s|0x%X, %s|0x%X: expected vt %s hr 0x%lX, got vt %s hr 0x%lX\n",
                        wine_dbgstr_vt(leftvt), ExtraFlags[i], wine_dbgstr_vt(rightvt), ExtraFlags[i], wine_dbgstr_vt(resvt),
                        S_OK, wine_dbgstr_vt(V_VT(&result)), hres);
            }
        }
    }

    /* Test return values for all the good cases */
    VARIDIV(EMPTY,0,NULL,0,NULL,0);
    VARIDIV(EMPTY,0,I2,1,I2,0);
    VARIDIV(EMPTY,0,I4,1,I4,0);
    VARIDIV(EMPTY,0,R4,1.0f,I4,0);
    VARIDIV(EMPTY,0,R8,1.0,I4,0);
    VARIDIV(EMPTY,0,DATE,1.0,I4,0);
    VARIDIV(EMPTY,0,BSTR,num1_str,I4,0);
    VARIDIV(EMPTY,0,BOOL,VARIANT_TRUE,I2,0);
    VARIDIV(EMPTY,0,I1,1,I4,0);
    VARIDIV(EMPTY,0,UI1,1,I2,0);
    VARIDIV(EMPTY,0,UI2,1,I4,0);
    VARIDIV(EMPTY,0,UI4,1,I4,0);
    if (has_i8)
    {
        VARIDIV(EMPTY,0,I8,1,I8,0);
        VARIDIV(EMPTY,0,UI8,1,I4,0);
    }
    VARIDIV(EMPTY,0,INT,1,I4,0);
    VARIDIV(EMPTY,0,UINT,1,I4,0);
    VARIDIV(NULL,0,EMPTY,0,NULL,0);
    VARIDIV(NULL,0,NULL,0,NULL,0);
    VARIDIV(NULL,0,I2,1,NULL,0);
    VARIDIV(NULL,0,I4,1,NULL,0);
    VARIDIV(NULL,0,R4,1,NULL,0);
    VARIDIV(NULL,0,R8,1,NULL,0);
    VARIDIV(NULL,0,DATE,1,NULL,0);
    VARIDIV(NULL,0,BSTR,num1_str,NULL,0);
    VARIDIV(NULL,0,BOOL,VARIANT_TRUE,NULL,0);
    VARIDIV(NULL,0,I1,1,NULL,0);
    VARIDIV(NULL,0,UI1,1,NULL,0);
    VARIDIV(NULL,0,UI2,1,NULL,0);
    VARIDIV(NULL,0,UI4,1,NULL,0);
    if (has_i8)
    {
        VARIDIV(NULL,0,I8,1,NULL,0);
        VARIDIV(NULL,0,UI8,1,NULL,0);
    }
    VARIDIV(NULL,0,INT,1,NULL,0);
    VARIDIV(NULL,0,UINT,1,NULL,0);
    VARIDIV(I2,2,NULL,0,NULL,0);
    VARIDIV(I2,2,I2,1,I2,2);
    VARIDIV(I2,2,I4,1,I4,2);
    VARIDIV(I2,2,R4,1,I4,2);
    VARIDIV(I2,2,R8,1,I4,2);
    VARIDIV(I2,2,DATE,1,I4,2);
    VARIDIV(I2,2,BSTR,num1_str,I4,2);
    VARIDIV(I2,2,BOOL,VARIANT_TRUE,I2,-2);
    VARIDIV(I2,2,I1,1,I4,2);
    VARIDIV(I2,2,UI1,1,I2,2);
    VARIDIV(I2,2,UI2,1,I4,2);
    VARIDIV(I2,2,UI4,1,I4,2);
    if (has_i8)
    {
        VARIDIV(I2,2,I8,1,I8,2);
        VARIDIV(I2,2,UI8,1,I4,2);
    }
    VARIDIV(I2,2,INT,1,I4,2);
    VARIDIV(I2,2,UINT,1,I4,2);
    VARIDIV(I4,2,NULL,0,NULL,0);
    VARIDIV(I4,2,I2,1,I4,2);
    VARIDIV(I4,2,I4,1,I4,2);
    VARIDIV(I4,2,R4,1,I4,2);
    VARIDIV(I4,2,R8,1,I4,2);
    VARIDIV(I4,2,DATE,1,I4,2);
    VARIDIV(I4,2,BSTR,num1_str,I4,2);
    VARIDIV(I4,2,BOOL,VARIANT_TRUE,I4,-2);
    VARIDIV(I4,2,I1,1,I4,2);
    VARIDIV(I4,2,UI1,1,I4,2);
    VARIDIV(I4,2,UI2,1,I4,2);
    VARIDIV(I4,2,UI4,1,I4,2);
    if (has_i8)
    {
        VARIDIV(I4,2,I8,1,I8,2);
        VARIDIV(I4,2,UI8,1,I4,2);
    }
    VARIDIV(I4,2,INT,1,I4,2);
    VARIDIV(I4,2,UINT,1,I4,2);
    VARIDIV(R4,2.0f,NULL,0,NULL,0);
    VARIDIV(R4,2.0f,I2,1,I4,2);
    VARIDIV(R4,2.0f,I4,1,I4,2);
    VARIDIV(R4,2.0f,R4,1.0f,I4,2);
    VARIDIV(R4,2.0f,R8,1.0,I4,2);
    VARIDIV(R4,2.0f,DATE,1,I4,2);
    VARIDIV(R4,2.0f,BSTR,num1_str,I4,2);
    VARIDIV(R4,2.0f,BOOL,VARIANT_TRUE,I4,-2);
    VARIDIV(R4,2.0f,I1,1,I4,2);
    VARIDIV(R4,2.0f,UI1,1,I4,2);
    VARIDIV(R4,2.0f,UI2,1,I4,2);
    VARIDIV(R4,2.0f,UI4,1,I4,2);
    if (has_i8)
    {
        VARIDIV(R4,2.0f,I8,1,I8,2);
        VARIDIV(R4,2.0f,UI8,1,I4,2);
    }
    VARIDIV(R4,2.0f,INT,1,I4,2);
    VARIDIV(R4,2.0f,UINT,1,I4,2);
    VARIDIV(R8,2.0,NULL,0,NULL,0);
    VARIDIV(R8,2.0,I2,1,I4,2);
    VARIDIV(R8,2.0,I4,1,I4,2);
    VARIDIV(R8,2.0,R4,1,I4,2);
    VARIDIV(R8,2.0,R8,1,I4,2);
    VARIDIV(R8,2.0,DATE,1,I4,2);
    VARIDIV(R8,2.0,BSTR,num1_str,I4,2);
    VARIDIV(R8,2.0,BOOL,VARIANT_TRUE,I4,-2);
    VARIDIV(R8,2.0,I1,1,I4,2);
    VARIDIV(R8,2.0,UI1,1,I4,2);
    VARIDIV(R8,2.0,UI2,1,I4,2);
    VARIDIV(R8,2.0,UI4,1,I4,2);
    if (has_i8)
    {
        VARIDIV(R8,2.0,I8,1,I8,2);
        VARIDIV(R8,2.0,UI8,1,I4,2);
    }
    VARIDIV(R8,2.0,INT,1,I4,2);
    VARIDIV(R8,2.0,UINT,1,I4,2);
    VARIDIV(DATE,2,NULL,0,NULL,0);
    VARIDIV(DATE,2,I2,1,I4,2);
    VARIDIV(DATE,2,I4,1,I4,2);
    VARIDIV(DATE,2,R4,1,I4,2);
    VARIDIV(DATE,2,R8,1,I4,2);
    VARIDIV(DATE,2,DATE,1,I4,2);
    VARIDIV(DATE,2,BSTR,num1_str,I4,2);
    VARIDIV(DATE,2,BOOL,VARIANT_TRUE,I4,-2);
    VARIDIV(DATE,2,I1,1,I4,2);
    VARIDIV(DATE,2,UI1,1,I4,2);
    VARIDIV(DATE,2,UI2,1,I4,2);
    VARIDIV(DATE,2,UI4,1,I4,2);
    if (has_i8)
    {
        VARIDIV(DATE,2,I8,1,I8,2);
        VARIDIV(DATE,2,UI8,1,I4,2);
    }
    VARIDIV(DATE,2,INT,1,I4,2);
    VARIDIV(DATE,2,UINT,1,I4,2);
    VARIDIV(BSTR,num2_str,NULL,0,NULL,0);
    VARIDIV(BSTR,num2_str,I2,1,I4,2);
    VARIDIV(BSTR,num2_str,I4,1,I4,2);
    VARIDIV(BSTR,num2_str,R4,1.0f,I4,2);
    VARIDIV(BSTR,num2_str,R8,1.0,I4,2);
    VARIDIV(BSTR,num2_str,DATE,1,I4,2);
    VARIDIV(BSTR,num2_str,BSTR,num1_str,I4,2);
    VARIDIV(BSTR,num2_str,BOOL,VARIANT_TRUE,I4,-2);
    VARIDIV(BSTR,num2_str,I1,1,I4,2);
    VARIDIV(BSTR,num2_str,UI1,1,I4,2);
    VARIDIV(BSTR,num2_str,UI2,1,I4,2);
    VARIDIV(BSTR,num2_str,UI4,1,I4,2);
    if (has_i8)
    {
        VARIDIV(BSTR,num2_str,I8,1,I8,2);
        VARIDIV(BSTR,num2_str,UI8,1,I4,2);
    }
    VARIDIV(BSTR,num2_str,INT,1,I4,2);
    VARIDIV(BSTR,num2_str,UINT,1,I4,2);
    VARIDIV(BOOL,VARIANT_TRUE,NULL,0,NULL,0);
    VARIDIV(BOOL,VARIANT_TRUE,I2,1,I2,-1);
    VARIDIV(BOOL,VARIANT_TRUE,I4,1,I4,-1);
    VARIDIV(BOOL,VARIANT_TRUE,R4,1.0f,I4,-1);
    VARIDIV(BOOL,VARIANT_TRUE,R8,1.0,I4,-1);
    VARIDIV(BOOL,VARIANT_TRUE,DATE,1,I4,-1);
    VARIDIV(BOOL,VARIANT_TRUE,BSTR,num1_str,I4,-1);
    VARIDIV(BOOL,VARIANT_TRUE,BOOL,VARIANT_TRUE,I2,1);
    VARIDIV(BOOL,VARIANT_TRUE,I1,1,I4,-1);
    VARIDIV(BOOL,VARIANT_TRUE,UI1,1,I2,-1);
    VARIDIV(BOOL,VARIANT_TRUE,UI2,1,I4,-1);
    VARIDIV(BOOL,VARIANT_TRUE,UI4,1,I4,-1);
    if (has_i8)
    {
        VARIDIV(BOOL,VARIANT_TRUE,I8,1,I8,-1);
        VARIDIV(BOOL,VARIANT_TRUE,UI8,1,I4,-1);
    }
    VARIDIV(BOOL,VARIANT_TRUE,INT,1,I4,-1);
    VARIDIV(BOOL,VARIANT_TRUE,UINT,1,I4,-1);
    VARIDIV(I1,2,NULL,0,NULL,0);
    VARIDIV(I1,2,I2,1,I4,2);
    VARIDIV(I1,2,I4,1,I4,2);
    VARIDIV(I1,2,R4,1.0f,I4,2);
    VARIDIV(I1,2,R8,1.0,I4,2);
    VARIDIV(I1,2,DATE,1,I4,2);
    VARIDIV(I1,2,BSTR,num1_str,I4,2);
    VARIDIV(I1,2,BOOL,VARIANT_TRUE,I4,-2);
    VARIDIV(I1,2,I1,1,I4,2);
    VARIDIV(I1,2,UI1,1,I4,2);
    VARIDIV(I1,2,UI2,1,I4,2);
    VARIDIV(I1,2,UI4,1,I4,2);
    if (has_i8)
    {
        VARIDIV(I1,2,I8,1,I8,2);
        VARIDIV(I1,2,UI8,1,I4,2);
    }
    VARIDIV(I1,2,INT,1,I4,2);
    VARIDIV(I1,2,UINT,1,I4,2);
    VARIDIV(UI1,2,NULL,0,NULL,0);
    VARIDIV(UI1,2,I2,1,I2,2);
    VARIDIV(UI1,2,I4,1,I4,2);
    VARIDIV(UI1,2,R4,1.0f,I4,2);
    VARIDIV(UI1,2,R8,1.0,I4,2);
    VARIDIV(UI1,2,DATE,1,I4,2);
    VARIDIV(UI1,2,BSTR,num1_str,I4,2);
    VARIDIV(UI1,2,BOOL,VARIANT_TRUE,I2,-2);
    VARIDIV(UI1,2,I1,1,I4,2);
    VARIDIV(UI1,2,UI1,1,UI1,2);
    VARIDIV(UI1,2,UI2,1,I4,2);
    VARIDIV(UI1,2,UI4,1,I4,2);
    if (has_i8)
    {
        VARIDIV(UI1,2,I8,1,I8,2);
        VARIDIV(UI1,2,UI8,1,I4,2);
    }
    VARIDIV(UI1,2,INT,1,I4,2);
    VARIDIV(UI1,2,UINT,1,I4,2);
    VARIDIV(UI2,2,NULL,0,NULL,0);
    VARIDIV(UI2,2,I2,1,I4,2);
    VARIDIV(UI2,2,I4,1,I4,2);
    VARIDIV(UI2,2,R4,1.0f,I4,2);
    VARIDIV(UI2,2,R8,1.0,I4,2);
    VARIDIV(UI2,2,DATE,1,I4,2);
    VARIDIV(UI2,2,BSTR,num1_str,I4,2);
    VARIDIV(UI2,2,BOOL,VARIANT_TRUE,I4,-2);
    VARIDIV(UI2,2,I1,1,I4,2);
    VARIDIV(UI2,2,UI1,1,I4,2);
    VARIDIV(UI2,2,UI2,1,I4,2);
    VARIDIV(UI2,2,UI4,1,I4,2);
    if (has_i8)
    {
        VARIDIV(UI2,2,I8,1,I8,2);
        VARIDIV(UI2,2,UI8,1,I4,2);
    }
    VARIDIV(UI2,2,INT,1,I4,2);
    VARIDIV(UI2,2,UINT,1,I4,2);
    VARIDIV(UI4,2,NULL,0,NULL,0);
    VARIDIV(UI4,2,I2,1,I4,2);
    VARIDIV(UI4,2,I4,1,I4,2);
    VARIDIV(UI4,2,R4,1.0f,I4,2);
    VARIDIV(UI4,2,R8,1.0,I4,2);
    VARIDIV(UI4,2,DATE,1,I4,2);
    VARIDIV(UI4,2,BSTR,num1_str,I4,2);
    VARIDIV(UI4,2,BOOL,VARIANT_TRUE,I4,-2);
    VARIDIV(UI4,2,I1,1,I4,2);
    VARIDIV(UI4,2,UI1,1,I4,2);
    VARIDIV(UI4,2,UI2,1,I4,2);
    VARIDIV(UI4,2,UI4,1,I4,2);
    if (has_i8)
    {
        VARIDIV(UI4,2,I8,1,I8,2);
        VARIDIV(UI4,2,UI8,1,I4,2);
    }
    VARIDIV(UI4,2,INT,1,I4,2);
    VARIDIV(UI4,2,UINT,1,I4,2);
    if (has_i8)
    {
        VARIDIV(I8,2,NULL,0,NULL,0);
        VARIDIV(I8,2,I2,1,I8,2);
        VARIDIV(I8,2,I4,1,I8,2);
        VARIDIV(I8,2,R4,1.0f,I8,2);
        VARIDIV(I8,2,R8,1.0,I8,2);
        VARIDIV(I8,2,DATE,1,I8,2);
        VARIDIV(I8,2,BSTR,num1_str,I8,2);
        VARIDIV(I8,2,BOOL,1,I8,2);
        VARIDIV(I8,2,I1,1,I8,2);
        VARIDIV(I8,2,UI1,1,I8,2);
        VARIDIV(I8,2,UI2,1,I8,2);
        VARIDIV(I8,2,UI4,1,I8,2);
        VARIDIV(I8,2,I8,1,I8,2);
        VARIDIV(I8,2,UI8,1,I8,2);
        VARIDIV(I8,2,UINT,1,I8,2);
        VARIDIV(UI8,2,NULL,0,NULL,0);
        VARIDIV(UI8,2,I2,1,I4,2);
        VARIDIV(UI8,2,I4,1,I4,2);
        VARIDIV(UI8,2,R4,1.0f,I4,2);
        VARIDIV(UI8,2,R8,1.0,I4,2);
        VARIDIV(UI8,2,DATE,1,I4,2);
        VARIDIV(UI8,2,BSTR,num1_str,I4,2);
        VARIDIV(UI8,2,BOOL,VARIANT_TRUE,I4,-2);
        VARIDIV(UI8,2,I1,1,I4,2);
        VARIDIV(UI8,2,UI1,1,I4,2);
        VARIDIV(UI8,2,UI2,1,I4,2);
        VARIDIV(UI8,2,UI4,1,I4,2);
        VARIDIV(UI8,2,I8,1,I8,2);
        VARIDIV(UI8,2,UI8,1,I4,2);
        VARIDIV(UI8,2,INT,1,I4,2);
        VARIDIV(UI8,2,UINT,1,I4,2);
    }
    VARIDIV(INT,2,NULL,0,NULL,0);
    VARIDIV(INT,2,I2,1,I4,2);
    VARIDIV(INT,2,I4,1,I4,2);
    VARIDIV(INT,2,R4,1.0f,I4,2);
    VARIDIV(INT,2,R8,1.0,I4,2);
    VARIDIV(INT,2,DATE,1,I4,2);
    VARIDIV(INT,2,BSTR,num1_str,I4,2);
    VARIDIV(INT,2,BOOL,VARIANT_TRUE,I4,-2);
    VARIDIV(INT,2,I1,1,I4,2);
    VARIDIV(INT,2,UI1,1,I4,2);
    VARIDIV(INT,2,UI2,1,I4,2);
    VARIDIV(INT,2,UI4,1,I4,2);
    if (has_i8)
    {
        VARIDIV(INT,2,UI8,1,I4,2);
    }
    VARIDIV(INT,2,INT,1,I4,2);
    VARIDIV(INT,2,UINT,1,I4,2);
    VARIDIV(UINT,2,NULL,0,NULL,0);
    VARIDIV(UINT,2,I2,1,I4,2);
    VARIDIV(UINT,2,I4,1,I4,2);
    VARIDIV(UINT,2,R4,1.0f,I4,2);
    VARIDIV(UINT,2,R8,1.0,I4,2);
    VARIDIV(UINT,2,DATE,1,I4,2);
    VARIDIV(UINT,2,BSTR,num1_str,I4,2);
    VARIDIV(UINT,2,BOOL,VARIANT_TRUE,I4,-2);
    VARIDIV(UINT,2,I1,1,I4,2);
    VARIDIV(UINT,2,UI1,1,I4,2);
    VARIDIV(UINT,2,UI2,1,I4,2);
    VARIDIV(UINT,2,UI4,1,I4,2);
    if (has_i8)
    {
        VARIDIV(UINT,2,I8,1,I8,2);
        VARIDIV(UINT,2,UI8,1,I4,2);
    }
    VARIDIV(UINT,2,INT,1,I4,2);
    VARIDIV(UINT,2,UINT,1,I4,2);

    /* Manually test some VT_CY, VT_DECIMAL variants */
    V_VT(&cy) = VT_CY;
    hres = VarCyFromI4(10000, &V_CY(&cy));
    ok(hres == S_OK, "VarCyFromI4 failed!\n");
    V_VT(&dec) = VT_DECIMAL;
    hres = VarDecFromR8(2.0, &V_DECIMAL(&dec));
    ok(hres == S_OK, "VarDecFromR4 failed!\n");
    memset(&left, 0, sizeof(left));
    memset(&right, 0, sizeof(right));
    V_VT(&left) = VT_I4;
    V_I4(&left) = 100;
    V_VT(&right) = VT_I8;
    V_UI1(&right) = 2;

    hres = pVarIdiv(&cy, &cy, &result);
    ok(hres == S_OK && V_VT(&result) == VT_I4,
        "VARIDIV: expected coerced hres 0x%lX type VT_I4, got hres 0x%lX type %s!\n",
        S_OK, hres, wine_dbgstr_vt(V_VT(&result)));
    ok(hres == S_OK && V_I4(&result) == 1,
        "VARIDIV: CY value %ld, expected %d\n", V_I4(&result), 1);

    if (has_i8)
    {
        hres = pVarIdiv(&cy, &right, &result);
        ok(hres == S_OK && V_VT(&result) == VT_I8,
            "VARIDIV: expected coerced hres 0x%lX type VT_I8, got hres 0x%lX type %s!\n",
            S_OK, hres, wine_dbgstr_vt(V_VT(&result)));
        ok(hres == S_OK && V_I8(&result) == 5000,
            "VARIDIV: CY value %#I64x, expected %#x\n",
	    V_I8(&result), 5000);
    }

    hres = pVarIdiv(&left, &cy, &result);
    ok(hres == S_OK && V_VT(&result) == VT_I4,
        "VARIDIV: expected coerced hres 0x%lX type VT_I4, got hres 0x%lX type %s!\n",
        S_OK, hres, wine_dbgstr_vt(V_VT(&result)));
    ok(hres == S_OK && V_I4(&result) == 0,
        "VARIDIV: CY value %ld, expected %d\n", V_I4(&result), 0);

    hres = pVarIdiv(&left, &dec, &result);
    ok(hres == S_OK && V_VT(&result) == VT_I4,
        "VARIDIV: expected coerced hres 0x%lX type VT_I4, got hres 0x%lX type %s!\n",
        S_OK, hres, wine_dbgstr_vt(V_VT(&result)));
    ok(hres == S_OK && V_I4(&result) == 50,
        "VARIDIV: DECIMAL value %ld, expected %d\n", V_I4(&result), 50);

    hres = pVarIdiv(&dec, &dec, &result);
    ok(hres == S_OK && V_VT(&result) == VT_I4,
        "VARIDIV: expected coerced hres 0x%lX type VT_I4, got hres 0x%lX type %s!\n",
        S_OK, hres, wine_dbgstr_vt(V_VT(&result)));
    ok(hres == S_OK && V_I4(&result) == 1,
        "VARIDIV: DECIMAL value %ld, expected %d\n", V_I4(&result), 1);

    if (has_i8)
    {
        hres = pVarIdiv(&dec, &right, &result);
        ok(hres == S_OK && V_VT(&result) == VT_I8,
            "VARIDIV: expected coerced hres 0x%lX type VT_I8, got hres 0x%lX type %s!\n",
            S_OK, hres, wine_dbgstr_vt(V_VT(&result)));
        ok(hres == S_OK && V_I8(&result) == 1,
            "VARIDIV: DECIMAL value %I64d, expected %d\n",
	    V_I8(&result), 1);
    }

    /* Check for division by zero */
    V_VT(&left) = VT_INT;
    V_I4(&left) = 1;
    V_VT(&right) = VT_INT;
    V_I4(&right) = 0;
    hres = pVarIdiv(&left, &right, &result);
    ok(hres == DISP_E_DIVBYZERO && V_VT(&result) == VT_EMPTY,
        "VARIDIV: Division by 0 should result in DISP_E_DIVBYZERO but got 0x%lX\n", hres);

    V_VT(&left) = VT_INT;
    V_I4(&left) = 0;
    V_VT(&right) = VT_INT;
    V_I4(&right) = 0;
    hres = pVarIdiv(&left, &right, &result);
    ok(hres == DISP_E_DIVBYZERO && V_VT(&result) == VT_EMPTY,
        "VARIDIV: Division by 0 should result in DISP_E_DIVBYZERO but got 0x%lX\n", hres);

    SysFreeString(num1_str);
    SysFreeString(num2_str);
}


static HRESULT (WINAPI *pVarImp)(LPVARIANT,LPVARIANT,LPVARIANT);

#define VARIMP(vt1,val1,vt2,val2,rvt,rval)              \
        V_VT(&left) = VT_##vt1; V_##vt1(&left) = val1;   \
        V_VT(&right) = VT_##vt2; V_##vt2(&right) = val2; \
        V_VT(&exp) = VT_##rvt; V_##rvt(&exp) = rval;     \
        test_var_call2( __LINE__, pVarImp, &left, &right, &exp )

/* Skip any type that is not defined or produces an error for every case */
#define SKIPTESTIMP(a)                            \
    if (a == VT_ERROR || a == VT_VARIANT ||       \
        a == VT_DISPATCH || a == VT_UNKNOWN ||    \
        a == VT_RECORD || a > VT_UINT ||          \
        a == 15 /*not defined*/)                  \
        continue

static void test_VarImp(void)
{
    static const WCHAR szFalse[] = { '#','F','A','L','S','E','#','\0' };
    static const WCHAR szTrue[] = { '#','T','R','U','E','#','\0' };
    VARIANT left, right, exp, result, cy, dec;
    BSTR true_str, false_str;
    VARTYPE i;
    HRESULT hres;

    CHECKPTR(VarImp);

    true_str = SysAllocString(szTrue);
    false_str = SysAllocString(szFalse);

    /* Test all possible flag/vt combinations & the resulting vt type */
    for (i = 0; i < ARRAY_SIZE(ExtraFlags); i++)
    {
        VARTYPE leftvt, rightvt, resvt;

        for (leftvt = 0; leftvt <= VT_BSTR_BLOB; leftvt++)
        {
            SKIPTESTIMP(leftvt);

            /* Check if we need/have support for I8 and/or UI8 */
            if ((leftvt == VT_I8 || leftvt == VT_UI8) && !has_i8)
                continue;

            for (rightvt = 0; rightvt <= VT_BSTR_BLOB; rightvt++)
            {
                BOOL bFail = FALSE;
                SKIPTESTIMP(rightvt);

                /* Native crashes when using the extra flag VT_BYREF
                 * or with the following VT combinations
                 */
                if ((leftvt == VT_UI4 && rightvt == VT_BSTR) ||
                    (leftvt == VT_UI8 && rightvt == VT_BSTR) ||
                    ExtraFlags[i] == VT_BYREF)
                    continue;

                /* Check if we need/have support for I8 and/or UI8 */
                if ((rightvt == VT_I8 || rightvt == VT_UI8) && !has_i8)
                    continue;

                memset(&left, 0, sizeof(left));
                memset(&right, 0, sizeof(right));
                V_VT(&left) = leftvt | ExtraFlags[i];
                V_VT(&right) = rightvt | ExtraFlags[i];
                V_VT(&result) = VT_EMPTY;
                resvt = VT_EMPTY;

                if (leftvt == VT_BSTR)
                    V_BSTR(&left) = true_str;

                /* This allows us to test return types that are not NULL
                 * (NULL Imp value = n, NULL Imp 0 = NULL)
                 */
                switch(rightvt)
                {
                case VT_BSTR:
                    V_BSTR(&right) = true_str;
                    break;
                case VT_DECIMAL:
                    VarDecFromR8(2.0, &V_DECIMAL(&right));
                    V_VT(&right) = rightvt | ExtraFlags[i];
                    break;
                case VT_BOOL:
                    V_BOOL(&right) = VARIANT_TRUE;
                    break;
                case VT_I1: V_I1(&right) = 2; break;
                case VT_I2: V_I2(&right) = 2; break;
                case VT_I4: V_I4(&right) = 2; break;
                case VT_R4: V_R4(&right) = 2.0f; break;
                case VT_R8: V_R8(&right) = 2.0; break;
                case VT_CY: V_CY(&right).int64 = 10000; break;
                case VT_DATE: V_DATE(&right) = 2; break;
                case VT_I8: V_I8(&right) = 2; break;
                case VT_INT: V_INT(&right) = 2; break;
                case VT_UINT: V_UINT(&right) = 2; break;
                case VT_UI1: V_UI1(&right) = 2; break;
                case VT_UI2: V_UI2(&right) = 2; break;
                case VT_UI4: V_UI4(&right) = 2; break;
                case VT_UI8: V_UI8(&right) = 2; break;
                default: break;
                }

                /* Native VarImp always returns an error when using extra
                 * flags or if the variants are I8 and INT.
                 */
                if ((leftvt == VT_I8 && rightvt == VT_INT) ||
                    ExtraFlags[i] != 0)
                    bFail = TRUE;

                /* Determine result type */
                else if ((leftvt == VT_BSTR && rightvt == VT_NULL) ||
                    (leftvt == VT_NULL && rightvt == VT_NULL) ||
                    (leftvt == VT_NULL && rightvt == VT_EMPTY))
                    resvt = VT_NULL;
                else if (leftvt == VT_I8 || rightvt == VT_I8)
                    resvt = VT_I8;
                else if (leftvt == VT_I4 || rightvt == VT_I4 ||
                    leftvt == VT_INT || rightvt == VT_INT ||
                    leftvt == VT_UINT || rightvt == VT_UINT ||
                    leftvt == VT_UI4 || rightvt == VT_UI4 ||
                    leftvt == VT_UI8 || rightvt == VT_UI8 ||
                    leftvt == VT_UI2 || rightvt == VT_UI2 ||
                    leftvt == VT_DECIMAL || rightvt == VT_DECIMAL ||
                    leftvt == VT_DATE || rightvt == VT_DATE ||
                    leftvt == VT_CY || rightvt == VT_CY ||
                    leftvt == VT_R8 || rightvt == VT_R8 ||
                    leftvt == VT_R4 || rightvt == VT_R4 ||
                    leftvt == VT_I1 || rightvt == VT_I1)
                    resvt = VT_I4;
                else if ((leftvt == VT_UI1 && rightvt == VT_UI1) ||
                    (leftvt == VT_UI1 && rightvt == VT_NULL) ||
                    (leftvt == VT_NULL && rightvt == VT_UI1))
                    resvt = VT_UI1;
                else if (leftvt == VT_EMPTY || rightvt == VT_EMPTY ||
                    leftvt == VT_I2 || rightvt == VT_I2 ||
                    leftvt == VT_UI1 || rightvt == VT_UI1)
                    resvt = VT_I2;
                else if (leftvt == VT_BOOL || rightvt == VT_BOOL ||
                    leftvt == VT_BSTR || rightvt == VT_BSTR)
                    resvt = VT_BOOL;

                hres = pVarImp(&left, &right, &result);

                /* Check expected HRESULT and if result variant type is correct */
                if (bFail)
                    ok (hres == DISP_E_BADVARTYPE || hres == DISP_E_TYPEMISMATCH,
                        "VarImp: %s|0x%X, %s|0x%X: got vt %s hr 0x%lX\n",
                        wine_dbgstr_vt(leftvt), ExtraFlags[i], wine_dbgstr_vt(rightvt), ExtraFlags[i],
                        wine_dbgstr_vt(V_VT(&result)), hres);
                else
                    ok (hres == S_OK && resvt == V_VT(&result),
                        "VarImp: %s|0x%X, %s|0x%X: expected vt %s hr 0x%lX, got vt %s hr 0x%lX\n",
                        wine_dbgstr_vt(leftvt), ExtraFlags[i], wine_dbgstr_vt(rightvt), ExtraFlags[i], wine_dbgstr_vt(resvt),
                        S_OK, wine_dbgstr_vt(V_VT(&result)), hres);
            }
        }
    }

    VARIMP(EMPTY,0,EMPTY,0,I2,-1);
    VARIMP(EMPTY,0,NULL,0,I2,-1);
    VARIMP(EMPTY,0,I2,-1,I2,-1);
    VARIMP(EMPTY,0,I4,-1,I4,-1);
    VARIMP(EMPTY,0,R4,0.0f,I4,-1);
    VARIMP(EMPTY,0,R8,-1.0,I4,-1);
    VARIMP(EMPTY,0,DATE,0,I4,-1);
    VARIMP(EMPTY,0,BSTR,true_str,I2,-1);
    VARIMP(EMPTY,0,BOOL,VARIANT_FALSE,I2,-1);
    VARIMP(EMPTY,0,I1,0,I4,-1);
    VARIMP(EMPTY,0,UI1,1,I2,-1);
    VARIMP(EMPTY,0,UI2,1,I4,-1);
    VARIMP(EMPTY,0,UI4,1,I4,-1);
    if (has_i8)
    {
        VARIMP(EMPTY,0,I8,1,I8,-1);
        VARIMP(EMPTY,0,UI8,1,I4,-1);
    }
    VARIMP(EMPTY,0,INT,-1,I4,-1);
    VARIMP(EMPTY,0,UINT,1,I4,-1);
    VARIMP(NULL,0,EMPTY,0,NULL,0);
    VARIMP(NULL,0,NULL,0,NULL,0);
    VARIMP(NULL,0,I2,-1,I2,-1);
    VARIMP(NULL,0,I4,-1,I4,-1);
    VARIMP(NULL,0,R4,0.0f,NULL,0);
    VARIMP(NULL,0,R8,-1.0,I4,-1);
    VARIMP(NULL,0,DATE,0,NULL,0);
    VARIMP(NULL,0,BSTR,true_str,BOOL,-1);
    VARIMP(NULL,0,BOOL,VARIANT_FALSE,NULL,0);
    VARIMP(NULL,0,I1,0,NULL,0);
    VARIMP(NULL,0,UI1,1,UI1,1);
    VARIMP(NULL,0,UI2,1,I4,1);
    VARIMP(NULL,0,UI4,1,I4,1);
    if (has_i8)
    {
        VARIMP(NULL,0,I8,1,I8,1);
        VARIMP(NULL,0,UI8,1,I4,1);
    }
    VARIMP(NULL,0,INT,-1,I4,-1);
    VARIMP(NULL,0,UINT,1,I4,1);
    VARIMP(I2,-1,EMPTY,0,I2,0);
    VARIMP(I2,-1,I2,-1,I2,-1);
    VARIMP(I2,-1,I4,-1,I4,-1);
    VARIMP(I2,-1,R4,0.0f,I4,0);
    VARIMP(I2,-1,R8,-1.0,I4,-1);
    VARIMP(I2,-1,DATE,0,I4,0);
    VARIMP(I2,-1,BSTR,true_str,I2,-1);
    VARIMP(I2,-1,BOOL,VARIANT_FALSE,I2,0);
    VARIMP(I2,-1,I1,0,I4,0);
    VARIMP(I2,-1,UI1,1,I2,1);
    VARIMP(I2,-1,UI2,1,I4,1);
    VARIMP(I2,-1,UI4,1,I4,1);
    if (has_i8)
    {
        VARIMP(I2,-1,I8,1,I8,1);
        VARIMP(I2,-1,UI8,1,I4,1);
    }
    VARIMP(I2,-1,INT,-1,I4,-1);
    VARIMP(I2,-1,UINT,1,I4,1);
    VARIMP(I4,2,EMPTY,0,I4,-3);
    VARIMP(I4,2,NULL,0,I4,-3);
    VARIMP(I4,2,I2,-1,I4,-1);
    VARIMP(I4,2,I4,-1,I4,-1);
    VARIMP(I4,2,R4,0.0f,I4,-3);
    VARIMP(I4,2,R8,-1.0,I4,-1);
    VARIMP(I4,2,DATE,0,I4,-3);
    VARIMP(I4,2,BSTR,true_str,I4,-1);
    VARIMP(I4,2,BOOL,VARIANT_FALSE,I4,-3);
    VARIMP(I4,2,I1,0,I4,-3);
    VARIMP(I4,2,UI1,1,I4,-3);
    VARIMP(I4,2,UI2,1,I4,-3);
    VARIMP(I4,2,UI4,1,I4,-3);
    if (has_i8)
    {
        VARIMP(I4,2,I8,1,I8,-3);
        VARIMP(I4,2,UI8,1,I4,-3);
    }
    VARIMP(I4,2,INT,-1,I4,-1);
    VARIMP(I4,2,UINT,1,I4,-3);
    VARIMP(R4,-1.0f,EMPTY,0,I4,0);
    VARIMP(R4,-1.0f,NULL,0,NULL,0);
    VARIMP(R4,-1.0f,I2,-1,I4,-1);
    VARIMP(R4,-1.0f,I4,-1,I4,-1);
    VARIMP(R4,-1.0f,R4,0.0f,I4,0);
    VARIMP(R4,-1.0f,R8,-1.0,I4,-1);
    VARIMP(R4,-1.0f,DATE,1,I4,1);
    VARIMP(R4,-1.0f,BSTR,true_str,I4,-1);
    VARIMP(R4,-1.0f,BOOL,VARIANT_FALSE,I4,0);
    VARIMP(R4,-1.0f,I1,0,I4,0);
    VARIMP(R4,-1.0f,UI1,1,I4,1);
    VARIMP(R4,-1.0f,UI2,1,I4,1);
    VARIMP(R4,-1.0f,UI4,1,I4,1);
    if (has_i8)
    {
        VARIMP(R4,-1.0f,I8,1,I8,1);
        VARIMP(R4,-1.0f,UI8,1,I4,1);
    }
    VARIMP(R4,-1.0f,INT,-1,I4,-1);
    VARIMP(R4,-1.0f,UINT,1,I4,1);
    VARIMP(R8,1.0,EMPTY,0,I4,-2);
    VARIMP(R8,1.0,NULL,0,I4,-2);
    VARIMP(R8,1.0,I2,-1,I4,-1);
    VARIMP(R8,1.0,I4,-1,I4,-1);
    VARIMP(R8,1.0,R4,0.0f,I4,-2);
    VARIMP(R8,1.0,R8,-1.0,I4,-1);
    VARIMP(R8,1.0,DATE,0,I4,-2);
    VARIMP(R8,1.0,BSTR,true_str,I4,-1);
    VARIMP(R8,1.0,BOOL,VARIANT_FALSE,I4,-2);
    VARIMP(R8,1.0,I1,0,I4,-2);
    VARIMP(R8,1.0,UI1,1,I4,-1);
    VARIMP(R8,1.0,UI2,1,I4,-1);
    VARIMP(R8,1.0,UI4,1,I4,-1);
    if (has_i8)
    {
        VARIMP(R8,1.0,I8,1,I8,-1);
        VARIMP(R8,1.0,UI8,1,I4,-1);
    }
    VARIMP(R8,1.0,INT,-1,I4,-1);
    VARIMP(R8,1.0,UINT,1,I4,-1);
    VARIMP(DATE,0,EMPTY,0,I4,-1);
    VARIMP(DATE,0,NULL,0,I4,-1);
    VARIMP(DATE,0,I2,-1,I4,-1);
    VARIMP(DATE,0,I4,-1,I4,-1);
    VARIMP(DATE,0,R4,0.0f,I4,-1);
    VARIMP(DATE,0,R8,-1.0,I4,-1);
    VARIMP(DATE,0,DATE,0,I4,-1);
    VARIMP(DATE,0,BSTR,true_str,I4,-1);
    VARIMP(DATE,0,BOOL,VARIANT_FALSE,I4,-1);
    VARIMP(DATE,0,I1,0,I4,-1);
    VARIMP(DATE,0,UI1,1,I4,-1);
    VARIMP(DATE,0,UI2,1,I4,-1);
    VARIMP(DATE,0,UI4,1,I4,-1);
    if (has_i8)
    {
        VARIMP(DATE,0,I8,1,I8,-1);
        VARIMP(DATE,0,UI8,1,I4,-1);
    }
    VARIMP(DATE,0,INT,-1,I4,-1);
    VARIMP(DATE,0,UINT,1,I4,-1);
    VARIMP(BSTR,false_str,EMPTY,0,I2,-1);
    VARIMP(BSTR,false_str,NULL,0,BOOL,-1);
    VARIMP(BSTR,false_str,I2,-1,I2,-1);
    VARIMP(BSTR,false_str,I4,-1,I4,-1);
    VARIMP(BSTR,false_str,R4,0.0f,I4,-1);
    VARIMP(BSTR,false_str,R8,-1.0,I4,-1);
    VARIMP(BSTR,false_str,DATE,0,I4,-1);
    VARIMP(BSTR,false_str,BSTR,true_str,BOOL,-1);
    VARIMP(BSTR,false_str,BOOL,VARIANT_FALSE,BOOL,-1);
    VARIMP(BSTR,false_str,I1,0,I4,-1);
    VARIMP(BSTR,false_str,UI1,1,I2,-1);
    VARIMP(BSTR,false_str,UI2,1,I4,-1);
    VARIMP(BSTR,false_str,UI4,1,I4,-1);
    if (has_i8)
    {
        VARIMP(BSTR,false_str,I8,1,I8,-1);
        VARIMP(BSTR,false_str,UI8,1,I4,-1);
    }
    VARIMP(BSTR,false_str,INT,-1,I4,-1);
    VARIMP(BSTR,false_str,UINT,1,I4,-1);
    VARIMP(BOOL,VARIANT_TRUE,EMPTY,0,I2,0);
    VARIMP(BOOL,VARIANT_TRUE,NULL,0,NULL,0);
    VARIMP(BOOL,VARIANT_TRUE,I2,-1,I2,-1);
    VARIMP(BOOL,VARIANT_TRUE,I4,-1,I4,-1);
    VARIMP(BOOL,VARIANT_TRUE,R4,0.0f,I4,0);
    VARIMP(BOOL,VARIANT_TRUE,R8,-1.0,I4,-1);
    VARIMP(BOOL,VARIANT_TRUE,DATE,0,I4,0);
    VARIMP(BOOL,VARIANT_TRUE,BSTR,true_str,BOOL,-1);
    VARIMP(BOOL,VARIANT_TRUE,BOOL,VARIANT_FALSE,BOOL,0);
    VARIMP(BOOL,VARIANT_TRUE,I1,0,I4,0);
    VARIMP(BOOL,VARIANT_TRUE,UI1,1,I2,1);
    VARIMP(BOOL,VARIANT_TRUE,UI2,1,I4,1);
    VARIMP(BOOL,VARIANT_TRUE,UI4,1,I4,1);
    if (has_i8)
    {
        VARIMP(BOOL,VARIANT_TRUE,I8,1,I8,1);
        VARIMP(BOOL,VARIANT_TRUE,UI8,1,I4,1);
    }
    VARIMP(BOOL,VARIANT_TRUE,INT,-1,I4,-1);
    VARIMP(BOOL,VARIANT_TRUE,UINT,1,I4,1);
    VARIMP(I1,-1,EMPTY,0,I4,0);
    VARIMP(I1,-1,NULL,0,NULL,0);
    VARIMP(I1,-1,I2,-1,I4,-1);
    VARIMP(I1,-1,I4,-1,I4,-1);
    VARIMP(I1,-1,R4,0.0f,I4,0);
    VARIMP(I1,-1,R8,-1.0,I4,-1);
    VARIMP(I1,-1,DATE,0,I4,0);
    VARIMP(I1,-1,BSTR,true_str,I4,-1);
    VARIMP(I1,-1,BOOL,VARIANT_FALSE,I4,0);
    VARIMP(I1,-1,I1,0,I4,0);
    VARIMP(I1,-1,UI1,1,I4,1);
    VARIMP(I1,-1,UI2,1,I4,1);
    VARIMP(I1,-1,UI4,1,I4,1);
    if (has_i8)
    {
        VARIMP(I1,-1,I8,1,I8,1);
        VARIMP(I1,-1,UI8,1,I4,1);
    }
    VARIMP(I1,-1,INT,-1,I4,-1);
    VARIMP(I1,-1,UINT,1,I4,1);
    VARIMP(UI1,0,EMPTY,0,I2,-1);
    VARIMP(UI1,0,NULL,0,UI1,255);
    VARIMP(UI1,0,I2,-1,I2,-1);
    VARIMP(UI1,0,I4,-1,I4,-1);
    VARIMP(UI1,0,R4,0.0f,I4,-1);
    VARIMP(UI1,0,R8,-1.0,I4,-1);
    VARIMP(UI1,0,DATE,0,I4,-1);
    VARIMP(UI1,0,BSTR,true_str,I2,-1);
    VARIMP(UI1,0,BOOL,VARIANT_FALSE,I2,-1);
    VARIMP(UI1,0,I1,0,I4,-1);
    VARIMP(UI1,0,UI1,1,UI1,255);
    VARIMP(UI1,0,UI2,1,I4,-1);
    VARIMP(UI1,0,UI4,1,I4,-1);
    if (has_i8)
    {
        VARIMP(UI1,0,I8,1,I8,-1);
        VARIMP(UI1,0,UI8,1,I4,-1);
    }
    VARIMP(UI1,0,INT,-1,I4,-1);
    VARIMP(UI1,0,UINT,1,I4,-1);
    VARIMP(UI2,0,EMPTY,0,I4,-1);
    VARIMP(UI2,0,NULL,0,I4,-1);
    VARIMP(UI2,0,I2,-1,I4,-1);
    VARIMP(UI2,0,I4,-1,I4,-1);
    VARIMP(UI2,0,R4,0.0f,I4,-1);
    VARIMP(UI2,0,R8,-1.0,I4,-1);
    VARIMP(UI2,0,DATE,0,I4,-1);
    VARIMP(UI2,0,BSTR,true_str,I4,-1);
    VARIMP(UI2,0,BOOL,VARIANT_FALSE,I4,-1);
    VARIMP(UI2,0,I1,0,I4,-1);
    VARIMP(UI2,0,UI1,1,I4,-1);
    VARIMP(UI2,0,UI2,1,I4,-1);
    VARIMP(UI2,0,UI4,1,I4,-1);
    if (has_i8)
    {
        VARIMP(UI2,0,I8,1,I8,-1);
        VARIMP(UI2,0,UI8,1,I4,-1);
    }
    VARIMP(UI2,0,INT,-1,I4,-1);
    VARIMP(UI2,0,UINT,1,I4,-1);
    VARIMP(UI4,0,EMPTY,0,I4,-1);
    VARIMP(UI4,0,NULL,0,I4,-1);
    VARIMP(UI4,0,I2,-1,I4,-1);
    VARIMP(UI4,0,I4,-1,I4,-1);
    VARIMP(UI4,0,R4,0.0f,I4,-1);
    VARIMP(UI4,0,R8,-1.0,I4,-1);
    VARIMP(UI4,0,DATE,0,I4,-1);
    VARIMP(UI4,0,BSTR,true_str,I4,-1);
    VARIMP(UI4,0,BOOL,VARIANT_FALSE,I4,-1);
    VARIMP(UI4,0,I1,0,I4,-1);
    VARIMP(UI4,0,UI1,1,I4,-1);
    VARIMP(UI4,0,UI2,1,I4,-1);
    VARIMP(UI4,0,UI4,1,I4,-1);
    if (has_i8)
    {
        VARIMP(UI4,0,I8,1,I8,-1);
        VARIMP(UI4,0,UI8,1,I4,-1);
    }
    VARIMP(UI4,0,INT,-1,I4,-1);
    VARIMP(UI4,0,UINT,1,I4,-1);
    if (has_i8)
    {
        VARIMP(I8,-1,EMPTY,0,I8,0);
        VARIMP(I8,-1,NULL,0,NULL,0);
        VARIMP(I8,-1,I2,-1,I8,-1);
        VARIMP(I8,-1,I4,-1,I8,-1);
        VARIMP(I8,-1,R4,0.0f,I8,0);
        VARIMP(I8,-1,R8,-1.0,I8,-1);
        VARIMP(I8,-1,DATE,0,I8,0);
        VARIMP(I8,-1,BSTR,true_str,I8,-1);
        VARIMP(I8,-1,BOOL,VARIANT_FALSE,I8,0);
        VARIMP(I8,-1,I1,0,I8,0);
        VARIMP(I8,-1,UI1,1,I8,1);
        VARIMP(I8,-1,UI2,1,I8,1);
        VARIMP(I8,-1,UI4,1,I8,1);
        VARIMP(I8,-1,I8,1,I8,1);
        VARIMP(I8,-1,UI8,1,I8,1);
        VARIMP(I8,-1,UINT,1,I8,1);
        VARIMP(UI8,0,EMPTY,0,I4,-1);
        VARIMP(UI8,0,NULL,0,I4,-1);
        VARIMP(UI8,0,I2,-1,I4,-1);
        VARIMP(UI8,0,I4,-1,I4,-1);
        VARIMP(UI8,0,R4,0.0f,I4,-1);
        VARIMP(UI8,0,R8,-1.0,I4,-1);
        VARIMP(UI8,0,DATE,0,I4,-1);
        VARIMP(UI8,0,BSTR,true_str,I4,-1);
        VARIMP(UI8,0,BOOL,VARIANT_FALSE,I4,-1);
        VARIMP(UI8,0,I1,0,I4,-1);
        VARIMP(UI8,0,UI1,1,I4,-1);
        VARIMP(UI8,0,UI2,1,I4,-1);
        VARIMP(UI8,0,UI4,1,I4,-1);
        VARIMP(UI8,0,I8,1,I8,-1);
        VARIMP(UI8,0,UI8,1,I4,-1);
        VARIMP(UI8,0,INT,-1,I4,-1);
        VARIMP(UI8,0,UINT,1,I4,-1);
    }
    VARIMP(INT,-1,EMPTY,0,I4,0);
    VARIMP(INT,-1,NULL,0,NULL,0);
    VARIMP(INT,-1,I2,-1,I4,-1);
    VARIMP(INT,-1,I4,-1,I4,-1);
    VARIMP(INT,-1,R4,0.0f,I4,0);
    VARIMP(INT,-1,R8,-1.0,I4,-1);
    VARIMP(INT,-1,DATE,0,I4,0);
    VARIMP(INT,-1,BSTR,true_str,I4,-1);
    VARIMP(INT,-1,BOOL,VARIANT_FALSE,I4,0);
    VARIMP(INT,-1,I1,0,I4,0);
    VARIMP(INT,-1,UI1,1,I4,1);
    VARIMP(INT,-1,UI2,1,I4,1);
    VARIMP(INT,-1,UI4,1,I4,1);
    if (has_i8)
    {
        VARIMP(INT,-1,I8,1,I8,1);
        VARIMP(INT,-1,UI8,1,I4,1);
    }
    VARIMP(INT,-1,INT,-1,I4,-1);
    VARIMP(INT,-1,UINT,1,I4,1);
    VARIMP(UINT,1,EMPTY,0,I4,-2);
    VARIMP(UINT,1,NULL,0,I4,-2);
    VARIMP(UINT,1,I2,-1,I4,-1);
    VARIMP(UINT,1,I4,-1,I4,-1);
    VARIMP(UINT,1,R4,0.0f,I4,-2);
    VARIMP(UINT,1,R8,-1.0,I4,-1);
    VARIMP(UINT,1,DATE,0,I4,-2);
    VARIMP(UINT,1,BSTR,true_str,I4,-1);
    VARIMP(UINT,1,BOOL,VARIANT_FALSE,I4,-2);
    VARIMP(UINT,1,I1,0,I4,-2);
    VARIMP(UINT,1,UI1,1,I4,-1);
    VARIMP(UINT,1,UI2,1,I4,-1);
    VARIMP(UINT,1,UI4,1,I4,-1);
    if (has_i8)
    {
        VARIMP(UINT,1,I8,1,I8,-1);
        VARIMP(UINT,1,UI8,1,I4,-1);
    }
    VARIMP(UINT,1,INT,-1,I4,-1);
    VARIMP(UINT,1,UINT,1,I4,-1);

    /* Manually test some VT_CY, VT_DECIMAL variants */
    V_VT(&cy) = VT_CY;
    hres = VarCyFromI4(1, &V_CY(&cy));
    ok(hres == S_OK, "VarCyFromI4 failed!\n");
    V_VT(&dec) = VT_DECIMAL;
    hres = VarDecFromR8(2.0, &V_DECIMAL(&dec));
    ok(hres == S_OK, "VarDecFromR4 failed!\n");
    memset(&left, 0, sizeof(left));
    memset(&right, 0, sizeof(right));
    V_VT(&left) = VT_I4;
    V_I4(&left) = 0;
    V_VT(&right) = VT_I8;
    V_UI1(&right) = 0;

    hres = pVarImp(&cy, &cy, &result);
    ok(hres == S_OK && V_VT(&result) == VT_I4,
        "VARIMP: expected coerced hres 0x%lX type VT_I4, got hres 0x%lX type %s!\n",
        S_OK, hres, wine_dbgstr_vt(V_VT(&result)));
    ok(hres == S_OK && V_I4(&result) == -1,
        "VARIMP: CY value %ld, expected %d\n", V_I4(&result), -1);

    if (has_i8)
    {
        hres = pVarImp(&cy, &right, &result);
        ok(hres == S_OK && V_VT(&result) == VT_I8,
            "VARIMP: expected coerced hres 0x%lX type VT_I8, got hres 0x%lX type %s!\n",
            S_OK, hres, wine_dbgstr_vt(V_VT(&result)));
        ok(hres == S_OK && V_I8(&result) == -2,
            "VARIMP: CY value %I64d, expected %d\n",
            V_I8(&result), -2);
    }

    hres = pVarImp(&left, &cy, &result);
    ok(hres == S_OK && V_VT(&result) == VT_I4,
        "VARIMP: expected coerced hres 0x%lX type VT_I4, got hres 0x%lX type %s!\n",
        S_OK, hres, wine_dbgstr_vt(V_VT(&result)));
    ok(hres == S_OK && V_I4(&result) == -1,
        "VARIMP: CY value %ld, expected %d\n", V_I4(&result), -1);

    hres = pVarImp(&left, &dec, &result);
    ok(hres == S_OK && V_VT(&result) == VT_I4,
        "VARIMP: expected coerced hres 0x%lX type VT_I4, got hres 0x%lX type %s!\n",
        S_OK, hres, wine_dbgstr_vt(V_VT(&result)));
    ok(hres == S_OK && V_I4(&result) == -1,
        "VARIMP: DECIMAL value %ld, expected %d\n", V_I4(&result), -1);

    hres = pVarImp(&dec, &dec, &result);
    ok(hres == S_OK && V_VT(&result) == VT_I4,
        "VARIMP: expected coerced hres 0x%lX type VT_I4, got hres 0x%lX type %s!\n",
        S_OK, hres, wine_dbgstr_vt(V_VT(&result)));
    ok(hres == S_OK && V_I4(&result) == -1,
        "VARIMP: DECIMAL value %ld, expected %d\n", V_I4(&result), -1);

    if (has_i8)
    {
        hres = pVarImp(&dec, &right, &result);
        ok(hres == S_OK && V_VT(&result) == VT_I8,
            "VARIMP: expected coerced hres 0x%lX type VT_I8, got hres 0x%lX type %s!\n",
            S_OK, hres, wine_dbgstr_vt(V_VT(&result)));
        ok(hres == S_OK && V_I8(&result) == -3,
            "VARIMP: DECIMAL value %#I64x, expected %d\n",
	    V_I8(&result), -3);
    }

    SysFreeString(false_str);
    SysFreeString(true_str);
}

START_TEST(vartest)
{
  init();

  test_VariantInit();
  test_VariantClear();
  test_VariantCopy();
  test_VariantCopyInd();
  test_VarParseNumFromStrEn();
  test_VarParseNumFromStrFr();
  test_VarParseNumFromStrMisc();
  test_VarNumFromParseNum();
  test_VarUdateFromDate();
  test_VarDateFromUdate();
  test_SystemTimeToVariantTime();
  test_VariantTimeToSystemTime();
  test_DosDateTimeToVariantTime();
  test_VariantTimeToDosDateTime();
  test_VarAbs();
  test_VarNot();
  test_VarSub();
  test_VarMod();
  test_VarFix();
  test_VarInt();
  test_VarNeg();
  test_VarRound();
  test_VarXor();
  test_VarOr();
  test_VarPow();
  test_VarEqv();
  test_VarMul();
  test_VarAdd();
  test_VarCmp();
  test_VarCat();
  test_VarAnd();
  test_VarDiv();
  test_VarIdiv();
  test_VarImp();
}
