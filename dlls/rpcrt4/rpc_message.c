/*
 * RPC messages
 *
 * Copyright 2001-2002 Ove Kåven, TransGaming Technologies
 * Copyright 2004 Filip Navara
 * Copyright 2006 CodeWeavers
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
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"

#include "rpc.h"
#include "rpcndr.h"
#include "rpcdcep.h"

#include "wine/debug.h"

#include "rpc_binding.h"
#include "rpc_misc.h"
#include "rpc_defs.h"
#include "rpc_message.h"

WINE_DEFAULT_DEBUG_CHANNEL(rpc);

/* note: the DCE/RPC spec says the alignment amount should be 4, but
 * MS/RPC servers seem to always use 16 */
#define AUTH_ALIGNMENT 16

/* gets the amount needed to round a value up to the specified alignment */
#define ROUND_UP_AMOUNT(value, alignment) \
    (((alignment) - (((value) % (alignment)))) % (alignment))

static DWORD RPCRT4_GetHeaderSize(RpcPktHdr *Header)
{
  static const DWORD header_sizes[] = {
    sizeof(Header->request), 0, sizeof(Header->response),
    sizeof(Header->fault), 0, 0, 0, 0, 0, 0, 0, sizeof(Header->bind),
    sizeof(Header->bind_ack), sizeof(Header->bind_nack),
    0, 0, 0, 0, 0
  };
  ULONG ret = 0;
  
  if (Header->common.ptype < sizeof(header_sizes) / sizeof(header_sizes[0])) {
    ret = header_sizes[Header->common.ptype];
    if (ret == 0)
      FIXME("unhandled packet type\n");
    if (Header->common.flags & RPC_FLG_OBJECT_UUID)
      ret += sizeof(UUID);
  } else {
    TRACE("invalid packet type\n");
  }

  return ret;
}

static VOID RPCRT4_BuildCommonHeader(RpcPktHdr *Header, unsigned char PacketType,
                              unsigned long DataRepresentation)
{
  Header->common.rpc_ver = RPC_VER_MAJOR;
  Header->common.rpc_ver_minor = RPC_VER_MINOR;
  Header->common.ptype = PacketType;
  Header->common.drep[0] = LOBYTE(LOWORD(DataRepresentation));
  Header->common.drep[1] = HIBYTE(LOWORD(DataRepresentation));
  Header->common.drep[2] = LOBYTE(HIWORD(DataRepresentation));
  Header->common.drep[3] = HIBYTE(HIWORD(DataRepresentation));
  Header->common.auth_len = 0;
  Header->common.call_id = 1;
  Header->common.flags = 0;
  /* Flags and fragment length are computed in RPCRT4_Send. */
}                              

static RpcPktHdr *RPCRT4_BuildRequestHeader(unsigned long DataRepresentation,
                                     unsigned long BufferLength,
                                     unsigned short ProcNum,
                                     UUID *ObjectUuid)
{
  RpcPktHdr *header;
  BOOL has_object;
  RPC_STATUS status;

  has_object = (ObjectUuid != NULL && !UuidIsNil(ObjectUuid, &status));
  header = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                     sizeof(header->request) + (has_object ? sizeof(UUID) : 0));
  if (header == NULL) {
    return NULL;
  }

  RPCRT4_BuildCommonHeader(header, PKT_REQUEST, DataRepresentation);
  header->common.frag_len = sizeof(header->request);
  header->request.alloc_hint = BufferLength;
  header->request.context_id = 0;
  header->request.opnum = ProcNum;
  if (has_object) {
    header->common.flags |= RPC_FLG_OBJECT_UUID;
    header->common.frag_len += sizeof(UUID);
    memcpy(&header->request + 1, ObjectUuid, sizeof(UUID));
  }

  return header;
}

static RpcPktHdr *RPCRT4_BuildResponseHeader(unsigned long DataRepresentation,
                                      unsigned long BufferLength)
{
  RpcPktHdr *header;

  header = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(header->response));
  if (header == NULL) {
    return NULL;
  }

  RPCRT4_BuildCommonHeader(header, PKT_RESPONSE, DataRepresentation);
  header->common.frag_len = sizeof(header->response);
  header->response.alloc_hint = BufferLength;

  return header;
}

RpcPktHdr *RPCRT4_BuildFaultHeader(unsigned long DataRepresentation,
                                   RPC_STATUS Status)
{
  RpcPktHdr *header;

  header = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(header->fault));
  if (header == NULL) {
    return NULL;
  }

  RPCRT4_BuildCommonHeader(header, PKT_FAULT, DataRepresentation);
  header->common.frag_len = sizeof(header->fault);
  header->fault.status = Status;

  return header;
}

RpcPktHdr *RPCRT4_BuildBindHeader(unsigned long DataRepresentation,
                                  unsigned short MaxTransmissionSize,
                                  unsigned short MaxReceiveSize,
                                  RPC_SYNTAX_IDENTIFIER *AbstractId,
                                  RPC_SYNTAX_IDENTIFIER *TransferId)
{
  RpcPktHdr *header;

  header = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(header->bind));
  if (header == NULL) {
    return NULL;
  }

  RPCRT4_BuildCommonHeader(header, PKT_BIND, DataRepresentation);
  header->common.frag_len = sizeof(header->bind);
  header->bind.max_tsize = MaxTransmissionSize;
  header->bind.max_rsize = MaxReceiveSize;
  header->bind.num_elements = 1;
  header->bind.num_syntaxes = 1;
  memcpy(&header->bind.abstract, AbstractId, sizeof(RPC_SYNTAX_IDENTIFIER));
  memcpy(&header->bind.transfer, TransferId, sizeof(RPC_SYNTAX_IDENTIFIER));

  return header;
}

RpcPktHdr *RPCRT4_BuildAuthHeader(unsigned long DataRepresentation)
{
  RpcPktHdr *header;

  header = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                     sizeof(header->common) + 12);
  if (header == NULL)
    return NULL;

  RPCRT4_BuildCommonHeader(header, PKT_AUTH3, DataRepresentation);
  header->common.frag_len = 0x14;
  header->common.auth_len = 0;

  return header;
}

RpcPktHdr *RPCRT4_BuildBindNackHeader(unsigned long DataRepresentation,
                                      unsigned char RpcVersion,
                                      unsigned char RpcVersionMinor)
{
  RpcPktHdr *header;

  header = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(header->bind_nack));
  if (header == NULL) {
    return NULL;
  }

  RPCRT4_BuildCommonHeader(header, PKT_BIND_NACK, DataRepresentation);
  header->common.frag_len = sizeof(header->bind_nack);
  header->bind_nack.protocols_count = 1;
  header->bind_nack.protocols[0].rpc_ver = RpcVersion;
  header->bind_nack.protocols[0].rpc_ver_minor = RpcVersionMinor;

  return header;
}

RpcPktHdr *RPCRT4_BuildBindAckHeader(unsigned long DataRepresentation,
                                     unsigned short MaxTransmissionSize,
                                     unsigned short MaxReceiveSize,
                                     LPSTR ServerAddress,
                                     unsigned long Result,
                                     unsigned long Reason,
                                     RPC_SYNTAX_IDENTIFIER *TransferId)
{
  RpcPktHdr *header;
  unsigned long header_size;
  RpcAddressString *server_address;
  RpcResults *results;
  RPC_SYNTAX_IDENTIFIER *transfer_id;

  header_size = sizeof(header->bind_ack) + sizeof(RpcResults) +
                sizeof(RPC_SYNTAX_IDENTIFIER) + sizeof(RpcAddressString) +
                strlen(ServerAddress);

  header = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, header_size);
  if (header == NULL) {
    return NULL;
  }

  RPCRT4_BuildCommonHeader(header, PKT_BIND_ACK, DataRepresentation);
  header->common.frag_len = header_size;
  header->bind_ack.max_tsize = MaxTransmissionSize;
  header->bind_ack.max_rsize = MaxReceiveSize;
  server_address = (RpcAddressString*)(&header->bind_ack + 1);
  server_address->length = strlen(ServerAddress) + 1;
  strcpy(server_address->string, ServerAddress);
  results = (RpcResults*)((ULONG_PTR)server_address + sizeof(RpcAddressString) + server_address->length - 1);
  results->num_results = 1;
  results->results[0].result = Result;
  results->results[0].reason = Reason;
  transfer_id = (RPC_SYNTAX_IDENTIFIER*)(results + 1);
  memcpy(transfer_id, TransferId, sizeof(RPC_SYNTAX_IDENTIFIER));

  return header;
}

VOID RPCRT4_FreeHeader(RpcPktHdr *Header)
{
  HeapFree(GetProcessHeap(), 0, Header);
}

/***********************************************************************
 *           RPCRT4_SendAuth (internal)
 * 
 * Transmit a packet with authorization data over connection in acceptable fragments.
 */
static RPC_STATUS RPCRT4_SendAuth(RpcConnection *Connection, RpcPktHdr *Header,
                                  void *Buffer, unsigned int BufferLength,
                                  void *Auth, unsigned int AuthLength)
{
  PUCHAR buffer_pos;
  DWORD hdr_size;
  LONG count;
  unsigned char *pkt;
  LONG alen = AuthLength ? (AuthLength + sizeof(RpcAuthVerifier)) : 0;

  buffer_pos = Buffer;
  /* The packet building functions save the packet header size, so we can use it. */
  hdr_size = Header->common.frag_len;
  Header->common.auth_len = AuthLength;
  Header->common.flags |= RPC_FLG_FIRST;
  Header->common.flags &= ~RPC_FLG_LAST;
  while (!(Header->common.flags & RPC_FLG_LAST)) {
    unsigned char auth_pad_len = AuthLength ? ROUND_UP_AMOUNT(BufferLength, AUTH_ALIGNMENT) : 0;
    unsigned int pkt_size = BufferLength + hdr_size + alen + auth_pad_len;

    /* decide if we need to split the packet into fragments */
   if (pkt_size <= Connection->MaxTransmissionSize) {
     Header->common.flags |= RPC_FLG_LAST;
     Header->common.frag_len = pkt_size;
    } else {
      auth_pad_len = 0;
      /* make sure packet payload will be a multiple of 16 */
      Header->common.frag_len =
        ((Connection->MaxTransmissionSize - hdr_size - alen) & ~(AUTH_ALIGNMENT-1)) +
        hdr_size + alen;
    }

    pkt = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Header->common.frag_len);

    memcpy(pkt, Header, hdr_size);

    /* fragment consisted of header only and is the last one */
    if (hdr_size == Header->common.frag_len)
      goto write;

    memcpy(pkt + hdr_size, buffer_pos, Header->common.frag_len - hdr_size - auth_pad_len - alen);

    /* add the authorization info */
    if (Connection->AuthInfo && AuthLength)
    {
      RpcAuthVerifier *auth_hdr = (RpcAuthVerifier *)&pkt[Header->common.frag_len - alen];

      auth_hdr->auth_type = Connection->AuthInfo->AuthnSvc;
      auth_hdr->auth_level = Connection->AuthInfo->AuthnLevel;
      auth_hdr->auth_pad_length = auth_pad_len;
      auth_hdr->auth_reserved = 0;
      /* a unique number... */
      auth_hdr->auth_context_id = (unsigned long)Connection;

      memcpy(auth_hdr + 1, Auth, AuthLength);
    }

write:
    count = rpcrt4_conn_write(Connection, pkt, Header->common.frag_len);
    HeapFree(GetProcessHeap(), 0, pkt);
    if (count<0) {
      WARN("rpcrt4_conn_write failed (auth)\n");
      return RPC_S_PROTOCOL_ERROR;
    }

    buffer_pos += Header->common.frag_len - hdr_size - alen - auth_pad_len;
    BufferLength -= Header->common.frag_len - hdr_size - alen - auth_pad_len;
    Header->common.flags &= ~RPC_FLG_FIRST;
  }

  return RPC_S_OK;
}

/***********************************************************************
 *           RPCRT4_AuthNegotiate (internal)
 */
static void RPCRT4_AuthNegotiate(RpcConnection *conn, SecBuffer *out)
{
  SECURITY_STATUS r;
  SecBufferDesc out_desc;
  unsigned char *buffer;

  buffer = HeapAlloc(GetProcessHeap(), 0, 0x100);

  out->BufferType = SECBUFFER_TOKEN;
  out->cbBuffer = 0x100;
  out->pvBuffer = buffer;

  out_desc.ulVersion = 0;
  out_desc.cBuffers = 1;
  out_desc.pBuffers = out;

  conn->attr = 0;
  conn->ctx.dwLower = 0;
  conn->ctx.dwUpper = 0;

  r = InitializeSecurityContextA(&conn->AuthInfo->cred, NULL, NULL,
        ISC_REQ_CONNECTION | ISC_REQ_USE_DCE_STYLE | ISC_REQ_MUTUAL_AUTH |
        ISC_REQ_DELEGATE, 0, SECURITY_NETWORK_DREP,
        NULL, 0, &conn->ctx, &out_desc, &conn->attr, &conn->exp);

  TRACE("r = %08lx cbBuffer = %ld attr = %08lx\n", r, out->cbBuffer, conn->attr);
}

/***********************************************************************
 *           RPCRT4_AuthorizeBinding (internal)
 */
static RPC_STATUS RPCRT_AuthorizeConnection(RpcConnection* conn,
                                            BYTE *challenge, ULONG count)
{
  SecBufferDesc inp_desc, out_desc;
  SecBuffer inp, out;
  SECURITY_STATUS r;
  unsigned char buffer[0x100];
  RpcPktHdr *resp_hdr;
  RPC_STATUS status;

  TRACE("challenge %s, %ld bytes\n", challenge, count);

  out.BufferType = SECBUFFER_TOKEN;
  out.cbBuffer = sizeof buffer;
  out.pvBuffer = buffer;

  out_desc.ulVersion = 0;
  out_desc.cBuffers = 1;
  out_desc.pBuffers = &out;

  inp.BufferType = SECBUFFER_TOKEN;
  inp.pvBuffer = challenge;
  inp.cbBuffer = count;

  inp_desc.cBuffers = 1;
  inp_desc.pBuffers = &inp;
  inp_desc.ulVersion = 0;

  r = InitializeSecurityContextA(&conn->AuthInfo->cred, &conn->ctx, NULL,
        ISC_REQ_CONNECTION | ISC_REQ_USE_DCE_STYLE | ISC_REQ_MUTUAL_AUTH |
        ISC_REQ_DELEGATE, 0, SECURITY_NETWORK_DREP,
        &inp_desc, 0, &conn->ctx, &out_desc, &conn->attr, &conn->exp);
  if (r)
  {
    WARN("InitializeSecurityContext failed with error 0x%08lx\n", r);
    return ERROR_ACCESS_DENIED;
  }

  resp_hdr = RPCRT4_BuildAuthHeader(NDR_LOCAL_DATA_REPRESENTATION);
  if (!resp_hdr)
    return E_OUTOFMEMORY;

  status = RPCRT4_SendAuth(conn, resp_hdr, NULL, 0, out.pvBuffer, out.cbBuffer);

  RPCRT4_FreeHeader(resp_hdr);

  return status;
}

/***********************************************************************
 *           RPCRT4_Send (internal)
 * 
 * Transmit a packet over connection in acceptable fragments.
 */
RPC_STATUS RPCRT4_Send(RpcConnection *Connection, RpcPktHdr *Header,
                       void *Buffer, unsigned int BufferLength)
{
  RPC_STATUS r;
  SecBuffer out;

  /* if we've already authenticated, just send the context */
  if (Connection->ctx.dwUpper || Connection->ctx.dwLower)
  {
    unsigned char buffer[0x10];

    memset(buffer, 0, sizeof buffer);
    buffer[0] = 1; /* version number lsb */

    return RPCRT4_SendAuth(Connection, Header, Buffer, BufferLength, buffer, sizeof buffer);
  }

  if (!Connection->AuthInfo ||
      Connection->AuthInfo->AuthnLevel == RPC_C_AUTHN_LEVEL_DEFAULT ||
      Connection->AuthInfo->AuthnLevel == RPC_C_AUTHN_LEVEL_NONE)
  {
    return RPCRT4_SendAuth(Connection, Header, Buffer, BufferLength, NULL, 0);
  }

  out.BufferType = SECBUFFER_TOKEN;
  out.cbBuffer = 0;
  out.pvBuffer = NULL;

  /* tack on a negotiate packet */
  RPCRT4_AuthNegotiate(Connection, &out);
  r = RPCRT4_SendAuth(Connection, Header, Buffer, BufferLength, out.pvBuffer, out.cbBuffer);
  HeapFree(GetProcessHeap(), 0, out.pvBuffer);

  return r;
}

/***********************************************************************
 *           RPCRT4_Receive (internal)
 * 
 * Receive a packet from connection and merge the fragments.
 */
RPC_STATUS RPCRT4_Receive(RpcConnection *Connection, RpcPktHdr **Header,
                          PRPC_MESSAGE pMsg)
{
  RPC_STATUS status;
  DWORD hdr_length;
  LONG dwRead;
  unsigned short first_flag;
  unsigned long data_length;
  unsigned long buffer_length;
  unsigned char *buffer_ptr;
  RpcPktCommonHdr common_hdr;

  *Header = NULL;

  TRACE("(%p, %p, %p)\n", Connection, Header, pMsg);

  /* read packet common header */
  dwRead = rpcrt4_conn_read(Connection, &common_hdr, sizeof(common_hdr));
  if (dwRead != sizeof(common_hdr)) {
    WARN("Short read of header, %ld bytes\n", dwRead);
    status = RPC_S_PROTOCOL_ERROR;
    goto fail;
  }

  /* verify if the header really makes sense */
  if (common_hdr.rpc_ver != RPC_VER_MAJOR ||
      common_hdr.rpc_ver_minor != RPC_VER_MINOR) {
    WARN("unhandled packet version\n");
    status = RPC_S_PROTOCOL_ERROR;
    goto fail;
  }

  hdr_length = RPCRT4_GetHeaderSize((RpcPktHdr*)&common_hdr);
  if (hdr_length == 0) {
    WARN("header length == 0\n");
    status = RPC_S_PROTOCOL_ERROR;
    goto fail;
  }

  *Header = HeapAlloc(GetProcessHeap(), 0, hdr_length);
  memcpy(*Header, &common_hdr, sizeof(common_hdr));

  /* read the rest of packet header */
  dwRead = rpcrt4_conn_read(Connection, &(*Header)->common + 1, hdr_length - sizeof(common_hdr));
  if (dwRead != hdr_length - sizeof(common_hdr)) {
    WARN("bad header length, %ld/%ld bytes\n", dwRead, hdr_length - sizeof(common_hdr));
    status = RPC_S_PROTOCOL_ERROR;
    goto fail;
  }

  /* read packet body */
  switch (common_hdr.ptype) {
  case PKT_RESPONSE:
    pMsg->BufferLength = (*Header)->response.alloc_hint;
    break;
  case PKT_REQUEST:
    pMsg->BufferLength = (*Header)->request.alloc_hint;
    break;
  default:
    pMsg->BufferLength = common_hdr.frag_len - hdr_length;
  }

  TRACE("buffer length = %u\n", pMsg->BufferLength);

  status = I_RpcGetBuffer(pMsg);
  if (status != RPC_S_OK) goto fail;

  first_flag = RPC_FLG_FIRST;
  buffer_length = 0;
  buffer_ptr = pMsg->Buffer;
  while (buffer_length < pMsg->BufferLength)
  {
    data_length = (*Header)->common.frag_len - hdr_length;
    if (((*Header)->common.flags & RPC_FLG_FIRST) != first_flag ||
        data_length + buffer_length > pMsg->BufferLength) {
      TRACE("invalid packet flags or buffer length\n");
      status = RPC_S_PROTOCOL_ERROR;
      goto fail;
    }

    if (data_length == 0) dwRead = 0; else
    dwRead = rpcrt4_conn_read(Connection, buffer_ptr, data_length);
    if (dwRead != data_length) {
      WARN("bad data length, %ld/%ld\n", dwRead, data_length);
      status = RPC_S_PROTOCOL_ERROR;
      goto fail;
    }

    /* when there is no more data left, it should be the last packet */
    if (buffer_length == pMsg->BufferLength &&
        ((*Header)->common.flags & RPC_FLG_LAST) == 0) {
      WARN("no more data left, but not last packet\n");
      status = RPC_S_PROTOCOL_ERROR;
      goto fail;
    }

    buffer_length += data_length;
    if (buffer_length < pMsg->BufferLength) {
      TRACE("next header\n");

      /* read the header of next packet */
      dwRead = rpcrt4_conn_read(Connection, *Header, hdr_length);
      if (dwRead != hdr_length) {
        WARN("invalid packet header size (%ld)\n", dwRead);
        status = RPC_S_PROTOCOL_ERROR;
        goto fail;
      }

      buffer_ptr += data_length;
      first_flag = 0;
    }
  }

  /* respond to authorization request */
  if (common_hdr.ptype == PKT_BIND_ACK && common_hdr.auth_len > 8)
  {
    unsigned int offset;

    offset = common_hdr.frag_len - hdr_length - common_hdr.auth_len;
    status = RPCRT_AuthorizeConnection(Connection, (LPBYTE)pMsg->Buffer + offset,
                                       common_hdr.auth_len);
    if (status)
        goto fail;
  }

  /* success */
  status = RPC_S_OK;

fail:
  if (status != RPC_S_OK && *Header) {
    RPCRT4_FreeHeader(*Header);
    *Header = NULL;
  }
  return status;
}

/***********************************************************************
 *           I_RpcGetBuffer [RPCRT4.@]
 */
RPC_STATUS WINAPI I_RpcGetBuffer(PRPC_MESSAGE pMsg)
{
  TRACE("(%p): BufferLength=%d\n", pMsg, pMsg->BufferLength);
  /* FIXME: pfnAllocate? */
  pMsg->Buffer = HeapAlloc(GetProcessHeap(), 0, pMsg->BufferLength);

  TRACE("Buffer=%p\n", pMsg->Buffer);
  /* FIXME: which errors to return? */
  return pMsg->Buffer ? S_OK : E_OUTOFMEMORY;
}

/***********************************************************************
 *           I_RpcFreeBuffer [RPCRT4.@]
 */
RPC_STATUS WINAPI I_RpcFreeBuffer(PRPC_MESSAGE pMsg)
{
  TRACE("(%p) Buffer=%p\n", pMsg, pMsg->Buffer);
  /* FIXME: pfnFree? */
  HeapFree(GetProcessHeap(), 0, pMsg->Buffer);
  pMsg->Buffer = NULL;
  return S_OK;
}

/***********************************************************************
 *           I_RpcSend [RPCRT4.@]
 */
RPC_STATUS WINAPI I_RpcSend(PRPC_MESSAGE pMsg)
{
  RpcBinding* bind = (RpcBinding*)pMsg->Handle;
  RpcConnection* conn;
  RPC_CLIENT_INTERFACE* cif = NULL;
  RPC_SERVER_INTERFACE* sif = NULL;
  RPC_STATUS status;
  RpcPktHdr *hdr;

  TRACE("(%p)\n", pMsg);
  if (!bind) return RPC_S_INVALID_BINDING;

  if (bind->server) {
    sif = pMsg->RpcInterfaceInformation;
    if (!sif) return RPC_S_INTERFACE_NOT_FOUND; /* ? */
    status = RPCRT4_OpenBinding(bind, &conn, &sif->TransferSyntax,
                                &sif->InterfaceId);
  } else {
    cif = pMsg->RpcInterfaceInformation;
    if (!cif) return RPC_S_INTERFACE_NOT_FOUND; /* ? */

    if (!bind->Endpoint || !bind->Endpoint[0])
    {
      TRACE("automatically resolving partially bound binding\n");
      status = RpcEpResolveBinding(bind, cif);
      if (status != RPC_S_OK) return status;
    }

    status = RPCRT4_OpenBinding(bind, &conn, &cif->TransferSyntax,
                                &cif->InterfaceId);
  }

  if (status != RPC_S_OK) return status;

  if (bind->server) {
    if (pMsg->RpcFlags & WINE_RPCFLAG_EXCEPTION) {
      hdr = RPCRT4_BuildFaultHeader(pMsg->DataRepresentation,
                                    RPC_S_CALL_FAILED);
    } else {
      hdr = RPCRT4_BuildResponseHeader(pMsg->DataRepresentation,
                                       pMsg->BufferLength);
    }
  } else {
    hdr = RPCRT4_BuildRequestHeader(pMsg->DataRepresentation,
                                    pMsg->BufferLength, pMsg->ProcNum,
                                    &bind->ObjectUuid);
    hdr->common.call_id = conn->NextCallId++;
  }

  status = RPCRT4_Send(conn, hdr, pMsg->Buffer, pMsg->BufferLength);

  RPCRT4_FreeHeader(hdr);

  /* success */
  if (!bind->server) {
    /* save the connection, so the response can be read from it */
    pMsg->ReservedForRuntime = conn;
    return status;
  }
  RPCRT4_CloseBinding(bind, conn);

  return status;
}

/***********************************************************************
 *           I_RpcReceive [RPCRT4.@]
 */
RPC_STATUS WINAPI I_RpcReceive(PRPC_MESSAGE pMsg)
{
  RpcBinding* bind = (RpcBinding*)pMsg->Handle;
  RpcConnection* conn;
  RPC_CLIENT_INTERFACE* cif = NULL;
  RPC_SERVER_INTERFACE* sif = NULL;
  RPC_STATUS status;
  RpcPktHdr *hdr = NULL;

  TRACE("(%p)\n", pMsg);
  if (!bind) return RPC_S_INVALID_BINDING;

  if (pMsg->ReservedForRuntime) {
    conn = pMsg->ReservedForRuntime;
    pMsg->ReservedForRuntime = NULL;
  } else {
    if (bind->server) {
      sif = pMsg->RpcInterfaceInformation;
      if (!sif) return RPC_S_INTERFACE_NOT_FOUND; /* ? */
      status = RPCRT4_OpenBinding(bind, &conn, &sif->TransferSyntax,
                                  &sif->InterfaceId);
    } else {
      cif = pMsg->RpcInterfaceInformation;
      if (!cif) return RPC_S_INTERFACE_NOT_FOUND; /* ? */

      if (!bind->Endpoint || !bind->Endpoint[0])
      {
        TRACE("automatically resolving partially bound binding\n");
        status = RpcEpResolveBinding(bind, cif);
        if (status != RPC_S_OK) return status;
      }

      status = RPCRT4_OpenBinding(bind, &conn, &cif->TransferSyntax,
                                  &cif->InterfaceId);
    }
    if (status != RPC_S_OK) return status;
  }

  status = RPCRT4_Receive(conn, &hdr, pMsg);
  if (status != RPC_S_OK) {
    WARN("receive failed with error %lx\n", status);
    goto fail;
  }

  status = RPC_S_PROTOCOL_ERROR;

  switch (hdr->common.ptype) {
  case PKT_RESPONSE:
    if (bind->server) goto fail;
    break;
  case PKT_REQUEST:
    if (!bind->server) goto fail;
    break;
  case PKT_FAULT:
    pMsg->RpcFlags |= WINE_RPCFLAG_EXCEPTION;
    ERR ("we got fault packet with status 0x%lx\n", hdr->fault.status);
    status = hdr->fault.status; /* FIXME: do translation from nca error codes */
    goto fail;
  default:
    WARN("bad packet type %d\n", hdr->common.ptype);
    goto fail;
  }

  /* success */
  status = RPC_S_OK;

fail:
  if (hdr) {
    RPCRT4_FreeHeader(hdr);
  }
  RPCRT4_CloseBinding(bind, conn);
  return status;
}

/***********************************************************************
 *           I_RpcSendReceive [RPCRT4.@]
 */
RPC_STATUS WINAPI I_RpcSendReceive(PRPC_MESSAGE pMsg)
{
  RPC_STATUS status;

  TRACE("(%p)\n", pMsg);
  status = I_RpcSend(pMsg);
  if (status == RPC_S_OK)
    status = I_RpcReceive(pMsg);
  return status;
}
