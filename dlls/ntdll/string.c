/*
 * NTDLL string functions
 *
 * Copyright 2000 Alexandre Julliard
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

#include "config.h"

#include <ctype.h>
#include <string.h>

#include "windef.h"

/*********************************************************************
 *                  _memicmp   (NTDLL.@)
 */
INT __cdecl NTDLL__memicmp( LPCSTR s1, LPCSTR s2, DWORD len )
{
    int ret = 0;
    while (len--)
    {
        if ((ret = tolower(*s1) - tolower(*s2))) break;
        s1++;
        s2++;
    }
    return ret;
}

/*********************************************************************
 *                  _strupr   (NTDLL.@)
 */
LPSTR __cdecl _strupr( LPSTR str )
{
    LPSTR ret = str;
    for ( ; *str; str++) *str = toupper(*str);
    return ret;
}

/*********************************************************************
 *                  _strlwr   (NTDLL.@)
 *
 * convert a string in place to lowercase
 */
LPSTR __cdecl _strlwr( LPSTR str )
{
    LPSTR ret = str;
    for ( ; *str; str++) *str = tolower(*str);
    return ret;
}


/*********************************************************************
 *                  _ultoa   (NTDLL.@)
 */
LPSTR  __cdecl _ultoa( unsigned long x, LPSTR buf, INT radix )
{
    char *p, buffer[8*sizeof(unsigned long) + 1];  /* assume 8-bit chars */

    p = buffer + sizeof(buffer);
    *--p = 0;
    do
    {
        int rem = x % radix;
        *--p = (rem <= 9) ? rem + '0' : rem + 'a' - 10;
        x /= radix;
    } while (x);
    strcpy( buf, p );
    return buf;
}


/*********************************************************************
 *                  _ltoa   (NTDLL.@)
 */
LPSTR  __cdecl _ltoa( long x, LPSTR buf, INT radix )
{
    LPSTR p = buf;
    if (x < 0)
    {
        *p++ = '-';
        x = -x;
    }
    _ultoa( x, p, radix );
    return buf;
}


/*********************************************************************
 *                  _itoa           (NTDLL.@)
 */
LPSTR  __cdecl _itoa( int x, LPSTR buf, INT radix )
{
    return _ltoa( x, buf, radix );
}


/*********************************************************************
 *		_splitpath (NTDLL.@)
 */
void __cdecl _splitpath(const char* inpath, char * drv, char * dir,
                        char* fname, char * ext )
{
  /* Modified PD code from 'snippets' collection. */
  char ch, *ptr, *p;
  char pathbuff[MAX_PATH], *path=pathbuff;

  strcpy(pathbuff, inpath);

  /* convert slashes to backslashes for searching */
  for (ptr = (char*)path; *ptr; ++ptr)
    if ('/' == *ptr)
      *ptr = '\\';

  /* look for drive spec */
  if ('\0' != (ptr = strchr(path, ':')))
  {
    ++ptr;
    if (drv)
    {
      strncpy(drv, path, ptr - path);
      drv[ptr - path] = '\0';
    }
    path = ptr;
  }
  else if (drv)
    *drv = '\0';

  /* find rightmost backslash or leftmost colon */
  if (NULL == (ptr = strrchr(path, '\\')))
    ptr = (strchr(path, ':'));

  if (!ptr)
  {
    ptr = (char *)path; /* no path */
    if (dir)
      *dir = '\0';
  }
  else
  {
    ++ptr; /* skip the delimiter */
    if (dir)
    {
      ch = *ptr;
      *ptr = '\0';
      strcpy(dir, path);
      *ptr = ch;
    }
  }

  if (NULL == (p = strrchr(ptr, '.')))
  {
    if (fname)
      strcpy(fname, ptr);
    if (ext)
      *ext = '\0';
  }
  else
  {
    *p = '\0';
    if (fname)
      strcpy(fname, ptr);
    *p = '.';
    if (ext)
      strcpy(ext, p);
  }

  /* Fix pathological case - Win returns ':' as part of the
   * directory when no drive letter is given.
   */
  if (drv && drv[0] == ':')
  {
    *drv = '\0';
    if (dir)
    {
      pathbuff[0] = ':';
      pathbuff[1] = '\0';
      strcat(pathbuff,dir);
      strcpy(dir,pathbuff);
    }
  }
}
