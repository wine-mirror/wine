/*
 * Large integer functions
 *
 * Copyright 2000 Alexandre Julliard
 */

#include "winnt.h"
#include "ntddk.h"

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
    UINT ret, rem;
    __asm__("div %4,%%eax"
            : "=a" (ret), "=d" (rem)
            : "0" (*(UINT*)&a), "1" (*((UINT*)&a+1)), "g" (b) );
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
 * This allows replacing a division by a longlong constant
 * by a multiplication by the inverse constant.
 *
 * If 'c' is the constant divisor, the constants 'b' and 'shift'
 * must be chosen such that b = 2^(64+shift) / c.
 * Then we have RtlExtendedMagicDivide(a,b,shift) == a * b / 2^(64+shift) == a / c.
 *
 * I'm too lazy to implement it right now...
 */
/* LONGLONG WINAPI RtlExtendedMagicDivide( LONGLONG a, LONGLONG b, INT shift )
 * {
 *     return 0;
 * }
 */


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
