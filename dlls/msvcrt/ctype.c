/*
 * msvcrt.dll ctype functions
 *
 * Copyright 2000 Jon Griffiths
 */
#include "msvcrt.h"

DEFAULT_DEBUG_CHANNEL(msvcrt);

/* ASCII char classification table - binary compatible */
#define MSVCRT_UPPER		C1_UPPER
#define MSVCRT_LOWER		C1_LOWER
#define MSVCRT_DIGIT		C1_DIGIT
#define MSVCRT_SPACE		C1_SPACE
#define MSVCRT_PUNCT		C1_PUNCT
#define MSVCRT_CONTROL		C1_CNTRL
#define MSVCRT_BLANK		C1_BLANK
#define MSVCRT_HEX		C1_XDIGIT
#define MSVCRT_LEADBYTE  0x8000
#define MSVCRT_ALPHA    (C1_ALPHA|MSVCRT_UPPER|MSVCRT_LOWER)

#define _C_ MSVCRT_CONTROL
#define _S_ MSVCRT_SPACE
#define _P_ MSVCRT_PUNCT
#define _D_ MSVCRT_DIGIT
#define _H_ MSVCRT_HEX
#define _U_ MSVCRT_UPPER
#define _L_ MSVCRT_LOWER

WORD MSVCRT__ctype [257] = {
  0, _C_, _C_, _C_, _C_, _C_, _C_, _C_, _C_, _C_, _S_|_C_, _S_|_C_,
  _S_|_C_, _S_|_C_, _S_|_C_, _C_, _C_, _C_, _C_, _C_, _C_, _C_, _C_,
  _C_, _C_, _C_, _C_, _C_, _C_, _C_, _C_, _C_, _C_, _S_|MSVCRT_BLANK,
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

/* Internal: Current ctype table for locale */
WORD MSVCRT_current_ctype[257];

/* pctype is used by macros in the Win32 headers. It must point
 * To a table of flags exactly like ctype. To allow locale
 * changes to affect ctypes (i.e. isleadbyte), we use a second table
 * and update its flags whenever the current locale changes.
 */
WORD* MSVCRT__pctype = MSVCRT_current_ctype + 1;

/* mbctype data */
extern int MSVCRT___mb_cur_max;
extern LCID MSVCRT_current_lc_all_lcid;

/*********************************************************************
 *		MSVCRT___p__pctype (MSVCRT.@)
 */
WORD** MSVCRT___p__pctype(void)
{
  return &MSVCRT__pctype;
}

/*********************************************************************
 *		_isctype (MSVCRT.@)
 */
int __cdecl MSVCRT__isctype(int c, int type)
{
  if (c >= -1 && c <= 255)
    return MSVCRT__pctype[c] & type;

  if (MSVCRT___mb_cur_max != 1 && c > 0)
  {
    /* FIXME: Is there a faster way to do this? */
    WORD typeInfo;
    char convert[3], *pconv = convert;

    if (MSVCRT__pctype[(UINT)c >> 8] & MSVCRT_LEADBYTE)
      *pconv++ = (UINT)c >> 8;
    *pconv++ = c & 0xff;
    *pconv = 0;
    /* FIXME: Use ctype LCID, not lc_all */
    if (GetStringTypeExA(MSVCRT_current_lc_all_lcid, CT_CTYPE1,
                         convert, convert[1] ? 2 : 1, &typeInfo))
      return typeInfo & type;
  }
  return 0;
}

/*********************************************************************
 *		isalnum (MSVCRT.@)
 */
int __cdecl MSVCRT_isalnum(int c)
{
  return MSVCRT__isctype( c,MSVCRT_ALPHA | MSVCRT_DIGIT );
}

/*********************************************************************
 *		isalpha (MSVCRT.@)
 */
int __cdecl MSVCRT_isalpha(int c)
{
  return MSVCRT__isctype( c, MSVCRT_ALPHA );
}

/*********************************************************************
 *		iscntrl (MSVCRT.@)
 */
int __cdecl MSVCRT_iscntrl(int c)
{
  return MSVCRT__isctype( c, MSVCRT_CONTROL );
}

/*********************************************************************
 *		isdigit (MSVCRT.@)
 */
int __cdecl MSVCRT_isdigit(int c)
{
  return MSVCRT__isctype( c, MSVCRT_DIGIT );
}

/*********************************************************************
 *		isgraph (MSVCRT.@)
 */
int __cdecl MSVCRT_isgraph(int c)
{
  return MSVCRT__isctype( c, MSVCRT_ALPHA | MSVCRT_DIGIT | MSVCRT_PUNCT );
}

/*********************************************************************
 *		isleadbyte (MSVCRT.@)
 */
int __cdecl MSVCRT_isleadbyte(int c)
{
  return MSVCRT__isctype( c, MSVCRT_LEADBYTE );
}

/*********************************************************************
 *		islower (MSVCRT.@)
 */
int __cdecl MSVCRT_islower(int c)
{
  return MSVCRT__isctype( c, MSVCRT_LOWER );
}

/*********************************************************************
 *		isprint (MSVCRT.@)
 */
int __cdecl MSVCRT_isprint(int c)
{
  return MSVCRT__isctype( c, MSVCRT_ALPHA | MSVCRT_DIGIT |
			  MSVCRT_BLANK | MSVCRT_PUNCT );
}

/*********************************************************************
 *		ispunct (MSVCRT.@)
 */
int __cdecl MSVCRT_ispunct(int c)
{
  return MSVCRT__isctype( c, MSVCRT_PUNCT );
}

/*********************************************************************
 *		isspace (MSVCRT.@)
 */
int __cdecl MSVCRT_isspace(int c)
{
  return MSVCRT__isctype( c, MSVCRT_SPACE );
}

/*********************************************************************
 *		isupper (MSVCRT.@)
 */
int __cdecl MSVCRT_isupper(int c)
{
  return MSVCRT__isctype( c, MSVCRT_UPPER );
}

/*********************************************************************
 *		isxdigit (MSVCRT.@)
 */
int __cdecl MSVCRT_isxdigit(int c)
{
  return MSVCRT__isctype( c, MSVCRT_HEX );
}

/*********************************************************************
 *		__isascii (MSVCRT.@)
 */
int __cdecl MSVCRT___isascii(int c)
{
  return isascii((unsigned)c);
}

/*********************************************************************
 *		__toascii (MSVCRT.@)
 */
int __cdecl MSVCRT___toascii(int c)
{
  return (unsigned)c & 0x7f;
}

/*********************************************************************
 *		iswascii (MSVCRT.@)
 *
 */
int __cdecl MSVCRT_iswascii(WCHAR c)
{
  return ((unsigned)c < 0x80);
}

/*********************************************************************
 *		__iscsym (MSVCRT.@)
 */
int __cdecl MSVCRT___iscsym(int c)
{
  return (c < 127 && (isalnum(c) || c == '_'));
}

/*********************************************************************
 *		__iscsymf (MSVCRT.@)
 */
int __cdecl MSVCRT___iscsymf(int c)
{
  return (c < 127 && (isalpha(c) || c == '_'));
}

/*********************************************************************
 *		_toupper (MSVCRT.@)
 */
int __cdecl MSVCRT__toupper(int c)
{
  return toupper(c);
}

/*********************************************************************
 *		_tolower (MSVCRT.@)
 */
int __cdecl MSVCRT__tolower(int c)
{
  return tolower(c);
}
