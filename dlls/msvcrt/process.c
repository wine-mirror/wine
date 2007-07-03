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

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

/* INTERNAL: Spawn a child process */
static MSVCRT_intptr_t msvcrt_spawn(int flags, const char* exe, char* cmdline, char* env)
{
  STARTUPINFOA si;
  PROCESS_INFORMATION pi;

  if ((unsigned)flags > MSVCRT__P_DETACH)
  {
    *MSVCRT__errno() = MSVCRT_EINVAL;
    return -1;
  }

  memset(&si, 0, sizeof(si));
  si.cb = sizeof(si);
  msvcrt_create_io_inherit_block(&si);
  if (!CreateProcessA(exe, cmdline, NULL, NULL, TRUE,
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

  ret = MSVCRT_malloc(size + 1);
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
  size = 0;
  do {
      size += strlen(arg) + 1;
      arg = va_arg(alist, char*);
  } while (arg != NULL);

  ret = MSVCRT_malloc(size + 1);
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
 *		_execl (MSVCRT.@)
 *
 * Like on Windows, this function does not handle arguments with spaces
 * or double-quotes.
 */
MSVCRT_intptr_t CDECL _execl(const char* name, const char* arg0, ...)
{
  va_list ap;
  char * args;
  MSVCRT_intptr_t ret;

  va_start(ap, arg0);
  args = msvcrt_valisttos(arg0, ap, ' ');
  va_end(ap);

  ret = msvcrt_spawn(MSVCRT__P_OVERLAY, name, args, NULL);
  MSVCRT_free(args);

  return ret;
}

/*********************************************************************
 *		_execle (MSVCRT.@)
 */
MSVCRT_intptr_t CDECL _execle(const char* name, const char* arg0, ...)
{
    FIXME("stub\n");
    return -1;
}

/*********************************************************************
 *		_execlp (MSVCRT.@)
 *
 * Like on Windows, this function does not handle arguments with spaces
 * or double-quotes.
 */
MSVCRT_intptr_t CDECL _execlp(const char* name, const char* arg0, ...)
{
  va_list ap;
  char * args;
  MSVCRT_intptr_t ret;
  char fullname[MAX_PATH];

  _searchenv(name, "PATH", fullname);

  va_start(ap, arg0);
  args = msvcrt_valisttos(arg0, ap, ' ');
  va_end(ap);

  ret = msvcrt_spawn(MSVCRT__P_OVERLAY, fullname[0] ? fullname : name, args, NULL);
  MSVCRT_free(args);

  return ret;
}

/*********************************************************************
 *		_execlpe (MSVCRT.@)
 */
MSVCRT_intptr_t CDECL _execlpe(const char* name, const char* arg0, ...)
{
    FIXME("stub\n");
    return -1;
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
  char * args;
  MSVCRT_intptr_t ret;

  va_start(ap, arg0);
  args = msvcrt_valisttos(arg0, ap, ' ');
  va_end(ap);

  ret = msvcrt_spawn(flags, name, args, NULL);
  MSVCRT_free(args);

  return ret;
}

/*********************************************************************
 *		_spawnle (MSVCRT.@)
 */
MSVCRT_intptr_t CDECL _spawnle(int flags, const char* name, const char* arg0, ...)
{
    va_list ap;
    char *args, *envs = NULL;
    const char * const *envp;
    MSVCRT_intptr_t ret;

    va_start(ap, arg0);
    args = msvcrt_valisttos(arg0, ap, ' ');
    va_end(ap);

    va_start(ap, arg0);
    while (va_arg( ap, char * ) != NULL) /*nothing*/;
    envp = va_arg( ap, const char * const * );
    if (envp) envs = msvcrt_argvtos(envp, 0);
    va_end(ap);

    ret = msvcrt_spawn(flags, name, args, envs);

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
  va_list ap;
  char * args;
  MSVCRT_intptr_t ret;
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
 *		_spawnlpe (MSVCRT.@)
 */
MSVCRT_intptr_t CDECL _spawnlpe(int flags, const char* name, const char* arg0, ...)
{
    va_list ap;
    char *args, *envs = NULL;
    const char * const *envp;
    MSVCRT_intptr_t ret;
    char fullname[MAX_PATH];

    _searchenv(name, "PATH", fullname);

    va_start(ap, arg0);
    args = msvcrt_valisttos(arg0, ap, ' ');
    va_end(ap);

    va_start(ap, arg0);
    while (va_arg( ap, char * ) != NULL) /*nothing*/;
    envp = va_arg( ap, const char * const * );
    if (envp) envs = msvcrt_argvtos(envp, 0);
    va_end(ap);

    ret = msvcrt_spawn(flags, fullname[0] ? fullname : name, args, envs);

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
  char * args = msvcrt_argvtos(argv,' ');
  char * envs = msvcrt_argvtos(envv,0);
  char fullname[MAX_PATH];
  const char *p;
  int len;
  MSVCRT_intptr_t ret = -1;

  TRACE(":call (%s), params (%s), env (%s)\n",debugstr_a(name),debugstr_a(args),
   envs?"Custom":"Null");

  /* no check for NULL name.
     native doesn't do it */

  p = memchr(name, '\0', MAX_PATH);
  if( !p )
    p = name + MAX_PATH - 1;
  len = p - name;

  /* extra-long names are silently truncated. */
  memcpy(fullname, name, len);

  for( p--; p >= name; p-- )
  {
    if( *p == '\\' || *p == '/' || *p == ':' || *p == '.' )
      break;
  }

  /* if no extension is given, assume .exe */
  if( (p < name || *p != '.') && len <= MAX_PATH - 5 )
  {
    FIXME("only trying .exe when no extension given\n");
    memcpy(fullname+len, ".exe", 4);
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
 *		_popen (MSVCRT.@)
 * FIXME: convert to _wpopen and call that from here instead?  But it
 * would have to convert the command back to ANSI to call msvcrt_spawn,
 * less than ideal.
 */
MSVCRT_FILE* CDECL MSVCRT__popen(const char* command, const char* mode)
{
  static const char wcmd[] = "cmd", cmdFlag[] = " /C ", comSpec[] = "COMSPEC";
  MSVCRT_FILE *ret;
  BOOL readPipe = TRUE;
  int textmode, fds[2], fdToDup, fdToOpen, fdStdHandle = -1, fdStdErr = -1;
  const char *p;
  char *cmdcopy;
  DWORD comSpecLen;

  TRACE("(command=%s, mode=%s)\n", debugstr_a(command), debugstr_a(mode));

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
  if (_pipe(fds, 0, textmode) == -1)
    return NULL;

  fdToDup = readPipe ? 1 : 0;
  fdToOpen = readPipe ? 0 : 1;

  if ((fdStdHandle = _dup(fdToDup)) == -1)
    goto error;
  if (_dup2(fds[fdToDup], fdToDup) != 0)
    goto error;
  if (readPipe)
  {
    if ((fdStdErr = _dup(MSVCRT_STDERR_FILENO)) == -1)
      goto error;
    if (_dup2(fds[fdToDup], MSVCRT_STDERR_FILENO) != 0)
      goto error;
  }

  _close(fds[fdToDup]);

  comSpecLen = GetEnvironmentVariableA(comSpec, NULL, 0);
  if (!comSpecLen)
    comSpecLen = strlen(wcmd) + 1;
  cmdcopy = HeapAlloc(GetProcessHeap(), 0, comSpecLen + strlen(cmdFlag)
   + strlen(command));
  if (!GetEnvironmentVariableA(comSpec, cmdcopy, comSpecLen))
    strcpy(cmdcopy, wcmd);
  strcat(cmdcopy, cmdFlag);
  strcat(cmdcopy, command);
  if (msvcrt_spawn(MSVCRT__P_NOWAIT, NULL, cmdcopy, NULL) == -1)
  {
    _close(fds[fdToOpen]);
    ret = NULL;
  }
  else
  {
    ret = MSVCRT__fdopen(fds[fdToOpen], mode);
    if (!ret)
      _close(fds[fdToOpen]);
  }
  HeapFree(GetProcessHeap(), 0, cmdcopy);
  _dup2(fdStdHandle, fdToDup);
  _close(fdStdHandle);
  if (readPipe)
  {
    _dup2(fdStdErr, MSVCRT_STDERR_FILENO);
    _close(fdStdErr);
  }
  return ret;

error:
  if (fdStdHandle != -1) _close(fdStdHandle);
  if (fdStdErr != -1)    _close(fdStdErr);
  _close(fds[0]);
  _close(fds[1]);
  return NULL;
}

/*********************************************************************
 *		_wpopen (MSVCRT.@)
 */
MSVCRT_FILE* CDECL MSVCRT__wpopen(const MSVCRT_wchar_t* command, const MSVCRT_wchar_t* mode)
{
  FIXME("(command=%s, mode=%s): stub\n", debugstr_w(command), debugstr_w(mode));
  return NULL;
}

/*********************************************************************
 *		_pclose (MSVCRT.@)
 */
int CDECL MSVCRT__pclose(MSVCRT_FILE* file)
{
  return MSVCRT_fclose(file);
}

/*********************************************************************
 *		system (MSVCRT.@)
 */
int CDECL MSVCRT_system(const char* cmd)
{
    char* cmdcopy;
    int res;

    /* Make a writable copy for CreateProcess */
    cmdcopy=_strdup(cmd);
    /* FIXME: should probably launch cmd interpreter in COMSPEC */
    res=msvcrt_spawn(MSVCRT__P_WAIT, NULL, cmdcopy, NULL);
    MSVCRT_free(cmdcopy);
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
