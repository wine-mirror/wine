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
MSVCRT_wchar_t* msvcrt_wstrndup(const MSVCRT_wchar_t *buf, unsigned int size)
{
  MSVCRT_wchar_t* ret;
  unsigned int len = strlenW(buf), max_len;

  max_len = size <= len? size : len + 1;

  ret = MSVCRT_malloc(max_len * sizeof (MSVCRT_wchar_t));
  if (ret)
  {
    memcpy(ret,buf,max_len * sizeof (MSVCRT_wchar_t));
    ret[max_len] = 0;
  }
  return ret;
}

/*********************************************************************
 *		_wcsdup (MSVCRT.@)
 */
MSVCRT_wchar_t* _wcsdup( const MSVCRT_wchar_t* str )
{
  MSVCRT_wchar_t* ret = NULL;
  if (str)
  {
    int size = (strlenW(str) + 1) * sizeof(MSVCRT_wchar_t);
    ret = MSVCRT_malloc( size );
    if (ret) memcpy( ret, str, size );
  }
  return ret;
}

/*********************************************************************
 *		_wcsicoll (MSVCRT.@)
 */
INT _wcsicoll( const MSVCRT_wchar_t* str1, const MSVCRT_wchar_t* str2 )
{
  /* FIXME: handle collates */
  return strcmpiW( str1, str2 );
}

/*********************************************************************
 *		_wcsnset (MSVCRT.@)
 */
MSVCRT_wchar_t* _wcsnset( MSVCRT_wchar_t* str, MSVCRT_wchar_t c, MSVCRT_size_t n )
{
  MSVCRT_wchar_t* ret = str;
  while ((n-- > 0) && *str) *str++ = c;
  return ret;
}

/*********************************************************************
 *		_wcsrev (MSVCRT.@)
 */
MSVCRT_wchar_t* _wcsrev( MSVCRT_wchar_t* str )
{
  MSVCRT_wchar_t* ret = str;
  MSVCRT_wchar_t* end = str + strlenW(str) - 1;
  while (end > str)
  {
    MSVCRT_wchar_t t = *end;
    *end--  = *str;
    *str++  = t;
  }
  return ret;
}

/*********************************************************************
 *		_wcsset (MSVCRT.@)
 */
MSVCRT_wchar_t* _wcsset( MSVCRT_wchar_t* str, MSVCRT_wchar_t c )
{
  MSVCRT_wchar_t* ret = str;
  while (*str) *str++ = c;
  return ret;
}

/*********************************************************************
 *		_vsnwprintf (MSVCRT.@)
 */
int _vsnwprintf(MSVCRT_wchar_t *str, unsigned int len,
                const MSVCRT_wchar_t *format, va_list valist)
{
/* If you fix a bug in this function, fix it in ntdll/wcstring.c also! */
  unsigned int written = 0;
  const MSVCRT_wchar_t *iter = format;
  char bufa[256], fmtbufa[64], *fmta;

  TRACE("(%d,%s)\n",len,debugstr_w(format));

  while (*iter)
  {
    while (*iter && *iter != '%')
    {
     if (written++ >= len)
       return -1;
     *str++ = *iter++;
    }
    if (*iter == '%')
    {
      fmta = fmtbufa;
      *fmta++ = *iter++;
      while (*iter == '0' ||
             *iter == '+' ||
             *iter == '-' ||
             *iter == ' ' ||
             *iter == '0' ||
             *iter == '*' ||
             *iter == '#')
      {
        if (*iter == '*')
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

      if (*iter == '.')
      {
        *fmta++ = *iter++;
        if (*iter == '*')
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
      if (*iter == 'h' ||
          *iter == 'l')
          *fmta++ = *iter++;

      switch (*iter)
      {
      case 's':
        {
          static const MSVCRT_wchar_t none[] = { '(', 'n', 'u', 'l', 'l', ')', 0 };
          const MSVCRT_wchar_t *wstr = va_arg(valist, const MSVCRT_wchar_t *);
          const MSVCRT_wchar_t *striter = wstr ? wstr : none;
          while (*striter)
          {
            if (written++ >= len)
              return -1;
            *str++ = *striter++;
          }
          iter++;
          break;
        }

      case 'c':
        if (written++ >= len)
          return -1;
        *str++ = (MSVCRT_wchar_t)va_arg(valist, int);
        iter++;
        break;

      default:
        {
          /* For non wc types, use system sprintf and append to wide char output */
          /* FIXME: for unrecognised types, should ignore % when printing */
          char *bufaiter = bufa;
          if (*iter == 'p')
            sprintf(bufaiter, "%08lX", va_arg(valist, long));
          else
          {
            *fmta++ = *iter;
            *fmta = '\0';
            if (*iter == 'f')
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
  *str++ = 0;
  return (int)written;
}

/*********************************************************************
 *		vswprintf (MSVCRT.@)
 */
int MSVCRT_vswprintf( MSVCRT_wchar_t* str, const MSVCRT_wchar_t* format, va_list args )
{
  return _vsnwprintf( str, INT_MAX, format, args );
}

/*********************************************************************
 *		wcscoll (MSVCRT.@)
 */
int MSVCRT_wcscoll( const MSVCRT_wchar_t* str1, const MSVCRT_wchar_t* str2 )
{
  /* FIXME: handle collates */
  return strcmpW( str1, str2 );
}

/*********************************************************************
 *		wcspbrk (MSVCRT.@)
 */
MSVCRT_wchar_t* MSVCRT_wcspbrk( const MSVCRT_wchar_t* str, const MSVCRT_wchar_t* accept )
{
  const MSVCRT_wchar_t* p;
  while (*str)
  {
    for (p = accept; *p; p++) if (*p == *str) return (MSVCRT_wchar_t*)str;
      str++;
  }
  return NULL;
}

/*********************************************************************
 *		wctomb (MSVCRT.@)
 */
INT MSVCRT_wctomb( char *dst, MSVCRT_wchar_t ch )
{
  return WideCharToMultiByte( CP_ACP, 0, &ch, 1, dst, 6, NULL, NULL );
}

/*********************************************************************
 *		iswalnum (MSVCRT.@)
 */
INT MSVCRT_iswalnum( MSVCRT_wchar_t wc )
{
    return isalnumW( wc );
}

/*********************************************************************
 *		iswalpha (MSVCRT.@)
 */
INT MSVCRT_iswalpha( MSVCRT_wchar_t wc )
{
    return isalphaW( wc );
}

/*********************************************************************
 *		iswcntrl (MSVCRT.@)
 */
INT MSVCRT_iswcntrl( MSVCRT_wchar_t wc )
{
    return iscntrlW( wc );
}

/*********************************************************************
 *		iswdigit (MSVCRT.@)
 */
INT MSVCRT_iswdigit( MSVCRT_wchar_t wc )
{
    return isdigitW( wc );
}

/*********************************************************************
 *		iswgraph (MSVCRT.@)
 */
INT MSVCRT_iswgraph( MSVCRT_wchar_t wc )
{
    return isgraphW( wc );
}

/*********************************************************************
 *		iswlower (MSVCRT.@)
 */
INT MSVCRT_iswlower( MSVCRT_wchar_t wc )
{
    return islowerW( wc );
}

/*********************************************************************
 *		iswprint (MSVCRT.@)
 */
INT MSVCRT_iswprint( MSVCRT_wchar_t wc )
{
    return isprintW( wc );
}

/*********************************************************************
 *		iswpunct (MSVCRT.@)
 */
INT MSVCRT_iswpunct( MSVCRT_wchar_t wc )
{
    return ispunctW( wc );
}

/*********************************************************************
 *		iswspace (MSVCRT.@)
 */
INT MSVCRT_iswspace( MSVCRT_wchar_t wc )
{
    return isspaceW( wc );
}

/*********************************************************************
 *		iswupper (MSVCRT.@)
 */
INT MSVCRT_iswupper( MSVCRT_wchar_t wc )
{
    return isupperW( wc );
}

/*********************************************************************
 *		iswxdigit (MSVCRT.@)
 */
INT MSVCRT_iswxdigit( MSVCRT_wchar_t wc )
{
    return isxdigitW( wc );
}
