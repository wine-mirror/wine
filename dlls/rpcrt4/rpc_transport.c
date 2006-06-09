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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 */

#include "config.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

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
#include "epm_towers.h"

WINE_DEFAULT_DEBUG_CHANNEL(rpc);

static CRITICAL_SECTION connection_pool_cs;
static CRITICAL_SECTION_DEBUG connection_pool_cs_debug =
{
    0, 0, &connection_pool_cs,
    { &connection_pool_cs_debug.ProcessLocksList, &connection_pool_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": connection_pool") }
};
static CRITICAL_SECTION connection_pool_cs = { &connection_pool_cs_debug, -1, 0, 0, 0, 0 };

static struct list connection_pool = LIST_INIT(connection_pool);

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
  if (npc->pipe == INVALID_HANDLE_VALUE) {
    WARN("CreateNamedPipe failed with error %ld\n", GetLastError());
    return RPC_S_SERVER_UNAVAILABLE;
  }

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

static size_t rpcrt4_ncacn_np_get_top_of_tower(unsigned char *tower_data,
                                               const char *networkaddr,
                                               const char *endpoint)
{
    twr_empty_floor_t *smb_floor;
    twr_empty_floor_t *nb_floor;
    size_t size;
    size_t networkaddr_size;
    size_t endpoint_size;

    TRACE("(%p, %s, %s)\n", tower_data, networkaddr, endpoint);

    networkaddr_size = strlen(networkaddr) + 1;
    endpoint_size = strlen(endpoint) + 1;
    size = sizeof(*smb_floor) + endpoint_size + sizeof(*nb_floor) + networkaddr_size;

    if (!tower_data)
        return size;

    smb_floor = (twr_empty_floor_t *)tower_data;

    tower_data += sizeof(*smb_floor);

    smb_floor->count_lhs = sizeof(smb_floor->protid);
    smb_floor->protid = EPM_PROTOCOL_SMB;
    smb_floor->count_rhs = endpoint_size;

    memcpy(tower_data, endpoint, endpoint_size);
    tower_data += endpoint_size;

    nb_floor = (twr_empty_floor_t *)tower_data;

    tower_data += sizeof(*nb_floor);

    nb_floor->count_lhs = sizeof(nb_floor->protid);
    nb_floor->protid = EPM_PROTOCOL_NETBIOS;
    nb_floor->count_rhs = networkaddr_size;

    memcpy(tower_data, networkaddr, networkaddr_size);
    tower_data += networkaddr_size;

    return size;
}

static RPC_STATUS rpcrt4_ncacn_np_parse_top_of_tower(const unsigned char *tower_data,
                                                     size_t tower_size,
                                                     char **networkaddr,
                                                     char **endpoint)
{
    const twr_empty_floor_t *smb_floor = (const twr_empty_floor_t *)tower_data;
    const twr_empty_floor_t *nb_floor;

    TRACE("(%p, %d, %p, %p)\n", tower_data, tower_size, networkaddr, endpoint);

    if (tower_size < sizeof(*smb_floor))
        return EPT_S_NOT_REGISTERED;

    tower_data += sizeof(*smb_floor);
    tower_size -= sizeof(*smb_floor);

    if ((smb_floor->count_lhs != sizeof(smb_floor->protid)) ||
        (smb_floor->protid != EPM_PROTOCOL_SMB) ||
        (smb_floor->count_rhs > tower_size))
        return EPT_S_NOT_REGISTERED;

    if (endpoint)
    {
        *endpoint = HeapAlloc(GetProcessHeap(), 0, smb_floor->count_rhs);
        if (!*endpoint)
            return RPC_S_OUT_OF_RESOURCES;
        memcpy(*endpoint, tower_data, smb_floor->count_rhs);
    }
    tower_data += smb_floor->count_rhs;
    tower_size -= smb_floor->count_rhs;

    if (tower_size < sizeof(*nb_floor))
        return EPT_S_NOT_REGISTERED;

    nb_floor = (const twr_empty_floor_t *)tower_data;

    tower_data += sizeof(*nb_floor);
    tower_size -= sizeof(*nb_floor);

    if ((nb_floor->count_lhs != sizeof(nb_floor->protid)) ||
        (nb_floor->protid != EPM_PROTOCOL_NETBIOS) ||
        (nb_floor->count_rhs > tower_size))
        return EPT_S_NOT_REGISTERED;

    if (networkaddr)
    {
        *networkaddr = HeapAlloc(GetProcessHeap(), 0, nb_floor->count_rhs);
        if (!*networkaddr)
        {
            if (endpoint)
            {
                HeapFree(GetProcessHeap(), 0, *endpoint);
                *endpoint = NULL;
            }
            return RPC_S_OUT_OF_RESOURCES;
        }
        memcpy(*networkaddr, tower_data, nb_floor->count_rhs);
    }

    return RPC_S_OK;
}

static size_t rpcrt4_ncalrpc_get_top_of_tower(unsigned char *tower_data,
                                              const char *networkaddr,
                                              const char *endpoint)
{
    twr_empty_floor_t *pipe_floor;
    size_t size;
    size_t endpoint_size;

    TRACE("(%p, %s, %s)\n", tower_data, networkaddr, endpoint);

    endpoint_size = strlen(networkaddr) + 1;
    size = sizeof(*pipe_floor) + endpoint_size;

    if (!tower_data)
        return size;

    pipe_floor = (twr_empty_floor_t *)tower_data;

    tower_data += sizeof(*pipe_floor);

    pipe_floor->count_lhs = sizeof(pipe_floor->protid);
    pipe_floor->protid = EPM_PROTOCOL_SMB;
    pipe_floor->count_rhs = endpoint_size;

    memcpy(tower_data, endpoint, endpoint_size);
    tower_data += endpoint_size;

    return size;
}

static RPC_STATUS rpcrt4_ncalrpc_parse_top_of_tower(const unsigned char *tower_data,
                                                    size_t tower_size,
                                                    char **networkaddr,
                                                    char **endpoint)
{
    const twr_empty_floor_t *pipe_floor = (const twr_empty_floor_t *)tower_data;

    TRACE("(%p, %d, %p, %p)\n", tower_data, tower_size, networkaddr, endpoint);

    *networkaddr = NULL;
    *endpoint = NULL;

    if (tower_size < sizeof(*pipe_floor))
        return EPT_S_NOT_REGISTERED;

    tower_data += sizeof(*pipe_floor);
    tower_size -= sizeof(*pipe_floor);

    if ((pipe_floor->count_lhs != sizeof(pipe_floor->protid)) ||
        (pipe_floor->protid != EPM_PROTOCOL_SMB) ||
        (pipe_floor->count_rhs > tower_size))
        return EPT_S_NOT_REGISTERED;

    if (endpoint)
    {
        *endpoint = HeapAlloc(GetProcessHeap(), 0, pipe_floor->count_rhs);
        if (!*endpoint)
            return RPC_S_OUT_OF_RESOURCES;
        memcpy(*endpoint, tower_data, pipe_floor->count_rhs);
    }

    return RPC_S_OK;
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
    ERR("getaddrinfo failed: %s\n", gai_strerror(ret));
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

static size_t rpcrt4_ncacn_ip_tcp_get_top_of_tower(unsigned char *tower_data,
                                                   const char *networkaddr,
                                                   const char *endpoint)
{
    twr_tcp_floor_t *tcp_floor;
    twr_ipv4_floor_t *ipv4_floor;
    struct addrinfo *ai;
    struct addrinfo hints;
    int ret;
    size_t size = sizeof(*tcp_floor) + sizeof(*ipv4_floor);

    TRACE("(%p, %s, %s)\n", tower_data, networkaddr, endpoint);

    if (!tower_data)
        return size;

    tcp_floor = (twr_tcp_floor_t *)tower_data;
    tower_data += sizeof(*tcp_floor);

    ipv4_floor = (twr_ipv4_floor_t *)tower_data;

    tcp_floor->count_lhs = sizeof(tcp_floor->protid);
    tcp_floor->protid = EPM_PROTOCOL_TCP;
    tcp_floor->count_rhs = sizeof(tcp_floor->port);

    ipv4_floor->count_lhs = sizeof(ipv4_floor->protid);
    ipv4_floor->protid = EPM_PROTOCOL_IP;
    ipv4_floor->count_rhs = sizeof(ipv4_floor->ipv4addr);

    hints.ai_flags          = AI_NUMERICHOST;
    /* FIXME: only support IPv4 at the moment. how is IPv6 represented by the EPM? */
    hints.ai_family         = PF_INET;
    hints.ai_socktype       = SOCK_STREAM;
    hints.ai_protocol       = IPPROTO_TCP;
    hints.ai_addrlen        = 0;
    hints.ai_addr           = NULL;
    hints.ai_canonname      = NULL;
    hints.ai_next           = NULL;

    ret = getaddrinfo(networkaddr, endpoint, &hints, &ai);
    if (ret < 0)
    {
        ret = getaddrinfo("0.0.0.0", endpoint, &hints, &ai);
        if (ret < 0)
        {
            ERR("getaddrinfo failed: %s\n", gai_strerror(ret));
            return 0;
        }
    }

    if (ai->ai_family == PF_INET)
    {
        const struct sockaddr_in *sin = (const struct sockaddr_in *)ai->ai_addr;
        tcp_floor->port = sin->sin_port;
        ipv4_floor->ipv4addr = sin->sin_addr.s_addr;
    }
    else
    {
        ERR("unexpected protocol family %d\n", ai->ai_family);
        return 0;
    }

    freeaddrinfo(ai);

    return size;
}

static RPC_STATUS rpcrt4_ncacn_ip_tcp_parse_top_of_tower(const unsigned char *tower_data,
                                                         size_t tower_size,
                                                         char **networkaddr,
                                                         char **endpoint)
{
    const twr_tcp_floor_t *tcp_floor = (const twr_tcp_floor_t *)tower_data;
    const twr_ipv4_floor_t *ipv4_floor;
    struct in_addr in_addr;

    TRACE("(%p, %d, %p, %p)\n", tower_data, tower_size, networkaddr, endpoint);

    if (tower_size < sizeof(*tcp_floor))
        return EPT_S_NOT_REGISTERED;

    tower_data += sizeof(*tcp_floor);
    tower_size -= sizeof(*tcp_floor);

    if (tower_size < sizeof(*ipv4_floor))
        return EPT_S_NOT_REGISTERED;

    ipv4_floor = (const twr_ipv4_floor_t *)tower_data;

    if ((tcp_floor->count_lhs != sizeof(tcp_floor->protid)) ||
        (tcp_floor->protid != EPM_PROTOCOL_TCP) ||
        (tcp_floor->count_rhs != sizeof(tcp_floor->port)) ||
        (ipv4_floor->count_lhs != sizeof(ipv4_floor->protid)) ||
        (ipv4_floor->protid != EPM_PROTOCOL_IP) ||
        (ipv4_floor->count_rhs != sizeof(ipv4_floor->ipv4addr)))
        return EPT_S_NOT_REGISTERED;

    if (endpoint)
    {
        *endpoint = HeapAlloc(GetProcessHeap(), 0, 6);
        if (!*endpoint)
            return RPC_S_OUT_OF_RESOURCES;
        sprintf(*endpoint, "%u", ntohs(tcp_floor->port));
    }

    if (networkaddr)
    {
        *networkaddr = HeapAlloc(GetProcessHeap(), 0, INET_ADDRSTRLEN);
        if (!*networkaddr)
        {
            if (endpoint)
            {
                HeapFree(GetProcessHeap(), 0, *endpoint);
                *endpoint = NULL;
            }
            return RPC_S_OUT_OF_RESOURCES;
        }
        in_addr.s_addr = ipv4_floor->ipv4addr;
        if (!inet_ntop(AF_INET, &in_addr, *networkaddr, INET_ADDRSTRLEN))
        {
            ERR("inet_ntop: %s\n", strerror(errno));
            HeapFree(GetProcessHeap(), 0, *networkaddr);
            *networkaddr = NULL;
            if (endpoint)
            {
                HeapFree(GetProcessHeap(), 0, *endpoint);
                *endpoint = NULL;
            }
            return EPT_S_NOT_REGISTERED;
        }
    }

    return RPC_S_OK;
}

static const struct protseq_ops protseq_list[] = {
  { "ncacn_np",
    { EPM_PROTOCOL_NCACN, EPM_PROTOCOL_SMB },
    rpcrt4_conn_np_alloc,
    rpcrt4_ncacn_np_open,
    rpcrt4_conn_np_get_connect_event,
    rpcrt4_conn_np_handoff,
    rpcrt4_conn_np_read,
    rpcrt4_conn_np_write,
    rpcrt4_conn_np_close,
    rpcrt4_ncacn_np_get_top_of_tower,
    rpcrt4_ncacn_np_parse_top_of_tower,
  },
  { "ncalrpc",
    { EPM_PROTOCOL_NCALRPC, EPM_PROTOCOL_PIPE },
    rpcrt4_conn_np_alloc,
    rpcrt4_ncalrpc_open,
    rpcrt4_conn_np_get_connect_event,
    rpcrt4_conn_np_handoff,
    rpcrt4_conn_np_read,
    rpcrt4_conn_np_write,
    rpcrt4_conn_np_close,
    rpcrt4_ncalrpc_get_top_of_tower,
    rpcrt4_ncalrpc_parse_top_of_tower,
  },
  { "ncacn_ip_tcp",
    { EPM_PROTOCOL_NCACN, EPM_PROTOCOL_TCP },
    rpcrt4_conn_tcp_alloc,
    rpcrt4_ncacn_ip_tcp_open,
    rpcrt4_conn_tcp_get_wait_handle,
    rpcrt4_conn_tcp_handoff,
    rpcrt4_conn_tcp_read,
    rpcrt4_conn_tcp_write,
    rpcrt4_conn_tcp_close,
    rpcrt4_ncacn_ip_tcp_get_top_of_tower,
    rpcrt4_ncacn_ip_tcp_parse_top_of_tower,
  }
};

#define MAX_PROTSEQ (sizeof protseq_list / sizeof protseq_list[0])

static const struct protseq_ops *rpcrt4_get_protseq_ops(const char *protseq)
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
  const struct protseq_ops *ops;
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
  list_init(&NewConnection->conn_pool_entry);

  TRACE("connection: %p\n", NewConnection);
  *Connection = NewConnection;

  return RPC_S_OK;
}

RpcConnection *RPCRT4_GetIdleConnection(const RPC_SYNTAX_IDENTIFIER *InterfaceId,
    const RPC_SYNTAX_IDENTIFIER *TransferSyntax, LPCSTR Protseq, LPCSTR NetworkAddr,
    LPCSTR Endpoint, RpcAuthInfo* AuthInfo)
{
  RpcConnection *Connection;
  /* try to find a compatible connection from the connection pool */
  EnterCriticalSection(&connection_pool_cs);
  LIST_FOR_EACH_ENTRY(Connection, &connection_pool, RpcConnection, conn_pool_entry)
    if ((Connection->AuthInfo == AuthInfo) &&
        !memcmp(&Connection->ActiveInterface, InterfaceId,
           sizeof(RPC_SYNTAX_IDENTIFIER)) &&
        !strcmp(rpcrt4_conn_get_name(Connection), Protseq) &&
        !strcmp(Connection->NetworkAddr, NetworkAddr) &&
        !strcmp(Connection->Endpoint, Endpoint))
    {
      list_remove(&Connection->conn_pool_entry);
      LeaveCriticalSection(&connection_pool_cs);
      TRACE("got connection from pool %p\n", Connection);
      return Connection;
    }

  LeaveCriticalSection(&connection_pool_cs);
  return NULL;
}

void RPCRT4_ReleaseIdleConnection(RpcConnection *Connection)
{
  assert(!Connection->server);
  EnterCriticalSection(&connection_pool_cs);
  list_add_head(&connection_pool, &Connection->conn_pool_entry);
  LeaveCriticalSection(&connection_pool_cs);
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

RPC_STATUS RpcTransport_GetTopOfTower(unsigned char *tower_data,
                                      size_t *tower_size,
                                      const char *protseq,
                                      const char *networkaddr,
                                      const char *endpoint)
{
    twr_empty_floor_t *protocol_floor;
    const struct protseq_ops *protseq_ops = rpcrt4_get_protseq_ops(protseq);

    *tower_size = 0;

    if (!protseq_ops)
        return RPC_S_INVALID_RPC_PROTSEQ;

    if (!tower_data)
    {
        *tower_size = sizeof(*protocol_floor);
        *tower_size += protseq_ops->get_top_of_tower(NULL, networkaddr, endpoint);
        return RPC_S_OK;
    }

    protocol_floor = (twr_empty_floor_t *)tower_data;
    protocol_floor->count_lhs = sizeof(protocol_floor->protid);
    protocol_floor->protid = protseq_ops->epm_protocols[0];
    protocol_floor->count_rhs = 0;

    tower_data += sizeof(*protocol_floor);

    *tower_size = protseq_ops->get_top_of_tower(tower_data, networkaddr, endpoint);
    if (!*tower_size)
        return EPT_S_NOT_REGISTERED;

    *tower_size += sizeof(*protocol_floor);

    return RPC_S_OK;
}

RPC_STATUS RpcTransport_ParseTopOfTower(const unsigned char *tower_data,
                                        size_t tower_size,
                                        char **protseq,
                                        char **networkaddr,
                                        char **endpoint)
{
    twr_empty_floor_t *protocol_floor;
    twr_empty_floor_t *floor4;
    const struct protseq_ops *protseq_ops = NULL;
    RPC_STATUS status;
    int i;

    if (tower_size < sizeof(*protocol_floor))
        return EPT_S_NOT_REGISTERED;

    protocol_floor = (twr_empty_floor_t *)tower_data;
    tower_data += sizeof(*protocol_floor);
    tower_size -= sizeof(*protocol_floor);
    if ((protocol_floor->count_lhs != sizeof(protocol_floor->protid)) ||
        (protocol_floor->count_rhs > tower_size))
        return EPT_S_NOT_REGISTERED;
    tower_data += protocol_floor->count_rhs;
    tower_size -= protocol_floor->count_rhs;

    floor4 = (twr_empty_floor_t *)tower_data;
    if ((tower_size < sizeof(*floor4)) ||
        (floor4->count_lhs != sizeof(floor4->protid)))
        return EPT_S_NOT_REGISTERED;

    for(i = 0; i < MAX_PROTSEQ; i++)
        if ((protocol_floor->protid == protseq_list[i].epm_protocols[0]) &&
            (floor4->protid == protseq_list[i].epm_protocols[1]))
        {
            protseq_ops = &protseq_list[i];
            break;
        }

    if (!protseq_ops)
        return EPT_S_NOT_REGISTERED;

    status = protseq_ops->parse_top_of_tower(tower_data, tower_size, networkaddr, endpoint);

    if ((status == RPC_S_OK) && protseq)
    {
        *protseq = HeapAlloc(GetProcessHeap(), 0, strlen(protseq_ops->name) + 1);
        strcpy(*protseq, protseq_ops->name);
    }

    return status;
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
