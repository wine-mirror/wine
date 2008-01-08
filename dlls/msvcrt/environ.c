/*
 * msvcrt.dll environment functions
 *
 * Copyright 1996,1998 Marcus Meissner
 * Copyright 1996 Jukka Iivonen
 * Copyright 1997,2000 Uwe Bonnes
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
#include "wine/unicode.h"
#include "msvcrt.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

/*********************************************************************
 *		getenv (MSVCRT.@)
 */
char * CDECL MSVCRT_getenv(const char *name)
{
    char **environ;
    unsigned int length=strlen(name);

    for (environ = *__p__environ(); *environ; environ++)
    {
        char *str = *environ;
        char *pos = strchr(str,'=');
        if (pos && ((pos - str) == length) && !strncasecmp(str,name,length))
        {
            TRACE("(%s): got %s\n", debugstr_a(name), debugstr_a(pos + 1));
            return pos + 1;
        }
    }
    return NULL;
}

/*********************************************************************
 *		_wgetenv (MSVCRT.@)
 */
MSVCRT_wchar_t * CDECL _wgetenv(const MSVCRT_wchar_t *name)
{
    MSVCRT_wchar_t **environ;
    unsigned int length=strlenW(name);

    for (environ = *__p__wenviron(); *environ; environ++)
    {
        MSVCRT_wchar_t *str = *environ;
        MSVCRT_wchar_t *pos = strchrW(str,'=');
        if (pos && ((pos - str) == length) && !strncmpiW(str,name,length))
        {
            TRACE("(%s): got %s\n", debugstr_w(name), debugstr_w(pos + 1));
            return pos + 1;
        }
    }
    return NULL;
}

/*********************************************************************
 *		_putenv (MSVCRT.@)
 */
int CDECL _putenv(const char *str)
{
 char *name, *value;
 char *dst;
 int ret;

 TRACE("%s\n", debugstr_a(str));

 if (!str)
   return -1;
   
 name = HeapAlloc(GetProcessHeap(), 0, strlen(str) + 1);
 if (!name)
   return -1;
 dst = name;
 while (*str && *str != '=')
  *dst++ = *str++;
 if (!*str++)
 {
   ret = -1;
   goto finish;
 }
 *dst++ = '\0';
 value = dst;
 while (*str)
  *dst++ = *str++;
 *dst = '\0';

 ret = SetEnvironmentVariableA(name, value[0] ? value : NULL) ? 0 : -1;

 /* _putenv returns success on deletion of nonexistent variable, unlike [Rtl]SetEnvironmentVariable */
 if ((ret == -1) && (GetLastError() == ERROR_ENVVAR_NOT_FOUND)) ret = 0;

 /* Update the __p__environ array only when already initialized */
 if (MSVCRT__environ)
   MSVCRT__environ = msvcrt_SnapshotOfEnvironmentA(MSVCRT__environ);
 if (_wenviron)
   _wenviron = msvcrt_SnapshotOfEnvironmentW(_wenviron);
   
finish:
 HeapFree(GetProcessHeap(), 0, name);
 return ret;
}

/*********************************************************************
 *		_wputenv (MSVCRT.@)
 */
int CDECL _wputenv(const MSVCRT_wchar_t *str)
{
 MSVCRT_wchar_t *name, *value;
 MSVCRT_wchar_t *dst;
 int ret;

 TRACE("%s\n", debugstr_w(str));

 if (!str)
   return -1;
 name = HeapAlloc(GetProcessHeap(), 0, (strlenW(str) + 1) * sizeof(MSVCRT_wchar_t));
 if (!name)
   return -1;
 dst = name;
 while (*str && *str != '=')
  *dst++ = *str++;
 if (!*str++)
 {
   ret = -1;
   goto finish;
 }
 *dst++ = 0;
 value = dst;
 while (*str)
  *dst++ = *str++;
 *dst = 0;

 ret = SetEnvironmentVariableW(name, value[0] ? value : NULL) ? 0 : -1;

 /* _putenv returns success on deletion of nonexistent variable, unlike [Rtl]SetEnvironmentVariable */
 if ((ret == -1) && (GetLastError() == ERROR_ENVVAR_NOT_FOUND)) ret = 0;

 /* Update the __p__environ array only when already initialized */
 if (MSVCRT__environ)
   MSVCRT__environ = msvcrt_SnapshotOfEnvironmentA(MSVCRT__environ);
 if (_wenviron)
   _wenviron = msvcrt_SnapshotOfEnvironmentW(_wenviron);

finish:
 HeapFree(GetProcessHeap(), 0, name);
 return ret;
}
