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
#include "stddebug.h"
#include "debug.h"

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
    dprintf_string(stddeb,"strcat: Append '%s' to '%s'\n",
		 (src)?src:"NULL",(dst)?dst:"NULL");
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
    dprintf_string(stddeb,"strcatn add %d chars from '%s' to '%s'\n",
		 n,(src)?src:"NULL",(dst)?dst:"NULL");
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
    dprintf_string(stddeb,"strcmp: '%s' and '%s'\n",
		 (str1)?str1:"NULL",(str2)?str2:"NULL");
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

    dprintf_string(stddeb,"strcmpi '%s' and '%s'\n",
		 (str1)?str1:"NULL",(str2)?str2:"NULL");
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
    dprintf_string(stddeb,"strcpy '%s'\n",
		 (src)?src:"NULL");
    if (!src || !dst) return NULL;
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
    dprintf_string(stddeb,"strcpyn '%s' for %d chars\n",
		 (src)?src:"NULL",n);
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
    dprintf_string(stddeb,"strlen '%s'\n", (str)?str:"NULL");
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
 *           lstrncmp32A   (Not a Windows API)
 */
INT32 lstrncmp32A( LPCSTR str1, LPCSTR str2, INT32 n )
{
    dprintf_string(stddeb,"strncmp '%s' and '%s' for %d chars\n",
		 (str1)?str1:"NULL",(str2)?str2:"NULL",n);
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
 *           lstrncmpi32A   (Not a Windows API)
 */
INT32 lstrncmpi32A( LPCSTR str1, LPCSTR str2, INT32 n )
{
    INT32 res;

    dprintf_string(stddeb,"strncmpi '%s' and '%s' for %d chars\n",
		 (str1)?str1:"NULL",(str2)?str2:"NULL",n);
    if (!n) return 0;
    while ((--n > 0) && *str1)
      if ( (res = toupper(*str1++) - toupper(*str2++)) ) return res;

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


/***********************************************************************
 *           lstrcpyAtoW   (Not a Windows API)
 */
LPWSTR lstrcpyAtoW( LPWSTR dst, LPCSTR src )
{
    register LPWSTR p = dst;
    while ((*p++ = (WCHAR)(unsigned char)*src++));
    return dst;
}


/***********************************************************************
 *           lstrcpyWtoA   (Not a Windows API)
 */
LPSTR lstrcpyWtoA( LPSTR dst, LPCWSTR src )
{
    register LPSTR p = dst;
    while ((*p++ = (CHAR)*src++));
    return dst;
}


/***********************************************************************
 *           lstrcpynAtoW   (Not a Windows API)
 */
LPWSTR lstrcpynAtoW( LPWSTR dst, LPCSTR src, INT32 n )
{
    LPWSTR p = dst;
    while ((n-- > 1) && *src) *p++ = (WCHAR)(unsigned char)*src++;
    *p = 0;
    return dst;
}


/***********************************************************************
 *           lstrcpynWtoA   (Not a Windows API)
 */
LPSTR lstrcpynWtoA( LPSTR dst, LPCWSTR src, INT32 n )
{
    LPSTR p = dst;
    while ((n-- > 1) && *src) *p++ = (CHAR)*src++;
    *p = 0;
    return dst;
}


/***********************************************************************
 *           Copy   (GDI.250)
 */
void Copy( LPVOID src, LPVOID dst, WORD size )
{
    memcpy( dst, src, size );
}


/***********************************************************************
 *           RtlFillMemory   (KERNEL32.441)
 */
VOID RtlFillMemory( LPVOID ptr, UINT32 len, UINT32 fill )
{
    memset( ptr, fill, len );
}


/***********************************************************************
 *           RtlMoveMemory   (KERNEL32.442)
 */
VOID RtlMoveMemory( LPVOID dst, LPCVOID src, UINT32 len )
{
    memmove( dst, src, len );
}


/***********************************************************************
 *           RtlZeroMemory   (KERNEL32.444)
 */
VOID RtlZeroMemory( LPVOID ptr, UINT32 len )
{
    memset( ptr, 0, len );
}


/***********************************************************************
 *           AnsiToOem16   (KEYBOARD.5)
 */
INT16 AnsiToOem16( LPCSTR s, LPSTR d )
{
    CharToOem32A( s, d );
    return -1;
}


/***********************************************************************
 *           OemToAnsi16   (KEYBOARD.6)
 */
INT16 OemToAnsi16( LPCSTR s, LPSTR d )
{
    OemToChar32A( s, d );
    return -1;
}


/***********************************************************************
 *           AnsiToOemBuff16   (KEYBOARD.134)
 */
void AnsiToOemBuff16( LPCSTR s, LPSTR d, UINT16 len )
{
    CharToOemBuff32A( s, d, len ? len : 65536 );
}


/***********************************************************************
 *           OemToAnsiBuff16   (KEYBOARD.135)
 */
void OemToAnsiBuff16( LPCSTR s, LPSTR d, UINT16 len )
{
    OemToCharBuff32A( s, d, len ? len : 65536 );
}


/***********************************************************************
 *           CharToOem32A   (USER32.36)
 */
BOOL32 CharToOem32A( LPCSTR s, LPSTR d )
{
    if (!s || !d) return TRUE;
    dprintf_string(stddeb,"CharToOem '%s'\n", (s)?s:"NULL");
    while ((*d++ = ANSI_TO_OEM(*s++)));
    dprintf_string(stddeb," to '%s'\n", (d)?d:"NULL");
    return TRUE;
}


/***********************************************************************
 *           CharToOemBuff32A   (USER32.37)
 */
BOOL32 CharToOemBuff32A( LPCSTR s, LPSTR d, DWORD len )
{
    while (len--) *d++ = ANSI_TO_OEM(*s++);
    return TRUE;
}


/***********************************************************************
 *           CharToOemBuff32W   (USER32.38)
 */
BOOL32 CharToOemBuff32W( LPCWSTR s, LPSTR d, DWORD len )
{
    while (len--) *d++ = ANSI_TO_OEM(*s++);
    return TRUE;
}


/***********************************************************************
 *           CharToOem32W   (USER32.39)
 */
BOOL32 CharToOem32W( LPCWSTR s, LPSTR d )
{
    while ((*d++ = ANSI_TO_OEM(*s++)));
    return TRUE;
}


/***********************************************************************
 *           OemToChar32A   (USER32.401)
 */
BOOL32 OemToChar32A( LPCSTR s, LPSTR d )
{
    dprintf_string(stddeb,"OemToChar '%s'\n", (s)?s:"NULL");
    while ((*d++ = OEM_TO_ANSI(*s++)));
    dprintf_string(stddeb," to '%s'\n", (d)?d:"NULL");
    return TRUE;
}


/***********************************************************************
 *           OemToCharBuff32A   (USER32.402)
 */
BOOL32 OemToCharBuff32A( LPCSTR s, LPSTR d, DWORD len )
{
    dprintf_string(stddeb,"OemToCharBuff '%s' for %ld chars\n", (s)?s:"NULL",len);
    while (len--) *d++ = OEM_TO_ANSI(*s++);
    return TRUE;
}


/***********************************************************************
 *           OemToCharBuff32W   (USER32.403)
 */
BOOL32 OemToCharBuff32W( LPCSTR s, LPWSTR d, DWORD len )
{
    while (len--) *d++ = (WCHAR)OEM_TO_ANSI(*s++);
    return TRUE;
}


/***********************************************************************
 *           OemToChar32W   (USER32.404)
 */
BOOL32 OemToChar32W( LPCSTR s, LPWSTR d )
{
    while ((*d++ = (WCHAR)OEM_TO_ANSI(*s++)));
    return TRUE;
}
