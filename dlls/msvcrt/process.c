/*
 * msvcrt.dll spawn/exec functions
 *
 * Copyright 1996,1998 Marcus Meissner
 * Copyright 1996 Jukka Iivonen
 * Copyright 1997,2000 Uwe Bonnes
 * Copyright 2000 Jon Griffiths
 *
 * FIXME:
 * -File handles need some special handling. Sometimes children get
 *  open file handles, sometimes not. The docs are confusing
 * -No check for maximum path/argument/environment size is done
 */
#include "msvcrt.h"
#include "ms_errno.h"

#include "msvcrt/process.h"
#include "msvcrt/stdlib.h"

DEFAULT_DEBUG_CHANNEL(msvcrt);

/* FIXME: Check file extensions for app to run */
static const unsigned int EXE = 'e' << 16 | 'x' << 8 | 'e';
static const unsigned int BAT = 'b' << 16 | 'a' << 8 | 't';
static const unsigned int CMD = 'c' << 16 | 'm' << 8 | 'd';
static const unsigned int COM = 'c' << 16 | 'o' << 8 | 'm';

/* INTERNAL: Spawn a child process */
static int msvcrt_spawn(int flags, const char* exe, char* args, char* env)
{
  STARTUPINFOA si;
  PROCESS_INFORMATION pi;

  if (sizeof(HANDLE) != sizeof(int))
    WARN("This call is unsuitable for your architecture\n");

  if ((unsigned)flags > _P_DETACH)
  {
    SET_THREAD_VAR(errno,MSVCRT_EINVAL);
    return -1;
  }

  FIXME(":must dup/kill streams for child process\n");

  memset(&si, 0, sizeof(si));
  si.cb = sizeof(si);

  if (!CreateProcessA(exe, args, NULL, NULL, TRUE,
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

/* INTERNAL: Convert argv list to a single 'delim'-separated string */
static char* msvcrt_argvtos(const char* const* arg, char delim)
{
  const char* const* search = arg;
  long size = 0;
  char *ret;

  if (!arg && !delim)
    return NULL;

  /* get length */
  while(*search)
  {
    size += strlen(*search) + 1;
    search++;
  }

  if (!(ret = (char *)MSVCRT_calloc(size + 1, 1)))
    return NULL;

  /* fill string */
  search = arg;
  size = 0;
  while(*search)
  {
    int strsize = strlen(*search);
    memcpy(ret+size,*search,strsize);
    ret[size+strsize] = delim;
    size += strsize + 1;
    search++;
  }
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
    SET_THREAD_VAR(errno, MSVCRT_ECHILD);
    SET_THREAD_VAR(doserrno,doserrno);
  }
  else
    MSVCRT__set_errno(doserrno);

  return status ? *status = -1 : -1;
}

/*********************************************************************
 *		_spawnve (MSVCRT.@)
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
 */
int _spawnv(int flags, const char* name, const char* const* argv)
{
  return _spawnve(flags, name, argv, NULL);
}

/*********************************************************************
 *		_spawnvpe (MSVCRT.@)
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
  /* FIXME: should probably launch cmd interpreter in COMSPEC */
  return msvcrt_spawn(_P_WAIT, cmd, NULL, NULL);
}

/*********************************************************************
 *		_loaddll (MSVCRT.@)
 */
int _loaddll(const char* dllname)
{
  return LoadLibraryA(dllname);
}

/*********************************************************************
 *		_unloaddll (MSVCRT.@)
 */
int _unloaddll(int dll)
{
  if (FreeLibrary((HANDLE)dll))
    return 0;
  else
  {
    int err = GetLastError();
    MSVCRT__set_errno(err);
    return err;
  }
}

