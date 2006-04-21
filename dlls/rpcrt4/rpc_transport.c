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

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

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

static struct protseq_ops *rpcrt4_get_protseq_ops(const char *protseq);

static RPC_STATUS rpcrt4_connect_pipe(RpcConnection *Connection, LPCSTR pname)
{
  TRACE("listening on %s\n", pname);

  Connection->conn = CreateNamedPipeA(pname, PIPE_ACCESS_DUPLEX,
                                      PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE,
                                      PIPE_UNLIMITED_INSTANCES,
                                      RPC_MAX_PACKET_SIZE, RPC_MAX_PACKET_SIZE, 5000, NULL);
  memset(&Connection->ovl, 0, sizeof(Connection->ovl));
  Connection->ovl.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
  if (ConnectNamedPipe(Connection->conn, &Connection->ovl))
     return RPC_S_OK;

  WARN("Couldn't ConnectNamedPipe (error was %ld)\n", GetLastError());
  if (GetLastError() == ERROR_PIPE_CONNECTED) {
    SetEvent(Connection->ovl.hEvent);
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
  HANDLE conn;
  DWORD err, dwMode;

  TRACE("connecting to %s\n", pname);

  while (TRUE) {
    conn = CreateFileA(pname, GENERIC_READ|GENERIC_WRITE, 0, NULL,
                       OPEN_EXISTING, 0, 0);
    if (conn != INVALID_HANDLE_VALUE) break;
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
  memset(&Connection->ovl, 0, sizeof(Connection->ovl));
  /* pipe is connected; change to message-read mode. */
  dwMode = PIPE_READMODE_MESSAGE;
  SetNamedPipeHandleState(conn, &dwMode, NULL, NULL);
  Connection->ovl.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
  Connection->conn = conn;

  return RPC_S_OK;
}

static RPC_STATUS rpcrt4_ncalrpc_open(RpcConnection* Connection)
{
  static LPCSTR prefix = "\\\\.\\pipe\\lrpc\\";
  RPC_STATUS r;
  LPSTR pname;

  /* already connected? */
  if (Connection->conn)
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
  static LPCSTR prefix = "\\\\.";
  RPC_STATUS r;
  LPSTR pname;

  /* already connected? */
  if (Connection->conn)
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

static HANDLE rpcrt4_conn_np_get_connect_event(RpcConnection *conn)
{
  return conn->ovl.hEvent;
}

static RPC_STATUS rpcrt4_conn_np_handoff(RpcConnection *old_conn, RpcConnection *new_conn)
{
  /* because of the way named pipes work, we'll transfer the connected pipe
   * to the child, then reopen the server binding to continue listening */
  
  new_conn->conn = old_conn->conn;
  new_conn->ovl = old_conn->ovl;
  old_conn->conn = 0;
  memset(&old_conn->ovl, 0, sizeof(old_conn->ovl));
  return RPCRT4_OpenConnection(old_conn);
}

static int rpcrt4_conn_np_read(RpcConnection *Connection,
                        void *buffer, unsigned int count)
{
  DWORD dwRead = 0;
  if (!ReadFile(Connection->conn, buffer, count, &dwRead, NULL) &&
      (GetLastError() != ERROR_MORE_DATA))
    return -1;
  return dwRead;
}

static int rpcrt4_conn_np_write(RpcConnection *Connection,
                             const void *buffer, unsigned int count)
{
  DWORD dwWritten = 0;
  if (!WriteFile(Connection->conn, buffer, count, &dwWritten, NULL))
    return -1;
  return dwWritten;
}

static int rpcrt4_conn_np_close(RpcConnection *Connection)
{
  if (Connection->conn) {
    FlushFileBuffers(Connection->conn);
    CloseHandle(Connection->conn);
    Connection->conn = 0;
  }
  if (Connection->ovl.hEvent) {
    CloseHandle(Connection->ovl.hEvent);
    Connection->ovl.hEvent = 0;
  }
  return 0;
}

struct protseq_ops protseq_list[] = {
  { "ncacn_np",
    rpcrt4_ncalrpc_open,
    rpcrt4_conn_np_get_connect_event,
    rpcrt4_conn_np_handoff,
    rpcrt4_conn_np_read,
    rpcrt4_conn_np_write,
    rpcrt4_conn_np_close,
  },
  { "ncalrpc",
    rpcrt4_ncacn_np_open,
    rpcrt4_conn_np_get_connect_event,
    rpcrt4_conn_np_handoff,
    rpcrt4_conn_np_read,
    rpcrt4_conn_np_write,
    rpcrt4_conn_np_close,
  },
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

RPC_STATUS RPCRT4_CreateConnection(RpcConnection** Connection, BOOL server, LPCSTR Protseq, LPCSTR NetworkAddr, LPCSTR Endpoint, LPCSTR NetworkOptions, RpcBinding* Binding)
{
  struct protseq_ops *ops;
  RpcConnection* NewConnection;

  ops = rpcrt4_get_protseq_ops(Protseq);
  if (!ops)
    return RPC_S_PROTSEQ_NOT_SUPPORTED;

  NewConnection = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(RpcConnection));
  NewConnection->server = server;
  NewConnection->ops = ops;
  NewConnection->NetworkAddr = RPCRT4_strdupA(NetworkAddr);
  NewConnection->Endpoint = RPCRT4_strdupA(Endpoint);
  NewConnection->Used = Binding;
  NewConnection->MaxTransmissionSize = RPC_MAX_PACKET_SIZE;

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
                                OldConnection->Endpoint, NULL, NULL);
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
  HeapFree(GetProcessHeap(), 0, Connection);
  return RPC_S_OK;
}
