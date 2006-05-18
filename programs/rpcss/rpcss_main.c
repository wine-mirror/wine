/*
 * Copyright 2001, Ove Kåven, TransGaming Technologies Inc.
 * Copyright 2002 Greg Turner
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
 * ---- rpcss_main.c:
 *   Initialize and start serving requests.  Bail if rpcss already is
 *   running.
 *
 * ---- RPCSS.EXE:
 *   
 *   Wine needs a server whose role is somewhat like that
 *   of rpcss.exe in windows.  This is not a clone of
 *   windows rpcss at all.  It has been given the same name, however,
 *   to provide for the possibility that at some point in the future, 
 *   it may become interface compatible with the "real" rpcss.exe on
 *   Windows.
 *
 * ---- KNOWN BUGS / TODO:
 *
 *   o Service hooks are unimplemented (if you bother to implement
 *     these, also implement net.exe, at least for "net start" and
 *     "net stop" (should be pretty easy I guess, assuming the rest
 *     of the services API infrastructure works.
 * 
 *   o Is supposed to use RPC, not random kludges, to map endpoints.
 *
 *   o Probably name services should be implemented here as well.
 *
 *   o Wine's named pipes (in general) may not interoperate with those of 
 *     Windows yet (?)
 *
 *   o There is a looming problem regarding listening on privileged
 *     ports.  We will need to be able to coexist with SAMBA, and be able
 *     to function without running winelib code as root.  This may
 *     take some doing, including significant reconceptualization of the
 *     role of rpcss.exe in wine.
 *
 *   o Who knows?  Whatever rpcss does, we ought to at
 *     least think about doing... but what /does/ it do?
 */

#include <stdio.h>
#include <limits.h>
#include <assert.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "rpcss.h"
#include "winnt.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

static HANDLE master_mutex;
static long max_lazy_timeout = RPCSS_DEFAULT_MAX_LAZY_TIMEOUT;

HANDLE RPCSS_GetMasterMutex(void)
{
  return master_mutex;
}

void RPCSS_SetMaxLazyTimeout(long mlt)
{
  /* FIXME: this max ensures that no caller will decrease our wait time,
     but could have other bad results.  fix: Store "next_max_lazy_timeout" 
     and install it as necessary next time we "do work"? */
  max_lazy_timeout = max(RPCSS_GetLazyTimeRemaining(), mlt);
}

long RPCSS_GetMaxLazyTimeout(void)
{
  return max_lazy_timeout;
}

/* when do we just give up and bail? (UTC) */
static SYSTEMTIME lazy_timeout_time;

#if defined(NONAMELESSSTRUCT)
# define FILETIME_TO_ULARGEINT(filetime, ularge) \
    ( ularge.u.LowPart = filetime.dwLowDateTime, \
      ularge.u.HighPart = filetime.dwHighDateTime )
# define ULARGEINT_TO_FILETIME(ularge, filetime) \
    ( filetime.dwLowDateTime = ularge.u.LowPart, \
      filetime.dwHighDateTime = ularge.u.HighPart )
#else
# define FILETIME_TO_ULARGEINT(filetime, ularge) \
    ( ularge.LowPart = filetime.dwLowDateTime, \
      ularge.HighPart = filetime.dwHighDateTime )
# define ULARGEINT_TO_FILETIME(ularge, filetime) \
    ( filetime.dwLowDateTime = ularge.LowPart, \
      filetime.dwHighDateTime = ularge.HighPart )
#endif /* NONAMELESSSTRUCT */

#define TEN_MIL ((ULONGLONG)10000000)

/* returns time remaining in seconds */
long RPCSS_GetLazyTimeRemaining(void)
{
  SYSTEMTIME st_just_now;
  FILETIME ft_jn, ft_ltt;
  ULARGE_INTEGER ul_jn, ul_ltt;

  GetSystemTime(&st_just_now);
  SystemTimeToFileTime(&st_just_now, &ft_jn);
  FILETIME_TO_ULARGEINT(ft_jn, ul_jn);

  SystemTimeToFileTime(&lazy_timeout_time, &ft_ltt);
  FILETIME_TO_ULARGEINT(ft_ltt, ul_ltt);
  
  if (ul_jn.QuadPart > ul_ltt.QuadPart)
    return 0;
  else
    return (ul_ltt.QuadPart - ul_jn.QuadPart) / TEN_MIL;
}

void RPCSS_SetLazyTimeRemaining(long seconds)
{
  SYSTEMTIME st_just_now;
  FILETIME ft_jn, ft_ltt;
  ULARGE_INTEGER ul_jn, ul_ltt;

  WINE_TRACE("(seconds == %ld)\n", seconds);

  assert(seconds >= 0);     /* negatives are not allowed */
  
  GetSystemTime(&st_just_now);
  SystemTimeToFileTime(&st_just_now, &ft_jn);
  FILETIME_TO_ULARGEINT(ft_jn, ul_jn);

  /* we want to find the time ltt, s.t. ltt = just_now + seconds */
  ul_ltt.QuadPart = ul_jn.QuadPart + seconds * TEN_MIL;

  /* great. just remember it */
  ULARGEINT_TO_FILETIME(ul_ltt, ft_ltt);
  if (! FileTimeToSystemTime(&ft_ltt, &lazy_timeout_time))
    assert(FALSE);
}

#undef FILETIME_TO_ULARGEINT
#undef ULARGEINT_TO_FILETIME
#undef TEN_MIL

static BOOL RPCSS_work(void)
{
  return RPCSS_NPDoWork();
}

static BOOL RPCSS_Empty(void)
{
  BOOL rslt = TRUE;

  rslt = RPCSS_EpmapEmpty();

  return rslt;
}

BOOL RPCSS_ReadyToDie(void)
{
  long ltr = RPCSS_GetLazyTimeRemaining();
  LONG stc = RPCSS_SrvThreadCount();
  BOOL empty = RPCSS_Empty();
  return ( empty && (ltr <= 0) && (stc == 0) );
}

static BOOL RPCSS_Initialize(void)
{
  WINE_TRACE("\n");

  master_mutex = CreateMutexA( NULL, FALSE, RPCSS_MASTER_MUTEX_NAME);
  if (!master_mutex) {
    WINE_ERR("Failed to create master mutex\n");
    return FALSE;
  }

  if (!RPCSS_BecomePipeServer()) {
    WINE_WARN("Server already running: exiting.\n");

    CloseHandle(master_mutex);
    master_mutex = NULL;

    return FALSE;
  }

  return TRUE;
}

/* returns false if we discover at the last moment that we
   aren't ready to terminate */
static BOOL RPCSS_Shutdown(void)
{
  if (!RPCSS_UnBecomePipeServer())
    return FALSE;
   
  if (!CloseHandle(master_mutex))
    WINE_WARN("Failed to release master mutex\n");

  master_mutex = NULL;

  return TRUE;
}

static void RPCSS_MainLoop(void)
{
  BOOL did_something_new;

  WINE_TRACE("\n");

  for (;;) {
    did_something_new = FALSE;

    while ( (! did_something_new) && (! RPCSS_ReadyToDie()) )
      did_something_new = (RPCSS_work() || did_something_new);

    if ((! did_something_new) && RPCSS_ReadyToDie())
      break; /* that's it for us */

    if (did_something_new)
      RPCSS_SetLazyTimeRemaining(max_lazy_timeout);
  }
}

static BOOL RPCSS_ProcessArgs( int argc, char **argv )
{
  int i;
  char *c,*c1;

  for (i = 1; i < argc; i++) {
    c = argv[i];
    while (*c == ' ') c++;
    if ((*c != '-') && (*c != '/'))
      return FALSE;
    c++;
    switch (*(c++)) {
      case 't':
      case 'T':
        while (*c == ' ') c++;
	if (*c == '\0')  {
	  /* next arg */
	  if (++i >= argc)
	    return FALSE;
	  c = argv[i];
	  while (*c == ' ') c++;
	  if (c == '\0')
	    return FALSE;
	} else
	  return FALSE;
        max_lazy_timeout = strtol(c, &c1, 0);
        if (c == c1)
	  return FALSE;
	c = c1;
	if (max_lazy_timeout <= 0)
	  return FALSE;
	if (max_lazy_timeout == LONG_MAX)
	  return FALSE;
	WINE_TRACE("read timeout argument: %ld\n", max_lazy_timeout);
	break;
      default: 
        return FALSE;
	break;
    }
    while (*c == ' ') c++;
    if (*c != '\0') return FALSE;
  }

  return TRUE;
}

static void RPCSS_Usage(void)
{
  printf("\nWine RPCSS\n");
  printf("\nsyntax: rpcss [-t timeout]\n\n");
  printf("  -t: rpcss (or the running rpcss process) will\n");
  printf("      execute with at least the specified timeout.\n");
  printf("\n");
}

int main( int argc, char **argv )
{
  /* 
   * We are invoked as a standard executable; we act in a
   * "lazy" manner.  We open up our pipe, and hang around until we have
   * nothing left to do, and then silently terminate.  When we're needed
   * again, rpcrt4.dll.so will invoke us automatically.
   */
       
  if (!RPCSS_ProcessArgs(argc, argv)) {
    RPCSS_Usage();
    return 1;
  }
      
  /* we want to wait for something to happen, and then 
     timeout when no activity occurs. */
  RPCSS_SetLazyTimeRemaining(max_lazy_timeout);

  if (RPCSS_Initialize()) {
    do
      RPCSS_MainLoop();
    while (!RPCSS_Shutdown());
  }

  return 0;
}
