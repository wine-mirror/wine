/*
 * CRTDLL multi-byte string functions
 *
 * Copyright 1999 Alexandre Julliard
 * Copyright 2000 Jon Griffths
 *
 * NOTES
 * See msdn.microsoft.com
 * /library/devprods/vs6/visualc/vccore/_crt__ismbb_routines.htm
 * For details and CP 932 information.
 *
 * This code assumes that MB_LEN_MAX is 2 and that [0,0] is an
 * invalid MB char. If that changes, this will need to too.
 *
 * CRTDLL reports valid CP 932 multibyte chars when the current
 * code page is not 932. MSVCRT fixes this bug - we implement
 * MSVCRT's behaviour, since its correct. However, MSVCRT fails
 * to set the code page correctly when the locale is changed, so
 * it must be explicitly set to get matching results.
 *
 * FIXME
 * Not currently binary compatable with win32. CRTDLL_mbctype must be
 * populated correctly and the ismb* functions should reference it.
 */

#include "crtdll.h"

DEFAULT_DEBUG_CHANNEL(crtdll);

UCHAR CRTDLL_mbctype[257];


/*********************************************************************
 *                  _mbscmp      (CRTDLL.??)
 *
 * Compare two multibyte strings.
 */
INT __cdecl CRTDLL__mbscmp( LPCSTR str, LPCSTR cmp )
{
  if (CRTDLL__mb_cur_max_dll > 1)
  {
    UINT strc, cmpc;
    do {
      if (!*str)
        return *cmp ? -1 : 0;
      if (!*cmp)
        return 1;
      strc = CRTDLL__mbsnextc(str);
      cmpc = CRTDLL__mbsnextc(cmp);
      if (strc != cmpc)
        return strc < cmpc ? -1 : 1;
      str += (strc > 255) ? 2 : 1;
      cmp += (strc > 255) ? 2 : 1; /* equal, use same increment */
    } while (1);
  }
  return strcmp(str, cmp); /* ASCII CP */
}


/*********************************************************************
 *                  _mbsicmp      (CRTDLL.??)
 *
 * Compare two multibyte strings case insensitively.
 */
INT __cdecl CRTDLL__mbsicmp( LPCSTR str, LPCSTR cmp )
{
  /* FIXME: No tolower() for mb strings yet */
  if (CRTDLL__mb_cur_max_dll > 1)
    return CRTDLL__mbscmp( str, cmp );
  return strcasecmp( str, cmp ); /* ASCII CP */
}


/*********************************************************************
 *                  _mbsncmp     (CRTDLL.??)
 *
 * Compare two multibyte strings to 'len' characters.
 */
INT __cdecl CRTDLL__mbsncmp( LPCSTR str, LPCSTR cmp, UINT len )
{
  if (!len)
    return 0;

  if (CRTDLL__mb_cur_max_dll > 1)
  {
    UINT strc, cmpc;
    while (len--)
    {
      if (!*str)
        return *cmp ? -1 : 0;
      if (!*cmp)
        return 1;
      strc = CRTDLL__mbsnextc(str);
      cmpc = CRTDLL__mbsnextc(cmp);
      if (strc != cmpc)
        return strc < cmpc ? -1 : 1;
      str += (strc > 255) ? 2 : 1;
      cmp += (strc > 255) ? 2 : 1; /* Equal, use same increment */
    }
    return 0; /* Matched len chars */
  }
  return strncmp(str, cmp, len); /* ASCII CP */
}


/*********************************************************************
 *                  _mbsnicmp      (CRTDLL.??)
 *
 * Compare two multibyte strings case insensitively to 'len' characters.
 */
INT __cdecl CRTDLL__mbsnicmp( LPCSTR str, LPCSTR cmp, UINT len )
{
  /* FIXME: No tolower() for mb strings yet */
  if (CRTDLL__mb_cur_max_dll > 1)
    return CRTDLL__mbsncmp( str, cmp, len );
  return strncasecmp( str, cmp, len ); /* ASCII CP */
}


/*********************************************************************
 *                  _mbsinc        (CRTDLL.205)
 *
 * Return the next character of a string.
 */
LPSTR __cdecl CRTDLL__mbsinc( LPCSTR str )
{
  if (CRTDLL__mb_cur_max_dll > 1 &&
      CRTDLL_isleadbyte(*str))
    return (LPSTR)str + 2; /* MB char */

  return (LPSTR)str + 1; /* ASCII CP or SB char */
}


/*********************************************************************
 *                  _mbsninc       (CRTDLL.??)
 *
 * Return the 'num'th character of a string.
 */
LPSTR CRTDLL__mbsninc( LPCSTR str, INT num )
{
  if (!str || num < 1)
    return NULL;
  if (CRTDLL__mb_cur_max_dll > 1)
  {
    while(num--)
      str = CRTDLL__mbsinc( str );
    return (LPSTR)str;
  }
  return (LPSTR)str + num; /* ASCII CP */
}


/*********************************************************************
 *                  _mbslen        (CRTDLL.206)
 *
 * Get the length of a string.
 */
INT __cdecl CRTDLL__mbslen( LPCSTR str )
{
  if (CRTDLL__mb_cur_max_dll > 1)
  {
    INT len = 0;
    while (*str)
    {
      str += CRTDLL_isleadbyte( *str ) ? 2 : 1;
      len++;
    }
    return len;
  }
  return strlen( str ); /* ASCII CP */
}


/*********************************************************************
 *                  _mbsrchr           (CRTDLL.223)
 */
LPSTR __cdecl CRTDLL__mbsrchr(LPCSTR s,CHAR x)
{
    /* FIXME: handle multibyte strings */
    return strrchr(s,x);
}


/*********************************************************************
 *                   mbtowc            (CRTDLL.430)
 */
INT __cdecl CRTDLL_mbtowc( WCHAR *dst, LPCSTR str, INT n )
{
    if (n <= 0) return 0;
    if (!str) return 0;
    if (!MultiByteToWideChar( CP_ACP, 0, str, n, dst, 1 )) return 0;
    /* return the number of bytes from src that have been used */
    if (!*str) return 0;
    if (n >= 2 && CRTDLL_isleadbyte(*str) && str[1]) return 2;
    return 1;
}


/*********************************************************************
 *                  _mbccpy           (CRTDLL.??)
 *
 * Copy one multibyte character to another
 */
VOID __cdecl CRTDLL__mbccpy(LPSTR dest, LPSTR src)
{
  *dest++ = *src;
  if (CRTDLL__mb_cur_max_dll > 1 && CRTDLL_isleadbyte(*src))
    *dest = *++src; /* MB char */
}


/*********************************************************************
 *                  _mbsdec           (CRTDLL.??)
 *
 * Return the character before 'cur'.
 */
LPSTR __cdecl CRTDLL__mbsdec( LPCSTR start, LPCSTR cur )
{
  if (CRTDLL__mb_cur_max_dll > 1)
    return (LPSTR)(CRTDLL__ismbstrail(start,cur-1) ? cur - 2 : cur -1);

  return (LPSTR)cur - 1; /* ASCII CP or SB char */
}


/*********************************************************************
 *                  _mbbtombc         (CRTDLL.??)
 *
 * Convert a single byte character to a multi byte character.
 */
USHORT __cdecl CRTDLL__mbbtombc( USHORT c ) 
{
  if (CRTDLL__mb_cur_max_dll > 1 &&
      ((c >= 0x20 && c <=0x7e) || (c >= 0xa1 && c <= 0xdf)))
  {
    /* FIXME: I can't get this function to return anything
     * different to what I pass it...
     */
  }
  return c;  /* ASCII CP or no MB char */
}


/*********************************************************************
 *                  _mbclen          (CRTDLL.??)
 *
 * Get the length of a multibyte character.
 */
INT __cdecl CRTDLL__mbclen( LPCSTR str )
{
  return CRTDLL_isleadbyte( *str ) ? 2 : 1;
}


/*********************************************************************
 *                  _ismbbkana       (CRTDLL.??)
 *
 * Is the given single byte character Katakana?
 */
INT __cdecl CRTDLL__ismbbkana( UINT c )
{
  /* FIXME: use lc_ctype when supported, not lc_all */
  if (__CRTDLL_current_lc_all_cp == 932)
  {
    /* Japanese/Katakana, CP 932 */
    return (c >= 0xa1 && c <= 0xdf);
  }
  return 0;
}


/*********************************************************************
 *                  _ismbchira       (CRTDLL.??)
 *
 * Is the given character Hiragana?
 */
INT __cdecl CRTDLL__ismbchira( UINT c )
{
  /* FIXME: use lc_ctype when supported, not lc_all */
  if (__CRTDLL_current_lc_all_cp == 932)
  {
    /* Japanese/Hiragana, CP 932 */
    return (c >= 0x829f && c <= 0x82f1);
  }
  return 0;
}


/*********************************************************************
 *                  _ismbckata       (CRTDLL.??)
 *
 * Is the given double byte character Katakana?
 */
INT __cdecl CRTDLL__ismbckata( UINT c )
{
  /* FIXME: use lc_ctype when supported, not lc_all */
  if (__CRTDLL_current_lc_all_cp == 932)
  {
    if ( c < 256)
      return CRTDLL__ismbbkana( c );
    /* Japanese/Katakana, CP 932 */
    return (c >= 0x8340 && c <= 0x8396 && c != 0x837f);
  }
  return 0;
}


/*********************************************************************
 *                  _ismbblead        (CRTDLL.??)
 *
 * Is the given single byte character a lead byte?
 */
INT __cdecl CRTDLL__ismbblead( UINT c )
{
  /* FIXME: should reference CRTDLL_mbctype */
  return CRTDLL__mb_cur_max_dll > 1 && CRTDLL_isleadbyte( c );
}


/*********************************************************************
 *                  _ismbbtrail       (CRTDLL.??)
 *
 * Is the given single byte character a trail byte?
 */
INT __cdecl CRTDLL__ismbbtrail( UINT c )
{
  /* FIXME: should reference CRTDLL_mbctype */
  return !CRTDLL__ismbblead( c );
}


/*********************************************************************
 *                  _ismbslead        (CRTDLL.??)
 *
 * Is the character pointed to 'str' a lead byte?
 */
INT __cdecl CRTDLL__ismbslead( LPCSTR start, LPCSTR str )
{
  /* Lead bytes can also be trail bytes if caller messed up
   * iterating through the string...
   */
  if (CRTDLL__mb_cur_max_dll > 1)
  {
    while (start < str)
      start += CRTDLL_isleadbyte ( *str ) ? 2 : 1;

    if (start == str)
      return CRTDLL_isleadbyte( *str );
  }
  return 0; /* Must have been a trail, we skipped it */
}


/*********************************************************************
 *                  _ismbstrail        (CRTDLL.??)
 *
 * Is the character pointed to 'str' a trail byte?
 */
INT __cdecl CRTDLL__ismbstrail( LPCSTR start, LPCSTR str )
{
  /* Must not be a lead, and must be preceeded by one */
  return !CRTDLL__ismbslead( start, str ) && CRTDLL_isleadbyte(str[-1]);
}


/*********************************************************************
 *                  _mbsset            (CRTDLL.??)
 *
 * Fill a multibyte string with a value.
 */
LPSTR __cdecl CRTDLL__mbsset( LPSTR str, UINT c )
{
  LPSTR ret = str;

  if (CRTDLL__mb_cur_max_dll == 1 || c < 256)
    return CRTDLL__strset( str, c ); /* ASCII CP or SB char */

  c &= 0xffff; /* Strip high bits */

  while (str[0] && str[1])
  {
    *str++ = c >> 8;
    *str++ = c & 0xff;
  }
  if (str[0])
    str[0] = '\0'; /* FIXME: OK to shorten? */

  return ret;
}

/*********************************************************************
 *                  _mbsnset            (CRTDLL.??)
 *
 * Fill a multibyte string with a value up to 'len' characters.
 */
LPSTR __cdecl CRTDLL__mbsnset( LPSTR str, UINT c, UINT len )
{
  LPSTR ret = str;

  if (!len)
    return ret;

  if (CRTDLL__mb_cur_max_dll == 1 || c < 256)
    return CRTDLL__strnset( str, c, len ); /* ASCII CP or SB char */

  c &= 0xffff; /* Strip high bits */

  while (str[0] && str[1] && len--)
  {
    *str++ = c >> 8;
    *str++ = c & 0xff;
  }
  if (len && str[0])
    str[0] = '\0'; /* FIXME: OK to shorten? */

  return ret;
}


/*********************************************************************
 *                  _mbstrlen           (CRTDLL.??)
 *
 * Get the length of a multibyte string.
 */
INT __cdecl CRTDLL__mbstrlen( LPCSTR str )
{
  if (CRTDLL__mb_cur_max_dll > 1)
  {
    INT len = 0;
    while (*str)
    {
      str += CRTDLL_isleadbyte ( *str ) ? 2 : 1;
      len++;
    }
    return len;
  }
  return strlen( str ); /* ASCII CP */
}


/*********************************************************************
 *                  _mbsnextc          (CRTDLL.??)
 * 
 * Get the next character from a multibyte string.
 */
UINT __cdecl CRTDLL__mbsnextc( LPCSTR str )
{
  if (CRTDLL__mb_cur_max_dll > 1 && CRTDLL_isleadbyte( *str ))
    return *str << 8 | str[1];

  return *str; /* ASCII CP or SB char */
}


/*********************************************************************
 *                  _mbsncpy           (CRTDLL.??)
 *
 * Copy one multibyte string to another up to 'len' characters.
 */
LPSTR __cdecl CRTDLL__mbsncpy( LPSTR dst, LPCSTR src, UINT len )
{
  if (!len)
    return dst;
  if (CRTDLL__mb_cur_max_dll > 1)
  {
    LPSTR ret = dst;
    while (src[0] && src[1] && len--)
    {
      *dst++ = *src++;
      *dst++ = *src++;
    }
    if (len--)
    {
      *dst++ = *src++; /* Last char or '\0' */
      while(len--)
        *dst++ = '\0';
    }
    return ret;
  }
  return strncpy( dst, src, len ); /* ASCII CP */
}


/*********************************************************************
 *                  _mbschr            (CRTDLL.??)
 *
 * Find a multibyte character in a multibyte string.
 */
LPSTR __cdecl CRTDLL__mbschr( LPCSTR str, UINT c )
{
  if (CRTDLL__mb_cur_max_dll > 1)
  {
    UINT next;
    while((next = CRTDLL__mbsnextc( str )))
    {
      if (next == c)
        return (LPSTR)str;
      str += next > 255 ? 2 : 1;
    }
    return c ? NULL : (LPSTR)str;
  }
  return strchr( str, c ); /* ASCII CP */
}


/*********************************************************************
 *                  _mbsnccnt          (CRTDLL.??)
 *
 * Return the number of mutibyte characters in 'len' bytes of a string.
 */
UINT __cdecl CRTDLL__mbsnccnt( LPCSTR str, UINT len )
{
  int ret = 0;

  if (CRTDLL__mb_cur_max_dll > 1)
  {
    while(*str && len-- > 0)
    {
      if (CRTDLL_isleadbyte ( *str ))
      {
        str++;
        len--;
      }
      ret++;
      str++;
    }
    return ret;
  }
  return min( strlen( str ), len ); /* ASCII CP */
}


/*********************************************************************
 *                  _mbsncat           (CRTDLL.??)
 *
 * Add 'len' characters from one multibyte string to another.
 */
LPSTR __cdecl CRTDLL__mbsncat( LPSTR dst, LPCSTR src, UINT len )
{
  if (CRTDLL__mb_cur_max_dll > 1)
  {
    LPSTR res = dst;
    dst += CRTDLL__mbslen( dst );
    while (*src && len--)
    {
      *dst = *src;
      if (CRTDLL_isleadbyte( *src )) 
        *++dst = *++src;
      dst++;
      src++;
    }
    *dst++ = '\0';
    return res;
  }
  return strncat( dst, src, len ); /* ASCII CP */
}

