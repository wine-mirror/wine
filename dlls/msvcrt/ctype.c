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

/* pctype is used by macros in the Win32 headers. It must point
 * To a table of flags exactly like ctype. To allow locale
 * changes to affect ctypes (i.e. isleadbyte), we use a second table
 * and update its flags whenever the current locale changes.
 */
WORD* MSVCRT__pctype = NULL;

/*********************************************************************
 *		__pctype_func (MSVCRT.@)
 */
WORD** CDECL MSVCRT___pctype_func(void)
{
  return &get_locale()->locinfo->pctype;
}

/*********************************************************************
 *		_isctype (MSVCRT.@)
 */
int CDECL _isctype(int c, int type)
{
  MSVCRT__locale_t locale = get_locale();

  if (c >= -1 && c <= 255)
    return locale->locinfo->pctype[c] & type;

  if (locale->locinfo->mb_cur_max != 1 && c > 0)
  {
    /* FIXME: Is there a faster way to do this? */
    WORD typeInfo;
    char convert[3], *pconv = convert;

    if (locale->locinfo->pctype[(UINT)c >> 8] & MSVCRT__LEADBYTE)
      *pconv++ = (UINT)c >> 8;
    *pconv++ = c & 0xff;
    *pconv = 0;
    /* FIXME: Use ctype LCID, not lc_all */
    if (GetStringTypeExA(get_locale()->locinfo->lc_handle[MSVCRT_LC_CTYPE],
                CT_CTYPE1, convert, convert[1] ? 2 : 1, &typeInfo))
      return typeInfo & type;
  }
  return 0;
}

/*********************************************************************
 *		isalnum (MSVCRT.@)
 */
int CDECL MSVCRT_isalnum(int c)
{
  return _isctype( c, MSVCRT__ALPHA | MSVCRT__DIGIT );
}

/*********************************************************************
 *		isalpha (MSVCRT.@)
 */
int CDECL MSVCRT_isalpha(int c)
{
  return _isctype( c, MSVCRT__ALPHA );
}

/*********************************************************************
 *		iscntrl (MSVCRT.@)
 */
int CDECL MSVCRT_iscntrl(int c)
{
  return _isctype( c, MSVCRT__CONTROL );
}

/*********************************************************************
 *		isdigit (MSVCRT.@)
 */
int CDECL MSVCRT_isdigit(int c)
{
  return _isctype( c, MSVCRT__DIGIT );
}

/*********************************************************************
 *		isgraph (MSVCRT.@)
 */
int CDECL MSVCRT_isgraph(int c)
{
  return _isctype( c, MSVCRT__ALPHA | MSVCRT__DIGIT | MSVCRT__PUNCT );
}

/*********************************************************************
 *		isleadbyte (MSVCRT.@)
 */
int CDECL MSVCRT_isleadbyte(int c)
{
  return _isctype( c, MSVCRT__LEADBYTE );
}

/*********************************************************************
 *		islower (MSVCRT.@)
 */
int CDECL MSVCRT_islower(int c)
{
  return _isctype( c, MSVCRT__LOWER );
}

/*********************************************************************
 *		isprint (MSVCRT.@)
 */
int CDECL MSVCRT_isprint(int c)
{
  return _isctype( c, MSVCRT__ALPHA | MSVCRT__DIGIT | MSVCRT__BLANK | MSVCRT__PUNCT );
}

/*********************************************************************
 *		ispunct (MSVCRT.@)
 */
int CDECL MSVCRT_ispunct(int c)
{
  return _isctype( c, MSVCRT__PUNCT );
}

/*********************************************************************
 *		isspace (MSVCRT.@)
 */
int CDECL MSVCRT_isspace(int c)
{
  return _isctype( c, MSVCRT__SPACE );
}

/*********************************************************************
 *		isupper (MSVCRT.@)
 */
int CDECL MSVCRT_isupper(int c)
{
  return _isctype( c, MSVCRT__UPPER );
}

/*********************************************************************
 *		isxdigit (MSVCRT.@)
 */
int CDECL MSVCRT_isxdigit(int c)
{
  return _isctype( c, MSVCRT__HEX );
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
 *		_toupper (MSVCRT.@)
 */
int CDECL MSVCRT__toupper(int c)
{
    return c - 0x20;  /* sic */
}

/*********************************************************************
 *		_tolower (MSVCRT.@)
 */
int CDECL MSVCRT__tolower(int c)
{
    return c + 0x20;  /* sic */
}
