/*
 * msvcrt.dll wide-char functions
 *
 * Copyright 1999 Alexandre Julliard
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <limits.h>
#include <stdio.h>
#include "msvcrt.h"
#include "winnls.h"
#include "wine/unicode.h"

#include "msvcrt/stdio.h"
#include "msvcrt/stdlib.h"
#include "msvcrt/string.h"
#include "msvcrt/wctype.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);


/* INTERNAL: MSVCRT_malloc() based wstrndup */
WCHAR* msvcrt_wstrndup(LPCWSTR buf, unsigned int size)
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
WCHAR* _wcsdup( const WCHAR* str )
{
  WCHAR* ret = NULL;
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
INT _wcsicoll( const WCHAR* str1, const WCHAR* str2 )
{
  /* FIXME: handle collates */
  return strcmpiW( str1, str2 );
}

/*********************************************************************
 *		_wcsnset (MSVCRT.@)
 */
WCHAR* _wcsnset( WCHAR* str, WCHAR c, MSVCRT_size_t n )
{
  WCHAR* ret = str;
  while ((n-- > 0) && *str) *str++ = c;
  return ret;
}

/*********************************************************************
 *		_wcsrev (MSVCRT.@)
 */
WCHAR* _wcsrev( WCHAR* str )
{
  WCHAR* ret = str;
  WCHAR* end = str + strlenW(str) - 1;
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
WCHAR* _wcsset( WCHAR* str, WCHAR c )
{
  WCHAR* ret = str;
  while (*str) *str++ = c;
  return ret;
}

/*********************************************************************
 *		_vsnwprintf (MSVCRT.@)
 */
int _vsnwprintf(WCHAR *str, unsigned int len,
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
          *fmta++ = *iter++;

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
        *str++ = (WCHAR)va_arg(valist, int);
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
            if (*iter == (WCHAR)L'f')
              sprintf(bufaiter, fmtbufa, va_arg(valist, double));
            else
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
int MSVCRT_vswprintf( WCHAR* str, const WCHAR* format, va_list args )
{
  return _vsnwprintf( str, INT_MAX, format, args );
}

/*********************************************************************
 *		wcscoll (MSVCRT.@)
 */
int MSVCRT_wcscoll( const WCHAR* str1, const WCHAR* str2 )
{
  /* FIXME: handle collates */
  return strcmpW( str1, str2 );
}

/*********************************************************************
 *		wcspbrk (MSVCRT.@)
 */
WCHAR* MSVCRT_wcspbrk( const WCHAR* str, const WCHAR* accept )
{
  const WCHAR* p;
  while (*str)
  {
    for (p = accept; *p; p++) if (*p == *str) return (WCHAR*)str;
      str++;
  }
  return NULL;
}

/*********************************************************************
 *		wctomb (MSVCRT.@)
 */
INT MSVCRT_wctomb( char *dst, WCHAR ch )
{
  return WideCharToMultiByte( CP_ACP, 0, &ch, 1, dst, 6, NULL, NULL );
}

/*********************************************************************
 *		iswalnum (MSVCRT.@)
 */
INT MSVCRT_iswalnum( WCHAR wc )
{
    return isalnumW( wc );
}

/*********************************************************************
 *		iswalpha (MSVCRT.@)
 */
INT MSVCRT_iswalpha( WCHAR wc )
{
    return isalphaW( wc );
}

/*********************************************************************
 *		iswcntrl (MSVCRT.@)
 */
INT MSVCRT_iswcntrl( WCHAR wc )
{
    return iscntrlW( wc );
}

/*********************************************************************
 *		iswdigit (MSVCRT.@)
 */
INT MSVCRT_iswdigit( WCHAR wc )
{
    return isdigitW( wc );
}

/*********************************************************************
 *		iswgraph (MSVCRT.@)
 */
INT MSVCRT_iswgraph( WCHAR wc )
{
    return isgraphW( wc );
}

/*********************************************************************
 *		iswlower (MSVCRT.@)
 */
INT MSVCRT_iswlower( WCHAR wc )
{
    return islowerW( wc );
}

/*********************************************************************
 *		iswprint (MSVCRT.@)
 */
INT MSVCRT_iswprint( WCHAR wc )
{
    return isprintW( wc );
}

/*********************************************************************
 *		iswpunct (MSVCRT.@)
 */
INT MSVCRT_iswpunct( WCHAR wc )
{
    return ispunctW( wc );
}

/*********************************************************************
 *		iswspace (MSVCRT.@)
 */
INT MSVCRT_iswspace( WCHAR wc )
{
    return isspaceW( wc );
}

/*********************************************************************
 *		iswupper (MSVCRT.@)
 */
INT MSVCRT_iswupper( WCHAR wc )
{
    return isupperW( wc );
}

/*********************************************************************
 *		iswxdigit (MSVCRT.@)
 */
INT MSVCRT_iswxdigit( WCHAR wc )
{
    return isxdigitW( wc );
}

/*********************************************************************
 *		_itow (MSVCRT.@)
 */
WCHAR* _itow(int value,WCHAR* out,int base)
{
  char buf[64];
  _itoa(value, buf, base);
  MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, buf, -1, out, 128);
  return out;
}

/*********************************************************************
 *		_ltow (MSVCRT.@)
 */
WCHAR* _ltow(long value,WCHAR* out,int base)
{
  char buf[128];
  _ltoa(value, buf, base);
  MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, buf, -1, out, 128);
  return out;
}

