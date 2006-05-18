/*
 * VARIANT test program
 *
 * Copyright 1998 Jean-Claude Cote
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
#include <time.h>

#include "windef.h"
#include "winbase.h"
#include "winsock.h"
#include "wine/test.h"
#include "winuser.h"
#include "wingdi.h"
#include "winnls.h"
#include "winerror.h"
#include "winnt.h"

#include "wtypes.h"
#include "oleauto.h"

static HMODULE hOleaut32;

static HRESULT (WINAPI *pVarUdateFromDate)(DATE,ULONG,UDATE*);
static HRESULT (WINAPI *pVarDateFromUdate)(UDATE*,ULONG,DATE*);
static INT (WINAPI *pSystemTimeToVariantTime)(LPSYSTEMTIME,double*);
static INT (WINAPI *pVariantTimeToSystemTime)(double,LPSYSTEMTIME);
static INT (WINAPI *pDosDateTimeToVariantTime)(USHORT,USHORT,double*);
static INT (WINAPI *pVariantTimeToDosDateTime)(double,USHORT*,USHORT *);
static HRESULT (WINAPI *pVarFormatNumber)(LPVARIANT,int,int,int,int,ULONG,BSTR*);
static HRESULT (WINAPI *pVarFormat)(LPVARIANT,LPOLESTR,int,int,ULONG,BSTR*);

/* Get a conversion function ptr, return if function not available */
#define CHECKPTR(func) p##func = (void*)GetProcAddress(hOleaut32, #func); \
  if (!p##func) { trace("function " # func " not available, not testing it\n"); return; }

  /* Is a given function exported from oleaut32? */
#define HAVE_FUNC(func) ((void*)GetProcAddress(hOleaut32, #func) != NULL)

/* Have IRecordInfo data type? */
#define HAVE_OLEAUT32_RECORD  HAVE_FUNC(SafeArraySetRecordInfo)
/* Have CY data type? */
#define HAVE_OLEAUT32_CY      HAVE_FUNC(VarCyAdd)
/* Have I8/UI8 data type? */
#define HAVE_OLEAUT32_I8      HAVE_FUNC(VarI8FromI1)
/* Is this an ancient version with support for only I2/I4/R4/R8/DATE? */
#define IS_ANCIENT (!HAVE_FUNC(VarI1FromI2))

/* When comparing floating point values we cannot expect an exact match
 * because the rounding errors depend on the exact algorithm.
 */
#define EQ_DOUBLE(a,b)     (fabs((a)-(b))<1e-14)
#define EQ_FLOAT(a,b)      (fabs((a)-(b))<1e-7)

#define SKIPTESTS(a)  if((a > VT_CLSID+10) && (a < VT_BSTR_BLOB-10)) continue;

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

/* Macros to set a DECIMAL */
#define SETDEC(dec, scl, sgn, hi, lo) S(U(dec)).scale = (BYTE)scl; \
        S(U(dec)).sign = (BYTE)sgn; dec.Hi32 = (ULONG)hi; \
        U1(dec).Lo64 = (ULONG64)lo
#define SETDEC64(dec, scl, sgn, hi, mid, lo) S(U(dec)).scale = (BYTE)scl; \
        S(U(dec)).sign = (BYTE)sgn; dec.Hi32 = (ULONG)hi; \
        S1(U1(dec)).Mid32 = mid; S1(U1(dec)).Lo32 = lo;

static inline int strcmpW( const WCHAR *str1, const WCHAR *str2 )
{
    while (*str1 && (*str1 == *str2)) { str1++; str2++; }
    return *str1 - *str2;
}

/* return the string text of a given variant type */
static const char *vtstr(int x)
{
	switch(x) {
	case 0:
		return "VT_EMPTY";
	case 1:
		return "VT_NULL";
	case 2:
		return "VT_I2";
	case 3:
		return "VT_I4";
	case 4:
		return "VT_R4";
	case 5:
		return "VT_R8";
	case 6:
		return "VT_CY";
	case 7:
		return "VT_DATE";
	case 8:
		return "VT_BSTR";
	case 9:
		return "VT_DISPATCH";
	case 10:
		return "VT_ERROR";
	case 11:
		return "VT_BOOL";
	case 12:
		return "VT_VARIANT";
	case 13:
		return "VT_UNKNOWN";
	case 14:
		return "VT_DECIMAL";
	case 15:
		return "notdefined";
	case 16:
		return "VT_I1";
	case 17:
		return "VT_UI1";
	case 18:
		return "VT_UI2";
	case 19:
		return "VT_UI4";
	case 20:
		return "VT_I8";
	case 21:
		return "VT_UI8";
	case 22:
		return "VT_INT";
	case 23:
		return "VT_UINT";
	case 24:
		return "VT_VOID";
	case 25:
		return "VT_HRESULT";
	case 26:
		return "VT_PTR";
	case 27:
		return "VT_SAFEARRAY";
	case 28:
		return "VT_CARRAY";
	case 29:
		return "VT_USERDEFINED";
	case 30:
		return "VT_LPSTR";
	case 31:
		return "VT_LPWSTR";
	case 36:
		return "VT_RECORD";
	case 64:
		return "VT_FILETIME";
	case 65:
		return "VT_BLOB";
	case 66:
		return "VT_STREAM";
	case 67:
		return "VT_STORAGE";
	case 68:
		return "VT_STREAMED_OBJECT";
	case 69:
		return "VT_STORED_OBJECT";
	case 70:
		return "VT_BLOB_OBJECT";
	case 71:
		return "VT_CF";
	case 72:
		return "VT_CLSID";
	case 0xFFF:
		return "VT_BSTR_BLOB/VT_ILLEGALMASKED/VT_TYPEMASK";
	case 0x1000:
		return "VT_VECTOR";
	case 0x2000:
		return "VT_ARRAY";
	case 0x4000:
		return "VT_BYREF";
	case 0x8000:
		return "VT_BYREF";
	case 0xFFFF:
		return "VT_ILLEGAL";

	default:
		return "defineme";
	}
}

static void test_VariantInit(void)
{
  VARIANTARG v1, v2;

  /* Test that VariantInit() only sets the type */
  memset(&v1, -1, sizeof(v1));
  v2 = v1;
  V_VT(&v2) = VT_EMPTY;
  VariantInit(&v1);
  ok(!memcmp(&v1, &v2, sizeof(v1)), "VariantInit() set extra fields\n");
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
static int IsValidVariantClearVT(VARTYPE vt, VARTYPE extraFlags)
{
  int ret = 0;

  /* Only the following flags/types are valid */
  if ((vt <= VT_LPWSTR || vt == VT_RECORD || vt == VT_CLSID) &&
      vt != (VARTYPE)15 &&
      (vt < (VARTYPE)24 || vt > (VARTYPE)31) &&
      (!(extraFlags & (VT_BYREF|VT_ARRAY)) || vt > VT_NULL) &&
      (extraFlags == 0 || extraFlags == VT_BYREF || extraFlags == VT_ARRAY ||
       extraFlags == (VT_ARRAY|VT_BYREF)))
    ret = 1; /* ok */

  if ((vt == VT_RECORD && !HAVE_OLEAUT32_RECORD) ||
      ((vt == VT_I8 || vt == VT_UI8) && !HAVE_OLEAUT32_I8))
    ret = 0; /* Old versions of oleaut32 */
  return ret;
}

typedef struct
{
    const IUnknownVtbl *lpVtbl;
    LONG               ref;
    LONG               events;
} test_VariantClearImpl;

static HRESULT WINAPI VC_QueryInterface(LPUNKNOWN iface,REFIID riid,LPVOID *ppobj)
{
    test_VariantClearImpl *This = (test_VariantClearImpl *)iface;
    This->events |= 0x1;
    return E_NOINTERFACE;
}

static ULONG WINAPI VC_AddRef(LPUNKNOWN iface) {
    test_VariantClearImpl *This = (test_VariantClearImpl *)iface;
    This->events |= 0x2;
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI VC_Release(LPUNKNOWN iface) {
    test_VariantClearImpl *This = (test_VariantClearImpl *)iface;
    /* static class, won't be  freed */
    This->events |= 0x4;
    return InterlockedDecrement(&This->ref);
}

static const IUnknownVtbl test_VariantClear_vtbl = {
    VC_QueryInterface,
    VC_AddRef,
    VC_Release,
};

static test_VariantClearImpl test_myVariantClearImpl = {&test_VariantClear_vtbl, 1, 0};

static void test_VariantClear(void)
{
  HRESULT hres;
  VARIANTARG v;
  VARIANT v2;
  size_t i;
  long i4;
  IUnknown *punk;

#if 0
  /* Crashes: Native does not test input for NULL, so neither does Wine */
  hres = VariantClear(NULL);
#endif

  /* Only the type field is set, to VT_EMPTY */
  V_VT(&v) = VT_UI4;
  V_UI4(&v) = ~0u;
  hres = VariantClear(&v);
  ok((hres == S_OK && V_VT(&v) == VT_EMPTY) ||
     (IS_ANCIENT && hres == DISP_E_BADVARTYPE && V_VT(&v) == VT_UI4),
     "VariantClear: Type set to %d, res %08lx\n", V_VT(&v), hres);
  ok(V_UI4(&v) == ~0u, "VariantClear: Overwrote value\n");

  /* Test all possible V_VT values.
   * Also demonstrates that null pointers in 'v' are not dereferenced.
   * Individual variant tests should test VariantClear() with non-NULL values.
   */
  for (i = 0; i < sizeof(ExtraFlags)/sizeof(ExtraFlags[0]) && !IS_ANCIENT; i++)
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
  V_UNKNOWN(&v) = (IUnknown*)&test_myVariantClearImpl;
  test_myVariantClearImpl.events = 0;
  hres = VariantClear(&v);
  ok(hres == S_OK, "ret %08lx\n", hres);
  ok(V_VT(&v) == 0, "vt %04x\n", V_VT(&v));
  ok(V_UNKNOWN(&v) == (IUnknown*)&test_myVariantClearImpl, "unknown %p\n", V_UNKNOWN(&v));
  /* Check that Release got called, but nothing else */
  ok(test_myVariantClearImpl.events ==  0x4, "Unexpected call. events %08lx\n", test_myVariantClearImpl.events);

  /* UNKNOWN BYREF */
  punk = (IUnknown*)&test_myVariantClearImpl;
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
  V_DISPATCH(&v) = (IDispatch*)&test_myVariantClearImpl;
  test_myVariantClearImpl.events = 0;
  hres = VariantClear(&v);
  ok(hres == S_OK, "ret %08lx\n", hres);
  ok(V_VT(&v) == 0, "vt %04x\n", V_VT(&v));
  ok(V_DISPATCH(&v) == (IDispatch*)&test_myVariantClearImpl, "dispatch %p\n", V_DISPATCH(&v));
  /* Check that Release got called, but nothing else */
  ok(test_myVariantClearImpl.events ==  0x4, "Unexpected call. events %08lx\n", test_myVariantClearImpl.events);

  /* DISPATCH BYREF */
  punk = (IUnknown*)&test_myVariantClearImpl;
  V_VT(&v) = VT_DISPATCH | VT_BYREF;
  V_DISPATCHREF(&v) = (IDispatch**)&punk;
  test_myVariantClearImpl.events = 0;
  hres = VariantClear(&v);
  ok(hres == S_OK, "ret %08lx\n", hres);
  ok(V_VT(&v) == 0, "vt %04x\n", V_VT(&v));
  ok(V_DISPATCHREF(&v) == (IDispatch**)&punk, "dispatch ref %p\n", V_DISPATCHREF(&v));
  /* Check that nothing got called */
  ok(test_myVariantClearImpl.events ==  0, "Unexpected call. events %08lx\n", test_myVariantClearImpl.events);
}

static void test_VariantCopy(void)
{
  VARIANTARG vSrc, vDst;
  VARTYPE vt;
  size_t i;
  HRESULT hres, hExpected;

  /* Establish that the failure/other cases are dealt with. Individual tests
   * for each type should verify that data is copied correctly, references
   * are updated, etc.
   */

  /* vSrc == vDst */
  for (i = 0; i < sizeof(ExtraFlags)/sizeof(ExtraFlags[0]) && !IS_ANCIENT; i++)
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

  for (i = 0; i < sizeof(ExtraFlags)/sizeof(ExtraFlags[0]) && !IS_ANCIENT; i++)
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
  for (i = 0; i < sizeof(ExtraFlags)/sizeof(ExtraFlags[0]) && !IS_ANCIENT; i++)
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
        ok(V_VT(&vDst) == (vt|ExtraFlags[i]),
           "Copy(bad src): expected vt = %d, got %d\n",
           vt | ExtraFlags[i], V_VT(&vDst));
    }
  }
}

/* Determine if a vt is valid for VariantCopyInd() */
static int IsValidVariantCopyIndVT(VARTYPE vt, VARTYPE extraFlags)
{
  int ret = 0;

  if ((extraFlags & VT_ARRAY) ||
     (vt > VT_NULL && vt != (VARTYPE)15 && vt < VT_VOID &&
     !(extraFlags & (VT_VECTOR|VT_RESERVED))))
  {
    ret = 1; /* ok */
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
  for (i = 0; i < sizeof(ExtraFlags)/sizeof(ExtraFlags[0]) && !IS_ANCIENT; i++)
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
          if (HAVE_OLEAUT32_I8)
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

  for (i = 0; i < sizeof(ExtraFlags)/sizeof(ExtraFlags[0]) && !IS_ANCIENT; i++)
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
  for (i = 0; i < sizeof(ExtraFlags)/sizeof(ExtraFlags[0]) && !IS_ANCIENT; i++)
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
          if (HAVE_OLEAUT32_I8)
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

static HRESULT (WINAPI *pVarParseNumFromStr)(OLECHAR*,LCID,ULONG,NUMPARSE*,BYTE*);

/* Macros for converting and testing the result of VarParseNumFromStr */
#define FAILDIG 255
#define CONVERTN(str,dig,flags) MultiByteToWideChar(CP_ACP,0,str,-1,buff,sizeof(buff)/sizeof(WCHAR)); \
  memset(rgb, FAILDIG, sizeof(rgb)); memset(&np,-1,sizeof(np)); np.cDig = dig; np.dwInFlags = flags; \
  hres = pVarParseNumFromStr(buff,lcid,LOCALE_NOUSEROVERRIDE,&np,rgb)
#define CONVERT(str,flags) CONVERTN(str,sizeof(rgb),flags)
#define EXPECT(a,b,c,d,e,f) ok(hres == (HRESULT)S_OK, "Call failed, hres = %08lx\n", hres); \
  if (hres == (HRESULT)S_OK) { \
    ok(np.cDig == (a), "Expected cDig = %d, got %d\n", (a), np.cDig); \
    ok(np.dwInFlags == (b), "Expected dwInFlags = 0x%lx, got 0x%lx\n", (ULONG)(b), np.dwInFlags); \
    ok(np.dwOutFlags == (c), "Expected dwOutFlags = 0x%lx, got 0x%lx\n", (ULONG)(c), np.dwOutFlags); \
    ok(np.cchUsed == (d), "Expected cchUsed = %d, got %d\n", (d), np.cchUsed); \
    ok(np.nBaseShift == (e), "Expected nBaseShift = %d, got %d\n", (e), np.nBaseShift); \
    ok(np.nPwr10 == (f), "Expected nPwr10 = %d, got %d\n", (f), np.nPwr10); \
  }
#define EXPECTRGB(a,b) ok(rgb[a] == b, "Digit[%d], expected %d, got %d\n", a, b, rgb[a])
#define EXPECTFAIL ok(hres == (HRESULT)DISP_E_TYPEMISMATCH, "Call succeeded, hres = %08lx\n", hres)
#define EXPECT2(a,b) EXPECTRGB(0,a); EXPECTRGB(1,b)

static void test_VarParseNumFromStr(void)
{
  HRESULT hres;
  OLECHAR buff[128];
  /* Ensure all tests are using the same locale characters for '$', ',' etc */
  LCID lcid = MAKELCID(MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),SORT_DEFAULT);
  NUMPARSE np;
  BYTE rgb[128];

  /** No flags **/

  CHECKPTR(VarParseNumFromStr);

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


  /* With flag, consumes whitespace */
  CONVERT(" 0", NUMPRS_LEADING_WHITE);
  EXPECT(1,NUMPRS_LEADING_WHITE,NUMPRS_LEADING_WHITE,2,0,0);
  EXPECT2(0,FAILDIG);

  /* Test TAB once, then assume it acts as space for all cases */
  CONVERT("\t0", NUMPRS_LEADING_WHITE);
  EXPECT(1,NUMPRS_LEADING_WHITE,NUMPRS_LEADING_WHITE,2,0,0);
  EXPECT2(0,FAILDIG);


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

  /* Doesn't recognise hex in .asm sytax */
  CONVERT("0h", NUMPRS_HEX_OCT);
  EXPECT(1,NUMPRS_HEX_OCT,0,1,0,0);
  EXPECT2(0,FAILDIG);

  /* Doesn't fail with valid leading string but no digits */
  CONVERT("0x", NUMPRS_HEX_OCT);
  EXPECT(1,NUMPRS_HEX_OCT,0,1,0,0);
  EXPECT2(0,FAILDIG);

  /* Doesn't recognise hex format humbers at all! */
  CONVERT("0x0", NUMPRS_HEX_OCT);
  EXPECT(1,NUMPRS_HEX_OCT,0,1,0,0);
  EXPECT2(0,FAILDIG);

  /* Doesn't recognise plain hex digits either */
  CONVERT("FE", NUMPRS_HEX_OCT);
  EXPECTFAIL;
  EXPECTRGB(0,FAILDIG);

  /* Octal */
  CONVERT("0100", NUMPRS_HEX_OCT);
  EXPECT(1,NUMPRS_HEX_OCT,0,4,0,2);
  EXPECTRGB(0,1);
  EXPECTRGB(1,0);
  EXPECTRGB(2,0);
  EXPECTRGB(3,FAILDIG);

  /* VB hex */
  CONVERT("&HF800", NUMPRS_HEX_OCT);
  EXPECT(4,NUMPRS_HEX_OCT,0x40,6,4,0);
  EXPECTRGB(0,15);
  EXPECTRGB(1,8);
  EXPECTRGB(2,0);
  EXPECTRGB(3,0);
  EXPECTRGB(4,FAILDIG);

  /* VB hex lower case and leading zero */
  CONVERT("&h0abcd", NUMPRS_HEX_OCT);
  EXPECT(4,NUMPRS_HEX_OCT,0x40,7,4,0);
  EXPECTRGB(0,10);
  EXPECTRGB(1,11);
  EXPECTRGB(2,12);
  EXPECTRGB(3,13);
  EXPECTRGB(4,FAILDIG);

  /* VB oct */
  CONVERT("&O300", NUMPRS_HEX_OCT);
  EXPECT(3,NUMPRS_HEX_OCT,0x40,5,3,0);
  EXPECTRGB(0,3);
  EXPECTRGB(1,0);
  EXPECTRGB(2,0);
  EXPECTRGB(3,FAILDIG);

  /* VB oct lower case and leading zero */
  CONVERT("&o0777", NUMPRS_HEX_OCT);
  EXPECT(3,NUMPRS_HEX_OCT,0x40,6,3,0);
  EXPECTRGB(0,7);
  EXPECTRGB(1,7);
  EXPECTRGB(2,7);
  EXPECTRGB(3,FAILDIG);

  /* VB oct char bigger than 7 */
  CONVERT("&o128", NUMPRS_HEX_OCT);
/*
  Native versions 2.x of oleaut32 allow this to succeed: later versions and Wine don't
  EXPECTFAIL;
  EXPECTRGB(0,FAILDIG);
*/
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

  /* With flag, thousands sep. and following digits consumed */
  CONVERT("1,000", NUMPRS_THOUSANDS);
  EXPECT(1,NUMPRS_THOUSANDS,NUMPRS_THOUSANDS,5,0,3);
  EXPECTRGB(0,1);

  /* With flag and decimal point, thousands sep. but not decimals consumed */
  CONVERT("1,000.0", NUMPRS_THOUSANDS);
  EXPECT(1,NUMPRS_THOUSANDS,NUMPRS_THOUSANDS,5,0,3);
  EXPECTRGB(0,1);

  /** NUMPRS_CURRENCY **/

  /* Without flag, chokes on currency sign */
  CONVERT("$11", 0);
  EXPECTFAIL;
  EXPECTRGB(0,FAILDIG);

  /* With flag, consumes currency sign */
  CONVERT("$11", NUMPRS_CURRENCY);
  EXPECT(2,NUMPRS_CURRENCY,NUMPRS_CURRENCY,3,0,0);
  EXPECT2(1,1);
  EXPECTRGB(2,FAILDIG);

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

  /* Thousands flag can only be used with currency */
  CONVERT("$1,234", NUMPRS_CURRENCY|NUMPRS_THOUSANDS);
  EXPECT(4,NUMPRS_CURRENCY|NUMPRS_THOUSANDS,NUMPRS_CURRENCY|NUMPRS_THOUSANDS,6,0,0);
  EXPECT2(1,2);
  EXPECTRGB(2,3);
  EXPECTRGB(3,4);
  EXPECTRGB(4,FAILDIG);

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

  /* Consumes only one decimal point */
  CONVERT("1.1.", NUMPRS_DECIMAL);
  EXPECT(2,NUMPRS_DECIMAL,NUMPRS_DECIMAL,3,0,-1);
  EXPECT2(1,1);
  EXPECTRGB(2,FAILDIG);

  /** NUMPRS_EXPONENT **/

  /* Without flag, doesn't consume exponent */
  CONVERT("1e1", 0);
  EXPECT(1,0,0,1,0,0);
  EXPECT2(1,FAILDIG);

  /* With flag, consumes exponent */
  CONVERT("1e1", NUMPRS_EXPONENT);
  EXPECT(1,NUMPRS_EXPONENT,NUMPRS_EXPONENT,3,0,1);
  EXPECT2(1,FAILDIG);

  /* Negative exponents are accepted without flags */
  CONVERT("1e-1", NUMPRS_EXPONENT);
  EXPECT(1,NUMPRS_EXPONENT,NUMPRS_EXPONENT,4,0,-1);
  EXPECT2(1,FAILDIG);

  /* As are positive exponents and leading exponent 0's */
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

  /* Leading whitepace and plus, doesn't consume trailing whitespace */
  CONVERT("+ 0 ", NUMPRS_LEADING_PLUS|NUMPRS_LEADING_WHITE);
  EXPECT(1,NUMPRS_LEADING_PLUS|NUMPRS_LEADING_WHITE,NUMPRS_LEADING_PLUS|NUMPRS_LEADING_WHITE,3,0,0);
  EXPECT2(0,FAILDIG);

  /* Order of whitepace and plus is unimportant */
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
}

static HRESULT (WINAPI *pVarNumFromParseNum)(NUMPARSE*,BYTE*,ULONG,VARIANT*);

/* Macros for converting and testing the result of VarNumFromParseNum */
#define SETRGB(indx,val) if (!indx) memset(rgb, FAILDIG, sizeof(rgb)); rgb[indx] = val
#undef CONVERT
#define CONVERT(a,b,c,d,e,f,bits) \
    np.cDig = (a); np.dwInFlags = (b); np.dwOutFlags = (c); np.cchUsed = (d); \
    np.nBaseShift = (e); np.nPwr10 = (f); hres = pVarNumFromParseNum(&np, rgb, bits, &vOut)
static const char *szFailOverflow = "Expected overflow, hres = %08lx\n";
#define EXPECT_OVERFLOW ok(hres == (HRESULT)DISP_E_OVERFLOW, szFailOverflow, hres)
static const char *szFailOk = "Call failed, hres = %08lx\n";
#define EXPECT_OK ok(hres == (HRESULT)S_OK, szFailOk, hres); \
  if (hres == (HRESULT)S_OK)
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
  ok(V_UI4(&vOut) == val, "Expected ui4 = %ld, got %ld\n", (ULONG)val, V_UI4(&vOut)); }
#define EXPECT_I8(high,low) EXPECT_OK { EXPECT_TYPE(VT_I8); \
  ok(V_I8(&vOut) == ((((LONG64)(high))<<32)|(low)), "Expected i8 = %lx%08lx, got %lx%08lx\n", \
     (LONG)(high), (LONG)(low), (LONG)(V_I8(&vOut)>>32), (LONG)V_I8(&vOut) ); }
#define EXPECT_UI8(val) EXPECT_OK { EXPECT_TYPE(VT_UI8); \
  ok(V_UI8(&vOut) == val, "Expected ui8 = %lld, got %lld\n", (ULONG64)val, V_UI8(&vOut)); }
#define EXPECT_R4(val) EXPECT_OK { EXPECT_TYPE(VT_R4); \
  ok(V_R4(&vOut) == val, "Expected r4 = %f, got %f\n", val, V_R4(&vOut)); }
#define EXPECT_R8(val) EXPECT_OK { EXPECT_TYPE(VT_R8); \
  ok(V_R8(&vOut) == val, "Expected r8 = %g, got %g\n", val, V_R8(&vOut)); }
#define CY_MULTIPLIER 10000
#define EXPECT_CY(val) EXPECT_OK { EXPECT_TYPE(VT_CY); \
  ok(V_CY(&vOut).int64 == (LONG64)(val * CY_MULTIPLIER), "Expected r8 = %lld, got %lld\n", (LONG64)val, V_CY(&vOut).int64); }

static void test_VarNumFromParseNum(void)
{
  HRESULT hres;
  NUMPARSE np;
  BYTE rgb[128];
  VARIANT vOut;

  CHECKPTR(VarNumFromParseNum);
    
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
  if (HAVE_OLEAUT32_I8)
  {
    /* We cannot use INTEGER_VTBITS as WinXP and Win2003 are broken(?). They
       truncate the number to the smallest integer size requested:
       CONVERT(16,0,0,16,4,0, INTEGER_VTBITS); EXPECT_I1((signed char)0xff); */
    CONVERT(16,0,0,16,4,0, VTBIT_I8); EXPECT_I8(0x7fffffff,0xffffffff);
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
  if (HAVE_OLEAUT32_I8)
  {
    /* We cannot use INTEGER_VTBITS as WinXP and Win2003 are broken(?). They
       truncate the number to the smallest integer size requested:
       CONVERT(16,0,0,16,4,0, INTEGER_VTBITS & ~VTBIT_I1);
       EXPECT_I2((signed short)0x0002); */
    CONVERT(16,0,0,16,4,0, VTBIT_I8); EXPECT_I8(0x80000000,0x00000002);
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
  if (HAVE_OLEAUT32_I8)
  {
    CONVERT(16,0,0,16,4,0, VTBIT_I8); EXPECT_I8(0x80000000,0x00000002);
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
}

static const char* szUdateFromDateFail = "%.16g expected %lx, %d,%d,%d,%d,%d,%d,%d  %d %d"
                                         ", got %lx, %d,%d,%d,%d,%d,%d,%d  %d %d\n";
#define DT2UD(dt,flags,r,d,m,y,h,mn,s,ms,dw,dy) \
  memset(&ud, 0, sizeof(ud)); \
  res = pVarUdateFromDate(dt, flags, &ud); \
  ok(r == res && (FAILED(r) || (ud.st.wYear == y && ud.st.wMonth == m && ud.st.wDay == d && \
     ud.st.wHour == h && ud.st.wMinute == mn && ud.st.wSecond == s && \
     ud.st.wMilliseconds == ms && ud.st.wDayOfWeek == dw && ud.wDayOfYear == dy)), \
     szUdateFromDateFail, dt, r, d, m, y, h, mn, s, ms, dw, dy, res, ud.st.wDay, ud.st.wMonth, \
     ud.st.wYear, ud.st.wHour, ud.st.wMinute, ud.st.wSecond, \
     ud.st.wMilliseconds, ud.st.wDayOfWeek, ud.wDayOfYear)

static void test_VarUdateFromDate(void)
{
  UDATE ud;
  HRESULT res;

  CHECKPTR(VarUdateFromDate);
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
}

#define UD2T(d,m,y,h,mn,s,ms,dw,dy,flags,r,dt) \
  ud.st.wYear = (y); ud.st.wMonth = (m); ud.st.wDay = (d); ud.st.wHour = (h); \
  ud.st.wMinute = (mn); ud.st.wSecond = (s); ud.st.wMilliseconds = (ms); \
  ud.st.wDayOfWeek = (dw); ud.wDayOfYear = (dy); \
  res = pVarDateFromUdate(&ud, (flags), &out); \
  ok((r) == res && (FAILED(r) || fabs(out-(dt)) < 1.0e-11), \
     "expected %lx, %.16g, got %lx, %.16g\n", r, dt, res, out)

static void test_VarDateFromUdate(void)
{
  UDATE ud;
  double out;
  HRESULT res;

  CHECKPTR(VarDateFromUdate);
  UD2T(1,1,1980,0,0,0,0,2,1,0,S_OK,29221.0);      /* 1 Jan 1980 */
  UD2T(2,1,1980,0,0,0,0,3,2,0,S_OK,29222.0);      /* 2 Jan 1980 */
  UD2T(31,12,1990,0,0,0,0,0,0,0,S_OK,33238.0);    /* 31 Dec 1990 */
  UD2T(31,12,90,0,0,0,0,0,0,0,S_OK,33238.0);      /* year < 100 is 1900+year! */
  UD2T(30,12,1899,0,0,0,0,6,364,0,S_OK,0.0);      /* 30 Dec 1899 - VT_DATE 0.0 */
  UD2T(1,1,100,0,0,0,0,0,0,0,S_OK,-657434.0);     /* 1 Jan 100 - Min */
  UD2T(31,12,9999,0,0,0,0,0,0,0,S_OK,2958465.0);  /* 31 Dec 9999 - Max */
  UD2T(1,1,10000,0,0,0,0,0,0,0,E_INVALIDARG,0.0); /* > 31 Dec 9999 => err  */

  UD2T(1,1,1980,18,1,16,0,2,1,0,S_OK,29221.75087962963); /* 6:18:02 PM */

  UD2T(0,1,1980,0,0,0,0,2,1,0,S_OK,29220.0);      /* Rolls back to 31 Dec 1899 */
  UD2T(1,13,1980,0,0,0,0,2,1,0,S_OK,29587.0);     /* Rolls fwd to 1/1/1981 */
}

#define ST2DT(d,m,y,h,mn,s,ms,r,dt) \
  st.wYear = y; st.wMonth = m; st.wDay = d; st.wHour = h; st.wMinute = mn; \
  st.wSecond = s; st.wMilliseconds = ms; st.wDayOfWeek = 0; \
  res = pSystemTimeToVariantTime(&st, &out); \
  ok(r == res && (!r || fabs(out-dt) < 1.0e-11), \
     "expected %d, %.16g, got %d, %.16g\n", r, dt, res, out)

static void test_SystemTimeToVariantTime(void)
{
  SYSTEMTIME st;
  double out;
  int res;

  CHECKPTR(SystemTimeToVariantTime);
  ST2DT(1,1,1980,0,0,0,0,TRUE,29221.0);
  ST2DT(2,1,1980,0,0,0,0,TRUE,29222.0);
  ST2DT(0,1,1980,0,0,0,0,TRUE,29220.0);   /* Rolls back to 31 Dec 1899 */
  ST2DT(1,13,1980,0,0,0,0,FALSE,29587.0); /* Fails on invalid month */
  ST2DT(31,12,90,0,0,0,0,TRUE,33238.0);   /* year < 100 is 1900+year! */
}

#define DT2ST(dt,r,d,m,y,h,mn,s,ms) \
  memset(&st, 0, sizeof(st)); \
  res = pVariantTimeToSystemTime(dt, &st); \
  ok(r == res && (!r || (st.wYear == y && st.wMonth == m && st.wDay == d && \
     st.wHour == h && st.wMinute == mn && st.wSecond == s && \
     st.wMilliseconds == ms)), \
     "%.16g expected %d, %d,%d,%d,%d,%d,%d,%d, got %d, %d,%d,%d,%d,%d,%d,%d\n", \
     dt, r, d, m, y, h, mn, s, ms, res, st.wDay, st.wMonth, st.wYear, \
     st.wHour, st.wMinute, st.wSecond, st.wMilliseconds)

static void test_VariantTimeToSystemTime(void)
{
  SYSTEMTIME st;
  int res;

  CHECKPTR(VariantTimeToSystemTime);
  DT2ST(29221.0,1,1,1,1980,0,0,0,0);
  DT2ST(29222.0,1,2,1,1980,0,0,0,0);
}

#define MKDOSDATE(d,m,y) ((d & 0x1f) | ((m & 0xf) << 5) | (((y-1980) & 0x7f) << 9))
#define MKDOSTIME(h,m,s) (((s>>1) & 0x1f) | ((m & 0x3f) << 5) | ((h & 0x1f) << 11))

static const char *szDosDateToVarTimeFail = "expected %d, %.16g, got %d, %.16g\n";
#define DOS2DT(d,m,y,h,mn,s,r,dt) out = 0.0; \
  dosDate = MKDOSDATE(d,m,y); \
  dosTime = MKDOSTIME(h,mn,s); \
  res = pDosDateTimeToVariantTime(dosDate, dosTime, &out); \
  ok(r == res && (!r || fabs(out-dt) < 1.0e-11), \
     szDosDateToVarTimeFail, r, dt, res, out)

static void test_DosDateTimeToVariantTime(void)
{
  USHORT dosDate, dosTime;
  double out;
  INT res;

  CHECKPTR(DosDateTimeToVariantTime);

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
  DOS2DT(1,1,1980,0,60,0,0,0.0);               /* Invalid seconds */
  DOS2DT(1,1,1980,23,0,0,1,29221.95833333333); /* 1/1/1980 11:00:00 PM */
  DOS2DT(1,1,1980,24,0,0,0,0.0);               /* Invalid hours */
}

#define DT2DOS(dt,r,d,m,y,h,mn,s) dosTime = dosDate = 0; \
  expDosDate = MKDOSDATE(d,m,y); \
  expDosTime = MKDOSTIME(h,mn,s); \
  res = pVariantTimeToDosDateTime(dt, &dosDate, &dosTime); \
  ok(r == res && (!r || (dosTime == expDosTime && dosDate == expDosDate)), \
     "%g: expected %d,%d(%d/%d/%d),%d(%d:%d:%d) got %d,%d(%d/%d/%d),%d(%d:%d:%d)\n", \
     dt, r, expDosDate, expDosDate & 0x1f, (expDosDate >> 5) & 0xf, 1980 + (expDosDate >> 9), \
     expDosTime, expDosTime >> 11, (expDosTime >> 5) & 0x3f, (expDosTime & 0x1f), \
     res, dosDate, dosDate & 0x1f, (dosDate >> 5) & 0xf, 1980 + (dosDate >> 9), \
     dosTime, dosTime >> 11, (dosTime >> 5) & 0x3f, (dosTime & 0x1f))

static void test_VariantTimeToDosDateTime(void)
{
  USHORT dosDate, dosTime, expDosDate, expDosTime;
  INT res;

  CHECKPTR(VariantTimeToDosDateTime);

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

#define FMT_NUMBER(vt,val) \
  VariantInit(&v); V_VT(&v) = vt; val(&v) = 1; \
  hres = pVarFormatNumber(&v,2,0,0,0,0,&str); \
  ok(hres == S_OK, "VarFormatNumber (vt %d): returned %8lx\n", vt, hres); \
  if (hres == S_OK) \
    ok(str && strcmpW(str,szResult1) == 0, \
       "VarFormatNumber (vt %d): string different\n", vt)

static void test_VarFormatNumber(void)
{
  static const WCHAR szSrc1[] = { '1','\0' };
  static const WCHAR szResult1[] = { '1','.','0','0','\0' };
  static const WCHAR szSrc2[] = { '-','1','\0' };
  static const WCHAR szResult2[] = { '(','1','.','0','0',')','\0' };
  char buff[8];
  HRESULT hres;
  VARIANT v;
  BSTR str = NULL;

  CHECKPTR(VarFormatNumber);

  GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, buff, sizeof(buff)/sizeof(char));
  if (buff[0] != '.' || buff[1])
  {
    trace("Skipping VarFormatNumber tests as decimal separator is '%s'\n", buff);
    return;
  }

  FMT_NUMBER(VT_I1, V_I1);
  FMT_NUMBER(VT_UI1, V_UI1);
  FMT_NUMBER(VT_I2, V_I2);
  FMT_NUMBER(VT_UI2, V_UI2);
  FMT_NUMBER(VT_I4, V_I4);
  FMT_NUMBER(VT_UI4, V_UI4);
  if (HAVE_OLEAUT32_I8)
  {
    FMT_NUMBER(VT_I8, V_I8);
    FMT_NUMBER(VT_UI8, V_UI8);
  }
  FMT_NUMBER(VT_R4, V_R4);
  FMT_NUMBER(VT_R8, V_R8);
  FMT_NUMBER(VT_BOOL, V_BOOL);

  V_VT(&v) = VT_BSTR;
  V_BSTR(&v) = SysAllocString(szSrc1);

  hres = pVarFormatNumber(&v,2,0,0,0,0,&str);
  ok(hres == S_OK, "VarFormatNumber (bstr): returned %8lx\n", hres);
  if (hres == S_OK)
    ok(str && strcmpW(str, szResult1) == 0, "VarFormatNumber (bstr): string different\n");
  SysFreeString(V_BSTR(&v));
  SysFreeString(str);

  V_BSTR(&v) = SysAllocString(szSrc2);
  hres = pVarFormatNumber(&v,2,0,-1,0,0,&str);
  ok(hres == S_OK, "VarFormatNumber (bstr): returned %8lx\n", hres);
  if (hres == S_OK)
    ok(str && strcmpW(str, szResult2) == 0, "VarFormatNumber (-bstr): string different\n");
  SysFreeString(V_BSTR(&v));
  SysFreeString(str);
}

#define SIGNED_VTBITS (VTBIT_I1|VTBIT_I2|VTBIT_I4|VTBIT_I8|VTBIT_R4|VTBIT_R8)

static const char *szVarFmtFail = "VT %d|0x%04x Format %s: expected 0x%08lx, '%s', got 0x%08lx, '%s'\n";
#define VARFMT(vt,v,val,fmt,ret,str) do { \
  if (out) SysFreeString(out); out = NULL; \
  V_VT(&in) = (vt); v(&in) = val; \
  if (fmt) MultiByteToWideChar(CP_ACP, 0, fmt, -1, buffW, sizeof(buffW)/sizeof(WCHAR)); \
  hres = pVarFormat(&in,fmt ? buffW : NULL,fd,fw,flags,&out); \
  if (SUCCEEDED(hres)) WideCharToMultiByte(CP_ACP, 0, out, -1, buff, sizeof(buff),0,0); \
  else buff[0] = '\0'; \
  ok(hres == ret && (FAILED(ret) || !strcmp(buff, str)), \
     szVarFmtFail, \
     (vt)&VT_TYPEMASK,(vt)&~VT_TYPEMASK,fmt?fmt:"<null>",ret,str,hres,buff); \
  } while(0)

typedef struct tagFMTRES
{
  LPCSTR fmt;
  LPCSTR one_res;
  LPCSTR zero_res;
} FMTRES;

static const FMTRES VarFormat_results[] =
{
  { NULL, "1", "0" },
  { "", "1", "0" },
  { "General Number", "1", "0" },
  { "Percent", "100.00%", "0.00%" },
  { "Standard", "1.00", "0.00" },
  { "Scientific","1.00E+00", "0.00E+00" },
  { "True/False", "True", "False" },
  { "On/Off", "On", "Off" },
  { "Yes/No", "Yes", "No" },
  { "#", "1", "" },
  { "##", "1", "" },
  { "#.#", "1.", "." },
  { "0", "1", "0" },
  { "00", "01", "00" },
  { "0.0", "1.0", "0.0" },
  { "00\\c\\o\\p\\y", "01copy","00copy" },
  { "\"pos\";\"neg\"", "pos", "pos" },
  { "\"pos\";\"neg\";\"zero\"","pos", "zero" }
};

typedef struct tagFMTDATERES
{
  DATE   val;
  LPCSTR fmt;
  LPCSTR res;
} FMTDATERES;

static const FMTDATERES VarFormat_date_results[] =
{
  { 0.0, "w", "7" },
  { 0.0, "w", "6" },
  { 0.0, "w", "5" },
  { 0.0, "w", "4" },
  { 0.0, "w", "3" },
  { 0.0, "w", "2" },
  { 0.0, "w", "1" }, /* First 7 entries must remain in this order! */
  { 2.525, "am/pm", "pm" },
  { 2.525, "AM/PM", "PM" },
  { 2.525, "A/P", "P" },
  { 2.525, "a/p", "p" },
  { 2.525, "q", "1" },
  { 2.525, "d", "1" },
  { 2.525, "dd", "01" },
  { 2.525, "ddd", "Mon" },
  { 2.525, "dddd", "Monday" },
  { 2.525, "mmm", "Jan" },
  { 2.525, "mmmm", "January" },
  { 2.525, "y", "1" },
  { 2.525, "yy", "00" },
  { 2.525, "yyy", "001" },
  { 2.525, "yyyy", "1900" },
  { 2.525, "dd mm yyyy hh:mm:ss", "01 01 1900 12:36:00" },
  { 2.525, "dd mm yyyy mm", "01 01 1900 01" },
  { 2.525, "dd mm yyyy :mm", "01 01 1900 :01" },
  { 2.525, "dd mm yyyy hh:mm", "01 01 1900 12:36" },
  { 2.525, "mm mm", "01 01" },
  { 2.525, "mm :mm:ss", "01 :01:00" },
  { 2.525, "mm :ss:mm", "01 :00:01" },
  { 2.525, "hh:mm :ss:mm", "12:36 :00:01" },
  { 2.525, "hh:dd :mm:mm", "12:01 :01:01" },
  { 2.525, "dd:hh :mm:mm", "01:12 :36:01" },
  { 2.525, "hh :mm:mm", "12 :36:01" },
  { 2.525, "dd :mm:mm", "01 :01:01" },
  { 2.525, "dd :mm:nn", "01 :01:36" },
  { 2.725, "hh:nn:ss A/P", "05:24:00 P" }
};

#define VNUMFMT(vt,v) \
  for (i = 0; i < sizeof(VarFormat_results)/sizeof(FMTRES); i++) \
  { \
    VARFMT(vt,v,1,VarFormat_results[i].fmt,S_OK,VarFormat_results[i].one_res); \
    VARFMT(vt,v,0,VarFormat_results[i].fmt,S_OK,VarFormat_results[i].zero_res); \
  } \
  if ((1 << vt) & SIGNED_VTBITS) \
  { \
    VARFMT(vt,v,-1,"\"pos\";\"neg\"",S_OK,"neg"); \
    VARFMT(vt,v,-1,"\"pos\";\"neg\";\"zero\"",S_OK,"neg"); \
  }

static void test_VarFormat(void)
{
  static const WCHAR szTesting[] = { 't','e','s','t','i','n','g','\0' };
  size_t i;
  WCHAR buffW[256];
  char buff[256];
  VARIANT in;
  VARIANT_BOOL bTrue = VARIANT_TRUE, bFalse = VARIANT_FALSE;
  int fd = 0, fw = 0;
  ULONG flags = 0;
  BSTR bstrin, out = NULL;
  HRESULT hres;

  CHECKPTR(VarFormat);

  if (PRIMARYLANGID(LANGIDFROMLCID(GetUserDefaultLCID())) != LANG_ENGLISH)
  {
    trace("Skipping VarFormat tests for non english language\n");
    return;
  }
  GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, buff, sizeof(buff)/sizeof(char));
  if (buff[0] != '.' || buff[1])
  {
    trace("Skipping VarFormat tests as decimal separator is '%s'\n", buff);
    return;
  }
  GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_IDIGITS, buff, sizeof(buff)/sizeof(char));
  if (buff[0] != '2' || buff[1])
  {
    trace("Skipping VarFormat tests as decimal places is '%s'\n", buff);
    return;
  }

  VARFMT(VT_BOOL,V_BOOL,VARIANT_TRUE,"True/False",S_OK,"True");
  VARFMT(VT_BOOL,V_BOOL,VARIANT_FALSE,"True/False",S_OK,"False");

  VNUMFMT(VT_I1,V_I1);
  VNUMFMT(VT_I2,V_I2);
  VNUMFMT(VT_I4,V_I4);
  if (HAVE_OLEAUT32_I8)
  {
    VNUMFMT(VT_I8,V_I8);
  }
  VNUMFMT(VT_INT,V_INT);
  VNUMFMT(VT_UI1,V_UI1);
  VNUMFMT(VT_UI2,V_UI2);
  VNUMFMT(VT_UI4,V_UI4);
  if (HAVE_OLEAUT32_I8)
  {
    VNUMFMT(VT_UI8,V_UI8);
  }
  VNUMFMT(VT_UINT,V_UINT);
  VNUMFMT(VT_R4,V_R4);
  VNUMFMT(VT_R8,V_R8);

  /* Reference types are dereferenced */
  VARFMT(VT_BOOL|VT_BYREF,V_BOOLREF,&bTrue,"True/False",S_OK,"True");
  VARFMT(VT_BOOL|VT_BYREF,V_BOOLREF,&bFalse,"True/False",S_OK,"False");

  /* Dates */
  for (i = 0; i < sizeof(VarFormat_date_results)/sizeof(FMTDATERES); i++)
  {
    if (i < 7)
      fd = i + 1; /* Test first day */
    else
      fd = 0;
    VARFMT(VT_DATE,V_DATE,VarFormat_date_results[i].val,
           VarFormat_date_results[i].fmt,S_OK,
           VarFormat_date_results[i].res);
  }

  /* Strings */
  bstrin = SysAllocString(szTesting);
  VARFMT(VT_BSTR,V_BSTR,bstrin,"",S_OK,"testing");
  VARFMT(VT_BSTR,V_BSTR,bstrin,"@",S_OK,"testing");
  VARFMT(VT_BSTR,V_BSTR,bstrin,"&",S_OK,"testing");
  VARFMT(VT_BSTR,V_BSTR,bstrin,"\\x@\\x@",S_OK,"xtxesting");
  VARFMT(VT_BSTR,V_BSTR,bstrin,"\\x&\\x&",S_OK,"xtxesting");
  VARFMT(VT_BSTR,V_BSTR,bstrin,"@\\x",S_OK,"txesting");
  VARFMT(VT_BSTR,V_BSTR,bstrin,"@@@@@@@@",S_OK," testing");
  VARFMT(VT_BSTR,V_BSTR,bstrin,"@\\x@@@@@@@",S_OK," xtesting");
  VARFMT(VT_BSTR,V_BSTR,bstrin,"&&&&&&&&",S_OK,"testing");
  VARFMT(VT_BSTR,V_BSTR,bstrin,"!&&&&&&&",S_OK,"testing");
  VARFMT(VT_BSTR,V_BSTR,bstrin,"&&&&&&&!",S_OK,"testing");
  VARFMT(VT_BSTR,V_BSTR,bstrin,">&&",S_OK,"TESTING");
  VARFMT(VT_BSTR,V_BSTR,bstrin,"<&&",S_OK,"testing");
  VARFMT(VT_BSTR,V_BSTR,bstrin,"<&>&",S_OK,"testing");
  SysFreeString(bstrin);
  /* Numeric values are converted to strings then output */
  VARFMT(VT_I1,V_I1,1,"<&>&",S_OK,"1");

  /* Number formats */
  VARFMT(VT_I4,V_I4,1,"#00000000",S_OK,"00000001");
  VARFMT(VT_I4,V_I4,1,"000###",S_OK,"000001");
  VARFMT(VT_I4,V_I4,1,"#00##00#0",S_OK,"00000001");
  VARFMT(VT_I4,V_I4,1,"1#####0000",S_OK,"10001");
  todo_wine {
  VARFMT(VT_I4,V_I4,100000,"#,###,###,###",S_OK,"100,000");
  }
  VARFMT(VT_R8,V_R8,1.23456789,"0#.0#0#0#0#0",S_OK,"01.234567890");
  VARFMT(VT_R8,V_R8,1.2,"0#.0#0#0#0#0",S_OK,"01.200000000");
  VARFMT(VT_R8,V_R8,9.87654321,"#0.#0#0#0#0#",S_OK,"9.87654321");
  VARFMT(VT_R8,V_R8,9.8,"#0.#0#0#0#0#",S_OK,"9.80000000");
  VARFMT(VT_R8,V_R8,0.00000008,"#0.#0#0#0#0#0",S_OK,"0.0000000800");
  VARFMT(VT_R8,V_R8,0.00010705,"#0.##########",S_OK,"0.00010705");
  VARFMT(VT_I4,V_I4,17,"#0",S_OK,"17");
  VARFMT(VT_I4,V_I4,4711,"#0",S_OK,"4711");
  VARFMT(VT_I4,V_I4,17,"#00",S_OK,"17");
  VARFMT(VT_I4,V_I4,100,"0##",S_OK,"100");
  VARFMT(VT_I4,V_I4,17,"#000",S_OK,"017");
  VARFMT(VT_I4,V_I4,17,"#0.00",S_OK,"17.00");
  VARFMT(VT_I4,V_I4,17,"#0000.00",S_OK,"0017.00");
  VARFMT(VT_I4,V_I4,17,"#.00",S_OK,"17.00");
  VARFMT(VT_R8,V_R8,1.7,"#.00",S_OK,"1.70");
  VARFMT(VT_R8,V_R8,.17,"#.00",S_OK,".17");
  VARFMT(VT_I4,V_I4,17,"#3",S_OK,"173");
  VARFMT(VT_I4,V_I4,17,"#33",S_OK,"1733");
  VARFMT(VT_I4,V_I4,17,"#3.33",S_OK,"173.33");
  VARFMT(VT_I4,V_I4,17,"#3333.33",S_OK,"173333.33");
  VARFMT(VT_I4,V_I4,17,"#.33",S_OK,"17.33");
  VARFMT(VT_R8,V_R8,.17,"#.33",S_OK,".33");
  VARFMT(VT_R8,V_R8,1.7,"0.0000E-000",S_OK,"1.7000E000");
  VARFMT(VT_R8,V_R8,1.7,"0.0000e-1",S_OK,"1.7000e01");
  VARFMT(VT_R8,V_R8,86.936849,"#0.000000000000e-000",S_OK,"86.936849000000e000");
  VARFMT(VT_R8,V_R8,1.7,"#0",S_OK,"2");
  VARFMT(VT_R8,V_R8,1.7,"#.33",S_OK,"2.33");
  VARFMT(VT_R8,V_R8,1.7,"#3",S_OK,"23");
  VARFMT(VT_R8,V_R8,1.73245,"0.0000E+000",S_OK,"1.7325E+000");
  VARFMT(VT_R8,V_R8,9.9999999,"#0.000000",S_OK,"10.000000");
  VARFMT(VT_R8,V_R8,1.7,"0.0000e+0#",S_OK,"1.7000e+0");
  VARFMT(VT_R8,V_R8,100.0001e+0,"0.0000E+0",S_OK,"1.0000E+2");
  VARFMT(VT_R8,V_R8,1000001,"0.0000e+1",S_OK,"1.0000e+61");
  VARFMT(VT_R8,V_R8,100.0001e+25,"0.0000e+0",S_OK,"1.0000e+27");
  VARFMT(VT_R8,V_R8,450.0001e+43,"#000.0000e+0",S_OK,"4500.0010e+42");
  VARFMT(VT_R8,V_R8,0.0001e-11,"##00.0000e-0",S_OK,"1000.0000e-18");
  VARFMT(VT_R8,V_R8,0.0317e-11,"0000.0000e-0",S_OK,"3170.0000e-16");
  VARFMT(VT_R8,V_R8,0.0021e-11,"00##.0000e-0",S_OK,"2100.0000e-17");
  VARFMT(VT_R8,V_R8,1.0001e-27,"##00.0000e-0",S_OK,"1000.1000e-30");
  VARFMT(VT_R8,V_R8,47.11,".0000E+0",S_OK,".4711E+2");
  VARFMT(VT_R8,V_R8,3.0401e-13,"#####.####e-0%",S_OK,"30401.e-15%");


  /* 'out' is not cleared */
  out = (BSTR)0x1;
  pVarFormat(&in,NULL,fd,fw,flags,&out); /* Would crash if out is cleared */
  out = NULL;

  /* VT_NULL */
  V_VT(&in) = VT_NULL;
  hres = pVarFormat(&in,NULL,fd,fw,0,&out);
  ok(hres == S_OK, "VarFormat failed with 0x%08lx\n", hres);
  ok(out == NULL, "expected NULL formatted string\n");

  /* Invalid args */
  hres = pVarFormat(&in,NULL,fd,fw,flags,NULL);
  ok(hres == E_INVALIDARG, "Null out: expected E_INVALIDARG, got 0x%08lx\n", hres);
  hres = pVarFormat(NULL,NULL,fd,fw,flags,&out);
  ok(hres == E_INVALIDARG, "Null in: expected E_INVALIDARG, got 0x%08lx\n", hres);
  fd = -1;
  VARFMT(VT_BOOL,V_BOOL,VARIANT_TRUE,"",E_INVALIDARG,"");
  fd = 8;
  VARFMT(VT_BOOL,V_BOOL,VARIANT_TRUE,"",E_INVALIDARG,"");
  fd = 0; fw = -1;
  VARFMT(VT_BOOL,V_BOOL,VARIANT_TRUE,"",E_INVALIDARG,"");
  fw = 4;
  VARFMT(VT_BOOL,V_BOOL,VARIANT_TRUE,"",E_INVALIDARG,"");
}

static HRESULT (WINAPI *pVarAbs)(LPVARIANT,LPVARIANT);

static const char *szVarAbsFail = "VarAbs: expected 0x0,%d,%d, got 0x%lX,%d,%d\n";
#define VARABS(vt,val,rvt,rval) V_VT(&v) = VT_##vt; V_##vt(&v) = val; \
        memset(&vDst,0,sizeof(vDst)); hres = pVarAbs(&v,&vDst); \
        ok(hres == S_OK && V_VT(&vDst) == VT_##rvt && V_##rvt(&vDst) == (rval), \
           szVarAbsFail, VT_##rvt, (int)(rval), \
           hres, V_VT(&vDst), (int)V_##rvt(&vDst))

static void test_VarAbs(void)
{
    static const WCHAR szNum[] = {'-','1','.','1','\0' };
    char buff[8];
    HRESULT hres;
    VARIANT v, vDst;
    size_t i;

    CHECKPTR(VarAbs);

    /* Test all possible V_VT values.
     */
    for (i = 0; i < sizeof(ExtraFlags)/sizeof(ExtraFlags[0]); i++)
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
            if (ExtraFlags[i] & (VT_ARRAY|VT_ARRAY) ||
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
    VARABS(DATE,1,DATE,1);
    VARABS(DATE,-1,DATE,1);
    V_VT(&v) = VT_CY;
    V_CY(&v).int64 = -10000;
    memset(&vDst,0,sizeof(vDst));
    hres = pVarAbs(&v,&vDst);
    ok(hres == S_OK && V_VT(&vDst) == VT_CY && V_CY(&vDst).int64 == 10000,
       "VarAbs(CY): expected 0x0 got 0x%lX\n", hres);
    GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, buff, sizeof(buff)/sizeof(char));
    if (buff[0] != '.' || buff[1])
    {
        trace("Skipping VarAbs(BSTR) as decimal separator is '%s'\n", buff);
        return;
    }
    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = (BSTR)szNum;
    memset(&vDst,0,sizeof(vDst));
    hres = pVarAbs(&v,&vDst);
    ok(hres == S_OK && V_VT(&vDst) == VT_R8 && V_R8(&vDst) == 1.1,
       "VarAbs: expected 0x0,%d,%g, got 0x%lX,%d,%g\n", VT_R8, 1.1, hres, V_VT(&vDst), V_R8(&vDst));
}

static HRESULT (WINAPI *pVarNot)(LPVARIANT,LPVARIANT);

static const char *szVarNotFail = "VarNot: expected 0x0,%d,%d, got 0x%lX,%d,%d\n";
#define VARNOT(vt,val,rvt,rval) V_VT(&v) = VT_##vt; V_##vt(&v) = val; \
        memset(&vDst,0,sizeof(vDst)); hres = pVarNot(&v,&vDst); \
        ok(hres == S_OK && V_VT(&vDst) == VT_##rvt && V_##rvt(&vDst) == (rval), \
        szVarNotFail, VT_##rvt, (int)(rval), \
        hres, V_VT(&vDst), (int)V_##rvt(&vDst))

static void test_VarNot(void)
{
    static const WCHAR szNum0[] = {'0','\0' };
    static const WCHAR szNum1[] = {'1','\0' };
    HRESULT hres;
    VARIANT v, vDst;
    DECIMAL *pdec = &V_DECIMAL(&v);
    CY *pcy = &V_CY(&v);
    size_t i;

    CHECKPTR(VarNot);

    /* Test all possible V_VT values */
    for (i = 0; i < sizeof(ExtraFlags)/sizeof(ExtraFlags[0]); i++)
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
                if (HAVE_OLEAUT32_I8)
                    hExpected = S_OK;
                break;
            case VT_RECORD:
                if (HAVE_OLEAUT32_RECORD)
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
    if (HAVE_OLEAUT32_I8)
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

    V_VT(&v) = VT_DECIMAL;
    S(U(*pdec)).sign = DECIMAL_NEG;
    S(U(*pdec)).scale = 0;
    pdec->Hi32 = 0;
    S1(U1(*pdec)).Mid32 = 0;
    S1(U1(*pdec)).Lo32 = 1;
    VARNOT(DECIMAL,*pdec,I4,0);

    pcy->int64 = 10000;
    VARNOT(CY,*pcy,I4,-2);

    pcy->int64 = 0;
    VARNOT(CY,*pcy,I4,-1);

    pcy->int64 = -1;
    VARNOT(CY,*pcy,I4,-1);
}
static HRESULT (WINAPI *pVarSub)(LPVARIANT,LPVARIANT,LPVARIANT);

static void test_VarSub(void)
{
    VARIANT va, vb, vc;
    HRESULT hr;

    CHECKPTR(VarSub);

    V_VT(&va) = VT_DATE;
    V_DATE(&va) = 200000.0;
    V_VT(&vb) = VT_DATE;
    V_DATE(&vb) = 100000.0;

    hr = pVarSub(&va, &vb, &vc);
    ok(hr == S_OK,"VarSub of VT_DATE - VT_DATE failed with %lx\n", hr);
    ok(V_VT(&vc) == VT_R8,"VarSub of VT_DATE - VT_DATE returned vt 0x%x\n", V_VT(&vc));
    ok(((V_R8(&vc) >  99999.9) && (V_R8(&vc) < 100000.1)),"VarSub of VT_DATE - VT_DATE  should return 100000.0, but returned %g\n", V_R8(&vc));
    /* fprintf(stderr,"VarSub of 10000-20000 returned: %g\n", V_R8(&vc)); */
}

static const char *szVarModFail = "VarMod: expected 0x%lx,%d(%s),%d, got 0x%lX,%d(%s),%d\n";
#define VARMOD(vt1,vt2,val1,val2,rvt,rval,hexpected) V_VT(&v1) = VT_##vt1; V_##vt1(&v1) = val1; \
	V_VT(&v2) = VT_##vt2; V_##vt2(&v2) = val2;                  \
        memset(&vDst,0,sizeof(vDst)); hres = pVarMod(&v1,&v2,&vDst);			\
        ok(hres == hexpected && V_VT(&vDst) == VT_##rvt && V_##rvt(&vDst) == (rval), \
        szVarModFail, hexpected, VT_##rvt, vtstr(VT_##rvt), (int)(rval), \
        hres, V_VT(&vDst), vtstr(V_VT(&vDst)), (int)V_##rvt(&vDst))

static const char *szVarMod2Fail = "VarMod: expected 0x%lx,%d(%s),%d, got 0x%lX,%d(%s),%d\n";
#define VARMOD2(vt1,vt2,val1,val2,rvt,rval,hexpected) V_VT(&v1) = VT_##vt1; V_I4(&v1) = val1; \
	V_VT(&v2) = VT_##vt2; V_I4(&v2) = val2;                                \
        memset(&vDst,0,sizeof(vDst)); hres = pVarMod(&v1,&v2,&vDst);                     \
        ok(hres == hexpected && V_VT(&vDst) == VT_##rvt && V_I4(&vDst) == (rval), \
        szVarMod2Fail, hexpected, VT_##rvt, vtstr(VT_##rvt), (int)(rval), \
        hres, V_VT(&vDst), vtstr(V_VT(&vDst)), (int)V_I4(&vDst))

static HRESULT (WINAPI *pVarMod)(LPVARIANT,LPVARIANT,LPVARIANT);

static void test_VarMod(void)
{
  VARIANT v1, v2, vDst;
  HRESULT hres;
  HRESULT hexpected = 0;
  static const WCHAR szNum0[] = {'1','2','5','\0'};
  static const WCHAR szNum1[] = {'1','0','\0'};
  int l, r;
  BOOL lFound, rFound;
  BOOL lValid, rValid;
  BSTR strNum0, strNum1;

  CHECKPTR(VarMod);

  VARMOD(I1,BOOL,100,10,I4,0,S_OK);
  VARMOD(I1,I1,100,10,I4,0,S_OK);
  VARMOD(I1,UI1,100,10,I4,0,S_OK);
  VARMOD(I1,I2,100,10,I4,0,S_OK);
  VARMOD(I1,UI2,100,10,I4,0,S_OK);
  VARMOD(I1,I4,100,10,I4,0,S_OK);
  VARMOD(I1,UI4,100,10,I4,0,S_OK);
  VARMOD(I1,R4,100,10,I4,0,S_OK);
  VARMOD(I1,R8,100,10,I4,0,S_OK);

  VARMOD(UI1,BOOL,100,10,I2,0,S_OK);
  VARMOD(UI1,I1,100,10,I4,0,S_OK);
  VARMOD(UI1,UI1,100,10,UI1,0,S_OK);
  VARMOD(UI1,I2,100,10,I2,0,S_OK);
  VARMOD(UI1,UI2,100,10,I4,0,S_OK);
  VARMOD(UI1,I4,100,10,I4,0,S_OK);
  VARMOD(UI1,UI4,100,10,I4,0,S_OK);
  VARMOD(UI1,R4,100,10,I4,0,S_OK);
  VARMOD(UI1,R8,100,10,I4,0,S_OK);

  VARMOD(I2,BOOL,100,10,I2,0,S_OK);
  VARMOD(I2,I1,100,10,I4,0,S_OK);
  VARMOD(I2,UI1,100,10,I2,0,S_OK);
  VARMOD(I2,I2,100,10,I2,0,S_OK);
  VARMOD(I2,UI2,100,10,I4,0,S_OK);
  VARMOD(I2,I4,100,10,I4,0,S_OK);
  VARMOD(I2,UI4,100,10,I4,0,S_OK);
  VARMOD(I2,R4,100,10,I4,0,S_OK);
  VARMOD(I2,R8,100,10,I4,0,S_OK);

  VARMOD(I4,BOOL,100,10,I4,0,S_OK);
  VARMOD(I4,I1,100,10,I4,0,S_OK);
  VARMOD(I4,UI1,100,10,I4,0,S_OK);
  VARMOD(I4,I2,100,10,I4,0,S_OK);
  VARMOD(I4,UI2,100,10,I4,0,S_OK);
  VARMOD(I4,I4,100,10,I4,0,S_OK);
  VARMOD(I4,UI4,100,10,I4,0,S_OK);
  VARMOD(I4,R4,100,10,I4,0,S_OK);
  VARMOD(I4,R8,100,10,I4,0,S_OK);
  VARMOD(UI4,BOOL,100,10,I4,0,S_OK);
  VARMOD(UI4,I1,100,10,I4,0,S_OK);
  VARMOD(UI4,UI1,100,10,I4,0,S_OK);
  VARMOD(UI4,I2,100,10,I4,0,S_OK);
  VARMOD(UI4,UI2,100,10,I4,0,S_OK);
  VARMOD(UI4,I4,100,10,I4,0,S_OK);
  VARMOD(UI4,UI4,100,10,I4,0,S_OK);
  VARMOD(UI4,R4,100,10,I4,0,S_OK);
  VARMOD(UI4,R8,100,10,I4,0,S_OK);
  VARMOD(R4,BOOL,100,10,I4,0,S_OK);
  VARMOD(R4,I1,100,10,I4,0,S_OK);
  VARMOD(R4,UI1,100,10,I4,0,S_OK);
  VARMOD(R4,I2,100,10,I4,0,S_OK);
  VARMOD(R4,UI2,100,10,I4,0,S_OK);
  VARMOD(R4,I4,100,10,I4,0,S_OK);
  VARMOD(R4,UI4,100,10,I4,0,S_OK);
  VARMOD(R4,R4,100,10,I4,0,S_OK);
  VARMOD(R4,R8,100,10,I4,0,S_OK);
  VARMOD(R8,BOOL,100,10,I4,0,S_OK);
  VARMOD(R8,I1,100,10,I4,0,S_OK);
  VARMOD(R8,UI1,100,10,I4,0,S_OK);
  VARMOD(R8,I2,100,10,I4,0,S_OK);
  VARMOD(R8,UI2,100,10,I4,0,S_OK);
  VARMOD(R8,I4,100,10,I4,0,S_OK);
  VARMOD(R8,UI4,100,10,I4,0,S_OK);
  VARMOD(R8,R4,100,10,I4,0,S_OK);
  VARMOD(R8,R8,100,10,I4,0,S_OK);

  VARMOD(INT,INT,100,10,I4,0,S_OK);
  VARMOD(INT,UINT,100,10,I4,0,S_OK);

  VARMOD(BOOL,BOOL,100,10,I2,0,S_OK);
  VARMOD(BOOL,I1,100,10,I4,0,S_OK);
  VARMOD(BOOL,UI1,100,10,I2,0,S_OK);
  VARMOD(BOOL,I2,100,10,I2,0,S_OK);
  VARMOD(BOOL,UI2,100,10,I4,0,S_OK);
  VARMOD(BOOL,I4,100,10,I4,0,S_OK);
  VARMOD(BOOL,UI4,100,10,I4,0,S_OK);
  VARMOD(BOOL,R4,100,10,I4,0,S_OK);
  VARMOD(BOOL,R8,100,10,I4,0,S_OK);
  VARMOD(BOOL,DATE,100,10,I4,0,S_OK);

  VARMOD(DATE,BOOL,100,10,I4,0,S_OK);
  VARMOD(DATE,I1,100,10,I4,0,S_OK);
  VARMOD(DATE,UI1,100,10,I4,0,S_OK);
  VARMOD(DATE,I2,100,10,I4,0,S_OK);
  VARMOD(DATE,UI2,100,10,I4,0,S_OK);
  VARMOD(DATE,I4,100,10,I4,0,S_OK);
  VARMOD(DATE,UI4,100,10,I4,0,S_OK);
  VARMOD(DATE,R4,100,10,I4,0,S_OK);
  VARMOD(DATE,R8,100,10,I4,0,S_OK);
  VARMOD(DATE,DATE,100,10,I4,0,S_OK);

  strNum0 = SysAllocString(szNum0);
  strNum1 = SysAllocString(szNum1);
  VARMOD(BSTR,BSTR,strNum0,strNum1,I4,5,S_OK);
  VARMOD(BSTR,I1,strNum0,10,I4,5,S_OK);
  VARMOD(BSTR,I2,strNum0,10,I4,5,S_OK);
  VARMOD(BSTR,I4,strNum0,10,I4,5,S_OK);
  VARMOD(BSTR,R4,strNum0,10,I4,5,S_OK);
  VARMOD(BSTR,R8,strNum0,10,I4,5,S_OK);
  VARMOD(I4,BSTR,125,strNum1,I4,5,S_OK);

  if (HAVE_OLEAUT32_I8)
  {
    VARMOD(BOOL,I8,100,10,I8,0,S_OK);
    VARMOD(I1,I8,100,10,I8,0,S_OK);
    VARMOD(UI1,I8,100,10,I8,0,S_OK);
    VARMOD(I2,I8,100,10,I8,0,S_OK);
    VARMOD(I4,I8,100,10,I8,0,S_OK);
    VARMOD(UI4,I8,100,10,I8,0,S_OK);
    VARMOD(R4,I8,100,10,I8,0,S_OK);
    VARMOD(R8,I8,100,10,I8,0,S_OK);
    VARMOD(DATE,I8,100,10,I8,0,S_OK);

    VARMOD(I8,BOOL,100,10,I8,0,S_OK);
    VARMOD(I8,I1,100,10,I8,0,S_OK);
    VARMOD(I8,UI1,100,10,I8,0,S_OK);
    VARMOD(I8,I2,100,10,I8,0,S_OK);
    VARMOD(I8,UI2,100,10,I8,0,S_OK);
    VARMOD(I8,I4,100,10,I8,0,S_OK);
    VARMOD(I8,UI4,100,10,I8,0,S_OK);
    VARMOD(I8,R4,100,10,I8,0,S_OK);
    VARMOD(I8,R8,100,10,I8,0,S_OK);
    VARMOD(I8,I8,100,10,I8,0,S_OK);

    VARMOD(BSTR,I8,strNum0,10,I8,5,S_OK);
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
	  hexpected = S_OK;
	  break;
	case VT_ERROR:
	case VT_VARIANT:
	case VT_UNKNOWN:
	case VT_DECIMAL:
	case VT_RECORD:
	  lValid = FALSE;
	  break;
	default:
	  lFound = FALSE;
	  hexpected = DISP_E_BADVARTYPE;
	  break;
	}

      rFound = TRUE;
      rValid = TRUE;
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
	case VT_CY:
	  hexpected = S_OK;
	  break;
	case VT_ERROR:
	case VT_VARIANT:
	case VT_UNKNOWN:
	case VT_DECIMAL:
	case VT_RECORD:
	  rValid = FALSE;
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
      } else if((l == VT_NULL) && (r == VT_DECIMAL))
      {
	hexpected = DISP_E_OVERFLOW;
      } else if((l == VT_UNKNOWN) || ((r == VT_UNKNOWN) && lFound && lValid))
      {
	hexpected = DISP_E_TYPEMISMATCH;
      } else if((l == VT_NULL) && rFound)
      {
	hexpected = S_OK;
      } else if((l == VT_DECIMAL) || ((r == VT_DECIMAL) && lFound && lValid))
      {
	hexpected = DISP_E_OVERFLOW;
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
      } else if(lFound && !rFound)
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
      else if(l == VT_DATE)
	V_DATE(&v1) = 1000;
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
      else if(r == VT_DATE)
	V_DATE(&v2) = 1000;
      else
	V_I4(&v2) = 10000;

      if ((l != VT_I8 && l != VT_UI8 && r != VT_I8 && r != VT_UI8) || HAVE_OLEAUT32_I8)
      {
        hres = pVarMod(&v1,&v2,&vDst);
        ok(hres == hexpected,
           "VarMod: expected 0x%lx, got 0x%lX for l type of %d, r type of %d,\n", hexpected, hres, l, r);
      }
    }
  }


  /****************************/
  /* test some bad parameters */
  VARMOD(I4,I4,-1,-1,I4,0,S_OK);

  /* test modulus with zero */
  VARMOD2(I4,I4,100,0,EMPTY,0,DISP_E_DIVBYZERO);

  VARMOD(I4,I4,0,10,I4,0,S_OK); /* test 0 mod 10 */

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
  todo_wine {
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
  }

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
  if (HAVE_OLEAUT32_I8)
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
  ok(hres == DISP_E_BADVARTYPE && V_VT(&vDst) == VT_EMPTY && V_I4(&vDst) == 0,
     "VarMod: expected 0x%lx,%d,%d, got 0x%lX,%d,%ld\n", DISP_E_BADVARTYPE, VT_EMPTY, 0, hres, V_VT(&vDst), V_I4(&vDst));
}

static HRESULT (WINAPI *pVarFix)(LPVARIANT,LPVARIANT);

static const char *szVarFixFail = "VarFix: expected 0x0,%d,%d, got 0x%lX,%d,%d\n";
#define VARFIX(vt,val,rvt,rval) V_VT(&v) = VT_##vt; V_##vt(&v) = val; \
        memset(&vDst,0,sizeof(vDst)); hres = pVarFix(&v,&vDst); \
        ok(hres == S_OK && V_VT(&vDst) == VT_##rvt && V_##rvt(&vDst) == (rval), \
        szVarFixFail, VT_##rvt, (int)(rval), \
        hres, V_VT(&vDst), (int)V_##rvt(&vDst))

static void test_VarFix(void)
{
    static const WCHAR szNumMinus1[] = {'-','1','\0' };
    HRESULT hres;
    VARIANT v, vDst;
    DECIMAL *pdec = &V_DECIMAL(&v);
    CY *pcy = &V_CY(&v);
    size_t i;

    CHECKPTR(VarFix);

    /* Test all possible V_VT values */
    for (i = 0; i < sizeof(ExtraFlags)/sizeof(ExtraFlags[0]); i++)
    {
        VARTYPE vt;

        for (vt = 0; vt <= VT_BSTR_BLOB; vt++)
        {
            HRESULT bFail = TRUE;

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
                if (HAVE_OLEAUT32_I8)
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
    if (HAVE_OLEAUT32_I8)
    {
        VARFIX(I8,-1,I8,-1);
    }
    VARFIX(R4,1.4,R4,1);
    VARFIX(R4,1.5,R4,1);
    VARFIX(R4,1.6,R4,1);
    VARFIX(R4,-1.4,R4,-1);
    VARFIX(R4,-1.5,R4,-1);
    VARFIX(R4,-1.6,R4,-1);
    /* DATE & R8 round as for R4 */
    VARFIX(DATE,-1,DATE,-1);
    VARFIX(R8,-1,R8,-1);
    VARFIX(BSTR,(BSTR)szNumMinus1,R8,-1);

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
    S(U(*pdec)).sign = DECIMAL_NEG;
    S(U(*pdec)).scale = 0;
    pdec->Hi32 = 0;
    S1(U1(*pdec)).Mid32 = 0;
    S1(U1(*pdec)).Lo32 = 1;
    hres = pVarFix(&v,&vDst);
    ok(hres == S_OK && V_VT(&vDst) == VT_DECIMAL && !memcmp(&v, &vDst, sizeof(v)),
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

static const char *szVarIntFail = "VarInt: expected 0x0,%d,%d, got 0x%lX,%d,%d\n";
#define VARINT(vt,val,rvt,rval) V_VT(&v) = VT_##vt; V_##vt(&v) = val; \
        memset(&vDst,0,sizeof(vDst)); hres = pVarInt(&v,&vDst); \
        ok(hres == S_OK && V_VT(&vDst) == VT_##rvt && V_##rvt(&vDst) == (rval), \
        szVarIntFail, VT_##rvt, (int)(rval), \
        hres, V_VT(&vDst), (int)V_##rvt(&vDst))

static void test_VarInt(void)
{
    static const WCHAR szNumMinus1[] = {'-','1','\0' };
    HRESULT hres;
    VARIANT v, vDst;
    DECIMAL *pdec = &V_DECIMAL(&v);
    CY *pcy = &V_CY(&v);
    size_t i;

    CHECKPTR(VarInt);

    /* Test all possible V_VT values */
    for (i = 0; i < sizeof(ExtraFlags)/sizeof(ExtraFlags[0]); i++)
    {
        VARTYPE vt;

        for (vt = 0; vt <= VT_BSTR_BLOB; vt++)
        {
            HRESULT bFail = TRUE;

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
                if (HAVE_OLEAUT32_I8)
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
    if (HAVE_OLEAUT32_I8)
    {
        VARINT(I8,-1,I8,-1);
    }
    VARINT(R4,1.4,R4,1);
    VARINT(R4,1.5,R4,1);
    VARINT(R4,1.6,R4,1);
    VARINT(R4,-1.4,R4,-2); /* Note these 3 are different from VarFix */
    VARINT(R4,-1.5,R4,-2);
    VARINT(R4,-1.6,R4,-2);
    /* DATE & R8 round as for R4 */
    VARINT(DATE,-1,DATE,-1);
    VARINT(R8,-1,R8,-1);
    VARINT(BSTR,(BSTR)szNumMinus1,R8,-1);

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
    S(U(*pdec)).sign = DECIMAL_NEG;
    S(U(*pdec)).scale = 0;
    pdec->Hi32 = 0;
    S1(U1(*pdec)).Mid32 = 0;
    S1(U1(*pdec)).Lo32 = 1;
    hres = pVarInt(&v,&vDst);
    ok(hres == S_OK && V_VT(&vDst) == VT_DECIMAL && !memcmp(&v, &vDst, sizeof(v)),
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
       "VarInt: VT_CY wrong, hres=0x%lX %lld\n", hres,V_CY(&vDst).int64);
}

static HRESULT (WINAPI *pVarNeg)(LPVARIANT,LPVARIANT);

static const char *szVarNegFail = "VarNeg: expected 0x0,%d,%d, got 0x%lX,%d,%d\n";
#define VARNEG(vt,val,rvt,rval) V_VT(&v) = VT_##vt; V_##vt(&v) = val; \
        memset(&vDst,0,sizeof(vDst)); hres = pVarNeg(&v,&vDst); \
        ok(hres == S_OK && V_VT(&vDst) == VT_##rvt && V_##rvt(&vDst) == (rval), \
        szVarNegFail, VT_##rvt, (int)(rval), \
        hres, V_VT(&vDst), (int)V_##rvt(&vDst))

static void test_VarNeg(void)
{
    static const WCHAR szNumMinus1[] = {'-','1','\0' };
    static const WCHAR szNum1[] = {'1','\0' };
    HRESULT hres;
    VARIANT v, vDst;
    DECIMAL *pdec = &V_DECIMAL(&v);
    CY *pcy = &V_CY(&v);
    size_t i;

    CHECKPTR(VarNeg);

    /* Test all possible V_VT values. But don't test the exact return values
     * except for success/failure, since M$ made a hash of them in the
     * native version. This at least ensures (as with all tests here) that
     * we will notice if/when new vtypes/flags are added in native.
     */
    for (i = 0; i < sizeof(ExtraFlags)/sizeof(ExtraFlags[0]); i++)
    {
        VARTYPE vt;

        for (vt = 0; vt <= VT_BSTR_BLOB; vt++)
        {
            HRESULT bFail = TRUE;

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
                if (HAVE_OLEAUT32_I8)
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
    if (HAVE_OLEAUT32_I8)
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
    VARNEG(BSTR,(BSTR)szNumMinus1,R8,1);
    VARNEG(BSTR,(BSTR)szNum1,R8,-1);

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
    S(U(*pdec)).sign = DECIMAL_NEG;
    S(U(*pdec)).scale = 0;
    pdec->Hi32 = 0;
    S1(U1(*pdec)).Mid32 = 0;
    S1(U1(*pdec)).Lo32 = 1;
    hres = pVarNeg(&v,&vDst);
    ok(hres == S_OK && V_VT(&vDst) == VT_DECIMAL &&
       S(U(V_DECIMAL(&vDst))).sign == 0,
       "VarNeg: expected 0x0,%d,0x00, got 0x%lX,%d,%02x\n", VT_DECIMAL,
       hres, V_VT(&vDst), S(U(V_DECIMAL(&vDst))).sign);

    S(U(*pdec)).sign = 0;
    hres = pVarNeg(&v,&vDst);
    ok(hres == S_OK && V_VT(&vDst) == VT_DECIMAL &&
       S(U(V_DECIMAL(&vDst))).sign == DECIMAL_NEG,
       "VarNeg: expected 0x0,%d,0x7f, got 0x%lX,%d,%02x\n", VT_DECIMAL,
       hres, V_VT(&vDst), S(U(V_DECIMAL(&vDst))).sign);

    V_VT(&v) = VT_CY;
    pcy->int64 = -10000;
    hres = pVarNeg(&v,&vDst);
    ok(hres == S_OK && V_VT(&vDst) == VT_CY && V_CY(&vDst).int64 == 10000,
       "VarNeg: VT_CY wrong, hres=0x%lX\n", hres);
}

static HRESULT (WINAPI *pVarRound)(LPVARIANT,int,LPVARIANT);

#define VARROUND(vt,val,deci,rvt,rval) V_VT(&v) = VT_##vt; V_##vt(&v) = val; \
        memset(&vDst,0,sizeof(vDst)); hres = pVarRound(&v,deci,&vDst); \
        ok(hres == S_OK && V_VT(&vDst) == VT_##rvt && V_##rvt(&vDst) == (rval), \
        "VarRound: expected 0x0,%d,%d, got 0x%lX,%d,%d\n", VT_##rvt, (int)(rval), \
        hres, V_VT(&vDst), (int)V_##rvt(&vDst))

#define VARROUNDF(vt,val,deci,rvt,rval) V_VT(&v) = VT_##vt; V_##vt(&v) = val; \
        memset(&vDst,0,sizeof(vDst)); hres = pVarRound(&v,deci,&vDst); \
        ok(hres == S_OK && V_VT(&vDst) == VT_##rvt && V_##rvt(&vDst) == (rval), \
        "VarRound: expected 0x0,%d,%f, got 0x%lX,%d,%f\n", VT_##rvt, rval, \
        hres, V_VT(&vDst), V_##rvt(&vDst))

static void test_VarRound(void)
{
    /* static const WCHAR szNumMin[] = {'-','1','.','4','5','\0' };
       static const WCHAR szNum[] = {'1','.','4','5','\0' }; */
    HRESULT hres;
    VARIANT v, vDst;
    CY *pcy = &V_CY(&v);

    CHECKPTR(VarRound);

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

    VARROUNDF(R4,1.0,0,R4,1.0);
    VARROUNDF(R4,-1.0,0,R4,-1.0);
    VARROUNDF(R8,1.0,0,R8,1.0);
    VARROUNDF(R8,-1.0,0,R8,-1.0);

    /* floating point numbers aren't exactly equal and we can't just
     * compare the first few digits.
    todo_wine {
        VARROUNDF(DATE,1.451,1,DATE,1.5);
        VARROUNDF(DATE,-1.45,1,DATE,-1.4);
        VARROUNDF(BSTR,(BSTR)szNumMin,1,R8,-1.40);
        VARROUNDF(BSTR,(BSTR)szNum,1,R8,1.50);

        VARROUNDF(R4,1.23456,0,R4,1.0);
        VARROUNDF(R4,1.23456,1,R4,1.2);
        VARROUNDF(R4,1.23456,2,R4,1.23);
        VARROUNDF(R4,1.23456,3,R4,1.235);
        VARROUNDF(R4,1.23456,4,R4,1.2346);
        VARROUNDF(R4,-1.23456,0,R4,-1.0);
        VARROUNDF(R4,-1.23456,1,R4,-1.2);
        VARROUNDF(R4,-1.23456,2,R4,-1.23);
        VARROUNDF(R4,-1.23456,3,R4,-1.235);
        VARROUNDF(R4,-1.23456,4,R4,-1.2346);

        VARROUNDF(R8,1.23456,0,R8,1.0);
        VARROUNDF(R8,1.23456,1,R8,1.2);
        VARROUNDF(R8,1.23456,2,R8,1.23);
        VARROUNDF(R8,1.23456,3,R8,1.235);
        VARROUNDF(R8,1.23456,4,R8,1.2346);
        VARROUNDF(R8,-1.23456,0,R8,-1.0);
        VARROUNDF(R8,-1.23456,1,R8,-1.2);
        VARROUNDF(R8,-1.23456,2,R8,-1.23);
        VARROUNDF(R8,-1.23456,3,R8,-1.235);
        VARROUNDF(R8,-1.23456,4,R8,-1.2346);
    }
    */

    V_VT(&v) = VT_EMPTY;
    hres = pVarRound(&v,0,&vDst);
    ok(hres == S_OK && V_VT(&vDst) == VT_I2 && V_I2(&vDst) == 0,
        "VarRound: expected 0x0,%d,0 got 0x%lX,%d,%d\n", VT_EMPTY,
        hres, V_VT(&vDst), V_I2(&vDst));

    V_VT(&v) = VT_NULL;
    hres = pVarRound(&v,0,&vDst);
    ok(hres == S_OK && V_VT(&vDst) == VT_NULL,
        "VarRound: expected 0x0,%d got 0x%lX,%d\n", VT_NULL, hres, V_VT(&vDst));

    /* not yet implemented so no use testing yet
    todo_wine {
        DECIMAL *pdec = &V_DECIMAL(&v);
        V_VT(&v) = VT_DECIMAL;
        S(U(*pdec)).sign = DECIMAL_NEG;
        S(U(*pdec)).scale = 0;
        pdec->Hi32 = 0;
        S1(U1(*pdec)).Mid32 = 0;
        S1(U1(*pdec)).Lo32 = 1;
        hres = pVarRound(&v,0,&vDst);
        ok(hres == S_OK && V_VT(&vDst) == VT_DECIMAL &&
            S(U(V_DECIMAL(&vDst))).sign == 0,
            "VarRound: expected 0x0,%d,0x00, got 0x%lX,%d,%02x\n", VT_DECIMAL,
            hres, V_VT(&vDst), S(U(V_DECIMAL(&vDst))).sign);

        S(U(*pdec)).sign = 0;
        hres = pVarRound(&v,0,&vDst);
        ok(hres == S_OK && V_VT(&vDst) == VT_DECIMAL &&
            S(U(V_DECIMAL(&vDst))).sign == DECIMAL_NEG,
            "VarRound: expected 0x0,%d,0x7f, got 0x%lX,%d,%02x\n", VT_DECIMAL,
            hres, V_VT(&vDst), S(U(V_DECIMAL(&vDst))).sign);
    }
    */

    V_VT(&v) = VT_CY;
    pcy->int64 = 10000;
    hres = pVarRound(&v,0,&vDst);
    ok(hres == S_OK && V_VT(&vDst) == VT_CY && V_CY(&vDst).int64 == 10000,
        "VarRound: VT_CY wrong, hres=0x%lX\n", hres);

}

static HRESULT (WINAPI *pVarXor)(LPVARIANT,LPVARIANT,LPVARIANT);

static const char *szVarXorFail = "VarXor(%d,%d): expected 0x0,%d,%d, got 0x%lX,%d,%d\n";
#define VARXOR(vt1,val1,vt2,val2,rvt,rval) \
        V_VT(&left) = VT_##vt1; V_##vt1(&left) = val1; \
        V_VT(&right) = VT_##vt2; V_##vt2(&right) = val2; \
        memset(&result,0,sizeof(result)); hres = pVarXor(&left,&right,&result); \
        ok(hres == S_OK && V_VT(&result) == VT_##rvt && V_##rvt(&result) == (rval), \
        szVarXorFail, VT_##vt1, VT_##vt2, \
        VT_##rvt, (int)(rval), hres, V_VT(&result), (int)V_##rvt(&result)); \
        ok(V_VT(&left) == VT_##vt1 && V_##vt1(&left) == val1 && \
           V_VT(&right) == VT_##vt2 && V_##vt2(&right) == val2, \
           "VarXor(%d,%d): Modified input arguments\n",VT_##vt1,VT_##vt2)
#define VARXORCY(vt1,val1,val2,rvt,rval) \
        V_VT(&left) = VT_##vt1; V_##vt1(&left) = val1; \
        V_VT(&right) = VT_CY; V_CY(&right).int64 = val2; \
        memset(&result,0,sizeof(result)); hres = pVarXor(&left,&right,&result); \
        ok(hres == S_OK && V_VT(&result) == VT_##rvt && V_##rvt(&result) == (rval), \
        "VarXor(%d,%d): expected 0x0,%d,%d, got 0x%lX,%d,%d\n", VT_##vt1, VT_CY, \
        VT_##rvt, (int)(rval), hres, V_VT(&result), (int)V_##rvt(&result)); \
        ok(V_VT(&left) == VT_##vt1 && V_##vt1(&left) == val1 && \
           V_VT(&right) == VT_CY && V_CY(&right).int64 == val2, \
           "VarXor(%d,%d): Modified input arguments\n",VT_##vt1,VT_CY)

static void test_VarXor(void)
{
    static const WCHAR szFalse[] = { '#','F','A','L','S','E','#','\0' };
    static const WCHAR szTrue[] = { '#','T','R','U','E','#','\0' };
    VARIANT left, right, result;
    BSTR lbstr, rbstr;
    VARTYPE i;
    HRESULT hres;

    CHECKPTR(VarXor);

    /* Test all possible flag/vt combinations & the resulting vt type */
    for (i = 0; i < sizeof(ExtraFlags)/sizeof(ExtraFlags[0]); i++)
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
    if (HAVE_OLEAUT32_I8)
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
    rbstr = SysAllocString(szTrue);
    VARXOR(EMPTY,0,BSTR,rbstr,I2,-1);
    VARXORCY(EMPTY,0,10000,I4,1);

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
    if (HAVE_OLEAUT32_I8)
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
    rbstr = SysAllocString(szTrue);
    VARXOR(NULL,0,BSTR,rbstr,NULL,0);
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
    if (HAVE_OLEAUT32_I8)
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
    rbstr = SysAllocString(szTrue);
    VARXOR(BOOL,VARIANT_FALSE,BSTR,rbstr,BOOL,VARIANT_TRUE);
    VARXOR(BOOL,VARIANT_TRUE,BSTR,rbstr,BOOL,VARIANT_FALSE);
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
    if (HAVE_OLEAUT32_I8)
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
    rbstr = SysAllocString(szTrue);
    VARXOR(I1,0,BSTR,rbstr,I4,-1);
    VARXOR(I1,-1,BSTR,rbstr,I4,0);
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
    if (HAVE_OLEAUT32_I8)
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
    rbstr = SysAllocString(szTrue);
    VARXOR(UI1,0,BSTR,rbstr,I2,-1);
    VARXOR(UI1,255,BSTR,rbstr,I2,-256);
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
    if (HAVE_OLEAUT32_I8)
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
    rbstr = SysAllocString(szTrue);
    VARXOR(I2,0,BSTR,rbstr,I2,-1);
    VARXOR(I2,-1,BSTR,rbstr,I2,0);
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
    if (HAVE_OLEAUT32_I8)
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
    rbstr = SysAllocString(szTrue);
    VARXOR(UI2,0,BSTR,rbstr,I4,-1);
    VARXOR(UI2,65535,BSTR,rbstr,I4,-65536);
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
    if (HAVE_OLEAUT32_I8)
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
    rbstr = SysAllocString(szTrue);
    VARXOR(I4,0,BSTR,rbstr,I4,-1);
    VARXOR(I4,-1,BSTR,rbstr,I4,0);
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
    if (HAVE_OLEAUT32_I8)
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
    rbstr = SysAllocString(szTrue);
    VARXOR(UI4,0,BSTR,rbstr,I4,-1);
    VARXOR(UI4,0xffffffff,BSTR,rbstr,I4,0);
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
    if (HAVE_OLEAUT32_I8)
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
    rbstr = SysAllocString(szTrue);
    VARXOR(R4,0,BSTR,rbstr,I4,-1);
    VARXOR(R4,-1,BSTR,rbstr,I4,0);
    VARXORCY(R4,-1,10000,I4,-2);
    VARXORCY(R4,-1,0,I4,-1);
    VARXORCY(R4,0,0,I4,0);

    VARXOR(R8,-1,R8,-1,I4,0);
    VARXOR(R8,-1,R8,0,I4,-1);
    VARXOR(R8,0,R8,0,I4,0);
    VARXOR(R8,-1,DATE,-1,I4,0);
    VARXOR(R8,-1,DATE,0,I4,-1);
    VARXOR(R8,0,DATE,0,I4,0);
    if (HAVE_OLEAUT32_I8)
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
    rbstr = SysAllocString(szTrue);
    VARXOR(R8,0,BSTR,rbstr,I4,-1);
    VARXOR(R8,-1,BSTR,rbstr,I4,0);
    VARXORCY(R8,-1,10000,I4,-2);
    VARXORCY(R8,-1,0,I4,-1);
    VARXORCY(R8,0,0,I4,0);

    VARXOR(DATE,-1,DATE,-1,I4,0);
    VARXOR(DATE,-1,DATE,0,I4,-1);
    VARXOR(DATE,0,DATE,0,I4,0);
    if (HAVE_OLEAUT32_I8)
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
    rbstr = SysAllocString(szTrue);
    VARXOR(DATE,0,BSTR,rbstr,I4,-1);
    VARXOR(DATE,-1,BSTR,rbstr,I4,0);
    VARXORCY(DATE,-1,10000,I4,-2);
    VARXORCY(DATE,-1,0,I4,-1);
    VARXORCY(DATE,0,0,I4,0);

    if (HAVE_OLEAUT32_I8)
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
        rbstr = SysAllocString(szTrue);
        VARXOR(I8,0,BSTR,rbstr,I8,-1);
        VARXOR(I8,-1,BSTR,rbstr,I8,0);
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
        rbstr = SysAllocString(szTrue);
        VARXOR(UI8,0,BSTR,rbstr,I4,-1);
        VARXOR(UI8,0xffff,BSTR,rbstr,I4,-65536);
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
    rbstr = SysAllocString(szTrue);
    VARXOR(INT,0,BSTR,rbstr,I4,-1);
    VARXOR(INT,-1,BSTR,rbstr,I4,0);
    VARXORCY(INT,-1,10000,I4,-2);
    VARXORCY(INT,-1,0,I4,-1);
    VARXORCY(INT,0,0,I4,0);

    VARXOR(UINT,0xffff,UINT,0xffff,I4,0);
    VARXOR(UINT,0xffff,UINT,0,I4,0xffff);
    VARXOR(UINT,0,UINT,0,I4,0);
    rbstr = SysAllocString(szFalse);
    VARXOR(UINT,0,BSTR,rbstr,I4,0);
    VARXOR(UINT,0xffff,BSTR,rbstr,I4,0xffff);
    rbstr = SysAllocString(szTrue);
    VARXOR(UINT,0,BSTR,rbstr,I4,-1);
    VARXOR(UINT,0xffff,BSTR,rbstr,I4,-65536);
    VARXORCY(UINT,0xffff,10000,I4,65534);
    VARXORCY(UINT,0xffff,0,I4,0xffff);
    VARXORCY(UINT,0,0,I4,0);

    lbstr = SysAllocString(szFalse);
    rbstr = SysAllocString(szFalse);
    VARXOR(BSTR,lbstr,BSTR,rbstr,BOOL,0);
    rbstr = SysAllocString(szTrue);
    VARXOR(BSTR,lbstr,BSTR,rbstr,BOOL,VARIANT_TRUE);
    lbstr = SysAllocString(szTrue);
    VARXOR(BSTR,lbstr,BSTR,rbstr,BOOL,VARIANT_FALSE);
    VARXORCY(BSTR,lbstr,10000,I4,-2);
    lbstr = SysAllocString(szFalse);
    VARXORCY(BSTR,lbstr,10000,I4,1);
}

static HRESULT (WINAPI *pVarOr)(LPVARIANT,LPVARIANT,LPVARIANT);

static const char *szVarOrFail = "VarOr(%d,%d): expected 0x0,%d,%d, got 0x%lX,%d,%d\n";
static const char *szVarOrChanged = "VarOr(%d,%d): Modified input arguments\n";
#define VAROR(vt1,val1,vt2,val2,rvt,rval) \
        V_VT(&left) = VT_##vt1; V_##vt1(&left) = val1; \
        V_VT(&right) = VT_##vt2; V_##vt2(&right) = val2; \
        memset(&result,0,sizeof(result)); hres = pVarOr(&left,&right,&result); \
        ok(hres == S_OK && V_VT(&result) == VT_##rvt && V_##rvt(&result) == (rval), \
        szVarOrFail, VT_##vt1, VT_##vt2, \
        VT_##rvt, (int)(rval), hres, V_VT(&result), (int)V_##rvt(&result)); \
        ok(V_VT(&left) == VT_##vt1 && V_##vt1(&left) == val1 && \
           V_VT(&right) == VT_##vt2 && V_##vt2(&right) == val2, \
           szVarOrChanged,VT_##vt1,VT_##vt2)
#define VARORCY(vt1,val1,val2,rvt,rval) \
        V_VT(&left) = VT_##vt1; V_##vt1(&left) = val1; \
        V_VT(&right) = VT_CY; V_CY(&right).int64 = val2; \
        memset(&result,0,sizeof(result)); hres = pVarOr(&left,&right,&result); \
        ok(hres == S_OK && V_VT(&result) == VT_##rvt && V_##rvt(&result) == (rval), \
        "VarOr(%d,%d): expected 0x0,%d,%d, got 0x%lX,%d,%d\n", VT_##vt1, VT_CY, \
        VT_##rvt, (int)(rval), hres, V_VT(&result), (int)V_##rvt(&result)); \
        ok(V_VT(&left) == VT_##vt1 && V_##vt1(&left) == val1 && \
           V_VT(&right) == VT_CY && V_CY(&right).int64 == val2, \
           "VarOr(%d,%d): Modified input arguments\n",VT_##vt1,VT_CY)

static void test_VarOr(void)
{
    static const WCHAR szFalse[] = { '#','F','A','L','S','E','#','\0' };
    static const WCHAR szTrue[] = { '#','T','R','U','E','#','\0' };
    VARIANT left, right, result;
    BSTR lbstr, rbstr;
    VARTYPE i;
    HRESULT hres;

    CHECKPTR(VarOr);

    /* Test all possible flag/vt combinations & the resulting vt type */
    for (i = 0; i < sizeof(ExtraFlags)/sizeof(ExtraFlags[0]); i++)
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
    if (HAVE_OLEAUT32_I8)
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
    rbstr = SysAllocString(szTrue);
    VAROR(EMPTY,0,BSTR,rbstr,I2,-1);
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
    if (HAVE_OLEAUT32_I8)
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
    rbstr = SysAllocString(szTrue);
    VAROR(NULL,0,BSTR,rbstr,BOOL,VARIANT_TRUE);
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
    if (HAVE_OLEAUT32_I8)
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
    rbstr = SysAllocString(szTrue);
    VAROR(BOOL,VARIANT_FALSE,BSTR,rbstr,BOOL,VARIANT_TRUE);
    VAROR(BOOL,VARIANT_TRUE,BSTR,rbstr,BOOL,VARIANT_TRUE);
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
    if (HAVE_OLEAUT32_I8)
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
    rbstr = SysAllocString(szTrue);
    VAROR(I1,0,BSTR,rbstr,I4,-1);
    VAROR(I1,-1,BSTR,rbstr,I4,-1);
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
    if (HAVE_OLEAUT32_I8)
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
    rbstr = SysAllocString(szTrue);
    VAROR(UI1,0,BSTR,rbstr,I2,-1);
    VAROR(UI1,255,BSTR,rbstr,I2,-1);
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
    if (HAVE_OLEAUT32_I8)
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
    rbstr = SysAllocString(szTrue);
    VAROR(I2,0,BSTR,rbstr,I2,-1);
    VAROR(I2,-1,BSTR,rbstr,I2,-1);
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
    if (HAVE_OLEAUT32_I8)
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
    rbstr = SysAllocString(szTrue);
    VAROR(UI2,0,BSTR,rbstr,I4,-1);
    VAROR(UI2,65535,BSTR,rbstr,I4,-1);
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
    if (HAVE_OLEAUT32_I8)
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
    rbstr = SysAllocString(szTrue);
    VAROR(I4,0,BSTR,rbstr,I4,-1);
    VAROR(I4,-1,BSTR,rbstr,I4,-1);
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
    if (HAVE_OLEAUT32_I8)
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
    rbstr = SysAllocString(szTrue);
    VAROR(UI4,0,BSTR,rbstr,I4,-1);
    VAROR(UI4,0xffffffff,BSTR,rbstr,I4,-1);
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
    if (HAVE_OLEAUT32_I8)
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
    rbstr = SysAllocString(szTrue);
    VAROR(R4,0,BSTR,rbstr,I4,-1);
    VAROR(R4,-1,BSTR,rbstr,I4,-1);
    VARORCY(R4,-1,10000,I4,-1);
    VARORCY(R4,-1,0,I4,-1);
    VARORCY(R4,0,0,I4,0);

    VAROR(R8,-1,R8,-1,I4,-1);
    VAROR(R8,-1,R8,0,I4,-1);
    VAROR(R8,0,R8,0,I4,0);
    VAROR(R8,-1,DATE,-1,I4,-1);
    VAROR(R8,-1,DATE,0,I4,-1);
    VAROR(R8,0,DATE,0,I4,0);
    if (HAVE_OLEAUT32_I8)
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
    rbstr = SysAllocString(szTrue);
    VAROR(R8,0,BSTR,rbstr,I4,-1);
    VAROR(R8,-1,BSTR,rbstr,I4,-1);
    VARORCY(R8,-1,10000,I4,-1);
    VARORCY(R8,-1,0,I4,-1);
    VARORCY(R8,0,0,I4,0);

    VAROR(DATE,-1,DATE,-1,I4,-1);
    VAROR(DATE,-1,DATE,0,I4,-1);
    VAROR(DATE,0,DATE,0,I4,0);
    if (HAVE_OLEAUT32_I8)
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
    rbstr = SysAllocString(szTrue);
    VAROR(DATE,0,BSTR,rbstr,I4,-1);
    VAROR(DATE,-1,BSTR,rbstr,I4,-1);
    VARORCY(DATE,-1,10000,I4,-1);
    VARORCY(DATE,-1,0,I4,-1);
    VARORCY(DATE,0,0,I4,0);

    if (HAVE_OLEAUT32_I8)
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
        rbstr = SysAllocString(szTrue);
        VAROR(I8,0,BSTR,rbstr,I8,-1);
        VAROR(I8,-1,BSTR,rbstr,I8,-1);
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
        rbstr = SysAllocString(szTrue);
        VAROR(UI8,0,BSTR,rbstr,I4,-1);
        VAROR(UI8,0xffff,BSTR,rbstr,I4,-1);
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
    rbstr = SysAllocString(szTrue);
    VAROR(INT,0,BSTR,rbstr,I4,-1);
    VAROR(INT,-1,BSTR,rbstr,I4,-1);
    VARORCY(INT,-1,10000,I4,-1);
    VARORCY(INT,-1,0,I4,-1);
    VARORCY(INT,0,0,I4,0);

    VAROR(UINT,0xffff,UINT,0xffff,I4,0xffff);
    VAROR(UINT,0xffff,UINT,0,I4,0xffff);
    VAROR(UINT,0,UINT,0,I4,0);
    rbstr = SysAllocString(szFalse);
    VAROR(UINT,0,BSTR,rbstr,I4,0);
    VAROR(UINT,0xffff,BSTR,rbstr,I4,0xffff);
    rbstr = SysAllocString(szTrue);
    VAROR(UINT,0,BSTR,rbstr,I4,-1);
    VAROR(UINT,0xffff,BSTR,rbstr,I4,-1);
    VARORCY(UINT,0xffff,10000,I4,0xffff);
    VARORCY(UINT,0xffff,0,I4,0xffff);
    VARORCY(UINT,0,0,I4,0);

    lbstr = SysAllocString(szFalse);
    rbstr = SysAllocString(szFalse);
    VAROR(BSTR,lbstr,BSTR,rbstr,BOOL,0);
    rbstr = SysAllocString(szTrue);
    VAROR(BSTR,lbstr,BSTR,rbstr,BOOL,VARIANT_TRUE);
    lbstr = SysAllocString(szTrue);
    VAROR(BSTR,lbstr,BSTR,rbstr,BOOL,VARIANT_TRUE);
    VARORCY(BSTR,lbstr,10000,I4,-1);
    lbstr = SysAllocString(szFalse);
    VARORCY(BSTR,lbstr,10000,I4,1);
}

static HRESULT (WINAPI *pVarEqv)(LPVARIANT,LPVARIANT,LPVARIANT);

static const char *szVarEqvFail = "VarEqv(%d,%d): expected 0x0,%d,%d, got 0x%lX,%d,%d\n";
#define VAREQV(vt1,val1,vt2,val2,rvt,rval) \
        V_VT(&left) = VT_##vt1; V_##vt1(&left) = val1; \
        V_VT(&right) = VT_##vt2; V_##vt2(&right) = val2; \
        memset(&result,0,sizeof(result)); hres = pVarEqv(&left,&right,&result); \
        ok(hres == S_OK && V_VT(&result) == VT_##rvt && V_##rvt(&result) == (rval), \
        szVarEqvFail, VT_##vt1, VT_##vt2, \
        VT_##rvt, (int)(rval), hres, V_VT(&result), (int)V_##rvt(&result))

static void test_VarEqv(void)
{
    VARIANT left, right, result;
    VARTYPE i;
    HRESULT hres;

    CHECKPTR(VarEqv);

    /* Test all possible flag/vt combinations & the resulting vt type */
    for (i = 0; i < sizeof(ExtraFlags)/sizeof(ExtraFlags[0]); i++)
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
    if (HAVE_OLEAUT32_I8)
    {
        VAREQV(UI4,VARIANT_FALSE,I8,VARIANT_FALSE,I8,-1);
        VAREQV(UI4,5,I8,19,I8,-23);
        VAREQV(UI4,VARIANT_FALSE,UI8,VARIANT_FALSE,I4,-1);
    }
}

static HRESULT (WINAPI *pVarMul)(LPVARIANT,LPVARIANT,LPVARIANT);

static const char *szVarMulI4 = "VarMul(%d,%d): expected 0x0,%d,%d, got 0x%lX,%d,%d\n";
static const char *szVarMulR8 = "VarMul(%d,%d): expected 0x0,%d,%f, got 0x%lX,%d,%f\n";

#define VARMUL(vt1,val1,vt2,val2,rvt,rval) \
        V_VT(&left) = VT_##vt1; V_##vt1(&left) = val1; \
        V_VT(&right) = VT_##vt2; V_##vt2(&right) = val2; \
        memset(&result,0,sizeof(result)); hres = pVarMul(&left,&right,&result); \
        if (VT_##rvt == VT_R4 || VT_##rvt == VT_R8) { \
        ok(hres == S_OK && V_VT(&result) == VT_##rvt && \
        EQ_FLOAT(V_##rvt(&result), rval), \
        szVarMulR8, VT_##vt1, VT_##vt2, \
        VT_##rvt, (double)(rval), hres, V_VT(&result), (double)V_##rvt(&result)); \
        } else { \
        ok(hres == S_OK && V_VT(&result) == VT_##rvt && V_##rvt(&result) == (rval), \
        szVarMulI4, VT_##vt1, VT_##vt2, \
        VT_##rvt, (int)(rval), hres, V_VT(&result), (int)V_##rvt(&result)); }

static void test_VarMul(void)
{
    static const WCHAR sz12[] = {'1','2','\0'};
    VARIANT left, right, result, cy, dec;
    VARTYPE i;
    BSTR lbstr, rbstr;
    HRESULT hres;
    double r;

    CHECKPTR(VarMul);

    lbstr = SysAllocString(sz12);
    rbstr = SysAllocString(sz12);

    /* Test all possible flag/vt combinations & the resulting vt type */
    for (i = 0; i < sizeof(ExtraFlags)/sizeof(ExtraFlags[0]); i++)
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
    VARMUL(I2,7,R4,0.5,R4,3.5);
    VARMUL(R4,0.5,I4,5,R8,2.5);
    VARMUL(R8,7.1,BOOL,0,R8,0);
    VARMUL(BSTR,lbstr,I2,4,R8,48);
    VARMUL(BSTR,lbstr,BOOL,1,R8,12);
    VARMUL(BSTR,lbstr,R4,0.1,R8,1.2);
    VARMUL(BSTR,lbstr,BSTR,rbstr,R8,144);
    VARMUL(R4,0.2,BSTR,rbstr,R8,2.4);
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

    hres = VarMul(&cy, &right, &result);
    ok(hres == S_OK && V_VT(&result) == VT_CY, "VarMul: expected coerced type VT_CY, got %s!\n", vtstr(V_VT(&result)));
    hres = VarR8FromCy(V_CY(&result), &r);
    ok(hres == S_OK && EQ_DOUBLE(r, 42399), "VarMul: CY value %f, expected %f\n", r, (double)42399);

    hres = VarMul(&left, &dec, &result);
    ok(hres == S_OK && V_VT(&result) == VT_DECIMAL, "VarMul: expected coerced type VT_DECIMAL, got %s!\n", vtstr(V_VT(&result)));
    hres = VarR8FromDec(&V_DECIMAL(&result), &r);
    ok(hres == S_OK && EQ_DOUBLE(r, 46.2), "VarMul: DECIMAL value %f, expected %f\n", r, (double)46.2);

    SysFreeString(lbstr);
    SysFreeString(rbstr);
}

static HRESULT (WINAPI *pVarAdd)(LPVARIANT,LPVARIANT,LPVARIANT);

static const char *szVarAddI4 = "VarAdd(%d,%d): expected 0x0,%d,%d, got 0x%lX,%d,%d\n";
static const char *szVarAddR8 = "VarAdd(%d,%d): expected 0x0,%d,%f, got 0x%lX,%d,%f\n";

#define VARADD(vt1,val1,vt2,val2,rvt,rval) \
        V_VT(&left) = VT_##vt1; V_##vt1(&left) = val1; \
        V_VT(&right) = VT_##vt2; V_##vt2(&right) = val2; \
        memset(&result,0,sizeof(result)); hres = pVarAdd(&left,&right,&result); \
        if (VT_##rvt == VT_R4 || VT_##rvt == VT_R8 || VT_##rvt == VT_DATE) { \
        ok(hres == S_OK && V_VT(&result) == VT_##rvt && \
        EQ_FLOAT(V_##rvt(&result), rval), \
        szVarAddR8, VT_##vt1, VT_##vt2, \
        VT_##rvt, (double)(rval), hres, V_VT(&result), (double)V_##rvt(&result)); \
        } else { \
        ok(hres == S_OK && V_VT(&result) == VT_##rvt && V_##rvt(&result) == (rval), \
        szVarAddI4, VT_##vt1, VT_##vt2, \
        VT_##rvt, (int)(rval), hres, V_VT(&result), (int)V_##rvt(&result)); }

static void test_VarAdd(void)
{
    static const WCHAR sz12[] = {'1','2','\0'};
    VARIANT left, right, result, cy, dec;
    VARTYPE i;
    BSTR lbstr, rbstr;
    HRESULT hres;
    double r;

    CHECKPTR(VarAdd);

    lbstr = SysAllocString(sz12);
    rbstr = SysAllocString(sz12);

    /* Test all possible flag/vt combinations & the resulting vt type */
    for (i = 0; i < sizeof(ExtraFlags)/sizeof(ExtraFlags[0]); i++)
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
            }
        }
    }

    /* Test returned values */
    VARADD(I4,4,I4,2,I4,6);
    VARADD(I2,4,I2,2,I2,6);
    VARADD(I2,-13,I4,5,I4,-8);
    VARADD(I4,-13,I4,5,I4,-8);
    VARADD(I2,7,R4,0.5,R4,7.5);
    VARADD(R4,0.5,I4,5,R8,5.5);
    VARADD(R8,7.1,BOOL,0,R8,7.1);
    VARADD(BSTR,lbstr,I2,4,R8,16);
    VARADD(BSTR,lbstr,BOOL,1,R8,13);
    VARADD(BSTR,lbstr,R4,0.1,R8,12.1);
    VARADD(R4,0.2,BSTR,rbstr,R8,12.2);
    VARADD(DATE,2.25,I4,7,DATE,9.25);
    VARADD(DATE,1.25,R4,-1.7,DATE,-0.45);

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

    /* Manualy test BSTR + BSTR */
    V_VT(&left) = VT_BSTR;
    V_BSTR(&left) = lbstr;
    V_VT(&right) = VT_BSTR;
    V_BSTR(&right) = rbstr;
    hres = VarAdd(&left, &right, &result);
    ok(hres == S_OK && V_VT(&result) == VT_BSTR, "VarAdd: expected coerced type VT_BSTR, got %s!\n", vtstr(V_VT(&result)));
    hres = VarR8FromStr(V_BSTR(&result), 0, 0, &r);
    ok(hres == S_OK && EQ_DOUBLE(r, 1212), "VarAdd: BSTR value %f, expected %f\n", r, (double)1212);

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

    hres = VarAdd(&cy, &right, &result);
    ok(hres == S_OK && V_VT(&result) == VT_CY, "VarAdd: expected coerced type VT_CY, got %s!\n", vtstr(V_VT(&result)));
    hres = VarR8FromCy(V_CY(&result), &r);
    ok(hres == S_OK && EQ_DOUBLE(r, 4720), "VarAdd: CY value %f, expected %f\n", r, (double)4720);

    hres = VarAdd(&left, &dec, &result);
    ok(hres == S_OK && V_VT(&result) == VT_DECIMAL, "VarAdd: expected coerced type VT_DECIMAL, got %s!\n", vtstr(V_VT(&result)));
    hres = VarR8FromDec(&V_DECIMAL(&result), &r);
    ok(hres == S_OK && EQ_DOUBLE(r, -15.2), "VarAdd: DECIMAL value %f, expected %f\n", r, (double)-15.2);

    SysFreeString(lbstr);
    SysFreeString(rbstr);
}

static HRESULT (WINAPI *pVarCmp)(LPVARIANT,LPVARIANT,LCID,ULONG);

/* ERROR from wingdi.h is interfering here */
#undef ERROR
#define _VARCMP(vt1,val1,vtfl1,vt2,val2,vtfl2,lcid,flags,result) \
        V_##vt1(&left) = val1; V_VT(&left) = VT_##vt1 | vtfl1; \
        V_##vt2(&right) = val2; V_VT(&right) = VT_##vt2 | vtfl2; \
        hres = pVarCmp(&left,&right,lcid,flags); \
        ok(hres == result, "VarCmp(VT_" #vt1 "|" #vtfl1 ",VT_" #vt2 "|" #vtfl2 "): expected " #result ", got hres=0x%lx\n", hres)
#define VARCMPEX(vt1,val1,vt2,val2,res1,res2,res3,res4) \
        _VARCMP(vt1,val1,0,vt2,val2,0,lcid,0,res1); \
        _VARCMP(vt1,val1,VT_RESERVED,vt2,val2,0,lcid,0,res2); \
        _VARCMP(vt1,val1,0,vt2,val2,VT_RESERVED,lcid,0,res3); \
        _VARCMP(vt1,val1,VT_RESERVED,vt2,val2,VT_RESERVED,lcid,0,res4)
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
    static const WCHAR szhuh[] = {'h','u','h','?','\0'};
    static const WCHAR sz2cents[] = {'2','c','e','n','t','s','\0'};
    static const WCHAR szempty[] = {'\0'};
    static const WCHAR sz0[] = {'0','\0'};
    static const WCHAR sz1[] = {'1','\0'};
    static const WCHAR sz7[] = {'7','\0'};
    static const WCHAR sz42[] = {'4','2','\0'};
    static const WCHAR sz1neg[] = {'-','1','\0'};
    static const WCHAR sz666neg[] = {'-','6','6','6','\0'};
    static const WCHAR sz1few[] = {'1','.','0','0','0','0','0','0','0','1','\0'};
    BSTR bstrhuh, bstrempty, bstr0, bstr1, bstr7, bstr42, bstr1neg, bstr666neg;
    BSTR bstr2cents, bstr1few;

    CHECKPTR(VarCmp);

    lcid = MAKELCID(MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),SORT_DEFAULT);
    bstrempty = SysAllocString(szempty);
    bstrhuh = SysAllocString(szhuh);
    bstr2cents = SysAllocString(sz2cents);
    bstr0 = SysAllocString(sz0);
    bstr1 = SysAllocString(sz1);
    bstr7 = SysAllocString(sz7);
    bstr42 = SysAllocString(sz42);
    bstr1neg = SysAllocString(sz1neg);
    bstr666neg = SysAllocString(sz666neg);
    bstr1few = SysAllocString(sz1few);

    /* Test all possible flag/vt combinations & the resulting vt type */
    for (i = 0; i < sizeof(ExtraFlags)/sizeof(ExtraFlags[0]); i++)
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

                hres = pVarCmp(&left, &right, LOCALE_USER_DEFAULT, 0);
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

    /* VARCMP{,EX} run each 4 tests with a permutation of all posible
       input variants with (1) and without (0) VT_RESERVED set. The order
       of the permutations is (0,0); (1,0); (0,1); (1,1) */
    VARCMP(INT,4711,I2,4711,VARCMP_EQ);
    ok(V_VT(&left) & V_VT(&right) & VT_RESERVED, "VT_RESERVED filtered out!\n");
    VARCMP(INT,4711,I2,-4711,VARCMP_GT);
    VARCMP(ERROR,0,ERROR,0,VARCMP_EQ);
    VARCMP(ERROR,0,UI1,0,DISP_E_TYPEMISMATCH);
    VARCMP(EMPTY,0,EMPTY,0,VARCMP_EQ);
    VARCMP(I4,1,R8,1.0,VARCMP_EQ);
    VARCMP(EMPTY,19,I2,0,VARCMP_EQ);
    ok(V_EMPTY(&left) == 19, "VT_EMPTY modified!\n");
    ok(V_VT(&left) & V_VT(&right) & VT_RESERVED, "VT_RESERVED filtered out!\n");
    VARCMP(I4,1,UI1,1,VARCMP_EQ);
    VARCMP(I2,2,I2,2,VARCMP_EQ);
    VARCMP(I2,1,I2,2,VARCMP_LT);
    VARCMP(I2,2,I2,1,VARCMP_GT);
    VARCMP(I2,2,EMPTY,1,VARCMP_GT);
    VARCMP(I2,2,NULL_,1,VARCMP_NULL);

    /* BSTR handling, especialy in conjunction with VT_RESERVED */
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
    ok(V_VT(&left) & V_VT(&right) & VT_RESERVED, "VT_RESERVED filtered out!\n");
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
    SETDEC(dec,0,0,0,0);
    VARCMPEX(DECIMAL,dec,BSTR,bstr0,VARCMP_LT,VARCMP_EQ,VARCMP_EQ,VARCMP_LT);
    SETDEC64(dec,0,0,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF); /* max DECIMAL */
    VARCMP(DECIMAL,dec,R8,R8_MAX,VARCMP_LT);    /* R8 has bigger range */
    VARCMP(DECIMAL,dec,DATE,R8_MAX,VARCMP_LT);  /* DATE has bigger range */
    SETDEC64(dec,0,0x80,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF);
    VARCMP(DECIMAL,dec,R8,-R8_MAX,VARCMP_GT);
    SETDEC64(dec,20,0,0x5,0x6BC75E2D,0x63100001);     /* 1+1e-20 */
    VARCMP(DECIMAL,dec,R8,1,VARCMP_GT); /* DECIMAL has higher precission */

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

    /* R4 precission handling */
    VARCMP(R4,1,R8,1+1e-8,VARCMP_EQ);
    VARCMP(R8,1+1e-8,R4,1,VARCMP_EQ);
    VARCMP(R8,1+1e-8,R8,1,VARCMP_GT);
    VARCMP(R8,R4_MAX*1.1,R4,R4_MAX,VARCMP_GT);
    VARCMP(R4,R4_MAX,R8,R8_MAX,VARCMP_LT);
    VARCMP(R4,1,DATE,1+1e-8,VARCMP_EQ);
    VARCMP(R4,1,BSTR,bstr1few,VARCMP_LT); /* bstr1few == 1+1e-8 */
    SETDEC(dec,8,0,0,0x5F5E101);          /* 1+1e-8 */
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

START_TEST(vartest)
{
  hOleaut32 = LoadLibraryA("oleaut32.dll");

  test_VariantInit();
  test_VariantClear();
  test_VariantCopy();
  test_VariantCopyInd();
  test_VarParseNumFromStr();
  test_VarNumFromParseNum();
  test_VarUdateFromDate();
  test_VarDateFromUdate();
  test_SystemTimeToVariantTime();
  test_VariantTimeToSystemTime();
  test_DosDateTimeToVariantTime();
  test_VariantTimeToDosDateTime();
  test_VarFormatNumber();
  test_VarFormat();
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
  test_VarEqv();
  test_VarMul();
  test_VarAdd();
  test_VarCmp();
}
