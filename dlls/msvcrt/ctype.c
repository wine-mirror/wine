/*
 * msvcrt.dll ctype functions
 *
 * Copyright 2000 Jon Griffiths
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
 */

#include "msvcrt.h"
#include "winnls.h"

/* Some abbreviations to make the following table readable */
#define _C_ MSVCRT__CONTROL
#define _S_ MSVCRT__SPACE
#define _P_ MSVCRT__PUNCT
#define _D_ MSVCRT__DIGIT
#define _H_ MSVCRT__HEX
#define _U_ MSVCRT__UPPER
#define _L_ MSVCRT__LOWER

WORD MSVCRT__ctype [257] = {
  0, _C_, _C_, _C_, _C_, _C_, _C_, _C_, _C_, _C_, _S_|_C_, _S_|_C_,
  _S_|_C_, _S_|_C_, _S_|_C_, _C_, _C_, _C_, _C_, _C_, _C_, _C_, _C_,
  _C_, _C_, _C_, _C_, _C_, _C_, _C_, _C_, _C_, _C_, _S_|MSVCRT__BLANK,
  _P_, _P_, _P_, _P_, _P_, _P_, _P_, _P_, _P_, _P_, _P_, _P_, _P_, _P_,
  _P_, _D_|_H_, _D_|_H_, _D_|_H_, _D_|_H_, _D_|_H_, _D_|_H_, _D_|_H_,
  _D_|_H_, _D_|_H_, _D_|_H_, _P_, _P_, _P_, _P_, _P_, _P_, _P_, _U_|_H_,
  _U_|_H_, _U_|_H_, _U_|_H_, _U_|_H_, _U_|_H_, _U_, _U_, _U_, _U_, _U_,
  _U_, _U_, _U_, _U_, _U_, _U_, _U_, _U_, _U_, _U_, _U_, _U_, _U_, _U_,
  _U_, _P_, _P_, _P_, _P_, _P_, _P_, _L_|_H_, _L_|_H_, _L_|_H_, _L_|_H_,
  _L_|_H_, _L_|_H_, _L_, _L_, _L_, _L_, _L_, _L_, _L_, _L_, _L_, _L_,
  _L_, _L_, _L_, _L_, _L_, _L_, _L_, _L_, _L_, _L_, _P_, _P_, _P_, _P_,
  _C_, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

#if _MSVCR_VER <= 110
# define B110 MSVCRT__BLANK
#else
# define B110 0
#endif

#if _MSVCR_VER == 120
# define D120 0
#else
# define D120 4
#endif

#if _MSVCR_VER >= 140
# define S140 MSVCRT__SPACE
# define L140 MSVCRT__LOWER | 0x100
# define C140 MSVCRT__CONTROL
#else
# define S140 0
# define L140 0
# define C140 0
#endif
WORD MSVCRT__wctype[257] =
{
    0,
    /* 00 */
    0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020,
    0x0020, 0x0028 | B110, 0x0028, 0x0028, 0x0028, 0x0028, 0x0020, 0x0020,
    /* 10 */
    0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020,
    0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020,
    /* 20 */
    0x0048, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    /* 30 */
    0x0084, 0x0084, 0x0084, 0x0084, 0x0084, 0x0084, 0x0084, 0x0084,
    0x0084, 0x0084, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    /* 40 */
    0x0010, 0x0181, 0x0181, 0x0181, 0x0181, 0x0181, 0x0181, 0x0101,
    0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101,
    /* 50 */
    0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101,
    0x0101, 0x0101, 0x0101, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    /* 60 */
    0x0010, 0x0182, 0x0182, 0x0182, 0x0182, 0x0182, 0x0182, 0x0102,
    0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102,
    /* 70 */
    0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102,
    0x0102, 0x0102, 0x0102, 0x0010, 0x0010, 0x0010, 0x0010, 0x0020,
    /* 80 */
    0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020 | S140, 0x0020, 0x0020,
    0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020,
    /* 90 */
    0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020,
    0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020,
    /* a0 */
    0x0008 | B110, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    0x0010, 0x0010, 0x0010 | L140, 0x0010, 0x0010, 0x0010 | C140, 0x0010, 0x0010,
    /* b0 */
    0x0010, 0x0010, 0x0010 | D120, 0x0010 | D120, 0x0010, 0x0010 | L140, 0x0010, 0x0010,
    0x0010, 0x0010 | D120, 0x0010 | L140, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
    /* c0 */
    0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101,
    0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101,
    /* d0 */
    0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0010,
    0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0101, 0x0102,
    /* e0 */
    0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102,
    0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102,
    /* f0 */
    0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0010,
    0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102, 0x0102
};

WORD *MSVCRT__pwctype = MSVCRT__wctype + 1;

/*********************************************************************
 *		__p__pctype (MSVCRT.@)
 */
unsigned short** CDECL MSVCRT___p__pctype(void)
{
    return &get_locinfo()->pctype;
}

/*********************************************************************
 *		__pctype_func (MSVCRT.@)
 */
const unsigned short* CDECL MSVCRT___pctype_func(void)
{
    return get_locinfo()->pctype;
}

/*********************************************************************
 *		_isctype_l (MSVCRT.@)
 */
int CDECL MSVCRT__isctype_l(int c, int type, MSVCRT__locale_t locale)
{
  MSVCRT_pthreadlocinfo locinfo;

  if(!locale)
    locinfo = get_locinfo();
  else
    locinfo = locale->locinfo;

  if (c >= -1 && c <= 255)
    return locinfo->pctype[c] & type;

  if (locinfo->mb_cur_max != 1 && c > 0)
  {
    /* FIXME: Is there a faster way to do this? */
    WORD typeInfo;
    char convert[3], *pconv = convert;

    if (locinfo->pctype[(UINT)c >> 8] & MSVCRT__LEADBYTE)
      *pconv++ = (UINT)c >> 8;
    *pconv++ = c & 0xff;
    *pconv = 0;

    if (GetStringTypeExA(locinfo->lc_handle[MSVCRT_LC_CTYPE],
                CT_CTYPE1, convert, convert[1] ? 2 : 1, &typeInfo))
      return typeInfo & type;
  }
  return 0;
}

/*********************************************************************
 *              _isctype (MSVCRT.@)
 */
int CDECL MSVCRT__isctype(int c, int type)
{
    return MSVCRT__isctype_l(c, type, NULL);
}

/*********************************************************************
 *		_isalnum_l (MSVCRT.@)
 */
int CDECL MSVCRT__isalnum_l(int c, MSVCRT__locale_t locale)
{
  return MSVCRT__isctype_l( c, MSVCRT__ALPHA | MSVCRT__DIGIT, locale );
}

/*********************************************************************
 *		isalnum (MSVCRT.@)
 */
int CDECL MSVCRT_isalnum(int c)
{
  return MSVCRT__isctype( c, MSVCRT__ALPHA | MSVCRT__DIGIT );
}

/*********************************************************************
 *		_isalpha_l (MSVCRT.@)
 */
int CDECL MSVCRT__isalpha_l(int c, MSVCRT__locale_t locale)
{
  return MSVCRT__isctype_l( c, MSVCRT__ALPHA, locale );
}

/*********************************************************************
 *		isalpha (MSVCRT.@)
 */
int CDECL MSVCRT_isalpha(int c)
{
  return MSVCRT__isctype( c, MSVCRT__ALPHA );
}

/*********************************************************************
 *		_iscntrl_l (MSVCRT.@)
 */
int CDECL MSVCRT__iscntrl_l(int c, MSVCRT__locale_t locale)
{
  return MSVCRT__isctype_l( c, MSVCRT__CONTROL, locale );
}

/*********************************************************************
 *		iscntrl (MSVCRT.@)
 */
int CDECL MSVCRT_iscntrl(int c)
{
  return MSVCRT__isctype( c, MSVCRT__CONTROL );
}

/*********************************************************************
 *		_isdigit_l (MSVCRT.@)
 */
int CDECL MSVCRT__isdigit_l(int c, MSVCRT__locale_t locale)
{
  return MSVCRT__isctype_l( c, MSVCRT__DIGIT, locale );
}

/*********************************************************************
 *		isdigit (MSVCRT.@)
 */
int CDECL MSVCRT_isdigit(int c)
{
  return MSVCRT__isctype( c, MSVCRT__DIGIT );
}

/*********************************************************************
 *		_isgraph_l (MSVCRT.@)
 */
int CDECL MSVCRT__isgraph_l(int c, MSVCRT__locale_t locale)
{
  return MSVCRT__isctype_l( c, MSVCRT__ALPHA | MSVCRT__DIGIT | MSVCRT__PUNCT, locale );
}

/*********************************************************************
 *		isgraph (MSVCRT.@)
 */
int CDECL MSVCRT_isgraph(int c)
{
  return MSVCRT__isctype( c, MSVCRT__ALPHA | MSVCRT__DIGIT | MSVCRT__PUNCT );
}

/*********************************************************************
 *		_isleadbyte_l (MSVCRT.@)
 */
int CDECL MSVCRT__isleadbyte_l(int c, MSVCRT__locale_t locale)
{
  return MSVCRT__isctype_l( c, MSVCRT__LEADBYTE, locale );
}

/*********************************************************************
 *		isleadbyte (MSVCRT.@)
 */
int CDECL MSVCRT_isleadbyte(int c)
{
  return MSVCRT__isctype( c, MSVCRT__LEADBYTE );
}

/*********************************************************************
 *		_islower_l (MSVCRT.@)
 */
int CDECL MSVCRT__islower_l(int c, MSVCRT__locale_t locale)
{
  return MSVCRT__isctype_l( c, MSVCRT__LOWER, locale );
}

/*********************************************************************
 *		islower (MSVCRT.@)
 */
int CDECL MSVCRT_islower(int c)
{
  return MSVCRT__isctype( c, MSVCRT__LOWER );
}

/*********************************************************************
 *		_isprint_l (MSVCRT.@)
 */
int CDECL MSVCRT__isprint_l(int c, MSVCRT__locale_t locale)
{
  return MSVCRT__isctype_l( c, MSVCRT__ALPHA | MSVCRT__DIGIT | MSVCRT__BLANK | MSVCRT__PUNCT, locale );
}

/*********************************************************************
 *		isprint (MSVCRT.@)
 */
int CDECL MSVCRT_isprint(int c)
{
  return MSVCRT__isctype( c, MSVCRT__ALPHA | MSVCRT__DIGIT | MSVCRT__BLANK | MSVCRT__PUNCT );
}

/*********************************************************************
 *		ispunct (MSVCRT.@)
 */
int CDECL MSVCRT_ispunct(int c)
{
  return MSVCRT__isctype( c, MSVCRT__PUNCT );
}

/*********************************************************************
 *		_ispunct_l (MSVCR80.@)
 */
int CDECL MSVCRT__ispunct_l(int c, MSVCRT__locale_t locale)
{
  return MSVCRT__isctype_l( c, MSVCRT__PUNCT, locale );
}

/*********************************************************************
 *		_isspace_l (MSVCRT.@)
 */
int CDECL MSVCRT__isspace_l(int c, MSVCRT__locale_t locale)
{
  return MSVCRT__isctype_l( c, MSVCRT__SPACE, locale );
}

/*********************************************************************
 *		isspace (MSVCRT.@)
 */
int CDECL MSVCRT_isspace(int c)
{
  return MSVCRT__isctype( c, MSVCRT__SPACE );
}

/*********************************************************************
 *		_isupper_l (MSVCRT.@)
 */
int CDECL MSVCRT__isupper_l(int c, MSVCRT__locale_t locale)
{
  return MSVCRT__isctype_l( c, MSVCRT__UPPER, locale );
}

/*********************************************************************
 *		isupper (MSVCRT.@)
 */
int CDECL MSVCRT_isupper(int c)
{
  return MSVCRT__isctype( c, MSVCRT__UPPER );
}

/*********************************************************************
 *		_isxdigit_l (MSVCRT.@)
 */
int CDECL MSVCRT__isxdigit_l(int c, MSVCRT__locale_t locale)
{
  return MSVCRT__isctype_l( c, MSVCRT__HEX, locale );
}

/*********************************************************************
 *		isxdigit (MSVCRT.@)
 */
int CDECL MSVCRT_isxdigit(int c)
{
  return MSVCRT__isctype( c, MSVCRT__HEX );
}

/*********************************************************************
 *		_isblank_l (MSVCRT.@)
 */
int CDECL MSVCRT__isblank_l(int c, MSVCRT__locale_t locale)
{
  return c == '\t' || MSVCRT__isctype_l( c, MSVCRT__BLANK, locale );
}

/*********************************************************************
 *		isblank (MSVCRT.@)
 */
int CDECL MSVCRT_isblank(int c)
{
  return c == '\t' || MSVCRT__isctype( c, MSVCRT__BLANK );
}

/*********************************************************************
 *		__isascii (MSVCRT.@)
 */
int CDECL MSVCRT___isascii(int c)
{
  return isascii((unsigned)c);
}

/*********************************************************************
 *		__toascii (MSVCRT.@)
 */
int CDECL MSVCRT___toascii(int c)
{
  return (unsigned)c & 0x7f;
}

/*********************************************************************
 *		iswascii (MSVCRT.@)
 *
 */
int CDECL MSVCRT_iswascii(MSVCRT_wchar_t c)
{
  return ((unsigned)c < 0x80);
}

/*********************************************************************
 *		__iscsym (MSVCRT.@)
 */
int CDECL MSVCRT___iscsym(int c)
{
  return (c < 127 && (isalnum(c) || c == '_'));
}

/*********************************************************************
 *		__iscsymf (MSVCRT.@)
 */
int CDECL MSVCRT___iscsymf(int c)
{
  return (c < 127 && (isalpha(c) || c == '_'));
}

/*********************************************************************
 *		_toupper_l (MSVCRT.@)
 */
int CDECL MSVCRT__toupper_l(int c, MSVCRT__locale_t locale)
{
    MSVCRT_pthreadlocinfo locinfo;
    unsigned char str[2], *p = str, ret[2];

    if(!locale)
        locinfo = get_locinfo();
    else
        locinfo = locale->locinfo;

    if((unsigned)c < 256)
        return locinfo->pcumap[c];

    if(locinfo->pctype[(c>>8)&255] & MSVCRT__LEADBYTE)
        *p++ = (c>>8) & 255;
    else {
        *MSVCRT__errno() = MSVCRT_EILSEQ;
        str[1] = 0;
    }
    *p++ = c & 255;

    switch(__crtLCMapStringA(locinfo->lc_handle[MSVCRT_LC_CTYPE], LCMAP_UPPERCASE,
                (char*)str, p-str, (char*)ret, 2, locinfo->lc_codepage, 0))
    {
    case 0:
        return c;
    case 1:
        return ret[0];
    default:
        return ret[0] + (ret[1]<<8);
    }
}

/*********************************************************************
 *		toupper (MSVCRT.@)
 */
int CDECL MSVCRT_toupper(int c)
{
    if(initial_locale)
        return c>='a' && c<='z' ? c-'a'+'A' : c;
    return MSVCRT__toupper_l(c, NULL);
}

/*********************************************************************
 *		_toupper (MSVCRT.@)
 */
int CDECL MSVCRT__toupper(int c)
{
    return c - 0x20;  /* sic */
}

/*********************************************************************
 *              _tolower_l (MSVCRT.@)
 */
int CDECL MSVCRT__tolower_l(int c, MSVCRT__locale_t locale)
{
    MSVCRT_pthreadlocinfo locinfo;
    unsigned char str[2], *p = str, ret[2];

    if(!locale)
        locinfo = get_locinfo();
    else
        locinfo = locale->locinfo;

    if((unsigned)c < 256)
        return locinfo->pclmap[c];

    if(locinfo->pctype[(c>>8)&255] & MSVCRT__LEADBYTE)
        *p++ = (c>>8) & 255;
    else {
        *MSVCRT__errno() = MSVCRT_EILSEQ;
        str[1] = 0;
    }
    *p++ = c & 255;

    switch(__crtLCMapStringA(locinfo->lc_handle[MSVCRT_LC_CTYPE], LCMAP_LOWERCASE,
                (char*)str, p-str, (char*)ret, 2, locinfo->lc_codepage, 0))
    {
    case 0:
        return c;
    case 1:
        return ret[0];
    default:
        return ret[0] + (ret[1]<<8);
    }
}

/*********************************************************************
 *              tolower (MSVCRT.@)
 */
int CDECL MSVCRT_tolower(int c)
{
    if(initial_locale)
        return c>='A' && c<='Z' ? c-'A'+'a' : c;
    return MSVCRT__tolower_l(c, NULL);
}

/*********************************************************************
 *		_tolower (MSVCRT.@)
 */
int CDECL MSVCRT__tolower(int c)
{
    return c + 0x20;  /* sic */
}

#if _MSVCR_VER>=120
/*********************************************************************
 *              wctype (MSVCR120.@)
 */
unsigned short __cdecl wctype(const char *property)
{
    static const struct {
        const char *name;
        unsigned short mask;
    } properties[] = {
        { "alnum", MSVCRT__DIGIT|MSVCRT__ALPHA },
        { "alpha", MSVCRT__ALPHA },
        { "cntrl", MSVCRT__CONTROL },
        { "digit", MSVCRT__DIGIT },
        { "graph", MSVCRT__DIGIT|MSVCRT__PUNCT|MSVCRT__ALPHA },
        { "lower", MSVCRT__LOWER },
        { "print", MSVCRT__DIGIT|MSVCRT__PUNCT|MSVCRT__BLANK|MSVCRT__ALPHA },
        { "punct", MSVCRT__PUNCT },
        { "space", MSVCRT__SPACE },
        { "upper", MSVCRT__UPPER },
        { "xdigit", MSVCRT__HEX }
    };
    unsigned int i;

    for(i=0; i<ARRAY_SIZE(properties); i++)
        if(!strcmp(property, properties[i].name))
            return properties[i].mask;

    return 0;
}
#endif
