/*
 * RPC binding API
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
 */

#ifndef __WINE_RPC_BINDING_H
#define __WINE_RPC_BINDING_H

#include "wine/rpcss_shared.h"

/* don't know what MS's structure looks like */
typedef struct _RpcBinding
{
  DWORD refs;
  struct _RpcBinding* Next; /* for rpc server */
  BOOL server;
  UUID ObjectUuid;
  LPSTR Protseq;
  LPSTR NetworkAddr;
  LPSTR Endpoint;
  RPC_BLOCKING_FN BlockingFn;
  ULONG ServerTid;
  HANDLE conn, thread;
  OVERLAPPED ovl;
} RpcBinding;

LPSTR RPCRT4_strndupA(LPSTR src, INT len);
LPWSTR RPCRT4_strndupW(LPWSTR src, INT len);
LPSTR RPCRT4_strdupWtoA(LPWSTR src);
LPWSTR RPCRT4_strdupAtoW(LPSTR src);
void RPCRT4_strfree(LPSTR src);

#define RPCRT4_strdupA(x) RPCRT4_strndupA((x),-1)
#define RPCRT4_strdupW(x) RPCRT4_strndupW((x),-1)

RPC_STATUS RPCRT4_CreateBindingA(RpcBinding** Binding, BOOL server, LPSTR Protseq);
RPC_STATUS RPCRT4_CreateBindingW(RpcBinding** Binding, BOOL server, LPWSTR Protseq);
RPC_STATUS RPCRT4_CompleteBindingA(RpcBinding* Binding, LPSTR NetworkAddr,  LPSTR Endpoint,  LPSTR NetworkOptions);
RPC_STATUS RPCRT4_CompleteBindingW(RpcBinding* Binding, LPWSTR NetworkAddr, LPWSTR Endpoint, LPWSTR NetworkOptions);
RPC_STATUS RPCRT4_ResolveBinding(RpcBinding* Binding, LPSTR Endpoint);
RPC_STATUS RPCRT4_SetBindingObject(RpcBinding* Binding, UUID* ObjectUuid);
RPC_STATUS RPCRT4_SpawnBinding(RpcBinding** Binding, RpcBinding* OldBinding);
RPC_STATUS RPCRT4_ExportBinding(RpcBinding** Binding, RpcBinding* OldBinding);
RPC_STATUS RPCRT4_DestroyBinding(RpcBinding* Binding);
RPC_STATUS RPCRT4_OpenBinding(RpcBinding* Binding);
RPC_STATUS RPCRT4_CloseBinding(RpcBinding* Binding);
BOOL RPCRT4_RPCSSOnDemandCall(PRPCSS_NP_MESSAGE msg, char *vardata_payload, PRPCSS_NP_REPLY reply);
HANDLE RPCRT4_GetMasterMutex(void);
HANDLE RPCRT4_RpcssNPConnect(void);

#endif
