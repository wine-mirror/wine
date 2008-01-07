/*
 * msvcrt.dll spawn/exec functions
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
 *
 * FIXME:
 * -File handles need some special handling. Sometimes children get
 *  open file handles, sometimes not. The docs are confusing
 * -No check for maximum path/argument/environment size is done
 */
#include "config.h"

#include <stdarg.h>

#include "msvcrt.h"
#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

static MSVCRT_intptr_t msvcrt_spawn(int flags, const MSVCRT_wchar_t* exe, MSVCRT_wchar_t* cmdline, MSVCRT_wchar_t* env)
{
  STARTUPINFOW si;
  PROCESS_INFORMATION pi;

  if ((unsigned)flags > MSVCRT__P_DETACH)
  {
    *MSVCRT__errno() = MSVCRT_EINVAL;
    return -1;
  }

  memset(&si, 0, sizeof(si));
  si.cb = sizeof(si);
  msvcrt_create_io_inherit_block(&si.cbReserved2, &si.lpReserved2);
  if (!CreateProcessW(exe, cmdline, NULL, NULL, TRUE,
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
  long size;
  MSVCRT_wchar_t* p;
  MSVCRT_wchar_t* ret;

  if (!arg && !delim)
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
  unsigned long len;
  MSVCRT_wchar_t *p, *ret;

  if (!arg && !delim)
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
static MSVCRT_wchar_t *msvcrt_valisttos(const MSVCRT_wchar_t *arg0, va_list alist, MSVCRT_wchar_t delim)
{
  va_list alist2;
  unsigned long len;
  const MSVCRT_wchar_t *arg;
  MSVCRT_wchar_t *p, *ret;

#ifdef HAVE_VA_COPY
  va_copy(alist2,alist);
#else
# ifdef HAVE___VA_COPY
  __va_copy(alist2,alist);
# else
  alist2 = alist;
# endif
#endif

  if (!arg0)
  {
      /* Return NULL for an empty environment list */
      return NULL;
  }

  /* get length */
  arg = arg0;
  len = 0;
  do {
      len += strlenW(arg) + 1;
      arg = va_arg(alist, MSVCRT_wchar_t*);
  } while (arg != NULL);

  ret = MSVCRT_malloc((len + 1) * sizeof(MSVCRT_wchar_t));
  if (!ret)
    return NULL;

  /* fill string */
  arg = arg0;
  p = ret;
  do {
      len = strlenW(arg);
      memcpy(p, arg, len * sizeof(MSVCRT_wchar_t));
      p += len;
      *p++ = delim;
      arg = va_arg(alist2, MSVCRT_wchar_t*);
  } while (arg != NULL);
  if (delim && p > ret) p[-1] = 0;
  else *p = 0;
  return ret;
}

/* INTERNAL: Convert ansi va_list to a single 'delim'-separated wide string, with an
 * extra '\0' to terminate it.
 */
static MSVCRT_wchar_t *msvcrt_valisttos_aw(const char *arg0, va_list alist, MSVCRT_wchar_t delim)
{
  va_list alist2;
  unsigned long len;
  const char *arg;
  MSVCRT_wchar_t *p, *ret;

#ifdef HAVE_VA_COPY
  va_copy(alist2,alist);
#else
# ifdef HAVE___VA_COPY
  __va_copy(alist2,alist);
# else
  alist2 = alist;
# endif
#endif

  if (!arg0)
  {
      /* Return NULL for an empty environment list */
      return NULL;
  }

  /* get length */
  arg = arg0;
  len = 0;
  do {
      len += MultiByteToWideChar(CP_ACP, 0, arg, -1, NULL, 0);
      arg = va_arg(alist, char*);
  } while (arg != NULL);

  ret = MSVCRT_malloc((len + 1) * sizeof(MSVCRT_wchar_t));
  if (!ret)
    return NULL;

  /* fill string */
  arg = arg0;
  p = ret;
  do {
      p += MultiByteToWideChar(CP_ACP, 0, arg, strlen(arg), p, len - (p - ret));
      *p++ = delim;
      arg = va_arg(alist2, char*);
  } while (arg != NULL);
  if (delim && p > ret) p[-1] = 0;
  else *p = 0;
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

  action = action; /* Remove warning */

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
  va_list ap;
  MSVCRT_wchar_t *args;
  MSVCRT_intptr_t ret;

  va_start(ap, arg0);
  args = msvcrt_valisttos(arg0, ap, ' ');
  va_end(ap);

  ret = msvcrt_spawn(MSVCRT__P_OVERLAY, name, args, NULL);

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
  va_list ap;
  MSVCRT_wchar_t *nameW, *args;
  MSVCRT_intptr_t ret = -1;

  if (!(nameW = msvcrt_wstrdupa(name))) return -1;

  va_start(ap, arg0);
  args = msvcrt_valisttos_aw(arg0, ap, ' ');
  va_end(ap);

  ret = msvcrt_spawn(MSVCRT__P_OVERLAY, nameW, args, NULL);

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
  va_list ap;
  MSVCRT_wchar_t *args, *envs = NULL;
  const MSVCRT_wchar_t * const *envp;
  MSVCRT_intptr_t ret;

  va_start(ap, arg0);
  args = msvcrt_valisttos(arg0, ap, ' ');
  va_end(ap);

  va_start(ap, arg0);
  while (va_arg( ap, MSVCRT_wchar_t * ) != NULL) /*nothing*/;
  envp = va_arg( ap, const MSVCRT_wchar_t * const * );
  if (envp) envs = msvcrt_argvtos(envp, 0);
  va_end(ap);

  ret = msvcrt_spawn(MSVCRT__P_OVERLAY, name, args, envs);

  MSVCRT_free(args);
  MSVCRT_free(envs);
  return ret;
}

/*********************************************************************
 *		_execle (MSVCRT.@)
 */
MSVCRT_intptr_t CDECL _execle(const char* name, const char* arg0, ...)
{
  va_list ap;
  MSVCRT_wchar_t *nameW, *args, *envs = NULL;
  const char * const *envp;
  MSVCRT_intptr_t ret;

  if (!(nameW = msvcrt_wstrdupa(name))) return -1;

  va_start(ap, arg0);
  args = msvcrt_valisttos_aw(arg0, ap, ' ');
  va_end(ap);

  va_start(ap, arg0);
  while (va_arg( ap, char * ) != NULL) /*nothing*/;
  envp = va_arg( ap, const char * const * );
  if (envp) envs = msvcrt_argvtos_aw(envp, 0);
  va_end(ap);

  ret = msvcrt_spawn(MSVCRT__P_OVERLAY, nameW, args, envs);

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
  static const MSVCRT_wchar_t path[] = {'P','A','T','H',0};
  va_list ap;
  MSVCRT_wchar_t *args, fullname[MAX_PATH];
  MSVCRT_intptr_t ret;

  _wsearchenv(name, path, fullname);

  va_start(ap, arg0);
  args = msvcrt_valisttos(arg0, ap, ' ');
  va_end(ap);

  ret = msvcrt_spawn(MSVCRT__P_OVERLAY, fullname[0] ? fullname : name, args, NULL);

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
  static const MSVCRT_wchar_t path[] = {'P','A','T','H',0};
  va_list ap;
  MSVCRT_wchar_t *nameW, *args, fullname[MAX_PATH];
  MSVCRT_intptr_t ret;

  if (!(nameW = msvcrt_wstrdupa(name))) return -1;
  _wsearchenv(nameW, path, fullname);

  va_start(ap, arg0);
  args = msvcrt_valisttos_aw(arg0, ap, ' ');
  va_end(ap);

  ret = msvcrt_spawn(MSVCRT__P_OVERLAY, fullname[0] ? fullname : nameW, args, NULL);

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
  static const MSVCRT_wchar_t path[] = {'P','A','T','H',0};
  va_list ap;
  MSVCRT_wchar_t *args, *envs = NULL, fullname[MAX_PATH];
  const MSVCRT_wchar_t * const *envp;
  MSVCRT_intptr_t ret;

  _wsearchenv(name, path, fullname);

  va_start(ap, arg0);
  args = msvcrt_valisttos(arg0, ap, ' ');
  va_end(ap);

  va_start(ap, arg0);
  while (va_arg( ap, MSVCRT_wchar_t * ) != NULL) /*nothing*/;
  envp = va_arg( ap, const MSVCRT_wchar_t * const * );
  if (envp) envs = msvcrt_argvtos(envp, 0);
  va_end(ap);

  ret = msvcrt_spawn(MSVCRT__P_OVERLAY, fullname[0] ? fullname : name, args, envs);

  MSVCRT_free(args);
  MSVCRT_free(envs);
  return ret;
}

/*********************************************************************
 *		_execlpe (MSVCRT.@)
 */
MSVCRT_intptr_t CDECL _execlpe(const char* name, const char* arg0, ...)
{
  static const MSVCRT_wchar_t path[] = {'P','A','T','H',0};
  va_list ap;
  MSVCRT_wchar_t *nameW, *args, *envs = NULL, fullname[MAX_PATH];
  const char * const *envp;
  MSVCRT_intptr_t ret;

  if (!(nameW = msvcrt_wstrdupa(name))) return -1;
  _wsearchenv(nameW, path, fullname);

  va_start(ap, arg0);
  args = msvcrt_valisttos_aw(arg0, ap, ' ');
  va_end(ap);

  va_start(ap, arg0);
  while (va_arg( ap, char * ) != NULL) /*nothing*/;
  envp = va_arg( ap, const char * const * );
  if (envp) envs = msvcrt_argvtos_aw(envp, 0);
  va_end(ap);

  ret = msvcrt_spawn(MSVCRT__P_OVERLAY, fullname[0] ? fullname : nameW, args, envs);

  MSVCRT_free(nameW);
  MSVCRT_free(args);
  MSVCRT_free(envs);
  return ret;
}

/*********************************************************************
 *		_execv (MSVCRT.@)
 *
 * Like on Windows, this function does not handle arguments with spaces
 * or double-quotes.
 */
MSVCRT_intptr_t CDECL _execv(const char* name, char* const* argv)
{
  return _spawnve(MSVCRT__P_OVERLAY, name, (const char* const*) argv, NULL);
}

/*********************************************************************
 *		_execve (MSVCRT.@)
 *
 * Like on Windows, this function does not handle arguments with spaces
 * or double-quotes.
 */
MSVCRT_intptr_t CDECL MSVCRT__execve(const char* name, char* const* argv, const char* const* envv)
{
  return _spawnve(MSVCRT__P_OVERLAY, name, (const char* const*) argv, envv);
}

/*********************************************************************
 *		_execvpe (MSVCRT.@)
 *
 * Like on Windows, this function does not handle arguments with spaces
 * or double-quotes.
 */
MSVCRT_intptr_t CDECL _execvpe(const char* name, char* const* argv, const char* const* envv)
{
  char fullname[MAX_PATH];

  _searchenv(name, "PATH", fullname);
  return _spawnve(MSVCRT__P_OVERLAY, fullname[0] ? fullname : name,
                  (const char* const*) argv, envv);
}

/*********************************************************************
 *		_execvp (MSVCRT.@)
 *
 * Like on Windows, this function does not handle arguments with spaces
 * or double-quotes.
 */
MSVCRT_intptr_t CDECL _execvp(const char* name, char* const* argv)
{
  return _execvpe(name, argv, NULL);
}

/*********************************************************************
 *		_spawnl (MSVCRT.@)
 *
 * Like on Windows, this function does not handle arguments with spaces
 * or double-quotes.
 */
MSVCRT_intptr_t CDECL _spawnl(int flags, const char* name, const char* arg0, ...)
{
  va_list ap;
  MSVCRT_wchar_t *nameW, *args;
  MSVCRT_intptr_t ret;

  if (!(nameW = msvcrt_wstrdupa(name))) return -1;

  va_start(ap, arg0);
  args = msvcrt_valisttos_aw(arg0, ap, ' ');
  va_end(ap);

  ret = msvcrt_spawn(flags, nameW, args, NULL);

  MSVCRT_free(nameW);
  MSVCRT_free(args);
  return ret;
}

/*********************************************************************
 *		_spawnle (MSVCRT.@)
 */
MSVCRT_intptr_t CDECL _spawnle(int flags, const char* name, const char* arg0, ...)
{
  va_list ap;
  MSVCRT_wchar_t *nameW, *args, *envs = NULL;
  const char * const *envp;
  MSVCRT_intptr_t ret;

  if (!(nameW = msvcrt_wstrdupa(name))) return -1;

  va_start(ap, arg0);
  args = msvcrt_valisttos_aw(arg0, ap, ' ');
  va_end(ap);

  va_start(ap, arg0);
  while (va_arg( ap, char * ) != NULL) /*nothing*/;
  envp = va_arg( ap, const char * const * );
  if (envp) envs = msvcrt_argvtos_aw(envp, 0);
  va_end(ap);

  ret = msvcrt_spawn(flags, nameW, args, envs);

  MSVCRT_free(nameW);
  MSVCRT_free(args);
  MSVCRT_free(envs);
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
  static const MSVCRT_wchar_t path[] = {'P','A','T','H',0};
  va_list ap;
  MSVCRT_wchar_t *nameW, *args, fullname[MAX_PATH];
  MSVCRT_intptr_t ret;

  if (!(nameW = msvcrt_wstrdupa(name))) return -1;
  _wsearchenv(nameW, path, fullname);

  va_start(ap, arg0);
  args = msvcrt_valisttos_aw(arg0, ap, ' ');
  va_end(ap);

  ret = msvcrt_spawn(flags, fullname[0] ? fullname : nameW, args, NULL);

  MSVCRT_free(nameW);
  MSVCRT_free(args);
  return ret;
}

/*********************************************************************
 *		_spawnlpe (MSVCRT.@)
 */
MSVCRT_intptr_t CDECL _spawnlpe(int flags, const char* name, const char* arg0, ...)
{
  static const MSVCRT_wchar_t path[] = {'P','A','T','H',0};
  va_list ap;
  MSVCRT_wchar_t *nameW, *args, *envs = NULL, fullname[MAX_PATH];
  const char * const *envp;
  MSVCRT_intptr_t ret;

  if (!(nameW = msvcrt_wstrdupa(name))) return -1;
  _wsearchenv(nameW, path, fullname);

  va_start(ap, arg0);
  args = msvcrt_valisttos_aw(arg0, ap, ' ');
  va_end(ap);

  va_start(ap, arg0);
  while (va_arg( ap, char * ) != NULL) /*nothing*/;
  envp = va_arg( ap, const char * const * );
  if (envp) envs = msvcrt_argvtos_aw(envp, 0);
  va_end(ap);

  ret = msvcrt_spawn(flags, fullname[0] ? fullname : nameW, args, envs);

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
MSVCRT_intptr_t CDECL _spawnve(int flags, const char* name, const char* const* argv,
                               const char* const* envv)
{
  MSVCRT_wchar_t *nameW, *args, *envs;
  MSVCRT_intptr_t ret;

  if (!(nameW = msvcrt_wstrdupa(name))) return -1;

  args = msvcrt_argvtos_aw(argv, ' ');
  envs = msvcrt_argvtos_aw(envv, 0);

  ret = msvcrt_spawn(flags, nameW, args, envs);

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
MSVCRT_intptr_t CDECL _wspawnve(int flags, const MSVCRT_wchar_t* name, const MSVCRT_wchar_t* const* argv,
                                const MSVCRT_wchar_t* const* envv)
{
  MSVCRT_wchar_t * args = msvcrt_argvtos(argv,' ');
  MSVCRT_wchar_t * envs = msvcrt_argvtos(envv,0);
  MSVCRT_wchar_t fullname[MAX_PATH];
  const MSVCRT_wchar_t *p;
  int len;
  MSVCRT_intptr_t ret = -1;

  TRACE(":call (%s), params (%s), env (%s)\n",debugstr_w(name),debugstr_w(args),
   envs?"Custom":"Null");

  /* no check for NULL name.
     native doesn't do it */

  p = memchrW(name, '\0', MAX_PATH);
  if( !p )
    p = name + MAX_PATH - 1;
  len = p - name;

  /* extra-long names are silently truncated. */
  memcpy(fullname, name, len * sizeof(MSVCRT_wchar_t));

  for( p--; p >= name; p-- )
  {
    if( *p == '\\' || *p == '/' || *p == ':' || *p == '.' )
      break;
  }

  /* if no extension is given, assume .exe */
  if( (p < name || *p != '.') && len <= MAX_PATH - 5 )
  {
    static const MSVCRT_wchar_t dotexe[] = {'.','e','x','e'};

    FIXME("only trying .exe when no extension given\n");
    memcpy(fullname+len, dotexe, 4 * sizeof(MSVCRT_wchar_t));
    len += 4;
  }

  fullname[len] = '\0';

  if (args)
  {
    ret = msvcrt_spawn(flags, fullname, args, envs);
    MSVCRT_free(args);
  }
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
  return _spawnve(flags, name, argv, NULL);
}

/*********************************************************************
 *      _wspawnv (MSVCRT.@)
 *
 * Unicode version of _spawnv
 */
MSVCRT_intptr_t CDECL _wspawnv(int flags, const MSVCRT_wchar_t* name, const MSVCRT_wchar_t* const* argv)
{
  return _wspawnve(flags, name, argv, NULL);
}

/*********************************************************************
 *		_spawnvpe (MSVCRT.@)
 *
 * Like on Windows, this function does not handle arguments with spaces
 * or double-quotes.
 */
MSVCRT_intptr_t CDECL _spawnvpe(int flags, const char* name, const char* const* argv,
                                const char* const* envv)
{
  char fullname[MAX_PATH];
  _searchenv(name, "PATH", fullname);
  return _spawnve(flags, fullname[0] ? fullname : name, argv, envv);
}

/*********************************************************************
 *      _wspawnvpe (MSVCRT.@)
 *
 * Unicode version of _spawnvpe
 */
MSVCRT_intptr_t CDECL _wspawnvpe(int flags, const MSVCRT_wchar_t* name, const MSVCRT_wchar_t* const* argv,
                                 const MSVCRT_wchar_t* const* envv)
{
  static const MSVCRT_wchar_t path[] = {'P','A','T','H',0};
  MSVCRT_wchar_t fullname[MAX_PATH];

  _wsearchenv(name, path, fullname);
  return _wspawnve(flags, fullname[0] ? fullname : name, argv, envv);
}

/*********************************************************************
 *		_spawnvp (MSVCRT.@)
 *
 * Like on Windows, this function does not handle arguments with spaces
 * or double-quotes.
 */
MSVCRT_intptr_t CDECL _spawnvp(int flags, const char* name, const char* const* argv)
{
  return _spawnvpe(flags, name, argv, NULL);
}

/*********************************************************************
 *      _wspawnvp (MSVCRT.@)
 *
 * Unicode version of _spawnvp
 */
MSVCRT_intptr_t CDECL _wspawnvp(int flags, const MSVCRT_wchar_t* name, const MSVCRT_wchar_t* const* argv)
{
  return _wspawnvpe(flags, name, argv, NULL);
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
  int textmode, fds[2], fdToDup, fdToOpen, fdStdHandle = -1, fdStdErr = -1;
  const MSVCRT_wchar_t *p;
  MSVCRT_wchar_t *comspec, *fullcmd;
  unsigned int len;
  static const MSVCRT_wchar_t flag[] = {' ','/','c',' ',0};

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

  if ((fdStdHandle = MSVCRT__dup(fdToDup)) == -1)
    goto error;
  if (MSVCRT__dup2(fds[fdToDup], fdToDup) != 0)
    goto error;
  if (readPipe)
  {
    if ((fdStdErr = MSVCRT__dup(MSVCRT_STDERR_FILENO)) == -1)
      goto error;
    if (MSVCRT__dup2(fds[fdToDup], MSVCRT_STDERR_FILENO) != 0)
      goto error;
  }

  MSVCRT__close(fds[fdToDup]);

  if (!(comspec = msvcrt_get_comspec())) goto error;
  len = strlenW(comspec) + strlenW(flag) + strlenW(command) + 1;

  if (!(fullcmd = HeapAlloc(GetProcessHeap(), 0, len * sizeof(MSVCRT_wchar_t)))) goto error;
  strcpyW(fullcmd, comspec);
  strcatW(fullcmd, flag);
  strcatW(fullcmd, command);
  HeapFree(GetProcessHeap(), 0, comspec);

  if (msvcrt_spawn(MSVCRT__P_NOWAIT, NULL, fullcmd, NULL) == -1)
  {
    MSVCRT__close(fds[fdToOpen]);
    ret = NULL;
  }
  else
  {
    ret = MSVCRT__wfdopen(fds[fdToOpen], mode);
    if (!ret)
      MSVCRT__close(fds[fdToOpen]);
  }
  HeapFree(GetProcessHeap(), 0, fullcmd);
  MSVCRT__dup2(fdStdHandle, fdToDup);
  MSVCRT__close(fdStdHandle);
  if (readPipe)
  {
    MSVCRT__dup2(fdStdErr, MSVCRT_STDERR_FILENO);
    MSVCRT__close(fdStdErr);
  }
  return ret;

error:
  if (fdStdHandle != -1) MSVCRT__close(fdStdHandle);
  if (fdStdErr != -1)    MSVCRT__close(fdStdErr);
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
  return MSVCRT_fclose(file);
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

  if (!(comspec = msvcrt_get_comspec())) return -1;
  len = strlenW(comspec) + strlenW(flag) + strlenW(cmd) + 1;

  if (!(fullcmd = HeapAlloc(GetProcessHeap(), 0, len * sizeof(MSVCRT_wchar_t))))
  {
    HeapFree(GetProcessHeap(), 0, comspec);
    return -1;
  }
  strcpyW(fullcmd, comspec);
  strcatW(fullcmd, flag);
  strcatW(fullcmd, cmd);

  res = msvcrt_spawn(MSVCRT__P_WAIT, comspec, fullcmd, NULL);

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
