/*
 * msvcrt.dll spawn/exec functions
 *
 * Copyright 1996,1998 Marcus Meissner
 * Copyright 1996 Jukka Iivonen
 * Copyright 1997,2000 Uwe Bonnes
 * Copyright 2000 Jon Griffiths
 * Copyright 2007 Hans Leidekker
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
 *
 * FIXME:
 * -File handles need some special handling. Sometimes children get
 *  open file handles, sometimes not. The docs are confusing
 * -No check for maximum path/argument/environment size is done
 */
#include "config.h"

#include <stdarg.h>

#include "msvcrt.h"
#include "mtdll.h"
#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

static void msvcrt_search_executable(const MSVCRT_wchar_t *name, MSVCRT_wchar_t *fullname, int use_path)
{
  static const MSVCRT_wchar_t path[] = {'P','A','T','H',0};
  static const MSVCRT_wchar_t suffix[][5] =
    {{'.','c','o','m',0}, {'.','e','x','e',0}, {'.','b','a','t',0}, {'.','c','m','d',0}};

  MSVCRT_wchar_t buffer[MAX_PATH];
  const MSVCRT_wchar_t *env, *p;
  unsigned int i, name_len, path_len;
  int extension = 1;

  *fullname = '\0';
  msvcrt_set_errno(ERROR_FILE_NOT_FOUND);

  p = memchrW(name, '\0', MAX_PATH);
  if (!p) p = name + MAX_PATH - 1;
  name_len = p - name;

  /* FIXME extra-long names are silently truncated */
  memcpy(buffer, name, name_len * sizeof(MSVCRT_wchar_t));
  buffer[name_len] = '\0';

  /* try current dir first */
  if (GetFileAttributesW(buffer) != INVALID_FILE_ATTRIBUTES)
  {
    strcpyW(fullname, buffer);
    return;
  }

  for (p--; p >= name; p--)
    if (*p == '\\' || *p == '/' || *p == ':' || *p == '.') break;

  /* if there's no extension, try some well-known extensions */
  if ((p < name || *p != '.') && name_len <= MAX_PATH - 5)
  {
    for (i = 0; i < 4; i++)
    {
      memcpy(buffer + name_len, suffix[i], 5 * sizeof(MSVCRT_wchar_t));
      if (GetFileAttributesW(buffer) != INVALID_FILE_ATTRIBUTES)
      {
        strcpyW(fullname, buffer);
        return;
      }
    }
    extension = 0;
  }

  if (!use_path || !(env = MSVCRT__wgetenv(path))) return;

  /* now try search path */
  do
  {
    p = env;
    while (*p && *p != ';') p++;
    if (p == env) return;

    path_len = p - env;
    if (path_len + name_len <= MAX_PATH - 2)
    {
      memcpy(buffer, env, path_len * sizeof(MSVCRT_wchar_t));
      if (buffer[path_len] != '/' && buffer[path_len] != '\\')
      {
        buffer[path_len++] = '\\';
        buffer[path_len] = '\0';
      }
      else buffer[path_len] = '\0';

      strcatW(buffer, name);
      if (GetFileAttributesW(buffer) != INVALID_FILE_ATTRIBUTES)
      {
        strcpyW(fullname, buffer);
        return;
      }
    }
    /* again, if there's no extension, try some well-known extensions */
    if (!extension && path_len + name_len <= MAX_PATH - 5)
    {
      for (i = 0; i < 4; i++)
      {
        memcpy(buffer + path_len + name_len, suffix[i], 5 * sizeof(MSVCRT_wchar_t));
        if (GetFileAttributesW(buffer) != INVALID_FILE_ATTRIBUTES)
        {
          strcpyW(fullname, buffer);
          return;
        }
      }
    }
    env = *p ? p + 1 : p;
  } while(1);
}

static MSVCRT_intptr_t msvcrt_spawn(int flags, const MSVCRT_wchar_t* exe, MSVCRT_wchar_t* cmdline,
                                    MSVCRT_wchar_t* env, int use_path)
{
  STARTUPINFOW si;
  PROCESS_INFORMATION pi;
  MSVCRT_wchar_t fullname[MAX_PATH];

  TRACE("%x %s %s %s %d\n", flags, debugstr_w(exe), debugstr_w(cmdline), debugstr_w(env), use_path);

  if ((unsigned)flags > MSVCRT__P_DETACH)
  {
    *MSVCRT__errno() = MSVCRT_EINVAL;
    return -1;
  }

  msvcrt_search_executable(exe, fullname, use_path);

  memset(&si, 0, sizeof(si));
  si.cb = sizeof(si);
  msvcrt_create_io_inherit_block(&si.cbReserved2, &si.lpReserved2);
  if (!CreateProcessW(fullname, cmdline, NULL, NULL, TRUE,
                     flags == MSVCRT__P_DETACH ? DETACHED_PROCESS : 0,
                     env, NULL, &si, &pi))
  {
    msvcrt_set_errno(GetLastError());
    MSVCRT_free(si.lpReserved2);
    return -1;
  }

  MSVCRT_free(si.lpReserved2);
  switch(flags)
  {
  case MSVCRT__P_WAIT:
    WaitForSingleObject(pi.hProcess, INFINITE);
    GetExitCodeProcess(pi.hProcess,&pi.dwProcessId);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return pi.dwProcessId;
  case MSVCRT__P_DETACH:
    CloseHandle(pi.hProcess);
    pi.hProcess = 0;
    /* fall through */
  case MSVCRT__P_NOWAIT:
  case MSVCRT__P_NOWAITO:
    CloseHandle(pi.hThread);
    return (MSVCRT_intptr_t)pi.hProcess;
  case  MSVCRT__P_OVERLAY:
    MSVCRT__exit(0);
  }
  return -1; /* can't reach here */
}

/* INTERNAL: Convert wide argv list to a single 'delim'-separated wide string, with an
 * extra '\0' to terminate it.
 */
static MSVCRT_wchar_t* msvcrt_argvtos(const MSVCRT_wchar_t* const* arg, MSVCRT_wchar_t delim)
{
  const MSVCRT_wchar_t* const* a;
  int size;
  MSVCRT_wchar_t* p;
  MSVCRT_wchar_t* ret;

  if (!arg)
  {
      /* Return NULL for an empty environment list */
      return NULL;
  }

  /* get length */
  a = arg;
  size = 0;
  while (*a)
  {
    size += strlenW(*a) + 1;
    a++;
  }

  ret = MSVCRT_malloc((size + 1) * sizeof(MSVCRT_wchar_t));
  if (!ret)
    return NULL;

  /* fill string */
  a = arg;
  p = ret;
  while (*a)
  {
    int len = strlenW(*a);
    memcpy(p,*a,len * sizeof(MSVCRT_wchar_t));
    p += len;
    *p++ = delim;
    a++;
  }
  if (delim && p > ret) p[-1] = 0;
  else *p = 0;
  return ret;
}

/* INTERNAL: Convert ansi argv list to a single 'delim'-separated wide string, with an
 * extra '\0' to terminate it.
 */
static MSVCRT_wchar_t *msvcrt_argvtos_aw(const char * const *arg, MSVCRT_wchar_t delim)
{
  const char * const *a;
  unsigned int len;
  MSVCRT_wchar_t *p, *ret;

  if (!arg)
  {
      /* Return NULL for an empty environment list */
      return NULL;
  }

  /* get length */
  a = arg;
  len = 0;
  while (*a)
  {
    len += MultiByteToWideChar(CP_ACP, 0, *a, -1, NULL, 0);
    a++;
  }

  ret = MSVCRT_malloc((len + 1) * sizeof(MSVCRT_wchar_t));
  if (!ret)
    return NULL;

  /* fill string */
  a = arg;
  p = ret;
  while (*a)
  {
    p += MultiByteToWideChar(CP_ACP, 0, *a, strlen(*a), p, len - (p - ret));
    *p++ = delim;
    a++;
  }
  if (delim && p > ret) p[-1] = 0;
  else *p = 0;
  return ret;
}

/* INTERNAL: Convert wide va_list to a single 'delim'-separated wide string, with an
 * extra '\0' to terminate it.
 */
static MSVCRT_wchar_t *msvcrt_valisttos(const MSVCRT_wchar_t *arg0, __ms_va_list alist, MSVCRT_wchar_t delim)
{
    unsigned int size = 0, pos = 0;
    const MSVCRT_wchar_t *arg;
    MSVCRT_wchar_t *new, *ret = NULL;

    for (arg = arg0; arg; arg = va_arg( alist, MSVCRT_wchar_t * ))
    {
        unsigned int len = strlenW( arg ) + 1;
        if (pos + len >= size)
        {
            size = max( 256, size * 2 );
            size = max( size, pos + len + 1 );
            if (!(new = MSVCRT_realloc( ret, size * sizeof(MSVCRT_wchar_t) )))
            {
                MSVCRT_free( ret );
                return NULL;
            }
            ret = new;
        }
        strcpyW( ret + pos, arg );
        pos += len;
        ret[pos - 1] = delim;
    }
    if (pos)
    {
        if (delim) ret[pos - 1] = 0;
        else ret[pos] = 0;
    }
    return ret;
}

/* INTERNAL: Convert ansi va_list to a single 'delim'-separated wide string, with an
 * extra '\0' to terminate it.
 */
static MSVCRT_wchar_t *msvcrt_valisttos_aw(const char *arg0, __ms_va_list alist, MSVCRT_wchar_t delim)
{
    unsigned int size = 0, pos = 0;
    const char *arg;
    MSVCRT_wchar_t *new, *ret = NULL;

    for (arg = arg0; arg; arg = va_arg( alist, char * ))
    {
        unsigned int len = MultiByteToWideChar( CP_ACP, 0, arg, -1, NULL, 0 );
        if (pos + len >= size)
        {
            size = max( 256, size * 2 );
            size = max( size, pos + len + 1 );
            if (!(new = MSVCRT_realloc( ret, size * sizeof(MSVCRT_wchar_t) )))
            {
                MSVCRT_free( ret );
                return NULL;
            }
            ret = new;
        }
        pos += MultiByteToWideChar( CP_ACP, 0, arg, -1, ret + pos, size - pos );
        ret[pos - 1] = delim;
    }
    if (pos)
    {
        if (delim) ret[pos - 1] = 0;
        else ret[pos] = 0;
    }
    return ret;
}

/* INTERNAL: retrieve COMSPEC environment variable */
static MSVCRT_wchar_t *msvcrt_get_comspec(void)
{
  static const MSVCRT_wchar_t cmd[] = {'c','m','d',0};
  static const MSVCRT_wchar_t comspec[] = {'C','O','M','S','P','E','C',0};
  MSVCRT_wchar_t *ret;
  unsigned int len;

  if (!(len = GetEnvironmentVariableW(comspec, NULL, 0))) len = sizeof(cmd)/sizeof(MSVCRT_wchar_t);
  if ((ret = HeapAlloc(GetProcessHeap(), 0, len * sizeof(MSVCRT_wchar_t))))
  {
    if (!GetEnvironmentVariableW(comspec, ret, len)) strcpyW(ret, cmd);
  }
  return ret;
}

/*********************************************************************
 *		_cwait (MSVCRT.@)
 */
MSVCRT_intptr_t CDECL _cwait(int *status, MSVCRT_intptr_t pid, int action)
{
  HANDLE hPid = (HANDLE)pid;
  int doserrno;

  if (!WaitForSingleObject(hPid, INFINITE))
  {
    if (status)
    {
      DWORD stat;
      GetExitCodeProcess(hPid, &stat);
      *status = (int)stat;
    }
    return pid;
  }
  doserrno = GetLastError();

  if (doserrno == ERROR_INVALID_HANDLE)
  {
    *MSVCRT__errno() =  MSVCRT_ECHILD;
    *MSVCRT___doserrno() = doserrno;
  }
  else
    msvcrt_set_errno(doserrno);

  return status ? *status = -1 : -1;
}

/*********************************************************************
 *      _wexecl (MSVCRT.@)
 *
 * Unicode version of _execl
 */
MSVCRT_intptr_t CDECL _wexecl(const MSVCRT_wchar_t* name, const MSVCRT_wchar_t* arg0, ...)
{
  __ms_va_list ap;
  MSVCRT_wchar_t *args;
  MSVCRT_intptr_t ret;

  __ms_va_start(ap, arg0);
  args = msvcrt_valisttos(arg0, ap, ' ');
  __ms_va_end(ap);

  ret = msvcrt_spawn(MSVCRT__P_OVERLAY, name, args, NULL, 0);

  MSVCRT_free(args);
  return ret;
}

/*********************************************************************
 *		_execl (MSVCRT.@)
 *
 * Like on Windows, this function does not handle arguments with spaces
 * or double-quotes.
 */
MSVCRT_intptr_t CDECL _execl(const char* name, const char* arg0, ...)
{
  __ms_va_list ap;
  MSVCRT_wchar_t *nameW, *args;
  MSVCRT_intptr_t ret;

  if (!(nameW = msvcrt_wstrdupa(name))) return -1;

  __ms_va_start(ap, arg0);
  args = msvcrt_valisttos_aw(arg0, ap, ' ');
  __ms_va_end(ap);

  ret = msvcrt_spawn(MSVCRT__P_OVERLAY, nameW, args, NULL, 0);

  MSVCRT_free(nameW);
  MSVCRT_free(args);
  return ret;
}

/*********************************************************************
 *      _wexecle (MSVCRT.@)
 *
 * Unicode version of _execle
 */
MSVCRT_intptr_t CDECL _wexecle(const MSVCRT_wchar_t* name, const MSVCRT_wchar_t* arg0, ...)
{
  __ms_va_list ap;
  MSVCRT_wchar_t *args, *envs = NULL;
  const MSVCRT_wchar_t * const *envp;
  MSVCRT_intptr_t ret;

  __ms_va_start(ap, arg0);
  args = msvcrt_valisttos(arg0, ap, ' ');
  __ms_va_end(ap);

  __ms_va_start(ap, arg0);
  while (va_arg( ap, MSVCRT_wchar_t * ) != NULL) /*nothing*/;
  envp = va_arg( ap, const MSVCRT_wchar_t * const * );
  if (envp) envs = msvcrt_argvtos(envp, 0);
  __ms_va_end(ap);

  ret = msvcrt_spawn(MSVCRT__P_OVERLAY, name, args, envs, 0);

  MSVCRT_free(args);
  MSVCRT_free(envs);
  return ret;
}

/*********************************************************************
 *		_execle (MSVCRT.@)
 */
MSVCRT_intptr_t CDECL _execle(const char* name, const char* arg0, ...)
{
  __ms_va_list ap;
  MSVCRT_wchar_t *nameW, *args, *envs = NULL;
  const char * const *envp;
  MSVCRT_intptr_t ret;

  if (!(nameW = msvcrt_wstrdupa(name))) return -1;

  __ms_va_start(ap, arg0);
  args = msvcrt_valisttos_aw(arg0, ap, ' ');
  __ms_va_end(ap);

  __ms_va_start(ap, arg0);
  while (va_arg( ap, char * ) != NULL) /*nothing*/;
  envp = va_arg( ap, const char * const * );
  if (envp) envs = msvcrt_argvtos_aw(envp, 0);
  __ms_va_end(ap);

  ret = msvcrt_spawn(MSVCRT__P_OVERLAY, nameW, args, envs, 0);

  MSVCRT_free(nameW);
  MSVCRT_free(args);
  MSVCRT_free(envs);
  return ret;
}

/*********************************************************************
 *      _wexeclp (MSVCRT.@)
 *
 * Unicode version of _execlp
 */
MSVCRT_intptr_t CDECL _wexeclp(const MSVCRT_wchar_t* name, const MSVCRT_wchar_t* arg0, ...)
{
  __ms_va_list ap;
  MSVCRT_wchar_t *args;
  MSVCRT_intptr_t ret;

  __ms_va_start(ap, arg0);
  args = msvcrt_valisttos(arg0, ap, ' ');
  __ms_va_end(ap);

  ret = msvcrt_spawn(MSVCRT__P_OVERLAY, name, args, NULL, 1);

  MSVCRT_free(args);
  return ret;
}

/*********************************************************************
 *		_execlp (MSVCRT.@)
 *
 * Like on Windows, this function does not handle arguments with spaces
 * or double-quotes.
 */
MSVCRT_intptr_t CDECL _execlp(const char* name, const char* arg0, ...)
{
  __ms_va_list ap;
  MSVCRT_wchar_t *nameW, *args;
  MSVCRT_intptr_t ret;

  if (!(nameW = msvcrt_wstrdupa(name))) return -1;

  __ms_va_start(ap, arg0);
  args = msvcrt_valisttos_aw(arg0, ap, ' ');
  __ms_va_end(ap);

  ret = msvcrt_spawn(MSVCRT__P_OVERLAY, nameW, args, NULL, 1);

  MSVCRT_free(nameW);
  MSVCRT_free(args);
  return ret;
}

/*********************************************************************
 *      _wexeclpe (MSVCRT.@)
 *
 * Unicode version of _execlpe
 */
MSVCRT_intptr_t CDECL _wexeclpe(const MSVCRT_wchar_t* name, const MSVCRT_wchar_t* arg0, ...)
{
  __ms_va_list ap;
  MSVCRT_wchar_t *args, *envs = NULL;
  const MSVCRT_wchar_t * const *envp;
  MSVCRT_intptr_t ret;

  __ms_va_start(ap, arg0);
  args = msvcrt_valisttos(arg0, ap, ' ');
  __ms_va_end(ap);

  __ms_va_start(ap, arg0);
  while (va_arg( ap, MSVCRT_wchar_t * ) != NULL) /*nothing*/;
  envp = va_arg( ap, const MSVCRT_wchar_t * const * );
  if (envp) envs = msvcrt_argvtos(envp, 0);
  __ms_va_end(ap);

  ret = msvcrt_spawn(MSVCRT__P_OVERLAY, name, args, envs, 1);

  MSVCRT_free(args);
  MSVCRT_free(envs);
  return ret;
}

/*********************************************************************
 *		_execlpe (MSVCRT.@)
 */
MSVCRT_intptr_t CDECL _execlpe(const char* name, const char* arg0, ...)
{
  __ms_va_list ap;
  MSVCRT_wchar_t *nameW, *args, *envs = NULL;
  const char * const *envp;
  MSVCRT_intptr_t ret;

  if (!(nameW = msvcrt_wstrdupa(name))) return -1;

  __ms_va_start(ap, arg0);
  args = msvcrt_valisttos_aw(arg0, ap, ' ');
  __ms_va_end(ap);

  __ms_va_start(ap, arg0);
  while (va_arg( ap, char * ) != NULL) /*nothing*/;
  envp = va_arg( ap, const char * const * );
  if (envp) envs = msvcrt_argvtos_aw(envp, 0);
  __ms_va_end(ap);

  ret = msvcrt_spawn(MSVCRT__P_OVERLAY, nameW, args, envs, 1);

  MSVCRT_free(nameW);
  MSVCRT_free(args);
  MSVCRT_free(envs);
  return ret;
}

/*********************************************************************
 *      _wexecv (MSVCRT.@)
 *
 * Unicode version of _execv
 */
MSVCRT_intptr_t CDECL _wexecv(const MSVCRT_wchar_t* name, const MSVCRT_wchar_t* const* argv)
{
  return MSVCRT__wspawnve(MSVCRT__P_OVERLAY, name, argv, NULL);
}

/*********************************************************************
 *		_execv (MSVCRT.@)
 *
 * Like on Windows, this function does not handle arguments with spaces
 * or double-quotes.
 */
MSVCRT_intptr_t CDECL _execv(const char* name, const char* const* argv)
{
  return MSVCRT__spawnve(MSVCRT__P_OVERLAY, name, argv, NULL);
}

/*********************************************************************
 *      _wexecve (MSVCRT.@)
 *
 * Unicode version of _execve
 */
MSVCRT_intptr_t CDECL _wexecve(const MSVCRT_wchar_t* name, const MSVCRT_wchar_t* const* argv, const MSVCRT_wchar_t* const* envv)
{
  return MSVCRT__wspawnve(MSVCRT__P_OVERLAY, name, argv, envv);
}

/*********************************************************************
 *		_execve (MSVCRT.@)
 *
 * Like on Windows, this function does not handle arguments with spaces
 * or double-quotes.
 */
MSVCRT_intptr_t CDECL MSVCRT__execve(const char* name, const char* const* argv, const char* const* envv)
{
  return MSVCRT__spawnve(MSVCRT__P_OVERLAY, name, argv, envv);
}

/*********************************************************************
 *      _wexecvpe (MSVCRT.@)
 *
 * Unicode version of _execvpe
 */
MSVCRT_intptr_t CDECL _wexecvpe(const MSVCRT_wchar_t* name, const MSVCRT_wchar_t* const* argv, const MSVCRT_wchar_t* const* envv)
{
  return MSVCRT__wspawnvpe(MSVCRT__P_OVERLAY, name, argv, envv);
}

/*********************************************************************
 *		_execvpe (MSVCRT.@)
 *
 * Like on Windows, this function does not handle arguments with spaces
 * or double-quotes.
 */
MSVCRT_intptr_t CDECL _execvpe(const char* name, const char* const* argv, const char* const* envv)
{
  return MSVCRT__spawnvpe(MSVCRT__P_OVERLAY, name, argv, envv);
}

/*********************************************************************
 *      _wexecvp (MSVCRT.@)
 *
 * Unicode version of _execvp
 */
MSVCRT_intptr_t CDECL _wexecvp(const MSVCRT_wchar_t* name, const MSVCRT_wchar_t* const* argv)
{
  return _wexecvpe(name, argv, NULL);
}

/*********************************************************************
 *		_execvp (MSVCRT.@)
 *
 * Like on Windows, this function does not handle arguments with spaces
 * or double-quotes.
 */
MSVCRT_intptr_t CDECL _execvp(const char* name, const char* const* argv)
{
  return _execvpe(name, argv, NULL);
}

/*********************************************************************
 *      _wspawnl (MSVCRT.@)
 *
 * Unicode version of _spawnl
 */
MSVCRT_intptr_t CDECL _wspawnl(int flags, const MSVCRT_wchar_t* name, const MSVCRT_wchar_t* arg0, ...)
{
  __ms_va_list ap;
  MSVCRT_wchar_t *args;
  MSVCRT_intptr_t ret;

  __ms_va_start(ap, arg0);
  args = msvcrt_valisttos(arg0, ap, ' ');
  __ms_va_end(ap);

  ret = msvcrt_spawn(flags, name, args, NULL, 0);

  MSVCRT_free(args);
  return ret;
}

/*********************************************************************
 *		_spawnl (MSVCRT.@)
 *
 * Like on Windows, this function does not handle arguments with spaces
 * or double-quotes.
 */
MSVCRT_intptr_t CDECL _spawnl(int flags, const char* name, const char* arg0, ...)
{
  __ms_va_list ap;
  MSVCRT_wchar_t *nameW, *args;
  MSVCRT_intptr_t ret;

  if (!(nameW = msvcrt_wstrdupa(name))) return -1;

  __ms_va_start(ap, arg0);
  args = msvcrt_valisttos_aw(arg0, ap, ' ');
  __ms_va_end(ap);

  ret = msvcrt_spawn(flags, nameW, args, NULL, 0);

  MSVCRT_free(nameW);
  MSVCRT_free(args);
  return ret;
}

/*********************************************************************
 *      _wspawnle (MSVCRT.@)
 *
 * Unicode version of _spawnle
 */
MSVCRT_intptr_t CDECL _wspawnle(int flags, const MSVCRT_wchar_t* name, const MSVCRT_wchar_t* arg0, ...)
{
  __ms_va_list ap;
  MSVCRT_wchar_t *args, *envs = NULL;
  const MSVCRT_wchar_t * const *envp;
  MSVCRT_intptr_t ret;

  __ms_va_start(ap, arg0);
  args = msvcrt_valisttos(arg0, ap, ' ');
  __ms_va_end(ap);

  __ms_va_start(ap, arg0);
  while (va_arg( ap, MSVCRT_wchar_t * ) != NULL) /*nothing*/;
  envp = va_arg( ap, const MSVCRT_wchar_t * const * );
  if (envp) envs = msvcrt_argvtos(envp, 0);
  __ms_va_end(ap);

  ret = msvcrt_spawn(flags, name, args, envs, 0);

  MSVCRT_free(args);
  MSVCRT_free(envs);
  return ret;
}

/*********************************************************************
 *		_spawnle (MSVCRT.@)
 */
MSVCRT_intptr_t CDECL _spawnle(int flags, const char* name, const char* arg0, ...)
{
  __ms_va_list ap;
  MSVCRT_wchar_t *nameW, *args, *envs = NULL;
  const char * const *envp;
  MSVCRT_intptr_t ret;

  if (!(nameW = msvcrt_wstrdupa(name))) return -1;

  __ms_va_start(ap, arg0);
  args = msvcrt_valisttos_aw(arg0, ap, ' ');
  __ms_va_end(ap);

  __ms_va_start(ap, arg0);
  while (va_arg( ap, char * ) != NULL) /*nothing*/;
  envp = va_arg( ap, const char * const * );
  if (envp) envs = msvcrt_argvtos_aw(envp, 0);
  __ms_va_end(ap);

  ret = msvcrt_spawn(flags, nameW, args, envs, 0);

  MSVCRT_free(nameW);
  MSVCRT_free(args);
  MSVCRT_free(envs);
  return ret;
}

/*********************************************************************
 *      _wspawnlp (MSVCRT.@)
 *
 * Unicode version of _spawnlp
 */
MSVCRT_intptr_t CDECL _wspawnlp(int flags, const MSVCRT_wchar_t* name, const MSVCRT_wchar_t* arg0, ...)
{
  __ms_va_list ap;
  MSVCRT_wchar_t *args;
  MSVCRT_intptr_t ret;

  __ms_va_start(ap, arg0);
  args = msvcrt_valisttos(arg0, ap, ' ');
  __ms_va_end(ap);

  ret = msvcrt_spawn(flags, name, args, NULL, 1);

  MSVCRT_free(args);
  return ret;
}

/*********************************************************************
 *		_spawnlp (MSVCRT.@)
 *
 * Like on Windows, this function does not handle arguments with spaces
 * or double-quotes.
 */
MSVCRT_intptr_t CDECL _spawnlp(int flags, const char* name, const char* arg0, ...)
{
  __ms_va_list ap;
  MSVCRT_wchar_t *nameW, *args;
  MSVCRT_intptr_t ret;

  if (!(nameW = msvcrt_wstrdupa(name))) return -1;

  __ms_va_start(ap, arg0);
  args = msvcrt_valisttos_aw(arg0, ap, ' ');
  __ms_va_end(ap);

  ret = msvcrt_spawn(flags, nameW, args, NULL, 1);

  MSVCRT_free(nameW);
  MSVCRT_free(args);
  return ret;
}

/*********************************************************************
 *      _wspawnlpe (MSVCRT.@)
 *
 * Unicode version of _spawnlpe
 */
MSVCRT_intptr_t CDECL _wspawnlpe(int flags, const MSVCRT_wchar_t* name, const MSVCRT_wchar_t* arg0, ...)
{
  __ms_va_list ap;
  MSVCRT_wchar_t *args, *envs = NULL;
  const MSVCRT_wchar_t * const *envp;
  MSVCRT_intptr_t ret;

  __ms_va_start(ap, arg0);
  args = msvcrt_valisttos(arg0, ap, ' ');
  __ms_va_end(ap);

  __ms_va_start(ap, arg0);
  while (va_arg( ap, MSVCRT_wchar_t * ) != NULL) /*nothing*/;
  envp = va_arg( ap, const MSVCRT_wchar_t * const * );
  if (envp) envs = msvcrt_argvtos(envp, 0);
  __ms_va_end(ap);

  ret = msvcrt_spawn(flags, name, args, envs, 1);

  MSVCRT_free(args);
  MSVCRT_free(envs);
  return ret;
}

/*********************************************************************
 *		_spawnlpe (MSVCRT.@)
 */
MSVCRT_intptr_t CDECL _spawnlpe(int flags, const char* name, const char* arg0, ...)
{
  __ms_va_list ap;
  MSVCRT_wchar_t *nameW, *args, *envs = NULL;
  const char * const *envp;
  MSVCRT_intptr_t ret;

  if (!(nameW = msvcrt_wstrdupa(name))) return -1;

  __ms_va_start(ap, arg0);
  args = msvcrt_valisttos_aw(arg0, ap, ' ');
  __ms_va_end(ap);

  __ms_va_start(ap, arg0);
  while (va_arg( ap, char * ) != NULL) /*nothing*/;
  envp = va_arg( ap, const char * const * );
  if (envp) envs = msvcrt_argvtos_aw(envp, 0);
  __ms_va_end(ap);

  ret = msvcrt_spawn(flags, nameW, args, envs, 1);

  MSVCRT_free(nameW);
  MSVCRT_free(args);
  MSVCRT_free(envs);
  return ret;
}

/*********************************************************************
 *		_spawnve (MSVCRT.@)
 *
 * Like on Windows, this function does not handle arguments with spaces
 * or double-quotes.
 */
MSVCRT_intptr_t CDECL MSVCRT__spawnve(int flags, const char* name, const char* const* argv,
                               const char* const* envv)
{
  MSVCRT_wchar_t *nameW, *args, *envs;
  MSVCRT_intptr_t ret;

  if (!(nameW = msvcrt_wstrdupa(name))) return -1;

  args = msvcrt_argvtos_aw(argv, ' ');
  envs = msvcrt_argvtos_aw(envv, 0);

  ret = msvcrt_spawn(flags, nameW, args, envs, 0);

  MSVCRT_free(nameW);
  MSVCRT_free(args);
  MSVCRT_free(envs);
  return ret;
}

/*********************************************************************
 *      _wspawnve (MSVCRT.@)
 *
 * Unicode version of _spawnve
 */
MSVCRT_intptr_t CDECL MSVCRT__wspawnve(int flags, const MSVCRT_wchar_t* name, const MSVCRT_wchar_t* const* argv,
                                const MSVCRT_wchar_t* const* envv)
{
  MSVCRT_wchar_t *args, *envs;
  MSVCRT_intptr_t ret;

  args = msvcrt_argvtos(argv, ' ');
  envs = msvcrt_argvtos(envv, 0);

  ret = msvcrt_spawn(flags, name, args, envs, 0);

  MSVCRT_free(args);
  MSVCRT_free(envs);
  return ret;
}

/*********************************************************************
 *		_spawnv (MSVCRT.@)
 *
 * Like on Windows, this function does not handle arguments with spaces
 * or double-quotes.
 */
MSVCRT_intptr_t CDECL _spawnv(int flags, const char* name, const char* const* argv)
{
  return MSVCRT__spawnve(flags, name, argv, NULL);
}

/*********************************************************************
 *      _wspawnv (MSVCRT.@)
 *
 * Unicode version of _spawnv
 */
MSVCRT_intptr_t CDECL _wspawnv(int flags, const MSVCRT_wchar_t* name, const MSVCRT_wchar_t* const* argv)
{
  return MSVCRT__wspawnve(flags, name, argv, NULL);
}

/*********************************************************************
 *		_spawnvpe (MSVCRT.@)
 *
 * Like on Windows, this function does not handle arguments with spaces
 * or double-quotes.
 */
MSVCRT_intptr_t CDECL MSVCRT__spawnvpe(int flags, const char* name, const char* const* argv,
                                const char* const* envv)
{
  MSVCRT_wchar_t *nameW, *args, *envs;
  MSVCRT_intptr_t ret;

  if (!(nameW = msvcrt_wstrdupa(name))) return -1;

  args = msvcrt_argvtos_aw(argv, ' ');
  envs = msvcrt_argvtos_aw(envv, 0);

  ret = msvcrt_spawn(flags, nameW, args, envs, 1);

  MSVCRT_free(nameW);
  MSVCRT_free(args);
  MSVCRT_free(envs);
  return ret;
}

/*********************************************************************
 *      _wspawnvpe (MSVCRT.@)
 *
 * Unicode version of _spawnvpe
 */
MSVCRT_intptr_t CDECL MSVCRT__wspawnvpe(int flags, const MSVCRT_wchar_t* name, const MSVCRT_wchar_t* const* argv,
                                 const MSVCRT_wchar_t* const* envv)
{
  MSVCRT_wchar_t *args, *envs;
  MSVCRT_intptr_t ret;

  args = msvcrt_argvtos(argv, ' ');
  envs = msvcrt_argvtos(envv, 0);

  ret = msvcrt_spawn(flags, name, args, envs, 1);

  MSVCRT_free(args);
  MSVCRT_free(envs);
  return ret;
}

/*********************************************************************
 *		_spawnvp (MSVCRT.@)
 *
 * Like on Windows, this function does not handle arguments with spaces
 * or double-quotes.
 */
MSVCRT_intptr_t CDECL _spawnvp(int flags, const char* name, const char* const* argv)
{
  return MSVCRT__spawnvpe(flags, name, argv, NULL);
}

/*********************************************************************
 *      _wspawnvp (MSVCRT.@)
 *
 * Unicode version of _spawnvp
 */
MSVCRT_intptr_t CDECL _wspawnvp(int flags, const MSVCRT_wchar_t* name, const MSVCRT_wchar_t* const* argv)
{
  return MSVCRT__wspawnvpe(flags, name, argv, NULL);
}

static struct popen_handle {
    MSVCRT_FILE *f;
    HANDLE proc;
} *popen_handles;
static DWORD popen_handles_size;

void msvcrt_free_popen_data(void)
{
    MSVCRT_free(popen_handles);
}

/*********************************************************************
 *		_wpopen (MSVCRT.@)
 *
 * Unicode version of _popen
 */
MSVCRT_FILE* CDECL MSVCRT__wpopen(const MSVCRT_wchar_t* command, const MSVCRT_wchar_t* mode)
{
  MSVCRT_FILE *ret;
  BOOL readPipe = TRUE;
  int textmode, fds[2], fdToDup, fdToOpen, fdStdHandle = -1;
  const MSVCRT_wchar_t *p;
  MSVCRT_wchar_t *comspec, *fullcmd;
  unsigned int len;
  static const MSVCRT_wchar_t flag[] = {' ','/','c',' ',0};
  struct popen_handle *container;
  DWORD i;

  TRACE("(command=%s, mode=%s)\n", debugstr_w(command), debugstr_w(mode));

  if (!command || !mode)
    return NULL;

  textmode = *__p__fmode() & (MSVCRT__O_BINARY | MSVCRT__O_TEXT);
  for (p = mode; *p; p++)
  {
    switch (*p)
    {
      case 'W':
      case 'w':
        readPipe = FALSE;
        break;
      case 'B':
      case 'b':
        textmode |= MSVCRT__O_BINARY;
        textmode &= ~MSVCRT__O_TEXT;
        break;
      case 'T':
      case 't':
        textmode |= MSVCRT__O_TEXT;
        textmode &= ~MSVCRT__O_BINARY;
        break;
    }
  }
  if (MSVCRT__pipe(fds, 0, textmode) == -1)
    return NULL;

  fdToDup = readPipe ? 1 : 0;
  fdToOpen = readPipe ? 0 : 1;

  _mlock(_POPEN_LOCK);
  for(i=0; i<popen_handles_size; i++)
  {
    if (!popen_handles[i].f)
      break;
  }
  if (i==popen_handles_size)
  {
    i = (popen_handles_size ? popen_handles_size*2 : 8);
    container = MSVCRT_realloc(popen_handles, i*sizeof(*container));
    if (!container) goto error;

    popen_handles = container;
    container = popen_handles+popen_handles_size;
    memset(container, 0, (i-popen_handles_size)*sizeof(*container));
    popen_handles_size = i;
  }
  else container = popen_handles+i;

  if ((fdStdHandle = MSVCRT__dup(fdToDup)) == -1)
    goto error;
  if (MSVCRT__dup2(fds[fdToDup], fdToDup) != 0)
    goto error;

  MSVCRT__close(fds[fdToDup]);

  if (!(comspec = msvcrt_get_comspec())) goto error;
  len = strlenW(comspec) + strlenW(flag) + strlenW(command) + 1;

  if (!(fullcmd = HeapAlloc(GetProcessHeap(), 0, len * sizeof(MSVCRT_wchar_t))))
  {
    HeapFree(GetProcessHeap(), 0, comspec);
    goto error;
  }

  strcpyW(fullcmd, comspec);
  strcatW(fullcmd, flag);
  strcatW(fullcmd, command);

  if ((container->proc = (HANDLE)msvcrt_spawn(MSVCRT__P_NOWAIT, comspec, fullcmd, NULL, 1))
          == INVALID_HANDLE_VALUE)
  {
    MSVCRT__close(fds[fdToOpen]);
    ret = NULL;
  }
  else
  {
    ret = MSVCRT__wfdopen(fds[fdToOpen], mode);
    if (!ret)
      MSVCRT__close(fds[fdToOpen]);
    container->f = ret;
  }
  _munlock(_POPEN_LOCK);
  HeapFree(GetProcessHeap(), 0, comspec);
  HeapFree(GetProcessHeap(), 0, fullcmd);
  MSVCRT__dup2(fdStdHandle, fdToDup);
  MSVCRT__close(fdStdHandle);
  return ret;

error:
  _munlock(_POPEN_LOCK);
  if (fdStdHandle != -1) MSVCRT__close(fdStdHandle);
  MSVCRT__close(fds[0]);
  MSVCRT__close(fds[1]);
  return NULL;
}

/*********************************************************************
 *      _popen (MSVCRT.@)
 */
MSVCRT_FILE* CDECL MSVCRT__popen(const char* command, const char* mode)
{
  MSVCRT_FILE *ret;
  MSVCRT_wchar_t *cmdW, *modeW;

  TRACE("(command=%s, mode=%s)\n", debugstr_a(command), debugstr_a(mode));

  if (!command || !mode)
    return NULL;

  if (!(cmdW = msvcrt_wstrdupa(command))) return NULL;
  if (!(modeW = msvcrt_wstrdupa(mode)))
  {
    HeapFree(GetProcessHeap(), 0, cmdW);
    return NULL;
  }

  ret = MSVCRT__wpopen(cmdW, modeW);

  HeapFree(GetProcessHeap(), 0, cmdW);
  HeapFree(GetProcessHeap(), 0, modeW);
  return ret;
}

/*********************************************************************
 *		_pclose (MSVCRT.@)
 */
int CDECL MSVCRT__pclose(MSVCRT_FILE* file)
{
  HANDLE h;
  DWORD i;

  if (!MSVCRT_CHECK_PMT(file != NULL)) return -1;

  _mlock(_POPEN_LOCK);
  for(i=0; i<popen_handles_size; i++)
  {
    if (popen_handles[i].f == file)
      break;
  }
  if(i == popen_handles_size)
  {
    _munlock(_POPEN_LOCK);
    *MSVCRT__errno() = MSVCRT_EBADF;
    return -1;
  }

  h = popen_handles[i].proc;
  popen_handles[i].f = NULL;
  _munlock(_POPEN_LOCK);

  MSVCRT_fclose(file);
  if(WaitForSingleObject(h, INFINITE)==WAIT_FAILED || !GetExitCodeProcess(h, &i))
  {
    msvcrt_set_errno(GetLastError());
    CloseHandle(h);
    return -1;
  }

  CloseHandle(h);
  return i;
}

/*********************************************************************
 *      _wsystem (MSVCRT.@)
 *
 * Unicode version of system
 */
int CDECL _wsystem(const MSVCRT_wchar_t* cmd)
{
  int res;
  MSVCRT_wchar_t *comspec, *fullcmd;
  unsigned int len;
  static const MSVCRT_wchar_t flag[] = {' ','/','c',' ',0};

  comspec = msvcrt_get_comspec();

  if (cmd == NULL)
  {
    if (comspec == NULL)
    {
        *MSVCRT__errno() = MSVCRT_ENOENT;
        return 0;
    }
    HeapFree(GetProcessHeap(), 0, comspec);
    return 1;
  }

  if ( comspec == NULL)
    return -1;

  len = strlenW(comspec) + strlenW(flag) + strlenW(cmd) + 1;

  if (!(fullcmd = HeapAlloc(GetProcessHeap(), 0, len * sizeof(MSVCRT_wchar_t))))
  {
    HeapFree(GetProcessHeap(), 0, comspec);
    return -1;
  }
  strcpyW(fullcmd, comspec);
  strcatW(fullcmd, flag);
  strcatW(fullcmd, cmd);

  res = msvcrt_spawn(MSVCRT__P_WAIT, comspec, fullcmd, NULL, 1);

  HeapFree(GetProcessHeap(), 0, comspec);
  HeapFree(GetProcessHeap(), 0, fullcmd);
  return res;
}

/*********************************************************************
 *		system (MSVCRT.@)
 */
int CDECL MSVCRT_system(const char* cmd)
{
  int res = -1;
  MSVCRT_wchar_t *cmdW;

  if (cmd == NULL)
    return _wsystem(NULL);

  if ((cmdW = msvcrt_wstrdupa(cmd)))
  {
    res = _wsystem(cmdW);
    HeapFree(GetProcessHeap(), 0, cmdW);
  }
  return res;
}

/*********************************************************************
 *		_loaddll (MSVCRT.@)
 */
MSVCRT_intptr_t CDECL _loaddll(const char* dllname)
{
  return (MSVCRT_intptr_t)LoadLibraryA(dllname);
}

/*********************************************************************
 *		_unloaddll (MSVCRT.@)
 */
int CDECL _unloaddll(MSVCRT_intptr_t dll)
{
  if (FreeLibrary((HMODULE)dll))
    return 0;
  else
  {
    int err = GetLastError();
    msvcrt_set_errno(err);
    return err;
  }
}

/*********************************************************************
 *		_getdllprocaddr (MSVCRT.@)
 */
void * CDECL _getdllprocaddr(MSVCRT_intptr_t dll, const char *name, int ordinal)
{
    if (name)
    {
        if (ordinal != -1) return NULL;
        return GetProcAddress( (HMODULE)dll, name );
    }
    if (HIWORD(ordinal)) return NULL;
    return GetProcAddress( (HMODULE)dll, (LPCSTR)(ULONG_PTR)ordinal );
}

/*********************************************************************
 *              _getpid (MSVCRT.@)
 */
int CDECL _getpid(void)
{
    return GetCurrentProcessId();
}

/*********************************************************************
 *  __crtTerminateProcess (MSVCR110.@)
 */
int CDECL MSVCR110__crtTerminateProcess(UINT exit_code)
{
    return TerminateProcess(GetCurrentProcess(), exit_code);
}
