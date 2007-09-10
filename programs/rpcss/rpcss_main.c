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
#include "irot.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

static HANDLE master_mutex;
static HANDLE exit_event;

extern HANDLE __wine_make_process_system(void);

HANDLE RPCSS_GetMasterMutex(void)
{
  return master_mutex;
}

static BOOL RPCSS_work(HANDLE exit_event)
{
  return RPCSS_NPDoWork(exit_event);
}

static BOOL RPCSS_Initialize(void)
{
  static unsigned short irot_protseq[] = IROT_PROTSEQ;
  static unsigned short irot_endpoint[] = IROT_ENDPOINT;
  RPC_STATUS status;

  WINE_TRACE("\n");

  exit_event = __wine_make_process_system();

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

  status = RpcServerUseProtseqEpW(irot_protseq, RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
                                  irot_endpoint, NULL);
  if (status == RPC_S_OK)
      status = RpcServerRegisterIf(Irot_v0_2_s_ifspec, NULL, NULL);
  if (status == RPC_S_OK)
      status = RpcServerListen(1, RPC_C_LISTEN_MAX_CALLS_DEFAULT, TRUE);
  else
      RpcServerUnregisterIf(Irot_v0_2_s_ifspec, NULL, FALSE);

  return status == RPC_S_OK;
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

  RpcMgmtStopServerListening(NULL);
  RpcServerUnregisterIf(Irot_v0_2_s_ifspec, NULL, TRUE);

  CloseHandle(exit_event);

  return TRUE;
}

static void RPCSS_MainLoop(void)
{
  WINE_TRACE("\n");

  while ( RPCSS_work(exit_event) )
      ;
}

int main( int argc, char **argv )
{
  /* 
   * We are invoked as a standard executable; we act in a
   * "lazy" manner.  We open up our pipe, and hang around until we all
   * user processes exit, and then silently terminate.
   */

  if (RPCSS_Initialize()) {
    RPCSS_MainLoop();
    RPCSS_Shutdown();
  }

  return 0;
}
