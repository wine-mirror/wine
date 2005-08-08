/*
 * Variant Inlines
 *
 * Copyright 2003 Jon Griffiths
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
#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winerror.h"
#include "objbase.h"
#include "oleauto.h"
#include <math.h>

/* Get just the type from a variant pointer */
#define V_TYPE(v)  (V_VT((v)) & VT_TYPEMASK)

/* Flags set in V_VT, other than the actual type value */
#define VT_EXTRA_TYPE (VT_VECTOR|VT_ARRAY|VT_BYREF|VT_RESERVED)

/* Get the extra flags from a variant pointer */
#define V_EXTRA_TYPE(v) (V_VT((v)) & VT_EXTRA_TYPE)

extern const char* wine_vtypes[];
#define debugstr_vt(v) (((v)&VT_TYPEMASK) <= VT_CLSID ? wine_vtypes[((v)&VT_TYPEMASK)] : \
  ((v)&VT_TYPEMASK) == VT_BSTR_BLOB ? "VT_BSTR_BLOB": "Invalid")
#define debugstr_VT(v) (!(v) ? "(null)" : debugstr_vt(V_TYPE((v))))

extern const char* wine_vflags[];
#define debugstr_vf(v) (wine_vflags[((v)&VT_EXTRA_TYPE)>>12])
#define debugstr_VF(v) (!(v) ? "(null)" : debugstr_vf(V_EXTRA_TYPE(v)))

/* Size constraints */
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
#define R4_MAX 3.402823567797336e38
#define R4_MIN 1.40129846432481707e-45
#define R8_MAX 1.79769313486231470e+308
#define R8_MIN 4.94065645841246544e-324

/* Value of sign for a positive decimal number */
#define DECIMAL_POS 0

/* Native headers don't change the union ordering for DECIMAL sign/scale (duh).
 * This means that the signscale member is only useful for setting both members to 0.
 * SIGNSCALE creates endian-correct values so that we can properly set both at once
 * to values other than 0.
 */
#ifdef WORDS_BIGENDIAN
#define SIGNSCALE(sign,scale) (((scale) << 8) | sign)
#else
#define SIGNSCALE(sign,scale) (((sign) << 8) | scale)
#endif

/* Macros for getting at a DECIMAL's parts */
#define DEC_SIGN(d)      ((d)->u.s.sign)
#define DEC_SCALE(d)     ((d)->u.s.scale)
#define DEC_SIGNSCALE(d) ((d)->u.signscale)
#define DEC_HI32(d)      ((d)->Hi32)
#define DEC_MID32(d)     ((d)->u1.s1.Mid32)
#define DEC_LO32(d)      ((d)->u1.s1.Lo32)
#define DEC_LO64(d)      ((d)->u1.Lo64)

#define DEC_MAX_SCALE    28 /* Maximum scale for a decimal */

/* Inline return type */
#define RETTYP inline static HRESULT

/* Simple compiler cast from one type to another */
#define SIMPLE(dest, src, func) RETTYP _##func(src in, dest* out) { \
  *out = in; return S_OK; }

/* Compiler cast where input cannot be negative */
#define NEGTST(dest, src, func) RETTYP _##func(src in, dest* out) { \
  if (in < (src)0) return DISP_E_OVERFLOW; *out = in; return S_OK; }

/* Compiler cast where input cannot be > some number */
#define POSTST(dest, src, func, tst) RETTYP _##func(src in, dest* out) { \
  if (in > (dest)tst) return DISP_E_OVERFLOW; *out = in; return S_OK; }

/* Compiler cast where input cannot be < some number or >= some other number */
#define BOTHTST(dest, src, func, lo, hi) RETTYP _##func(src in, dest* out) { \
  if (in < (dest)lo || in > hi) return DISP_E_OVERFLOW; *out = in; return S_OK; }

#define CY_MULTIPLIER   10000             /* 4 dp of precision */
#define CY_MULTIPLIER_F 10000.0
#define CY_HALF         (CY_MULTIPLIER/2) /* 0.5 */
#define CY_HALF_F       (CY_MULTIPLIER_F/2.0)

/* I1 */
POSTST(signed char, BYTE, VarI1FromUI1, I1_MAX);
BOTHTST(signed char, SHORT, VarI1FromI2, I1_MIN, I1_MAX);
BOTHTST(signed char, LONG, VarI1FromI4, I1_MIN, I1_MAX);
SIMPLE(signed char, VARIANT_BOOL, VarI1FromBool);
POSTST(signed char, USHORT, VarI1FromUI2, I1_MAX);
POSTST(signed char, ULONG, VarI1FromUI4, I1_MAX);
BOTHTST(signed char, LONG64, VarI1FromI8, I1_MIN, I1_MAX);
POSTST(signed char, ULONG64, VarI1FromUI8, I1_MAX);

/* UI1 */
BOTHTST(BYTE, SHORT, VarUI1FromI2, UI1_MIN, UI1_MAX);
SIMPLE(BYTE, VARIANT_BOOL, VarUI1FromBool);
NEGTST(BYTE, signed char, VarUI1FromI1);
POSTST(BYTE, USHORT, VarUI1FromUI2, UI1_MAX);
BOTHTST(BYTE, LONG, VarUI1FromI4, UI1_MIN, UI1_MAX);
POSTST(BYTE, ULONG, VarUI1FromUI4, UI1_MAX);
BOTHTST(BYTE, LONG64, VarUI1FromI8, UI1_MIN, UI1_MAX);
POSTST(BYTE, ULONG64, VarUI1FromUI8, UI1_MAX);

/* I2 */
SIMPLE(SHORT, BYTE, VarI2FromUI1);
BOTHTST(SHORT, LONG, VarI2FromI4, I2_MIN, I2_MAX);
SIMPLE(SHORT, VARIANT_BOOL, VarI2FromBool);
SIMPLE(SHORT, signed char, VarI2FromI1);
POSTST(SHORT, USHORT, VarI2FromUI2, I2_MAX);
POSTST(SHORT, ULONG, VarI2FromUI4, I2_MAX);
BOTHTST(SHORT, LONG64, VarI2FromI8, I2_MIN, I2_MAX);
POSTST(SHORT, ULONG64, VarI2FromUI8, I2_MAX);

/* UI2 */
SIMPLE(USHORT, BYTE, VarUI2FromUI1);
NEGTST(USHORT, SHORT, VarUI2FromI2);
BOTHTST(USHORT, LONG, VarUI2FromI4, UI2_MIN, UI2_MAX);
SIMPLE(USHORT, VARIANT_BOOL, VarUI2FromBool);
NEGTST(USHORT, signed char, VarUI2FromI1);
POSTST(USHORT, ULONG, VarUI2FromUI4, UI2_MAX);
BOTHTST(USHORT, LONG64, VarUI2FromI8, UI2_MIN, UI2_MAX);
POSTST(USHORT, ULONG64, VarUI2FromUI8, UI2_MAX);

/* I4 */
SIMPLE(LONG, BYTE, VarI4FromUI1);
SIMPLE(LONG, SHORT, VarI4FromI2);
SIMPLE(LONG, VARIANT_BOOL, VarI4FromBool);
SIMPLE(LONG, signed char, VarI4FromI1);
SIMPLE(LONG, USHORT, VarI4FromUI2);
POSTST(LONG, ULONG, VarI4FromUI4, I4_MAX);
BOTHTST(LONG, LONG64, VarI4FromI8, I4_MIN, I4_MAX);
POSTST(LONG, ULONG64, VarI4FromUI8, I4_MAX);

/* UI4 */
SIMPLE(ULONG, BYTE, VarUI4FromUI1);
NEGTST(ULONG, SHORT, VarUI4FromI2);
NEGTST(ULONG, LONG, VarUI4FromI4);
SIMPLE(ULONG, VARIANT_BOOL, VarUI4FromBool);
NEGTST(ULONG, signed char, VarUI4FromI1);
SIMPLE(ULONG, USHORT, VarUI4FromUI2);
BOTHTST(ULONG, LONG64, VarUI4FromI8, UI4_MIN, UI4_MAX);
POSTST(ULONG, ULONG64, VarUI4FromUI8, UI4_MAX);

/* I8 */
SIMPLE(LONG64, BYTE, VarI8FromUI1);
SIMPLE(LONG64, SHORT, VarI8FromI2);
SIMPLE(LONG64, signed char, VarI8FromI1);
SIMPLE(LONG64, USHORT, VarI8FromUI2);
SIMPLE(LONG64, LONG, VarI8FromI4);
SIMPLE(LONG64, ULONG, VarI8FromUI4);
POSTST(LONG64, ULONG64, VarI8FromUI8, I8_MAX);

/* UI8 */
SIMPLE(ULONG64, BYTE, VarUI8FromUI1);
NEGTST(ULONG64, SHORT, VarUI8FromI2);
NEGTST(ULONG64, signed char, VarUI8FromI1);
SIMPLE(ULONG64, USHORT, VarUI8FromUI2);
NEGTST(ULONG64, LONG, VarUI8FromI4);
SIMPLE(ULONG64, ULONG, VarUI8FromUI4);
NEGTST(ULONG64, LONG64, VarUI8FromI8);

/* R4 (float) */
SIMPLE(float, BYTE, VarR4FromUI1);
SIMPLE(float, SHORT, VarR4FromI2);
SIMPLE(float, signed char, VarR4FromI1);
SIMPLE(float, USHORT, VarR4FromUI2);
SIMPLE(float, LONG, VarR4FromI4);
SIMPLE(float, ULONG, VarR4FromUI4);
SIMPLE(float, LONG64, VarR4FromI8);
SIMPLE(float, ULONG64, VarR4FromUI8);

/* R8 (double) */
SIMPLE(double, BYTE, VarR8FromUI1);
SIMPLE(double, SHORT, VarR8FromI2);
SIMPLE(double, float, VarR8FromR4);
RETTYP _VarR8FromCy(CY i, double* o) { *o = (double)i.int64 / CY_MULTIPLIER_F; return S_OK; }
SIMPLE(double, DATE, VarR8FromDate);
SIMPLE(double, signed char, VarR8FromI1);
SIMPLE(double, USHORT, VarR8FromUI2);
SIMPLE(double, LONG, VarR8FromI4);
SIMPLE(double, ULONG, VarR8FromUI4);
SIMPLE(double, LONG64, VarR8FromI8);
SIMPLE(double, ULONG64, VarR8FromUI8);

/* Internal flags for low level conversion functions */
#define  VAR_BOOLONOFF 0x0400 /* Convert bool to "On"/"Off" */
#define  VAR_BOOLYESNO 0x0800 /* Convert bool to "Yes"/"No" */
#define  VAR_NEGATIVE  0x1000 /* Number is negative */

/* Macro to inline conversion from a float or double to any integer type,
 * rounding according to the 'dutch' convention.
 */
#define OLEAUT32_DutchRound(typ, value, res) do { \
  double whole = (double)value < 0 ? ceil((double)value) : floor((double)value); \
  double fract = (double)value - whole; \
  if (fract > 0.5) res = (typ)whole + (typ)1; \
  else if (fract == 0.5) { typ is_odd = (typ)whole & 1; res = whole + is_odd; } \
  else if (fract >= 0.0) res = (typ)whole; \
  else if (fract == -0.5) { typ is_odd = (typ)whole & 1; res = whole - is_odd; } \
  else if (fract > -0.5) res = (typ)whole; \
  else res = (typ)whole - (typ)1; \
} while(0);

/* The localised characters that make up a valid number */
typedef struct tagVARIANT_NUMBER_CHARS
{
  WCHAR cNegativeSymbol;
  WCHAR cPositiveSymbol;
  WCHAR cDecimalPoint;
  WCHAR cDigitSeperator;
  WCHAR cCurrencyLocal;
  WCHAR cCurrencyLocal2;
  WCHAR cCurrencyDecimalPoint;
  WCHAR cCurrencyDigitSeperator;
} VARIANT_NUMBER_CHARS;

void VARIANT_GetLocalisedNumberChars(VARIANT_NUMBER_CHARS*,LCID,DWORD);
