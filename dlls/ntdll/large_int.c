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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"

#ifndef _WIN64

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
 *
 * Add two 64 bit integers.
 *
 * PARAMS
 *  a [I] Initial number.
 *  b [I] Number to add to a.
 *
 * RETURNS
 *  The sum of a and b.
 */
LONGLONG WINAPI RtlLargeIntegerAdd( LONGLONG a, LONGLONG b )
{
    return a + b;
}


/******************************************************************************
 *        RtlLargeIntegerSubtract   (NTDLL.@)
 *
 * Subtract two 64 bit integers.
 *
 * PARAMS
 *  a [I] Initial number.
 *  b [I] Number to subtract from a.
 *
 * RETURNS
 *  The difference of a and b.
 */
LONGLONG WINAPI RtlLargeIntegerSubtract( LONGLONG a, LONGLONG b )
{
    return a - b;
}


/******************************************************************************
 *        RtlLargeIntegerNegate   (NTDLL.@)
 *
 * Negate a 64 bit integer.
 *
 * PARAMS
 *  a     [I] Initial number.
 *
 * RETURNS
 *  The value of a negated.
 */
LONGLONG WINAPI RtlLargeIntegerNegate( LONGLONG a )
{
    return -a;
}


/******************************************************************************
 *        RtlLargeIntegerShiftLeft   (NTDLL.@)
 *
 * Perform a shift left on a 64 bit integer.
 *
 * PARAMS
 *  a     [I] Initial number.
 *  count [I] Number of bits to shift by
 *
 * RETURNS
 *  The value of a following the shift.
 */
LONGLONG WINAPI RtlLargeIntegerShiftLeft( LONGLONG a, INT count )
{
    return a << count;
}


/******************************************************************************
 *        RtlLargeIntegerShiftRight   (NTDLL.@)
 *
 * Perform a shift right on a 64 bit integer.
 *
 * PARAMS
 *  a     [I] Initial number.
 *  count [I] Number of bits to shift by
 *
 * RETURNS
 *  The value of a following the shift.
 */
LONGLONG WINAPI RtlLargeIntegerShiftRight( LONGLONG a, INT count )
{
    return (ULONGLONG)a >> count;
}


/******************************************************************************
 *        RtlLargeIntegerArithmeticShift   (NTDLL.@)
 *
 * Perform an arithmetic shift right on a 64 bit integer.
 *
 * PARAMS
 *  a     [I] Initial number.
 *  count [I] Number of bits to shift by
 *
 * RETURNS
 *  The value of a following the shift.
 */
LONGLONG WINAPI RtlLargeIntegerArithmeticShift( LONGLONG a, INT count )
{
    /* FIXME: gcc does arithmetic shift here, but it may not be true on all platforms */
    return a >> count;
}


/******************************************************************************
 *        RtlLargeIntegerDivide   (NTDLL.@)
 *
 * Divide one 64 bit unsigned integer by another, with remainder.
 *
 * PARAMS
 *  a   [I] Initial number.
 *  b   [I] Number to divide a by
 *  rem [O] Destination for remainder
 *
 * RETURNS
 *  The dividend of a and b. If rem is non-NULL it is set to the remainder.
 *
 * FIXME
 *  Should it be signed division instead?
 */
ULONGLONG WINAPI RtlLargeIntegerDivide( ULONGLONG a, ULONGLONG b, ULONGLONG *rem )
{
    ULONGLONG ret = a / b;
    if (rem) *rem = a - ret * b;
    return ret;
}


/******************************************************************************
 *        RtlConvertLongToLargeInteger   (NTDLL.@)
 *
 * Convert a 32 bit integer into 64 bits.
 *
 * PARAMS
 *  a [I] Number to convert
 *
 * RETURNS
 *  a.
 */
LONGLONG WINAPI RtlConvertLongToLargeInteger( LONG a )
{
    return a;
}


/******************************************************************************
 *        RtlConvertUlongToLargeInteger   (NTDLL.@)
 *
 * Convert a 32 bit unsigned integer into 64 bits.
 *
 * PARAMS
 *  a [I] Number to convert
 *
 * RETURNS
 *  a.
 */
ULONGLONG WINAPI RtlConvertUlongToLargeInteger( ULONG a )
{
    return a;
}


/******************************************************************************
 *        RtlEnlargedIntegerMultiply   (NTDLL.@)
 *
 * Multiply two integers giving a 64 bit integer result.
 *
 * PARAMS
 *  a [I] Initial number.
 *  b [I] Number to multiply a by.
 *
 * RETURNS
 *  The product of a and b.
 */
LONGLONG WINAPI RtlEnlargedIntegerMultiply( INT a, INT b )
{
    return (LONGLONG)a * b;
}


/******************************************************************************
 *        RtlEnlargedUnsignedMultiply   (NTDLL.@)
 *
 * Multiply two unsigned integers giving a 64 bit unsigned integer result.
 *
 * PARAMS
 *  a [I] Initial number.
 *  b [I] Number to multiply a by.
 *
 * RETURNS
 *  The product of a and b.
 */
ULONGLONG WINAPI RtlEnlargedUnsignedMultiply( UINT a, UINT b )
{
    return (ULONGLONG)a * b;
}


/******************************************************************************
 *        RtlEnlargedUnsignedDivide   (NTDLL.@)
 *
 * Divide one 64 bit unsigned integer by a 32 bit unsigned integer, with remainder.
 *
 * PARAMS
 *  a      [I] Initial number.
 *  b      [I] Number to divide a by
 *  remptr [O] Destination for remainder
 *
 * RETURNS
 *  The dividend of a and b. If remptr is non-NULL it is set to the remainder.
 */
UINT WINAPI RtlEnlargedUnsignedDivide( ULONGLONG a, UINT b, UINT *remptr )
{
#if defined(__i386__) && defined(__GNUC__)
    UINT ret, rem;

    __asm__("divl %4"
            : "=a" (ret), "=d" (rem)
            : "0" ((UINT)a), "1" ((UINT)(a >> 32)), "g" (b) );
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
 *
 * Divide one 64 bit integer by a 32 bit integer, with remainder.
 *
 * PARAMS
 *  a   [I] Initial number.
 *  b   [I] Number to divide a by
 *  rem [O] Destination for remainder
 *
 * RETURNS
 *  The dividend of a and b. If rem is non-NULL it is set to the remainder.
 */
LONGLONG WINAPI RtlExtendedLargeIntegerDivide( LONGLONG a, INT b, INT *rem )
{
    LONGLONG ret = a / b;
    if (rem) *rem = a - b * ret;
    return ret;
}


/******************************************************************************
 *        RtlExtendedIntegerMultiply   (NTDLL.@)
 *
 * Multiply one 64 bit integer by another 32 bit integer.
 *
 * PARAMS
 *  a [I] Initial number.
 *  b [I] Number to multiply a by.
 *
 * RETURNS
 *  The product of a and b.
 */
LONGLONG WINAPI RtlExtendedIntegerMultiply( LONGLONG a, INT b )
{
    return a * b;
}


/******************************************************************************
 *        RtlExtendedMagicDivide   (NTDLL.@)
 *
 * Allows replacing a division by a longlong constant with a multiplication by
 * the inverse constant.
 *
 * RETURNS
 *  (dividend * inverse_divisor) >> (64 + shift)
 *
 * NOTES
 *  If the divisor of a division is constant, the constants inverse_divisor and
 *  shift must be chosen such that inverse_divisor = 2^(64 + shift) / divisor.
 *  Then we have RtlExtendedMagicDivide(dividend,inverse_divisor,shift) ==
 *  dividend * inverse_divisor / 2^(64 + shift) == dividend / divisor.
 *
 *  The Parameter inverse_divisor although defined as LONGLONG is used as
 *  ULONGLONG.
 */
#define LOWER_32(A) ((A) & 0xffffffff)
#define UPPER_32(A) ((A) >> 32)
LONGLONG WINAPI RtlExtendedMagicDivide(
    LONGLONG dividend,        /* [I] Dividend to be divided by the constant divisor */
    LONGLONG inverse_divisor, /* [I] Constant computed manually as 2^(64+shift) / divisor */
    INT shift)                /* [I] Constant shift chosen to make inverse_divisor as big as possible for 64 bits */
{
    ULONGLONG dividend_high;
    ULONGLONG dividend_low;
    ULONGLONG inverse_divisor_high;
    ULONGLONG inverse_divisor_low;
    ULONGLONG ah_bl;
    ULONGLONG al_bh;
    LONGLONG result;
    int positive;

    if (dividend < 0) {
	dividend_high = UPPER_32((ULONGLONG) -dividend);
	dividend_low =  LOWER_32((ULONGLONG) -dividend);
	positive = 0;
    } else {
	dividend_high = UPPER_32((ULONGLONG) dividend);
	dividend_low =  LOWER_32((ULONGLONG) dividend);
	positive = 1;
    } /* if */
    inverse_divisor_high = UPPER_32((ULONGLONG) inverse_divisor);
    inverse_divisor_low =  LOWER_32((ULONGLONG) inverse_divisor);

    ah_bl = dividend_high * inverse_divisor_low;
    al_bh = dividend_low * inverse_divisor_high;

    result = (LONGLONG) ((dividend_high * inverse_divisor_high +
	    UPPER_32(ah_bl) +
	    UPPER_32(al_bh) +
	    UPPER_32(LOWER_32(ah_bl) + LOWER_32(al_bh) +
		     UPPER_32(dividend_low * inverse_divisor_low))) >> shift);

    if (positive) {
	return result;
    } else {
	return -result;
    } /* if */
}


/*************************************************************************
 *        RtlInterlockedCompareExchange64   (NTDLL.@)
 */
LONGLONG WINAPI RtlInterlockedCompareExchange64( LONGLONG *dest, LONGLONG xchg, LONGLONG compare )
{
    return interlocked_cmpxchg64( dest, xchg, compare );
}

#endif  /* _WIN64 */

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
 *  Str is '\0' terminated when length allows it.
 *  When str fits exactly in length characters the '\0' is omitted.
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


#ifdef __i386__

/******************************************************************************
 *        _alldiv   (NTDLL.@)
 *
 * Divide two 64 bit unsigned integers.
 *
 * PARAMS
 *  a [I] Initial number.
 *  b [I] Number to divide a by.
 *
 * RETURNS
 *  The dividend of a and b.
 */
LONGLONG WINAPI _alldiv( LONGLONG a, LONGLONG b )
{
    return a / b;
}


/******************************************************************************
 *        _allmul   (NTDLL.@)
 *
 * Multiply two 64 bit integers.
 *
 * PARAMS
 *  a [I] Initial number.
 *  b [I] Number to multiply a by.
 *
 * RETURNS
 *  The product of a and b.
 */
LONGLONG WINAPI _allmul( LONGLONG a, LONGLONG b )
{
    return a * b;
}


/******************************************************************************
 *        _allrem   (NTDLL.@)
 *
 * Calculate the remainder after dividing two 64 bit integers.
 *
 * PARAMS
 *  a [I] Initial number.
 *  b [I] Number to divide a by.
 *
 * RETURNS
 *  The remainder of a divided by b.
 */
LONGLONG WINAPI _allrem( LONGLONG a, LONGLONG b )
{
    return a % b;
}


/******************************************************************************
 *        _aulldiv   (NTDLL.@)
 *
 * Divide two 64 bit unsigned integers.
 *
 * PARAMS
 *  a [I] Initial number.
 *  b [I] Number to divide a by.
 *
 * RETURNS
 *  The dividend of a and b.
 */
ULONGLONG WINAPI _aulldiv( ULONGLONG a, ULONGLONG b )
{
    return a / b;
}

/******************************************************************************
 *        _allshl   (NTDLL.@)
 *
 * Shift a 64 bit integer to the left.
 *
 * PARAMS
 *  a [I] Initial number.
 *  b [I] Number to shift a by to the left.
 *
 * RETURNS
 *  The left-shifted value.
 */
LONGLONG WINAPI _allshl( LONGLONG a, LONG b )
{
    return a << b;
}

/******************************************************************************
 *        _allshr   (NTDLL.@)
 *
 * Shift a 64 bit integer to the right.
 *
 * PARAMS
 *  a [I] Initial number.
 *  b [I] Number to shift a by to the right.
 *
 * RETURNS
 *  The right-shifted value.
 */
LONGLONG WINAPI _allshr( LONGLONG a, LONG b )
{
    return a >> b;
}

/******************************************************************************
 *        _alldvrm   (NTDLL.@)
 *
 * Divide two 64 bit integers.
 *
 * PARAMS
 *  a [I] Initial number.
 *  b [I] Number to divide a by.
 *
 * RETURNS
 *  Returns the quotient of a and b in edx:eax.
 *  Returns the remainder of a and b in ebx:ecx.
 */
__ASM_STDCALL_FUNC( _alldvrm, 16,
                    "pushl %ebp\n\t"
                    __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                    __ASM_CFI(".cfi_rel_offset %ebp,0\n\t")
                    "movl %esp,%ebp\n\t"
                    __ASM_CFI(".cfi_def_cfa_register %ebp\n\t")
                    "pushl 20(%ebp)\n\t"
                    "pushl 16(%ebp)\n\t"
                    "pushl 12(%ebp)\n\t"
                    "pushl 8(%ebp)\n\t"
                    "call " __ASM_NAME("_allrem") "\n\t"
                    "movl %edx,%ebx\n\t"
                    "pushl %eax\n\t"
                    "pushl 20(%ebp)\n\t"
                    "pushl 16(%ebp)\n\t"
                    "pushl 12(%ebp)\n\t"
                    "pushl 8(%ebp)\n\t"
                    "call " __ASM_NAME("_alldiv") "\n\t"
                    "popl %ecx\n\t"
                    "leave\n\t"
                    __ASM_CFI(".cfi_def_cfa %esp,4\n\t")
                    __ASM_CFI(".cfi_same_value %ebp\n\t")
                    "ret $16" )

/******************************************************************************
 *        _aullrem   (NTDLL.@)
 *
 * Calculate the remainder after dividing two 64 bit unsigned integers.
 *
 * PARAMS
 *  a [I] Initial number.
 *  b [I] Number to divide a by.
 *
 * RETURNS
 *  The remainder of a divided by b.
 */
ULONGLONG WINAPI _aullrem( ULONGLONG a, ULONGLONG b )
{
    return a % b;
}

/******************************************************************************
 *        _aullshr   (NTDLL.@)
 *
 * Shift a 64 bit unsigned integer to the right.
 *
 * PARAMS
 *  a [I] Initial number.
 *  b [I] Number to shift a by to the right.
 *
 * RETURNS
 *  The right-shifted value.
 */
ULONGLONG WINAPI _aullshr( ULONGLONG a, LONG b )
{
    return a >> b;
}

/******************************************************************************
 *        _aulldvrm   (NTDLL.@)
 *
 * Divide two 64 bit unsigned integers.
 *
 * PARAMS
 *  a [I] Initial number.
 *  b [I] Number to divide a by.
 *
 * RETURNS
 *  Returns the quotient of a and b in edx:eax.
 *  Returns the remainder of a and b in ebx:ecx.
 */
__ASM_STDCALL_FUNC( _aulldvrm, 16,
                    "pushl %ebp\n\t"
                    __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                    __ASM_CFI(".cfi_rel_offset %ebp,0\n\t")
                    "movl %esp,%ebp\n\t"
                    __ASM_CFI(".cfi_def_cfa_register %ebp\n\t")
                    "pushl 20(%ebp)\n\t"
                    "pushl 16(%ebp)\n\t"
                    "pushl 12(%ebp)\n\t"
                    "pushl 8(%ebp)\n\t"
                    "call " __ASM_NAME("_aullrem") "\n\t"
                    "movl %edx,%ebx\n\t"
                    "pushl %eax\n\t"
                    "pushl 20(%ebp)\n\t"
                    "pushl 16(%ebp)\n\t"
                    "pushl 12(%ebp)\n\t"
                    "pushl 8(%ebp)\n\t"
                    "call " __ASM_NAME("_aulldiv") "\n\t"
                    "popl %ecx\n\t"
                    "leave\n\t"
                    __ASM_CFI(".cfi_def_cfa %esp,4\n\t")
                    __ASM_CFI(".cfi_same_value %ebp\n\t")
                    "ret $16" )

#endif  /* __i386__ */
