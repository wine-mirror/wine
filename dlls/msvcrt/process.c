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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * FIXME:
 * -File handles need some special handling. Sometimes children get
 *  open file handles, sometimes not. The docs are confusing
 * -No check for maximum path/argument/environment size is done
 */
#include "config.h"

#include <stdarg.h>

#include "msvcrt.h"
#include "msvcrt/errno.h"

#include "msvcrt/process.h"
#include "msvcrt/stdlib.h"
#include "msvcrt/string.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

/* FIXME: Check file extensions for app to run */
static const unsigned int EXE = 'e' << 16 | 'x' << 8 | 'e';
static const unsigned int BAT = 'b' << 16 | 'a' << 8 | 't';
static const unsigned int CMD = 'c' << 16 | 'm' << 8 | 'd';
static const unsigned int COM = 'c' << 16 | 'o' << 8 | 'm';

/* INTERNAL: Spawn a child process */
static int msvcrt_spawn(int flags, const char* exe, char* cmdline, char* env)
{
  STARTUPINFOA si;
  PROCESS_INFORMATION pi;

  if (sizeof(HANDLE) != sizeof(int))
    WARN("This call is unsuitable for your architecture\n");

  if ((unsigned)flags > _P_DETACH)
  {
    *MSVCRT__errno() = MSVCRT_EINVAL;
    return -1;
  }

  FIXME(":must dup/kill streams for child process\n");

  memset(&si, 0, sizeof(si));
  si.cb = sizeof(si);

  if (!CreateProcessA(exe, cmdline, NULL, NULL, TRUE,
                     flags == _P_DETACH ? DETACHED_PROCESS : 0,
                     env, NULL, &si, &pi))
  {
    MSVCRT__set_errno(GetLastError());
    return -1;
  }

  switch(flags)
  {
  case _P_WAIT:
    WaitForSingleObject(pi.hProcess,-1); /* wait forvever */
    GetExitCodeProcess(pi.hProcess,&pi.dwProcessId);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return (int)pi.dwProcessId;
  case _P_DETACH:
    CloseHandle(pi.hProcess);
    pi.hProcess = 0;
    /* fall through */
  case _P_NOWAIT:
  case _P_NOWAITO:
    CloseHandle(pi.hThread);
    return (int)pi.hProcess;
  case  _P_OVERLAY:
    MSVCRT__exit(0);
  }
  return -1; /* can't reach here */
}

/* INTERNAL: Convert argv list to a single 'delim'-separated string, with an
 * extra '\0' to terminate it
 */
static char* msvcrt_argvtos(const char* const* arg, char delim)
{
  const char* const* a;
  long size;
  char* p;
  char* ret;

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
    size += strlen(*a) + 1;
    a++;
  }

  ret = (char*)MSVCRT_malloc(size + 1);
  if (!ret)
    return NULL;

  /* fill string */
  a = arg;
  p = ret;
  while (*a)
  {
    int len = strlen(*a);
    memcpy(p,*a,len);
    p += len;
    *p++ = delim;
    a++;
  }
  if (delim && p > ret) p[-1] = 0;
  else *p = 0;
  return ret;
}

/* INTERNAL: Convert va_list to a single 'delim'-separated string, with an
 * extra '\0' to terminate it
 */
static char* msvcrt_valisttos(const char* arg0, va_list alist, char delim)
{
  va_list alist2;
  long size;
  const char *arg;
  char* p;
  char *ret;

#if HAVE_VA_COPY
  va_copy(alist2,alist);
#else
# if HAVE___VA_COPY
  __va_copy(alist2,alist);
# else
  alist2 = alist;
# endif
#endif

  if (!arg0 && !delim)
  {
      /* Return NULL for an empty environment list */
      return NULL;
  }

  /* get length */
  arg = arg0;
  size = 0;
  do {
      size += strlen(arg) + 1;
      arg = va_arg(alist, char*);
  } while (arg != NULL);

  ret = (char*)MSVCRT_malloc(size + 1);
  if (!ret)
    return NULL;

  /* fill string */
  arg = arg0;
  p = ret;
  do {
      int len = strlen(arg);
      memcpy(p,arg,len);
      p += len;
      *p++ = delim;
      arg = va_arg(alist2, char*);
  } while (arg != NULL);
  if (delim && p > ret) p[-1] = 0;
  else *p = 0;
  return ret;
}

/*********************************************************************
 *		_cwait (MSVCRT.@)
 */
int _cwait(int *status, int pid, int action)
{
  HANDLE hPid = (HANDLE)pid;
  int doserrno;

  action = action; /* Remove warning */

  if (!WaitForSingleObject(hPid, -1)) /* wait forever */
  {
    if (status)
    {
      DWORD stat;
      GetExitCodeProcess(hPid, &stat);
      *status = (int)stat;
    }
    return (int)pid;
  }
  doserrno = GetLastError();

  if (doserrno == ERROR_INVALID_HANDLE)
  {
    *MSVCRT__errno() =  MSVCRT_ECHILD;
    *__doserrno() = doserrno;
  }
  else
    MSVCRT__set_errno(doserrno);

  return status ? *status = -1 : -1;
}

/*********************************************************************
 *		_execl (MSVCRT.@)
 *
 * Like on Windows, this function does not handle arguments with spaces
 * or double-quotes.
 */
int _execl(const char* name, const char* arg0, ...)
{
  va_list ap;
  char * args;
  int ret;

  va_start(ap, arg0);
  args = msvcrt_valisttos(arg0, ap, ' ');
  va_end(ap);

  ret = msvcrt_spawn(_P_OVERLAY, name, args, NULL);
  MSVCRT_free(args);

  return ret;
}

/*********************************************************************
 *		_execlp (MSVCRT.@)
 *
 * Like on Windows, this function does not handle arguments with spaces
 * or double-quotes.
 */
int _execlp(const char* name, const char* arg0, ...)
{
  va_list ap;
  char * args;
  int ret;
  char fullname[MAX_PATH];

  _searchenv(name, "PATH", fullname);

  va_start(ap, arg0);
  args = msvcrt_valisttos(arg0, ap, ' ');
  va_end(ap);

  ret = msvcrt_spawn(_P_OVERLAY, fullname[0] ? fullname : name, args, NULL);
  MSVCRT_free(args);

  return ret;
}

/*********************************************************************
 *		_execv (MSVCRT.@)
 *
 * Like on Windows, this function does not handle arguments with spaces
 * or double-quotes.
 */
int _execv(const char* name, char* const* argv)
{
  return _spawnve(_P_OVERLAY, name, (const char* const*) argv, NULL);
}

/*********************************************************************
 *		_execve (MSVCRT.@)
 *
 * Like on Windows, this function does not handle arguments with spaces
 * or double-quotes.
 */
int _execve(const char* name, char* const* argv, const char* const* envv)
{
  return _spawnve(_P_OVERLAY, name, (const char* const*) argv, envv);
}

/*********************************************************************
 *		_execvpe (MSVCRT.@)
 *
 * Like on Windows, this function does not handle arguments with spaces
 * or double-quotes.
 */
int _execvpe(const char* name, char* const* argv, const char* const* envv)
{
  char fullname[MAX_PATH];

  _searchenv(name, "PATH", fullname);
  return _spawnve(_P_OVERLAY, fullname[0] ? fullname : name,
                  (const char* const*) argv, envv);
}

/*********************************************************************
 *		_execvp (MSVCRT.@)
 *
 * Like on Windows, this function does not handle arguments with spaces
 * or double-quotes.
 */
int _execvp(const char* name, char* const* argv)
{
  return _execvpe(name, argv, NULL);
}

/*********************************************************************
 *		_spawnl (MSVCRT.@)
 *
 * Like on Windows, this function does not handle arguments with spaces
 * or double-quotes.
 */
int _spawnl(int flags, const char* name, const char* arg0, ...)
{
  va_list ap;
  char * args;
  int ret;

  va_start(ap, arg0);
  args = msvcrt_valisttos(arg0, ap, ' ');
  va_end(ap);

  ret = msvcrt_spawn(flags, name, args, NULL);
  MSVCRT_free(args);

  return ret;
}

/*********************************************************************
 *		_spawnlp (MSVCRT.@)
 *
 * Like on Windows, this function does not handle arguments with spaces
 * or double-quotes.
 */
int _spawnlp(int flags, const char* name, const char* arg0, ...)
{
  va_list ap;
  char * args;
  int ret;
  char fullname[MAX_PATH];

  _searchenv(name, "PATH", fullname);

  va_start(ap, arg0);
  args = msvcrt_valisttos(arg0, ap, ' ');
  va_end(ap);

  ret = msvcrt_spawn(flags, fullname[0] ? fullname : name, args, NULL);
  MSVCRT_free(args);

  return ret;
}

/*********************************************************************
 *		_spawnve (MSVCRT.@)
 *
 * Like on Windows, this function does not handle arguments with spaces
 * or double-quotes.
 */
int _spawnve(int flags, const char* name, const char* const* argv,
                            const char* const* envv)
{
  char * args = msvcrt_argvtos(argv,' ');
  char * envs = msvcrt_argvtos(envv,0);
  const char *fullname = name;
  int ret = -1;

  FIXME(":not translating name %s to locate program\n",fullname);
  TRACE(":call (%s), params (%s), env (%s)\n",name,args,envs?"Custom":"Null");

  if (args)
  {
    ret = msvcrt_spawn(flags, fullname, args, envs);
    MSVCRT_free(args);
  }
  if (envs)
    MSVCRT_free(envs);

  return ret;
}

/*********************************************************************
 *		_spawnv (MSVCRT.@)
 *
 * Like on Windows, this function does not handle arguments with spaces
 * or double-quotes.
 */
int _spawnv(int flags, const char* name, const char* const* argv)
{
  return _spawnve(flags, name, argv, NULL);
}

/*********************************************************************
 *		_spawnvpe (MSVCRT.@)
 *
 * Like on Windows, this function does not handle arguments with spaces
 * or double-quotes.
 */
int _spawnvpe(int flags, const char* name, const char* const* argv,
                            const char* const* envv)
{
  char fullname[MAX_PATH];
  _searchenv(name, "PATH", fullname);
  return _spawnve(flags, fullname[0] ? fullname : name, argv, envv);
}

/*********************************************************************
 *		_spawnvp (MSVCRT.@)
 *
 * Like on Windows, this function does not handle arguments with spaces
 * or double-quotes.
 */
int _spawnvp(int flags, const char* name, const char* const* argv)
{
  return _spawnvpe(flags, name, argv, NULL);
}

/*********************************************************************
 *		system (MSVCRT.@)
 */
int MSVCRT_system(const char* cmd)
{
    char* cmdcopy;
    int res;

    /* Make a writable copy for CreateProcess */
    cmdcopy=_strdup(cmd);
    /* FIXME: should probably launch cmd interpreter in COMSPEC */
    res=msvcrt_spawn(_P_WAIT, NULL, cmdcopy, NULL);
    MSVCRT_free(cmdcopy);
    return res;
}

/*********************************************************************
 *		_loaddll (MSVCRT.@)
 */
int _loaddll(const char* dllname)
{
  return (int)LoadLibraryA(dllname);
}

/*********************************************************************
 *		_unloaddll (MSVCRT.@)
 */
int _unloaddll(int dll)
{
  if (FreeLibrary((HMODULE)dll))
    return 0;
  else
  {
    int err = GetLastError();
    MSVCRT__set_errno(err);
    return err;
  }
}
