/*
 * msvcrt.dll mbcs functions
 *
 * Copyright 1999 Alexandre Julliard
 * Copyright 2000 Jon Griffths
 *
 * FIXME
 * Not currently binary compatible with win32. MSVCRT_mbctype must be
 * populated correctly and the ismb* functions should reference it.
 */
#include "msvcrt.h"

DEFAULT_DEBUG_CHANNEL(msvcrt);

unsigned char MSVCRT_mbctype[257];
int MSVCRT___mb_cur_max = 1;

int MSVCRT_isleadbyte(int);
char *_strset(char *, int);
char *_strnset(char *, int, unsigned int);
extern unsigned int MSVCRT_current_lc_all_cp;


/*********************************************************************
 *		__p__mbctype (MSVCRT.@)
 */
unsigned char *__p__mbctype(void)
{
  return MSVCRT_mbctype;
}

/*********************************************************************
 *		__p___mb_cur_max(MSVCRT.@)
 */
int *__p___mb_cur_max(void)
{
  return &MSVCRT___mb_cur_max;
}

/*********************************************************************
 *		_mbsnextc(MSVCRT.@)
 */
unsigned int _mbsnextc(const unsigned char *str)
{
  if(MSVCRT___mb_cur_max > 1 && MSVCRT_isleadbyte(*str))
    return *str << 8 | str[1];
  return *str; /* ASCII CP or SB char */
}

/*********************************************************************
 *		_mbscmp(MSVCRT.@)
 */
int _mbscmp(const char *str, const char *cmp)
{
  if(MSVCRT___mb_cur_max > 1)
  {
    unsigned int strc, cmpc;
    do {
      if(!*str)
        return *cmp ? -1 : 0;
      if(!*cmp)
        return 1;
      strc = _mbsnextc(str);
      cmpc = _mbsnextc(cmp);
      if(strc != cmpc)
        return strc < cmpc ? -1 : 1;
      str +=(strc > 255) ? 2 : 1;
      cmp +=(strc > 255) ? 2 : 1; /* equal, use same increment */
    } while(1);
  }
  return strcmp(str, cmp); /* ASCII CP */
}

/*********************************************************************
 *		_mbsicmp(MSVCRT.@)
 */
int _mbsicmp(const char *str, const char *cmp)
{
  /* FIXME: No tolower() for mb strings yet */
  if(MSVCRT___mb_cur_max > 1)
    return _mbscmp(str, cmp);
  return strcasecmp(str, cmp); /* ASCII CP */
}

/*********************************************************************
 *		_mbsncmp    (MSVCRT.@)
 */
int _mbsncmp(const char *str, const char *cmp, unsigned int len)
{
  if(!len)
    return 0;

  if(MSVCRT___mb_cur_max > 1)
  {
    unsigned int strc, cmpc;
    while(len--)
    {
      if(!*str)
        return *cmp ? -1 : 0;
      if(!*cmp)
        return 1;
      strc = _mbsnextc(str);
      cmpc = _mbsnextc(cmp);
      if(strc != cmpc)
        return strc < cmpc ? -1 : 1;
      str +=(strc > 255) ? 2 : 1;
      cmp +=(strc > 255) ? 2 : 1; /* Equal, use same increment */
    }
    return 0; /* Matched len chars */
  }
  return strncmp(str, cmp, len); /* ASCII CP */
}

/*********************************************************************
 *                  _mbsnicmp(MSVCRT.@)
 *
 * Compare two multibyte strings case insensitively to 'len' characters.
 */
int _mbsnicmp(const char *str, const char *cmp, unsigned int len)
{
  /* FIXME: No tolower() for mb strings yet */
  if(MSVCRT___mb_cur_max > 1)
    return _mbsncmp(str, cmp, len);
  return strncasecmp(str, cmp, len); /* ASCII CP */
}

/*********************************************************************
 *		_mbsinc(MSVCRT.@)
 */
char *_mbsinc(const unsigned char *str)
{
  if(MSVCRT___mb_cur_max > 1 && MSVCRT_isleadbyte(*str))
    return (char *)str + 2; /* MB char */

  return (char *)str + 1; /* ASCII CP or SB char */
}

/*********************************************************************
 *		_mbsninc(MSVCRT.@)
 */
char *_mbsninc(const char *str, unsigned int num)
{
  if(!str || num < 1)
    return NULL;
  if(MSVCRT___mb_cur_max > 1)
  {
    while(num--)
      str = _mbsinc(str);
    return (char *)str;
  }
  return (char *)str + num; /* ASCII CP */
}

/*********************************************************************
 *		_mbslen(MSVCRT.206)
 */
int _mbslen(const unsigned char *str)
{
  if(MSVCRT___mb_cur_max > 1)
  {
    int len = 0;
    while(*str)
    {
      str += MSVCRT_isleadbyte(*str) ? 2 : 1;
      len++;
    }
    return len;
  }
  return strlen(str); /* ASCII CP */
}

/*********************************************************************
 *		_mbsrchr(MSVCRT.@)
 */
char *_mbsrchr(const char *s,unsigned int x)
{
  /* FIXME: handle multibyte strings */
  return strrchr(s,x);
}

/*********************************************************************
 *		mbtowc(MSVCRT.@)
 */
int MSVCRT_mbtowc(WCHAR *dst, const unsigned char *str, unsigned int n)
{
  if(n <= 0 || !str)
    return 0;
  if(!MultiByteToWideChar(CP_ACP, 0, str, n, dst, 1))
    return 0;
  /* return the number of bytes from src that have been used */
  if(!*str)
    return 0;
  if(n >= 2 && MSVCRT_isleadbyte(*str) && str[1])
    return 2;
  return 1;
}

/*********************************************************************
 *		_mbccpy(MSVCRT.@)
 */
void _mbccpy(char *dest, const unsigned char *src)
{
  *dest++ = *src;
  if(MSVCRT___mb_cur_max > 1 && MSVCRT_isleadbyte(*src))
    *dest = *++src; /* MB char */
}

/*********************************************************************
 *		_mbbtombc(MSVCRT.@)
 */
unsigned int _mbbtombc(unsigned int c)
{
  if(MSVCRT___mb_cur_max > 1 &&
     ((c >= 0x20 && c <=0x7e) ||(c >= 0xa1 && c <= 0xdf)))
  {
    /* FIXME: I can't get this function to return anything
     * different to what I pass it...
     */
  }
  return c;  /* ASCII CP or no MB char */
}

/*********************************************************************
 *		_mbclen(MSVCRT.@)
 */
unsigned int _mbclen(const unsigned char *str)
{
  return MSVCRT_isleadbyte(*str) ? 2 : 1;
}

/*********************************************************************
 *		_ismbbkana(MSVCRT.@)
 */
int _ismbbkana(unsigned int c)
{
  /* FIXME: use lc_ctype when supported, not lc_all */
  if(MSVCRT_current_lc_all_cp == 932)
  {
    /* Japanese/Katakana, CP 932 */
    return (c >= 0xa1 && c <= 0xdf);
  }
  return 0;
}

/*********************************************************************
 *		_ismbchira(MSVCRT.@)
 */
int _ismbchira(unsigned int c)
{
  /* FIXME: use lc_ctype when supported, not lc_all */
  if(MSVCRT_current_lc_all_cp == 932)
  {
    /* Japanese/Hiragana, CP 932 */
    return (c >= 0x829f && c <= 0x82f1);
  }
  return 0;
}

/*********************************************************************
 *		_ismbckata(MSVCRT.@)
 */
int _ismbckata(unsigned int c)
{
  /* FIXME: use lc_ctype when supported, not lc_all */
  if(MSVCRT_current_lc_all_cp == 932)
  {
    if(c < 256)
      return _ismbbkana(c);
    /* Japanese/Katakana, CP 932 */
    return (c >= 0x8340 && c <= 0x8396 && c != 0x837f);
  }
  return 0;
}

/*********************************************************************
 *		_ismbblead(MSVCRT.@)
 */
int _ismbblead(unsigned int c)
{
  /* FIXME: should reference MSVCRT_mbctype */
  return MSVCRT___mb_cur_max > 1 && MSVCRT_isleadbyte(c);
}


/*********************************************************************
 *		_ismbbtrail(MSVCRT.@)
 */
int _ismbbtrail(unsigned int c)
{
  /* FIXME: should reference MSVCRT_mbctype */
  return !_ismbblead(c);
}

/*********************************************************************
 *		_ismbslead(MSVCRT.@)
 */
int _ismbslead(const unsigned char *start, const unsigned char *str)
{
  /* Lead bytes can also be trail bytes if caller messed up
   * iterating through the string...
   */
  if(MSVCRT___mb_cur_max > 1)
  {
    while(start < str)
      start += MSVCRT_isleadbyte(*str) ? 2 : 1;

    if(start == str)
      return MSVCRT_isleadbyte(*str);
  }
  return 0; /* Must have been a trail, we skipped it */
}

/*********************************************************************
 *		_ismbstrail(MSVCRT.@)
 */
int _ismbstrail(const char *start, const unsigned char *str)
{
  /* Must not be a lead, and must be preceeded by one */
  return !_ismbslead(start, str) && MSVCRT_isleadbyte(str[-1]);
}

/*********************************************************************
 *		_mbsdec(MSVCRT.@)
 */
char *_mbsdec(const char *start, const char *cur)
{
  if(MSVCRT___mb_cur_max > 1)
    return (char *)(_ismbstrail(start,cur-1) ? cur - 2 : cur -1);

  return (char *)cur - 1; /* ASCII CP or SB char */
}

/*********************************************************************
 *		_mbsset(MSVCRT.@)
 */
char *_mbsset(char *str, unsigned int c)
{
  char *ret = str;

  if(MSVCRT___mb_cur_max == 1 || c < 256)
    return _strset(str, c); /* ASCII CP or SB char */

  c &= 0xffff; /* Strip high bits */

  while(str[0] && str[1])
  {
    *str++ = c >> 8;
    *str++ = c & 0xff;
  }
  if(str[0])
    str[0] = '\0'; /* FIXME: OK to shorten? */

  return ret;
}

/*********************************************************************
 *		_mbsnset(MSVCRT.@)
 */
char *_mbsnset(char *str, unsigned int c, unsigned int len)
{
  char *ret = str;

  if(!len)
    return ret;

  if(MSVCRT___mb_cur_max == 1 || c < 256)
    return _strnset(str, c, len); /* ASCII CP or SB char */

  c &= 0xffff; /* Strip high bits */

  while(str[0] && str[1] && len--)
  {
    *str++ = c >> 8;
    *str++ = c & 0xff;
  }
  if(len && str[0])
    str[0] = '\0'; /* FIXME: OK to shorten? */

  return ret;
}

/*********************************************************************
 *		_mbstrlen(MSVCRT.@)
 */
int _mbstrlen(const unsigned char *str)
{
  if(MSVCRT___mb_cur_max > 1)
  {
    int len = 0;
    while(*str)
    {
      str += MSVCRT_isleadbyte(*str) ? 2 : 1;
      len++;
    }
    return len;
  }
  return strlen(str); /* ASCII CP */
}

/*********************************************************************
 *		_mbsncpy(MSVCRT.@)
 */
char *_mbsncpy(char *dst, const char *src, unsigned int len)
{
  if(!len)
    return dst;
  if(MSVCRT___mb_cur_max > 1)
  {
    char *ret = dst;
    while(src[0] && src[1] && len--)
    {
      *dst++ = *src++;
      *dst++ = *src++;
    }
    if(len--)
    {
      *dst++ = *src++; /* Last char or '\0' */
      while(len--)
        *dst++ = '\0';
    }
    return ret;
  }
  return strncpy(dst, src, len); /* ASCII CP */
}

/*********************************************************************
 *		_mbschr(MSVCRT.@)
 *
 * Find a multibyte character in a multibyte string.
 */
char *_mbschr(const char *str, unsigned int c)
{
  if(MSVCRT___mb_cur_max > 1)
  {
    unsigned int next;
    while((next = _mbsnextc(str)))
    {
      if(next == c)
        return (char *)str;
      str += next > 255 ? 2 : 1;
    }
    return c ? NULL :(char *)str;
  }
  return strchr(str, c); /* ASCII CP */
}

/*********************************************************************
 *		_mbsnccnt(MSVCRT.@)
 */
unsigned int _mbsnccnt(const unsigned char *str, unsigned int len)
{
  int ret = 0;

  if(MSVCRT___mb_cur_max > 1)
  {
    while(*str && len-- > 0)
    {
      if(MSVCRT_isleadbyte(*str))
      {
        str++;
        len--;
      }
      ret++;
      str++;
    }
    return ret;
  }
  return min(strlen(str), len); /* ASCII CP */
}


/*********************************************************************
 *		_mbsncat(MSVCRT.@)
 */
char *_mbsncat(char *dst, const unsigned char *src, unsigned int len)
{
  if(MSVCRT___mb_cur_max > 1)
  {
    char *res = dst;
    dst += _mbslen(dst);
    while(*src && len--)
    {
      *dst = *src;
      if(MSVCRT_isleadbyte(*src))
        *++dst = *++src;
      dst++;
      src++;
    }
    *dst++ = '\0';
    return res;
  }
  return strncat(dst, src, len); /* ASCII CP */
}

