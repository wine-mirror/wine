/*
 * msvcrt.dll wide-char functions
 *
 * Copyright 1999 Alexandre Julliard
 * Copyright 2000 Jon Griffiths
 */
#include "msvcrt.h"
#include "winnls.h"
#include "wine/unicode.h"

DEFAULT_DEBUG_CHANNEL(msvcrt);

/* INTERNAL: MSVCRT_malloc() based wstrndup */
LPWSTR MSVCRT__wstrndup(LPCWSTR buf, unsigned int size)
{
  WCHAR* ret;
  unsigned int len = strlenW(buf), max_len;

  max_len = size <= len? size : len + 1;

  ret = MSVCRT_malloc(max_len * sizeof (WCHAR));
  if (ret)
  {
    memcpy(ret,buf,max_len * sizeof (WCHAR));
    ret[max_len] = 0;
  }
  return ret;
}

/*********************************************************************
 *		MSVCRT__wcsdup (MSVCRT.@)
 */
LPWSTR __cdecl MSVCRT__wcsdup( LPCWSTR str )
{
  LPWSTR ret = NULL;
  if (str)
  {
    int size = (strlenW(str) + 1) * sizeof(WCHAR);
    ret = MSVCRT_malloc( size );
    if (ret) memcpy( ret, str, size );
  }
  return ret;
}

/*********************************************************************
 *		MSVCRT__wcsicoll (MSVCRT.@)
 */
INT __cdecl MSVCRT__wcsicoll( LPCWSTR str1, LPCWSTR str2 )
{
  /* FIXME: handle collates */
  return strcmpiW( str1, str2 );
}

/*********************************************************************
 *		MSVCRT__wcsnset (MSVCRT.@)
 */
LPWSTR __cdecl MSVCRT__wcsnset( LPWSTR str, WCHAR c, INT n )
{
  LPWSTR ret = str;
  while ((n-- > 0) && *str) *str++ = c;
  return ret;
}

/*********************************************************************
 *		MSVCRT__wcsrev (MSVCRT.@)
 */
LPWSTR __cdecl MSVCRT__wcsrev( LPWSTR str )
{
  LPWSTR ret = str;
  LPWSTR end = str + strlenW(str) - 1;
  while (end > str)
  {
    WCHAR t = *end;
    *end--  = *str;
    *str++  = t;
  }
  return ret;
}

/*********************************************************************
 *		MSVCRT__wcsset (MSVCRT.@)
 */
LPWSTR __cdecl MSVCRT__wcsset( LPWSTR str, WCHAR c )
{
  LPWSTR ret = str;
  while (*str) *str++ = c;
  return ret;
}

/*********************************************************************
 *		MSVCRT_wcscoll (MSVCRT.@)
 */
DWORD __cdecl MSVCRT_wcscoll( LPCWSTR str1, LPCWSTR str2 )
{
  /* FIXME: handle collates */
  return strcmpW( str1, str2 );
}

/*********************************************************************
 *		MSVCRT_wcspbrk (MSVCRT.@)
 */
LPWSTR __cdecl MSVCRT_wcspbrk( LPCWSTR str, LPCWSTR accept )
{
  LPCWSTR p;
  while (*str)
  {
    for (p = accept; *p; p++) if (*p == *str) return (LPWSTR)str;
      str++;
  }
  return NULL;
}

/*********************************************************************
 *		MSVCRT_wctomb (MSVCRT.@)
 */
INT __cdecl MSVCRT_wctomb( LPSTR dst, WCHAR ch )
{
  return WideCharToMultiByte( CP_ACP, 0, &ch, 1, dst, 6, NULL, NULL );
}

/*********************************************************************
 *		MSVCRT_iswalnum (MSVCRT.@)
 */
INT __cdecl MSVCRT_iswalnum( WCHAR wc )
{
  return get_char_typeW(wc) & (C1_ALPHA|C1_DIGIT|C1_LOWER|C1_UPPER);
}

/*********************************************************************
 *		MSVCRT_iswalpha (MSVCRT.@)
 */
INT __cdecl MSVCRT_iswalpha( WCHAR wc )
{
  return get_char_typeW(wc) & (C1_ALPHA|C1_LOWER|C1_UPPER);
}

/*********************************************************************
 *		MSVCRT_iswcntrl (MSVCRT.@)
 */
INT __cdecl MSVCRT_iswcntrl( WCHAR wc )
{
  return get_char_typeW(wc) & C1_CNTRL;
}

/*********************************************************************
 *		MSVCRT_iswdigit (MSVCRT.@)
 */
INT __cdecl MSVCRT_iswdigit( WCHAR wc )
{
  return get_char_typeW(wc) & C1_DIGIT;
}

/*********************************************************************
 *		MSVCRT_iswgraph (MSVCRT.@)
 */
INT __cdecl MSVCRT_iswgraph( WCHAR wc )
{
  return get_char_typeW(wc) & (C1_ALPHA|C1_PUNCT|C1_DIGIT|C1_LOWER|C1_UPPER);
}

/*********************************************************************
 *		MSVCRT_iswlower (MSVCRT.@)
 */
INT __cdecl MSVCRT_iswlower( WCHAR wc )
{
  return get_char_typeW(wc) & C1_LOWER;
}

/*********************************************************************
 *		MSVCRT_iswprint (MSVCRT.@)
 */
INT __cdecl MSVCRT_iswprint( WCHAR wc )
{
  return get_char_typeW(wc) & (C1_ALPHA|C1_BLANK|C1_PUNCT|C1_DIGIT|C1_LOWER|C1_UPPER);
}

/*********************************************************************
 *		MSVCRT_iswpunct (MSVCRT.@)
 */
INT __cdecl MSVCRT_iswpunct( WCHAR wc )
{
  return get_char_typeW(wc) & C1_PUNCT;
}

/*********************************************************************
 *		MSVCRT_iswspace (MSVCRT.@)
 */
INT __cdecl MSVCRT_iswspace( WCHAR wc )
{
  return get_char_typeW(wc) & C1_SPACE;
}

/*********************************************************************
 *		MSVCRT_iswupper (MSVCRT.@)
 */
INT __cdecl MSVCRT_iswupper( WCHAR wc )
{
  return get_char_typeW(wc) & C1_UPPER;
}

/*********************************************************************
 *		MSVCRT_iswxdigit (MSVCRT.@)
 */
INT __cdecl MSVCRT_iswxdigit( WCHAR wc )
{
  return get_char_typeW(wc) & C1_XDIGIT;
}

extern LPSTR  __cdecl _itoa( long , LPSTR , INT);
extern LPSTR  __cdecl _ultoa( long , LPSTR , INT);
extern LPSTR  __cdecl _ltoa( long , LPSTR , INT);

/*********************************************************************
 *		_itow (MSVCRT.@)
 */
WCHAR* __cdecl MSVCRT__itow(int value,WCHAR* out,int base)
{
  char buf[64];
  _itoa(value, buf, base);
  MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, buf, -1, out, 128);
  return out;
}

/*********************************************************************
 *		_ltow (MSVCRT.@)
 */
WCHAR* __cdecl MSVCRT__ltow(long value,WCHAR* out,int base)
{
  char buf[128];
  _ltoa(value, buf, base);
  MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, buf, -1, out, 128);
  return out;
}

/*********************************************************************
 *		_ultow (MSVCRT.@)
 */
WCHAR* __cdecl MSVCRT__ultow(unsigned long value,WCHAR* out,int base)
{
  char buf[128];
  _ultoa(value, buf, base);
  MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, buf, -1, out, 128);
  return out;
}

