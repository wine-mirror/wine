/*
 * RPC transport layer
 *
 * Copyright 2001 Ove Kåven, TransGaming Technologies
 * Copyright 2003 Mike Hearn
 * Copyright 2004 Filip Navara
 * Copyright 2006 Mike McCormack
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
 */

#include "config.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winerror.h"
#include "winreg.h"
#include "winternl.h"
#include "wine/unicode.h"

#include "rpc.h"
#include "rpcndr.h"

#include "wine/debug.h"

#include "rpc_binding.h"
#include "rpc_message.h"

WINE_DEFAULT_DEBUG_CHANNEL(rpc);

/**** ncacn_np support ****/

typedef struct _RpcConnection_np
{
  RpcConnection common;
  HANDLE pipe, thread;
  OVERLAPPED ovl;
} RpcConnection_np;

static RpcConnection *rpcrt4_conn_np_alloc(void)
{
  return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(RpcConnection_np));
}

static RPC_STATUS rpcrt4_connect_pipe(RpcConnection *Connection, LPCSTR pname)
{
  RpcConnection_np *npc = (RpcConnection_np *) Connection;
  TRACE("listening on %s\n", pname);

  npc->pipe = CreateNamedPipeA(pname, PIPE_ACCESS_DUPLEX,
                               PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE,
                               PIPE_UNLIMITED_INSTANCES,
                               RPC_MAX_PACKET_SIZE, RPC_MAX_PACKET_SIZE, 5000, NULL);
  memset(&npc->ovl, 0, sizeof(npc->ovl));
  npc->ovl.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
  if (ConnectNamedPipe(npc->pipe, &npc->ovl))
     return RPC_S_OK;

  WARN("Couldn't ConnectNamedPipe (error was %ld)\n", GetLastError());
  if (GetLastError() == ERROR_PIPE_CONNECTED) {
    SetEvent(npc->ovl.hEvent);
    return RPC_S_OK;
  }
  if (GetLastError() == ERROR_IO_PENDING) {
    /* FIXME: looks like we need to GetOverlappedResult here? */
    return RPC_S_OK;
  }
  return RPC_S_SERVER_UNAVAILABLE;
}

static RPC_STATUS rpcrt4_open_pipe(RpcConnection *Connection, LPCSTR pname, BOOL wait)
{
  RpcConnection_np *npc = (RpcConnection_np *) Connection;
  HANDLE pipe;
  DWORD err, dwMode;

  TRACE("connecting to %s\n", pname);

  while (TRUE) {
    pipe = CreateFileA(pname, GENERIC_READ|GENERIC_WRITE, 0, NULL,
                       OPEN_EXISTING, 0, 0);
    if (pipe != INVALID_HANDLE_VALUE) break;
    err = GetLastError();
    if (err == ERROR_PIPE_BUSY) {
      TRACE("connection failed, error=%lx\n", err);
      return RPC_S_SERVER_TOO_BUSY;
    }
    if (!wait)
      return RPC_S_SERVER_UNAVAILABLE;
    if (!WaitNamedPipeA(pname, NMPWAIT_WAIT_FOREVER)) {
      err = GetLastError();
      WARN("connection failed, error=%lx\n", err);
      return RPC_S_SERVER_UNAVAILABLE;
    }
  }

  /* success */
  memset(&npc->ovl, 0, sizeof(npc->ovl));
  /* pipe is connected; change to message-read mode. */
  dwMode = PIPE_READMODE_MESSAGE;
  SetNamedPipeHandleState(pipe, &dwMode, NULL, NULL);
  npc->ovl.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
  npc->pipe = pipe;

  return RPC_S_OK;
}

static RPC_STATUS rpcrt4_ncalrpc_open(RpcConnection* Connection)
{
  RpcConnection_np *npc = (RpcConnection_np *) Connection;
  static LPCSTR prefix = "\\\\.\\pipe\\lrpc\\";
  RPC_STATUS r;
  LPSTR pname;

  /* already connected? */
  if (npc->pipe)
    return RPC_S_OK;

  /* protseq=ncalrpc: supposed to use NT LPC ports,
   * but we'll implement it with named pipes for now */
  pname = HeapAlloc(GetProcessHeap(), 0, strlen(prefix) + strlen(Connection->Endpoint) + 1);
  strcat(strcpy(pname, prefix), Connection->Endpoint);

  if (Connection->server)
    r = rpcrt4_connect_pipe(Connection, pname);
  else
    r = rpcrt4_open_pipe(Connection, pname, TRUE);
  HeapFree(GetProcessHeap(), 0, pname);

  return r;
}

static RPC_STATUS rpcrt4_ncacn_np_open(RpcConnection* Connection)
{
  RpcConnection_np *npc = (RpcConnection_np *) Connection;
  static LPCSTR prefix = "\\\\.";
  RPC_STATUS r;
  LPSTR pname;

  /* already connected? */
  if (npc->pipe)
    return RPC_S_OK;

  /* protseq=ncacn_np: named pipes */
  pname = HeapAlloc(GetProcessHeap(), 0, strlen(prefix) + strlen(Connection->Endpoint) + 1);
  strcat(strcpy(pname, prefix), Connection->Endpoint);
  if (Connection->server)
    r = rpcrt4_connect_pipe(Connection, pname);
  else
    r = rpcrt4_open_pipe(Connection, pname, FALSE);
  HeapFree(GetProcessHeap(), 0, pname);

  return r;
}

static HANDLE rpcrt4_conn_np_get_connect_event(RpcConnection *Connection)
{
  RpcConnection_np *npc = (RpcConnection_np *) Connection;
  return npc->ovl.hEvent;
}

static RPC_STATUS rpcrt4_conn_np_handoff(RpcConnection *old_conn, RpcConnection *new_conn)
{
  RpcConnection_np *old_npc = (RpcConnection_np *) old_conn;
  RpcConnection_np *new_npc = (RpcConnection_np *) new_conn;
  /* because of the way named pipes work, we'll transfer the connected pipe
   * to the child, then reopen the server binding to continue listening */

  new_npc->pipe = old_npc->pipe;
  new_npc->ovl = old_npc->ovl;
  old_npc->pipe = 0;
  memset(&old_npc->ovl, 0, sizeof(old_npc->ovl));
  return RPCRT4_OpenConnection(old_conn);
}

static int rpcrt4_conn_np_read(RpcConnection *Connection,
                        void *buffer, unsigned int count)
{
  RpcConnection_np *npc = (RpcConnection_np *) Connection;
  DWORD dwRead = 0;
  if (!ReadFile(npc->pipe, buffer, count, &dwRead, NULL) &&
      (GetLastError() != ERROR_MORE_DATA))
    return -1;
  return dwRead;
}

static int rpcrt4_conn_np_write(RpcConnection *Connection,
                             const void *buffer, unsigned int count)
{
  RpcConnection_np *npc = (RpcConnection_np *) Connection;
  DWORD dwWritten = 0;
  if (!WriteFile(npc->pipe, buffer, count, &dwWritten, NULL))
    return -1;
  return dwWritten;
}

static int rpcrt4_conn_np_close(RpcConnection *Connection)
{
  RpcConnection_np *npc = (RpcConnection_np *) Connection;
  if (npc->pipe) {
    FlushFileBuffers(npc->pipe);
    CloseHandle(npc->pipe);
    npc->pipe = 0;
  }
  if (npc->ovl.hEvent) {
    CloseHandle(npc->ovl.hEvent);
    npc->ovl.hEvent = 0;
  }
  return 0;
}

/**** ncacn_ip_tcp support ****/

typedef struct _RpcConnection_tcp
{
  RpcConnection common;
  int sock;
} RpcConnection_tcp;

static RpcConnection *rpcrt4_conn_tcp_alloc(void)
{
  RpcConnection_tcp *tcpc;
  tcpc = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(RpcConnection_tcp));
  tcpc->sock = -1;
  return &tcpc->common;
}

static RPC_STATUS rpcrt4_ncacn_ip_tcp_open(RpcConnection* Connection)
{
  RpcConnection_tcp *tcpc = (RpcConnection_tcp *) Connection;
  int sock;
  int ret;
  struct addrinfo *ai;
  struct addrinfo *ai_cur;
  struct addrinfo hints;

  TRACE("(%s, %s)\n", Connection->NetworkAddr, Connection->Endpoint);

  if (Connection->server)
  {
    ERR("ncacn_ip_tcp servers not supported yet\n");
    return RPC_S_SERVER_UNAVAILABLE;
  }

  if (tcpc->sock != -1)
    return RPC_S_OK;

  hints.ai_flags          = 0;
  hints.ai_family         = PF_UNSPEC;
  hints.ai_socktype       = SOCK_STREAM;
  hints.ai_protocol       = IPPROTO_TCP;
  hints.ai_addrlen        = 0;
  hints.ai_addr           = NULL;
  hints.ai_canonname      = NULL;
  hints.ai_next           = NULL;

  ret = getaddrinfo(Connection->NetworkAddr, Connection->Endpoint, &hints, &ai);
  if (ret < 0)
  {
    ERR("getaddrinfo failed with %d\n", ret);
    return RPC_S_SERVER_UNAVAILABLE;
  }

  for (ai_cur = ai; ai_cur; ai_cur = ai->ai_next)
  {
    if (TRACE_ON(rpc))
    {
      char host[256];
      char service[256];
      getnameinfo(ai_cur->ai_addr, ai_cur->ai_addrlen,
        host, sizeof(host), service, sizeof(service),
        NI_NUMERICHOST | NI_NUMERICSERV);
      TRACE("trying %s:%s\n", host, service);
    }

    sock = socket(ai_cur->ai_family, ai_cur->ai_socktype, ai_cur->ai_protocol);
    if (sock < 0)
    {
      WARN("socket() failed\n");
      continue;
    }

    if (0>connect(sock, ai_cur->ai_addr, ai_cur->ai_addrlen))
    {
      WARN("connect() failed\n");
      close(sock);
      continue;
    }

    tcpc->sock = sock;

    freeaddrinfo(ai);
    TRACE("connected\n");
    return RPC_S_OK;
  }

  freeaddrinfo(ai);
  ERR("couldn't connect to %s:%s\n", Connection->NetworkAddr, Connection->Endpoint);
  return RPC_S_SERVER_UNAVAILABLE;
}

static HANDLE rpcrt4_conn_tcp_get_wait_handle(RpcConnection *Connection)
{
  assert(0);
  return 0;
}

static RPC_STATUS rpcrt4_conn_tcp_handoff(RpcConnection *old_conn, RpcConnection *new_conn)
{
  assert(0);
  return RPC_S_SERVER_UNAVAILABLE;
}

static int rpcrt4_conn_tcp_read(RpcConnection *Connection,
                                void *buffer, unsigned int count)
{
  RpcConnection_tcp *tcpc = (RpcConnection_tcp *) Connection;
  int r = recv(tcpc->sock, buffer, count, MSG_WAITALL);
  TRACE("%d %p %u -> %d\n", tcpc->sock, buffer, count, r);
  return r;
}

static int rpcrt4_conn_tcp_write(RpcConnection *Connection,
                                 const void *buffer, unsigned int count)
{
  RpcConnection_tcp *tcpc = (RpcConnection_tcp *) Connection;
  int r = write(tcpc->sock, buffer, count);
  TRACE("%d %p %u -> %d\n", tcpc->sock, buffer, count, r);
  return r;
}

static int rpcrt4_conn_tcp_close(RpcConnection *Connection)
{
  RpcConnection_tcp *tcpc = (RpcConnection_tcp *) Connection;

  TRACE("%d\n", tcpc->sock);
  if (tcpc->sock != -1)
    close(tcpc->sock);
  tcpc->sock = -1;
  return 0;
}

struct protseq_ops protseq_list[] = {
  { "ncacn_np",
    rpcrt4_conn_np_alloc,
    rpcrt4_ncacn_np_open,
    rpcrt4_conn_np_get_connect_event,
    rpcrt4_conn_np_handoff,
    rpcrt4_conn_np_read,
    rpcrt4_conn_np_write,
    rpcrt4_conn_np_close,
  },
  { "ncalrpc",
    rpcrt4_conn_np_alloc,
    rpcrt4_ncalrpc_open,
    rpcrt4_conn_np_get_connect_event,
    rpcrt4_conn_np_handoff,
    rpcrt4_conn_np_read,
    rpcrt4_conn_np_write,
    rpcrt4_conn_np_close,
  },
  { "ncacn_ip_tcp",
    rpcrt4_conn_tcp_alloc,
    rpcrt4_ncacn_ip_tcp_open,
    rpcrt4_conn_tcp_get_wait_handle,
    rpcrt4_conn_tcp_handoff,
    rpcrt4_conn_tcp_read,
    rpcrt4_conn_tcp_write,
    rpcrt4_conn_tcp_close,
  }
};

#define MAX_PROTSEQ (sizeof protseq_list / sizeof protseq_list[0])

static struct protseq_ops *rpcrt4_get_protseq_ops(const char *protseq)
{
  int i;
  for(i=0; i<MAX_PROTSEQ; i++)
    if (!strcmp(protseq_list[i].name, protseq))
      return &protseq_list[i];
  return NULL;
}

/**** interface to rest of code ****/

RPC_STATUS RPCRT4_OpenConnection(RpcConnection* Connection)
{
  TRACE("(Connection == ^%p)\n", Connection);

  return Connection->ops->open_connection(Connection);
}

RPC_STATUS RPCRT4_CloseConnection(RpcConnection* Connection)
{
  TRACE("(Connection == ^%p)\n", Connection);
  rpcrt4_conn_close(Connection);
  return RPC_S_OK;
}

RPC_STATUS RPCRT4_CreateConnection(RpcConnection** Connection, BOOL server,
    LPCSTR Protseq, LPCSTR NetworkAddr, LPCSTR Endpoint,
    LPCSTR NetworkOptions, RpcAuthInfo* AuthInfo, RpcBinding* Binding)
{
  struct protseq_ops *ops;
  RpcConnection* NewConnection;

  ops = rpcrt4_get_protseq_ops(Protseq);
  if (!ops)
    return RPC_S_PROTSEQ_NOT_SUPPORTED;

  NewConnection = ops->alloc();
  NewConnection->server = server;
  NewConnection->ops = ops;
  NewConnection->NetworkAddr = RPCRT4_strdupA(NetworkAddr);
  NewConnection->Endpoint = RPCRT4_strdupA(Endpoint);
  NewConnection->Used = Binding;
  NewConnection->MaxTransmissionSize = RPC_MAX_PACKET_SIZE;
  NewConnection->NextCallId = 1;
  if (AuthInfo) RpcAuthInfo_AddRef(AuthInfo);
  NewConnection->AuthInfo = AuthInfo;

  TRACE("connection: %p\n", NewConnection);
  *Connection = NewConnection;

  return RPC_S_OK;
}

RPC_STATUS RPCRT4_SpawnConnection(RpcConnection** Connection, RpcConnection* OldConnection)
{
  RPC_STATUS err;

  err = RPCRT4_CreateConnection(Connection, OldConnection->server,
                                rpcrt4_conn_get_name(OldConnection),
                                OldConnection->NetworkAddr,
                                OldConnection->Endpoint, NULL,
                                OldConnection->AuthInfo, NULL);
  if (err == RPC_S_OK)
    rpcrt4_conn_handoff(OldConnection, *Connection);
  return err;
}

RPC_STATUS RPCRT4_DestroyConnection(RpcConnection* Connection)
{
  TRACE("connection: %p\n", Connection);

  RPCRT4_CloseConnection(Connection);
  RPCRT4_strfree(Connection->Endpoint);
  RPCRT4_strfree(Connection->NetworkAddr);
  if (Connection->AuthInfo) RpcAuthInfo_Release(Connection->AuthInfo);
  HeapFree(GetProcessHeap(), 0, Connection);
  return RPC_S_OK;
}

/***********************************************************************
 *             RpcNetworkIsProtseqValidW (RPCRT4.@)
 *
 * Checks if the given protocol sequence is known by the RPC system.
 * If it is, returns RPC_S_OK, otherwise RPC_S_PROTSEQ_NOT_SUPPORTED.
 *
 */
RPC_STATUS WINAPI RpcNetworkIsProtseqValidW(LPWSTR protseq)
{
  char ps[0x10];

  WideCharToMultiByte(CP_ACP, 0, protseq, -1,
                      ps, sizeof ps, NULL, NULL);
  if (rpcrt4_get_protseq_ops(ps))
    return RPC_S_OK;

  FIXME("Unknown protseq %s\n", debugstr_w(protseq));

  return RPC_S_INVALID_RPC_PROTSEQ;
}

/***********************************************************************
 *             RpcNetworkIsProtseqValidA (RPCRT4.@)
 */
RPC_STATUS WINAPI RpcNetworkIsProtseqValidA(unsigned char *protseq)
{
  UNICODE_STRING protseqW;

  if (RtlCreateUnicodeStringFromAsciiz(&protseqW, (char*)protseq))
  {
    RPC_STATUS ret = RpcNetworkIsProtseqValidW(protseqW.Buffer);
    RtlFreeUnicodeString(&protseqW);
    return ret;
  }
  return RPC_S_OUT_OF_MEMORY;
}
