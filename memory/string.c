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
#include "winerror.h"
#include "crtdll.h"
#include "ldt.h"
#include "debugtools.h"
#include "winnls.h"

DEFAULT_DEBUG_CHANNEL(string)

static const BYTE STRING_Oem2Ansi[256] =
"\000\001\002\003\004\005\006\007\010\011\012\013\014\015\016\244"
"\020\021\022\023\266\247\026\027\030\031\032\033\034\035\036\037"
"\040\041\042\043\044\045\046\047\050\051\052\053\054\055\056\057"
"\060\061\062\063\064\065\066\067\070\071\072\073\074\075\076\077"
"\100\101\102\103\104\105\106\107\110\111\112\113\114\115\116\117"
"\120\121\122\123\124\125\126\127\130\131\132\133\134\135\136\137"
"\140\141\142\143\144\145\146\147\150\151\152\153\154\155\156\157"
"\160\161\162\163\164\165\166\167\170\171\172\173\174\175\176\177"
"\307\374\351\342\344\340\345\347\352\353\350\357\356\354\304\305"
"\311\346\306\364\366\362\373\371\377\326\334\242\243\245\120\203"
"\341\355\363\372\361\321\252\272\277\137\254\275\274\241\253\273"
"\137\137\137\246\246\246\246\053\053\246\246\053\053\053\053\053"
"\053\055\055\053\055\053\246\246\053\053\055\055\246\055\053\055"
"\055\055\055\053\053\053\053\053\053\053\053\137\137\246\137\137"
"\137\337\137\266\137\137\265\137\137\137\137\137\137\137\137\137"
"\137\261\137\137\137\137\367\137\260\225\267\137\156\262\137\137";

static const BYTE STRING_Ansi2Oem[256] =
"\000\001\002\003\004\005\006\007\010\011\012\013\014\015\016\017"
"\020\021\022\023\024\025\026\027\030\031\032\033\034\035\036\037"
"\040\041\042\043\044\045\046\047\050\051\052\053\054\055\056\057"
"\060\061\062\063\064\065\066\067\070\071\072\073\074\075\076\077"
"\100\101\102\103\104\105\106\107\110\111\112\113\114\115\116\117"
"\120\121\122\123\124\125\126\127\130\131\132\133\134\135\136\137"
"\140\141\142\143\144\145\146\147\150\151\152\153\154\155\156\157"
"\160\161\162\163\164\165\166\167\170\171\172\173\174\175\176\177"
"\200\201\054\237\054\137\375\374\210\045\123\074\117\215\216\217"
"\220\140\047\042\042\371\055\137\230\231\163\076\157\235\236\131"
"\040\255\233\234\017\235\335\025\042\143\246\256\252\055\162\137"
"\370\361\375\063\047\346\024\372\054\061\247\257\254\253\137\250"
"\101\101\101\101\216\217\222\200\105\220\105\105\111\111\111\111"
"\104\245\117\117\117\117\231\170\117\125\125\125\232\131\137\341"
"\205\240\203\141\204\206\221\207\212\202\210\211\215\241\214\213"
"\144\244\225\242\223\157\224\366\157\227\243\226\201\171\137\230";

#define OEM_TO_ANSI(ch) (STRING_Oem2Ansi[(unsigned char)(ch)])
#define ANSI_TO_OEM(ch) (STRING_Ansi2Oem[(unsigned char)(ch)])

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
        CRTDLL_wcscat( dst, src );
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
 *           lstrcmp32A   (KERNEL.602)
 */
INT WINAPI lstrcmpA( LPCSTR str1, LPCSTR str2 )
{
    return CompareStringA(LOCALE_SYSTEM_DEFAULT,0,str1,-1,str2,-1) - 2 ;
}


/***********************************************************************
 *           lstrcmp32W   (KERNEL.603)
 * FIXME : should call CompareString32W, when it is implemented.
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
 *           lstrcmpi32A   (KERNEL32.605)
 */
INT WINAPI lstrcmpiA( LPCSTR str1, LPCSTR str2 )
{    TRACE("strcmpi %s and %s\n",
		   debugstr_a (str1), debugstr_a (str2));
    return CompareStringA(LOCALE_SYSTEM_DEFAULT,NORM_IGNORECASE,str1,-1,str2,-1)-2;
}


/***********************************************************************
 *           lstrcmpi32W   (KERNEL32.606)
 */
INT WINAPI lstrcmpiW( LPCWSTR str1, LPCWSTR str2 )
{
    INT res;

#if 0
    /* Too much!  (From registry loading.)  */
    TRACE("strcmpi %s and %s\n",
		   debugstr_w (str1), debugstr_w (str2));
#endif
    if (!str1 || !str2) {
    	SetLastError(ERROR_INVALID_PARAMETER);
	return 0;
    }
    while (*str1)
    {
        if ((*str1<0x100 ) && (*str2<0x100)) {
	    if ((res = toupper(*str1) - toupper(*str2)) != 0) return res;
	} else {
	    if ((res = towupper(*str1) - towupper(*str2)) != 0) return res;
	}
        str1++;
        str2++;
    }
    return towupper(*str1) - towupper(*str2);
}


/***********************************************************************
 *           lstrcpy16   (KERNEL.88)
 */
SEGPTR WINAPI lstrcpy16( SEGPTR dst, LPCSTR src )
{
    /* this is how Windows does it */
    memmove( (LPSTR)PTR_SEG_TO_LIN(dst), src, strlen(src)+1 );
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
        CRTDLL_wcscpy( dst, src );
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
 *           lstrcpyn32A   (KERNEL32.611)
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
 *           lstrcpyn32W   (KERNEL32.612)
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
        ret = CRTDLL_wcslen(str);
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
    register LPWSTR p = dst;

    TRACE("(%p, %s)\n", dst, debugstr_a(src));

    while ((*p++ = (WCHAR)(unsigned char)*src++));
    return dst;
}


/***********************************************************************
 *           lstrcpyWtoA   (Not a Windows API)
 */
LPSTR WINAPI lstrcpyWtoA( LPSTR dst, LPCWSTR src )
{
    register LPSTR p = dst;

    TRACE("(%p, %s)\n", dst, debugstr_w(src));

    while ((*p++ = (CHAR)*src++));
    return dst;
}


/***********************************************************************
 *           lstrcpynAtoW   (Not a Windows API)
 * Note: this function differs from the UNIX strncpy, it _always_ writes
 * a terminating \0
 */
LPWSTR WINAPI lstrcpynAtoW( LPWSTR dst, LPCSTR src, INT n )
{
    LPWSTR p = dst;

    TRACE("(%p, %s, %i)\n", dst, debugstr_an(src,n), n);

    while ((n-- > 1) && *src) *p++ = (WCHAR)(unsigned char)*src++;
    if (n >= 0) *p = 0;
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
    if (--n >= 0)
    {
        TRACE("(%p, %s, %i)\n", dst, debugstr_wn(src,n), n);
        n = CRTDLL_wcstombs( dst, src, n );
        if(n<0)
                 n=0;
        dst[n] = 0;
    }
    return dst;
}

/***********************************************************************
 *           UnicodeToAnsi   (KERNEL.434)
 */
INT16 WINAPI UnicodeToAnsi16( LPCWSTR src, LPSTR dst, INT16 codepage )
{
    if ( codepage != -1 )
        FIXME("codepage %d not supported\n", codepage );

    lstrcpyWtoA( dst, src );

    return (INT16)lstrlenA( dst );
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
 *           CharToOem32A   (USER32.37)
 */
BOOL WINAPI CharToOemA( LPCSTR s, LPSTR d )
{
    LPSTR oldd = d;
    if (!s || !d) return TRUE;
    TRACE("CharToOem %s\n", debugstr_a (s));
    while ((*d++ = ANSI_TO_OEM(*s++)));
    TRACE("       to %s\n", debugstr_a (oldd));
    return TRUE;
}


/***********************************************************************
 *           CharToOemBuff32A   (USER32.38)
 */
BOOL WINAPI CharToOemBuffA( LPCSTR s, LPSTR d, DWORD len )
{
    while (len--) *d++ = ANSI_TO_OEM(*s++);
    return TRUE;
}


/***********************************************************************
 *           CharToOemBuff32W   (USER32.39)
 */
BOOL WINAPI CharToOemBuffW( LPCWSTR s, LPSTR d, DWORD len )
{
    while (len--) *d++ = ANSI_TO_OEM(*s++);
    return TRUE;
}


/***********************************************************************
 *           CharToOem32W   (USER32.40)
 */
BOOL WINAPI CharToOemW( LPCWSTR s, LPSTR d )
{
    LPSTR oldd = d;
    if (!s || !d) return TRUE;
    TRACE("CharToOem %s\n", debugstr_w (s));
    while ((*d++ = ANSI_TO_OEM(*s++)));
    TRACE("       to %s\n", debugstr_a (oldd));
    return TRUE;
}


/***********************************************************************
 *           OemToChar32A   (USER32.402)
 */
BOOL WINAPI OemToCharA( LPCSTR s, LPSTR d )
{
    LPSTR oldd = d;
    TRACE("OemToChar %s\n", debugstr_a (s));
    while ((*d++ = OEM_TO_ANSI(*s++)));
    TRACE("       to %s\n", debugstr_a (oldd));
    return TRUE;
}


/***********************************************************************
 *           OemToCharBuff32A   (USER32.403)
 */
BOOL WINAPI OemToCharBuffA( LPCSTR s, LPSTR d, DWORD len )
{
    TRACE("OemToCharBuff %s\n", debugstr_an (s, len));
    while (len--) *d++ = OEM_TO_ANSI(*s++);
    return TRUE;
}


/***********************************************************************
 *           OemToCharBuff32W   (USER32.404)
 */
BOOL WINAPI OemToCharBuffW( LPCSTR s, LPWSTR d, DWORD len )
{
    TRACE("OemToCharBuff %s\n", debugstr_an (s, len));
    while (len--) *d++ = (WCHAR)OEM_TO_ANSI(*s++);
    return TRUE;
}


/***********************************************************************
 *           OemToChar32W   (USER32.405)
 */
BOOL WINAPI OemToCharW( LPCSTR s, LPWSTR d )
{
    while ((*d++ = (WCHAR)OEM_TO_ANSI(*s++)));
    return TRUE;
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
