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
#include <stdlib.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winuser.h"

#include "rpc.h"
#include "rpcndr.h"
#include "rpcdcep.h"

#include "wine/debug.h"

#include "rpc_binding.h"
#include "rpc_defs.h"
#include "rpc_message.h"
#include "rpc_assoc.h"
#include "ncastatus.h"

WINE_DEFAULT_DEBUG_CHANNEL(rpc);

/* note: the DCE/RPC spec says the alignment amount should be 4, but
 * MS/RPC servers seem to always use 16 */
#define AUTH_ALIGNMENT 16

/* gets the amount needed to round a value up to the specified alignment */
#define ROUND_UP_AMOUNT(value, alignment) \
    (((alignment) - (((value) % (alignment)))) % (alignment))
#define ROUND_UP(value, alignment) (((value) + ((alignment) - 1)) & ~((alignment)-1))

static RPC_STATUS I_RpcReAllocateBuffer(PRPC_MESSAGE pMsg);

DWORD RPCRT4_GetHeaderSize(const RpcPktHdr *Header)
{
  static const DWORD header_sizes[] = {
    sizeof(Header->request), 0, sizeof(Header->response),
    sizeof(Header->fault), 0, 0, 0, 0, 0, 0, 0, sizeof(Header->bind),
    sizeof(Header->bind_ack), sizeof(Header->bind_nack),
    0, 0, sizeof(Header->auth3), 0, 0, 0, sizeof(Header->http)
  };
  ULONG ret = 0;

  if (Header->common.ptype < ARRAY_SIZE(header_sizes)) {
    ret = header_sizes[Header->common.ptype];
    if (ret == 0)
      FIXME("unhandled packet type %u\n", Header->common.ptype);
    if (Header->common.flags & RPC_FLG_OBJECT_UUID)
      ret += sizeof(UUID);
  } else {
    WARN("invalid packet type %u\n", Header->common.ptype);
  }

  return ret;
}

static BOOL packet_has_body(const RpcPktHdr *Header)
{
    return (Header->common.ptype == PKT_FAULT) ||
           (Header->common.ptype == PKT_REQUEST) ||
           (Header->common.ptype == PKT_RESPONSE);
}

static BOOL packet_has_auth_verifier(const RpcPktHdr *Header)
{
    return !(Header->common.ptype == PKT_BIND_NACK) &&
           !(Header->common.ptype == PKT_SHUTDOWN);
}

static BOOL packet_does_auth_negotiation(const RpcPktHdr *Header)
{
    switch (Header->common.ptype)
    {
    case PKT_BIND:
    case PKT_BIND_ACK:
    case PKT_AUTH3:
    case PKT_ALTER_CONTEXT:
    case PKT_ALTER_CONTEXT_RESP:
        return TRUE;
    default:
        return FALSE;
    }
}

static VOID RPCRT4_BuildCommonHeader(RpcPktCommonHdr *Header, unsigned char PacketType,
                                     ULONG DataRepresentation)
{
  Header->rpc_ver = RPC_VER_MAJOR;
  Header->rpc_ver_minor = RPC_VER_MINOR;
  Header->ptype = PacketType;
  Header->drep[0] = LOBYTE(LOWORD(DataRepresentation));
  Header->drep[1] = HIBYTE(LOWORD(DataRepresentation));
  Header->drep[2] = LOBYTE(HIWORD(DataRepresentation));
  Header->drep[3] = HIBYTE(HIWORD(DataRepresentation));
  Header->auth_len = 0;
  Header->call_id = 1;
  Header->flags = 0;
  /* Flags and fragment length are computed in RPCRT4_Send. */
}

static RpcPktHdr *RPCRT4_BuildRequestHeader(ULONG DataRepresentation,
                                     ULONG BufferLength,
                                     unsigned short ProcNum,
                                     UUID *ObjectUuid)
{
  RpcPktHdr *header;
  BOOL has_object;
  RPC_STATUS status;

  has_object = (ObjectUuid != NULL && !UuidIsNil(ObjectUuid, &status));
  header = calloc(1, sizeof(header->request) + (has_object ? sizeof(UUID) : 0));
  if (header == NULL) {
    return NULL;
  }

  RPCRT4_BuildCommonHeader(&header->common, PKT_REQUEST, DataRepresentation);
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

RpcPktHdr *RPCRT4_BuildResponseHeader(ULONG DataRepresentation, ULONG BufferLength)
{
  RpcPktResponseHdr *header;

  header = calloc(1, sizeof(*header));
  if (header == NULL) {
    return NULL;
  }

  RPCRT4_BuildCommonHeader(&header->common, PKT_RESPONSE, DataRepresentation);
  header->common.frag_len = sizeof(*header);
  header->alloc_hint = BufferLength;

  return (RpcPktHdr *)header;
}

RpcPktHdr *RPCRT4_BuildFaultHeader(ULONG DataRepresentation, RPC_STATUS Status)
{
  RpcPktFaultHdr *header;

  header = calloc(1, sizeof(*header));
  if (header == NULL) {
    return NULL;
  }

  RPCRT4_BuildCommonHeader(&header->common, PKT_FAULT, DataRepresentation);
  header->common.frag_len = sizeof(*header);
  header->status = Status;

  return (RpcPktHdr *)header;
}

RpcPktHdr *RPCRT4_BuildBindHeader(ULONG DataRepresentation,
                                  unsigned short MaxTransmissionSize,
                                  unsigned short MaxReceiveSize,
                                  ULONG  AssocGroupId,
                                  const RPC_SYNTAX_IDENTIFIER *AbstractId,
                                  const RPC_SYNTAX_IDENTIFIER *TransferId)
{
  RpcPktHdr *header;
  RpcContextElement *ctxt_elem;

  header = calloc(1, sizeof(header->bind) + FIELD_OFFSET(RpcContextElement, transfer_syntaxes[1]));
  if (header == NULL) {
    return NULL;
  }
  ctxt_elem = (RpcContextElement *)(&header->bind + 1);

  RPCRT4_BuildCommonHeader(&header->common, PKT_BIND, DataRepresentation);
  header->common.frag_len = sizeof(header->bind) + FIELD_OFFSET(RpcContextElement, transfer_syntaxes[1]);
  header->bind.max_tsize = MaxTransmissionSize;
  header->bind.max_rsize = MaxReceiveSize;
  header->bind.assoc_gid = AssocGroupId;
  header->bind.num_elements = 1;
  ctxt_elem->num_syntaxes = 1;
  ctxt_elem->abstract_syntax = *AbstractId;
  ctxt_elem->transfer_syntaxes[0] = *TransferId;

  return header;
}

static RpcPktHdr *RPCRT4_BuildAuthHeader(ULONG DataRepresentation)
{
  RpcPktAuth3Hdr *header;

  header = calloc(1, sizeof(*header));
  if (header == NULL)
    return NULL;

  RPCRT4_BuildCommonHeader(&header->common, PKT_AUTH3, DataRepresentation);
  header->common.frag_len = sizeof(*header);

  return (RpcPktHdr*)header;
}

RpcPktHdr *RPCRT4_BuildBindNackHeader(ULONG DataRepresentation,
                                      unsigned char RpcVersion,
                                      unsigned char RpcVersionMinor,
                                      unsigned short RejectReason)
{
  RpcPktBindNAckHdr *header;
  C_ASSERT(sizeof(*header) >= FIELD_OFFSET(RpcPktBindNAckHdr, protocols[1]));

  header = calloc(1, sizeof(*header));
  if (header == NULL) {
    return NULL;
  }

  RPCRT4_BuildCommonHeader(&header->common, PKT_BIND_NACK, DataRepresentation);
  header->common.frag_len = FIELD_OFFSET(RpcPktBindNAckHdr, protocols[1]);
  header->reject_reason = RejectReason;
  header->protocols_count = 1;
  header->protocols[0].rpc_ver = RpcVersion;
  header->protocols[0].rpc_ver_minor = RpcVersionMinor;

  return (RpcPktHdr *)header;
}

RpcPktHdr *RPCRT4_BuildBindAckHeader(ULONG DataRepresentation,
                                     unsigned short MaxTransmissionSize,
                                     unsigned short MaxReceiveSize,
                                     ULONG AssocGroupId,
                                     LPCSTR ServerAddress,
                                     unsigned char ResultCount,
                                     const RpcResult *Results)
{
  RpcPktHdr *header;
  ULONG header_size;
  RpcAddressString *server_address;
  RpcResultList *results;

  header_size = sizeof(header->bind_ack) +
                ROUND_UP(FIELD_OFFSET(RpcAddressString, string[strlen(ServerAddress) + 1]), 4) +
                FIELD_OFFSET(RpcResultList, results[ResultCount]);

  header = calloc(1, header_size);
  if (header == NULL) {
    return NULL;
  }

  RPCRT4_BuildCommonHeader(&header->common, PKT_BIND_ACK, DataRepresentation);
  header->common.frag_len = header_size;
  header->bind_ack.max_tsize = MaxTransmissionSize;
  header->bind_ack.max_rsize = MaxReceiveSize;
  header->bind_ack.assoc_gid = AssocGroupId;
  server_address = (RpcAddressString*)(&header->bind_ack + 1);
  server_address->length = strlen(ServerAddress) + 1;
  strcpy(server_address->string, ServerAddress);
  /* results is 4-byte aligned */
  results = (RpcResultList*)((ULONG_PTR)server_address + ROUND_UP(FIELD_OFFSET(RpcAddressString, string[server_address->length]), 4));
  results->num_results = ResultCount;
  memcpy(&results->results[0], Results, ResultCount * sizeof(*Results));

  return header;
}

RpcPktHdr *RPCRT4_BuildHttpHeader(ULONG DataRepresentation,
                                  unsigned short flags,
                                  unsigned short num_data_items,
                                  unsigned int payload_size)
{
  RpcPktHdr *header;

  header = calloc(1, sizeof(header->http) + payload_size);
  if (header == NULL) {
      ERR("failed to allocate memory\n");
    return NULL;
  }

  RPCRT4_BuildCommonHeader(&header->common, PKT_HTTP, DataRepresentation);
  /* since the packet isn't current sent using RPCRT4_Send, set the flags
   * manually here */
  header->common.flags = RPC_FLG_FIRST|RPC_FLG_LAST;
  header->common.call_id = 0;
  header->common.frag_len = sizeof(header->http) + payload_size;
  header->http.flags = flags;
  header->http.num_data_items = num_data_items;

  return header;
}

#define WRITE_HTTP_PAYLOAD_FIELD_UINT32(payload, type, value) \
    do { \
        *(unsigned int *)(payload) = (type); \
        (payload) += 4; \
        *(unsigned int *)(payload) = (value); \
        (payload) += 4; \
    } while (0)

#define WRITE_HTTP_PAYLOAD_FIELD_UUID(payload, type, uuid) \
    do { \
        *(unsigned int *)(payload) = (type); \
        (payload) += 4; \
        *(UUID *)(payload) = (uuid); \
        (payload) += sizeof(UUID); \
    } while (0)

#define WRITE_HTTP_PAYLOAD_FIELD_FLOW_CONTROL(payload, bytes_transmitted, flow_control_increment, uuid) \
    do { \
        *(unsigned int *)(payload) = 0x00000001; \
        (payload) += 4; \
        *(unsigned int *)(payload) = (bytes_transmitted); \
        (payload) += 4; \
        *(unsigned int *)(payload) = (flow_control_increment); \
        (payload) += 4; \
        *(UUID *)(payload) = (uuid); \
        (payload) += sizeof(UUID); \
    } while (0)

RpcPktHdr *RPCRT4_BuildHttpConnectHeader(int out_pipe,
                                         const UUID *connection_uuid,
                                         const UUID *pipe_uuid,
                                         const UUID *association_uuid)
{
  RpcPktHdr *header;
  unsigned int size;
  char *payload;

  size = 8 + 4 + sizeof(UUID) + 4 + sizeof(UUID) + 8;
  if (!out_pipe)
    size += 8 + 4 + sizeof(UUID);

  header = RPCRT4_BuildHttpHeader(NDR_LOCAL_DATA_REPRESENTATION, 0,
                                  out_pipe ? 4 : 6, size);
  if (!header) return NULL;
  payload = (char *)(&header->http+1);

  /* FIXME: what does this part of the payload do? */
  WRITE_HTTP_PAYLOAD_FIELD_UINT32(payload, 0x00000006, 0x00000001);

  WRITE_HTTP_PAYLOAD_FIELD_UUID(payload, 0x00000003, *connection_uuid);
  WRITE_HTTP_PAYLOAD_FIELD_UUID(payload, 0x00000003, *pipe_uuid);

  if (out_pipe)
    /* FIXME: what does this part of the payload do? */
    WRITE_HTTP_PAYLOAD_FIELD_UINT32(payload, 0x00000000, 0x00010000);
  else
  {
    /* FIXME: what does this part of the payload do? */
    WRITE_HTTP_PAYLOAD_FIELD_UINT32(payload, 0x00000004, 0x40000000);
    /* FIXME: what does this part of the payload do? */
    WRITE_HTTP_PAYLOAD_FIELD_UINT32(payload, 0x00000005, 0x000493e0);

    WRITE_HTTP_PAYLOAD_FIELD_UUID(payload, 0x0000000c, *association_uuid);
  }

  return header;
}

RpcPktHdr *RPCRT4_BuildHttpFlowControlHeader(BOOL server, ULONG bytes_transmitted,
                                             ULONG flow_control_increment,
                                             const UUID *pipe_uuid)
{
  RpcPktHdr *header;
  char *payload;

  header = RPCRT4_BuildHttpHeader(NDR_LOCAL_DATA_REPRESENTATION, 0x2, 2,
                                  5 * sizeof(ULONG) + sizeof(UUID));
  if (!header) return NULL;
  payload = (char *)(&header->http+1);

  WRITE_HTTP_PAYLOAD_FIELD_UINT32(payload, 0x0000000d, (server ? 0x0 : 0x3));

  WRITE_HTTP_PAYLOAD_FIELD_FLOW_CONTROL(payload, bytes_transmitted,
                                        flow_control_increment, *pipe_uuid);
  return header;
}

NCA_STATUS RPC2NCA_STATUS(RPC_STATUS status)
{
    switch (status)
    {
    case ERROR_INVALID_HANDLE:              return NCA_S_FAULT_CONTEXT_MISMATCH;
    case ERROR_OUTOFMEMORY:                 return NCA_S_FAULT_REMOTE_NO_MEMORY;
    case RPC_S_NOT_LISTENING:               return NCA_S_SERVER_TOO_BUSY;
    case RPC_S_UNKNOWN_IF:                  return NCA_S_UNK_IF;
    case RPC_S_SERVER_TOO_BUSY:             return NCA_S_SERVER_TOO_BUSY;
    case RPC_S_CALL_FAILED:                 return NCA_S_FAULT_UNSPEC;
    case RPC_S_CALL_FAILED_DNE:             return NCA_S_MANAGER_NOT_ENTERED;
    case RPC_S_PROTOCOL_ERROR:              return NCA_S_PROTO_ERROR;
    case RPC_S_UNSUPPORTED_TYPE:            return NCA_S_UNSUPPORTED_TYPE;
    case RPC_S_INVALID_TAG:                 return NCA_S_FAULT_INVALID_TAG;
    case RPC_S_INVALID_BOUND:               return NCA_S_FAULT_INVALID_BOUND;
    case RPC_S_PROCNUM_OUT_OF_RANGE:        return NCA_S_OP_RNG_ERROR;
    case RPC_X_SS_HANDLES_MISMATCH:         return NCA_S_FAULT_CONTEXT_MISMATCH;
    case RPC_S_CALL_CANCELLED:              return NCA_S_FAULT_CANCEL;
    case RPC_S_COMM_FAILURE:                return NCA_S_COMM_FAILURE;
    case RPC_X_WRONG_PIPE_ORDER:            return NCA_S_FAULT_PIPE_ORDER;
    case RPC_X_PIPE_CLOSED:                 return NCA_S_FAULT_PIPE_CLOSED;
    case RPC_X_PIPE_DISCIPLINE_ERROR:       return NCA_S_FAULT_PIPE_DISCIPLINE;
    case RPC_X_PIPE_EMPTY:                  return NCA_S_FAULT_PIPE_EMPTY;
    case STATUS_FLOAT_DIVIDE_BY_ZERO:       return NCA_S_FAULT_FP_DIV_ZERO;
    case STATUS_FLOAT_INVALID_OPERATION:    return NCA_S_FAULT_FP_ERROR;
    case STATUS_FLOAT_OVERFLOW:             return NCA_S_FAULT_FP_OVERFLOW;
    case STATUS_FLOAT_UNDERFLOW:            return NCA_S_FAULT_FP_UNDERFLOW;
    case STATUS_INTEGER_DIVIDE_BY_ZERO:     return NCA_S_FAULT_INT_DIV_BY_ZERO;
    case STATUS_INTEGER_OVERFLOW:           return NCA_S_FAULT_INT_OVERFLOW;
    default:                                return status;
    }
}

static RPC_STATUS NCA2RPC_STATUS(NCA_STATUS status)
{
    switch (status)
    {
    case NCA_S_COMM_FAILURE:            return RPC_S_COMM_FAILURE;
    case NCA_S_OP_RNG_ERROR:            return RPC_S_PROCNUM_OUT_OF_RANGE;
    case NCA_S_UNK_IF:                  return RPC_S_UNKNOWN_IF;
    case NCA_S_YOU_CRASHED:             return RPC_S_CALL_FAILED;
    case NCA_S_PROTO_ERROR:             return RPC_S_PROTOCOL_ERROR;
    case NCA_S_OUT_ARGS_TOO_BIG:        return ERROR_NOT_ENOUGH_SERVER_MEMORY;
    case NCA_S_SERVER_TOO_BUSY:         return RPC_S_SERVER_TOO_BUSY;
    case NCA_S_UNSUPPORTED_TYPE:        return RPC_S_UNSUPPORTED_TYPE;
    case NCA_S_FAULT_INT_DIV_BY_ZERO:   return RPC_S_ZERO_DIVIDE;
    case NCA_S_FAULT_ADDR_ERROR:        return RPC_S_ADDRESS_ERROR;
    case NCA_S_FAULT_FP_DIV_ZERO:       return RPC_S_FP_DIV_ZERO;
    case NCA_S_FAULT_FP_UNDERFLOW:      return RPC_S_FP_UNDERFLOW;
    case NCA_S_FAULT_FP_OVERFLOW:       return RPC_S_FP_OVERFLOW;
    case NCA_S_FAULT_INVALID_TAG:       return RPC_S_INVALID_TAG;
    case NCA_S_FAULT_INVALID_BOUND:     return RPC_S_INVALID_BOUND;
    case NCA_S_RPC_VERSION_MISMATCH:    return RPC_S_PROTOCOL_ERROR;
    case NCA_S_UNSPEC_REJECT:           return RPC_S_CALL_FAILED_DNE;
    case NCA_S_BAD_ACTID:               return RPC_S_CALL_FAILED_DNE;
    case NCA_S_WHO_ARE_YOU_FAILED:      return RPC_S_CALL_FAILED;
    case NCA_S_MANAGER_NOT_ENTERED:     return RPC_S_CALL_FAILED_DNE;
    case NCA_S_FAULT_CANCEL:            return RPC_S_CALL_CANCELLED;
    case NCA_S_FAULT_ILL_INST:          return RPC_S_ADDRESS_ERROR;
    case NCA_S_FAULT_FP_ERROR:          return RPC_S_FP_OVERFLOW;
    case NCA_S_FAULT_INT_OVERFLOW:      return RPC_S_ADDRESS_ERROR;
    case NCA_S_FAULT_UNSPEC:            return RPC_S_CALL_FAILED;
    case NCA_S_FAULT_PIPE_EMPTY:        return RPC_X_PIPE_EMPTY;
    case NCA_S_FAULT_PIPE_CLOSED:       return RPC_X_PIPE_CLOSED;
    case NCA_S_FAULT_PIPE_ORDER:        return RPC_X_WRONG_PIPE_ORDER;
    case NCA_S_FAULT_PIPE_DISCIPLINE:   return RPC_X_PIPE_DISCIPLINE_ERROR;
    case NCA_S_FAULT_PIPE_COMM_ERROR:   return RPC_S_COMM_FAILURE;
    case NCA_S_FAULT_PIPE_MEMORY:       return ERROR_OUTOFMEMORY;
    case NCA_S_FAULT_CONTEXT_MISMATCH:  return ERROR_INVALID_HANDLE;
    case NCA_S_FAULT_REMOTE_NO_MEMORY:  return ERROR_NOT_ENOUGH_SERVER_MEMORY;
    default:                            return status;
    }
}

/* assumes the common header fields have already been validated */
BOOL RPCRT4_IsValidHttpPacket(RpcPktHdr *hdr, unsigned char *data,
                              unsigned short data_len)
{
  unsigned short i;
  BYTE *p = data;

  for (i = 0; i < hdr->http.num_data_items; i++)
  {
    ULONG type;

    if (data_len < sizeof(ULONG))
      return FALSE;

    type = *(ULONG *)p;
    p += sizeof(ULONG);
    data_len -= sizeof(ULONG);

    switch (type)
    {
      case 0x3:
      case 0xc:
        if (data_len < sizeof(GUID))
          return FALSE;
        p += sizeof(GUID);
        data_len -= sizeof(GUID);
        break;
      case 0x0:
      case 0x2:
      case 0x4:
      case 0x5:
      case 0x6:
      case 0xd:
        if (data_len < sizeof(ULONG))
          return FALSE;
        p += sizeof(ULONG);
        data_len -= sizeof(ULONG);
        break;
      case 0x1:
        if (data_len < 24)
          return FALSE;
        p += 24;
        data_len -= 24;
        break;
      default:
        FIXME("unimplemented type 0x%lx\n", type);
        break;
    }
  }
  return TRUE;
}

/* assumes the HTTP packet has been validated */
static unsigned char *RPCRT4_NextHttpHeaderField(unsigned char *data)
{
  ULONG type;

  type = *(ULONG *)data;
  data += sizeof(ULONG);

  switch (type)
  {
    case 0x3:
    case 0xc:
      return data + sizeof(GUID);
    case 0x0:
    case 0x2:
    case 0x4:
    case 0x5:
    case 0x6:
    case 0xd:
      return data + sizeof(ULONG);
    case 0x1:
      return data + 24;
    default:
      FIXME("unimplemented type 0x%lx\n", type);
      return data;
  }
}

#define READ_HTTP_PAYLOAD_FIELD_TYPE(data) *(ULONG *)(data)
#define GET_HTTP_PAYLOAD_FIELD_DATA(data) ((data) + sizeof(ULONG))

/* assumes the HTTP packet has been validated */
RPC_STATUS RPCRT4_ParseHttpPrepareHeader1(RpcPktHdr *header,
                                          unsigned char *data, ULONG *field1)
{
  ULONG type;
  if (header->http.flags != 0x0)
  {
    ERR("invalid flags 0x%x\n", header->http.flags);
    return RPC_S_PROTOCOL_ERROR;
  }
  if (header->http.num_data_items != 1)
  {
    ERR("invalid number of data items %d\n", header->http.num_data_items);
    return RPC_S_PROTOCOL_ERROR;
  }
  type = READ_HTTP_PAYLOAD_FIELD_TYPE(data);
  if (type != 0x00000002)
  {
    ERR("invalid type 0x%08lx\n", type);
    return RPC_S_PROTOCOL_ERROR;
  }
  *field1 = *(ULONG *)GET_HTTP_PAYLOAD_FIELD_DATA(data);
  return RPC_S_OK;
}

/* assumes the HTTP packet has been validated */
RPC_STATUS RPCRT4_ParseHttpPrepareHeader2(RpcPktHdr *header,
                                          unsigned char *data, ULONG *field1,
                                          ULONG *bytes_until_next_packet,
                                          ULONG *field3)
{
  ULONG type;
  if (header->http.flags != 0x0)
  {
    ERR("invalid flags 0x%x\n", header->http.flags);
    return RPC_S_PROTOCOL_ERROR;
  }
  if (header->http.num_data_items != 3)
  {
    ERR("invalid number of data items %d\n", header->http.num_data_items);
    return RPC_S_PROTOCOL_ERROR;
  }

  type = READ_HTTP_PAYLOAD_FIELD_TYPE(data);
  if (type != 0x00000006)
  {
    ERR("invalid type for field 1: 0x%08lx\n", type);
    return RPC_S_PROTOCOL_ERROR;
  }
  *field1 = *(ULONG *)GET_HTTP_PAYLOAD_FIELD_DATA(data);
  data = RPCRT4_NextHttpHeaderField(data);

  type = READ_HTTP_PAYLOAD_FIELD_TYPE(data);
  if (type != 0x00000000)
  {
    ERR("invalid type for field 2: 0x%08lx\n", type);
    return RPC_S_PROTOCOL_ERROR;
  }
  *bytes_until_next_packet = *(ULONG *)GET_HTTP_PAYLOAD_FIELD_DATA(data);
  data = RPCRT4_NextHttpHeaderField(data);

  type = READ_HTTP_PAYLOAD_FIELD_TYPE(data);
  if (type != 0x00000002)
  {
    ERR("invalid type for field 3: 0x%08lx\n", type);
    return RPC_S_PROTOCOL_ERROR;
  }
  *field3 = *(ULONG *)GET_HTTP_PAYLOAD_FIELD_DATA(data);

  return RPC_S_OK;
}

RPC_STATUS RPCRT4_ParseHttpFlowControlHeader(RpcPktHdr *header,
                                             unsigned char *data, BOOL server,
                                             ULONG *bytes_transmitted,
                                             ULONG *flow_control_increment,
                                             UUID *pipe_uuid)
{
  ULONG type;
  if (header->http.flags != 0x2)
  {
    ERR("invalid flags 0x%x\n", header->http.flags);
    return RPC_S_PROTOCOL_ERROR;
  }
  if (header->http.num_data_items != 2)
  {
    ERR("invalid number of data items %d\n", header->http.num_data_items);
    return RPC_S_PROTOCOL_ERROR;
  }

  type = READ_HTTP_PAYLOAD_FIELD_TYPE(data);
  if (type != 0x0000000d)
  {
    ERR("invalid type for field 1: 0x%08lx\n", type);
    return RPC_S_PROTOCOL_ERROR;
  }
  if (*(ULONG *)GET_HTTP_PAYLOAD_FIELD_DATA(data) != (server ? 0x3 : 0x0))
  {
    ERR("invalid type for 0xd field data: 0x%08lx\n", *(ULONG *)GET_HTTP_PAYLOAD_FIELD_DATA(data));
    return RPC_S_PROTOCOL_ERROR;
  }
  data = RPCRT4_NextHttpHeaderField(data);

  type = READ_HTTP_PAYLOAD_FIELD_TYPE(data);
  if (type != 0x00000001)
  {
    ERR("invalid type for field 2: 0x%08lx\n", type);
    return RPC_S_PROTOCOL_ERROR;
  }
  *bytes_transmitted = *(ULONG *)GET_HTTP_PAYLOAD_FIELD_DATA(data);
  *flow_control_increment = *(ULONG *)(GET_HTTP_PAYLOAD_FIELD_DATA(data) + 4);
  *pipe_uuid = *(UUID *)(GET_HTTP_PAYLOAD_FIELD_DATA(data) + 8);

  return RPC_S_OK;
}


RPC_STATUS RPCRT4_default_secure_packet(RpcConnection *Connection,
    enum secure_packet_direction dir,
    RpcPktHdr *hdr, unsigned int hdr_size,
    unsigned char *stub_data, unsigned int stub_data_size,
    RpcAuthVerifier *auth_hdr,
    unsigned char *auth_value, unsigned int auth_value_size)
{
    SecBufferDesc message;
    SecBuffer buffers[4];
    SECURITY_STATUS sec_status;

    message.ulVersion = SECBUFFER_VERSION;
    message.cBuffers = ARRAY_SIZE(buffers);
    message.pBuffers = buffers;

    buffers[0].cbBuffer = hdr_size;
    buffers[0].BufferType = SECBUFFER_DATA|SECBUFFER_READONLY_WITH_CHECKSUM;
    buffers[0].pvBuffer = hdr;
    buffers[1].cbBuffer = stub_data_size;
    buffers[1].BufferType = SECBUFFER_DATA;
    buffers[1].pvBuffer = stub_data;
    buffers[2].cbBuffer = sizeof(*auth_hdr);
    buffers[2].BufferType = SECBUFFER_DATA|SECBUFFER_READONLY_WITH_CHECKSUM;
    buffers[2].pvBuffer = auth_hdr;
    buffers[3].cbBuffer = auth_value_size;
    buffers[3].BufferType = SECBUFFER_TOKEN;
    buffers[3].pvBuffer = auth_value;

    if (dir == SECURE_PACKET_SEND)
    {
        if ((auth_hdr->auth_level == RPC_C_AUTHN_LEVEL_PKT_PRIVACY) && packet_has_body(hdr))
        {
            sec_status = EncryptMessage(&Connection->ctx, 0, &message, 0 /* FIXME */);
            if (sec_status != SEC_E_OK)
            {
                ERR("EncryptMessage failed with 0x%08lx\n", sec_status);
                return RPC_S_SEC_PKG_ERROR;
            }
        }
        else if (auth_hdr->auth_level != RPC_C_AUTHN_LEVEL_NONE)
        {
            sec_status = MakeSignature(&Connection->ctx, 0, &message, 0 /* FIXME */);
            if (sec_status != SEC_E_OK)
            {
                ERR("MakeSignature failed with 0x%08lx\n", sec_status);
                return RPC_S_SEC_PKG_ERROR;
            }
        }
    }
    else if (dir == SECURE_PACKET_RECEIVE)
    {
        if ((auth_hdr->auth_level == RPC_C_AUTHN_LEVEL_PKT_PRIVACY) && packet_has_body(hdr))
        {
            sec_status = DecryptMessage(&Connection->ctx, &message, 0 /* FIXME */, 0);
            if (sec_status != SEC_E_OK)
            {
                ERR("DecryptMessage failed with 0x%08lx\n", sec_status);
                return RPC_S_SEC_PKG_ERROR;
            }
        }
        else if (auth_hdr->auth_level != RPC_C_AUTHN_LEVEL_NONE)
        {
            sec_status = VerifySignature(&Connection->ctx, &message, 0 /* FIXME */, NULL);
            if (sec_status != SEC_E_OK)
            {
                ERR("VerifySignature failed with 0x%08lx\n", sec_status);
                return RPC_S_SEC_PKG_ERROR;
            }
        }
    }

    return RPC_S_OK;
}
         
/***********************************************************************
 *           RPCRT4_SendWithAuth (internal)
 * 
 * Transmit a packet with authorization data over connection in acceptable fragments.
 */
RPC_STATUS RPCRT4_SendWithAuth(RpcConnection *Connection, RpcPktHdr *Header,
                               void *Buffer, unsigned int BufferLength,
                               const void *Auth, unsigned int AuthLength)
{
  PUCHAR buffer_pos;
  DWORD hdr_size;
  LONG count;
  unsigned char *pkt;
  LONG alen;
  RPC_STATUS status;

  RPCRT4_SetThreadCurrentConnection(Connection);

  buffer_pos = Buffer;
  /* The packet building functions save the packet header size, so we can use it. */
  hdr_size = Header->common.frag_len;
  if (AuthLength)
    Header->common.auth_len = AuthLength;
  else if (Connection->AuthInfo && packet_has_auth_verifier(Header))
  {
    if ((Connection->AuthInfo->AuthnLevel == RPC_C_AUTHN_LEVEL_PKT_PRIVACY) && packet_has_body(Header))
      Header->common.auth_len = Connection->encryption_auth_len;
    else
      Header->common.auth_len = Connection->signature_auth_len;
  }
  else
    Header->common.auth_len = 0;
  Header->common.flags |= RPC_FLG_FIRST;
  Header->common.flags &= ~RPC_FLG_LAST;

  alen = RPC_AUTH_VERIFIER_LEN(&Header->common);

  while (!(Header->common.flags & RPC_FLG_LAST)) {
    unsigned char auth_pad_len = Header->common.auth_len ? ROUND_UP_AMOUNT(BufferLength, AUTH_ALIGNMENT) : 0;
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

    pkt = calloc(1, Header->common.frag_len);

    memcpy(pkt, Header, hdr_size);

    /* fragment consisted of header only and is the last one */
    if (hdr_size == Header->common.frag_len)
      goto write;

    memcpy(pkt + hdr_size, buffer_pos, Header->common.frag_len - hdr_size - auth_pad_len - alen);

    /* add the authorization info */
    if (Header->common.auth_len)
    {
      RpcAuthVerifier *auth_hdr = (RpcAuthVerifier *)&pkt[Header->common.frag_len - alen];

      auth_hdr->auth_type = Connection->AuthInfo->AuthnSvc;
      auth_hdr->auth_level = Connection->AuthInfo->AuthnLevel;
      auth_hdr->auth_pad_length = auth_pad_len;
      auth_hdr->auth_reserved = 0;
      /* a unique number... */
      auth_hdr->auth_context_id = Connection->auth_context_id;

      if (AuthLength)
        memcpy(auth_hdr + 1, Auth, AuthLength);
      else
      {
        status = rpcrt4_conn_secure_packet(Connection, SECURE_PACKET_SEND,
            (RpcPktHdr *)pkt, hdr_size,
            pkt + hdr_size, Header->common.frag_len - hdr_size - alen,
            auth_hdr,
            (unsigned char *)(auth_hdr + 1), Header->common.auth_len);
        if (status != RPC_S_OK)
        {
          free(pkt);
          RPCRT4_SetThreadCurrentConnection(NULL);
          return status;
        }
      }
    }

write:
    count = rpcrt4_conn_write(Connection, pkt, Header->common.frag_len);
    free(pkt);
    if (count<0) {
      WARN("rpcrt4_conn_write failed (auth)\n");
      RPCRT4_SetThreadCurrentConnection(NULL);
      return RPC_S_CALL_FAILED;
    }

    buffer_pos += Header->common.frag_len - hdr_size - alen - auth_pad_len;
    BufferLength -= Header->common.frag_len - hdr_size - alen - auth_pad_len;
    Header->common.flags &= ~RPC_FLG_FIRST;
  }

  RPCRT4_SetThreadCurrentConnection(NULL);
  return RPC_S_OK;
}

/***********************************************************************
 *           RPCRT4_default_authorize (internal)
 *
 * Authorize a client connection.
 */
RPC_STATUS RPCRT4_default_authorize(RpcConnection *conn, BOOL first_time,
                                    unsigned char *in_buffer,
                                    unsigned int in_size,
                                    unsigned char *out_buffer,
                                    unsigned int *out_size)
{
  SECURITY_STATUS r;
  SecBufferDesc out_desc;
  SecBufferDesc inp_desc;
  SecPkgContext_Sizes secctx_sizes;
  BOOL continue_needed;
  ULONG context_req;
  SecBuffer in, out;

  if (!out_buffer)
  {
    *out_size = conn->AuthInfo->cbMaxToken;
    return RPC_S_OK;
  }

  in.BufferType = SECBUFFER_TOKEN;
  in.pvBuffer = in_buffer;
  in.cbBuffer = in_size;

  out.BufferType = SECBUFFER_TOKEN;
  out.pvBuffer = out_buffer;
  out.cbBuffer = *out_size;

  out_desc.ulVersion = 0;
  out_desc.cBuffers = 1;
  out_desc.pBuffers = &out;

  inp_desc.ulVersion = 0;
  inp_desc.cBuffers = 1;
  inp_desc.pBuffers = &in;

  if (conn->server)
  {
      context_req = ASC_REQ_CONNECTION | ASC_REQ_USE_DCE_STYLE |
                    ASC_REQ_DELEGATE;

      if (conn->AuthInfo->AuthnLevel == RPC_C_AUTHN_LEVEL_PKT_INTEGRITY)
          context_req |= ASC_REQ_INTEGRITY;
      else if (conn->AuthInfo->AuthnLevel == RPC_C_AUTHN_LEVEL_PKT_PRIVACY)
          context_req |= ASC_REQ_CONFIDENTIALITY | ASC_REQ_INTEGRITY;

      r = AcceptSecurityContext(&conn->AuthInfo->cred,
                                first_time ? NULL : &conn->ctx,
                                &inp_desc, context_req, SECURITY_NETWORK_DREP,
                                &conn->ctx,
                                &out_desc, &conn->attr, &conn->exp);
      if (r == SEC_E_OK || r == SEC_I_COMPLETE_NEEDED)
      {
          /* authorisation done, so nothing more to send */
          out.cbBuffer = 0;
      }
  }
  else
  {
      context_req = ISC_REQ_CONNECTION | ISC_REQ_USE_DCE_STYLE |
                    ISC_REQ_MUTUAL_AUTH | ISC_REQ_DELEGATE;

      if (conn->AuthInfo->AuthnLevel == RPC_C_AUTHN_LEVEL_PKT_INTEGRITY)
          context_req |= ISC_REQ_INTEGRITY;
      else if (conn->AuthInfo->AuthnLevel == RPC_C_AUTHN_LEVEL_PKT_PRIVACY)
          context_req |= ISC_REQ_CONFIDENTIALITY | ISC_REQ_INTEGRITY;

      r = InitializeSecurityContextW(&conn->AuthInfo->cred,
                                     first_time ? NULL: &conn->ctx,
                                     first_time ? conn->AuthInfo->server_principal_name : NULL,
                                     context_req, 0, SECURITY_NETWORK_DREP,
                                     first_time ? NULL : &inp_desc, 0, &conn->ctx,
                                     &out_desc, &conn->attr, &conn->exp);
  }
  if (FAILED(r))
  {
      WARN("InitializeSecurityContext failed with error 0x%08lx\n", r);
      goto failed;
  }

  TRACE("r = 0x%08lx, attr = 0x%08lx\n", r, conn->attr);
  continue_needed = ((r == SEC_I_CONTINUE_NEEDED) ||
                     (r == SEC_I_COMPLETE_AND_CONTINUE));

  if ((r == SEC_I_COMPLETE_NEEDED) || (r == SEC_I_COMPLETE_AND_CONTINUE))
  {
      TRACE("complete needed\n");
      r = CompleteAuthToken(&conn->ctx, &out_desc);
      if (FAILED(r))
      {
          WARN("CompleteAuthToken failed with error 0x%08lx\n", r);
          goto failed;
      }
  }

  TRACE("cbBuffer = %ld\n", out.cbBuffer);

  if (!continue_needed)
  {
      r = QueryContextAttributesA(&conn->ctx, SECPKG_ATTR_SIZES, &secctx_sizes);
      if (FAILED(r))
      {
          WARN("QueryContextAttributes failed with error 0x%08lx\n", r);
          goto failed;
      }
      conn->signature_auth_len = secctx_sizes.cbMaxSignature;
      conn->encryption_auth_len = secctx_sizes.cbSecurityTrailer;
  }

  *out_size = out.cbBuffer;
  return RPC_S_OK;

failed:
  *out_size = 0;
  return ERROR_ACCESS_DENIED; /* FIXME: is this correct? */
}

/***********************************************************************
 *           RPCRT4_ClientConnectionAuth (internal)
 */
RPC_STATUS RPCRT4_ClientConnectionAuth(RpcConnection* conn, BYTE *challenge,
                                       ULONG count)
{
  RpcPktHdr *resp_hdr;
  RPC_STATUS status;
  unsigned char *out_buffer;
  unsigned int out_len = 0;

  TRACE("challenge %s, %ld bytes\n", challenge, count);

  status = rpcrt4_conn_authorize(conn, FALSE, challenge, count, NULL, &out_len);
  if (status) return status;
  out_buffer = malloc(out_len);
  if (!out_buffer) return RPC_S_OUT_OF_RESOURCES;
  status = rpcrt4_conn_authorize(conn, FALSE, challenge, count, out_buffer, &out_len);
  if (status) return status;

  resp_hdr = RPCRT4_BuildAuthHeader(NDR_LOCAL_DATA_REPRESENTATION);

  if (resp_hdr)
    status = RPCRT4_SendWithAuth(conn, resp_hdr, NULL, 0, out_buffer, out_len);
  else
    status = RPC_S_OUT_OF_RESOURCES;

  free(out_buffer);
  free(resp_hdr);

  return status;
}

/***********************************************************************
 *           RPCRT4_ServerConnectionAuth (internal)
 */
RPC_STATUS RPCRT4_ServerConnectionAuth(RpcConnection* conn,
                                       BOOL start,
                                       RpcAuthVerifier *auth_data_in,
                                       ULONG auth_length_in,
                                       unsigned char **auth_data_out,
                                       ULONG *auth_length_out)
{
    unsigned char *out_buffer;
    unsigned int out_size;
    RPC_STATUS status;

    if (start)
    {
        /* remove any existing authentication information */
        if (conn->AuthInfo)
        {
            RpcAuthInfo_Release(conn->AuthInfo);
            conn->AuthInfo = NULL;
        }
        if (SecIsValidHandle(&conn->ctx))
        {
            DeleteSecurityContext(&conn->ctx);
            SecInvalidateHandle(&conn->ctx);
        }
        if (auth_length_in >= sizeof(RpcAuthVerifier))
        {
            CredHandle cred;
            TimeStamp exp;
            ULONG max_token;

            status = RPCRT4_ServerGetRegisteredAuthInfo(
                auth_data_in->auth_type, &cred, &exp, &max_token);
            if (status != RPC_S_OK)
            {
                ERR("unknown authentication service %u\n", auth_data_in->auth_type);
                return status;
            }

            status = RpcAuthInfo_Create(auth_data_in->auth_level,
                                        auth_data_in->auth_type, cred, exp,
                                        max_token, NULL, &conn->AuthInfo);
            if (status != RPC_S_OK)
            {
                FreeCredentialsHandle(&cred);
                return status;
            }

            /* FIXME: should auth_data_in->auth_context_id be checked in the !start case? */
            conn->auth_context_id = auth_data_in->auth_context_id;
        }
    }

    if (auth_length_in < sizeof(RpcAuthVerifier))
        return RPC_S_OK;

    if (!conn->AuthInfo)
        /* should have filled in authentication info by now */
        return RPC_S_PROTOCOL_ERROR;

    status = rpcrt4_conn_authorize(
        conn, start, (unsigned char *)(auth_data_in + 1),
        auth_length_in - sizeof(RpcAuthVerifier), NULL, &out_size);
    if (status) return status;

    out_buffer = malloc(out_size);
    if (!out_buffer) return RPC_S_OUT_OF_RESOURCES;

    status = rpcrt4_conn_authorize(
        conn, start, (unsigned char *)(auth_data_in + 1),
        auth_length_in - sizeof(RpcAuthVerifier), out_buffer, &out_size);
    if (status != RPC_S_OK)
    {
        free(out_buffer);
        return status;
    }

    if (out_size && !auth_length_out)
    {
        ERR("expected authentication to be complete but SSP returned data of "
            "%u bytes to be sent back to client\n", out_size);
        free(out_buffer);
        return RPC_S_SEC_PKG_ERROR;
    }
    else
    {
        *auth_data_out = out_buffer;
        *auth_length_out = out_size;
    }

    return status;
}

/***********************************************************************
 *           RPCRT4_default_is_authorized (internal)
 *
 * Has a connection started the process of authorizing with the server?
 */
BOOL RPCRT4_default_is_authorized(RpcConnection *Connection)
{
    return Connection->AuthInfo && SecIsValidHandle(&Connection->ctx);
}

/***********************************************************************
 *           RPCRT4_default_impersonate_client (internal)
 *
 */
RPC_STATUS RPCRT4_default_impersonate_client(RpcConnection *conn)
{
    SECURITY_STATUS sec_status;

    TRACE("(%p)\n", conn);

    if (!conn->AuthInfo || !SecIsValidHandle(&conn->ctx))
        return RPC_S_NO_CONTEXT_AVAILABLE;
    sec_status = ImpersonateSecurityContext(&conn->ctx);
    if (sec_status != SEC_E_OK)
        WARN("ImpersonateSecurityContext returned 0x%08lx\n", sec_status);
    switch (sec_status)
    {
    case SEC_E_UNSUPPORTED_FUNCTION:
        return RPC_S_CANNOT_SUPPORT;
    case SEC_E_NO_IMPERSONATION:
        return RPC_S_NO_CONTEXT_AVAILABLE;
    case SEC_E_OK:
        return RPC_S_OK;
    default:
        return RPC_S_SEC_PKG_ERROR;
    }
}

/***********************************************************************
 *           RPCRT4_default_revert_to_self (internal)
 *
 */
RPC_STATUS RPCRT4_default_revert_to_self(RpcConnection *conn)
{
    SECURITY_STATUS sec_status;

    TRACE("(%p)\n", conn);

    if (!conn->AuthInfo || !SecIsValidHandle(&conn->ctx))
        return RPC_S_NO_CONTEXT_AVAILABLE;
    sec_status = RevertSecurityContext(&conn->ctx);
    if (sec_status != SEC_E_OK)
        WARN("RevertSecurityContext returned 0x%08lx\n", sec_status);
    switch (sec_status)
    {
    case SEC_E_UNSUPPORTED_FUNCTION:
        return RPC_S_CANNOT_SUPPORT;
    case SEC_E_NO_IMPERSONATION:
        return RPC_S_NO_CONTEXT_AVAILABLE;
    case SEC_E_OK:
        return RPC_S_OK;
    default:
        return RPC_S_SEC_PKG_ERROR;
    }
}

/***********************************************************************
 *           RPCRT4_default_inquire_auth_client (internal)
 *
 * Default function to retrieve the authentication details that the client
 * is using to call the server.
 */
RPC_STATUS RPCRT4_default_inquire_auth_client(
    RpcConnection *conn, RPC_AUTHZ_HANDLE *privs, RPC_WSTR *server_princ_name,
    ULONG *authn_level, ULONG *authn_svc, ULONG *authz_svc, ULONG flags)
{
    if (!conn->AuthInfo) return RPC_S_BINDING_HAS_NO_AUTH;

    if (privs)
    {
        FIXME("privs not implemented\n");
        *privs = NULL;
    }
    if (server_princ_name)
    {
        *server_princ_name = wcsdup(conn->AuthInfo->server_principal_name);
        if (!*server_princ_name) return ERROR_OUTOFMEMORY;
    }
    if (authn_level) *authn_level = conn->AuthInfo->AuthnLevel;
    if (authn_svc) *authn_svc = conn->AuthInfo->AuthnSvc;
    if (authz_svc)
    {
        FIXME("authorization service not implemented\n");
        *authz_svc = RPC_C_AUTHZ_NONE;
    }
    if (flags)
        FIXME("flags 0x%lx not implemented\n", flags);

    return RPC_S_OK;
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

  if (packet_does_auth_negotiation(Header) &&
      Connection->AuthInfo &&
      !rpcrt4_conn_is_authorized(Connection))
  {
      unsigned int out_size = 0;
      unsigned char *out_buffer;

      r = rpcrt4_conn_authorize(Connection, TRUE, NULL, 0, NULL, &out_size);
      if (r != RPC_S_OK) return r;

      out_buffer = malloc(out_size);
      if (!out_buffer) return RPC_S_OUT_OF_RESOURCES;

      /* tack on a negotiate packet */
      r = rpcrt4_conn_authorize(Connection, TRUE, NULL, 0, out_buffer, &out_size);
      if (r == RPC_S_OK)
          r = RPCRT4_SendWithAuth(Connection, Header, Buffer, BufferLength, out_buffer, out_size);

      free(out_buffer);
  }
  else
    r = RPCRT4_SendWithAuth(Connection, Header, Buffer, BufferLength, NULL, 0);

  return r;
}

/* validates version and frag_len fields */
RPC_STATUS RPCRT4_ValidateCommonHeader(const RpcPktCommonHdr *hdr)
{
  DWORD hdr_length;

  /* verify if the header really makes sense */
  if (hdr->rpc_ver != RPC_VER_MAJOR ||
      hdr->rpc_ver_minor != RPC_VER_MINOR)
  {
    WARN("unhandled packet version\n");
    return RPC_S_PROTOCOL_ERROR;
  }

  hdr_length = RPCRT4_GetHeaderSize((const RpcPktHdr*)hdr);
  if (hdr_length == 0)
  {
    WARN("header length == 0\n");
    return RPC_S_PROTOCOL_ERROR;
  }

  if (hdr->frag_len < hdr_length)
  {
    WARN("bad frag length %d\n", hdr->frag_len);
    return RPC_S_PROTOCOL_ERROR;
  }

  return RPC_S_OK;
}

/***********************************************************************
 *           RPCRT4_default_receive_fragment (internal)
 * 
 * Receive a fragment from a connection.
 */
static RPC_STATUS RPCRT4_default_receive_fragment(RpcConnection *Connection, RpcPktHdr **Header, void **Payload)
{
  RPC_STATUS status;
  DWORD hdr_length;
  LONG dwRead;
  RpcPktCommonHdr common_hdr;

  *Header = NULL;
  *Payload = NULL;

  TRACE("(%p, %p, %p)\n", Connection, Header, Payload);

  /* read packet common header */
  dwRead = rpcrt4_conn_read(Connection, &common_hdr, sizeof(common_hdr));
  if (dwRead != sizeof(common_hdr)) {
    WARN("Short read of header, %ld bytes\n", dwRead);
    status = RPC_S_CALL_FAILED;
    goto fail;
  }

  status = RPCRT4_ValidateCommonHeader(&common_hdr);
  if (status != RPC_S_OK) goto fail;

  hdr_length = RPCRT4_GetHeaderSize((RpcPktHdr*)&common_hdr);
  if (hdr_length == 0) {
    WARN("header length == 0\n");
    status = RPC_S_PROTOCOL_ERROR;
    goto fail;
  }

  *Header = malloc(hdr_length);
  memcpy(*Header, &common_hdr, sizeof(common_hdr));

  /* read the rest of packet header */
  dwRead = rpcrt4_conn_read(Connection, &(*Header)->common + 1, hdr_length - sizeof(common_hdr));
  if (dwRead != hdr_length - sizeof(common_hdr)) {
    WARN("bad header length, %ld bytes, hdr_length %ld\n", dwRead, hdr_length);
    status = RPC_S_CALL_FAILED;
    goto fail;
  }

  if (common_hdr.frag_len - hdr_length)
  {
    *Payload = malloc(common_hdr.frag_len - hdr_length);
    if (!*Payload)
    {
      status = RPC_S_OUT_OF_RESOURCES;
      goto fail;
    }

    dwRead = rpcrt4_conn_read(Connection, *Payload, common_hdr.frag_len - hdr_length);
    if (dwRead != common_hdr.frag_len - hdr_length)
    {
      WARN("bad data length, %ld/%ld\n", dwRead, common_hdr.frag_len - hdr_length);
      status = RPC_S_CALL_FAILED;
      goto fail;
    }
  }
  else
    *Payload = NULL;

  /* success */
  status = RPC_S_OK;

fail:
  if (status != RPC_S_OK) {
    free(*Header);
    *Header = NULL;
    free(*Payload);
    *Payload = NULL;
  }
  return status;
}

static RPC_STATUS RPCRT4_receive_fragment(RpcConnection *Connection, RpcPktHdr **Header, void **Payload)
{
    if (Connection->ops->receive_fragment)
        return Connection->ops->receive_fragment(Connection, Header, Payload);
    else
        return RPCRT4_default_receive_fragment(Connection, Header, Payload);
}

/***********************************************************************
 *           RPCRT4_ReceiveWithAuth (internal)
 *
 * Receive a packet from connection, merge the fragments and return the auth
 * data.
 */
RPC_STATUS RPCRT4_ReceiveWithAuth(RpcConnection *Connection, RpcPktHdr **Header,
                                  PRPC_MESSAGE pMsg,
                                  unsigned char **auth_data_out,
                                  ULONG *auth_length_out)
{
  RPC_STATUS status;
  DWORD hdr_length;
  unsigned short first_flag;
  ULONG data_length;
  ULONG buffer_length;
  ULONG auth_length = 0;
  unsigned char *auth_data = NULL;
  RpcPktHdr *CurrentHeader = NULL;
  void *payload = NULL;

  *Header = NULL;
  pMsg->Buffer = NULL;
  if (auth_data_out) *auth_data_out = NULL;
  if (auth_length_out) *auth_length_out = 0;

  TRACE("(%p, %p, %p, %p)\n", Connection, Header, pMsg, auth_data_out);

  RPCRT4_SetThreadCurrentConnection(Connection);

  status = RPCRT4_receive_fragment(Connection, Header, &payload);
  if (status != RPC_S_OK) goto fail;

  hdr_length = RPCRT4_GetHeaderSize(*Header);

  /* read packet body */
  switch ((*Header)->common.ptype) {
  case PKT_RESPONSE:
    pMsg->BufferLength = (*Header)->response.alloc_hint;
    break;
  case PKT_REQUEST:
    pMsg->BufferLength = (*Header)->request.alloc_hint;
    break;
  default:
    pMsg->BufferLength = (*Header)->common.frag_len - hdr_length - RPC_AUTH_VERIFIER_LEN(&(*Header)->common);
  }

  TRACE("buffer length = %u\n", pMsg->BufferLength);

  pMsg->Buffer = I_RpcAllocate(pMsg->BufferLength);
  if (!pMsg->Buffer)
  {
    status = ERROR_OUTOFMEMORY;
    goto fail;
  }

  first_flag = RPC_FLG_FIRST;
  auth_length = (*Header)->common.auth_len;
  if (auth_length) {
    auth_data = malloc(RPC_AUTH_VERIFIER_LEN(&(*Header)->common));
    if (!auth_data) {
      status = RPC_S_OUT_OF_RESOURCES;
      goto fail;
    }
  }
  CurrentHeader = *Header;
  buffer_length = 0;
  while (TRUE)
  {
    unsigned int header_auth_len = RPC_AUTH_VERIFIER_LEN(&CurrentHeader->common);

    /* verify header fields */

    if ((CurrentHeader->common.frag_len < hdr_length) ||
        (CurrentHeader->common.frag_len - hdr_length < header_auth_len)) {
      WARN("frag_len %d too small for hdr_length %ld and auth_len %d\n",
        CurrentHeader->common.frag_len, hdr_length, CurrentHeader->common.auth_len);
      status = RPC_S_PROTOCOL_ERROR;
      goto fail;
    }

    if (CurrentHeader->common.auth_len != auth_length) {
      WARN("auth_len header field changed from %ld to %d\n",
        auth_length, CurrentHeader->common.auth_len);
      status = RPC_S_PROTOCOL_ERROR;
      goto fail;
    }

    if ((CurrentHeader->common.flags & RPC_FLG_FIRST) != first_flag) {
      TRACE("invalid packet flags\n");
      status = RPC_S_PROTOCOL_ERROR;
      goto fail;
    }

    data_length = CurrentHeader->common.frag_len - hdr_length - header_auth_len;
    if (data_length + buffer_length > pMsg->BufferLength) {
      TRACE("allocation hint exceeded, new buffer length = %ld\n",
        data_length + buffer_length);
      pMsg->BufferLength = data_length + buffer_length;
      status = I_RpcReAllocateBuffer(pMsg);
      if (status != RPC_S_OK) goto fail;
    }

    memcpy((unsigned char *)pMsg->Buffer + buffer_length, payload, data_length);

    if (header_auth_len) {
      if (header_auth_len < sizeof(RpcAuthVerifier) ||
          header_auth_len > RPC_AUTH_VERIFIER_LEN(&(*Header)->common)) {
        WARN("bad auth verifier length %d\n", header_auth_len);
        status = RPC_S_PROTOCOL_ERROR;
        goto fail;
      }

      /* FIXME: we should accumulate authentication data for the bind,
       * bind_ack, alter_context and alter_context_response if necessary.
       * however, the details of how this is done is very sketchy in the
       * DCE/RPC spec. for all other packet types that have authentication
       * verifier data then it is just duplicated in all the fragments */
      memcpy(auth_data, (unsigned char *)payload + data_length, header_auth_len);

      /* these packets are handled specially, not by the generic SecurePacket
       * function */
      if (!packet_does_auth_negotiation(*Header) && rpcrt4_conn_is_authorized(Connection))
      {
        status = rpcrt4_conn_secure_packet(Connection, SECURE_PACKET_RECEIVE,
            CurrentHeader, hdr_length,
            (unsigned char *)pMsg->Buffer + buffer_length, data_length,
            (RpcAuthVerifier *)auth_data,
            auth_data + sizeof(RpcAuthVerifier),
            header_auth_len - sizeof(RpcAuthVerifier));
        if (status != RPC_S_OK) goto fail;
      }
    }

    buffer_length += data_length;
    if (!(CurrentHeader->common.flags & RPC_FLG_LAST)) {
      TRACE("next header\n");

      if (*Header != CurrentHeader)
      {
          free(CurrentHeader);
          CurrentHeader = NULL;
      }
      free(payload);
      payload = NULL;

      status = RPCRT4_receive_fragment(Connection, &CurrentHeader, &payload);
      if (status != RPC_S_OK) goto fail;

      first_flag = 0;
    } else {
      break;
    }
  }
  pMsg->BufferLength = buffer_length;

  /* success */
  status = RPC_S_OK;

fail:
  RPCRT4_SetThreadCurrentConnection(NULL);
  if (CurrentHeader != *Header)
    free(CurrentHeader);
  if (status != RPC_S_OK) {
    I_RpcFree(pMsg->Buffer);
    pMsg->Buffer = NULL;
    free(*Header);
    *Header = NULL;
  }
  if (auth_data_out && status == RPC_S_OK) {
    *auth_length_out = auth_length;
    *auth_data_out = auth_data;
  }
  else
    free(auth_data);
  free(payload);
  return status;
}

/***********************************************************************
 *           RPCRT4_Receive (internal)
 *
 * Receive a packet from connection and merge the fragments.
 */
static RPC_STATUS RPCRT4_Receive(RpcConnection *Connection, RpcPktHdr **Header,
                                 PRPC_MESSAGE pMsg)
{
    return RPCRT4_ReceiveWithAuth(Connection, Header, pMsg, NULL, NULL);
}

/***********************************************************************
 *           I_RpcNegotiateTransferSyntax [RPCRT4.@]
 *
 * Negotiates the transfer syntax used by a client connection by connecting
 * to the server.
 *
 * PARAMS
 *  pMsg   [I] RPC Message structure.
 *  pAsync [I] Asynchronous state to set.
 *
 * RETURNS
 *  Success: RPC_S_OK.
 *  Failure: Any error code.
 */
RPC_STATUS WINAPI I_RpcNegotiateTransferSyntax(PRPC_MESSAGE pMsg)
{
  RpcBinding* bind = pMsg->Handle;
  RpcConnection* conn;
  RPC_STATUS status = RPC_S_OK;

  TRACE("(%p)\n", pMsg);

  if (!bind || bind->server)
  {
    ERR("no binding\n");
    return RPC_S_INVALID_BINDING;
  }

  /* if we already have a connection, we don't need to negotiate again */
  if (!pMsg->ReservedForRuntime)
  {
    RPC_CLIENT_INTERFACE *cif = pMsg->RpcInterfaceInformation;
    if (!cif) return RPC_S_INTERFACE_NOT_FOUND;

    if (!bind->Endpoint || !bind->Endpoint[0])
    {
      TRACE("automatically resolving partially bound binding\n");
      status = RpcEpResolveBinding(bind, cif);
      if (status != RPC_S_OK) return status;
    }

    status = RPCRT4_OpenBinding(bind, &conn, &cif->TransferSyntax,
                                &cif->InterfaceId, NULL);

    if (status == RPC_S_OK)
    {
      pMsg->ReservedForRuntime = conn;
      RPCRT4_AddRefBinding(bind);
    }
  }

  return status;
}

/***********************************************************************
 *           I_RpcGetBuffer [RPCRT4.@]
 *
 * Allocates a buffer for use by I_RpcSend or I_RpcSendReceive and binds to the
 * server interface.
 *
 * PARAMS
 *  pMsg [I/O] RPC message information.
 *
 * RETURNS
 *  Success: RPC_S_OK.
 *  Failure: RPC_S_INVALID_BINDING if pMsg->Handle is invalid.
 *           RPC_S_SERVER_UNAVAILABLE if unable to connect to server.
 *           ERROR_OUTOFMEMORY if buffer allocation failed.
 *
 * NOTES
 *  The pMsg->BufferLength field determines the size of the buffer to allocate,
 *  in bytes.
 *
 *  Use I_RpcFreeBuffer() to unbind from the server and free the message buffer.
 *
 * SEE ALSO
 *  I_RpcFreeBuffer(), I_RpcSend(), I_RpcReceive(), I_RpcSendReceive().
 */
RPC_STATUS WINAPI I_RpcGetBuffer(PRPC_MESSAGE pMsg)
{
  RPC_STATUS status;
  RpcBinding* bind = pMsg->Handle;

  TRACE("(%p): BufferLength=%d\n", pMsg, pMsg->BufferLength);

  if (!bind)
  {
    WARN("no binding\n");
    return RPC_S_INVALID_BINDING;
  }

  pMsg->Buffer = I_RpcAllocate(pMsg->BufferLength);
  TRACE("Buffer=%p\n", pMsg->Buffer);

  if (!pMsg->Buffer)
    return ERROR_OUTOFMEMORY;

  if (!bind->server)
  {
    status = I_RpcNegotiateTransferSyntax(pMsg);
    if (status != RPC_S_OK)
      I_RpcFree(pMsg->Buffer);
  }
  else
    status = RPC_S_OK;

  return status;
}

/***********************************************************************
 *           I_RpcReAllocateBuffer (internal)
 */
static RPC_STATUS I_RpcReAllocateBuffer(PRPC_MESSAGE pMsg)
{
  TRACE("(%p): BufferLength=%d\n", pMsg, pMsg->BufferLength);
  pMsg->Buffer = realloc(pMsg->Buffer, pMsg->BufferLength);

  TRACE("Buffer=%p\n", pMsg->Buffer);
  return pMsg->Buffer ? RPC_S_OK : ERROR_OUTOFMEMORY;
}

/***********************************************************************
 *           I_RpcFreeBuffer [RPCRT4.@]
 *
 * Frees a buffer allocated by I_RpcGetBuffer or I_RpcReceive and unbinds from
 * the server interface.
 *
 * PARAMS
 *  pMsg [I/O] RPC message information.
 *
 * RETURNS
 *  RPC_S_OK.
 *
 * SEE ALSO
 *  I_RpcGetBuffer(), I_RpcReceive().
 */
RPC_STATUS WINAPI I_RpcFreeBuffer(PRPC_MESSAGE pMsg)
{
  RpcBinding* bind = pMsg->Handle;

  TRACE("(%p) Buffer=%p\n", pMsg, pMsg->Buffer);

  if (!bind)
  {
    ERR("no binding\n");
    return RPC_S_INVALID_BINDING;
  }

  if (pMsg->ReservedForRuntime)
  {
    RpcConnection *conn = pMsg->ReservedForRuntime;
    RPCRT4_CloseBinding(bind, conn);
    RPCRT4_ReleaseBinding(bind);
    pMsg->ReservedForRuntime = NULL;
  }
  I_RpcFree(pMsg->Buffer);
  return RPC_S_OK;
}

static void CALLBACK async_apc_notifier_proc(ULONG_PTR ulParam)
{
    RPC_ASYNC_STATE *state = (RPC_ASYNC_STATE *)ulParam;
    state->u.APC.NotificationRoutine(state, NULL, state->Event);
}

static DWORD WINAPI async_notifier_proc(LPVOID p)
{
    RpcConnection *conn = p;
    RPC_ASYNC_STATE *state = conn->async_state;

    if (state && conn->ops->wait_for_incoming_data(conn) != -1)
    {
        state->Event = RpcCallComplete;
        switch (state->NotificationType)
        {
        case RpcNotificationTypeEvent:
            TRACE("RpcNotificationTypeEvent %p\n", state->u.hEvent);
            SetEvent(state->u.hEvent);
            break;
        case RpcNotificationTypeApc:
            TRACE("RpcNotificationTypeApc %p\n", state->u.APC.hThread);
            QueueUserAPC(async_apc_notifier_proc, state->u.APC.hThread, (ULONG_PTR)state);
            break;
        case RpcNotificationTypeIoc:
            TRACE("RpcNotificationTypeIoc %p, 0x%lx, 0x%Ix, %p\n",
                state->u.IOC.hIOPort, state->u.IOC.dwNumberOfBytesTransferred,
                state->u.IOC.dwCompletionKey, state->u.IOC.lpOverlapped);
            PostQueuedCompletionStatus(state->u.IOC.hIOPort,
                state->u.IOC.dwNumberOfBytesTransferred,
                state->u.IOC.dwCompletionKey,
                state->u.IOC.lpOverlapped);
            break;
        case RpcNotificationTypeHwnd:
            TRACE("RpcNotificationTypeHwnd %p 0x%x\n", state->u.HWND.hWnd,
                state->u.HWND.Msg);
            PostMessageW(state->u.HWND.hWnd, state->u.HWND.Msg, 0, 0);
            break;
        case RpcNotificationTypeCallback:
            TRACE("RpcNotificationTypeCallback %p\n", state->u.NotificationRoutine);
            state->u.NotificationRoutine(state, NULL, state->Event);
            break;
        case RpcNotificationTypeNone:
            TRACE("RpcNotificationTypeNone\n");
            break;
        default:
            FIXME("unknown NotificationType: %d/0x%x\n", state->NotificationType, state->NotificationType);
            break;
        }
    }

    return 0;
}

/***********************************************************************
 *           I_RpcSend [RPCRT4.@]
 *
 * Sends a message to the server.
 *
 * PARAMS
 *  pMsg [I/O] RPC message information.
 *
 * RETURNS
 *  Unknown.
 *
 * NOTES
 *  The buffer must have been allocated with I_RpcGetBuffer().
 *
 * SEE ALSO
 *  I_RpcGetBuffer(), I_RpcReceive(), I_RpcSendReceive().
 */
RPC_STATUS WINAPI I_RpcSend(PRPC_MESSAGE pMsg)
{
  RpcBinding* bind = pMsg->Handle;
  RPC_CLIENT_INTERFACE *cif;
  RpcConnection* conn;
  RPC_STATUS status;
  RpcPktHdr *hdr;
  BOOL from_cache = TRUE;

  TRACE("(%p)\n", pMsg);
  if (!bind || bind->server || !pMsg->ReservedForRuntime) return RPC_S_INVALID_BINDING;

  for (;;)
  {
      conn = pMsg->ReservedForRuntime;
      hdr = RPCRT4_BuildRequestHeader(pMsg->DataRepresentation,
                                      pMsg->BufferLength,
                                      pMsg->ProcNum & ~RPC_FLAGS_VALID_BIT,
                                      &bind->ObjectUuid);
      if (!hdr)
          return ERROR_OUTOFMEMORY;

      hdr->common.call_id = conn->NextCallId++;
      status = RPCRT4_Send(conn, hdr, pMsg->Buffer, pMsg->BufferLength);
      free(hdr);
      if (status == RPC_S_OK || conn->server || !from_cache)
          break;

      WARN("Send failed, trying to reconnect\n");
      cif = pMsg->RpcInterfaceInformation;
      RPCRT4_ReleaseConnection(conn);
      pMsg->ReservedForRuntime = NULL;
      status = RPCRT4_OpenBinding(bind, &conn, &cif->TransferSyntax, &cif->InterfaceId, &from_cache);
      if (status != RPC_S_OK) break;
      pMsg->ReservedForRuntime = conn;
  }

  if (status == RPC_S_OK && pMsg->RpcFlags & RPC_BUFFER_ASYNC)
  {
    if (!QueueUserWorkItem(async_notifier_proc, conn, WT_EXECUTEDEFAULT | WT_EXECUTELONGFUNCTION))
        status = RPC_S_OUT_OF_RESOURCES;
  }

  return status;
}

/* is this status something that the server can't recover from? */
static inline BOOL is_hard_error(RPC_STATUS status)
{
    switch (status)
    {
    case 0: /* user-defined fault */
    case ERROR_ACCESS_DENIED:
    case ERROR_INVALID_PARAMETER:
    case RPC_S_PROTOCOL_ERROR:
    case RPC_S_CALL_FAILED:
    case RPC_S_CALL_FAILED_DNE:
    case RPC_S_SEC_PKG_ERROR:
        return TRUE;
    default:
        return FALSE;
    }
}

/***********************************************************************
 *           I_RpcReceive [RPCRT4.@]
 */
RPC_STATUS WINAPI I_RpcReceive(PRPC_MESSAGE pMsg)
{
  RpcBinding* bind = pMsg->Handle;
  RPC_STATUS status;
  RpcPktHdr *hdr = NULL;
  RpcConnection *conn;

  TRACE("(%p)\n", pMsg);
  if (!bind || bind->server || !pMsg->ReservedForRuntime) return RPC_S_INVALID_BINDING;

  conn = pMsg->ReservedForRuntime;
  status = RPCRT4_Receive(conn, &hdr, pMsg);
  if (status != RPC_S_OK) {
    WARN("receive failed with error %lx\n", status);
    goto fail;
  }

  switch (hdr->common.ptype) {
  case PKT_RESPONSE:
    break;
  case PKT_FAULT:
    ERR ("we got fault packet with status 0x%x\n", hdr->fault.status);
    status = NCA2RPC_STATUS(hdr->fault.status);
    if (is_hard_error(status))
        goto fail;
    break;
  default:
    WARN("bad packet type %d\n", hdr->common.ptype);
    status = RPC_S_PROTOCOL_ERROR;
    goto fail;
  }

  /* success */
  free(hdr);
  return status;

fail:
  free(hdr);
  RPCRT4_ReleaseConnection(conn);
  pMsg->ReservedForRuntime = NULL;
  return status;
}

/***********************************************************************
 *           I_RpcSendReceive [RPCRT4.@]
 *
 * Sends a message to the server and receives the response.
 *
 * PARAMS
 *  pMsg [I/O] RPC message information.
 *
 * RETURNS
 *  Success: RPC_S_OK.
 *  Failure: Any error code.
 *
 * NOTES
 *  The buffer must have been allocated with I_RpcGetBuffer().
 *
 * SEE ALSO
 *  I_RpcGetBuffer(), I_RpcSend(), I_RpcReceive().
 */
RPC_STATUS WINAPI I_RpcSendReceive(PRPC_MESSAGE pMsg)
{
  RPC_STATUS status;
  void *original_buffer;

  TRACE("(%p)\n", pMsg);

  original_buffer = pMsg->Buffer;
  status = I_RpcSend(pMsg);
  if (status == RPC_S_OK)
    status = I_RpcReceive(pMsg);
  /* free the buffer replaced by a new buffer in I_RpcReceive */
  if (status == RPC_S_OK)
    I_RpcFree(original_buffer);
  return status;
}

/***********************************************************************
 *           I_RpcAsyncSetHandle [RPCRT4.@]
 *
 * Sets the asynchronous state of the handle contained in the RPC message
 * structure.
 *
 * PARAMS
 *  pMsg   [I] RPC Message structure.
 *  pAsync [I] Asynchronous state to set.
 *
 * RETURNS
 *  Success: RPC_S_OK.
 *  Failure: Any error code.
 */
RPC_STATUS WINAPI I_RpcAsyncSetHandle(PRPC_MESSAGE pMsg, PRPC_ASYNC_STATE pAsync)
{
    RpcBinding* bind = pMsg->Handle;
    RpcConnection *conn;

    TRACE("(%p, %p)\n", pMsg, pAsync);

    if (!bind || bind->server || !pMsg->ReservedForRuntime) return RPC_S_INVALID_BINDING;

    conn = pMsg->ReservedForRuntime;
    conn->async_state = pAsync;

    return RPC_S_OK;
}

/***********************************************************************
 *           I_RpcAsyncAbortCall [RPCRT4.@]
 *
 * Aborts an asynchronous call.
 *
 * PARAMS
 *  pAsync        [I] Asynchronous state.
 *  ExceptionCode [I] Exception code.
 *
 * RETURNS
 *  Success: RPC_S_OK.
 *  Failure: Any error code.
 */
RPC_STATUS WINAPI I_RpcAsyncAbortCall(PRPC_ASYNC_STATE pAsync, ULONG ExceptionCode)
{
    FIXME("(%p, %ld): stub\n", pAsync, ExceptionCode);
    return RPC_S_INVALID_ASYNC_HANDLE;
}
