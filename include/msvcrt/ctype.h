/*
 * Character type definitions
 *
 * Derived from the mingw header written by Colin Peters.
 * Modified for Wine use by Jon Griffiths and Francois Gouget.
 * This file is in the public domain.
 */
#ifndef __WINE_CTYPE_H
#define __WINE_CTYPE_H
#ifndef __WINE_USE_MSVCRT
#define __WINE_USE_MSVCRT
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _WCHAR_T_DEFINED
#define _WCHAR_T_DEFINED
#ifndef __cplusplus
typedef unsigned short wchar_t;
#endif
#endif

#ifndef WEOF
#define WEOF        (wint_t)(0xFFFF)
#endif

#ifndef _WCTYPE_T_DEFINED
typedef unsigned short  wint_t;
typedef unsigned short  wctype_t;
#define _WCTYPE_T_DEFINED
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

int __isascii(int);
int __iscsym(int);
int __iscsymf(int);
int __toascii(int);
int _isctype(int,int);
int _tolower(int);
int _toupper(int);
int isalnum(int);
int isalpha(int);
int iscntrl(int);
int isdigit(int);
int isgraph(int);
int islower(int);
int isprint(int);
int ispunct(int);
int isspace(int);
int isupper(int);
int isxdigit(int);
int tolower(int);
int toupper(int);

#ifndef _WCTYPE_DEFINED
#define _WCTYPE_DEFINED
int is_wctype(wint_t,wctype_t);
int isleadbyte(int);
int iswalnum(wint_t);
int iswalpha(wint_t);
int iswascii(wint_t);
int iswcntrl(wint_t);
int iswctype(wint_t,wctype_t);
int iswdigit(wint_t);
int iswgraph(wint_t);
int iswlower(wint_t);
int iswprint(wint_t);
int iswpunct(wint_t);
int iswspace(wint_t);
int iswupper(wint_t);
int iswxdigit(wint_t);
wchar_t towlower(wchar_t);
wchar_t towupper(wchar_t);
#endif /* _WCTYPE_DEFINED */

#ifdef __cplusplus
}
#endif


static inline int isascii(int c) { return __isascii(c); }
static inline int iscsym(int c) { return __iscsym(c); }
static inline int iscsymf(int c) { return __iscsymf(c); }
static inline int toascii(int c) { return __toascii(c); }

#endif /* __WINE_CTYPE_H */
