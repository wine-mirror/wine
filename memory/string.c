/*
 * String functions
 *
 * Copyright 1993 Yngvi Sigurjonsson
 * Copyright 1996 Alexandre Julliard
 */

#include <ctype.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/winbase16.h"
#include "wine/winuser16.h"
#include "wine/keyboard16.h"
#include "wine/exception.h"
#include "wine/unicode.h"
#include "winerror.h"
#include "ldt.h"
#include "debugtools.h"
#include "winnls.h"

DEFAULT_DEBUG_CHANNEL(string);

/* Internaly used by strchr family functions */
static BOOL ChrCmpA( WORD word1, WORD word2);


/* filter for page-fault exceptions */
static WINE_EXCEPTION_FILTER(page_fault)
{
    if (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION)
        return EXCEPTION_EXECUTE_HANDLER;
    return EXCEPTION_CONTINUE_SEARCH;
}


/***********************************************************************
 *           hmemcpy   (KERNEL.348)
 */
void WINAPI hmemcpy16( LPVOID dst, LPCVOID src, LONG count )
{
    memcpy( dst, src, count );
}


/***********************************************************************
 *           lstrcat16   (KERNEL.89)
 */
SEGPTR WINAPI lstrcat16( SEGPTR dst, LPCSTR src )
{
    /* Windows does not check for NULL pointers here, so we don't either */
    strcat( (LPSTR)PTR_SEG_TO_LIN(dst), src );
    return dst;
}


/***********************************************************************
 *           lstrcatA   (KERNEL32.599)
 */
LPSTR WINAPI lstrcatA( LPSTR dst, LPCSTR src )
{
    __TRY
    {
        strcat( dst, src );
    }
    __EXCEPT(page_fault)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }
    __ENDTRY
    return dst;
}


/***********************************************************************
 *           lstrcatW   (KERNEL32.600)
 */
LPWSTR WINAPI lstrcatW( LPWSTR dst, LPCWSTR src )
{
    __TRY
    {
        strcatW( dst, src );
    }
    __EXCEPT(page_fault)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }
    __ENDTRY
    return dst;
}


/***********************************************************************
 *           lstrcatn16   (KERNEL.352)
 */
SEGPTR WINAPI lstrcatn16( SEGPTR dst, LPCSTR src, INT16 n )
{
    LPSTR p = (LPSTR)PTR_SEG_TO_LIN(dst);

    while (*p) p++;
    if ((n -= (p - (LPSTR)PTR_SEG_TO_LIN(dst))) <= 0) return dst;
    lstrcpynA( p, src, n );
    return dst;
}


/***********************************************************************
 *           lstrcmp16   (USER.430)
 */
INT16 WINAPI lstrcmp16( LPCSTR str1, LPCSTR str2 )
{
    return (INT16)strcmp( str1, str2 );
}


/***********************************************************************
 *           lstrcmpA   (KERNEL.602)
 */
INT WINAPI lstrcmpA( LPCSTR str1, LPCSTR str2 )
{
    return CompareStringA(LOCALE_SYSTEM_DEFAULT,0,str1,-1,str2,-1) - 2 ;
}


/***********************************************************************
 *           lstrcmpW   (KERNEL.603)
 * FIXME : should call CompareStringW, when it is implemented.
 *    This implementation is not "word sort", as it should.
 */
INT WINAPI lstrcmpW( LPCWSTR str1, LPCWSTR str2 )
{
    TRACE("%s and %s\n",
		   debugstr_w (str1), debugstr_w (str2));
    if (!str1 || !str2) {
    	SetLastError(ERROR_INVALID_PARAMETER);
	return 0;
    }
    while (*str1 && (*str1 == *str2)) { str1++; str2++; }
    return (INT)(*str1 - *str2);
}


/***********************************************************************
 *           lstrcmpi16   (USER.471)
 */
INT16 WINAPI lstrcmpi16( LPCSTR str1, LPCSTR str2 )
{
    return (INT16)lstrcmpiA( str1, str2 );
}


/***********************************************************************
 *           lstrcmpiA   (KERNEL32.605)
 */
INT WINAPI lstrcmpiA( LPCSTR str1, LPCSTR str2 )
{    TRACE("strcmpi %s and %s\n",
		   debugstr_a (str1), debugstr_a (str2));
    return CompareStringA(LOCALE_SYSTEM_DEFAULT,NORM_IGNORECASE,str1,-1,str2,-1)-2;
}


/***********************************************************************
 *           lstrcmpiW   (KERNEL32.606)
 */
INT WINAPI lstrcmpiW( LPCWSTR str1, LPCWSTR str2 )
{
    if (!str1 || !str2) {
    	SetLastError(ERROR_INVALID_PARAMETER);
	return 0;
    }
    return strcmpiW( str1, str2 );
}


/***********************************************************************
 *           lstrcpy16   (KERNEL.88)
 */
SEGPTR WINAPI lstrcpy16( SEGPTR dst, LPCSTR src )
{
    if (!lstrcpyA( PTR_SEG_TO_LIN(dst), src )) dst = 0;
    return dst;
}


/***********************************************************************
 *           lstrcpyA   (KERNEL32.608)
 */
LPSTR WINAPI lstrcpyA( LPSTR dst, LPCSTR src )
{
    __TRY
    {
        /* this is how Windows does it */
        memmove( dst, src, strlen(src)+1 );
    }
    __EXCEPT(page_fault)
    {
        ERR("(%p, %p): page fault occurred ! Caused by bug ?\n", dst, src);
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }
    __ENDTRY
    return dst;
}


/***********************************************************************
 *           lstrcpyW   (KERNEL32.609)
 */
LPWSTR WINAPI lstrcpyW( LPWSTR dst, LPCWSTR src )
{
    __TRY
    {
        strcpyW( dst, src );
    }
    __EXCEPT(page_fault)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }
    __ENDTRY
    return dst;
}


/***********************************************************************
 *           lstrcpyn16   (KERNEL.353)
 */
SEGPTR WINAPI lstrcpyn16( SEGPTR dst, LPCSTR src, INT16 n )
{
    lstrcpynA( (LPSTR)PTR_SEG_TO_LIN(dst), src, n );
    return dst;
}


/***********************************************************************
 *           lstrcpynA   (KERNEL32.611)
 * Note: this function differs from the UNIX strncpy, it _always_ writes
 * a terminating \0
 */
LPSTR WINAPI lstrcpynA( LPSTR dst, LPCSTR src, INT n )
{
    LPSTR p = dst;
    TRACE("(%p, %s, %i)\n", dst, debugstr_an(src,n), n);
    /* In real windows the whole function is protected by an exception handler
     * that returns ERROR_INVALID_PARAMETER on faulty parameters
     * We currently just check for NULL.
     */
    if (!dst || !src) {
    	SetLastError(ERROR_INVALID_PARAMETER);
	return 0;
    }
    while ((n-- > 1) && *src) *p++ = *src++;
    if (n >= 0) *p = 0;
    return dst;
}


/***********************************************************************
 *           lstrcpynW   (KERNEL32.612)
 * Note: this function differs from the UNIX strncpy, it _always_ writes
 * a terminating \0
 */
LPWSTR WINAPI lstrcpynW( LPWSTR dst, LPCWSTR src, INT n )
{
    LPWSTR p = dst;
    TRACE("(%p, %s, %i)\n", dst,  debugstr_wn(src,n), n);
    /* In real windows the whole function is protected by an exception handler
     * that returns ERROR_INVALID_PARAMETER on faulty parameters
     * We currently just check for NULL.
     */
    if (!dst || !src) {
    	SetLastError(ERROR_INVALID_PARAMETER);
	return 0;
    }
    while ((n-- > 1) && *src) *p++ = *src++;
    if (n >= 0) *p = 0;
    return dst;
}


/***********************************************************************
 *           lstrlen16   (KERNEL.90)
 */
INT16 WINAPI lstrlen16( LPCSTR str )
{
    return (INT16)lstrlenA( str );
}


/***********************************************************************
 *           lstrlenA   (KERNEL32.614)
 */
INT WINAPI lstrlenA( LPCSTR str )
{
    INT ret;
    __TRY
    {
        ret = strlen(str);
    }
    __EXCEPT(page_fault)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    __ENDTRY
    return ret;
}


/***********************************************************************
 *           lstrlenW   (KERNEL32.615)
 */
INT WINAPI lstrlenW( LPCWSTR str )
{
    INT ret;
    __TRY
    {
        ret = strlenW(str);
    }
    __EXCEPT(page_fault)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    __ENDTRY
    return ret;
}


/***********************************************************************
 *           lstrcpyAtoW   (Not a Windows API)
 */
LPWSTR WINAPI lstrcpyAtoW( LPWSTR dst, LPCSTR src )
{
    MultiByteToWideChar( CP_ACP, 0, src, -1, dst, 0x7fffffff );
    return dst;
}


/***********************************************************************
 *           lstrcpyWtoA   (Not a Windows API)
 */
LPSTR WINAPI lstrcpyWtoA( LPSTR dst, LPCWSTR src )
{
    WideCharToMultiByte( CP_ACP, 0, src, -1, dst, 0x7fffffff, NULL, NULL );
    return dst;
}


/***********************************************************************
 *           lstrcpynAtoW   (Not a Windows API)
 * Note: this function differs from the UNIX strncpy, it _always_ writes
 * a terminating \0
 */
LPWSTR WINAPI lstrcpynAtoW( LPWSTR dst, LPCSTR src, INT n )
{
    if (n > 0 && !MultiByteToWideChar( CP_ACP, 0, src, -1, dst, n )) dst[n-1] = 0;
    return dst;
}


/***********************************************************************
 *           lstrcpynWtoA   (Not a Windows API)
 * Note: this function differs from the UNIX strncpy, it _always_ writes
 * a terminating \0
 *
 * The terminating zero should be written at the end of the string, not
 * the end of the buffer, as some programs specify the wrong size for 
 * the buffer (eg. winnt's sol.exe)
 */
LPSTR WINAPI lstrcpynWtoA( LPSTR dst, LPCWSTR src, INT n )
{
    if (n > 0 && !WideCharToMultiByte( CP_ACP, 0, src, -1, dst, n, NULL, NULL )) dst[n-1] = 0;
    return dst;
}

/***********************************************************************
 *           UnicodeToAnsi   (KERNEL.434)
 */
INT16 WINAPI UnicodeToAnsi16( LPCWSTR src, LPSTR dst, INT16 codepage )
{
    if ( codepage == -1 )
	codepage = CP_ACP;

    return WideCharToMultiByte( codepage, 0, src, -1, dst, 0x7fffffff, NULL, NULL );
}


/***********************************************************************
 *           Copy   (GDI.250)
 */
void WINAPI Copy16( LPVOID src, LPVOID dst, WORD size )
{
    memcpy( dst, src, size );
}

/***********************************************************************
 *           AnsiToOem16   (KEYBOARD.5)
 */
INT16 WINAPI AnsiToOem16( LPCSTR s, LPSTR d )
{
    CharToOemA( s, d );
    return -1;
}


/***********************************************************************
 *           OemToAnsi16   (KEYBOARD.6)
 */
INT16 WINAPI OemToAnsi16( LPCSTR s, LPSTR d )
{
    OemToCharA( s, d );
    return -1;
}


/***********************************************************************
 *           AnsiToOemBuff16   (KEYBOARD.134)
 */
void WINAPI AnsiToOemBuff16( LPCSTR s, LPSTR d, UINT16 len )
{
    if (len != 0) CharToOemBuffA( s, d, len );
}


/***********************************************************************
 *           OemToAnsiBuff16   (KEYBOARD.135)
 */
void WINAPI OemToAnsiBuff16( LPCSTR s, LPSTR d, UINT16 len )
{
    if (len != 0) OemToCharBuffA( s, d, len );
}


/***********************************************************************
 *           CharToOemA   (USER32.37)
 */
BOOL WINAPI CharToOemA( LPCSTR s, LPSTR d )
{
    return CharToOemBuffA( s, d, strlen( s ) + 1 );
}


/***********************************************************************
 *           CharToOemBuffA   (USER32.38)
 */
BOOL WINAPI CharToOemBuffA( LPCSTR s, LPSTR d, DWORD len )
{
    WCHAR *bufW;

    bufW = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
    if( bufW )
    {
	MultiByteToWideChar( CP_ACP, 0, s, len, bufW, len );
	WideCharToMultiByte( CP_OEMCP, 0, bufW, len, d, len, NULL, NULL );
	HeapFree( GetProcessHeap(), 0, bufW );
    }
    return TRUE;
}


/***********************************************************************
 *           CharToOemBuffW   (USER32.39)
 */
BOOL WINAPI CharToOemBuffW( LPCWSTR s, LPSTR d, DWORD len )
{
    WideCharToMultiByte( CP_OEMCP, 0, s, len, d, len, NULL, NULL );
    return TRUE;
}


/***********************************************************************
 *           CharToOemW   (USER32.40)
 */
BOOL WINAPI CharToOemW( LPCWSTR s, LPSTR d )
{
    return CharToOemBuffW( s, d, strlenW( s ) + 1 );
}


/***********************************************************************
 *           OemToCharA   (USER32.402)
 */
BOOL WINAPI OemToCharA( LPCSTR s, LPSTR d )
{
    return OemToCharBuffA( s, d, strlen( s ) + 1 );
}


/***********************************************************************
 *           OemToCharBuffA   (USER32.403)
 */
BOOL WINAPI OemToCharBuffA( LPCSTR s, LPSTR d, DWORD len )
{
    WCHAR *bufW;

    bufW = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
    if( bufW )
    {
	MultiByteToWideChar( CP_OEMCP, 0, s, len, bufW, len );
	WideCharToMultiByte( CP_ACP, 0, bufW, len, d, len, NULL, NULL );
	HeapFree( GetProcessHeap(), 0, bufW );
    }
    return TRUE;
}


/***********************************************************************
 *           OemToCharBuffW   (USER32.404)
 */
BOOL WINAPI OemToCharBuffW( LPCSTR s, LPWSTR d, DWORD len )
{
    MultiByteToWideChar( CP_OEMCP, 0, s, len, d, len );
    return TRUE;
}


/***********************************************************************
 *           OemToCharW   (USER32.405)
 */
BOOL WINAPI OemToCharW( LPCSTR s, LPWSTR d )
{
    return OemToCharBuffW( s, d, strlen( s ) + 1 );
}

/***********************************************************************
 *           lstrrchr   (Not a Windows API)
 *
 * This is the implementation meant to be invoked from within
 * COMCTL32_StrRChrA and shell32(TODO)...
 *
 * Return a pointer to the last occurence of wMatch in lpStart
 * not looking further than lpEnd...
 */
LPSTR WINAPI lstrrchr( LPCSTR lpStart, LPCSTR lpEnd, WORD wMatch )
{
  LPCSTR lpGotIt = NULL;

  TRACE("(%p, %p, %x)\n", lpStart, lpEnd, wMatch);

  if (!lpEnd) lpEnd = lpStart + strlen(lpStart);

  for(; lpStart < lpEnd; lpStart = CharNextA(lpStart)) 
    if (!ChrCmpA( GET_WORD(lpStart), wMatch)) 
      lpGotIt = lpStart;
    
  return ((LPSTR)lpGotIt);
}

/***********************************************************************
 *           ChrCmpW   
 * This fuction returns FALSE if both words match, TRUE otherwise...
 */
static BOOL ChrCmpW( WORD word1, WORD word2) {
  return (word1 != word2);
}

/***********************************************************************
 *           lstrrchrw    (Not a Windows API)
 *
 * This is the implementation meant to be invoked form within
 * COMCTL32_StrRChrW and shell32(TODO)...
 *
 * Return a pointer to the last occurence of wMatch in lpStart
 * not looking further than lpEnd...
 */  
LPWSTR WINAPI lstrrchrw( LPCWSTR lpStart, LPCWSTR lpEnd, WORD wMatch )
{
  LPCWSTR lpGotIt = NULL;

  TRACE("(%p, %p, %x)\n", lpStart, lpEnd, wMatch);
  if (!lpEnd) lpEnd = lpStart + lstrlenW(lpStart);

  for(; lpStart < lpEnd; lpStart = CharNextW(lpStart)) 
    if (!ChrCmpW( GET_WORD(lpStart), wMatch)) 
      lpGotIt = lpStart;
    
  return (LPWSTR)lpGotIt;
}

/***********************************************************************
 *           ChrCmpA   
 * This fuction returns FALSE if both words match, TRUE otherwise...
 */
static BOOL ChrCmpA( WORD word1, WORD word2) {
  if (LOBYTE(word1) == LOBYTE(word2)) {
    if (IsDBCSLeadByte(LOBYTE(word1))) {
      return (word1 != word2);
    }
    return FALSE;
  }
  return TRUE;
}
