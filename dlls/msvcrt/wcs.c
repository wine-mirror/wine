/*
 * msvcrt.dll wide-char functions
 *
 * Copyright 1999 Alexandre Julliard
 * Copyright 2000 Jon Griffiths
 */
#include <limits.h>
#include <stdio.h>
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
 *		_wcsdup (MSVCRT.@)
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
 *		_wcsicoll (MSVCRT.@)
 */
INT __cdecl MSVCRT__wcsicoll( LPCWSTR str1, LPCWSTR str2 )
{
  /* FIXME: handle collates */
  return strcmpiW( str1, str2 );
}

/*********************************************************************
 *		_wcsnset (MSVCRT.@)
 */
LPWSTR __cdecl MSVCRT__wcsnset( LPWSTR str, WCHAR c, INT n )
{
  LPWSTR ret = str;
  while ((n-- > 0) && *str) *str++ = c;
  return ret;
}

/*********************************************************************
 *		_wcsrev (MSVCRT.@)
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
 *		_wcsset (MSVCRT.@)
 */
LPWSTR __cdecl MSVCRT__wcsset( LPWSTR str, WCHAR c )
{
  LPWSTR ret = str;
  while (*str) *str++ = c;
  return ret;
}

/*********************************************************************
 *		_vsnwprintf (MSVCRT.@)
 */
int __cdecl MSVCRT__vsnwprintf(WCHAR *str, unsigned int len,
                              const WCHAR *format, va_list valist)
{
/* If you fix a bug in this function, fix it in ntdll/wcstring.c also! */
  unsigned int written = 0;
  const WCHAR *iter = format;
  char bufa[256], fmtbufa[64], *fmta;

  TRACE("(%d,%s)\n",len,debugstr_w(format));

  while (*iter)
  {
    while (*iter && *iter != (WCHAR)L'%')
    {
     if (written++ >= len)
       return -1;
     *str++ = *iter++;
    }
    if (*iter == (WCHAR)L'%')
    {
      fmta = fmtbufa;
      *fmta++ = *iter++;
      while (*iter == (WCHAR)L'0' ||
             *iter == (WCHAR)L'+' ||
             *iter == (WCHAR)L'-' ||
             *iter == (WCHAR)L' ' ||
             *iter == (WCHAR)L'0' ||
             *iter == (WCHAR)L'*' ||
             *iter == (WCHAR)L'#')
      {
        if (*iter == (WCHAR)L'*')
        {
          char *buffiter = bufa;
          int fieldlen = va_arg(valist, int);
          sprintf(buffiter, "%d", fieldlen);
          while (*buffiter)
            *fmta++ = *buffiter++;
        }
        else
          *fmta++ = *iter;
        iter++;
      }

      while (isdigit(*iter))
        *fmta++ = *iter++;

      if (*iter == (WCHAR)L'.')
      {
        *fmta++ = *iter++;
        if (*iter == (WCHAR)L'*')
        {
          char *buffiter = bufa;
          int fieldlen = va_arg(valist, int);
          sprintf(buffiter, "%d", fieldlen);
          while (*buffiter)
            *fmta++ = *buffiter++;
        }
        else
          while (isdigit(*iter))
            *fmta++ = *iter++;
      }
      if (*iter == (WCHAR)L'h' ||
          *iter == (WCHAR)L'l')
      {
          *fmta++ = *iter++;
          *fmta++ = *iter++;
      }

      switch (*iter)
      {
      case (WCHAR)L's':
        {
          static const WCHAR none[] = { '(', 'n', 'u', 'l', 'l', ')', 0 };
          const WCHAR *wstr = va_arg(valist, const WCHAR *);
          const WCHAR *striter = wstr ? wstr : none;
          while (*striter)
          {
            if (written++ >= len)
              return -1;
            *str++ = *striter++;
          }
          iter++;
          break;
        }

      case (WCHAR)L'c':
        if (written++ >= len)
          return -1;
        *str++ = va_arg(valist, WCHAR);
        iter++;
        break;

      default:
        {
          /* For non wc types, use system sprintf and append to wide char output */
          /* FIXME: for unrecognised types, should ignore % when printing */
          char *bufaiter = bufa;
          if (*iter == (WCHAR)L'p')
            sprintf(bufaiter, "%08lX", va_arg(valist, long));
          else
          {
            *fmta++ = *iter;
            *fmta = '\0';
            sprintf(bufaiter, fmtbufa, va_arg(valist, void *));
          }
          while (*bufaiter)
          {
            if (written++ >= len)
              return -1;
            *str++ = *bufaiter++;
          }
          iter++;
          break;
        }
      }
    }
  }
  if (written >= len)
    return -1;
  *str++ = (WCHAR)L'\0';
  return (int)written;
}

/*********************************************************************
 *		vswprintf (MSVCRT.@)
 */
int __cdecl MSVCRT_vswprintf( LPWSTR str, LPCWSTR format, va_list args )
{
  return MSVCRT__vsnwprintf( str, INT_MAX, format, args );
}

/*********************************************************************
 *		wcscoll (MSVCRT.@)
 */
DWORD __cdecl MSVCRT_wcscoll( LPCWSTR str1, LPCWSTR str2 )
{
  /* FIXME: handle collates */
  return strcmpW( str1, str2 );
}

/*********************************************************************
 *		wcspbrk (MSVCRT.@)
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
 *		wctomb (MSVCRT.@)
 */
INT __cdecl MSVCRT_wctomb( char *dst, WCHAR ch )
{
  return WideCharToMultiByte( CP_ACP, 0, &ch, 1, dst, 6, NULL, NULL );
}

/*********************************************************************
 *		iswalnum (MSVCRT.@)
 */
INT __cdecl MSVCRT_iswalnum( WCHAR wc )
{
  return get_char_typeW(wc) & (C1_ALPHA|C1_DIGIT|C1_LOWER|C1_UPPER);
}

/*********************************************************************
 *		iswalpha (MSVCRT.@)
 */
INT __cdecl MSVCRT_iswalpha( WCHAR wc )
{
  return get_char_typeW(wc) & (C1_ALPHA|C1_LOWER|C1_UPPER);
}

/*********************************************************************
 *		iswcntrl (MSVCRT.@)
 */
INT __cdecl MSVCRT_iswcntrl( WCHAR wc )
{
  return get_char_typeW(wc) & C1_CNTRL;
}

/*********************************************************************
 *		iswdigit (MSVCRT.@)
 */
INT __cdecl MSVCRT_iswdigit( WCHAR wc )
{
  return get_char_typeW(wc) & C1_DIGIT;
}

/*********************************************************************
 *		iswgraph (MSVCRT.@)
 */
INT __cdecl MSVCRT_iswgraph( WCHAR wc )
{
  return get_char_typeW(wc) & (C1_ALPHA|C1_PUNCT|C1_DIGIT|C1_LOWER|C1_UPPER);
}

/*********************************************************************
 *		iswlower (MSVCRT.@)
 */
INT __cdecl MSVCRT_iswlower( WCHAR wc )
{
  return get_char_typeW(wc) & C1_LOWER;
}

/*********************************************************************
 *		iswprint (MSVCRT.@)
 */
INT __cdecl MSVCRT_iswprint( WCHAR wc )
{
  return get_char_typeW(wc) & (C1_ALPHA|C1_BLANK|C1_PUNCT|C1_DIGIT|C1_LOWER|C1_UPPER);
}

/*********************************************************************
 *		iswpunct (MSVCRT.@)
 */
INT __cdecl MSVCRT_iswpunct( WCHAR wc )
{
  return get_char_typeW(wc) & C1_PUNCT;
}

/*********************************************************************
 *		iswspace (MSVCRT.@)
 */
INT __cdecl MSVCRT_iswspace( WCHAR wc )
{
  return get_char_typeW(wc) & C1_SPACE;
}

/*********************************************************************
 *		iswupper (MSVCRT.@)
 */
INT __cdecl MSVCRT_iswupper( WCHAR wc )
{
  return get_char_typeW(wc) & C1_UPPER;
}

/*********************************************************************
 *		iswxdigit (MSVCRT.@)
 */
INT __cdecl MSVCRT_iswxdigit( WCHAR wc )
{
  return get_char_typeW(wc) & C1_XDIGIT;
}

extern char *__cdecl _itoa( long , char *, int);
extern char *__cdecl _ultoa( long , char *, int);
extern char *__cdecl _ltoa( long , char *, int);

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

