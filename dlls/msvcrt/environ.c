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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "wine/unicode.h"
#include "msvcrt.h"

#include "msvcrt/stdlib.h"


#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

/*********************************************************************
 *		getenv (MSVCRT.@)
 */
char *MSVCRT_getenv(const char *name)
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
MSVCRT_wchar_t *_wgetenv(const MSVCRT_wchar_t *name)
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
int _putenv(const char *str)
{
 char name[256], value[512];
 char *dst = name;
 int ret;

 TRACE("%s\n", str);

 if (!str)
   return -1;
 while (*str && *str != '=')
  *dst++ = *str++;
 if (!*str++)
   return -1;
 *dst = '\0';
 dst = value;
 while (*str)
  *dst++ = *str++;
 *dst = '\0';

 ret = !SetEnvironmentVariableA(name, value[0] ? value : NULL);
 /* Update the __p__environ array only when already initialized */
 if (MSVCRT__environ)
   MSVCRT__environ = msvcrt_SnapshotOfEnvironmentA(MSVCRT__environ);
 if (MSVCRT__wenviron)
   MSVCRT__wenviron = msvcrt_SnapshotOfEnvironmentW(MSVCRT__wenviron);
 return ret;
}

/*********************************************************************
 *		_wputenv (MSVCRT.@)
 */
int _wputenv(const MSVCRT_wchar_t *str)
{
 MSVCRT_wchar_t name[256], value[512];
 MSVCRT_wchar_t *dst = name;
 int ret;

 TRACE("%s\n", debugstr_w(str));

 if (!str)
   return -1;
 while (*str && *str != '=')
  *dst++ = *str++;
 if (!*str++)
   return -1;
 *dst = 0;
 dst = value;
 while (*str)
  *dst++ = *str++;
 *dst = 0;

 ret = !SetEnvironmentVariableW(name, value[0] ? value : NULL);
 /* Update the __p__environ array only when already initialized */
 if (MSVCRT__environ)
   MSVCRT__environ = msvcrt_SnapshotOfEnvironmentA(MSVCRT__environ);
 if (MSVCRT__wenviron)
   MSVCRT__wenviron = msvcrt_SnapshotOfEnvironmentW(MSVCRT__wenviron);
 return ret;
}
