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
#include "security.h"


typedef struct _RpcAuthInfo
{
  LONG refs;

  unsigned long AuthnLevel;
  unsigned long AuthnSvc;
  CredHandle cred;
  TimeStamp exp;
} RpcAuthInfo;

struct protseq_ops;

typedef struct _RpcConnection
{
  struct _RpcConnection* Next;
  struct _RpcBinding* Used;
  BOOL server;
  LPSTR NetworkAddr;
  LPSTR Endpoint;
  struct protseq_ops *ops;
  USHORT MaxTransmissionSize;
  /* The active interface bound to server. */
  RPC_SYNTAX_IDENTIFIER ActiveInterface;
  USHORT NextCallId;

  /* authentication */
  CtxtHandle ctx;
  TimeStamp exp;
  ULONG attr;
  RpcAuthInfo *AuthInfo;
} RpcConnection;

struct protseq_ops {
  char *name;
  RpcConnection *(*alloc)(void);
  RPC_STATUS (*open_connection)(RpcConnection *conn);
  HANDLE (*get_connect_wait_handle)(RpcConnection *conn);
  RPC_STATUS (*handoff)(RpcConnection *old_conn, RpcConnection *new_conn);
  int (*read)(RpcConnection *conn, void *buffer, unsigned int len);
  int (*write)(RpcConnection *conn, const void *buffer, unsigned int len);
  int (*close)(RpcConnection *conn);
};

/* don't know what MS's structure looks like */
typedef struct _RpcBinding
{
  LONG refs;
  struct _RpcBinding* Next;
  BOOL server;
  UUID ObjectUuid;
  LPSTR Protseq;
  LPSTR NetworkAddr;
  LPSTR Endpoint;
  RPC_BLOCKING_FN BlockingFn;
  ULONG ServerTid;
  RpcConnection* FromConn;

  /* authentication */
  RpcAuthInfo *AuthInfo;
} RpcBinding;

LPSTR RPCRT4_strndupA(LPCSTR src, INT len);
LPWSTR RPCRT4_strndupW(LPWSTR src, INT len);
LPSTR RPCRT4_strdupWtoA(LPWSTR src);
LPWSTR RPCRT4_strdupAtoW(LPSTR src);
void RPCRT4_strfree(LPSTR src);

#define RPCRT4_strdupA(x) RPCRT4_strndupA((x),-1)
#define RPCRT4_strdupW(x) RPCRT4_strndupW((x),-1)

ULONG RpcAuthInfo_AddRef(RpcAuthInfo *AuthInfo);
ULONG RpcAuthInfo_Release(RpcAuthInfo *AuthInfo);

RPC_STATUS RPCRT4_CreateConnection(RpcConnection** Connection, BOOL server, LPCSTR Protseq, LPCSTR NetworkAddr, LPCSTR Endpoint, LPCSTR NetworkOptions, RpcAuthInfo* AuthInfo, RpcBinding* Binding);
RPC_STATUS RPCRT4_DestroyConnection(RpcConnection* Connection);
RPC_STATUS RPCRT4_OpenConnection(RpcConnection* Connection);
RPC_STATUS RPCRT4_CloseConnection(RpcConnection* Connection);
RPC_STATUS RPCRT4_SpawnConnection(RpcConnection** Connection, RpcConnection* OldConnection);

RPC_STATUS RPCRT4_CreateBindingA(RpcBinding** Binding, BOOL server, LPSTR Protseq);
RPC_STATUS RPCRT4_CreateBindingW(RpcBinding** Binding, BOOL server, LPWSTR Protseq);
RPC_STATUS RPCRT4_CompleteBindingA(RpcBinding* Binding, LPSTR NetworkAddr,  LPSTR Endpoint,  LPSTR NetworkOptions);
RPC_STATUS RPCRT4_CompleteBindingW(RpcBinding* Binding, LPWSTR NetworkAddr, LPWSTR Endpoint, LPWSTR NetworkOptions);
RPC_STATUS RPCRT4_ResolveBinding(RpcBinding* Binding, LPSTR Endpoint);
RPC_STATUS RPCRT4_SetBindingObject(RpcBinding* Binding, UUID* ObjectUuid);
RPC_STATUS RPCRT4_MakeBinding(RpcBinding** Binding, RpcConnection* Connection);
RPC_STATUS RPCRT4_ExportBinding(RpcBinding** Binding, RpcBinding* OldBinding);
RPC_STATUS RPCRT4_DestroyBinding(RpcBinding* Binding);
RPC_STATUS RPCRT4_OpenBinding(RpcBinding* Binding, RpcConnection** Connection, PRPC_SYNTAX_IDENTIFIER TransferSyntax, PRPC_SYNTAX_IDENTIFIER InterfaceId);
RPC_STATUS RPCRT4_CloseBinding(RpcBinding* Binding, RpcConnection* Connection);
BOOL RPCRT4_RPCSSOnDemandCall(PRPCSS_NP_MESSAGE msg, char *vardata_payload, PRPCSS_NP_REPLY reply);
HANDLE RPCRT4_GetMasterMutex(void);
HANDLE RPCRT4_RpcssNPConnect(void);

static inline const char *rpcrt4_conn_get_name(RpcConnection *Connection)
{
  return Connection->ops->name;
}

static inline int rpcrt4_conn_read(RpcConnection *Connection,
                     void *buffer, unsigned int len)
{
  return Connection->ops->read(Connection, buffer, len);
}

static inline int rpcrt4_conn_write(RpcConnection *Connection,
                     const void *buffer, unsigned int len)
{
  return Connection->ops->write(Connection, buffer, len);
}

static inline int rpcrt4_conn_close(RpcConnection *Connection)
{
  return Connection->ops->close(Connection);
}

static inline HANDLE rpcrt4_conn_get_wait_object(RpcConnection *Connection)
{
  return Connection->ops->get_connect_wait_handle(Connection);
}

static inline RPC_STATUS rpcrt4_conn_handoff(RpcConnection *old_conn, RpcConnection *new_conn)
{
  return old_conn->ops->handoff(old_conn, new_conn);
}

#endif
