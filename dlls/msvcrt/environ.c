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
#include "msvcrt.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

/*********************************************************************
 *		getenv (MSVCRT.@)
 */
char * CDECL getenv(const char *name)
{
    char **env;
    unsigned int length=strlen(name);

    for (env = MSVCRT__environ; *env; env++)
    {
        char *str = *env;
        char *pos = strchr(str,'=');
        if (pos && ((pos - str) == length) && !_strnicmp(str,name,length))
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
wchar_t * CDECL _wgetenv(const wchar_t *name)
{
    wchar_t **env;
    unsigned int length=wcslen(name);

    /* Initialize the _wenviron array if it's not already created. */
    if (!MSVCRT__wenviron)
        MSVCRT__wenviron = msvcrt_SnapshotOfEnvironmentW(NULL);

    for (env = MSVCRT__wenviron; *env; env++)
    {
        wchar_t *str = *env;
        wchar_t *pos = wcschr(str,'=');
        if (pos && ((pos - str) == length) && !_wcsnicmp(str,name,length))
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

 MSVCRT__environ = msvcrt_SnapshotOfEnvironmentA(MSVCRT__environ);
 /* Update the __p__wenviron array only when already initialized */
 if (MSVCRT__wenviron)
   MSVCRT__wenviron = msvcrt_SnapshotOfEnvironmentW(MSVCRT__wenviron);
   
finish:
 HeapFree(GetProcessHeap(), 0, name);
 return ret;
}

/*********************************************************************
 *		_wputenv (MSVCRT.@)
 */
int CDECL _wputenv(const wchar_t *str)
{
 wchar_t *name, *value;
 wchar_t *dst;
 int ret;

 TRACE("%s\n", debugstr_w(str));

 if (!str)
   return -1;
 name = HeapAlloc(GetProcessHeap(), 0, (wcslen(str) + 1) * sizeof(wchar_t));
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

 MSVCRT__environ = msvcrt_SnapshotOfEnvironmentA(MSVCRT__environ);
 MSVCRT__wenviron = msvcrt_SnapshotOfEnvironmentW(MSVCRT__wenviron);

finish:
 HeapFree(GetProcessHeap(), 0, name);
 return ret;
}

/*********************************************************************
 *		_putenv_s (MSVCRT.@)
 */
errno_t CDECL _putenv_s(const char *name, const char *value)
{
    errno_t ret = 0;

    TRACE("%s %s\n", debugstr_a(name), debugstr_a(value));

    if (!MSVCRT_CHECK_PMT(name != NULL)) return EINVAL;
    if (!MSVCRT_CHECK_PMT(value != NULL)) return EINVAL;

    if (!SetEnvironmentVariableA(name, value[0] ? value : NULL))
    {
        /* _putenv returns success on deletion of nonexistent variable */
        if (GetLastError() != ERROR_ENVVAR_NOT_FOUND)
        {
            msvcrt_set_errno(GetLastError());
            ret = *_errno();
        }
    }

    MSVCRT__environ = msvcrt_SnapshotOfEnvironmentA(MSVCRT__environ);
    MSVCRT__wenviron = msvcrt_SnapshotOfEnvironmentW(MSVCRT__wenviron);

    return ret;
}

/*********************************************************************
 *		_wputenv_s (MSVCRT.@)
 */
errno_t CDECL _wputenv_s(const wchar_t *name, const wchar_t *value)
{
    errno_t ret = 0;

    TRACE("%s %s\n", debugstr_w(name), debugstr_w(value));

    if (!MSVCRT_CHECK_PMT(name != NULL)) return EINVAL;
    if (!MSVCRT_CHECK_PMT(value != NULL)) return EINVAL;

    if (!SetEnvironmentVariableW(name, value[0] ? value : NULL))
    {
        /* _putenv returns success on deletion of nonexistent variable */
        if (GetLastError() != ERROR_ENVVAR_NOT_FOUND)
        {
            msvcrt_set_errno(GetLastError());
            ret = *_errno();
        }
    }

    MSVCRT__environ = msvcrt_SnapshotOfEnvironmentA(MSVCRT__environ);
    MSVCRT__wenviron = msvcrt_SnapshotOfEnvironmentW(MSVCRT__wenviron);

    return ret;
}

#if _MSVCR_VER>=80

/******************************************************************
 *		_dupenv_s (MSVCR80.@)
 */
int CDECL _dupenv_s(char **buffer, size_t *numberOfElements, const char *varname)
{
    char *e;
    size_t sz;

    if (!MSVCRT_CHECK_PMT(buffer != NULL)) return EINVAL;
    if (!MSVCRT_CHECK_PMT(varname != NULL)) return EINVAL;

    if (!(e = getenv(varname))) return *_errno() = EINVAL;

    sz = strlen(e) + 1;
    if (!(*buffer = malloc(sz)))
    {
        if (numberOfElements) *numberOfElements = 0;
        return *_errno() = ENOMEM;
    }
    strcpy(*buffer, e);
    if (numberOfElements) *numberOfElements = sz;
    return 0;
}

/******************************************************************
 *		_wdupenv_s (MSVCR80.@)
 */
int CDECL _wdupenv_s(wchar_t **buffer, size_t *numberOfElements,
                     const wchar_t *varname)
{
    wchar_t *e;
    size_t sz;

    if (!MSVCRT_CHECK_PMT(buffer != NULL)) return EINVAL;
    if (!MSVCRT_CHECK_PMT(varname != NULL)) return EINVAL;

    if (!(e = _wgetenv(varname))) return *_errno() = EINVAL;

    sz = wcslen(e) + 1;
    if (!(*buffer = malloc(sz * sizeof(wchar_t))))
    {
        if (numberOfElements) *numberOfElements = 0;
        return *_errno() = ENOMEM;
    }
    wcscpy(*buffer, e);
    if (numberOfElements) *numberOfElements = sz;
    return 0;
}

#endif /* _MSVCR_VER>=80 */

/******************************************************************
 *		getenv_s (MSVCRT.@)
 */
int CDECL getenv_s(size_t *pReturnValue, char* buffer, size_t numberOfElements, const char *varname)
{
    char *e;

    if (!MSVCRT_CHECK_PMT(pReturnValue != NULL)) return EINVAL;
    if (!MSVCRT_CHECK_PMT(!(buffer == NULL && numberOfElements > 0))) return EINVAL;
    if (!MSVCRT_CHECK_PMT(varname != NULL)) return EINVAL;

    if (!(e = getenv(varname)))
    {
        *pReturnValue = 0;
        return *_errno() = EINVAL;
    }
    *pReturnValue = strlen(e) + 1;
    if (numberOfElements < *pReturnValue)
    {
        return *_errno() = ERANGE;
    }
    strcpy(buffer, e);
    return 0;
}

/******************************************************************
 *		_wgetenv_s (MSVCRT.@)
 */
int CDECL _wgetenv_s(size_t *pReturnValue, wchar_t *buffer, size_t numberOfElements,
                     const wchar_t *varname)
{
    wchar_t *e;

    if (!MSVCRT_CHECK_PMT(pReturnValue != NULL)) return EINVAL;
    if (!MSVCRT_CHECK_PMT(!(buffer == NULL && numberOfElements > 0))) return EINVAL;
    if (!MSVCRT_CHECK_PMT(varname != NULL)) return EINVAL;

    if (!(e = _wgetenv(varname)))
    {
        *pReturnValue = 0;
        return *_errno() = EINVAL;
    }
    *pReturnValue = wcslen(e) + 1;
    if (numberOfElements < *pReturnValue)
    {
        return *_errno() = ERANGE;
    }
    wcscpy(buffer, e);
    return 0;
}

/*********************************************************************
 *		_get_environ (MSVCRT.@)
 */
void CDECL _get_environ(char ***ptr)
{
    *ptr = MSVCRT__environ;
}

/*********************************************************************
 *		_get_wenviron (MSVCRT.@)
 */
void CDECL _get_wenviron(wchar_t ***ptr)
{
    *ptr = MSVCRT__wenviron;
}
