/*
 * RPC server API
 *
 * Copyright 2001 Ove Kåven, TransGaming Technologies
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
 * TODO:
 *  - a whole lot
 */

#include <stdio.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"

#include "rpc.h"

#include "wine/debug.h"

#include "rpc_server.h"
#include "rpc_defs.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

static RpcServerProtseq* protseqs;
static RpcServerInterface* ifs;

static CRITICAL_SECTION server_cs = CRITICAL_SECTION_INIT("RpcServer");
static BOOL std_listen;
static LONG listen_count = -1;
static HANDLE mgr_event, server_thread;

static RpcServerInterface* RPCRT4_find_interface(UUID* object, UUID* if_id)
{
  UUID* MgrType = NULL;
  RpcServerInterface* cif = NULL;
  RPC_STATUS status;

  /* FIXME: object -> MgrType */
  EnterCriticalSection(&server_cs);
  cif = ifs;
  while (cif) {
    if (UuidEqual(if_id, &cif->If->InterfaceId.SyntaxGUID, &status) &&
       UuidEqual(MgrType, &cif->MgrTypeUuid, &status) &&
       (std_listen || (cif->Flags & RPC_IF_AUTOLISTEN))) break;
    cif = cif->Next;
  }
  LeaveCriticalSection(&server_cs);
  return cif;
}

static DWORD CALLBACK RPCRT4_io_thread(LPVOID the_arg)
{
  RpcBinding* bind = (RpcBinding*)the_arg;
  RpcPktHdr hdr;
  DWORD dwRead;
  RPC_MESSAGE msg;
  RpcServerInterface* sif;
  RPC_DISPATCH_FUNCTION func;

  memset(&msg, 0, sizeof(msg));
  msg.Handle = (RPC_BINDING_HANDLE)bind;

  for (;;) {
    /* read packet header */
#ifdef OVERLAPPED_WORKS
    if (!ReadFile(bind->conn, &hdr, sizeof(hdr), &dwRead, &bind->ovl)) {
      DWORD err = GetLastError();
      if (err != ERROR_IO_PENDING) {
        TRACE("connection lost, error=%08lx\n", err);
        break;
      }
      if (!GetOverlappedResult(bind->conn, &bind->ovl, &dwRead, TRUE)) break;
    }
#else
    if (!ReadFile(bind->conn, &hdr, sizeof(hdr), &dwRead, NULL)) {
      TRACE("connection lost, error=%08lx\n", GetLastError());
      break;
    }
#endif
    if (dwRead != sizeof(hdr)) {
      TRACE("protocol error\n");
      break;
    }

    /* read packet body */
    msg.BufferLength = hdr.len;
    msg.Buffer = HeapAlloc(GetProcessHeap(), 0, msg.BufferLength);
#ifdef OVERLAPPED_WORKS
    if (!ReadFile(bind->conn, msg.Buffer, msg.BufferLength, &dwRead, &bind->ovl)) {
      DWORD err = GetLastError();
      if (err != ERROR_IO_PENDING) {
        TRACE("connection lost, error=%08lx\n", err);
        break;
      }
      if (!GetOverlappedResult(bind->conn, &bind->ovl, &dwRead, TRUE)) break;
    }
#else
    if (!ReadFile(bind->conn, msg.Buffer, msg.BufferLength, &dwRead, NULL)) {
      TRACE("connection lost, error=%08lx\n", GetLastError());
      break;
    }
#endif
    if (dwRead != hdr.len) {
      TRACE("protocol error\n");
      break;
    }

    sif = RPCRT4_find_interface(&hdr.object, &hdr.if_id);
    if (sif) {
      msg.RpcInterfaceInformation = sif->If;
      /* associate object with binding (this is a bit of a hack...
       * a new binding should probably be created for each object) */
      RPCRT4_SetBindingObject(bind, &hdr.object);
      /* process packet*/
      switch (hdr.ptype) {
      case PKT_REQUEST:
       /* find dispatch function */
       msg.ProcNum = hdr.opnum;
       if (sif->Flags & RPC_IF_OLE) {
         /* native ole32 always gives us a dispatch table with a single entry
          * (I assume that's a wrapper for IRpcStubBuffer::Invoke) */
         func = *sif->If->DispatchTable->DispatchTable;
       } else {
         if (msg.ProcNum >= sif->If->DispatchTable->DispatchTableCount) {
           ERR("invalid procnum\n");
           func = NULL;
         }
         func = sif->If->DispatchTable->DispatchTable[msg.ProcNum];
       }

       /* dispatch */
       if (func) func(&msg);

       /* prepare response packet */
       hdr.ptype = PKT_RESPONSE;
       break;
      default:
       ERR("unknown packet type\n");
       goto no_reply;
      }

      /* write reply packet */
      hdr.len = msg.BufferLength;
      WriteFile(bind->conn, &hdr, sizeof(hdr), NULL, NULL);
      WriteFile(bind->conn, msg.Buffer, msg.BufferLength, NULL, NULL);

    no_reply:
      /* un-associate object */
      RPCRT4_SetBindingObject(bind, NULL);
      msg.RpcInterfaceInformation = NULL;
    }
    else {
      ERR("got RPC packet to unregistered interface %s\n", debugstr_guid(&hdr.if_id));
    }

    /* clean up */
    HeapFree(GetProcessHeap(), 0, msg.Buffer);
    msg.Buffer = NULL;
  }
  if (msg.Buffer) HeapFree(GetProcessHeap(), 0, msg.Buffer);
  RPCRT4_DestroyBinding(bind);
  return 0;
}

static void RPCRT4_new_client(RpcBinding* bind)
{
  bind->thread = CreateThread(NULL, 0, RPCRT4_io_thread, bind, 0, NULL);
}

static DWORD CALLBACK RPCRT4_server_thread(LPVOID the_arg)
{
  HANDLE m_event = mgr_event, b_handle;
  HANDLE *objs = NULL;
  DWORD count, res;
  RpcServerProtseq* cps;
  RpcBinding* bind;
  RpcBinding* cbind;

  for (;;) {
    EnterCriticalSection(&server_cs);
    /* open and count bindings */
    count = 1;
    cps = protseqs;
    while (cps) {
      bind = cps->bind;
      while (bind) {
       RPCRT4_OpenBinding(bind);
        if (bind->ovl.hEvent) count++;
        bind = bind->Next;
      }
      cps = cps->Next;
    }
    /* make array of bindings */
    objs = HeapReAlloc(GetProcessHeap(), 0, objs, count*sizeof(HANDLE));
    objs[0] = m_event;
    count = 1;
    cps = protseqs;
    while (cps) {
      bind = cps->bind;
      while (bind) {
        if (bind->ovl.hEvent) objs[count++] = bind->ovl.hEvent;
        bind = bind->Next;
      }
      cps = cps->Next;
    }
    LeaveCriticalSection(&server_cs);

    /* start waiting */
    res = WaitForMultipleObjects(count, objs, FALSE, INFINITE);
    if (res == WAIT_OBJECT_0) {
      if (listen_count == -1) break;
    }
    else if (res == WAIT_FAILED) {
      ERR("wait failed\n");
    }
    else {
      b_handle = objs[res - WAIT_OBJECT_0];
      /* find which binding got a RPC */
      EnterCriticalSection(&server_cs);
      bind = NULL;
      cps = protseqs;
      while (cps) {
        bind = cps->bind;
        while (bind) {
          if (bind->ovl.hEvent == b_handle) break;
          bind = bind->Next;
        }
        if (bind) break;
        cps = cps->Next;
      }
      cbind = NULL;
      if (bind) RPCRT4_SpawnBinding(&cbind, bind);
      LeaveCriticalSection(&server_cs);
      if (!bind) {
        ERR("failed to locate binding for handle %p\n", b_handle);
      }
      if (cbind) RPCRT4_new_client(cbind);
    }
  }
  HeapFree(GetProcessHeap(), 0, objs);
  EnterCriticalSection(&server_cs);
  /* close bindings */
  cps = protseqs;
  while (cps) {
    bind = cps->bind;
    while (bind) {
      RPCRT4_CloseBinding(bind);
      bind = bind->Next;
    }
    cps = cps->Next;
  }
  LeaveCriticalSection(&server_cs);
  return 0;
}

static void RPCRT4_start_listen(void)
{
  TRACE("\n");
  if (!InterlockedIncrement(&listen_count)) {
    mgr_event = CreateEventA(NULL, FALSE, FALSE, NULL);
    server_thread = CreateThread(NULL, 0, RPCRT4_server_thread, NULL, 0, NULL);
  }
  else SetEvent(mgr_event);
}

/* not used (WTF?) ---------------------------

static void RPCRT4_stop_listen(void)
{
  HANDLE m_event = mgr_event;
  if (InterlockedDecrement(&listen_count) < 0)
    SetEvent(m_event);
}

--------------------- */

static RPC_STATUS RPCRT4_use_protseq(RpcServerProtseq* ps)
{
  RPCRT4_CreateBindingA(&ps->bind, TRUE, ps->Protseq);
  RPCRT4_CompleteBindingA(ps->bind, NULL, ps->Endpoint, NULL);

  EnterCriticalSection(&server_cs);
  ps->Next = protseqs;
  protseqs = ps;
  LeaveCriticalSection(&server_cs);

  if (listen_count >= 0) SetEvent(mgr_event);

  return RPC_S_OK;
}

/***********************************************************************
 *             RpcServerInqBindings (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcServerInqBindings( RPC_BINDING_VECTOR** BindingVector )
{
  RPC_STATUS status;
  DWORD count;
  RpcServerProtseq* ps;
  RpcBinding* bind;

  if (BindingVector)
    TRACE("(*BindingVector == ^%p)\n", *BindingVector);
  else
    ERR("(BindingVector == ^null!!?)\n");

  EnterCriticalSection(&server_cs);
  /* count bindings */
  count = 0;
  ps = protseqs;
  while (ps) {
    bind = ps->bind;
    while (bind) {
      count++;
      bind = bind->Next;
    }
    ps = ps->Next;
  }
  if (count) {
    /* export bindings */
    *BindingVector = HeapAlloc(GetProcessHeap(), 0,
                              sizeof(RPC_BINDING_VECTOR) +
                              sizeof(RPC_BINDING_HANDLE)*(count-1));
    (*BindingVector)->Count = count;
    count = 0;
    ps = protseqs;
    while (ps) {
      bind = ps->bind;
      while (bind) {
       RPCRT4_ExportBinding((RpcBinding**)&(*BindingVector)->BindingH[count],
                            bind);
       count++;
       bind = bind->Next;
      }
      ps = ps->Next;
    }
    status = RPC_S_OK;
  } else {
    *BindingVector = NULL;
    status = RPC_S_NO_BINDINGS;
  }
  LeaveCriticalSection(&server_cs);
  return status;
}

/***********************************************************************
 *             RpcServerUseProtseqEpA (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcServerUseProtseqEpA( LPSTR Protseq, UINT MaxCalls, LPSTR Endpoint, LPVOID SecurityDescriptor )
{
  RPC_POLICY policy;
  
  TRACE( "(%s,%u,%s,%p)\n", Protseq, MaxCalls, Endpoint, SecurityDescriptor );
  
  /* This should provide the default behaviour */
  policy.Length        = sizeof( policy );
  policy.EndpointFlags = 0;
  policy.NICFlags      = 0;
  
  return RpcServerUseProtseqEpExA( Protseq, MaxCalls, Endpoint, SecurityDescriptor, &policy );
}

/***********************************************************************
 *             RpcServerUseProtseqEpW (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcServerUseProtseqEpW( LPWSTR Protseq, UINT MaxCalls, LPWSTR Endpoint, LPVOID SecurityDescriptor )
{
  RPC_POLICY policy;
  
  TRACE( "(%s,%u,%s,%p)\n", debugstr_w( Protseq ), MaxCalls, debugstr_w( Endpoint ), SecurityDescriptor );
  
  /* This should provide the default behaviour */
  policy.Length        = sizeof( policy );
  policy.EndpointFlags = 0;
  policy.NICFlags      = 0;
  
  return RpcServerUseProtseqEpExW( Protseq, MaxCalls, Endpoint, SecurityDescriptor, &policy );
}

/***********************************************************************
 *             RpcServerUseProtseqEpExA (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcServerUseProtseqEpExA( LPSTR Protseq, UINT MaxCalls, LPSTR Endpoint, LPVOID SecurityDescriptor,
                                            PRPC_POLICY lpPolicy )
{
  RpcServerProtseq* ps;

  TRACE("(%s,%u,%s,%p,{%u,%lu,%lu})\n", debugstr_a( Protseq ), MaxCalls,
       debugstr_a( Endpoint ), SecurityDescriptor,
       lpPolicy->Length, lpPolicy->EndpointFlags, lpPolicy->NICFlags );

  ps = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(RpcServerProtseq));
  ps->MaxCalls = MaxCalls;
  ps->Protseq = RPCRT4_strdupA(Protseq);
  ps->Endpoint = RPCRT4_strdupA(Endpoint);

  return RPCRT4_use_protseq(ps);
}

/***********************************************************************
 *             RpcServerUseProtseqEpExW (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcServerUseProtseqEpExW( LPWSTR Protseq, UINT MaxCalls, LPWSTR Endpoint, LPVOID SecurityDescriptor,
                                            PRPC_POLICY lpPolicy )
{
  RpcServerProtseq* ps;

  TRACE("(%s,%u,%s,%p,{%u,%lu,%lu})\n", debugstr_w( Protseq ), MaxCalls,
       debugstr_w( Endpoint ), SecurityDescriptor,
       lpPolicy->Length, lpPolicy->EndpointFlags, lpPolicy->NICFlags );

  ps = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(RpcServerProtseq));
  ps->MaxCalls = MaxCalls;
  /* FIXME: Did Ove have these next two as RPCRT4_strdupW for a reason? */
  ps->Protseq = RPCRT4_strdupWtoA(Protseq);
  ps->Endpoint = RPCRT4_strdupWtoA(Endpoint);

  return RPCRT4_use_protseq(ps);
}

/***********************************************************************
 *             RpcServerUseProtseqA (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcServerUseProtseqA(LPSTR Protseq, unsigned int MaxCalls, void *SecurityDescriptor)
{
  TRACE("(Protseq == %s, MaxCalls == %d, SecurityDescriptor == ^%p)", debugstr_a(Protseq), MaxCalls, SecurityDescriptor);
  return RpcServerUseProtseqEpA(Protseq, MaxCalls, NULL, SecurityDescriptor);
}

/***********************************************************************
 *             RpcServerUseProtseqW (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcServerUseProtseqW(LPWSTR Protseq, unsigned int MaxCalls, void *SecurityDescriptor)
{
  TRACE("Protseq == %s, MaxCalls == %d, SecurityDescriptor == ^%p)", debugstr_w(Protseq), MaxCalls, SecurityDescriptor);
  return RpcServerUseProtseqEpW(Protseq, MaxCalls, NULL, SecurityDescriptor);
}

/***********************************************************************
 *             RpcServerRegisterIf (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcServerRegisterIf( RPC_IF_HANDLE IfSpec, UUID* MgrTypeUuid, RPC_MGR_EPV* MgrEpv )
{
  TRACE("(%p,%s,%p)\n", IfSpec, debugstr_guid(MgrTypeUuid), MgrEpv);
  return RpcServerRegisterIf2( IfSpec, MgrTypeUuid, MgrEpv, 0, RPC_C_LISTEN_MAX_CALLS_DEFAULT, (UINT)-1, NULL );
}

/***********************************************************************
 *             RpcServerRegisterIfEx (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcServerRegisterIfEx( RPC_IF_HANDLE IfSpec, UUID* MgrTypeUuid, RPC_MGR_EPV* MgrEpv,
                       UINT Flags, UINT MaxCalls, RPC_IF_CALLBACK_FN* IfCallbackFn )
{
  TRACE("(%p,%s,%p,%u,%u,%p)\n", IfSpec, debugstr_guid(MgrTypeUuid), MgrEpv, Flags, MaxCalls, IfCallbackFn);
  return RpcServerRegisterIf2( IfSpec, MgrTypeUuid, MgrEpv, Flags, MaxCalls, (UINT)-1, IfCallbackFn );
}

/***********************************************************************
 *             RpcServerRegisterIf2 (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcServerRegisterIf2( RPC_IF_HANDLE IfSpec, UUID* MgrTypeUuid, RPC_MGR_EPV* MgrEpv,
                      UINT Flags, UINT MaxCalls, UINT MaxRpcSize, RPC_IF_CALLBACK_FN* IfCallbackFn )
{
  PRPC_SERVER_INTERFACE If = (PRPC_SERVER_INTERFACE)IfSpec;
  RpcServerInterface* sif;
  int i;

  TRACE("(%p,%s,%p,%u,%u,%u,%p)\n", IfSpec, debugstr_guid(MgrTypeUuid), MgrEpv, Flags, MaxCalls,
         MaxRpcSize, IfCallbackFn);
  TRACE(" interface id: %s %d.%d\n", debugstr_guid(&If->InterfaceId.SyntaxGUID),
                                     If->InterfaceId.SyntaxVersion.MajorVersion,
                                     If->InterfaceId.SyntaxVersion.MinorVersion);
  TRACE(" transfer syntax: %s %d.%d\n", debugstr_guid(&If->TransferSyntax.SyntaxGUID),
                                        If->TransferSyntax.SyntaxVersion.MajorVersion,
                                        If->TransferSyntax.SyntaxVersion.MinorVersion);
  TRACE(" dispatch table: %p\n", If->DispatchTable);
  if (If->DispatchTable) {
    TRACE("  dispatch table count: %d\n", If->DispatchTable->DispatchTableCount);
    for (i=0; i<If->DispatchTable->DispatchTableCount; i++) {
      TRACE("   entry %d: %p\n", i, If->DispatchTable->DispatchTable[i]);
    }
    TRACE("  reserved: %ld\n", If->DispatchTable->Reserved);
  }
  TRACE(" protseq endpoint count: %d\n", If->RpcProtseqEndpointCount);
  TRACE(" default manager epv: %p\n", If->DefaultManagerEpv);
  TRACE(" interpreter info: %p\n", If->InterpreterInfo);
  TRACE(" flags: %08x\n", If->Flags);

  sif = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(RpcServerInterface));
  sif->If           = If;
  if (MgrTypeUuid)
    memcpy(&sif->MgrTypeUuid, MgrTypeUuid, sizeof(UUID));
  else
    memset(&sif->MgrTypeUuid, 0, sizeof(UUID));
  sif->MgrEpv       = MgrEpv;
  sif->Flags        = Flags;
  sif->MaxCalls     = MaxCalls;
  sif->MaxRpcSize   = MaxRpcSize;
  sif->IfCallbackFn = IfCallbackFn;

  EnterCriticalSection(&server_cs);
  sif->Next = ifs;
  ifs = sif;
  LeaveCriticalSection(&server_cs);

  if (sif->Flags & RPC_IF_AUTOLISTEN) {
    /* well, start listening, I think... */
    RPCRT4_start_listen();
  }

  return RPC_S_OK;
}

/***********************************************************************
 *             RpcServerRegisterAuthInfoA (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcServerRegisterAuthInfoA( LPSTR ServerPrincName, ULONG AuthnSvc, RPC_AUTH_KEY_RETRIEVAL_FN GetKeyFn,
                            LPVOID Arg )
{
  FIXME( "(%s,%lu,%p,%p): stub\n", ServerPrincName, AuthnSvc, GetKeyFn, Arg );
  
  return RPC_S_UNKNOWN_AUTHN_SERVICE; /* We don't know any authentication services */
}

/***********************************************************************
 *             RpcServerRegisterAuthInfoW (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcServerRegisterAuthInfoW( LPWSTR ServerPrincName, ULONG AuthnSvc, RPC_AUTH_KEY_RETRIEVAL_FN GetKeyFn,
                            LPVOID Arg )
{
  FIXME( "(%s,%lu,%p,%p): stub\n", debugstr_w( ServerPrincName ), AuthnSvc, GetKeyFn, Arg );
  
  return RPC_S_UNKNOWN_AUTHN_SERVICE; /* We don't know any authentication services */
}

/***********************************************************************
 *             RpcServerListen (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcServerListen( UINT MinimumCallThreads, UINT MaxCalls, UINT DontWait )
{
  TRACE("(%u,%u,%u)\n", MinimumCallThreads, MaxCalls, DontWait);

  if (std_listen)
    return RPC_S_ALREADY_LISTENING;

  if (!protseqs)
    return RPC_S_NO_PROTSEQS_REGISTERED;

  std_listen = TRUE;
  RPCRT4_start_listen();

  if (DontWait) return RPC_S_OK;

  return RpcMgmtWaitServerListen();
}

/***********************************************************************
 *             RpcMgmtServerWaitListen (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcMgmtWaitServerListen( void )
{
  RPC_STATUS rslt = RPC_S_OK;

  TRACE("\n");
  if (!std_listen)
    if ( (rslt = RpcServerListen(1, 0, TRUE)) != RPC_S_OK )
      return rslt;
  
  while (std_listen) {
    WaitForSingleObject(mgr_event, 1000);
  }

  return rslt;
}

/***********************************************************************
 *             I_RpcServerStartListening (RPCRT4.@)
 */
RPC_STATUS WINAPI I_RpcServerStartListening( void* hWnd )
{
  FIXME( "(%p): stub\n", hWnd );

  return RPC_S_OK;
}

/***********************************************************************
 *             I_RpcServerStopListening (RPCRT4.@)
 */
RPC_STATUS WINAPI I_RpcServerStopListening( void )
{
  FIXME( "(): stub\n" );

  return RPC_S_OK;
}

/***********************************************************************
 *             I_RpcWindowProc (RPCRT4.@)
 */
UINT WINAPI I_RpcWindowProc( void* hWnd, UINT Message, UINT wParam, ULONG lParam )
{
  FIXME( "(%p,%08x,%08x,%08lx): stub\n", hWnd, Message, wParam, lParam );

  return 0;
}
