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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <time.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
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
/* Is vt a type unavailable to ancient versions? */
#define IS_MODERN_VTYPE(vt) (vt==VT_VARIANT||vt==VT_DECIMAL|| \
    vt==VT_I1||vt==VT_UI2||vt==VT_UI4||vt == VT_INT||vt == VT_UINT)

/* When comparing floating point values we cannot expect an exact match
 * because the rounding errors depend on the exact algorithm.
 */
#define EQ_DOUBLE(a,b)     (fabs((a)-(b))<1e-14)

static inline int strcmpW( const WCHAR *str1, const WCHAR *str2 )
{
    while (*str1 && (*str1 == *str2)) { str1++; str2++; }
    return *str1 - *str2;
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

static void test_VariantClear(void)
{
  HRESULT hres;
  VARIANTARG v;
  size_t i;

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

      memset(&v, 0, sizeof(v));
      V_VT(&v) = vt | ExtraFlags[i];

      hres = VariantClear(&v);

      if (IsValidVariantClearVT(vt, ExtraFlags[i]))
        hExpected = S_OK;

      ok(hres == hExpected, "VariantClear: expected 0x%lX, got 0x%lX for vt %d | 0x%X\n",
         hExpected, hres, vt, ExtraFlags[i]);
    }
  }
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
#define CONVERTN(str,dig,flags) MultiByteToWideChar(CP_ACP,0,str,-1,buff,sizeof(buff)); \
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
  todo_wine
  {
    EXPECTRGB(1,0);
    EXPECTRGB(2,0);
  }
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
}

static HRESULT (WINAPI *pVarNumFromParseNum)(NUMPARSE*,BYTE*,ULONG,VARIANT*);

/* Macros for converting and testing the result of VarNumFromParseNum */
#define SETRGB(indx,val) if (!indx) memset(rgb, FAILDIG, sizeof(rgb)); rgb[indx] = val
#undef CONVERT
#define CONVERT(a,b,c,d,e,f,bits) \
    np.cDig = (a); np.dwInFlags = (b); np.dwOutFlags = (c); np.cchUsed = (d); \
    np.nBaseShift = (e); np.nPwr10 = (f); hres = pVarNumFromParseNum(&np, rgb, bits, &vOut)
#define EXPECT_OVERFLOW ok(hres == (HRESULT)DISP_E_OVERFLOW, "Expected overflow, hres = %08lx\n", hres)
#define EXPECT_OK ok(hres == (HRESULT)S_OK, "Call failed, hres = %08lx\n", hres); \
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
#define EXPECT_I8(val) EXPECT_OK { EXPECT_TYPE(VT_I8); \
  ok(V_I8(&vOut) == val, "Expected i8 = %lld, got %lld\n", (LONG64)val, V_I8(&vOut)); }
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

  /* Negative numbers overflow if we have only unsigned outputs */
  /* -1 */
  SETRGB(0, 1); CONVERT(1,0,NUMPRS_NEG,1,0,0, VTBIT_UI1); EXPECT_OVERFLOW;
  /* -0.6 */
  SETRGB(0, 6); CONVERT(1,0,NUMPRS_NEG,1,0,~0u, VTBIT_UI1); EXPECT_OVERFLOW;

  /* Except that rounding is done first, so -0.5 to 0 are accepted as 0 */
  /* -0.5 */
  SETRGB(0, 5); CONVERT(1,0,NUMPRS_NEG,1,0,~0u, VTBIT_UI1); EXPECT_UI1(0);

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

#define DT2UD(dt,flags,r,d,m,y,h,mn,s,ms,dw,dy) \
  memset(&ud, 0, sizeof(ud)); \
  res = pVarUdateFromDate(dt, flags, &ud); \
  ok(r == res && (FAILED(r) || (ud.st.wYear == y && ud.st.wMonth == m && ud.st.wDay == d && \
     ud.st.wHour == h && ud.st.wMinute == mn && ud.st.wSecond == s && \
     ud.st.wMilliseconds == ms && ud.st.wDayOfWeek == dw && ud.wDayOfYear == dy)), \
     "%.16g expected %lx, %d,%d,%d,%d,%d,%d,%d  %d %d, got %lx, %d,%d,%d,%d,%d,%d,%d  %d %d\n", \
     dt, r, d, m, y, h, mn, s, ms, dw, dy, res, ud.st.wDay, ud.st.wMonth, \
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

#define DOS2DT(d,m,y,h,mn,s,r,dt) out = 0.0; \
  dosDate = MKDOSDATE(d,m,y); \
  dosTime = MKDOSTIME(h,mn,s); \
  res = pDosDateTimeToVariantTime(dosDate, dosTime, &out); \
  ok(r == res && (!r || fabs(out-dt) < 1.0e-11), \
     "expected %d, %.16g, got %d, %.16g\n", r, dt, res, out)

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
  static WCHAR szSrc1[] = { '1','\0' };
  static WCHAR szResult1[] = { '1','.','0','0','\0' };
  static WCHAR szSrc2[] = { '-','1','\0' };
  static WCHAR szResult2[] = { '(','1','.','0','0',')','\0' };
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

#define VARFMT(vt,v,val,fmt,ret,str) do { \
  if (out) SysFreeString(out); out = NULL; \
  V_VT(&in) = (vt); v(&in) = val; \
  if (fmt) MultiByteToWideChar(CP_ACP, 0, fmt, -1, buffW, sizeof(buffW)/sizeof(WCHAR)); \
  hres = pVarFormat(&in,fmt ? buffW : NULL,fd,fw,flags,&out); \
  if (SUCCEEDED(hres)) WideCharToMultiByte(CP_ACP, 0, out, -1, buff, sizeof(buff),0,0); \
  else buff[0] = '\0'; \
  ok(hres == ret && (FAILED(ret) || !strcmp(buff, str)), \
     "VT %d|0x%04x Format %s: expected 0x%08lx, '%s', got 0x%08lx, '%s'\n", \
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

  /* 'out' is not cleared */
  out = (BSTR)0x1;
  pVarFormat(&in,NULL,fd,fw,flags,&out); /* Would crash if out is cleared */
  out = NULL;

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

#define VARABS(vt,val,rvt,rval) V_VT(&v) = VT_##vt; V_##vt(&v) = val; \
        memset(&vDst,0,sizeof(vDst)); hres = pVarAbs(&v,&vDst); \
        ok(hres == S_OK && V_VT(&vDst) == VT_##rvt && V_##rvt(&vDst) == (rval), \
           "VarAbs: expected 0x0,%d,%d, got 0x%lX,%d,%d\n", VT_##rvt, (int)(rval), \
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
    VARABS(I2,1,I2,1);
    VARABS(I2,-1,I2,1);
    VARABS(I4,1,I4,1);
    VARABS(I4,-1,I4,1);
    VARABS(UI1,1,UI1,1);
    VARABS(R4,1,R4,1);
    VARABS(R4,-1,R4,1);
    VARABS(R8,1,R8,1);
    VARABS(R8,-1,R8,1);
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

#define VARNOT(vt,val,rvt,rval) V_VT(&v) = VT_##vt; V_##vt(&v) = val; \
        memset(&vDst,0,sizeof(vDst)); hres = pVarNot(&v,&vDst); \
        ok(hres == S_OK && V_VT(&vDst) == VT_##rvt && V_##rvt(&vDst) == (rval), \
        "VarNot: expected 0x0,%d,%d, got 0x%lX,%d,%d\n", VT_##rvt, (int)(rval), \
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
    /* R4,R8,BSTR,DECIMAL,CY->I4, all others remain the same */
    VARNOT(BOOL,VARIANT_TRUE,BOOL,VARIANT_FALSE);
    VARNOT(BOOL,VARIANT_FALSE,BOOL,VARIANT_TRUE);
    VARNOT(I2,-1,I2,0);
    VARNOT(I2,0,I2,-1);
    VARNOT(I2,1,I2,-2);
    VARNOT(I4,1,I4,-2);
    VARNOT(I4,0,I4,-1);
    VARNOT(UI1,1,UI1,254);
    VARNOT(UI1,0,UI1,255);
    VARNOT(R4,1,I4,-2);
    VARNOT(R4,0,I4,-1);
    VARNOT(R8,1,I4,-2);
    VARNOT(R8,0,I4,-1);
    VARNOT(BSTR,(BSTR)szNum0,I4,-1);
    VARNOT(BSTR,(BSTR)szNum1,I4,-2);

    V_VT(&v) = VT_DECIMAL;
    pdec->u.s.sign = DECIMAL_NEG;
    pdec->u.s.scale = 0;
    pdec->Hi32 = 0;
    pdec->u1.s1.Mid32 = 0;
    pdec->u1.s1.Lo32 = 1;
    VARNOT(DECIMAL,*pdec,I4,0);
    todo_wine {
    pcy->int64 = 10000;
    VARNOT(CY,*pcy,I4,-2);
    }
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

static HRESULT (WINAPI *pVarFix)(LPVARIANT,LPVARIANT);

#define VARFIX(vt,val,rvt,rval) V_VT(&v) = VT_##vt; V_##vt(&v) = val; \
        memset(&vDst,0,sizeof(vDst)); hres = pVarFix(&v,&vDst); \
        ok(hres == S_OK && V_VT(&vDst) == VT_##rvt && V_##rvt(&vDst) == (rval), \
        "VarFix: expected 0x0,%d,%d, got 0x%lX,%d,%d\n", VT_##rvt, (int)(rval), \
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
    pdec->u.s.sign = DECIMAL_NEG;
    pdec->u.s.scale = 0;
    pdec->Hi32 = 0;
    pdec->u1.s1.Mid32 = 0;
    pdec->u1.s1.Lo32 = 1;
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

#define VARINT(vt,val,rvt,rval) V_VT(&v) = VT_##vt; V_##vt(&v) = val; \
        memset(&vDst,0,sizeof(vDst)); hres = pVarInt(&v,&vDst); \
        ok(hres == S_OK && V_VT(&vDst) == VT_##rvt && V_##rvt(&vDst) == (rval), \
        "VarInt: expected 0x0,%d,%d, got 0x%lX,%d,%d\n", VT_##rvt, (int)(rval), \
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
    pdec->u.s.sign = DECIMAL_NEG;
    pdec->u.s.scale = 0;
    pdec->Hi32 = 0;
    pdec->u1.s1.Mid32 = 0;
    pdec->u1.s1.Lo32 = 1;
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

#define VARNEG(vt,val,rvt,rval) V_VT(&v) = VT_##vt; V_##vt(&v) = val; \
        memset(&vDst,0,sizeof(vDst)); hres = pVarNeg(&v,&vDst); \
        ok(hres == S_OK && V_VT(&vDst) == VT_##rvt && V_##rvt(&vDst) == (rval), \
        "VarNeg: expected 0x0,%d,%d, got 0x%lX,%d,%d\n", VT_##rvt, (int)(rval), \
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
    pdec->u.s.sign = DECIMAL_NEG;
    pdec->u.s.scale = 0;
    pdec->Hi32 = 0;
    pdec->u1.s1.Mid32 = 0;
    pdec->u1.s1.Lo32 = 1;
    hres = pVarNeg(&v,&vDst);
    ok(hres == S_OK && V_VT(&vDst) == VT_DECIMAL &&
       V_DECIMAL(&vDst).u.s.sign == 0,
       "VarNeg: expected 0x0,%d,0x00, got 0x%lX,%d,%02x\n", VT_DECIMAL,
       hres, V_VT(&vDst), V_DECIMAL(&vDst).u.s.sign);

    pdec->u.s.sign = 0;
    hres = pVarNeg(&v,&vDst);
    ok(hres == S_OK && V_VT(&vDst) == VT_DECIMAL &&
       V_DECIMAL(&vDst).u.s.sign == DECIMAL_NEG,
       "VarNeg: expected 0x0,%d,0x7f, got 0x%lX,%d,%02x\n", VT_DECIMAL,
       hres, V_VT(&vDst), V_DECIMAL(&vDst).u.s.sign);

    V_VT(&v) = VT_CY;
    pcy->int64 = -10000;
    hres = pVarNeg(&v,&vDst);
    ok(hres == S_OK && V_VT(&vDst) == VT_CY && V_CY(&vDst).int64 == 10000,
       "VarNeg: VT_CY wrong, hres=0x%lX\n", hres);
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
  test_VarFix();
  test_VarInt();
  test_VarNeg();
}
