/*
 * String functions
 *
 * Copyright 1993 Yngvi Sigurjonsson
 * Copyright 1996 Alexandre Julliard
 */

#include <ctype.h>
#include <string.h>
#include "windows.h"
#include "ldt.h"


/***********************************************************************
 *           hmemcpy   (KERNEL.348)
 */
void hmemcpy( LPVOID dst, LPCVOID src, LONG count )
{
    memcpy( dst, src, count );
}


/***********************************************************************
 *           lstrcat16   (KERNEL.89)
 */
SEGPTR lstrcat16( SEGPTR dst, SEGPTR src )
{
    lstrcat32A( (LPSTR)PTR_SEG_TO_LIN(dst), (LPCSTR)PTR_SEG_TO_LIN(src) );
    return dst;
}


/***********************************************************************
 *           lstrcat32A   (KERNEL32.599)
 */
LPSTR lstrcat32A( LPSTR dst, LPCSTR src )
{
    strcat( dst, src );
    return dst;
}


/***********************************************************************
 *           lstrcat32W   (KERNEL32.600)
 */
LPWSTR lstrcat32W( LPWSTR dst, LPCWSTR src )
{
    register LPWSTR p = dst;
    while (*p) p++;
    while ((*p++ = *src++));
    return dst;
}


/***********************************************************************
 *           lstrcatn16   (KERNEL.352)
 */
SEGPTR lstrcatn16( SEGPTR dst, SEGPTR src, INT16 n )
{
    lstrcatn32A( (LPSTR)PTR_SEG_TO_LIN(dst), (LPCSTR)PTR_SEG_TO_LIN(src), n );
    return dst;
}


/***********************************************************************
 *           lstrcatn32A   (Not a Windows API)
 */
LPSTR lstrcatn32A( LPSTR dst, LPCSTR src, INT32 n )
{
    register LPSTR p = dst;
    while (*p) p++;
    if ((n -= (INT32)(p - dst)) <= 0) return dst;
    lstrcpyn32A( p, src, n );
    return dst;
}


/***********************************************************************
 *           lstrcatn32W   (Not a Windows API)
 */
LPWSTR lstrcatn32W( LPWSTR dst, LPCWSTR src, INT32 n )
{
    register LPWSTR p = dst;
    while (*p) p++;
    if ((n -= (INT32)(p - dst)) <= 0) return dst;
    lstrcpyn32W( p, src, n );
    return dst;
}


/***********************************************************************
 *           lstrcmp16   (USER.430)
 */
INT16 lstrcmp16( LPCSTR str1, LPCSTR str2 )
{
    return (INT16)lstrcmp32A( str1, str2 );
}


/***********************************************************************
 *           lstrcmp32A   (KERNEL.602)
 */
INT32 lstrcmp32A( LPCSTR str1, LPCSTR str2 )
{
    return (INT32)strcmp( str1, str2 );
}


/***********************************************************************
 *           lstrcmp32W   (KERNEL.603)
 */
INT32 lstrcmp32W( LPCWSTR str1, LPCWSTR str2 )
{
    while (*str1 && (*str1 == *str2)) { str1++; str2++; }
    return (INT32)(*str1 - *str2);
}


/***********************************************************************
 *           lstrcmpi16   (USER.471)
 */
INT16 lstrcmpi16( LPCSTR str1, LPCSTR str2 )
{
    return (INT16)lstrcmpi32A( str1, str2 );
}


/***********************************************************************
 *           lstrcmpi32A   (KERNEL32.605)
 */
INT32 lstrcmpi32A( LPCSTR str1, LPCSTR str2 )
{
    INT32 res;

    while (*str1)
    {
        if ((res = toupper(*str1) - toupper(*str2)) != 0) return res;
        str1++;
        str2++;
    }
    return toupper(*str1) - toupper(*str2);
}


/***********************************************************************
 *           lstrcmpi32W   (KERNEL32.606)
 */
INT32 lstrcmpi32W( LPCWSTR str1, LPCWSTR str2 )
{
    INT32 res;

    while (*str1)
    {
        /* FIXME: Unicode */
        if ((res = toupper(*str1) - toupper(*str2)) != 0) return res;
        str1++;
        str2++;
    }
    return toupper(*str1) - toupper(*str2);
}


/***********************************************************************
 *           lstrcpy16   (KERNEL.88)
 */
SEGPTR lstrcpy16( SEGPTR dst, SEGPTR src )
{
    lstrcpy32A( (LPSTR)PTR_SEG_TO_LIN(dst), (LPCSTR)PTR_SEG_TO_LIN(src) );
    return dst;
}


/***********************************************************************
 *           lstrcpy32A   (KERNEL32.608)
 */
LPSTR lstrcpy32A( LPSTR dst, LPCSTR src )
{
    strcpy( dst, src );
    return dst;
}


/***********************************************************************
 *           lstrcpy32W   (KERNEL32.609)
 */
LPWSTR lstrcpy32W( LPWSTR dst, LPCWSTR src )
{
    register LPWSTR p = dst;
    while ((*p++ = *src++));
    return dst;
}


/***********************************************************************
 *           lstrcpyn16   (KERNEL.353)
 */
SEGPTR lstrcpyn16( SEGPTR dst, SEGPTR src, INT16 n )
{
    lstrcpyn32A( (LPSTR)PTR_SEG_TO_LIN(dst), (LPCSTR)PTR_SEG_TO_LIN(src), n );
    return dst;
}


/***********************************************************************
 *           lstrcpyn32A   (KERNEL32.611)
 */
LPSTR lstrcpyn32A( LPSTR dst, LPCSTR src, INT32 n )
{
    LPSTR p = dst;
    while ((n-- > 1) && *src) *p++ = *src++;
    *p = 0;
    return dst;
}


/***********************************************************************
 *           lstrcpyn32W   (KERNEL32.612)
 */
LPWSTR lstrcpyn32W( LPWSTR dst, LPCWSTR src, INT32 n )
{
    LPWSTR p = dst;
    while ((n-- > 1) && *src) *p++ = *src++;
    *p = 0;
    return dst;
}


/***********************************************************************
 *           lstrlen16   (KERNEL.90)
 */
INT16 lstrlen16( LPCSTR str )
{
    return (INT16)lstrlen32A( str );
}


/***********************************************************************
 *           lstrlen32A   (KERNEL32.614)
 */
INT32 lstrlen32A( LPCSTR str )
{
    /* looks weird, but win3.1 KERNEL got a GeneralProtection handler
     * in lstrlen() ... we check only for NULL pointer reference.
     * - Marcus Meissner
     */
    if (!str) return 0;
    return (INT32)strlen(str);
}


/***********************************************************************
 *           lstrlen32W   (KERNEL32.615)
 */
INT32 lstrlen32W( LPCWSTR str )
{
    INT32 len = 0;
    if (!str) return 0;
    while (*str++) len++;
    return len;
}


/***********************************************************************
 *           lstrncmp16   (Not a Windows API)
 */
INT16 lstrncmp16( LPCSTR str1, LPCSTR str2, INT16 n )
{
    return (INT16)lstrncmp32A( str1, str2, n );
}


/***********************************************************************
 *           lstrncmp32A   (Not a Windows API)
 */
INT32 lstrncmp32A( LPCSTR str1, LPCSTR str2, INT32 n )
{
    return (INT32)strncmp( str1, str2, n );
}


/***********************************************************************
 *           lstrncmp32W   (Not a Windows API)
 */
INT32 lstrncmp32W( LPCWSTR str1, LPCWSTR str2, INT32 n )
{
    if (!n) return 0;
    while ((--n > 0) && *str1 && (*str1 == *str2)) { str1++; str2++; }
    return (INT32)(*str1 - *str2);
}


/***********************************************************************
 *           lstrncmpi16   (Not a Windows API)
 */
INT16 lstrncmpi16( LPCSTR str1, LPCSTR str2, INT16 n )
{
    return (INT16)lstrncmpi32A( str1, str2, n );
}


/***********************************************************************
 *           lstrncmpi32A   (Not a Windows API)
 */
INT32 lstrncmpi32A( LPCSTR str1, LPCSTR str2, INT32 n )
{
    INT32 res;

    if (!n) return 0;
    while ((--n > 0) && *str1)
    {
        if ((res = toupper(*str1) - toupper(*str2)) != 0) return res;
        str1++;
        str2++;
    }
    return toupper(*str1) - toupper(*str2);
}


/***********************************************************************
 *           lstrncmpi32W   (Not a Windows API)
 */
INT32 lstrncmpi32W( LPCWSTR str1, LPCWSTR str2, INT32 n )
{
    INT32 res;

    if (!n) return 0;
    while ((--n > 0) && *str1)
    {
        /* FIXME: Unicode */
        if ((res = toupper(*str1) - toupper(*str2)) != 0) return res;
        str1++;
        str2++;
    }
    return toupper(*str1) - toupper(*str2);
}
