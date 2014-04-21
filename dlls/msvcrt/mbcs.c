/*
 * msvcrt.dll mbcs functions
 *
 * Copyright 1999 Alexandre Julliard
 * Copyright 2000 Jon Griffths
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
 *
 * FIXME
 * Not currently binary compatible with win32. MSVCRT_mbctype must be
 * populated correctly and the ismb* functions should reference it.
 */

#include <stdio.h>

#include "msvcrt.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

unsigned char MSVCRT_mbctype[257] = { 0 };

/* It seems that the data about valid trail bytes is not available from kernel32
 * so we have to store is here. The format is the same as for lead bytes in CPINFO */
struct cp_extra_info_t
{
    int cp;
    BYTE TrailBytes[MAX_LEADBYTES];
};

static struct cp_extra_info_t g_cpextrainfo[] =
{
    {932, {0x40, 0x7e, 0x80, 0xfc, 0, 0}},
    {936, {0x40, 0xfe, 0, 0}},
    {949, {0x41, 0xfe, 0, 0}},
    {950, {0x40, 0x7e, 0xa1, 0xfe, 0, 0}},
    {1361, {0x31, 0x7e, 0x81, 0xfe, 0, 0}},
    {20932, {1, 255, 0, 0}},  /* seems to give different results on different systems */
    {0, {1, 255, 0, 0}}       /* match all with FIXME */
};

/* Maps cp932 single byte character to multi byte character */
static const unsigned char mbbtombc_932[] = {
  0x40,0x49,0x68,0x94,0x90,0x93,0x95,0x66,0x69,0x6a,0x96,0x7b,0x43,0x7c,0x44,0x5e,
  0x4f,0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x46,0x47,0x83,0x81,0x84,0x48,
  0x97,0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,
  0x6f,0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x6d,0x8f,0x6e,0x4f,0x76,
  0x77,0x78,0x79,0x6d,0x8f,0x6e,0x4f,0x51,0x65,0x81,0x82,0x83,0x84,0x85,0x86,0x87,
  0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,0x90,0x91,0x92,0x93,0x94,0x95,0x50,
       0x42,0x75,0x76,0x41,0x45,0x92,0x40,0x42,0x44,0x46,0x48,0x83,0x85,0x87,0x62,
  0x5b,0x41,0x43,0x45,0x47,0x49,0x4a,0x4c,0x4e,0x50,0x52,0x54,0x56,0x58,0x5a,0x5c,
  0x5e,0x60,0x63,0x65,0x67,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x71,0x74,0x77,0x7a,0x7d,
  0x7e,0x80,0x81,0x82,0x84,0x86,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8f,0x93,0x4a,0x4b };

/* Maps multibyte cp932 punctuation marks to single byte equivalents */
static const unsigned char mbctombb_932_punct[] = {
  0x20,0xa4,0xa1,0x2c,0x2e,0xa5,0x3a,0x3b,0x3f,0x21,0xde,0xdf,0x00,0x00,0x00,0x5e,
  0x7e,0x5f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xb0,0x00,0x00,0x2f,0x00,
  0x00,0x00,0x7c,0x00,0x00,0x60,0x27,0x00,0x22,0x28,0x29,0x00,0x00,0x5b,0x5d,0x7b,
  0x7d,0x00,0x00,0x00,0x00,0xa2,0xa3,0x00,0x00,0x00,0x00,0x2b,0x2d,0x00,0x00,0x00,
  0x00,0x3d,0x00,0x3c,0x3e,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x5c,
  0x24,0x00,0x00,0x25,0x23,0x26,0x2a,0x40};

/* Maps multibyte cp932 hiragana/katakana to single-byte equivalents */
static const unsigned char mbctombb_932_kana[] = {
  0xa7,0xb1,0xa8,0xb2,0xa9,0xb3,0xaa,0xb4,0xab,0xb5,0xb6,0xb6,0xb7,0xb7,0xb8,0xb8,
  0xb9,0xb9,0xba,0xba,0xbb,0xbb,0xbc,0xbc,0xbd,0xbd,0xbe,0xbe,0xbf,0xbf,0xc0,0xc0,
  0xc1,0xc1,0xaf,0xc2,0xc2,0xc3,0xc3,0xc4,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xca,
  0xca,0xcb,0xcb,0xcb,0xcc,0xcc,0xcc,0xcd,0xcd,0xcd,0xce,0xce,0xce,0xcf,0xd0,0xd1,
  0xd2,0xd3,0xac,0xd4,0xad,0xd5,0xae,0xd6,0xd7,0xd8,0xd9,0xda,0xdb,0xdc,0xdc,0xb2,
  0xb4,0xa6,0xdd,0xb3,0xb6,0xb9};

static MSVCRT_wchar_t msvcrt_mbc_to_wc(unsigned int ch)
{
  MSVCRT_wchar_t chW;
  char mbch[2];
  int n_chars;

  if (ch <= 0xff) {
    mbch[0] = ch;
    n_chars = 1;
  } else {
    mbch[0] = (ch >> 8) & 0xff;
    mbch[1] = ch & 0xff;
    n_chars = 2;
  }
  if (!MultiByteToWideChar(get_mbcinfo()->mbcodepage, 0, mbch, n_chars, &chW, 1))
  {
    WARN("MultiByteToWideChar failed on %x\n", ch);
    return 0;
  }
  return chW;
}

static inline MSVCRT_size_t u_strlen( const unsigned char *str )
{
  return strlen( (const char*) str );
}

static inline unsigned char* u_strncat( unsigned char* dst, const unsigned char* src, MSVCRT_size_t len )
{
  return (unsigned char*)strncat( (char*)dst, (const char*)src, len);
}

static inline int u_strcmp( const unsigned char *s1, const unsigned char *s2 )
{
  return strcmp( (const char*)s1, (const char*)s2 );
}

static inline int u_strcasecmp( const unsigned char *s1, const unsigned char *s2 )
{
  return strcasecmp( (const char*)s1, (const char*)s2 );
}

static inline int u_strncmp( const unsigned char *s1, const unsigned char *s2, MSVCRT_size_t len )
{
  return strncmp( (const char*)s1, (const char*)s2, len );
}

static inline int u_strncasecmp( const unsigned char *s1, const unsigned char *s2, MSVCRT_size_t len )
{
  return strncasecmp( (const char*)s1, (const char*)s2, len );
}

static inline unsigned char *u_strchr( const unsigned char *s, unsigned char x )
{
  return (unsigned char*) strchr( (const char*)s, x );
}

static inline unsigned char *u_strrchr( const unsigned char *s, unsigned char x )
{
  return (unsigned char*) strrchr( (const char*)s, x );
}

static inline unsigned char *u__strset( unsigned char *s, unsigned char c )
{
  return (unsigned char*) _strset( (char*)s, c);
}

static inline unsigned char *u__strnset( unsigned char *s, unsigned char c, MSVCRT_size_t len )
{
  return (unsigned char*) MSVCRT__strnset( (char*)s, c, len );
}

static inline MSVCRT_size_t u_strcspn( const unsigned char *s, const unsigned char *rej )
{
  return strcspn( (const char *)s, (const char*)rej );
}

/*********************************************************************
 *		__p__mbctype (MSVCRT.@)
 */
unsigned char* CDECL __p__mbctype(void)
{
  return get_mbcinfo()->mbctype;
}

/*********************************************************************
 *		__p___mb_cur_max(MSVCRT.@)
 */
int* CDECL __p___mb_cur_max(void)
{
  return &get_locinfo()->mb_cur_max;
}

/*********************************************************************
 *		___mb_cur_max_func(MSVCRT.@)
 */
int CDECL MSVCRT____mb_cur_max_func(void)
{
  return get_locinfo()->mb_cur_max;
}

/* ___mb_cur_max_l_func - not exported in native msvcrt */
int* CDECL ___mb_cur_max_l_func(MSVCRT__locale_t locale)
{
  MSVCRT_pthreadlocinfo locinfo;

  if(!locale)
    locinfo = get_locinfo();
  else
    locinfo = locale->locinfo;

  return &locinfo->mb_cur_max;
}

/*********************************************************************
 * INTERNAL: _setmbcp_l
 */
int _setmbcp_l(int cp, LCID lcid, MSVCRT_pthreadmbcinfo mbcinfo)
{
  const char format[] = ".%d";

  int newcp;
  CPINFO cpi;
  BYTE *bytes;
  WORD chartypes[256];
  char bufA[256];
  WCHAR bufW[256];
  int charcount;
  int ret;
  int i;

  if(!mbcinfo)
      mbcinfo = get_mbcinfo();

  switch (cp)
  {
    case _MB_CP_ANSI:
      newcp = GetACP();
      break;
    case _MB_CP_OEM:
      newcp = GetOEMCP();
      break;
    case _MB_CP_LOCALE:
      newcp = get_locinfo()->lc_codepage;
      if(newcp)
          break;
      /* fall through (C locale) */
    case _MB_CP_SBCS:
      newcp = 20127;   /* ASCII */
      break;
    default:
      newcp = cp;
      break;
  }

  if(lcid == -1) {
    sprintf(bufA, format, newcp);
    mbcinfo->mblcid = MSVCRT_locale_to_LCID(bufA, NULL);
  } else {
    mbcinfo->mblcid = lcid;
  }

  if(mbcinfo->mblcid == -1)
  {
    WARN("Can't assign LCID to codepage (%d)\n", mbcinfo->mblcid);
    mbcinfo->mblcid = 0;
  }

  if (!GetCPInfo(newcp, &cpi))
  {
    WARN("Codepage %d not found\n", newcp);
    *MSVCRT__errno() = MSVCRT_EINVAL;
    return -1;
  }

  /* setup the _mbctype */
  memset(mbcinfo->mbctype, 0, sizeof(unsigned char[257]));
  memset(mbcinfo->mbcasemap, 0, sizeof(unsigned char[256]));

  bytes = cpi.LeadByte;
  while (bytes[0] || bytes[1])
  {
    for (i = bytes[0]; i <= bytes[1]; i++)
      mbcinfo->mbctype[i + 1] |= _M1;
    bytes += 2;
  }

  if (cpi.MaxCharSize > 1)
  {
    /* trail bytes not available through kernel32 but stored in a structure in msvcrt */
    struct cp_extra_info_t *cpextra = g_cpextrainfo;

    mbcinfo->ismbcodepage = 1;
    while (TRUE)
    {
      if (cpextra->cp == 0 || cpextra->cp == newcp)
      {
        if (cpextra->cp == 0)
          FIXME("trail bytes data not available for DBCS codepage %d - assuming all bytes\n", newcp);

        bytes = cpextra->TrailBytes;
        while (bytes[0] || bytes[1])
        {
          for (i = bytes[0]; i <= bytes[1]; i++)
            mbcinfo->mbctype[i + 1] |= _M2;
          bytes += 2;
        }
        break;
      }
      cpextra++;
    }
  }
  else
    mbcinfo->ismbcodepage = 0;

  /* we can't use GetStringTypeA directly because we don't have a locale - only a code page
   */
  charcount = 0;
  for (i = 0; i < 256; i++)
    if (!(mbcinfo->mbctype[i + 1] & _M1))
      bufA[charcount++] = i;

  ret = MultiByteToWideChar(newcp, 0, bufA, charcount, bufW, charcount);
  if (ret != charcount)
    ERR("MultiByteToWideChar of chars failed for cp %d, ret=%d (exp %d), error=%d\n", newcp, ret, charcount, GetLastError());

  GetStringTypeW(CT_CTYPE1, bufW, charcount, chartypes);

  charcount = 0;
  for (i = 0; i < 256; i++)
    if (!(mbcinfo->mbctype[i + 1] & _M1))
    {
      if (chartypes[charcount] & C1_UPPER)
      {
        mbcinfo->mbctype[i + 1] |= _SBUP;
        bufW[charcount] = tolowerW(bufW[charcount]);
      }
      else if (chartypes[charcount] & C1_LOWER)
      {
	mbcinfo->mbctype[i + 1] |= _SBLOW;
        bufW[charcount] = toupperW(bufW[charcount]);
      }
      charcount++;
    }

  ret = WideCharToMultiByte(newcp, 0, bufW, charcount, bufA, charcount, NULL, NULL);
  if (ret != charcount)
    ERR("WideCharToMultiByte failed for cp %d, ret=%d (exp %d), error=%d\n", newcp, ret, charcount, GetLastError());

  charcount = 0;
  for (i = 0; i < 256; i++)
  {
    if(!(mbcinfo->mbctype[i + 1] & _M1))
    {
      if(mbcinfo->mbctype[i] & (C1_UPPER|C1_LOWER))
        mbcinfo->mbcasemap[i] = bufA[charcount];
      charcount++;
    }
  }

  if (newcp == 932)   /* CP932 only - set _MP and _MS */
  {
    /* On Windows it's possible to calculate the _MP and _MS from CT_CTYPE1
     * and CT_CTYPE3. But as of Wine 0.9.43 we return wrong values what makes
     * it hard. As this is set only for codepage 932 we hardcode it what gives
     * also faster execution.
     */
    for (i = 161; i <= 165; i++)
      mbcinfo->mbctype[i + 1] |= _MP;
    for (i = 166; i <= 223; i++)
      mbcinfo->mbctype[i + 1] |= _MS;
  }

  mbcinfo->mbcodepage = newcp;
  if(MSVCRT_locale && mbcinfo == MSVCRT_locale->mbcinfo)
    memcpy(MSVCRT_mbctype, MSVCRT_locale->mbcinfo->mbctype, sizeof(MSVCRT_mbctype));

  return 0;
}

/*********************************************************************
 *              _setmbcp (MSVCRT.@)
 */
int CDECL _setmbcp(int cp)
{
    return _setmbcp_l(cp, -1, NULL);
}

/*********************************************************************
 *		_getmbcp (MSVCRT.@)
 */
int CDECL _getmbcp(void)
{
  return get_mbcinfo()->mbcodepage;
}

/*********************************************************************
 *		_mbsnextc(MSVCRT.@)
 */
unsigned int CDECL _mbsnextc(const unsigned char* str)
{
  if(_ismbblead(*str))
    return *str << 8 | str[1];
  return *str;
}

/*********************************************************************
 *		_mbctolower(MSVCRT.@)
 */
unsigned int CDECL _mbctolower(unsigned int c)
{
    if (_ismbblead(c))
    {
      FIXME("Handle MBC chars\n");
      return c;
    }
    return tolower(c); /* ASCII CP or SB char */
}

/*********************************************************************
 *		_mbctoupper(MSVCRT.@)
 */
unsigned int CDECL _mbctoupper(unsigned int c)
{
    if (_ismbblead(c))
    {
      FIXME("Handle MBC chars\n");
      return c;
    }
    return toupper(c); /* ASCII CP or SB char */
}

/*********************************************************************
 *		_mbctombb (MSVCRT.@)
 */
unsigned int CDECL _mbctombb(unsigned int c)
{
    unsigned int value;

    if(get_mbcinfo()->mbcodepage == 932)
    {
        if(c >= 0x829f && c <= 0x82f1)    /* Hiragana */
            return mbctombb_932_kana[c - 0x829f];
        if(c >= 0x8340 && c <= 0x8396 && c != 0x837f)    /* Katakana */
            return mbctombb_932_kana[c - 0x8340 - (c >= 0x837f ? 1 : 0)];
        if(c >= 0x8140 && c <= 0x8197)    /* Punctuation */
        {
            value = mbctombb_932_punct[c - 0x8140];
            return value ? value : c;
        }
        if((c >= 0x824f && c <= 0x8258) || /* Fullwidth digits */
           (c >= 0x8260 && c <= 0x8279))   /* Fullwidth capitals letters */
            return c - 0x821f;
        if(c >= 0x8281 && c <= 0x829a)     /* Fullwidth small letters */
            return c - 0x8220;
        /* all other cases return c */
    }
    return c;
}

/*********************************************************************
 *		_mbcjistojms(MSVCRT.@)
 *
 *		Converts a jis character to sjis.
 *		Based on description from
 *		http://www.slayers.ne.jp/~oouchi/code/jistosjis.html
 */
unsigned int CDECL _mbcjistojms(unsigned int c)
{
  /* Conversion takes place only when codepage is 932.
     In all other cases, c is returned unchanged */
  if(get_mbcinfo()->mbcodepage == 932)
  {
    if(HIBYTE(c) >= 0x21 && HIBYTE(c) <= 0x7e &&
       LOBYTE(c) >= 0x21 && LOBYTE(c) <= 0x7e)
    {
      if(HIBYTE(c) % 2)
        c += 0x1f;
      else
        c += 0x7d;

      if(LOBYTE(c) >= 0x7F)
        c += 0x1;

      c = (((HIBYTE(c) - 0x21)/2 + 0x81) << 8) | LOBYTE(c);

      if(HIBYTE(c) > 0x9f)
        c += 0x4000;
    }
    else
      return 0; /* Codepage is 932, but c can't be converted */
  }

  return c;
}

/*********************************************************************
 *		_mbcjmstojis(MSVCRT.@)
 *
 *		Converts a sjis character to jis.
 */
unsigned int CDECL _mbcjmstojis(unsigned int c)
{
  /* Conversion takes place only when codepage is 932.
     In all other cases, c is returned unchanged */
  if(get_mbcinfo()->mbcodepage == 932)
  {
    if(_ismbclegal(c) && HIBYTE(c) < 0xf0)
    {
      if(HIBYTE(c) >= 0xe0)
        c -= 0x4000;

      c = (((HIBYTE(c) - 0x81)*2 + 0x21) << 8) | LOBYTE(c);

      if(LOBYTE(c) > 0x7f)
        c -= 0x1;

      if(LOBYTE(c) > 0x9d)
        c += 0x83;
      else
        c -= 0x1f;
    }
    else
      return 0; /* Codepage is 932, but c can't be converted */
  }

  return c;
}

/*********************************************************************
 *		_mbsdec(MSVCRT.@)
 */
unsigned char* CDECL _mbsdec(const unsigned char* start, const unsigned char* cur)
{
  if(get_mbcinfo()->ismbcodepage)
    return (unsigned char *)(_ismbstrail(start,cur-1) ? cur - 2 : cur -1);

  return (unsigned char *)cur - 1; /* ASCII CP or SB char */
}

/*********************************************************************
 *		_mbclen(MSVCRT.@)
 */
unsigned int CDECL _mbclen(const unsigned char* str)
{
  return _ismbblead(*str) ? 2 : 1;
}

/*********************************************************************
 *		_mbsinc(MSVCRT.@)
 */
unsigned char* CDECL _mbsinc(const unsigned char* str)
{
  return (unsigned char *)(str + _mbclen(str));
}

/*********************************************************************
 *		_mbsninc(MSVCRT.@)
 */
unsigned char* CDECL _mbsninc(const unsigned char* str, MSVCRT_size_t num)
{
  if(!str)
    return NULL;

  while (num > 0 && *str)
  {
    if (_ismbblead(*str))
    {
      if (!*(str+1))
         break;
      str++;
    }
    str++;
    num--;
  }

  return (unsigned char*)str;
}

/*********************************************************************
 *		_mbslen(MSVCRT.@)
 */
MSVCRT_size_t CDECL _mbslen(const unsigned char* str)
{
  MSVCRT_size_t len = 0;
  while(*str)
  {
    if (_ismbblead(*str))
    {
      str++;
      if (!*str)  /* count only full chars */
        break;
    }
    str++;
    len++;
  }
  return len;
}

/*********************************************************************
 *		_mbccpy(MSVCRT.@)
 */
void CDECL _mbccpy(unsigned char* dest, const unsigned char* src)
{
  *dest = *src;
  if(_ismbblead(*src))
    *++dest = *++src; /* MB char */
}

/*********************************************************************
 *		_mbsncpy(MSVCRT.@)
 * REMARKS
 *  The parameter n is the number or characters to copy, not the size of
 *  the buffer. Use _mbsnbcpy for a function analogical to strncpy
 */
unsigned char* CDECL _mbsncpy(unsigned char* dst, const unsigned char* src, MSVCRT_size_t n)
{
  unsigned char* ret = dst;
  if(!n)
    return dst;
  if (get_mbcinfo()->ismbcodepage)
  {
    while (*src && n)
    {
      n--;
      if (_ismbblead(*src))
      {
        if (!*(src+1))
        {
            *dst++ = 0;
            *dst++ = 0;
            break;
        }

        *dst++ = *src++;
      }

      *dst++ = *src++;
    }
  }
  else
  {
    while (n)
    {
        n--;
        if (!(*dst++ = *src++)) break;
    }
  }
  while (n--) *dst++ = 0;
  return ret;
}

/*********************************************************************
 *              _mbsnbcpy_s(MSVCRT.@)
 * REMARKS
 * Unlike _mbsnbcpy this function does not pad the rest of the dest
 * string with 0
 */
int CDECL _mbsnbcpy_s(unsigned char* dst, MSVCRT_size_t size, const unsigned char* src, MSVCRT_size_t n)
{
    MSVCRT_size_t pos = 0;

    if(!dst || size == 0)
        return MSVCRT_EINVAL;
    if(!src)
    {
        dst[0] = '\0';
        return MSVCRT_EINVAL;
    }
    if(!n)
        return 0;

    if(get_mbcinfo()->ismbcodepage)
    {
        int is_lead = 0;
        while (*src && n)
        {
            if(pos == size)
            {
                dst[0] = '\0';
                return MSVCRT_ERANGE;
            }
            is_lead = (!is_lead && _ismbblead(*src));
            n--;
            dst[pos++] = *src++;
        }

        if (is_lead) /* if string ends with a lead, remove it */
            dst[pos - 1] = 0;
    }
    else
    {
        while (n)
        {
            n--;
            if(pos == size)
            {
                dst[0] = '\0';
                return MSVCRT_ERANGE;
            }

            if(!(*src)) break;
            dst[pos++] = *src++;
        }
    }

    if(pos < size)
        dst[pos] = '\0';
    else
    {
        dst[0] = '\0';
        return MSVCRT_ERANGE;
    }

    return 0;
}

/*********************************************************************
 *              _mbsnbcpy(MSVCRT.@)
 * REMARKS
 *  Like strncpy this function doesn't enforce the string to be
 *  NUL-terminated
 */
unsigned char* CDECL _mbsnbcpy(unsigned char* dst, const unsigned char* src, MSVCRT_size_t n)
{
  unsigned char* ret = dst;
  if(!n)
    return dst;
  if(get_mbcinfo()->ismbcodepage)
  {
    int is_lead = 0;
    while (*src && n)
    {
      is_lead = (!is_lead && _ismbblead(*src));
      n--;
      *dst++ = *src++;
    }

    if (is_lead) /* if string ends with a lead, remove it */
	*(dst - 1) = 0;
  }
  else
  {
    while (n)
    {
        n--;
        if (!(*dst++ = *src++)) break;
    }
  }
  while (n--) *dst++ = 0;
  return ret;
}

/*********************************************************************
 *		_mbscmp(MSVCRT.@)
 */
int CDECL _mbscmp(const unsigned char* str, const unsigned char* cmp)
{
  if(get_mbcinfo()->ismbcodepage)
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
  return u_strcmp(str, cmp); /* ASCII CP */
}

/*********************************************************************
 *              _mbsnbicoll_l(MSVCRT.@)
 */
int CDECL _mbsnbicoll_l(const unsigned char *str1, const unsigned char *str2, MSVCRT_size_t len, MSVCRT__locale_t locale)
{
    MSVCRT_pthreadmbcinfo mbcinfo;

    if(!locale)
        mbcinfo = get_mbcinfo();
    else
        mbcinfo = locale->mbcinfo;

    if(!mbcinfo->ismbcodepage)
        return MSVCRT__strnicoll_l((const char*)str1, (const char*)str2, len, locale);
    return CompareStringA(mbcinfo->mblcid, NORM_IGNORECASE, (const char*)str1, len, (const char*)str2, len)-CSTR_EQUAL;
}

/*********************************************************************
 *              _mbsicoll_l(MSVCRT.@)
 */
int CDECL _mbsicoll_l(const unsigned char *str1, const unsigned char *str2, MSVCRT__locale_t locale)
{
    return _mbsnbicoll_l(str1, str2, -1, locale);
}

/*********************************************************************
 *              _mbsnbicoll(MSVCRT.@)
 */
int CDECL _mbsnbicoll(const unsigned char *str1, const unsigned char *str2, MSVCRT_size_t len)
{
    return _mbsnbicoll_l(str1, str2, len, NULL);
}

/*********************************************************************
 *		_mbsicoll(MSVCRT.@)
 */
int CDECL _mbsicoll(const unsigned char* str, const unsigned char* cmp)
{
    return _mbsnbicoll_l(str, cmp, -1, NULL);
}

/*********************************************************************
 *              _mbsnbcoll_l(MSVCRT.@)
 */
int CDECL _mbsnbcoll_l(const unsigned char *str1, const unsigned char *str2, MSVCRT_size_t len, MSVCRT__locale_t locale)
{
    MSVCRT_pthreadmbcinfo mbcinfo;

    if(!locale)
        mbcinfo = get_mbcinfo();
    else
        mbcinfo = locale->mbcinfo;

    if(!mbcinfo->ismbcodepage)
        return MSVCRT__strncoll_l((const char*)str1, (const char*)str2, len, locale);
    return CompareStringA(mbcinfo->mblcid, 0, (const char*)str1, len, (const char*)str2, len)-CSTR_EQUAL;
}

/*********************************************************************
 *              _mbscoll_l(MSVCRT.@)
 */
int CDECL _mbscoll_l(const unsigned char *str1, const unsigned char *str2, MSVCRT__locale_t locale)
{
    return _mbsnbcoll_l(str1, str2, -1, locale);
}

/*********************************************************************
 *              _mbsnbcoll(MSVCRT.@)
 */
int CDECL _mbsnbcoll(const unsigned char *str1, const unsigned char *str2, MSVCRT_size_t len)
{
    return _mbsnbcoll_l(str1, str2, len, NULL);
}

/*********************************************************************
 *		_mbscoll(MSVCRT.@)
 */
int CDECL _mbscoll(const unsigned char* str, const unsigned char* cmp)
{
    return _mbsnbcoll_l(str, cmp, -1, NULL);
}

/*********************************************************************
 *		_mbsicmp(MSVCRT.@)
 */
int CDECL _mbsicmp(const unsigned char* str, const unsigned char* cmp)
{
  if(get_mbcinfo()->ismbcodepage)
  {
    unsigned int strc, cmpc;
    do {
      if(!*str)
        return *cmp ? -1 : 0;
      if(!*cmp)
        return 1;
      strc = _mbctolower(_mbsnextc(str));
      cmpc = _mbctolower(_mbsnextc(cmp));
      if(strc != cmpc)
        return strc < cmpc ? -1 : 1;
      str +=(strc > 255) ? 2 : 1;
      cmp +=(strc > 255) ? 2 : 1; /* equal, use same increment */
    } while(1);
  }
  return u_strcasecmp(str, cmp); /* ASCII CP */
}

/*********************************************************************
 *		_mbsncmp(MSVCRT.@)
 */
int CDECL _mbsncmp(const unsigned char* str, const unsigned char* cmp, MSVCRT_size_t len)
{
  if(!len)
    return 0;

  if(get_mbcinfo()->ismbcodepage)
  {
    unsigned int strc, cmpc;
    while(len--)
    {
      int inc;
      if(!*str)
        return *cmp ? -1 : 0;
      if(!*cmp)
        return 1;
      strc = _mbsnextc(str);
      cmpc = _mbsnextc(cmp);
      if(strc != cmpc)
        return strc < cmpc ? -1 : 1;
      inc=(strc > 255) ? 2 : 1; /* Equal, use same increment */
      str += inc;
      cmp += inc;
    }
    return 0; /* Matched len chars */
  }
  return u_strncmp(str, cmp, len); /* ASCII CP */
}

/*********************************************************************
 *              _mbsnbcmp(MSVCRT.@)
 */
int CDECL _mbsnbcmp(const unsigned char* str, const unsigned char* cmp, MSVCRT_size_t len)
{
  if (!len)
    return 0;
  if(get_mbcinfo()->ismbcodepage)
  {
    unsigned int strc, cmpc;
    while (len)
    {
      int clen;
      if(!*str)
        return *cmp ? -1 : 0;
      if(!*cmp)
        return 1;
      if (_ismbblead(*str))
      {
        strc=(len>=2)?_mbsnextc(str):0;
        clen=2;
      }
      else
      {
        strc=*str;
        clen=1;
      }
      if (_ismbblead(*cmp))
        cmpc=(len>=2)?_mbsnextc(cmp):0;
      else
        cmpc=*cmp;
      if(strc != cmpc)
        return strc < cmpc ? -1 : 1;
      len -= clen;
      str += clen;
      cmp += clen;
    }
    return 0; /* Matched len chars */
  }
  return u_strncmp(str,cmp,len);
}

/*********************************************************************
 *		_mbsnicmp(MSVCRT.@)
 *
 * Compare two multibyte strings case insensitively to 'len' characters.
 */
int CDECL _mbsnicmp(const unsigned char* str, const unsigned char* cmp, MSVCRT_size_t len)
{
  /* FIXME: No tolower() for mb strings yet */
  if(get_mbcinfo()->ismbcodepage)
  {
    unsigned int strc, cmpc;
    while(len--)
    {
      if(!*str)
        return *cmp ? -1 : 0;
      if(!*cmp)
        return 1;
      strc = _mbctolower(_mbsnextc(str));
      cmpc = _mbctolower(_mbsnextc(cmp));
      if(strc != cmpc)
        return strc < cmpc ? -1 : 1;
      str +=(strc > 255) ? 2 : 1;
      cmp +=(strc > 255) ? 2 : 1; /* Equal, use same increment */
    }
    return 0; /* Matched len chars */
  }
  return u_strncasecmp(str, cmp, len); /* ASCII CP */
}

/*********************************************************************
 *              _mbsnbicmp(MSVCRT.@)
 */
int CDECL _mbsnbicmp(const unsigned char* str, const unsigned char* cmp, MSVCRT_size_t len)
{
  if (!len)
    return 0;
  if(get_mbcinfo()->ismbcodepage)
  {
    unsigned int strc, cmpc;
    while (len)
    {
      int clen;
      if(!*str)
        return *cmp ? -1 : 0;
      if(!*cmp)
        return 1;
      if (_ismbblead(*str))
      {
        strc=(len>=2)?_mbsnextc(str):0;
        clen=2;
      }
      else
      {
        strc=*str;
        clen=1;
      }
      if (_ismbblead(*cmp))
        cmpc=(len>=2)?_mbsnextc(cmp):0;
      else
        cmpc=*cmp;
      strc = _mbctolower(strc);
      cmpc = _mbctolower(cmpc);
      if(strc != cmpc)
        return strc < cmpc ? -1 : 1;
      len -= clen;
      str += clen;
      cmp += clen;
    }
    return 0; /* Matched len bytes */
  }
  return u_strncasecmp(str,cmp,len);
}

/*********************************************************************
 *		_mbscat (MSVCRT.@)
 */
unsigned char * CDECL _mbscat( unsigned char *dst, const unsigned char *src )
{
    strcat( (char *)dst, (const char *)src );
    return dst;
}

/*********************************************************************
 *		_mbscpy (MSVCRT.@)
 */
unsigned char* CDECL _mbscpy( unsigned char *dst, const unsigned char *src )
{
    strcpy( (char *)dst, (const char *)src );
    return dst;
}

/*********************************************************************
 *		_mbsstr (MSVCRT.@)
 */
unsigned char * CDECL _mbsstr(const unsigned char *haystack, const unsigned char *needle)
{
    return (unsigned char *)strstr( (const char *)haystack, (const char *)needle );
}

/*********************************************************************
 *		_mbschr(MSVCRT.@)
 *
 * Find a multibyte character in a multibyte string.
 */
unsigned char* CDECL _mbschr(const unsigned char* s, unsigned int x)
{
  if(get_mbcinfo()->ismbcodepage)
  {
    unsigned int c;
    while (1)
    {
      c = _mbsnextc(s);
      if (c == x)
        return (unsigned char*)s;
      if (!c)
        return NULL;
      s += c > 255 ? 2 : 1;
    }
  }
  return u_strchr(s, x); /* ASCII CP */
}

/*********************************************************************
 *		_mbsrchr(MSVCRT.@)
 */
unsigned char* CDECL _mbsrchr(const unsigned char* s, unsigned int x)
{
  if(get_mbcinfo()->ismbcodepage)
  {
    unsigned int c;
    unsigned char* match=NULL;
    if(!s)
      return NULL;
    while (1) {
      c = _mbsnextc(s);
      if (c == x)
        match=(unsigned char*)s;
      if (!c)
        return match;
      s +=(c > 255) ? 2 : 1;
    }
  }
  return u_strrchr(s, x);
}

/*********************************************************************
 *              _mbstok_s_l(MSVCRT.@)
 */
unsigned char* CDECL _mbstok_s_l(unsigned char *str, const unsigned char *delim,
        unsigned char **ctx, MSVCRT__locale_t locale)
{
    MSVCRT_pthreadmbcinfo mbcinfo;
    unsigned int c;

    if(!MSVCRT_CHECK_PMT(delim != NULL)) return NULL;
    if(!MSVCRT_CHECK_PMT(ctx != NULL)) return NULL;
    if(!MSVCRT_CHECK_PMT(str || *ctx)) return NULL;

    if(locale)
        mbcinfo = locale->mbcinfo;
    else
        mbcinfo = get_mbcinfo();

    if(!mbcinfo->ismbcodepage)
        return (unsigned char*)MSVCRT_strtok_s((char*)str, (const char*)delim, (char**)ctx);

    if(!str)
        str = *ctx;

    while((c=_mbsnextc(str)) && _mbschr(delim, c))
        str += c>255 ? 2 : 1;
    if(!*str)
        return NULL;

    *ctx = str + (c>255 ? 2 : 1);
    while((c=_mbsnextc(*ctx)) && !_mbschr(delim, c))
        *ctx += c>255 ? 2 : 1;
    if (**ctx) {
        *(*ctx)++ = 0;
        if(c > 255)
            *(*ctx)++ = 0;
    }

    return str;
}


/*********************************************************************
 *              _mbstok_s(MSVCRT.@)
 */
unsigned char* CDECL _mbstok_s(unsigned char *str,
        const unsigned char *delim, unsigned char **ctx)
{
    return _mbstok_s_l(str, delim, ctx, NULL);
}

/*********************************************************************
 *              _mbstok_l(MSVCRT.@)
 */
unsigned char* CDECL _mbstok_l(unsigned char *str,
        const unsigned char *delim, MSVCRT__locale_t locale)
{
    return _mbstok_s_l(str, delim, &msvcrt_get_thread_data()->mbstok_next, locale);
}

/*********************************************************************
 *		_mbstok(MSVCRT.@)
 */
unsigned char* CDECL _mbstok(unsigned char *str, const unsigned char *delim)
{
    thread_data_t *data = msvcrt_get_thread_data();

#if _MSVCR_VER == 0
    if(!str && !data->mbstok_next)
        return NULL;
#endif

    return _mbstok_s_l(str, delim, &data->mbstok_next, NULL);
}

/*********************************************************************
 *		_mbbtombc(MSVCRT.@)
 */
unsigned int CDECL _mbbtombc(unsigned int c)
{
  if(get_mbcinfo()->mbcodepage == 932)
  {
    if(c >= 0x20 && c <= 0x7e) {
      if((c >= 0x41 && c <= 0x5a) || (c >= 0x61 && c <= 0x7a) || (c >= 0x30 && c <= 0x39))
        return mbbtombc_932[c - 0x20] | 0x8200;
      else
        return mbbtombc_932[c - 0x20] | 0x8100;
    }
    else if(c >= 0xa1 && c <= 0xdf) {
      if(c >= 0xa6 && c <= 0xdd && c != 0xb0)
        return mbbtombc_932[c - 0xa1 + 0x5f] | 0x8300;
      else
        return mbbtombc_932[c - 0xa1 + 0x5f] | 0x8100;
    }
  }
  return c;  /* not Japanese or no MB char */
}

/*********************************************************************
 *		_mbbtype(MSVCRT.@)
 */
int CDECL _mbbtype(unsigned char c, int type)
{
    if (type == 1)
    {
        if ((c >= 0x20 && c <= 0x7e) || (c >= 0xa1 && c <= 0xdf))
            return _MBC_SINGLE;
        else if ((c >= 0x40 && c <= 0x7e) || (c >= 0x80 && c <= 0xfc))
            return _MBC_TRAIL;
        else
            return _MBC_ILLEGAL;
    }
    else
    {
        if ((c >= 0x20 && c <= 0x7e) || (c >= 0xa1 && c <= 0xdf))
            return _MBC_SINGLE;
        else if ((c >= 0x81 && c <= 0x9f) || (c >= 0xe0 && c <= 0xfc))
            return _MBC_LEAD;
        else
            return _MBC_ILLEGAL;
    }
}

/*********************************************************************
 *		_ismbbkana(MSVCRT.@)
 */
int CDECL _ismbbkana(unsigned int c)
{
  /* FIXME: use lc_ctype when supported, not lc_all */
  if(get_mbcinfo()->mbcodepage == 932)
  {
    /* Japanese/Katakana, CP 932 */
    return (c >= 0xa1 && c <= 0xdf);
  }
  return 0;
}

/*********************************************************************
 *              _ismbcdigit(MSVCRT.@)
 */
int CDECL _ismbcdigit(unsigned int ch)
{
    MSVCRT_wchar_t wch = msvcrt_mbc_to_wc( ch );
    return (get_char_typeW( wch ) & C1_DIGIT);
}

/*********************************************************************
 *              _ismbcgraph(MSVCRT.@)
 */
int CDECL _ismbcgraph(unsigned int ch)
{
    MSVCRT_wchar_t wch = msvcrt_mbc_to_wc( ch );
    return (get_char_typeW( wch ) & (C1_UPPER | C1_LOWER | C1_DIGIT | C1_PUNCT | C1_ALPHA));
}

/*********************************************************************
 *              _ismbcalpha (MSVCRT.@)
 */
int CDECL _ismbcalpha(unsigned int ch)
{
    MSVCRT_wchar_t wch = msvcrt_mbc_to_wc( ch );
    return (get_char_typeW( wch ) & C1_ALPHA);
}

/*********************************************************************
 *              _ismbclower (MSVCRT.@)
 */
int CDECL _ismbclower(unsigned int ch)
{
    MSVCRT_wchar_t wch = msvcrt_mbc_to_wc( ch );
    return (get_char_typeW( wch ) & C1_UPPER);
}

/*********************************************************************
 *              _ismbcupper (MSVCRT.@)
 */
int CDECL _ismbcupper(unsigned int ch)
{
    MSVCRT_wchar_t wch = msvcrt_mbc_to_wc( ch );
    return (get_char_typeW( wch ) & C1_LOWER);
}

/*********************************************************************
 *              _ismbcsymbol(MSVCRT.@)
 */
int CDECL _ismbcsymbol(unsigned int ch)
{
    MSVCRT_wchar_t wch = msvcrt_mbc_to_wc( ch );
    WORD ctype;
    if (!GetStringTypeW(CT_CTYPE3, &wch, 1, &ctype))
    {
        WARN("GetStringTypeW failed on %x\n", ch);
        return 0;
    }
    return ((ctype & C3_SYMBOL) != 0);
}

/*********************************************************************
 *              _ismbcalnum (MSVCRT.@)
 */
int CDECL _ismbcalnum(unsigned int ch)
{
    MSVCRT_wchar_t wch = msvcrt_mbc_to_wc( ch );
    return (get_char_typeW( wch ) & (C1_ALPHA | C1_DIGIT));
}

/*********************************************************************
 *              _ismbcspace (MSVCRT.@)
 */
int CDECL _ismbcspace(unsigned int ch)
{
    MSVCRT_wchar_t wch = msvcrt_mbc_to_wc( ch );
    return (get_char_typeW( wch ) & C1_SPACE);
}

/*********************************************************************
 *              _ismbcprint (MSVCRT.@)
 */
int CDECL _ismbcprint(unsigned int ch)
{
    MSVCRT_wchar_t wch = msvcrt_mbc_to_wc( ch );
    return (get_char_typeW( wch ) & (C1_UPPER | C1_LOWER | C1_DIGIT | C1_PUNCT | C1_ALPHA | C1_SPACE));
}

/*********************************************************************
 *              _ismbcpunct(MSVCRT.@)
 */
int CDECL _ismbcpunct(unsigned int ch)
{
    MSVCRT_wchar_t wch = msvcrt_mbc_to_wc( ch );
    return (get_char_typeW( wch ) & C1_PUNCT);
}

/*********************************************************************
 *		_ismbchira(MSVCRT.@)
 */
int CDECL _ismbchira(unsigned int c)
{
  /* FIXME: use lc_ctype when supported, not lc_all */
  if(get_mbcinfo()->mbcodepage == 932)
  {
    /* Japanese/Hiragana, CP 932 */
    return (c >= 0x829f && c <= 0x82f1);
  }
  return 0;
}

/*********************************************************************
 *		_ismbckata(MSVCRT.@)
 */
int CDECL _ismbckata(unsigned int c)
{
  /* FIXME: use lc_ctype when supported, not lc_all */
  if(get_mbcinfo()->mbcodepage == 932)
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
int CDECL _ismbblead(unsigned int c)
{
  return (get_mbcinfo()->mbctype[(c&0xff) + 1] & _M1) != 0;
}


/*********************************************************************
 *		_ismbbtrail(MSVCRT.@)
 */
int CDECL _ismbbtrail(unsigned int c)
{
  return (get_mbcinfo()->mbctype[(c&0xff) + 1] & _M2) != 0;
}

/*********************************************************************
 *              _ismbclegal(MSVCRT.@)
 */
int CDECL _ismbclegal(unsigned int c)
{
    return _ismbblead(HIBYTE(c)) && _ismbbtrail(LOBYTE(c));
}

/*********************************************************************
 *		_ismbslead(MSVCRT.@)
 */
int CDECL _ismbslead(const unsigned char* start, const unsigned char* str)
{
  int lead = 0;

  if(!get_mbcinfo()->ismbcodepage)
    return 0;

  /* Lead bytes can also be trail bytes so we need to analyse the string
   */
  while (start <= str)
  {
    if (!*start)
      return 0;
    lead = !lead && _ismbblead(*start);
    start++;
  }

  return lead ? -1 : 0;
}

/*********************************************************************
 *		_ismbstrail(MSVCRT.@)
 */
int CDECL _ismbstrail(const unsigned char* start, const unsigned char* str)
{
  /* Note: this function doesn't check _ismbbtrail */
  if ((str > start) && _ismbslead(start, str-1))
    return -1;
  else
    return 0;
}

/*********************************************************************
 *		_mbsbtype (MSVCRT.@)
 */
int CDECL _mbsbtype(const unsigned char *str, MSVCRT_size_t count)
{
  int lead = 0;
  const unsigned char *end = str + count;

  /* Lead bytes can also be trail bytes so we need to analyse the string.
   * Also we must return _MBC_ILLEGAL for chars past the end of the string
   */
  while (str < end) /* Note: we skip the last byte - will check after the loop */
  {
    if (!*str)
      return _MBC_ILLEGAL;
    lead = get_mbcinfo()->ismbcodepage && !lead && _ismbblead(*str);
    str++;
  }

  if (lead)
    if (_ismbbtrail(*str))
      return _MBC_TRAIL;
    else
      return _MBC_ILLEGAL;
  else
    if (_ismbblead(*str))
      return _MBC_LEAD;
    else
      return _MBC_SINGLE;
}

/*********************************************************************
 *		_mbsset(MSVCRT.@)
 */
unsigned char* CDECL _mbsset(unsigned char* str, unsigned int c)
{
  unsigned char* ret = str;

  if(!get_mbcinfo()->ismbcodepage || c < 256)
    return u__strset(str, c); /* ASCII CP or SB char */

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
 *		_mbsnbset(MSVCRT.@)
 */
unsigned char* CDECL _mbsnbset(unsigned char *str, unsigned int c, MSVCRT_size_t len)
{
    unsigned char *ret = str;

    if(!len)
	return ret;

    if(!get_mbcinfo()->ismbcodepage || c < 256)
	return u__strnset(str, c, len); /* ASCII CP or SB char */

    c &= 0xffff; /* Strip high bits */

    while(str[0] && str[1] && (len > 1))
    {
	*str++ = c >> 8;
	len--;
	*str++ = c & 0xff;
	len--;
    }
    if(len && str[0]) {
	/* as per msdn pad with a blank character */
	str[0] = ' ';
    }

    return ret;
}

/*********************************************************************
 *		_mbsnset(MSVCRT.@)
 */
unsigned char* CDECL _mbsnset(unsigned char* str, unsigned int c, MSVCRT_size_t len)
{
  unsigned char *ret = str;

  if(!len)
    return ret;

  if(!get_mbcinfo()->ismbcodepage || c < 256)
    return u__strnset(str, c, len); /* ASCII CP or SB char */

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
 *		_mbsnccnt(MSVCRT.@)
 * 'c' is for 'character'.
 */
MSVCRT_size_t CDECL _mbsnccnt(const unsigned char* str, MSVCRT_size_t len)
{
  MSVCRT_size_t ret;
  if(get_mbcinfo()->ismbcodepage)
  {
    ret=0;
    while(*str && len-- > 0)
    {
      if(_ismbblead(*str))
      {
        if (!len)
          break;
        len--;
        str++;
      }
      str++;
      ret++;
    }
    return ret;
  }
  ret=u_strlen(str);
  return min(ret, len); /* ASCII CP */
}

/*********************************************************************
 *		_mbsnbcnt(MSVCRT.@)
 * 'b' is for byte count.
 */
MSVCRT_size_t CDECL _mbsnbcnt(const unsigned char* str, MSVCRT_size_t len)
{
  MSVCRT_size_t ret;
  if(get_mbcinfo()->ismbcodepage)
  {
    const unsigned char* xstr = str;
    while(*xstr && len-- > 0)
    {
      if (_ismbblead(*xstr++))
        xstr++;
    }
    return xstr-str;
  }
  ret=u_strlen(str);
  return min(ret, len); /* ASCII CP */
}

/*********************************************************************
 *		_mbsnbcat(MSVCRT.@)
 */
unsigned char* CDECL _mbsnbcat(unsigned char* dst, const unsigned char* src, MSVCRT_size_t len)
{
    if(get_mbcinfo()->ismbcodepage)
    {
        unsigned char *res = dst;
        while (*dst) {
	    if (_ismbblead(*dst++)) {
		if (*dst) {
		    dst++;
		} else {
		    /* as per msdn overwrite the lead byte in front of '\0' */
		    dst--;
		    break;
		}
	    }
	}
        while (*src && len--) *dst++ = *src++;
        *dst = '\0';
        return res;
    }
    return u_strncat(dst, src, len); /* ASCII CP */
}

int CDECL _mbsnbcat_s(unsigned char *dst, MSVCRT_size_t size, const unsigned char *src, MSVCRT_size_t len)
{
    unsigned char *ptr = dst;
    MSVCRT_size_t i;

    if (!dst && !size && !src && !len)
        return 0;

    if (!dst || !size || !src)
    {
        if (dst && size)
            *dst = '\0';

        *MSVCRT__errno() = MSVCRT_EINVAL;
        return MSVCRT_EINVAL;
    }

    /* Find the null terminator of the destination buffer. */
    while (size && *ptr)
        size--, ptr++;

    if (!size)
    {
        *dst = '\0';
        *MSVCRT__errno() = MSVCRT_EINVAL;
        return MSVCRT_EINVAL;
    }

    /* If necessary, check that the character preceding the null terminator is
     * a lead byte and move the pointer back by one for later overwrite. */
    if (ptr != dst && get_mbcinfo()->ismbcodepage && _ismbblead(*(ptr - 1)))
        size++, ptr--;

    for (i = 0; *src && i < len; i++)
    {
        *ptr++ = *src++;
        size--;

        if (!size)
        {
            *dst = '\0';
            *MSVCRT__errno() = MSVCRT_ERANGE;
            return MSVCRT_ERANGE;
        }
    }

    *ptr = '\0';
    return 0;
}

/*********************************************************************
 *		_mbsncat(MSVCRT.@)
 */
unsigned char* CDECL _mbsncat(unsigned char* dst, const unsigned char* src, MSVCRT_size_t len)
{
  if(get_mbcinfo()->ismbcodepage)
  {
    unsigned char *res = dst;
    while (*dst)
    {
      if (_ismbblead(*dst++))
        dst++;
    }
    while (*src && len--)
    {
      *dst++ = *src;
      if(_ismbblead(*src++))
        *dst++ = *src++;
    }
    *dst = '\0';
    return res;
  }
  return u_strncat(dst, src, len); /* ASCII CP */
}


/*********************************************************************
 *              _mbslwr(MSVCRT.@)
 */
unsigned char* CDECL _mbslwr(unsigned char* s)
{
  unsigned char *ret = s;
  if (!s)
    return NULL;
  if (get_mbcinfo()->ismbcodepage)
  {
    unsigned int c;
    while (*s)
    {
      c = _mbctolower(_mbsnextc(s));
      /* Note that I assume that the size of the character is unchanged */
      if (c > 255)
      {
          *s++=(c>>8);
          c=c & 0xff;
      }
      *s++=c;
    }
  }
  else for ( ; *s; s++) *s = tolower(*s);
  return ret;
}


/*********************************************************************
 *              _mbslwr_s(MSVCRT.@)
 */
int CDECL _mbslwr_s(unsigned char* s, MSVCRT_size_t len)
{
  if (!s && !len)
  {
    return 0;
  }
  else if (!s || !len)
  {
    *MSVCRT__errno() = MSVCRT_EINVAL;
    return MSVCRT_EINVAL;
  }
  if (get_mbcinfo()->ismbcodepage)
  {
    unsigned int c;
    for ( ; *s && len > 0; len--)
    {
      c = _mbctolower(_mbsnextc(s));
      /* Note that I assume that the size of the character is unchanged */
      if (c > 255)
      {
          *s++=(c>>8);
          c=c & 0xff;
      }
      *s++=c;
    }
  }
  else for ( ; *s && len > 0; s++, len--) *s = tolower(*s);
  if (*s)
  {
    *s = '\0';
    *MSVCRT__errno() = MSVCRT_EINVAL;
    return MSVCRT_EINVAL;
  }
  return 0;
}


/*********************************************************************
 *              _mbsupr(MSVCRT.@)
 */
unsigned char* CDECL _mbsupr(unsigned char* s)
{
  unsigned char *ret = s;
  if (!s)
    return NULL;
  if (get_mbcinfo()->ismbcodepage)
  {
    unsigned int c;
    while (*s)
    {
      c = _mbctoupper(_mbsnextc(s));
      /* Note that I assume that the size of the character is unchanged */
      if (c > 255)
      {
          *s++=(c>>8);
          c=c & 0xff;
      }
      *s++=c;
    }
  }
  else for ( ; *s; s++) *s = toupper(*s);
  return ret;
}


/*********************************************************************
 *              _mbsupr_s(MSVCRT.@)
 */
int CDECL _mbsupr_s(unsigned char* s, MSVCRT_size_t len)
{
  if (!s && !len)
  {
    return 0;
  }
  else if (!s || !len)
  {
    *MSVCRT__errno() = MSVCRT_EINVAL;
    return MSVCRT_EINVAL;
  }
  if (get_mbcinfo()->ismbcodepage)
  {
    unsigned int c;
    for ( ; *s && len > 0; len--)
    {
      c = _mbctoupper(_mbsnextc(s));
      /* Note that I assume that the size of the character is unchanged */
      if (c > 255)
      {
          *s++=(c>>8);
          c=c & 0xff;
      }
      *s++=c;
    }
  }
  else for ( ; *s && len > 0; s++, len--) *s = toupper(*s);
  if (*s)
  {
    *s = '\0';
    *MSVCRT__errno() = MSVCRT_EINVAL;
    return MSVCRT_EINVAL;
  }
  return 0;
}


/*********************************************************************
 *              _mbsspn (MSVCRT.@)
 */
MSVCRT_size_t CDECL _mbsspn(const unsigned char* string, const unsigned char* set)
{
    const unsigned char *p, *q;

    for (p = string; *p; p++)
    {
        if (_ismbblead(*p))
        {
            for (q = set; *q; q++)
            {
                if (!q[1])
                    break;
                if ((*p == *q) &&  (p[1] == q[1]))
                    break;
                q++;
            }
            if (!q[0] || !q[1]) break;
        }
        else
        {
            for (q = set; *q; q++)
                if (*p == *q)
                    break;
            if (!*q) break;
        }
    }
    return p - string;
}

/*********************************************************************
 *              _mbsspnp (MSVCRT.@)
 */
unsigned char* CDECL _mbsspnp(const unsigned char* string, const unsigned char* set)
{
    const unsigned char *p, *q;

    for (p = string; *p; p++)
    {
        if (_ismbblead(*p))
        {
            for (q = set; *q; q++)
            {
                if (!q[1])
                    break;
                if ((*p == *q) &&  (p[1] == q[1]))
                    break;
                q++;
            }
            if (!q[0] || !q[1]) break;
        }
        else
        {
            for (q = set; *q; q++)
                if (*p == *q)
                    break;
            if (!*q) break;
        }
    }
    if (*p == '\0')
        return NULL;
    return (unsigned char *)p;
}

/*********************************************************************
 *		_mbscspn(MSVCRT.@)
 */
MSVCRT_size_t CDECL _mbscspn(const unsigned char* str, const unsigned char* cmp)
{
  if (get_mbcinfo()->ismbcodepage)
    FIXME("don't handle double character case\n");
  return u_strcspn(str, cmp);
}

/*********************************************************************
 *              _mbsrev (MSVCRT.@)
 */
unsigned char* CDECL _mbsrev(unsigned char* str)
{
    int i, len = _mbslen(str);
    unsigned char *p, *temp=MSVCRT_malloc(len*2);

    if(!temp)
        return str;

    /* unpack multibyte string to temp buffer */
    p=str;
    for(i=0; i<len; i++)
    {
        if (_ismbblead(*p))
        {
            temp[i*2]=*p++;
            temp[i*2+1]=*p++;
        }
        else
        {
            temp[i*2]=*p++;
            temp[i*2+1]=0;
        }
    }

    /* repack it in the reverse order */
    p=str;
    for(i=len-1; i>=0; i--)
    {
        if(_ismbblead(temp[i*2]))
        {
            *p++=temp[i*2];
            *p++=temp[i*2+1];
        }
        else
        {
            *p++=temp[i*2];
        }
    }

    MSVCRT_free(temp);

    return str;
}

/*********************************************************************
 *		_mbspbrk (MSVCRT.@)
 */
unsigned char* CDECL _mbspbrk(const unsigned char* str, const unsigned char* accept)
{
    const unsigned char* p;

    while(*str)
    {
        for(p = accept; *p; p += (_ismbblead(*p)?2:1) )
        {
            if (*p == *str)
                if( !_ismbblead(*p) || ( *(p+1) == *(str+1) ) )
                     return (unsigned char*)str;
        }
        str += (_ismbblead(*str)?2:1);
    }
    return NULL;
}


/*
 * Functions depending on locale codepage
 */

/*********************************************************************
 *		mblen(MSVCRT.@)
 * REMARKS
 *  Unlike most of the multibyte string functions this function uses
 *  the locale codepage, not the codepage set by _setmbcp
 */
int CDECL MSVCRT_mblen(const char* str, MSVCRT_size_t size)
{
  if (str && *str && size)
  {
    if(get_locinfo()->mb_cur_max == 1)
      return 1; /* ASCII CP */

    return !MSVCRT_isleadbyte((unsigned char)*str) ? 1 : (size>1 ? 2 : -1);
  }
  return 0;
}

/*********************************************************************
 *              mbrlen(MSVCRT.@)
 */
MSVCRT_size_t CDECL MSVCRT_mbrlen(const char *str, MSVCRT_size_t len, MSVCRT_mbstate_t *state)
{
    MSVCRT_mbstate_t s = (state ? *state : 0);
    MSVCRT_size_t ret;

    if(!len || !str || !*str)
        return 0;

    if(get_locinfo()->mb_cur_max == 1) {
        return 1;
    }else if(!s && MSVCRT_isleadbyte((unsigned char)*str)) {
        if(len == 1) {
            s = (unsigned char)*str;
            ret = -2;
        }else {
            ret = 2;
        }
    }else if(!s) {
        ret = 1;
    }else {
        s = 0;
        ret = 2;
    }

    if(state)
        *state = s;
    return ret;
}

/*********************************************************************
 *		_mbstrlen_l(MSVCRT.@)
 */
MSVCRT_size_t CDECL _mbstrlen_l(const char* str, MSVCRT__locale_t locale)
{
    MSVCRT_pthreadlocinfo locinfo;

    if(!locale)
        locinfo = get_locinfo();
    else
        locinfo = locale->locinfo;

    if(locinfo->mb_cur_max > 1) {
        MSVCRT_size_t len;
        len = MultiByteToWideChar(locinfo->lc_codepage, MB_ERR_INVALID_CHARS,
                                  str, -1, NULL, 0);
        if (!len) {
            *MSVCRT__errno() = MSVCRT_EILSEQ;
            return -1;
        }
        return len - 1;
    }

    return strlen(str);
}

/*********************************************************************
 *		_mbstrlen(MSVCRT.@)
 */
MSVCRT_size_t CDECL _mbstrlen(const char* str)
{
    return _mbstrlen_l(str, NULL);
}

/*********************************************************************
 *		_mbtowc_l(MSVCRT.@)
 */
int CDECL MSVCRT_mbtowc_l(MSVCRT_wchar_t *dst, const char* str, MSVCRT_size_t n, MSVCRT__locale_t locale)
{
    MSVCRT_pthreadlocinfo locinfo;
    MSVCRT_wchar_t tmpdst = '\0';

    if(!locale)
        locinfo = get_locinfo();
    else
        locinfo = locale->locinfo;

    if(n <= 0 || !str)
        return 0;
    if(!locinfo->lc_codepage)
        tmpdst = (unsigned char)*str;
    else if(!MultiByteToWideChar(locinfo->lc_codepage, 0, str, n, &tmpdst, 1))
        return -1;
    if(dst)
        *dst = tmpdst;
    /* return the number of bytes from src that have been used */
    if(!*str)
        return 0;
    if(n >= 2 && MSVCRT__isleadbyte_l((unsigned char)*str, locale) && str[1])
        return 2;
    return 1;
}

/*********************************************************************
 *              mbtowc(MSVCRT.@)
 */
int CDECL MSVCRT_mbtowc(MSVCRT_wchar_t *dst, const char* str, MSVCRT_size_t n)
{
    return MSVCRT_mbtowc_l(dst, str, n, NULL);
}

/*********************************************************************
 *              mbrtowc(MSVCRT.@)
 */
MSVCRT_size_t CDECL MSVCRT_mbrtowc(MSVCRT_wchar_t *dst, const char *str,
        MSVCRT_size_t n, MSVCRT_mbstate_t *state)
{
    MSVCRT_pthreadlocinfo locinfo = get_locinfo();
    MSVCRT_mbstate_t s = (state ? *state : 0);
    char tmpstr[2];
    int len = 0;

    if(dst)
        *dst = 0;

    if(!n || !str || !*str)
        return 0;

    if(locinfo->mb_cur_max == 1) {
        tmpstr[len++] = *str;
    }else if(!s && MSVCRT_isleadbyte((unsigned char)*str)) {
        if(n == 1) {
            s = (unsigned char)*str;
            len = -2;
        }else {
            tmpstr[0] = str[0];
            tmpstr[1] = str[1];
            len = 2;
        }
    }else if(!s) {
        tmpstr[len++] = *str;
    }else {
        tmpstr[0] = s;
        tmpstr[1] = *str;
        len = 2;
        s = 0;
    }

    if(len > 0) {
        if(!MultiByteToWideChar(locinfo->lc_codepage, 0, tmpstr, len, dst, dst ? 1 : 0))
            len = -1;
    }

    if(state)
        *state = s;
    return len;
}

/*********************************************************************
 *		_mbstowcs_l(MSVCRT.@)
 */
MSVCRT_size_t CDECL MSVCRT__mbstowcs_l(MSVCRT_wchar_t *wcstr, const char *mbstr,
        MSVCRT_size_t count, MSVCRT__locale_t locale)
{
    MSVCRT_pthreadlocinfo locinfo;
    MSVCRT_size_t i, size;

    if(!mbstr) {
        *MSVCRT__errno() = MSVCRT_EINVAL;
        return -1;
    }

    if(!locale)
        locinfo = get_locinfo();
    else
        locinfo = locale->locinfo;

    /* Ignore count parameter */
    if(!wcstr)
        return MultiByteToWideChar(locinfo->lc_codepage, 0, mbstr, -1, NULL, 0)-1;

    for(i=0, size=0; i<count; i++) {
        if(mbstr[size] == '\0')
            break;

        size += (MSVCRT__isleadbyte_l((unsigned char)mbstr[size], locale) ? 2 : 1);
    }

    size = MultiByteToWideChar(locinfo->lc_codepage, 0,
            mbstr, size, wcstr, count);

    if(size<count && wcstr)
        wcstr[size] = '\0';

    return size;
}

/*********************************************************************
 *		mbstowcs(MSVCRT.@)
 */
MSVCRT_size_t CDECL MSVCRT_mbstowcs(MSVCRT_wchar_t *wcstr,
        const char *mbstr, MSVCRT_size_t count)
{
    return MSVCRT__mbstowcs_l(wcstr, mbstr, count, NULL);
}

/*********************************************************************
 *              _mbstowcs_s_l(MSVCRT.@)
 */
int CDECL MSVCRT__mbstowcs_s_l(MSVCRT_size_t *ret, MSVCRT_wchar_t *wcstr,
        MSVCRT_size_t size, const char *mbstr,
        MSVCRT_size_t count, MSVCRT__locale_t locale)
{
    MSVCRT_size_t conv;

    if(!wcstr && !size) {
        conv = MSVCRT__mbstowcs_l(NULL, mbstr, 0, locale);
        if(ret)
            *ret = conv+1;
        return 0;
    }

    if (!MSVCRT_CHECK_PMT(wcstr != NULL)) return MSVCRT_EINVAL;
    if (!MSVCRT_CHECK_PMT(mbstr != NULL)) {
        if(size) wcstr[0] = '\0';
        return MSVCRT_EINVAL;
    }

    if(count==MSVCRT__TRUNCATE || size<count)
        conv = size;
    else
        conv = count;

    conv = MSVCRT__mbstowcs_l(wcstr, mbstr, conv, locale);
    if(conv<size)
        wcstr[conv++] = '\0';
    else if(conv==size && (count==MSVCRT__TRUNCATE || wcstr[conv-1]=='\0'))
        wcstr[conv-1] = '\0';
    else {
        MSVCRT_INVALID_PMT("wcstr[size] is too small", MSVCRT_ERANGE);
        if(size)
            wcstr[0] = '\0';
        return MSVCRT_ERANGE;
    }

    if(ret)
        *ret = conv;
    return 0;
}

/*********************************************************************
 *              mbstowcs_s(MSVCRT.@)
 */
int CDECL MSVCRT__mbstowcs_s(MSVCRT_size_t *ret, MSVCRT_wchar_t *wcstr,
        MSVCRT_size_t size, const char *mbstr, MSVCRT_size_t count)
{
    return MSVCRT__mbstowcs_s_l(ret, wcstr, size, mbstr, count, NULL);
}

/*********************************************************************
 *              mbsrtowcs(MSVCRT.@)
 */
MSVCRT_size_t CDECL MSVCRT_mbsrtowcs(MSVCRT_wchar_t *wcstr,
        const char **pmbstr, MSVCRT_size_t count, MSVCRT_mbstate_t *state)
{
    MSVCRT_mbstate_t s = (state ? *state : 0);
    MSVCRT_wchar_t tmpdst;
    MSVCRT_size_t ret = 0;

    if(!MSVCRT_CHECK_PMT(pmbstr != NULL))
        return -1;

    while(!wcstr || count>ret) {
        int ch_len = MSVCRT_mbrtowc(&tmpdst, *pmbstr, 2, &s);
        if(wcstr)
            wcstr[ret] = tmpdst;

        if(ch_len < 0) {
            return -1;
        }else if(ch_len == 0) {
            *pmbstr = NULL;
            return ret;
        }

        *pmbstr += ch_len;
        ret++;
    }

    return ret;
}
