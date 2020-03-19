/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the Wine project.
 */

#ifndef _WCTYPE_DEFINED
#define _WCTYPE_DEFINED

#include <corecrt.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ASCII char classification table - binary compatible */
#define _UPPER        0x0001  /* C1_UPPER */
#define _LOWER        0x0002  /* C1_LOWER */
#define _DIGIT        0x0004  /* C1_DIGIT */
#define _SPACE        0x0008  /* C1_SPACE */
#define _PUNCT        0x0010  /* C1_PUNCT */
#define _CONTROL      0x0020  /* C1_CNTRL */
#define _BLANK        0x0040  /* C1_BLANK */
#define _HEX          0x0080  /* C1_XDIGIT */
#define _LEADBYTE     0x8000
#define _ALPHA       (0x0100|_UPPER|_LOWER)  /* (C1_ALPHA|_UPPER|_LOWER) */

int     __cdecl _iswblank_l(wint_t,_locale_t);
int     __cdecl _iswctype_l(wint_t,wctype_t,_locale_t);
wchar_t __cdecl _towlower_l(wchar_t,_locale_t);
wchar_t __cdecl _towupper_l(wchar_t,_locale_t);
int     __cdecl is_wctype(wint_t,wctype_t);
int     __cdecl isleadbyte(int);
int     __cdecl iswalnum(wint_t);
int     __cdecl iswalpha(wint_t);
int     __cdecl iswascii(wint_t);
int     __cdecl iswblank(wint_t);
int     __cdecl iswcntrl(wint_t);
int     __cdecl iswctype(wint_t,wctype_t);
int     __cdecl iswdigit(wint_t);
int     __cdecl iswgraph(wint_t);
int     __cdecl iswlower(wint_t);
int     __cdecl iswprint(wint_t);
int     __cdecl iswpunct(wint_t);
int     __cdecl iswspace(wint_t);
int     __cdecl iswupper(wint_t);
int     __cdecl iswxdigit(wint_t);
wchar_t __cdecl towlower(wchar_t);
wchar_t __cdecl towupper(wchar_t);

#ifdef __cplusplus
}
#endif

#endif /* _WCTYPE_DEFINED */
