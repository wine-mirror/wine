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

#include "msvcrt.h"
#include "wine/unicode.h"
#include "wine/debug.h"
#include "msvcrt/mbctype.h"
#include "msvcrt/mbstring.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

unsigned char MSVCRT_mbctype[257];
static int g_mbcp_is_multibyte = 0;

int MSVCRT___mb_cur_max = 1;
extern int MSVCRT___lc_collate_cp;

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
    {20932, {1, 255, 0, 0}},  /* seems to give different results on different systems */
    {0, {1, 255, 0, 0}}       /* match all with FIXME */
};

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
  if (!MultiByteToWideChar(MSVCRT___lc_codepage, 0, mbch, n_chars, &chW, 1))
  {
    WARN("MultiByteToWideChar failed on %x\n", ch);
    return 0;
  }
  return chW;
}

static inline size_t u_strlen( const unsigned char *str )
{
  return strlen( (const char*) str );
}

static inline unsigned char* u_strncat( unsigned char* dst, const unsigned char* src, size_t len )
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

static inline int u_strncmp( const unsigned char *s1, const unsigned char *s2, size_t len )
{
  return strncmp( (const char*)s1, (const char*)s2, len );
}

static inline int u_strncasecmp( const unsigned char *s1, const unsigned char *s2, size_t len )
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

static inline unsigned char *u_strtok( unsigned char *s, const unsigned char *delim )
{
  return (unsigned char*) strtok( (char*)s, (const char*)delim );
}

static inline unsigned char *u__strset( unsigned char *s, unsigned char c )
{
  return (unsigned char*) _strset( (char*)s, c);
}

static inline unsigned char *u__strnset( unsigned char *s, unsigned char c, size_t len )
{
  return (unsigned char*) _strnset( (char*)s, c, len );
}

static inline size_t u_strcspn( const unsigned char *s, const unsigned char *rej )
{
  return strcspn( (const char *)s, (const char*)rej );
}

/*********************************************************************
 *		__p__mbctype (MSVCRT.@)
 */
unsigned char* CDECL __p__mbctype(void)
{
  return MSVCRT_mbctype;
}

/*********************************************************************
 *		__p___mb_cur_max(MSVCRT.@)
 */
int* CDECL __p___mb_cur_max(void)
{
  return &MSVCRT___mb_cur_max;
}

/*********************************************************************
 *		_setmbcp (MSVCRT.@)
 */
int CDECL _setmbcp(int cp)
{
  int newcp;
  CPINFO cpi;
  BYTE *bytes;
  WORD chartypes[256];
  WORD *curr_type;
  char bufA[256];
  WCHAR bufW[256];
  int charcount;
  int ret;
  int i;

  switch (cp)
  {
    case _MB_CP_ANSI:
      newcp = GetACP();
      break;
    case _MB_CP_OEM:
      newcp = GetOEMCP();
      break;
    case _MB_CP_LOCALE:
      newcp = MSVCRT___lc_codepage;
      break;
    case _MB_CP_SBCS:
      newcp = 20127;   /* ASCII */
      break;
    default:
      newcp = cp;
      break;
  }

  if (!GetCPInfo(newcp, &cpi))
  {
    WARN("Codepage %d not found\n", newcp);
    msvcrt_set_errno(MSVCRT_EINVAL);
    return -1;
  }

  /* setup the _mbctype */
  memset(MSVCRT_mbctype, 0, sizeof(MSVCRT_mbctype));

  bytes = cpi.LeadByte;
  while (bytes[0] || bytes[1])
  {
    for (i = bytes[0]; i <= bytes[1]; i++)
      MSVCRT_mbctype[i + 1] |= _M1;
    bytes += 2;
  }

  if (cpi.MaxCharSize > 1)
  {
    /* trail bytes not available through kernel32 but stored in a structure in msvcrt */
    struct cp_extra_info_t *cpextra = g_cpextrainfo;

    g_mbcp_is_multibyte = 1;
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
            MSVCRT_mbctype[i + 1] |= _M2;
          bytes += 2;
        }
        break;
      }
      cpextra++;
    }
  }
  else
    g_mbcp_is_multibyte = 0;

  /* we can't use GetStringTypeA directly because we don't have a locale - only a code page
   */
  charcount = 0;
  for (i = 0; i < 256; i++)
    if (!(MSVCRT_mbctype[i + 1] & _M1))
      bufA[charcount++] = i;

  ret = MultiByteToWideChar(newcp, 0, bufA, charcount, bufW, charcount);
  if (ret != charcount)
    ERR("MultiByteToWideChar of chars failed for cp %d, ret=%d (exp %d), error=%d\n", newcp, ret, charcount, GetLastError());

  GetStringTypeW(CT_CTYPE1, bufW, charcount, chartypes);

  curr_type = chartypes;
  for (i = 0; i < 256; i++)
    if (!(MSVCRT_mbctype[i + 1] & _M1))
    {
	if ((*curr_type) & C1_UPPER)
	    MSVCRT_mbctype[i + 1] |= _SBUP;
	if ((*curr_type) & C1_LOWER)
	    MSVCRT_mbctype[i + 1] |= _SBLOW;
	curr_type++;
    }

  if (newcp == 932)   /* CP932 only - set _MP and _MS */
  {
    /* On Windows it's possible to calculate the _MP and _MS from CT_CTYPE1
     * and CT_CTYPE3. But as of Wine 0.9.43 we return wrong values what makes
     * it hard. As this is set only for codepage 932 we hardcode it what gives
     * also faster execution.
     */
    for (i = 161; i <= 165; i++)
      MSVCRT_mbctype[i + 1] |= _MP;
    for (i = 166; i <= 223; i++)
      MSVCRT_mbctype[i + 1] |= _MS;
  }

  MSVCRT___lc_collate_cp = MSVCRT___lc_codepage = newcp;
  TRACE("(%d) -> %d\n", cp, MSVCRT___lc_codepage);
  return 0;
}

/*********************************************************************
 *		_getmbcp (MSVCRT.@)
 */
int CDECL _getmbcp(void)
{
  return MSVCRT___lc_codepage;
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
    if (MSVCRT_isleadbyte(c))
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
    if (MSVCRT_isleadbyte(c))
    {
      FIXME("Handle MBC chars\n");
      return c;
    }
    return toupper(c); /* ASCII CP or SB char */
}

/*********************************************************************
 *		_mbsdec(MSVCRT.@)
 */
unsigned char* CDECL _mbsdec(const unsigned char* start, const unsigned char* cur)
{
  if(MSVCRT___mb_cur_max > 1)
    return (unsigned char *)(_ismbstrail(start,cur-1) ? cur - 2 : cur -1);

  return (unsigned char *)cur - 1; /* ASCII CP or SB char */
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
 *		_mbclen(MSVCRT.@)
 */
unsigned int CDECL _mbclen(const unsigned char* str)
{
  return _ismbblead(*str) ? 2 : 1;
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
  if (g_mbcp_is_multibyte)
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
  if(g_mbcp_is_multibyte)
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
  return u_strcmp(str, cmp); /* ASCII CP */
}

/*********************************************************************
 *		_mbsicoll(MSVCRT.@)
 * FIXME: handle locales.
 */
int CDECL _mbsicoll(const unsigned char* str, const unsigned char* cmp)
{
  if(MSVCRT___mb_cur_max > 1)
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
 *		_mbscoll(MSVCRT.@)
 * Performs a case-sensitive comparison according to the current code page
 * RETURN
 *   _NLSCMPERROR if error
 * FIXME: handle locales.
 */
int CDECL _mbscoll(const unsigned char* str, const unsigned char* cmp)
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
  return u_strcmp(str, cmp); /* ASCII CP */
}


/*********************************************************************
 *		_mbsicmp(MSVCRT.@)
 */
int CDECL _mbsicmp(const unsigned char* str, const unsigned char* cmp)
{
  if(MSVCRT___mb_cur_max > 1)
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

  if(MSVCRT___mb_cur_max > 1)
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
  if(MSVCRT___mb_cur_max > 1)
  {
    unsigned int strc, cmpc;
    while (len)
    {
      int clen;
      if(!*str)
        return *cmp ? -1 : 0;
      if(!*cmp)
        return 1;
      if (MSVCRT_isleadbyte(*str))
      {
        strc=(len>=2)?_mbsnextc(str):0;
        clen=2;
      }
      else
      {
        strc=*str;
        clen=1;
      }
      if (MSVCRT_isleadbyte(*cmp))
        cmpc=(len>=2)?_mbsnextc(cmp):0;
      else
        cmpc=*str;
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
  if(MSVCRT___mb_cur_max > 1)
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
  if(MSVCRT___mb_cur_max > 1)
  {
    unsigned int strc, cmpc;
    while (len)
    {
      int clen;
      if(!*str)
        return *cmp ? -1 : 0;
      if(!*cmp)
        return 1;
      if (MSVCRT_isleadbyte(*str))
      {
        strc=(len>=2)?_mbsnextc(str):0;
        clen=2;
      }
      else
      {
        strc=*str;
        clen=1;
      }
      if (MSVCRT_isleadbyte(*cmp))
        cmpc=(len>=2)?_mbsnextc(cmp):0;
      else
        cmpc=*str;
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
  if(MSVCRT___mb_cur_max > 1)
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
  if(MSVCRT___mb_cur_max > 1)
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
 *		_mbstok(MSVCRT.@)
 *
 * Find and extract tokens from strings
 */
unsigned char* CDECL _mbstok(unsigned char *str, const unsigned char *delim)
{
    thread_data_t *data = msvcrt_get_thread_data();
    unsigned char *ret;

    if(MSVCRT___mb_cur_max > 1)
    {
	unsigned int c;

	if (!str)
    	    if (!(str = data->mbstok_next)) return NULL;

	while ((c = _mbsnextc(str)) && _mbschr(delim, c)) {
	    str += c > 255 ? 2 : 1;
	}
	if (!*str) return NULL;
	ret = str++;
	while ((c = _mbsnextc(str)) && !_mbschr(delim, c)) {
	    str += c > 255 ? 2 : 1;
	}
	if (*str) {
	    *str++ = 0;
	    if (c > 255) *str++ = 0;
	}
	data->mbstok_next = str;
	return ret;
    }
    return u_strtok(str, delim); /* ASCII CP */
}

/*********************************************************************
 *		mbtowc(MSVCRT.@)
 */
int CDECL MSVCRT_mbtowc(MSVCRT_wchar_t *dst, const char* str, MSVCRT_size_t n)
{
    /* temp var needed because MultiByteToWideChar wants non NULL destination */
    MSVCRT_wchar_t tmpdst = '\0';

    if(n <= 0 || !str)
        return 0;
    if(!MultiByteToWideChar(CP_ACP, 0, str, n, &tmpdst, 1))
        return -1;
    if(dst)
        *dst = tmpdst;
    /* return the number of bytes from src that have been used */
    if(!*str)
        return 0;
    if(n >= 2 && MSVCRT_isleadbyte(*str) && str[1])
        return 2;
    return 1;
}

/*********************************************************************
 *		_mbbtombc(MSVCRT.@)
 */
unsigned int CDECL _mbbtombc(unsigned int c)
{
  if(MSVCRT___mb_cur_max > 1 &&
     ((c >= 0x20 && c <=0x7e) ||(c >= 0xa1 && c <= 0xdf)))
  {
    /* FIXME: I can't get this function to return anything
     * different from what I pass it...
     */
  }
  return c;  /* ASCII CP or no MB char */
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
  if(MSVCRT___lc_codepage == 932)
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
  if(MSVCRT___lc_codepage == 932)
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
  if(MSVCRT___lc_codepage == 932)
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
  return (MSVCRT_mbctype[(c&0xff) + 1] & _M1) != 0;
}


/*********************************************************************
 *		_ismbbtrail(MSVCRT.@)
 */
int CDECL _ismbbtrail(unsigned int c)
{
  return (MSVCRT_mbctype[(c&0xff) + 1] & _M2) != 0;
}

/*********************************************************************
 *		_ismbslead(MSVCRT.@)
 */
int CDECL _ismbslead(const unsigned char* start, const unsigned char* str)
{
  int lead = 0;

  if(!g_mbcp_is_multibyte)
    return 0;

  /* Lead bytes can also be trail bytes so we need to analise the string
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
 *		_mbsset(MSVCRT.@)
 */
unsigned char* CDECL _mbsset(unsigned char* str, unsigned int c)
{
  unsigned char* ret = str;

  if(MSVCRT___mb_cur_max == 1 || c < 256)
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

    if(MSVCRT___mb_cur_max == 1 || c < 256)
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

  if(MSVCRT___mb_cur_max == 1 || c < 256)
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
  if(MSVCRT___mb_cur_max > 1)
  {
    ret=0;
    while(*str && len-- > 0)
    {
      if(MSVCRT_isleadbyte(*str))
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
  if(MSVCRT___mb_cur_max > 1)
  {
    const unsigned char* xstr = str;
    while(*xstr && len-- > 0)
    {
      if (MSVCRT_isleadbyte(*xstr++))
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
    if(MSVCRT___mb_cur_max > 1)
    {
        unsigned char *res = dst;
        while (*dst) {
	    if (MSVCRT_isleadbyte(*dst++)) {
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


/*********************************************************************
 *		_mbsncat(MSVCRT.@)
 */
unsigned char* CDECL _mbsncat(unsigned char* dst, const unsigned char* src, MSVCRT_size_t len)
{
  if(MSVCRT___mb_cur_max > 1)
  {
    unsigned char *res = dst;
    while (*dst)
    {
      if (MSVCRT_isleadbyte(*dst++))
        dst++;
    }
    while (*src && len--)
    {
      *dst++ = *src;
      if(MSVCRT_isleadbyte(*src++))
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
  if (MSVCRT___mb_cur_max > 1)
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
 *              _mbsupr(MSVCRT.@)
 */
unsigned char* CDECL _mbsupr(unsigned char* s)
{
  unsigned char *ret = s;
  if (!s)
    return NULL;
  if (MSVCRT___mb_cur_max > 1)
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
 *              _mbsspn (MSVCRT.@)
 */
MSVCRT_size_t CDECL _mbsspn(const unsigned char* string, const unsigned char* set)
{
    const unsigned char *p, *q;

    for (p = string; *p; p++)
    {
        if (MSVCRT_isleadbyte(*p))
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
        if (MSVCRT_isleadbyte(*p))
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
  if (MSVCRT___mb_cur_max > 1)
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
        if (MSVCRT_isleadbyte(*p))
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
        if(MSVCRT_isleadbyte(temp[i*2]))
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
        for(p = accept; *p; p += (MSVCRT_isleadbyte(*p)?2:1) )
        {
            if (*p == *str)
                if( !MSVCRT_isleadbyte(*p) || ( *(p+1) == *(str+1) ) )
                     return (unsigned char*)str;
        }
        str += (MSVCRT_isleadbyte(*str)?2:1);
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
    if(MSVCRT___mb_cur_max == 1)
      return 1; /* ASCII CP */

    return !MSVCRT_isleadbyte(*str) ? 1 : (size>1 ? 2 : -1);
  }
  return 0;
}

/*********************************************************************
 *		_mbstrlen(MSVCRT.@)
 * REMARKS
 *  Unlike most of the multibyte string functions this function uses
 *  the locale codepage, not the codepage set by _setmbcp
 */
MSVCRT_size_t CDECL _mbstrlen(const char* str)
{
  if(MSVCRT___mb_cur_max > 1)
  {
    MSVCRT_size_t len = 0;
    while(*str)
    {
      /* FIXME: According to the documentation we are supposed to test for
       * multi-byte character validity. Whatever that means
       */
      str += MSVCRT_isleadbyte(*str) ? 2 : 1;
      len++;
    }
    return len;
  }
  return strlen(str); /* ASCII CP */
}
