/*
 * Large integer functions
 *
 * Copyright 2000 Alexandre Julliard
 * Copyright 2003 Thomas Mertes
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

#include "windef.h"
#include "winternl.h"

/*
 * Note: we use LONGLONG instead of LARGE_INTEGER, because
 * the latter is a structure and the calling convention for
 * returning a structure would not be binary-compatible.
 *
 * FIXME: for platforms that don't have a native LONGLONG type,
 * we should define LONGLONG as a structure similar to LARGE_INTEGER
 * and do everything by hand. You are welcome to do it...
 */

/******************************************************************************
 *        RtlLargeIntegerAdd   (NTDLL.@)
 */
LONGLONG WINAPI RtlLargeIntegerAdd( LONGLONG a, LONGLONG b )
{
    return a + b;
}


/******************************************************************************
 *        RtlLargeIntegerSubtract   (NTDLL.@)
 */
LONGLONG WINAPI RtlLargeIntegerSubtract( LONGLONG a, LONGLONG b )
{
    return a - b;
}


/******************************************************************************
 *        RtlLargeIntegerNegate   (NTDLL.@)
 */
LONGLONG WINAPI RtlLargeIntegerNegate( LONGLONG a )
{
    return -a;
}


/******************************************************************************
 *        RtlLargeIntegerShiftLeft   (NTDLL.@)
 */
LONGLONG WINAPI RtlLargeIntegerShiftLeft( LONGLONG a, INT count )
{
    return a << count;
}


/******************************************************************************
 *        RtlLargeIntegerShiftRight   (NTDLL.@)
 */
LONGLONG WINAPI RtlLargeIntegerShiftRight( LONGLONG a, INT count )
{
    return (ULONGLONG)a >> count;
}


/******************************************************************************
 *        RtlLargeIntegerArithmeticShift   (NTDLL.@)
 */
LONGLONG WINAPI RtlLargeIntegerArithmeticShift( LONGLONG a, INT count )
{
    /* FIXME: gcc does arithmetic shift here, but it may not be true on all platforms */
    return a >> count;
}


/******************************************************************************
 *        RtlLargeIntegerDivide   (NTDLL.@)
 *
 * FIXME: should it be signed division instead?
 */
ULONGLONG WINAPI RtlLargeIntegerDivide( ULONGLONG a, ULONGLONG b, ULONGLONG *rem )
{
    ULONGLONG ret = a / b;
    if (rem) *rem = a - ret * b;
    return ret;
}


/******************************************************************************
 *        RtlConvertLongToLargeInteger   (NTDLL.@)
 */
LONGLONG WINAPI RtlConvertLongToLargeInteger( LONG a )
{
    return a;
}


/******************************************************************************
 *        RtlConvertUlongToLargeInteger   (NTDLL.@)
 */
ULONGLONG WINAPI RtlConvertUlongToLargeInteger( ULONG a )
{
    return a;
}


/******************************************************************************
 *        RtlEnlargedIntegerMultiply   (NTDLL.@)
 */
LONGLONG WINAPI RtlEnlargedIntegerMultiply( INT a, INT b )
{
    return (LONGLONG)a * b;
}


/******************************************************************************
 *        RtlEnlargedUnsignedMultiply   (NTDLL.@)
 */
ULONGLONG WINAPI RtlEnlargedUnsignedMultiply( UINT a, UINT b )
{
    return (ULONGLONG)a * b;
}


/******************************************************************************
 *        RtlEnlargedUnsignedDivide   (NTDLL.@)
 */
UINT WINAPI RtlEnlargedUnsignedDivide( ULONGLONG a, UINT b, UINT *remptr )
{
#if defined(__i386__) && defined(__GNUC__)
    UINT ret, rem, p1, p2;

    p1 = a >> 32;
    p2 = a &  0xffffffffLL;

    __asm__("div %4,%%eax"
            : "=a" (ret), "=d" (rem)
            : "0" (p2), "1" (p1), "g" (b) );
    if (remptr) *remptr = rem;
    return ret;
#else
    UINT ret = a / b;
    if (remptr) *remptr = a % b;
    return ret;
#endif
}


/******************************************************************************
 *        RtlExtendedLargeIntegerDivide   (NTDLL.@)
 */
LONGLONG WINAPI RtlExtendedLargeIntegerDivide( LONGLONG a, INT b, INT *rem )
{
    LONGLONG ret = a / b;
    if (rem) *rem = a - b * ret;
    return ret;
}


/******************************************************************************
 *        RtlExtendedIntegerMultiply   (NTDLL.@)
 */
LONGLONG WINAPI RtlExtendedIntegerMultiply( LONGLONG a, INT b )
{
    return a * b;
}


/******************************************************************************
 *        RtlExtendedMagicDivide   (NTDLL.@)
 *
 * This function computes (a * b) >> (64 + shift)
 *
 * RETURNS
 *  (a * b) >> (64 + shift)
 *
 * NOTES
 *  This allows replacing a division by a longlong constant
 *  by a multiplication by the inverse constant.
 *
 *  If 'c' is the constant divisor, the constants 'b' and 'shift'
 *  must be chosen such that b = 2^(64+shift) / c.
 *  Then we have RtlExtendedMagicDivide(a,b,shift) == a * b / 2^(64+shift) == a / c.
 *
 *  The Parameter b although defined as LONGLONG is used as ULONGLONG.
 */
#define LOWER_32(A) ((A) & 0xffffffff)
#define UPPER_32(A) ((A) >> 32)
LONGLONG WINAPI RtlExtendedMagicDivide(
    LONGLONG a, /* [I] Dividend to be divided by the constant divisor */
    LONGLONG b, /* [I] Constant computed manually as 2^(64+shift) / divisor */
    INT shift)  /* [I] Constant shift chosen to make b as big as possible for 64 bits */
{
    ULONGLONG a_high;
    ULONGLONG a_low;
    ULONGLONG b_high;
    ULONGLONG b_low;
    ULONGLONG ah_bl;
    ULONGLONG al_bh;
    LONGLONG result;
    int positive;

    if (a < 0) {
	a_high = UPPER_32((ULONGLONG) -a);
	a_low =  LOWER_32((ULONGLONG) -a);
	positive = 0;
    } else {
	a_high = UPPER_32((ULONGLONG) a);
	a_low =  LOWER_32((ULONGLONG) a);
	positive = 1;
    } /* if */
    b_high = UPPER_32((ULONGLONG) b);
    b_low =  LOWER_32((ULONGLONG) b);

    ah_bl = a_high * b_low;
    al_bh = a_low * b_high;

    result = (LONGLONG) ((a_high * b_high +
	    UPPER_32(ah_bl) +
	    UPPER_32(al_bh) +
	    UPPER_32(LOWER_32(ah_bl) + LOWER_32(al_bh) + UPPER_32(a_low * b_low))) >> shift);

    if (positive) {
	return result;
    } else {
	return -result;
    } /* if */
}


/******************************************************************************
 *      RtlLargeIntegerToChar	[NTDLL.@]
 *
 * Convert an unsigned large integer to a character string.
 *
 * RETURNS
 *  Success: STATUS_SUCCESS. str contains the converted number
 *  Failure: STATUS_INVALID_PARAMETER, if base is not 0, 2, 8, 10 or 16.
 *           STATUS_BUFFER_OVERFLOW, if str would be larger than length.
 *           STATUS_ACCESS_VIOLATION, if str is NULL.
 *
 * NOTES
 *  Instead of base 0 it uses 10 as base.
 *  Writes at most length characters to the string str.
 *  Str is '\0' terminated when length allowes it.
 *  When str fits exactly in length characters the '\0' is ommitted.
 *  If value_ptr is NULL it crashes, as the native function does.
 *
 * DIFFERENCES
 * - Accept base 0 as 10 instead of crashing as native function does.
 * - The native function does produce garbage or STATUS_BUFFER_OVERFLOW for
 *   base 2, 8 and 16 when the value is larger than 0xFFFFFFFF. 
 */
NTSTATUS WINAPI RtlLargeIntegerToChar(
    const ULONGLONG *value_ptr, /* [I] Pointer to the value to be converted */
    ULONG base,                 /* [I] Number base for conversion (allowed 0, 2, 8, 10 or 16) */
    ULONG length,               /* [I] Length of the str buffer in bytes */
    PCHAR str)                  /* [O] Destination for the converted value */
{
    ULONGLONG value = *value_ptr;
    CHAR buffer[65];
    PCHAR pos;
    CHAR digit;
    ULONG len;

    if (base == 0) {
	base = 10;
    } else if (base != 2 && base != 8 && base != 10 && base != 16) {
	return STATUS_INVALID_PARAMETER;
    } /* if */

    pos = &buffer[64];
    *pos = '\0';

    do {
	pos--;
	digit = value % base;
	value = value / base;
	if (digit < 10) {
	    *pos = '0' + digit;
	} else {
	    *pos = 'A' + digit - 10;
	} /* if */
    } while (value != 0L);

    len = &buffer[64] - pos;
    if (len > length) {
	return STATUS_BUFFER_OVERFLOW;
    } else if (str == NULL) {
	return STATUS_ACCESS_VIOLATION;
    } else if (len == length) {
	memcpy(str, pos, len);
    } else {
	memcpy(str, pos, len + 1);
    } /* if */
    return STATUS_SUCCESS;
}


/**************************************************************************
 *      RtlInt64ToUnicodeString (NTDLL.@)
 *
 * Convert a large unsigned integer to a '\0' terminated unicode string.
 *
 * RETURNS
 *  Success: STATUS_SUCCESS. str contains the converted number
 *  Failure: STATUS_INVALID_PARAMETER, if base is not 0, 2, 8, 10 or 16.
 *           STATUS_BUFFER_OVERFLOW, if str is too small to hold the string
 *                  (with the '\0' termination). In this case str->Length
 *                  is set to the length, the string would have (which can
 *                  be larger than the MaximumLength).
 *
 * NOTES
 *  Instead of base 0 it uses 10 as base.
 *  If str is NULL it crashes, as the native function does.
 *
 * DIFFERENCES
 * - Accept base 0 as 10 instead of crashing as native function does.
 * - Do not return STATUS_BUFFER_OVERFLOW when the string is long enough.
 *   The native function does this when the string would be longer than 31
 *   characters even when the string parameter is long enough.
 * - The native function does produce garbage or STATUS_BUFFER_OVERFLOW for
 *   base 2, 8 and 16 when the value is larger than 0xFFFFFFFF. 
 */
NTSTATUS WINAPI RtlInt64ToUnicodeString(
    ULONGLONG value,     /* [I] Value to be converted */
    ULONG base,          /* [I] Number base for conversion (allowed 0, 2, 8, 10 or 16) */
    UNICODE_STRING *str) /* [O] Destination for the converted value */
{
    WCHAR buffer[65];
    PWCHAR pos;
    WCHAR digit;

    if (base == 0) {
	base = 10;
    } else if (base != 2 && base != 8 && base != 10 && base != 16) {
	return STATUS_INVALID_PARAMETER;
    } /* if */

    pos = &buffer[64];
    *pos = '\0';

    do {
	pos--;
	digit = value % base;
	value = value / base;
	if (digit < 10) {
	    *pos = '0' + digit;
	} else {
	    *pos = 'A' + digit - 10;
	} /* if */
    } while (value != 0L);

    str->Length = (&buffer[64] - pos) * sizeof(WCHAR);
    if (str->Length >= str->MaximumLength) {
	return STATUS_BUFFER_OVERFLOW;
    } else {
	memcpy(str->Buffer, pos, str->Length + sizeof(WCHAR));
    } /* if */
    return STATUS_SUCCESS;
}


/******************************************************************************
 *        _alldiv   (NTDLL.@)
 */
LONGLONG WINAPI _alldiv( LONGLONG a, LONGLONG b )
{
    return a / b;
}


/******************************************************************************
 *        _allmul   (NTDLL.@)
 */
LONGLONG WINAPI _allmul( LONGLONG a, LONGLONG b )
{
    return a * b;
}


/******************************************************************************
 *        _allrem   (NTDLL.@)
 */
LONGLONG WINAPI _allrem( LONGLONG a, LONGLONG b )
{
    return a % b;
}


/******************************************************************************
 *        _aulldiv   (NTDLL.@)
 */
ULONGLONG WINAPI _aulldiv( ULONGLONG a, ULONGLONG b )
{
    return a / b;
}


/******************************************************************************
 *        _aullrem   (NTDLL.@)
 */
ULONGLONG WINAPI _aullrem( ULONGLONG a, ULONGLONG b )
{
    return a % b;
}
