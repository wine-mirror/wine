/*
 * String functions
 *
 * Copyright 1993 Yngvi Sigurjonsson
 * Copyright 1996 Alexandre Julliard
 */

#include <ctype.h>
#include <string.h>
#include "winbase.h"
#include "winuser.h"
#include "winerror.h"
#include "ldt.h"
#include "debug.h"
#include "winnls.h"

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
static BOOL32 ChrCmpA( WORD word1, WORD word2);
static BOOL32 ChrCmpW( WORD word1, WORD word2);

extern LPWSTR __cdecl CRTDLL_wcschr(LPCWSTR str,WCHAR xchar);


/***********************************************************************
 *           hmemcpy   (KERNEL.348)
 */
void WINAPI hmemcpy( LPVOID dst, LPCVOID src, LONG count )
{
    memcpy( dst, src, count );
}


/***********************************************************************
 *           lstrcat16   (KERNEL.89)
 */
SEGPTR WINAPI lstrcat16( SEGPTR dst, LPCSTR src )
{
    lstrcat32A( (LPSTR)PTR_SEG_TO_LIN(dst), src );
    return dst;
}


/***********************************************************************
 *           lstrcat32A   (KERNEL32.599)
 */
LPSTR WINAPI lstrcat32A( LPSTR dst, LPCSTR src )
{
    TRACE(string,"Append %s to %s\n",
		   debugstr_a (src), debugstr_a (dst));
    /* Windows does not check for NULL pointers here, so we don't either */
    strcat( dst, src );
    return dst;
}


/***********************************************************************
 *           lstrcat32W   (KERNEL32.600)
 */
LPWSTR WINAPI lstrcat32W( LPWSTR dst, LPCWSTR src )
{
    register LPWSTR p = dst;
    TRACE(string,"Append L%s to L%s\n",
		   debugstr_w (src), debugstr_w (dst));
    /* Windows does not check for NULL pointers here, so we don't either */
    while (*p) p++;
    while ((*p++ = *src++));
    return dst;
}


/***********************************************************************
 *           lstrcatn16   (KERNEL.352)
 */
SEGPTR WINAPI lstrcatn16( SEGPTR dst, LPCSTR src, INT16 n )
{
    lstrcatn32A( (LPSTR)PTR_SEG_TO_LIN(dst), src, n );
    return dst;
}


/***********************************************************************
 *           lstrcatn32A   (Not a Windows API)
 */
LPSTR WINAPI lstrcatn32A( LPSTR dst, LPCSTR src, INT32 n )
{
    register LPSTR p = dst;
    TRACE(string,"strcatn add %d chars from %s to %s\n",
		   n, debugstr_an (src, n), debugstr_a (dst));
    while (*p) p++;
    if ((n -= (INT32)(p - dst)) <= 0) return dst;
    lstrcpyn32A( p, src, n );
    return dst;
}


/***********************************************************************
 *           lstrcatn32W   (Not a Windows API)
 */
LPWSTR WINAPI lstrcatn32W( LPWSTR dst, LPCWSTR src, INT32 n )
{
    register LPWSTR p = dst;
    TRACE(string,"strcatn add %d chars from L%s to L%s\n",
		   n, debugstr_wn (src, n), debugstr_w (dst));
    while (*p) p++;
    if ((n -= (INT32)(p - dst)) <= 0) return dst;
    lstrcpyn32W( p, src, n );
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
INT32 WINAPI lstrcmp32A( LPCSTR str1, LPCSTR str2 )
{
    return CompareString32A(LOCALE_SYSTEM_DEFAULT,0,str1,-1,str2,-1) - 2 ;
}


/***********************************************************************
 *           lstrcmp32W   (KERNEL.603)
 * FIXME : should call CompareString32W, when it is implemented.
 *    This implementation is not "word sort", as it should.
 */
INT32 WINAPI lstrcmp32W( LPCWSTR str1, LPCWSTR str2 )
{
    TRACE(string,"L%s and L%s\n",
		   debugstr_w (str1), debugstr_w (str2));
    if (!str1 || !str2) {
    	SetLastError(ERROR_INVALID_PARAMETER);
	return 0;
    }
    while (*str1 && (*str1 == *str2)) { str1++; str2++; }
    return (INT32)(*str1 - *str2);
}


/***********************************************************************
 *           lstrcmpi16   (USER.471)
 */
INT16 WINAPI lstrcmpi16( LPCSTR str1, LPCSTR str2 )
{
    return (INT16)lstrcmpi32A( str1, str2 );
}


/***********************************************************************
 *           lstrcmpi32A   (KERNEL32.605)
 */
INT32 WINAPI lstrcmpi32A( LPCSTR str1, LPCSTR str2 )
{    TRACE(string,"strcmpi %s and %s\n",
		   debugstr_a (str1), debugstr_a (str2));
    return CompareString32A(LOCALE_SYSTEM_DEFAULT,NORM_IGNORECASE,str1,-1,str2,-1)-2;
}


/***********************************************************************
 *           lstrcmpi32W   (KERNEL32.606)
 */
INT32 WINAPI lstrcmpi32W( LPCWSTR str1, LPCWSTR str2 )
{
    INT32 res;

#if 0
    /* Too much!  (From registry loading.)  */
    TRACE(string,"strcmpi L%s and L%s\n",
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
    lstrcpy32A( (LPSTR)PTR_SEG_TO_LIN(dst), src );
    return dst;
}


/***********************************************************************
 *           lstrcpy32A   (KERNEL32.608)
 */
LPSTR WINAPI lstrcpy32A( LPSTR dst, LPCSTR src )
{
    TRACE(string,"strcpy %s\n", debugstr_a (src));
    /* In real windows the whole function is protected by an exception handler
     * that returns ERROR_INVALID_PARAMETER on faulty parameters
     * We currently just check for NULL.
     */
    if (!dst || !src) {
    	SetLastError(ERROR_INVALID_PARAMETER);
	return 0;
    }
    strcpy( dst, src );
    return dst;
}


/***********************************************************************
 *           lstrcpy32W   (KERNEL32.609)
 */
LPWSTR WINAPI lstrcpy32W( LPWSTR dst, LPCWSTR src )
{
    register LPWSTR p = dst;
    TRACE(string,"strcpy L%s\n", debugstr_w (src));
    /* In real windows the whole function is protected by an exception handler
     * that returns ERROR_INVALID_PARAMETER on faulty parameters
     * We currently just check for NULL.
     */
    if (!dst || !src) {
    	SetLastError(ERROR_INVALID_PARAMETER);
	return 0;
    }
    while ((*p++ = *src++));
    return dst;
}


/***********************************************************************
 *           lstrcpyn16   (KERNEL.353)
 */
SEGPTR WINAPI lstrcpyn16( SEGPTR dst, LPCSTR src, INT16 n )
{
    lstrcpyn32A( (LPSTR)PTR_SEG_TO_LIN(dst), src, n );
    return dst;
}


/***********************************************************************
 *           lstrcpyn32A   (KERNEL32.611)
 * Note: this function differs from the UNIX strncpy, it _always_ writes
 * a terminating \0
 */
LPSTR WINAPI lstrcpyn32A( LPSTR dst, LPCSTR src, INT32 n )
{
    LPSTR p = dst;
    TRACE(string,"strcpyn %s for %d chars\n",
		   debugstr_an (src,n), n);
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
LPWSTR WINAPI lstrcpyn32W( LPWSTR dst, LPCWSTR src, INT32 n )
{
    LPWSTR p = dst;
    TRACE(string,"strcpyn L%s for %d chars\n",
		   debugstr_wn (src,n), n);
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
    return (INT16)lstrlen32A( str );
}


/***********************************************************************
 *           lstrlen32A   (KERNEL32.614)
 */
INT32 WINAPI lstrlen32A( LPCSTR str )
{
    /* looks weird, but win3.1 KERNEL got a GeneralProtection handler
     * in lstrlen() ... we check only for NULL pointer reference.
     * - Marcus Meissner
     */
    TRACE(string,"strlen %s\n", debugstr_a (str));
    if (!str) return 0;
    return (INT32)strlen(str);
}


/***********************************************************************
 *           lstrlen32W   (KERNEL32.615)
 */
INT32 WINAPI lstrlen32W( LPCWSTR str )
{
    INT32 len = 0;
    TRACE(string,"strlen L%s\n", debugstr_w (str));
    if (!str) return 0;
    while (*str++) len++;
    return len;
}


/***********************************************************************
 *           lstrncmp32A   (Not a Windows API)
 */
INT32 WINAPI lstrncmp32A( LPCSTR str1, LPCSTR str2, INT32 n )
{
    TRACE(string,"strncmp %s and %s for %d chars\n",
		   debugstr_an (str1, n), debugstr_an (str2, n), n);
    return (INT32)strncmp( str1, str2, n );
}


/***********************************************************************
 *           lstrncmp32W   (Not a Windows API)
 */
INT32 WINAPI lstrncmp32W( LPCWSTR str1, LPCWSTR str2, INT32 n )
{
    TRACE(string,"strncmp L%s and L%s for %d chars\n",
		   debugstr_wn (str1, n), debugstr_wn (str2, n), n);
    if (!n) return 0;
    while ((--n > 0) && *str1 && (*str1 == *str2)) { str1++; str2++; }
    return (INT32)(*str1 - *str2);
}


/***********************************************************************
 *           lstrncmpi32A   (Not a Windows API)
 */
INT32 WINAPI lstrncmpi32A( LPCSTR str1, LPCSTR str2, INT32 n )
{
    INT32 res;

    TRACE(string,"strncmpi %s and %s for %d chars\n",
		   debugstr_an (str1, n), debugstr_an (str2, n), n);
    if (!n) return 0;
    while ((--n > 0) && *str1)
      if ( (res = toupper(*str1++) - toupper(*str2++)) ) return res;

    return toupper(*str1) - toupper(*str2);
}


/***********************************************************************
 *           lstrncmpi32W   (Not a Windows API)
 */
INT32 WINAPI lstrncmpi32W( LPCWSTR str1, LPCWSTR str2, INT32 n )
{
    INT32 res;

    TRACE(string,"strncmpi L%s and L%s for %d chars\n",
		   debugstr_wn (str1, n), debugstr_wn (str2, n), n);
    if (!n) return 0;
    while ((--n > 0) && *str1)
    {
        if ((res = towupper(*str1) - towupper(*str2)) != 0) return res;
        str1++;
        str2++;
    }
    return towupper(*str1) - towupper(*str2);
}


/***********************************************************************
 *           lstrcpyAtoW   (Not a Windows API)
 */
LPWSTR WINAPI lstrcpyAtoW( LPWSTR dst, LPCSTR src )
{
    register LPWSTR p = dst;

    TRACE(string,"%s\n",src);

    while ((*p++ = (WCHAR)(unsigned char)*src++));
    return dst;
}


/***********************************************************************
 *           lstrcpyWtoA   (Not a Windows API)
 */
LPSTR WINAPI lstrcpyWtoA( LPSTR dst, LPCWSTR src )
{
    register LPSTR p = dst;

    TRACE(string,"L%s\n",debugstr_w(src));

    while ((*p++ = (CHAR)*src++));
    return dst;
}


/***********************************************************************
 *           lstrcpynAtoW   (Not a Windows API)
 * Note: this function differs from the UNIX strncpy, it _always_ writes
 * a terminating \0
 */
LPWSTR WINAPI lstrcpynAtoW( LPWSTR dst, LPCSTR src, INT32 n )
{
    LPWSTR p = dst;

    TRACE(string,"%s %i\n",src, n);

    while ((n-- > 1) && *src) *p++ = (WCHAR)(unsigned char)*src++;
    if (n >= 0) *p = 0;
    return dst;
}


/***********************************************************************
 *           lstrcpynWtoA   (Not a Windows API)
 * Note: this function differs from the UNIX strncpy, it _always_ writes
 * a terminating \0
 */
LPSTR WINAPI lstrcpynWtoA( LPSTR dst, LPCWSTR src, INT32 n )
{
    LPSTR p = dst;

    TRACE(string,"L%s %i\n",debugstr_w(src), n);

    while ((n-- > 1) && *src) *p++ = (CHAR)*src++;
    if (n >= 0) *p = 0;
    return dst;
}

/***********************************************************************
 *           UnicodeToAnsi   (KERNEL.434)
 */
INT16 WINAPI UnicodeToAnsi( LPCWSTR src, LPSTR dst, INT16 codepage )
{
    if ( codepage != -1 )
        FIXME( string, "codepage %d not supported\n", codepage );

    lstrcpyWtoA( dst, src );

    return (INT16)lstrlen32A( dst );
}


/***********************************************************************
 *           Copy   (GDI.250)
 */
void WINAPI Copy( LPVOID src, LPVOID dst, WORD size )
{
    memcpy( dst, src, size );
}


/***********************************************************************
 *           RtlFillMemory   (KERNEL32.441)
 */
VOID WINAPI RtlFillMemory( LPVOID ptr, UINT32 len, UINT32 fill )
{
    memset( ptr, fill, len );
}


/***********************************************************************
 *           RtlMoveMemory   (KERNEL32.442)
 */
VOID WINAPI RtlMoveMemory( LPVOID dst, LPCVOID src, UINT32 len )
{
    memmove( dst, src, len );
}


/***********************************************************************
 *           RtlZeroMemory   (KERNEL32.444)
 */
VOID WINAPI RtlZeroMemory( LPVOID ptr, UINT32 len )
{
    memset( ptr, 0, len );
}


/***********************************************************************
 *           AnsiToOem16   (KEYBOARD.5)
 */
INT16 WINAPI AnsiToOem16( LPCSTR s, LPSTR d )
{
    CharToOem32A( s, d );
    return -1;
}


/***********************************************************************
 *           OemToAnsi16   (KEYBOARD.6)
 */
INT16 WINAPI OemToAnsi16( LPCSTR s, LPSTR d )
{
    OemToChar32A( s, d );
    return -1;
}


/***********************************************************************
 *           AnsiToOemBuff16   (KEYBOARD.134)
 */
void WINAPI AnsiToOemBuff16( LPCSTR s, LPSTR d, UINT16 len )
{
    CharToOemBuff32A( s, d, len ? len : 65536 );
}


/***********************************************************************
 *           OemToAnsiBuff16   (KEYBOARD.135)
 */
void WINAPI OemToAnsiBuff16( LPCSTR s, LPSTR d, UINT16 len )
{
    OemToCharBuff32A( s, d, len ? len : 65536 );
}


/***********************************************************************
 *           CharToOem32A   (USER32.37)
 */
BOOL32 WINAPI CharToOem32A( LPCSTR s, LPSTR d )
{
    LPSTR oldd = d;
    if (!s || !d) return TRUE;
    TRACE(string,"CharToOem %s\n", debugstr_a (s));
    while ((*d++ = ANSI_TO_OEM(*s++)));
    TRACE(string,"       to %s\n", debugstr_a (oldd));
    return TRUE;
}


/***********************************************************************
 *           CharToOemBuff32A   (USER32.38)
 */
BOOL32 WINAPI CharToOemBuff32A( LPCSTR s, LPSTR d, DWORD len )
{
    while (len--) *d++ = ANSI_TO_OEM(*s++);
    return TRUE;
}


/***********************************************************************
 *           CharToOemBuff32W   (USER32.39)
 */
BOOL32 WINAPI CharToOemBuff32W( LPCWSTR s, LPSTR d, DWORD len )
{
    while (len--) *d++ = ANSI_TO_OEM(*s++);
    return TRUE;
}


/***********************************************************************
 *           CharToOem32W   (USER32.40)
 */
BOOL32 WINAPI CharToOem32W( LPCWSTR s, LPSTR d )
{
    LPSTR oldd = d;
    if (!s || !d) return TRUE;
    TRACE(string,"CharToOem L%s\n", debugstr_w (s));
    while ((*d++ = ANSI_TO_OEM(*s++)));
    TRACE(string,"       to %s\n", debugstr_a (oldd));
    return TRUE;
}


/***********************************************************************
 *           OemToChar32A   (USER32.402)
 */
BOOL32 WINAPI OemToChar32A( LPCSTR s, LPSTR d )
{
    LPSTR oldd = d;
    TRACE(string,"OemToChar %s\n", debugstr_a (s));
    while ((*d++ = OEM_TO_ANSI(*s++)));
    TRACE(string,"       to %s\n", debugstr_a (oldd));
    return TRUE;
}


/***********************************************************************
 *           OemToCharBuff32A   (USER32.403)
 */
BOOL32 WINAPI OemToCharBuff32A( LPCSTR s, LPSTR d, DWORD len )
{
    TRACE(string,"OemToCharBuff %s\n", debugstr_an (s, len));
    while (len--) *d++ = OEM_TO_ANSI(*s++);
    return TRUE;
}


/***********************************************************************
 *           OemToCharBuff32W   (USER32.404)
 */
BOOL32 WINAPI OemToCharBuff32W( LPCSTR s, LPWSTR d, DWORD len )
{
    TRACE(string,"OemToCharBuff %s\n", debugstr_an (s, len));
    while (len--) *d++ = (WCHAR)OEM_TO_ANSI(*s++);
    return TRUE;
}


/***********************************************************************
 *           OemToChar32W   (USER32.405)
 */
BOOL32 WINAPI OemToChar32W( LPCSTR s, LPWSTR d )
{
    while ((*d++ = (WCHAR)OEM_TO_ANSI(*s++)));
    return TRUE;
}

/***********************************************************************
 *  WideCharToLocal (Not a Windows API)
 *  similar lstrcpyWtoA, should handle codepages properly
 *
 *  RETURNS
 *    strlen of the destination string
 */
 
INT32 WINAPI WideCharToLocal32(
    LPSTR pLocal, 
		LPWSTR pWide, 
		INT32 dwChars)
{ *pLocal = 0;
  TRACE(string,"(%p, %s, %i)\n",	pLocal, debugstr_w(pWide),dwChars);
  WideCharToMultiByte(CP_ACP,0,pWide,-1,pLocal,dwChars,NULL,NULL);
  return strlen(pLocal);
}
/***********************************************************************
 *  LocalToWideChar (Not a Windows API)
 *  similar lstrcpyAtoW, should handle codepages properly
 *
 *  RETURNS
 *    strlen of the destination string
 */
INT32 WINAPI LocalToWideChar32(
    LPWSTR pWide, 
		LPSTR pLocal, 
		INT32 dwChars)
{ *pWide = 0;
  TRACE(string,"(%p, %s, %i)\n",pWide,	pLocal, dwChars);
	MultiByteToWideChar(CP_ACP,0,pLocal,-1,pWide,dwChars); 
  return lstrlen32W(pWide);
}


/***********************************************************************
 *           lstrrchr   (Not a Windows API)
 *
 * This is the implementation meant to be invoked form within
 * COMCTL32_StrRChrA and shell32(TODO)...
 *
 * Return a pointer to the last occurence of wMatch in lpStart
 * not looking further than lpEnd...
 */
LPSTR WINAPI lstrrchr( LPCSTR lpStart, LPCSTR lpEnd, WORD wMatch )
{
  LPCSTR lpGotIt = NULL;

  TRACE(string,"(%s, %s)\n", lpStart, lpEnd);

  if (!lpEnd) lpEnd = lpStart + strlen(lpStart);

  for(; lpStart < lpEnd; lpStart = CharNext32A(lpStart)) 
    if (!ChrCmpA( GET_WORD(lpStart), wMatch)) 
      lpGotIt = lpStart;
    
  return ((LPSTR)lpGotIt);
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

  TRACE(string,"(%p, %p, %c)\n", lpStart,      lpEnd, wMatch);
  if (!lpEnd) lpEnd = lpStart + lstrlen32W(lpStart);

  for(; lpStart < lpEnd; lpStart = CharNext32W(lpStart)) 
    if (!ChrCmpW( GET_WORD(lpStart), wMatch)) 
      lpGotIt = lpStart;
    
  return (LPWSTR)lpGotIt;
}

/***********************************************************************
 *           strstrw   (Not a Windows API)
 *
 * This is the implementation meant to be invoked form within
 * COMCTL32_StrStrW and shell32(TODO)...
 *
 */
LPWSTR WINAPI strstrw( LPCWSTR lpFirst, LPCWSTR lpSrch) {
  UINT32 uSrchLen  = (UINT32)lstrlen32W(lpSrch);
  WORD wMatchBeg   = *(WORD*)lpSrch;

  TRACE(string,"(%p, %p)\n", lpFirst,  lpSrch);

  for(; 
    ((lpFirst=CRTDLL_wcschr(lpFirst, wMatchBeg))!=0) && 
      lstrncmp32W(lpFirst, lpSrch, uSrchLen); 
    lpFirst++) {
      continue;
  }
  return (LPWSTR)lpFirst;
}


/***********************************************************************
 *           ChrCmpA   
 * This fuction returns FALSE if both words match, TRUE otherwise...
 */
static BOOL32 ChrCmpA( WORD word1, WORD word2) {
  if (LOBYTE(word1) == LOBYTE(word2)) {
    if (IsDBCSLeadByte32(LOBYTE(word1))) {
      return (word1 != word2);
    }
    return FALSE;
  }
  return TRUE;
}

/***********************************************************************
 *           ChrCmpW   
 * This fuction returns FALSE if both words match, TRUE otherwise...
 */
static BOOL32 ChrCmpW( WORD word1, WORD word2) {
  return (word1 != word2);
}


