/*
 * CRTDLL spawn functions
 * 
 * Copyright 1996,1998 Marcus Meissner
 * Copyright 1996 Jukka Iivonen
 * Copyright 1997,2000 Uwe Bonnes
 * Copyright 2000 Jon Griffiths
 *
 * These functions differ in whether they pass arguments as an array
 * (v in the name) or as varags (l in the name), whether they
 * seach the path (p in the name) and/or whether they take an
 * environment (e in the name) or pass the parents environment.
 *                  Args as   Search   Take
 * Name             varargs?  path?    environment?
 * spawnl              N         N        N
 * spawnle             N         N        Y
 * spawnlp             N         Y        N
 * spawnlpe            N         Y        Y
 * spawnv              Y         N        N
 * spawnve             Y         N        Y
 * spawnvp             Y         Y        N
 * spawnvpe            Y         Y        Y
 *
 * Implementation Notes:
 * MT Safe - But only because of missing functionality.
 * 
 * After translating input arguments into the required format for
 * CreateProcess(), the internal function __CRTDLL__spawn() is
 * called to perform the actual spawning.
 *
 * FIXME:
 * -File handles need some special handling. Sometimes children get
 *  open file handles, sometimes not. The docs are confusing.
 * -No check for maximum path/argument/environment size is done.
 * -Wine has a "process.h" which is not the same as any crt version.
 * Unresolved issues Uwe Bonnes 970904:
 * -system-call calls another wine process, but without debugging arguments
 *  and uses the first wine executable in the path
 */

#include "crtdll.h"
#include <errno.h>
#include "process.h"
#include <stdlib.h>


DEFAULT_DEBUG_CHANNEL(crtdll);

/* Process creation flags */
#define _P_WAIT    0
#define _P_NOWAIT  1
#define _P_OVERLAY 2
#define _P_NOWAITO 3
#define _P_DETACH  4


extern void __CRTDLL__set_errno(ULONG err);
extern LPVOID __cdecl CRTDLL_calloc(DWORD size, DWORD count);
extern VOID __cdecl CRTDLL_free(void *ptr);
extern VOID __cdecl CRTDLL__exit(LONG ret);
extern INT CRTDLL_doserrno;


/* INTERNAL: Spawn a child process */
static int __CRTDLL__spawn(INT flags, LPSTR exe, LPSTR args, LPSTR env);
static int __CRTDLL__spawn(INT flags, LPSTR exe, LPSTR args, LPSTR env)
{
  STARTUPINFOA si;
  PROCESS_INFORMATION pi;

  if ((unsigned)flags > _P_DETACH)
  {
    CRTDLL_errno = EINVAL;
    return -1;
  }

  FIXME(":must dup/kill streams for child process\n");

  memset(&si, 0, sizeof(si));
  si.cb = sizeof(si);

  if (!CreateProcessA(exe, args, NULL, NULL, TRUE,
		      flags == _P_DETACH ? DETACHED_PROCESS : 0,
		      env, NULL, &si, &pi))
  {
    __CRTDLL__set_errno(GetLastError());
    return -1;
  }

  switch(flags)
  {
  case _P_WAIT:
    WaitForSingleObject(pi.hProcess,-1); /* wait forvever */
    GetExitCodeProcess(pi.hProcess,&pi.dwProcessId);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return pi.dwProcessId;
  case _P_DETACH:
    CloseHandle(pi.hProcess);
    pi.hProcess = 0;
    /* fall through */
  case _P_NOWAIT:
  case _P_NOWAITO:
    CloseHandle(pi.hThread);
    return pi.hProcess;
  case  _P_OVERLAY:
    CRTDLL__exit(0);
  }
  return -1; /* cant reach here */
}


/* INTERNAL: Convert argv list to a single 'delim'-seperated string */
static LPSTR __CRTDLL__argvtos(LPSTR *arg, CHAR delim);
static LPSTR __CRTDLL__argvtos(LPSTR *arg, CHAR delim)
{
  LPSTR *search = arg;
  LONG size = 0;
  LPSTR ret;

  if (!arg && !delim)
    return NULL;

  /* get length */
  while(*search)
  {
    size += strlen(*search) + 1;
    search++;
  }

  if (!(ret = (LPSTR)CRTDLL_calloc(size + 1, 1)))
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
 *                  _spawnve    (CRTDLL.274)
 *
 * Spawn a process.
 *
 */
HANDLE __cdecl CRTDLL__spawnve(INT flags, LPSTR name, LPSTR *argv, LPSTR *envv)
{
  LPSTR args = __CRTDLL__argvtos(argv,' ');
  LPSTR envs = __CRTDLL__argvtos(envv,0);
  LPSTR fullname = name;

  FIXME(":not translating name %s to locate program\n",fullname);
  TRACE(":call (%s), params (%s), env (%s)\n",name,args,envs?"Custom":"Null");

  if (args)
  {
    HANDLE ret = __CRTDLL__spawn(flags, fullname, args, envs);
    CRTDLL_free(args);
    CRTDLL_free(envs);
    return ret;
  }
  if (envs)
    CRTDLL_free(envs);

  WARN(":No argv[0] passed - process will not be executed");
  return -1;
}


/*********************************************************************
 *                  system       (CRTDLL.485)
 */
INT __cdecl CRTDLL_system(LPSTR x)
{
    /* FIXME: should probably launch cmd interpreter in COMSPEC */
    return __CRTDLL__spawn(_P_WAIT, NULL, x, NULL);
}
