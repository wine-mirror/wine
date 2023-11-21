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

int env_init(BOOL unicode, BOOL modif)
{
    if (!unicode && (!MSVCRT___initenv || modif))
    {
        char *environ_strings = GetEnvironmentStringsA();
        int count = 1, len = 1, i = 0; /* keep space for the trailing NULLS */
        char **blk, *ptr;

        for (ptr = environ_strings; *ptr; ptr += strlen(ptr) + 1)
        {
            /* Don't count environment variables starting with '=' which are command shell specific */
            if (*ptr != '=') count++;
            len += strlen(ptr) + 1;
        }
        if (MSVCRT___initenv != MSVCRT__environ)
            blk = realloc(MSVCRT__environ, count * sizeof(*MSVCRT__environ) + len);
        else
            blk = malloc(count * sizeof(*MSVCRT__environ) + len);
        if (!blk)
        {
            FreeEnvironmentStringsA(environ_strings);
            return -1;
        }
        MSVCRT__environ = blk;

        memcpy(&MSVCRT__environ[count], environ_strings, len);
        for (ptr = (char *)&MSVCRT__environ[count]; *ptr; ptr += strlen(ptr) + 1)
        {
            /* Skip special environment strings set by the command shell */
            if (*ptr != '=') MSVCRT__environ[i++] = ptr;
        }
        MSVCRT__environ[i] = NULL;
        FreeEnvironmentStringsA(environ_strings);

        if (!MSVCRT___initenv)
            MSVCRT___initenv = MSVCRT__environ;
    }

    if (unicode && (!MSVCRT___winitenv || modif))
    {
        wchar_t *wenviron_strings = GetEnvironmentStringsW();
        int count = 1, len = 1, i = 0; /* keep space for the trailing NULLS */
        wchar_t **wblk, *wptr;

        for (wptr = wenviron_strings; *wptr; wptr += wcslen(wptr) + 1)
        {
            /* Don't count environment variables starting with '=' which are command shell specific */
            if (*wptr != '=') count++;
            len += wcslen(wptr) + 1;
        }
        if (MSVCRT___winitenv != MSVCRT__wenviron)
            wblk = realloc(MSVCRT__wenviron, count * sizeof(*MSVCRT__wenviron) + len * sizeof(wchar_t));
        else
            wblk = malloc(count * sizeof(*MSVCRT__wenviron) + len * sizeof(wchar_t));
        if (!wblk)
        {
            FreeEnvironmentStringsW(wenviron_strings);
            return -1;
        }
        MSVCRT__wenviron = wblk;

        memcpy(&MSVCRT__wenviron[count], wenviron_strings, len * sizeof(wchar_t));
        for (wptr = (wchar_t *)&MSVCRT__wenviron[count]; *wptr; wptr += wcslen(wptr) + 1)
        {
            /* Skip special environment strings set by the command shell */
            if (*wptr != '=') MSVCRT__wenviron[i++] = wptr;
        }
        MSVCRT__wenviron[i] = NULL;
        FreeEnvironmentStringsW(wenviron_strings);

        if (!MSVCRT___winitenv)
            MSVCRT___winitenv = MSVCRT__wenviron;
    }

    return 0;
}

static int env_get_index(const char *name)
{
    int i, len;

    len = strlen(name);
    for (i = 0; MSVCRT__environ[i]; i++)
    {
        if (!strncmp(name, MSVCRT__environ[i], len) && MSVCRT__environ[i][len] == '=')
            return i;
    }
    return i;
}

static int wenv_get_index(const wchar_t *name)
{
    int i, len;

    len = wcslen(name);
    for (i = 0; MSVCRT__wenviron[i]; i++)
    {
        if (!wcsncmp(name, MSVCRT__wenviron[i], len) && MSVCRT__wenviron[i][len] == '=')
            return i;
    }
    return i;
}

static char * getenv_helper(const char *name)
{
    int idx;

    if (!name) return NULL;

    idx = env_get_index(name);
    if (!MSVCRT__environ[idx]) return NULL;
    return strchr(MSVCRT__environ[idx], '=') + 1;
}

/*********************************************************************
 *              getenv (MSVCRT.@)
 */
char * CDECL getenv(const char *name)
{
    if (!MSVCRT_CHECK_PMT(name != NULL)) return NULL;

    return getenv_helper(name);
}

static wchar_t * wgetenv_helper(const wchar_t *name)
{
    int idx;

    if (!name) return NULL;
    if (env_init(TRUE, FALSE)) return NULL;

    idx = wenv_get_index(name);
    if (!MSVCRT__wenviron[idx]) return NULL;
    return wcschr(MSVCRT__wenviron[idx], '=') + 1;
}

/*********************************************************************
 *              _wgetenv (MSVCRT.@)
 */
wchar_t * CDECL _wgetenv(const wchar_t *name)
{
    if (!MSVCRT_CHECK_PMT(name != NULL)) return NULL;

    return wgetenv_helper(name);
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

 if (ret != -1) ret = env_init(FALSE, TRUE);
 /* Update the __p__wenviron array only when already initialized */
 if (ret != -1 && MSVCRT__wenviron) ret = env_init(TRUE, TRUE);
   
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

 if (env_init(TRUE, FALSE)) return -1;

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

 if (ret != -1) ret = env_init(FALSE, TRUE);
 if (ret != -1) ret = env_init(TRUE, TRUE);

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

    env_init(FALSE, TRUE);
    env_init(TRUE, TRUE);
    return ret;
}

/*********************************************************************
 *		_wputenv_s (MSVCRT.@)
 */
errno_t CDECL _wputenv_s(const wchar_t *name, const wchar_t *value)
{
    errno_t ret = 0;

    TRACE("%s %s\n", debugstr_w(name), debugstr_w(value));

    env_init(TRUE, FALSE);

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

    env_init(FALSE, TRUE);
    env_init(TRUE, TRUE);
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

    if (!(e = getenv(varname)))
    {
        *buffer = NULL;
        if (numberOfElements) *numberOfElements = 0;
        return 0;
    }

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

    if (!(e = _wgetenv(varname)))
    {
        *buffer = NULL;
        if (numberOfElements) *numberOfElements = 0;
        return 0;
    }

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
int CDECL getenv_s(size_t *ret_len, char* buffer, size_t len, const char *varname)
{
    char *e;

    if (!MSVCRT_CHECK_PMT(ret_len != NULL)) return EINVAL;
    *ret_len = 0;
    if (!MSVCRT_CHECK_PMT((buffer && len > 0) || (!buffer && !len))) return EINVAL;
    if (buffer) buffer[0] = 0;

    if (!(e = getenv_helper(varname))) return 0;
    *ret_len = strlen(e) + 1;
    if (!len) return 0;
    if (len < *ret_len) return ERANGE;

    strcpy(buffer, e);
    return 0;
}

/******************************************************************
 *		_wgetenv_s (MSVCRT.@)
 */
int CDECL _wgetenv_s(size_t *ret_len, wchar_t *buffer, size_t len,
                     const wchar_t *varname)
{
    wchar_t *e;

    if (!MSVCRT_CHECK_PMT(ret_len != NULL)) return EINVAL;
    *ret_len = 0;
    if (!MSVCRT_CHECK_PMT((buffer && len > 0) || (!buffer && !len))) return EINVAL;
    if (buffer) buffer[0] = 0;

    if (!(e = wgetenv_helper(varname))) return 0;
    *ret_len = wcslen(e) + 1;
    if (!len) return 0;
    if (len < *ret_len) return ERANGE;

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
